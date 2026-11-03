// NiD3DPass (vtable 0x99BDEC: 0 the vector deleting destructor, 1 SetupShaderPrograms, 2
// SetupRenderingPass, 3 PostProcessRenderingPass; 0x64 bytes): one pass of an NiD3DShader, its
// texture stages, render states and shader programs; passes come from a pool (NiTObjectPool, at
// 0x20E6D80 under the lock at 0x20E6D84) and go back to it. The array of its stages is an
// NiTObjectArray<NiD3DTextureStagePtr> (vtable 0x99BDFC; a stage counts its references at +0x5C
// plainly and goes back to its own pool, 0x89F670, at zero).
//
// Layout (from the constructor 0x8845D0): +0x4 a byte, +0x14 the stage count (the highest + 1),
// +0x18 the stages set, +0x1C those with a texture, +0x20 the stage array, +0x30 the render state
// group, +0x34 the pixel constant map (a reference), +0x38..+0x40 the pixel shader's file, entry
// point and target (NiMalloc'd), +0x44 the pixel shader (a reference), +0x48 the vertex constant
// map, +0x4C..+0x54 the vertex shader's strings, +0x58 the vertex shader, +0x5C software vertex
// processing, +0x5D from the pool, +0x60 the reference count.
//
// NiD3DPass's statics: 0x20E6D78 the renderer, 0x20E6D74 its device (held), 0x20E6D7C the render
// state.

#include <cstddef>
#include <cstdint>
#include <initializer_list>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::c39 {
using namespace ni::dx9;
namespace {

constexpr uintptr_t kPassVtable = 0x0099BDEC;
constexpr uintptr_t kStageArrayVtable = 0x0099BDFC;
constexpr uint32_t kPassSize = 0x64;

GAME_VAR(uint8_t*, ms_pkD3DRenderer, 0x020E6D78);
GAME_VAR(void*, ms_pkD3DDevice, 0x020E6D74);
GAME_VAR(uint8_t*, ms_pkD3DRenderState, 0x020E6D7C);
GAME_VAR(uint8_t*, ms_pkPassPool, 0x020E6D80);
GAME_VAR(NiCriticalSection, ms_kPassPoolLock, 0x020E6D84);
GAME_VAR(uint32_t, ms_uiMaxTextureBlendStages, 0x00BCB7F8);
GAME_VAR(uint32_t, ms_uiMaxSimultaneousTextures, 0x00BCB7FC);
GAME_VAR(uint32_t, ms_uiMaxSamplers, 0x00BCB800);

GAME_FN(void, __thiscall, Stage_ReturnStageToPool, 0x0089F670, void* stage);
GAME_FN(uint8_t, __thiscall, Stage_ApplyTextureStage, 0x0089F4B0, void* stage);
GAME_FN(void, __cdecl, Stage_DisableStage, 0x0089E6E0, uint32_t stage);
GAME_FN(uint8_t, __thiscall, Stage_GetStageState, 0x0089FAB0, void* stage, uint32_t state, uint32_t* value,
        uint8_t* saved);
GAME_FN(void, __thiscall, RenderStateGroup_SetRenderStates, 0x0089DC20, void* self);
GAME_FN(void, __thiscall, RenderStateGroup_RestoreRenderStates, 0x0089DCC0, void* self);
GAME_FN(void, __cdecl, RenderStateGroup_Release, 0x0089E480, void* group);
GAME_FN(void, __thiscall, RenderState_SetSoftwareVertexProcessing, 0x00883480, void* self, int software);
GAME_FN(re::Ptr32<void>*, __cdecl, NewPointerArray, 0x00856120, uint32_t count);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);   // NiMemObject::operator new[]
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);       // (the game's operator new)
GAME_FN(void, __stdcall, VectorConstructor, 0x00401240, void* p, uint32_t size, uint32_t count, void* ctor);
GAME_FN(void, __thiscall, Set_Realloc, 0x0083AD20, void* set, uint32_t allocated);
GAME_FN(void, __thiscall, Pool_Destruct, 0x00800050, void* pool);

