// NiFloatInterpolator (vtable 0x966B00, 0x18 bytes: a float from a NiFloatData's keys, or posed
// without data) and NiFloatData (vtable 0x965AC0, 0x18 bytes: one channel of float keys), from
// src/gamebryo/NiAnimControllers.h. Construction, cloning, streaming, comparison, the key queries
// (shared with the other one-channel interpolators that keep their data at +0x10), Update,
// Collapse, the sequence interpolator and data, viewer strings.
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
using scene::GetViewerStringFloat;
using scene::GetViewerStringPointer;
using scene::GetViewerStringUint;
using scene::LoadBytes;
using scene::SaveBytes;
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiFloatInterpolator_RTTI, 0x020E4FCC);
GAME_VAR(RTTI, NiFloatData_RTTI, 0x020E5034);
GAME_VAR(float, kInvalidFloatValue, 0x00968244);   // NiFloatInterpolator's invalid value (-FLT_MAX)

constexpr uintptr_t NiFloatInterpolator_vtable = 0x00966B00;
constexpr uintptr_t NiFloatData_vtable = 0x00965AC0;

GAME_FN(float, __cdecl, FloatKey_GenInterp, 0x007F8300, float time, NiAnimationKey* keys, KeyType type,
        uint32_t numKeys, uint32_t* lastIdx, uint8_t size);
GAME_FN(void, __cdecl, AnimationKey_CopySequence, 0x007FF7D0, KeyContent content, KeyType type,
        NiAnimationKey* keys, uint32_t numKeys, float startTime, float endTime, re::Ptr32<NiAnimationKey>* newKeys,
        uint32_t* newNumKeys);
GAME_FN(void, __cdecl, AnimationKey_GuaranteeKeyAtStartAndEnd, 0x007FFAF0, KeyContent content, KeyType type,
        re::Ptr32<NiAnimationKey>* keys, uint32_t* numKeys, float startTime, float endTime);
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
GAME_FN(void, __thiscall, KeyBasedInterpolator_CopyMembers, 0x008253E0, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, Interpolator_LoadBinary, 0x007F7AB0, void* self, void* stream);
GAME_FN(void, __thiscall, Interpolator_SaveBinary, 0x008253F0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, Interpolator_IsEqual, 0x00825400, void* self, void* other);
GAME_FN(void, __thiscall, KeyBasedInterpolator_GetViewerStrings, 0x008255C0, void* self, NiTArray<char*>* strings);
GAME_FN(NiInterpolator*, __thiscall, Interpolator_GetSequenceInterpolator, 0x00825DF0, void* self, float start,
        float end);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, FloatInterpolator_CopyMembers, 0x008126F0, NiFloatInterpolator* self,
        NiFloatInterpolator* dest, void* cloning);
GAME_FN(void, __thiscall, FloatData_GuaranteeKeysAtStartAndEnd, 0x00812BB0, NiKeyChannelData* self, float start,
        float end);
GAME_FN(void, __thiscall, FloatData_ReplaceAnim, 0x00812D30, NiKeyChannelData* self, NiAnimationKey* keys,
        uint32_t numKeys, KeyType type);
GAME_FN(re::Ptr32<NiKeyChannelData>*, __thiscall, FloatData_GetSequenceData, 0x00812D90, NiKeyChannelData* self,
        re::Ptr32<NiKeyChannelData>* result, float start, float end);

// a data pointer compared as the IsEquals have it: both there and equal (slot 8), or both missing
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
inline void InitMembers(NiFloatInterpolator* self, float value) {
    self->vtable = reinterpret_cast<void*>(NiFloatInterpolator_vtable);
    self->value = value;
    self->data = nullptr;
    self->lastIdx = 0;
}

// ---- NiFloatInterpolator

