// NiD3DXEffectTechnique (vtable 0x950A08, 0x48 bytes) and NiD3DXEffectTechniqueSet (vtable 0x950B08,
// 0x3C), src/gamebryo/NiD3DXEffect.h: one implementation of a shader name in an effect (its
// requirements, the parameters it uses, which passes have vertex / pixel shaders, the
// NiD3DXEffectShader made for it once it validates) and the implementations of one name (the one
// in use, the NiShaderDesc the library lists).

#include "gamebryo/NiD3DXEffect.h"
#include "reversed.h"
#include "sysabi.h"

namespace ni {
namespace {

using namespace ni::d3dx;

constexpr uintptr_t RefObject_vtable = 0x008FEFD4;
constexpr uintptr_t Technique_vtable = 0x00950A08;
constexpr uintptr_t TechniqueSet_vtable = 0x00950B08;
constexpr uintptr_t PointerList_vtable = 0x00952E10;
constexpr uintptr_t PointerListBase_vtable = 0x0099C748;
constexpr uintptr_t TechniqueArray_vtable = 0x009893C0;

GAME_VAR(long, NiRefObject_ms_uiObjects, 0x020E2168);

GAME_FN(void, __cdecl, NiDelete_, 0x00752540, void* p, uint32_t size);
GAME_FN(void*, __cdecl, NiNew_, 0x00752460, uint32_t size);
GAME_FN(void*, __cdecl, NiMalloc_, 0x00752300, uint32_t size);
GAME_FN(void, __cdecl, NiFree_, 0x00752380, void* p);
GAME_FN(void, __cdecl, OperatorDelete, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, NiArrayFree, 0x00836F80, void* p);
GAME_FN(void, __cdecl, NiD3DUtility_SetString, 0x00888530, re::Ptr32<char>* to, uintptr_t size, const char* from);
GAME_FN(void, __cdecl, NiD3DRenderer_Error, 0x00888620, const char* format, ...);
GAME_FN(void, __thiscall, NiTPointerList_RemoveAll, 0x0075DB60, PointerList* list);
// NiTObjectArray<NiD3DXEffectTechniquePtr>
GAME_FN(void, __thiscall, TechniqueArray_RemoveAll, 0x0040E7A0, NiTArray<NiD3DXEffectTechnique*>* self);
GAME_FN(void, __thiscall, TechniqueArray_SetSize, 0x00856170, NiTArray<NiD3DXEffectTechnique*>* self, uint32_t size);
GAME_FN(void, __thiscall, TechniqueArray_SetAt, 0x0040E800, NiTArray<NiD3DXEffectTechnique*>* self, uint32_t index,
        const re::Ptr32<NiD3DXEffectTechnique>* technique);
// NiShaderRequirementDesc, NiShaderDesc, NiShaderAttributeDesc (another chunk's)
GAME_FN(void*, __thiscall, RequirementDesc_Construct, 0x0077E870, void* self);
GAME_FN(uint32_t, __thiscall, RequirementDesc_GetBonesPerPartition, 0x008A2F70, void* self);
GAME_FN(uint32_t, __thiscall, RequirementDesc_GetBinormalTangentMethod, 0x008A30A0, void* self);
GAME_FN(uint8_t, __thiscall, RequirementDesc_GetFlag3E, 0x0077E930, void* self);
GAME_FN(uint8_t, __thiscall, RequirementDesc_GetFlag3F, 0x0077E940, void* self);
GAME_FN(NiRefObject*, __thiscall, ShaderDesc_Construct, 0x007844C0, void* self);
GAME_FN(void, __thiscall, ShaderDesc_SetTechniqueCount, 0x007842D0, NiRefObject* self, uint32_t count);
GAME_FN(void, __thiscall, ShaderDesc_AddAttribute, 0x00784470, NiRefObject* self, void* attribute);
GAME_FN(void, __thiscall, ShaderDesc_SetTechnique, 0x00784540, NiRefObject* self, uint32_t index, void* requirements);
GAME_FN(void*, __thiscall, AttributeDesc_Construct, 0x00784580, void* self);
// the renderer's
GAME_FN(uint8_t, __thiscall, RenderState_GetSoftwareVertexProcessing, 0x00882550, void* renderState);
GAME_FN(void, __thiscall, RenderState_SetSoftwareVertexProcessing, 0x00883480, void* renderState, int software);
GAME_FN(void, __thiscall, Shader_SetName, 0x0077C3A0, void* shader, const re::Ptr32<const char>* name);
GAME_FN(void, __thiscall, Shader_SetImplementation, 0x008C00F0, void* shader, uint32_t implementation);
// this library's, called where modes apply
GAME_FN(NiD3DXEffectShader*, __thiscall, EffectShader_Construct, 0x00747150, void* self);
GAME_FN(NiD3DXEffectStateManager*, __cdecl, StateManager_GetInstance, 0x0074D710, void* renderer);
GAME_FN(void, __thiscall, Technique_Destruct, 0x00749E70, NiD3DXEffectTechnique* self);
GAME_FN(void, __thiscall, Technique_DestroyRendererData, 0x00749F40, NiD3DXEffectTechnique* self);
GAME_FN(NiD3DXEffectShader*, __thiscall, Technique_GetD3DShader, 0x00749FA0, NiD3DXEffectTechnique* self,
        void* renderer);
GAME_FN(void*, __thiscall, Technique_GetRequirements, 0x00749E60, NiD3DXEffectTechnique* self);
GAME_FN(NiD3DXEffectShader*, __thiscall, TechniqueSet_GetBestD3DShader, 0x0074DC30, NiD3DXEffectTechniqueSet* self,
        void* renderer);
GAME_FN(void, __thiscall, TechniqueSet_Destruct, 0x0074DD10, NiD3DXEffectTechniqueSet* self);
GAME_FN(void*, __thiscall, AttributeTable_GetFirst, 0x00751140, NiD3DXEffectAttributeTable* self);
GAME_FN(void*, __thiscall, AttributeTable_GetNext, 0x00751160, NiD3DXEffectAttributeTable* self);

// whether every / any pass has a flag
inline uint8_t AllPasses(const uint8_t* flags, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        if (flags[i] == 0) {
            return 0;
        }
    }
    return 1;
}
inline uint8_t AnyPass(const uint8_t* flags, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        if (flags[i] == 1) {
            return 1;
        }
    }
    return 0;
}

} // namespace

