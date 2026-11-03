// NiD3D10DeviceState (vtable 0x9A07F8: one slot, the deleting destructor; 0x26C bytes, the
// renderer's +0x444, made by its initialization 0x8A9000 with the device): a cache of what's set
// on the ID3D10Device, each object held (AddRef / Release) while it's set. Its setters pass
// straight to the device and keep what they set. Layout from the constructor 0x8B3570:
//   +0x4 the ID3D10Device (held), +0x8 / +0xC / +0x10 the blend, depth-stencil and rasterizer
//   states, +0x14 / +0x54 / +0x94 the vertex, geometry and pixel samplers (16 each), +0xD4 the
//   blend factor, +0xE4 the sample mask (-1), +0xE8 the stencil reference, +0xEC / +0x12C /
//   +0x16C the stages' shader resource views (16 each), +0x1AC / +0x1E8 / +0x224 their constant
//   buffers (15 each), +0x260 / +0x264 / +0x268 the vertex, geometry and pixel shaders.

#include "gamebryo/NiD3D10C43.h"
#include "reversed.h"

namespace ni::c43 {
namespace {

struct DeviceState {
    re::Ptr32<void> vtable;                 // +0x0
    re::Ptr32<void> device;                 // +0x4
    re::Ptr32<void> blendState;             // +0x8
    re::Ptr32<void> depthStencilState;      // +0xC
    re::Ptr32<void> rasterizerState;        // +0x10
    re::Ptr32<void> vsSamplers[16];         // +0x14
    re::Ptr32<void> gsSamplers[16];         // +0x54
    re::Ptr32<void> psSamplers[16];         // +0x94
    float blendFactor[4];         // +0xD4
    uint32_t sampleMask;          // +0xE4
    uint32_t stencilRef;          // +0xE8
    re::Ptr32<void> vsResources[16];        // +0xEC
    re::Ptr32<void> gsResources[16];        // +0x12C
    re::Ptr32<void> psResources[16];        // +0x16C
    re::Ptr32<void> vsConstantBuffers[15];  // +0x1AC
    re::Ptr32<void> gsConstantBuffers[15];  // +0x1E8
    re::Ptr32<void> psConstantBuffers[15];  // +0x224
    re::Ptr32<void> vertexShader;           // +0x260
    re::Ptr32<void> geometryShader;         // +0x264
    re::Ptr32<void> pixelShader;            // +0x268
};
static_assert(offsetof(DeviceState, vsSamplers) == 0x14);
static_assert(offsetof(DeviceState, blendFactor) == 0xD4);
static_assert(offsetof(DeviceState, vsResources) == 0xEC);
static_assert(offsetof(DeviceState, vsConstantBuffers) == 0x1AC);
static_assert(offsetof(DeviceState, vertexShader) == 0x260);
static_assert(sizeof(DeviceState) == 0x26C);

constexpr uintptr_t kDeviceStateVtable = 0x009A07F8;

GAME_FN(void, __thiscall, DeviceState_Destruct, 0x008B3670, DeviceState* self);

// a slot range set on the device (start below the slots and an array given: the count clamped,
// the device told, then each slot assigned)
void SetSlots(DeviceState* self, re::Ptr32<void>* slots, uint32_t limit, int deviceSlot, uint32_t start, uint32_t count,
              const re::Ptr32<void>* objects) {
    if (start < limit && objects != nullptr) {
        if (limit - start < count) {
            count = limit - start;
        }
        Com<void>(self->device, deviceSlot, start, count, objects);
        for (uint32_t i = 0; i < count; i++) {
            ComAssign(slots[start + i], objects[i]);
        }
    }
}

// the constant buffers' kind: the device told only up to the last non-null one given (not at all
// when they're all null), every slot assigned
void SetConstantBuffers(DeviceState* self, re::Ptr32<void>* slots, int deviceSlot, uint32_t start, uint32_t count,
                        const re::Ptr32<void>* buffers) {
    if (start < 0xF && buffers != nullptr) {
        if (0xF - start < count) {
            count = 0xF - start;
        }
        uint32_t used = count;
        while (used != 0 && buffers[used - 1] == nullptr) {
            used--;
        }
        if (used != 0) {
            Com<void>(self->device, deviceSlot, start, used, buffers);
        }
        for (uint32_t i = 0; i < count; i++) {
            ComAssign(slots[start + i], buffers[i]);
        }
    }
}

// a stage's views all let go, then the device given a 16-view array: `given` (the vertex
// stage's own for all three stages, as the game passes it)
void ClearResources(DeviceState* self, re::Ptr32<void>* slots, int deviceSlot, const re::Ptr32<void>* given) {
    for (int i = 0; i < 16; i++) {
        if (slots[i] != nullptr) {
            ComRelease(slots[i]);
            slots[i] = nullptr;
        }
    }
    Com<void>(self->device, deviceSlot, 0u, 0x10u, given);
}

} // namespace

// 0x8B3570 (thiscall): NiD3D10DeviceState::NiD3D10DeviceState(device)
DeviceState* __fastcall DeviceState_Construct(DeviceState* self, re::Edx, void* device) {
    self->vtable = reinterpret_cast<void*>(kDeviceStateVtable);
    self->device = device;
    self->blendState = nullptr;
    self->depthStencilState = nullptr;
    self->rasterizerState = nullptr;
    self->sampleMask = 0xFFFFFFFF;
    self->stencilRef = 0;
    self->vertexShader = nullptr;
    self->geometryShader = nullptr;
    self->pixelShader = nullptr;
    ComAddRef(device);
    crt::Memset(self->vsSamplers, 0, 0x40);
    crt::Memset(self->gsSamplers, 0, 0x40);
    crt::Memset(self->psSamplers, 0, 0x40);
    for (float& f : self->blendFactor) {
        f = 0.0f;
    }
    crt::Memset(self->vsResources, 0, 0x40);
    crt::Memset(self->gsResources, 0, 0x40);
    crt::Memset(self->psResources, 0, 0x40);
    crt::Memset(self->vsConstantBuffers, 0, 0x3C);
    crt::Memset(self->gsConstantBuffers, 0, 0x3C);
    crt::Memset(self->psConstantBuffers, 0, 0x3C);
    return self;
}

// 0x8B3670 (thiscall): NiD3D10DeviceState::~NiD3D10DeviceState (the device first, then the
// states, the samplers and views stage by stage per slot, the shaders; the constant buffers are
// not let go, as in the game)
void __fastcall DeviceState_DestructImpl(DeviceState* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kDeviceStateVtable);
    ComRelease(self->device);
    ComRelease(self->blendState);
    ComRelease(self->depthStencilState);
    ComRelease(self->rasterizerState);
    for (int i = 0; i < 16; i++) {
        ComRelease(self->vsSamplers[i]);
        ComRelease(self->gsSamplers[i]);
        ComRelease(self->psSamplers[i]);
        ComRelease(self->vsResources[i]);
        ComRelease(self->gsResources[i]);
        ComRelease(self->psResources[i]);
    }
    ComRelease(self->vertexShader);
    ComRelease(self->geometryShader);
    ComRelease(self->pixelShader);
}

