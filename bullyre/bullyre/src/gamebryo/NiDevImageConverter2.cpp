// NiDevImageConverter's pieces in chunk 44 (the class: src/gamebryo/NiDevImageConverter.h and .cpp):
// the RGB row converters ConvertFormat picks (cdecl: width, height, the destination's bytes, the
// destination, the source's palette, the destination's channel masks, the source, the source's
// channel masks), the channel masks' constructor, the destructor, the image files read through
// the readers, and the formats it can convert between. A row converter moves each channel
// through 8 bits: down by the source's shift, up by its lost bits (an 8-bit shift), down by the
// destination's lost bits, up by its shift, masked.

#include "gamebryo/NiD3D10C44.h"
#include "gamebryo/NiFile.h"
#include "reversed.h"

namespace ni::c44 {
namespace {

constexpr uintptr_t kConverterVtable = 0x009A18C0;
constexpr uintptr_t kReaderListVtable = 0x00952E10;
constexpr uintptr_t kListAllocatorVtable = 0x0099C748;

// the formats (NiPixelFormat statics)
constexpr uintptr_t kPAL4 = 0x00BC7630, kPALA4 = 0x00BC7678, kPAL8 = 0x00BC76C0, kPALA8 = 0x00BC7708;
constexpr uintptr_t kRGBA32 = 0x00BC7750, kRGB24 = 0x00BC7798, kRGBA5551 = 0x00BC77E0, kBGRA5551 = 0x00BC7828;
constexpr uintptr_t kBUMP16 = 0x00BC7870, kBUMPLUMA32 = 0x00BC78B8;
constexpr uintptr_t kBGRA8888 = 0x00BC8200, kARGB8888 = 0x00BC8248;

GAME_FN(uint32_t, __thiscall, PixelFormat_GetMask, 0x007548E0, const void* self, int component);
GAME_FN(uint8_t, __thiscall, PixelFormat_GetShift, 0x00754980, const void* self, int component);
GAME_FN(uint8_t, __thiscall, PixelFormat_Equals, 0x007D3AB0, const void* self, const void* other);
GAME_FN(uint8_t, __thiscall, PixelFormat_NotEquals, 0x0076A090, const void* self, const void* other);
GAME_FN(uint8_t, __thiscall, PixelFormat_EqualsIgnoringEndian, 0x0076AE80, const void* self, const void* other);
GAME_FN(uint8_t, __thiscall, PixelFormat_IsCompressed, 0x005F0A20, const void* self);
GAME_FN(void, __thiscall, PointerList_RemoveAll, 0x0075DB60, void* list);
GAME_FN(void, __thiscall, Decoder_Destruct, 0x0049A650, void* self);   // (a lone ret)
GAME_FN(void, __thiscall, ImageConverter_Destruct, 0x007D3B80, void* self);
GAME_FN(uint8_t, __cdecl, Path_Standardize, 0x00752C20, char* path);
GAME_FN(void, __thiscall, Filename_Construct, 0x007538C0, NiFilename* self, const char* path);
GAME_FN(void*, __cdecl, File_GetFile, 0x00753380, const char* path, int mode, uint32_t bufferSize);
// this file's (called where modes apply)
GAME_FN(void*, __thiscall, ChannelMasks_Construct, 0x008C6820, void* self, const void* format);

// a format's channels (red, green, blue, alpha): masks, shifts, and the bits short of 8
struct Masks {
    uint32_t mask[4];   // +0x0
    uint8_t shift[4];   // +0x10
    uint8_t loss[4];    // +0x14
};
static_assert(sizeof(Masks) == 0x18);

// a source channel as 8 bits
inline uint8_t Unpack(uint32_t value, const Masks* m, int c) {
    uint8_t bits = uint8_t((value & m->mask[c]) >> (m->shift[c] & 31));
    return uint8_t(uint32_t(bits) << (m->loss[c] & 31));
}
// 8 bits into a destination channel
inline uint32_t Pack(uint8_t value, const Masks* m, int c) {
    return (uint32_t(uint8_t(value >> (m->loss[c] & 31))) << (m->shift[c] & 31)) & m->mask[c];
}
inline const void* Format(uintptr_t address) {
    return reinterpret_cast<const void*>(address);
}
inline bool Is(const void* format, uintptr_t other) {
    return PixelFormat_Equals(format, Format(other)) != 0;
}
// a reader's virtual (1 CanReadFile(extension), 2 ReadFile(file, data), 3 ReadHeader)
inline uint8_t CanRead(void* reader, const NiFilename& filename) {
    return Virtual<uint8_t>(reader, 1, filename.ext);
}

} // namespace

// 0x8C5A50 (thiscall, slot 7): whether a format reads as RGBA32 or RGB24 (slots 8, 9)
uint8_t __fastcall Converter_IsReadableFormat(void* self, re::Edx, const void* format) {
    if (Virtual<uint8_t>(self, 8, format) || Virtual<uint8_t>(self, 9, format)) {
        return 1;
    }
    return 0;
}

// 0x8C5A80 (cdecl): 16 bits to 16 bits by the masks (RGBA5551 <-> BGRA5551)
void __cdecl Convert16To16(uint32_t width, uint32_t height, uint32_t, uint16_t* dst, void*, const Masks* dstMasks,
                           const uint16_t* src, const Masks* srcMasks) {
    for (uint32_t y = height; y != 0; y--) {
        for (uint32_t x = width; x != 0; x--) {
            uint32_t v = *src++;
            *dst++ = uint16_t(Pack(Unpack(v, srcMasks, 3), dstMasks, 3) | Pack(Unpack(v, srcMasks, 2), dstMasks, 2) |
                              Pack(Unpack(v, srcMasks, 0), dstMasks, 0) | Pack(Unpack(v, srcMasks, 1), dstMasks, 1));
        }
    }
}

// 0x8C5BB0 (cdecl): RGB24 to 16 bits (opaque)
void __cdecl ConvertRGB24To16(uint32_t width, uint32_t height, uint32_t, uint16_t* dst, void*, const Masks* m,
                              const uint8_t* src, const Masks*) {
    for (uint32_t y = height; y != 0; y--) {
        for (uint32_t x = width; x != 0; x--) {
            uint8_t r = src[0];
            uint8_t g = src[1];
            uint8_t b = src[2];
            src += 3;
            *dst++ = uint16_t(Pack(0xFF, m, 3) | Pack(r, m, 0) | Pack(b, m, 2) | Pack(g, m, 1));
        }
    }
}

// 0x8C5C90 (cdecl): RGB24 to 24 bits (swapped to BGR, or copied; nothing for other masks)
void __cdecl ConvertRGB24To24(uint32_t width, uint32_t height, uint32_t size, uint8_t* dst, void*, const Masks* m,
                              const uint8_t* src, const Masks*) {
    if (m->mask[1] != 0xFF00) {
        return;
    }
    if (m->mask[0] == 0xFF0000) {
        for (uint32_t y = height; y != 0; y--) {
            for (uint32_t x = width; x != 0; x--) {
                dst[0] = src[2];
                dst[1] = src[1];
                dst[2] = src[0];
                dst += 3;
                src += 3;
            }
        }
    } else if (m->mask[0] == 0xFF) {
        crt::MemcpyS(dst, size, src, width * height * 3);
    }
}

// 0x8C5D20 (cdecl): RGB24 to 32 bits (opaque)
void __cdecl ConvertRGB24To32(uint32_t width, uint32_t height, uint32_t, uint32_t* dst, void*, const Masks* m,
                              const uint8_t* src, const Masks*) {
    for (uint32_t y = height; y != 0; y--) {
        for (uint32_t x = width; x != 0; x--) {
            uint8_t r = src[0];
            uint8_t g = src[1];
            uint8_t b = src[2];
            src += 3;
            *dst++ = Pack(r, m, 0) | Pack(g, m, 1) | Pack(b, m, 2) | Pack(0xFF, m, 3);
        }
    }
}

// 0x8C5DE0 (cdecl): RGBA32 to 16 bits
void __cdecl ConvertRGBA32To16(uint32_t width, uint32_t height, uint32_t, uint16_t* dst, void*, const Masks* m,
                               const uint8_t* src, const Masks*) {
    for (uint32_t y = height; y != 0; y--) {
        for (uint32_t x = width; x != 0; x--) {
            uint8_t r = src[0];
            uint8_t g = src[1];
            uint8_t b = src[2];
            uint8_t a = src[3];
            src += 4;
            *dst++ = uint16_t(Pack(g, m, 1) | Pack(r, m, 0) | Pack(a, m, 3) | Pack(b, m, 2));
        }
    }
}

// 0x8C5EC0 (cdecl): RGBA32 to 24 bits (swapped to BGR, or in order; nothing for other masks)
void __cdecl ConvertRGBA32To24(uint32_t width, uint32_t height, uint32_t, uint8_t* dst, void*, const Masks* m,
                               const uint8_t* src, const Masks*) {
    if (m->mask[1] != 0xFF00) {
        return;
    }
    if (m->mask[0] == 0xFF0000) {
        for (uint32_t y = height; y != 0; y--) {
            for (uint32_t x = width; x != 0; x--) {
                dst[0] = src[2];
                dst[1] = src[1];
                dst[2] = src[0];
                dst += 3;
                src += 4;
            }
        }
    } else if (m->mask[0] == 0xFF) {
        for (uint32_t y = height; y != 0; y--) {
            for (uint32_t x = width; x != 0; x--) {
                dst[0] = src[0];
                dst[1] = src[1];
                dst[2] = src[2];
                dst += 3;
                src += 4;
            }
        }
    }
}

// 0x8C5F90 (cdecl): 32 bits to 32 bits by the masks
void __cdecl Convert32To32(uint32_t width, uint32_t height, uint32_t, uint32_t* dst, void*, const Masks* dstMasks,
                           const uint32_t* src, const Masks* srcMasks) {
    for (uint32_t y = height; y != 0; y--) {
        for (uint32_t x = width; x != 0; x--) {
            uint32_t v = *src++;
            *dst++ = Pack(Unpack(v, srcMasks, 0), dstMasks, 0) | Pack(Unpack(v, srcMasks, 1), dstMasks, 1) |
                     Pack(Unpack(v, srcMasks, 3), dstMasks, 3) | Pack(Unpack(v, srcMasks, 2), dstMasks, 2);
        }
    }
}

// 0x8C60A0 (cdecl): 16 bits to RGB24
void __cdecl Convert16ToRGB24(uint32_t width, uint32_t height, uint32_t, uint8_t* dst, void*, const Masks*,
                              const uint16_t* src, const Masks* s) {
    for (uint32_t y = height; y != 0; y--) {
        for (uint32_t x = width; x != 0; x--) {
            uint32_t v = *src++;
            dst[0] = Unpack(v, s, 0);
            dst[1] = Unpack(v, s, 1);
            dst[2] = Unpack(v, s, 2);
            dst += 3;
        }
    }
}

// 0x8C6130 (cdecl): 16 bits with alpha to RGBA32
void __cdecl Convert16AlphaToRGBA32(uint32_t width, uint32_t height, uint32_t, uint8_t* dst, void*, const Masks*,
                                    const uint16_t* src, const Masks* s) {
    for (uint32_t y = height; y != 0; y--) {
        for (uint32_t x = width; x != 0; x--) {
            uint32_t v = *src++;
            dst[0] = Unpack(v, s, 0);
            dst[1] = Unpack(v, s, 1);
            dst[2] = Unpack(v, s, 2);
            dst[3] = Unpack(v, s, 3);
            dst += 4;
        }
    }
}

// 0x8C61E0 (cdecl): 16 bits without alpha to RGBA32 (opaque)
void __cdecl Convert16ToRGBA32(uint32_t width, uint32_t height, uint32_t, uint8_t* dst, void*, const Masks*,
                               const uint16_t* src, const Masks* s) {
    for (uint32_t y = height; y != 0; y--) {
        for (uint32_t x = width; x != 0; x--) {
            uint32_t v = *src++;
            dst[0] = Unpack(v, s, 0);
            dst[1] = Unpack(v, s, 1);
            dst[2] = Unpack(v, s, 2);
            dst[3] = 0xFF;
            dst += 4;
        }
    }
}

// 0x8C6270 (cdecl): 24 bits to RGB24 (by the source's masks: swapped from BGR, or copied)
void __cdecl Convert24ToRGB24(uint32_t width, uint32_t height, uint32_t size, uint8_t* dst, void*, const Masks*,
                              const uint8_t* src, const Masks* s) {
    if (s->mask[1] != 0xFF00) {
        return;
    }
    if (s->mask[0] == 0xFF0000) {
        for (uint32_t y = height; y != 0; y--) {
            for (uint32_t x = width; x != 0; x--) {
                dst[0] = src[2];
                dst[1] = src[1];
                dst[2] = src[0];
                dst += 3;
                src += 3;
            }
        }
    } else if (s->mask[0] == 0xFF) {
        crt::MemcpyS(dst, size, src, width * height * 3);
    }
}

// 0x8C6300 (cdecl): 24 bits to RGBA32 (swapped from BGR, else in order; opaque)
void __cdecl Convert24ToRGBA32(uint32_t width, uint32_t height, uint32_t, uint8_t* dst, void*, const Masks*,
                               const uint8_t* src, const Masks* s) {
    if (s->mask[1] != 0xFF00) {
        return;
    }
    bool swap = s->mask[0] == 0xFF0000;
    for (uint32_t y = height; y != 0; y--) {
        for (uint32_t x = width; x != 0; x--) {
            if (swap) {
                dst[0] = src[2];
                dst[1] = src[1];
                dst[2] = src[0];
            } else {
                dst[0] = src[0];
                dst[1] = src[1];
                dst[2] = src[2];
            }
            dst[3] = 0xFF;
            dst += 4;
            src += 3;
        }
    }
}

// 0x8C63D0 (cdecl): 32 bits to RGB24
void __cdecl Convert32ToRGB24(uint32_t width, uint32_t height, uint32_t, uint8_t* dst, void*, const Masks*,
                              const uint32_t* src, const Masks* s) {
    for (uint32_t y = height; y != 0; y--) {
        for (uint32_t x = width; x != 0; x--) {
            uint32_t v = *src++;
            dst[0] = Unpack(v, s, 0);
            dst[1] = Unpack(v, s, 1);
            dst[2] = Unpack(v, s, 2);
            dst += 3;
        }
    }
}

// 0x8C6450 (cdecl): 32 bits with alpha to RGBA32
void __cdecl Convert32AlphaToRGBA32(uint32_t width, uint32_t height, uint32_t, uint8_t* dst, void*, const Masks*,
                                    const uint32_t* src, const Masks* s) {
    for (uint32_t y = height; y != 0; y--) {
        for (uint32_t x = width; x != 0; x--) {
            uint32_t v = *src++;
            dst[0] = Unpack(v, s, 0);
            dst[1] = Unpack(v, s, 1);
            dst[2] = Unpack(v, s, 2);
            dst[3] = Unpack(v, s, 3);
            dst += 4;
        }
    }
}

// 0x8C6500 (cdecl): 32 bits without alpha to RGBA32 (opaque)
void __cdecl Convert32ToRGBA32(uint32_t width, uint32_t height, uint32_t, uint8_t* dst, void*, const Masks*,
                               const uint32_t* src, const Masks* s) {
    for (uint32_t y = height; y != 0; y--) {
        for (uint32_t x = width; x != 0; x--) {
            uint32_t v = *src++;
            dst[0] = Unpack(v, s, 0);
            dst[1] = Unpack(v, s, 1);
            dst[2] = Unpack(v, s, 2);
            dst[3] = 0xFF;
            dst += 4;
        }
    }
}

// 0x8C6590 (cdecl): BUMPLUMA32 to 32 bits (du to red, dv to green, the luminance to alpha)
void __cdecl ConvertBumpLuma32To32(uint32_t width, uint32_t height, uint32_t, uint32_t* dst, void*, const Masks* m,
                                   const uint8_t* src, const Masks*) {
    for (uint32_t y = height; y != 0; y--) {
        for (uint32_t x = width; x != 0; x--) {
            uint8_t du = src[0];
            uint8_t dv = src[1];
            uint8_t luma = src[2];
            src += 4;
            *dst++ = Pack(du, m, 0) | Pack(luma, m, 3) | Pack(dv, m, 1);
        }
    }
}

// 0x8C6630 (cdecl): BUMPLUMA32 to 16 bits
void __cdecl ConvertBumpLuma32To16(uint32_t width, uint32_t height, uint32_t, uint16_t* dst, void*, const Masks* m,
                                   const uint8_t* src, const Masks*) {
    for (uint32_t y = height; y != 0; y--) {
        for (uint32_t x = width; x != 0; x--) {
            uint8_t du = src[0];
            uint8_t dv = src[1];
            uint8_t luma = src[2];
            src += 4;
            *dst++ = uint16_t(Pack(luma, m, 3) | Pack(dv, m, 1) | Pack(du, m, 0));
        }
    }
}

// 0x8C66F0 (cdecl): BUMP16 to 32 bits (the alpha bits all set: from all 32 bits shifted)
void __cdecl ConvertBump16To32(uint32_t width, uint32_t height, uint32_t, uint32_t* dst, void*, const Masks* m,
                               const uint8_t* src, const Masks*) {
    for (uint32_t y = height; y != 0; y--) {
        for (uint32_t x = width; x != 0; x--) {
            uint8_t du = src[0];
            uint8_t dv = src[1];
            src += 2;
            uint32_t alpha = ((0xFFFFFFFFu >> (m->loss[3] & 31)) << (m->shift[3] & 31)) & m->mask[3];
            *dst++ = Pack(du, m, 0) | Pack(dv, m, 1) | alpha;
        }
    }
}

// 0x8C6780 (cdecl): BUMP16 to 16 bits
void __cdecl ConvertBump16To16(uint32_t width, uint32_t height, uint32_t, uint16_t* dst, void*, const Masks* m,
                               const uint8_t* src, const Masks*) {
    for (uint32_t y = height; y != 0; y--) {
        for (uint32_t x = width; x != 0; x--) {
            uint8_t du = src[0];
            uint8_t dv = src[1];
            src += 2;
            uint32_t alpha = ((0xFFFFFFFFu >> (m->loss[3] & 31)) << (m->shift[3] & 31)) & m->mask[3];
            *dst++ = uint16_t(Pack(du, m, 0) | Pack(dv, m, 1) | alpha);
        }
    }
}

// 0x8C6820 (thiscall): the channel masks of a format (each channel's mask and shift; its lost
// bits: 8 less the bits of the first component of that channel, 8 with none)
Masks* __fastcall ChannelMasks_ConstructImpl(Masks* self, re::Edx, const uint8_t* format) {
    for (int c = 0; c < 4; c++) {
        self->mask[c] = PixelFormat_GetMask(format, c);
        self->shift[c] = PixelFormat_GetShift(format, c);
        uint8_t bits = 0;
        for (uint32_t i = 0; i < 4; i++) {
            if (At<int32_t>(format, 0x14 + int(i) * 0xC) == c) {
                bits = At<uint8_t>(format, 0x1C + int(i) * 0xC);
                break;
            }
        }
        self->loss[c] = uint8_t(8 - bits);
    }
    return self;
}

// 0x8C6950 (thiscall): the destructor (each reader deleted with its node; the list's, the
// decoder's and NiImageConverter's destructors)
void __fastcall Converter_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kConverterVtable;
    uint8_t* list = self + 0x648;
    while (At<uint32_t>(list, 0xC) != 0) {
        uint8_t* node = At<uint8_t*>(list, 4);
        uint8_t* next = At<uint8_t*>(node, 0);
        At<uint8_t*>(list, 4) = next;
        if (next != nullptr) {
            At<uint8_t*>(next, 4) = nullptr;
        } else {
            At<uint8_t*>(list, 8) = nullptr;
        }
        void* reader = At<void*>(node, 8);
        Virtual<void>(list, 2, node);   // the node freed
        At<uint32_t>(list, 0xC) -= 1;
        if (reader != nullptr) {
            Virtual<void>(reader, 0, 1);
        }
    }
    At<uint32_t>(list, 0) = kReaderListVtable;
    PointerList_RemoveAll(list);
    At<uint32_t>(list, 0) = kListAllocatorVtable;
    Decoder_Destruct(self + 0x438);
    ImageConverter_Destruct(self);
}

