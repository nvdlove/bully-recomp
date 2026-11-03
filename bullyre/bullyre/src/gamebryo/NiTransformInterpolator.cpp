// NiTransformInterpolator (src/gamebryo/NiInterpolators.h, vtable 0x965C00, 0x3C bytes): a
// translation, rotation and scale sampled from a NiTransformData's three key channels (0
// position, 1 rotation, 2 scale), or posed (no data: the value it holds). NiQuatTransform's
// small helpers sit with it. Construction, cloning, streaming, comparison, the key channel
// queries, Update, Collapse (constant channels folded into the posed value), the sequence
// interpolator, viewer strings.
//
// Main-thread float maths at the game's 24-bit x87 precision.

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
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiTransformInterpolator_RTTI, 0x020E4B0C);

constexpr uintptr_t NiTransformInterpolator_vtable = 0x00965C00;

// the key sampling (NiPosKey / NiRotKey / NiFloatKey::GenInterp: the value at the time, the last
// index kept for the next call)
GAME_FN(NiPoint3*, __cdecl, PosKey_GenInterp, 0x007F8B50, NiPoint3* result, float time, NiAnimationKey* keys,
        KeyType type, uint32_t numKeys, uint32_t* lastIdx, uint8_t size);
GAME_FN(NiQuaternion*, __cdecl, RotKey_GenInterp, 0x007F9E90, NiQuaternion* result, float time,
        NiAnimationKey* keys, KeyType type, uint32_t numKeys, uint32_t* lastIdx, uint8_t size);
GAME_FN(float, __cdecl, FloatKey_GenInterp, 0x007F8300, float time, NiAnimationKey* keys, KeyType type,
        uint32_t numKeys, uint32_t* lastIdx, uint8_t size);
GAME_FN(uint8_t, __thiscall, Quaternion_Equal, 0x007F9E40, const NiQuaternion* self, const NiQuaternion* other);
GAME_FN(uint8_t, __thiscall, Point3_NotEqual, 0x0076BEB0, const NiPoint3* self, const NiPoint3* other);
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
GAME_FN(void, __thiscall, QuatTransform_LoadBinary, 0x00825B50, NiQuatTransform* self, void* stream);
GAME_FN(void, __thiscall, QuatTransform_SaveBinary, 0x00825C50, NiQuatTransform* self, void* stream);
GAME_FN(void, __thiscall, QuatTransform_GetViewerStrings, 0x00825CA0, NiQuatTransform* self,
        NiTArray<char*>* strings);
GAME_FN(NiObject*, __thiscall, Stream_ReadLinkIDObject, 0x0075A820, void* stream);
// NiTransformData's (src/gamebryo/NiTransformData.cpp)
GAME_FN(void, __thiscall, TransformData_GuaranteeKeysAtStartAndEnd, 0x0080F840, NiTransformData* self, float start,
        float end);
GAME_FN(re::Ptr32<NiTransformData>*, __thiscall, TransformData_GetSequenceData, 0x0080FCA0, NiTransformData* self,
        re::Ptr32<NiTransformData>* result, float start, float end);
GAME_FN(void, __thiscall, TransformData_ReplaceRotAnim, 0x0080FB70, NiTransformData* self, NiAnimationKey* keys,
        uint32_t numKeys, KeyType type);
GAME_FN(void, __thiscall, TransformData_ReplacePosAnim, 0x0080FBE0, NiTransformData* self, NiAnimationKey* keys,
        uint32_t numKeys, KeyType type);
GAME_FN(void, __thiscall, TransformData_ReplaceScaleAnim, 0x0080FC40, NiTransformData* self, NiAnimationKey* keys,
        uint32_t numKeys, KeyType type);
// (this file's, called where modes apply)
GAME_FN(uint8_t, __thiscall, QuatTransform_Equal, 0x00805210, const NiQuatTransform* self,
        const NiQuatTransform* other);
GAME_FN(void, __thiscall, TransformInterpolator_CopyMembers, 0x00805560, NiTransformInterpolator* self,
        NiTransformInterpolator* dest, void* cloning);
GAME_FN(void, __thiscall, TransformInterpolator_SetData, 0x00805780, NiTransformInterpolator* self,
        NiTransformData* data);

