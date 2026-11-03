// The renderer data of rendered and dynamic textures, NiDX9TextureData's kinds (0x64 bytes unless
// said): NiDX9RenderedTextureData (vtable 0x99D530: a render target texture, its buffer data made
// from its top level; flag 2 at +0x62), NiDX9RenderedCubeMapData (0x99D668: six faces'), and
// NiDX9DynamicTextureData (0x99D750, 0x6C: a texture the CPU writes, +0x64 its system memory copy
// when the device can't make dynamic textures, +0x68 locked; flag 4). Each tells the renderer it
// goes on destruction (its slots 102 cube maps, 103 rendered textures, 104 dynamic textures).

#include "gamebryo/NiDX9RendererData.h"
#include "reversed.h"

namespace ni::c40 {
namespace {

constexpr uintptr_t kRenderedTextureDataVtable = 0x0099D530;
constexpr uintptr_t kRenderedCubeMapDataVtable = 0x0099D668;
constexpr uintptr_t kDynamicTextureDataVtable = 0x0099D750;
constexpr uintptr_t IID_IDirect3DTexture9 = 0x009A20E4;
const char* const kEmpty = reinterpret_cast<const char*>(0x008FEFBB);   // ""

enum : int {
    kCreateTexture = 23,
    kCreateCubeTexture = 25,
    kQueryInterface = 0,
    kRelease = 2,
};

GAME_FN(const char*, __cdecl, NiDX9ErrorString, 0x00899150, long hr);
GAME_FN(void*, __cdecl, TextureData_FindClosestPixelFormat, 0x008925F0, const uint32_t* prefs, const re::Ptr32<void>* formats);
GAME_FN(void*, __thiscall, TextureData_Construct, 0x00892960, void* self, void* texture, void* renderer);
GAME_FN(void, __thiscall, TextureData_Destruct, 0x008929B0, void* self);
GAME_FN(void*, __cdecl, TextureBufferData_Create, 0x00894930, void* texture, re::Ptr32<void>* buffer);
GAME_FN(void*, __cdecl, TextureBufferData_CreateCubeFace, 0x00894A50, void* cube, uint32_t face, void* first,
        void* device, re::Ptr32<void>* buffer);
// this file's (called where modes apply)
GAME_FN(void*, __thiscall, RenderedTextureData_Construct, 0x00899680, void* self, void* texture, void* renderer);
GAME_FN(void, __thiscall, RenderedTextureData_Destruct, 0x008996B0, void* self);
GAME_FN(void*, __thiscall, RenderedTextureData_CreateSurf, 0x00899720, void* self, void* texture);
GAME_FN(void*, __thiscall, RenderedCubeMapData_CreateSurf, 0x00899950, void* self, void* texture);
GAME_FN(void*, __thiscall, DynamicTextureData_CreateSurf, 0x00899B50, void* self, void* texture, int dynamic);

inline uint8_t* RendererOf(uint8_t* self) {
    return At<uint8_t*>(self, 0x58);
}
inline void* DeviceOf(uint8_t* self) {
    return At<void*>(RendererOf(self), 0xA4);
}
// the texture's preferences (its +0x18) and the closest format in one of the renderer's tables
inline void* ChooseFormat(uint8_t* self, uint8_t* texture, int table) {
    uint32_t prefs[3] = {At<uint32_t>(texture, 0x18), At<uint32_t>(texture, 0x1C), At<uint32_t>(texture, 0x20)};
    return TextureData_FindClosestPixelFormat(prefs, re::PtrCast<const re::Ptr32<void>*>(RendererOf(self) + table));
}
inline void DeleteData(void* data) {
    if (data != nullptr) {
        Virtual<void*>(data, 0, 1);
    }
}

} // namespace

// 0x899680 (thiscall): NiDX9RenderedTextureData's constructor
void* __fastcall NiDX9RenderedTextureData_Construct(uint8_t* self, re::Edx, void* texture, void* renderer) {
    TextureData_Construct(self, texture, renderer);
    At<uint16_t>(self, 0x62) |= 2;
    At<uint32_t>(self, 0) = kRenderedTextureDataVtable;
    return self;
}

// 0x8996B0 (thiscall): NiDX9RenderedTextureData's destructor (the renderer told: its slot 103)
void __fastcall NiDX9RenderedTextureData_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kRenderedTextureDataVtable;
    Virtual<void>(RendererOf(self), 103, At<void*>(self, 8));
    TextureData_Destruct(self);
}

