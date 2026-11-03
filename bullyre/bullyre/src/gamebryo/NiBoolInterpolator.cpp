// The bool animation of src/gamebryo/NiBoolInterpolator.h: NiBoolTimelineInterpolator (its
// Update, construction, cloning, destructor, viewer strings), NiBoolInterpolator (construction,
// cloning, streaming, comparison, the key queries, Update, Collapse, the time range and the
// sequence interpolator) and NiBoolData (its keys replaced, guaranteed at both ends and cut to a
// span, streaming, comparison, viewer strings).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiBoolInterpolator.h"
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
using scene::Object_Destruct;
using scene::Object_GetViewerStrings;
using scene::Object_IsEqual;
using scene::Object_LoadBinary;
using scene::RefObject_Construct;
using scene::SaveBytes;
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiBoolTimelineInterpolator_RTTI, 0x020E53A8);
GAME_VAR(RTTI, NiBoolInterpolator_RTTI, 0x020E53DC);
GAME_VAR(RTTI, NiBoolData_RTTI, 0x020E5410);

constexpr uintptr_t NiBoolTimelineInterpolator_vtable = 0x009676E8;
constexpr uintptr_t NiBoolInterpolator_vtable = 0x00967798;
constexpr uintptr_t NiBoolData_vtable = 0x00967868;

