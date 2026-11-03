// The action tree's small AI tracks around social life, allies and targets: flee /
// catch / pass / receive / slinger objectives, seeking pickups / snow / a surface, prop
// usage, the action timer, allies (recruit, dismiss, the leader's target, state and
// morale), socialising (emotions, wait time, kisses, armor bonus, stance), targeting
// (nearest car, the player, attack rights, semaphores, grapple partners), evading
// grenades and projectiles, the tether, blips, the girlfriend register.
// 0x628E40..0x62DEA6.
// The ped: +0x2E0 its move state / inputs, +0x2F0 its ally chain (+0x2F4 its leader,
// +0x318 / +0x31C the ally state / morale), +0x358 its tether, +0x3BC its scanner, +0x438
// its prop, +0x5F0 its objectives, +0x654 its steering, +0x668 its combat slots, +0xB44
// its social, +0x131C evading, +0x1580 kept on screen, +0x1584 not moving out of the way.

#include <cstdint>

#include "game/ActionTracksCommon.h"
#include "game/Pool.h"
#include "reversed.h"

using namespace actc;

namespace {

constexpr uint32_t kVtSetTargetAsGrappleTargetTrack = 0x009336B0;
constexpr uint32_t kVtReference = 0x0092DAAC;
constexpr uint32_t kVtFleeObjective = 0x00911894;

GAME_VAR(CPool<uint8_t>*, vehiclePool, 0x00C0F5F4);   // CPools::ms_pVehiclePool
GAME_VAR(uint8_t*, hudRadar, 0x00C674CC);
GAME_VAR(void*, yearbookHud, 0x00C1AA04);
GAME_VAR(void*, attitudeTable, 0x00C2AF9C);
GAME_VAR(void*, stimuli, 0x00C2C1A0);
inline uint8_t* const pickups = reinterpret_cast<uint8_t*>(0x00C0A100);   // 0x48 bytes each
inline void* const playerInfo = reinterpret_cast<void*>(0x00C1AEA8);
inline void* const conversations = reinterpret_cast<void*>(0x00B9DAB8);

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Entity_RegisterReference, 0x004657D0, uint8_t* entity, re::Ptr32<uint8_t>* ref);
GAME_FN(uint8_t*, __thiscall, Objective_Construct, 0x0046FA90, void* self, int type, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, AttackPedObjective_Construct, 0x00470D60, void* self, uint8_t* ped, uint8_t* target,
        int kind);
GAME_FN(uint8_t*, __thiscall, SlingerObjective_Construct, 0x00471220, void* self, uint8_t* ped, int32_t value);
GAME_FN(uint8_t*, __thiscall, Queue_GetCurrent, 0x00470340, uint8_t* queue);
GAME_FN(uint8_t*, __thiscall, Queue_FindOfType, 0x004701D0, uint8_t* queue, int32_t type);
GAME_FN(void, __thiscall, Queue_Abort, 0x00470260, uint8_t* queue, int32_t type);
GAME_FN(uint8_t, __thiscall, Queue_Prepend, 0x00471470, uint8_t* queue, uint8_t* objective);
GAME_FN(uint8_t, __thiscall, Queue_Add, 0x00471390, uint8_t* queue, uint8_t* objective);
GAME_FN(int, __cdecl, Pickups_Find, 0x004438B0, int handle);
GAME_FN(uint8_t, __thiscall, DoorLock_Request, 0x004EADC0, uint8_t* lock, uint8_t* ped, int32_t request, int flag);
GAME_FN(void, __thiscall, Ped_SetActionTimer, 0x00474990, uint8_t* ped, float seconds);
GAME_FN(void, __thiscall, ActionInputs_SetRequest, 0x005F3A90, uint8_t* inputs, int button, int kind, float time);
GAME_FN(void, __thiscall, Countdown_StartIfStopped, 0x0045BD50, uint8_t* timer, uint32_t ms);
GAME_FN(uint8_t, __thiscall, Timer_IsDone, 0x0045BDD0, uint8_t* timer);
GAME_FN(uint8_t, __thiscall, Timer_IsRunning, 0x0045BDB0, uint8_t* timer);
GAME_FN(void, __thiscall, Timer_Start, 0x0045BDF0, uint8_t* timer, float least, float most);
GAME_FN(float, __cdecl, FindGroundZ, 0x004636B0, float x, float y, float z, uint8_t* found, int flag);
GAME_FN(uint8_t, __thiscall, Steering_GoTo, 0x004A5000, uint8_t* steering, float x, float y, float z, uint32_t flags,
        float speed, float radius, uint8_t* entity, int flag);
GAME_FN(uint8_t, __thiscall, Steering_GoToWith, 0x004A4910, uint8_t* steering, float x, float y, float z, int unused,
        uint32_t flags, float speed, float radius, float a, float b);
GAME_FN(void, __thiscall, Steering_Stop, 0x004A4710, uint8_t* steering);
GAME_FN(void, __thiscall, Ally_AddFollower, 0x004860F0, uint8_t* ally, uint8_t* ped, int keep);
GAME_FN(void, __thiscall, Ally_RemoveFollowers, 0x00485A40, uint8_t* ally);
GAME_FN(uint8_t, __thiscall, Ally_Leave, 0x00485A50, uint8_t* ally);
GAME_FN(uint8_t, __thiscall, Social_UpdateSocialize, 0x004952B0, uint8_t* social);
GAME_FN(uint8_t, __thiscall, Social_UpdateActionComplete, 0x00495590, uint8_t* social);
GAME_FN(uint8_t, __thiscall, Social_EngageWithPlayer, 0x00494FF0, uint8_t* social, uint8_t* target);
GAME_FN(void, __thiscall, Social_SetMood, 0x0048F1D0, uint8_t* social, uint8_t* other, int mood, int keep, int keepKept);
GAME_FN(void, __thiscall, Social_WantsToSocialize, 0x004925E0, uint8_t* social, uint8_t* ped, int a, int b, int c);
GAME_FN(void, __thiscall, Social_Stop, 0x004910A0, uint8_t* social);
GAME_FN(void, __thiscall, Social_IncKissCount, 0x0048E650, uint8_t* social);
GAME_FN(float, __thiscall, Social_ArmorBonus, 0x0048E2F0, uint8_t* social);
GAME_FN(void, __thiscall, EntityRef_Set, 0x005D7F10, re::Ptr32<uint8_t>* ref, uint8_t* entity);
GAME_FN(void, __thiscall, Events_Add, 0x004A6670, void* events, int kind, uint8_t* ped, uint8_t* other, const float* pos,
        float a, float b);
