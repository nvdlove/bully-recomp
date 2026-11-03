// NiStandardMaterial (src/gamebryo/NiFragmentMaterial.h, NiShadeTree.h): its construction,
// destruction and RTTI, its default shader fallbacks (the functions GenerateShader asks when a
// pass fails: they move texture maps or lights to a new pass, or drop the parallax or shadow
// maps), the render pass descriptions made from its descriptor, the predefined uniforms, the
// texture transforms in the constant map, the programs' check; and NiMaterialFragmentNode's
// construction (the node class of its libraries).
//
// A fallback is cdecl (material descriptor, return code (1 the pixel program, 4 the vertex
// program failed), failed pass, passes, room for passes, pass count&, its own data&): true when
// it changed the passes. The light bits of a pass's pixel descriptor are numbered point lights,
// directional lights, spot lights.

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

constexpr uintptr_t NiStandardMaterial_vtable = 0x00955E60;
constexpr uintptr_t NiMaterialFragmentNode_vtable = 0x00955FC4;
constexpr uintptr_t PrimitiveArray_vtable = 0x009893C0;   // NiTPrimitiveArray<...> (the code blocks)
GAME_VAR(uint8_t, NiStandardMaterial_RTTI, 0x020E3528);
GAME_VAR(uint8_t, NiMaterialFragmentNode_RTTI, 0x020E41B4);
// the literal the semantic is compared with by address first
constexpr uintptr_t kTexTransformLiteral = 0x0095576C;

using Fallback = uint8_t(__cdecl*)(NiMaterialDescriptor* desc, uint32_t rc, uint32_t failedPass,
                                   RenderPassDescriptor* passes, uint32_t maxCount, uint32_t* count, uint32_t* data);

GAME_FN(NiFragmentMaterial*, __thiscall, FragmentMaterial_Construct, 0x00788260, void* self, NiRefObject* library,
        const re::Ptr32<const char>* name, uint32_t vertexVersion, uint32_t geometryVersion, uint32_t pixelVersion,
        int autoCreateCaches);
GAME_FN(void, __thiscall, FragmentMaterial_Destruct, 0x00788390, void* self);
GAME_FN(void, __thiscall, FragmentMaterial_AddDefaultFallbacks, 0x0049A650, void* self);   // (empty)
GAME_FN(uint32_t, __thiscall, AddShaderFallbackFunc, 0x007879A0, void* self, void* function);
GAME_FN(uint8_t, __thiscall, FragmentMaterial_AddResourceToShaderConstantMap, 0x00786D40, void* self,
        NiRefObject* shader, void* map, void* resource, NiMaterialDescriptor* vertexDesc,
        NiMaterialDescriptor* geometryDesc, NiMaterialDescriptor* pixelDesc, void* offsets, uint32_t pass,
        uint32_t programType);
GAME_FN(uint8_t, __cdecl, ConstantMap_LookUpPredefinedMappingName, 0x0077F610, uint32_t mapping, re::Ptr32<const char>* name);
GAME_FN(uint32_t, __cdecl, ConstantMap_GetPredefinedMappingType, 0x0077EF10, uint32_t mapping, uint32_t registers);
// NiMaterialNode's and NiString's (the fragment node's base and description)
GAME_FN(void, __thiscall, MaterialNode_Construct, 0x007E3060, void* self);
GAME_FN(void, __thiscall, MaterialNode_Destruct, 0x007E2FB0, void* self);
GAME_FN(void, __thiscall, NiString_ConstructSized, 0x007740D0, void* self, uint32_t size);
GAME_FN(void, __cdecl, NiTMalloc_Free, 0x00836F80, void* p);
// the fallbacks (this file's, called where modes apply)
GAME_FN(uint8_t, __cdecl, SplitTextureMaps_, 0x007902A0, NiMaterialDescriptor* desc, uint32_t rc, uint32_t failedPass,
        RenderPassDescriptor* passes, uint32_t maxCount, uint32_t* count, uint32_t* data);
GAME_FN(uint8_t, __cdecl, DropParallaxMap_, 0x00790B40, NiMaterialDescriptor* desc, uint32_t rc, uint32_t failedPass,
        RenderPassDescriptor* passes, uint32_t maxCount, uint32_t* count, uint32_t* data);
GAME_FN(uint8_t, __cdecl, SplitPerPixelLights_, 0x00796D90, NiMaterialDescriptor* desc, uint32_t rc,
        uint32_t failedPass, RenderPassDescriptor* passes, uint32_t maxCount, uint32_t* count, uint32_t* data);

// ---- the fallbacks' pieces

// the texture usage (UV set, texgen) of a pixel program's map appended, when the program has it
struct Usages {
    uint32_t uvSets[16];
    uint32_t texGens[16];
    uint32_t count;

    void Reset() {
        crt::Memset(uvSets, 0xFF, sizeof(uvSets));
        crt::Memset(texGens, 0, sizeof(texGens));
        count = 0;
    }
    // a fixed map (0..12), its index as the code inlines it
    bool Add(NiMaterialDescriptor* desc, uint32_t p0, uint32_t map) {
        if (((p0 >> (7 + map)) & 1) == 0) {
            return false;
        }
        int32_t index = MapIndex(p0, map);
        if (index < 0) {
            return false;
        }
        StandardDesc_GetTextureUsage(desc, uint32_t(index), &uvSets[count], &texGens[count]);
        count++;
        return true;
    }
    // the decals (their index from MapIndexFromTextureEnum)
    void AddDecals(NiMaterialDescriptor* desc, NiMaterialDescriptor* pixel) {
        uint32_t decals = (pixel->bits[0] >> 20) & 7;
        for (uint32_t i = 0; i < decals; i++) {
            int32_t index = MapIndexFromTextureEnum(i + 13, pixel);
            if (index >= 0) {
                StandardDesc_GetTextureUsage(desc, uint32_t(index), &uvSets[count], &texGens[count]);
                count++;
            }
        }
    }
};

