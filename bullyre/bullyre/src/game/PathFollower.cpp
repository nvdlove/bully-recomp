// Following paths: the path graph at 0xBF6228 (up to 550 positions, 0xC bytes, then
// 1750 nodes, 0x18 bytes, from +0x19C8; positions in 1/8 units), a follower moving
// along a curve between two nodes or along a patrol path over time (GTA's CAutoPilot
// "on rails" timing: a curve's time is its length * 1000 / speed), and a vehicle's
// path target.
// (x87 extended precision in the game; what it stores as floats is kept as floats.)

#include <cmath>
#include <cstdint>

#include "reversed.h"

namespace c01path {

struct CVector {
    float x, y, z;
};
using re::At;

// The graph: positions (short x, y, z; int8 direction x, y at +6/+7) and nodes (short
// x, y, z at +8)
GAME_ARRAY(uint8_t, thePathGraph, 0x00BF6228);
GAME_ARRAY(uint8_t, pathNodes, 0x00BF7BF0);     // thePathGraph + 0x19C8
GAME_VAR(int32_t, numPathPositions, 0x00C06950);
constexpr uint32_t NUM_PATH_NODES = 0x6D6;
GAME_VAR(void*, ms_pPatrolPaths, 0x00D13CB4);
GAME_VAR(uint32_t, m_snTimeInMilliseconds, 0x00C1A9B4);
GAME_VAR(uint32_t, timeInMs_C1A9AC, 0x00C1A9AC);   // (the pausable clock, GTA's ...PauseMode?)
GAME_VAR(CVector, vecUnset, 0x00BCC564);
inline void* const theVehicleTargets = reinterpret_cast<void*>(0x00D09660);

GAME_FN(void*, __thiscall, PatrolPath_Get, 0x005ED3A0, void* owner, int id);
GAME_FN(uint32_t, __thiscall, Path_GetNumNodes, 0x005EC670, void* path);
GAME_FN(float*, __thiscall, Path_GetNode, 0x005EC8C0, void* path, uint32_t index);
GAME_FN(void, __thiscall, PathPosition_GetDirection, 0x0043F970, const uint8_t* position, float* dx, float* dy, int direction);
GAME_FN(CVector*, __thiscall, PathPosition_GetLanePoint, 0x004403C0, const uint8_t* position, CVector* out, float dx, float dy,
        int lane, float laneWidth);
GAME_FN(float, __cdecl, CurveLength_428DA0, 0x00428DA0, const CVector* a, const CVector* b, float dax, float day, float dbx, float dby);
GAME_FN(CVector*, __thiscall, Follower_GetPrevPoint, 0x004179C0, void* self, CVector* out);
GAME_FN(CVector*, __thiscall, Follower_GetPoint, 0x00417A30, void* self, CVector* out);
GAME_FN(void, __thiscall, Follower_GetPrevDirection, 0x00417AA0, void* self, float* dx, float* dy);
GAME_FN(void, __thiscall, Follower_GetNextDirection, 0x00417B10, void* self, float* dx, float* dy);
GAME_FN(void*, __thiscall, VehicleSeats_GetOccupant, 0x004CE560, void* seats, int seat);
GAME_FN(uint8_t*, __thiscall, PathGraph_FindNodeClosestToCoors_x64, 0x00441950, void* graph, void* ped, float, float, float, int, float maxDist, int, int, int, int);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t* PathGraph_FindNodeClosestToCoors(void* a0, void* a1, CVector a2, int a3, float a4, int a5, int a6, int a7, int a8) {
    return PathGraph_FindNodeClosestToCoors_x64(a0, a1, a2.x, a2.y, a2.z, a3, a4, a5, a6, a7, a8);
}
GAME_FN(uint8_t, __cdecl, VehicleTargets_Find_5E2460, 0x005E2460, void* targets, const CVector* pos, void* self, int, uintptr_t, int, int, int,
        uintptr_t);
GAME_FN(uint8_t*, __thiscall, VehicleTargets_GetFound_5E0DA0, 0x005E0DA0, void* targets);
GAME_FN(uint8_t, __cdecl, Vehicle_IsNear_41E6C0, 0x0041E6C0, void* vehicle, CVector pos, int);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(uint8_t, __cdecl, Vehicle_AdvanceNode_41B770, 0x0041B770, void* vehicle);
GAME_FN(CVector*, __thiscall, PathGraph_GetNodeOffset_441790, 0x00441790, void* graph, CVector* out, const uint8_t* node,
        const uint8_t* prevNode);
GAME_FN(void, __thiscall, PathTarget_Update, 0x004182C0, void* self, void* vehicle);

uint8_t* PositionAt(int index) {
    return (index >= 0 && index < numPathPositions) ? thePathGraph + index * 0xC : nullptr;
}
uint8_t* NodeAt(uint32_t index) {
    return index < NUM_PATH_NODES ? pathNodes + index * 0x18 : nullptr;
}
CVector NodePosition(const uint8_t* node) {
    return {static_cast<float>(At<int16_t>(const_cast<uint8_t*>(node), 8) * 0.125),
            static_cast<float>(At<int16_t>(const_cast<uint8_t*>(node), 0xA) * 0.125),
            static_cast<float>(At<int16_t>(const_cast<uint8_t*>(node), 0xC) * 0.125)};
}
// an unsigned int read as the game's fild (+2^32 when negative)
double Unsigned(uint32_t value) {
    return static_cast<double>(value);
}
// the curve's time: trunc(trunc(1000 / speed) * length) (64-bit truncations, the low
// 32 bits kept)
uint32_t CurveTime(float speed, float length) {
    uint32_t perUnit = static_cast<uint32_t>(static_cast<int64_t>(1000.0 / speed));
    return static_cast<uint32_t>(static_cast<int64_t>(Unsigned(perUnit) * length));
}

// ---- the graph ----

// 0x417850 (thiscall, the graph): a node (nullptr from 1750)
uint8_t* __fastcall PathGraph_GetNode(uint8_t* self, re::Edx, uint32_t index) {
    if (index < NUM_PATH_NODES) {
        return self + (index * 3 + 0x339) * 8;
    }
    return nullptr;
}

// 0x417C00 (thiscall, a node): its position
CVector* __fastcall PathNode_GetPosition(uint8_t* self, re::Edx, CVector* out) {
    out->x = static_cast<float>(At<int16_t>(self, 8) * 0.125);
    out->y = static_cast<float>(At<int16_t>(self, 0xA) * 0.125);
    out->z = static_cast<float>(At<int16_t>(self, 0xC) * 0.125);
    return out;
}

// 0x417920, 0x417940, 0x417960 (thiscall): the nodes of a link (+4, +0, +8)
uint8_t* __fastcall Link_GetNode4(uint8_t* self, re::Edx) {
    return NodeAt(At<uint32_t>(self, 4));
}
uint8_t* __fastcall Link_GetNode0(uint8_t* self, re::Edx) {
    return NodeAt(At<uint32_t>(self, 0));
}
uint8_t* __fastcall Link_GetNode8(uint8_t* self, re::Edx) {
    return NodeAt(At<uint32_t>(self, 8));
}

// 0x417870: exactly equal
uint32_t __cdecl VectorsEqual(const CVector* a, const CVector* b) {
    if (b->x == a->x && b->y == a->y && b->z == a->z) {
        return 1;
    }
    return 0;
}

// ---- the follower ----
// +0xC the curve's start time, +0x10 its time (-1: none), +0x14 how far along it,
// +0x18/+0x1C the positions from/to, +0x2D/+0x2E their directions, +0x2F/+0x30
// lanes, +0x32 the kind (0x16/0x17: a patrol path, reversed for 0x17), +0x38 speed,
// +0x54 [8?] a route of node indices, +0x74 its length, +0x84 the patrol path, +0x88
// the patrol node reached

// 0x4178B0 (thiscall): the route's first node dropped
void __fastcall Follower_PopRoute(uint8_t* self, re::Edx) {
    At<int16_t>(self, 0x74)--;
    for (int16_t i = 0; i < At<int16_t>(self, 0x74); i++) {
        At<int32_t>(self, 0x54 + i * 4) = At<int32_t>(self, 0x58 + i * 4);
    }
}

// 0x4178E0 / 0x417900 (thiscall): the positions to / from
uint8_t* __fastcall Follower_GetToPosition(uint8_t* self, re::Edx) {
    return PositionAt(At<int32_t>(self, 0x1C));
}
uint8_t* __fastcall Follower_GetFromPosition(uint8_t* self, re::Edx) {
    return PositionAt(At<int32_t>(self, 0x18));
}

// 0x417980 (thiscall): on to the patrol path's next node; false past its end
uint8_t __fastcall Follower_NextPatrolNode(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0x88)++;
    void* path = PatrolPath_Get(ms_pPatrolPaths, At<int32_t>(self, 0x84));
    if (!path) {
        return 0;
    }
    return At<uint32_t>(self, 0x88) < Path_GetNumNodes(path);
}

