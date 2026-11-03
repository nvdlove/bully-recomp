// The shadow write materials' other slots (src/gamebryo/NiShadowWriteMaterials.cpp has their
// shaders and pass descriptions): NiPointShadowWriteMaterial, NiSpotShadowWriteMaterial and
// NiDirectionalShadowWriteMaterial, NiStandardMaterials of 0x3C4 bytes (their own descriptor
// name at +0x3C0) sharing every slot but the pixel program and GetRTTI. Their descriptor (the
// skinning, the base map's alpha test, the shadow technique of the shadowed lights), their vertex
// inputs and packing (position and skinning only), their transform pipeline (no normals: the
// position, the projection also passed through to the pixel program, the view vector), the
// directional one's pixel program (the technique's directional write fragment), the spot and
// directional constructors, the destructor.
//
// And NiStandardMaterial::HandlePositionFragment (slot 36, shared by all of them): the local
// position transformed to the world, rigid or skinned (with or without bone indices).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiFragmentMaterial.h"
#include "gamebryo/NiShadeTree.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::shade;

struct NiShadowWriteMaterial : NiStandardMaterial {
    re::Ptr32<const char> writeDescriptorName;   // +0x3C0 ("NiStandardMaterialDescriptor")
};
static_assert(offsetof(NiShadowWriteMaterial, writeDescriptorName) == 0x3C0);
static_assert(sizeof(NiShadowWriteMaterial) == 0x3C4);

using Self = NiShadowWriteMaterial;
using Resource = void;

constexpr uintptr_t NiSpotShadowWriteMaterial_vtable = 0x0095FA50;
constexpr uintptr_t NiDirectionalShadowWriteMaterial_vtable = 0x0095FBB8;
GAME_VAR(uint8_t, NiSpotShadowWriteMaterial_RTTI, 0x020E3700);
GAME_VAR(uint8_t, NiDirectionalShadowWriteMaterial_RTTI, 0x020E3734);

// NiShaderDeclaration's usage names (POSITION, BLENDWEIGHT, BLENDINDICES...)
GAME_ARRAY(NiFixedString, UsageNames, 0x020E31CC);

GAME_FN(NiStandardMaterial*, __thiscall, StandardMaterial_ConstructNamed, 0x00797F30, void* self,
        const re::Ptr32<const char>* name, NiRefObject* library, uint32_t vertexVersion, uint32_t geometryVersion,
        uint32_t pixelVersion, int autoCreateCaches);
GAME_FN(void, __thiscall, FragmentMaterial_Destruct, 0x00788390, void* self);
GAME_FN(void, __thiscall, ShadowWriteMaterial_Destruct, 0x007AE570, void* self);   // (this file's)
GAME_FN(uint8_t, __thiscall, Geometry_ContainsVertexData, 0x00516B10, void* geometry, int semantic);
GAME_FN(void, __thiscall, StandardDesc_SetTextureUsage, 0x007777A0, NiMaterialDescriptor* self, uint32_t map,
        uint32_t uvSet, uint32_t transform);
// AddBinding(output's name, output node, input's name, input node)
GAME_FN(uint8_t, __thiscall, Configurator_AddBindingNames, 0x007E95B0, void* self, const re::Ptr32<const char>* outputName,
        void* outputNode, const re::Ptr32<const char>* inputName, void* inputNode);

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

// slot 31 AddOutputPredefined(uniforms, mapping, registers, count, extra)
inline void* PredefinedN(void* self, MaterialContext* context, uint32_t mapping, uint32_t registers,
                         uint32_t count = 1) {
    return Virtual<void*>(self, 31, context->uniforms, mapping, registers, count, 0u);
}

// AddBinding by both names (0x7E95B0): the input's name made first, the output's let go first
inline void BindNames(MaterialContext* context, const char* output, void* outputNode, const char* input,
                      void* inputNode, Temp kind) {
    NiFixedString in = MakeTemp(kind, input);
    NiFixedString out = MakeTemp(kind, output);
    Configurator_AddBindingNames(context->configurator, &out, outputNode, &in, inputNode);
    DropTemp(kind, out);
    DropTemp(kind, in);
}

