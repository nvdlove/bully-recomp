// NiD3DXEffectShader (vtable 0x9502C0, 0x78 bytes): an NiD3DShader that draws one technique of an
// ID3DXEffect (src/gamebryo/NiD3DXEffect.h). Its virtuals here: 0 the deleting destructor, 2
// GetRTTI, 4 Initialize, 5 SetupGeometry, 6 PreProcessPipeline, 9 SetupTransformations, 10
// SetupShaderPrograms, 12 PostRender, 15 FirstPass, 16 NextPass, 28 HandleLostDevice, 29
// HandleResetDevice, 30 DestroyRendererData, 31 RecreateRendererData, 32..35 whether the passes
// have vertex / pixel shaders. Also NiD3DShader's constructor, GetRTTI and deleting destructor,
// its pass array (NiTObjectArray<NiD3DPassPtr>), and NiTimeSyncController's constructor (the
// controller the effect's time parameters want on the geometry).

#include "gamebryo/NiD3DXEffect.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::d3dx;

constexpr uintptr_t NiD3DShader_vtable = 0x00950668;
constexpr uintptr_t NiD3DXEffectShader_vtable = 0x009502C0;
constexpr uintptr_t PassArray_vtable = 0x00950660;
constexpr uintptr_t NiTimeSyncController_vtable = 0x009503E8;
constexpr uintptr_t IID_ID3DXEffect = 0x009A2104;

GAME_VAR(uint8_t, NiD3DXEffectShader_RTTI, 0x020DCE10);
GAME_VAR(uint8_t, NiD3DShader_RTTI, 0x020E6E58);
GAME_VAR(uint8_t, NiTimeSyncController_RTTI, 0x020E3278);
GAME_VAR(const char*, kNullFixedString, 0x00951028);
GAME_VAR(int32_t, NiDX9Renderer_ms_iMaxTextures, 0x00BCB800);   // the texture stages cleared after a pass
GAME_VAR(uint8_t, ms_bOverrideCullMode, 0x00BC74BC);            // (Bully's: the cull mode by draw mode)
GAME_ARRAY(uint32_t, ms_auiCullModes, 0x00BC74C0);              // [4 draw modes][2: mirrored]
GAME_VAR(uint8_t, ms_bWarnMissingDynamicEffects, 0x020E3238);
// the semantic names NiShaderDeclaration entries take (NiFixedString[], by usage)
constexpr uintptr_t ms_akUsageNames = 0x020E31CC;

GAME_FN(void, __cdecl, NiOutputDebugString, 0x0049A650, const char* format, ...);   // (empty)
GAME_FN(void, __cdecl, NiDelete_, 0x00752540, void* p, uint32_t size);
GAME_FN(void*, __cdecl, NiNew_, 0x00752460, uint32_t size);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t size);
GAME_FN(void*, __cdecl, OperatorNew, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, OperatorDelete, 0x005EEAB0, void* p);
GAME_FN(re::Ptr32<uint8_t>*, __cdecl, NiNewPointerArray, 0x00856120, uint32_t count);
// NiD3DShaderInterface / NiD3DShader (other chunks')
GAME_FN(void, __thiscall, D3DShaderInterface_Construct, 0x00886F90, void* self);
GAME_FN(uint8_t, __thiscall, D3DShaderInterface_Initialize, 0x00888740, void* self);
GAME_FN(void, __thiscall, D3DShader_Destruct, 0x00889CD0, void* self);
GAME_FN(uint32_t, __thiscall, D3DShader_PreProcessPipeline, 0x00889210, void* self, void* geometry, void* skin,
        void* rendererData, void* state, void* effects, void* world, void* worldBound);
GAME_FN(uint32_t, __thiscall, D3DShader_SetupTransformations, 0x00889270, void* self, void* geometry, void* skin,
        void* partition, void* rendererData, void* state, void* effects, void* world, void* worldBound);
GAME_FN(uint8_t, __thiscall, D3DShader_SetupGeometry, 0x00889CB0, void* self, void* geometry);
// NiD3DPass's release (its count reached 0)
GAME_FN(void, __thiscall, D3DPass_Release, 0x008846D0, uint8_t* pass);
// NiDX9ShaderDeclaration::Create(renderer, entries, streams), and the usage to its entry's
// register, type, usage name and index
GAME_FN(NiRefObject*, __cdecl, ShaderDeclaration_Create, 0x00885E80, void* renderer, uint32_t entries,
        uint32_t streams);
GAME_FN(uint8_t, __cdecl, ShaderDeclaration_FromD3DUsage, 0x00884CC0, uint32_t usage, uint32_t usageIndex,
        uint32_t* registerIndex, uint32_t* type, uint32_t* usageName, uint32_t* method);
// the renderer and its render state
GAME_FN(void, __thiscall, Renderer_SetSkinnedModelTransforms, 0x0087C9B0, void* renderer, void* skin,
        void* world, int transposeExtra, uint32_t registers, int transpose);