// 0x4179C0 (thiscall): the patrol node left (vecUnset without a path)
CVector* __fastcall Follower_GetPrevPointImpl(uint8_t* self, re::Edx, CVector* out) {
    void* path = PatrolPath_Get(ms_pPatrolPaths, At<int32_t>(self, 0x84));
    if (!path) {
        *out = vecUnset;
        return out;
    }
    const float* node = Path_GetNode(path, At<uint32_t>(self, 0x88) - 1);
    *out = {node[0], node[1], node[2]};
    return out;
}

// 0x417A30 (thiscall): the patrol node heading to
CVector* __fastcall Follower_GetPointImpl(uint8_t* self, re::Edx, CVector* out) {
    void* path = PatrolPath_Get(ms_pPatrolPaths, At<int32_t>(self, 0x84));
    if (!path) {
        *out = vecUnset;
        return out;
    }
    const float* node = Path_GetNode(path, At<uint32_t>(self, 0x88));
    *out = {node[0], node[1], node[2]};
    return out;
}

// 0x417AA0 (thiscall): the direction of the patrol segment being followed
void __fastcall Follower_GetPrevDirectionImpl(uint8_t* self, re::Edx, float* dx, float* dy) {
    void* path = PatrolPath_Get(ms_pPatrolPaths, At<int32_t>(self, 0x84));
    if (!path) {
        return;
    }
    const float* from;
    uint32_t to;
    if (At<uint32_t>(self, 0x88) == 0) {
        from = Path_GetNode(path, 0);
        to = 1;
    } else {
        from = Path_GetNode(path, At<uint32_t>(self, 0x88) - 1);
        to = At<uint32_t>(self, 0x88);
    }
    const float* next = Path_GetNode(path, to);
    *dx = next[0] - from[0];
    *dy = next[1] - from[1];
}

