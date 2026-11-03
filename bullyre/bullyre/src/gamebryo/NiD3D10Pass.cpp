// NiD3D10Pass (vtable 0x9A1610: 0 the deleting destructor, 1 DeleteThis, 2 SetupShaderPrograms (the
// shaders set on the device state), 3 the constant maps set, 4 the shader resources set, 5
// SetupRenderingPass, 6 PostProcessRenderingPass (0x7627D0: a no-op returning 0); 0x248 bytes,
// made by CreateNewPass): one pass of an NiD3D10Shader (the DX9 NiD3DPass's counterpart).
//
// +0x8 a byte (unused here), +0x18 its render state group (NiD3D10RenderStateGroup, held: set by
// the same code as NiD3D10ShaderInterface::SetShaderDecl), +0x1C / +0x2C / +0x3C its vertex /
// geometry / pixel constant maps (NiTObjectArray<NiD3D10ShaderConstantMapPtr>: +0x4 the array, +0x8
// the allocated size, +0xA the size, +0xE the growth), +0x4C / +0x50 / +0x54 its vertex /
// geometry / pixel shader (held), +0x58 / +0x98 / +0xD8 the vertex / geometry / pixel shaders'
// textures (16 each, held), +0x118..+0x138 nine NiMalloc'd strings (the programs' file names and
// entry points: a guess), +0x13C the texture count, +0x140 the sampler count, +0x144 the textures'
// names, +0x184 the samplers' names (16 NiFixedStrings each), +0x1C4 by the pixel shader's texture
// slot the texture it binds (16 bytes; 0xFF none), +0x1D4 by its sampler slot the sampler (16
// bytes), +0x1E4 each texture's map flags (its map | instance), +0x224 each texture's effect flags
// (16-bit), +0x244 whether the constant maps know the programs' reflection, +0x245 whether the
// texture / sampler slots are found.

#include "gamebryo/NiD3D10C44.h"
#include "reversed.h"

