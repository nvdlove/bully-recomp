// NiPixelFormat and NiPixelData (src/gamebryo/NiPixelData.h): the formats (from presets or
// channel masks), their comparison and streaming; the pixel data's construction (the mipmap
// chain's sizes), copying, comparison, byte order swap, streaming.

#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiPixelData.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiPixelData_RTTI, 0x020E2A44);
GAME_ARRAY(uint32_t, NiPixelFormat_StaticTable, 0x020E2AAC);

constexpr uintptr_t NiPixelData_vtable = 0x009525D0;

// the preset formats (NiPixelFormat's statics)
GAME_VAR(NiPixelFormat, PixelFormat_PAL8, 0x00BC76C0);
GAME_VAR(NiPixelFormat, PixelFormat_PALA8, 0x00BC7708);
GAME_VAR(NiPixelFormat, PixelFormat_RGBA32, 0x00BC7750);
GAME_VAR(NiPixelFormat, PixelFormat_RGB24, 0x00BC7798);
GAME_VAR(NiPixelFormat, PixelFormat_RGBA5551, 0x00BC77E0);
GAME_VAR(NiPixelFormat, PixelFormat_BGRA5551, 0x00BC7828);
GAME_VAR(NiPixelFormat, PixelFormat_BUMP16, 0x00BC7870);
GAME_VAR(NiPixelFormat, PixelFormat_BUMPLUMA32, 0x00BC78B8);
GAME_VAR(NiPixelFormat, PixelFormat_DXT1, 0x00BC7900);
GAME_VAR(NiPixelFormat, PixelFormat_DXT3, 0x00BC7948);
GAME_VAR(NiPixelFormat, PixelFormat_DXT5, 0x00BC7990);
GAME_VAR(NiPixelFormat, PixelFormat_RGB565, 0x00BC8008);
GAME_VAR(NiPixelFormat, PixelFormat_RGBA4444, 0x00BC8128);
GAME_VAR(NiPixelFormat, PixelFormat_BGR888, 0x00BC81B8);
GAME_VAR(NiPixelFormat, PixelFormat_BGRA8888, 0x00BC8200);

GAME_FN(void, __thiscall, Object_DoNothing, 0x008A20A0, void* self, void* stream);
GAME_FN(NiObject*, __thiscall, Palette_CopyConstruct, 0x007D4560, void* self, NiObject* other);
GAME_FN(void, __thiscall, PixelData_AllocateData, 0x007D41E0, NiPixelData* self, uint32_t levels, uint32_t faces,
        uint32_t bytes);
GAME_FN(void, __thiscall, PixelData_FreeData, 0x007D4230, NiPixelData* self);
GAME_FN(void, __cdecl, NiEndian_Swap64, 0x00752A00, void* data, uint32_t count);
// (this chunk's, called where modes apply)
GAME_FN(uint32_t, __cdecl, PixelData_ComputeMipmapLevels, 0x00769FD0, uint32_t width, uint32_t height);
GAME_FN(uint8_t, __thiscall, PixelFormat_NotEqual, 0x0076A090, const NiPixelFormat* self, const NiPixelFormat* other);
GAME_FN(NiPixelFormat*, __thiscall, PixelFormat_Construct, 0x0076B280, NiPixelFormat* self);
GAME_FN(void, __thiscall, PixelFormat_ConstructWith, 0x0076AFD0, NiPixelFormat* self, int32_t format, int bits,
        const uint32_t* masks);
GAME_FN(void, __thiscall, PixelFormat_LoadBinary, 0x0076B520, NiPixelFormat* self, void* stream);
GAME_FN(void, __thiscall, PixelFormat_SaveBinary, 0x0076B870, NiPixelFormat* self, void* stream);
GAME_FN(void, __thiscall, PixelData_Destruct, 0x0076A820, NiPixelData* self);

