// NiD3D10Shader (vtable 0x9A1578, 34 slots; NiRTTI 0x20E922C on NiD3D10ShaderInterface's; 0x70
// bytes): an NiD3D10ShaderInterface (0x1C: NiShader's 0x14, +0x14 its best-implementation flag,
// +0x18 its shader declaration) drawing in passes. +0x10 (NiShader's) whether it's initialized,
// +0x1C its render state group (held), +0x20 / +0x30 / +0x40 its global vertex / geometry / pixel
// constant maps (NiTObjectArray<NiD3D10ShaderConstantMapPtr>, vtable 0x9893C0: +0x4 the array,
// +0xA the size), +0x50 the current pass's index, +0x54 its passes (the same kind: +0x58 the array,
// +0x5E the count), +0x64 the current pass, +0x68 the bone matrix registers (4), +0x6C / +0x6D the
// bone matrices' transposition / world space flags, +0x6E whether it uses the NiProperty state.
//
// Its slots here (the DX9 NiD3DShader's names): 0 the deleting destructor, 2 GetRTTI, 4 Initialize,
// 6 PreProcessPipeline, 8 SetupRenderingPass, 9 SetupTransformations, 10 SetupShaderPrograms, 11
// PrepareGeometryForRendering (30 / 31 its particles' and lines' kinds: guesses), 15 FirstPass, 16
// NextPass, 21 SetUsesNiRenderState, 27 SetShaderDecl (NiD3D10ShaderInterface's: the same code is
// NiD3D10Pass's SetRenderStateGroup), 28 / 29 Destroy / RecreateRendererData. A pass (NiD3D10Pass,
// src/gamebryo/NiD3D10Pass.cpp) has its vertex / geometry / pixel programs at +0x4C / +0x50 / +0x54.
//
// Also the statics the D3D10 shaders share (the SCM extra data's name "__NDL_SCMData", five
// callbacks), set up and let go by the renderer's static initialization.

#include "gamebryo/NiD3D10C44.h"
#include "reversed.h"

