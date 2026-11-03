#pragma once

// NiAlphaProperty (vtable 0x8FFB9C, 0x1C bytes) and NiZBufferProperty (vtable 0x8FFCA4, 0x30
// bytes), from their CreateClones (0x76CE60, 0x76D5E0: the constructors inlined). NiProperty
// adds nothing to NiObjectNET.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"

namespace ni {

struct NiAlphaProperty : NiObjectNET {
    uint16_t flags;        // +0x18 (bit 0 blending; bits 1..4 the source blend, 5..8 the
                           //  destination blend (ALPHA_ONE .. ALPHA_SRCALPHASAT); 9 the alpha
                           //  test; 10..12 its function (TEST_ALWAYS .. TEST_NEVER); 13 no
                           //  sorter; 0xEC by default: SRCALPHA / INVSRCALPHA)
    uint8_t testRef;       // +0x1A
    uint8_t pad1B;
};
static_assert(offsetof(NiAlphaProperty, flags) == 0x18);
static_assert(offsetof(NiAlphaProperty, testRef) == 0x1A);
static_assert(sizeof(NiAlphaProperty) == 0x1C);

struct NiZBufferProperty : NiObjectNET {
    uint16_t flags;        // +0x18 (bit 0 the z test, 1 the z write, 2..5 the test function;
                           //  0xF by default: both, TEST_LESSEQUAL; bits 6..7 Bully's)
    uint16_t pad1A;
    float unknown1C;       // +0x1C (0: Bully's, compared by IsEqual)
    float unknown20;       // +0x20 (0: Bully's, compared)
    uint32_t unknown24;    // +0x24 (0: Bully's, not compared)
    float unknown28;       // +0x28 (1: Bully's, compared)
    float unknown2C;       // +0x2C (1: Bully's, compared)
};
static_assert(offsetof(NiZBufferProperty, unknown1C) == 0x1C);
static_assert(offsetof(NiZBufferProperty, unknown2C) == 0x2C);
static_assert(sizeof(NiZBufferProperty) == 0x30);

} // namespace ni
