#pragma once

// NiDynamicEffect (vtable 0x952D50, 0xF8 bytes): an NiAVObject that lights or projects onto the
// nodes it affects, with its shadow generator; and NiLight (vtable 0x952E68, 0x124 bytes). From
// their constructors at 0x772180 and 0x773900. (NiPointLight / NiSpotLight: NiPointLight.h.)
//
// The node lists are NiTPointerList<NiNode*> (vtable 0x95145C: slot 1 a new node, 2 a node
// freed): the affected nodes (each also holds the effect: NiNode::AttachEffect) and the
// unaffected ones.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni {

struct NiShadowGenerator;

struct NiDynamicEffect : NiAVObject {
    uint8_t on;                                  // +0xC4 (1)
    uint8_t effectType;                          // +0xC5 (0xFF; a light's type: 0 ambient, 1
                                                 //  directional, 2 point, 4 spot; +3 while it has
                                                 //  a shadow generator)
    uint8_t padC6[2];
    int32_t index;                               // +0xC8 (from ms_iNextIndex, 0xBC8A6C, interlocked)
    uint32_t pushCount;                          // +0xCC (0)
    uint32_t revisionID;                         // +0xD0 (1)
    re::Ptr32<NiShadowGenerator> shadowGenerator;          // +0xD4 (not a reference)
    NiTPointerList<NiNode*> affectedNodes;       // +0xD8
    NiTPointerList<NiNode*> unaffectedNodes;     // +0xE8
};
static_assert(offsetof(NiDynamicEffect, on) == 0xC4);
static_assert(offsetof(NiDynamicEffect, index) == 0xC8);
static_assert(offsetof(NiDynamicEffect, shadowGenerator) == 0xD4);
static_assert(offsetof(NiDynamicEffect, affectedNodes) == 0xD8);
static_assert(offsetof(NiDynamicEffect, unaffectedNodes) == 0xE8);
static_assert(sizeof(NiDynamicEffect) == 0xF8);

struct NiLight : NiDynamicEffect {
    float dimmer;            // +0xF8 (1)
    NiPoint3 ambient;        // +0xFC (NiColor: 1, 1, 1)
    NiPoint3 diffuse;        // +0x108 (1, 1, 1)
    NiPoint3 specular;       // +0x114 (1, 1, 1)
    re::Ptr32<void> rendererData;      // +0x120
};
static_assert(offsetof(NiLight, dimmer) == 0xF8);
static_assert(offsetof(NiLight, specular) == 0x114);
static_assert(sizeof(NiLight) == 0x124);

} // namespace ni