namespace ni::c44 {
namespace {

constexpr uintptr_t kShaderVtable = 0x009A1578;
constexpr uintptr_t kMapArrayVtable = 0x009893C0;   // NiTObjectArray<...Ptr>
constexpr uint32_t kShaderSize = 0x70;

GAME_VAR(uint8_t, NiRTTI_NiD3D10Shader, 0x020E922C);
GAME_VAR(char*, ms_kSCMExtraDataName, 0x020E9260);
GAME_ARRAY(void*, ms_apfnCallbacks, 0x020E9218);   // [5] (a guess: the shaders' user callbacks)
constexpr uintptr_t kNoOp = 0x0049A650;            // (a lone ret: the defaults)

// the shader interface's (chunk 43's)
GAME_FN(void*, __thiscall, ShaderInterface_Construct, 0x008B93E0, void* self);
GAME_FN(void, __thiscall, ShaderInterface_Destruct, 0x008B9420, void* self);
// the renderer's (chunk 42's) and its managers'
GAME_FN(void*, __thiscall, Renderer_GetRenderStateManager, 0x008A9110, const void* renderer);
GAME_FN(void*, __thiscall, Renderer_GetShaderResourceManager, 0x008A9130, const void* renderer);
GAME_FN(float*, __thiscall, Renderer_GetInverseViewMatrix, 0x008A93A0, void* self);
GAME_FN(void, __thiscall, Renderer_SetModelTransform, 0x008A94A0, void* self, const void* transform);
GAME_FN(void, __thiscall, Renderer_CalculateBoneMatrices, 0x008ACEF0, void* self, void* skin, const void* world,
        int transposed, uint32_t registers, int worldSpace);
GAME_FN(uint8_t, __thiscall, Renderer_PackGeometry, 0x008ABCB0, void* self, void* data, void* model, void* skin,
        void* declaration, int force);
GAME_FN(uint8_t, __thiscall, Renderer_PackLines, 0x008A9DE0, void* self, void* data, void* model, void* declaration,
        int force);
GAME_FN(uint8_t, __thiscall, Renderer_PackParticles, 0x008ADA50, void* self, void* data, void* model,
        void* declaration, const float* right, const float* up, int force);
GAME_FN(uint8_t, __thiscall, RenderStateManager_SetProperties, 0x008B14D0, void* self, const void* state);
GAME_FN(uint8_t, __thiscall, RenderStateManager_SetRenderStateGroup, 0x008B0F40, void* self, const void* group);
GAME_FN(void, __thiscall, ConstantManager_SetConstantMap, 0x008AFCD0, void* self, void* program, void* map);
// NiD3D10GeometryData's (chunk 43's): its flags, the input layout from a vertex shader's signature
GAME_FN(uint32_t, __thiscall, GeometryData_GetFlags, 0x008B7460, void* data);
GAME_FN(void, __thiscall, GeometryData_SetFlags, 0x008B7620, void* data, uint32_t flags);
GAME_FN(uint32_t, __cdecl, GeometryData_MakeFlags, 0x008B7630, int flag400000, int flag800000, int flag200000,
        uint32_t count);
GAME_FN(uint8_t, __cdecl, GeometryData_FlagsHave400000, 0x008B7670, uint32_t flags);
GAME_FN(void, __thiscall, GeometryData_UpdateInputLayout, 0x008B7DD0, void* data, void* signature, uint32_t length);
// the rest of the library
GAME_FN(void, __cdecl, ShaderDeclaration_CreateDeclForGeometry, 0x0077D090, void* geometry);
GAME_FN(void*, __thiscall, ObjectNET_GetExtraData, 0x00764250, void* self, const re::Ptr32<char>* name);
GAME_FN(float*, __thiscall, Matrix3_TimesPoint, 0x00768A70, const float* self, float* out, const float* p);
GAME_FN(void*, __thiscall, Program_GetShaderByteCode, 0x0077E8D0, void* program);   // (+0x10: an ID3D10Blob)
GAME_FN(void, __cdecl, NiTArray_Free, 0x00836F80, void* p);
// NiD3D10ShaderConstantMap's
GAME_FN(uint32_t, __thiscall, ConstantMap_SetShaderConstants, 0x008C0090, void* self, void* geometry, void* skin,
        void* partition, void* bufferData, void* state, void* effects, const void* world, const void* bound,
        uint32_t pass, int global);
// this file's (called where modes apply)
GAME_FN(void, __thiscall, Shader_Destruct, 0x008C0E40, void* self);

inline void* PassVertexShader(void* pass) {
    return At<void*>(pass, 0x4C);
}
inline void* PassGeometryShader(void* pass) {
    return At<void*>(pass, 0x50);
}
inline void* PassPixelShader(void* pass) {
    return At<void*>(pass, 0x54);
}
inline uint16_t PassCount(const uint8_t* self) {
    return At<uint16_t>(self, 0x5E);
}
inline void* PassAt(const uint8_t* self, uint32_t index) {
    return At<re::Ptr32<void>*>(self, 0x58)[index];
}

// each pass's programs' slot called (5 DestroyRendererData, 6 RecreateRendererData)
void ForEachProgram(uint8_t* self, int slot) {
    uint32_t count = PassCount(self);
    for (uint32_t i = 0; i < count; i++) {
        void* pass = PassAt(self, i);
        if (pass == nullptr) {
            continue;
        }
        if (void* program = PassVertexShader(pass)) {
            Virtual<void>(program, slot);
        }
        if (void* program = PassGeometryShader(pass)) {
            Virtual<void>(program, slot);
        }
        if (void* program = PassPixelShader(pass)) {
            Virtual<void>(program, slot);
        }
    }
}

// the geometry data's input layout made from the current pass's vertex shader's byte code
void UpdateInputLayout(uint8_t* self, void* bufferData) {
    void* blob = Program_GetShaderByteCode(PassVertexShader(At<void*>(self, 0x64)));
    uint32_t length = Com<uint32_t>(blob, 4);    // ID3D10Blob::GetBufferSize
    void* code = Com<void*>(blob, 3);            // GetBufferPointer
    GeometryData_UpdateInputLayout(bufferData, code, length);
}

// whether the geometry's buffers are packed again: on the first pass, or on any for dynamic data
// with the flag (the data's +0x34 consistency: 0x8000 volatile)
inline bool PackThisPass(const uint8_t* self, const uint8_t* geometry, int repack) {
    if (At<uint32_t>(self, 0x50) == 0) {
        return true;
    }
    return (At<uint16_t>(At<uint8_t*>(geometry, 0xCC), 0x34) & 0xF000) == 0x8000 && repack != 0;
}

// a constant map array's maps set for the program
void SetMaps(uint8_t* self, int arrayOffset, void* program, void* manager, void* geometry, void* skin,
             void* partition, void* bufferData, void* state, void* effects, const void* world, const void* bound) {
    uint32_t count = At<uint16_t>(self, arrayOffset + 0xA);
    for (uint32_t i = 0; i < count; i++) {
        void* map = At<re::Ptr32<void>*>(self, arrayOffset + 4)[i];
        if (map == nullptr) {
            continue;
        }
        ConstantMap_SetShaderConstants(map, geometry, skin, partition, bufferData, state, effects, world, bound,
                                       At<uint32_t>(self, 0x50), 1);
        ConstantManager_SetConstantMap(manager, program, map);
    }
}

} // namespace

// 0x8C06D0 (thiscall, slot 8): SetupRenderingPass (the current pass's own, slot 5)
uint32_t __fastcall Shader_SetupRenderingPass(uint8_t* self, re::Edx, void* geometry, void* skin, void* bufferData,
                                              void* state, void* effects, const void* world, const void* bound) {
    return Virtual<uint32_t>(At<void*>(self, 0x64), 5, geometry, skin, bufferData, state, effects, world, bound, 0);
}

// 0x8C0710 (thiscall, slot 9): SetupTransformations (on the first pass: the skin's bone matrices
// computed, the model transform set)
uint32_t __fastcall Shader_SetupTransformations(uint8_t* self, re::Edx, void*, void* skin, void*, void*, void*,
                                                void*, const void* world, const void*) {
    uint8_t* renderer = D3D10Renderer_GetRenderer();
    if (At<uint32_t>(self, 0x50) == 0) {
        if (skin != nullptr) {
            Renderer_CalculateBoneMatrices(renderer, skin, world, At<uint8_t>(self, 0x6C), At<uint32_t>(self, 0x68),
                                           At<uint8_t>(self, 0x6D));
        }
        Renderer_SetModelTransform(renderer, world);
    }
    return 0;
}

// 0x8C0760 (thiscall, slot 21): SetUsesNiRenderState
uint8_t __fastcall Shader_SetUsesNiRenderState(uint8_t* self, re::Edx, int uses) {
    At<uint8_t>(self, 0x6E) = uint8_t(uses);
    return 1;
}

// 0x8C0770 (thiscall): the bone matrices' parameters (transposed, registers, world space; a guess
// at the names)
void __fastcall Shader_SetBoneParameters(uint8_t* self, re::Edx, int transposed, uint32_t registers, int worldSpace) {
    At<uint8_t>(self, 0x6C) = uint8_t(transposed);
    At<uint32_t>(self, 0x68) = registers;
    At<uint8_t>(self, 0x6D) = uint8_t(worldSpace);
}

// 0x8C07A0 (thiscall, slot 6): PreProcessPipeline (-1 when not initialized; the property state
// and the shader's render state group set on the render state manager)
uint32_t __fastcall Shader_PreProcessPipeline(uint8_t* self, re::Edx, void*, void*, void*, void* state, void*,
                                              const void*, const void*) {
    if (At<uint8_t>(self, 0x10) == 0) {
        return 0xFFFFFFFF;
    }
    void* manager = Renderer_GetRenderStateManager(D3D10Renderer_GetRenderer());
    if (At<uint8_t>(self, 0x6E) != 0) {
        RenderStateManager_SetProperties(manager, state);
    }
    void* group = At<void*>(self, 0x1C);
    if (group != nullptr) {
        RenderStateManager_SetRenderStateGroup(manager, group);
    }
    return 0;
}

// 0x8C07F0 (thiscall, slot 15): FirstPass
void __fastcall Shader_FirstPass(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0x50) = 0;
    At<void*>(self, 0x64) = PassCount(self) != 0 ? PassAt(self, 0) : nullptr;
}

