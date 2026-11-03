// NiD3DTextureStage (0x60 bytes, no vtable) and its NiD3DTextureStageGroup (0x218): one texture
// stage of a pass, its texture, its texture transform and its stage and sampler states. Both come
// from pools (NiTObjectPool, as NiD3DRenderStateGroup's: +0x0 the free set, +0xC the blocks, +0x10
// the next block's size; stages at 0x20E73CC under the lock at 0x20E7410, groups at 0x20E7434).
//
// A stage (the constructor 0x89E590): +0x0 its stage index, +0x4 its texture, +0x8 its map flags,
// +0xC its state group (from the pool), +0x10 the texture transform's kind (the top nibble: 0 its
// own matrix, 1 a global constant's, 2 its own matrix; the rest the transform type for the
// matrix to be computed from, 0 for the matrix as it is), +0x14 the global constant
// (NiGlobalConstantEntry: its name +0x8, its data +0x14), +0x18 the matrix (D3DMATRIX), +0x58 its
// effect flags, +0x5A whether it has a texture transform, +0x5B from the pool, +0x5C its reference
// count (plain).
//
// A group: +0x0 from the pool; the 33 stage states (D3DTEXTURESTAGESTATETYPE 0..32) twice: +0x4
// the count, +0xC the values, +0x90 the flags of those saved, +0xB4/+0xBC/+0x140 of those set
// plainly; the 14 sampler states (Gamebryo's index, D3DSAMPLERSTATETYPE by the table at 0xBCB748)
// twice: +0x164/+0x16C/+0x1A4 saved, +0x1B4/+0x1BC/+0x1F4 plain; +0x204 the count and +0x208 the
// flags of the sampler states that take the map's value.
//
// The statics: 0x20E73C4 the renderer, 0x20E73C0 its device (held), 0x20E73C8 its render state
// (NiDX9RenderState: +0x920 the texture stage states (33 a stage, 8 bytes each), +0x19A0 the
// sampler states (14 a sampler), +0x20A0 the textures, +0x2238 the device); the groups apply
// through the render state at 0x20E7430 (NiD3DShaderInterface's).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::c41 {
using namespace ni::dx9;
namespace {

constexpr uint32_t kStageSize = 0x60;
constexpr uint32_t kStageGroupSize = 0x218;
constexpr uintptr_t kStageConstructor = 0x0089E590;
constexpr uintptr_t kStageGroupConstructor = 0x0089F8A0;
constexpr int kDevSetTransform = 44;
constexpr int kDevSetTexture = 65;
constexpr int kDevSetTextureStageState = 67;
constexpr int kDevSetSamplerState = 69;

GAME_VAR(uint8_t*, ms_pkStageRenderer, 0x020E73C4);
GAME_VAR(void*, ms_pkStageDevice, 0x020E73C0);
GAME_VAR(uint8_t*, ms_pkStageRenderState, 0x020E73C8);
GAME_VAR(uint8_t*, ms_pkStagePool, 0x020E73CC);
GAME_VAR(NiCriticalSection, ms_kStagePoolLock, 0x020E7410);
GAME_VAR(uint8_t*, ms_pkStageGroupPool, 0x020E7434);
GAME_VAR(uint8_t*, ms_pkGroupRenderState, 0x020E7430);
GAME_VAR(uint32_t, ms_uiMaxTextureBlendStages, 0x00BCB7F8);
GAME_VAR(uint32_t, ms_uiMaxSamplers, 0x00BCB800);
GAME_ARRAY(uint32_t, ms_auiSamplerStates, 0x00BCB748);      // D3DSAMPLERSTATETYPE by index (14)
GAME_ARRAY(uint32_t, ms_auiClampModes, 0x020E6CE0);         // ADDRESSU, ADDRESSV by clamp mode
GAME_ARRAY(uint32_t, ms_auiFilterModes, 0x020E6C98);        // MIN?, MAG?, MIP by filter mode
GAME_ARRAY(float, ms_afTextureTransform, 0x020E73D0);       // the matrix ComputeTextureTransform makes

GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __stdcall, VectorConstructor, 0x00401240, void* p, uint32_t size, uint32_t count, uintptr_t ctor);
GAME_FN(void, __thiscall, Set_Realloc, 0x0083AD20, void* set, uint32_t allocated);
GAME_FN(void, __thiscall, Set_AddUnique, 0x00884380, void* set, const re::Ptr32<void>* element);
GAME_FN(uint8_t, __cdecl, ShaderFactory_ReleaseGlobalShaderConstant, 0x00756B40, const re::Ptr32<char>* key);
GAME_FN(void, __thiscall, RenderState_SetTextureStageState, 0x0074D400, void* self, uint32_t stage, uint32_t state,
        uint32_t value);

// this file's (called where modes apply)
GAME_FN(uint8_t*, __cdecl, StageGroup_GetFree, 0x008A0020);
GAME_FN(void, __cdecl, StageGroup_Release, 0x008A0060, uint8_t* group);
GAME_FN(void, __thiscall, StageGroup_SetSamplerState, 0x0089FB10, void* self, uint32_t index, uint32_t value,
        int save, int useMapValue);
GAME_FN(uint8_t, __thiscall, StageGroup_GetStageState, 0x0089FAB0, void* self, uint32_t state, uint32_t* value,
        uint8_t* saved);
GAME_FN(uint32_t, __thiscall, StageGroup_SetStageStates, 0x0089FCE0, void* self, uint32_t stage);
GAME_FN(uint32_t, __thiscall, StageGroup_SetSamplerStates, 0x0089FDD0, void* self, uint32_t stage);
GAME_FN(uint32_t, __thiscall, StageGroup_SetTextureStage, 0x0089FE70, void* self, uint32_t stage);
GAME_FN(void, __thiscall, StageGroup_RemoveAllStageStates, 0x0089FC50, void* self);
GAME_FN(void, __thiscall, StageGroup_RemoveAllSamplerStates, 0x0089FC80, void* self);
GAME_FN(uint8_t*, __cdecl, StageGroup_NewArray, 0x0089FD80, uint32_t count);
GAME_FN(void, __thiscall, StageGroupPool_CreateNewObjects, 0x0089FEA0, void* pool, uint32_t count);
GAME_FN(void, __thiscall, StageGroupBlock_Destruct, 0x0089FF50, void* self);
GAME_FN(void, __thiscall, StageGroupBlock_DeletingDestructor, 0x0089FCC0, void* self, uint32_t flags);
GAME_FN(void, __thiscall, StageGroupPool_Destruct, 0x0089FF90, void* self);
GAME_FN(void, __thiscall, RenderState_SetSamplerState, 0x0089F090, void* self, uint32_t sampler, uint32_t index,
        uint32_t value);