namespace ni::c44 {
namespace {

constexpr uintptr_t kPassVtable = 0x009A1610;
constexpr uintptr_t kMapArrayVtable = 0x009893C0;
constexpr uint32_t kPassSize = 0x248;
constexpr uint32_t kTextures = 0x10;

GAME_VAR(void*, ms_pkRenderer, 0x020E2380);   // NiRenderer::ms_pkRenderer

// the renderer's (chunk 42's) and its device / render state managers'
GAME_FN(void*, __thiscall, Renderer_GetDeviceState, 0x008A9100, const void* renderer);
GAME_FN(void*, __thiscall, Renderer_GetRenderStateManager, 0x008A9110, const void* renderer);
GAME_FN(void*, __thiscall, Renderer_GetShaderResourceManager, 0x008A9130, const void* renderer);
GAME_FN(void, __thiscall, DeviceState_VSSetShader, 0x008B3E60, void* self, void* shader);
GAME_FN(void, __thiscall, DeviceState_VSClearShader, 0x008B3EB0, void* self);
GAME_FN(void, __thiscall, DeviceState_GSSetShader, 0x008B3EE0, void* self, void* shader);
GAME_FN(void, __thiscall, DeviceState_GSClearShader, 0x008B3F30, void* self);
GAME_FN(void, __thiscall, DeviceState_PSSetShader, 0x008B3F60, void* self, void* shader);
GAME_FN(void, __thiscall, DeviceState_PSClearShader, 0x008B3FB0, void* self);
GAME_FN(void, __thiscall, DeviceState_VSSetShaderResources, 0x008B3A10, void* self, uint32_t start, uint32_t count,
        const re::Ptr32<void>* views);
GAME_FN(void, __thiscall, DeviceState_VSClearShaderResources, 0x008B3A90, void* self);
GAME_FN(void, __thiscall, DeviceState_GSSetShaderResources, 0x008B3AE0, void* self, uint32_t start, uint32_t count,
        const re::Ptr32<void>* views);
GAME_FN(void, __thiscall, DeviceState_GSClearShaderResources, 0x008B3B60, void* self);
GAME_FN(void, __thiscall, DeviceState_PSSetShaderResources, 0x008B3BB0, void* self, uint32_t start, uint32_t count,
        const re::Ptr32<void>* views);
GAME_FN(void, __thiscall, DeviceState_PSClearShaderResources, 0x008B3C30, void* self);
GAME_FN(uint8_t, __thiscall, RenderStateManager_SetRenderStateGroup, 0x008B0F40, void* self, const void* group);
GAME_FN(void, __thiscall, ConstantManager_SetConstantMap, 0x008AFCD0, void* self, void* program, void* map);
// the programs' (a folded getter: +0x24 the D3D10 shader object; +0x18 the reflection)
GAME_FN(void*, __thiscall, Program_GetShader, 0x008C3450, void* program);
GAME_FN(void*, __thiscall, Program_GetReflection, 0x008B40F0, void* program);
GAME_FN(void*, __thiscall, TextureData_GetResourceView, 0x008C46E0, void* data);   // (a folded getter: +0x5C)
// the arrays'
GAME_FN(void, __thiscall, MapArray_SetSize, 0x00856170, void* array, uint32_t size);
GAME_FN(void, __thiscall, MapArray_SetAt, 0x0040E800, void* array, uint32_t index, const re::Ptr32<void>* map);
GAME_FN(void, __cdecl, NiTArray_Free, 0x00836F80, void* p);
// the render state group's and the constant map's
GAME_FN(void, __thiscall, Group_SetSamplerMap, 0x008C0640, void* group, uint32_t stage, const void* map);
GAME_FN(uint8_t, __thiscall, ConstantMap_SetShaderReflection, 0x008BF280, void* map, void* reflection);
GAME_FN(uint32_t, __thiscall, ConstantMap_SetShaderConstants, 0x008C0090, void* self, void* geometry, void* skin,
        void* partition, void* bufferData, void* state, void* effects, const void* world, const void* bound,
        uint32_t pass, int global);
// this file's (called where modes apply)
GAME_FN(void, __thiscall, Pass_LinkReflections, 0x008C11F0, void* self);
GAME_FN(uint8_t, __thiscall, Pass_FindTextureSlots, 0x008C12A0, void* self);
GAME_FN(void, __thiscall, Pass_ResetTextures, 0x008C18D0, void* self);
GAME_FN(void, __thiscall, Pass_Destruct, 0x008C1AB0, void* self);
GAME_FN(void*, __thiscall, Pass_Construct, 0x008C1D00, void* self);

// a held program slot assigned (the old let go first); the reflection to be linked again
void SetProgram(uint8_t* self, int offset, void* program) {
    AssignRef(At<void*>(self, offset), program);
    At<uint8_t>(self, 0x244) = 0;
}

// a constant map array's element (null past its size)
void* MapAt(const uint8_t* self, int array, uint32_t index) {
    if (index >= At<uint16_t>(self, array + 0xA)) {
        return nullptr;
    }
    return At<re::Ptr32<void>*>(self, array + 4)[index];
}

// a constant map set at an index (the array grown to it)
void SetMap(uint8_t* self, int array, uint32_t index, void* map) {
    re::Ptr32<void> held = map;
    AddRefRef(map);
    void* object = self + array;
    if (index >= At<uint16_t>(self, array + 8)) {
        MapArray_SetSize(object, At<uint16_t>(self, array + 0xE) + index);
    }
    MapArray_SetAt(object, index, &held);
    ReleaseRef(map);
    At<uint8_t>(self, 0x244) = 0;
}

// a texture's shader resource view (the texture precached first when it has no renderer data;
// null when it still hasn't)
void* ResourceView(void* texture) {
    void* data = At<void*>(texture, 0x24);
    if (data == nullptr) {
        Virtual<void>(ms_pkRenderer, 43, texture, 0, 0);   // NiRenderer::PrecacheTexture
        data = At<void*>(texture, 0x24);
        if (data == nullptr) {
            return nullptr;
        }
    }
    return TextureData_GetResourceView(data);
}

// the index of a name in a list of them (0xFF when absent)
uint8_t FindName(const uint8_t* self, int list, uint32_t count, const char* name) {
    for (uint32_t i = 0; i < count; i++) {
        if (crt::Strcmp(name, At<const char*>(self, list + int(i) * 4)) == 0) {
            return uint8_t(i);
        }
    }
    return 0xFF;
}

// the constant maps of an array set for its program
void SetMaps(uint8_t* self, int array, void* program, void* manager, void* geometry, void* skin, void* partition,
             void* bufferData, void* state, void* effects, const void* world, const void* bound, uint32_t pass) {
    uint32_t count = At<uint16_t>(self, array + 0xA);
    for (uint32_t i = 0; i < count; i++) {
        void* map = At<re::Ptr32<void>*>(self, array + 4)[i];
        if (map == nullptr) {
            continue;
        }
        ConstantMap_SetShaderConstants(map, geometry, skin, partition, bufferData, state, effects, world, bound, pass,
                                       1);
        ConstantManager_SetConstantMap(manager, program, map);
    }
}

// the constant maps of an array told the program's reflection
void LinkMaps(uint8_t* self, int array, void* program) {
    void* reflection = Program_GetReflection(program);
    if (reflection == nullptr) {
        return;
    }
    uint32_t count = At<uint16_t>(self, array + 0xA);
    for (uint32_t i = 0; i < count; i++) {
        void* map = At<re::Ptr32<void>*>(self, array + 4)[i];
        if (map != nullptr) {
            ConstantMap_SetShaderReflection(map, reflection);
        }
    }
}

} // namespace

// 0x8C0F80 (thiscall): AddTexture (its index; -1 when there are 16)
uint32_t __fastcall Pass_AddTexture(uint8_t* self, re::Edx) {
    uint32_t count = At<uint32_t>(self, 0x13C);
    if (count >= kTextures) {
        return 0xFFFFFFFF;
    }
    At<uint32_t>(self, 0x13C) = count + 1;
    return count;
}

// 0x8C0FA0 (thiscall): GetTextureMapFlags (0 past the count)
uint32_t __fastcall Pass_GetTextureMapFlags(uint8_t* self, re::Edx, uint32_t index) {
    if (index >= At<uint32_t>(self, 0x13C)) {
        return 0;
    }
    return At<uint32_t>(self, 0x1E4 + int(index) * 4);
}

// 0x8C0FC0 (thiscall): GetTextureEffectFlags (0 past the count)
uint16_t __fastcall Pass_GetTextureEffectFlags(uint8_t* self, re::Edx, uint32_t index) {
    if (index >= At<uint32_t>(self, 0x13C)) {
        return 0;
    }
    return At<uint16_t>(self, 0x224 + int(index) * 2);
}

// 0x8C1030 (thiscall): GetVertexConstantMap
void* __fastcall Pass_GetVertexConstantMap(uint8_t* self, re::Edx, uint32_t index) {
    return MapAt(self, 0x1C, index);
}

// 0x8C1050 (thiscall): SetVertexShader
void __fastcall Pass_SetVertexShader(uint8_t* self, re::Edx, void* program) {
    SetProgram(self, 0x4C, program);
}

// 0x8C10A0 (thiscall): GetGeometryConstantMap
void* __fastcall Pass_GetGeometryConstantMap(uint8_t* self, re::Edx, uint32_t index) {
    return MapAt(self, 0x2C, index);
}

// 0x8C10C0 (thiscall): SetGeometryShader
void __fastcall Pass_SetGeometryShader(uint8_t* self, re::Edx, void* program) {
    SetProgram(self, 0x50, program);
}

// 0x8C1110 (thiscall): GetPixelConstantMap
void* __fastcall Pass_GetPixelConstantMap(uint8_t* self, re::Edx, uint32_t index) {
    return MapAt(self, 0x3C, index);
}

// 0x8C1140 (thiscall): SetPixelShader
void __fastcall Pass_SetPixelShader(uint8_t* self, re::Edx, void* program) {
    SetProgram(self, 0x54, program);
}

// 0x8C1190 (thiscall): SetPixelTexture (by index, 0..15; held)
void __fastcall Pass_SetPixelTexture(uint8_t* self, re::Edx, uint32_t index, void* texture) {
    if (index < kTextures) {
        AssignRef(At<void*>(self, 0xD8 + int(index) * 4), texture);
    }
}

// 0x8C11F0 (thiscall): each stage's constant maps told its program's reflection
void __fastcall Pass_LinkReflectionsImpl(uint8_t* self, re::Edx) {
    if (void* program = At<void*>(self, 0x4C)) {
        LinkMaps(self, 0x1C, program);
    }
    if (void* program = At<void*>(self, 0x50)) {
        LinkMaps(self, 0x2C, program);
    }
    if (void* program = At<void*>(self, 0x54)) {
        LinkMaps(self, 0x3C, program);
    }
    At<uint8_t>(self, 0x244) = 1;
}

// 0x8C12A0 (thiscall): the pixel shader's texture and sampler slots found by name from its
// reflection (ID3D10ShaderReflection: 3 GetDesc, 6 GetResourceBindingDesc); 0 when it can't be
uint8_t __fastcall Pass_FindTextureSlotsImpl(uint8_t* self, re::Edx) {
    for (int i = 0; i < 0x10; i++) {
        At<uint8_t>(self, 0x1C4 + i) = 0xFF;
    }
    for (int i = 0; i < 0x10; i++) {
        At<uint8_t>(self, 0x1D4 + i) = uint8_t(i);
    }
    void* program = At<void*>(self, 0x54);
    if (program != nullptr) {
        void* reflection = Program_GetReflection(program);
        if (reflection != nullptr) {
            uint32_t desc[0x20];   // D3D10_SHADER_DESC (0x70 bytes; +0x10 the bound resources)
            if (Com<long>(reflection, 3, desc) < 0) {
                return 0;
            }
            uint32_t resources = desc[4];
            for (uint32_t i = 0; i < resources; i++) {
                struct BindDesc {
                    const char* name;
                    uint32_t type;
                    uint32_t bindPoint;
                    uint32_t bindCount;
                    uint32_t flags;
                    uint32_t returnType;
                    uint32_t dimension;
                    uint32_t samples;
                } bind;
                if (Com<long>(reflection, 6, i, &bind) < 0) {
                    return 0;
                }
                if (bind.type == 0 || bind.type == 1) {   // a cbuffer / tbuffer
                    continue;
                }
                if (bind.type == 3) {   // a sampler
                    uint8_t index = FindName(self, 0x184, At<uint32_t>(self, 0x140), bind.name);
                    if (bind.bindCount != 0) {
                        crt::Memset(self + 0x1D4 + bind.bindPoint, index, bind.bindCount);
                    }
                } else {
                    uint8_t index = FindName(self, 0x144, At<uint32_t>(self, 0x13C), bind.name);
                    if (bind.bindCount != 0) {
                        crt::Memset(self + 0x1C4 + bind.bindPoint, index, bind.bindCount);
                    }
                }
            }
        }
    }
    At<uint8_t>(self, 0x245) = 1;
    return 1;
}

// 0x8C1490 (thiscall, slot 2): the pass's shaders set on the device state (cleared where it has
// none)
uint32_t __fastcall Pass_SetupShaderPrograms(uint8_t* self, re::Edx, void*, void*, void*, void*, void*, void*,
                                             const void*, const void*, uint32_t) {
    void* state = Renderer_GetDeviceState(D3D10Renderer_GetRenderer());
    if (void* program = At<void*>(self, 0x4C)) {
        DeviceState_VSSetShader(state, Program_GetShader(program));
    } else {
        DeviceState_VSClearShader(state);
    }
    if (void* program = At<void*>(self, 0x50)) {
        DeviceState_GSSetShader(state, Program_GetShader(program));
    } else {
        DeviceState_GSClearShader(state);
    }
    if (void* program = At<void*>(self, 0x54)) {
        DeviceState_PSSetShader(state, Program_GetShader(program));
    } else {
        DeviceState_PSClearShader(state);
    }
    return 0;
}

// 0x8C1510 (thiscall, slot 3): the constant maps' values set (their reflection linked first)
uint32_t __fastcall Pass_SetConstantMaps(uint8_t* self, re::Edx, void* geometry, void* skin, void* partition,
                                         void* bufferData, void* state, void* effects, const void* world,
                                         const void* bound, uint32_t pass) {
    void* manager = Renderer_GetShaderResourceManager(D3D10Renderer_GetRenderer());
    if (At<uint8_t>(self, 0x244) == 0) {
        Pass_LinkReflections(self);
    }
    if (void* program = At<void*>(self, 0x4C)) {
        SetMaps(self, 0x1C, program, manager, geometry, skin, partition, bufferData, state, effects, world, bound,
                pass);
    }
    if (void* program = At<void*>(self, 0x50)) {
        SetMaps(self, 0x2C, program, manager, geometry, skin, partition, bufferData, state, effects, world, bound,
                pass);
    }
    if (void* program = At<void*>(self, 0x54)) {
        SetMaps(self, 0x3C, program, manager, geometry, skin, partition, bufferData, state, effects, world, bound,
                pass);
    }
    return 0;
}

// 0x8C16A0 (thiscall, slot 4): the shaders' textures set on the device state (the pixel shader's
// by its slots)
uint32_t __fastcall Pass_SetShaderResources(uint8_t* self, re::Edx, void*, void*, void*, void*, void*, void*,
                                            const void*, const void*) {
    void* state = Renderer_GetDeviceState(D3D10Renderer_GetRenderer());
    re::Ptr32<void> views[0x10];
    if (At<void*>(self, 0x4C) != nullptr) {
        crt::Memset(views, 0, sizeof(views));
        for (int i = 0; i < 0x10; i++) {
            void* texture = At<void*>(self, 0x58 + i * 4);
            if (texture != nullptr) {
                if (void* view = ResourceView(texture)) {
                    views[i] = view;
                }
            }
        }
        DeviceState_VSSetShaderResources(state, 0, 0x10, views);
    } else {
        DeviceState_VSClearShaderResources(state);
    }
    if (At<void*>(self, 0x50) != nullptr) {
        crt::Memset(views, 0, sizeof(views));
        for (int i = 0; i < 0x10; i++) {
            void* texture = At<void*>(self, 0x98 + i * 4);
            if (texture != nullptr) {
                if (void* view = ResourceView(texture)) {
                    views[i] = view;
                }
            }
        }
        DeviceState_GSSetShaderResources(state, 0, 0x10, views);
    } else {
        DeviceState_GSClearShaderResources(state);
    }
    if (At<void*>(self, 0x54) == nullptr) {
        DeviceState_PSClearShaderResources(state);
        return 0;
    }
    crt::Memset(views, 0, sizeof(views));
    for (int i = 0; i < 0x10; i++) {
        uint8_t slot = At<uint8_t>(self, 0x1C4 + i);
        if (slot == 0xFF) {
            continue;
        }
        void* texture = At<void*>(self, 0xD8 + int(slot) * 4);
        if (texture != nullptr) {
            if (void* view = ResourceView(texture)) {
                views[i] = view;
            }
        }
    }
    DeviceState_PSSetShaderResources(state, 0, 0x10, views);
    return 0;
}

// 0x8C1850 (thiscall, slot 5): SetupRenderingPass (the slots found when they haven't been; the
// render state group given the pixel samplers' map and set; the resources set, slot 4)
uint32_t __fastcall Pass_SetupRenderingPass(uint8_t* self, re::Edx, void* a1, void* a2, void* a3, void* a4, void* a5,
                                            void* a6, const void* a7, const void* a8) {
    if (At<uint8_t>(self, 0x245) == 0) {
        Pass_FindTextureSlots(self);
    }
    void* manager = Renderer_GetRenderStateManager(D3D10Renderer_GetRenderer());
    void* group = At<void*>(self, 0x18);
    if (group != nullptr) {
        Group_SetSamplerMap(group, 1, self + 0x1D4);
        RenderStateManager_SetRenderStateGroup(manager, At<void*>(self, 0x18));
    }
    Virtual<uint32_t>(self, 4, a1, a2, a3, a4, a5, a6, a7, a8);
    return 0;
}

// 0x8C18D0 (thiscall): the textures and samplers forgotten (their counts, slots, flags and names)
void __fastcall Pass_ResetTexturesImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0x13C) = 0;
    At<uint32_t>(self, 0x140) = 0;
    At<uint32_t>(self, 0x1C4) = 0;
    At<uint32_t>(self, 0x1C8) = 0;
    At<uint32_t>(self, 0x1CC) = 0;
    At<uint32_t>(self, 0x1D0) = 0;
    crt::Memset(self + 0x1E4, 0, 0x40);
    for (int i = 0; i < 8; i++) {
        At<uint32_t>(self, 0x224 + i * 4) = 0;
    }
    for (int i = 0; i < 0x10; i++) {
        At<uint8_t>(self, 0x1D4 + i) = uint8_t(i);
        FixedString_AssignText(&At<char*>(self, 0x184 + i * 4), Text(0x008FEFBB));   // ""
        FixedString_AssignText(&At<char*>(self, 0x144 + i * 4), Text(0x008FEFBB));
    }
}

