// The action tree's AI fight tracks: pursuit, attack (the ped's combos, blocks and
// dodges by what its target plays), action requests, button history, node requests
// (dialog), block, evade, fight tokens, attack objectives (ped, prop, vehicle), set
// target / stationary, ranged weapon fire, pass ball, attack prop / vehicle, projectile
// cover, manage targets, set health. 0x625D40..0x62E51D (the combat locomotion track is
// in AiCombatLocoTracks.cpp).
// The ped: +0x2E0 its inputs (ActionInputs; its +0x258 the action controller the
// IsPlaying tests use), +0x3BC its scanner, +0x4D0 / +0x4D4 / +0x550 when it last
// blocked / evaded / attacked (ms), +0x4E4 stationary, +0x5F0 its objectives, +0x654
// its steering, +0x668 its combat slots, +0xA64 its sight / social, +0x1340 / +0x131C
// its moving flags.

#include <cstdint>

#include "game/ActionTracksCommon.h"

#include "game/Crt.h"
#include "reversed.h"
using namespace actc;

namespace {

// the fight nodes the tracks look for (name keys)
inline const uint32_t* const nodeAttack = reinterpret_cast<const uint32_t*>(0x01B65260);
inline const uint32_t* const nodeBlockA = reinterpret_cast<const uint32_t*>(0x01B65268);
inline const uint32_t* const nodeBlockB = reinterpret_cast<const uint32_t*>(0x01B6526C);
inline const uint32_t* const nodeCombo = reinterpret_cast<const uint32_t*>(0x01B65270);
inline const uint32_t* const nodeComboEnd = reinterpret_cast<const uint32_t*>(0x01B65278);
GAME_VAR(int32_t, attackComboLength, 0x00B0C25C);
GAME_VAR(uint8_t*, buttonHistory, 0x00C674F4);
GAME_VAR(uint8_t*, conversation, 0x01B8919C);
GAME_VAR(uint8_t*, punishment, 0x00C0F78C);
GAME_VAR(uint32_t, coverGuard, 0x01B65724);
inline void* const conversations = reinterpret_cast<void*>(0x00B9DAB8);
inline void* const pathBlockers = reinterpret_cast<void*>(0x00C2AB70);
inline uint8_t* const staticCover = reinterpret_cast<uint8_t*>(0x01B656B8);   // (its own cover objective)
inline void* const staticCoverAtExit = reinterpret_cast<void*>(0x008F6290);
GAME_FN(void, __thiscall, ActionObjective_Destruct, 0x00471190, void* self);

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
inline int __cdecl AtExit(void* fn) { return crt::Atexit((void(__cdecl*)())fn); }
inline int __cdecl GameRand() { return crt::Rand(); }
GAME_FN(int, __cdecl, GetFrameMs, 0x00446870);
GAME_FN(float, __cdecl, RandomFloat, 0x0041A5B0, float low, float high);
GAME_FN(uint8_t*, __thiscall, Queue_GetCurrent, 0x00470340, uint8_t* queue);
GAME_FN(void, __thiscall, Queue_RemoveFirst, 0x004702C0, uint8_t* queue);
GAME_FN(uint8_t, __thiscall, Queue_Prepend, 0x00471470, uint8_t* queue, uint8_t* objective);
GAME_FN(uint8_t, __thiscall, Queue_Add, 0x00471390, uint8_t* queue, uint8_t* objective);
GAME_FN(uint8_t*, __thiscall, AttackPedObjective_Construct, 0x00470D60, void* self, uint8_t* ped, uint8_t* target,
        int kind);
GAME_FN(uint8_t*, __thiscall, AttackVehicleObjective_Construct, 0x00470DB0, void* self, uint8_t* ped, uint8_t* vehicle);
GAME_FN(uint8_t*, __thiscall, BreakPropObjective_Construct, 0x00470E50, void* self, uint8_t* ped, uint8_t* prop,
        int value, float a, float b, float c);
GAME_FN(uint8_t*, __thiscall, CoverObjective_Construct, 0x004710B0, void* self, uint8_t* ped, uint8_t* target, int a,
        int b);
GAME_FN(uint8_t, __thiscall, Objective_BallTimeUp, 0x004711D0, uint8_t* objective);
GAME_FN(uint8_t*, __thiscall, Objective_GetEntity, 0x0077E8E0, uint8_t* objective);
GAME_FN(void, __thiscall, ActionInputs_SetRequest, 0x005F3A90, uint8_t* inputs, int button, int kind, float time);
GAME_FN(uint8_t, __thiscall, ActionInputs_IsHeld, 0x005F3700, uint8_t* inputs, int button);
GAME_FN(void, __thiscall, ActionInputs_RequestNode, 0x005F3740, uint8_t* inputs, const uint32_t* name);
GAME_FN(uint8_t, __thiscall, Ctl_IsPlaying, 0x005F4800, uint8_t* controller, const uint32_t* name, int ancestors,
        int nested);
GAME_FN(uint8_t*, __thiscall, Scanner_48C030, 0x0048C030, uint8_t* scanner);
GAME_FN(uint8_t*, __thiscall, Scanner_Nearest, 0x0048C060, uint8_t* scanner);
GAME_FN(void, __thiscall, Scanner_Watch, 0x0048C570, uint8_t* scanner, uint8_t* ped);
GAME_FN(void, __thiscall, Scanner_ClearRing, 0x0048BF80, uint8_t* scanner);
GAME_FN(void, __thiscall, Scanner_SetBallTarget, 0x0062A840, uint8_t* scanner, uint8_t* entity);
GAME_FN(void, __thiscall, Ped_SetTargetEntity, 0x00498B10, uint8_t* ped, uint8_t* target);
GAME_FN(void, __thiscall, Ped_SetTarget, 0x00497E70, uint8_t* ped, uint8_t* target, int kind, const float* offset);
GAME_FN(uint8_t*, __thiscall, Ped_ChooseTargetIfAsked, 0x00499470, uint8_t* ped);
GAME_FN(void, __thiscall, Ped_SetAimPoint, 0x0061A900, uint8_t* ped, const float* pos);
GAME_FN(V3*, __thiscall, Ped_GetAimPoint, 0x00496EC0, uint8_t* ped, V3* out);
GAME_FN(void, __thiscall, Ped_SetMoveTarget, 0x00474A00, uint8_t* ped, const float* pos);
GAME_FN(uint8_t, __thiscall, Ped_HasWeapon, 0x00474BF0, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Ped_IsStateIdle, 0x0041AA50, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Ped_EveryNthFrame, 0x00465A60, uint8_t* ped, uint32_t n);
GAME_FN(uint8_t, __thiscall, Ped_DropAttackOn, 0x00496C00, uint8_t* ped, uint8_t* target);
GAME_FN(void, __thiscall, Ped_SetHealth, 0x004782E0, uint8_t* ped, float health);
GAME_FN(void, __thiscall, Ped_AddHealth, 0x00474CC0, uint8_t* ped, float health);
GAME_FN(void, __thiscall, Ped_TakeHealth, 0x004783F0, uint8_t* ped, float amount);
GAME_FN(void, __thiscall, Social_48D7F0, 0x0048D7F0, uint8_t* social);
GAME_FN(void, __thiscall, Social_CheckHurt, 0x00492AA0, uint8_t* social);
GAME_FN(uint8_t, __thiscall, Social_CanTalkTo, 0x0048D470, uint8_t* social, uint8_t* ped);
GAME_FN(void, __thiscall, Sight_SetEye, 0x0048CE00, uint8_t* sight, const float* pos);
GAME_FN(void, __thiscall, Sight_ClearEye, 0x0048CE30, uint8_t* sight);
GAME_FN(float, __thiscall, Weapon_GetRangeFactor, 0x004D96B0, uint8_t* weapon);
GAME_FN(uint8_t, __thiscall, Combat_HasUsableWeapon, 0x00486BE0, uint8_t* combat);
GAME_FN(void, __thiscall, Combat_486CE0, 0x00486CE0, uint8_t* combat);
GAME_FN(uint8_t, __thiscall, Combat_InAttackCone, 0x004866F0, uint8_t* combat, uint8_t* target);
GAME_FN(void, __thiscall, Combat_RemoveFromSlots, 0x00486A30, uint8_t* combat, uint8_t* ped);
GAME_FN(void, __thiscall, Combat_ReleaseSlots, 0x00486AB0, uint8_t* combat);
GAME_FN(void, __thiscall, Combat_LockSlots, 0x00486B10, uint8_t* combat, int32_t ms);
GAME_FN(uint8_t, __thiscall, LineOfFire_IsClear, 0x0048A700, uint8_t* combat, const float* from, uint8_t* target,
        int flags);
GAME_FN(void, __thiscall, ButtonHistory_AddPress, 0x0053B330, uint8_t* history, int buffer, int button, int held);
GAME_FN(uint8_t*, __thiscall, Conversations_GetSpeaker, 0x006A29F0, void* conversations, int32_t which);
GAME_FN(uint8_t, __thiscall, Steering_GoTo, 0x004A5000, uint8_t* steering, float x, float y, float z, uint32_t flags,
        float speed, float radius, uint8_t* entity, int flag);
GAME_FN(uint8_t, __thiscall, Steering_GoToPed, 0x004A5150, uint8_t* steering, uint8_t* ped, uint32_t flags,
        float speed, float radius);
GAME_FN(uint8_t, __thiscall, Steering_GoToEntity, 0x004A5360, uint8_t* steering, uint8_t* entity, uint32_t flags,
        float speed, float radius);
GAME_FN(V3*, __thiscall, Steering_ToPoint, 0x004A48A0, uint8_t* steering, V3* out, const float* point);
GAME_FN(uint8_t, __thiscall, PathBlockers_Blocks, 0x0046EFB0, void* blockers, uint8_t* ped, const float* pos, int a);
GAME_FN(uint8_t, __thiscall, Nav_CopyPoint, 0x004852E0, uint8_t* follower, uint8_t* out);
GAME_FN(uint8_t, __cdecl, Nav_FindPointNear, 0x005EAEB0, const float* pos, uint8_t* out, float range, void* entity,
        uint8_t* offMesh, float drop);
GAME_FN(uint8_t, __cdecl, Nav_RouteBetween, 0x005E2620, const uint8_t* start, const uint8_t* goal, int flags,
        void* route, float maxDist);
GAME_FN(uint8_t, __thiscall, Vehicle_HasCollidedWith, 0x00468D40, uint8_t* vehicle, uint8_t* other);
GAME_FN(float, __cdecl, Distance, 0x0041A950, const float* a, const float* b);
GAME_FN(float, __cdecl, Distance2D, 0x00470110, const float* a, const float* b);
GAME_FN(float, __cdecl, DistanceBeyond, 0x0041A4D0, const float* a, const float* b, float range);
GAME_FN(float, __cdecl, DistanceSquaredBeyond, 0x00487E50, const float* a, const float* b, float range);
GAME_FN(V3*, __cdecl, CrossProduct, 0x00414730, V3* out, const V3* a, const V3* b);
GAME_FN(V3*, __cdecl, VectorAdd, 0x004129A0, V3* out, const float* a, const float* b);
GAME_FN(V3*, __cdecl, FlatDirection, 0x0041A520, V3* out, const V3* v, float length);
GAME_FN(uint8_t, __cdecl, VectorNotEqual, 0x004305F0, const float* a, const float* b);
GAME_FN(uint8_t, __thiscall, Timer_IsDone, 0x0045BDD0, uint8_t* timer);
GAME_FN(void, __thiscall, EntityRef_Set, 0x00478550, re::Ptr32<uint8_t>* ref, uint8_t* entity);
GAME_FN(void, __cdecl, Entity_RegisterReference, 0x004657D0, uint8_t* entity, re::Ptr32<uint8_t>* ref);
GAME_FN(void, __thiscall, Targeting_SetTarget, 0x004D79E0, uint8_t* targeting, uint8_t* target);
GAME_FN(void, __thiscall, NamedRef_Set, 0x00471300, uint8_t* ref, uint8_t* entity, int value);
GAME_FN(float*, __thiscall, Route_First, 0x006D59B0, uint8_t* route);
GAME_FN(float*, __thiscall, Route_Next, 0x006D59F0, uint8_t* route);
// the track's own, by address (so their modes apply)
GAME_FN(uint8_t, __thiscall, DialogActionNodeRequestTrack_Play, 0x00626350, uint8_t* self, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, ProjectileCoverTrack_ChooseCover, 0x0061BE50, uint8_t* self);
GAME_FN(void, __thiscall, ProjectileCoverTrack_SetState, 0x0061BF30, uint8_t* self, int32_t state);
GAME_FN(void, __thiscall, ProjectileCoverTrack_UpdateState, 0x006270C0, uint8_t* self);
GAME_FN(uint8_t, __thiscall, ProjectileCoverTrack_CheckAttack, 0x00627410, uint8_t* self);
GAME_FN(uint8_t, __thiscall, ProjectileCoverTrack_Refresh, 0x0062CDA0, uint8_t* self);

uint8_t* Inputs(uint8_t* ped) {
    return At<uint8_t*>(ped, 0x2E0);
}
// the controller the fight tests look at (the inputs' +0x258)
uint8_t* FightController(uint8_t* ped) {
    return At<uint8_t*>(ped, 0x2E0) + 0x258;
}
// the ped's target when it's a ped
uint8_t* PedTarget(uint8_t* ped) {
    uint8_t* target = Ped_GetTarget(ped);
    return target && IsPedEntity(target) ? target : nullptr;
}
float RandomFraction() {
    return float(GameRand()) / 32767.0f * (1.0f - 0.0f) + 0.0f;
}
// a delay in ms: now + seconds (the game's truncation of seconds * 1000)
uint32_t After(float seconds, float scale) {
    int32_t ms = int32_t(int64_t(seconds * -1000.0f * scale));
    return timeInMs - uint32_t(ms);
}

// 0x625D40 (thiscall): PursuitLocoTrack::Start
uint8_t __fastcall PursuitLocoTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<float>(self, 0x2C) = 0.0f;
    At<float>(self, 0x34) = 0.0f;
    At<float>(self, 0x38) = 0.0f;
    At<float>(self, 0x3C) = 0.0f;
    At<float>(self, 0x40) = 0.0f;
    At<uint8_t*>(self, 0x24) = owner;
    self[0x30] = 0;
    At<int32_t>(OwnerPed(owner), 0x1340) = 0;
    At<uint32_t>(self, 0x44) = timeInMs;
    return 1;
}

