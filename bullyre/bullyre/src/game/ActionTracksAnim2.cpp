// The action tree's animation tracks (continued): the base AnimationTrack's Resolve /
// Update / End, animation group release, hit reactions, the joint driver's attach and
// end, the steer / target sync / parametric controller drivers, the spud cannon's
// sounds, the door and vehicle animation tracks; and the node lookup the effect tracks
// use. 0x60ADA0..0x60C130.

#include <cmath>
#include <cstdint>

#include "game/ActionTracksCommon.h"
#include "reversed.h"

using namespace actc;

namespace {

constexpr uint32_t kVtLocomotionTrack = 0x00930EC4;
constexpr uint32_t kVtLocomotionAnimationNewTrack = 0x00930EDC;
constexpr uint32_t kVtParametricTrackerTrack = 0x00930FC4;

inline void* const conversations = reinterpret_cast<void*>(0x00B9DAB8);
inline void* const sounds_CF3DA0 = reinterpret_cast<void*>(0x00CF3DA0);
GAME_VAR(uint8_t*, loopSounds_CF4274, 0x00CF4274);
GAME_VAR(float, lookInputA_C3CD8C, 0x00C3CD8C);
GAME_VAR(float, lookInputB_C3CD90, 0x00C3CD90);
GAME_VAR(int32_t, trackerCount_1B65030, 0x01B65030);
GAME_VAR(int32_t, trackerTurn_1B65034, 0x01B65034);
GAME_VAR(uint8_t, trackerSaved_1B6502C, 0x01B6502C);
GAME_VAR(uint8_t, parametricStatic_B0B134, 0x00B0B134);
inline const char* const spudFire = reinterpret_cast<const char*>(0x00931670);      // "SPUDCANNONFIRE3"
inline const char* const spudDryFire = reinterpret_cast<const char*>(0x00931660);   // "SpudCanDryFire"
inline const char* const spudMisfire = reinterpret_cast<const char*>(0x00931650);   // "SpudCanMisfire"

GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(uint8_t*, __thiscall, Skeleton_FindBone, 0x00465580, void* skeleton, uint32_t hash, int32_t* bone);
GAME_FN(uint8_t*, __thiscall, AnimGroup_Find, 0x0042E350, uint8_t* table, uintptr_t name, uint32_t* out);
GAME_FN(int, __cdecl, AnimGroup_StreamId, 0x0052DBD0, uint8_t* table, uint32_t group);
GAME_FN(uint8_t, __cdecl, Streaming_IsLoaded, 0x004151E0, int id);
GAME_FN(uint8_t, __cdecl, Streaming_IsRequested, 0x004151C0, int id);
GAME_FN(uint8_t, __cdecl, Streaming_IsLoading, 0x00450C00, int id);
GAME_FN(void, __thiscall, Ped_RemoveAnimGroupRef, 0x00473A50, uint8_t* ped, uint32_t group);
GAME_FN(uint8_t*, __thiscall, Blend_GetAnim, 0x006B1F50, uint8_t* blend, uint32_t name);
GAME_FN(int32_t, __thiscall, Ped_GetStat, 0x00477060, uint8_t* ped, int32_t stat);
GAME_FN(void, __thiscall, AnimManager_SetSpeed, 0x006C0E60, void* manager, uint8_t* blend, uint8_t* anim, int,
        float speed);
GAME_FN(void, __thiscall, AnimManager_GetTime, 0x006C0F30, void* manager, uint8_t* blend, uint8_t* anim, int,
        float* out);
GAME_FN(void, __thiscall, AnimManager_Update, 0x006C0DC0, void* manager, uint8_t* blend, uint8_t* anim, int);
GAME_FN(void, __thiscall, AnimManager_BlendOut, 0x006BD480, void* manager, uint8_t* blend, uint8_t* anim,
        float time);
GAME_FN(uint8_t*, __thiscall, Conversations_GetSpeaker, 0x006A29F0, void* conversations, int32_t which);
GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_FN(uint8_t*, __cdecl, UsePoint_GetEntity, 0x004EABB0, uint8_t* owner);
GAME_FN(float, __cdecl, RemapRange, 0x0072F500, float value, float from, float to, float outFrom, float outTo);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int);
GAME_FN(int32_t, __thiscall, Sounds_GetId, 0x00595B80, void* sounds, const char* name);
GAME_FN(void, __thiscall, LoopSounds_Stop, 0x00592400, uint8_t* loops, uint8_t* entity, uint8_t* entity2, int32_t id);
GAME_FN(int32_t, __thiscall, DoorControl_StateFor, 0x006080C0, uint8_t* track, int32_t control);