// 0x812260: NiFloatInterpolator(data): unposed, the data held
NiFloatInterpolator* __fastcall FI_ConstructWithData(NiFloatInterpolator* self, re::Edx, NiKeyChannelData* data) {
    KeyBasedInterpolator_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiFloatInterpolator_vtable);
    self->value = kInvalidFloat;
    self->data = data;
    AddRefImport(data);
    self->lastIdx = 0;
    return self;
}

const RTTI* __fastcall FI_GetRTTI(NiFloatInterpolator*, re::Edx) {
    return &NiFloatInterpolator_RTTI;
}

// 0x8122B0: NiFloatInterpolator(value): posed
NiFloatInterpolator* __fastcall FI_ConstructPosed(NiFloatInterpolator* self, re::Edx, float value) {
    KeyBasedInterpolator_Construct(self);
    InitMembers(self, value);
    return self;
}

// 0x8122E0: GetKeyCount(channel) (slot 38; every one-channel interpolator with its data at +0x10)
uint32_t __fastcall FI_GetKeyCount(NiFloatInterpolator* self, re::Edx, uint16_t) {
    return self->data != nullptr ? self->data->numKeys : 0;
}

// 0x8122F0: GetKeyType(channel) (slot 39)
uint32_t __fastcall FI_GetKeyType(NiFloatInterpolator* self, re::Edx, uint16_t) {
    return self->data != nullptr ? self->data->type : 0;
}

// 0x812300: GetKeyArray(channel) (slot 41)
NiAnimationKey* __fastcall FI_GetKeyArray(NiFloatInterpolator* self, re::Edx, uint16_t) {
    return self->data != nullptr ? self->data->keys : nullptr;
}

// 0x812310: GetChannelPosed(channel) (slot 43): no data and a valid value
uint8_t __fastcall FI_GetChannelPosed(NiFloatInterpolator* self, re::Edx, uint16_t) {
    if (self->data != nullptr) {
        return 0;
    }
    return self->value == kInvalidFloatValue ? 0 : 1;
}

