// NiShaderConstantMap's predefined-mapping tables (0x20E3470, 138 entries, and
// 0x20E3478, 30): an id and a NiFixedString name each, filled by 0x77F710 (19 KB the
// compiler unrolled). Written from the code by tools/gen_constmaptables.py (it runs the function
// in unicorn and reads the tables back): don't edit, regenerate. Each entry is made as the game
// makes it: the name's handle from the global string table, assigned to a temporary pair and
// from it to the entry, the temporaries let go; the tables are new[]s of pairs constructed by
// 0x77F5D0; 0x77EE50 runs last.

#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using ni::scene::FixedStringRelease;

struct Mapping {
    uint32_t id;
    const char* name;
};

const Mapping kTable1[138] = {
    {0x0, "Invalid"},
    {0x1, "Proj"},
    {0x1, "Projection"},
    {0x2, "InvProj"},
    {0x2, "InvProjection"},
    {0x3, "View"},
    {0x4, "InvView"},
    {0x5, "World"},
    {0x6, "InvWorld"},
    {0x7, "WorldView"},
    {0x8, "InvWorldView"},
    {0x9, "ViewProj"},
    {0x9, "ViewProjection"},
    {0xA, "InvViewProj"},
    {0xA, "InvViewProjection"},
    {0xB, "WorldViewProj"},
    {0xB, "WorldViewProjection"},
    {0xC, "InvWorldViewProj"},
    {0xC, "InvWorldViewProjection"},
    {0xD, "ProjTranspose"},
    {0xD, "ProjectionTranspose"},
    {0xE, "InvProjTranspose"},
    {0xE, "InvProjectionTranspose"},
    {0xF, "ViewTranspose"},
    {0x10, "InvViewTranspose"},
    {0x11, "WorldTranspose"},
    {0x12, "InvWorldTranspose"},
    {0x13, "WorldViewTranspose"},
    {0x14, "InvWorldViewTranspose"},
    {0x15, "ViewProjTranspose"},
    {0x15, "ViewProjectionTranspose"},
    {0x16, "InvViewProjTranspose"},
    {0x16, "InvViewProjectionTranspose"},
    {0x17, "WorldViewProjTranspose"},
    {0x17, "WorldViewProjectionTranspose"},
    {0x18, "InvWorldViewProjTranspose"},
    {0x18, "InvWorldViewProjectionTranspose"},
    {0x19, "SkinWorldView"},
    {0x1A, "InvSkinWorldView"},
    {0x1B, "SkinWorldViewTranspose"},
    {0x1C, "InvSkinWorldViewTranspose"},
    {0x1D, "SkinWorldViewProj"},
    {0x1D, "SkinWorldViewProjection"},
    {0x1E, "InvSkinWorldViewProj"},
    {0x1E, "InvSkinWorldViewProjection"},
    {0x1F, "SkinWorldViewProjTranspose"},
    {0x1F, "SkinWorldViewProjectionTranspose"},
    {0x20, "InvSkinWorldViewProjTranspose"},
    {0x20, "InvSkinWorldViewProjectionTranspose"},
    {0x21, "BoneMatrix3"},
    {0x22, "BoneMatrix4"},
    {0x23, "SkinBoneMatrix3"},
    {0x24, "SkinBoneMatrix4"},
    {0x25, "TexTransformBase"},
    {0x26, "InvTexTransformBase"},
    {0x27, "TexTransformBaseTranspose"},
    {0x28, "InvTexTransformBaseTranspose"},
    {0x29, "TexTransformDark"},
    {0x2A, "InvTexTransformDark"},
    {0x2B, "TexTransformDarkTranspose"},
    {0x2C, "InvTexTransformDarkTranspose"},
    {0x2D, "TexTransformDetail"},
    {0x2E, "InvTexTransformDetail"},
    {0x2F, "TexTransformDetailTranspose"},
    {0x30, "InvTexTransformDetailTranspose"},
    {0x31, "TexTransformGloss"},
    {0x32, "InvTexTransformGloss"},
    {0x33, "TexTransformGlossTranspose"},
    {0x34, "InvTexTransformGlossTranspose"},
    {0x35, "TexTransformGlow"},
    {0x36, "InvTexTransformGlow"},
    {0x37, "TexTransformGlowTranspose"},
    {0x38, "InvTexTransformGlowTranspose"},
    {0x39, "TexTransformBump"},
    {0x3A, "InvTexTransformBump"},
    {0x3B, "TexTransformBumpTranspose"},
    {0x3C, "InvTexTransformBumpTranspose"},
    {0x3D, "TexTransformDecal"},
    {0x3E, "InvTexTransformDecal"},
    {0x3F, "TexTransformDecalTranspose"},
    {0x40, "InvTexTransformDecalTranspose"},
    {0x41, "TexTransformShader"},
    {0x42, "InvTexTransformShader"},
    {0x43, "TexTransformShaderTranspose"},
    {0x44, "InvTexTransformShaderTranspose"},
    {0x45, "LightPosWS"},
    {0x46, "LightDirWS"},
    {0x47, "LightPosOS"},
    {0x48, "LightDirOS"},
    {0x49, "MaterialDiffuse"},
    {0x49, "Diffuse"},
    {0x4A, "MaterialAmbient"},
    {0x4A, "Ambient"},
    {0x4B, "MaterialSpecular"},
    {0x4B, "Specular"},
    {0x4C, "MaterialEmissive"},
    {0x4D, "MaterialPower"},
    {0x4D, "SpecularPower"},
    {0x4E, "EyePos"},
    {0x4F, "EyeDir"},
    {0x50, "TaylorSin"},
    {0x51, "TaylorCos"},
    {0x52, "time"},
    {0x53, "sin_time"},
    {0x54, "cos_time"},
    {0x55, "tan_time"},
    {0x56, "time_sin_cos_tan"},
    {0x57, "AmbientLight"},
    {0x5C, "TexTransformParallax"},
    {0x5D, "InvTexTransformParallax"},
    {0x5E, "TexTransformParallaxTranspose"},
    {0x5F, "InvTexTransformParallaxTranspose"},
    {0x58, "TexTransformNormal"},
    {0x59, "InvTexTransformNormal"},
    {0x5A, "TexTransformNormalTranspose"},
    {0x5B, "InvTexTransformNormalTranspose"},
    {0x60, "SkinWorld"},
    {0x61, "InvSkinWorld"},
    {0x62, "SkinWorldTranspose"},
    {0x63, "InvSkinWorldTranspose"},
    {0x64, "FogDensity"},
    {0x65, "FogNearFar"},
    {0x66, "FogColor"},
    {0x67, "BumpMatrix"},
    {0x68, "BumpLumaAndScale"},
    {0x69, "ParallaxOffset"},
    {0x6A, "TexSizeBase"},
    {0x6B, "TexSizeDark"},
    {0x6C, "TexSizeDetail"},
    {0x6D, "TexSizeGloss"},
    {0x6E, "TexSizeGlow"},
    {0x6F, "TexSizeBump"},
    {0x70, "TexSizeNormal"},
    {0x71, "TexSizeParallax"},
    {0x72, "TexSizeDecal"},
    {0x73, "TexSizeShader"},
    {0x74, "AlphaTestFunction"},
    {0x75, "AlphaTestRef"},
};

