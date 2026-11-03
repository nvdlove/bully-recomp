// NiD3DShader (vtable 0x950668, NiRTTI 0x20E6E58; 0x58 bytes, see NiD3DXEffect.h): an
// NiD3DShaderInterface drawing in passes. +0x24/+0x25 whether it uses NiRenderState / the light
// state, +0x28 its declaration, +0x2C its render state group, +0x30/+0x34 its global pixel /
// vertex constant maps, +0x38 the current pass, +0x3C the pass count, +0x40 the current pass
// (an NiD3DPassPtr: the pass counts at +0x60 plainly), +0x44 the passes (NiTObjectArray, vtable
// 0x950660; +0x48 the array, +0x4E its size).
//
// Its slots here: 5 SetupGeometry, 6 PreProcessPipeline, 8 SetupRenderingPass, 9
// SetupTransformations, 10 SetupShaderPrograms, 11 PrepareGeometryForRendering, 13
// PostProcessPipeline, 14 GetVertexInputSemantics, 15 FirstPass, 16 NextPass, 27 SetShaderDecl,
// 30/31 Destroy / RecreateRendererData, 32..35 the vertex / pixel shader presence, 38 ResetPasses.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::c39 {
using namespace ni::dx9;
namespace {

constexpr uintptr_t kShaderVtable = 0x00950668;
constexpr uintptr_t kPassArrayVtable = 0x00950660;
GAME_VAR(NiFixedString, ms_kSCMDataName, 0x020E6E54);   // "__NDL_SCMData"

GAME_FN(void, __thiscall, Pass_ReturnPassToPool, 0x008846D0, void* pass);
GAME_FN(void, __cdecl, Pass_CreatePassPool, 0x00884660);
GAME_FN(void, __cdecl, Pass_DestroyPassPool, 0x00884970);
GAME_FN(void, __cdecl, TextureStage_CreateStagePool, 0x0089F600);
GAME_FN(void, __cdecl, TextureStage_DestroyStagePool, 0x0089F770);
GAME_FN(void, __thiscall, RenderState_UpdateRenderState, 0x008839F0, void* self, void* properties);
GAME_FN(void, __thiscall, LightManager_SetState, 0x00892260, void* self, void* effects, void* material,
        void* vertexColor);
GAME_FN(void, __thiscall, RenderStateGroup_SetRenderStates, 0x0089DC20, void* self);
GAME_FN(void, __thiscall, RenderStateGroup_RestoreRenderStates, 0x0089DCC0, void* self);
GAME_FN(void, __cdecl, RenderStateGroup_Release, 0x0089E480, void* group);
GAME_FN(void, __thiscall, Renderer_CalculateBoneMatrices, 0x0087C9B0, void* self, void* skin, const void* world,
        int, uint32_t, int);
GAME_FN(void, __thiscall, Renderer_SetSkinnedModelTransform, 0x0087C920, void* self, void* skin, void* partition,
        const void* world);
GAME_FN(void, __thiscall, Renderer_SetModelTransform, 0x0087C850, void* self, const void* world, int push);
GAME_FN(void, __thiscall, Renderer_PackGeometryBuffer, 0x0087A2F0, void* self, void* bufferData, void* data,
        void* skin, void* declaration, int);
GAME_FN(void, __thiscall, Renderer_PackSkinnedGeometryBuffer, 0x0087A690, void* self, void* bufferData, void* data,
        void* skin, void* partition, void* declaration, int);
GAME_FN(void, __thiscall, Renderer_ReleaseD3DShader, 0x008815E0, void* self, void* shader);
GAME_FN(void, __cdecl, ShaderDeclaration_CreateDeclForGeometry, 0x0077D090, void* geometry);
GAME_FN(void*, __thiscall, ObjectNET_GetExtraData, 0x00764250, void* self, const re::Ptr32<const char>* name);
GAME_FN(uint8_t, __thiscall, ObjectNET_RemoveExtraData, 0x007642D0, void* self, const re::Ptr32<const char>* name);
GAME_FN(uint8_t, __thiscall, ObjectNET_AddExtraData, 0x00764990, void* self, void* extraData);
GAME_FN(void*, __thiscall, SCMExtraData_Construct, 0x008CA3F0, void* self, const re::Ptr32<const char>* name,
        uint32_t vertexEntries, uint32_t geometryEntries, uint32_t pixelEntries);
GAME_FN(void, __thiscall, PassArray_SetAt, 0x00746990, void* self, uint32_t index, const re::Ptr32<void>* pass);
GAME_FN(void, __thiscall, PassArray_RemoveAll, 0x00746AE0, void* self);
GAME_FN(void, __cdecl, PassArray_DeleteArray, 0x00747030, void* array);
GAME_FN(void*, __cdecl, ShaderFactory_GetInstance, 0x0089C9C0);
GAME_FN(uint32_t, __thiscall, Shader_GetImplementation, 0x00853140, void* self);
GAME_FN(void, __thiscall, D3DShaderInterface_Destruct, 0x00887160, void* self);

// this file's (called where modes apply)
GAME_FN(void, __cdecl, Shader_SetupSCMExtraData, 0x008898D0, void* shader, void* geometry);

inline uint8_t* Pass(const uint8_t* self) {
    return At<uint8_t*>(self, 0x40);
}

// the current pass set from the array (NiD3DPassPtr's assignment)
inline void SetCurrentPass(uint8_t* self, uint8_t* pass) {
    uint8_t* old = At<uint8_t*>(self, 0x40);
    if (old != pass) {
        if (old != nullptr && --At<uint32_t>(old, 0x60) == 0) {
            Pass_ReturnPassToPool(old);
        }
        At<uint8_t*>(self, 0x40) = pass;
        if (pass != nullptr) {
            At<uint32_t>(pass, 0x60)++;
        }
    }
}

// an entry of a constant map (slot 11) that maps extra data (0x3 in its flags' top nibble)
inline bool IsExtraDataEntry(const uint8_t* entry) {
    return entry != nullptr && (At<uint32_t>(entry, 0x10) & 0xF0000000u) == 0x30000000u;
}

inline uint32_t CountExtraDataEntries(void* map) {
    uint32_t count = 0;
    uint32_t entries = At<uint16_t>(map, 0x28);
    for (uint32_t i = 0; i < entries; i++) {
        if (IsExtraDataEntry(Virtual<uint8_t*>(map, 11, i))) {
            count++;
        }
    }
    return count;
}

// the extra data a map's entries name added to the SCM data (at `list`, counted at `counter`)
inline void AddExtraDataEntries(void* map, void* geometry, uint8_t* scm, int list, int counter, uint32_t global) {
    uint32_t entries = map != nullptr ? At<uint16_t>(map, 0x28) : 0;
    for (uint32_t i = 0; i < entries; i++) {
        uint8_t* entry = Virtual<uint8_t*>(map, 11, i);
        if (!IsExtraDataEntry(entry)) {
            continue;
        }
        void* extra = ObjectNET_GetExtraData(geometry, reinterpret_cast<const re::Ptr32<const char>*>(entry + 8));
        if (extra == nullptr) {
            continue;
        }
        uint8_t* slot = At<uint8_t*>(scm, list) + At<int32_t>(scm, counter) * 8;
        At<uint32_t>(slot, 0) = At<uint32_t>(entry, 0x18) | global;
        At<void*>(slot, 4) = extra;
        At<int32_t>(scm, counter)++;
    }
}

} // namespace