// the default value (each part invalid)
inline void SetInvalid(NiQuatTransform& t) {
    t.translate = kInvalidTranslate;
    t.rotate = kInvalidRotate;
    t.scale = kInvalidFloat;
}

// the members a new one gets (the constructors' and CreateClone's inlined copy)
inline void InitMembers(NiTransformInterpolator* self) {
    self->vtable = reinterpret_cast<void*>(NiTransformInterpolator_vtable);
    SetInvalid(self->value);
    self->data = nullptr;
    self->lastTransIdx = 0;
    self->lastRotIdx = 0;
    self->lastScaleIdx = 0;
}

// a value valid in any of its parts (the scale, the rotation's x, the translation's x)
inline uint8_t AnyValid(const NiQuatTransform& t) {
    return t.scale != kInvalidFloat || t.rotate.x != kInvalidFloat || t.translate.x != kInvalidFloat ? 1 : 0;
}

// ---- NiQuatTransform

// 0x805180: NiQuatTransform() (each part invalid)
NiQuatTransform* __fastcall QuatTransform_Construct(NiQuatTransform* self, re::Edx) {
    self->translate = kInvalidTranslate;
    self->rotate = kInvalidRotate;
    self->scale = kInvalidFloat;
    return self;
}

// 0x8051D0: IsTransformInvalid (scale, rotation and translation all invalid)
uint8_t __fastcall QuatTransform_IsTransformInvalid(const NiQuatTransform* self, re::Edx) {
    return AnyValid(*self) ? 0 : 1;
}

// 0x805210: operator==
uint8_t __fastcall QuatTransformEqual(const NiQuatTransform* self, re::Edx, const NiQuatTransform* other) {
    if (other->translate.x != self->translate.x || other->translate.y != self->translate.y ||
        other->translate.z != self->translate.z) {
        return 0;
    }
    if (!Quaternion_Equal(&self->rotate, &other->rotate)) {
        return 0;
    }
    if (other->scale != self->scale) {
        return 0;
    }
    return 1;
}

// ---- NiTransformInterpolator

// 0x805140: GetKeyChannelCount
uint16_t __fastcall GetKeyChannelCount(NiTransformInterpolator*, re::Edx) {
    return 3;
}

// 0x805150: GetKeyContent(channel): POSKEY, ROTKEY, FLOATKEY (NUMKEYCONTENTS past them)
uint32_t __fastcall GetKeyContent(NiTransformInterpolator*, re::Edx, uint16_t channel) {
    if (channel == 0) {
        return POSKEY;
    }
    if (channel == 1) {
        return ROTKEY;
    }
    return channel != 2 ? NUMKEYCONTENTS : FLOATKEY;
}

// the channel's keys (none past the three, or without data)
inline const KeyChannel* Channel(const NiTransformInterpolator* self, uint16_t channel) {
    if (channel == 0) {
        return self->data != nullptr ? &self->data->pos : nullptr;
    }
    if (channel == 1) {
        return self->data != nullptr ? &self->data->rot : nullptr;
    }
    if (channel == 2) {
        return self->data != nullptr ? &self->data->scale : nullptr;
    }
    return nullptr;
}

// 0x805280: NiTransformInterpolator(data)
NiTransformInterpolator* __fastcall ConstructWithData(NiTransformInterpolator* self, re::Edx, NiTransformData* data) {
    KeyBasedInterpolator_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiTransformInterpolator_vtable);
    SetInvalid(self->value);
    self->data = data;
    AddRefImport(data);
    self->lastTransIdx = 0;
    self->lastRotIdx = 0;
    self->lastScaleIdx = 0;
    return self;
}

const RTTI* __fastcall GetRTTI(NiTransformInterpolator*, re::Edx) {
    return &NiTransformInterpolator_RTTI;
}

// 0x805320: NiTransformInterpolator(pose) (the transform by value)
NiTransformInterpolator* __fastcall ConstructWithPose(NiTransformInterpolator* self, re::Edx, NiQuatTransform pose) {
    KeyBasedInterpolator_Construct(self);
    self->value = pose;
    self->vtable = reinterpret_cast<void*>(NiTransformInterpolator_vtable);
    self->data = nullptr;
    self->lastTransIdx = 0;
    self->lastRotIdx = 0;
    self->lastScaleIdx = 0;
    return self;
}

