// NiStandardMaterial's shade tree handlers (src/gamebryo/NiFragmentMaterial.h): each adds the
// fragments of one feature (normals, view and reflection vectors, material colors, texture
// coordinate generation, shading coefficients, the texture maps, projected lights and shadows,
// the lights) to the configurator and binds them, returning the resources the next ones take.
//
// The nodes come from the libraries by name (slot 35, not checked for null where the game
// doesn't), uniforms from slots 31 (predefined mappings), 32 (object mappings) and 30 (texture
// samplers); texture maps are inserted by slot 79 (by map) and 78 (by map name).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiFragmentMaterial.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::frag;

using Self = NiStandardMaterial;
using Resource = void;

GAME_VAR(void*, NiShadowManager_ms_pkShadowManager, 0x020E2410);
// a texture effect type's map name (0 the projected light, 1 the projected shadow, 2 the
// environment map; NiPixelData.cpp registers the function as NiPixelFormat::StaticEntry)
GAME_FN(const re::Ptr32<const char>*, __cdecl, TextureEffectMapName, 0x0076BA30, int index);
GAME_FN(const char*, __cdecl, AttributeDesc_GetObjectTypeName, 0x00784BE0, uint32_t type);

// slot 31 AddOutputPredefined(node, mapping, registers, count, extra) on the uniforms
inline Resource* Predefined(Self* self, MaterialContext* context, uint32_t mapping) {
    return Virtual<Resource*>(self, 31, context->uniforms, mapping, 1u, 1u, 0u);
}

// a constant of the statics: the value made first, the type let go first
inline Resource* Constant(MaterialContext* context, const char* type, const char* value) {
    NiFixedString v = String(value);
    NiFixedString t = String(type);
    Resource* constant = Node_AddOutputConstant(context->statics, &t, &v);
    Drop(t);
    Drop(v);
    return constant;
}

// the configurator's binding to a node's input by name
inline void BindByName(MaterialContext* context, Resource* output, void* node, const char* input) {
    NiFixedString s = String(input);
    Configurator_AddBindingByName(context->configurator, output, &s, node);
    Drop(s);
}

// slot 79 InsertTexture(context, map, occurrence (overwritten), apply, sample, uv, input, scale,
// color&, opacity&) and slot 78, the same by the map's name
inline uint8_t InsertTextureMap(Self* self, MaterialContext* context, uint32_t map, uint32_t apply, uint32_t sample,
                                Resource* uv, Resource* input, Resource* scale, re::Ptr32<Resource>* color, re::Ptr32<Resource>* opacity) {
    return Virtual<uint8_t>(self, 79, context, map, 0u, apply, sample, uv, input, scale, color, opacity);
}

// ---- the vertex side

// slot 37 HandleNormals(context, kind (0 none, 1 normals, 2 normals binormals and tangents),
// world matrix, worldNormal&, worldBinormal&, worldTangent&): the vertex inputs transformed
uint8_t __fastcall HandleNormals(Self* self, re::Edx, MaterialContext* context, uint32_t kind, Resource* world,
                                 re::Ptr32<Resource>* worldNormal, re::Ptr32<Resource>* worldBinormal, re::Ptr32<Resource>* worldTangent) {
    *worldNormal = nullptr;
    *worldBinormal = nullptr;
    *worldTangent = nullptr;
    if (kind == 0) {
        return 1;
    }
    if (kind == 1) {
        Resource* normal = AddResource(context->inputs, "float3", "Normal", "Local", "Normal");
        void* node = LibraryNode(self, "TransformNormal");
        AddNode(context, node);
        BindInput(context, normal, node, "Normal");
        BindInput(context, world, node, "World");
        *worldNormal = OutputOf(node, "WorldNrm");
        return 1;
    }
    if (kind != 2) {
        return 0;
    }
    Resource* normal = AddResource(context->inputs, "float3", "Normal", "Local", "Normal");
    Resource* binormal = AddResource(context->inputs, "float3", "Binormal", "Local", "Binormal");
    Resource* tangent = AddResource(context->inputs, "float3", "Tangent", "Local", "Tangent");
    void* node = LibraryNode(self, "TransformNBT");
    AddNode(context, node);
    BindInput(context, normal, node, "Normal");
    BindInput(context, binormal, node, "Binormal");
    BindInput(context, tangent, node, "Tangent");
    BindInput(context, world, node, "World");
    *worldNormal = OutputOf(node, "WorldNrm");
    *worldBinormal = OutputOf(node, "WorldBinormal");
    *worldTangent = OutputOf(node, "WorldTangent");
    return 1;
}

