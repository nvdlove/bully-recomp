// NiDX9TextureData (vtable 0x99C758, 0x64 bytes; NiTexture::RendererData's +0x8 the texture,
// +0xC width, +0x10 height, +0x14 its NiPixelFormat; then +0x58 the renderer, +0x5C the D3D
// texture, +0x60 the levels, +0x62): the base of the renderer data of source, rendered, cube and
// dynamic textures. Here its constructor, destructors, InitializeFromD3DTexture (slot 18), the
// texture formats' choice for a texture's preferences (with and without a conversion from the
// source's own format), ClearTextureData (every texture's renderer data dropped) and
// NiDX9RenderState::ClearTexture (a texture no longer bound to any stage).
//
// The formats array the choices search: NiDX9Renderer's +0x4FC (22 NiPixelFormat* by
// NiDX9Renderer's texture format: 0 RGB565-ish... 6/7 the "best" 32-bit ones, 8..10 compressed,
// 11..13 bump maps, 14/15 single-channel, 16..21 floating point) for the texture's usage.

#include "gamebryo/NiDX9RendererData.h"
#include "reversed.h"

namespace ni::c40 {
namespace {

constexpr uintptr_t kTextureDataVtable = 0x0099C758;
constexpr uintptr_t kRendererDataVtable = 0x00961358;   // NiTexture::RendererData

GAME_VAR(uint8_t*, ms_pkTextureListHead, 0x020E2178);   // NiTexture's list of all textures

GAME_FN(void, __thiscall, RefObject_Construct, 0x00755260, void* self);
GAME_FN(void*, __cdecl, ImageConverter_Get, 0x007D3B40);   // NiImageConverter::GetImageConverter
GAME_FN(void, __thiscall, RendererData_Destruct, 0x007769D0, void* self);   // NiTexture::RendererData's
GAME_FN(void, __thiscall, PixelFormat_Construct, 0x0076B280, void* self);
GAME_FN(uint8_t, __thiscall, PixelFormat_GetComponent, 0x0076A020, const void* self, uint32_t index,
        uint32_t* component, uint32_t* representation, uint8_t* bitsPerComponent, uint8_t* isSigned);
GAME_FN(uint32_t, __thiscall, PixelFormat_GetNumComponents, 0x0076A070, const void* self);
GAME_FN(void, __cdecl, PixelFormat_FromD3DFormat, 0x00895800, uint32_t format, void* pixelFormat);
// this file's (called where modes apply)
GAME_FN(void, __thiscall, RenderState_ClearTexture, 0x00892910, void* self, void* texture);

// IDirect3DBaseTexture9's slots
enum : int {
    kGetLevelCount = 13,
    kGetType = 10,
    kGetLevelDesc = 17,   // (IDirect3DTexture9 / IDirect3DCubeTexture9 / IDirect3DVolumeTexture9)
};

inline void ReleaseD3DTexture(uint8_t* self) {
    void* texture = At<void*>(self, 0x5C);
    if (texture != nullptr) {
        RenderState_ClearTexture(At<void*>(At<uint8_t*>(self, 0x58), 0x6B4), texture);
        Com<long>(At<void*>(self, 0x5C), 2);   // Release
        At<void*>(self, 0x5C) = nullptr;
    }
}

} // namespace

// 0x8925F0 (cdecl): NiDX9TextureData::FindClosestPixelFormat (the first format the device has for
// the preferences' pixel layout and alpha format, in Gamebryo's order of preference; the
// "default" layout's best formats first)
void* __cdecl NiDX9TextureData_FindClosestPixelFormat(const uint32_t* prefs, const re::Ptr32<void>* f) {
    uint32_t layout = prefs[0];
    int alpha = int(prefs[1]);
    if (layout == 6) {
        layout = 2;
    }
    if (alpha == 3) {
        alpha = 0;
    }
    void* r;
    if ((layout == 0 || layout == 5) && alpha == 0) {
        if (f[6] != nullptr) {
            return f[6];
        }
    } else if ((layout == 0 || layout == 5) && (alpha == 1 || alpha == 2)) {
        if (f[7] != nullptr) {
            return f[7];
        }
    }
    switch (layout) {
    case 0:
    case 3:
    case 5:
        if (alpha >= 0) {
            if (alpha <= 1) {
                if (f[8] != nullptr) {
                    return f[8];
                }
            } else if (alpha == 2 && f[9] != nullptr) {
                return f[9];
            }
        }
        [[fallthrough]];
    case 1:
        if (alpha == 0) {
            goto rgb;
        }
        if (alpha == 1) {
            goto oneBit;
        }
        if (alpha == 2) {
            goto smooth;
        }
        return nullptr;
    case 4:
        if (f[0xD] != nullptr) {
            return f[0xD];
        }
        if (alpha == 0) {
            if (f[0xB] != nullptr) {
                return f[0xB];
            }
            if (f[0xC] != nullptr) {
                return f[0xC];
            }
        } else {
            if (f[0xC] != nullptr) {
                return f[0xC];
            }
            if (f[0xB] != nullptr) {
                return f[0xB];
            }
        }
        [[fallthrough]];
    case 2:
        if (alpha == 0) {
            if (f[2] != nullptr) {
                return f[2];
            }
            if (f[5] != nullptr) {
                return f[5];
            }
            if (f[1] != nullptr) {
                return f[1];
            }
            r = f[0];
            goto tail5;
        }
        if (alpha == 1) {
            r = f[5];
            goto tail5;
        }
        if (alpha == 2) {
            r = f[5];
            goto tail3;
        }
        return nullptr;
    case 7:
        if (alpha != 0) {
            if (f[0xF] != nullptr) {
                return f[0xF];
            }
            goto oneBit;
        }
        if (f[0xE] != nullptr) {
            return f[0xE];
        }
        goto rgb;
    case 8:
        if (f[0x10] != nullptr) {
            return f[0x10];
        }
        if (f[0x13] != nullptr) {
            return f[0x13];
        }
        [[fallthrough]];
    case 10:
        if (f[0x11] != nullptr) {
            return f[0x11];
        }
        if (f[0x14] != nullptr) {
            return f[0x14];
        }
        [[fallthrough]];
    case 12:
    case 13:
        if (f[0x12] != nullptr) {
            return f[0x12];
        }
        return f[0x15];
    case 9:
        if (f[0x13] != nullptr) {
            return f[0x13];
        }
        if (f[0x14] != nullptr) {
            return f[0x14];
        }
        if (f[0x15] != nullptr) {
            return f[0x15];
        }
        r = f[0x10];
        goto tail11;
    case 11:
        if (f[0x14] != nullptr) {
            return f[0x14];
        }
        r = f[0x15];
        goto tail11;
    case 14:
        if (f[0x15] != nullptr) {
            return f[0x15];
        }
        return f[0x12];
    default:
        return nullptr;
    }
rgb:
    if (f[1] != nullptr) {
        return f[1];
    }
    if (f[0] != nullptr) {
        return f[0];
    }
    if (f[2] != nullptr) {
        return f[2];
    }
    r = f[5];
tail5:
    if (r != nullptr) {
        return r;
    }
    r = f[3];
tail3:
    if (r != nullptr) {
        return r;
    }
    return f[4];
oneBit:
    if (f[3] != nullptr) {
        return f[3];
    }
smooth:
    if (f[4] != nullptr) {
        return f[4];
    }
    return f[5];
tail11:
    if (r != nullptr) {
        return r;
    }
    if (f[0x11] != nullptr) {
        return f[0x11];
    }
    return f[0x12];
}

// 0x892840 (thiscall): NiDX9TextureData::InitializeFromD3DTexture (the levels, the top level's
// size and pixel format; false when its description can't be had or it's palettized)
uint8_t __fastcall NiDX9TextureData_InitializeFromD3DTexture(uint8_t* self, re::Edx, void* texture) {
    At<uint16_t>(self, 0x60) = uint16_t(Com<uint32_t>(texture, kGetLevelCount));
    uint32_t type = Com<uint32_t>(texture, kGetType);
    uint32_t desc[8];   // D3DSURFACE_DESC, or D3DVOLUME_DESC for a volume
    if (type == 3 || type == 5) {   // a texture, a cube texture
        if (Com<long>(texture, kGetLevelDesc, 0u, desc) < 0) {
            return 0;
        }
        At<uint32_t>(self, 0xC) = desc[6];
        At<uint32_t>(self, 0x10) = desc[7];
    } else if (type == 4) {   // a volume texture
        if (Com<long>(texture, kGetLevelDesc, 0u, desc) < 0) {
            return 0;
        }
        At<uint32_t>(self, 0xC) = desc[4];
        At<uint32_t>(self, 0x10) = desc[5];
    } else {
        return 1;
    }
    if (desc[0] == 0x28 || desc[0] == 0x29) {   // D3DFMT_A8P8, D3DFMT_P8
        return 0;
    }
    PixelFormat_FromD3DFormat(desc[0], self + 0x14);
    return 1;
}

// 0x892910 (thiscall): NiDX9RenderState::ClearTexture (every stage it's bound to cleared)
void __fastcall NiDX9RenderState_ClearTexture(uint8_t* self, re::Edx, void* texture) {
    if (texture == nullptr) {
        return;
    }
    for (uint32_t i = 0; i < 16; i++) {
        if (At<void*>(self, 0x20A0 + int(i) * 4) == texture) {
            At<void*>(self, 0x20A0 + int(i) * 4) = nullptr;
            Com<long>(At<void*>(self, 0x2238), 65, i, static_cast<void*>(nullptr));   // SetTexture
        }
    }
}

// 0x892960 (thiscall): NiDX9TextureData's constructor
void* __fastcall NiDX9TextureData_Construct(uint8_t* self, re::Edx, void* texture, void* renderer) {
    RefObject_Construct(self);
    At<uint32_t>(self, 0) = kRendererDataVtable;
    At<void*>(self, 8) = texture;
    At<uint32_t>(self, 0xC) = 0;
    At<uint32_t>(self, 0x10) = 0;
    PixelFormat_Construct(self + 0x14);
    At<void*>(self, 0x5C) = nullptr;
    At<uint16_t>(self, 0x60) = 0;
    At<uint16_t>(self, 0x62) = 0;
    At<uint32_t>(self, 0) = kTextureDataVtable;
    At<void*>(self, 0x58) = renderer;
    return self;
}

// 0x8929B0 (thiscall): NiDX9TextureData's destructor (the D3D texture unbound and let go)
void __fastcall NiDX9TextureData_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kTextureDataVtable;
    ReleaseD3DTexture(self);
    At<uint32_t>(self, 0) = kRendererDataVtable;
    RendererData_Destruct(self);
}