// this file's (called where modes apply)
GAME_FN(void, __thiscall, StageArray_SetAt, 0x00884120, void* self, uint32_t index, const re::Ptr32<uint8_t>* stage);
GAME_FN(void, __thiscall, StageArray_RemoveAll, 0x008841B0, void* self);
GAME_FN(void, __thiscall, StageArray_SetSize, 0x00884230, void* self, uint32_t size);
GAME_FN(void, __cdecl, StageArray_DeleteArray, 0x008840D0, re::Ptr32<void>* array);
GAME_FN(void, __thiscall, Set_AddUnique, 0x00884380, void* self, const re::Ptr32<void>* element);
GAME_FN(void, __thiscall, Pass_Destruct, 0x008843E0, void* self);
GAME_FN(uint8_t*, __cdecl, Pass_NewArray, 0x00884910, uint32_t count);
GAME_FN(void, __thiscall, PassPool_CreateNewObjects, 0x008849E0, void* pool, uint32_t count);
constexpr uintptr_t kPassConstructor = 0x008845D0;

// a stage reference let go (the plain count at +0x5C)
inline void StageRelease(uint8_t* stage) {
    if (stage != nullptr && --At<uint32_t>(stage, 0x5C) == 0) {
        Stage_ReturnStageToPool(stage);
    }
}
inline void StageAddRef(uint8_t* stage) {
    if (stage != nullptr) {
        At<uint32_t>(stage, 0x5C)++;
    }
}

} // namespace

// 0x883C60: NiD3DPass::GetD3DRenderState
void* __cdecl Pass_GetD3DRenderState() {
    return ms_pkD3DRenderState;
}

// 0x883C70 (thiscall, slot 3): NiD3DPass::PostProcessRenderingPass (its render states restored)
uint32_t __fastcall Pass_PostProcessRenderingPass(uint8_t* self, re::Edx, uint32_t) {
    if (At<void*>(self, 0x30) != nullptr) {
        RenderStateGroup_RestoreRenderStates(At<void*>(self, 0x30));
    }
    return 0;
}

// 0x883C90 (thiscall): whether `stages` more stages and `textures` more textures fit the device
// (the stage count against the blend stages, both counts against the simultaneous textures).
// (Name a guess: NiD3DPass::CheckFreeStages.)
uint8_t __fastcall Pass_CheckFreeStages(uint8_t* self, re::Edx, uint32_t stages, uint32_t textures) {
    uint32_t count = At<uint32_t>(self, 0x14);
    if (ms_uiMaxTextureBlendStages < stages + count) {
        return 0;
    }
    if (ms_uiMaxSimultaneousTextures < At<uint32_t>(self, 0x1C) + textures) {
        return 0;
    }
    return ms_uiMaxSimultaneousTextures >= count + textures ? 1 : 0;
}

// 0x883D00 (thiscall, slot 2): NiD3DPass::SetupRenderingPass (software vertex processing as the
// vertex shader says; its render states; each stage applied up to the first empty one, the next
// stage disabled; the last stage's result)
uint32_t __fastcall Pass_SetupRenderingPass(uint8_t* self, re::Edx, void*, void*, void*, void*, void*, void*, void*,
                                            void*) {
    uint32_t result = 0;
    uint32_t software;
    if (At<void*>(self, 0x58) != nullptr) {
        software = Virtual<uint32_t>(At<void*>(self, 0x58), 22);   // GetSoftwareVertexProcessing
    } else {
        software = At<uint8_t>(self, 0x5C);
    }
    RenderState_SetSoftwareVertexProcessing(ms_pkD3DRenderState, int(software));
    if (At<void*>(self, 0x30) != nullptr) {
        RenderStateGroup_SetRenderStates(At<void*>(self, 0x30));
    }
    uint32_t count = At<uint16_t>(self, 0x2A);
    uint32_t i = 0;
    for (; i < count; i++) {
        void* stage = At<re::Ptr32<void>*>(self, 0x24)[i];
        if (stage == nullptr) {
            break;
        }
        result = Stage_ApplyTextureStage(stage);
    }
    if (i < ms_uiMaxSamplers) {
        Stage_DisableStage(i);
    }
    return result;
}

