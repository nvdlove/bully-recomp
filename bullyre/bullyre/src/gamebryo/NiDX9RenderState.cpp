// NiDX9RenderState (vtable 0x99BDE4: only the destructor and DeleteThis; 0x2240 bytes, made by
// Create from InitializeDevice's caps): the device state cache NiDX9Renderer sets its render
// states, texture stage states, samplers, shaders and declarations through, and how the legacy
// properties (alpha, fog, stencil, z buffer...) turn into render states.
//
// Layout (from the constructor 0x883580): +0x8 flags (1 table fog, 2 z buffer, 4 border
// addressing), +0xC..+0x1C the vertex blend values by bone count, +0x20 the D3DBLEND values by
// NiAlphaProperty function (11), +0x4C the D3DCMP values by test function (8), +0x6C/+0x70 the
// source / destination blend caps as bits of those, +0x74 a default NiAlphaProperty (a
// reference), +0x78..+0x88 the fog values (+0x7C start, +0x80 depth scale, +0x84 end scale),
// +0x8C..+0x94 the fog color and +0x98 it packed, +0x9C the stencil functions, +0xBC the stencil
// actions, +0xD4 the cull modes (two per draw mode, +0xF4 which), +0xF8 the shader constant cache,
// +0x100 the D3DCMP values by z buffer function, +0x120 the render states (8 bytes each: the value
// in force, the saved one; 0x7FFFFFFF unknown), +0x920 the texture stage states (33 a stage),
// +0x19A0 the sampler states (14 a sampler), +0x20A0 the textures (16), +0x20E0 the vertex
// shader, +0x20E4, +0x20E8 the pixel shader, +0x20EC, +0x20F0 the shader constant manager (a
// reference), +0x20F4/+0x20F5 normalize-normals overrides, +0x20F6 software vertex processing,
// +0x20F7 a declaration (not an FVF) in force, +0x20F8 the FVF, +0x2100 the declaration, +0x2108
// the D3DCAPS9 copy (0x130), +0x2238 the device (held), +0x223C the renderer.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::c39 {
using namespace ni::dx9;
namespace {

constexpr uintptr_t kNiRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t kRenderStateVtable = 0x0099BDE4;
constexpr uintptr_t kNiAlphaPropertyVtable = 0x008FFB9C;
constexpr uint32_t kRenderStateSize = 0x2240;
constexpr uint32_t kUnknownState = 0x7FFFFFFF;

GAME_VAR(uint32_t, NiRefObject_ms_uiObjects, 0x020E2168);
GAME_VAR(uint32_t, ms_uiMaxTextureBlendStages, 0x00BCB7F8);
GAME_VAR(uint32_t, ms_uiMaxSimultaneousTextures, 0x00BCB7FC);
GAME_VAR(uint32_t, ms_uiMaxSamplers, 0x00BCB800);
GAME_FN(void*, __cdecl, NiMemObject_New_, 0x00752460, uint32_t size);   // NiMemObject::operator new
GAME_VAR(void*, ms_pkConstantManagerCreator, 0x020E6D70);   // (a 4-byte object: slot 0 makes one)
// the default states: (state, value) pairs ending at -1 (the code changes some of the values)
GAME_ARRAY(int32_t, ms_akDefaultRenderStates, 0x00BCAE30);
GAME_ARRAY(int32_t, ms_akDefaultTextureStageStates, 0x00BCB638);
GAME_ARRAY(int32_t, ms_akDefaultSamplerStates, 0x00BCB780);
GAME_ARRAY(const float, NiFogProperty_ms_kDefaultColor, 0x020E25E8);

GAME_FN(void*, __thiscall, ObjectNET_Construct, 0x00764550, void* self);
GAME_FN(void, __thiscall, ConstantCache_PurgeMaterial, 0x0089DAC0, void* self, void* material);
GAME_FN(void, __thiscall, ConstantCache_Destruct, 0x0089DAF0, void* self);
GAME_FN(void, __thiscall, ConstantCache_SetMaterial, 0x0089DB20, void* self, void* material);
GAME_FN(void, __thiscall, ConstantCache_Apply, 0x0089DAA0, void* self, void* device);
GAME_FN(void, __thiscall, ConstantCache_Reset, 0x0089DB80, void* self);

// this file's (called where modes apply)
GAME_FN(void, __thiscall, RenderState_SetRenderState, 0x00406630, void* self, uint32_t state, uint32_t value);
GAME_FN(void, __thiscall, RenderState_ClearVertexShader, 0x00882560, void* self, void* shader);
GAME_FN(void, __thiscall, RenderState_ClearTextureList, 0x008825B0, void* self);
GAME_FN(void, __thiscall, RenderState_SetD3DRenderer, 0x00882670, void* self, void* renderer);
GAME_FN(void, __thiscall, RenderState_ApplyAlpha, 0x008826E0, void* self, void* property);
GAME_FN(void, __thiscall, RenderState_ApplyFog, 0x008828F0, void* self, void* property);
GAME_FN(void, __thiscall, RenderState_ApplyStencil, 0x00882BC0, void* self, void* property);
GAME_FN(void, __thiscall, RenderState_ApplyZBuffer, 0x00882D80, void* self, void* property);
GAME_FN(void, __thiscall, RenderState_InitRenderStates, 0x00882EE0, void* self);
GAME_FN(void, __thiscall, RenderState_InitTextureStageStates, 0x00882F30, void* self);
GAME_FN(void, __thiscall, RenderState_ReadCaps, 0x008834D0, void* self);
GAME_FN(void*, __thiscall, RenderState_Construct, 0x00883580, void* self, void* renderer, const void* caps);
GAME_FN(void, __thiscall, RenderState_Destruct, 0x00883940, void* self);

inline void* Device(const uint8_t* self) {
    return At<void*>(self, 0x2238);
}

// SetRenderState as the code inlines it: sent only when the cached value differs
inline void SetRS(uint8_t* self, uint32_t state, uint32_t value) {
    uint32_t& cached = At<uint32_t>(self, 0x120 + int(state) * 8);
    if (cached != value) {
        Com<long>(Device(self), 57, state, value);   // SetRenderState
        cached = value;
    }
}

inline uint32_t FloatBits(float value) {
    uint32_t bits;
    std::memcpy(&bits, &value, 4);
    return bits;
}

// a color channel to a byte as the code does it: times 255.0 at the main thread's 24-bit
// precision (a float's rounding), truncated (fistp with the rounding mode set to chop), the low
// byte
inline uint32_t ChannelByte(float channel) {
    float scaled = float(double(channel) * 255.0);
    return uint32_t(int32_t(scaled)) & 0xFF;
}

// D3DCOLOR_XRGB of the fog color at +0x8C
inline uint32_t PackFogColor(const uint8_t* self) {
    uint32_t color = (ChannelByte(At<float>(self, 0x8C)) | 0xFFFFFF00u) << 8;
    color = (color | ChannelByte(At<float>(self, 0x90))) << 8;
    return color | ChannelByte(At<float>(self, 0x94));
}

} // namespace

