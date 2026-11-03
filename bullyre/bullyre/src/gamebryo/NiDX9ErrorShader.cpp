// NiDX9ErrorShader (vtable 0x99D848, 39 slots, NiRTTI 0x20E7338; 0x5C bytes): the shader the DX9
// renderer draws geometry with when its own shader failed. One pass, one stage: the colour is the
// texture factor (a random colour, each channel 0x80..0xFF, each draw), no texture, no specular /
// alpha blending / alpha test / stencil; the model transform jittered each draw by up to a tenth
// of the bound's radius (the "something is wrong" shimmer).
//
// Layout past NiD3DShader's 0x58: +0x58 the pass (a raw pointer to the one the pass array at +0x44
// holds). Its constructor is 0x89A260 (NiDX9RendererCreate.cpp calls it "Obj5C_Construct").

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9RendererData.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::c40 {
namespace {

constexpr uintptr_t kErrorShaderVtable = 0x0099D848;
constexpr uint32_t kErrorShaderSize = 0x5C;
GAME_VAR(uint8_t, NiRTTI_NiDX9ErrorShader, 0x020E7338);
const char* const kErrorShaderName = reinterpret_cast<const char*>(0x0099D8E4);   // "NiDX9ErrorShader"

// D3DRENDERSTATETYPE / D3DTEXTURESTAGESTATETYPE
constexpr uint32_t D3DRS_ALPHATESTENABLE = 0x0F;
constexpr uint32_t D3DRS_ALPHABLENDENABLE = 0x1B;
constexpr uint32_t D3DRS_SPECULARENABLE = 0x1D;
constexpr uint32_t D3DRS_STENCILENABLE = 0x34;
constexpr uint32_t D3DRS_TEXTUREFACTOR = 0x3C;
constexpr uint32_t D3DTSS_COLOROP = 1;
constexpr uint32_t D3DTSS_COLORARG1 = 2;
constexpr uint32_t D3DTSS_ALPHAOP = 4;
constexpr uint32_t D3DTSS_ALPHAARG1 = 5;
constexpr uint32_t D3DTSS_TEXCOORDINDEX = 0xB;
constexpr uint32_t D3DTSS_TEXTURETRANSFORMFLAGS = 0x18;

GAME_FN(void*, __thiscall, ErrorShader_D3DShaderConstruct, 0x007470B0, void* self);
GAME_FN(void, __thiscall, ErrorShader_D3DShaderDestruct, 0x00889CD0, void* self);
GAME_FN(uint8_t, __thiscall, ErrorShader_D3DShaderInitialize, 0x00888740, void* self);
GAME_FN(uint32_t, __thiscall, ErrorShader_ShaderUpdatePipeline, 0x0077C2A0, void* self, void* geometry, void* skin,
        void* bufferData, void* properties, void* effects, const void* world, const void* bound);
GAME_FN(uint32_t, __thiscall, ErrorShader_D3DShaderSetupTransformations, 0x00889270, void* self, void* geometry,
        void* skin, void* partition, void* bufferData, void* properties, void* effects, const void* world,
        const void* bound);
GAME_FN(void, __thiscall, ErrorShader_SetName, 0x0077C3A0, void* self, re::Ptr32<char>* name);   // NiObjectNET::SetName
GAME_FN(void*, __cdecl, ErrorShader_DeclarationCreate, 0x00885E80, void* renderer, uint32_t entries,
        uint32_t streams);
GAME_FN(void, __cdecl, ErrorShader_CreateNewPass, 0x00884A90, re::Ptr32<uint8_t>* pass);
GAME_FN(void, __thiscall, ErrorShader_ReturnPassToPool, 0x008846D0, void* pass);
GAME_FN(uint32_t, __thiscall, ErrorShader_PassSetStage, 0x00884830, void* pass, uint32_t index, uint8_t* stage);
GAME_FN(void, __thiscall, ErrorShader_PassArraySetAt, 0x00746990, void* array, uint32_t index, const re::Ptr32<uint8_t>* pass);
GAME_FN(void, __thiscall, ErrorShader_PassArrayRemoveAll, 0x00746AE0, void* array);
GAME_FN(void, __cdecl, ErrorShader_CreateNewStage, 0x0089F810, re::Ptr32<uint8_t>* stage);
GAME_FN(void, __thiscall, ErrorShader_ReturnStageToPool, 0x0089F670, void* stage);
GAME_FN(void, __thiscall, ErrorShader_SetStageState, 0x0089F9E0, void* group, uint32_t state, uint32_t value,
        int save);
GAME_FN(void, __thiscall, ErrorShader_SetSamplerState, 0x0089FB10, void* group, uint32_t state, uint32_t value,
        int save, int useMapValue);
GAME_FN(void*, __cdecl, ErrorShader_GetFreeRenderStateGroup, 0x0089E3A0);
GAME_FN(void, __thiscall, ErrorShader_SetGroupRenderState, 0x0089E1F0, void* group, uint32_t state, uint32_t value,
        int save);

// the pass's render state group (made when it has none) given a state, saved
void ErrorShader_PassRenderState(uint8_t* self, uint32_t state, uint32_t value) {
    uint8_t* pass = At<uint8_t*>(self, 0x58);
    if (At<void*>(pass, 0x30) == nullptr) {
        At<void*>(pass, 0x30) = ErrorShader_GetFreeRenderStateGroup();
    }
    ErrorShader_SetGroupRenderState(At<void*>(pass, 0x30), state, value, 1);
}

// one colour channel: 0x80 less the low byte of rand() / -32767 (a float) * 127 (a float),
// truncated: 0x80..0xFF
uint8_t ErrorShader_RandomChannel() {
    float scaled = float(double(crt::Rand()) / -32767.0);
    scaled = float(double(scaled) * 127.0);
    return uint8_t(0x80 - uint8_t(int32_t(scaled)));
}

} // namespace

// 0x899D80 (thiscall, slot 7): NiDX9ErrorShader::UpdatePipeline (the texture factor a random
// opaque colour, red drawn first; specular, alpha blending, alpha test and stencil off, all saved
// in the pass's render state group; then NiShader's, 0x77C2A0)
uint32_t __fastcall ErrorShader_UpdatePipeline(uint8_t* self, re::Edx, void* geometry, void* skin, void* bufferData,
                                               void* properties, void* effects, const void* world,
                                               const void* bound) {
    uint8_t red = ErrorShader_RandomChannel();
    uint8_t green = ErrorShader_RandomChannel();
    uint8_t blue = ErrorShader_RandomChannel();
    uint32_t color = (((uint32_t(red) | 0xFFFFFF00u) << 8 | green) << 8) | blue;
    ErrorShader_PassRenderState(self, D3DRS_TEXTUREFACTOR, color);
    ErrorShader_PassRenderState(self, D3DRS_SPECULARENABLE, 0);
    ErrorShader_PassRenderState(self, D3DRS_ALPHABLENDENABLE, 0);
    ErrorShader_PassRenderState(self, D3DRS_ALPHATESTENABLE, 0);
    ErrorShader_PassRenderState(self, D3DRS_STENCILENABLE, 0);
    return ErrorShader_ShaderUpdatePipeline(self, geometry, skin, bufferData, properties, effects, world, bound);
}

// 0x899F50 (thiscall, slot 9): NiDX9ErrorShader::SetupTransformations (the world transform
// copied, its translation moved by (2 * rand() / 32767 - 1) * radius / 10 on each axis; then
// NiD3DShader's with that, no skin and no partition)
uint32_t __fastcall ErrorShader_SetupTransformations(uint8_t* self, re::Edx, void* geometry, void*, void*,
                                                     void* bufferData, void* properties, void* effects,
                                                     const float* world, const float* bound) {
    float local[13];   // NiTransform: the rotation, +0x24 the translation, +0x30 the scale
    for (int i = 0; i < 13; i++) {
        local[i] = world[i];
    }
    float scale = float(double(bound[3]) * double(0.1f));   // 0.1 as the float the double holds
    for (int axis = 0; axis < 3; axis++) {
        int32_t r = crt::Rand();
        float jitter = float(double(r + r) / 32767.0);
        jitter = float(double(jitter) - 1.0);
        local[9 + axis] = local[9 + axis] + jitter * scale;
    }
    return ErrorShader_D3DShaderSetupTransformations(self, geometry, nullptr, nullptr, bufferData, properties,
                                                     effects, local, bound);
}

// 0x89A040 (slot 2): GetRTTI
const void* __fastcall ErrorShader_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiDX9ErrorShader;
}