// ---- the shared slots

// slot 5 GetVertexInputSemantics(geometry, (unused), entries): the position, and a skin's blend
// weights and (with a bone palette) indices; the list ended by a usage index of -1
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
    entries[count].usageIndex = 0xFFFFFFFF;
    return 1;
}

// the shadow technique ID of the effect state's shadowed lights of one kind (both types: the
// light's and its shadowed one's), the highest of them and `highest`. The effect state's lights:
// a list at +0xC (next +0x0, the light +0x4); a light's shadow generator +0xD4 (active: bit 3 of
// its flags at +8; its technique +0xC, the technique's ID +0x44)
inline uint32_t HighestTechnique(void* list, uint8_t type, uint8_t shadowedType, uint32_t highest) {
    for (void* node = list; node != nullptr;) {
        void* light = At<void*>(node, 4);
        node = At<void*>(node, 0);
        if (light == nullptr) {
            continue;
        }
        uint8_t kind = At<uint8_t>(light, 0xC5);
        if (kind != type && kind != shadowedType) {
            continue;
        }
        void* generator = At<void*>(light, 0xD4);
        if (generator == nullptr || ((At<uint8_t>(generator, 8) >> 3) & 1) == 0) {
            continue;
        }
        uint32_t id = At<uint16_t>(At<void*>(generator, 0xC), 0x44);
        if (int32_t(id) > int32_t(highest)) {
            highest = id;
        }
    }
    return highest;
}

// slot 18 GenerateDescriptor(geometry, skin, property state, effect state, descriptor): false
// without properties. The skinning; the alpha test (alpha testing on with "greater": test mode
// 4); with the test and a texturing property, the base map's texture usage (its UV set clamped
// to the set count, as NiStandardMaterial has it) and the UV set count; the highest shadow
// technique of the effects' shadowed lights in bits 28..29 of the fourth word
uint8_t __fastcall GenerateDescriptor(Self* self, re::Edx, void* geometry, void* skin, void* state, void* effects,
                                      NiMaterialDescriptor* desc) {
    if (state == nullptr) {
        return 0;
    }
    FixedString_Assign(&desc->identifier, &self->writeDescriptorName);
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

    void* alpha = At<void*>(state, 0x8);
    void* texturing = At<void*>(state, 0x28);
    if (alpha != nullptr) {
        uint16_t flags = At<uint16_t>(alpha, 0x18);
        if (((flags >> 9) & 1) != 0 && (flags & 0x1C00) == 0x1000) {
            bits[3] |= 0x40000000;
        } else {
            bits[3] &= 0xBFFFFFFF;
        }
    }
    if (texturing != nullptr && alpha != nullptr && (bits[3] & 0x40000000) != 0) {
        void* slots[16];
        crt::Memset(slots, 0, sizeof(slots));
        uint32_t uvSets = 0;
        for (uint32_t i = 0; i < 8; i++) {
            if (Geometry_ContainsVertexData(geometry, int(i + 5))) {
                uvSets++;
            }
        }
        if ((At<uint8_t>(texturing, 0x18) & 0xE) != 0) {
            bits[0] |= 0x200;
        } else {
            bits[0] &= ~0x200u;
        }
        if (uvSets != 0) {
            void* base = At<re::Ptr32<void>*>(texturing, 0x20)[0];
            if (base != nullptr && At<void*>(base, 8) != nullptr) {
                bits[0] |= 0x10000;
                slots[1] = base;
            }
        }
        // (the per-set count of untransformed maps the game keeps here is never read)
        uint32_t untransformed[16];
        crt::Memset(untransformed, 0, sizeof(untransformed));
        uint32_t index = 0;
        for (uint32_t i = 0; i < 16; i++) {
            void* map = slots[i];
            if (map == nullptr || uvSets == 0) {
                continue;
            }
            uint32_t uvSet = At<uint8_t>(map, 4);
            if (uvSet > uvSets) {
                uvSet = uvSets;
            }
            uint32_t transformed = 0;
            if (At<void*>(map, 0xC) != nullptr) {
                transformed = 1;
            } else {
                untransformed[uvSet]++;
            }
            StandardDesc_SetTextureUsage(desc, index, uvSet, transformed);
            index++;
        }
        bits[0] = (uvSets << 12) | (bits[0] & 0xFFFF8FFF);
    }
    if (effects != nullptr) {
        void* lights = At<void*>(effects, 0xC);
        uint32_t highest = 0;
        highest = HighestTechnique(lights, 2, 5, highest);
        highest = HighestTechnique(lights, 3, 6, highest);
        highest = HighestTechnique(lights, 4, 7, highest);
        bits[3] = (bits[3] & 0xCFFFFFFF) | (highest << 28);
    }
    return 1;
}

