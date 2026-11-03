// NiAnimationSDM: the animation library's static data (its Init and Shutdown given to
// NiStaticDataManager by the static object's constructor): NiStream's loaders for the animation
// classes (by name, unregistered at Shutdown), NiStream's post-process function (0x804EA0),
// NiActorManager's text keys and the geometry morpher task pool; the timeline node allocator's
// blocks freed at Shutdown.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni {
namespace {

GAME_VAR(uint8_t, NiAnimationSDM_ms_bInitialized, 0x020E4361);
GAME_VAR(int32_t, NiAnimationSDM_ms_iInitCount, 0x020E4364);
// the timeline nodes' allocator (0x1C-byte nodes in NiNew[] blocks; each block's first node
// links the next block)
GAME_VAR(void*, TimelineAllocator_ms_pBlockList, 0x020E4AA8);
GAME_VAR(void*, TimelineAllocator_ms_pFreeList, 0x020E4AAC);

constexpr uintptr_t kInit = 0x007FEFF0;
constexpr uintptr_t kShutdown = 0x007FF410;
constexpr uintptr_t kPostProcess = 0x00804EA0;   // the stream's post-process (animation conversion)

using CreateFn = void*(__cdecl*)();
using PostProcessFn = void(__cdecl*)(void* stream);
GAME_FN(void, __cdecl, NiStream_RegisterLoader, 0x00759EC0, const char* name, CreateFn create);
GAME_FN(void, __cdecl, NiStream_UnregisterLoader, 0x00759EE0, const char* name);
GAME_FN(void, __cdecl, NiStream_RegisterPostProcessFunction, 0x0075B7F0, PostProcessFn function);
GAME_FN(void, __cdecl, NiStream_UnregisterPostProcessFunction, 0x00759F00, PostProcessFn function);
GAME_FN(void, __cdecl, NiStaticDataManager_AddLibrary, 0x00753680, void(__cdecl* init)(), void(__cdecl* shutdown)());
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __thiscall, TimelineData_Destruct, 0x00402D30, void* self);
GAME_FN(void, __cdecl, ActorManager_SDMInit, 0x00800D60);
GAME_FN(void, __cdecl, ActorManager_SDMShutdown, 0x00800D80);
GAME_FN(void, __cdecl, GeomMorpherUpdateTask_SDMInit, 0x00800160);
GAME_FN(void, __cdecl, GeomMorpherUpdateTask_SDMShutdown, 0x00800260);
// (this file's, called where modes apply)
GAME_FN(void, __cdecl, TimelineAllocator_SDMShutdown, 0x007FF3A0);

// the loaders: each class name (the game's string: the loader map keeps the pointer) and its
// CreateObject, in the order Init registers them
struct Loader {
    uintptr_t name;
    uintptr_t create;
};
constexpr Loader kLoaders[] = {
    {0x00965960, 0x00822680},   // NiAlphaController
    {0x0096593C, 0x008215D0},   // NiBlendAccumTransformInterpolator
    {0x00965924, 0x00820C20},   // NiBlendBoolInterpolator
    {0x00965908, 0x00820670},   // NiBlendColorInterpolator
    {0x009658EC, 0x00820230},   // NiBlendFloatInterpolator
    {0x009658D0, 0x0081FD00},   // NiBlendPoint3Interpolator
    {0x009658B0, 0x0081F6E0},   // NiBlendQuaternionInterpolator
    {0x00965890, 0x0081F000},   // NiBlendTransformInterpolator
    {0x0096587C, 0x0081E900},   // NiBoneLODController
    {0x00965870, 0x0081D6C0},   // NiBoolData
    {0x0096585C, 0x0081D210},   // NiBoolInterpolator
    {0x00965840, 0x0081CED0},   // NiBoolTimelineInterpolator
    {0x0096582C, 0x0081CAC0},   // NiBSplineBasisData
    {0x0096581C, 0x0081BD40},   // NiBSplineData
    {0x00965800, 0x0081BB90},   // NiBSplineColorInterpolator
    {0x009657E0, 0x0081B810},   // NiBSplineCompColorInterpolator
    {0x009657C0, 0x0081B540},   // NiBSplineCompFloatInterpolator
    {0x009657A0, 0x0081B2F0},   // NiBSplineCompPoint3Interpolator
    {0x0096577C, 0x0081ADE0},   // NiBSplineCompTransformInterpolator
    {0x00965760, 0x0081A9C0},   // NiBSplineFloatInterpolator
    {0x00965744, 0x0081A730},   // NiBSplinePoint3Interpolator
    {0x00965724, 0x0081A080},   // NiBSplineTransformInterpolator
    {0x00965718, 0x00819A00},   // NiColorData
    {0x009656FC, 0x008196E0},   // NiColorExtraDataController
    {0x009656E8, 0x008190C0},   // NiColorInterpolator
    {0x009656D4, 0x00818DC0},   // NiControllerManager
    {0x009656BC, 0x008158C0},   // NiControllerSequence
    {0x009656A8, 0x008136D0},   // NiFlipController
    {0x0096569C, 0x00812C70},   // NiFloatData
    {0x00965680, 0x00812980},   // NiFloatExtraDataController
    {0x0096566C, 0x00812490},   // NiFloatInterpolator
    {0x00965650, 0x00812010},   // NiFloatsExtraDataController
    {0x0096562C, 0x00811D20},   // NiFloatsExtraDataPoint3Controller
    {0x00965614, 0x008116F0},   // NiGeomMorpherController
    {0x009655FC, 0x00810970},   // NiKeyframeController
    {0x009655EC, 0x0080F900},   // NiKeyframeData
    {0x009655D4, 0x0080F7C0},   // NiLightColorController
    {0x009655BC, 0x0080F2E0},   // NiLightDimmerController
    {0x009655A8, 0x0080F010},   // NiLookAtController
    {0x00965590, 0x0080E640},   // NiLookAtInterpolator
    {0x00965574, 0x0080DA90},   // NiMaterialColorController
    {0x00965568, 0x0080CB80},   // NiMorphData
    {0x00965544, 0x0080C930},   // NiMultiTargetTransformController
    {0x00965530, 0x0080C030},   // NiPathController
    {0x0096551C, 0x0080A830},   // NiPathInterpolator
    {0x00965504, 0x00808790},   // NiPoint3Interpolator
    {0x009654F8, 0x00808190},   // NiPosData
    {0x009654DC, 0x00807BC0},   // NiQuaternionInterpolator
    {0x009654C8, 0x008078C0},   // NiRollController
    {0x009654BC, 0x008072D0},   // NiRotData
    {0x009654AC, 0x00807110},   // NiStringPalette
    {0x00965498, 0x00806AB0},   // NiTextKeyExtraData
    {0x00965478, 0x008069E0},   // NiTextureTransformController
    {0x00965460, 0x00810970},   // NiTransformController
    {0x00965450, 0x0080F900},   // NiTransformData
    {0x00965438, 0x008055E0},   // NiTransformInterpolator
    {0x00965428, 0x008050C0},   // NiVisController
    {0x0096541C, 0x0081D6C0},   // NiVisData
};
static_assert(sizeof(kLoaders) / sizeof(kLoaders[0]) == 58);