GAME_FN(uint8_t*, __thiscall, Targeting_GetLookTarget, 0x004D7780, uint8_t* targeting);
GAME_FN(void, __thiscall, Ped_SetTargetEntity, 0x00498B10, uint8_t* ped, uint8_t* target);
GAME_FN(void, __thiscall, Ped_SetHealth, 0x004782E0, uint8_t* ped, float health);
GAME_FN(int32_t*, __thiscall, Ped_GetAttitude, 0x00475380, uint8_t* ped, int32_t* out, int32_t pedType);
GAME_FN(void, __thiscall, Ped_SetPedToTypeAttitude, 0x004731D0, uint8_t* ped, int32_t type, int32_t attitude);
GAME_FN(void, __thiscall, AttitudeTable_Worsen, 0x0046F930, void* table, int32_t pedType, int32_t otherType);
GAME_FN(void, __thiscall, Ped_SetGrapplePartner, 0x004744B0, uint8_t* ped, uint8_t* partner);
GAME_FN(void, __thiscall, Ped_GetGirlfriendIndex, 0x0049BBD0, uint8_t* player, int model, int32_t* out);
GAME_FN(uint32_t, __thiscall, PlayerInfo_GetYearbookBits, 0x004466F0, void* info, int which);
GAME_FN(void, __thiscall, PlayerInfo_SetYearbookBits, 0x004466D0, void* info, int which, uint32_t bits);
GAME_FN(void, __thiscall, YearbookHud_RegisterGirlfriend, 0x0045BEF0, void* hud, int32_t index);
GAME_FN(uint8_t, __thiscall, Combat_DetectGrenade, 0x0048A280, uint8_t* combat, V3* pos, float* radius);
GAME_FN(uint8_t, __thiscall, Combat_DetectProjectile, 0x0048A450, uint8_t* combat, V3* a, V3* b, V3* c);
GAME_FN(void, __thiscall, Combat_486CE0, 0x00486CE0, uint8_t* combat);
GAME_FN(void, __thiscall, Combat_AcquireAttackRights, 0x00486D90, uint8_t* combat, uint8_t* ped);
GAME_FN(void, __thiscall, Combat_ReleaseAttackRights, 0x00486310, uint8_t* combat, uint8_t* ped);
GAME_FN(void, __thiscall, Combat_SetSemaphore, 0x00486F00, uint8_t* combat, uint8_t* ped, float seconds);
GAME_FN(void, __thiscall, Scanner_Alert, 0x0048C480, uint8_t* scanner, uint8_t* ped);
GAME_FN(void, __thiscall, Tether_SetEntity, 0x00499AB0, uint8_t* tether, uint8_t* entity, float radius,
        const float* offset);
GAME_FN(void, __thiscall, Tether_Clear, 0x00499A40, uint8_t* tether);
GAME_FN(void, __thiscall, Prop_ReleasePed, 0x004EE060, uint8_t* prop, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, Conversations_GetSpeaker, 0x006A29F0, void* conversations, int32_t which);
GAME_FN(void, __thiscall, Radar_RemoveBlipFor, 0x0054A890, uint8_t* radar, int kind, int32_t handle);
GAME_FN(uint8_t, __thiscall, Radar_HasBlipFor, 0x00548AD0, uint8_t* radar, int32_t handle);
GAME_FN(int32_t, __thiscall, Radar_AddBlipFor, 0x0054A5B0, uint8_t* radar, int kind, int32_t handle, uint32_t colour,
        int a, int icon, int c);
GAME_FN(void, __thiscall, Radar_SetBlipSize, 0x00548BF0, uint8_t* radar, int32_t blip, int size);
GAME_FN(void, __thiscall, Node_Destruct, 0x005F4E50, uint8_t* node);
GAME_FN(uint8_t*, __thiscall, Reference_Get0C, 0x005F4F90, uint8_t* ref, int32_t key);
GAME_FN(void, __thiscall, Reference_ResolveByPath, 0x005F5F50, uint8_t* ref);
GAME_FN(void, __thiscall, Ctl_QueueTracks, 0x005F3F80, uint8_t* controller, uint8_t* node);
GAME_FN(void, __thiscall, Ctl_PlayNode, 0x005F45F0, uint8_t* controller, uint8_t* node);

uint8_t* Steering(uint8_t* ped) {
    return ped + 0x654;
}
uint8_t* Inputs(uint8_t* ped) {
    return At<uint8_t*>(ped, 0x2E0);
}
// the ped's target when it's a ped
uint8_t* PedTarget(uint8_t* ped) {
    uint8_t* target = Ped_GetTarget(ped);
    return target && IsPedEntity(target) ? target : nullptr;
}
// the ped's target, else what it looks at, when a ped
uint8_t* PedTargetOrLook(uint8_t* ped) {
    uint8_t* target = PedTarget(ped);
    if (target) {
        return target;
    }
    uint8_t* look = Targeting_GetLookTarget(At<uint8_t*>(ped, 0x1C8));
    return look && IsPedEntity(look) ? look : nullptr;
}

// 0x628E40 (thiscall): FleeObjectiveTrack::Start: a flee objective (type 0x16) from the
// ped's +0x544 (+0x19), or the owner's +0x10's entity, or the ped's target; queued
// last (+0x18) or first
uint8_t __fastcall FleeObjectiveTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* ped = OwnerPed(owner);
    uint8_t* from;
    if (self[0x19] != 0) {
        from = At<uint8_t*>(ped, 0x544);
    } else {
        from = At<uint8_t*>(At<uint8_t*>(owner, 0x10), 4);
        if (!from) {
            from = At<uint8_t*>(ped, 0x538);
        }
    }
    if (!from) {
        return 1;
    }
    uint8_t* objective = static_cast<uint8_t*>(Malloc(0x14));
    if (objective) {
        Objective_Construct(objective, 0x16, ped);
        At<uint32_t>(objective, 0) = kVtFleeObjective;
        At<uint8_t*>(objective, 0x10) = from;
        Entity_RegisterReference(from, &At<uint8_t*>(objective, 0x10));
    }
    if (self[0x18] != 0) {
        Queue_Add(ped + 0x5F0, objective);
    } else {
        Queue_Prepend(ped + 0x5F0, objective);
    }
    return 1;
}

// 0x628EE0 (thiscall): SeekToPickupTrack::Update: to the pickup of a seek-pickup
// objective (0x17; the current one is read without a check, as the game does) while it's
// there (+0x44 1): flags 0x1C, speed 1, radius 0.3
uint8_t __fastcall SeekToPickupTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    uint8_t* current = Queue_GetCurrent(ped + 0x5F0);
    if (At<int32_t>(current, 8) != 0x17) {
        return 0;
    }
    int index = Pickups_Find(At<int32_t>(current, 0x10));
    if (index == -1) {
        return 0;
    }
    uint8_t* pickup = pickups + index * 0x48;
    if (pickup[0x44] != 1) {
        return 0;
    }
    const float* p = &At<float>(pickup, 8);
    return Steering_GoTo(Steering(ped), p[0], p[1], p[2], 0x1C, 1.0f, 0.3f, nullptr, 0) != 0 ? 1 : 0;
}

