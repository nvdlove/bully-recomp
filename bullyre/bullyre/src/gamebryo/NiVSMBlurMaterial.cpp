// NiVSMBlurMaterial (vtable 0x9627E0, 0x3BC bytes: NiFragmentMaterial's and its descriptor name at
// +0x3B8): the material the variance shadow maps are blurred with, one pass over a screen quad
// (src/gamebryo/NiFragmentMaterial.h; its nodes from NiStandardMaterial's library). Its
// descriptor (the skinning, the base map's apply mode and transform, the VSM technique's kernel
// size), its pass description, its vertex program (the position projected, a texture coordinate
// passed through, optionally transformed) and pixel program (a horizontal or vertical blur sample
// of the base map over the kernel), its vertex inputs and packing.
//
// Its slots past NiFragmentMaterial's: 36 HandleBlurSample, 37 HandleTextureTransform, 38
// HandlePositionFragment (NiStandardMaterial's code, 0x7AF6F0), 39 HandleViewProjectionFragment
// (the code NiStandardMaterial's slot 39 shares), 40 HandleTransforms.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiFragmentMaterial.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::frag;

struct NiVSMBlurMaterial : NiFragmentMaterial {
    re::Ptr32<const char> descriptorName;   // +0x3B8 ("NiVSMBlurMaterialDescriptor")
};
static_assert(offsetof(NiVSMBlurMaterial, descriptorName) == 0x3B8);
static_assert(sizeof(NiVSMBlurMaterial) == 0x3BC);

using Self = NiVSMBlurMaterial;
using Resource = void;

constexpr uintptr_t kVSMBlurMaterialVtable = 0x009627E0;
GAME_VAR(uint8_t, NiVSMBlurMaterial_RTTI, 0x020E4244);
GAME_VAR(void*, ms_pkShadowManager, 0x020E2410);
// NiShaderDeclaration's usage names (POSITION, BLENDWEIGHT, BLENDINDICES, ..., TEXCOORD at 5)
GAME_ARRAY(NiFixedString, UsageNames, 0x020E31CC);

GAME_FN(NiFragmentMaterial*, __thiscall, FragmentMaterial_Construct, 0x00788260, void* self, NiRefObject* library,
        const re::Ptr32<const char>* name, uint32_t vertexVersion, uint32_t geometryVersion, uint32_t pixelVersion,
        int autoCreateCaches);
GAME_FN(void, __thiscall, FragmentMaterial_Destruct, 0x00788390, void* self);
GAME_FN(void, __thiscall, SetFailedGPUProgram, 0x00786520, void* self, uint32_t type, const NiMaterialDescriptor* desc);
GAME_FN(void, __thiscall, Context_Destruct, 0x0078A030, MaterialContext* self);
GAME_FN(void, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<NiRefObject>* slot, NiRefObject* value);
GAME_FN(NiRefObject*, __thiscall, Configurator_Construct, 0x007E92F0, void* self, NiRefObject* cache);
GAME_FN(NiRefObject*, __thiscall, ProducerNode_Construct, 0x007EA880, void* self, const re::Ptr32<const char>* name,
        const re::Ptr32<const char>* type);
GAME_FN(NiRefObject*, __thiscall, ConsumerNode_Construct, 0x007EA7D0, void* self, const re::Ptr32<const char>* name,
        const re::Ptr32<const char>* type);
GAME_FN(void*, __thiscall, Node_AddInputResource, 0x007E31A0, void* self, const re::Ptr32<const char>* type,
        const re::Ptr32<const char>* semantic, const re::Ptr32<const char>* label, const re::Ptr32<const char>* variable, uint32_t count,
        uint32_t source, uint32_t objectType, uint32_t extraData);
GAME_FN(uint8_t, __thiscall, Configurator_AddBindingFromOutput, 0x007E96C0, void* self,
        const re::Ptr32<const char>* outputName, void* node, void* input);
GAME_FN(void, __thiscall, StandardDesc_SetTextureUsage, 0x007777A0, NiMaterialDescriptor* self, uint32_t map,
        uint32_t uvSet, uint32_t transform);