// one read or write of `bytes` in one component of `componentSize`
inline void LoadRaw(void* stream, void* buffer, uint32_t bytes, uint32_t componentSize) {
    BinaryStream* in = At<BinaryStream*>(stream, 0x188);
    in->position += in->read(in, buffer, bytes, &componentSize, 1);
}
inline void SaveRaw(void* stream, const void* buffer, uint32_t bytes, uint32_t componentSize) {
    BinaryStream* out = At<BinaryStream*>(stream, 0x18C);
    out->position += out->write(out, buffer, bytes, &componentSize, 1);
}

// NiPointer::operator=(T*) as inlined here: the old one let go first
inline void AssignReleaseFirst(re::Ptr32<NiObject>& slot, NiObject* value) {
    NiObject* old = slot;
    if (old != value) {
        Release(old);
        slot = value;
        AddRef(value);
    }
}

// ---- NiPixelFormat

// GetComponent(uiComponent, eComponent, eRepresentation, ucBitsPerComponent, bSigned): false
// for an empty one
uint8_t __fastcall PixelFormatGetComponent(const NiPixelFormat* self, re::Edx, uint32_t index, uint32_t* component,
                                           uint32_t* representation, uint8_t* bits, uint8_t* isSigned) {
    const NiPixelComponent& c = self->components[index];
    *isSigned = c.isSigned;
    *bits = c.bitsPerComponent;
    *representation = c.representation;
    uint32_t value = c.component;
    *component = value;
    return value != 0x13 ? 1 : 0;
}

// GetNumComponents: up to the first empty one
uint32_t __fastcall PixelFormatGetNumComponents(const NiPixelFormat* self, re::Edx) {
    for (uint32_t i = 0; i < 4; i++) {
        if (self->components[i].component == 0x13) {
            return i;
        }
    }
    return 4;
}

// the components the same (the type, the representation, the bits, the sign)
inline bool ComponentsEqual(const NiPixelFormat* a, const NiPixelFormat* b) {
    for (int i = 0; i < 4; i++) {
        const NiPixelComponent& x = a->components[i];
        const NiPixelComponent& y = b->components[i];
        if (x.component != y.component || x.representation != y.representation ||
            x.bitsPerComponent != y.bitsPerComponent || x.isSigned != y.isSigned) {
            return false;
        }
    }
    return true;
}

// a renderer hint of 0xFFFFFFFF matches any
inline bool HintsMatch(uint32_t a, uint32_t b) {
    return a == b || b == 0xFFFFFFFF || a == 0xFFFFFFFF;
}

// operator!= (the whole flags byte compared)
uint8_t __fastcall PixelFormatNotEqual(const NiPixelFormat* self, re::Edx, const NiPixelFormat* other) {
    if (other->format != self->format || other->flags != self->flags || other->tiling != self->tiling ||
        other->bitsPerPixel != self->bitsPerPixel || !HintsMatch(other->rendererHint, self->rendererHint)) {
        return 1;
    }
    return ComponentsEqual(other, self) ? 0 : 1;
}

// operator== (the flags' endian bit not compared; the sRGB flag is)
uint8_t __fastcall PixelFormatIsEqual(const NiPixelFormat* self, re::Edx, const NiPixelFormat* other) {
    if (other->format != self->format || ((other->flags ^ self->flags) & 0xFE) != 0 ||
        other->tiling != self->tiling || other->srgb != self->srgb || other->bitsPerPixel != self->bitsPerPixel ||
        !HintsMatch(other->rendererHint, self->rendererHint)) {
        return 0;
    }
    return ComponentsEqual(other, self) ? 1 : 0;
}