// slot 28 SetupPackingRequirements(shader, descriptor, passes, count): the shader's vertex
// declaration (its slot 18) with the position and the skinning's streams (the declaration's slot
// 18: SetEntry(entry, input, type, stream)), held through the import's interlocked calls
uint8_t __fastcall SetupPackingRequirements(Self*, re::Edx, void* shader, NiMaterialDescriptor* desc, void*, uint32_t) {
    NiRefObject* declaration = Virtual<NiRefObject*>(shader, 18, 0xFu, 1u);
    if (declaration == nullptr) {
        return 0;
    }
    ni::sys::Increment(&declaration->refCount);
    Virtual<void>(declaration, 18, 0u, 0u, 2u, 0u);
    uint32_t skinning = desc->bits[0] & 3;
    if (skinning == 1) {
        Virtual<void>(declaration, 18, 1u, 1u, 3u, 0u);
        Virtual<void>(declaration, 18, 2u, 2u, 5u, 0u);
    } else if (skinning == 2) {
        Virtual<void>(declaration, 18, 1u, 1u, 3u, 0u);
    }
    if (ni::sys::Decrement(&declaration->refCount) == 0) {
        Virtual<void>(declaration, 1);
    }
    return 1;
}

// slot 39 HandleViewProjectionFragment(context, force view position, world position, projected&,
// view position&): the world position projected (through the view when its position is wanted),
// then the projected position also put out as a texture coordinate (PosProjectedPassThrough)
// through a "TeeFloat4": its first output to that, its second the projected position. Only the
// projection's node is checked
uint8_t __fastcall HandleViewProjectionFragment(Self* self, re::Edx, MaterialContext* context, int forceViewPos,
                                                Resource* worldPos, re::Ptr32<Resource>* projected, re::Ptr32<Resource>* viewPos) {
    void* node;
    if (uint8_t(forceViewPos) != 0) {
        node = LibraryNode(self, "ProjectPositionWorldToViewToProj");
        if (node == nullptr) {
            return 0;
        }
        Configurator_AddNode(context->configurator, node);
        BindInput(context, PredefinedN(self, context, 3, 4), node, "ViewTransform");
        BindInput(context, PredefinedN(self, context, 1, 4), node, "ProjTransform");
        BindInput(context, worldPos, node, "WorldPosition");
        *viewPos = OutputOf(node, "ViewPos");
    } else {
        node = LibraryNode(self, "ProjectPositionWorldToProj");
        if (node == nullptr) {
            return 0;
        }
        Configurator_AddNode(context->configurator, node);
        BindInput(context, PredefinedN(self, context, 9, 4), node, "ViewProjection");
        BindInput(context, worldPos, node, "WorldPosition");
    }
    *projected = OutputOf(node, "ProjPos");

    Resource* passThrough =
        NamedResource(true, context->outputs, Temp::Inline, "float4", "TexCoord", "World", "PosProjectedPassThrough");
    void* tee = LibraryNode(self, "TeeFloat4");
    Configurator_AddNode(context->configurator, tee);
    BindInput(context, *projected, tee, "Input");
    NiFixedString output = String("Output1");
    void* configurator = context->configurator;
    Configurator_AddBinding(configurator, Node_GetOutputResourceByVariableName(tee, &output), passThrough);
    Drop(output);
    *projected = OutputOf(tee, "Output2");
    return 1;
}