// 0x8996E0 (thiscall): NiDX9RenderedTextureData's deleting destructor
void* __fastcall NiDX9RenderedTextureData_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = kRenderedTextureDataVtable;
    Virtual<void>(RendererOf(self), 103, At<void*>(self, 8));
    TextureData_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x64);
    }
    return self;
}

// 0x899720 (thiscall): NiDX9RenderedTextureData::CreateSurf (a render target texture of the
// closest render target format (+0x554); the format returned)
void* __fastcall NiDX9RenderedTextureData_CreateSurf(uint8_t* self, re::Edx, uint8_t* texture) {
    if (texture == nullptr) {
        return nullptr;
    }
    At<uint32_t>(self, 0xC) = Virtual<uint32_t>(texture, 18);   // GetWidth
    At<uint32_t>(self, 0x10) = Virtual<uint32_t>(texture, 19);  // GetHeight
    At<uint16_t>(self, 0x60) = 1;
    uint8_t* format = static_cast<uint8_t*>(ChooseFormat(self, texture, 0x554));
    if (format == nullptr) {
        return nullptr;
    }
    crt::Memcpy(self + 0x14, format, 0x44);
    void* d3dTexture = nullptr;
    long hr = Com<long>(DeviceOf(self), kCreateTexture, At<uint32_t>(self, 0xC), At<uint32_t>(self, 0x10), 1u, 1u,
                        At<uint32_t>(format, 0xC), 0u, &d3dTexture, static_cast<void*>(nullptr));
    if (hr < 0) {
        ni::dx9::NiOutputDebugString("NiDX9RenderedTextureData::CreateSurf> Failed CreateImageSurface - %s",
                                     NiDX9ErrorString(hr));
        At<void*>(self, 0x5C) = nullptr;
        return nullptr;
    }
    At<void*>(self, 0x5C) = d3dTexture;
    return format;
}

// 0x899800 (cdecl): NiDX9RenderedTextureData::Create (the texture's top level made its buffer's
// render target data. Quirk kept: the buffer the buffer data makes isn't given to the texture)
void* __cdecl NiDX9RenderedTextureData_Create(uint8_t* texture, void* renderer) {
    uint8_t* data = static_cast<uint8_t*>(NiNew(0x64));
    if (data != nullptr) {
        TextureData_Construct(data, texture, renderer);
        At<uint16_t>(data, 0x62) |= 2;
        At<uint32_t>(data, 0) = kRenderedTextureDataVtable;
    }
    if (RenderedTextureData_CreateSurf(data, texture) == nullptr) {
        DeleteData(data);
        return nullptr;
    }
    At<void*>(At<uint8_t*>(data, 8), 0x24) = data;
    void* d3dBase = At<void*>(data, 0x5C);
    re::Ptr32<void> buffer = Virtual<void*>(texture, 20);   // the texture's buffer
    void* d3dTexture = nullptr;
    long hr = Com<long>(d3dBase, kQueryInterface, reinterpret_cast<const void*>(IID_IDirect3DTexture9), &d3dTexture);
    if (hr < 0) {
        ni::dx9::NiOutputDebugString("NiDX9RenderedTextureData::Create> Failed QueryInterface IID_IDirect3DTexture9 - %s",
                                     NiDX9ErrorString(hr));
        DeleteData(data);
        return nullptr;
    }
    void* bufferData = TextureBufferData_Create(d3dTexture, &buffer);
    Com<long>(d3dTexture, kRelease);
    if (bufferData == nullptr) {
        ni::dx9::NiOutputDebugString("NiDX9RenderedTextureData::Create> Failed NiDX92DBufferData::Create - %s", kEmpty);
        DeleteData(data);
        return nullptr;
    }
    return data;
}

// 0x899910 (thiscall): NiDX9RenderedCubeMapData's deleting destructor (the renderer's slot 102,
// then the rendered texture data's destructor: its slot 103 too)
void* __fastcall NiDX9RenderedCubeMapData_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = kRenderedCubeMapDataVtable;
    Virtual<void>(RendererOf(self), 102, At<void*>(self, 8));
    RenderedTextureData_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x64);
    }
    return self;
}