// NiPixelFormat(eFormat, ucBits, auiMasks): one of the presets when the format, size and masks
// match one; else the format becomes 0x10 (the components left as they were)
void __fastcall PixelFormatConstructWith(NiPixelFormat* self, re::Edx, int32_t format, int bits,
                                         const uint32_t* masks) {
    uint8_t bpp = uint8_t(bits);
    self->bitsPerPixel = bpp;
    self->srgb = 0;
    self->format = format;
    self->rendererHint = 0xFFFFFFFF;
    self->extraData = 0;
    self->flags = 1;
    self->tiling = 0;
    const NiPixelFormat* preset = nullptr;
    switch (format) {
    case 0:
        if (bpp == 16) {
            if (masks[0] == 0xF800 && masks[1] == 0x7E0 && masks[2] == 0x1F && masks[3] == 0) {
                preset = &PixelFormat_RGB565;
            }
        } else if (bpp == 24) {
            if (masks[0] == 0xFF && masks[1] == 0xFF00 && masks[2] == 0xFF0000 && masks[3] == 0) {
                preset = &PixelFormat_RGB24;
            } else if (masks[0] == 0xFF0000 && masks[1] == 0xFF00 && masks[2] == 0xFF && masks[3] == 0) {
                preset = &PixelFormat_BGR888;
            }
        }
        break;
    case 1:
        if (bpp == 16) {
            if (masks[0] == 0x1F && masks[1] == 0x3E0 && masks[2] == 0x7C00 && masks[3] == 0x8000) {
                preset = &PixelFormat_RGBA5551;
            } else if (masks[0] == 0x7C00 && masks[1] == 0x3E0 && masks[2] == 0x1F && masks[3] == 0x8000) {
                preset = &PixelFormat_BGRA5551;
            } else if (masks[0] == 0xF00 && masks[1] == 0xF0 && masks[2] == 0xF && masks[3] == 0xF000) {
                preset = &PixelFormat_RGBA4444;
            }
        } else if (bpp == 32) {
            if (masks[0] == 0xFF0000 && masks[1] == 0xFF00 && masks[2] == 0xFF && masks[3] == 0xFF000000) {
                preset = &PixelFormat_BGRA8888;
            } else {
                preset = &PixelFormat_RGBA32;
            }
        }
        break;
    case 2:
        if (bpp == 8) {
            preset = &PixelFormat_PAL8;
        }
        break;
    case 3:
        if (bpp == 8) {
            preset = &PixelFormat_PALA8;
        }
        break;
    case 4:
        preset = &PixelFormat_DXT1;
        break;
    case 5:
        preset = &PixelFormat_DXT3;
        break;
    case 6:
        preset = &PixelFormat_DXT5;
        break;
    case 8:
        preset = &PixelFormat_BUMP16;
        break;
    case 9:
        preset = &PixelFormat_BUMPLUMA32;
        break;
    default:
        break;
    }
    if (preset != nullptr) {
        std::memcpy(self, preset, sizeof(NiPixelFormat));
        return;
    }
    self->format = 0x10;
}

// NiPixelFormat(): 8-bit RGB (format 2's defaults: one 8-bit component 0x10, the rest empty)
NiPixelFormat* __fastcall PixelFormatConstruct(NiPixelFormat* self, re::Edx) {
    self->bitsPerPixel = 8;
    self->components[0].bitsPerComponent = 8;
    self->srgb = 0;
    self->format = 2;
    self->rendererHint = 0xFFFFFFFF;
    self->extraData = 0;
    self->flags = 1;
    self->tiling = 0;
    self->components[0].component = 0x10;
    self->components[0].representation = 3;
    self->components[0].isSigned = 0;
    for (int i = 1; i < 4; i++) {
        self->components[i].component = 0x13;
        self->components[i].representation = 5;
        self->components[i].bitsPerComponent = 0;
        self->components[i].isSigned = 0;
    }
    return self;
}