// 0x8C1A10 (thiscall): a texture's name, map flags (its map | instance) and effect flags (by index,
// 0..15; the count raised to it only when it's at most the count, as the original has it)
uint8_t __fastcall Pass_SetTextureInfo(uint8_t* self, re::Edx, uint32_t index, const re::Ptr32<char>* name, uint32_t map,
                                       uint32_t instance, uint32_t effect) {
    if (index >= kTextures) {
        return 0;
    }
    if (index <= At<uint32_t>(self, 0x13C)) {
        At<uint32_t>(self, 0x13C) = index + 1;
    }
    At<uint32_t>(self, 0x1E4 + int(index) * 4) = uint16_t(instance) | map;
    At<uint16_t>(self, 0x224 + int(index) * 2) = uint16_t(effect);
    FixedString_Assign(&At<char*>(self, 0x144 + int(index) * 4), name);
    At<uint8_t>(self, 0x245) = 0;
    return 1;
}

// 0x8C1A70 (thiscall): a sampler's name (by index, 0..15; the count as above)
uint8_t __fastcall Pass_SetSamplerName(uint8_t* self, re::Edx, uint32_t index, const re::Ptr32<char>* name) {
    if (index >= kTextures) {
        return 0;
    }
    if (index <= At<uint32_t>(self, 0x140)) {
        At<uint32_t>(self, 0x140) = index + 1;
    }
    FixedString_Assign(&At<char*>(self, 0x184 + int(index) * 4), name);
    return 1;
}