// 0x8929F0 (thiscall, the object unused): NiDX9TextureData::FindMatchingPixelFormat (as the
// closest choice, but each candidate only when the image converter can convert the source's
// format to it; a source that isn't a plain RGB(A) format (+0x8 set or +0x0's bit 0 clear) none)
void* __fastcall NiDX9TextureData_FindMatchingPixelFormat(void*, re::Edx, const uint8_t* source, const uint32_t* prefs,
                                                          const re::Ptr32<void>* f) {
    uint32_t sourceFormat = At<uint32_t>(source, 4);
    void* converter = ImageConverter_Get();
    auto fits = [&](void* candidate) {
        return candidate != nullptr && Virtual<uint8_t>(converter, 4, source, candidate) != 0;
    };
    if (At<uint32_t>(source, 8) != 0 || (At<uint8_t>(source, 0) & 1) == 0) {
        return nullptr;
    }
    uint32_t layout = prefs[0];
    if (layout == 4) {   // a bump map
        if (sourceFormat == 9 || sourceFormat == 1) {
            if (fits(f[0xD])) {
                return f[0xD];
            }
            if (fits(f[0xC])) {
                return f[0xC];
            }
            return fits(f[0xB]) ? f[0xB] : nullptr;
        }
        if (fits(f[0xB])) {
            return f[0xB];
        }
        if (fits(f[0xD])) {
            return f[0xD];
        }
        return fits(f[0xC]) ? f[0xC] : nullptr;
    }
    switch (sourceFormat) {
    case 0:
    case 2:
        if (layout == 3) {
            if (fits(f[8])) {
                return f[8];
            }
            if (fits(f[9])) {
                return f[9];
            }
            if (fits(f[10])) {
                return f[10];
            }
        }
        if (prefs[0] == 1) {
            if (fits(f[1])) {
                return f[1];
            }
            if (fits(f[0])) {
                return f[0];
            }
            if (fits(f[2])) {
                return f[2];
            }
            if (fits(f[5])) {
                return f[5];
            }
        } else {
            if (fits(f[2])) {
                return f[2];
            }
            if (fits(f[5])) {
                return f[5];
            }
            if (fits(f[1])) {
                return f[1];
            }
            if (fits(f[0])) {
                return f[0];
            }
        }
        if (fits(f[3])) {
            return f[3];
        }
        return fits(f[4]) ? f[4] : nullptr;
    case 4:
    case 5:
    case 6:
        if (layout != 1 && layout != 2) {
            void* compressed = f[8 + (sourceFormat - 4)];
            if (fits(compressed)) {
                return compressed;
            }
        }
        break;
    case 7:
    case 10:
        return nullptr;
    case 8:
        if (fits(f[0xB])) {
            return f[0xB];
        }
        if (fits(f[0xD])) {
            return f[0xD];
        }
        return fits(f[0xC]) ? f[0xC] : nullptr;
    case 9:
        if (fits(f[0xC])) {
            return f[0xC];
        }
        if (fits(f[0xD])) {
            return f[0xD];
        }
        return fits(f[0xB]) ? f[0xB] : nullptr;
    case 11:
        if (prefs[1] == 2) {
            if (fits(f[0xF])) {
                return f[0xF];
            }
        } else if (fits(f[0xE])) {
            return f[0xE];
        }
        break;
    case 1:
    case 3:
    case 12:
    case 13:
    case 14:
        break;
    default:
        return nullptr;
    }
    // the floating point formats by the source's first component
    uint32_t component = 0x13;
    uint32_t representation = 5;
    uint8_t bits = 0;
    uint8_t isSigned;
    if (PixelFormat_GetComponent(source, 0, &component, &representation, &bits, &isSigned) != 0 &&
        (representation == 2 || representation == 1)) {
        uint32_t count = PixelFormat_GetNumComponents(source);
        if (bits == 0x10) {
            if (count < 2) {
                if (fits(f[0x10])) {
                    return f[0x10];
                }
                if (fits(f[0x13])) {
                    return f[0x13];
                }
            }
            if (count < 3) {
                if (fits(f[0x11])) {
                    return f[0x11];
                }
                if (fits(f[0x14])) {
                    return f[0x14];
                }
            }
            if (fits(f[0x12])) {
                return f[0x12];
            }
            if (fits(f[0x15])) {
                return f[0x15];
            }
        } else if (bits == 0x20) {
            if (count < 2 && fits(f[0x13])) {
                return f[0x13];
            }
            if (count < 3 && fits(f[0x14])) {
                return f[0x14];
            }
            if (fits(f[0x15])) {
                return f[0x15];
            }
            if (count < 2 && fits(f[0x10])) {
                return f[0x10];
            }
            if (count < 3 && fits(f[0x11])) {
                return f[0x11];
            }
            if (fits(f[0x12])) {
                return f[0x12];
            }
        }
    }
    if (prefs[0] == 3) {   // compressed
        if (prefs[1] == 1) {
            if (fits(f[8])) {
                return f[8];
            }
            if (fits(f[9])) {
                return f[9];
            }
            if (fits(f[10])) {
                return f[10];
            }
        } else {
            if (fits(f[9])) {
                return f[9];
            }
            if (fits(f[10])) {
                return f[10];
            }
            if (fits(f[8])) {
                return f[8];
            }
        }
    }
    void* last;
    if (prefs[0] == 1) {
        if (prefs[1] == 1) {
            if (fits(f[3])) {
                return f[3];
            }
            if (fits(f[4])) {
                return f[4];
            }
            last = f[5];
        } else {
            if (fits(f[4])) {
                return f[4];
            }
            if (fits(f[5])) {
                return f[5];
            }
            last = f[3];
        }
    } else {
        if (fits(f[5])) {
            return f[5];
        }
        if (prefs[1] == 1) {
            if (fits(f[3])) {
                return f[3];
            }
            last = f[4];
        } else {
            if (fits(f[4])) {
                return f[4];
            }
            last = f[3];
        }
    }
    return fits(last) ? last : nullptr;
}