// 0x883D80: NiD3DPass::SetD3DRenderer (the renderer, its device held, its render state)
void __cdecl Pass_SetD3DRenderer(uint8_t* renderer) {
    ms_pkD3DRenderer = renderer;
    if (renderer != nullptr) {
        void* device = At<void*>(renderer, kDevice);
        if (ms_pkD3DDevice != nullptr) {
            Com<long>(ms_pkD3DDevice, kRelease);
        }
        ms_pkD3DDevice = device;
        if (device != nullptr) {
            Com<long>(device, kAddRef);
        }
        ms_pkD3DRenderState = At<uint8_t*>(ms_pkD3DRenderer, kRenderState);
        return;
    }
    if (ms_pkD3DDevice != nullptr) {
        Com<long>(ms_pkD3DDevice, kRelease);
    }
    ms_pkD3DDevice = nullptr;
    ms_pkD3DRenderState = nullptr;
}

// 0x883DF0 (thiscall, slot 1): NiD3DPass::SetupShaderPrograms (the pixel shader set and its
// constants; the vertex shader set and its constants, every stage's texture coordinate index its
// own and projected texture coordinates where a stage asks; without a vertex shader the buffer
// data's FVF, else its declaration). (The game reuses its argument slots for locals: the
// arguments themselves don't change.)
uint32_t __fastcall Pass_SetupShaderPrograms(uint8_t* self, re::Edx, uint8_t* geometry, void* skin, uint8_t* partition,
                                             uint8_t* bufferData, void* properties, void* effects,
                                             const void* world, const void* bound, uint32_t pass) {
    void* pixelShader = At<void*>(self, 0x44);
    if (pixelShader == nullptr) {
        uint8_t* state = ms_pkD3DRenderState;
        if (At<void*>(state, 0x20E8) != nullptr) {
            At<void*>(state, 0x20E8) = nullptr;
            Com<long>(At<void*>(state, 0x2238), 107, static_cast<void*>(nullptr));   // SetPixelShader
        }
    } else {
        void* handle = Virtual<void*>(pixelShader, 16);   // GetShaderHandle
        uint8_t* state = ms_pkD3DRenderState;
        if (At<void*>(state, 0x20E8) != handle) {
            At<void*>(state, 0x20E8) = handle;
            Com<long>(At<void*>(state, 0x2238), 107, handle);
        }
        if (At<void*>(self, 0x34) != nullptr) {
            Virtual<uint32_t>(At<void*>(self, 0x34), 12, At<void*>(self, 0x44), geometry, skin, partition, bufferData,
                              properties, effects, world, bound, pass, 0u);   // SetShaderConstants
        }
    }
    void* vertexShader = At<void*>(self, 0x58);
    if (vertexShader == nullptr) {
        uint8_t* state = ms_pkD3DRenderState;
        if (At<void*>(state, 0x20E0) != nullptr) {
            At<void*>(state, 0x20E0) = nullptr;
            Com<long>(At<void*>(state, 0x2238), 92, static_cast<void*>(nullptr));   // SetVertexShader
            state = ms_pkD3DRenderState;
        }
        if (bufferData == nullptr) {
            bufferData = partition != nullptr ? At<uint8_t*>(partition, 0x28)
                                              : At<uint8_t*>(At<uint8_t*>(geometry, 0xCC), 0x3C);
        }
        uint32_t fvf = At<uint32_t>(bufferData, 0xC);
        if (fvf != 0) {
            if (At<uint8_t>(state, 0x20F7) != 0 || At<uint32_t>(state, 0x20F8) != fvf) {
                At<uint32_t>(state, 0x20F8) = fvf;
                At<uint8_t>(state, 0x20F7) = 0;
                Com<long>(At<void*>(state, 0x2238), 89, fvf);   // SetFVF
            }
            return 0;
        }
        void* declaration = At<void*>(bufferData, 0x10);
        if (declaration != nullptr &&
            (At<uint8_t>(state, 0x20F7) == 0 || At<void*>(state, 0x2100) != declaration)) {
            At<void*>(state, 0x2100) = declaration;
            At<uint8_t>(state, 0x20F7) = 1;
            Com<long>(At<void*>(state, 0x2238), 87, declaration);   // SetVertexDeclaration
        }
        return 0;
    }
    void* handle = Virtual<void*>(vertexShader, 18);   // GetShaderHandle
    uint8_t* state = ms_pkD3DRenderState;
    if (At<void*>(state, 0x20E0) != handle) {
        At<void*>(state, 0x20E0) = handle;
        Com<long>(At<void*>(state, 0x2238), 92, handle);
    }
    if (At<void*>(self, 0x48) != nullptr) {
        Virtual<uint32_t>(At<void*>(self, 0x48), 12, At<void*>(self, 0x58), geometry, skin, partition, bufferData,
                          properties, effects, world, bound, pass, 0u);
    }
    for (uint32_t i = 0; i < ms_uiMaxTextureBlendStages; i++) {
        int offset = int(i) * 0x108;
        if (At<uint32_t>(ms_pkD3DRenderState, offset + 0x978) != i) {   // TEXCOORDINDEX
            Com<long>(At<void*>(ms_pkD3DRenderState, 0x2238), 67, i, 11u, i);
            At<uint32_t>(ms_pkD3DRenderState, offset + 0x978) = i;
        }
        uint32_t flags = 0;
        uint8_t* stage = nullptr;
        if (i < At<uint32_t>(self, 0x18) && (stage = At<re::Ptr32<uint8_t>*>(self, 0x24)[i]) != nullptr &&
            At<void*>(stage, 0xC) != nullptr) {
            uint32_t value = 0;
            uint8_t saved = 0;
            if (Stage_GetStageState(stage, 0x18, &value, &saved) && (value & 0x100)) {
                flags = 0x100;   // D3DTTFF_PROJECTED
            }
        }
        uint8_t* rs = ms_pkD3DRenderState;
        if (At<uint32_t>(rs, offset + 0x9E0) != flags) {   // TEXTURETRANSFORMFLAGS
            Com<long>(At<void*>(rs, 0x2238), 67, i, 0x18u, flags);
            At<uint32_t>(rs, offset + 0x9E0) = flags;
        }
    }
    return 0;
}

