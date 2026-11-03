// The image readers of the developer image converter (src/gamebryo/NiImageReaders.h),
// 0x8CD2F0..0x8D0350: NiTGAReader (its row converters, run-length packets and palette),
// NiSGIReader (planar channels, run-length rows), NiDDSReader (DXT and float formats read
// as they are; 16, 24 and 32-bit RGB expanded to RGB24/RGBA32) and NiBMPReader (4 and 8-bit
// palettized, 24 and 32-bit). Each reader holds its critical section while it reads.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiFile.h"
#include "gamebryo/NiImageReaders.h"
#include "gamebryo/NiPixelData.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using sys::Import;
using sys::NiCriticalSection;

constexpr uintptr_t kNiImageReaderVtable = 0x00965308;
constexpr uintptr_t kNiTGAReaderVtable = 0x009A1AA8;
constexpr uintptr_t kNiSGIReaderVtable = 0x009A1AE0;
constexpr uintptr_t kNiDDSReaderVtable = 0x009A1AF0;
constexpr uintptr_t kNiBMPReaderVtable = 0x009A1B00;

GAME_VAR(int, NiFile_ms_iSeekCur, 0x00950FBC);   // (1)
GAME_VAR(NiPixelFormat, PixelFormat_PAL8, 0x00BC76C0);
GAME_VAR(NiPixelFormat, PixelFormat_PALA8, 0x00BC7708);
GAME_VAR(NiPixelFormat, PixelFormat_RGBA32, 0x00BC7750);
GAME_VAR(NiPixelFormat, PixelFormat_RGB24, 0x00BC7798);
GAME_VAR(NiPixelFormat, PixelFormat_DXT1, 0x00BC7900);
GAME_VAR(NiPixelFormat, PixelFormat_DXT3, 0x00BC7948);
GAME_VAR(NiPixelFormat, PixelFormat_DXT5, 0x00BC7990);
GAME_VAR(NiPixelFormat, PixelFormat_RGBA128, 0x00BC79D8);   // 32-bit float channels
GAME_VAR(NiPixelFormat, PixelFormat_RGBA64, 0x00BC7A20);    // 16-bit float channels
GAME_VAR(NiPixelFormat, PixelFormat_RG64, 0x00BC7A68);
GAME_VAR(NiPixelFormat, PixelFormat_RG32, 0x00BC7AB0);
GAME_VAR(NiPixelFormat, PixelFormat_R32, 0x00BC7AF8);
GAME_VAR(NiPixelFormat, PixelFormat_R16, 0x00BC7B40);
GAME_VAR(NiPixelFormat, PixelFormat_PAL4, 0x00BC7630);      // (Targa's 16-entry palettes)
GAME_VAR(NiPixelFormat, PixelFormat_PALA4, 0x00BC7678);

GAME_FN(int, __cdecl, NiStricmp, 0x00753820, const char* a, const char* b);
GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __thiscall, CriticalSection_Unlock, 0x00401000, NiCriticalSection* self);
GAME_FN(NiPixelFormat*, __thiscall, PixelFormat_Construct, 0x0076B280, NiPixelFormat* self);
GAME_FN(uint8_t, __thiscall, PixelFormat_Equals, 0x007D3AB0, const NiPixelFormat* self, const NiPixelFormat* other);
GAME_FN(uint8_t, __thiscall, PixelFormat_NotEquals, 0x0076A090, const NiPixelFormat* self,
        const NiPixelFormat* other);
GAME_FN(uint32_t, __thiscall, PixelFormat_GetMask, 0x007548E0, const NiPixelFormat* self, int component);
GAME_FN(NiPixelFormat*, __cdecl, PixelFormat_CreateFromRGBAMasks, 0x0076B370, NiPixelFormat* result, int bits,
        uint32_t red, uint32_t green, uint32_t blue, uint32_t alpha);
GAME_FN(NiPixelData*, __thiscall, PixelData_Construct, 0x0076A610, void* self, uint32_t width, uint32_t height,
        const NiPixelFormat* format, uint32_t levels, uint32_t faces);
GAME_FN(void, __thiscall, PixelData_SetPalette, 0x007D3E40, NiPixelData* self, NiObject* palette);
GAME_FN(NiObject*, __thiscall, Palette_Construct, 0x007D45F0, void* self, int alpha, uint32_t count,
        const void* entries);
GAME_FN(void, __cdecl, Stream_LoadBytes, 0x008885F0, NiBinaryStream* stream, void* buffer, uint32_t count);
GAME_FN(void, __cdecl, Stream_LoadUInt32, 0x00822840, NiBinaryStream* stream, uint32_t* value);
// the masks, shifts and lost bits of a format's four channels (0x18: chunk 44's)
struct ChannelMasks {
    uint32_t mask[4];   // +0x0 (red, green, blue, alpha)
    uint8_t shift[4];   // +0x10
    uint8_t loss[4];    // +0x14 (8 - the channel's bits; 8 for none)
};
static_assert(sizeof(ChannelMasks) == 0x18);
GAME_FN(void, __thiscall, ChannelMasks_Construct, 0x008C6820, ChannelMasks* self, const NiPixelFormat* format);
// this file's, called where modes apply
GAME_FN(void, __thiscall, TGA_ReadRLE, 0x008CD710, NiTGAReader* self, NiBinaryStream* file, uint8_t* dst,
        uint32_t size);
GAME_FN(void, __thiscall, TGA_ReadPalette, 0x008CD870, NiTGAReader* self, NiBinaryStream* file);
GAME_FN(void, __thiscall, SGI_ExpandRLE8, 0x008CE150, NiSGIReader* self, uint8_t* dst, const uint8_t* src,
        int32_t stride);
GAME_FN(void, __thiscall, SGI_ExpandRLE16, 0x008CE1E0, NiSGIReader* self, uint8_t* dst, const uint8_t* src,
        int32_t stride);
GAME_FN(void, __cdecl, Stream_LoadUInt32s, 0x008CE2E0, NiBinaryStream* stream, uint32_t* values, uint32_t count);
GAME_FN(uint8_t, __thiscall, DDS_IsSupportedRGB, 0x008CEB60, NiDDSReader* self, uint32_t red, uint32_t green,
        uint32_t blue, uint32_t alpha, uint32_t bits);
// (declared by hand, not with GAME_FN: tools/gamefncheck.py counts the format passed by value as
//  4 bytes, where the game pops all 0x44 of it with the rest: ret 0x48)
inline NiPixelFormat*(__thiscall* const DDS_FindClosestFormat)(NiDDSReader*, NiPixelFormat*, NiPixelFormat) =
    reinterpret_cast<NiPixelFormat*(__thiscall*)(NiDDSReader*, NiPixelFormat*, NiPixelFormat)>(0x008CEB80);
GAME_FN(void, __cdecl, DDS_Load24, 0x008CF1D0, NiBinaryStream* file, NiPixelData* pixels, NiPixelFormat source,
        NiPixelFormat dest, uint32_t level, uint32_t face);
GAME_FN(void, __cdecl, DDS_Load32, 0x008CF2A0, NiBinaryStream* file, NiPixelData* pixels, NiPixelFormat source,
        NiPixelFormat dest, uint32_t level, uint32_t face);
GAME_FN(void, __cdecl, DDS_Load16, 0x008CF730, NiBinaryStream* file, NiPixelData* pixels, NiPixelFormat source,
        NiPixelFormat dest, uint32_t level, uint32_t face);

template <typename R = void, typename... A>
R Virtual(const void* object, int slot, A... args) {
    return reinterpret_cast<R(__thiscall*)(const void*, re::Arg<A>...)>(static_cast<void*>(VtableOf(object)[slot]))(object, args...);
}

// NiStreamLoadBinary as inlined: `bytes` read as one component of `componentSize`
void Read(NiBinaryStream* file, void* buffer, uint32_t bytes, uint32_t componentSize) {
    uint32_t size = componentSize;
    file->absoluteCurrentPos += file->read(file, buffer, bytes, &size, 1);
}

