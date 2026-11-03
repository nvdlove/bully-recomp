// The action tree's tracks that play other nodes: NodeReferenceTrack (an embedded
// ActionNodeReference at +0x18 to the node it plays) and its Sequence / Execute / Play /
// TargetPlay / Opportunity (and Struggle, ElseAction, RandomLatch) / Spawn / NISobject
// kinds, ExecuteDoorTrack, and the two motion blur tracks. Their virtuals, type ids,
// destructors and field initialisers. 0x5F7110..0x5F9800 (the collision tracks are in
// CollisionTracks.cpp).
//
// A track's owner is the playing ped's action context: +0 its controller, +8 the ped,
// +0x44 / +0x48 its key stack (see ActionTree.h). Spawn and NIS tracks carry their own
// context (+0x2C, 0x58 bytes) and controller (+0x84).
//
// Arithmetic is float in the game's order (its x87 runs in single precision, D3D9's
// default).

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "game/ActionTree.h"

#include "game/Crt.h"
#include "reversed.h"
using namespace acttree;

namespace {

constexpr uint32_t kVtReference = 0x0092DAAC;
constexpr uint32_t kVtNodeReferenceTrack = 0x0092DE58;
constexpr uint32_t kVtSpawnTrack = 0x0092DE80;
constexpr uint32_t kVtNisTrack = 0x0092DEF0;
constexpr uint32_t kVtMotionBlurTrack = 0x0092DE28;
constexpr uint32_t kVtBodyMotionBlurTrack = 0x0092DE40;

GAME_VAR(int32_t, reResolveTracks_1B64A1C, 0x01B64A1C);   // non-zero: Execute tracks resolved and restarted every update
GAME_VAR(uint32_t, frameCounter_C1A99C, 0x00C1A99C);
GAME_VAR(uint32_t, nodeReferenceTrackIdGuard, 0x01B64A6C);
GAME_VAR(int16_t, nodeReferenceTrackId, 0x01B64A68);
GAME_VAR(uint32_t, nisTrackIdGuard, 0x01B64A74);
GAME_VAR(int16_t, nisTrackId, 0x01B64A70);
GAME_VAR(uint8_t, currentChapter_A147E0, 0x00A147E0);
GAME_ARRAY(int32_t, chapterSeason_A147EC, 0x00A147EC);
GAME_ARRAY(uint8_t*, nisObjects_20C5B68, 0x020C5B68);   // 30 slots
GAME_VAR(double, randMax_900CE8, 0x00900CE8);
GAME_VAR(uint32_t, doorPathA_1B65560, 0x01B65560);   // the door path's name hashes
GAME_VAR(uint32_t, doorPathB_1B65564, 0x01B65564);
GAME_VAR(uint32_t, doorPathC_1B652F4, 0x01B652F4);
inline void* const blurManager_1B850E0 = reinterpret_cast<void*>(0x01B850E0);
inline void* const boneLookup_20C4910 = reinterpret_cast<void*>(0x020C4910);
GAME_ARRAY(uint8_t*, ms_modelInfoPtrs, 0x00C67738);

GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void, __thiscall, NameKey_Destruct, 0x0049A650, void* key);
GAME_FN(void, __thiscall, NameKey_FromValue, 0x005FA870, uint32_t* key, uint32_t value);
GAME_FN(void, __thiscall, NameKey_Copy, 0x005FA880, uint32_t* key, const uint32_t* from);
GAME_FN(void, __thiscall, HashPath_Set, 0x005FA8C0, int32_t* path, int32_t count, uint32_t* hashes);
GAME_FN(void, __stdcall, VectorDestruct, 0x004017C0, void* array, size_t size, int count, void* dtor);
GAME_FN(void, __thiscall, StringRef_Assign, 0x005772F0, uint32_t* field, uint32_t value);
GAME_FN(void, __thiscall, TrackBase_Destruct, 0x0061A8B0, uint8_t* track);
GAME_FN(uint8_t, __cdecl, Track_IsAStatic, 0x004437B0, int id);
GAME_FN(uint8_t, __cdecl, NodeReferenceTrack_IsAStatic, 0x005F7C60, int id);
GAME_FN(void, __thiscall, Reference_ResolveByPath, 0x005F5F50, uint8_t* ref);
GAME_FN(uint8_t*, __thiscall, Reference_Get0C, 0x005F4F90, uint8_t* ref, int32_t key);
GAME_FN(void, __thiscall, Reference_ClearTarget, 0x005F55A0, uint8_t* ref);
GAME_FN(void, __thiscall, Node_Destruct, 0x005F4E50, uint8_t* node);
GAME_FN(uint8_t*, __cdecl, ResolveHashedPath, 0x005F4E60, int32_t* path, uint8_t* from);
GAME_FN(void, __thiscall, Owner_SetNode, 0x005F3830, uint8_t* owner, uint8_t* node);
GAME_FN(void, __thiscall, Owner_PushKey, 0x005F3860, uint8_t* owner, int32_t key);
GAME_FN(void, __thiscall, Context_Assign, 0x005F88D0, uint8_t* context, const uint8_t* from);
GAME_FN(void, __thiscall, Ctl_RequestPlay, 0x005F3D20, uint8_t* controller, uint8_t* node, int priority, int force);
GAME_FN(void, __thiscall, Ctl_QueueTracks, 0x005F3F80, uint8_t* controller, uint8_t* node);
GAME_FN(void, __thiscall, Ctl_Stop, 0x005F40A0, uint8_t* controller);
GAME_FN(void, __thiscall, Ctl_DropTracks, 0x005F40F0, uint8_t* controller);
GAME_FN(void, __thiscall, Ctl_StartTracks, 0x005F4110, uint8_t* controller, float dt, int all);
GAME_FN(void, __thiscall, Ctl_PlayNode, 0x005F45F0, uint8_t* controller, uint8_t* node);
GAME_FN(void, __thiscall, Ctl_EndTrackWithKey, 0x005F4790, uint8_t* controller, int32_t key);
GAME_FN(void, __thiscall, Ctl_End, 0x005F4A10, uint8_t* controller, float dt);
GAME_FN(void, __thiscall, Ctl_Play, 0x005F4A50, uint8_t* controller, uint8_t* node);
GAME_FN(void, __thiscall, Ctl_QueueAllTracks, 0x005F4AC0, uint8_t* controller, uint8_t* node);
GAME_FN(void, __thiscall, Ctl_Update, 0x005F4B00, uint8_t* controller, float dt);
GAME_FN(void, __thiscall, Ctl_QueueNodeAllTracks, 0x005F4BB0, uint8_t* controller, uint8_t* node);
GAME_FN(uint8_t, __thiscall, Conditions_Evaluate, 0x005FBB10, uint8_t* conditions, uint8_t* owner);
GAME_FN(uint8_t*, __thiscall, ActionInputs_GetRequest, 0x005F3720, uint8_t* inputs, int button);
GAME_FN(void, __thiscall, ActionInputs_SetRequest, 0x005F3A90, uint8_t* inputs, int button, int kind, float time);
GAME_FN(uint8_t, __thiscall, Opportunity_Update, 0x005F7ED0, uint8_t* track, float dt);
GAME_FN(uint8_t, __thiscall, Ped_EveryNthFrame, 0x00465A60, uint8_t* ped, uint32_t n);
GAME_FN(void, __thiscall, Ped_SetTargetFlag_474440, 0x00474440, uint8_t* ped, int, int value);
GAME_FN(void, __cdecl, Entity_CleanUpOldReference, 0x004657E0, uint8_t* entity, re::Ptr32<uint8_t>* ref);
GAME_FN(uint8_t, __thiscall, MI_IsDoorModel_520440, 0x00520440, uint8_t* mi);
GAME_FN(int32_t, __thiscall, Blur_Create, 0x0066BB00, void* manager, int length, float width, uint32_t color, int kind);
GAME_FN(void, __thiscall, Blur_SetMode, 0x0066BF20, void* manager, int32_t handle, int mode);
GAME_FN(void, __thiscall, Blur_Release, 0x0066BF40, void* manager, int32_t handle);
GAME_FN(void, __thiscall, Blur_SetOrigin, 0x0066BD80, void* manager, int32_t handle, const float* pos);
struct BlurPoint {
    float x, y, z, w;
};
GAME_FN(void, __thiscall, Blur_AddSegment_x64, 0x0066BF90, void* manager, int32_t handle, float, float, float, float, float, float, float, float);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Blur_AddSegment(void* a0, int32_t a1, BlurPoint a2, BlurPoint a3) {
    return Blur_AddSegment_x64(a0, a1, re::FloatAt(a2, 0), re::FloatAt(a2, 1), re::FloatAt(a2, 2), re::FloatAt(a2, 3), re::FloatAt(a3, 0), re::FloatAt(a3, 1), re::FloatAt(a3, 2), re::FloatAt(a3, 3));
}
GAME_FN(uint8_t*, __cdecl, Clump_GetHierarchy, 0x005EE1A0, void* clump);
GAME_FN(uint8_t*, __cdecl, Hierarchy_GetMatrices, 0x006C7710, uint8_t* hierarchy);
GAME_FN(uint8_t*, __thiscall, BoneTable_Find, 0x00465580, uint8_t* table, uint32_t hash, int32_t* bone);
GAME_FN(void, __thiscall, Bones_GetPosition, 0x006C1190, void* lookup, uint8_t* model, int32_t bone, BlurPoint* out, uintptr_t);
GAME_FN(uint8_t, __cdecl, Model_FindDummy, 0x00612380, BlurPoint* out, const char* name, uint8_t* model);
GAME_FN(void, __thiscall, Ped_BodyBlurSetup, 0x00472EB0, uint8_t* ped, int kind);
GAME_FN(void, __thiscall, Ped_BodyBlurSet, 0x00473890, uint8_t* ped, float blur, float v38, int kind, float high, float low, int count, int32_t* bones);
GAME_FN(void, __thiscall, Ped_BodyBlurEnable, 0x00473970, uint8_t* ped, int on);

