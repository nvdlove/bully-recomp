// NiD3D10Renderer's helper objects (src/gamebryo/NiD3D10Renderer.h):
//
// The shader constant manager (vtable 0x9A0284, 0xBC bytes, the renderer's +0x454; name a
// guess): +0x4 the device state cache, +0x8 / +0x44 / +0x80 fifteen constant buffers for the vertex,
// geometry and pixel shaders (held).
// The resource factory (vtable 0x9A0294, 8 bytes, the renderer's +0x450): +0x4 the device (held);
// buffers and 2D textures made with the errors reported.
// NiD3D10RenderStateManager (the renderer's +0x44C, 0x1500 bytes; its constructor is not this
// chunk's): +0x4 the device, +0x8 the device state cache, the default states (+0xC the blend
// description 0x44, +0x50 the depth/stencil one 0x34, +0x84 the rasterizer one 0x28, +0xAC the
// samplers 3 x 16 x 0x34, +0xA6C the blend factor and sample mask, +0xA80 the stencil reference),
// the current ones (+0xA84 blend, +0xAC8 depth/stencil, +0xAFC rasterizer, +0xB24 samplers (vertex,
// pixel, geometry), +0x14E4 blend factor, +0x14F4 sample mask, +0x14F8 stencil reference), +0x14FC
// the renderer's left-right swap. The setters take a mask of the fields to change.

#include "gamebryo/NiD3D10Renderer.h"
#include "reversed.h"

