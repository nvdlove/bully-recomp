// NiDX9SourceTextureData (vtable 0x99CDC0, 0x7C bytes; NiDX9TextureData's then +0x64 static
// (replacement data or no updates), +0x65 mipmapped, +0x68 its size in bytes, +0x6C the palette
// (a reference), +0x70 the mipmap levels skipped, +0x74 the pixel data's revision, +0x78 the
// palette's): a source texture's D3D texture, made from its pixel data (converted to the chosen
// format), from its persistent renderer data, or straight from its file through D3DX; the
// replacement texture when none can be (the format, the size, a missing file). Here Create, the
// texture's creation and copies (levels from the skip level on), InitializeFromD3DTexture (slot
// 18), Update (slot 19: the pixel data again when its revision or palette changed), GetPalette
// (slot 20), the destructors.
//
// The NiPixelData read: src/gamebryo/NiPixelData.h. NiPersistentSrcTextureRendererData read by
// offset: +0x14 its NiPixelFormat, +0x58 the pixels, +0x5C widths, +0x60 heights, +0x64 offsets,
// +0x6C levels, +0x70 pixel stride, +0x74 faces, +0x88 the palette.

#include "gamebryo/NiDX9RendererData.h"
#include "gamebryo/NiPixelData.h"
#include "reversed.h"