uint8_t* Ref(uint8_t* track) {
    return track + 0x18;
}
uint8_t* Controller(uint8_t* owner) {
    return vc::At<uint8_t*>(owner, 0);
}
uint8_t* Ped(uint8_t* owner) {
    return vc::At<uint8_t*>(owner, 8);
}
// the node the track plays for the owner (under its key)
uint8_t* RefNode(uint8_t* track, uint8_t* owner) {
    return Reference_Get0C(Ref(track), OwnerKey(owner));
}
// a track's end time 0 or more: it keeps its node for a duration
uint8_t HasDuration(float end) {
    return end >= 0.0f ? 1 : 0;
}

// 0x5F7110 (thiscall): one of two sub-entries' start (-1: none)
float __fastcall Track_GetSubStart(uint8_t* self, re::Edx, int i) {
    if (i < 2) {
        uint8_t* entry = vc::At<uint8_t*>(self, 0x18 + i * 0x14);
        if (entry) {
            return vc::At<float>(entry, 0xC);
        }
    }
    return -1.0f;
}

// 0x5F7150 (thiscall): NodeReferenceTrack::Resolve: its node found
uint32_t __fastcall NodeReferenceTrack_Resolve(uint8_t* self, re::Edx) {
    Reference_ResolveByPath(Ref(self));
    return Reference_Get0C(Ref(self), -1) != nullptr ? 1 : 0;
}

// 0x5F7170 (thiscall)
void __fastcall NodeReferenceTrack_ClearNode(uint8_t* self, re::Edx) {
    Reference_ClearTarget(Ref(self));
}

// 0x5F7180 (thiscall): SetNode: the reference's parent
void __fastcall NodeReferenceTrack_SetNode(uint8_t* self, re::Edx, uint8_t* node) {
    vc::Call<void>(Ref(self), kNodeSetParent, node);
}

// 0x5F7190 (thiscall): SequenceTrack::End: its node asked to play next
uint8_t __fastcall SequenceTrack_End(uint8_t* self, re::Edx) {
    uint8_t* owner = vc::At<uint8_t*>(self, 0x28);
    uint8_t* controller = Controller(owner);
    uint8_t* node = RefNode(self, owner);
    if (node) {
        uint8_t* playable = vc::Call<uint8_t*>(node, kNodeFindPlayable, vc::At<uint8_t*>(self, 0x28));
        if (playable) {
            Ctl_RequestPlay(controller, playable, 0, 0);
        }
    }
    return 1;
}

// 0x5F71D0 (thiscall): ExecuteTrack::Start: its node's tracks queued on the owner's
// controller
uint8_t __fastcall ExecuteTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    vc::At<uint8_t*>(self, 0x28) = owner;
    vc::At<uint8_t*>(self, 0x2C) = nullptr;
    self[0x30] = 0;
    self[0x31] = 0;
    uint8_t* controller = Controller(owner);
    uint8_t* node = RefNode(self, owner);
    if (node) {
        uint8_t* playable = vc::Call<uint8_t*>(node, kNodeFindPlayable, vc::At<uint8_t*>(self, 0x28));
        vc::At<uint8_t*>(self, 0x2C) = playable;
        if (playable) {
            self[0x31] = HasDuration(TrackEnd(self));
            Ctl_QueueTracks(controller, playable);
        }
    }
    return 1;
}

// 0x5F7250 (thiscall): ExecuteTrack::Update: the queued tracks started once
uint8_t __fastcall ExecuteTrack_Update(uint8_t* self, re::Edx, float dt) {
    uint8_t* owner = vc::At<uint8_t*>(self, 0x28);
    if (reResolveTracks_1B64A1C != 0) {
        currentKey = OwnerKey(owner);
        vc::Call<void>(self, kTrackResolve);
        vc::Call<uint8_t>(self, kTrackStart, vc::At<uint8_t*>(self, 0x28));
    }
    if (!self[0x30]) {
        uint8_t* controller = Controller(vc::At<uint8_t*>(self, 0x28));
        Ctl_StartTracks(controller, dt, controller[0xC]);
        self[0x30] = 1;
    }
    return self[0x31] != 0;
}

// the playable node's tracks ended on the owner's controller
void EndNodeTracks(uint8_t* owner, uint8_t* playable) {
    uint8_t* controller = Controller(owner);
    int count = vc::Call<int>(playable, kNodeNumTracks);
    for (int i = 0; i < count; i++) {
        int32_t key = vc::Call<int32_t>(playable, kNodeGetTrackKey, i);
        if (key) {
            Ctl_EndTrackWithKey(controller, key);
        }
    }
}

// 0x5F72B0 (thiscall): ExecuteTrack::End: with a duration, its node's tracks ended
uint8_t __fastcall ExecuteTrack_End(uint8_t* self, re::Edx) {
    if (vc::At<uint8_t*>(self, 0x2C) && self[0x31]) {
        EndNodeTracks(vc::At<uint8_t*>(self, 0x28), vc::At<uint8_t*>(self, 0x2C));
    }
    return 1;
}