GAME_FN(uint8_t, __thiscall, Stage_ComputeTextureTransform, 0x0089E830, void* self, uint32_t type,
        const float* matrix);
GAME_FN(void, __thiscall, Stage_ApplyNonPow2Restrictions, 0x0089F1D0, void* self, int nonPow2);
GAME_FN(void, __thiscall, Stage_ApplyTextureTransform, 0x0089F2C0, void* self);
GAME_FN(void, __thiscall, Stage_ApplyTexture, 0x0089F370, void* self);
GAME_FN(void, __thiscall, Stage_Destruct, 0x0089F150, void* self);
GAME_FN(uint8_t*, __cdecl, Stage_NewArray, 0x0089F040, uint32_t count);
GAME_FN(void, __cdecl, Stage_DeleteArray, 0x0089F590, uint8_t* array);
GAME_FN(void, __thiscall, StagePool_CreateNewObjects, 0x0089F4E0, void* pool, uint32_t count);
GAME_FN(void, __thiscall, StageBlock_Destruct, 0x0089F5D0, void* self);
GAME_FN(void, __thiscall, StageBlock_DeletingDestructor, 0x0089E5C0, void* self, uint32_t flags);

// the byte count of `count` objects of `size` with NiNew[]'s count before them (0xFFFFFFFF on
// overflow, as the compiler's checked multiply)
inline uint32_t CheckedArraySize(uint32_t count, uint32_t size) {
    uint64_t bytes = uint64_t(count) * size;
    uint32_t total = (bytes >> 32) != 0 ? 0xFFFFFFFFu : uint32_t(bytes);
    return total > 0xFFFFFFFBu ? 0xFFFFFFFFu : total + 4;
}

// an object put in a pool's free set (NiTObjectSet::Add, inlined)
inline void SetAdd(uint8_t* set, void* object) {
    uint32_t allocated = At<uint32_t>(set, 4);
    if (At<uint32_t>(set, 8) == allocated) {
        Set_Realloc(set, allocated != 0 ? allocated * 2 : 1);
    }
    At<re::Ptr32<void>*>(set, 0)[At<uint32_t>(set, 8)] = object;
    At<uint32_t>(set, 8)++;
}

// the first free object taken (the last moved into its place)
inline uint8_t* TakeFree(uint8_t* pool) {
    re::Ptr32<void>* array = At<re::Ptr32<void>*>(pool, 0);
    uint8_t* object = static_cast<uint8_t*>(array[0]);
    At<uint32_t>(pool, 8)--;
    array[0] = array[At<uint32_t>(pool, 8)];
    return object;
}

// the stage's global constant given back (its name held across NiShaderFactory's release)
inline void ReleaseGlobalConstant(uint8_t* entry) {
    re::Ptr32<char> name = ni::sys::FixedStringAddRef(At<char*>(entry, 8));
    ShaderFactory_ReleaseGlobalShaderConstant(&name);
    ni::sys::FixedStringRelease(&name);
}

inline uint32_t& StageStateCache(uint8_t* renderState, uint32_t stage, uint32_t state) {
    return At<uint32_t>(renderState, 0x920 + int(stage * 0x21 + state) * 8);
}
inline uint32_t& SamplerStateCache(uint8_t* renderState, uint32_t sampler, uint32_t state) {
    return At<uint32_t>(renderState, 0x19A0 + int(state + sampler * 0xE) * 8);
}

} // namespace

// 0x89E590 (thiscall): NiD3DTextureStage::NiD3DTextureStage (its state group from the pool)
void __fastcall Stage_Construct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = 0;
    At<void*>(self, 4) = nullptr;
    At<uint32_t>(self, 8) = 0;
    At<uint32_t>(self, 0x10) = 0;
    At<void*>(self, 0x14) = nullptr;
    At<uint16_t>(self, 0x58) = 0;
    At<uint8_t>(self, 0x5A) = 0;
    At<uint8_t>(self, 0x5B) = 0;
    At<uint32_t>(self, 0x5C) = 0;
    At<uint8_t*>(self, 0xC) = StageGroup_GetFree();
}

// 0x89E5C0 (thiscall): a stage block's deleting destructor
void* __fastcall StageBlock_DeletingDestructorImpl(uint8_t* self, re::Edx, uint32_t flags) {
    StageBlock_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0xC);
    }
    return self;
}

// 0x89E5E0 (thiscall): NiD3DTextureStage::SetClampMode (the U and V addressing by the mode's table)
void __fastcall Stage_SetClampMode(uint8_t* self, re::Edx, uint32_t mode) {
    StageGroup_SetSamplerState(At<void*>(self, 0xC), 0, ms_auiClampModes[mode * 2], 0, 0);
    StageGroup_SetSamplerState(At<void*>(self, 0xC), 1, ms_auiClampModes[mode * 2 + 1], 0, 0);
}

// 0x89E640 (thiscall): NiD3DTextureStage::SetFilterMode (the three filters by the mode's table)
void __fastcall Stage_SetFilterMode(uint8_t* self, re::Edx, uint32_t mode) {
    StageGroup_SetSamplerState(At<void*>(self, 0xC), 4, ms_auiFilterModes[mode * 3 + 1], 0, 0);
    StageGroup_SetSamplerState(At<void*>(self, 0xC), 5, ms_auiFilterModes[mode * 3], 0, 0);
    StageGroup_SetSamplerState(At<void*>(self, 0xC), 6, ms_auiFilterModes[mode * 3 + 2], 0, 0);
}

// 0x89E6E0: NiD3DTextureStage::DisableStage (from `stage` on: no texture; the blend stages' colour
// and alpha disabled, their texture transforms off; past them, up to the samplers, no texture)
void __cdecl Stage_DisableStage(uint32_t stage) {
    uint8_t* renderState = ms_pkStageRenderState;
    for (; stage < ms_uiMaxTextureBlendStages; stage++) {
        if (At<void*>(renderState, 0x20A0 + int(stage) * 4) != nullptr) {
            At<void*>(renderState, 0x20A0 + int(stage) * 4) = nullptr;
            Com<long>(At<void*>(renderState, 0x2238), kDevSetTexture, stage, static_cast<void*>(nullptr));
            renderState = ms_pkStageRenderState;
        }
        if (StageStateCache(renderState, stage, 1) != 1) {   // COLOROP DISABLE
            Com<long>(At<void*>(renderState, 0x2238), kDevSetTextureStageState, stage, 1u, 1u);
            StageStateCache(renderState, stage, 1) = 1;
            renderState = ms_pkStageRenderState;
        }
        if (StageStateCache(renderState, stage, 4) != 1) {   // ALPHAOP DISABLE
            Com<long>(At<void*>(renderState, 0x2238), kDevSetTextureStageState, stage, 4u, 1u);
            StageStateCache(renderState, stage, 4) = 1;
            renderState = ms_pkStageRenderState;
        }
        if (StageStateCache(renderState, stage, 0x18) != 0) {   // TEXTURETRANSFORMFLAGS
            Com<long>(At<void*>(renderState, 0x2238), kDevSetTextureStageState, stage, 0x18u, 0u);
            StageStateCache(renderState, stage, 0x18) = 0;
            renderState = ms_pkStageRenderState;
        }
    }
    for (; stage < ms_uiMaxSamplers; stage++) {
        if (At<void*>(renderState, 0x20A0 + int(stage) * 4) != nullptr) {
            At<void*>(renderState, 0x20A0 + int(stage) * 4) = nullptr;
            Com<long>(At<void*>(renderState, 0x2238), kDevSetTexture, stage, static_cast<void*>(nullptr));
            renderState = ms_pkStageRenderState;
        }
    }
}