namespace ni::c40 {
namespace {

constexpr uintptr_t kSourceTextureDataVtable = 0x0099CDC0;
// IDirect3D9 / device / texture / surface slots
enum : int {
    kCheckDeviceFormat = 10,
    kCreateTexture = 23,
    kGetType = 10,
    kGetLevelCount = 13,
    kGetLevelDesc = 17,
    kGetSurfaceLevel = 18,
    kSurfGetDesc = 12,
    kSurfLockRect = 13,
    kSurfUnlockRect = 14,
    kRelease = 2,
};
constexpr uintptr_t kD3DXGetImageInfoFromFileInMemory = 0x008FC2C4;
constexpr uintptr_t kD3DXCreateTextureFromFileInMemory = 0x008FC2C8;
constexpr uintptr_t kD3DXCreateCubeTextureFromFileInMemory = 0x008FC30C;
constexpr uintptr_t kD3DXCreateVolumeTextureFromFileInMemory = 0x008FC310;

GAME_VAR(uint16_t, ms_usSkipLevels, 0x020E7300);        // NiDX9Renderer's mipmap skip level (its low half)
GAME_VAR(uint32_t, NiSourceCubeMap_ClassBit, 0x020E3CA8);
GAME_VAR(uint8_t, ms_bMipmapByDefault, 0x020E2180);     // NiTexture's (a "default" mipmap preference's)
GAME_VAR(uint8_t, ms_bDestroyAppDataByDefault, 0x00BC7622);

GAME_FN(const char*, __cdecl, NiDX9ErrorString, 0x00899150, long hr);
GAME_FN(uint32_t, __cdecl, PixelFormat_GetD3DFormat, 0x00895340, const void* format);
GAME_FN(void, __cdecl, PixelFormat_FromD3DFormat, 0x00895800, uint32_t format, void* out);
GAME_FN(void*, __thiscall, PixelFormat_Construct, 0x0076B280, void* self);
GAME_FN(void*, __cdecl, ImageConverter_Get, 0x007D3B40);   // NiImageConverter::GetImageConverter
GAME_FN(void*, __thiscall, TextureData_Construct, 0x00892960, void* self, void* texture, void* renderer);
GAME_FN(void, __thiscall, TextureData_Destruct, 0x008929B0, void* self);
GAME_FN(void*, __thiscall, TextureData_FindMatchingPixelFormat, 0x008929F0, void* self, const void* source,
        const uint32_t* prefs, const re::Ptr32<void>* formats);
GAME_FN(void, __thiscall, NiPointer_Assign, 0x00800420, void* slot, void* object);
GAME_FN(NiPixelData*, __cdecl, Texture_GetReplacementData, 0x00755170, void* texture, uint32_t error, int warn);
GAME_FN(void*, __cdecl, SourceCubeMapData_Create, 0x00897D40, void* texture, void* renderer);
GAME_FN(void, __cdecl, Path_Standardize, 0x00752C20, char* path);
GAME_FN(void, __thiscall, Filename_Construct, 0x007538C0, void* self, const char* path);
GAME_FN(int, __cdecl, NiStricmp, 0x00753820, const char* a, const char* b);
GAME_FN(void*, __cdecl, NiFile_GetFile, 0x00753380, const char* name, int mode, uint32_t bufferSize);
GAME_FN(uint32_t, __thiscall, BinaryStream_Read, 0x00404F10, void* self, void* buffer, uint32_t bytes);
// this file's (called where modes apply)
GAME_FN(uint8_t, __thiscall, SourceTextureData_CreateSurf, 0x00895B60, void* self, const NiPixelData* pixels);
GAME_FN(void, __cdecl, SourceTextureData_CopyDataToSurfaceLevel, 0x00895CD0, const NiPixelData* pixels,
        uint32_t level, void* surface, uint32_t face);
GAME_FN(uint8_t, __thiscall, SourceTextureData_CreateSurfFromRendererData, 0x00895E30, void* self,
        const uint8_t* persistent, uint32_t format);
GAME_FN(void, __cdecl, SourceTextureData_CopyRendererDataToSurfaceLevel, 0x00895F90, const uint8_t* persistent,
        uint32_t level, void* surface, uint32_t face);
GAME_FN(void*, __thiscall, SourceTextureData_Construct, 0x00896370, void* self, void* texture, void* renderer);
GAME_FN(void, __thiscall, SourceTextureData_CopyDataToSurface, 0x00896400, void* self, const NiPixelData* pixels);
GAME_FN(void, __thiscall, SourceTextureData_CopyRendererDataToSurface, 0x008964C0, void* self,
        const uint8_t* persistent);
GAME_FN(uint8_t, __thiscall, SourceTextureData_LoadTextureFile, 0x00896620, void* self, const char* name,
        uint8_t* renderer, const uint32_t* prefs);

// D3DSURFACE_DESC, D3DLOCKED_RECT
struct SurfaceDesc {
    uint32_t format;
    uint32_t type;
    uint32_t usage;
    uint32_t pool;
    uint32_t multiSampleType;
    uint32_t multiSampleQuality;
    uint32_t width;
    uint32_t height;
};
struct LockedRect {
    uint32_t pitch;
    uint8_t* bits;
};

inline bool PowerOfTwo(uint32_t n) {
    return n != 0 && (n & (n - 1)) == 0;
}
inline bool IsDXT(uint32_t format) {
    return format == 0x31545844 || format == 0x32545844 || format == 0x33545844 || format == 0x34545844 ||
           format == 0x35545844;
}
inline uint32_t RendererFlags(void* renderer) {
    return Virtual<uint32_t>(renderer, 19);   // GetFlags
}
inline void ReleaseObject(NiRefObject* object) {
    if (object != nullptr && Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

// the texture made: the levels from the source (one for a non power of two size the device can't
// mipmap), the skip level taken off (the size halved, rounding up, that many times; at least 4 for
// DXT), managed. Returns CreateTexture's result; *made whether there's a texture
long CreateTexture(uint8_t* self, uint32_t format, uint16_t sourceLevels, uint8_t* mipmapFlag, bool* made) {
    if (!PowerOfTwo(At<uint32_t>(self, 0xC)) || !PowerOfTwo(At<uint32_t>(self, 0x10))) {
        if ((RendererFlags(At<void*>(self, 0x58)) & 8) == 0) {
            At<uint16_t>(self, 0x60) = 1;
            goto levelsSet;
        }
    }
    At<uint16_t>(self, 0x60) = sourceLevels;
levelsSet:
    uint16_t levels = At<uint16_t>(self, 0x60);
    if (mipmapFlag != nullptr) {
        *mipmapFlag = levels != 1 ? 1 : 0;
    }
    uint16_t skip = ms_usSkipLevels;
    if (int(skip) > int(levels) - 1) {
        skip = uint16_t(levels - 1);
    }
    if (skip != 0) {
        uint32_t width = At<uint32_t>(self, 0xC);
        uint32_t height = At<uint32_t>(self, 0x10);
        for (uint32_t i = skip; i != 0; i--) {
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
    if (IsDXT(format)) {
        if (At<uint32_t>(self, 0xC) < 4) {
            At<uint32_t>(self, 0xC) = 4;
        }
        if (At<uint32_t>(self, 0x10) < 4) {
            At<uint32_t>(self, 0x10) = 4;
        }
    }
    At<uint32_t>(self, 0x70) = skip;
    void* device = At<void*>(At<uint8_t*>(self, 0x58), 0xA4);
    void* texture = nullptr;
    long hr = Com<long>(device, kCreateTexture, At<uint32_t>(self, 0xC), At<uint32_t>(self, 0x10),
                        uint32_t(levels) - skip, 0u, format, 1u, &texture, static_cast<void*>(nullptr));
    *made = hr >= 0 && texture != nullptr;
    if (*made) {
        At<void*>(self, 0x5C) = texture;
    }
    return hr;
}

// one level's pixels copied into a surface: row by row unless the pitch matches (a DXT format's
// rows of blocks: the pitch quartered, the rows rounded up to 4, a row's bytes the blocks'; the
// destination still advances by the whole pitch, as the original does)
void CopyLevel(const uint8_t* pixels, const uint32_t* widths, const uint32_t* heights, const uint32_t* offsets,
               uint32_t levels, uint32_t stride, uint32_t sourceFormat, uint32_t level, void* surface, uint32_t face,
               const char* descFailed, const char* lockFailed) {
    SurfaceDesc desc;
    long hr = Com<long>(surface, kSurfGetDesc, &desc);
    if (hr < 0) {
        ni::dx9::NiOutputDebugString(descFailed, NiDX9ErrorString(hr));
        return;
    }
    LockedRect locked;
    hr = Com<long>(surface, kSurfLockRect, &locked, static_cast<void*>(nullptr), 0u);
    if (hr < 0) {
        ni::dx9::NiOutputDebugString(lockFailed, NiDX9ErrorString(hr));
        return;
    }
    uint32_t pitch = locked.pitch;
    uint32_t width = widths[level];
    uint32_t rows = heights[level];
    uint32_t rowBytes = stride * width;
    if (rows > desc.height) {
        rows = desc.height;
    }
    if (int(sourceFormat) >= 4 && int(sourceFormat) <= 6) {   // DXT1/3/5
        pitch >>= 2;
        rowBytes = width + 3;
        if (desc.format == 0x31545844) {
            rowBytes = (rowBytes >> 1) & 0x7FFFFFE;
        } else {
            rowBytes &= 0xFFFFFFC;
        }
        rows = (rows + 3) & 0xFFFFFFC;
    }
    const uint8_t* source = pixels + offsets[levels] * face + offsets[level];
    if (rowBytes == pitch) {
        crt::MemcpyS(locked.bits, rows * rowBytes, source, rows * rowBytes);
    } else {
        uint32_t bytes = rowBytes > pitch ? pitch : rowBytes;
        uint8_t* destination = locked.bits;
        for (uint32_t i = rows; i != 0; i--) {
            crt::MemcpyS(destination, bytes, source, bytes);
            destination += locked.pitch;
            source += rowBytes;
        }
    }
    Com<long>(surface, kSurfUnlockRect);
}

// every level from the skip level on copied (a 2D texture's only)
template <typename Copy>
void CopyLevels(uint8_t* self, const char* failed, Copy copy) {
    void* texture = At<void*>(self, 0x5C);
    if (texture == nullptr || Com<uint32_t>(texture, kGetType) == 5 ||
        Com<uint32_t>(At<void*>(self, 0x5C), kGetType) == 4) {
        return;
    }
    texture = At<void*>(self, 0x5C);
    for (uint32_t i = 0; i < uint32_t(At<uint16_t>(self, 0x60)) - At<uint32_t>(self, 0x70); i++) {
        void* surface;
        long hr = Com<long>(texture, kGetSurfaceLevel, i, &surface);
        if (hr < 0) {
            ni::dx9::NiOutputDebugString(failed, i, NiDX9ErrorString(hr));
            return;
        }
        copy(At<uint32_t>(self, 0x70) + i, surface);
        Com<long>(surface, kRelease);
    }
}

// the palette taken (its revision remembered); true when it changed
inline bool TakePalette(uint8_t* self, uint8_t* palette) {
    bool changed = false;
    if (palette != nullptr) {
        changed = At<uint32_t>(self, 0x78) != At<uint32_t>(palette, 0x10);
        At<uint32_t>(self, 0x78) = At<uint32_t>(palette, 0x10);
        if (At<void*>(self, 0x6C) != palette) {
            NiPointer_Assign(self + 0x6C, palette);
            changed = true;
        }
    }
    return changed;
}

} // namespace

// 0x895B60 (thiscall): NiDX9SourceTextureData::CreateSurf (the texture for the pixel data, of its
// own pixel format's D3D format)
uint8_t __fastcall NiDX9SourceTextureData_CreateSurf(uint8_t* self, re::Edx, const NiPixelData* pixels) {
    uint32_t format = PixelFormat_GetD3DFormat(self + 0x14);
    At<uint32_t>(self, 0xC) = pixels->widths[0];
    At<uint32_t>(self, 0x10) = pixels->heights[0];
    bool made;
    long hr = CreateTexture(self, format, uint16_t(pixels->mipmapLevels), nullptr, &made);
    if (made) {
        return 1;
    }
    ni::dx9::NiOutputDebugString("NiDX9SourceTextureData::CreateSurf> Failed CreateImageSurface - %s\n",
                                 NiDX9ErrorString(hr));
    At<void*>(self, 0x5C) = nullptr;
    return 0;
}

// 0x895CD0 (cdecl): NiDX9SourceTextureData::CopyDataToSurfaceLevel (Quirk kept: a lock failure is
// reported as GetSurfaceLevel's)
void __cdecl NiDX9SourceTextureData_CopyDataToSurfaceLevel(const NiPixelData* pixels, uint32_t level, void* surface,
                                                           uint32_t face) {
    CopyLevel(pixels->pixels, pixels->widths, pixels->heights, pixels->offsets, pixels->mipmapLevels,
              pixels->pixelStride, uint32_t(pixels->format.format), level, surface, face,
              "NiDX9SourceTextureData::CopyDataToSurfaceLevel> Failed GetDesc - %s",
              "NiDX9SourceTextureData::CopyDataToSurfaceLevel> Failed GetSurfaceLevel - %s");
}

// 0x895E30 (thiscall): NiDX9SourceTextureData::CreateSurfFromRendererData (the texture for the
// persistent data in the format given; its size already set)
uint8_t __fastcall NiDX9SourceTextureData_CreateSurfFromRendererData(uint8_t* self, re::Edx, const uint8_t* persistent,
                                                                     uint32_t format) {
    bool made;
    long hr = CreateTexture(self, format, uint16_t(At<uint32_t>(persistent, 0x6C)), self + 0x65, &made);
    if (made) {
        return 1;
    }
    ni::dx9::NiOutputDebugString(
        "NiDX9SourceTextureData::CreateSurfFromRendererData> FAILED to create texture of format: 0x%x, D3D error: %s",
        format, NiDX9ErrorString(hr));
    At<void*>(self, 0x5C) = nullptr;
    return 0;
}

// 0x895F90 (cdecl): NiDX9SourceTextureData::CopyRendererDataToSurfaceLevel
void __cdecl NiDX9SourceTextureData_CopyRendererDataToSurfaceLevel(const uint8_t* persistent, uint32_t level,
                                                                   void* surface, uint32_t face) {
    CopyLevel(At<const uint8_t*>(persistent, 0x58), At<const uint32_t*>(persistent, 0x5C),
              At<const uint32_t*>(persistent, 0x60), At<const uint32_t*>(persistent, 0x64),
              At<uint32_t>(persistent, 0x6C), At<uint32_t>(persistent, 0x70), At<uint32_t>(persistent, 0x18), level,
              surface, face, "NiDX9SourceTextureData::CopyRendererDataToSurfaceLevel> Failed GetDesc - %s",
              "NiDX9SourceTextureData::CopyRendererDataToSurfaceLevel> Failed GetSurfaceLevel - %s");
}

// 0x8960F0 (thiscall): NiDX9SourceTextureData::InitializeFromD3DTexture (the levels, the size and
// format of the top one, the total size: a cube's six faces; the texture kept (its reference
// taken); false for a palettized one)
uint8_t __fastcall NiDX9SourceTextureData_InitializeFromD3DTexture(uint8_t* self, re::Edx, void* texture) {
    uint16_t levels = uint16_t(Com<uint32_t>(texture, kGetLevelCount));
    At<uint16_t>(self, 0x60) = levels;
    At<uint8_t>(self, 0x65) = levels != 1 ? 1 : 0;
    At<uint32_t>(self, 0x68) = 0;
    uint32_t type = Com<uint32_t>(texture, kGetType);
    if (type != 3 && type != 5 && type != 4) {
        At<void*>(self, 0x5C) = texture;
        return 1;
    }
    for (uint16_t i = 0; i < At<uint16_t>(self, 0x60); i++) {
        uint32_t desc[8];   // D3DSURFACE_DESC (D3DVOLUME_DESC for a volume)
        if (Com<long>(texture, kGetLevelDesc, uint32_t(i), desc) < 0) {
            return 0;
        }
        uint32_t width = type == 4 ? desc[4] : desc[6];
        uint32_t height = type == 4 ? desc[5] : desc[7];
        if (i == 0) {
            At<uint32_t>(self, 0xC) = width;
            At<uint32_t>(self, 0x10) = height;
            if (desc[0] == 0x28 || desc[0] == 0x29) {
                return 0;
            }
            alignas(4) uint8_t blank[0x44];
            crt::Memcpy(self + 0x14, PixelFormat_Construct(blank), 0x44);
            PixelFormat_FromD3DFormat(desc[0], self + 0x14);
        }
        uint32_t bits = At<uint8_t>(self, 0x15);
        if (type == 4) {
            At<uint32_t>(self, 0x68) += (bits * desc[6] * height * width) >> 3;
        } else {
            At<uint32_t>(self, 0x68) += (bits * height * width) >> 3;
        }
    }
    At<void*>(self, 0x5C) = texture;
    if (type == 5) {
        At<uint32_t>(self, 0x68) *= 6;
    }
    return 1;
}

// 0x896340 (thiscall): NiDX9SourceTextureData::UpdatePalette (true when it changed)
uint8_t __fastcall NiDX9SourceTextureData_UpdatePalette(uint8_t* self, re::Edx, uint8_t* palette) {
    return TakePalette(self, palette) ? 1 : 0;
}

// 0x896370 (thiscall): NiDX9SourceTextureData's constructor
void* __fastcall NiDX9SourceTextureData_Construct(uint8_t* self, re::Edx, void* texture, void* renderer) {
    TextureData_Construct(self, texture, renderer);
    At<uint8_t>(self, 0x64) = 0;
    At<uint8_t>(self, 0x65) = 0;
    At<uint32_t>(self, 0x68) = 0;
    At<uint32_t>(self, 0) = kSourceTextureDataVtable;
    At<void*>(self, 0x6C) = nullptr;
    At<uint16_t>(self, 0x62) |= 1;
    At<uint32_t>(self, 0x70) = 0;
    At<uint32_t>(self, 0x74) = 0;
    At<uint32_t>(self, 0x78) = 0;
    return self;
}

// 0x8963B0 (thiscall, 4 bytes: under a hook's size): NiDX9SourceTextureData::GetPalette
void* __fastcall NiDX9SourceTextureData_GetPalette(uint8_t* self, re::Edx) {
    return At<void*>(self, 0x6C);
}
RE_UNHOOKED_FN("NiDX9SourceTextureData::GetPalette", 0x008963B0, &NiDX9SourceTextureData_GetPalette);

// 0x8963C0 (thiscall): NiDX9SourceTextureData's destructor
void __fastcall NiDX9SourceTextureData_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kSourceTextureDataVtable;
    ReleaseObject(At<NiRefObject*>(self, 0x6C));
    TextureData_Destruct(self);
}

// 0x896400 (thiscall): NiDX9SourceTextureData::CopyDataToSurface (the levels its pixel data has)
void __fastcall NiDX9SourceTextureData_CopyDataToSurface(uint8_t* self, re::Edx, const NiPixelData* pixels) {
    void* texture = At<void*>(self, 0x5C);
    if (texture == nullptr || Com<uint32_t>(texture, kGetType) == 5 ||
        Com<uint32_t>(At<void*>(self, 0x5C), kGetType) == 4) {
        return;
    }
    At<uint16_t>(self, 0x60) = uint16_t(pixels->mipmapLevels);
    texture = At<void*>(self, 0x5C);
    for (uint32_t i = 0; i < uint32_t(At<uint16_t>(self, 0x60)) - At<uint32_t>(self, 0x70); i++) {
        void* surface;
        long hr = Com<long>(texture, kGetSurfaceLevel, i, &surface);
        if (hr < 0) {
            ni::dx9::NiOutputDebugString("NiDX9SourceTextureData::CopyDataToSurface> Failed GetSurfaceLevel (%d)- %s", i,
                                         NiDX9ErrorString(hr));
            return;
        }
        SourceTextureData_CopyDataToSurfaceLevel(pixels, At<uint32_t>(self, 0x70) + i, surface, 0);
        Com<long>(surface, kRelease);
    }
}

// 0x8964C0 (thiscall): NiDX9SourceTextureData::CopyRendererDataToSurface
void __fastcall NiDX9SourceTextureData_CopyRendererDataToSurface(uint8_t* self, re::Edx, const uint8_t* persistent) {
    CopyLevels(self, "NiDX9SourceTextureData::CopyRendererDataToSurface> Failed GetSurfaceLevel (%d)- %s",
               [&](uint32_t level, void* surface) {
                   SourceTextureData_CopyRendererDataToSurfaceLevel(persistent, level, surface, 0);
               });
}

// 0x896570 (thiscall): NiDX9SourceTextureData::Update (unless static: the texture's pixel data
// converted and copied again when its revision or palette changed)
void __fastcall NiDX9SourceTextureData_Update(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 0x64) != 0) {
        return;
    }
    NiPixelData* pixels = At<NiPixelData*>(At<uint8_t*>(self, 8), 0x3C);
    if (pixels == nullptr) {
        return;
    }
    bool changed = TakePalette(self, re::PtrCast<uint8_t*>(pixels->palette));
    if (pixels->revisionID == At<uint32_t>(self, 0x74) && !changed) {
        return;
    }
    void* converter = ImageConverter_Get();
    NiPixelData* converted =
        Virtual<NiPixelData*>(converter, 5, pixels, self + 0x14, pixels, uint32_t(At<uint8_t>(self, 0x65)));
    At<uint32_t>(self, 0x74) = pixels->revisionID;
    if (At<void*>(self, 8) != nullptr) {
        SourceTextureData_CopyDataToSurface(self, converted);
    }
    if (converted != nullptr) {   // (a NiPointer of it, gone)
        Increment(&converted->refCount);
        ReleaseObject(converted);
    }
}

// 0x896620 (thiscall): NiDX9SourceTextureData::LoadTextureFile (a .bmp, .tga or .dds read whole
// and made a D3D texture by D3DX, of its own kind (for a bump map, only one of the bump formats);
// true when it initialized the data)
uint8_t __fastcall NiDX9SourceTextureData_LoadTextureFile(uint8_t* self, re::Edx, const char* name, uint8_t* renderer,
                                                          const uint32_t* prefs) {
    if (renderer == nullptr) {
        return 0;
    }
    void* device = At<void*>(renderer, 0xA4);
    if (device == nullptr) {
        return 0;
    }
    char path[0x104];
    alignas(4) char filename[0x404];   // NiFilename (its extension at +0x103)
    crt::StrcpyS(path, sizeof(path), name);
    Path_Standardize(path);
    Filename_Construct(filename, path);
    const char* extension = filename + 0x103;
    if (NiStricmp(extension, ".bmp") != 0 && NiStricmp(extension, ".tga") != 0 && NiStricmp(extension, ".dds") != 0) {
        return 0;
    }
    void* file = NiFile_GetFile(path, 0, 0x8000);
    if (file == nullptr) {
        return 0;
    }
    if (Virtual<uint8_t>(file, 1) == 0) {   // operator bool
        Virtual<void*>(file, 0, 1);
        return 0;
    }
    uint32_t size = Virtual<uint32_t>(file, 6);   // GetFileSize
    if (size == 0) {
        Virtual<void*>(file, 0, 1);
        return 0;
    }
    void* buffer = NiMalloc(size);
    BinaryStream_Read(file, buffer, size);
    Virtual<void*>(file, 0, 1);
    uint32_t info[7];   // D3DXIMAGE_INFO: width, height, depth, levels, format, resource type, file format
    long hr = Import<long>(kD3DXGetImageInfoFromFileInMemory, buffer, size, info);
    if (hr < 0 || (prefs[0] == 4 && info[4] != 0x3C && info[4] != 0x3F && info[4] != 0x40 && info[4] != 0x6E &&
                   info[4] != 0x75 && info[4] != 0x3D && info[4] != 0x3E && info[4] != 0x43)) {
        NiFree(buffer);
        return 0;
    }
    void* texture = nullptr;
    if (info[5] == 3) {
        hr = Import<long>(kD3DXCreateTextureFromFileInMemory, device, buffer, size, &texture);
    } else if (info[5] == 5) {
        hr = Import<long>(kD3DXCreateCubeTextureFromFileInMemory, device, buffer, size, &texture);
    } else if (info[5] == 4) {
        hr = Import<long>(kD3DXCreateVolumeTextureFromFileInMemory, device, buffer, size, &texture);
    } else {
        hr = long(0x8876086C);   // D3DERR_INVALIDCALL
    }
    NiFree(buffer);
    if (hr < 0 || texture == nullptr) {
        return 0;
    }
    uint8_t ok = Virtual<uint8_t>(self, 18, texture);   // InitializeFromD3DTexture
    if (ok == 0) {
        Com<long>(texture, kRelease);
    }
    return ok;
}

// 0x8968A0 (thiscall): NiDX9SourceTextureData's deleting destructor
void* __fastcall NiDX9SourceTextureData_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = kSourceTextureDataVtable;
    ReleaseObject(At<NiRefObject*>(self, 0x6C));
    TextureData_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x7C);
    }
    return self;
}

