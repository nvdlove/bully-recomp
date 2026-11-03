// NiGeomMorpherController (vtable 0x9668E8, 0x44 bytes: src/gamebryo/NiAnimControllers.h): a
// NiInterpController with one float interpolator per morph target of its NiMorphData. Update
// only marks the pre-display pass needed (or hands it to the task manager as a
// NiGeomMorpherUpdateTask); OnPreDisplay weighs the targets' vertices into the target geometry's.
// Its interpolator slots, streaming (older files' weights and interpolators), cloning,
// comparison, the time extrema; and the slots it shares with the float interpolator controllers
// (InterpolatorIsCorrectType) and the point ones (CreateBlendInterpolator).
//
// Main-thread float maths at the game's 24-bit x87 precision.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiAnimControllers.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiGeometry.h"
#include "gamebryo/NiGeometryData.h"
#include "gamebryo/NiInterpolators.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni::anim {

namespace {

using scene::AddViewerString;
using scene::GetViewerString;
using scene::LoadBytes;
using scene::SaveBytes;
using scene::Stream_ReadLinkID;
using scene::Stream_ReadMultipleLinkIDs;
using scene::Stream_ResolveLinkID;
using scene::Stream_GetNumberOfLinkIDs;
using scene::StreamVersion;
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiGeomMorpherController_RTTI, 0x020E4F30);
GAME_VAR(uint32_t, kNiGeometryClassIndex, 0x020E25BC);
GAME_VAR(uint32_t, kNiFloatInterpolatorClassIndex, 0x020E4FD4);
GAME_VAR(uint8_t, ms_bAlwaysUpdate, 0x020E4F2C);   // (copied onto both by CopyMembers)
GAME_VAR(uint8_t, ms_bUseTasks, 0x020E4F2D);       // (the morph handed to the task manager)
GAME_VAR(void*, ms_pkTaskManager, 0x020E94E8);     // (+0x9C: it's running; slot 18 AddTask)
GAME_VAR(uint32_t, kTaskPriority, 0x00BC8C30);
GAME_VAR(NiPoint3, kZero, 0x020E2D44);             // NiPoint3::ZERO
GAME_VAR(uint16_t, kInvalidIndex, 0x00967FA0);     // NiInterpController::INVALID_INDEX

constexpr uintptr_t NiGeomMorpherController_vtable = 0x009668E8;
constexpr uintptr_t MorphInterpItem_Construct_address = 0x008003D0;   // {null, 0}
constexpr uint32_t kVersion_10_1_0_104 = 0x0A010068;
constexpr uint32_t kVersion_20_1_0_3 = 0x14010003;

GAME_FN(void*, __cdecl, NiAllocArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiFreeArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* ctor);
GAME_FN(int, __cdecl, NiStricmp, 0x00753820, const char* a, const char* b);
GAME_FN(NiObject*, __thiscall, Stream_ReadLinkIDObject, 0x0075A820, void* stream);
GAME_FN(NiObject*, __thiscall, Object_CreateSharedClone, 0x007553B0, void* self, void* cloning);
GAME_FN(void, __thiscall, Bound_ComputeFromData, 0x007D2890, NiBound* self, uint32_t count, const NiPoint3* points);
GAME_FN(void, __thiscall, Bound_Merge, 0x007D2E60, NiBound* self, const NiBound* other);
GAME_FN(void, __cdecl, Point3_AddScaledArray, 0x00770E40, NiPoint3* dest, float scale, const NiPoint3* src,
        uint32_t count);