// the file's vf4 SetEndianSwap and vf5 Seek(offset, from the current position)
void SetEndianSwap(NiBinaryStream* file, uint8_t swap) {
    Virtual(file, 4, uint32_t(swap));
}
void Skip(NiBinaryStream* file, int offset) {
    Virtual(file, 5, offset, NiFile_ms_iSeekCur);
}

// NiImageReader's constructor, inlined
void ReaderConstruct(NiImageReader* self) {
    self->vtable = reinterpret_cast<void*>(kNiImageReaderVtable);
    self->section.owner = 0;
    self->section.lockCount = 0;
    Import<void>(sys::kInitializeCriticalSection, &self->section.section);
}

void ReaderDestruct(NiImageReader* self) {
    self->vtable = reinterpret_cast<void*>(kNiImageReaderVtable);
    Import<void>(sys::kDeleteCriticalSection, &self->section.section);
}

// the destination when it's the image's size and format, else a new NiPixelData
NiPixelData* Destination(NiPixelData* dest, uint32_t width, uint32_t height, const NiPixelFormat* format) {
    if (dest != nullptr && dest->widths[0] == width && dest->heights[0] == height &&
        PixelFormat_Equals(&dest->format, format) != 0) {
        return dest;
    }
    void* p = NiNew(sizeof(NiPixelData));
    return p != nullptr ? PixelData_Construct(p, width, height, format, 1, 1) : nullptr;
}

// ---- NiTGAReader

// the row converters (thiscall(src, dst), one row of `width` pixels)

// 4-bit palettized: half a row of bytes, less the palette's origin
void __fastcall TGARowPal4(NiTGAReader* self, re::Edx, const uint8_t* src, uint8_t* dst) {
    for (uint32_t i = 0; i < uint32_t(self->width >> 1); i++) {
        *dst++ = uint8_t(*src++ - uint8_t(self->colorMapOrigin));
    }
}

// 8-bit palettized: the indices less the palette's origin
void __fastcall TGARowPal8(NiTGAReader* self, re::Edx, const uint8_t* src, uint8_t* dst) {
    for (uint32_t i = 0; i < self->width; i++) {
        *dst++ = uint8_t(*src++ - uint8_t(self->colorMapOrigin));
    }
}

// 8-bit grey: RGB
void __fastcall TGARowGrey8(NiTGAReader* self, re::Edx, const uint8_t* src, uint8_t* dst) {
    for (uint32_t i = 0; i < self->width; i++) {
        uint8_t v = *src++;
        dst[2] = v;
        dst[1] = v;
        dst[0] = v;
        dst += 3;
    }
}

// 16-bit (A1R5G5B5): RGB, each channel's 5 bits shifted up
void __fastcall TGARow16(NiTGAReader* self, re::Edx, const uint8_t* src, uint8_t* dst) {
    for (uint32_t i = 0; i < self->width; i++) {
        dst[0] = uint8_t((src[1] & 0xFC) * 2);
        dst[1] = uint8_t(((src[0] >> 2) & 0x38) + (src[1] << 6));
        dst[2] = uint8_t(src[0] * 8);
        dst += 3;
        src += 2;
    }
}

// 16-bit palette indices: the palette's RGB / RGBA
void __fastcall TGARowPal16(NiTGAReader* self, re::Edx, const uint8_t* src, uint8_t* dst) {
    for (uint32_t i = 0; i < self->width; i++) {
        const uint8_t* entry = self->palette + ((uint32_t(src[1]) << 8) - self->colorMapOrigin + src[0]) * 4;
        dst[0] = entry[0];
        dst[1] = entry[1];
        dst[2] = entry[2];
        dst += 3;
        src += 2;
    }
}

void __fastcall TGARowPal16Alpha(NiTGAReader* self, re::Edx, const uint8_t* src, uint8_t* dst) {
    for (uint32_t i = 0; i < self->width; i++) {
        const uint8_t* entry = self->palette + ((uint32_t(src[1]) << 8) - self->colorMapOrigin + src[0]) * 4;
        dst[0] = entry[0];
        dst[1] = entry[1];
        dst[2] = entry[2];
        dst[3] = entry[3];
        dst += 4;
        src += 2;
    }
}

// 24-bit BGR: RGB
void __fastcall TGARow24(NiTGAReader* self, re::Edx, const uint8_t* src, uint8_t* dst) {
    for (uint32_t i = 0; i < self->width; i++) {
        dst[0] = src[2];
        dst[1] = src[1];
        dst[2] = src[0];
        dst += 3;
        src += 3;
    }
}

// 32-bit BGRA: RGBA
void __fastcall TGARow32(NiTGAReader* self, re::Edx, const uint8_t* src, uint8_t* dst) {
    for (uint32_t i = 0; i < self->width; i++) {
        dst[0] = src[2];
        dst[1] = src[1];
        dst[2] = src[0];
        dst[3] = src[3];
        dst += 4;
        src += 4;
    }
}

// vf1 CanReadFile
uint8_t __fastcall TGACanReadFile(NiTGAReader*, re::Edx, const char* extension) {
    if (NiStricmp(extension, ".tga") != 0 && NiStricmp(extension, ".targa") != 0) {
        return 0;
    }
    return 1;
}

// NiStreamLoadBinary(stream, uint16&)
void __cdecl StreamLoadUInt16(NiBinaryStream* stream, uint16_t* value) {
    Read(stream, value, 2, 2);
}

NiTGAReader* __fastcall TGAConstruct(NiTGAReader* self, re::Edx) {
    ReaderConstruct(self);
    self->vtable = reinterpret_cast<void*>(kNiTGAReaderVtable);
    PixelFormat_Construct(&self->format);
    self->idLength = 0;
    self->colorMapType = 0;
    self->imageType = 0;
    self->colorMapOrigin = 0;
    self->colorMapLength = 0;
    self->colorMapDepth = 0;
    self->xOrigin = 0;
    self->yOrigin = 0;
    self->width = 0;
    self->height = 0;
    self->bitsPerPixel = 0;
    self->descriptor = 0;
    self->bytesPerPixel = 0;
    self->hasAlpha = 0;
    self->palettized = 0;
    self->bottomUp = 0;
    self->rle = 0;
    self->format = PixelFormat_RGB24;
    self->rowFn = nullptr;
    self->rowBufferSize = 0;
    self->rowBuffer = nullptr;
    self->paletteSize = 0;
    self->palette = nullptr;
    self->packetLeft = 0;
    self->packetRepeats = 0;
    self->packetPixel[0] = 0;
    self->packetPixel[1] = 0;
    self->packetPixel[2] = 0;
    self->packetPixel[3] = 0;
    self->rowFnAdjust = 0;
    return self;
}

NiTGAReader* __fastcall TGADeletingDestructor(NiTGAReader* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(kNiTGAReaderVtable);
    NiFree(self->rowBuffer);
    NiDeleteArray(self->palette, 4);
    ReaderDestruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(NiTGAReader));
    }
    return self;
}

// one run-length encoded row into `dst` (packets go on across rows): a raw packet's pixels
// read, a run's one pixel copied (memcpy_s with the whole buffer's size each time)
void __fastcall TGAReadRLE(NiTGAReader* self, re::Edx, NiBinaryStream* file, uint8_t* dst, uint32_t size) {
    uint32_t done = 0;
    while (done < self->width) {
        if (self->packetLeft == 0) {
            uint8_t header = 0;
            Read(file, &header, 1, 1);
            self->packetRepeats = uint8_t(header >> 7);
            if (header & 0x80) {
                self->packetLeft = uint32_t(header) - 0x7F;
                self->packetRepeats = 1;
                Read(file, self->packetPixel, self->bytesPerPixel, 1);
            } else {
                self->packetRepeats = 0;
                self->packetLeft = uint32_t(header) + 1;
            }
        }
        uint32_t count = self->width - done;
        if (self->packetLeft < count) {
            count = self->packetLeft;
        }
        if (self->packetRepeats == 0) {
            Read(file, dst, self->bytesPerPixel * count, 1);
            dst += self->bytesPerPixel * count;
        } else {
            for (uint32_t i = 0; i < count; i++) {
                crt::MemcpyS(dst, size, self->packetPixel, self->bytesPerPixel);
                dst += self->bytesPerPixel;
            }
        }
        self->packetLeft -= count;
        done += count;
    }
}