// ---- NiD3DXEffectTechnique

// 0x749BD0: Initialize (from the parser's description: its names, the requirements (an
// NiShaderRequirementDesc: the pixel and vertex shader versions, the user version, the bones per
// partition, the binormal-tangent method), the implementation and the state switches)
void __fastcall NiD3DXEffectTechnique_Initialize(NiD3DXEffectTechnique* self, re::Edx, TechniqueDesc* desc) {
    NiD3DUtility_SetString(&self->techniqueName, 0, desc->techniqueName);
    void* memory = NiNew_(0x44);
    void* requirements = memory != nullptr ? RequirementDesc_Construct(memory) : nullptr;
    self->requirements = requirements;
    Virtual<void>(requirements, 4, desc->shaderName);                 // SetName
    Virtual<void>(self->requirements, 5, desc->description);          // SetDescription
    Virtual<void>(self->requirements, 20, desc->pixelShaderVersion);  // (the +0x20 pair)
    Virtual<void>(self->requirements, 18, desc->vertexShaderVersion); // (the +0x10 pair)
    Virtual<void>(self->requirements, 21, desc->userVersion);         // (the +0x28 pair)
    Virtual<void>(self->requirements, 24, 1);
    Virtual<void>(self->requirements, 26, 1);
    Virtual<void>(self->requirements, 28, desc->bonesPerPartition);
    Virtual<void>(self->requirements, 29, desc->nbtMethod);
    self->implementation = desc->implementation;
    self->usesNiRenderState = desc->usesNiRenderState;
    self->usesNiLightState = desc->usesNiLightState;
    self->softwareVPRequired = desc->softwareVPRequired;
    self->softwareVPFallback = desc->softwareVPFallback;
}

