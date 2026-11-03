// A ped's obstacle avoidance, continued (0x5DFB80..0x5E0B20, 0x5E28B0; PedAvoidance.cpp
// has the rest and the global state at 0xD09518..0xD09640): the per-frame pass over the
// peds near the walking ped and the vehicles around it, each one's path swept against
// the ped's, and the entry the navigator calls. The record (the navigator's +0x438):
// +0 the ped, +4 an entity to ignore, +8 the nearest ped (registered), +0x18 the speed,
// +0x6D flags (1 blocked, 2 a ped avoided, 4 a vehicle avoided).
//
// The state's w components the game fills from stack it never sets are zero here.

#include <cmath>
#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace {

struct AvVec {
    float x, y, z;
};

template <typename T>
re::Field<T>& avAt(const void* base, int offset) {
    return *reinterpret_cast<re::Field<T>*>(reinterpret_cast<uintptr_t>(base) + offset);
}
template <typename R>
R avVCall(void* self, int offset) {
    using Fn = R(__thiscall*)(void*);
    return re::GameFnAt<Fn>(*static_cast<re::Ptr32<uint8_t>*>(self) + offset)(self);
}

// The state (see PedAvoidance.cpp)
inline float* const avPos = reinterpret_cast<float*>(0x00D09520);        // the ped (4)
inline float* const avDir = reinterpret_cast<float*>(0x00D09530);        // its move direction (4)
inline float* const avTarget = reinterpret_cast<float*>(0x00D09540);     // where it's going (4)
inline float* const avToTarget = reinterpret_cast<float*>(0x00D09550);   // (4, z 0)
GAME_VAR(float, avSpeed, 0x00D09560);          // its speed (per second: 50 frames)
GAME_VAR(float, avTimeToTarget, 0x00D09564);
GAME_VAR(float, avLeftCost, 0x00D09568);
GAME_VAR(float, avRightCost, 0x00D0956C);
GAME_VAR(float, avRadius, 0x00D09570);
GAME_VAR(float, avHeading, 0x00D09574);
inline float* const avObstaclePos = reinterpret_cast<float*>(0x00D09578);   // (4)
inline float* const avObstacleDir = reinterpret_cast<float*>(0x00D09588);   // obstacle - ped (4)
inline float* const avObstacleGoal = reinterpret_cast<float*>(0x00D09598);  // where it's going (4)
inline float* const avObstacleMove = reinterpret_cast<float*>(0x00D095A8);  // (4, z 0)
GAME_VAR(float, avObstacleSpeed, 0x00D095B8);
GAME_VAR(float, avObstacleRadius, 0x00D095BC);
inline float* const avMove = reinterpret_cast<float*>(0x00D095C0);          // (4)
inline float* const avSeg1 = reinterpret_cast<float*>(0x00D095D0);          // a segment (8)
GAME_VAR(float, avLookTime, 0x00D09610);
GAME_VAR(float, avStep, 0x00D09614);
GAME_VAR(float, avMaxAngle, 0x00D09618);
GAME_VAR(float, avMinAngle, 0x00D0961C);
GAME_VAR(float, avNearestDist, 0x00D09628);
GAME_ARRAY(uint8_t*, avObstacles, 0x00D0962C);
GAME_VAR(int32_t, avObstacleCount, 0x00D09640);

GAME_VAR(uint8_t*, av_player, 0x00C1AEA8);
GAME_VAR(int32_t, av_poiPointModel, 0x00BF42DC);
GAME_VAR(uint8_t, av_chapter, 0x00A147E0);
GAME_ARRAY(uint8_t*, av_modelInfos, 0x00C67738);
GAME_VAR(uint8_t*, av_vehiclePool, 0x00C0F5F4);

