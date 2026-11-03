// NiD3D10RenderStateGroup (vtable 0x9A156C: 0 the deleting destructor, 1 DeleteThis; 0xB80 bytes):
// a pass's render states as D3D10 descriptions (src/gamebryo/NiD3D10RenderStateManager.cpp takes
// them over), each field with its "set" bit: +0x8 the blend description (D3D10_BLEND_DESC, 0x44;
// +0xC the blend enables, +0x2C the source blend, +0x30 the destination blend, +0x34 the blend
// op), +0x4C the depth / stencil description (0x34), +0x80 the rasterizer description (0x28),
// +0xA8 the sampler descriptions (3 stages x 16, 0x34 each: +0x0 the filter, +0x4 / +0x8 the
// addressing, +0x30 the max LOD), +0xA68 the blend factor, +0xA78 the sample mask, +0xA7C the
// stencil reference; +0xA80 / +0xA84 / +0xA88 the blend / depth stencil / rasterizer set bits
// (0x2 << target a blend enable, 0x200 the source blend, 0x400 the destination blend, 0x800 the
// op), +0xA8C the samplers' set bits (by stage * 16 + sampler: 1 filter, 2 U, 4 V, 0x200 max LOD),
// +0xB4C the sampler maps (16 bytes per stage), +0xB7C / +0xB7D / +0xB7E whether the blend factor,
// sample mask, stencil reference are set.

#include "gamebryo/NiD3D10C44.h"