// 0x749CA0..0x749D30: whether all / any passes have a vertex / pixel shader
uint8_t __fastcall NiD3DXEffectTechnique_GetVSPresentAllPasses(NiD3DXEffectTechnique* self, re::Edx) {
    return AllPasses(self->passHasVertexShader, self->passCount);
}
uint8_t __fastcall NiD3DXEffectTechnique_GetVSPresentAnyPass(NiD3DXEffectTechnique* self, re::Edx) {
    return AnyPass(self->passHasVertexShader, self->passCount);
}
uint8_t __fastcall NiD3DXEffectTechnique_GetPSPresentAllPasses(NiD3DXEffectTechnique* self, re::Edx) {
    return AllPasses(self->passHasPixelShader, self->passCount);
}
uint8_t __fastcall NiD3DXEffectTechnique_GetPSPresentAnyPass(NiD3DXEffectTechnique* self, re::Edx) {
    return AnyPass(self->passHasPixelShader, self->passCount);
}

// 0x749D60: GetVSPresent (one pass's)
uint8_t __fastcall NiD3DXEffectTechnique_GetVSPresent(NiD3DXEffectTechnique* self, re::Edx, uint32_t pass) {
    if (self->passHasVertexShader != nullptr && pass < self->passCount) {
        return self->passHasVertexShader[pass];
    }
    return 0;
}

// 0x749D80: the technique's handle (by name) and which passes have shaders, read from the effect
// when the handle changed. (The flag arrays, NiMalloc'd, are freed with operator delete when they
// grow; the pass count only grows)
void __fastcall NiD3DXEffectTechnique_GetPassDescs(NiD3DXEffectTechnique* self, re::Edx, void* effect) {
    const char* handle = Com<const char*>(effect, 13, self->techniqueName);   // GetTechniqueByName
    if (handle == self->handle) {
        return;
    }
    self->handle = handle;
    D3DXTechniqueDesc technique;
    Com<long>(effect, kGetTechniqueDesc, handle, &technique);
    if (self->passCount < technique.passes) {
        OperatorDelete(self->passHasVertexShader);
        self->passHasVertexShader = static_cast<uint8_t*>(NiMalloc_(technique.passes));
        OperatorDelete(self->passHasPixelShader);
        self->passHasPixelShader = static_cast<uint8_t*>(NiMalloc_(technique.passes));
        self->passCount = technique.passes;
    }
    for (uint32_t i = 0; i < self->passCount; i++) {
        const char* pass = Com<const char*>(effect, kGetPass, self->handle, i);
        D3DXPassDesc desc;
        long hr = Com<long>(effect, kGetPassDesc, pass, &desc);
        self->passHasVertexShader[i] = (hr >= 0 && desc.vertexShaderFunction != nullptr) ? 1 : 0;
        self->passHasPixelShader[i] = (hr >= 0 && desc.pixelShaderFunction != nullptr) ? 1 : 0;
    }
}

// 0x749E60: GetRequirements (4 bytes: not hooked)
void* __fastcall NiD3DXEffectTechnique_GetRequirements(NiD3DXEffectTechnique* self, re::Edx) {
    return self->requirements;
}
RE_UNHOOKED_FN("NiD3DXEffectTechnique::GetRequirements", 0x00749E60, &NiD3DXEffectTechnique_GetRequirements);

// 0x749E70: the destructor (the requirements aren't freed)
void __fastcall NiD3DXEffectTechnique_Destruct(NiD3DXEffectTechnique* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(Technique_vtable);
    NiD3DRenderer_Error("Killing NiD3DXEffectTechnique %d\n", self->implementation);
    NiFree_(self->techniqueName);
    NiFree_(self->passHasVertexShader);
    NiFree_(self->passHasPixelShader);
    NiTPointerList_RemoveAll(&self->parameters);
    self->parameters.vtable = reinterpret_cast<void*>(PointerList_vtable);
    NiTPointerList_RemoveAll(&self->parameters);
    self->parameters.vtable = reinterpret_cast<void*>(PointerListBase_vtable);
    ReleaseRef(self->shader);
    self->vtable = reinterpret_cast<void*>(RefObject_vtable);
    Decrement(&NiRefObject_ms_uiObjects);
}

