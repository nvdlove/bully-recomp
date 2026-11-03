#pragma once

// Gamebryo's bool animation (NiAnimation): NiBoolData (vtable 0x967868, 0x18 bytes: one channel of
// bool keys), NiBoolInterpolator (vtable 0x967798, 0x18 bytes: a key-based interpolator giving a
// bool, posed without data) and NiBoolTimelineInterpolator (vtable 0x9676E8, 0x20 bytes: the same,
// but a value that flipped and flipped back between two updates still shows the flip). Layouts
// from the constructors (0x81CF70, 0x81CED0, 0x81D6C0); everything in ni::anim.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiAnimationKey.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiInterpolators.h"
#include "reversed.h"

namespace ni::anim {

// the bool interpolators' invalid value (2: a static the code compares with)
GAME_VAR(uint8_t, kInvalidBool, 0x00968240);

struct NiBoolData : NiObject {
    KeyChannel bools;   // +0x8 (NiBoolKeys)
};
static_assert(sizeof(NiBoolData) == 0x18);

struct NiBoolInterpolator : NiKeyBasedInterpolator {
    uint8_t value;      // +0xC (the last result or the pose; kInvalidBool when none)
    uint8_t padD[3];
    re::Ptr32<NiBoolData> data;   // +0x10 (a reference)
    uint32_t lastIdx;   // +0x14
};
static_assert(offsetof(NiBoolInterpolator, data) == 0x10);
static_assert(sizeof(NiBoolInterpolator) == 0x18);

struct NiBoolTimelineInterpolator : NiBoolInterpolator {
    uint32_t previousIdx;     // +0x18 (the key index before the last update)
    uint8_t previousValue;    // +0x1C (the keys' value at the last update)
    uint8_t pad1D[3];
};
static_assert(sizeof(NiBoolTimelineInterpolator) == 0x20);

}   // namespace ni::anim
