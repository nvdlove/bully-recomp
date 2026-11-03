// NiDevImageConverter (src/gamebryo/NiDevImageConverter.h), 0x8C7080..0x8CA1D0 and its DXT
// decompressor, 0x8CC740..0x8CD100: converting pixel data between formats (the palettized row
// converters here, the others chunk 44's), box-filtered mipmaps, bump maps from height maps,
// the formats each platform takes, reading mipmap levels from files, and DXT1/3/5 decompressed
// to RGBA32. The constructor lists the BMP, DDS, SGI, Targa and NIF readers.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDevImageConverter.h"
#include "gamebryo/NiPixelData.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using sys::Decrement;
using sys::Increment;

constexpr uintptr_t kNiDevImageConverterVtable = 0x009A18C0;
constexpr uintptr_t kReaderListVtable = 0x0095145C;

// the formats (NiPixelFormat statics in .data)
constexpr uintptr_t kPAL4 = 0x00BC7630, kPALA4 = 0x00BC7678, kPAL8 = 0x00BC76C0, kPALA8 = 0x00BC7708;
constexpr uintptr_t kRGBA32 = 0x00BC7750, kRGB24 = 0x00BC7798, kRGBA5551 = 0x00BC77E0, kBGRA5551 = 0x00BC7828;
constexpr uintptr_t kBUMP16 = 0x00BC7870, kBUMPLUMA32 = 0x00BC78B8;
constexpr uintptr_t kDXT1 = 0x00BC7900, kDXT3 = 0x00BC7948, kDXT5 = 0x00BC7990;
constexpr uintptr_t kRGBA128F = 0x00BC79D8, kRGBA64F = 0x00BC7A20, kRG64F = 0x00BC7A68, kRG32F = 0x00BC7AB0;
constexpr uintptr_t kR32F = 0x00BC7AF8, kR16F = 0x00BC7B40, kRGBA128F_B = 0x00BC7B88, kRGBA64F_B = 0x00BC7BD0;
constexpr uintptr_t kA8 = 0x00BC7EA0, kBGR565 = 0x00BC8008, kBGRA5551_B = 0x00BC8098, kBGRA4444 = 0x00BC8128;
constexpr uintptr_t kBGRA8888 = 0x00BC8200, kARGB8888 = 0x00BC8248, kBGRX8888 = 0x00BC8290, kL8 = 0x00BC84D0;

const NiPixelFormat* Format(uintptr_t address) {
    return reinterpret_cast<const NiPixelFormat*>(address);
}

// the masks, shifts and lost bits of a format's four channels (0x18: chunk 44's)
struct ChannelMasks {
    uint32_t mask[4];   // +0x0 (red, green, blue, alpha)
    uint8_t shift[4];   // +0x10
    uint8_t loss[4];    // +0x14 (8 - the channel's bits)
};
static_assert(sizeof(ChannelMasks) == 0x18);

// a row converter: (width, height, destination bytes, destination, source palette, the
// destination's masks, source, the source's masks)
using RowConverter = void(__cdecl*)(uint32_t width, uint32_t height, uint32_t size, void* dst, NiObject* palette,
                                    const ChannelMasks* dstMasks, const uint8_t* src, const ChannelMasks* srcMasks);

GAME_FN(void, __thiscall, ChannelMasks_Construct, 0x008C6820, ChannelMasks* self, const NiPixelFormat* format);
GAME_FN(void, __thiscall, Converter_Destruct, 0x008C6950, NiDevImageConverter* self);
GAME_FN(void, __thiscall, ImageConverter_Construct, 0x007D3B50, NiDevImageConverter* self);
GAME_FN(void*, __thiscall, NIFReader_Construct, 0x007F75B0, void* self);
GAME_FN(void*, __thiscall, TGAReader_Construct, 0x008CD5E0, void* self);
GAME_FN(void*, __thiscall, SGIReader_Construct, 0x008CE320, void* self);
GAME_FN(void*, __thiscall, DDSReader_Construct, 0x008CEBF0, void* self);
GAME_FN(void*, __thiscall, BMPReader_Construct, 0x008CFF60, void* self);
GAME_FN(NiPixelFormat*, __thiscall, PixelFormat_Construct, 0x0076B280, NiPixelFormat* self);
GAME_FN(uint8_t, __thiscall, PixelFormat_Equals, 0x007D3AB0, const NiPixelFormat* self, const NiPixelFormat* other);
GAME_FN(uint8_t, __thiscall, PixelFormat_EqualsIgnoringEndian, 0x0076AE80, const NiPixelFormat* self,
        const NiPixelFormat* other);
GAME_FN(uint8_t, __thiscall, PixelFormat_IsCompressed, 0x005F0A20, const NiPixelFormat* self);
GAME_FN(uint8_t, __thiscall, PixelFormat_IsPalettized, 0x007DD370, const NiPixelFormat* self);
GAME_FN(uint8_t, __thiscall, PixelFormat_GetBits, 0x007DD390, const NiPixelFormat* self, int component);
GAME_FN(NiPixelData*, __thiscall, PixelData_Construct, 0x0076A610, void* self, uint32_t width, uint32_t height,
        const NiPixelFormat* format, uint32_t levels, uint32_t faces);
GAME_FN(NiPixelData*, __thiscall, PixelData_CopyConstruct, 0x0076A4A0, void* self, const NiPixelData* other);
GAME_FN(uint8_t, __thiscall, PixelData_SwapEndian, 0x0076A310, NiPixelData* self);
GAME_FN(void, __thiscall, PixelData_SetPalette, 0x007D3E40, NiPixelData* self, NiObject* palette);
GAME_FN(NiObject*, __thiscall, Palette_Construct, 0x007D45F0, void* self, int alpha, uint32_t count,
        const void* entries);
GAME_FN(NiObject*, __thiscall, Palette_CopyConstruct, 0x007D4560, void* self, NiObject* other);
GAME_FN(NiObject*, __thiscall, Palette_Assign, 0x007D4430, NiObject* self, NiObject* other);
GAME_FN(void, __thiscall, NiPointer_Assign, 0x00800420, void* slot, void* value);
GAME_FN(void, __thiscall, NiPointer_Destruct, 0x007549D0, void* slot);
GAME_FN(void, __cdecl, CopyBytes, 0x0074D2A0, void* to, const void* from, uint32_t size);
// this file's, called where modes apply
GAME_FN(NiPixelData*, __thiscall, Converter_GenerateMipmaps, 0x008C7080, NiDevImageConverter* self,
        NiPixelData* src, NiPixelData* dest);
GAME_FN(DXTDecoder*, __thiscall, Decoder_Construct, 0x008CCFB0, DXTDecoder* self);
GAME_FN(void, __thiscall, Decoder_WriteBlock, 0x008CD080, DXTDecoder* self, uint8_t* dst, uint32_t columns,
        uint32_t rows);
GAME_FN(NiPixelData*, __thiscall, Decoder_Decompress, 0x008CD100, DXTDecoder* self, const NiPixelData* src);

template <typename R = void, typename... A>
R Virtual(const void* object, int slot, A... args) {
    return reinterpret_cast<R(__thiscall*)(const void*, re::Arg<A>...)>(static_cast<void*>(VtableOf(object)[slot]))(object, args...);
}

void ReleaseRef(NiRefObject* object) {
    if (object != nullptr && Decrement(&object->refCount) == 0) {
        Virtual(object, 1);
    }
}

// a level of a face in pixel data, and its size
uint8_t* LevelStart(const NiPixelData* pixels, uint32_t level, uint32_t face) {
    return pixels->pixels + pixels->offsets[pixels->mipmapLevels] * face + pixels->offsets[level];
}
uint32_t LevelSize(const NiPixelData* pixels, uint32_t level) {
    return pixels->offsets[level + 1] - pixels->offsets[level];
}

