// CCarCtrl steering and next-node picking for the chasing / path-following / random
// missions (GTA's SteerAICarBlockingPlayerForwardAndBack, PickNextNodeToChaseCar,
// PickNextNodeToFollowPath, PickNextNodeRandomly).
// (x87 extended precision in the game; what it stores as floats is kept as floats.)

#include <cmath>
#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace c01steer {

struct CVector {
    float x, y, z;
};
using re::At;

GAME_FN(CVector*, __cdecl, FindPlayerSpeed, 0x00445D60);
GAME_FN(void*, __cdecl, FindPlayerEntity, 0x00445D80);
GAME_FN(CVector*, __cdecl, FindPlayerCoors, 0x00446900, CVector* out);
GAME_FN(CVector*, __thiscall, Placeable_GetForward, 0x004154A0, void* placeable, CVector* out);
GAME_FN(float*, __thiscall, Entity_GetMatrix, 0x008B6120, void* entity);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);

CVector* PositionOf(void* placeable) {
    uint8_t* matrix = At<uint8_t*>(placeable, 0x14);
    return matrix ? reinterpret_cast<CVector*>(matrix + 0x30) : reinterpret_cast<CVector*>(static_cast<uint8_t*>(placeable) + 4);
}

// 0x41BBF0: GTA's SteerAICarBlockingPlayerForwardAndBack: no swerve; forward or back
// to where the player (a little ahead of it) will cross the vehicle's side line
void __cdecl SteerAICarBlockingPlayerForwardAndBack(uint8_t* vehicle, float* swerve, float* accel, float* brake, uint8_t* handbrake) {
    *swerve = 0.0f;
    *handbrake = 0;
    CVector player = *FindPlayerSpeed();
    CVector playerForward;
    const CVector* pf = Placeable_GetForward(FindPlayerEntity(), &playerForward);
    float aheadX = static_cast<float>(pf->x * 0.10000000149011612);
    float aheadY = static_cast<float>(0.10000000149011612 * pf->y);
    player.x = aheadX + player.x;
    player.y = player.y + aheadY;
    const float* m = Entity_GetMatrix(vehicle);
    CVector right = {m[0], m[1], m[2]};
    right.z = 0.0f;
    Vector_Normalise(&right);
    CVector forward;
    Placeable_GetForward(vehicle, &forward);
    forward.z = 0.0f;
    Vector_Normalise(&forward);
    CVector coors;
    CVector diff;
    VectorSub(&diff, FindPlayerCoors(&coors), PositionOf(vehicle));
    float diffAlongRight = static_cast<float>(static_cast<double>(right.y) * diff.y + static_cast<double>(diff.x) * right.x +
                                              static_cast<double>(right.z) * diff.z);
    float playerAlongRight = static_cast<float>(static_cast<double>(player.y) * right.y + static_cast<double>(right.x) * player.x +
                                                static_cast<double>(right.z) * 0.0);
    if (playerAlongRight == 0.0f) {
        playerAlongRight = 0.009999999776482582f;
    }
    float t = static_cast<float>(-diffAlongRight / static_cast<double>(playerAlongRight));
    if (t < 0.0f) {
        *accel = 0.0f;
        *brake = 0.0f;
        return;
    }
    const CVector& speed = At<CVector>(vehicle, 0x124);
    float speedAlong = static_cast<float>(static_cast<double>(speed.y) * forward.y + static_cast<double>(speed.x) * forward.x +
                                          static_cast<double>(speed.z) * forward.z);
    float playerAlong = static_cast<float>(static_cast<double>(forward.y) * player.y + static_cast<double>(forward.x) * player.x +
                                           static_cast<double>(forward.z) * 0.0);
    float projection = static_cast<float>(playerAlong * static_cast<double>(t) +
                                          (static_cast<double>(forward.z) * diff.z + static_cast<double>(diff.y) * forward.y +
                                           static_cast<double>(diff.x) * forward.x));
    float distance = static_cast<float>(projection - static_cast<double>(speedAlong) * t);
    if (distance > 0.0f) {
        float a = static_cast<float>(distance * 0.10000000149011612);
        if (a > 1.0) {
            a = 1.0f;
        }
        *accel = a;
        *brake = 0.0f;
        return;
    }
    if (speedAlong > 0.0f) {
        *accel = 0.0f;
        float b = static_cast<float>(-distance * 0.10000000149011612);
        if (b > 1.0) {
            b = 1.0f;
        }
        *brake = b;
        if (b > 0.949999988079071) {
            *handbrake = 1;
        }
        return;
    }
    float a = static_cast<float>(distance * 0.10000000149011612);
    if (a < -1.0) {
        a = -1.0f;
    }
    *accel = a;
    *brake = 0.0f;
}

