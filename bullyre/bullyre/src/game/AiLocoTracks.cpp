// The action tree's AI locomotion tracks: chase, guard, keep distance, tether, wait for
// target, circle around the target, the bike dash, jump and rat locomotion.
// 0x6276B0..0x62E59F.
// The ped: +0x2E0 its move state (+0 the direction, +0xC the facing, +0x1C the speed),
// +0x2F0 its ally chain, +0x358 its tether (+0x35C the tethered-to (0x4995B0 its
// position, 0x499600 it when a ped), +0x360 set, +0x364 its speed scale, +0x378 its
// offset, +0x3B4 follow the target, +0x3B8 the speed), +0x654 its steering, +0x668
// its combat slots, +0xB44 its social, +0x1340 facing on its own, +0x1E08 the entity it
// guards.

#include <cmath>
#include <cstdint>

#include "game/ActionTracksCommon.h"
#include "game/TrackMath.h"

#include "game/Crt.h"
#include "reversed.h"
using namespace actc;

namespace {

GAME_VAR(float, combatStepRate, 0x00B0C0B0);
GAME_VAR(float, chaseRadius, 0x00B0C260);
GAME_VAR(float, chaseMostSpeed, 0x00B0C264);
GAME_VAR(float, chaseLeastSpeed, 0x00B0C268);
GAME_VAR(float, chaseSpeedPerUnit, 0x00B0C26C);
GAME_VAR(uint8_t*, gameModeObject, 0x020C8704);   // (+4 on)

GAME_FN(uint8_t, __thiscall, Steering_GoTo, 0x004A5000, uint8_t* steering, float x, float y, float z, uint32_t flags,
        float speed, float radius, uint8_t* entity, int flag);
GAME_FN(uint8_t, __thiscall, Steering_GoToEntity, 0x004A5360, uint8_t* steering, uint8_t* entity, uint32_t flags,
        float speed, float radius);
GAME_FN(V3*, __thiscall, Steering_ToPoint, 0x004A48A0, uint8_t* steering, V3* out, const float* point);
GAME_FN(void, __thiscall, Steering_Stop, 0x004A4710, uint8_t* steering);
GAME_FN(uint8_t, __thiscall, Tether_IsEntityAllowed, 0x00499880, uint8_t* tether, uint8_t* entity, float margin);
GAME_FN(uint8_t, __thiscall, Tether_Clamp, 0x00499970, uint8_t* tether, V3* pos, float margin);
GAME_FN(uint8_t, __thiscall, Tether_IsInside, 0x004998F0, uint8_t* tether, float margin);
GAME_FN(V3*, __thiscall, TetherTarget_GetPosition, 0x004995B0, uint8_t* target, V3* out);
GAME_FN(uint8_t*, __thiscall, TetherTarget_GetPed, 0x00499600, uint8_t* target);
GAME_FN(uint8_t*, __thiscall, Ally_GetTop, 0x00485560, uint8_t* ally);
GAME_FN(void, __thiscall, Ally_FollowSpot, 0x004855E0, uint8_t* ally, V3* pos);
GAME_FN(uint8_t, __thiscall, Combat_JoinCurrentTarget, 0x00487D40, uint8_t* combat);
GAME_FN(uint8_t, __thiscall, Combat_IsOutOfReach, 0x00487AE0, uint8_t* combat, uint8_t* target, float a, float b);
GAME_FN(uint8_t, __thiscall, Social_48FB40, 0x0048FB40, uint8_t* social);
GAME_FN(float, __thiscall, Vector_Magnitude2D, 0x004246B0, const float* v);
GAME_FN(float, __cdecl, Distance, 0x0041A950, const float* a, const float* b);
GAME_FN(float, __cdecl, Distance2D, 0x00470110, const float* a, const float* b);
GAME_FN(float, __cdecl, DistanceBeyond, 0x0041A4D0, const float* a, const float* b, float range);
GAME_FN(float, __cdecl, Clamp, 0x0041A470, float value, float low, float high);
GAME_FN(V3*, __cdecl, VectorAdd, 0x004129A0, V3* out, const float* a, const float* b);
GAME_FN(V3*, __cdecl, FlatDirection, 0x0041A520, V3* out, const V3* v, float length);
GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_VAR(float, combatMaxAngle, 0x00B0C0A0);
GAME_VAR(float, randomAngleRange, 0x00B0C0AC);   // (the rat's turns, the wait spots' angles)
GAME_VAR(uint8_t, ratAvoidFlag, 0x01B65728);
inline float& waitSpotAngleRange = randomAngleRange;
inline float& ratTurnRange = randomAngleRange;
inline int __cdecl GameRand() { return crt::Rand(); }
GAME_FN(float, __cdecl, RandomFloat, 0x0041A5B0, float low, float high);
GAME_FN(uint8_t, __thiscall, Ped_IsMovingFast, 0x00473DD0, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Ped_IsSwimming, 0x00476D70, uint8_t* ped);
GAME_FN(void, __thiscall, Ped_SetHeading, 0x00475730, uint8_t* ped, float heading);
GAME_FN(uint8_t, __thiscall, Ped_HasWeapon, 0x00474BF0, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Ped_EveryNthFrame, 0x00465A60, uint8_t* ped, uint32_t n);
GAME_FN(void, __thiscall, Ped_SetMoveTarget, 0x00474A00, uint8_t* ped, const float* pos);
GAME_FN(uint8_t, __thiscall, Tether_IsPosAllowed, 0x00499840, uint8_t* tether, const float* pos, float range);
GAME_FN(uint8_t, __thiscall, LineOfFire_IsClear, 0x0048A700, uint8_t* combat, const float* from, uint8_t* target,
        int flags);
GAME_FN(uint8_t, __thiscall, Combat_HasRangedWeapon, 0x00486C30, uint8_t* combat);
GAME_FN(uint8_t*, __thiscall, Scanner_48C030, 0x0048C030, uint8_t* scanner);
GAME_FN(float, __cdecl, HeadingBetween, 0x00433350, const float* from, const float* to);
GAME_FN(float, __cdecl, HeadingFromVector, 0x00433310, const V3* v);
GAME_FN(float, __cdecl, LimitAngle, 0x00432EE0, float angle);
GAME_FN(V3*, __cdecl, Vector_FromAngle, 0x00432DF0, V3* out, float angle);
GAME_FN(V3*, __cdecl, VectorScale, 0x00412350, V3* out, const V3* v, float s);
GAME_FN(void, __thiscall, Vector_AddTo, 0x00415200, V3* v, const V3* add);
GAME_FN(float, __cdecl, DistanceSquaredBeyond, 0x00487E50, const float* a, const float* b, float range);
GAME_FN(float, __cdecl, DistToLine2D, 0x00569330, const float* a, const float* b, const float* point);
GAME_FN(float, __cdecl, FindGroundZ2D, 0x004634A0, float x, float y, int flag);
GAME_FN(float*, __cdecl, FindPlayerCoors, 0x00446900, float* out);
GAME_FN(uint8_t, __cdecl, GetIsLineOfSightClear, 0x004617D0, const float* a, const float* b, int flags);
GAME_FN(uint8_t, __cdecl, World_TestLine, 0x00461EA0, const float* from, const float* to, ColPoint* colPoint,
        re::Ptr32<uint8_t>* hit, int flags, int a, int b);
GAME_FN(void, __thiscall, ColPoint_Construct, 0x00415FA0, ColPoint* colPoint);
GAME_FN(void, __thiscall, ColPoint_Destruct, 0x00415500, ColPoint* colPoint);
GAME_FN(uint8_t, __cdecl, Walk_IsOnMesh, 0x005EB5E0, const float* pos, uint8_t* inObstacle);
GAME_FN(uint8_t, __cdecl, JF_IsReachable, 0x00489100, const float* a, const float* b, const float* c);
GAME_FN(uint8_t, __cdecl, JF_PedOnLine, 0x004891D0, uint8_t* ped, const float* a, const float* b);
GAME_FN(void, __thiscall, Objectives_Clear, 0x00470350, uint8_t* queue);
GAME_FN(uint8_t*, __thiscall, Queue_GetCurrent, 0x00470340, uint8_t* queue);
GAME_FN(void, __thiscall, Queue_RemoveFirst, 0x004702C0, uint8_t* queue);
GAME_FN(void, __thiscall, ActionInputs_SetRequest, 0x005F3A90, uint8_t* inputs, int button, int kind, float time);
GAME_FN(void, __thiscall, Timer_Reset, 0x0045BD10, uint8_t* timer);
GAME_FN(void, __thiscall, Countdown_Start, 0x0045BD30, uint8_t* timer, uint32_t ms);
GAME_FN(void, __thiscall, Timer_Set, 0x0045BD70, uint8_t* timer, float seconds);
GAME_FN(uint8_t, __thiscall, Timer_IsDone, 0x0045BDD0, uint8_t* timer);
// the tracks' own, by address (so their modes apply)
GAME_FN(uint8_t, __thiscall, WaitForTargetLocoTrack_CanWaitAt, 0x006288C0, uint8_t* self, const float* pos,
        int checkLine);
GAME_FN(uint8_t, __thiscall, RatLocoTrack_Refresh, 0x00629410, uint8_t* self);
GAME_FN(uint8_t, __thiscall, RatLocoTrack_CanGoTo, 0x00629470, uint8_t* self, const float* unused, const float* pos);
GAME_FN(void, __thiscall, RatLocoTrack_Step, 0x0062D880, uint8_t* self);
GAME_FN(void, __thiscall, RatLocoTrack_Wobble, 0x0061C370, uint8_t* self);
GAME_FN(uint8_t, __thiscall, RatLocoTrack_FindFood, 0x0061C470, uint8_t* self);

float* Move(uint8_t* ped) {
    return At<float*>(ped, 0x2E0);
}
uint8_t* Steering(uint8_t* ped) {
    return ped + 0x654;
}
// the flat direction from the ped towards a point, through its steering
V3 FacingTo(uint8_t* ped, const float* point) {
    V3 to, dir;
    V3* t = Steering_ToPoint(Steering(ped), &to, point);
    return *FlatDirection(&dir, t, 1.0f);
}
void SetFacing(float* move, const V3& v) {
    move[3] = v.x;
    move[4] = v.y;
    move[5] = v.z;
}
void SetDirection(float* move, const V3& v) {
    move[0] = v.x;
    move[1] = v.y;
    move[2] = v.z;
}

// the locomotion tracks' Start: the owner kept, two flags cleared, facing freed
template <int OwnerAt, int FlagsAt>
uint8_t __fastcall LocoTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, OwnerAt) = owner;
    self[FlagsAt] = 0;
    self[FlagsAt + 1] = 0;
    At<int32_t>(OwnerPed(owner), 0x1340) = 0;
    return 1;
}
// and their End: facing freed
template <int OwnerAt>
uint8_t __fastcall LocoTrack_End(uint8_t* self, re::Edx) {
    At<int32_t>(OwnerPed(At<uint8_t*>(self, OwnerAt)), 0x1340) = 0;
    return 1;
}

