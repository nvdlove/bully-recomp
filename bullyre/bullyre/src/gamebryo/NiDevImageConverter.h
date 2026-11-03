#pragma once

// NiDevImageConverter as Bully's exe has it (vtable 0x9A18C0, 0x658; the image converter the
// game installs, 0x82B4A1): NiImageConverter (src/gamebryo/NiImageConverter.cpp, 0x438) with a
// DXT decompressor and the list of image readers (src/gamebryo/NiImageReaders.h) it reads files
// with. Layout from the constructor 0x8CA1D0.
//
// Its slots past NiImageConverter's (2 ReadImageFileInfo, 3 ReadImageFile, 4
// CanConvertPixelData, 5 ConvertPixelData): 6 the file name for the platform, 7, 8/9 whether
// a format reads as RGBA32 / RGB24, 10 GenerateMipmaps, 11 ReadMipmapFiles, 12 ConvertFormat
// (one level or all), 13 ConvertToBump, 14 AddReader, 15 GetPlatformFormat, 16
// ConvertForPlatform, 17, 18..20 the platform formats (platforms 6, 4 and 5).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni {

struct NiImageReader;

// the DXT decompressor (0x210, constructed by 0x8CCFB0): the alpha and colour block decoders
// for the format (thiscall(out): a 4x4 block's alpha bytes, every 4th, or its RGB(A)), the
// block being read, the destination's row bytes, and each byte's two nibbles as 8 bits
struct DXTDecoder {
    re::Ptr32<void> alphaFn;                 // +0x0 (none for DXT1)
    re::Ptr32<void> colorFn;                 // +0x4
    re::Ptr32<const uint8_t> source;         // +0x8
    uint32_t pitch;                // +0xC
    uint8_t nibbles[256][2];       // +0x10 (low, high: n * 17)
};
static_assert(sizeof(DXTDecoder) == 0x210);

struct NiDevImageConverter : NiRefObject {
    uint8_t nifReader[0x430];                  // +0x8 (NiImageConverter's NiNIFImageReader)
    DXTDecoder decoder;                        // +0x438
    NiTPointerList<NiImageReader*> readers;    // +0x648 (vtable 0x95145C; owned)
};
static_assert(offsetof(NiDevImageConverter, decoder) == 0x438);
static_assert(offsetof(NiDevImageConverter, readers) == 0x648);
static_assert(sizeof(NiDevImageConverter) == 0x658);

} // namespace ni