// 0x8840D0: an NiD3DTextureStagePtr array (new[]'d with its count before it) deleted, its
// references let go from the last
void __cdecl StageArray_DeleteArrayImpl(re::Ptr32<uint8_t>* array) {
    if (array == nullptr) {
        return;
    }
    int32_t count = reinterpret_cast<int32_t*>(array)[-1];
    for (int32_t i = count - 1; i >= 0; i--) {
        StageRelease(array[i]);
    }
    NiDeleteArray(reinterpret_cast<int32_t*>(array) - 1, 4);
}

// 0x884120 (thiscall): NiTObjectArray<NiD3DTextureStagePtr>::SetAt
void __fastcall StageArray_SetAtImpl(uint8_t* self, re::Edx, uint32_t index, const re::Ptr32<uint8_t>* stage) {
    if (index < At<uint16_t>(self, 0xA)) {
        if (*stage != nullptr) {
            if (At<re::Ptr32<uint8_t>*>(self, 4)[index] == nullptr) {
                At<uint16_t>(self, 0xC)++;
            }
        } else if (At<re::Ptr32<uint8_t>*>(self, 4)[index] != nullptr) {
            At<uint16_t>(self, 0xC)--;
        }
    } else {
        At<uint16_t>(self, 0xA) = uint16_t(index + 1);
        if (*stage != nullptr) {
            At<uint16_t>(self, 0xC)++;
        }
    }
    re::Ptr32<uint8_t>& slot = At<re::Ptr32<uint8_t>*>(self, 4)[index];
    if (slot != *stage) {
        StageRelease(slot);
        slot = *stage;
        StageAddRef(slot);
    }
}

// 0x8841B0 (thiscall): NiTObjectArray<NiD3DTextureStagePtr>::RemoveAll
void __fastcall StageArray_RemoveAllImpl(uint8_t* self, re::Edx) {
    for (uint16_t i = 0; i < At<uint16_t>(self, 0xA); i++) {
        re::Ptr32<uint8_t>& slot = At<re::Ptr32<uint8_t>*>(self, 4)[i];
        if (slot != nullptr) {
            StageRelease(slot);
            slot = nullptr;
        }
    }
    At<uint16_t>(self, 0xC) = 0;
    At<uint16_t>(self, 0xA) = 0;
}

// 0x884200 (thiscall): NiTObjectArray<NiD3DTextureStagePtr>'s deleting destructor
uint8_t* __fastcall StageArray_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kStageArrayVtable;
    StageArray_DeleteArray(At<re::Ptr32<void>*>(self, 4));
    if (flags & 1) {
        NiDelete(self, 0x10);
    }
    return self;
}