// the passes from the failed one on moved up one (the new pass after the failed one its copy)
void MovePassesUp(RenderPassDescriptor* passes, uint32_t failedPass, uint32_t count) {
    for (uint32_t k = count; k > failedPass; k--) {
        RenderPass_Assign(&passes[k], &passes[k - 1]);
    }
}

// a new pass added onto the frame (alpha blended with the previous source blend, destination 0)
void SetAddedPassBlending(RenderPassDescriptor* pass) {
    pass->alphaOverride = 1;
    pass->alphaBlend = 1;
    pass->usePreviousSrcBlendMode = 1;
    pass->usePreviousDestBlendMode = 0;
    pass->srcBlendMode = 0;
    pass->destBlendMode = 0;
}

// the bit of a light (as the shifts give it: their counts taken mod 32)
inline uint32_t LightBit(uint32_t index) {
    return (0xFFFFFFFFu << ((index + 1) & 31)) ^ (0xFFFFFFFFu << (index & 31));
}

// the failed pass's shadow maps dropped (its pixel program's shadow mask), when the pixel program
// failed (0x790270's code, inlined in the fallbacks that start with it)
uint8_t DropShadowMaps(uint32_t rc, uint32_t failedPass, RenderPassDescriptor* passes) {
    if ((rc & 1) == 0) {
        return 0;
    }
    uint8_t* mask = reinterpret_cast<uint8_t*>(&passes[failedPass].pixelDesc.bits[3]) + 1;
    if (*mask == 0) {
        return 0;
    }
    *mask = 0;
    return 1;
}

// ---- the fallbacks

// SplitTextureMaps: a failing pixel or vertex program with a glow or environment map: its bump,
// glow, environment (and, without specular, gloss) maps moved to a new pass (with the parallax,
// base and normal maps; no lights, decals, dark or detail maps, projections)
uint8_t __cdecl SplitTextureMaps(NiMaterialDescriptor* desc, uint32_t rc, uint32_t failedPass,
                                 RenderPassDescriptor* passes, uint32_t maxCount, uint32_t* count, uint32_t*) {
    if ((rc & 5) == 0) {
        return 0;
    }
    uint32_t passCount = *count;
    if (passCount == maxCount - 1) {
        return 0;
    }
    RenderPassDescriptor* pass = &passes[failedPass];
    NiMaterialDescriptor* pixel = &pass->pixelDesc;
    uint8_t specular = uint8_t(pixel->bits[3] >> 16) & 1;
    uint8_t notPerVertex = uint8_t(~uint8_t(pixel->bits[1] >> 11)) & 1;
    uint32_t lights = ((pixel->bits[3] >> 4) & 0xF) + (pixel->bits[2] >> 28) + (pixel->bits[3] & 0xF);
    if ((pixel->bits[0] & 0x7004000) == 0) {
        return 0;
    }
    MovePassesUp(passes, failedPass, passCount);
    RenderPassDescriptor* added = &passes[failedPass + 1];
    NiMaterialDescriptor* addedPixel = &added->pixelDesc;

    Usages usages;
    usages.Reset();
    static const uint8_t kKept[] = {0, 1, 2, 3, 4};
    for (uint8_t map : kKept) {
        usages.Add(desc, pixel->bits[0], map);
    }
    if (specular != 0) {
        usages.Add(desc, pixel->bits[0], 6);
    }
    for (uint32_t map = 8; map <= 12; map++) {
        usages.Add(desc, pixel->bits[0], map);
    }
    usages.AddDecals(desc, pixel);
    pass->vertexDesc.bits[0] &= 0xFFFFC7FF;
    if (specular == 0 || notPerVertex == 0) {
        pass->vertexDesc.bits[0] &= ~0x10u;
        pixel->bits[0] &= ~0x10u;
    }
    pixel->bits[0] &= 0xF8FFAFFF;
    if (specular == 0) {
        pixel->bits[0] &= 0xFFFFDFFF;
    }
    AssignTextureCoordinates(usages.uvSets, usages.texGens, usages.count, &pass->vertexDesc, pixel);

    usages.Reset();
    usages.Add(desc, addedPixel->bits[0], 0);
    if (usages.Add(desc, addedPixel->bits[0], 1)) {
        addedPixel->bits[3] |= 0x400000;
    }
    static const uint8_t kMoved[] = {2, 5, 6, 7};
    for (uint8_t map : kMoved) {
        usages.Add(desc, addedPixel->bits[0], map);
    }
    added->vertexDesc.bits[0] &= 0xFFE73FFF;
    if (notPerVertex == 0 && lights == 0) {
        pass->vertexDesc.bits[0] &= ~4u;
        pixel->bits[0] &= ~2u;
    }
    addedPixel->bits[0] &= 0xE78FF3FF;
    addedPixel->bits[1] &= 0xFFFFFFE7;
    addedPixel->bits[2] &= 0x0FFFFFFF;
    addedPixel->bits[3] &= 0xFF5FFF00;
    added->vertexDesc.bits[0] &= ~4u;
    addedPixel->bits[0] &= ~2u;
    AssignTextureCoordinates(usages.uvSets, usages.texGens, usages.count, &added->vertexDesc, addedPixel);
    added->srcBlendMode = 0;
    added->destBlendMode = 0;
    added->alphaOverride = 1;
    added->alphaBlend = 1;
    added->usePreviousSrcBlendMode = 1;
    added->usePreviousDestBlendMode = 0;
    *count = *count + 1;
    return 1;
}