// 0x625DA0 (thiscall): PursuitLocoTrack::Update: after the scanner's pick (the player,
// made the target, first when +0x18) with flags 0x2001C, speed +0x1C, radius +0x20; a
// stationary ped doesn't move. Done when the steering is.
uint8_t __fastcall PursuitLocoTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x24));
    if (self[0x18] != 0) {
        uint8_t* player = playerPed;
        At<uint8_t*>(self, 0x28) = player;
        Ped_SetTargetEntity(ped, player);
    }
    uint8_t* chased = Scanner_48C030(ped + 0x3BC);
    At<uint8_t*>(self, 0x28) = chased;
    if (!chased) {
        return 0;
    }
    uint8_t going = Steering_GoToPed(ped + 0x654, chased, 0x2001C, At<float>(self, 0x1C), At<float>(self, 0x20));
    if (ped[0x4E4] != 0) {
        At<float*>(ped, 0x2E0)[7] = 0.0f;
    }
    return going != 0 ? 1 : 0;
}

// 0x625E40 (thiscall): PursuitLocoTrack::End (also ChaseLoco's and AttackVehicle's)
uint8_t __fastcall PursuitLocoTrack_End(uint8_t* self, re::Edx) {
    At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x24)), 0x1340) = 0;
    return 1;
}

