// The B-spline animation data (src/gamebryo/NiBSplineData.h): NiBSplineColorInterpolator and the
// compressed kind's slots of this part of the library, NiBSplineData (the control points and their
// cubic evaluation, plain and compressed), NiBSplineBasisData and the uniform cubic basis
// NiBSplineBasis<float, 3>::Compute.
//
// Main-thread float maths at the game's 24-bit x87 precision: every operation rounded to float,
// in the game's order (the trees below are the exe's, operand for operand).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiBSplineData.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni::anim {
namespace {

using scene::AddViewerString;
using scene::BinaryStream;
using scene::GetViewerString;
using scene::Object_Destruct;
using scene::Object_GetViewerStrings;
using scene::Object_IsEqual;
using scene::Object_LoadBinary;
using scene::RefObject_Construct;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiBSplineCompColorInterpolator_RTTI, 0x020E52D8);
GAME_VAR(RTTI, NiBSplineColorInterpolator_RTTI, 0x020E530C);
GAME_VAR(RTTI, NiBSplineData_RTTI, 0x020E5340);
GAME_VAR(RTTI, NiBSplineBasisData_RTTI, 0x020E5374);

constexpr uintptr_t NiBSplineColorInterpolator_vtable = 0x00967550;
constexpr uintptr_t NiBSplineData_vtable = 0x00967620;
constexpr uintptr_t NiBSplineBasisData_vtable = 0x009676A0;
constexpr float kThird = 0.333333343f;   // (the exe's 1/3, float and double alike)

GAME_FN(char*, __cdecl, GetViewerStringUint, 0x007856F0, const char* prefix, uint32_t value);
GAME_FN(void, __thiscall, Object_SaveBinary, 0x008A20A0, void* self, void* stream);
GAME_FN(void, __thiscall, Object_CopyMembers, 0x00755410, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, ColorA_LoadBinary, 0x007625D0, NiColorA* self, void* stream);
GAME_FN(void, __thiscall, ColorA_SaveBinary, 0x00762670, const NiColorA* self, void* stream);
GAME_FN(uint8_t, __thiscall, ColorA_NotEqual, 0x00769390, const NiColorA* self, const NiColorA* other);
// shorts in -32767..32767 to floats over an offset and a half range (the decompressor the
// compressed B-splines share; its second argument unused)
GAME_FN(void, __cdecl, DecompressControlPoints, 0x008C5690, const int16_t* src, uint32_t unused, float offset,
        float halfRange, float* dest, uint32_t count);
// NiBSplineBasisData::GetDegree3Basis (this + 8: a getter folded with others)
GAME_FN(NiBSplineBasis3*, __thiscall, BasisData_GetDegree3Basis, 0x0077C280, NiBSplineBasisData* self);
// NiBSplineInterpolator's (the next chunk's)
GAME_FN(void, __thiscall, BSplineInterpolator_Construct, 0x00828560, void* self, NiBSplineData* data,
        NiBSplineBasisData* basisData);
GAME_FN(uint8_t, __thiscall, BSplineInterpolator_RegisterStreamables, 0x00828490, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, BSplineInterpolator_IsEqual, 0x008284D0, void* self, void* other);
GAME_FN(void, __thiscall, BSplineInterpolator_CopyMembers, 0x00828610, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, BSplineInterpolator_LoadBinary, 0x008286C0, void* self, void* stream);
GAME_FN(void, __thiscall, BSplineInterpolator_SaveBinary, 0x008287A0, void* self, void* stream);
GAME_FN(void, __thiscall, BSplineInterpolator_GetViewerStrings, 0x00828840, void* self, NiTArray<char*>* strings);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, BSplineColor_LoadBinary, 0x0081BBD0, NiBSplineColorInterpolator* self, void* stream);
GAME_FN(void, __thiscall, BSplineColor_SaveBinary, 0x0081BC20, NiBSplineColorInterpolator* self, void* stream);
GAME_FN(void, __thiscall, BSplineColor_GetViewerStrings, 0x0081BCB0, NiBSplineColorInterpolator* self,
        NiTArray<char*>* strings);