// 0x417B10 (thiscall): the direction of the patrol segment after the node heading to
// (the last one's own at the end)
void __fastcall Follower_GetNextDirectionImpl(uint8_t* self, re::Edx, float* dx, float* dy) {
    void* path = PatrolPath_Get(ms_pPatrolPaths, At<int32_t>(self, 0x84));
    if (!path) {
        return;
    }
    uint32_t index = At<uint32_t>(self, 0x88);
    const float* from;
    uint32_t to;
    if (index == Path_GetNumNodes(path) - 1) {
        from = Path_GetNode(path, index - 1);
        to = At<uint32_t>(self, 0x88);
    } else {
        from = Path_GetNode(path, index);
        to = At<uint32_t>(self, 0x88) + 1;
    }
    const float* next = Path_GetNode(path, to);
    *dx = next[0] - from[0];
    *dy = next[1] - from[1];
}

// 0x417C50 (thiscall): the speed changed: the curve's time recomputed (keeping how far
// along it is), or at a stop (speed 1e-4 or less) the progress kept and the curve
// timing dropped
void __fastcall Follower_SetSpeed(uint8_t* self, re::Edx, float speed) {
    CVector from = {0.0f, 0.0f, 0.0f};
    CVector to = {0.0f, 0.0f, 0.0f};
    int32_t duration = At<int32_t>(self, 0x10);
    At<float>(self, 0x38) = speed;
    if (!(speed > 1.0000000474974513e-4) && !std::isnan(speed)) {
        if (duration != -1) {
            uint32_t now = timeInMs_C1A9AC;
            float progress;
            if (now < At<uint32_t>(self, 0xC)) {
                progress = 0.0f;
            } else {
                progress = static_cast<float>(Unsigned(now - At<uint32_t>(self, 0xC)) / Unsigned(static_cast<uint32_t>(duration)));
            }
            At<float>(self, 0x14) = progress;
        }
        At<float>(self, 0x38) = 0.0f;
        At<int32_t>(self, 0x10) = -1;
        At<int32_t>(self, 0xC) = -1;
        return;
    }
    float t;
    if (duration == -1) {
        t = At<float>(self, 0x14);
    } else if (timeInMs_C1A9AC < At<uint32_t>(self, 0xC)) {
        t = 0.0f;
    } else {
        t = static_cast<float>(Unsigned(timeInMs_C1A9AC - At<uint32_t>(self, 0xC)) / Unsigned(static_cast<uint32_t>(duration)));
    }
    float d0x, d0y, d1x, d1y;
    uint8_t kind = At<uint8_t>(self, 0x32);
    if (kind == 0x16 || kind == 0x17) {
        CVector tmp;
        from = *Follower_GetPrevPoint(self, &tmp);
        to = *Follower_GetPoint(self, &tmp);
        Follower_GetPrevDirection(self, &d0x, &d0y);
        Follower_GetNextDirection(self, &d1x, &d1y);
    } else {
        uint8_t* a = PositionAt(At<int32_t>(self, 0x18));
        uint8_t* b = PositionAt(At<int32_t>(self, 0x1C));
        PathPosition_GetDirection(a, &d0x, &d0y, static_cast<int8_t>(At<uint8_t>(self, 0x2D)));
        PathPosition_GetDirection(b, &d1x, &d1y, static_cast<int8_t>(At<uint8_t>(self, 0x2E)));
        double laneA = (static_cast<int8_t>(At<uint8_t>(self, 0x2F)) + 0.5) * 5.0;
        float ax = static_cast<float>(At<int16_t>(a, 0) * 0.125);
        from.x = static_cast<float>(ax + d0y * laneA);
        float ay = static_cast<float>(At<int16_t>(a, 2) * 0.125);
        from.y = static_cast<float>(ay - laneA * d0x);
        double laneB = (static_cast<int8_t>(At<uint8_t>(self, 0x30)) + 0.5) * 5.0;
        float bx = static_cast<float>(At<int16_t>(b, 0) * 0.125);
        to.x = static_cast<float>(bx + d1y * laneB);
        float by = static_cast<float>(At<int16_t>(b, 2) * 0.125);
        to.y = static_cast<float>(by - laneB * d1x);
    }
    float length = CurveLength_428DA0(&from, &to, d0x, d0y, d1x, d1y);
    uint32_t time = CurveTime(At<float>(self, 0x38), length);
    At<uint32_t>(self, 0x10) = time;
    double elapsed = Unsigned(time) * t;
    uint32_t now = timeInMs_C1A9AC;
    if (Unsigned(now) < elapsed) {
        if (time != 0xFFFFFFFF) {
            At<float>(self, 0x14) = t;
        }
        At<int32_t>(self, 0x10) = -1;
        At<int32_t>(self, 0xC) = -1;
        return;
    }
    At<uint32_t>(self, 0xC) = now - static_cast<uint32_t>(static_cast<int64_t>(elapsed));
}