const Mapping kTable2[30] = {
    {0x0, "Invalid"},
    {0x1, "Dimmer"},
    {0x2, "UndimmedAmbient"},
    {0x3, "UndimmedDiffuse"},
    {0x4, "UndimmedSpecular"},
    {0x5, "Ambient"},
    {0x6, "Diffuse"},
    {0x7, "Specular"},
    {0x8, "WorldPosition"},
    {0x9, "ModelPosition"},
    {0xA, "WorldDirection"},
    {0xB, "ModelDirection"},
    {0xC, "WorldTransform"},
    {0xD, "ModelTransform"},
    {0xE, "SpotAttenuation"},
    {0xF, "Attenuation"},
    {0x10, "WorldProjectionMatrix"},
    {0x11, "ModelProjectionMatrix"},
    {0x12, "WorldProjectionTranslation"},
    {0x13, "ModelProjectionTranslation"},
    {0x14, "WorldClippingPlane"},
    {0x15, "ModelClippingPlane"},
    {0x16, "TexCoordGen"},
    {0x17, "WorldProjectionTransform"},
    {0x18, "ModelProjectionTransform"},
    {0x19, "WorldToShadowMapMatrix"},
    {0x1A, "ShadowMap"},
    {0x1B, "TexSizeShadow"},
    {0x1C, "ShadowBias"},
    {0x1D, "ShadowVSMPowerEpsilon"},
};

