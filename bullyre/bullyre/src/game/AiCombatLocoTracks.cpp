// The action tree's CombatLocoTrack: a ped circling its opponent in a fight, keeping
// between the AI's engage distances (closer with a gun), joining the opponent's
// attacker slots, and picking where to go next by state. 0x625390..0x62EBF2 (its
// virtuals and steps; the helpers of 0x61B510..0x61BA40 are in TracksAICombat.cpp).
// A combat locomotion track: +0x18 fight the player, +0x19 hold still, +0x1C the owner,
// +0x20 the ped, +0x24 the goal's kind, +0x28 the state (0 reposition, 1 go, 2 wait, 3
// circle), +0x2C the goal (+0x38 from the centre), +0x44 ms in the state, +0x48 ms
// waited, +0x4C ms out of talking reach, +0x50 ms unable to shoot, +0x54 can shoot from
// here, +0x5C the opponent, +0x60 the centre (its position), +0x6C / +0x70 the near /
// far distance, +0x74 their mood bonus, +0x78 think this frame, +0x79 think next frame,
// +0x7A has a usable gun, +0x7C its range.
// The ped: +0x358 its tether, +0x668 its combat slots, +0xA64 its social, +0x1340 /
// +0x14A0 moving / circling flags, +0x9B4 / +0x9B8 (a ped it's grappled with / by).

#include <cmath>
#include <cstdint>

#include "game/ActionTracksCommon.h"

#include "game/Crt.h"
using namespace actc;

