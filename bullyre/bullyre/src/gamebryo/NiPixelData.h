#pragma once

// NiPixelFormat (0x44 bytes, a value type) and NiPixelData (vtable 0x9525D0, 0x70 bytes: the
// pixels of every mipmap level of every face, in one block), from their constructors at
// 0x76B280 / 0x76AFD0 and 0x76A610.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni {

// NiPixelFormat::NiComponentSpec: the channel (0x13 COMP_EMPTY) and how it's stored (5
// REP_UNKNOWN)
struct NiPixelComponent {
    uint32_t component;        // +0x0
    uint32_t representation;   // +0x4
    uint8_t bitsPerComponent;  // +0x8
    uint8_t isSigned;          // +0x9
    uint8_t padA[2];
};
static_assert(sizeof(NiPixelComponent) == 0xC);

struct NiPixelFormat {
    uint8_t flags;             // +0x0 (bit 0 little endian)
    uint8_t bitsPerPixel;      // +0x1
    uint8_t srgb;              // +0x2
    uint8_t pad3;
    int32_t format;            // +0x4 (0 RGB, 1 RGBA, 2 PAL, 3 PALALPHA, 4..6 DXT1/3/5, 8 BUMP,
                               //  9 BUMPLUMA; 0x10 unknown)
    uint32_t tiling;           // +0x8
    uint32_t rendererHint;     // +0xC (0xFFFFFFFF: any, matches every hint)
    uint32_t extraData;        // +0x10
    NiPixelComponent components[4];   // +0x14
};
static_assert(offsetof(NiPixelFormat, components) == 0x14);
static_assert(sizeof(NiPixelFormat) == 0x44);

struct NiPixelData : NiObject {
    NiPixelFormat format;      // +0x8
    re::Ptr32<NiObject> palette;         // +0x4C (NiPalettePtr)
    re::Ptr32<uint8_t> pixels;           // +0x50 (offsets[levels] bytes per face)
    re::Ptr32<uint32_t> widths;          // +0x54 (per level)
    re::Ptr32<uint32_t> heights;         // +0x58
    re::Ptr32<uint32_t> offsets;         // +0x5C (levels + 1: the last is a face's size)
    uint32_t mipmapLevels;     // +0x60
    uint32_t pixelStride;      // +0x64 (bytes per pixel; 0 for block-compressed and sub-byte)
    uint32_t revisionID;       // +0x68
    uint32_t faces;            // +0x6C
};
static_assert(offsetof(NiPixelData, format) == 0x8);
static_assert(offsetof(NiPixelData, palette) == 0x4C);
static_assert(offsetof(NiPixelData, mipmapLevels) == 0x60);
static_assert(offsetof(NiPixelData, faces) == 0x6C);
static_assert(sizeof(NiPixelData) == 0x70);

} // namespace ni
