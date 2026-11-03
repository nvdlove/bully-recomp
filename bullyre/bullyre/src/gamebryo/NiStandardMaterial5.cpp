// NiStandardMaterial's vertex and pixel program builders (slots 21 and 23; src/gamebryo/
// NiFragmentMaterial.h, NiShadeTree.h): the program cache's program of the descriptor's name, or
// one built from the shade tree its handlers make (a configurator, the context's producer and
// consumer nodes, the fragments bound), evaluated by the configurator. A failure is told to the
// cache (SetFailedGPUProgram) and the context let go.
//
// The vertex descriptor's words as used here: V0 (bits[0]) the skinning (0..1), the normals (6..7),
// the fog (9..10), the environment texgen (11..13), the projected lights' count and clipped mask
// (14..15, 16..18) and the projected shadows' (19..20, 21..23), the output UV count (24..27); V2
// (bits[2]) the light counts (12..15 point, 16..19 spot, 20..23 directional), vertex colors (24),
// the material colors computed (25), the vertex color mode (26..27), lighting mode (28..29),
// vertex lighting (30). The pixel descriptor's: P0 the varyings (1 world position, 2..3 normal, 3
// binormal and tangent, 4 view vector, 7 tangent view, which is also the parallax map) and
// lighting (0); P1 per-vertex lighting (11); P2/P3 the light counts (P2 28..31, P3 0..3, 4..7),
// P3 the shadow mask (8..15), specular (16), vertex color mode (17..18), lighting mode (19..20),
// ambient (21), emissive (23).

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

using Self = NiStandardMaterial;
using Resource = void;

// ---- slot 21: the vertex program