// 0x625E60 (thiscall): AttackTrack::Update: at its target (a ped, +0x2C): beyond +0x1C
// it may lunge (0xB, 1 in 200); in reach it attacks after a random wait (+0x24 * +0x18
// ms), carries a combo on while the target blocks (0xC after +0x3C hits, reset at the
// combo length by chance or at once when the target combos), or, the target attacking
// within 2 (every 2 s), dodges (4) sideways or back
uint8_t __fastcall AttackTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x28));
    uint8_t* target = PedTarget(ped);
    At<uint8_t*>(self, 0x2C) = target;
    if (!target) {
        return 0;
    }
    V3 d;
    V3* r = VectorSub(&d, Pos(target), Pos(ped));
    float dist = SqrtAbs(r->x * r->x + r->y * r->y);
    V3 to;
    Steering_ToPoint(ped + 0x654, &to, Pos(At<uint8_t*>(self, 0x2C)));
    float chance = RandomFraction();
    if (At<float>(self, 0x1C) < dist) {
        self[0x30] = 1;
        if (0.005f > chance) {
            ActionInputs_SetRequest(Inputs(ped), 0xB, 1, 0.0f);
        }
        return 1;
    }
    if (Ctl_IsPlaying(FightController(ped), nodeAttack, 1, 1)) {
        At<int32_t>(self, 0x38) = 0;
    }
    if (!Ctl_IsPlaying(FightController(ped), nodeAttack, 1, 1) && At<int32_t>(self, 0x38) == 0) {
        float wait = float(At<int32_t>(self, 0x24)) * chance * At<float>(self, 0x18);
        At<int32_t>(self, 0x38) = int32_t(int64_t(wait)) + int32_t(timeInMs);
    }
    target = At<uint8_t*>(self, 0x2C);
    if (At<int32_t>(target, 0x1344) != 0 || At<int32_t>(target, 0x138C) != 0) {
        return 1;
    }
    uint8_t blockA = Ctl_IsPlaying(FightController(target), nodeBlockA, 1, 1);
    uint8_t blockB = Ctl_IsPlaying(FightController(At<uint8_t*>(self, 0x2C)), nodeBlockB, 1, 1);
    if (blockA && !blockB) {
        At<int32_t>(self, 0x3C) += 1;
        if (At<int32_t>(self, 0x3C) == attackComboLength && 0.75f < chance) {
            At<int32_t>(self, 0x3C) = 0;
        }
        if (Ctl_IsPlaying(FightController(At<uint8_t*>(self, 0x2C)), nodeCombo, 1, 1)) {
            At<int32_t>(self, 0x3C) = 10;
        }
        if (At<int32_t>(self, 0x3C) >= 10) {
            ActionInputs_SetRequest(Inputs(ped), 0xC, 1, 0.0f);
        }
        if (Ctl_IsPlaying(FightController(ped), nodeComboEnd, 1, 1)) {
            At<int32_t>(self, 0x38) = -1;
        }
        return 1;
    }
    if (!blockA && !blockB) {
        if (int32_t(timeInMs) > At<int32_t>(self, 0x38)) {
            ActionInputs_SetRequest(Inputs(ped), chance < 0.8f ? 1 : 2, 1, 0.0f);
        }
        return 1;
    }
    if (!(2.0f > dist) || !(At<uint32_t>(self, 0x34) < timeInMs - 2000u)) {
        return 1;
    }
    V3 up = {0.0f, 0.0f, 1.0f};
    V3 side;
    CrossProduct(&side, &to, &up);
    ActionInputs_SetRequest(Inputs(ped), 4, 1, 0.0f);
    float* move = At<float*>(ped, 0x2E0);
    if (chance < 0.33f) {
        move[0] = -to.x;
        move[1] = -to.y;
        move[2] = -to.z;
    } else if (chance < 0.66f) {
        move[0] = side.x;
        move[1] = side.y;
        move[2] = side.z;
    } else {
        move[0] = -side.x;
        move[1] = -side.y;
        move[2] = -side.z;
    }
    At<uint32_t>(self, 0x34) = timeInMs;
    At<int32_t>(self, 0x38) = -1;
    return 1;
}

// 0x626230 (thiscall): ActionRequestTrack::Update: the button (+0x18) requested (kind
// +0x1C); attack buttons (1, 2, 3, 8) note the attack time while attacking, 0xC the
// block time; still going while +0x20
uint8_t __fastcall ActionRequestTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x24));
    int32_t button = At<int32_t>(self, 0x18);
    ActionInputs_SetRequest(Inputs(ped), button, At<int32_t>(self, 0x1C), 0.0f);
    if (button == 1 || button == 2 || button == 3 || button == 8) {
        if (Ctl_IsPlaying(At<uint8_t*>(At<uint8_t*>(self, 0x24), 0), nodeAttack, 1, 1)) {
            At<uint32_t>(ped, 0x550) = timeInMs;
        }
    } else if (button == 0xC) {
        At<uint32_t>(ped, 0x4D0) = timeInMs;
    }
    return self[0x20] != 0 ? 1 : 0;
}

// 0x6262C0 (thiscall): ActionRequestTrack::End: a held request (kind 2) released (3)
uint8_t __fastcall ActionRequestTrack_End(uint8_t* self, re::Edx) {
    if (At<int32_t>(self, 0x1C) == 2) {
        ActionInputs_SetRequest(Inputs(OwnerPed(At<uint8_t*>(self, 0x24))), At<int32_t>(self, 0x18), 3, 0.0f);
    }
    return 1;
}

// 0x6262F0 (thiscall): ActionRegisterButtonHistoryTrack::Update: the button (+0x20;
// the test lets any through) pressed into the HUD's history buffer +0x1C (0..3) once
uint8_t __fastcall ActionRegisterButtonHistoryTrack_Update(uint8_t* self, re::Edx, float) {
    if (OwnerPed(At<uint8_t*>(self, 0x28))) {
        uint8_t* history = buttonHistory;
        int32_t button = At<int32_t>(self, 0x20);
        uint32_t buffer = At<uint32_t>(self, 0x24);
        int32_t which = At<int32_t>(self, 0x1C);
        if (history && history[0x6D] == 0 && (button > 0 || button < 0x19) && buffer <= 3 && self[0x30] == 0) {
            ButtonHistory_AddPress(history, which, button, self[0x18]);
            self[0x30] = 1;
        }
    }
    return 0;
}

// 0x626350 (thiscall): DialogActionNodeRequestTrack's play: the node (+0x18) requested
// of the ped once (+0x24 while it plays); still going while it plays with +0x1D, or
// when it didn't start with +0x1C / +0x1D; else notes the attack time and goes on by
// +0x1E
uint8_t __fastcall DialogActionNodeRequestTrack_PlayImpl(uint8_t* self, re::Edx, uint8_t* ped) {
    if (!ped) {
        return 0;
    }
    bool check = true;
    if (self[0x24] == 0) {
        ActionInputs_RequestNode(Inputs(ped), &At<uint32_t>(self, 0x18));
        uint8_t playing = Ctl_IsPlaying(FightController(ped), &At<uint32_t>(self, 0x18), 1, 1);
        self[0x24] = playing;
        if (!playing) {
            if (self[0x1C] != 0 || self[0x1D] != 0) {
                return 1;
            }
            check = false;
        }
    }
    if (check && self[0x1D] != 0) {
        uint8_t playing = Ctl_IsPlaying(FightController(ped), &At<uint32_t>(self, 0x18), 1, 1);
        if (playing) {
            return 1;
        }
        self[0x24] = playing;
    }
    if (Ctl_IsPlaying(At<uint8_t*>(At<uint8_t*>(self, 0x20), 0), nodeAttack, 1, 1)) {
        At<uint32_t>(ped, 0x550) = timeInMs;
    }
    return self[0x1E] != 0 ? 1 : 0;
}

// 0x626400 (thiscall): ActionNodeRequestTrack::Update: the play for the owner's ped
uint8_t __fastcall ActionNodeRequestTrack_Update(uint8_t* self, re::Edx, float) {
    return DialogActionNodeRequestTrack_Play(self, OwnerPed(At<uint8_t*>(self, 0x20)));
}

// 0x626410 (thiscall): DialogActionNodeRequestTrack::Update: waits (going) until the
// conversation (0x1B8919C) is gone or done (+0x59); then plays on the speaker +0x28 (8
// the ped, 9 the ped's +0x155C, else the conversation's)
uint8_t __fastcall DialogActionNodeRequestTrack_Update(uint8_t* self, re::Edx, float) {
    if (self[0x2C] == 0) {
        uint8_t* talk = conversation;
        if (!talk || talk[0x59] != 0) {
            self[0x2C] = 1;
        }
        return 1;
    }
    int32_t which = At<int32_t>(self, 0x28);
    if (which == 8) {
        return DialogActionNodeRequestTrack_Play(self, OwnerPed(At<uint8_t*>(self, 0x20)));
    }
    uint8_t* speaker = nullptr;
    if (which == 9) {
        uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x20));
        if (ped && At<uint8_t*>(ped, 0x155C)) {
            return DialogActionNodeRequestTrack_Play(self, At<uint8_t*>(ped, 0x155C));
        }
    } else {
        speaker = Conversations_GetSpeaker(conversations, which);
    }
    return DialogActionNodeRequestTrack_Play(self, speaker);
}

// 0x626490 (thiscall): BlockTrack::Update: blocking (0xC held) against its target (a
// ped) for a random time up to +0x1C s (+0x24 ms, counted down while the target isn't
// in block A); the block time noted. Not with +0x18.
uint8_t __fastcall BlockTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x20));
    At<uint32_t>(ped, 0x4D0) = timeInMs;
    if (self[0x18] != 0) {
        return 0;
    }
    uint8_t* target = Ped_GetTarget(ped);
    if (!target || !IsPedEntity(target)) {
        return 0;
    }
    uint8_t blockA = Ctl_IsPlaying(FightController(target), nodeBlockA, 1, 1);
    Ctl_IsPlaying(FightController(target), nodeBlockB, 1, 1);
    if (At<int32_t>(self, 0x24) == 0) {
        float most = At<float>(self, 0x1C) * 1000.0f;
        At<int32_t>(self, 0x24) = int32_t(int64_t(RandomFloat(0.0f, most)));
    }
    if (!blockA) {
        At<int32_t>(self, 0x24) -= GetFrameMs();
    }
    ActionInputs_SetRequest(Inputs(ped), 0xC, 2, 0.0f);
    At<uint32_t>(ped, 0x4D0) = timeInMs;
    return 1;
}