GAME_FN(void, __thiscall, StandardDesc_GetTextureUsage, 0x007F80F0, const NiMaterialDescriptor* self, uint32_t map,
        uint32_t* uvSet, uint32_t* transform);
GAME_FN(void, __thiscall, VertexDesc_SetOutputUV, 0x007E5D00, NiMaterialDescriptor* self, uint32_t output,
        uint32_t uvSet, uint32_t texOutput);
GAME_FN(void, __thiscall, VertexDesc_GetOutputUV, 0x007E5F20, const NiMaterialDescriptor* self, uint32_t output,
        uint32_t* uvSet, uint32_t* texOutput);
GAME_FN(void, __thiscall, PixelDesc_SetUVSetForMap, 0x007E3690, NiMaterialDescriptor* self, uint32_t map,
        uint32_t uvSet);
GAME_FN(void*, __cdecl, ShadowManager_GetShadowTechnique, 0x00757220, const re::Ptr32<const char>* name);
GAME_FN(uint32_t, __thiscall, Get50, 0x007D0E60, void* self);   // the VSM technique's kernel size (+0x50)
GAME_FN(uint8_t, __thiscall, SkinInstance_ContainsVertexData, 0x00776F00, void* skin, int semantic);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);

// ---- the idioms (as NiStandardMaterial's builders have them)

// a context node: NiNew'd, constructed with its names (made type first in the given way), held in
// the context's slot, then its names let go (the name first)
template <typename Ctor>
NiRefObject* NewContextNode(Ctor construct, Temp kind, const char* type, const char* name, re::Ptr32<NiRefObject>* slot) {
    void* memory = NiNew(0x34);
    NiRefObject* node = nullptr;
    NiFixedString t = nullptr;
    NiFixedString n = nullptr;
    if (memory != nullptr) {
        t = MakeTemp(kind, type);
        n = MakeTemp(kind, name);
        node = construct(memory, &n, &t);
    }
    Pointer_Assign(slot, node);
    if (memory != nullptr) {
        DropTemp(kind, n);
        DropTemp(kind, t);
    }
    return node;
}

// a node's input or output resource named by four temporary names: made variable, label,
// semantic, type, let go the other way round
void* NamedResource(bool input, void* node, Temp kind, const char* type, const char* semantic, const char* label,
                    const char* variable) {
    NiFixedString v = MakeTemp(kind, variable);
    NiFixedString l = MakeTemp(kind, label);
    NiFixedString s = MakeTemp(kind, semantic);
    NiFixedString t = MakeTemp(kind, type);
    void* resource = input ? Node_AddInputResource(node, &t, &s, &l, &v, 1, 0, 0, 0)
                           : Node_AddOutputResource(node, &t, &s, &l, &v, 1, 0, 0, 0);
    DropTemp(kind, t);
    DropTemp(kind, s);
    DropTemp(kind, l);
    DropTemp(kind, v);
    return resource;
}

// slot 31 AddOutputPredefined(uniforms, mapping, registers, count, extra)
inline void* Predefined(Self* self, MaterialContext* context, uint32_t mapping, uint32_t registers) {
    return Virtual<void*>(self, 31, context->uniforms, mapping, registers, 1u, 0u);
}

// the configurator's binding of an output to the node's input by its name
inline void BindByName(MaterialContext* context, void* output, void* node, const char* input) {
    NiFixedString s = String(input);
    Configurator_AddBindingByName(context->configurator, output, &s, node);
    Drop(s);
}

// NiShaderDeclaration::ShaderRegisterEntry (0x18 bytes)
struct ShaderRegisterEntry {
    uint32_t preferredRegister;   // +0x0
    uint32_t input;               // +0x4
    uint32_t type;                // +0x8
    re::Ptr32<const char> usage;          // +0xC
    uint32_t usageIndex;          // +0x10 (-1 ends the list)
    uint32_t extraData;           // +0x14
};
static_assert(sizeof(ShaderRegisterEntry) == 0x18);

