// NiStandardMaterial's remaining shade tree handlers (src/gamebryo/NiFragmentMaterial.h,
// NiShadeTree.h): the vertex program's transform pipeline, texture coordinates and fog; the
// pixel program's texture maps (parallax, normal, dark, base, detail, decals, the custom maps,
// projected lights and shadows, gloss; then bump, environment, glow), each light and its shadow,
// the fog and the alpha test; InsertTexture by map. Each adds its fragments to the configurator
// and binds them, returning the resources the next ones take.
//
// The temporary names are made and let go the way the game's code does at each place (Temp).

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

GAME_FN(const re::Ptr32<const char>*, __cdecl, TexturingProperty_GetMapNameFromID, 0x00765A00, int id);

// the node's input of that name (the name made, looked up and let go)
Resource* InputOf(void* node, const char* name) {
    NiFixedString s = String(name);
    Resource* input = Node_GetInputResourceByVariableName(node, &s);
    Drop(s);
    return input;
}

// the UV coordinates of the pixel program's map of that index (GetUVForMap inlined: the slot before
// the array when the index is -1)
Resource* UVOf(NiMaterialDescriptor* desc, re::Ptr32<Resource>* uvs, int32_t index) {
    int32_t set = index == -1 ? -1 : int32_t(PixelDesc_GetUVSetForMap(desc, uint32_t(index)));
    return uvs[set];
}

// ---- the vertex program

// slot 42 SetupTransformPipeline(context, projected position output, vertex descriptor, force world
// view, force view position, worldPos&, viewPos&, worldNormal&, worldView&): the position (slot 36)
// and normals (37) transformed, the hook (38), the view projection (39) bound to the output; the
// world position, normal, binormal and tangent put out as the descriptor asks (as inputs of the
// outputs node, and outputs of it too), the view vector (40) when it's put out or asked for
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
    Resource* worldBinormal = nullptr;
    Resource* worldTangent = nullptr;
    uint32_t normals = (desc->bits[0] >> 6) & 3;
    if (!Virtual<uint8_t>(self, 37, context, normals, world, worldNormal, &worldBinormal, &worldTangent)) {
        return 0;
    }
    if (!Virtual<uint8_t>(self, 38, context, desc, worldPos, worldNormal)) {
        return 0;
    }
    Resource* position = nullptr;
    if (!Virtual<uint8_t>(self, 39, context, forceViewPos, *worldPos, &position, viewPos)) {   // HandleViewProjection
        return 0;
    }
    if (position == nullptr || projected == nullptr) {
        return 0;
    }
    Configurator_AddBinding(context->configurator, position, projected);

    if ((uint8_t(desc->bits[0]) & 4) != 0) {
        Resource* input =
            NamedResource(true, context->outputs, Temp::Constructed, "float4", "TexCoord", "", "WorldPos");
        NamedResource(false, context->outputs, Temp::Constructed, "float4", "TexCoord", "", "WorldPos");
        Configurator_AddBinding(context->configurator, *worldPos, input);
    }
    if ((uint8_t(desc->bits[0]) & 8) != 0) {
        if (normals != 0) {
            Resource* input =
                NamedResource(true, context->outputs, Temp::Constructed, "float3", "TexCoord", "", "WorldNormal");
            NamedResource(false, context->outputs, Temp::Constructed, "float3", "TexCoord", "", "WorldNormal");
            Configurator_AddBinding(context->configurator, *worldNormal, input);
        }
        if (normals == 2) {
            Resource* input =
                NamedResource(true, context->outputs, Temp::Constructed, "float3", "TexCoord", "", "WorldBinormal");
            NamedResource(false, context->outputs, Temp::Constructed, "float3", "TexCoord", "", "WorldBinormal");
            Configurator_AddBinding(context->configurator, worldBinormal, input);
            input = NamedResource(true, context->outputs, Temp::Constructed, "float3", "TexCoord", "", "WorldTangent");
            NamedResource(false, context->outputs, Temp::Constructed, "float3", "TexCoord", "", "WorldTangent");
            Configurator_AddBinding(context->configurator, worldTangent, input);
        }
    }
    uint32_t d0 = desc->bits[0];
    bool view = (uint8_t(d0) & 0x10) == 0x10;
    uint8_t tangent = (uint8_t(d0) & 0x20) == 0x20 ? 1 : 0;
    if (view || tangent != 0 || uint8_t(forceWorldView) != 0) {
        Resource* tangentView = nullptr;
        if (!Virtual<uint8_t>(self, 40, context, *worldPos, *worldNormal, worldBinormal, worldTangent,
                              uint32_t(tangent), worldView, &tangentView)) {   // HandleViewVector
            return 0;
        }
        if (view) {
            Resource* input = NamedResource(true, context->outputs, Temp::Constructed, "float3", "TexCoord",
                                            "WorldView", "WorldView");
            Configurator_AddBinding(context->configurator, *worldView, input);
        }
        if (tangent != 0) {
            Resource* input = NamedResource(true, context->outputs, Temp::Constructed, "float3", "TexCoord",
                                            "TangentSpaceView", "TangentSpaceView");
            Configurator_AddBinding(context->configurator, tangentView, input);
        }
    }
    return 1;
}

