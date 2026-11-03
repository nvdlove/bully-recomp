#pragma once

// NiSourceTexture (vtable 0x952560, 0x44 bytes): a texture from an image file or from pixel
// data held by the object, from its constructor at 0x7697A0. Slots past NiTexture's: 18
// GetWidth, 19 GetHeight, 20 LoadPixelDataFromFile, 21 DestroyAppPixelData, 22
// CreateRendererData. (NiSourceCubeMap, vtable 0x961064, builds on it.)

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiTexture.h"
#include "reversed.h"

namespace ni {

struct NiSourceTexture : NiTexture {
    re::Ptr32<const char> filename;             // +0x30
    re::Ptr32<const char> platformFilename;     // +0x34
    re::Ptr32<NiObject> persistentSrcRendererData;  // +0x38 (NiPersistentSrcTextureRendererDataPtr: its
                                        //  widths +0x5C, heights +0x60)
    re::Ptr32<NiObject> srcPixelData;             // +0x3C (NiPixelDataPtr: its widths +0x54, heights +0x58)
    uint8_t isStatic;                   // +0x40 (1)
    uint8_t loadDirectToRendererHint;   // +0x41
    uint8_t srcRendererDataIsPersistent;  // +0x42
    uint8_t pad43;
};
static_assert(offsetof(NiSourceTexture, filename) == 0x30);
static_assert(offsetof(NiSourceTexture, srcPixelData) == 0x3C);
static_assert(offsetof(NiSourceTexture, isStatic) == 0x40);
static_assert(sizeof(NiSourceTexture) == 0x44);

} // namespace ni