// 0x884230 (thiscall): NiTObjectArray<NiD3DTextureStagePtr>::SetSize (the slots past the new size
// emptied; a new array, the references copied over, the old one deleted)
void __fastcall StageArray_SetSizeImpl(uint8_t* self, re::Edx, uint32_t size) {
    if (size == At<uint16_t>(self, 8)) {
        return;
    }
    uint16_t newSize = uint16_t(size);
    if (size < At<uint16_t>(self, 0xA)) {
        for (uint32_t i = size & 0xFFFF; uint16_t(i) < At<uint16_t>(self, 0xA); i++) {
            re::Ptr32<uint8_t>& slot = At<re::Ptr32<uint8_t>*>(self, 4)[i & 0xFFFF];
            if (slot != nullptr) {
                StageRelease(slot);
                slot = nullptr;
                At<uint16_t>(self, 0xC)--;
            }
        }
        At<uint16_t>(self, 0xA) = newSize;
    }
    re::Ptr32<uint8_t>* old = At<re::Ptr32<uint8_t>*>(self, 4);
    At<uint16_t>(self, 8) = newSize;
    if (size == 0) {
        At<re::Ptr32<uint8_t>*>(self, 4) = nullptr;
    } else {
        At<re::Ptr32<uint8_t>*>(self, 4) = reinterpret_cast<re::Ptr32<uint8_t>*>(NewPointerArray(size & 0xFFFF));
        for (uint16_t i = 0; i < At<uint16_t>(self, 0xA); i++) {
            re::Ptr32<uint8_t>& slot = At<re::Ptr32<uint8_t>*>(self, 4)[i];
            if (slot != old[i]) {
                StageRelease(slot);
                slot = old[i];
                StageAddRef(slot);
            }
        }
        for (uint16_t i = At<uint16_t>(self, 0xA); i < At<uint16_t>(self, 8); i++) {
            re::Ptr32<uint8_t>& slot = At<re::Ptr32<uint8_t>*>(self, 4)[i];
            if (slot != nullptr) {
                StageRelease(slot);
                slot = nullptr;
            }
        }
    }
    StageArray_DeleteArray(reinterpret_cast<re::Ptr32<void>*>(old));
}

// 0x884380 (thiscall): NiTPrimitiveSet<T*>::AddUnique (one copy shared by several element types:
// {+0 the elements, +4 allocated, +8 used}; doubled when full)
void __fastcall Set_AddUniqueImpl(uint8_t* self, re::Edx, const re::Ptr32<void>* element) {
    uint32_t used = At<uint32_t>(self, 8);
    for (uint32_t i = 0; i < used; i++) {
        if (At<re::Ptr32<void>*>(self, 0)[i] == *element) {
            return;
        }
    }
    uint32_t allocated = At<uint32_t>(self, 4);
    if (used == allocated) {
        Set_Realloc(self, allocated == 0 ? 1 : allocated * 2);
    }
    At<re::Ptr32<void>*>(self, 0)[At<uint32_t>(self, 8)] = *element;
    At<uint32_t>(self, 8)++;
}

// 0x8843E0 (thiscall): NiD3DPass::~NiD3DPass
void __fastcall Pass_DestructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kPassVtable;
    RenderStateGroup_Release(At<void*>(self, 0x30));
    if (At<void*>(self, 0x34) != nullptr) {
        ReleaseRef(At<void*>(self, 0x34));
        At<void*>(self, 0x34) = nullptr;
    }
    NiFree(At<void*>(self, 0x38));
    NiFree(At<void*>(self, 0x3C));
    NiFree(At<void*>(self, 0x40));
    if (At<void*>(self, 0x44) != nullptr) {
        ReleaseRef(At<void*>(self, 0x44));
        At<void*>(self, 0x44) = nullptr;
    }
    if (At<void*>(self, 0x48) != nullptr) {
        ReleaseRef(At<void*>(self, 0x48));
        At<void*>(self, 0x48) = nullptr;
    }
    NiFree(At<void*>(self, 0x4C));
    NiFree(At<void*>(self, 0x50));
    NiFree(At<void*>(self, 0x54));
    if (At<void*>(self, 0x58) != nullptr) {
        ReleaseRef(At<void*>(self, 0x58));
        At<void*>(self, 0x58) = nullptr;
    }
    StageArray_RemoveAll(self + 0x20);
    At<uint32_t>(self, 0x14) = 0;
    At<uint32_t>(self, 0x18) = 0;
    At<uint32_t>(self, 0x1C) = 0;
    // the members' destructors (the references already let go)
    ReleaseRef(At<void*>(self, 0x58));
    ReleaseRef(At<void*>(self, 0x48));
    ReleaseRef(At<void*>(self, 0x44));
    ReleaseRef(At<void*>(self, 0x34));
    At<uint32_t>(self, 0x20) = kStageArrayVtable;
    StageArray_DeleteArray(At<re::Ptr32<void>*>(self, 0x24));
}

