// NiD3DXEffectStateManager (vtable 0x950AB0, 0x14 bytes, src/gamebryo/NiD3DXEffect.h): the
// ID3DXEffectStateManager the effects set their device states through, so they go through the
// renderer's state cache (NiDX9RenderState) and its shader constant cache; COM methods (stdcall,
// the object first), slot 21 the deleting destructor. One, made on first use (operator new).
// Also the constant cache's setters and NiDX9RenderState::SetTextureStageState.

#include "gamebryo/NiD3DXEffect.h"

#include <cstring>
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::d3dx;

constexpr uintptr_t StateManager_vtable = 0x00950AB0;
constexpr uintptr_t IID_IUnknown = 0x0098CB9C;
constexpr uintptr_t IID_ID3DXEffectStateManager = 0x009A20F4;
constexpr long D3DERR_INVALIDCALL = long(0x8876086C);
constexpr long E_NOINTERFACE_ = long(0x80004002);

GAME_FN(void*, __cdecl, OperatorNew, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, OperatorDelete, 0x005EEAB0, void* p);
// this file's, called where modes apply
GAME_FN(int, __cdecl, NiD3DX_IsEqualGUID, 0x0074D200, const void* a, const void* b);
GAME_FN(uint8_t, __thiscall, Constants_SetVertexShaderF, 0x0074D2C0, uint8_t* cache, uint32_t start,
        const void* data, uint32_t count);
GAME_FN(uint8_t, __thiscall, Constants_SetVertexShaderI, 0x0074D310, uint8_t* cache, uint32_t start,
        const void* data, uint32_t count);
GAME_FN(uint8_t, __thiscall, Constants_SetPixelShaderF, 0x0074D360, uint8_t* cache, uint32_t start,
        const void* data, uint32_t count);
GAME_FN(uint8_t, __thiscall, Constants_SetPixelShaderI, 0x0074D3B0, uint8_t* cache, uint32_t start,
        const void* data, uint32_t count);
GAME_FN(NiD3DXEffectStateManager*, __thiscall, StateManager_Construct, 0x0074D6D0, void* self, uint8_t* renderState,
        void* device);

inline uint8_t* Cache(NiD3DXEffectStateManager* self) {
    return At<uint8_t*>(self->renderState, 0x20F0);
}
inline void* Device(uint8_t* renderState) {
    return At<void*>(renderState, 0x2238);
}

// a range of constants copied into the cache (`size` bytes each), the dirty range widened; whether
// it fits
inline uint8_t SetConstants(uint8_t* cache, int base, int low, int high, int limit, uint32_t size, uint32_t start,
                            const void* data, uint32_t count) {
    uint32_t end = start + count;
    if (end > At<uint32_t>(cache, limit)) {
        return 0;
    }
    crt::MemcpyS(At<uint8_t*>(cache, base) + start * size, count * size, data, count * size);
    if (start < At<uint32_t>(cache, low)) {
        At<uint32_t>(cache, low) = start;
    }
    if (At<uint32_t>(cache, high) < end) {
        At<uint32_t>(cache, high) = end;
    }
    return 1;
}

} // namespace

// ---- IUnknown

// 0x74D0F0: AddRef (the count's interlocked increment)
unsigned long __stdcall NiD3DXEffectStateManager_AddRef(NiD3DXEffectStateManager* self) {
    return static_cast<unsigned long>(Increment(&self->refCount));
}

// 0x74D110: Release (deleted at 0)
unsigned long __stdcall NiD3DXEffectStateManager_Release(NiD3DXEffectStateManager* self) {
    if (Decrement(&self->refCount) == 0) {
        if (self != nullptr) {
            Virtual<void>(self, 21, 1);
        }
        return 0;
    }
    return self->refCount;
}

// 0x74D490: QueryInterface (IUnknown and ID3DXEffectStateManager)
long __stdcall NiD3DXEffectStateManager_QueryInterface(NiD3DXEffectStateManager* self, const void* iid,
                                                      re::Ptr32<void>* object) {
    if (!NiD3DX_IsEqualGUID(iid, reinterpret_cast<const void*>(IID_IUnknown)) &&
        !NiD3DX_IsEqualGUID(iid, reinterpret_cast<const void*>(IID_ID3DXEffectStateManager))) {
        *object = nullptr;
        return E_NOINTERFACE_;
    }
    *object = self;
    Com<unsigned long>(self, 1);   // AddRef
    return 0;
}

