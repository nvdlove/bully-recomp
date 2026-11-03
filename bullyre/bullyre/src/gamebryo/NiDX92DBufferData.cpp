// NiDX92DBufferData and its kinds: the DX9 renderer data of an Ni2DBuffer (a render target or a
// depth / stencil buffer). NiDX92DBufferData (vtable 0x99C86C, NiRTTI 0x20E70A4; 0x18 bytes): +0x8
// the buffer, +0xC its NiPixelFormat (NiNew'd, 0x44), +0x10 the multisampling (samples << 8 |
// quality; 0x100 at first), +0x14 the D3D surface. Kinds: NiDX9TextureBufferData (0x99C908,
// 0x1C: +0x18 the texture it's a level of), NiDX9DepthStencilBufferData (0x99C970, 0x18),
// NiDX9AdditionalDepthStencilBufferData (0x99C998, 0x1C: +0x18 the D3D format; all of them in a
// list under a lock, made again after a device reset), NiDX9ImplicitDepthStencilBufferData
// (0x99CA58: the device's own), NiDX9ImplicitBufferData (0x99C9FC, 0x54: the back buffer; +0x18 the
// D3DPRESENT_PARAMETERS, +0x50 the device).
//
// Slots: 0 the deleting destructor, 2 GetRTTI, 3 CanDisplayFrame, 4 DisplayFrame, 5 Shutdown, 6
// Recreate(device), 7 SetRenderTarget(device, index), 8 SetDepthTarget(device), 9
// ReleaseReferences. The statics: the render targets set now (4, 0x20E7088), the depth stencil
// set now (0x20E7098), the renderer's maximum render targets and independent bit depths.

#include "gamebryo/NiDX9RendererData.h"
#include "reversed.h"
#include "runflags.h"

#if defined(_M_X64)
#include <windows.h>
#endif

