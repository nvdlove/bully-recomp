#pragma once

// Gamebryo's blend interpolators (NiAnimation): NiBlendInterpolator (vtable 0x9683D8, 0x30 bytes,
// constructors 0x827320 / 0x826400) blends the results of an array of weighted, prioritised
// interpolators (a controller sequence's each); its kinds keep the last blended value:
// NiBlendTransformInterpolator (vtable 0x967940, 0x30), NiBlendQuaternionInterpolator (0x9679E8,
// 0x40), NiBlendPoint3Interpolator (0x967A90, 0x40), NiBlendFloatInterpolator (0x967B38, 0x34),
// NiBlendColorInterpolator (0x967BE0, 0x44), NiBlendBoolInterpolator (0x967C88, 0x34) and
// NiBlendAccumTransformInterpolator (0x966798, 0x58: the transform accumulated over the loops of
// the animation, for root motion). Everything in ni::anim.
//
// NiBlendInterpolator's slots past NiInterpolator's: 37 AddInterpInfo(interp, weight, priority,
// ease spinner); 38 RemoveInterpInfo(index) (a NiInterpolatorPtr returned); 39
// InterpolatorIsCorrectType(interp); 40 ReallocateArrays; (the accumulating one's 41
// GetAccumulatedTransform(transform&)).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiAnimationKey.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiInterpolators.h"
#include "reversed.h"

namespace ni::anim {

// one blended interpolator
struct InterpArrayItem {
    re::Ptr32<NiInterpolator> interpolator;   // +0x0 (a reference)
    float weight;                   // +0x4
    float normalizedWeight;         // +0x8 (ComputeNormalizedWeights')
    int8_t priority;                // +0xC
    uint8_t padD[3];
    float easeSpinner;              // +0x10
    float updateTime;               // +0x14 (the time a manager sets; -FLT_MAX: none)
};
static_assert(sizeof(InterpArrayItem) == 0x18);

struct NiBlendInterpolator : NiInterpolator {
    uint8_t flags;                    // +0xC (1 manager controlled, 2 only the highest weight,
                                      //  4 the weights to normalize)
    uint8_t arraySize;                // +0xD
    uint8_t interpCount;              // +0xE
    uint8_t singleIdx;                // +0xF (the only one's index when there's one; 0xFF none)
    int8_t highPriority;              // +0x10 (-128)
    int8_t nextHighPriority;          // +0x11
    uint8_t pad12[2];
    re::Ptr32<InterpArrayItem> interpArray;     // +0x14 (NiNew[]: the count before it)
    re::Ptr32<NiInterpolator> singleInterpolator;   // +0x18 (not a reference)
    float weightThreshold;            // +0x1C
    float singleTime;                 // +0x20 (the only one's update time)
    float highSumOfWeights;           // +0x24
    float nextHighSumOfWeights;       // +0x28
    float highEaseSpinner;            // +0x2C
};
static_assert(offsetof(NiBlendInterpolator, interpArray) == 0x14);
static_assert(offsetof(NiBlendInterpolator, singleTime) == 0x20);
static_assert(sizeof(NiBlendInterpolator) == 0x30);

using NiBlendTransformInterpolator = NiBlendInterpolator;

struct NiBlendFloatInterpolator : NiBlendInterpolator {
    float value;   // +0x30 (-FLT_MAX: invalid)
};
static_assert(sizeof(NiBlendFloatInterpolator) == 0x34);

struct NiBlendBoolInterpolator : NiBlendInterpolator {
    uint8_t value;   // +0x30 (2: invalid)
    uint8_t pad31[3];
};
static_assert(sizeof(NiBlendBoolInterpolator) == 0x34);

struct NiBlendPoint3Interpolator : NiBlendInterpolator {
    NiPoint3 value;      // +0x30
    uint8_t validValue;  // +0x3C
    uint8_t pad3D[3];
};
static_assert(sizeof(NiBlendPoint3Interpolator) == 0x40);

struct NiBlendQuaternionInterpolator : NiBlendInterpolator {
    NiQuaternion value;   // +0x30
};
static_assert(sizeof(NiBlendQuaternionInterpolator) == 0x40);

struct NiBlendColorInterpolator : NiBlendInterpolator {
    NiColorA value;      // +0x30
    uint8_t validValue;  // +0x40
    uint8_t pad41[3];
};
static_assert(sizeof(NiBlendColorInterpolator) == 0x44);

struct NiBlendAccumTransformInterpolator : NiBlendInterpolator {
    NiQuatTransform value;        // +0x30 (the accumulated transform)
    re::Ptr32<AccumArrayItem> accumArray;   // +0x50 (one per array slot: NiAllocArray'd, no count before it)
    uint8_t reset;                // +0x54 (the accumulation to start again)
    uint8_t pad55[3];
};
static_assert(offsetof(NiBlendAccumTransformInterpolator, accumArray) == 0x50);
static_assert(sizeof(NiBlendAccumTransformInterpolator) == 0x58);

}   // namespace ni::anim
