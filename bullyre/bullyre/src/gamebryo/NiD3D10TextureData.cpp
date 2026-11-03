// The D3D10 renderer's texture data. NiD3D10TextureData (vtable 0x9A0858, 0x64 bytes; NiTexture::
// RendererData's own GetRTTI: NiObject's): NiTexture::RendererData's +0x8 the texture, +0xC width,
// +0x10 height, +0x14 its NiPixelFormat; then +0x58 the mipmap levels, +0x5A its type flags (0x1
// set by both kinds here, 0x2 tested by the renderer (a guess: rendered), 0x8 a cube map, 0x2000 /
// 0x4000 / 0x8000 a 1D / 2D / 3D resource), +0x5C its ID3D10ShaderResourceView, +0x60 its
// ID3D10Resource (both held). Its static choices of a pixel format: for a source's data (with a
// conversion by NiImageConverter) and for no source (rendered textures), among the D3D10 pixel
// formats the renderer keeps by DXGI format (0x20E75E0, 0x48 each from DXGI_FORMAT 1) and that it
// supports for the use asked (NiD3D10Renderer 0x8A90E0: its format support bits & the flags).
//
// NiD3D10SourceTextureData (vtable 0x9A0B38, 0x7C bytes): +0x64 static (replacement data), +0x65
// mipmapped, +0x68 the pixel data's revision, +0x6C its size in bytes, +0x70 the levels skipped
// (the renderer's skip level, 0x20E916C), +0x72 the faces (array size), +0x74 the palette (a
// reference), +0x78 its revision. Its texture is made immutable (or default) from the pixel data
// converted to the chosen format, its levels from the skip level on; Update (slot 18) copies the
// pixel data again when it changed.
//
// NiD3D10RenderedTextureData (vtable 0x9A0B88, 0x68 bytes): +0x64 the array size (6 for a cube
// map): a render target texture with a shader resource view and the render target buffer data of
// its buffer (or of each cube face's).

#include "gamebryo/NiD3D10C43.h"
#include "gamebryo/NiPixelData.h"
#include "reversed.h"

