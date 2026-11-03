// NiD3DDefaultShader (vtable 0x99D910, NiRTTI 0x20E736C; 0xE4 bytes): chunk 40's part of it, the
// slots it overrides around the pipeline (4 Initialize, 6 PreProcessPipeline, 13
// PostProcessPipeline), its destructor and the stage builders the pass construction (chunk 41's
// NiD3DDefaultShader.cpp, which has the layout) calls: a texturing map's stage, the bump map's, a
// texture effect's. Names past Gamebryo's slots are chunk 41's guesses, kept the same.
//
// Every builder works on the current pass (+0x40): a new stage from the pool (held by a local and
// let go at the end, the pass keeping its own reference), set as the pass's next stage; the
// stages (+0xC0) and textures (+0xC4) left in the pass counted down. +0xB9 set means the pass
// still has to take the vertex colour's alpha (ALPHAARG2 DIFFUSE once).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9RendererData.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::c40 {
namespace {

constexpr uintptr_t kDefaultShaderVtable = 0x0099D910;
constexpr uintptr_t kPrimitiveArrayVtable = 0x00961988;   // NiTPrimitiveArray<NiTextureEffect*>
GAME_VAR(uint8_t, NiRTTI_NiD3DDefaultShader, 0x020E736C);
// the texture effects' texture transform set-up, by their texture type (+0x168)
GAME_ARRAY(void*, DefaultShader_apfnTextureEffectSetup, 0x020E6E40);
using TextureEffectSetup = void(__cdecl*)(void* worldProjection, void* worldTranslate, uint8_t* stage, uint32_t,
                                          void* renderer);

// D3DRENDERSTATETYPE / D3DTEXTURESTAGESTATETYPE
constexpr uint32_t D3DRS_SRCBLEND = 0x13;
constexpr uint32_t D3DRS_DESTBLEND = 0x14;
constexpr uint32_t D3DRS_LIGHTING = 0x89;
constexpr uint32_t D3DTSS_COLOROP = 1;
constexpr uint32_t D3DTSS_COLORARG1 = 2;
constexpr uint32_t D3DTSS_COLORARG2 = 3;
constexpr uint32_t D3DTSS_ALPHAOP = 4;
constexpr uint32_t D3DTSS_ALPHAARG1 = 5;
constexpr uint32_t D3DTSS_ALPHAARG2 = 6;
constexpr uint32_t D3DTSS_BUMPENVMAT00 = 7;
constexpr uint32_t D3DTSS_BUMPENVMAT01 = 8;
constexpr uint32_t D3DTSS_BUMPENVMAT10 = 9;
constexpr uint32_t D3DTSS_BUMPENVMAT11 = 10;
constexpr uint32_t D3DTSS_TEXCOORDINDEX = 0xB;
constexpr uint32_t D3DTSS_BUMPENVLSCALE = 0x16;
constexpr uint32_t D3DTSS_BUMPENVLOFFSET = 0x17;
constexpr uint32_t D3DTSS_TEXTURETRANSFORMFLAGS = 0x18;
constexpr uint32_t D3DTSS_RESULTARG = 0x1C;
// D3DTA_* / D3DTOP_*
constexpr uint32_t D3DTA_DIFFUSE = 0;
constexpr uint32_t D3DTA_CURRENT = 1;
constexpr uint32_t D3DTA_TEXTURE = 2;
constexpr uint32_t D3DTOP_SELECTARG1 = 2;
constexpr uint32_t D3DTOP_SELECTARG2 = 3;

GAME_FN(void, __thiscall, DefaultShaderC40_D3DShaderDestruct, 0x00889CD0, void* self);
GAME_FN(uint8_t, __thiscall, DefaultShaderC40_D3DShaderInitialize, 0x00888740, void* self);
GAME_FN(uint32_t, __thiscall, DefaultShaderC40_D3DShaderPreProcessPipeline, 0x00889210, void* self, void* geometry,
        void* skin, void* bufferData, void* properties, void* effects, const void* world, const void* bound);
GAME_FN(uint32_t, __thiscall, DefaultShaderC40_D3DShaderPostProcessPipeline, 0x00888750, void* self, void* geometry,
        void* skin, void* bufferData, void* properties, void* effects, const void* world, const void* bound);
// the device's bump map (+0xD4) and bump op (+0xD8) choices read from the renderer's caps (outside
// the chunk; name a guess)
GAME_FN(void, __thiscall, DefaultShaderC40_InitializeBumpOp, 0x008A6460, void* self);
GAME_FN(void, __thiscall, DefaultShaderC40_UpdateRenderState, 0x008839F0, void* renderState, void* properties);
GAME_FN(void, __thiscall, DefaultShaderC40_LightManagerSetState, 0x00892260, void* lights, void* effects,
        void* material, void* vertexColor);
GAME_FN(uint8_t, __thiscall, DefaultShaderC40_CheckFreeStages, 0x00883C90, void* pass, uint32_t stages,
        uint32_t textures);
GAME_FN(uint32_t, __thiscall, DefaultShaderC40_PassSetStage, 0x00884830, void* pass, uint32_t index, uint8_t* stage);
GAME_FN(void, __cdecl, DefaultShaderC40_CreateNewStage, 0x0089F810, re::Ptr32<uint8_t>* stage);
GAME_FN(void, __thiscall, DefaultShaderC40_ReturnStageToPool, 0x0089F670, void* stage);
GAME_FN(void, __thiscall, DefaultShaderC40_SetFilterMode, 0x0089E640, void* stage, uint32_t mode);
GAME_FN(void, __thiscall, DefaultShaderC40_SetClampMode, 0x0089E5E0, void* stage, uint32_t mode);
GAME_FN(void, __thiscall, DefaultShaderC40_SetStageState, 0x0089F9E0, void* group, uint32_t state, uint32_t value,
        int save);
GAME_FN(uint8_t, __thiscall, DefaultShaderC40_GetRenderState, 0x0089DE70, void* group, uint32_t state,
        uint32_t* value, uint8_t* saved);
GAME_FN(const float*, __thiscall, DefaultShaderC40_TextureTransformGetMatrix, 0x0074A440, void* transform);
GAME_FN(void, __thiscall, DefaultShaderC40_TextureTransformUpdateMatrix, 0x0077EB30, void* transform);
GAME_FN(void, __thiscall, DefaultShaderC40_SetTextureTransform, 0x0089A830, void* self, const float* matrix,
        uint8_t* stage);

inline void StageState(uint8_t* stage, uint32_t state, uint32_t value) {
    DefaultShaderC40_SetStageState(At<void*>(stage, 0xC), state, value, 0);
}

// ALPHAARG2: the vertex colour once when the pass still has to take it (+0xB9, then cleared),
// else `otherwise`
inline void AlphaArg2(uint8_t* self, uint8_t* stage, uint32_t otherwise) {
    if (At<uint8_t>(self, 0xB9) != 0) {
        StageState(stage, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        At<uint8_t>(self, 0xB9) = 0;
    } else {
        StageState(stage, D3DTSS_ALPHAARG2, otherwise);
    }
}

// the stage local's reference let go (a plain count, as the library inlines it)
inline void ReleaseStage(uint8_t* stage) {
    if (stage != nullptr && --At<int32_t>(stage, 0x5C) == 0) {
        DefaultShaderC40_ReturnStageToPool(stage);
    }
}

// one of the six NiTPrimitiveArray<NiTextureEffect*> emptied: its used slots nulled, size and
// used count 0
inline void ClearEffectArray(uint8_t* array) {
    for (uint16_t i = 0; i < At<uint16_t>(array, 0xA); i++) {
        At<re::Ptr32<void>*>(array, 4)[i] = nullptr;
    }
    At<uint16_t>(array, 0xA) = 0;
    At<uint16_t>(array, 0xC) = 0;
}

} // namespace

// 0x89A5F0 (thiscall, slot 4): NiD3DDefaultShader::Initialize (once: NiD3DShader's, then the bump
// op choice; false when NiD3DShader's fails)
uint8_t __fastcall DefaultShaderC40_Initialize(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 0x10) == 0) {
        if (DefaultShaderC40_D3DShaderInitialize(self) == 0) {
            return 0;
        }
        DefaultShaderC40_InitializeBumpOp(self);
    }
    return 1;
}