// 0x418010 / 0x418080 (thiscall): the from / to position's point in the follower's
// lane (5 units a lane)
CVector* __fastcall Follower_GetFromLanePoint(uint8_t* self, re::Edx, CVector* out) {
    uint8_t* position = PositionAt(At<int32_t>(self, 0x18));
    int lane = static_cast<int8_t>(At<uint8_t>(self, 0x2F));
    float dx, dy;
    PathPosition_GetDirection(position, &dx, &dy, static_cast<int8_t>(At<uint8_t>(self, 0x2D)));
    PathPosition_GetLanePoint(position, out, dx, dy, lane, 5.0f);
    return out;
}
CVector* __fastcall Follower_GetToLanePoint(uint8_t* self, re::Edx, CVector* out) {
    uint8_t* position = PositionAt(At<int32_t>(self, 0x1C));
    int lane = static_cast<int8_t>(At<uint8_t>(self, 0x30));
    float dx, dy;
    PathPosition_GetDirection(position, &dx, &dy, static_cast<int8_t>(At<uint8_t>(self, 0x2E)));
    PathPosition_GetLanePoint(position, out, dx, dy, lane, 5.0f);
    return out;
}

// 0x4180F0 / 0x418130 (thiscall): the from / to position's direction
void __fastcall Follower_GetFromDirection(uint8_t* self, re::Edx, float* dx, float* dy) {
    PathPosition_GetDirection(PositionAt(At<int32_t>(self, 0x18)), dx, dy, static_cast<int8_t>(At<uint8_t>(self, 0x2D)));
}
void __fastcall Follower_GetToDirection(uint8_t* self, re::Edx, float* dx, float* dy) {
    PathPosition_GetDirection(PositionAt(At<int32_t>(self, 0x1C)), dx, dy, static_cast<int8_t>(At<uint8_t>(self, 0x2E)));
}