namespace ni::c40 {
namespace {

using ni::sys::NiCriticalSection;

constexpr uintptr_t kRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t k2DBufferDataVtable = 0x0099C86C;
constexpr uintptr_t kTextureBufferDataVtable = 0x0099C908;
constexpr uintptr_t kDepthStencilBufferDataVtable = 0x0099C970;
constexpr uintptr_t kAdditionalDepthStencilVtable = 0x0099C998;
constexpr uintptr_t kImplicitBufferDataVtable = 0x0099C9FC;
constexpr uintptr_t kImplicitDepthStencilVtable = 0x0099CA58;

GAME_VAR(uint32_t, ms_uiObjects, 0x020E2168);   // NiRefObject's count
GAME_ARRAY(void*, ms_apkCurrentRenderTargets, 0x020E7088);   // [4]
GAME_VAR(void*, ms_pkCurrentDepthStencil, 0x020E7098);
GAME_VAR(uint32_t, ms_uiMaxRenderTargets, 0x020E709C);
GAME_VAR(uint8_t, ms_bIndependentBitDepths, 0x020E70A0);
GAME_VAR(NiCriticalSection, ms_kAdditionalLock, 0x020E7210);
// the additional depth stencil buffers (NiTPointerList: its allocator's vtable at 0xBCB930)
constexpr uintptr_t kAdditionalList = 0x00BCB930;
GAME_VAR(re::Ptr32<void>*, ms_pkAdditionalHead, 0x00BCB934);
GAME_VAR(re::Ptr32<void>*, ms_pkAdditionalTail, 0x00BCB938);
GAME_VAR(uint32_t, ms_uiAdditionalCount, 0x00BCB93C);
GAME_VAR(uint8_t, NiRTTI_NiDX92DBufferData, 0x020E70A4);
GAME_VAR(uint8_t, NiRTTI_NiDX9TextureBufferData, 0x020E70D8);
GAME_VAR(uint8_t, NiRTTI_NiDX9DepthStencilBufferData, 0x020E7140);
GAME_VAR(uint8_t, NiRTTI_NiDX9ImplicitDepthStencilBufferData, 0x020E7174);
GAME_VAR(uint8_t, NiRTTI_NiDX9AdditionalDepthStencilBufferData, 0x020E71DC);
GAME_VAR(uint8_t, NiRTTI_NiDX9ImplicitBufferData, 0x020E7264);

// IDirect3DDevice9 / IDirect3D9 / surface slots
enum : int {
    kRelease = 2,
    kAddRef = 1,
    kPresent = 17,
    kGetSurfaceLevel = 18,   // (IDirect3DTexture9; IDirect3DCubeTexture9's GetCubeMapSurface)
    kCreateDepthStencilSurface = 29,
    kSetRenderTarget = 37,
    kGetRenderTarget = 38,
    kSetDepthStencilSurface = 39,
    kGetDepthStencilSurface = 40,
    kSurfGetDesc = 12,
    kCheckDeviceFormat = 10,
};

GAME_FN(const char*, __cdecl, NiDX9ErrorString, 0x00899150, long hr);
GAME_FN(const char*, __cdecl, GetD3DFormatString, 0x00890530, uint32_t format);
GAME_FN(uint32_t, __cdecl, PixelFormat_GetD3DFormat, 0x00895340, const void* format);
GAME_FN(void*, __cdecl, PixelFormat_CreateFromD3DFormat, 0x00895B20, uint32_t format);
GAME_FN(uint8_t, __thiscall, PixelFormat_HasComponent, 0x007DD390, const void* self, uint32_t component);
GAME_FN(void*, __cdecl, Ni2DBuffer_Create, 0x007D1EA0, uint32_t width, uint32_t height, void* data);
GAME_FN(void*, __cdecl, NiDepthStencilBuffer_Create, 0x007ABFA0, uint32_t width, uint32_t height, void* data);
GAME_FN(void, __thiscall, Ni2DBuffer_SetRendererData, 0x007ABEF0, void* self, void* data);
GAME_FN(void, __thiscall, Ni2DBuffer_ResetDimensions, 0x007D1CF0, void* self, uint32_t width, uint32_t height);
GAME_FN(void, __thiscall, PointerList_Remove, 0x0075F630, void* list, const re::Ptr32<void>* element);
// this file's (called where modes apply)
GAME_FN(uint8_t, __cdecl, ClearDepthTarget, 0x00894310, void* device);
GAME_FN(void, __cdecl, GetD3DMultisampleType, 0x00894390, uint32_t word, uint32_t* type, uint32_t* quality);
GAME_FN(uint32_t, __cdecl, GetMultisampleWord, 0x008943E0, int type, int quality);
GAME_FN(void, __thiscall, TextureBufferData_Destruct, 0x00894890, void* self);
GAME_FN(void, __thiscall, DepthStencilBufferData_Destruct, 0x00894BC0, void* self);
GAME_FN(void*, __thiscall, AdditionalDepthStencil_Construct, 0x00894CA0, void* self);
GAME_FN(void, __thiscall, ImplicitBufferData_Destruct, 0x00894EE0, void* self);
GAME_FN(void, __thiscall, AdditionalDepthStencil_Destruct, 0x00895020, void* self);

// D3DSURFACE_DESC
struct SurfaceDesc {
    uint32_t format;
    uint32_t type;
    uint32_t usage;
    uint32_t pool;
    int32_t multiSampleType;
    uint32_t multiSampleQuality;
    uint32_t width;
    uint32_t height;
};

inline re::Ptr32<void>& Surface(uint8_t* self) {
    return At<void*>(self, 0x14);
}
inline void ReleaseCom(re::Ptr32<void>& slot) {
    if (slot != nullptr) {
        Com<long>(slot, kRelease);
        slot = nullptr;
    }
}
// the NiDX92DBufferData part of a destructor: the surface, the pixel format, then NiRefObject's
inline void DestructBase(uint8_t* self) {
    At<uint32_t>(self, 0) = k2DBufferDataVtable;
    ReleaseCom(Surface(self));
    NiDelete(At<void*>(self, 0xC), 0x44);
    At<void*>(self, 0xC) = nullptr;
    At<uint32_t>(self, 0) = kRefObjectVtable;
    Decrement(&ms_uiObjects);
}
// the constructors' common part, inlined
inline void ConstructBase(uint8_t* self, uintptr_t vtable) {
    At<uint32_t>(self, 0) = kRefObjectVtable;
    At<uint32_t>(self, 4) = 0;
    Increment(&ms_uiObjects);
    At<void*>(self, 8) = nullptr;
    At<void*>(self, 0xC) = nullptr;
    At<uint16_t>(self, 0x10) = 0x100;
    At<void*>(self, 0x14) = nullptr;
    At<uint32_t>(self, 0) = vtable;
}
// the multisampling word from a surface's description (one sample 0xFF: "none")
inline uint16_t MultisampleWord(const SurfaceDesc& desc) {
    int samples = desc.multiSampleType;
    if (samples == 1) {
        samples = 0xFF;
    } else if (samples == 0 || samples <= 1) {
        samples = 1;
    }
    return uint16_t(((samples & 0xFF) << 8) | (desc.multiSampleQuality & 0xFF));
}
// the buffer of the data made, or given the data
inline void AttachBuffer(uint8_t* data, re::Ptr32<void>* buffer, const SurfaceDesc& desc, bool depth) {
    if (*buffer == nullptr) {
        *buffer = depth ? NiDepthStencilBuffer_Create(desc.width, desc.height, data)
                        : Ni2DBuffer_Create(desc.width, desc.height, data);
    } else {
        Ni2DBuffer_SetRendererData(*buffer, data);
    }
    At<void*>(data, 8) = *buffer;
}

} // namespace

// 0x8942C0 (cdecl): NiDX92DBufferData::ClearRenderTarget (a render target past the first unset)
void __cdecl NiDX92DBufferData_ClearRenderTarget(void* device, uint32_t index) {
    if (index != 0 && ms_apkCurrentRenderTargets[index] != nullptr) {
        Com<long>(device, kSetRenderTarget, index, static_cast<void*>(nullptr));
        ms_apkCurrentRenderTargets[index] = nullptr;
    }
}

// 0x894300 (thiscall): NiDX92DBufferData::Shutdown (ReleaseReferences)
uint8_t __fastcall NiDX92DBufferData_Shutdown(uint8_t* self, re::Edx) {
    Virtual<void>(self, 9);
    return 1;
}

// 0x894310 (cdecl): NiDX9DepthStencilBufferData::ClearDepthTarget (the device's depth stencil unset)
uint8_t __cdecl NiDX9DepthStencilBufferData_ClearDepthTarget(void* device) {
    if (ms_pkCurrentDepthStencil != nullptr) {
        if (Com<long>(device, kSetDepthStencilSurface, static_cast<void*>(nullptr)) < 0) {
            return 0;
        }
        ms_pkCurrentDepthStencil = nullptr;
    }
    return 1;
}

#if defined(_M_X64)
// bullyre_x64's frame picture: the back buffer (resolved first when multisampled) saved as a PNG
// through D3DX
void SaveBackBuffer(void* device, const char* path) {
    using SaveFn = long(__stdcall*)(const char*, int, void*, const void*, const RECT*);
    static SaveFn save = reinterpret_cast<SaveFn>(GetProcAddress(GetModuleHandleA("d3dx9_38.dll"), "D3DXSaveSurfaceToFileA"));
    if (save == nullptr) {
        return;
    }
    void* back = nullptr;
    if (Com<long>(device, 18, 0u, 0u, 0u, &back) < 0 || back == nullptr) {   // GetBackBuffer
        return;
    }
    struct {
        uint32_t format, type, usage, pool, multiSample, quality, width, height;
    } desc{};
    Com<long>(back, 12, static_cast<void*>(&desc));   // IDirect3DSurface9::GetDesc
    void* plain = back;
    if (desc.multiSample != 0) {
        void* resolved = nullptr;
        // CreateRenderTarget (no multisampling), StretchRect into it
        if (Com<long>(device, 28, desc.width, desc.height, desc.format, 0u, 0u, 0, &resolved, static_cast<void*>(nullptr)) >= 0 &&
            resolved != nullptr) {
            Com<long>(device, 34, back, static_cast<void*>(nullptr), resolved, static_cast<void*>(nullptr), 0u);
            plain = resolved;
        }
    }
    save(path, 3, plain, nullptr, nullptr);   // D3DXIFF_PNG
    if (plain != back) {
        Com<unsigned long>(plain, kRelease);
    }
    Com<unsigned long>(back, kRelease);
}
#endif

// 0x894340 (thiscall): NiDX9ImplicitBufferData::DisplayFrame (Present)
uint8_t __fastcall NiDX9ImplicitBufferData_DisplayFrame(uint8_t* self, re::Edx) {
#if defined(_M_X64)
    if (re::TakeFrameGrab()) {
        SaveBackBuffer(At<void*>(self, 0x50), re::FrameGrabPath());
    }
#endif
    long hr = Com<long>(At<void*>(self, 0x50), kPresent, static_cast<void*>(nullptr), static_cast<void*>(nullptr),
                        static_cast<void*>(nullptr), static_cast<void*>(nullptr));
    return hr >= 0 ? 1 : 0;
}

// 0x894360 (cdecl): the multisampling word for a sample count and quality (at least one sample;
// returned in ax, the count's high half above it as the original leaves it)
uint32_t __cdecl NiDX92DBufferData_MakeMultisampleWord(int samples, int quality) {
    if (samples <= 1) {
        samples = 1;
    }
    return (uint32_t(samples) & 0xFFFF0000) | ((uint32_t(samples) & 0xFF) << 8) | (uint32_t(quality) & 0xFF);
}

// 0x894380 (thiscall): NiDX92DBufferData::GetRTTI
void* __fastcall NiDX92DBufferData_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiDX92DBufferData;
}