// 0x5F7310 (thiscall): ExecuteDoorTrack::Update
uint8_t __fastcall ExecuteDoorTrack_Update(uint8_t* self, re::Edx, float dt) {
    if (!self[0x20]) {
        uint8_t* controller = Controller(vc::At<uint8_t*>(self, 0x18));
        Ctl_StartTracks(controller, dt, controller[0xC]);
        self[0x20] = 1;
    }
    return self[0x21] != 0;
}

// 0x5F7340 (thiscall): ExecuteDoorTrack::End
uint8_t __fastcall ExecuteDoorTrack_End(uint8_t* self, re::Edx) {
    if (vc::At<uint8_t*>(self, 0x1C) && self[0x21]) {
        EndNodeTracks(vc::At<uint8_t*>(self, 0x18), vc::At<uint8_t*>(self, 0x1C));
    }
    return 1;
}

// 0x5F73A0 (thiscall): the NIS track's fields: kept alive, no name
void __fastcall NisTrack_InitFields(uint8_t* self, re::Edx) {
    self[8] = 1;
    uint32_t name;
    NameKey_FromValue(&name, 0);
    StringRef_Assign(&vc::At<uint32_t>(self, 0x28), name);
    NameKey_Destruct(&name);
}

// 0x5F73D0 (thiscall): NISobjectTrack::Update: its own controller updated while it plays
uint8_t __fastcall NisTrack_Update(uint8_t* self, re::Edx, float dt) {
    if (vc::At<uint8_t*>(self, 0x94)) {
        Ctl_Update(self + 0x84, dt);
        return 1;
    }
    return 0;
}

// 0x5F7400 (thiscall): NISobjectTrack::End: its controller ended and stopped
uint8_t __fastcall NisTrack_End(uint8_t* self, re::Edx) {
    Ctl_End(self + 0x84, 0.0001f);
    Ctl_Stop(self + 0x84);
    return 1;
}

// 0x5F7430 (thiscall): the spawn track's fields
void __fastcall SpawnTrack_InitFields(uint8_t* self, re::Edx) {
    self[8] = 0;
    self[0x28] = 1;
    self[0x29] = 0;
    self[0x2A] = 0;
}

// the spawn track's context plays its node again
void SpawnTrack_Replay(uint8_t* self) {
    uint8_t* context = self + 0x2C;
    uint8_t* node = vc::At<uint8_t*>(self, 0x6C);
    if (node) {
        vc::At<int32_t>(context, 0x44) = 0;
        uint8_t* playable = vc::Call<uint8_t*>(node, kNodeFindPlayable, context);
        Ctl_PlayNode(vc::At<uint8_t*>(context, 0), playable);
    }
}

// 0x5F7440 (thiscall): SpawnTrack::Update: its controller updated (the node replayed
// when it ends and looping, +0x29)
uint8_t __fastcall SpawnTrack_Update(uint8_t* self, re::Edx, float dt) {
    if (vc::At<uint8_t*>(self, 0x94)) {
        Ctl_Update(self + 0x84, dt);
        if (!vc::At<uint8_t*>(self, 0x94) && self[0x29]) {
            SpawnTrack_Replay(self);
        }
        Ctl_StartTracks(self + 0x84, 0.0f, 0);
        return 1;
    }
    if (self[0x29]) {
        SpawnTrack_Replay(self);
        Ctl_Update(self + 0x84, dt);
        return 1;
    }
    return 0;
}

// 0x5F74F0 (thiscall): SpawnTrack::End: the owner's spawn count down, the controller
// ended and stopped
uint8_t __fastcall SpawnTrack_End(uint8_t* self, re::Edx) {
    if (self[0x2A]) {
        uint8_t* owner = vc::At<uint8_t*>(self, 0x80);
        if (owner && vc::At<int32_t>(owner, 0x50) > 0) {
            vc::At<int32_t>(owner, 0x50) -= 1;
        }
    }
    vc::At<uint8_t*>(self, 0x80) = nullptr;
    Ctl_End(self + 0x84, 0.0001f);
    Ctl_Stop(self + 0x84);
    return 1;
}

// 0x5F7540 / 0x5F7550 (thiscall): the play track's fields (queue only / play)
void __fastcall PlayTrack_InitQueue(uint8_t* self, re::Edx) {
    self[8] = 0;
    self[0x28] = 1;
    self[0x29] = 1;
}
void __fastcall PlayTrack_InitPlay(uint8_t* self, re::Edx) {
    self[8] = 0;
    self[0x28] = 0;
    self[0x29] = 1;
}

void OpportunityInit(uint8_t* self) {
    vc::At<float>(self, 0x28) = -1.0f;
    vc::At<int32_t>(self, 0x34) = 0;
    self[0x30] = 0;
    vc::At<float>(self, 0x2C) = 0.0f;
    vc::At<int32_t>(self, 0x3C) = 0;
    self[8] = 0;
    vc::At<int32_t>(self, 0x38) = 0;
}

// 0x5F7560 (thiscall): the opportunity track's fields
void __fastcall OpportunityTrack_InitFields(uint8_t* self, re::Edx) {
    OpportunityInit(self);
}

// 0x5F7580 (thiscall): OpportunityTrack::Start
uint8_t __fastcall OpportunityTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    vc::At<uint8_t*>(self, 0x40) = owner;
    self[0x49] = 0;
    vc::At<uint8_t*>(self, 0x44) = nullptr;
    self[0x48] = 0;
    return 1;
}

// 0x5F75A0 (thiscall): OpportunityTrack::End: the node it found asked to play
uint8_t __fastcall OpportunityTrack_End(uint8_t* self, re::Edx) {
    if (self[0x48]) {
        Ctl_RequestPlay(Controller(vc::At<uint8_t*>(self, 0x40)), vc::At<uint8_t*>(self, 0x44), vc::At<int32_t>(self, 0x34), 1);
    }
    return 1;
}

// 0x5F75C0 (thiscall): the struggle track's fields
void __fastcall StruggleTrack_InitFields(uint8_t* self, re::Edx) {
    OpportunityInit(self);
    self[0x4C] = 0;
    vc::At<int32_t>(self, 0x50) = 3;
    vc::At<int32_t>(self, 0x54) = 3;
}

// 0x5F75F0 (thiscall): OpportunityStruggleTrack::Start
uint8_t __fastcall StruggleTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    OpportunityTrack_Start(self, nullptr, owner);
    vc::At<int32_t>(self, 0x58) = 0;
    return 1;
}

// 0x5F7610 (thiscall): the else-action track's fields
void __fastcall ElseActionTrack_InitFields(uint8_t* self, re::Edx) {
    OpportunityInit(self);
    vc::At<int32_t>(self, 0x4C) = 3;
    vc::At<int32_t>(self, 0x50) = 1;
}

// 0x5F7640 (thiscall): the random latch track's fields
void __fastcall RandomLatchTrack_InitFields(uint8_t* self, re::Edx) {
    OpportunityInit(self);
    vc::At<float>(self, 0x4C) = 0.0f;
    vc::At<float>(self, 0x50) = -1.0f;
}

// 0x5F7670 (thiscall): OpportunityRandomLatchTrack::Start: its window's end picked
// between +0x4C and +0x50
uint8_t __fastcall RandomLatchTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    float high = vc::At<float>(self, 0x50);
    vc::At<uint8_t*>(self, 0x40) = owner;
    float low = vc::At<float>(self, 0x4C);
    self[0x49] = 0;
    vc::At<uint8_t*>(self, 0x44) = nullptr;
    self[0x48] = 0;
    float r = float(double(Rand()) / randMax_900CE8);
    vc::At<float>(self, 0x54) = r * (high - low) + low;
    return 1;
}