// 0x8968F0 (cdecl): NiDX9SourceTextureData::Create (a cube map's own data for a cube map; else the
// data from the persistent renderer data, or straight from the file when the texture wants it, or
// from its pixel data (read from the file when it has none) converted to the best format the
// device takes (the replacement data, static, for a missing file, an unusable format or size);
// the texture's renderer data set)
void* __cdecl NiDX9SourceTextureData_Create(uint8_t* texture, uint8_t* renderer) {
    if (texture != nullptr && ni::dx9::HasClassBit(texture, NiSourceCubeMap_ClassBit)) {
        return SourceCubeMapData_Create(texture, renderer);
    }
    const re::Ptr32<void>* formats = reinterpret_cast<const re::Ptr32<void>*>(renderer + 0x4FC);
    void* memory = NiNew(0x7C);
    uint8_t* data = memory != nullptr ? static_cast<uint8_t*>(SourceTextureData_Construct(memory, texture, renderer))
                                      : nullptr;
    uint8_t* persistent = At<uint8_t*>(texture, 0x38);
    NiPixelData* pixels = nullptr;
    const uint32_t* prefs = reinterpret_cast<const uint32_t*>(texture + 0x18);
    if (persistent == nullptr) {
        pixels = At<NiPixelData*>(texture, 0x3C);
        if (pixels == nullptr) {
            if (At<uint8_t>(texture, 0x41) != 0 &&
                SourceTextureData_LoadTextureFile(data, At<const char*>(texture, 0x34), renderer, prefs) != 0) {
                At<void*>(texture, 0x24) = data;
                return data;
            }
            pixels = Virtual<NiPixelData*>(ImageConverter_Get(), 3, At<const char*>(texture, 0x34),
                                           static_cast<void*>(nullptr));   // ReadImageFile
        }
    }
    void* format = nullptr;
    uint32_t d3dFormat = 0;
    NiPixelData* held = pixels;   // (a NiPointer of the pixel data)
    if (pixels != nullptr) {
        Increment(&pixels->refCount);
    }
    auto replace = [&](uint32_t error) {
        pixels = Texture_GetReplacementData(texture, error, 1);
        At<uint8_t>(data, 0x64) = 1;
    };
    if (persistent == nullptr) {
        if (pixels == nullptr) {
            pixels = Texture_GetReplacementData(texture, 0x80000005, 1);
            ni::dx9::NiOutputDebugString("Failed to find texture %s\n", At<const char*>(texture, 0x34));
            At<uint8_t>(data, 0x64) = 1;
        } else {
            uint32_t width = pixels->widths[0];
            uint32_t height = pixels->heights[0];
            bool searched = false;
            if ((!PowerOfTwo(width) || !PowerOfTwo(height)) && (RendererFlags(renderer) & 8) == 0) {
                if ((RendererFlags(renderer) & 4) == 0) {
                    replace(0x80000006);
                    searched = true;
                } else {
                    uint32_t local[3];
                    const uint32_t* usePrefs = prefs;
                    if (prefs[0] == 3) {   // compressed: high colour instead
                        local[0] = prefs[0];
                        local[1] = prefs[1];
                        local[2] = prefs[2];
                        local[0] = 2;
                        usePrefs = local;
                    }
                    format = TextureData_FindMatchingPixelFormat(data, &pixels->format, usePrefs, formats);
                    if (format == nullptr) {
                        replace(0x80000004);
                    }
                    searched = true;
                }
            }
            if (!searched) {
                format = TextureData_FindMatchingPixelFormat(data, &pixels->format, prefs, formats);
                if (format == nullptr) {
                    replace(0x80000004);
                }
            }
            uint32_t w = pixels->widths[0];
            uint32_t h = pixels->heights[0];
            uint32_t aspect = At<uint32_t>(renderer, 0x110);
            if (w > At<uint32_t>(renderer, 0x100) || h > At<uint32_t>(renderer, 0x104) ||
                (aspect != 0 && (w / h > aspect || h / w > aspect))) {
                replace(0x80000006);
            }
        }
    } else {
        const uint8_t* source = persistent + 0x14;
        uint32_t width = At<const uint32_t*>(persistent, 0x5C)[0];
        uint32_t height = At<const uint32_t*>(persistent, 0x60)[0];
        bool searched = false;
        if ((!PowerOfTwo(width) || !PowerOfTwo(height)) && (RendererFlags(renderer) & 8) == 0) {
            if ((RendererFlags(renderer) & 4) == 0) {
                replace(0x80000006);
                searched = true;
            }
        }
        if (!searched) {
            format = TextureData_FindMatchingPixelFormat(data, source, prefs, formats);
            if (format == nullptr) {
                replace(0x80000004);
            }
        }
        d3dFormat = PixelFormat_GetD3DFormat(source);
        if (d3dFormat == 0) {
            replace(0x80000004);
        }
        uint8_t* global = Renderer();   // (the checks here read NiRenderer::ms_pkRenderer)
        if (Com<long>(ni::dx9::ms_pkD3D9, kCheckDeviceFormat, At<uint32_t>(global, 0x3E0), At<uint32_t>(global, 0x3E4),
                      At<uint32_t>(global, 0x3EC), 0u, 3u, d3dFormat) < 0) {
            replace(0x80000004);
        }
        uint32_t w = At<const uint32_t*>(persistent, 0x5C)[0];
        uint32_t aspect = At<uint32_t>(global, 0x110);
        if (w > At<uint32_t>(global, 0x100)) {
            replace(0x80000006);
        } else {
            uint32_t h = At<const uint32_t*>(persistent, 0x60)[0];
            if (h > At<uint32_t>(global, 0x104) || (aspect != 0 && (w / h > aspect || h / w > aspect))) {
                replace(0x80000006);
            }
        }
    }
    if (At<uint8_t>(data, 0x64) == 0 && persistent != nullptr) {
        crt::Memcpy(data + 0x14, persistent + 0x14, 0x44);
        At<uint32_t>(data, 0xC) = At<const uint32_t*>(persistent, 0x5C)[0];
        At<uint32_t>(data, 0x10) = At<const uint32_t*>(persistent, 0x60)[0];
        uint16_t levels = uint16_t(At<uint32_t>(persistent, 0x6C));
        At<uint16_t>(data, 0x60) = levels;
        At<uint8_t>(data, 0x65) = levels != 1 ? 1 : 0;
        At<uint8_t>(data, 0x64) = 0;
        At<uint8_t*>(data, 0x58) = renderer;
        At<uint32_t>(data, 0x68) = At<const uint32_t*>(persistent, 0x64)[At<uint32_t>(persistent, 0x6C)] *
                                   At<uint32_t>(persistent, 0x74);
        TakePalette(data, At<uint8_t*>(persistent, 0x88));
        if (At<void*>(data, 0x5C) != nullptr ||
            (SourceTextureData_CreateSurfFromRendererData(data, persistent, d3dFormat),
             At<void*>(data, 0x5C) != nullptr)) {
            SourceTextureData_CopyRendererDataToSurface(data, persistent);
        }
    } else {
        uint8_t isStatic = At<uint8_t>(data, 0x64);
        if (isStatic != 0) {
            format = formats[At<uint32_t>(renderer, 0x67C)];   // the replacement data's format
        }
        At<uint32_t>(data, 0x74) = pixels->revisionID;
        crt::Memcpy(data + 0x14, format, 0x44);
        At<uint32_t>(data, 0xC) = pixels->widths[0];
        At<uint32_t>(data, 0x10) = pixels->heights[0];
        uint8_t mipmap = 0;
        if (isStatic == 0) {
            uint32_t want = At<uint32_t>(texture, 0x20);   // the preferences' mipmapping
            if (want == 1 || (want == 2 && ms_bMipmapByDefault != 0)) {
                mipmap = 1;
            }
        }
        At<uint8_t>(data, 0x65) = mipmap;
        NiPixelData* converted;
        if (isStatic != 0) {
            converted = pixels;
        } else {
            converted = Virtual<NiPixelData*>(ImageConverter_Get(), 5, pixels, format, pixels, uint32_t(mipmap));
        }
        if (At<void*>(data, 0x5C) != nullptr ||
            (SourceTextureData_CreateSurf(data, converted), At<void*>(data, 0x5C) != nullptr)) {
            SourceTextureData_CopyDataToSurface(data, converted);
        }
        At<uint16_t>(data, 0x60) = uint16_t(converted->mipmapLevels);
        TakePalette(data, re::PtrCast<uint8_t*>(pixels->palette));
        At<uint32_t>(data, 0x68) = converted->offsets[converted->mipmapLevels] * converted->faces;
        Increment(&converted->refCount);   // (a NiPointer of it, gone)
        ReleaseObject(converted);
    }
    At<void*>(texture, 0x24) = data;
    if (ms_bDestroyAppDataByDefault != 0) {
        Virtual<void>(texture, 21);   // DestroyAppPixelData
    }
    ReleaseObject(held);
    return data;
}

