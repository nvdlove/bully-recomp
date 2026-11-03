// NiColorInterpolator (vtable 0x966E00, 0x24 bytes: a colour from a NiColorData's keys, or posed
// without data) and NiColorData (vtable 0x966F78, 0x18 bytes: one channel of colour keys), from
// src/gamebryo/NiAnimControllers.h. Some of the interpolator's slots are NiQuaternionInterpolator's
// too (the same layout: the value at +0xC, the data at +0x1C): its LoadBinary, SaveBinary,
// deleting destructor and GetKeyStride.
//
// Main-thread float maths at the game's 24-bit x87 precision.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiAnimControllers.h"
#include "gamebryo/NiAnimationKey.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiInterpolators.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni::anim {

namespace {

using scene::AddViewerString;
using scene::GetViewerString;
using scene::GetViewerStringPointer;
using scene::GetViewerStringUint;
using scene::LoadBytes;
using scene::SaveBytes;
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiColorInterpolator_RTTI, 0x020E5104);
GAME_VAR(RTTI, NiColorData_RTTI, 0x020E516C);
GAME_VAR(NiColorA, kInvalidColor, 0x00BC8C54);   // NiColorInterpolator's invalid value

constexpr uintptr_t NiColorInterpolator_vtable = 0x00966E00;
constexpr uintptr_t NiColorData_vtable = 0x00966F78;

GAME_FN(NiColorA*, __cdecl, ColorKey_GenInterp, 0x007FADC0, NiColorA* result, float time, NiAnimationKey* keys,
        KeyType type, uint32_t numKeys, uint32_t* lastIdx, uint8_t size);
GAME_FN(void, __cdecl, AnimationKey_CopySequence, 0x007FF7D0, KeyContent content, KeyType type,
        NiAnimationKey* keys, uint32_t numKeys, float startTime, float endTime, re::Ptr32<NiAnimationKey>* newKeys,
        uint32_t* newNumKeys);
GAME_FN(void, __cdecl, AnimationKey_GuaranteeKeyAtStartAndEnd, 0x007FFAF0, KeyContent content, KeyType type,
        re::Ptr32<NiAnimationKey>* keys, uint32_t* numKeys, float startTime, float endTime);
// the four floats compared (NiQuaternion's ==, reached for colours too) and NiColorA's !=
GAME_FN(uint8_t, __thiscall, Float4_Equal, 0x007F9E40, const NiColorA* self, const NiColorA* other);
GAME_FN(uint8_t, __thiscall, ColorA_NotEqual, 0x00769390, const NiColorA* self, const NiColorA* other);
GAME_FN(void, __thiscall, ColorA_LoadBinary, 0x007625D0, NiColorA* self, void* stream);
GAME_FN(void, __thiscall, ColorA_SaveBinary, 0x00762670, const NiColorA* self, void* stream);
GAME_FN(char*, __thiscall, ColorA_GetViewerString, 0x00762770, const NiColorA* self, const char* prefix);
GAME_FN(NiObject*, __thiscall, Stream_ReadLinkIDObject, 0x0075A820, void* stream);
GAME_FN(re::Ptr32<NiRefObject>*, __thiscall, Object_CreateDeepCopy, 0x00755430, void* self, re::Ptr32<NiRefObject>* result);
GAME_FN(void, __thiscall, Object_Construct, 0x00755260, void* self);
GAME_FN(void, __thiscall, Object_Destruct, 0x007769D0, void* self);
GAME_FN(void, __thiscall, Object_LoadBinary, 0x007554E0, void* self, void* stream);
GAME_FN(void, __thiscall, Object_SaveBinary, 0x008A20A0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, Object_IsEqual, 0x007552A0, void* self, void* other);
GAME_FN(void, __thiscall, Object_GetViewerStrings, 0x007555A0, void* self, NiTArray<char*>* strings);
// the bases' (NiKeyBasedInterpolator / NiInterpolator)
GAME_FN(void, __thiscall, KeyBasedInterpolator_Construct, 0x008253B0, void* self);
GAME_FN(void, __thiscall, KeyBasedInterpolator_Destruct, 0x008253D0, void* self);
GAME_FN(void, __thiscall, Interpolator_LoadBinary, 0x007F7AB0, void* self, void* stream);
GAME_FN(void, __thiscall, Interpolator_SaveBinary, 0x008253F0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, Interpolator_IsEqual, 0x00825400, void* self, void* other);
GAME_FN(void, __thiscall, KeyBasedInterpolator_GetViewerStrings, 0x008255C0, void* self, NiTArray<char*>* strings);
GAME_FN(NiInterpolator*, __thiscall, Interpolator_GetSequenceInterpolator, 0x00825DF0, void* self, float start,
        float end);