namespace ni::c43 {
namespace {

constexpr uintptr_t kTextureDataVtable = 0x009A0858;
constexpr uintptr_t kRendererDataVtable = 0x00961358;   // NiTexture::RendererData
constexpr uintptr_t kSourceTextureDataVtable = 0x009A0B38;
constexpr uintptr_t kRenderedTextureDataVtable = 0x009A0B88;
constexpr uintptr_t kPixelFormats = 0x020E75E0;   // the D3D10 pixel formats, by DXGI_FORMAT - 1

GAME_VAR(uint16_t, ms_usSkipLevels, 0x020E916C);   // NiD3D10Renderer's mipmap skip level
GAME_VAR(uint32_t, NiSourceCubeMap_ClassBit, 0x020E3CA8);
GAME_VAR(uint32_t, NiRenderedCubeMap_ClassBit, 0x020E3874);
GAME_VAR(uint8_t, ms_bMipmapByDefault, 0x020E2180);
GAME_VAR(uint8_t, ms_bDestroyAppDataByDefault, 0x00BC7622);

GAME_FN(void, __thiscall, RefObject_Construct, 0x00755260, void* self);
GAME_FN(void, __thiscall, RendererData_Destruct, 0x007769D0, void* self);   // NiTexture::RendererData's
GAME_FN(void, __thiscall, PixelFormat_Construct, 0x0076B280, void* self);
GAME_FN(uint32_t, __thiscall, PixelFormat_GetNumComponents, 0x0076A070, const void* self);
GAME_FN(void*, __cdecl, ImageConverter_Get, 0x007D3B40);   // NiImageConverter::GetImageConverter
GAME_FN(NiPixelData*, __cdecl, Texture_GetReplacementData, 0x00755170, void* texture, uint32_t error, int warn);
GAME_FN(void, __thiscall, NiPointer_Assign, 0x00800420, void* slot, void* object);
// the renderer's (chunk 42's)
GAME_FN(uint8_t*, __cdecl, D3D10Renderer_GetRenderer, 0x008A9300);
GAME_FN(uint32_t, __cdecl, D3D10_GetDXGIFormat, 0x008A8D80, const void* pixelFormat);
GAME_FN(uint32_t, __thiscall, D3D10Renderer_IsFormatSupported, 0x008A90E0, const void* renderer, uint32_t format,
        uint32_t flags);
GAME_FN(void*, __thiscall, D3D10Renderer_GetResourceManager, 0x008A9120, const void* renderer);
GAME_FN(void*, __thiscall, D3D10Renderer_GetD3DDevice, 0x008A9140, const void* renderer);
GAME_FN(void*, __thiscall, ResourceManager_CreateTexture2D, 0x008AFF00, void* self, uint32_t width, uint32_t height,
        uint32_t mipLevels, uint32_t arraySize, uint32_t format, uint32_t sampleCount, uint32_t sampleQuality,
        uint32_t usage, uint32_t bindFlags, uint32_t cpuAccessFlags, uint32_t miscFlags, const void* initialData);
// the render target buffer data's (this chunk's NiD3D10BufferData.cpp)
GAME_FN(void*, __cdecl, RenderTargetBufferData_Create, 0x008B62D0, void* texture, re::Ptr32<void>* buffer, const void* desc);
// this file's (called where modes apply)
GAME_FN(uint8_t, __thiscall, TextureData_IsCubeMap, 0x008B4590, const void* self);
GAME_FN(uint8_t, __thiscall, TextureData_InitializeFromD3D10Resource, 0x008B45A0, void* self, void* resource,
        void* view);
GAME_FN(void*, __thiscall, TextureData_Construct, 0x008B4620, void* self, void* texture);
GAME_FN(void, __thiscall, TextureData_Destruct, 0x008B4670, void* self);
GAME_FN(const uint8_t*, __thiscall, TextureData_FindMatchingPixelFormat, 0x008B46B0, const void* self,
        const NiPixelFormat* source, const uint32_t* prefs, uint32_t flags);
GAME_FN(const uint8_t*, __thiscall, TextureData_FindClosestPixelFormat, 0x008B5030, const void* self,
        const uint32_t* prefs, uint32_t flags);
GAME_FN(uint8_t, __thiscall, SourceTextureData_CreateTexture, 0x008B6730, void* self, const NiPixelData* pixels,
        int cube, int immutable);
GAME_FN(uint8_t, __thiscall, SourceTextureData_CopyDataToTexture, 0x008B6A20, void* self, const NiPixelData* pixels);
GAME_FN(uint8_t, __thiscall, SourceTextureData_UpdatePalette, 0x008B6B60, void* self, void* palette);
GAME_FN(void*, __thiscall, SourceTextureData_Construct, 0x008B6B90, void* self, void* texture);
GAME_FN(uint8_t, __thiscall, SourceTextureData_Initialize, 0x008B6C10, void* self);
GAME_FN(void*, __thiscall, RenderedTextureData_Construct, 0x008B6F50, void* self, void* texture);
GAME_FN(uint8_t, __thiscall, RenderedTextureData_CreateSurf, 0x008B6FC0, void* self, uint32_t multisample);

// D3D10_SUBRESOURCE_DATA, D3D10_SHADER_RESOURCE_VIEW_DESC, D3D10_RENDER_TARGET_VIEW_DESC
struct SubresourceData {
    const void* sysMem;
    uint32_t pitch;
    uint32_t slicePitch;
};
struct ViewDesc {
    uint32_t format;      // +0x0
    uint32_t dimension;   // +0x4
    uint32_t u[4];        // +0x8 (the dimension's union)
};
static_assert(sizeof(ViewDesc) == 0x18);

inline const uint8_t* PixelFormatOf(uint32_t dxgi) {
    return reinterpret_cast<const uint8_t*>(kPixelFormats + (dxgi - 1) * 0x48);
}

// the pixel formats' flags (bits 0x2 and 0x8 of the type flags)
inline uint16_t& TypeFlags(void* self) {
    return At<uint16_t>(self, 0x5A);
}

// a texture is of a class (NiIsKindOf by its class bit)
inline bool IsKindOf(void* object, uint32_t bit) {
    const uint8_t* rtti = Virtual<const uint8_t*>(object, 2);
    return (At<uint32_t>(rtti, 0xC + int(bit >> 5) * 4) & (1u << (bit & 0x1F))) != 0;
}

// the block-compressed formats with 8-byte blocks (BC1, BC4 and BC5's DXGI numbers, as the game
// lists them)
inline bool IsBlock8(uint32_t format) {
    return format == 0x46 || format == 0x47 || format == 0x48 || format == 0x4F || format == 0x50 ||
           format == 0x51 || format == 0x52 || format == 0x53 || format == 0x54;
}

// a pixel data's level's row pitch (block-compressed: the width rounded up to 4 (masked with
// 0x0FFFFFFC), halved for 8-byte blocks; as the game computes it)
inline uint32_t RowPitch(const NiPixelData* pixels, uint32_t level, bool compressed, uint32_t blockSize) {
    uint32_t width = pixels->widths[level];
    uint32_t pitch = pixels->pixelStride * width;
    if (compressed) {
        pitch = (width + 3) & 0x0FFFFFFCu;
        if (blockSize == 8) {
            pitch >>= 1;
        }
    }
    return pitch;
}

// a temporary NiPointer of an object, gone (held then let go: deleted when it was the last)
inline void TouchRef(NiObject* object) {
    Increment(&object->refCount);
    if (Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

} // namespace

// 0x8B4580 (thiscall): NiD3D10TextureData's type flag 0x2 (a guess: rendered)
uint8_t __fastcall TextureData_IsRenderedTexture(void* self, re::Edx) {
    return uint8_t((TypeFlags(self) >> 1) & 1);
}

// 0x8B4590 (thiscall): NiD3D10TextureData::IsCubeMap (type flag 0x8)
uint8_t __fastcall TextureData_IsCubeMapImpl(void* self, re::Edx) {
    return uint8_t((TypeFlags(self) >> 3) & 1);
}

// 0x8B45A0 (thiscall): NiD3D10TextureData::InitializeFromD3D10Resource(resource, view): both
// kept (not held: the caller's references are taken over) unless the resource is a buffer; the
// dimension's flag set
uint8_t __fastcall TextureData_InitializeFromD3D10ResourceImpl(void* self, re::Edx, void* resource, void* view) {
    if (resource == nullptr || view == nullptr) {
        return 0;
    }
    uint32_t dimension = 0;
    Com<void>(resource, kResourceGetType, &dimension);
    if (dimension == 1) {
        return 0;
    }
    At<void*>(self, 0x5C) = view;
    At<void*>(self, 0x60) = resource;
    if (dimension == 2) {
        TypeFlags(self) |= 0x2000;
    } else if (dimension == 3) {
        TypeFlags(self) |= 0x4000;
    } else if (dimension == 4) {
        TypeFlags(self) |= 0x8000;
    }
    return 1;
}

// 0x8B4620 (thiscall): NiD3D10TextureData::NiD3D10TextureData(texture)
void* __fastcall TextureData_ConstructImpl(void* self, re::Edx, void* texture) {
    RefObject_Construct(self);
    At<uint32_t>(self, 0) = kRendererDataVtable;
    At<void*>(self, 8) = texture;
    At<uint32_t>(self, 0xC) = 0;
    At<uint32_t>(self, 0x10) = 0;
    PixelFormat_Construct(static_cast<uint8_t*>(self) + 0x14);
    At<uint16_t>(self, 0x58) = 0;
    At<uint16_t>(self, 0x5A) = 0;
    At<void*>(self, 0x5C) = nullptr;
    At<void*>(self, 0x60) = nullptr;
    At<uint32_t>(self, 0) = kTextureDataVtable;
    return self;
}

// 0x8B4670 (thiscall): NiD3D10TextureData::~NiD3D10TextureData (the view and resource let go)
void __fastcall TextureData_DestructImpl(void* self, re::Edx) {
    At<uint32_t>(self, 0) = kTextureDataVtable;
    ComRelease(At<void*>(self, 0x5C));
    ComRelease(At<void*>(self, 0x60));
    At<uint32_t>(self, 0) = kRendererDataVtable;
    RendererData_Destruct(self);
}

// 0x8B46B0 (thiscall, `this` unused): NiD3D10TextureData::FindMatchingPixelFormat(source format,
// the texture's format preferences, the support flags): the first D3D10 format the renderer
// supports for the flags and the image converter can convert the source to, in the game's order:
// for compressed sources the matching BC format (unless the preferences ask for 16 or 32-bit
// colour); for half / float sources the float formats by the source's bits and components; then
// by the preferred layout (bump maps: the signed formats; compressed: BC1..3 by the alpha
// preference, sRGB or not; else R8G8B8A8 (sRGB first when the source is), R10G10B10A2,
// R16G16B16A16). Only for little endian, untiled sources; 0 without a renderer.
const uint8_t* __fastcall TextureData_FindMatchingPixelFormatImpl(const void*, re::Edx, const NiPixelFormat* source,
                                                                  const uint32_t* prefs, uint32_t flags) {
    int32_t sourceFormat = source->format;
    void* converter = ImageConverter_Get();
    if (source->tiling != 0 || (source->flags & 1) == 0) {
        return nullptr;
    }
    uint8_t* renderer = D3D10Renderer_GetRenderer();
    if (renderer == nullptr) {
        return nullptr;
    }
    // a format tried: supported for the flags, and the source convertible to it
    auto take = [&](uint32_t dxgi) -> const uint8_t* {
        const uint8_t* format = PixelFormatOf(dxgi);
        if (D3D10Renderer_IsFormatSupported(renderer, At<uint32_t>(format, 0xC), flags) != 0 &&
            Virtual<uint8_t>(converter, 4, source, format) != 0) {
            return format;
        }
        return nullptr;
    };
    const uint8_t* found = nullptr;
#define GB43_TRY(dxgi)              \
    if ((found = take(dxgi)) != nullptr) { \
        return found;               \
    }
    uint32_t representation = source->components[0].representation;
    uint8_t bits = source->components[0].bitsPerComponent;
    if (source->components[0].component != 0x13) {
        if (representation == 4) {   // compressed
            if (prefs[0] != 1 && prefs[0] != 2) {
                if (sourceFormat == 4) {
                    GB43_TRY(71)   // BC1_UNORM
                } else if (sourceFormat == 5) {
                    GB43_TRY(74)   // BC2_UNORM
                } else if (sourceFormat == 6) {
                    GB43_TRY(77)   // BC3_UNORM
                }
            }
        } else if (representation == 2 || representation == 1) {   // float, half
            uint32_t components = PixelFormat_GetNumComponents(source);
            if (bits == 0x10) {
                if (components < 2) {
                    GB43_TRY(54)   // R16_FLOAT
                    GB43_TRY(41)   // R32_FLOAT
                }
                if (components < 3) {
                    GB43_TRY(34)   // R16G16_FLOAT
                    GB43_TRY(16)   // R32G32_FLOAT
                }
                if (components < 4) {
                    GB43_TRY(6)    // R32G32B32_FLOAT
                }
                GB43_TRY(10)   // R16G16B16A16_FLOAT
                GB43_TRY(2)    // R32G32B32A32_FLOAT
            } else if (bits == 0x20) {
                if (components < 2) {
                    GB43_TRY(41)
                }
                if (components < 3) {
                    GB43_TRY(16)
                }
                if (components < 4) {
                    GB43_TRY(6)
                }
                GB43_TRY(2)
                if (components < 2) {
                    GB43_TRY(54)
                }
                if (components < 3) {
                    GB43_TRY(34)
                }
                GB43_TRY(10)
            }
        }
    }
    if (prefs[0] == 4) {   // bump map
        if (sourceFormat == 9 || sourceFormat == 1) {
            GB43_TRY(31)   // R8G8B8A8_SNORM
            GB43_TRY(13)   // R16G16B16A16_SNORM
            GB43_TRY(37)   // R16G16_SNORM
            GB43_TRY(51)   // R8G8_SNORM
            GB43_TRY(84)   // BC5_SNORM
        } else {
            GB43_TRY(37)
            GB43_TRY(51)
            GB43_TRY(84)
            GB43_TRY(31)
            GB43_TRY(13)
        }
        return nullptr;
    }
    if (prefs[0] == 3) {   // compressed
        if (prefs[1] == 1) {   // binary alpha
            if (source->srgb == 0) {
                GB43_TRY(71)
                GB43_TRY(74)
                GB43_TRY(77)
            } else {
                GB43_TRY(72)   // BC1_UNORM_SRGB
                GB43_TRY(75)   // BC2_UNORM_SRGB
                GB43_TRY(78)   // BC3_UNORM_SRGB
            }
        } else if (source->srgb == 0) {
            GB43_TRY(74)
            GB43_TRY(77)
            GB43_TRY(71)
        } else {
            GB43_TRY(75)
            GB43_TRY(78)
            GB43_TRY(72)
        }
    }
    if (source->srgb != 0) {
        GB43_TRY(29)   // R8G8B8A8_UNORM_SRGB
    }
    GB43_TRY(28)   // R8G8B8A8_UNORM
    GB43_TRY(24)   // R10G10B10A2_UNORM
    GB43_TRY(11)   // R16G16B16A16_UNORM
#undef GB43_TRY
    return nullptr;
}

// 0x8B5030 (thiscall, `this` unused): NiD3D10TextureData::FindClosestPixelFormat(the format
// preferences, the support flags): for no source (rendered textures): the first format the
// renderer supports by the preferred layout (bump maps the signed ones; the single / double
// colour and float layouts their own lists), then R10G10B10A2 for 16-bit colour, else
// R8G8B8A8_UNORM or none
const uint8_t* __fastcall TextureData_FindClosestPixelFormatImpl(const void*, re::Edx, const uint32_t* prefs,
                                                                 uint32_t flags) {
    uint8_t* renderer = D3D10Renderer_GetRenderer();
    if (renderer == nullptr) {
        return nullptr;
    }
    auto supported = [&](uint32_t dxgi) {
        return D3D10Renderer_IsFormatSupported(renderer, At<uint32_t>(PixelFormatOf(dxgi), 0xC), flags) != 0;
    };
    // the layouts' lists (the game's switch, its shared tails written out)
    static const uint8_t kBump[] = {31, 13, 37, 51};
    static const uint8_t kSingle8[] = {61, 56, 49, 35};
    static const uint8_t kSingle16[] = {54, 41, 34, 16, 10, 2};
    static const uint8_t kSingle32[] = {41, 16, 2, 54, 34, 10};
    static const uint8_t kDouble32[] = {34, 16, 10, 2};
    static const uint8_t kDouble64[] = {16, 2, 34, 10};
    static const uint8_t kFloat[] = {10, 2};
    static const uint8_t kFloat128[] = {2, 10};
    const uint8_t* list = nullptr;
    uint32_t count = 0;
    switch (prefs[0]) {
    case 4:
        list = kBump;
        count = sizeof(kBump);
        break;
    case 7:
        list = kSingle8;
        count = sizeof(kSingle8);
        break;
    case 8:
        list = kSingle16;
        count = sizeof(kSingle16);
        break;
    case 9:
        list = kSingle32;
        count = sizeof(kSingle32);
        break;
    case 10:
        list = kDouble32;
        count = sizeof(kDouble32);
        break;
    case 11:
        list = kDouble64;
        count = sizeof(kDouble64);
        break;
    case 12:
    case 13:
        list = kFloat;
        count = sizeof(kFloat);
        break;
    case 14:
        list = kFloat128;
        count = sizeof(kFloat128);
        break;
    default:
        break;
    }
    for (uint32_t i = 0; i < count; i++) {
        if (supported(list[i])) {
            return PixelFormatOf(list[i]);
        }
    }
    if (prefs[0] == 1 && supported(24)) {
        return PixelFormatOf(24);
    }
    return supported(28) ? PixelFormatOf(28) : nullptr;
}

// 0x8B53F0 (thiscall): NiD3D10TextureData's deleting destructor
void* __fastcall TextureData_DeletingDestructor(void* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kTextureDataVtable;
    ComRelease(At<void*>(self, 0x5C));
    ComRelease(At<void*>(self, 0x60));
    At<uint32_t>(self, 0) = kRendererDataVtable;
    RendererData_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x64);
    }
    return self;
}

// 0x8B6730 (thiscall): NiD3D10SourceTextureData::CreateTexture(pixel data, cube, immutable): the
// levels from the skip level on (the size from the first kept), one subresource per face and level
// (the face's data at the pixel data's face size apart), a 2D texture (immutable or default,
// TEXTURECUBE when cube) and its shader resource view (cube, 2D or 2D array); the data initialized
// from them. (A view that fails is let go, the texture is kept, as in the game.)
uint8_t __fastcall SourceTextureData_CreateTextureImpl(uint8_t* self, re::Edx, const NiPixelData* pixels, int cube,
                                                       int immutable) {
    uint32_t format = D3D10_GetDXGIFormat(self + 0x14);
    uint16_t levels = uint16_t(pixels->mipmapLevels);
    At<uint16_t>(self, 0x58) = levels;
    At<uint16_t>(self, 0x70) = ms_usSkipLevels;
    if (int32_t(At<uint16_t>(self, 0x70)) > int32_t(levels) - 1) {
        At<uint16_t>(self, 0x70) = uint16_t(levels - 1);
    }
    uint16_t skip = At<uint16_t>(self, 0x70);
    At<uint16_t>(self, 0x58) = uint16_t(At<uint16_t>(self, 0x58) - skip);
    At<uint32_t>(self, 0xC) = pixels->widths[skip];
    At<uint32_t>(self, 0x10) = pixels->heights[skip];
    uint16_t faces = uint16_t(pixels->faces);
    At<uint16_t>(self, 0x72) = faces;
    uint32_t usage = re::Flag(immutable) ? 1u : 0u;      // IMMUTABLE / DEFAULT
    uint32_t misc = re::Flag(cube) ? 4u : 0u;             // TEXTURECUBE
    uint32_t blockSize = 0x10;
    bool compressed = false;
    int32_t pixelFormat = At<int32_t>(self, 0x18);
    if (pixelFormat >= 4 && pixelFormat <= 6) {
        compressed = true;
        if (IsBlock8(At<uint32_t>(self, 0x20))) {
            blockSize = 8;
        }
    }
    SubresourceData data[0x86];   // (the game's 0x650-byte stack array: uninitialised past the used)
    uint32_t index = 0;
    if (faces != 0) {
        for (uint32_t face = 0, left = At<uint16_t>(self, 0x72); left != 0; face++, left--) {
            if (At<uint16_t>(self, 0x58) == 0) {
                continue;
            }
            uint16_t level = At<uint16_t>(self, 0x70);
            for (uint32_t n = At<uint16_t>(self, 0x58); n != 0; n--, level++) {
                SubresourceData& d = data[index++];
                d.pitch = RowPitch(pixels, level, compressed, blockSize);
                d.sysMem = pixels->pixels + pixels->offsets[level] + pixels->offsets[pixels->mipmapLevels] * face;
                d.slicePitch = pixels->offsets[level + 1] - pixels->offsets[level];
            }
        }
    }
    uint8_t* renderer = D3D10Renderer_GetRenderer();
    void* texture = ResourceManager_CreateTexture2D(D3D10Renderer_GetResourceManager(renderer), At<uint32_t>(self, 0xC),
                                                    At<uint32_t>(self, 0x10), At<uint16_t>(self, 0x58),
                                                    At<uint16_t>(self, 0x72), format, 1, 0, usage, 8, 0, misc, data);
    if (texture == nullptr) {
        return 0;
    }
    ViewDesc desc;
    desc.format = format;
    if (re::Flag(cube)) {
        desc.dimension = 9;   // TEXTURECUBE
        desc.u[1] = At<uint16_t>(self, 0x58);
    } else if (At<uint16_t>(self, 0x72) == 1) {
        desc.dimension = 4;   // TEXTURE2D
        desc.u[1] = At<uint16_t>(self, 0x58);
    } else {
        desc.dimension = 5;   // TEXTURE2DARRAY
        desc.u[2] = 0;
        desc.u[3] = At<uint16_t>(self, 0x72);
        desc.u[1] = At<uint16_t>(self, 0x58);
    }
    desc.u[0] = 0;
    void* view = nullptr;
    long hr = Com<long>(D3D10Renderer_GetD3DDevice(renderer), kCreateShaderResourceView, texture, &desc, &view);
    if (hr < 0) {
        ComRelease(view);
        return 0;
    }
    if (view == nullptr) {
        return 0;
    }
    return TextureData_InitializeFromD3D10Resource(self, texture, view);
}

// 0x8B6A20 (thiscall): NiD3D10SourceTextureData::CopyDataToTexture(pixel data): the size and the
// skip level worked out again (halved, rounding up, per level skipped), then each level (counted
// from 0, as the game does) copied into the resource by UpdateSubresource
uint8_t __fastcall SourceTextureData_CopyDataToTextureImpl(uint8_t* self, re::Edx, const NiPixelData* pixels) {
    D3D10_GetDXGIFormat(self + 0x14);   // (its result unused)
    At<uint32_t>(self, 0xC) = pixels->widths[0];
    At<uint32_t>(self, 0x10) = pixels->heights[0];
    uint32_t levels = pixels->mipmapLevels;
    At<uint16_t>(self, 0x58) = uint16_t(levels);
    uint16_t skip = ms_usSkipLevels;
    if (int32_t(skip) > int32_t(uint16_t(levels)) - 1) {
        skip = uint16_t(levels - 1);
    }
    if (skip != 0) {
        uint32_t width = At<uint32_t>(self, 0xC);
        uint32_t height = At<uint32_t>(self, 0x10);
        for (uint32_t n = skip; n != 0; n--) {
            if (width & 1) {
                width++;
            }
            width >>= 1;
            if (height & 1) {
                height++;
            }
            height >>= 1;
        }
        At<uint32_t>(self, 0xC) = width;
        At<uint32_t>(self, 0x10) = height;
    }
    At<uint16_t>(self, 0x70) = skip;
    uint8_t* renderer = D3D10Renderer_GetRenderer();
    uint32_t blockSize = IsBlock8(At<uint32_t>(self, 0x20)) ? 8 : 0x10;
    for (uint32_t level = 0; level < At<uint16_t>(self, 0x58); level++) {
        int32_t pixelFormat = At<int32_t>(self, 0x18);
        uint32_t pitch = RowPitch(pixels, level, pixelFormat >= 4 && pixelFormat <= 6, blockSize);
        void* device = D3D10Renderer_GetD3DDevice(renderer);
        Com<void>(device, kUpdateSubresource, At<void*>(self, 0x60), level, static_cast<const void*>(nullptr),
                  static_cast<const void*>(pixels->pixels + pixels->offsets[level]), pitch, 0u);
    }
    return 1;
}

// 0x8B6B60 (thiscall): NiD3D10SourceTextureData::UpdatePalette (true when its revision or the
// palette itself changed)
uint8_t __fastcall SourceTextureData_UpdatePaletteImpl(uint8_t* self, re::Edx, uint8_t* palette) {
    uint8_t changed = 0;
    if (palette != nullptr) {
        uint32_t revision = At<uint32_t>(palette, 0x10);
        if (At<uint32_t>(self, 0x78) != revision) {
            changed = 1;
        }
        At<uint32_t>(self, 0x78) = revision;
        if (At<uint8_t*>(self, 0x74) != palette) {
            NiPointer_Assign(self + 0x74, palette);
            changed = 1;
        }
    }
    return changed;
}

// 0x8B6B90 (thiscall): NiD3D10SourceTextureData::NiD3D10SourceTextureData(texture) (a cube map for
// an NiSourceCubeMap)
void* __fastcall SourceTextureData_ConstructImpl(uint8_t* self, re::Edx, void* texture) {
    TextureData_Construct(self, texture);
    At<uint32_t>(self, 0) = kSourceTextureDataVtable;
    At<uint8_t>(self, 0x64) = 0;
    At<uint8_t>(self, 0x65) = 0;
    At<uint32_t>(self, 0x68) = 0;
    At<uint32_t>(self, 0x6C) = 0;
    At<uint16_t>(self, 0x70) = 0;
    At<uint16_t>(self, 0x72) = 0;
    At<void*>(self, 0x74) = nullptr;
    TypeFlags(self) |= 1;
    At<uint32_t>(self, 0x78) = 0;
    if (texture != nullptr && IsKindOf(texture, NiSourceCubeMap_ClassBit)) {
        TypeFlags(self) |= 8;
    }
    return self;
}

// 0x8B6C10 (thiscall): NiD3D10SourceTextureData::Initialize: the texture's pixel data (read from its
// file when it has none), the replacement data when there's none, a cube map's faces aren't
// square, or no format fits (static then); else converted to the format chosen (mipmapped by the
// preference, or by NiTexture's default); the size, then the D3D texture made; the palette taken;
// the texture's renderer data set (its app data dropped when NiTexture says so). Always true.
uint8_t __fastcall SourceTextureData_InitializeImpl(uint8_t* self, re::Edx) {
    uint8_t* texture = At<uint8_t*>(self, 8);
    const uint32_t* prefs = reinterpret_cast<const uint32_t*>(texture + 0x18);
    uint8_t cube = TextureData_IsCubeMap(self);
    At<uint8_t>(self, 0x64) = 0;
    NiPixelData* pixels = At<NiPixelData*>(texture, 0x3C);
    NiPixelData* source = pixels;   // (what's left at the end for the palette)
    if (pixels == nullptr) {
        const char* name = At<const char*>(texture, 0x34);
        if (name != nullptr) {
            pixels = Virtual<NiPixelData*>(ImageConverter_Get(), 3, name, static_cast<void*>(nullptr));   // ReadImageFile
            source = pixels;
        }
    }
    NiPixelData* held = pixels;
    if (pixels != nullptr) {
        Increment(&pixels->refCount);
    }
    const uint8_t* format = nullptr;
    uint32_t error = 0;
    if (pixels == nullptr) {
        error = (cube != 0 ? 3u : 0u) + 0x80000005;
    } else if (cube != 0 && pixels->widths[0] != pixels->heights[0]) {
        error = 0x80000009;
    } else {
        format = TextureData_FindMatchingPixelFormat(self, &pixels->format, prefs, (cube != 0 ? 0x60u : 0u) + 0x20);
        if (format == nullptr) {
            error = (cube != 0 ? 3u : 0u) + 0x80000004;
        }
    }
    if (error != 0) {
        pixels = Texture_GetReplacementData(texture, error, 1);
        source = pixels;
        At<uint8_t>(self, 0x64) = 1;
    }
    NiPixelData* converted;
    if (At<uint8_t>(self, 0x64) != 0) {
        format = reinterpret_cast<const uint8_t*>(&pixels->format);
        At<uint8_t>(self, 0x65) = 0;
        converted = pixels;
    } else {
        uint32_t mipmap = prefs[2];
        At<uint8_t>(self, 0x65) = (mipmap == 1 || (mipmap == 2 && ms_bMipmapByDefault != 0)) ? 1 : 0;
        converted = Virtual<NiPixelData*>(ImageConverter_Get(), 5, pixels, format, pixels,
                                          uint32_t(At<uint8_t>(self, 0x65)));
    }
    At<uint32_t>(self, 0x68) = pixels->revisionID;
    crt::Memcpy(self + 0x14, format, 0x44);
    At<uint32_t>(self, 0x6C) = converted->offsets[converted->mipmapLevels] * converted->faces;
    SourceTextureData_CreateTexture(self, converted, cube, At<uint8_t>(texture, 0x40));
    uint8_t* palette = re::PtrCast<uint8_t*>(source->palette);
    if (palette != nullptr) {
        uint8_t* old = At<uint8_t*>(self, 0x74);
        At<uint32_t>(self, 0x78) = At<uint32_t>(palette, 0x10);
        if (old != palette) {
            NiPointer_Assign(self + 0x74, palette);
        }
    }
    TouchRef(converted);
    At<void*>(texture, 0x24) = self;
    if (ms_bDestroyAppDataByDefault != 0) {
        Virtual<void>(texture, 21);   // DestroyAppPixelData
    }
    if (held != nullptr && Decrement(&held->refCount) == 0) {
        Virtual<void>(held, 1);
    }
    return 1;
}

// 0x8B6E00 (thiscall, slot 18): NiD3D10SourceTextureData::Update (unless static or the texture's
// data is static: the pixel data converted and copied again when its revision or palette changed)
void __fastcall SourceTextureData_Update(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 0x64) != 0) {
        return;
    }
    uint8_t* texture = At<uint8_t*>(self, 8);
    if (At<uint8_t>(texture, 0x40) != 0) {
        return;
    }
    NiPixelData* pixels = At<NiPixelData*>(texture, 0x3C);
    if (pixels == nullptr) {
        return;
    }
    uint8_t changed = SourceTextureData_UpdatePalette(self, pixels->palette);
    if (pixels->revisionID == At<uint32_t>(self, 0x68) && changed == 0) {
        return;
    }
    NiPixelData* converted = Virtual<NiPixelData*>(ImageConverter_Get(), 5, pixels, self + 0x14, pixels,
                                                   uint32_t(At<uint8_t>(self, 0x65)));
    At<uint32_t>(self, 0x68) = pixels->revisionID;
    if (At<void*>(self, 0x60) != nullptr) {
        SourceTextureData_CopyDataToTexture(self, converted);
    }
    if (converted != nullptr) {
        TouchRef(converted);
    }
}