GAME_FN(void, __thiscall, Renderer_SetModelTransform, 0x0087C850, void* renderer, void* world, int push);
GAME_FN(void, __thiscall, RenderState_SetSoftwareVertexProcessing, 0x00883480, void* renderState, int software);
// NiShaderTimeController / NiTimeSyncController (other chunks')
GAME_FN(void, __thiscall, ShaderTimeController_Construct, 0x0077D770, void* self);
GAME_FN(void, __thiscall, TimeSyncController_Destruct, 0x0077D810, void* self);
// this library's, called where modes apply
GAME_FN(uint8_t, __thiscall, Technique_GetVSPresentAllPasses, 0x00749CA0, NiD3DXEffectTechnique* self);
GAME_FN(uint8_t, __thiscall, Technique_GetVSPresentAnyPass, 0x00749CD0, NiD3DXEffectTechnique* self);
GAME_FN(uint8_t, __thiscall, Technique_GetPSPresentAllPasses, 0x00749D00, NiD3DXEffectTechnique* self);
GAME_FN(uint8_t, __thiscall, Technique_GetPSPresentAnyPass, 0x00749D30, NiD3DXEffectTechnique* self);
GAME_FN(uint8_t, __thiscall, Technique_GetVSPresent, 0x00749D60, NiD3DXEffectTechnique* self, uint32_t pass);
GAME_FN(NiD3DXEffectParameter*, __thiscall, Technique_GetFirstParameter, 0x0074A340, NiD3DXEffectTechnique* self);
GAME_FN(NiD3DXEffectParameter*, __thiscall, Technique_GetNextParameter, 0x00749F60, NiD3DXEffectTechnique* self);
GAME_FN(uint32_t, __thiscall, Parameter_SetParameter, 0x0074C690, NiD3DXEffectParameter* self, void* effect,
        void* geometry, void* skin, void* partition, void* rendererData, void* state, void* effects,
        void* world, void* worldBound, uint32_t pass, int vertexShaderPresent);
GAME_FN(NiD3DXEffectFactory*, __cdecl, Factory_GetInstance, 0x00748CB0, int create);
GAME_FN(void, __thiscall, Factory_HandleLostDevice, 0x007487D0, NiD3DXEffectFactory* self, uint32_t id);
GAME_FN(void, __thiscall, Factory_HandleResetDevice, 0x00748850, NiD3DXEffectFactory* self, uint32_t id);
GAME_FN(void, __thiscall, File_DestroyRendererData, 0x007476B0, NiD3DXEffectFile* self);
GAME_FN(void, __thiscall, File_Initialize, 0x00747A60, NiD3DXEffectFile* self, void* renderer);
GAME_FN(NiD3DXEffectTechniqueSet*, __thiscall, File_GetTechniqueSet, 0x007475F0, NiD3DXEffectFile* self,
        const char* name);
GAME_FN(NiD3DXEffectStateManager*, __cdecl, StateManager_GetInstance, 0x0074D710, void* renderer);
GAME_FN(void, __thiscall, StateManager_ResetHighestTexture, 0x0074D1F0, NiD3DXEffectStateManager* self);
GAME_FN(int32_t, __thiscall, StateManager_GetHighestTexture, 0x0066B5D0, NiD3DXEffectStateManager* self);
GAME_FN(void, __thiscall, Shader_CreateShaderDeclaration, 0x00746720, NiD3DXEffectShader* self);
GAME_FN(void, __thiscall, PassArray_SetAt, 0x00746990, NiTArray<uint8_t*>* self, uint32_t index,
        const re::Ptr32<uint8_t>* pass);
GAME_FN(void, __thiscall, PassArray_RemoveAll, 0x00746AE0, NiTArray<uint8_t*>* self);
GAME_FN(void, __thiscall, D3DShader_Construct, 0x007470B0, void* self);
GAME_FN(void*, __thiscall, TimeSyncController_Construct, 0x00746A20, void* self);

inline uint8_t* RenderState() {
    return At<uint8_t*>(NiDX9Renderer_ms_pkRenderer, 0x6B4);
}

// a render state set through the cache (NiDX9RenderState::SetRenderState, inlined)
inline void SetCachedRenderState(uint8_t* renderState, uint32_t state, uint32_t value, int cacheOffset) {
    if (At<uint32_t>(renderState, cacheOffset) != value) {
        Com<long>(At<void*>(renderState, 0x2238), kDevSetRenderState, state, value);
        At<uint32_t>(renderState, cacheOffset) = value;
    }
}

// D3DX's semantics (D3DXSEMANTIC)
struct Semantic {
    uint32_t usage;
    uint32_t usageIndex;
};

// operator new[] of `count` elements of `size`, -1 when it overflows (the compiler's check)
inline void* NewArray(uint32_t count, uint32_t size) {
    uint64_t bytes = uint64_t(count) * size;
    return OperatorNew(bytes > 0xFFFFFFFFull ? 0xFFFFFFFFu : uint32_t(bytes));
}

} // namespace

// ---- NiD3DXEffectShader's virtuals