GAME_FN(void, __thiscall, Basis_Compute, 0x0081BDB0, NiBSplineBasis3* self, float t, int32_t* outMin,
        int32_t* outMax);
GAME_FN(void, __thiscall, BSplineData_GetValueDegree3, 0x0081C490, NiBSplineData* self, float t, float* pos,
        int32_t dimension, NiBSplineBasisData* basisData, uint32_t handle);

// NiStream's binary reads and writes of an array: `bytes` bytes of `componentSize` components
inline void LoadArray(void* stream, void* buffer, uint32_t bytes, uint32_t componentSize) {
    BinaryStream* in = scene::At<BinaryStream*>(stream, 0x188);
    uint32_t size = componentSize;
    in->position += in->read(in, buffer, bytes, &size, 1);
}
inline void SaveArray(void* stream, const void* buffer, uint32_t bytes, uint32_t componentSize) {
    BinaryStream* out = scene::At<BinaryStream*>(stream, 0x18C);
    uint32_t size = componentSize;
    out->position += out->write(out, buffer, bytes, &size, 1);
}

// ---- NiBSplineColorInterpolator / NiBSplineCompColorInterpolator

inline void ColorInitMembers(NiBSplineColorInterpolator* self, uint32_t handle) {
    self->vtable = reinterpret_cast<void*>(NiBSplineColorInterpolator_vtable);
    self->value = {0.0f, 0.0f, 0.0f, 0.0f};
    self->handle = handle;
}

inline void ColorCopyValues(const NiBSplineColorInterpolator* self, NiBSplineColorInterpolator* dest) {
    dest->value = self->value;
    dest->handle = self->handle;
}

// 0x81B850: NiBSplineCompColorInterpolator::LoadBinary (the offset and half range after)
void __fastcall CompColor_LoadBinary(NiBSplineCompColorInterpolator* self, re::Edx, void* stream) {
    BSplineColor_LoadBinary(self, stream);
    LoadArray(stream, self->compScalars, 8, 4);
}

// 0x81B890: NiBSplineCompColorInterpolator::SaveBinary
void __fastcall CompColor_SaveBinary(NiBSplineCompColorInterpolator* self, re::Edx, void* stream) {
    BSplineColor_SaveBinary(self, stream);
    SaveArray(stream, self->compScalars, 8, 4);
}

// 0x81B8D0: NiBSplineCompColorInterpolator::GetViewerStrings (the scalars not listed)
void __fastcall CompColor_GetViewerStrings(NiBSplineCompColorInterpolator* self, re::Edx, NiTArray<char*>* strings) {
    BSplineColor_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBSplineCompColorInterpolator_RTTI.name));
}

// 0x81B920: GetChannelCount (slot 37): 1. (The linker folded every "return 1" of this kind
// into it: the one-channel key interpolators' GetKeyChannelCount, the single-interpolator
// controllers' GetInterpolatorCount)
uint16_t __fastcall Color_GetChannelCount(void*, re::Edx) {
    return 1;
}

// 0x81B930: GetDimension(channel) (slot 39): 4 floats for the colour channel
uint32_t __fastcall Color_GetDimension(NiBSplineColorInterpolator*, re::Edx, uint16_t channel) {
    return channel != 0 ? 0 : 4;
}

// 0x81B950: GetControlHandle(channel) (slot 44)
uint32_t __fastcall Color_GetControlHandle(NiBSplineColorInterpolator* self, re::Edx, uint16_t channel) {
    return channel == 0 ? self->handle : kInvalidBSplineHandle;
}

// 0x81B970: SetControlHandle(handle, channel) (slot 45)
void __fastcall Color_SetControlHandle(NiBSplineColorInterpolator* self, re::Edx, uint32_t handle, uint16_t channel) {
    if (channel == 0) {
        self->handle = handle;
    }
}

