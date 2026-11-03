#pragma once

// NiPointLight (vtable 0x952FF0, 0x130 bytes) and NiSpotLight (vtable 0x952F20, 0x150 bytes), from
// their constructors (0x7751A0, 0x774D60). They build on NiLight (0x124 bytes) and NiDynamicEffect,
// whose code is another chunk's: those bytes are kept opaque here (the fields this code touches
// are named in comments).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"

namespace ni {

struct NiPointLight : NiAVObject {
    // NiDynamicEffect / NiLight: +0xC5 the effect type (2 point, 4 spot), +0xD0 the revision ID
    // (bumped when the light changes), +0xF8 the dimmer and the ambient, diffuse, specular colors
    uint8_t lightData[0x124 - 0xC4];   // +0xC4
    float atten0;                      // +0x124 (constant: 0)
    float atten1;                      // +0x128 (linear: 1)
    float atten2;                      // +0x12C (quadratic: 0)
};
static_assert(offsetof(NiPointLight, atten0) == 0x124);
static_assert(sizeof(NiPointLight) == 0x130);

struct NiSpotLight : NiPointLight {
    NiPoint3 worldDir;        // +0x130 (the world rotation's first column: 1, 0, 0)
    float outerSpotAngle;     // +0x13C (degrees: 0)
    float innerSpotAngle;     // +0x140 (0)
    float spotExponent;       // +0x144 (1)
    float cosOuterSpotAngle;  // +0x148 (1)
    float cosInnerSpotAngle;  // +0x14C (1)
};
static_assert(offsetof(NiSpotLight, worldDir) == 0x130);
static_assert(offsetof(NiSpotLight, spotExponent) == 0x144);
static_assert(sizeof(NiSpotLight) == 0x150);

} // namespace ni