// DropShadowMapsThenSplitTextureMaps: the shadow maps dropped the first time, then SplitTextureMaps
uint8_t __cdecl DropShadowMapsThenSplitTextureMaps(NiMaterialDescriptor* desc, uint32_t rc, uint32_t failedPass,
                                                   RenderPassDescriptor* passes, uint32_t maxCount, uint32_t* count,
                                                   uint32_t* data) {
    if (*data != 0) {
        return SplitTextureMaps_(desc, rc, failedPass, passes, maxCount, count, data);
    }
    *data = 1;
    return DropShadowMaps(rc, failedPass, passes);
}

// DropParallaxMap: the failed pass's parallax map dropped (its texture coordinates assigned again
// without it; its binormals and tangents too when nothing else needs them)
uint8_t __cdecl DropParallaxMap(NiMaterialDescriptor* desc, uint32_t rc, uint32_t failedPass,
                                RenderPassDescriptor* passes, uint32_t, uint32_t*, uint32_t*) {
    if ((rc & 5) == 0) {
        return 0;
    }
    RenderPassDescriptor* pass = &passes[failedPass];
    NiMaterialDescriptor* pixel = &pass->pixelDesc;
    if (((pixel->bits[0] >> 7) & 1) == 0) {
        return 0;
    }
    uint8_t notPerVertex = uint8_t(~uint8_t(pixel->bits[1] >> 11)) & 1;
    uint32_t lights = ((pixel->bits[3] >> 4) & 0xF) + (pixel->bits[2] >> 28) + (pixel->bits[3] & 0xF);
    Usages usages;
    usages.Reset();
    usages.Add(desc, pixel->bits[0], 1);
    uint32_t normalMap = (pixel->bits[0] >> 9) & 1;
    for (uint32_t map = 2; map <= 12; map++) {
        usages.Add(desc, pixel->bits[0], map);
    }
    usages.AddDecals(desc, pixel);
    pixel->bits[0] &= ~0x80u;
    pass->vertexDesc.bits[0] &= ~0x20u;
    if ((notPerVertex == 0 || lights == 0) && normalMap == 0) {
        pass->vertexDesc.bits[0] &= ~8u;
        pixel->bits[0] &= ~4u;
        pixel->bits[0] &= ~8u;
    }
    AssignTextureCoordinates(usages.uvSets, usages.texGens, usages.count, &pass->vertexDesc, pixel);
    return 1;
}

// SplitPerPixelLights: a failing pixel program lighting per pixel with more than one light (or
// one and the ambient): half its lights of each kind (the last ones, half rounded up, no more than
// half rounded up of all) moved to a new pass with their shadow bits and object offsets
uint8_t __cdecl SplitPerPixelLights(NiMaterialDescriptor*, uint32_t rc, uint32_t failedPass,
                                    RenderPassDescriptor* passes, uint32_t maxCount, uint32_t* count, uint32_t*) {
    if ((rc & 1) == 0) {
        return 0;
    }
    uint32_t passCount = *count;
    if (passCount == maxCount - 1) {
        return 0;
    }
    NiMaterialDescriptor* pixel = &passes[failedPass].pixelDesc;
    if ((pixel->bits[1] & 0x800) != 0) {
        return 0;
    }
    uint32_t p2 = pixel->bits[2];
    uint32_t p3 = pixel->bits[3];
    uint8_t emissive = uint8_t(p3 >> 23) & 1;
    uint32_t directional = (p3 >> 4) & 0xF;
    uint32_t spot = p3 & 0xF;
    uint32_t point = p2 >> 28;
    uint8_t ambient = uint8_t(p3 >> 21) & 1;
    uint32_t total = point + spot + directional;
    if (total == 0) {
        return 0;
    }
    if (total == 1 && ambient == 0) {
        return 0;
    }
    MovePassesUp(passes, failedPass, passCount);
    RenderPassDescriptor* added = &passes[failedPass + 1];
    uint32_t budget = total - (total >> 1);
    uint32_t spotNew = spot - (spot >> 1);
    added->objectOffsets.count = 0;
    added->resetObjectOffsets = 0;
    if (spotNew > budget) {
        spotNew = budget;
    }
    budget -= spotNew;
    uint32_t directionalNew = directional - (directional >> 1);
    if (directionalNew > budget) {
        directionalNew = budget;
    }
    budget -= directionalNew;
    uint32_t pointNew = point - (point >> 1);
    if (pointNew > budget) {
        pointNew = budget;
    }
    uint32_t directionalOld = directional - directionalNew;
    uint32_t pointOld = point - pointNew;
    uint32_t spotOld = spot - spotNew;
    NiMaterialDescriptor* addedPixel = &added->pixelDesc;
    addedPixel->bits[3] = (addedPixel->bits[3] & 0xFFFFFF0F) | (directionalNew << 4);
    addedPixel->bits[2] = (addedPixel->bits[2] & 0x0FFFFFFF) | (pointNew << 28);
    addedPixel->bits[3] = (addedPixel->bits[3] & 0xFFFFFFF0) | spotNew;
    addedPixel->bits[3] &= 0xFF5FFFFF;
    reinterpret_cast<uint8_t*>(&addedPixel->bits[3])[1] = 0;

    // the moved lights' shadow bits: from their place among the old ones to their new place
    uint32_t oldMask = uint8_t(pixel->bits[3] >> 8);
    uint32_t newMask = 0;
    uint32_t to = 0;
    uint32_t from = pointOld;
    auto move = [&](uint32_t n) {
        for (uint32_t i = 0; i < n; i++, to++, from++) {
            uint32_t bit = LightBit(from);
            if ((oldMask & bit) != 0) {
                oldMask &= ~bit;
                newMask |= LightBit(to);
            }
        }
    };
    move(pointNew);
    from += directionalOld;
    move(directionalNew);
    from += spotOld;
    move(spotNew);
    addedPixel->bits[3] = (addedPixel->bits[3] & 0xFFFF00FF) | (newMask << 8);
    pixel->bits[3] = (pixel->bits[3] & 0xFFFF00FF) | (oldMask << 8);
    SetAddedPassBlending(added);

    // the moved lights' object offsets (in the pixel program) past the ones that stay
    auto addOffset = [&](uint32_t objectType, uint32_t base) {
        ObjectResourceOffset offset;
        ObjectOffset_Construct(&offset);
        offset.objectType = objectType;
        offset.programType = 1;
        offset.offset = GetHighestObjectOffset(objectType, passes, passCount) + base;
        NiTSet<ObjectResourceOffset>& set = added->objectOffsets;
        if (set.count == set.allocated) {
            ObjectOffsets_Realloc(&set, set.allocated != 0 ? set.allocated * 2 : 1);
        }
        set.base[set.count] = offset;
        set.count += 1;
    };
    if (directionalNew != 0) {
        addOffset(3, directionalOld);
    }
    if (spotNew != 0) {
        addOffset(4, spotOld);
    }
    if (pointNew != 0) {
        addOffset(2, pointOld);
    }
    pixel->bits[3] = (pixel->bits[3] & 0xFFFFFF0F) | (directionalOld << 4);
    pixel->bits[2] = (pixel->bits[2] & 0x0FFFFFFF) | (pointOld << 28);
    pixel->bits[3] = (((pixel->bits[3] & 0xFFFFFFF0) | spotOld) & 0xFFDFFFFF) | (uint32_t(ambient) << 21);
    pixel->bits[3] = (pixel->bits[3] & 0xFF7FFFFF) | (uint32_t(emissive) << 23);
    *count = *count + 1;
    return 1;
}

