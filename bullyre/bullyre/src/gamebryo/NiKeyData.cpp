// The key data objects of src/gamebryo/NiInterpolators.h: NiTransformData (vtable 0x966718, 0x38
// bytes: rotation, position and scale keys), NiRotData (vtable 0x965E80, 0x18) and NiPosData's
// pieces here (vtable 0x965B08, 0x18; its GetRTTI and destructor slot are the key chunk's).
// Each channel is a key array with its type and key size, handled through the key classes'
// function tables (src/gamebryo/NiAnimationKey.h). Setting and replacing the arrays, the
// sequence copies (a deep copy with the keys cut to a span), the keys guaranteed at both ends,
// streaming, comparison, viewer strings.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiInterpolators.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni::anim {
namespace {

using scene::AddViewerString;
using scene::GetViewerString;
using scene::GetViewerStringUint;
using scene::LoadBytes;
using scene::Object_GetViewerStrings;
using scene::Object_IsEqual;
using scene::Object_LoadBinary;
using scene::RefObject_Construct;
using scene::SaveBytes;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiTransformData_RTTI, 0x020E4EC4);
GAME_VAR(RTTI, NiRotData_RTTI, 0x020E4BDC);
GAME_VAR(RTTI, NiPosData_RTTI, 0x020E4C78);

constexpr uintptr_t NiTransformData_vtable = 0x00966718;
constexpr uintptr_t NiRotData_vtable = 0x00965E80;
constexpr uintptr_t NiPosData_vtable = 0x00965B08;

GAME_FN(void, __thiscall, Object_Destruct, 0x007769D0, void* self);
GAME_FN(void, __thiscall, Object_SaveBinaryNone, 0x008A20A0, void* self, void* stream);
// NiObject::CreateDeepCopy (streamed out and back in)
GAME_FN(re::Ptr32<NiObject>*, __thiscall, Object_CreateDeepCopy, 0x00755430, void* self, re::Ptr32<NiObject>* result);
// the key classes' statics: keys made at the span's ends; the keys within a span copied
GAME_FN(void, __cdecl, Key_GuaranteeKeysAtStartAndEnd, 0x007FFAF0, KeyContent content, KeyType type,
        re::Ptr32<NiAnimationKey>* keys, uint32_t* numKeys, float start, float end);
GAME_FN(void, __cdecl, Key_CopySequence, 0x007FF7D0, KeyContent content, KeyType type, NiAnimationKey* keys,
        uint32_t numKeys, float start, float end, re::Ptr32<NiAnimationKey>* newKeys, uint32_t* newNumKeys);
// NiEulerRotKey's three sequences let go
GAME_FN(void, __thiscall, EulerRotKey_Cleanup, 0x007FB1F0, NiAnimationKey* key);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, TransformData_Destruct, 0x0080FB00, NiTransformData* self);
GAME_FN(void, __thiscall, TransformData_ReplaceRotAnim, 0x0080FB70, NiTransformData* self, NiAnimationKey* keys,
        uint32_t numKeys, KeyType type);
GAME_FN(void, __thiscall, TransformData_ReplacePosAnim, 0x0080FBE0, NiTransformData* self, NiAnimationKey* keys,
        uint32_t numKeys, KeyType type);
GAME_FN(void, __thiscall, TransformData_ReplaceScaleAnim, 0x0080FC40, NiTransformData* self, NiAnimationKey* keys,
        uint32_t numKeys, KeyType type);
GAME_FN(void, __thiscall, RotData_ReplaceAnim, 0x008073F0, NiRotData* self, NiAnimationKey* keys, uint32_t numKeys,
        KeyType type);
GAME_FN(void, __thiscall, PosData_ReplaceAnim, 0x00808250, NiPosData* self, NiAnimationKey* keys, uint32_t numKeys,
        KeyType type);

// a channel set from new keys (none when either is missing)
inline void SetChannel(KeyChannel& c, NiAnimationKey* keys, uint32_t numKeys, KeyType type, KeyContent content) {
    if (keys != nullptr && numKeys != 0) {
        c.numKeys = numKeys;
        c.keys = keys;
        c.type = type;
        c.size = ms_keySizes[KeyIndex(content, type)];
        return;
    }
    c.numKeys = 0;
    c.keys = nullptr;
    c.type = NOINTERP;
    c.size = 0;
}

