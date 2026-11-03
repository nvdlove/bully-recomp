// NiD3D10RenderStateManager (vtable 0x9A0300: one slot, the deleting destructor; 0x1500 bytes,
// the renderer's +0x44C, made by its initialization 0x8A9000): the D3D10 state descriptions the
// renderer's NiProperty state turns into, in two sets: the defaults (+0xC) and the current ones
// (+0xA84; RestoreDefaults, chunk 42's 0x8B0410, copies the first over the second). Each set: the
// blend, depth / stencil and rasterizer descriptions, 16 sampler descriptions for each of the
// three shader stages (by program type: vertex, pixel, geometry), the blend factor, sample mask and stencil
// reference. +0x4 the ID3D10Device (held), +0x8 the NiD3D10DeviceState it sets the made states
// on. Here the Gamebryo-to-D3D10 conversions (statics, cdecl), the property setters (alpha,
// stencil, wireframe, z buffer), a NiD3D10RenderStateGroup's state taken over, the constructor
// and destructor, and the current state applied (chunk 42 writes the state objects' creation).

#include "gamebryo/NiD3D10C43.h"
#include "reversed.h"

namespace ni::c43 {
namespace {

// ---- the D3D10 descriptions (D3D10_*_DESC), as the manager keeps them
struct BlendDesc {
    uint32_t alphaToCoverageEnable;   // +0x0
    uint32_t blendEnable[8];          // +0x4
    uint32_t srcBlend;                // +0x24
    uint32_t destBlend;               // +0x28
    uint32_t blendOp;                 // +0x2C
    uint32_t srcBlendAlpha;           // +0x30
    uint32_t destBlendAlpha;          // +0x34
    uint32_t blendOpAlpha;            // +0x38
    uint8_t renderTargetWriteMask[8]; // +0x3C
};
static_assert(sizeof(BlendDesc) == 0x44);

struct StencilOpDesc {
    uint32_t failOp;       // +0x0
    uint32_t depthFailOp;  // +0x4
    uint32_t passOp;       // +0x8
    uint32_t func;         // +0xC
};
struct DepthStencilDesc {
    uint32_t depthEnable;      // +0x0
    uint32_t depthWriteMask;   // +0x4
    uint32_t depthFunc;        // +0x8
    uint32_t stencilEnable;    // +0xC
    uint8_t stencilReadMask;   // +0x10
    uint8_t stencilWriteMask;  // +0x11
    uint8_t pad12[2];
    StencilOpDesc frontFace;   // +0x14
    StencilOpDesc backFace;    // +0x24
};
static_assert(sizeof(DepthStencilDesc) == 0x34);

struct RasterizerDesc {
    uint32_t fillMode;               // +0x0 (2 wireframe, 3 solid)
    uint32_t cullMode;               // +0x4 (1 none, 2 front, 3 back)
    uint32_t frontCounterClockwise;  // +0x8
    int32_t depthBias;               // +0xC
    float depthBiasClamp;            // +0x10
    float slopeScaledDepthBias;      // +0x14
    uint32_t depthClipEnable;        // +0x18
    uint32_t scissorEnable;          // +0x1C
    uint32_t multisampleEnable;      // +0x20
    uint32_t antialiasedLineEnable;  // +0x24
};
static_assert(sizeof(RasterizerDesc) == 0x28);

struct SamplerDesc {
    uint32_t filter;          // +0x0
    uint32_t addressU;        // +0x4
    uint32_t addressV;        // +0x8
    uint32_t addressW;        // +0xC
    float mipLODBias;         // +0x10
    uint32_t maxAnisotropy;   // +0x14
    uint32_t comparisonFunc;  // +0x18
    float borderColor[4];     // +0x1C
    float minLOD;             // +0x2C
    float maxLOD;             // +0x30
};
static_assert(sizeof(SamplerDesc) == 0x34);

struct StateSet {
    BlendDesc blend;                 // +0x0
    DepthStencilDesc depthStencil;   // +0x44
    RasterizerDesc rasterizer;       // +0x78
    SamplerDesc samplers[3][16];     // +0xA0 (by NiGPUProgram type: vertex, pixel, geometry)
    float blendFactor[4];            // +0xA60
    uint32_t sampleMask;             // +0xA70
    uint32_t stencilRef;             // +0xA74
};
static_assert(offsetof(StateSet, samplers) == 0xA0);
static_assert(sizeof(StateSet) == 0xA78);

struct RenderStateManager {
    re::Ptr32<void> vtable;          // +0x0
    re::Ptr32<void> device;          // +0x4 (ID3D10Device, held)
    re::Ptr32<void> deviceState;     // +0x8 (NiD3D10DeviceState)
    StateSet defaults;     // +0xC
    StateSet current;      // +0xA84
    uint8_t unknown14FC;   // +0x14FC
    uint8_t pad14FD[3];
};
static_assert(offsetof(RenderStateManager, current) == 0xA84);
static_assert(offsetof(RenderStateManager, current.depthStencil) == 0xAC8);
static_assert(offsetof(RenderStateManager, current.rasterizer) == 0xAFC);
static_assert(offsetof(RenderStateManager, current.samplers) == 0xB24);
static_assert(offsetof(RenderStateManager, current.blendFactor) == 0x14E4);
static_assert(offsetof(RenderStateManager, current.stencilRef) == 0x14F8);
static_assert(sizeof(RenderStateManager) == 0x1500);

constexpr uintptr_t kRenderStateManagerVtable = 0x009A0300;

// the property flags (NiProperty's +0x18, a word) and the stencil property's reference (+0x1C)
// and mask (+0x20)
inline uint32_t PropertyFlags(const uint8_t* property) {
    return At<uint16_t>(property, 0x18);
}

// NiD3D10RenderStateGroup's getters (thiscall on the group)
GAME_FN(void, __thiscall, Group_GetBlendDesc, 0x008C04B0, const void* group, BlendDesc* desc, uint32_t* valid);
GAME_FN(uint8_t, __thiscall, Group_GetBlendFactor, 0x008C0350, const void* group, float* factor);
GAME_FN(uint8_t, __thiscall, Group_GetSampleMask, 0x008C0390, const void* group, uint32_t* mask);
GAME_FN(void, __thiscall, Group_GetDepthStencilDesc, 0x008C04E0, const void* group, DepthStencilDesc* desc,
        uint32_t* valid);
GAME_FN(uint8_t, __thiscall, Group_GetStencilRef, 0x008C03B0, const void* group, uint32_t* ref);
GAME_FN(void, __thiscall, Group_GetRasterizerDesc, 0x008C0510, const void* group, RasterizerDesc* desc,
        uint32_t* valid);
GAME_FN(const uint8_t*, __thiscall, Group_GetSamplerMap, 0x008C0490, const void* group, uint32_t stage);
GAME_FN(void, __thiscall, Group_GetSamplerDesc, 0x008C0540, const void* group, uint32_t stage, uint32_t sampler,
        SamplerDesc* desc, uint32_t* valid);
// the manager's setters and updates (chunk 42's)
GAME_FN(void, __thiscall, Manager_SetBlendDesc, 0x008B0010, void* self, const BlendDesc* desc, uint32_t valid);
GAME_FN(void, __thiscall, Manager_SetDepthStencilDesc, 0x008B0190, void* self, const DepthStencilDesc* desc,
        uint32_t valid);
GAME_FN(void, __thiscall, Manager_SetRasterizerDesc, 0x008B0280, void* self, const RasterizerDesc* desc,
        uint32_t valid);
GAME_FN(void, __thiscall, Manager_SetSamplerDesc, 0x008B0360, void* self, uint32_t stage, uint32_t sampler,
        const SamplerDesc* desc, uint32_t valid);
GAME_FN(void, __thiscall, Manager_RestoreDefaults, 0x008B0410, void* self);
GAME_FN(void, __thiscall, Manager_UpdateBlendState, 0x008B0520, void* self);
GAME_FN(void, __thiscall, Manager_UpdateDepthStencilState, 0x008B05A0, void* self);
GAME_FN(void, __thiscall, Manager_UpdateRasterizerState, 0x008B0620, void* self);
GAME_FN(void, __thiscall, Manager_UpdateVertexSamplers, 0x008B06B0, void* self, uint32_t start, uint32_t count);
GAME_FN(void, __thiscall, Manager_UpdateGeometrySamplers, 0x008B0790, void* self, uint32_t start, uint32_t count);
GAME_FN(void, __thiscall, Manager_UpdatePixelSamplers, 0x008B0870, void* self, uint32_t start, uint32_t count);
// the conversions (chunk 42's, and this file's: called where modes apply)
GAME_FN(uint32_t, __cdecl, ConvertGbAlphaBlendToD3D10Blend, 0x008B0950, uint32_t mode);
GAME_FN(uint32_t, __cdecl, ConvertGbStencilFuncToD3D10Comparison, 0x008B09E0, uint32_t func);
GAME_FN(uint32_t, __cdecl, ConvertGbStencilActionToD3D10StencilOp, 0x008B0A50, uint32_t action);
GAME_FN(uint32_t, __cdecl, ConvertGbDepthFuncToD3D10Comparison, 0x008B0AB0, uint32_t func);
GAME_FN(uint8_t, __thiscall, Manager_SetAlphaProperty, 0x008B0C70, void* self, const uint8_t* property);
GAME_FN(uint8_t, __thiscall, Manager_SetStencilProperty, 0x008B0D50, void* self, const uint8_t* property);
GAME_FN(uint8_t, __thiscall, Manager_SetWireframeProperty, 0x008B0E70, void* self, const uint8_t* property);
GAME_FN(uint8_t, __thiscall, Manager_SetZBufferProperty, 0x008B0EB0, void* self, const uint8_t* property);

void SetDefaultSampler(SamplerDesc& s) {
    s.filter = 0x15;   // MIN_MAG_MIP_LINEAR
    s.addressU = 3;    // CLAMP
    s.addressV = 3;
    s.addressW = 3;
    s.mipLODBias = 0.0f;
    s.maxAnisotropy = 1;
    s.comparisonFunc = 1;   // NEVER
    s.borderColor[0] = 1.0f;
    s.borderColor[1] = 1.0f;
    s.borderColor[2] = 1.0f;
    s.borderColor[3] = 1.0f;
    s.minLOD = -3.402823466e+38f;   // -FLT_MAX (0x9009B8)
    s.maxLOD = 3.402823466e+38f;    // FLT_MAX (0x9009BC)
}

} // namespace

// 0x8B0A50 (cdecl): NiD3D10RenderStateManager::ConvertGbStencilActionToD3D10StencilOp
// (keep, zero, replace, incr_sat, decr_sat, invert: 1..6; anything else warned about, KEEP)
uint32_t __cdecl RenderStateManager_ConvertGbStencilActionToD3D10StencilOp(uint32_t action) {
    switch (action) {
    case 0: return 1;
    case 1: return 2;
    case 2: return 3;
    case 3: return 4;
    case 4: return 5;
    case 5: return 6;
    default:
        D3D10Warning("Invalid stencil action passed into "
                     "NiD3D10RenderStateManager::ConvertGbStencilActionToD3D10StencilOp");
        return 1;
    }
}

// 0x8B0AB0 (cdecl): NiD3D10RenderStateManager::ConvertGbDepthFuncToD3D10Comparison (always 8,
// less 2, equal 3, less_equal 4, greater 5, not_equal 6, greater_equal 7, never 1)
uint32_t __cdecl RenderStateManager_ConvertGbDepthFuncToD3D10Comparison(uint32_t func) {
    switch (func) {
    case 0: return 8;
    case 1: return 2;
    case 2: return 3;
    case 3: return 4;
    case 4: return 5;
    case 5: return 6;
    case 6: return 7;
    case 7: return 1;
    default:
        D3D10Warning("Invalid Z buffer test function passed into "
                     "NiD3D10RenderStateManager::ConvertGbDepthFuncToD3D10Comparison");
        return 1;
    }
}

// 0x8B0B20 (cdecl): NiD3D10RenderStateManager::ConvertGbFilterModeToD3D10Filter (nearest and
// nearest-mip-nearest point, bilerp and its mip-nearest 0x14, trilerp 0x15, nearest-mip-lerp 1)
uint32_t __cdecl RenderStateManager_ConvertGbFilterModeToD3D10Filter(uint32_t mode) {
    switch (mode) {
    case 0:
    case 3:
        return 0;
    case 1:
    case 5:
        return 0x14;
    case 2:
        return 0x15;
    case 4:
        return 1;
    default:
        D3D10Warning("Invalid filter mode passed into "
                     "NiD3D10RenderStateManager::ConvertGbFilterModeToD3D10Filter");
        return 0;
    }
}

// 0x8B0B70 (cdecl): NiD3D10RenderStateManager::ConvertGbFilterModeToMipmapEnable
uint8_t __cdecl RenderStateManager_ConvertGbFilterModeToMipmapEnable(uint32_t mode) {
    switch (mode) {
    case 0:
    case 1:
        return 0;
    case 2:
    case 3:
    case 4:
    case 5:
        return 1;
    default:
        D3D10Warning("Invalid filter mode passed into "
                     "NiD3D10RenderStateManager::ConvertGbFilterModeToMipmapEnable");
        return 1;
    }
}

// 0x8B0BB0 (cdecl): NiD3D10RenderStateManager::ConvertGbClampModeToD3D10AddressU (clamp_s_*:
// CLAMP 3, wrap_s_*: WRAP 1)
uint32_t __cdecl RenderStateManager_ConvertGbClampModeToD3D10AddressU(uint32_t mode) {
    switch (mode) {
    case 0:
    case 1:
        return 3;
    case 2:
    case 3:
        return 1;
    default:
        D3D10Warning("Invalid address mode passed into "
                     "NiD3D10RenderStateManager::ConvertGbClampModeToD3D10AddressU");
        return 3;
    }
}

// 0x8B0BF0 (cdecl): NiD3D10RenderStateManager::ConvertGbClampModeToD3D10AddressV
uint32_t __cdecl RenderStateManager_ConvertGbClampModeToD3D10AddressV(uint32_t mode) {
    switch (mode) {
    case 0:
    case 2:
        return 3;
    case 1:
    case 3:
        return 1;
    default:
        D3D10Warning("Invalid address mode passed into "
                     "NiD3D10RenderStateManager::ConvertGbClampModeToD3D10AddressV");
        return 3;
    }
}

// 0x8B0C30 (thiscall): NiD3D10RenderStateManager's deleting destructor (the device let go)
RenderStateManager* __fastcall RenderStateManager_DeletingDestructor(RenderStateManager* self, re::Edx,
                                                                     uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(kRenderStateManagerVtable);
    ComRelease(self->device);
    if (flags & 1) {
        NiDelete(self, 0x1500);
    }
    return self;
}

// 0x8B0C70 (thiscall): NiD3D10RenderStateManager::SetAlphaProperty (blending: every target's
// blend enabled, the source and destination factors converted, ADD; otherwise every target's
// blend off and the rest left)
uint8_t __fastcall RenderStateManager_SetAlphaProperty(RenderStateManager* self, re::Edx, const uint8_t* property) {
    if (property == nullptr) {
        D3D10Warning("Cannot apply alpha state with NULL alpha property.");
        return 0;
    }
    BlendDesc& blend = self->current.blend;
    if (At<uint8_t>(property, 0x18) & 1) {
        for (uint32_t& enable : blend.blendEnable) {
            enable = 1;
        }
        blend.srcBlend = ConvertGbAlphaBlendToD3D10Blend((At<uint8_t>(property, 0x18) >> 1) & 0xF);
        blend.destBlend = ConvertGbAlphaBlendToD3D10Blend((PropertyFlags(property) >> 5) & 0xF);
        blend.blendOp = 1;   // ADD
        return 1;
    }
    for (uint32_t& enable : blend.blendEnable) {
        enable = 0;
    }
    return 1;
}

// 0x8B0D50 (thiscall): NiD3D10RenderStateManager::SetStencilProperty (enabled: the mask as both
// read and write masks, the reference, the function and the three actions for both faces; then
// the cull mode from the draw mode: CW culls the front, BOTH none, else the back; the winding
// counter-clockwise)
uint8_t __fastcall RenderStateManager_SetStencilProperty(RenderStateManager* self, re::Edx, const uint8_t* property) {
    if (property == nullptr) {
        D3D10Warning("Cannot apply stencil state with NULL stencil property.");
        return 0;
    }
    DepthStencilDesc& ds = self->current.depthStencil;
    if (At<uint8_t>(property, 0x18) & 1) {
        ds.stencilEnable = 1;
        ds.stencilWriteMask = At<uint8_t>(property, 0x20);
        ds.stencilReadMask = At<uint8_t>(property, 0x20);
        self->current.stencilRef = At<uint32_t>(property, 0x1C);
        ds.frontFace.func = ConvertGbStencilFuncToD3D10Comparison(PropertyFlags(property) >> 12);
        ds.frontFace.depthFailOp = ConvertGbStencilActionToD3D10StencilOp((At<uint8_t>(property, 0x18) >> 4) & 7);
        ds.frontFace.passOp = ConvertGbStencilActionToD3D10StencilOp((PropertyFlags(property) >> 7) & 7);
        uint32_t fail = ConvertGbStencilActionToD3D10StencilOp((At<uint8_t>(property, 0x18) >> 1) & 7);
        ds.frontFace.failOp = fail;
        ds.backFace.failOp = fail;
        ds.backFace.depthFailOp = ds.frontFace.depthFailOp;
        ds.backFace.passOp = ds.frontFace.passOp;
        ds.backFace.func = ds.frontFace.func;
    } else {
        ds.stencilEnable = 0;
    }
    uint32_t draw = (PropertyFlags(property) >> 10) & 3;
    RasterizerDesc& rs = self->current.rasterizer;
    rs.frontCounterClockwise = 1;
    if (draw == 2) {
        rs.cullMode = draw;
        return 1;
    }
    rs.cullMode = (draw != 3 ? 1u : 0u) * 2 + 1;
    return 1;
}

// 0x8B0E70 (thiscall): NiD3D10RenderStateManager::SetWireframeProperty (WIREFRAME 2 / SOLID 3)
uint8_t __fastcall RenderStateManager_SetWireframeProperty(RenderStateManager* self, re::Edx,
                                                           const uint8_t* property) {
    if (property == nullptr) {
        D3D10Warning("Cannot apply wireframe state with NULL wireframe property.");
        return 0;
    }
    self->current.rasterizer.fillMode = (uint8_t(~At<uint8_t>(property, 0x18)) & 1u) | 2u;
    return 1;
}

// 0x8B0EB0 (thiscall): NiD3D10RenderStateManager::SetZBufferProperty (depth on when the test or
// the write is; the write mask; the function when testing, ALWAYS otherwise)
uint8_t __fastcall RenderStateManager_SetZBufferProperty(RenderStateManager* self, re::Edx, const uint8_t* property) {
    if (property == nullptr) {
        D3D10Warning("Cannot apply Z buffer state with NULL Z buffer property.");
        return 0;
    }
    DepthStencilDesc& ds = self->current.depthStencil;
    uint8_t flags = At<uint8_t>(property, 0x18);
    ds.depthEnable = ((flags & 1) == 0 && ((flags >> 1) & 1) == 0) ? 0u : 1u;
    ds.depthWriteMask = (At<uint8_t>(property, 0x18) >> 1) & 1;
    if (At<uint8_t>(property, 0x18) & 1) {
        ds.depthFunc = ConvertGbDepthFuncToD3D10Comparison((At<uint8_t>(property, 0x18) >> 2) & 0xF);
        return 1;
    }
    ds.depthFunc = 8;
    return 1;
}

// 0x8B0F40 (thiscall): NiD3D10RenderStateManager::SetRenderStateGroup (a render state group's
// descriptions taken over, each with its valid bits; its blend factor, sample mask and stencil
// reference where it has them; then its sampler descriptions, the group's sampler map picking
// which of its samplers fills each of the 16 per stage). The descriptions are stack buffers: a
// sampler index of 16 or more leaves the buffer as it was (uninitialised the first time), as in
// the game. The blend factor goes through the x87 (fld / fstp: a copy).
uint8_t __fastcall RenderStateManager_SetRenderStateGroup(RenderStateManager* self, re::Edx, const void* group) {
    uint32_t blendValid = 0;
    BlendDesc blend;
    Group_GetBlendDesc(group, &blend, &blendValid);
    Manager_SetBlendDesc(self, &blend, blendValid);
    float factor[4];
    if (Group_GetBlendFactor(group, factor) != 0) {
        self->current.blendFactor[0] = factor[0];
        self->current.blendFactor[1] = factor[1];
        self->current.blendFactor[2] = factor[2];
        self->current.blendFactor[3] = factor[3];
    }
    uint32_t mask = 0;
    if (Group_GetSampleMask(group, &mask) != 0) {
        self->current.sampleMask = mask;
    }
    uint32_t depthValid = 0;
    DepthStencilDesc depthStencil;
    Group_GetDepthStencilDesc(group, &depthStencil, &depthValid);
    Manager_SetDepthStencilDesc(self, &depthStencil, depthValid);
    uint32_t ref = 0;
    if (Group_GetStencilRef(group, &ref) != 0) {
        self->current.stencilRef = ref;
    }
    uint32_t rasterValid = 0;
    RasterizerDesc rasterizer;
    Group_GetRasterizerDesc(group, &rasterizer, &rasterValid);
    Manager_SetRasterizerDesc(self, &rasterizer, rasterValid);
    uint32_t samplerValid = 0;
    SamplerDesc sampler;
    for (uint32_t stage = 0; stage < 3; stage++) {
        const uint8_t* map = Group_GetSamplerMap(group, stage);
        for (uint32_t i = 0; i < 0x10; i++) {
            samplerValid = 0;
            Group_GetSamplerDesc(group, stage, map[i], &sampler, &samplerValid);
            Manager_SetSamplerDesc(self, stage, i, &sampler, samplerValid);
        }
    }
    return 1;
}

// 0x8B10D0 (thiscall): NiD3D10RenderStateManager::NiD3D10RenderStateManager(device, device
// state): D3D10's own defaults in both sets (blending off, ONE / ZERO / ADD, all channels
// written; depth on, LESS; stencil off; solid, back culled, counter-clockwise, depth
// clip, multisampling and antialiased lines on; every sampler trilinear and clamped), then
// RestoreDefaults
RenderStateManager* __fastcall RenderStateManager_Construct(RenderStateManager* self, re::Edx, void* device,
                                                            void* deviceState) {
    self->deviceState = deviceState;
    self->vtable = reinterpret_cast<void*>(kRenderStateManagerVtable);
    self->device = device;
    self->defaults.sampleMask = 0xFFFFFFFF;
    self->defaults.stencilRef = 0;
    self->current.sampleMask = 0xFFFFFFFF;
    self->current.stencilRef = 0;
    self->unknown14FC = 0;
    ComAddRef(device);
    StateSet* sets[2] = {&self->defaults, &self->current};
    for (StateSet* set : sets) {
        BlendDesc& b = set->blend;
        crt::Memset(&b, 0, sizeof(b));
        b.alphaToCoverageEnable = 0;
        for (uint32_t& enable : b.blendEnable) {
            enable = 0;
        }
        b.srcBlend = 2;        // ONE
        b.destBlend = 1;       // ZERO
        b.blendOp = 1;         // ADD
        b.srcBlendAlpha = 2;
        b.destBlendAlpha = 1;
        b.blendOpAlpha = 1;
        for (uint8_t& write : b.renderTargetWriteMask) {
            write = 0xF;
        }
    }
    for (StateSet* set : sets) {
        DepthStencilDesc& d = set->depthStencil;
        crt::Memset(&d, 0, sizeof(d));
        d.depthEnable = 1;
        d.depthWriteMask = 1;   // ALL
        d.depthFunc = 2;        // LESS
        d.stencilEnable = 0;
        d.stencilReadMask = 0;
        d.stencilWriteMask = 0;
        d.frontFace = {1, 1, 1, 8};   // KEEP x3, ALWAYS
        d.backFace = {1, 1, 1, 8};
    }
    for (StateSet* set : sets) {
        RasterizerDesc& r = set->rasterizer;
        r.fillMode = 3;
        r.cullMode = 3;
        r.frontCounterClockwise = 0;
        r.depthBias = 0;
        r.depthBiasClamp = 0.0f;
        r.slopeScaledDepthBias = 0.0f;
        r.depthClipEnable = 1;
        r.scissorEnable = 0;
        r.multisampleEnable = 0;
        r.antialiasedLineEnable = 0;
    }
    for (uint32_t stage = 0; stage < 3; stage++) {
        crt::Memset(self->defaults.samplers[stage], 0, sizeof(self->defaults.samplers[stage]));
        crt::Memset(self->current.samplers[stage], 0, sizeof(self->current.samplers[stage]));
        for (uint32_t i = 0; i < 0x10; i++) {
            SetDefaultSampler(self->defaults.samplers[stage][i]);
            SetDefaultSampler(self->current.samplers[stage][i]);
        }
    }
    for (StateSet* set : sets) {
        set->blendFactor[0] = 0.0f;
        set->blendFactor[1] = 0.0f;
        set->blendFactor[2] = 0.0f;
        set->blendFactor[3] = 0.0f;
        set->rasterizer.frontCounterClockwise = 1;
        set->rasterizer.multisampleEnable = 1;
        set->rasterizer.antialiasedLineEnable = 1;
    }
    Manager_RestoreDefaults(self);
    return self;
}

// 0x8B14D0 (thiscall): NiD3D10RenderStateManager::SetProperties (an NiPropertyState's alpha
// (+0x8), stencil (+0x24), wireframe (+0x30) and z buffer (+0x34) properties, stopping at the
// first that fails)
uint8_t __fastcall RenderStateManager_SetProperties(RenderStateManager* self, re::Edx, const uint8_t* state) {
    if (state != nullptr && Manager_SetAlphaProperty(self, At<const uint8_t*>(state, 0x8)) != 0 &&
        Manager_SetStencilProperty(self, At<const uint8_t*>(state, 0x24)) != 0 &&
        Manager_SetWireframeProperty(self, At<const uint8_t*>(state, 0x30)) != 0 &&
        Manager_SetZBufferProperty(self, At<const uint8_t*>(state, 0x34)) != 0) {
        return 1;
    }
    return 0;
}

// 0x8B1530 (thiscall): NiD3D10RenderStateManager::ApplyCurrentState (the blend, depth / stencil
// and rasterizer states made and set, then every stage's 16 samplers)
void __fastcall RenderStateManager_ApplyCurrentState(RenderStateManager* self, re::Edx) {
    Manager_UpdateBlendState(self);
    Manager_UpdateDepthStencilState(self);
    Manager_UpdateRasterizerState(self);
    Manager_UpdateVertexSamplers(self, 0, 0x10);
    Manager_UpdateGeometrySamplers(self, 0, 0x10);
    Manager_UpdatePixelSamplers(self, 0, 0x10);
}

#define GB43A_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

GB43A_REVERSED("NiD3D10RenderStateManager::ConvertGbStencilActionToD3D10StencilOp", 0x008B0A50,
               RenderStateManager_ConvertGbStencilActionToD3D10StencilOp);
GB43A_REVERSED("NiD3D10RenderStateManager::ConvertGbDepthFuncToD3D10Comparison", 0x008B0AB0,
               RenderStateManager_ConvertGbDepthFuncToD3D10Comparison);
GB43A_REVERSED("NiD3D10RenderStateManager::ConvertGbFilterModeToD3D10Filter", 0x008B0B20,
               RenderStateManager_ConvertGbFilterModeToD3D10Filter);
GB43A_REVERSED("NiD3D10RenderStateManager::ConvertGbFilterModeToMipmapEnable", 0x008B0B70,
               RenderStateManager_ConvertGbFilterModeToMipmapEnable);
GB43A_REVERSED("NiD3D10RenderStateManager::ConvertGbClampModeToD3D10AddressU", 0x008B0BB0,
               RenderStateManager_ConvertGbClampModeToD3D10AddressU);
GB43A_REVERSED("NiD3D10RenderStateManager::ConvertGbClampModeToD3D10AddressV", 0x008B0BF0,
               RenderStateManager_ConvertGbClampModeToD3D10AddressV);
GB43A_REVERSED("NiD3D10RenderStateManager::DeletingDestructor", 0x008B0C30, RenderStateManager_DeletingDestructor);
GB43A_REVERSED("NiD3D10RenderStateManager::SetAlphaProperty", 0x008B0C70, RenderStateManager_SetAlphaProperty);
GB43A_REVERSED("NiD3D10RenderStateManager::SetStencilProperty", 0x008B0D50, RenderStateManager_SetStencilProperty);
GB43A_REVERSED("NiD3D10RenderStateManager::SetWireframeProperty", 0x008B0E70,
               RenderStateManager_SetWireframeProperty);
GB43A_REVERSED("NiD3D10RenderStateManager::SetZBufferProperty", 0x008B0EB0, RenderStateManager_SetZBufferProperty);
GB43A_REVERSED("NiD3D10RenderStateManager::SetRenderStateGroup", 0x008B0F40,
               RenderStateManager_SetRenderStateGroup);
GB43A_REVERSED("NiD3D10RenderStateManager::NiD3D10RenderStateManager", 0x008B10D0, RenderStateManager_Construct);
GB43A_REVERSED("NiD3D10RenderStateManager::SetProperties", 0x008B14D0, RenderStateManager_SetProperties);
GB43A_REVERSED("NiD3D10RenderStateManager::ApplyCurrentState", 0x008B1530, RenderStateManager_ApplyCurrentState);

} // namespace ni::c43
