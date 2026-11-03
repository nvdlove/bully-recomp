// NiDX9SourceCubeMapData (vtable 0x99D078 on NiDX9SourceTextureData's; 0x80 bytes: +0x7C the
// pixel data's revision): a source cube map's D3D cube texture (square power of two faces, the
// renderer's cube formats at +0x5AC; mipmapped only when the device mipmaps cube maps, +0x4F0),
// from its persistent data, straight from the file, or from its pixel data converted; the
// replacement data otherwise. Here Create, CreateCubeMap, the faces' copies, Update (slot 19) and
// the deleting destructor.

#include "gamebryo/NiDX9RendererData.h"
#include "gamebryo/NiPixelData.h"
#include "reversed.h"

namespace ni::c40 {
namespace {

constexpr uintptr_t kSourceCubeMapDataVtable = 0x0099D078;
enum : int {
    kCheckDeviceFormat = 10,
    kCreateCubeTexture = 25,
    kGetCubeMapSurface = 18,
    kRelease = 2,
};

GAME_VAR(uint8_t, ms_bMipmapByDefault, 0x020E2180);

GAME_FN(const char*, __cdecl, NiDX9ErrorString, 0x00899150, long hr);
GAME_FN(uint32_t, __cdecl, PixelFormat_GetD3DFormat, 0x00895340, const void* format);
GAME_FN(void*, __cdecl, ImageConverter_Get, 0x007D3B40);
GAME_FN(NiPixelData*, __cdecl, Texture_GetReplacementData, 0x00755170, void* texture, uint32_t error, int warn);
GAME_FN(void*, __thiscall, SourceTextureData_Construct, 0x00896370, void* self, void* texture, void* renderer);
GAME_FN(void, __thiscall, SourceTextureData_Destruct, 0x008963C0, void* self);
GAME_FN(uint8_t, __thiscall, SourceTextureData_UpdatePalette, 0x00896340, void* self, void* palette);
GAME_FN(uint8_t, __thiscall, SourceTextureData_LoadTextureFile, 0x00896620, void* self, const char* name,
        uint8_t* renderer, const uint32_t* prefs);
GAME_FN(void, __cdecl, SourceTextureData_CopyDataToSurfaceLevel, 0x00895CD0, const NiPixelData* pixels,
        uint32_t level, void* surface, uint32_t face);
GAME_FN(void, __cdecl, SourceTextureData_CopyRendererDataToSurfaceLevel, 0x00895F90, const uint8_t* persistent,
        uint32_t level, void* surface, uint32_t face);
GAME_FN(void*, __thiscall, TextureData_FindMatchingPixelFormat, 0x008929F0, void* self, const void* source,
        const uint32_t* prefs, const re::Ptr32<void>* formats);
// this file's (called where modes apply)
GAME_FN(uint8_t, __thiscall, CubeMapData_CreateCubeMap, 0x00897BB0, void* self);
GAME_FN(void, __thiscall, CubeMapData_CopyDataToSurface, 0x00897C20, void* self, const NiPixelData* pixels,
        uint32_t face);
GAME_FN(void, __thiscall, CubeMapData_CopyRendererDataToSurface, 0x00897CB0, void* self, const uint8_t* persistent,
        uint32_t face);

inline bool PowerOfTwo(uint32_t n) {
    return n != 0 && (n & (n - 1)) == 0;
}
inline void ReleaseObject(NiRefObject* object) {
    if (object != nullptr && Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

// each level of one face copied
template <typename Copy>
void CopyFace(uint8_t* self, uint32_t face, Copy copy) {
    void* cube = At<void*>(self, 0x5C);
    for (uint16_t level = 0; level < At<uint16_t>(self, 0x60); level++) {
        void* surface;
        long hr = Com<long>(cube, kGetCubeMapSurface, face, uint32_t(level), &surface);
        if (hr < 0) {
            ni::dx9::NiOutputDebugString("NiDX9SourceCubeMapData::CopyDataToSurface> Failed GetSurfaceLevel (%d)- %s",
                                         uint32_t(level), NiDX9ErrorString(hr));
            return;
        }
        copy(uint32_t(level), surface);
        Com<long>(surface, kRelease);
    }
}

} // namespace

// 0x897B80 (thiscall): NiDX9SourceCubeMapData's deleting destructor
void* __fastcall NiDX9SourceCubeMapData_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = kSourceCubeMapDataVtable;
    SourceTextureData_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x80);
    }
    return self;
}