// slot 44 HandleVertexFog(context, viewPos, fog type (0 none, 1 linear, 2 range, 3 other)): the
// fog computed (CalculateFog) and put out
uint8_t __fastcall HandleVertexFog(Self* self, re::Edx, MaterialContext* context, Resource* viewPos, uint32_t fogType) {
    if (fogType == 0) {
        return 1;
    }
    void* node = LibraryNode(self, "CalculateFog");
    AddNode(context, node);
    Resource* fog = NamedResource(true, context->outputs, Temp::Inline, "float", "Fog", "", "FogOut");
    NiFixedString s = String("FogOut");
    void* configurator = context->configurator;
    Configurator_AddBinding(configurator, Node_GetOutputResourceByVariableName(node, &s), fog);
    Drop(s);
    uint8_t range = 0;
    uint32_t type = 0;
    if (fogType == 1) {
        range = 0;
        type = 3;
    } else if (fogType == 2) {
        range = 1;
        type = 3;
    }
    char text[0x20];
    NiSprintf(text, 0x20, "(%d)", type);
    BindConstant(context, node, "FogType", "int", text);
    BindConstant(context, node, "FogRange", "bool", range != 0 ? "(true)" : "(false)");
    Resource* density = Predefined(self, context, 0x64);
    BindInput(context, density, node, "FogDensity");
    Resource* startEnd = Predefined(self, context, 0x65);
    BindInput(context, startEnd, node, "FogStartEnd");
    BindInput(context, viewPos, node, "ViewPosition");
    return 1;
}

// slot 45 HandleTextureUVSets(context, vertex descriptor, outputs[], (their room), count&): an input
// of each UV set up to the highest the output UVs use, each output UV made (slot 51); the plain
// ones keep their index, the generated ones come after the sets used (count)
uint8_t __fastcall HandleTextureUVSets(Self* self, re::Edx, MaterialContext* context, NiMaterialDescriptor* desc,
                                       re::Ptr32<Resource>* outputs, uint32_t, uint32_t* count) {
    uint32_t uvCount = uint8_t(desc->bits[0] >> 24) & 0xF;
    int32_t used = -1;
    uint32_t uvSet = 0xFFFFFFFF;
    int32_t highest = -1;
    uint32_t texGen = 0xFFFFFFFF;
    uint8_t taken[12] = {};
    for (uint32_t i = 0; i < uvCount; i++) {
        VertexDesc_GetOutputUV(desc, i, &uvSet, &texGen);
        if (int32_t(uvSet) > highest) {
            highest = int32_t(uvSet);
        }
        if (texGen == 0 && taken[uvSet] == 0) {
            used++;
            taken[uvSet] = 1;
        }
    }
    Resource* sets[12];
    crt::Memset(sets, 0, sizeof(sets));
    for (int32_t i = 0; i <= highest; i++) {
        NiFixedString variable;
        Virtual<re::Ptr32<const char>*>(self, 29, &variable, uint32_t(i));   // GenerateUVSetName
        NiFixedString label = String("");
        NiFixedString semantic = String("TexCoord");
        NiFixedString type = String("float2");
        sets[i] = Node_AddOutputResource(context->inputs, &type, &semantic, &label, &variable, 1, 0, 0, 0);
        Drop(type);
        Drop(semantic);
        Drop(label);
        Drop(variable);
    }
    *count = uint32_t(used + 1);
    for (uint32_t i = 0; i < uvCount; i++) {
        VertexDesc_GetOutputUV(desc, i, &uvSet, &texGen);
        uint32_t output = i;
        if (texGen != 0) {
            output = *count;
            *count = output + 1;
        }
        NiFixedString variable;
        Virtual<re::Ptr32<const char>*>(self, 29, &variable, i);
        uint8_t ok = Virtual<uint8_t>(self, 51, context, sets[uvSet], &outputs[output], &variable, output, texGen);
        Drop(variable);
        if (ok == 0) {
            return 0;
        }
    }
    return 1;
}