// 0x89E830 (thiscall, `this` unused): the texture transform of a kind made from a matrix into the
// static matrix (0x20E73D0): kinds 1 and 2 the inverse view's 3x3 times the matrix with its row 3
// added under it, kinds 3, 5 and 6 the inverse view's 3x3 times the matrix with the matrix's row
// 3 as is (the column 3 zero), kind 4 a fixed scale-and-offset; false for any other. (Kinds 1/2
// and 3/5/6 differ only in the order the game multiplies in.)
uint8_t __fastcall Stage_ComputeTextureTransformImpl(void*, re::Edx, uint32_t type, const float* m) {
    float* out = ms_afTextureTransform;
    switch (type) {
    case 1:
    case 2:
    case 3:
    case 5:
    case 6: {
        const float* v = &At<float>(ms_pkStageRenderer, kInvView);
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) {
                out[r * 4 + c] = (v[r * 4] * m[c] + v[r * 4 + 1] * m[4 + c]) + v[r * 4 + 2] * m[8 + c];
            }
        }
        for (int c = 0; c < 3; c++) {
            if (type <= 2) {
                out[12 + c] = ((v[12] * m[c] + v[13] * m[4 + c]) + v[14] * m[8 + c]) + m[12 + c];
            } else {
                out[12 + c] = m[12 + c];
            }
        }
        out[3] = 0.0f;
        out[7] = 0.0f;
        out[11] = 0.0f;
        out[15] = 0.0f;
        return 1;
    }
    case 4:
        crt::Memset(out, 0, 0x40);
        out[5] = 0.5f;
        out[8] = 0.5f;
        out[12] = 0.5f;
        out[13] = 0.5f;
        return 1;
    default:
        return 0;
    }
}

// 0x89F040: an array of `count` stages (NiNew[] with its count before it)
uint8_t* __cdecl Stage_NewArrayImpl(uint32_t count) {
    uint32_t* memory = static_cast<uint32_t*>(NiNewArray(CheckedArraySize(count, kStageSize)));
    if (memory == nullptr) {
        return nullptr;
    }
    *memory = count;
    VectorConstructor(memory + 1, kStageSize, count, kStageConstructor);
    return reinterpret_cast<uint8_t*>(memory + 1);
}

// 0x89F090 (thiscall on the NiDX9RenderState): a sampler state set on the device where it differs
// (the index mapped to D3DSAMPLERSTATETYPE)
void __fastcall RenderState_SetSamplerStateImpl(uint8_t* self, re::Edx, uint32_t sampler, uint32_t index,
                                                uint32_t value) {
    uint32_t state = ms_auiSamplerStates[index];
    if (SamplerStateCache(self, sampler, state) != value) {
        Com<long>(At<void*>(self, 0x2238), kDevSetSamplerState, sampler, ms_auiSamplerStates[index], value);
        SamplerStateCache(self, sampler, state) = value;
    }
}

// 0x89F0E0: NiD3DTextureStage::SetD3DRenderer (the renderer, its device held, its render state)
void __cdecl Stage_SetD3DRenderer(uint8_t* renderer) {
    ms_pkStageRenderer = renderer;
    if (renderer != nullptr) {
        void* device = At<void*>(renderer, kDevice);
        if (ms_pkStageDevice != nullptr) {
            Com<long>(ms_pkStageDevice, kRelease);
        }
        ms_pkStageDevice = device;
        if (device != nullptr) {
            Com<long>(device, kAddRef);
        }
        ms_pkStageRenderState = At<uint8_t*>(ms_pkStageRenderer, kRenderState);
        return;
    }
    if (ms_pkStageDevice != nullptr) {
        Com<long>(ms_pkStageDevice, kRelease);
    }
    ms_pkStageDevice = nullptr;
    ms_pkStageRenderState = nullptr;
}

// 0x89F150 (thiscall): NiD3DTextureStage::~NiD3DTextureStage (its global constant given back, its
// group to the pool)
void __fastcall Stage_DestructImpl(uint8_t* self, re::Edx) {
    if (At<uint8_t*>(self, 0x14) != nullptr) {
        ReleaseGlobalConstant(At<uint8_t*>(self, 0x14));
    }
    StageGroup_Release(At<uint8_t*>(self, 0xC));
}

// 0x89F1D0 (thiscall): a texture whose size isn't a power of two, on a device that only takes
// those conditionally: clamped (U and V) and no coordinate wrapping (WRAP0..15 off). (Name a
// guess.)
void __fastcall Stage_ApplyNonPow2RestrictionsImpl(uint8_t* self, re::Edx, int nonPow2) {
    if (uint8_t(nonPow2) == 0 || (Virtual<uint32_t>(ms_pkStageRenderer, 19) & 8) != 0 ||
        (Virtual<uint32_t>(ms_pkStageRenderer, 19) & 4) == 0) {
        return;
    }
    RenderState_SetSamplerState(ms_pkStageRenderState, At<uint32_t>(self, 0), 0, 3);   // CLAMP
    RenderState_SetSamplerState(ms_pkStageRenderState, At<uint32_t>(self, 0), 1, 3);
    uint8_t* renderState = ms_pkStageRenderState;
    for (uint32_t i = 0; i < 8; i++) {
        uint32_t wrap = 0x80 + i;    // D3DRS_WRAP0..7
        uint32_t wrapHigh = 0xC6 + i;   // D3DRS_WRAP8..15
        if (At<uint32_t>(renderState, 0x120 + int(wrap) * 8) != 0) {
            Com<long>(At<void*>(renderState, 0x2238), kDevSetRenderState, wrap, 0u);
            At<uint32_t>(renderState, 0x120 + int(wrap) * 8) = 0;
            renderState = ms_pkStageRenderState;
        }
        if (At<uint32_t>(renderState, 0x120 + int(wrapHigh) * 8) != 0) {
            Com<long>(At<void*>(renderState, 0x2238), kDevSetRenderState, wrapHigh, 0u);
            At<uint32_t>(renderState, 0x120 + int(wrapHigh) * 8) = 0;
            renderState = ms_pkStageRenderState;
        }
    }
}