// 0x894390 (cdecl): NiDX92DBufferData::GetD3DMultisampleType (the word's sample count as a
// D3DMULTISAMPLE_TYPE (one: none; over 16: the nonmaskable kind) and its quality)
void __cdecl NiDX92DBufferData_GetD3DMultisampleType(uint32_t word, uint32_t* type, uint32_t* quality) {
    uint32_t samples = (word & 0xFFFF) >> 8;
    *quality = word & 0xFF;
    if (int(samples) <= 1) {
        samples = 1;
    }
    *quality = word & 0xFF;
    if (samples == 1) {
        *type = 0;
        return;
    }
    if (samples > 16) {
        *type = 1;
        return;
    }
    *type = samples;
}

// 0x8943E0 (cdecl): the multisampling word from a D3D type and quality (D3DMULTISAMPLE_NONMASKABLE
// 0xFF; none one sample)
uint32_t __cdecl NiDX92DBufferData_GetMultisampleWord(int type, int quality) {
    if (type == 1) {
        return 0xFF00u | (uint32_t(quality) & 0xFF);
    }
    if (type == 0 || type <= 1) {
        type = 1;
    }
    return (uint32_t(type) & 0xFFFF0000) | ((uint32_t(type) & 0xFF) << 8) | (uint32_t(quality) & 0xFF);
}

// 0x894420 (thiscall): NiDX92DBufferData::SetRenderTarget (unless it's the one set already)
uint8_t __fastcall NiDX92DBufferData_SetRenderTarget(uint8_t* self, re::Edx, void* device, uint32_t index) {
    void* surface = Surface(self);
    if (surface == nullptr || index > ms_uiMaxRenderTargets) {
        return 0;
    }
    if (surface != ms_apkCurrentRenderTargets[index]) {
        if (Com<long>(device, kSetRenderTarget, index, surface) < 0) {
            return 0;
        }
        ms_apkCurrentRenderTargets[index] = Surface(self);
    }
    return 1;
}

// 0x894480 (thiscall): NiDX9DepthStencilBufferData::SetDepthTarget
uint8_t __fastcall NiDX9DepthStencilBufferData_SetDepthTarget(uint8_t* self, re::Edx, void* device) {
    void* surface = Surface(self);
    if (surface == nullptr) {
        return 0;
    }
    if (surface != ms_pkCurrentDepthStencil) {
        if (Com<long>(device, kSetDepthStencilSurface, surface) < 0) {
            return 0;
        }
        ms_pkCurrentDepthStencil = Surface(self);
    }
    return 1;
}

// 0x8944C0 (thiscall): NiDX9DepthStencilBufferData::HasValidDepthBuffer (a depth component)
uint8_t __fastcall NiDX9DepthStencilBufferData_HasValidDepthBuffer(uint8_t* self, re::Edx) {
    if (At<void*>(self, 0xC) != nullptr && Surface(self) != nullptr) {
        return PixelFormat_HasComponent(At<void*>(self, 0xC), 0x11) != 0 ? 1 : 0;
    }
    return 0;
}