// 0x628F70 (thiscall): PropSetUsageStateTrack::Start: the ped's door (a prop of kind 6)
// gets the lock request +0x18 (+0x1C)
uint8_t __fastcall PropSetUsageStateTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    uint8_t* ped = OwnerPed(owner);
    uint8_t* prop = At<uint8_t*>(ped, 0x438);
    if (prop && (prop[0x108] & 7) == 6) {
        DoorLock_Request(prop + 0x398, ped, At<int32_t>(self, 0x18), self[0x1C]);
    }
    return 1;
}

// 0x628FB0 (thiscall): SetActionTimerTrack::Update: the ped's action time +0x18 s on
uint8_t __fastcall SetActionTimerTrack_Update(uint8_t* self, re::Edx, float) {
    Ped_SetActionTimer(OwnerPed(At<uint8_t*>(self, 0x1C)), At<float>(self, 0x18));
    return 0;
}

// 0x6294C0 (thiscall): SeekToSnowTrack::Update: to the ped's snow spot (+0x510, when
// +0x50C); there it requests +0x18 and waits a second
uint8_t __fastcall SeekToSnowTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x1C));
    if (ped[0x50C] == 0) {
        return 0;
    }
    if (!Steering_GoTo(Steering(ped), At<float>(ped, 0x510), At<float>(ped, 0x514), At<float>(ped, 0x518), 0x1C, 1.0f,
                       0.3f, nullptr, 0)) {
        ActionInputs_SetRequest(Inputs(ped), At<int32_t>(self, 0x18), 1, 0.0f);
        Countdown_StartIfStopped(self + 0x20, 1000);
    }
    return Timer_IsDone(self + 0x20) ? 0 : 1;
}

// 0x629560 (thiscall): SeekToSurfaceTrack::Update: to the ped's spot (+0x510, on the
// ground found from a unit above); there the spot's dropped
uint8_t __fastcall SeekToSurfaceTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    if (ped[0x50C] == 0) {
        return 0;
    }
    uint8_t found = 0;
    float above = At<float>(ped, 0x518) + 1.0f;
    float ground = FindGroundZ(At<float>(ped, 0x510), At<float>(ped, 0x514), above, &found, 1);
    float x = At<float>(ped, 0x510);
    float y = At<float>(ped, 0x514);
    float z = At<float>(ped, 0x518);
    if (found) {
        z = ground;
    }
    if (Steering_GoTo(Steering(ped), x, y, z, 0x1C, 1.0f, 0.3f, nullptr, 0)) {
        return 1;
    }
    ped[0x50C] = 0;
    return 0;
}

// 0x629640 / 0x6296C0 / 0x629740 (thiscall): Play catch / pass ball / receive ball
// objective tracks' Start: that objective with the ped's target (a ped) and +0x18,
// queued last
template <uintptr_t Construct>
uint8_t __fastcall BallObjectiveTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* ped = OwnerPed(owner);
    uint8_t* target = Ped_GetTarget(ped);
    if (!target || !IsPedEntity(target)) {
        return 0;
    }
    void* mem = Malloc(0x18);
    uint8_t* objective = nullptr;
    if (mem) {
        objective = reinterpret_cast<uint8_t*(__thiscall*)(void*, uint8_t*, uint8_t*, int32_t)>(Construct)(
            mem, ped, target, At<int32_t>(self, 0x18));
    }
    Queue_Add(ped + 0x5F0, objective);
    return 1;
}

// 0x6297C0 (thiscall): RecruitAllyTrack::Update: its target (a ped, or none) as follower
uint8_t __fastcall RecruitAllyTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    Ally_AddFollower(ped + 0x2F0, PedTarget(ped), 0);
    return 0;
}

// 0x629810 (thiscall): DismissAllyTrack::Update: its followers let go
uint8_t __fastcall DismissAllyTrack_Update(uint8_t* self, re::Edx, float) {
    Ally_RemoveFollowers(OwnerPed(At<uint8_t*>(self, 0x18)) + 0x2F0);
    return 0;
}

// 0x629830 (thiscall): NeverOffscreenTrack::Start: kept on screen
uint8_t __fastcall NeverOffscreenTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    At<int32_t>(OwnerPed(owner), 0x1580) += 1;
    return 1;
}

// 0x629850 (thiscall): NeverOffscreenTrack::End: let go (with +0x18)
uint8_t __fastcall NeverOffscreenTrack_End(uint8_t* self, re::Edx) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x1C));
    if (self[0x18] != 0) {
        At<int32_t>(ped, 0x1580) -= 1;
    }
    return 1;
}

// 0x629870 (thiscall): SocializeTrack::Update: the social's socialising goes on
uint8_t __fastcall SocializeTrack_Update(uint8_t* self, re::Edx, float) {
    return Social_UpdateSocialize(OwnerPed(At<uint8_t*>(self, 0x18)) + 0xB44);
}

// 0x629890 (thiscall): SocializeActionCompleteTrack::Update
uint8_t __fastcall SocializeActionCompleteTrack_Update(uint8_t* self, re::Edx, float) {
    return Social_UpdateActionComplete(OwnerPed(At<uint8_t*>(self, 0x18)) + 0xB44);
}

// 0x6298B0 (thiscall): PlayerSocializingTrack::Start: without +0x18 the ped engages the
// player; with it, whom it's with (+0x24: its +0x155C, else its target (a ped))
uint8_t __fastcall PlayerSocializingTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (self[0x18] == 0) {
        Social_EngageWithPlayer(ped + 0xB44, nullptr);
        return 1;
    }
    re::Ptr32<uint8_t>* with = &At<uint8_t*>(self, 0x24);
    EntityRef_Set(with, At<uint8_t*>(ped, 0x155C));
    if (!*with) {
        EntityRef_Set(with, PedTarget(ped));
    }
    return 1;
}

// 0x629930 (thiscall): PlayerSocializingTrack::End: with +0x18 the ped engages whom it
// was with, sending event +0x1C (unless 0x56) about them at its position
uint8_t __fastcall PlayerSocializingTrack_End(uint8_t* self, re::Edx) {
    if (self[0x18] == 0) {
        return 1;
    }
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x20));
    Social_EngageWithPlayer(ped + 0xB44, At<uint8_t*>(self, 0x24));
    int32_t event = At<int32_t>(self, 0x1C);
    uint8_t* with = At<uint8_t*>(self, 0x24);
    if (event != 0x56 && with) {
        const float* p = Pos(ped);
        V3 at = {p[0], p[1], p[2]};
        Events_Add(stimuli, event, ped, with, &at.x, -1.0f, -1.0f);
    }
    return 1;
}

