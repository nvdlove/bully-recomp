// The action tree's SeekEntityPositionTrack (a ped walking to an entity's node or door,
// then turning to it and snapping there), and the Wander / RideAround tracks' Start and
// Wander's Update. 0x62A880..0x62BE8A (their helpers of 0x61D4A0..0x61EDA0 are in
// TracksAIReward.cpp and the other chunk-30 files).
// A seek-entity-position track: +0x18 keep the speed, +0x19 snap there, +0x1C the kind
// (3 get on a vehicle, 0xB the ped itself, 0xE its vehicle), +0x20 the speed, +0x24 the
// radius, +0x28 the near distance, +0x2C the facing tolerance (radians), +0x30 / +0x38
// the node, +0x34 the entity, +0x3C the owner, +0x40 arrived, +0x41 near, +0x44 the speed
// kept.
// A wander track: +0x18 the owner, +0x1C / +0x20 the last / next node, +0x24 the node
// reach, +0x28 tries, +0x2C the speed.

#include <cstdint>

#include "game/ActionTracksCommon.h"
#include "reversed.h"

using namespace actc;

namespace {

GAME_VAR(uint8_t, playerSeeking, 0x00C2B10A);
GAME_VAR(int32_t, seekUpdatesAtOnce, 0x00B0C29C);
GAME_VAR(float, combatStepRate, 0x00B0C0B0);

GAME_FN(void, __cdecl, Entity_RegisterReference, 0x004657D0, uint8_t* entity, re::Ptr32<uint8_t>* ref);
GAME_FN(uint8_t*, __thiscall, Ped_FindByType, 0x00477C00, uint8_t* ped, int32_t type, int32_t* extra);
GAME_FN(void, __thiscall, Scanner_SetTimeSince, 0x0048C190, uint8_t* scanner, int32_t ms);
GAME_FN(uint8_t*, __thiscall, Queue_GetCurrent, 0x00470340, uint8_t* queue);
GAME_FN(uint8_t*, __thiscall, Shop_GetClerk, 0x0077E8D0, uint8_t* objective);
GAME_FN(void, __thiscall, Ped_SetTarget, 0x00497E70, uint8_t* ped, uint8_t* target, int kind, const float* offset);
GAME_FN(void, __thiscall, NamedRef_Set, 0x00471300, uint8_t* ref, uint8_t* entity, int value);
GAME_FN(float, __cdecl, SeekEntityPosition_MoveTime, 0x0061D4F0, uint8_t* entity, const float* target, float least);
GAME_FN(uint8_t, __cdecl, NearlyEqual, 0x00432A60, float a, float b, float epsilon);
GAME_FN(void, __thiscall, Entity_SetPosition, 0x0041C4E0, uint8_t* entity, float x, float y, float z);
GAME_FN(float, __cdecl, SafeAcos, 0x00413D60, float x);
GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_FN(uint8_t, __thiscall, Steering_GoTo, 0x004A5000, uint8_t* steering, float x, float y, float z, uint32_t flags,
        float speed, float radius, uint8_t* entity, int flag);
GAME_FN(uint8_t, __thiscall, Ped_EveryNthFrame, 0x00465A60, uint8_t* ped, uint32_t n);
// the tracks' own, by address (so their modes apply)
GAME_FN(void, __thiscall, SeekEntityPositionTrack_GetGoal, 0x0061D690, uint8_t* self, uint8_t* out);
GAME_FN(uint8_t, __thiscall, CirculateTrack_FindStart, 0x0061EDA0, uint8_t* self);

// the goal transform (an NiTransform: the rotation's rows, +0x24 the position, +0x30
// the scale; what the goal leaves unset is the stack's)
struct Goal {
    float rot[9];
    V3 pos;
    float scale;
};
static_assert(sizeof(Goal) == 0x34);

// near the goal (within +0x28): the ped's +0x146C (on arriving) and +0x1514 flags
void UpdateNear(uint8_t* self, uint8_t* ped, float distance) {
    if (At<float>(self, 0x28) > distance) {
        if (self[0x41] == 0) {
            At<int32_t>(ped, 0x146C) = 1;
        }
        At<int32_t>(ped, 0x1514) = 1;
        self[0x41] = 1;
    } else {
        if (self[0x41] != 0) {
            At<int32_t>(ped, 0x146C) = 0;
            At<int32_t>(ped, 0x1514) = 0;
        }
        self[0x41] = 0;
    }
}
float Distance3(const Goal& goal, uint8_t* ped) {
    const float* p = Pos(ped);
    V3 d = {goal.pos.x - p[0], goal.pos.y - p[1], goal.pos.z - p[2]};
    return SqrtAbs(d.x * d.x + d.y * d.y + d.z * d.z);
}

// 0x62A880 (thiscall): SeekEntityPositionTrack::Start: the entity (+0x34, found by kind
// from the ped) referenced; the player involved marks the player seeking; without one a
// shop objective (0x1F) targets its clerk instead (0); else near-ness checked and (when
// the game says so) a first update
uint8_t __fastcall SeekEntityPositionTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x3C) = owner;
    uint8_t* ped = OwnerPed(owner);
    At<float>(self, 0x44) = -1.0f;
    At<int32_t>(self, 0x38) = At<int32_t>(self, 0x30);
    self[0x41] = 0;
    self[0x40] = 0;
    uint8_t* entity = Ped_FindByType(ped, At<int32_t>(self, 0x1C), &At<int32_t>(self, 0x38));
    re::Ptr32<uint8_t>* ref = &At<uint8_t*>(self, 0x34);
    if (entity != *ref) {
        if (*ref) {
            Entity_CleanUpOldReference(*ref, ref);
        }
        *ref = entity;
        if (entity) {
            Entity_RegisterReference(entity, ref);
        }
    }
    if (At<int32_t>(self, 0x30) != 0) {
        At<int32_t>(self, 0x38) = At<int32_t>(self, 0x30);
    }
    uint8_t* player = playerPed;
    if (ped == player) {
        Scanner_SetTimeSince(player + 0x3BC, 5000);
        playerSeeking = 1;
    } else if (*ref == player) {
        playerSeeking = 1;
    }
    if (!*ref) {
        uint8_t* current = Queue_GetCurrent(ped + 0x5F0);
        if (current && At<int32_t>(current, 8) == 0x1F) {
            uint8_t* clerk = Shop_GetClerk(current);
            if (clerk) {
                V3 none = {0.0f, 0.0f, 0.0f};
                Ped_SetTarget(ped, clerk, 3, &none.x);
                NamedRef_Set(ped + 0x438, clerk, 0);
            }
        }
        return 0;
    }
    Goal goal;
    goal.scale = 1.0f;
    SeekEntityPositionTrack_GetGoal(self, reinterpret_cast<uint8_t*>(&goal));
    UpdateNear(self, ped, Distance3(goal, ped));
    int32_t atOnce = seekUpdatesAtOnce;
    At<float>(self, 0x44) = At<float*>(ped, 0x2E0)[7];
    if (atOnce != 0) {
        vc::Call<uint8_t>(self, acttree::kTrackUpdate, combatStepRate * timeStep);
    }
    return 1;
}