// 0x60ADD0 (thiscall): LocomotionAnimationNewTrack's deleting destructor
// (the plain ones use TrackDeletingDtor)

// 0x60AE00 (cdecl): the first of a blend's animation objects (its +0xB0 list, +0xC4 its
// count) whose skeleton has a node named name: that object and the node's index
uint8_t __cdecl FindAnimNode(re::Ptr32<uint8_t>* clump, int32_t* index, const char* name, uint8_t* blend) {
    if (!name || !blend) {
        return 0;
    }
    uint8_t* item = At<int32_t>(blend, 0xC4) != 0 ? *At<re::Ptr32<uint8_t>*>(blend, 0xB0) : nullptr;
    uint8_t found = 0;
    while (item) {
        if (found) {
            break;
        }
        uint8_t* object = At<uint8_t*>(item, 0x14);
        if (object) {
            int32_t node = 0;
            if (Skeleton_FindBone(At<void*>(object, 0x94), HashName(name), &node)) {
                *clump = At<uint8_t*>(item, 0x14);
                *index = node;
                found = 1;
            }
        }
        uint8_t* link = At<uint8_t*>(item, 0x10);
        if (!link || !At<re::Ptr32<uint8_t>*>(link, 8)) {
            break;
        }
        item = *At<re::Ptr32<uint8_t>*>(link, 8);
    }
    return found;
}

// 0x60AEA0 (thiscall): AnimationGroupReleaseTrack::Start: the ped's reference to group
// +0x18 dropped once it's streamed in and settled
uint8_t __fastcall AnimationGroupReleaseTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    if (At<uint32_t>(self, 0x18) == 0) {
        return 0;
    }
    uint8_t* ped = OwnerPed(owner);
    if (!ped || !At<uint8_t*>(ped, 0x114)) {
        return 1;
    }
    uint8_t* table = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(ped, 0x114), 0x94), 0x10);
    if (!table) {
        return 1;
    }
    uint32_t group;
    if (!AnimGroup_Find(table, At<uint32_t>(self, 0x18), &group)) {
        return 1;
    }
    int id = AnimGroup_StreamId(table, group);
    if (Streaming_IsLoaded(id) && !Streaming_IsRequested(id) && !Streaming_IsLoading(id)) {
        Ped_RemoveAnimGroupRef(OwnerPed(At<uint8_t*>(self, 0x1C)), group);
    }
    return 1;
}

// 0x60AF30 (thiscall): AnimationTrack::Resolve: the blend of the ped (else the owner's
// +0xC entity)
void __fastcall AnimationTrack_Resolve(uint8_t* self, re::Edx) {
    uint8_t* owner = At<uint8_t*>(self, 0x40);
    if (OwnerPed(owner)) {
        At<uint8_t*>(self, 0x4C) = At<uint8_t*>(OwnerPed(owner), 0x114);
        return;
    }
    if (At<uint8_t*>(owner, 0xC)) {
        At<uint8_t*>(self, 0x4C) = At<uint8_t*>(At<uint8_t*>(owner, 0xC), 0x114);
    }
}