// 0x89F2C0 (thiscall): the stage's texture transform set on the device (off without one; its own
// or the global constant's matrix, as is or made by ComputeTextureTransform)
void __fastcall Stage_ApplyTextureTransformImpl(uint8_t* self, re::Edx) {
    uint32_t stage = At<uint32_t>(self, 0);
    if (stage >= ms_uiMaxTextureBlendStages) {
        return;
    }
    if (At<uint8_t>(self, 0x5A) == 0) {
        RenderState_SetTextureStageState(ms_pkStageRenderState, stage, 0x18, 0);
        return;
    }
    uint32_t kind = At<uint32_t>(self, 0x10);
    const float* matrix;
    switch (kind & 0xF0000000) {
    case 0:
        matrix = &At<float>(self, 0x18);
        break;
    case 0x10000000:
        matrix = At<const float*>(At<uint8_t*>(self, 0x14), 0x14);
        if ((kind & 0x0FFFFFFF) == 0) {
            break;
        }
        if (Stage_ComputeTextureTransform(self, kind & 0x0FFFFFFF, matrix) == 0) {
            return;
        }
        Com<long>(ms_pkStageDevice, kDevSetTransform, At<uint32_t>(self, 0) + 0x10, ms_afTextureTransform);
        return;
    case 0x20000000:
        matrix = &At<float>(self, 0x18);
        if ((kind & 0x0FFFFFFF) == 0) {
            break;
        }
        if (Stage_ComputeTextureTransform(self, kind & 0x0FFFFFFF, matrix) == 0) {
            return;
        }
        Com<long>(ms_pkStageDevice, kDevSetTransform, At<uint32_t>(self, 0) + 0x10, ms_afTextureTransform);
        return;
    default:
        return;
    }
    Com<long>(ms_pkStageDevice, kDevSetTransform, stage + 0x10, matrix);   // D3DTS_TEXTURE0 + stage
}

// 0x89F370 (thiscall): the stage's texture coordinate index (its own state, else its stage
// index), its texture (made ready by the texture manager) and with a texture: no mipmap level
// limit unless mipmapped, the non-power-of-two restrictions, the texture transform. (Name a
// guess.)
void __fastcall Stage_ApplyTextureImpl(uint8_t* self, re::Edx) {
    if (At<uint32_t>(self, 0) < ms_uiMaxTextureBlendStages) {
        uint32_t value = 0;
        uint8_t saved = 0;
        uint32_t index = value;
        if (StageGroup_GetStageState(At<void*>(self, 0xC), 0xB, &value, &saved) != 0) {
            index = value;
        } else {
            index = At<uint32_t>(self, 0);
        }
        uint32_t stage = At<uint32_t>(self, 0);
        uint8_t* renderState = ms_pkStageRenderState;
        if (StageStateCache(renderState, stage, 0xB) != index) {
            Com<long>(At<void*>(renderState, 0x2238), kDevSetTextureStageState, stage, 0xBu, index);
        }
        StageStateCache(renderState, stage, 0xB) = index;
    }
    void* d3dTexture = nullptr;
    uint8_t changed = 0;
    uint8_t mipmap = 0;     // (the game's stack, set by the texture manager when there's a texture)
    uint32_t nonPow2 = 0;   // (likewise: its low byte written)
    if (At<void*>(self, 4) != nullptr) {
        void* textureManager = At<void*>(ms_pkStageRenderer, kTextureManager);
        d3dTexture = Virtual<void*>(textureManager, 3, At<void*>(self, 4), &changed, &mipmap,
                                    reinterpret_cast<uint8_t*>(&nonPow2));
    }
    uint32_t stage = At<uint32_t>(self, 0);
    uint8_t* renderState = ms_pkStageRenderState;
    if (At<void*>(renderState, 0x20A0 + int(stage) * 4) != d3dTexture) {
        At<void*>(renderState, 0x20A0 + int(stage) * 4) = d3dTexture;
        Com<long>(At<void*>(renderState, 0x2238), kDevSetTexture, stage, d3dTexture);
        renderState = ms_pkStageRenderState;
    }
    if (d3dTexture == nullptr) {
        return;
    }
    if (mipmap == 0) {
        uint32_t state = ms_auiSamplerStates[6];
        uint32_t sampler = At<uint32_t>(self, 0);
        if (SamplerStateCache(renderState, sampler, state) != 0) {
            Com<long>(At<void*>(renderState, 0x2238), kDevSetSamplerState, sampler, state, 0u);
            SamplerStateCache(renderState, sampler, state) = 0;
        }
    }
    Stage_ApplyNonPow2Restrictions(self, int(nonPow2));
    Stage_ApplyTextureTransform(self);
}

// 0x89F4B0 (thiscall): NiD3DTextureStage::ApplyTextureStage (its states and samplers set; when
// that fails false, else the texture applied)
uint8_t __fastcall Stage_ApplyTextureStage(uint8_t* self, re::Edx) {
    if (StageGroup_SetTextureStage(At<void*>(self, 0xC), At<uint32_t>(self, 0)) != 0) {
        return 0;
    }
    Stage_ApplyTexture(self);
    return 1;
}

// 0x89F4E0 (thiscall): the stage pool's next block of `count` made, its stages made free
void __fastcall StagePool_CreateNewObjectsImpl(uint8_t* pool, re::Edx, uint32_t count) {
    uint8_t* block = static_cast<uint8_t*>(NiNew(0xC));
    if (block != nullptr) {
        At<uint32_t>(block, 4) = count;
        At<void*>(block, 8) = nullptr;
        At<uint8_t*>(block, 0) = count == 0 ? nullptr : Stage_NewArray(count);
    }
    for (uint32_t i = 0; i < count; i++) {
        uint8_t* stage = i < At<uint32_t>(block, 4) ? At<uint8_t*>(block, 0) + i * kStageSize : nullptr;
        SetAdd(pool, stage);
    }
    At<uint8_t*>(block, 8) = At<uint8_t*>(pool, 0xC);
    At<uint8_t*>(pool, 0xC) = block;
}