inline void SetEntry(ShaderRegisterEntry* entry, uint32_t input, uint32_t type, const re::Ptr32<const char>* usage) {
    entry->preferredRegister = 0;
    entry->input = input;
    entry->type = type;
    FixedString_Assign(&entry->usage, usage);
    entry->usageIndex = 0;
    entry->extraData = 0;
}

// ---- the slots

// vf18 GenerateDescriptor(geometry, skin, property state, effect state, descriptor): false without
// properties. The skinning; the texturing's apply mode bits and the base map's transform; the VSM
// technique's kernel size less one in bits 4..7 of the second word (4 when there's no technique)
uint8_t __fastcall GenerateDescriptor(Self* self, re::Edx, void*, void* skin, void* state, void*,
                                      NiMaterialDescriptor* desc) {
    if (state == nullptr) {
        return 0;
    }
    FixedString_Assign(&desc->identifier, &self->descriptorName);
    uint32_t* bits = desc->bits;
    void* partition = skin != nullptr ? At<void*>(skin, 0xC) : nullptr;
    if (partition == nullptr) {
        bits[0] &= ~3u;
    } else {
        void* partitions = At<void*>(partition, 0xC);
        if (At<uint32_t>(partition, 8) != 0 && partitions != nullptr && At<uint32_t>(partitions, 0x10) != 0) {
            bits[0] = (bits[0] & ~2u) | 1;
        } else {
            bits[0] = (bits[0] & ~1u) | 2;
        }
    }
    void* texturing = At<void*>(state, 0x28);
    if (texturing != nullptr) {
        if ((At<uint8_t>(texturing, 0x18) & 0xE) != 0) {
            bits[0] |= 0x200;
        } else {
            bits[0] &= ~0x200u;
        }
        void* base = At<re::Ptr32<void>*>(texturing, 0x20)[0];
        uint32_t transformed = base != nullptr && At<void*>(base, 0xC) != nullptr ? 1 : 0;
        StandardDesc_SetTextureUsage(desc, 0, 0, transformed);
    }
    uint32_t kernel = 4;
    NiFixedString name = String("NiVSMShadowTechnique");
    void* technique = ShadowManager_GetShadowTechnique(&name);
    Drop(name);
    if (technique != nullptr) {
        kernel = Get50(technique);
    }
    bits[1] = ((kernel - 1) << 4) | (bits[1] & 0xFFFFFF0F);
    return 1;
}

// vf23 the pixel program: the texture coordinate's blur sample (horizontal or vertical as the
// descriptor's bit 0 says, over its kernel) written to the color. (The pixel cache isn't checked for
// null, and a failure isn't told to it, as the game has it)
NiRefObject* __fastcall GeneratePixelShader(Self* self, re::Edx, NiMaterialDescriptor* desc, void* uniforms) {
    char name[0xF4];
    Descriptor_ToString(desc, name, 0x30);
    NiRefObject* cache = self->programCaches[1];
    uint8_t failed = 0;
    NiRefObject* cached = Virtual<NiRefObject*>(cache, 3, static_cast<const char*>(name), uniforms, &failed);
    if (cached != nullptr) {
        return cached;
    }
    if (failed != 0) {
        return nullptr;
    }
    MaterialContext context = {};
    void* memory = NiNew(0x34);
    Pointer_Assign(&context.configurator,
                   memory != nullptr ? Configurator_Construct(memory, self->programCaches[1]) : nullptr);
    NewContextNode(ProducerNode_Construct, Temp::Destructed, "Uniform", "Uniforms", &context.uniforms);
    Configurator_AddNode(context.configurator, context.uniforms);
    NewContextNode(ProducerNode_Construct, Temp::Destructed, "Static", "Statics", &context.statics);
    Configurator_AddNode(context.configurator, context.statics);
    NewContextNode(ProducerNode_Construct, Temp::Destructed, "Vertex", "VertexOut", &context.inputs);
    NamedResource(false, context.inputs, Temp::Inline, "float4", "Position", "World", "PosProjected");
    Configurator_AddNode(context.configurator, context.inputs);
    NewContextNode(ConsumerNode_Construct, Temp::Destructed, "Pixel", "PixelOut", &context.outputs);
    void* color = NamedResource(true, context.outputs, Temp::Inline, "float4", "Color", "", "Color");
    Configurator_AddNode(context.configurator, context.outputs);
    Resource* unused = nullptr;
    void* uv = NamedResource(false, context.inputs, Temp::Inline, "float2", "TexCoord", "", "UVSet0");
    void* sampleNode = nullptr;
    NiFixedString fragment = NullFixedString;
    FixedString_AssignText(&fragment, (desc->bits[0] & 1) != 0 ? "VertBlurSample" : "HorzBlurSample");
    uint32_t kernel = (desc->bits[2] >> 28) + 1;
    if (!Virtual<uint8_t>(self, 36, &context, uv, &unused, &sampleNode, &fragment, kernel)) {   // HandleBlurSample
        FixedString_Destruct(&fragment);
        Context_Destruct(&context);
        return nullptr;
    }
    NiFixedString out = String("ColorOut");
    Configurator_AddBindingFromOutput(context.configurator, &out, sampleNode, color);
    FixedString_Destruct(&out);
    NiRefObject* program = Virtual<NiRefObject*>(context.configurator, 2, static_cast<const char*>(name), 1u, uniforms);
    FixedString_Destruct(&fragment);
    Context_Destruct(&context);
    return program;
}