// 0x5F76D0 (thiscall): the collision tracks' base fields
void __fastcall BaseCollisionTrack_InitFields(uint8_t* self, re::Edx) {
    vc::At<float>(self, 0x18) = 0.0f;
    self[0x58] = 0;
    self[0x59] = 0;
    vc::At<float>(self, 0x1C) = -1.0f;
    self[8] = 0;
    vc::At<int32_t>(self, 0x24) = 1;
    vc::At<float>(self, 0x30) = 5.0f;
    vc::At<float>(self, 0x34) = 0.0f;
    vc::At<float>(self, 0x3C) = 0.0f;
    vc::At<float>(self, 0x48) = 0.0f;
    vc::At<float>(self, 0x4C) = 0.0f;
    vc::At<float>(self, 0x38) = -1.0f;
    vc::At<float>(self, 0x40) = -1.0f;
    vc::At<float>(self, 0x44) = -1.0f;
    vc::At<float>(self, 0x50) = 20.0f;
    vc::At<float>(self, 0x54) = 0.0f;
}

void ClearName(uint8_t* self, int at) {
    uint32_t name;
    NameKey_FromValue(&name, 0);
    StringRef_Assign(&vc::At<uint32_t>(self, at), name);
    NameKey_Destruct(&name);
}

// 0x5F7800 (thiscall): the motion blur track's fields
void __fastcall MotionBlurTrack_InitFields(uint8_t* self, re::Edx) {
    vc::At<float>(self, 0x20) = 0.5f;
    vc::At<float>(self, 0x24) = 0.5f;
    vc::At<float>(self, 0x28) = 0.5f;
    vc::At<float>(self, 0x2C) = 0.5f;
    vc::At<float>(self, 0x30) = 5.0f;
    vc::At<float>(self, 0x34) = 0.5f;
    ClearName(self, 0x44);
    ClearName(self, 0x48);
    vc::At<int32_t>(self, 0x18) = 0x18;
    vc::At<int32_t>(self, 0x1C) = 0x14;
    ClearName(self, 0x3C);
    ClearName(self, 0x40);
    self[0x38] = 1;
    self[8] = 0;
}

void DestructBlurNames(uint8_t* self) {
    NameKey_Destruct(self + 0x48);
    NameKey_Destruct(self + 0x44);
    NameKey_Destruct(self + 0x40);
    NameKey_Destruct(self + 0x3C);
    TrackBase_Destruct(self);
}

// 0x5F78C0 (thiscall): MotionBlurTrack's deleting destructor
uint8_t* __fastcall MotionBlurTrack_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    vc::At<uint32_t>(self, 0) = kVtMotionBlurTrack;
    DestructBlurNames(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

uint8_t ColorByte(float value) {
    return uint8_t(int32_t(float(double(value) * 255.0)));
}

// 0x5F7910 (thiscall): MotionBlurTrack::Start: a trail made with its colour
uint8_t __fastcall MotionBlurTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    vc::At<uint8_t*>(self, 0x4C) = owner;
    vc::At<int32_t>(self, 0x50) = -1;
    vc::At<int32_t>(self, 0x54) = -1;
    vc::At<int32_t>(self, 0x58) = -1;
    uint32_t color = uint32_t(ColorByte(vc::At<float>(self, 0x20))) | uint32_t(ColorByte(vc::At<float>(self, 0x24))) << 8 |
                     uint32_t(ColorByte(vc::At<float>(self, 0x28))) << 16 | uint32_t(ColorByte(vc::At<float>(self, 0x2C))) << 24;
    float width = vc::At<float>(self, 0x34);
    int length = int(vc::At<float>(self, 0x30));   // (__ftol2)
    int32_t handle = Blur_Create(blurManager_1B850E0, length, width, color, 2);
    vc::At<int32_t>(self, 0x58) = handle;
    Blur_SetMode(blurManager_1B850E0, handle, 5);
    return 1;
}

// 0x5F7A30 (thiscall): MotionBlurTrack::End
uint8_t __fastcall MotionBlurTrack_End(uint8_t* self, re::Edx) {
    Blur_Release(blurManager_1B850E0, vc::At<int32_t>(self, 0x58));
    return 1;
}

// 0x5F7A50 (thiscall): the body motion blur track's fields
void __fastcall BodyMotionBlurTrack_InitFields(uint8_t* self, re::Edx) {
    vc::At<float>(self, 0x2C) = 0.05f;
    vc::At<int32_t>(self, 0x28) = 6;
    vc::At<float>(self, 0x30) = 80.0f;
    vc::At<int32_t>(self, 0x18) = 0x14;
    vc::At<int32_t>(self, 0x1C) = 0x1C;
    vc::At<int32_t>(self, 0x20) = 0x24;
    vc::At<float>(self, 0x34) = 0.0f;
    vc::At<int32_t>(self, 0x24) = 0x24;
    vc::At<float>(self, 0x38) = 0.0f;
    self[8] = 0;
}