// 0x8C0810 (thiscall, slot 16): NextPass (the current one's post-processing, slot 6; the passes
// left)
uint32_t __fastcall Shader_NextPass(uint8_t* self, re::Edx) {
    void* pass = At<void*>(self, 0x64);
    if (pass != nullptr) {
        Virtual<void>(pass, 6, At<uint32_t>(self, 0x50));
    }
    At<uint32_t>(self, 0x50) += 1;
    uint32_t count = PassCount(self);
    uint32_t index = At<uint32_t>(self, 0x50);
    if (index < count) {
        At<void*>(self, 0x64) = PassAt(self, index);
        return count - index;
    }
    At<void*>(self, 0x64) = nullptr;
    return 0;
}

// 0x8C0850 (thiscall, slot 28): DestroyRendererData (each pass's programs')
void __fastcall Shader_DestroyRendererData(uint8_t* self, re::Edx) {
    ForEachProgram(self, 5);
    At<uint8_t>(self, 0x10) = 0;
}

// 0x8C08C0 (thiscall, slot 29): RecreateRendererData (with a renderer)
void __fastcall Shader_RecreateRendererData(uint8_t* self, re::Edx) {
    if (D3D10Renderer_GetRenderer() == nullptr) {
        return;
    }
    ForEachProgram(self, 6);
    At<uint8_t>(self, 0x10) = 1;
}

