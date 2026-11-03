// CCarCtrl's next-node pickers for chasing a point and for driving around at random
// (GTA's PickNextNodeToChaseCar and PickNextNodeRandomly). The vehicle's autopilot is at
// +0x2A8: +0x2A8 / +0x2AC / +0x2B0 the current, next and previous nodes, +0x2B4 the
// curve's start time and +0x2B8 its length in ms, +0x2C0 / +0x2C4 / +0x2C8 the navi
// links, +0x2D4.. the directions (1 / -1) and +0x2D7 / +0x2D8 the lanes, +0x2DA the
// mission, +0x2E0 the cruise speed, +0x338 the random seed.
// (x87 extended precision in the game; what it stores as floats is kept as floats.)

#include <cmath>
#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace c01pick {

struct CVector {
    float x, y, z;
};

using re::At;

// 0x41ACE0 (cdecl; callers pass the target vehicle too, unused)
void __cdecl PickNextNodeToChaseCar(uint8_t* vehicle, float x, float y);
// 0x41DCC0 (cdecl)
void __cdecl PickNextNodeRandomly(uint8_t* vehicle);

} // namespace c01pick

namespace {

using namespace c01pick;

inline void* const thePaths = reinterpret_cast<void*>(0x00BF6228);
GAME_ARRAY(uint8_t, naviLinks, 0x00BF6228);   // 0xC each: +0 x, +2 y in eighths, +6 / +7 the direction in hundredths, +8 the lanes
GAME_ARRAY(uint8_t, pathNodes, 0x00BF7BF0);   // 0x18 each: +8 x, y, z in eighths, +0x14 the links (bits 0-3)

inline void __cdecl Srand(uint32_t seed) { crt::Srand((unsigned int)seed); }
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(uint8_t, __cdecl, RandomChance_41A5E0, 0x0041A5E0);   // (Rand() < 0x3FFF)
GAME_FN(float, __cdecl, GetATanOfXY, 0x00432FE0, float x, float y);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(void, __thiscall, Paths_DoPathSearch_x64, 0x00442FC0, void* paths, int type, float, float, float, int startNode, float, float, float, re::Ptr32<uint8_t>* nodes, int16_t* numNodes, int maxNodes, void* vehicle, float* distance, float maxDistance, int targetNode);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Paths_DoPathSearch(void* a0, int a1, CVector a2, int a3, CVector a4, re::Ptr32<uint8_t>* a5, int16_t* a6, int a7, void* a8, float* a9, float a10, int a11) {
    return Paths_DoPathSearch_x64(a0, a1, a2.x, a2.y, a2.z, a3, a4.x, a4.y, a4.z, a5, a6, a7, a8, a9, a10, a11);
}
GAME_FN(uint8_t*, __thiscall, AutoPilot_GetCurrentNode, 0x00417940, void* autopilot);
GAME_FN(uint8_t*, __thiscall, AutoPilot_GetNextNode, 0x00417920, void* autopilot);
GAME_FN(uint8_t*, __thiscall, AutoPilot_GetToLink, 0x004178E0, void* autopilot);
GAME_FN(void, __thiscall, AutoPilot_SetNextNode, 0x005AB1A0, void* autopilot, uintptr_t node);
GAME_FN(uint32_t, __thiscall, Node_GetIndex, 0x0043F8A0, void* node);
GAME_FN(uint32_t, __thiscall, Node_GetLinkedNode, 0x0043F7B0, void* node, int link);
GAME_FN(uint8_t*, __thiscall, Node_GetLinkNode, 0x0043F7E0, void* node, int link);
GAME_FN(int32_t, __thiscall, Node_GetNaviLinkId, 0x0043F820, void* node, int link);
GAME_FN(int32_t, __thiscall, Node_FindNaviLinkTo, 0x0043F850, void* node, int other);
GAME_FN(uint16_t, __thiscall, NaviLink_GetLanesTo, 0x0043F8F0, void* link, void* node);
GAME_FN(uint16_t, __thiscall, NaviLink_GetLanesFrom, 0x0043F930, void* link, void* node);
GAME_FN(float, __thiscall, NaviLink_GetWidth, 0x0043FA60, void* link);
GAME_FN(uint32_t, __cdecl, FindNextNode, 0x0041DC00, uint8_t* vehicle, uint8_t* current, uint8_t* previous, int link,
        uint8_t turns, int level);
GAME_FN(CVector*, __thiscall, Follower_GetFromLanePoint, 0x00418010, void* autopilot, CVector* out);
GAME_FN(CVector*, __thiscall, Follower_GetToLanePoint, 0x00418080, void* autopilot, CVector* out);
GAME_FN(void, __thiscall, Follower_GetFromDirection, 0x004180F0, void* autopilot, float* dx, float* dy);
GAME_FN(void, __thiscall, Follower_GetToDirection, 0x00418130, void* autopilot, float* dx, float* dy);
GAME_FN(float, __cdecl, CurveLength, 0x00428DA0, const CVector* a, const CVector* b, float dax, float day, float dbx,
        float dby);

CVector* PositionOf(void* placeable) {
    uint8_t* matrix = At<uint8_t*>(placeable, 0x14);
    return matrix ? reinterpret_cast<CVector*>(matrix + 0x30) : reinterpret_cast<CVector*>(static_cast<uint8_t*>(placeable) + 4);
}

// a node's (or navi link's) position in eighths, as floats
CVector NodePos(const uint8_t* node) {
    return {float(*reinterpret_cast<const int16_t*>(node + 8) * 0.125),
            float(*reinterpret_cast<const int16_t*>(node + 0xA) * 0.125),
            float(*reinterpret_cast<const int16_t*>(node + 0xC) * 0.125)};
}

// the links, directions and lanes shifted along to the new link
void ShiftLinks(uint8_t* vehicle) {
    At<int32_t>(vehicle, 0x2B4) += At<int32_t>(vehicle, 0x2B8);
    int32_t from = At<int32_t>(vehicle, 0x2C0);
    At<int32_t>(vehicle, 0x2C0) = At<int32_t>(vehicle, 0x2C4);
    At<int32_t>(vehicle, 0x2C8) = from;
    vehicle[0x2D4] = vehicle[0x2D5];
    vehicle[0x2D5] = vehicle[0x2D6];
    vehicle[0x2D7] = vehicle[0x2D8];
}

// the lane kept within the link's lanes, and 0 for a vehicle keeping to the first
// (+0x2EC 8)
void LimitLane(uint8_t* vehicle, int lanes) {
    float lane = float(int8_t(vehicle[0x2D8]));
    float last = float(double(lanes) - 1.0);
    float limited = last > lane ? lane : last;
    vehicle[0x2D8] = uint8_t(int32_t(limited));
    float value = float(int8_t(vehicle[0x2D8]));
    vehicle[0x2D8] = uint8_t(int32_t(value > 0.0f ? value : 0.0f));
    if (vehicle[0x2EC] & 8) {
        vehicle[0x2D8] = 0;
    }
}

// the time on the new curve: its length * trunc(1000 / cruise speed), at least 10
void SetCurveTime(uint8_t* vehicle, float length) {
    int32_t perUnit = int32_t(1000.0 / At<float>(vehicle, 0x2E0));
    int32_t time = int32_t(int64_t(double(length) * perUnit));
    At<int32_t>(vehicle, 0x2B8) = time;
    float timeF = float(double(uint32_t(time)));
    float clamped = timeF > 10.0f ? timeF : 10.0f;
    At<int32_t>(vehicle, 0x2B8) = int32_t(int64_t(clamped));
}

} // namespace

