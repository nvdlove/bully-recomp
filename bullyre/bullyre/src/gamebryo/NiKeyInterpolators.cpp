// NiQuaternionInterpolator (vtable 0x965FB0, 0x24 bytes: a rotation from a NiRotData's keys) and
// NiPoint3Interpolator (vtable 0x966088, 0x20 bytes: a point from a NiPosData's keys), both
// key-based with one channel, posed (no data) holding their value. Construction, cloning,
// streaming, comparison, the key queries, Update, Collapse, the sequence interpolators,
// viewer strings. (Their other slots, LoadBinary and the destructor of the quaternion one and
// GetKeyStride, are the next chunk's.)

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
using scene::GetViewerStringPointer;
using scene::Point3_GetViewerString;
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiQuaternionInterpolator_RTTI, 0x020E4C44);
GAME_VAR(RTTI, NiPoint3Interpolator_RTTI, 0x020E4CAC);

constexpr uintptr_t NiQuaternionInterpolator_vtable = 0x00965FB0;
constexpr uintptr_t NiPoint3Interpolator_vtable = 0x00966088;

GAME_VAR(NiPoint3, kInvalidPoint, 0x00BC8C48);   // NiPoint3Interpolator's invalid value

GAME_FN(NiPoint3*, __cdecl, PosKey_GenInterp, 0x007F8B50, NiPoint3* result, float time, NiAnimationKey* keys,
        KeyType type, uint32_t numKeys, uint32_t* lastIdx, uint8_t size);
GAME_FN(NiQuaternion*, __cdecl, RotKey_GenInterp, 0x007F9E90, NiQuaternion* result, float time,
        NiAnimationKey* keys, KeyType type, uint32_t numKeys, uint32_t* lastIdx, uint8_t size);
GAME_FN(uint8_t, __thiscall, Quaternion_Equal, 0x007F9E40, const NiQuaternion* self, const NiQuaternion* other);
GAME_FN(uint8_t, __thiscall, Quaternion_NotEqual, 0x006B1C10, const NiQuaternion* self, const NiQuaternion* other);
GAME_FN(void, __thiscall, Quaternion_SaveBinary, 0x00776310, const NiQuaternion* self, void* stream);
GAME_FN(char*, __thiscall, Quaternion_GetViewerString, 0x007763B0, const NiQuaternion* self, const char* prefix);
GAME_FN(uint8_t, __thiscall, Point3_NotEqual, 0x0076BEB0, const NiPoint3* self, const NiPoint3* other);
GAME_FN(void, __thiscall, Point3_LoadBinary, 0x007768A0, NiPoint3* self, void* stream);
GAME_FN(void, __thiscall, Point3_SaveBinary, 0x00762550, const NiPoint3* self, void* stream);
GAME_FN(NiObject*, __thiscall, Stream_ReadLinkIDObject, 0x0075A820, void* stream);
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
// the data's (src/gamebryo/NiKeyData.cpp)
GAME_FN(void, __thiscall, PosData_GuaranteeKeysAtStartAndEnd, 0x008080E0, void* self, float start, float end);
GAME_FN(re::Ptr32<NiRotData>*, __thiscall, RotData_GetSequenceData, 0x00807450, NiRotData* self, re::Ptr32<NiRotData>* result,
        float start, float end);
GAME_FN(re::Ptr32<NiPosData>*, __thiscall, PosData_GetSequenceData, 0x008082B0, NiPosData* self, re::Ptr32<NiPosData>* result,
        float start, float end);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, QuaternionInterpolator_CopyMembers, 0x00807B30, NiQuaternionInterpolator* self,
        NiQuaternionInterpolator* dest, void* cloning);
GAME_FN(void, __thiscall, Point3Interpolator_CopyMembers, 0x00808700, NiPoint3Interpolator* self,
        NiPoint3Interpolator* dest, void* cloning);

// a data pointer compared as both IsEqual have it: both there and equal (slot 8), or both missing
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

inline uint8_t PointIsInvalid(const NiPoint3& p) {
    return kInvalidPoint.x == p.x && kInvalidPoint.y == p.y && kInvalidPoint.z == p.z ? 1 : 0;
}

// ---- NiQuaternionInterpolator