// 0x8944E0 (thiscall): NiDX9DepthStencilBufferData::HasValidStencilBuffer (a stencil component)
uint8_t __fastcall NiDX9DepthStencilBufferData_HasValidStencilBuffer(uint8_t* self, re::Edx) {
    if (At<void*>(self, 0xC) != nullptr && Surface(self) != nullptr) {
        return PixelFormat_HasComponent(At<void*>(self, 0xC), 0x12) != 0 ? 1 : 0;
    }
    return 0;
}

// 0x894500 (thiscall): NiDX9ImplicitDepthStencilBufferData::Recreate (the device's depth stencil
// surface again; the buffer's size from it)
uint8_t __fastcall NiDX9ImplicitDepthStencilBufferData_Recreate(uint8_t* self, re::Edx, void* device) {
    if (Surface(self) != nullptr) {
        Virtual<uint8_t>(self, 5);
    }
    if (Com<long>(device, kGetDepthStencilSurface, &Surface(self)) < 0) {
        return 0;
    }
    SurfaceDesc desc;
    if (Com<long>(Surface(self), kSurfGetDesc, &desc) < 0) {
        ReleaseCom(Surface(self));
        return 0;
    }
    At<void*>(self, 0xC) = PixelFormat_CreateFromD3DFormat(desc.format);
    Ni2DBuffer_ResetDimensions(At<void*>(self, 8), desc.width, desc.height);
    return 1;
}

// 0x894590 (cdecl): NiDX9AdditionalDepthStencilBufferData::ShutdownAll (before a reset)
void __cdecl NiDX9AdditionalDepthStencilBufferData_ShutdownAll() {
    ni::sys::Lock(ms_kAdditionalLock);
    for (re::Ptr32<void>* node = ms_pkAdditionalHead; node != nullptr;) {
        void* data = node[2];
        node = static_cast<re::Ptr32<void>*>(node[0]);
        Virtual<uint8_t>(data, 5);
    }
    ni::sys::Unlock(ms_kAdditionalLock);
}

// 0x8945F0 (cdecl): NiDX9AdditionalDepthStencilBufferData::RecreateAll (after a reset)
void __cdecl NiDX9AdditionalDepthStencilBufferData_RecreateAll(void* device) {
    ni::sys::Lock(ms_kAdditionalLock);
    for (re::Ptr32<void>* node = ms_pkAdditionalHead; node != nullptr;) {
        void* data = node[2];
        node = static_cast<re::Ptr32<void>*>(node[0]);
        Virtual<uint8_t>(data, 6, device);
    }
    ni::sys::Unlock(ms_kAdditionalLock);
}

// 0x894660 (thiscall): NiDX9AdditionalDepthStencilBufferData::Recreate (its surface made again of
// its format, the buffer's size and its multisampling; the renderer's slot 29 is called and its
// result dropped, as the original does)
uint8_t __fastcall NiDX9AdditionalDepthStencilBufferData_Recreate(uint8_t* self, re::Edx, void* device) {
    if (Surface(self) != nullptr) {
        Virtual<uint8_t>(self, 5);
    }
    if (At<uint32_t>(self, 0x18) == 0) {
        return 0;
    }
    uint8_t* renderer = Renderer();
    Virtual<void*>(renderer, 29);
    uint32_t format = At<uint32_t>(self, 0x18);
    long hr = Com<long>(ni::dx9::ms_pkD3D9, kCheckDeviceFormat, At<uint32_t>(renderer, 0x3E0),
                        At<uint32_t>(renderer, 0x3E4), At<uint32_t>(renderer, 0x3EC), 2u, 1u, format);
    if (hr >= 0) {
        uint32_t type = 0;
        uint32_t quality = 0;
        GetD3DMultisampleType(At<uint16_t>(self, 0x10), &type, &quality);
        uint8_t* buffer = At<uint8_t*>(self, 8);
        hr = Com<long>(device, kCreateDepthStencilSurface, At<uint32_t>(buffer, 8), At<uint32_t>(buffer, 0xC),
                       At<uint32_t>(self, 0x18), type, quality, 0, &Surface(self), static_cast<void*>(nullptr));
        if (hr >= 0) {
            SurfaceDesc desc;
            if (Com<long>(Surface(self), kSurfGetDesc, &desc) < 0) {
                ReleaseCom(Surface(self));
                return 0;
            }
            At<void*>(self, 0xC) = PixelFormat_CreateFromD3DFormat(desc.format);
            return 1;
        }
    }
    ni::dx9::NiOutputDebugString("NiDX9AdditionalDepthStencilBufferData::Recreate> FAILED %s", NiDX9ErrorString(hr));
    return 0;
}
// (0x894682 is the middle of 0x894660, listed apart: nothing calls it)
RE_UNHOOKED("NiDX9AdditionalDepthStencilBufferData::Recreate_894682", 0x00894682);