// slot 40 HandleViewVectorFragment(context, world position, normal, binormal, tangent, compute
// tangent, world view&, tangent view&): the world view vector from the camera's position (not
// checked for its node), also put out as a texture coordinate; no tangent view vector
uint8_t __fastcall HandleViewVectorFragment(Self* self, re::Edx, MaterialContext* context, Resource* worldPos,
                                            Resource*, Resource*, Resource*, int, re::Ptr32<Resource>* worldView,
                                            re::Ptr32<Resource>* tangentView) {
    *worldView = nullptr;
    *tangentView = nullptr;
    void* node = LibraryNode(self, "CalculateViewVector");
    Configurator_AddNode(context->configurator, node);
    BindInput(context, worldPos, node, "WorldPos");
    BindInput(context, PredefinedN(self, context, 0x4E, 1), node, "CameraPos");
    *worldView = OutputOf(node, "WorldViewVector");
    Resource* input =
        NamedResource(true, context->outputs, Temp::Inline, "float3", "TexCoord", "World", "WorldViewVector");
    Configurator_AddBinding(context->configurator, *worldView, input);
    return 1;
}

// slot 42 SetupTransformPipeline(context, projected position output, vertex descriptor, force world
// view, force view position, worldPos&, viewPos&, worldNormal&, worldView&): the position (slot 36)
// and its projection (39) bound to the output, no normals; the view vector (40) only when forced;
// the world position put out as asked
uint8_t __fastcall SetupTransformPipeline(Self* self, re::Edx, MaterialContext* context, Resource* projected,
                                          NiMaterialDescriptor* desc, int forceWorldView, int forceViewPos,
                                          re::Ptr32<Resource>* worldPos, re::Ptr32<Resource>* viewPos, re::Ptr32<Resource>* worldNormal,
                                          re::Ptr32<Resource>* worldView) {
    *worldPos = nullptr;
    *worldNormal = nullptr;
    *worldView = nullptr;
    Resource* world = nullptr;
    if (!Virtual<uint8_t>(self, 36, context, desc->bits[0] & 3, worldPos, &world)) {   // HandlePositionFragment
        return 0;
    }
    Resource* position = nullptr;
    if (!Virtual<uint8_t>(self, 39, context, forceViewPos, *worldPos, &position, viewPos)) {
        return 0;
    }
    if (position == nullptr || projected == nullptr) {
        return 0;
    }
    if (uint8_t(forceWorldView) != 0) {
        Resource* tangentView = nullptr;
        if (!Virtual<uint8_t>(self, 40, context, *worldPos, *worldNormal, static_cast<Resource*>(nullptr),
                              static_cast<Resource*>(nullptr), 0u, worldView, &tangentView)) {
            return 0;
        }
    }
    Configurator_AddBinding(context->configurator, position, projected);
    if ((uint8_t(desc->bits[0]) & 4) != 0) {
        Resource* input =
            NamedResource(true, context->outputs, Temp::Constructed, "float4", "TexCoord", "", "WorldPos");
        NamedResource(false, context->outputs, Temp::Constructed, "float4", "TexCoord", "", "WorldPos");
        Configurator_AddBinding(context->configurator, *worldPos, input);
    }
    return 1;
}