// slot 38 (an empty hook: true)
uint8_t __fastcall HandlePostWorldTransform(Self*, re::Edx, void*, void*, void*, void*) {
    return 1;
}

// slot 40 HandleViewVector(context, worldPos, worldNormal, worldBinormal, worldTangent,
// tangentSpace, worldView&, tangentView&): the view vector from the camera position (mapping
// 0x4E), and in tangent space when asked
uint8_t __fastcall HandleViewVector(Self* self, re::Edx, MaterialContext* context, Resource* worldPos,
                                    Resource* worldNormal, Resource* worldBinormal, Resource* worldTangent,
                                    int tangentSpace, re::Ptr32<Resource>* worldView, re::Ptr32<Resource>* tangentView) {
    *worldView = nullptr;
    *tangentView = nullptr;
    void* node = LibraryNode(self, "CalculateViewVector");
    AddNode(context, node);
    BindInput(context, worldPos, node, "WorldPos");
    Resource* camera = Predefined(self, context, 0x4E);
    BindInput(context, camera, node, "CameraPos");
    *worldView = OutputOf(node, "WorldViewVector");
    if (uint8_t(tangentSpace) != 0) {
        void* toTangent = LibraryNode(self, "WorldToTangent");
        AddNode(context, toTangent);
        BindInput(context, *worldView, toTangent, "VectorIn");
        BindInput(context, worldNormal, toTangent, "WorldNormalIn");
        BindInput(context, worldBinormal, toTangent, "WorldBinormalIn");
        BindInput(context, worldTangent, toTangent, "WorldTangentIn");
        *tangentView = OutputOf(toTangent, "VectorOut");
    }
    return 1;
}

// slot 41 HandleReflectionVector(context, worldNormal, normalize, worldView, worldReflect&)
uint8_t __fastcall HandleReflectionVector(Self* self, re::Edx, MaterialContext* context, Resource* worldNormal,
                                          int normalize, Resource* worldView, re::Ptr32<Resource>* worldReflect) {
    void* node = LibraryNode(self, "WorldReflect");
    AddNode(context, node);
    BindInput(context, worldNormal, node, "WorldNrm");
    BindInput(context, worldView, node, "WorldViewVector");
    BindConstant(context, node, "NormalizeNormal", "bool", uint8_t(normalize) != 0 ? "(true)" : "(false)");
    *worldReflect = OutputOf(node, "WorldReflect");
    return 1;
}

// slot 43 HandleMaterialColors(context, specular, vertex color mode (0 ignore, 1 emissive, 2
// ambient and diffuse), lighting mode (1: emissive, ambient and diffuse), diffuse&, specular&,
// specularPower&, ambient&, emissive&, opacity&): the material's colors as uniforms (the
// mappings named by their use here: 0x49 diffuse, 0x4A ambient, 0x4B specular, 0x4C emissive,
// 0x4D power) or the vertex colors,
// the opacity split off the diffuse (or the emissive without one)
uint8_t __fastcall HandleMaterialColors(Self* self, re::Edx, MaterialContext* context, int specularOn,
                                        uint32_t vertexColorMode, uint32_t lightingMode, re::Ptr32<Resource>* diffuse,
                                        re::Ptr32<Resource>* specular, re::Ptr32<Resource>* specularPower, re::Ptr32<Resource>* ambient,
                                        re::Ptr32<Resource>* emissive, re::Ptr32<Resource>* opacity) {
    *specular = nullptr;
    *diffuse = nullptr;
    *ambient = nullptr;
    *emissive = nullptr;
    *specularPower = nullptr;
    *opacity = nullptr;
    if (uint8_t(specularOn) != 0) {
        *specular = Predefined(self, context, 0x4B);
        *specularPower = Predefined(self, context, 0x4D);
    }
    bool vertexColors = vertexColorMode == 1 || (vertexColorMode == 2 && lightingMode == 1);
    Resource* colors = nullptr;
    if (vertexColors) {
        colors = AddResource(context->inputs, "float4", "Color", "", "VertexColors");
    }
    switch (vertexColorMode) {
    case 0:
        *emissive = Predefined(self, context, 0x4C);
        if (lightingMode == 1) {
            *diffuse = Predefined(self, context, 0x49);
            *ambient = Predefined(self, context, 0x4A);
        }
        break;
    case 1:
        *emissive = colors;
        if (lightingMode == 1) {
            *diffuse = Predefined(self, context, 0x49);
            *ambient = Predefined(self, context, 0x4A);
        }
        break;
    case 2:
        *emissive = Predefined(self, context, 0x4C);
        if (lightingMode == 1) {
            *diffuse = colors;
            *ambient = colors;
        }
        break;
    default:
        break;
    }
    if (*diffuse == nullptr) {
        return Virtual<uint8_t>(self, 80, context, *emissive, emissive, opacity) ? 1 : 0;
    }
    return Virtual<uint8_t>(self, 80, context, *diffuse, diffuse, opacity) ? 1 : 0;
}