namespace ni::c42 {
namespace {

using namespace ni::d3d10;

constexpr uintptr_t kShaderConstantManagerVtable = 0x009A0284;
constexpr uintptr_t kResourceFactoryVtable = 0x009A0294;

// ID3D10Device slots
enum : int {
    kAddRef = 1,
    kCreateBuffer = 71,
    kCreateTexture2D = 73,
    kCreateBlendState = 83,
    kCreateDepthStencilState = 84,
    kCreateRasterizerState = 85,
    kCreateSamplerState = 86,
};

GAME_FN(void*, __thiscall, ConstantMap_GetShaderBuffer, 0x0077E8E0, void* self);   // (+0x1C)
GAME_FN(void*, __thiscall, Object_Get8, 0x00856850, void* self);                  // (+0x8)
GAME_FN(int32_t, __thiscall, ConstantMap_GetIndex, 0x00853140, void* self);         // (+0xC)
GAME_FN(re::Ptr32<char>*, __thiscall, ConstantMap_GetName, 0x0046FB80, void* self);           // (+0x20)
GAME_FN(uint32_t, __thiscall, Program_GetConstantBufferCount, 0x007E2BE0, void* self);   // (+0x20)
GAME_FN(re::Ptr32<char>*, __thiscall, Program_GetConstantBufferName, 0x008BACB0, void* self, re::Ptr32<char>* name, uint32_t index);
GAME_FN(void, __thiscall, FixedString_AssignChars, 0x00406730, re::Ptr32<char>* self, const char* text);
GAME_FN(void, __thiscall, FixedString_Destruct, 0x00517750, re::Ptr32<char>* self);
// the device state cache's (not this chunk's)
GAME_FN(void, __thiscall, StateCache_VSSetConstantBuffers, 0x008B3C80, void* self, uint32_t start, uint32_t count,
        const re::Ptr32<void>* buffers);
GAME_FN(void, __thiscall, StateCache_GSSetConstantBuffers, 0x008B3D20, void* self, uint32_t start, uint32_t count,
        const re::Ptr32<void>* buffers);
GAME_FN(void, __thiscall, StateCache_PSSetConstantBuffers, 0x008B3DC0, void* self, uint32_t start, uint32_t count,
        const re::Ptr32<void>* buffers);
GAME_FN(void, __thiscall, StateCache_OMSetBlendState, 0x008B3780, void* self, void* state, const float* factor,
        uint32_t mask);
GAME_FN(void, __thiscall, StateCache_OMSetDepthStencilState, 0x008B3800, void* self, void* state, uint32_t ref);
GAME_FN(void, __thiscall, StateCache_RSSetState, 0x008B3850, void* self, void* state);
GAME_FN(void, __thiscall, StateCache_VSSetSamplers, 0x008B3890, void* self, uint32_t start, uint32_t count,
        const re::Ptr32<void>* samplers);
GAME_FN(void, __thiscall, StateCache_GSSetSamplers, 0x008B3910, void* self, uint32_t start, uint32_t count,
        const re::Ptr32<void>* samplers);
GAME_FN(void, __thiscall, StateCache_PSSetSamplers, 0x008B3990, void* self, uint32_t start, uint32_t count,
        const re::Ptr32<void>* samplers);
GAME_FN(void, __thiscall, ShaderConstantManager_Destruct, 0x008AFC10, void* self);

// a description's fields copied under a mask: dwords, then bytes, by bit
inline void MaskedCopy(uint8_t* to, const uint8_t* from, uint32_t mask, const int* offsets, const int* sizes,
                       int count) {
    for (int i = 0; i < count; i++) {
        if ((mask & (1u << i)) == 0) {
            continue;
        }
        if (sizes[i] == 1) {
            to[offsets[i]] = from[offsets[i]];
        } else {
            At<uint32_t>(to, offsets[i]) = At<uint32_t>(from, offsets[i]);
        }
    }
}

// one shader stage's samplers made from the current descriptions and set (the ones that can't
// be made reported, left null)
void ApplySamplers(uint8_t* self, int descriptions, uint32_t start, uint32_t count,
                   void(__thiscall* const& set)(void*, uint32_t, uint32_t, const re::Ptr32<void>*)) {
    if (start >= 0x10) {
        return;
    }
    if (count > 0x10 - start) {
        count = 0x10 - start;
    }
    re::Ptr32<void> samplers[16];
    crt::Memset(samplers, 0, 0x40);
    uint8_t* description = self + descriptions + start * 0x34;
    for (uint32_t i = 0; i < count; i++) {
        long hr = Com<long>(At<void*>(self, 4), kCreateSamplerState, description, &samplers[i]);
        description += 0x34;
        if (hr < 0) {
            Error(0xE, Text(0x0099E584), hr);
        } else if (samplers[i] == nullptr) {
            Error(0xE, Text(0x009A03B8));
        }
    }
    set(At<void*>(self, 8), start, count, samplers);
    for (uint32_t i = 0; i < count; i++) {
        ComRelease(samplers[i]);
    }
}

} // namespace

// ---- the shader constant manager (0x9A0284)

// 0x8AFBC0 (thiscall): its constructor (the device state cache; no buffers)
uint8_t* __fastcall NiD3D10ShaderConstantManager_Construct(uint8_t* self, re::Edx, void* stateCache) {
    At<uint32_t>(self, 0) = kShaderConstantManagerVtable;
    At<void*>(self, 4) = stateCache;
    crt::Memset(self + 8, 0, 0x3C);
    crt::Memset(self + 0x44, 0, 0x3C);
    crt::Memset(self + 0x80, 0, 0x3C);
    return self;
}

// 0x8AFC10 (thiscall): its destructor (the buffers let go)
void __fastcall NiD3D10ShaderConstantManager_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kShaderConstantManagerVtable;
    for (int i = 0; i < 15; i++) {
        ComRelease(At<void*>(self, 8 + i * 4));
        ComRelease(At<void*>(self, 0x44 + i * 4));
        ComRelease(At<void*>(self, 0x80 + i * 4));
    }
}

// 0x8AFC60 (thiscall): the buffers set on the device (vertex, geometry, pixel)
void __fastcall NiD3D10ShaderConstantManager_ApplyConstantBuffers(uint8_t* self, re::Edx) {
    StateCache_VSSetConstantBuffers(At<void*>(self, 4), 0, 0xF, re::PtrCast<const re::Ptr32<void>*>(self + 8));
    StateCache_GSSetConstantBuffers(At<void*>(self, 4), 0, 0xF, re::PtrCast<const re::Ptr32<void>*>(self + 0x44));
    StateCache_PSSetConstantBuffers(At<void*>(self, 4), 0, 0xF, re::PtrCast<const re::Ptr32<void>*>(self + 0x80));
}