namespace c01pick {

// 0x41ACE0: the path searched from the next node towards (x, y); when it leads off
// the next node that's the one after, else the link pointing closest to (x, y) (not
// back where it came from, unless it's the only one). Then the lane (a random change on
// long links, not for missions 2, 4, 0xE, 0x10) and the curve's time, from the links'
// own positions and directions
void __cdecl PickNextNodeToChaseCar(uint8_t* vehicle, float x, float y) {
    if (At<uint16_t>(vehicle, 0x338) != 0) {
        Srand(At<uint16_t>(vehicle, 0x338));
    }
    int32_t prevNode = At<int32_t>(vehicle, 0x2A8);
    int32_t curNode = At<int32_t>(vehicle, 0x2AC);
    uint8_t* autopilot = vehicle + 0x2A8;
    AutoPilot_GetCurrentNode(autopilot);   // (the result unused)
    uint8_t* node = AutoPilot_GetNextNode(autopilot);
    CVector target = {x, y, 0.0f};
    CVector nodePos = NodePos(node);
    re::Ptr32<uint8_t> found[2] = {nullptr, nullptr};
    // (the count lives in the vehicle argument's slot: its low word until written)
    int16_t count = int16_t(reinterpret_cast<uintptr_t>(vehicle));
    float distance;
    Paths_DoPathSearch(thePaths, 0, nodePos, curNode, target, found, &count, 2, vehicle, &distance, 999999.875f, -1);
    uint32_t newNode = 0;
    int16_t link = 0;
    bool picked = false;
    if (count == 1 || count == 2) {
        uint8_t* next = nullptr;
        if (found[0] != node) {
            next = found[0];
        } else if (count == 2 && found[1] != node) {
            next = found[1];
        }
        if (next) {
            newNode = Node_GetIndex(next);
            while (Node_GetLinkNode(node, link) != next) {
                link++;
            }
            picked = true;
        }
    }
    if (!picked) {
        CVector* pos = PositionOf(vehicle);
        float dy = y - pos->y;
        float dx = x - pos->x;
        float targetAngle = GetATanOfXY(dx, dy);
        uint16_t numLinks = node[0x14] & 0xF;
        float best = 10.0f;
        for (int16_t i = 0; i < int16_t(numLinks); i++) {
            uint32_t linked = Node_GetLinkedNode(node, i);
            uint8_t* other = Node_GetLinkNode(node, i);
            if (int32_t(linked) == prevNode && int16_t(numLinks) > 1) {
                continue;
            }
            CVector from = NodePos(node);
            CVector to = NodePos(other);
            CVector d;
            VectorSub(&d, &to, &from);
            float diff = float(double(GetATanOfXY(d.x, d.y)) - targetAngle);
            while (3.1415927410125732 < diff) {
                diff = float(diff - 6.2831854820251465);
            }
            while (diff < -3.1415927410125732) {
                diff = float(diff + 6.2831854820251465);
            }
            diff = std::fabs(diff);
            if (!(best < diff)) {
                newNode = linked;
                best = diff;
                link = i;
            }
        }
    }

    At<int32_t>(vehicle, 0x2B0) = At<int32_t>(vehicle, 0x2A8);
    At<int32_t>(vehicle, 0x2A8) = At<int32_t>(vehicle, 0x2AC);
    AutoPilot_SetNextNode(autopilot, newNode);
    ShiftLinks(vehicle);
    int32_t info = Node_GetNaviLinkId(node, link);
    At<int32_t>(vehicle, 0x2C4) = info;
    int lanes;
    if (curNode < At<int32_t>(vehicle, 0x2AC)) {
        vehicle[0x2D6] = 0xFF;
        lanes = naviLinks[info * 0xC + 8] & 7;
    } else {
        vehicle[0x2D6] = 1;
        lanes = (naviLinks[info * 0xC + 8] >> 3) & 7;
    }
    // the directions, in hundredths, by which way along each link it goes
    uint8_t* fromLink = &naviLinks[At<int32_t>(vehicle, 0x2C0) * 0xC];
    uint8_t* toLink = &naviLinks[info * 0xC];
    double fromSign = int8_t(vehicle[0x2D5]);
    double toSign = int8_t(vehicle[0x2D6]);
    float fromDirX = float(int8_t(fromLink[6]) * 0.009999999776482582 * fromSign);
    float fromDirY = float(fromSign * (int8_t(fromLink[7]) * 0.009999999776482582));
    float toDirX = float(int8_t(toLink[6]) * 0.009999999776482582 * toSign);
    float toDirY = float(toSign * (0.009999999776482582 * int8_t(toLink[7])));
    // (the game tests the lanes for below 0 here: never)
    float toY = float(At<int16_t>(toLink, 2) * 0.125);
    float fromY = float(At<int16_t>(fromLink, 2) * 0.125);
    float toX = float(At<int16_t>(toLink, 0) * 0.125);
    float fromX = float(At<int16_t>(fromLink, 0) * 0.125);
    double ly = double(toY) - fromY;
    double lx = double(toX) - fromX;
    if (lx * lx + ly * ly > 49.0) {
        uint8_t mission = vehicle[0x2DA];
        if (mission != 2 && mission != 4 && mission != 0xE && mission != 0x10 && (Rand() & 0x600) == 0) {
            if (RandomChance_41A5E0()) {
                vehicle[0x2D8]++;
            } else {
                vehicle[0x2D8]--;
            }
        }
    }
    LimitLane(vehicle, lanes);

    // the curve from the old link's lane to the new one's
    fromLink = &naviLinks[At<int32_t>(vehicle, 0x2C0) * 0xC];
    float fromOffset = float((double(NaviLink_GetWidth(fromLink)) + int8_t(vehicle[0x2D7])) * 5.0);
    toLink = &naviLinks[At<int32_t>(vehicle, 0x2C4) * 0xC];
    float toOffset = float((double(NaviLink_GetWidth(toLink)) + int8_t(vehicle[0x2D8])) * 5.0);
    fromX = float(At<int16_t>(fromLink, 0) * 0.125);
    CVector from, to;
    from.x = float(fromX + double(fromOffset) * fromDirY);
    fromY = float(At<int16_t>(fromLink, 2) * 0.125);
    from.y = float(fromY - double(fromOffset) * fromDirX);
    from.z = 0.0f;
    toX = float(At<int16_t>(toLink, 0) * 0.125);
    to.x = float(toX + double(toOffset) * toDirY);
    toY = float(At<int16_t>(toLink, 2) * 0.125);
    to.y = float(toY - double(toOffset) * toDirX);
    to.z = 0.0f;
    SetCurveTime(vehicle, CurveLength(&from, &to, fromDirX, fromDirY, toDirX, toDirY));
}

// 0x41DCC0: from a random link on, the first node FindNextNode allows (by the lane:
// 4 in the first, 2 in the last, 1 with fewer than 3 or neither) that doesn't turn back
// more than 100 degrees, up to 4 levels of leniency; with none it turns back (status 3).
// Then the lane (a random change on links of 14 or more) and the curve's time
void __cdecl PickNextNodeRandomly(uint8_t* vehicle) {
    if (At<uint16_t>(vehicle, 0x338) != 0) {
        Srand(At<uint16_t>(vehicle, 0x338));
    }
    int32_t prevNode = At<int32_t>(vehicle, 0x2A8);
    int32_t curNode = At<int32_t>(vehicle, 0x2AC);
    uint8_t* autopilot = vehicle + 0x2A8;
    uint8_t* previous = AutoPilot_GetCurrentNode(autopilot);
    uint8_t* node = AutoPilot_GetNextNode(autopilot);
    int numLinks = node[0x14] & 0xF;
    uint8_t* toLink = AutoPilot_GetToLink(autopilot);
    uint16_t lanesFrom = NaviLink_GetLanesFrom(toLink, node);
    NaviLink_GetLanesTo(toLink, node);   // (the result unused)
    uint8_t turns = 0;
    if (vehicle[0x2D8] == 0) {
        turns = 4;
    }
    if (int8_t(vehicle[0x2D8]) == int16_t(lanesFrom) - 1) {
        turns |= 2;
    }
    if (int16_t(lanesFrom) < 3 || turns == 0) {
        turns |= 1;
    }
    At<int32_t>(vehicle, 0x2B0) = At<int32_t>(vehicle, 0x2A8);
    At<int32_t>(vehicle, 0x2A8) = At<int32_t>(vehicle, 0x2AC);
    int32_t chosenLink = -1;
    uint32_t chosenNode = 0xFFFFFFFF;
    if (numLinks > 1) {
        int start = Rand() % numLinks;
        CVector prevPos = NodePos(previous);
        CVector curPos = NodePos(node);
        CVector tmp;
        CVector dir = *VectorSub(&tmp, &curPos, &prevPos);
        dir.z = 0.0f;
        Vector_Normalise(&dir);
        for (int level = 0; level < 4; level++) {
            if (chosenLink != -1) {
                break;
            }
            int i = start;
            do {
                uint32_t found = FindNextNode(vehicle, node, previous, i, turns, level);
                if (found != 0xFFFFFFFF) {
                    chosenLink = i;
                    chosenNode = found;
                    // (the game also skips a null node: never for one in range)
                    if (found <= 0x6D5) {
                        CVector nextPos = NodePos(&pathNodes[found * 0x18]);
                        CVector d = *VectorSub(&tmp, &nextPos, &curPos);
                        d.z = 0.0f;
                        Vector_Normalise(&d);
                        float dot = float((double(d.y) * dir.y + double(d.x) * dir.x) + double(d.z) * dir.z);
                        if (dot > -0.17364799976348877f) {
                            break;
                        }
                    }
                }
                i = (i + 1) % numLinks;
            } while (i != start);
        }
    }

    ShiftLinks(vehicle);
    if (chosenNode != 0xFFFFFFFF) {
        AutoPilot_SetNextNode(autopilot, chosenNode);
        At<int32_t>(vehicle, 0x2C4) = Node_GetNaviLinkId(node, chosenLink);
    } else {
        // nowhere to go: back the way it came
        AutoPilot_SetNextNode(autopilot, prevNode);
        At<int32_t>(vehicle, 0x2C4) = Node_FindNaviLinkTo(node, prevNode);
        re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(vehicle);
        re::PtrCast<void(__thiscall*)(void*, int)>(vtable[0x18 / 4])(vehicle, 3);   // vf6 SetStatus
    }
    toLink = AutoPilot_GetToLink(autopilot);
    int lanes;
    if (curNode < At<int32_t>(vehicle, 0x2AC)) {
        vehicle[0x2D6] = 0xFF;
        lanes = (toLink[8] >> 3) & 7;
    } else {
        vehicle[0x2D6] = 1;
        lanes = toLink[8] & 7;
    }
    // (the game tests the lanes for below 0 here: never)
    if ((Rand() & 0x600) == 0) {
        CVector nextPos = NodePos(AutoPilot_GetNextNode(autopilot));
        CVector curPos = NodePos(AutoPilot_GetCurrentNode(autopilot));
        CVector d;
        VectorSub(&d, &nextPos, &curPos);
        float distSq = float(double(d.x) * d.x + double(d.y) * d.y);
        if (!(distSq < 196.0f)) {
            if (Rand() < 0x3FFF) {
                vehicle[0x2D8]++;
            } else {
                vehicle[0x2D8]--;
            }
        }
    }
    LimitLane(vehicle, lanes);

    CVector from, to;
    Follower_GetFromLanePoint(autopilot, &from);
    Follower_GetToLanePoint(autopilot, &to);
    float fromDX, fromDY, toDX, toDY;
    Follower_GetFromDirection(autopilot, &fromDX, &fromDY);
    Follower_GetToDirection(autopilot, &toDX, &toDY);
    SetCurveTime(vehicle, CurveLength(&from, &to, fromDX, fromDY, toDX, toDY));
}

} // namespace c01pick

// ---- registration ----------------------------------------------------------------------

#define PICK_REVERSED_C01(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

PICK_REVERSED_C01("CCarCtrl::PickNextNodeToChaseCar", 0x0041ACE0, &c01pick::PickNextNodeToChaseCar);
PICK_REVERSED_C01("CCarCtrl::PickNextNodeRandomly", 0x0041DCC0, &c01pick::PickNextNodeRandomly);