GAME_ARRAY(uint8_t, pathNodes, 0x00BF7BF0);
GAME_ARRAY(uint8_t, thePathGraph, 0x00BF6228);
inline void* const ThePaths = reinterpret_cast<void*>(0x00BF6228);
inline void __cdecl Srand(uint32_t seed) { crt::Srand((unsigned int)seed); }
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(void, __thiscall, Paths_DoPathSearch_x64, 0x00442FC0, void* paths, int type, float, float, float, int startNode, float, float, float, re::Ptr32<uint8_t>* nodes, int16_t* numNodes, int maxNodes, void* vehicle, float* distance, float maxDistance, int targetNode);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Paths_DoPathSearch(void* a0, int a1, CVector a2, int a3, CVector a4, re::Ptr32<uint8_t>* a5, int16_t* a6, int a7, void* a8, float* a9, float a10, int a11) {
    return Paths_DoPathSearch_x64(a0, a1, a2.x, a2.y, a2.z, a3, a4.x, a4.y, a4.z, a5, a6, a7, a8, a9, a10, a11);
}
GAME_FN(void, __thiscall, AutoPilot_PopRoute, 0x004178B0, void* autopilot);
GAME_FN(void, __thiscall, AutoPilot_SetNextNode_5AB1A0, 0x005AB1A0, void* autopilot, uintptr_t node);
GAME_FN(uint8_t*, __thiscall, AutoPilot_GetCurrentNode, 0x00417940, void* autopilot);
GAME_FN(uint32_t, __thiscall, Node_GetIndex, 0x0043F8A0, void* node);
GAME_FN(uint32_t, __thiscall, Node_GetLinkedNode, 0x0043F7B0, void* node, int link);
GAME_FN(int32_t, __thiscall, Node_GetLinkInfo_43F820, 0x0043F820, void* node, int link);
GAME_FN(CVector*, __thiscall, Follower_GetFromLanePoint, 0x00418010, void* autopilot, CVector* out);
GAME_FN(CVector*, __thiscall, Follower_GetToLanePoint, 0x00418080, void* autopilot, CVector* out);
GAME_FN(void, __thiscall, Follower_GetFromDirection, 0x004180F0, void* autopilot, float* dx, float* dy);
GAME_FN(void, __thiscall, Follower_GetToDirection, 0x00418130, void* autopilot, float* dx, float* dy);
GAME_FN(float, __cdecl, CurveLength_428DA0, 0x00428DA0, const CVector* a, const CVector* b, float dax, float day, float dbx, float dby);

// the autopilot moved on to the next node: the previous/current/next nodes, the curve
// start time, the links, directions and lane shifted along (shared by the pickers)
void ShiftAutoPilot(uint8_t* vehicle, uint32_t nextNode) {
    At<uint32_t>(vehicle, 0x2B0) = At<uint32_t>(vehicle, 0x2A8);
    At<uint32_t>(vehicle, 0x2A8) = At<uint32_t>(vehicle, 0x2AC);
    AutoPilot_SetNextNode_5AB1A0(vehicle + 0x2A8, nextNode);
}
void ShiftLinks(uint8_t* vehicle) {
    At<int32_t>(vehicle, 0x2B4) += At<int32_t>(vehicle, 0x2B8);
    At<int32_t>(vehicle, 0x2C8) = At<int32_t>(vehicle, 0x2C0);
    At<int32_t>(vehicle, 0x2C0) = At<int32_t>(vehicle, 0x2C4);
    vehicle[0x2D4] = vehicle[0x2D5];
    vehicle[0x2D5] = vehicle[0x2D6];
    vehicle[0x2D7] = vehicle[0x2D8];
}
// the lane kept within the link's lanes (a random change on long links first)
void ChooseLane(uint8_t* vehicle, int lanes) {
    CVector from, to;
    Follower_GetFromLanePoint(vehicle + 0x2A8, &from);
    Follower_GetToLanePoint(vehicle + 0x2A8, &to);
    CVector diff;
    const CVector* d = VectorSub(&diff, &from, &to);
    float distSq = static_cast<float>(static_cast<double>(d->x) * d->x + static_cast<double>(d->y) * d->y);
    if (distSq > 49.0f && (Rand() & 0x600) == 0) {
        if (Rand() < 0x3FFF) {
            vehicle[0x2D8]++;
        } else {
            vehicle[0x2D8]--;
        }
    }
    float lane = static_cast<float>(static_cast<int8_t>(vehicle[0x2D8]));
    float last = static_cast<float>(lanes - 1.0);
    float limited = last > lane ? lane : last;
    vehicle[0x2D8] = static_cast<uint8_t>(static_cast<int>(limited));
    float value = static_cast<float>(static_cast<int8_t>(vehicle[0x2D8]));
    vehicle[0x2D8] = static_cast<uint8_t>(static_cast<int>(value > 0.0f ? value : 0.0f));
}
// the time on the new curve: its length * trunc(1000 / max speed), at least 10
void TimeCurve(uint8_t* vehicle) {
    float d1x, d1y, d2x, d2y;
    Follower_GetFromDirection(vehicle + 0x2A8, &d1x, &d1y);
    Follower_GetToDirection(vehicle + 0x2A8, &d2x, &d2y);
    CVector from, to;
    Follower_GetFromLanePoint(vehicle + 0x2A8, &from);
    Follower_GetToLanePoint(vehicle + 0x2A8, &to);
    float length = CurveLength_428DA0(&from, &to, d1x, d1y, d2x, d2y);
    int32_t perUnit = static_cast<int32_t>(1000.0 / At<float>(vehicle, 0x2E0));
    int32_t time = static_cast<int32_t>(static_cast<int64_t>(static_cast<double>(length) * perUnit));
    At<int32_t>(vehicle, 0x2B8) = time;
    float timeF = static_cast<float>(static_cast<double>(static_cast<uint32_t>(time)));
    float clamped = timeF > 10.0f ? timeF : 10.0f;
    At<int32_t>(vehicle, 0x2B8) = static_cast<int32_t>(static_cast<int64_t>(clamped));
}

