// NiDX9FragmentShader (vtable 0x99C7B8, NiRTTI 0x20E7054 on NiD3DShader's; 0x70 bytes): the
// generically configurable NiD3DShader the fragment materials (NiStandardMaterial...) build: its
// passes, programs, samplers and alpha overrides set from outside, its texture stages pointed at
// the texturing property's / dynamic effects' textures each update. +0x58 its
// NiFragmentShaderInstanceDescriptor (vtable 0x99C7AC on NiShader::NiShaderInstanceDescriptor's
// 0x99C7A4; 0x18 bytes standalone: +0x4 the material descriptor, its first a NiFixedString).
//
// Its own slots: 0 the deleting destructor, 2 GetRTTI, 7 UpdatePipeline, 9 SetupTransformations,
// 18 CreateShaderDeclaration, 19 AppendRenderPass, 20 SetAlphaOverride, 22 SetGPUProgram, 23
// CreateShaderConstantMap, 24 AppendTextureSampler, 25 GetShaderInstanceDesc, 39 (whether it's
// initialized, initializing it if not: a guess). The rest are NiD3DShader's, most through the
// 5-byte thunks at 0x893160..0x8931F0 (RE_UNHOOKED below).
//
// Also the helpers it uses: NiD3DPass's program and constant map setters (+0x34 the pixel
// constant map, +0x44 the pixel shader, +0x48 the vertex constant map, +0x58 the vertex shader;
// the last one is also what the game's BullyMainFrame calls at 0x893860, the same code folded),
// NiD3DPass::SetRenderState, NiD3DTextureStage's texture coordinate index (its state group at
// +0xC; +0x0 its stage, +0x4 its texture, +0x8 its map flags, +0x58 its effect flags, +0x5C its
// reference count), the passes' NiTObjectArray::SetSize.

#include "gamebryo/NiDX9RendererData.h"
#include "reversed.h"

