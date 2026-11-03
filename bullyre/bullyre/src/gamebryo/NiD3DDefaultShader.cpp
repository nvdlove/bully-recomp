// NiD3DDefaultShader (vtable 0x99D910, 0xE4 bytes; NiD3DShader's RTTI): the fixed-function shader
// the DX9 renderer draws legacy geometry with ("Default Shader From Gamebryo 2.1", the renderer's
// +0x8A0). Each UpdatePipeline (slot 7) throws its passes away and builds them again from the
// texturing property's maps, the dynamic effects, the alpha and fog properties: as many maps and
// effects in a pass as the device's blend stages and simultaneous textures allow, the rest in
// more passes blended onto the first.
//
// Layout past NiD3DShader's 0x58 (the constructor 0x89B820): six NiTPrimitiveArray<NiTextureEffect*>
// (vtable 0x961988, 0x10 each) the dynamic effects sorted into by PrepareEffects: +0x58 the
// projected lights with a clipping plane, +0x68 those without, +0x78 / +0x88 the projected shadows
// likewise, +0x98 the environment maps, +0xA8 the fog maps; +0xB8 / +0xB9 the bytes the current
// pass was opened with (+0xB8: the base map goes again on its alpha; +0xB9: a pass with a vertex
// colour), +0xBC the highest texture coordinate set, +0xC0 / +0xC4 the blend stages and textures
// left in the current pass, +0xCC the passes' fog flags (NiMalloc'd) and +0xD0 their capacity,
// +0xD4 whether the device blends by the texture's alpha (D3DTEXOPCAPS_BLENDTEXTUREALPHA, set by
// Initialize 0x8A6460 from the caps' TextureOpCaps bit 12: decals and fog maps then share the
// first pass), +0xD8 the bump map stage's COLOROP (1 DISABLE from the constructor; Initialize
// makes it 0x16 BUMPENVMAP or 0x17 BUMPENVMAPLUMINANCE as the caps allow: the bump map dropped
// while it's DISABLE), +0xDC 1 from the constructor (decals in the first pass allowed), +0xE0 the
// dynamic effect state last sorted.
//
// The pass being built is NiD3DShader's +0x40 (an NiD3DPassPtr counting at the pass's +0x60), the
// pass count its +0x3C. Names past Gamebryo's own (UpdatePipeline, the constructor) are guesses.
// The rest of the class (its stage builders 0x89A680..0x89ADB0, the destructor) is chunk 40's.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::c41 {
using namespace ni::dx9;
namespace {

constexpr uintptr_t kDefaultShaderVtable = 0x0099D910;
constexpr uintptr_t kPrimitiveArrayVtable = 0x00961988;
constexpr uint32_t kDefaultShaderSize = 0xE4;

GAME_VAR(uint32_t, ms_uiMaxTextureBlendStages, 0x00BCB7F8);
GAME_VAR(uint32_t, ms_uiMaxSimultaneousTextures, 0x00BCB7FC);

// D3DRENDERSTATETYPE
constexpr uint32_t D3DRS_SRCBLEND = 0x13;
constexpr uint32_t D3DRS_DESTBLEND = 0x14;
constexpr uint32_t D3DRS_ALPHABLENDENABLE = 0x1B;
constexpr uint32_t D3DRS_FOGENABLE = 0x1C;
constexpr uint32_t D3DRS_FOGCOLOR = 0x22;
constexpr uint32_t D3DRS_TEXTUREFACTOR = 0x3C;
// D3DTEXTURESTAGESTATETYPE
constexpr uint32_t D3DTSS_COLOROP = 1;
constexpr uint32_t D3DTSS_COLORARG1 = 2;
constexpr uint32_t D3DTSS_COLORARG2 = 3;
constexpr uint32_t D3DTSS_ALPHAOP = 4;
constexpr uint32_t D3DTSS_ALPHAARG1 = 5;
constexpr uint32_t D3DTSS_ALPHAARG2 = 6;
constexpr uint32_t D3DTSS_TEXCOORDINDEX = 0xB;
constexpr uint32_t D3DTSS_TEXTURETRANSFORMFLAGS = 0x18;

GAME_FN(void*, __thiscall, D3DShader_Construct, 0x007470B0, void* self);
GAME_FN(re::Ptr32<char>*, __thiscall, FixedString_AssignCString, 0x00406730, re::Ptr32<char>* self, const char* s);
GAME_FN(uint8_t, __thiscall, BufferData_IsVBValid, 0x00896EB0, void* self);
GAME_FN(uint32_t, __thiscall, PrimitiveArray_Add, 0x004099B0, void* array, const re::Ptr32<void>* value);
GAME_FN(uint8_t, __thiscall, Pass_CheckFreeStages, 0x00883C90, void* pass, uint32_t stages, uint32_t textures);
GAME_FN(re::Ptr32<uint8_t>*, __cdecl, Pass_CreateNewPass, 0x00884A90, re::Ptr32<uint8_t>* pass);
GAME_FN(void, __thiscall, Pass_ReturnPassToPool, 0x008846D0, void* pass);
GAME_FN(uint32_t, __thiscall, Pass_SetStage, 0x00884830, void* pass, uint32_t index, uint8_t* stage);
GAME_FN(void, __thiscall, Pass_SetRenderState, 0x00893310, void* pass, uint32_t state, uint32_t value, int save);
GAME_FN(void, __thiscall, PassArray_SetSize, 0x00893D70, void* array, uint32_t size);
GAME_FN(void, __thiscall, PassArray_SetAt, 0x00746990, void* array, uint32_t index, const re::Ptr32<uint8_t>* pass);
GAME_FN(re::Ptr32<uint8_t>*, __cdecl, Stage_CreateNewStage, 0x0089F810, re::Ptr32<uint8_t>* stage);
GAME_FN(void, __thiscall, Stage_ReturnStageToPool, 0x0089F670, void* stage);
GAME_FN(void, __thiscall, Stage_SetFilterMode, 0x0089E640, void* stage, uint32_t mode);
GAME_FN(void, __thiscall, Stage_SetClampMode, 0x0089E5E0, void* stage, uint32_t mode);
GAME_FN(void, __thiscall, StageGroup_SetStageState, 0x0089F9E0, void* group, uint32_t state, uint32_t value, int save);
GAME_FN(void*, __cdecl, RenderStateGroup_GetFree, 0x0089E3A0);
GAME_FN(void, __thiscall, RenderStateGroup_SetRenderState, 0x0089E1F0, void* group, uint32_t state, uint32_t value,
        int save);
GAME_FN(uint8_t, __thiscall, RenderStateGroup_GetRenderState, 0x0089DE70, void* group, uint32_t state,
        uint32_t* value, uint8_t* saved);
// the class's own, chunk 40's: whether the current pass blends (src, dest) already; a map's stage
// (its texture coordinate set); the bump map's; a texture effect's; the effect arrays cleared; the
// destructor
GAME_FN(uint8_t, __thiscall, DefaultShader_CanAddToPass, 0x0089A680, void* self, uint32_t src, uint32_t dest);
GAME_FN(uint8_t*, __thiscall, DefaultShader_AddMapStage, 0x0089A9B0, void* self, void* map, uint32_t texCoord);
GAME_FN(uint8_t*, __thiscall, DefaultShader_AddBumpMapStage, 0x0089ABB0, void* self, void* map, uint32_t texCoord);
GAME_FN(uint8_t*, __thiscall, DefaultShader_AddEffectStage, 0x0089ADB0, void* self, void* effect);
GAME_FN(void, __thiscall, DefaultShader_ClearEffects, 0x0089A700, void* self);
GAME_FN(void, __thiscall, DefaultShader_Destruct, 0x0089A8A0, void* self);

// this file's (called where modes apply)
GAME_FN(uint8_t*, __thiscall, DefaultShader_AddClippingStage, 0x0089AF00, void* self, uint8_t* effect);
GAME_FN(void, __thiscall, DefaultShader_ClosePass, 0x0089B160, void* self, void* baseMap);
GAME_FN(uint8_t, __thiscall, DefaultShader_AddEffect, 0x0089B220, void* self, uint8_t* effect, int clipped);
GAME_FN(void, __thiscall, DefaultShader_OpenPass, 0x0089B310, void* self, uint32_t src, uint32_t dest,
        int baseAlpha, int vertexColor);
GAME_FN(void, __thiscall, DefaultShader_PrepareEffects, 0x0089B460, void* self, uint8_t* effects);
GAME_FN(void, __thiscall, DefaultShader_SetupFog, 0x0089B4F0, void* self, uint8_t* fog, uint8_t* alpha,
        void* baseMap, uint32_t applyMode);
GAME_FN(uint32_t, __thiscall, DefaultShader_ConstructPipeline, 0x0089B940, void* self, uint8_t* geometry, void* skin,
        uint8_t* bufferData, uint8_t* properties, uint8_t* effects, const void* world, const void* bound);

inline re::Ptr32<uint8_t>& CurrentPass(uint8_t* self) {
    return At<uint8_t*>(self, 0x40);
}

// the pass's render state group, made when it has none
inline void* PassGroup(uint8_t* pass) {
    if (At<void*>(pass, 0x30) == nullptr) {
        At<void*>(pass, 0x30) = RenderStateGroup_GetFree();
    }
    return At<void*>(pass, 0x30);
}

inline void StageState(uint8_t* stage, uint32_t state, uint32_t value) {
    StageGroup_SetStageState(At<void*>(stage, 0xC), state, value, 0);
}

// the current pass let go and counted, its stage and texture budget emptied (the inlined end of
// a pass)
inline void EndPass(uint8_t* self) {
    uint8_t* pass = CurrentPass(self);
    if (pass != nullptr) {
        if (--At<uint32_t>(pass, 0x60) == 0) {
            Pass_ReturnPassToPool(pass);
        }
        CurrentPass(self) = nullptr;
    }
    At<uint32_t>(self, 0x3C)++;
    At<uint32_t>(self, 0xC0) = 0;
    At<uint32_t>(self, 0xC4) = 0;
}

inline bool PassHasRoom(const uint8_t* self) {
    return At<uint32_t>(self, 0xC0) != 0 && At<uint32_t>(self, 0xC4) != 0;
}

// a map's texture coordinate set, no higher than the geometry's highest (that compared in 32 bits,
// replaced by its low 16)
inline uint32_t MapTexCoord(const uint8_t* self, const uint8_t* map) {
    uint16_t set = At<uint8_t>(map, 4);
    if (set > At<uint32_t>(self, 0xBC)) {
        set = At<uint16_t>(self, 0xBC);
    }
    return set;
}

// the source blend the alpha property's function maps to (the render state's table)
inline uint32_t AlphaSrcBlend(const uint8_t* self, const uint8_t* alpha) {
    return At<uint32_t>(At<uint8_t*>(self, 0x1C), 0x20 + int((At<uint8_t>(alpha, 0x18) >> 1) & 0xF) * 4);
}

} // namespace