// NiQuaternionInterpolator::CopyMembers (src/gamebryo/NiKeyInterpolators.cpp: the same layout)
GAME_FN(void, __thiscall, QuaternionInterpolator_CopyMembers, 0x00807B30, void* self, void* dest, void* cloning);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, ColorData_GuaranteeKeysAtStartAndEnd, 0x008199B0, NiKeyChannelData* self, float start,
        float end);
GAME_FN(void, __thiscall, ColorData_ReplaceAnim, 0x00819B00, NiKeyChannelData* self, NiAnimationKey* keys,
        uint32_t numKeys, KeyType type);
GAME_FN(re::Ptr32<NiKeyChannelData>*, __thiscall, ColorData_GetSequenceData, 0x00819B60, NiKeyChannelData* self,
        re::Ptr32<NiKeyChannelData>* result, float start, float end);

inline uint8_t DataEqual(NiObject* data, NiObject* other) {
    if (data != nullptr) {
        if (other == nullptr) {
            return 0;
        }
    } else {
        if (other != nullptr) {
            return 0;
        }
        return 1;
    }
    return Virtual<uint8_t>(data, 8, other) ? 1 : 0;
}

// the members the constructors and CreateObject set (no data)
inline void InitMembers(NiColorInterpolator* self, const NiColorA& value) {
    self->vtable = reinterpret_cast<void*>(NiColorInterpolator_vtable);
    self->value = value;
    self->data = nullptr;
    self->lastIdx = 0;
}

// ---- NiColorInterpolator

// 0x818F40: GetKeyContent(channel) (slot 40)
uint32_t __fastcall CI_GetKeyContent(NiColorInterpolator*, re::Edx, uint16_t) {
    return COLORKEY;
}

// 0x818F50: NiColorInterpolator(data): unposed, the data held
NiColorInterpolator* __fastcall CI_ConstructWithData(NiColorInterpolator* self, re::Edx, NiKeyChannelData* data) {
    KeyBasedInterpolator_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiColorInterpolator_vtable);
    self->value = kInvalidColor;
    self->data = data;
    AddRefImport(data);
    self->lastIdx = 0;
    return self;
}

const RTTI* __fastcall CI_GetRTTI(NiColorInterpolator*, re::Edx) {
    return &NiColorInterpolator_RTTI;
}

// 0x818FC0: NiColorInterpolator(colour): posed
NiColorInterpolator* __fastcall CI_ConstructPosed(NiColorInterpolator* self, re::Edx, NiColorA value) {
    KeyBasedInterpolator_Construct(self);
    InitMembers(self, value);
    return self;
}

// 0x819000: GetKeyStride(channel) (slot 42; NiQuaternionInterpolator's too)
uint8_t __fastcall CI_GetKeyStride(NiColorInterpolator* self, re::Edx, uint16_t) {
    return self->data != nullptr ? self->data->size : 0;
}

// 0x819020: GetChannelPosed(channel) (slot 43): no data and a valid value
uint8_t __fastcall CI_GetChannelPosed(NiColorInterpolator* self, re::Edx, uint16_t) {
    if (self->data != nullptr) {
        return 0;
    }
    return Float4_Equal(&self->value, &kInvalidColor) == 0 ? 1 : 0;
}