// 0x7464F0: NextPass (slot 16): the pass moved on; End when it was the last (the passes left)
uint32_t __fastcall NiD3DXEffectShader_NextPass(NiD3DXEffectShader* self, re::Edx) {
    self->currentPass++;
    uint32_t left = self->passCount - self->currentPass;
    if (left == 0) {
        Com<long>(self->d3dxEffect, kEnd);
    }
    return left;
}

// 0x746520..0x746550: whether the passes have vertex / pixel shaders (slots 32..35), its technique's
uint8_t __fastcall NiD3DXEffectShader_GetVSPresentAllPasses(NiD3DXEffectShader* self, re::Edx) {
    return self->technique != nullptr ? Technique_GetVSPresentAllPasses(self->technique) : 0;
}
uint8_t __fastcall NiD3DXEffectShader_GetVSPresentAnyPass(NiD3DXEffectShader* self, re::Edx) {
    return self->technique != nullptr ? Technique_GetVSPresentAnyPass(self->technique) : 0;
}
uint8_t __fastcall NiD3DXEffectShader_GetPSPresentAllPasses(NiD3DXEffectShader* self, re::Edx) {
    return self->technique != nullptr ? Technique_GetPSPresentAllPasses(self->technique) : 0;
}
uint8_t __fastcall NiD3DXEffectShader_GetPSPresentAnyPass(NiD3DXEffectShader* self, re::Edx) {
    return self->technique != nullptr ? Technique_GetPSPresentAnyPass(self->technique) : 0;
}

// 0x746560: NiD3DShader::SetUsesNiRenderState (slot 21; NiD3DXEffectShader's and NiDX9ErrorShader's too)
uint8_t __fastcall NiD3DShader_SetUsesNiRenderState(NiD3DShader* self, re::Edx, int uses) {
    self->usesNiRenderState = uint8_t(uses);
    return 1;
}

// 0x746570: GetRTTI (slot 2)
void* __fastcall NiD3DXEffectShader_GetRTTI(NiD3DXEffectShader*, re::Edx) {
    return &NiD3DXEffectShader_RTTI;
}

// 0x746580: the deleting destructor (slot 0): NiD3DShader's destructor is the whole of it
NiD3DXEffectShader* __fastcall NiD3DXEffectShader_DeletingDestructor(NiD3DXEffectShader* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(NiD3DXEffectShader_vtable);
    D3DShader_Destruct(self);
    if (flags & 1) {
        NiDelete_(self, sizeof(NiD3DXEffectShader));
    }
    return self;
}

// 0x7465B0: Initialize (slot 4): NiD3DShaderInterface's, then the effect made (slot 31) when the
// shader has a name; whether it was initialized (the effect and its technique there)
uint8_t __fastcall NiD3DXEffectShader_Initialize(NiD3DXEffectShader* self, re::Edx) {
    if (self->initialized) {
        return self->initialized;
    }
    if (!D3DShaderInterface_Initialize(self)) {
        return 0;
    }
    const char* name = self->name;
    if (name == kNullFixedString || name == kNullFixedString ||
        *reinterpret_cast<const uint16_t*>(name - 4) == 0) {
        return 0;
    }
    Virtual<void>(self, 31);
    if (self->d3dxEffect == nullptr || self->techniqueHandle == nullptr) {
        return 0;
    }
    return self->initialized;
}

// 0x746600: PostRender (slot 12): EndPass
uint32_t __fastcall NiD3DXEffectShader_PostRender(NiD3DXEffectShader* self, re::Edx, void*, void*, void*, void*,
                                                  void*, void*, void*, void*) {
    if (Com<long>(self->d3dxEffect, kEndPass) < 0) {
        NiOutputDebugString("NiD3DXEffectShader::PostRender - %s - ID3DXEffect::EndPass failed\n", self->name);
    }
    return 0;
}

// 0x746630: FirstPass (slot 15): Begin (the state saved: flags 7 = D3DXFX_DONOTSAVE*), the pass
// count; End at once when there are none
uint32_t __fastcall NiD3DXEffectShader_FirstPass(NiD3DXEffectShader* self, re::Edx) {
    if (Com<long>(self->d3dxEffect, kBegin, &self->passCount, 7u) < 0) {
        NiOutputDebugString("NiD3DXEffectShader::FirstPass - %s - ID3DXEffect::Begin failed\n", self->name);
    }
    self->currentPass = 0;
    if (self->passCount == 0) {
        Com<long>(self->d3dxEffect, kEnd);
    }
    return self->passCount;
}

// 0x746680 / 0x7466A0: HandleLostDevice / HandleResetDevice (slots 28, 29): the factory's, once per
// device reset (the renderer's count)
void __fastcall NiD3DXEffectShader_HandleLostDevice(NiD3DXEffectShader* self, re::Edx) {
    uint32_t id = At<uint32_t>(self->d3dRenderer, 0x4F4);
    Factory_HandleLostDevice(Factory_GetInstance(1), id);
}
void __fastcall NiD3DXEffectShader_HandleResetDevice(NiD3DXEffectShader* self, re::Edx) {
    uint32_t id = At<uint32_t>(self->d3dRenderer, 0x4F4);
    Factory_HandleResetDevice(Factory_GetInstance(1), id);
}

