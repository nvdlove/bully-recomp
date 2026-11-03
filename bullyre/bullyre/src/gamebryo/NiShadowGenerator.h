#pragma once

// NiShadowGenerator (vtable 0x952E20, 0x68 bytes): what casts an NiDynamicEffect's shadows:
// its technique, the nodes excluded as casters / receivers, the callbacks that hand it shadow
// maps, from its constructor at 0x773110.
//
// The shadow maps (NiShadowMap: flags +0x4, the low nibble its kind (0 a map, 1 a cube map),
// texture +0x8, render click +0xC; NiShadowCubeMap: the six faces' clicks +0x10). A click's
// +0x80 is its generator, +0x84 bit 0x10 its "needs rendering".

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni {

struct NiDynamicEffect;
struct NiShadowGenerator;

// the callbacks (cdecl)
using ShadowMapAssignCallback = uint8_t(__cdecl*)(NiShadowGenerator* generator, void* geometry, void* map,
                                                  uint32_t index);
using ShadowMapRetrieveCallback = void*(__cdecl*)(NiShadowGenerator* generator, uint32_t index);
using ShadowMapCountCallback = uint32_t(__cdecl*)(NiShadowGenerator* generator);
using ShadowCleanupCallback = void(__cdecl*)(NiShadowGenerator* generator);

struct NiShadowGenerator : NiObject {
    uint16_t flags;                              // +0x8 (bit 0 the maps dirty, 1 maps released, 2
                                                 //  static, 3 active; 0x3D0 set by the constructor;
                                                 //  bits 10.. left as the memory had them)
    uint16_t padA;
    re::Ptr32<void> technique;                             // +0xC (NiShadowTechnique: its name +0x4)
    NiTPointerList<NiNode*> unaffectedCasters;   // +0x10 (vtable 0x95145C)
    NiTPointerList<NiNode*> unaffectedReceivers; // +0x20
    re::Ptr32<void> callback30;                            // +0x30
    re::Ptr32<uint8_t __cdecl(NiShadowGenerator* generator, void* geometry, void* map, uint32_t index)> assignShadowMap;     // +0x34
    re::Ptr32<void* __cdecl(NiShadowGenerator* generator, uint32_t index)> retrieveShadowMap; // +0x38
    re::Ptr32<uint32_t __cdecl(NiShadowGenerator* generator)> shadowMapCount;       // +0x3C
    re::Ptr32<void> callback40;                            // +0x40
    re::Ptr32<void> callback44;                            // +0x44
    re::Ptr32<void __cdecl(NiShadowGenerator* generator)> cleanup;               // +0x48
    re::Ptr32<NiDynamicEffect> dynamicEffect;              // +0x4C
    re::Ptr32<void> extraData;                             // +0x50
    float depthBias;                             // +0x54
    float unknown58;                             // +0x58 (streamed from 20.3.0.7)
    float unknown5C;                             // +0x5C
    float unknown60;                             // +0x60
    uint16_t sizeHint;                           // +0x64 (0x400)
    uint16_t pad66;
};
static_assert(offsetof(NiShadowGenerator, technique) == 0xC);
static_assert(offsetof(NiShadowGenerator, unaffectedReceivers) == 0x20);
static_assert(offsetof(NiShadowGenerator, assignShadowMap) == 0x34);
static_assert(offsetof(NiShadowGenerator, cleanup) == 0x48);
static_assert(offsetof(NiShadowGenerator, dynamicEffect) == 0x4C);
static_assert(offsetof(NiShadowGenerator, sizeHint) == 0x64);
static_assert(sizeof(NiShadowGenerator) == 0x68);

} // namespace ni
