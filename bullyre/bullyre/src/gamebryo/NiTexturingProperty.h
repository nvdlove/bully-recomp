#pragma once

// NiTexturingProperty (vtable 0x952098, 0x34 bytes; NiProperty type 8) and its maps, from the
// constructors at 0x765DD0 / 0x765F30 and the map constructors in its LoadBinary.
//
// The maps array: 0 base, 1 dark, 2 detail, 3 gloss, 4 glow, 5 bump (a BumpMap), 6 normal,
// 7 parallax (a ParallaxMap), 8.. decals. The shader maps in an array of their own.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni {

// NiTexturingProperty::Map (vtable 0x9520E8, 0x10 bytes): slot 0 destructor, 1 LoadBinary,
// 2 SaveBinary, 3 GetClassType (0 map, 1 bump, 2 parallax)
struct NiTexturingMap {
    re::Ptr32<void> vtable;
    uint16_t flags;          // +0x4 (texture coordinate set: bits 0..7; filter mode 8..11; clamp
                             //  mode 12..13; 0x3100 by default: wrap/wrap, bilinear)
    uint16_t pad6;
    re::Ptr32<NiObject> texture;       // +0x8 (NiTexturePtr)
    re::Ptr32<void> transform;         // +0xC (NiTextureTransform, 0x48 bytes, owned)
};
static_assert(sizeof(NiTexturingMap) == 0x10);

// BumpMap (vtable 0x9520F8, 0x28 bytes)
struct NiTexturingBumpMap : NiTexturingMap {
    float lumaScale;         // +0x10 (1)
    float lumaOffset;        // +0x14 (0)
    float bumpMat[4];        // +0x18 (00, 01, 10, 11: 0.5, 0, 0, 0.5)
};
static_assert(sizeof(NiTexturingBumpMap) == 0x28);

// ParallaxMap (vtable 0x952108, 0x14 bytes) and ShaderMap (vtable 0x8FFA78, 0x14 bytes): one
// more 32-bit value (the offset, a float: 0.5; the shader map's ID)
struct NiTexturingMap14 : NiTexturingMap {
    uint32_t value;          // +0x10
};
static_assert(sizeof(NiTexturingMap14) == 0x14);

struct NiTexturingProperty : NiObjectNET {
    uint16_t flags;                        // +0x18 (1 multi-texture; apply mode bits 1..3;
                                           //  decal count bits 4..11)
    uint16_t pad1A;
    NiTArray<NiTexturingMap*> maps;        // +0x1C (NiTPrimitiveArray, vtable 0x961988)
    uint32_t unknown2C;                    // +0x2C
    re::Ptr32<NiTArray<NiTexturingMap*>> shaderMaps; // +0x30 (created when first needed)
};
static_assert(offsetof(NiTexturingProperty, flags) == 0x18);
static_assert(offsetof(NiTexturingProperty, maps) == 0x1C);
static_assert(offsetof(NiTexturingProperty, shaderMaps) == 0x30);
static_assert(sizeof(NiTexturingProperty) == 0x34);

} // namespace ni