// MorphTarget's name and interpolator through the getters the code reaches them by (folded
// ones: +0x4 and +0xC)
GAME_FN(re::Ptr32<const char>*, __thiscall, MorphTarget_GetNameAddress, 0x004F02A0, const MorphTarget* self);
GAME_FN(NiInterpolator*, __thiscall, MorphTarget_GetInterpolator, 0x00853140, const MorphTarget* self);
GAME_FN(uint8_t, __thiscall, TimeController_DontDoUpdate, 0x0077D890, void* self, float time);
GAME_FN(void, __thiscall, TimeController_SetTarget, 0x0077E050, void* self, NiObjectNET* target);
GAME_FN(void, __thiscall, TimeController_ProcessClone, 0x0077DEE0, void* self, void* cloning);
GAME_FN(void, __thiscall, TimeController_Destruct, 0x0077E010, void* self);
// NiInterpController's (the next chunks')
GAME_FN(void, __thiscall, InterpController_Construct, 0x00824F10, void* self);
GAME_FN(void, __thiscall, InterpController_CopyMembers, 0x00824ED0, void* self, void* dest, void* cloning);
GAME_FN(uint8_t, __thiscall, InterpController_RegisterStreamables, 0x00824EE0, void* self, void* stream);
GAME_FN(void, __thiscall, InterpController_SaveBinary, 0x00824EF0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, InterpController_IsEqual, 0x00824F00, void* self, void* other);
GAME_FN(void, __thiscall, InterpController_LinkObject, 0x00824F80, void* self, void* stream);
GAME_FN(void, __thiscall, InterpController_LoadBinary, 0x00824F90, void* self, void* stream);
// NiGeomMorpherUpdateTask's (src/gamebryo/NiGeomMorpherUpdateTask.cpp)
GAME_FN(NiObject*, __cdecl, GeomMorpherUpdateTask_GetFreeObject, 0x008002E0);
GAME_FN(void, __thiscall, GeomMorpherUpdateTask_SetController, 0x007FFEC0, NiObject* self, void* controller);
// the interpolators made (NiFloatInterpolator: src/gamebryo/NiFloatInterpolator.cpp; the blend
// point one: the next chunk's)
GAME_FN(NiInterpolator*, __thiscall, FloatInterpolator_ConstructPosed, 0x008122B0, void* self, float value);
GAME_FN(void, __thiscall, FloatInterpolator_SetPoseValue, 0x00812420, void* self, float value);
GAME_FN(NiInterpolator*, __thiscall, BlendPoint3Interpolator_Construct, 0x0081FCA0, void* self,
        int managerControlled, float weightThreshold, int arraySize);
// (this file's, called where modes apply)
GAME_FN(uint32_t, __thiscall, GeomMorpher_GetNumTargets, 0x00810B90, NiGeomMorpherController* self);
GAME_FN(void, __thiscall, GeomMorpher_UpdateMorphs, 0x00810D10, NiGeomMorpherController* self, float time);
GAME_FN(void, __thiscall, GeomMorpher_AllocateInterpItems, 0x00811600, NiGeomMorpherController* self);
GAME_FN(void, __thiscall, GeomMorpher_Destruct, 0x008114E0, NiGeomMorpherController* self);
GAME_FN(void, __thiscall, GeomMorpher_SetMorphData, 0x00811890, NiGeomMorpherController* self, NiMorphData* data,
        int updateBound);
GAME_FN(void, __thiscall, GeomMorpher_CopyMembers, 0x008118F0, NiGeomMorpherController* self,
        NiGeomMorpherController* dest, void* cloning);

inline NiGeometryData* TargetData(const NiGeomMorpherController* self) {
    return static_cast<NiGeometryData*>(static_cast<NiGeometry*>(self->target)->modelData);
}

// the morph target at an index, or null past the count (as the inlined GetTarget has it)
inline MorphTarget* TargetAt(const NiMorphData* data, uint32_t index) {
    return index < data->numTargets ? &data->targets[index] : nullptr;
}

// an interp item array for count targets (NiNew[]'d: the count before the first)
inline MorphInterpItem* NewItems(uint32_t count) {
    uint32_t* block = static_cast<uint32_t*>(NiAllocArray(KeyArraySize(count, sizeof(MorphInterpItem))));
    if (block == nullptr) {
        return nullptr;
    }
    *block = count;
    MorphInterpItem* items = reinterpret_cast<MorphInterpItem*>(block + 1);
    VectorConstruct(items, sizeof(MorphInterpItem), count, reinterpret_cast<void*>(MorphInterpItem_Construct_address));
    return items;
}

// the item array deleted (each interpolator let go, from the last)
inline void DeleteItems(MorphInterpItem* items) {
    uint32_t* block = reinterpret_cast<uint32_t*>(items) - 1;
    for (int32_t i = int32_t(*block) - 1; i >= 0; i--) {
        ReleaseRef(items[i].interpolator);
    }
    NiFreeArray(block, sizeof(MorphInterpItem));
}