// 0x8B6E90 (thiscall): NiD3D10SourceTextureData's deleting destructor (the palette let go)
void* __fastcall SourceTextureData_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kSourceTextureDataVtable;
    ReleaseRef(At<void*>(self, 0x74));
    TextureData_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x7C);
    }
    return self;
}

// 0x8B6EE0 (cdecl): NiD3D10SourceTextureData::Create(texture) (initialized even when the allocation
// failed, as the game does; the data, or 0 when it failed)
void* __cdecl SourceTextureData_Create(void* texture) {
    void* memory = NiNew(0x7C);
    void* data = memory != nullptr ? SourceTextureData_Construct(memory, texture) : nullptr;
    return SourceTextureData_Initialize(data) != 0 ? data : nullptr;
}

// 0x8B6F20 (thiscall): NiD3D10RenderedTextureData's deleting destructor
void* __fastcall RenderedTextureData_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kRenderedTextureDataVtable;
    TextureData_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x68);
    }
    return self;
}

// 0x8B6F50 (thiscall): NiD3D10RenderedTextureData::NiD3D10RenderedTextureData(texture) (six slices
// and a cube map for an NiRenderedCubeMap)
void* __fastcall RenderedTextureData_ConstructImpl(uint8_t* self, re::Edx, void* texture) {
    TextureData_Construct(self, texture);
    TypeFlags(self) |= 1;
    At<uint32_t>(self, 0) = kRenderedTextureDataVtable;
    At<uint16_t>(self, 0x64) = 1;
    if (texture != nullptr && IsKindOf(texture, NiRenderedCubeMap_ClassBit)) {
        TypeFlags(self) |= 8;
        At<uint16_t>(self, 0x64) = 6;
    }
    return self;
}