// an entry of the tables
struct Pair {
    uint32_t id;          // +0x0
    re::Ptr32<const char> name;   // +0x4
};
static_assert(sizeof(Pair) == 8);

GAME_VAR(Pair*, gTable1, 0x020E3470);
GAME_VAR(uint32_t, gTable1Count, 0x020E3474);
GAME_VAR(Pair*, gTable2, 0x020E3478);
GAME_VAR(uint32_t, gTable2Count, 0x020E347C);
GAME_VAR(NiFixedString, kEmptyFixedString, 0x00951028);

GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* construct);
GAME_FN(const char*, __cdecl, StringTable_AddString, 0x00754760, const char* text);   // (a native return: a Ptr32 by value is returned through memory on x64)
GAME_FN(re::Ptr32<const char>*, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<const char>* self, const re::Ptr32<const char>* other);
GAME_FN(void, __cdecl, FillShaderConstantTypeTable, 0x0077EE50);
// NiTPair<unsigned int, NiFixedString>'s vector deleting destructor (3: a new[]'d array)
GAME_FN(Pair*, __thiscall, Pair_VectorDeletingDestructor, 0x00750770, Pair* self, uint32_t flags);
constexpr uintptr_t kPairConstruct = 0x0077F5D0;

// a new[] of `count` pairs with the count before them (the size saturating as the compiler's)
Pair* NewPairs(uint32_t count) {
    uint64_t bytes = uint64_t(count) * sizeof(Pair);
    uint32_t size = bytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(bytes);
    size = size > 0xFFFFFFFBu ? 0xFFFFFFFFu : size + 4;
    auto* block = static_cast<uint32_t*>(NiNewArray(size));
    if (block == nullptr) {
        return nullptr;
    }
    *block = count;
    VectorConstruct(block + 1, sizeof(Pair), count, reinterpret_cast<void*>(kPairConstruct));
    return reinterpret_cast<Pair*>(block + 1);
}

void Fill(Pair* table, const Mapping* mappings, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        NiFixedString handle = StringTable_AddString(mappings[i].name);
        Pair temporary;
        temporary.name = kEmptyFixedString;
        temporary.id = mappings[i].id;
        FixedString_Assign(&temporary.name, &handle);
        table[i].id = temporary.id;
        FixedString_Assign(&table[i].name, &temporary.name);
        FixedStringRelease(&temporary.name);
        FixedStringRelease(&handle);
    }
}

void __cdecl FillPredefinedMappingTables() {
    gTable1Count = 138;
    gTable1 = NewPairs(138);
    Fill(gTable1, kTable1, 138);
    gTable2Count = 30;
    gTable2 = NewPairs(30);
    Fill(gTable2, kTable2, 30);
    FillShaderConstantTypeTable();
}

// 0x784140: the tables deleted (delete[], the object mappings' first; NiMainSDM::Shutdown's)
void __cdecl DeletePredefinedMappingTables() {
    if (gTable2 != nullptr) {
        Pair_VectorDeletingDestructor(gTable2, 3);
    }
    gTable2 = nullptr;
    if (gTable1 != nullptr) {
        Pair_VectorDeletingDestructor(gTable1, 3);
    }
    gTable1 = nullptr;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NICONSTMAPTABLES_REVERSED(name, address, fn)                                           \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NICONSTMAPTABLES_REVERSED("NiShaderConstantMap::FillPredefinedMappingTables", 0x0077F710,
                          &ni::FillPredefinedMappingTables);
// (never hooked in the 32-bit game: a body at its address for the 64-bit build)
RE_UNHOOKED_FN("NiShaderConstantMap::DeletePredefinedMappingTables", 0x00784140, &ni::DeletePredefinedMappingTables);