// 0x6276D0 (thiscall; 0x62781E is a piece of it): ChaseLocoTrack::Update: after its
// target beyond the range (+0x1C, +0x18 once close (+0x28)): the target itself when the
// tether allows it (speed by the distance, 1.5..3, radius 0.8), else the tether's
// nearest point (speed by the distance there, radius 2, 1 once going (+0x29)); facing
// the target when not going or not steering its own way (+0x20 sets that from going)
uint8_t __fastcall ChaseLocoTrack_Update(uint8_t* self, re::Edx, float) {
    float range = At<float>(self, 0x18);
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x24));
    if (self[0x28] == 0) {
        range = At<float>(self, 0x1C);
    }
    uint8_t* target = Ped_GetTarget(ped);
    if (!target) {
        return 0;
    }
    const float* tp = Pos(target);
    V3 at = {tp[0], tp[1], tp[2]};
    const float* pp = Pos(ped);
    V3 from = {pp[0], pp[1], pp[2]};
    V3 d;
    VectorSub(&d, &at.x, &from.x);
    d.z = 0.0f;
    Vector_Normalise(&d);   // (not used)
    float dist = Distance(&from.x, &at.x);
    uint8_t going = 0;
    if (!(range < dist)) {
        self[0x28] = 1;
    } else {
        self[0x28] = 0;
        uint8_t* tether = ped + 0x358;
        if (Tether_IsEntityAllowed(tether, target, 0.0f)) {
            float speed = chaseLeastSpeed + (dist - range) * chaseSpeedPerUnit;
            if (speed < chaseLeastSpeed) {
                speed = chaseLeastSpeed;
            } else if (chaseMostSpeed < speed) {
                speed = chaseMostSpeed;
            }
            going = Steering_GoToEntity(Steering(ped), target, 0x1C, speed, chaseRadius);
        } else {
            const float* tq = Pos(target);
            V3 spot = {tq[0], tq[1], tq[2]};
            if (Tether_Clamp(tether, &spot, -0.3f)) {
                float radius = 2.0f;
                if (self[0x29] != 0) {
                    radius = 1.0f;
                }
                float left = (Distance(Pos(ped), &spot.x) - radius) * 0.5f;
                float speed = Clamp(left, 0.0f, 1.0f);
                going = Steering_GoTo(Steering(ped), spot.x, spot.y, spot.z, 0x1C, speed, radius, nullptr, 0);
            }
        }
    }
    if (self[0x20] != 0) {
        At<int32_t>(ped, 0x1340) = going == 0 ? 1 : 0;
    }
    self[0x29] = going;
    if (At<int32_t>(ped, 0x1340) != 0 || !going) {
        V3 v;
        V3* r = VectorSub(&v, Pos(target), Pos(ped));
        V3 face = {r->x, r->y, 0.0f};
        Vector_Normalise(&face);
        SetFacing(Move(ped), face);
        if (self[0x29] == 0) {
            SetDirection(Move(ped), face);
        }
    }
    return 1;
}