inline void QuatInitMembers(NiQuaternionInterpolator* self) {
    self->vtable = reinterpret_cast<void*>(NiQuaternionInterpolator_vtable);
    self->value = kInvalidQuaternion;
    self->data = nullptr;
    self->lastIdx = 0;
}

// 0x807950: GetKeyContent
uint32_t __fastcall Quat_GetKeyContent(NiQuaternionInterpolator*, re::Edx, uint16_t) {
    return ROTKEY;
}

const RTTI* __fastcall Quat_GetRTTI(NiQuaternionInterpolator*, re::Edx) {
    return &NiQuaternionInterpolator_RTTI;
}

// 0x807970: GetKeyCount
uint32_t __fastcall Quat_GetKeyCount(NiQuaternionInterpolator* self, re::Edx, uint16_t) {
    return self->data != nullptr ? self->data->rot.numKeys : 0;
}

// 0x807980: GetKeyType
uint32_t __fastcall Quat_GetKeyType(NiQuaternionInterpolator* self, re::Edx, uint16_t) {
    return self->data != nullptr ? self->data->rot.type : 0;
}

// 0x807990: GetKeyArray
NiAnimationKey* __fastcall Quat_GetKeyArray(NiQuaternionInterpolator* self, re::Edx, uint16_t) {
    return self->data != nullptr ? self->data->rot.keys : nullptr;
}

// 0x8079A0: GetChannelPosed (no data and a valid value)
uint8_t __fastcall Quat_GetChannelPosed(NiQuaternionInterpolator* self, re::Edx, uint16_t) {
    if (self->data != nullptr) {
        return 0;
    }
    return Quaternion_Equal(&self->value, &kInvalidQuaternion) == 0 ? 1 : 0;
}

