// CCarCtrl: a vehicle put on the road graph (GTA's JoinCarWithRoadSystem and
// JoinCarWithRoadSystemGotoCoors). The vehicle's autopilot is at +0x2A8: +0 the node
// it's coming from, +4 the node it's heading to, +8, +0x54 the route (8 nodes) and
// +0x74 its length (vehicle +0x2FC / +0x31C).
// (x87 extended precision in the game; what it stores as floats is kept as floats.)

#include <cmath>
#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace c01join {

struct CVector {
    float x, y, z;
};
using re::At;

GAME_ARRAY(uint8_t, pathNodes, 0x00BF7BF0);
constexpr uint32_t NUM_PATH_NODES = 0x6D6;
inline void* const ThePaths = reinterpret_cast<void*>(0x00BF6228);

GAME_FN(CVector*, __thiscall, Placeable_GetForward, 0x004154A0, void* placeable, CVector* out);
GAME_FN(float*, __thiscall, Entity_GetMatrix, 0x008B6120, void* entity);
GAME_FN(uint8_t*, __thiscall, Paths_FindNodeClosestToCoorsFavourDirection_x64, 0x00441E90, void* paths, float, float, float, int type, float dirX, float dirY);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t* Paths_FindNodeClosestToCoorsFavourDirection(void* a0, CVector a1, int a2, float a3, float a4) {
    return Paths_FindNodeClosestToCoorsFavourDirection_x64(a0, a1.x, a1.y, a1.z, a2, a3, a4);
}
GAME_FN(void, __thiscall, Paths_DoPathSearch_x64, 0x00442FC0, void* paths, int type, float, float, float, int startNode, float, float, float, re::Ptr32<uint8_t>* nodes, int16_t* numNodes, int maxNodes, void* vehicle, float* distance, float maxDistance, int targetNode);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Paths_DoPathSearch(void* a0, int a1, CVector a2, int a3, CVector a4, re::Ptr32<uint8_t>* a5, int16_t* a6, int a7, void* a8, float* a9, float a10, int a11) {
    return Paths_DoPathSearch_x64(a0, a1, a2.x, a2.y, a2.z, a3, a4.x, a4.y, a4.z, a5, a6, a7, a8, a9, a10, a11);
}
GAME_FN(void, __thiscall, Paths_RemoveBadStartNode_x64, 0x00440780, void* paths, float, float, float, re::Ptr32<uint8_t>* nodes, int16_t* numNodes);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Paths_RemoveBadStartNode(void* a0, CVector a1, re::Ptr32<uint8_t>* a2, int16_t* a3) {
    return Paths_RemoveBadStartNode_x64(a0, a1.x, a1.y, a1.z, a2, a3);
}
GAME_FN(uint32_t, __thiscall, Node_GetIndex, 0x0043F8A0, void* node);
GAME_FN(uint32_t, __thiscall, Node_GetLinkedNode, 0x0043F7B0, void* node, int link);
GAME_FN(uint8_t*, __thiscall, Node_GetLink, 0x0043FE40, void* node, int link);
GAME_FN(uint8_t, __thiscall, Link_IsBlockedFrom_43F9C0, 0x0043F9C0, void* link, void* node);
GAME_FN(void, __thiscall, AutoPilot_PopRoute, 0x004178B0, void* autopilot);
GAME_FN(void, __thiscall, AutoPilot_SetNextNode_5AB1A0, 0x005AB1A0, void* autopilot, uintptr_t node);
GAME_FN(void, __cdecl, PickNextNodeAccordingStrategy_41BF30, 0x0041BF30, void* vehicle);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);

CVector* PositionOf(void* placeable) {
    uint8_t* matrix = At<uint8_t*>(placeable, 0x14);
    return matrix ? reinterpret_cast<CVector*>(matrix + 0x30) : reinterpret_cast<CVector*>(static_cast<uint8_t*>(placeable) + 4);
}
CVector NodePosition(const uint8_t* node) {
    return {static_cast<float>(At<int16_t>(const_cast<uint8_t*>(node), 8) * 0.125),
            static_cast<float>(At<int16_t>(const_cast<uint8_t*>(node), 0xA) * 0.125),
            static_cast<float>(At<int16_t>(const_cast<uint8_t*>(node), 0xC) * 0.125)};
}
uint8_t* NodeAt(uint32_t index) {
    return index < NUM_PATH_NODES ? pathNodes + index * 0x18 : nullptr;
}
float LengthSqr(const CVector* v) {
    return static_cast<float>((static_cast<double>(v->y) * v->y + static_cast<double>(v->x) * v->x) + static_cast<double>(v->z) * v->z);
}