// 0x894780 (thiscall): NiDX9ImplicitBufferData::Recreate (the current targets forgotten; the
// device held again; its back buffer again; the buffer's size from it)
uint8_t __fastcall NiDX9ImplicitBufferData_Recreate(uint8_t* self, re::Edx, void* device) {
    if (Surface(self) != nullptr) {
        Virtual<uint8_t>(self, 5);
    }
    ms_apkCurrentRenderTargets[0] = nullptr;
    ms_pkCurrentDepthStencil = nullptr;
    re::Ptr32<void>& held = At<void*>(self, 0x50);
    ReleaseCom(held);
    held = device;
    Com<long>(device, kAddRef);
    if (Com<long>(held, kGetRenderTarget, 0u, &Surface(self)) < 0) {
        return 0;
    }
    SurfaceDesc desc;
    if (Com<long>(Surface(self), kSurfGetDesc, &desc) < 0) {
        ReleaseCom(Surface(self));
        return 0;
    }
    Ni2DBuffer_ResetDimensions(At<void*>(self, 8), desc.width, desc.height);
    At<void*>(self, 0xC) = PixelFormat_CreateFromD3DFormat(desc.format);
    ni::dx9::NiOutputDebugString("NiDX9Renderer::Recreate> Using %s backbuffer format\n", GetD3DFormatString(desc.format));
    return 1;
}

// 0x894860 (thiscall): NiDX92DBufferData::ReleaseReferences (the surface, the pixel format)
void __fastcall NiDX92DBufferData_ReleaseReferences(uint8_t* self, re::Edx) {
    ReleaseCom(Surface(self));
    NiDelete(At<void*>(self, 0xC), 0x44);
    At<void*>(self, 0xC) = nullptr;
}

// 0x894890 (thiscall): NiDX9TextureBufferData's destructor (the texture let go)
void __fastcall NiDX9TextureBufferData_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kTextureBufferDataVtable;
    ReleaseCom(At<void*>(self, 0x18));
    DestructBase(self);
}

// 0x894900 (thiscall): NiDX9TextureBufferData::GetRTTI
void* __fastcall NiDX9TextureBufferData_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiDX9TextureBufferData;
}

// 0x894910 (thiscall): NiDX9TextureBufferData's deleting destructor
void* __fastcall NiDX9TextureBufferData_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    TextureBufferData_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x1C);
    }
    return self;
}

// 0x894930 (cdecl): NiDX9TextureBufferData::Create (a texture's top level as a render target: the
// buffer made, or given the data)
void* __cdecl NiDX9TextureBufferData_Create(void* texture, re::Ptr32<void>* buffer) {
    uint8_t* data = static_cast<uint8_t*>(NiNew(0x1C));
    if (data != nullptr) {
        ConstructBase(data, kTextureBufferDataVtable);
        At<void*>(data, 0x18) = nullptr;
    }
    At<void*>(data, 0x18) = texture;
    Com<long>(texture, kAddRef);
    long hr = Com<long>(texture, kGetSurfaceLevel, 0u, &Surface(data));
    SurfaceDesc desc;
    if (hr < 0) {
        ni::dx9::NiOutputDebugString("NiDX9TextureBufferData::Create> FAILED- GetSurfaceLevel - %s", NiDX9ErrorString(hr));
    } else if (Com<long>(Surface(data), kSurfGetDesc, &desc) >= 0) {
        At<void*>(data, 0xC) = PixelFormat_CreateFromD3DFormat(desc.format);
        AttachBuffer(data, buffer, desc, false);
        At<uint16_t>(data, 0x10) = MultisampleWord(desc);
        return data;
    }
    Virtual<void*>(data, 0, 1);   // the deleting destructor
    return nullptr;
}

// 0x894A50 (cdecl): NiDX9TextureBufferData::CreateCubeFace (a cube texture's face, its top level;
// the third and fourth arguments aren't used)
void* __cdecl NiDX9TextureBufferData_CreateCubeFace(void* cube, uint32_t face, void*, void*, re::Ptr32<void>* buffer) {
    uint8_t* data = static_cast<uint8_t*>(NiNew(0x1C));
    if (data != nullptr) {
        ConstructBase(data, kTextureBufferDataVtable);
        At<void*>(data, 0x18) = nullptr;
    }
    At<void*>(data, 0x18) = cube;
    Com<long>(cube, kAddRef);
    long hr = Com<long>(At<void*>(data, 0x18), kGetSurfaceLevel, face, 0u, &Surface(data));
    SurfaceDesc desc;
    if (hr < 0) {
        ni::dx9::NiOutputDebugString("NiDX9TextureBufferData::Create> FAILED- GetSurfaceLevel - %s", NiDX9ErrorString(hr));
        Surface(data) = nullptr;
    } else if (Com<long>(Surface(data), kSurfGetDesc, &desc) >= 0) {
        At<void*>(data, 0xC) = PixelFormat_CreateFromD3DFormat(desc.format);
        AttachBuffer(data, buffer, desc, false);
        At<uint16_t>(data, 0x10) = MultisampleWord(desc);
        return data;
    }
    Virtual<void*>(data, 0, 1);
    return nullptr;
}

// 0x894B70 (thiscall): NiDX9TextureBufferData::ReleaseReferences (and the texture)
void __fastcall NiDX9TextureBufferData_ReleaseReferences(uint8_t* self, re::Edx) {
    ReleaseCom(Surface(self));
    NiDelete(At<void*>(self, 0xC), 0x44);
    At<void*>(self, 0xC) = nullptr;
    ReleaseCom(At<void*>(self, 0x18));
}

// 0x894BC0 (thiscall): NiDX9DepthStencilBufferData's destructor (unset from the device when it's
// its depth stencil now)
void __fastcall NiDX9DepthStencilBufferData_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kDepthStencilBufferDataVtable;
    uint8_t* renderer = Renderer();
    if (renderer != nullptr) {
        void* current = nullptr;
        if (Com<long>(At<void*>(renderer, 0xA4), kGetDepthStencilSurface, &current) >= 0) {
            if (current == Surface(self)) {
                ClearDepthTarget(At<void*>(renderer, 0xA4));
            }
            Com<long>(current, kRelease);
        }
    }
    DestructBase(self);
}