// 0x418170 (thiscall): a patrol path to follow (started now: its first curve timed;
// no such path: kind 0)
void __fastcall Follower_SetPatrolPath(uint8_t* self, re::Edx, int path, int start, int reversed) {
    At<uint8_t>(self, 0x32) = 0x16;
    if (re::Flag(reversed)) {
        At<uint8_t>(self, 0x32) = 0x17;
    }
    At<int32_t>(self, 0x84) = path;
    At<uint32_t>(self, 0x88) = 0;
    if (!re::Flag(start)) {
        return;
    }
    At<uint32_t>(self, 0x88) = 1;
    void* patrol = PatrolPath_Get(ms_pPatrolPaths, path);
    if (!patrol || At<uint32_t>(self, 0x88) >= Path_GetNumNodes(patrol)) {
        At<uint8_t>(self, 0x32) = 0;
        return;
    }
    At<uint32_t>(self, 0xC) = m_snTimeInMilliseconds;
    CVector from, to;
    Follower_GetPrevPoint(self, &from);
    Follower_GetPoint(self, &to);
    float d0x, d0y, d1x, d1y;
    Follower_GetPrevDirection(self, &d0x, &d0y);
    Follower_GetNextDirection(self, &d1x, &d1y);
    float length = CurveLength_428DA0(&from, &to, d0x, d0y, d1x, d1y);
    At<uint32_t>(self, 0x10) = CurveTime(At<float>(self, 0x38), length);
}

// ---- a vehicle's path target: the point (+0), +0xC close enough, +0xD going by the
// path nodes, +0xE worked out ----