// 0x6299C0 (thiscall): SetEmotionTowardsTargetTrack::Start: the mood +0x18 towards its
// target (or whom it looks at), a ped
uint8_t __fastcall SetEmotionTowardsTargetTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* ped = OwnerPed(owner);
    uint8_t* other = PedTargetOrLook(ped);
    if (other) {
        Social_SetMood(ped + 0xB44, other, At<int32_t>(self, 0x18), 0, 0);
    }
    return 1;
}

// 0x629A30 (thiscall): SetEmotionTowardsPlayerTrack::Start
uint8_t __fastcall SetEmotionTowardsPlayerTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    Social_SetMood(OwnerPed(owner) + 0xB44, playerPed, At<int32_t>(self, 0x18), 0, 0);
    return 1;
}

// 0x629A60 (thiscall): SetWantsToSocializeWithTargetTrack::Start
uint8_t __fastcall SetWantsToSocializeWithTargetTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    uint8_t* ped = OwnerPed(owner);
    uint8_t* other = PedTargetOrLook(ped);
    if (other) {
        Social_WantsToSocialize(ped + 0xB44, other, self[0x18], At<int32_t>(self, 0x1C), self[0x20]);
    }
    return 1;
}

// 0x629AD0 (thiscall): SetSocialWaitTimeTrack::Start
uint8_t __fastcall SetSocialWaitTimeTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    At<int32_t>(OwnerPed(owner), 0xD50) = At<int32_t>(self, 0x18);
    return 1;
}

// 0x629AF0 / 0x629B10 (thiscall): StopSocializing / IncKissCount tracks' Start: the
// social's call
template <uintptr_t Fn>
uint8_t __fastcall SocialCallTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    reinterpret_cast<void(__thiscall*)(uint8_t*)>(Fn)(OwnerPed(owner) + 0xB44);
    return 1;
}

// 0x629B30 (thiscall): GiveArmorBonusTrack::Start: the bonus health (+0x1C)
uint8_t __fastcall GiveArmorBonusTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    At<float>(self, 0x1C) = Social_ArmorBonus(OwnerPed(owner) + 0xB44);
    return 1;
}

// 0x629B60 (thiscall): GiveArmorBonusTrack::Update: health up by 4 a step to the bonus
// (done there)
uint8_t __fastcall GiveArmorBonusTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    if (!(At<float>(self, 0x1C) > At<float>(ped, 0x1CB8))) {
        return 1;
    }
    ped = OwnerPed(At<uint8_t*>(self, 0x18));
    Ped_SetHealth(ped, At<float>(ped, 0x1CB8) + 4.0f);
    ped = OwnerPed(At<uint8_t*>(self, 0x18));
    if (At<float>(self, 0x1C) < At<float>(ped, 0x1CB8)) {
        Ped_SetHealth(ped, At<float>(self, 0x1C));
        return 0;
    }
    return 1;
}

// 0x629BE0 (thiscall): GiveArmorBonusTrack::End: the bonus in full
uint8_t __fastcall GiveArmorBonusTrack_End(uint8_t* self, re::Edx) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    if (At<float>(self, 0x1C) > At<float>(ped, 0x1CB8)) {
        Ped_SetHealth(ped, At<float>(self, 0x1C));
    }
    return 1;
}

// 0x629C10 (thiscall): TargetNearestCarTrack::Update: the nearest car (kind 0, flat) is
// made the ped's target
uint8_t __fastcall TargetNearestCarTrack_Update(uint8_t* self, re::Edx, float) {
    float best = 3.4028235e+38f;
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    uint8_t* nearest = nullptr;
    CPool<uint8_t>* pool = vehiclePool;
    const float* pp = Pos(ped);
    V3 from = {pp[0], pp[1], pp[2]};
    for (int i = 0; i < pool->size; i++) {
        if (pool->flags[i] & 0x80) {
            continue;
        }
        uint8_t* vehicle = re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * i;
        if (!vehicle || At<int32_t>(vehicle, 0x46C) != 0) {
            continue;
        }
        V3 d;
        V3* r = VectorSub(&d, Pos(vehicle), &from.x);
        float sq = r->x * r->x + r->y * r->y;
        if (best > sq) {
            best = sq;
            nearest = vehicle;
        }
    }
    if (!nearest) {
        return 0;
    }
    Ped_SetTargetEntity(OwnerPed(At<uint8_t*>(self, 0x18)), nearest);
    return 1;
}

// 0x629D00 (thiscall): TargetPlayerTrack::Update: the player targeted unless +0x1C98
uint8_t __fastcall TargetPlayerTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* player = playerPed;
    if (player[0x1C98] != 0) {
        return 0;
    }
    Ped_SetTargetEntity(OwnerPed(At<uint8_t*>(self, 0x18)), player);
    return 1;
}

// 0x629D30 (thiscall): EvadeGrenadeTrack::Update: a grenade seen (when +0x34 asks)
// is kept (+0x24, its radius +0x30) and run from (flags 0x14, speed 1.5, the path kept
// the radius + +0x44 away) for 0.1..0.3 s; +0x18 marks the ped evading while it can't
// get away
uint8_t __fastcall EvadeGrenadeTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x1C));
    V3 pos = {0.0f, 0.0f, 0.0f};
    float radius;
    uint8_t going = 0;
    if (Combat_DetectGrenade(ped + 0x668, &pos, &radius) && self[0x34] != 0) {
        At<float>(self, 0x30) = radius;
        At<float>(self, 0x24) = pos.x;
        At<float>(self, 0x28) = pos.y;
        self[0x20] = 1;
        At<float>(self, 0x2C) = pos.z;
        Timer_Start(self + 0x38, 0.1f, 0.3f);
    }
    if (self[0x20] != 0) {
        float away = At<float>(self, 0x44) + At<float>(self, 0x30);
        going = Steering_GoToWith(Steering(ped), At<float>(self, 0x24), At<float>(self, 0x28), At<float>(self, 0x2C),
                                  self[0x34], 0x14, 1.5f, 1.0f, away, 20.0f);
    }
    self[0x34] = 0;
    if (self[0x18] != 0) {
        At<int32_t>(ped, 0x131C) = (self[0x20] != 0 && !going) ? 1 : 0;
    }
    return Timer_IsRunning(self + 0x38);
}

// 0x629E40 (thiscall): EvadeProjectileTrack::End (and EvadeGrenade's): not evading
uint8_t __fastcall EvadeProjectileTrack_End(uint8_t* self, re::Edx) {
    if (self[0x18] != 0) {
        At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x1C)), 0x131C) = 0;
    }
    return 1;
}

