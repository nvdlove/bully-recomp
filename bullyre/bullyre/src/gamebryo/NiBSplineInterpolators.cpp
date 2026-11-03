// The B-spline interpolators of this chunk (src/gamebryo/NiBSplineInterpolators.h):
// NiBSplineTransformInterpolator, NiBSplinePoint3Interpolator, NiBSplineFloatInterpolator and their
// compressed kinds, and the compressed colour one's slots of this range (the colour ones' others:
// src/gamebryo/NiBSplineData.cpp). Their channel queries, Update (the curve at the time's fraction
// of the range), construction, cloning, streaming, comparison and viewer strings.
//
// Main-thread float maths at the game's 24-bit x87 precision.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiBSplineData.h"
#include "gamebryo/NiBSplineInterpolators.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiInterpolators.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni::anim {

namespace {

using scene::AddViewerString;
using scene::BinaryStream;
using scene::GetViewerString;
using scene::GetViewerStringFloat;
using scene::GetViewerStringUint;
using scene::LoadBytes;
using scene::SaveBytes;
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiBSplineTransformInterpolator_RTTI, 0x020E51A0);
GAME_VAR(RTTI, NiBSplinePoint3Interpolator_RTTI, 0x020E51D4);
GAME_VAR(RTTI, NiBSplineFloatInterpolator_RTTI, 0x020E5208);
GAME_VAR(RTTI, NiBSplineCompTransformInterpolator_RTTI, 0x020E523C);
GAME_VAR(RTTI, NiBSplineCompPoint3Interpolator_RTTI, 0x020E5270);
GAME_VAR(RTTI, NiBSplineCompFloatInterpolator_RTTI, 0x020E52A4);
GAME_VAR(RTTI, NiBSplineCompColorInterpolator_RTTI, 0x020E52D8);

constexpr uintptr_t NiBSplineTransformInterpolator_vtable = 0x00966FC8;
constexpr uintptr_t NiBSplinePoint3Interpolator_vtable = 0x009670B8;
constexpr uintptr_t NiBSplineFloatInterpolator_vtable = 0x00967188;
constexpr uintptr_t NiBSplineCompTransformInterpolator_vtable = 0x00967258;
constexpr uintptr_t NiBSplineCompPoint3Interpolator_vtable = 0x00967318;
constexpr uintptr_t NiBSplineCompFloatInterpolator_vtable = 0x009673D8;
constexpr uintptr_t NiBSplineCompColorInterpolator_vtable = 0x00967498;
constexpr float kFloatMax = 3.40282347e+38f;

GAME_FN(NiQuaternion*, __thiscall, Quaternion_Construct, 0x00775B90, NiQuaternion* self, float w, float x, float y,
        float z);
GAME_FN(void, __thiscall, Quaternion_FastNormalize, 0x007764D0, NiQuaternion* self);
GAME_FN(uint8_t, __thiscall, QuatTransform_Equal, 0x00805210, const NiQuatTransform* self, const NiQuatTransform* other);
GAME_FN(void, __thiscall, QuatTransform_LoadBinary, 0x00825B50, NiQuatTransform* self, void* stream);
GAME_FN(void, __thiscall, QuatTransform_SaveBinary, 0x00825C50, NiQuatTransform* self, void* stream);
GAME_FN(void, __thiscall, QuatTransform_GetViewerStrings, 0x00825CA0, NiQuatTransform* self, NiTArray<char*>* strings);
GAME_FN(uint8_t, __thiscall, Point3_NotEqual, 0x0076BEB0, const NiPoint3* self, const NiPoint3* other);
GAME_FN(void, __thiscall, Point3_LoadBinary, 0x007768A0, NiPoint3* self, void* stream);
GAME_FN(void, __thiscall, Point3_SaveBinary, 0x00762550, const NiPoint3* self, void* stream);
// NiBSplineInterpolator's (the next chunks')
GAME_FN(void, __thiscall, BSplineInterpolator_Construct, 0x00828560, void* self, NiBSplineData* data,
        NiBSplineBasisData* basisData);
GAME_FN(void, __thiscall, BSplineInterpolator_Destruct, 0x008285C0, void* self);
GAME_FN(uint8_t, __thiscall, BSplineInterpolator_IsEqual, 0x008284D0, void* self, void* other);
GAME_FN(void, __thiscall, BSplineInterpolator_CopyMembers, 0x00828610, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, BSplineInterpolator_LoadBinary, 0x008286C0, void* self, void* stream);
GAME_FN(void, __thiscall, BSplineInterpolator_SaveBinary, 0x008287A0, void* self, void* stream);
GAME_FN(void, __thiscall, BSplineInterpolator_GetViewerStrings, 0x00828840, void* self, NiTArray<char*>* strings);
// the colour ones' and the data's (src/gamebryo/NiBSplineData.cpp)
GAME_FN(NiBSplineColorInterpolator*, __thiscall, BSplineColor_Construct, 0x0081BA00, void* self, NiBSplineData* data,
        uint32_t handle, NiBSplineBasisData* basisData);
GAME_FN(void, __thiscall, BSplineColor_CopyMembers, 0x0081B990, void* self, void* dest, void* cloning);
GAME_FN(uint8_t, __thiscall, BSplineColor_RegisterStreamables, 0x0081B9D0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, BSplineColor_IsEqual, 0x0081BC70, void* self, void* other);
GAME_FN(void, __thiscall, BSplineData_GetValueDegree3, 0x0081C490, NiBSplineData* self, float t, float* pos,
        int32_t dimension, NiBSplineBasisData* basisData, uint32_t handle);
GAME_FN(void, __thiscall, BSplineData_GetCompactedValueDegree3, 0x0081C600, NiBSplineData* self, float t, float* pos,
        int32_t dimension, NiBSplineBasisData* basisData, uint32_t handle, float offset, float halfRange);
// (this file's, called where modes apply)
GAME_FN(NiBSplineTransformInterpolator*, __thiscall, BSplineTransform_Construct, 0x00819EF0, void* self,
        NiBSplineData* data, uint32_t transHandle, uint32_t rotHandle, uint32_t scaleHandle,
        NiBSplineBasisData* basisData);
GAME_FN(void, __thiscall, BSplineTransform_CopyMembers, 0x00819EA0, void* self, void* dest, void* cloning);
GAME_FN(uint8_t, __thiscall, BSplineTransform_IsEqual, 0x0081A0B0, void* self, void* other);
GAME_FN(void, __thiscall, BSplineTransform_LoadBinary, 0x0081A2E0, void* self, void* stream);
GAME_FN(void, __thiscall, BSplineTransform_SaveBinary, 0x0081A370, void* self, void* stream);
GAME_FN(void, __thiscall, BSplineTransform_GetViewerStrings, 0x0081A400, void* self, NiTArray<char*>* strings);
GAME_FN(NiBSplinePoint3Interpolator*, __thiscall, BSplinePoint3_Construct, 0x0081A5B0, void* self, NiBSplineData* data,
        uint32_t handle, NiBSplineBasisData* basisData);