// 0x89AF00 (thiscall): a projected light's or shadow's clipping plane as a stage of the current
// pass (when one stage and one texture fit): the renderer's clipper image looked up by the plane's
// distance (camera space position as texture coordinates, the plane (negated for a shadow) through
// the inverse view as the texture transform's first column, 0.5 added and the plane's constant
// taken off its last), point filtered and clamped. The stage, null without room. (Name a guess.)
uint8_t* __fastcall DefaultShader_AddClippingStageImpl(uint8_t* self, re::Edx, uint8_t* effect) {
    if (CurrentPass(self) == nullptr || Pass_CheckFreeStages(CurrentPass(self), 1, 1) == 0) {
        return nullptr;
    }
    re::Ptr32<uint8_t> held = nullptr;
    Stage_CreateNewStage(&held);
    uint8_t* stage = held;
    At<uint32_t>(stage, 0) = At<uint32_t>(CurrentPass(self), 0x14);
    At<void*>(stage, 4) = At<void*>(At<uint8_t*>(self, 0x18), kClipperImage);
    StageGroup_SetStageState(At<void*>(stage, 0xC), D3DTSS_TEXCOORDINDEX, 0x20000, 0);   // CAMERASPACEPOSITION
    At<uint8_t>(stage, 0x5A) = 1;
    StageGroup_SetStageState(At<void*>(stage, 0xC), D3DTSS_TEXTURETRANSFORMFLAGS, 2, 0);   // COUNT2
    StageGroup_SetStageState(At<void*>(stage, 0xC), D3DTSS_COLOROP, 4, 0);                 // MODULATE
    StageGroup_SetStageState(At<void*>(stage, 0xC), D3DTSS_COLORARG1, 2, 0);               // TEXTURE
    StageGroup_SetStageState(At<void*>(stage, 0xC), D3DTSS_COLORARG2, 1, 0);               // CURRENT
    StageGroup_SetStageState(At<void*>(stage, 0xC), D3DTSS_ALPHAOP, 3, 0);                 // SELECTARG2
    StageGroup_SetStageState(At<void*>(stage, 0xC), D3DTSS_ALPHAARG1, 2, 0);
    if (At<uint8_t>(self, 0xB9) == 0) {
        StageGroup_SetStageState(At<void*>(stage, 0xC), D3DTSS_ALPHAARG2, 1, 0);
    } else {
        StageGroup_SetStageState(At<void*>(stage, 0xC), D3DTSS_ALPHAARG2, 0, 0);   // DIFFUSE
        At<uint8_t>(self, 0xB9) = 0;
    }
    Stage_SetFilterMode(stage, 0);
    Stage_SetClampMode(stage, 0);
    float x;
    float y;
    float z;
    float w;
    if (At<uint32_t>(effect, 0x164) == 1) {   // a projected shadow
        x = -At<float>(effect, 0x180);
        y = -At<float>(effect, 0x184);
        z = -At<float>(effect, 0x188);
        w = -At<float>(effect, 0x18C);
    } else {
        x = At<float>(effect, 0x180);
        y = At<float>(effect, 0x184);
        z = At<float>(effect, 0x188);
        w = At<float>(effect, 0x18C);
    }
    const float* m = &At<float>(At<uint8_t*>(self, 0x18), kInvView);
    At<float>(stage, 0x18) = (m[0] * x + m[1] * y) + m[2] * z;
    At<float>(stage, 0x28) = (m[5] * y + m[4] * x) + m[6] * z;
    At<float>(stage, 0x38) = (m[9] * y + m[8] * x) + m[10] * z;
    At<float>(stage, 0x48) = (((x * m[12] + 0.5f) + y * m[13]) + z * m[14]) - w;
    At<uint32_t>(self, 0xC0)--;
    At<uint32_t>(self, 0xC4)--;
    Pass_SetStage(CurrentPass(self), At<uint32_t>(CurrentPass(self), 0x14), stage);
    if (held != nullptr && --At<uint32_t>(held, 0x5C) == 0) {
        Stage_ReturnStageToPool(held);
    }
    return stage;
}