// 0x626590 (thiscall): BlockTrack::End: the block released
uint8_t __fastcall BlockTrack_End(uint8_t* self, re::Edx) {
    ActionInputs_SetRequest(Inputs(OwnerPed(At<uint8_t*>(self, 0x20))), 0xC, 3, 0.0f);
    return 1;
}

// 0x6265B0 (thiscall): EvadeTrack::Update: a dodge (4) away from its target (a ped):
// back (+0x18 2), to its left (0) or right (else); the evade time noted
uint8_t __fastcall EvadeTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x1C));
    uint8_t* target = Ped_GetTarget(ped);
    if (!target || !IsPedEntity(target)) {
        return 0;
    }
    V3 to;
    Steering_ToPoint(ped + 0x654, &to, Pos(target));
    ActionInputs_SetRequest(Inputs(ped), 4, 1, 0.0f);
    float* move = At<float*>(ped, 0x2E0);
    if (At<int32_t>(self, 0x18) == 2) {
        move[0] = -to.x;
        move[1] = -to.y;
        move[2] = -to.z;
    } else {
        V3 up = {0.0f, 0.0f, 1.0f};
        V3 side;
        CrossProduct(&side, &to, &up);
        move = At<float*>(ped, 0x2E0);
        if (At<int32_t>(self, 0x18) == 0) {
            move[0] = side.x;
            move[1] = side.y;
            move[2] = side.z;
        } else {
            move[0] = -side.x;
            move[1] = -side.y;
            move[2] = -side.z;
        }
    }
    At<uint32_t>(ped, 0x4D4) = timeInMs;
    return 0;
}

// 0x6266E0 (thiscall): ReleaseFightTokenTrack::Update: out of its target's slots
uint8_t __fastcall ReleaseFightTokenTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x1C));
    uint8_t* target = Ped_GetTarget(ped);
    if (target && IsPedEntity(target)) {
        Combat_RemoveFromSlots(target + 0x668, ped);
    }
    return 0;
}

// 0x626720 (thiscall): EnableFightTokensTrack::Update: its slots released
uint8_t __fastcall EnableFightTokensTrack_Update(uint8_t* self, re::Edx, float) {
    Combat_ReleaseSlots(OwnerPed(At<uint8_t*>(self, 0x18)) + 0x668);
    return 0;
}

// 0x626740 (thiscall): DisableFightTokensTrack::Update: its slots locked for +0x18 s
// (0: the default)
uint8_t __fastcall DisableFightTokensTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x1C));
    int32_t ms = 0;
    if (0.0f < At<float>(self, 0x18)) {
        ms = int32_t(int64_t(At<float>(self, 0x18) * 1000.0f));
    }
    Combat_LockSlots(ped + 0x668, ms);
    return 0;
}

// 0x6267A0 (thiscall): AttackObjectiveTrack::Update: an attack-ped objective on the
// scanner's nearest (the player with +0x19; +0x1A the ped's +0x1404; its kind 3, or with
// +0x1C 1 when idle / +0x1570 is 2 else 0), queued last (+0x1B) or first. Not while
// +0x1360.
uint8_t __fastcall AttackObjectiveTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x20));
    uint8_t* target = Scanner_Nearest(ped + 0x3BC);
    if (At<int32_t>(ped, 0x1360) != 0) {
        return 0;
    }
    if (self[0x19] != 0) {
        target = playerPed;
    }
    if (!target) {
        return 0;
    }
    At<int32_t>(ped, 0x1404) = self[0x1A];
    int kind = 3;
    if (self[0x1C] != 0) {
        kind = (ped[0x1570] == 2 || Ped_IsStateIdle(ped)) ? 1 : 0;
    }
    void* mem = Malloc(0x1C);
    uint8_t* objective = mem ? AttackPedObjective_Construct(mem, ped, target, kind) : nullptr;
    if (self[0x1B] != 0) {
        Queue_Add(ped + 0x5F0, objective);
    } else {
        Queue_Prepend(ped + 0x5F0, objective);
    }
    return 0;
}

// 0x626850 (thiscall): AttackPropObjectiveTrack::Update: a break-prop objective on the
// ped's prop (+0x438, when its vf31 allows), queued last; unless +0x18 (which marks the
// objective's +0x24), only when the ped can walk there (its nav point, the prop's spot
// unblocked and on the meshes, a route between them)
uint8_t __fastcall AttackPropObjectiveTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x1C));
    uint8_t* prop = At<uint8_t*>(ped, 0x438);
    if (!prop || !vc::Call<uint8_t>(prop, 0x7C)) {
        return 0;
    }
    if (self[0x18] == 0) {
        const float* pp = Pos(ped);
        V3 from = {pp[0], pp[1], pp[2]};
        V3 spot[2];   // (the prop's vf25 fills its position; room left for more)
        vc::Call<void*>(prop, 0x64, &spot[0], &from);
        uint8_t start[0x14];
        uint8_t goal[0x14];
        At<int32_t>(goal, 0) = 0;
        At<int32_t>(goal, 4) = -1;
        At<float>(goal, 8) = 0.0f;
        At<float>(goal, 0xC) = 0.0f;
        At<float>(goal, 0x10) = 0.0f;
        At<int32_t>(start, 0) = 0;
        At<int32_t>(start, 4) = -1;
        At<float>(start, 8) = 0.0f;
        At<float>(start, 0xC) = 0.0f;
        At<float>(start, 0x10) = 0.0f;
        uint8_t offMesh = 1;
        if (!Nav_CopyPoint(ped + 0xDAC, start)) {
            return 0;
        }
        if (PathBlockers_Blocks(pathBlockers, ped, &spot[0].x, 0)) {
            return 0;
        }
        if (!Nav_FindPointNear(&spot[0].x, goal, 3.5f, nullptr, &offMesh, 50.0f)) {
            return 0;
        }
        if (!Nav_RouteBetween(start, goal, 0, nullptr, 0.3f)) {
            return 0;
        }
    }
    void* mem = Malloc(0x28);
    uint8_t* objective = mem ? BreakPropObjective_Construct(mem, ped, prop, 3, 0.0f, 0.0f, 0.0f) : nullptr;
    if (self[0x18] != 0 && objective) {
        objective[0x24] = 1;
    }
    Queue_Add(ped + 0x5F0, objective);
    return 0;
}

// 0x6269E0 (thiscall): AttackVehicleObjectiveTrack::Update: an attack-vehicle objective
// on the ped's vehicle (+0x430, a vehicle) queued last
uint8_t __fastcall AttackVehicleObjectiveTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    uint8_t* vehicle = At<uint8_t*>(ped, 0x430);
    if ((vehicle[0x108] & 7) == 2) {
        void* mem = Malloc(0x14);
        uint8_t* objective = mem ? AttackVehicleObjective_Construct(mem, ped, vehicle) : nullptr;
        Queue_Add(ped + 0x5F0, objective);
    }
    return 0;
}

// 0x626A40 (thiscall): SetTargetTrack::Update: the scanner watches its nearest, who's
// made the target
uint8_t __fastcall SetTargetTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    uint8_t* scanner = ped + 0x3BC;
    Scanner_Watch(scanner, Scanner_Nearest(scanner));
    uint8_t* targeting = At<uint8_t*>(ped, 0x1C8);
    Targeting_SetTarget(targeting, Scanner_Nearest(scanner));
    return 0;
}

// 0x626A80 (thiscall): SetStationaryTrack::Update: the ped's stationary flag from +0x18
uint8_t __fastcall SetStationaryTrack_Update(uint8_t* self, re::Edx, float) {
    OwnerPed(At<uint8_t*>(self, 0x1C))[0x4E4] = self[0x18];
    return 0;
}