// 0x8AFCA0 (thiscall): its deleting destructor
void* __fastcall NiD3D10ShaderConstantManager_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    ShaderConstantManager_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0xBC);
    }
    return self;
}

// 0x8AFCD0 (thiscall): a constant map's buffer put in its program's slot (its own index, else the
// slot whose buffer has its name, "$Globals" for none; by the program's kind: 0 vertex, 1 pixel,
// 2 geometry)
void __fastcall NiD3D10ShaderConstantManager_SetConstantMap(uint8_t* self, re::Edx, uint8_t* program, void* map) {
    if (program == nullptr || map == nullptr) {
        return;
    }
    void* shaderMap = ConstantMap_GetShaderBuffer(map);
    if (shaderMap == nullptr) {
        return;
    }
    void* buffer = Object_Get8(Object_Get8(shaderMap));
    if (buffer == nullptr) {
        return;
    }
    int32_t index = ConstantMap_GetIndex(shaderMap);
    if (index == -1) {
        uint32_t count = Program_GetConstantBufferCount(program);
        bool found = false;
        for (uint32_t i = 0; i < count; i++) {
            re::Ptr32<char> name = *ConstantMap_GetName(map);
            sys::FixedStringAddRef(name);
            if (name == sys::kNullFixedString || *reinterpret_cast<uint16_t*>(name - 4) == 0) {
                FixedString_AssignChars(&name, Text(0x009A0288));   // "$Globals"
            }
            re::Ptr32<char> bufferName;
            bool same = name == *Program_GetConstantBufferName(program, &bufferName, i);
            FixedString_Destruct(&bufferName);
            FixedString_Destruct(&name);
            if (same) {
                index = int32_t(i);
                found = true;
                break;
            }
        }
        if (!found || index == -1) {
            return;
        }
    }
    uint8_t* buffers = nullptr;
    switch (At<uint32_t>(program, 8)) {
    case 0:
        buffers = self + 8;
        break;
    case 1:
        buffers = self + 0x80;
        break;
    case 2:
        buffers = self + 0x44;
        break;
    default:
        break;
    }
    Com<long>(buffer, kAddRef);
    re::Ptr32<void>& slot = reinterpret_cast<re::Ptr32<void>*>(buffers)[index];
    ComRelease(slot);
    slot = buffer;
}

// ---- the resource factory (0x9A0294)

// 0x8AFE30 (thiscall): its constructor (the device held)
uint8_t* __fastcall NiD3D10ResourceFactory_Construct(uint8_t* self, re::Edx, void* device) {
    At<uint32_t>(self, 0) = kResourceFactoryVtable;
    At<void*>(self, 4) = device;
    if (device != nullptr) {
        Com<long>(device, kAddRef);
    }
    return self;
}

// 0x8AFE60 (thiscall): CreateBuffer(size, bind flags, usage, CPU access, misc, initial data)
void* __fastcall NiD3D10ResourceFactory_CreateBuffer(uint8_t* self, re::Edx, uint32_t size, uint32_t bind,
                                                     uint32_t usage, uint32_t cpuAccess, uint32_t misc,
                                                     const void* initial) {
    const uint32_t description[5] = {size, usage, bind, cpuAccess, misc};
    void* buffer = nullptr;
    long hr = Com<long>(At<void*>(self, 4), kCreateBuffer, description, initial, &buffer);
    if (hr >= 0) {
        if (buffer != nullptr) {
            return buffer;
        }
        Error(8, Text(0x009A0298));
    } else {
        Error(8, Text(0x0099E584), hr);
    }
    if (buffer != nullptr) {
        ComRelease(buffer);
        return nullptr;
    }
    return buffer;
}