// 0x627A20 (thiscall): GuardLocoTrack::Update: the ped keeps near the entity it guards
// (+0x1E08): with a target (joining its slots) between the guarded and the target (at
// most the target's distance less +0x20 out); pulled onto the circle of +0x18 (inside
// it) or of +0x1C (beyond it); speed half the distance (0.2..1) * +0x2C, radius +0x28
// (1.5 of it once stopped), flags 0x11C; arrived (within +0x24, twice once there): it
// faces the target
uint8_t __fastcall GuardLocoTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x30));
    uint8_t* guarded = At<uint8_t*>(ped, 0x1E08);
    if (!guarded) {
        return 0;
    }
    const float* gp = Pos(guarded);
    V3 centre = {gp[0], gp[1], gp[2]};
    const float* pp = Pos(ped);
    V3 spot = {pp[0], pp[1], pp[2]};
    uint8_t* target = Ped_GetTarget(ped);
    float inner = At<float>(self, 0x18);
    float outer = At<float>(self, 0x1C);
    float offset = At<float>(self, 0x20);
    float arrive = At<float>(self, 0x24);
    float radius = At<float>(self, 0x28);
    float speedScale = At<float>(self, 0x2C);
    if (self[0x34] == 0) {
        radius = radius * 1.5f;
    }
    if (self[0x35] != 0) {
        arrive = arrive + arrive;
    }
    At<int32_t>(ped, 0x1340) = 0;
    if (target) {
        Combat_JoinCurrentTarget(ped + 0x668);
        const float* tp = Pos(target);
        spot = {tp[0], tp[1], tp[2]};
        float reach = Distance2D(&centre.x, &spot.x) - offset;
        if (outer > reach) {
            outer = reach;
        }
    }
    spot.z = centre.z;
    if (DistanceBeyond(&centre.x, &spot.x, inner) < 0.0f) {
        V3 d, dir, sum;
        VectorSub(&d, &spot.x, &centre.x);
        FlatDirection(&dir, &d, inner);
        spot = *VectorAdd(&sum, &centre.x, &dir.x);
    } else if (DistanceBeyond(&centre.x, &spot.x, outer) > 0.0f) {
        V3 d, dir, sum;
        VectorSub(&d, &spot.x, &centre.x);
        FlatDirection(&dir, &d, outer);
        spot = *VectorAdd(&sum, &centre.x, &dir.x);
    }
    float speed = Distance2D(Pos(ped), &spot.x) * 0.5f;
    if (speed < 0.2f) {
        speed = 0.2f;
    } else if (1.0f < speed) {
        speed = 1.0f;
    }
    float scaled = speed * speedScale;
    uint8_t going = Steering_GoTo(Steering(ped), spot.x, spot.y, spot.z, 0x11C, scaled, radius, nullptr, 0);
    self[0x34] = going;
    if (going && !(DistanceBeyond(Pos(ped), &spot.x, arrive) < 0.0f)) {
        self[0x35] = 0;
        return 1;
    }
    self[0x35] = 1;
    At<int32_t>(ped, 0x1340) = 1;
    if (target) {
        float* move = Move(ped);
        SetFacing(move, FacingTo(ped, Pos(target)));
    }
    return 1;
}

// 0x627DC0 (thiscall): KeepDistanceLocoTrack::Update: the ped keeps (5 + 0.8) / 2 away
// from its target (2.6's when it's talking to the player), on its side (flags 0x18
// beyond it, else 0x10; speed 1); when it stops it faces the target, standing
uint8_t __fastcall KeepDistanceLocoTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x20));
    uint8_t* target = Ped_GetTarget(ped);
    if (!target) {
        return 0;
    }
    float keep = 5.0f;
    if (At<uint8_t*>(ped, 0xBB4) == playerPed && Social_48FB40(ped + 0xB44)) {
        keep = 2.6f;
    }
    float half = (keep + 0.8f) * 0.5f;
    uint32_t flags = 0x10;
    V3 d;
    V3* r = VectorSub(&d, Pos(ped), Pos(target));
    V3 dir = *r;
    Vector_Normalise(&dir);
    V3 offset = {dir.x * half, dir.y * half, half * dir.z};
    V3 spot;
    VectorAdd(&spot, Pos(target), &offset.x);
    float beyond = DistanceBeyond(Pos(ped), &spot.x, half);
    if (0.0f < beyond) {
        flags = 0x18;
    }
    uint8_t going = Steering_GoTo(Steering(ped), spot.x, spot.y, spot.z, uint16_t(flags), 1.0f, half, nullptr, 0);
    self[0x24] = going;
    At<int32_t>(ped, 0x1340) = (beyond <= 0.0f && going) ? 1 : 0;
    if (self[0x24] == 0) {
        float* move = Move(ped);
        SetFacing(move, FacingTo(ped, Pos(target)));
        move = Move(ped);
        move[0] = move[3];
        move[1] = move[4];
        move[2] = move[5];
        Move(ped)[7] = 0.0f;
        return 1;
    }
    if (At<int32_t>(ped, 0x1340) != 0) {
        float* move = Move(ped);
        SetFacing(move, FacingTo(ped, Pos(target)));
    }
    return 1;
}

// 0x628070 (thiscall): TetherLocoTrack::Start: the tether's offset (+0x1C, +0x20) and,
// from +0x24 up, its speed scale (the old one kept at +0xCC); the tether's position kept
uint8_t __fastcall TetherLocoTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x3C) = owner;
    self[0x40] = 0;
    uint8_t* ped = OwnerPed(owner);
    At<int32_t>(ped, 0x1340) = 0;
    if (At<int32_t>(ped, 0x360) != 0) {
        V3 pos = {At<float>(self, 0x1C), At<float>(self, 0x20), 0.0f};
        At<float>(ped, 0x378) = pos.x;
        At<float>(ped, 0x37C) = pos.y;
        At<float>(ped, 0x380) = pos.z;
        if (At<float>(self, 0x24) >= 0.0f) {
            At<float>(self, 0xCC) = At<float>(ped, 0x364);
            At<float>(ped, 0x364) = At<float>(self, 0x24);
        }
        TetherTarget_GetPosition(ped + 0x35C, &pos);
        At<float>(self, 0xC0) = pos.x;
        At<float>(self, 0xC4) = pos.y;
        At<float>(self, 0xC8) = pos.z;
    }
    return 1;
}

