#pragma once

// What the files of Gamebryo chunk 43 (the D3D10 renderer's helpers and data classes,
// 0x8B0A50..0x8BB360) share: COM calls by slot (the project has no D3D10 headers: the slot
// numbers of ID3D10Device and the others below), the renderer's warning / error reporters, the
// NiPointer handling the library inlines. Everything in ni::c43 so nothing clashes with the other
// Gamebryo files (NiDX9Renderer.h's and NiSystem.h's names are used read-only).
//
// Bully renders with DX9: the D3D10 renderer is only reached through its vtables and
// NiApplication's CreateD3D10Renderer ("-d3d10"), which the game never asks for. Cold code,
// written exactly all the same.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::c43 {

using ni::dx9::Com;
using ni::sys::At;
using ni::sys::Decrement;
using ni::sys::Import;
using ni::sys::Increment;
using ni::sys::Virtual;

// ---- COM slots (IUnknown, ID3D10Device, ID3D10Resource / views / blobs)
enum : int {
    kQueryInterface = 0,
    kAddRef = 1,
    kRelease = 2,
};
// ID3D10Device
enum D3D10DeviceSlot : int {
    kVSSetConstantBuffers = 3,
    kPSSetShaderResources = 4,
    kPSSetShader = 5,
    kPSSetSamplers = 6,
    kVSSetShader = 7,
    kDrawIndexed = 8,
    kDraw = 9,
    kPSSetConstantBuffers = 10,
    kIASetInputLayout = 11,
    kIASetVertexBuffers = 12,
    kIASetIndexBuffer = 13,
    kGSSetConstantBuffers = 16,
    kGSSetShader = 17,
    kIASetPrimitiveTopology = 18,
    kVSSetShaderResources = 19,
    kVSSetSamplers = 20,
    kGSSetShaderResources = 22,
    kGSSetSamplers = 23,
    kOMSetRenderTargets = 24,
    kOMSetBlendState = 25,
    kOMSetDepthStencilState = 26,
    kRSSetState = 29,
    kCopyResource = 33,
    kUpdateSubresource = 34,
    kClearRenderTargetView = 35,
    kClearDepthStencilView = 36,
    kCreateBuffer = 71,
    kCreateTexture1D = 72,
    kCreateTexture2D = 73,
    kCreateTexture3D = 74,
    kCreateShaderResourceView = 75,
    kCreateRenderTargetView = 76,
    kCreateDepthStencilView = 77,
    kCreateInputLayout = 78,
    kCreateVertexShader = 79,
    kCreateGeometryShader = 80,
    kCreatePixelShader = 82,
};
// ID3D10DeviceChild (ID3D10Resource, the views, the shaders): 3 GetDevice; ID3D10Resource: 7
// GetType; ID3D10Buffer: 10 Map, 11 Unmap, 12 GetDesc; ID3D10Texture1D/2D/3D: 10 Map, 11 Unmap,
// 12 GetDesc; the views: 7 GetResource, 8 GetDesc
enum : int {
    kGetDevice = 3,
    kResourceGetType = 7,
    kMap = 10,
    kUnmap = 11,
    kResourceGetDesc = 12,
    kViewGetResource = 7,
    kViewGetDesc = 8,
};
// ID3D10Blob: 3 GetBufferPointer, 4 GetBufferSize
enum : int {
    kGetBufferPointer = 3,
    kGetBufferSize = 4,
};

inline void ComRelease(void* object) {
    if (object != nullptr) {
        Com<long>(object, kRelease);
    }
}
inline void ComAddRef(void* object) {
    if (object != nullptr) {
        Com<long>(object, kAddRef);
    }
}
// a COM pointer slot assigned as the library does it: the old released, the new held (only
// when they differ)
inline void ComAssign(re::Ptr32<void>& slot, void* value) {
    if (slot != value) {
        ComRelease(slot);
        slot = value;
        ComAddRef(value);
    }
}

// ---- the renderer's reporters (NiD3D10Renderer's, chunk 42's): a warning (printf style, to the
// debug output, a no-op in this build's library) and an error code with a message
GAME_FN(void, __cdecl, D3D10Warning, 0x008AF9B0, const char* format, ...);
GAME_FN(void, __cdecl, D3D10Error, 0x008AFAC0, uint32_t code, const char* format, ...);

// ---- NiPointer as the library inlines it (InterlockedIncrement / Decrement through the imports,
// DeleteThis, slot 1, when it was the last)
inline void ReleaseRef(void* object) {
    if (object != nullptr && Decrement(&At<uint32_t>(object, 4)) == 0) {
        Virtual<void>(object, 1);
    }
}
inline void AddRefRef(void* object) {
    if (object != nullptr) {
        Increment(&At<uint32_t>(object, 4));
    }
}
// NiPointer::operator= (the new one held first only when it differs; the old let go)
inline void AssignRef(re::Ptr32<void>& slot, void* value) {
    if (slot != value) {
        ReleaseRef(slot);
        slot = value;
        AddRefRef(value);
    }
}

// the statics every NiRefObject counts in, and the empty NiFixedString handle
GAME_VAR(uint32_t, ms_uiObjects, 0x020E2168);
constexpr uintptr_t kRefObjectVtable = 0x008FEFD4;

// the D3D10 renderer (NiRenderer::ms_pkRenderer: NiD3D10Renderer when it's the one made)
inline uint8_t* Renderer() {
    return static_cast<uint8_t*>(ni::dx9::ms_pkRenderer);
}
// the renderer's fields this chunk reads (chunk 42 writes the class)
enum RendererField : int {
    kRendererDevice = 0x100,         // ID3D10Device*
    kRendererDeviceState = 0x444,    // NiD3D10DeviceState*
    kRendererPacker = 0x448,         // NiD3D10GeometryPacker*
    kRendererStateManager = 0x44C,   // NiD3D10RenderStateManager*
};

} // namespace ni::c43