// 0x89A610 (thiscall, slot 13): NiD3DDefaultShader::PostProcessPipeline (lighting put back to what
// PreProcessPipeline found (+0xC8), straight to the device through the render state's cache;
// then NiD3DShader's)
uint32_t __fastcall DefaultShaderC40_PostProcessPipeline(uint8_t* self, re::Edx, void* geometry, void* skin,
                                                         void* bufferData, void* properties, void* effects,
                                                         const void* world, const void* bound) {
    uint32_t lighting = At<uint32_t>(self, 0xC8);
    uint8_t* state = At<uint8_t*>(self, 0x1C);
    if (At<uint32_t>(state, 0x120 + D3DRS_LIGHTING * 8) != lighting) {
        Com<long>(At<void*>(state, 0x2238), 57, D3DRS_LIGHTING, lighting);   // SetRenderState
        At<uint32_t>(state, 0x120 + D3DRS_LIGHTING * 8) = lighting;
    }
    return DefaultShaderC40_D3DShaderPostProcessPipeline(self, geometry, skin, bufferData, properties, effects, world,
                                                         bound);
}

// 0x89A680 (thiscall): whether the current pass blends with (src, dest) (true with no pass yet or
// no passes counted; else its group's SRCBLEND and DESTBLEND compared). (The game compares
// uninitialized locals when the pass has no render state group or the group lacks the state:
// they're 0 here, which matches no D3DBLEND.) (Name chunk 41's guess: CanAddToPass.)
uint8_t __fastcall DefaultShaderC40_CanAddToPass(uint8_t* self, re::Edx, uint32_t src, uint32_t dest) {
    if (At<uint32_t>(self, 0x3C) == 0 || At<uint8_t*>(self, 0x40) == nullptr) {
        return 1;
    }
    uint32_t value = 0;
    uint8_t saved = 0;
    void* group = At<void*>(At<uint8_t*>(self, 0x40), 0x30);
    if (group != nullptr) {
        DefaultShaderC40_GetRenderState(group, D3DRS_SRCBLEND, &value, &saved);
    }
    if (value != src) {
        return 0;
    }
    uint32_t value2 = 0;
    group = At<void*>(At<uint8_t*>(self, 0x40), 0x30);
    if (group != nullptr) {
        DefaultShaderC40_GetRenderState(group, D3DRS_DESTBLEND, &value2, &saved);
    }
    return value2 == dest ? 1 : 0;
}