// CreateFromRGBAMasks(ucBits, uiRed, uiGreen, uiBlue, uiAlpha): RGBA when there's an alpha
// mask; the red, green, blue channels ordered by their masks (lowest first), each with its
// mask's bit count; the empty ones last
NiPixelFormat* __cdecl PixelFormatCreateFromRGBAMasks(NiPixelFormat* result, int bits, uint32_t red,
                                                      uint32_t green, uint32_t blue, uint32_t alpha) {
    PixelFormat_Construct(result);
    result->flags |= 1;
    result->bitsPerPixel = uint8_t(bits);
    result->rendererHint = 0xFFFFFFFF;
    result->tiling = 0;
    result->format = alpha != 0 ? 1 : 0;

    uint32_t masks[4] = {red, green, blue, alpha};
    uint8_t counts[4];
    uint32_t channels[4] = {0, 1, 2, 3};
    for (int i = 0; i < 4; i++) {
        uint8_t count = 0;
        for (uint32_t m = masks[i]; m != 0; m >>= 1) {
            count = uint8_t(count + (m & 1));
        }
        counts[i] = count;
    }
    for (int i = 0; i < 3; i++) {
        if (masks[i] == 0 || i >= 2) {
            continue;
        }
        for (int j = i + 1; j < 3; j++) {
            if (masks[i] > masks[j]) {
                uint32_t m = masks[i];
                masks[i] = masks[j];
                masks[j] = m;
                uint8_t c = counts[i];
                counts[i] = counts[j];
                counts[j] = c;
                uint32_t ch = channels[i];
                channels[i] = channels[j];
                channels[j] = ch;
            }
        }
    }
    uint32_t n = 0;
    for (int i = 0; i < 4; i++) {
        if (masks[i] != 0) {
            NiPixelComponent& c = result->components[n];
            c.bitsPerComponent = counts[i];
            c.isSigned = 0;
            c.representation = 0;
            c.component = channels[i];
            n++;
        }
    }
    for (; n < 4; n++) {
        NiPixelComponent& c = result->components[n];
        c.isSigned = 0;
        c.bitsPerComponent = 0;
        c.representation = 5;
        c.component = 0x13;
    }
    return result;
}

// LoadBinary: before 10.3.0.3 the format, the channel masks, the size and three old words,
// rebuilt through the preset constructor; after it, every field (the sRGB flag from
// 20.3.0.4; the components' sign guessed from their type before that)
void __fastcall PixelFormatLoadBinary(NiPixelFormat* self, re::Edx, void* stream) {
    if (StreamVersion(stream) < 0x0A030003) {
        uint32_t format;
        LoadBytes(stream, &format, 4);
        self->format = int32_t(format);
        uint32_t masks[4];
        for (int i = 0; i < 4; i++) {
            LoadBytes(stream, &masks[i], 4);
        }
        uint32_t bits;
        LoadBytes(stream, &bits, 4);
        self->bitsPerPixel = uint8_t(bits);
        uint32_t unused;
        LoadBytes(stream, &unused, 4);
        LoadBytes(stream, &unused, 4);
        LoadBytes(stream, &unused, 4);
        PixelFormat_ConstructWith(self, self->format, self->bitsPerPixel, masks);
        return;
    }
    uint32_t format;
    LoadBytes(stream, &format, 4);
    self->format = int32_t(format);
    LoadBytes(stream, &self->bitsPerPixel, 1);
    LoadBytes(stream, &self->rendererHint, 4);
    LoadBytes(stream, &self->extraData, 4);
    LoadBytes(stream, &self->flags, 1);
    uint32_t tiling;
    LoadBytes(stream, &tiling, 4);
    self->tiling = tiling;
    if (StreamVersion(stream) >= 0x14030004) {
        uint8_t srgb = 0;
        LoadBytes(stream, &srgb, 1);
        self->srgb = srgb != 0 ? 1 : 0;
    } else {
        self->srgb = 0;
    }
    for (int i = 0; i < 4; i++) {
        NiPixelComponent& c = self->components[i];
        uint32_t value;
        LoadBytes(stream, &value, 4);
        c.component = value;
        LoadBytes(stream, &value, 4);
        c.representation = value;
        LoadBytes(stream, &c.bitsPerComponent, 1);
        uint8_t isSigned;
        LoadBytes(stream, &isSigned, 1);
        if (StreamVersion(stream) >= 0x14030004) {
            c.isSigned = isSigned != 0 ? 1 : 0;
        } else {
            uint32_t type = c.component;
            c.isSigned = (type == 5 || type == 6 || type == 7 || type == 8 || c.representation == 2 ||
                          c.representation == 1)
                             ? 1
                             : 0;
        }
    }
}