// 0x89B120: BeginFrame (slot-less; called by NiDX9Renderer::Do_BeginFrame): a lone jmp to the
// effect arrays' clearing (0x89A700)
RE_UNHOOKED("NiD3DDefaultShader::BeginFrame", 0x0089B120);

// 0x89B130 (thiscall, slot 0): the deleting destructor
uint8_t* __fastcall DefaultShader_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    DefaultShader_Destruct(self);
    if (flags & 1) {
        NiDelete(self, kDefaultShaderSize);
    }
    return self;
}

// 0x89B160 (thiscall): the current pass closed: when it was opened to repeat the base map's alpha
// (+0xB8), the base map's stage added first (its alpha selected, times the diffuse with a vertex
// colour); the pass counted. (Name a guess.)
void __fastcall DefaultShader_ClosePassImpl(uint8_t* self, re::Edx, uint8_t* baseMap) {
    if (CurrentPass(self) == nullptr) {
        return;
    }
    if (At<uint8_t>(self, 0xB8) != 0 && baseMap != nullptr) {
        uint8_t* stage = DefaultShader_AddMapStage(self, baseMap, MapTexCoord(self, baseMap));
        if (stage != nullptr) {
            StageState(stage, D3DTSS_COLOROP, 3);   // SELECTARG2
            if (At<uint8_t>(self, 0xB8) != 0) {
                StageState(stage, D3DTSS_ALPHAOP, 4);   // MODULATE
                StageState(stage, D3DTSS_ALPHAARG2, 0);   // DIFFUSE
            } else {
                StageState(stage, D3DTSS_ALPHAOP, 2);
            }
        }
    }
    EndPass(self);
}

// 0x89B220 (thiscall): a texture effect sorted into its array when it's on and has a texture whose
// format (the texture manager's slot 5) isn't 0x16: projected lights and shadows by their clipping
// plane, environment and fog maps. True when sorted (or an unknown type); false when skipped.
// (Name a guess.)
uint8_t __fastcall DefaultShader_AddEffectImpl(uint8_t* self, re::Edx, uint8_t* effect, int clipped) {
    if (At<uint8_t>(effect, 0xC4) == 0) {
        return 0;
    }
    void* texture = At<void*>(effect, 0x158);
    if (texture == nullptr) {
        return 0;
    }
    void* textureManager = At<void*>(At<uint8_t*>(self, 0x18), kTextureManager);
    if (Virtual<uint32_t>(textureManager, 5, texture) == 0x16) {
        return 0;
    }
    re::Ptr32<void> value = effect;
    switch (At<uint32_t>(effect, 0x164)) {
    case 0:   // projected light
        PrimitiveArray_Add(self + (uint8_t(clipped) != 0 ? 0x58 : 0x68), &value);
        break;
    case 1:   // projected shadow
        PrimitiveArray_Add(self + (uint8_t(clipped) != 0 ? 0x78 : 0x88), &value);
        break;
    case 2:   // environment map
        PrimitiveArray_Add(self + 0x98, &value);
        break;
    case 3:   // fog map
        PrimitiveArray_Add(self + 0xA8, &value);
        break;
    default:
        break;
    }
    return 1;
}

// 0x89B310 (thiscall): a new pass opened when none is (from the pass pool, held, put at the pass
// count's index of the array): past the first, blended with (src, dest) (alpha blending off,
// saved, for ONE/ZERO); the bytes it's opened with and a full stage and texture budget (one each
// less when it repeats the base map's alpha). (Name a guess.)
void __fastcall DefaultShader_OpenPassImpl(uint8_t* self, re::Edx, uint32_t src, uint32_t dest, int baseAlpha,
                                           int vertexColor) {
    if (CurrentPass(self) != nullptr) {
        return;
    }
    re::Ptr32<uint8_t> created = nullptr;
    re::Ptr32<uint8_t>* made = Pass_CreateNewPass(&created);
    uint8_t* old = CurrentPass(self);
    if (old != *made) {
        if (old != nullptr && --At<uint32_t>(old, 0x60) == 0) {
            Pass_ReturnPassToPool(old);
        }
        CurrentPass(self) = *made;
        if (*made != nullptr) {
            At<uint32_t>(*made, 0x60)++;
        }
    }
    if (created != nullptr && --At<uint32_t>(created, 0x60) == 0) {
        Pass_ReturnPassToPool(created);
    }
    uint32_t index = At<uint32_t>(self, 0x3C);
    uint8_t* passes = self + 0x44;
    if (index >= At<uint16_t>(self, 0x4C)) {
        PassArray_SetSize(passes, At<uint16_t>(passes, 0xE) + index);
    }
    PassArray_SetAt(passes, index, &CurrentPass(self));
    if (At<uint32_t>(self, 0x3C) != 0) {
        if (src == 2 && dest == 1) {
            Pass_SetRenderState(CurrentPass(self), D3DRS_ALPHABLENDENABLE, 0, 1);
        } else {
            RenderStateGroup_SetRenderState(PassGroup(CurrentPass(self)), D3DRS_SRCBLEND, src, 0);
            RenderStateGroup_SetRenderState(PassGroup(CurrentPass(self)), D3DRS_DESTBLEND, dest, 0);
            RenderStateGroup_SetRenderState(PassGroup(CurrentPass(self)), D3DRS_ALPHABLENDENABLE, 1, 1);
        }
    }
    At<uint8_t>(self, 0xB8) = uint8_t(baseAlpha);
    At<uint8_t>(self, 0xB9) = uint8_t(vertexColor);
    At<uint32_t>(self, 0xC0) = ms_uiMaxTextureBlendStages;
    At<uint32_t>(self, 0xC4) = ms_uiMaxSimultaneousTextures;
    if (uint8_t(baseAlpha) != 0) {
        At<uint32_t>(self, 0xC0) += 0xFFFFFFFF;
        At<uint32_t>(self, 0xC4) += 0xFFFFFFFF;
    }
}