// a palette entry's channel into a destination format's bits
uint32_t PackChannel(uint8_t value, const ChannelMasks* masks, int channel) {
    return (uint32_t(value >> (masks->loss[channel] & 31)) << (masks->shift[channel] & 31)) & masks->mask[channel];
}

// the destination format's alpha bits all set (a palette without alpha)
uint32_t OpaqueAlpha(const ChannelMasks* masks) {
    return (uint32_t(0xFF >> (masks->loss[3] & 31)) << (masks->shift[3] & 31)) & masks->mask[3];
}

const uint8_t* PaletteEntries(NiObject* palette) {
    return *reinterpret_cast<const re::Ptr32<const uint8_t>*>(reinterpret_cast<const uint8_t*>(palette) + 0x14);
}

// a 4-bit image's nibbles (high first) a byte each, in a NiMalloc'd buffer
uint8_t* UnpackNibbles(uint32_t width, uint32_t height, const uint8_t* src) {
    auto* indices = static_cast<uint8_t*>(NiMalloc(width * height));
    uint32_t count = (width >> 1) * height;
    uint8_t* out = indices;
    for (uint32_t i = 0; i < count; i++) {
        out[0] = uint8_t(src[i] >> 4);
        out[1] = uint8_t(src[i] & 0xF);
        out += 2;
    }
    return indices;
}

// ---- the palettized row converters

// 4-bit palette to 32 bits (the palette's alpha ignored)
void __cdecl Pal4To32(uint32_t width, uint32_t height, uint32_t, void* dst, NiObject* palette,
                      const ChannelMasks* masks, const uint8_t* src, const ChannelMasks*) {
    const uint8_t* entries = PaletteEntries(palette);
    uint8_t* indices = UnpackNibbles(width, height, src);
    uint32_t alpha = OpaqueAlpha(masks);
    uint32_t table[16];
    for (int i = 0; i < 16; i++) {
        const uint8_t* e = entries + i * 4;
        table[i] = PackChannel(e[0], masks, 0) | PackChannel(e[2], masks, 2) | PackChannel(e[1], masks, 1) | alpha;
    }
    auto* out = static_cast<uint32_t*>(dst);
    const uint8_t* index = indices;
    for (uint32_t y = height; y != 0; y--) {
        for (uint32_t x = width; x != 0; x--) {
            *out++ = table[*index++];
        }
    }
    NiFree(indices);
}

// a palette's colours to 24 bits as RGB or BGR (by the destination's red mask)
void CopyEntries24(uint32_t width, uint32_t height, uint8_t* out, const uint8_t* entries, const uint8_t* index,
                   const ChannelMasks* masks) {
    if (masks->mask[0] == 0xFF) {
        for (uint32_t y = height; y != 0; y--) {
            for (uint32_t x = width; x != 0; x--) {
                const uint8_t* e = entries + uint32_t(*index++) * 4;
                out[0] = e[0];
                out[1] = e[1];
                out[2] = e[2];
                out += 3;
            }
        }
    } else if (masks->mask[0] == 0xFF0000) {
        for (uint32_t y = height; y != 0; y--) {
            for (uint32_t x = width; x != 0; x--) {
                const uint8_t* e = entries + uint32_t(*index++) * 4;
                out[0] = e[2];
                out[1] = e[1];
                out[2] = e[0];
                out += 3;
            }
        }
    }
}

// 4-bit palette to 24 bits (green must be the middle byte)
void __cdecl Pal4To24(uint32_t width, uint32_t height, uint32_t, void* dst, NiObject* palette,
                      const ChannelMasks* masks, const uint8_t* src, const ChannelMasks*) {
    const uint8_t* entries = PaletteEntries(palette);
    if (masks->mask[1] != 0xFF00) {
        return;
    }
    uint8_t* indices = UnpackNibbles(width, height, src);
    CopyEntries24(width, height, static_cast<uint8_t*>(dst), entries, indices, masks);
    NiFree(indices);
}

// 4-bit palette with alpha to 32 bits
void __cdecl PalA4To32(uint32_t width, uint32_t height, uint32_t, void* dst, NiObject* palette,
                       const ChannelMasks* masks, const uint8_t* src, const ChannelMasks*) {
    const uint8_t* entries = PaletteEntries(palette);
    uint8_t* indices = UnpackNibbles(width, height, src);
    uint32_t table[16];
    for (int i = 0; i < 16; i++) {
        const uint8_t* e = entries + i * 4;
        table[i] = PackChannel(e[0], masks, 0) | PackChannel(e[1], masks, 1) | PackChannel(e[3], masks, 3) |
                   PackChannel(e[2], masks, 2);
    }
    auto* out = static_cast<uint32_t*>(dst);
    const uint8_t* index = indices;
    for (uint32_t y = height; y != 0; y--) {
        for (uint32_t x = width; x != 0; x--) {
            *out++ = table[*index++];
        }
    }
    NiFree(indices);
}

// 8-bit palette to 16 bits (the palette's alpha ignored)
void __cdecl Pal8To16(uint32_t width, uint32_t height, uint32_t, void* dst, NiObject* palette,
                      const ChannelMasks* masks, const uint8_t* src, const ChannelMasks*) {
    const uint8_t* entries = PaletteEntries(palette);
    uint32_t alpha = OpaqueAlpha(masks);
    uint16_t table[256];
    for (int i = 0; i < 256; i++) {
        const uint8_t* e = entries + i * 4;
        table[i] = uint16_t(PackChannel(e[2], masks, 2) | PackChannel(e[0], masks, 0) | PackChannel(e[1], masks, 1) |
                            alpha);
    }
    auto* out = static_cast<uint16_t*>(dst);
    for (uint32_t y = height; y != 0; y--) {
        for (uint32_t x = width; x != 0; x--) {
            *out++ = table[*src++];
        }
    }
}

// 8-bit palette to 32 bits (the palette's alpha ignored)
void __cdecl Pal8To32(uint32_t width, uint32_t height, uint32_t, void* dst, NiObject* palette,
                      const ChannelMasks* masks, const uint8_t* src, const ChannelMasks*) {
    const uint8_t* entries = PaletteEntries(palette);
    uint32_t alpha = OpaqueAlpha(masks);
    uint32_t table[256];
    for (int i = 0; i < 256; i++) {
        const uint8_t* e = entries + i * 4;
        table[i] = PackChannel(e[0], masks, 0) | PackChannel(e[2], masks, 2) | PackChannel(e[1], masks, 1) | alpha;
    }
    auto* out = static_cast<uint32_t*>(dst);
    for (uint32_t y = height; y != 0; y--) {
        for (uint32_t x = width; x != 0; x--) {
            *out++ = table[*src++];
        }
    }
}

// 8-bit palette with alpha to 16 bits
void __cdecl PalA8To16(uint32_t width, uint32_t height, uint32_t, void* dst, NiObject* palette,
                       const ChannelMasks* masks, const uint8_t* src, const ChannelMasks*) {
    const uint8_t* entries = PaletteEntries(palette);
    uint16_t table[256];
    for (int i = 0; i < 256; i++) {
        const uint8_t* e = entries + i * 4;
        table[i] = uint16_t(PackChannel(e[0], masks, 0) | PackChannel(e[1], masks, 1) | PackChannel(e[3], masks, 3) |
                            PackChannel(e[2], masks, 2));
    }
    auto* out = static_cast<uint16_t*>(dst);
    for (uint32_t y = height; y != 0; y--) {
        for (uint32_t x = width; x != 0; x--) {
            *out++ = table[*src++];
        }
    }
}

// 8-bit palette to 24 bits (green must be the middle byte)
void __cdecl Pal8To24(uint32_t width, uint32_t height, uint32_t, void* dst, NiObject* palette,
                      const ChannelMasks* masks, const uint8_t* src, const ChannelMasks*) {
    const uint8_t* entries = PaletteEntries(palette);
    if (masks->mask[1] != 0xFF00) {
        return;
    }
    CopyEntries24(width, height, static_cast<uint8_t*>(dst), entries, src, masks);
}

