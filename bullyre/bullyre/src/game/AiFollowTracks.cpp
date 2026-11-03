// The action tree's AI tracks that follow or chase an entity, on foot or on a bike:
// FollowFocus (the ped's threat, or an objective's target), FollowFocusOnBike,
// FollowTargetOnBike, AttackOnBike and StealBike. 0x622B40..0x6236CD.
// The ped: +0x1C8 its targeting, +0x2E0 its move state (+0 the direction, +0xC the
// facing, +0x1C the speed), +0x3BC its scanner (the threat), +0x538 its target,
// +0x5F0 its objectives, +0x654 its steering, +0x1554 the vehicle it's on (+0x124 the
// move speed). Stats: 0x19 the bike's attack speed, 0x1C the matching distance, 0x24 /
// 0x25 the least / most follow speed (percent).

#include <cstdint>

#include "game/ActionTracksCommon.h"

using namespace actc;

namespace {

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(uint8_t*, __thiscall, IdleObjective_Construct, 0x0046FC60, void* self, uint8_t* ped);
GAME_FN(void, __thiscall, Queue_SetDefault, 0x00470300, uint8_t* queue, uint8_t* objective);
GAME_FN(uint8_t*, __thiscall, Queue_GetCurrent, 0x00470340, uint8_t* queue);
GAME_FN(uint8_t*, __thiscall, Scanner_GetThreat, 0x0048C070, uint8_t* scanner);
GAME_FN(int32_t, __thiscall, Ped_GetStat, 0x00477060, uint8_t* ped, int32_t stat);
GAME_FN(uint8_t*, __thiscall, Entity_GetMatrix, 0x008B6120, uint8_t* entity);
GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_FN(void, __thiscall, Targeting_SetTarget, 0x004D79E0, uint8_t* targeting, uint8_t* target);
GAME_FN(uint8_t, __thiscall, Steering_GoTo, 0x004A5000, uint8_t* steering, float x, float y, float z, uint32_t flags,
        float speed, float radius, uint8_t* entity, int flag);

uint8_t* Steering(uint8_t* ped) {
    return ped + 0x654;
}
float* Move(uint8_t* ped) {
    return At<float*>(ped, 0x2E0);
}
// a stat in percent as a fraction
float StatFraction(uint8_t* ped, int32_t stat) {
    return float(Ped_GetStat(ped, stat)) / 100.0f;
}
// the flat distance between two positions (a's minus b's)
float FlatDistance(const float* a, const float* b, V3* d) {
    VectorSub(d, a, b);
    return SqrtAbs(d->x * d->x + d->y * d->y);
}
// no threat: an idle objective as the default of the ped's queue (a failed allocation
// sets null)
void SetIdleDefault(uint8_t* ped) {
    void* mem = Malloc(0x10);
    uint8_t* idle = mem ? IdleObjective_Construct(mem, ped) : nullptr;
    Queue_SetDefault(ped + 0x5F0, idle);
}
// the entity's move speed (its vehicle's when it's on one)
const float* MoveSpeedOf(uint8_t* entity) {
    uint8_t* vehicle = At<uint8_t*>(entity, 0x1554);
    return &At<float>(vehicle ? vehicle : entity, 0x124);
}

// the speed a bike follows at: half the other's inside `range`; its most (`most`) beyond
// the matching distance `near`; else the other's speed kept between the least and most
float BikeFollowSpeed(float dist, float range, float near, float other, float least, float most) {
    if (range > dist) {
        return other * 0.5f;
    }
    if (near < dist) {
        return most;
    }
    float v = least < other ? other : least;
    return most < v ? most : v;
}

// 0x622B40 (thiscall): FollowFocusTrack::Update: the ped follows its threat (or the
// current type-0x11 objective's target; without either it gets an idle default and
// stops): beyond the range +0x18 (+0x1C more when standing) it walks there (flags 4,
// 1.1, or the other's speed while inside twice the range); inside it faces it and stops
uint8_t __fastcall FollowFocusTrack_Update(uint8_t* self, re::Edx, float) {
    float range = At<float>(self, 0x18);
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x20));
    if (0.0f == Move(ped)[7]) {
        range = At<float>(self, 0x1C) + range;
    }
    uint8_t* focus = Scanner_GetThreat(ped + 0x3BC);
    if (!focus) {
        uint8_t* current = Queue_GetCurrent(ped + 0x5F0);
        if (!current || At<int32_t>(current, 8) != 0x11) {
            return 0;
        }
        focus = At<uint8_t*>(current, 0x10);
        if (!focus) {
            SetIdleDefault(ped);
            return 0;
        }
    }
    V3 d;
    float dist = FlatDistance(Pos(focus), Pos(ped), &d);
    if (range > dist) {
        Vector_Normalise(&d);
        float* move = Move(ped);
        move[0] = d.x;
        move[1] = d.y;
        move[2] = d.z;
        move = Move(ped);
        move[3] = d.x;
        move[4] = d.y;
        move[5] = d.z;
        Move(ped)[7] = 0.0f;
        return 1;
    }
    float speed = 1.1f;
    if (range + range > dist) {
        float other = Move(focus)[7];
        if (other > 0.0f) {
            speed = other;
        }
    }
    const float* to = Pos(focus);
    Steering_GoTo(Steering(ped), to[0], to[1], to[2], 4, speed, range, nullptr, 0);
    return 1;
}