// 0x884560 (thiscall): NiD3DPass's vector deleting destructor
uint8_t* __fastcall Pass_VectorDeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    if ((flags & 2) == 0) {
        Pass_Destruct(self);
        if (flags & 1) {
            NiDelete(self, kPassSize);
        }
        return self;
    }
    int32_t count = At<int32_t>(self, -4);
    while (--count >= 0) {
        Pass_Destruct(self + count * int32_t(kPassSize));
    }
    if (flags & 1) {
        NiDeleteArray(self - 4, kPassSize);
    }
    return self - 4;
}

// 0x8845D0 (thiscall): NiD3DPass::NiD3DPass (as many stage slots as blend stages, growing by 4)
uint8_t* __fastcall Pass_Construct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kPassVtable;
    At<uint32_t>(self, 0x14) = 0;
    At<uint32_t>(self, 0x18) = 0;
    At<uint32_t>(self, 0x1C) = 0;
    uint32_t stages = ms_uiMaxTextureBlendStages;
    At<uint32_t>(self, 0x20) = kStageArrayVtable;
    At<uint16_t>(self, 0x28) = uint16_t(stages);
    At<uint16_t>(self, 0x2E) = 4;
    At<uint16_t>(self, 0x2A) = 0;
    At<uint16_t>(self, 0x2C) = 0;
    At<re::Ptr32<void>*>(self, 0x24) = uint16_t(stages) == 0 ? nullptr : NewPointerArray(stages & 0xFFFF);
    At<uint32_t>(self, 0x20) = kStageArrayVtable;
    for (int offset = 0x30; offset <= 0x58; offset += 4) {
        At<void*>(self, offset) = nullptr;
    }
    At<uint8_t>(self, 0x5C) = 0;
    At<uint8_t>(self, 0x5D) = 0;
    At<uint32_t>(self, 0x60) = 0;
    At<uint8_t>(self, 4) = 0;
    return self;
}

// 0x884660: NiD3DPass::CreatePassPool (an object pool growing by 8)
void __cdecl Pass_CreatePassPool() {
    Lock(ms_kPassPoolLock);
    uint32_t* pool = static_cast<uint32_t*>(NiNew(0x18));
    if (pool != nullptr) {
        pool[0] = 0;
        pool[1] = 0;
        pool[2] = 0;
        pool[3] = 0;
        pool[4] = 8;
        pool[5] = 8;
    }
    ms_pkPassPool = reinterpret_cast<uint8_t*>(pool);
    Unlock(ms_kPassPoolLock);
}

// 0x8846D0 (thiscall): NiD3DPass::ReturnPassToPool (deleted when it isn't a pooled one; else its
// references, strings and stages let go and it goes back into the pool's free set)
void __fastcall Pass_ReturnPassToPool(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 0x5D) == 0) {
        Virtual<void>(self, 0, 1);
        return;
    }
    for (int offset : {0x34, 0x44, 0x48, 0x58}) {
        if (At<void*>(self, offset) != nullptr) {
            ReleaseRef(At<void*>(self, offset));
            At<void*>(self, offset) = nullptr;
        }
    }
    RenderStateGroup_Release(At<void*>(self, 0x30));
    At<void*>(self, 0x30) = nullptr;
    for (int offset : {0x38, 0x3C, 0x40, 0x4C, 0x50, 0x54}) {
        NiFree(At<void*>(self, offset));
        At<void*>(self, offset) = nullptr;
    }
    StageArray_RemoveAll(self + 0x20);
    At<uint32_t>(self, 0x14) = 0;
    At<uint32_t>(self, 0x18) = 0;
    At<uint32_t>(self, 0x1C) = 0;
    At<uint8_t>(self, 0x5C) = 0;
    Lock(ms_kPassPoolLock);
    uint8_t* pass = self;
    Set_AddUnique(ms_pkPassPool, re::PtrCast<const re::Ptr32<void>*>(&pass));
    Unlock(ms_kPassPoolLock);
}