// 0x812340: Collapse (slot 30): no keys: the invalid value; one key, or linear / step keys all
// the same: that value posed. Either way the data let go
void __fastcall FI_Collapse(NiFloatInterpolator* self, re::Edx) {
    NiKeyChannelData* data = self->data;
    if (data == nullptr) {
        return;
    }
    KeyType type = data->type;
    uint8_t size = data->size;
    uint32_t numKeys = data->numKeys;
    const NiAnimationKey* keys = data->keys;
    if (numKeys == 0) {
        ReleaseRef(data);
        self->data = nullptr;
        self->value = kInvalidFloatValue;
        return;
    }
    float value = reinterpret_cast<const NiFloatKey*>(keys)->value;
    if (numKeys != 1) {
        if (type != LINKEY && type != STEPKEY) {
            return;
        }
        uint8_t same = 1;
        for (uint32_t i = 1; i < numKeys; i++) {
            if (!(KeyAt<NiFloatKey>(keys, i, size)->value == value)) {
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
    self->value = value;
}

// 0x812420: SetPoseValue(value): the data let go
void __fastcall FI_SetPoseValue(NiFloatInterpolator* self, re::Edx, float value) {
    if (self->data != nullptr) {
        ReleaseRef(self->data);
        self->data = nullptr;
    }
    self->value = value;
}

// 0x812460: GuaranteeTimeRange(start, end) (slot 32)
void __fastcall FI_GuaranteeTimeRange(NiFloatInterpolator* self, re::Edx, float start, float end) {
    if (self->data != nullptr) {
        FloatData_GuaranteeKeysAtStartAndEnd(self->data, start, end);
    }
}

// 0x812490: CreateObject
NiObject* __cdecl FI_CreateObject() {
    void* memory = NiNew(0x18);
    if (memory == nullptr) {
        return nullptr;
    }
    NiFloatInterpolator* self = static_cast<NiFloatInterpolator*>(memory);
    KeyBasedInterpolator_Construct(self);
    InitMembers(self, kInvalidFloat);
    return self;
}

// 0x8124D0: IsEqual
uint8_t __fastcall FI_IsEqual(NiFloatInterpolator* self, re::Edx, NiFloatInterpolator* other) {
    if (!Interpolator_IsEqual(self, other)) {
        return 0;
    }
    if (!(other->value == self->value)) {
        return 0;
    }
    return DataEqual(self->data, other->data);
}

// 0x812530: the deleting destructor (NiBoolInterpolator's too: its data at +0x10)
NiFloatInterpolator* __fastcall FI_DeletingDestructor(NiFloatInterpolator* self, re::Edx, uint32_t flags) {
    ReleaseRef(self->data);
    KeyBasedInterpolator_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x18);
    }
    return self;
}

// 0x812580: GetSequenceInterpolator(start, end) (slot 33): the data cut to the range
NiInterpolator* __fastcall FI_GetSequenceInterpolator(NiFloatInterpolator* self, re::Edx, float start, float end) {
    NiFloatInterpolator* clone =
        static_cast<NiFloatInterpolator*>(Interpolator_GetSequenceInterpolator(self, start, end));
    if (self->data != nullptr) {
        re::Ptr32<NiKeyChannelData> data = nullptr;
        FloatData_GetSequenceData(self->data, &data, start, end);
        AssignReleasingFirst(clone->data, data);
        clone->lastIdx = 0;
        ReleaseRef(data);
    }
    return clone;
}

// 0x812640: Update(time, target, float&) (slot 22): false when the value is the invalid one
uint8_t __fastcall FI_Update(NiFloatInterpolator* self, re::Edx, float time, NiObjectNET*, float* value) {
    if (time == self->lastTime) {
        *value = self->value;
        return self->value == kInvalidFloatValue ? 0 : 1;
    }
    NiKeyChannelData* data = self->data;
    if (data != nullptr && data->numKeys != 0) {
        self->value = FloatKey_GenInterp(time, data->keys, data->type, data->numKeys, &self->lastIdx, data->size);
    }
    if (self->value == kInvalidFloatValue) {
        return 0;
    }
    *value = self->value;
    self->lastTime = time;
    return 1;
}

// 0x8126F0: CopyMembers (the value, the data shared, the key index)
void __fastcall FI_CopyMembers(NiFloatInterpolator* self, re::Edx, NiFloatInterpolator* dest, void* cloning) {
    KeyBasedInterpolator_CopyMembers(self, dest, cloning);
    dest->value = self->value;
    AssignReleasingFirst(dest->data, self->data);
    dest->lastIdx = self->lastIdx;
}

// 0x812770: LoadBinary (the value, the data link)
void __fastcall FI_LoadBinary(NiFloatInterpolator* self, re::Edx, void* stream) {
    Interpolator_LoadBinary(self, stream);
    LoadBytes(stream, &self->value, 4);
    NiObject* data = Stream_ReadLinkIDObject(stream);
    AssignReleasingFirst(self->data, data);
}

// 0x8127F0: SaveBinary
void __fastcall FI_SaveBinary(NiFloatInterpolator* self, re::Edx, void* stream) {
    Interpolator_SaveBinary(self, stream);
    SaveBytes(stream, &self->value, 4);
    Virtual<void>(stream, 12, self->data);   // SaveLinkID
}

// 0x812840: CreateClone (a failed allocation still copied onto null, as the game has it)
NiFloatInterpolator* __fastcall FI_CreateClone(NiFloatInterpolator* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x18);
    if (memory == nullptr) {
        FloatInterpolator_CopyMembers(self, nullptr, cloning);
        return nullptr;
    }
    NiFloatInterpolator* clone = static_cast<NiFloatInterpolator*>(memory);
    KeyBasedInterpolator_Construct(clone);
    InitMembers(clone, kInvalidFloat);
    FloatInterpolator_CopyMembers(self, clone, cloning);
    return clone;
}