namespace {

GAME_VAR(float, combatStepRate, 0x00B0C0B0);
GAME_VAR(float, combatMaxAngle, 0x00B0C0A0);
GAME_VAR(float, combatAngleOffset, 0x00B0C0A4);
GAME_VAR(float, combatRetreatSpread, 0x00B0C0A8);
GAME_VAR(uint8_t, playerCircled, 0x00CF3AE6);

GAME_FN(uint8_t*, __thiscall, Scanner_48C030, 0x0048C030, uint8_t* scanner);
GAME_FN(void, __thiscall, Ped_SetTargetEntity, 0x00498B10, uint8_t* ped, uint8_t* target);
GAME_FN(uint8_t, __thiscall, Combat_HasUsableWeapon, 0x00486BE0, uint8_t* combat);
GAME_FN(uint8_t, __thiscall, Combat_JoinTarget, 0x00487BC0, uint8_t* combat, uint8_t* target, int list, int dontCommit);
GAME_FN(void, __thiscall, Combat_Leave_487B70, 0x00487B70, uint8_t* combat);
GAME_FN(void, __thiscall, Combat_Leave_486D50, 0x00486D50, uint8_t* combat);
GAME_FN(float, __thiscall, Weapon_GetRangeFactor, 0x004D96B0, uint8_t* weapon);
GAME_FN(int32_t*, __thiscall, Social_GetMood, 0x0048EBC0, uint8_t* social, int32_t* out, uint8_t* ped, int a, uintptr_t b,
        uintptr_t c);
GAME_FN(uint8_t, __thiscall, Social_CanTalkTo, 0x0048D470, uint8_t* social, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Tether_IsPosAllowed, 0x00499840, uint8_t* tether, const float* pos, float range);
GAME_FN(uint8_t, __thiscall, LineOfFire_IsClear, 0x0048A700, uint8_t* combat, const float* from, uint8_t* target,
        int flags);
GAME_FN(uint8_t, __cdecl, Walk_TestLine, 0x005EB1B0, const float* from, const float* to, uint8_t* blocked, int a, int b);
GAME_FN(uint8_t, __thiscall, NavPoint_IsValid, 0x005E7AF0, const uint8_t* point);
GAME_FN(uint8_t, __cdecl, Nav_WalkToward, 0x005E9F50, uint8_t* from, const float* to, int a, uintptr_t b, int c, int d, uintptr_t e);
GAME_FN(V3*, __cdecl, VectorAdd, 0x004129A0, V3* out, const float* a, const float* b);
GAME_FN(V3*, __cdecl, VectorScale, 0x00412350, V3* out, const V3* v, float s);
GAME_FN(V3*, __cdecl, FlatDirection, 0x0041A520, V3* out, const V3* v, float length);
GAME_FN(float, __cdecl, DistanceBeyond, 0x0041A4D0, const float* a, const float* b, float range);
GAME_FN(float, __cdecl, HeadingBetween, 0x00433350, const float* from, const float* to);
GAME_FN(V3*, __cdecl, Vector_FromAngle, 0x00432DF0, V3* out, float angle);
GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_FN(V3*, __thiscall, Entity_GetRight, 0x004736F0, uint8_t* entity, V3* out);
GAME_FN(float, __cdecl, RandomFloat, 0x0041A5B0, float low, float high);
inline int __cdecl GameRand() { return crt::Rand(); }
GAME_FN(int, __cdecl, GetFrameMs, 0x00446870);
GAME_FN(uint8_t, __thiscall, Ped_IsSeatedInVehicle, 0x004CE690, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Ped_IsMovingFast, 0x00473DD0, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Ped_EveryNthFrame, 0x00465A60, uint8_t* ped, uint32_t n);
GAME_FN(uint8_t, __thiscall, Steering_GoTo, 0x004A5000, uint8_t* steering, float x, float y, float z, uint32_t flags,
        float speed, float radius, uint8_t* entity, int flag);
// the track's own, by address (so their modes apply)
GAME_FN(void, __thiscall, CombatLocoTrack_SetGoal, 0x0061B590, uint8_t* self, const float* pos, int32_t kind);
GAME_FN(V3*, __thiscall, CombatLocoTrack_Circle, 0x0061B5F0, uint8_t* self, V3* out);
GAME_FN(void, __thiscall, CombatLocoTrack_PickAngle, 0x0061B800, uint8_t* self);
GAME_FN(void, __thiscall, CombatLocoTrack_UpdateTimer, 0x0061B860, uint8_t* self);
GAME_FN(float, __thiscall, CombatLocoTrack_CentreUrge, 0x0061B980, uint8_t* self);
GAME_FN(float, __thiscall, CombatLocoTrack_GoalUrge, 0x0061BA40, uint8_t* self);
GAME_FN(uint8_t, __thiscall, CombatLocoTrack_FindOpponent, 0x006253D0, uint8_t* self);
GAME_FN(uint8_t, __thiscall, CombatLocoTrack_JoinSlots, 0x00625430, uint8_t* self);
GAME_FN(void, __thiscall, CombatLocoTrack_UpdateRange, 0x00625540, uint8_t* self);
GAME_FN(uint8_t, __thiscall, CombatLocoTrack_CanShootFrom, 0x00625620, uint8_t* self, const float* pos);
GAME_FN(uint8_t, __thiscall, CombatLocoTrack_Approach, 0x006256A0, uint8_t* self);
GAME_FN(uint8_t, __thiscall, CombatLocoTrack_Wait, 0x006258A0, uint8_t* self);
GAME_FN(void, __thiscall, CombatLocoTrack_CircleStep, 0x00625920, uint8_t* self);
GAME_FN(void, __thiscall, CombatLocoTrack_Move, 0x00625AD0, uint8_t* self);
GAME_FN(uint8_t, __thiscall, CombatLocoTrack_TryGoal, 0x0062C650, uint8_t* self, const V3* offset, int32_t kind);
GAME_FN(void, __thiscall, CombatLocoTrack_UpdateTimers, 0x0062C6A0, uint8_t* self);
GAME_FN(void, __thiscall, CombatLocoTrack_Reposition, 0x0062C8A0, uint8_t* self);
GAME_FN(void, __thiscall, CombatLocoTrack_Think, 0x0062E280, uint8_t* self);

// 0x61B510: a point pulled to a flat distance from a centre (its arguments in esi, edi
// and on the stack: called the game's way)
__declspec(noinline) void KeepAtDistance(V3* point, const float* centre, float distance) {
#if defined(_M_IX86)
    void* fn = reinterpret_cast<void*>(0x0061B510);
    __asm {
        mov esi, point
        mov edi, centre
        push distance
        call fn
        add esp, 4
    }
#else
    re::Ours<void (*)(V3*, const float*, float)>(0x0061B510)(point, centre, distance);
#endif
}

uint8_t* Ped(uint8_t* self) {
    return At<uint8_t*>(self, 0x20);
}
float* Centre(uint8_t* self) {
    return &At<float>(self, 0x60);
}
int32_t StepMs() {
    return int32_t(int64_t(combatStepRate * timeStep * 1000.0f));
}
void Rethink(uint8_t* self) {
    self[0x79] = 1;
    At<int32_t>(self, 0x28) = 0;
}
// a nav point (the opponent's path follower's, +0xDAC +0x4BC) walked towards pos
uint8_t CanWalkTo(uint8_t* opponent, const float* pos) {
    uint8_t* point = opponent + 0xDAC + 0x4BC;
    uint8_t copy[0x14];
    for (int i = 0; i < 0x14; i += 4) {
        At<uint32_t>(copy, i) = At<uint32_t>(point, i);
    }
    return NavPoint_IsValid(point) && Nav_WalkToward(copy, pos, 0, 0, 0, 1, 0);
}

// 0x625390 (thiscall): CombatLocoTrack::End: the ped's moving / circling flags cleared,
// its combat slots left
uint8_t __fastcall CombatLocoTrack_End(uint8_t* self, re::Edx) {
    At<int32_t>(Ped(self), 0x14A0) = 0;
    At<int32_t>(Ped(self), 0x1340) = 0;
    Combat_Leave_487B70(Ped(self) + 0x668);
    Combat_Leave_486D50(Ped(self) + 0x668);
    return 1;
}

// 0x6253D0 (thiscall): the opponent (+0x5C): the player (made the ped's target) when
// +0x18, else the scanner's; its position is the centre
uint8_t __fastcall CombatLocoTrack_FindOpponentImpl(uint8_t* self, re::Edx) {
    if (self[0x18] != 0) {
        uint8_t* player = playerPed;
        At<uint8_t*>(self, 0x5C) = player;
        Ped_SetTargetEntity(Ped(self), player);
    } else {
        At<uint8_t*>(self, 0x5C) = Scanner_48C030(Ped(self) + 0x3BC);
    }
    uint8_t* opponent = At<uint8_t*>(self, 0x5C);
    if (!opponent) {
        return 0;
    }
    const float* p = Pos(opponent);
    At<float>(self, 0x60) = p[0];
    At<float>(self, 0x64) = p[1];
    At<float>(self, 0x68) = p[2];
    return 1;
}

// 0x625430 (thiscall): when thinking: a usable gun (+0x7A) takes its range (+0x7C: the
// ped's +0xA6C capped by the weapon's +0x120, when it has a range factor) and the
// ranged list (2); else the first of lists 0, 1, 2 that takes it
uint8_t __fastcall CombatLocoTrack_JoinSlotsImpl(uint8_t* self, re::Edx) {
    if (self[0x78] == 0) {
        return 1;
    }
    uint8_t* combat = Ped(self) + 0x668;
    uint8_t gun = Combat_HasUsableWeapon(combat);
    self[0x7A] = gun;
    if (gun) {
        uint8_t* weapon = At<uint8_t*>(Ped(self), 0x1D0);
        if (!(Weapon_GetRangeFactor(weapon) == 0.0f)) {
            float own = At<float>(Ped(self), 0xA6C);
            float most = At<float>(weapon, 0x120);
            At<float>(self, 0x7C) = own > most ? most : own;
        }
        return Combat_JoinTarget(Ped(self) + 0x668, At<uint8_t*>(self, 0x5C), 2, 0) ? 1 : 0;
    }
    for (int list = 0; list < 3; list++) {
        if (Combat_JoinTarget(Ped(self) + 0x668, At<uint8_t*>(self, 0x5C), list, 0)) {
            return 1;
        }
    }
    return 0;
}

// 0x625540 (thiscall): when thinking: the near / far distances from the AI's engage
// distances by the ped's slot (+0x66C: 0..3), a gun's from far to its range; half a
// unit more both ways when its mood to the opponent is 4..6
void __fastcall CombatLocoTrack_UpdateRangeImpl(uint8_t* self, re::Edx) {
    if (self[0x78] == 0) {
        return;
    }
    uint8_t* ped = Ped(self);
    int32_t slot = At<int32_t>(ped, 0x66C);
    int at = slot == 0 ? 0x28 : slot == 1 ? 0x2C : slot == 2 ? 0x30 : 0x34;
    At<float>(self, 0x6C) = At<float>(aiButes, at);
    At<float>(self, 0x70) = At<float>(aiButes, at + 4);
    if (self[0x7A] != 0) {
        At<float>(self, 0x6C) = At<float>(self, 0x70);
        At<float>(self, 0x70) = At<float>(self, 0x7C);
    }
    int32_t mood;
    Social_GetMood(ped + 0xB44, &mood, At<uint8_t*>(self, 0x5C), 0, 0, 0);
    At<float>(self, 0x74) = (mood == 4 || mood == 5 || mood == 6) ? 0.0f + 0.5f : 0.0f;
    At<float>(self, 0x6C) = At<float>(self, 0x6C) + At<float>(self, 0x74);
    At<float>(self, 0x70) = At<float>(self, 0x74) + At<float>(self, 0x70);
}

// 0x625620 (thiscall): the ped could fight from pos: inside its tether, a clear line of
// fire to the opponent, and without a gun a walkable straight line from the centre
uint8_t __fastcall CombatLocoTrack_CanShootFromImpl(uint8_t* self, re::Edx, const float* pos) {
    if (!Tether_IsPosAllowed(Ped(self) + 0x358, pos, 0.0f)) {
        return 0;
    }
    if (!LineOfFire_IsClear(Ped(self) + 0x668, pos, At<uint8_t*>(self, 0x5C), 3)) {
        return 0;
    }
    if (self[0x7A] == 0) {
        uint8_t blocked = 0;
        if (!Walk_TestLine(Centre(self), pos, &blocked, 0, 0) || blocked) {
            return 0;
        }
    }
    return 1;
}

// 0x6256A0 (thiscall): a goal (kind 9) at the ped's bearing from the centre, pulled in
// to the near distance / out to the far one (0.8 of it with a gun); without a gun only
// where the opponent's nav point can walk to it
uint8_t __fastcall CombatLocoTrack_ApproachImpl(uint8_t* self, re::Edx) {
    V3 d;
    VectorSub(&d, Pos(Ped(self)), Centre(self));
    float far = At<float>(self, 0x70);
    if (self[0x7A] != 0) {
        far = far * 0.8f;
    }
    float near = At<float>(self, 0x6C);
    V3 e;
    VectorSub(&e, zeroVector, &d.x);
    float sq = e.y * e.y + e.x * e.x + 0.0f * 0.0f;
    if (sq - near * near < 0.0f) {
        KeepAtDistance(&d, zeroVector, At<float>(self, 0x6C));
    }
    V3 e2;
    VectorSub(&e2, zeroVector, &d.x);
    float sq2 = e2.y * e2.y + e2.x * e2.x + 0.0f;
    if (0.0f < sq2 - far * far) {
        KeepAtDistance(&d, zeroVector, far);
    }
    V3 goal;
    VectorAdd(&goal, Centre(self), &d.x);
    if (self[0x7A] == 0 && !CanWalkTo(At<uint8_t*>(self, 0x5C), &goal.x)) {
        return 0;
    }
    CombatLocoTrack_SetGoal(self, &goal.x, 9);
    return 1;
}

// 0x6258A0 (thiscall): state 2, waiting: a gun re-approaches when thinking; else after
// 300 ms, a coin toss: reposition (state 0), or wait again
uint8_t __fastcall CombatLocoTrack_WaitImpl(uint8_t* self, re::Edx) {
    if (self[0x7A] != 0) {
        if (self[0x78] != 0) {
            return CombatLocoTrack_Approach(self);
        }
        return 0;
    }
    At<int32_t>(self, 0x48) += StepMs();
    if (At<int32_t>(self, 0x48) > 300) {
        if (GameRand() & 1) {
            At<int32_t>(self, 0x28) = 0;
            self[0x79] = 1;
            return 1;
        }
        At<int32_t>(self, 0x48) = 0;
    }
    return 0;
}

// 0x625920 (thiscall; not in Ghidra's list, reached from Think's switch): state 3,
// circling: inside the far distance of an opponent it can talk to (or that sits in a
// vehicle) and that's slow (under 0.055 a step) or not moving fast, it rethinks; else
// the next point round (kind 0xC), or the centre when the opponent's nav can't get there
void __fastcall CombatLocoTrack_CircleStepImpl(uint8_t* self, re::Edx) {
    float far = At<float>(self, 0x70);
    V3 d;
    VectorSub(&d, Pos(Ped(self)), Centre(self));
    float sq = d.y * d.y + d.x * d.x + 0.0f * 0.0f;
    if (0.0f > sq - far * far) {
        uint8_t* opponent = At<uint8_t*>(self, 0x5C);
        if (Social_CanTalkTo(Ped(self) + 0xA64, opponent) || Ped_IsSeatedInVehicle(At<uint8_t*>(self, 0x5C))) {
            opponent = At<uint8_t*>(self, 0x5C);
            float perStep = At<float>(opponent, 0x19C) / timeStep;
            if (perStep < 0.055f || !Ped_IsMovingFast(opponent)) {
                At<int32_t>(self, 0x28) = 0;
                self[0x79] = 1;
                return;
            }
        }
    }
    V3 out = {0.0f, 0.0f, 0.0f};
    V3 next;
    out = *CombatLocoTrack_Circle(self, &next);
    if (CanWalkTo(At<uint8_t*>(self, 0x5C), &out.x)) {
        CombatLocoTrack_SetGoal(self, &out.x, 0xC);
        return;
    }
    CombatLocoTrack_SetGoal(self, Centre(self), 0xC);
}

// 0x625AD0 (thiscall): the ped sent to the goal (when thinking, the centre + the goal's
// offset once it's moved over half a unit): speed by the goal's kind (0 for 2 and 4, the
// centre urge for 0xC, the goal urge for 7 and 9, else 1.5), radius 0.2, flags 0x2241C
// (0x6241C circling); facing the ped it's grappled with (kind 4) or the centre (unless
// kinds 3, 7, 0xC); circling (kinds 3, 0xC) the player marks the player circled
void __fastcall CombatLocoTrack_MoveImpl(uint8_t* self, re::Edx) {
    int32_t kind = At<int32_t>(self, 0x24);
    uint8_t face = (kind == 3 || kind == 7 || kind == 0xC) ? 0 : 1;
    float speed;
    if (kind == 2 || kind == 4) {
        speed = 0.0f;
    } else if (kind == 0xC) {
        speed = CombatLocoTrack_CentreUrge(self);
    } else if (kind == 9 || kind == 7) {
        speed = CombatLocoTrack_GoalUrge(self);
    } else {
        speed = 1.5f;
    }
    kind = At<int32_t>(self, 0x24);
    uint8_t circling = (kind == 3 || kind == 0xC) ? 1 : 0;
    if (self[0x78] != 0) {
        V3 goal;
        VectorAdd(&goal, Centre(self), &At<float>(self, 0x38));
        if (DistanceBeyond(Pos(Ped(self)), &goal.x, 0.5f) > 0.0f) {
            At<float>(self, 0x2C) = goal.x;
            At<float>(self, 0x30) = goal.y;
            At<float>(self, 0x34) = goal.z;
        }
    }
    uint32_t flags = At<int32_t>(self, 0x28) == 3 ? 0x6241C : 0x2241C;
    Steering_GoTo(Ped(self) + 0x654, At<float>(self, 0x2C), At<float>(self, 0x30), At<float>(self, 0x34), flags, speed,
                  0.2f, At<uint8_t*>(self, 0x5C), 0);
    At<int32_t>(Ped(self), 0x1340) = face;
    if (At<int32_t>(self, 0x24) == 4 && At<uint8_t*>(Ped(self), 0x9B8)) {
        uint8_t* ped = Ped(self);
        const float* from = Pos(ped);
        V3 d, dir;
        VectorSub(&d, Pos(At<uint8_t*>(ped, 0x9B8)), from);
        FlatDirection(&dir, &d, 1.0f);
        float* move = At<float*>(Ped(self), 0x2E0);
        move[0] = dir.x;
        move[1] = dir.y;
        move[2] = dir.z;
        move = At<float*>(Ped(self), 0x2E0);
        move[3] = dir.x;
        move[4] = dir.y;
        move[5] = dir.z;
    } else if (face) {
        V3 d;
        V3* r = VectorSub(&d, Centre(self), Pos(Ped(self)));
        V3 dir = {r->x, r->y, 0.0f};
        Vector_Normalise(&dir);
        float* move = At<float*>(Ped(self), 0x2E0);
        move[3] = dir.x;
        move[4] = dir.y;
        move[5] = dir.z;
    }
    At<int32_t>(Ped(self), 0x14A0) = circling;
    if (circling && At<uint8_t*>(self, 0x5C) && At<uint8_t*>(self, 0x5C) == playerPed) {
        playerCircled = 1;
    }
}

// 0x62C650 (thiscall): the centre + offset as the goal (of kind) when the ped could
// fight from there
uint8_t __fastcall CombatLocoTrack_TryGoalImpl(uint8_t* self, re::Edx, const float* offset, int32_t kind) {
    V3 pos;
    VectorAdd(&pos, Centre(self), offset);
    if (CombatLocoTrack_CanShootFrom(self, &pos.x)) {
        CombatLocoTrack_SetGoal(self, &pos.x, kind);
        return 1;
    }
    return 0;
}

// 0x62C6A0 (thiscall): out of reach (not circling: grappling (+0x1448) or beyond far + 2
// of the centre) a new angle; a second out of talking reach, a new angle; a gun that
// can't fire from here for 3 s steps back to a random spot (kind 7, state 1)
void __fastcall CombatLocoTrack_UpdateTimersImpl(uint8_t* self, re::Edx) {
    if (At<int32_t>(self, 0x28) != 3) {
        uint8_t* ped = Ped(self);
        if (At<int32_t>(ped, 0x1448) != 0) {
            CombatLocoTrack_PickAngle(self);
            return;
        }
        float range = At<float>(self, 0x70) + 2.0f;
        if (DistanceBeyond(Centre(self), Pos(ped), range) >= 0.0f) {
            CombatLocoTrack_PickAngle(self);
            return;
        }
    }
    if (!Social_CanTalkTo(Ped(self) + 0xA64, At<uint8_t*>(self, 0x5C))) {
        At<int32_t>(self, 0x4C) += StepMs();
    } else {
        At<int32_t>(self, 0x4C) = 0;
    }
    if (At<int32_t>(self, 0x4C) > 1000) {
        At<int32_t>(self, 0x4C) = 0;
        CombatLocoTrack_PickAngle(self);
        return;
    }
    if (self[0x7A] == 0) {
        return;
    }
    if (self[0x78] != 0) {
        self[0x54] = CombatLocoTrack_CanShootFrom(self, Pos(Ped(self)));
    }
    if (self[0x54] == 0) {
        At<int32_t>(self, 0x50) += GetFrameMs();
    } else {
        At<int32_t>(self, 0x50) = 0;
    }
    if (At<int32_t>(self, 0x50) <= 3000) {
        return;
    }
    float spread = combatRetreatSpread + combatRetreatSpread;
    float far = At<float>(self, 0x70) * 0.8f;
    float dist = RandomFloat(At<float>(self, 0x6C), far);
    float heading = HeadingBetween(Centre(self), Pos(Ped(self)));
    float angle = RandomFloat(-spread, spread) + heading;
    V3 dir;
    V3* r = Vector_FromAngle(&dir, angle);
    V3 offset = {dist * r->x, r->y * dist, r->z * dist};
    if (CombatLocoTrack_TryGoal(self, &offset, 7)) {
        At<int32_t>(self, 0x50) = 0;
        At<int32_t>(self, 0x44) = 0;
        At<int32_t>(self, 0x28) = 1;
        self[0x79] = 1;
    }
}

// 0x62C8A0 (thiscall): state 0: when thinking, a new goal (kind 8) at a random distance
// between near and far: beside the opponent's vehicle (its side the ped is on), else at
// a random angle off the ped's bearing (+-the most angle, pushed out by the angle
// offset); failing that (or with a gun) the approach; then state 1. Otherwise (or when
// nothing fits) the ped holds (kind 10) inside near, else heads for the centre (0xB).
void __fastcall CombatLocoTrack_RepositionImpl(uint8_t* self, re::Edx) {
    if (self[0x78] != 0) {
        uint8_t ok = 0;
        bool placed = false;
        if (self[0x7A] == 0) {
            float most = combatMaxAngle;
            float near = At<float>(self, 0x6C);
            float far = At<float>(self, 0x70);
            float q = float(GameRand()) / 32767.0f;
            float dist = q * (far - near) + near;
            uint8_t* opponent = At<uint8_t*>(self, 0x5C);
            float angle;
            if (opponent && At<uint8_t*>(opponent, 0x1554)) {
                uint8_t* vehicle = At<uint8_t*>(opponent, 0x1554);
                V3 d, right, side;
                V3* dv = VectorSub(&d, Pos(Ped(self)), Pos(vehicle));
                V3* r = Entity_GetRight(vehicle, &right);
                float dot = r->y * dv->y + dv->x * r->x + r->z * dv->z;
                vehicle = At<uint8_t*>(At<uint8_t*>(self, 0x5C), 0x1554);
                V3* s;
                if (dot > 0.0f) {
                    s = VectorAdd(&side, Centre(self), &Entity_GetRight(vehicle, &right)->x);
                } else {
                    s = VectorSub(&side, Centre(self), &Entity_GetRight(vehicle, &right)->x);
                }
                float heading = HeadingBetween(Centre(self), &s->x);
                angle = 0.0f + combatAngleOffset + heading;
            } else {
                float heading = HeadingBetween(Centre(self), Pos(Ped(self)));
                float least = -most;
                float q2 = float(GameRand()) / 32767.0f;
                float a = q2 * (most - least) + least;
                if (a < 0.0f || a != a) {
                    angle = heading - (combatAngleOffset - a);
                } else {
                    angle = a + combatAngleOffset + heading;
                }
            }
            V3 dir;
            V3* r = Vector_FromAngle(&dir, angle);
            V3 offset = {dist * r->x, r->y * dist, r->z * dist};
            if (CombatLocoTrack_TryGoal(self, &offset, 8)) {
                At<int32_t>(self, 0x44) = 0;
                At<int32_t>(self, 0x28) = 1;
                self[0x79] = 1;
                ok = 1;
                placed = true;
            }
        }
        if (!placed) {
            ok = CombatLocoTrack_Approach(self);
        }
        if (ok) {
            At<int32_t>(self, 0x44) = 0;
            At<int32_t>(self, 0x28) = 1;
            self[0x79] = 1;
            return;
        }
    }
    const float* p = Pos(Ped(self));
    if (DistanceBeyond(Centre(self), p, At<float>(self, 0x6C)) < 0.0f) {
        CombatLocoTrack_SetGoal(self, Pos(Ped(self)), 10);
        return;
    }
    CombatLocoTrack_SetGoal(self, Centre(self), 0xB);
}

// 0x62E280 (thiscall): the ped holds (kind 2) when told to (+0x19) or when +0x4E4;
// more than 2 above / below the centre it heads there (3); grappled (+0x9B8) it holds
// (4); grappling (+0x9B4), when thinking, it goes 1.3 in front of the opponent (6) or
// to the centre (5); else the state's step. Holding / heading: rethink.
void __fastcall CombatLocoTrack_ThinkImpl(uint8_t* self, re::Edx) {
    uint8_t* ped = Ped(self);
    if (self[0x19] != 0 || ped[0x4E4] != 0) {
        CombatLocoTrack_SetGoal(self, Pos(Ped(self)), 2);
        Rethink(self);
        return;
    }
    float dz = At<float>(self, 0x68) - Pos(ped)[2];
    if (std::fabs(dz) > 2.0f) {
        CombatLocoTrack_SetGoal(self, Centre(self), 3);
        Rethink(self);
        return;
    }
    if (At<uint8_t*>(ped, 0x9B8)) {
        CombatLocoTrack_SetGoal(self, Pos(ped), 4);
        Rethink(self);
        return;
    }
    if (At<uint8_t*>(ped, 0x9B4)) {
        if (self[0x78] == 0) {
            Rethink(self);
            return;
        }
        V3 f, v;
        VectorScale(&v, Placeable_GetForward(At<uint8_t*>(self, 0x5C), &f), 1.3f);
        if (CombatLocoTrack_TryGoal(self, &v, 6)) {
            Rethink(self);
            return;
        }
        CombatLocoTrack_SetGoal(self, Centre(self), 5);
        At<int32_t>(self, 0x28) = 0;
        self[0x79] = 1;
        return;
    }
    CombatLocoTrack_UpdateTimers(self);
    switch (At<int32_t>(self, 0x28)) {
    case 0:
        CombatLocoTrack_Reposition(self);
        return;
    case 1:
        CombatLocoTrack_UpdateTimer(self);
        return;
    case 2:
        CombatLocoTrack_Wait(self);
        return;
    case 3:
        CombatLocoTrack_CircleStep(self);
        return;
    default:
        return;
    }
}

// 0x62EB40 (thiscall): CombatLocoTrack::Start: all reset (thinking), then a first think
// when it has an opponent and slots
uint8_t __fastcall CombatLocoTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    At<float>(self, 0x74) = 0.0f;
    At<uint8_t*>(self, 0x20) = OwnerPed(owner);
    At<int32_t>(self, 0x24) = 0;
    At<int32_t>(self, 0x28) = 0;
    self[0x7A] = 0;
    At<int32_t>(self, 0x44) = 0;
    At<int32_t>(self, 0x48) = 0;
    At<int32_t>(self, 0x4C) = 0;
    At<int32_t>(self, 0x50) = 0;
    self[0x78] = 1;
    self[0x79] = 0;
    if (CombatLocoTrack_FindOpponent(self) && CombatLocoTrack_JoinSlots(self)) {
        CombatLocoTrack_UpdateRange(self);
        CombatLocoTrack_Think(self);
    }
    return 1;
}