// 0x8C1AB0 (thiscall): the destructor
void __fastcall Pass_DestructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kPassVtable;
    for (int i = 0; i < 9; i++) {
        NiFree(At<void*>(self, 0x118 + i * 4));
    }
    for (int i = 15; i >= 0; i--) {
        StringRelease(&At<char*>(self, 0x184 + i * 4));
    }
    for (int i = 15; i >= 0; i--) {
        StringRelease(&At<char*>(self, 0x144 + i * 4));
    }
    for (int i = 15; i >= 0; i--) {
        ReleaseRef(At<void*>(self, 0xD8 + i * 4));
    }
    for (int i = 15; i >= 0; i--) {
        ReleaseRef(At<void*>(self, 0x98 + i * 4));
    }
    ReleaseRef(At<void*>(self, 0x54));
    ReleaseRef(At<void*>(self, 0x50));
    ReleaseRef(At<void*>(self, 0x4C));
    At<uint32_t>(self, 0x3C) = kMapArrayVtable;
    NiTArray_Free(At<void*>(self, 0x40));
    At<uint32_t>(self, 0x2C) = kMapArrayVtable;
    NiTArray_Free(At<void*>(self, 0x30));
    At<uint32_t>(self, 0x1C) = kMapArrayVtable;
    NiTArray_Free(At<void*>(self, 0x20));
    ReleaseRef(At<void*>(self, 0x18));
    At<uint32_t>(self, 0) = kRefObjectVtable;
    Decrement(&ms_uiObjects);
}