namespace ni::c40 {
namespace {

constexpr uintptr_t kFragmentShaderVtable = 0x0099C7B8;
constexpr uintptr_t kInstanceDescriptorVtable = 0x0099C7AC;   // NiFragmentShaderInstanceDescriptor
constexpr uintptr_t kShaderInstanceDescriptorVtable = 0x0099C7A4;   // NiShader::NiShaderInstanceDescriptor
GAME_VAR(uint8_t, NiRTTI_NiShaderInstanceDescriptor, 0x020E3198);
GAME_VAR(uint8_t, NiRTTI_NiFragmentShaderInstanceDescriptor, 0x020E40E4);
GAME_VAR(uint8_t, NiRTTI_NiDX9FragmentShader, 0x020E7054);
// the shadow map samplers' semantics (NiFixedString)
GAME_VAR(const char*, ms_kShadowSemantic0, 0x020E23F0);
GAME_VAR(const char*, ms_kShadowSemantic1, 0x020E23F4);
GAME_VAR(const char*, ms_kShadowSemantic2, 0x020E23F8);
// the texture effects' texture transform set-up, by their texture type
GAME_ARRAY(void*, ms_apfnTextureEffectSetup, 0x020E6E40);

// D3DRENDERSTATETYPE / D3DTEXTURESTAGESTATETYPE
constexpr uint32_t D3DRS_SRCBLEND = 0x13;
constexpr uint32_t D3DRS_DESTBLEND = 0x14;
constexpr uint32_t D3DRS_ALPHABLENDENABLE = 0x1B;
constexpr uint32_t D3DRS_VERTEXBLEND = 0x97;
constexpr uint32_t D3DTSS_TEXCOORDINDEX = 0xB;

GAME_FN(void*, __thiscall, D3DShader_Construct, 0x007470B0, void* self);
GAME_FN(void, __thiscall, D3DShader_Destruct, 0x00889CD0, void* self);
GAME_FN(uint32_t, __thiscall, D3DShader_UpdatePipeline, 0x0077C2A0, void* self, void* geometry, void* skin,
        void* bufferData, void* state, void* effects, const NiTransform* world, const NiBound* bound);
GAME_FN(uint32_t, __thiscall, D3DShader_SetupTransformations, 0x00889270, void* self, void* geometry, void* skin,
        void* partition, void* bufferData, void* state, void* effects, const NiTransform* world,
        const NiBound* bound);
GAME_FN(uint8_t, __thiscall, D3DShader_Initialize, 0x00888740, void* self);
GAME_FN(void, __thiscall, MaterialDescriptor_Construct, 0x007D5470, void* self);
GAME_FN(void, __thiscall, MaterialDescriptor_Assign, 0x007D5490, void* self, const void* other);
GAME_FN(void*, __cdecl, ShaderDeclaration_Create, 0x00885E80, void* renderer, uint32_t entries, uint32_t streams);
GAME_FN(void, __cdecl, Pass_CreateNewPass, 0x00884A90, re::Ptr32<uint8_t>* pass);
GAME_FN(void, __thiscall, Pass_ReturnPassToPool, 0x008846D0, void* pass);
GAME_FN(uint8_t*, __cdecl, Pass_GetD3DRenderState, 0x00883C60);
GAME_FN(uint32_t, __thiscall, Pass_SetStage, 0x00884830, void* self, uint32_t index, uint8_t* stage);
GAME_FN(void, __thiscall, PassArray_SetAt, 0x00746990, void* array, uint32_t index, const re::Ptr32<uint8_t>* pass);
GAME_FN(void, __cdecl, PassArray_Delete, 0x00747030, void* array);
GAME_FN(re::Ptr32<void>*, __cdecl, NiNewPointerArray, 0x00856120, uint32_t count);
GAME_FN(void, __cdecl, Stage_CreateNewStage, 0x0089F810, re::Ptr32<uint8_t>* stage);
GAME_FN(void, __thiscall, Stage_ReturnStageToPool, 0x0089F670, void* stage);
GAME_FN(void, __thiscall, Stage_SetClampMode, 0x0089E5E0, void* stage, uint32_t mode);
GAME_FN(void, __thiscall, Stage_SetFilterMode, 0x0089E640, void* stage, uint32_t mode);
GAME_FN(void*, __cdecl, RenderStateGroup_GetFree, 0x0089E3A0);
GAME_FN(void, __thiscall, RenderStateGroup_SetRenderState, 0x0089E1F0, void* self, uint32_t state, uint32_t value,
        int save);
GAME_FN(void, __thiscall, RenderStateGroup_RemoveRenderState, 0x0089E310, void* self, uint32_t state);
GAME_FN(uint8_t, __thiscall, StageGroup_GetStageState, 0x0089FAB0, void* self, uint32_t state, uint32_t* value,
        uint8_t* save);
GAME_FN(void, __thiscall, StageGroup_SetStageState, 0x0089F9E0, void* self, uint32_t state, uint32_t value, int save);
GAME_FN(uint8_t, __cdecl, TexturingProperty_GetMapIDFromName, 0x00765B50, const re::Ptr32<char>* name, uint32_t* id);
GAME_FN(uint8_t, __cdecl, TextureEffect_GetTextureTypeFromName, 0x0076BB50, const re::Ptr32<char>* name, uint32_t* type);
GAME_FN(uint8_t*, __thiscall, TexturingProperty_GetShaderMap, 0x0074A7E0, void* self, uint32_t index);
GAME_FN(uint8_t*, __cdecl, GetDynamicEffectForObject, 0x0077F380, void* effects, uint32_t objectType, uint32_t index);
GAME_FN(void, __thiscall, Renderer_SetSkinnedModelTransforms, 0x0087C9B0, void* self, void* skin,
        const NiTransform* world, int a3, int a4, int a5);
GAME_FN(void, __thiscall, Renderer_SetModelTransform, 0x0087C850, void* self, const NiTransform* world, int push);
// this file's (called where modes apply)
GAME_FN(uint8_t, __cdecl, GetShadowSamplerIndex, 0x00893250, const re::Ptr32<char>* semantic, uint32_t* index);
GAME_FN(uint32_t, __thiscall, Stage_GetTexCoordIndex, 0x008932A0, void* self);
GAME_FN(uint8_t, __thiscall, Stage_GetTexCoordIndexSave, 0x008932E0, void* self);
GAME_FN(void, __thiscall, Pass_SetRenderState, 0x00893310, void* self, uint32_t state, uint32_t value, int save);
GAME_FN(void, __cdecl, SetupTextureEffectTransform, 0x00893340, uint8_t* effect, void* stage, int save,
        void* renderer);
GAME_FN(void, __thiscall, Pass_SetPixelConstantMap, 0x00893770, void* self, NiRefObject* map);
GAME_FN(void, __thiscall, Pass_SetPixelShader, 0x008937C0, void* self, NiRefObject* program);
GAME_FN(void, __thiscall, Pass_SetVertexConstantMap, 0x00893810, void* self, NiRefObject* map);
GAME_FN(void, __thiscall, Pass_SetVertexShader, 0x00893860, void* self, NiRefObject* program);
GAME_FN(void, __thiscall, FragmentShader_UpdateTextureStage, 0x008939C0, void* self, void* geometry, uint8_t* stage,
        uint8_t* texturing, uint8_t* effects, uint32_t pass);
GAME_FN(void, __thiscall, PassArray_SetSize, 0x00893D70, void* self, uint32_t size);
GAME_FN(void*, __thiscall, ConstantMap_Construct, 0x00894150, void* self, void* renderer, uint32_t programType);

inline uint8_t* Pass(const uint8_t* shader, uint32_t index) {
    return At<re::Ptr32<uint8_t>*>(shader, 0x48)[index];
}
// a pass's or stage's reference given back (counted plainly: +0x60 a pass's, +0x5C a stage's)
inline void ReleasePass(uint8_t* pass) {
    if (pass != nullptr && --At<uint32_t>(pass, 0x60) == 0) {
        Pass_ReturnPassToPool(pass);
    }
}
inline void ReleaseStage(uint8_t* stage) {
    if (stage != nullptr && --At<uint32_t>(stage, 0x5C) == 0) {
        Stage_ReturnStageToPool(stage);
    }
}
// NiPointer<NiRefObject>::operator= at a pass's offset
inline void AssignAt(uint8_t* self, int offset, NiRefObject* value) {
    re::Ptr32<NiRefObject>& slot = At<NiRefObject*>(self, offset);
    if (slot != value) {
        if (slot != nullptr && Decrement(&slot->refCount) == 0) {
            Virtual<void>(slot, 1);
        }
        slot = value;
        if (value != nullptr) {
            Increment(&value->refCount);
        }
    }
}
inline void ReleaseDescriptorString(uint8_t* self) {
    ni::sys::FixedStringRelease(&At<char*>(self, 0x5C));
}

} // namespace

// 0x893140 (thiscall): NiDX9FragmentShader slot 39 (initialized (+0x10), or initialized now)
uint8_t __fastcall NiDX9FragmentShader_EnsureInitialized(uint8_t* self, re::Edx, int) {
    if (At<uint8_t>(self, 0x10) != 0) {
        return 1;
    }
    return D3DShader_Initialize(self) != 0 ? 1 : 0;
}

// NiDX9FragmentShader's slots that are NiD3DShader's, through lone 5-byte jmp thunks (nothing to
// hook): 6 PreProcessPipeline 0x889210, 8 SetupRenderingPass 0x8887E0, 10 SetupShaderPrograms
// 0x889340, 11 PrepareGeometryForRendering 0x889490, 12 PostRender 0x77C2B0, 13
// PostProcessPipeline 0x888750, 15 FirstPass 0x889170, 16 NextPass 0x8891B0, 30
// DestroyRendererData 0x889770, 31 RecreateRendererData 0x8897C0
RE_UNHOOKED("NiDX9FragmentShader::PreProcessPipeline", 0x00893160);
RE_UNHOOKED("NiDX9FragmentShader::SetupRenderingPass", 0x00893170);
RE_UNHOOKED("NiDX9FragmentShader::SetupShaderPrograms", 0x00893180);
RE_UNHOOKED("NiDX9FragmentShader::PrepareGeometryForRendering", 0x00893190);
RE_UNHOOKED("NiDX9FragmentShader::PostRender", 0x008931A0);
RE_UNHOOKED("NiDX9FragmentShader::PostProcessPipeline", 0x008931B0);
RE_UNHOOKED("NiDX9FragmentShader::FirstPass", 0x008931C0);
RE_UNHOOKED("NiDX9FragmentShader::NextPass", 0x008931D0);
RE_UNHOOKED("NiDX9FragmentShader::DestroyRendererData", 0x008931E0);
RE_UNHOOKED("NiDX9FragmentShader::RecreateRendererData", 0x008931F0);

// 0x893200 (thiscall, 4 bytes: under a hook's size): NiDX9FragmentShader::GetShaderInstanceDesc
void* __fastcall NiDX9FragmentShader_GetShaderInstanceDesc(uint8_t* self, re::Edx) {
    return self + 0x58;
}
RE_UNHOOKED_FN("NiDX9FragmentShader::GetShaderInstanceDesc", 0x00893200, &NiDX9FragmentShader_GetShaderInstanceDesc);

// 0x893210 (thiscall): NiShader::NiShaderInstanceDescriptor::GetRTTI
void* __fastcall NiShaderInstanceDescriptor_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiShaderInstanceDescriptor;
}