void __fastcall PixelFormatSaveBinary(NiPixelFormat* self, re::Edx, void* stream) {
    SaveBytes(stream, &self->format, 4);
    SaveBytes(stream, &self->bitsPerPixel, 1);
    SaveBytes(stream, &self->rendererHint, 4);
    SaveBytes(stream, &self->extraData, 4);
    SaveBytes(stream, &self->flags, 1);
    SaveBytes(stream, &self->tiling, 4);
    uint8_t srgb = self->srgb;
    SaveBytes(stream, &srgb, 1);
    for (int i = 0; i < 4; i++) {
        NiPixelComponent& c = self->components[i];
        SaveBytes(stream, &c.component, 4);
        SaveBytes(stream, &c.representation, 4);
        SaveBytes(stream, &c.bitsPerComponent, 1);
        uint8_t isSigned = c.isSigned;
        SaveBytes(stream, &isSigned, 1);
    }
}

// an entry of a static table by index (its address)
uint32_t* __cdecl PixelFormatStaticEntry(int index) {
    return &NiPixelFormat_StaticTable[index];
}

// ---- NiPixelData

// ComputeMipmapLevels(uiWidth, uiHeight): the longer side's bit length (0 for an empty side)
uint32_t __cdecl ComputeMipmapLevels(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) {
        return 0;
    }
    uint32_t widthLevels = 1;
    while ((width >>= 1) != 0) {
        widthLevels++;
    }
    uint32_t heightLevels = 1;
    while ((height >>= 1) != 0) {
        heightLevels++;
    }
    return heightLevels < widthLevels ? widthLevels : heightLevels;
}

// vf5 LinkObject: the palette
void __fastcall LinkObject(NiPixelData* self, re::Edx, void* stream) {
    Object_DoNothing(self, stream);
    AssignReleaseFirst(self->palette, static_cast<NiObject*>(Stream_ResolveLinkID(stream)));
}

// vf6 RegisterStreamables (the palette's answer ignored)
uint8_t __fastcall RegisterStreamables(NiPixelData* self, re::Edx, void* stream) {
    if (!Object_RegisterStreamables(self, stream)) {
        return 0;
    }
    if (self->palette != nullptr) {
        Virtual<uint8_t>(self->palette, 6, stream);
    }
    return 1;
}

// vf8 IsEqual: the format, the palette (when this one has one), the levels' sizes, the faces,
// and a face's worth of pixels
uint8_t __fastcall IsEqual(NiPixelData* self, re::Edx, NiPixelData* other) {
    if (!Object_IsEqual(self, other)) {
        return 0;
    }
    if (PixelFormat_NotEqual(&self->format, &other->format)) {
        return 0;
    }
    if (self->palette != nullptr && !Virtual<uint8_t>(self->palette, 8, other->palette)) {
        return 0;
    }
    uint32_t levels = self->mipmapLevels;
    if (levels != other->mipmapLevels || self->pixelStride != other->pixelStride) {
        return 0;
    }
    for (uint32_t i = 0; i < levels; i++) {
        if (self->widths[i] != other->widths[i] || self->heights[i] != other->heights[i] ||
            self->offsets[i] != other->offsets[i]) {
            return 0;
        }
    }
    if (self->offsets[levels] != other->offsets[levels] || self->faces != other->faces) {
        return 0;
    }
    return std::memcmp(self->pixels, other->pixels, self->offsets[levels]) == 0 ? 1 : 0;
}