// 0x8C1D00 (thiscall): the constructor
void* __fastcall Pass_ConstructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kRefObjectVtable;
    At<uint32_t>(self, 4) = 0;
    Increment(&ms_uiObjects);
    At<uint32_t>(self, 0) = kPassVtable;
    At<void*>(self, 0x18) = nullptr;
    static const int kArrays[3] = {0x1C, 0x2C, 0x3C};
    for (int array : kArrays) {
        At<uint16_t>(self, array + 8) = 0;
        At<uint16_t>(self, array + 0xA) = 0;
        At<uint16_t>(self, array + 0xC) = 0;
        At<void*>(self, array + 4) = nullptr;
        At<uint32_t>(self, array) = kMapArrayVtable;
        At<uint16_t>(self, array + 0xE) = 1;
    }
    At<void*>(self, 0x4C) = nullptr;
    At<void*>(self, 0x50) = nullptr;
    At<void*>(self, 0x54) = nullptr;
    for (int offset = 0x58; offset < 0x118; offset += 4) {
        At<uint32_t>(self, offset) = 0;
    }
    for (int offset = 0x118; offset <= 0x140; offset += 4) {
        At<uint32_t>(self, offset) = 0;
    }
    for (int i = 0; i < 0x10; i++) {
        At<char*>(self, 0x144 + i * 4) = NullString();
    }
    for (int i = 0; i < 0x10; i++) {
        At<char*>(self, 0x184 + i * 4) = NullString();
    }
    At<uint8_t>(self, 0x244) = 0;
    At<uint8_t>(self, 0x245) = 0;
    At<uint8_t>(self, 8) = 0;
    Pass_ResetTextures(self);
    return self;
}