// 0x897BB0 (thiscall): NiDX9SourceCubeMapData::CreateCubeMap (managed, of its pixel format's own
// D3D format (+0x20))
uint8_t __fastcall NiDX9SourceCubeMapData_CreateCubeMap(uint8_t* self, re::Edx) {
    void* cube = nullptr;
    void* device = At<void*>(At<uint8_t*>(self, 0x58), 0xA4);
    long hr = Com<long>(device, kCreateCubeTexture, At<uint32_t>(self, 0xC), uint32_t(At<uint16_t>(self, 0x60)), 0u,
                        At<uint32_t>(self, 0x20), 1u, &cube, static_cast<void*>(nullptr));
    if (hr < 0) {
        ni::dx9::NiOutputDebugString("NiDX9SourceCubeMapData::CreateCubeMap> Failed CreateCubeTexture - %s\n",
                                     NiDX9ErrorString(hr));
        return 0;
    }
    At<void*>(self, 0x5C) = cube;
    return 1;
}

// 0x897C20 (thiscall): NiDX9SourceCubeMapData::CopyDataToSurface (one face's levels)
void __fastcall NiDX9SourceCubeMapData_CopyDataToSurface(uint8_t* self, re::Edx, const NiPixelData* pixels,
                                                         uint32_t face) {
    CopyFace(self, face, [&](uint32_t level, void* surface) {
        SourceTextureData_CopyDataToSurfaceLevel(pixels, level, surface, face);
    });
}

// 0x897CB0 (thiscall): NiDX9SourceCubeMapData::CopyRendererDataToSurface
void __fastcall NiDX9SourceCubeMapData_CopyRendererDataToSurface(uint8_t* self, re::Edx, const uint8_t* persistent,
                                                                 uint32_t face) {
    CopyFace(self, face, [&](uint32_t level, void* surface) {
        SourceTextureData_CopyRendererDataToSurfaceLevel(persistent, level, surface, face);
    });
}