// slot 46 HandleTexGenOutputs(context, outputs[], (unused), worldPos, worldNormal, worldReflect,
// environment texgen, projected lights, their clipped mask, projected shadows, their clipped
// mask, count&): the generated texture coordinates (slot 47) of the environment map and each
// projected light and shadow appended to the outputs
uint8_t __fastcall HandleTexGenOutputs(Self* self, re::Edx, MaterialContext* context, re::Ptr32<Resource>* outputs, void*,
                                       Resource* worldPos, Resource* worldNormal, Resource* worldReflect,
                                       uint32_t environmentTexGen, uint32_t lightCount, uint32_t lightClipped,
                                       uint32_t shadowCount, uint32_t shadowClipped, uint32_t* count) {
    Resource* generated = nullptr;
    if (!Virtual<uint8_t>(self, 47, context, worldPos, worldNormal, worldReflect, 8u, environmentTexGen, 0u,
                          &generated)) {
        return 0;
    }
    if (generated != nullptr) {
        outputs[(*count)++] = generated;
    }
    uint32_t mask = 1;
    for (uint32_t i = 0; i < lightCount; i++) {
        uint32_t texGen = (lightClipped & mask) != 0 ? 2 : 1;
        mask += mask;
        if (!Virtual<uint8_t>(self, 47, context, worldPos, worldNormal, worldReflect, 10u, texGen, i, &generated)) {
            return 0;
        }
        if (generated != nullptr) {
            outputs[(*count)++] = generated;
        }
    }
    mask = 1;
    for (uint32_t i = 0; i < shadowCount; i++) {
        uint32_t texGen = (shadowClipped & mask) != 0 ? 2 : 1;
        mask += mask;
        if (!Virtual<uint8_t>(self, 47, context, worldPos, worldNormal, worldReflect, 9u, texGen, i, &generated)) {
            return 0;
        }
        if (generated != nullptr) {
            outputs[(*count)++] = generated;
        }
    }
    return 1;
}

// slot 47 HandleTexGen(context, worldPos, worldNormal, worldReflect, object type, texgen,
// index, output&): the coordinates projected by the object's texture transform (mapping
// 0x17): from the position (1, 2), the normalized reflection (3), the normal (4) or the
// reflection (5); none for texgen 0 or no object type
uint8_t __fastcall HandleTexGen(Self* self, re::Edx, MaterialContext* context, Resource* worldPos,
                                Resource* worldNormal, Resource* worldReflect, uint32_t objectType, uint32_t texGen,
                                uint32_t index, re::Ptr32<Resource>* output) {
    *output = nullptr;
    Resource* input;
    switch (texGen) {
    case 0:
        return 1;
    case 1:
    case 2:
        input = worldPos;
        break;
    case 3:
        Virtual<uint8_t>(self, 70, context, &worldReflect);   // NormalizeVector
        input = worldReflect;
        break;
    case 4:
        input = worldNormal;
        break;
    case 5:
        input = worldReflect;
        break;
    default:
        return 0;
    }
    if (input == nullptr || objectType == 0) {
        return 1;
    }
    void* node = LibraryNode(self, "ProjectTextureCoordinates", Temp::Destructed);
    AddNode(context, node);
    BindInput(context, input, node, "TexCoord", Temp::Destructed);
    const char* modifier = AttributeDesc_GetObjectTypeName(objectType);
    Resource* transform = Virtual<Resource*>(self, 32, context->uniforms, 0x17u, objectType, index, modifier, 1u);
    BindInput(context, transform, node, "TexTransform", Temp::Destructed);
    *output = OutputOf(node, "TexCoordOut", Temp::Destructed);
    return 1;
}