// 0x8079C0: Collapse (slot 30): no keys: the invalid value; one key (not an Euler one), or linear /
// step keys all the same: that rotation posed. Either way the data let go
void __fastcall Quat_Collapse(NiQuaternionInterpolator* self, re::Edx) {
    NiRotData* data = self->data;
    if (data == nullptr) {
        return;
    }
    uint32_t numKeys = data->rot.numKeys;
    uint8_t size = data->rot.size;
    const NiAnimationKey* keys = data->rot.keys;
    KeyType type = data->rot.type;
    if (numKeys == 0) {
        ReleaseRef(data);
        self->data = nullptr;
        self->value = kInvalidQuaternion;
        return;
    }
    NiQuaternion q = reinterpret_cast<const NiRotKey*>(keys)->quat;
    if (numKeys != 1 || type == EULERKEY) {
        if (type != LINKEY && type != STEPKEY) {
            return;
        }
        uint8_t same = 1;
        for (uint32_t i = 1; i < numKeys; i++) {
            if (Quaternion_NotEqual(&KeyAt<NiRotKey>(keys, i, size)->quat, &q)) {
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
    self->value = q;
}

// 0x807B00: GuaranteeTimeRange(start, end) (through NiPosData's, 0x8080E0, as the game has it)
void __fastcall Quat_GuaranteeTimeRange(NiQuaternionInterpolator* self, re::Edx, float start, float end) {
    if (self->data != nullptr) {
        PosData_GuaranteeKeysAtStartAndEnd(self->data, start, end);
    }
}

// 0x807B30: CopyMembers
void __fastcall Quat_CopyMembers(NiQuaternionInterpolator* self, re::Edx, NiQuaternionInterpolator* dest,
                                 void* cloning) {
    KeyBasedInterpolator_CopyMembers(self, dest, cloning);
    dest->value = self->value;
    AssignReleasingFirst(dest->data, self->data);
    dest->lastIdx = self->lastIdx;
}

// 0x807BC0: CreateObject
NiObject* __cdecl Quat_CreateObject() {
    void* memory = NiNew(0x24);
    if (memory == nullptr) {
        return nullptr;
    }
    NiQuaternionInterpolator* self = static_cast<NiQuaternionInterpolator*>(memory);
    KeyBasedInterpolator_Construct(self);
    QuatInitMembers(self);
    return self;
}

// 0x807C20: RegisterStreamables
uint8_t __fastcall Quat_RegisterStreamables(NiQuaternionInterpolator* self, re::Edx, void* stream) {
    if (!Interpolator_RegisterStreamables(self, stream)) {
        return 0;
    }
    if (self->data != nullptr) {
        Virtual<uint8_t>(self->data, 6, stream);
    }
    return 1;
}

// 0x807C50: SaveBinary
void __fastcall Quat_SaveBinary(NiQuaternionInterpolator* self, re::Edx, void* stream) {
    Interpolator_SaveBinary(self, stream);
    Quaternion_SaveBinary(&self->value, stream);
    Virtual<void>(stream, 12, self->data);   // SaveLinkID
}

// 0x807C80: IsEqual
uint8_t __fastcall Quat_IsEqual(NiQuaternionInterpolator* self, re::Edx, NiQuaternionInterpolator* other) {
    if (!Interpolator_IsEqual(self, other)) {
        return 0;
    }
    if (Quaternion_NotEqual(&self->value, &other->value)) {
        return 0;
    }
    return DataEqual(self->data, other->data);
}

// 0x807CE0: GetActiveTimeRange(&begin, &end): the first and last key times (an Euler key's three
// sequences, empty ones read all the same, as the game has it); no keys: 0 and 0
void __fastcall Quat_GetActiveTimeRange(NiQuaternionInterpolator* self, re::Edx, float* begin, float* end) {
    NiRotData* data = self->data;
    if (data != nullptr && data->rot.numKeys != 0) {
        const NiAnimationKey* keys = data->rot.keys;
        if (data->rot.type != EULERKEY) {
            *begin = keys->time;
            *end = KeyAt(keys, data->rot.numKeys - 1, data->rot.size)->time;
            return;
        }
        const NiEulerRotKey* euler = reinterpret_cast<const NiEulerRotKey*>(keys);
        float lo = 3.40282347e+38f;
        float hi = kInvalidFloat;
        for (int i = 0; i < 3; i++) {
            const NiFloatKey* seq = euler->keys[i];
            float first = seq->time;
            if (first < lo) {
                lo = first;
            }
            float last = KeyAt(seq, euler->numKeys[i] - 1, euler->size[i])->time;
            if (hi < last) {
                hi = last;
            }
        }
        *begin = lo;
        *end = hi;
        return;
    }
    *begin = 0.0f;
    *end = 0.0f;
}

// 0x807DE0: GetSequenceInterpolator(start, end)
NiInterpolator* __fastcall Quat_GetSequenceInterpolator(NiQuaternionInterpolator* self, re::Edx, float start,
                                                        float end) {
    NiQuaternionInterpolator* clone =
        static_cast<NiQuaternionInterpolator*>(Interpolator_GetSequenceInterpolator(self, start, end));
    if (self->data != nullptr) {
        re::Ptr32<NiRotData> data = nullptr;
        RotData_GetSequenceData(self->data, &data, start, end);
        AssignReleasingFirst(clone->data, data);
        clone->lastIdx = 0;
        ReleaseRef(data);
    }
    return clone;
}

// 0x807EA0: Update(time, target, quaternion&) (slot 21): false when the value is the invalid one
uint8_t __fastcall Quat_Update(NiQuaternionInterpolator* self, re::Edx, float time, NiObjectNET*,
                               NiQuaternion* value) {
    if (time == self->lastTime) {
        *value = self->value;
        return Quaternion_Equal(&self->value, &kInvalidQuaternion) == 0 ? 1 : 0;
    }
    NiRotData* data = self->data;
    if (data != nullptr && data->rot.numKeys != 0) {
        NiQuaternion q;
        const NiQuaternion* r = RotKey_GenInterp(&q, time, data->rot.keys, data->rot.type, data->rot.numKeys,
                                                 &self->lastIdx, data->rot.size);
        self->value = *r;
    }
    if (Quaternion_Equal(&self->value, &kInvalidQuaternion)) {
        return 0;
    }
    *value = self->value;
    self->lastTime = time;
    return 1;
}

// 0x807F90: CreateClone
NiQuaternionInterpolator* __fastcall Quat_CreateClone(NiQuaternionInterpolator* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x24);
    if (memory == nullptr) {
        QuaternionInterpolator_CopyMembers(self, nullptr, cloning);
        return nullptr;
    }
    NiQuaternionInterpolator* clone = static_cast<NiQuaternionInterpolator*>(memory);
    KeyBasedInterpolator_Construct(clone);
    QuatInitMembers(clone);
    QuaternionInterpolator_CopyMembers(self, clone, cloning);
    return clone;
}

// 0x808010: GetViewerStrings
void __fastcall Quat_GetViewerStrings(NiQuaternionInterpolator* self, re::Edx, NiTArray<char*>* strings) {
    KeyBasedInterpolator_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiQuaternionInterpolator_RTTI.name));
    AddViewerString(strings, Quaternion_GetViewerString(&self->value, "m_kQuaternionValue"));
    AddViewerString(strings, GetViewerStringPointer("m_spQuaternionData", self->data));
}