// 0x60AF60 (thiscall): AnimationTrack::Update: while playing (+0x44) its animation (+0x24)
// runs at +0x30 (scaled by the ped's stat 0x14 percent; +0x3D: +0x54 instead) until its
// end (+0x2C, else the animation's length); kind +0x20: 0 stops with it, 1 also stops the
// animation, 2 never stops
uint8_t __fastcall AnimationTrack_Update(uint8_t* self, re::Edx, float dt) {
    vc::Call<void>(self, acttree::kTrackResolve);
    uint8_t* blend = At<uint8_t*>(self, 0x4C);
    if (!blend || !At<void*>(blend, 0x94)) {
        return 0;
    }
    int32_t kind = At<int32_t>(self, 0x20);
    if (self[0x44] == 0) {
        return kind != 0 ? 1 : 0;
    }
    if (kind != 2) {
        uint8_t* anim = Blend_GetAnim(At<uint8_t*>(self, 0x4C), At<uint32_t>(self, 0x24));
        if (!anim) {
            return 0;
        }
        float speed = At<float>(self, 0x30);
        float scale = 1.0f;
        uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x40));
        if (ped) {
            scale = float(double(Ped_GetStat(ped, 0x14)) / 100.0);
        }
        float rate = scale * speed;
        if (self[0x3D] != 0) {
            rate = At<float>(self, 0x54);
        }
        AnimManager_SetSpeed(animManager, At<uint8_t*>(self, 0x4C), anim, 0, rate);
        float time;
        AnimManager_GetTime(animManager, At<uint8_t*>(self, 0x4C), anim, 0, &time);
        time = rate * dt + time;
        float length = At<float>(At<uint8_t*>(anim, 0x18), 0xC);
        bool pastLength = length < time || length == time;
        float end = At<float>(self, 0x2C);
        bool stop;
        if (end < 0.0f || end > length) {
            stop = pastLength;
        } else {
            stop = end < time || end == time;
        }
        if (stop) {
            self[0x44] = 0;
            if (kind == 1) {
                AnimManager_Update(animManager, At<uint8_t*>(self, 0x4C), anim, -1);
            }
        }
    }
    if (self[0x44] != 0) {
        return 1;
    }
    return kind != 0 ? 1 : 0;
}

// 0x60B0E0 (thiscall): AnimationTrack::End: its animation blended out over +0x38; the
// ped's +0xE4 set to +0x50 (+0x3C)
uint8_t __fastcall AnimationTrack_End(uint8_t* self, re::Edx) {
    vc::Call<void>(self, acttree::kTrackResolve);
    if (!At<uint8_t*>(self, 0x4C)) {
        return 0;
    }
    if (At<int32_t>(self, 0x24) > 0) {
        uint8_t* anim = Blend_GetAnim(At<uint8_t*>(self, 0x4C), At<uint32_t>(self, 0x24));
        if (anim) {
            AnimManager_BlendOut(animManager, At<uint8_t*>(self, 0x4C), anim, At<float>(self, 0x38));
        }
    }
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x40));
    if (ped && self[0x3C] != 0) {
        At<uint32_t>(ped, 0xE4) = self[0x50];
    }
    return 1;
}

// 0x60B140 (thiscall): DialogAnimationTrack::Resolve: the blend of the dialog's
// participant +0x58 (8 the speaker, 9 who it grapples)
void __fastcall DialogAnimationTrack_Resolve(uint8_t* self, re::Edx) {
    int32_t which = At<int32_t>(self, 0x58);
    uint8_t* owner = At<uint8_t*>(self, 0x40);
    uint8_t* entity;
    if (which == 8) {
        entity = OwnerPed(owner);
        if (entity) {
            At<uint8_t*>(self, 0x4C) = At<uint8_t*>(entity, 0x114);
            return;
        }
        entity = At<uint8_t*>(owner, 0xC);
        if (entity) {
            At<uint8_t*>(self, 0x4C) = At<uint8_t*>(entity, 0x114);
        }
        return;
    }
    if (which == 9) {
        entity = OwnerPed(owner);
        if (entity && At<uint8_t*>(entity, 0x155C)) {
            At<uint8_t*>(self, 0x4C) = At<uint8_t*>(At<uint8_t*>(entity, 0x155C), 0x114);
            return;
        }
    } else {
        entity = Conversations_GetSpeaker(conversations, which);
        if (entity) {
            At<uint8_t*>(self, 0x4C) = At<uint8_t*>(entity, 0x114);
            return;
        }
    }
    At<uint8_t*>(self, 0x4C) = nullptr;
}