// 0x81B990: CopyMembers
void __fastcall Color_CopyMembers(NiBSplineColorInterpolator* self, re::Edx, NiBSplineColorInterpolator* dest,
                                  void* cloning) {
    BSplineInterpolator_CopyMembers(self, dest, cloning);
    ColorCopyValues(self, dest);
}

// 0x81B9D0: RegisterStreamables
uint8_t __fastcall Color_RegisterStreamables(NiBSplineColorInterpolator* self, re::Edx, void* stream) {
    return BSplineInterpolator_RegisterStreamables(self, stream) != 0 ? 1 : 0;
}

// 0x81B9F0
const RTTI* __fastcall Color_GetRTTI(NiBSplineColorInterpolator*, re::Edx) {
    return &NiBSplineColorInterpolator_RTTI;
}

// 0x81BA00: NiBSplineColorInterpolator(data, handle, basis data)
NiBSplineColorInterpolator* __fastcall Color_Construct(NiBSplineColorInterpolator* self, re::Edx, NiBSplineData* data,
                                                       uint32_t handle, NiBSplineBasisData* basisData) {
    BSplineInterpolator_Construct(self, data, basisData);
    ColorInitMembers(self, handle);
    return self;
}

// 0x81BA40: Update(time, target, colour&) (slot 19): the curve at the time's fraction of the
// range (none without control points: the last value kept)
uint8_t __fastcall Color_Update(NiBSplineColorInterpolator* self, re::Edx, float time, NiObjectNET*, NiColorA* value) {
    if (time == self->lastTime) {
        *value = self->value;
        return 1;
    }
    if (self->handle != kInvalidBSplineHandle) {
        float t = (time - self->startTime) / (self->endTime - self->startTime);
        NiColorA color;
        BSplineData_GetValueDegree3(self->data, t, &color.r, 4, self->basisData, self->handle);
        self->value = color;
    }
    *value = self->value;
    self->lastTime = time;
    return 1;
}

// 0x81BB20: CreateClone (its CopyMembers inlined; a failed allocation still copied onto null, as
// the game has it)
NiBSplineColorInterpolator* __fastcall Color_CreateClone(NiBSplineColorInterpolator* self, re::Edx, void* cloning) {
    auto* clone = static_cast<NiBSplineColorInterpolator*>(NiNew(0x34));
    if (clone != nullptr) {
        BSplineInterpolator_Construct(clone, nullptr, nullptr);
        ColorInitMembers(clone, kInvalidBSplineHandle);
    } else {
        clone = nullptr;
    }
    BSplineInterpolator_CopyMembers(self, clone, cloning);
    ColorCopyValues(self, clone);
    return clone;
}

// 0x81BB90: CreateObject
NiObject* __cdecl Color_CreateObject() {
    auto* self = static_cast<NiBSplineColorInterpolator*>(NiNew(0x34));
    if (self == nullptr) {
        return nullptr;
    }
    BSplineInterpolator_Construct(self, nullptr, nullptr);
    ColorInitMembers(self, kInvalidBSplineHandle);
    return self;
}

// 0x81BBD0: LoadBinary
void __fastcall Color_LoadBinary(NiBSplineColorInterpolator* self, re::Edx, void* stream) {
    BSplineInterpolator_LoadBinary(self, stream);
    ColorA_LoadBinary(&self->value, stream);
    LoadArray(stream, &self->handle, 4, 4);
}

// 0x81BC20: SaveBinary
void __fastcall Color_SaveBinary(NiBSplineColorInterpolator* self, re::Edx, void* stream) {
    BSplineInterpolator_SaveBinary(self, stream);
    ColorA_SaveBinary(&self->value, stream);
    SaveArray(stream, &self->handle, 4, 4);
}