// 0x7FEFF0 Init (once): the loaders, the post-process function, NiActorManager's statics, the
// task pool
void __cdecl Init() {
    if (NiAnimationSDM_ms_bInitialized != 0) {
        return;
    }
    NiAnimationSDM_ms_bInitialized = 1;
    for (const Loader& loader : kLoaders) {
        NiStream_RegisterLoader(reinterpret_cast<const char*>(loader.name), reinterpret_cast<CreateFn>(loader.create));
    }
    NiStream_RegisterPostProcessFunction(reinterpret_cast<PostProcessFn>(kPostProcess));
    ActorManager_SDMInit();
    GeomMorpherUpdateTask_SDMInit();
}

// 0x7FF3A0 the timeline node allocator's shutdown: each block's nodes destroyed from the last,
// the block freed (NiDelete[]), then no blocks and no free list
void __cdecl TimelineAllocatorSDMShutdown() {
    uint8_t* block = static_cast<uint8_t*>(TimelineAllocator_ms_pBlockList);
    while (block != nullptr) {
        uint32_t* header = reinterpret_cast<uint32_t*>(block) - 1;
        uint32_t count = *header;
        uint8_t* next = *reinterpret_cast<re::Ptr32<uint8_t>*>(block);
        for (int32_t i = int32_t(count) - 1; i >= 0; i--) {
            TimelineData_Destruct(block + i * 0x1C + 8);
        }
        NiDeleteArray(header, 0x1C);
        block = next;
    }
    TimelineAllocator_ms_pBlockList = nullptr;
    TimelineAllocator_ms_pFreeList = nullptr;
}

// 0x7FF410 Shutdown (once Init ran): the loaders unregistered, the post-process function, the
// timeline allocator, NiActorManager's statics, the task pool
void __cdecl Shutdown() {
    if (NiAnimationSDM_ms_bInitialized == 0) {
        return;
    }
    NiAnimationSDM_ms_bInitialized = 0;
    for (const Loader& loader : kLoaders) {
        NiStream_UnregisterLoader(reinterpret_cast<const char*>(loader.name));
    }
    NiStream_UnregisterPostProcessFunction(reinterpret_cast<PostProcessFn>(kPostProcess));
    TimelineAllocator_SDMShutdown();
    ActorManager_SDMShutdown();
    GeomMorpherUpdateTask_SDMShutdown();
}

// 0x7FF690 NiAnimationSDM(): the first one gives Init and Shutdown to the static data manager
void* __fastcall Construct(void* self, re::Edx) {
    int32_t count = NiAnimationSDM_ms_iInitCount;
    NiAnimationSDM_ms_iInitCount = count + 1;
    if (count == 0) {
        NiStaticDataManager_AddLibrary(reinterpret_cast<void(__cdecl*)()>(kInit),
                                       reinterpret_cast<void(__cdecl*)()>(kShutdown));
    }
    return self;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIANIMSDM_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIANIMSDM_REVERSED("NiAnimationSDM::Init", 0x007FEFF0, ni::Init);
NIANIMSDM_REVERSED("NiTObjectAllocator<NiActorManager::TimelineData>::_SDMShutdown", 0x007FF3A0,
                   ni::TimelineAllocatorSDMShutdown);
NIANIMSDM_REVERSED("NiAnimationSDM::Shutdown", 0x007FF410, ni::Shutdown);
NIANIMSDM_REVERSED("NiAnimationSDM::NiAnimationSDM", 0x007FF690, ni::Construct);