// 0x89B460 (thiscall): a dynamic effect state's texture effects sorted (when it isn't the one last
// sorted): the arrays emptied, its projected lights and shadows (each with its clipping flag,
// +0x16C) and its environment and fog maps added. (Name a guess.)
void __fastcall DefaultShader_PrepareEffectsImpl(uint8_t* self, re::Edx, uint8_t* effects) {
    if (effects == At<uint8_t*>(self, 0xE0)) {
        return;
    }
    DefaultShader_ClearEffects(self);
    At<uint8_t*>(self, 0xE0) = effects;
    if (effects == nullptr) {
        return;
    }
    for (uint8_t* node = At<uint8_t*>(effects, 0x10); node != nullptr;) {
        uint8_t* effect = At<uint8_t*>(node, 4);
        node = At<uint8_t*>(node, 0);
        DefaultShader_AddEffect(self, effect, At<uint8_t>(effect, 0x16C));
    }
    for (uint8_t* node = At<uint8_t*>(effects, 0x14); node != nullptr;) {
        uint8_t* effect = At<uint8_t*>(node, 4);
        node = At<uint8_t*>(node, 0);
        DefaultShader_AddEffect(self, effect, At<uint8_t>(effect, 0x16C));
    }
    if (At<uint8_t*>(effects, 0x18) != nullptr) {
        DefaultShader_AddEffect(self, At<uint8_t*>(effects, 0x18), 0);
    }
    if (At<uint8_t*>(effects, 0x1C) != nullptr) {
        DefaultShader_AddEffect(self, At<uint8_t*>(effects, 0x1C), 0);
    }
}

// 0x89B4F0 (thiscall): fog over several passes (when fog is on): each pass past the first fogged
// with black or not at all by how it blends (those that add take no fog; one that replaces what's
// below fogs everything up to it with black), then one more pass laying the fog colour (the
// texture factor's black, the fog's colour) blended by the alpha property's source function over
// ONE, with the base map's alpha when it's alpha blended or tested. (Name a guess.)
void __fastcall DefaultShader_SetupFogImpl(uint8_t* self, re::Edx, uint8_t* fog, uint8_t* alpha, uint8_t* baseMap,
                                           uint32_t applyMode) {
    uint32_t passCount = At<uint32_t>(self, 0x3C);
    if (passCount < 2 || (At<uint8_t>(fog, 0x18) & 1) == 0) {
        return;
    }
    bool baseAlpha = false;
    bool vertexAlpha = false;
    if ((At<uint8_t>(alpha, 0x18) & 1) != 0 || ((At<uint16_t>(alpha, 0x18) >> 9) & 1) != 0) {
        baseAlpha = baseMap != nullptr && applyMode != 1;
        vertexAlpha = applyMode != 0;
    }
    if (At<uint32_t>(self, 0xD0) < passCount) {
        NiFree(At<void*>(self, 0xCC));
        At<void*>(self, 0xCC) = NiMalloc(At<uint32_t>(self, 0x3C) * 4);
        At<uint32_t>(self, 0xD0) = At<uint32_t>(self, 0x3C);
    }
    uint32_t* flags = At<uint32_t*>(self, 0xCC);
    flags[0] = 1;
    // (the blends read stay as they were when a pass's group lacks them: the game reads its
    // uninitialised stack the first time)
    uint32_t srcBlend = 0;
    uint32_t destBlend = 0;
    uint8_t saved = 0;
    for (uint32_t i = 1; i < At<uint32_t>(self, 0x3C); i++) {
        uint8_t* pass = At<re::Ptr32<uint8_t>*>(self, 0x48)[i];
        if (At<void*>(pass, 0x30) != nullptr) {
            RenderStateGroup_GetRenderState(At<void*>(pass, 0x30), D3DRS_SRCBLEND, &srcBlend, &saved);
        }
        if (At<void*>(pass, 0x30) != nullptr) {
            RenderStateGroup_GetRenderState(At<void*>(pass, 0x30), D3DRS_DESTBLEND, &destBlend, &saved);
        }
        flags = At<uint32_t*>(self, 0xCC);
        if (srcBlend == 3 || srcBlend == 4 || destBlend == 4 || destBlend == 9) {
            flags[i] = 1;
        } else if (srcBlend == 1 || srcBlend == 9) {
            flags[i] = 0;
        } else if (destBlend == 1 || destBlend == 3) {
            for (uint32_t j = 0; j < i; j++) {
                At<uint32_t*>(self, 0xCC)[j] = 0;
            }
            At<uint32_t*>(self, 0xCC)[i] = 1;
        } else {
            flags[i] = 1;
        }
    }
    uint32_t fogColor = At<uint32_t>(At<uint8_t*>(self, 0x1C), 0x230);
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x3C); i++) {
        uint8_t* pass = At<re::Ptr32<uint8_t>*>(self, 0x48)[i];
        if (At<uint32_t*>(self, 0xCC)[i] == 0) {
            RenderStateGroup_SetRenderState(PassGroup(pass), D3DRS_FOGENABLE, 0, 0);
        } else {
            RenderStateGroup_SetRenderState(PassGroup(pass), D3DRS_FOGENABLE, 1, 0);
            RenderStateGroup_SetRenderState(PassGroup(pass), D3DRS_FOGCOLOR, 0xFF000000, 0);
        }
    }
    DefaultShader_OpenPass(self, AlphaSrcBlend(self, alpha), 2, 0, 0);
    uint32_t texCoord = 0;
    if (!baseAlpha) {
        baseMap = nullptr;
    } else if (baseMap != nullptr) {
        texCoord = At<uint8_t>(baseMap, 4);
        if (texCoord > At<uint32_t>(self, 0xBC)) {
            texCoord = At<uint32_t>(self, 0xBC);
        }
    }
    uint8_t* stage = DefaultShader_AddMapStage(self, baseMap, texCoord);
    RenderStateGroup_SetRenderState(PassGroup(CurrentPass(self)), D3DRS_TEXTUREFACTOR, 0xFF000000, 0);
    StageState(stage, D3DTSS_COLOROP, 2);     // SELECTARG1
    StageState(stage, D3DTSS_COLORARG1, 3);   // TFACTOR
    if (vertexAlpha) {
        if (baseAlpha) {
            StageState(stage, D3DTSS_ALPHAOP, 4);   // MODULATE
        } else {
            StageState(stage, D3DTSS_ALPHAOP, 3);   // SELECTARG2
        }
        StageState(stage, D3DTSS_ALPHAARG2, 0);
    } else {
        StageState(stage, D3DTSS_ALPHAOP, 2);
    }
    RenderStateGroup_SetRenderState(PassGroup(CurrentPass(self)), D3DRS_FOGENABLE, 1, 0);
    RenderStateGroup_SetRenderState(PassGroup(CurrentPass(self)), D3DRS_FOGCOLOR, fogColor, 0);
    if (CurrentPass(self) != nullptr) {
        EndPass(self);
    }
}