// the constructor CreateObject and CreateClone inline
inline void ConstructInline(NiGeomMorpherController* self) {
    InterpController_Construct(self);
    *reinterpret_cast<re::Ptr32<void>*>(self) = reinterpret_cast<void*>(NiGeomMorpherController_vtable);
    self->data = nullptr;
    self->items = nullptr;
    self->morphFlags = 0;
    self->needPreDisplay = 0;
    self->ignoreBaseTarget = 0;
    self->alwaysUpdate = 0;
    self->bound = 0;
}

// ---- the slots shared with other controllers

// 0x8109F0: InterpolatorIsCorrectType(interpolator, index) (slot 39: NiFloatInterpController's,
// shared by the float controllers and this one): one that gives floats (slot 25)
uint8_t __fastcall Float_InterpolatorIsCorrectType(NiTimeController*, re::Edx, NiInterpolator* interpolator, uint16_t) {
    return Virtual<uint8_t>(interpolator, 25);
}

// 0x811A70: CreateBlendInterpolator(index, managerControlled, accumulate, weightThreshold,
// arraySize) (slot 37: NiPoint3InterpController's): a NiBlendPoint3Interpolator
NiInterpolator* __fastcall Point3_CreateBlendInterpolator(NiTimeController*, re::Edx, uint16_t, int managerControlled,
                                                          int, float weightThreshold, int arraySize) {
    void* memory = NiNew(0x40);
    if (memory == nullptr) {
        return nullptr;
    }
    return BlendPoint3Interpolator_Construct(memory, managerControlled, weightThreshold, arraySize);
}

// ---- NiGeomMorpherController

// 0x810A00: GuaranteeTimeRange(start, end) (slot 38): each interpolator's
void __fastcall GM_GuaranteeTimeRange(NiGeomMorpherController* self, re::Edx, float start, float end) {
    for (uint32_t i = 0; i < self->data->numTargets; i++) {
        NiInterpolator* interpolator = Virtual<NiInterpolator*>(self, 31, uint16_t(i));
        if (interpolator != nullptr) {
            Virtual<void>(interpolator, 32, start, end);
        }
    }
}

// 0x810A50: RegisterStreamables: the morph data and the interpolators too
uint8_t __fastcall GM_RegisterStreamables(NiGeomMorpherController* self, re::Edx, void* stream) {
    if (!InterpController_RegisterStreamables(self, stream)) {
        return 0;
    }
    if (self->data != nullptr) {
        Virtual<uint8_t>(self->data, 6, stream);
    }
    for (uint16_t i = 0; i < Virtual<uint16_t>(self, 28); i++) {
        NiInterpolator* interpolator = Virtual<NiInterpolator*>(self, 31, i);
        if (interpolator != nullptr) {
            Virtual<uint8_t>(interpolator, 6, stream);
        }
    }
    return 1;
}

// 0x810AC0: GetInterpolatorID(index) (slot 29): the target's name (past the count, the name of a
// null target read: the game faults there too)
const char* __fastcall GM_GetInterpolatorID(NiGeomMorpherController* self, re::Edx, uint16_t index) {
    return *MorphTarget_GetNameAddress(TargetAt(self->data, index));
}

// 0x810AF0: GetInterpolator(index) (slot 31)
NiInterpolator* __fastcall GM_GetInterpolator(NiGeomMorpherController* self, re::Edx, uint16_t index) {
    return self->items[index].interpolator;
}

// 0x810B00: SetInterpolator(interpolator, index) (slot 32)
void __fastcall GM_SetInterpolator(NiGeomMorpherController* self, re::Edx, NiInterpolator* interpolator,
                                   uint16_t index) {
    AssignReleasingFirst(self->items[index].interpolator, interpolator);
}

// 0x810B50: CreatePoseInterpolator(index) (slot 35): a float interpolator posed at the weight
NiInterpolator* __fastcall GM_CreatePoseInterpolator(NiGeomMorpherController* self, re::Edx, uint16_t index) {
    float weight = 0.0f;
    if (self->items != nullptr) {
        weight = self->items[index].weight;
    }
    void* memory = NiNew(0x18);
    if (memory == nullptr) {
        return nullptr;
    }
    return FloatInterpolator_ConstructPosed(memory, weight);
}