// 0x8AFF00 (thiscall): CreateTexture2D(width, height, mips, array size, format, samples, quality,
// usage, bind flags, CPU access, misc, initial data)
void* __fastcall NiD3D10ResourceFactory_CreateTexture2D(uint8_t* self, re::Edx, uint32_t width, uint32_t height,
                                                        uint32_t mips, uint32_t arraySize, uint32_t format,
                                                        uint32_t samples, uint32_t quality, uint32_t usage,
                                                        uint32_t bind, uint32_t cpuAccess, uint32_t misc,
                                                        const void* initial) {
    const uint32_t description[11] = {width, height, mips, arraySize, format, samples,
                                      quality, usage, bind, cpuAccess, misc};
    void* texture = nullptr;
    long hr = Com<long>(At<void*>(self, 4), kCreateTexture2D, description, initial, &texture);
    if (hr >= 0) {
        if (texture != nullptr) {
            return texture;
        }
        Error(7, Text(0x009A02CC));
    } else {
        Error(7, Text(0x0099E584), hr);
    }
    if (texture != nullptr) {
        ComRelease(texture);
        return nullptr;
    }
    return texture;
}

// 0x8AFFD0 (thiscall): its deleting destructor (the device let go)
void* __fastcall NiD3D10ResourceFactory_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kResourceFactoryVtable;
    ComRelease(At<void*>(self, 4));
    if (flags & 1) {
        NiDelete(self, 8);
    }
    return self;
}

// ---- NiD3D10RenderStateManager

// 0x8B0000 (thiscall): SetLeftRightSwap
void __fastcall NiD3D10RenderStateManager_SetLeftRightSwap(uint8_t* self, re::Edx, int swap) {
    At<uint8_t>(self, 0x14FC) = uint8_t(swap);
}

// 0x8B0010 (thiscall): SetBlendDesc(description, mask) (fifteen dwords, then eight bytes)
void __fastcall NiD3D10RenderStateManager_SetBlendDesc(uint8_t* self, re::Edx, const uint8_t* description,
                                                      uint32_t mask) {
    static constexpr int kOffsets[23] = {0,    4,    8,    0xC,  0x10, 0x14, 0x18, 0x1C, 0x20, 0x24, 0x28, 0x2C,
                                         0x30, 0x34, 0x38, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43};
    static constexpr int kSizes[23] = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1};
    MaskedCopy(self + 0xA84, description, mask, kOffsets, kSizes, 23);
}

// 0x8B0190 (thiscall): SetDepthStencilDesc(description, mask)
void __fastcall NiD3D10RenderStateManager_SetDepthStencilDesc(uint8_t* self, re::Edx, const uint8_t* description,
                                                             uint32_t mask) {
    static constexpr int kOffsets[14] = {0, 4, 8, 0xC, 0x10, 0x11, 0x14, 0x18, 0x1C, 0x20, 0x24, 0x28, 0x2C, 0x30};
    static constexpr int kSizes[14] = {4, 4, 4, 4, 1, 1, 4, 4, 4, 4, 4, 4, 4, 4};
    MaskedCopy(self + 0xAC8, description, mask, kOffsets, kSizes, 14);
}

// 0x8B0280 (thiscall): SetRasterizerDesc(description, mask)
void __fastcall NiD3D10RenderStateManager_SetRasterizerDesc(uint8_t* self, re::Edx, const uint8_t* description,
                                                           uint32_t mask) {
    static constexpr int kOffsets[10] = {0, 4, 8, 0xC, 0x10, 0x14, 0x18, 0x1C, 0x20, 0x24};
    static constexpr int kSizes[10] = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4};
    MaskedCopy(self + 0xAFC, description, mask, kOffsets, kSizes, 10);
}

// 0x8B0320 (thiscall): GetRasterizerDesc
void __fastcall NiD3D10RenderStateManager_GetRasterizerDesc(uint8_t* self, re::Edx, uint32_t* description) {
    for (int i = 0; i < 10; i++) {
        description[i] = 0;
    }
    for (int i = 0; i < 10; i++) {
        description[i] = At<uint32_t>(self, 0xAFC + i * 4);
    }
}