// ---- NiDirectionalShadowWriteMaterial's pixel program (slot 23): as the spot light's, with the
// technique's directional write fragment (its +0x14). The pixel cache isn't checked for null, and
// a failure isn't told to it, as the game has it
NiRefObject* __fastcall DirectionalGeneratePixelShader(Self* self, re::Edx, NiMaterialDescriptor* desc, void* uniforms) {
    char name[0x104];
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
    AddResource(context.inputs, "float4", "Position", "World", "PosProjected");
    Resource* worldPosProjected = AddResource(context.inputs, "float4", "TexCoord", "World", "WorldPosProjected");
    Resource* worldView = AddResource(context.inputs, "float3", "TexCoord", "World", "WorldViewVector");
    Configurator_AddNode(context.configurator, context.inputs);

    re::Ptr32<Resource> coordinates[12];
    for (uint32_t i = 0; i < PixelDesc_GetInputUVCount(desc); i++) {
        NiFixedString label = String("");
        NiFixedString semantic = String("TexCoord");
        NiFixedString type = String("float2");
        void* inputs = context.inputs;
        NiFixedString variable;
        Virtual<re::Ptr32<const char>*>(self, 29, &variable, i);   // GenerateUVSetName
        coordinates[i] = Node_AddOutputResource(inputs, &type, &semantic, &label, &variable, 1, 0, 0, 0);
        Drop(variable);
        Drop(type);
        Drop(semantic);
        Drop(label);
    }

    if ((desc->bits[3] & 0x10000000) != 0 && (desc->bits[0] & 0x100) != 0) {
        Resource* color = nullptr;
        Resource* opacity = nullptr;
        Resource* uv = GetUVForMap(1, desc, static_cast<re::Ptr32<Resource>*>(coordinates));
        if (!Virtual<uint8_t>(self, 53, &context, uv, &color, &opacity, 1u)) {   // HandleBaseMap
            Context_Destruct(&context);
            return nullptr;
        }
        void* node = LibraryNode(self, "ApplyAlphaTest", Temp::Destructed);
        Configurator_AddNode(context.configurator, node);
        Resource* function = Predefined(self, &context, 0x74);
        BindInput(&context, function, node, "AlphaTestFunction", Temp::Destructed);
        Resource* reference = Predefined(self, &context, 0x75);
        BindInput(&context, reference, node, "AlphaTestRef", Temp::Destructed);
        BindInput(&context, opacity, node, "AlphaTestValue", Temp::Destructed);
    }

    // the outputs (held inline: the old one let go before the new one is held)
    NiRefObject* outputs = nullptr;
    NiFixedString type = nullptr;
    NiFixedString nodeName = nullptr;
    memory = NiNew(0x34);
    if (memory != nullptr) {
        type = String("Pixel");
        nodeName = String("PixelOut");
        outputs = ConsumerNode_Construct(memory, &nodeName, &type);
    }
    NiRefObject* old = context.outputs;
    if (old != outputs) {
        if (old != nullptr && ni::sys::Decrement(&old->refCount) == 0) {
            Virtual<void>(old, 1);
        }
        context.outputs = outputs;
        if (outputs != nullptr) {
            ni::sys::Increment(&outputs->refCount);
        }
    }
    if (memory != nullptr) {
        Drop(nodeName);
        Drop(type);
    }
    Resource* color = NamedResource(true, context.outputs, Temp::Inline, "float4", "Color", "", "Color");
    Configurator_AddNode(context.configurator, context.outputs);

    void* technique = At<void*>(ms_pkShadowManager, 0x10 + int(uint8_t(desc->bits[3] >> 24) & 0xF) * 4);
    void* node = GetAttachableNodeFromLibrary(
        self, reinterpret_cast<const re::Ptr32<const char>*>(reinterpret_cast<uintptr_t>(technique) + 0x14));
    Configurator_AddNode(context.configurator, node);
    BindInput(&context, worldView, node, "WorldViewVector");
    BindInput(&context, worldPosProjected, node, "WorldPosProjected");
    NiFixedString output = String("OutputColor");
    Configurator_AddBindingFromOutput(context.configurator, &output, node, color);
    Drop(output);
    NiRefObject* program = Virtual<NiRefObject*>(context.configurator, 2, static_cast<const char*>(name), 1u, uniforms);
    Context_Destruct(&context);
    return program;
}

// ---- construction and destruction