// 0x8B3780 (thiscall): NiD3D10DeviceState::OMSetBlendState(state, factor, mask) (the factor
// through the x87: a copy)
void __fastcall DeviceState_OMSetBlendState(DeviceState* self, re::Edx, void* state, const float* factor,
                                            uint32_t mask) {
    Com<void>(self->device, kOMSetBlendState, state, factor, mask);
    ComAssign(self->blendState, state);
    self->blendFactor[0] = factor[0];
    self->blendFactor[1] = factor[1];
    self->blendFactor[2] = factor[2];
    self->sampleMask = mask;
    self->blendFactor[3] = factor[3];
}

// 0x8B3800 (thiscall): NiD3D10DeviceState::OMSetDepthStencilState(state, reference)
void __fastcall DeviceState_OMSetDepthStencilState(DeviceState* self, re::Edx, void* state, uint32_t ref) {
    Com<void>(self->device, kOMSetDepthStencilState, state, ref);
    ComAssign(self->depthStencilState, state);
    self->stencilRef = ref;
}

// 0x8B3850 (thiscall): NiD3D10DeviceState::RSSetState
void __fastcall DeviceState_RSSetState(DeviceState* self, re::Edx, void* state) {
    Com<void>(self->device, kRSSetState, state);
    ComAssign(self->rasterizerState, state);
}

// 0x8B3890 (thiscall): NiD3D10DeviceState::VSSetSamplers(start, count, samplers)
void __fastcall DeviceState_VSSetSamplers(DeviceState* self, re::Edx, uint32_t start, uint32_t count,
                                          const re::Ptr32<void>* samplers) {
    SetSlots(self, self->vsSamplers, 0x10, kVSSetSamplers, start, count, samplers);
}

// 0x8B3910 (thiscall): NiD3D10DeviceState::GSSetSamplers
void __fastcall DeviceState_GSSetSamplers(DeviceState* self, re::Edx, uint32_t start, uint32_t count,
                                          const re::Ptr32<void>* samplers) {
    SetSlots(self, self->gsSamplers, 0x10, kGSSetSamplers, start, count, samplers);
}

// 0x8B3990 (thiscall): NiD3D10DeviceState::PSSetSamplers
void __fastcall DeviceState_PSSetSamplers(DeviceState* self, re::Edx, uint32_t start, uint32_t count,
                                          const re::Ptr32<void>* samplers) {
    SetSlots(self, self->psSamplers, 0x10, kPSSetSamplers, start, count, samplers);
}