// 0x7466C0: DestroyRendererData (slot 30): the file's effect dropped, the shader declaration
// released, not initialized any more. (The QueryInterface'd effect at +0x64 isn't released)
void __fastcall NiD3DXEffectShader_DestroyRendererData(NiD3DXEffectShader* self, re::Edx) {
    File_DestroyRendererData(self->technique->file);
    if (self->effect != nullptr) {
        Com<unsigned long>(self->effect, kRelease);
        self->effect = nullptr;
        if (self->d3dxEffect != nullptr) {
            self->d3dxEffect = nullptr;
        }
    }
    self->techniqueHandle = nullptr;
    if (self->shaderDecl != nullptr) {
        ReleaseRef(self->shaderDecl);
        self->shaderDecl = nullptr;
    }
    self->initialized = 0;
}

// 0x746720: the shader declaration made from the inputs the passes' vertex shaders read (each
// usage once; a later pass's repeats dropped), unless there is one
void __fastcall NiD3DXEffectShader_CreateShaderDeclaration(NiD3DXEffectShader* self, re::Edx) {
    if (self->shaderDecl != nullptr) {
        return;
    }
    D3DXTechniqueDesc technique;
    Com<long>(self->effect, kGetTechniqueDesc, self->techniqueHandle, &technique);
    D3DXPassDesc* passes = static_cast<D3DXPassDesc*>(NewArray(technique.passes, sizeof(D3DXPassDesc)));
    uint32_t total = 0;
    for (uint32_t i = 0; i < technique.passes; i++) {
        const char* pass = Com<const char*>(self->effect, kGetPass, self->techniqueHandle, i);
        Com<long>(self->effect, kGetPassDesc, pass, &passes[i]);
        if (passes[i].vertexShaderFunction != nullptr) {
            uint32_t count = 0;
            Import<long>(kD3DXGetShaderInputSemantics, passes[i].vertexShaderFunction, static_cast<Semantic*>(nullptr),
                         &count);
            total += count;
        }
    }
    if (total == 0) {
        OperatorDelete(passes);
        return;
    }
    Semantic* semantics = static_cast<Semantic*>(NewArray(total, sizeof(Semantic)));
    uint32_t filled = 0;
    for (uint32_t i = 0; i < technique.passes; i++) {
        if (passes[i].vertexShaderFunction == nullptr) {
            continue;
        }
        uint32_t count = 0;
        Semantic* added = semantics + filled;
        Import<long>(kD3DXGetShaderInputSemantics, passes[i].vertexShaderFunction, added, &count);
        if (i != 0) {
            for (uint32_t j = 0; j < count; j++) {
                for (Semantic* s = semantics; s < added; s++) {
                    if (s->usage == added[j].usage && s->usageIndex == added[j].usageIndex) {
                        total--;
                        added[j].usage = 0xFFFFFFFF;
                        added[j].usageIndex = 0xFFFFFFFF;
                        break;
                    }
                }
            }
        }
        filled += count;
    }
    OperatorDelete(passes);
    if (filled == 0) {
        return;   // (the semantics leak; can't happen: total was not 0)
    }
    NiRefObject* decl = ShaderDeclaration_Create(self->d3dRenderer, total, 1);
    uint32_t entry = 0;
    for (uint32_t k = 0; k < filled; k++) {
        Semantic& s = semantics[k];
        if (s.usage == 0xFFFFFFFF || s.usageIndex == 0xFFFFFFFF) {
            continue;
        }
        uint32_t registerIndex, type, usageName, method;
        if (ShaderDeclaration_FromD3DUsage(s.usage, s.usageIndex, &registerIndex, &type, &usageName, &method)) {
            // SetEntry(stream, entry, register, type, usage name, usage index, method)
            Virtual<uint8_t>(decl, 19, 0u, entry, registerIndex, type,
                             reinterpret_cast<const void*>(ms_akUsageNames + usageName * 4), s.usageIndex, method);
            entry++;
        }
    }
    OperatorDelete(semantics);
    Virtual<void>(decl, 24, int(self->softwareVertexProcessing));   // SetSoftwareVertexProcessing
    Virtual<void>(self, 27, decl);                                  // SetShaderDecl
}

// ---- NiTObjectArray<NiD3DPassPtr> (an NiD3DPass counts its references at +0x60, not interlocked)

namespace {
inline void PassRelease(uint8_t* pass) {
    if (pass != nullptr && --At<int32_t>(pass, 0x60) == 0) {
        D3DPass_Release(pass);
    }
}
} // namespace

// 0x746990: SetAt
void __fastcall PassArray_SetAtImpl(NiTArray<uint8_t*>* self, re::Edx, uint32_t index, const re::Ptr32<uint8_t>* pass) {
    if (index < self->size) {
        if (*pass == nullptr) {
            if (self->base[index] != nullptr) {
                self->esize--;
            }
        } else if (self->base[index] == nullptr) {
            self->esize++;
        }
    } else {
        self->size = uint16_t(index + 1);
        if (*pass != nullptr) {
            self->esize++;
        }
    }
    re::Ptr32<uint8_t>& slot = self->base[index];
    if (slot != *pass) {
        PassRelease(slot);
        slot = *pass;
        if (slot != nullptr) {
            At<int32_t>(slot, 0x60)++;
        }
    }
}