// 0x41E370: the vehicle onto the nearest node (favouring its direction) and the
// neighbour nearest to it, the way it faces; `reversed` set when it had to turn round
void __cdecl JoinCarWithRoadSystem(uint8_t* vehicle, uint8_t* reversed) {
    if (reversed) {
        *reversed = 0;
    }
    At<int32_t>(vehicle, 0x2AC) = 0;
    At<int32_t>(vehicle, 0x2A8) = 0;
    At<int32_t>(vehicle, 0x2B0) = 0;
    At<int32_t>(vehicle, 0x2C4) = 0;
    At<int32_t>(vehicle, 0x2C8) = 0;
    At<int32_t>(vehicle, 0x2C0) = 0;
    CVector* pos = PositionOf(vehicle);
    CVector f1, f2;
    CVector* forward1 = Placeable_GetForward(vehicle, &f1);
    CVector* forward2 = Placeable_GetForward(vehicle, &f2);
    uint8_t* node = Paths_FindNodeClosestToCoorsFavourDirection(ThePaths, *pos, 0, forward1->x, forward2->y);
    if (!node) {
        return;
    }
    int links = node[0x14] & 0xF;
    uint32_t nodeIndex = Node_GetIndex(node);
    float best = 999999.9f;
    int32_t bestIndex = -1;
    bool oneWay = false;
    for (int i = 0; i < links; i++) {
        uint32_t index = Node_GetLinkedNode(node, i);
        uint8_t* other = index < NUM_PATH_NODES ? pathNodes + index * 0x18 : nullptr;
        uint8_t* link = Node_GetLink(node, i);
        if (Link_IsBlockedFrom_43F9C0(link, other)) {
            continue;
        }
        CVector a = NodePosition(node);
        CVector b = NodePosition(other);
        CVector diff;
        const CVector* d = VectorSub(&diff, &b, &a);
        float distSq = static_cast<float>(static_cast<double>(d->x) * d->x + static_cast<double>(d->y) * d->y);
        if (distSq < best) {
            best = distSq;
            bestIndex = static_cast<int32_t>(index);
            oneWay = Link_IsBlockedFrom_43F9C0(link, node) != 0;
        }
    }
    if (bestIndex < 0) {
        return;
    }
    uint8_t* bestNode = bestIndex < static_cast<int32_t>(NUM_PATH_NODES) ? pathNodes + bestIndex * 0x18 : nullptr;
    uint32_t from = static_cast<uint32_t>(bestIndex);
    uint32_t to = nodeIndex;
    if (!oneWay) {
        float fx = Entity_GetMatrix(vehicle)[4];
        float fy = Entity_GetMatrix(vehicle)[5];
        if (fx == 0.0f && fy == 0.0f) {
            fx = 1.0f;
        }
        CVector a = NodePosition(node);
        CVector b = NodePosition(bestNode);
        double along = (static_cast<double>(a.y) - b.y) * fy + (static_cast<double>(a.x) - b.x) * fx;
        if (along < 0.0) {
            from = nodeIndex;
            to = static_cast<uint32_t>(bestIndex);
            if (reversed) {
                *reversed = 1;
            }
        }
    }
    At<int32_t>(vehicle, 0x2B0) = 0;
    At<uint32_t>(vehicle, 0x2A8) = from;
    AutoPilot_SetNextNode_5AB1A0(vehicle + 0x2A8, to);
    At<int16_t>(vehicle, 0x31C) = 0;
    PickNextNodeAccordingStrategy_41BF30(vehicle);
    vehicle[0x2D7] = 0;
    vehicle[0x2D8] = 0;
}