// vf21 the vertex program: the position transformed and projected (slot 40), the first texture
// coordinate passed through, transformed when the descriptor's output says so (slot 37)
NiRefObject* __fastcall GenerateVertexShader(Self* self, re::Edx, NiMaterialDescriptor* desc, void* uniforms) {
    char name[0xF4];
    Descriptor_ToString(desc, name, 0x30);
    NiRefObject* cache = self->programCaches[0];
    if (cache == nullptr) {
        return nullptr;
    }
    uint8_t failed = 0;
    NiRefObject* cached = Virtual<NiRefObject*>(cache, 3, static_cast<const char*>(name), uniforms, &failed);
    if (cached != nullptr) {
        return cached;
    }
    if (failed != 0) {
        return nullptr;
    }
    MaterialContext context = {};
    void* memory = NiNew(0x34);
    Pointer_Assign(&context.configurator,
                   memory != nullptr ? Configurator_Construct(memory, self->programCaches[0]) : nullptr);
    NewContextNode(ProducerNode_Construct, Temp::Constructed, "Vertex", "VertexIn", &context.inputs);
    Configurator_AddNode(context.configurator, context.inputs);
    NewContextNode(ProducerNode_Construct, Temp::Constructed, "Uniform", "Uniforms", &context.uniforms);
    Configurator_AddNode(context.configurator, context.uniforms);
    NewContextNode(ProducerNode_Construct, Temp::Constructed, "Static", "Statics", &context.statics);
    Configurator_AddNode(context.configurator, context.statics);
    NewContextNode(ConsumerNode_Construct, Temp::Constructed, "Vertex", "VertexOut", &context.outputs);
    void* projected = NamedResource(true, context.outputs, Temp::Destructed, "float4", "Position", "World",
                                    "PosProjected");
    Configurator_AddNode(context.configurator, context.outputs);
    Resource* uvOut = nullptr;
    void* uvIn = NamedResource(false, context.inputs, Temp::Destructed, "float2", "TexCoord", "", "UVSet0");
    void* uvInput = NamedResource(true, context.outputs, Temp::Destructed, "float2", "TexCoord", "", "UVSet0");
    uint32_t uvSet;
    uint32_t texOutput;
    VertexDesc_GetOutputUV(desc, 0, &uvSet, &texOutput);
    NiFixedString base = String("Base");
    Virtual<uint8_t>(self, 37, &context, uvIn, &uvOut, &base, 0u, texOutput);   // HandleTextureTransform
    FixedString_Destruct(&base);
    Configurator_AddBinding(context.configurator, uvOut, uvInput);
    Resource* worldPos = nullptr;
    Resource* viewPos = nullptr;
    Resource* worldNormal = nullptr;
    Resource* worldView = nullptr;
    if (!Virtual<uint8_t>(self, 40, &context, projected, desc, 1, 0, &worldPos, &viewPos, &worldNormal,
                          &worldView)) {   // HandleTransforms
        SetFailedGPUProgram(self, 0, desc);
        Context_Destruct(&context);
        return nullptr;
    }
    NiRefObject* program = Virtual<NiRefObject*>(context.configurator, 2, static_cast<const char*>(name), 0u, uniforms);
    if (program == nullptr) {
        SetFailedGPUProgram(self, 0, desc);
    }
    Context_Destruct(&context);
    return program;
}