#define C40H_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C40H_REVERSED("NiDX9SourceTextureData::CreateSurf", 0x00895B60, NiDX9SourceTextureData_CreateSurf);
C40H_REVERSED("NiDX9SourceTextureData::CopyDataToSurfaceLevel", 0x00895CD0, NiDX9SourceTextureData_CopyDataToSurfaceLevel);
C40H_REVERSED("NiDX9SourceTextureData::CreateSurfFromRendererData", 0x00895E30,
              NiDX9SourceTextureData_CreateSurfFromRendererData);
C40H_REVERSED("NiDX9SourceTextureData::CopyRendererDataToSurfaceLevel", 0x00895F90,
              NiDX9SourceTextureData_CopyRendererDataToSurfaceLevel);
C40H_REVERSED("NiDX9SourceTextureData::InitializeFromD3DTexture", 0x008960F0,
              NiDX9SourceTextureData_InitializeFromD3DTexture);
C40H_REVERSED("NiDX9SourceTextureData::UpdatePalette", 0x00896340, NiDX9SourceTextureData_UpdatePalette);
C40H_REVERSED("NiDX9SourceTextureData::NiDX9SourceTextureData", 0x00896370, NiDX9SourceTextureData_Construct);
C40H_REVERSED("NiDX9SourceTextureData::~NiDX9SourceTextureData", 0x008963C0, NiDX9SourceTextureData_Destruct);
C40H_REVERSED("NiDX9SourceTextureData::CopyDataToSurface", 0x00896400, NiDX9SourceTextureData_CopyDataToSurface);
C40H_REVERSED("NiDX9SourceTextureData::CopyRendererDataToSurface", 0x008964C0,
              NiDX9SourceTextureData_CopyRendererDataToSurface);
C40H_REVERSED("NiDX9SourceTextureData::Update", 0x00896570, NiDX9SourceTextureData_Update);
C40H_REVERSED("NiDX9SourceTextureData::LoadTextureFile", 0x00896620, NiDX9SourceTextureData_LoadTextureFile);
C40H_REVERSED("NiDX9SourceTextureData::DeletingDestructor", 0x008968A0, NiDX9SourceTextureData_DeletingDestructor);
C40H_REVERSED("NiDX9SourceTextureData::Create", 0x008968F0, NiDX9SourceTextureData_Create);

} // namespace ni::c40