// 0x41E6C0: a route to the target coordinates searched; when the route's end is no
// further from the target than the vehicle, the vehicle put on its first nodes and
// false returned; true (no route) otherwise. (The last argument is unused.)
uint8_t __cdecl JoinCarWithRoadSystemGotoCoors(uint8_t* vehicle, CVector target, int) {
    At<CVector>(vehicle, 0x2F0) = target;
    re::Ptr32<uint8_t>* route = reinterpret_cast<re::Ptr32<uint8_t>*>(vehicle + 0x2FC);
    int16_t* count = reinterpret_cast<int16_t*>(vehicle + 0x31C);
    Paths_DoPathSearch(ThePaths, 0, *PositionOf(vehicle), -1, target, route, count, 8, vehicle, nullptr, 999999.9f, -1);
    Paths_RemoveBadStartNode(ThePaths, *PositionOf(vehicle), route, count);
    if (*count >= 2) {
        CVector diff;
        float vehicleDistSq = LengthSqr(VectorSub(&diff, PositionOf(vehicle), &target));
        CVector end = NodePosition(route[*count - 1]);
        float endDistSq = LengthSqr(VectorSub(&diff, &end, &target));
        if (endDistSq < vehicleDistSq || endDistSq == vehicleDistSq) {
            At<int32_t>(vehicle, 0x2B0) = 0;
            At<uint32_t>(vehicle, 0x2A8) = Node_GetIndex(route[0]);
            AutoPilot_PopRoute(vehicle + 0x2A8);
            AutoPilot_SetNextNode_5AB1A0(vehicle + 0x2A8, Node_GetIndex(route[0]));
            AutoPilot_PopRoute(vehicle + 0x2A8);
            PickNextNodeAccordingStrategy_41BF30(vehicle);
            vehicle[0x2D7] = 0;
            vehicle[0x2D8] = 0;
            return 0;
        }
    }
    *count = 0;
    At<int32_t>(vehicle, 0x2AC) = 0;
    At<int32_t>(vehicle, 0x2A8) = 0;
    At<int32_t>(vehicle, 0x2B0) = 0;
    return 1;
}

inline void __cdecl Srand(uint32_t seed) { crt::Srand((unsigned int)seed); }
GAME_FN(uint8_t*, __thiscall, AutoPilot_GetCurrentNode, 0x00417940, void* autopilot);
GAME_FN(int32_t, __thiscall, Node_GetLinkInfo_43F820, 0x0043F820, void* node, int link);
GAME_FN(float, __cdecl, DistanceToSegment_5691A0, 0x005691A0, const CVector* a, const CVector* b, const CVector* point);

// 0x41BF30: GTA's FindLinksToGoWithTheseNodes: the links (+0x2C4 to the next node,
// +0x2C0 the one arrived by: the neighbour nearest the vehicle, other than the next)
// and their directions (+0x2D6, +0x2D5); the random seed (+0x338) set first
void __cdecl FindLinksToGoWithTheseNodes(uint8_t* vehicle) {
    if (At<uint16_t>(vehicle, 0x338) != 0) {
        Srand(At<uint16_t>(vehicle, 0x338));
    }
    void* autopilot = vehicle + 0x2A8;
    int16_t i = 0;
    for (; i < 12; i++) {
        if (Node_GetLinkedNode(AutoPilot_GetCurrentNode(autopilot), i) == At<uint32_t>(vehicle, 0x2AC)) {
            break;
        }
    }
    At<int32_t>(vehicle, 0x2C4) = Node_GetLinkInfo_43F820(AutoPilot_GetCurrentNode(autopilot), i);
    vehicle[0x2D6] = At<int32_t>(vehicle, 0x2A8) >= At<int32_t>(vehicle, 0x2AC) ? 1 : 0xFF;
    uint8_t* node = AutoPilot_GetCurrentNode(autopilot);
    int links = node[0x14] & 0xF;
    int link = 0;
    int32_t info;
    if (links == 1) {
        info = Node_GetLinkInfo_43F820(node, 0);
    } else {
        float best = 999999.9f;
        link = -1;
        for (int l = 0; l < links; l++) {
            uint32_t other = Node_GetLinkedNode(node, l);
            if (other == At<uint32_t>(vehicle, 0x2AC)) {
                continue;
            }
            CVector a = NodePosition(node);
            CVector b = NodePosition(NodeAt(other));
            float dist = DistanceToSegment_5691A0(&a, &b, PositionOf(vehicle));
            if (dist < best) {
                best = dist;
                link = l;
            }
        }
        info = Node_GetLinkInfo_43F820(node, link);
    }
    At<int32_t>(vehicle, 0x2C0) = info;
    vehicle[0x2D5] = static_cast<int32_t>(Node_GetLinkedNode(node, link)) >= At<int32_t>(vehicle, 0x2A8) ? 1 : 0xFF;
}

