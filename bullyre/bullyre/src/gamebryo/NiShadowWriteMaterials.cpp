// The shadow write materials (src/gamebryo/NiFragmentMaterial.h, NiShadeTree.h): the materials
// the shadow maps are rendered with, NiStandardMaterials of 0x3C4 bytes (vtables
// NiPointShadowWriteMaterial 0x95F8D8, NiSpotShadowWriteMaterial 0x95FA50,
// NiDirectionalShadowWriteMaterial 0x95FBB8; their own descriptor name at +0x3C0) over their
// node libraries (src/gamebryo/NiMaterialLibraries.cpp). Their render pass descriptions (one
// alpha-overridden pass with the alpha test and the base map's coordinates), their programs
// (the vertex one shared: position, view vector and UV sets; the pixel one the shadow technique's
// write fragment, with the alpha test when the base map's alpha is wanted), their shader (the
// renderer's slot 78).

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

constexpr uintptr_t NiPointShadowWriteMaterial_vtable = 0x0095F8D8;
GAME_VAR(uint8_t, NiPointShadowWriteMaterial_RTTI, 0x020E36CC);

GAME_FN(NiStandardMaterial*, __thiscall, StandardMaterial_ConstructNamed, 0x00797F30, void* self,
        const re::Ptr32<const char>* name, NiRefObject* library, uint32_t vertexVersion, uint32_t geometryVersion,
        uint32_t pixelVersion, int autoCreateCaches);

// slot 27 CreateShader(descriptor): the renderer's (its slot 78)
void* __fastcall CreateShader(Self*, re::Edx, NiMaterialDescriptor* desc) {
    void* renderer = ms_pkRenderer;
    if (renderer == nullptr) {
        return nullptr;
    }
    return Virtual<void*>(renderer, 78, desc);
}

// slot 21 (the three materials'): the vertex program, as NiStandardMaterial's but for the world
// view forced, and no lighting, fog or generated coordinates
NiRefObject* __fastcall GenerateVertexShader(Self* self, re::Edx, NiMaterialDescriptor* desc, void* uniforms) {
    char name[0x104];
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
    auto fail = [&]() -> NiRefObject* {
        SetFailedGPUProgram(self, 0, desc);
        Context_Destruct(&context);
        return nullptr;
    };
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
    Resource* projected =
        NamedResource(true, context.outputs, Temp::Destructed, "float4", "Position", "World", "PosProjected");
    Configurator_AddNode(context.configurator, context.outputs);

    Resource* worldPos = nullptr;
    Resource* viewPos = nullptr;
    Resource* worldNormal = nullptr;
    Resource* worldView = nullptr;
    if (!Virtual<uint8_t>(self, 42, &context, projected, desc, 1u, 0u, &worldPos, &viewPos, &worldNormal,
                          &worldView)) {   // SetupTransformPipeline
        return fail();
    }
    re::Ptr32<Resource> coordinates[12];
    crt::Memset(coordinates, 0, sizeof(coordinates));
    uint32_t count = 0;
    if (!Virtual<uint8_t>(self, 45, &context, desc, static_cast<re::Ptr32<Resource>*>(coordinates), 0xCu,
                          &count)) {   // HandleTextureUVSets
        return fail();
    }
    for (uint32_t i = 0; i < count; i++) {
        NiFixedString label = String("");
        NiFixedString semantic = String("TexCoord");
        void* outputs = context.outputs;
        NiFixedString variable;
        Virtual<re::Ptr32<const char>*>(self, 29, &variable, i);   // GenerateUVSetName
        Resource* input = Node_AddInputResource(outputs, Resource_GetType(coordinates[i]), &semantic, &label,
                                                &variable, 1, 0, 0, 0);
        FixedString_Destruct(&variable);
        FixedString_Destruct(&semantic);
        FixedString_Destruct(&label);
        Configurator_AddBinding(context.configurator, coordinates[i], input);
    }
    NiRefObject* program = Virtual<NiRefObject*>(context.configurator, 2, static_cast<const char*>(name), 0u, uniforms);
    if (program == nullptr) {
        SetFailedGPUProgram(self, 0, desc);
    }
    Context_Destruct(&context);
    return program;
}

// slot 23 of the point and spot materials: the pixel program, the shadow technique's write fragment
// for the light (its +0x18 for a point light, +0x1C for a spot light) with the view vector (and
// the projected world position for a spot light), the alpha test on the base map's alpha when the
// descriptor asks for both. (The pixel cache isn't checked for null, and a failure isn't told to
// it, as the game has it)
NiRefObject* ShadowPixelShader(Self* self, NiMaterialDescriptor* desc, void* uniforms, bool spot) {
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
    Resource* worldPosProjected = nullptr;
    if (spot) {
        worldPosProjected = AddResource(context.inputs, "float4", "TexCoord", "World", "WorldPosProjected");
    }
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
        self, reinterpret_cast<const re::Ptr32<const char>*>(reinterpret_cast<uintptr_t>(technique) + (spot ? 0x1C : 0x18)));
    Configurator_AddNode(context.configurator, node);
    BindInput(&context, worldView, node, "WorldViewVector");
    if (spot) {
        BindInput(&context, worldPosProjected, node, "WorldPosProjected");
    }
    NiFixedString output = String("OutputColor");
    Configurator_AddBindingFromOutput(context.configurator, &output, node, color);
    Drop(output);
    NiRefObject* program = Virtual<NiRefObject*>(context.configurator, 2, static_cast<const char*>(name), 1u, uniforms);
    Context_Destruct(&context);
    return program;
}