// NiSpotShadowWriteMaterial(library, create caches): "NiSpotShadowWriteMat", versions 2, 0, 3
Self* __fastcall SpotConstruct(Self* self, re::Edx, NiRefObject* library, int autoCreateCaches) {
    NiFixedString name = String("NiSpotShadowWriteMat");
    StandardMaterial_ConstructNamed(self, &name, library, 2, 0, 3, autoCreateCaches);
    Drop(name);
    self->vtable = reinterpret_cast<void*>(NiSpotShadowWriteMaterial_vtable);
    self->writeDescriptorName = String("NiStandardMaterialDescriptor");
    return self;
}

// NiDirectionalShadowWriteMaterial(library, create caches): "NiDirShadowWriteMat", versions 2, 0, 3
Self* __fastcall DirectionalConstruct(Self* self, re::Edx, NiRefObject* library, int autoCreateCaches) {
    NiFixedString name = String("NiDirShadowWriteMat");
    StandardMaterial_ConstructNamed(self, &name, library, 2, 0, 3, autoCreateCaches);
    Drop(name);
    self->vtable = reinterpret_cast<void*>(NiDirectionalShadowWriteMaterial_vtable);
    self->writeDescriptorName = String("NiStandardMaterialDescriptor");
    return self;
}

const void* __fastcall SpotGetRTTI(Self*, re::Edx) {
    return &NiSpotShadowWriteMaterial_RTTI;
}

const void* __fastcall DirectionalGetRTTI(Self*, re::Edx) {
    return &NiDirectionalShadowWriteMaterial_RTTI;
}

// the destructor (the three's): both descriptor names let go, NiFragmentMaterial's destructor
void __fastcall Destruct(Self* self, re::Edx) {
    Drop(self->writeDescriptorName);
    Drop(self->descriptorName);
    FragmentMaterial_Destruct(self);
}

Self* __fastcall DeletingDestructor(Self* self, re::Edx, uint32_t flags) {
    ShadowWriteMaterial_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x3C4);
    }
    return self;
}

// ---- NiStandardMaterial::HandlePositionFragment (slot 36)