// the colour map read into the palette (grown when needed) as RGBA entries
void __fastcall TGAReadPalette(NiTGAReader* self, re::Edx, NiBinaryStream* file) {
    if (self->paletteSize < self->colorMapLength) {
        self->paletteSize = self->colorMapLength;
        NiDeleteArray(self->palette, 4);
        uint64_t bytes = uint64_t(self->paletteSize) * 4;
        self->palette = static_cast<uint8_t*>(NiNewArray((bytes >> 32) != 0 ? 0xFFFFFFFF : uint32_t(bytes)));
    }
    uint32_t needed = uint32_t(self->colorMapLength) * 4;
    if (self->rowBufferSize < needed) {
        self->rowBufferSize = needed;
        NiFree(self->rowBuffer);
        self->rowBuffer = static_cast<uint8_t*>(NiMalloc(self->rowBufferSize));
    }
    uint8_t* entry = self->palette;
    const uint8_t* src = self->rowBuffer;
    switch (self->colorMapDepth) {
    case 8:
        Stream_LoadBytes(file, self->rowBuffer, self->colorMapLength);
        for (uint32_t i = 0; i < self->colorMapLength; i++) {
            uint8_t v = *src++;
            entry[2] = v;
            entry[1] = v;
            entry[0] = v;
            entry[3] = 0xFF;
            entry += 4;
        }
        break;
    case 15:
    case 16:
        Stream_LoadBytes(file, self->rowBuffer, uint32_t(self->colorMapLength) * 2);
        for (uint32_t i = 0; i < self->colorMapLength; i++) {
            entry[0] = uint8_t((src[1] & 0xFC) * 2);
            entry[1] = uint8_t(((src[0] >> 2) & 0x38) + (src[1] << 6));
            entry[2] = uint8_t(src[0] * 8);
            entry[3] = 0xFF;
            entry += 4;
            src += 2;
        }
        break;
    case 24:
        Stream_LoadBytes(file, self->rowBuffer, uint32_t(self->colorMapLength) * 3);
        for (uint32_t i = 0; i < self->colorMapLength; i++) {
            entry[2] = src[0];
            entry[1] = src[1];
            entry[0] = src[2];
            entry[3] = 0xFF;
            src += 3;
            entry += 4;
        }
        break;
    case 32:
        Stream_LoadBytes(file, self->rowBuffer, uint32_t(self->colorMapLength) * 4);
        for (uint32_t i = 0; i < self->colorMapLength; i++) {
            entry[2] = src[0];
            entry[1] = src[1];
            entry[0] = src[2];
            entry[3] = src[3];
            src += 4;
            entry += 4;
        }
        break;
    default:
        break;
    }
}

// vf3 ReadHeader: the 18-byte header (little endian), the image ID skipped; the image type
// (palettized and/or run-length; others refused), the format (32 bits or a 32-bit colour map:
// with alpha) and the row converter by the pixel size (4 bits only palettized)
uint8_t __fastcall TGAReadHeader(NiTGAReader* self, re::Edx, NiBinaryStream* file, uint32_t* width, uint32_t* height,
                                 NiPixelFormat* format, uint8_t* mipmaps, uint32_t* faces) {
    SetEndianSwap(file, 0);
    sys::Lock(self->section);
    *faces = 1;
    Read(file, &self->idLength, 1, 1);
    Read(file, &self->colorMapType, 1, 1);
    Read(file, &self->imageType, 1, 1);
    Read(file, &self->colorMapOrigin, 2, 2);
    Read(file, &self->colorMapLength, 2, 2);
    Read(file, &self->colorMapDepth, 1, 1);
    Read(file, &self->xOrigin, 2, 2);
    Read(file, &self->yOrigin, 2, 2);
    Read(file, &self->width, 2, 2);
    Read(file, &self->height, 2, 2);
    Read(file, &self->bitsPerPixel, 1, 1);
    Read(file, &self->descriptor, 1, 1);
    if (self->idLength != 0) {
        Skip(file, self->idLength);
    }
    self->bottomUp = uint8_t(~(self->descriptor >> 5) & 1);
    switch (self->imageType) {
    case 1:
        self->palettized = 1;
        self->rle = 0;
        break;
    case 2:
    case 3:
        self->rle = 0;
        self->palettized = 0;
        break;
    case 9:
        self->palettized = 1;
        self->rle = 1;
        break;
    case 10:
    case 11:
        self->rle = 1;
        self->palettized = 0;
        break;
    default:
        sys::Unlock(self->section);
        return 0;
    }
    uint8_t depth = self->palettized != 0 ? self->colorMapDepth : self->bitsPerPixel;
    uint8_t alpha = depth == 0x20;
    self->hasAlpha = alpha;
    const NiPixelFormat* chosen;
    if (self->palettized != 0) {
        if (self->colorMapLength == 0x10) {
            chosen = alpha != 0 ? &PixelFormat_PALA4 : &PixelFormat_PAL4;
        } else {
            chosen = alpha != 0 ? &PixelFormat_PALA8 : &PixelFormat_PAL8;
        }
    } else {
        chosen = alpha != 0 ? &PixelFormat_RGBA32 : &PixelFormat_RGB24;
    }
    self->format = *chosen;
    void* rowFn = self->rowFn;
    switch (self->bitsPerPixel) {
    case 4:
        self->bytesPerPixel = 0;
        if (self->palettized == 0) {
            CriticalSection_Unlock(&self->section);
            return 0;
        }
        rowFn = reinterpret_cast<void*>(0x008CD2F0);
        self->rowFnAdjust = 0;
        break;
    case 8:
        self->bytesPerPixel = 1;
        self->rowFnAdjust = 0;
        rowFn = reinterpret_cast<void*>(self->palettized != 0 ? 0x008CD330 : 0x008CD360);
        break;
    case 15:
    case 16:
        self->bytesPerPixel = 2;
        if (self->palettized != 0) {
            self->rowFnAdjust = 0;
            rowFn = reinterpret_cast<void*>(alpha != 0 ? 0x008CD460 : 0x008CD400);
        } else {
            rowFn = reinterpret_cast<void*>(0x008CD3A0);
            self->rowFnAdjust = 0;
        }
        break;
    case 24:
        self->bytesPerPixel = 3;
        rowFn = reinterpret_cast<void*>(0x008CD4D0);
        self->rowFnAdjust = 0;
        break;
    case 32:
        self->bytesPerPixel = 4;
        rowFn = reinterpret_cast<void*>(0x008CD520);
        self->rowFnAdjust = 0;
        break;
    default:
        break;   // (the converter left as it was)
    }
    self->rowFn = rowFn;
    self->packetRepeats = 0;
    self->packetLeft = 0;
    *width = self->width;
    *height = self->height;
    *format = self->format;
    *mipmaps = 0;
    sys::Unlock(self->section);
    return 1;
}