// 8-bit palette with alpha to 32 bits
void __cdecl PalA8To32(uint32_t width, uint32_t height, uint32_t, void* dst, NiObject* palette,
                       const ChannelMasks* masks, const uint8_t* src, const ChannelMasks*) {
    const uint8_t* entries = PaletteEntries(palette);
    uint32_t table[256];
    for (int i = 0; i < 256; i++) {
        const uint8_t* e = entries + i * 4;
        table[i] = PackChannel(e[3], masks, 3) | PackChannel(e[0], masks, 0) | PackChannel(e[1], masks, 1) |
                   PackChannel(e[2], masks, 2);
    }
    auto* out = static_cast<uint32_t*>(dst);
    for (uint32_t y = height; y != 0; y--) {
        for (uint32_t x = width; x != 0; x--) {
            *out++ = table[*src++];
        }
    }
}

// ---- NiDevImageConverter

// vf10 GenerateMipmaps: the image as RGB24 or RGBA32 (the destination when it fits, with levels
// and the same faces; else a new full chain), then each level a 2x2 box filter of the one
// before (a 1-wide or 1-high level: pairs)
NiPixelData* __fastcall GenerateMipmaps(NiDevImageConverter* self, re::Edx, NiPixelData* src, NiPixelData* dest) {
    NiPixelFormat format;
    PixelFormat_Construct(&format);
    if (Virtual<uint8_t>(self, 9, &src->format) != 0) {
        format = *Format(kRGB24);
    } else if (Virtual<uint8_t>(self, 8, &src->format) != 0) {
        format = *Format(kRGBA32);
    } else {
        return nullptr;
    }
    NiPixelData* out = dest;
    if (dest == nullptr || PixelFormat_Equals(&dest->format, &format) == 0 || dest->mipmapLevels <= 1 ||
        dest->faces != src->faces) {
        void* p = NiNew(sizeof(NiPixelData));
        out = p != nullptr ? PixelData_Construct(p, src->widths[0], src->heights[0], &format, 0, src->faces) : nullptr;
    }
    uint32_t levels = out->mipmapLevels;
    if (src != out && Virtual<uint8_t>(self, 12, out, src, 0u) == 0) {
        return nullptr;
    }
    for (uint32_t face = 0; face < src->faces; face++) {
        uint32_t bytes;
        if (PixelFormat_Equals(&format, Format(kRGB24)) != 0) {
            bytes = 3;
        } else if (PixelFormat_Equals(&format, Format(kRGBA32)) != 0) {
            bytes = 4;
        } else {
            continue;
        }
        for (uint32_t level = 1; level < levels; level++) {
            const uint8_t* s = LevelStart(out, level - 1, face);
            uint8_t* d = LevelStart(out, level, face);
            uint32_t width = out->widths[level];
            uint32_t height = out->heights[level];
            uint32_t prevWidth = out->widths[level - 1];
            uint32_t rowBytes = prevWidth * bytes;
            if (prevWidth == 1) {
                for (uint32_t y = height; y != 0; y--) {
                    for (uint32_t c = 0; c < bytes; c++) {
                        *d++ = uint8_t((uint32_t(s[c + bytes]) + s[c]) >> 1);
                    }
                    s += bytes * 2;
                }
            } else if (out->heights[level - 1] == 1) {
                for (uint32_t x = width; x != 0; x--) {
                    for (uint32_t c = 0; c < bytes; c++) {
                        *d++ = uint8_t((uint32_t(s[c + bytes]) + s[c]) >> 1);
                    }
                    s += bytes * 2;
                }
            } else {
                for (uint32_t y = height; y != 0; y--) {
                    const uint8_t* below = s + rowBytes;
                    for (uint32_t x = width; x != 0; x--) {
                        for (uint32_t c = 0; c < bytes; c++) {
                            *d++ = uint8_t((uint32_t(s[c]) + s[c + bytes] + below[c] + below[c + bytes]) >> 2);
                        }
                        s += bytes * 2;
                        below += bytes * 2;
                    }
                    s += rowBytes;
                }
            }
        }
    }
    return out;
}

// vf16 ConvertForPlatform: the platform's format for the pixels (vf15); the same: them; else
// converted (when it can be) into new pixel data
NiPixelData* __fastcall ConvertForPlatform(NiDevImageConverter* self, re::Edx, uint32_t platform, const int32_t* prefs,
                                           NiPixelData* pixels, const NiPixelFormat* wanted) {
    const NiPixelFormat* source = &pixels->format;
    auto* target = Virtual<const NiPixelFormat*>(self, 15, platform, prefs, source, wanted);
    if (target == source) {
        return pixels;
    }
    if (target == nullptr) {
        return nullptr;
    }
    if (PixelFormat_Equals(target, source) != 0) {
        return pixels;
    }
    if (Virtual<uint8_t>(self, 4, source, target) == 0) {
        return nullptr;
    }
    void* p = NiNew(sizeof(NiPixelData));
    NiPixelData* out = p != nullptr ? PixelData_Construct(p, pixels->widths[0], pixels->heights[0], target,
                                                          pixels->mipmapLevels, pixels->faces)
                                    : nullptr;
    return Virtual<uint8_t>(self, 12, out, pixels, 0xFFFFFFFFu) != 0 ? out : nullptr;
}

// vf15 GetPlatformFormat: platform 0: RGBA32 for a palettized or 16-bit format when the
// preferences ask for layout 4, else the format as it is; 4, 5, 6: their finders; others none
const NiPixelFormat* __fastcall GetPlatformFormat(NiDevImageConverter* self, re::Edx, uint32_t platform,
                                                  const int32_t* prefs, const NiPixelFormat* source,
                                                  const NiPixelFormat* wanted) {
    switch (platform) {
    case 0:
        if ((wanted->format == 2 || wanted->format == 3 || wanted->bitsPerPixel == 0x10) && prefs[0] == 4) {
            return Format(kRGBA32);
        }
        return wanted;
    case 4:
        return Virtual<const NiPixelFormat*>(self, 19, prefs, source, wanted);
    case 5:
        return Virtual<const NiPixelFormat*>(self, 20, prefs, source, wanted);
    case 6:
        return Virtual<const NiPixelFormat*>(self, 18, prefs, source, wanted);
    default:
        return nullptr;
    }
}

// whether a source format has alpha (palettized with alpha; an alpha channel with bits; any
// DXT)
bool SourceHasAlpha(const NiPixelFormat* source) {
    int32_t f = source->format;
    bool alpha;
    if (f == 2 || f == 3) {
        alpha = f == 3;
    } else {
        alpha = false;
        for (const NiPixelComponent& component : source->components) {
            if (component.component == 3) {
                alpha = component.bitsPerComponent != 0;
                break;
            }
        }
    }
    if (f >= 4 && f <= 6) {
        alpha = true;
    }
    return alpha;
}

// the DXT a format maps to (DXT1/3/5 as they are; else by the alpha choice)
const NiPixelFormat* DXTFor(int32_t format, bool five) {
    switch (format) {
    case 4: return Format(kDXT1);
    case 5: return Format(kDXT3);
    case 6: return Format(kDXT5);
    default: return five ? Format(kDXT5) : Format(kDXT1);
    }
}