// 0x819040: SetPoseValue(colour): the data let go
void __fastcall CI_SetPoseValue(NiColorInterpolator* self, re::Edx, NiColorA value) {
    if (self->data != nullptr) {
        ReleaseRef(self->data);
        self->data = nullptr;
    }
    self->value = value;
}

// 0x819090: GuaranteeTimeRange(start, end) (slot 32)
void __fastcall CI_GuaranteeTimeRange(NiColorInterpolator* self, re::Edx, float start, float end) {
    if (self->data != nullptr) {
        ColorData_GuaranteeKeysAtStartAndEnd(self->data, start, end);
    }
}

// 0x8190C0: CreateObject
NiObject* __cdecl CI_CreateObject() {
    void* memory = NiNew(0x24);
    if (memory == nullptr) {
        return nullptr;
    }
    NiColorInterpolator* self = static_cast<NiColorInterpolator*>(memory);
    KeyBasedInterpolator_Construct(self);
    InitMembers(self, kInvalidColor);
    return self;
}

// 0x819120: LoadBinary (the value, the data link; NiQuaternionInterpolator's too)
void __fastcall CI_LoadBinary(NiColorInterpolator* self, re::Edx, void* stream) {
    Interpolator_LoadBinary(self, stream);
    ColorA_LoadBinary(&self->value, stream);
    NiObject* data = Stream_ReadLinkIDObject(stream);
    AssignReleasingFirst(self->data, data);
}

// 0x819180: SaveBinary (NiQuaternionInterpolator's too)
void __fastcall CI_SaveBinary(NiColorInterpolator* self, re::Edx, void* stream) {
    Interpolator_SaveBinary(self, stream);
    ColorA_SaveBinary(&self->value, stream);
    Virtual<void>(stream, 12, self->data);   // SaveLinkID
}

// 0x8191B0: the deleting destructor (NiQuaternionInterpolator's too)
NiColorInterpolator* __fastcall CI_DeletingDestructor(NiColorInterpolator* self, re::Edx, uint32_t flags) {
    ReleaseRef(self->data);
    KeyBasedInterpolator_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x24);
    }
    return self;
}

// 0x819200: Collapse (slot 30): no keys: the invalid value; one key, or linear / step keys all
// the same: that colour posed. Either way the data let go
void __fastcall CI_Collapse(NiColorInterpolator* self, re::Edx) {
    NiKeyChannelData* data = self->data;
    if (data == nullptr) {
        return;
    }
    uint32_t numKeys = data->numKeys;
    uint8_t size = data->size;
    const NiAnimationKey* keys = data->keys;
    KeyType type = data->type;
    if (numKeys == 0) {
        ReleaseRef(data);
        self->data = nullptr;
        self->value = kInvalidColor;
        return;
    }
    NiColorA color = reinterpret_cast<const NiColorKey*>(keys)->color;
    if (numKeys != 1) {
        if (type != LINKEY && type != STEPKEY) {
            return;
        }
        uint8_t same = 1;
        for (uint32_t i = 1; i < numKeys; i++) {
            if (ColorA_NotEqual(&KeyAt<NiColorKey>(keys, i, size)->color, &color)) {
                same = 0;
            }
            if (!same) {
                return;
            }
        }
    }
    if (self->data != nullptr) {
        ReleaseRef(self->data);
        self->data = nullptr;
    }
    self->value = color;
}

// 0x819340: GetSequenceInterpolator(start, end) (slot 33): the data cut to the range
NiInterpolator* __fastcall CI_GetSequenceInterpolator(NiColorInterpolator* self, re::Edx, float start, float end) {
    NiColorInterpolator* clone =
        static_cast<NiColorInterpolator*>(Interpolator_GetSequenceInterpolator(self, start, end));
    if (self->data != nullptr) {
        re::Ptr32<NiKeyChannelData> data = nullptr;
        ColorData_GetSequenceData(self->data, &data, start, end);
        AssignReleasingFirst(clone->data, data);
        clone->lastIdx = 0;
        ReleaseRef(data);
    }
    return clone;
}