// 0x882540 (thiscall): NiDX9RenderState::PurgeMaterial (the shader constant cache's)
void __fastcall RenderState_PurgeMaterial(uint8_t* self, re::Edx, void* material) {
    ConstantCache_PurgeMaterial(self + 0xF8, material);
}

// 0x882550 (thiscall): NiDX9RenderState::GetSoftwareVertexProcessing
uint8_t __fastcall RenderState_GetSoftwareVertexProcessing(uint8_t* self, re::Edx) {
    return At<uint8_t>(self, 0x20F6);
}

// 0x882560 (thiscall): a vertex shader forgotten (unset on the device when it's the one in force)
void __fastcall RenderState_ClearVertexShaderImpl(uint8_t* self, re::Edx, void* shader) {
    if (At<void*>(self, 0x20E0) == shader) {
        At<void*>(self, 0x20E0) = nullptr;
        Com<long>(Device(self), 92, static_cast<void*>(nullptr));   // SetVertexShader
    }
    if (At<void*>(self, 0x20E4) == shader) {
        At<void*>(self, 0x20E4) = nullptr;
    }
}

// 0x8825B0 (thiscall): the cached textures forgotten
void __fastcall RenderState_ClearTextureListImpl(uint8_t* self, re::Edx) {
    for (int i = 0; i < 16; i++) {
        At<void*>(self, 0x20A0 + i * 4) = nullptr;
    }
}

// 0x882620: the shader constant manager creator made (a 4-byte object: its vtable 0x8FF18C; a
// static's init; the global left as it was when the allocation fails). Not in Ghidra's list: the
// 64-bit dispatch's body, not hooked in the 32-bit game
void __cdecl RenderState_InitConstantManagerCreatorImpl() {
    void* creator = NiMemObject_New_(4);
    if (creator != nullptr) {
        *static_cast<uint32_t*>(creator) = 0x008FF18C;
        ms_pkConstantManagerCreator = creator;
    }
}

// 0x882650: the shader constant manager creator deleted (a static's shutdown)
void __cdecl RenderState_ShutdownConstantManagerCreator() {
    if (ms_pkConstantManagerCreator != nullptr) {
        NiDelete(ms_pkConstantManagerCreator, 4);
    }
    ms_pkConstantManagerCreator = nullptr;
}

// 0x882670 (thiscall): NiDX9RenderState::SetD3DRenderer (the renderer and its device, held; the
// old device released first)
void __fastcall RenderState_SetD3DRendererImpl(uint8_t* self, re::Edx, uint8_t* renderer) {
    At<uint8_t*>(self, 0x223C) = renderer;
    if (renderer != nullptr) {
        void* device = At<void*>(renderer, kDevice);
        if (Device(self) != nullptr) {
            Com<long>(Device(self), kRelease);
        }
        At<void*>(self, 0x2238) = device;
        if (device != nullptr) {
            Com<long>(device, kAddRef);
        }
        return;
    }
    if (Device(self) != nullptr) {
        Com<long>(Device(self), kRelease);
    }
    At<void*>(self, 0x2238) = nullptr;
}