// SwapEndian: every channel 16, 32 or 64 bits wide (or a 16-bit pixel), each value's bytes
// reversed, the format's endian flag toggled; false when the channels differ in size
uint8_t __fastcall SwapEndian(NiPixelData* self, re::Edx) {
    uint32_t size = 0;
    bool uniform = true;
    for (uint32_t i = 0; i < PixelFormatGetNumComponents(&self->format, nullptr); i++) {
        const NiPixelComponent& c = self->format.components[i];
        uint8_t bits = c.bitsPerComponent;
        if (c.component != 0x13) {
            if (size == 0) {
                size = bits;
            }
            if (size != bits) {
                uniform = false;
            }
        }
    }
    if (self->format.bitsPerPixel == 16) {
        size = 16;
    } else if (!uniform || (size != 16 && size != 32 && size != 64)) {
        return 0;
    }
    for (uint32_t face = 0; face < self->faces; face++) {
        for (uint32_t level = 0; level < self->mipmapLevels; level++) {
            uint32_t* offsets = self->offsets;
            uint8_t* p = self->pixels + offsets[self->mipmapLevels] * face + offsets[level];
            uint32_t count = ((offsets[level + 1] - offsets[level]) * 8) / size;
            if (size == 16) {
                for (; count != 0; count--) {
                    uint8_t b = p[0];
                    p[0] = p[1];
                    p[1] = b;
                    p += 2;
                }
            } else if (size == 32) {
                for (; count != 0; count--) {
                    uint8_t b = p[0];
                    p[0] = p[3];
                    p[3] = b;
                    b = p[1];
                    p[1] = p[2];
                    p[2] = b;
                    p += 4;
                }
            } else if (size == 64) {
                NiEndian_Swap64(p, count);
            }
        }
    }
    self->format.flags ^= 1;
    return 1;
}

const RTTI* __fastcall GetRTTI(NiPixelData*, re::Edx) {
    return &NiPixelData_RTTI;
}

// NiPixelData(kSrc): the format, a copy of the palette, the sizes; only the first face's
// pixels copied
NiPixelData* __fastcall CopyConstruct(NiPixelData* self, re::Edx, const NiPixelData* other) {
    RefObject_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiPixelData_vtable);
    PixelFormat_Construct(&self->format);
    self->palette = nullptr;
    std::memcpy(&self->format, &other->format, sizeof(NiPixelFormat));
    if (other->palette != nullptr) {
        void* p = NiNew(0x24);
        NiObject* palette = p != nullptr ? Palette_CopyConstruct(p, other->palette) : nullptr;
        AssignReleaseFirst(self->palette, palette);
    }
    uint32_t levels = other->mipmapLevels;
    self->mipmapLevels = levels;
    self->faces = other->faces;
    self->pixelStride = other->pixelStride;
    uint32_t offsets[17];
    uint32_t widths[16];
    uint32_t heights[16];
    for (uint32_t i = 0; i < self->mipmapLevels; i++) {
        offsets[i] = other->offsets[i];
        heights[i] = other->heights[i];
        widths[i] = other->widths[i];
    }
    levels = self->mipmapLevels;
    uint32_t faceSize = other->offsets[levels];
    offsets[levels] = faceSize;
    PixelData_AllocateData(self, levels, self->faces, faceSize);
    uint32_t bytes = self->mipmapLevels * 4;
    crt::MemcpyS(self->widths, bytes, widths, bytes);
    crt::MemcpyS(self->heights, bytes, heights, bytes);
    bytes = self->mipmapLevels * 4 + 4;
    crt::MemcpyS(self->offsets, bytes, offsets, bytes);
    crt::MemcpyS(self->pixels, faceSize, other->pixels, faceSize);
    self->revisionID = 1;
    return self;
}