// vf40 HandleTransforms(context, projected output, descriptor, (1), (0), worldPos&, viewPos&,
// worldNormal&, worldView&): the position to the world (slot 38) and projected (slot 39) into the
// output; the world position passed on too when the descriptor asks (bit 2: a "WorldPos" input
// and output on the outputs node). The last two left null
uint8_t __fastcall HandleTransforms(Self* self, re::Edx, MaterialContext* context, Resource* projected,
                                   NiMaterialDescriptor* desc, int, int forceViewPos, re::Ptr32<Resource>* worldPos,
                                   re::Ptr32<Resource>* viewPos, re::Ptr32<Resource>* worldNormal, re::Ptr32<Resource>* worldView) {
    *worldPos = nullptr;
    *worldNormal = nullptr;
    *worldView = nullptr;
    Resource* world = nullptr;
    if (!Virtual<uint8_t>(self, 38, context, desc->bits[0] & 3, worldPos, &world)) {   // HandlePositionFragment
        return 0;
    }
    Resource* position = nullptr;
    if (!Virtual<uint8_t>(self, 39, context, forceViewPos, *worldPos, &position, viewPos)) {
        return 0;
    }
    if (position == nullptr || projected == nullptr) {
        return 0;
    }
    Configurator_AddBinding(context->configurator, position, projected);
    if ((desc->bits[0] & 4) != 0) {
        void* input = NamedResource(true, context->outputs, Temp::Constructed, "float4", "TexCoord", "", "WorldPos");
        NamedResource(false, context->outputs, Temp::Constructed, "float4", "TexCoord", "", "WorldPos");
        Configurator_AddBinding(context->configurator, *worldPos, input);
    }
    return 1;
}

// vf24 GenerateShaderDescArray(descriptor, passes, max passes, count&): one pass, alpha overridden
// without blending; the vertex program's transform and first output UV (the base map's), the
// pixel program's apply mode bit, kernel size and the active VSM technique's ID (+0x46 of the
// shadow manager's technique of the descriptor's bits 28..29), its map 0 on UV set 0
uint32_t __fastcall GenerateShaderDescArray(Self*, re::Edx, NiMaterialDescriptor* desc, RenderPassDescriptor* passes,
                                            uint32_t, uint32_t* count) {
    *count = 0;
    if (FixedString_Equals(&desc->identifier, "NiVSMBlurMaterialDescriptor") == 0) {
        return 8;
    }
    RenderPassDescriptor* pass = passes;
    pass->usesNiRenderState = 1;
    pass->alphaOverride = 1;
    pass->alphaBlend = 0;
    pass->resetObjectOffsets = 1;
    FixedString_AssignText(&pass->vertexDesc.identifier, "NiVSMBlurVertexProgramDescriptor");
    FixedString_AssignText(&pass->pixelDesc.identifier, "NiVSMBlurPixelProgramDescriptor");
    pass->vertexDesc.bits[0] ^= (pass->vertexDesc.bits[0] ^ desc->bits[0]) & 3;
    uint32_t uvSet;
    uint32_t transform;
    StandardDesc_GetTextureUsage(desc, 0, &uvSet, &transform);
    VertexDesc_SetOutputUV(&pass->vertexDesc, 0, uvSet, transform);
    uint32_t* pixel = pass->pixelDesc.bits;
    pixel[0] ^= ((desc->bits[0] >> 9) ^ pixel[0]) & 1;
    uint32_t kernel = desc->bits[1] << 24;
    pixel[2] = ((kernel ^ pixel[2]) & 0x0FFFFFFF) ^ kernel;
    PixelDesc_SetUVSetForMap(&pass->pixelDesc, 0, 0);
    void* technique = static_cast<re::Ptr32<void>*>(ms_pkShadowManager)[(desc->bits[3] >> 28) & 3];
    pixel[3] = (uint32_t(At<uint16_t>(technique, 0x46)) << 24) | (pixel[3] & 0xF0FFFFFF);
    *count = *count + 1;
    return 0;
}