// 0x8C1F20 (thiscall, slot 0): the deleting destructor
void* __fastcall Pass_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    Pass_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, kPassSize);
    }
    return self;
}

// 0x8C1F50 (cdecl): CreateNewPass (into the reference; true)
uint8_t __cdecl Pass_CreateNewPass(re::Ptr32<void>* result) {
    void* memory = NiNew(kPassSize);
    void* made = memory != nullptr ? Pass_Construct(memory) : nullptr;
    AssignRef(*result, made);
    return 1;
}

// 0x8C1FB0 (thiscall): SetVertexConstantMap (at an index)
void __fastcall Pass_SetVertexConstantMap(uint8_t* self, re::Edx, uint32_t index, void* map) {
    SetMap(self, 0x1C, index, map);
}

// 0x8C2020 (thiscall): SetGeometryConstantMap
void __fastcall Pass_SetGeometryConstantMap(uint8_t* self, re::Edx, uint32_t index, void* map) {
    SetMap(self, 0x2C, index, map);
}

// 0x8C2090 (thiscall): SetPixelConstantMap
void __fastcall Pass_SetPixelConstantMap(uint8_t* self, re::Edx, uint32_t index, void* map) {
    SetMap(self, 0x3C, index, map);
}

// 0x8C1130 (thiscall, 4 bytes: under a hook's size): GetPixelShader (+0x54)
void* __fastcall Pass_GetPixelShader(uint8_t* self, re::Edx) {
    return At<void*>(self, 0x54);
}
RE_UNHOOKED_FN("NiD3D10Pass::GetPixelShader", 0x008C1130, &Pass_GetPixelShader);