// 0x8C0940 (cdecl): the statics set up (the SCM extra data's name, the callbacks the no-op)
void __cdecl Shader_InitializeStatics() {
    FixedString_AssignText(&ms_kSCMExtraDataName, Text(0x0099C0C8));   // "__NDL_SCMData"
    for (int i = 0; i < 5; i++) {
        ms_apfnCallbacks[i] = reinterpret_cast<void*>(kNoOp);
    }
}

// 0x8C0990 (cdecl): the statics let go
void __cdecl Shader_ShutdownStatics() {
    FixedString_AssignText(&ms_kSCMExtraDataName, nullptr);
}

// 0x8C09A0 (thiscall, slot 11): PrepareGeometryForRendering (the geometry's declaration made when
// it has none; its buffers packed when they need to be; the input layout for the vertex shader)
void* __fastcall Shader_PrepareGeometryForRendering(uint8_t* self, re::Edx, uint8_t* geometry, int repack,
                                                    void* bufferData, void*) {
    uint8_t* renderer = D3D10Renderer_GetRenderer();
    void* declaration = At<void*>(geometry, 0xE4);
    if (declaration == nullptr) {
        ShaderDeclaration_CreateDeclForGeometry(geometry);
        declaration = At<void*>(geometry, 0xE4);
    }
    if (PackThisPass(self, geometry, repack)) {
        Renderer_PackGeometry(renderer, bufferData, At<void*>(geometry, 0xCC), At<void*>(geometry, 0xD0), declaration,
                              0);
    }
    UpdateInputLayout(self, bufferData);
    return bufferData;
}

// 0x8C0A40 (thiscall, slot 30): the same for particles (the declaration made with the data's flags
// for one... point sprite, as it has them; the camera's right and up in model space packed with them)
void* __fastcall Shader_PrepareParticlesForRendering(uint8_t* self, re::Edx, uint8_t* geometry, int repack,
                                                     void* bufferData, void*) {
    uint8_t* renderer = D3D10Renderer_GetRenderer();
    void* declaration = At<void*>(geometry, 0xE4);
    if (declaration == nullptr) {
        uint32_t flags = GeometryData_GetFlags(bufferData);
        uint8_t has = GeometryData_FlagsHave400000(flags);
        GeometryData_SetFlags(bufferData, GeometryData_MakeFlags(has, 1, 1, 1));
        ShaderDeclaration_CreateDeclForGeometry(geometry);
        declaration = At<void*>(geometry, 0xE4);
        GeometryData_SetFlags(bufferData, flags);
    }
    if (PackThisPass(self, geometry, repack)) {
        const float* inverse = Renderer_GetInverseViewMatrix(renderer);
        const float* rotate = reinterpret_cast<const float*>(geometry + 0x60);   // its world rotation
        float v[3] = {inverse[0], inverse[1], inverse[2]};
        float right[3];
        Matrix3_TimesPoint(rotate, right, v);
        v[0] = inverse[4];
        v[1] = inverse[5];
        v[2] = inverse[6];
        float up[3];
        Matrix3_TimesPoint(rotate, up, v);
        Renderer_PackParticles(renderer, bufferData, At<void*>(geometry, 0xCC), declaration, right, up, 0);
    }
    UpdateInputLayout(self, bufferData);
    return bufferData;
}