// 0x805360: GetKeyCount(channel)
uint32_t __fastcall GetKeyCount(NiTransformInterpolator* self, re::Edx, uint16_t channel) {
    const KeyChannel* c = Channel(self, channel);
    return c != nullptr ? c->numKeys : 0;
}

// 0x8053B0: GetKeyType(channel)
uint32_t __fastcall GetKeyType(NiTransformInterpolator* self, re::Edx, uint16_t channel) {
    const KeyChannel* c = Channel(self, channel);
    return c != nullptr ? c->type : 0;
}

// 0x805400: GetKeyArray(channel)
NiAnimationKey* __fastcall GetKeyArray(NiTransformInterpolator* self, re::Edx, uint16_t channel) {
    const KeyChannel* c = Channel(self, channel);
    return c != nullptr ? c->keys : nullptr;
}

// 0x805450: GetKeyStride(channel)
uint8_t __fastcall GetKeyStride(NiTransformInterpolator* self, re::Edx, uint16_t channel) {
    const KeyChannel* c = Channel(self, channel);
    return c != nullptr ? c->size : 0;
}

// 0x8054A0: GuaranteeTimeRange(start, end): keys made at both ends
void __fastcall GuaranteeTimeRange(NiTransformInterpolator* self, re::Edx, float start, float end) {
    if (self->data != nullptr) {
        TransformData_GuaranteeKeysAtStartAndEnd(self->data, start, end);
    }
}

// 0x8054D0: GetChannelPosed(channel): no keys in it (by GetKeyCount, slot 38) and a valid value
uint8_t __fastcall GetChannelPosed(NiTransformInterpolator* self, re::Edx, uint32_t channel) {
    float value;
    switch (uint16_t(channel)) {
    case 0:
        if (self->data != nullptr && Virtual<uint32_t>(self, 38, channel) != 0) {
            return 0;
        }
        value = self->value.translate.x;
        break;
    case 1:
        if (self->data != nullptr && Virtual<uint32_t>(self, 38, channel) != 0) {
            return 0;
        }
        value = self->value.rotate.x;
        break;
    case 2:
        if (self->data != nullptr && Virtual<uint32_t>(self, 38, channel) != 0) {
            return 0;
        }
        value = self->value.scale;
        break;
    default:
        return 0;
    }
    return value != kInvalidFloat ? 1 : 0;
}

// 0x805560: CopyMembers (the value, the data shared, the indices)
void __fastcall CopyMembers(NiTransformInterpolator* self, re::Edx, NiTransformInterpolator* dest, void* cloning) {
    KeyBasedInterpolator_CopyMembers(self, dest, cloning);
    dest->value = self->value;
    AssignReleasingFirst(dest->data, self->data);
    dest->lastTransIdx = self->lastTransIdx;
    dest->lastRotIdx = self->lastRotIdx;
    dest->lastScaleIdx = self->lastScaleIdx;
}

// 0x8055E0: CreateObject (the stream's factory)
NiObject* __cdecl CreateObject() {
    void* memory = NiNew(0x3C);
    if (memory == nullptr) {
        return nullptr;
    }
    NiTransformInterpolator* self = static_cast<NiTransformInterpolator*>(memory);
    KeyBasedInterpolator_Construct(self);
    InitMembers(self);
    return self;
}

// 0x805660: LoadBinary (the value, then the data's link read and resolved at once)
void __fastcall LoadBinary(NiTransformInterpolator* self, re::Edx, void* stream) {
    Interpolator_LoadBinary(self, stream);
    QuatTransform_LoadBinary(&self->value, stream);
    NiObject* data = Stream_ReadLinkIDObject(stream);
    AssignReleasingFirst(self->data, data);
}

// 0x8056C0: RegisterStreamables
uint8_t __fastcall RegisterStreamables(NiTransformInterpolator* self, re::Edx, void* stream) {
    if (!Interpolator_RegisterStreamables(self, stream)) {
        return 0;
    }
    if (self->data != nullptr) {
        Virtual<uint8_t>(self->data, 6, stream);
    }
    return 1;
}