// 0x629E60 (thiscall): EvadeProjectileTrack::Update: a projectile seen (its start +0x24,
// direction +0x30, end +0x3C) is side-stepped for 0.5..1 s: within 3 of its end the ped
// steps across its line (the side away from it), else it stands; +0x18 marks it evading
// while it isn't stepping
uint8_t __fastcall EvadeProjectileTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x1C));
    V3 a = {0.0f, 0.0f, 0.0f};
    V3 b = {0.0f, 0.0f, 0.0f};
    V3 c = {0.0f, 0.0f, 0.0f};
    uint8_t stepping = 0;
    if (Combat_DetectProjectile(ped + 0x668, &a, &b, &c)) {
        At<float>(self, 0x24) = a.x;
        At<float>(self, 0x28) = a.y;
        At<float>(self, 0x2C) = a.z;
        V3 dir = {b.x, b.y, 0.0f};
        self[0x20] = 1;
        Vector_Normalise(&dir);
        At<float>(self, 0x30) = dir.x;
        At<float>(self, 0x34) = dir.y;
        At<float>(self, 0x38) = dir.z;
        At<float>(self, 0x3C) = c.x;
        At<float>(self, 0x40) = c.y;
        At<float>(self, 0x44) = c.z;
        Timer_Start(self + 0x4C, 0.5f, 1.0f);
    }
    const float* pp = Pos(ped);
    V3 from = {pp[0], pp[1], pp[2]};
    bool stop = true;
    if (self[0x20] != 0) {
        V3 d;
        VectorSub(&d, &c.x, &from.x);
        float sq = d.y * d.y + d.x * d.x + 0.0f * 0.0f;
        float left = sq - 9.0f;
        if (0.0f > left) {
            stop = false;
            V3 side = {-b.y, b.x, 0.0f};
            stepping = 1;
            V3 e;
            V3* r = VectorSub(&e, &from.x, &a.x);
            float dot = r->x * side.x + r->y * side.y + r->z * 0.0f;
            float* move = At<float*>(ped, 0x2E0);
            if (!(dot < 0.0f || dot != dot)) {
                move[0] = side.x;
                move[1] = side.y;
                move[2] = side.z;
            } else {
                move[0] = -side.x;
                move[1] = -side.y;
                move[2] = -0.0f;
            }
            At<float*>(ped, 0x2E0)[7] = 1.0f;
        }
    }
    if (stop) {
        Steering_Stop(Steering(ped));
    }
    self[0x48] = 0;
    if (self[0x18] != 0) {
        At<int32_t>(ped, 0x131C) = (self[0x20] != 0 && !stepping) ? 1 : 0;
    }
    return Timer_IsRunning(self + 0x4C);
}

// 0x62A0D0 (thiscall): RecruitAllySelfTrack::Start: the ped follows its target (a ped)
uint8_t __fastcall RecruitAllySelfTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* ped = OwnerPed(owner);
    uint8_t* target = Ped_GetTarget(ped);
    if (target && IsPedEntity(target)) {
        Ally_AddFollower(target + 0x2F0, ped, self[0x18]);
    }
    return 1;
}

// 0x62A110 (thiscall): DismissAllySelfTrack::Start: the ped leaves its leader
uint8_t __fastcall DismissAllySelfTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    Ally_Leave(OwnerPed(owner) + 0x2F0);
    return 1;
}

// 0x62A130 (thiscall): DismissAllySelfTrack::Update
uint8_t __fastcall DismissAllySelfTrack_Update(uint8_t* self, re::Edx, float) {
    Ally_Leave(OwnerPed(At<uint8_t*>(self, 0x18)) + 0x2F0);
    return 0;
}

// 0x62A150 (thiscall): SetMyTargetToMyLeadersTargetTrack::Start: the leader's target
// taken, and alerted to (when a ped)
uint8_t __fastcall SetMyTargetToMyLeadersTargetTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (!ped || !At<uint8_t*>(ped, 0x2F4)) {
        return 1;
    }
    uint8_t* target = Ped_GetTarget(At<uint8_t*>(ped, 0x2F4));
    Ped_SetTargetEntity(ped, target);
    Scanner_Alert(ped + 0x3BC, target && IsPedEntity(target) ? target : nullptr);
    return 1;
}

// 0x62A1C0 (thiscall): AttackMyLeadersTargetTrack::Start: an attack-ped objective on the
// leader's target (a ped; kind 2 with +0x19, else 3), queued last (+0x1A) or first
uint8_t __fastcall AttackMyLeadersTargetTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* ped = OwnerPed(owner);
    uint8_t* leader = At<uint8_t*>(ped, 0x2F4);
    if (!leader) {
        return 1;
    }
    uint8_t* target = Ped_GetTarget(leader);
    if (!target || !IsPedEntity(target)) {
        return 1;
    }
    int kind = self[0x19] != 0 ? 2 : 3;
    void* mem = Malloc(0x1C);
    uint8_t* objective = mem ? AttackPedObjective_Construct(mem, ped, target, kind) : nullptr;
    if (self[0x1A] != 0) {
        Queue_Add(ped + 0x5F0, objective);
    } else {
        Queue_Prepend(ped + 0x5F0, objective);
    }
    return 1;
}

// 0x62A250 (thiscall): PedSetConversationStanceTrack::Start: the speaker +0x18's stance
// (+0x1E70, +0x1E74) from +0x1C, +0x20
uint8_t __fastcall PedSetConversationStanceTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    uint8_t* speaker = Conversations_GetSpeaker(conversations, At<int32_t>(self, 0x18));
    if (!speaker) {
        return 0;
    }
    At<int32_t>(speaker, 0x1E70) = At<int32_t>(self, 0x1C);
    At<int32_t>(speaker, 0x1E74) = At<int32_t>(self, 0x20);
    return 1;
}

// 0x62A290 (thiscall): DisableMoveOutOfWayTrack::Start (the owner's taken from +0x18,
// not the argument)
uint8_t __fastcall DisableMoveOutOfWayTrack_Start(uint8_t* self, re::Edx, uint8_t*) {
    At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x18)), 0x1584) += 1;
    return 1;
}

// 0x62A2B0 (thiscall): DisableMoveOutOfWayTrack::End
uint8_t __fastcall DisableMoveOutOfWayTrack_End(uint8_t* self, re::Edx) {
    At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x18)), 0x1584) -= 1;
    return 1;
}

// 0x62A2C0 (thiscall): EquipRangedWeaponTrack::Start
uint8_t __fastcall EquipRangedWeaponTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    Combat_486CE0(OwnerPed(owner) + 0x668);
    return 0;
}

// 0x62A2E0 (thiscall): ClearCurrentObjectiveTrack::Update: objectives of type +0x18
// aborted
uint8_t __fastcall ClearCurrentObjectiveTrack_Update(uint8_t* self, re::Edx, float) {
    Queue_Abort(OwnerPed(At<uint8_t*>(self, 0x1C)) + 0x5F0, At<int32_t>(self, 0x18));
    return 0;
}