// 0x628120 (thiscall): TetherLocoTrack::Update: the ped goes to its tether (or its
// target with +0x18 and the tether's +0x3B4; a top ally's follow spot) at a speed from
// its distance beyond +0x28 over the tether's scale (or the tether's own), plus how fast
// the spot moves while it's ahead (+0x2F, past +0x34; * +0x30), capped at 1.5 and
// averaged over the last 30 (+0x44.., +0xBC the next; not with +0x2E); still (no speed,
// or a still leader) clears the history. It stops for a leader out of reach, or inside
// the tether with a leader that's +0x548 (not with +0x38 outside it or while the game
// mode is on). Facing: the target (+0x2C) when not going, or the leader; with +0x2D a
// still top ally's.
uint8_t __fastcall TetherLocoTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x3C));
    const float* pp = Pos(ped);
    V3 spot = {pp[0], pp[1], pp[2]};
    if (At<int32_t>(ped, 0x360) == 0) {
        return 0;
    }
    float* move = Move(ped);
    uint8_t* target = nullptr;
    uint8_t* leader = TetherTarget_GetPed(ped + 0x35C);
    bool useTarget = false;
    if (self[0x18] != 0) {
        target = Ped_GetTarget(ped);
        useTarget = target && ped[0x3B4] != 0;
    }
    if (useTarget) {
        const float* tp = Pos(target);
        spot = {tp[0], tp[1], tp[2]};
    } else {
        V3 at;
        TetherTarget_GetPosition(ped + 0x35C, &at);
        spot = at;
        if (leader && leader == Ally_GetTop(ped + 0x2F0)) {
            Ally_FollowSpot(ped + 0x2F0, &spot);
        }
    }
    float radius = At<float>(self, 0x28);
    float dist = Distance2D(&spot.x, Pos(ped));
    uint8_t go = 1;
    if (self[0x38] != 0) {
        if (!Tether_IsInside(ped + 0x358, 0.0f)) {
            go = 0;
        } else if (gameModeObject && gameModeObject[4] != 0) {
            go = 0;
        }
    }
    float speed = At<float>(ped, 0x3B8);
    uint8_t face = 0;
    float excess = dist - radius;
    float scale = At<float>(ped, 0x364);
    if (!(0.0f < excess)) {
        excess = 0.0f;
    }
    if (scale > 0.0f) {
        speed = excess / scale;
    }
    if (self[0x2F] != 0 && At<float>(self, 0x34) <= dist) {
        V3 f, d;
        V3* fwd = Placeable_GetForward(ped, &f);
        V3* r = VectorSub(&d, &spot.x, Pos(ped));
        float ahead = r->y * fwd->y + fwd->x * r->x + r->z * fwd->z;
        if (ahead > 0.45f) {
            float step = combatStepRate * timeStep;
            float moved = Distance2D(&At<float>(self, 0xC0), &spot.x) / step;
            speed = At<float>(self, 0x30) * moved + speed;
        }
    }
    bool clear = false;
    if (speed < 0.0f) {
        speed = 0.0f;
        clear = true;
    } else {
        if (1.5f < speed) {
            speed = 1.5f;
        } else if (speed == 0.0f) {
            clear = true;
        }
        if (!clear && leader && Vector_Magnitude2D(&At<float>(leader, 0x124)) < 0.01f) {
            clear = true;
        }
        if (!clear && self[0x2E] == 0) {
            int32_t at = At<int32_t>(self, 0xBC);
            At<float>(self, 0x44 + at * 4) = speed;
            At<int32_t>(self, 0xBC) += 1;
            if (At<int32_t>(self, 0xBC) >= 0x1E) {
                At<int32_t>(self, 0xBC) = 0;
            }
            int32_t n = 0;
            float sum = 0.0f;
            for (int i = 0; i < 30; i++) {
                float h = At<float>(self, 0x44 + i * 4);
                if (0.0f < h) {
                    n++;
                }
                sum = h + sum;
            }
            if (n > 0) {
                speed = sum / float(n);
            }
        }
    }
    if (clear) {
        for (int i = 0; i < 0x1F; i++) {
            At<int32_t>(self, 0x44 + i * 4) = 0;
        }
    }
    if (go) {
        if (leader && (Combat_IsOutOfReach(ped + 0x668, leader, 5.0f, 1.0f) ||
                       (Tether_IsInside(ped + 0x358, 0.0f) && At<int32_t>(leader, 0x548) != 0))) {
            self[0x40] = 0;
            Steering_Stop(Steering(ped));
            return 0;
        }
        uint32_t flags = At<int32_t>(ped, 0x14EC) != 0 ? 0x1C : 0x20014;
        self[0x40] = Steering_GoTo(Steering(ped), spot.x, spot.y, spot.z, flags, speed, radius, nullptr, 0);
    }
    At<float>(self, 0xC0) = spot.x;
    At<float>(self, 0xC4) = spot.y;
    At<float>(self, 0xC8) = spot.z;
    if (self[0x40] != 0 && speed < 0.1f) {
        self[0x40] = 0;
    }
    if (self[0x2C] != 0) {
        if (target && self[0x40] == 0) {
            face = 1;
            SetFacing(move, FacingTo(ped, Pos(target)));
        } else if (leader && self[0x40] == 0 &&
                   (leader == Ally_GetTop(ped + 0x2F0) || (leader == playerPed && Social_48FB40(ped + 0xB44)))) {
            V3 f = FacingTo(ped, Pos(leader));
            SetFacing(move, f);
            move[7] = 0.0f;
            move[0] = move[3];
            move[1] = move[4];
            move[2] = move[5];
        }
    } else if (self[0x2D] != 0) {
        bool parked = At<int32_t>(ped, 0x153C) == 0 && At<int32_t>(ped, 0x14EC) != 0;
        if (leader && !parked) {
            float* lead = Move(leader);
            if (leader == Ally_GetTop(ped + 0x2F0) && 0.0f == lead[7]) {
                move[3] = lead[3];
                move[4] = lead[4];
                move[5] = lead[5];
                move[0] = lead[0];
                move[1] = lead[1];
                move[2] = lead[2];
                move[7] = 0.0f;
            }
        }
    }
    At<int32_t>(ped, 0x1340) = face;
    if (target) {
        Combat_JoinCurrentTarget(ped + 0x668);
    }
    return 1;
}

// 0x628810 (thiscall): TetherLocoTrack::End: the tether's offset cleared, its speed
// scale back
uint8_t __fastcall TetherLocoTrack_End(uint8_t* self, re::Edx) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x3C));
    At<int32_t>(ped, 0x1340) = 0;
    At<float>(ped, 0x378) = zeroVector[0];
    At<float>(ped, 0x37C) = zeroVector[1];
    At<float>(ped, 0x380) = zeroVector[2];
    if (At<float>(self, 0x24) >= 0.0f) {
        At<float>(ped, 0x364) = At<float>(self, 0xCC);
    }
    return 1;
}

// 0x628870 (thiscall): WaitForTargetLocoTrack::Start: the ped (+0x1C); the ring step
// +0x44 a random 1..3
uint8_t __fastcall WaitForTargetLocoTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    uint8_t* ped = OwnerPed(owner);
    At<uint8_t*>(self, 0x1C) = ped;
    At<int32_t>(ped, 0x1340) = 0;
    At<float>(self, 0x40) = 0.0f;
    float q = float(GameRand()) / 32767.0f;
    At<float>(self, 0x44) = q + q + 1.0f;
    return 1;
}

// 0x6288C0 (thiscall): a spot the ped can wait at for its target (+0x20, last seen at
// +0x24): not ahead of a fast-moving target, inside the tether, within 135 degrees of
// the ped's bearing from where the target was, a clear line of fire from it, and (with
// checkLine) nothing in the way from the ped
uint8_t __fastcall WaitForTargetLocoTrack_CanWaitAtImpl(uint8_t* self, re::Edx, const float* pos, int checkLine) {
    const float* pp = Pos(At<uint8_t*>(self, 0x1C));
    V3 from = {pp[0], pp[1], pp[2]};
    if (Ped_IsMovingFast(At<uint8_t*>(self, 0x20))) {
        float* tm = Move(At<uint8_t*>(self, 0x20));
        V3 v = {tm[0], tm[1], tm[2]};
        V3 d;
        V3* r = VectorSub(&d, pos, &At<float>(self, 0x24));
        if (r->y * v.y + r->x * v.x + r->z * v.z > 0.0f) {
            return 0;
        }
    }
    if (!Tether_IsPosAllowed(At<uint8_t*>(self, 0x1C) + 0x358, pos, 0.0f)) {
        return 0;
    }
    float toSpot = HeadingBetween(&At<float>(self, 0x24), pos);
    float toPed = HeadingBetween(&At<float>(self, 0x24), &from.x);
    float off = std::fabs(LimitAngle(toSpot - toPed));
    if (off > 2.356f) {
        return 0;
    }
    if (!LineOfFire_IsClear(OwnerPed(At<uint8_t*>(self, 0x18)) + 0x668, pos, At<uint8_t*>(self, 0x20), 3)) {
        return 0;
    }
    if (re::Flag(checkLine)) {
        ColPoint cp;
        ColPoint_Construct(&cp);
        re::Ptr32<uint8_t> hit = nullptr;
        World_TestLine(&from.x, pos, &cp, &hit, 9, 0, 0);
        if (hit) {
            ColPoint_Destruct(&cp);
            return 0;
        }
        ColPoint_Destruct(&cp);
    }
    return 1;
}