// 0x41B770: GTA's PickNextNodeToFollowPath: the next node of the route to the target
// coordinates (searched when the route's empty; true when there's none)
uint8_t __cdecl PickNextNodeToFollowPath(uint8_t* vehicle) {
    if (At<uint16_t>(vehicle, 0x338) != 0) {
        Srand(At<uint16_t>(vehicle, 0x338));
    }
    int32_t curNode = At<int32_t>(vehicle, 0x2AC);
    int16_t* count = reinterpret_cast<int16_t*>(vehicle + 0x31C);
    re::Ptr32<uint8_t>* route = reinterpret_cast<re::Ptr32<uint8_t>*>(vehicle + 0x2FC);
    if (*count == 0) {
        Paths_DoPathSearch(ThePaths, 0, *PositionOf(vehicle), curNode, At<CVector>(vehicle, 0x2F0), route, count, 8, vehicle, nullptr,
                           999999.9f, -1);
        if (*count < 2) {
            return 1;
        }
        AutoPilot_PopRoute(vehicle + 0x2A8);
    }
    uint8_t* first = route[0];
    At<uint32_t>(vehicle, 0x2B0) = At<uint32_t>(vehicle, 0x2A8);
    At<uint32_t>(vehicle, 0x2A8) = At<uint32_t>(vehicle, 0x2AC);
    AutoPilot_SetNextNode_5AB1A0(vehicle + 0x2A8, Node_GetIndex(first));
    AutoPilot_PopRoute(vehicle + 0x2A8);
    ShiftLinks(vehicle);
    uint8_t* node = AutoPilot_GetCurrentNode(vehicle + 0x2A8);
    int16_t link = 0;
    while (static_cast<int32_t>(Node_GetLinkedNode(node, link)) != At<int32_t>(vehicle, 0x2AC)) {
        link++;
    }
    int32_t info = Node_GetLinkInfo_43F820(node, link);
    At<int32_t>(vehicle, 0x2C4) = info;
    uint8_t laneBits = thePathGraph[info * 0xC + 8];
    int lanes;
    if (curNode < At<int32_t>(vehicle, 0x2AC)) {
        vehicle[0x2D6] = 0xFF;
        lanes = laneBits & 7;
    } else {
        vehicle[0x2D6] = 1;
        lanes = (laneBits >> 3) & 7;
    }
    ChooseLane(vehicle, lanes);
    if (vehicle[0x2EC] & 8) {
        vehicle[0x2D8] = 0;
    }
    TimeCurve(vehicle);
    return 0;
}

} // namespace c01steer

// ---- registration ----------------------------------------------------------------------

#define STEER_REVERSED_C01(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace c01steer {
STEER_REVERSED_C01("CCarCtrl::SteerAICarBlockingPlayerForwardAndBack", 0x0041BBF0, &SteerAICarBlockingPlayerForwardAndBack);
STEER_REVERSED_C01("CCarCtrl::PickNextNodeToFollowPath", 0x0041B770, &PickNextNodeToFollowPath);
} // namespace c01steer