// 0x62A300 / 0x62A340 (thiscall): TargetAttackRights Acquire / Release tracks' Start:
// the target's (a ped's) attack rights for the ped (the owner isn't kept)
template <uintptr_t Fn>
uint8_t __fastcall TargetAttackRightsTrack_Start(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* target = Ped_GetTarget(OwnerPed(owner));
    if (target && IsPedEntity(target)) {
        reinterpret_cast<void(__thiscall*)(uint8_t*, uint8_t*)>(Fn)(target + 0x668, OwnerPed(owner));
    }
    return 0;
}

// 0x62A380 (thiscall): ClearLastVehicleUsedTrack::Start
uint8_t __fastcall ClearLastVehicleUsedTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (ped) {
        At<int32_t>(ped, 0x1560) = 0;
    }
    return 1;
}

// 0x62A3A0 (thiscall): TetherToTarget::Start: tethered to its target (a ped), radius
// +0x18
uint8_t __fastcall TetherToTarget_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!ped) {
        return 0;
    }
    uint8_t* target = Ped_GetTarget(ped);
    if (!target || !IsPedEntity(target)) {
        return 0;
    }
    Tether_SetEntity(OwnerPed(owner) + 0x358, target, At<float>(self, 0x18), zeroVector);
    return 1;
}

// 0x62A3F0 (thiscall): TetherClear::Start
uint8_t __fastcall TetherClear_Start(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!ped || At<int32_t>(ped, 0x360) == 0) {
        return 0;
    }
    Tether_Clear(ped + 0x358);
    return 1;
}

// 0x62A420 / 0x62A450 (thiscall): SetAllyState / SetAllyMorale tracks' Start: a
// follower's ally field from +0x18
template <int FieldAt>
uint8_t __fastcall SetAllyFieldTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!ped || !At<uint8_t*>(ped, 0x2F4)) {
        return 0;
    }
    At<int32_t>(ped, FieldAt) = At<int32_t>(self, 0x18);
    return 1;
}

// 0x62A480 (thiscall): SlingerObjectiveTrack::Start: the slinger objective's (0x20)
// value +0x18, a new one queued last without one
uint8_t __fastcall SlingerObjectiveTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* found = Queue_FindOfType(OwnerPed(At<uint8_t*>(self, 0x1C)) + 0x5F0, 0x20);
    if (found && At<int32_t>(found, 8) == 0x20) {
        At<int32_t>(found, 0x10) = At<int32_t>(self, 0x18);
        return 1;
    }
    void* mem = Malloc(0x14);
    uint8_t* objective = nullptr;
    if (mem) {
        objective = SlingerObjective_Construct(mem, OwnerPed(At<uint8_t*>(self, 0x1C)), At<int32_t>(self, 0x18));
    }
    Queue_Add(OwnerPed(At<uint8_t*>(self, 0x1C)) + 0x5F0, objective);
    return 1;
}

// 0x62A500 (thiscall): RegisterAsGirlfriendTrack::Start: the ped's model as one of the
// player's girlfriends in the yearbook (with +0x18: its bit cleared)
uint8_t __fastcall RegisterAsGirlfriendTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* ped = OwnerPed(owner);
    int32_t index = 0;
    if (!ped) {
        return 1;
    }
    Ped_GetGirlfriendIndex(playerPed, int(At<int16_t>(ped, 0x10E)), &index);
    if (index == 0) {
        return 1;
    }
    if (self[0x18] != 0) {
        uint32_t bits = PlayerInfo_GetYearbookBits(playerInfo, 0x1B);
        PlayerInfo_SetYearbookBits(playerInfo, 0x1B, ~uint32_t(index) & bits);
        return 1;
    }
    YearbookHud_RegisterGirlfriend(yearbookHud, index);
    return 1;
}

// 0x62A580 (thiscall): DeteriorateAttitudeToTargetTrack::Update: the attitude table
// worsened for the ped's type towards its target's (a ped), and its own attitude one
// down (when above 0)
uint8_t __fastcall DeteriorateAttitudeToTargetTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    if (!ped) {
        return 0;
    }
    uint8_t* target = Ped_GetTarget(ped);
    if (!target || !IsPedEntity(target)) {
        return 0;
    }
    AttitudeTable_Worsen(attitudeTable, At<int32_t>(ped, 0x1310), At<int32_t>(target, 0x1310));
    int32_t attitude;
    if (*Ped_GetAttitude(ped, &attitude, At<int32_t>(target, 0x1310)) > 0) {
        int32_t now = *Ped_GetAttitude(ped, &attitude, At<int32_t>(target, 0x1310));
        Ped_SetPedToTypeAttitude(ped, At<int32_t>(target, 0x1310), now - 1);
    }
    return 0;
}

void DestructReference(uint8_t* ref) {
    At<uint32_t>(ref, 0) = kVtReference;
    NameKey_Destruct(ref + 8);
    Node_Destruct(ref);
}
// a reference's node (looked up by path when not found)
bool ResolveReference(uint8_t* ref) {
    if (!Reference_Get0C(ref, -1)) {
        Reference_ResolveByPath(ref);
    }
    return Reference_Get0C(ref, -1) != nullptr;
}

// 0x62A610 (thiscall): SetTargetAsGrappleTargetTrack's deleting destructor (its two node
// references, +0x1C and +0x2C)
uint8_t* __fastcall SetTargetAsGrappleTargetTrack_DeletingDtor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kVtSetTargetAsGrappleTargetTrack;
    DestructReference(self + 0x2C);
    DestructReference(self + 0x1C);
    TrackBase_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x62A670 (thiscall): SetTargetAsGrappleTargetTrack::Resolve: both references' nodes
uint32_t __fastcall SetTargetAsGrappleTargetTrack_Resolve(uint8_t* self, re::Edx) {
    bool first = ResolveReference(self + 0x1C);
    bool second = ResolveReference(self + 0x2C);
    return first && second ? 1 : 0;
}

// 0x62A6E0 (thiscall): SetTargetAsGrappleTargetTrack::Start: the ped and its target (a
// ped not held by another, +0x1DD4) made grapple partners (+0x1500 from +0x18); the
// first reference's node played (queued with +0x19) on the ped, the second's (+0x1A) on
// the target
uint8_t __fastcall SetTargetAsGrappleTargetTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x3C) = owner;
    uint8_t* ped = OwnerPed(owner);
    uint8_t* target = PedTarget(ped);
    if (!ped || !target || At<int32_t>(target, 0x1DD4) != 0) {
        return 1;
    }
    Ped_SetGrapplePartner(ped, target);
    Ped_SetGrapplePartner(target, ped);
    At<int32_t>(ped, 0x1500) = self[0x18];
    At<int32_t>(target, 0x1500) = self[0x18];
    uint8_t* first = Reference_Get0C(self + 0x1C, -1);
    uint8_t* second = Reference_Get0C(self + 0x2C, -1);
    if (first) {
        uint8_t* node = vc::Call<uint8_t*>(first, acttree::kNodeFindPlayable, At<uint8_t*>(self, 0x3C));
        if (node) {
            uint8_t* controller = Inputs(ped) + 0x258;
            if (self[0x19] != 0) {
                Ctl_QueueTracks(controller, node);
            } else {
                Ctl_PlayNode(controller, node);
            }
        }
    }
    if (second) {
        uint8_t* node = vc::Call<uint8_t*>(second, acttree::kNodeFindPlayable, Inputs(target) + 0x300);
        if (node) {
            uint8_t* controller = Inputs(target) + 0x258;
            if (self[0x1A] != 0) {
                Ctl_QueueTracks(controller, node);
            } else {
                Ctl_PlayNode(controller, node);
            }
        }
    }
    Ped_SetGrapplePartner(ped, target);
    Ped_SetGrapplePartner(target, ped);
    return 1;
}