// ---- the states passed straight to the device

// 0x74D150 / 0x74D170 / 0x74D190 / 0x74D1B0 / 0x74D1D0
long __stdcall NiD3DXEffectStateManager_SetTransform(NiD3DXEffectStateManager* self, uint32_t state,
                                                    const void* matrix) {
    return Com<long>(self->device, kDevSetTransform, state, matrix);
}
long __stdcall NiD3DXEffectStateManager_SetMaterial(NiD3DXEffectStateManager* self, const void* material) {
    return Com<long>(self->device, kDevSetMaterial, material);
}
long __stdcall NiD3DXEffectStateManager_SetLight(NiD3DXEffectStateManager* self, uint32_t index, const void* light) {
    return Com<long>(self->device, kDevSetLight, index, light);
}
long __stdcall NiD3DXEffectStateManager_LightEnable(NiD3DXEffectStateManager* self, uint32_t index, int enable) {
    return Com<long>(self->device, kDevLightEnable, index, enable);
}
long __stdcall NiD3DXEffectStateManager_SetNPatchMode(NiD3DXEffectStateManager* self, float segments) {
    return Com<long>(self->device, kDevSetNPatchMode, segments);
}

// 0x74D1F0: no texture stage set yet this pass
void __fastcall NiD3DXEffectStateManager_ResetHighestTexture(NiD3DXEffectStateManager* self, re::Edx) {
    self->highestTexture = -1;
}

// 0x74D200: IsEqualGUID
int __cdecl NiD3DX_IsEqualGUIDImpl(const void* a, const void* b) {
    return std::memcmp(a, b, 16) == 0 ? 1 : 0;
}

// 0x74D2A0: a memcpy (memcpy_s of the same size)
void __cdecl NiD3DX_Memcpy(void* to, const void* from, uint32_t size) {
    crt::MemcpyS(to, size, from, size);
}

// ---- the shader constant cache (NiDX9RenderState +0x20F0): the float and integer constants (16
// bytes each), their dirty ranges and counts

// 0x74D2C0 / 0x74D310 / 0x74D360 / 0x74D3B0: vertex shader floats, integers; pixel shader floats,
// integers
uint8_t __fastcall ShaderConstants_SetVertexShaderF(uint8_t* self, re::Edx, uint32_t start, const void* data,
                                                    uint32_t count) {
    return SetConstants(self, 0x8, 0x18, 0x1C, 0x28, 16, start, data, count);
}
uint8_t __fastcall ShaderConstants_SetVertexShaderI(uint8_t* self, re::Edx, uint32_t start, const void* data,
                                                    uint32_t count) {
    return SetConstants(self, 0x30, 0x40, 0x44, 0x50, 16, start, data, count);
}
uint8_t __fastcall ShaderConstants_SetPixelShaderF(uint8_t* self, re::Edx, uint32_t start, const void* data,
                                                   uint32_t count) {
    return SetConstants(self, 0x10, 0x20, 0x24, 0x2C, 16, start, data, count);
}
uint8_t __fastcall ShaderConstants_SetPixelShaderI(uint8_t* self, re::Edx, uint32_t start, const void* data,
                                                   uint32_t count) {
    return SetConstants(self, 0x38, 0x48, 0x4C, 0x54, 16, start, data, count);
}

// 0x74D400: NiDX9RenderState::SetTextureStageState (through the cache)
void __fastcall RenderState_SetTextureStageState(uint8_t* self, re::Edx, uint32_t stage, uint32_t type,
                                                 uint32_t value) {
    uint32_t& cached = At<uint32_t>(self, 0x920 + (stage * 0x21 + type) * 8);
    if (cached != value) {
        Com<long>(Device(self), kDevSetTextureStageState, stage, type, value);
        cached = value;
    }
}

// 0x74D450: the deleting destructor (slot 21): the device released, the instance forgotten
NiD3DXEffectStateManager* __fastcall NiD3DXEffectStateManager_DeletingDestructor(NiD3DXEffectStateManager* self,
                                                                                 re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(StateManager_vtable);
    Com<unsigned long>(self->device, kRelease);
    NiD3DXEffectStateManager_ms_pkManager = nullptr;
    if (flags & 1) {
        OperatorDelete(self);
    }
    return self;
}