// vf2 ReadFile: the header, the colour map (with too many entries refused), the pixels (the
// destination when it fits) row by row through the converter, bottom-up images flipped; a
// palettized image given its NiPalette. (Failing after the header, nothing is let go.)
NiPixelData* __fastcall TGAReadFile(NiTGAReader* self, re::Edx, NiBinaryStream* file, NiPixelData* dest) {
    NiPixelFormat format;
    PixelFormat_Construct(&format);
    sys::Lock(self->section);
    uint32_t width;
    uint32_t height;
    uint8_t mipmaps;
    uint32_t faces;
    if (Virtual<uint8_t>(self, 3, file, &width, &height, &format, &mipmaps, &faces) == 0) {
        sys::Unlock(self->section);
        return nullptr;
    }
    uint32_t rowBytes = uint32_t(self->bytesPerPixel) * self->width;
    if (self->bytesPerPixel == 0) {
        if (self->bitsPerPixel != 4) {
            sys::Unlock(self->section);
            return nullptr;
        }
        rowBytes = uint32_t(self->width) >> 1;
    }
    if (self->colorMapType != 0) {
        if (uint32_t(self->colorMapLength) + self->colorMapOrigin + 1 >= 0x4000) {
            sys::Unlock(self->section);
            return nullptr;
        }
        TGA_ReadPalette(self, file);
    } else if (self->palettized != 0) {
        sys::Unlock(self->section);
        return nullptr;
    }
    NiPixelData* pixels = Destination(dest, self->width, self->height, &self->format);
    if (self->palettized != 0) {
        void* p = NiNew(0x24);
        NiObject* palette = p != nullptr ? Palette_Construct(p, self->hasAlpha, self->colorMapLength, self->palette)
                                         : nullptr;
        PixelData_SetPalette(pixels, palette);
    }
    if (self->rowBufferSize < rowBytes) {
        uint8_t* old = self->rowBuffer;
        self->rowBufferSize = rowBytes;
        NiFree(old);
        self->rowBuffer = static_cast<uint8_t*>(NiMalloc(self->rowBufferSize));
    }
    uint8_t* dst = pixels->pixels + pixels->offsets[0];
    int32_t stride = int32_t(pixels->pixelStride * pixels->widths[0]);
    if (stride == 0) {
        stride = int32_t(pixels->widths[0] >> 1);
    }
    if (self->bottomUp != 0) {
        dst += (uint32_t(self->height) - 1) * uint32_t(stride);
        stride = -stride;
    }
    auto rowFn = re::PtrCast<void(__thiscall*)(void*, const uint8_t*, uint8_t*)>(self->rowFn);
    for (uint32_t row = 0; row < self->height; row++) {
        if (self->rle != 0) {
            TGA_ReadRLE(self, file, self->rowBuffer, self->rowBufferSize);
        } else {
            Read(file, self->rowBuffer, rowBytes, 1);
        }
        rowFn(reinterpret_cast<uint8_t*>(self) + self->rowFnAdjust, self->rowBuffer, dst);
        dst += stride;
    }
    sys::Unlock(self->section);
    return pixels;
}

// ---- NiSGIReader

// a run-length row of one channel (8 bits) into every `stride`th byte: packets of raw bytes or
// a run, until a zero count. (The pixel count it checks against the width gains 255 a packet:
// the game's bug, so it's only ever the zero count that stops it.)
void __fastcall SGIExpandRLE8(NiSGIReader* self, re::Edx, uint8_t* dst, const uint8_t* src, int32_t stride) {
    uint8_t header = *src;
    uint16_t count = 0;
    while (header != 0) {
        if (count == self->width) {
            return;
        }
        src++;
        uint8_t n = header & 0x7F;
        if (header & 0x80) {
            while (n != 0) {
                n--;
                *dst = *src++;
                dst += stride;
            }
        } else {
            uint8_t value = *src++;
            while (n != 0) {
                n--;
                *dst = value;
                dst += stride;
            }
        }
        header = *src;
        count = uint16_t(count + uint8_t(n - 1));
    }
}

// the same for 16-bit channels: each value's high byte (the counts still read as bytes)
void __fastcall SGIExpandRLE16(NiSGIReader* self, re::Edx, uint8_t* dst, const uint8_t* src, int32_t stride) {
    uint8_t header = *src;
    uint16_t count = 0;
    while (header != 0) {
        if (count == self->width) {
            return;
        }
        src++;
        uint8_t n = header & 0x7F;
        if (header & 0x80) {
            while (n != 0) {
                n--;
                *dst = *src;
                src += 2;
                dst += stride;
            }
        } else {
            uint8_t value = *src;
            src += 2;
            while (n != 0) {
                n--;
                *dst = value;
                dst += stride;
            }
        }
        header = *src;
        count = uint16_t(count + uint8_t(n - 1));
    }
}

uint8_t __fastcall SGICanReadFile(NiSGIReader*, re::Edx, const char* extension) {
    if (NiStricmp(extension, ".sgi") != 0 && NiStricmp(extension, ".rgb") != 0 &&
        NiStricmp(extension, ".rgba") != 0 && NiStricmp(extension, ".int") != 0 &&
        NiStricmp(extension, ".inta") != 0) {
        return 0;
    }
    return 1;
}

// NiStreamLoadBinary(stream, uint32 values[count])
void __cdecl StreamLoadUInt32s(NiBinaryStream* stream, uint32_t* values, uint32_t count) {
    Read(stream, values, count * 4, 4);
}

NiSGIReader* __fastcall SGIConstruct(NiSGIReader* self, re::Edx) {
    ReaderConstruct(self);
    self->vtable = reinterpret_cast<void*>(kNiSGIReaderVtable);
    PixelFormat_Construct(&self->format);
    self->width = 0;
    self->height = 0;
    self->channels = 0;
    self->bytesPerChannel = 0;
    self->rle = 0;
    self->format = PixelFormat_RGB24;
    return self;
}

// (the deleting destructor the DDS and BMP readers' vtables don't share: each has its own)
NiSGIReader* __fastcall SGIDeletingDestructor(NiSGIReader* self, re::Edx, uint32_t flags) {
    ReaderDestruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(NiSGIReader));
    }
    return self;
}

// vf3 ReadHeader: the 512-byte big-endian header (magic 474; the magic and storage read
// before the lock); verbatim or run-length, 1..4 channels of 1 or 2 bytes: RGBA32 with alpha
// (2 or 4 channels), else RGB24
uint8_t __fastcall SGIReadHeader(NiSGIReader* self, re::Edx, NiBinaryStream* file, uint32_t* width, uint32_t* height,
                                 NiPixelFormat* format, uint8_t* mipmaps, uint32_t* faces) {
    SetEndianSwap(file, 1);
    *faces = 1;
    uint16_t magic = 0;
    Read(file, &magic, 2, 2);
    if (magic != 0x1DA) {
        return 0;
    }
    uint8_t storage = 0;
    Read(file, &storage, 1, 1);
    sys::Lock(self->section);
    Read(file, &self->bytesPerChannel, 1, 1);
    uint16_t dimension;
    Read(file, &dimension, 2, 2);
    Read(file, &self->width, 2, 2);
    Read(file, &self->height, 2, 2);
    Read(file, &self->channels, 2, 2);
    uint32_t pixel;
    Read(file, &pixel, 4, 4);   // minimum
    Read(file, &pixel, 4, 4);   // maximum
    Skip(file, 0x54);           // (4 bytes, the 80-byte name)
    Read(file, &pixel, 4, 4);   // colour map
    Skip(file, 0x194);
    if (storage == 1) {
        self->rle = storage;
    } else if (storage == 0) {
        self->rle = 0;
    } else {
        sys::Unlock(self->section);
        return 0;
    }
    uint16_t channels = self->channels;
    if (channels > 4 || self->bytesPerChannel > 2 || self->bytesPerChannel == 0) {
        sys::Unlock(self->section);
        return 0;
    }
    self->format = channels == 4 || channels == 2 ? PixelFormat_RGBA32 : PixelFormat_RGB24;
    *width = self->width;
    *height = self->height;
    *format = self->format;
    *mipmaps = 0;
    sys::Unlock(self->section);
    return 1;
}