// 0x819400: Update(time, target, colour&) (slot 19): false when the value is the invalid one
uint8_t __fastcall CI_Update(NiColorInterpolator* self, re::Edx, float time, NiObjectNET*, NiColorA* value) {
    if (time == self->lastTime) {
        *value = self->value;
        return Float4_Equal(&self->value, &kInvalidColor) == 0 ? 1 : 0;
    }
    NiKeyChannelData* data = self->data;
    if (data != nullptr && data->numKeys != 0) {
        NiColorA c;
        const NiColorA* r =
            ColorKey_GenInterp(&c, time, data->keys, data->type, data->numKeys, &self->lastIdx, data->size);
        self->value = *r;
    }
    if (Float4_Equal(&self->value, &kInvalidColor)) {
        return 0;
    }
    *value = self->value;
    self->lastTime = time;
    return 1;
}

// 0x8194F0: CreateClone (a failed allocation still copied onto null, as the game has it)
NiColorInterpolator* __fastcall CI_CreateClone(NiColorInterpolator* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x24);
    if (memory == nullptr) {
        QuaternionInterpolator_CopyMembers(self, nullptr, cloning);
        return nullptr;
    }
    NiColorInterpolator* clone = static_cast<NiColorInterpolator*>(memory);
    KeyBasedInterpolator_Construct(clone);
    InitMembers(clone, kInvalidColor);
    QuaternionInterpolator_CopyMembers(self, clone, cloning);
    return clone;
}

// 0x819570: IsEqual
uint8_t __fastcall CI_IsEqual(NiColorInterpolator* self, re::Edx, NiColorInterpolator* other) {
    if (!Interpolator_IsEqual(self, other)) {
        return 0;
    }
    if (ColorA_NotEqual(&self->value, &other->value)) {
        return 0;
    }
    return DataEqual(self->data, other->data);
}

// 0x8195D0: GetViewerStrings
void __fastcall CI_GetViewerStrings(NiColorInterpolator* self, re::Edx, NiTArray<char*>* strings) {
    KeyBasedInterpolator_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiColorInterpolator_RTTI.name));
    AddViewerString(strings, ColorA_GetViewerString(&self->value, "m_kColorValue"));
    AddViewerString(strings, GetViewerStringPointer("m_spColorData", self->data));
}

// ---- NiColorData

// 0x8199B0: GuaranteeKeysAtStartAndEnd(start, end)
void __fastcall CD_GuaranteeKeysAtStartAndEnd(NiKeyChannelData* self, re::Edx, float start, float end) {
    re::Ptr32<NiAnimationKey> keys = self->keys;
    AnimationKey_GuaranteeKeyAtStartAndEnd(COLORKEY, self->type, &keys, &self->numKeys, start, end);
    self->keys = keys;
}

const RTTI* __fastcall CD_GetRTTI(NiKeyChannelData*, re::Edx) {
    return &NiColorData_RTTI;
}

// 0x819A00: CreateObject (the key size left as it was, as the game has it)
NiObject* __cdecl CD_CreateObject() {
    void* memory = NiNew(0x18);
    if (memory == nullptr) {
        return nullptr;
    }
    NiKeyChannelData* self = static_cast<NiKeyChannelData*>(memory);
    Object_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiColorData_vtable);
    self->numKeys = 0;
    self->keys = nullptr;
    self->type = NOINTERP;
    return self;
}

// 0x819A40: IsEqual (the count, kind and every key)
uint8_t __fastcall CD_IsEqual(NiKeyChannelData* self, re::Edx, NiKeyChannelData* other) {
    if (!Object_IsEqual(self, other) || self->numKeys != other->numKeys || self->type != other->type ||
        self->size != other->size) {
        return 0;
    }
    EqualFunction equal = ms_equal[KeyIndex(COLORKEY, self->type)];
    for (uint32_t i = 0; i < self->numKeys; i++) {
        uint32_t offset = self->size * i;
        if (!equal(KeyAt(self->keys, offset, 1), KeyAt(other->keys, offset, 1))) {
            return 0;
        }
    }
    return 1;
}