// 0x628A70 (thiscall): ClearObjectivesTrack::Start
uint8_t __fastcall ClearObjectivesTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    Objectives_Clear(OwnerPed(owner) + 0x5F0);
    return 1;
}

// 0x628A90 (thiscall): CircleAroundTargetLocoTrack::Update: the ped goes to a point
// +0x18 from its target, 0.35 radians round from its bearing (+0x20: the other way),
// turning the point further round itself until it's in sight (flags 0x18, speed +0x1C,
// following the target)
uint8_t __fastcall CircleAroundTargetLocoTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x24));
    uint8_t* target = Ped_GetTarget(ped);
    if (!target) {
        return 0;
    }
    const float* pp = Pos(ped);
    V3 from = {pp[0], pp[1], pp[2]};
    V3 d;
    VectorSub(&d, Pos(target), &from.x);
    float ex = d.x * -1.0f;
    float ey = d.y * -1.0f;
    float angle = trk::CrtAtan2(ex, ey);
    angle = self[0x20] != 0 ? angle + 0.35f : angle - 0.35f;
    float s = trk::CrtSin(angle);
    float c = trk::CrtCos(angle);
    float r = At<float>(self, 0x18);
    V3 off = {r * s, r * c, r * 0.0f};
    const float* tp = Pos(target);
    V3 goal = {tp[0] + off.x, tp[1] + off.y, tp[2] + off.z};
    while (!GetIsLineOfSightClear(&from.x, &goal.x, 1)) {
        V3 g;
        VectorSub(&g, &goal.x, &from.x);
        float a = trk::CrtAtan2(g.x, g.y);
        float len = SqrtAbs(g.y * g.y + g.x * g.x);
        a = self[0x20] != 0 ? a + 0.35f : a - 0.35f;
        float s2 = trk::CrtSin(a);
        float c2 = trk::CrtCos(a);
        V3 o = {len * s2, len * c2, len * 0.0f};
        goal = {from.x + o.x, from.y + o.y, from.z + o.z};
    }
    return Steering_GoTo(Steering(ped), goal.x, goal.y, goal.z, 0x18, At<float>(self, 0x1C), 0.0f, target, 0) != 0 ? 1
                                                                                                                  : 0;
}

// 0x628D50 (thiscall): OnBikeDashTrack::Update: between +0x20 and +0x24 ms after its
// start (+0x2C) the ped rides at its target, otherwise straight on (speed +0x18)
uint8_t __fastcall OnBikeDashTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x28));
    uint8_t* target = Ped_GetTarget(ped);
    if (!target) {
        return 0;
    }
    float t = float(double(timeInMs) - double(At<float>(self, 0x2C)));
    V3 f;
    Placeable_GetForward(ped, &f);
    float speed = At<float>(self, 0x18);
    const float* to;
    V3 ahead;
    if (t < At<float>(self, 0x20) || At<float>(self, 0x24) < t) {
        to = &VectorAdd(&ahead, Pos(ped), &f.x)->x;
    } else {
        to = Pos(target);
    }
    return Steering_GoTo(Steering(ped), to[0], to[1], to[2], 0, speed, 0.0f, nullptr, 0);
}

// 0x628FD0 (thiscall): JumpLocoTrack::Start: the ped's jump count up, state 0, 30 s
uint8_t __fastcall JumpLocoTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    At<int32_t>(OwnerPed(owner), 0x1580) += 1;
    At<int32_t>(self, 0x1C) = 0;
    At<int32_t>(self, 0x20) = 0;
    Timer_Set(self + 0x24, 30.0f);
    return 1;
}

// 0x629000 (thiscall): JumpLocoTrack::Update: for a jump objective (0x1A: the take-off
// spot, heading, speed, a landing spot; 0x1B / 0x1C: the spot and heading only): state
// 0 walk to the take-off (flags 0xC1C) and jump (4) there, facing its heading (up to 3
// tries); 1 in the air (keeping the heading while it swims) then 2 walk to the landing
// (0xC08) for 3 s. Done (the objective removed) once landed or after the timer.
uint8_t __fastcall JumpLocoTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    V3 spot = {0.0f, 0.0f, 0.0f};
    float speed = 0.0f;
    uint8_t hasLanding = 0;
    V3 dir = {0.0f, 0.0f, 0.0f};
    V3 landing = {0.0f, 0.0f, 0.0f};
    float heading;
    uint8_t* queue = ped + 0x5F0;
    uint8_t* current = Queue_GetCurrent(queue);
    if (!current) {
        return 0;
    }
    int32_t type = At<int32_t>(current, 8);
    if (type == 0x1A) {
        spot = {At<float>(current, 0x10), At<float>(current, 0x14), At<float>(current, 0x18)};
        heading = At<float>(current, 0x1C);
        V3 h;
        V3* r = Vector_FromAngle(&h, heading);
        speed = At<float>(current, 0x20);
        dir = *r;
        hasLanding = current[0x24];
        landing = {At<float>(current, 0x28), At<float>(current, 0x2C), At<float>(current, 0x30)};
    } else if (type == 0x1B || type == 0x1C) {
        spot = {At<float>(current, 0x10), At<float>(current, 0x14), At<float>(current, 0x18)};
        heading = At<float>(current, 0x1C);
    } else {
        return 0;
    }
    Distance2D(Pos(ped), &spot.x);   // (not used)
    uint8_t landed = 0;
    if (hasLanding && JF_IsReachable(Pos(ped), &spot.x, &landing.x)) {
        landed = 1;
    }
    auto faceJump = [&]() {
        Ped_SetHeading(ped, heading);
        float* move = Move(ped);
        move[0] = dir.x;
        move[1] = dir.y;
        move[2] = dir.z;
        move = Move(ped);
        move[3] = dir.x;
        move[4] = dir.y;
        move[5] = dir.z;
        Move(ped)[7] = speed;
    };
    switch (At<int32_t>(self, 0x1C)) {
    case 0:
        if (Ped_IsSwimming(ped)) {
            break;
        }
        if (At<int32_t>(self, 0x20) >= 3) {
            landed = 1;
            break;
        }
        if (hasLanding && JF_PedOnLine(ped, &spot.x, &landing.x)) {
            Steering_GoTo(Steering(ped), spot.x, spot.y, spot.z, 0xC1C, 1.0f, 5.0f, nullptr, 0);
            break;
        }
        if (0.0f > DistanceBeyond(Pos(ped), &spot.x, 0.3f)) {
            faceJump();
            ActionInputs_SetRequest(At<uint8_t*>(ped, 0x2E0), 4, 1, 0.0f);
            At<int32_t>(self, 0x20) += 1;
            At<int32_t>(self, 0x1C) = 1;
            Timer_Set(self + 0x24, 10.0f);
            break;
        }
        Steering_GoTo(Steering(ped), spot.x, spot.y, spot.z, 0xC1C, 1.0f, 0.0f, nullptr, 0);
        break;
    case 1:
        if (Ped_IsSwimming(ped)) {
            faceJump();
        } else if (!hasLanding) {
            landed = 1;
        } else {
            At<int32_t>(self, 0x1C) = 2;
            Timer_Set(self + 0x24, 3.0f);
        }
        break;
    case 2:
        if (Timer_IsDone(self + 0x24)) {
            At<int32_t>(self, 0x1C) = 0;
            Timer_Set(self + 0x24, 10.0f);
            break;
        }
        if (0.0f > DistanceBeyond(Pos(ped), &landing.x, 0.3f)) {
            landed = 1;
            break;
        }
        Steering_GoTo(Steering(ped), landing.x, landing.y, landing.z, 0xC08, 1.0f, 0.0f, nullptr, 0);
        break;
    default:
        break;
    }
    if (!Timer_IsDone(self + 0x24) && !landed) {
        return 1;
    }
    Queue_RemoveFirst(queue);
    return 0;
}