// ---- NiPoint3Interpolator

inline void PointInitMembers(NiPoint3Interpolator* self) {
    self->vtable = reinterpret_cast<void*>(NiPoint3Interpolator_vtable);
    self->value = kInvalidPoint;
    self->data = nullptr;
    self->lastIdx = 0;
}

// 0x808530: GetKeyContent
uint32_t __fastcall Point_GetKeyContent(NiPoint3Interpolator*, re::Edx, uint16_t) {
    return POSKEY;
}

// 0x808540: NiPoint3Interpolator(data)
NiPoint3Interpolator* __fastcall Point_ConstructWithData(NiPoint3Interpolator* self, re::Edx, NiPosData* data) {
    KeyBasedInterpolator_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiPoint3Interpolator_vtable);
    self->value = kInvalidPoint;
    self->data = data;
    AddRefImport(data);
    self->lastIdx = 0;
    return self;
}

const RTTI* __fastcall Point_GetRTTI(NiPoint3Interpolator*, re::Edx) {
    return &NiPoint3Interpolator_RTTI;
}

// 0x8085A0: NiPoint3Interpolator(pose) (the point by value)
NiPoint3Interpolator* __fastcall Point_ConstructWithPose(NiPoint3Interpolator* self, re::Edx, NiPoint3 pose) {
    KeyBasedInterpolator_Construct(self);
    self->value.x = pose.x;
    self->value.y = pose.y;
    self->vtable = reinterpret_cast<void*>(NiPoint3Interpolator_vtable);
    self->value.z = pose.z;
    self->data = nullptr;
    self->lastIdx = 0;
    return self;
}

// 0x8085E0: GetKeyCount
uint32_t __fastcall Point_GetKeyCount(NiPoint3Interpolator* self, re::Edx, uint16_t) {
    return self->data != nullptr ? self->data->pos.numKeys : 0;
}

// 0x8085F0: GetKeyType
uint32_t __fastcall Point_GetKeyType(NiPoint3Interpolator* self, re::Edx, uint16_t) {
    return self->data != nullptr ? self->data->pos.type : 0;
}

// 0x808600: GetKeyArray
NiAnimationKey* __fastcall Point_GetKeyArray(NiPoint3Interpolator* self, re::Edx, uint16_t) {
    return self->data != nullptr ? self->data->pos.keys : nullptr;
}

// 0x808610: GetKeyStride
uint8_t __fastcall Point_GetKeyStride(NiPoint3Interpolator* self, re::Edx, uint16_t) {
    return self->data != nullptr ? self->data->pos.size : 0;
}

// 0x808630: GetChannelPosed (no data and a valid value)
uint8_t __fastcall Point_GetChannelPosed(NiPoint3Interpolator* self, re::Edx, uint16_t) {
    if (self->data != nullptr) {
        return 0;
    }
    return PointIsInvalid(self->value) ? 0 : 1;
}

// 0x808680: SetPoseValue(point) (the data let go; the point by value)
void __fastcall Point_SetPoseValue(NiPoint3Interpolator* self, re::Edx, NiPoint3 pose) {
    if (self->data != nullptr) {
        ReleaseRef(self->data);
        self->data = nullptr;
    }
    self->value = pose;
}

// 0x8086D0: GuaranteeTimeRange(start, end)
void __fastcall Point_GuaranteeTimeRange(NiPoint3Interpolator* self, re::Edx, float start, float end) {
    if (self->data != nullptr) {
        PosData_GuaranteeKeysAtStartAndEnd(self->data, start, end);
    }
}

// 0x808700: CopyMembers
void __fastcall Point_CopyMembers(NiPoint3Interpolator* self, re::Edx, NiPoint3Interpolator* dest, void* cloning) {
    KeyBasedInterpolator_CopyMembers(self, dest, cloning);
    dest->value = self->value;
    AssignReleasingFirst(dest->data, self->data);
    dest->lastIdx = self->lastIdx;
}