// 0x897D40 (cdecl): NiDX9SourceCubeMapData::Create (none when the device has no cube formats.
// Quirks kept: the conversion is asked for the "mipmapped" flag before it's set (always false);
// the size comes from the faces' width twice; the persistent data's pixel format gets the D3D
// format written into it; a pixel data read from the file isn't let go)
void* __cdecl NiDX9SourceCubeMapData_Create(uint8_t* texture, uint8_t* renderer) {
    const re::Ptr32<void>* formats = reinterpret_cast<const re::Ptr32<void>*>(renderer + 0x5AC);
    uint32_t first = 0;
    while (first < 22 && formats[first] == nullptr) {
        first++;
    }
    if (first == 22) {
        ni::dx9::NiOutputDebugString("NiDX9SourceCubeMapData::Create> No Cube Map texture formats supported\n");
        return nullptr;
    }
    void* memory = NiNew(0x80);
    uint8_t* data = nullptr;
    if (memory != nullptr) {
        data = static_cast<uint8_t*>(SourceTextureData_Construct(memory, texture, renderer));
        At<uint32_t>(data, 0) = kSourceCubeMapDataVtable;
        At<uint32_t>(data, 0x7C) = 0;
    }
    uint8_t* persistent = At<uint8_t*>(texture, 0x38);
    const uint32_t* prefs = reinterpret_cast<const uint32_t*>(texture + 0x18);
    NiPixelData* pixels = nullptr;
    auto replace = [&](uint32_t error) {
        pixels = Texture_GetReplacementData(texture, error, 1);
        At<uint8_t>(data, 0x64) = 1;
    };
    if (persistent == nullptr) {
        pixels = At<NiPixelData*>(texture, 0x3C);
        if (pixels == nullptr) {
            const char* name = At<const char*>(texture, 0x34);
            if (At<uint8_t>(texture, 0x41) != 0 && SourceTextureData_LoadTextureFile(data, name, renderer, prefs) != 0) {
                At<void*>(texture, 0x24) = data;
                return data;
            }
            if (name != nullptr) {
                pixels = Virtual<NiPixelData*>(ImageConverter_Get(), 3, name, static_cast<void*>(nullptr));
            } else {
                replace(0x80000008);
            }
        }
    }
    void* format = nullptr;
    if (persistent == nullptr) {
        if (pixels == nullptr) {
            pixels = Texture_GetReplacementData(texture, 0x80000008, 1);
            At<uint8_t>(data, 0x64) = 1;
            ni::dx9::NiOutputDebugString("Failed to find texture %s\n", At<const char*>(texture, 0x34));
        } else {
            uint32_t width = pixels->widths[0];
            if (width == pixels->heights[0] && PowerOfTwo(width)) {
                format = TextureData_FindMatchingPixelFormat(data, &pixels->format, prefs, formats);
                if (format == nullptr || pixels->format.format == 2 || pixels->format.format == 3) {
                    replace(0x80000007);
                }
            } else {
                replace(0x80000009);
            }
            if (pixels->widths[0] > At<uint32_t>(renderer, 0x100) || pixels->heights[0] > At<uint32_t>(renderer, 0x104)) {
                replace(0x80000006);
            }
        }
    } else {
        uint32_t width = At<const uint32_t*>(persistent, 0x5C)[0];
        const uint8_t* source = persistent + 0x14;
        if (width == At<const uint32_t*>(persistent, 0x60)[0] && PowerOfTwo(width)) {
            format = TextureData_FindMatchingPixelFormat(data, source, prefs, formats);
            if (format == nullptr) {
                replace(0x80000007);
            }
        } else {
            replace(0x80000009);
        }
        if (At<const uint32_t*>(persistent, 0x5C)[0] > At<uint32_t>(renderer, 0x100) ||
            At<const uint32_t*>(persistent, 0x60)[0] > At<uint32_t>(renderer, 0x104)) {
            replace(0x80000009);
        }
        if (At<uint8_t>(data, 0x64) == 0) {
            uint32_t d3dFormat = PixelFormat_GetD3DFormat(source);
            uint8_t* global = Renderer();   // (the check reads NiRenderer::ms_pkRenderer)
            if (d3dFormat == 0 ||
                Com<long>(ni::dx9::ms_pkD3D9, kCheckDeviceFormat, At<uint32_t>(global, 0x3E0), At<uint32_t>(global, 0x3E4),
                          At<uint32_t>(global, 0x3EC), 0u, 3u, d3dFormat) < 0) {
                replace(0x80000007);
            } else {
                At<uint32_t>(persistent, 0x14 + 0xC) = d3dFormat;
            }
        }
    }
    if (At<uint8_t>(data, 0x64) == 0 && persistent != nullptr) {
        crt::Memcpy(data + 0x14, persistent + 0x14, 0x44);
        At<uint32_t>(data, 0xC) = At<const uint32_t*>(persistent, 0x5C)[0];
        At<uint32_t>(data, 0x10) = At<const uint32_t*>(persistent, 0x60)[0];
        uint16_t levels = At<uint8_t>(renderer, 0x4F0) != 0 ? uint16_t(At<uint32_t>(persistent, 0x6C)) : 1;
        At<uint16_t>(data, 0x60) = levels;
        At<uint8_t*>(data, 0x58) = renderer;
        At<uint8_t>(data, 0x65) = levels != 1 ? 1 : 0;
        At<uint8_t>(data, 0x64) = 0;
        SourceTextureData_UpdatePalette(data, At<void*>(persistent, 0x88));
        At<uint32_t>(data, 0x68) = At<const uint32_t*>(persistent, 0x64)[At<uint32_t>(persistent, 0x6C)] *
                                   At<uint32_t>(persistent, 0x74);
        if (CubeMapData_CreateCubeMap(data) != 0 && At<void*>(data, 0x5C) != nullptr) {
            for (uint32_t face = 0; face < 6; face++) {
                CubeMapData_CopyRendererDataToSurface(data, persistent, face);
            }
            At<void*>(texture, 0x24) = data;
            return data;
        }
    } else {
        NiPixelData* converted;
        if (At<uint8_t>(data, 0x64) != 0) {
            format = formats[At<uint32_t>(renderer, 0x67C)];
            converted = pixels;
        } else {
            converted = Virtual<NiPixelData*>(ImageConverter_Get(), 5, pixels, format, pixels,
                                              uint32_t(At<uint8_t>(data, 0x65)));
        }
        if (converted == nullptr) {
            converted = Texture_GetReplacementData(texture, 0x80000007, 1);
            format = formats[At<uint32_t>(renderer, 0x67C)];
        }
        At<uint32_t>(data, 0x7C) = pixels->revisionID;
        crt::Memcpy(data + 0x14, format, 0x44);
        uint32_t width = converted->widths[0];
        At<uint32_t>(data, 0x10) = width;
        At<uint32_t>(data, 0xC) = width;
        uint32_t want = prefs[2];   // the preferences' mipmapping
        if ((At<uint8_t>(data, 0x64) == 0 && want == 1) ||
            (want == 2 && ms_bMipmapByDefault != 0 && At<uint8_t>(renderer, 0x4F0) != 0)) {
            At<uint8_t>(data, 0x65) = 1;
        } else {
            At<uint8_t>(data, 0x65) = 0;
        }
        At<uint16_t>(data, 0x60) = At<uint8_t>(renderer, 0x4F0) != 0 ? uint16_t(converted->mipmapLevels) : 1;
        if (CubeMapData_CreateCubeMap(data) != 0 && At<void*>(data, 0x5C) != nullptr) {
            for (uint32_t face = 0; face < 6; face++) {
                CubeMapData_CopyDataToSurface(data, converted, face);
            }
            SourceTextureData_UpdatePalette(data, pixels->palette);
            At<uint32_t>(data, 0x68) = converted->offsets[converted->mipmapLevels] * converted->faces;
            Increment(&converted->refCount);   // (a NiPointer of it, gone)
            ReleaseObject(converted);
            At<void*>(texture, 0x24) = data;
            return data;
        }
    }
    Virtual<void*>(data, 0, 1);   // the deleting destructor
    return nullptr;
}