// 0x89B820 (thiscall): NiD3DDefaultShader::NiD3DDefaultShader
uint8_t* __fastcall DefaultShader_Construct(uint8_t* self, re::Edx) {
    D3DShader_Construct(self);
    At<uint32_t>(self, 0) = kDefaultShaderVtable;
    for (int offset = 0x58; offset <= 0xA8; offset += 0x10) {
        At<uint32_t>(self, offset) = kPrimitiveArrayVtable;
        At<void*>(self, offset + 4) = nullptr;
        At<uint16_t>(self, offset + 8) = 0;
        At<uint16_t>(self, offset + 0xA) = 0;
        At<uint16_t>(self, offset + 0xC) = 0;
        At<uint16_t>(self, offset + 0xE) = 1;
    }
    At<void*>(self, 0xCC) = nullptr;
    At<uint32_t>(self, 0xD0) = 0;
    At<uint8_t>(self, 0xD4) = 0;
    At<uint32_t>(self, 0xD8) = 1;
    At<uint8_t>(self, 0xDC) = 1;
    At<void*>(self, 0xE0) = nullptr;
    FixedString_AssignCString(&At<char*>(self, 8), reinterpret_cast<const char*>(0x0099D9AC));   // its name
    At<uint8_t>(self, 0x20) = 1;
    return self;
}