// 0x81BC70: IsEqual
uint8_t __fastcall Color_IsEqual(NiBSplineColorInterpolator* self, re::Edx, NiBSplineColorInterpolator* other) {
    if (!BSplineInterpolator_IsEqual(self, other)) {
        return 0;
    }
    if (ColorA_NotEqual(&self->value, &other->value)) {
        return 0;
    }
    return other->handle == self->handle ? 1 : 0;
}

// 0x81BCB0: GetViewerStrings
void __fastcall Color_GetViewerStrings(NiBSplineColorInterpolator* self, re::Edx, NiTArray<char*>* strings) {
    BSplineInterpolator_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBSplineColorInterpolator_RTTI.name));
    AddViewerString(strings, GetViewerStringUint("m_kColorACPHandle", self->handle));
}

// ---- NiBSplineData

// 0x81BD40: CreateObject
NiObject* __cdecl Data_CreateObject() {
    auto* self = static_cast<NiBSplineData*>(NiNew(0x18));
    if (self == nullptr) {
        return nullptr;
    }
    RefObject_Construct(self);
    self->controlPoints = nullptr;
    self->compactControlPoints = nullptr;
    self->controlPointCount = 0;
    self->compactCount = 0;
    self->vtable = reinterpret_cast<void*>(NiBSplineData_vtable);
    return self;
}

// 0x81BD80: IsEqual (the counts only)
uint8_t __fastcall Data_IsEqual(NiBSplineData* self, re::Edx, NiBSplineData* other) {
    if (Object_IsEqual(self, other) && other->controlPointCount == self->controlPointCount &&
        other->compactCount == self->compactCount) {
        return 1;
    }
    return 0;
}

// 0x81BDB0: NiBSplineBasis<float, 3>::Compute(t, min&, max&): the four basis values at t (0..1
// over the curve) of the open uniform cubic B-spline over the control points, and the control
// points they weigh (min..max); the last time's kept. Under 4 control points: the values left
// alone.
void __fastcall Basis_ComputeImpl(NiBSplineBasis3* self, re::Edx, float t, int32_t* outMin, int32_t* outMax) {
    int32_t n = self->numControlPoints;
    int32_t nm3 = n - 3;
    if (t == self->lastTime) {
        *outMin = self->lastMin;
        *outMax = self->lastMax;
        return;
    }
    self->lastTime = t;
    float q = float(nm3);
    int32_t i;
    if (1.0f > t) {
        i = crt::FloatToInt(q * t) + 3;
    } else {
        i = n - 1;   // (and for a NaN)
    }
    int32_t mn = i - 3;
    *outMin = mn;
    self->lastMin = mn;
    *outMax = i;
    self->lastMax = i;
    float u = q * t;
    float* v = self->values;
    if (n >= 7) {
        // the knots near the ends are doubled up: the weights' factors picked by the interval
        float A = i > 5 ? float(i - 5) : 0.0f;
        float B = i > 4 ? float(i - 4) : 0.0f;
        float C = float(i - 3);
        float D = float(i - 2);
        float E = i < n - 2 ? float(i - 1) : q;
        float F = i < nm3 ? float(i) : q;
        float G = i == 3 ? 1.0f : 0.5f;
        float H = i == n - 1 ? 1.0f : 0.5f;
        float J = i == 3 ? 1.0f : (i == 4 ? 0.5f : kThird);
        float K = (i == 3 || i == n - 1) ? 0.5f : kThird;
        float L = i == n - 1 ? 1.0f : (i == n - 2 ? 0.5f : kThird);
        float uB = u - B;
        float uC = u - C;
        float Du = D - u;
        float Eu = E - u;
        float DuG = Du * G;
        float uCH = uC * H;
        float t1 = (Du * J) * DuG;
        float t2 = (DuG * uB + uCH * Eu) * K;
        float t3 = uCH * (uC * L);
        v[0] = Du * t1;
        v[1] = Eu * t2 + t1 * (u - A);
        v[2] = t2 * uB + (F - u) * t3;
        v[3] = uC * t3;
        return;
    }
    if (n == 6) {
        if (i == 3) {
            float a = 1.0f - u;
            float b = 2.0f - u;
            float c = u * 0.5f;
            float a2 = a * a;
            float m = 0.5f * (c * b + a * u);
            float r = c * (u * kThird);
            v[0] = a * a2;
            v[1] = a2 * u + b * m;
            v[2] = m * u + (3.0f - u) * r;
            v[3] = u * r;
        } else if (i == 4) {
            float a = u - 1.0f;
            float b = 3.0f - u;
            float c = 1.0f - u * 0.5f;
            float d = 0.5f * a;
            float c2 = c * c;
            float m = (c * u + d * b) * kThird;
            float d2 = d * d;
            v[0] = (2.0f - u) * c2;
            v[1] = c2 * u + m * b;
            v[2] = u * m + b * d2;
            v[3] = d2 * a;
        } else {
            float a = u - 1.0f;
            float b = u - 2.0f;
            float c = 3.0f - u;
            float h = c * 0.5f;
            float b2 = b * b;
            float r = (c * kThird) * h;
            float m = 0.5f * (h * a + c * b);
            v[0] = r * c;
            v[1] = u * r + m * c;
            v[2] = a * m + c * b2;
            v[3] = b2 * b;
        }
        return;
    }
    if (n == 5) {
        if (i == 3) {
            float a = 1.0f - u;
            float b = 2.0f - u;
            float c = u * 0.5f;
            float a2 = a * a;
            float m = 0.5f * (c * b + a * u);
            float r = c * c;
            v[0] = a * a2;
            v[1] = a2 * u + m * b;
            v[2] = m * u + b * r;
            v[3] = u * r;
        } else {
            float a = u - 1.0f;
            float b = 2.0f - u;
            float c = u * 0.5f;
            float a2 = a * a;
            float d = 1.0f - c;
            float d2 = d * d;
            float m = (c + a) * d;
            v[0] = d2 * b;
            v[1] = d2 * u + m * b;
            v[2] = u * m + b * a2;
            v[3] = a2 * a;
        }
        return;
    }
    if (n == 4) {
        // (a Bezier segment)
        float a = 1.0f - u;
        float u2 = u * u;
        float a2 = a * a;
        v[0] = a2 * a;
        v[1] = a2 * (u * 3.0f);
        v[2] = a * (3.0f * u2);
        v[3] = u * u2;
    }
}