// a channel's keys let go through its key table
inline void DeleteKeys(const KeyChannel& c, KeyContent content) {
    ms_delete[KeyIndex(content, c.type)](c.keys);
}

// the keys made at both ends of a channel (its array may be replaced)
inline void GuaranteeChannel(KeyChannel& c, KeyContent content, float start, float end) {
    if (c.numKeys != 0) {
        re::Ptr32<NiAnimationKey> keys = c.keys;
        Key_GuaranteeKeysAtStartAndEnd(content, c.type, &keys, &c.numKeys, start, end);
        c.keys = keys;
    }
}

// a channel's keys compared one by one through the key table
inline uint8_t ChannelKeysEqual(const KeyChannel& a, const KeyChannel& b, KeyContent content) {
    EqualFunction equal = ms_equal[KeyIndex(content, a.type)];
    for (uint32_t i = 0; i < a.numKeys; i++) {
        uint32_t offset = a.size * i;
        const NiAnimationKey* key0 = reinterpret_cast<const NiAnimationKey*>(re::PtrCast<uintptr_t>(a.keys) + offset);
        const NiAnimationKey* key1 = reinterpret_cast<const NiAnimationKey*>(re::PtrCast<uintptr_t>(b.keys) + offset);
        if (!equal(key0, key1)) {
            return 0;
        }
    }
    return 1;
}

// a channel read: the count, then (when there are keys) the type, the keys made by the key table
// and their derived values filled; the replace function takes them
template <typename Replace, typename Self>
inline void LoadChannel(void* stream, KeyContent content, Self* self, Replace replace) {
    uint32_t numKeys;
    LoadBytes(stream, &numKeys, 4);
    if (numKeys != 0) {
        KeyType type;
        LoadBytes(stream, &type, 4);
        uint8_t size = ms_keySizes[KeyIndex(content, type)];
        NiAnimationKey* keys = ms_create[KeyIndex(content, type)](stream, numKeys);
        ms_fillDerived[KeyIndex(content, type)](keys, numKeys, size);
        replace(self, keys, numKeys, type);
    }
}

// a channel written: the count, then (when there are keys) the type and the keys
inline void SaveChannel(void* stream, KeyChannel& c, KeyContent content) {
    SaveBytes(stream, &c.numKeys, 4);
    if (c.numKeys != 0) {
        SaveBytes(stream, &c.type, 4);
        ms_save[KeyIndex(content, c.type)](stream, c.keys, c.numKeys);
    }
}

// ---- NiTransformData

// 0x80F840: GuaranteeKeysAtStartAndEnd(start, end)
void __fastcall TransformData_GuaranteeKeysAtStartAndEnd(NiTransformData* self, re::Edx, float start, float end) {
    GuaranteeChannel(self->rot, ROTKEY, start, end);
    GuaranteeChannel(self->pos, POSKEY, start, end);
    GuaranteeChannel(self->scale, FLOATKEY, start, end);
}

// 0x80F900: CreateObject
NiObject* __cdecl TransformData_CreateObject() {
    void* memory = NiNew(0x38);
    if (memory == nullptr) {
        return nullptr;
    }
    NiTransformData* self = static_cast<NiTransformData*>(memory);
    RefObject_Construct(self);
    self->rot.numKeys = 0;
    self->rot.keys = nullptr;
    self->rot.type = NOINTERP;
    self->rot.size = 0;
    self->pos.numKeys = 0;
    self->pos.keys = nullptr;
    self->pos.type = NOINTERP;
    self->pos.size = 0;
    self->scale.numKeys = 0;
    self->scale.keys = nullptr;
    self->scale.type = NOINTERP;
    self->scale.size = 0;
    self->vtable = reinterpret_cast<void*>(NiTransformData_vtable);
    return self;
}

const RTTI* __fastcall TransformData_GetRTTI(NiTransformData*, re::Edx) {
    return &NiTransformData_RTTI;
}

// 0x80F960: SetPosAnim(keys, count, type) (the old keys not let go)
void __fastcall TransformData_SetPosAnim(NiTransformData* self, re::Edx, NiAnimationKey* keys, uint32_t numKeys,
                                         KeyType type) {
    SetChannel(self->pos, keys, numKeys, type, POSKEY);
}