// 0x888740: slot 4, Initialize: a lone jmp to NiD3DShaderInterface::Initialize
RE_UNHOOKED("NiD3DShader::Initialize", 0x00888740);

// 0x888750 (thiscall, slot 13): NiD3DShader::PostProcessPipeline (its render states restored)
uint32_t __fastcall Shader_PostProcessPipeline(uint8_t* self, re::Edx, void*, void*, void*, void*, void*, void*,
                                               void*) {
    if (At<void*>(self, 0x2C) != nullptr) {
        RenderStateGroup_RestoreRenderStates(At<void*>(self, 0x2C));
    }
    return 0;
}

// 0x888770: the pass and texture stage pools made (Name a guess.)
void __cdecl Shader_CreatePools() {
    Pass_CreatePassPool();
    TextureStage_CreateStagePool();
}

// 0x888780: and destroyed
void __cdecl Shader_DestroyPools() {
    Pass_DestroyPassPool();
    TextureStage_DestroyStagePool();
}

// 0x8887E0 (thiscall, slot 8): NiD3DShader::SetupRenderingPass (the current pass's)
uint32_t __fastcall Shader_SetupRenderingPass(uint8_t* self, re::Edx, void* geometry, void* skin, void* bufferData,
                                              void* properties, void* effects, const void* world,
                                              const void* bound) {
    return Virtual<uint32_t>(Pass(self), 2, geometry, skin, bufferData, properties, effects, world, bound,
                             static_cast<void*>(nullptr));
}