// vf18 (platform 6): the preferences' layout (prefs[0]) and alpha choice (prefs[1]) as a format
const NiPixelFormat* __fastcall PlatformFormat6(NiDevImageConverter*, re::Edx, const int32_t* prefs,
                                                const NiPixelFormat*, const NiPixelFormat* format) {
    int32_t layout = prefs[0];
    switch (layout) {
    case 7: return prefs[1] != 0 ? Format(kA8) : Format(kL8);
    case 8: return Format(kR16F);
    case 9: return Format(kR32F);
    case 10: return Format(kRG32F);
    case 11: return Format(kRG64F);
    case 14: return Format(kRGBA128F);
    case 13:
    case 12: return Format(kRGBA64F);
    case 2: return prefs[1] != 0 ? Format(kBGRA8888) : Format(kBGRX8888);
    case 1:
        if (prefs[1] == 1) {
            return Format(kBGRA5551_B);
        }
        return prefs[1] == 2 ? Format(kBGRA4444) : Format(kBGR565);
    case 0:
    case 5: return DXTFor(-1, prefs[1] == 2);
    case 3: return DXTFor(format->format, prefs[1] == 2);
    case 4: return Format(kBUMPLUMA32);
    case 6:
        break;
    default:
        return format;
    }
    if (PixelFormat_IsCompressed(format) != 0 || PixelFormat_IsPalettized(format) != 0) {
        return DXTFor(format->format, prefs[1] == 2);
    }
    if (format->bitsPerPixel == 0x10) {
        bool alpha = PixelFormat_IsPalettized(format) != 0 ? format->format == 3
                                                           : PixelFormat_GetBits(format, 3) != 0;
        if (PixelFormat_IsCompressed(format) != 0) {
            alpha = true;
        }
        int32_t choice = prefs[1];
        if (choice == 1) {
            return Format(kBGRA5551_B);
        }
        if (choice == 2 || (choice != 0 && alpha)) {
            return Format(kBGRA4444);
        }
        return Format(kBGR565);
    }
    if (format->bitsPerPixel != 0x20) {
        return format;
    }
    int32_t choice = prefs[1];
    if (choice == 0) {
        return Format(kBGRX8888);
    }
    if (choice == 1 || choice == 2) {
        return Format(kBGRA8888);
    }
    if (format->format == 0) {
        return Format(kBGRX8888);
    }
    return format->format == 1 ? Format(kBGRA8888) : format;
}

// vf19 (platform 4)
const NiPixelFormat* __fastcall PlatformFormat4(NiDevImageConverter*, re::Edx, const int32_t* prefs,
                                                const NiPixelFormat* source, const NiPixelFormat* format) {
    bool alpha = SourceHasAlpha(source);
    switch (prefs[0]) {
    case 14: return Format(kRGBA128F);
    case 13: return Format(kRGBA64F);
    case 2: return Format(kBGRA8888);
    case 1: return alpha ? Format(kBGRA4444) : Format(kBGR565);
    case 0:
    case 5: return DXTFor(-1, alpha);
    case 3: return DXTFor(format->format, alpha);
    case 4: return Format(kBUMPLUMA32);
    case 6:
        if (PixelFormat_IsPalettized(format) != 0 || PixelFormat_IsCompressed(format) != 0) {
            return DXTFor(format->format, alpha);
        }
        switch (format->bitsPerPixel) {
        case 0x10: return alpha ? Format(kBGRA4444) : Format(kBGR565);
        case 0x40: return Format(kRGBA64F);
        case 0x80: return Format(kRGBA128F);
        default: return Format(kBGRA8888);
        }
    default:
        return Format(kBGRA8888);
    }
}

// vf20 (platform 5)
const NiPixelFormat* __fastcall PlatformFormat5(NiDevImageConverter*, re::Edx, const int32_t* prefs,
                                                const NiPixelFormat* source, const NiPixelFormat* format) {
    bool alpha = SourceHasAlpha(source);
    switch (prefs[0]) {
    case 14: return Format(kRGBA128F_B);
    case 13:
    case 12: return Format(kRGBA64F_B);
    case 1: return alpha ? Format(kBGRA4444) : Format(kBGR565);
    case 0:
    case 5: return DXTFor(-1, prefs[1] == 2);
    case 3: return DXTFor(format->format, prefs[1] == 2);
    case 4: return Format(kBUMPLUMA32);
    case 6:
        break;
    default:
        return Format(kARGB8888);
    }
    if (PixelFormat_IsCompressed(format) != 0 || PixelFormat_IsPalettized(format) != 0) {
        return DXTFor(format->format, prefs[1] == 2);
    }
    if (format->bitsPerPixel == 0x10) {
        bool alpha16 = PixelFormat_IsPalettized(format) != 0 ? format->format == 3
                                                             : PixelFormat_GetBits(format, 3) != 0;
        if (PixelFormat_IsCompressed(format) != 0) {
            alpha16 = true;
        }
        int32_t choice = prefs[1];
        if (choice == 1 || choice == 2 || (choice != 0 && alpha16)) {
            return Format(kBGRA4444);
        }
        return Format(kBGR565);
    }
    if (PixelFormat_Equals(format, Format(kRGBA64F)) != 0 || PixelFormat_Equals(format, Format(kRGBA64F_B)) != 0) {
        return Format(kRGBA64F_B);
    }
    if (PixelFormat_Equals(format, Format(kRGBA128F)) != 0 || PixelFormat_Equals(format, Format(kRGBA128F_B)) != 0) {
        return Format(kRGBA128F_B);
    }
    return Format(kARGB8888);
}

// vf13 ConvertToBump: a height map (the red channel) as a bump map: each pixel's slopes to the
// right and down (the last column and row looking back; one pixel: 0), and for BUMPLUMA32 the
// luminance (RGBA32: its alpha; RGB24: 255) with the height. RGB24 to anything else: BUMP16.
NiPixelData* __fastcall ConvertToBump(NiDevImageConverter*, re::Edx, const NiPixelData* src,
                                      const NiPixelFormat* format) {
    uint32_t width0 = src->widths[0];
    uint32_t height0 = src->heights[0];
    uint32_t levels = src->mipmapLevels;
    NiPixelData* out;
    uint32_t srcBytes;
    uint32_t dstBytes;
    if (PixelFormat_Equals(&src->format, Format(kRGBA32)) != 0) {
        void* p = NiNew(sizeof(NiPixelData));
        out = p != nullptr ? PixelData_Construct(p, width0, height0, Format(kBUMPLUMA32), levels, 1) : nullptr;
        for (uint32_t level = 0; level < levels; level++) {
            uint8_t* d = out->pixels + out->offsets[level];
            const uint8_t* s = src->pixels + src->offsets[level];
            for (uint32_t y = src->heights[level]; y != 0; y--) {
                for (uint32_t x = src->widths[level]; x != 0; x--) {
                    d[2] = s[3];
                    d[3] = s[0];
                    s += 4;
                    d += 4;
                }
            }
        }
        srcBytes = 4;
        dstBytes = 4;
    } else if (PixelFormat_Equals(&src->format, Format(kRGB24)) != 0 &&
               PixelFormat_Equals(format, Format(kBUMPLUMA32)) != 0) {
        void* p = NiNew(sizeof(NiPixelData));
        out = p != nullptr ? PixelData_Construct(p, width0, height0, Format(kBUMPLUMA32), levels, 1) : nullptr;
        for (uint32_t level = 0; level < levels; level++) {
            uint8_t* d = out->pixels + out->offsets[level];
            const uint8_t* s = src->pixels + src->offsets[level];
            for (uint32_t y = src->heights[level]; y != 0; y--) {
                for (uint32_t x = src->widths[level]; x != 0; x--) {
                    d[2] = 0xFF;
                    d[3] = s[0];
                    s += 3;
                    d += 4;
                }
            }
        }
        dstBytes = 4;
        srcBytes = 3;
    } else if (PixelFormat_Equals(&src->format, Format(kRGB24)) != 0) {
        void* p = NiNew(sizeof(NiPixelData));
        out = p != nullptr ? PixelData_Construct(p, width0, height0, Format(kBUMP16), levels, 1) : nullptr;
        dstBytes = 2;
        srcBytes = 3;
    } else {
        return nullptr;
    }
    for (uint32_t level = 0; level < levels; level++) {
        uint8_t* d = out->pixels + out->offsets[level];
        const uint8_t* s = src->pixels + src->offsets[level];
        uint32_t width = src->widths[level];
        uint32_t lastX = width - 1;
        uint32_t lastY = src->heights[level] - 1;
        const uint8_t* right = s + srcBytes;
        const uint8_t* below = s + width * srcBytes;
        for (uint32_t y = lastY; y != 0; y--) {
            for (uint32_t x = lastX; x != 0; x--) {
                d[0] = uint8_t(*right - *s);
                d[1] = uint8_t(*below - *s);
                d += dstBytes;
                s += srcBytes;
                right += srcBytes;
                below += srcBytes;
            }
            d[0] = lastX != 0 ? uint8_t(*s - *(s - srcBytes)) : uint8_t(0);
            d[1] = uint8_t(*below - *s);
            s += srcBytes;
            d += dstBytes;
            right += srcBytes;
            below += srcBytes;
        }
        // the last row: the slope down taken from the row above
        const uint8_t* above = s - width * srcBytes;
        for (uint32_t x = lastX; x != 0; x--) {
            uint8_t value = *s;
            d[0] = uint8_t(*right - value);
            d[1] = uint8_t(value - *above);
            d += dstBytes;
            above += srcBytes;
            s += srcBytes;
            right += srcBytes;
        }
        d[0] = lastX != 0 ? uint8_t(*s - *(s - srcBytes)) : uint8_t(0);
        d[1] = lastY != 0 ? uint8_t(*s - *above) : uint8_t(0);
    }
    return out;
}