GAME_FN(void, __thiscall, BSplinePoint3_CopyMembers, 0x0081A570, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, BSplinePoint3_LoadBinary, 0x0081A770, void* self, void* stream);
GAME_FN(void, __thiscall, BSplinePoint3_SaveBinary, 0x0081A7C0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, BSplinePoint3_IsEqual, 0x0081A810, void* self, void* other);
GAME_FN(void, __thiscall, BSplinePoint3_GetViewerStrings, 0x0081A850, void* self, NiTArray<char*>* strings);
GAME_FN(NiBSplineFloatInterpolator*, __thiscall, BSplineFloat_Construct, 0x0081A8E0, void* self, NiBSplineData* data,
        uint32_t handle, NiBSplineBasisData* basisData);
GAME_FN(void, __thiscall, BSplineFloat_CopyMembers, 0x0081A990, void* self, void* dest, void* cloning);
GAME_FN(uint8_t, __thiscall, BSplineFloat_IsEqual, 0x0081AA00, void* self, void* other);
GAME_FN(void, __thiscall, BSplineFloat_LoadBinary, 0x0081AB40, void* self, void* stream);
GAME_FN(void, __thiscall, BSplineFloat_SaveBinary, 0x0081ABA0, void* self, void* stream);
GAME_FN(void, __thiscall, BSplineFloat_GetViewerStrings, 0x0081AC00, void* self, NiTArray<char*>* strings);

// NiStream's binary reads and writes of a block of 4-byte components
inline void LoadFloats(void* stream, void* buffer, uint32_t bytes) {
    BinaryStream* in = scene::At<BinaryStream*>(stream, 0x188);
    uint32_t componentSize = 4;
    in->position += in->read(in, buffer, bytes, &componentSize, 1);
}

inline void SaveFloats(void* stream, const void* buffer, uint32_t bytes) {
    BinaryStream* out = scene::At<BinaryStream*>(stream, 0x18C);
    uint32_t componentSize = 4;
    out->position += out->write(out, buffer, bytes, &componentSize, 1);
}

// the fraction of the key range the time is at
inline float Fraction(const NiBSplineInterpolator* self, float time) {
    return (time - self->startTime) / (self->endTime - self->startTime);
}

inline bool TransformValid(const NiQuatTransform& value) {
    return !(value.scale == kInvalidFloat) || !(value.rotate.x == kInvalidFloat) ||
           !(value.translate.x == kInvalidFloat);
}

// the deleting destructor every kind has (NiBSplineInterpolator's destructor, then the size freed)
inline void* DeleteBSpline(void* self, uint32_t flags, uint32_t size) {
    BSplineInterpolator_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, size);
    }
    return self;
}

// a compressed kind's floats compared one by one (unequal, or a NaN: false)
inline bool ScalarsEqual(const float* mine, const float* theirs, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        if (!(theirs[i] == mine[i])) {
            return false;
        }
    }
    return true;
}

// the transform's three channels evaluated with GetValue (compressed: their offsets and ranges)
template <bool Compressed>
inline uint8_t TransformUpdate(NiBSplineTransformInterpolator* self, float time, NiQuatTransform* value) {
    if (time == self->lastTime) {
        *value = self->value;
        return TransformValid(self->value) ? 1 : 0;
    }
    float t = Fraction(self, time);
    const float* scalars = nullptr;
    if constexpr (Compressed) {
        scalars = static_cast<NiBSplineCompTransformInterpolator*>(self)->compScalars;
    }
    float result[4];
    if (self->transHandle != kInvalidBSplineHandle) {
        if constexpr (Compressed) {
            BSplineData_GetCompactedValueDegree3(self->data, t, result, 3, self->basisData, self->transHandle,
                                                 scalars[0], scalars[1]);
        } else {
            BSplineData_GetValueDegree3(self->data, t, result, 3, self->basisData, self->transHandle);
        }
        self->value.translate.x = result[0];
        self->value.translate.y = result[1];
        self->value.translate.z = result[2];
    }
    if (self->rotHandle != kInvalidBSplineHandle) {
        if constexpr (Compressed) {
            BSplineData_GetCompactedValueDegree3(self->data, t, result, 4, self->basisData, self->rotHandle,
                                                 scalars[2], scalars[3]);
        } else {
            BSplineData_GetValueDegree3(self->data, t, result, 4, self->basisData, self->rotHandle);
        }
        NiQuaternion q;
        Quaternion_Construct(&q, result[0], result[1], result[2], result[3]);
        Quaternion_FastNormalize(&q);
        self->value.rotate = q;
    }
    if (self->scaleHandle != kInvalidBSplineHandle) {
        if constexpr (Compressed) {
            BSplineData_GetCompactedValueDegree3(self->data, t, result, 1, self->basisData, self->scaleHandle,
                                                 scalars[4], scalars[5]);
        } else {
            BSplineData_GetValueDegree3(self->data, t, result, 1, self->basisData, self->scaleHandle);
        }
        self->value.scale = result[0];
    }
    *value = self->value;
    if (!TransformValid(self->value)) {
        return 0;
    }
    self->lastTime = time;
    return 1;
}

// ---- NiBSplineTransformInterpolator

// 0x819DC0: GetDimension(channel) (slot 39): 3, 4, 1
uint32_t __fastcall Xf_GetDimension(NiBSplineTransformInterpolator*, re::Edx, uint16_t channel) {
    if (channel == 0) {
        return 3;
    }
    if (channel == 1) {
        return 4;
    }
    if (channel == 2) {
        return 1;
    }
    return 0;
}

// 0x819E00: GetDegree(channel) (slot 40): cubic
uint32_t __fastcall Xf_GetDegree(NiBSplineTransformInterpolator*, re::Edx, uint16_t channel) {
    if (channel != 0 && channel != 1 && channel != 2) {
        return 0;
    }
    return 3;
}

// 0x819E30: GetControlHandle(channel) (slot 44)
uint32_t __fastcall Xf_GetControlHandle(NiBSplineTransformInterpolator* self, re::Edx, uint16_t channel) {
    if (channel == 0) {
        return self->transHandle;
    }
    if (channel == 1) {
        return self->rotHandle;
    }
    if (channel == 2) {
        return self->scaleHandle;
    }
    return kInvalidBSplineHandle;
}

// 0x819E60: SetControlHandle(handle, channel) (slot 45)
void __fastcall Xf_SetControlHandle(NiBSplineTransformInterpolator* self, re::Edx, uint32_t handle, uint16_t channel) {
    if (channel == 0) {
        self->transHandle = handle;
    } else if (channel == 1) {
        self->rotHandle = handle;
    } else if (channel == 2) {
        self->scaleHandle = handle;
    }
}

// 0x819EA0: CopyMembers
void __fastcall Xf_CopyMembers(NiBSplineTransformInterpolator* self, re::Edx, NiBSplineTransformInterpolator* dest,
                               void* cloning) {
    BSplineInterpolator_CopyMembers(self, dest, cloning);
    dest->value = self->value;
    dest->transHandle = self->transHandle;
    dest->rotHandle = self->rotHandle;
    dest->scaleHandle = self->scaleHandle;
}

const RTTI* __fastcall Xf_GetRTTI(NiBSplineTransformInterpolator*, re::Edx) {
    return &NiBSplineTransformInterpolator_RTTI;
}