GAME_FN(char*, __cdecl, GetViewerStringByte, 0x00785600, const char* prefix, uint8_t value);
GAME_FN(void, __thiscall, Object_SaveBinaryNone, 0x008A20A0, void* self, void* stream);
GAME_FN(re::Ptr32<NiObject>*, __thiscall, Object_CreateDeepCopy, 0x00755430, void* self, re::Ptr32<NiObject>* result);
GAME_FN(NiObject*, __thiscall, Stream_ReadLinkIDObject, 0x0075A820, void* stream);
GAME_FN(void, __cdecl, NiFreeArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(uint8_t, __cdecl, BoolKey_GenInterp, 0x007FA9D0, float time, NiAnimationKey* keys, KeyType type,
        uint32_t numKeys, uint32_t* lastIdx, uint8_t size);
GAME_FN(void, __cdecl, Key_GuaranteeKeysAtStartAndEnd, 0x007FFAF0, KeyContent content, KeyType type,
        re::Ptr32<NiAnimationKey>* keys, uint32_t* numKeys, float start, float end);
GAME_FN(void, __cdecl, Key_CopySequence, 0x007FF7D0, KeyContent content, KeyType type, NiAnimationKey* keys,
        uint32_t numKeys, float start, float end, re::Ptr32<NiAnimationKey>* newKeys, uint32_t* newNumKeys);
// the bases' (NiKeyBasedInterpolator / NiInterpolator)
GAME_FN(void, __thiscall, KeyBasedInterpolator_Construct, 0x008253B0, void* self);
GAME_FN(void, __thiscall, KeyBasedInterpolator_Destruct, 0x008253D0, void* self);
GAME_FN(void, __thiscall, KeyBasedInterpolator_CopyMembers, 0x008253E0, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, Interpolator_LoadBinary, 0x007F7AB0, void* self, void* stream);
GAME_FN(void, __thiscall, Interpolator_SaveBinary, 0x008253F0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, Interpolator_RegisterStreamables, 0x007DC970, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, Interpolator_IsEqual, 0x00825400, void* self, void* other);
GAME_FN(void, __thiscall, KeyBasedInterpolator_GetViewerStrings, 0x008255C0, void* self, NiTArray<char*>* strings);
GAME_FN(NiInterpolator*, __thiscall, Interpolator_GetSequenceInterpolator, 0x00825DF0, void* self, float start,
        float end);
// (this file's, called where modes apply)
GAME_FN(NiBoolInterpolator*, __thiscall, BoolInterpolator_Construct, 0x0081CF70, void* self, NiBoolData* data);
GAME_FN(void, __thiscall, BoolInterpolator_CopyMembers, 0x0081D190, NiBoolInterpolator* self, NiBoolInterpolator* dest,
        void* cloning);
GAME_FN(void, __thiscall, BoolInterpolator_GetViewerStrings, 0x0081D5A0, NiBoolInterpolator* self,
        NiTArray<char*>* strings);
GAME_FN(void, __thiscall, BoolData_GuaranteeKeysAtStartAndEnd, 0x0081D670, NiBoolData* self, float start, float end);
GAME_FN(re::Ptr32<NiBoolData>*, __thiscall, BoolData_GetSequenceData, 0x0081D7E0, NiBoolData* self, re::Ptr32<NiBoolData>* result,
        float start, float end);
GAME_FN(void, __thiscall, BoolData_ReplaceAnim, 0x0081D780, NiBoolData* self, NiAnimationKey* keys, uint32_t numKeys,
        KeyType type);

inline uint32_t BoolIndex(KeyType type) {
    return KeyIndex(BOOLKEY, type);
}

// the value handed out as Update does: none (false, and a failed update) when invalid
inline uint8_t GiveValue(const NiBoolInterpolator* self, uint8_t* value) {
    if (self->value == kInvalidBool) {
        *value = 0;
        return 0;
    }
    *value = self->value != 0 ? 1 : 0;
    return 1;
}

// a data pointer compared as the IsEqual have it: both there and equal (slot 8), or both missing
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

// ---- NiBoolTimelineInterpolator

inline void TimelineInitMembers(NiBoolTimelineInterpolator* self) {
    self->vtable = reinterpret_cast<void*>(NiBoolTimelineInterpolator_vtable);
    self->previousIdx = 0;
    self->previousValue = 0;
}

// 0x81CC80
const RTTI* __fastcall Timeline_GetRTTI(NiBoolTimelineInterpolator*, re::Edx) {
    return &NiBoolTimelineInterpolator_RTTI;
}

// 0x81CC90: Update(time, target, bool&) (slot 23): the keys' value; when it's the same as at the
// last update, the keys passed over since (forwards, or round the end when the index went back)
// are searched for the other value, and a flip there is reported
uint8_t __fastcall Timeline_Update(NiBoolTimelineInterpolator* self, re::Edx, float time, NiObjectNET*, uint8_t* value) {
    if (time == self->lastTime) {
        return GiveValue(self, value);
    }
    NiBoolData* data = self->data;
    if (data != nullptr) {
        uint8_t size = data->bools.size;
        KeyType type = data->bools.type;
        NiAnimationKey* keys = data->bools.keys;
        uint32_t numKeys = data->bools.numKeys;
        if (numKeys != 0) {
            self->previousIdx = self->lastIdx;
            uint8_t result = BoolKey_GenInterp(time, keys, type, numKeys, &self->lastIdx, size);
            uint32_t idx = self->lastIdx;
            uint8_t shown = result;
            if (result == self->previousValue) {
                uint32_t prev = self->previousIdx;
                uint8_t other = result == 0 ? 1 : 0;
                if (prev == idx || prev + 1 <= idx) {
                    for (uint32_t c = prev + 1; c < idx && c < numKeys; c++) {
                        if (KeyAt<NiBoolKey>(keys, c, size)->value == other) {
                            shown = other;
                            break;
                        }
                    }
                } else {
                    bool found = false;
                    for (uint32_t c = prev + 1; c < numKeys; c++) {
                        if (KeyAt<NiBoolKey>(keys, c, size)->value == other) {
                            shown = other;
                            found = true;
                            break;
                        }
                    }
                    if (!found && result != other) {
                        for (uint32_t c = 0; c < idx; c++) {
                            if (KeyAt<NiBoolKey>(keys, c, size)->value == other) {
                                shown = other;
                                break;
                            }
                        }
                    }
                }
            }
            self->value = shown != 0 ? 1 : 0;
            self->previousValue = result;
        }
    }
    if (self->value == kInvalidBool) {
        *value = 0;
        return 0;
    }
    *value = self->value != 0 ? 1 : 0;
    self->lastTime = time;
    return 1;
}

// 0x81CE20: the deleting destructor (the data let go before the base's destructor)
NiBoolTimelineInterpolator* __fastcall Timeline_DeletingDestructor(NiBoolTimelineInterpolator* self, re::Edx,
                                                                   uint32_t flags) {
    ReleaseRef(self->data);
    KeyBasedInterpolator_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x20);
    }
    return self;
}