// 0x8B3A10 (thiscall): NiD3D10DeviceState::VSSetShaderResources
void __fastcall DeviceState_VSSetShaderResources(DeviceState* self, re::Edx, uint32_t start, uint32_t count,
                                                 const re::Ptr32<void>* views) {
    SetSlots(self, self->vsResources, 0x10, kVSSetShaderResources, start, count, views);
}

// 0x8B3A90 (thiscall): NiD3D10DeviceState::VSClearShaderResources (the cleared array given)
void __fastcall DeviceState_VSClearShaderResources(DeviceState* self, re::Edx) {
    ClearResources(self, self->vsResources, kVSSetShaderResources, self->vsResources);
}

// 0x8B3AE0 (thiscall): NiD3D10DeviceState::GSSetShaderResources
void __fastcall DeviceState_GSSetShaderResources(DeviceState* self, re::Edx, uint32_t start, uint32_t count,
                                                 const re::Ptr32<void>* views) {
    SetSlots(self, self->gsResources, 0x10, kGSSetShaderResources, start, count, views);
}

// 0x8B3B60 (thiscall): NiD3D10DeviceState::GSClearShaderResources (the game hands the device the
// vertex stage's array, +0xEC, not the cleared one)
void __fastcall DeviceState_GSClearShaderResources(DeviceState* self, re::Edx) {
    ClearResources(self, self->gsResources, kGSSetShaderResources, self->vsResources);
}

// 0x8B3BB0 (thiscall): NiD3D10DeviceState::PSSetShaderResources
void __fastcall DeviceState_PSSetShaderResources(DeviceState* self, re::Edx, uint32_t start, uint32_t count,
                                                 const re::Ptr32<void>* views) {
    SetSlots(self, self->psResources, 0x10, kPSSetShaderResources, start, count, views);
}

// 0x8B3C30 (thiscall): NiD3D10DeviceState::PSClearShaderResources (the vertex stage's array given,
// as for the geometry stage)
void __fastcall DeviceState_PSClearShaderResources(DeviceState* self, re::Edx) {
    ClearResources(self, self->psResources, kPSSetShaderResources, self->vsResources);
}

// 0x8B3C80 (thiscall): NiD3D10DeviceState::VSSetConstantBuffers(start, count, buffers)
void __fastcall DeviceState_VSSetConstantBuffers(DeviceState* self, re::Edx, uint32_t start, uint32_t count,
                                                 const re::Ptr32<void>* buffers) {
    SetConstantBuffers(self, self->vsConstantBuffers, kVSSetConstantBuffers, start, count, buffers);
}

// 0x8B3D20 (thiscall): NiD3D10DeviceState::GSSetConstantBuffers
void __fastcall DeviceState_GSSetConstantBuffers(DeviceState* self, re::Edx, uint32_t start, uint32_t count,
                                                 const re::Ptr32<void>* buffers) {
    SetConstantBuffers(self, self->gsConstantBuffers, kGSSetConstantBuffers, start, count, buffers);
}

// 0x8B3DC0 (thiscall): NiD3D10DeviceState::PSSetConstantBuffers
void __fastcall DeviceState_PSSetConstantBuffers(DeviceState* self, re::Edx, uint32_t start, uint32_t count,
                                                 const re::Ptr32<void>* buffers) {
    SetConstantBuffers(self, self->psConstantBuffers, kPSSetConstantBuffers, start, count, buffers);
}

// 0x8B3E60 (thiscall): NiD3D10DeviceState::VSSetShader (kept, then set)
void __fastcall DeviceState_VSSetShader(DeviceState* self, re::Edx, void* shader) {
    ComAssign(self->vertexShader, shader);
    Com<void>(self->device, kVSSetShader, shader);
}

// 0x8B3EB0 (thiscall): NiD3D10DeviceState::VSClearShader
void __fastcall DeviceState_VSClearShader(DeviceState* self, re::Edx) {
    if (self->vertexShader != nullptr) {
        ComRelease(self->vertexShader);
        self->vertexShader = nullptr;
    }
    Com<void>(self->device, kVSSetShader, static_cast<void*>(nullptr));
}

// 0x8B3EE0 (thiscall): NiD3D10DeviceState::GSSetShader
void __fastcall DeviceState_GSSetShader(DeviceState* self, re::Edx, void* shader) {
    ComAssign(self->geometryShader, shader);
    Com<void>(self->device, kGSSetShader, shader);
}