// GTA's CColPoint as built on the stack here
struct CColPoint {
    CVector point;       // +0
    re::Ptr32<void> entity;        // +0xC a reference (let go of after)
    CVector normal;      // +0x10
    uint8_t unknown1C[8];
    float depth;         // +0x24
};
GAME_FN(uint8_t, __cdecl, IsStoredPolyStillValidVerticalLine_56BB20, 0x0056BB20, const CVector* pos, float z, CColPoint* point,
        void* poly);
GAME_FN(uint8_t, __cdecl, ProcessVerticalLine_4629C0, 0x004629C0, const CVector* pos, float z, CColPoint* point, void* ignored, int,
        void* poly);
GAME_FN(void, __thiscall, Matrix_Reorthogonalise, 0x00413A80, float* matrix);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(void, __thiscall, Placeable_SetPosition, 0x0041C4E0, void* placeable, float x, float y, float z);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, re::Ptr32<void>* ref);
GAME_VAR(uint32_t, m_snTimeInMilliseconds, 0x00C1A9B4);
GAME_VAR(uint32_t, timeInMs_C1A9AC, 0x00C1A9AC);

// the ground under `probe` (x, y): the stored poly, else a line from z + 1.5 down to
// z - 2, else from `high` down to `low`; `previous` when none
float GroundBelow(CVector* probe, float z, float high, float low, CColPoint* point, void* ignored, void* poly, float previous) {
    probe->z = high;
    if (IsStoredPolyStillValidVerticalLine_56BB20(probe, low, point, poly)) {
        return point->point.z;
    }
    probe->z = static_cast<float>(z + 1.5);
    if (ProcessVerticalLine_4629C0(probe, static_cast<float>(z - 2.0), point, ignored, 1, poly)) {
        return point->point.z;
    }
    probe->z = high;
    if (ProcessVerticalLine_4629C0(probe, low, point, ignored, 1, poly)) {
        return point->point.z;
    }
    return previous;
}

// 0x41FA20: GTA's DragCarToPoint: the vehicle put between the point and a point 3
// back from it towards its rear, on the ground at both (their heights kept in +0x34C
// and +0x350), its matrix rebuilt from them (re-orthonormalised once a second)
void __cdecl DragCarToPoint(uint8_t* vehicle, const CVector* target, void* ignored) {
    CColPoint point;
    point.point = {0.0f, 0.0f, 0.0f};
    point.entity = nullptr;
    point.normal = {0.0f, 0.0f, 0.0f};
    point.depth = 3.4028234663852886e+38f;
    CVector forward;
    Placeable_GetForward(vehicle, &forward);
    CVector pos = *PositionOf(vehicle);
    float behindX = static_cast<float>(pos.x - forward.x * 3.0 * 0.5);
    float behindY = static_cast<float>(pos.y - forward.y * 3.0 * 0.5);
    float tx = target->x;
    float ty = target->y;
    float dx = tx - behindX;
    float dy = ty - behindY;
    float dist = static_cast<float>(std::sqrt(std::fabs(static_cast<double>(static_cast<float>(
        static_cast<double>(dx) * dx + static_cast<double>(dy) * dy)))));
    double k = 3.0 / dist;
    float midX = static_cast<float>((static_cast<double>(behindX) - tx) * k + tx);
    float midY = static_cast<float>(ty + (static_cast<double>(behindY) - ty) * k);
    float low = static_cast<float>(pos.z - 10.0);
    float high = static_cast<float>(pos.z + 10.0);
    CVector probe = {tx, ty, high};
    float aheadZ = GroundBelow(&probe, pos.z, high, low, &point, ignored, vehicle + 0x374, At<float>(vehicle, 0x34C));
    At<float>(vehicle, 0x34C) = aheadZ;
    probe = {midX, midY, high};
    float behindZ = GroundBelow(&probe, pos.z, high, low, &point, ignored, vehicle + 0x3E0, At<float>(vehicle, 0x350));
    At<float>(vehicle, 0x350) = behindZ;
    float pitch = static_cast<float>(std::atan(static_cast<double>(static_cast<float>((aheadZ - static_cast<double>(behindZ)) / 3.0))));
    float c = static_cast<float>(std::cos(static_cast<double>(pitch)));
    float s = static_cast<float>(std::sin(static_cast<double>(pitch)));
    float* m = Entity_GetMatrix(vehicle);
    float rightX = static_cast<float>((static_cast<double>(ty) - midY) / 3.0);
    float rightY = static_cast<float>((static_cast<double>(tx) - midX) / -3.0);
    m[0] = rightX;
    m[1] = rightY;
    m[2] = 0.0f;
    m[4] = -c * m[1];
    m[5] = c * m[0];
    m[6] = s;
    // (m[7] and m[11] get uninitialised stack in the game; left here)
    m[8] = static_cast<float>(static_cast<double>(m[6]) * m[1] - static_cast<double>(m[5]) * m[2]);
    m[9] = static_cast<float>(static_cast<double>(m[4]) * m[2] - static_cast<double>(m[6]) * m[0]);
    m[10] = static_cast<float>(static_cast<double>(m[5]) * m[0] - static_cast<double>(m[4]) * m[1]);
    if (m_snTimeInMilliseconds / 1000 != timeInMs_C1A9AC / 1000) {
        Matrix_Reorthogonalise(m);
    }
    CVector ahead = {tx, ty, aheadZ};
    CVector behind = {midX, midY, behindZ};
    CVector sum;
    const CVector* p = VectorAdd(&sum, &ahead, &behind);
    float x = static_cast<float>(p->x * 0.5);
    float y = static_cast<float>(0.5 * p->y);
    float z = static_cast<float>(0.5 * p->z);
    float lift = reinterpret_cast<float(__thiscall*)(void*)>(static_cast<void*>(At<re::Ptr32<void>*>(vehicle, 0)[0xD8 / 4]))(vehicle);
    z = lift + z;
    Placeable_SetPosition(vehicle, x, y, z);
    if (point.entity) {
        CleanUpOldReference(point.entity, &point.entity);
    }
}

