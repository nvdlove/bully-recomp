// CCarCtrl on the road graph: the turn at a node (from the previous node through the
// current to the next), the next node allowed through a link, which next-node picker a
// mission uses. Nodes are 0x18 bytes at 0xBF7BF0 (1750; short x, y, z at +8 in 1/8
// units; +0x10 the first link, +0x14 flags and link count, +0x15 speed bits).
// (x87 extended precision in the game; what it stores as floats is kept as floats.)

#include <cmath>
#include <cstdint>

#include "reversed.h"

namespace c01roads {

struct CVector {
    float x, y, z;
};
using re::At;

GAME_ARRAY(uint8_t, pathNodes, 0x00BF7BF0);
constexpr uint32_t NUM_PATH_NODES = 0x6D6;

GAME_FN(uint32_t, __thiscall, Node_GetLinkedNode, 0x0043F7B0, void* node, int link);
GAME_FN(uint8_t*, __thiscall, Node_GetLink, 0x0043FE40, void* node, int link);
GAME_FN(uint8_t, __thiscall, Link_GetLanes, 0x0043F8F0, void* link, void* fromNode);
GAME_FN(uint8_t, __cdecl, GetTurnDirection, 0x0041AB20, const uint8_t* previous, const uint8_t* current, const uint8_t* next);
GAME_FN(uint8_t*, __thiscall, Link_GetNode4, 0x00417920, void* link);
GAME_FN(CVector*, __cdecl, FindPlayerCoors, 0x00446900, CVector* out);
GAME_FN(uint8_t*, __cdecl, FindPlayerVehicle, 0x00446970, uintptr_t);
GAME_FN(void, __cdecl, PickNextNodeToChaseCar_41ACE0, 0x0041ACE0, void* vehicle, float x, float y, void* target);
GAME_FN(uint8_t, __cdecl, PickNextNodeToFollowPath_41B770, 0x0041B770, void* vehicle);
GAME_FN(void, __cdecl, PickNextNodeRandomly_41DCC0, 0x0041DCC0, void* vehicle);

float Sqrt(float value) {
    return static_cast<float>(std::sqrt(static_cast<double>(value)));
}
CVector* PositionOf(void* placeable) {
    uint8_t* matrix = At<uint8_t*>(placeable, 0x14);
    return matrix ? reinterpret_cast<CVector*>(matrix + 0x30) : reinterpret_cast<CVector*>(static_cast<uint8_t*>(placeable) + 4);
}

// 0x41AB20: the turn at `current`: 0 degenerate, 1 straight on, 2 and 4 the two ways
// round (the cross of the unit directions beyond -+0.77)
uint8_t __cdecl GetTurnDirectionImpl(const uint8_t* previous, const uint8_t* current, const uint8_t* next) {
    float cx = static_cast<float>(At<int16_t>(const_cast<uint8_t*>(current), 8) * 0.125);
    float px = static_cast<float>(At<int16_t>(const_cast<uint8_t*>(previous), 8) * 0.125);
    float d1x = cx - px;
    float cy = static_cast<float>(At<int16_t>(const_cast<uint8_t*>(current), 0xA) * 0.125);
    float py = static_cast<float>(At<int16_t>(const_cast<uint8_t*>(previous), 0xA) * 0.125);
    float d1y = cy - py;
    float nx = static_cast<float>(At<int16_t>(const_cast<uint8_t*>(next), 8) * 0.125);
    float d2x = nx - cx;
    float ny = static_cast<float>(At<int16_t>(const_cast<uint8_t*>(next), 0xA) * 0.125);
    float d2y = ny - cy;
    float len1 = Sqrt(std::fabs(static_cast<float>(static_cast<double>(d1x) * d1x + static_cast<double>(d1y) * d1y)));
    if (len1 == 0.0f) {
        return 0;
    }
    float n1x = d1x / len1;
    float n1y = d1y / len1;
    float len2 = Sqrt(std::fabs(static_cast<float>(static_cast<double>(d2x) * d2x + static_cast<double>(d2y) * d2y)));
    if (len2 == 0.0f) {
        return 0;
    }
    float n2y = d2y / len2;
    float n2x = d2x / len2;
    float cross = static_cast<float>(static_cast<double>(n2y) * n1x - static_cast<double>(n2x) * n1y);
    if (cross > 0.7699999809265137f) {
        return 4;
    }
    return cross < -0.7699999809265137f ? 2 : 1;
}

// 0x41DC00: the node through link `link` of `current`, if the vehicle may go there
// (not back, lanes that way, the flag classes 0x60 / 0x80 / 0x10 matching the
// previous node's by `level`, the turn in `turns` below level 2); -1 if not
uint32_t __cdecl FindNextNode(uint8_t* vehicle, uint8_t* current, uint8_t* previous, int link, uint8_t turns, int level) {
    uint32_t index = Node_GetLinkedNode(current, link);
    uint8_t* next = index < NUM_PATH_NODES ? pathNodes + index * 0x18 : nullptr;
    if (next == previous) {
        return 0xFFFFFFFF;
    }
    uint8_t* linkData = Node_GetLink(current, link);
    uint8_t turn = GetTurnDirection(previous, current, next);
    if (!Link_GetLanes(linkData, current)) {
        return 0xFFFFFFFF;
    }
    uint8_t flags = next[0x14];
    if ((flags & 0x60) && !(previous[0x14] & 0x60)) {
        return 0xFFFFFFFF;
    }
    if ((vehicle[0x2EC] & 4) && (flags & 0x80) && !(previous[0x14] & 0x80)) {
        return 0xFFFFFFFF;
    }
    if (level <= 0 && (flags & 0x10) && !(previous[0x14] & 0x10)) {
        return 0xFFFFFFFF;
    }
    if (level <= 1 && !(turns & turn)) {
        return 0xFFFFFFFF;
    }
    return index;
}

// 0x41F8D0: the next node picked by the vehicle's mission (the speed mode from the
// node it's heading to first)
uint8_t __cdecl PickNextNode(uint8_t* vehicle) {
    uint8_t* node = Link_GetNode4(vehicle + 0x2A8);
    vehicle[0x2E5] = (node[0x15] >> 4) & 3;
    switch (static_cast<int8_t>(vehicle[0x2DA])) {
    case 2:
    case 4: {
        CVector a, b;
        float x = FindPlayerCoors(&a)->x;
        uint8_t* target = FindPlayerVehicle(0);
        float y = FindPlayerCoors(&b)->y;
        PickNextNodeToChaseCar_41ACE0(vehicle, x, y, target);
        return 0;
    }
    case 8:
    case 0xC:
        return PickNextNodeToFollowPath_41B770(vehicle);
    case 0xE:
    case 0x10: {
        uint8_t* target = At<uint8_t*>(vehicle, 0x324);
        CVector* pos = PositionOf(target);
        PickNextNodeToChaseCar_41ACE0(vehicle, pos->x, pos->y, target);
        return 0;
    }
    case 0x1A:
    case 0x1B: {
        CVector* pos = PositionOf(At<uint8_t*>(vehicle, 0x328));
        PickNextNodeToChaseCar_41ACE0(vehicle, pos->x, pos->y, nullptr);
        return 0;
    }
    default:
        PickNextNodeRandomly_41DCC0(vehicle);
        return 0;
    }
}

} // namespace c01roads

// ---- registration ----------------------------------------------------------------------

#define ROADS_REVERSED_C01(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace c01roads {
ROADS_REVERSED_C01("CCarCtrl::GetTurnDirection", 0x0041AB20, &GetTurnDirectionImpl);
ROADS_REVERSED_C01("CCarCtrl::FindNextNode", 0x0041DC00, &FindNextNode);
ROADS_REVERSED_C01("CCarCtrl::PickNextNode", 0x0041F8D0, &PickNextNode);
} // namespace c01roads