// 0x8B0360 (thiscall): SetSamplerDesc(stage (0 vertex, 1 pixel, 2 geometry), sampler, description,
// mask) (bit 7: the four border colour components)
void __fastcall NiD3D10RenderStateManager_SetSamplerDesc(uint8_t* self, re::Edx, uint32_t stage, uint32_t sampler,
                                                        const uint32_t* description, uint32_t mask) {
    if (sampler >= 0x10) {
        return;
    }
    auto* to = &At<uint32_t>(self, int((stage * 0x10 + sampler) * 0x34 + 0xB24));
    for (int i = 0; i < 7; i++) {
        if ((mask & (1u << i)) != 0) {
            to[i] = description[i];
        }
    }
    if ((mask & 0x80) != 0) {
        to[7] = description[7];
        to[8] = description[8];
        to[9] = description[9];
        to[10] = description[10];
    }
    if ((mask & 0x100) != 0) {
        to[11] = description[11];
    }
    if ((mask & 0x200) != 0) {
        to[12] = description[12];
    }
}

// 0x8B0410 (thiscall): RestoreDefaultStates (the defaults copied into the current states)
void __fastcall NiD3D10RenderStateManager_RestoreDefaultStates(uint8_t* self, re::Edx) {
    crt::Memcpy(self + 0xA84, self + 0xC, 0x44);
    crt::Memcpy(self + 0xAC8, self + 0x50, 0x34);
    crt::Memcpy(self + 0xAFC, self + 0x84, 0x28);
    for (int stage = 0; stage < 3; stage++) {
        crt::Memcpy(self + 0xAC + stage * 0x340 + 0xA78, self + 0xAC + stage * 0x340, 0x340);
    }
    for (int i = 0; i < 4; i++) {
        At<float>(self, 0x14E4 + i * 4) = At<float>(self, 0xA6C + i * 4);
    }
    At<uint32_t>(self, 0x14F4) = At<uint32_t>(self, 0xA7C);
    At<uint32_t>(self, 0x14F8) = At<uint32_t>(self, 0xA80);
}

// 0x8B04C0 (thiscall): SetDefaultRasterizerDesc
void __fastcall NiD3D10RenderStateManager_SetDefaultRasterizerDesc(uint8_t* self, re::Edx, const uint32_t* description) {
    for (int i = 0; i < 10; i++) {
        At<uint32_t>(self, 0x84 + i * 4) = description[i];
    }
}

// 0x8B04E0 (thiscall): GetDefaultRasterizerDesc
void __fastcall NiD3D10RenderStateManager_GetDefaultRasterizerDesc(uint8_t* self, re::Edx, uint32_t* description) {
    for (int i = 0; i < 10; i++) {
        description[i] = 0;
    }
    for (int i = 0; i < 10; i++) {
        description[i] = At<uint32_t>(self, 0x84 + i * 4);
    }
}

// 0x8B0520 (thiscall): ApplyBlendState (made from the current description, set with the factor
// and mask)
void __fastcall NiD3D10RenderStateManager_ApplyBlendState(uint8_t* self, re::Edx) {
    void* state = nullptr;
    long hr = Com<long>(At<void*>(self, 4), kCreateBlendState, self + 0xA84, &state);
    if (hr < 0) {
        Error(0xB, Text(0x0099E584), hr);
    } else if (state == nullptr) {
        Error(0xB, Text(0x009A0304));
    } else {
        StateCache_OMSetBlendState(At<void*>(self, 8), state, &At<float>(self, 0x14E4), At<uint32_t>(self, 0x14F4));
    }
    ComRelease(state);
}