// 0x81C440
const RTTI* __fastcall Data_GetRTTI(NiBSplineData*, re::Edx) {
    return &NiBSplineData_RTTI;
}

// 0x81C450: the deleting destructor (both point arrays freed)
NiBSplineData* __fastcall Data_DeletingDestructor(NiBSplineData* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiBSplineData_vtable);
    NiFree(self->controlPoints);
    NiFree(self->compactControlPoints);
    Object_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x18);
    }
    return self;
}

// the weighted sum of the four control points (dimension floats each, from src) the basis gives
inline void SumDegree3(const NiBSplineBasis3* basis, int32_t mn, int32_t mx, const float* src, float* pos,
                       int32_t dimension) {
    float b = basis->values[0];
    for (uint32_t j = 0; j < uint32_t(dimension); j++) {
        pos[j] = *src++ * b;
    }
    for (int32_t i = mn + 1; i <= mx; i++) {
        b = basis->values[i - mn];
        for (uint32_t j = 0; j < uint32_t(dimension); j++) {
            pos[j] = *src++ * b + pos[j];
        }
    }
}

// 0x81C490: GetValueDegree3(t, pos, dimension, basis data, handle): the curve's point at t
void __fastcall Data_GetValueDegree3(NiBSplineData* self, re::Edx, float t, float* pos, int32_t dimension,
                                     NiBSplineBasisData* basisData, uint32_t handle) {
    NiBSplineBasis3* basis = BasisData_GetDegree3Basis(basisData);
    int32_t mn = 0;
    int32_t mx = 0;
    Basis_Compute(basis, t, &mn, &mx);
    const float* src = self->controlPoints + (uint32_t(mn * dimension) + handle);
    SumDegree3(basis, mn, mx, src, pos, dimension);
}

