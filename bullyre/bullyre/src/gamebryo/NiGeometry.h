#pragma once

// NiGeometry (vtable 0x951CC8): an NiAVObject drawn from geometry data, with the property and
// effect states pushed down to it, a skin instance, and its materials (each an NiMaterial with
// the shader it last gave). From the constructors at 0x761CA0 and 0x7624E0.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni {

// NiMaterialInstance (0x10 bytes)
struct NiMaterialInstance {
    re::Ptr32<NiObject> cachedShader;   // +0x0 (a reference)
    re::Ptr32<NiObject> material;       // +0x4 (NiMaterialPtr)
    uint32_t extraData;       // +0x8
    uint32_t needsUpdate;     // +0xC (0, 1; 2: when the geometry's default material doesn't)
};
static_assert(sizeof(NiMaterialInstance) == 0x10);

struct NiGeometry : NiAVObject {
    re::Ptr32<NiObject> propertyState;          // +0xC4 (NiPropertyStatePtr)
    re::Ptr32<NiObject> effectState;            // +0xC8 (NiDynamicEffectStatePtr)
    re::Ptr32<NiObject> modelData;              // +0xCC (NiGeometryDataPtr: its bound +0x10, vertex count
                                      //  +0x30 (16 bits), vertices +0x20, normals +0x24, flags
                                      //  +0x34: the consistency in bits 12..15)
    re::Ptr32<NiObject> skinInstance;           // +0xD0 (NiSkinInstancePtr: its partition +0xC)
    re::Ptr32<NiMaterialInstance> materials;    // +0xD4 (an array new[]'d with its count before it)
    uint32_t materialsAllocated;      // +0xD8
    uint32_t materialCount;           // +0xDC
    uint32_t activeMaterial;          // +0xE0 (-1: none)
    re::Ptr32<NiObject> unknownE4;              // +0xE4 (a reference the destructor lets go)
    uint8_t defaultMaterialNeedsUpdate;   // +0xE8 (1 from the constructor)
    uint8_t unknownE9[3];
};
static_assert(offsetof(NiGeometry, propertyState) == 0xC4);
static_assert(offsetof(NiGeometry, modelData) == 0xCC);
static_assert(offsetof(NiGeometry, materials) == 0xD4);
static_assert(offsetof(NiGeometry, activeMaterial) == 0xE0);
static_assert(offsetof(NiGeometry, defaultMaterialNeedsUpdate) == 0xE8);
static_assert(sizeof(NiGeometry) == 0xEC);

} // namespace ni
