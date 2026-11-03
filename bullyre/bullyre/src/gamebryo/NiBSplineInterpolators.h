#pragma once

// The B-spline interpolators of a transform, a point and a float, and their compressed kinds
// (control points as shorts over an offset and a half range per channel), on what
// src/gamebryo/NiBSplineData.h has (NiBSplineInterpolator, the data and basis). Layouts from the
// constructors (0x819EF0, 0x81A5B0, 0x81A8E0) and CreateObjects; everything in ni::anim.
//
// NiBSplineInterpolator's slots past NiInterpolator's: 37 GetChannelCount, 38
// GetControlPointCount(channel), 39 GetDimension(channel), 40 GetDegree(channel), 41
// GetChannelPosed(channel), 42 UsesCompressedControlPoints, 44 GetControlHandle(channel), 45
// SetControlHandle(handle, channel).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiBSplineData.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiInterpolators.h"

namespace ni::anim {

// NiBSplineTransformInterpolator (vtable 0x966FC8, 0x48 bytes): channels 0 translation (3
// floats), 1 rotation (4), 2 scale (1)
struct NiBSplineTransformInterpolator : NiBSplineInterpolator {
    NiQuatTransform value;      // +0x1C (the invalid transform at first)
    uint32_t transHandle;       // +0x3C
    uint32_t rotHandle;         // +0x40
    uint32_t scaleHandle;       // +0x44
};
static_assert(offsetof(NiBSplineTransformInterpolator, transHandle) == 0x3C);
static_assert(sizeof(NiBSplineTransformInterpolator) == 0x48);

// NiBSplineCompTransformInterpolator (vtable 0x967258, 0x60 bytes): each channel's offset and half
// range (FLT_MAX at first)
struct NiBSplineCompTransformInterpolator : NiBSplineTransformInterpolator {
    float compScalars[6];       // +0x48 (translation, rotation, scale: offset, half range)
};
static_assert(sizeof(NiBSplineCompTransformInterpolator) == 0x60);

// NiBSplinePoint3Interpolator (vtable 0x9670B8, 0x2C bytes)
struct NiBSplinePoint3Interpolator : NiBSplineInterpolator {
    NiPoint3 value;             // +0x1C (not set by the constructors)
    uint32_t handle;            // +0x28
};
static_assert(sizeof(NiBSplinePoint3Interpolator) == 0x2C);

// NiBSplineCompPoint3Interpolator (vtable 0x967318, 0x34 bytes)
struct NiBSplineCompPoint3Interpolator : NiBSplinePoint3Interpolator {
    float compScalars[2];       // +0x2C (offset, half range)
};
static_assert(sizeof(NiBSplineCompPoint3Interpolator) == 0x34);

// NiBSplineFloatInterpolator (vtable 0x967188, 0x24 bytes)
struct NiBSplineFloatInterpolator : NiBSplineInterpolator {
    float value;                // +0x1C (not set by the constructors)
    uint32_t handle;            // +0x20
};
static_assert(sizeof(NiBSplineFloatInterpolator) == 0x24);

// NiBSplineCompFloatInterpolator (vtable 0x9673D8, 0x2C bytes)
struct NiBSplineCompFloatInterpolator : NiBSplineFloatInterpolator {
    float compScalars[2];       // +0x24 (offset, half range)
};
static_assert(sizeof(NiBSplineCompFloatInterpolator) == 0x2C);

}   // namespace ni::anim