// 0x893220 (thiscall): NiShader::NiShaderInstanceDescriptor's deleting destructor (4 bytes)
void* __fastcall NiShaderInstanceDescriptor_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = kShaderInstanceDescriptorVtable;
    if (flags & 1) {
        NiDelete(self, 4);
    }
    return self;
}

// 0x893250 (cdecl): the shadow map sampler a semantic names (0..2; false for none)
uint8_t __cdecl NiDX9FragmentShader_GetShadowSamplerIndex(const re::Ptr32<char>* semantic, uint32_t* index) {
    const char* name = *semantic;
    if (name == ms_kShadowSemantic2) {
        *index = 2;
        return 1;
    }
    if (name == ms_kShadowSemantic1) {
        *index = 1;
        return 1;
    }
    if (name == ms_kShadowSemantic0) {
        *index = 0;
        return 1;
    }
    return 0;
}

// 0x8932A0 (thiscall): NiD3DTextureStage::GetTexCoordIndex (its state group's
// D3DTSS_TEXCOORDINDEX, else its stage)
uint32_t __fastcall NiD3DTextureStage_GetTexCoordIndex(uint8_t* self, re::Edx) {
    uint32_t value = 0;
    uint8_t save = 0;
    if (StageGroup_GetStageState(At<void*>(self, 0xC), D3DTSS_TEXCOORDINDEX, &value, &save) == 0) {
        value = At<uint32_t>(self, 0);
    }
    return value;
}

// 0x8932E0 (thiscall): NiD3DTextureStage::GetTexCoordIndexSave (its D3DTSS_TEXCOORDINDEX's save
// flag; false when it has none)
uint8_t __fastcall NiD3DTextureStage_GetTexCoordIndexSave(uint8_t* self, re::Edx) {
    uint32_t value = 0;
    uint8_t save = 0;
    uint8_t found = StageGroup_GetStageState(At<void*>(self, 0xC), D3DTSS_TEXCOORDINDEX, &value, &save);
    return found != 0 ? save : 0;
}

// 0x893310 (thiscall): NiD3DPass::SetRenderState (into its render state group, made when it has
// none)
void __fastcall NiD3DPass_SetRenderState(uint8_t* self, re::Edx, uint32_t state, uint32_t value, int save) {
    if (At<void*>(self, 0x30) == nullptr) {
        At<void*>(self, 0x30) = RenderStateGroup_GetFree();
    }
    RenderStateGroup_SetRenderState(At<void*>(self, 0x30), state, value, save);
}