// 0x8056F0: SaveBinary
void __fastcall SaveBinary(NiTransformInterpolator* self, re::Edx, void* stream) {
    Interpolator_SaveBinary(self, stream);
    QuatTransform_SaveBinary(&self->value, stream);
    Virtual<void>(stream, 12, self->data);   // SaveLinkID
}

// 0x805720: IsEqual (the values, and the data both there and equal, or both missing)
uint8_t __fastcall IsEqual(NiTransformInterpolator* self, re::Edx, NiTransformInterpolator* other) {
    if (!Interpolator_IsEqual(self, other)) {
        return 0;
    }
    if (!QuatTransform_Equal(&self->value, &other->value)) {
        return 0;
    }
    NiTransformData* data = self->data;
    if (data != nullptr) {
        if (other->data == nullptr) {
            return 0;
        }
    } else {
        if (other->data != nullptr) {
            return 0;
        }
        return 1;
    }
    if (!Virtual<uint8_t>(data, 8, other->data)) {
        return 0;
    }
    return 1;
}

// 0x805780: SetData(data) (the indices back to 0)
void __fastcall SetData(NiTransformInterpolator* self, re::Edx, NiTransformData* data) {
    AssignReleasingFirst(self->data, data);
    self->lastRotIdx = 0;
    self->lastScaleIdx = 0;
    self->lastTransIdx = 0;
}

// 0x8057E0: the deleting destructor
NiTransformInterpolator* __fastcall DeletingDestructor(NiTransformInterpolator* self, re::Edx, uint32_t flags) {
    ReleaseRef(self->data);
    KeyBasedInterpolator_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x3C);
    }
    return self;
}

// a channel's first and last key times taken into the range (when it has keys)
inline void TakeRange(const NiAnimationKey* keys, uint32_t numKeys, uint8_t size, float* begin, float* end) {
    float first = keys->time;
    if (first < *begin) {
        *begin = first;
    }
    float last = KeyAt(keys, numKeys - 1, size)->time;
    if (*end < last) {
        *end = last;
    }
}

// 0x805830: GetActiveTimeRange(&begin, &end): the keys' span over the three channels (an Euler
// rotation key's three sequences each); none: 0 and 0
void __fastcall GetActiveTimeRange(NiTransformInterpolator* self, re::Edx, float* begin, float* end) {
    NiTransformData* data = self->data;
    *begin = 3.40282347e+38f;
    *end = kInvalidFloat;
    uint8_t found = 0;
    if (data != nullptr) {
        if (data->pos.numKeys != 0) {
            TakeRange(data->pos.keys, data->pos.numKeys, data->pos.size, begin, end);
            found = 1;
        }
        if (data->rot.numKeys != 0) {
            if (data->rot.type == EULERKEY) {
                const NiEulerRotKey* euler = re::PtrCast<const NiEulerRotKey*>(data->rot.keys);
                for (int i = 0; i < 3; i++) {
                    if (euler->numKeys[i] != 0) {
                        TakeRange(euler->keys[i], euler->numKeys[i], euler->size[i], begin, end);
                        found = 1;
                    }
                }
            } else {
                TakeRange(data->rot.keys, data->rot.numKeys, data->rot.size, begin, end);
                found = 1;
            }
        }
        if (data->scale.numKeys != 0) {
            TakeRange(data->scale.keys, data->scale.numKeys, data->scale.size, begin, end);
            return;
        }
    }
    if (!found) {
        *begin = 0.0f;
        *end = 0.0f;
    }
}

// 0x805A20: GetSequenceInterpolator(start, end): the base's copy with the keys cut to the span
NiInterpolator* __fastcall GetSequenceInterpolator(NiTransformInterpolator* self, re::Edx, float start, float end) {
    NiTransformInterpolator* clone =
        static_cast<NiTransformInterpolator*>(Interpolator_GetSequenceInterpolator(self, start, end));
    if (self->data == nullptr) {
        return clone;
    }
    re::Ptr32<NiTransformData> data = nullptr;
    TransformData_GetSequenceData(self->data, &data, start, end);
    TransformInterpolator_SetData(clone, data);
    clone->lastTransIdx = 0;
    clone->lastRotIdx = 0;
    clone->lastScaleIdx = 0;
    ReleaseRef(data);
    return clone;
}