// DropShadowMapsThenSplitPerPixelLights
uint8_t __cdecl DropShadowMapsThenSplitPerPixelLights(NiMaterialDescriptor* desc, uint32_t rc, uint32_t failedPass,
                                                      RenderPassDescriptor* passes, uint32_t maxCount,
                                                      uint32_t* count, uint32_t* data) {
    if (*data != 0) {
        return SplitPerPixelLights_(desc, rc, failedPass, passes, maxCount, count, data);
    }
    *data = 1;
    return DropShadowMaps(rc, failedPass, passes);
}

// SplitPerVertexLights: a failing vertex program lighting per vertex: half its lights of each kind
// moved to a new pass with their object offsets (in the vertex program)
uint8_t __cdecl SplitPerVertexLights(NiMaterialDescriptor*, uint32_t rc, uint32_t failedPass,
                                     RenderPassDescriptor* passes, uint32_t maxCount, uint32_t* count, uint32_t*) {
    if ((rc & 4) == 0) {
        return 0;
    }
    uint32_t passCount = *count;
    if (passCount == maxCount - 1) {
        return 0;
    }
    RenderPassDescriptor* pass = &passes[failedPass];
    uint32_t v2 = pass->vertexDesc.bits[2];
    uint32_t directional = (v2 >> 20) & 0xF;
    uint32_t spot = (v2 >> 16) & 0xF;
    uint32_t point = (v2 >> 12) & 0xF;
    uint32_t total = point + spot + directional;
    if ((pass->pixelDesc.bits[1] & 0x800) == 0 || total == 0) {
        return 0;
    }
    MovePassesUp(passes, failedPass, passCount);
    RenderPassDescriptor* added = &passes[failedPass + 1];
    uint32_t budget = total - (total >> 1);
    uint32_t spotNew = spot - (spot >> 1);
    added->objectOffsets.count = 0;
    added->resetObjectOffsets = 0;
    if (spotNew > budget) {
        spotNew = budget;
    }
    budget -= spotNew;
    uint32_t pointNew = point - (point >> 1);
    if (pointNew > budget) {
        pointNew = budget;
    }
    budget -= pointNew;
    uint32_t directionalNew = directional - (directional >> 1);
    if (directionalNew > budget) {
        directionalNew = budget;
    }
    uint32_t pointOld = point - pointNew;
    uint32_t v = added->vertexDesc.bits[2];
    v = (v & 0xFF0FFFFF) | (directionalNew << 20);
    v = (v & 0xFFFF0FFF) | (pointNew << 12);
    v = (v & 0xFFF0FFFF) | (spotNew << 16);
    added->vertexDesc.bits[2] = v;
    uint32_t spotOld = spot - spotNew;
    uint32_t directionalOld = directional - directionalNew;
    SetAddedPassBlending(added);
    auto addOffset = [&](uint32_t objectType, uint32_t base) {
        ObjectResourceOffset offset;
        ObjectOffset_Construct(&offset);
        offset.objectType = objectType;
        offset.programType = 0;
        offset.offset = GetHighestObjectOffset(objectType, passes, passCount) + base;
        ObjectOffsets_Add(&added->objectOffsets, &offset);
    };
    if (directionalNew != 0) {
        addOffset(3, directionalOld);
    }
    if (spotNew != 0) {
        addOffset(4, spotOld);
    }
    if (pointNew != 0) {
        addOffset(2, pointOld);
    }
    v = pass->vertexDesc.bits[2];
    v = (v & 0xFF0FFFFF) | (directionalOld << 20);
    v = (v & 0xFFFF0FFF) | (pointOld << 12);
    v = (v & 0xFFF0FFFF) | (spotOld << 16);
    pass->vertexDesc.bits[2] = v;
    *count = *count + 1;
    return 1;
}