// 0x8B3F30 (thiscall): NiD3D10DeviceState::GSClearShader
void __fastcall DeviceState_GSClearShader(DeviceState* self, re::Edx) {
    if (self->geometryShader != nullptr) {
        ComRelease(self->geometryShader);
        self->geometryShader = nullptr;
    }
    Com<void>(self->device, kGSSetShader, static_cast<void*>(nullptr));
}

// 0x8B3F60 (thiscall): NiD3D10DeviceState::PSSetShader
void __fastcall DeviceState_PSSetShader(DeviceState* self, re::Edx, void* shader) {
    ComAssign(self->pixelShader, shader);
    Com<void>(self->device, kPSSetShader, shader);
}

// 0x8B3FB0 (thiscall): NiD3D10DeviceState::PSClearShader
void __fastcall DeviceState_PSClearShader(DeviceState* self, re::Edx) {
    if (self->pixelShader != nullptr) {
        ComRelease(self->pixelShader);
        self->pixelShader = nullptr;
    }
    Com<void>(self->device, kPSSetShader, static_cast<void*>(nullptr));
}

// 0x8B3FE0 (thiscall): NiD3D10DeviceState's deleting destructor
DeviceState* __fastcall DeviceState_DeletingDestructor(DeviceState* self, re::Edx, uint32_t flags) {
    DeviceState_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x26C);
    }
    return self;
}

#define GB43C_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

GB43C_REVERSED("NiD3D10DeviceState::NiD3D10DeviceState", 0x008B3570, DeviceState_Construct);
GB43C_REVERSED("NiD3D10DeviceState::~NiD3D10DeviceState", 0x008B3670, DeviceState_DestructImpl);
GB43C_REVERSED("NiD3D10DeviceState::OMSetBlendState", 0x008B3780, DeviceState_OMSetBlendState);
GB43C_REVERSED("NiD3D10DeviceState::OMSetDepthStencilState", 0x008B3800, DeviceState_OMSetDepthStencilState);
GB43C_REVERSED("NiD3D10DeviceState::RSSetState", 0x008B3850, DeviceState_RSSetState);
GB43C_REVERSED("NiD3D10DeviceState::VSSetSamplers", 0x008B3890, DeviceState_VSSetSamplers);
GB43C_REVERSED("NiD3D10DeviceState::GSSetSamplers", 0x008B3910, DeviceState_GSSetSamplers);
GB43C_REVERSED("NiD3D10DeviceState::PSSetSamplers", 0x008B3990, DeviceState_PSSetSamplers);
GB43C_REVERSED("NiD3D10DeviceState::VSSetShaderResources", 0x008B3A10, DeviceState_VSSetShaderResources);
GB43C_REVERSED("NiD3D10DeviceState::VSClearShaderResources", 0x008B3A90, DeviceState_VSClearShaderResources);
GB43C_REVERSED("NiD3D10DeviceState::GSSetShaderResources", 0x008B3AE0, DeviceState_GSSetShaderResources);
GB43C_REVERSED("NiD3D10DeviceState::GSClearShaderResources", 0x008B3B60, DeviceState_GSClearShaderResources);
GB43C_REVERSED("NiD3D10DeviceState::PSSetShaderResources", 0x008B3BB0, DeviceState_PSSetShaderResources);
GB43C_REVERSED("NiD3D10DeviceState::PSClearShaderResources", 0x008B3C30, DeviceState_PSClearShaderResources);
GB43C_REVERSED("NiD3D10DeviceState::VSSetConstantBuffers", 0x008B3C80, DeviceState_VSSetConstantBuffers);
GB43C_REVERSED("NiD3D10DeviceState::GSSetConstantBuffers", 0x008B3D20, DeviceState_GSSetConstantBuffers);
GB43C_REVERSED("NiD3D10DeviceState::PSSetConstantBuffers", 0x008B3DC0, DeviceState_PSSetConstantBuffers);
GB43C_REVERSED("NiD3D10DeviceState::VSSetShader", 0x008B3E60, DeviceState_VSSetShader);
GB43C_REVERSED("NiD3D10DeviceState::VSClearShader", 0x008B3EB0, DeviceState_VSClearShader);
GB43C_REVERSED("NiD3D10DeviceState::GSSetShader", 0x008B3EE0, DeviceState_GSSetShader);
GB43C_REVERSED("NiD3D10DeviceState::GSClearShader", 0x008B3F30, DeviceState_GSClearShader);
GB43C_REVERSED("NiD3D10DeviceState::PSSetShader", 0x008B3F60, DeviceState_PSSetShader);
GB43C_REVERSED("NiD3D10DeviceState::PSClearShader", 0x008B3FB0, DeviceState_PSClearShader);
GB43C_REVERSED("NiD3D10DeviceState::DeletingDestructor", 0x008B3FE0, DeviceState_DeletingDestructor);

} // namespace ni::c43