// 0x8C69E0 (thiscall): the reader list
void* __fastcall Converter_GetReaders(uint8_t* self, re::Edx) {
    return self + 0x648;
}

// 0x8C69F0 (thiscall, slot 6): whether a file can be read (the first reader taking its extension,
// and the file opening)
uint8_t __fastcall Converter_CanReadFile(uint8_t* self, re::Edx, const char* path) {
    re::Ptr32<void>* node = At<re::Ptr32<void>*>(self, 0x64C);
    char buffer[0x104];
    crt::StrcpyS(buffer, 0x104, path);
    Path_Standardize(buffer);
    NiFilename filename;
    Filename_Construct(&filename, buffer);
    while (node != nullptr) {
        void* reader = node[2];
        node = static_cast<re::Ptr32<void>*>(node[0]);
        if (!CanRead(reader, filename)) {
            continue;
        }
        void* file = File_GetFile(buffer, 0, 0x8000);
        if (file == nullptr) {
            return 0;
        }
        uint8_t good = Virtual<uint8_t>(file, 1);   // operator bool
        Virtual<void>(file, 0, 1);
        return good != 0 ? 1 : 0;
    }
    return 0;
}

// 0x8C6AE0 (thiscall, slot 2): ReadImageFileInfo (each reader taking the extension tried in turn:
// its header read)
uint8_t __fastcall Converter_ReadImageFileInfo(uint8_t* self, re::Edx, const char* path, void* format, void* mipmap,
                                               uint32_t* width, uint32_t* height, uint32_t* faces) {
    re::Ptr32<void>* node = At<re::Ptr32<void>*>(self, 0x64C);
    char buffer[0x104];
    crt::StrcpyS(buffer, 0x104, path);
    Path_Standardize(buffer);
    NiFilename filename;
    Filename_Construct(&filename, buffer);
    while (node != nullptr) {
        void* reader = node[2];
        node = static_cast<re::Ptr32<void>*>(node[0]);
        if (!CanRead(reader, filename)) {
            continue;
        }
        void* file = File_GetFile(buffer, 0, 0x8000);
        if (file == nullptr) {
            return 0;
        }
        if (!Virtual<uint8_t>(file, 1)) {
            Virtual<void>(file, 0, 1);
            return 0;
        }
        uint8_t read = Virtual<uint8_t>(reader, 3, file, width, height, format, mipmap, faces);   // ReadHeader
        Virtual<void>(file, 0, 1);
        if (read != 0) {
            return 1;
        }
    }
    return 0;
}