// 0x8B05A0 (thiscall): ApplyDepthStencilState
void __fastcall NiD3D10RenderStateManager_ApplyDepthStencilState(uint8_t* self, re::Edx) {
    void* state = nullptr;
    long hr = Com<long>(At<void*>(self, 4), kCreateDepthStencilState, self + 0xAC8, &state);
    if (hr < 0) {
        Error(0xC, Text(0x0099E584), hr);
    } else if (state == nullptr) {
        Error(0xC, Text(0x009A033C));
    } else {
        StateCache_OMSetDepthStencilState(At<void*>(self, 8), state, At<uint32_t>(self, 0x14F8));
    }
    ComRelease(state);
}

// 0x8B0620 (thiscall): ApplyRasterizerState (quirk kept: with the left-right swap the winding is
// flipped in the current description itself, each time)
void __fastcall NiD3D10RenderStateManager_ApplyRasterizerState(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 0x14FC) != 0) {
        At<uint32_t>(self, 0xB04) = At<uint32_t>(self, 0xB04) == 0 ? 1 : 0;
    }
    void* state = nullptr;
    long hr = Com<long>(At<void*>(self, 4), kCreateRasterizerState, self + 0xAFC, &state);
    if (hr < 0) {
        Error(0xD, Text(0x0099E584), hr);
    } else if (state == nullptr) {
        Error(0xD, Text(0x009A037C));
    } else {
        StateCache_RSSetState(At<void*>(self, 8), state);
    }
    ComRelease(state);
}

// 0x8B06B0 / 0x8B0790 / 0x8B0870 (thiscall): ApplyVertexSamplers, ApplyGeometrySamplers,
// ApplyPixelSamplers(start, count)
void __fastcall NiD3D10RenderStateManager_ApplyVertexSamplers(uint8_t* self, re::Edx, uint32_t start, uint32_t count) {
    ApplySamplers(self, 0xB24, start, count, StateCache_VSSetSamplers);
}
void __fastcall NiD3D10RenderStateManager_ApplyGeometrySamplers(uint8_t* self, re::Edx, uint32_t start,
                                                               uint32_t count) {
    ApplySamplers(self, 0x11A4, start, count, StateCache_GSSetSamplers);
}
void __fastcall NiD3D10RenderStateManager_ApplyPixelSamplers(uint8_t* self, re::Edx, uint32_t start, uint32_t count) {
    ApplySamplers(self, 0xE64, start, count, StateCache_PSSetSamplers);
}

// 0x8B0950: NiD3D10RenderStateManager::ConvertGbBlendToD3D10Blend (NiAlphaProperty's function to
// D3D10_BLEND; ZERO for an unknown one, reported)
uint32_t __cdecl NiD3D10RenderStateManager_ConvertGbBlendToD3D10Blend(uint32_t function) {
    static constexpr uint32_t kBlends[11] = {2, 1, 3, 4, 9, 10, 5, 6, 7, 8, 0xB};
    if (function < 11) {
        return kBlends[function];
    }
    Warning(Text(0x009A03F0));
    return 1;
}

// 0x8B09E0: NiD3D10RenderStateManager::ConvertGbStencilFuncToD3D10Comparison (NEVER for an
// unknown one, reported)
uint32_t __cdecl NiD3D10RenderStateManager_ConvertGbStencilFuncToD3D10Comparison(uint32_t function) {
    if (function == 0) {
        return 1;
    }
    if (function <= 7) {
        return function + 1;
    }
    Warning(Text(0x009A0450));
    return 1;
}

#define C42G_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C42G_REVERSED("NiD3D10ShaderConstantManager::NiD3D10ShaderConstantManager", 0x008AFBC0,
              NiD3D10ShaderConstantManager_Construct);
C42G_REVERSED("NiD3D10ShaderConstantManager::~NiD3D10ShaderConstantManager", 0x008AFC10,
              NiD3D10ShaderConstantManager_Destruct);
C42G_REVERSED("NiD3D10ShaderConstantManager::ApplyConstantBuffers", 0x008AFC60,
              NiD3D10ShaderConstantManager_ApplyConstantBuffers);
C42G_REVERSED("NiD3D10ShaderConstantManager::DeletingDestructor", 0x008AFCA0,
              NiD3D10ShaderConstantManager_DeletingDestructor);