// 0x60B1B0 (thiscall): HitReactTrack::Start: the ped's hit reaction (vf 0x3C) from its
// front / back (+0x34 1: from behind, 2: in front), its controller's reaction timing
uint8_t __fastcall HitReactTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x3C) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (!ped) {
        return 0;
    }
    V3 dir = {0.0f, 0.0f, 0.0f};
    float* p = Pos(ped);
    V3 pos = {p[0], p[1], p[2]};
    V3 forward;
    Placeable_GetForward(ped, &forward);
    if (At<int32_t>(self, 0x34) == 1) {
        forward.x = -forward.x;
        forward.y = -forward.y;
        forward.z = -forward.z;
        dir = forward;
    } else if (At<int32_t>(self, 0x34) == 2) {
        dir = forward;
    }
    vc::Call<void>(ped, 0x3C, At<float>(self, 0x1C), 0.0f, 0, -1, &pos, &dir, 0, 0, At<int32_t>(self, 0x18), 0,
                   At<float>(self, 0x20), At<float>(self, 0x24), int(self[0x30]), 1, 0, -1, 0.0f, 0);
    At<float>(At<uint8_t*>(vc::Call<uint8_t*>(ped, 0x40), 0x10), 0x2C) = -0.95f;
    float a = At<float>(self, 0x2C);
    At<float>(At<uint8_t*>(vc::Call<uint8_t*>(ped, 0x40), 0x10), 0x54) = a;
    float b = At<float>(self, 0x28);
    At<float>(At<uint8_t*>(vc::Call<uint8_t*>(ped, 0x40), 0x10), 0x58) = b;
    return 0;
}

// 0x60B300 (thiscall): HitReactTrack::Update: the reaction timer cleared
uint8_t __fastcall HitReactTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x3C));
    if (ped) {
        At<float>(At<uint8_t*>(vc::Call<uint8_t*>(ped, 0x40), 0x10), 0x2C) = 0.0f;
    }
    return 1;
}

// 0x60B320 (thiscall): JointDriverTrack's attachment (+0x58: 0 the ped, 1 the other, 2 its
// vehicle, 3 its use point's entity, 4 none) flagged busy (+0xE4, the old flag kept at
// +0x71, the entity at +0x78); the driven one (+0x60: 0 the ped, 1 the other) at +0x6C
uint8_t __fastcall JointDriverTrack_Attach(uint8_t* self, re::Edx, uint8_t* ped, uint8_t* other) {
    if (!ped) {
        return 0;
    }
    uint8_t attached = 0;
    At<uint8_t*>(self, 0x78) = nullptr;
    At<uint8_t*>(self, 0x6C) = nullptr;
    uint8_t* entity = nullptr;
    switch (At<uint32_t>(self, 0x58)) {
    case 0:
        entity = ped;
        break;
    case 1:
        entity = other;
        break;
    case 2:
        entity = At<uint8_t*>(ped, 0x1554);
        if (!entity) {
            entity = At<uint8_t*>(ped, 0x430);
        }
        break;
    case 3:
        entity = UsePoint_GetEntity(vc::Call<uint8_t*>(ped, 0x40));
        break;
    case 4:
        attached = 1;
        break;
    default:
        break;
    }
    if (At<uint32_t>(self, 0x58) <= 3 && entity) {
        self[0x71] = At<int32_t>(entity, 0xE4) != 0 ? 1 : 0;
        At<int32_t>(entity, 0xE4) = 1;
        At<uint8_t*>(self, 0x78) = entity;
        attached = 1;
    }
    if (At<int32_t>(self, 0x60) == 0) {
        At<uint8_t*>(self, 0x6C) = ped;
    } else if (At<int32_t>(self, 0x60) == 1 && other) {
        At<uint8_t*>(self, 0x6C) = other;
    }
    return attached;
}