GAME_FN(float, __cdecl, AV_Heading, 0x00433310, const void* v);
GAME_FN(float, __cdecl, AV_Min, 0x00414470, float a, float b);
GAME_FN(float, __thiscall, AV_Normalise2, 0x004294A0, float* v);
GAME_FN(float*, __thiscall, AV_GetForward, 0x004154A0, void* placeable, float* out);
GAME_FN(void*, __cdecl, AV_GetColModel, 0x0051AF50, void* entity);
GAME_FN(void, __thiscall, AV_EntityRefSet, 0x00478550, void* ref, void* entity);
GAME_FN(int, __thiscall, AV_Vehicle_4CE560, 0x004CE560, void* self, int);
GAME_FN(float, __thiscall, AV_Seg_FlatDistance, 0x005E6E60, const float* seg, const float* point);
GAME_FN(float*, __thiscall, AV_Seg_Normal2D, 0x005E7220, const float* seg, float* out);
GAME_FN(uint8_t, __thiscall, AV_Navigator_GetTarget, 0x005E3110, void* navigator, float* out);
GAME_FN(float, __stdcall, AV_AvoidCost, 0x005DC5A0, float speed, float angle, float dist);
GAME_FN(void, __thiscall, AV_AvoidCircle, 0x005DC670, void* self, const float* pos);
GAME_FN(void, __thiscall, AV_AvoidNearLines, 0x005DC990, void* self);
GAME_FN(void, __thiscall, AV_ChooseSide, 0x005DD6B0, void* self, int flag);
GAME_FN(void, __thiscall, AV_5DE2A0, 0x005DE2A0, void* self);
GAME_FN(uint8_t, __thiscall, AV_5DE7C0, 0x005DE7C0, void* self);
GAME_FN(uint8_t, __thiscall, AV_5DEAF0, 0x005DEAF0, void* self, void* entity);
GAME_FN(void, __thiscall, AV_5DF660, 0x005DF660, void* self);
GAME_FN(uint8_t, __thiscall, AV_SweepAgainst, 0x005DFB80, void* self, void* entity);
GAME_FN(void, __thiscall, AV_ConsiderPed, 0x005DFEC0, void* self, uint8_t* ped);
GAME_FN(void, __thiscall, AV_ConsiderVehicle, 0x005E01F0, void* self, uint8_t* vehicle);
GAME_FN(void, __thiscall, AV_Avoid, 0x005E0340, void* self, float* dir, const float* target, float speed, int flag);

const float* EntityPos(const uint8_t* entity) {
    const uint8_t* matrix = avAt<const uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<const float*>(matrix + 0x30) : reinterpret_cast<const float*>(entity + 4);
}
float SqrtAbs(double v) {
    return float(std::sqrt(double(std::fabs(float(v)))));
}
// the flat speed of a physical's move speed (+0x124), per second
float FlatSpeed(const uint8_t* physical) {
    float x = avAt<float>(physical, 0x124);
    float y = avAt<float>(physical, 0x128);
    return SqrtAbs(double(x) * x + double(y) * y);
}