// 0x89B940 (thiscall): the passes built for a geometry (-1 without vertex data to draw). The
// effects sorted; the texturing property's maps (base, dark, detail, gloss, glow, bump, decals from
// 8) and the counts of the sorted effects. Then, in order: the base map, dark map, detail map and
// projected lights in as few passes as fit (modulated, the first pass's vertex colour by the apply
// mode, the base map's alpha repeated in later passes when the alpha property blends or tests on
// it); the decals (on the first pass when allowed and nothing else is there, else a pass each);
// the projected shadows; the environment map with the bump and gloss maps; the glow map; the fog
// maps; the last pass closed and the fog set up. (Name a guess: Gamebryo's ConstructPipeline.)
uint32_t __fastcall DefaultShader_ConstructPipelineImpl(uint8_t* self, re::Edx, uint8_t* geometry, void*,
                                                        uint8_t* bufferData, uint8_t* properties, uint8_t* effects,
                                                        const void*, const void*) {
    if (geometry == nullptr) {
        if (bufferData == nullptr || BufferData_IsVBValid(bufferData) == 0 || properties == nullptr) {
            return 0xFFFFFFFF;
        }
    } else if (bufferData == nullptr) {
        return 0xFFFFFFFF;
    }
    DefaultShader_PrepareEffects(self, effects);
    uint8_t* texturing = At<uint8_t*>(properties, 0x28);
    uint8_t* baseMap = nullptr;
    uint8_t* bumpMap = nullptr;
    uint8_t* darkMap = nullptr;
    uint8_t* detailMap = nullptr;
    uint8_t* glowMap = nullptr;
    uint8_t* glossMap = nullptr;
    if (bufferData == nullptr) {
        bufferData = At<uint8_t*>(At<uint8_t*>(geometry, 0xCC), 0x3C);
    }
    uint32_t sets;
    if (geometry == nullptr || (At<uint16_t>(At<uint8_t*>(geometry, 0xCC), 0x34) & 0xF000) == 0x4000) {
        sets = At<uint8_t>(bufferData, 7);
    } else {
        sets = At<uint8_t>(At<uint8_t*>(geometry, 0xCC), 0x32) & 0x3F;
    }
    uint16_t setCount = uint16_t(sets);
    if (sets > ms_uiMaxTextureBlendStages) {
        setCount = uint16_t(ms_uiMaxTextureBlendStages);
    }
    At<uint32_t>(self, 0xBC) = 0;
    uint32_t hasBase = 0;
    if (setCount != 0) {
        re::Ptr32<uint8_t>* maps = At<re::Ptr32<uint8_t>*>(texturing, 0x20);
        baseMap = maps[0];
        bumpMap = maps[5];
        darkMap = maps[1];
        detailMap = maps[2];
        glowMap = maps[4];
        glossMap = maps[3];
        At<uint32_t>(self, 0xBC) = uint32_t(setCount) - 1;
        if (baseMap != nullptr && At<void*>(baseMap, 8) != nullptr) {
            hasBase = 1;
        }
    }
    uint32_t hasDetail = (detailMap != nullptr && At<void*>(detailMap, 8) != nullptr) ? 1 : 0;
    uint32_t hasDark = (darkMap != nullptr && At<void*>(darkMap, 8) != nullptr) ? 1 : 0;
    uint32_t hasBump = (bumpMap != nullptr && At<void*>(bumpMap, 8) != nullptr) ? 1 : 0;
    uint32_t decals = (At<uint16_t>(texturing, 0x18) >> 4) & 0xFF;
    bool hasGlow = glowMap != nullptr && At<void*>(glowMap, 8) != nullptr;
    uint32_t hasGloss = (glossMap != nullptr && At<void*>(glossMap, 8) != nullptr) ? 1 : 0;
    // the decal slots without a texture don't count (from map 8 to the array's size; the game
    // counts size - 8 down to zero, wrapping when there are fewer)
    {
        re::Ptr32<uint8_t>* decalMaps = At<re::Ptr32<uint8_t>*>(texturing, 0x20) + 8;
        for (uint32_t n = uint32_t(At<uint16_t>(texturing, 0x26)) - 8; n != 0; n--, decalMaps++) {
            if (*decalMaps != nullptr && At<void*>(*decalMaps, 8) == nullptr) {
                decals--;
            }
        }
    }
    uint32_t clippedLights = At<uint16_t>(self, 0x62);
    uint32_t shadows = At<uint16_t>(self, 0x92);
    uint32_t lights = At<uint16_t>(self, 0x72);
    uint32_t clippedShadows = At<uint16_t>(self, 0x82);
    uint32_t envMaps = At<uint16_t>(self, 0xA2);
    uint32_t fogMaps = At<uint16_t>(self, 0xB2);
    if (envMaps != 0 && hasBump != 0 && At<uint32_t>(At<re::Ptr32<uint8_t>*>(self, 0x9C)[0], 0x168) != 2) {
        hasBump = 0;
    }
    uint32_t applyMode = (At<uint16_t>(texturing, 0x18) >> 1) & 7;
    bool vertexColor = applyMode != 0;
    uint8_t* alpha = At<uint8_t*>(properties, 8);
    // the alpha property blends on the source or destination alpha, or tests
    bool alphaBlend = (At<uint8_t>(alpha, 0x18) & 1) != 0;
    if (alphaBlend) {
        uint32_t src = (At<uint16_t>(alpha, 0x18) >> 1) & 0xF;
        uint32_t dest = (At<uint16_t>(alpha, 0x18) >> 5) & 0xF;
        if (src != 6 && src != 7 && src != 8 && src != 9 && src != 10 && dest != 6 && dest != 7 && dest != 8 &&
            dest != 9 && dest != 10) {
            alphaBlend = false;
        }
    }
    bool alphaTest = ((At<uint16_t>(alpha, 0x18) >> 9) & 1) != 0;
    uint8_t repeatBaseAlpha = 0;   // later passes repeat the base map's alpha
    uint8_t repeatVertexColor = 0;
    if (alphaBlend || alphaTest) {
        repeatBaseAlpha = (baseMap != nullptr && applyMode != 1) ? 1 : 0;
        repeatVertexColor = vertexColor ? 1 : 0;
    }

    // ---- base, dark, detail maps and projected lights
    uint32_t stagesLeft = lights + clippedLights * 2 + hasDark;   // stages still to place
    uint32_t texturesLeft = hasDetail + hasBase;
    uint32_t lightsLeft = lights;
    uint32_t clippedLeft = clippedLights;
    bool vertexColorLeft = vertexColor;
    while (vertexColorLeft || texturesLeft + stagesLeft != 0) {
        uint32_t textures = texturesLeft;
        if (vertexColorLeft && (clippedLeft != 0 || stagesLeft > 1 || texturesLeft == 0)) {
            textures = texturesLeft + 1;
        }
        bool fits;
        if (At<uint32_t>(self, 0x3C) == 0) {
            fits = clippedLights <= 1 && textures + stagesLeft <= ms_uiMaxSimultaneousTextures;
        } else {
            fits = stagesLeft == 0 && textures <= ms_uiMaxSimultaneousTextures;
        }
        uint8_t passBaseAlpha = fits ? 0 : repeatBaseAlpha;
        uint8_t passVertexColor = fits ? 0 : repeatVertexColor;
        if (stagesLeft == 0) {
            DefaultShader_OpenPass(self, 1, 3, passBaseAlpha, passVertexColor);
        } else {
            DefaultShader_OpenPass(self, alphaBlend ? AlphaSrcBlend(self, alpha) : 2, 2, passBaseAlpha,
                                   passVertexColor);
        }
        bool staged = false;
        if (clippedLeft != 0 && PassHasRoom(self)) {
            uint8_t* effect = At<re::Ptr32<uint8_t>*>(self, 0x5C)[clippedLights - clippedLeft];
            DefaultShader_AddEffectStage(self, effect);
            stagesLeft--;
            clippedLeft--;
            if (PassHasRoom(self)) {
                uint8_t* stage = DefaultShader_AddClippingStage(self, effect);
                StageState(stage, D3DTSS_COLOROP, 4);
                StageState(stage, D3DTSS_ALPHAOP, 3);
            }
            stagesLeft--;
            staged = true;
        }
        while (hasDark != 0 && PassHasRoom(self)) {
            uint8_t* stage = DefaultShader_AddMapStage(self, darkMap, MapTexCoord(self, darkMap));
            if (staged) {
                StageState(stage, D3DTSS_COLOROP, 7);
            } else {
                if (vertexColorLeft && clippedLights + lights + hasDark < 2) {
                    StageState(stage, D3DTSS_COLOROP, 4);
                    StageState(stage, D3DTSS_COLORARG2, 0);
                    StageState(stage, D3DTSS_ALPHAOP, 3);
                    StageState(stage, D3DTSS_ALPHAARG2, 0);
                    vertexColorLeft = false;
                }
                staged = true;
            }
            stagesLeft--;
            hasDark--;
        }
        if (lightsLeft != 0) {
            uint32_t index = lights - lightsLeft;
            do {
                if (!PassHasRoom(self)) {
                    break;
                }
                uint8_t* stage = DefaultShader_AddEffectStage(self, At<re::Ptr32<uint8_t>*>(self, 0x6C)[index]);
                if (staged) {
                    StageState(stage, D3DTSS_COLOROP, 7);
                } else {
                    if (vertexColorLeft && clippedLights + lights + hasDark < 2) {
                        StageState(stage, D3DTSS_COLOROP, 4);
                        StageState(stage, D3DTSS_COLORARG2, 0);
                        StageState(stage, D3DTSS_ALPHAOP, 3);
                        StageState(stage, D3DTSS_ALPHAARG2, 0);
                        vertexColorLeft = false;
                    }
                    staged = true;
                }
                stagesLeft--;
                lightsLeft--;
                index++;
            } while (lightsLeft != 0);
        }
        if (!fits) {
            DefaultShader_ClosePass(self, baseMap);
            continue;
        }
        if (hasBase != 0) {
            uint8_t* stage = DefaultShader_AddMapStage(self, baseMap, MapTexCoord(self, baseMap));
            if (staged) {
                if (repeatVertexColor && !vertexColorLeft) {
                    StageState(stage, D3DTSS_COLOROP, 4);
                    StageState(stage, D3DTSS_ALPHAOP, 4);
                } else {
                    StageState(stage, D3DTSS_COLOROP, 4);
                    StageState(stage, D3DTSS_ALPHAOP, 2);
                }
            } else if (vertexColorLeft) {
                StageState(stage, D3DTSS_COLOROP, 4);
                StageState(stage, D3DTSS_COLORARG2, 0);
                StageState(stage, D3DTSS_ALPHAOP, 4);
                StageState(stage, D3DTSS_ALPHAARG2, 0);
                vertexColorLeft = false;
            } else {
                StageState(stage, D3DTSS_COLOROP, 2);
                StageState(stage, D3DTSS_ALPHAOP, 2);
            }
            texturesLeft--;
            staged = true;
        }
        if (hasDetail != 0) {
            uint8_t* stage = DefaultShader_AddMapStage(self, detailMap, MapTexCoord(self, detailMap));
            if (!staged) {
                StageState(stage, D3DTSS_COLORARG2, 0);
                StageState(stage, D3DTSS_ALPHAARG2, 0);
                StageState(stage, D3DTSS_ALPHAOP, 5);
                vertexColorLeft = false;
                staged = true;
            }
            StageState(stage, D3DTSS_COLOROP, 5);
            texturesLeft--;
        }
        if (vertexColorLeft) {
            uint8_t* stage = DefaultShader_AddMapStage(self, nullptr, 0);
            if (staged && applyMode == 2) {
                StageState(stage, D3DTSS_COLOROP, 4);
                StageState(stage, D3DTSS_ALPHAOP, 4);
            } else {
                StageState(stage, D3DTSS_COLOROP, 2);
                StageState(stage, D3DTSS_ALPHAOP, 2);
            }
            vertexColorLeft = false;
        }
    }

    // ---- decals
    if (decals != 0) {
        uint32_t index = 0;
        re::Ptr32<uint8_t>* decalMaps = At<re::Ptr32<uint8_t>*>(texturing, 0x20) + 8;
        if (At<uint8_t>(self, 0xD4) != 0 && At<uint8_t>(self, 0xDC) != 0 && At<uint32_t>(self, 0x3C) == 0 &&
            !alphaBlend && !alphaTest && !vertexColor) {
            // on the first pass while there's room
            for (;;) {
                if (!PassHasRoom(self)) {
                    if (decals == 0) {
                        goto decalsDone;
                    }
                    DefaultShader_ClosePass(self, nullptr);
                    break;
                }
                decalMaps = At<re::Ptr32<uint8_t>*>(texturing, 0x20) + 8;
                uint8_t* map = decalMaps[index];
                index++;
                if (map != nullptr && At<void*>(map, 8) != nullptr) {
                    uint8_t* stage = DefaultShader_AddMapStage(self, map, MapTexCoord(self, map));
                    StageState(stage, D3DTSS_COLOROP, 0xD);   // BLENDTEXTUREALPHA
                    decals--;
                }
                if (decals == 0) {
                    goto decalsDone;
                }
            }
        } else {
            DefaultShader_ClosePass(self, baseMap);
        }
        // a pass each (several to a pass when allowed)
        do {
            uint8_t* map = (At<re::Ptr32<uint8_t>*>(texturing, 0x20) + 8)[index];
            index++;
            if (map != nullptr && At<void*>(map, 8) != nullptr) {
                uint32_t texCoord = MapTexCoord(self, map);
                DefaultShader_OpenPass(self, 5, 6, 0, 0);
                uint8_t* stage = DefaultShader_AddMapStage(self, map, texCoord);
                if (At<uint8_t>(self, 0xD4) != 0 && At<uint8_t>(self, 0xDC) != 0 && !vertexColor) {
                    StageState(stage, D3DTSS_COLOROP, 4);
                    StageState(stage, D3DTSS_COLORARG2, 0x22);   // TEXTURE | ALPHAREPLICATE
                    StageState(stage, D3DTSS_ALPHAOP, 2);
                    decals--;
                    if (decals == 0) {
                        goto decalsDone;
                    }
                    for (;;) {
                        if (!PassHasRoom(self)) {
                            if (decals == 0) {
                                goto decalsDone;
                            }
                            break;
                        }
                        uint8_t* next = (At<re::Ptr32<uint8_t>*>(texturing, 0x20) + 8)[index];
                        index++;
                        if (next != nullptr && At<void*>(next, 8) != nullptr) {
                            uint8_t* nextStage = DefaultShader_AddMapStage(self, next, MapTexCoord(self, next));
                            StageState(nextStage, D3DTSS_COLOROP, 0xD);
                            StageState(nextStage, D3DTSS_ALPHAOP, 7);
                            decals--;
                        }
                        if (decals == 0) {
                            goto decalsDone;
                        }
                    }
                    if (CurrentPass(self) == nullptr) {
                        continue;
                    }
                } else {
                    if (vertexColor) {
                        StageState(stage, D3DTSS_COLOROP, 4);
                        StageState(stage, D3DTSS_ALPHAOP, 4);
                        StageState(stage, D3DTSS_COLORARG2, 0);
                        StageState(stage, D3DTSS_ALPHAARG2, 0);
                    } else {
                        StageState(stage, D3DTSS_ALPHAOP, 2);
                    }
                    decals--;
                    if (CurrentPass(self) == nullptr) {
                        continue;
                    }
                }
                EndPass(self);
            }
        } while (decals != 0);
    }
decalsDone:

    // ---- projected shadows
    if (shadows != 0 || clippedShadows != 0) {
        uint32_t clippedLeftShadows = clippedShadows;
        uint32_t shadowsLeft = shadows;
        if (clippedShadows != 0 || DefaultShader_CanAddToPass(self, 1, 3) == 0) {
            DefaultShader_ClosePass(self, baseMap);
        }
        uint32_t total = clippedShadows + shadows;
        if (total != 0) {
            uint32_t clippedIndex = 0;
            for (;;) {
                bool staged = true;
                if (CurrentPass(self) == nullptr) {
                    DefaultShader_OpenPass(self, 1, 3, repeatBaseAlpha, repeatVertexColor);
                    staged = false;
                    if (clippedLeftShadows != 0) {
                        uint8_t* effect = At<re::Ptr32<uint8_t>*>(self, 0x7C)[clippedIndex];
                        DefaultShader_AddEffectStage(self, effect);
                        total--;
                        clippedLeftShadows--;
                        clippedIndex++;
                        if (PassHasRoom(self)) {
                            uint8_t* stage = DefaultShader_AddClippingStage(self, effect);
                            StageState(stage, D3DTSS_COLOROP, 7);
                        }
                        staged = true;
                    }
                }
                if (shadowsLeft != 0) {
                    uint32_t index = shadows - shadowsLeft;
                    do {
                        if (!PassHasRoom(self)) {
                            break;
                        }
                        uint8_t* stage = DefaultShader_AddEffectStage(self, At<re::Ptr32<uint8_t>*>(self, 0x8C)[index]);
                        if (staged) {
                            StageState(stage, D3DTSS_COLOROP, 4);
                        } else {
                            staged = true;
                        }
                        total--;
                        shadowsLeft--;
                        index++;
                    } while (shadowsLeft != 0);
                }
                if (total == 0) {
                    break;
                }
                DefaultShader_ClosePass(self, baseMap);
            }
        }
    }

    // ---- the environment map, with the bump and gloss maps
    if (envMaps != 0) {
        uint32_t bump = At<uint32_t>(self, 0xD8) == 1 ? 0 : hasBump;   // (no bump COLOROP: DISABLE)
        uint32_t gloss = hasGloss;
        uint32_t needed = (envMaps != 0 ? 1 : 0) + gloss + bump;
        uint32_t withAlpha = needed + (repeatBaseAlpha != 0 ? 1 : 0);
        if (withAlpha > ms_uiMaxTextureBlendStages) {
            do {
                if (withAlpha <= ms_uiMaxSimultaneousTextures) {
                    break;
                }
                if (gloss != 0) {
                    gloss--;
                } else if (bump != 0) {
                    bump--;
                }
                withAlpha--;
                needed--;
            } while (withAlpha > ms_uiMaxTextureBlendStages);
            hasGloss = gloss;
        }
        if (gloss != 0 || DefaultShader_CanAddToPass(self, 2, 2) == 0 || At<uint32_t>(self, 0xC0) < needed ||
            At<uint32_t>(self, 0xC4) < needed) {
            DefaultShader_ClosePass(self, baseMap);
        }
        bool staged = true;
        if (CurrentPass(self) == nullptr) {
            DefaultShader_OpenPass(self, alphaBlend ? AlphaSrcBlend(self, alpha) : 2, 2, repeatBaseAlpha,
                                   repeatVertexColor);
            staged = false;
        }
        if (bump != 0 && At<uint32_t>(self, 0xC0) > 1 && At<uint32_t>(self, 0xC4) > 1) {
            DefaultShader_AddBumpMapStage(self, bumpMap, MapTexCoord(self, bumpMap));
        }
        uint8_t* stage = DefaultShader_AddEffectStage(self, At<re::Ptr32<uint8_t>*>(self, 0x9C)[0]);
        if (staged) {
            StageState(stage, D3DTSS_COLOROP, 7);
        }
        if (hasGloss != 0) {
            uint8_t* glossStage = DefaultShader_AddMapStage(self, glossMap, MapTexCoord(self, glossMap));
            StageState(glossStage, D3DTSS_COLOROP, 4);
        }
    }

    // ---- the glow map
    if (hasGlow) {
        if (DefaultShader_CanAddToPass(self, 2, 2) == 0 || At<uint32_t>(self, 0xC0) == 0 ||
            At<uint32_t>(self, 0xC4) == 0) {
            DefaultShader_ClosePass(self, baseMap);
        }
        uint32_t op = 7;   // ADD
        if (CurrentPass(self) == nullptr) {
            DefaultShader_OpenPass(self, alphaBlend ? AlphaSrcBlend(self, alpha) : 2, 2, repeatBaseAlpha,
                                   repeatVertexColor);
            op = 2;
        }
        uint8_t* stage = DefaultShader_AddMapStage(self, glowMap, MapTexCoord(self, glowMap));
        StageState(stage, D3DTSS_COLOROP, op);
    }

    // ---- fog maps
    if (fogMaps != 0) {
        uint32_t left = fogMaps;
        uint32_t index = 0;
        if (At<uint8_t>(self, 0xD4) != 0 && At<uint32_t>(self, 0x3C) == 0) {
            for (;;) {
                if (!PassHasRoom(self)) {
                    if (left == 0) {
                        goto fogDone;
                    }
                    goto fogClose;
                }
                uint8_t* stage = DefaultShader_AddEffectStage(self, At<re::Ptr32<uint8_t>*>(self, 0xAC)[index]);
                index++;
                StageState(stage, D3DTSS_COLOROP, 0xD);
                if (--left == 0) {
                    goto fogDone;
                }
            }
        }
        if (DefaultShader_CanAddToPass(self, 5, 6) == 0) {
        fogClose:
            if (CurrentPass(self) != nullptr) {
                EndPass(self);
            }
        }
        if (left != 0) {
            do {
                if (!PassHasRoom(self) && CurrentPass(self) != nullptr) {
                    EndPass(self);
                }
                uint8_t* effect = At<re::Ptr32<uint8_t>*>(self, 0xAC)[index];
                index++;
                if (CurrentPass(self) == nullptr) {
                    DefaultShader_OpenPass(self, 5, 6, repeatBaseAlpha, repeatVertexColor);
                }
                uint8_t* stage = DefaultShader_AddEffectStage(self, effect);
                StageState(stage, D3DTSS_ALPHAOP, 2);
                left--;
                if (CurrentPass(self) != nullptr) {
                    EndPass(self);
                }
            } while (left != 0);
        }
    }
fogDone:
    DefaultShader_ClosePass(self, baseMap);
    DefaultShader_SetupFog(self, At<uint8_t*>(properties, 0x10), alpha, baseMap, applyMode);
    return 0;
}