// 0x884830 (thiscall): NiD3DPass::SetStage (a stage set at an index past the samplers ignored;
// past the blend stages the array grown first; the stage given its index and counted). (Clearing
// the last stage spins forever when the slot below it isn't empty: the game's loop never changes
// what it tests.)
uint32_t __fastcall Pass_SetStage(uint8_t* self, re::Edx, uint32_t index, re::Ptr32<uint8_t> stage) {
    if (index >= ms_uiMaxSamplers) {
        return 1;
    }
    StageAddRef(stage);   // (the argument's copy)
    if (index >= ms_uiMaxTextureBlendStages && index >= At<uint16_t>(self, 0x28)) {
        StageArray_SetSize(self + 0x20, At<uint16_t>(self, 0x2E) + index);
    }
    StageArray_SetAt(self + 0x20, index, &stage);
    if (stage == nullptr) {
        uint32_t count = At<uint32_t>(self, 0x14);
        if (count == index + 1 && count != 0) {
            volatile uint8_t* const* slots = At<volatile uint8_t* const*>(self, 0x24);
            while (slots[count - 1] != nullptr) {
                if (count == 0) {
                    return 0;
                }
            }
            At<uint32_t>(self, 0x14) = count - 1;
        }
        return 0;
    }
    StageRelease(stage);
    At<uint32_t>(stage, 0) = index;
    if (At<uint32_t>(self, 0x14) == index) {
        At<uint32_t>(self, 0x14) = index + 1;
    }
    At<uint32_t>(self, 0x18)++;
    if (At<void*>(stage, 4) != nullptr) {
        At<uint32_t>(self, 0x1C)++;
    }
    return 0;
}

// 0x884910: NiD3DPass's new[] (the count before the passes, each constructed)
uint8_t* __cdecl Pass_NewArrayImpl(uint32_t count) {
    uint32_t* memory;
    if (count == 0) {
        memory = static_cast<uint32_t*>(Malloc(4));
    } else {
        uint64_t bytes = uint64_t(count) * kPassSize;
        uint32_t size = (bytes >> 32) != 0 ? 0xFFFFFFFFu : uint32_t(bytes);
        size = size > 0xFFFFFFFBu ? 0xFFFFFFFFu : size + 4;
        memory = static_cast<uint32_t*>(NiNewArray(size));
    }
    if (memory == nullptr) {
        return nullptr;
    }
    memory[0] = count;
    VectorConstructor(memory + 1, kPassSize, count, reinterpret_cast<void*>(kPassConstructor));
    return reinterpret_cast<uint8_t*>(memory + 1);
}

// 0x884970: NiD3DPass::DestroyPassPool
void __cdecl Pass_DestroyPassPool() {
    Lock(ms_kPassPoolLock);
    uint8_t* pool = ms_pkPassPool;
    if (pool != nullptr) {
        Pool_Destruct(pool);
        NiDelete(pool, 0x18);
    }
    ms_pkPassPool = nullptr;
    Unlock(ms_kPassPoolLock);
}

// 0x8849E0 (thiscall): NiTObjectPool<NiD3DPass>::CreateNewObjects (a block of `count` passes, each
// into the free set; the block put at the head of the pool's list)
void __fastcall PassPool_CreateNewObjectsImpl(uint8_t* self, re::Edx, uint32_t count) {
    uint32_t* block = static_cast<uint32_t*>(NiNew(0xC));
    if (block != nullptr) {
        block[1] = count;
        block[2] = 0;
        block[0] = count == 0 ? 0 : reinterpret_cast<uint32_t>(Pass_NewArray(count));
    }
    for (uint32_t i = 0; i < count; i++) {
        uint8_t* pass = i < block[1] ? reinterpret_cast<uint8_t*>(block[0]) + i * kPassSize : nullptr;
        uint32_t allocated = At<uint32_t>(self, 4);
        if (At<uint32_t>(self, 8) == allocated) {
            Set_Realloc(self, allocated == 0 ? 1 : allocated * 2);
        }
        At<re::Ptr32<uint8_t>*>(self, 0)[At<uint32_t>(self, 8)] = pass;
        At<uint32_t>(self, 8)++;
    }
    block[2] = At<uint32_t>(self, 0xC);
    At<uint32_t*>(self, 0xC) = block;
}

