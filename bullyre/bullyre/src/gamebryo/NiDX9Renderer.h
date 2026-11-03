#pragma once

// NiDX9Renderer as Bully's exe has it (Gamebryo 2.5: SetDefaultProgramCache, the batch
// rendering slots), vtable 0x99B5A8 (105 slots: NiRenderer's 100, then 100 DrawPrimitive, 101
// DrawSkinnedPrimitive, 102..104), constructor 0x87FAA0, 0x930 bytes (Bully's derived
// BullyNiDX9Renderer, vtable 0x8FEFE4, only overrides a few slots). What the code here and the
// game's code read of its layout, by offset (static_asserts where the constructor and the code
// agree); NiRenderer's own part (0..0xA4) is src/gamebryo/NiRenderer.cpp's.
//
// NiRenderer's virtual slots (2.5's order, from what each one does; NiObject's are 0..17):
//   18 GetDriverInfo 19 GetFlags 20 SetDepthClear 21 GetDepthClear 22/23 SetBackgroundColor
//   (NiColorA / NiColor) 24 GetBackgroundColor 25 SetStencilClear 26 GetStencilClear
//   27 ValidateRenderTargetGroup 28 IsDepthBufferCompatible 29 GetDefaultRenderTargetGroup
//   30 GetCurrentRenderTargetGroup 31 GetDefaultDepthStencilBuffer 32 GetDefaultBackBuffer
//   34 PrecacheTexture 36 FindClosestPixelFormat 37 FindClosestDepthStencilFormat
//   39 GetMaxBuffersPerRenderTargetGroup 40 GetIndependentBufferBitDepths
//   41 UseLegacyPipelineAsDefaultMaterial 42 ApplyDefaultMaterial 44 SetMipmapSkipLevel
//   45 GetMipmapSkipLevel 48 PurgeGeometryData 49 PurgeMaterial 50 PurgeEffect
//   51 PurgeScreenTextureRendererData (NiScreenTexture's) 52 PurgeSkinPartitionRendererData
//   53 PurgeSkinInstance (NiGeometry's, ~NiSkinInstance's) 54 PurgeTexture 56 TakeScreenShot
//   57 SaveScreenShot 58 FastCopy 59 Copy
//   62/63 Get/SetMaxFogValue 67 CreateSourceTextureRendererData 69 CreateSourceCubeMapRendererData
//   76/77 SetDefaultProgramCache 80/81 Do_LockRenderer / Do_UnlockRenderer 82 Do_BeginFrame
//   83 Do_EndFrame 84 Do_DisplayFrame 85 Do_ClearBuffer 86 Do_SetCameraData
//   87 Do_SetScreenSpaceCameraData 88 Do_GetCameraData 89 Do_BeginUsingRenderTargetGroup
//   90 Do_EndUsingRenderTargetGroup 91 Do_BeginBatch 92 Do_EndBatch 93 Do_BatchRenderShape
//   94 Do_BatchRenderStrips 95 Do_RenderShape 96 Do_RenderTristrips 97 Do_RenderPoints
//   98 Do_RenderLines 99 Do_RenderScreenTexture
// (33, 35, 43, 46, 47, 55: not this chunk's; their names a guess or unknown.)

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::dx9 {

using ni::sys::At;
using ni::sys::Decrement;
using ni::sys::Import;
using ni::sys::Increment;
using ni::sys::Lock;
using ni::sys::NiCriticalSection;
using ni::sys::Unlock;
using ni::sys::Virtual;

// a COM call by vtable slot (stdcall, the interface first)
template <typename R = long, typename... A>
inline R Com(const void* object, int slot, A... args) {
    return re::ComCall<R>(object, slot, args...);
}

// ---- D3DPRESENT_PARAMETERS: the game keeps them in its layout (0x38 bytes: 14 dwords, the
// device window a 4-byte handle at +0x1C). Direct3D's on x64 is 0x40 (the handle 8 bytes at
// +0x20, the rest after it): the calls below take the game's and convert both ways (Direct3D
// corrects the back buffer count, size and format in place)
struct NativePresentParams {
    uint32_t backBufferWidth, backBufferHeight, backBufferFormat, backBufferCount;
    uint32_t multiSampleType, multiSampleQuality, swapEffect;
    void* deviceWindow;
    int32_t windowed, enableAutoDepthStencil;
    uint32_t autoDepthStencilFormat, flags, refreshRate, presentationInterval;
};
static_assert(sizeof(NativePresentParams) == (sizeof(void*) == 8 ? 0x40 : 0x38));

// a 32-bit window handle as the system's (sign-extended, as LongToHandle)
inline void* HandleOf(uint32_t handle) {
    return reinterpret_cast<void*>(intptr_t(int32_t(handle)));
}

inline void ToNative(const uint32_t* game, NativePresentParams* native) {
    std::memcpy(native, game, 0x1C);
    native->deviceWindow = HandleOf(game[7]);
    std::memcpy(&native->windowed, game + 8, 0x18);
}
inline void FromNative(const NativePresentParams* native, uint32_t* game) {
    std::memcpy(game, native, 0x1C);
    game[7] = uint32_t(reinterpret_cast<uintptr_t>(native->deviceWindow));
    std::memcpy(game + 8, &native->windowed, 0x18);
}

// IDirect3D9::CreateDevice (slot 16) with the game's present parameters
inline long CreateDevice9(void* d3d, uint32_t adapter, uint32_t devType, uint32_t focusWindow, uint32_t behavior,
                          void* gameParams, re::Ptr32<void>* device) {
    NativePresentParams native;
    ToNative(static_cast<uint32_t*>(gameParams), &native);
    long hr = Com<long>(d3d, 16, adapter, devType, HandleOf(focusWindow), behavior, static_cast<void*>(&native), device);
    FromNative(&native, static_cast<uint32_t*>(gameParams));
    return hr;
}

// IDirect3DDevice9::Reset (slot 16) with the game's present parameters
inline long ResetDevice9(void* device, void* gameParams) {
    NativePresentParams native;
    ToNative(static_cast<uint32_t*>(gameParams), &native);
    long hr = Com<long>(device, 16, static_cast<void*>(&native));
    FromNative(&native, static_cast<uint32_t*>(gameParams));
    return hr;
}

// ---- the renderer's fields (offsets)
enum Field : int {
    kRendererLock = 0x30,        // NiRenderer's three locks: +0x30 the renderer's, +0x50 the
    kSourceDataLock = 0x50,      //  source data's, +0x70 the precache's (NiCriticalSection)
    kPrecacheLock = 0x70,
    kFrameState = 0x94,
    kDevice = 0xA4,              // IDirect3DDevice9*
    kCaps = 0xA8,                // D3DCAPS9 (0x130)
    kDeviceWindow = 0x1D8,       // HWND
    kFocusWindow = 0x1DC,
    kDriverDesc = 0x1E0,         // char[0x200] (GetDriverInfo)
    kAdapter = 0x3E0,
    kDevType = 0x3E4,            // D3DDEVTYPE (1 HAL)
    kBehaviorFlags = 0x3E8,      // D3DCREATE_*
    kAdapterFormat = 0x3EC,      // D3DFORMAT
    kSWVertexCapable = 0x3F0,    // bool
    kSWVertexSwitchable = 0x3F1, // bool
    kAdapterDesc = 0x3F4,        // NiDX9AdapterDesc*
    kDeviceDesc = 0x3F8,         // NiDX9DeviceDesc*
    kBackground = 0x3FC,         // D3DCOLOR
    kZClear = 0x400,
    kStencilClear = 0x404,
    kRendFlags = 0x408,          // CAPS_*: 2 hardware skinning, 4 non-power-of-2 conditional, 8 any
    kBehavior = 0x40C,           // char[0x20] (" HWVP"...)
    kHWBones = 0x4E0,            // the vertex blend matrices
    kMaxStreams = 0x4E4,
    kPixelShaderVersion = 0x4E8,
    kVertexShaderVersion = 0x4EC,
    kMipmapCubeMaps = 0x4F0,     // bool
    kDynamicTexturesCapable = 0x4F1,   // bool
    kResetCounter = 0x4F4,
    kDeviceLost = 0x4F8,         // bool
    kTextureFormats = 0x4FC,     // NiPixelFormat* [4 usages][22 formats]
    kDefaultTextureFormat = 0x65C,   // NiPixelFormat* [4]
    kDefaultTextureData = 0x66C,     // NiPixelDataPtr [4]
    kReplacementDataFormat = 0x67C,
    kDefaultRenderTargetGroup = 0x680,
    kCurrentRenderTargetGroup = 0x684,
    kCurrentOnscreenTarget = 0x688,
    kMaxRenderTargets = 0x69C,
    kIndependentBitDepths = 0x6A0,     // bool
    kMRTPostPixelShaderBlending = 0x6A1,   // bool
    kManager6A8 = 0x6A8,           // NiGeometryGroupManager* (+0x6AC the static group, +0x6B0 the
                                   //  dynamic one; slot 6 purges a screen texture's data, 7 a
                                   //  geometry's)
    kRenderState = 0x6B4,          // NiDX9RenderState*
    kVBManager = 0x6B8,            // NiDX9VertexBufferManager*
    kIBManager = 0x6BC,
    kTextureManager = 0x6C0,
    kLightManager = 0x6C4,
    kClipperImage = 0x6F8,         // NiSourceTexturePtr (+0x42C the precache map, +0x43C..+0x44C the
                                   //  batch: list head / tail, property / effect state, shader;
                                   //  +0x70C the D3D shaders: NiTPointerList)
    kBuffersInUse = 0x6FC,         // NiTPointerList (vtable 0x95127C)
    kWorld = 0x750,                // D3DMATRIX
    kView = 0x790,
    kProjection = 0x7D0,
    kInvView = 0x810,
    kLegacyDefaultMaterial = 0x8A4,
    kFrustum = 0x8F8,              // NiFrustum (left, right, top, bottom, near, far, ortho)
    kPort = 0x914,                 // NiRect<float> (left, right, top, bottom)
};
constexpr uint32_t kRendererSize = 0x930;

// the statics
GAME_VAR(void*, ms_pkRenderer, 0x020E2380);                // NiRenderer::ms_pkRenderer
GAME_VAR_NATIVE(void*, ms_hDXLib, 0x020E6D00);             // D3D9.DLL (x64: a DLL above 4 GB)
GAME_VAR(void*, ms_pkD3D9, 0x020E6D04);                    // IDirect3D9*
GAME_VAR_NATIVE(void*, ms_pfnD3DCreate9, 0x020E6D08);      // Direct3DCreate9 (x64: in that DLL)
GAME_VAR(void*, ms_pkSystemDesc, 0x020E6D10);              // NiDX9SystemDesc*
GAME_VAR(NiCriticalSection, ms_kD3DCriticalSection, 0x020E6D4C);
GAME_ARRAY(uint8_t, ms_aucD3DFormatSizes, 0x020E6C20);     // bits per pixel by D3DFORMAT (0x76)
GAME_VAR(uint32_t, ms_uiSkipLevels, 0x020E7300);           // the mipmap skip level
GAME_ARRAY(char, ms_acErrorString, 0x020E2280);            // NiRenderer's last error (0x100)

// the NiRTTI class bits the code tests (NiIsKindOf by bit)
GAME_VAR(uint32_t, NiLight_ClassBit, 0x020E2C70);
GAME_VAR(uint32_t, NiDX92DBufferData_ClassBit, 0x020E70AC);   // (the buffers' renderer data)
GAME_VAR(uint32_t, NiDX9DepthStencilBufferData_ClassBit, 0x020E7148);
GAME_VAR(uint32_t, NiDX9OnscreenBufferData_ClassBit, 0x020E7238);
GAME_VAR(uint32_t, NiTriShapeData_ClassBit, 0x020E3138);

// an object's RTTI (slot 2) has the class bit (the library's NiIsKindOf by bit)
inline bool HasClassBit(const void* object, uint32_t bit) {
    const uint8_t* rtti = Virtual<const uint8_t*>(object, 2);
    return (At<uint32_t>(rtti, 0xC + int(bit >> 5) * 4) & (1u << (bit & 0x1F))) != 0;
}

// the NiPointer release / hold the code inlines
inline void ReleaseRef(void* object) {
    if (object != nullptr && Decrement(static_cast<uint8_t*>(object) + 4) == 0) {
        Virtual<void>(object, 1);
    }
}
inline void AddRefRef(void* object) {
    if (object != nullptr) {
        Increment(static_cast<uint8_t*>(object) + 4);
    }
}
inline void AssignRef(re::Ptr32<void>& slot, void* value) {
    if (slot != value) {
        ReleaseRef(slot);
        slot = value;
        AddRefRef(value);
    }
}

// NiOutputDebugString (a no-op in this build: 0x49A650 is a lone ret)
GAME_FN(void, __cdecl, NiOutputDebugString, 0x0049A650, const char* text, ...);
// NiRenderer::SetLastErrorString (0x401790: strncpy_s into ms_acErrorString)
GAME_FN(void, __cdecl, NiRenderer_SetLastErrorString, 0x00401790, const char* text);

// ---- IDirect3D9 / IDirect3DDevice9 / IDirect3DSurface9 slots
enum D3DSlot : int {
    kAddRef = 1,
    kRelease = 2,
    kD3DGetAdapterIdentifier = 5,
    kD3DCheckDeviceMultiSampleType = 11,
    kDevTestCooperativeLevel = 3,
    kDevGetDeviceCaps = 7,
    kDevPresent = 17,
    kDevGetBackBuffer = 18,
    kDevStretchRect = 34,
    kDevSetRenderTarget = 37,
    kDevSetDepthStencilSurface = 39,
    kDevClear = 43,
    kDevSetTransform = 44,
    kDevSetViewport = 47,
    kDevSetRenderState = 57,
    kDevDrawPrimitive = 81,
    kDevDrawIndexedPrimitive = 82,
    kDevSetVertexShaderConstantF = 94,
    kDevSetStreamSource = 100,
    kDevSetIndices = 104,
    kSurfGetDesc = 12,
};

} // namespace ni::dx9