// 0x80F9A0: SetScaleAnim(keys, count, type)
void __fastcall TransformData_SetScaleAnim(NiTransformData* self, re::Edx, NiAnimationKey* keys, uint32_t numKeys,
                                           KeyType type) {
    SetChannel(self->scale, keys, numKeys, type, FLOATKEY);
}

// 0x80F9E0: IsEqual (the counts, types and key sizes, then every key)
uint8_t __fastcall TransformData_IsEqual(NiTransformData* self, re::Edx, NiTransformData* other) {
    if (!Object_IsEqual(self, other)) {
        return 0;
    }
    if (self->rot.numKeys != other->rot.numKeys || self->rot.type != other->rot.type ||
        self->pos.numKeys != other->pos.numKeys || self->pos.type != other->pos.type ||
        self->scale.numKeys != other->scale.numKeys || self->scale.type != other->scale.type ||
        self->rot.size != other->rot.size || self->pos.size != other->pos.size ||
        self->scale.size != other->scale.size) {
        return 0;
    }
    if (!ChannelKeysEqual(self->rot, other->rot, ROTKEY)) {
        return 0;
    }
    if (!ChannelKeysEqual(self->pos, other->pos, POSKEY)) {
        return 0;
    }
    if (!ChannelKeysEqual(self->scale, other->scale, FLOATKEY)) {
        return 0;
    }
    return 1;
}

// 0x80FB00: ~NiTransformData (an Euler rotation key's sequences let go first)
void __fastcall TransformData_DestructImpl(NiTransformData* self, re::Edx) {
    NiAnimationKey* rotKeys = self->rot.keys;
    self->vtable = reinterpret_cast<void*>(NiTransformData_vtable);
    if (rotKeys != nullptr) {
        if (self->rot.type == EULERKEY) {
            EulerRotKey_Cleanup(rotKeys);
        }
        ms_delete[KeyIndex(ROTKEY, self->rot.type)](rotKeys);
    }
    if (self->pos.keys != nullptr) {
        DeleteKeys(self->pos, POSKEY);
    }
    if (self->scale.keys != nullptr) {
        DeleteKeys(self->scale, FLOATKEY);
    }
    Object_Destruct(self);
}

// 0x80FB70: ReplaceRotAnim(keys, count, type) (the old keys let go)
void __fastcall TransformData_ReplaceRotAnimImpl(NiTransformData* self, re::Edx, NiAnimationKey* keys, uint32_t numKeys,
                                                 KeyType type) {
    NiAnimationKey* old = self->rot.keys;
    if (old != nullptr) {
        if (self->rot.type == EULERKEY) {
            EulerRotKey_Cleanup(old);
        }
        ms_delete[KeyIndex(ROTKEY, self->rot.type)](old);
    }
    SetChannel(self->rot, keys, numKeys, type, ROTKEY);
}

// 0x80FBE0: ReplacePosAnim(keys, count, type)
void __fastcall TransformData_ReplacePosAnimImpl(NiTransformData* self, re::Edx, NiAnimationKey* keys, uint32_t numKeys,
                                                 KeyType type) {
    if (self->pos.keys != nullptr) {
        DeleteKeys(self->pos, POSKEY);
    }
    SetChannel(self->pos, keys, numKeys, type, POSKEY);
}

// 0x80FC40: ReplaceScaleAnim(keys, count, type)
void __fastcall TransformData_ReplaceScaleAnimImpl(NiTransformData* self, re::Edx, NiAnimationKey* keys,
                                                   uint32_t numKeys, KeyType type) {
    if (self->scale.keys != nullptr) {
        DeleteKeys(self->scale, FLOATKEY);
    }
    SetChannel(self->scale, keys, numKeys, type, FLOATKEY);
}