// 0x8826E0 (thiscall): NiDX9RenderState::ApplyAlpha (blending and its factors, alpha testing and
// its function and reference, the alpha-to-coverage hacks (D3DRS_POINTSIZE 'ATOC', D3DRS_ADAPTIVE
// TESS_Y 'A2M1' / 'A2M0') from the property's no-sort flag)
void __fastcall RenderState_ApplyAlphaImpl(uint8_t* self, re::Edx, uint8_t* alpha) {
    uint16_t flags = At<uint16_t>(alpha, 0x18);
    if ((flags & 1) == 0) {
        SetRS(self, 27, 0);   // ALPHABLENDENABLE
    } else {
        SetRS(self, 27, 1);
        SetRS(self, 19, At<uint32_t>(self, 0x20 + ((flags >> 1) & 0xF) * 4));   // SRCBLEND
        SetRS(self, 20, At<uint32_t>(self, 0x20 + ((At<uint16_t>(alpha, 0x18) >> 5) & 0xF) * 4));   // DESTBLEND
    }
    if (((At<uint16_t>(alpha, 0x18) >> 9) & 1) == 0) {
        SetRS(self, 15, 0);   // ALPHATESTENABLE
    } else {
        SetRS(self, 15, 1);
        SetRS(self, 25, At<uint32_t>(self, 0x4C + ((At<uint16_t>(alpha, 0x18) >> 10) & 7) * 4));   // ALPHAFUNC
        SetRS(self, 24, At<uint8_t>(alpha, 0x1A));   // ALPHAREF
    }
    uint32_t adaptive;
    if (((At<uint16_t>(alpha, 0x18) >> 14) & 1) == 0) {
        SetRS(self, 181, 0);   // ADAPTIVETESS_Y
        adaptive = 0x304D3241;
    } else {
        SetRS(self, 181, 0x434F5441);
        adaptive = 0x314D3241;
    }
    SetRS(self, 154, adaptive);   // POINTSIZE
}

// 0x8828F0 (thiscall): NiDX9RenderState::ApplyFog (fog on: the density, start and end from the
// property's depth; table or vertex fog, range fog by the function; the color packed when it
// changed. The cache compares the floats' bits)
void __fastcall RenderState_ApplyFogImpl(uint8_t* self, re::Edx, uint8_t* fog) {
    if ((At<uint8_t>(fog, 0x18) & 1) == 0) {
        SetRS(self, 28, 0);   // FOGENABLE
        return;
    }
    SetRS(self, 28, 1);
    float depth = At<float>(self, 0x80) * At<float>(fog, 0x1C);
    float start = At<float>(self, 0x7C) - depth;
    float end = At<float>(self, 0x84) * depth + At<float>(self, 0x7C);
    float density = 1.0f / depth;
    SetRS(self, 38, FloatBits(density));   // FOGDENSITY
    SetRS(self, 36, FloatBits(start));     // FOGSTART
    SetRS(self, 37, FloatBits(end));       // FOGEND
    uint32_t function = (At<uint8_t>(fog, 0x18) >> 1) & 3;
    if (function == 0) {
        if (At<uint8_t>(self, 8) & 1) {
            RenderState_SetRenderState(self, 35, 3);    // FOGTABLEMODE LINEAR
            RenderState_SetRenderState(self, 140, 0);   // FOGVERTEXMODE NONE
        }
        SetRS(self, 48, 0);   // RANGEFOGENABLE
    } else if (function == 1) {
        if (At<uint8_t>(self, 8) & 1) {
            RenderState_SetRenderState(self, 35, 0);
            RenderState_SetRenderState(self, 140, 3);
        }
        RenderState_SetRenderState(self, 48, 1);
    } else {
        RenderState_SetRenderState(self, 28, 0);
    }
    if (At<float>(self, 0x8C) != At<float>(fog, 0x20) || At<float>(self, 0x90) != At<float>(fog, 0x24) ||
        At<float>(self, 0x94) != At<float>(fog, 0x28)) {
        uint32_t color = (ChannelByte(At<float>(fog, 0x20)) | 0xFFFFFF00u) << 8;
        color = (color | ChannelByte(At<float>(fog, 0x24))) << 8;
        At<uint32_t>(self, 0x98) = color | ChannelByte(At<float>(fog, 0x28));
        At<uint32_t>(self, 0x8C) = At<uint32_t>(fog, 0x20);
        At<uint32_t>(self, 0x90) = At<uint32_t>(fog, 0x24);
        At<uint32_t>(self, 0x94) = At<uint32_t>(fog, 0x28);
    }
    SetRS(self, 34, At<uint32_t>(self, 0x98));   // FOGCOLOR
}