// slot 51 HandleUVTransform(context, uv, output&, name, output index, texgen): the UV set as it
// is, or (texgen 1) through its texture transform, a uniform "<name>_TexTransform"
uint8_t __fastcall HandleUVTransform(Self* self, re::Edx, MaterialContext* context, Resource* uv, re::Ptr32<Resource>* output,
                                     const re::Ptr32<const char>* name, uint32_t index, uint32_t texGen) {
    NiFixedString type = NullFixedString;
    *output = uv;
    if (uv == nullptr) {
        Drop(type);
        return 0;
    }
    FixedString_AssignText(&type, "float2");
    NiFixedString copy = CopyHandle(name);
    if (texGen == 1) {
        void* node = LibraryNode(self, "TexTransformApply");
        AddNode(context, node);
        BindInput(context, uv, node, "TexCoord");
        char text[0x20];
        NiSprintf(text, 0x20, "%s_TexTransform", *name);
        Resource* transform = NamedResource(false, context->uniforms, Temp::Inline, "float4x4", "TexTransform", "",
                                            text, 1, 2, 0, index);
        BindInput(context, transform, node, "TexTransform");
        *output = OutputOf(node, "TexCoordOut");
    }
    Drop(copy);
    Drop(type);
    return 1;
}

// ---- the pixel program