// 0x749F40: DestroyRendererData: the handle forgotten, the shader's renderer data destroyed (slot 30)
void __fastcall NiD3DXEffectTechnique_DestroyRendererData(NiD3DXEffectTechnique* self, re::Edx) {
    self->handle = nullptr;
    if (self->shader != nullptr) {
        Virtual<void>(self->shader, 30);
    }
}

// 0x749F60: GetNextParameter
NiD3DXEffectParameter* __fastcall NiD3DXEffectTechnique_GetNextParameter(NiD3DXEffectTechnique* self, re::Edx) {
    if (self->parameterIterator == nullptr) {
        return nullptr;
    }
    ListNode* node = self->parameterIterator;
    self->parameterIterator = node->next;
    return static_cast<NiD3DXEffectParameter*>(node->element);
}

// 0x749F80: the deleting destructor (slot 0)
NiD3DXEffectTechnique* __fastcall NiD3DXEffectTechnique_DeletingDestructor(NiD3DXEffectTechnique* self, re::Edx,
                                                                           int flags) {
    Technique_Destruct(self);
    if (flags & 1) {
        NiDelete_(self, sizeof(NiD3DXEffectTechnique));
    }
    return self;
}

// 0x749FA0: GetD3DShader: made once the technique validates on the device (software vertex
// processing switched as it requires or falls back to, then put back), with the skinning
// parameter's register count and layout. (The skinning layout is the last parameter looked at's,
// whether or not it matched)
NiD3DXEffectShader* __fastcall NiD3DXEffectTechnique_GetD3DShader(NiD3DXEffectTechnique* self, re::Edx,
                                                                  uint8_t* renderer) {
    void* effect = self->file->rendererDataCreated ? self->file->effect : nullptr;
    if (self->shader != nullptr) {
        return self->shader;
    }
    if (self->handle == nullptr) {
        return nullptr;
    }
    uint32_t bones = RequirementDesc_GetBonesPerPartition(self->requirements);
    uint32_t transpose = 0;
    uint32_t registers = 4;
    if (bones != 0 && self->parameters.head != nullptr) {
        NiD3DXEffectParameter* parameter;
        uint32_t mapping = 0;
        ListNode* node = self->parameters.head;
        do {
            parameter = static_cast<NiD3DXEffectParameter*>(node->element);
            node = node->next;
            if (parameter != nullptr) {
                mapping = parameter->predefined;
                if (mapping - 0x21 <= 3 && bones <= parameter->maxCount &&
                    Com<long>(effect, kIsParameterUsed, parameter->handle, self->handle) != 0) {
                    break;
                }
            }
        } while (node != nullptr);
        if (parameter != nullptr) {
            if (mapping == 0x21) {
                transpose = 1;
                registers = 3;
            } else if (mapping == 0x22) {
                transpose = 1;
                registers = 4;
            } else if (mapping == 0x23) {
                transpose = 0;
                registers = 3;
            } else if (mapping == 0x24) {
                transpose = 0;
                registers = 4;
            }
        }
    }
    uint8_t hardware = At<uint8_t>(renderer, 0x3F0);
    uint8_t mixed = At<uint8_t>(renderer, 0x3F1);
    void* renderState = At<void*>(renderer, 0x6B4);
    uint8_t software = RenderState_GetSoftwareVertexProcessing(renderState);
    uint8_t useSoftware = software;
    bool changed = false;
    if (self->softwareVPRequired) {
        if (!hardware) {
            return nullptr;
        }
        if (!software) {
            if (!mixed) {
                return nullptr;
            }
            RenderState_SetSoftwareVertexProcessing(renderState, 1);
            useSoftware = 1;
            changed = true;
        }
    } else if (self->softwareVPFallback && software) {
        if (!mixed) {
            return nullptr;
        }
        RenderState_SetSoftwareVertexProcessing(renderState, 0);
        useSoftware = 0;
        changed = true;
    }
    long hr = Com<long>(effect, kValidateTechnique, self->handle);
    if (hr < 0 && self->softwareVPFallback && mixed) {
        RenderState_SetSoftwareVertexProcessing(renderState, 1);
        useSoftware = 1;
        changed = software == 0;
        hr = Com<long>(effect, kValidateTechnique, self->handle);
    }
    if (changed) {
        RenderState_SetSoftwareVertexProcessing(renderState, software);
    }
    if (hr < 0) {
        return self->shader;
    }
    // (ID3DXEffectStateManager: IUnknown's 3 and its 18 methods)
    Com<long>(effect, kSetStateManager, re::NativeInterface(StateManager_GetInstance(renderer), 21));
    void* memory = NiNew_(sizeof(NiD3DXEffectShader));
    NiD3DXEffectShader* shader = memory != nullptr ? EffectShader_Construct(memory) : nullptr;
    AssignRef(self->shader, shader);
    self->shader->technique = self;
    const char* name = Virtual<const char*>(self->requirements, 2);   // GetName
    re::Ptr32<char> fixed = name != nullptr ? ni::sys::GlobalStringTable_AddString(name) : static_cast<char*>(ni::sys::kNullFixedString);
    Shader_SetName(self->shader, re::PtrCast<const re::Ptr32<const char>*>(&fixed));
    ni::sys::FixedStringRelease(&fixed);
    Shader_SetImplementation(self->shader, self->implementation);
    self->shader->techniqueIndex = self->implementation;
    Virtual<void>(self->shader, 21, int(self->usesNiRenderState));   // SetUsesNiRenderState
    self->shader->usesNiLightState = self->usesNiLightState;
    self->shader->softwareVertexProcessing = useSoftware;
    if (bones != 0) {
        self->shader->boneMatrixRegisters = registers;
        self->shader->transposeBones = transpose;
    }
    return self->shader;
}