// 0x626AA0 (thiscall): RangedWeaponFireTrack::Start: the first shot after +0x18 s (a
// random part of it with +0x28), +0x20 shots; stationary on a vehicle (the flag kept)
uint8_t __fastcall RangedWeaponFireTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x30) = owner;
    At<int32_t>(self, 0x34) = 0;
    self[0x38] = 0;
    self[0x39] = 0;
    self[0x3A] = 0;
    self[0x3B] = 0;
    uint8_t* ped = OwnerPed(owner);
    float part = self[0x28] != 0 ? RandomFraction() : 1.0f;
    self[0x38] = 0;
    At<int32_t>(self, 0x3C) = At<int32_t>(self, 0x20);
    At<uint32_t>(self, 0x34) = After(At<float>(self, 0x18), part);
    self[0x3B] = ped[0x4E4];
    if (At<uint8_t*>(ped, 0x1554)) {
        ped[0x4E4] = 1;
    }
    return 1;
}

// 0x626B60 (thiscall): RangedWeaponFireTrack::Update: not while comboing; at its time
// the fire button (+0x2C) is held (then +0x1C s, random with +0x29); shots left (+0x3C)
// are let go (3) when due or when the target's in the attack cone (then +0x24 s, random
// with +0x2A); done once past due with nothing to do
uint8_t __fastcall RangedWeaponFireTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x30));
    if (Ctl_IsPlaying(FightController(ped), nodeCombo, 1, 1)) {
        return 0;
    }
    uint8_t* target = PedTarget(ped);
    uint8_t cone = 0;
    if (target) {
        cone = Combat_InAttackCone(ped + 0x668, target);
    }
    uint32_t due = At<uint32_t>(self, 0x34);
    uint32_t now = timeInMs;
    if (now >= due && self[0x38] == 0) {
        ActionInputs_SetRequest(Inputs(ped), At<int32_t>(self, 0x2C), 2, 0.0f);
        float part = 1.0f;
        self[0x38] = 1;
        if (self[0x29] != 0) {
            part = RandomFloat(0.0f, part);
        }
        self[0x39] = 0;
        At<uint32_t>(self, 0x34) = After(At<float>(self, 0x1C), part);
        return 1;
    }
    int32_t shots = At<int32_t>(self, 0x3C);
    if ((shots > 0 && now > due && self[0x39] == 0) || (cone && shots > 0 && self[0x39] == 0)) {
        ActionInputs_SetRequest(Inputs(ped), At<int32_t>(self, 0x2C), 3, 0.0f);
        At<uint32_t>(ped, 0x550) = timeInMs;
        At<int32_t>(self, 0x3C) -= 1;
        self[0x39] = 1;
        if (At<int32_t>(self, 0x3C) > 0) {
            self[0x38] = 0;
            self[0x39] = 0;
        }
        float part = 1.0f;
        if (self[0x2A] != 0) {
            part = RandomFloat(0.0f, part);
        }
        At<uint32_t>(self, 0x34) = After(At<float>(self, 0x24), part);
        return 1;
    }
    return now <= due ? 1 : 0;
}

// 0x626D50 (thiscall): RangedWeaponFireTrack::End: the button let go, the stationary
// flag back
uint8_t __fastcall RangedWeaponFireTrack_End(uint8_t* self, re::Edx) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x30));
    ActionInputs_SetRequest(Inputs(ped), At<int32_t>(self, 0x2C), 3, 0.0f);
    ped[0x4E4] = self[0x3B];
    return 1;
}

// 0x626D80 (thiscall): PassBallTrack::Start: the pass button (8) held until +0x18 s
uint8_t __fastcall PassBallTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    ActionInputs_SetRequest(Inputs(OwnerPed(owner)), 8, 2, 0.0f);
    At<uint32_t>(self, 0x20) = After(At<float>(self, 0x18), 1.0f);
    return 1;
}

// 0x626DF0 (thiscall): AttackPropTrack::Start
uint8_t __fastcall AttackPropTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    At<int32_t>(OwnerPed(owner), 0x1340) = 0;
    return 1;
}

// 0x626E10 (thiscall): AttackVehicleTrack::Start
uint8_t __fastcall AttackVehicleTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    At<int32_t>(OwnerPed(owner), 0x1340) = 0;
    return 1;
}

// 0x626E30 (thiscall): AttackVehicleTrack::Update: for an attack-vehicle objective
// (0xB) on a vehicle: done past +0x18 / +0x1C s in the node and a random time between
// them (twice +0x18 when +0x1C is less); the vehicle targeted and remembered (+0x430),
// its attack spot (vf25) approached (flags 0x1C, speed 1, radius 0.3) inside 25 (or the
// gun's range + 5), shooting inside 0.5 (or the gun's range) of it; touching it or not
// getting there, the ped faces the spot
uint8_t __fastcall AttackVehicleTrack_Update(uint8_t* self, re::Edx, float) {
    float limit;
    if (At<float>(self, 0x1C) < At<float>(self, 0x18)) {
        limit = At<float>(self, 0x18) + At<float>(self, 0x18);
    } else {
        float most = At<float>(self, 0x1C);
        float least = At<float>(self, 0x18);
        float q = float(GameRand()) / 32767.0f;
        limit = q * (most - least) + least;
    }
    uint8_t* owner = At<uint8_t*>(self, 0x24);
    float t = At<float>(At<uint8_t*>(owner, 0), 8);
    if ((At<float>(self, 0x18) < t || At<float>(self, 0x1C) < t) && limit < t) {
        return 0;
    }
    uint8_t* ped = OwnerPed(owner);
    uint8_t* current = Queue_GetCurrent(ped + 0x5F0);
    if (!current || At<int32_t>(current, 8) != 0xB) {
        return 0;
    }
    uint8_t* vehicle = At<uint8_t*>(current, 0x10);
    if (!vehicle || (vehicle[0x108] & 7) != 2) {
        return 0;
    }
    V3 offset = {zeroVector[0], zeroVector[1], zeroVector[2]};
    Ped_SetTarget(ped, vehicle, 3, &offset.x);
    At<uint8_t*>(ped, 0x430) = vehicle;
    V3 spot;
    vc::Call<void*>(vehicle, 0x64, &spot, Pos(ped));
    float near = 0.5f;
    float far = 25.0f;
    if (Ped_HasWeapon(ped)) {
        uint8_t* weapon = At<uint8_t*>(ped, 0x1D0);
        if (!(Weapon_GetRangeFactor(weapon) == 0.0f)) {
            near = At<float>(weapon, 0x120);
            if (near > 25.0f) {
                far = near + 5.0f;
            }
        }
    }
    if (!Vehicle_HasCollidedWith(vehicle, ped)) {
        float dist = Distance(Pos(ped), &spot.x);
        if (far < dist) {
            return 0;
        }
        if (!(near < dist)) {
            LineOfFire_IsClear(ped + 0x668, &spot.x, vehicle, 1);
        }
        if (Steering_GoToEntity(ped + 0x654, vehicle, 0x1C, 1.0f, 0.3f)) {
            return 1;
        }
    }
    V3 d, dir;
    VectorSub(&d, &spot.x, Pos(ped));
    FlatDirection(&dir, &d, 1.0f);
    float* move = At<float*>(ped, 0x2E0);
    move[0] = dir.x;
    move[1] = dir.y;
    move[2] = dir.z;
    move = At<float*>(ped, 0x2E0);
    move[3] = dir.x;
    move[4] = dir.y;
    move[5] = dir.z;
    return 1;
}