// vf28 SetupPackingRequirements(shader, descriptor, passes, count): the shader's vertex declaration
// (its slot 18: 15 entries, one stream) with the position, the skinning's streams and the texture
// coordinate (the declaration's slot 18: SetEntry(entry, input, type, stream)), held through the
// import's interlocked calls
uint8_t __fastcall SetupPackingRequirements(Self*, re::Edx, void* shader, NiMaterialDescriptor* desc, void*, uint32_t) {
    NiRefObject* declaration = Virtual<NiRefObject*>(shader, 18, 0xFu, 1u);
    if (declaration == nullptr) {
        return 0;
    }
    sys::Increment(&declaration->refCount);
    Virtual<void>(declaration, 18, 0u, 0u, 2u, 0u);
    uint32_t next = 1;
    uint32_t skinning = desc->bits[0] & 3;
    if (skinning == 1) {
        Virtual<void>(declaration, 18, 1u, 1u, 3u, 0u);
        Virtual<void>(declaration, 18, 2u, 2u, 5u, 0u);
        next = 3;
    } else if (skinning == 2) {
        Virtual<void>(declaration, 18, 1u, 1u, 3u, 0u);
        next = 2;
    }
    Virtual<void>(declaration, 18, next, 5u, 1u, 0u);
    if (sys::Decrement(&declaration->refCount) == 0) {
        Virtual<void>(declaration, 1);
    }
    return 1;
}

// vf39 HandleViewProjectionFragment(context, force view position, world position, projected&,
// view position&): the world position through the view and projection (the view position given
// back too when forced), or the view-projection at once. NiStandardMaterial's slot 39 is this code
uint8_t __fastcall HandleViewProjectionFragment(NiFragmentMaterial* self, re::Edx, MaterialContext* context,
                                               int forceViewPos, Resource* worldPos, re::Ptr32<Resource>* projected,
                                               re::Ptr32<Resource>* viewPos) {
    void* node;
    if (re::Flag(forceViewPos)) {
        node = LibraryNode(self, "ProjectPositionWorldToViewToProj");
        if (node == nullptr) {
            return 0;
        }
        AddNode(context, node);
        void* view = Virtual<void*>(self, 31, context->uniforms, 3u, 4u, 1u, 0u);
        BindInput(context, view, node, "ViewTransform");
        void* projection = Virtual<void*>(self, 31, context->uniforms, 1u, 4u, 1u, 0u);
        BindInput(context, projection, node, "ProjTransform");
        BindInput(context, worldPos, node, "WorldPosition");
        *viewPos = OutputOf(node, "ViewPos");
    } else {
        node = LibraryNode(self, "ProjectPositionWorldToProj");
        if (node == nullptr) {
            return 0;
        }
        AddNode(context, node);
        void* viewProjection = Virtual<void*>(self, 31, context->uniforms, 9u, 4u, 1u, 0u);
        BindInput(context, viewProjection, node, "ViewProjection");
        BindInput(context, worldPos, node, "WorldPosition");
    }
    *projected = OutputOf(node, "ProjPos");
    return 1;
}