// 0x8128B0: GetViewerStrings
void __fastcall FI_GetViewerStrings(NiFloatInterpolator* self, re::Edx, NiTArray<char*>* strings) {
    KeyBasedInterpolator_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiFloatInterpolator_RTTI.name));
    AddViewerString(strings, GetViewerStringFloat("m_fFloatValue", self->value));
    AddViewerString(strings, GetViewerStringPointer("m_spFloatData", self->data));
}

// ---- NiFloatData

// 0x812BB0: GuaranteeKeysAtStartAndEnd(start, end)
void __fastcall FD_GuaranteeKeysAtStartAndEnd(NiKeyChannelData* self, re::Edx, float start, float end) {
    re::Ptr32<NiAnimationKey> keys = self->keys;
    AnimationKey_GuaranteeKeyAtStartAndEnd(FLOATKEY, self->type, &keys, &self->numKeys, start, end);
    self->keys = keys;
}

// 0x812BF0: ~NiFloatData (the keys deleted by their kind)
void __fastcall FD_Destruct(NiKeyChannelData* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiFloatData_vtable);
    if (self->keys != nullptr) {
        ms_delete[KeyIndex(FLOATKEY, self->type)](self->keys);
    }
    Object_Destruct(self);
}

// 0x812C20: SetAnim(keys, numKeys, type): taken over (all three needed, else none)
void __fastcall FD_SetAnim(NiKeyChannelData* self, re::Edx, NiAnimationKey* keys, uint32_t numKeys, KeyType type) {
    if (keys != nullptr && numKeys != 0 && type != NOINTERP) {
        self->keys = keys;
        self->numKeys = numKeys;
        self->type = type;
        self->size = ms_keySizes[KeyIndex(FLOATKEY, type)];
        return;
    }
    self->numKeys = 0;
    self->keys = nullptr;
    self->type = NOINTERP;
    self->size = 0;
}

// 0x812C70: CreateObject
NiObject* __cdecl FD_CreateObject() {
    void* memory = NiNew(0x18);
    if (memory == nullptr) {
        return nullptr;
    }
    NiKeyChannelData* self = static_cast<NiKeyChannelData*>(memory);
    Object_Construct(self);
    self->numKeys = 0;
    self->keys = nullptr;
    self->type = NOINTERP;
    self->size = 0;
    self->vtable = reinterpret_cast<void*>(NiFloatData_vtable);
    return self;
}

// 0x812CB0: IsEqual (the count, kind and every key)
uint8_t __fastcall FD_IsEqual(NiKeyChannelData* self, re::Edx, NiKeyChannelData* other) {
    if (!Object_IsEqual(self, other) || self->numKeys != other->numKeys || self->type != other->type ||
        self->size != other->size) {
        return 0;
    }
    EqualFunction equal = ms_equal[KeyIndex(FLOATKEY, self->type)];
    for (uint32_t i = 0; i < self->numKeys; i++) {
        uint32_t offset = self->size * i;
        if (!equal(KeyAt(self->keys, offset, 1), KeyAt(other->keys, offset, 1))) {
            return 0;
        }
    }
    return 1;
}

// 0x812D30: ReplaceAnim(keys, numKeys, type): the old keys deleted first
void __fastcall FD_ReplaceAnim(NiKeyChannelData* self, re::Edx, NiAnimationKey* keys, uint32_t numKeys, KeyType type) {
    if (self->keys != nullptr) {
        ms_delete[KeyIndex(FLOATKEY, self->type)](self->keys);
    }
    if (keys != nullptr && numKeys != 0 && type != NOINTERP) {
        self->keys = keys;
        self->size = ms_keySizes[KeyIndex(FLOATKEY, type)];
        self->numKeys = numKeys;
        self->type = type;
        return;
    }
    self->numKeys = 0;
    self->keys = nullptr;
    self->size = 0;
    self->type = NOINTERP;
}