// 0x6270C0 (thiscall): ProjectileCoverTrack's state step (+0x68, see 0x61BF30): 0 go to
// the cover (+0x48; there: hide), 1 hide (out of it: 0; peek when +0x60 or the timer's
// done with a clear line, else hide again), 2 go to the peek spot (+0x54; there: wait),
// 3 wait, 4 pop out (fire held with +0x79), 5 fire (then 0 unless +0x60 holds it)
void __fastcall ProjectileCoverTrack_UpdateStateImpl(uint8_t* self, re::Edx) {
    int32_t button = At<int32_t>(self, 0x18);
    self[0x78] = 0;
    uint8_t* ped = At<uint8_t*>(self, 0x24);
    switch (At<int32_t>(self, 0x68)) {
    case 0: {
        float dist = Distance2D(Pos(ped), &At<float>(self, 0x48));
        if (dist < 0.5f || dist == 0.5f) {
            At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x20)), 0x1340) = 0;
            ProjectileCoverTrack_SetState(self, 1);
            return;
        }
        At<int32_t>(At<uint8_t*>(self, 0x24), 0x1340) = dist < 1.5f ? 1 : 0;
        At<int32_t>(At<uint8_t*>(self, 0x24), 0x131C) = 0;
        self[0x78] = Steering_GoTo(At<uint8_t*>(self, 0x24) + 0x654, At<float>(self, 0x48), At<float>(self, 0x4C),
                                   At<float>(self, 0x50), 0x11C, 0.7f, 0.5f, nullptr, 0);
        if (ActionInputs_IsHeld(Inputs(At<uint8_t*>(self, 0x24)), 8)) {
            ActionInputs_SetRequest(Inputs(At<uint8_t*>(self, 0x24)), 8, 3, 0.0f);
        }
        return;
    }
    case 1:
        if (DistanceBeyond(Pos(ped), &At<float>(self, 0x48), 1.0f) > 0.0f) {
            ProjectileCoverTrack_SetState(self, 0);
            return;
        }
        if (At<uint8_t*>(self, 0x40)) {
            if (self[0x60] != 0) {
                ProjectileCoverTrack_SetState(self, 2);
                return;
            }
            if (At<uint8_t*>(self, 0x40) && Timer_IsDone(self + 0x6C)) {
                if (LineOfFire_IsClear(At<uint8_t*>(self, 0x24) + 0x668, &At<float>(self, 0x54),
                                       At<uint8_t*>(self, 0x40), 0)) {
                    ProjectileCoverTrack_SetState(self, 2);
                } else {
                    ProjectileCoverTrack_SetState(self, 1);
                }
                return;
            }
        }
        At<int32_t>(At<uint8_t*>(self, 0x24), 0x1340) = 1;
        At<int32_t>(At<uint8_t*>(self, 0x24), 0x131C) = At<int32_t>(self, 0x44) == 0 ? 1 : 0;
        return;
    case 2:
        if (DistanceBeyond(Pos(ped), &At<float>(self, 0x54), 0.5f) < 0.0f) {
            ProjectileCoverTrack_SetState(self, 3);
            return;
        }
        At<int32_t>(At<uint8_t*>(self, 0x24), 0x1340) = 1;
        At<int32_t>(At<uint8_t*>(self, 0x24), 0x131C) = 0;
        self[0x78] = Steering_GoTo(At<uint8_t*>(self, 0x24) + 0x654, At<float>(self, 0x54), At<float>(self, 0x58),
                                   At<float>(self, 0x5C), 0x11C, 1.0f, 0.25f, nullptr, 0);
        return;
    case 3:
        if (Timer_IsDone(self + 0x6C)) {
            ProjectileCoverTrack_SetState(self, 4);
            return;
        }
        At<int32_t>(At<uint8_t*>(self, 0x24), 0x1340) = 1;
        At<int32_t>(At<uint8_t*>(self, 0x24), 0x131C) = 0;
        return;
    case 4:
        if (Timer_IsDone(self + 0x6C)) {
            ProjectileCoverTrack_SetState(self, 5);
            return;
        }
        At<int32_t>(At<uint8_t*>(self, 0x24), 0x1340) = 1;
        At<int32_t>(At<uint8_t*>(self, 0x24), 0x131C) = 0;
        if (self[0x79] != 0) {
            ActionInputs_SetRequest(Inputs(At<uint8_t*>(self, 0x24)), button, 2, 0.0f);
        }
        return;
    case 5:
        if (!Timer_IsDone(self + 0x6C) && self[0x60] == 0) {
            At<int32_t>(At<uint8_t*>(self, 0x24), 0x1340) = 1;
            At<int32_t>(At<uint8_t*>(self, 0x24), 0x131C) = 0;
            return;
        }
        ProjectileCoverTrack_SetState(self, 0);
        return;
    default:
        return;
    }
}

// 0x627410 (thiscall): ProjectileCoverTrack's attack check: with a threat (+0x40) and
// the cover not holding (+0x44 of its timings) and no gun or the threat inside the
// cover's range (+0x20), the ped leaves cover (the pass let go) and, with an objective
// (+0x28) against a ped threat, gets an attack-ped objective first instead (1). (A
// ped without a usable gun gets 0x486CE0 on its slots first.)
uint8_t __fastcall ProjectileCoverTrack_CheckAttackImpl(uint8_t* self, re::Edx) {
    uint8_t* cover = At<uint8_t*>(self, 0x2C);
    float range = At<float>(cover, 0x20);
    uint8_t hold = cover[0x44];
    if (!At<uint8_t*>(self, 0x40)) {
        return 0;
    }
    if (!Combat_HasUsableWeapon(At<uint8_t*>(self, 0x24) + 0x668)) {
        Combat_486CE0(At<uint8_t*>(self, 0x24) + 0x668);
    }
    if (hold) {
        return 0;
    }
    if (Combat_HasUsableWeapon(At<uint8_t*>(self, 0x24) + 0x668)) {
        const float* threat = Pos(At<uint8_t*>(self, 0x40));
        float beyond = DistanceSquaredBeyond(Pos(At<uint8_t*>(self, 0x24)), threat, range);
        if (!(beyond < 0.0f || beyond != beyond)) {
            return 0;
        }
    }
    At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x20)), 0x1340) = 0;
    At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x20)), 0x131C) = 0;
    if (ActionInputs_IsHeld(Inputs(At<uint8_t*>(self, 0x24)), 8)) {
        ActionInputs_SetRequest(Inputs(At<uint8_t*>(self, 0x24)), 8, 3, 0.0f);
    }
    if (At<uint8_t*>(self, 0x28) && IsPedEntity(At<uint8_t*>(self, 0x40))) {
        void* mem = Malloc(0x1C);
        uint8_t* objective = nullptr;
        if (mem) {
            objective = AttackPedObjective_Construct(mem, At<uint8_t*>(self, 0x24), At<uint8_t*>(self, 0x40),
                                                     At<int32_t>(At<uint8_t*>(self, 0x28), 0x10));
        }
        Queue_RemoveFirst(At<uint8_t*>(self, 0x24) + 0x5F0);
        Queue_Prepend(At<uint8_t*>(self, 0x24) + 0x5F0, objective);
    }
    return 1;
}

// 0x627550 (thiscall): ManageTargetsTrack::Start: the scanner's ring cleared
uint8_t __fastcall ManageTargetsTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    At<int32_t>(self, 0x20) = 0;
    At<int32_t>(self, 0x24) = 0;
    Scanner_ClearRing(OwnerPed(owner) + 0x3BC);
    return 1;
}

// 0x627580 (thiscall): ManageTargetsTrack::Update: the scanner's pick (+0x20; the
// player, made the target, first with +0x18); the player: the social's replay every 8th
// frame and the hurt check. Ped types 0, 7, 8: a pick with punishment points that can be
// talked to gets +0x14A8; one without has the attack on it dropped (done); within the
// punishment threshold the ped's +0x1DB4 is cleared
uint8_t __fastcall ManageTargetsTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x1C));
    if (self[0x18] != 0) {
        uint8_t* player = playerPed;
        At<uint8_t*>(self, 0x20) = player;
        Ped_SetTargetEntity(ped, player);
    }
    uint8_t* pick = Scanner_48C030(ped + 0x3BC);
    At<uint8_t*>(self, 0x20) = pick;
    if (!pick) {
        return 0;
    }
    if (pick == playerPed) {
        if (Ped_EveryNthFrame(ped, 8)) {
            Social_48D7F0(ped + 0xB44);
        }
        Social_CheckHurt(ped + 0xB44);
    }
    int32_t type = At<int32_t>(ped, 0x1310);
    if (type == 0 || type == 8 || type == 7) {
        if (At<int32_t>(At<uint8_t*>(self, 0x20), 0x1D40) != 0 && Social_CanTalkTo(ped + 0xA64, At<uint8_t*>(self, 0x20))) {
            At<int32_t>(At<uint8_t*>(self, 0x20), 0x14A8) = 1;
        }
        pick = At<uint8_t*>(self, 0x20);
        int32_t points = At<int32_t>(pick, 0x1D40);
        if (points == 0) {
            Ped_DropAttackOn(ped, pick);
            return 0;
        }
        if (points <= At<int32_t>(punishment, 0x34)) {
            At<int32_t>(ped, 0x1DB4) = 0;
        }
    }
    return 1;
}

// 0x627660 (thiscall): SetHealthTrack::Update: the health set (+0x18), added (+0x19)
// or taken (+0x1A) by +0x1C
uint8_t __fastcall SetHealthTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x20));
    if (self[0x18] != 0) {
        Ped_SetHealth(ped, At<float>(self, 0x1C));
    } else if (self[0x19] != 0) {
        Ped_AddHealth(ped, At<float>(self, 0x1C));
    } else if (self[0x1A] != 0) {
        Ped_TakeHealth(ped, At<float>(self, 0x1C));
    }
    return 0;
}