// 0x898250 (thiscall): NiDX9SourceCubeMapData::Update (unless static: the pixel data converted and
// every face copied again when its revision or palette changed)
void __fastcall NiDX9SourceCubeMapData_Update(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 0x64) != 0) {
        return;
    }
    NiPixelData* pixels = At<NiPixelData*>(At<uint8_t*>(self, 8), 0x3C);
    if (pixels == nullptr) {
        return;
    }
    bool changed = SourceTextureData_UpdatePalette(self, pixels->palette) != 0;
    if (pixels->revisionID == At<uint32_t>(self, 0x7C) && !changed) {
        return;
    }
    NiPixelData* converted = Virtual<NiPixelData*>(ImageConverter_Get(), 5, pixels, self + 0x14, pixels,
                                                   uint32_t(At<uint8_t>(self, 0x65)));
    At<uint32_t>(self, 0x7C) = pixels->revisionID;
    if (At<void*>(self, 0x5C) != nullptr) {
        for (uint32_t face = 0; face < converted->faces; face++) {
            CubeMapData_CopyDataToSurface(self, converted, face);
        }
    }
    if (converted != nullptr) {
        Increment(&converted->refCount);
        ReleaseObject(converted);
    }
}

#define C40J_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C40J_REVERSED("NiDX9SourceCubeMapData::DeletingDestructor", 0x00897B80, NiDX9SourceCubeMapData_DeletingDestructor);
C40J_REVERSED("NiDX9SourceCubeMapData::CreateCubeMap", 0x00897BB0, NiDX9SourceCubeMapData_CreateCubeMap);
C40J_REVERSED("NiDX9SourceCubeMapData::CopyDataToSurface", 0x00897C20, NiDX9SourceCubeMapData_CopyDataToSurface);
C40J_REVERSED("NiDX9SourceCubeMapData::CopyRendererDataToSurface", 0x00897CB0,
              NiDX9SourceCubeMapData_CopyRendererDataToSurface);
C40J_REVERSED("NiDX9SourceCubeMapData::Create", 0x00897D40, NiDX9SourceCubeMapData_Create);
C40J_REVERSED("NiDX9SourceCubeMapData::Update", 0x00898250, NiDX9SourceCubeMapData_Update);

} // namespace ni::c40