// 0x819AC0: the deleting destructor (the keys deleted by their kind)
NiKeyChannelData* __fastcall CD_DeletingDestructor(NiKeyChannelData* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiColorData_vtable);
    if (self->keys != nullptr) {
        ms_delete[KeyIndex(COLORKEY, self->type)](self->keys);
    }
    Object_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x18);
    }
    return self;
}

// 0x819B00: ReplaceAnim(keys, numKeys, type): the old keys deleted first (a key kind of 0 taken,
// unlike NiFloatData's)
void __fastcall CD_ReplaceAnim(NiKeyChannelData* self, re::Edx, NiAnimationKey* keys, uint32_t numKeys, KeyType type) {
    if (self->keys != nullptr) {
        ms_delete[KeyIndex(COLORKEY, self->type)](self->keys);
    }
    if (keys != nullptr && numKeys != 0) {
        self->keys = keys;
        self->size = ms_keySizes[KeyIndex(COLORKEY, type)];
        self->numKeys = numKeys;
        self->type = type;
        return;
    }
    self->numKeys = 0;
    self->keys = nullptr;
    self->type = NOINTERP;
    self->size = 0;
}

// 0x819B60: GetSequenceData(start, end): a deep copy with the keys cut to the range
re::Ptr32<NiKeyChannelData>* __fastcall CD_GetSequenceData(NiKeyChannelData* self, re::Edx, re::Ptr32<NiKeyChannelData>* result,
                                                 float start, float end) {
    re::Ptr32<NiRefObject> copy;
    re::Ptr32<NiRefObject>* made = Object_CreateDeepCopy(self, &copy);
    *result = static_cast<NiKeyChannelData*>(*made);
    AddRefImport(*result);
    ReleaseRef(copy);
    if (self->numKeys != 0) {
        re::Ptr32<NiAnimationKey> newKeys = nullptr;
        uint32_t newNumKeys = 0;
        AnimationKey_CopySequence(COLORKEY, self->type, self->keys, self->numKeys, start, end, &newKeys, &newNumKeys);
        ColorData_ReplaceAnim(*result, newKeys, newNumKeys, self->type);
    }
    return result;
}

// 0x819C10: LoadBinary (the count, then the kind and the keys; no derived values filled in)
void __fastcall CD_LoadBinary(NiKeyChannelData* self, re::Edx, void* stream) {
    Object_LoadBinary(self, stream);
    uint32_t numKeys;
    LoadBytes(stream, &numKeys, 4);
    if (numKeys == 0) {
        return;
    }
    KeyType type;
    LoadBytes(stream, &type, 4);
    NiAnimationKey* keys = ms_create[KeyIndex(COLORKEY, type)](stream, numKeys);
    ColorData_ReplaceAnim(self, keys, numKeys, type);
}

// 0x819CB0: SaveBinary
void __fastcall CD_SaveBinary(NiKeyChannelData* self, re::Edx, void* stream) {
    Object_SaveBinary(self, stream);
    SaveBytes(stream, &self->numKeys, 4);
    if (self->numKeys != 0) {
        SaveBytes(stream, &self->type, 4);
        ms_save[KeyIndex(COLORKEY, self->type)](stream, self->keys, self->numKeys);
    }
}

// 0x819D30: GetViewerStrings
void __fastcall CD_GetViewerStrings(NiKeyChannelData* self, re::Edx, NiTArray<char*>* strings) {
    Object_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiColorData_RTTI.name));
    AddViewerString(strings, GetViewerStringUint("m_uiNumKeys", self->numKeys));
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC31_COLORINTERP_REVERSED(name, address, fn)                                              \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIC31_COLORINTERP_REVERSED("NiColorInterpolator::GetKeyContent", 0x00818F40, &ni::anim::CI_GetKeyContent);
NIC31_COLORINTERP_REVERSED("NiColorInterpolator::NiColorInterpolator(data)", 0x00818F50,
                           &ni::anim::CI_ConstructWithData);