// 0x810B90: the number of targets (none without data)
uint32_t __fastcall GM_GetNumTargets(NiGeomMorpherController* self, re::Edx) {
    if (self->data != nullptr) {
        return self->data->numTargets;
    }
    return 0;
}

// 0x810BA0: ResetTimeExtrema (slot 33): unless the manager controls it, the widest of the
// interpolators' active ranges
void __fastcall GM_ResetTimeExtrema(NiGeomMorpherController* self, re::Edx) {
    if (((uint8_t(self->flags) >> 5) & 1) != 0) {
        return;
    }
    uint32_t count = self->data != nullptr ? self->data->numTargets : 0;
    self->loKeyTime = 3.40282347e+38f;
    self->hiKeyTime = -3.40282347e+38f;
    for (uint32_t i = 0; i < count; i++) {
        NiInterpolator* interpolator = Virtual<NiInterpolator*>(self, 31, uint16_t(i));
        if (interpolator == nullptr) {
            continue;
        }
        float begin = 3.40282347e+38f;
        float end = -3.40282347e+38f;
        Virtual<void>(interpolator, 31, &begin, &end);
        if (begin < self->loKeyTime) {
            self->loKeyTime = begin;
        }
        if (self->hiKeyTime < end) {
            self->hiKeyTime = end;
        }
    }
}

// 0x810C60: Update(time) (slot 20): the pre-display pass marked needed; with tasks on (and the
// task manager running, the target data not shared) the morph handed to a task, done at once when
// the manager won't take it
void __fastcall GM_Update(NiGeomMorpherController* self, re::Edx, float time) {
    if (self->data == nullptr) {
        return;
    }
    if (((uint8_t(self->flags) >> 5) & 1) != 0) {
        self->needPreDisplay = 1;
        self->scaledTime = kInvalidFloat;
    } else if (!TimeController_DontDoUpdate(self, time) || self->alwaysUpdate != 0) {
        self->needPreDisplay = 1;
    }
    if (self->needPreDisplay == 0 || ms_bUseTasks == 0) {
        return;
    }
    uint8_t* manager = static_cast<uint8_t*>(ms_pkTaskManager);
    if (manager == nullptr || manager[0x9C] == 0) {
        return;
    }
    if (TargetData(self)->refCount != 1) {
        return;
    }
    NiObject* task = GeomMorpherUpdateTask_GetFreeObject();
    if (task == nullptr) {
        return;
    }
    GeomMorpherUpdateTask_SetController(task, self);
    if (!Virtual<uint8_t>(manager, 18, task, kTaskPriority)) {
        Virtual<uint8_t>(task, 20);   // Clear
    }
}

// 0x810D10: the morph (the time unused): each target's weight from its interpolator (a relative
// base target's: 1, or 0 when it's ignored), the vertices cleared (unless the base is ignored),
// then each target with a weight past +-0.001 added in
void __fastcall GM_UpdateMorphs(NiGeomMorpherController* self, re::Edx, float) {
    NiPoint3* vertices = Virtual<NiPoint3*>(self, 40);
    uint32_t vertexCount = TargetData(self)->vertexCount;
    uint32_t count = self->data->numTargets;
    for (uint32_t i = 0; i < count; i++) {
        NiInterpolator* interpolator = Virtual<NiInterpolator*>(self, 31, uint16_t(i));
        float weight = 0.0f;
        if (self->items != nullptr) {
            weight = self->items[i].weight;
        }
        if (i == 0 && self->data->relativeTargets != 0) {
            weight = self->ignoreBaseTarget != 0 ? 0.0f : 1.0f;
        } else {
            if (interpolator == nullptr) {
                continue;
            }
            if (!Virtual<uint8_t>(interpolator, 22, self->scaledTime, self->target, &weight)) {
                continue;
            }
        }
        if (self->items != nullptr) {
            self->items[i].weight = weight;
        }
    }
    if (self->ignoreBaseTarget == 0) {
        crt::Memset(vertices, 0, vertexCount * 12);
    }
    for (uint32_t i = 0; i < count; i++) {
        if (self->items == nullptr) {
            continue;
        }
        float weight = self->items[i].weight;
        if (weight >= 0.001f || weight <= -0.001f) {
            Point3_AddScaledArray(vertices, weight, self->data->targets[i].verts, vertexCount);
        }
    }
}