// 0x882BC0 (thiscall): NiDX9RenderState::ApplyStencil (the test, its function, reference, mask
// and actions; the cull mode by the draw mode)
void __fastcall RenderState_ApplyStencilImpl(uint8_t* self, re::Edx, uint8_t* stencil) {
    if ((At<uint8_t>(stencil, 0x18) & 1) == 0) {
        SetRS(self, 52, 0);   // STENCILENABLE
    } else {
        SetRS(self, 52, 1);
        SetRS(self, 56, At<uint32_t>(self, 0x9C + (At<uint16_t>(stencil, 0x18) >> 12) * 4));   // STENCILFUNC
        SetRS(self, 57, At<uint32_t>(stencil, 0x1C));   // STENCILREF
        SetRS(self, 58, At<uint32_t>(stencil, 0x20));   // STENCILMASK
        SetRS(self, 53, At<uint32_t>(self, 0xBC + ((At<uint8_t>(stencil, 0x18) >> 1) & 7) * 4));   // STENCILFAIL
        SetRS(self, 54, At<uint32_t>(self, 0xBC + ((At<uint8_t>(stencil, 0x18) >> 4) & 7) * 4));   // STENCILZFAIL
        SetRS(self, 55, At<uint32_t>(self, 0xBC + ((At<uint16_t>(stencil, 0x18) >> 7) & 7) * 4));   // STENCILPASS
    }
    uint32_t mode = (At<uint16_t>(stencil, 0x18) >> 10) & 3;
    SetRS(self, 22, At<uint32_t>(self, 0xD4 + int(At<uint32_t>(self, 0xF4) + mode * 2) * 4));   // CULLMODE
}

// 0x882D80 (thiscall): NiDX9RenderState::ApplyZBuffer (z testing (always when only writing), its
// function, z writes)
void __fastcall RenderState_ApplyZBufferImpl(uint8_t* self, re::Edx, uint8_t* zbuffer) {
    uint8_t flags = At<uint8_t>(zbuffer, 0x18);
    if ((flags & 1) == 0 && ((flags >> 1) & 1) == 0) {
        SetRS(self, 7, 0);   // ZENABLE
    } else {
        SetRS(self, 7, 1);
        uint32_t function = (At<uint8_t>(zbuffer, 0x18) & 1) == 0
                                ? 8   // D3DCMP_ALWAYS
                                : At<uint32_t>(self, 0x100 + ((At<uint8_t>(zbuffer, 0x18) >> 2) & 0xF) * 4);
        SetRS(self, 23, function);   // ZFUNC
    }
    SetRS(self, 14, (At<uint8_t>(zbuffer, 0x18) >> 1) & 1);   // ZWRITEENABLE
}

// 0x882E60 (thiscall): normals normalized unless the transform's scale is 0.99..1.01 (and
// neither override is set). (Name a guess: NiDX9RenderState::SetModelTransform.)
void __fastcall RenderState_SetModelTransform(uint8_t* self, re::Edx, const uint8_t* transform) {
    float scale = At<float>(transform, 0x30);
    if (double(scale) >= double(0.99f) && 1.01f >= scale && At<uint8_t>(self, 0x20F5) == 0 &&
        At<uint8_t>(self, 0x20F4) == 0) {
        RenderState_SetRenderState(self, 143, 0);   // NORMALIZENORMALS
        return;
    }
    SetRS(self, 143, 1);
}

// 0x882EE0 (thiscall): the default render states sent and cached (both values)
void __fastcall RenderState_InitRenderStatesImpl(uint8_t* self, re::Edx) {
    for (int32_t* entry = ms_akDefaultRenderStates; entry[0] != -1; entry += 2) {
        uint32_t value = uint32_t(entry[1]);
        At<uint32_t>(self, 0x120 + entry[0] * 8) = value;
        At<uint32_t>(self, 0x124 + entry[0] * 8) = value;
        Com<long>(Device(self), 57, uint32_t(entry[0]), uint32_t(entry[1]));
    }
}

// 0x882F30 (thiscall): the default texture stage states for every stage (the texture coordinate
// index the stage's; past stage 0 the color and alpha operations disabled: written into the
// table, which keeps them) and sampler states for every sampler, sent and cached
void __fastcall RenderState_InitTextureStageStatesImpl(uint8_t* self, re::Edx) {
    for (uint32_t stage = 0; stage < ms_uiMaxTextureBlendStages; stage++) {
        int base = 0x124 + int(stage) * 0x21;
        for (int32_t* entry = ms_akDefaultTextureStageStates; entry[0] != -1; entry += 2) {
            int32_t type = entry[0];
            if (type == 11) {   // TEXCOORDINDEX
                entry[1] = int32_t(stage);
            } else if ((type == 1 || type == 4) && base != 0x124) {   // COLOROP, ALPHAOP
                entry[1] = 1;   // D3DTOP_DISABLE
            }
            uint32_t value = uint32_t(entry[1]);
            At<uint32_t>(self, (base + type) * 8) = value;
            At<uint32_t>(self, (base + type) * 8 + 4) = value;
            Com<long>(Device(self), 67, stage, uint32_t(type), uint32_t(entry[1]));   // SetTextureStageState
        }
    }
    for (uint32_t sampler = 0; sampler < ms_uiMaxSamplers; sampler++) {
        int base = 0x334 + int(sampler) * 0xE;
        for (int32_t* entry = ms_akDefaultSamplerStates; entry[0] != -1; entry += 2) {
            int32_t type = entry[0];
            uint32_t value = uint32_t(entry[1]);
            At<uint32_t>(self, (base + type) * 8) = value;
            At<uint32_t>(self, (base + type) * 8 + 4) = value;
            Com<long>(Device(self), 69, sampler, uint32_t(type), uint32_t(entry[1]));   // SetSamplerState
        }
    }
}