// 0x81CE70: CreateClone (NiBoolInterpolator's CopyMembers: the timeline's own state not copied;
// a failed allocation still copied onto null)
NiBoolTimelineInterpolator* __fastcall Timeline_CreateClone(NiBoolTimelineInterpolator* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x20);
    if (memory == nullptr) {
        BoolInterpolator_CopyMembers(self, nullptr, cloning);
        return nullptr;
    }
    auto* clone = static_cast<NiBoolTimelineInterpolator*>(BoolInterpolator_Construct(memory, nullptr));
    TimelineInitMembers(clone);
    BoolInterpolator_CopyMembers(self, clone, cloning);
    return clone;
}

// 0x81CED0: CreateObject
NiObject* __cdecl Timeline_CreateObject() {
    void* memory = NiNew(0x20);
    if (memory == nullptr) {
        return nullptr;
    }
    auto* self = static_cast<NiBoolTimelineInterpolator*>(BoolInterpolator_Construct(memory, nullptr));
    TimelineInitMembers(self);
    return self;
}

// 0x81CF10: GetViewerStrings
void __fastcall Timeline_GetViewerStrings(NiBoolTimelineInterpolator* self, re::Edx, NiTArray<char*>* strings) {
    BoolInterpolator_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBoolTimelineInterpolator_RTTI.name));
}

// ---- NiBoolInterpolator

inline void BoolInitMembers(NiBoolInterpolator* self) {
    self->vtable = reinterpret_cast<void*>(NiBoolInterpolator_vtable);
    self->value = kInvalidBool;
    self->data = nullptr;
    self->lastIdx = 0;
}

// 0x81CF60: GetKeyContent (slot 40)
uint32_t __fastcall Bool_GetKeyContent(NiBoolInterpolator*, re::Edx, uint16_t) {
    return BOOLKEY;
}

// 0x81CF70: NiBoolInterpolator(data)
NiBoolInterpolator* __fastcall Bool_ConstructWithData(NiBoolInterpolator* self, re::Edx, NiBoolData* data) {
    KeyBasedInterpolator_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiBoolInterpolator_vtable);
    self->value = kInvalidBool;
    self->data = data;
    AddRefImport(data);
    self->lastIdx = 0;
    return self;
}

// 0x81CFB0
const RTTI* __fastcall Bool_GetRTTI(NiBoolInterpolator*, re::Edx) {
    return &NiBoolInterpolator_RTTI;
}

// 0x81CFC0: NiBoolInterpolator(pose)
NiBoolInterpolator* __fastcall Bool_ConstructWithPose(NiBoolInterpolator* self, re::Edx, uint8_t pose) {
    KeyBasedInterpolator_Construct(self);
    self->value = pose;
    self->vtable = reinterpret_cast<void*>(NiBoolInterpolator_vtable);
    self->data = nullptr;
    self->lastIdx = 0;
    return self;
}

// 0x81CFF0: GetKeyStride (slot 42; NiFloatInterpolator's too)
uint8_t __fastcall Bool_GetKeyStride(NiBoolInterpolator* self, re::Edx, uint16_t) {
    return self->data != nullptr ? self->data->bools.size : 0;
}