// slot 49 HandleShadingCoefficients(context, emissive, diffuse, ambient, specular,
// lightSpecular, lightDiffuse, lightAmbient, diffuseCoefficient&, specularCoefficient&): the
// material colors (white, or black emissive, when missing) and the lights' sums combined, with
// the saturation switch (+0x3BD)
uint8_t __fastcall HandleShadingCoefficients(Self* self, re::Edx, MaterialContext* context, Resource* emissive,
                                             Resource* diffuse, Resource* ambient, Resource* specular,
                                             Resource* lightSpecular, Resource* lightDiffuse, Resource* lightAmbient,
                                             re::Ptr32<Resource>* diffuseCoefficient, re::Ptr32<Resource>* specularCoefficient) {
    void* node = LibraryNode(self, "ComputeShadingCoefficients");
    AddNode(context, node);
    if (diffuse == nullptr) {
        diffuse = Constant(context, "float3", "(1.0, 1.0, 1.0)");
    }
    BindInput(context, diffuse, node, "MatDiffuse");
    if (specular == nullptr) {
        specular = Constant(context, "float3", "(1.0, 1.0, 1.0)");
    }
    BindInput(context, specular, node, "MatSpecular");
    if (ambient == nullptr) {
        ambient = Constant(context, "float3", "(1.0, 1.0, 1.0)");
    }
    BindInput(context, ambient, node, "MatAmbient");
    if (emissive == nullptr) {
        emissive = Constant(context, "float3", "(0.0, 0.0, 0.0)");
    }
    BindInput(context, emissive, node, "MatEmissive");
    if (lightDiffuse != nullptr) {
        BindInput(context, lightDiffuse, node, "LightDiffuseAccum");
    }
    if (lightSpecular != nullptr) {
        BindInput(context, lightSpecular, node, "LightSpecularAccum");
    }
    if (lightAmbient != nullptr) {
        BindInput(context, lightAmbient, node, "LightAmbientAccum");
    }
    BindConstantByName(context, node, "Saturate", "bool", self->unknown3BD != 0 ? "(true)" : "(false)");
    *diffuseCoefficient = OutputOf(node, "Diffuse");
    *specularCoefficient = OutputOf(node, "Specular");
    return 1;
}

// ---- the texture maps

// slot 52 HandleParallaxMap(context, uv&, tangentView): the coordinates offset by the height map
// (mapping 0x69 the offset scale)
uint8_t __fastcall HandleParallaxMap(Self* self, re::Edx, MaterialContext* context, re::Ptr32<Resource>* uv,
                                     Resource* tangentView) {
    re::Ptr32<Resource> height = nullptr;
    re::Ptr32<Resource> opacity = nullptr;
    if (!InsertTextureMap(self, context, 0, 0, 1, *uv, nullptr, nullptr, &height, &opacity)) {
        return 0;
    }
    void* node = LibraryNode(self, "CalculateParallaxOffset");
    AddNode(context, node);
    BindInput(context, *uv, node, "TexCoord");
    BindInput(context, height, node, "Height");
    Resource* offsetScale = Predefined(self, context, 0x69);
    BindInput(context, offsetScale, node, "OffsetScale");
    if (tangentView != nullptr) {
        BindInput(context, tangentView, node, "TangentSpaceEyeVec");
    }
    *uv = OutputOf(node, "ParallaxOffsetUV");
    return 1;
}

// slot 53 HandleBaseMap(context, uv, color&, opacity&, alpha): the base map modulating the color
// (sampled RGBA when alpha is wanted), its opacity multiplied into the one there is
uint8_t __fastcall HandleBaseMap(Self* self, re::Edx, MaterialContext* context, Resource* uv, re::Ptr32<Resource>* color,
                                 re::Ptr32<Resource>* opacity, int alpha) {
    re::Ptr32<Resource> sampledOpacity = nullptr;
    if (!InsertTextureMap(self, context, 1, 0, uint8_t(alpha) != 0 ? 4 : 0, uv, *color, nullptr, color,
                          &sampledOpacity)) {
        return 0;
    }
    if (*opacity != nullptr) {
        return Virtual<uint8_t>(self, 72, context, *opacity, sampledOpacity, opacity);   // MultiplyVector
    }
    *opacity = sampledOpacity;
    return 1;
}