// HandlePositionFragment(context, transform type, worldPos&, worldMatrix&): 0 the rigid transform
// (TransformPosition: the local position by the world matrix), 1 the skinned one with bone indices
// (TransformSkinnedPosition: the blend weights and indices, the bone matrices, the skin to world
// transform; the world matrix given back is the node's skin bone transform), 2 the same without
// indices (TransformSkinnedPositionNoIndices). The nodes aren't checked; another type does
// nothing (true, both left null)
uint8_t __fastcall HandlePositionFragment(NiStandardMaterial* self, re::Edx, MaterialContext* context, uint32_t type,
                                          re::Ptr32<Resource>* worldPos, re::Ptr32<Resource>* worldMatrix) {
    *worldPos = nullptr;
    *worldMatrix = nullptr;
    if (type == 0) {
        void* node = LibraryNode(self, "TransformPosition");
        Configurator_AddNode(context->configurator, node);
        NamedResource(false, context->inputs, Temp::Inline, "float3", "Position", "Local", "Position");
        *worldMatrix = PredefinedN(self, context, 5, 4);
        BindNames(context, "Position", context->inputs, "Position", node, Temp::Inline);
        BindByName(context, *worldMatrix, node, "World", Temp::Inline);
        *worldPos = OutputOf(node, "WorldPos");
    } else if (type == 1) {
        void* node = LibraryNode(self, "TransformSkinnedPosition");
        Configurator_AddNode(context->configurator, node);
        NamedResource(false, context->inputs, Temp::Inline, "float3", "Position", "Local", "Position");
        NamedResource(false, context->inputs, Temp::Inline, "float4", "BlendWeight", "Local", "BlendWeights");
        NamedResource(false, context->inputs, Temp::Inline, "int4", "BlendIndices", "Local", "BlendIndices");
        Resource* bones = PredefinedN(self, context, 0x21, 3, 0x1E);
        *worldMatrix = PredefinedN(self, context, 0x60, 4);
        BindNames(context, "Position", context->inputs, "Position", node, Temp::Inline);
        BindNames(context, "BlendWeights", context->inputs, "BlendWeights", node, Temp::Inline);
        BindNames(context, "BlendIndices", context->inputs, "BlendIndices", node, Temp::Inline);
        BindByName(context, bones, node, "Bones", Temp::Inline);
        BindByName(context, *worldMatrix, node, "SkinToWorldTransform", Temp::Inline);
        *worldPos = OutputOf(node, "WorldPos");
        *worldMatrix = OutputOf(node, "SkinBoneTransform");
    } else if (type == 2) {
        void* node = LibraryNode(self, "TransformSkinnedPositionNoIndices", Temp::Destructed);
        Configurator_AddNode(context->configurator, node);
        NamedResource(false, context->inputs, Temp::Destructed, "float3", "Position", "Local", "Position");
        NamedResource(false, context->inputs, Temp::Destructed, "float4", "BlendWeight", "Local", "BlendWeights");
        Resource* bones = PredefinedN(self, context, 0x21, 3, 4);
        *worldMatrix = PredefinedN(self, context, 0x60, 4);
        BindNames(context, "Position", context->inputs, "Position", node, Temp::Destructed);
        BindNames(context, "BlendWeights", context->inputs, "BlendWeights", node, Temp::Destructed);
        BindByName(context, bones, node, "Bones", Temp::Destructed);
        BindByName(context, *worldMatrix, node, "SkinToWorldTransform", Temp::Destructed);
        *worldPos = OutputOf(node, "WorldPos", Temp::Destructed);
        *worldMatrix = OutputOf(node, "SkinBoneTransform", Temp::Destructed);
    }
    return 1;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISHADOWWRITE2_REVERSED(name, address, fn)                                             \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISHADOWWRITE2_REVERSED("NiPointShadowWriteMaterial::SetupPackingRequirements", 0x007ADD50,
                        &ni::SetupPackingRequirements);
NISHADOWWRITE2_REVERSED("NiPointShadowWriteMaterial::HandleViewProjectionFragment", 0x007ADDF0,
                        &ni::HandleViewProjectionFragment);
NISHADOWWRITE2_REVERSED("NiPointShadowWriteMaterial::GetVertexInputSemantics", 0x007AE420,
                        &ni::GetVertexInputSemantics);
NISHADOWWRITE2_REVERSED("NiSpotShadowWriteMaterial::NiSpotShadowWriteMaterial", 0x007AE4D0, &ni::SpotConstruct);
NISHADOWWRITE2_REVERSED("NiSpotShadowWriteMaterial::GetRTTI", 0x007AE560, &ni::SpotGetRTTI);
NISHADOWWRITE2_REVERSED("NiPointShadowWriteMaterial::~NiPointShadowWriteMaterial", 0x007AE570, &ni::Destruct);
NISHADOWWRITE2_REVERSED("NiPointShadowWriteMaterial::DeletingDestructor", 0x007AE5F0, &ni::DeletingDestructor);
NISHADOWWRITE2_REVERSED("NiPointShadowWriteMaterial::GenerateDescriptor", 0x007AE620, &ni::GenerateDescriptor);
NISHADOWWRITE2_REVERSED("NiDirectionalShadowWriteMaterial::GeneratePixelShader", 0x007AE8F0,
                        &ni::DirectionalGeneratePixelShader);
NISHADOWWRITE2_REVERSED("NiPointShadowWriteMaterial::SetupTransformPipeline", 0x007AF4D0,
                        &ni::SetupTransformPipeline);
NISHADOWWRITE2_REVERSED("NiStandardMaterial::HandlePositionFragment", 0x007AF6F0, &ni::HandlePositionFragment);
NISHADOWWRITE2_REVERSED("NiPointShadowWriteMaterial::HandleViewVectorFragment", 0x007B0380,
                        &ni::HandleViewVectorFragment);
NISHADOWWRITE2_REVERSED("NiDirectionalShadowWriteMaterial::NiDirectionalShadowWriteMaterial", 0x007B0650,
                        &ni::DirectionalConstruct);
NISHADOWWRITE2_REVERSED("NiDirectionalShadowWriteMaterial::GetRTTI", 0x007B06E0, &ni::DirectionalGetRTTI);