NiRefObject* __fastcall GenerateVertexShader(Self* self, re::Edx, NiMaterialDescriptor* desc, void* uniforms) {
    char name[0x104];
    uint32_t fogType = 0;
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
    NiString description;
    void* configurator = context.configurator;
    Configurator_SetDescription(configurator, NiString_GetChars(VertexDesc_ToString(desc, &description)));
    NiString_Destruct(&description);

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

    Resource* worldNormal = nullptr;
    Resource* worldView = nullptr;
    Resource* worldPos = nullptr;
    Resource* reflect = nullptr;
    Resource* viewPos = nullptr;
    uint32_t d2 = desc->bits[2];
    uint32_t spotCount = (d2 >> 16) & 0xF;
    uint32_t pointCount = (d2 >> 12) & 0xF;
    uint32_t directionalCount = (d2 >> 20) & 0xF;
    uint32_t d0 = desc->bits[0];
    uint8_t specularOn = (d0 & 0x100) == 0x100 ? 1 : 0;
    uint32_t lights = spotCount + directionalCount + pointCount;
    bool materialColors = (desc->bits[2] & 0x2000000) == 0x2000000;
    uint32_t texGen = (d0 >> 11) & 7;
    uint8_t forceWorldView = 0;
    uint8_t forceViewPos = 0;
    if (texGen == 3 || texGen == 5 || (lights != 0 && specularOn != 0)) {
        forceWorldView = 1;
    }
    fogType = (d0 >> 9) & 3;
    if (fogType != 0) {
        forceViewPos = 1;
    }

    // slot 42 SetupTransformPipeline
    if (!Virtual<uint8_t>(self, 42, &context, projected, desc, uint32_t(forceWorldView), uint32_t(forceViewPos),
                          &worldPos, &viewPos, &worldNormal, &worldView)) {
        return fail();
    }
    if ((texGen == 3 || texGen == 5) &&
        !Virtual<uint8_t>(self, 41, &context, worldNormal, 1u, worldView, &reflect)) {   // HandleReflectionVector
        return fail();
    }

    d2 = desc->bits[2];
    uint32_t lightingMode = (d2 >> 28) & 3;
    uint32_t vertexColorMode = (d2 >> 26) & 3;
    if ((d2 & 0x40000000) != 0) {
        if (lights == 0 && !materialColors) {
            // the vertex colors passed on as they are
            if ((uint8_t(desc->bits[2] >> 24) & 1) != 0 &&
                (vertexColorMode == 1 || (vertexColorMode == 2 && lightingMode == 1))) {
                Resource* colors =
                    NamedResource(false, context.inputs, Temp::Constructed, "float4", "Color", "", "VertexColors");
                NiFixedString variable;
                FixedString_Construct(&variable, "VertexColors");
                void* outputs = context.outputs;
                const re::Ptr32<const char>* label = Resource_GetLabel(colors);
                const re::Ptr32<const char>* semantic = Resource_GetSemantic(colors);
                const re::Ptr32<const char>* type = Resource_GetType(colors);
                Resource* input = Node_AddInputResource(outputs, type, semantic, label, &variable, 1, 0, 0, 0);
                FixedString_Destruct(&variable);
                Configurator_AddBinding(context.configurator, colors, input);
            }
        } else {
            // per-vertex lighting
            Resource* diffuse = nullptr;
            Resource* specular = nullptr;
            Resource* specularPower = nullptr;
            Resource* ambient = nullptr;
            Resource* emissive = nullptr;
            Resource* opacity = nullptr;
            if (materialColors &&
                !Virtual<uint8_t>(self, 43, &context, uint32_t(specularOn), vertexColorMode, lightingMode, &diffuse,
                                  &specular, &specularPower, &ambient, &emissive, &opacity)) {   // HandleMaterialColors
                return fail();
            }
            Resource* lightDiffuse = nullptr;
            Resource* lightSpecular = nullptr;
            Resource* lightAmbient = nullptr;
            if (lightingMode == 1) {
                lightAmbient = Predefined(self, &context, 0x57);
                if (worldNormal != nullptr && !Virtual<uint8_t>(self, 70, &context, &worldNormal)) {   // NormalizeVector
                    return fail();
                }
                if (worldView != nullptr && !Virtual<uint8_t>(self, 70, &context, &worldView)) {
                    return fail();
                }
                // HandleLights(context, specular, point, directional, spot, shadow mask, 0, ...)
                if (!Virtual<uint8_t>(self, 65, &context, uint32_t(specularOn), pointCount, directionalCount, spotCount,
                                      0u, 0u, worldPos, worldNormal, worldView, specularPower, &lightAmbient,
                                      &lightDiffuse, &lightSpecular)) {
                    return fail();
                }
            }
            if (materialColors) {
                Resource* diffuseCoefficient = nullptr;
                Resource* specularCoefficient = nullptr;
                if (!Virtual<uint8_t>(self, 49, &context, emissive, diffuse, ambient, specular, lightSpecular,
                                      lightDiffuse, lightAmbient, &diffuseCoefficient,
                                      &specularCoefficient)) {   // HandleShadingCoefficients
                    return fail();
                }
                void* node = LibraryNode(self, "CompositeFinalRGBAColor", Temp::Constructed);
                Configurator_AddNode(context.configurator, node);
                BindByName(&context, diffuseCoefficient, node, "FinalColor", Temp::Constructed);
                if (opacity != nullptr) {
                    BindByName(&context, opacity, node, "FinalOpacity", Temp::Constructed);
                }
                Resource* color = OutputOf(node, "OutputColor", Temp::Constructed);
                Resource* input = InputFor(context.outputs, Temp::Constructed, color, "TexCoord", "DiffuseAccum");
                Configurator_AddBinding(context.configurator, color, input);
                if (specularOn != 0) {
                    input = InputFor(context.outputs, Temp::Constructed, specularCoefficient, "TexCoord",
                                     "SpecularAccum");
                    Configurator_AddBinding(context.configurator, specularCoefficient, input);
                }
            } else {
                if (lightDiffuse != nullptr) {
                    Resource* input =
                        InputFor(context.outputs, Temp::Constructed, lightDiffuse, "TexCoord", "DiffuseAccum");
                    Configurator_AddBinding(context.configurator, lightDiffuse, input);
                }
                if (lightAmbient != nullptr) {
                    Resource* input =
                        InputFor(context.outputs, Temp::Constructed, lightAmbient, "TexCoord", "AmbientAccum");
                    Configurator_AddBinding(context.configurator, lightAmbient, input);
                }
                if (lightSpecular != nullptr) {
                    Resource* input =
                        InputFor(context.outputs, Temp::Constructed, lightSpecular, "TexCoord", "SpecularAccum");
                    Configurator_AddBinding(context.configurator, lightSpecular, input);
                }
            }
        }
    }

    // the texture coordinates: the UV sets (slot 45), the generated ones (slot 46), each an output
    re::Ptr32<Resource> coordinates[12];
    crt::Memset(coordinates, 0, sizeof(coordinates));
    uint32_t count = 0;
    if (!Virtual<uint8_t>(self, 45, &context, desc, static_cast<re::Ptr32<Resource>*>(coordinates), 0xCu, &count)) {
        return fail();
    }
    d0 = desc->bits[0];
    if (!Virtual<uint8_t>(self, 46, &context, static_cast<re::Ptr32<Resource>*>(coordinates), 0xCu, worldPos, worldNormal,
                          reflect, texGen, (d0 >> 14) & 3, (d0 >> 16) & 7, (d0 >> 19) & 3, (d0 >> 21) & 7,
                          &count)) {
        return fail();
    }
    for (uint32_t i = 0; i < count; i++) {
        NiFixedString label;
        FixedString_Construct(&label, "");
        NiFixedString semantic;
        FixedString_Construct(&semantic, "TexCoord");
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
    if (!Virtual<uint8_t>(self, 44, &context, viewPos, fogType)) {   // HandleVertexFog
        return fail();
    }
    NiRefObject* program = Virtual<NiRefObject*>(context.configurator, 2, static_cast<const char*>(name), 0u, uniforms);
    if (program == nullptr) {
        SetFailedGPUProgram(self, 0, desc);
    }
    Context_Destruct(&context);
    return program;
}

// ---- slot 23: the pixel program

NiRefObject* __fastcall GeneratePixelShader(Self* self, re::Edx, NiMaterialDescriptor* desc, void* uniforms) {
    char name[0x104];
    Descriptor_ToString(desc, name, 0x30);
    NiRefObject* cache = self->programCaches[1];
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
        SetFailedGPUProgram(self, 1, desc);
        Context_Destruct(&context);
        return nullptr;
    };

    void* memory = NiNew(0x34);
    Pointer_Assign(&context.configurator,
                   memory != nullptr ? Configurator_Construct(memory, self->programCaches[1]) : nullptr);
    NiString description;
    void* configurator = context.configurator;
    Configurator_SetDescription(configurator, NiString_GetChars(PixelDesc_ToString(desc, &description)));
    NiString_Destruct(&description);

    NewContextNode(ProducerNode_Construct, Temp::Constructed, "Uniform", "Uniforms", &context.uniforms);
    Configurator_AddNode(context.configurator, context.uniforms);
    NewContextNode(ProducerNode_Construct, Temp::Constructed, "Static", "Statics", &context.statics);
    Configurator_AddNode(context.configurator, context.statics);
    // the inputs: what the vertex program put out
    NewContextNode(ProducerNode_Construct, Temp::Constructed, "Vertex", "VertexOut", &context.inputs);
    NamedResource(false, context.inputs, Temp::Destructed, "float4", "Position", "World", "PosProjected");
    Configurator_AddNode(context.configurator, context.inputs);
    NewContextNode(ConsumerNode_Construct, Temp::Constructed, "Pixel", "PixelOut", &context.outputs);
    Resource* color0 = NamedResource(true, context.outputs, Temp::Destructed, "float4", "Color", "", "Color0");
    Configurator_AddNode(context.configurator, context.outputs);

    // the varyings asked for (the vectors normalized again)
    Resource* worldPos = nullptr;
    Resource* worldNormal = nullptr;
    Resource* worldBinormal = nullptr;
    Resource* worldTangent = nullptr;
    Resource* worldView = nullptr;
    Resource* tangentView = nullptr;
    if ((uint8_t(desc->bits[0]) & 2) != 0) {
        worldPos = NamedResource(false, context.inputs, Temp::Constructed, "float4", "TexCoord", "", "WorldPos");
    }
    if ((desc->bits[0] & 0xC) != 0) {
        worldNormal = NamedResource(false, context.inputs, Temp::Destructed, "float3", "TexCoord", "", "WorldNormal");
        if (!Virtual<uint8_t>(self, 70, &context, &worldNormal)) {   // NormalizeVector
            return fail();
        }
    }
    if ((uint8_t(desc->bits[0]) & 8) != 0) {
        worldBinormal =
            NamedResource(false, context.inputs, Temp::Constructed, "float3", "TexCoord", "", "WorldBinormal");
        worldTangent =
            NamedResource(false, context.inputs, Temp::Constructed, "float3", "TexCoord", "", "WorldTangent");
        if (!Virtual<uint8_t>(self, 70, &context, &worldBinormal) ||
            !Virtual<uint8_t>(self, 70, &context, &worldTangent)) {
            return fail();
        }
    }
    if ((uint8_t(desc->bits[0]) & 0x10) != 0) {
        worldView = NamedResource(false, context.inputs, Temp::Constructed, "float3", "TexCoord", "", "WorldView");
        if (!Virtual<uint8_t>(self, 70, &context, &worldView)) {
            return fail();
        }
    }
    if ((uint8_t(desc->bits[0]) & 0x80) != 0) {
        tangentView =
            NamedResource(false, context.inputs, Temp::Constructed, "float3", "TexCoord", "", "TangentSpaceView");
        if (!Virtual<uint8_t>(self, 70, &context, &tangentView)) {
            return fail();
        }
    }

    uint32_t p3 = desc->bits[3];
    uint32_t directionalCount = (p3 >> 4) & 0xF;
    uint32_t spotCount = p3 & 0xF;
    uint32_t pointCount = desc->bits[2] >> 28;
    uint32_t lights = directionalCount + spotCount + pointCount;
    uint32_t lightingMode = (p3 >> 19) & 3;
    uint8_t specularOn = uint8_t(p3 >> 16) & 1;
    uint32_t vertexColorMode = (p3 >> 17) & 3;
    uint8_t perVertex = (desc->bits[1] & 0x800) == 0x800 ? 1 : 0;
    Resource* diffuseColor = nullptr;     // the lit color (its specular part below)
    Resource* specularColor = nullptr;
    Resource* opacity = nullptr;
    Resource* lightDiffuse = nullptr;
    Resource* lightSpecular = nullptr;
    Resource* lightAmbient = nullptr;
    Resource* diffuse = nullptr;
    Resource* specular = nullptr;
    Resource* specularPower = nullptr;
    Resource* gloss = nullptr;
    Resource* ambient = nullptr;
    Resource* emissive = nullptr;
    Resource* textureColor = nullptr;
    Resource* unused = nullptr;
    // (the texture count's slot is written by slot 48 before it's read)
    uint32_t textures = 0;

    if ((uint8_t(desc->bits[0]) & 1) != 0) {
        if (perVertex != 0) {
            // the vertex program's light sums
            lightDiffuse =
                NamedResource(false, context.inputs, Temp::Constructed, "float4", "TexCoord", "", "DiffuseAccum");
            if (!Virtual<uint8_t>(self, 80, &context, lightDiffuse, &lightDiffuse, &opacity)) {   // SplitColorAndOpacity
                return fail();
            }
            if (lights != 0) {
                lightAmbient =
                    NamedResource(false, context.inputs, Temp::Constructed, "float4", "TexCoord", "", "AmbientAccum");
            }
            if (specularOn != 0) {
                lightSpecular = NamedResource(false, context.inputs, Temp::Constructed, "float3", "TexCoord", "",
                                              "SpecularAccum");
            }
        } else {
            uint8_t useEmissive = uint8_t(desc->bits[3] >> 23) & 1;
            if (((desc->bits[3] >> 21) & 1) != 0) {
                lightAmbient = Predefined(self, &context, 0x57);
            }
            if (!Virtual<uint8_t>(self, 43, &context, uint32_t(specularOn), vertexColorMode, lightingMode, &diffuse,
                                  &specular, &specularPower, &ambient, &emissive, &opacity)) {   // HandleMaterialColors
                return fail();
            }
            if (useEmissive == 0) {
                emissive = nullptr;
            }
        }
    }

    // the texture coordinates: the UV sets, then the generated ones (environment, projections)
    re::Ptr32<Resource> coordinates[12];
    uint32_t setCount = 0;
    for (; setCount < PixelDesc_GetInputUVCount(desc); setCount++) {
        NiFixedString label = String("");
        NiFixedString semantic = String("TexCoord");
        NiFixedString type = String("float2");
        void* inputs = context.inputs;
        NiFixedString variable;
        Virtual<re::Ptr32<const char>*>(self, 29, &variable, setCount);   // GenerateUVSetName
        coordinates[setCount] = Node_AddOutputResource(inputs, &type, &semantic, &label, &variable, 1, 0, 0, 0);
        FixedString_Destruct(&variable);
        FixedString_Destruct(&type);
        FixedString_Destruct(&semantic);
        FixedString_Destruct(&label);
    }
    uint32_t uvSets = setCount;
    uint32_t p0 = desc->bits[0];
    uint32_t generated = ((desc->bits[1] >> 3) & 3) + ((p0 >> 27) & 3);
    if ((p0 & 0x7000000) != 0 && (p0 & 0x200) == 0) {
        generated++;
    }
    uint32_t index = setCount;
    for (uint32_t i = generated; i != 0; i--, index++) {
        NiFixedString label = String("");
        NiFixedString semantic = String("TexCoord");
        NiFixedString type = String("float4");
        void* inputs = context.inputs;
        NiFixedString variable;
        Virtual<re::Ptr32<const char>*>(self, 29, &variable, index);
        coordinates[index] = Node_AddOutputResource(inputs, &type, &semantic, &label, &variable, 1, 0, 0, 0);
        Drop(variable);
        Drop(type);
        Drop(semantic);
        Drop(label);
    }

    // slot 48 HandleTextureMaps (the parallax, normal, dark, base, detail, decal, custom, projected
    // and gloss maps)
    if (!Virtual<uint8_t>(self, 48, &context, desc, &worldPos, &worldNormal, &worldBinormal, &worldTangent, &worldView,
                          &tangentView, &diffuse, &specular, &specularPower, &gloss, &ambient, &emissive, &opacity,
                          &textureColor, &unused, &textures, static_cast<re::Ptr32<Resource>*>(coordinates), uvSets,
                          generated)) {
        return fail();
    }
    // HandleLights(context, specular, point, directional, spot, shadow mask, shadow technique, ...)
    if (!Virtual<uint8_t>(self, 65, &context, uint32_t(specularOn), pointCount, directionalCount, spotCount,
                          uint32_t(uint8_t(desc->bits[3] >> 8)), uint32_t(uint8_t(desc->bits[3] >> 24) & 0xF), worldPos,
                          worldNormal, worldView, specularPower, &lightAmbient, &lightDiffuse, &lightSpecular)) {
        return fail();
    }

    // the lit color: the shading coefficients (or the vertex sums as they are) times the texture
    Resource* litDiffuse = nullptr;
    Resource* litSpecular = nullptr;
    if ((uint8_t(desc->bits[0]) & 1) == 0) {
        litDiffuse = nullptr;
        if (textureColor == nullptr) {
            return fail();
        }
        diffuseColor = textureColor;
    } else {
        if (perVertex != 0 && lights == 0) {
            litDiffuse = lightDiffuse;
            litSpecular = lightSpecular;
        } else {
            if (lightingMode == 0) {
                lightDiffuse = nullptr;
                lightAmbient = nullptr;
            }
            if (!Virtual<uint8_t>(self, 49, &context, emissive, diffuse, ambient, specular, lightSpecular, lightDiffuse,
                                  lightAmbient, &litDiffuse, &litSpecular)) {   // HandleShadingCoefficients
                return fail();
            }
        }
        if (litDiffuse == nullptr) {
            if (textureColor == nullptr) {
                return fail();
            }
            diffuseColor = textureColor;
        } else if (textureColor == nullptr) {
            diffuseColor = litDiffuse;
        } else {
            if (!Virtual<uint8_t>(self, 72, &context, litDiffuse, textureColor, &diffuseColor)) {   // MultiplyVector
                return fail();
            }
            if (self->unknown3BE != 0 &&
                !Virtual<uint8_t>(self, 76, &context, diffuseColor, &diffuseColor)) {   // SaturateVector
                return fail();
            }
        }
    }
    specularColor = litSpecular;
    if (gloss != nullptr && litSpecular != nullptr &&
        !Virtual<uint8_t>(self, 72, &context, litSpecular, gloss, &specularColor)) {
        return fail();
    }
    // slot 50 HandleEffectMaps (the bump, environment and glow maps)
    if (!Virtual<uint8_t>(self, 50, &context, desc, &worldNormal, worldView, &opacity, &diffuseColor, &specularColor,
                          gloss, &textures, static_cast<re::Ptr32<Resource>*>(coordinates), uvSets, generated)) {
        return fail();
    }

    Resource* color = nullptr;
    void* rgb = LibraryNode(self, "CompositeFinalRGBColor", Temp::Constructed);
    Configurator_AddNode(context.configurator, rgb);
    if (diffuseColor != nullptr) {
        BindByName(&context, diffuseColor, rgb, "DiffuseColor", Temp::Constructed);
    }
    if (specularColor != nullptr) {
        BindByName(&context, specularColor, rgb, "SpecularColor", Temp::Constructed);
    }
    color = OutputOf(rgb, "OutputColor", Temp::Constructed);
    if (!Virtual<uint8_t>(self, 68, &context, desc, color, &color)) {   // HandlePixelFog
        return fail();
    }
    void* rgba = LibraryNode(self, "CompositeFinalRGBAColor", Temp::Constructed);
    Configurator_AddNode(context.configurator, rgba);
    BindByName(&context, color, rgba, "FinalColor", Temp::Constructed);
    if (opacity != nullptr) {
        Virtual<uint8_t>(self, 69, &context, desc, opacity);   // HandleAlphaTest (its result not looked at)
        BindByName(&context, opacity, rgba, "FinalOpacity", Temp::Constructed);
    }
    NiFixedString output;
    FixedString_Construct(&output, "OutputColor");
    Configurator_AddBindingFromOutput(context.configurator, &output, rgba, color0);
    FixedString_Destruct(&output);
    NiRefObject* program = Virtual<NiRefObject*>(context.configurator, 2, static_cast<const char*>(name), 1u, uniforms);
    if (program == nullptr) {
        SetFailedGPUProgram(self, 1, desc);
    }
    Context_Destruct(&context);
    return program;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISTDMAT5_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISTDMAT5_REVERSED("NiStandardMaterial::GenerateVertexShader", 0x00791150, &ni::GenerateVertexShader);
NISTDMAT5_REVERSED("NiStandardMaterial::GeneratePixelShader", 0x00791D90, &ni::GeneratePixelShader);