// 0x60B430 (thiscall): JointDriverTrack::End: the attachment's busy flag put back, its
// joint (+0x74) released (vf 0x50 (3), then its reference count), then AnimationTrack's
uint8_t __fastcall JointDriverTrack_End(uint8_t* self, re::Edx) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x40));
    uint8_t* entity = nullptr;
    switch (At<uint32_t>(self, 0x58)) {
    case 0:
        At<uint32_t>(ped, 0xE4) = self[0x71];
        break;
    case 1:
        if (At<uint8_t*>(ped, 0x155C)) {
            At<uint32_t>(At<uint8_t*>(ped, 0x155C), 0xE4) = self[0x71];
        }
        break;
    case 2:
        entity = At<uint8_t*>(ped, 0x1554);
        if (!entity) {
            entity = At<uint8_t*>(ped, 0x430);
        }
        if (entity) {
            At<uint32_t>(entity, 0xE4) = self[0x71];
        }
        break;
    case 3:
        entity = UsePoint_GetEntity(vc::Call<uint8_t*>(ped, 0x40));
        if (entity) {
            At<uint32_t>(entity, 0xE4) = self[0x71];
        }
        break;
    default:
        break;
    }
    uint8_t* joint = At<uint8_t*>(self, 0x74);
    if (joint) {
        vc::Call<void>(joint, 0x50, 3);
        joint = At<uint8_t*>(self, 0x74);
        At<int32_t>(joint, 4) = At<int32_t>(joint, 4) - 1;
        if (At<int32_t>(joint, 4) < 1) {
            vc::Call<void>(joint, 4, 1);
        }
    }
    return reinterpret_cast<uint8_t(__thiscall*)(uint8_t*)>(0x0060B0E0)(self);
}

// 0x60B4E0 (thiscall): AnimationDriverSteerTrack::Update: the owner's parameter from the
// bike's steering (+0x20..+0x24 onto +0x18..+0x1C)
uint8_t __fastcall SteerTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* owner = At<uint8_t*>(self, 0x28);
    uint8_t* bike = At<uint8_t*>(OwnerPed(owner), 0x1554);
    if (bike) {
        *At<float*>(owner, 0x38) = RemapRange(At<float>(bike, 0x2A0), At<float>(self, 0x20), At<float>(self, 0x24),
                                              At<float>(self, 0x18), At<float>(self, 0x1C));
    }
    return 1;
}

// 0x60B9C0 (thiscall): AnimationDriverTargetSyncTrack::Update: the grappled ped's
// parameter follows the owner's
uint8_t __fastcall TargetSyncDriverTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* owner = At<uint8_t*>(self, 0x18);
    uint8_t* target = At<uint8_t*>(OwnerPed(owner), 0x155C);
    if (target) {
        float value = *At<float*>(owner, 0x38);
        *At<float*>(vc::Call<uint8_t*>(target, 0x40), 0x38) = value;
    }
    return 1;
}