// 0x5F7A90 (thiscall): BodyMotionBlurTrack's deleting destructor
uint8_t* __fastcall BodyMotionBlurTrack_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    vc::At<uint32_t>(self, 0) = kVtBodyMotionBlurTrack;
    DestructBlurNames(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x5F7AE0 (thiscall) / 0x5F7C60 (cdecl): NodeReferenceTrack::IsA
uint8_t NodeReferenceTrackIsA(int id) {
    if (int16_t(id) == TypeId(nodeReferenceTrackIdGuard, nodeReferenceTrackId)) {
        return 1;
    }
    if (int16_t(id) == NodeReferenceTrackId()) {
        return 1;
    }
    return Track_IsAStatic(id);
}
uint8_t __fastcall NodeReferenceTrack_IsA(uint8_t*, re::Edx, int id) {
    return NodeReferenceTrackIsA(id);
}
uint8_t __cdecl NodeReferenceTrack_IsAStaticImpl(int id) {
    return NodeReferenceTrackIsA(id);
}

// 0x5F7B60 (thiscall): the node set on the reference (the key -1)
void __fastcall NodeReferenceTrack_SetTarget(uint8_t* self, re::Edx, uint8_t* node, int setParent) {
    vc::Call<void>(Ref(self), kNodeSetTarget, node, -1, setParent);
}

void DestructNodeReferenceTrack(uint8_t* self) {
    vc::At<uint32_t>(self, 0) = kVtNodeReferenceTrack;
    vc::At<uint32_t>(Ref(self), 0) = kVtReference;
    NameKey_Destruct(Ref(self) + 8);
    Node_Destruct(Ref(self));
    TrackBase_Destruct(self);
}

// 0x5F7B80 (thiscall): NodeReferenceTrack's deleting destructor
uint8_t* __fastcall NodeReferenceTrack_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    DestructNodeReferenceTrack(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x5F7BC0 (thiscall): NodeReferenceTrack's destructor
void __fastcall NodeReferenceTrack_Destruct(uint8_t* self, re::Edx) {
    DestructNodeReferenceTrack(self);
}

// the context's two entity references let go
void ReleaseContextRefs(uint8_t* self) {
    re::Ptr32<uint8_t>* refA = &vc::At<uint8_t*>(self, 0x38);
    if (*refA) {
        Entity_CleanUpOldReference(*refA, refA);
    }
    re::Ptr32<uint8_t>* refB = &vc::At<uint8_t*>(self, 0x34);
    if (*refB) {
        Entity_CleanUpOldReference(*refB, refB);
    }
}

// 0x5F7BF0 (thiscall): SpawnTrack's destructor
void __fastcall SpawnTrack_Destruct(uint8_t* self, re::Edx) {
    vc::At<uint32_t>(self, 0) = kVtSpawnTrack;
    Ctl_DropTracks(self + 0x84);
    ReleaseContextRefs(self);
    DestructNodeReferenceTrack(self);
}

// 0x5F7CD0 (thiscall): SpawnTrack::IsA (a controller track)
uint8_t __fastcall SpawnTrack_IsA(uint8_t*, re::Edx, int id) {
    if (int16_t(id) == ControllerTrackId()) {
        return 1;
    }
    return NodeReferenceTrack_IsAStatic(id);
}

// 0x5F7D20 (thiscall): SpawnTrack's deleting destructor
uint8_t* __fastcall SpawnTrack_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    reinterpret_cast<void(__thiscall*)(uint8_t*)>(0x005F7BF0)(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x5F7D40 (thiscall): PlayTrack::Update: its node played (or its tracks queued) on the
// ped's controller
uint8_t __fastcall PlayTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* owner = vc::At<uint8_t*>(self, 0x2C);
    uint8_t* node = RefNode(self, owner);
    if (!node) {
        return 0;
    }
    uint8_t* pedOwner = vc::Call<uint8_t*>(Ped(vc::At<uint8_t*>(self, 0x2C)), 0x40);
    uint8_t* playable = vc::Call<uint8_t*>(node, kNodeFindPlayable, pedOwner);
    if (playable) {
        uint8_t* controller = vc::At<uint8_t*>(Ped(vc::At<uint8_t*>(self, 0x2C)), 0x2E0) + 0x258;
        if (self[0x28]) {
            Ctl_QueueAllTracks(controller, playable);
            return self[0x29] != 0;
        }
        Ctl_PlayNode(controller, playable);
    }
    return self[0x29] != 0;
}

// 0x5F7DD0 (thiscall): TargetPlayTrack::Start: its node played on the ped's target
uint8_t __fastcall TargetPlayTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    vc::At<uint8_t*>(self, 0x2C) = owner;
    uint8_t* node = RefNode(self, owner);
    if (!node) {
        return 0;
    }
    uint8_t* target = vc::At<uint8_t*>(Ped(vc::At<uint8_t*>(self, 0x2C)), 0x155C);
    if (target) {
        Ped_SetTargetFlag_474440(target, 1, self[0x29]);
        uint8_t* actions = vc::At<uint8_t*>(target, 0x2E0);
        int32_t key = OwnerKey(vc::At<uint8_t*>(self, 0x2C));
        if (OwnerKey(actions + 0x300) != key) {
            Owner_PushKey(actions + 0x300, key);
        }
        if (self[0x28]) {
            Ctl_QueueNodeAllTracks(actions + 0x258, node);
            return 1;
        }
        Ctl_Play(actions + 0x258, node);
        Ctl_StartTracks(actions + 0x258, 0.0f, 0);
    }
    return 1;
}

// 0x5F7E80 (thiscall): OpportunityTrack::NumConditions
uint32_t __fastcall OpportunityTrack_NumConditions(uint8_t* self, re::Edx) {
    uint8_t* conditions = vc::At<uint8_t*>(self, 0x3C);
    if (!conditions) {
        return 0;
    }
    return vc::At<uint32_t>(conditions, 0xC);
}

// 0x5F7E90 (thiscall): OpportunityTrack::GetCondition
uint8_t* __fastcall OpportunityTrack_GetCondition(uint8_t* self, re::Edx, int i) {
    uint8_t* conditions = vc::At<uint8_t*>(self, 0x3C);
    if (conditions && i >= 0 && i < vc::At<int32_t>(conditions, 0xC)) {
        if (i < 1) {
            return vc::At<uint8_t*>(conditions, i * 4);
        }
        return vc::At<re::Ptr32<uint8_t>*>(conditions, 4)[i - 1];
    }
    return nullptr;
}

// every Nth frame (N at +0x38): by the ped's own count, else the frame counter
bool OpportunityThisFrame(uint8_t* self) {
    uint32_t n = vc::At<uint32_t>(self, 0x38);
    if (int32_t(n) > 1) {
        uint8_t* owner = vc::At<uint8_t*>(self, 0x40);
        uint8_t result;
        if (owner && Ped(owner)) {
            result = Ped_EveryNthFrame(Ped(owner), n);
        } else {
            result = frameCounter_C1A99C % n == 0;
        }
        if (!result) {
            return false;
        }
    }
    return true;
}

// the opportunity's node found once its conditions hold (+0x48 set, the node at +0x44)
void OpportunityTrigger(uint8_t* self) {
    if (self[0x48]) {
        return;
    }
    uint8_t* conditions = vc::At<uint8_t*>(self, 0x3C);
    if (conditions && !Conditions_Evaluate(conditions, vc::At<uint8_t*>(self, 0x40))) {
        return;
    }
    uint8_t* node = RefNode(self, vc::At<uint8_t*>(self, 0x40));
    if (node) {
        uint8_t* playable = vc::Call<uint8_t*>(node, kNodeFindPlayable, vc::At<uint8_t*>(self, 0x40));
        if (!playable) {
            return;
        }
        if (vc::At<uint8_t*>(Controller(vc::At<uint8_t*>(self, 0x40)), 0x10) == playable && !self[0x30]) {
            return;
        }
        vc::At<uint8_t*>(self, 0x44) = playable;
    } else {
        const char* path = vc::At<const char*>(self, 0x20);
        if (path && *path) {
            return;
        }
    }
    self[0x48] = 1;
}

// 0 once triggered and `window` (0 or more) has passed, or untriggered past the start
uint8_t OpportunityResult(uint8_t* self, float window) {
    float time = vc::At<float>(Controller(vc::At<uint8_t*>(self, 0x40)), 8);
    if (self[0x48]) {
        if (window <= time && window >= 0.0f) {
            return 0;
        }
        return 1;
    }
    float start = vc::At<float>(self, 0x28);
    if (start > 0.0f && time > start) {
        return 0;
    }
    return 1;
}

// 0x5F7ED0 (thiscall): OpportunityTrack::Update
uint8_t __fastcall OpportunityTrack_Update(uint8_t* self, re::Edx, float) {
    if (!OpportunityThisFrame(self)) {
        return 1;
    }
    OpportunityTrigger(self);
    return OpportunityResult(self, vc::At<float>(self, 0x2C));
}

// the node's time past the track's start (0) or not (1)
uint8_t PastStart(uint8_t* self) {
    float start = vc::At<float>(self, 0x28);
    if (!(start >= 0.0f)) {
        return 1;
    }
    float time = vc::At<float>(Controller(vc::At<uint8_t*>(self, 0x40)), 8);
    if (!(time >= start)) {
        return 1;
    }
    return 0;
}

// 0x5F7FF0 (thiscall): OpportunityStruggleTrack::Update: presses of its button counted
// (+0x58) until enough (+0x54)
uint8_t __fastcall StruggleTrack_Update(uint8_t* self, re::Edx, float dt) {
    if (!self[0x4C]) {
        uint8_t* request = ActionInputs_GetRequest(vc::At<uint8_t*>(Ped(vc::At<uint8_t*>(self, 0x40)), 0x2E0), vc::At<int32_t>(self, 0x50));
        if (request && (request[0] & 1)) {
            vc::At<int32_t>(self, 0x58) += 1;
        }
    }
    if (vc::At<int32_t>(self, 0x58) >= vc::At<int32_t>(self, 0x54)) {
        return Opportunity_Update(self, dt);
    }
    return PastStart(self);
}

// 0x5F8070 (thiscall): OpportunityElseActionTrack::Update: without a node found, its
// button requested instead
uint8_t __fastcall ElseActionTrack_Update(uint8_t* self, re::Edx, float dt) {
    Opportunity_Update(self, dt);
    if (vc::At<uint8_t*>(self, 0x44)) {
        return 0;
    }
    ActionInputs_SetRequest(vc::At<uint8_t*>(Ped(vc::At<uint8_t*>(self, 0x40)), 0x2E0), vc::At<int32_t>(self, 0x4C),
                            vc::At<int32_t>(self, 0x50), 0.0f);
    return PastStart(self);
}

// 0x5F80E0 (thiscall): OpportunityRandomLatchTrack::Update (its window the random one)
uint8_t __fastcall RandomLatchTrack_Update(uint8_t* self, re::Edx, float) {
    if (!OpportunityThisFrame(self)) {
        return 1;
    }
    OpportunityTrigger(self);
    return OpportunityResult(self, vc::At<float>(self, 0x54));
}

// 0x5F8290 (thiscall): the collision tracks' IsA (node reference tracks)
uint8_t __fastcall CollisionTracks_IsA(uint8_t*, re::Edx, int id) {
    if (int16_t(id) == NodeReferenceTrackId()) {
        return 1;
    }
    return Track_IsAStatic(id);
}

// a named point on the ped (a bone by name, or a model dummy) or the bone's matrix row
bool BlurPointOf(uint8_t* self, const char* name, int32_t bone, uint8_t* hierarchy, BlurPoint* out, bool* found) {
    uint8_t* ped = Ped(vc::At<uint8_t*>(self, 0x4C));
    if (!name || !*name) {
        uint8_t* matrices = Hierarchy_GetMatrices(hierarchy);
        *out = *reinterpret_cast<BlurPoint*>(matrices + bone * 0x34 + 0x24);
        return true;
    }
    uint8_t* model = vc::At<uint8_t*>(ped, 0x114);
    int32_t index = 0;
    if (BoneTable_Find(vc::At<uint8_t*>(model, 0x94), HashName(name), &index)) {
        Bones_GetPosition(boneLookup_20C4910, vc::At<uint8_t*>(Ped(vc::At<uint8_t*>(self, 0x4C)), 0x114), index, out, 0);
        return true;
    }
    *found = Model_FindDummy(out, name, vc::At<uint8_t*>(Ped(vc::At<uint8_t*>(self, 0x4C)), 0x114)) != 0;
    return *found;
}

// 0x5F83E0 (thiscall): MotionBlurTrack::Update: the trail's next segment between its
// two points (bones +0x18 / +0x1C, or named points +0x3C / +0x40)
uint8_t __fastcall MotionBlurTrack_Update(uint8_t* self, re::Edx, float) {
    int32_t boneA = vc::At<int32_t>(self, 0x18);
    int32_t boneB = vc::At<int32_t>(self, 0x1C);
    uint8_t* hierarchy = Clump_GetHierarchy(vc::At<void*>(Ped(vc::At<uint8_t*>(self, 0x4C)), 0x18));
    const char* nameA = vc::At<const char*>(self, 0x3C);
    const char* nameB = vc::At<const char*>(self, 0x40);
    bool valid = true;
    BlurPoint a{}, b{};   // (their fourth floats are uninitialised stack in the game)
    bool foundA = true;
    BlurPointOf(self, nameA, boneA, hierarchy, &a, &foundA);
    if (!foundA) {
        valid = false;
    }
    bool foundB = true;
    BlurPointOf(self, nameB, boneB, hierarchy, &b, &foundB);
    if (!foundB) {
        return 1;
    }
    if (valid) {
        if (!self[0x38]) {
            uint8_t* ped = Ped(vc::At<uint8_t*>(self, 0x4C));
            uint8_t* matrix = vc::At<uint8_t*>(ped, 0x14);
            const float* pos = matrix ? reinterpret_cast<float*>(matrix + 0x30) : reinterpret_cast<float*>(ped + 4);
            Blur_SetOrigin(blurManager_1B850E0, vc::At<int32_t>(self, 0x58), pos);
        }
        Blur_AddSegment(blurManager_1B850E0, vc::At<int32_t>(self, 0x58), a, b);
    }
    return 1;
}

float Clamp100(float v) {
    if (v > 100.0f) {
        return 100.0f;
    }
    if (v < 0.0f) {
        return 0.0f;
    }
    return v;
}

// 0x5F8610 (thiscall): BodyMotionBlurTrack::Start: the ped's body blur on its bones
// (sorted; 0x23 among them: none)
uint8_t __fastcall BodyMotionBlurTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    float blur = vc::At<float>(self, 0x2C);
    vc::At<uint8_t*>(self, 0x4C) = owner;
    float v38 = vc::At<float>(self, 0x38);
    float high = Clamp100(vc::At<float>(self, 0x30));
    float low = Clamp100(vc::At<float>(self, 0x34));
    if (low > high) {
        float t = high;
        high = low;
        low = t;
    }
    int32_t kind = vc::At<int32_t>(self, 0x28);
    int32_t bones[4] = {vc::At<int32_t>(self, 0x18), vc::At<int32_t>(self, 0x1C), vc::At<int32_t>(self, 0x20), vc::At<int32_t>(self, 0x24)};
    for (int i = 0; i < 3; i++) {
        for (int j = i + 1; j < 4; j++) {
            if (bones[i] > bones[j]) {
                int32_t t = bones[i];
                bones[i] = bones[j];
                bones[j] = t;
            }
        }
    }
    int count = 0;
    for (int i = 0; i < 4; i++) {
        if (bones[i] < 0x23) {
            count++;
        }
    }
    for (int i = 0; i < 4; i++) {
        if (bones[i] == 0x23) {
            count = 0;
            bones[0] = bones[1] = bones[2] = bones[3] = 0;
            break;
        }
    }
    uint8_t* ped = Ped(owner);
    Ped_BodyBlurSetup(ped, kind);
    Ped_BodyBlurSet(ped, blur, v38, kind, high, low, count, bones);
    Ped_BodyBlurEnable(ped, 1);
    return 1;
}