// 0x883040 (thiscall): NiDX9RenderState::InitRenderState (the defaults sent; FVF and declaration,
// shaders, textures and the constant cache reset; the states the legacy properties use set to
// their defaults; the fog color from NiFogProperty's default)
void __fastcall RenderState_InitRenderStateImpl(uint8_t* self, re::Edx) {
    RenderState_InitRenderStates(self);
    At<uint8_t>(self, 0x20F7) = 0;
    At<uint32_t>(self, 0x20F8) = 0;
    At<uint32_t>(self, 0x20FC) = 1;
    At<uint32_t>(self, 0x2100) = 0;
    At<uint32_t>(self, 0x2104) = 1;
    At<uint32_t>(self, 0x20E0) = 0;
    At<uint32_t>(self, 0x20E4) = 1;
    At<uint32_t>(self, 0x20E8) = 0;
    At<uint32_t>(self, 0x20EC) = 1;
    RenderState_InitTextureStageStates(self);
    RenderState_ClearTextureList(self);
    ConstantCache_Reset(self + 0xF8);
    SetRS(self, 22, 2);    // CULLMODE CW
    SetRS(self, 8, 3);     // FILLMODE SOLID
    SetRS(self, 9, 2);     // SHADEMODE GOURAUD
    SetRS(self, 27, 0);    // ALPHABLENDENABLE
    SetRS(self, 19, At<uint32_t>(self, 0x38));
    SetRS(self, 20, At<uint32_t>(self, 0x3C));
    SetRS(self, 15, 0);    // ALPHATESTENABLE
    SetRS(self, 25, 8);    // ALPHAFUNC ALWAYS
    SetRS(self, 24, 0);    // ALPHAREF
    SetRS(self, 23, 8);    // ZFUNC ALWAYS
    SetRS(self, 14, 0);    // ZWRITEENABLE
    SetRS(self, 26, 0);    // DITHERENABLE
    SetRS(self, 29, 0);    // SPECULARENABLE
    SetRS(self, 28, 0);    // FOGENABLE
    SetRS(self, 35, (At<uint8_t>(self, 8) & 1) ? 3u : 0u);   // FOGTABLEMODE
    SetRS(self, 140, 3);   // FOGVERTEXMODE LINEAR
    SetRS(self, 48, 0);    // RANGEFOGENABLE
    SetRS(self, 34, 0);    // FOGCOLOR
    SetRS(self, 143, 1);   // NORMALIZENORMALS
    SetRS(self, 7, 1);     // ZENABLE
    SetRS(self, 141, 1);   // COLORVERTEX
    At<float>(self, 0x8C) = NiFogProperty_ms_kDefaultColor[0];
    At<float>(self, 0x90) = NiFogProperty_ms_kDefaultColor[1];
    At<float>(self, 0x94) = NiFogProperty_ms_kDefaultColor[2];
    At<uint32_t>(self, 0x98) = PackFogColor(self);
}

// 0x883480 (thiscall): NiDX9RenderState::SetSoftwareVertexProcessing (only when the device can
// switch; the vertex shader in force set again after)
void __fastcall RenderState_SetSoftwareVertexProcessing(uint8_t* self, re::Edx, int software) {
    uint8_t value = uint8_t(software);
    if (value == At<uint8_t>(self, 0x20F6) || At<uint8_t>(At<uint8_t*>(self, 0x223C), kSWVertexSwitchable) == 0) {
        return;
    }
    At<uint8_t>(self, 0x20F6) = value;
    Com<long>(Device(self), 77, uint32_t(value));   // SetSoftwareVertexProcessing
    RenderState_ClearVertexShader(self, At<void*>(self, 0x20E0));
}