// 0x8C6C20 (thiscall, slot 3): ReadImageFile (each reader taking the extension tried in turn;
// the pixel data, or null)
void* __fastcall Converter_ReadImageFile(uint8_t* self, re::Edx, const char* path, void* existing) {
    re::Ptr32<void>* node = At<re::Ptr32<void>*>(self, 0x64C);
    char buffer[0x104];
    crt::StrcpyS(buffer, 0x104, path);
    Path_Standardize(buffer);
    NiFilename filename;
    Filename_Construct(&filename, buffer);
    while (node != nullptr) {
        void* reader = node[2];
        node = static_cast<re::Ptr32<void>*>(node[0]);
        if (!CanRead(reader, filename)) {
            continue;
        }
        void* file = File_GetFile(buffer, 0, 0x8000);
        if (file == nullptr) {
            return nullptr;
        }
        if (!Virtual<uint8_t>(file, 1)) {
            Virtual<void>(file, 0, 1);
            return nullptr;
        }
        void* data = Virtual<void*>(reader, 2, file, existing);   // ReadFile
        Virtual<void>(file, 0, 1);
        if (data != nullptr) {
            return data;
        }
    }
    return nullptr;
}

// 0x8C6D20 (thiscall, slot 4): CanConvertPixelData (source, destination formats)
uint8_t __fastcall Converter_CanConvertPixelData(void*, re::Edx, const uint8_t* source, const uint8_t* dest) {
    int32_t code = At<int32_t>(dest, 4);
    uint32_t bits = At<uint8_t>(dest, 1);
    Masks masks;
    ChannelMasks_Construct(&masks, dest);
    if (PixelFormat_Equals(source, dest)) {
        return 1;
    }
    if (At<int32_t>(dest, 4) >= 4 && At<int32_t>(dest, 4) <= 6) {
        return 0;
    }
    if (((source[0] ^ dest[0]) & 1) != 0 && PixelFormat_EqualsIgnoringEndian(source, dest)) {
        return 1;
    }
    if (((source[0] ^ dest[0]) & 1) != 0) {
        return 0;
    }
    if (bits == 0x18) {
        if (masks.mask[1] != 0xFF00) {
            return 0;
        }
        if (masks.mask[0] != 0xFF && masks.mask[0] != 0xFF0000) {
            return 0;
        }
    }
    bool toRGB = code == 0 || code == 1;
    if (Is(source, kPAL4) || Is(source, kPALA4)) {
        if (toRGB) {
            return bits == 0x18 || bits == 0x20 ? 1 : 0;
        }
        if (!PixelFormat_NotEquals(dest, Format(kPAL4))) {
            return 1;
        }
        return PixelFormat_NotEquals(dest, Format(kPALA4)) ? 0 : 1;
    }
    if (Is(source, kPAL8) || Is(source, kPALA8)) {
        if (toRGB) {
            return bits == 0x10 || bits == 0x18 || bits == 0x20 ? 1 : 0;
        }
        if (!PixelFormat_NotEquals(dest, Format(kPAL8))) {
            return 1;
        }
        return PixelFormat_NotEquals(dest, Format(kPALA8)) ? 0 : 1;
    }
    if (Is(source, kRGB24) || Is(source, kRGBA32) || PixelFormat_IsCompressed(source)) {
        if (!toRGB && code != 8 && code != 9) {
            return 0;
        }
        return bits == 0x10 || bits == 0x18 || bits == 0x20 ? 1 : 0;
    }
    if (Is(source, kBUMP16) || Is(source, kBUMPLUMA32)) {
        if (code != 8 && code != 9) {
            return 0;
        }
        return bits == 0x10 || bits == 0x18 || bits == 0x20 ? 1 : 0;
    }
    if (Is(source, kRGBA5551) || Is(source, kBGRA5551)) {
        if (bits != 0x10 || masks.mask[3] != 0x8000 || masks.mask[1] != 0x3E0) {
            return 0;
        }
        if (masks.mask[0] == 0x1F) {
            return masks.mask[2] == 0x7C00 ? 1 : 0;
        }
        if (masks.mask[0] != 0x7C00) {
            return 0;
        }
        return masks.mask[2] == 0x1F ? 1 : 0;
    }
    if (!Is(dest, kBGRA8888) && !Is(dest, kARGB8888) && !Is(dest, kRGBA32)) {
        return 0;
    }
    if (Is(source, kBGRA8888) || Is(source, kARGB8888) || Is(source, kRGBA32)) {
        return 1;
    }
    return 0;
}