// 0x8930B0 (cdecl): NiDX9TextureData::ClearTextureData (every texture's renderer data deleted;
// the renderer isn't read)
void __cdecl NiDX9TextureData_ClearTextureData(void*) {
    for (uint8_t* texture = ms_pkTextureListHead; texture != nullptr; texture = At<uint8_t*>(texture, 0x2C)) {
        void* data = At<void*>(texture, 0x24);
        if (data != nullptr) {
            At<void*>(texture, 0x24) = nullptr;
            Virtual<void*>(data, 0, 1);   // the deleting destructor
        }
    }
}

// 0x8930E0 (thiscall): NiDX9TextureData's deleting destructor
void* __fastcall NiDX9TextureData_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = kTextureDataVtable;
    ReleaseD3DTexture(self);
    At<uint32_t>(self, 0) = kRendererDataVtable;
    RendererData_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x64);
    }
    return self;
}

#define C40D_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C40D_REVERSED("NiDX9TextureData::FindClosestPixelFormat", 0x008925F0, NiDX9TextureData_FindClosestPixelFormat);
C40D_REVERSED("NiDX9TextureData::InitializeFromD3DTexture", 0x00892840, NiDX9TextureData_InitializeFromD3DTexture);
C40D_REVERSED("NiDX9RenderState::ClearTexture", 0x00892910, NiDX9RenderState_ClearTexture);
C40D_REVERSED("NiDX9TextureData::NiDX9TextureData", 0x00892960, NiDX9TextureData_Construct);
C40D_REVERSED("NiDX9TextureData::~NiDX9TextureData", 0x008929B0, NiDX9TextureData_Destruct);
C40D_REVERSED("NiDX9TextureData::FindMatchingPixelFormat", 0x008929F0, NiDX9TextureData_FindMatchingPixelFormat);
C40D_REVERSED("NiDX9TextureData::ClearTextureData", 0x008930B0, NiDX9TextureData_ClearTextureData);
C40D_REVERSED("NiDX9TextureData::DeletingDestructor", 0x008930E0, NiDX9TextureData_DeletingDestructor);

} // namespace ni::c40