// 0x808790: CreateObject
NiObject* __cdecl Point_CreateObject() {
    void* memory = NiNew(0x20);
    if (memory == nullptr) {
        return nullptr;
    }
    NiPoint3Interpolator* self = static_cast<NiPoint3Interpolator*>(memory);
    KeyBasedInterpolator_Construct(self);
    PointInitMembers(self);
    return self;
}

// 0x8087E0: LoadBinary
void __fastcall Point_LoadBinary(NiPoint3Interpolator* self, re::Edx, void* stream) {
    Interpolator_LoadBinary(self, stream);
    Point3_LoadBinary(&self->value, stream);
    NiObject* data = Stream_ReadLinkIDObject(stream);
    AssignReleasingFirst(self->data, data);
}

// 0x808840: RegisterStreamables
uint8_t __fastcall Point_RegisterStreamables(NiPoint3Interpolator* self, re::Edx, void* stream) {
    if (!Interpolator_RegisterStreamables(self, stream)) {
        return 0;
    }
    if (self->data != nullptr) {
        Virtual<uint8_t>(self->data, 6, stream);
    }
    return 1;
}

// 0x808870: SaveBinary
void __fastcall Point_SaveBinary(NiPoint3Interpolator* self, re::Edx, void* stream) {
    Interpolator_SaveBinary(self, stream);
    Point3_SaveBinary(&self->value, stream);
    Virtual<void>(stream, 12, self->data);   // SaveLinkID
}

// 0x8088A0: the deleting destructor
NiPoint3Interpolator* __fastcall Point_DeletingDestructor(NiPoint3Interpolator* self, re::Edx, uint32_t flags) {
    ReleaseRef(self->data);
    KeyBasedInterpolator_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x20);
    }
    return self;
}