// 0x810E60: SetTarget(target) (slot 21): only a geometry (else none); with morph data the
// bound computed
void __fastcall GM_SetTarget(NiGeomMorpherController* self, re::Edx, NiObjectNET* target) {
    if (target != nullptr && IsKindOf(target, kNiGeometryClassIndex)) {
        TimeController_SetTarget(self, target);
        if (self->data != nullptr) {
            Virtual<void>(self, 41);
            self->bound = 1;
        }
        return;
    }
    TimeController_SetTarget(self, nullptr);
}

// 0x810ED0: OnPreDisplay (slot 25; NiGeomMorpherUpdateTask's work): the morph, the target data's
// vertices marked changed, its normals made again when asked for and it has them
void __fastcall GM_OnPreDisplay(NiGeomMorpherController* self, re::Edx) {
    if (self->target == nullptr || self->needPreDisplay == 0) {
        return;
    }
    GeomMorpher_UpdateMorphs(self, self->scaledTime);
    NiGeometryData* data = TargetData(self);
    data->dirtyFlags |= 1;
    if ((self->morphFlags & 1) != 0 && data->normals != nullptr) {
        Virtual<void>(data, 20);   // CalculateNormals
    }
    self->needPreDisplay = 0;
}

// 0x810F20: ComputeBound (slot 41): the target data's bound, every morph target's merged (the
// origin and no radius without targets)
void __fastcall GM_ComputeBound(NiGeomMorpherController* self, re::Edx) {
    NiMorphData* data = self->data;
    uint32_t count = data->numTargets;
    NiBound bound;
    if (count == 0) {
        bound.center = kZero;
        bound.radius = 0.0f;
    } else {
        Bound_ComputeFromData(&bound, data->numVertsPerTarget, data->targets[0].verts);
        for (uint32_t i = 1; i < count; i++) {
            NiBound other;
            Bound_ComputeFromData(&other, self->data->numVertsPerTarget, TargetAt(self->data, i)->verts);
            Bound_Merge(&bound, &other);
        }
    }
    TargetData(self)->bound = bound;
}

// 0x810FF0: ProcessClone: the interpolators' too
void __fastcall GM_ProcessClone(NiGeomMorpherController* self, re::Edx, void* cloning) {
    TimeController_ProcessClone(self, cloning);
    for (uint16_t i = 0; i < GeomMorpher_GetNumTargets(self); i++) {
        NiInterpolator* interpolator = self->items[i].interpolator;
        if (interpolator != nullptr) {
            Virtual<void>(interpolator, 11, cloning);
        }
    }
}

// 0x811040: LoadBinary: the flags, the data link, always-update; from 10.1.0.104 the
// interpolators (links before 20.1.0.3; then a count of interpolator links and weights)
void __fastcall GM_LoadBinary(NiGeomMorpherController* self, re::Edx, void* stream) {
    InterpController_LoadBinary(self, stream);
    LoadBytes(stream, &self->morphFlags, 2);
    Stream_ReadLinkID(stream);
    uint8_t alwaysUpdate;
    LoadBytes(stream, &alwaysUpdate, 1);
    self->alwaysUpdate = alwaysUpdate != 0 ? 1 : 0;
    uint32_t version = StreamVersion(stream);
    if (version < kVersion_10_1_0_104) {
        return;
    }
    if (version < kVersion_20_1_0_3) {
        Stream_ReadMultipleLinkIDs(stream);
        return;
    }
    uint32_t count;
    LoadBytes(stream, &count, 4);
    self->items = NewItems(count);
    for (uint16_t i = 0; i < count; i++) {
        NiObject* interpolator = Stream_ReadLinkIDObject(stream);
        AssignReleasingFirst(self->items[i].interpolator, interpolator);
        LoadBytes(stream, &self->items[i].weight, 4);
    }
}