// the deleting destructor (the destructor is chunk 44's)
NiDevImageConverter* __fastcall DeletingDestructor(NiDevImageConverter* self, re::Edx, uint32_t flags) {
    Converter_Destruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(NiDevImageConverter));
    }
    return self;
}

// NiTPointerList's AddHead, inlined: a node from its allocator (slot 1)
void ReadersAddHead(NiTPointerList<NiImageReader*>* list, NiImageReader* reader) {
    auto* node = Virtual<NiTListNode<NiImageReader*>*>(list, 1);
    node->element = reader;
    node->prev = nullptr;
    node->next = list->head;
    if (list->head != nullptr) {
        list->head->prev = node;
        list->count++;
        list->head = node;
    } else {
        list->count++;
        list->tail = node;
        list->head = node;
    }
}

// vf14 AddReader: put first, so it's asked before the others
void __fastcall AddReader(NiDevImageConverter* self, re::Edx, NiImageReader* reader) {
    ReadersAddHead(&self->readers, reader);
}

// vf5 ConvertPixelData: the pixels in another format (the destination when it fits): byte
// order swapped first when only that differs (in place when the destination is the source),
// DXT decompressed (from the original pixels), mipmaps made when asked for a power-of-two
// image, a bump map made for the bump formats, then converted (vf12)
NiPixelData* __fastcall ConvertPixelData(NiDevImageConverter* self, re::Edx, NiPixelData* src,
                                         const NiPixelFormat* format, NiPixelData* dest, uint8_t mipmap) {
    NiPixelData* swapped = nullptr;
    NiPixelData* decompressed = nullptr;
    NiPixelData* current = src;
    uint8_t convertible = 0;
    bool endianDiffers = ((format->flags ^ src->format.flags) & 1) != 0;
    if (endianDiffers) {
        convertible = PixelFormat_EqualsIgnoringEndian(&src->format, format);
    }
    if (endianDiffers) {
        if (convertible == 0) {
            return nullptr;
        }
        if (dest != nullptr && src == dest) {
            if (PixelData_SwapEndian(dest) == 0) {
                NiPointer_Destruct(&swapped);
                NiPointer_Destruct(&decompressed);
                return nullptr;
            }
        } else {
            void* p = NiNew(sizeof(NiPixelData));
            NiPointer_Assign(&swapped, p != nullptr ? PixelData_CopyConstruct(p, src) : nullptr);
            current = swapped;
            if (PixelData_SwapEndian(current) == 0) {
                ReleaseRef(current);
                return nullptr;
            }
        }
    }
    int32_t code = src->format.format;
    if (code >= 4 && code <= 6 && format->format != code) {
        NiPointer_Assign(&decompressed, Decoder_Decompress(&self->decoder, src));
        current = decompressed;
    }
    NiPixelData* mipmapped = nullptr;
    NiPixelData* result = current;
    if (mipmap != 0 && current->mipmapLevels <= 1) {
        uint32_t width = current->widths[0];
        uint32_t height = current->heights[0];
        int32_t wanted = format->format;
        if (width != 0 && (width & (width - 1)) == 0 && height != 0 && (height & (height - 1)) == 0 &&
            (wanted == 0 || wanted == 1 || wanted == 8 || wanted == 9)) {
            NiPointer_Assign(&mipmapped, Converter_GenerateMipmaps(self, current, dest));
            result = mipmapped != nullptr ? mipmapped : current;
        }
    }
    if (dest != nullptr && PixelFormat_Equals(&dest->format, format) != 0 &&
        (mipmap == 0 || dest->mipmapLevels > 1) &&
        (dest == result || Virtual<uint8_t>(self, 12, dest, result, 0xFFFFFFFFu) != 0)) {
        NiPointer_Destruct(&mipmapped);
        NiPointer_Destruct(&swapped);
        NiPointer_Destruct(&decompressed);
        return dest;
    }
    NiPixelData* bump = nullptr;
    if (format->format == 8 || format->format == 9) {
        auto* made = Virtual<NiPixelData*>(self, 13, result, format);
        if (made != nullptr) {
            Increment(&made->refCount);
            bump = made;
            result = made;
        }
    }
    void* p = NiNew(sizeof(NiPixelData));
    NiPixelData* out = p != nullptr ? PixelData_Construct(p, result->widths[0], result->heights[0], format,
                                                          result->mipmapLevels, result->faces)
                                    : nullptr;
    uint8_t converted = Virtual<uint8_t>(self, 12, out, result, 0xFFFFFFFFu);
    ReleaseRef(bump);
    ReleaseRef(mipmapped);
    ReleaseRef(swapped);
    ReleaseRef(decompressed);
    return converted != 0 ? out : nullptr;   // (a failed conversion's pixel data is leaked)
}

// every face's levels first..last copied as they are
void CopyLevels(NiPixelData* dst, const NiPixelData* src, uint32_t first, uint32_t last, uint32_t faces) {
    for (uint32_t face = 0; face < faces; face++) {
        for (uint32_t level = first; level <= last; level++) {
            crt::MemcpyS(LevelStart(dst, level, face), LevelSize(dst, level), LevelStart(src, level, face),
                         LevelSize(src, level));
        }
    }
}

// the source's palette given to the destination (copied, or assigned over its own); its first
// entry made opaque
uint8_t CopyPaletteOpaque(NiPixelData* dst, const NiPixelData* src) {
    if (src->palette == nullptr) {
        return 0;
    }
    if (dst->palette == nullptr) {
        void* p = NiNew(0x24);
        PixelData_SetPalette(dst, p != nullptr ? Palette_CopyConstruct(p, src->palette) : nullptr);
    } else {
        Palette_Assign(dst->palette, src->palette);
    }
    NiObject* palette = dst->palette;
    uint32_t count = *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(palette) + 0xC);
    if (count != 0) {
        uint8_t* entries = *reinterpret_cast<re::Ptr32<uint8_t>*>(reinterpret_cast<uint8_t*>(palette) + 0x14);
        entries[3] = 0xFF;
    }
    return 1;
}

// the source's palette's entries in a new palette without alpha
uint8_t CopyPaletteNoAlpha(NiPixelData* dst, const NiPixelData* src, uint32_t count) {
    if (src->palette == nullptr) {
        return 0;
    }
    void* p = NiNew(0x24);
    PixelData_SetPalette(dst, p != nullptr ? Palette_Construct(p, 0, count, PaletteEntries(src->palette)) : nullptr);
    return 1;
}