// vf5 GetVertexInputSemantics(geometry, (unused), entries): the position, a skin's blend weights
// and (with a bone palette) indices, the first texture coordinate when the data or skin has one;
// the list ended by a usage index of -1
uint8_t __fastcall GetVertexInputSemantics(Self*, re::Edx, void* geometry, uint32_t, ShaderRegisterEntry* entries) {
    SetEntry(&entries[0], 0, 2, &UsageNames[0]);
    uint32_t count = 1;
    void* skin = At<void*>(geometry, 0xD0);
    if (skin != nullptr) {
        void* partition = At<void*>(skin, 0xC);
        if (partition != nullptr) {
            void* partitions = At<void*>(partition, 0xC);
            SetEntry(&entries[1], 1, 2, &UsageNames[1]);
            count = 2;
            if (At<uint32_t>(partition, 8) != 0 && partitions != nullptr && At<uint32_t>(partitions, 0x10) != 0) {
                SetEntry(&entries[2], 2, 5, &UsageNames[2]);
                count = 3;
            }
        }
    }
    void* data = At<void*>(geometry, 0xCC);
    bool texture = Virtual<uint8_t>(data, 19, 5) != 0;
    if (!texture) {
        void* geometrySkin = At<void*>(geometry, 0xD0);
        texture = geometrySkin != nullptr && SkinInstance_ContainsVertexData(geometrySkin, 5) != 0;
    }
    if (texture) {
        SetEntry(&entries[count], 5, 1, &UsageNames[5]);
        count++;
    }
    entries[count].usageIndex = 0xFFFFFFFF;
    return 1;
}

// vf37 HandleTextureTransform(context, uv, uv out&, map name, (0), texture output): the coordinate
// as it is, or through TexTransformApply with the map's transform (an object uniform, mapping 0x25;
// the "<map>_TexTransform" name it formats isn't used) when the output asks for one. False without
// a coordinate
uint8_t __fastcall HandleTextureTransform(Self* self, re::Edx, MaterialContext* context, Resource* uv, re::Ptr32<Resource>* out,
                                         const re::Ptr32<const char>* map, uint32_t, uint32_t texOutput) {
    NiFixedString type = NullFixedString;
    *out = uv;
    if (uv == nullptr) {
        FixedStringRelease(&type);
        return 0;
    }
    FixedString_AssignText(&type, "float2");
    NiFixedString mapName = *map;   // a copy (NiFixedString's)
    if (mapName != NullFixedString) {
        sys::Increment(re::PtrCast<char*>(mapName) - 8);
    }
    mapName = *map;
    if (texOutput == 1) {
        void* node = LibraryNode(self, "TexTransformApply");
        AddNode(context, node);
        BindInput(context, uv, node, "TexCoord");
        char transformName[0x20];
        NiSprintf(transformName, 0x20, "%s_TexTransform", *map);
        void* transform = Virtual<void*>(self, 31, context->uniforms, 0x25u, 1u, 1u, 0u);
        BindInput(context, transform, node, "TexTransform");
        *out = OutputOf(node, "TexCoordOut");
    }
    FixedStringRelease(&mapName);
    FixedStringRelease(&type);
    return 1;
}

// vf36 HandleBlurSample(context, uv, (unused out), sample node&, fragment, kernel size): the
// fragment of that name with the base map's sampler, the texture coordinate, the map size (an
// object uniform, mapping 0x6A) and the kernel size and "no saturation" as constants; its color
// output the color. False when the fragment, sampler or either of the first two bindings fails
uint8_t __fastcall HandleBlurSample(Self* self, re::Edx, MaterialContext* context, Resource* uv, re::Ptr32<Resource>* color,
                                   re::Ptr32<void>* node, const re::Ptr32<const char>* fragment, uint32_t kernel) {
    *node = GetAttachableNodeFromLibrary(self, fragment);
    NiFixedString base = String("Base");
    void* sampler = Virtual<void*>(self, 30, context, &base, 0u, 0u, 0u);   // InsertTextureSampler
    Drop(base);
    if (sampler == nullptr) {
        return 0;
    }
    AddNode(context, *node);
    uint8_t ok = BindInput(context, uv, *node, "TexCoord") & 1;
    ok &= BindInput(context, sampler, *node, "Sampler");
    BindConstantByName(context, *node, "Saturate", "bool", "(false)");
    void* mapSize = Virtual<void*>(self, 31, context->uniforms, 0x6Au, 1u, 1u, 0u);
    BindByName(context, mapSize, *node, "MapSize");
    char value[0x100];
    NiSprintf(value, 0x100, "(%d.0)", kernel);
    BindConstant(context, *node, "KernelSize", "float", value);
    if (ok == 0) {
        return 0;
    }
    *color = OutputOf(*node, "ColorOut");
    return ok;
}