// 0x5DFB80 (thiscall): the obstacle's move (avObstacleMove, at avObstacleSpeed) against
// the ped's, over the time until the ped's target (at most 10): when the relative move
// passes within both radii of the obstacle, its cost goes to the side it's on
uint8_t __fastcall SweepAgainst(void* self, re::Edx, void* entity) {
    float time;
    float a = std::fabs(float(avObstacleSpeed - 0.0));
    if (a < double(0.05f) || a == double(0.05f)) {
        time = float(3.4028234663852886e+38);
    } else {
        float length = SqrtAbs(double(avObstacleMove[0]) * avObstacleMove[0]
                               + double(avObstacleMove[1]) * avObstacleMove[1]);
        time = float(double(length) / avObstacleSpeed);
    }
    float limited = time > avTimeToTarget ? avTimeToTarget : time;
    avLookTime = limited < 10.0f ? limited : 10.0f;
    AvVec mine = {avToTarget[0], avToTarget[1], avToTarget[2]};
    AvVec theirs = {avObstacleMove[0], avObstacleMove[1], avObstacleMove[2]};
    if (avTimeToTarget != 0.0f) {
        float k = float(double(avLookTime) / avTimeToTarget);
        mine = {mine.x * k, mine.y * k, k * mine.z};
    }
    if (time != 0.0f) {
        float k = float(double(avLookTime) / time);
        theirs = {theirs.x * k, theirs.y * k, k * theirs.z};
    }
    AvVec move = {mine.x - theirs.x, mine.y - theirs.y, mine.z - theirs.z};
    avMove[0] = move.x;
    avMove[1] = move.y;
    avMove[2] = move.z;
    avMove[3] = 0.0f;
    float px = avPos[0];
    float py = avPos[1];
    float pz = avPos[2];
    avSeg1[0] = px;
    avSeg1[1] = py;
    avSeg1[2] = pz;
    avSeg1[3] = 0.0f;
    avSeg1[4] = px + move.x;
    avSeg1[5] = py + move.y;
    avSeg1[6] = pz + move.z;
    avSeg1[7] = 0.0f;
    float dist = AV_Seg_FlatDistance(avSeg1, avObstaclePos);
    float reach = avObstacleRadius + avRadius;
    if (reach < dist) {
        return 0;
    }
    if (!AV_5DEAF0(self, entity)) {
        return 0;
    }
    float normal[3];
    AV_Seg_Normal2D(avSeg1, normal);
    AV_Normalise2(normal);
    float side = float(double(normal[1]) * avObstacleDir[1] + double(normal[0]) * avObstacleDir[0]
                       + double(normal[2]) * avObstacleDir[2]);
    float cost = AV_AvoidCost(avStep, side, avObstacleSpeed);
    if (side > 0.0f) {
        avRightCost = cost + avRightCost;
    } else {
        avLeftCost = cost + avLeftCost;
    }
    return 1;
}

// 0x5DFEC0 (thiscall): a nearby ped: where it's heading (the player: 3 ahead of it
// while within 5; others: their navigator's target, else where they stand), swept
// against this ped's path when it moves (a vehicle it's in instead), else avoided as a
// circle a step along its way. It's dropped from the obstacles when it was the last one
// added.
void __fastcall ConsiderPed(void* self, re::Edx, uint8_t* ped) {
    if (ped == av_player) {
        float distSq = float(double(avObstacleDir[0]) * avObstacleDir[0]
                             + double(avObstacleDir[1]) * avObstacleDir[1]);
        if (!(distSq < 25.0)) {
            return;
        }
        float f[3];
        float fx = AV_GetForward(ped, f)[0];
        float fy = AV_GetForward(ped, f)[1];
        float fz = AV_GetForward(ped, f)[2];
        AvVec ahead = {float(fx * 3.0), float(fy * 3.0), float(3.0 * fz)};
        avObstacleGoal[0] = ahead.x + avObstaclePos[0];
        avObstacleGoal[1] = ahead.y + avObstaclePos[1];
        avObstacleGoal[2] = ahead.z + avObstaclePos[2];
        avObstacleGoal[3] = 0.0f;
    } else if (!AV_Navigator_GetTarget(ped + 0xDAC, avObstacleGoal)) {
        avObstacleGoal[0] = avObstaclePos[0];
        avObstacleGoal[1] = avObstaclePos[1];
        avObstacleGoal[2] = avObstaclePos[2];
        avObstacleGoal[3] = avObstaclePos[3];
    }
    avObstacleMove[0] = avObstacleGoal[0] - avObstaclePos[0];
    avObstacleMove[3] = 0.0f;
    avObstacleMove[1] = avObstacleGoal[1] - avObstaclePos[1];
    avObstacleMove[2] = avObstacleGoal[2] - avObstaclePos[2];
    avObstacleMove[2] = 0.0f;
    uint8_t* vehicle = avAt<uint8_t*>(ped, 0x1554);
    float speed = vehicle ? FlatSpeed(vehicle) : FlatSpeed(ped);
    avObstacleSpeed = float(speed * 50.0);
    avObstacleRadius = avVCall<float>(ped, 0x74);
    uint8_t& flags = avAt<uint8_t>(self, 0x6D);
    if (vehicle) {
        if (!AV_SweepAgainst(self, vehicle)) {
            return;
        }
        flags |= 4;
    } else {
        if (!AV_5DE7C0(self)) {
            return;
        }
        flags |= 2;
        float dir[3] = {avObstacleMove[0], avObstacleMove[1], avObstacleMove[2]};
        AV_Normalise2(dir);
        float step = avStep;
        if (!(step < double(0.1f))) {
            step = 0.1f;
        }
        float scale = avObstacleSpeed * step;
        float along[3] = {dir[0] * scale, dir[1] * scale, scale * dir[2]};
        float at[3] = {along[0] + avObstaclePos[0], along[1] + avObstaclePos[1], along[2] + avObstaclePos[2]};
        AV_AvoidCircle(self, at);
    }
    if (avObstacleCount > 0 && avObstacles[avObstacleCount - 1] == ped) {
        avObstacleCount--;
    }
}