// 0x888820 (thiscall, slot 27): NiD3DShader::SetShaderDecl
void __fastcall Shader_SetShaderDecl(uint8_t* self, re::Edx, void* declaration) {
    void* old = At<void*>(self, 0x28);
    if (old != declaration) {
        ReleaseRef(old);
        At<void*>(self, 0x28) = declaration;
        AddRefRef(declaration);
    }
}

// 0x889170 (thiscall, slot 15): NiD3DShader::FirstPass (the first pass made current; the count)
uint32_t __fastcall Shader_FirstPass(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0x38) = 0;
    SetCurrentPass(self, At<re::Ptr32<uint8_t>*>(self, 0x48)[0]);
    return At<uint32_t>(self, 0x3C);
}

// 0x8891B0 (thiscall, slot 16): NiD3DShader::NextPass (the current one finished (slot 3), the next
// made current; how many are left)
uint32_t __fastcall Shader_NextPass(uint8_t* self, re::Edx) {
    if (Pass(self) != nullptr) {
        Virtual<void>(Pass(self), 3, At<uint32_t>(self, 0x38));   // PostProcessRenderingPass
    }
    At<uint32_t>(self, 0x38)++;
    if (At<uint32_t>(self, 0x38) == At<uint32_t>(self, 0x3C)) {
        return 0;
    }
    SetCurrentPass(self, At<re::Ptr32<uint8_t>*>(self, 0x48)[At<uint32_t>(self, 0x38)]);
    return At<uint32_t>(self, 0x3C) - At<uint32_t>(self, 0x38);
}

// 0x889210 (thiscall, slot 6): NiD3DShader::PreProcessPipeline (-1 unless initialized; the
// property state applied, the lights set, its render states)
uint32_t __fastcall Shader_PreProcessPipeline(uint8_t* self, re::Edx, void*, void*, void*, uint8_t* properties,
                                              void* effects, const void*, const void*) {
    if (At<uint8_t>(self, 0x10) == 0) {
        return 0xFFFFFFFF;
    }
    if (At<uint8_t>(self, 0x24) != 0) {
        RenderState_UpdateRenderState(At<void*>(self, 0x1C), properties);
    }
    if (At<uint8_t>(self, 0x25) != 0) {
        void* lights = At<void*>(At<uint8_t*>(self, 0x18), kLightManager);
        if (lights != nullptr) {
            LightManager_SetState(lights, effects, At<void*>(properties, 0x28), At<void*>(properties, 0x2C));
        }
    }
    if (At<void*>(self, 0x2C) != nullptr) {
        RenderStateGroup_SetRenderStates(At<void*>(self, 0x2C));
    }
    return 0;
}