// 0x62A800 (thiscall): an objective queue's flee-on-path objective (type 0xD) among its
// list (+8, +0x58 of them) or its default (+0x5C)
uint8_t* __fastcall Queue_FindFleeOnPath(uint8_t* queue, re::Edx) {
    uint32_t count = At<uint32_t>(queue, 0x58);
    for (uint32_t i = 0; i < count; i++) {
        uint8_t* objective = At<uint8_t*>(queue, 8 + i * 4);
        if (objective && At<int32_t>(objective, 8) == 0xD) {
            return objective;
        }
    }
    uint8_t* objective = At<uint8_t*>(queue, 0x5C);
    if (!objective || At<int32_t>(objective, 8) != 0xD) {
        return nullptr;
    }
    return objective;
}

// 0x62A840 (thiscall): the scanner's +0x190 entity reference set
void __fastcall Scanner_SetBallTargetImpl(uint8_t* scanner, re::Edx, uint8_t* entity) {
    re::Ptr32<uint8_t>* ref = &At<uint8_t*>(scanner, 0x190);
    if (entity == *ref) {
        return;
    }
    if (*ref) {
        Entity_CleanUpOldReference(*ref, ref);
    }
    *ref = entity;
    if (entity) {
        Entity_RegisterReference(entity, ref);
    }
}

// 0x62CB60 (thiscall): PassBallTrack::Update: once due, the pass let go and the
// scanner's ball target set from a pass objective (0x13, dropped when its time's up)
// or a receive one (0x14, dropped); done
uint8_t __fastcall PassBallTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x1C));
    if (timeInMs < At<uint32_t>(self, 0x20)) {
        return 1;
    }
    ActionInputs_SetRequest(Inputs(ped), 8, 3, 0.0f);
    uint8_t* queue = ped + 0x5F0;
    uint8_t* current = Queue_GetCurrent(queue);
    if (current) {
        uint8_t* target = nullptr;
        if (At<int32_t>(current, 8) == 0x13) {
            target = At<uint8_t*>(current, 0x10);
            if (Objective_BallTimeUp(current)) {
                Queue_RemoveFirst(queue);
            }
        }
        if (At<int32_t>(current, 8) == 0x14) {   // (read again after a removal, as the game does)
            target = At<uint8_t*>(current, 0x10);
            Queue_RemoveFirst(queue);
        }
        Scanner_SetBallTarget(ped + 0x3BC, target);
    }
    return 0;
}

// 0x62CBF0 (thiscall): AttackPropTrack::Update: for a break-prop objective (8) on an
// entity: targeted at the objective's offset, remembered (+0x438); approached at the
// aim point (with a target and an offset) or its attack spot (vf25), speed 0.5 (or the
// gun's range), radius 0.45, following it
uint8_t __fastcall AttackPropTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x1C));
    uint8_t* current = Queue_GetCurrent(ped + 0x5F0);
    if (!current || At<int32_t>(current, 8) != 8) {
        return 0;
    }
    uint8_t* entity = Objective_GetEntity(current);
    if (!entity) {
        return 0;
    }
    const float* offset = &At<float>(current, 0x10);
    Ped_SetTarget(ped, entity, 3, offset);
    NamedRef_Set(ped + 0x438, entity, 0);
    V3 to;
    if (Ped_GetTarget(ped) && VectorNotEqual(offset, zeroVector)) {
        Ped_SetAimPoint(ped, offset);
        V3 aim;
        to = *Ped_GetAimPoint(ped, &aim);
    } else {
        vc::Call<void*>(entity, 0x64, &to, Pos(ped));
    }
    float speed = 0.5f;
    if (Ped_HasWeapon(ped)) {
        uint8_t* weapon = At<uint8_t*>(ped, 0x1D0);
        if (!(Weapon_GetRangeFactor(weapon) == 0.0f)) {
            speed = At<float>(weapon, 0x120);
        }
    }
    Steering_GoTo(ped + 0x654, to.x, to.y, to.z, 0x14, speed, 0.45f, entity, 0);
    return 1;
}

// the threat reference (+0x64) let go (its cover count +0x9A4 down)
void ReleaseThreat(uint8_t* self) {
    re::Ptr32<uint8_t>* ref = &At<uint8_t*>(self, 0x64);
    if (*ref) {
        (*ref)[0x9A4] = uint8_t((*ref)[0x9A4] - 1);
        if (*ref) {
            Entity_CleanUpOldReference(*ref, ref);
            *ref = nullptr;
        }
    }
}

// 0x62CD40 (thiscall): ProjectileCoverTrack::End: moving flags off, the eye override
// dropped, the threat let go
uint8_t __fastcall ProjectileCoverTrack_End(uint8_t* self, re::Edx) {
    At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x20)), 0x1340) = 0;
    At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x20)), 0x131C) = 0;
    Sight_ClearEye(OwnerPed(At<uint8_t*>(self, 0x20)) + 0xA64);
    ReleaseThreat(self);
    return 1;
}

// 0x62CDA0 (thiscall): ProjectileCoverTrack's refresh: the ped (+0x24) and its threat
// (+0x40, a ped threat counted in its +0x9A4 while referenced at +0x64); the cover:
// with +0x1C a still one where the ped stands (its own static cover objective), else
// the cover objective (0x10)'s route (its first and second points; one point: no side)
// or a cover point; the eye 1.5 above the peek spot, +0x60 (pop out) for a cover
// without timings or a threat within its range, +0x79 with a ranged gun; the ped's move
// target the peek spot
uint8_t __fastcall ProjectileCoverTrack_RefreshImpl(uint8_t* self, re::Edx) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x20));
    At<uint8_t*>(self, 0x24) = ped;
    if (!ped) {
        return 0;
    }
    uint8_t* threat = Ped_ChooseTargetIfAsked(ped);
    re::Ptr32<uint8_t>* ref = &At<uint8_t*>(self, 0x64);
    At<uint8_t*>(self, 0x40) = threat;
    if (threat != *ref) {
        ReleaseThreat(self);
        threat = At<uint8_t*>(self, 0x40);
        if (threat && IsPedEntity(threat)) {
            threat[0x9A4] = uint8_t(threat[0x9A4] + 1);
            EntityRef_Set(ref, threat);
        }
    }
    const float* peek = nullptr;
    if (self[0x1C] != 0) {
        if (!(coverGuard & 1)) {
            coverGuard |= 1;
            CoverObjective_Construct(staticCover, nullptr, nullptr, 0, 0);
            AtExit(staticCoverAtExit);
        }
        uint8_t* p = At<uint8_t*>(self, 0x24);
        At<uint8_t*>(self, 0x28) = staticCover;
        At<uint8_t*>(self, 0x2C) = staticCover + 0x20;
        At<int32_t>(self, 0x44) = 0;
        const float* at = Pos(p);
        At<float>(self, 0x48) = at[0];
        At<float>(self, 0x4C) = at[1];
        At<float>(self, 0x50) = at[2];
        peek = Pos(p);
    } else {
        uint8_t* current = Queue_GetCurrent(At<uint8_t*>(self, 0x24) + 0x5F0);
        if (current && At<int32_t>(current, 8) != 0x10) {
            current = nullptr;
        }
        At<uint8_t*>(self, 0x28) = current;
        At<uint8_t*>(self, 0x2C) = reinterpret_cast<uint8_t*>(uintptr_t(current) + 0x20);
        if (!current) {
            return 0;
        }
        if (!Objective_GetEntity(current)) {
            if (!ProjectileCoverTrack_ChooseCover(self)) {
                return 0;
            }
        } else {
            uint8_t* route = Objective_GetEntity(At<uint8_t*>(self, 0x28));
            float* first = route ? Route_First(route) : nullptr;
            if (!first) {
                Queue_RemoveFirst(At<uint8_t*>(self, 0x24) + 0x5F0);
                return 0;
            }
            At<float>(self, 0x48) = first[0];
            At<float>(self, 0x4C) = first[1];
            At<float>(self, 0x50) = first[2];
            float* next = Route_Next(route);
            if (!next) {
                At<int32_t>(self, 0x44) = 0;
                At<float>(self, 0x54) = At<float>(self, 0x48);
                At<float>(self, 0x58) = At<float>(self, 0x4C);
                At<float>(self, 0x5C) = At<float>(self, 0x50);
            } else {
                At<int32_t>(self, 0x44) = 1;
                peek = next;
            }
        }
    }
    if (peek) {
        At<float>(self, 0x54) = peek[0];
        At<float>(self, 0x58) = peek[1];
        At<float>(self, 0x5C) = peek[2];
    }
    V3 up = {0.0f, 0.0f, 1.5f};
    V3 eye;
    uint8_t* sight = At<uint8_t*>(self, 0x24) + 0xA64;
    Sight_SetEye(sight, &VectorAdd(&eye, &At<float>(self, 0x54), &up.x)->x);
    uint8_t* timings = At<uint8_t*>(self, 0x2C);
    self[0x60] = 0;
    if (At<float>(timings, 0x24) <= 0.0f) {
        if (At<float>(timings, 0x28) <= 0.0f) {
            self[0x60] = 1;
        }
    }
    if (At<uint8_t*>(self, 0x40)) {
        if (DistanceSquaredBeyond(&At<float>(self, 0x48), Pos(At<uint8_t*>(self, 0x40)), At<float>(timings, 0x20)) <
            0.0f) {
            self[0x60] = 1;
        }
    }
    if (Ped_HasWeapon(At<uint8_t*>(self, 0x24))) {
        self[0x79] = !(Weapon_GetRangeFactor(At<uint8_t*>(At<uint8_t*>(self, 0x24), 0x1D0)) == 0.0f) ? 1 : 0;
    }
    Ped_SetMoveTarget(At<uint8_t*>(self, 0x24), &At<float>(self, 0x54));
    return 1;
}