// 0x62AAA0 (thiscall): SeekEntityPositionTrack::Update: to the goal (flags 0x1011C,
// speed +0x20 (doubled for a ped in a pair), the ped's own when faster, radius +0x24,
// following the entity), slowed to its move animation's time; near the node (or the
// ped's vehicle): facing the goal's frame; stopped short (or at no speed) it stands
// facing it and arrives, else it walks straight at it; not facing within +0x2C it keeps
// going. Arrived: stopped, snapped there with +0x19.
uint8_t __fastcall SeekEntityPositionTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* entity = At<uint8_t*>(self, 0x34);
    int32_t kind = At<int32_t>(self, 0x1C);
    if (!entity) {
        if (kind != 0xB && kind != 0xE) {
            return 0;
        }
    } else if ((entity[0x108] & 7) != 6 && At<uint8_t*>(entity, 0x18) == nullptr &&
               (!At<uint8_t*>(entity, 0x114) || At<int32_t>(At<uint8_t*>(entity, 0x114), 0x94) == 0)) {
        if (kind != 3 || At<int32_t>(entity, 0x46C) != 0) {
            return 0;
        }
    }
    float speed = At<float>(self, 0x20);
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x3C));
    float* move = At<float*>(ped, 0x2E0);
    float current = move[7];
    if (At<int32_t>(ped, 0x1310) != 0xD && At<int32_t>(ped, 0xBB0) != 0 && At<int32_t>(ped, 0xBB4) != 0 &&
        At<int32_t>(ped, 0xC30) <= 1) {
        speed = speed + speed;
    }
    if (current >= 0.0f && !(speed > current)) {
        speed = current;
    }
    float radius = At<float>(self, 0x24);
    Goal goal;
    goal.scale = 1.0f;
    SeekEntityPositionTrack_GetGoal(self, reinterpret_cast<uint8_t*>(&goal));
    uint8_t going = Steering_GoTo(ped + 0x654, goal.pos.x, goal.pos.y, goal.pos.z, 0x1011C, speed, radius,
                                  At<uint8_t*>(self, 0x34), 0);
    if (going) {
        if (At<int32_t>(ped, 0x1310) == 0xD) {
            if (!(current < speed)) {
                speed = current;
            }
            move[7] = speed;
        }
        At<float>(self, 0x44) = move[7];
        float t = SeekEntityPosition_MoveTime(ped, &goal.pos.x, speed);
        if (t >= 0.0f && At<float>(self, 0x44) > t) {
            move[7] = t;
        }
    }
    const float* p = Pos(ped);
    V3 d = {goal.pos.x - p[0], goal.pos.y - p[1], goal.pos.z - p[2]};
    float distance = SqrtAbs(d.x * d.x + d.y * d.y + d.z * d.z);
    UpdateNear(self, ped, distance);
    bool vehicleKind = At<int32_t>(self, 0x1C) == 0xE;
    bool face = false;
    V3 facing = {0.0f, 0.0f, 0.0f};
    if (self[0x41] != 0) {
        if (vehicleKind) {
            facing = {goal.rot[0], goal.rot[3], goal.rot[6]};
            face = true;
        } else if (At<int32_t>(self, 0x38) != 0) {
            facing = {goal.rot[1], goal.rot[4], goal.rot[7]};
            face = true;
        }
    }
    if (face) {
        move[3] = facing.x;
        move[4] = facing.y;
        if (!going) {
            if (!(distance > 0.02f) || NearlyEqual(At<float>(self, 0x20), 0.0f, 1e-5f)) {
                move[0] = facing.x;
                move[1] = facing.y;
                self[0x40] = 1;
            } else {
                d.z = 0.0f;
                Vector_Normalise(&d);
                move[0] = d.x;
                move[1] = d.y;
                move[7] = speed;
                float t = SeekEntityPosition_MoveTime(ped, &goal.pos.x, speed);
                if (t >= 0.0f && At<float>(self, 0x44) > t) {
                    move[7] = t;
                }
            }
        }
        float tolerance = At<float>(self, 0x2C);
        if (!going && tolerance >= 0.0f) {
            V3 f;
            Placeable_GetForward(ped, &f);
            Vector_Normalise(&f);
            float off = SafeAcos(f.y * facing.y + f.x * facing.x + f.z * facing.z);
            if (tolerance < off || off != off) {
                going = 1;
            } else {
                self[0x40] = 1;
            }
        }
    }
    if (self[0x40] != 0) {
        move[7] = 0.0f;
        if (self[0x19] != 0) {
            Entity_SetPosition(ped, goal.pos.x, goal.pos.y, goal.pos.z);
        }
    }
    if (!going && self[0x18] != 0 && At<float>(self, 0x44) >= 0.0f) {
        move[7] = At<float>(self, 0x44);
    }
    return going;
}