// vf12 ConvertFormat: one level (or all, -1) of every face of the source into the destination
// (same size and faces): a copy when the formats match (the palette too), else through the row
// converter for the pair of formats (palettized ones also to their alpha / no-alpha twin)
uint8_t __fastcall ConvertFormat(NiDevImageConverter*, re::Edx, NiPixelData* dst, const NiPixelData* src,
                                 uint32_t level) {
    if (src->widths[0] != dst->widths[0] || src->heights[0] != dst->heights[0] || src->faces != dst->faces) {
        return 0;
    }
    uint32_t faces = src->faces;
    uint32_t first;
    uint32_t last;
    if (level == 0xFFFFFFFF) {
        if (src->mipmapLevels < dst->mipmapLevels) {
            return 0;
        }
        first = 0;
        last = dst->mipmapLevels - 1;
    } else {
        if (level >= src->mipmapLevels || level >= dst->mipmapLevels) {
            return 0;
        }
        first = level;
        last = level;
    }
    const NiPixelFormat* source = &src->format;
    const NiPixelFormat* target = &dst->format;
    if (PixelFormat_Equals(source, target) != 0) {
        CopyLevels(dst, src, first, last, faces);
        if (src->palette == nullptr) {
            return 1;
        }
        if (dst->palette != nullptr) {
            Palette_Assign(dst->palette, src->palette);
            return 1;
        }
        void* p = NiNew(0x24);
        PixelData_SetPalette(dst, p != nullptr ? Palette_CopyConstruct(p, src->palette) : nullptr);
        return 1;
    }
    int32_t dstCode = target->format;
    int32_t srcCode = source->format;
    uint32_t dstBits = target->bitsPerPixel;
    uint32_t srcBits = source->bitsPerPixel;
    ChannelMasks dstMasks;
    ChannelMasks srcMasks;
    ChannelMasks_Construct(&dstMasks, target);
    ChannelMasks_Construct(&srcMasks, source);
    if (dstCode == 4 || dstCode == 5 || dstCode == 6) {
        return 0;
    }
    if (dstBits == 0x18 &&
        (dstMasks.mask[1] != 0xFF00 || (dstMasks.mask[0] != 0xFF && dstMasks.mask[0] != 0xFF0000))) {
        return 0;
    }
    bool toRGB = dstCode == 0 || dstCode == 1;
    uintptr_t converter = 0;
    if (PixelFormat_Equals(source, Format(kPAL4)) != 0) {
        if (src->palette == nullptr) {
            return 0;
        }
        if (toRGB) {
            converter = dstBits == 0x18 ? 0x008C8260 : dstBits == 0x20 ? 0x008C7FE0 : 0;
        } else {
            if (PixelFormat_Equals(target, Format(kPALA4)) == 0) {
                return 0;
            }
            CopyLevels(dst, src, first, last, faces);
            return CopyPaletteOpaque(dst, src);
        }
    } else if (PixelFormat_Equals(source, Format(kPALA4)) != 0) {
        if (src->palette == nullptr) {
            return 0;
        }
        if (toRGB) {
            converter = dstBits == 0x18 ? 0x008C8260 : dstBits == 0x20 ? 0x008C83A0 : 0;
        } else {
            if (PixelFormat_Equals(target, Format(kPAL4)) == 0) {
                return 0;
            }
            CopyLevels(dst, src, first, last, faces);
            return CopyPaletteNoAlpha(dst, src, 0x10);
        }
    } else if (PixelFormat_Equals(source, Format(kPAL8)) != 0) {
        if (src->palette == nullptr) {
            return 0;
        }
        if (toRGB) {
            converter = dstBits == 0x10 ? 0x008C8670 : dstBits == 0x18 ? 0x008C8EC0 : dstBits == 0x20 ? 0x008C8960 : 0;
        } else {
            if (PixelFormat_Equals(target, Format(kPALA8)) == 0) {
                return 0;
            }
            CopyLevels(dst, src, first, last, faces);
            return CopyPaletteOpaque(dst, src);
        }
    } else if (PixelFormat_Equals(source, Format(kPALA8)) != 0) {
        if (src->palette == nullptr) {
            return 0;
        }
        if (toRGB) {
            converter = dstBits == 0x10 ? 0x008C8B80 : dstBits == 0x18 ? 0x008C8EC0 : dstBits == 0x20 ? 0x008C8FA0 : 0;
        } else {
            if (PixelFormat_Equals(target, Format(kPAL8)) == 0) {
                return 0;
            }
            CopyLevels(dst, src, first, last, faces);
            return CopyPaletteNoAlpha(dst, src, 0x100);
        }
    } else if (PixelFormat_Equals(source, Format(kRGB24)) != 0) {
        if (!toRGB) {
            return 0;
        }
        converter = dstBits == 0x10 ? 0x008C5BB0 : dstBits == 0x18 ? 0x008C5C90 : dstBits == 0x20 ? 0x008C5D20 : 0;
    } else {
        bool dst32 = PixelFormat_Equals(target, Format(kBGRA8888)) != 0 ||
                     PixelFormat_Equals(target, Format(kARGB8888)) != 0 ||
                     PixelFormat_Equals(target, Format(kRGBA32)) != 0;
        if (dst32 && (PixelFormat_Equals(source, Format(kBGRA8888)) != 0 ||
                      PixelFormat_Equals(source, Format(kARGB8888)) != 0 ||
                      PixelFormat_Equals(source, Format(kRGBA32)) != 0)) {
            converter = 0x008C5F90;
        } else if (PixelFormat_Equals(source, Format(kRGBA32)) != 0) {
            if (!toRGB) {
                return 0;
            }
            converter = dstBits == 0x10 ? 0x008C5DE0 : dstBits == 0x18 ? 0x008C5EC0 : dstBits == 0x20 ? 0x008C5F90 : 0;
        } else if (PixelFormat_Equals(source, Format(kBUMP16)) != 0) {
            if (dstCode != 8 && dstCode != 9) {
                return 0;
            }
            converter = dstBits == 0x10 ? 0x008C6780 : dstBits == 0x20 ? 0x008C66F0 : 0;
        } else if (PixelFormat_Equals(source, Format(kBUMPLUMA32)) != 0) {
            if (dstCode != 8 && dstCode != 9) {
                return 0;
            }
            converter = dstBits == 0x10 ? 0x008C6630 : dstBits == 0x20 ? 0x008C6590 : 0;
        } else if (PixelFormat_Equals(source, Format(kRGBA5551)) != 0 &&
                   PixelFormat_Equals(target, Format(kBGRA5551)) != 0) {
            converter = 0x008C5A80;
        } else if (PixelFormat_Equals(source, Format(kBGRA5551)) != 0 &&
                   PixelFormat_Equals(target, Format(kRGBA5551)) != 0) {
            converter = 0x008C5BA0;
        } else {
            if (srcCode != 0 && srcCode != 1) {
                return 0;
            }
            if (PixelFormat_Equals(target, Format(kRGBA32)) != 0) {
                if (srcBits == 0x10) {
                    converter = srcMasks.mask[3] != 0 ? 0x008C6130 : 0x008C61E0;
                } else if (srcBits == 0x18) {
                    converter = 0x008C6300;
                } else if (srcBits == 0x20) {
                    converter = srcMasks.mask[3] != 0 ? 0x008C6450 : 0x008C6500;
                }
            } else if (PixelFormat_Equals(target, Format(kRGB24)) != 0) {
                converter = srcBits == 0x10 ? 0x008C60A0 : srcBits == 0x18 ? 0x008C6270 : srcBits == 0x20 ? 0x008C63D0 : 0;
            }
        }
    }
    if (converter == 0) {
        return 0;
    }
    auto convert = reinterpret_cast<RowConverter>(converter);
    for (uint32_t face = 0; face < faces; face++) {
        for (uint32_t l = first; l <= last; l++) {
            convert(dst->widths[l], dst->heights[l], LevelSize(dst, l), LevelStart(dst, l, face), src->palette,
                    &dstMasks, LevelStart(src, l, face), &srcMasks);
        }
    }
    return 1;
}