// 0x811200: SaveBinary
void __fastcall GM_SaveBinary(NiGeomMorpherController* self, re::Edx, void* stream) {
    InterpController_SaveBinary(self, stream);
    SaveBytes(stream, &self->morphFlags, 2);
    Virtual<void>(stream, 12, self->data);   // SaveLinkID
    uint8_t alwaysUpdate = self->alwaysUpdate;
    SaveBytes(stream, &alwaysUpdate, 1);
    uint32_t count = self->data != nullptr ? self->data->numTargets : 0;
    SaveBytes(stream, &count, 4);
    for (uint32_t i = 0; i < count; i++) {
        Virtual<void>(stream, 12, self->items[i].interpolator);
        SaveBytes(stream, &self->items[i].weight, 4);
    }
}

// 0x811310: IsEqual (the morph data, each interpolator and weight)
uint8_t __fastcall GM_IsEqual(NiGeomMorpherController* self, re::Edx, NiGeomMorpherController* other) {
    if (!InterpController_IsEqual(self, other)) {
        return 0;
    }
    if (!Virtual<uint8_t>(self->data, 8, other->data)) {
        return 0;
    }
    for (uint32_t i = 0; i < GeomMorpher_GetNumTargets(self); i++) {
        MorphInterpItem& theirs = other->items[i];
        NiInterpolator* mine = self->items[i].interpolator;
        if (theirs.interpolator != nullptr) {
            if (mine == nullptr) {
                return 0;
            }
        } else if (mine != nullptr) {
            return 0;
        }
        if (theirs.interpolator != nullptr && !Virtual<uint8_t>(theirs.interpolator, 8, mine)) {
            return 0;
        }
        if (!(theirs.weight == self->items[i].weight)) {
            return 0;
        }
    }
    return 1;
}

// 0x8113D0: GetInterpolatorCount (slot 28)
uint16_t __fastcall GM_GetInterpolatorCount(NiGeomMorpherController* self, re::Edx) {
    if (self->data != nullptr) {
        return uint16_t(self->data->numTargets);
    }
    return 0;
}

// 0x8113E0: GetInterpolatorIndex(id) (slot 30): the target of that name (case ignored)
uint16_t __fastcall GM_GetInterpolatorIndex(NiGeomMorpherController* self, re::Edx, const char* id) {
    if (id == nullptr) {
        return kInvalidIndex;
    }
    for (uint16_t i = 0; i < GeomMorpher_GetNumTargets(self); i++) {
        if (NiStricmp(*MorphTarget_GetNameAddress(TargetAt(self->data, i)), id) == 0) {
            return i;
        }
    }
    return kInvalidIndex;
}

// 0x811460: SynchronizePoseInterpolator(interpolator, index) (slot 36): the float interpolator
// posed at the weight (a null one, or another kind, posed through null as the game has it)
void __fastcall GM_SynchronizePoseInterpolator(NiGeomMorpherController* self, re::Edx, NiInterpolator* interpolator,
                                               uint16_t index) {
    float weight = 0.0f;
    if (self->items != nullptr) {
        weight = self->items[index].weight;
    }
    if (interpolator == nullptr) {
        FloatInterpolator_SetPoseValue(nullptr, weight);
        return;
    }
    NiInterpolator* floatInterpolator = IsKindOf(interpolator, kNiFloatInterpolatorClassIndex) ? interpolator : nullptr;
    FloatInterpolator_SetPoseValue(floatInterpolator, weight);
}

// 0x8114E0: ~NiGeomMorpherController (the data let go and cleared, the items deleted, then the
// data pointer's own destructor)
void __fastcall GM_Destruct(NiGeomMorpherController* self, re::Edx) {
    *reinterpret_cast<re::Ptr32<void>*>(self) = reinterpret_cast<void*>(NiGeomMorpherController_vtable);
    if (self->data != nullptr) {
        ReleaseRef(self->data);
        self->data = nullptr;
    }
    if (self->items != nullptr) {
        DeleteItems(self->items);
    }
    ReleaseRef(self->data);
    TimeController_Destruct(self);
}

const RTTI* __fastcall GM_GetRTTI(NiGeomMorpherController*, re::Edx) {
    return &NiGeomMorpherController_RTTI;
}

// 0x8115B0: TargetIsRequiredType (slot 27): a NiGeometry
uint8_t __fastcall GM_TargetIsRequiredType(NiGeomMorpherController* self, re::Edx) {
    if (self->target == nullptr) {
        return 0;
    }
    return IsKindOf(self->target, kNiGeometryClassIndex);
}