// slot 55 HandleDarkMap(context, uv, color&): apply 1, as the game has it
uint8_t __fastcall HandleDarkMap(Self* self, re::Edx, MaterialContext* context, Resource* uv, re::Ptr32<Resource>* color, void*) {
    re::Ptr32<Resource> opacity = nullptr;
    return InsertTextureMap(self, context, 3, 1, 1, uv, *color, nullptr, color, &opacity);
}

// slot 56 HandleDetailMap(context, uv, color&): modulated by twice the sample
uint8_t __fastcall HandleDetailMap(Self* self, re::Edx, MaterialContext* context, Resource* uv, re::Ptr32<Resource>* color,
                                   void*) {
    re::Ptr32<Resource> opacity = nullptr;
    NiFixedString value = String("(2.0)");
    NiFixedString type = String("float");
    Resource* two = Node_AddOutputConstant(context->statics, &type, &value);
    uint8_t result = InsertTextureMap(self, context, 4, 0, 1, uv, *color, two, color, &opacity);
    Drop(type);
    Drop(value);
    return result;
}

// slot 57 HandleGlossMap(context, uv, gloss&)
uint8_t __fastcall HandleGlossMap(Self* self, re::Edx, MaterialContext* context, Resource* uv, re::Ptr32<Resource>* gloss) {
    re::Ptr32<Resource> opacity = nullptr;
    return InsertTextureMap(self, context, 6, 0, 1, uv, *gloss, nullptr, gloss, &opacity);
}

// slot 58 HandleBumpMap(context, uv, bumpOffset&): the DuDv sample through the bump matrix
// (mapping 0x67; its input looked up before the uniform is made)
uint8_t __fastcall HandleBumpMap(Self* self, re::Edx, MaterialContext* context, Resource* uv, re::Ptr32<Resource>* bumpOffset) {
    re::Ptr32<Resource> duDv = nullptr;
    re::Ptr32<Resource> opacity = nullptr;
    if (!InsertTextureMap(self, context, 5, 0, 1, uv, nullptr, nullptr, &duDv, &opacity)) {
        return 0;
    }
    void* node = LibraryNode(self, "CalculateBumpOffset");
    AddNode(context, node);
    BindInput(context, duDv, node, "DuDv");
    NiFixedString s = String("BumpMatrix");
    void* configurator = context->configurator;
    void* input = Node_GetInputResourceByVariableName(node, &s);
    Resource* matrix = Predefined(self, context, 0x67);
    Configurator_AddBinding(configurator, matrix, input);
    Drop(s);
    *bumpOffset = OutputOf(node, "BumpOffset");
    return 1;
}

// slot 59 HandleGlowMap(context, uv, color&): added
uint8_t __fastcall HandleGlowMap(Self* self, re::Edx, MaterialContext* context, Resource* uv, re::Ptr32<Resource>* color, void*) {
    re::Ptr32<Resource> opacity = nullptr;
    return InsertTextureMap(self, context, 7, 1, 1, uv, *color, nullptr, color, &opacity);
}

// slot 60 (the custom maps: none, true)
uint8_t __fastcall HandleCustomMaps(Self*, re::Edx, void*, void*, void*, void*, void*, void*, void*, void*, void*, void*,
                                    void*, void*) {
    return 1;
}

// slot 61 HandleEnvMap(context, env type, uv, (unused), color&, bumpOffset, input): the
// environment map (sampled 2D for types 1 and 3, projected for 2, cube for 4 and 5) at the
// coordinates offset by the bump, modulating the input, added to the color
uint8_t __fastcall HandleEnvMap(Self* self, re::Edx, MaterialContext* context, uint32_t type, Resource* uv, void*,
                                re::Ptr32<Resource>* color, Resource* bumpOffset, Resource* input) {
    const re::Ptr32<const char>* mapName = TextureEffectMapName(2);
    Resource* coordinates = uv;
    if (bumpOffset != nullptr) {
        void* node;
        if (TypeIs(uv, "float2")) {
            node = LibraryNode(self, "OffsetUVFloat2");
        } else if (TypeIs(uv, "float3")) {
            node = LibraryNode(self, "OffsetUVFloat3", Temp::Destructed);
        } else if (TypeIs(uv, "float4")) {
            node = LibraryNode(self, "OffsetUVFloat4", Temp::Constructed);
        } else {
            return 0;
        }
        if (node == nullptr) {
            return 0;
        }
        AddNode(context, node);
        BindByName(context, uv, node, "TexCoordIn");
        BindByName(context, bumpOffset, node, "TexCoordOffset");
        coordinates = OutputOf(node, "TexCoordOut");
    }
    Resource* sampled = nullptr;
    Resource* opacity = nullptr;
    uint32_t sample = 0;
    bool insert = true;
    switch (type) {
    case 1:
    case 3:
        sample = 1;
        break;
    case 2:
        sample = 2;
        break;
    case 4:
    case 5:
        sample = 3;
        break;
    default:
        insert = false;
        break;
    }
    if (insert && !Virtual<uint8_t>(self, 78, context, mapName, 0u, 0u, sample, coordinates, input, nullptr,
                                    &sampled, &opacity)) {
        return 0;
    }
    if (*color != nullptr) {
        return Virtual<uint8_t>(self, 71, context, sampled, *color, color) ? 1 : 0;   // AddVector
    }
    *color = sampled;
    return 1;
}