// 0x74A2D0: AddParameter (at the tail); a time parameter (0x52..0x56) wants a time controller
void __fastcall NiD3DXEffectTechnique_AddParameter(NiD3DXEffectTechnique* self, re::Edx,
                                                   NiD3DXEffectParameter* parameter) {
    PointerList* list = &self->parameters;
    ListNode* node = Virtual<ListNode*>(list, 1);
    node->element = parameter;
    node->next = nullptr;
    node->prev = list->tail;
    if (list->tail != nullptr) {
        list->tail->next = node;
    } else {
        list->head = node;
    }
    list->count += 1;
    list->tail = node;
    uint32_t mapping = parameter->predefined;
    if (mapping == 0x52 || mapping == 0x53 || mapping == 0x54 || mapping == 0x55 || mapping == 0x56) {
        self->usesTime = 1;
    }
}

// 0x74A340: GetFirstParameter
NiD3DXEffectParameter* __fastcall NiD3DXEffectTechnique_GetFirstParameter(NiD3DXEffectTechnique* self, re::Edx) {
    ListNode* node = self->parameters.head;
    self->parameterIterator = node;
    if (node == nullptr) {
        return nullptr;
    }
    self->parameterIterator = node->next;
    return static_cast<NiD3DXEffectParameter*>(node->element);
}

// 0x74A360: the constructor (the requirements left unset)
NiD3DXEffectTechnique* __fastcall NiD3DXEffectTechnique_Construct(NiD3DXEffectTechnique* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(RefObject_vtable);
    self->refCount = 0;
    Increment(&NiRefObject_ms_uiObjects);
    self->techniqueName = nullptr;
    self->implementation = 0;
    self->handle = nullptr;
    self->file = nullptr;
    self->vtable = reinterpret_cast<void*>(Technique_vtable);
    self->shader = nullptr;
    self->parameters.count = 0;
    self->parameters.head = nullptr;
    self->parameters.tail = nullptr;
    self->parameters.vtable = reinterpret_cast<void*>(PointerList_vtable);
    self->parameterIterator = nullptr;
    self->passCount = 0;
    self->passHasVertexShader = nullptr;
    self->passHasPixelShader = nullptr;
    self->usesTime = 0;
    self->usesNiRenderState = 0;
    self->usesNiLightState = 0;
    self->softwareVPRequired = 0;
    self->softwareVPFallback = 0;
    return self;
}

// ---- NiD3DXEffectTechniqueSet

// 0x74D9B0: SetName (a copy)
void __fastcall NiD3DXEffectTechniqueSet_SetName(NiD3DXEffectTechniqueSet* self, re::Edx, const char* name) {
    NiD3DUtility_SetString(&self->name, 0, name);
}