// DropParallaxMapThenSplitLights: the parallax map dropped the first time, then the per-pixel
// lights split
uint8_t __cdecl DropParallaxMapThenSplitLights(NiMaterialDescriptor* desc, uint32_t rc, uint32_t failedPass,
                                               RenderPassDescriptor* passes, uint32_t maxCount, uint32_t* count,
                                               uint32_t* data) {
    if (*data == 0) {
        *data = 1;
        return DropParallaxMap_(desc, rc, failedPass, passes, maxCount, count, data);
    }
    return SplitPerPixelLights_(desc, rc, failedPass, passes, maxCount, count, data);
}

// DropShadowMapsThenDropParallaxMapThenSplitLights
uint8_t __cdecl DropShadowMapsThenDropParallaxMapThenSplitLights(NiMaterialDescriptor* desc, uint32_t rc,
                                                                 uint32_t failedPass, RenderPassDescriptor* passes,
                                                                 uint32_t maxCount, uint32_t* count, uint32_t* data) {
    uint32_t step = *data;
    if (step == 0) {
        *data = 1;
        return DropShadowMaps(rc, failedPass, passes);
    }
    if (step == 1) {
        *data = 2;
        return DropParallaxMap_(desc, rc, failedPass, passes, maxCount, count, data);
    }
    return SplitPerPixelLights_(desc, rc, failedPass, passes, maxCount, count, data);
}

// GetUVForMap(map, pixel descriptor, uvs[]): the UV coordinates of the map's set (uvs[-1] when the
// pixel program hasn't the map)
void* __cdecl GetUVForMapImpl(uint32_t map, NiMaterialDescriptor* pixelDesc, re::Ptr32<void>* uvs) {
    int32_t index = MapIndexFromTextureEnum(map, pixelDesc);
    if (index == -1) {
        return uvs[-1];
    }
    return uvs[PixelDesc_GetUVSetForMap(pixelDesc, uint32_t(index))];
}

// slot 13 AddDefaultFallbacks: the fallbacks in the order they're tried, then the base's (empty;
// what comes back is the last index, left in eax)
uint32_t __fastcall AddDefaultFallbacks(Self* self, re::Edx) {
    static const uintptr_t kFallbacks[] = {0x00796D90, 0x00790270, 0x007972E0, 0x00797320, 0x007902A0,
                                           0x00790B00, 0x00790B40, 0x007975C0, 0x007975F0};
    uint32_t index = 0;
    for (uintptr_t fallback : kFallbacks) {
        index = AddShaderFallbackFunc(self, reinterpret_cast<void*>(fallback));
    }
    FragmentMaterial_AddDefaultFallbacks(self);
    return index;
}

// ---- the material's interface