C42G_REVERSED("NiD3D10ShaderConstantManager::SetConstantMap", 0x008AFCD0, NiD3D10ShaderConstantManager_SetConstantMap);
C42G_REVERSED("NiD3D10ResourceFactory::NiD3D10ResourceFactory", 0x008AFE30, NiD3D10ResourceFactory_Construct);
C42G_REVERSED("NiD3D10ResourceFactory::CreateBuffer", 0x008AFE60, NiD3D10ResourceFactory_CreateBuffer);
C42G_REVERSED("NiD3D10ResourceFactory::CreateTexture2D", 0x008AFF00, NiD3D10ResourceFactory_CreateTexture2D);
C42G_REVERSED("NiD3D10ResourceFactory::DeletingDestructor", 0x008AFFD0, NiD3D10ResourceFactory_DeletingDestructor);
C42G_REVERSED("NiD3D10RenderStateManager::SetLeftRightSwap", 0x008B0000, NiD3D10RenderStateManager_SetLeftRightSwap);
C42G_REVERSED("NiD3D10RenderStateManager::SetBlendDesc", 0x008B0010, NiD3D10RenderStateManager_SetBlendDesc);
C42G_REVERSED("NiD3D10RenderStateManager::SetDepthStencilDesc", 0x008B0190,
              NiD3D10RenderStateManager_SetDepthStencilDesc);
C42G_REVERSED("NiD3D10RenderStateManager::SetRasterizerDesc", 0x008B0280, NiD3D10RenderStateManager_SetRasterizerDesc);
C42G_REVERSED("NiD3D10RenderStateManager::GetRasterizerDesc", 0x008B0320, NiD3D10RenderStateManager_GetRasterizerDesc);
C42G_REVERSED("NiD3D10RenderStateManager::SetSamplerDesc", 0x008B0360, NiD3D10RenderStateManager_SetSamplerDesc);
C42G_REVERSED("NiD3D10RenderStateManager::RestoreDefaultStates", 0x008B0410,
              NiD3D10RenderStateManager_RestoreDefaultStates);
C42G_REVERSED("NiD3D10RenderStateManager::SetDefaultRasterizerDesc", 0x008B04C0,
              NiD3D10RenderStateManager_SetDefaultRasterizerDesc);
C42G_REVERSED("NiD3D10RenderStateManager::GetDefaultRasterizerDesc", 0x008B04E0,
              NiD3D10RenderStateManager_GetDefaultRasterizerDesc);
C42G_REVERSED("NiD3D10RenderStateManager::ApplyBlendState", 0x008B0520, NiD3D10RenderStateManager_ApplyBlendState);
C42G_REVERSED("NiD3D10RenderStateManager::ApplyDepthStencilState", 0x008B05A0,
              NiD3D10RenderStateManager_ApplyDepthStencilState);
C42G_REVERSED("NiD3D10RenderStateManager::ApplyRasterizerState", 0x008B0620,
              NiD3D10RenderStateManager_ApplyRasterizerState);
C42G_REVERSED("NiD3D10RenderStateManager::ApplyVertexSamplers", 0x008B06B0,
              NiD3D10RenderStateManager_ApplyVertexSamplers);
C42G_REVERSED("NiD3D10RenderStateManager::ApplyGeometrySamplers", 0x008B0790,
              NiD3D10RenderStateManager_ApplyGeometrySamplers);
C42G_REVERSED("NiD3D10RenderStateManager::ApplyPixelSamplers", 0x008B0870, NiD3D10RenderStateManager_ApplyPixelSamplers);
C42G_REVERSED("NiD3D10RenderStateManager::ConvertGbBlendToD3D10Blend", 0x008B0950,
              NiD3D10RenderStateManager_ConvertGbBlendToD3D10Blend);
C42G_REVERSED("NiD3D10RenderStateManager::ConvertGbStencilFuncToD3D10Comparison", 0x008B09E0,
              NiD3D10RenderStateManager_ConvertGbStencilFuncToD3D10Comparison);

} // namespace ni::c42