// 0x622D40 (thiscall): FollowFocusOnBikeTrack::Update: the ped rides after its threat
// (flags 0x14, radius 0.5) at BikeFollowSpeed (range +0x1C); no threat: the idle default
// for a type-0x11 objective, and stops
uint8_t __fastcall FollowFocusOnBikeTrack_Update(uint8_t* self, re::Edx, float) {
    float range = At<float>(self, 0x1C);
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x24));
    float near = float(Ped_GetStat(ped, 0x1C));
    float most = StatFraction(ped, 0x25);
    float least = StatFraction(ped, 0x24);
    uint8_t* threat = Scanner_GetThreat(ped + 0x3BC);
    if (!threat) {
        uint8_t* current = Queue_GetCurrent(ped + 0x5F0);
        if (current && At<int32_t>(current, 8) == 0x11) {
            SetIdleDefault(ped);
        }
        return 0;
    }
    V3 d;
    float dist = FlatDistance(Pos(threat), Pos(ped), &d);
    float speed = BikeFollowSpeed(dist, range, near, Move(threat)[7], least, most);
    const float* to = Pos(threat);
    Steering_GoTo(Steering(ped), to[0], to[1], to[2], 0x14, speed, 0.5f, nullptr, 0);
    return 1;
}

// 0x622F40 (thiscall): FollowTargetOnBikeTrack::Start: the ped's bike (if any) gets
// steering mode 0x1C (+0x2DA)
uint8_t __fastcall FollowTargetOnBikeTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    uint8_t* vehicle = At<uint8_t*>(OwnerPed(owner), 0x1554);
    if (vehicle) {
        vehicle[0x2DA] = 0x1C;
    }
    return 1;
}

// 0x622F60 (thiscall): FollowTargetOnBikeTrack::Update: FollowFocusOnBike's riding after
// the ped's target (a ped), the most speed at least +0x20
uint8_t __fastcall FollowTargetOnBikeTrack_Update(uint8_t* self, re::Edx, float) {
    float range = At<float>(self, 0x1C);
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x24));
    float near = float(Ped_GetStat(ped, 0x1C));
    float mostStat = StatFraction(ped, 0x25);
    float most = At<float>(self, 0x20);
    if (most < mostStat) {
        most = mostStat;
    }
    float least = StatFraction(ped, 0x24);
    uint8_t* target = At<uint8_t*>(ped, 0x538);
    if (!target || !IsPedEntity(target)) {
        return 0;
    }
    V3 d;
    float dist = FlatDistance(Pos(target), Pos(ped), &d);
    float speed = BikeFollowSpeed(dist, range, near, Move(target)[7], least, most);
    const float* to = Pos(target);
    Steering_GoTo(Steering(ped), to[0], to[1], to[2], 0x14, speed, 0.5f, nullptr, 0);
    return 1;
}