// 0x62BD20 (thiscall): WanderTrack::Update: without a node, every 8th frame one's looked
// for (counting tries); a node (with a stop flag, 0x60, the ped's vf24 and done) steered
// to (vf8 the point, vf6 the steering, vf5 the next node)
uint8_t __fastcall WanderTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    if (At<uint8_t*>(self, 0x20) == nullptr) {
        if (!Ped_EveryNthFrame(ped, 8)) {
            return 1;
        }
        if (!CirculateTrack_FindStart(self) && self[0x28] != 0) {
            self[0x28] = uint8_t(self[0x28] + 1);
            return 1;
        }
    }
    uint8_t* node = At<uint8_t*>(self, 0x20);
    if (!node) {
        return 0;
    }
    if (node[0x14] & 0x60) {
        vc::Call<void>(ped, 0x60);
        return 0;
    }
    V3 at = {0.0f, 0.0f, 0.0f};
    if (!vc::Call<uint8_t>(self, 0x20, &at)) {
        return 0;
    }
    if (reinterpret_cast<uint8_t(__thiscall*)(uint8_t*, float, float, float)>(
            static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(self))[6]))(self, at.x, at.y, at.z)) {   // (vf6 takes the floats)
        return 1;
    }
    return vc::Call<uint8_t>(self, 0x14) ? 1 : 0;
}