// 0x5F87E0 (thiscall): BodyMotionBlurTrack::End
uint8_t __fastcall BodyMotionBlurTrack_End(uint8_t* self, re::Edx) {
    Ped_BodyBlurEnable(Ped(vc::At<uint8_t*>(self, 0x4C)), 0);
    return 1;
}

// 0x5F87F0 (thiscall): NISobjectTrack's destructor
void __fastcall NisTrack_Destruct(uint8_t* self, re::Edx) {
    vc::At<uint32_t>(self, 0) = kVtNisTrack;
    Ctl_DropTracks(self + 0x84);
    ReleaseContextRefs(self);
    NameKey_Destruct(self + 0x28);
    DestructNodeReferenceTrack(self);
}

// 0x5F8860 (thiscall): NISobjectTrack::IsA
uint8_t __fastcall NisTrack_IsA(uint8_t*, re::Edx, int id) {
    if (int16_t(id) == TypeId(nisTrackIdGuard, nisTrackId)) {
        return 1;
    }
    return NodeReferenceTrack_IsAStatic(id);
}

// 0x5F88B0 (thiscall): NISobjectTrack's deleting destructor
uint8_t* __fastcall NisTrack_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    reinterpret_cast<void(__thiscall*)(uint8_t*)>(0x005F87F0)(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// an entity reference slot set (the old one let go, the new one registered)
void SetEntityRef(re::Ptr32<uint8_t>* slot, uint8_t* entity) {
    if (entity == *slot) {
        return;
    }
    if (*slot) {
        Entity_CleanUpOldReference(*slot, slot);
    }
    *slot = entity;
    if (entity) {
        reinterpret_cast<void(__cdecl*)(uint8_t*, re::Ptr32<uint8_t>*)>(0x004657D0)(entity, slot);
    }
}

// a float moved through the x87 (fld / fstp): a signalling NaN comes out quiet
uint32_t X87FloatCopy(uint32_t bits) {
    if ((bits & 0x7F800000) == 0x7F800000 && (bits & 0x7FFFFF) != 0) {
        bits |= 0x400000;
    }
    return bits;
}

// 0x5F88D0 (thiscall): an action context copied (its two entity references kept
// registered)
uint8_t* __fastcall Context_AssignImpl(uint8_t* self, re::Edx, const uint8_t* from) {
    vc::At<uint32_t>(self, 0) = vc::At<uint32_t>(const_cast<uint8_t*>(from), 0);
    vc::At<uint32_t>(self, 4) = vc::At<uint32_t>(const_cast<uint8_t*>(from), 4);
    SetEntityRef(&vc::At<uint8_t*>(self, 8), vc::At<uint8_t*>(const_cast<uint8_t*>(from), 8));
    SetEntityRef(&vc::At<uint8_t*>(self, 0xC), vc::At<uint8_t*>(const_cast<uint8_t*>(from), 0xC));
    std::memcpy(self + 0x10, from + 0x10, 0xC);
    self[0x1C] = from[0x1C];
    self[0x1D] = from[0x1D];
    std::memcpy(self + 0x20, from + 0x20, 4);
    for (int at = 0x24; at <= 0x30; at += 4) {
        vc::At<uint32_t>(self, at) = X87FloatCopy(vc::At<uint32_t>(const_cast<uint8_t*>(from), at));
    }
    std::memcpy(self + 0x34, from + 0x34, 8);
    self[0x3C] = from[0x3C];
    std::memcpy(self + 0x40, from + 0x40, 0x18);
    return self;
}

// the spawn / NIS track's context started from the owner's, its controller playing the
// track's node
void StartOwnContext(uint8_t* self, uint8_t* owner) {
    uint8_t* context = self + 0x2C;
    Context_Assign(context, owner);
    vc::At<uint8_t*>(context, 0) = self + 0x84;
    vc::At<uint8_t*>(self, 0x9C) = context;
}
void PlayOwnNode(uint8_t* self) {
    uint8_t* context = self + 0x2C;
    Owner_SetNode(context, Reference_Get0C(Ref(self), OwnerKey(context)));
    uint8_t* node = vc::At<uint8_t*>(context, 0x40);
    if (node) {
        vc::At<int32_t>(context, 0x44) = 0;
        uint8_t* playable = vc::Call<uint8_t*>(node, kNodeFindPlayable, context);
        Ctl_PlayNode(vc::At<uint8_t*>(context, 0), playable);
    }
}

// 0x5F89C0 (thiscall): SpawnTrack::Start
uint8_t __fastcall SpawnTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    if (self[0x2A]) {
        vc::At<int32_t>(owner, 0x50) += 1;
    }
    vc::At<uint8_t*>(self, 0x80) = owner;
    StartOwnContext(self, owner);
    PlayOwnNode(self);
    if (self[0x28]) {
        vc::Call<uint8_t>(self, kTrackUpdate, 0.0001f);
    }
    return 1;
}