// 0x8834D0 (thiscall): the flags from the caps copy (table fog when both fog table and w fog are
// there; the blend caps as bits of the D3DBLEND values (the value masked with the caps, as the
// library does it); border addressing) and software processing when the device has only that
void __fastcall RenderState_ReadCapsImpl(uint8_t* self, re::Edx) {
    uint32_t raster = At<uint32_t>(self, 0x212C);   // RasterCaps
    if ((raster & 0x100) && (raster & 0x100000)) {
        At<uint32_t>(self, 8) |= 1;
    } else {
        At<uint32_t>(self, 8) &= ~1u;
    }
    uint32_t sourceCaps = At<uint32_t>(self, 0x2134);
    uint32_t destinationCaps = At<uint32_t>(self, 0x2138);
    At<uint32_t>(self, 0x70) = 0;
    At<uint32_t>(self, 0x6C) = 0;
    for (uint32_t i = 0; i < 11; i++) {
        uint32_t blend = At<uint32_t>(self, 0x20 + int(i) * 4);
        if (blend & sourceCaps) {
            At<uint32_t>(self, 0x6C) |= 1u << i;
        }
        if (blend & destinationCaps) {
            At<uint32_t>(self, 0x70) |= 1u << i;
        }
    }
    if (At<uint8_t>(self, 0x2154) & 0x10) {   // TextureAddressCaps BORDER
        At<uint32_t>(self, 8) |= 4;
    } else {
        At<uint32_t>(self, 8) &= ~4u;
    }
    uint8_t* renderer = At<uint8_t*>(self, 0x223C);
    At<uint8_t>(self, 0x20F6) =
        (At<uint8_t>(renderer, kSWVertexCapable) != 0 && At<uint8_t>(renderer, kSWVertexSwitchable) == 0) ? 1 : 0;
}

// 0x883580 (thiscall): NiDX9RenderState::NiDX9RenderState(renderer, caps)
uint8_t* __fastcall RenderState_ConstructImpl(uint8_t* self, re::Edx, uint8_t* renderer, const void* caps) {
    At<uint32_t>(self, 0) = kNiRefObjectVtable;
    At<uint32_t>(self, 4) = 0;
    Increment(&NiRefObject_ms_uiObjects);
    At<uint32_t>(self, 0) = kRenderStateVtable;
    At<void*>(self, 0x74) = nullptr;
    At<float>(self, 0x8C) = 0.0f;
    At<float>(self, 0x90) = 0.0f;
    At<float>(self, 0x94) = 0.0f;
    // (the shader constant cache at +0xF8 has an empty constructor)
    for (int i = 0; i < 0x100; i++) {
        At<uint32_t>(self, 0x120 + i * 8) = kUnknownState;
        At<uint32_t>(self, 0x124 + i * 8) = kUnknownState;
    }
    for (int i = 0; i < 0x210; i++) {
        At<uint32_t>(self, 0x920 + i * 8) = kUnknownState;
        At<uint32_t>(self, 0x924 + i * 8) = kUnknownState;
    }
    for (int i = 0; i < 0xE0; i++) {
        At<uint32_t>(self, 0x19A0 + i * 8) = kUnknownState;
        At<uint32_t>(self, 0x19A4 + i * 8) = kUnknownState;
    }
    At<void*>(self, 0x20F0) = nullptr;
    At<uint8_t>(self, 0x20F6) = 0;
    At<void*>(self, 0x2238) = nullptr;
    At<void*>(self, 0x223C) = nullptr;
    RenderState_SetD3DRenderer(self, renderer);
    // the vertex blend values by bone count
    At<uint32_t>(self, 0xC) = 0;
    At<uint32_t>(self, 0x10) = 1;
    At<uint32_t>(self, 0x14) = 1;
    At<uint32_t>(self, 0x18) = 2;
    At<uint32_t>(self, 0x1C) = 3;
    At<uint32_t>(self, 0x6C) = 0;
    At<uint32_t>(self, 0x70) = 0;
    // the default alpha property (blending one / one-minus-source-alpha... as 0xEC says)
    uint8_t* alpha = static_cast<uint8_t*>(NiNew(0x1C));
    if (alpha != nullptr) {
        ObjectNET_Construct(alpha);
        At<uint32_t>(alpha, 0) = kNiAlphaPropertyVtable;
        At<uint8_t>(alpha, 0x1A) = 0;
        At<uint16_t>(alpha, 0x18) = 0xEC;
    }
    void* old = At<void*>(self, 0x74);
    if (old != alpha) {
        ReleaseRef(old);
        At<void*>(self, 0x74) = alpha;
        AddRefRef(alpha);
    }
    At<float>(self, 0x78) = 0.0f;
    At<uint32_t>(self, 0x2C) = 4;
    At<float>(self, 0x7C) = 0.0f;
    At<uint32_t>(self, 0x58) = 4;
    At<float>(self, 0x80) = 0.0f;
    At<uint32_t>(self, 0xA8) = 4;
    At<float>(self, 0x84) = 0.0f;
    At<uint32_t>(self, 0xC8) = 4;
    At<float>(self, 0x88) = 1.0f;
    // the D3DBLEND, D3DCMP, stencil, cull and z function tables
    static const struct {
        int16_t offset;
        uint16_t value;
    } kTables[] = {
        {0x38, 5}, {0x5C, 5}, {0xAC, 5}, {0xCC, 5}, {0x20, 2}, {0x24, 1}, {0x28, 3}, {0x30, 9},
        {0x34, 10}, {0x3C, 6}, {0x40, 7}, {0x44, 8}, {0x48, 11}, {0x4C, 8}, {0x50, 2}, {0x54, 3},
        {0x60, 6}, {0x64, 7}, {0x68, 1}, {0xB8, 8}, {0xA0, 2}, {0xA4, 3}, {0xB0, 6}, {0xB4, 7},
        {0x9C, 1}, {0xBC, 1}, {0xC0, 2}, {0xC4, 3}, {0xD0, 6}, {0xD4, 2}, {0xDC, 2}, {0xE4, 3},
        {0xEC, 1}, {0xD8, 3}, {0xE0, 3}, {0xE8, 2}, {0xF0, 1}, {0xF4, 0},
    };
    for (const auto& entry : kTables) {
        At<uint32_t>(self, entry.offset) = entry.value;
    }
    At<float>(self, 0x8C) = NiFogProperty_ms_kDefaultColor[0];
    At<float>(self, 0x90) = NiFogProperty_ms_kDefaultColor[1];
    At<float>(self, 0x94) = NiFogProperty_ms_kDefaultColor[2];
    At<uint32_t>(self, 0x114) = 6;
    At<uint32_t>(self, 8) = 0;
    At<uint32_t>(self, 0x100) = 8;
    At<uint32_t>(self, 0x104) = 2;
    At<uint32_t>(self, 0x108) = 3;
    At<uint32_t>(self, 0x10C) = 4;
    At<uint32_t>(self, 0x98) = PackFogColor(self);
    At<uint32_t>(self, 0x110) = 5;
    At<uint32_t>(self, 0x118) = 7;
    At<uint32_t>(self, 0x11C) = 1;
    At<uint8_t>(self, 0x20F4) = 0;
    At<uint8_t>(self, 0x20F5) = 0;
    crt::MemcpyS(self + 0x2108, 0x130, caps, 0x130);
    return self;
}