// 0x81C600: GetCompactedValueDegree3(t, pos, dimension, basis data, handle, offset, half range):
// the same from the compressed points (the four needed decompressed first)
void __fastcall Data_GetCompactedValueDegree3(NiBSplineData* self, re::Edx, float t, float* pos, int32_t dimension,
                                              NiBSplineBasisData* basisData, uint32_t handle, float offset,
                                              float halfRange) {
    float buffer[16];
    NiBSplineBasis3* basis = BasisData_GetDegree3Basis(basisData);
    int32_t mn = 0;
    int32_t mx = 0;
    Basis_Compute(basis, t, &mn, &mx);
    DecompressControlPoints(self->compactControlPoints + (uint32_t(mn * dimension) + handle), uint32_t(dimension) * 4,
                            offset, halfRange, buffer, uint32_t(dimension) * 4);
    SumDegree3(basis, mn, mx, buffer, pos, dimension);
}

// 0x81C7D0: LoadBinary (each array NiMalloc'd when it has points)
void __fastcall Data_LoadBinary(NiBSplineData* self, re::Edx, void* stream) {
    Object_LoadBinary(self, stream);
    LoadArray(stream, &self->controlPointCount, 4, 4);
    if (self->controlPointCount != 0) {
        self->controlPoints = static_cast<float*>(NiMalloc(self->controlPointCount * 4));
        LoadArray(stream, self->controlPoints, self->controlPointCount * 4, 4);
    }
    LoadArray(stream, &self->compactCount, 4, 4);
    if (self->compactCount != 0) {
        self->compactControlPoints = static_cast<int16_t*>(NiMalloc(self->compactCount * 2));
        LoadArray(stream, self->compactControlPoints, self->compactCount * 2, 2);
    }
}

// 0x81C8B0: SaveBinary
void __fastcall Data_SaveBinary(NiBSplineData* self, re::Edx, void* stream) {
    Object_SaveBinary(self, stream);
    SaveArray(stream, &self->controlPointCount, 4, 4);
    if (self->controlPointCount != 0) {
        SaveArray(stream, self->controlPoints, self->controlPointCount * 4, 4);
    }
    SaveArray(stream, &self->compactCount, 4, 4);
    if (self->compactCount != 0) {
        SaveArray(stream, self->compactControlPoints, self->compactCount * 2, 2);
    }
}

// 0x81C980: GetViewerStrings
void __fastcall Data_GetViewerStrings(NiBSplineData* self, re::Edx, NiTArray<char*>* strings) {
    Object_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBSplineData_RTTI.name));
    AddViewerString(strings, GetViewerStringUint("m_uiControlPointCount", self->controlPointCount));
    AddViewerString(strings, GetViewerStringUint("m_uiCompactControlPointCount", self->compactCount));
}

// ---- NiBSplineBasisData

inline void BasisInitMembers(NiBSplineBasisData* self) {
    self->vtable = reinterpret_cast<void*>(NiBSplineBasisData_vtable);
    self->basis.lastTime = -1.0f;
    self->basis.numControlPoints = 0;
    self->basis.lastMin = 0;
    self->basis.lastMax = 3;
}

// 0x81CA50
const RTTI* __fastcall Basis_GetRTTI(NiBSplineBasisData*, re::Edx) {
    return &NiBSplineBasisData_RTTI;
}

// 0x81CA60: CreateClone (the basis copied whole; a failed allocation still copied onto null)
NiBSplineBasisData* __fastcall Basis_CreateClone(NiBSplineBasisData* self, re::Edx, void* cloning) {
    auto* clone = static_cast<NiBSplineBasisData*>(NiNew(0x28));
    if (clone != nullptr) {
        RefObject_Construct(clone);
        BasisInitMembers(clone);
    } else {
        clone = nullptr;
    }
    Object_CopyMembers(self, clone, cloning);
    clone->basis = self->basis;
    return clone;
}