// 0x81D010: GetChannelPosed (slot 43): no data and a valid value
uint8_t __fastcall Bool_GetChannelPosed(NiBoolInterpolator* self, re::Edx, uint16_t) {
    if (self->data != nullptr) {
        return 0;
    }
    return self->value != kInvalidBool ? 1 : 0;
}

// 0x81D030: Collapse (slot 30): no keys: invalid; one key, or linear / step keys all the same:
// that value posed. Either way the data let go
void __fastcall Bool_Collapse(NiBoolInterpolator* self, re::Edx) {
    NiBoolData* data = self->data;
    if (data == nullptr) {
        return;
    }
    KeyType type = data->bools.type;
    uint8_t size = data->bools.size;
    uint32_t numKeys = data->bools.numKeys;
    const NiAnimationKey* keys = data->bools.keys;
    if (numKeys == 0) {
        ReleaseRef(data);
        self->data = nullptr;
        self->value = kInvalidBool;
        return;
    }
    uint8_t v = reinterpret_cast<const NiBoolKey*>(keys)->value;
    if (numKeys != 1) {
        if (type != LINKEY && type != STEPKEY) {
            return;
        }
        uint8_t same = 1;
        for (uint32_t c = 1; c < numKeys; c++) {
            if (KeyAt<NiBoolKey>(keys, c, size)->value != v) {
                same = 0;
            }
            if (!same) {
                return;
            }
        }
        if (!same) {
            return;
        }
    }
    if (self->data != nullptr) {
        ReleaseRef(self->data);
        self->data = nullptr;
    }
    self->value = v;
}

// 0x81D110: SetPoseValue(bool): the data let go
void __fastcall Bool_SetPoseValue(NiBoolInterpolator* self, re::Edx, uint8_t pose) {
    if (self->data != nullptr) {
        ReleaseRef(self->data);
        self->data = nullptr;
    }
    self->value = pose;
}

// 0x81D160: GuaranteeTimeRange(start, end) (slot 32)
void __fastcall Bool_GuaranteeTimeRange(NiBoolInterpolator* self, re::Edx, float start, float end) {
    if (self->data != nullptr) {
        BoolData_GuaranteeKeysAtStartAndEnd(self->data, start, end);
    }
}

// 0x81D190: CopyMembers
void __fastcall Bool_CopyMembers(NiBoolInterpolator* self, re::Edx, NiBoolInterpolator* dest, void* cloning) {
    KeyBasedInterpolator_CopyMembers(self, dest, cloning);
    dest->value = self->value;
    AssignReleasingFirst(dest->data, self->data);
    dest->lastIdx = self->lastIdx;
}

// 0x81D210: CreateObject
NiObject* __cdecl Bool_CreateObject() {
    auto* self = static_cast<NiBoolInterpolator*>(NiNew(0x18));
    if (self == nullptr) {
        return nullptr;
    }
    KeyBasedInterpolator_Construct(self);
    BoolInitMembers(self);
    return self;
}

// 0x81D250: RegisterStreamables (NiFloatInterpolator's too)
uint8_t __fastcall Bool_RegisterStreamables(NiBoolInterpolator* self, re::Edx, void* stream) {
    if (!Interpolator_RegisterStreamables(self, stream)) {
        return 0;
    }
    if (self->data != nullptr) {
        Virtual<uint8_t>(self->data, 6, stream);
    }
    return 1;
}

// 0x81D280: IsEqual
uint8_t __fastcall Bool_IsEqual(NiBoolInterpolator* self, re::Edx, NiBoolInterpolator* other) {
    if (!Interpolator_IsEqual(self, other) || self->value != other->value) {
        return 0;
    }
    return DataEqual(self->data, other->data);
}