// 0x899950 (thiscall): NiDX9RenderedCubeMapData::CreateSurf (a render target cube of the closest
// cube render target format (+0x604), its edge the width)
void* __fastcall NiDX9RenderedCubeMapData_CreateSurf(uint8_t* self, re::Edx, uint8_t* texture) {
    if (texture == nullptr) {
        return nullptr;
    }
    uint32_t edge = Virtual<uint32_t>(texture, 18);
    At<uint32_t>(self, 0x10) = edge;
    At<uint32_t>(self, 0xC) = edge;
    At<uint16_t>(self, 0x60) = 1;
    uint8_t* format = static_cast<uint8_t*>(ChooseFormat(self, texture, 0x604));
    if (format == nullptr) {
        return nullptr;
    }
    void* cube = nullptr;
    long hr = Com<long>(DeviceOf(self), kCreateCubeTexture, At<uint32_t>(self, 0xC), 1u, 1u, At<uint32_t>(format, 0xC),
                        0u, &cube, static_cast<void*>(nullptr));
    if (hr < 0) {
        ni::dx9::NiOutputDebugString("NiDX9RenderedCubeMapData::CreateSurf> Failed CreateImageSurface - %s",
                                     NiDX9ErrorString(hr));
        At<void*>(self, 0x5C) = nullptr;
        return nullptr;
    }
    At<void*>(self, 0x5C) = cube;
    return format;
}

// 0x899A20 (cdecl): NiDX9RenderedCubeMapData::Create (each face's buffer (the texture's +0x38..)
// made a render target of its face)
void* __cdecl NiDX9RenderedCubeMapData_Create(uint8_t* texture, uint8_t* renderer) {
    uint8_t* data = static_cast<uint8_t*>(NiNew(0x64));
    if (data != nullptr) {
        RenderedTextureData_Construct(data, texture, renderer);
        At<uint32_t>(data, 0) = kRenderedCubeMapDataVtable;
    }
    if (RenderedCubeMapData_CreateSurf(data, texture) == nullptr) {
        DeleteData(data);
        return nullptr;
    }
    At<void*>(At<uint8_t*>(data, 8), 0x24) = data;
    void* first = nullptr;
    for (uint32_t face = 0; face < 6; face++) {
        re::Ptr32<void> buffer = At<void*>(texture, 0x38 + int(face) * 4);
        void* made = TextureBufferData_CreateCubeFace(At<void*>(data, 0x5C), face, first, At<void*>(renderer, 0xA4),
                                                      &buffer);
        if (made == nullptr) {
            ni::dx9::NiOutputDebugString("NiDX9RenderedCubeMapData::Create> NiDX9TextureBufferData::CreateCubeFace - %s",
                                         kEmpty);
            Virtual<void*>(data, 0, 1);
            return nullptr;
        }
        if (first == nullptr) {
            first = made;
        }
    }
    return data;
}

// 0x899AF0 (thiscall, 4 bytes: under a hook's size): NiDX9DynamicTextureData::GetSysMemTexture
void* __fastcall NiDX9DynamicTextureData_GetSysMemTexture(uint8_t* self, re::Edx) {
    return At<void*>(self, 0x64);
}
RE_UNHOOKED_FN("NiDX9DynamicTextureData::GetSysMemTexture", 0x00899AF0, &NiDX9DynamicTextureData_GetSysMemTexture);

// 0x899B00 (thiscall, 4 bytes: under a hook's size): NiDX9DynamicTextureData::IsLocked
uint8_t __fastcall NiDX9DynamicTextureData_IsLocked(uint8_t* self, re::Edx) {
    return At<uint8_t>(self, 0x68);
}
RE_UNHOOKED_FN("NiDX9DynamicTextureData::IsLocked", 0x00899B00, &NiDX9DynamicTextureData_IsLocked);

// 0x899B10 (thiscall): NiDX9DynamicTextureData's deleting destructor (the renderer's slot 104)
void* __fastcall NiDX9DynamicTextureData_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = kDynamicTextureDataVtable;
    Virtual<void>(RendererOf(self), 104, At<void*>(self, 8));
    TextureData_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x6C);
    }
    return self;
}