// 0x62BDF0 (thiscall): WanderTrack::Start: the speed from a wander objective (0xA; else
// 0.3), a first node looked for
uint8_t __fastcall WanderTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* current = Queue_GetCurrent(OwnerPed(owner) + 0x5F0);
    if (current && At<int32_t>(current, 8) == 0xA) {
        At<float>(self, 0x2C) = At<float>(current, 0x10);
    } else {
        At<float>(self, 0x2C) = 0.3f;
    }
    At<int32_t>(self, 0x20) = 0;
    At<int32_t>(self, 0x1C) = 0;
    self[0x28] = 0;
    At<float>(self, 0x24) = 3.0f;
    At<uint8_t*>(self, 0x18) = owner;
    CirculateTrack_FindStart(self);
    At<float>(self, 0x24) = 3.0f;
    return 1;
}

// 0x62BE50 (thiscall): RideAroundTrack::Start: the ped's +0x534 cleared, a first node
// looked for (reach 3, then 2.5)
uint8_t __fastcall RideAroundTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<int32_t>(OwnerPed(owner), 0x534) = 0;
    At<float>(self, 0x24) = 3.0f;
    At<int32_t>(self, 0x20) = 0;
    At<int32_t>(self, 0x1C) = 0;
    self[0x28] = 0;
    At<uint8_t*>(self, 0x18) = owner;
    CirculateTrack_FindStart(self);
    At<float>(self, 0x24) = 2.5f;
    return 1;
}

} // namespace

#define AISEEKTRK_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AISEEKTRK_REVERSED("SeekEntityPositionTrack::Start", 0x0062A880, &SeekEntityPositionTrack_Start);
AISEEKTRK_REVERSED("SeekEntityPositionTrack::Update", 0x0062AAA0, &SeekEntityPositionTrack_Update);
AISEEKTRK_REVERSED("WanderTrack::Update", 0x0062BD20, &WanderTrack_Update);
AISEEKTRK_REVERSED("WanderTrack::Start", 0x0062BDF0, &WanderTrack_Start);
AISEEKTRK_REVERSED("RideAroundTrack::Start", 0x0062BE50, &RideAroundTrack_Start);