// 0x629410 (thiscall): RatLocoTrack's refresh: the rat (+0x4C), its position (+0x50)
// and the player's (+0x5C)
uint8_t __fastcall RatLocoTrack_RefreshImpl(uint8_t* self, re::Edx) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x28));
    At<uint8_t*>(self, 0x4C) = ped;
    const float* p = Pos(ped);
    At<float>(self, 0x50) = p[0];
    At<float>(self, 0x54) = p[1];
    At<float>(self, 0x58) = p[2];
    V3 player;
    float* pc = FindPlayerCoors(&player.x);
    At<float>(self, 0x5C) = pc[0];
    At<float>(self, 0x60) = pc[1];
    At<float>(self, 0x64) = pc[2];
    return 1;
}

// 0x629470 (thiscall): the rat can run to pos: inside its tether and on the meshes,
// not inside an obstacle (the first argument isn't read)
uint8_t __fastcall RatLocoTrack_CanGoToImpl(uint8_t* self, re::Edx, const float*, const float* pos) {
    if (!Tether_IsPosAllowed(At<uint8_t*>(self, 0x4C) + 0x358, pos, 0.0f)) {
        return 0;
    }
    uint8_t inObstacle = 0;
    if (!Walk_IsOnMesh(pos, &inObstacle)) {
        return 0;
    }
    return inObstacle == 0 ? 1 : 0;
}

// the rat back to wandering (state 0): its timer cleared, standing
void RatWander(uint8_t* self) {
    At<int32_t>(self, 0x2C) = 0;
    Timer_Reset(self + 0x30);
    At<float>(self, 0x40) = At<float>(self, 0x50);
    At<float>(self, 0x44) = At<float>(self, 0x54);
    self[0x3C] = 0;
    At<float>(self, 0x48) = At<float>(self, 0x58);
}
// towards its goal (+0x40), dropped within half a unit or when the steering fails
void RatRun(uint8_t* self, uint32_t flags) {
    if (DistanceBeyond(&At<float>(self, 0x50), &At<float>(self, 0x40), 0.5f) < 0.0f) {
        self[0x3C] = 0;
    }
    if (self[0x3C] != 0 &&
        !Steering_GoTo(Steering(At<uint8_t*>(self, 0x4C)), At<float>(self, 0x40), At<float>(self, 0x44),
                       At<float>(self, 0x48), flags, At<float>(self, 0x18), 0.3f, nullptr, 0)) {
        self[0x3C] = 0;
    }
}
// a goal +len at a random angle from where it is
void RatPickGoal(uint8_t* self, float len, float angle) {
    V3 dir, step;
    V3* d = Vector_FromAngle(&dir, angle);
    V3* s = VectorScale(&step, d, len);
    Vector_AddTo(reinterpret_cast<V3*>(&At<float>(self, 0x40)), s);
    self[0x3C] = RatLocoTrack_CanGoTo(self, &At<float>(self, 0x50), &At<float>(self, 0x40));
}

// 0x62D880 (thiscall): RatLocoTrack's step (+0x2C): 0 wander (every 4th frame: to
// food (2), else a new goal 5..10 away), not towards the player (within +0x20 of the
// line); 1 flee the player beyond +0x24 (back to wandering when it's far); 2 run (food /
// away), failing: 3 hide 3 s; 3 then wander again (a 10 s timer at +0x6C)
void __fastcall RatLocoTrack_StepImpl(uint8_t* self, re::Edx) {
    uint32_t flags = ratAvoidFlag != 0 ? 0x1C : 0xC;
    uint8_t frame = Ped_EveryNthFrame(At<uint8_t*>(self, 0x4C), 4);
    switch (At<int32_t>(self, 0x2C)) {
    case 0: {
        if (frame) {
            if (RatLocoTrack_FindFood(self)) {
                if (At<int32_t>(self, 0x2C) != 2) {
                    At<int32_t>(self, 0x2C) = 2;
                }
                return;
            }
            if (self[0x3C] == 0) {
                At<float>(self, 0x40) = At<float>(self, 0x50);
                At<float>(self, 0x44) = At<float>(self, 0x54);
                At<float>(self, 0x48) = At<float>(self, 0x58);
                float len = RandomFloat(5.0f, 10.0f);
                float angle = RandomFloat(0.0f, ratTurnRange);
                RatPickGoal(self, len, angle);
                if (self[0x3C] == 0) {
                    return;
                }
            }
        } else if (self[0x3C] == 0) {
            return;
        }
        if (DistanceBeyond(&At<float>(self, 0x50), &At<float>(self, 0x40), 0.5f) < 0.0f) {
            self[0x3C] = 0;
        }
        float keep = At<float>(self, 0x20);
        float line = DistToLine2D(&At<float>(self, 0x50), &At<float>(self, 0x40), &At<float>(self, 0x5C));
        if (keep > line) {
            self[0x3C] = 0;
        }
        if (self[0x3C] != 0 &&
            !Steering_GoTo(Steering(At<uint8_t*>(self, 0x4C)), At<float>(self, 0x40), At<float>(self, 0x44),
                           At<float>(self, 0x48), flags, At<float>(self, 0x18), 0.3f, nullptr, 0)) {
            self[0x3C] = 0;
        }
        return;
    }
    case 1: {
        float flee = At<float>(self, 0x24);
        if (DistanceSquaredBeyond(&At<float>(self, 0x50), &At<float>(self, 0x5C), flee) > 0.0f) {
            if (At<int32_t>(self, 0x2C) != 0) {
                RatWander(self);
            }
            RatLocoTrack_Step(self);
            return;
        }
        bool pick = self[0x3C] == 0;
        if (!pick) {
            float near = At<float>(self, 0x24);
            if (DistanceSquaredBeyond(&At<float>(self, 0x5C), &At<float>(self, 0x40), near) < 0.0f) {
                float len = At<float>(self, 0x24) + 1.0f;
                V3 d, dir, sum;
                VectorSub(&d, &At<float>(self, 0x50), &At<float>(self, 0x5C));
                FlatDirection(&dir, &d, len);
                V3* g = VectorAdd(&sum, &At<float>(self, 0x5C), &dir.x);
                At<float>(self, 0x40) = g->x;
                At<float>(self, 0x44) = g->y;
                At<float>(self, 0x48) = g->z;
                self[0x3C] = RatLocoTrack_CanGoTo(self, &At<float>(self, 0x50), &At<float>(self, 0x40));
            }
            pick = self[0x3C] == 0;
        }
        bool run = true;
        if (pick) {
            float len = At<float>(self, 0x24) + 1.0f;
            At<float>(self, 0x40) = At<float>(self, 0x5C);
            At<float>(self, 0x44) = At<float>(self, 0x60);
            At<float>(self, 0x48) = At<float>(self, 0x64);
            float angle = RandomFloat(0.0f, ratTurnRange);
            RatPickGoal(self, len, angle);
            run = self[0x3C] != 0;
        }
        if (run) {
            RatRun(self, flags);
        }
        [[fallthrough]];
    }
    case 2:
        if (!Steering_GoTo(Steering(At<uint8_t*>(self, 0x4C)), At<float>(self, 0x40), At<float>(self, 0x44),
                           At<float>(self, 0x48), flags, At<float>(self, 0x18), 0.3f, nullptr, 0) &&
            At<int32_t>(self, 0x2C) != 3) {
            At<int32_t>(self, 0x2C) = 3;
            Countdown_Start(self + 0x30, 3000);
        }
        return;
    case 3:
        if (Timer_IsDone(self + 0x30)) {
            if (At<int32_t>(self, 0x2C) != 0) {
                RatWander(self);
            }
            Countdown_Start(self + 0x6C, 10000);
        }
        return;
    default:
        return;
    }
}