// 0x889270 (thiscall, slot 9): NiD3DShader::SetupTransformations (skinned: the bone matrices on the
// first pass, the vertex blend by the partition's bones, the skinned transform; else the vertex
// blend off and the model transform on the first pass or when the renderer asks every pass)
uint32_t __fastcall Shader_SetupTransformations(uint8_t* self, re::Edx, void*, void* skin, uint8_t* partition, void*,
                                                void*, void*, const void* world, const void*) {
    uint8_t* state = At<uint8_t*>(self, 0x1C);
    if (partition != nullptr) {
        if (At<uint32_t>(self, 0x38) == 0) {
            Renderer_CalculateBoneMatrices(At<void*>(self, 0x18), skin, world, 0, 4, 0);
        }
        state = At<uint8_t*>(self, 0x1C);
        uint32_t blend = At<uint32_t>(state, 0xC + At<uint16_t>(partition, 0x24) * 4);
        if (At<uint32_t>(state, 0x5D8) != blend) {
            Com<long>(At<void*>(state, 0x2238), 57, 151u, blend);   // VERTEXBLEND
            At<uint32_t>(state, 0x5D8) = blend;
        }
        Renderer_SetSkinnedModelTransform(At<void*>(self, 0x18), skin, partition, world);
        return 0;
    }
    if (At<uint32_t>(self, 0x38) == 0 || At<uint8_t>(At<uint8_t*>(self, 0x18), 0xA0) != 0) {
        uint32_t blend = At<uint32_t>(state, 0xC);
        if (At<uint32_t>(state, 0x5D8) != blend) {
            Com<long>(At<void*>(state, 0x2238), 57, 151u, blend);
            At<uint32_t>(state, 0x5D8) = blend;
        }
        Renderer_SetModelTransform(At<void*>(self, 0x18), world, 1);
    }
    return 0;
}

// 0x889340 (thiscall, slot 10): NiD3DShader::SetupShaderPrograms (the geometry's SCM data cleared;
// the current pass's programs; the global constant maps set for its shaders; the geometry's
// declaration on the first pass)
uint32_t __fastcall Shader_SetupShaderPrograms(uint8_t* self, re::Edx, uint8_t* geometry, void* skin, void* partition,
                                               void* bufferData, void* properties, void* effects, const void* world,
                                               const void* bound) {
    if (geometry != nullptr) {
        uint8_t* scm = static_cast<uint8_t*>(ObjectNET_GetExtraData(geometry, &ms_kSCMDataName));
        if (scm != nullptr) {
            At<uint32_t>(scm, 0x18) = 0;
            At<uint32_t>(scm, 0x20) = 0;
            At<uint32_t>(scm, 0x1C) = 0;
        }
    }
    uint32_t result = Virtual<uint32_t>(Pass(self), 1, geometry, skin, partition, bufferData, properties, effects,
                                        world, bound, At<uint32_t>(self, 0x38));
    if (At<void*>(self, 0x30) != nullptr && At<void*>(Pass(self), 0x44) != nullptr) {
        Virtual<uint32_t>(At<void*>(self, 0x30), 12, At<void*>(Pass(self), 0x44), geometry, skin, partition,
                          bufferData, properties, effects, world, bound, At<uint32_t>(self, 0x38), 1u);
    }
    if (At<void*>(self, 0x34) != nullptr && At<void*>(Pass(self), 0x58) != nullptr) {
        Virtual<uint32_t>(At<void*>(self, 0x34), 12, At<void*>(Pass(self), 0x58), geometry, skin, partition,
                          bufferData, properties, effects, world, bound, At<uint32_t>(self, 0x38), 1u);
    }
    if (At<uint32_t>(self, 0x38) == 0 && geometry != nullptr && At<void*>(geometry, 0xE4) != nullptr) {
        void* declaration = Virtual<void*>(At<void*>(geometry, 0xE4), 28);   // GetD3DDeclaration
        uint8_t* state = At<uint8_t*>(self, 0x1C);
        if (declaration != nullptr &&
            (At<uint8_t>(state, 0x20F7) == 0 || At<void*>(state, 0x2100) != declaration)) {
            At<uint8_t>(state, 0x20F7) = 1;
            At<void*>(state, 0x2100) = declaration;
            Com<long>(At<void*>(state, 0x2238), 87, declaration);   // SetVertexDeclaration
        }
    }
    return result;
}