// 0x80FCA0: GetSequenceData(start, end) (a deep copy, each channel with keys cut to the span;
// the out arrays zeroed once, before the first)
re::Ptr32<NiTransformData>* __fastcall TransformData_GetSequenceData(NiTransformData* self, re::Edx, re::Ptr32<NiTransformData>* result,
                                                           float start, float end) {
    re::Ptr32<NiObject> copy = nullptr;
    NiObject* made = *Object_CreateDeepCopy(self, &copy);
    *result = static_cast<NiTransformData*>(made);
    AddRefImport(made);
    ReleaseRef(copy);
    re::Ptr32<NiAnimationKey> newKeys = nullptr;
    uint32_t newNumKeys = 0;
    if (self->rot.numKeys != 0) {
        Key_CopySequence(ROTKEY, self->rot.type, self->rot.keys, self->rot.numKeys, start, end, &newKeys,
                         &newNumKeys);
        TransformData_ReplaceRotAnim(*result, newKeys, newNumKeys, self->rot.type);
    }
    if (self->pos.numKeys != 0) {
        Key_CopySequence(POSKEY, self->pos.type, self->pos.keys, self->pos.numKeys, start, end, &newKeys,
                         &newNumKeys);
        TransformData_ReplacePosAnim(*result, newKeys, newNumKeys, self->pos.type);
    }
    if (self->scale.numKeys != 0) {
        Key_CopySequence(FLOATKEY, self->scale.type, self->scale.keys, self->scale.numKeys, start, end, &newKeys,
                         &newNumKeys);
        TransformData_ReplaceScaleAnim(*result, newKeys, newNumKeys, self->scale.type);
    }
    return result;
}

// 0x80FDF0: LoadBinary (rotation, position, scale channels)
void __fastcall TransformData_LoadBinary(NiTransformData* self, re::Edx, void* stream) {
    Object_LoadBinary(self, stream);
    LoadChannel(stream, ROTKEY, self, TransformData_ReplaceRotAnim);
    LoadChannel(stream, POSKEY, self, TransformData_ReplacePosAnim);
    LoadChannel(stream, FLOATKEY, self, TransformData_ReplaceScaleAnim);
}

// 0x80FFD0: SaveBinary
void __fastcall TransformData_SaveBinary(NiTransformData* self, re::Edx, void* stream) {
    Object_SaveBinaryNone(self, stream);
    SaveChannel(stream, self->rot, ROTKEY);
    SaveChannel(stream, self->pos, POSKEY);
    SaveChannel(stream, self->scale, FLOATKEY);
}

// 0x810130: the deleting destructor
NiTransformData* __fastcall TransformData_DeletingDestructor(NiTransformData* self, re::Edx, uint32_t flags) {
    TransformData_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x38);
    }
    return self;
}

// 0x810150: GetViewerStrings
void __fastcall TransformData_GetViewerStrings(NiTransformData* self, re::Edx, NiTArray<char*>* strings) {
    Object_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiTransformData_RTTI.name));
    AddViewerString(strings, GetViewerStringUint("m_uiNumRotKeys", self->rot.numKeys));
    AddViewerString(strings, GetViewerStringUint("m_uiNumPosKeys", self->pos.numKeys));
    AddViewerString(strings, GetViewerStringUint("m_uiNumScaleKeys", self->scale.numKeys));
}

// ---- NiRotData

// 0x8072D0: CreateObject
NiObject* __cdecl RotData_CreateObject() {
    void* memory = NiNew(0x18);
    if (memory == nullptr) {
        return nullptr;
    }
    NiRotData* self = static_cast<NiRotData*>(memory);
    RefObject_Construct(self);
    self->rot.numKeys = 0;
    self->rot.keys = nullptr;
    self->rot.type = NOINTERP;
    self->rot.size = 0;
    self->vtable = reinterpret_cast<void*>(NiRotData_vtable);
    return self;
}

const RTTI* __fastcall RotData_GetRTTI(NiRotData*, re::Edx) {
    return &NiRotData_RTTI;
}

// 0x807330: IsEqual
uint8_t __fastcall RotData_IsEqual(NiRotData* self, re::Edx, NiRotData* other) {
    if (!Object_IsEqual(self, other) || self->rot.numKeys != other->rot.numKeys ||
        self->rot.type != other->rot.type || self->rot.size != other->rot.size) {
        return 0;
    }
    return ChannelKeysEqual(self->rot, other->rot, ROTKEY);
}

// 0x8073B0: the deleting destructor (an Euler key's sequences not let go, as the game has it)
NiRotData* __fastcall RotData_DeletingDestructor(NiRotData* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiRotData_vtable);
    if (self->rot.keys != nullptr) {
        DeleteKeys(self->rot, ROTKEY);
    }
    Object_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x18);
    }
    return self;
}