// 0x894C70 (thiscall): NiDX9DepthStencilBufferData::GetRTTI
void* __fastcall NiDX9DepthStencilBufferData_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiDX9DepthStencilBufferData;
}

// 0x894C80 (thiscall): NiDX9DepthStencilBufferData's deleting destructor
void* __fastcall NiDX9DepthStencilBufferData_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    DepthStencilBufferData_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x18);
    }
    return self;
}

// 0x894CA0 (thiscall): NiDX9AdditionalDepthStencilBufferData's constructor (added to the list)
void* __fastcall NiDX9AdditionalDepthStencilBufferData_Construct(uint8_t* self, re::Edx) {
    ConstructBase(self, kAdditionalDepthStencilVtable);
    At<uint32_t>(self, 0x18) = 0;
    ni::sys::Lock(ms_kAdditionalLock);
    re::Ptr32<void>* node = Virtual<re::Ptr32<void>*>(reinterpret_cast<void*>(kAdditionalList), 1);
    node[2] = self;
    node[1] = nullptr;
    node[0] = ms_pkAdditionalHead;
    if (ms_pkAdditionalHead != nullptr) {
        ms_pkAdditionalHead[1] = node;
    } else {
        ms_pkAdditionalTail = node;
    }
    ms_uiAdditionalCount++;
    ms_pkAdditionalHead = node;
    ni::sys::Unlock(ms_kAdditionalLock);
    return self;
}

// 0x894D50 (thiscall): NiDX9AdditionalDepthStencilBufferData::GetRTTI
void* __fastcall NiDX9AdditionalDepthStencilBufferData_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiDX9AdditionalDepthStencilBufferData;
}

// 0x894D60 (cdecl): NiDX9AdditionalDepthStencilBufferData::Create (a depth stencil surface of the
// format, the buffer's size and multisampling; the renderer's slot 29 called, its result dropped)
void* __cdecl NiDX9AdditionalDepthStencilBufferData_Create(void* device, re::Ptr32<void>* buffer, const void* format,
                                                            uint32_t multisample) {
    if (format == nullptr) {
        return nullptr;
    }
    uint32_t d3dFormat = PixelFormat_GetD3DFormat(format);
    if (d3dFormat == 0) {
        return nullptr;
    }
    uint8_t* renderer = Renderer();
    Virtual<void*>(renderer, 29);
    long hr = Com<long>(ni::dx9::ms_pkD3D9, kCheckDeviceFormat, At<uint32_t>(renderer, 0x3E0),
                        At<uint32_t>(renderer, 0x3E4), At<uint32_t>(renderer, 0x3EC), 2u, 1u, d3dFormat);
    if (hr < 0) {
        ni::dx9::NiOutputDebugString("NiDX9AdditionalDepthStencilBufferData::Create> FAILED %s", NiDX9ErrorString(hr));
        return nullptr;
    }
    void* memory = NiNew(0x1C);
    uint8_t* data = memory != nullptr ? static_cast<uint8_t*>(AdditionalDepthStencil_Construct(memory)) : nullptr;
    uint32_t type = 0;
    uint32_t quality = 0;
    GetD3DMultisampleType(multisample, &type, &quality);
    uint8_t* target = static_cast<uint8_t*>(*buffer);
    hr = Com<long>(device, kCreateDepthStencilSurface, At<uint32_t>(target, 8), At<uint32_t>(target, 0xC), d3dFormat,
                   type, quality, 0, &Surface(data), static_cast<void*>(nullptr));
    if (hr < 0) {
        ni::dx9::NiOutputDebugString("NiDX9AdditionalDepthStencilBufferData::Create> FAILED %s", NiDX9ErrorString(hr));
        if (data != nullptr) {
            Virtual<void*>(data, 0, 1);
        }
        return nullptr;
    }
    SurfaceDesc desc;
    if (Com<long>(Surface(data), kSurfGetDesc, &desc) < 0) {
        Virtual<void*>(data, 0, 1);
        return nullptr;
    }
    void* pixelFormat = PixelFormat_CreateFromD3DFormat(desc.format);
    At<uint32_t>(data, 0x18) = d3dFormat;
    At<void*>(data, 0xC) = pixelFormat;
    Ni2DBuffer_SetRendererData(*buffer, data);
    At<void*>(data, 8) = *buffer;
    At<uint16_t>(data, 0x10) = uint16_t(GetMultisampleWord(desc.multiSampleType, int(desc.multiSampleQuality)));
    return data;
}

// 0x894EE0 (thiscall): NiDX9ImplicitBufferData's destructor (the device let go)
void __fastcall NiDX9ImplicitBufferData_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kImplicitBufferDataVtable;
    ReleaseCom(At<void*>(self, 0x50));
    DestructBase(self);
}

// 0x894F50 (thiscall): NiDX9ImplicitBufferData::GetRTTI
void* __fastcall NiDX9ImplicitBufferData_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiDX9ImplicitBufferData;
}

// 0x894F60 (thiscall): NiDX9ImplicitBufferData::ReleaseReferences (and the device)
void __fastcall NiDX9ImplicitBufferData_ReleaseReferences(uint8_t* self, re::Edx) {
    ReleaseCom(Surface(self));
    NiDelete(At<void*>(self, 0xC), 0x44);
    At<void*>(self, 0xC) = nullptr;
    ReleaseCom(At<void*>(self, 0x50));
}