// 0x89F590: an array of stages deleted (each destroyed, last first)
void __cdecl Stage_DeleteArrayImpl(uint8_t* array) {
    if (array == nullptr) {
        return;
    }
    int32_t count = At<int32_t>(array, -4);
    for (int32_t i = count - 1; i >= 0; i--) {
        Stage_Destruct(array + i * kStageSize);
    }
    NiDeleteArray(array - 4, kStageSize);
}

// 0x89F5D0 (thiscall): a stage block's destructor (its stages and the blocks after it deleted)
void __fastcall StageBlock_DestructImpl(uint8_t* self, re::Edx) {
    Stage_DeleteArray(At<uint8_t*>(self, 0));
    uint8_t* next = At<uint8_t*>(self, 8);
    if (next != nullptr) {
        StageBlock_Destruct(next);
        NiDelete(next, 0xC);
    }
}

// 0x89F600: NiD3DTextureStage::CreateStagePool (8 at a time; under the pool's lock)
void __cdecl Stage_CreateStagePool() {
    Lock(ms_kStagePoolLock);
    uint8_t* pool = static_cast<uint8_t*>(NiNew(0x18));
    if (pool != nullptr) {
        At<void*>(pool, 0) = nullptr;
        At<uint32_t>(pool, 4) = 0;
        At<uint32_t>(pool, 8) = 0;
        At<void*>(pool, 0xC) = nullptr;
        At<uint32_t>(pool, 0x10) = 8;
        At<uint32_t>(pool, 0x14) = 8;
    }
    ms_pkStagePool = pool;
    Unlock(ms_kStagePoolLock);
}

// 0x89F670 (thiscall): NiD3DTextureStage::ReturnStageToPool (one from the pool emptied and given
// back under the lock; another one destroyed and deleted)
void __fastcall Stage_ReturnStageToPool(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 0x5B) == 0) {
        Stage_Destruct(self);
        NiDelete(self, kStageSize);
        return;
    }
    At<void*>(self, 4) = nullptr;
    StageGroup_RemoveAllStageStates(At<void*>(self, 0xC));
    StageGroup_RemoveAllSamplerStates(At<void*>(self, 0xC));
    if (At<uint8_t*>(self, 0x14) != nullptr) {
        ReleaseGlobalConstant(At<uint8_t*>(self, 0x14));
    }
    At<void*>(self, 0x14) = nullptr;
    At<uint32_t>(self, 0) = 0;
    At<uint32_t>(self, 8) = 0;
    At<uint16_t>(self, 0x58) = 0;
    At<uint32_t>(self, 0x10) = 0;
    At<uint8_t>(self, 0x5A) = 0;
    Lock(ms_kStagePoolLock);
    uint8_t* stage = self;
    Set_AddUnique(ms_pkStagePool, re::PtrCast<const re::Ptr32<void>*>(&stage));
    Unlock(ms_kStagePoolLock);
}

// 0x89F770: NiD3DTextureStage::DestroyStagePool (its blocks and stages deleted, under the lock)
void __cdecl Stage_DestroyStagePool() {
    Lock(ms_kStagePoolLock);
    uint8_t* pool = ms_pkStagePool;
    if (pool != nullptr) {
        uint8_t* block = At<uint8_t*>(pool, 0xC);
        At<uint32_t>(pool, 8) = 0;
        if (block != nullptr) {
            Stage_DeleteArray(At<uint8_t*>(block, 0));
            if (At<void*>(block, 8) != nullptr) {
                StageBlock_DeletingDestructor(At<void*>(block, 8), 1);
            }
            NiDelete(block, 0xC);
        }
        NiFree(At<void*>(pool, 0));
        NiDelete(pool, 0x18);
    }
    ms_pkStagePool = nullptr;
    Unlock(ms_kStagePoolLock);
}

// 0x89F810: NiD3DTextureStage::CreateNewStage (one from the pool, more made when it's empty
// (doubling the next block); held through `out`; marked as pooled)
re::Ptr32<uint8_t>* __cdecl Stage_CreateNewStage(re::Ptr32<uint8_t>* out) {
    Lock(ms_kStagePoolLock);
    uint8_t* pool = ms_pkStagePool;
    if (At<uint32_t>(pool, 8) == 0) {
        StagePool_CreateNewObjects(pool, At<uint32_t>(pool, 0x10));
        At<uint32_t>(pool, 0x10) *= 2;
    }
    uint8_t* stage = TakeFree(pool);
    *out = stage;
    if (stage != nullptr) {
        At<uint32_t>(stage, 0x5C)++;
    }
    Unlock(ms_kStagePoolLock);
    At<uint8_t>(*out, 0x5B) = 1;
    return out;
}

// 0x89F8A0 (thiscall): NiD3DTextureStageGroup::NiD3DTextureStageGroup (everything cleared)
void __fastcall StageGroup_Construct(uint8_t* self, re::Edx) {
    At<uint8_t>(self, 0) = 0;
    At<uint32_t>(self, 4) = 0;
    At<uint32_t>(self, 8) = 0;
    At<uint32_t>(self, 0xB4) = 0;
    At<uint32_t>(self, 0xB8) = 0;
    At<uint32_t>(self, 0x164) = 0;
    At<uint32_t>(self, 0x168) = 0;
    At<uint32_t>(self, 0x1B4) = 0;
    At<uint32_t>(self, 0x1B8) = 0;
    At<uint32_t>(self, 0x204) = 0;
    crt::Memset(self + 0xC, 0, 0x84);
    crt::Memset(self + 0x90, 0, 0x21);
    crt::Memset(self + 0xBC, 0, 0x84);
    crt::Memset(self + 0x140, 0, 0x21);
    crt::Memset(self + 0x16C, 0, 0x38);
    crt::Memset(self + 0x1A4, 0, 0xE);
    crt::Memset(self + 0x1BC, 0, 0x38);
    crt::Memset(self + 0x1F4, 0, 0xE);
    crt::Memset(self + 0x208, 0, 0xE);
}