// 0x8073F0: ReplaceAnim(keys, count, type)
void __fastcall RotData_ReplaceAnimImpl(NiRotData* self, re::Edx, NiAnimationKey* keys, uint32_t numKeys, KeyType type) {
    if (self->rot.keys != nullptr) {
        DeleteKeys(self->rot, ROTKEY);
    }
    SetChannel(self->rot, keys, numKeys, type, ROTKEY);
}

// 0x807450: GetSequenceData(start, end)
re::Ptr32<NiRotData>* __fastcall RotData_GetSequenceData(NiRotData* self, re::Edx, re::Ptr32<NiRotData>* result, float start, float end) {
    re::Ptr32<NiObject> copy = nullptr;
    NiObject* made = *Object_CreateDeepCopy(self, &copy);
    *result = static_cast<NiRotData*>(made);
    AddRefImport(made);
    ReleaseRef(copy);
    if (self->rot.numKeys != 0) {
        re::Ptr32<NiAnimationKey> newKeys = nullptr;
        uint32_t newNumKeys = 0;
        Key_CopySequence(ROTKEY, self->rot.type, self->rot.keys, self->rot.numKeys, start, end, &newKeys,
                         &newNumKeys);
        RotData_ReplaceAnim(*result, newKeys, newNumKeys, self->rot.type);
    }
    return result;
}

// 0x807500: LoadBinary (the key size set before the keys are made)
void __fastcall RotData_LoadBinary(NiRotData* self, re::Edx, void* stream) {
    Object_LoadBinary(self, stream);
    uint32_t numKeys;
    LoadBytes(stream, &numKeys, 4);
    if (numKeys != 0) {
        KeyType type;
        LoadBytes(stream, &type, 4);
        self->rot.size = ms_keySizes[KeyIndex(ROTKEY, type)];
        NiAnimationKey* keys = ms_create[KeyIndex(ROTKEY, type)](stream, numKeys);
        ms_fillDerived[KeyIndex(ROTKEY, type)](keys, numKeys, self->rot.size);
        RotData_ReplaceAnim(self, keys, numKeys, type);
    }
}

// 0x8075C0: SaveBinary
void __fastcall RotData_SaveBinary(NiRotData* self, re::Edx, void* stream) {
    Object_SaveBinaryNone(self, stream);
    SaveChannel(stream, self->rot, ROTKEY);
}

// 0x807640: GetViewerStrings
void __fastcall RotData_GetViewerStrings(NiRotData* self, re::Edx, NiTArray<char*>* strings) {
    Object_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiRotData_RTTI.name));
    AddViewerString(strings, GetViewerStringUint("m_uiNumKeys", self->rot.numKeys));
}

// ---- NiPosData

// 0x8080E0: NiPosData::GuaranteeKeysAtStartAndEnd(start, end) (the caller checks for keys).
// NiQuaternionInterpolator::GuaranteeTimeRange calls this one on its NiRotData too (the same
// layout): its rotation keys then go through the position key functions, as the game has it
void __fastcall PosData_GuaranteeKeysAtStartAndEnd(NiPosData* self, re::Edx, float start, float end) {
    re::Ptr32<NiAnimationKey> keys = self->pos.keys;
    Key_GuaranteeKeysAtStartAndEnd(POSKEY, self->pos.type, &keys, &self->pos.numKeys, start, end);
    self->pos.keys = keys;
}

// 0x808120: ~NiPosData
void __fastcall PosData_Destruct(NiPosData* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiPosData_vtable);
    if (self->pos.keys != nullptr) {
        DeleteKeys(self->pos, POSKEY);
    }
    Object_Destruct(self);
}

// 0x808150: SetAnim(keys, count, type) (the old keys not let go)
void __fastcall PosData_SetAnim(NiPosData* self, re::Edx, NiAnimationKey* keys, uint32_t numKeys, KeyType type) {
    SetChannel(self->pos, keys, numKeys, type, POSKEY);
}

// 0x808190: CreateObject
NiObject* __cdecl PosData_CreateObject() {
    void* memory = NiNew(0x18);
    if (memory == nullptr) {
        return nullptr;
    }
    NiPosData* self = static_cast<NiPosData*>(memory);
    RefObject_Construct(self);
    self->pos.numKeys = 0;
    self->pos.keys = nullptr;
    self->pos.type = NOINTERP;
    self->pos.size = 0;
    self->vtable = reinterpret_cast<void*>(NiPosData_vtable);
    return self;
}