// 0x5E01F0 (thiscall): a nearby vehicle: its move over 50 frames swept against the
// ped's path
void __fastcall ConsiderVehicle(void* self, re::Edx, uint8_t* vehicle) {
    float vx = avAt<float>(vehicle, 0x124);
    float vy = avAt<float>(vehicle, 0x128);
    float vz = avAt<float>(vehicle, 0x12C);
    float speed = SqrtAbs(double(vx) * vx + double(vy) * vy);
    avObstacleSpeed = float(speed * 50.0);
    avObstacleRadius = avVCall<float>(vehicle, 0x74);
    AvVec move = {float(vx * 50.0), float(vy * 50.0), float(50.0 * vz)};
    float gx = avObstaclePos[0] + move.x;
    float gy = avObstaclePos[1] + move.y;
    float gz = avObstaclePos[2] + move.z;
    avObstacleGoal[0] = gx;
    avObstacleGoal[3] = 0.0f;
    avObstacleGoal[1] = gy;
    avObstacleGoal[2] = gz;
    avObstacleMove[0] = gx - avObstaclePos[0];
    avObstacleMove[3] = 0.0f;
    avObstacleMove[1] = gy - avObstaclePos[1];
    avObstacleMove[2] = gz - avObstaclePos[2];
    avObstacleMove[2] = 0.0f;
    if (AV_SweepAgainst(self, vehicle)) {
        avAt<uint8_t>(self, 0x6D) |= 4;
    }
}

