// NiD3D10Renderer (src/gamebryo/NiD3D10Renderer.h): the D3D10 library's loading, the device's
// creation, and the renderer's small slots and helpers (clear values, render target groups,
// renderer data, the swap chain and its render target group, the display result's callbacks).

#include "gamebryo/NiD3D10Renderer.h"
#include "reversed.h"

namespace ni::c42 {
namespace {

using namespace ni::d3d10;

constexpr uint32_t DXGI_ERROR_DEVICE_REMOVED = 0x887A0005;
constexpr uint32_t DXGI_ERROR_DEVICE_HUNG = 0x887A0006;
constexpr uint32_t DXGI_ERROR_DEVICE_RESET = 0x887A0007;
constexpr uint32_t DXGI_STATUS_OCCLUDED = 0x087A0001;
constexpr long E_INVALIDARG_ = long(0x80070057);
constexpr uintptr_t kLoadLibraryA = 0x008FC0C8;
constexpr uintptr_t kGetProcAddress = 0x008FC0C4;
constexpr uintptr_t kFreeLibrary = 0x008FC0C0;

// the D3D10 entry points' names, in their slots' order (the game's strings)
constexpr uintptr_t kEntryNames[26] = {
    0x0099F9C4, 0x0099F9B0, 0x0099F990, 0x0099F978, 0x0099F954, 0x0099F938, 0x0099F91C,
    0x0099F8FC, 0x0099F8DC, 0x0099F8B8, 0x0099F898, 0x0099F874, 0x0099F854, 0x0099F834,
    0x0099F818, 0x0099F804, 0x0099F7EC, 0x0099F7CC, 0x0099F7B0, 0x0099F794, 0x0099F77C,
    0x0099F768, 0x0099F748, 0x0099F72C, 0x0099F70C, 0x0099F6F4,
};

// ID3D10Device slots
enum : int {
    kOMSetRenderTargets = 24,
    kClearRenderTargetView = 35,
    kClearDepthStencilView = 36,
    kCheckFormatSupport = 90,
    kGetDeviceRemovedReason = 63,
};
// IDXGIOutput 9 FindClosestMatchingMode; IDXGIFactory 10 CreateSwapChain
constexpr int kFindClosestMatchingMode = 9;
constexpr int kCreateSwapChain = 10;

GAME_VAR(void*, ms_pkRenderer, 0x020E2380);   // NiRenderer::ms_pkRenderer
GAME_VAR(uint8_t, ms_bStaticFlagBCB9A1, 0x00BCB9A1);
GAME_VAR(uint32_t, NiRenderedCubeMap_ClassBit, 0x020E3874);
constexpr uintptr_t NiSourceTexture_RTTI = 0x020E2A10;
constexpr uintptr_t kFormatR8G8B8A8 = 0x020E7D78;   // the R8G8B8A8_UNORM pixel format

GAME_FN(void, __thiscall, CriticalSection_Unlock, 0x00401000, NiCriticalSection* self);
GAME_FN(void*, __cdecl, DynamicCast, 0x00401FF0, uintptr_t rtti, void* object);
GAME_FN(void, __cdecl, AlignedFree, 0x007523A0, void* p);
GAME_FN(void, __cdecl, GameOperatorDelete, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, Renderer_SetDisplayFormat, 0x00754A50, uintptr_t format);   // (0x44 bytes to 0x20E2200)
GAME_FN(void*, __cdecl, RenderTargetGroup_Create, 0x0075D670, void* buffer, void* renderer, void* depthStencil);
GAME_FN(void, __thiscall, PointerList_AddTail, 0x0075F630, void* list, const re::Ptr32<void>* value);
GAME_FN(void, __thiscall, PointerList_RemovePos, 0x00771DA0, void* list, re::Ptr32<void>* position);
GAME_FN(void*, __cdecl, DepthStencilBuffer_Create, 0x007ABE40, uint32_t width, uint32_t height, void* renderer,
        const void* format, uint32_t msaa);
GAME_FN(void*, __cdecl, DepthStencilBuffer_CreateCompatible, 0x007ABF40, void* buffer, void* renderer);
GAME_FN(void*, __thiscall, OutputDesc_GetOutput, 0x00856850, void* self);
GAME_FN(void*, __thiscall, SystemDesc_GetFactory, 0x007E2BE0, void* self);
GAME_FN(void, __thiscall, NiPointer_Assign, 0x00800420, re::Ptr32<void>* slot, void* object);
GAME_FN(void, __cdecl, TextureData_ClearTextureData, 0x008930B0);
// the D3D10 renderer's other parts (not this chunk's)
GAME_FN(void*, __thiscall, ResourceManager_Construct, 0x008B3570, void* self, void* device);
GAME_FN(void*, __thiscall, Manager448_Construct, 0x008B1570, void* self);
GAME_FN(void*, __thiscall, RenderStateManager_Construct, 0x008B10D0, void* self, void* device, void* resources);
GAME_FN(uint32_t, __cdecl, ResourceManager_Function8B4380, 0x008B4380, void* a, void* b);
GAME_FN(uint8_t, __thiscall, TextureData_IsRenderedTexture, 0x008B4580, void* data);
GAME_FN(void*, __thiscall, FragmentShader_Construct, 0x008B5E80, void* self, void* descriptor);
GAME_FN(void*, __thiscall, ShadowWriteShader_Construct, 0x008B5FA0, void* self, void* descriptor);
GAME_FN(void, __cdecl, SetModelTransform_8B60A0, 0x008B60A0, void* matrix, const void* transform);
GAME_FN(void*, __thiscall, BufferData_GetView, 0x008B6120, void* data);
GAME_FN(void*, __cdecl, SwapChainBufferData_Create, 0x008B6420, void* swapChain, re::Ptr32<void>* buffer);
GAME_FN(void*, __cdecl, DepthStencilBufferData_Create, 0x008B6600, void* texture, re::Ptr32<void>* buffer);
GAME_FN(void*, __cdecl, SourceTextureData_Create, 0x008B6EE0, void* texture);
GAME_FN(void*, __cdecl, RenderedTextureData_Create, 0x008B7270, void* texture, uint32_t format);
// this chunk's
GAME_FN(const char*, __cdecl, GetFormatName, 0x008A88B0, uint32_t format, int full);
GAME_FN(uint8_t*, __thiscall, SystemDesc_GetAdapterDesc, 0x008A7A60, void* self, uint32_t index);
GAME_FN(uint32_t, __thiscall, AdapterDesc_GetOutputCount, 0x008A7AE0, void* self);
GAME_FN(uint8_t*, __thiscall, AdapterDesc_GetOutputDesc, 0x008A7AF0, void* self, uint32_t index);
GAME_FN(const void*, __cdecl, PixelFormat_GetPixelFormat, 0x008A8450, uint32_t format);
GAME_FN(uint32_t, __cdecl, PixelFormat_DetermineDXGIFormat, 0x008A8D80, const void* pixelFormat);
GAME_FN(void*, __thiscall, ShaderResourceManager_Construct, 0x008AFBC0, void* self, void* resources);
GAME_FN(void*, __thiscall, TextureCreator_Construct, 0x008AFE30, void* self, void* device);
GAME_FN(void*, __thiscall, TextureCreator_CreateTexture2D, 0x008AFF00, void* self, uint32_t width, uint32_t height,
        uint32_t mips, uint32_t arraySize, uint32_t format, uint32_t samples, uint32_t quality, uint32_t usage,
        uint32_t bind, uint32_t cpuAccess, uint32_t misc, void* initial);
GAME_FN(void, __thiscall, RenderStateManager_SetLeftRightSwap, 0x008B0000, void* self, int swap);
GAME_FN(uint8_t, __cdecl, D3D10_ReleaseLibrary, 0x008A95C0);
GAME_FN(uint8_t, __cdecl, D3D10_LoadLibrary, 0x008AB270);
GAME_FN(uint8_t, __cdecl, D3D10_EnsureLibrary, 0x008AD700);

inline uint8_t* Device(uint8_t* self) {
    return At<uint8_t*>(self, kDevice);
}
inline void* CurrentGroup(uint8_t* self) {
    return At<void*>(self, kCurrentRenderTargetGroup);
}

// the D3D10 library's lock, as the code inlines NiCriticalSection's
inline void LockLibrary() {
    sys::Lock(ms_kD3D10LibraryLock);
}
inline void UnlockLibrary() {
    sys::Unlock(ms_kD3D10LibraryLock);
}

// a renderer data's NiPointer at +0x10 of a buffer let go
inline void ReleaseBufferRendererData(uint8_t* buffer) {
    re::Ptr32<void>& data = At<void*>(buffer, 0x10);
    if (data != nullptr) {
        ReleaseRef(data);
        data = nullptr;
    }
}

// the display result's callbacks (an array of functions and one of their data): false when one
// refuses
template <typename... A>
bool RunCallbacks(uint8_t* self, int functions, A... args) {
    uint32_t count = At<uint16_t>(self, functions + 0xA);
    for (uint32_t i = 0; i < count; i++) {
        auto callback = At<re::Ptr32<uint8_t __cdecl(A..., void*)>*>(self, functions + 4)[i];
        void* data = At<re::Ptr32<void>*>(self, functions + 0x14)[i];
        if (callback != nullptr && callback(args..., data) == 0) {
            return false;
        }
    }
    return true;
}

} // namespace

// ---- the D3D10 library

// 0x8A94C0: D3D10CreateBlob, through the loaded library (E_INVALIDARG without it)
long __cdecl NiD3D10Renderer_D3D10CreateBlob(uint32_t size, re::Ptr32<void>* blob) {
    if (ms_hD3D10 == nullptr) {
        return E_INVALIDARG_;
    }
    return re::PtrCast<long(__stdcall*)(uint32_t, re::Ptr32<void>*)>(ms_apfnD3D10[kD3D10CreateBlob])(size, blob);
}

// 0x8A95C0: NiD3D10Renderer::ReleaseD3D10 (the library freed, its entry points forgotten)
uint8_t __cdecl NiD3D10Renderer_ReleaseD3D10() {
    LockLibrary();
    if (ms_hD3D10 != nullptr) {
        Import<int>(kFreeLibrary, ms_hD3D10);
        ms_hD3D10 = nullptr;
        for (int i = 0; i < 26; i++) {
            ms_apfnD3D10[i] = nullptr;
        }
    }
    UnlockLibrary();
    return 1;
}
RE_UNHOOKED_FN("NiD3D10Renderer::ReleaseD3D10_Thunk", 0x008AB260, &NiD3D10Renderer_ReleaseD3D10);   // (a lone jmp to 0x8A95C0)

// 0x8AB270: NiD3D10Renderer::LoadD3D10 ("D3D10.dll" and its 26 entry points; quirk kept: when the
// library can't be loaded the lock is never left)
uint8_t __cdecl NiD3D10Renderer_LoadD3D10() {
    LockLibrary();
    if (ms_hD3D10 != nullptr) {
        UnlockLibrary();
        return 1;
    }
    void* module = Import<void*>(kLoadLibraryA, Text(0x0099F9D4));   // "D3D10.dll"
    ms_hD3D10 = module;
    if (module == nullptr) {
        Error(2, static_cast<const char*>(nullptr));
        return 0;
    }
    for (int i = 0; i < 26; i++) {
        ms_apfnD3D10[i] = Import<void*>(kGetProcAddress, ms_hD3D10, Text(kEntryNames[i]));
    }
    bool all = true;
    for (int i = 0; i < 26; i++) {
        if (ms_apfnD3D10[i] == nullptr) {
            all = false;
            break;
        }
    }
    if (all) {
        CriticalSection_Unlock(&ms_kD3D10LibraryLock);
        return 1;
    }
    Error(2, Text(0x0099F6A8));   // "Library loaded but some procedure addresses not found; releasing library."
    D3D10_ReleaseLibrary();
    UnlockLibrary();
    return 0;
}

// 0x8AD700: NiD3D10Renderer::EnsureD3D10Loaded
uint8_t __cdecl NiD3D10Renderer_EnsureD3D10Loaded() {
    LockLibrary();
    uint8_t loaded = 1;
    if (ms_hD3D10 == nullptr) {
        loaded = D3D10_LoadLibrary();
    }
    UnlockLibrary();
    return loaded;
}

// 0x8AD8F0: NiD3D10Renderer::CreateDevice(adapter, driver type, flags) (no adapter for the
// reference device; null on failure, reported)
void* __cdecl NiD3D10Renderer_CreateDevice(void* adapter, uint32_t driverType, uint32_t flags) {
    void* device = nullptr;
    LockLibrary();
    if (D3D10_EnsureLibrary() == 0) {
        Warning(Text(0x0099FE40));
    } else {
        if (driverType == 1) {
            adapter = nullptr;
        }
        long hr;
        if (ms_hD3D10 == nullptr) {
            hr = E_INVALIDARG_;
        } else {
            hr = re::PtrCast<long(__stdcall*)(void*, uint32_t, void*, uint32_t, uint32_t, void**)>(
                ms_apfnD3D10[kD3D10CreateDevice])(adapter, driverType, nullptr, flags, 0x1D, &device);
        }
        if (hr < 0) {
            Error(4, Text(0x0099E584), hr);
        } else if (device == nullptr) {
            Error(4, Text(0x0099FDD4));
        }
        if (hr < 0 || device == nullptr) {
            if (device != nullptr) {
                ComRelease(device);
            }
        }
    }
    UnlockLibrary();
    return device;
}

// ---- the renderer's set-up helpers

// 0x8A8F80 (thiscall): NiD3D10Renderer::CheckFormatSupport (every format but the last four;
// R8G8B8A8_UNORM made the display format)
void __fastcall NiD3D10Renderer_CheckFormatSupport(uint8_t* self, re::Edx) {
    uint32_t* support = &At<uint32_t>(self, kFormatSupport + 4);
    for (uint32_t format = 1; format < 0x59; format++) {
        if (format == 0x55 || format == 0x56 || format == 0x57 || format == 0x58) {
            continue;
        }
        long hr = Com<long>(Device(self), kCheckFormatSupport, format, support);
        if (hr < 0) {
            Warning(Text(0x0099F1B0), GetFormatName(format, 0), hr);
        }
        support++;
    }
    Renderer_SetDisplayFormat(kFormatR8G8B8A8);
}

// 0x8A9000 (thiscall): NiD3D10Renderer::CreateManagers (the resource, render state, texture and
// shader resource managers; names guesses)
void __fastcall NiD3D10Renderer_CreateManagers(uint8_t* self, re::Edx) {
    void* memory = NiNew(0x26C);
    At<void*>(self, kResourceManager) = memory != nullptr ? ResourceManager_Construct(memory, Device(self)) : nullptr;
    memory = NiNew(4);
    At<void*>(self, kManager448) = memory != nullptr ? Manager448_Construct(memory) : nullptr;
    memory = NiNew(0x1500);
    At<void*>(self, kRenderStateManager) =
        memory != nullptr ? RenderStateManager_Construct(memory, Device(self), At<void*>(self, kResourceManager))
                          : nullptr;
    memory = NiNew(8);
    At<void*>(self, kManager450) = memory != nullptr ? TextureCreator_Construct(memory, Device(self)) : nullptr;
    memory = NiNew(0xBC);
    if (memory != nullptr) {
        At<void*>(self, kShaderResourceManager) =
            ShaderResourceManager_Construct(memory, At<void*>(self, kResourceManager));
        return;
    }
    At<void*>(self, kShaderResourceManager) = nullptr;
}

// 0x8A90E0 (thiscall): NiD3D10Renderer::GetFormatSupport(format, flags) (any of them)
uint8_t __fastcall NiD3D10Renderer_GetFormatSupport(uint8_t* self, re::Edx, uint32_t format, uint32_t flags) {
    return (At<uint32_t>(self, kFormatSupport + int(format) * 4) & flags) != 0 ? 1 : 0;
}

// 0x8A9100 (thiscall): the resource manager
void* __fastcall NiD3D10Renderer_GetResourceManager(uint8_t* self, re::Edx) {
    return At<void*>(self, kResourceManager);
}
// 0x8A9110 (thiscall): the render state manager
void* __fastcall NiD3D10Renderer_GetRenderStateManager(uint8_t* self, re::Edx) {
    return At<void*>(self, kRenderStateManager);
}
// 0x8A9120 (thiscall): the texture creator (+0x450)
void* __fastcall NiD3D10Renderer_GetTextureCreator(uint8_t* self, re::Edx) {
    return At<void*>(self, kManager450);
}
// 0x8A9130 (thiscall): the shader resource manager
void* __fastcall NiD3D10Renderer_GetShaderResourceManager(uint8_t* self, re::Edx) {
    return At<void*>(self, kShaderResourceManager);
}
// 0x8A9140 (thiscall): the ID3D10Device
void* __fastcall NiD3D10Renderer_GetD3D10Device(uint8_t* self, re::Edx) {
    return At<void*>(self, kDevice);
}

// ---- NiRenderer's slots

// 0x8A9150 (thiscall, slot 19): GetFlags
uint32_t __fastcall NiD3D10Renderer_GetFlags(void*, re::Edx) {
    return 0x1A;
}

// 0x8A9160 (thiscall, slot 20): SetDepthClear
void __fastcall NiD3D10Renderer_SetDepthClear(uint8_t* self, re::Edx, float clear) {
    At<float>(self, kDepthClear) = clear;
}

// 0x8A9170 (thiscall, slot 21): GetDepthClear
float __fastcall NiD3D10Renderer_GetDepthClear(uint8_t* self, re::Edx) {
    return At<float>(self, kDepthClear);
}

// 0x8A9180 (thiscall, slot 23): SetBackgroundColor(NiColor) (alpha 1)
void __fastcall NiD3D10Renderer_SetBackgroundColorRGB(uint8_t* self, re::Edx, const float* color) {
    At<float>(self, kBackground) = color[0];
    At<float>(self, kBackground + 4) = color[1];
    At<float>(self, kBackground + 8) = color[2];
    At<float>(self, kBackground + 0xC) = 1.0f;
}

// 0x8A91B0 (thiscall, slot 22): SetBackgroundColor(NiColorA)
void __fastcall NiD3D10Renderer_SetBackgroundColorRGBA(uint8_t* self, re::Edx, const float* color) {
    for (int i = 0; i < 4; i++) {
        At<float>(self, kBackground + i * 4) = color[i];
    }
}

// 0x8A91E0 (thiscall, slot 24): GetBackgroundColor
void __fastcall NiD3D10Renderer_GetBackgroundColor(uint8_t* self, re::Edx, float* color) {
    for (int i = 0; i < 4; i++) {
        color[i] = At<float>(self, kBackground + i * 4);
    }
}

// 0x8A9210 (thiscall, slot 25): SetStencilClear (a byte)
void __fastcall NiD3D10Renderer_SetStencilClear(uint8_t* self, re::Edx, uint32_t clear) {
    At<uint8_t>(self, kStencilClear) = uint8_t(clear);
}

// 0x8A9220 (thiscall, slot 26): GetStencilClear
uint8_t __fastcall NiD3D10Renderer_GetStencilClear(uint8_t* self, re::Edx) {
    return At<uint8_t>(self, kStencilClear);
}

// 0x8A9230 (thiscall, slot 30): GetCurrentRenderTargetGroup
void* __fastcall NiD3D10Renderer_GetCurrentRenderTargetGroup(uint8_t* self, re::Edx) {
    return CurrentGroup(self);
}

// 0x8A9240 (thiscall, slot 47; `this` unused): handed to 0x8B4380 (unknown)
uint32_t __fastcall NiD3D10Renderer_Slot47(void*, re::Edx, void* a, void* b) {
    return ResourceManager_Function8B4380(a, b);
}

// 0x8A9260 (thiscall, slot 55): PurgeAllTextures (every texture's data cleared; the flag unused)
uint8_t __fastcall NiD3D10Renderer_PurgeAllTextures(void*, re::Edx, int) {
    TextureData_ClearTextureData();
    return 1;
}

// 0x8A9270 (thiscall, slot 60): GetLeftRightSwap
uint8_t __fastcall NiD3D10Renderer_GetLeftRightSwap(uint8_t* self, re::Edx) {
    return At<uint8_t>(self, kLeftRightSwap);
}

// 0x8A9280 (thiscall, slot 61): SetLeftRightSwap (the render state manager told)
uint8_t __fastcall NiD3D10Renderer_SetLeftRightSwap(uint8_t* self, re::Edx, int swap) {
    At<uint8_t>(self, kLeftRightSwap) = uint8_t(swap);
    RenderStateManager_SetLeftRightSwap(At<void*>(self, kRenderStateManager), swap);
    return 1;
}

// 0x8A92A0 (thiscall, slot 62): GetMaxFogValue
float __fastcall NiD3D10Renderer_GetMaxFogValue(uint8_t* self, re::Edx) {
    return At<float>(self, kMaxFogValue);
}

// 0x8A92B0 (thiscall, slot 63): SetMaxFogValue (at least 1e-5; the factor 1 / v - 1)
void __fastcall NiD3D10Renderer_SetMaxFogValue(uint8_t* self, re::Edx, float value) {
    if (value <= 0.0f) {
        value = *reinterpret_cast<const float*>(0x008FF230);   // 1e-5
    }
    At<float>(self, kMaxFogValue) = value;
    At<float>(self, kMaxFogFactor) = 1.0f / value - 1.0f;
}

// 0x8A92F0 (thiscall): GetMaxFogFactor
float __fastcall NiD3D10Renderer_GetMaxFogFactor(uint8_t* self, re::Edx) {
    return At<float>(self, kMaxFogFactor);
}

// 0x8A9300: NiD3D10Renderer::GetRenderer (NiRenderer's instance)
void* __cdecl NiD3D10Renderer_GetRenderer() {
    return ms_pkRenderer;
}

// 0x8A9310 (thiscall, slot 69): CreateSourceCubeMapRendererData (as a source texture's)
uint8_t __fastcall NiD3D10Renderer_CreateSourceCubeMapRendererData(uint8_t* self, re::Edx, void* cubeMap) {
    return Virtual<uint8_t>(self, 67, cubeMap);
}

// 0x8A9320 (thiscall, slot 70): CreateRenderedCubeMapRendererData (a rendered texture's, format
// 0x100)
uint8_t __fastcall NiD3D10Renderer_CreateRenderedCubeMapRendererData(uint8_t* self, re::Edx, void* cubeMap) {
    return Virtual<uint8_t>(self, 68, cubeMap, 0x100u);
}

// 0x8A9340 (thiscall, slot 75): GetFragmentShader (a new NiD3D10FragmentShader)
void* __fastcall NiD3D10Renderer_GetFragmentShader(void*, re::Edx, void* descriptor) {
    void* memory = NiNew(0x88);
    if (memory != nullptr) {
        return FragmentShader_Construct(memory, descriptor);
    }
    return nullptr;
}

// 0x8A9360 (thiscall, slot 78): GetShadowWriteShader (a new NiD3D10ShadowWriteShader)
void* __fastcall NiD3D10Renderer_GetShadowWriteShader(void*, re::Edx, void* descriptor) {
    void* memory = NiNew(0x88);
    if (memory != nullptr) {
        return ShadowWriteShader_Construct(memory, descriptor);
    }
    return nullptr;
}

// 0x8A9380 / 0x8A9390 / 0x8A93A0 / 0x8A93B0 (thiscall): the projection, view, inverse view and
// world matrices
void* __fastcall NiD3D10Renderer_GetProjectionMatrix(uint8_t* self, re::Edx) {
    return self + kProjection;
}
void* __fastcall NiD3D10Renderer_GetViewMatrix(uint8_t* self, re::Edx) {
    return self + kView;
}
void* __fastcall NiD3D10Renderer_GetInverseViewMatrix(uint8_t* self, re::Edx) {
    return self + kInvView;
}
void* __fastcall NiD3D10Renderer_GetWorldMatrix(uint8_t* self, re::Edx) {
    return self + kWorld;
}

// 0x8A93C0 (thiscall): the camera's near and far
void __fastcall NiD3D10Renderer_GetCameraNearAndFar(uint8_t* self, re::Edx, float* nearPlane, float* farPlane) {
    *nearPlane = At<float>(self, kFrustum + 0x10);
    *farPlane = At<float>(self, kFrustum + 0x14);
}

// 0x8A93E0 (thiscall, slot 88): Do_GetCameraData (the inverse view's rows, frustum and port)
void __fastcall NiD3D10Renderer_GetCameraData(uint8_t* self, re::Edx, float* location, float* direction, float* up,
                                              float* right, uint32_t* frustum, float* port) {
    for (int i = 0; i < 3; i++) {
        right[i] = At<float>(self, kInvView + i * 4);
        up[i] = At<float>(self, kInvView + 0x10 + i * 4);
        direction[i] = At<float>(self, kInvView + 0x20 + i * 4);
        location[i] = At<float>(self, kInvView + 0x30 + i * 4);
    }
    for (int i = 0; i < 7; i++) {
        frustum[i] = At<uint32_t>(self, kFrustum + i * 4);
    }
    for (int i = 0; i < 4; i++) {
        port[i] = At<float>(self, kPort + i * 4);
    }
}

// 0x8A94A0 (thiscall): the world matrix from a transform
void __fastcall NiD3D10Renderer_SetModelTransform(uint8_t* self, re::Edx, const void* transform) {
    SetModelTransform_8B60A0(self + kWorld, transform);
}

// 0x8A96C0 (thiscall): NiD3D10Renderer::CreateSwapChain(description, output) (fitted to the
// output's closest mode when there's an adapter; null on failure, reported)
void* __fastcall NiD3D10Renderer_CreateSwapChain(uint8_t* self, re::Edx, const uint32_t* description,
                                                 uint32_t output) {
    if (Device(self) == nullptr) {
        Warning(Text(0x0099F238));
        return nullptr;
    }
    uint32_t local[15];
    for (int i = 0; i < 15; i++) {
        local[i] = description[i];
    }
    uint32_t adapterIndex = At<uint32_t>(self, kAdapterIndex);
    if (adapterIndex != 0xFFFFFFFF) {
        uint8_t* adapter = SystemDesc_GetAdapterDesc(At<void*>(self, kSystemDesc), adapterIndex);
        if (output >= AdapterDesc_GetOutputCount(adapter)) {
            output = 0;
        }
        void* dxgiOutput = OutputDesc_GetOutput(AdapterDesc_GetOutputDesc(adapter, output));
        Com<long>(dxgiOutput, kFindClosestMatchingMode, description, local, Device(self));
    }
    void* factory = SystemDesc_GetFactory(At<void*>(self, kSystemDesc));
    void* swapChain = nullptr;
    long hr = Com<long>(factory, kCreateSwapChain, Device(self), local, &swapChain);
    if (hr < 0) {
        Error(6, Text(0x0099E584), hr);
    } else {
        if (swapChain != nullptr) {
            return swapChain;
        }
        Error(6, Text(0x0099F200));
    }
    if (swapChain != nullptr) {
        ComRelease(swapChain);
    }
    return nullptr;
}

// 0x8A9AF0 (thiscall, slot 31): GetDefaultDepthStencilBuffer (the default group's)
void* __fastcall NiD3D10Renderer_GetDefaultDepthStencilBuffer(uint8_t* self, re::Edx) {
    void* group = At<void*>(self, kDefaultRenderTargetGroup);
    if (group != nullptr) {
        return Virtual<void*>(group, 28);
    }
    return nullptr;
}

// 0x8A9B10 (thiscall, slot 32): GetDefaultBackBuffer (the default group's first)
void* __fastcall NiD3D10Renderer_GetDefaultBackBuffer(uint8_t* self, re::Edx) {
    void* group = At<void*>(self, kDefaultRenderTargetGroup);
    if (group != nullptr) {
        return Virtual<void*>(group, 27, 0u);
    }
    return nullptr;
}

// 0x8A9B30 (thiscall, slot 37): FindClosestDepthStencilFormat(front, depth bits, stencil bits)
// (D32_FLOAT_S8X24, D32_FLOAT, D24_UNORM_S8, D16_UNORM in an order by the bits asked; the first
// the device can use as a depth/stencil buffer)
const void* __fastcall NiD3D10Renderer_FindClosestDepthStencilFormat(uint8_t* self, re::Edx, const void*,
                                                                     uint32_t depthBits, uint32_t stencilBits) {
    const uintptr_t formats[4] = {0x020E7B38, 0x020E80D8, 0x020E8240, 0x020E8510};
    uint32_t order[4];
    if (stencilBits != 0) {
        if (depthBits <= 0x18) {
            order[0] = 2, order[1] = 0, order[2] = 3, order[3] = 1;
        } else {
            order[0] = 0, order[1] = 2, order[2] = 1, order[3] = 3;
        }
    } else if (depthBits <= 0x10) {
        order[0] = 3, order[1] = 2, order[2] = 1, order[3] = 0;
    } else if (depthBits <= 0x18) {
        order[0] = 2, order[1] = 1, order[2] = 0, order[3] = 3;
    } else {
        order[0] = 1, order[1] = 0, order[2] = 2, order[3] = 3;
    }
    for (uint32_t i = 0; i < 4; i++) {
        uintptr_t format = formats[order[i]];
        uint32_t dxgi = At<uint32_t>(reinterpret_cast<void*>(format), 0xC);
        if ((At<uint8_t>(self, kFormatSupport + 2 + int(dxgi) * 4) & 1) != 0) {   // DEPTH_STENCIL
            return reinterpret_cast<const void*>(format);
        }
    }
    return nullptr;
}

// 0x8A9C20 (thiscall, slot 48; `this` unused): PurgeGeometryData
void __fastcall NiD3D10Renderer_PurgeGeometryData(void*, re::Edx, uint8_t* data) {
    void* rendererData = At<void*>(data, 0x3C);
    if (rendererData != nullptr) {
        Virtual<void*>(rendererData, 0, 1);
        At<void*>(data, 0x3C) = nullptr;
    }
}

// 0x8A9C40 (thiscall, slot 53; `this` unused): PurgeSkinInstance (its bone matrices freed: the
// aligned ones, and the ones offset by the byte before them)
void __fastcall NiD3D10Renderer_PurgeSkinInstance(void*, re::Edx, uint8_t* skin) {
    AlignedFree(At<void*>(skin, 0x28));
    uint8_t* matrices = At<uint8_t*>(skin, 0x2C);
    At<void*>(skin, 0x28) = nullptr;
    At<uint32_t>(skin, 0x24) = 0;
    if (matrices != nullptr) {
        GameOperatorDelete(matrices - matrices[-1]);
    }
    At<void*>(skin, 0x2C) = nullptr;
}

// 0x8A9C80 (thiscall, slot 67): CreateSourceTextureRendererData (under the source data lock)
uint8_t __fastcall NiD3D10Renderer_CreateSourceTextureRendererData(uint8_t* self, re::Edx, uint8_t* texture) {
    NiCriticalSection& lock = At<NiCriticalSection>(self, kSourceDataLock);
    sys::Lock(lock);
    void* data = At<void*>(texture, 0x24);
    if (data == nullptr) {
        data = SourceTextureData_Create(texture);
    }
    sys::Unlock(lock);
    return data != nullptr ? 1 : 0;
}

// 0x8A9CD0 (thiscall, slot 68; `this` unused): CreateRenderedTextureRendererData
uint8_t __fastcall NiD3D10Renderer_CreateRenderedTextureRendererData(void*, re::Edx, uint8_t* texture, uint32_t format) {
    if (At<void*>(texture, 0x24) != nullptr) {
        return 1;
    }
    return RenderedTextureData_Create(texture, format) != nullptr ? 1 : 0;
}

// 0x8A9D00 (thiscall, slot 34): CreateDepthStencilRendererData(buffer, format, multisample) (a
// depth/stencil texture its size, D24_UNORM_S8_UINT by default; its data made)
uint8_t __fastcall NiD3D10Renderer_CreateDepthStencilRendererData(uint8_t* self, re::Edx, uint8_t* buffer,
                                                                  const void* format, uint32_t multisample) {
    if (Device(self) == nullptr) {
        Warning(Text(0x0099F358));
        return 0;
    }
    if (buffer == nullptr) {
        return 0;
    }
    uint32_t dxgi = format != nullptr ? PixelFormat_DetermineDXGIFormat(format) : 0x2D;
    uint32_t msaa = multisample & 0xFFFF;
    int32_t samples = int32_t(msaa >> 8);
    if (samples <= 1) {
        samples = 1;
    }
    void* texture = TextureCreator_CreateTexture2D(At<void*>(self, kManager450), At<uint32_t>(buffer, 8),
                                                   At<uint32_t>(buffer, 0xC), 1, 1, dxgi, uint32_t(samples),
                                                   msaa & 0xFF, 0, 0x40, 0, 0, nullptr);
    if (texture == nullptr) {
        Warning(Text(0x0099F2F8));
        return 0;
    }
    re::Ptr32<void> bufferSlot = buffer;
    void* data = DepthStencilBufferData_Create(texture, &bufferSlot);
    ComRelease(texture);
    if (data == nullptr) {
        Warning(Text(0x0099F270));
        return 0;
    }
    return 1;
}

// 0x8AA680 (thiscall): NiD3D10Renderer::HandleDisplayFrameResult (the occlusion callbacks told
// when it starts and ends; the device removed ones once, its reason reported)
uint8_t __fastcall NiD3D10Renderer_HandleDisplayFrameResult(uint8_t* self, re::Edx, long hr) {
    if (uint32_t(hr) == DXGI_STATUS_OCCLUDED) {
        if (At<uint8_t>(self, kOccluded) == 0) {
            if (!RunCallbacks<int>(self, kOcclusionCallbacks, 1)) {
                return 0;
            }
            At<uint8_t>(self, kOccluded) = 1;
            return 1;
        }
        return 1;
    }
    if (hr < 0) {
        if (uint32_t(hr) == DXGI_ERROR_DEVICE_REMOVED) {
            uint32_t reason = uint32_t(Com<long>(Device(self), kGetDeviceRemovedReason));
            if (reason == DXGI_ERROR_DEVICE_REMOVED) {
                Warning(Text(0x0099F520));
            } else if (reason == DXGI_ERROR_DEVICE_HUNG) {
                Warning(Text(0x0099F440));
            } else if (reason == DXGI_ERROR_DEVICE_RESET) {
                Warning(Text(0x0099F3A0));
            }
        } else {
            Error(0x10, Text(0x0099E584), hr);
        }
        if (At<uint8_t>(self, kDeviceRemoved) == 0) {
            if (!RunCallbacks<>(self, kDeviceRemovedCallbacks)) {
                return 0;
            }
            At<uint8_t>(self, kDeviceRemoved) = 1;
        }
        return 0;
    }
    if (At<uint8_t>(self, kOccluded) != 0) {
        if (!RunCallbacks<int>(self, kOcclusionCallbacks, 0)) {
            return 0;
        }
        At<uint8_t>(self, kOccluded) = 0;
    }
    return 1;
}

// 0x8AA800 (thiscall, slot 85): Do_ClearBuffer(rect, flags) (1 the targets with the background,
// 4 depth, 2 stencil; the rectangle unused)
void __fastcall NiD3D10Renderer_ClearBuffer(uint8_t* self, re::Edx, const void*, uint32_t flags) {
    if (Device(self) == nullptr) {
        Warning(Text(0x0099F5C8));
        return;
    }
    uint8_t mode = uint8_t(flags);
    if ((mode & 1) != 0) {
        for (uint32_t i = 0; i < 8; i++) {
            uint8_t* buffer = Virtual<uint8_t*>(CurrentGroup(self), 27, i);
            if (buffer == nullptr) {
                continue;
            }
            void* view = BufferData_GetView(At<void*>(buffer, 0x10));
            if (view != nullptr) {
                Com<long>(Device(self), kClearRenderTargetView, view, self + kBackground);
            }
        }
    }
    uint32_t clear = 0;
    if ((mode & 4) != 0) {
        clear = 1;   // D3D10_CLEAR_DEPTH
    }
    if ((mode & 2) != 0) {
        clear |= 2;   // D3D10_CLEAR_STENCIL
    }
    if (clear == 0) {
        return;
    }
    uint8_t* depthStencil = Virtual<uint8_t*>(CurrentGroup(self), 28);
    if (depthStencil == nullptr) {
        return;
    }
    void* view = BufferData_GetView(At<void*>(depthStencil, 0x10));
    if (view != nullptr) {
        Com<long>(Device(self), kClearDepthStencilView, view, clear, At<float>(self, kDepthClear),
                  uint32_t(At<uint8_t>(self, kStencilClear)));
    }
}

// 0x8AB150 (thiscall, slot 89): Do_BeginUsingRenderTargetGroup (its views bound, made current,
// cleared as asked)
uint8_t __fastcall NiD3D10Renderer_BeginUsingRenderTargetGroup(uint8_t* self, re::Edx, uint8_t* group,
                                                               uint32_t clearMode) {
    if (Device(self) == nullptr) {
        Warning(Text(0x0099F668));
        return 0;
    }
    void* views[8] = {};
    for (uint32_t i = 0; i < 8; i++) {
        uint8_t* buffer = Virtual<uint8_t*>(group, 27, i);
        if (buffer != nullptr) {
            views[i] = BufferData_GetView(At<void*>(buffer, 0x10));
        }
    }
    void* depthView = nullptr;
    uint8_t* depthStencil = Virtual<uint8_t*>(group, 28);
    if (depthStencil != nullptr) {
        depthView = BufferData_GetView(At<void*>(depthStencil, 0x10));
    }
    Com<long>(Device(self), kOMSetRenderTargets, 8u, views, depthView);
    At<uint8_t*>(self, kCurrentRenderTargetGroup) = group;
    Virtual<void>(self, 85, static_cast<void*>(nullptr), clearMode);
    return 1;
}

// 0x8AB220 (thiscall, slot 92): Do_EndBatch (the batch's reference let go)
void __fastcall NiD3D10Renderer_EndBatch(uint8_t* self, re::Edx) {
    re::Ptr32<void>& batch = At<void*>(self, 0x470);
    if (batch != nullptr) {
        ReleaseRef(batch);
        batch = nullptr;
    }
}

// 0x8AB620 (thiscall): NiD3D10Renderer::CreateRenderTargetGroupFromSwapChain(swap chain, with a
// depth/stencil buffer, its format) (a compatible depth/stencil buffer when the format can't be
// one)
void* __fastcall NiD3D10Renderer_CreateRenderTargetGroupFromSwapChain(uint8_t* self, re::Edx, void* swapChain,
                                                                      int withDepthStencil, uint32_t format) {
    if (Device(self) == nullptr) {
        Warning(Text(0x0099FAA0));
        return nullptr;
    }
    if (swapChain == nullptr) {
        Warning(Text(0x0099FA50));
        return nullptr;
    }
    re::Ptr32<void> backBuffer = nullptr;
    SwapChainBufferData_Create(swapChain, &backBuffer);
    void* held = backBuffer;
    AddRef(held);
    re::Ptr32<void> depthStencil = nullptr;
    if (re::Flag(withDepthStencil)) {
        if ((At<uint8_t>(self, kFormatSupport + 2 + int(format) * 4) & 1) != 0) {
            const void* pixelFormat = PixelFormat_GetPixelFormat(format);
            void* data = At<void*>(backBuffer, 0x10);
            uint32_t msaa = data != nullptr ? At<uint16_t>(data, 0x10) : 0x100;
            NiPointer_Assign(&depthStencil,
                             DepthStencilBuffer_Create(At<uint32_t>(backBuffer, 8), At<uint32_t>(backBuffer, 0xC),
                                                       self, pixelFormat, msaa));
        } else {
            NiPointer_Assign(&depthStencil, DepthStencilBuffer_CreateCompatible(backBuffer, self));
            if (depthStencil == nullptr) {
                Warning(Text(0x0099F9E0));
            }
        }
    }
    void* group = RenderTargetGroup_Create(backBuffer, self, depthStencil);
    ReleaseRef(depthStencil);
    ReleaseRef(held);
    return group;
}
RE_UNHOOKED("NiD3D10Renderer::CreateRenderTargetGroupFromSwapChain_8AB65D", 0x008AB65D);   // (0x8AB620's
                                                                                           //  body)

// 0x8AB920 (thiscall, slot 43): PrecacheTexture (a source texture's data made, under the source
// data lock)
uint8_t __fastcall NiD3D10Renderer_PrecacheTexture(uint8_t* self, re::Edx, uint8_t* texture, int, int) {
    if (texture == nullptr) {
        return 0;
    }
    NiCriticalSection& lock = At<NiCriticalSection>(self, kSourceDataLock);
    sys::Lock(lock);
    void* data = At<void*>(texture, 0x24);
    if (data == nullptr) {
        void* source = DynamicCast(NiSourceTexture_RTTI, texture);
        if (source != nullptr) {
            data = SourceTextureData_Create(source);
        }
    }
    sys::Unlock(lock);
    return data != nullptr ? 1 : 0;
}

// 0x8AB990 (thiscall): an object put at the head of the precache list (NiTPointerList::AddHead)
void __fastcall NiD3D10Renderer_AddToPrecacheListHead(uint8_t* self, re::Edx, void* object) {
    uint8_t* list = self + kPrecacheList;
    auto* node = Virtual<uint8_t*>(list, 1);
    At<void*>(node, 8) = object;
    At<void*>(node, 4) = nullptr;
    At<void*>(node, 0) = At<void*>(list, 4);
    uint8_t* head = At<uint8_t*>(list, 4);
    if (head != nullptr) {
        At<uint8_t*>(head, 4) = node;
        At<uint32_t>(list, 0xC)++;
        At<uint8_t*>(list, 4) = node;
        return;
    }
    At<uint32_t>(list, 0xC)++;
    At<uint8_t*>(list, 8) = node;
    At<uint8_t*>(list, 4) = node;
}

// 0x8AB9E0 (thiscall, slot 54): PurgeTexture (a rendered texture's buffers' data let go first: a
// cube map's six faces); the data deleted
uint8_t __fastcall NiD3D10Renderer_PurgeTexture(void*, re::Edx, uint8_t* texture) {
    void* data = At<void*>(texture, 0x24);
    if (data == nullptr) {
        return 1;
    }
    if (TextureData_IsRenderedTexture(data) != 0) {
        if (HasClassBit(texture, NiRenderedCubeMap_ClassBit)) {
            for (int face = 0; face < 6; face++) {
                ReleaseBufferRendererData(At<uint8_t*>(texture, 0x38 + face * 4));
            }
        } else {
            ReleaseBufferRendererData(Virtual<uint8_t*>(texture, 20));
        }
    }
    At<void*>(texture, 0x24) = nullptr;
    Virtual<void*>(data, 0, 1);
    return 1;
}

// 0x8AD6E0 (thiscall, slot 2): GetRTTI
const void* __fastcall NiD3D10Renderer_GetRTTI(void*, re::Edx) {
    return reinterpret_cast<const void*>(kRTTI);
}

// 0x8AD6F0 (thiscall, slot 79; `this` unused): a static flag set (0xBCB9A1; unknown)
void __fastcall NiD3D10Renderer_SetStaticFlag(void*, re::Edx, int value) {
    ms_bStaticFlagBCB9A1 = uint8_t(value);
}

// 0x8AD9C0 (thiscall): an object put at the precache list's tail
void __fastcall NiD3D10Renderer_AddToPrecacheList(uint8_t* self, re::Edx, re::Ptr32<void> object) {
    PointerList_AddTail(self + kPrecacheList, &object);
}

// 0x8AD9E0 (thiscall): NiD3D10Renderer::PurgePrecacheList (each object's slot 28 called, its
// node taken out; quirk kept: an object no longer in the list loops for ever)
void __fastcall NiD3D10Renderer_PurgePrecacheList(uint8_t* self, re::Edx) {
    uint8_t* list = self + kPrecacheList;
    while (At<uint32_t>(list, 0xC) != 0) {
        void* object = At<void*>(At<uint8_t*>(list, 4), 8);
        Virtual<void>(object, 28);
        re::Ptr32<void> position = nullptr;
        for (uint8_t* node = At<uint8_t*>(list, 4); node != nullptr; node = At<uint8_t*>(node, 0)) {
            if (object == At<void*>(node, 8)) {
                position = node;
                break;
            }
        }
        if (position != nullptr) {
            PointerList_RemovePos(list, &position);
        }
    }
}

#define C42E_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C42E_REVERSED("NiD3D10Renderer::CheckFormatSupport", 0x008A8F80, NiD3D10Renderer_CheckFormatSupport);
C42E_REVERSED("NiD3D10Renderer::CreateManagers", 0x008A9000, NiD3D10Renderer_CreateManagers);
C42E_REVERSED("NiD3D10Renderer::GetFormatSupport", 0x008A90E0, NiD3D10Renderer_GetFormatSupport);
C42E_REVERSED("NiD3D10Renderer::GetResourceManager", 0x008A9100, NiD3D10Renderer_GetResourceManager);
C42E_REVERSED("NiD3D10Renderer::GetRenderStateManager", 0x008A9110, NiD3D10Renderer_GetRenderStateManager);
C42E_REVERSED("NiD3D10Renderer::GetTextureCreator", 0x008A9120, NiD3D10Renderer_GetTextureCreator);
C42E_REVERSED("NiD3D10Renderer::GetShaderResourceManager", 0x008A9130, NiD3D10Renderer_GetShaderResourceManager);
C42E_REVERSED("NiD3D10Renderer::GetD3D10Device", 0x008A9140, NiD3D10Renderer_GetD3D10Device);
C42E_REVERSED("NiD3D10Renderer::GetFlags", 0x008A9150, NiD3D10Renderer_GetFlags);
C42E_REVERSED("NiD3D10Renderer::SetDepthClear", 0x008A9160, NiD3D10Renderer_SetDepthClear);
C42E_REVERSED("NiD3D10Renderer::GetDepthClear", 0x008A9170, NiD3D10Renderer_GetDepthClear);
C42E_REVERSED("NiD3D10Renderer::SetBackgroundColor_RGB", 0x008A9180, NiD3D10Renderer_SetBackgroundColorRGB);
C42E_REVERSED("NiD3D10Renderer::SetBackgroundColor_RGBA", 0x008A91B0, NiD3D10Renderer_SetBackgroundColorRGBA);
C42E_REVERSED("NiD3D10Renderer::GetBackgroundColor", 0x008A91E0, NiD3D10Renderer_GetBackgroundColor);
C42E_REVERSED("NiD3D10Renderer::SetStencilClear", 0x008A9210, NiD3D10Renderer_SetStencilClear);
C42E_REVERSED("NiD3D10Renderer::GetStencilClear", 0x008A9220, NiD3D10Renderer_GetStencilClear);
C42E_REVERSED("NiD3D10Renderer::GetCurrentRenderTargetGroup", 0x008A9230,
              NiD3D10Renderer_GetCurrentRenderTargetGroup);
C42E_REVERSED("NiD3D10Renderer::Slot47", 0x008A9240, NiD3D10Renderer_Slot47);
C42E_REVERSED("NiD3D10Renderer::PurgeAllTextures", 0x008A9260, NiD3D10Renderer_PurgeAllTextures);
C42E_REVERSED("NiD3D10Renderer::GetLeftRightSwap", 0x008A9270, NiD3D10Renderer_GetLeftRightSwap);
C42E_REVERSED("NiD3D10Renderer::SetLeftRightSwap", 0x008A9280, NiD3D10Renderer_SetLeftRightSwap);
C42E_REVERSED("NiD3D10Renderer::GetMaxFogValue", 0x008A92A0, NiD3D10Renderer_GetMaxFogValue);
C42E_REVERSED("NiD3D10Renderer::SetMaxFogValue", 0x008A92B0, NiD3D10Renderer_SetMaxFogValue);
C42E_REVERSED("NiD3D10Renderer::GetMaxFogFactor", 0x008A92F0, NiD3D10Renderer_GetMaxFogFactor);
C42E_REVERSED("NiD3D10Renderer::GetRenderer", 0x008A9300, NiD3D10Renderer_GetRenderer);
C42E_REVERSED("NiD3D10Renderer::CreateSourceCubeMapRendererData", 0x008A9310,
              NiD3D10Renderer_CreateSourceCubeMapRendererData);
C42E_REVERSED("NiD3D10Renderer::CreateRenderedCubeMapRendererData", 0x008A9320,
              NiD3D10Renderer_CreateRenderedCubeMapRendererData);
C42E_REVERSED("NiD3D10Renderer::GetFragmentShader", 0x008A9340, NiD3D10Renderer_GetFragmentShader);
C42E_REVERSED("NiD3D10Renderer::GetShadowWriteShader", 0x008A9360, NiD3D10Renderer_GetShadowWriteShader);
C42E_REVERSED("NiD3D10Renderer::GetProjectionMatrix", 0x008A9380, NiD3D10Renderer_GetProjectionMatrix);
C42E_REVERSED("NiD3D10Renderer::GetViewMatrix", 0x008A9390, NiD3D10Renderer_GetViewMatrix);
C42E_REVERSED("NiD3D10Renderer::GetInverseViewMatrix", 0x008A93A0, NiD3D10Renderer_GetInverseViewMatrix);
C42E_REVERSED("NiD3D10Renderer::GetWorldMatrix", 0x008A93B0, NiD3D10Renderer_GetWorldMatrix);
C42E_REVERSED("NiD3D10Renderer::GetCameraNearAndFar", 0x008A93C0, NiD3D10Renderer_GetCameraNearAndFar);
C42E_REVERSED("NiD3D10Renderer::Do_GetCameraData", 0x008A93E0, NiD3D10Renderer_GetCameraData);
C42E_REVERSED("NiD3D10Renderer::SetModelTransform", 0x008A94A0, NiD3D10Renderer_SetModelTransform);
C42E_REVERSED("NiD3D10Renderer::D3D10CreateBlob", 0x008A94C0, NiD3D10Renderer_D3D10CreateBlob);
C42E_REVERSED("NiD3D10Renderer::ReleaseD3D10", 0x008A95C0, NiD3D10Renderer_ReleaseD3D10);
C42E_REVERSED("NiD3D10Renderer::CreateSwapChain", 0x008A96C0, NiD3D10Renderer_CreateSwapChain);
C42E_REVERSED("NiD3D10Renderer::GetDefaultDepthStencilBuffer", 0x008A9AF0,
              NiD3D10Renderer_GetDefaultDepthStencilBuffer);
C42E_REVERSED("NiD3D10Renderer::GetDefaultBackBuffer", 0x008A9B10, NiD3D10Renderer_GetDefaultBackBuffer);
C42E_REVERSED("NiD3D10Renderer::FindClosestDepthStencilFormat", 0x008A9B30,
              NiD3D10Renderer_FindClosestDepthStencilFormat);
C42E_REVERSED("NiD3D10Renderer::PurgeGeometryData", 0x008A9C20, NiD3D10Renderer_PurgeGeometryData);
C42E_REVERSED("NiD3D10Renderer::PurgeSkinInstance", 0x008A9C40, NiD3D10Renderer_PurgeSkinInstance);
C42E_REVERSED("NiD3D10Renderer::CreateSourceTextureRendererData", 0x008A9C80,
              NiD3D10Renderer_CreateSourceTextureRendererData);
C42E_REVERSED("NiD3D10Renderer::CreateRenderedTextureRendererData", 0x008A9CD0,
              NiD3D10Renderer_CreateRenderedTextureRendererData);
C42E_REVERSED("NiD3D10Renderer::CreateDepthStencilRendererData", 0x008A9D00,
              NiD3D10Renderer_CreateDepthStencilRendererData);
C42E_REVERSED("NiD3D10Renderer::HandleDisplayFrameResult", 0x008AA680, NiD3D10Renderer_HandleDisplayFrameResult);
C42E_REVERSED("NiD3D10Renderer::Do_ClearBuffer", 0x008AA800, NiD3D10Renderer_ClearBuffer);
C42E_REVERSED("NiD3D10Renderer::Do_BeginUsingRenderTargetGroup", 0x008AB150,
              NiD3D10Renderer_BeginUsingRenderTargetGroup);
C42E_REVERSED("NiD3D10Renderer::Do_EndBatch", 0x008AB220, NiD3D10Renderer_EndBatch);
C42E_REVERSED("NiD3D10Renderer::LoadD3D10", 0x008AB270, NiD3D10Renderer_LoadD3D10);
C42E_REVERSED("NiD3D10Renderer::CreateRenderTargetGroupFromSwapChain", 0x008AB620,
              NiD3D10Renderer_CreateRenderTargetGroupFromSwapChain);
C42E_REVERSED("NiD3D10Renderer::PrecacheTexture", 0x008AB920, NiD3D10Renderer_PrecacheTexture);
C42E_REVERSED("NiD3D10Renderer::AddToPrecacheListHead", 0x008AB990, NiD3D10Renderer_AddToPrecacheListHead);
C42E_REVERSED("NiD3D10Renderer::PurgeTexture", 0x008AB9E0, NiD3D10Renderer_PurgeTexture);
C42E_REVERSED("NiD3D10Renderer::GetRTTI", 0x008AD6E0, NiD3D10Renderer_GetRTTI);
C42E_REVERSED("NiD3D10Renderer::SetStaticFlag", 0x008AD6F0, NiD3D10Renderer_SetStaticFlag);
C42E_REVERSED("NiD3D10Renderer::EnsureD3D10Loaded", 0x008AD700, NiD3D10Renderer_EnsureD3D10Loaded);
C42E_REVERSED("NiD3D10Renderer::CreateDevice", 0x008AD8F0, NiD3D10Renderer_CreateDevice);
C42E_REVERSED("NiD3D10Renderer::AddToPrecacheList", 0x008AD9C0, NiD3D10Renderer_AddToPrecacheList);
C42E_REVERSED("NiD3D10Renderer::PurgePrecacheList", 0x008AD9E0, NiD3D10Renderer_PurgePrecacheList);

} // namespace ni::c42