// slot 48 HandleTextureMaps(context, pixel descriptor, worldPos&, worldNormal&, worldBinormal&,
// worldTangent&, worldView&, tangentView&, diffuse&, specular&, specularPower&, gloss&, ambient&,
// emissive&, opacity&, color&, (unused)&, textures&, uvs[], UV set count, generated count): the
// parallax offset (the UVs it moves), the normal map, the dark map, the projected lights, the base
// map (white without one), the detail map, the decals, the custom maps (slot 60), the projected
// shadows, the gloss map; the textures counted (the projections not)
uint8_t __fastcall HandleTextureMaps(Self* self, re::Edx, MaterialContext* context, NiMaterialDescriptor* desc,
                                     re::Ptr32<Resource>* worldPos, re::Ptr32<Resource>* worldNormal, re::Ptr32<Resource>* worldBinormal,
                                     re::Ptr32<Resource>* worldTangent, re::Ptr32<Resource>* worldView, re::Ptr32<Resource>* tangentView,
                                     re::Ptr32<Resource>* diffuse, re::Ptr32<Resource>* specular, re::Ptr32<Resource>* specularPower,
                                     re::Ptr32<Resource>* gloss, re::Ptr32<Resource>* ambient, re::Ptr32<Resource>* emissive, re::Ptr32<Resource>* opacity,
                                     re::Ptr32<Resource>* color, re::Ptr32<Resource>* unused, uint32_t* textureCount, re::Ptr32<Resource>* uvs,
                                     uint32_t uvCount, uint32_t) {
    (void)worldView;
    uint32_t textures = 0;
    if ((uint8_t(desc->bits[0]) & 0x80) != 0) {
        uint32_t set = PixelDesc_GetUVSetForMap(desc, 0);
        Resource* uv = uvs[set];
        if (!Virtual<uint8_t>(self, 52, context, &uv, *tangentView)) {   // HandleParallaxMap
            return 0;
        }
        textures++;
        uvs[set] = uv;
    }
    uint32_t p0 = desc->bits[0];
    if (((p0 >> 9) & 1) != 0) {
        uint32_t type = (p0 >> 5) & 3;
        Resource* uv = UVOf(desc, uvs, MapIndex(p0, 2));
        if (!Virtual<uint8_t>(self, 54, context, uv, type, worldNormal, *worldBinormal, *worldTangent)) {
            return 0;
        }
        textures++;
    }
    p0 = desc->bits[0];
    if ((p0 & 0x400) != 0) {
        Resource* uv = UVOf(desc, uvs, MapIndex(p0, 3));
        if (!Virtual<uint8_t>(self, 55, context, uv, color, unused)) {   // HandleDarkMap
            return 0;
        }
        textures++;
    }

    // the projected lights (their coordinates after the UV sets and the environment's)
    p0 = desc->bits[0];
    uint32_t offset = 0;
    if ((p0 & 0x7000000) != 0 && (p0 & 0x200) == 0) {
        offset = 1;
    }
    uint32_t lights = (p0 >> 27) & 3;
    if (lights != 0) {
        re::Ptr32<Resource>* uv = &uvs[uvCount + offset];
        uint32_t mask = 1;
        for (uint32_t i = 0; i < lights; i++) {
            uint32_t a = desc->bits[0];
            uint32_t b = desc->bits[1];
            uint8_t first = (uint8_t(mask) & uint8_t(a >> 29)) != 0 ? 1 : 0;
            uint8_t second = (uint8_t(b & mask) & 7) != 0 ? 1 : 0;
            if (!Virtual<uint8_t>(self, 63, context, *uv, i, uint32_t(first), uint32_t(second), *worldPos, color,
                                  unused)) {   // HandleProjLightMap
                return 0;
            }
            offset++;
            uv++;
            mask += mask;
        }
    }

    p0 = desc->bits[0];
    if ((p0 & 0x100) != 0) {
        uint8_t alpha = (desc->bits[3] & 0x400000) == 0x400000 ? 1 : 0;
        Resource* uv = UVOf(desc, uvs, int32_t((p0 >> 7) & 1));
        if (!Virtual<uint8_t>(self, 53, context, uv, color, opacity, uint32_t(alpha))) {   // HandleBaseMap
            return 0;
        }
        textures++;
    } else if (*color == nullptr) {
        NiFixedString value = String("(1.0f, 1.0f, 1.0f)");
        NiFixedString type = String("float3");
        *color = Node_AddOutputConstant(context->statics, &type, &value);
        Drop(type);
        Drop(value);
    }
    p0 = desc->bits[0];
    if ((p0 & 0x800) != 0) {
        Resource* uv = UVOf(desc, uvs, MapIndex(p0, 4));
        if (!Virtual<uint8_t>(self, 56, context, uv, color, unused)) {   // HandleDetailMap
            return 0;
        }
        textures++;
    }
    uint32_t decals = (desc->bits[0] >> 20) & 7;
    for (uint32_t i = 0; i < decals; i++) {
        Resource* uv = UVOf(desc, uvs, MapIndexFromTextureEnum(i + 13, desc));
        if (!Virtual<uint8_t>(self, 62, context, uv, i, color, unused)) {   // HandleDecalMap
            return 0;
        }
        textures++;
    }
    if (!Virtual<uint8_t>(self, 60, context, desc, &textures, uvs, uvCount, diffuse, specular, specularPower, ambient,
                          emissive, color, unused)) {   // HandleCustomMaps
        return 0;
    }

    // the projected shadows
    uint32_t shadows = (desc->bits[1] >> 3) & 3;
    if (shadows != 0) {
        re::Ptr32<Resource>* uv = &uvs[uvCount + offset];
        uint32_t mask = 1;
        for (uint32_t i = 0; i < shadows; i++) {
            uint32_t b = desc->bits[1];
            uint8_t first = (uint8_t((b >> 5) & mask) & 7) != 0 ? 1 : 0;
            uint8_t second = (uint8_t((b >> 8) & mask) & 7) != 0 ? 1 : 0;
            if (!Virtual<uint8_t>(self, 64, context, *uv, i, uint32_t(first), uint32_t(second), *worldPos, color,
                                  unused)) {   // HandleProjShadowMap
                return 0;
            }
            uv++;
            mask += mask;
        }
    }
    p0 = desc->bits[0];
    if ((p0 & 0x2000) != 0) {
        Resource* uv = UVOf(desc, uvs, MapIndex(p0, 6));
        if (!Virtual<uint8_t>(self, 57, context, uv, gloss)) {   // HandleGlossMap
            return 0;
        }
        textures++;
    }
    *textureCount = textures;
    return 1;
}