// slot 62 HandleDecalMap(context, uv, decal, color&): blended by its alpha
uint8_t __fastcall HandleDecalMap(Self* self, re::Edx, MaterialContext* context, Resource* uv, uint32_t decal,
                                  re::Ptr32<Resource>* color, void*) {
    re::Ptr32<Resource> opacity = nullptr;
    return InsertTextureMap(self, context, decal + 13, 2, 0, uv, *color, nullptr, color, &opacity);
}

// the clipping plane test of a projected light or shadow (its clip plane an object uniform,
// mapping 0x14): its scalar output
Resource* ClippingPlaneTest(Self* self, MaterialContext* context, Resource* worldPos, uint32_t objectType,
                            uint32_t index, const char* modifier, bool invert) {
    void* node = LibraryNode(self, "ClippingPlaneTest");
    AddNode(context, node);
    BindInput(context, worldPos, node, "WorldPos");
    NiFixedString s = String("WorldClipPlane");
    void* configurator = context->configurator;
    void* input = Node_GetInputResourceByVariableName(node, &s);
    Resource* plane = Virtual<Resource*>(self, 32, context->uniforms, 0x14u, objectType, index, modifier, 1u);
    Configurator_AddBinding(configurator, plane, input);
    Drop(s);
    if (invert) {
        BindConstant(context, node, "InvertClip", "bool", "(true)");
    }
    return OutputOf(node, "Scalar");
}

// slot 63 HandleProjLightMap(context, uv, index, perspective, clipped, worldPos, color&): added
// to the color (scaled by the clipping test)
uint8_t __fastcall HandleProjLightMap(Self* self, re::Edx, MaterialContext* context, Resource* uv, uint32_t index,
                                      int perspective, int clipped, Resource* worldPos, re::Ptr32<Resource>* color, void*) {
    Resource* scalar = nullptr;
    if (uint8_t(clipped) != 0) {
        scalar = ClippingPlaneTest(self, context, worldPos, 0xA, index, "ProjLightMap", false);
    }
    Resource* opacity = nullptr;
    uint32_t sample = uint8_t(perspective) != 0 ? 2 : 1;
    return Virtual<uint8_t>(self, 78, context, TextureEffectMapName(0), index, 1u, sample, uv, *color, scalar, color,
                            &opacity) != 0
               ? 1
               : 0;
}

// slot 64 HandleProjShadowMap(context, uv, index, perspective, clipped, worldPos, color&): the
// shadow map sampled (its sampler inserted by hand) and multiplied in, saturated, by the
// inverted clipping test (slot 77)
uint8_t __fastcall HandleProjShadowMap(Self* self, re::Edx, MaterialContext* context, Resource* uv, uint32_t index,
                                       int perspective, int clipped, Resource* worldPos, re::Ptr32<Resource>* color, void*) {
    Resource* scalar = nullptr;
    if (uint8_t(clipped) != 0) {
        scalar = ClippingPlaneTest(self, context, worldPos, 9, index, "ProjShadowMap", true);
    }
    Resource* sampler = Virtual<Resource*>(self, 30, context, TextureEffectMapName(1), 0u, index, 0u);
    void* node = uint8_t(perspective) != 0 ? LibraryNode(self, "TextureRGBProjectSample")
                                           : LibraryNode(self, "TextureRGBSample");
    AddNode(context, node);
    BindInput(context, uv, node, "TexCoord");
    BindInput(context, sampler, node, "Sampler");
    Resource* sampled = OutputOf(node, "ColorOut");
    return Virtual<uint8_t>(self, 77, context, *color, sampled, scalar, color) != 0 ? 1 : 0;
}

