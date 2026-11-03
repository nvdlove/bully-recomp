#pragma once

// The ped paths' nodes and links (see PathNodes.cpp).

#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace paths {

// a node (0x18)
struct PathNode {
    re::Ptr32<void> vtable;          // +0 (CPathNode's: IArchivable)
    int16_t prev;          // +4 the search list's (below 0x200: a bucket at 0xC0695C; else a node + 0x200)
    int16_t next;          // +6
    int16_t pos[3];        // +8 x, y, z in eighths
    int16_t distance;      // +0xE (0x7FFE: none)
    int16_t firstLink;     // +0x10
    uint8_t width;         // +0x12
    uint8_t unknown13;     // +0x13
    uint8_t flags14;       // +0x14 bits 0-3 the links, 4 on a dead end, 5 switched off, 6 (0x43FA40 clears), 7 between levels
    uint8_t flags15;       // +0x15 bit 1 water, 2 under a bridge, 4-5 the speed
    uint8_t flags16;       // +0x16 bits 0-3 the density
    uint8_t unknown17;     // +0x17

    int NumLinks() const { return flags14 & 0xF; }
};
static_assert(sizeof(PathNode) == 0x18);
static_assert(offsetof(PathNode, firstLink) == 0x10);

// a navi link (0xC)
struct NaviLink {
    uint8_t unknown0[4];   // +0
    int16_t node;          // +4 the node it starts at
    int8_t dirX;           // +6 the direction in hundredths (0x43F970)
    int8_t dirY;           // +7
    uint8_t lanes;         // +8 bits 0-2 to, 3-5 from
    uint8_t unknown9;      // +9
    uint8_t width;         // +0xA
    uint8_t unknownB;      // +0xB
};
static_assert(sizeof(NaviLink) == 0xC);

// a link as saved (CPathLink)
struct PathLink {
    re::Ptr32<void> vtable;          // +0
    int32_t dest;          // +4
    int32_t src;           // +8
    float distance;        // +0xC
    re::Ptr32<NaviLink> navi;        // +0x10
};

GAME_ARRAY(PathNode, ms_nodes, 0x00BF7BF0);           // 1750
GAME_ARRAY(uint16_t, ms_linkNodes, 0x00C02000);       // bits 0-13 the node
GAME_ARRAY(int16_t, ms_naviLinkIds, 0x00C04BF2);
GAME_ARRAY(NaviLink, ms_naviLinks, 0x00BF6228);
GAME_VAR(int32_t, ms_numNodes, 0x00C06944);
GAME_VAR(int32_t, ms_numNaviLinks, 0x00C06950);
GAME_VAR(int16_t, ms_numLinksLoaded, 0x00C0694E);
GAME_VAR(int32_t, ms_unknownBF6210, 0x00BF6210);

inline int NodeIndex(const void* node) {
    return int((reinterpret_cast<intptr_t>(node) - 0x00BF7BF0) / 0x18);
}

inline PathNode* NodeById(uint32_t id) {
    return id <= 0x6D5 ? &ms_nodes[id] : nullptr;
}

} // namespace paths