// 0x805AA0: Update(time, target, value&) (slot 18): the same time as last: the value kept; else
// each channel with keys sampled. False when no part of the value is valid (the time then not
// remembered)
uint8_t __fastcall Update(NiTransformInterpolator* self, re::Edx, float time, NiObjectNET*, NiQuatTransform* value) {
    if (time == self->lastTime) {
        *value = self->value;
        return AnyValid(self->value);
    }
    NiTransformData* data = self->data;
    if (data != nullptr && data->pos.numKeys != 0) {
        NiPoint3 p;
        const NiPoint3* r = PosKey_GenInterp(&p, time, data->pos.keys, data->pos.type, data->pos.numKeys,
                                             &self->lastTransIdx, data->pos.size);
        self->value.translate = *r;
    }
    data = self->data;
    if (data != nullptr && data->rot.numKeys != 0) {
        NiQuaternion q;
        const NiQuaternion* r = RotKey_GenInterp(&q, time, data->rot.keys, data->rot.type, data->rot.numKeys,
                                                 &self->lastRotIdx, data->rot.size);
        self->value.rotate = *r;
    }
    data = self->data;
    if (data != nullptr && data->scale.numKeys != 0) {
        self->value.scale = FloatKey_GenInterp(time, data->scale.keys, data->scale.type, data->scale.numKeys,
                                               &self->lastScaleIdx, data->scale.size);
    }
    *value = self->value;
    if (!AnyValid(self->value)) {
        return 0;
    }
    self->lastTime = time;
    return 1;
}

// 0x805C50: Collapse (slot 30): a channel without keys keeps the posed part; one with a single
// key, or linear / step keys all the same (an Euler rotation's always kept), is cleared and its
// value posed. The data let go when no keys are left
void __fastcall Collapse(NiTransformInterpolator* self, re::Edx) {
    NiTransformData* data = self->data;
    if (data == nullptr) {
        return;
    }
    NiQuatTransform posed;
    SetInvalid(posed);

    // position
    uint32_t numPos = data->pos.numKeys;
    const NiAnimationKey* posKeys = data->pos.keys;
    KeyType posType = data->pos.type;
    bool posCollapse = false;
    NiPoint3 p{};
    if (numPos == 0) {
        TransformData_ReplacePosAnim(data, nullptr, 0, NOINTERP);
    } else {
        p = reinterpret_cast<const NiPosKey*>(posKeys)->pos;
        if (numPos == 1) {
            posCollapse = true;
        } else if (posType == LINKEY || posType == STEPKEY) {
            uint8_t same = 1;
            uint32_t i = 1;
            for (;;) {
                if (i >= numPos) {
                    posCollapse = same != 0;
                    break;
                }
                if (Point3_NotEqual(&KeyAt<NiPosKey>(posKeys, i, data->pos.size)->pos, &p)) {
                    same = 0;
                }
                i++;
                if (!same) {
                    break;
                }
            }
        }
    }
    if (posCollapse) {
        TransformData_ReplacePosAnim(data, nullptr, 0, NOINTERP);
        posed.translate = p;
    } else if (self->value.translate.x != kInvalidFloat) {
        posed.translate = self->value.translate;
    }

    // rotation
    data = self->data;
    uint32_t numRot = data->rot.numKeys;
    KeyType rotType = data->rot.type;
    const NiAnimationKey* rotKeys = data->rot.keys;
    bool rotCollapse = false;
    NiQuaternion q{};
    if (numRot == 0) {
        TransformData_ReplaceRotAnim(data, nullptr, 0, NOINTERP);
    } else {
        q = reinterpret_cast<const NiRotKey*>(rotKeys)->quat;
        if (numRot == 1 && rotType != EULERKEY) {
            rotCollapse = true;
        } else if (rotType == LINKEY || rotType == STEPKEY) {
            uint8_t same = 1;
            uint32_t i = 1;
            for (;;) {
                if (i >= data->rot.numKeys) {
                    rotCollapse = same != 0;
                    break;
                }
                const NiQuaternion& k = KeyAt<NiRotKey>(rotKeys, i, data->rot.size)->quat;
                if (k.w != q.w || k.x != q.x || k.y != q.y || k.z != q.z) {
                    same = 0;
                }
                i++;
                if (!same) {
                    break;
                }
            }
        }
    }
    if (rotCollapse) {
        TransformData_ReplaceRotAnim(data, nullptr, 0, NOINTERP);
        posed.rotate = q;
    } else if (self->value.rotate.x != kInvalidFloat) {
        posed.rotate = self->value.rotate;
    }

    // scale
    data = self->data;
    uint32_t numScale = data->scale.numKeys;
    KeyType scaleType = data->scale.type;
    const NiAnimationKey* scaleKeys = data->scale.keys;
    bool scaleCollapse = false;
    float s = 0.0f;
    if (numScale == 0) {
        TransformData_ReplaceScaleAnim(data, nullptr, 0, NOINTERP);
    } else {
        s = reinterpret_cast<const NiFloatKey*>(scaleKeys)->value;
        if (numScale == 1) {
            scaleCollapse = true;
        } else if (scaleType == LINKEY || scaleType == STEPKEY) {
            uint8_t same = 1;
            uint32_t i = 1;
            for (;;) {
                if (i >= numScale) {
                    scaleCollapse = same != 0;
                    break;
                }
                if (KeyAt<NiFloatKey>(scaleKeys, i, data->scale.size)->value != s) {
                    same = 0;
                }
                i++;
                if (!same) {
                    break;
                }
            }
        }
    }
    if (scaleCollapse) {
        TransformData_ReplaceScaleAnim(data, nullptr, 0, NOINTERP);
        posed.scale = s;
    } else if (self->value.scale != kInvalidFloat) {
        posed.scale = self->value.scale;
    }

    data = self->data;
    if (data->pos.numKeys == 0 && data->rot.numKeys == 0 && data->scale.numKeys == 0 && data != nullptr) {
        ReleaseRef(data);
        self->data = nullptr;
    }
    self->value = posed;
}