// vf11 ReadMipmapFiles: the first file's size and format (one face only) for new pixel data of
// that many levels, each level read from its file (the given pixels for the first) and copied
// in (sizes must match); a palettized format takes the first file's palette
NiPixelData* __fastcall ReadMipmapFiles(NiDevImageConverter* self, re::Edx, const re::Ptr32<const char>* files, uint32_t count,
                                        NiPixelData* first) {
    if (files[0] == nullptr) {
        return nullptr;
    }
    NiPixelFormat format;
    PixelFormat_Construct(&format);
    uint8_t mipmaps;
    uint32_t width;
    uint32_t height;
    uint32_t faces;
    if (Virtual<uint8_t>(self, 2, files[0], &format, &mipmaps, &width, &height, &faces) == 0 || faces != 1) {
        return nullptr;
    }
    void* p = NiNew(sizeof(NiPixelData));
    if (p == nullptr) {
        return nullptr;
    }
    NiPixelData* out = PixelData_Construct(p, width, height, &format, count, 1);
    if (out == nullptr) {
        return nullptr;
    }
    for (uint32_t i = 0; i < count; i++) {
        if (files[i] == nullptr) {
            Virtual(out, 0, 1u);
            return nullptr;
        }
        NiPixelData* level = nullptr;
        NiPointer_Assign(&level, i == 0 && first != nullptr ? first
                                                            : Virtual<NiPixelData*>(self, 3, files[i],
                                                                                    static_cast<NiPixelData*>(nullptr)));
        if (level == nullptr) {
            Virtual(out, 0, 1u);
            return nullptr;
        }
        uint32_t size = LevelSize(out, i);
        if (size != level->offsets[1] - level->offsets[0]) {
            Virtual(out, 0, 1u);
            NiPointer_Destruct(&level);
            return nullptr;
        }
        CopyBytes(out->pixels + out->offsets[i], level->pixels + level->offsets[0], size);
        if (i == 0 && PixelFormat_IsPalettized(&format) != 0) {
            PixelData_SetPalette(out, level->palette);
        }
        NiPointer_Destruct(&level);
    }
    return out;
}

// the constructor: the image converter, the DXT decoder, and the readers (BMP, DDS, SGI,
// Targa, NIF: each put first, so NIF is asked first)
NiDevImageConverter* __fastcall Construct(NiDevImageConverter* self, re::Edx) {
    ImageConverter_Construct(self);
    self->vtable = reinterpret_cast<void*>(kNiDevImageConverterVtable);
    Decoder_Construct(&self->decoder);
    auto* readers = &self->readers;
    readers->count = 0;
    readers->head = nullptr;
    readers->tail = nullptr;
    readers->vtable = reinterpret_cast<void*>(kReaderListVtable);
    struct {
        uint32_t size;
        void*(__thiscall* construct)(void*);
    } const kReaders[] = {
        {0x80, BMPReader_Construct}, {0x74, DDSReader_Construct}, {0x70, SGIReader_Construct},
        {0xB0, TGAReader_Construct}, {0x430, NIFReader_Construct},
    };
    for (const auto& reader : kReaders) {
        void* p = NiNew(reader.size);
        ReadersAddHead(readers, static_cast<NiImageReader*>(p != nullptr ? reader.construct(p) : nullptr));
    }
    return self;
}

// ---- the DXT decoder

// DXT3's explicit alpha: each byte's two nibbles
void __fastcall DXT3Alpha(DXTDecoder* self, re::Edx, uint8_t* out) {
    for (int i = 0; i < 8; i++) {
        uint8_t value = *self->source;
        out[i * 8] = self->nibbles[value][0];
        out[i * 8 + 4] = self->nibbles[value][1];
        self->source++;
    }
}

// DXT5's interpolated alpha: two ends and six (or four, with 0 and 255) between, 3-bit indices
void __fastcall DXT5Alpha(DXTDecoder* self, re::Edx, uint8_t* out) {
    uint8_t palette[8];
    const uint8_t* source = self->source;
    uint32_t a0 = source[0];
    uint32_t a1 = source[1];
    palette[0] = uint8_t(a0);
    palette[1] = uint8_t(a1);
    self->source = source + 2;
    if (a1 < a0) {
        palette[2] = uint8_t((a1 + a0 * 6) / 7);
        palette[3] = uint8_t((a0 * 5 + a1 * 2) / 7);
        palette[4] = uint8_t((a1 * 3 + a0 * 4) / 7);
        palette[5] = uint8_t((a0 * 3 + a1 * 4) / 7);
        palette[6] = uint8_t((a1 * 5 + a0 * 2) / 7);
        palette[7] = uint8_t((a0 + a1 * 6) / 7);
    } else {
        palette[2] = uint8_t((a1 + a0 * 4) / 5);
        palette[3] = uint8_t((a0 * 3 + a1 * 2) / 5);
        palette[4] = uint8_t((a1 * 3 + a0 * 2) / 5);
        palette[5] = uint8_t((a0 + a1 * 4) / 5);
        palette[6] = 0;
        palette[7] = 0xFF;
    }
    for (int half = 0; half < 2; half++) {
        const uint8_t* b = self->source;
        uint8_t* o = out + half * 0x20;
        o[0x0] = palette[b[0] & 7];
        o[0x4] = palette[(b[0] >> 3) & 7];
        o[0x8] = palette[(b[1] & 1) * 4 + (b[0] >> 6)];
        o[0xC] = palette[(b[1] >> 1) & 7];
        o[0x10] = palette[(b[1] >> 4) & 7];
        o[0x14] = palette[(b[2] & 3) * 2 + (b[1] >> 7)];
        o[0x18] = palette[(b[2] >> 2) & 7];
        o[0x1C] = palette[b[2] >> 5];
        self->source += 3;
    }
}

// a 565 colour's channels as the game unpacks them (green keeps a bit of blue: 0xFE)
void Unpack565(uint16_t colour, uint8_t* rgb) {
    rgb[0] = uint8_t(colour >> 8) & 0xF8;
    rgb[1] = uint8_t(colour >> 3) & 0xFE;
    rgb[2] = uint8_t(colour << 3);
}

// DXT3/5's colours: always four (the ends and two thirds between), RGB only
void __fastcall DXTColor(DXTDecoder* self, re::Edx, uint8_t* out) {
    const uint8_t* source = self->source;
    uint16_t c0 = uint16_t(source[1] * 0x100 + source[0]);
    uint16_t c1 = uint16_t(source[3] * 0x100 + source[2]);
    self->source = source + 4;
    uint8_t palette[12];
    Unpack565(c0, palette);
    Unpack565(c1, palette + 3);
    for (int c = 0; c < 3; c++) {
        palette[6 + c] = uint8_t((uint32_t(palette[3 + c]) + uint32_t(palette[c]) * 2) / 3);
        palette[9 + c] = uint8_t((uint32_t(palette[c]) + uint32_t(palette[3 + c]) * 2) / 3);
    }
    for (int row = 0; row < 4; row++) {
        uint8_t bits = *self->source;
        for (int x = 0; x < 4; x++) {
            const uint8_t* colour = palette + ((bits >> (x * 2)) & 3) * 3;
            out[x * 4] = colour[0];
            out[x * 4 + 1] = colour[1];
            out[x * 4 + 2] = colour[2];
        }
        self->source++;
        out += 0x10;
    }
}