// 0x8B6FC0 (thiscall): NiD3D10RenderedTextureData::CreateSurf(multisample: samples << 8 | quality):
// the texture's size, one level, the closest format for its preferences; a render target texture
// (shader resource and render target bound) and its view (cube, 2D, 2D multisampled, or arrays of
// them); the render target buffer data of the texture's buffer, or of each cube face's (a slice
// each); the data initialized. (A view that fails is let go but still handed on, as in the game.)
uint8_t __fastcall RenderedTextureData_CreateSurfImpl(uint8_t* self, re::Edx, uint32_t multisample) {
    uint8_t* texture = At<uint8_t*>(self, 8);
    if (texture == nullptr) {
        return 0;
    }
    At<uint32_t>(self, 0xC) = Virtual<uint32_t>(texture, 18);   // GetWidth
    At<uint32_t>(self, 0x10) = Virtual<uint32_t>(texture, 19);  // GetHeight
    At<uint16_t>(self, 0x58) = 1;
    uint32_t prefs[3] = {At<uint32_t>(texture, 0x18), At<uint32_t>(texture, 0x1C), At<uint32_t>(texture, 0x20)};
    uint8_t cube = TextureData_IsCubeMap(self);
    const uint8_t* closest = TextureData_FindClosestPixelFormat(self, prefs, (cube != 0 ? 0x60u : 0u) + 0x20);
    crt::Memcpy(self + 0x14, closest, 0x44);
    uint32_t format = D3D10_GetDXGIFormat(self + 0x14);
    int32_t samples = int32_t(uint16_t(multisample) >> 8);
    if (samples <= 1) {
        samples = 1;
    }
    uint8_t* renderer = D3D10Renderer_GetRenderer();
    void* resource = ResourceManager_CreateTexture2D(
        D3D10Renderer_GetResourceManager(renderer), At<uint32_t>(self, 0xC), At<uint32_t>(self, 0x10),
        At<uint16_t>(self, 0x58), At<uint16_t>(self, 0x64), format, uint32_t(samples), uint8_t(multisample), 0, 0x28, 0,
        cube != 0 ? 4u : 0u, nullptr);
    if (resource == nullptr) {
        return 0;
    }
    ViewDesc desc;
    desc.format = format;
    uint16_t slices = At<uint16_t>(self, 0x64);
    if (cube != 0) {
        desc.dimension = 9;   // TEXTURECUBE
        desc.u[1] = At<uint16_t>(self, 0x58);
        desc.u[0] = 0;
    } else if (slices == 1) {
        if (samples == 1) {
            desc.dimension = 4;   // TEXTURE2D
            desc.u[1] = At<uint16_t>(self, 0x58);
            desc.u[0] = 0;
        } else {
            desc.dimension = 6;   // TEXTURE2DMS
        }
    } else if (samples == 1) {
        desc.dimension = 5;   // TEXTURE2DARRAY
        desc.u[2] = 0;
        desc.u[3] = slices;
        desc.u[1] = At<uint16_t>(self, 0x58);
        desc.u[0] = 0;
    } else {
        desc.dimension = 7;   // TEXTURE2DMSARRAY
        desc.u[1] = slices;
        desc.u[0] = 0;
    }
    void* view = nullptr;
    long hr = Com<long>(D3D10Renderer_GetD3DDevice(renderer), kCreateShaderResourceView, resource, &desc, &view);
    if (hr < 0 && view != nullptr) {
        ComRelease(view);
    }
    if (Virtual<void*>(texture, 20) != nullptr) {   // GetBuffer
        re::Ptr32<void> buffer = Virtual<void*>(texture, 20);
        if (RenderTargetBufferData_Create(resource, &buffer, nullptr) == nullptr) {
            ni::dx9::NiOutputDebugString("NiD3D10RenderedTextureData::Create> Failed "
                                         "NiD3D10RenderTargetBufferData::Create - %s",
                                         "");
            return 0;
        }
    }
    if (TextureData_IsCubeMap(self) != 0) {
        ViewDesc target;   // D3D10_RENDER_TARGET_VIEW_DESC: a slice each
        target.format = format;
        target.u[0] = 0;
        if (samples == 1) {
            target.dimension = 5;   // TEXTURE2DARRAY: mip slice, first slice, one slice
            target.u[1] = 0;
            target.u[2] = 1;
        } else {
            target.dimension = 7;   // TEXTURE2DMSARRAY: first slice, one slice
            target.u[1] = 1;
        }
        for (uint32_t face = 0; face < 6; face++) {
            re::Ptr32<void> buffer = At<void*>(texture, 0x38 + int(face) * 4);   // (NiRenderedCubeMap's face buffers)
            if (samples == 1) {
                target.u[1] = face;
            } else {
                target.u[0] = face;
            }
            if (RenderTargetBufferData_Create(resource, &buffer, &target) == nullptr) {
                ni::dx9::NiOutputDebugString("NiD3D10RenderedTextureData::Create> Failed "
                                             "NiD3D10RenderTargetBufferData::Create - %s",
                                             "");
                return 0;
            }
        }
    }
    if (TextureData_InitializeFromD3D10Resource(self, resource, view) == 0) {
        return 0;
    }
    At<void*>(texture, 0x24) = self;
    return 1;
}