// 0x8C7000 (slot 8): whether a format reads as RGBA32 (PALA4, PALA8, RGBA32)
uint8_t __fastcall Converter_ReadsAsRGBA32(void*, re::Edx, const void* format) {
    return Is(format, kPALA4) || Is(format, kPALA8) || Is(format, kRGBA32) ? 1 : 0;
}

// 0x8C7040 (slot 9): whether a format reads as RGB24 (PAL4, PAL8, RGB24)
uint8_t __fastcall Converter_ReadsAsRGB24(void*, re::Edx, const void* format) {
    return Is(format, kPAL4) || Is(format, kPAL8) || Is(format, kRGB24) ? 1 : 0;
}

// 0x8C5BA0: a lone jmp to 0x8C5A80 (BGRA5551 to RGBA5551, the same conversion)
RE_UNHOOKED_FN("NiDevImageConverter::ConvertBGRA5551ToRGBA5551", 0x008C5BA0, &Convert16To16);

#define GB44I_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

GB44I_REVERSED("NiDevImageConverter::IsReadableFormat", 0x008C5A50, Converter_IsReadableFormat);
GB44I_REVERSED("NiDevImageConverter::Convert16To16", 0x008C5A80, Convert16To16);
GB44I_REVERSED("NiDevImageConverter::ConvertRGB24To16", 0x008C5BB0, ConvertRGB24To16);
GB44I_REVERSED("NiDevImageConverter::ConvertRGB24To24", 0x008C5C90, ConvertRGB24To24);
GB44I_REVERSED("NiDevImageConverter::ConvertRGB24To32", 0x008C5D20, ConvertRGB24To32);
GB44I_REVERSED("NiDevImageConverter::ConvertRGBA32To16", 0x008C5DE0, ConvertRGBA32To16);
GB44I_REVERSED("NiDevImageConverter::ConvertRGBA32To24", 0x008C5EC0, ConvertRGBA32To24);
GB44I_REVERSED("NiDevImageConverter::Convert32To32", 0x008C5F90, Convert32To32);
GB44I_REVERSED("NiDevImageConverter::Convert16ToRGB24", 0x008C60A0, Convert16ToRGB24);
GB44I_REVERSED("NiDevImageConverter::Convert16AlphaToRGBA32", 0x008C6130, Convert16AlphaToRGBA32);
GB44I_REVERSED("NiDevImageConverter::Convert16ToRGBA32", 0x008C61E0, Convert16ToRGBA32);
GB44I_REVERSED("NiDevImageConverter::Convert24ToRGB24", 0x008C6270, Convert24ToRGB24);
GB44I_REVERSED("NiDevImageConverter::Convert24ToRGBA32", 0x008C6300, Convert24ToRGBA32);
GB44I_REVERSED("NiDevImageConverter::Convert32ToRGB24", 0x008C63D0, Convert32ToRGB24);
GB44I_REVERSED("NiDevImageConverter::Convert32AlphaToRGBA32", 0x008C6450, Convert32AlphaToRGBA32);
GB44I_REVERSED("NiDevImageConverter::Convert32ToRGBA32", 0x008C6500, Convert32ToRGBA32);
GB44I_REVERSED("NiDevImageConverter::ConvertBumpLuma32To32", 0x008C6590, ConvertBumpLuma32To32);
GB44I_REVERSED("NiDevImageConverter::ConvertBumpLuma32To16", 0x008C6630, ConvertBumpLuma32To16);
GB44I_REVERSED("NiDevImageConverter::ConvertBump16To32", 0x008C66F0, ConvertBump16To32);
GB44I_REVERSED("NiDevImageConverter::ConvertBump16To16", 0x008C6780, ConvertBump16To16);
GB44I_REVERSED("NiDevImageConverter::ChannelMasks", 0x008C6820, ChannelMasks_ConstructImpl);
GB44I_REVERSED("NiDevImageConverter::~NiDevImageConverter", 0x008C6950, Converter_Destruct);
GB44I_REVERSED("NiDevImageConverter::GetReaders", 0x008C69E0, Converter_GetReaders);
GB44I_REVERSED("NiDevImageConverter::CanReadFile", 0x008C69F0, Converter_CanReadFile);
GB44I_REVERSED("NiDevImageConverter::ReadImageFileInfo", 0x008C6AE0, Converter_ReadImageFileInfo);
GB44I_REVERSED("NiDevImageConverter::ReadImageFile", 0x008C6C20, Converter_ReadImageFile);
GB44I_REVERSED("NiDevImageConverter::CanConvertPixelData", 0x008C6D20, Converter_CanConvertPixelData);
GB44I_REVERSED("NiDevImageConverter::ReadsAsRGBA32", 0x008C7000, Converter_ReadsAsRGBA32);
GB44I_REVERSED("NiDevImageConverter::ReadsAsRGB24", 0x008C7040, Converter_ReadsAsRGB24);

} // namespace ni::c44