// 0x8088F0: Collapse (slot 30): no keys: the invalid point; one key, or linear / step keys all the
// same: that point posed. Either way the data let go
void __fastcall Point_Collapse(NiPoint3Interpolator* self, re::Edx) {
    NiPosData* data = self->data;
    if (data == nullptr) {
        return;
    }
    uint8_t size = data->pos.size;
    KeyType type = data->pos.type;
    uint32_t numKeys = data->pos.numKeys;
    const NiAnimationKey* keys = data->pos.keys;
    if (numKeys == 0) {
        ReleaseRef(data);
        self->data = nullptr;
        self->value = kInvalidPoint;
        return;
    }
    NiPoint3 p = reinterpret_cast<const NiPosKey*>(keys)->pos;
    if (numKeys != 1) {
        if (type != LINKEY && type != STEPKEY) {
            return;
        }
        uint8_t same = 1;
        for (uint32_t i = 1; i < numKeys; i++) {
            if (Point3_NotEqual(&KeyAt<NiPosKey>(keys, i, size)->pos, &p)) {
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
    self->value = p;
}

// 0x808A10: GetSequenceInterpolator(start, end)
NiInterpolator* __fastcall Point_GetSequenceInterpolator(NiPoint3Interpolator* self, re::Edx, float start, float end) {
    NiPoint3Interpolator* clone =
        static_cast<NiPoint3Interpolator*>(Interpolator_GetSequenceInterpolator(self, start, end));
    if (self->data != nullptr) {
        re::Ptr32<NiPosData> data = nullptr;
        PosData_GetSequenceData(self->data, &data, start, end);
        AssignReleasingFirst(clone->data, data);
        clone->lastIdx = 0;
        ReleaseRef(data);
    }
    return clone;
}

// 0x808AD0: Update(time, target, point&) (slot 20): false when the value is the invalid point (the
// cached value is written out first, a fresh one only when valid)
uint8_t __fastcall Point_Update(NiPoint3Interpolator* self, re::Edx, float time, NiObjectNET*, NiPoint3* value) {
    if (time == self->lastTime) {
        *value = self->value;
        return PointIsInvalid(self->value) ? 0 : 1;
    }
    NiPosData* data = self->data;
    if (data != nullptr && data->pos.numKeys != 0) {
        NiPoint3 p;
        const NiPoint3* r = PosKey_GenInterp(&p, time, data->pos.keys, data->pos.type, data->pos.numKeys,
                                             &self->lastIdx, data->pos.size);
        self->value = *r;
    }
    if (PointIsInvalid(self->value)) {
        return 0;
    }
    *value = self->value;
    self->lastTime = time;
    return 1;
}

// 0x808C00: CreateClone
NiPoint3Interpolator* __fastcall Point_CreateClone(NiPoint3Interpolator* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x20);
    if (memory == nullptr) {
        Point3Interpolator_CopyMembers(self, nullptr, cloning);
        return nullptr;
    }
    NiPoint3Interpolator* clone = static_cast<NiPoint3Interpolator*>(memory);
    KeyBasedInterpolator_Construct(clone);
    PointInitMembers(clone);
    Point3Interpolator_CopyMembers(self, clone, cloning);
    return clone;
}

// 0x808C80: IsEqual
uint8_t __fastcall Point_IsEqual(NiPoint3Interpolator* self, re::Edx, NiPoint3Interpolator* other) {
    if (!Interpolator_IsEqual(self, other)) {
        return 0;
    }
    if (Point3_NotEqual(&self->value, &other->value)) {
        return 0;
    }
    return DataEqual(self->data, other->data);
}

// 0x808CE0: GetViewerStrings
void __fastcall Point_GetViewerStrings(NiPoint3Interpolator* self, re::Edx, NiTArray<char*>* strings) {
    KeyBasedInterpolator_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiPoint3Interpolator_RTTI.name));
    AddViewerString(strings, Point3_GetViewerString(&self->value, "m_kPoint3Value"));
    AddViewerString(strings, GetViewerStringPointer("m_spPoint3Data", self->data));
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC30_KEYINTERP_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIC30_KEYINTERP_REVERSED("NiQuaternionInterpolator::GetKeyContent", 0x00807950, &ni::anim::Quat_GetKeyContent);
NIC30_KEYINTERP_REVERSED("NiQuaternionInterpolator::GetRTTI", 0x00807960, &ni::anim::Quat_GetRTTI);
NIC30_KEYINTERP_REVERSED("NiQuaternionInterpolator::GetKeyCount", 0x00807970, &ni::anim::Quat_GetKeyCount);
NIC30_KEYINTERP_REVERSED("NiQuaternionInterpolator::GetKeyType", 0x00807980, &ni::anim::Quat_GetKeyType);
NIC30_KEYINTERP_REVERSED("NiQuaternionInterpolator::GetKeyArray", 0x00807990, &ni::anim::Quat_GetKeyArray);
NIC30_KEYINTERP_REVERSED("NiQuaternionInterpolator::GetChannelPosed", 0x008079A0, &ni::anim::Quat_GetChannelPosed);
NIC30_KEYINTERP_REVERSED("NiQuaternionInterpolator::Collapse", 0x008079C0, &ni::anim::Quat_Collapse);
NIC30_KEYINTERP_REVERSED("NiQuaternionInterpolator::GuaranteeTimeRange", 0x00807B00,
                         &ni::anim::Quat_GuaranteeTimeRange);
NIC30_KEYINTERP_REVERSED("NiQuaternionInterpolator::CopyMembers", 0x00807B30, &ni::anim::Quat_CopyMembers);
NIC30_KEYINTERP_REVERSED("NiQuaternionInterpolator::CreateObject", 0x00807BC0, &ni::anim::Quat_CreateObject);
NIC30_KEYINTERP_REVERSED("NiQuaternionInterpolator::RegisterStreamables", 0x00807C20,
                         &ni::anim::Quat_RegisterStreamables);
NIC30_KEYINTERP_REVERSED("NiQuaternionInterpolator::SaveBinary", 0x00807C50, &ni::anim::Quat_SaveBinary);
NIC30_KEYINTERP_REVERSED("NiQuaternionInterpolator::IsEqual", 0x00807C80, &ni::anim::Quat_IsEqual);
NIC30_KEYINTERP_REVERSED("NiQuaternionInterpolator::GetActiveTimeRange", 0x00807CE0,
                         &ni::anim::Quat_GetActiveTimeRange);
NIC30_KEYINTERP_REVERSED("NiQuaternionInterpolator::GetSequenceInterpolator", 0x00807DE0,
                         &ni::anim::Quat_GetSequenceInterpolator);
NIC30_KEYINTERP_REVERSED("NiQuaternionInterpolator::Update", 0x00807EA0, &ni::anim::Quat_Update);
NIC30_KEYINTERP_REVERSED("NiQuaternionInterpolator::CreateClone", 0x00807F90, &ni::anim::Quat_CreateClone);
NIC30_KEYINTERP_REVERSED("NiQuaternionInterpolator::GetViewerStrings", 0x00808010, &ni::anim::Quat_GetViewerStrings);
NIC30_KEYINTERP_REVERSED("NiPoint3Interpolator::GetKeyContent", 0x00808530, &ni::anim::Point_GetKeyContent);
NIC30_KEYINTERP_REVERSED("NiPoint3Interpolator::NiPoint3Interpolator(data)", 0x00808540,
                         &ni::anim::Point_ConstructWithData);
NIC30_KEYINTERP_REVERSED("NiPoint3Interpolator::GetRTTI", 0x00808590, &ni::anim::Point_GetRTTI);
NIC30_KEYINTERP_REVERSED("NiPoint3Interpolator::NiPoint3Interpolator(pose)", 0x008085A0,
                         &ni::anim::Point_ConstructWithPose);
NIC30_KEYINTERP_REVERSED("NiPoint3Interpolator::GetKeyCount", 0x008085E0, &ni::anim::Point_GetKeyCount);
NIC30_KEYINTERP_REVERSED("NiPoint3Interpolator::GetKeyType", 0x008085F0, &ni::anim::Point_GetKeyType);
NIC30_KEYINTERP_REVERSED("NiPoint3Interpolator::GetKeyArray", 0x00808600, &ni::anim::Point_GetKeyArray);
NIC30_KEYINTERP_REVERSED("NiPoint3Interpolator::GetKeyStride", 0x00808610, &ni::anim::Point_GetKeyStride);
NIC30_KEYINTERP_REVERSED("NiPoint3Interpolator::GetChannelPosed", 0x00808630, &ni::anim::Point_GetChannelPosed);
NIC30_KEYINTERP_REVERSED("NiPoint3Interpolator::SetPoseValue", 0x00808680, &ni::anim::Point_SetPoseValue);
NIC30_KEYINTERP_REVERSED("NiPoint3Interpolator::GuaranteeTimeRange", 0x008086D0,
                         &ni::anim::Point_GuaranteeTimeRange);
NIC30_KEYINTERP_REVERSED("NiPoint3Interpolator::CopyMembers", 0x00808700, &ni::anim::Point_CopyMembers);
NIC30_KEYINTERP_REVERSED("NiPoint3Interpolator::CreateObject", 0x00808790, &ni::anim::Point_CreateObject);
NIC30_KEYINTERP_REVERSED("NiPoint3Interpolator::LoadBinary", 0x008087E0, &ni::anim::Point_LoadBinary);
NIC30_KEYINTERP_REVERSED("NiPoint3Interpolator::RegisterStreamables", 0x00808840,
                         &ni::anim::Point_RegisterStreamables);
NIC30_KEYINTERP_REVERSED("NiPoint3Interpolator::SaveBinary", 0x00808870, &ni::anim::Point_SaveBinary);
NIC30_KEYINTERP_REVERSED("NiPoint3Interpolator::DeletingDestructor", 0x008088A0,
                         &ni::anim::Point_DeletingDestructor);
NIC30_KEYINTERP_REVERSED("NiPoint3Interpolator::Collapse", 0x008088F0, &ni::anim::Point_Collapse);
NIC30_KEYINTERP_REVERSED("NiPoint3Interpolator::GetSequenceInterpolator", 0x00808A10,
                         &ni::anim::Point_GetSequenceInterpolator);
NIC30_KEYINTERP_REVERSED("NiPoint3Interpolator::Update", 0x00808AD0, &ni::anim::Point_Update);
NIC30_KEYINTERP_REVERSED("NiPoint3Interpolator::CreateClone", 0x00808C00, &ni::anim::Point_CreateClone);
NIC30_KEYINTERP_REVERSED("NiPoint3Interpolator::IsEqual", 0x00808C80, &ni::anim::Point_IsEqual);
NIC30_KEYINTERP_REVERSED("NiPoint3Interpolator::GetViewerStrings", 0x00808CE0, &ni::anim::Point_GetViewerStrings);