// NiPixelData(uiWidth, uiHeight, kFormat, uiMipmapLevels, uiFaces): the chain's sizes (no
// levels: the full chain), each level at least 1x1; block-compressed formats in 4x4 blocks of 8
// (DXT1) or 16 bytes
NiPixelData* __fastcall Construct(NiPixelData* self, re::Edx, uint32_t width, uint32_t height,
                                  const NiPixelFormat* format, uint32_t levels, uint32_t faces) {
    RefObject_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiPixelData_vtable);
    PixelFormat_Construct(&self->format);
    self->palette = nullptr;
    std::memcpy(&self->format, format, sizeof(NiPixelFormat));
    if (levels == 0) {
        levels = PixelData_ComputeMipmapLevels(width, height);
    }
    self->mipmapLevels = levels;
    self->faces = faces;
    uint32_t offsets[17];
    uint32_t widths[16];
    uint32_t heights[16];
    offsets[0] = 0;
    int32_t kind = format->format;
    if (kind < 4 || kind > 6) {
        uint32_t stride = uint32_t(format->bitsPerPixel) >> 3;
        self->pixelStride = stride;
        for (uint32_t i = 0; i < self->mipmapLevels; i++) {
            uint32_t w = width != 0 ? width : 1;
            widths[i] = w;
            uint32_t h = height != 0 ? height : 1;
            heights[i] = h;
            if (stride == 0) {
                offsets[i + 1] = ((h * w) >> 1) + offsets[i];   // 4 bits a pixel
            } else {
                offsets[i + 1] = stride * h * w + offsets[i];
            }
            width >>= 1;
            height >>= 1;
        }
    } else {
        self->pixelStride = 0;
        uint32_t blockBytes = format->format != 4 ? 16 : 8;
        for (uint32_t i = 0; i < self->mipmapLevels; i++) {
            uint32_t w = width != 0 ? width : 1;
            widths[i] = w;
            uint32_t h = height != 0 ? height : 1;
            heights[i] = h;
            offsets[i + 1] = (((h + 3) & ~3u) * ((w + 3) & ~3u) >> 4) * blockBytes + offsets[i];
            width >>= 1;
            height >>= 1;
        }
    }
    levels = self->mipmapLevels;
    PixelData_AllocateData(self, levels, faces, offsets[levels]);
    uint32_t bytes = self->mipmapLevels * 4;
    crt::MemcpyS(self->widths, bytes, widths, bytes);
    crt::MemcpyS(self->heights, bytes, heights, bytes);
    bytes = self->mipmapLevels * 4 + 4;
    crt::MemcpyS(self->offsets, bytes, offsets, bytes);
    self->revisionID = 1;
    return self;
}

// ~NiPixelData
void __fastcall Destruct(NiPixelData* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiPixelData_vtable);
    if (self->palette != nullptr) {
        Release(self->palette);
        self->palette = nullptr;
    }
    PixelData_FreeData(self);
    Release(self->palette);
    Object_Destruct(self);
}

// vf4 LoadBinary: the format, the palette's link, the levels' sizes, the faces (one before
// 10.3.0.6), then every face's pixels in one read
void __fastcall LoadBinary(NiPixelData* self, re::Edx, void* stream) {
    Object_LoadBinary(self, stream);
    PixelFormat_LoadBinary(&self->format, stream);
    Stream_ReadLinkID(stream);
    LoadBytes(stream, &self->mipmapLevels, 4);
    LoadBytes(stream, &self->pixelStride, 4);
    uint32_t offsets[17];
    uint32_t widths[16];
    uint32_t heights[16];
    for (uint32_t i = 0; i < self->mipmapLevels; i++) {
        LoadBytes(stream, &widths[i], 4);
        LoadBytes(stream, &heights[i], 4);
        LoadBytes(stream, &offsets[i], 4);
    }
    LoadBytes(stream, &offsets[self->mipmapLevels], 4);
    if (StreamVersion(stream) < 0x0A030006) {
        self->faces = 1;
    } else {
        LoadBytes(stream, &self->faces, 4);
    }
    uint32_t levels = self->mipmapLevels;
    PixelData_AllocateData(self, levels, self->faces, offsets[levels]);
    uint32_t bytes = self->mipmapLevels * 4;
    crt::MemcpyS(self->widths, bytes, widths, bytes);
    crt::MemcpyS(self->heights, bytes, heights, bytes);
    bytes = self->mipmapLevels * 4 + 4;
    crt::MemcpyS(self->offsets, bytes, offsets, bytes);
    LoadRaw(stream, self->pixels, self->offsets[self->mipmapLevels] * self->faces, 1);
}