// slot 50 HandleEffectMaps(context, pixel descriptor, worldNormal&, worldView, opacity&, diffuse
// color&, specular color&, gloss, textures&, uvs[], UV set count, (generated count)): the bump map's
// offset, the environment map (its coordinates generated from the per-pixel normal when the normal
// map's node is in the tree, else the first generated set), the glow map; the textures counted
uint8_t __fastcall HandleEffectMaps(Self* self, re::Edx, MaterialContext* context, NiMaterialDescriptor* desc,
                                    re::Ptr32<Resource>* worldNormal, Resource* worldView, re::Ptr32<Resource>* opacity,
                                    re::Ptr32<Resource>* diffuseColor, re::Ptr32<Resource>* specularColor, Resource* gloss,
                                    uint32_t* textureCount, re::Ptr32<Resource>* uvs, uint32_t uvCount, uint32_t) {
    (void)opacity;
    Resource* bumpOffset = nullptr;
    uint32_t textures = 0;
    uint32_t p0 = desc->bits[0];
    if ((p0 & 0x1000) != 0) {
        Resource* uv = UVOf(desc, uvs, MapIndex(p0, 5));
        if (!Virtual<uint8_t>(self, 58, context, uv, &bumpOffset)) {   // HandleBumpMap
            return 0;
        }
        textures = 1;
    }
    uint32_t envType = uint8_t(desc->bits[0] >> 24) & 7;
    if (envType != 0) {
        Resource* envUV = nullptr;
        uint8_t perPixelNormal = 0;
        if (*worldNormal != nullptr) {
            NiFixedString s = String("CalculateNormalFromColor");
            perPixelNormal = Configurator_GetNode(context->configurator, &s, 0) != nullptr ? 1 : 0;
            Drop(s);
        }
        if (perPixelNormal != 0) {
            Resource* reflect = nullptr;
            if ((envType == 3 || envType == 5) &&
                !Virtual<uint8_t>(self, 41, context, *worldNormal, 0u, worldView, &reflect)) {   // HandleReflectionVector
                return 0;
            }
            if (!Virtual<uint8_t>(self, 47, context, static_cast<Resource*>(nullptr), *worldNormal, reflect, 8u, envType,
                                  0u, &envUV)) {   // HandleTexGen
                return 0;
            }
        } else {
            envUV = uvs[uvCount];
        }
        if (!Virtual<uint8_t>(self, 61, context, envType, envUV, diffuseColor, specularColor, bumpOffset,
                              gloss)) {   // HandleEnvMap
            return 0;
        }
    }
    p0 = desc->bits[0];
    if ((p0 & 0x4000) != 0) {
        Resource* uv = UVOf(desc, uvs, MapIndex(p0, 7));
        if (!Virtual<uint8_t>(self, 59, context, uv, diffuseColor, specularColor)) {   // HandleGlowMap
            return 0;
        }
        textures++;
    }
    *textureCount += textures;
    return 1;
}

// slot 54 HandleNormalMap(context, uv, normal map type, worldNormal&, worldBinormal, worldTangent):
// the normal map sampled (its sampler inserted by hand) and turned into the world normal
uint8_t __fastcall HandleNormalMap(Self* self, re::Edx, MaterialContext* context, Resource* uv, uint32_t type,
                                   re::Ptr32<Resource>* worldNormal, Resource* worldBinormal, Resource* worldTangent) {
    NiFixedString mapName = NullFixedString;
    FixedString_Assign(&mapName, TexturingProperty_GetMapNameFromID(6));
    Resource* sampler = Virtual<Resource*>(self, 30, context, &mapName, 0u, 0u, 0u);   // InsertTextureSampler
    void* sample = LibraryNode(self, "TextureRGBASample");
    AddNode(context, sample);
    BindInput(context, uv, sample, "TexCoord");
    BindInput(context, sampler, sample, "Sampler");
    BindConstantByName(context, sample, "Saturate", "bool", "(false)");
    Resource* color = OutputOf(sample, "ColorOut");
    if (color == nullptr) {
        Drop(mapName);
        return 0;
    }
    void* node = LibraryNode(self, "CalculateNormalFromColor");
    AddNode(context, node);
    BindInput(context, color, node, "NormalMap");
    BindInput(context, *worldNormal, node, "WorldNormalIn");
    BindInput(context, worldBinormal, node, "WorldBinormalIn");
    BindInput(context, worldTangent, node, "WorldTangentIn");
    char text[0xA];
    NiSprintf(text, 0xA, "(%d)", type);
    BindConstant(context, node, "NormalMapType", "int", text);
    *worldNormal = OutputOf(node, "WorldNormalOut");
    uint8_t result = *worldNormal != nullptr ? 1 : 0;
    Drop(mapName);
    return result;
}

