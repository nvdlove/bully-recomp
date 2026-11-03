#pragma once

// NiTexture (vtable 0x9519B8, 0x30 bytes) and NiRenderedTexture (vtable 0x951A08, 0x34 bytes),
// from the constructor at 0x75D8E0 (NiRenderedTexture's, with NiTexture's inlined). Every
// texture is on a global list (ms_pkHead 0x20E2178 .. ms_pkTail 0x20E217C, under the critical
// section at 0x20E21B8) while it lives.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni {

struct NiTexture : NiObjectNET {
    // FormatPrefs
    uint32_t pixelLayout;    // +0x18 (6 by default)
    uint32_t alphaFormat;    // +0x1C (3)
    uint32_t mipMapped;      // +0x20 (2)
    re::Ptr32<void> rendererData;      // +0x24 (deleted through its slot 0)
    re::Ptr32<NiTexture> prev;         // +0x28 (the global list)
    re::Ptr32<NiTexture> next;         // +0x2C
};
static_assert(offsetof(NiTexture, pixelLayout) == 0x18);
static_assert(offsetof(NiTexture, rendererData) == 0x24);
static_assert(offsetof(NiTexture, prev) == 0x28);
static_assert(sizeof(NiTexture) == 0x30);

struct NiRenderedTexture : NiTexture {
    re::Ptr32<NiObject> buffer;        // +0x30 (Ni2DBufferPtr: width +0x8, height +0xC)
};
static_assert(sizeof(NiRenderedTexture) == 0x34);

} // namespace ni