// 0x805FE0: CreateClone (a new one, CopyMembers onto it; a failed allocation still copied onto
// null as the game has it)
NiTransformInterpolator* __fastcall CreateClone(NiTransformInterpolator* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x3C);
    if (memory == nullptr) {
        TransformInterpolator_CopyMembers(self, nullptr, cloning);
        return nullptr;
    }
    NiTransformInterpolator* clone = static_cast<NiTransformInterpolator*>(memory);
    KeyBasedInterpolator_Construct(clone);
    InitMembers(clone);
    TransformInterpolator_CopyMembers(self, clone, cloning);
    return clone;
}

// 0x806090: GetViewerStrings
void __fastcall GetViewerStrings(NiTransformInterpolator* self, re::Edx, NiTArray<char*>* strings) {
    KeyBasedInterpolator_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiTransformInterpolator_RTTI.name));
    QuatTransform_GetViewerStrings(&self->value, strings);
    if (self->data != nullptr) {
        Virtual<void>(self->data, 9, strings);
    }
}

// 0x8105B0: SetPoseValue(pose) (by value): each valid part posed, that channel's keys dropped
// (rotation, scale, then translation)
void __fastcall SetPoseValue(NiTransformInterpolator* self, re::Edx, NiQuatTransform pose) {
    if (pose.rotate.x != kInvalidFloat) {
        self->value.rotate = pose.rotate;
        if (self->data != nullptr) {
            TransformData_ReplaceRotAnim(self->data, nullptr, 0, NOINTERP);
        }
    }
    if (pose.scale != kInvalidFloat) {
        self->value.scale = pose.scale;
        if (self->data != nullptr) {
            TransformData_ReplaceScaleAnim(self->data, nullptr, 0, NOINTERP);
        }
    }
    if (pose.translate.x != kInvalidFloat) {
        self->value.translate = pose.translate;
        if (self->data != nullptr) {
            TransformData_ReplacePosAnim(self->data, nullptr, 0, NOINTERP);
        }
    }
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC30_XFINTERP_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIC30_XFINTERP_REVERSED("NiTransformInterpolator::GetKeyChannelCount", 0x00805140, &ni::anim::GetKeyChannelCount);
NIC30_XFINTERP_REVERSED("NiTransformInterpolator::GetKeyContent", 0x00805150, &ni::anim::GetKeyContent);
NIC30_XFINTERP_REVERSED("NiQuatTransform::NiQuatTransform", 0x00805180, &ni::anim::QuatTransform_Construct);
NIC30_XFINTERP_REVERSED("NiQuatTransform::IsTransformInvalid", 0x008051D0,
                        &ni::anim::QuatTransform_IsTransformInvalid);
NIC30_XFINTERP_REVERSED("NiQuatTransform::operator==", 0x00805210, &ni::anim::QuatTransformEqual);
NIC30_XFINTERP_REVERSED("NiTransformInterpolator::NiTransformInterpolator(data)", 0x00805280,
                        &ni::anim::ConstructWithData);
NIC30_XFINTERP_REVERSED("NiTransformInterpolator::GetRTTI", 0x00805310, &ni::anim::GetRTTI);
NIC30_XFINTERP_REVERSED("NiTransformInterpolator::NiTransformInterpolator(pose)", 0x00805320,
                        &ni::anim::ConstructWithPose);
NIC30_XFINTERP_REVERSED("NiTransformInterpolator::GetKeyCount", 0x00805360, &ni::anim::GetKeyCount);
NIC30_XFINTERP_REVERSED("NiTransformInterpolator::GetKeyType", 0x008053B0, &ni::anim::GetKeyType);
NIC30_XFINTERP_REVERSED("NiTransformInterpolator::GetKeyArray", 0x00805400, &ni::anim::GetKeyArray);
NIC30_XFINTERP_REVERSED("NiTransformInterpolator::GetKeyStride", 0x00805450, &ni::anim::GetKeyStride);
NIC30_XFINTERP_REVERSED("NiTransformInterpolator::GuaranteeTimeRange", 0x008054A0, &ni::anim::GuaranteeTimeRange);
NIC30_XFINTERP_REVERSED("NiTransformInterpolator::GetChannelPosed", 0x008054D0, &ni::anim::GetChannelPosed);
NIC30_XFINTERP_REVERSED("NiTransformInterpolator::CopyMembers", 0x00805560, &ni::anim::CopyMembers);
NIC30_XFINTERP_REVERSED("NiTransformInterpolator::CreateObject", 0x008055E0, &ni::anim::CreateObject);
NIC30_XFINTERP_REVERSED("NiTransformInterpolator::LoadBinary", 0x00805660, &ni::anim::LoadBinary);
NIC30_XFINTERP_REVERSED("NiTransformInterpolator::RegisterStreamables", 0x008056C0, &ni::anim::RegisterStreamables);
NIC30_XFINTERP_REVERSED("NiTransformInterpolator::SaveBinary", 0x008056F0, &ni::anim::SaveBinary);
NIC30_XFINTERP_REVERSED("NiTransformInterpolator::IsEqual", 0x00805720, &ni::anim::IsEqual);
NIC30_XFINTERP_REVERSED("NiTransformInterpolator::SetData", 0x00805780, &ni::anim::SetData);
NIC30_XFINTERP_REVERSED("NiTransformInterpolator::DeletingDestructor", 0x008057E0, &ni::anim::DeletingDestructor);
NIC30_XFINTERP_REVERSED("NiTransformInterpolator::GetActiveTimeRange", 0x00805830, &ni::anim::GetActiveTimeRange);
NIC30_XFINTERP_REVERSED("NiTransformInterpolator::GetSequenceInterpolator", 0x00805A20,
                        &ni::anim::GetSequenceInterpolator);
NIC30_XFINTERP_REVERSED("NiTransformInterpolator::Update", 0x00805AA0, &ni::anim::Update);
NIC30_XFINTERP_REVERSED("NiTransformInterpolator::Collapse", 0x00805C50, &ni::anim::Collapse);
NIC30_XFINTERP_REVERSED("NiTransformInterpolator::CreateClone", 0x00805FE0, &ni::anim::CreateClone);
NIC30_XFINTERP_REVERSED("NiTransformInterpolator::GetViewerStrings", 0x00806090, &ni::anim::GetViewerStrings);
NIC30_XFINTERP_REVERSED("NiTransformInterpolator::SetPoseValue", 0x008105B0, &ni::anim::SetPoseValue);