// vf2 ReadFile: the channels read whole (run-length: the row start and length tables first,
// the rows expanded from the data after the 512-byte header and tables) and spread into the
// pixels, bottom row first; grey copied into green and blue (and alpha moved up). (Failing
// to allocate the data, the start table isn't freed.)
NiPixelData* __fastcall SGIReadFile(NiSGIReader* self, re::Edx, NiBinaryStream* file, NiPixelData* dest) {
    NiPixelFormat format;
    PixelFormat_Construct(&format);
    sys::Lock(self->section);
    uint32_t width;
    uint32_t height;
    uint8_t mipmaps;
    uint32_t faces;
    if (Virtual<uint8_t>(self, 3, file, &width, &height, &format, &mipmaps, &faces) == 0) {
        sys::Unlock(self->section);
        return nullptr;
    }
    NiPixelData* pixels = Destination(dest, self->width, self->height, &self->format);
    uint32_t* starts = nullptr;
    uint32_t size;
    if (self->rle != 0) {
        uint32_t rows = uint32_t(self->channels) * self->height;
        starts = static_cast<uint32_t*>(NiMalloc(rows * 4));
        if (starts == nullptr) {
            sys::Unlock(self->section);
            return nullptr;
        }
        Stream_LoadUInt32s(file, starts, rows);
        size = 0;
        for (uint32_t i = rows; i != 0; i--) {
            uint32_t length;
            Read(file, &length, 4, 4);
            size += length;
        }
    } else {
        size = uint32_t(self->bytesPerChannel) * self->width * self->channels * self->height;
    }
    auto* data = static_cast<uint8_t*>(NiMalloc(size));
    if (data == nullptr) {
        sys::Unlock(self->section);
        return nullptr;
    }
    Read(file, data, size, 1);
    int32_t stride = int32_t(pixels->pixelStride);
    int32_t rowBytes = int32_t(pixels->widths[0] * pixels->pixelStride);
    uint8_t* base = pixels->pixels + pixels->offsets[0];
    if (self->rle != 0) {
        uint32_t dataStart = uint32_t(self->channels) * self->height * 8 + 0x200;
        for (uint32_t channel = 0; channel < self->channels; channel++) {
            uint8_t* dst = base + (uint32_t(self->height) - 1) * uint32_t(rowBytes) + channel;
            for (uint32_t row = 0; row < self->height; row++) {
                const uint8_t* src = data + (starts[self->height * channel + row] - dataStart);
                if (self->bytesPerChannel == 1) {
                    SGI_ExpandRLE8(self, dst, src, stride);
                } else {
                    SGI_ExpandRLE16(self, dst, src, stride);
                }
                dst -= rowBytes;
            }
        }
    } else {
        const uint8_t* src = data;
        uint32_t step = self->bytesPerChannel == 1 ? 1 : 2;
        for (uint32_t channel = 0; channel < self->channels; channel++) {
            uint8_t* dst = base + (uint32_t(self->height) - 1) * uint32_t(rowBytes) + channel;
            for (uint32_t row = 0; row < self->height; row++) {
                for (uint16_t x = 0; x < self->width; x++) {
                    *dst = *src;
                    src += step;
                    dst += stride;
                }
                dst -= rowBytes * 2;
            }
        }
    }
    NiFree(data);
    NiFree(starts);
    uint8_t* p = pixels->pixels + pixels->offsets[0];
    uint32_t count = uint32_t(self->width) * self->height;
    if (self->channels == 1) {
        for (; count != 0; count--) {
            uint8_t v = p[0];
            p[2] = v;
            p[1] = v;
            p += 3;
        }
    } else if (self->channels == 2) {
        for (; count != 0; count--) {
            p[3] = p[1];
            uint8_t v = p[0];
            p[2] = v;
            p[1] = v;
            p += 4;
        }
    }
    sys::Unlock(self->section);
    return pixels;
}

// ---- NiDDSReader

uint8_t __fastcall DDSCanReadFile(NiDDSReader*, re::Edx, const char* extension) {
    return NiStricmp(extension, ".dds") == 0;
}

// an uncompressed format it reads: 16, 24 or 32 bits a pixel
uint8_t __fastcall DDSIsSupportedRGB(NiDDSReader*, re::Edx, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t bits) {
    return bits == 0x10 || bits == 0x18 || bits == 0x20;
}

// what an uncompressed format is read as: 16 bits RGBA32 with alpha else RGB24, 24 bits
// RGB24, 32 bits RGBA32; others as they are
NiPixelFormat* __fastcall DDSFindClosestFormat(NiDDSReader*, re::Edx, NiPixelFormat* result, NiPixelFormat format) {
    switch (format.bitsPerPixel) {
    case 0x10:
        *result = PixelFormat_GetMask(&format, 3) != 0 ? PixelFormat_RGBA32 : PixelFormat_RGB24;
        break;
    case 0x18:
        *result = PixelFormat_RGB24;
        break;
    case 0x20:
        *result = PixelFormat_RGBA32;
        break;
    default:
        *result = format;
        break;
    }
    return result;
}

NiDDSReader* __fastcall DDSConstruct(NiDDSReader* self, re::Edx) {
    ReaderConstruct(self);
    self->vtable = reinterpret_cast<void*>(kNiDDSReaderVtable);
    PixelFormat_Construct(&self->format);
    self->height = 0;
    self->width = 0;
    self->format = PixelFormat_DXT1;
    return self;
}

NiDDSReader* __fastcall DDSDeletingDestructor(NiDDSReader* self, re::Edx, uint32_t flags) {
    ReaderDestruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(NiDDSReader));
    }
    return self;
}

// vf3 ReadHeader: "DDS " (byte-swapped: a big-endian file), the DDSURFACEDESC2: caps, height
// and width needed, volumes refused; DXT1/3/5 and the float FourCCs read as they are, RGB
// formats of 16..32 bits converted (the format made from the masks, then the closest one);
// a cube map needs all six faces; the mipmap count kept
uint8_t __fastcall DDSReadHeader(NiDDSReader* self, re::Edx, NiBinaryStream* file, uint32_t* width, uint32_t* height,
                                 NiPixelFormat* format, uint8_t* mipmaps, uint32_t* faces) {
    sys::Lock(self->section);
    uint32_t magic;
    Read(file, &magic, 4, 4);
    if (magic != 0x20534444) {
        uint32_t swapped = (magic >> 24) | ((magic >> 8) & 0xFF00) | ((magic << 8) & 0xFF0000) | (magic << 24);
        if (swapped != 0x20534444) {
            sys::Unlock(self->section);
            return 0;
        }
        SetEndianSwap(file, 1);
    }
    uint32_t size;
    Read(file, &size, 4, 4);
    if (size != 0x7C) {
        sys::Unlock(self->section);
        return 0;
    }
    uint32_t flags;
    Read(file, &flags, 4, 4);
    if ((flags & 0x1007) != 0x1007) {   // caps, height, width, pixel format
        sys::Unlock(self->section);
        return 0;
    }
    Read(file, &self->height, 4, 4);
    Read(file, &self->width, 4, 4);
    uint32_t value;
    Read(file, &value, 4, 4);   // pitch or linear size
    Read(file, &value, 4, 4);   // depth
    if ((flags & 0x800000) != 0 && value != 0) {
        sys::Unlock(self->section);
        return 0;
    }
    uint32_t mipmapCount;
    Read(file, &mipmapCount, 4, 4);
    Skip(file, 0x2C);
    uint32_t pfSize;
    uint32_t pfFlags;
    uint32_t fourCC;
    Read(file, &pfSize, 4, 4);
    Read(file, &pfFlags, 4, 4);
    Read(file, &fourCC, 4, 4);
    if (pfFlags & 4) {   // DDPF_FOURCC
        const NiPixelFormat* chosen;
        switch (fourCC) {
        case 0x31545844: chosen = &PixelFormat_DXT1; break;
        case 0x33545844: chosen = &PixelFormat_DXT3; break;
        case 0x35545844: chosen = &PixelFormat_DXT5; break;
        case 0x6F: chosen = &PixelFormat_R16; break;       // D3DFMT_R16F
        case 0x70: chosen = &PixelFormat_RG32; break;      // G16R16F
        case 0x71: chosen = &PixelFormat_RGBA64; break;    // A16B16G16R16F
        case 0x72: chosen = &PixelFormat_R32; break;       // R32F
        case 0x73: chosen = &PixelFormat_RG64; break;      // G32R32F
        case 0x74: chosen = &PixelFormat_RGBA128; break;   // A32B32G32R32F
        default:
            CriticalSection_Unlock(&self->section);
            return 0;
        }
        self->format = *chosen;
        *format = self->format;
        Skip(file, 0x14);   // (the bit count and masks)
    } else {
        if ((pfFlags & 0x40) == 0) {   // DDPF_RGB
            CriticalSection_Unlock(&self->section);
            return 0;
        }
        uint32_t bits;
        uint32_t red;
        uint32_t green;
        uint32_t blue;
        uint32_t alpha;
        Stream_LoadUInt32(file, &bits);
        Stream_LoadUInt32(file, &red);
        Stream_LoadUInt32(file, &green);
        Stream_LoadUInt32(file, &blue);
        Stream_LoadUInt32(file, &alpha);
        if ((pfFlags & 1) == 0) {   // DDPF_ALPHAPIXELS
            alpha = 0;
        }
        if (DDS_IsSupportedRGB(self, red, green, blue, alpha, bits) == 0) {
            CriticalSection_Unlock(&self->section);
            return 0;
        }
        NiPixelFormat masks;
        *format = *PixelFormat_CreateFromRGBAMasks(&masks, int(bits), red, green, blue, alpha);
        format->flags |= 1;
        self->format = *format;
        *format = *DDS_FindClosestFormat(self, &masks, *format);
    }
    uint32_t caps1;
    Stream_LoadUInt32(file, &caps1);
    if ((caps1 & 8) == 0 && (caps1 & 0x401000) == 0) {
        CriticalSection_Unlock(&self->section);
        return 0;
    }
    uint32_t caps2;
    Stream_LoadUInt32(file, &caps2);
    if ((caps2 & 0x200000) != 0) {   // a volume
        CriticalSection_Unlock(&self->section);
        return 0;
    }
    uint32_t cube = caps2 & 0x200;
    if (cube != 0 && ((caps2 & 0x400) == 0 || (caps2 & 0x800) == 0 || (caps2 & 0x1000) == 0 ||
                      (caps2 & 0x2000) == 0 || (caps2 & 0x4000) == 0 || (caps2 & 0x8000) == 0)) {
        CriticalSection_Unlock(&self->section);
        return 0;
    }
    *faces = cube != 0 ? 6 : 1;
    Skip(file, 0xC);
    *width = self->width;
    *height = self->height;
    if ((caps1 & 8) != 0 && (caps1 & 0x400000) != 0 && mipmapCount != 1) {
        *mipmaps = 1;
        self->mipmapCount = mipmapCount;
    } else {
        *mipmaps = 0;
        self->mipmapCount = 1;
    }
    CriticalSection_Unlock(&self->section);
    return 1;
}