// 0x746AE0: RemoveAll
void __fastcall PassArray_RemoveAllImpl(NiTArray<uint8_t*>* self, re::Edx) {
    for (uint16_t i = 0; i < self->size; i++) {
        re::Ptr32<uint8_t>& slot = self->base[i];
        if (slot != nullptr) {
            PassRelease(slot);
            slot = nullptr;
        }
    }
    self->esize = 0;
    self->size = 0;
}

// 0x747030: delete[] of NiD3DPassPtr (the count before the elements, released last to first)
void __cdecl PassPointerArray_Delete(re::Ptr32<uint8_t>* elements) {
    if (elements == nullptr) {
        return;
    }
    int32_t count = reinterpret_cast<int32_t*>(elements)[-1];
    for (int32_t i = count - 1; i >= 0; i--) {
        PassRelease(elements[i]);
    }
    NiDeleteArray(reinterpret_cast<int32_t*>(elements) - 1, 4);
}

// ---- NiTimeSyncController

// 0x746A20: the constructor (NiShaderTimeController's, no target reference, scaled time off, the
// key times 0)
void* __fastcall NiTimeSyncController_Construct(uint8_t* self, re::Edx) {
    ShaderTimeController_Construct(self);
    At<void*>(self, 0) = reinterpret_cast<void*>(NiTimeSyncController_vtable);
    At<void*>(self, 0x34) = nullptr;
    void* old = At<void*>(self, 0x34);   // (the reference assigned null: nothing to release)
    if (old != nullptr) {
        ReleaseRef(old);
        At<void*>(self, 0x34) = nullptr;
    }
    At<uint16_t>(self, 8) &= 0xFFBF;
    At<float>(self, 0x14) = 0.0f;
    At<float>(self, 0x18) = 0.0f;
    return self;
}

// 0x746A70: GetRTTI (slot 2)
void* __fastcall NiTimeSyncController_GetRTTI(void*, re::Edx) {
    return &NiTimeSyncController_RTTI;
}

// 0x746A80: the deleting destructor (slot 0)
void* __fastcall NiTimeSyncController_DeletingDestructor(void* self, re::Edx, int flags) {
    TimeSyncController_Destruct(self);
    if (flags & 1) {
        NiDelete_(self, 0x38);
    }
    return self;
}

// ---- NiDX9RenderState

// 0x746AA0: the vertex blend for a bone count (D3DRS_VERTEXBLEND through the cache)
void __fastcall RenderState_SetBoneCount(uint8_t* self, re::Edx, int bones) {
    uint32_t value = At<uint32_t>(self, 0xC + uint16_t(bones) * 4);
    SetCachedRenderState(self, 0x97, value, 0x5D8);
}

// ---- NiD3DXEffectShader's pipeline

// 0x746B30: PreProcessPipeline (slot 6): NiD3DShader's; Bully's cull mode by the draw mode; the
// effect made if it isn't (slot 31), its technique set
uint32_t __fastcall NiD3DXEffectShader_PreProcessPipeline(NiD3DXEffectShader* self, re::Edx, void* geometry,
                                                          void* skin, void* rendererData, uint8_t* state,
                                                          void* effects, void* world, void* worldBound) {
    uint32_t result = D3DShader_PreProcessPipeline(self, geometry, skin, rendererData, state, effects, world,
                                                   worldBound);
    if (ms_bOverrideCullMode) {
        uint32_t drawMode = (At<uint16_t>(At<uint8_t*>(state, 0x24), 0x18) >> 10) & 3;
        uint8_t* renderState = RenderState();
        uint32_t mirrored = At<uint32_t>(renderState, 0xF4) != 0 ? 1 : 0;
        uint32_t value = ms_auiCullModes[mirrored + drawMode * 2];
        SetCachedRenderState(renderState, 0x16, value, 0x1D0);   // D3DRS_CULLMODE
    }
    if (self->d3dxEffect == nullptr || self->techniqueHandle == nullptr) {
        Virtual<void>(self, 31);
        if (self->effect == nullptr) {
            NiOutputDebugString("NiD3DXEffectShader::PreProcessPipeline - %s - Effect failed to load\n",
                                self->name);
        }
    }
    Com<long>(self->d3dxEffect, kSetTechnique, self->techniqueHandle);
    return result;
}