// 0x81D2E0: GetSequenceInterpolator(start, end) (slot 33): the copy's data cut to the span
NiInterpolator* __fastcall Bool_GetSequenceInterpolator(NiBoolInterpolator* self, re::Edx, float start, float end) {
    auto* clone = static_cast<NiBoolInterpolator*>(Interpolator_GetSequenceInterpolator(self, start, end));
    if (self->data != nullptr) {
        re::Ptr32<NiBoolData> data = nullptr;
        BoolData_GetSequenceData(self->data, &data, start, end);
        AssignReleasingFirst(clone->data, data);
        clone->lastIdx = 0;
        ReleaseRef(data);
    }
    return clone;
}

// 0x81D3A0: Update(time, target, bool&) (slot 23)
uint8_t __fastcall Bool_Update(NiBoolInterpolator* self, re::Edx, float time, NiObjectNET*, uint8_t* value) {
    if (time == self->lastTime) {
        return GiveValue(self, value);
    }
    NiBoolData* data = self->data;
    if (data != nullptr && data->bools.numKeys != 0) {
        uint8_t result = BoolKey_GenInterp(time, data->bools.keys, data->bools.type, data->bools.numKeys,
                                           &self->lastIdx, data->bools.size);
        self->value = result != 0 ? 1 : 0;
    }
    if (self->value == kInvalidBool) {
        *value = 0;
        return 0;
    }
    *value = self->value != 0 ? 1 : 0;
    self->lastTime = time;
    return 1;
}

// 0x81D460: CreateClone (a failed allocation still copied onto null)
NiBoolInterpolator* __fastcall Bool_CreateClone(NiBoolInterpolator* self, re::Edx, void* cloning) {
    auto* clone = static_cast<NiBoolInterpolator*>(NiNew(0x18));
    if (clone == nullptr) {
        BoolInterpolator_CopyMembers(self, nullptr, cloning);
        return nullptr;
    }
    KeyBasedInterpolator_Construct(clone);
    BoolInitMembers(clone);
    BoolInterpolator_CopyMembers(self, clone, cloning);
    return clone;
}

// 0x81D4D0: LoadBinary (the value, the data link resolved)
void __fastcall Bool_LoadBinary(NiBoolInterpolator* self, re::Edx, void* stream) {
    Interpolator_LoadBinary(self, stream);
    LoadBytes(stream, &self->value, 1);
    AssignReleasingFirst(self->data, Stream_ReadLinkIDObject(stream));
}

// 0x81D550: SaveBinary
void __fastcall Bool_SaveBinary(NiBoolInterpolator* self, re::Edx, void* stream) {
    Interpolator_SaveBinary(self, stream);
    SaveBytes(stream, &self->value, 1);
    Virtual<void>(stream, 12, self->data);   // SaveLinkID
}

// 0x81D5A0: GetViewerStrings
void __fastcall Bool_GetViewerStrings(NiBoolInterpolator* self, re::Edx, NiTArray<char*>* strings) {
    KeyBasedInterpolator_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBoolInterpolator_RTTI.name));
    AddViewerString(strings, GetViewerStringByte("m_bBoolValue", self->value));
    AddViewerString(strings, GetViewerStringPointer("m_spBoolData", self->data));
}

// ---- NiBoolData

// 0x81D670: GuaranteeKeysAtStartAndEnd(start, end) (the caller checks for keys)
void __fastcall BoolData_GuaranteeKeysImpl(NiBoolData* self, re::Edx, float start, float end) {
    re::Ptr32<NiAnimationKey> keys = self->bools.keys;
    Key_GuaranteeKeysAtStartAndEnd(BOOLKEY, self->bools.type, &keys, &self->bools.numKeys, start, end);
    self->bools.keys = keys;
}

// 0x81D6B0
const RTTI* __fastcall BoolData_GetRTTI(NiBoolData*, re::Edx) {
    return &NiBoolData_RTTI;
}