// 0x8115E0: GetTargetVertices (slot 40)
NiPoint3* __fastcall GM_GetTargetVertices(NiGeomMorpherController* self, re::Edx) {
    if (self->target != nullptr) {
        return TargetData(self)->vertices;
    }
    return nullptr;
}

// 0x811600: the item array made again, one per target
void __fastcall GM_AllocateInterpItems(NiGeomMorpherController* self, re::Edx) {
    if (self->items != nullptr) {
        DeleteItems(self->items);
    }
    self->items = nullptr;
    if (self->data == nullptr) {
        return;
    }
    uint32_t count = self->data->numTargets;
    if (count == 0) {
        return;
    }
    self->items = NewItems(count);
}

// 0x8116D0: the deleting destructor
NiGeomMorpherController* __fastcall GM_DeletingDestructor(NiGeomMorpherController* self, re::Edx, uint32_t flags) {
    GeomMorpher_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x44);
    }
    return self;
}

// 0x8116F0: CreateObject
NiObject* __cdecl GM_CreateObject() {
    void* memory = NiNew(0x44);
    if (memory == nullptr) {
        return nullptr;
    }
    ConstructInline(static_cast<NiGeomMorpherController*>(memory));
    return static_cast<NiObject*>(memory);
}

// 0x811740: LinkObject: the morph data (the bound computed when there's a target); before
// 20.1.0.3 the weights from the targets' legacy weights; before 10.1.0.104 the interpolators
// from the targets, then (to 20.1.0.3) from links
void __fastcall GM_LinkObject(NiGeomMorpherController* self, re::Edx, void* stream) {
    InterpController_LinkObject(self, stream);
    NiMorphData* data = static_cast<NiMorphData*>(Stream_ResolveLinkID(stream));
    AssignReleasingFirst(self->data, data);
    if (self->target != nullptr) {
        Virtual<void>(self, 41);
        self->bound = 1;
    }
    if (StreamVersion(stream) < kVersion_20_1_0_3) {
        GeomMorpher_AllocateInterpItems(self);
        for (uint32_t i = 0; i < GeomMorpher_GetNumTargets(self); i++) {
            self->items[i].weight = TargetAt(self->data, i)->legacyWeight;
        }
    }
    uint32_t version = StreamVersion(stream);
    if (version < kVersion_10_1_0_104 && self->data != nullptr) {
        uint32_t count = self->data->numTargets;
        for (uint32_t i = 0; i < count; i++) {
            NiInterpolator* interpolator = MorphTarget_GetInterpolator(TargetAt(self->data, i));
            Virtual<void>(self, 32, interpolator, uint16_t(i));
        }
    } else if (version < kVersion_20_1_0_3) {
        uint32_t count = Stream_GetNumberOfLinkIDs(stream);
        for (uint32_t i = 0; i < count; i++) {
            NiInterpolator* interpolator = static_cast<NiInterpolator*>(Stream_ResolveLinkID(stream));
            Virtual<void>(self, 32, interpolator, uint16_t(i));
        }
    }
}

// 0x811890: SetMorphData(data, updateBound): the items made again for it
void __fastcall GM_SetMorphData(NiGeomMorpherController* self, re::Edx, NiMorphData* data, int updateBound) {
    AssignReleasingFirst(self->data, data);
    GeomMorpher_AllocateInterpItems(self);
    if (re::Flag(updateBound)) {
        Virtual<void>(self, 41);
    }
}

// 0x8118F0: CopyMembers(dest, cloning): the flags, the data (shared), the interpolators'
// shared clones and the weights; always-update set on both when it's on for all
void __fastcall GM_CopyMembers(NiGeomMorpherController* self, re::Edx, NiGeomMorpherController* dest, void* cloning) {
    InterpController_CopyMembers(self, dest, cloning);
    dest->morphFlags = self->morphFlags;
    if (self->data != nullptr) {
        GeomMorpher_SetMorphData(dest, self->data, dest->target != nullptr ? 1 : 0);
    }
    if (ms_bAlwaysUpdate != 0) {
        self->alwaysUpdate = 1;
        dest->alwaysUpdate = 1;
    }
    for (uint16_t i = 0; i < GeomMorpher_GetNumTargets(self); i++) {
        MorphInterpItem& item = self->items[i];
        if (item.interpolator != nullptr) {
            NiObject* clone = Object_CreateSharedClone(item.interpolator, cloning);
            AssignReleasingFirst(dest->items[i].interpolator, clone);
        }
        dest->items[i].weight = item.weight;
    }
}