// 0x623140 (thiscall): AttackOnBikeTrack::Update: the ped rides at its target (a ped)
// from the side it's on (the target's right * 1.05 either way) and where it will be (3
// of its move speed on), at least the stat 0x19 speed (flags 0x1C108, radius 0.4). When
// it can't: it faces the way the target (or its bike) faces, turned round when both are
// still and the target is behind. Near (< 80 squared) a still target slows it
// ((d² + 20) / 100); else within 500 its speed is capped at the target's + 0.3 + a bit
uint8_t __fastcall AttackOnBikeTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    uint8_t* target = At<uint8_t*>(ped, 0x538);
    if (!target || !IsPedEntity(target)) {
        return 0;
    }
    const float* tp = Pos(target);
    V3 to = {tp[0], tp[1], tp[2]};
    uint8_t* m = Entity_GetMatrix(target);
    V3 side = {At<float>(m, 0), At<float>(m, 4), At<float>(m, 8)};
    side.x = side.x * 1.05f;
    side.y = side.y * 1.05f;
    side.z = side.z * 1.05f;
    V3 d;
    VectorSub(&d, &to.x, Pos(ped));
    float distSq = d.x * d.x + d.y * d.y + d.z * d.z;
    Vector_Normalise(&d);
    float dot = side.x * d.x + side.y * d.y + side.z * d.z;
    if (dot > 0.0f) {
        to.x = to.x - side.x;
        to.y = to.y - side.y;
        to.z = to.z - side.z;
    } else {
        to.x = side.x + to.x;
        to.y = side.y + to.y;
        to.z = side.z + to.z;
    }
    const float* v = MoveSpeedOf(target);
    to.x = v[0] * 3.0f + to.x;
    to.y = to.y + v[1] * 3.0f;
    to.z = to.z + v[2] * 3.0f;
    v = MoveSpeedOf(target);
    float speedSq = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    Targeting_SetTarget(At<uint8_t*>(ped, 0x1C8), target);
    float other = Move(target)[7];
    float* move = Move(ped);
    float least = StatFraction(ped, 0x19);
    float speed = other < least ? least : other;
    if (!Steering_GoTo(Steering(ped), to.x, to.y, to.z, 0x1C108, speed, 0.4f, nullptr, 0)) {
        uint8_t* vehicle = At<uint8_t*>(target, 0x1554);
        V3 f;
        V3* fwd = Placeable_GetForward(vehicle ? vehicle : target, &f);
        move[3] = fwd->x;
        move[4] = fwd->y;
        move[5] = fwd->z;
        move[0] = move[3];
        move[1] = move[4];
        move[2] = move[5];
        if (speedSq < 0.01f) {
            float behind = move[0] * d.x + move[1] * d.y + d.z * move[2];
            if (behind < -0.5f) {
                move[0] = -move[0];
                move[1] = -move[1];
                move[2] = -move[2];
            }
        }
    }
    if (distSq < 80.0f && speedSq < 0.01f) {
        move[7] = (distSq + 20.0f) / 100.0f * move[7];
        return 1;
    }
    if (500.0f > distSq) {
        float cap = 0.3f + Move(target)[7] + distSq / 500.0f * 0.3f;
        if (move[7] > cap) {
            move[7] = cap;
        }
    }
    return 1;
}

// 0x623580 (thiscall): StealBikeTrack::Update: the ped runs to its threat's side (the
// side it's on; flags 0xC, speed 2, radius 0.6), targeting it; still going
uint8_t __fastcall StealBikeTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    uint8_t* threat = Scanner_GetThreat(ped + 0x3BC);
    const float* tp = Pos(threat);
    V3 to = {tp[0], tp[1], tp[2]};
    uint8_t* m = Entity_GetMatrix(threat);
    V3 side = {At<float>(m, 0), At<float>(m, 4), At<float>(m, 8)};
    V3 d;
    VectorSub(&d, Pos(threat), Pos(ped));
    Vector_Normalise(&d);
    if (side.x * d.x + side.y * d.y + side.z * d.z > 0.0f) {
        to.x = to.x - side.x;
        to.y = to.y - side.y;
        to.z = to.z - side.z;
    } else {
        to.x = side.x + to.x;
        to.y = side.y + to.y;
        to.z = side.z + to.z;
    }
    Targeting_SetTarget(At<uint8_t*>(ped, 0x1C8), threat);
    return Steering_GoTo(Steering(ped), to.x, to.y, to.z, 0xC, 2.0f, 0.6f, nullptr, 0) != 0 ? 1 : 0;
}

} // namespace

#define AIFOLLOWTRK_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AIFOLLOWTRK_REVERSED("FollowFocusTrack::Update", 0x00622B40, &FollowFocusTrack_Update);
AIFOLLOWTRK_REVERSED("FollowFocusOnBikeTrack::Update", 0x00622D40, &FollowFocusOnBikeTrack_Update);
AIFOLLOWTRK_REVERSED("FollowTargetOnBikeTrack::Start", 0x00622F40, &FollowTargetOnBikeTrack_Start);
AIFOLLOWTRK_REVERSED("FollowTargetOnBikeTrack::Update", 0x00622F60, &FollowTargetOnBikeTrack_Update);
AIFOLLOWTRK_REVERSED("AttackOnBikeTrack::Update", 0x00623140, &AttackOnBikeTrack_Update);
AIFOLLOWTRK_REVERSED("StealBikeTrack::Update", 0x00623580, &StealBikeTrack_Update);