// 0x893340 (cdecl): a texture effect's texture transform set on a stage (its texture type's
// function, given the projection matrix and translation)
void __cdecl NiDX9FragmentShader_SetupTextureEffectTransform(uint8_t* effect, void* stage, int save, void* renderer) {
    auto setup = re::PtrCast<void(__cdecl*)(void*, void*, void*, int, void*)>(
        ms_apfnTextureEffectSetup[At<uint32_t>(effect, 0x168)]);
    setup(effect + 0x128, effect + 0x14C, stage, save, renderer);
}

// 0x893380 (thiscall): NiDX9FragmentShader::CreateShaderDeclaration (made for the renderer, then
// set as its own: slot 27)
void* __fastcall NiDX9FragmentShader_CreateShaderDeclaration(uint8_t* self, re::Edx, uint32_t entries, uint32_t streams) {
    void* declaration = ShaderDeclaration_Create(ni::dx9::ms_pkRenderer, entries, streams);
    Virtual<void>(self, 27, declaration);
    return declaration;
}

// 0x8933B0 (thiscall): NiDX9FragmentShader::SetAlphaOverride (the pass's alpha blending on with
// these blend modes, or the previous ones (the pass's own states removed); or off)
uint8_t __fastcall NiDX9FragmentShader_SetAlphaOverride(uint8_t* self, re::Edx, uint32_t index, int alphaBlend,
                                                        int usePreviousSource, uint32_t source,
                                                        int usePreviousDestination, uint32_t destination) {
    if (index >= At<uint32_t>(self, 0x3C)) {
        return 0;
    }
    uint8_t* pass = Pass(self, index);
    if (pass == nullptr) {
        return 0;
    }
    if (uint8_t(alphaBlend) == 0) {
        if (At<void*>(pass, 0x30) == nullptr) {
            At<void*>(pass, 0x30) = RenderStateGroup_GetFree();
        }
        RenderStateGroup_SetRenderState(At<void*>(pass, 0x30), D3DRS_ALPHABLENDENABLE, 0, 0);
        return 1;
    }
    if (At<void*>(pass, 0x30) == nullptr) {
        At<void*>(pass, 0x30) = RenderStateGroup_GetFree();
    }
    RenderStateGroup_SetRenderState(At<void*>(pass, 0x30), D3DRS_ALPHABLENDENABLE, 1, 0);
    uint8_t* renderState = Pass_GetD3DRenderState();
    if (uint8_t(usePreviousSource) == 0) {
        Pass_SetRenderState(pass, D3DRS_SRCBLEND, At<uint32_t>(renderState, 0x20 + int(source) * 4), 0);
    } else if (At<void*>(pass, 0x30) != nullptr) {
        RenderStateGroup_RemoveRenderState(At<void*>(pass, 0x30), D3DRS_SRCBLEND);
    }
    if (uint8_t(usePreviousDestination) == 0) {
        Pass_SetRenderState(pass, D3DRS_DESTBLEND, At<uint32_t>(renderState, 0x20 + int(destination) * 4), 0);
        return 1;
    }
    if (At<void*>(pass, 0x30) != nullptr) {
        RenderStateGroup_RemoveRenderState(At<void*>(pass, 0x30), D3DRS_DESTBLEND);
    }
    return 1;
}

// 0x893490 (thiscall): NiDX9FragmentShader::AppendTextureSampler (a new stage on the pass, set to
// read the texturing property's map, a texture effect's or a shadow map the semantic names; the
// variable name isn't used. False for a bad pass, a stage the pass refused or a map it can't
// name)
uint8_t __fastcall NiDX9FragmentShader_AppendTextureSampler(uint8_t* self, re::Edx, uint32_t index, uint32_t* sampler,
                                                            const re::Ptr32<char>* semantic, const re::Ptr32<char>*, uint32_t instance) {
    if (index >= At<uint32_t>(self, 0x3C)) {
        return 0;
    }
    uint8_t* pass = Pass(self, index);
    if (pass == nullptr) {
        return 0;
    }
    re::Ptr32<uint8_t> stage = nullptr;
    Stage_CreateNewStage(&stage);
    if (Pass_SetStage(pass, At<uint32_t>(pass, 0x14), stage) != 0) {
        ReleaseStage(stage);
        return 0;
    }
    *sampler = At<uint32_t>(pass, 0x18) - 1;
    uint32_t id = 0;
    if (TexturingProperty_GetMapIDFromName(semantic, &id) != 0) {
        uint32_t& flags = At<uint32_t>(stage, 8);
        if (id < 8) {   // base, dark, detail, gloss, glow, bump, normal, parallax
            flags &= 0xF0FFFFFF;
            flags |= (id + 2) << 20;
        } else if (id == 8) {   // a decal
            flags &= 0xF00FFFFF;
            flags |= instance | 0x1000000;
        } else if (id == 9) {   // a shader map
            flags &= 0xF00FFFFF;
            flags |= instance | 0x2000000;
        } else {
            ReleaseStage(stage);
            return 0;
        }
    } else if (TextureEffect_GetTextureTypeFromName(semantic, &id) != 0) {
        uint32_t kind = 0;
        switch (id) {
        case 0:
            kind = 0xA00;
            break;
        case 1:
            kind = 0x900;
            break;
        case 2:
            kind = 0x800;
            break;
        case 3:
            kind = 0xB00;
            break;
        default:
            break;
        }
        At<uint16_t>(stage, 0x58) = uint16_t((kind & 0xFF00) | instance);
    } else if (GetShadowSamplerIndex(semantic, &id) != 0) {
        uint32_t kind = id == 0 ? 0xD00 : id == 1 ? 0xC00 : id == 2 ? 0xE00 : 0;
        At<uint16_t>(stage, 0x58) = uint16_t((kind & 0xFF00) | instance);
    }
    ReleaseStage(stage);
    return 1;
}