// slot 24 GenerateShaderDescArray(descriptor, passes, (room), count&): one pass, its vertex and
// pixel program descriptors made from the material descriptor's bits (the lights in the vertex
// program, or in the pixel program with lighting per pixel), its texture coordinates assigned.
// 0 on success, 8 for a descriptor of another kind
uint32_t __fastcall GenerateShaderDescArray(Self*, re::Edx, NiMaterialDescriptor* desc, RenderPassDescriptor* passes,
                                            uint32_t, uint32_t* count) {
    *count = 0;
    if (!FixedString_Equals(&desc->identifier, "NiStandardMaterialDescriptor")) {
        return 8;
    }
    RenderPassDescriptor* pass = passes;
    pass->usesNiRenderState = 1;
    pass->resetObjectOffsets = 1;
    FixedString_AssignText(&pass->vertexDesc.identifier, "NiStandardVertexProgramDescriptor");
    NiMaterialDescriptor* pixel = &pass->pixelDesc;
    FixedString_AssignText(&pixel->identifier, "NiStandardPixelProgramDescriptor");
    const uint32_t* m = desc->bits;
    uint32_t* v = pass->vertexDesc.bits;
    uint32_t* p = pixel->bits;

    uint32_t lighting = (m[0] >> 9) & 1;
    p[0] = (p[0] & ~1u) | lighting;
    p[0] ^= ((m[0] >> 13) ^ p[0]) & 0x60;
    uint32_t parallax = (m[0] >> 15) & 1;
    p[0] = (p[0] & ~0x80u) | (parallax << 7);
    p[0] ^= ((m[0] >> 8) ^ p[0]) & 0x100;
    uint32_t normalMap = (m[0] >> 17) & 1;
    p[0] = (p[0] & ~0x200u) | (normalMap << 9);
    p[0] ^= ((m[0] >> 10) ^ p[0]) & 0x400;
    p[0] ^= ((m[0] >> 10) ^ p[0]) & 0x800;
    p[0] ^= ((m[0] >> 10) ^ p[0]) & 0x1000;
    p[0] ^= ((m[0] >> 10) ^ p[0]) & 0x2000;
    p[0] ^= ((m[0] >> 10) ^ p[0]) & 0x4000;
    p[0] ^= ((m[0] >> 13) ^ p[0]) & 0x8000;
    p[0] ^= ((m[0] >> 13) ^ p[0]) & 0x10000;
    p[0] ^= ((m[0] >> 13) ^ p[0]) & 0x20000;
    p[0] ^= ((m[0] >> 13) ^ p[0]) & 0x40000;
    p[0] ^= ((m[1] << 19) ^ p[0]) & 0x80000;
    p[0] = (p[0] & 0xFF8FFFFF) | ((m[1] & 6) << 19);
    uint32_t projectedLights = uint8_t(m[1] >> 24) & 3;
    p[0] = (p[0] & 0xE7FFFFFF) | (projectedLights << 27);
    uint32_t projectedShadows = m[2] & 3;
    p[1] = (p[1] & ~0x18u) | (projectedShadows * 8);
    p[1] ^= ((m[2] * 8) ^ p[1]) & 0xE0;
    p[0] = ((p[0] ^ (m[1] * 8)) & 0x1FFFFFFF) ^ (m[1] * 8);
    p[1] ^= ((m[2] * 8) ^ p[1]) & 0x700;
    p[1] = (p[1] & ~7u) | (m[1] >> 29);
    uint32_t specular = (m[0] >> 4) & 1;
    p[3] = (p[3] & 0xFFFEFFFF) | (specular << 16);
    uint32_t vertexColorMode = (m[0] >> 6) & 3;
    p[3] = (p[3] & 0xFFF9FFFF) | (vertexColorMode << 17);
    uint32_t lightingMode = (m[0] >> 8) & 1;
    p[3] = (p[3] & 0xFFE7FFFF) | (lightingMode << 19);
    uint32_t perVertex = (m[1] >> 3) & 1;
    p[1] = (p[1] & ~0x800u) | (perVertex << 11);
    uint32_t fog = (m[0] >> 10) & 3;
    p[0] = (p[0] & 0xFF7FFFFF) | (uint32_t(fog != 0 ? 1 : 0) << 23);
    p[3] ^= ((m[3] >> 2) ^ p[3]) & 0x10000000;
    void* manager = ms_pkShadowManager;
    if (manager != nullptr && At<uint8_t>(manager, 0xBC) != 0) {
        p[3] = (p[3] & 0xFFFF00FF) | (uint32_t(uint8_t(m[1] >> 16)) << 8);
        void* technique = At<void*>(ms_pkShadowManager, int((m[3] >> 28) & 3) * 4);
        p[3] = (p[3] & 0xF0FFFFFF) | (uint32_t(At<uint16_t>(technique, 0x46)) << 24);
    } else {
        p[3] &= 0xF0FFFFFF;
    }

    // the lights: in the vertex program (or none), or in the pixel program
    uint32_t pointCount = (m[1] >> 4) & 0xF;
    uint32_t shadowMask = uint8_t(m[1] >> 16);
    uint32_t spotCount = (m[1] >> 8) & 0xF;
    uint32_t directionalCount = (m[1] >> 12) & 0xF;
    uint32_t total = spotCount + pointCount + directionalCount;
    if (lighting == 0 || perVertex != 0) {
        if (lighting == 0) {
            pointCount = 0;
            spotCount = 0;
            directionalCount = 0;
            specular = 0;
        }
        v[2] = (v[2] & 0xFFFF0FFF) | (pointCount << 12);
        v[2] = (v[2] & 0xFFF0FFFF) | (spotCount << 16);
        v[2] = (v[2] & 0xFF0FFFFF) | (directionalCount << 20);
        v[0] = (v[0] & ~0x100u) | (specular << 8);
        p[3] &= 0xFF5FFFFF;
        v[2] |= 0x2000000;
    } else {
        p[2] = (p[2] & 0x0FFFFFFF) | (pointCount << 28);
        p[3] = (p[3] & 0xFFFFFFF0) | spotCount;
        p[3] = (p[3] & 0xFFFFFF0F) | (directionalCount << 4);
        p[3] = (p[3] & 0xFFFF00FF) | (shadowMask << 8);
        p[3] |= 0xA00000;
        if (specular != 0) {
            v[0] = (v[0] & ~0x10u) | (specular << 4);
            p[0] |= 0x10;
        }
        if (total != 0) {
            v[0] |= 4;
            v[0] |= 8;
            p[0] |= 4;
            p[0] |= 2;
            v[2] &= 0xFDFFFFFF;
        } else {
            v[2] |= 0x2000000;
            p[1] |= 0x800;
        }
    }

    v[0] ^= (v[0] ^ m[0]) & 3;
    v[0] ^= ((m[0] << 4) ^ v[0]) & 0xC0;
    v[0] = (v[0] & 0xFFFFF9FF) | (fog << 9);
    v[2] ^= ((m[0] << 19) ^ v[2]) & 0x1000000;
    v[0] = (v[0] & 0xFFFF3FFF) | (projectedLights << 14);
    v[0] = (v[0] & 0xFFE7FFFF) | (projectedShadows << 19);
    uint32_t texGen = (m[0] >> 25) & 7;
    if (normalMap != 0) {
        v[0] |= 8;
        p[0] |= 8;
        v[0] &= 0xFFFFC7FF;
        p[0] = (p[0] & 0xF8FFFFFF) | (texGen << 24);
        if (texGen == 5 || texGen == 3) {
            v[0] |= 0x10;
            p[0] |= 0x10;
        }
    } else {
        v[0] = (v[0] & 0xFFFFC7FF) | (texGen << 11);
        p[0] = (p[0] & 0xF8FFFFFF) | (texGen << 24);
    }
    if (parallax != 0) {
        v[0] |= 0x20;
        v[0] |= 8;
        p[0] |= 8;
    }
    if ((uint8_t(p[1]) & 7) != 0 || (p[1] & 0x700) != 0) {
        v[0] |= 4;
        p[0] |= 2;
    }

    // the texture coordinates
    uint32_t uvSets[16];
    uint32_t texGens[16];
    crt::Memset(uvSets, 0xFF, sizeof(uvSets));
    crt::Memset(texGens, 0, sizeof(texGens));
    uint32_t textures = StandardDesc_GetInputCount(desc);
    for (uint32_t i = 0; i < textures; i++) {
        StandardDesc_GetTextureUsage(desc, i, &uvSets[i], &texGens[i]);
    }
    AssignTextureCoordinates(uvSets, texGens, textures, &pass->vertexDesc, pixel);
    uint32_t v2 = v[2];
    v2 = (v2 & 0xF3FFFFFF) | (vertexColorMode << 26);
    v2 = (v2 & 0xCFFFFFFF) | (lightingMode << 28);
    v2 = (v2 & 0xBFFFFFFF) | (lighting << 30);
    v[2] = v2;
    *count = *count + 1;
    return 0;
}