namespace ni::c44 {
namespace {

constexpr uintptr_t kGroupVtable = 0x009A156C;
constexpr uint32_t kGroupSize = 0xB80;

enum : int {
    kBlendDesc = 0x8,
    kBlendEnable = 0xC,
    kSrcBlend = 0x2C,
    kDestBlend = 0x30,
    kBlendOp = 0x34,
    kDepthStencilDesc = 0x4C,
    kRasterizerDesc = 0x80,
    kSamplerDescs = 0xA8,
    kBlendFactor = 0xA68,
    kSampleMask = 0xA78,
    kStencilRef = 0xA7C,
    kBlendValid = 0xA80,
    kDepthStencilValid = 0xA84,
    kRasterizerValid = 0xA88,
    kSamplerValid = 0xA8C,
    kSamplerMaps = 0xB4C,
    kBlendFactorSet = 0xB7C,
    kSampleMaskSet = 0xB7D,
    kStencilRefSet = 0xB7E,
};
constexpr uint32_t kSamplerDescSize = 0x34;

// this file's (called where modes apply)
GAME_FN(void, __thiscall, Group_ResetAll, 0x008C0580, void* self);

inline uint32_t& BlendValid(uint8_t* self) {
    return At<uint32_t>(self, kBlendValid);
}
inline uint8_t* SamplerDesc(uint8_t* self, uint32_t index) {
    return self + kSamplerDescs + index * kSamplerDescSize;
}
inline uint32_t& SamplerValid(uint8_t* self, uint32_t index) {
    return At<uint32_t>(self, kSamplerValid + int(index) * 4);
}
inline void CopyWords(void* to, const void* from, int count) {
    uint32_t* t = static_cast<uint32_t*>(to);
    const uint32_t* f = static_cast<const uint32_t*>(from);
    for (int i = 0; i < count; i++) {
        t[i] = f[i];
    }
}

} // namespace

// 0x8C0260 (thiscall): SetBlendEnable (a render target's, 0..7)
void __fastcall Group_SetBlendEnable(uint8_t* self, re::Edx, uint32_t target, int enable) {
    if (target < 8) {
        At<uint32_t>(self, kBlendEnable + int(target) * 4) = uint8_t(enable);
        BlendValid(self) |= 2u << (target & 0x1F);
    }
}

// 0x8C0290 (thiscall): SetSrcBlend
void __fastcall Group_SetSrcBlend(uint8_t* self, re::Edx, uint32_t blend) {
    BlendValid(self) |= 0x200;
    At<uint32_t>(self, kSrcBlend) = blend;
}

// 0x8C02B0 (thiscall): GetSrcBlend (whether it's set)
uint8_t __fastcall Group_GetSrcBlend(uint8_t* self, re::Edx, uint32_t* blend) {
    if ((BlendValid(self) & 0x200) == 0) {
        return 0;
    }
    *blend = At<uint32_t>(self, kSrcBlend);
    return 1;
}

// 0x8C02D0 (thiscall): RemoveSrcBlend
void __fastcall Group_RemoveSrcBlend(uint8_t* self, re::Edx) {
    BlendValid(self) &= ~0x200u;
}

// 0x8C02E0 (thiscall): SetDestBlend
void __fastcall Group_SetDestBlend(uint8_t* self, re::Edx, uint32_t blend) {
    BlendValid(self) |= 0x400;
    At<uint32_t>(self, kDestBlend) = blend;
}

// 0x8C0300 (thiscall): GetDestBlend
uint8_t __fastcall Group_GetDestBlend(uint8_t* self, re::Edx, uint32_t* blend) {
    if ((BlendValid(self) & 0x400) == 0) {
        return 0;
    }
    *blend = At<uint32_t>(self, kDestBlend);
    return 1;
}

// 0x8C0320 (thiscall): RemoveDestBlend
void __fastcall Group_RemoveDestBlend(uint8_t* self, re::Edx) {
    BlendValid(self) &= ~0x400u;
}

// 0x8C0330 (thiscall): SetBlendOp
void __fastcall Group_SetBlendOp(uint8_t* self, re::Edx, uint32_t op) {
    BlendValid(self) |= 0x800;
    At<uint32_t>(self, kBlendOp) = op;
}

// 0x8C0350 (thiscall): GetBlendFactor (whether it's set)
uint8_t __fastcall Group_GetBlendFactor(uint8_t* self, re::Edx, float* factor) {
    if (At<uint8_t>(self, kBlendFactorSet) == 0) {
        return 0;
    }
    factor[0] = At<float>(self, kBlendFactor);
    factor[1] = At<float>(self, kBlendFactor + 4);
    factor[2] = At<float>(self, kBlendFactor + 8);
    factor[3] = At<float>(self, kBlendFactor + 0xC);
    return 1;
}

// 0x8C0390 (thiscall): GetSampleMask
uint8_t __fastcall Group_GetSampleMask(uint8_t* self, re::Edx, uint32_t* mask) {
    if (At<uint8_t>(self, kSampleMaskSet) == 0) {
        return 0;
    }
    *mask = At<uint32_t>(self, kSampleMask);
    return 1;
}

// 0x8C03B0 (thiscall): GetStencilRef
uint8_t __fastcall Group_GetStencilRef(uint8_t* self, re::Edx, uint32_t* ref) {
    if (At<uint8_t>(self, kStencilRefSet) == 0) {
        return 0;
    }
    *ref = At<uint32_t>(self, kStencilRef);
    return 1;
}

// 0x8C03D0 (thiscall): SetSamplerFilter (a stage's sampler, 0..15)
void __fastcall Group_SetSamplerFilter(uint8_t* self, re::Edx, uint32_t stage, uint32_t sampler, uint32_t filter) {
    if (sampler < 0x10) {
        uint32_t index = sampler + (stage << 4);
        At<uint32_t>(SamplerDesc(self, index), 0) = filter;
        SamplerValid(self, index) |= 1;
    }
}

// 0x8C0400 (thiscall): SetSamplerAddressU
void __fastcall Group_SetSamplerAddressU(uint8_t* self, re::Edx, uint32_t stage, uint32_t sampler, uint32_t mode) {
    if (sampler < 0x10) {
        uint32_t index = sampler + (stage << 4);
        At<uint32_t>(SamplerDesc(self, index), 4) = mode;
        SamplerValid(self, index) |= 2;
    }
}

// 0x8C0430 (thiscall): SetSamplerAddressV
void __fastcall Group_SetSamplerAddressV(uint8_t* self, re::Edx, uint32_t stage, uint32_t sampler, uint32_t mode) {
    if (sampler < 0x10) {
        uint32_t index = sampler + (stage << 4);
        At<uint32_t>(SamplerDesc(self, index), 8) = mode;
        SamplerValid(self, index) |= 4;
    }
}

// 0x8C0460 (thiscall): SetSamplerMaxLOD
void __fastcall Group_SetSamplerMaxLOD(uint8_t* self, re::Edx, uint32_t stage, uint32_t sampler, float lod) {
    if (sampler < 0x10) {
        uint32_t index = sampler + (stage << 4);
        At<float>(SamplerDesc(self, index), 0x30) = lod;
        SamplerValid(self, index) |= 0x200;
    }
}

// 0x8C0490 (thiscall): GetSamplerMap (a stage's 16 bytes)
uint8_t* __fastcall Group_GetSamplerMap(uint8_t* self, re::Edx, uint32_t stage) {
    return self + kSamplerMaps + int(stage << 4);
}

// 0x8C04B0 (thiscall): GetBlendDesc (the description and its set bits)
void __fastcall Group_GetBlendDesc(uint8_t* self, re::Edx, void* desc, uint32_t* valid) {
    CopyWords(desc, self + kBlendDesc, 0x11);
    *valid = At<uint32_t>(self, kBlendValid);
}

// 0x8C04E0 (thiscall): GetDepthStencilDesc
void __fastcall Group_GetDepthStencilDesc(uint8_t* self, re::Edx, void* desc, uint32_t* valid) {
    CopyWords(desc, self + kDepthStencilDesc, 0xD);
    *valid = At<uint32_t>(self, kDepthStencilValid);
}

// 0x8C0510 (thiscall): GetRasterizerDesc
void __fastcall Group_GetRasterizerDesc(uint8_t* self, re::Edx, void* desc, uint32_t* valid) {
    CopyWords(desc, self + kRasterizerDesc, 0xA);
    *valid = At<uint32_t>(self, kRasterizerValid);
}

// 0x8C0540 (thiscall): GetSamplerDesc (a stage's sampler, 0..15)
void __fastcall Group_GetSamplerDesc(uint8_t* self, re::Edx, uint32_t stage, uint32_t sampler, void* desc,
                                     uint32_t* valid) {
    if (sampler < 0x10) {
        uint32_t index = sampler + (stage << 4);
        CopyWords(desc, SamplerDesc(self, index), 0xD);
        *valid = SamplerValid(self, index);
    }
}

// 0x8C0580 (thiscall): every state unset (the set bits and flags cleared, the sampler maps zeroed)
void __fastcall Group_ResetAllImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, kBlendValid) = 0;
    At<uint32_t>(self, kDepthStencilValid) = 0;
    At<uint32_t>(self, kRasterizerValid) = 0;
    At<uint8_t>(self, kBlendFactorSet) = 0;
    At<uint8_t>(self, kSampleMaskSet) = 0;
    At<uint8_t>(self, kStencilRefSet) = 0;
    for (int stage = 0; stage < 3; stage++) {
        crt::Memset(self + kSamplerValid + stage * 0x40, 0, 0x40);
        for (int i = 0; i < 0x10; i++) {
            At<uint8_t>(self, kSamplerMaps + stage * 0x10 + i) = 0;
        }
    }
}