// 0x893770 (thiscall): NiD3DPass::SetPixelConstantMap
void __fastcall NiD3DPass_SetPixelConstantMap(uint8_t* self, re::Edx, NiRefObject* map) {
    AssignAt(self, 0x34, map);
}

// 0x8937C0 (thiscall): NiD3DPass::SetPixelShader
void __fastcall NiD3DPass_SetPixelShader(uint8_t* self, re::Edx, NiRefObject* program) {
    AssignAt(self, 0x44, program);
}

// 0x893810 (thiscall): NiD3DPass::SetVertexConstantMap
void __fastcall NiD3DPass_SetVertexConstantMap(uint8_t* self, re::Edx, NiRefObject* map) {
    AssignAt(self, 0x48, map);
}

// 0x893860 (thiscall): NiD3DPass::SetVertexShader (any NiPointer at +0x58: the game's
// BullyMainFrame's scene setter is this same code)
void __fastcall NiD3DPass_SetVertexShader(uint8_t* self, re::Edx, NiRefObject* program) {
    AssignAt(self, 0x58, program);
}

// 0x8938B0 (thiscall): NiDX9FragmentShader::SetGPUProgram (the pass's vertex or pixel shader, by
// the program's type (+0x8); its type returned)
uint8_t __fastcall NiDX9FragmentShader_SetGPUProgram(uint8_t* self, re::Edx, uint32_t index, NiRefObject* program,
                                                     uint32_t* type) {
    if (index >= At<uint32_t>(self, 0x3C)) {
        return 0;
    }
    uint8_t* pass = Pass(self, index);
    if (pass == nullptr) {
        return 0;
    }
    switch (At<uint32_t>(program, 8)) {
    case 0:
        Pass_SetVertexShader(pass, program);
        *type = 0;
        return 1;
    case 1:
        Pass_SetPixelShader(pass, program);
        *type = 1;
        return 1;
    default:
        return 1;
    }
}

// 0x893910 (thiscall): NiDX9FragmentShader::SetupTransformations (a skin partition: on the first
// pass the bones' transforms set, D3DRS_VERTEXBLEND for its bone count, the model transform; else
// NiD3DShader's)
uint32_t __fastcall NiDX9FragmentShader_SetupTransformations(uint8_t* self, re::Edx, void* geometry, void* skin,
                                                             uint8_t* partition, void* bufferData, void* state,
                                                             void* effects, const NiTransform* world,
                                                             const NiBound* bound) {
    if (partition == nullptr) {
        return D3DShader_SetupTransformations(self, geometry, skin, nullptr, bufferData, state, effects, world, bound);
    }
    if (At<uint32_t>(self, 0x38) == 0) {
        Renderer_SetSkinnedModelTransforms(At<void*>(self, 0x18), skin, world, 1, 3, 1);
    }
    uint8_t* renderState = At<uint8_t*>(self, 0x1C);
    uint32_t blend = At<uint32_t>(renderState, 0xC + int(At<uint16_t>(partition, 0x24)) * 4);
    if (At<uint32_t>(renderState, 0x5D8) != blend) {
        Com<long>(At<void*>(renderState, 0x2238), 57, D3DRS_VERTEXBLEND, blend);   // SetRenderState
        At<uint32_t>(renderState, 0x5D8) = blend;
    }
    Renderer_SetModelTransform(At<void*>(self, 0x18), world, 0);
    return 0;
}