// ---- the cached states

// 0x74D4F0: SetRenderState
long __stdcall NiD3DXEffectStateManager_SetRenderState(NiD3DXEffectStateManager* self, uint32_t state,
                                                      uint32_t value) {
    uint8_t* renderState = self->renderState;
    if (At<uint32_t>(renderState, 0x120 + state * 8) != value) {
        Com<long>(Device(renderState), kDevSetRenderState, state, value);
        At<uint32_t>(renderState, 0x120 + state * 8) = value;
    }
    return 0;
}

// 0x74D530: SetTexture (the highest stage set remembered)
long __stdcall NiD3DXEffectStateManager_SetTexture(NiD3DXEffectStateManager* self, int32_t stage, void* texture) {
    uint8_t* renderState = self->renderState;
    if (At<void*>(renderState, 0x20A0 + stage * 4) != texture) {
        At<void*>(renderState, 0x20A0 + stage * 4) = texture;
        Com<long>(Device(renderState), kDevSetTexture, stage, texture);
    }
    if (self->highestTexture < stage) {
        self->highestTexture = stage;
    }
    return 0;
}

// 0x74D580: SetTextureStageState
long __stdcall NiD3DXEffectStateManager_SetTextureStageState(NiD3DXEffectStateManager* self, uint32_t stage,
                                                            uint32_t type, uint32_t value) {
    uint32_t& cached = At<uint32_t>(self->renderState, 0x920 + (stage * 0x21 + type) * 8);
    if (cached != value) {
        Com<long>(Device(self->renderState), kDevSetTextureStageState, stage, type, value);
        cached = value;
    }
    return 0;
}

// 0x74D5D0: SetSamplerState
long __stdcall NiD3DXEffectStateManager_SetSamplerState(NiD3DXEffectStateManager* self, uint32_t sampler,
                                                       uint32_t type, uint32_t value) {
    uint32_t& cached = At<uint32_t>(self->renderState, 0x19A0 + (type + sampler * 0xE) * 8);
    if (cached != value) {
        Com<long>(Device(self->renderState), kDevSetSamplerState, sampler, type, value);
        cached = value;
    }
    return 0;
}

// 0x74D620: SetFVF (none: nothing)
long __stdcall NiD3DXEffectStateManager_SetFVF(NiD3DXEffectStateManager* self, uint32_t fvf) {
    uint8_t* renderState = self->renderState;
    if (fvf != 0 && (At<uint8_t>(renderState, 0x20F7) != 0 || At<uint32_t>(renderState, 0x20F8) != fvf)) {
        At<uint8_t>(renderState, 0x20F7) = 0;
        At<uint32_t>(renderState, 0x20F8) = fvf;
        Com<long>(Device(renderState), kDevSetFVF, fvf);
    }
    return 0;
}

// 0x74D670: SetVertexShader
long __stdcall NiD3DXEffectStateManager_SetVertexShader(NiD3DXEffectStateManager* self, void* shader) {
    uint8_t* renderState = self->renderState;
    if (At<void*>(renderState, 0x20E0) != shader) {
        At<void*>(renderState, 0x20E0) = shader;
        Com<long>(Device(renderState), kDevSetVertexShader, shader);
    }
    return 0;
}

// 0x74D6D0: the constructor (the device held)
NiD3DXEffectStateManager* __fastcall NiD3DXEffectStateManager_Construct(NiD3DXEffectStateManager* self, re::Edx,
                                                                        uint8_t* renderState, void* device) {
    self->renderState = renderState;
    self->vtable = reinterpret_cast<void*>(StateManager_vtable);
    self->highestTexture = -1;
    self->refCount = 0;
    self->device = device;
    Com<unsigned long>(device, 1);   // AddRef
    return self;
}