// slot 79 InsertTexture(context, map, occurrence, apply, sample, uv, input, scale, color&,
// opacity&): slot 78 with the map's name (the occurrence the map's)
uint8_t __fastcall InsertTextureMap(Self* self, re::Edx, MaterialContext* context, uint32_t map, uint32_t occurrence,
                                    uint32_t apply, uint32_t sample, Resource* uv, Resource* input, Resource* scale,
                                    re::Ptr32<Resource>* color, re::Ptr32<Resource>* opacity) {
    NiFixedString name = NullFixedString;
    if (!GetTextureNameFromTextureEnum(map, &name, &occurrence)) {
        Drop(name);
        return 0;
    }
    uint8_t result = Virtual<uint8_t>(self, 78, context, &name, occurrence, apply, sample, uv, input, scale, color,
                                      opacity);
    Drop(name);
    return result;
}

// slot 66 HandleShadow(context, light, type (0 directional, 1 point, 2 spot), index, worldPos,
// (worldNormal), shadow technique, shadow&): the technique's shadow fragment for the light type
// with the light's uniforms (the ones it takes), its shadow map (a cube for a point light when the
// technique says so, else 2D with the projection matrix)
uint8_t __fastcall HandleShadow(Self* self, re::Edx, MaterialContext* context, uint32_t, uint32_t type, uint32_t index,
                                Resource* worldPos, Resource*, uint32_t technique, re::Ptr32<Resource>* shadow) {
    void* tech = At<void*>(ms_pkShadowManager, 0x10 + int(uint16_t(technique)) * 4);
    if (tech == nullptr) {
        return 0;
    }
    void* node = GetAttachableNodeFromLibrary(
        self, reinterpret_cast<const re::Ptr32<const char>*>(reinterpret_cast<uintptr_t>(tech) + 8 + type * 4));
    AddNode(context, node);
    BindInput(context, worldPos, node, "WorldPos");
    char text[0x100];
    NiSprintf(text, 0x100, "(%d)", type);
    BindConstant(context, node, "LightType", "int", text);

    const char* modifier = nullptr;
    uint32_t objectType = 1;
    if (type == 0) {
        objectType = 6;
        modifier = "ShadowDir";
    } else if (type == 1) {
        objectType = 5;
        modifier = "ShadowPoint";
    } else if (type == 2) {
        objectType = 7;
        modifier = "ShadowSpot";
        Resource* direction = ObjectUniform(self, context, 0xA, objectType, index, modifier);
        BindInput(context, direction, node, "LightDirection");
    }
    // the uniforms the fragment takes
    static const struct {
        const char* input;
        uint32_t mapping;
    } kUniforms[4] = {{"LightPos", 8}, {"ShadowMapSize", 0x1B}, {"ShadowBias", 0x1C}, {"ShadowVSMPowerEpsilon", 0x1D}};
    for (const auto& u : kUniforms) {
        Resource* input = InputOf(node, u.input);
        if (input != nullptr) {
            Resource* uniform = ObjectUniform(self, context, u.mapping, objectType, index, modifier);
            Configurator_AddBinding(context->configurator, uniform, input);
        }
    }

    // the shadow map (the index becomes the map's occurrence)
    NiFixedString mapName = NullFixedString;
    uint32_t map = 0x17;
    if (type == 0) {
        map = index + 0x17;
    } else if (type == 1) {
        map = index + 0x1F;
    } else if (type == 2) {
        map = index + 0x27;
    }
    if (!GetTextureNameFromTextureEnum(map, &mapName, &index)) {
        Drop(mapName);
        return 0;
    }
    Resource* sampler;
    if (type == 1 && At<uint8_t>(tech, 0x49) != 0) {
        sampler = Virtual<Resource*>(self, 30, context, &mapName, 1u, index, objectType);   // InsertTextureSampler
    } else {
        Resource* matrix = ObjectUniform(self, context, 0x19, objectType, index, modifier);
        BindInput(context, matrix, node, "WorldToLightProjMat");
        sampler = Virtual<Resource*>(self, 30, context, &mapName, 0u, index, objectType);
    }
    BindInput(context, sampler, node, "ShadowMap");
    *shadow = OutputOf(node, "ShadowOut");
    Drop(mapName);
    return 1;
}