// 0x889490 (thiscall, slot 11): NiD3DShader::PrepareGeometryForRendering (the buffers packed on the
// first pass (or a volatile skinned partition's every pass), the declaration made first; the
// streams and indices set). Returns the buffer data.
uint8_t* __fastcall Shader_PrepareGeometryForRendering(uint8_t* self, re::Edx, uint8_t* geometry, uint8_t* partition,
                                                       uint8_t* bufferData, void*) {
    if (geometry != nullptr) {
        bool pack;
        if (At<uint32_t>(self, 0x38) == 0) {
            pack = true;
        } else {
            uint8_t* data = At<uint8_t*>(geometry, 0xCC);
            pack = (At<uint16_t>(data, 0x34) & 0xF000) == 0x8000 && partition != nullptr;
        }
        if (pack) {
            void* declaration = At<void*>(geometry, 0xE4);
            void* skin = At<void*>(geometry, 0xD0);
            uint8_t* data = At<uint8_t*>(geometry, 0xCC);
            if (declaration == nullptr) {
                ShaderDeclaration_CreateDeclForGeometry(geometry);
                declaration = At<void*>(geometry, 0xE4);
            }
            if (partition != nullptr) {
                bufferData = At<uint8_t*>(partition, 0x28);
                Renderer_PackSkinnedGeometryBuffer(At<void*>(self, 0x18), bufferData, data, skin, partition,
                                                   declaration, 0);
            } else {
                bufferData = At<uint8_t*>(data, 0x3C);
                Renderer_PackGeometryBuffer(At<void*>(self, 0x18), bufferData, data, skin, declaration, 0);
            }
        }
    }
    if (bufferData != nullptr) {
        for (uint32_t i = 0; i < At<uint32_t>(bufferData, 0x20); i++) {
            uint32_t count = At<uint32_t>(bufferData, 0x20);
            uint32_t stride = i < count ? At<uint32_t*>(bufferData, 0x24)[i] : 0;
            uint8_t* chip = i < count ? At<re::Ptr32<uint8_t>*>(bufferData, 0x28)[i] : nullptr;
            Com<long>(At<void*>(self, 0x14), 100, i, At<void*>(chip, 8), 0u, stride);   // SetStreamSource
        }
        Com<long>(At<void*>(self, 0x14), 104, At<void*>(bufferData, 0x34));   // SetIndices
    }
    return bufferData;
}

// 0x889590 (thiscall, slot 14): NiD3DShader::GetVertexInputSemantics (the declaration's used entries,
// 15 at most, each with its stream; ended by usage index -1; -2 without a declaration)
uint8_t __fastcall Shader_GetVertexInputSemantics(uint8_t* self, re::Edx, uint8_t* out) {
    uint8_t* declaration = Virtual<uint8_t*>(self, 26);   // GetShaderDecl
    if (declaration == nullptr) {
        At<uint32_t>(out, 0x10) = 0xFFFFFFFE;
        return 0;
    }
    uint32_t streams = At<uint32_t>(declaration, 0xC);
    uint32_t written = 0;
    for (uint32_t s = 0; s < streams && written != 0xF; s++) {
        uint32_t entries = (At<void*>(declaration, 0x10) != nullptr && s < At<uint32_t>(declaration, 0xC))
                               ? At<uint32_t>(At<uint8_t*>(declaration, 0x10), int(s) * 0x10)
                               : 0;
        for (uint32_t e = 0; e < entries; e++) {
            uint8_t* streamTable = At<uint8_t*>(declaration, 0x10);
            if (s >= At<uint32_t>(declaration, 0xC) || streamTable == nullptr ||
                e >= At<uint32_t>(streamTable, int(s) * 0x10)) {
                continue;
            }
            uint8_t* entry = At<uint8_t*>(streamTable, int(s) * 0x10 + 4) + e * 0x18;
            if (entry == nullptr) {
                continue;
            }
            uint8_t* to = out + written * 0x18;
            At<uint32_t>(to, 0) = s;
            At<uint32_t>(to, 4) = At<uint32_t>(entry, 4);
            At<uint32_t>(to, 8) = At<uint32_t>(entry, 8);
            char* usage = At<char*>(entry, 0xC);
            if (At<char*>(to, 0xC) != usage) {
                ni::sys::FixedStringAddRef(usage);
                ni::sys::FixedStringRelease(&At<char*>(to, 0xC));
                At<char*>(to, 0xC) = usage;
            }
            At<uint32_t>(to, 0x10) = At<uint32_t>(entry, 0x10);
            At<uint32_t>(to, 0x14) = At<uint32_t>(entry, 0x14);
            if (++written == 0xF) {
                break;
            }
        }
    }
    At<uint32_t>(out, written * 0x18 + 0x10) = 0xFFFFFFFF;
    return 1;
}