// 0x74D710: GetInstance (made for the renderer's render state and device, when it has them)
NiD3DXEffectStateManager* __cdecl NiD3DXEffectStateManager_GetInstance(uint8_t* renderer) {
    if (NiD3DXEffectStateManager_ms_pkManager != nullptr) {
        return NiD3DXEffectStateManager_ms_pkManager;
    }
    if (renderer == nullptr) {
        return nullptr;
    }
    uint8_t* renderState = At<uint8_t*>(renderer, 0x6B4);
    if (renderState == nullptr) {
        return nullptr;
    }
    void* device = At<void*>(renderer, 0xA4);
    if (device == nullptr) {
        return nullptr;
    }
    void* memory = OperatorNew(sizeof(NiD3DXEffectStateManager));
    NiD3DXEffectStateManager* manager = memory != nullptr ? StateManager_Construct(memory, renderState, device)
                                                          : nullptr;
    NiD3DXEffectStateManager_ms_pkManager = manager;
    return manager;
}

// 0x74D770 / 0x74D7B0 / 0x74D850 / 0x74D890: the shader constants into the cache
// (D3DERR_INVALIDCALL past its end)
long __stdcall NiD3DXEffectStateManager_SetVertexShaderConstantF(NiD3DXEffectStateManager* self, uint32_t start,
                                                                const void* data, uint32_t count) {
    return Constants_SetVertexShaderF(Cache(self), start, data, count) ? 0 : D3DERR_INVALIDCALL;
}
long __stdcall NiD3DXEffectStateManager_SetVertexShaderConstantI(NiD3DXEffectStateManager* self, uint32_t start,
                                                                const void* data, uint32_t count) {
    return Constants_SetVertexShaderI(Cache(self), start, data, count) ? 0 : D3DERR_INVALIDCALL;
}
long __stdcall NiD3DXEffectStateManager_SetPixelShaderConstantF(NiD3DXEffectStateManager* self, uint32_t start,
                                                               const void* data, uint32_t count) {
    return Constants_SetPixelShaderF(Cache(self), start, data, count) ? 0 : D3DERR_INVALIDCALL;
}
long __stdcall NiD3DXEffectStateManager_SetPixelShaderConstantI(NiD3DXEffectStateManager* self, uint32_t start,
                                                               const void* data, uint32_t count) {
    return Constants_SetPixelShaderI(Cache(self), start, data, count) ? 0 : D3DERR_INVALIDCALL;
}