// the start of a level of a face in the pixels
uint8_t* LevelStart(const NiPixelData* pixels, uint32_t level, uint32_t face) {
    return pixels->pixels + pixels->offsets[pixels->mipmapLevels] * face + pixels->offsets[level];
}

// a channel of a pixel to 8 bits, its bits repeated down into the ones it lacks
uint8_t ExpandChannel(uint32_t pixel, uint32_t mask, uint8_t shift, uint8_t loss) {
    uint8_t bits = uint8_t(8 - loss);
    uint32_t value = (pixel & mask) >> (shift & 31);
    uint32_t repeated = 0;
    uint8_t left = loss;
    while (bits <= left) {   // (a channel with no bits never ends: the game's too)
        repeated = (repeated | value) << (bits & 31);
        left = uint8_t(left - bits);
    }
    return uint8_t(value >> ((8 - left - loss) & 31)) | uint8_t(repeated >> (bits & 31)) |
           uint8_t(uint8_t(value) << (loss & 31));
}

// a 24-bit level read in place; with another format whose red is the top byte, red and blue
// swapped
void __cdecl DDSLoad24(NiBinaryStream* file, NiPixelData* pixels, NiPixelFormat source, NiPixelFormat dest,
                       uint32_t level, uint32_t face) {
    uint8_t* p = LevelStart(pixels, level, face);
    Read(file, p, pixels->offsets[level + 1] - pixels->offsets[level], 1);
    ChannelMasks masks;
    ChannelMasks_Construct(&masks, &source);
    uint32_t width = pixels->widths[level];
    uint32_t height = pixels->heights[level];
    if (PixelFormat_NotEquals(&source, &dest) == 0 || masks.mask[0] != 0xFF0000) {
        return;
    }
    uint8_t* in = p;
    uint8_t* out = p;
    for (uint32_t y = height; y != 0; y--) {
        for (uint32_t x = width; x != 0; x--) {
            uint8_t b = in[2];
            uint8_t g = in[1];
            uint8_t r = in[0];
            *out++ = b;
            *out++ = g;
            *out++ = r;
            in += 3;
        }
    }
}

// a 32-bit level read in place and, in another format, each pixel's channels expanded to
// RGBA bytes (alpha 255 when the source has none)
void __cdecl DDSLoad32(NiBinaryStream* file, NiPixelData* pixels, NiPixelFormat source, NiPixelFormat dest,
                       uint32_t level, uint32_t face) {
    uint8_t* p = LevelStart(pixels, level, face);
    Read(file, p, pixels->offsets[level + 1] - pixels->offsets[level], 1);
    ChannelMasks masks;
    ChannelMasks_Construct(&masks, &source);
    uint32_t width = pixels->widths[level];
    uint32_t height = pixels->heights[level];
    if (PixelFormat_NotEquals(&source, &dest) == 0) {
        return;
    }
    bool alpha = false;
    for (int i = 0; i < 4; i++) {
        if (source.components[i].component == 3) {
            alpha = source.components[i].bitsPerComponent != 0;
            break;
        }
    }
    const uint32_t* in = reinterpret_cast<const uint32_t*>(p);
    uint8_t* out = p;
    for (uint32_t y = height; y != 0; y--) {
        for (uint32_t x = width; x != 0; x--) {
            uint32_t pixel = *in++;
            uint8_t r = ExpandChannel(pixel, masks.mask[0], masks.shift[0], masks.loss[0]);
            uint8_t g = ExpandChannel(pixel, masks.mask[1], masks.shift[1], masks.loss[1]);
            uint8_t b = ExpandChannel(pixel, masks.mask[2], masks.shift[2], masks.loss[2]);
            uint8_t a = alpha ? ExpandChannel(pixel, masks.mask[3], masks.shift[3], masks.loss[3]) : uint8_t(0xFF);
            out[0] = r;
            out[1] = g;
            out[2] = b;
            out[3] = a;
            out += 4;
        }
    }
}

// a 16-bit level read 512 bytes at a time and expanded to RGBA32 or RGB24 bytes (any other
// destination: stops after the first block)
void __cdecl DDSLoad16(NiBinaryStream* file, NiPixelData* pixels, NiPixelFormat source, NiPixelFormat dest,
                       uint32_t level, uint32_t face) {
    uint8_t* out = LevelStart(pixels, level, face);
    uint32_t total = pixels->heights[level] * pixels->widths[level] * 2;
    ChannelMasks masks;
    ChannelMasks_Construct(&masks, &source);
    uint8_t buffer[0x200];
    for (uint32_t done = 0; done < total;) {
        uint32_t chunk = total - done < 0x200 ? total - done : 0x200;
        Read(file, buffer, chunk, 1);
        int channels;
        if (PixelFormat_Equals(&dest, &PixelFormat_RGBA32) != 0) {
            channels = 4;
        } else if (PixelFormat_Equals(&dest, &PixelFormat_RGB24) != 0) {
            channels = 3;
        } else {
            return;
        }
        const uint16_t* in = reinterpret_cast<const uint16_t*>(buffer);
        for (uint32_t n = chunk >> 1; n != 0; n--) {
            uint32_t pixel = *in++;
            for (int c = 0; c < channels; c++) {
                *out++ = ExpandChannel(pixel, masks.mask[c], masks.shift[c], masks.loss[c]);
            }
        }
        done += chunk;
    }
}