// 0x62D780 (thiscall): PropReleaseUsageStateTrack::End: the ped's prop anim let go
// (+0x548; its reference too unless +0x1418); a type-0xD ped's prop too
uint8_t __fastcall PropReleaseUsageStateTrack_End(uint8_t* self, re::Edx) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    re::Ptr32<uint8_t>* anim = &At<uint8_t*>(ped, 0x548);
    if (*anim) {
        Prop_ReleasePed(*anim, ped);
    }
    if (At<int32_t>(ped, 0x1418) == 0 && *anim) {
        Entity_CleanUpOldReference(*anim, anim);
        *anim = nullptr;
    }
    if (At<int32_t>(ped, 0x1310) == 0xD) {
        re::Ptr32<uint8_t>* prop = &At<uint8_t*>(ped, 0x438);
        if (*prop) {
            Entity_CleanUpOldReference(*prop, prop);
            *prop = nullptr;
        }
        At<int32_t>(ped, 0x43C) = 0;
    }
    return 1;
}

// 0x62DD00 / 0x62DD50 (thiscall): SetTargetSemaphoreTrack::Start / Update: the target's
// (a ped's) semaphore for the ped, +0x18 s
uint8_t SetSemaphore(uint8_t* self) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x1C));
    uint8_t* target = Ped_GetTarget(ped);
    if (target && IsPedEntity(target)) {
        Combat_SetSemaphore(target + 0x668, ped, At<float>(self, 0x18));
    }
    return 1;
}
uint8_t __fastcall SetTargetSemaphoreTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    return SetSemaphore(self);
}
uint8_t __fastcall SetTargetSemaphoreTrack_Update(uint8_t* self, re::Edx, float) {
    return SetSemaphore(self);
}

// 0x62DD90 (thiscall): TandemHelperClearTrack::Start: the grapple pair's references
// cleared (the grappler's +0x9B4 and the ped's +0x9B8; the owner isn't kept)
uint8_t __fastcall TandemHelperClearTrack_Start(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* grappler = At<uint8_t*>(OwnerPed(owner), 0x9B8);
    if (grappler) {
        re::Ptr32<uint8_t>* ref = &At<uint8_t*>(grappler, 0x9B4);
        if (*ref) {
            Entity_CleanUpOldReference(*ref, ref);
            *ref = nullptr;
        }
    }
    re::Ptr32<uint8_t>* mine = &At<uint8_t*>(OwnerPed(owner), 0x9B8);
    if (*mine) {
        Entity_CleanUpOldReference(*mine, mine);
        *mine = nullptr;
    }
    return 0;
}

// the ped's pool handle (its slot << 8 | the slot's reuse count)
int32_t PedHandle(uint8_t* ped) {
    CPool<uint8_t>* pool = pedPool;
    int32_t slot = int32_t((ped - re::PtrCast<uint8_t*>(pool->objects)) / pool->elemSize);
    return (slot << 8) + pool->flags[slot];
}

// 0x62DDF0 (thiscall): AddBlipTrack::Start: the ped's radar blip (kind 2) remade, size 5
uint8_t __fastcall AddBlipTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (!ped) {
        return 1;
    }
    int32_t handle = PedHandle(ped);
    uint8_t* radar = hudRadar;
    Radar_RemoveBlipFor(radar, 2, handle);
    if (!Radar_HasBlipFor(radar, handle)) {
        int32_t blip = Radar_AddBlipFor(radar, 2, handle, 0, 1, 2, 0);
        Radar_SetBlipSize(radar, blip, 5);
    }
    return 1;
}

// 0x62DE70 (thiscall): ClearBlipTrack::Start: the ped's radar blip removed
uint8_t __fastcall ClearBlipTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (ped) {
        Radar_RemoveBlipFor(hudRadar, 2, PedHandle(ped));
    }
    return 1;
}

} // namespace

#define AISOCIALTRK_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AISOCIALTRK_REVERSED("FleeObjectiveTrack::Start", 0x00628E40, &FleeObjectiveTrack_Start);
AISOCIALTRK_REVERSED("SeekToPickupTrack::Update", 0x00628EE0, &SeekToPickupTrack_Update);
AISOCIALTRK_REVERSED("PropSetUsageStateTrack::Start", 0x00628F70, &PropSetUsageStateTrack_Start);
AISOCIALTRK_REVERSED("SetActionTimerTrack::Update", 0x00628FB0, &SetActionTimerTrack_Update);
AISOCIALTRK_REVERSED("SeekToSnowTrack::Update", 0x006294C0, &SeekToSnowTrack_Update);
AISOCIALTRK_REVERSED("SeekToSurfaceTrack::Update", 0x00629560, &SeekToSurfaceTrack_Update);
AISOCIALTRK_REVERSED("PlayCatchObjectiveTrack::Start", 0x00629640, &BallObjectiveTrack_Start<0x004718C0>);
AISOCIALTRK_REVERSED("PassBallObjectiveTrack::Start", 0x006296C0, &BallObjectiveTrack_Start<0x00471990>);
AISOCIALTRK_REVERSED("ReceiveBallObjectiveTrack::Start", 0x00629740, &BallObjectiveTrack_Start<0x00471A60>);
AISOCIALTRK_REVERSED("RecruitAllyTrack::Update", 0x006297C0, &RecruitAllyTrack_Update);
AISOCIALTRK_REVERSED("DismissAllyTrack::Update", 0x00629810, &DismissAllyTrack_Update);
AISOCIALTRK_REVERSED("NeverOffscreenTrack::Start", 0x00629830, &NeverOffscreenTrack_Start);
AISOCIALTRK_REVERSED("NeverOffscreenTrack::End", 0x00629850, &NeverOffscreenTrack_End);
AISOCIALTRK_REVERSED("SocializeTrack::Update", 0x00629870, &SocializeTrack_Update);
AISOCIALTRK_REVERSED("SocializeActionCompleteTrack::Update", 0x00629890, &SocializeActionCompleteTrack_Update);
AISOCIALTRK_REVERSED("PlayerSocializingTrack::Start", 0x006298B0, &PlayerSocializingTrack_Start);
AISOCIALTRK_REVERSED("PlayerSocializingTrack::End", 0x00629930, &PlayerSocializingTrack_End);
AISOCIALTRK_REVERSED("SetEmotionTowardsTargetTrack::Start", 0x006299C0, &SetEmotionTowardsTargetTrack_Start);
AISOCIALTRK_REVERSED("SetEmotionTowardsPlayerTrack::Start", 0x00629A30, &SetEmotionTowardsPlayerTrack_Start);
AISOCIALTRK_REVERSED("SetWantsToSocializeWithTargetTrack::Start", 0x00629A60,
                     &SetWantsToSocializeWithTargetTrack_Start);