// 0x81D6C0: CreateObject
NiObject* __cdecl BoolData_CreateObject() {
    auto* self = static_cast<NiBoolData*>(NiNew(0x18));
    if (self == nullptr) {
        return nullptr;
    }
    RefObject_Construct(self);
    self->bools.numKeys = 0;
    self->bools.keys = nullptr;
    self->bools.type = NOINTERP;
    self->bools.size = 0;
    self->vtable = reinterpret_cast<void*>(NiBoolData_vtable);
    return self;
}

// 0x81D700: IsEqual (the keys compared one by one through the key table)
uint8_t __fastcall BoolData_IsEqual(NiBoolData* self, re::Edx, NiBoolData* other) {
    if (!Object_IsEqual(self, other)) {
        return 0;
    }
    if (self->bools.numKeys != other->bools.numKeys || self->bools.type != other->bools.type ||
        self->bools.size != other->bools.size) {
        return 0;
    }
    EqualFunction equal = ms_equal[BoolIndex(self->bools.type)];
    for (uint32_t i = 0; i < self->bools.numKeys; i++) {
        uint32_t offset = self->bools.size * i;
        const NiAnimationKey* key0 =
            reinterpret_cast<const NiAnimationKey*>(re::PtrCast<uintptr_t>(self->bools.keys) + offset);
        const NiAnimationKey* key1 =
            reinterpret_cast<const NiAnimationKey*>(re::PtrCast<uintptr_t>(other->bools.keys) + offset);
        if (!equal(key0, key1)) {
            return 0;
        }
    }
    return 1;
}

// 0x81D780: ReplaceAnim(keys, count, type): the old keys deleted through the key table; none
// when either is missing
void __fastcall BoolData_ReplaceAnimImpl(NiBoolData* self, re::Edx, NiAnimationKey* keys, uint32_t numKeys,
                                         KeyType type) {
    if (self->bools.keys != nullptr) {
        ms_delete[BoolIndex(self->bools.type)](self->bools.keys);
    }
    if (keys != nullptr && numKeys != 0) {
        self->bools.keys = keys;
        self->bools.size = ms_keySizes[BoolIndex(type)];
        self->bools.numKeys = numKeys;
        self->bools.type = type;
        return;
    }
    self->bools.numKeys = 0;
    self->bools.keys = nullptr;
    self->bools.type = NOINTERP;
    self->bools.size = 0;
}

// 0x81D7E0: GetSequenceData(start, end): a deep copy with the keys cut to the span
re::Ptr32<NiBoolData>* __fastcall BoolData_GetSequenceDataImpl(NiBoolData* self, re::Edx, re::Ptr32<NiBoolData>* result, float start,
                                                     float end) {
    re::Ptr32<NiObject> copy = nullptr;
    NiObject* made = *Object_CreateDeepCopy(self, &copy);
    *result = static_cast<NiBoolData*>(made);
    AddRefImport(made);
    ReleaseRef(copy);
    if (self->bools.numKeys != 0) {
        re::Ptr32<NiAnimationKey> newKeys = nullptr;
        uint32_t newNumKeys = 0;
        Key_CopySequence(BOOLKEY, self->bools.type, self->bools.keys, self->bools.numKeys, start, end, &newKeys,
                         &newNumKeys);
        BoolData_ReplaceAnim(*result, newKeys, newNumKeys, self->bools.type);
    }
    return result;
}

// 0x81D890: LoadBinary (the type from 10.1.0.104 on, step keys before; the key size set before
// the keys are made)
void __fastcall BoolData_LoadBinary(NiBoolData* self, re::Edx, void* stream) {
    Object_LoadBinary(self, stream);
    uint32_t numKeys;
    LoadBytes(stream, &numKeys, 4);
    if (numKeys == 0) {
        return;
    }
    KeyType type = STEPKEY;
    if (scene::StreamVersion(stream) >= 0x0A010068) {
        LoadBytes(stream, &type, 4);
    }
    self->bools.size = ms_keySizes[BoolIndex(type)];
    NiAnimationKey* keys = ms_create[BoolIndex(type)](stream, numKeys);
    ms_fillDerived[BoolIndex(type)](keys, numKeys, self->bools.size);
    BoolData_ReplaceAnim(self, keys, numKeys, type);
}