// 0x62EBA0 (thiscall): CombatLocoTrack::Update: thinks every 8th frame (or when asked);
// ends without an opponent or slots
uint8_t __fastcall CombatLocoTrack_Update(uint8_t* self, re::Edx, float) {
    self[0x78] = Ped_EveryNthFrame(Ped(self), 8);
    if (self[0x79] != 0) {
        self[0x78] = 1;
        self[0x79] = 0;
    }
    if (CombatLocoTrack_FindOpponent(self) && CombatLocoTrack_JoinSlots(self)) {
        CombatLocoTrack_UpdateRange(self);
        CombatLocoTrack_Think(self);
        CombatLocoTrack_Move(self);
        return 1;
    }
    return 0;
}

} // namespace

#define AICOMBATLOCO_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AICOMBATLOCO_REVERSED("CombatLocoTrack::End", 0x00625390, &CombatLocoTrack_End);
AICOMBATLOCO_REVERSED("CombatLocoTrack::FindOpponent", 0x006253D0, &CombatLocoTrack_FindOpponentImpl);
AICOMBATLOCO_REVERSED("CombatLocoTrack::JoinSlots", 0x00625430, &CombatLocoTrack_JoinSlotsImpl);
AICOMBATLOCO_REVERSED("CombatLocoTrack::UpdateRange", 0x00625540, &CombatLocoTrack_UpdateRangeImpl);
AICOMBATLOCO_REVERSED("CombatLocoTrack::CanShootFrom", 0x00625620, &CombatLocoTrack_CanShootFromImpl);
AICOMBATLOCO_REVERSED("CombatLocoTrack::Approach", 0x006256A0, &CombatLocoTrack_ApproachImpl);
AICOMBATLOCO_REVERSED("CombatLocoTrack::Wait", 0x006258A0, &CombatLocoTrack_WaitImpl);
AICOMBATLOCO_REVERSED("CombatLocoTrack::CircleStep", 0x00625920, &CombatLocoTrack_CircleStepImpl);
AICOMBATLOCO_REVERSED("CombatLocoTrack::Move", 0x00625AD0, &CombatLocoTrack_MoveImpl);
AICOMBATLOCO_REVERSED("CombatLocoTrack::TryGoal", 0x0062C650, &CombatLocoTrack_TryGoalImpl);
AICOMBATLOCO_REVERSED("CombatLocoTrack::UpdateTimers", 0x0062C6A0, &CombatLocoTrack_UpdateTimersImpl);
AICOMBATLOCO_REVERSED("CombatLocoTrack::Reposition", 0x0062C8A0, &CombatLocoTrack_RepositionImpl);
AICOMBATLOCO_REVERSED("CombatLocoTrack::Think", 0x0062E280, &CombatLocoTrack_ThinkImpl);
AICOMBATLOCO_REVERSED("CombatLocoTrack::Start", 0x0062EB40, &CombatLocoTrack_Start);
AICOMBATLOCO_REVERSED("CombatLocoTrack::Update", 0x0062EBA0, &CombatLocoTrack_Update);