// 0x5F9670 (thiscall): ExecuteDoorTrack::Start: the door node for the prop the ped
// uses (by path, else the path without the prop's name) queued on its controller
uint8_t __fastcall ExecuteDoorTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    vc::At<uint8_t*>(self, 0x18) = owner;
    vc::At<uint8_t*>(self, 0x1C) = nullptr;
    self[0x20] = 0;
    self[0x21] = 0;
    uint8_t* object = vc::At<uint8_t*>(Ped(owner), 0x438);
    if (!object || (object[0x108] & 7) != 6 || !MI_IsDoorModel_520440(ms_modelInfoPtrs[vc::At<int16_t>(object, 0x10E)])) {
        return 1;
    }
    uint8_t* prop = vc::At<uint8_t*>(object, 0x23C);
    uint32_t hashes[4];
    NameKey_Copy(&hashes[0], &doorPathA_1B65560);
    NameKey_Copy(&hashes[1], &doorPathA_1B65560);
    NameKey_Copy(&hashes[2], &doorPathB_1B65564);
    NameKey_Copy(&hashes[3], &doorPathC_1B652F4);
    hashes[3] = vc::At<uint32_t>(prop, 0x2C);
    int32_t path[2];
    HashPath_Set(path, 4, hashes);
    uint8_t* node = ResolveHashedPath(path, nullptr);
    if (!node) {
        uint32_t fallback[4];
        NameKey_Copy(&fallback[0], &doorPathA_1B65560);
        NameKey_Copy(&fallback[1], &doorPathA_1B65560);
        NameKey_Copy(&fallback[2], &doorPathB_1B65564);
        NameKey_Copy(&fallback[3], &doorPathC_1B652F4);
        int32_t path2[2];
        HashPath_Set(path2, 4, fallback);
        node = ResolveHashedPath(path2, nullptr);
        VectorDestruct(fallback, 4, 4, reinterpret_cast<void*>(0x0049A650));
    }
    uint8_t* playable = vc::Call<uint8_t*>(node, kNodeFindPlayable, vc::At<uint8_t*>(self, 0x18));
    vc::At<uint8_t*>(self, 0x1C) = playable;
    if (playable) {
        self[0x21] = HasDuration(TrackEnd(self));
        Ctl_QueueTracks(Controller(vc::At<uint8_t*>(self, 0x18)), playable);
    }
    VectorDestruct(hashes, 4, 4, reinterpret_cast<void*>(0x0049A650));
    return 1;
}

// 0x5F9800 (thiscall): NISobjectTrack::Start: its object found among the NIS objects by
// name (in winter also "<name>_W"), its node played on its own controller
uint8_t __fastcall NisTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    StartOwnContext(self, owner);
    uint32_t winterHash = 0xFFFFFFFF;
    if (chapterSeason_A147EC[currentChapter_A147E0] == 2) {
        char name[0x100];
        std::strcpy(name, vc::At<const char*>(self, 0x28));
        std::strcat(name, "_W");
        winterHash = HashName(name);
    }
    uint32_t hash = HashName(vc::At<const char*>(self, 0x28));
    for (re::Ptr32<uint8_t>* slot = nisObjects_20C5B68; slot < reinterpret_cast<re::Ptr32<uint8_t>*>(0x020C5BE0); slot++) {
        if (!*slot) {
            continue;
        }
        uint8_t* model = vc::At<uint8_t*>(*slot, 0x114);
        uint32_t name = vc::At<uint32_t>(vc::At<uint8_t*>(model, 0x94), 0x14);
        if (name == hash || name == winterHash) {
            vc::At<uint8_t*>(self, 0x30) = model;
            break;
        }
    }
    PlayOwnNode(self);
    return 1;
}

} // namespace

#define ACTTRACKS_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