// 0x746C10: SetupTransformations (slot 9): with a vertex shader in this pass, the skinning or model
// transforms and the vertex blend set here (else NiD3DShader's); then every parameter of the
// technique set on the effect. (The vertex shader flag goes to the parameters in the low byte of a
// slot whose other bytes are stale: they read the byte)
uint32_t __fastcall NiD3DXEffectShader_SetupTransformations(NiD3DXEffectShader* self, re::Edx, uint8_t* geometry,
                                                            void* skin, uint8_t* partition, void* rendererData,
                                                            void* state, void* effects, void* world,
                                                            void* worldBound) {
    uint32_t result = 0;
    RenderState_SetSoftwareVertexProcessing(self->d3dRenderState, self->softwareVertexProcessing);
    if (Technique_GetVSPresent(self->technique, self->currentPass)) {
        uint8_t* renderState;
        uint32_t value;
        if (partition != nullptr) {
            if (self->currentPass == 0) {
                Renderer_SetSkinnedModelTransforms(self->d3dRenderer, skin, world, 1, self->boneMatrixRegisters,
                                                   self->transposeBones == 1);
            }
            renderState = self->d3dRenderState;
            value = At<uint32_t>(renderState, 0xC + At<uint16_t>(partition, 0x24) * 4);
        } else {
            renderState = self->d3dRenderState;
            value = At<uint32_t>(renderState, 0xC);
        }
        SetCachedRenderState(renderState, 0x97, value, 0x5D8);   // D3DRS_VERTEXBLEND
        Renderer_SetModelTransform(self->d3dRenderer, world, 0);
    } else {
        result = D3DShader_SetupTransformations(self, geometry, skin, partition, rendererData, state, effects, world,
                                                worldBound);
    }
    NiD3DXEffectParameter* parameter = Technique_GetFirstParameter(self->technique);
    uint8_t vertexShader = Technique_GetVSPresent(self->technique, self->currentPass);
    while (parameter != nullptr) {
        uint32_t error = Parameter_SetParameter(parameter, self->d3dxEffect, geometry, skin, partition, rendererData,
                                                state, effects, world, worldBound, self->currentPass, vertexShader);
        if (error != 0) {
            if (error == 0x60) {
                if (ms_bWarnMissingDynamicEffects) {
                    NiOutputDebugString("NiD3DXEffectShader::SetupTransformations - %s - Parameter %s on geometry "
                                        "with name \"%s\" references a nonexistent NiDynamicEffect object. Default "
                                        "values used.\n",
                                        self->name, parameter->name, At<const char*>(geometry, 8));
                }
            } else {
                NiOutputDebugString("NiD3DXEffectShader::SetupTransformations - %s - Parameter %s failed to be set "
                                    "on geometry with name \"%s\"\n",
                                    self->name, parameter->name, At<const char*>(geometry, 8));
            }
        }
        parameter = Technique_GetNextParameter(self->technique);
    }
    return result;
}

// 0x746DD0: SetupShaderPrograms (slot 10): BeginPass and CommitChanges (the state manager catching
// the effect's states), the textures past the highest the pass set cleared; on the first pass the
// geometry's FVF (else its vertex declaration) set
uint32_t __fastcall NiD3DXEffectShader_SetupShaderPrograms(NiD3DXEffectShader* self, re::Edx, uint8_t* geometry,
                                                           void*, uint8_t* partition, uint8_t* rendererData, void*,
                                                           void*, void*, void*) {
    NiD3DXEffectStateManager* manager = StateManager_GetInstance(self->d3dRenderer);
    StateManager_ResetHighestTexture(manager);
    if (Com<long>(self->d3dxEffect, kBeginPass, self->currentPass) < 0) {
        NiOutputDebugString("NiD3DXEffectShader::SetupShaderPrograms - %s - ID3DXEffect::BeginPass failed\n",
                            self->name);
    }
    if (Com<long>(self->d3dxEffect, kCommitChanges) < 0) {
        NiOutputDebugString("NiD3DXEffectShader::SetupShaderPrograms - %s - ID3DXEffect::CommitChanges failed\n",
                            self->name);
    }
    for (int32_t stage = StateManager_GetHighestTexture(manager) + 1; stage < NiDX9Renderer_ms_iMaxTextures;
         stage++) {
        uint8_t* renderState = self->d3dRenderState;
        if (At<void*>(renderState, 0x20A0 + stage * 4) != nullptr) {
            At<void*>(renderState, 0x20A0 + stage * 4) = nullptr;
            Com<long>(At<void*>(renderState, 0x2238), kDevSetTexture, stage, static_cast<void*>(nullptr));
        }
    }
    if (self->currentPass != 0) {
        return 0;
    }
    if (rendererData == nullptr) {
        if (partition != nullptr) {
            rendererData = At<uint8_t*>(partition, 0x28);
        } else {
            rendererData = At<uint8_t*>(At<uint8_t*>(geometry, 0xCC), 0x3C);
        }
    }
    uint8_t* renderState = self->d3dRenderState;
    uint32_t fvf = At<uint32_t>(rendererData, 0xC);
    if (fvf != 0) {
        if (At<uint8_t>(renderState, 0x20F7) != 0 || At<uint32_t>(renderState, 0x20F8) != fvf) {
            At<uint8_t>(renderState, 0x20F7) = 0;
            At<uint32_t>(renderState, 0x20F8) = fvf;
            Com<long>(At<void*>(renderState, 0x2238), kDevSetFVF, fvf);
        }
        return 0;
    }
    void* declaration = At<void*>(rendererData, 0x10);
    if (declaration != nullptr &&
        (At<uint8_t>(renderState, 0x20F7) == 0 || At<void*>(renderState, 0x2100) != declaration)) {
        At<uint8_t>(renderState, 0x20F7) = 1;
        At<void*>(renderState, 0x2100) = declaration;
        Com<long>(At<void*>(renderState, 0x2238), kDevSetVertexDeclaration, declaration);
    }
    return 0;
}