// 0x884A90: NiD3DPass::CreateNewPass (one from the pool, more made when it's empty (doubling the
// next block); held through `out`; marked as pooled)
re::Ptr32<uint8_t>* __cdecl Pass_CreateNewPass(re::Ptr32<uint8_t>* out) {
    Lock(ms_kPassPoolLock);
    uint8_t* pool = ms_pkPassPool;
    if (At<uint32_t>(pool, 8) == 0) {
        PassPool_CreateNewObjects(pool, At<uint32_t>(pool, 0x10));
        At<uint32_t>(pool, 0x10) *= 2;
    }
    re::Ptr32<uint8_t>* set = At<re::Ptr32<uint8_t>*>(pool, 0);
    uint8_t* pass = set[0];
    At<uint32_t>(pool, 8)--;
    set[0] = set[At<uint32_t>(pool, 8)];
    *out = pass;
    if (pass != nullptr) {
        At<uint32_t>(pass, 0x60)++;
    }
    Unlock(ms_kPassPoolLock);
    if (At<uint8_t>(*out, 0x5D) == 0) {
        At<uint8_t>(*out, 0x5D) = 1;
    }
    return out;
}

#define C39D_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C39D_REVERSED("NiD3DPass::GetD3DRenderState", 0x00883C60, Pass_GetD3DRenderState);
C39D_REVERSED("NiD3DPass::PostProcessRenderingPass", 0x00883C70, Pass_PostProcessRenderingPass);
C39D_REVERSED("NiD3DPass::CheckFreeStages", 0x00883C90, Pass_CheckFreeStages);
C39D_REVERSED("NiD3DPass::SetupRenderingPass", 0x00883D00, Pass_SetupRenderingPass);
C39D_REVERSED("NiD3DPass::SetD3DRenderer", 0x00883D80, Pass_SetD3DRenderer);
C39D_REVERSED("NiD3DPass::SetupShaderPrograms", 0x00883DF0, Pass_SetupShaderPrograms);
C39D_REVERSED("NiTObjectArray<NiD3DTextureStagePtr>::DeleteArray_8840D0", 0x008840D0, StageArray_DeleteArrayImpl);
C39D_REVERSED("NiTObjectArray<NiD3DTextureStagePtr>::SetAt_884120", 0x00884120, StageArray_SetAtImpl);
C39D_REVERSED("NiTObjectArray<NiD3DTextureStagePtr>::RemoveAll_8841B0", 0x008841B0, StageArray_RemoveAllImpl);
C39D_REVERSED("NiTObjectArray<NiD3DTextureStagePtr>::DeletingDestructor_884200", 0x00884200,
              StageArray_DeletingDestructor);
C39D_REVERSED("NiTObjectArray<NiD3DTextureStagePtr>::SetSize_884230", 0x00884230, StageArray_SetSizeImpl);
C39D_REVERSED("NiTPrimitiveSet<void*>::AddUnique_884380", 0x00884380, Set_AddUniqueImpl);
C39D_REVERSED("NiD3DPass::~NiD3DPass", 0x008843E0, Pass_DestructImpl);
C39D_REVERSED("NiD3DPass::VectorDeletingDestructor", 0x00884560, Pass_VectorDeletingDestructor);
C39D_REVERSED("NiD3DPass::NiD3DPass", 0x008845D0, Pass_Construct);
C39D_REVERSED("NiD3DPass::CreatePassPool", 0x00884660, Pass_CreatePassPool);
C39D_REVERSED("NiD3DPass::ReturnPassToPool", 0x008846D0, Pass_ReturnPassToPool);
C39D_REVERSED("NiD3DPass::SetStage", 0x00884830, Pass_SetStage);
C39D_REVERSED("NiD3DPass::NewArray", 0x00884910, Pass_NewArrayImpl);
C39D_REVERSED("NiD3DPass::DestroyPassPool", 0x00884970, Pass_DestroyPassPool);
C39D_REVERSED("NiTObjectPool<NiD3DPass>::CreateNewObjects", 0x008849E0, PassPool_CreateNewObjectsImpl);
C39D_REVERSED("NiD3DPass::CreateNewPass", 0x00884A90, Pass_CreateNewPass);

} // namespace ni::c39