// slot 65 HandleLights(context, (passed on), point, directional and spot light counts, their
// shadow mask, then the lights' arguments): each light's shadow (slot 66, when it casts one and
// there's a shadow manager) and lighting (slot 67), numbered among the shadowed or unshadowed
// lights of its kind (point 1, directional 0, spot 2)
uint8_t __fastcall HandleLights(Self* self, re::Edx, MaterialContext* context, void* a2, uint32_t pointCount,
                                uint32_t directionalCount, uint32_t spotCount, uint32_t shadowMask, void* a7, void* a8,
                                void* a9, void* a10, void* a11, void* a12, void* a13, void* a14) {
    uint32_t light = 0;
    const uint32_t counts[3] = {pointCount, directionalCount, spotCount};
    const uint32_t types[3] = {1, 0, 2};
    for (int kind = 0; kind < 3; kind++) {
        uint32_t shadowed = 0;
        uint32_t unshadowed = 0;
        for (uint32_t i = 0; i < counts[kind]; i++, light++) {
            // the light's bit (as the shifts give it: past 31 they wrap)
            uint32_t bit = (0xFFFFFFFFu << ((light + 1) & 31)) ^ (0xFFFFFFFFu << (light & 31));
            void* shadow = nullptr;
            if ((shadowMask & bit) != 0 && NiShadowManager_ms_pkShadowManager != nullptr) {
                if (!Virtual<uint8_t>(self, 66, context, light, types[kind], shadowed, a8, a9, a7, &shadow)) {
                    return 0;
                }
                if (!Virtual<uint8_t>(self, 67, context, light, types[kind], shadowed, a8, a9, a10, a11, a12, a13, a14,
                                      a2, shadow)) {
                    return 0;
                }
                shadowed++;
            } else {
                if (!Virtual<uint8_t>(self, 67, context, light, types[kind], unshadowed, a8, a9, a10, a11, a12, a13,
                                      a14, a2, static_cast<void*>(nullptr))) {
                    return 0;
                }
                unshadowed++;
            }
        }
    }
    return 1;
}