// 0x819EF0: NiBSplineTransformInterpolator(data, transHandle, rotHandle, scaleHandle, basisData)
// (the invalid transform)
NiBSplineTransformInterpolator* __fastcall Xf_Construct(NiBSplineTransformInterpolator* self, re::Edx,
                                                        NiBSplineData* data, uint32_t transHandle, uint32_t rotHandle,
                                                        uint32_t scaleHandle, NiBSplineBasisData* basisData) {
    BSplineInterpolator_Construct(self, data, basisData);
    self->vtable = reinterpret_cast<void*>(NiBSplineTransformInterpolator_vtable);
    self->value.translate = kInvalidTranslate;
    self->value.rotate = kInvalidRotate;
    self->value.scale = kInvalidFloat;
    self->transHandle = transHandle;
    self->rotHandle = rotHandle;
    self->scaleHandle = scaleHandle;
    return self;
}

// 0x819F70: the deleting destructor
void* __fastcall Xf_DeletingDestructor(NiBSplineTransformInterpolator* self, re::Edx, uint32_t flags) {
    return DeleteBSpline(self, flags, 0x48);
}

// 0x819F90: GetChannelPosed(channel) (slot 41): no control points and a valid value
uint8_t __fastcall Xf_GetChannelPosed(NiBSplineTransformInterpolator* self, re::Edx, uint32_t channel) {
    float value;
    switch (uint16_t(channel)) {
    case 0:
        if (Virtual<uint32_t>(self, 38, channel) != 0) {
            return 0;
        }
        value = self->value.translate.x;
        break;
    case 1:
        if (Virtual<uint32_t>(self, 38, channel) != 0) {
            return 0;
        }
        value = self->value.rotate.x;
        break;
    case 2:
        if (Virtual<uint32_t>(self, 38, channel) != 0) {
            return 0;
        }
        value = self->value.scale;
        break;
    default:
        return 0;
    }
    return value == kInvalidFloat ? 0 : 1;
}

// 0x81A010: CreateClone (a failed allocation still copied onto null, as the game has it)
NiBSplineTransformInterpolator* __fastcall Xf_CreateClone(NiBSplineTransformInterpolator* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x48);
    NiBSplineTransformInterpolator* clone = nullptr;
    if (memory != nullptr) {
        clone = BSplineTransform_Construct(memory, nullptr, kInvalidBSplineHandle, kInvalidBSplineHandle,
                                           kInvalidBSplineHandle, nullptr);
    }
    BSplineInterpolator_CopyMembers(self, clone, cloning);
    clone->value = self->value;
    clone->transHandle = self->transHandle;
    clone->rotHandle = self->rotHandle;
    clone->scaleHandle = self->scaleHandle;
    return clone;
}

// 0x81A080: CreateObject
NiObject* __cdecl Xf_CreateObject() {
    void* memory = NiNew(0x48);
    if (memory == nullptr) {
        return nullptr;
    }
    return BSplineTransform_Construct(memory, nullptr, kInvalidBSplineHandle, kInvalidBSplineHandle,
                                      kInvalidBSplineHandle, nullptr);
}

// 0x81A0B0: IsEqual
uint8_t __fastcall Xf_IsEqual(NiBSplineTransformInterpolator* self, re::Edx, NiBSplineTransformInterpolator* other) {
    if (!BSplineInterpolator_IsEqual(self, other) || !QuatTransform_Equal(&self->value, &other->value) ||
        other->transHandle != self->transHandle || other->rotHandle != self->rotHandle ||
        other->scaleHandle != self->scaleHandle) {
        return 0;
    }
    return 1;
}

// 0x81A100: Update(time, target, transform&) (slot 18): the channels that have control points
// evaluated (the rotation normalised); false when the transform is all invalid
uint8_t __fastcall Xf_Update(NiBSplineTransformInterpolator* self, re::Edx, float time, NiObjectNET*,
                             NiQuatTransform* value) {
    return TransformUpdate<false>(self, time, value);
}

// 0x81A2E0: LoadBinary (the value, the handles)
void __fastcall Xf_LoadBinary(NiBSplineTransformInterpolator* self, re::Edx, void* stream) {
    BSplineInterpolator_LoadBinary(self, stream);
    QuatTransform_LoadBinary(&self->value, stream);
    LoadBytes(stream, &self->transHandle, 4);
    LoadBytes(stream, &self->rotHandle, 4);
    LoadBytes(stream, &self->scaleHandle, 4);
}

// 0x81A370: SaveBinary
void __fastcall Xf_SaveBinary(NiBSplineTransformInterpolator* self, re::Edx, void* stream) {
    BSplineInterpolator_SaveBinary(self, stream);
    QuatTransform_SaveBinary(&self->value, stream);
    SaveBytes(stream, &self->transHandle, 4);
    SaveBytes(stream, &self->rotHandle, 4);
    SaveBytes(stream, &self->scaleHandle, 4);
}

// 0x81A400: GetViewerStrings
void __fastcall Xf_GetViewerStrings(NiBSplineTransformInterpolator* self, re::Edx, NiTArray<char*>* strings) {
    BSplineInterpolator_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBSplineTransformInterpolator_RTTI.name));
    QuatTransform_GetViewerStrings(&self->value, strings);
    AddViewerString(strings, GetViewerStringUint("m_kTransCPHandle", self->transHandle));
    AddViewerString(strings, GetViewerStringUint("m_kRotCPHandle", self->rotHandle));
    AddViewerString(strings, GetViewerStringUint("m_kScaleCPHandle", self->scaleHandle));
}

// ---- NiBSplinePoint3Interpolator (and the one-channel slots several share)

// 0x81A510: GetDimension / GetDegree(channel) (the point ones' 39 and 40, the float and colour
// ones' 40): 3 for the channel
uint32_t __fastcall Pt_GetDimension(NiBSplineInterpolator*, re::Edx, uint16_t channel) {
    return channel != 0 ? 0 : 3;
}

// 0x81A530: GetControlHandle(channel) (slot 44)
uint32_t __fastcall Pt_GetControlHandle(NiBSplinePoint3Interpolator* self, re::Edx, uint16_t channel) {
    return channel == 0 ? self->handle : kInvalidBSplineHandle;
}

// 0x81A550: SetControlHandle(handle, channel) (slot 45)
void __fastcall Pt_SetControlHandle(NiBSplinePoint3Interpolator* self, re::Edx, uint32_t handle, uint16_t channel) {
    if (channel == 0) {
        self->handle = handle;
    }
}

// 0x81A570: CopyMembers
void __fastcall Pt_CopyMembers(NiBSplinePoint3Interpolator* self, re::Edx, NiBSplinePoint3Interpolator* dest,
                               void* cloning) {
    BSplineInterpolator_CopyMembers(self, dest, cloning);
    dest->value = self->value;
    dest->handle = self->handle;
}

const RTTI* __fastcall Pt_GetRTTI(NiBSplinePoint3Interpolator*, re::Edx) {
    return &NiBSplinePoint3Interpolator_RTTI;
}

// 0x81A5B0: NiBSplinePoint3Interpolator(data, handle, basisData)
NiBSplinePoint3Interpolator* __fastcall Pt_Construct(NiBSplinePoint3Interpolator* self, re::Edx, NiBSplineData* data,
                                                     uint32_t handle, NiBSplineBasisData* basisData) {
    BSplineInterpolator_Construct(self, data, basisData);
    self->vtable = reinterpret_cast<void*>(NiBSplinePoint3Interpolator_vtable);
    self->handle = handle;
    return self;
}