// 0x89C8F0 (thiscall, slot 7): NiD3DDefaultShader::UpdatePipeline (the passes reset (slot 38) and
// built again)
uint32_t __fastcall DefaultShader_UpdatePipeline(uint8_t* self, re::Edx, uint8_t* geometry, void* skin,
                                                 uint8_t* bufferData, uint8_t* properties, uint8_t* effects,
                                                 const void* world, const void* bound) {
    Virtual<void>(self, 38);
    return DefaultShader_ConstructPipeline(self, geometry, skin, bufferData, properties, effects, world, bound);
}

#define C41A_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C41A_REVERSED("NiD3DDefaultShader::AddClippingStage", 0x0089AF00, DefaultShader_AddClippingStageImpl);
C41A_REVERSED("NiD3DDefaultShader::DeletingDestructor", 0x0089B130, DefaultShader_DeletingDestructor);
C41A_REVERSED("NiD3DDefaultShader::ClosePass", 0x0089B160, DefaultShader_ClosePassImpl);
C41A_REVERSED("NiD3DDefaultShader::AddEffect", 0x0089B220, DefaultShader_AddEffectImpl);
C41A_REVERSED("NiD3DDefaultShader::OpenPass", 0x0089B310, DefaultShader_OpenPassImpl);
C41A_REVERSED("NiD3DDefaultShader::PrepareEffects", 0x0089B460, DefaultShader_PrepareEffectsImpl);
C41A_REVERSED("NiD3DDefaultShader::SetupFog", 0x0089B4F0, DefaultShader_SetupFogImpl);
C41A_REVERSED("NiD3DDefaultShader::NiD3DDefaultShader", 0x0089B820, DefaultShader_Construct);
C41A_REVERSED("NiD3DDefaultShader::ConstructPipeline", 0x0089B940, DefaultShader_ConstructPipelineImpl);
C41A_REVERSED("NiD3DDefaultShader::UpdatePipeline", 0x0089C8F0, DefaultShader_UpdatePipeline);

} // namespace ni::c41