// 0x89A050 (thiscall): the one pass built (a name guess: CreateStagesAndPasses): one stage, the
// texture factor as the colour and alpha, texture coordinates 0, clamped, linear without mips,
// no texture transform; the pass kept at +0x58 and set as pass 0, the pass count 1
uint8_t __fastcall ErrorShader_CreatePass(uint8_t* self, re::Edx) {
    re::Ptr32<uint8_t> pass = nullptr;
    re::Ptr32<uint8_t> stage = nullptr;
    ErrorShader_CreateNewPass(&pass);
    At<uint8_t*>(self, 0x58) = pass;
    ErrorShader_CreateNewStage(&stage);
    At<uint32_t>(stage, 0) = 0;   // the stage index
    At<uint32_t>(stage, 4) = 0;   // the texture
    ErrorShader_SetStageState(At<void*>(stage, 0xC), D3DTSS_COLOROP, 2, 0);     // SELECTARG1
    ErrorShader_SetStageState(At<void*>(stage, 0xC), D3DTSS_COLORARG1, 3, 0);   // TFACTOR
    ErrorShader_SetStageState(At<void*>(stage, 0xC), D3DTSS_ALPHAOP, 2, 0);
    ErrorShader_SetStageState(At<void*>(stage, 0xC), D3DTSS_ALPHAARG1, 3, 0);
    ErrorShader_SetStageState(At<void*>(stage, 0xC), D3DTSS_TEXCOORDINDEX, 0, 0);
    // Gamebryo's sampler states: 0 / 1 ADDRESSU / V (3 CLAMP), 4 / 5 MAG / MINFILTER (2 LINEAR),
    // 6 MIPFILTER (0 NONE)
    ErrorShader_SetSamplerState(At<void*>(stage, 0xC), 0, 3, 0, 0);
    ErrorShader_SetSamplerState(At<void*>(stage, 0xC), 1, 3, 0, 0);
    ErrorShader_SetSamplerState(At<void*>(stage, 0xC), 4, 2, 0, 0);
    ErrorShader_SetSamplerState(At<void*>(stage, 0xC), 5, 2, 0, 0);
    ErrorShader_SetSamplerState(At<void*>(stage, 0xC), 6, 0, 0, 0);
    At<uint8_t>(stage, 0x5A) = 0;
    ErrorShader_SetStageState(At<void*>(stage, 0xC), D3DTSS_TEXTURETRANSFORMFLAGS, 0, 0);
    ErrorShader_PassSetStage(pass, At<uint32_t>(pass, 0x14), stage);
    ErrorShader_PassArraySetAt(self + 0x44, 0, &pass);
    At<uint32_t>(self, 0x3C) = 1;
    // the locals' references let go (plain counts, as the library inlines them)
    if (stage != nullptr && --At<int32_t>(stage, 0x5C) == 0) {
        ErrorShader_ReturnStageToPool(stage);
    }
    if (pass != nullptr && --At<int32_t>(pass, 0x60) == 0) {
        ErrorShader_ReturnPassToPool(pass);
    }
    return 1;
}

