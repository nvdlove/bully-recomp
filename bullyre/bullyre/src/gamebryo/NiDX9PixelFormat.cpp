// The DX9 renderer's conversions between Gamebryo's NiPixelFormat (0x44 bytes: +0x0 flags, +0x1
// bits per pixel, +0x4 the format (0 RGB, 1 RGBA, 2 palettized, 4..6 DXT1/3/5, 8/9 bump maps, 11
// single channel, 12 two channel, 14 floating point, 15 depth / stencil...), +0xC the renderer's
// own format (0xFFFFFFFF unset), +0x10, the components from +0x14) and D3DFORMAT: the format a
// pixel format is, by its layout, bits and channel masks; the pixel format of a D3DFORMAT (one of
// the library's static NiPixelFormats, 0xBC76C0.., copied, the D3D format kept as its own).

#include "gamebryo/NiDX9RendererData.h"

namespace ni::c40 {
namespace {

GAME_FN(uint32_t, __thiscall, PixelFormat_GetMask, 0x007548E0, const void* self, uint32_t component);
GAME_FN(uint8_t, __thiscall, PixelFormat_GetBits, 0x007DD390, const void* self, uint32_t component);
GAME_FN(void*, __thiscall, PixelFormat_Construct, 0x0076B280, void* self);
// this file's (called where modes apply)
GAME_FN(void, __cdecl, PixelFormat_FromD3DFormat, 0x00895800, uint32_t format, void* out);

constexpr uint32_t kRed = 0;
constexpr uint32_t kAlpha = 9;
constexpr uint32_t kComponent3 = 3;
constexpr uint32_t kDepth = 0x11;
constexpr uint32_t kStencil = 0x12;

// the static NiPixelFormat the library describes a D3DFORMAT with
uintptr_t StaticFormatFor(uint32_t format) {
    switch (format) {
    case 0x31545844:   // DXT1
        return 0x00BC7900;
    case 0x33545844:   // DXT3
        return 0x00BC7948;
    case 0x35545844:   // DXT5
        return 0x00BC7990;
    case 0x14:
        return 0x00BC81B8;
    case 0x15:
        return 0x00BC8200;
    case 0x16:
        return 0x00BC8290;
    case 0x17:
        return 0x00BC8008;
    case 0x18:
        return 0x00BC80E0;
    case 0x19:
        return 0x00BC8098;
    case 0x1A:
        return 0x00BC8128;
    case 0x1B:
        return 0x00BC7EE8;
    case 0x1C:
        return 0x00BC7EA0;
    case 0x1D:
        return 0x00BC7F30;
    case 0x1E:
        return 0x00BC8170;
    case 0x1F:
        return 0x00BC8320;
    case 0x20:
        return 0x00BC7750;
    case 0x21:
        return 0x00BC82D8;
    case 0x22:
        return 0x00BC83F8;
    case 0x23:
        return 0x00BC8368;
    case 0x24:
    case 0x71:
        return 0x00BC7A20;
    case 0x28:
        return 0x00BC8488;
    case 0x29:
        return 0x00BC76C0;
    case 0x32:
        return 0x00BC84D0;
    case 0x33:
        return 0x00BC8560;
    case 0x34:
        return 0x00BC85A8;
    case 0x3C:
        return 0x00BC7870;
    case 0x3D:
        return 0x00BC8440;
    case 0x3E:
        return 0x00BC86C8;
    case 0x3F:
        return 0x00BC85F0;
    case 0x40:
        return 0x00BC8638;
    case 0x43:
        return 0x00BC8680;
    case 0x46:
    case 0x50:
        return 0x00BC8878;
    case 0x47:
        return 0x00BC88C0;
    case 0x49:
        return 0x00BC8908;
    case 0x4B:
        return 0x00BC8950;
    case 0x4D:
        return 0x00BC89E0;
    case 0x4F:
        return 0x00BC8A28;
    case 0x51:
        return 0x00BC8518;
    case 0x52:
    case 0x53:
        return 0x00BC87E8;
    case 0x6E:
        return 0x00BC8830;
    case 0x6F:
        return 0x00BC7B40;
    case 0x70:
        return 0x00BC7AB0;
    case 0x72:
        return 0x00BC7AF8;
    case 0x73:
        return 0x00BC7A68;
    case 0x74:
        return 0x00BC79D8;
    case 0x75:
        return 0x00BC87A0;
    default:
        return 0x00BC8758;   // (unknown)
    }
}

} // namespace

// 0x895340 (cdecl): NiDX9PixelFormat::GetD3DFormat (the pixel format's renderer format when it has
// one; else by its layout, bits per pixel and red (alpha, component 3, depth / stencil bits)
// masks; 0 when there's none)
uint32_t __cdecl NiDX9PixelFormat_GetD3DFormat(const uint8_t* format) {
    uint32_t own = At<uint32_t>(format, 0xC);
    uint8_t bits = At<uint8_t>(format, 1);
    if (own != 0xFFFFFFFF) {
        return own;
    }
    uint32_t result = 0;
    switch (At<uint32_t>(format, 4)) {
    case 0:   // RGB
        switch (bits) {
        case 8:
            if (PixelFormat_GetMask(format, kRed) == 0xE0) {
                result = 0x1B;   // R3G3B2
            } else if (PixelFormat_GetMask(format, kRed) == 0) {
                return 0x32;   // L8
            }
            break;
        case 0x10:
            if (PixelFormat_GetMask(format, kRed) == 0xF800) {
                return 0x17;
            }
            if (PixelFormat_GetMask(format, kRed) == 0x7C00) {
                return 0x18;
            }
            if (PixelFormat_GetMask(format, kRed) == 0xF00) {
                return 0x1E;
            }
            if (PixelFormat_GetMask(format, kRed) == 0) {
                return 0x51;
            }
            break;
        case 0x18:
            return 0x14;
        case 0x20:
            if (PixelFormat_GetMask(format, kRed) == 0xFF0000) {
                return 0x16;
            }
            if (PixelFormat_GetMask(format, kRed) == 0xFF) {
                return 0x21;
            }
            if (PixelFormat_GetMask(format, kRed) == 0xFFFF0000) {
                return 0x22;
            }
            break;
        default:
            return 0;
        }
        break;
    case 1:   // RGBA
        switch (bits) {
        case 8:
            if (PixelFormat_GetMask(format, kAlpha) == 0) {
                return 0x1C;
            }
            if (PixelFormat_GetMask(format, kAlpha) == 0xF) {
                return 0x34;
            }
            break;
        case 0x10:
            if (PixelFormat_GetMask(format, kRed) == 0x7C00) {
                return 0x19;
            }
            if (PixelFormat_GetMask(format, kRed) == 0xF00) {
                return 0x1A;
            }
            if (PixelFormat_GetMask(format, kRed) == 0xE0) {
                return 0x1D;
            }
            if (PixelFormat_GetMask(format, kRed) == 0) {
                return 0x33;
            }
            break;
        case 0x20:
            if (PixelFormat_GetMask(format, kRed) == 0xFF0000) {
                return 0x15;
            }
            if (PixelFormat_GetMask(format, kRed) == 0x3FF) {
                return 0x1F;
            }
            if (PixelFormat_GetMask(format, kRed) == 0xFF) {
                return 0x20;
            }
            if (PixelFormat_GetMask(format, kRed) == 0x3FF00000) {
                return 0x23;
            }
            break;
        case 0x40:
            return 0x71;
        case 0x80:
            return 0x74;
        default:
            return 0;
        }
        break;
    case 2:
        return 0x29;   // P8
    case 3:
    case 10:
    case 13:
        return 0;
    case 4:
        return 0x31545844;
    case 5:
        return 0x33545844;
    case 6:
        return 0x35545844;
    case 8:
        return 0x3C;   // V8U8
    case 9:
        if (bits == 0x10) {
            return 0x3D;
        }
        if (bits == 0x20) {
            return 0x3E;
        }
        break;
    case 11:
        if (bits == 8) {
            return PixelFormat_GetMask(format, kComponent3) != 0 ? 0x1C : 0x32;
        }
        if (bits == 0x10) {
            return 0x6F;
        }
        if (bits == 0x20) {
            return 0x72;
        }
        break;
    case 12:
        if (bits == 0x10) {
            return 0x33;
        }
        if (bits == 0x20) {
            return 0x70;
        }
        if (bits == 0x40) {
            return 0x73;
        }
        break;
    case 14:
        if (bits == 0x20) {
            return 0x15;
        }
        if (bits == 0x40) {
            return 0x71;
        }
        if (bits == 0x80) {
            return 0x74;
        }
        break;
    case 15:   // depth / stencil
        if (bits == 0x10) {
            return PixelFormat_GetBits(format, kStencil) != 1 ? 0x50 : 0x49;
        }
        if (bits == 0x20) {
            uint8_t stencil = PixelFormat_GetBits(format, kStencil);
            if (stencil == 4) {
                return 0x4F;
            }
            if (stencil == 8) {
                return 0x4B;
            }
            return PixelFormat_GetBits(format, kDepth) != 0x18 ? 0x47 : 0x4D;
        }
        break;
    default:
        break;
    }
    return result;
}

// 0x895800 (cdecl): NiDX9PixelFormat::FromD3DFormat (the static pixel format copied, the D3D format
// its own)
void __cdecl NiDX9PixelFormat_FromD3DFormat(uint32_t format, uint32_t* out) {
    const uint32_t* source = reinterpret_cast<const uint32_t*>(StaticFormatFor(format));
    for (int i = 0; i < 0x11; i++) {
        out[i] = source[i];
    }
    out[3] = format;
    out[4] = 0;
}

// 0x895B20 (cdecl): NiDX9PixelFormat::CreateFromD3DFormat (a new NiPixelFormat of it)
void* __cdecl NiDX9PixelFormat_CreateFromD3DFormat(uint32_t format) {
    void* memory = NiNew(0x44);
    if (memory != nullptr) {
        void* pixelFormat = PixelFormat_Construct(memory);
        PixelFormat_FromD3DFormat(format, pixelFormat);
        return pixelFormat;
    }
    PixelFormat_FromD3DFormat(format, nullptr);   // (as the original: it writes through null)
    return nullptr;
}

#define C40G_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C40G_REVERSED("NiDX9PixelFormat::GetD3DFormat", 0x00895340, NiDX9PixelFormat_GetD3DFormat);
C40G_REVERSED("NiDX9PixelFormat::FromD3DFormat", 0x00895800, NiDX9PixelFormat_FromD3DFormat);
C40G_REVERSED("NiDX9PixelFormat::CreateFromD3DFormat", 0x00895B20, NiDX9PixelFormat_CreateFromD3DFormat);

} // namespace ni::c40