// 0x89F9E0 (thiscall): NiD3DTextureStageGroup::SetStageState (into the saved or plain set; moved
// from the other one when it's there, the saved count then counted twice: the game's)
void __fastcall StageGroup_SetStageStateImpl(uint8_t* self, re::Edx, uint32_t state, uint32_t value, int save) {
    if (state >= 0x21) {
        return;
    }
    if (uint8_t(save) != 0) {
        if (At<uint8_t>(self, 0x90 + int(state)) != 0) {
            At<uint32_t>(self, 0xC + int(state) * 4) = value;
            return;
        }
        if (At<uint8_t>(self, 0x140 + int(state)) != 0) {
            At<uint8_t>(self, 0x140 + int(state)) = 0;
            At<uint8_t>(self, 0x90 + int(state)) = 1;
            At<uint32_t>(self, 0xC + int(state) * 4) = value;
            At<uint32_t>(self, 0xB4)--;
            At<uint32_t>(self, 4)++;
        }
        At<uint8_t>(self, 0x90 + int(state)) = 1;
        At<uint32_t>(self, 0xC + int(state) * 4) = value;
        At<uint32_t>(self, 4)++;
        return;
    }
    if (At<uint8_t>(self, 0x140 + int(state)) != 0) {
        At<uint32_t>(self, 0xBC + int(state) * 4) = value;
        return;
    }
    if (At<uint8_t>(self, 0x90 + int(state)) != 0) {
        At<uint8_t>(self, 0x90 + int(state)) = 0;
        At<uint8_t>(self, 0x140 + int(state)) = 1;
        At<uint32_t>(self, 0xBC + int(state) * 4) = value;
        At<uint32_t>(self, 0xB4)++;
        At<uint32_t>(self, 4)--;
    }
    At<uint8_t>(self, 0x140 + int(state)) = 1;
    At<uint32_t>(self, 0xBC + int(state) * 4) = value;
    At<uint32_t>(self, 0xB4)++;
}

// 0x89FAB0 (thiscall): NiD3DTextureStageGroup::GetStageState (the plain one first; false when
// it's in neither)
uint8_t __fastcall StageGroup_GetStageStateImpl(uint8_t* self, re::Edx, uint32_t state, uint32_t* value,
                                                uint8_t* saved) {
    if (state < 0x21) {
        if (At<uint8_t>(self, 0x140 + int(state)) != 0) {
            *value = At<uint32_t>(self, 0xBC + int(state) * 4);
            *saved = 0;
            return 1;
        }
        if (At<uint8_t>(self, 0x90 + int(state)) != 0) {
            *value = At<uint32_t>(self, 0xC + int(state) * 4);
            *saved = 1;
            return 1;
        }
    }
    return 0;
}

// 0x89FB10 (thiscall): NiD3DTextureStageGroup::SetSamplerState (into the saved or plain set,
// moved from the other one; whether it takes the map's value kept and counted)
void __fastcall StageGroup_SetSamplerStateImpl(uint8_t* self, re::Edx, uint32_t index, uint32_t value, int save,
                                               int useMapValue) {
    if (index >= 0xE) {
        return;
    }
    uint8_t* useMap = self + 0x208 + index;
    uint8_t mapValue = uint8_t(useMapValue);
    if (uint8_t(save) == 0) {
        if (At<uint8_t>(self, 0x1F4 + int(index)) != 0) {
            At<uint32_t>(self, 0x1BC + int(index) * 4) = value;
            goto update;
        }
        if (At<uint8_t>(self, 0x1A4 + int(index)) != 0) {
            At<uint8_t>(self, 0x1A4 + int(index)) = 0;
            At<uint32_t>(self, 0x164)--;
            if (*useMap != 0) {
                At<uint32_t>(self, 0x204)--;
            }
            *useMap = 0;
        }
        At<uint8_t>(self, 0x1F4 + int(index)) = 1;
        At<uint32_t>(self, 0x1BC + int(index) * 4) = value;
        At<uint32_t>(self, 0x1B4)++;
    } else {
        if (At<uint8_t>(self, 0x1A4 + int(index)) != 0) {
            At<uint32_t>(self, 0x16C + int(index) * 4) = value;
            goto update;
        }
        if (At<uint8_t>(self, 0x1F4 + int(index)) != 0) {
            At<uint8_t>(self, 0x1F4 + int(index)) = 0;
            At<uint32_t>(self, 0x1B4)--;
            if (*useMap != 0) {
                At<uint32_t>(self, 0x204)--;
            }
            *useMap = 0;
        }
        At<uint8_t>(self, 0x1A4 + int(index)) = 1;
        At<uint32_t>(self, 0x16C + int(index) * 4) = value;
        At<uint32_t>(self, 0x164)++;
    }
    if (mapValue != 0) {
        At<uint32_t>(self, 0x204)++;
    }
    *useMap = mapValue;
    return;
update:
    if (mapValue == 0) {
        if (*useMap != 0) {
            At<uint32_t>(self, 0x204)--;
        }
    } else if (*useMap == 0) {
        At<uint32_t>(self, 0x204)++;
    }
    *useMap = mapValue;
}

// 0x89FC50 (thiscall): NiD3DTextureStageGroup::RemoveAllStageStates
void __fastcall StageGroup_RemoveAllStageStatesImpl(uint8_t* self, re::Edx) {
    for (int i = 0; i < 0x21; i++) {
        At<uint8_t>(self, 0x90 + i) = 0;
        At<uint8_t>(self, 0x140 + i) = 0;
    }
    At<uint32_t>(self, 0xB4) = 0;
    At<uint32_t>(self, 4) = 0;
}

// 0x89FC80 (thiscall): NiD3DTextureStageGroup::RemoveAllSamplerStates
void __fastcall StageGroup_RemoveAllSamplerStatesImpl(uint8_t* self, re::Edx) {
    for (int i = 0; i < 0xE; i++) {
        At<uint8_t>(self, 0x1A4 + i) = 0;
        At<uint8_t>(self, 0x1F4 + i) = 0;
        At<uint8_t>(self, 0x208 + i) = 0;
    }
    At<uint32_t>(self, 0x1B4) = 0;
    At<uint32_t>(self, 0x164) = 0;
    At<uint32_t>(self, 0x204) = 0;
}

// 0x89FCC0 (thiscall): a stage group block's deleting destructor
void* __fastcall StageGroupBlock_DeletingDestructorImpl(uint8_t* self, re::Edx, uint32_t flags) {
    StageGroupBlock_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0xC);
    }
    return self;
}

// 0x89FCE0 (thiscall): NiD3DTextureStageGroup::SetStageStates (each state it has (the plain one
// first) set on the device where it differs; 0)
uint32_t __fastcall StageGroup_SetStageStatesImpl(uint8_t* self, re::Edx, uint32_t stage) {
    uint8_t* renderState = ms_pkGroupRenderState;
    for (uint32_t i = 0; i < 0x21; i++) {
        uint32_t value;
        if (At<uint8_t>(self, 0x140 + int(i)) != 0) {
            value = At<uint32_t>(self, 0xBC + int(i) * 4);
        } else if (At<uint8_t>(self, 0x90 + int(i)) != 0) {
            value = At<uint32_t>(self, 0xC + int(i) * 4);
        } else {
            continue;
        }
        if (StageStateCache(renderState, stage, i) != value) {
            Com<long>(At<void*>(renderState, 0x2238), kDevSetTextureStageState, stage, i, value);
            StageStateCache(renderState, stage, i) = value;
            renderState = ms_pkGroupRenderState;
        }
    }
    return 0;
}