// 0x81A5E0: the deleting destructor (NiBSplineCompFloatInterpolator's too: the same size)
void* __fastcall Pt_DeletingDestructor(NiBSplinePoint3Interpolator* self, re::Edx, uint32_t flags) {
    return DeleteBSpline(self, flags, 0x2C);
}

// 0x81A600: Update(time, target, point&) (slot 20): the curve when there are control points
uint8_t __fastcall Pt_Update(NiBSplinePoint3Interpolator* self, re::Edx, float time, NiObjectNET*, NiPoint3* value) {
    if (time == self->lastTime) {
        *value = self->value;
        return 1;
    }
    if (self->handle != kInvalidBSplineHandle) {
        float result[3];
        BSplineData_GetValueDegree3(self->data, Fraction(self, time), result, 3, self->basisData, self->handle);
        self->value.x = result[0];
        self->value.y = result[1];
        self->value.z = result[2];
    }
    *value = self->value;
    self->lastTime = time;
    return 1;
}

// the point / float constructor CreateObject and CreateClone inline (the value left as it was)
template <typename T>
inline T* ConstructInline(void* memory, uintptr_t vtable) {
    BSplineInterpolator_Construct(memory, nullptr, nullptr);
    T* self = static_cast<T*>(memory);
    self->vtable = reinterpret_cast<void*>(vtable);
    self->handle = kInvalidBSplineHandle;
    return self;
}

// 0x81A6D0: CreateClone
NiBSplinePoint3Interpolator* __fastcall Pt_CreateClone(NiBSplinePoint3Interpolator* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x2C);
    NiBSplinePoint3Interpolator* clone = nullptr;
    if (memory != nullptr) {
        clone = ConstructInline<NiBSplinePoint3Interpolator>(memory, NiBSplinePoint3Interpolator_vtable);
    }
    BSplineInterpolator_CopyMembers(self, clone, cloning);
    clone->value = self->value;
    clone->handle = self->handle;
    return clone;
}

// 0x81A730: CreateObject
NiObject* __cdecl Pt_CreateObject() {
    void* memory = NiNew(0x2C);
    if (memory == nullptr) {
        return nullptr;
    }
    return ConstructInline<NiBSplinePoint3Interpolator>(memory, NiBSplinePoint3Interpolator_vtable);
}

// 0x81A770: LoadBinary
void __fastcall Pt_LoadBinary(NiBSplinePoint3Interpolator* self, re::Edx, void* stream) {
    BSplineInterpolator_LoadBinary(self, stream);
    Point3_LoadBinary(&self->value, stream);
    LoadBytes(stream, &self->handle, 4);
}

// 0x81A7C0: SaveBinary
void __fastcall Pt_SaveBinary(NiBSplinePoint3Interpolator* self, re::Edx, void* stream) {
    BSplineInterpolator_SaveBinary(self, stream);
    Point3_SaveBinary(&self->value, stream);
    SaveBytes(stream, &self->handle, 4);
}

// 0x81A810: IsEqual
uint8_t __fastcall Pt_IsEqual(NiBSplinePoint3Interpolator* self, re::Edx, NiBSplinePoint3Interpolator* other) {
    if (!BSplineInterpolator_IsEqual(self, other) || Point3_NotEqual(&self->value, &other->value)) {
        return 0;
    }
    return other->handle == self->handle ? 1 : 0;
}

// 0x81A850: GetViewerStrings
void __fastcall Pt_GetViewerStrings(NiBSplinePoint3Interpolator* self, re::Edx, NiTArray<char*>* strings) {
    BSplineInterpolator_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBSplinePoint3Interpolator_RTTI.name));
    AddViewerString(strings, GetViewerStringUint("m_kPoint3CPHandle", self->handle));
}

// ---- NiBSplineFloatInterpolator

// 0x81A8E0: NiBSplineFloatInterpolator(data, handle, basisData)
NiBSplineFloatInterpolator* __fastcall Fl_Construct(NiBSplineFloatInterpolator* self, re::Edx, NiBSplineData* data,
                                                    uint32_t handle, NiBSplineBasisData* basisData) {
    BSplineInterpolator_Construct(self, data, basisData);
    self->vtable = reinterpret_cast<void*>(NiBSplineFloatInterpolator_vtable);
    self->handle = handle;
    return self;
}

// 0x81A910: GetChannelPosed(channel) (slot 41; the point, float and colour ones'): no control
// points for the channel
uint8_t __fastcall One_GetChannelPosed(NiBSplineInterpolator* self, re::Edx, uint16_t channel) {
    if (channel != 0) {
        return 0;
    }
    return Virtual<uint32_t>(self, 38, 0u) == 0 ? 1 : 0;
}

// 0x81A940: GetDimension(channel) (slot 39): 1 for the channel
uint32_t __fastcall Fl_GetDimension(NiBSplineFloatInterpolator*, re::Edx, uint16_t channel) {
    return channel == 0 ? 1 : 0;
}

// 0x81A950: GetControlHandle(channel) (slot 44)
uint32_t __fastcall Fl_GetControlHandle(NiBSplineFloatInterpolator* self, re::Edx, uint16_t channel) {
    return channel == 0 ? self->handle : kInvalidBSplineHandle;
}

// 0x81A970: SetControlHandle(handle, channel) (slot 45)
void __fastcall Fl_SetControlHandle(NiBSplineFloatInterpolator* self, re::Edx, uint32_t handle, uint16_t channel) {
    if (channel == 0) {
        self->handle = handle;
    }
}

// 0x81A990: CopyMembers
void __fastcall Fl_CopyMembers(NiBSplineFloatInterpolator* self, re::Edx, NiBSplineFloatInterpolator* dest,
                               void* cloning) {
    BSplineInterpolator_CopyMembers(self, dest, cloning);
    dest->value = self->value;
    dest->handle = self->handle;
}

// 0x81A9C0: CreateObject
NiObject* __cdecl Fl_CreateObject() {
    void* memory = NiNew(0x24);
    if (memory == nullptr) {
        return nullptr;
    }
    return ConstructInline<NiBSplineFloatInterpolator>(memory, NiBSplineFloatInterpolator_vtable);
}

// 0x81AA00: IsEqual
uint8_t __fastcall Fl_IsEqual(NiBSplineFloatInterpolator* self, re::Edx, NiBSplineFloatInterpolator* other) {
    if (!BSplineInterpolator_IsEqual(self, other) || !(other->value == self->value)) {
        return 0;
    }
    return other->handle == self->handle ? 1 : 0;
}

const RTTI* __fastcall Fl_GetRTTI(NiBSplineFloatInterpolator*, re::Edx) {
    return &NiBSplineFloatInterpolator_RTTI;
}

// 0x81AA50: the deleting destructor
void* __fastcall Fl_DeletingDestructor(NiBSplineFloatInterpolator* self, re::Edx, uint32_t flags) {
    return DeleteBSpline(self, flags, 0x24);
}

// 0x81AA70: Update(time, target, float&) (slot 22)
uint8_t __fastcall Fl_Update(NiBSplineFloatInterpolator* self, re::Edx, float time, NiObjectNET*, float* value) {
    if (time == self->lastTime) {
        *value = self->value;
        return 1;
    }
    if (self->handle != kInvalidBSplineHandle) {
        BSplineData_GetValueDegree3(self->data, Fraction(self, time), &self->value, 1, self->basisData, self->handle);
    }
    *value = self->value;
    self->lastTime = time;
    return 1;
}