// DXT1's colours: four when the first end is the greater, else three and a transparent black
// (its colour the middle one's here)
void __fastcall DXT1Color(DXTDecoder* self, re::Edx, uint8_t* out) {
    const uint8_t* source = self->source;
    uint16_t c0 = uint16_t(source[1] * 0x100 + source[0]);
    self->source = source + 2;
    uint16_t c1 = uint16_t(source[3] * 0x100 + source[2]);
    self->source = source + 4;
    uint8_t palette[16];
    Unpack565(c0, palette);
    Unpack565(c1, palette + 4);
    palette[3] = 0xFF;
    palette[7] = 0xFF;
    palette[11] = 0xFF;
    if (c1 < c0) {
        for (int c = 0; c < 3; c++) {
            palette[8 + c] = uint8_t((uint32_t(palette[4 + c]) + uint32_t(palette[c]) * 2) / 3);
            palette[12 + c] = uint8_t((uint32_t(palette[c]) + uint32_t(palette[4 + c]) * 2) / 3);
        }
        palette[15] = 0xFF;
    } else {
        for (int c = 0; c < 3; c++) {
            palette[8 + c] = uint8_t((uint32_t(palette[4 + c]) + palette[c]) >> 1);
            palette[12 + c] = palette[8 + c];
        }
        palette[15] = 0;
    }
    for (int row = 0; row < 4; row++) {
        uint8_t bits = *self->source;
        for (int x = 0; x < 4; x++) {
            const uint8_t* colour = palette + ((bits >> (x * 2)) & 3) * 4;
            out[x * 4] = colour[0];
            out[x * 4 + 1] = colour[1];
            out[x * 4 + 2] = colour[2];
            out[x * 4 + 3] = colour[3];
        }
        self->source++;
        out += 0x10;
    }
}

// the constructor: DXT1's colour decoder, each byte's nibbles scaled to 8 bits (in float, the
// game's x87 truncating)
DXTDecoder* __fastcall DecoderConstruct(DXTDecoder* self, re::Edx) {
    self->alphaFn = nullptr;
    self->colorFn = reinterpret_cast<void*>(0x008CCB60);
    self->source = nullptr;
    self->pitch = 0;
    for (uint32_t i = 0; i < 0x100; i++) {
        self->nibbles[i][0] = uint8_t(int32_t(float(i & 0xF) / 15.0f * 255.0f));
        self->nibbles[i][1] = uint8_t(int32_t(float((i >> 4) & 0xF) / 15.0f * 255.0f));
    }
    return self;
}

// one block decoded (alpha, then colour) and the given columns and rows of it written out
void __fastcall WriteBlock(DXTDecoder* self, re::Edx, uint8_t* dst, uint32_t columns, uint32_t rows) {
    uint8_t block[64];
    if (self->alphaFn != nullptr) {
        re::PtrCast<void(__thiscall*)(DXTDecoder*, uint8_t*)>(self->alphaFn)(self, block + 3);
    }
    re::PtrCast<void(__thiscall*)(DXTDecoder*, uint8_t*)>(self->colorFn)(self, block);
    uint32_t bytes = columns * 4;
    const uint8_t* row = block;
    for (uint32_t r = rows; r != 0; r--) {
        crt::MemcpyS(dst, bytes, row, bytes);
        dst += self->pitch;
        row += 0x10;
    }
}

// DXT1/3/5 pixels decompressed to new RGBA32 pixel data, block by block (partial blocks at the
// right and bottom edges)
NiPixelData* __fastcall Decompress(DXTDecoder* self, re::Edx, const NiPixelData* src) {
    switch (src->format.format) {
    case 4:
        self->alphaFn = nullptr;
        self->colorFn = reinterpret_cast<void*>(0x008CCD40);
        break;
    case 5:
        self->alphaFn = reinterpret_cast<void*>(0x008CC740);
        self->colorFn = reinterpret_cast<void*>(0x008CCB60);
        break;
    case 6:
        self->alphaFn = reinterpret_cast<void*>(0x008CC870);
        self->colorFn = reinterpret_cast<void*>(0x008CCB60);
        break;
    default:
        return nullptr;
    }
    void* p = NiNew(sizeof(NiPixelData));
    NiPixelData* out = p != nullptr ? PixelData_Construct(p, src->widths[0], src->heights[0], Format(kRGBA32),
                                                          src->mipmapLevels, src->faces)
                                    : nullptr;
    uint32_t faces = src->faces;
    uint32_t levels = src->mipmapLevels;
    for (uint32_t face = 0; face < faces; face++) {
        for (uint32_t level = 0; level < levels; level++) {
            self->source = LevelStart(src, level, face);
            uint8_t* dst = LevelStart(out, level, face);
            uint32_t width = src->widths[level];
            uint32_t height = src->heights[level];
            uint32_t blocksX = width >> 2;
            uint32_t restX = width & 3;
            uint32_t blocksY = height >> 2;
            uint32_t restY = height & 3;
            self->pitch = width * 4;
            for (uint32_t by = blocksY; by != 0; by--) {
                for (uint32_t bx = blocksX; bx != 0; bx--) {
                    Decoder_WriteBlock(self, dst, 4, 4);
                    dst += 0x10;
                }
                if (restX != 0) {
                    Decoder_WriteBlock(self, dst, restX, 4);
                    dst += restX * 4;
                }
                dst += width * 0xC;
            }
            if (restY != 0) {
                for (uint32_t bx = blocksX; bx != 0; bx--) {
                    Decoder_WriteBlock(self, dst, 4, restY);
                    dst += 0x10;
                }
                if (restX != 0) {
                    Decoder_WriteBlock(self, dst, restX, restY);
                }
            }
        }
    }
    return out;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIDEVIMAGECONV_REVERSED(name, address, fn)                                             \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIDEVIMAGECONV_REVERSED("NiDevImageConverter::GenerateMipmaps", 0x008C7080, &ni::GenerateMipmaps);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::ConvertForPlatform", 0x008C7590, &ni::ConvertForPlatform);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::GetPlatformFormat", 0x008C7640, &ni::GetPlatformFormat);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::PlatformFormat6", 0x008C76F0, &ni::PlatformFormat6);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::PlatformFormat4", 0x008C7920, &ni::PlatformFormat4);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::PlatformFormat5", 0x008C7A60, &ni::PlatformFormat5);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::ConvertToBump", 0x008C7C50, &ni::ConvertToBump);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::Pal4To32", 0x008C7FE0, &ni::Pal4To32);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::Pal4To24", 0x008C8260, &ni::Pal4To24);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::PalA4To32", 0x008C83A0, &ni::PalA4To32);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::Pal8To16", 0x008C8670, &ni::Pal8To16);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::Pal8To32", 0x008C8960, &ni::Pal8To32);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::PalA8To16", 0x008C8B80, &ni::PalA8To16);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::Pal8To24", 0x008C8EC0, &ni::Pal8To24);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::PalA8To32", 0x008C8FA0, &ni::PalA8To32);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::DeletingDestructor", 0x008C9210, &ni::DeletingDestructor);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::AddReader", 0x008C9240, &ni::AddReader);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::ConvertPixelData", 0x008C9290, &ni::ConvertPixelData);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::ConvertFormat", 0x008C9600, &ni::ConvertFormat);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::ReadMipmapFiles", 0x008CA040, &ni::ReadMipmapFiles);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::NiDevImageConverter", 0x008CA1D0, &ni::Construct);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::DXTDecoder::DXT3Alpha", 0x008CC740, &ni::DXT3Alpha);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::DXTDecoder::DXT5Alpha", 0x008CC870, &ni::DXT5Alpha);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::DXTDecoder::DXTColor", 0x008CCB60, &ni::DXTColor);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::DXTDecoder::DXT1Color", 0x008CCD40, &ni::DXT1Color);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::DXTDecoder::DXTDecoder", 0x008CCFB0, &ni::DecoderConstruct);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::DXTDecoder::WriteBlock", 0x008CD080, &ni::WriteBlock);
NIDEVIMAGECONV_REVERSED("NiDevImageConverter::DXTDecoder::Decompress", 0x008CD100, &ni::Decompress);
// the compiler's exception handler stub of a function past the converter (mov eax, FuncInfo;
// jmp __CxxFrameHandler3): not a function of its own
RE_UNHOOKED("__ehhandler$0x8D9BB0", 0x008D9BB0);