// 0x62D800 (thiscall): RatLocoTrack::Start: its wobble (+0x68) random up to the turn
// range; refreshed and wandering
uint8_t __fastcall RatLocoTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    float most = ratTurnRange;
    At<uint8_t*>(self, 0x28) = owner;
    float q = float(GameRand()) / 32767.0f;
    At<float>(self, 0x68) = q * (most - 0.0f) + 0.0f;
    if (!RatLocoTrack_Refresh(self)) {
        return 0;
    }
    if (At<int32_t>(self, 0x2C) != 0) {
        RatWander(self);
    }
    return 1;
}

// 0x62E520 (thiscall): RatLocoTrack::Update: refreshed; the player within +0x20: flee
// (state 1, running); the step and the wobble
uint8_t __fastcall RatLocoTrack_Update(uint8_t* self, re::Edx, float) {
    if (!RatLocoTrack_Refresh(self)) {
        return 0;
    }
    float scare = At<float>(self, 0x20);
    if (DistanceSquaredBeyond(&At<float>(self, 0x50), &At<float>(self, 0x5C), scare) < 0.0f &&
        At<int32_t>(self, 0x2C) != 1) {
        At<int32_t>(self, 0x2C) = 1;
        Timer_Reset(self + 0x30);
        At<float>(self, 0x40) = At<float>(self, 0x50);
        At<float>(self, 0x44) = At<float>(self, 0x54);
        self[0x3C] = 1;
        At<float>(self, 0x48) = At<float>(self, 0x58);
    }
    RatLocoTrack_Step(self);
    RatLocoTrack_Wobble(self);
    return 1;
}

// 0x62D040 (thiscall): WaitForTargetLocoTrack::Update: facing the scanner's pick (+0x20,
// its position kept at +0x24); outside its tether (with +0x3B5) back into it (speed 1.5);
// with a gun and the target more than 1 above / below: a waiting spot (+0x34, +0x30
// found) walked to (flags 0x2261C); a new one from a ring round the target's last spot
// (+0x40 out, +0x44 more each try, up to 4 tries each side of the ped's bearing) or,
// past the gun's reach, a random one; +0x4EE cleared there
uint8_t __fastcall WaitForTargetLocoTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* pick = Scanner_48C030(At<uint8_t*>(self, 0x1C) + 0x3BC);
    At<uint8_t*>(self, 0x20) = pick;
    if (pick) {
        const float* tp = Pos(pick);
        At<float>(self, 0x24) = tp[0];
        At<float>(self, 0x28) = tp[1];
        At<float>(self, 0x2C) = tp[2];
    }
    uint8_t* ped = At<uint8_t*>(self, 0x1C);
    if (!ped || !pick) {
        return 0;
    }
    V3 d, face;
    VectorSub(&d, Pos(pick), Pos(ped));
    FlatDirection(&face, &d, 1.0f);
    At<int32_t>(At<uint8_t*>(self, 0x1C), 0x1340) = 1;
    SetDirection(Move(At<uint8_t*>(self, 0x1C)), face);
    SetFacing(Move(At<uint8_t*>(self, 0x1C)), face);
    ped = At<uint8_t*>(self, 0x1C);
    const float* pp = Pos(ped);
    V3 from = {pp[0], pp[1], pp[2]};
    if (At<int32_t>(ped, 0x360) != 0 && ped[0x3B5] != 0 && !Tether_IsPosAllowed(ped + 0x358, &from.x, 0.0f)) {
        V3 back = from;
        Tether_Clamp(At<uint8_t*>(self, 0x1C) + 0x358, &back, -0.3f);
        Steering_GoTo(Steering(At<uint8_t*>(self, 0x1C)), back.x, back.y, back.z, 0, 1.5f, 0.3f, nullptr, 0);
        return 1;
    }
    if (!Combat_HasRangedWeapon(At<uint8_t*>(self, 0x1C) + 0x668)) {
        return 1;
    }
    if (!(std::fabs(from.z - At<float>(self, 0x2C)) > 1.0f)) {
        return 1;
    }
    uint8_t going = 0;
    bool search = true;
    if (self[0x30] != 0) {
        if (!WaitForTargetLocoTrack_CanWaitAt(self, &At<float>(self, 0x34), 0)) {
            self[0x30] = 0;
            At<float>(self, 0x40) = 0.0f;
        } else {
            going = Steering_GoTo(Steering(At<uint8_t*>(self, 0x1C)), At<float>(self, 0x34), At<float>(self, 0x38),
                                  At<float>(self, 0x3C), 0x2261C, 1.5f, 0.3f, nullptr, 0);
            search = !going;
        }
    } else {
        self[0x30] = 0;
    }
    if (search && self[0x30] == 0) {
        float reach = 1.5f;
        if (Ped_HasWeapon(At<uint8_t*>(self, 0x1C))) {
            reach = At<float>(At<uint8_t*>(At<uint8_t*>(self, 0x1C), 0x1D0), 0x120);
        }
        if (!(reach > At<float>(self, 0x40))) {
            float angle = RandomFloat(0.0f, waitSpotAngleRange);
            float len = RandomFloat(1.0f, reach);
            V3 dir, off, sum;
            Vector_FromAngle(&dir, angle);
            VectorScale(&off, &dir, len);
            V3* s = VectorAdd(&sum, &At<float>(self, 0x24), &off.x);
            V3 spot = {s->x, s->y, from.z + 1.0f};
            uint8_t ok = WaitForTargetLocoTrack_CanWaitAt(self, &spot.x, 1);
            self[0x30] = ok;
            if (!ok) {
                return 1;
            }
            At<float>(self, 0x40) = 0.0f;
            At<float>(self, 0x34) = spot.x;
            At<float>(self, 0x38) = spot.y;
            At<float>(self, 0x3C) = spot.z;
            At<float>(self, 0x44) = RandomFloat(1.0f, 3.0f);
        } else {
            V3 v, bearing;
            VectorSub(&v, &from.x, Pos(At<uint8_t*>(self, 0x20)));
            FlatDirection(&bearing, &v, 1.0f);
            for (int i = 0; i < 4; i++) {
                float heading = HeadingFromVector(&bearing);
                float step = float(i) * combatMaxAngle;
                float one = LimitAngle(heading + step);
                float other = LimitAngle(heading - step);
                V3 h;
                V3* r = Vector_FromAngle(&h, one);
                bearing = {r->x, r->y, 0.0f};
                Vector_Normalise(&bearing);
                float out = At<float>(self, 0x44) + At<float>(self, 0x40);
                At<float>(self, 0x40) = out;
                V3 off = {bearing.x * out, bearing.y * out, out * bearing.z};
                V3 sum;
                V3* s = VectorAdd(&sum, &At<float>(self, 0x24), &off.x);
                V3 spot = {s->x, s->y, s->z};
                spot.z = FindGroundZ2D(spot.x, spot.y, 1);
                uint8_t ok = WaitForTargetLocoTrack_CanWaitAt(self, &spot.x, 1);
                self[0x30] = ok;
                if (ok) {
                    At<float>(self, 0x34) = spot.x;
                    At<float>(self, 0x38) = spot.y;
                    At<float>(self, 0x3C) = spot.z;
                    break;
                }
                r = Vector_FromAngle(&h, other);
                bearing = {r->x, r->y, 0.0f};
                Vector_Normalise(&bearing);
                V3 off2;
                VectorScale(&off2, &bearing, At<float>(self, 0x40));
                s = VectorAdd(&sum, &At<float>(self, 0x24), &off2.x);
                spot = {s->x, s->y, s->z};
                spot.z = FindGroundZ2D(spot.x, spot.y, 1);
                ok = WaitForTargetLocoTrack_CanWaitAt(self, &spot.x, 1);
                self[0x30] = ok;
                if (ok) {
                    At<float>(self, 0x34) = spot.x;
                    At<float>(self, 0x38) = spot.y;
                    At<float>(self, 0x3C) = spot.z;
                    break;
                }
            }
        }
    }
    if (self[0x30] != 0 && going) {
        Ped_SetMoveTarget(OwnerPed(At<uint8_t*>(self, 0x18)), &At<float>(self, 0x34));
    }
    At<int32_t>(At<uint8_t*>(self, 0x1C), 0x1340) = 0;
    if (DistanceBeyond(Pos(At<uint8_t*>(self, 0x1C)), &At<float>(self, 0x34), 0.5f) < 0.0f) {
        OwnerPed(At<uint8_t*>(self, 0x18))[0x4EE] = 0;
    }
    if (!going) {
        At<int32_t>(At<uint8_t*>(self, 0x1C), 0x1340) = 1;
    } else if (At<uint8_t*>(self, 0x1C)[0x718] != 0) {
        At<int32_t>(At<uint8_t*>(self, 0x1C), 0x1340) = 1;
    } else if (Ped_IsMovingFast(At<uint8_t*>(self, 0x20))) {
        At<int32_t>(At<uint8_t*>(self, 0x1C), 0x1340) = 0;
    }
    ped = At<uint8_t*>(self, 0x1C);
    if (At<int32_t>(ped, 0x1340) != 0) {
        SetFacing(Move(ped), face);
    }
    return 1;
}

} // namespace