// 0x81CAC0: CreateObject
NiObject* __cdecl Basis_CreateObject() {
    auto* self = static_cast<NiBSplineBasisData*>(NiNew(0x28));
    if (self == nullptr) {
        return nullptr;
    }
    RefObject_Construct(self);
    BasisInitMembers(self);
    return self;
}

// 0x81CB10: IsEqual (the control point counts)
uint8_t __fastcall Basis_IsEqual(NiBSplineBasisData* self, re::Edx, NiBSplineBasisData* other) {
    if (!Object_IsEqual(self, other)) {
        return 0;
    }
    return other->basis.numControlPoints == self->basis.numControlPoints ? 1 : 0;
}

// 0x81CB40: the deleting destructor
NiBSplineBasisData* __fastcall Basis_DeletingDestructor(NiBSplineBasisData* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiBSplineBasisData_vtable);
    Object_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x28);
    }
    return self;
}

// 0x81CB70: LoadBinary (the count only)
void __fastcall Basis_LoadBinary(NiBSplineBasisData* self, re::Edx, void* stream) {
    Object_LoadBinary(self, stream);
    LoadArray(stream, &self->basis.numControlPoints, 4, 4);
}

// 0x81CBB0: SaveBinary
void __fastcall Basis_SaveBinary(NiBSplineBasisData* self, re::Edx, void* stream) {
    Object_SaveBinary(self, stream);
    SaveArray(stream, &self->basis.numControlPoints, 4, 4);
}

// 0x81CBF0: GetViewerStrings
void __fastcall Basis_GetViewerStrings(NiBSplineBasisData* self, re::Edx, NiTArray<char*>* strings) {
    Object_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBSplineBasisData_RTTI.name));
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC32_BSPLINE_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIC32_BSPLINE_REVERSED("NiBSplineCompColorInterpolator::LoadBinary", 0x0081B850, &ni::anim::CompColor_LoadBinary);
NIC32_BSPLINE_REVERSED("NiBSplineCompColorInterpolator::SaveBinary", 0x0081B890, &ni::anim::CompColor_SaveBinary);
NIC32_BSPLINE_REVERSED("NiBSplineCompColorInterpolator::GetViewerStrings", 0x0081B8D0,
                       &ni::anim::CompColor_GetViewerStrings);
NIC32_BSPLINE_REVERSED("NiBSplineColorInterpolator::GetChannelCount", 0x0081B920, &ni::anim::Color_GetChannelCount);
NIC32_BSPLINE_REVERSED("NiBSplineColorInterpolator::GetDimension", 0x0081B930, &ni::anim::Color_GetDimension);
NIC32_BSPLINE_REVERSED("NiBSplineColorInterpolator::GetControlHandle", 0x0081B950,
                       &ni::anim::Color_GetControlHandle);
NIC32_BSPLINE_REVERSED("NiBSplineColorInterpolator::SetControlHandle", 0x0081B970,
                       &ni::anim::Color_SetControlHandle);
NIC32_BSPLINE_REVERSED("NiBSplineColorInterpolator::CopyMembers", 0x0081B990, &ni::anim::Color_CopyMembers);
NIC32_BSPLINE_REVERSED("NiBSplineColorInterpolator::RegisterStreamables", 0x0081B9D0,
                       &ni::anim::Color_RegisterStreamables);
NIC32_BSPLINE_REVERSED("NiBSplineColorInterpolator::GetRTTI", 0x0081B9F0, &ni::anim::Color_GetRTTI);
NIC32_BSPLINE_REVERSED("NiBSplineColorInterpolator::NiBSplineColorInterpolator", 0x0081BA00,
                       &ni::anim::Color_Construct);