// 0x417B90 (thiscall): a new point (worked out again)
void __fastcall PathTarget_Set(uint8_t* self, re::Edx, const CVector* point) {
    CVector& current = At<CVector>(self, 0);
    if (point->x != current.x || point->y != current.y || point->z != current.z) {
        current = *point;
        At<uint8_t>(self, 0xE) = 0;
    }
}

// 0x417BE0 (thiscall): reached (not going by nodes, or close)
uint32_t __fastcall PathTarget_IsDirect(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 0xD) && At<uint8_t>(self, 0xC)) {
        return 0;
    }
    return 1;
}

// 0x4182C0 (thiscall): worked out for the vehicle: the nearest target point (the
// target list's, or this point), and whether to go there by the path nodes (not when
// near, the vehicle's node is the point's, or it's within 30 units flat)
void __fastcall PathTarget_UpdateImpl(uint8_t* self, re::Edx, uint8_t* vehicle) {
    CVector target = {0.0f, 0.0f, 0.0f};
    void* seats = vehicle + 0x1C4;
    At<uint8_t>(self, 0xD) = 1;
    At<uint8_t>(self, 0xC) = 0;
    CVector point = At<CVector>(self, 0);
    uint8_t* closest = PathGraph_FindNodeClosestToCoors(thePathGraph, VehicleSeats_GetOccupant(seats, 0), point, 0, 999999.9f, 0, 0, 0, 0);
    CVector closestPos = NodePosition(closest);
    if (VehicleTargets_Find_5E2460(theVehicleTargets, &closestPos, self, 0, 0, 0x32, 0, 1, 0)) {
        uint8_t* found = VehicleTargets_GetFound_5E0DA0(theVehicleTargets);
        target = {At<float>(found, 8), At<float>(found, 0xC), At<float>(found, 0x10)};
    } else {
        target = At<CVector>(self, 0);
    }
    uint8_t* node = nullptr;
    if (Vehicle_IsNear_41E6C0(vehicle, target, 0)) {
        At<uint8_t>(self, 0xD) = 0;
    } else {
        node = NodeAt(At<uint32_t>(vehicle, 0x2AC));
        point = At<CVector>(self, 0);
        if (node == PathGraph_FindNodeClosestToCoors(thePathGraph, VehicleSeats_GetOccupant(seats, 0), point, 0, 999999.9f, 0, 0, 0, 0)) {
            At<uint8_t>(self, 0xD) = 0;
        }
    }
    if (At<uint8_t>(self, 0xD)) {
        CVector nodePos = NodePosition(node);
        CVector tmp;
        const CVector* d = VectorSub(&tmp, &target, &nodePos);
        float distSq = d->x * d->x + d->y * d->y;
        if (distSq < 900.0) {
            At<uint8_t>(self, 0xD) = 0;
        }
    }
    At<CVector>(vehicle, 0x2F0) = target;
    At<uint8_t>(self, 0xE) = 1;
}

// 0x4184F0 (thiscall): where the vehicle drives to next: the point itself, or the
// next node's point (moving on to the node after within 4 units flat)
void __fastcall PathTarget_GetNextPoint(uint8_t* self, re::Edx, uint8_t* vehicle, CVector* out) {
    if (!At<uint8_t>(self, 0xE)) {
        PathTarget_Update(self, vehicle);
    }
    if (!At<uint8_t>(self, 0xD)) {
        *out = At<CVector>(self, 0);
        return;
    }
    uint8_t* matrix = At<uint8_t*>(vehicle, 0x14);
    const CVector* vehiclePos = matrix ? reinterpret_cast<CVector*>(matrix + 0x30) : reinterpret_cast<CVector*>(vehicle + 4);
    CVector nodePos = NodePosition(NodeAt(At<uint32_t>(vehicle, 0x2AC)));
    CVector tmp;
    const CVector* d = VectorSub(&tmp, &nodePos, vehiclePos);
    float distSq = d->x * d->x + d->y * d->y;
    float dist = static_cast<float>(std::sqrt(static_cast<double>(std::fabs(distSq))));
    if (dist < 4.0f) {
        At<uint8_t>(self, 0xC) = 1;
        if (Vehicle_AdvanceNode_41B770(vehicle)) {
            *out = At<CVector>(self, 0);
            At<uint8_t>(self, 0xD) = 0;
            return;
        }
    }
    uint8_t* prev = NodeAt(At<uint32_t>(vehicle, 0x2A8));
    uint8_t* node = NodeAt(At<uint32_t>(vehicle, 0x2AC));
    nodePos = NodePosition(NodeAt(At<uint32_t>(vehicle, 0x2AC)));
    CVector offset;
    CVector* o = PathGraph_GetNodeOffset_441790(thePathGraph, &offset, node, prev);
    CVector sum;
    *out = *VectorAdd(&sum, &nodePos, o);
}

} // namespace c01path