// 0x74D7F0 / 0x74D8D0: the boolean constants (4 bytes each), inlined
long __stdcall NiD3DXEffectStateManager_SetVertexShaderConstantB(NiD3DXEffectStateManager* self, uint32_t start,
                                                                const void* data, uint32_t count) {
    return SetConstants(Cache(self), 0x58, 0x68, 0x6C, 0x78, 4, start, data, count) ? 0 : D3DERR_INVALIDCALL;
}
long __stdcall NiD3DXEffectStateManager_SetPixelShaderConstantB(NiD3DXEffectStateManager* self, uint32_t start,
                                                               const void* data, uint32_t count) {
    return SetConstants(Cache(self), 0x60, 0x70, 0x74, 0x7C, 4, start, data, count) ? 0 : D3DERR_INVALIDCALL;
}

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NID3DXSTATE_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                                 \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NID3DXSTATE_REVERSED("NiD3DXEffectStateManager::AddRef", 0x0074D0F0, &ni::NiD3DXEffectStateManager_AddRef);
NID3DXSTATE_REVERSED("NiD3DXEffectStateManager::Release", 0x0074D110, &ni::NiD3DXEffectStateManager_Release);
NID3DXSTATE_REVERSED("NiD3DXEffectStateManager::SetTransform", 0x0074D150, &ni::NiD3DXEffectStateManager_SetTransform);
NID3DXSTATE_REVERSED("NiD3DXEffectStateManager::SetMaterial", 0x0074D170, &ni::NiD3DXEffectStateManager_SetMaterial);
NID3DXSTATE_REVERSED("NiD3DXEffectStateManager::SetLight", 0x0074D190, &ni::NiD3DXEffectStateManager_SetLight);
NID3DXSTATE_REVERSED("NiD3DXEffectStateManager::LightEnable", 0x0074D1B0, &ni::NiD3DXEffectStateManager_LightEnable);
NID3DXSTATE_REVERSED("NiD3DXEffectStateManager::SetNPatchMode", 0x0074D1D0, &ni::NiD3DXEffectStateManager_SetNPatchMode);
NID3DXSTATE_REVERSED("NiD3DXEffectStateManager::ResetHighestTexture", 0x0074D1F0, &ni::NiD3DXEffectStateManager_ResetHighestTexture);
NID3DXSTATE_REVERSED("NiD3DX::IsEqualGUID", 0x0074D200, &ni::NiD3DX_IsEqualGUIDImpl);
NID3DXSTATE_REVERSED("NiD3DX::Memcpy", 0x0074D2A0, &ni::NiD3DX_Memcpy);
NID3DXSTATE_REVERSED("NiDX9ShaderConstantCache::SetVertexShaderConstantF", 0x0074D2C0, &ni::ShaderConstants_SetVertexShaderF);
NID3DXSTATE_REVERSED("NiDX9ShaderConstantCache::SetVertexShaderConstantI", 0x0074D310, &ni::ShaderConstants_SetVertexShaderI);
NID3DXSTATE_REVERSED("NiDX9ShaderConstantCache::SetPixelShaderConstantF", 0x0074D360, &ni::ShaderConstants_SetPixelShaderF);
NID3DXSTATE_REVERSED("NiDX9ShaderConstantCache::SetPixelShaderConstantI", 0x0074D3B0, &ni::ShaderConstants_SetPixelShaderI);
NID3DXSTATE_REVERSED("NiDX9RenderState::SetTextureStageState_74D400", 0x0074D400, &ni::RenderState_SetTextureStageState);
NID3DXSTATE_REVERSED("NiD3DXEffectStateManager::DeletingDestructor", 0x0074D450, &ni::NiD3DXEffectStateManager_DeletingDestructor);
NID3DXSTATE_REVERSED("NiD3DXEffectStateManager::QueryInterface", 0x0074D490, &ni::NiD3DXEffectStateManager_QueryInterface);
NID3DXSTATE_REVERSED("NiD3DXEffectStateManager::SetRenderState", 0x0074D4F0, &ni::NiD3DXEffectStateManager_SetRenderState);
NID3DXSTATE_REVERSED("NiD3DXEffectStateManager::SetTexture", 0x0074D530, &ni::NiD3DXEffectStateManager_SetTexture);
NID3DXSTATE_REVERSED("NiD3DXEffectStateManager::SetTextureStageState", 0x0074D580, &ni::NiD3DXEffectStateManager_SetTextureStageState);
NID3DXSTATE_REVERSED("NiD3DXEffectStateManager::SetSamplerState", 0x0074D5D0, &ni::NiD3DXEffectStateManager_SetSamplerState);
NID3DXSTATE_REVERSED("NiD3DXEffectStateManager::SetFVF", 0x0074D620, &ni::NiD3DXEffectStateManager_SetFVF);
NID3DXSTATE_REVERSED("NiD3DXEffectStateManager::SetVertexShader", 0x0074D670, &ni::NiD3DXEffectStateManager_SetVertexShader);
NID3DXSTATE_REVERSED("NiD3DXEffectStateManager::NiD3DXEffectStateManager", 0x0074D6D0, &ni::NiD3DXEffectStateManager_Construct);
NID3DXSTATE_REVERSED("NiD3DXEffectStateManager::GetInstance", 0x0074D710, &ni::NiD3DXEffectStateManager_GetInstance);
NID3DXSTATE_REVERSED("NiD3DXEffectStateManager::SetVertexShaderConstantF", 0x0074D770, &ni::NiD3DXEffectStateManager_SetVertexShaderConstantF);
NID3DXSTATE_REVERSED("NiD3DXEffectStateManager::SetVertexShaderConstantI", 0x0074D7B0, &ni::NiD3DXEffectStateManager_SetVertexShaderConstantI);
NID3DXSTATE_REVERSED("NiD3DXEffectStateManager::SetVertexShaderConstantB", 0x0074D7F0, &ni::NiD3DXEffectStateManager_SetVertexShaderConstantB);
NID3DXSTATE_REVERSED("NiD3DXEffectStateManager::SetPixelShaderConstantF", 0x0074D850, &ni::NiD3DXEffectStateManager_SetPixelShaderConstantF);
NID3DXSTATE_REVERSED("NiD3DXEffectStateManager::SetPixelShaderConstantI", 0x0074D890, &ni::NiD3DXEffectStateManager_SetPixelShaderConstantI);
NID3DXSTATE_REVERSED("NiD3DXEffectStateManager::SetPixelShaderConstantB", 0x0074D8D0, &ni::NiD3DXEffectStateManager_SetPixelShaderConstantB);