// 0x62E3F0 (thiscall): ProjectileCoverTrack::Start: the threat reference dropped, a
// refresh, then state 0
uint8_t __fastcall ProjectileCoverTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    At<int32_t>(self, 0x30) = 0;
    re::Ptr32<uint8_t>* ref = &At<uint8_t*>(self, 0x64);
    if (*ref) {
        Entity_CleanUpOldReference(*ref, ref);
        *ref = nullptr;
    }
    if (!ProjectileCoverTrack_Refresh(self)) {
        return 0;
    }
    At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x20)), 0x1340) = 0;
    At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x20)), 0x131C) = 0;
    ProjectileCoverTrack_SetState(self, 0);
    return 1;
}

// 0x62E460 (thiscall): ProjectileCoverTrack::Update: refreshed, not attacking instead;
// facing the threat; the state step
uint8_t __fastcall ProjectileCoverTrack_Update(uint8_t* self, re::Edx, float) {
    if (!ProjectileCoverTrack_Refresh(self)) {
        return 0;
    }
    if (ProjectileCoverTrack_CheckAttack(self)) {
        return 0;
    }
    uint8_t* threat = At<uint8_t*>(self, 0x40);
    if (threat) {
        const float* from = Pos(At<uint8_t*>(self, 0x24));
        V3 d, dir;
        VectorSub(&d, Pos(threat), from);
        FlatDirection(&dir, &d, 1.0f);
        float* move = At<float*>(At<uint8_t*>(self, 0x24), 0x2E0);
        move[3] = dir.x;
        move[4] = dir.y;
        move[5] = dir.z;
        move = At<float*>(At<uint8_t*>(self, 0x24), 0x2E0);
        move[0] = dir.x;
        move[1] = dir.y;
        move[2] = dir.z;
    }
    ProjectileCoverTrack_UpdateState(self);
    return 1;
}

// 0x8F6290 (exit): the static cover objective's destructor
void __cdecl StaticCover_Exit() {
    ActionObjective_Destruct(staticCover);
}

} // namespace

#define AICOMBATTRK_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AICOMBATTRK_REVERSED("PursuitLocoTrack::Start", 0x00625D40, &PursuitLocoTrack_Start);
AICOMBATTRK_REVERSED("CombatTracks::StaticCoverExit", 0x008F6290, &StaticCover_Exit);
AICOMBATTRK_REVERSED("PursuitLocoTrack::Update", 0x00625DA0, &PursuitLocoTrack_Update);
AICOMBATTRK_REVERSED("PursuitLocoTrack::End", 0x00625E40, &PursuitLocoTrack_End);
AICOMBATTRK_REVERSED("AttackTrack::Update", 0x00625E60, &AttackTrack_Update);
AICOMBATTRK_REVERSED("ActionRequestTrack::Update", 0x00626230, &ActionRequestTrack_Update);
AICOMBATTRK_REVERSED("ActionRequestTrack::End", 0x006262C0, &ActionRequestTrack_End);
AICOMBATTRK_REVERSED("ActionRegisterButtonHistoryTrack::Update", 0x006262F0, &ActionRegisterButtonHistoryTrack_Update);
AICOMBATTRK_REVERSED("DialogActionNodeRequestTrack::Play", 0x00626350, &DialogActionNodeRequestTrack_PlayImpl);
AICOMBATTRK_REVERSED("ActionNodeRequestTrack::Update", 0x00626400, &ActionNodeRequestTrack_Update);
AICOMBATTRK_REVERSED("DialogActionNodeRequestTrack::Update", 0x00626410, &DialogActionNodeRequestTrack_Update);
AICOMBATTRK_REVERSED("BlockTrack::Update", 0x00626490, &BlockTrack_Update);
AICOMBATTRK_REVERSED("BlockTrack::End", 0x00626590, &BlockTrack_End);
AICOMBATTRK_REVERSED("EvadeTrack::Update", 0x006265B0, &EvadeTrack_Update);
AICOMBATTRK_REVERSED("ReleaseFightTokenTrack::Update", 0x006266E0, &ReleaseFightTokenTrack_Update);
AICOMBATTRK_REVERSED("EnableFightTokensTrack::Update", 0x00626720, &EnableFightTokensTrack_Update);
AICOMBATTRK_REVERSED("DisableFightTokensTrack::Update", 0x00626740, &DisableFightTokensTrack_Update);
AICOMBATTRK_REVERSED("AttackObjectiveTrack::Update", 0x006267A0, &AttackObjectiveTrack_Update);
AICOMBATTRK_REVERSED("AttackPropObjectiveTrack::Update", 0x00626850, &AttackPropObjectiveTrack_Update);
AICOMBATTRK_REVERSED("AttackVehicleObjectiveTrack::Update", 0x006269E0, &AttackVehicleObjectiveTrack_Update);
AICOMBATTRK_REVERSED("SetTargetTrack::Update", 0x00626A40, &SetTargetTrack_Update);
AICOMBATTRK_REVERSED("SetStationaryTrack::Update", 0x00626A80, &SetStationaryTrack_Update);
AICOMBATTRK_REVERSED("RangedWeaponFireTrack::Start", 0x00626AA0, &RangedWeaponFireTrack_Start);
AICOMBATTRK_REVERSED("RangedWeaponFireTrack::Update", 0x00626B60, &RangedWeaponFireTrack_Update);
AICOMBATTRK_REVERSED("RangedWeaponFireTrack::End", 0x00626D50, &RangedWeaponFireTrack_End);
AICOMBATTRK_REVERSED("PassBallTrack::Start", 0x00626D80, &PassBallTrack_Start);
AICOMBATTRK_REVERSED("AttackPropTrack::Start", 0x00626DF0, &AttackPropTrack_Start);
AICOMBATTRK_REVERSED("AttackVehicleTrack::Start", 0x00626E10, &AttackVehicleTrack_Start);
AICOMBATTRK_REVERSED("AttackVehicleTrack::Update", 0x00626E30, &AttackVehicleTrack_Update);
AICOMBATTRK_REVERSED("ProjectileCoverTrack::UpdateState", 0x006270C0, &ProjectileCoverTrack_UpdateStateImpl);
AICOMBATTRK_REVERSED("ProjectileCoverTrack::CheckAttack", 0x00627410, &ProjectileCoverTrack_CheckAttackImpl);
AICOMBATTRK_REVERSED("ManageTargetsTrack::Start", 0x00627550, &ManageTargetsTrack_Start);
AICOMBATTRK_REVERSED("ManageTargetsTrack::Update", 0x00627580, &ManageTargetsTrack_Update);
AICOMBATTRK_REVERSED("SetHealthTrack::Update", 0x00627660, &SetHealthTrack_Update);
AICOMBATTRK_REVERSED("ObjectiveQueue::FindFleeOnPath", 0x0062A800, &Queue_FindFleeOnPath);
AICOMBATTRK_REVERSED("PedScanner::SetBallTarget", 0x0062A840, &Scanner_SetBallTargetImpl);
AICOMBATTRK_REVERSED("PassBallTrack::Update", 0x0062CB60, &PassBallTrack_Update);
AICOMBATTRK_REVERSED("AttackPropTrack::Update", 0x0062CBF0, &AttackPropTrack_Update);
AICOMBATTRK_REVERSED("ProjectileCoverTrack::End", 0x0062CD40, &ProjectileCoverTrack_End);
AICOMBATTRK_REVERSED("ProjectileCoverTrack::Refresh", 0x0062CDA0, &ProjectileCoverTrack_RefreshImpl);
AICOMBATTRK_REVERSED("ProjectileCoverTrack::Start", 0x0062E3F0, &ProjectileCoverTrack_Start);
AICOMBATTRK_REVERSED("ProjectileCoverTrack::Update", 0x0062E460, &ProjectileCoverTrack_Update);