// 0x8C05F0 (thiscall): the constructor
void* __fastcall Group_Construct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kRefObjectVtable;
    At<uint32_t>(self, 4) = 0;
    Increment(&ms_uiObjects);
    At<uint32_t>(self, 0) = kGroupVtable;
    At<uint32_t>(self, kBlendValid) = 0;
    At<uint32_t>(self, kDepthStencilValid) = 0;
    At<uint32_t>(self, kRasterizerValid) = 0;
    At<uint8_t>(self, kBlendFactorSet) = 0;
    At<uint8_t>(self, kSampleMaskSet) = 0;
    At<uint8_t>(self, kStencilRefSet) = 0;
    Group_ResetAll(self);
    return self;
}

// 0x8C0640 (thiscall): SetSamplerMap (a stage's 16 bytes; zeros without one)
void __fastcall Group_SetSamplerMap(uint8_t* self, re::Edx, uint32_t stage, const uint32_t* map) {
    uint8_t* to = self + kSamplerMaps + int(stage << 4);
    if (map != nullptr) {
        CopyWords(to, map, 4);
        return;
    }
    for (int i = 0; i < 0x10; i++) {
        to[i] = 0;
    }
}

// 0x8C0690 (thiscall, slot 0): the deleting destructor
void* __fastcall Group_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kRefObjectVtable;
    Decrement(&ms_uiObjects);
    if ((flags & 1) != 0) {
        NiDelete(self, kGroupSize);
    }
    return self;
}