NiRefObject* __fastcall PointGeneratePixelShader(Self* self, re::Edx, NiMaterialDescriptor* desc, void* uniforms) {
    return ShadowPixelShader(self, desc, uniforms, false);
}

NiRefObject* __fastcall SpotGeneratePixelShader(Self* self, re::Edx, NiMaterialDescriptor* desc, void* uniforms) {
    return ShadowPixelShader(self, desc, uniforms, true);
}

// slot 24 (the three materials'): one pass, alpha overridden without blending, its pixel program's
// lighting flag, base map, alpha test and shadow technique's bits (the technique not checked for),
// the skinning, the texture coordinates assigned
uint32_t __fastcall GenerateShaderDescArray(Self*, re::Edx, NiMaterialDescriptor* desc, RenderPassDescriptor* passes,
                                            uint32_t, uint32_t* count) {
    *count = 0;
    if (!FixedString_Equals(&desc->identifier, "NiStandardMaterialDescriptor")) {
        return 8;
    }
    RenderPassDescriptor* pass = passes;
    pass->usesNiRenderState = 1;
    pass->alphaOverride = 1;
    pass->alphaBlend = 0;
    pass->resetObjectOffsets = 1;
    FixedString_AssignText(&pass->vertexDesc.identifier, "NiStandardVertexProgramDescriptor");
    NiMaterialDescriptor* pixel = &pass->pixelDesc;
    FixedString_AssignText(&pixel->identifier, "NiStandardPixelProgramDescriptor");
    const uint32_t* m = desc->bits;
    uint32_t* p = pixel->bits;
    uint32_t* v = pass->vertexDesc.bits;
    p[0] ^= ((m[0] >> 9) ^ p[0]) & 1;
    p[0] ^= ((m[0] >> 8) ^ p[0]) & 0x100;
    p[3] ^= ((m[3] >> 2) ^ p[3]) & 0x10000000;
    void* technique = At<void*>(ms_pkShadowManager, int((m[3] >> 28) & 3) * 4);
    p[3] = (p[3] & 0xF0FFFFFF) | (uint32_t(At<uint16_t>(technique, 0x46)) << 24);
    v[0] ^= (v[0] ^ m[0]) & 3;
    uint32_t uvSets[16];
    uint32_t texGens[16];
    crt::Memset(uvSets, 0xFF, sizeof(uvSets));
    crt::Memset(texGens, 0, sizeof(texGens));
    uint32_t textures = StandardDesc_GetInputCount(desc);
    for (uint32_t i = 0; i < textures; i++) {
        StandardDesc_GetTextureUsage(desc, i, &uvSets[i], &texGens[i]);
    }
    AssignTextureCoordinates(uvSets, texGens, textures, &pass->vertexDesc, pixel);
    *count = *count + 1;
    return 0;
}

// NiPointShadowWriteMaterial(library, create caches): "NiPointShadowWriteMat", versions 2, 0, 3
Self* __fastcall PointConstruct(Self* self, re::Edx, NiRefObject* library, int autoCreateCaches) {
    NiFixedString name = String("NiPointShadowWriteMat");
    StandardMaterial_ConstructNamed(self, &name, library, 2, 0, 3, autoCreateCaches);
    Drop(name);
    self->vtable = reinterpret_cast<void*>(NiPointShadowWriteMaterial_vtable);
    self->writeDescriptorName = String("NiStandardMaterialDescriptor");
    return self;
}

const void* __fastcall PointGetRTTI(Self*, re::Edx) {
    return &NiPointShadowWriteMaterial_RTTI;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISHADOWWRITE_REVERSED(name, address, fn)                                              \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISHADOWWRITE_REVERSED("NiPointShadowWriteMaterial::CreateShader", 0x007AC030, &ni::CreateShader);
NISHADOWWRITE_REVERSED("NiPointShadowWriteMaterial::GeneratePixelShader", 0x007AC050, &ni::PointGeneratePixelShader);
NISHADOWWRITE_REVERSED("NiPointShadowWriteMaterial::GenerateVertexShader", 0x007ACAC0, &ni::GenerateVertexShader);
NISHADOWWRITE_REVERSED("NiPointShadowWriteMaterial::NiPointShadowWriteMaterial", 0x007ACF80, &ni::PointConstruct);
NISHADOWWRITE_REVERSED("NiPointShadowWriteMaterial::GetRTTI", 0x007AD010, &ni::PointGetRTTI);
NISHADOWWRITE_REVERSED("NiSpotShadowWriteMaterial::GeneratePixelShader", 0x007AD020, &ni::SpotGeneratePixelShader);
NISHADOWWRITE_REVERSED("NiPointShadowWriteMaterial::GenerateShaderDescArray", 0x007ADC00,
                       &ni::GenerateShaderDescArray);