// 0x811A00: CreateClone (a failed allocation still copied onto null, as the game has it)
NiGeomMorpherController* __fastcall GM_CreateClone(NiGeomMorpherController* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x44);
    if (memory == nullptr) {
        GeomMorpher_CopyMembers(self, nullptr, cloning);
        return nullptr;
    }
    NiGeomMorpherController* clone = static_cast<NiGeomMorpherController*>(memory);
    ConstructInline(clone);
    GeomMorpher_CopyMembers(self, clone, cloning);
    return clone;
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC31_GEOMMORPH_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIC31_GEOMMORPH_REVERSED("NiFloatInterpController::InterpolatorIsCorrectType", 0x008109F0,
                         &ni::anim::Float_InterpolatorIsCorrectType);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::GuaranteeTimeRange", 0x00810A00, &ni::anim::GM_GuaranteeTimeRange);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::RegisterStreamables", 0x00810A50,
                         &ni::anim::GM_RegisterStreamables);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::GetInterpolatorID", 0x00810AC0, &ni::anim::GM_GetInterpolatorID);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::GetInterpolator", 0x00810AF0, &ni::anim::GM_GetInterpolator);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::SetInterpolator", 0x00810B00, &ni::anim::GM_SetInterpolator);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::CreatePoseInterpolator", 0x00810B50,
                         &ni::anim::GM_CreatePoseInterpolator);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::GetNumTargets", 0x00810B90, &ni::anim::GM_GetNumTargets);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::ResetTimeExtrema", 0x00810BA0, &ni::anim::GM_ResetTimeExtrema);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::Update", 0x00810C60, &ni::anim::GM_Update);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::UpdateMorphs", 0x00810D10, &ni::anim::GM_UpdateMorphs);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::SetTarget", 0x00810E60, &ni::anim::GM_SetTarget);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::OnPreDisplay", 0x00810ED0, &ni::anim::GM_OnPreDisplay);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::ComputeBound", 0x00810F20, &ni::anim::GM_ComputeBound);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::ProcessClone", 0x00810FF0, &ni::anim::GM_ProcessClone);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::LoadBinary", 0x00811040, &ni::anim::GM_LoadBinary);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::SaveBinary", 0x00811200, &ni::anim::GM_SaveBinary);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::IsEqual", 0x00811310, &ni::anim::GM_IsEqual);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::GetInterpolatorCount", 0x008113D0,
                         &ni::anim::GM_GetInterpolatorCount);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::GetInterpolatorIndex", 0x008113E0,
                         &ni::anim::GM_GetInterpolatorIndex);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::SynchronizePoseInterpolator", 0x00811460,
                         &ni::anim::GM_SynchronizePoseInterpolator);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::~NiGeomMorpherController", 0x008114E0, &ni::anim::GM_Destruct);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::GetRTTI", 0x008115A0, &ni::anim::GM_GetRTTI);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::TargetIsRequiredType", 0x008115B0,
                         &ni::anim::GM_TargetIsRequiredType);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::GetTargetVertices", 0x008115E0, &ni::anim::GM_GetTargetVertices);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::AllocateInterpItems", 0x00811600,
                         &ni::anim::GM_AllocateInterpItems);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::DeletingDestructor", 0x008116D0,
                         &ni::anim::GM_DeletingDestructor);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::CreateObject", 0x008116F0, &ni::anim::GM_CreateObject);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::LinkObject", 0x00811740, &ni::anim::GM_LinkObject);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::SetMorphData", 0x00811890, &ni::anim::GM_SetMorphData);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::CopyMembers", 0x008118F0, &ni::anim::GM_CopyMembers);
NIC31_GEOMMORPH_REVERSED("NiGeomMorpherController::CreateClone", 0x00811A00, &ni::anim::GM_CreateClone);
NIC31_GEOMMORPH_REVERSED("NiPoint3InterpController::CreateBlendInterpolator", 0x00811A70,
                         &ni::anim::Point3_CreateBlendInterpolator);