// 0x89FD80: an array of `count` stage groups (NiNew[] with its count before it)
uint8_t* __cdecl StageGroup_NewArrayImpl(uint32_t count) {
    uint32_t* memory = static_cast<uint32_t*>(NiNewArray(CheckedArraySize(count, kStageGroupSize)));
    if (memory == nullptr) {
        return nullptr;
    }
    *memory = count;
    VectorConstructor(memory + 1, kStageGroupSize, count, kStageGroupConstructor);
    return reinterpret_cast<uint8_t*>(memory + 1);
}

// 0x89FDD0 (thiscall): NiD3DTextureStageGroup::SetSamplerStates (each sampler state it has (the
// plain one first) set on the device where it differs; 0)
uint32_t __fastcall StageGroup_SetSamplerStatesImpl(uint8_t* self, re::Edx, uint32_t stage) {
    uint8_t* renderState = ms_pkGroupRenderState;
    for (uint32_t i = 0; i < 0xE; i++) {
        uint32_t value;
        if (At<uint8_t>(self, 0x1F4 + int(i)) != 0) {
            value = At<uint32_t>(self, 0x1BC + int(i) * 4);
        } else if (At<uint8_t>(self, 0x1A4 + int(i)) != 0) {
            value = At<uint32_t>(self, 0x16C + int(i) * 4);
        } else {
            continue;
        }
        uint32_t state = ms_auiSamplerStates[i];
        if (SamplerStateCache(renderState, stage, state) != value) {
            Com<long>(At<void*>(renderState, 0x2238), kDevSetSamplerState, stage, ms_auiSamplerStates[i], value);
            SamplerStateCache(renderState, stage, state) = value;
            renderState = ms_pkGroupRenderState;
        }
    }
    return 0;
}

// 0x89FE70 (thiscall): NiD3DTextureStageGroup::SetTextureStage (the stage states (for a blend
// stage) and the sampler states; the first one's result when it's not 0)
uint32_t __fastcall StageGroup_SetTextureStageImpl(uint8_t* self, re::Edx, uint32_t stage) {
    uint32_t result = 0;
    if (stage < ms_uiMaxTextureBlendStages) {
        result = StageGroup_SetStageStates(self, stage);
    }
    uint32_t samplers = StageGroup_SetSamplerStates(self, stage);
    return result != 0 ? result : samplers;
}

// 0x89FEA0 (thiscall): the stage group pool's next block of `count` made, its groups made free
void __fastcall StageGroupPool_CreateNewObjectsImpl(uint8_t* pool, re::Edx, uint32_t count) {
    uint8_t* block = static_cast<uint8_t*>(NiNew(0xC));
    if (block != nullptr) {
        At<uint32_t>(block, 4) = count;
        At<void*>(block, 8) = nullptr;
        At<uint8_t*>(block, 0) = count == 0 ? nullptr : StageGroup_NewArray(count);
    }
    for (uint32_t i = 0; i < count; i++) {
        uint8_t* group = i < At<uint32_t>(block, 4) ? At<uint8_t*>(block, 0) + i * kStageGroupSize : nullptr;
        SetAdd(pool, group);
    }
    At<uint8_t*>(block, 8) = At<uint8_t*>(pool, 0xC);
    At<uint8_t*>(pool, 0xC) = block;
}

// 0x89FF50 (thiscall): a stage group block's destructor (its array and the blocks after it
// deleted)
void __fastcall StageGroupBlock_DestructImpl(uint8_t* self, re::Edx) {
    if (At<uint8_t*>(self, 0) != nullptr) {
        NiDeleteArray(At<uint8_t*>(self, 0) - 4, kStageGroupSize);
    }
    uint8_t* next = At<uint8_t*>(self, 8);
    if (next != nullptr) {
        StageGroupBlock_Destruct(next);
        NiDelete(next, 0xC);
    }
}

// 0x89FF90 (thiscall): the stage group pool's destructor (its blocks deleted, its set freed)
void __fastcall StageGroupPool_DestructImpl(uint8_t* self, re::Edx) {
    uint8_t* block = At<uint8_t*>(self, 0xC);
    At<uint32_t>(self, 8) = 0;
    if (block != nullptr) {
        if (At<uint8_t*>(block, 0) != nullptr) {
            NiDeleteArray(At<uint8_t*>(block, 0) - 4, kStageGroupSize);
        }
        if (At<void*>(block, 8) != nullptr) {
            StageGroupBlock_DeletingDestructor(At<void*>(block, 8), 1);
        }
        NiDelete(block, 0xC);
    }
    NiFree(At<void*>(self, 0));
}

// 0x89FFE0: NiD3DTextureStageGroup::CreatePool (8 at a time)
void __cdecl StageGroup_CreatePool() {
    uint8_t* pool = static_cast<uint8_t*>(NiNew(0x18));
    if (pool != nullptr) {
        At<void*>(pool, 0) = nullptr;
        At<uint32_t>(pool, 4) = 0;
        At<uint32_t>(pool, 8) = 0;
        At<void*>(pool, 0xC) = nullptr;
        At<uint32_t>(pool, 0x10) = 8;
        At<uint32_t>(pool, 0x14) = 8;
        ms_pkStageGroupPool = pool;
        return;
    }
    ms_pkStageGroupPool = nullptr;
}

// 0x8A0020: NiD3DTextureStageGroup::GetFreeStageGroup (from the pool, more made when it's empty;
// marked as pooled)
uint8_t* __cdecl StageGroup_GetFreeImpl() {
    uint8_t* pool = ms_pkStageGroupPool;
    if (At<uint32_t>(pool, 8) == 0) {
        StageGroupPool_CreateNewObjects(pool, At<uint32_t>(pool, 0x10));
        At<uint32_t>(pool, 0x10) *= 2;
    }
    uint8_t* group = TakeFree(pool);
    if (At<uint8_t>(group, 0) == 0) {
        At<uint8_t>(group, 0) = 1;
    }
    return group;
}