// 0x899B50 (thiscall): NiDX9DynamicTextureData::CreateSurf (a dynamic texture when the device makes
// them, else a default pool one and a system memory copy to fill; the format returned)
void* __fastcall NiDX9DynamicTextureData_CreateSurf(uint8_t* self, re::Edx, uint8_t* texture, int dynamic) {
    if (texture == nullptr) {
        return nullptr;
    }
    At<uint32_t>(self, 0xC) = Virtual<uint32_t>(texture, 18);
    At<uint32_t>(self, 0x10) = Virtual<uint32_t>(texture, 19);
    At<uint16_t>(self, 0x60) = 1;
    uint8_t* format = static_cast<uint8_t*>(ChooseFormat(self, texture, 0x4FC));
    if (format == nullptr) {
        return nullptr;
    }
    crt::Memcpy(self + 0x14, format, 0x44);
    uint32_t d3dFormat = At<uint32_t>(format, 0xC);
    void* device = DeviceOf(self);
    void* d3dTexture = nullptr;
    long hr;
    if (uint8_t(dynamic) != 0) {
        hr = Com<long>(device, kCreateTexture, At<uint32_t>(self, 0xC), At<uint32_t>(self, 0x10), 1u, 0x200u, d3dFormat,
                       0u, &d3dTexture, static_cast<void*>(nullptr));
        if (hr >= 0) {
            At<void*>(self, 0x5C) = d3dTexture;
            return format;
        }
    } else {
        hr = Com<long>(device, kCreateTexture, At<uint32_t>(self, 0xC), At<uint32_t>(self, 0x10), 1u, 0u, d3dFormat, 0u,
                       &d3dTexture, static_cast<void*>(nullptr));
        if (hr >= 0) {
            At<void*>(self, 0x5C) = d3dTexture;
            d3dTexture = nullptr;
            hr = Com<long>(device, kCreateTexture, At<uint32_t>(self, 0xC), At<uint32_t>(self, 0x10), 1u, 0u, d3dFormat,
                           2u, &d3dTexture, static_cast<void*>(nullptr));
            if (hr < 0) {
                ni::dx9::NiOutputDebugString("NiDX9DynamicTextureData::CreateSurf> Failed CreateImageSurface - %s",
                                             NiDX9ErrorString(hr));
                At<void*>(self, 0x64) = nullptr;
                return nullptr;
            }
            At<void*>(self, 0x64) = d3dTexture;
            return format;
        }
    }
    ni::dx9::NiOutputDebugString("NiDX9DynamicTextureData::CreateSurf> Failed CreateImageSurface - %s",
                                 NiDX9ErrorString(hr));
    At<void*>(self, 0x5C) = nullptr;
    return nullptr;
}

// 0x899CE0 (cdecl): NiDX9DynamicTextureData::Create (none for a tiled texture; dynamic when the
// renderer can make dynamic textures (+0x4F1))
void* __cdecl NiDX9DynamicTextureData_Create(uint8_t* texture) {
    if (At<uint8_t>(texture, 0x38) != 0) {
        ni::dx9::NiOutputDebugString("NiDX9DynamicTextureData Failed> Can not create a tiled dynamic texture.\n");
        return nullptr;
    }
    uint8_t* renderer = Renderer();
    uint8_t* data = static_cast<uint8_t*>(NiNew(0x6C));
    if (data != nullptr) {
        TextureData_Construct(data, texture, renderer);
        At<uint16_t>(data, 0x62) |= 4;
        At<uint32_t>(data, 0) = kDynamicTextureDataVtable;
        At<void*>(data, 0x64) = nullptr;
        At<uint8_t>(data, 0x68) = 0;
    }
    if (DynamicTextureData_CreateSurf(data, texture, At<uint8_t>(renderer, 0x4F1)) == nullptr) {
        DeleteData(data);
        return nullptr;
    }
    At<void*>(At<uint8_t*>(data, 8), 0x24) = data;
    return data;
}

#define C40M_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C40M_REVERSED("NiDX9RenderedTextureData::NiDX9RenderedTextureData", 0x00899680, NiDX9RenderedTextureData_Construct);
C40M_REVERSED("NiDX9RenderedTextureData::~NiDX9RenderedTextureData", 0x008996B0, NiDX9RenderedTextureData_Destruct);
C40M_REVERSED("NiDX9RenderedTextureData::DeletingDestructor", 0x008996E0, NiDX9RenderedTextureData_DeletingDestructor);
C40M_REVERSED("NiDX9RenderedTextureData::CreateSurf", 0x00899720, NiDX9RenderedTextureData_CreateSurf);
C40M_REVERSED("NiDX9RenderedTextureData::Create", 0x00899800, NiDX9RenderedTextureData_Create);
C40M_REVERSED("NiDX9RenderedCubeMapData::DeletingDestructor", 0x00899910, NiDX9RenderedCubeMapData_DeletingDestructor);
C40M_REVERSED("NiDX9RenderedCubeMapData::CreateSurf", 0x00899950, NiDX9RenderedCubeMapData_CreateSurf);
C40M_REVERSED("NiDX9RenderedCubeMapData::Create", 0x00899A20, NiDX9RenderedCubeMapData_Create);
C40M_REVERSED("NiDX9DynamicTextureData::DeletingDestructor", 0x00899B10, NiDX9DynamicTextureData_DeletingDestructor);
C40M_REVERSED("NiDX9DynamicTextureData::CreateSurf", 0x00899B50, NiDX9DynamicTextureData_CreateSurf);
C40M_REVERSED("NiDX9DynamicTextureData::Create", 0x00899CE0, NiDX9DynamicTextureData_Create);

} // namespace ni::c40