// 0x812D90: GetSequenceData(start, end): a deep copy with the keys cut to the range
re::Ptr32<NiKeyChannelData>* __fastcall FD_GetSequenceData(NiKeyChannelData* self, re::Edx, re::Ptr32<NiKeyChannelData>* result,
                                                 float start, float end) {
    re::Ptr32<NiRefObject> copy;
    re::Ptr32<NiRefObject>* made = Object_CreateDeepCopy(self, &copy);
    *result = static_cast<NiKeyChannelData*>(*made);
    AddRefImport(*result);
    ReleaseRef(copy);
    if (self->numKeys != 0) {
        re::Ptr32<NiAnimationKey> newKeys = nullptr;
        uint32_t newNumKeys = 0;
        AnimationKey_CopySequence(FLOATKEY, self->type, self->keys, self->numKeys, start, end, &newKeys, &newNumKeys);
        FloatData_ReplaceAnim(*result, newKeys, newNumKeys, self->type);
    }
    return result;
}

// 0x812E40: LoadBinary (the count, then the kind and the keys made and filled in)
void __fastcall FD_LoadBinary(NiKeyChannelData* self, re::Edx, void* stream) {
    Object_LoadBinary(self, stream);
    uint32_t numKeys;
    LoadBytes(stream, &numKeys, 4);
    if (numKeys == 0) {
        return;
    }
    KeyType type;
    LoadBytes(stream, &type, 4);
    uint32_t index = KeyIndex(FLOATKEY, type);
    self->size = ms_keySizes[index];
    NiAnimationKey* keys = ms_create[index](stream, numKeys);
    ms_fillDerived[index](keys, numKeys, self->size);
    FloatData_ReplaceAnim(self, keys, numKeys, type);
}

// 0x812F00: SaveBinary
void __fastcall FD_SaveBinary(NiKeyChannelData* self, re::Edx, void* stream) {
    Object_SaveBinary(self, stream);
    SaveBytes(stream, &self->numKeys, 4);
    if (self->numKeys != 0) {
        SaveBytes(stream, &self->type, 4);
        ms_save[KeyIndex(FLOATKEY, self->type)](stream, self->keys, self->numKeys);
    }
}

// 0x812F80: GetViewerStrings
void __fastcall FD_GetViewerStrings(NiKeyChannelData* self, re::Edx, NiTArray<char*>* strings) {
    Object_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiFloatData_RTTI.name));
    AddViewerString(strings, GetViewerStringUint("m_uiNumKeys", self->numKeys));
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC31_FLOATINTERP_REVERSED(name, address, fn)                                              \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

// (a lone jmp to 0x8253F0, itself a jmp: NiInterpolator's LinkObject / SaveBinary, folded; many
// interpolators' slot 5 (LinkObject); 0x81ACD0 is a jmp to it)
RE_UNHOOKED("NiInterpolator::SaveBinary thunk", 0x00812250);

NIC31_FLOATINTERP_REVERSED("NiFloatInterpolator::NiFloatInterpolator(data)", 0x00812260,
                           &ni::anim::FI_ConstructWithData);
NIC31_FLOATINTERP_REVERSED("NiFloatInterpolator::GetRTTI", 0x008122A0, &ni::anim::FI_GetRTTI);
NIC31_FLOATINTERP_REVERSED("NiFloatInterpolator::NiFloatInterpolator(value)", 0x008122B0,
                           &ni::anim::FI_ConstructPosed);