// 0x74D9D0: GetShaderDesc (made once): the name, the first technique's description, the attributes
// the parser found, and each technique's requirements folded in and listed
void* __fastcall NiD3DXEffectTechniqueSet_GetShaderDesc(NiD3DXEffectTechniqueSet* self, re::Edx,
                                                        NiD3DXEffectAttributeTable* table) {
    if (self->shaderDesc != nullptr) {
        return self->shaderDesc;
    }
    void* memory = NiNew_(0x6C);
    NiRefObject* created = memory != nullptr ? ShaderDesc_Construct(memory) : nullptr;
    AssignRef(self->shaderDesc, created);
    Virtual<void>(self->shaderDesc, 4, self->name);   // SetName
    if (self->techniques.base[0] != nullptr) {
        const char* description = Virtual<const char*>(self->techniques.base[0]->requirements, 3);
        Virtual<void>(self->shaderDesc, 5, description);   // SetDescription
    }
    ShaderDesc_SetTechniqueCount(self->shaderDesc, self->techniques.esize);
    for (void* attribute = AttributeTable_GetFirst(table); attribute != nullptr;
         attribute = AttributeTable_GetNext(table)) {
        void* attributeMemory = NiNew_(0xE4);
        void* copy = attributeMemory != nullptr ? AttributeDesc_Construct(attributeMemory) : nullptr;
        Virtual<void>(copy, 6, attribute);   // (copied from the table's)
        ShaderDesc_AddAttribute(self->shaderDesc, copy);
    }
    for (uint32_t i = 0; i < self->techniques.size; i++) {
        NiD3DXEffectTechnique* technique = self->techniques.base[i];
        if (technique == nullptr) {
            continue;
        }
        void* requirements = Technique_GetRequirements(technique);
        Virtual<void>(self->shaderDesc, 20, Virtual<uint32_t>(requirements, 10));
        Virtual<void>(self->shaderDesc, 20, Virtual<uint32_t>(requirements, 11));
        Virtual<void>(self->shaderDesc, 18, Virtual<uint32_t>(requirements, 6));
        Virtual<void>(self->shaderDesc, 18, Virtual<uint32_t>(requirements, 7));
        Virtual<void>(self->shaderDesc, 21, Virtual<uint32_t>(requirements, 12));
        Virtual<void>(self->shaderDesc, 21, Virtual<uint32_t>(requirements, 13));
        Virtual<void>(self->shaderDesc, 22, Virtual<uint32_t>(requirements, 14));
        Virtual<void>(self->shaderDesc, 28, RequirementDesc_GetBonesPerPartition(requirements));
        Virtual<void>(self->shaderDesc, 29, RequirementDesc_GetBinormalTangentMethod(requirements));
        Virtual<void>(self->shaderDesc, 31, int(RequirementDesc_GetFlag3E(requirements)));
        Virtual<void>(self->shaderDesc, 32, int(RequirementDesc_GetFlag3F(requirements)));
        ShaderDesc_SetTechnique(self->shaderDesc, i, requirements);
    }
    return self->shaderDesc;
}

// 0x74DC30: GetD3DShader (the best: the one in use, else the first that makes a shader, which then
// has no implementation number)
NiD3DXEffectShader* __fastcall NiD3DXEffectTechniqueSet_GetBestD3DShader(NiD3DXEffectTechniqueSet* self, re::Edx,
                                                                         void* renderer) {
    if (self->best != nullptr) {
        return Technique_GetD3DShader(self->best, renderer);
    }
    for (uint32_t i = 0; i < self->techniques.size; i++) {
        NiD3DXEffectTechnique* technique = self->techniques.base[i];
        if (technique == nullptr) {
            continue;
        }
        NiD3DXEffectShader* shader = Technique_GetD3DShader(technique, renderer);
        if (shader != nullptr) {
            self->best = technique;
            Shader_SetImplementation(shader, 0xFFFFFFFF);
            return shader;
        }
    }
    return nullptr;
}