// 0x894FB0 (thiscall): NiDX92DBufferData's deleting destructor
void* __fastcall NiDX92DBufferData_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    DestructBase(self);
    if (flags & 1) {
        NiDelete(self, 0x18);
    }
    return self;
}

// 0x895010 (thiscall): NiDX9ImplicitDepthStencilBufferData::GetRTTI
void* __fastcall NiDX9ImplicitDepthStencilBufferData_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiDX9ImplicitDepthStencilBufferData;
}

// 0x895020 (thiscall): NiDX9AdditionalDepthStencilBufferData's destructor (off the list)
void __fastcall NiDX9AdditionalDepthStencilBufferData_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kAdditionalDepthStencilVtable;
    ni::sys::Lock(ms_kAdditionalLock);
    re::Ptr32<void> held = self;
    PointerList_Remove(reinterpret_cast<void*>(kAdditionalList), &held);
    ni::sys::Unlock(ms_kAdditionalLock);
    DepthStencilBufferData_Destruct(self);
}

// 0x895090 (thiscall): NiDX9ImplicitBufferData's deleting destructor
void* __fastcall NiDX9ImplicitBufferData_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    ImplicitBufferData_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x54);
    }
    return self;
}

// 0x8950B0 (cdecl): NiDX9ImplicitBufferData::Create (the device's back buffer, the presentation
// parameters kept; the renderer's target limits read the first time)
void* __cdecl NiDX9ImplicitBufferData_Create(void* device, const void* parameters, re::Ptr32<void>* buffer) {
    uint8_t* data = static_cast<uint8_t*>(NiNew(0x54));
    if (data != nullptr) {
        ConstructBase(data, kImplicitBufferDataVtable);
        At<void*>(data, 0x50) = nullptr;
    }
    crt::MemcpyS(data + 0x18, 0x38, parameters, 0x38);
    At<void*>(data, 0x50) = device;
    Com<long>(device, kAddRef);
    SurfaceDesc desc;
    if (Com<long>(device, kGetRenderTarget, 0u, &Surface(data)) < 0 ||
        Com<long>(Surface(data), kSurfGetDesc, &desc) < 0) {
        Virtual<void*>(data, 0, 1);
        return nullptr;
    }
    At<void*>(data, 0xC) = PixelFormat_CreateFromD3DFormat(desc.format);
    ni::dx9::NiOutputDebugString("NiDX9Renderer::Create> Using %s backbuffer format\n", GetD3DFormatString(desc.format));
    if (ms_uiMaxRenderTargets == 0) {
        uint8_t* renderer = Renderer();
        ms_uiMaxRenderTargets = Virtual<uint32_t>(renderer, 39);   // GetMaxBuffersPerRenderTargetGroup
        ms_bIndependentBitDepths = Virtual<uint8_t>(renderer, 40);   // GetIndependentBufferBitDepths
    }
    AttachBuffer(data, buffer, desc, false);
    At<uint16_t>(data, 0x10) = MultisampleWord(desc);
    return data;
}

// 0x895210 (cdecl): NiDX9ImplicitDepthStencilBufferData::Create (the device's depth stencil
// surface; made the current one)
void* __cdecl NiDX9ImplicitDepthStencilBufferData_Create(void* device, re::Ptr32<void>* buffer) {
    uint8_t* data = static_cast<uint8_t*>(NiNew(0x18));
    if (data != nullptr) {
        ConstructBase(data, kImplicitDepthStencilVtable);
    }
    if (Com<long>(device, kGetDepthStencilSurface, &Surface(data)) < 0) {
        if (data != nullptr) {
            Virtual<void*>(data, 0, 1);
        }
        return nullptr;
    }
    SurfaceDesc desc;
    if (Com<long>(Surface(data), kSurfGetDesc, &desc) < 0) {
        Virtual<void*>(data, 0, 1);
        return nullptr;
    }
    At<void*>(data, 0xC) = PixelFormat_CreateFromD3DFormat(desc.format);
    AttachBuffer(data, buffer, desc, true);
    At<uint16_t>(data, 0x10) = MultisampleWord(desc);
    ms_pkCurrentDepthStencil = Surface(data);
    return data;
}

// 0x895320 (thiscall): NiDX9AdditionalDepthStencilBufferData's deleting destructor
void* __fastcall NiDX9AdditionalDepthStencilBufferData_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    AdditionalDepthStencil_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x1C);
    }
    return self;
}

#define C40F_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C40F_REVERSED("NiDX92DBufferData::ClearRenderTarget", 0x008942C0, NiDX92DBufferData_ClearRenderTarget);
C40F_REVERSED("NiDX92DBufferData::Shutdown", 0x00894300, NiDX92DBufferData_Shutdown);
C40F_REVERSED("NiDX9DepthStencilBufferData::ClearDepthTarget", 0x00894310, NiDX9DepthStencilBufferData_ClearDepthTarget);
C40F_REVERSED("NiDX9ImplicitBufferData::DisplayFrame", 0x00894340, NiDX9ImplicitBufferData_DisplayFrame);
C40F_REVERSED("NiDX92DBufferData::MakeMultisampleWord", 0x00894360, NiDX92DBufferData_MakeMultisampleWord);
C40F_REVERSED("NiDX92DBufferData::GetRTTI", 0x00894380, NiDX92DBufferData_GetRTTI);
C40F_REVERSED("NiDX92DBufferData::GetD3DMultisampleType", 0x00894390, NiDX92DBufferData_GetD3DMultisampleType);
C40F_REVERSED("NiDX92DBufferData::GetMultisampleWord", 0x008943E0, NiDX92DBufferData_GetMultisampleWord);
C40F_REVERSED("NiDX92DBufferData::SetRenderTarget", 0x00894420, NiDX92DBufferData_SetRenderTarget);
C40F_REVERSED("NiDX9DepthStencilBufferData::SetDepthTarget", 0x00894480, NiDX9DepthStencilBufferData_SetDepthTarget);
C40F_REVERSED("NiDX9DepthStencilBufferData::HasValidDepthBuffer", 0x008944C0,
              NiDX9DepthStencilBufferData_HasValidDepthBuffer);