// slot 67 HandleLight(context, light, type, index, worldPos, worldNormal, worldView, specularPower,
// ambient&, diffuse&, specular&, specular on, shadow): the Light fragment with the sums so far, the
// light's uniforms (the shadowed kind's when it casts a shadow), the view vector and specular power
// for specular; the sums it puts out
uint8_t __fastcall HandleLight(Self* self, re::Edx, MaterialContext* context, uint32_t, uint32_t type, uint32_t index,
                               Resource* worldPos, Resource* worldNormal, Resource* worldView,
                               Resource* specularPower, re::Ptr32<Resource>* ambient, re::Ptr32<Resource>* diffuse, re::Ptr32<Resource>* specular,
                               int specularOn, Resource* shadow) {
    void* node = LibraryNode(self, "Light");
    AddNode(context, node);
    BindInput(context, worldPos, node, "WorldPos");
    BindInput(context, worldNormal, node, "WorldNrm");
    if (*ambient != nullptr) {
        BindInput(context, *ambient, node, "AmbientAccum");
    }
    if (*diffuse != nullptr) {
        BindInput(context, *diffuse, node, "DiffuseAccum");
    }
    if (*specular != nullptr && uint8_t(specularOn) != 0) {
        BindInput(context, *specular, node, "SpecularAccum");
    }
    BindConstant(context, node, "SpecularEnable", "bool", uint8_t(specularOn) != 0 ? "(true)" : "(false)");
    if (shadow != nullptr) {
        BindInput(context, shadow, node, "Shadow");
    }
    char text[0x20];
    NiSprintf(text, 0x20, "(%d)", type);
    BindConstant(context, node, "LightType", "int", text);

    uint32_t objectType = 1;
    const char* modifier = nullptr;
    if (type == 0) {
        objectType = shadow != nullptr ? 6 : 3;
        modifier = shadow != nullptr ? "ShadowDir" : "Dir";
    } else if (type == 1) {
        objectType = shadow != nullptr ? 5 : 2;
        modifier = shadow != nullptr ? "ShadowPoint" : "Point";
    } else if (type == 2) {
        objectType = shadow != nullptr ? 7 : 4;
        modifier = shadow != nullptr ? "ShadowSpot" : "Spot";
    }
    BindInput(context, ObjectUniform(self, context, 5, objectType, index, modifier), node, "LightAmbient");
    BindInput(context, ObjectUniform(self, context, 6, objectType, index, modifier), node, "LightDiffuse");
    BindInput(context, ObjectUniform(self, context, 7, objectType, index, modifier), node, "LightSpecular");
    BindInput(context, ObjectUniform(self, context, 8, objectType, index, modifier), node, "LightPos");
    if (type != 0) {
        BindInput(context, ObjectUniform(self, context, 0xF, objectType, index, modifier), node, "LightAttenuation");
    }
    if (type != 1) {
        BindInput(context, ObjectUniform(self, context, 0xA, objectType, index, modifier), node, "LightDirection");
    }
    if (type == 2) {
        BindInput(context, ObjectUniform(self, context, 0xE, objectType, index, modifier), node,
                  "LightSpotAttenuation");
    }
    if (uint8_t(specularOn) != 0) {
        if (worldView != nullptr) {
            BindInput(context, worldView, node, "WorldViewVector");
        }
        if (specularPower != nullptr) {
            BindInput(context, specularPower, node, "SpecularPower");
        }
    }
    *ambient = OutputOf(node, "AmbientAccumOut");
    *diffuse = OutputOf(node, "DiffuseAccumOut");
    if (uint8_t(specularOn) != 0) {
        *specular = OutputOf(node, "SpecularAccumOut");
    }
    return 1;
}