// 0x81AAF0: CreateClone
NiBSplineFloatInterpolator* __fastcall Fl_CreateClone(NiBSplineFloatInterpolator* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x24);
    NiBSplineFloatInterpolator* clone = nullptr;
    if (memory != nullptr) {
        clone = ConstructInline<NiBSplineFloatInterpolator>(memory, NiBSplineFloatInterpolator_vtable);
    }
    BSplineInterpolator_CopyMembers(self, clone, cloning);
    clone->value = self->value;
    clone->handle = self->handle;
    return clone;
}

// 0x81AB40: LoadBinary
void __fastcall Fl_LoadBinary(NiBSplineFloatInterpolator* self, re::Edx, void* stream) {
    BSplineInterpolator_LoadBinary(self, stream);
    LoadBytes(stream, &self->value, 4);
    LoadBytes(stream, &self->handle, 4);
}

// 0x81ABA0: SaveBinary
void __fastcall Fl_SaveBinary(NiBSplineFloatInterpolator* self, re::Edx, void* stream) {
    BSplineInterpolator_SaveBinary(self, stream);
    SaveBytes(stream, &self->value, 4);
    SaveBytes(stream, &self->handle, 4);
}

// 0x81AC00: GetViewerStrings
void __fastcall Fl_GetViewerStrings(NiBSplineFloatInterpolator* self, re::Edx, NiTArray<char*>* strings) {
    BSplineInterpolator_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBSplineFloatInterpolator_RTTI.name));
    AddViewerString(strings, GetViewerStringFloat("m_fFloatValue", self->value));
    AddViewerString(strings, GetViewerStringUint("m_kFloatCPHandle", self->handle));
}

// ---- NiBSplineCompTransformInterpolator

// 0x81ACE0: IsEqual (the six offsets and ranges too)
uint8_t __fastcall CXf_IsEqual(NiBSplineCompTransformInterpolator* self, re::Edx,
                               NiBSplineCompTransformInterpolator* other) {
    if (!BSplineTransform_IsEqual(self, other)) {
        return 0;
    }
    return ScalarsEqual(self->compScalars, other->compScalars, 6) ? 1 : 0;
}

const RTTI* __fastcall CXf_GetRTTI(NiBSplineCompTransformInterpolator*, re::Edx) {
    return &NiBSplineCompTransformInterpolator_RTTI;
}

// 0x81AD30: the deleting destructor
void* __fastcall CXf_DeletingDestructor(NiBSplineCompTransformInterpolator* self, re::Edx, uint32_t flags) {
    return DeleteBSpline(self, flags, 0x60);
}

inline NiBSplineCompTransformInterpolator* CXf_ConstructInline(void* memory) {
    auto* self = static_cast<NiBSplineCompTransformInterpolator*>(BSplineTransform_Construct(
        memory, nullptr, kInvalidBSplineHandle, kInvalidBSplineHandle, kInvalidBSplineHandle, nullptr));
    self->vtable = reinterpret_cast<void*>(NiBSplineCompTransformInterpolator_vtable);
    for (float& scalar : self->compScalars) {
        scalar = kFloatMax;
    }
    return self;
}

// 0x81AD50: CreateClone
NiBSplineCompTransformInterpolator* __fastcall CXf_CreateClone(NiBSplineCompTransformInterpolator* self, re::Edx,
                                                               void* cloning) {
    void* memory = NiNew(0x60);
    NiBSplineCompTransformInterpolator* clone = memory != nullptr ? CXf_ConstructInline(memory) : nullptr;
    BSplineTransform_CopyMembers(self, clone, cloning);
    for (int i = 0; i < 6; i++) {
        clone->compScalars[i] = self->compScalars[i];
    }
    return clone;
}

// 0x81ADE0: CreateObject
NiObject* __cdecl CXf_CreateObject() {
    void* memory = NiNew(0x60);
    if (memory == nullptr) {
        return nullptr;
    }
    return CXf_ConstructInline(memory);
}

// 0x81AE40: Update(time, target, transform&) (slot 18): from the compressed control points
uint8_t __fastcall CXf_Update(NiBSplineCompTransformInterpolator* self, re::Edx, float time, NiObjectNET*,
                              NiQuatTransform* value) {
    return TransformUpdate<true>(self, time, value);
}

// 0x81B050: LoadBinary (the six offsets and ranges after)
void __fastcall CXf_LoadBinary(NiBSplineCompTransformInterpolator* self, re::Edx, void* stream) {
    BSplineTransform_LoadBinary(self, stream);
    LoadFloats(stream, self->compScalars, 0x18);
}

// 0x81B090: SaveBinary
void __fastcall CXf_SaveBinary(NiBSplineCompTransformInterpolator* self, re::Edx, void* stream) {
    BSplineTransform_SaveBinary(self, stream);
    SaveFloats(stream, self->compScalars, 0x18);
}

// 0x81B0D0: GetViewerStrings
void __fastcall CXf_GetViewerStrings(NiBSplineCompTransformInterpolator* self, re::Edx, NiTArray<char*>* strings) {
    BSplineTransform_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBSplineCompTransformInterpolator_RTTI.name));
}

// 0x81B120: RegisterStreamables (the compressed kinds')
uint8_t __fastcall Comp_RegisterStreamables(NiBSplineInterpolator* self, re::Edx, void* stream) {
    return BSplineColor_RegisterStreamables(self, stream) != 0 ? 1 : 0;
}

// ---- NiBSplineCompPoint3Interpolator

// 0x81B140: IsEqual
uint8_t __fastcall CPt_IsEqual(NiBSplineCompPoint3Interpolator* self, re::Edx, NiBSplineCompPoint3Interpolator* other) {
    if (!BSplinePoint3_IsEqual(self, other)) {
        return 0;
    }
    return ScalarsEqual(self->compScalars, other->compScalars, 2) ? 1 : 0;
}

const RTTI* __fastcall CPt_GetRTTI(NiBSplineCompPoint3Interpolator*, re::Edx) {
    return &NiBSplineCompPoint3Interpolator_RTTI;
}

// 0x81B190: the deleting destructor (NiBSplineColorInterpolator's too: the same size)
void* __fastcall CPt_DeletingDestructor(NiBSplineCompPoint3Interpolator* self, re::Edx, uint32_t flags) {
    return DeleteBSpline(self, flags, 0x34);
}

// 0x81B1B0: Update(time, target, point&) (slot 20)
uint8_t __fastcall CPt_Update(NiBSplineCompPoint3Interpolator* self, re::Edx, float time, NiObjectNET*,
                              NiPoint3* value) {
    if (time == self->lastTime) {
        *value = self->value;
        return 1;
    }
    if (self->handle != kInvalidBSplineHandle) {
        float result[3];
        BSplineData_GetCompactedValueDegree3(self->data, Fraction(self, time), result, 3, self->basisData,
                                             self->handle, self->compScalars[0], self->compScalars[1]);
        self->value.x = result[0];
        self->value.y = result[1];
        self->value.z = result[2];
    }
    *value = self->value;
    self->lastTime = time;
    return 1;
}