// 0x7ECDD0 NiVSMBlurMaterial(library, create caches): "NiVSMBlurMaterial", versions 1, 0, 1
Self* __fastcall Construct(Self* self, re::Edx, NiRefObject* library, int autoCreateCaches) {
    NiFixedString name = String("NiVSMBlurMaterial");
    FragmentMaterial_Construct(self, library, &name, 1, 0, 1, autoCreateCaches);
    Drop(name);
    self->vtable = reinterpret_cast<void*>(kVSMBlurMaterialVtable);
    self->descriptorName = String("NiVSMBlurMaterialDescriptor");
    return self;
}

// vf2
const void* __fastcall GetRTTI(Self*, re::Edx) {
    return &NiVSMBlurMaterial_RTTI;
}

// vf0 the deleting destructor (~NiFragmentMaterial after its name)
Self* __fastcall DeletingDestructor(Self* self, re::Edx, uint32_t flags) {
    FixedStringRelease(&self->descriptorName);
    FragmentMaterial_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x3BC);
    }
    return self;
}

} // namespace

// the rest of GenerateDescriptor past its null check (a jump target inside it, not a function)
RE_UNHOOKED("NiVSMBlurMaterial::GenerateDescriptor (body)", 0x007EB0FF);

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIVSMBLUR_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIVSMBLUR_REVERSED("NiVSMBlurMaterial::GenerateDescriptor", 0x007EB0F0, ni::GenerateDescriptor);
NIVSMBLUR_REVERSED("NiVSMBlurMaterial::GeneratePixelShader", 0x007EB210, ni::GeneratePixelShader);
NIVSMBLUR_REVERSED("NiVSMBlurMaterial::GenerateVertexShader", 0x007EB900, ni::GenerateVertexShader);
NIVSMBLUR_REVERSED("NiVSMBlurMaterial::HandleTransforms", 0x007EBE60, ni::HandleTransforms);
NIVSMBLUR_REVERSED("NiVSMBlurMaterial::GenerateShaderDescArray", 0x007EC040, ni::GenerateShaderDescArray);
NIVSMBLUR_REVERSED("NiVSMBlurMaterial::SetupPackingRequirements", 0x007EC140, ni::SetupPackingRequirements);
NIVSMBLUR_REVERSED("NiStandardMaterial::HandleViewProjectionFragment", 0x007EC200, ni::HandleViewProjectionFragment);
NIVSMBLUR_REVERSED("NiVSMBlurMaterial::GetVertexInputSemantics", 0x007EC570, ni::GetVertexInputSemantics);
NIVSMBLUR_REVERSED("NiVSMBlurMaterial::HandleTextureTransform", 0x007EC680, ni::HandleTextureTransform);
NIVSMBLUR_REVERSED("NiVSMBlurMaterial::HandleBlurSample", 0x007EC950, ni::HandleBlurSample);
NIVSMBLUR_REVERSED("NiVSMBlurMaterial::NiVSMBlurMaterial", 0x007ECDD0, ni::Construct);
NIVSMBLUR_REVERSED("NiVSMBlurMaterial::GetRTTI", 0x007ECE60, ni::GetRTTI);
NIVSMBLUR_REVERSED("NiVSMBlurMaterial::DeletingDestructor", 0x007ECE70, ni::DeletingDestructor);