// 0x8A0060: NiD3DTextureStageGroup::ReleaseStageGroup (a pooled one cleared and given back; any
// other left alone)
void __cdecl StageGroup_ReleaseImpl(uint8_t* group) {
    if (At<uint8_t>(group, 0) == 0) {
        return;
    }
    crt::Memset(group + 0xC, 0, 0x84);
    crt::Memset(group + 0x90, 0, 0x21);
    crt::Memset(group + 0xBC, 0, 0x84);
    crt::Memset(group + 0x140, 0, 0x21);
    crt::Memset(group + 0x16C, 0, 0x38);
    crt::Memset(group + 0x1A4, 0, 0xE);
    crt::Memset(group + 0x1BC, 0, 0x38);
    crt::Memset(group + 0x1F4, 0, 0xE);
    crt::Memset(group + 0x208, 0, 0xE);
    At<uint32_t>(group, 0xB4) = 0;
    At<uint32_t>(group, 4) = 0;
    At<uint32_t>(group, 0x1B4) = 0;
    At<uint32_t>(group, 0x164) = 0;
    At<uint32_t>(group, 0x204) = 0;
    Set_AddUnique(ms_pkStageGroupPool, re::PtrCast<const re::Ptr32<void>*>(&group));
}

// 0x8A01B0: NiD3DTextureStageGroup::DestroyPool (the pointer left as it was)
void __cdecl StageGroup_DestroyPool() {
    uint8_t* pool = ms_pkStageGroupPool;
    if (pool != nullptr) {
        StageGroupPool_Destruct(pool);
        NiDelete(pool, 0x18);
    }
}

#define C41E_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C41E_REVERSED("NiD3DTextureStage::NiD3DTextureStage", 0x0089E590, Stage_Construct);
C41E_REVERSED("NiD3DTextureStage::Block::DeletingDestructor", 0x0089E5C0, StageBlock_DeletingDestructorImpl);
C41E_REVERSED("NiD3DTextureStage::SetClampMode", 0x0089E5E0, Stage_SetClampMode);
C41E_REVERSED("NiD3DTextureStage::SetFilterMode", 0x0089E640, Stage_SetFilterMode);
C41E_REVERSED("NiD3DTextureStage::DisableStage", 0x0089E6E0, Stage_DisableStage);
C41E_REVERSED("NiD3DTextureStage::ComputeTextureTransform", 0x0089E830, Stage_ComputeTextureTransformImpl);
C41E_REVERSED("NiD3DTextureStage::NewArray", 0x0089F040, Stage_NewArrayImpl);
C41E_REVERSED("NiDX9RenderState::SetSamplerState", 0x0089F090, RenderState_SetSamplerStateImpl);
C41E_REVERSED("NiD3DTextureStage::SetD3DRenderer", 0x0089F0E0, Stage_SetD3DRenderer);
C41E_REVERSED("NiD3DTextureStage::~NiD3DTextureStage", 0x0089F150, Stage_DestructImpl);
C41E_REVERSED("NiD3DTextureStage::ApplyNonPow2Restrictions", 0x0089F1D0, Stage_ApplyNonPow2RestrictionsImpl);
C41E_REVERSED("NiD3DTextureStage::ApplyTextureTransform", 0x0089F2C0, Stage_ApplyTextureTransformImpl);
C41E_REVERSED("NiD3DTextureStage::ApplyTexture", 0x0089F370, Stage_ApplyTextureImpl);
C41E_REVERSED("NiD3DTextureStage::ApplyTextureStage", 0x0089F4B0, Stage_ApplyTextureStage);
C41E_REVERSED("NiD3DTextureStage::Pool::CreateNewObjects", 0x0089F4E0, StagePool_CreateNewObjectsImpl);
C41E_REVERSED("NiD3DTextureStage::DeleteArray", 0x0089F590, Stage_DeleteArrayImpl);
C41E_REVERSED("NiD3DTextureStage::Block::~Block", 0x0089F5D0, StageBlock_DestructImpl);
C41E_REVERSED("NiD3DTextureStage::CreateStagePool", 0x0089F600, Stage_CreateStagePool);
C41E_REVERSED("NiD3DTextureStage::ReturnStageToPool", 0x0089F670, Stage_ReturnStageToPool);
C41E_REVERSED("NiD3DTextureStage::DestroyStagePool", 0x0089F770, Stage_DestroyStagePool);
C41E_REVERSED("NiD3DTextureStage::CreateNewStage", 0x0089F810, Stage_CreateNewStage);
C41E_REVERSED("NiD3DTextureStageGroup::NiD3DTextureStageGroup", 0x0089F8A0, StageGroup_Construct);
C41E_REVERSED("NiD3DTextureStageGroup::SetStageState", 0x0089F9E0, StageGroup_SetStageStateImpl);
C41E_REVERSED("NiD3DTextureStageGroup::GetStageState", 0x0089FAB0, StageGroup_GetStageStateImpl);
C41E_REVERSED("NiD3DTextureStageGroup::SetSamplerState", 0x0089FB10, StageGroup_SetSamplerStateImpl);
C41E_REVERSED("NiD3DTextureStageGroup::RemoveAllStageStates", 0x0089FC50, StageGroup_RemoveAllStageStatesImpl);
C41E_REVERSED("NiD3DTextureStageGroup::RemoveAllSamplerStates", 0x0089FC80, StageGroup_RemoveAllSamplerStatesImpl);
C41E_REVERSED("NiD3DTextureStageGroup::Block::DeletingDestructor", 0x0089FCC0,
              StageGroupBlock_DeletingDestructorImpl);
C41E_REVERSED("NiD3DTextureStageGroup::SetStageStates", 0x0089FCE0, StageGroup_SetStageStatesImpl);
C41E_REVERSED("NiD3DTextureStageGroup::NewArray", 0x0089FD80, StageGroup_NewArrayImpl);
C41E_REVERSED("NiD3DTextureStageGroup::SetSamplerStates", 0x0089FDD0, StageGroup_SetSamplerStatesImpl);
C41E_REVERSED("NiD3DTextureStageGroup::SetTextureStage", 0x0089FE70, StageGroup_SetTextureStageImpl);
C41E_REVERSED("NiD3DTextureStageGroup::Pool::CreateNewObjects", 0x0089FEA0, StageGroupPool_CreateNewObjectsImpl);
C41E_REVERSED("NiD3DTextureStageGroup::Block::~Block", 0x0089FF50, StageGroupBlock_DestructImpl);
C41E_REVERSED("NiD3DTextureStageGroup::Pool::~Pool", 0x0089FF90, StageGroupPool_DestructImpl);
C41E_REVERSED("NiD3DTextureStageGroup::CreatePool", 0x0089FFE0, StageGroup_CreatePool);
C41E_REVERSED("NiD3DTextureStageGroup::GetFreeStageGroup", 0x008A0020, StageGroup_GetFreeImpl);
C41E_REVERSED("NiD3DTextureStageGroup::ReleaseStageGroup", 0x008A0060, StageGroup_ReleaseImpl);
C41E_REVERSED("NiD3DTextureStageGroup::DestroyPool", 0x008A01B0, StageGroup_DestroyPool);

} // namespace ni::c41