// vf2 ReadFile: the header, the pixels (the destination when its size, format, levels and
// faces fit); every level of every face read as it is, or converted by the file's pixel size.
// (Faces past the sixth go to the first.)
NiPixelData* __fastcall DDSReadFile(NiDDSReader* self, re::Edx, NiBinaryStream* file, NiPixelData* dest) {
    NiPixelFormat format;
    PixelFormat_Construct(&format);
    sys::Lock(self->section);
    uint32_t width;
    uint32_t height;
    uint8_t mipmaps;
    uint32_t faces;
    if (Virtual<uint8_t>(self, 3, file, &width, &height, &format, &mipmaps, &faces) == 0) {
        sys::Unlock(self->section);
        return nullptr;
    }
    NiPixelData* pixels = dest;
    if (dest == nullptr || dest->widths[0] != self->width || dest->heights[0] != self->height ||
        PixelFormat_Equals(&dest->format, &format) == 0 || dest->mipmapLevels != self->mipmapCount ||
        dest->faces != faces) {
        void* p = NiNew(sizeof(NiPixelData));
        pixels = p != nullptr ? PixelData_Construct(p, self->width, self->height, &format, self->mipmapCount, faces)
                              : nullptr;
    }
    uint32_t levels = pixels->mipmapLevels;
    bool direct = PixelFormat_Equals(&format, &self->format) != 0;
    for (uint32_t f = 0; f < faces; f++) {
        uint32_t face = f <= 5 ? f : 0;
        for (uint32_t level = 0; level < levels; level++) {
            if (direct) {
                Read(file, LevelStart(pixels, level, face), pixels->offsets[level + 1] - pixels->offsets[level], 1);
                continue;
            }
            switch (self->format.bitsPerPixel) {
            case 0x20:
                DDS_Load32(file, pixels, self->format, format, level, face);
                break;
            case 0x18:
                DDS_Load24(file, pixels, self->format, format, level, face);
                break;
            case 0x10:
                DDS_Load16(file, pixels, self->format, format, level, face);
                break;
            default:
                break;
            }
        }
    }
    sys::Unlock(self->section);
    return pixels;
}

// ---- NiBMPReader

uint8_t __fastcall BMPCanReadFile(NiBMPReader*, re::Edx, const char* extension) {
    return NiStricmp(extension, ".bmp") == 0;
}

NiBMPReader* __fastcall BMPConstruct(NiBMPReader* self, re::Edx) {
    ReaderConstruct(self);
    self->vtable = reinterpret_cast<void*>(kNiBMPReaderVtable);
    PixelFormat_Construct(&self->format);
    self->width = 0;
    self->height = 0;
    self->format = PixelFormat_RGB24;
    self->bitCount = 0;
    self->colorsUsed = 0;
    self->dataOffset = 0;
    self->bottomUp = 0;
    self->field7D = 0;
    return self;
}

NiBMPReader* __fastcall BMPDeletingDestructor(NiBMPReader* self, re::Edx, uint32_t flags) {
    ReaderDestruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(NiBMPReader));
    }
    return self;
}

// vf3 ReadHeader: "BM" (read before the lock), the file and info headers (an info header of
// 4 bytes refused); 4 and 8 bits palettized (PAL8), 24 and 32 bits; RLE refused; a negative
// height top-down; no colour count: all of them for palettized. (Its mipmaps output is 1.)
uint8_t __fastcall BMPReadHeader(NiBMPReader* self, re::Edx, NiBinaryStream* file, uint32_t* width, uint32_t* height,
                                 NiPixelFormat* format, uint8_t* mipmaps, uint32_t* faces) {
    SetEndianSwap(file, 0);
    *faces = 1;
    uint16_t magic = 0;
    Read(file, &magic, 2, 2);
    if (magic != 0x4D42) {
        return 0;
    }
    sys::Lock(self->section);
    uint32_t value;
    uint16_t word;
    Read(file, &value, 4, 4);   // the file's size
    Read(file, &word, 2, 2);    // reserved
    Read(file, &word, 2, 2);
    Read(file, &self->dataOffset, 4, 4);
    uint32_t infoSize;
    int32_t infoWidth;
    int32_t infoHeight;
    uint32_t compression;
    Read(file, &infoSize, 4, 4);
    Read(file, &infoWidth, 4, 4);
    Read(file, &infoHeight, 4, 4);
    Read(file, &word, 2, 2);    // planes
    Read(file, &self->bitCount, 2, 2);
    Read(file, &compression, 4, 4);
    Read(file, &value, 4, 4);   // image size
    Read(file, &value, 4, 4);   // pixels per metre
    Read(file, &value, 4, 4);
    Read(file, &self->colorsUsed, 4, 4);
    Read(file, &value, 4, 4);   // colours important
    if (infoSize == 4) {
        sys::Unlock(self->section);
        return 0;
    }
    uint16_t bits = self->bitCount;
    const NiPixelFormat* chosen;
    switch (bits) {
    case 4:
    case 8:
        chosen = &PixelFormat_PAL8;
        break;
    case 0x18:
        chosen = &PixelFormat_RGB24;
        break;
    case 0x20:
        chosen = &PixelFormat_RGBA32;
        break;
    default:
        sys::Unlock(self->section);
        return 0;
    }
    self->format = *chosen;
    if (compression == 2 || compression == 1) {
        sys::Unlock(self->section);
        return 0;
    }
    self->width = uint32_t(infoWidth);
    self->bottomUp = infoHeight >= 0;
    self->height = uint32_t(infoHeight > 0 ? infoHeight : -infoHeight);
    if (self->colorsUsed == 0 && bits != 0x18 && bits != 0x20) {
        self->colorsUsed = 1u << (bits & 31);
    }
    *width = uint32_t(infoWidth);
    *height = self->height;
    *format = self->format;
    *mipmaps = 1;
    CriticalSection_Unlock(&self->section);
    return 1;
}

// vf2 ReadFile: the header; palettized: the palette read (BGRX to RGB with alpha 255) and set
// as a 256-entry NiPalette, else the palette's space skipped; the gap to the pixels skipped;
// the rows read (4 bits: unpacked into a byte a pixel; rows padded to 4 bytes, bottom-up ones
// flipped), then BGR swapped to RGB
NiPixelData* __fastcall BMPReadFile(NiBMPReader* self, re::Edx, NiBinaryStream* file, NiPixelData* dest) {
    NiPixelFormat format;
    PixelFormat_Construct(&format);
    sys::Lock(self->section);
    uint32_t width;
    uint32_t height;
    uint8_t mipmaps;
    uint32_t faces;
    if (Virtual<uint8_t>(self, 3, file, &width, &height, &format, &mipmaps, &faces) == 0) {
        sys::Unlock(self->section);
        return nullptr;
    }
    NiPixelData* pixels = dest;
    if (dest == nullptr || dest->widths[0] != self->width || dest->heights[0] != self->height ||
        PixelFormat_Equals(&dest->format, &self->format) == 0) {
        void* p = NiNew(sizeof(NiPixelData));
        pixels = p != nullptr ? PixelData_Construct(p, self->width, self->height, &self->format, 1, 1) : nullptr;
    }
    if (PixelFormat_Equals(&self->format, &PixelFormat_PAL8) != 0) {
        uint8_t entries[0x400];   // (more than 256 colours overrun it, as in the game)
        Read(file, entries, self->colorsUsed * 4, 1);
        uint8_t* entry = entries;
        for (uint32_t i = self->colorsUsed; i != 0; i--) {
            uint8_t b = entry[0];
            entry[0] = entry[2];
            entry[2] = b;
            entry[3] = 0xFF;
            entry += 4;
        }
        void* p = NiNew(0x24);
        NiObject* palette = p != nullptr ? Palette_Construct(p, 0, 0x100, entries) : nullptr;
        NiObject* old = pixels->palette;
        if (old != palette) {
            if (old != nullptr && sys::Decrement(&old->refCount) == 0) {
                Virtual(old, 1);
            }
            pixels->palette = palette;
            if (palette != nullptr) {
                sys::Increment(&palette->refCount);
            }
        }
    } else {
        Skip(file, int(self->colorsUsed * 4));
    }
    int32_t gap = int32_t(self->dataOffset - self->colorsUsed * 4 - 0x36);
    if (gap > 0) {
        Skip(file, gap);
    }
    uint32_t rowBytes = pixels->widths[0] * pixels->pixelStride;
    uint32_t total = ((uint32_t(self->bitCount) * self->width + 0x1F) >> 3 & 0x1FFFFFFC) * self->height;
    if (self->bitCount == 4) {
        uint32_t half = (self->width >> 1) + (self->width & 1);
        uint32_t padded = half + ((half & 3) != 0 ? 4 - (half & 3) : 0);
        auto* buffer = static_cast<uint8_t*>(NiMalloc(padded));
        uint8_t* dst;
        int32_t stride = int32_t(rowBytes);
        if (self->bottomUp != 0) {
            dst = (pixels->heights[0] - 1) * rowBytes + pixels->offsets[0] + pixels->pixels;
            stride = -stride;
        } else {
            dst = pixels->offsets[0] + pixels->pixels;
        }
        for (uint32_t row = 0; row < self->height; row++) {
            Read(file, buffer, padded, 1);
            uint8_t* out = dst;
            const uint8_t* in = buffer;
            for (uint32_t i = half; i != 0; i--) {
                out[0] = uint8_t(*in >> 4);
                out[1] = uint8_t(*in & 0xF);
                out += 2;
                in++;
            }
            dst += stride;
        }
        NiFree(buffer);
    } else if (self->bottomUp != 0) {
        uint8_t* dst = (self->height - 1) * rowBytes + pixels->offsets[0] + pixels->pixels;
        uint32_t pad = (rowBytes & 3) != 0 ? 4 - (rowBytes & 3) : 0;
        uint32_t scratch;
        for (uint32_t row = 0; row < self->height; row++) {
            Read(file, dst, rowBytes, 1);
            if (pad != 0) {
                Read(file, &scratch, pad, 1);
            }
            dst -= rowBytes;
        }
    } else if ((rowBytes & 3) != 0) {
        uint32_t pad = 4 - (rowBytes & 3);
        uint8_t* dst = pixels->offsets[0] + pixels->pixels;
        uint32_t scratch;
        for (uint32_t row = 0; row < self->height; row++) {
            Read(file, dst, rowBytes, 1);
            Read(file, &scratch, pad, 1);
            dst += rowBytes;
        }
    } else {
        Read(file, pixels->offsets[0] + pixels->pixels, total, 1);
    }
    if (PixelFormat_Equals(&self->format, &PixelFormat_RGB24) != 0 ||
        PixelFormat_Equals(&self->format, &PixelFormat_RGBA32) != 0) {
        uint8_t* p = pixels->offsets[0] + pixels->pixels;
        uint32_t stride = pixels->pixelStride;
        for (uint32_t n = self->height * self->width; n != 0; n--) {
            uint8_t r = p[0];
            p[0] = p[2];
            p[2] = r;
            p += stride;
        }
    }
    sys::Unlock(self->section);
    return pixels;
}

} // namespace