#define AILOCOTRK_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AILOCOTRK_REVERSED("ChaseLocoTrack::Start", 0x006276B0, (&LocoTrack_Start<0x24, 0x28>));
AILOCOTRK_REVERSED("ChaseLocoTrack::Update", 0x006276D0, &ChaseLocoTrack_Update);
// (0x62781E is not a function: the tail of ChaseLocoTrack::Update, written there)
RE_UNHOOKED("ChaseLocoTrack::Update_Tail62781E", 0x0062781E);
AILOCOTRK_REVERSED("GuardLocoTrack::Start", 0x00627A00, (&LocoTrack_Start<0x30, 0x34>));
AILOCOTRK_REVERSED("GuardLocoTrack::Update", 0x00627A20, &GuardLocoTrack_Update);
AILOCOTRK_REVERSED("GuardLocoTrack::End", 0x00627D80, &LocoTrack_End<0x30>);
AILOCOTRK_REVERSED("KeepDistanceLocoTrack::Start", 0x00627DA0, (&LocoTrack_Start<0x20, 0x24>));
AILOCOTRK_REVERSED("KeepDistanceLocoTrack::Update", 0x00627DC0, &KeepDistanceLocoTrack_Update);
AILOCOTRK_REVERSED("KeepDistanceLocoTrack::End", 0x00628050, &LocoTrack_End<0x20>);
AILOCOTRK_REVERSED("TetherLocoTrack::Start", 0x00628070, &TetherLocoTrack_Start);
AILOCOTRK_REVERSED("TetherLocoTrack::Update", 0x00628120, &TetherLocoTrack_Update);
AILOCOTRK_REVERSED("TetherLocoTrack::End", 0x00628810, &TetherLocoTrack_End);
AILOCOTRK_REVERSED("WaitForTargetLocoTrack::Start", 0x00628870, &WaitForTargetLocoTrack_Start);
AILOCOTRK_REVERSED("WaitForTargetLocoTrack::CanWaitAt", 0x006288C0, &WaitForTargetLocoTrack_CanWaitAtImpl);
AILOCOTRK_REVERSED("WaitForTargetLocoTrack::End", 0x00628A50, &LocoTrack_End<0x18>);
AILOCOTRK_REVERSED("ClearObjectivesTrack::Start", 0x00628A70, &ClearObjectivesTrack_Start);
AILOCOTRK_REVERSED("CircleAroundTargetLocoTrack::Update", 0x00628A90, &CircleAroundTargetLocoTrack_Update);
AILOCOTRK_REVERSED("OnBikeDashTrack::Update", 0x00628D50, &OnBikeDashTrack_Update);
AILOCOTRK_REVERSED("JumpLocoTrack::Start", 0x00628FD0, &JumpLocoTrack_Start);
AILOCOTRK_REVERSED("JumpLocoTrack::Update", 0x00629000, &JumpLocoTrack_Update);
AILOCOTRK_REVERSED("RatLocoTrack::Refresh", 0x00629410, &RatLocoTrack_RefreshImpl);
AILOCOTRK_REVERSED("RatLocoTrack::CanGoTo", 0x00629470, &RatLocoTrack_CanGoToImpl);
AILOCOTRK_REVERSED("WaitForTargetLocoTrack::Update", 0x0062D040, &WaitForTargetLocoTrack_Update);
AILOCOTRK_REVERSED("RatLocoTrack::Start", 0x0062D800, &RatLocoTrack_Start);
AILOCOTRK_REVERSED("RatLocoTrack::Step", 0x0062D880, &RatLocoTrack_StepImpl);
AILOCOTRK_REVERSED("RatLocoTrack::Update", 0x0062E520, &RatLocoTrack_Update);