GAME_ARRAY(uint8_t*, ms_modelInfoPtrs, 0x00C67738);
GAME_VAR(CVector, vecUnset, 0x00BCC564);
GAME_FN(uint8_t*, __thiscall, AutoPilot_GetNextNode, 0x00417920, void* autopilot);
GAME_FN(uint8_t*, __thiscall, AutoPilot_GetNextLinkPosition, 0x004178E0, void* autopilot);
GAME_FN(uint8_t*, __thiscall, AutoPilot_GetLinkPosition, 0x00417900, void* autopilot);
GAME_FN(void, __thiscall, PathPosition_GetDirection, 0x0043F970, const uint8_t* position, float* dx, float* dy, int direction);
GAME_FN(CVector*, __thiscall, PathPosition_GetLanePoint, 0x004403C0, const uint8_t* position, CVector* out, float dx, float dy,
        int lane, float laneWidth);
GAME_FN(float, __cdecl, CurveLength_428DA0, 0x00428DA0, const CVector* a, const CVector* b, float dax, float day, float dbx, float dby);
GAME_FN(void, __cdecl, CalcCurvePoint_428E70, 0x00428E70, const CVector* a, const CVector* b, const CVector* dirA, const CVector* dirB,
        float t, int32_t time, CVector* outPos, CVector* outSpeed);
GAME_FN(float, __cdecl, FindGroundZFor3DCoord_4635A0, 0x004635A0, float x, float y, float z, uint8_t* found, int);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);

double Unsigned(uint32_t value) {
    return static_cast<double>(value);
}