// 0x889720 (thiscall, slot 38): the passes let go and the counts cleared. (Name a guess.)
void __fastcall Shader_ResetPasses(uint8_t* self, re::Edx) {
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x3C); i++) {
        re::Ptr32<void> none = nullptr;
        PassArray_SetAt(self + 0x44, i, &none);
    }
    uint8_t* pass = At<uint8_t*>(self, 0x40);
    if (pass != nullptr) {
        if (--At<uint32_t>(pass, 0x60) == 0) {
            Pass_ReturnPassToPool(pass);
        }
        At<void*>(self, 0x40) = nullptr;
    }
    At<uint32_t>(self, 0x3C) = 0;
    At<uint32_t>(self, 0x38) = 0;
}

// 0x889770 (thiscall, slot 30): NiD3DShader::DestroyRendererData (every pass's programs' (pixel
// slot 18, vertex slot 24); not initialized after)
void __fastcall Shader_DestroyRendererData(uint8_t* self, re::Edx) {
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x3C); i++) {
        uint8_t* pass = At<re::Ptr32<uint8_t>*>(self, 0x48)[i];
        if (pass == nullptr) {
            continue;
        }
        if (At<void*>(pass, 0x44) != nullptr) {
            Virtual<void>(At<void*>(pass, 0x44), 18);
        }
        if (At<void*>(pass, 0x58) != nullptr) {
            Virtual<void>(At<void*>(pass, 0x58), 24);
        }
    }
    At<uint8_t>(self, 0x10) = 0;
}

// 0x8897C0 (thiscall, slot 31): NiD3DShader::RecreateRendererData (with a renderer: slots 19 / 25;
// initialized again)
void __fastcall Shader_RecreateRendererData(uint8_t* self, re::Edx) {
    if (At<void*>(self, 0x18) == nullptr) {
        return;
    }
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x3C); i++) {
        uint8_t* pass = At<re::Ptr32<uint8_t>*>(self, 0x48)[i];
        if (pass == nullptr) {
            continue;
        }
        if (At<void*>(pass, 0x44) != nullptr) {
            Virtual<void>(At<void*>(pass, 0x44), 19);
        }
        if (At<void*>(pass, 0x58) != nullptr) {
            Virtual<void>(At<void*>(pass, 0x58), 25);
        }
    }
    At<uint8_t>(self, 0x10) = 1;
}

// 0x889810, 0x889840, 0x889870, 0x8898A0 (thiscall, slots 32..35): NiD3DShader::
// GetVSPresentAllPasses / GetVSPresentAnyPass / GetPSPresentAllPasses / GetPSPresentAnyPass
uint8_t __fastcall Shader_GetVSPresentAllPasses(uint8_t* self, re::Edx) {
    for (uint32_t i = 0; i < At<uint16_t>(self, 0x4E); i++) {
        if (At<void*>(At<re::Ptr32<uint8_t>*>(self, 0x48)[i], 0x58) == nullptr) {
            return 0;
        }
    }
    return 1;
}
uint8_t __fastcall Shader_GetVSPresentAnyPass(uint8_t* self, re::Edx) {
    for (uint32_t i = 0; i < At<uint16_t>(self, 0x4E); i++) {
        if (At<void*>(At<re::Ptr32<uint8_t>*>(self, 0x48)[i], 0x58) != nullptr) {
            return 1;
        }
    }
    return 0;
}
uint8_t __fastcall Shader_GetPSPresentAllPasses(uint8_t* self, re::Edx) {
    for (uint32_t i = 0; i < At<uint16_t>(self, 0x4E); i++) {
        if (At<void*>(At<re::Ptr32<uint8_t>*>(self, 0x48)[i], 0x44) == nullptr) {
            return 0;
        }
    }
    return 1;
}
uint8_t __fastcall Shader_GetPSPresentAnyPass(uint8_t* self, re::Edx) {
    for (uint32_t i = 0; i < At<uint16_t>(self, 0x4E); i++) {
        if (At<void*>(At<re::Ptr32<uint8_t>*>(self, 0x48)[i], 0x44) != nullptr) {
            return 1;
        }
    }
    return 0;
}