// 0x8C0B80 (thiscall, slot 31): the same for lines
void* __fastcall Shader_PrepareLinesForRendering(uint8_t* self, re::Edx, uint8_t* geometry, int repack,
                                                 void* bufferData, void*) {
    uint8_t* renderer = D3D10Renderer_GetRenderer();
    void* declaration = At<void*>(geometry, 0xE4);
    if (declaration == nullptr) {
        ShaderDeclaration_CreateDeclForGeometry(geometry);
        declaration = At<void*>(geometry, 0xE4);
    }
    if (PackThisPass(self, geometry, repack)) {
        Renderer_PackLines(renderer, bufferData, At<void*>(geometry, 0xCC), declaration, 0);
    }
    UpdateInputLayout(self, bufferData);
    return bufferData;
}

// 0x8C0C20 (thiscall, slot 10): SetupShaderPrograms (the SCM extra data's cursors reset; the
// current pass's own set up, slot 2; each program's global constant maps set; the pass's
// post-setup, slot 3)
uint32_t __fastcall Shader_SetupShaderPrograms(uint8_t* self, re::Edx, void* geometry, void* skin, void* partition,
                                           void* bufferData, void* state, void* effects, const void* world,
                                           const void* bound) {
    if (geometry != nullptr) {
        uint8_t* scm = static_cast<uint8_t*>(ObjectNET_GetExtraData(geometry, &ms_kSCMExtraDataName));
        if (scm != nullptr) {
            At<uint32_t>(scm, 0x18) = 0;
            At<uint32_t>(scm, 0x20) = 0;
            At<uint32_t>(scm, 0x1C) = 0;
        }
    }
    Virtual<void>(At<void*>(self, 0x64), 2, geometry, skin, partition, bufferData, state, effects, world, bound,
                  At<uint32_t>(self, 0x50));
    void* manager = Renderer_GetShaderResourceManager(D3D10Renderer_GetRenderer());
    if (void* program = PassVertexShader(At<void*>(self, 0x64))) {
        SetMaps(self, 0x20, program, manager, geometry, skin, partition, bufferData, state, effects, world, bound);
    }
    if (void* program = PassGeometryShader(At<void*>(self, 0x64))) {
        SetMaps(self, 0x30, program, manager, geometry, skin, partition, bufferData, state, effects, world, bound);
    }
    if (void* program = PassPixelShader(At<void*>(self, 0x64))) {
        SetMaps(self, 0x40, program, manager, geometry, skin, partition, bufferData, state, effects, world, bound);
    }
    return Virtual<uint32_t>(At<void*>(self, 0x64), 3, geometry, skin, partition, bufferData, state, effects, world,
                             bound, At<uint32_t>(self, 0x50));
}

// 0x8C0E40 (thiscall): the destructor (the arrays' storage freed, the render state group let go)
void __fastcall Shader_DestructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kShaderVtable;
    At<uint32_t>(self, 0x54) = kMapArrayVtable;
    NiTArray_Free(At<void*>(self, 0x58));
    At<uint32_t>(self, 0x40) = kMapArrayVtable;
    NiTArray_Free(At<void*>(self, 0x44));
    At<uint32_t>(self, 0x30) = kMapArrayVtable;
    NiTArray_Free(At<void*>(self, 0x34));
    At<uint32_t>(self, 0x20) = kMapArrayVtable;
    NiTArray_Free(At<void*>(self, 0x24));
    ReleaseRef(At<void*>(self, 0x1C));
    ShaderInterface_Destruct(self);
}

// 0x8C0EB0 (slot 2): GetRTTI
void* __fastcall Shader_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiD3D10Shader;
}

// 0x8C0EC0 (thiscall, slot 0): the deleting destructor
void* __fastcall Shader_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    Shader_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, kShaderSize);
    }
    return self;
}