// 0x74DCA0: ReleaseShaders (every technique's renderer data destroyed)
void __fastcall NiD3DXEffectTechniqueSet_ReleaseShaders(NiD3DXEffectTechniqueSet* self, re::Edx) {
    for (uint32_t i = 0; i < self->techniques.size; i++) {
        if (self->techniques.base[i] != nullptr) {
            Technique_DestroyRendererData(self->techniques.base[i]);
        }
    }
}

// 0x74DCD0: GetD3DShader (the implementation's, else the best)
NiD3DXEffectShader* __fastcall NiD3DXEffectTechniqueSet_GetD3DShader(NiD3DXEffectTechniqueSet* self, re::Edx,
                                                                     void* renderer, uint32_t implementation) {
    if (implementation < self->techniques.size && self->techniques.base[implementation] != nullptr) {
        return Technique_GetD3DShader(self->techniques.base[implementation], renderer);
    }
    return TechniqueSet_GetBestD3DShader(self, renderer);
}

// 0x74DD10: the destructor
void __fastcall NiD3DXEffectTechniqueSet_Destruct(NiD3DXEffectTechniqueSet* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(TechniqueSet_vtable);
    NiD3DRenderer_Error("Killing NiD3DXEffectTechniqueSet %s\n", self->name);
    TechniqueArray_RemoveAll(&self->techniques);
    NiFree_(self->name);
    ReleaseRef(self->shaderDesc);
    self->techniques.vtable = reinterpret_cast<void*>(TechniqueArray_vtable);
    NiArrayFree(self->techniques.base);
    self->vtable = reinterpret_cast<void*>(RefObject_vtable);
    Decrement(&NiRefObject_ms_uiObjects);
}

// 0x74DD90: the constructor (no techniques: the array grows by 1)
NiD3DXEffectTechniqueSet* __fastcall NiD3DXEffectTechniqueSet_Construct(NiD3DXEffectTechniqueSet* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(RefObject_vtable);
    self->refCount = 0;
    Increment(&NiRefObject_ms_uiObjects);
    self->name = nullptr;
    self->vtable = reinterpret_cast<void*>(TechniqueSet_vtable);
    self->techniques.maxSize = 0;
    self->techniques.size = 0;
    self->techniques.esize = 0;
    self->techniques.base = nullptr;
    self->techniques.growBy = 1;
    self->techniques.vtable = reinterpret_cast<void*>(TechniqueArray_vtable);
    self->best = nullptr;
    self->shaderDesc = nullptr;
    return self;
}

// 0x74DDE0: the deleting destructor (slot 0)
NiD3DXEffectTechniqueSet* __fastcall NiD3DXEffectTechniqueSet_DeletingDestructor(NiD3DXEffectTechniqueSet* self,
                                                                                 re::Edx, int flags) {
    TechniqueSet_Destruct(self);
    if (flags & 1) {
        NiDelete_(self, sizeof(NiD3DXEffectTechniqueSet));
    }
    return self;
}