// 0x8939C0 (thiscall): NiDX9FragmentShader::UpdateTextureStage (the stage's texture, clamp and
// filter modes from the texturing property's map its flags name (and its UV set as the texture
// coordinate index), or from the dynamic effect it names (and its texture transform); the pass
// index isn't used)
void __fastcall NiDX9FragmentShader_UpdateTextureStage(uint8_t* self, re::Edx, void* geometry, uint8_t* stage,
                                                       uint8_t* texturing, uint8_t* effects, uint32_t) {
    uint16_t effectFlags = At<uint16_t>(stage, 0x58);
    uint32_t flags = At<uint32_t>(stage, 8);
    void* texture = nullptr;
    uint32_t filter = 0;
    uint32_t clamp = 0;
    if (effectFlags == 0) {
        uint8_t* map = nullptr;
        bool found = true;
        if (flags == 0) {
            found = false;
        } else if ((flags & 0xF000000) == 0) {
            uint32_t kind = flags & 0xF00000;
            if (kind >= 0x200000 && kind <= 0x900000) {
                map = At<re::Ptr32<uint8_t>*>(texturing, 0x20)[(kind >> 20) - 2];
            } else {
                found = false;
            }
        } else if ((flags & 0xF000000) == 0x1000000) {   // a decal
            uint32_t decal = flags & 0xFFFF;
            if (decal >= uint32_t(At<uint16_t>(texturing, 0x26)) - 8u) {   // (unsigned, as the original)
                found = false;
            } else {
                map = At<re::Ptr32<uint8_t>*>(texturing, 0x20)[8 + decal];
            }
        } else if ((flags & 0xF000000) == 0x2000000) {   // a shader map
            uint8_t* shaderMaps = At<uint8_t*>(texturing, 0x30);
            uint32_t shader = flags & 0xFFFF;
            if (shaderMaps == nullptr || shader >= At<uint16_t>(shaderMaps, 0xA)) {
                found = false;
            } else {
                map = TexturingProperty_GetShaderMap(texturing, shader);
            }
        } else {
            found = false;
        }
        if (found && map != nullptr) {
            uint16_t mapFlags = At<uint16_t>(map, 4);
            texture = At<void*>(map, 8);
            filter = (mapFlags >> 8) & 0xF;
            clamp = (mapFlags >> 12) & 3;
            if ((flags & 0xF0000000) == 0x10000000) {
                uint32_t uvSet = At<uint8_t>(map, 4);
                uint32_t value = uvSet | (Stage_GetTexCoordIndex(stage) & 0xFFFF0000);
                uint8_t save = Stage_GetTexCoordIndexSave(stage);
                StageGroup_SetStageState(At<void*>(stage, 0xC), D3DTSS_TEXCOORDINDEX, value, save);
            }
        }
    } else if (effects != nullptr && (effectFlags >> 8) >= 8 && (effectFlags >> 8) <= 14) {
        uint32_t which = effectFlags & 0xFF;
        uint8_t* effect = nullptr;
        bool use = false;
        switch (effectFlags >> 8) {
        case 8:   // (+0x18)
        case 11: {   // (+0x1C)
            effect = At<uint8_t*>(effects, (effectFlags >> 8) == 8 ? 0x18 : 0x1C);
            use = effect != nullptr && At<uint8_t>(effect, 0xC4) != 0;
            break;
        }
        case 9:
        case 10: {   // the which-th switched-on one of a list (+0x14, +0x10)
            re::Ptr32<void>* node = At<re::Ptr32<void>*>(effects, (effectFlags >> 8) == 9 ? 0x14 : 0x10);
            uint16_t count = 0;
            while (node != nullptr) {
                uint8_t* candidate = static_cast<uint8_t*>(node[1]);
                node = static_cast<re::Ptr32<void>*>(node[0]);
                if (candidate != nullptr && At<uint8_t>(candidate, 0xC4) == 1) {
                    uint16_t n = count;
                    count++;
                    if (n == which) {
                        effect = candidate;
                        use = true;
                        break;
                    }
                }
            }
            break;
        }
        default: {   // 12..14: Bully's shadowed lights' source textures (object types 5..7)
            uint8_t* light = GetDynamicEffectForObject(effects, (effectFlags >> 8) - 7, which);
            uint8_t* source = At<uint8_t*>(light, 0xD4);
            auto find = re::PtrCast<uint8_t*(__cdecl*)(void*, void*, uint32_t)>(At<void*>(source, 0x30));
            uint8_t* found = find(source, geometry, 0xFF);
            texture = At<void*>(found, 8);
            clamp = (At<uint16_t>(found, 4) >> 12) & 3;
            filter = (At<uint16_t>(found, 4) >> 8) & 0xF;
            break;
        }
        }
        if (use && effect != nullptr) {
            texture = At<void*>(effect, 0x158);
            filter = At<uint32_t>(effect, 0x15C);
            clamp = At<uint32_t>(effect, 0x160);
            if ((flags & 0xF0000000) == 0x10000000) {
                uint8_t save = Stage_GetTexCoordIndexSave(stage);
                SetupTextureEffectTransform(effect, stage, save, At<void*>(self, 0x18));
            }
        }
    }
    At<void*>(stage, 4) = texture;
    Stage_SetClampMode(stage, clamp);
    Stage_SetFilterMode(stage, filter);
}

// 0x893D70 (thiscall): NiTObjectArray<NiD3DPassPtr>::SetSize (the passes past the new size let
// go; a new array of NiPointers holding the rest)
void __fastcall NiTObjectArray_D3DPass_SetSize(uint8_t* self, re::Edx, uint32_t size) {
    if (size == At<uint16_t>(self, 8)) {
        return;
    }
    if (size < At<uint16_t>(self, 0xA)) {
        for (uint16_t i = uint16_t(size); i < At<uint16_t>(self, 0xA); i++) {
            re::Ptr32<uint8_t>& slot = At<re::Ptr32<uint8_t>*>(self, 4)[i];
            if (slot != nullptr) {
                ReleasePass(slot);
                slot = nullptr;
                At<uint16_t>(self, 0xC)--;
            }
        }
        At<uint16_t>(self, 0xA) = uint16_t(size);
    }
    re::Ptr32<uint8_t>* old = At<re::Ptr32<uint8_t>*>(self, 4);
    At<uint16_t>(self, 8) = uint16_t(size);
    if (size == 0) {
        At<re::Ptr32<uint8_t>*>(self, 4) = nullptr;
        PassArray_Delete(old);
        return;
    }
    At<re::Ptr32<void>*>(self, 4) = NiNewPointerArray(size & 0xFFFF);
    for (uint16_t i = 0; i < At<uint16_t>(self, 0xA); i++) {
        re::Ptr32<uint8_t>& slot = At<re::Ptr32<uint8_t>*>(self, 4)[i];
        if (slot != old[i]) {
            ReleasePass(slot);
            slot = old[i];
            if (slot != nullptr) {
                At<uint32_t>(slot, 0x60)++;
            }
        }
    }
    for (uint16_t i = At<uint16_t>(self, 0xA); i < At<uint16_t>(self, 8); i++) {
        re::Ptr32<uint8_t>& slot = At<re::Ptr32<uint8_t>*>(self, 4)[i];
        if (slot != nullptr) {
            ReleasePass(slot);
            slot = nullptr;
        }
    }
    PassArray_Delete(old);
}