// 0x81D960: SaveBinary
void __fastcall BoolData_SaveBinary(NiBoolData* self, re::Edx, void* stream) {
    Object_SaveBinaryNone(self, stream);
    SaveBytes(stream, &self->bools.numKeys, 4);
    if (self->bools.numKeys != 0) {
        SaveBytes(stream, &self->bools.type, 4);
        ms_save[BoolIndex(self->bools.type)](stream, self->bools.keys, self->bools.numKeys);
    }
}

// 0x81D9E0: the deleting destructor (the keys freed as a NiNew[] array of 8-byte keys, not
// through the key table)
NiBoolData* __fastcall BoolData_DeletingDestructor(NiBoolData* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiBoolData_vtable);
    if (self->bools.keys != nullptr) {
        NiFreeArray(re::PtrCast<uint8_t*>(self->bools.keys) - 4, 8);
    }
    Object_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x18);
    }
    return self;
}

// 0x81DA20: GetViewerStrings
void __fastcall BoolData_GetViewerStrings(NiBoolData* self, re::Edx, NiTArray<char*>* strings) {
    Object_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBoolData_RTTI.name));
    AddViewerString(strings, GetViewerStringUint("m_uiNumKeys", self->bools.numKeys));
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC32_BOOL_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

// (lone jmps: the timeline's LoadBinary, RegisterStreamables, SaveBinary and IsEqual are
// NiBoolInterpolator's)
RE_UNHOOKED("NiBoolTimelineInterpolator::LoadBinary (thunk)", 0x0081CC40);
RE_UNHOOKED("NiBoolTimelineInterpolator::RegisterStreamables (thunk)", 0x0081CC50);
RE_UNHOOKED("NiBoolTimelineInterpolator::SaveBinary (thunk)", 0x0081CC60);
RE_UNHOOKED("NiBoolTimelineInterpolator::IsEqual (thunk)", 0x0081CC70);

NIC32_BOOL_REVERSED("NiBoolTimelineInterpolator::GetRTTI", 0x0081CC80, &ni::anim::Timeline_GetRTTI);
NIC32_BOOL_REVERSED("NiBoolTimelineInterpolator::Update", 0x0081CC90, &ni::anim::Timeline_Update);
NIC32_BOOL_REVERSED("NiBoolTimelineInterpolator::DeletingDestructor", 0x0081CE20,
                    &ni::anim::Timeline_DeletingDestructor);
NIC32_BOOL_REVERSED("NiBoolTimelineInterpolator::CreateClone", 0x0081CE70, &ni::anim::Timeline_CreateClone);
NIC32_BOOL_REVERSED("NiBoolTimelineInterpolator::CreateObject", 0x0081CED0, &ni::anim::Timeline_CreateObject);
NIC32_BOOL_REVERSED("NiBoolTimelineInterpolator::GetViewerStrings", 0x0081CF10,
                    &ni::anim::Timeline_GetViewerStrings);