// 0x89A700 (thiscall): the dynamic effect arrays (+0x58..+0xA8) emptied, the effect state last
// sorted (+0xE0) forgotten (BeginFrame, 0x89B120, jumps here). (Name chunk 41's guess.)
void __fastcall DefaultShaderC40_ClearEffects(uint8_t* self, re::Edx) {
    for (int i = 0; i < 6; i++) {
        ClearEffectArray(self + 0x58 + i * 0x10);
    }
    At<uint32_t>(self, 0xE0) = 0;
}

// 0x89A830 (thiscall, `this` unused): a stage's texture transform from a 3x3 matrix's first two
// rows (COUNT2: the rows as the D3D matrix's first two columns, the rest 0). (Name a guess.)
void __fastcall DefaultShaderC40_SetTextureTransformImpl(void*, re::Edx, const float* m, uint8_t* stage) {
    At<uint8_t>(stage, 0x5A) = 1;
    StageState(stage, D3DTSS_TEXTURETRANSFORMFLAGS, 2);   // D3DTTFF_COUNT2
    At<float>(stage, 0x18) = m[0];
    At<float>(stage, 0x28) = m[1];
    At<float>(stage, 0x38) = m[2];
    At<float>(stage, 0x48) = 0.0f;
    At<float>(stage, 0x1C) = m[3];
    At<float>(stage, 0x2C) = m[4];
    At<float>(stage, 0x3C) = m[5];
    At<float>(stage, 0x4C) = 0.0f;
    At<float>(stage, 0x50) = 0.0f;
    At<float>(stage, 0x40) = 0.0f;
    At<float>(stage, 0x30) = 0.0f;
    At<float>(stage, 0x20) = 0.0f;
    At<float>(stage, 0x54) = 0.0f;
    At<float>(stage, 0x44) = 0.0f;
    At<float>(stage, 0x34) = 0.0f;
    At<float>(stage, 0x24) = 0.0f;
}