// 0x746F30: SetupGeometry (slot 5): a time controller on the geometry when the technique has time
// parameters (it and its parents then selectively updated), the declaration made, then NiD3DShader's
uint8_t __fastcall NiD3DXEffectShader_SetupGeometry(NiD3DXEffectShader* self, re::Edx, uint8_t* geometry) {
    if (geometry == nullptr) {
        return 0;
    }
    if (self->technique->usesTime) {
        void* memory = NiNew_(0x38);
        void* controller = memory != nullptr ? TimeSyncController_Construct(memory) : nullptr;
        Virtual<void>(controller, 21, geometry);   // SetTarget
        for (uint8_t* object = geometry; object != nullptr; object = At<uint8_t*>(object, 0x18)) {
            At<uint16_t>(object, 0xA8) |= 2;
        }
    }
    Shader_CreateShaderDeclaration(self);
    return D3DShader_SetupGeometry(self, geometry);
}

// 0x746FB0: RecreateRendererData (slot 31): the file's effect (the file initialized for it if it
// has none), QueryInterface'd; the technique's handle from its set. (With no technique at its
// index the game reads address 0x10)
void __fastcall NiD3DXEffectShader_RecreateRendererData(NiD3DXEffectShader* self, re::Edx) {
    NiD3DXEffectFile* file = self->technique->file;
    void* effect = file->effect;
    if (effect == nullptr) {
        File_Initialize(file, self->d3dRenderer);
        effect = file->effect;
        if (effect == nullptr) {
            return;
        }
    }
    self->effect = effect;
    Com<long>(effect, kQueryInterface, reinterpret_cast<const void*>(IID_ID3DXEffect), &self->d3dxEffect);
    NiD3DXEffectTechniqueSet* set = File_GetTechniqueSet(file, self->name);
    if (self->techniqueIndex < set->techniques.size) {
        self->techniqueHandle = set->techniques.base[self->techniqueIndex]->handle;
    } else {
        self->techniqueHandle = *reinterpret_cast<const char* volatile*>(uintptr_t(0x10));
    }
    Shader_CreateShaderDeclaration(self);
}

// ---- constructors

// 0x7470B0: NiD3DShader's constructor (NiD3DShaderInterface's; no passes: an array of 2, emptied)
void* __fastcall NiD3DShader_Construct(NiD3DShader* self, re::Edx) {
    D3DShaderInterface_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiD3DShader_vtable);
    self->usesNiRenderState = 0;
    self->usesNiLightState = 0;
    self->shaderDecl = nullptr;
    self->unknown2C = 0;
    self->unknown30 = 0;
    self->unknown34 = 0;
    self->currentPass = 0;
    self->passCount = 0;
    self->currentPassObject = nullptr;
    NiTArray<uint8_t*>* passes = &self->passes;
    passes->vtable = reinterpret_cast<void*>(PassArray_vtable);
    passes->maxSize = 2;
    passes->growBy = 2;
    passes->size = 0;
    passes->esize = 0;
    passes->base = NiNewPointerArray(2);
    passes->vtable = reinterpret_cast<void*>(PassArray_vtable);
    PassArray_RemoveAll(passes);
    return self;
}

// 0x747120: NiD3DShader's GetRTTI (slot 2)
void* __fastcall NiD3DShader_GetRTTI(NiD3DShader*, re::Edx) {
    return &NiD3DShader_RTTI;
}

// 0x747130: NiD3DShader's deleting destructor (slot 0)
NiD3DShader* __fastcall NiD3DShader_DeletingDestructor(NiD3DShader* self, re::Edx, int flags) {
    D3DShader_Destruct(self);
    if (flags & 1) {
        NiDelete_(self, sizeof(NiD3DShader));
    }
    return self;
}