// 0x8898D0: a geometry's SCM data made for a shader (the old one removed; the global maps' extra
// data entries counted per pass with each pass's own; an NiSCMExtraData filled from the geometry's
// extra data by those entries' names, the global ones flagged 0xFF0000; added to the geometry)
void __cdecl Shader_SetupSCMExtraDataImpl(uint8_t* self, uint8_t* geometry) {
    ObjectNET_RemoveExtraData(geometry, &ms_kSCMDataName);
    uint32_t vertexEntries = 0;
    uint32_t pixelEntries = 0;
    if (At<void*>(self, 0x34) != nullptr) {
        vertexEntries = At<uint16_t>(self, 0x4E) * CountExtraDataEntries(At<void*>(self, 0x34));
    }
    if (At<void*>(self, 0x30) != nullptr) {
        pixelEntries = At<uint16_t>(self, 0x4E) * CountExtraDataEntries(At<void*>(self, 0x30));
    }
    uint32_t passes = At<uint16_t>(self, 0x4E);
    for (uint32_t p = 0; p < passes; p++) {
        uint8_t* pass = At<re::Ptr32<uint8_t>*>(self, 0x48)[p];
        if (pass == nullptr) {
            continue;
        }
        if (At<void*>(pass, 0x34) != nullptr) {
            pixelEntries += CountExtraDataEntries(At<void*>(pass, 0x34));
        }
        if (At<void*>(pass, 0x48) != nullptr) {
            vertexEntries += CountExtraDataEntries(At<void*>(pass, 0x48));
        }
    }
    if (vertexEntries == 0 && pixelEntries == 0) {
        return;
    }
    void* memory = NiNew(0x30);
    uint8_t* scm = memory != nullptr ? static_cast<uint8_t*>(SCMExtraData_Construct(memory, &ms_kSCMDataName,
                                                                                    vertexEntries, 0, pixelEntries))
                                     : nullptr;
    for (uint32_t p = 0; p < passes; p++) {
        uint8_t* pass = At<re::Ptr32<uint8_t>*>(self, 0x48)[p];
        if (pass == nullptr) {
            continue;
        }
        AddExtraDataEntries(At<void*>(self, 0x30), geometry, scm, 0x28, 0x1C, 0xFF0000);
        AddExtraDataEntries(At<void*>(self, 0x34), geometry, scm, 0x24, 0x18, 0xFF0000);
        if (At<void*>(pass, 0x34) != nullptr) {
            AddExtraDataEntries(At<void*>(pass, 0x34), geometry, scm, 0x28, 0x1C, 0);
        }
        if (At<void*>(pass, 0x48) != nullptr) {
            AddExtraDataEntries(At<void*>(pass, 0x48), geometry, scm, 0x24, 0x18, 0);
        }
    }
    ObjectNET_AddExtraData(geometry, scm);
}

// 0x889CB0 (thiscall, slot 5): NiD3DShader::SetupGeometry
uint8_t __fastcall Shader_SetupGeometry(uint8_t* self, re::Edx, void* geometry) {
    Shader_SetupSCMExtraData(self, geometry);
    return 1;
}