// 0x60B9F0 (thiscall): ParametricAnimationControllerTrack::Update: a pad input (+0x1D:
// the look inputs; +0x18 / +0x19 which sticks, +0x1B which axis) moves the parameter
// (+0x30, rate +0x20) within +0x24..+0x28 (+0x1A wrapping), or (+0x1C) sets it directly
uint8_t __fastcall ParametricControllerTrack_Update(uint8_t* self, re::Edx, float) {
    float input = 0.0f;
    bool axis = self[0x1B] != 0;
    if (self[0x1D] != 0) {
        input = axis ? lookInputA_C3CD8C : lookInputB_C3CD90;
    } else if (self[0x18] != 0) {
        input = Pad_GetValue(axis ? 0x10 : 0x11, 0);
        if (self[0x19] != 0 && 0.0f == input) {
            input = Pad_GetValue(axis ? 0x12 : 0x13, 0);
        }
    } else if (self[0x19] != 0) {
        input = Pad_GetValue(axis ? 0x12 : 0x13, 0);
    }
    float lo = At<float>(self, 0x24);
    if (self[0x1C] != 0) {
        At<float>(self, 0x30) = (input + 1.0f) * 0.5f * At<float>(self, 0x28) + At<float>(self, 0x24);
    } else {
        float value = At<float>(self, 0x20) * input + At<float>(self, 0x30);
        At<float>(self, 0x30) = value;
        if (self[0x1A] != 0) {
            if (At<float>(self, 0x24) > value) {
                float range = At<float>(self, 0x28) - At<float>(self, 0x24);
                do {
                    At<float>(self, 0x30) = At<float>(self, 0x30) + range;
                } while (At<float>(self, 0x24) > At<float>(self, 0x30));
            }
            if (At<float>(self, 0x28) < At<float>(self, 0x30)) {
                float range = At<float>(self, 0x28) - At<float>(self, 0x24);
                do {
                    At<float>(self, 0x30) = At<float>(self, 0x30) - range;
                } while (At<float>(self, 0x28) < At<float>(self, 0x30));
            }
        } else if (lo > value) {
            At<float>(self, 0x30) = At<float>(self, 0x24);
        } else if (At<float>(self, 0x28) < value) {
            At<float>(self, 0x30) = At<float>(self, 0x28);
        }
    }
    *At<float*>(At<uint8_t*>(self, 0x34), 0x38) = At<float>(self, 0x30);
    return 1;
}

// 0x60BB80 (thiscall): ParametricTrackerTrack's deleting destructor: the last tracker
// gone, the parametric static flag set and the turn reset
uint8_t* __fastcall ParametricTrackerTrack_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kVtParametricTrackerTrack;
    trackerCount_1B65030 -= 1;
    if (trackerCount_1B65030 == 0) {
        parametricStatic_B0B134 = 1;
        trackerSaved_1B6502C = 1;
    }
    trackerTurn_1B65034 = 0;
    TrackBase_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x60BFB0 (thiscall): the spud cannon's loop sounds stopped (for the player: its own
// fire sound anywhere)
void __fastcall SpudCannonTrack_StopSounds(uint8_t* self, re::Edx, uint8_t* a, uint8_t* b) {
    if (!b) {
        return;
    }
    self[0x36] = 1;
    self[0x37] = 1;
    self[0x38] = 1;
    if (At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x3C)), 0x1310) == 13) {
        int32_t id = Sounds_GetId(sounds_CF3DA0, spudFire);
        LoopSounds_Stop(loopSounds_CF4274, nullptr, nullptr, id);
    } else {
        int32_t id = Sounds_GetId(sounds_CF3DA0, spudFire);
        LoopSounds_Stop(loopSounds_CF4274, a, b, id);
    }
    self[0x36] = 0;
    if (self[0x37] != 0) {
        LoopSounds_Stop(loopSounds_CF4274, a, b, Sounds_GetId(sounds_CF3DA0, spudDryFire));
        self[0x37] = 0;
    }
    if (self[0x38] != 0) {
        LoopSounds_Stop(loopSounds_CF4274, a, b, Sounds_GetId(sounds_CF3DA0, spudMisfire));
        self[0x38] = 0;
    }
}

// 0x60C060 (thiscall): AnimationDoorControlTrack::Update: the vehicle's door +0x18 moved
// to the owner's parameter over +0x1C..+0x20 (cars only)
uint8_t __fastcall DoorControlTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* owner = At<uint8_t*>(self, 0x24);
    uint8_t* ped = OwnerPed(owner);
    uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
    if (!vehicle) {
        vehicle = At<uint8_t*>(ped, 0x430);
        if (!vehicle) {
            return 0;
        }
    }
    int32_t control = At<int32_t>(self, 0x18);
    int32_t state = DoorControl_StateFor(self, control);
    if (state == 0) {
        return 0;
    }
    float from = At<float>(self, 0x1C);
    float to = At<float>(self, 0x20);
    if (to == from) {
        return 0;
    }
    float span = std::fabs(from - to);
    float amount = *At<float*>(owner, 0x38) / span;
    if (from < to) {
        amount = 1.0f - amount;
    }
    if (At<int32_t>(vehicle, 0x46C) == 0) {
        vc::Call<void>(vehicle, 0xB8, state, control, amount);
    }
    return 1;
}