inline NiBSplineCompPoint3Interpolator* CPt_ConstructInline(void* memory) {
    auto* self = static_cast<NiBSplineCompPoint3Interpolator*>(
        BSplinePoint3_Construct(memory, nullptr, kInvalidBSplineHandle, nullptr));
    self->vtable = reinterpret_cast<void*>(NiBSplineCompPoint3Interpolator_vtable);
    self->compScalars[0] = kFloatMax;
    self->compScalars[1] = kFloatMax;
    return self;
}

// 0x81B290: CreateClone
NiBSplineCompPoint3Interpolator* __fastcall CPt_CreateClone(NiBSplineCompPoint3Interpolator* self, re::Edx,
                                                            void* cloning) {
    void* memory = NiNew(0x34);
    NiBSplineCompPoint3Interpolator* clone = memory != nullptr ? CPt_ConstructInline(memory) : nullptr;
    BSplinePoint3_CopyMembers(self, clone, cloning);
    clone->compScalars[0] = self->compScalars[0];
    clone->compScalars[1] = self->compScalars[1];
    return clone;
}

// 0x81B2F0: CreateObject
NiObject* __cdecl CPt_CreateObject() {
    void* memory = NiNew(0x34);
    if (memory == nullptr) {
        return nullptr;
    }
    return CPt_ConstructInline(memory);
}

// 0x81B330: LoadBinary
void __fastcall CPt_LoadBinary(NiBSplineCompPoint3Interpolator* self, re::Edx, void* stream) {
    BSplinePoint3_LoadBinary(self, stream);
    LoadFloats(stream, self->compScalars, 8);
}

// 0x81B370: SaveBinary
void __fastcall CPt_SaveBinary(NiBSplineCompPoint3Interpolator* self, re::Edx, void* stream) {
    BSplinePoint3_SaveBinary(self, stream);
    SaveFloats(stream, self->compScalars, 8);
}

// 0x81B3B0: GetViewerStrings
void __fastcall CPt_GetViewerStrings(NiBSplineCompPoint3Interpolator* self, re::Edx, NiTArray<char*>* strings) {
    BSplinePoint3_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBSplineCompPoint3Interpolator_RTTI.name));
}

// ---- NiBSplineCompFloatInterpolator

// 0x81B400: IsEqual
uint8_t __fastcall CFl_IsEqual(NiBSplineCompFloatInterpolator* self, re::Edx, NiBSplineCompFloatInterpolator* other) {
    if (!BSplineFloat_IsEqual(self, other)) {
        return 0;
    }
    return ScalarsEqual(self->compScalars, other->compScalars, 2) ? 1 : 0;
}

const RTTI* __fastcall CFl_GetRTTI(NiBSplineCompFloatInterpolator*, re::Edx) {
    return &NiBSplineCompFloatInterpolator_RTTI;
}

// 0x81B450: Update(time, target, float&) (slot 22)
uint8_t __fastcall CFl_Update(NiBSplineCompFloatInterpolator* self, re::Edx, float time, NiObjectNET*, float* value) {
    if (time == self->lastTime) {
        *value = self->value;
        return 1;
    }
    if (self->handle != kInvalidBSplineHandle) {
        BSplineData_GetCompactedValueDegree3(self->data, Fraction(self, time), &self->value, 1, self->basisData,
                                             self->handle, self->compScalars[0], self->compScalars[1]);
    }
    *value = self->value;
    self->lastTime = time;
    return 1;
}

inline NiBSplineCompFloatInterpolator* CFl_ConstructInline(void* memory) {
    auto* self = static_cast<NiBSplineCompFloatInterpolator*>(
        BSplineFloat_Construct(memory, nullptr, kInvalidBSplineHandle, nullptr));
    self->vtable = reinterpret_cast<void*>(NiBSplineCompFloatInterpolator_vtable);
    self->compScalars[0] = kFloatMax;
    self->compScalars[1] = kFloatMax;
    return self;
}

// 0x81B4E0: CreateClone
NiBSplineCompFloatInterpolator* __fastcall CFl_CreateClone(NiBSplineCompFloatInterpolator* self, re::Edx,
                                                           void* cloning) {
    void* memory = NiNew(0x2C);
    NiBSplineCompFloatInterpolator* clone = memory != nullptr ? CFl_ConstructInline(memory) : nullptr;
    BSplineFloat_CopyMembers(self, clone, cloning);
    clone->compScalars[0] = self->compScalars[0];
    clone->compScalars[1] = self->compScalars[1];
    return clone;
}

// 0x81B540: CreateObject
NiObject* __cdecl CFl_CreateObject() {
    void* memory = NiNew(0x2C);
    if (memory == nullptr) {
        return nullptr;
    }
    return CFl_ConstructInline(memory);
}

// 0x81B580: LoadBinary
void __fastcall CFl_LoadBinary(NiBSplineCompFloatInterpolator* self, re::Edx, void* stream) {
    BSplineFloat_LoadBinary(self, stream);
    LoadFloats(stream, self->compScalars, 8);
}

// 0x81B5C0: SaveBinary
void __fastcall CFl_SaveBinary(NiBSplineCompFloatInterpolator* self, re::Edx, void* stream) {
    BSplineFloat_SaveBinary(self, stream);
    SaveFloats(stream, self->compScalars, 8);
}

// 0x81B600: GetViewerStrings
void __fastcall CFl_GetViewerStrings(NiBSplineCompFloatInterpolator* self, re::Edx, NiTArray<char*>* strings) {
    BSplineFloat_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBSplineCompFloatInterpolator_RTTI.name));
}

// ---- NiBSplineCompColorInterpolator (its slots in this range)

// 0x81B650: IsEqual
uint8_t __fastcall CCo_IsEqual(NiBSplineCompColorInterpolator* self, re::Edx, NiBSplineCompColorInterpolator* other) {
    if (!BSplineColor_IsEqual(self, other)) {
        return 0;
    }
    return ScalarsEqual(self->compScalars, other->compScalars, 2) ? 1 : 0;
}

const RTTI* __fastcall CCo_GetRTTI(NiBSplineCompColorInterpolator*, re::Edx) {
    return &NiBSplineCompColorInterpolator_RTTI;
}

// 0x81B6A0: the deleting destructor
void* __fastcall CCo_DeletingDestructor(NiBSplineCompColorInterpolator* self, re::Edx, uint32_t flags) {
    return DeleteBSpline(self, flags, 0x3C);
}

// 0x81B6C0: Update(time, target, colour&) (slot 19)
uint8_t __fastcall CCo_Update(NiBSplineCompColorInterpolator* self, re::Edx, float time, NiObjectNET*, NiColorA* value) {
    if (time == self->lastTime) {
        *value = self->value;
        return 1;
    }
    if (self->handle != kInvalidBSplineHandle) {
        float result[4];
        BSplineData_GetCompactedValueDegree3(self->data, Fraction(self, time), result, 4, self->basisData,
                                             self->handle, self->compScalars[0], self->compScalars[1]);
        self->value.r = result[0];
        self->value.g = result[1];
        self->value.b = result[2];
        self->value.a = result[3];
    }
    *value = self->value;
    self->lastTime = time;
    return 1;
}

inline NiBSplineCompColorInterpolator* CCo_ConstructInline(void* memory) {
    auto* self = static_cast<NiBSplineCompColorInterpolator*>(
        BSplineColor_Construct(memory, nullptr, kInvalidBSplineHandle, nullptr));
    self->vtable = reinterpret_cast<void*>(NiBSplineCompColorInterpolator_vtable);
    self->compScalars[0] = kFloatMax;
    self->compScalars[1] = kFloatMax;
    return self;
}