#define GB44R_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

GB44R_REVERSED("NiD3D10RenderStateGroup::SetBlendEnable", 0x008C0260, Group_SetBlendEnable);
GB44R_REVERSED("NiD3D10RenderStateGroup::SetSrcBlend", 0x008C0290, Group_SetSrcBlend);
GB44R_REVERSED("NiD3D10RenderStateGroup::GetSrcBlend", 0x008C02B0, Group_GetSrcBlend);
GB44R_REVERSED("NiD3D10RenderStateGroup::RemoveSrcBlend", 0x008C02D0, Group_RemoveSrcBlend);
GB44R_REVERSED("NiD3D10RenderStateGroup::SetDestBlend", 0x008C02E0, Group_SetDestBlend);
GB44R_REVERSED("NiD3D10RenderStateGroup::GetDestBlend", 0x008C0300, Group_GetDestBlend);
GB44R_REVERSED("NiD3D10RenderStateGroup::RemoveDestBlend", 0x008C0320, Group_RemoveDestBlend);
GB44R_REVERSED("NiD3D10RenderStateGroup::SetBlendOp", 0x008C0330, Group_SetBlendOp);
GB44R_REVERSED("NiD3D10RenderStateGroup::GetBlendFactor", 0x008C0350, Group_GetBlendFactor);
GB44R_REVERSED("NiD3D10RenderStateGroup::GetSampleMask", 0x008C0390, Group_GetSampleMask);
GB44R_REVERSED("NiD3D10RenderStateGroup::GetStencilRef", 0x008C03B0, Group_GetStencilRef);
GB44R_REVERSED("NiD3D10RenderStateGroup::SetSamplerFilter", 0x008C03D0, Group_SetSamplerFilter);
GB44R_REVERSED("NiD3D10RenderStateGroup::SetSamplerAddressU", 0x008C0400, Group_SetSamplerAddressU);
GB44R_REVERSED("NiD3D10RenderStateGroup::SetSamplerAddressV", 0x008C0430, Group_SetSamplerAddressV);
GB44R_REVERSED("NiD3D10RenderStateGroup::SetSamplerMaxLOD", 0x008C0460, Group_SetSamplerMaxLOD);
GB44R_REVERSED("NiD3D10RenderStateGroup::GetSamplerMap", 0x008C0490, Group_GetSamplerMap);
GB44R_REVERSED("NiD3D10RenderStateGroup::GetBlendDesc", 0x008C04B0, Group_GetBlendDesc);
GB44R_REVERSED("NiD3D10RenderStateGroup::GetDepthStencilDesc", 0x008C04E0, Group_GetDepthStencilDesc);
GB44R_REVERSED("NiD3D10RenderStateGroup::GetRasterizerDesc", 0x008C0510, Group_GetRasterizerDesc);
GB44R_REVERSED("NiD3D10RenderStateGroup::GetSamplerDesc", 0x008C0540, Group_GetSamplerDesc);
GB44R_REVERSED("NiD3D10RenderStateGroup::RemoveAllStates", 0x008C0580, Group_ResetAllImpl);
GB44R_REVERSED("NiD3D10RenderStateGroup::NiD3D10RenderStateGroup", 0x008C05F0, Group_Construct);
GB44R_REVERSED("NiD3D10RenderStateGroup::SetSamplerMap", 0x008C0640, Group_SetSamplerMap);
GB44R_REVERSED("NiD3D10RenderStateGroup::DeletingDestructor", 0x008C0690, Group_DeletingDestructor);

} // namespace ni::c44