// 0x60C130 (thiscall): VehicleAnimationTrack::Resolve: the blend of the ped's vehicle
void __fastcall VehicleAnimationTrack_Resolve(uint8_t* self, re::Edx) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x40));
    uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
    if (vehicle || (vehicle = At<uint8_t*>(ped, 0x430)) != nullptr) {
        At<uint8_t*>(self, 0x4C) = At<uint8_t*>(vehicle, 0x114);
    }
}

} // namespace

#define ACTTRACKSANIM2_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

ACTTRACKSANIM2_REVERSED("LocomotionTrack::DeletingDestruct", 0x0060ADA0, &TrackDeletingDtor<kVtLocomotionTrack>);
ACTTRACKSANIM2_REVERSED("LocomotionAnimationNewTrack::DeletingDestruct", 0x0060ADD0,
                        &TrackDeletingDtor<kVtLocomotionAnimationNewTrack, 0x18>);
ACTTRACKSANIM2_REVERSED("ActionTracks::FindAnimNode_60AE00", 0x0060AE00, &FindAnimNode);
ACTTRACKSANIM2_REVERSED("AnimationGroupReleaseTrack::Start", 0x0060AEA0, &AnimationGroupReleaseTrack_Start);
ACTTRACKSANIM2_REVERSED("AnimationTrack::Resolve", 0x0060AF30, &AnimationTrack_Resolve);
ACTTRACKSANIM2_REVERSED("AnimationTrack::Update", 0x0060AF60, &AnimationTrack_Update);
ACTTRACKSANIM2_REVERSED("AnimationTrack::End", 0x0060B0E0, &AnimationTrack_End);
ACTTRACKSANIM2_REVERSED("DialogAnimationTrack::Resolve", 0x0060B140, &DialogAnimationTrack_Resolve);
ACTTRACKSANIM2_REVERSED("HitReactTrack::Start", 0x0060B1B0, &HitReactTrack_Start);
ACTTRACKSANIM2_REVERSED("HitReactTrack::Update", 0x0060B300, &HitReactTrack_Update);
ACTTRACKSANIM2_REVERSED("JointDriverTrack::Attach", 0x0060B320, &JointDriverTrack_Attach);
ACTTRACKSANIM2_REVERSED("JointDriverTrack::End", 0x0060B430, &JointDriverTrack_End);
ACTTRACKSANIM2_REVERSED("AnimationDriverSteerTrack::Update", 0x0060B4E0, &SteerTrack_Update);
ACTTRACKSANIM2_REVERSED("AnimationDriverTargetSyncTrack::Update", 0x0060B9C0, &TargetSyncDriverTrack_Update);
ACTTRACKSANIM2_REVERSED("ParametricAnimationControllerTrack::Update", 0x0060B9F0, &ParametricControllerTrack_Update);
ACTTRACKSANIM2_REVERSED("ParametricTrackerTrack::DeletingDestruct", 0x0060BB80, &ParametricTrackerTrack_DeletingDestruct);
ACTTRACKSANIM2_REVERSED("SpudCannonFireControllerTrack::StopSounds", 0x0060BFB0, &SpudCannonTrack_StopSounds);
// (regcheck: the original of this keeps ecx / edx across its call to 0x6080C0: turn on
// together with AnimationDoorControlTrack::StateFor)
ACTTRACKSANIM2_REVERSED("AnimationDoorControlTrack::Update", 0x0060C060, &DoorControlTrack_Update);
ACTTRACKSANIM2_REVERSED("VehicleAnimationTrack::Resolve", 0x0060C130, &VehicleAnimationTrack_Resolve);