// 0x8C0EE0 (thiscall): the constructor
void* __fastcall Shader_Construct(uint8_t* self, re::Edx) {
    ShaderInterface_Construct(self);
    At<uint32_t>(self, 0) = kShaderVtable;
    At<void*>(self, 0x1C) = nullptr;
    static const int kArrays[3] = {0x20, 0x30, 0x40};
    for (int array : kArrays) {
        At<uint16_t>(self, array + 8) = 0;
        At<uint16_t>(self, array + 0xA) = 0;
        At<uint16_t>(self, array + 0xC) = 0;
        At<void*>(self, array + 4) = nullptr;
        At<uint16_t>(self, array + 0xE) = 1;
        At<uint32_t>(self, array) = kMapArrayVtable;
    }
    At<uint32_t>(self, 0x50) = 0;
    At<uint16_t>(self, 0x5C) = 0;
    At<uint16_t>(self, 0x5E) = 0;
    At<uint16_t>(self, 0x60) = 0;
    At<void*>(self, 0x58) = nullptr;
    At<uint16_t>(self, 0x62) = 1;
    At<uint32_t>(self, 0x54) = kMapArrayVtable;
    At<void*>(self, 0x64) = nullptr;
    At<uint8_t>(self, 0x6C) = 0;
    At<uint8_t>(self, 0x6D) = 0;
    At<uint8_t>(self, 0x6E) = 0;
    At<uint32_t>(self, 0x68) = 4;
    return self;
}

// 0x8C0FE0 (thiscall, NiD3D10ShaderInterface's slot 27): SetShaderDecl (+0x18, held; the old let go
// first). The same code is NiD3D10Pass::SetRenderStateGroup (its +0x18)
void __fastcall ShaderInterface_SetShaderDecl(uint8_t* self, re::Edx, void* declaration) {
    AssignRef(At<void*>(self, 0x18), declaration);
}

// 0x8C06C0: Initialize, a lone jmp to NiD3D10ShaderInterface::Initialize (0x8B93B0)
RE_UNHOOKED("NiD3D10Shader::Initialize", 0x008C06C0);

#define GB44S_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

GB44S_REVERSED("NiD3D10Shader::SetupRenderingPass", 0x008C06D0, Shader_SetupRenderingPass);
GB44S_REVERSED("NiD3D10Shader::SetupTransformations", 0x008C0710, Shader_SetupTransformations);
GB44S_REVERSED("NiD3D10Shader::SetUsesNiRenderState", 0x008C0760, Shader_SetUsesNiRenderState);
GB44S_REVERSED("NiD3D10Shader::SetBoneParameters", 0x008C0770, Shader_SetBoneParameters);
GB44S_REVERSED("NiD3D10Shader::PreProcessPipeline", 0x008C07A0, Shader_PreProcessPipeline);
GB44S_REVERSED("NiD3D10Shader::FirstPass", 0x008C07F0, Shader_FirstPass);
GB44S_REVERSED("NiD3D10Shader::NextPass", 0x008C0810, Shader_NextPass);
GB44S_REVERSED("NiD3D10Shader::DestroyRendererData", 0x008C0850, Shader_DestroyRendererData);
GB44S_REVERSED("NiD3D10Shader::RecreateRendererData", 0x008C08C0, Shader_RecreateRendererData);
GB44S_REVERSED("NiD3D10Shader::InitializeStatics", 0x008C0940, Shader_InitializeStatics);
GB44S_REVERSED("NiD3D10Shader::ShutdownStatics", 0x008C0990, Shader_ShutdownStatics);
GB44S_REVERSED("NiD3D10Shader::PrepareGeometryForRendering", 0x008C09A0, Shader_PrepareGeometryForRendering);
GB44S_REVERSED("NiD3D10Shader::PrepareParticlesForRendering", 0x008C0A40, Shader_PrepareParticlesForRendering);
GB44S_REVERSED("NiD3D10Shader::PrepareLinesForRendering", 0x008C0B80, Shader_PrepareLinesForRendering);
GB44S_REVERSED("NiD3D10Shader::SetupShaderPrograms", 0x008C0C20, Shader_SetupShaderPrograms);
GB44S_REVERSED("NiD3D10Shader::~NiD3D10Shader", 0x008C0E40, Shader_DestructImpl);
GB44S_REVERSED("NiD3D10Shader::GetRTTI", 0x008C0EB0, Shader_GetRTTI);
GB44S_REVERSED("NiD3D10Shader::DeletingDestructor", 0x008C0EC0, Shader_DeletingDestructor);
GB44S_REVERSED("NiD3D10Shader::NiD3D10Shader", 0x008C0EE0, Shader_Construct);
GB44S_REVERSED("NiD3D10ShaderInterface::SetShaderDecl", 0x008C0FE0, ShaderInterface_SetShaderDecl);

} // namespace ni::c44