AISOCIALTRK_REVERSED("SetSocialWaitTimeTrack::Start", 0x00629AD0, &SetSocialWaitTimeTrack_Start);
AISOCIALTRK_REVERSED("StopSocializingTrack::Start", 0x00629AF0, &SocialCallTrack_Start<0x004910A0>);
AISOCIALTRK_REVERSED("IncKissCountTrack::Start", 0x00629B10, &SocialCallTrack_Start<0x0048E650>);
AISOCIALTRK_REVERSED("GiveArmorBonusTrack::Start", 0x00629B30, &GiveArmorBonusTrack_Start);
AISOCIALTRK_REVERSED("GiveArmorBonusTrack::Update", 0x00629B60, &GiveArmorBonusTrack_Update);
AISOCIALTRK_REVERSED("GiveArmorBonusTrack::End", 0x00629BE0, &GiveArmorBonusTrack_End);
AISOCIALTRK_REVERSED("TargetNearestCarTrack::Update", 0x00629C10, &TargetNearestCarTrack_Update);
AISOCIALTRK_REVERSED("TargetPlayerTrack::Update", 0x00629D00, &TargetPlayerTrack_Update);
AISOCIALTRK_REVERSED("EvadeGrenadeTrack::Update", 0x00629D30, &EvadeGrenadeTrack_Update);
AISOCIALTRK_REVERSED("EvadeProjectileTrack::End", 0x00629E40, &EvadeProjectileTrack_End);
AISOCIALTRK_REVERSED("EvadeProjectileTrack::Update", 0x00629E60, &EvadeProjectileTrack_Update);
AISOCIALTRK_REVERSED("RecruitAllySelfTrack::Start", 0x0062A0D0, &RecruitAllySelfTrack_Start);
AISOCIALTRK_REVERSED("DismissAllySelfTrack::Start", 0x0062A110, &DismissAllySelfTrack_Start);
AISOCIALTRK_REVERSED("DismissAllySelfTrack::Update", 0x0062A130, &DismissAllySelfTrack_Update);
AISOCIALTRK_REVERSED("SetMyTargetToMyLeadersTargetTrack::Start", 0x0062A150,
                     &SetMyTargetToMyLeadersTargetTrack_Start);
AISOCIALTRK_REVERSED("AttackMyLeadersTargetTrack::Start", 0x0062A1C0, &AttackMyLeadersTargetTrack_Start);
AISOCIALTRK_REVERSED("PedSetConversationStanceTrack::Start", 0x0062A250, &PedSetConversationStanceTrack_Start);
AISOCIALTRK_REVERSED("DisableMoveOutOfWayTrack::Start", 0x0062A290, &DisableMoveOutOfWayTrack_Start);
AISOCIALTRK_REVERSED("DisableMoveOutOfWayTrack::End", 0x0062A2B0, &DisableMoveOutOfWayTrack_End);
AISOCIALTRK_REVERSED("EquipRangedWeaponTrack::Start", 0x0062A2C0, &EquipRangedWeaponTrack_Start);
AISOCIALTRK_REVERSED("ClearCurrentObjectiveTrack::Update", 0x0062A2E0, &ClearCurrentObjectiveTrack_Update);
AISOCIALTRK_REVERSED("TargetAttackRightsAcquireTrack::Start", 0x0062A300,
                     &TargetAttackRightsTrack_Start<0x00486D90>);
AISOCIALTRK_REVERSED("TargetAttackRightsReleaseTrack::Start", 0x0062A340,
                     &TargetAttackRightsTrack_Start<0x00486310>);
AISOCIALTRK_REVERSED("ClearLastVehicleUsedTrack::Start", 0x0062A380, &ClearLastVehicleUsedTrack_Start);
AISOCIALTRK_REVERSED("TetherToTarget::Start", 0x0062A3A0, &TetherToTarget_Start);
AISOCIALTRK_REVERSED("TetherClear::Start", 0x0062A3F0, &TetherClear_Start);
AISOCIALTRK_REVERSED("SetAllyStateTrack::Start", 0x0062A420, &SetAllyFieldTrack_Start<0x318>);
AISOCIALTRK_REVERSED("SetAllyMoraleTrack::Start", 0x0062A450, &SetAllyFieldTrack_Start<0x31C>);
AISOCIALTRK_REVERSED("SlingerObjectiveTrack::Start", 0x0062A480, &SlingerObjectiveTrack_Start);
AISOCIALTRK_REVERSED("RegisterAsGirlfriendTrack::Start", 0x0062A500, &RegisterAsGirlfriendTrack_Start);
AISOCIALTRK_REVERSED("DeteriorateAttitudeToTargetTrack::Update", 0x0062A580, &DeteriorateAttitudeToTargetTrack_Update);
AISOCIALTRK_REVERSED("SetTargetAsGrappleTargetTrack::DeletingDestruct", 0x0062A610,
                     &SetTargetAsGrappleTargetTrack_DeletingDtor);
AISOCIALTRK_REVERSED("SetTargetAsGrappleTargetTrack::Resolve", 0x0062A670, &SetTargetAsGrappleTargetTrack_Resolve);
AISOCIALTRK_REVERSED("SetTargetAsGrappleTargetTrack::Start", 0x0062A6E0, &SetTargetAsGrappleTargetTrack_Start);
AISOCIALTRK_REVERSED("PropReleaseUsageStateTrack::End", 0x0062D780, &PropReleaseUsageStateTrack_End);
AISOCIALTRK_REVERSED("SetTargetSemaphoreTrack::Start", 0x0062DD00, &SetTargetSemaphoreTrack_Start);
AISOCIALTRK_REVERSED("SetTargetSemaphoreTrack::Update", 0x0062DD50, &SetTargetSemaphoreTrack_Update);
AISOCIALTRK_REVERSED("TandemHelperClearTrack::Start", 0x0062DD90, &TandemHelperClearTrack_Start);
AISOCIALTRK_REVERSED("AddBlipTrack::Start", 0x0062DDF0, &AddBlipTrack_Start);
AISOCIALTRK_REVERSED("ClearBlipTrack::Start", 0x0062DE70, &ClearBlipTrack_Start);