// ---- registration ----------------------------------------------------------------------

#define PATH_REVERSED_C01(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace c01path {
PATH_REVERSED_C01("CPathGraph::GetNode", 0x00417850, &PathGraph_GetNode);
PATH_REVERSED_C01("CVector::Equals", 0x00417870, &VectorsEqual);
PATH_REVERSED_C01("CPathFollower::PopRoute", 0x004178B0, &Follower_PopRoute);
PATH_REVERSED_C01("CPathFollower::GetToPosition", 0x004178E0, &Follower_GetToPosition);
PATH_REVERSED_C01("CPathFollower::GetFromPosition", 0x00417900, &Follower_GetFromPosition);
PATH_REVERSED_C01("CPathLink::GetNode (+4)", 0x00417920, &Link_GetNode4);
PATH_REVERSED_C01("CPathLink::GetNode (+0)", 0x00417940, &Link_GetNode0);
PATH_REVERSED_C01("CPathLink::GetNode (+8)", 0x00417960, &Link_GetNode8);
PATH_REVERSED_C01("CPathFollower::NextPatrolNode", 0x00417980, &Follower_NextPatrolNode);
PATH_REVERSED_C01("CPathFollower::GetPrevPoint", 0x004179C0, &Follower_GetPrevPointImpl);
PATH_REVERSED_C01("CPathFollower::GetPoint", 0x00417A30, &Follower_GetPointImpl);
PATH_REVERSED_C01("CPathFollower::GetPrevDirection", 0x00417AA0, &Follower_GetPrevDirectionImpl);
PATH_REVERSED_C01("CPathFollower::GetNextDirection", 0x00417B10, &Follower_GetNextDirectionImpl);
PATH_REVERSED_C01("CPathTarget::Set", 0x00417B90, &PathTarget_Set);
PATH_REVERSED_C01("CPathTarget::IsDirect", 0x00417BE0, &PathTarget_IsDirect);
PATH_REVERSED_C01("CPathNode::GetPosition", 0x00417C00, &PathNode_GetPosition);
PATH_REVERSED_C01("CPathFollower::SetSpeed", 0x00417C50, &Follower_SetSpeed);
PATH_REVERSED_C01("CPathFollower::GetFromLanePoint", 0x00418010, &Follower_GetFromLanePoint);
PATH_REVERSED_C01("CPathFollower::GetToLanePoint", 0x00418080, &Follower_GetToLanePoint);
PATH_REVERSED_C01("CPathFollower::GetFromDirection", 0x004180F0, &Follower_GetFromDirection);
PATH_REVERSED_C01("CPathFollower::GetToDirection", 0x00418130, &Follower_GetToDirection);
PATH_REVERSED_C01("CPathFollower::SetPatrolPath", 0x00418170, &Follower_SetPatrolPath);
PATH_REVERSED_C01("CPathTarget::Update", 0x004182C0, &PathTarget_UpdateImpl);
PATH_REVERSED_C01("CPathTarget::GetNextPoint", 0x004184F0, &PathTarget_GetNextPoint);
} // namespace c01path