// 0x89A8A0 (thiscall): NiD3DDefaultShader::~NiD3DDefaultShader (the effects cleared, the fog flags
// and the six arrays' storage freed, then NiD3DShader's)
void __fastcall DefaultShaderC40_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kDefaultShaderVtable;
    DefaultShaderC40_ClearEffects(self, nullptr);
    NiFree(At<void*>(self, 0xCC));
    for (int i = 5; i >= 0; i--) {
        uint8_t* array = self + 0x58 + i * 0x10;
        At<uint32_t>(array, 0) = kPrimitiveArrayVtable;
        NiFree(At<void*>(array, 4));
    }
    DefaultShaderC40_D3DShaderDestruct(self);
}

// 0x89A930 (slot 2): GetRTTI
const void* __fastcall DefaultShaderC40_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiD3DDefaultShader;
}

// 0x89A940 (thiscall, slot 6): NiD3DDefaultShader::PreProcessPipeline (the property state applied
// and the lights set (the renderer's light manager, not checked for null), then NiD3DShader's
// (which does both again when its flags say so); the lighting state it ends with kept (+0xC8)
// for PostProcessPipeline)
uint32_t __fastcall DefaultShaderC40_PreProcessPipeline(uint8_t* self, re::Edx, void* geometry, void* skin,
                                                        void* bufferData, uint8_t* properties, void* effects,
                                                        const void* world, const void* bound) {
    DefaultShaderC40_UpdateRenderState(At<void*>(self, 0x1C), properties);
    DefaultShaderC40_LightManagerSetState(At<void*>(At<uint8_t*>(self, 0x18), ni::dx9::kLightManager), effects,
                                          At<void*>(properties, 0x28), At<void*>(properties, 0x2C));
    uint32_t result = DefaultShaderC40_D3DShaderPreProcessPipeline(self, geometry, skin, bufferData, properties,
                                                                   effects, world, bound);
    At<uint32_t>(self, 0xC8) = At<uint32_t>(At<uint8_t*>(self, 0x1C), 0x120 + D3DRS_LIGHTING * 8);
    return result;
}