// 0x41FF70: the vehicle put on its current curve (the links from +0x2C0 to +0x2C4)
// at `fraction` along it (kept a car's half length plus 1 from either end): the curve
// timing (+0x2B4 start, +0x2B8 time), the move speed, facing from node to node, on
// the ground (from `z` when `useZ`)
void __cdecl PlaceCarOnCurve(uint8_t* vehicle, float fraction, float z, int useZ) {
    void* autopilot = vehicle + 0x2A8;
    uint8_t* from = AutoPilot_GetCurrentNode(autopilot);
    uint8_t* to = AutoPilot_GetNextNode(autopilot);
    CVector pos = vecUnset;
    const float* col = At<float*>(ms_modelInfoPtrs[At<int16_t>(vehicle, 0x10E)], 0xC);
    float colW = col[11];
    float half = static_cast<float>(static_cast<float>(col[9] - col[5]) * 0.5 + 1.0);
    CVector a = NodePosition(from);
    CVector b = NodePosition(to);
    CVector diff;
    const CVector* d = VectorSub(&diff, &a, &b);
    float dist = static_cast<float>(std::sqrt(std::fabs(static_cast<double>(static_cast<float>(
        static_cast<double>(d->x) * d->x + static_cast<double>(d->y) * d->y)))));
    if (half > dist * 0.5) {
        fraction = 0.5f;
    } else {
        float margin = half / dist;
        if (!(margin < fraction)) {
            fraction = margin;
        }
        float other = 1.0f - margin;
        if (!(other > fraction)) {
            fraction = other;
        }
    }
    uint8_t* next = AutoPilot_GetNextLinkPosition(autopilot);
    uint8_t* current = AutoPilot_GetLinkPosition(autopilot);
    float d1x, d1y, d2x, d2y;
    PathPosition_GetDirection(current, &d1x, &d1y, static_cast<int8_t>(vehicle[0x2D5]));
    PathPosition_GetDirection(next, &d2x, &d2y, static_cast<int8_t>(vehicle[0x2D6]));
    CVector start, end;
    PathPosition_GetLanePoint(current, &start, d1x, d1y, static_cast<int8_t>(vehicle[0x2D7]), 5.0f);
    PathPosition_GetLanePoint(next, &end, d2x, d2y, static_cast<int8_t>(vehicle[0x2D8]), 5.0f);
    float length = CurveLength_428DA0(&start, &end, d1x, d1y, d2x, d2y);
    int32_t time = static_cast<int32_t>(static_cast<int64_t>(1.0 / At<float>(vehicle, 0x2E0) * 1000.0 * length));
    At<int32_t>(vehicle, 0x2B8) = time;
    float timeF = static_cast<float>(Unsigned(static_cast<uint32_t>(time)));
    int32_t startTime = static_cast<int32_t>(static_cast<int64_t>(Unsigned(m_snTimeInMilliseconds) - timeF * (fraction + 0.5)));
    At<int32_t>(vehicle, 0x2B4) = startTime;
    CVector dir1 = {d1x, d1y, 0.0f};
    CVector dir2 = {d2x, d2y, 0.0f};
    CVector speed = {0.0f, 0.0f, 0.0f};
    float t = static_cast<float>(1.0 / timeF * Unsigned(m_snTimeInMilliseconds - static_cast<uint32_t>(startTime)));
    CalcCurvePoint_428E70(&start, &end, &dir1, &dir2, t, time, &pos, &speed);
    At<float>(vehicle, 0x124) = static_cast<float>(speed.x * 0.03333333507180214);
    At<float>(vehicle, 0x128) = static_cast<float>(speed.y * 0.03333333507180214);
    At<float>(vehicle, 0x12C) = static_cast<float>(speed.z * 0.03333333507180214);
    CVector facing;
    a = NodePosition(from);
    b = NodePosition(to);
    VectorSub(&facing, &b, &a);
    facing.z = 0.0f;
    Vector_Normalise(&facing);
    float* m = Entity_GetMatrix(vehicle);
    m[4] = facing.x;
    m[5] = facing.y;
    m[6] = facing.z;
    m[7] = colW;
    m[0] = facing.y;
    m[1] = -facing.x;
    m[2] = 0.0f;
    m[8] = 0.0f;
    m[9] = 0.0f;
    m[10] = 1.0f;
    m[11] = colW;
    float baseZ = PositionOf(vehicle)->z;
    if (re::Flag(useZ)) {
        baseZ = z;
    }
    uint8_t found;
    float ground = FindGroundZFor3DCoord_4635A0(pos.x, pos.y, static_cast<float>(baseZ + 2.0), &found, 1);
    col = At<float*>(ms_modelInfoPtrs[At<int16_t>(vehicle, 0x10E)], 0xC);
    float finalZ = static_cast<float>(col[6] * -1.0 + (found ? ground : baseZ));
    Placeable_SetPosition(vehicle, pos.x, pos.y, finalZ);
}

} // namespace c01join

// ---- registration ----------------------------------------------------------------------

#define JOIN_REVERSED_C01(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace c01join {
JOIN_REVERSED_C01("CCarCtrl::JoinCarWithRoadSystem", 0x0041E370, &JoinCarWithRoadSystem);
JOIN_REVERSED_C01("CCarCtrl::JoinCarWithRoadSystemGotoCoors", 0x0041E6C0, &JoinCarWithRoadSystemGotoCoors);
JOIN_REVERSED_C01("CCarCtrl::FindLinksToGoWithTheseNodes", 0x0041BF30, &FindLinksToGoWithTheseNodes);
JOIN_REVERSED_C01("CCarCtrl::DragCarToPoint", 0x0041FA20, &DragCarToPoint);
JOIN_REVERSED_C01("CCarCtrl::PlaceCarOnCurve", 0x0041FF70, &PlaceCarOnCurve);
} // namespace c01join