// slot 25 VerifyShaderPrograms(vertex, geometry, pixel): 4 without a vertex program, 1 without a
// pixel program
uint32_t __fastcall VerifyShaderPrograms(Self*, re::Edx, void* vertex, void*, void* pixel) {
    uint32_t rc = 0;
    if (vertex == nullptr) {
        rc = 4;
    }
    if (pixel == nullptr) {
        rc |= 1;
    }
    return rc;
}

// slot 26 AddResourceToShaderConstantMap: a predefined TexTransform's entry is its texture map's
// transform mapping (the map whose UV set is the resource's extra data), with the map's occurrence;
// the rest NiFragmentMaterial's
uint8_t __fastcall AddResourceToShaderConstantMap(Self* self, re::Edx, NiRefObject* shader, void* map, void* resource,
                                                  NiMaterialDescriptor* vertexDesc,
                                                  NiMaterialDescriptor* geometryDesc,
                                                  NiMaterialDescriptor* pixelDesc, void* offsets, uint32_t pass,
                                                  uint32_t programType) {
    if (Resource_GetSource(resource) == 2) {
        const char* semantic = *Resource_GetSemantic(resource);
        if (semantic == reinterpret_cast<const char*>(kTexTransformLiteral) ||
            (semantic != nullptr && NiStricmp(semantic, "TexTransform") == 0)) {
            // (the identifier compared before the descriptor is checked, as the game has it)
            if (!FixedString_Equals(&pixelDesc->identifier, "NiStandardPixelProgramDescriptor") ||
                pixelDesc == nullptr) {
                return 0;
            }
            uint32_t textureMap = 0;
            for (; textureMap < 0x2F; textureMap++) {
                int32_t index = MapIndexFromTextureEnum(textureMap, pixelDesc);
                uint32_t set = index == -1 ? 0xFFFFFFFF : PixelDesc_GetUVSetForMap(pixelDesc, uint32_t(index));
                if (set == Resource_GetExtraData(resource)) {
                    break;
                }
            }
            uint32_t occurrence = Resource_GetExtraData(resource);
            uint32_t mapping = programType;
            if (!GetTextureTransformMappingFromEnum(textureMap, &mapping, &occurrence)) {
                return 0;
            }
            NiFixedString name = NullFixedString;
            if (!ConstantMap_LookUpPredefinedMappingName(mapping, &name)) {
                FixedString_Destruct(&name);
                return 0;
            }
            const char* variable = *Resource_GetVariable(resource);
            // AddPredefinedEntry(key, extra, flags, variable)
            uint32_t error = Virtual<uint32_t>(map, 3, name, occurrence, 0u, variable);
            FixedString_Destruct(&name);
            return error == 0 ? 1 : 0;
        }
    }
    return FragmentMaterial_AddResourceToShaderConstantMap(self, shader, map, resource, vertexDesc, geometryDesc,
                                                           pixelDesc, offsets, pass, programType);
}

// slot 31 AddOutputPredefined(node, mapping, registers, count, extra): a uniform for the predefined
// mapping, "g_<name>" (and the extra, past 0), its type the mapping's for that many registers
void* __fastcall AddOutputPredefined(Self* self, re::Edx, void* node, uint32_t mapping, uint32_t registers,
                                     uint32_t count, uint32_t extra) {
    NiFixedString name = NullFixedString;
    ConstantMap_LookUpPredefinedMappingName(mapping, &name);
    uint32_t attributeType = ConstantMap_GetPredefinedMappingType(mapping, registers);
    NiFixedString type;
    Virtual<re::Ptr32<const char>*>(self, 17, &type, attributeType);   // GetStringFromAttributeType
    char text[0x100];
    if (extra == 0) {
        NiSprintf(text, 0x100, "g_%s", name);
    } else {
        NiSprintf(text, 0x100, "g_%s%d", name, extra);
    }
    NiFixedString variable = String(text);
    NiFixedString label = String("");
    void* resource = Node_AddOutputResource(node, &type, &name, &label, &variable, count, 2, 0, extra);
    Drop(label);
    Drop(variable);
    Drop(type);
    Drop(name);
    return resource;
}

// ---- construction

// NiStandardMaterial(name, library, vertex, geometry and pixel versions, create caches): the
// derived materials' constructor
Self* __fastcall ConstructNamed(Self* self, re::Edx, const re::Ptr32<const char>* name, NiRefObject* library,
                                uint32_t vertexVersion, uint32_t geometryVersion, uint32_t pixelVersion,
                                int autoCreateCaches) {
    FragmentMaterial_Construct(self, library, name, vertexVersion, geometryVersion, pixelVersion, autoCreateCaches);
    self->vtable = reinterpret_cast<void*>(NiStandardMaterial_vtable);
    self->descriptorName = String("NiStandardMaterialDescriptor");
    self->unknown3BC = 0;
    self->unknown3BD = 0;
    self->unknown3BE = 0;
    return self;
}

// NiStandardMaterial(library, create caches): "NiStandardMaterial", versions 0x10, 0, 0x1A
Self* __fastcall Construct(Self* self, re::Edx, NiRefObject* library, int autoCreateCaches) {
    NiFixedString name = String("NiStandardMaterial");
    FragmentMaterial_Construct(self, library, &name, 0x10, 0, 0x1A, autoCreateCaches);
    Drop(name);
    self->vtable = reinterpret_cast<void*>(NiStandardMaterial_vtable);
    self->descriptorName = String("NiStandardMaterialDescriptor");
    self->unknown3BC = 0;
    self->unknown3BD = 0;
    self->unknown3BE = 0;
    return self;
}

const void* __fastcall GetRTTI(Self*, re::Edx) {
    return &NiStandardMaterial_RTTI;
}