// slot 78 InsertTexture(context, map name, occurrence, apply (0 modulate, 1 add, 2 lerp by the
// opacity), sample (0 RGBA, 1 RGB, 2 projected, 3 cube, 4 RGBA for its alpha only), uv, input,
// scale, color&, opacity&): the map's sampler and sample node, the sample split, scaled,
// applied to the input and saturated (+0x3BE)
uint8_t __fastcall InsertTexture(Self* self, re::Edx, MaterialContext* context, const re::Ptr32<const char>* mapName,
                                 uint32_t occurrence, uint32_t apply, uint32_t sample, Resource* uv, Resource* input,
                                 Resource* scale, re::Ptr32<Resource>* color, re::Ptr32<Resource>* opacity) {
    bool rgba = false;
    bool keepColor = true;
    uint32_t samplerType = 0;
    void* node;
    switch (sample) {
    case 0:
        node = LibraryNode(self, "TextureRGBASample", Temp::Constructed);
        rgba = true;
        break;
    case 4:
        node = LibraryNode(self, "TextureRGBASample", Temp::Constructed);
        rgba = true;
        keepColor = false;
        break;
    case 1:
        node = LibraryNode(self, "TextureRGBSample", Temp::Constructed);
        break;
    case 2:
        node = LibraryNode(self, "TextureRGBProjectSample", Temp::Constructed);
        break;
    case 3:
        node = LibraryNode(self, "TextureRGBCubeSample", Temp::Constructed);
        samplerType = 1;
        break;
    default:
        return 0;
    }
    Resource* sampler = Virtual<Resource*>(self, 30, context, mapName, samplerType, occurrence, 0u);
    if (sampler == nullptr) {
        return 0;
    }
    AddNode(context, node);
    uint8_t ok = BindInput(context, uv, node, "TexCoord") & 1;
    ok &= BindInput(context, sampler, node, "Sampler");
    BindConstantByName(context, node, "Saturate", "bool", "(false)");
    if (!ok) {
        return 0;
    }
    Resource* sampled = OutputOf(node, "ColorOut", Temp::Destructed);
    if (rgba && TypeIs(sampled, "float4")) {
        if (!Virtual<uint8_t>(self, 80, context, sampled, &sampled, opacity)) {   // SplitColorAndOpacity
            return 0;
        }
    } else {
        *opacity = nullptr;
    }
    if (!keepColor) {
        sampled = nullptr;
    }
    if (scale != nullptr && sampled != nullptr) {
        if (!Virtual<uint8_t>(self, 75, context, sampled, scale, &sampled)) {   // ScaleVector
            return 0;
        }
    }
    if (input != nullptr && sampled != nullptr) {
        switch (apply) {
        case 0:
            ok &= Virtual<uint8_t>(self, 72, context, input, sampled, &sampled);   // MultiplyVector
            break;
        case 1:
            ok &= Virtual<uint8_t>(self, 71, context, input, sampled, &sampled);   // AddVector
            break;
        case 2:
            ok &= Virtual<uint8_t>(self, 74, context, input, sampled, *opacity, &sampled);   // LerpVector
            break;
        default:
            ok = 0;
            break;
        }
        if (self->unknown3BE != 0) {
            ok &= Virtual<uint8_t>(self, 76, context, sampled, &sampled);   // SaturateVector
        }
    }
    if (!ok) {
        return 0;
    }
    if (keepColor) {
        *color = sampled;
    }
    return ok;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISTDMAT2_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISTDMAT2_REVERSED("NiStandardMaterial::HandleNormals", 0x0078A0D0, &ni::HandleNormals);
NISTDMAT2_REVERSED("NiStandardMaterial::HandlePostWorldTransform", 0x00789D00, &ni::HandlePostWorldTransform);
NISTDMAT2_REVERSED("NiStandardMaterial::HandleViewVector", 0x0078A9E0, &ni::HandleViewVector);
NISTDMAT2_REVERSED("NiStandardMaterial::HandleReflectionVector", 0x0078ADC0, &ni::HandleReflectionVector);
NISTDMAT2_REVERSED("NiStandardMaterial::HandleMaterialColors", 0x0078B020, &ni::HandleMaterialColors);
NISTDMAT2_REVERSED("NiStandardMaterial::HandleTexGenOutputs", 0x00789EB0, &ni::HandleTexGenOutputs);
NISTDMAT2_REVERSED("NiStandardMaterial::HandleTexGen", 0x0078C6F0, &ni::HandleTexGen);
NISTDMAT2_REVERSED("NiStandardMaterial::HandleShadingCoefficients", 0x0078CC30, &ni::HandleShadingCoefficients);
NISTDMAT2_REVERSED("NiStandardMaterial::HandleParallaxMap", 0x0078B300, &ni::HandleParallaxMap);
NISTDMAT2_REVERSED("NiStandardMaterial::HandleBaseMap", 0x00789D10, &ni::HandleBaseMap);
NISTDMAT2_REVERSED("NiStandardMaterial::HandleDarkMap", 0x00789DA0, &ni::HandleDarkMap);
NISTDMAT2_REVERSED("NiStandardMaterial::HandleDetailMap", 0x0078BA50, &ni::HandleDetailMap);
NISTDMAT2_REVERSED("NiStandardMaterial::HandleGlossMap", 0x00789DE0, &ni::HandleGlossMap);
NISTDMAT2_REVERSED("NiStandardMaterial::HandleBumpMap", 0x0078BB30, &ni::HandleBumpMap);
NISTDMAT2_REVERSED("NiStandardMaterial::HandleGlowMap", 0x00789E20, &ni::HandleGlowMap);
NISTDMAT2_REVERSED("NiStandardMaterial::HandleCustomMaps", 0x00789E60, &ni::HandleCustomMaps);
NISTDMAT2_REVERSED("NiStandardMaterial::HandleEnvMap", 0x0078BD00, &ni::HandleEnvMap);
NISTDMAT2_REVERSED("NiStandardMaterial::HandleDecalMap", 0x00789E70, &ni::HandleDecalMap);
NISTDMAT2_REVERSED("NiStandardMaterial::HandleProjLightMap", 0x0078C060, &ni::HandleProjLightMap);
NISTDMAT2_REVERSED("NiStandardMaterial::HandleProjShadowMap", 0x0078C260, &ni::HandleProjShadowMap);
NISTDMAT2_REVERSED("NiStandardMaterial::HandleLights", 0x0078C8A0, &ni::HandleLights);
NISTDMAT2_REVERSED("NiStandardMaterial::InsertTexture", 0x0078B590, &ni::InsertTexture);