C40F_REVERSED("NiDX9DepthStencilBufferData::HasValidStencilBuffer", 0x008944E0,
              NiDX9DepthStencilBufferData_HasValidStencilBuffer);
C40F_REVERSED("NiDX9ImplicitDepthStencilBufferData::Recreate", 0x00894500, NiDX9ImplicitDepthStencilBufferData_Recreate);
C40F_REVERSED("NiDX9AdditionalDepthStencilBufferData::ShutdownAll", 0x00894590,
              NiDX9AdditionalDepthStencilBufferData_ShutdownAll);
C40F_REVERSED("NiDX9AdditionalDepthStencilBufferData::RecreateAll", 0x008945F0,
              NiDX9AdditionalDepthStencilBufferData_RecreateAll);
C40F_REVERSED("NiDX9AdditionalDepthStencilBufferData::Recreate", 0x00894660,
              NiDX9AdditionalDepthStencilBufferData_Recreate);
C40F_REVERSED("NiDX9ImplicitBufferData::Recreate", 0x00894780, NiDX9ImplicitBufferData_Recreate);
C40F_REVERSED("NiDX92DBufferData::ReleaseReferences", 0x00894860, NiDX92DBufferData_ReleaseReferences);
C40F_REVERSED("NiDX9TextureBufferData::~NiDX9TextureBufferData", 0x00894890, NiDX9TextureBufferData_Destruct);
C40F_REVERSED("NiDX9TextureBufferData::GetRTTI", 0x00894900, NiDX9TextureBufferData_GetRTTI);
C40F_REVERSED("NiDX9TextureBufferData::DeletingDestructor", 0x00894910, NiDX9TextureBufferData_DeletingDestructor);
C40F_REVERSED("NiDX9TextureBufferData::Create", 0x00894930, NiDX9TextureBufferData_Create);
C40F_REVERSED("NiDX9TextureBufferData::CreateCubeFace", 0x00894A50, NiDX9TextureBufferData_CreateCubeFace);
C40F_REVERSED("NiDX9TextureBufferData::ReleaseReferences", 0x00894B70, NiDX9TextureBufferData_ReleaseReferences);
C40F_REVERSED("NiDX9DepthStencilBufferData::~NiDX9DepthStencilBufferData", 0x00894BC0,
              NiDX9DepthStencilBufferData_Destruct);
C40F_REVERSED("NiDX9DepthStencilBufferData::GetRTTI", 0x00894C70, NiDX9DepthStencilBufferData_GetRTTI);
C40F_REVERSED("NiDX9DepthStencilBufferData::DeletingDestructor", 0x00894C80,
              NiDX9DepthStencilBufferData_DeletingDestructor);
C40F_REVERSED("NiDX9AdditionalDepthStencilBufferData::NiDX9AdditionalDepthStencilBufferData", 0x00894CA0,
              NiDX9AdditionalDepthStencilBufferData_Construct);
C40F_REVERSED("NiDX9AdditionalDepthStencilBufferData::GetRTTI", 0x00894D50,
              NiDX9AdditionalDepthStencilBufferData_GetRTTI);
C40F_REVERSED("NiDX9AdditionalDepthStencilBufferData::Create", 0x00894D60, NiDX9AdditionalDepthStencilBufferData_Create);
C40F_REVERSED("NiDX9ImplicitBufferData::~NiDX9ImplicitBufferData", 0x00894EE0, NiDX9ImplicitBufferData_Destruct);
C40F_REVERSED("NiDX9ImplicitBufferData::GetRTTI", 0x00894F50, NiDX9ImplicitBufferData_GetRTTI);
C40F_REVERSED("NiDX9ImplicitBufferData::ReleaseReferences", 0x00894F60, NiDX9ImplicitBufferData_ReleaseReferences);
C40F_REVERSED("NiDX92DBufferData::DeletingDestructor", 0x00894FB0, NiDX92DBufferData_DeletingDestructor);
C40F_REVERSED("NiDX9ImplicitDepthStencilBufferData::GetRTTI", 0x00895010,
              NiDX9ImplicitDepthStencilBufferData_GetRTTI);
C40F_REVERSED("NiDX9AdditionalDepthStencilBufferData::~NiDX9AdditionalDepthStencilBufferData", 0x00895020,
              NiDX9AdditionalDepthStencilBufferData_Destruct);
C40F_REVERSED("NiDX9ImplicitBufferData::DeletingDestructor", 0x00895090, NiDX9ImplicitBufferData_DeletingDestructor);
C40F_REVERSED("NiDX9ImplicitBufferData::Create", 0x008950B0, NiDX9ImplicitBufferData_Create);
C40F_REVERSED("NiDX9ImplicitDepthStencilBufferData::Create", 0x00895210, NiDX9ImplicitDepthStencilBufferData_Create);
C40F_REVERSED("NiDX9AdditionalDepthStencilBufferData::DeletingDestructor", 0x00895320,
              NiDX9AdditionalDepthStencilBufferData_DeletingDestructor);

} // namespace ni::c40