// 0x5E0340 (thiscall): the avoidance pass: the state set up for the ped moving along
// dir towards target at `speed`, the peds near it (within 10 and 2.5 in height; the
// nearest within 2 kept, up to 4 remembered) and the vehicles within 10 considered, then
// the side chosen: dir comes back steered. (A nearby ped in a vehicle that isn't a
// bike stops it all, dir untouched.)
void __fastcall Avoid(void* self, re::Edx, float* dir, const float* target, float speed, int flag) {
    uint8_t* ped = avAt<uint8_t*>(self, 0);
    (void)speed;
    const float* pos = EntityPos(ped);
    float px = pos[0];
    float py = pos[1];
    float pz = pos[2];
    avPos[0] = px;
    avPos[3] = 0.0f;
    avPos[1] = py;
    avPos[2] = pz;
    avDir[0] = dir[0];
    avDir[1] = dir[1];
    avDir[2] = dir[2];
    avDir[3] = 0.0f;
    float tx = target[0];
    float ty = target[1];
    float tz = target[2];
    avTarget[0] = tx;
    avTarget[1] = ty;
    avTarget[3] = 0.0f;
    avTarget[2] = tz;
    avToTarget[0] = tx - px;
    avToTarget[3] = 0.0f;
    avToTarget[1] = ty - py;
    avToTarget[2] = tz - pz;
    avToTarget[2] = 0.0f;
    uint8_t* vehicle = avAt<uint8_t*>(ped, 0x1554);
    float radius;
    if (vehicle && avAt<int32_t>(vehicle, 0x46C) == 1) {
        float s = float(FlatSpeed(vehicle) * 50.0);
        avSpeed = s > 0.5f ? s : 0.5f;
        uint8_t* col = static_cast<uint8_t*>(AV_GetColModel(vehicle));
        radius = std::fabs(float(avAt<float>(col, 0x10) * 1.25));
    } else {
        avSpeed = float(FlatSpeed(ped) * 50.0);
        radius = avVCall<float>(ped, 0x74);
    }
    avRadius = radius;
    if (0.0f == avSpeed) {
        avTimeToTarget = float(3.4028234663852886e+38);
    } else {
        float dist = SqrtAbs(double(avToTarget[0]) * avToTarget[0] + double(avToTarget[1]) * avToTarget[1]);
        avTimeToTarget = float(double(dist) / avSpeed);
    }
    avLeftCost = 0.0f;
    avRightCost = 0.0f;
    avHeading = AV_Heading(dir);
    avMaxAngle = 0.0f;
    avObstacleCount = 0;
    avMinAngle = 0.0f;
    avNearestDist = 3.4028235e+38f;
    avSeg1[0] = avPos[0];
    avSeg1[1] = avPos[1];
    avSeg1[2] = avPos[2];
    avSeg1[3] = 0.0f;
    avSeg1[4] = avTarget[0];
    avSeg1[5] = avTarget[1];
    avSeg1[6] = avTarget[2];
    avSeg1[7] = 0.0f;
    AV_5DF660(self);
    AV_5DE2A0(self);
    int count = avAt<int32_t>(ped, 0x3C0);
    re::Ptr32<uint8_t>* nearby = &avAt<uint8_t*>(ped, 0x3C4);
    for (int i = 0; i < count; i++) {
        uint8_t* other = nearby[i];
        if (other == avAt<uint8_t*>(self, 4)) {
            continue;
        }
        if (avAt<int32_t>(other, 0x1310) == 0xD && avAt<int32_t>(other, 0x1E94) != 0) {
            continue;
        }
        int model = avAt<int16_t>(other, 0x10E);
        if (model == av_poiPointModel || avAt<uint32_t>(other, 0x20) == 0) {
            continue;
        }
        if (avAt<uint32_t>(other, 0x1C) == 0 && avAt<uint32_t>(other, 0x60) == 0) {
            continue;
        }
        if ((av_modelInfos[model][0xB] & uint8_t(1 << (av_chapter & 0x1F))) == 0) {
            continue;
        }
        if (avAt<uint32_t>(other, 0xF4) != 0) {
            continue;
        }
        const float* at = EntityPos(other);
        float ox = at[0];
        float oy = at[1];
        float oz = at[2];
        avObstaclePos[0] = ox;
        avObstaclePos[1] = oy;
        avObstaclePos[2] = oz;
        avObstaclePos[3] = 0.0f;
        float dx = ox - avPos[0];
        avObstacleDir[0] = dx;
        avObstacleDir[3] = 0.0f;
        float dy = oy - avPos[1];
        avObstacleDir[1] = dy;
        float dz = oz - avPos[2];
        avObstacleDir[2] = dz;
        float dist = SqrtAbs(double(dx) * dx + double(dy) * dy);
        if (!(dist < 10.0f)) {
            continue;
        }
        if (!(std::fabs(dz) < 2.5f)) {
            continue;
        }
        uint8_t* inVehicle = avAt<uint8_t*>(other, 0x1554);
        if (inVehicle && avAt<int32_t>(inVehicle, 0x46C) != 1) {
            return;
        }
        if (dist < 2.0f && !inVehicle) {
            avObstacles[avObstacleCount] = other;
            avObstacleCount = int32_t(AV_Min(float(avObstacleCount + 1), 4.0f));
            if (avNearestDist > dist) {
                avNearestDist = dist;
                AV_EntityRefSet(&avAt<uint8_t*>(self, 8), other);
            }
        }
        float dot = float(double(avObstacleDir[1]) * avToTarget[1] + double(avObstacleDir[0]) * avToTarget[0]
                          + double(avObstacleDir[2]) * avToTarget[2]);
        if (dot < 0.0f || std::isnan(dot)) {
            continue;
        }
        AV_ConsiderPed(self, other);
    }
    uint8_t* pool = av_vehiclePool;
    int size = *reinterpret_cast<int32_t*>(pool + 8);
    for (int i = 0; i < size; i++) {
        if ((*reinterpret_cast<re::Ptr32<uint8_t>*>(pool + 4))[i] & 0x80) {
            continue;
        }
        uint8_t* other = *reinterpret_cast<re::Ptr32<uint8_t>*>(pool) + i * *reinterpret_cast<int32_t*>(pool + 0xC);
        if (!other || other == avAt<uint8_t*>(self, 4)) {
            continue;
        }
        if (avAt<int32_t>(other, 0x46C) == 1 && AV_Vehicle_4CE560(other + 0x1C4, 0) != 0) {
            continue;
        }
        const float* at = EntityPos(other);
        float ox = at[0];
        float oy = at[1];
        float oz = at[2];
        avObstaclePos[2] = oz;
        avObstaclePos[0] = ox;
        avObstaclePos[3] = 0.0f;
        avObstaclePos[1] = oy;
        float dx = ox - avPos[0];
        avObstacleDir[0] = dx;
        avObstacleDir[3] = 0.0f;
        float dy = oy - avPos[1];
        avObstacleDir[1] = dy;
        float dz = oz - avPos[2];
        avObstacleDir[2] = dz;
        float distSq = float(double(dx) * dx + double(dy) * dy);
        if (!(distSq < 100.0)) {
            continue;
        }
        if (!(std::fabs(dz) < 2.5f)) {
            continue;
        }
        AV_ConsiderVehicle(self, other);
    }
    if (0.0f != avMaxAngle && 0.0f != avMinAngle) {
        AV_AvoidNearLines(self);
    }
    AV_ChooseSide(self, flag);
    dir[0] = avDir[0];
    dir[1] = avDir[1];
    dir[2] = avDir[2];
}