// 0x8081D0: IsEqual
uint8_t __fastcall PosData_IsEqual(NiPosData* self, re::Edx, NiPosData* other) {
    if (!Object_IsEqual(self, other) || self->pos.numKeys != other->pos.numKeys ||
        self->pos.type != other->pos.type || self->pos.size != other->pos.size) {
        return 0;
    }
    return ChannelKeysEqual(self->pos, other->pos, POSKEY);
}

// 0x808250: ReplaceAnim(keys, count, type)
void __fastcall PosData_ReplaceAnimImpl(NiPosData* self, re::Edx, NiAnimationKey* keys, uint32_t numKeys, KeyType type) {
    if (self->pos.keys != nullptr) {
        DeleteKeys(self->pos, POSKEY);
    }
    SetChannel(self->pos, keys, numKeys, type, POSKEY);
}

// 0x8082B0: GetSequenceData(start, end)
re::Ptr32<NiPosData>* __fastcall PosData_GetSequenceData(NiPosData* self, re::Edx, re::Ptr32<NiPosData>* result, float start, float end) {
    re::Ptr32<NiObject> copy = nullptr;
    NiObject* made = *Object_CreateDeepCopy(self, &copy);
    *result = static_cast<NiPosData*>(made);
    AddRefImport(made);
    ReleaseRef(copy);
    if (self->pos.numKeys != 0) {
        re::Ptr32<NiAnimationKey> newKeys = nullptr;
        uint32_t newNumKeys = 0;
        Key_CopySequence(POSKEY, self->pos.type, self->pos.keys, self->pos.numKeys, start, end, &newKeys,
                         &newNumKeys);
        PosData_ReplaceAnim(*result, newKeys, newNumKeys, self->pos.type);
    }
    return result;
}

// 0x808360: LoadBinary
void __fastcall PosData_LoadBinary(NiPosData* self, re::Edx, void* stream) {
    Object_LoadBinary(self, stream);
    uint32_t numKeys;
    LoadBytes(stream, &numKeys, 4);
    if (numKeys != 0) {
        KeyType type;
        LoadBytes(stream, &type, 4);
        self->pos.size = ms_keySizes[KeyIndex(POSKEY, type)];
        NiAnimationKey* keys = ms_create[KeyIndex(POSKEY, type)](stream, numKeys);
        ms_fillDerived[KeyIndex(POSKEY, type)](keys, numKeys, self->pos.size);
        PosData_ReplaceAnim(self, keys, numKeys, type);
    }
}

// 0x808420: SaveBinary
void __fastcall PosData_SaveBinary(NiPosData* self, re::Edx, void* stream) {
    Object_SaveBinaryNone(self, stream);
    SaveChannel(stream, self->pos, POSKEY);
}

// 0x8084A0: GetViewerStrings
void __fastcall PosData_GetViewerStrings(NiPosData* self, re::Edx, NiTArray<char*>* strings) {
    Object_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiPosData_RTTI.name));
    AddViewerString(strings, GetViewerStringUint("m_uiNumKeys", self->pos.numKeys));
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC30_KEYDATA_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

// (0x807310: a lone jmp to NiObject::RegisterStreamables 0x755230, the slot many classes share)
RE_UNHOOKED("NiObject::RegisterStreamables (thunk 0x807310)", 0x00807310);

NIC30_KEYDATA_REVERSED("NiRotData::GetRTTI", 0x00807320, &ni::anim::RotData_GetRTTI);
NIC30_KEYDATA_REVERSED("NiRotData::CreateObject", 0x008072D0, &ni::anim::RotData_CreateObject);
NIC30_KEYDATA_REVERSED("NiRotData::IsEqual", 0x00807330, &ni::anim::RotData_IsEqual);
NIC30_KEYDATA_REVERSED("NiRotData::DeletingDestructor", 0x008073B0, &ni::anim::RotData_DeletingDestructor);
NIC30_KEYDATA_REVERSED("NiRotData::ReplaceAnim", 0x008073F0, &ni::anim::RotData_ReplaceAnimImpl);
NIC30_KEYDATA_REVERSED("NiRotData::GetSequenceData", 0x00807450, &ni::anim::RotData_GetSequenceData);
NIC30_KEYDATA_REVERSED("NiRotData::LoadBinary", 0x00807500, &ni::anim::RotData_LoadBinary);
NIC30_KEYDATA_REVERSED("NiRotData::SaveBinary", 0x008075C0, &ni::anim::RotData_SaveBinary);
NIC30_KEYDATA_REVERSED("NiRotData::GetViewerStrings", 0x00807640, &ni::anim::RotData_GetViewerStrings);
NIC30_KEYDATA_REVERSED("NiPosData::GuaranteeKeysAtStartAndEnd", 0x008080E0,
                       &ni::anim::PosData_GuaranteeKeysAtStartAndEnd);
