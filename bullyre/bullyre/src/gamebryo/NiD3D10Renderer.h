#pragma once

// NiD3D10Renderer as Bully's exe has it (Gamebryo 2.5's Direct3D 10 renderer; Bully renders with
// DX9 and only reaches this through NiApplication's CreateD3D10Renderer 0x82EF00, which the game
// never asks for: cold code, written exactly all the same). Vtable 0x99FC00 (103 slots: NiRenderer's
// 100, then three no-ops), NiRTTI 0x20E8F30 "NiD3D10Renderer", 0x4C0 bytes (Create 0x8AF800,
// constructor 0x8AD280). What the code of the chunk-42 files reads of it, by offset; NiRenderer's own
// part (0..0xA4) is src/gamebryo/NiRenderer.cpp's (+0x14 / +0x18 its default materials, +0x20..+0x2C
// floats Initialize sets, +0x70 the source data lock: NiCriticalSection, owner +0x88, count +0x8C).
//
// Its slots are NiRenderer's (names in src/gamebryo/NiDX9Renderer.h). Those this chunk's code
// names and the DX9 header doesn't (or names otherwise): 34 CreateDepthStencilRendererData (the DX9
// header's "PrecacheTexture": DX9's slot 34 takes the same three arguments, likely this too), 43
// PrecacheTexture (a guess), 46 PrecacheGeometry, 47 handed on to 0x8B4380 (unknown), 55
// PurgeAllTextures (a guess), 60/61 Get/SetLeftRightSwap, 64 GetErrorShader, 65 the default
// material's set-up (Initialize), 75 GetFragmentShader / 78 GetShadowWriteShader (they make an
// NiD3D10FragmentShader / NiD3D10ShadowWriteShader), 79 a static flag's set (0xBCB9A1; unknown).
//
// The Direct3D 10 library is loaded at run time ("D3D10.dll": 0x20E8E9C, its 26 entry points from
// 0x20E8EA4, guarded by the critical section 0x20E8F0C). D3D10 / DXGI calls are COM calls by slot.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::d3d10 {

using dx9::Com;
using sys::At;
using sys::Import;
using sys::NiCriticalSection;
using sys::Virtual;

// ---- the renderer's fields (offsets)
enum Field : int {
    kRendererLock = 0x30,           // NiRenderer's
    kSourceDataLock = 0x70,
    kCreationParameters = 0xA4,     // CreationParameters (0x58: +0x0 the adapter, +0x4 the output,
                                    //  +0x8 the driver type, +0xC the device flags, +0x10 make a
                                    //  swap chain, +0x11 a depth/stencil buffer with it, +0x12
                                    //  associate the window, +0x14 its DXGI_MWA flags, +0x18 the
                                    //  depth format, +0x1C the DXGI_SWAP_CHAIN_DESC (0x3C), +0x48
                                    //  the window)
    kSystemDesc = 0xFC,             // NiD3D10SystemDesc* (a reference)
    kDevice = 0x100,                // ID3D10Device*
    kAdapterIndex = 0x104,          // (-1: none)
    kInitialized = 0x108,           // bool
    kFormatSupport = 0x10C,         // D3D10_FORMAT_SUPPORT by DXGI_FORMAT (+format * 4)
    kDefaultStates = 0x270,         // three default state objects (Initialize)
    kDefaultRenderTargetGroup = 0x27C,   // (a reference)
    kCurrentRenderTargetGroup = 0x280,
    kBuffersInUse = 0x284,          // NiTPointerList (vtable 0x95127C)
    kBackground = 0x294,            // NiColorA
    kDepthClear = 0x2A4,            // float
    kStencilClear = 0x2A8,          // byte
    kLeftRightSwap = 0x2A9,         // bool
    kFrustum = 0x2AC,               // NiFrustum (left, right, top, bottom, near, far, ortho)
    kPort = 0x2C8,                  // NiRect<float>
    kPrecacheMap = 0x2D8,           // NiTPointerMap (37 buckets)
    kMaxFogFactor = 0x2E8,          // 1 / max fog value - 1
    kMaxFogValue = 0x2EC,
    kPrecacheList = 0x2F0,          // NiTPointerList (vtable 0x952E10; head +0x2F4, tail +0x2F8,
                                    //  count +0x2FC)
    kView = 0x340,                  // D3DXMATRIX
    kInvView = 0x380,
    kProjection = 0x3C0,
    kWorld = 0x400,
    kResourceManager = 0x444,       // the managers 0x8A9000 makes
    kManager448 = 0x448,
    kRenderStateManager = 0x44C,
    kManager450 = 0x450,
    kShaderResourceManager = 0x454,
    kOccluded = 0x474,              // bool
    kDeviceRemoved = 0x475,         // bool
    kOcclusionCallbacks = 0x478,    // NiTPrimitiveArray (and their data at 0x488)
    kDeviceRemovedCallbacks = 0x498,   // (and their data at 0x4A8)
};
constexpr uint32_t kRendererSize = 0x4C0;
constexpr uintptr_t kVtable = 0x0099FC00;
constexpr uintptr_t kRTTI = 0x020E8F30;

// the D3D10 library (its load, entry points and lock)
GAME_VAR(NiCriticalSection, ms_kD3D10LibraryLock, 0x020E8F0C);
GAME_VAR(void*, ms_hD3D10, 0x020E8E9C);
GAME_ARRAY(void*, ms_apfnD3D10, 0x020E8EA4);   // [26]: D3D10CreateBlob, CreateDevice, ... DisassembleEffect
enum D3D10Entry : int {
    kD3D10CreateBlob = 0,
    kD3D10CreateDevice = 1,
    kD3D10GetGeometryShaderProfile = 17,
    kD3D10GetPixelShaderProfile = 18,
    kD3D10GetVertexShaderProfile = 19,
};

// the error reporting (src/gamebryo/NiD3D10Error.cpp)
GAME_FN(void, __cdecl, Warning, 0x008AF9B0, const char* format, ...);
GAME_FN(void, __cdecl, Error, 0x008AFAC0, uint32_t code, const char* format, ...);

// the game's strings
inline const char* Text(uintptr_t address) {
    return reinterpret_cast<const char*>(address);
}

inline void ComRelease(void* object) {
    if (object != nullptr) {
        Com<long>(object, 2);
    }
}

// an NiPointer let go as the library inlines it (through the import; DeleteThis when it was the
// last)
inline void ReleaseRef(void* object) {
    if (object != nullptr && sys::Decrement(&At<uint32_t>(object, 4)) == 0) {
        Virtual<void>(object, 1);
    }
}
inline void AddRef(void* object) {
    if (object != nullptr) {
        sys::Increment(&At<uint32_t>(object, 4));
    }
}
// assigned as the library's NiPointer::operator= does here: the old one let go first
inline void AssignRef(re::Ptr32<void>& slot, void* value) {
    if (slot != value) {
        ReleaseRef(slot);
        slot = value;
        AddRef(value);
    }
}

// an object's RTTI (slot 2) has the class bit (NiIsKindOf by bit)
inline bool HasClassBit(const void* object, uint32_t bit) {
    const uint8_t* rtti = Virtual<const uint8_t*>(object, 2);
    return (At<uint32_t>(rtti, 0xC + int(bit >> 5) * 4) & (1u << (bit & 0x1F))) != 0;
}

} // namespace ni::d3d10