NIC32_BOOL_REVERSED("NiBoolInterpolator::GetKeyContent", 0x0081CF60, &ni::anim::Bool_GetKeyContent);
NIC32_BOOL_REVERSED("NiBoolInterpolator::NiBoolInterpolator(data)", 0x0081CF70, &ni::anim::Bool_ConstructWithData);
NIC32_BOOL_REVERSED("NiBoolInterpolator::GetRTTI", 0x0081CFB0, &ni::anim::Bool_GetRTTI);
NIC32_BOOL_REVERSED("NiBoolInterpolator::NiBoolInterpolator(pose)", 0x0081CFC0, &ni::anim::Bool_ConstructWithPose);
NIC32_BOOL_REVERSED("NiBoolInterpolator::GetKeyStride", 0x0081CFF0, &ni::anim::Bool_GetKeyStride);
NIC32_BOOL_REVERSED("NiBoolInterpolator::GetChannelPosed", 0x0081D010, &ni::anim::Bool_GetChannelPosed);
NIC32_BOOL_REVERSED("NiBoolInterpolator::Collapse", 0x0081D030, &ni::anim::Bool_Collapse);
NIC32_BOOL_REVERSED("NiBoolInterpolator::SetPoseValue", 0x0081D110, &ni::anim::Bool_SetPoseValue);
NIC32_BOOL_REVERSED("NiBoolInterpolator::GuaranteeTimeRange", 0x0081D160, &ni::anim::Bool_GuaranteeTimeRange);
NIC32_BOOL_REVERSED("NiBoolInterpolator::CopyMembers", 0x0081D190, &ni::anim::Bool_CopyMembers);
NIC32_BOOL_REVERSED("NiBoolInterpolator::CreateObject", 0x0081D210, &ni::anim::Bool_CreateObject);
NIC32_BOOL_REVERSED("NiBoolInterpolator::RegisterStreamables", 0x0081D250, &ni::anim::Bool_RegisterStreamables);
NIC32_BOOL_REVERSED("NiBoolInterpolator::IsEqual", 0x0081D280, &ni::anim::Bool_IsEqual);
NIC32_BOOL_REVERSED("NiBoolInterpolator::GetSequenceInterpolator", 0x0081D2E0,
                    &ni::anim::Bool_GetSequenceInterpolator);
NIC32_BOOL_REVERSED("NiBoolInterpolator::Update", 0x0081D3A0, &ni::anim::Bool_Update);
NIC32_BOOL_REVERSED("NiBoolInterpolator::CreateClone", 0x0081D460, &ni::anim::Bool_CreateClone);
NIC32_BOOL_REVERSED("NiBoolInterpolator::LoadBinary", 0x0081D4D0, &ni::anim::Bool_LoadBinary);
NIC32_BOOL_REVERSED("NiBoolInterpolator::SaveBinary", 0x0081D550, &ni::anim::Bool_SaveBinary);
NIC32_BOOL_REVERSED("NiBoolInterpolator::GetViewerStrings", 0x0081D5A0, &ni::anim::Bool_GetViewerStrings);
NIC32_BOOL_REVERSED("NiBoolData::GuaranteeKeysAtStartAndEnd", 0x0081D670, &ni::anim::BoolData_GuaranteeKeysImpl);
NIC32_BOOL_REVERSED("NiBoolData::GetRTTI", 0x0081D6B0, &ni::anim::BoolData_GetRTTI);
NIC32_BOOL_REVERSED("NiBoolData::CreateObject", 0x0081D6C0, &ni::anim::BoolData_CreateObject);
NIC32_BOOL_REVERSED("NiBoolData::IsEqual", 0x0081D700, &ni::anim::BoolData_IsEqual);
NIC32_BOOL_REVERSED("NiBoolData::ReplaceAnim", 0x0081D780, &ni::anim::BoolData_ReplaceAnimImpl);
NIC32_BOOL_REVERSED("NiBoolData::GetSequenceData", 0x0081D7E0, &ni::anim::BoolData_GetSequenceDataImpl);
NIC32_BOOL_REVERSED("NiBoolData::LoadBinary", 0x0081D890, &ni::anim::BoolData_LoadBinary);
NIC32_BOOL_REVERSED("NiBoolData::SaveBinary", 0x0081D960, &ni::anim::BoolData_SaveBinary);
NIC32_BOOL_REVERSED("NiBoolData::DeletingDestructor", 0x0081D9E0, &ni::anim::BoolData_DeletingDestructor);
NIC32_BOOL_REVERSED("NiBoolData::GetViewerStrings", 0x0081DA20, &ni::anim::BoolData_GetViewerStrings);