ACTTRACKS_REVERSED("Track::GetSubStart_5F7110", 0x005F7110, &Track_GetSubStart);
ACTTRACKS_REVERSED("NodeReferenceTrack::Resolve", 0x005F7150, &NodeReferenceTrack_Resolve);
ACTTRACKS_REVERSED("NodeReferenceTrack::ClearNode", 0x005F7170, &NodeReferenceTrack_ClearNode);
ACTTRACKS_REVERSED("NodeReferenceTrack::SetNode", 0x005F7180, &NodeReferenceTrack_SetNode);
ACTTRACKS_REVERSED("SequenceTrack::End", 0x005F7190, &SequenceTrack_End);
ACTTRACKS_REVERSED("ExecuteTrack::Start", 0x005F71D0, &ExecuteTrack_Start);
ACTTRACKS_REVERSED("ExecuteTrack::Update", 0x005F7250, &ExecuteTrack_Update);
ACTTRACKS_REVERSED("ExecuteTrack::End", 0x005F72B0, &ExecuteTrack_End);
ACTTRACKS_REVERSED("ExecuteDoorTrack::Update", 0x005F7310, &ExecuteDoorTrack_Update);
ACTTRACKS_REVERSED("ExecuteDoorTrack::End", 0x005F7340, &ExecuteDoorTrack_End);
ACTTRACKS_REVERSED("NISobjectTrack::InitFields", 0x005F73A0, &NisTrack_InitFields);
ACTTRACKS_REVERSED("NISobjectTrack::Update", 0x005F73D0, &NisTrack_Update);
ACTTRACKS_REVERSED("NISobjectTrack::End", 0x005F7400, &NisTrack_End);
ACTTRACKS_REVERSED("SpawnTrack::InitFields", 0x005F7430, &SpawnTrack_InitFields);
ACTTRACKS_REVERSED("SpawnTrack::Update", 0x005F7440, &SpawnTrack_Update);
ACTTRACKS_REVERSED("SpawnTrack::End", 0x005F74F0, &SpawnTrack_End);
ACTTRACKS_REVERSED("PlayTrack::InitQueue", 0x005F7540, &PlayTrack_InitQueue);
ACTTRACKS_REVERSED("PlayTrack::InitPlay", 0x005F7550, &PlayTrack_InitPlay);
ACTTRACKS_REVERSED("OpportunityTrack::InitFields", 0x005F7560, &OpportunityTrack_InitFields);
ACTTRACKS_REVERSED("OpportunityTrack::Start", 0x005F7580, &OpportunityTrack_Start);
ACTTRACKS_REVERSED("OpportunityTrack::End", 0x005F75A0, &OpportunityTrack_End);
ACTTRACKS_REVERSED("OpportunityStruggleTrack::InitFields", 0x005F75C0, &StruggleTrack_InitFields);
ACTTRACKS_REVERSED("OpportunityStruggleTrack::Start", 0x005F75F0, &StruggleTrack_Start);
ACTTRACKS_REVERSED("OpportunityElseActionTrack::InitFields", 0x005F7610, &ElseActionTrack_InitFields);
ACTTRACKS_REVERSED("OpportunityRandomLatchTrack::InitFields", 0x005F7640, &RandomLatchTrack_InitFields);
ACTTRACKS_REVERSED("OpportunityRandomLatchTrack::Start", 0x005F7670, &RandomLatchTrack_Start);
ACTTRACKS_REVERSED("BaseCollisionTrack::InitFields", 0x005F76D0, &BaseCollisionTrack_InitFields);
ACTTRACKS_REVERSED("MotionBlurTrack::InitFields", 0x005F7800, &MotionBlurTrack_InitFields);
ACTTRACKS_REVERSED("MotionBlurTrack::DeletingDestruct", 0x005F78C0, &MotionBlurTrack_DeletingDestruct);
ACTTRACKS_REVERSED("MotionBlurTrack::Start", 0x005F7910, &MotionBlurTrack_Start);
ACTTRACKS_REVERSED("MotionBlurTrack::End", 0x005F7A30, &MotionBlurTrack_End);
ACTTRACKS_REVERSED("BodyMotionBlurTrack::InitFields", 0x005F7A50, &BodyMotionBlurTrack_InitFields);
ACTTRACKS_REVERSED("BodyMotionBlurTrack::DeletingDestruct", 0x005F7A90, &BodyMotionBlurTrack_DeletingDestruct);
ACTTRACKS_REVERSED("NodeReferenceTrack::IsA", 0x005F7AE0, &NodeReferenceTrack_IsA);
ACTTRACKS_REVERSED("NodeReferenceTrack::SetTarget", 0x005F7B60, &NodeReferenceTrack_SetTarget);
ACTTRACKS_REVERSED("NodeReferenceTrack::DeletingDestruct", 0x005F7B80, &NodeReferenceTrack_DeletingDestruct);
ACTTRACKS_REVERSED("NodeReferenceTrack::Destruct", 0x005F7BC0, &NodeReferenceTrack_Destruct);
ACTTRACKS_REVERSED("SpawnTrack::Destruct", 0x005F7BF0, &SpawnTrack_Destruct);
ACTTRACKS_REVERSED("NodeReferenceTrack::IsAStatic", 0x005F7C60, &NodeReferenceTrack_IsAStaticImpl);
ACTTRACKS_REVERSED("SpawnTrack::IsA", 0x005F7CD0, &SpawnTrack_IsA);
ACTTRACKS_REVERSED("SpawnTrack::DeletingDestruct", 0x005F7D20, &SpawnTrack_DeletingDestruct);
ACTTRACKS_REVERSED("PlayTrack::Update", 0x005F7D40, &PlayTrack_Update);
ACTTRACKS_REVERSED("TargetPlayTrack::Start", 0x005F7DD0, &TargetPlayTrack_Start);
ACTTRACKS_REVERSED("OpportunityTrack::NumConditions", 0x005F7E80, &OpportunityTrack_NumConditions);
ACTTRACKS_REVERSED("OpportunityTrack::GetCondition", 0x005F7E90, &OpportunityTrack_GetCondition);
ACTTRACKS_REVERSED("OpportunityTrack::Update", 0x005F7ED0, &OpportunityTrack_Update);
ACTTRACKS_REVERSED("OpportunityStruggleTrack::Update", 0x005F7FF0, &StruggleTrack_Update);
ACTTRACKS_REVERSED("OpportunityElseActionTrack::Update", 0x005F8070, &ElseActionTrack_Update);
ACTTRACKS_REVERSED("OpportunityRandomLatchTrack::Update", 0x005F80E0, &RandomLatchTrack_Update);
ACTTRACKS_REVERSED("BaseCollisionTrack::IsA", 0x005F8290, &CollisionTracks_IsA);
ACTTRACKS_REVERSED("MotionBlurTrack::Update", 0x005F83E0, &MotionBlurTrack_Update);
ACTTRACKS_REVERSED("BodyMotionBlurTrack::Start", 0x005F8610, &BodyMotionBlurTrack_Start);
ACTTRACKS_REVERSED("BodyMotionBlurTrack::End", 0x005F87E0, &BodyMotionBlurTrack_End);
ACTTRACKS_REVERSED("NISobjectTrack::Destruct", 0x005F87F0, &NisTrack_Destruct);
ACTTRACKS_REVERSED("NISobjectTrack::IsA", 0x005F8860, &NisTrack_IsA);
ACTTRACKS_REVERSED("NISobjectTrack::DeletingDestruct", 0x005F88B0, &NisTrack_DeletingDestruct);
ACTTRACKS_REVERSED("ActionContext::Assign", 0x005F88D0, &Context_AssignImpl);
ACTTRACKS_REVERSED("SpawnTrack::Start", 0x005F89C0, &SpawnTrack_Start);
ACTTRACKS_REVERSED("ExecuteDoorTrack::Start", 0x005F9670, &ExecuteDoorTrack_Start);
ACTTRACKS_REVERSED("NISobjectTrack::Start", 0x005F9800, &NisTrack_Start);