// vf7 SaveBinary
void __fastcall SaveBinary(NiPixelData* self, re::Edx, void* stream) {
    Object_DoNothing(self, stream);
    PixelFormat_SaveBinary(&self->format, stream);
    Virtual<void>(stream, 12, self->palette);   // SaveLinkID
    SaveBytes(stream, &self->mipmapLevels, 4);
    SaveBytes(stream, &self->pixelStride, 4);
    for (uint32_t i = 0; i < self->mipmapLevels; i++) {
        SaveBytes(stream, &self->widths[i], 4);
        SaveBytes(stream, &self->heights[i], 4);
        SaveBytes(stream, &self->offsets[i], 4);
    }
    SaveBytes(stream, &self->offsets[self->mipmapLevels], 4);
    SaveBytes(stream, &self->faces, 4);
    SaveRaw(stream, self->pixels, self->offsets[self->mipmapLevels] * self->faces, 1);
}

NiPixelData* __fastcall DeletingDestructor(NiPixelData* self, re::Edx, uint32_t flags) {
    PixelData_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x70);
    }
    return self;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIPIXEL_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIPIXEL_REVERSED("NiPixelData::ComputeMipmapLevels", 0x00769FD0, &ni::ComputeMipmapLevels);
NIPIXEL_REVERSED("NiPixelFormat::GetComponent", 0x0076A020, &ni::PixelFormatGetComponent);
NIPIXEL_REVERSED("NiPixelFormat::GetNumComponents", 0x0076A070, &ni::PixelFormatGetNumComponents);
NIPIXEL_REVERSED("NiPixelFormat::operator!=", 0x0076A090, &ni::PixelFormatNotEqual);
NIPIXEL_REVERSED("NiPixelData::LinkObject", 0x0076A120, &ni::LinkObject);
NIPIXEL_REVERSED("NiPixelData::RegisterStreamables", 0x0076A180, &ni::RegisterStreamables);
NIPIXEL_REVERSED("NiPixelData::IsEqual", 0x0076A1B0, &ni::IsEqual);
NIPIXEL_REVERSED("NiPixelData::SwapEndian", 0x0076A310, &ni::SwapEndian);
NIPIXEL_REVERSED("NiPixelData::GetRTTI", 0x0076A490, &ni::GetRTTI);
NIPIXEL_REVERSED("NiPixelData::NiPixelDataCopy", 0x0076A4A0, &ni::CopyConstruct);
NIPIXEL_REVERSED("NiPixelData::NiPixelData", 0x0076A610, &ni::Construct);
NIPIXEL_REVERSED("NiPixelData::~NiPixelData", 0x0076A820, &ni::Destruct);
NIPIXEL_REVERSED("NiPixelData::LoadBinary", 0x0076A8E0, &ni::LoadBinary);
NIPIXEL_REVERSED("NiPixelData::SaveBinary", 0x0076AAE0, &ni::SaveBinary);
NIPIXEL_REVERSED("NiPixelData::DeletingDestructor", 0x0076AC90, &ni::DeletingDestructor);
NIPIXEL_REVERSED("NiPixelFormat::operator==", 0x0076AE80, &ni::PixelFormatIsEqual);
NIPIXEL_REVERSED("NiPixelFormat::NiPixelFormatWith", 0x0076AFD0, &ni::PixelFormatConstructWith);
NIPIXEL_REVERSED("NiPixelFormat::NiPixelFormat", 0x0076B280, &ni::PixelFormatConstruct);
NIPIXEL_REVERSED("NiPixelFormat::CreateFromRGBAMasks", 0x0076B370, &ni::PixelFormatCreateFromRGBAMasks);
NIPIXEL_REVERSED("NiPixelFormat::LoadBinary", 0x0076B520, &ni::PixelFormatLoadBinary);
NIPIXEL_REVERSED("NiPixelFormat::SaveBinary", 0x0076B870, &ni::PixelFormatSaveBinary);
NIPIXEL_REVERSED("NiPixelFormat::StaticEntry", 0x0076BA30, &ni::PixelFormatStaticEntry);