NIC31_FLOATINTERP_REVERSED("NiFloatInterpolator::GetKeyCount", 0x008122E0, &ni::anim::FI_GetKeyCount);
NIC31_FLOATINTERP_REVERSED("NiFloatInterpolator::GetKeyType", 0x008122F0, &ni::anim::FI_GetKeyType);
NIC31_FLOATINTERP_REVERSED("NiFloatInterpolator::GetKeyArray", 0x00812300, &ni::anim::FI_GetKeyArray);
NIC31_FLOATINTERP_REVERSED("NiFloatInterpolator::GetChannelPosed", 0x00812310, &ni::anim::FI_GetChannelPosed);
NIC31_FLOATINTERP_REVERSED("NiFloatInterpolator::Collapse", 0x00812340, &ni::anim::FI_Collapse);
NIC31_FLOATINTERP_REVERSED("NiFloatInterpolator::SetPoseValue", 0x00812420, &ni::anim::FI_SetPoseValue);
NIC31_FLOATINTERP_REVERSED("NiFloatInterpolator::GuaranteeTimeRange", 0x00812460, &ni::anim::FI_GuaranteeTimeRange);
NIC31_FLOATINTERP_REVERSED("NiFloatInterpolator::CreateObject", 0x00812490, &ni::anim::FI_CreateObject);
NIC31_FLOATINTERP_REVERSED("NiFloatInterpolator::IsEqual", 0x008124D0, &ni::anim::FI_IsEqual);
NIC31_FLOATINTERP_REVERSED("NiFloatInterpolator::DeletingDestructor", 0x00812530, &ni::anim::FI_DeletingDestructor);
NIC31_FLOATINTERP_REVERSED("NiFloatInterpolator::GetSequenceInterpolator", 0x00812580,
                           &ni::anim::FI_GetSequenceInterpolator);
NIC31_FLOATINTERP_REVERSED("NiFloatInterpolator::Update", 0x00812640, &ni::anim::FI_Update);
NIC31_FLOATINTERP_REVERSED("NiFloatInterpolator::CopyMembers", 0x008126F0, &ni::anim::FI_CopyMembers);
NIC31_FLOATINTERP_REVERSED("NiFloatInterpolator::LoadBinary", 0x00812770, &ni::anim::FI_LoadBinary);
NIC31_FLOATINTERP_REVERSED("NiFloatInterpolator::SaveBinary", 0x008127F0, &ni::anim::FI_SaveBinary);
NIC31_FLOATINTERP_REVERSED("NiFloatInterpolator::CreateClone", 0x00812840, &ni::anim::FI_CreateClone);
NIC31_FLOATINTERP_REVERSED("NiFloatInterpolator::GetViewerStrings", 0x008128B0, &ni::anim::FI_GetViewerStrings);

NIC31_FLOATINTERP_REVERSED("NiFloatData::GuaranteeKeysAtStartAndEnd", 0x00812BB0,
                           &ni::anim::FD_GuaranteeKeysAtStartAndEnd);
NIC31_FLOATINTERP_REVERSED("NiFloatData::~NiFloatData", 0x00812BF0, &ni::anim::FD_Destruct);
NIC31_FLOATINTERP_REVERSED("NiFloatData::SetAnim", 0x00812C20, &ni::anim::FD_SetAnim);
NIC31_FLOATINTERP_REVERSED("NiFloatData::CreateObject", 0x00812C70, &ni::anim::FD_CreateObject);
NIC31_FLOATINTERP_REVERSED("NiFloatData::IsEqual", 0x00812CB0, &ni::anim::FD_IsEqual);
NIC31_FLOATINTERP_REVERSED("NiFloatData::ReplaceAnim", 0x00812D30, &ni::anim::FD_ReplaceAnim);
NIC31_FLOATINTERP_REVERSED("NiFloatData::GetSequenceData", 0x00812D90, &ni::anim::FD_GetSequenceData);
NIC31_FLOATINTERP_REVERSED("NiFloatData::LoadBinary", 0x00812E40, &ni::anim::FD_LoadBinary);
NIC31_FLOATINTERP_REVERSED("NiFloatData::SaveBinary", 0x00812F00, &ni::anim::FD_SaveBinary);
NIC31_FLOATINTERP_REVERSED("NiFloatData::GetViewerStrings", 0x00812F80, &ni::anim::FD_GetViewerStrings);