// 0x74DE00: AddTechnique (at its implementation's index; the reference passed by value: a null
// one would crash on the release)
void __fastcall NiD3DXEffectTechniqueSet_AddTechnique(NiD3DXEffectTechniqueSet* self, re::Edx,
                                                      re::Ptr32<NiD3DXEffectTechnique> technique) {
    AddRefRef(technique);
    uint32_t index = technique->implementation;
    if (index >= self->techniques.maxSize) {
        TechniqueArray_SetSize(&self->techniques, self->techniques.growBy + index);
    }
    TechniqueArray_SetAt(&self->techniques, index, &technique);
    if (Decrement(&technique->refCount) == 0) {
        Virtual<void>(technique, 1);
    }
}

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NID3DXTECH_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(Original_, __LINE__);                                                 \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NID3DXTECH_REVERSED("NiD3DXEffectTechnique::Initialize", 0x00749BD0, &ni::NiD3DXEffectTechnique_Initialize);
NID3DXTECH_REVERSED("NiD3DXEffectTechnique::GetVSPresentAllPasses", 0x00749CA0, &ni::NiD3DXEffectTechnique_GetVSPresentAllPasses);
NID3DXTECH_REVERSED("NiD3DXEffectTechnique::GetVSPresentAnyPass", 0x00749CD0, &ni::NiD3DXEffectTechnique_GetVSPresentAnyPass);
NID3DXTECH_REVERSED("NiD3DXEffectTechnique::GetPSPresentAllPasses", 0x00749D00, &ni::NiD3DXEffectTechnique_GetPSPresentAllPasses);
NID3DXTECH_REVERSED("NiD3DXEffectTechnique::GetPSPresentAnyPass", 0x00749D30, &ni::NiD3DXEffectTechnique_GetPSPresentAnyPass);
NID3DXTECH_REVERSED("NiD3DXEffectTechnique::GetVSPresent", 0x00749D60, &ni::NiD3DXEffectTechnique_GetVSPresent);
NID3DXTECH_REVERSED("NiD3DXEffectTechnique::GetPassDescs", 0x00749D80, &ni::NiD3DXEffectTechnique_GetPassDescs);
NID3DXTECH_REVERSED("NiD3DXEffectTechnique::~NiD3DXEffectTechnique", 0x00749E70, &ni::NiD3DXEffectTechnique_Destruct);
NID3DXTECH_REVERSED("NiD3DXEffectTechnique::DestroyRendererData", 0x00749F40, &ni::NiD3DXEffectTechnique_DestroyRendererData);
NID3DXTECH_REVERSED("NiD3DXEffectTechnique::GetNextParameter", 0x00749F60, &ni::NiD3DXEffectTechnique_GetNextParameter);
NID3DXTECH_REVERSED("NiD3DXEffectTechnique::DeletingDestructor", 0x00749F80, &ni::NiD3DXEffectTechnique_DeletingDestructor);
NID3DXTECH_REVERSED("NiD3DXEffectTechnique::GetD3DShader", 0x00749FA0, &ni::NiD3DXEffectTechnique_GetD3DShader);
NID3DXTECH_REVERSED("NiD3DXEffectTechnique::AddParameter", 0x0074A2D0, &ni::NiD3DXEffectTechnique_AddParameter);
NID3DXTECH_REVERSED("NiD3DXEffectTechnique::GetFirstParameter", 0x0074A340, &ni::NiD3DXEffectTechnique_GetFirstParameter);
NID3DXTECH_REVERSED("NiD3DXEffectTechnique::NiD3DXEffectTechnique", 0x0074A360, &ni::NiD3DXEffectTechnique_Construct);
NID3DXTECH_REVERSED("NiD3DXEffectTechniqueSet::SetName", 0x0074D9B0, &ni::NiD3DXEffectTechniqueSet_SetName);
NID3DXTECH_REVERSED("NiD3DXEffectTechniqueSet::GetShaderDesc", 0x0074D9D0, &ni::NiD3DXEffectTechniqueSet_GetShaderDesc);
NID3DXTECH_REVERSED("NiD3DXEffectTechniqueSet::GetBestD3DShader", 0x0074DC30, &ni::NiD3DXEffectTechniqueSet_GetBestD3DShader);
NID3DXTECH_REVERSED("NiD3DXEffectTechniqueSet::ReleaseShaders", 0x0074DCA0, &ni::NiD3DXEffectTechniqueSet_ReleaseShaders);
NID3DXTECH_REVERSED("NiD3DXEffectTechniqueSet::GetD3DShader", 0x0074DCD0, &ni::NiD3DXEffectTechniqueSet_GetD3DShader);
NID3DXTECH_REVERSED("NiD3DXEffectTechniqueSet::~NiD3DXEffectTechniqueSet", 0x0074DD10, &ni::NiD3DXEffectTechniqueSet_Destruct);
NID3DXTECH_REVERSED("NiD3DXEffectTechniqueSet::NiD3DXEffectTechniqueSet", 0x0074DD90, &ni::NiD3DXEffectTechniqueSet_Construct);
NID3DXTECH_REVERSED("NiD3DXEffectTechniqueSet::DeletingDestructor", 0x0074DDE0, &ni::NiD3DXEffectTechniqueSet_DeletingDestructor);
NID3DXTECH_REVERSED("NiD3DXEffectTechniqueSet::AddTechnique", 0x0074DE00, &ni::NiD3DXEffectTechniqueSet_AddTechnique);