#define GB44P_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

GB44P_REVERSED("NiD3D10Pass::AddTexture", 0x008C0F80, Pass_AddTexture);
GB44P_REVERSED("NiD3D10Pass::GetTextureMapFlags", 0x008C0FA0, Pass_GetTextureMapFlags);
GB44P_REVERSED("NiD3D10Pass::GetTextureEffectFlags", 0x008C0FC0, Pass_GetTextureEffectFlags);
GB44P_REVERSED("NiD3D10Pass::GetVertexConstantMap", 0x008C1030, Pass_GetVertexConstantMap);
GB44P_REVERSED("NiD3D10Pass::SetVertexShader", 0x008C1050, Pass_SetVertexShader);
GB44P_REVERSED("NiD3D10Pass::GetGeometryConstantMap", 0x008C10A0, Pass_GetGeometryConstantMap);
GB44P_REVERSED("NiD3D10Pass::SetGeometryShader", 0x008C10C0, Pass_SetGeometryShader);
GB44P_REVERSED("NiD3D10Pass::GetPixelConstantMap", 0x008C1110, Pass_GetPixelConstantMap);
GB44P_REVERSED("NiD3D10Pass::SetPixelShader", 0x008C1140, Pass_SetPixelShader);
GB44P_REVERSED("NiD3D10Pass::SetPixelTexture", 0x008C1190, Pass_SetPixelTexture);
GB44P_REVERSED("NiD3D10Pass::LinkReflections", 0x008C11F0, Pass_LinkReflectionsImpl);
GB44P_REVERSED("NiD3D10Pass::FindTextureSlots", 0x008C12A0, Pass_FindTextureSlotsImpl);
GB44P_REVERSED("NiD3D10Pass::SetupShaderPrograms", 0x008C1490, Pass_SetupShaderPrograms);
GB44P_REVERSED("NiD3D10Pass::SetConstantMaps", 0x008C1510, Pass_SetConstantMaps);
GB44P_REVERSED("NiD3D10Pass::SetShaderResources", 0x008C16A0, Pass_SetShaderResources);
GB44P_REVERSED("NiD3D10Pass::SetupRenderingPass", 0x008C1850, Pass_SetupRenderingPass);
GB44P_REVERSED("NiD3D10Pass::ResetTextures", 0x008C18D0, Pass_ResetTexturesImpl);
GB44P_REVERSED("NiD3D10Pass::SetTextureInfo", 0x008C1A10, Pass_SetTextureInfo);
GB44P_REVERSED("NiD3D10Pass::SetSamplerName", 0x008C1A70, Pass_SetSamplerName);
GB44P_REVERSED("NiD3D10Pass::~NiD3D10Pass", 0x008C1AB0, Pass_DestructImpl);
GB44P_REVERSED("NiD3D10Pass::NiD3D10Pass", 0x008C1D00, Pass_ConstructImpl);
GB44P_REVERSED("NiD3D10Pass::DeletingDestructor", 0x008C1F20, Pass_DeletingDestructor);
GB44P_REVERSED("NiD3D10Pass::CreateNewPass", 0x008C1F50, Pass_CreateNewPass);
GB44P_REVERSED("NiD3D10Pass::SetVertexConstantMap", 0x008C1FB0, Pass_SetVertexConstantMap);
GB44P_REVERSED("NiD3D10Pass::SetGeometryConstantMap", 0x008C2020, Pass_SetGeometryConstantMap);
GB44P_REVERSED("NiD3D10Pass::SetPixelConstantMap", 0x008C2090, Pass_SetPixelConstantMap);

} // namespace ni::c44