NIC30_KEYDATA_REVERSED("NiPosData::~NiPosData", 0x00808120, &ni::anim::PosData_Destruct);
NIC30_KEYDATA_REVERSED("NiPosData::SetAnim", 0x00808150, &ni::anim::PosData_SetAnim);
NIC30_KEYDATA_REVERSED("NiPosData::CreateObject", 0x00808190, &ni::anim::PosData_CreateObject);
NIC30_KEYDATA_REVERSED("NiPosData::IsEqual", 0x008081D0, &ni::anim::PosData_IsEqual);
NIC30_KEYDATA_REVERSED("NiPosData::ReplaceAnim", 0x00808250, &ni::anim::PosData_ReplaceAnimImpl);
NIC30_KEYDATA_REVERSED("NiPosData::GetSequenceData", 0x008082B0, &ni::anim::PosData_GetSequenceData);
NIC30_KEYDATA_REVERSED("NiPosData::LoadBinary", 0x00808360, &ni::anim::PosData_LoadBinary);
NIC30_KEYDATA_REVERSED("NiPosData::SaveBinary", 0x00808420, &ni::anim::PosData_SaveBinary);
NIC30_KEYDATA_REVERSED("NiPosData::GetViewerStrings", 0x008084A0, &ni::anim::PosData_GetViewerStrings);
NIC30_KEYDATA_REVERSED("NiTransformData::GuaranteeKeysAtStartAndEnd", 0x0080F840,
                       &ni::anim::TransformData_GuaranteeKeysAtStartAndEnd);
NIC30_KEYDATA_REVERSED("NiTransformData::CreateObject", 0x0080F900, &ni::anim::TransformData_CreateObject);
NIC30_KEYDATA_REVERSED("NiTransformData::GetRTTI", 0x0080F950, &ni::anim::TransformData_GetRTTI);
NIC30_KEYDATA_REVERSED("NiTransformData::SetPosAnim", 0x0080F960, &ni::anim::TransformData_SetPosAnim);
NIC30_KEYDATA_REVERSED("NiTransformData::SetScaleAnim", 0x0080F9A0, &ni::anim::TransformData_SetScaleAnim);
NIC30_KEYDATA_REVERSED("NiTransformData::IsEqual", 0x0080F9E0, &ni::anim::TransformData_IsEqual);
NIC30_KEYDATA_REVERSED("NiTransformData::~NiTransformData", 0x0080FB00, &ni::anim::TransformData_DestructImpl);
NIC30_KEYDATA_REVERSED("NiTransformData::ReplaceRotAnim", 0x0080FB70, &ni::anim::TransformData_ReplaceRotAnimImpl);
NIC30_KEYDATA_REVERSED("NiTransformData::ReplacePosAnim", 0x0080FBE0, &ni::anim::TransformData_ReplacePosAnimImpl);
NIC30_KEYDATA_REVERSED("NiTransformData::ReplaceScaleAnim", 0x0080FC40,
                       &ni::anim::TransformData_ReplaceScaleAnimImpl);
NIC30_KEYDATA_REVERSED("NiTransformData::GetSequenceData", 0x0080FCA0, &ni::anim::TransformData_GetSequenceData);
NIC30_KEYDATA_REVERSED("NiTransformData::LoadBinary", 0x0080FDF0, &ni::anim::TransformData_LoadBinary);
NIC30_KEYDATA_REVERSED("NiTransformData::SaveBinary", 0x0080FFD0, &ni::anim::TransformData_SaveBinary);
NIC30_KEYDATA_REVERSED("NiTransformData::DeletingDestructor", 0x00810130,
                       &ni::anim::TransformData_DeletingDestructor);
NIC30_KEYDATA_REVERSED("NiTransformData::GetViewerStrings", 0x00810150, &ni::anim::TransformData_GetViewerStrings);