// 0x883940 (thiscall): NiDX9RenderState::~NiDX9RenderState
void __fastcall RenderState_DestructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kRenderStateVtable;
    if (At<void*>(self, 0x74) != nullptr) {
        ReleaseRef(At<void*>(self, 0x74));
        At<void*>(self, 0x74) = nullptr;
    }
    At<void*>(self, 0x223C) = nullptr;
    if (Device(self) != nullptr) {
        Com<long>(Device(self), kRelease);
    }
    At<void*>(self, 0x2238) = nullptr;
    ReleaseRef(At<void*>(self, 0x20F0));
    ConstantCache_Destruct(self + 0xF8);
    ReleaseRef(At<void*>(self, 0x74));
    At<uint32_t>(self, 0) = kNiRefObjectVtable;
    Decrement(&NiRefObject_ms_uiObjects);
}

// 0x8839F0 (thiscall): NiDX9RenderState::UpdateRenderState (from a property state: wireframe,
// shading, alpha, z buffer, dither, specular, stencil, the material into the constant cache, the
// cache applied, fog)
void __fastcall RenderState_UpdateRenderState(uint8_t* self, re::Edx, const uint8_t* state) {
    SetRS(self, 8, (~At<uint8_t>(At<uint8_t*>(state, 0x30), 0x18) & 1) | 2);           // FILLMODE
    SetRS(self, 9, (At<uint8_t>(At<uint8_t*>(state, 0x1C), 0x18) & 1) != 0 ? 2u : 1u);   // SHADEMODE
    RenderState_ApplyAlpha(self, At<void*>(state, 8));
    RenderState_ApplyZBuffer(self, At<void*>(state, 0x34));
    SetRS(self, 26, At<uint8_t>(At<uint8_t*>(state, 0xC), 0x18) & 1);    // DITHERENABLE
    SetRS(self, 29, At<uint8_t>(At<uint8_t*>(state, 0x20), 0x18) & 1);   // SPECULARENABLE
    RenderState_ApplyStencil(self, At<void*>(state, 0x24));
    ConstantCache_SetMaterial(self + 0xF8, At<void*>(state, 0x14));
    ConstantCache_Apply(self + 0xF8, Device(self));
    RenderState_ApplyFog(self, At<void*>(state, 0x10));
}

// 0x883B10: NiDX9RenderState::Create (constructed, the caps read; the default z enable state set
// in the table; the z buffer flag; the shader constant manager from its creator)
void* __cdecl RenderState_Create(void* renderer, const void* caps, int zBuffer) {
    void* memory = NiNew(kRenderStateSize);
    uint8_t* state = memory != nullptr ? static_cast<uint8_t*>(RenderState_Construct(memory, renderer, caps)) : nullptr;
    RenderState_ReadCaps(state);
    uint8_t z = uint8_t(zBuffer);
    for (int i = 0; i < 0x100; i++) {
        if (ms_akDefaultRenderStates[i * 2] == -1) {
            break;
        }
        if (ms_akDefaultRenderStates[i * 2] == 7) {   // ZENABLE
            ms_akDefaultRenderStates[i * 2 + 1] = z != 0 ? 1 : 0;
        }
    }
    if (z != 0) {
        At<uint32_t>(state, 8) |= 2;
    }
    void* manager = Virtual<void*>(ms_pkConstantManagerCreator, 0, renderer, caps);
    void* old = At<void*>(state, 0x20F0);
    if (old != manager) {
        ReleaseRef(old);
        At<void*>(state, 0x20F0) = manager;
        AddRefRef(manager);
    }
    return state;
}