NIC32_BSPLINE_REVERSED("NiBSplineColorInterpolator::Update", 0x0081BA40, &ni::anim::Color_Update);
NIC32_BSPLINE_REVERSED("NiBSplineColorInterpolator::CreateClone", 0x0081BB20, &ni::anim::Color_CreateClone);
NIC32_BSPLINE_REVERSED("NiBSplineColorInterpolator::CreateObject", 0x0081BB90, &ni::anim::Color_CreateObject);
NIC32_BSPLINE_REVERSED("NiBSplineColorInterpolator::LoadBinary", 0x0081BBD0, &ni::anim::Color_LoadBinary);
NIC32_BSPLINE_REVERSED("NiBSplineColorInterpolator::SaveBinary", 0x0081BC20, &ni::anim::Color_SaveBinary);
NIC32_BSPLINE_REVERSED("NiBSplineColorInterpolator::IsEqual", 0x0081BC70, &ni::anim::Color_IsEqual);
NIC32_BSPLINE_REVERSED("NiBSplineColorInterpolator::GetViewerStrings", 0x0081BCB0,
                       &ni::anim::Color_GetViewerStrings);
NIC32_BSPLINE_REVERSED("NiBSplineData::CreateObject", 0x0081BD40, &ni::anim::Data_CreateObject);
NIC32_BSPLINE_REVERSED("NiBSplineData::IsEqual", 0x0081BD80, &ni::anim::Data_IsEqual);
NIC32_BSPLINE_REVERSED("NiBSplineBasis<float,3>::Compute", 0x0081BDB0, &ni::anim::Basis_ComputeImpl);
NIC32_BSPLINE_REVERSED("NiBSplineData::GetRTTI", 0x0081C440, &ni::anim::Data_GetRTTI);
NIC32_BSPLINE_REVERSED("NiBSplineData::DeletingDestructor", 0x0081C450, &ni::anim::Data_DeletingDestructor);
NIC32_BSPLINE_REVERSED("NiBSplineData::GetValueDegree3", 0x0081C490, &ni::anim::Data_GetValueDegree3);
NIC32_BSPLINE_REVERSED("NiBSplineData::GetCompactedValueDegree3", 0x0081C600,
                       &ni::anim::Data_GetCompactedValueDegree3);
NIC32_BSPLINE_REVERSED("NiBSplineData::LoadBinary", 0x0081C7D0, &ni::anim::Data_LoadBinary);
NIC32_BSPLINE_REVERSED("NiBSplineData::SaveBinary", 0x0081C8B0, &ni::anim::Data_SaveBinary);
NIC32_BSPLINE_REVERSED("NiBSplineData::GetViewerStrings", 0x0081C980, &ni::anim::Data_GetViewerStrings);
NIC32_BSPLINE_REVERSED("NiBSplineBasisData::GetRTTI", 0x0081CA50, &ni::anim::Basis_GetRTTI);
NIC32_BSPLINE_REVERSED("NiBSplineBasisData::CreateClone", 0x0081CA60, &ni::anim::Basis_CreateClone);
NIC32_BSPLINE_REVERSED("NiBSplineBasisData::CreateObject", 0x0081CAC0, &ni::anim::Basis_CreateObject);
NIC32_BSPLINE_REVERSED("NiBSplineBasisData::IsEqual", 0x0081CB10, &ni::anim::Basis_IsEqual);
NIC32_BSPLINE_REVERSED("NiBSplineBasisData::DeletingDestructor", 0x0081CB40,
                       &ni::anim::Basis_DeletingDestructor);
NIC32_BSPLINE_REVERSED("NiBSplineBasisData::LoadBinary", 0x0081CB70, &ni::anim::Basis_LoadBinary);
NIC32_BSPLINE_REVERSED("NiBSplineBasisData::SaveBinary", 0x0081CBB0, &ni::anim::Basis_SaveBinary);
NIC32_BSPLINE_REVERSED("NiBSplineBasisData::GetViewerStrings", 0x0081CBF0, &ni::anim::Basis_GetViewerStrings);