// 0x889CD0 (thiscall): NiD3DShader::~NiD3DShader (out of the shader factory when it's the one
// registered under its name (and as the best implementation); out of the renderer's list; its
// render states, passes, constant maps and declaration let go; then NiD3DShaderInterface's)
void __fastcall Shader_DestructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kShaderVtable;
    NiFixedString name = At<NiFixedString>(self, 8);
    if (name != ni::sys::kNullFixedString && *reinterpret_cast<const uint16_t*>(name - 4) != 0) {   // (tested twice)
        void* factory = ShaderFactory_GetInstance();
        NiFixedString shaderName = At<NiFixedString>(self, 8);
        if (Virtual<void*>(factory, 3, shaderName, Shader_GetImplementation(self)) == self) {
            Virtual<void>(factory, 7, At<NiFixedString>(self, 8), Shader_GetImplementation(self));
        }
        if (At<uint8_t>(self, 0x20) != 0 && Virtual<void*>(factory, 3, At<NiFixedString>(self, 8), 0xFFFFFFFFu) == self) {
            Virtual<void>(factory, 7, At<NiFixedString>(self, 8), 0xFFFFFFFFu);
        }
    }
    if (At<void*>(self, 0x18) != nullptr) {
        Renderer_ReleaseD3DShader(At<void*>(self, 0x18), self);
    }
    At<uint8_t>(self, 0x10) = 0;
    RenderStateGroup_Release(At<void*>(self, 0x2C));
    At<uint32_t>(self, 0x38) = 0;
    At<uint32_t>(self, 0x3C) = 0;
    for (uint32_t i = 0; i < At<uint16_t>(self, 0x4E); i++) {
        re::Ptr32<void> none = nullptr;
        PassArray_SetAt(self + 0x44, i, &none);
    }
    PassArray_RemoveAll(self + 0x44);
    At<uint32_t>(self, 0x44) = kPassArrayVtable;
    PassArray_DeleteArray(At<void*>(self, 0x48));
    uint8_t* pass = At<uint8_t*>(self, 0x40);
    if (pass != nullptr && --At<uint32_t>(pass, 0x60) == 0) {
        Pass_ReturnPassToPool(pass);
    }
    ReleaseRef(At<void*>(self, 0x34));
    ReleaseRef(At<void*>(self, 0x30));
    ReleaseRef(At<void*>(self, 0x28));
    D3DShaderInterface_Destruct(self);
}

#define C39M_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C39M_REVERSED("NiD3DShader::PostProcessPipeline", 0x00888750, Shader_PostProcessPipeline);
C39M_REVERSED("NiD3DShader::CreatePools", 0x00888770, Shader_CreatePools);
C39M_REVERSED("NiD3DShader::DestroyPools", 0x00888780, Shader_DestroyPools);
C39M_REVERSED("NiD3DShader::SetupRenderingPass", 0x008887E0, Shader_SetupRenderingPass);
C39M_REVERSED("NiD3DShader::SetShaderDecl", 0x00888820, Shader_SetShaderDecl);
C39M_REVERSED("NiD3DShader::FirstPass", 0x00889170, Shader_FirstPass);
C39M_REVERSED("NiD3DShader::NextPass", 0x008891B0, Shader_NextPass);
C39M_REVERSED("NiD3DShader::PreProcessPipeline", 0x00889210, Shader_PreProcessPipeline);
C39M_REVERSED("NiD3DShader::SetupTransformations", 0x00889270, Shader_SetupTransformations);
C39M_REVERSED("NiD3DShader::SetupShaderPrograms", 0x00889340, Shader_SetupShaderPrograms);
C39M_REVERSED("NiD3DShader::PrepareGeometryForRendering", 0x00889490, Shader_PrepareGeometryForRendering);
C39M_REVERSED("NiD3DShader::GetVertexInputSemantics", 0x00889590, Shader_GetVertexInputSemantics);
C39M_REVERSED("NiD3DShader::ResetPasses", 0x00889720, Shader_ResetPasses);
C39M_REVERSED("NiD3DShader::DestroyRendererData", 0x00889770, Shader_DestroyRendererData);
C39M_REVERSED("NiD3DShader::RecreateRendererData", 0x008897C0, Shader_RecreateRendererData);
C39M_REVERSED("NiD3DShader::GetVSPresentAllPasses", 0x00889810, Shader_GetVSPresentAllPasses);
C39M_REVERSED("NiD3DShader::GetVSPresentAnyPass", 0x00889840, Shader_GetVSPresentAnyPass);
C39M_REVERSED("NiD3DShader::GetPSPresentAllPasses", 0x00889870, Shader_GetPSPresentAllPasses);
C39M_REVERSED("NiD3DShader::GetPSPresentAnyPass", 0x008898A0, Shader_GetPSPresentAnyPass);
C39M_REVERSED("NiD3DShader::SetupSCMExtraData", 0x008898D0, Shader_SetupSCMExtraDataImpl);
C39M_REVERSED("NiD3DShader::SetupGeometry", 0x00889CB0, Shader_SetupGeometry);
C39M_REVERSED("NiD3DShader::~NiD3DShader", 0x00889CD0, Shader_DestructImpl);

} // namespace ni::c39