// 0x8B7270 (cdecl): NiD3D10RenderedTextureData::Create(texture, multisample) (deleted when it
// fails; made even when the allocation failed, as the game does)
void* __cdecl RenderedTextureData_Create(void* texture, uint32_t multisample) {
    void* memory = NiNew(0x68);
    void* data = memory != nullptr ? RenderedTextureData_Construct(memory, texture) : nullptr;
    if (RenderedTextureData_CreateSurf(data, multisample) != 0) {
        return data;
    }
    if (data != nullptr) {
        Virtual<void*>(data, 0, 1u);
    }
    return nullptr;
}

#define GB43E_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

GB43E_REVERSED("NiD3D10TextureData::IsRenderedTexture", 0x008B4580, TextureData_IsRenderedTexture);
GB43E_REVERSED("NiD3D10TextureData::IsCubeMap", 0x008B4590, TextureData_IsCubeMapImpl);
GB43E_REVERSED("NiD3D10TextureData::InitializeFromD3D10Resource", 0x008B45A0,
               TextureData_InitializeFromD3D10ResourceImpl);
GB43E_REVERSED("NiD3D10TextureData::NiD3D10TextureData", 0x008B4620, TextureData_ConstructImpl);
GB43E_REVERSED("NiD3D10TextureData::~NiD3D10TextureData", 0x008B4670, TextureData_DestructImpl);
GB43E_REVERSED("NiD3D10TextureData::FindMatchingPixelFormat", 0x008B46B0, TextureData_FindMatchingPixelFormatImpl);
GB43E_REVERSED("NiD3D10TextureData::FindClosestPixelFormat", 0x008B5030, TextureData_FindClosestPixelFormatImpl);
GB43E_REVERSED("NiD3D10TextureData::DeletingDestructor", 0x008B53F0, TextureData_DeletingDestructor);
GB43E_REVERSED("NiD3D10SourceTextureData::CreateTexture", 0x008B6730, SourceTextureData_CreateTextureImpl);
GB43E_REVERSED("NiD3D10SourceTextureData::CopyDataToTexture", 0x008B6A20, SourceTextureData_CopyDataToTextureImpl);
GB43E_REVERSED("NiD3D10SourceTextureData::UpdatePalette", 0x008B6B60, SourceTextureData_UpdatePaletteImpl);
GB43E_REVERSED("NiD3D10SourceTextureData::NiD3D10SourceTextureData", 0x008B6B90, SourceTextureData_ConstructImpl);
GB43E_REVERSED("NiD3D10SourceTextureData::Initialize", 0x008B6C10, SourceTextureData_InitializeImpl);
GB43E_REVERSED("NiD3D10SourceTextureData::Update", 0x008B6E00, SourceTextureData_Update);
GB43E_REVERSED("NiD3D10SourceTextureData::DeletingDestructor", 0x008B6E90, SourceTextureData_DeletingDestructor);
GB43E_REVERSED("NiD3D10SourceTextureData::Create", 0x008B6EE0, SourceTextureData_Create);
GB43E_REVERSED("NiD3D10RenderedTextureData::DeletingDestructor", 0x008B6F20, RenderedTextureData_DeletingDestructor);
GB43E_REVERSED("NiD3D10RenderedTextureData::NiD3D10RenderedTextureData", 0x008B6F50,
               RenderedTextureData_ConstructImpl);
GB43E_REVERSED("NiD3D10RenderedTextureData::CreateSurf", 0x008B6FC0, RenderedTextureData_CreateSurfImpl);
GB43E_REVERSED("NiD3D10RenderedTextureData::Create", 0x008B7270, RenderedTextureData_Create);

} // namespace ni::c43