// 0x89A190 (thiscall): the shader declaration (one entry, one stream: the position as a FLOAT3 in
// register 0) made and held at +0x28 (a name guess: CreateShaderDeclaration)
uint8_t __fastcall ErrorShader_CreateDeclaration(uint8_t* self, re::Edx) {
    NiRefObject* declaration = static_cast<NiRefObject*>(ErrorShader_DeclarationCreate(At<void*>(self, 0x18), 1, 1));
    re::Ptr32<NiRefObject>& slot = At<NiRefObject*>(self, 0x28);
    if (slot != declaration) {
        // the old let go before the new is held (NiPointer's order here)
        Release(slot);
        slot = declaration;
        AddRef(declaration);
    }
    // NiShaderDeclaration::SetEntry (slot 18): entry 0, input 0 (position), type 2 (FLOAT3), stream 0
    Virtual<uint8_t>(slot, 18, 0u, 0u, 2u, 0u);
    return 1;
}

// 0x89A200 (thiscall, slot 0): NiDX9ErrorShader's deleting destructor (the pass array emptied,
// NiD3DShader's destructor)
void* __fastcall ErrorShader_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kErrorShaderVtable;
    ErrorShader_PassArrayRemoveAll(self + 0x44);
    ErrorShader_D3DShaderDestruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, kErrorShaderSize);
    }
    return self;
}

// 0x89A230 (thiscall, slot 4): NiDX9ErrorShader::Initialize (once: NiD3DShader's, then the
// declaration; false when NiD3DShader's fails)
uint8_t __fastcall ErrorShader_Initialize(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 0x10) == 0) {
        if (ErrorShader_D3DShaderInitialize(self) == 0) {
            return 0;
        }
        ErrorShader_CreateDeclaration(self, nullptr);
    }
    return 1;
}

// 0x89A260 (thiscall): NiDX9ErrorShader::NiDX9ErrorShader (the pass built, named
// "NiDX9ErrorShader", +0x20 1)
uint8_t* __fastcall ErrorShader_Construct(uint8_t* self, re::Edx) {
    ErrorShader_D3DShaderConstruct(self);
    At<uint32_t>(self, 0) = kErrorShaderVtable;
    ErrorShader_CreatePass(self, nullptr);
    re::Ptr32<char> name = ni::sys::GlobalStringTable_AddString(kErrorShaderName);
    ErrorShader_SetName(self, &name);
    ni::sys::FixedStringRelease(&name);
    At<uint8_t>(self, 0x20) = 1;
    return self;
}

#define C40N_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C40N_REVERSED("NiDX9ErrorShader::UpdatePipeline", 0x00899D80, ErrorShader_UpdatePipeline);
C40N_REVERSED("NiDX9ErrorShader::SetupTransformations", 0x00899F50, ErrorShader_SetupTransformations);
C40N_REVERSED("NiDX9ErrorShader::GetRTTI", 0x0089A040, ErrorShader_GetRTTI);
C40N_REVERSED("NiDX9ErrorShader::CreatePass", 0x0089A050, ErrorShader_CreatePass);
C40N_REVERSED("NiDX9ErrorShader::CreateDeclaration", 0x0089A190, ErrorShader_CreateDeclaration);
C40N_REVERSED("NiDX9ErrorShader::DeletingDestructor", 0x0089A200, ErrorShader_DeletingDestructor);
C40N_REVERSED("NiDX9ErrorShader::Initialize", 0x0089A230, ErrorShader_Initialize);
C40N_REVERSED("NiDX9ErrorShader::NiDX9ErrorShader", 0x0089A260, ErrorShader_Construct);

} // namespace ni::c40