// 0x89A9B0 (thiscall): a texturing map's stage (the result CURRENT). With a texture: COLOROP
// SELECTARG1 of the texture, ALPHAOP SELECTARG2 of (texture, current / the vertex colour); the
// map's texture coordinate set, filter and clamp modes, its texture transform (COUNT2) or none; a
// texture counted. Without one (or no map): the vertex colour selected (alpha: SELECTARG1 of the
// vertex colour when it's still owed, else SELECTARG2 current), a texture counted only when any
// are left. The stage index isn't set here. The stage, null when the pass is full or there is
// none. (Name chunk 41's guess: AddMapStage.)
uint8_t* __fastcall DefaultShaderC40_AddMapStage(uint8_t* self, re::Edx, uint8_t* map, uint32_t texCoord) {
    if (At<uint8_t*>(self, 0x40) == nullptr ||
        DefaultShaderC40_CheckFreeStages(At<uint8_t*>(self, 0x40), 1, map != nullptr ? 1 : 0) == 0) {
        return nullptr;
    }
    re::Ptr32<uint8_t> held = nullptr;
    DefaultShaderC40_CreateNewStage(&held);
    uint8_t* stage = held;
    StageState(stage, D3DTSS_RESULTARG, D3DTA_CURRENT);
    if (map != nullptr && At<void*>(map, 8) != nullptr) {
        At<void*>(stage, 4) = At<void*>(map, 8);
        StageState(stage, D3DTSS_TEXCOORDINDEX, texCoord);
        StageState(stage, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        StageState(stage, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        StageState(stage, D3DTSS_COLORARG2, D3DTA_CURRENT);
        StageState(stage, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
        StageState(stage, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        AlphaArg2(self, stage, D3DTA_CURRENT);
        DefaultShaderC40_SetFilterMode(stage, At<uint8_t>(map, 5) & 0xF);
        DefaultShaderC40_SetClampMode(stage, (uint32_t(At<uint16_t>(map, 4)) >> 12) & 3);
        void* transform = At<void*>(map, 0xC);
        if (transform == nullptr) {
            At<uint8_t>(stage, 0x5A) = 0;
            StageState(stage, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
        } else {
            DefaultShaderC40_SetTextureTransform(self, DefaultShaderC40_TextureTransformGetMatrix(transform), stage);
        }
        At<uint32_t>(self, 0xC4)--;
    } else {
        StageState(stage, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        StageState(stage, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
        StageState(stage, D3DTSS_COLORARG2, D3DTA_CURRENT);
        if (At<uint8_t>(self, 0xB9) != 0) {
            StageState(stage, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
            At<uint8_t>(self, 0xB9) = 0;
        } else {
            StageState(stage, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
        }
        StageState(stage, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
        StageState(stage, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        if (At<uint32_t>(self, 0xC4) != 0) {
            At<uint32_t>(self, 0xC4)--;
        }
    }
    At<uint32_t>(self, 0xC0)--;
    DefaultShaderC40_PassSetStage(At<uint8_t*>(self, 0x40), At<uint32_t>(At<uint8_t*>(self, 0x40), 0x14), stage);
    ReleaseStage(held);
    return stage;
}

// 0x89ABB0 (thiscall): the bump map's stage (the pass's next index; COLOROP the device's bump op
// (+0xD8: BUMPENVMAP or BUMPENVMAPLUMINANCE) of the texture; alpha SELECTARG2 as a map's; with a
// texture: the filter / clamp modes, the texture transform (made current first when dirty) or
// none, and the bump matrix, luminance scale and offset from the map (the float bits)). A stage
// and a texture counted. (Name chunk 41's guess: AddBumpMapStage.)
uint8_t* __fastcall DefaultShaderC40_AddBumpMapStage(uint8_t* self, re::Edx, uint8_t* map, uint32_t texCoord) {
    if (At<uint8_t*>(self, 0x40) == nullptr || DefaultShaderC40_CheckFreeStages(At<uint8_t*>(self, 0x40), 1, 1) == 0) {
        return nullptr;
    }
    re::Ptr32<uint8_t> held = nullptr;
    DefaultShaderC40_CreateNewStage(&held);
    uint8_t* stage = held;
    At<uint32_t>(stage, 0) = At<uint32_t>(At<uint8_t*>(self, 0x40), 0x14);
    void* texture = At<void*>(map, 8);
    At<void*>(stage, 4) = texture;
    StageState(stage, D3DTSS_TEXCOORDINDEX, texCoord);
    StageState(stage, D3DTSS_COLOROP, At<uint32_t>(self, 0xD8));
    StageState(stage, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    StageState(stage, D3DTSS_COLORARG2, D3DTA_CURRENT);
    StageState(stage, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    StageState(stage, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    AlphaArg2(self, stage, D3DTA_CURRENT);
    if (texture != nullptr) {
        DefaultShaderC40_SetFilterMode(stage, At<uint8_t>(map, 5) & 0xF);
        DefaultShaderC40_SetClampMode(stage, (uint32_t(At<uint16_t>(map, 4)) >> 12) & 3);
        uint8_t* transform = At<uint8_t*>(map, 0xC);
        if (transform == nullptr) {
            At<uint8_t>(stage, 0x5A) = 0;
            StageState(stage, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
        } else {
            if (At<uint8_t>(transform, 0x1C) != 0) {
                DefaultShaderC40_TextureTransformUpdateMatrix(transform);
            }
            DefaultShaderC40_SetTextureTransform(self, &At<float>(transform, 0x20), stage);
        }
        StageState(stage, D3DTSS_BUMPENVMAT00, At<uint32_t>(map, 0x18));
        StageState(stage, D3DTSS_BUMPENVMAT01, At<uint32_t>(map, 0x1C));
        StageState(stage, D3DTSS_BUMPENVMAT10, At<uint32_t>(map, 0x20));
        StageState(stage, D3DTSS_BUMPENVMAT11, At<uint32_t>(map, 0x24));
        StageState(stage, D3DTSS_BUMPENVLSCALE, At<uint32_t>(map, 0x10));
        StageState(stage, D3DTSS_BUMPENVLOFFSET, At<uint32_t>(map, 0x14));
    }
    At<uint32_t>(self, 0xC0)--;
    At<uint32_t>(self, 0xC4)--;
    DefaultShaderC40_PassSetStage(At<uint8_t*>(self, 0x40), At<uint32_t>(At<uint8_t*>(self, 0x40), 0x14), stage);
    ReleaseStage(held);
    return stage;
}

// 0x89ADB0 (thiscall): a texture effect's stage (the pass's next index; the effect's texture
// (+0x158) on texture coordinate set 1, SELECTARG1 of it, alpha as a map's; the texture
// transform by the effect's texture type's set-up function (its projection +0x128 and
// translation +0x14C); its filter (+0x15C) and clamp (+0x160) modes). A stage and a texture
// counted. (Name chunk 41's guess: AddEffectStage.)
uint8_t* __fastcall DefaultShaderC40_AddEffectStage(uint8_t* self, re::Edx, uint8_t* effect) {
    if (At<uint8_t*>(self, 0x40) == nullptr || DefaultShaderC40_CheckFreeStages(At<uint8_t*>(self, 0x40), 1, 1) == 0) {
        return nullptr;
    }
    re::Ptr32<uint8_t> held = nullptr;
    DefaultShaderC40_CreateNewStage(&held);
    uint8_t* stage = held;
    At<uint32_t>(stage, 0) = At<uint32_t>(At<uint8_t*>(self, 0x40), 0x14);
    At<void*>(stage, 4) = At<void*>(effect, 0x158);
    StageState(stage, D3DTSS_TEXCOORDINDEX, 1);
    StageState(stage, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    StageState(stage, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    StageState(stage, D3DTSS_COLORARG2, D3DTA_CURRENT);
    StageState(stage, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    StageState(stage, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    AlphaArg2(self, stage, D3DTA_CURRENT);
    re::PtrCast<TextureEffectSetup>(DefaultShader_apfnTextureEffectSetup[At<uint32_t>(effect, 0x168)])(
        effect + 0x128, effect + 0x14C, stage, 0, At<void*>(self, 0x18));
    DefaultShaderC40_SetFilterMode(stage, At<uint32_t>(effect, 0x15C));
    DefaultShaderC40_SetClampMode(stage, At<uint32_t>(effect, 0x160));
    At<uint32_t>(self, 0xC0)--;
    At<uint32_t>(self, 0xC4)--;
    DefaultShaderC40_PassSetStage(At<uint8_t*>(self, 0x40), At<uint32_t>(At<uint8_t*>(self, 0x40), 0x14), stage);
    ReleaseStage(held);
    return stage;
}

#define C40P_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C40P_REVERSED("NiD3DDefaultShader::Initialize", 0x0089A5F0, DefaultShaderC40_Initialize);
C40P_REVERSED("NiD3DDefaultShader::PostProcessPipeline", 0x0089A610, DefaultShaderC40_PostProcessPipeline);
C40P_REVERSED("NiD3DDefaultShader::CanAddToPass", 0x0089A680, DefaultShaderC40_CanAddToPass);
C40P_REVERSED("NiD3DDefaultShader::ClearEffects", 0x0089A700, DefaultShaderC40_ClearEffects);
C40P_REVERSED("NiD3DDefaultShader::SetTextureTransform", 0x0089A830, DefaultShaderC40_SetTextureTransformImpl);
C40P_REVERSED("NiD3DDefaultShader::~NiD3DDefaultShader", 0x0089A8A0, DefaultShaderC40_Destruct);
C40P_REVERSED("NiD3DDefaultShader::GetRTTI", 0x0089A930, DefaultShaderC40_GetRTTI);
C40P_REVERSED("NiD3DDefaultShader::PreProcessPipeline", 0x0089A940, DefaultShaderC40_PreProcessPipeline);
C40P_REVERSED("NiD3DDefaultShader::AddMapStage", 0x0089A9B0, DefaultShaderC40_AddMapStage);
C40P_REVERSED("NiD3DDefaultShader::AddBumpMapStage", 0x0089ABB0, DefaultShaderC40_AddBumpMapStage);
C40P_REVERSED("NiD3DDefaultShader::AddEffectStage", 0x0089ADB0, DefaultShaderC40_AddEffectStage);

} // namespace ni::c40