// the deleting destructor: the descriptor's name let go, NiFragmentMaterial's destructor
Self* __fastcall DeletingDestructor(Self* self, re::Edx, uint32_t flags) {
    Drop(self->descriptorName);
    FragmentMaterial_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x3C0);
    }
    return self;
}

// ---- NiMaterialFragmentNode (0x44 bytes: NiMaterialNode's 0x30, the code blocks' NiTPrimitiveArray
// at +0x30, the description (an NiString of 0x100) at +0x40)

struct NiMaterialFragmentNode {
    re::Ptr32<void> vtable;              // +0x0
    uint8_t node[0x2C];        // +0x4 (NiMaterialNode's)
    re::Ptr32<void> blocksVtable;        // +0x30
    re::Ptr32<void> blocks;              // +0x34
    uint16_t blocksMax;        // +0x38
    uint16_t blocksSize;       // +0x3A
    uint16_t blocksUsed;       // +0x3C
    uint16_t blocksGrowBy;     // +0x3E
    NiString description;      // +0x40
};
static_assert(offsetof(NiMaterialFragmentNode, blocksVtable) == 0x30);
static_assert(offsetof(NiMaterialFragmentNode, description) == 0x40);
static_assert(sizeof(NiMaterialFragmentNode) == 0x44);

NiMaterialFragmentNode* __fastcall FragmentNodeConstruct(NiMaterialFragmentNode* self, re::Edx) {
    MaterialNode_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiMaterialFragmentNode_vtable);
    self->blocksMax = 0;
    self->blocksGrowBy = 1;
    self->blocksSize = 0;
    self->blocksUsed = 0;
    self->blocks = nullptr;
    self->blocksVtable = reinterpret_cast<void*>(PrimitiveArray_vtable);
    NiString_ConstructSized(&self->description, 0x100);
    return self;
}

const void* __fastcall FragmentNodeGetRTTI(NiMaterialFragmentNode*, re::Edx) {
    return &NiMaterialFragmentNode_RTTI;
}

NiMaterialFragmentNode* __fastcall FragmentNodeDeletingDestructor(NiMaterialFragmentNode* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiMaterialFragmentNode_vtable);
    NiString_Destruct(&self->description);
    self->blocksVtable = reinterpret_cast<void*>(PrimitiveArray_vtable);
    NiTMalloc_Free(self->blocks);
    MaterialNode_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x44);
    }
    return self;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISTDMAT4_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISTDMAT4_REVERSED("NiStandardMaterial::SplitTextureMaps", 0x007902A0, &ni::SplitTextureMaps);
NISTDMAT4_REVERSED("NiStandardMaterial::DropShadowMapsThenSplitTextureMaps", 0x00790B00,
                   &ni::DropShadowMapsThenSplitTextureMaps);
NISTDMAT4_REVERSED("NiStandardMaterial::DropParallaxMap", 0x00790B40, &ni::DropParallaxMap);
NISTDMAT4_REVERSED("NiStandardMaterial::SplitPerPixelLights", 0x00796D90, &ni::SplitPerPixelLights);
NISTDMAT4_REVERSED("NiStandardMaterial::DropShadowMapsThenSplitPerPixelLights", 0x007972E0,
                   &ni::DropShadowMapsThenSplitPerPixelLights);
NISTDMAT4_REVERSED("NiStandardMaterial::SplitPerVertexLights", 0x00797320, &ni::SplitPerVertexLights);
NISTDMAT4_REVERSED("NiStandardMaterial::DropParallaxMapThenSplitLights", 0x007975C0,
                   &ni::DropParallaxMapThenSplitLights);
NISTDMAT4_REVERSED("NiStandardMaterial::DropShadowMapsThenDropParallaxMapThenSplitLights", 0x007975F0,
                   &ni::DropShadowMapsThenDropParallaxMapThenSplitLights);
NISTDMAT4_REVERSED("NiStandardMaterial::GetUVForMap", 0x00797650, &ni::GetUVForMapImpl);
NISTDMAT4_REVERSED("NiStandardMaterial::AddDefaultFallbacks", 0x00797690, &ni::AddDefaultFallbacks);
NISTDMAT4_REVERSED("NiStandardMaterial::GenerateShaderDescArray", 0x00796760, &ni::GenerateShaderDescArray);
NISTDMAT4_REVERSED("NiStandardMaterial::VerifyShaderPrograms", 0x007AC010, &ni::VerifyShaderPrograms);
NISTDMAT4_REVERSED("NiStandardMaterial::AddResourceToShaderConstantMap", 0x007965F0,
                   &ni::AddResourceToShaderConstantMap);
NISTDMAT4_REVERSED("NiStandardMaterial::AddOutputPredefined", 0x00796430, &ni::AddOutputPredefined);
NISTDMAT4_REVERSED("NiStandardMaterial::NiStandardMaterial(name)", 0x00797F30, &ni::ConstructNamed);
NISTDMAT4_REVERSED("NiStandardMaterial::NiStandardMaterial", 0x00797F90, &ni::Construct);
NISTDMAT4_REVERSED("NiStandardMaterial::GetRTTI", 0x00797F20, &ni::GetRTTI);
NISTDMAT4_REVERSED("NiStandardMaterial::DeletingDestructor", 0x00798030, &ni::DeletingDestructor);
NISTDMAT4_REVERSED("NiMaterialFragmentNode::NiMaterialFragmentNode", 0x00798090, &ni::FragmentNodeConstruct);
NISTDMAT4_REVERSED("NiMaterialFragmentNode::GetRTTI", 0x007980D0, &ni::FragmentNodeGetRTTI);
NISTDMAT4_REVERSED("NiMaterialFragmentNode::DeletingDestructor", 0x007980E0, &ni::FragmentNodeDeletingDestructor);
