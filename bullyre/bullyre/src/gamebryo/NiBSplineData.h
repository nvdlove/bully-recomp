#pragma once

// Gamebryo's B-spline animation data (NiAnimation): the control points (NiBSplineData, vtable
// 0x967620, 0x18 bytes), the uniform cubic basis they're evaluated with (NiBSplineBasisData,
// vtable 0x9676A0, 0x28 bytes, holding a NiBSplineBasis<float, 3>), and what the B-spline
// interpolators share (NiBSplineInterpolator, vtable 0x9686B0, 0x1C bytes) with the colour one
// (NiBSplineColorInterpolator, vtable 0x967550, 0x34 bytes) and its compressed kind
// (NiBSplineCompColorInterpolator, vtable 0x967498, 0x3C bytes). Layouts from the constructors
// (0x828560, 0x81BA00, 0x81BD40, 0x81CAC0); everything in ni::anim.
//
// NiBSplineInterpolator's slots past NiInterpolator's: 37 GetChannelCount, 38
// GetControlPointCount(channel) (0x828440), 39 GetDimension(channel), 40 GetDegree(channel),
// 41 GetChannelPosed(channel) (no control points), 42 UsesCompressedControlPoints, 43 the
// channel's control point storage (0x8283E0), 44 GetControlHandle(channel), 45
// SetControlHandle(handle, channel).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiAnimationKey.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiInterpolators.h"
#include "reversed.h"

namespace ni::anim {

// NiBSplineBasis<float, 3>: the count of control points, the four basis values of the last time
// evaluated, that time and the control point range it covers
struct NiBSplineBasis3 {
    int32_t numControlPoints;   // +0x0
    float values[4];            // +0x4
    float lastTime;             // +0x14 (-1 at first)
    int32_t lastMin;            // +0x18
    int32_t lastMax;            // +0x1C (3 at first)
};
static_assert(sizeof(NiBSplineBasis3) == 0x20);

struct NiBSplineBasisData : NiObject {
    NiBSplineBasis3 basis;   // +0x8
};
static_assert(sizeof(NiBSplineBasisData) == 0x28);

// a channel's control points start at its handle (an index into the floats, or the shorts);
// 0xFFFF: none
constexpr uint32_t kInvalidBSplineHandle = 0xFFFF;

struct NiBSplineData : NiObject {
    re::Ptr32<float> controlPoints;           // +0x8 (NiMalloc'd)
    re::Ptr32<int16_t> compactControlPoints;  // +0xC (NiMalloc'd: -32767..32767 over an offset and a half range)
    uint32_t controlPointCount;     // +0x10 (floats)
    uint32_t compactCount;          // +0x14 (shorts)
};
static_assert(sizeof(NiBSplineData) == 0x18);

struct NiBSplineInterpolator : NiInterpolator {
    float startTime;                  // +0xC (FLT_MAX at first)
    float endTime;                    // +0x10 (-FLT_MAX)
    re::Ptr32<NiBSplineData> data;              // +0x14 (a reference)
    re::Ptr32<NiBSplineBasisData> basisData;    // +0x18 (a reference)
};
static_assert(sizeof(NiBSplineInterpolator) == 0x1C);

struct NiBSplineColorInterpolator : NiBSplineInterpolator {
    NiColorA value;       // +0x1C (0 at first)
    uint32_t handle;      // +0x2C (the colour channel's control points)
    uint32_t unknown30;   // +0x30 (allocated, never set)
};
static_assert(offsetof(NiBSplineColorInterpolator, handle) == 0x2C);
static_assert(sizeof(NiBSplineColorInterpolator) == 0x34);

struct NiBSplineCompColorInterpolator : NiBSplineColorInterpolator {
    float compScalars[2];   // +0x34 (the offset and the half range)
};
static_assert(sizeof(NiBSplineCompColorInterpolator) == 0x3C);

}   // namespace ni::anim