// 0x401000 (thiscall): NiCriticalSection::Unlock, the readers' (their lock count down, the owner
// cleared at 0, the section left). The game's first function: without a body until the 64-bit
// build read DDS files (a mod's textures, 2026-11-01)
void __fastcall CriticalSection_UnlockImpl(NiCriticalSection* self, re::Edx) {
    sys::Unlock(*self);
}

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIIMAGEREADERS_REVERSED(name, address, fn)                                             \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIIMAGEREADERS_REVERSED("NiCriticalSection::Unlock", 0x00401000, &ni::CriticalSection_UnlockImpl);
NIIMAGEREADERS_REVERSED("NiTGAReader::RowPal4", 0x008CD2F0, &ni::TGARowPal4);
NIIMAGEREADERS_REVERSED("NiTGAReader::RowPal8", 0x008CD330, &ni::TGARowPal8);
NIIMAGEREADERS_REVERSED("NiTGAReader::RowGrey8", 0x008CD360, &ni::TGARowGrey8);
NIIMAGEREADERS_REVERSED("NiTGAReader::Row16", 0x008CD3A0, &ni::TGARow16);
NIIMAGEREADERS_REVERSED("NiTGAReader::RowPal16", 0x008CD400, &ni::TGARowPal16);
NIIMAGEREADERS_REVERSED("NiTGAReader::RowPal16Alpha", 0x008CD460, &ni::TGARowPal16Alpha);
NIIMAGEREADERS_REVERSED("NiTGAReader::Row24", 0x008CD4D0, &ni::TGARow24);
NIIMAGEREADERS_REVERSED("NiTGAReader::Row32", 0x008CD520, &ni::TGARow32);
NIIMAGEREADERS_REVERSED("NiTGAReader::CanReadFile", 0x008CD570, &ni::TGACanReadFile);
NIIMAGEREADERS_REVERSED("NiStreamLoadBinary(uint16)", 0x008CD5B0, &ni::StreamLoadUInt16);
NIIMAGEREADERS_REVERSED("NiTGAReader::NiTGAReader", 0x008CD5E0, &ni::TGAConstruct);
NIIMAGEREADERS_REVERSED("NiTGAReader::DeletingDestructor", 0x008CD6B0, &ni::TGADeletingDestructor);
NIIMAGEREADERS_REVERSED("NiTGAReader::ReadRLE", 0x008CD710, &ni::TGAReadRLE);
NIIMAGEREADERS_REVERSED("NiTGAReader::ReadPalette", 0x008CD870, &ni::TGAReadPalette);
NIIMAGEREADERS_REVERSED("NiTGAReader::ReadHeader", 0x008CDAC0, &ni::TGAReadHeader);
NIIMAGEREADERS_REVERSED("NiTGAReader::ReadFile", 0x008CDEA0, &ni::TGAReadFile);
NIIMAGEREADERS_REVERSED("NiSGIReader::ExpandRLE8", 0x008CE150, &ni::SGIExpandRLE8);
NIIMAGEREADERS_REVERSED("NiSGIReader::ExpandRLE16", 0x008CE1E0, &ni::SGIExpandRLE16);
NIIMAGEREADERS_REVERSED("NiSGIReader::CanReadFile", 0x008CE270, &ni::SGICanReadFile);
NIIMAGEREADERS_REVERSED("NiStreamLoadBinary(uint32[])", 0x008CE2E0, &ni::StreamLoadUInt32s);
NIIMAGEREADERS_REVERSED("NiSGIReader::NiSGIReader", 0x008CE320, &ni::SGIConstruct);
NIIMAGEREADERS_REVERSED("NiSGIReader::DeletingDestructor", 0x008CE380, &ni::SGIDeletingDestructor);
NIIMAGEREADERS_REVERSED("NiSGIReader::ReadHeader", 0x008CE3B0, &ni::SGIReadHeader);
NIIMAGEREADERS_REVERSED("NiSGIReader::ReadFile", 0x008CE610, &ni::SGIReadFile);
NIIMAGEREADERS_REVERSED("NiDDSReader::CanReadFile", 0x008CEB40, &ni::DDSCanReadFile);
NIIMAGEREADERS_REVERSED("NiDDSReader::IsSupportedRGB", 0x008CEB60, &ni::DDSIsSupportedRGB);
NIIMAGEREADERS_REVERSED("NiDDSReader::FindClosestFormat", 0x008CEB80, &ni::DDSFindClosestFormat);
NIIMAGEREADERS_REVERSED("NiDDSReader::NiDDSReader", 0x008CEBF0, &ni::DDSConstruct);
NIIMAGEREADERS_REVERSED("NiDDSReader::DeletingDestructor", 0x008CEC40, &ni::DDSDeletingDestructor);
NIIMAGEREADERS_REVERSED("NiDDSReader::ReadHeader", 0x008CEC70, &ni::DDSReadHeader);
NIIMAGEREADERS_REVERSED("NiDDSReader::Load24", 0x008CF1D0, &ni::DDSLoad24);
NIIMAGEREADERS_REVERSED("NiDDSReader::Load32", 0x008CF2A0, &ni::DDSLoad32);
NIIMAGEREADERS_REVERSED("NiDDSReader::Load16", 0x008CF730, &ni::DDSLoad16);
NIIMAGEREADERS_REVERSED("NiDDSReader::ReadFile", 0x008CFBD0, &ni::DDSReadFile);
NIIMAGEREADERS_REVERSED("NiBMPReader::CanReadFile", 0x008CFF40, &ni::BMPCanReadFile);
NIIMAGEREADERS_REVERSED("NiBMPReader::NiBMPReader", 0x008CFF60, &ni::BMPConstruct);
NIIMAGEREADERS_REVERSED("NiBMPReader::DeletingDestructor", 0x008CFFC0, &ni::BMPDeletingDestructor);
NIIMAGEREADERS_REVERSED("NiBMPReader::ReadHeader", 0x008CFFF0, &ni::BMPReadHeader);
NIIMAGEREADERS_REVERSED("NiBMPReader::ReadFile", 0x008D0350, &ni::BMPReadFile);