// 0x5E0B20 (thiscall): the navigator's entry (pos by value): not for POI points (the
// speed only), nothing while the ped's +0x4E4 is set (the speed 0); the flags' low bit
// set from the others
void __fastcall AvoidAt(uint8_t* self, re::Edx, float x, float y, float z, float* dir, void*, float speed, int flag) {
    uint8_t* ped = avAt<uint8_t*>(self, 0);
    if (avAt<int16_t>(ped, 0x10E) == av_poiPointModel) {
        avAt<float>(self, 0x18) = speed;
        return;
    }
    bool free = avAt<uint8_t>(ped, 0x4E4) == 0;
    if (!free) {
        avAt<float>(self, 0x18) = 0.0f;
    }
    uint8_t& flags = avAt<uint8_t>(self, 0x6D);
    flags = uint8_t((flags & 0xE0) | ((flags & 0x1E) != 0 ? 1 : 0));
    if (free) {
        avAt<float>(self, 0x18) = speed;
        float pos[3] = {x, y, z};
        AV_Avoid(self, dir, pos, speed, flag);
    }
}

// 0x5E28B0 (thiscall): blocked: the flags' low bit without an avoidance this time
uint8_t __fastcall IsBlocked(const uint8_t* self) {
    uint8_t flags = avAt<uint8_t>(self, 0x6D);
    return (flags & 1) && (flags & 0x1E) == 0;
}

#define PEDAVOID2_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(PedAvoid2Original_, __LINE__);                                        \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(PedAvoid2Original_, __LINE__), \
             re::Mode::Original)

PEDAVOID2_REVERSED("PedAvoidance::SweepAgainst", 0x005DFB80, &SweepAgainst);
PEDAVOID2_REVERSED("PedAvoidance::ConsiderPed", 0x005DFEC0, &ConsiderPed);
PEDAVOID2_REVERSED("PedAvoidance::ConsiderVehicle", 0x005E01F0, &ConsiderVehicle);
PEDAVOID2_REVERSED("PedAvoidance::Avoid", 0x005E0340, &Avoid);
PEDAVOID2_REVERSED("PedAvoidance::AvoidAt", 0x005E0B20, &AvoidAt);
PEDAVOID2_REVERSED("PedAvoidance::IsBlocked", 0x005E28B0, &IsBlocked);

} // namespace