// 0x747150: the constructor (every pass slot of the array's allocation set empty)
void* __fastcall NiD3DXEffectShader_Construct(NiD3DXEffectShader* self, re::Edx) {
    D3DShader_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiD3DXEffectShader_vtable);
    self->technique = nullptr;
    self->techniqueHandle = nullptr;
    self->effect = nullptr;
    self->d3dxEffect = nullptr;
    self->techniqueIndex = 0;
    self->boneMatrixRegisters = 0;
    self->transposeBones = 0;
    self->softwareVertexProcessing = 0;
    for (uint32_t i = 0; i < self->passes.maxSize; i++) {
        re::Ptr32<uint8_t> none = nullptr;
        PassArray_SetAt(&self->passes, i, &none);
    }
    return self;
}

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NID3DXSHADER_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                                 \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NID3DXSHADER_REVERSED("NiD3DXEffectShader::NextPass", 0x007464F0, &ni::NiD3DXEffectShader_NextPass);
NID3DXSHADER_REVERSED("NiD3DXEffectShader::GetVSPresentAllPasses", 0x00746520, &ni::NiD3DXEffectShader_GetVSPresentAllPasses);
NID3DXSHADER_REVERSED("NiD3DXEffectShader::GetVSPresentAnyPass", 0x00746530, &ni::NiD3DXEffectShader_GetVSPresentAnyPass);
NID3DXSHADER_REVERSED("NiD3DXEffectShader::GetPSPresentAllPasses", 0x00746540, &ni::NiD3DXEffectShader_GetPSPresentAllPasses);
NID3DXSHADER_REVERSED("NiD3DXEffectShader::GetPSPresentAnyPass", 0x00746550, &ni::NiD3DXEffectShader_GetPSPresentAnyPass);
NID3DXSHADER_REVERSED("NiD3DShader::SetUsesNiRenderState", 0x00746560, &ni::NiD3DShader_SetUsesNiRenderState);
NID3DXSHADER_REVERSED("NiD3DXEffectShader::GetRTTI", 0x00746570, &ni::NiD3DXEffectShader_GetRTTI);
NID3DXSHADER_REVERSED("NiD3DXEffectShader::DeletingDestructor", 0x00746580, &ni::NiD3DXEffectShader_DeletingDestructor);
NID3DXSHADER_REVERSED("NiD3DXEffectShader::Initialize", 0x007465B0, &ni::NiD3DXEffectShader_Initialize);
NID3DXSHADER_REVERSED("NiD3DXEffectShader::PostRender", 0x00746600, &ni::NiD3DXEffectShader_PostRender);
NID3DXSHADER_REVERSED("NiD3DXEffectShader::FirstPass", 0x00746630, &ni::NiD3DXEffectShader_FirstPass);
NID3DXSHADER_REVERSED("NiD3DXEffectShader::HandleLostDevice", 0x00746680, &ni::NiD3DXEffectShader_HandleLostDevice);
NID3DXSHADER_REVERSED("NiD3DXEffectShader::HandleResetDevice", 0x007466A0, &ni::NiD3DXEffectShader_HandleResetDevice);
NID3DXSHADER_REVERSED("NiD3DXEffectShader::DestroyRendererData", 0x007466C0, &ni::NiD3DXEffectShader_DestroyRendererData);
NID3DXSHADER_REVERSED("NiD3DXEffectShader::CreateShaderDeclaration", 0x00746720, &ni::NiD3DXEffectShader_CreateShaderDeclaration);
NID3DXSHADER_REVERSED("NiTObjectArray<NiD3DPassPtr>::SetAt", 0x00746990, &ni::PassArray_SetAtImpl);
NID3DXSHADER_REVERSED("NiTimeSyncController::NiTimeSyncController", 0x00746A20, &ni::NiTimeSyncController_Construct);
NID3DXSHADER_REVERSED("NiTimeSyncController::GetRTTI", 0x00746A70, &ni::NiTimeSyncController_GetRTTI);
NID3DXSHADER_REVERSED("NiTimeSyncController::DeletingDestructor", 0x00746A80, &ni::NiTimeSyncController_DeletingDestructor);
NID3DXSHADER_REVERSED("NiDX9RenderState::SetBoneCount", 0x00746AA0, &ni::RenderState_SetBoneCount);
NID3DXSHADER_REVERSED("NiTObjectArray<NiD3DPassPtr>::RemoveAll", 0x00746AE0, &ni::PassArray_RemoveAllImpl);
NID3DXSHADER_REVERSED("NiD3DXEffectShader::PreProcessPipeline", 0x00746B30, &ni::NiD3DXEffectShader_PreProcessPipeline);
NID3DXSHADER_REVERSED("NiD3DXEffectShader::SetupTransformations", 0x00746C10, &ni::NiD3DXEffectShader_SetupTransformations);
NID3DXSHADER_REVERSED("NiD3DXEffectShader::SetupShaderPrograms", 0x00746DD0, &ni::NiD3DXEffectShader_SetupShaderPrograms);
NID3DXSHADER_REVERSED("NiD3DXEffectShader::SetupGeometry", 0x00746F30, &ni::NiD3DXEffectShader_SetupGeometry);
NID3DXSHADER_REVERSED("NiD3DXEffectShader::RecreateRendererData", 0x00746FB0, &ni::NiD3DXEffectShader_RecreateRendererData);
NID3DXSHADER_REVERSED("NiD3DPassPtr::operator delete[]", 0x00747030, &ni::PassPointerArray_Delete);
NID3DXSHADER_REVERSED("NiD3DShader::NiD3DShader", 0x007470B0, &ni::NiD3DShader_Construct);
NID3DXSHADER_REVERSED("NiD3DShader::GetRTTI", 0x00747120, &ni::NiD3DShader_GetRTTI);
NID3DXSHADER_REVERSED("NiD3DShader::DeletingDestructor", 0x00747130, &ni::NiD3DShader_DeletingDestructor);
NID3DXSHADER_REVERSED("NiD3DXEffectShader::NiD3DXEffectShader", 0x00747150, &ni::NiD3DXEffectShader_Construct);