// 0x81B7B0: CreateClone
NiBSplineCompColorInterpolator* __fastcall CCo_CreateClone(NiBSplineCompColorInterpolator* self, re::Edx,
                                                           void* cloning) {
    void* memory = NiNew(0x3C);
    NiBSplineCompColorInterpolator* clone = memory != nullptr ? CCo_ConstructInline(memory) : nullptr;
    BSplineColor_CopyMembers(self, clone, cloning);
    clone->compScalars[0] = self->compScalars[0];
    clone->compScalars[1] = self->compScalars[1];
    return clone;
}

// 0x81B810: CreateObject
NiObject* __cdecl CCo_CreateObject() {
    void* memory = NiNew(0x3C);
    if (memory == nullptr) {
        return nullptr;
    }
    return CCo_ConstructInline(memory);
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC31_BSPLINE_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

// (a lone jmp to 0x812250, the compressed kinds' slot 5)
RE_UNHOOKED("NiBSplineCompTransformInterpolator::LinkObject thunk", 0x0081ACD0);

NIC31_BSPLINE_REVERSED("NiBSplineTransformInterpolator::GetDimension", 0x00819DC0, &ni::anim::Xf_GetDimension);
NIC31_BSPLINE_REVERSED("NiBSplineTransformInterpolator::GetDegree", 0x00819E00, &ni::anim::Xf_GetDegree);
NIC31_BSPLINE_REVERSED("NiBSplineTransformInterpolator::GetControlHandle", 0x00819E30,
                       &ni::anim::Xf_GetControlHandle);
NIC31_BSPLINE_REVERSED("NiBSplineTransformInterpolator::SetControlHandle", 0x00819E60,
                       &ni::anim::Xf_SetControlHandle);
NIC31_BSPLINE_REVERSED("NiBSplineTransformInterpolator::CopyMembers", 0x00819EA0, &ni::anim::Xf_CopyMembers);
NIC31_BSPLINE_REVERSED("NiBSplineTransformInterpolator::GetRTTI", 0x00819EE0, &ni::anim::Xf_GetRTTI);
NIC31_BSPLINE_REVERSED("NiBSplineTransformInterpolator::NiBSplineTransformInterpolator", 0x00819EF0,
                       &ni::anim::Xf_Construct);
NIC31_BSPLINE_REVERSED("NiBSplineTransformInterpolator::DeletingDestructor", 0x00819F70,
                       &ni::anim::Xf_DeletingDestructor);
NIC31_BSPLINE_REVERSED("NiBSplineTransformInterpolator::GetChannelPosed", 0x00819F90, &ni::anim::Xf_GetChannelPosed);
NIC31_BSPLINE_REVERSED("NiBSplineTransformInterpolator::CreateClone", 0x0081A010, &ni::anim::Xf_CreateClone);
NIC31_BSPLINE_REVERSED("NiBSplineTransformInterpolator::CreateObject", 0x0081A080, &ni::anim::Xf_CreateObject);
NIC31_BSPLINE_REVERSED("NiBSplineTransformInterpolator::IsEqual", 0x0081A0B0, &ni::anim::Xf_IsEqual);
NIC31_BSPLINE_REVERSED("NiBSplineTransformInterpolator::Update", 0x0081A100, &ni::anim::Xf_Update);
NIC31_BSPLINE_REVERSED("NiBSplineTransformInterpolator::LoadBinary", 0x0081A2E0, &ni::anim::Xf_LoadBinary);
NIC31_BSPLINE_REVERSED("NiBSplineTransformInterpolator::SaveBinary", 0x0081A370, &ni::anim::Xf_SaveBinary);
NIC31_BSPLINE_REVERSED("NiBSplineTransformInterpolator::GetViewerStrings", 0x0081A400,
                       &ni::anim::Xf_GetViewerStrings);

NIC31_BSPLINE_REVERSED("NiBSplinePoint3Interpolator::GetDimension", 0x0081A510, &ni::anim::Pt_GetDimension);
NIC31_BSPLINE_REVERSED("NiBSplinePoint3Interpolator::GetControlHandle", 0x0081A530, &ni::anim::Pt_GetControlHandle);
NIC31_BSPLINE_REVERSED("NiBSplinePoint3Interpolator::SetControlHandle", 0x0081A550, &ni::anim::Pt_SetControlHandle);
NIC31_BSPLINE_REVERSED("NiBSplinePoint3Interpolator::CopyMembers", 0x0081A570, &ni::anim::Pt_CopyMembers);
NIC31_BSPLINE_REVERSED("NiBSplinePoint3Interpolator::GetRTTI", 0x0081A5A0, &ni::anim::Pt_GetRTTI);
NIC31_BSPLINE_REVERSED("NiBSplinePoint3Interpolator::NiBSplinePoint3Interpolator", 0x0081A5B0,
                       &ni::anim::Pt_Construct);
NIC31_BSPLINE_REVERSED("NiBSplinePoint3Interpolator::DeletingDestructor", 0x0081A5E0,
                       &ni::anim::Pt_DeletingDestructor);
NIC31_BSPLINE_REVERSED("NiBSplinePoint3Interpolator::Update", 0x0081A600, &ni::anim::Pt_Update);
NIC31_BSPLINE_REVERSED("NiBSplinePoint3Interpolator::CreateClone", 0x0081A6D0, &ni::anim::Pt_CreateClone);
NIC31_BSPLINE_REVERSED("NiBSplinePoint3Interpolator::CreateObject", 0x0081A730, &ni::anim::Pt_CreateObject);
NIC31_BSPLINE_REVERSED("NiBSplinePoint3Interpolator::LoadBinary", 0x0081A770, &ni::anim::Pt_LoadBinary);
NIC31_BSPLINE_REVERSED("NiBSplinePoint3Interpolator::SaveBinary", 0x0081A7C0, &ni::anim::Pt_SaveBinary);
NIC31_BSPLINE_REVERSED("NiBSplinePoint3Interpolator::IsEqual", 0x0081A810, &ni::anim::Pt_IsEqual);
NIC31_BSPLINE_REVERSED("NiBSplinePoint3Interpolator::GetViewerStrings", 0x0081A850, &ni::anim::Pt_GetViewerStrings);

NIC31_BSPLINE_REVERSED("NiBSplineFloatInterpolator::NiBSplineFloatInterpolator", 0x0081A8E0,
                       &ni::anim::Fl_Construct);
NIC31_BSPLINE_REVERSED("NiBSplinePoint3Interpolator::GetChannelPosed", 0x0081A910, &ni::anim::One_GetChannelPosed);
NIC31_BSPLINE_REVERSED("NiBSplineFloatInterpolator::GetDimension", 0x0081A940, &ni::anim::Fl_GetDimension);
NIC31_BSPLINE_REVERSED("NiBSplineFloatInterpolator::GetControlHandle", 0x0081A950, &ni::anim::Fl_GetControlHandle);
NIC31_BSPLINE_REVERSED("NiBSplineFloatInterpolator::SetControlHandle", 0x0081A970, &ni::anim::Fl_SetControlHandle);
NIC31_BSPLINE_REVERSED("NiBSplineFloatInterpolator::CopyMembers", 0x0081A990, &ni::anim::Fl_CopyMembers);
NIC31_BSPLINE_REVERSED("NiBSplineFloatInterpolator::CreateObject", 0x0081A9C0, &ni::anim::Fl_CreateObject);
NIC31_BSPLINE_REVERSED("NiBSplineFloatInterpolator::IsEqual", 0x0081AA00, &ni::anim::Fl_IsEqual);
NIC31_BSPLINE_REVERSED("NiBSplineFloatInterpolator::GetRTTI", 0x0081AA40, &ni::anim::Fl_GetRTTI);
NIC31_BSPLINE_REVERSED("NiBSplineFloatInterpolator::DeletingDestructor", 0x0081AA50,
                       &ni::anim::Fl_DeletingDestructor);
NIC31_BSPLINE_REVERSED("NiBSplineFloatInterpolator::Update", 0x0081AA70, &ni::anim::Fl_Update);
NIC31_BSPLINE_REVERSED("NiBSplineFloatInterpolator::CreateClone", 0x0081AAF0, &ni::anim::Fl_CreateClone);
NIC31_BSPLINE_REVERSED("NiBSplineFloatInterpolator::LoadBinary", 0x0081AB40, &ni::anim::Fl_LoadBinary);
NIC31_BSPLINE_REVERSED("NiBSplineFloatInterpolator::SaveBinary", 0x0081ABA0, &ni::anim::Fl_SaveBinary);
NIC31_BSPLINE_REVERSED("NiBSplineFloatInterpolator::GetViewerStrings", 0x0081AC00, &ni::anim::Fl_GetViewerStrings);

NIC31_BSPLINE_REVERSED("NiBSplineCompTransformInterpolator::IsEqual", 0x0081ACE0, &ni::anim::CXf_IsEqual);
NIC31_BSPLINE_REVERSED("NiBSplineCompTransformInterpolator::GetRTTI", 0x0081AD20, &ni::anim::CXf_GetRTTI);
NIC31_BSPLINE_REVERSED("NiBSplineCompTransformInterpolator::DeletingDestructor", 0x0081AD30,
                       &ni::anim::CXf_DeletingDestructor);
NIC31_BSPLINE_REVERSED("NiBSplineCompTransformInterpolator::CreateClone", 0x0081AD50, &ni::anim::CXf_CreateClone);
NIC31_BSPLINE_REVERSED("NiBSplineCompTransformInterpolator::CreateObject", 0x0081ADE0, &ni::anim::CXf_CreateObject);
NIC31_BSPLINE_REVERSED("NiBSplineCompTransformInterpolator::Update", 0x0081AE40, &ni::anim::CXf_Update);
NIC31_BSPLINE_REVERSED("NiBSplineCompTransformInterpolator::LoadBinary", 0x0081B050, &ni::anim::CXf_LoadBinary);
NIC31_BSPLINE_REVERSED("NiBSplineCompTransformInterpolator::SaveBinary", 0x0081B090, &ni::anim::CXf_SaveBinary);
NIC31_BSPLINE_REVERSED("NiBSplineCompTransformInterpolator::GetViewerStrings", 0x0081B0D0,
                       &ni::anim::CXf_GetViewerStrings);
NIC31_BSPLINE_REVERSED("NiBSplineCompTransformInterpolator::RegisterStreamables", 0x0081B120,
                       &ni::anim::Comp_RegisterStreamables);

NIC31_BSPLINE_REVERSED("NiBSplineCompPoint3Interpolator::IsEqual", 0x0081B140, &ni::anim::CPt_IsEqual);
NIC31_BSPLINE_REVERSED("NiBSplineCompPoint3Interpolator::GetRTTI", 0x0081B180, &ni::anim::CPt_GetRTTI);
NIC31_BSPLINE_REVERSED("NiBSplineCompPoint3Interpolator::DeletingDestructor", 0x0081B190,
                       &ni::anim::CPt_DeletingDestructor);
NIC31_BSPLINE_REVERSED("NiBSplineCompPoint3Interpolator::Update", 0x0081B1B0, &ni::anim::CPt_Update);
NIC31_BSPLINE_REVERSED("NiBSplineCompPoint3Interpolator::CreateClone", 0x0081B290, &ni::anim::CPt_CreateClone);
NIC31_BSPLINE_REVERSED("NiBSplineCompPoint3Interpolator::CreateObject", 0x0081B2F0, &ni::anim::CPt_CreateObject);
NIC31_BSPLINE_REVERSED("NiBSplineCompPoint3Interpolator::LoadBinary", 0x0081B330, &ni::anim::CPt_LoadBinary);
NIC31_BSPLINE_REVERSED("NiBSplineCompPoint3Interpolator::SaveBinary", 0x0081B370, &ni::anim::CPt_SaveBinary);
NIC31_BSPLINE_REVERSED("NiBSplineCompPoint3Interpolator::GetViewerStrings", 0x0081B3B0,
                       &ni::anim::CPt_GetViewerStrings);

NIC31_BSPLINE_REVERSED("NiBSplineCompFloatInterpolator::IsEqual", 0x0081B400, &ni::anim::CFl_IsEqual);
NIC31_BSPLINE_REVERSED("NiBSplineCompFloatInterpolator::GetRTTI", 0x0081B440, &ni::anim::CFl_GetRTTI);
NIC31_BSPLINE_REVERSED("NiBSplineCompFloatInterpolator::Update", 0x0081B450, &ni::anim::CFl_Update);
NIC31_BSPLINE_REVERSED("NiBSplineCompFloatInterpolator::CreateClone", 0x0081B4E0, &ni::anim::CFl_CreateClone);
NIC31_BSPLINE_REVERSED("NiBSplineCompFloatInterpolator::CreateObject", 0x0081B540, &ni::anim::CFl_CreateObject);
NIC31_BSPLINE_REVERSED("NiBSplineCompFloatInterpolator::LoadBinary", 0x0081B580, &ni::anim::CFl_LoadBinary);
NIC31_BSPLINE_REVERSED("NiBSplineCompFloatInterpolator::SaveBinary", 0x0081B5C0, &ni::anim::CFl_SaveBinary);
NIC31_BSPLINE_REVERSED("NiBSplineCompFloatInterpolator::GetViewerStrings", 0x0081B600,
                       &ni::anim::CFl_GetViewerStrings);

NIC31_BSPLINE_REVERSED("NiBSplineCompColorInterpolator::IsEqual", 0x0081B650, &ni::anim::CCo_IsEqual);
NIC31_BSPLINE_REVERSED("NiBSplineCompColorInterpolator::GetRTTI", 0x0081B690, &ni::anim::CCo_GetRTTI);
NIC31_BSPLINE_REVERSED("NiBSplineCompColorInterpolator::DeletingDestructor", 0x0081B6A0,
                       &ni::anim::CCo_DeletingDestructor);
NIC31_BSPLINE_REVERSED("NiBSplineCompColorInterpolator::Update", 0x0081B6C0, &ni::anim::CCo_Update);
NIC31_BSPLINE_REVERSED("NiBSplineCompColorInterpolator::CreateClone", 0x0081B7B0, &ni::anim::CCo_CreateClone);
NIC31_BSPLINE_REVERSED("NiBSplineCompColorInterpolator::CreateObject", 0x0081B810, &ni::anim::CCo_CreateObject);
