#pragma once

// The small NiProperty kinds whose code is in this part of the library: NiMaterialProperty (vtable
// 0x9530B0, 0x5C bytes), NiFogProperty (vtable 0x9179FC, 0x2C), NiStencilProperty (vtable 0x917AAC,
// 0x24), NiVertexColorProperty (vtable 0x92D0D4, 0x1C), NiShadeProperty (vtable 0x917A54, 0x1C).
// Layouts from their constructors and CreateClones (0x775660, 0x778190, 0x778850, 0x7794E0,
// 0x778650). NiProperty adds nothing to NiObjectNET. The colors are NiColors (r, g, b: NiPoint3's
// layout, and NiPoint3's binary streaming).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni {

struct NiMaterialProperty : NiObjectNET {
    int32_t index;         // +0x18 (from ms_iNextIndex, 0xBC8A70, interlocked)
    NiPoint3 ambient;      // +0x1C (0.5)
    NiPoint3 diffuse;      // +0x28 (0.5)
    NiPoint3 specular;     // +0x34 (0)
    NiPoint3 emissive;     // +0x40 (0)
    float shine;           // +0x4C (4)
    float alpha;           // +0x50 (1)
    uint32_t revisionID;   // +0x54 (1)
    re::Ptr32<void> rendererData;    // +0x58
};
static_assert(offsetof(NiMaterialProperty, index) == 0x18);
static_assert(offsetof(NiMaterialProperty, shine) == 0x4C);
static_assert(offsetof(NiMaterialProperty, rendererData) == 0x58);
static_assert(sizeof(NiMaterialProperty) == 0x5C);

struct NiFogProperty : NiObjectNET {
    uint16_t flags;        // +0x18 (bit 0 fog on; bits 1..2 the function: Z linear, range
                           //  squared, vertex alpha)
    uint16_t pad1A;
    float depth;           // +0x1C (1)
    NiPoint3 color;        // +0x20 (NiColor::BLACK, 0x20E25E8)
};
static_assert(offsetof(NiFogProperty, depth) == 0x1C);
static_assert(sizeof(NiFogProperty) == 0x2C);

struct NiStencilProperty : NiObjectNET {
    uint16_t flags;        // +0x18 (bit 0 enabled; bits 1..3 the fail action, 4..6 the depth
                           //  fail action, 7..9 the pass action, 10..11 the draw mode, 12..15
                           //  the test function; 0x4180 by default)
    uint16_t pad1A;
    uint32_t reference;    // +0x1C (0)
    uint32_t mask;         // +0x20 (0xFFFFFFFF)
};
static_assert(offsetof(NiStencilProperty, reference) == 0x1C);
static_assert(sizeof(NiStencilProperty) == 0x24);

// NiVertexColorProperty and NiShadeProperty: one 16-bit flags word
struct NiFlagsProperty : NiObjectNET {
    uint16_t flags;        // +0x18 (vertex colors: bit 3 the lighting mode, bits 4..5 the source
                           //  mode, 8 by default; shading: bit 0 smooth, 1 by default)
    uint16_t pad1A;
};
static_assert(sizeof(NiFlagsProperty) == 0x1C);

} // namespace ni