// slot 68 HandlePixelFog(context, pixel descriptor, color, color&): the fog applied in the pixel
// program when the descriptor asks for it, except on the PS3 and below ps_3_0 on DX9
uint8_t __fastcall HandlePixelFog(Self* self, re::Edx, MaterialContext* context, NiMaterialDescriptor* desc,
                                  Resource* color, re::Ptr32<Resource>* result) {
    NiFixedString platform = CopyHandle(Configurator_GetPlatform(context->configurator));
    if (FixedString_Equals(&platform, "DX9")) {
        void* cache = Configurator_GetProgramCache(context->configurator);
        NiFixedString target = CopyHandle(Virtual<const re::Ptr32<const char>*>(cache, 6));
        if (!FixedString_Equals(&target, "ps_3_0")) {
            *result = color;
            Drop(target);
            Drop(platform);
            return 1;
        }
        Drop(target);
    }
    if (FixedString_Equals(&platform, "PS3") || (desc->bits[0] & 0x800000) == 0) {
        *result = color;
        Drop(platform);
        return 1;
    }
    void* node = LibraryNode(self, "ApplyFog");
    AddNode(context, node);
    Resource* depth = AddResource(context->inputs, "float", "Fog", "", "FogDepth");
    BindInput(context, depth, node, "FogAmount");
    BindInput(context, color, node, "UnfoggedColor");
    Resource* fogColor = Predefined(self, context, 0x66);
    BindInput(context, fogColor, node, "FogColor");
    *result = OutputOf(node, "FoggedColor");
    Drop(platform);
    return 1;
}

// slot 69 HandleAlphaTest(context, pixel descriptor, opacity): the alpha test in the pixel program
// on D3D10 when the descriptor asks for it
uint8_t __fastcall HandleAlphaTest(Self* self, re::Edx, MaterialContext* context, NiMaterialDescriptor* desc,
                                   Resource* opacity) {
    NiFixedString platform = CopyHandle(Configurator_GetPlatform(context->configurator));
    if (FixedString_Equals(&platform, "D3D10") && (desc->bits[3] & 0x10000000) != 0) {
        void* node = LibraryNode(self, "ApplyAlphaTest");
        AddNode(context, node);
        Resource* function = Predefined(self, context, 0x74);
        BindInput(context, function, node, "AlphaTestFunction");
        Resource* reference = Predefined(self, context, 0x75);
        BindInput(context, reference, node, "AlphaTestRef");
        BindInput(context, opacity, node, "AlphaTestValue");
    }
    Drop(platform);
    return 1;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISTDMAT6_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISTDMAT6_REVERSED("NiStandardMaterial::SetupTransformPipeline", 0x00793010, &ni::SetupTransformPipeline);
NISTDMAT6_REVERSED("NiStandardMaterial::HandleNormalMap", 0x00793740, &ni::HandleNormalMap);
NISTDMAT6_REVERSED("NiStandardMaterial::InsertTexture(map)", 0x00793DB0, &ni::InsertTextureMap);
NISTDMAT6_REVERSED("NiStandardMaterial::HandleVertexFog", 0x00793E90, &ni::HandleVertexFog);
NISTDMAT6_REVERSED("NiStandardMaterial::HandleTextureUVSets", 0x007943E0, &ni::HandleTextureUVSets);
NISTDMAT6_REVERSED("NiStandardMaterial::HandleUVTransform", 0x007946D0, &ni::HandleUVTransform);
NISTDMAT6_REVERSED("NiStandardMaterial::HandleShadow", 0x00794A90, &ni::HandleShadow);
NISTDMAT6_REVERSED("NiStandardMaterial::HandleLight", 0x007951F0, &ni::HandleLight);
NISTDMAT6_REVERSED("NiStandardMaterial::HandlePixelFog", 0x00795DA0, &ni::HandlePixelFog);
NISTDMAT6_REVERSED("NiStandardMaterial::HandleAlphaTest", 0x00796200, &ni::HandleAlphaTest);
NISTDMAT6_REVERSED("NiStandardMaterial::HandleTextureMaps", 0x00797710, &ni::HandleTextureMaps);
NISTDMAT6_REVERSED("NiStandardMaterial::HandleEffectMaps", 0x00797CA0, &ni::HandleEffectMaps);