// 0x893EC0 (thiscall): NiFragmentShaderInstanceDescriptor::GetRTTI
void* __fastcall NiFragmentShaderInstanceDescriptor_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiFragmentShaderInstanceDescriptor;
}

// 0x893ED0 (thiscall): NiFragmentShaderInstanceDescriptor's deleting destructor (0x18)
void* __fastcall NiFragmentShaderInstanceDescriptor_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    ni::sys::FixedStringRelease(&At<char*>(self, 4));
    At<uint32_t>(self, 0) = kShaderInstanceDescriptorVtable;
    if (flags & 1) {
        NiDelete(self, 0x18);
    }
    return self;
}

// 0x893F30 (thiscall): NiDX9FragmentShader's destructor
void __fastcall NiDX9FragmentShader_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kFragmentShaderVtable;
    ReleaseDescriptorString(self);
    At<uint32_t>(self, 0x58) = kShaderInstanceDescriptorVtable;
    D3DShader_Destruct(self);
}

// 0x893F80 (thiscall): NiDX9FragmentShader::GetRTTI
void* __fastcall NiDX9FragmentShader_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiDX9FragmentShader;
}

// 0x893F90 (thiscall): NiDX9FragmentShader::UpdatePipeline (every pass's stages updated from the
// texturing property and the dynamic effects, then NiShader's)
uint32_t __fastcall NiDX9FragmentShader_UpdatePipeline(uint8_t* self, re::Edx, void* geometry, void* skin,
                                                       void* bufferData, uint8_t* state, uint8_t* effects,
                                                       const NiTransform* world, const NiBound* bound) {
    for (uint32_t i = 0; i < At<uint16_t>(self, 0x4E); i++) {
        uint8_t* pass = Pass(self, i);
        if (pass == nullptr) {
            continue;
        }
        for (uint32_t j = 0; j < At<uint32_t>(pass, 0x18); j++) {
            // (NiTObjectArray::GetAt: past its size a null pointer)
            uint8_t* stage = j < At<uint16_t>(pass, 0x2A) ? At<re::Ptr32<uint8_t>*>(pass, 0x24)[j] : nullptr;
            if (stage != nullptr) {
                FragmentShader_UpdateTextureStage(self, geometry, stage, At<uint8_t*>(state, 0x28), effects, i);
            }
        }
    }
    return D3DShader_UpdatePipeline(self, geometry, skin, bufferData, state, effects, world, bound);
}

// 0x894070 (thiscall): NiDX9FragmentShader's deleting destructor
void* __fastcall NiDX9FragmentShader_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = kFragmentShaderVtable;
    ReleaseDescriptorString(self);
    At<uint32_t>(self, 0x58) = kShaderInstanceDescriptorVtable;
    D3DShader_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x70);
    }
    return self;
}

// 0x8940E0 (thiscall): NiDX9FragmentShader::AppendRenderPass (a new pass from the pool at the end
// of the passes; its index returned)
uint8_t __fastcall NiDX9FragmentShader_AppendRenderPass(uint8_t* self, re::Edx, uint32_t* index) {
    re::Ptr32<uint8_t> pass = nullptr;   // (the original's NiD3DPassPtr is its own stack space)
    Pass_CreateNewPass(&pass);
    uint32_t count = At<uint32_t>(self, 0x3C);
    if (count >= At<uint16_t>(self, 0x4C)) {
        PassArray_SetSize(self + 0x44, At<uint16_t>(self, 0x52) + count);
    }
    PassArray_SetAt(self + 0x44, count, &pass);
    *index = At<uint32_t>(self, 0x3C);
    At<uint32_t>(self, 0x3C)++;
    ReleasePass(pass);
    return 1;
}

// 0x8941D0 (thiscall): NiDX9FragmentShader's constructor (its material descriptor copied)
void* __fastcall NiDX9FragmentShader_Construct(uint8_t* self, re::Edx, const void* descriptor) {
    D3DShader_Construct(self);
    At<uint32_t>(self, 0) = kFragmentShaderVtable;
    At<uint32_t>(self, 0x58) = kInstanceDescriptorVtable;
    MaterialDescriptor_Construct(self + 0x5C);
    MaterialDescriptor_Assign(self + 0x5C, descriptor);
    return self;
}

