#pragma once

// Gamebryo's image readers as Bully's exe has them: NiImageReader (vtable 0x965308, 0x24: a
// critical section) and the file formats the developer image converter (NiDevImageConverter,
// src/gamebryo/NiDevImageConverter.h) lists: Targa, SGI, DDS and BMP (NiNIFImageReader is
// src/gamebryo/NiNIFImageReader.cpp). None is an NiObject: no RTTI, only the four slots 0 the
// deleting destructor, 1 CanReadFile(extension), 2 ReadFile(file, optional destination: reused
// when its size and format match), 3 ReadHeader(file, width&, height&, format&, mipmaps&,
// faces&). Layouts from the constructors 0x8CD5E0, 0x8CE320, 0x8CEBF0, 0x8CFF60.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiPixelData.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {

struct NiImageReader {
    re::Ptr32<void> vtable;                       // +0x0
    sys::NiCriticalSection section;     // +0x4 (held while a file is read)
};
static_assert(sizeof(NiImageReader) == 0x24);

// NiTGAReader (vtable 0x9A1AA8, 0xB0): the file's header, what's derived from it, a row
// buffer, the palette (4 bytes an entry), the row converter (a member function pointer) and
// the state of the run-length packet being read
struct NiTGAReader : NiImageReader {
    uint8_t idLength;          // +0x24
    uint8_t colorMapType;      // +0x25
    uint8_t imageType;         // +0x26 (1/9 palettized, 2/10 true colour, 3/11 grey; 9.. RLE)
    uint8_t pad27;
    uint16_t colorMapOrigin;   // +0x28
    uint16_t colorMapLength;   // +0x2A
    uint8_t colorMapDepth;     // +0x2C
    uint8_t pad2D;
    uint16_t xOrigin;          // +0x2E
    uint16_t yOrigin;          // +0x30
    uint16_t width;            // +0x32
    uint16_t height;           // +0x34
    uint8_t bitsPerPixel;      // +0x36
    uint8_t descriptor;        // +0x37
    uint8_t bytesPerPixel;     // +0x38 (0 for 4 bits)
    uint8_t hasAlpha;          // +0x39 (32 bits)
    uint8_t palettized;        // +0x3A
    uint8_t bottomUp;          // +0x3B (the descriptor's bit 5 clear)
    uint8_t rle;               // +0x3C
    uint8_t pad3D[3];
    NiPixelFormat format;      // +0x40
    uint32_t rowBufferSize;    // +0x84
    re::Ptr32<uint8_t> rowBuffer;        // +0x88 (NiMalloc)
    uint32_t paletteSize;      // +0x8C (entries allocated)
    re::Ptr32<uint8_t> palette;          // +0x90 (new[]: r, g, b, a)
    uint32_t field94;          // +0x94
    re::Ptr32<void> rowFn;               // +0x98 (thiscall(src, dst): one row converted)
    int32_t rowFnAdjust;       // +0x9C (the member pointer's this adjustment)
    uint32_t packetLeft;       // +0xA0 (pixels left in the run-length packet)
    uint8_t packetRepeats;     // +0xA4 (a run of one pixel, not raw pixels)
    uint8_t packetPixel[4];    // +0xA5
    uint8_t padA9[7];
};
static_assert(offsetof(NiTGAReader, format) == 0x40);
static_assert(offsetof(NiTGAReader, rowFn) == 0x98 && offsetof(NiTGAReader, packetPixel) == 0xA5);
static_assert(sizeof(NiTGAReader) == 0xB0);

// NiSGIReader (vtable 0x9A1AE0, 0x70): big-endian SGI images (planar, a channel at a time)
struct NiSGIReader : NiImageReader {
    uint16_t width;            // +0x24 (xsize)
    uint16_t height;           // +0x26 (ysize)
    uint16_t channels;         // +0x28 (zsize: 1 grey, 2 grey and alpha, 3 RGB, 4 RGBA)
    uint8_t bytesPerChannel;   // +0x2A
    uint8_t rle;               // +0x2B
    NiPixelFormat format;      // +0x2C
};
static_assert(sizeof(NiSGIReader) == 0x70);

// NiDDSReader (vtable 0x9A1AF0, 0x74)
struct NiDDSReader : NiImageReader {
    uint32_t height;           // +0x24
    uint32_t width;            // +0x28
    uint32_t mipmapCount;      // +0x2C
    NiPixelFormat format;      // +0x30 (the file's)
};
static_assert(sizeof(NiDDSReader) == 0x74);

// NiBMPReader (vtable 0x9A1B00, 0x80)
struct NiBMPReader : NiImageReader {
    uint32_t width;            // +0x24
    uint32_t height;           // +0x28
    NiPixelFormat format;      // +0x2C
    uint16_t bitCount;         // +0x70
    uint16_t pad72;
    uint32_t colorsUsed;       // +0x74
    uint32_t dataOffset;       // +0x78
    uint8_t bottomUp;          // +0x7C (a positive height)
    uint8_t field7D;           // +0x7D
    uint8_t pad7E[2];
};
static_assert(offsetof(NiBMPReader, bitCount) == 0x70 && offsetof(NiBMPReader, bottomUp) == 0x7C);
static_assert(sizeof(NiBMPReader) == 0x80);

} // namespace ni