// 0x883C30 (thiscall): NiDX9RenderState's deleting destructor
uint8_t* __fastcall RenderState_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    RenderState_Destruct(self);
    if (flags & 1) {
        NiDelete(self, kRenderStateSize);
    }
    return self;
}

// 0x883CD0, 0x883CE0, 0x883CF0: the device's limits (from InitializeDeviceCaps)
void __cdecl RenderState_SetMaxTextureBlendStages(uint32_t stages) {
    ms_uiMaxTextureBlendStages = stages;
}
void __cdecl RenderState_SetMaxSimultaneousTextures(uint32_t textures) {
    ms_uiMaxSimultaneousTextures = textures;
}
void __cdecl RenderState_SetMaxSamplers(uint32_t samplers) {
    ms_uiMaxSamplers = samplers;
}

#define C39C_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C39C_REVERSED("NiDX9RenderState::PurgeMaterial", 0x00882540, RenderState_PurgeMaterial);
C39C_REVERSED("NiDX9RenderState::GetSoftwareVertexProcessing", 0x00882550, RenderState_GetSoftwareVertexProcessing);
C39C_REVERSED("NiDX9RenderState::ClearVertexShader", 0x00882560, RenderState_ClearVertexShaderImpl);
C39C_REVERSED("NiDX9RenderState::ClearTextureList", 0x008825B0, RenderState_ClearTextureListImpl);
RE_UNHOOKED_FN("NiDX9RenderState::InitConstantManagerCreator", 0x00882620, &RenderState_InitConstantManagerCreatorImpl);
C39C_REVERSED("NiDX9RenderState::ShutdownConstantManagerCreator", 0x00882650,
              RenderState_ShutdownConstantManagerCreator);
C39C_REVERSED("NiDX9RenderState::SetD3DRenderer", 0x00882670, RenderState_SetD3DRendererImpl);
C39C_REVERSED("NiDX9RenderState::ApplyAlpha", 0x008826E0, RenderState_ApplyAlphaImpl);
C39C_REVERSED("NiDX9RenderState::ApplyFog", 0x008828F0, RenderState_ApplyFogImpl);
C39C_REVERSED("NiDX9RenderState::ApplyStencil", 0x00882BC0, RenderState_ApplyStencilImpl);
C39C_REVERSED("NiDX9RenderState::ApplyZBuffer", 0x00882D80, RenderState_ApplyZBufferImpl);
C39C_REVERSED("NiDX9RenderState::SetModelTransform", 0x00882E60, RenderState_SetModelTransform);
C39C_REVERSED("NiDX9RenderState::InitRenderStates", 0x00882EE0, RenderState_InitRenderStatesImpl);
C39C_REVERSED("NiDX9RenderState::InitTextureStageStates", 0x00882F30, RenderState_InitTextureStageStatesImpl);
C39C_REVERSED("NiDX9RenderState::InitRenderState", 0x00883040, RenderState_InitRenderStateImpl);
C39C_REVERSED("NiDX9RenderState::SetSoftwareVertexProcessing", 0x00883480, RenderState_SetSoftwareVertexProcessing);
C39C_REVERSED("NiDX9RenderState::ReadCaps", 0x008834D0, RenderState_ReadCapsImpl);
C39C_REVERSED("NiDX9RenderState::NiDX9RenderState", 0x00883580, RenderState_ConstructImpl);
C39C_REVERSED("NiDX9RenderState::~NiDX9RenderState", 0x00883940, RenderState_DestructImpl);
C39C_REVERSED("NiDX9RenderState::UpdateRenderState", 0x008839F0, RenderState_UpdateRenderState);
C39C_REVERSED("NiDX9RenderState::Create", 0x00883B10, RenderState_Create);
C39C_REVERSED("NiDX9RenderState::DeletingDestructor", 0x00883C30, RenderState_DeletingDestructor);
C39C_REVERSED("NiDX9RenderState::SetMaxTextureBlendStages", 0x00883CD0, RenderState_SetMaxTextureBlendStages);
C39C_REVERSED("NiDX9RenderState::SetMaxSimultaneousTextures", 0x00883CE0, RenderState_SetMaxSimultaneousTextures);
C39C_REVERSED("NiDX9RenderState::SetMaxSamplers", 0x00883CF0, RenderState_SetMaxSamplers);

} // namespace ni::c39