// 0x894200 (thiscall): NiDX9FragmentShader::CreateShaderConstantMap (the pass's vertex or pixel
// constant map, made when it has none; the map index isn't used)
void* __fastcall NiDX9FragmentShader_CreateShaderConstantMap(uint8_t* self, re::Edx, uint32_t index, uint32_t type,
                                                             uint32_t) {
    if (index >= At<uint32_t>(self, 0x3C)) {
        return nullptr;
    }
    uint8_t* pass = Pass(self, index);
    if (pass == nullptr) {
        return nullptr;
    }
    void* map = nullptr;
    if (type == 0) {
        map = At<void*>(pass, 0x48);
        if (map == nullptr) {
            void* memory = NiNew(0x44);
            if (memory != nullptr) {
                map = ConstantMap_Construct(memory, ni::dx9::ms_pkRenderer, 0);
                Pass_SetVertexConstantMap(pass, static_cast<NiRefObject*>(map));
                return map;
            }
            Pass_SetVertexConstantMap(pass, nullptr);
            return nullptr;
        }
    } else if (type == 1) {
        map = At<void*>(pass, 0x34);
        if (map == nullptr) {
            void* memory = NiNew(0x44);
            if (memory != nullptr) {
                map = ConstantMap_Construct(memory, ni::dx9::ms_pkRenderer, 1);
                Pass_SetPixelConstantMap(pass, static_cast<NiRefObject*>(map));
                return map;
            }
            Pass_SetPixelConstantMap(pass, nullptr);
            return nullptr;
        }
    }
    return map;
}

#define C40E_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C40E_REVERSED("NiDX9FragmentShader::EnsureInitialized", 0x00893140, NiDX9FragmentShader_EnsureInitialized);
C40E_REVERSED("NiShader::NiShaderInstanceDescriptor::GetRTTI", 0x00893210, NiShaderInstanceDescriptor_GetRTTI);
C40E_REVERSED("NiShader::NiShaderInstanceDescriptor::DeletingDestructor", 0x00893220,
              NiShaderInstanceDescriptor_DeletingDestructor);
C40E_REVERSED("NiDX9FragmentShader::GetShadowSamplerIndex", 0x00893250, NiDX9FragmentShader_GetShadowSamplerIndex);
C40E_REVERSED("NiD3DTextureStage::GetTexCoordIndex", 0x008932A0, NiD3DTextureStage_GetTexCoordIndex);
C40E_REVERSED("NiD3DTextureStage::GetTexCoordIndexSave", 0x008932E0, NiD3DTextureStage_GetTexCoordIndexSave);
C40E_REVERSED("NiD3DPass::SetRenderState", 0x00893310, NiD3DPass_SetRenderState);
C40E_REVERSED("NiDX9FragmentShader::SetupTextureEffectTransform", 0x00893340,
              NiDX9FragmentShader_SetupTextureEffectTransform);
C40E_REVERSED("NiDX9FragmentShader::CreateShaderDeclaration", 0x00893380, NiDX9FragmentShader_CreateShaderDeclaration);
C40E_REVERSED("NiDX9FragmentShader::SetAlphaOverride", 0x008933B0, NiDX9FragmentShader_SetAlphaOverride);
C40E_REVERSED("NiDX9FragmentShader::AppendTextureSampler", 0x00893490, NiDX9FragmentShader_AppendTextureSampler);
C40E_REVERSED("NiD3DPass::SetPixelConstantMap", 0x00893770, NiD3DPass_SetPixelConstantMap);
C40E_REVERSED("NiD3DPass::SetPixelShader", 0x008937C0, NiD3DPass_SetPixelShader);
C40E_REVERSED("NiD3DPass::SetVertexConstantMap", 0x00893810, NiD3DPass_SetVertexConstantMap);
C40E_REVERSED("NiD3DPass::SetVertexShader", 0x00893860, NiD3DPass_SetVertexShader);
C40E_REVERSED("NiDX9FragmentShader::SetGPUProgram", 0x008938B0, NiDX9FragmentShader_SetGPUProgram);
C40E_REVERSED("NiDX9FragmentShader::SetupTransformations", 0x00893910, NiDX9FragmentShader_SetupTransformations);
C40E_REVERSED("NiDX9FragmentShader::UpdateTextureStage", 0x008939C0, NiDX9FragmentShader_UpdateTextureStage);
C40E_REVERSED("NiTObjectArray<NiD3DPassPtr>::SetSize", 0x00893D70, NiTObjectArray_D3DPass_SetSize);
C40E_REVERSED("NiFragmentShaderInstanceDescriptor::GetRTTI", 0x00893EC0, NiFragmentShaderInstanceDescriptor_GetRTTI);
C40E_REVERSED("NiFragmentShaderInstanceDescriptor::DeletingDestructor", 0x00893ED0,
              NiFragmentShaderInstanceDescriptor_DeletingDestructor);
C40E_REVERSED("NiDX9FragmentShader::~NiDX9FragmentShader", 0x00893F30, NiDX9FragmentShader_Destruct);
C40E_REVERSED("NiDX9FragmentShader::GetRTTI", 0x00893F80, NiDX9FragmentShader_GetRTTI);
C40E_REVERSED("NiDX9FragmentShader::UpdatePipeline", 0x00893F90, NiDX9FragmentShader_UpdatePipeline);
C40E_REVERSED("NiDX9FragmentShader::DeletingDestructor", 0x00894070, NiDX9FragmentShader_DeletingDestructor);
C40E_REVERSED("NiDX9FragmentShader::AppendRenderPass", 0x008940E0, NiDX9FragmentShader_AppendRenderPass);
C40E_REVERSED("NiDX9FragmentShader::NiDX9FragmentShader", 0x008941D0, NiDX9FragmentShader_Construct);
C40E_REVERSED("NiDX9FragmentShader::CreateShaderConstantMap", 0x00894200, NiDX9FragmentShader_CreateShaderConstantMap);

} // namespace ni::c40