NIC31_COLORINTERP_REVERSED("NiColorInterpolator::GetRTTI", 0x00818FB0, &ni::anim::CI_GetRTTI);
NIC31_COLORINTERP_REVERSED("NiColorInterpolator::NiColorInterpolator(color)", 0x00818FC0,
                           &ni::anim::CI_ConstructPosed);
NIC31_COLORINTERP_REVERSED("NiColorInterpolator::GetKeyStride", 0x00819000, &ni::anim::CI_GetKeyStride);
NIC31_COLORINTERP_REVERSED("NiColorInterpolator::GetChannelPosed", 0x00819020, &ni::anim::CI_GetChannelPosed);
NIC31_COLORINTERP_REVERSED("NiColorInterpolator::SetPoseValue", 0x00819040, &ni::anim::CI_SetPoseValue);
NIC31_COLORINTERP_REVERSED("NiColorInterpolator::GuaranteeTimeRange", 0x00819090, &ni::anim::CI_GuaranteeTimeRange);
NIC31_COLORINTERP_REVERSED("NiColorInterpolator::CreateObject", 0x008190C0, &ni::anim::CI_CreateObject);
NIC31_COLORINTERP_REVERSED("NiColorInterpolator::LoadBinary", 0x00819120, &ni::anim::CI_LoadBinary);
NIC31_COLORINTERP_REVERSED("NiColorInterpolator::SaveBinary", 0x00819180, &ni::anim::CI_SaveBinary);
NIC31_COLORINTERP_REVERSED("NiColorInterpolator::DeletingDestructor", 0x008191B0, &ni::anim::CI_DeletingDestructor);
NIC31_COLORINTERP_REVERSED("NiColorInterpolator::Collapse", 0x00819200, &ni::anim::CI_Collapse);
NIC31_COLORINTERP_REVERSED("NiColorInterpolator::GetSequenceInterpolator", 0x00819340,
                           &ni::anim::CI_GetSequenceInterpolator);
NIC31_COLORINTERP_REVERSED("NiColorInterpolator::Update", 0x00819400, &ni::anim::CI_Update);
NIC31_COLORINTERP_REVERSED("NiColorInterpolator::CreateClone", 0x008194F0, &ni::anim::CI_CreateClone);
NIC31_COLORINTERP_REVERSED("NiColorInterpolator::IsEqual", 0x00819570, &ni::anim::CI_IsEqual);
NIC31_COLORINTERP_REVERSED("NiColorInterpolator::GetViewerStrings", 0x008195D0, &ni::anim::CI_GetViewerStrings);

NIC31_COLORINTERP_REVERSED("NiColorData::GuaranteeKeysAtStartAndEnd", 0x008199B0,
                           &ni::anim::CD_GuaranteeKeysAtStartAndEnd);
NIC31_COLORINTERP_REVERSED("NiColorData::GetRTTI", 0x008199F0, &ni::anim::CD_GetRTTI);
NIC31_COLORINTERP_REVERSED("NiColorData::CreateObject", 0x00819A00, &ni::anim::CD_CreateObject);
NIC31_COLORINTERP_REVERSED("NiColorData::IsEqual", 0x00819A40, &ni::anim::CD_IsEqual);
NIC31_COLORINTERP_REVERSED("NiColorData::DeletingDestructor", 0x00819AC0, &ni::anim::CD_DeletingDestructor);
NIC31_COLORINTERP_REVERSED("NiColorData::ReplaceAnim", 0x00819B00, &ni::anim::CD_ReplaceAnim);
NIC31_COLORINTERP_REVERSED("NiColorData::GetSequenceData", 0x00819B60, &ni::anim::CD_GetSequenceData);
NIC31_COLORINTERP_REVERSED("NiColorData::LoadBinary", 0x00819C10, &ni::anim::CD_LoadBinary);
NIC31_COLORINTERP_REVERSED("NiColorData::SaveBinary", 0x00819CB0, &ni::anim::CD_SaveBinary);
NIC31_COLORINTERP_REVERSED("NiColorData::GetViewerStrings", 0x00819D30, &ni::anim::CD_GetViewerStrings);
