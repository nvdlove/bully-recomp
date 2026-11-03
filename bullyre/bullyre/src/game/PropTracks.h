#pragma once

// Shared by the action tree's prop, bike and weapon tracks of 0x655660..0x65E9E0
// (PropTracks*.cpp, BikeTracks.cpp, WeaponConditions*.cpp, WeaponTracks*.cpp): a node's
// transform, the owner's prop, and the name key helpers their field defaults use.

#include <cstdint>

#include "game/ActionTracksCommon.h"

namespace proptr {

using actc::At;
using actc::V3;

// a node's transform (NiTransform: the callers set the scale to 1 before asking)
struct NodeTransform {
    float m[9];   // +0x0
    V3 pos;       // +0x24
    float scale;  // +0x30
};
static_assert(sizeof(NodeTransform) == 0x34);

GAME_FN(uint8_t*, __cdecl, UsePoint_GetEntity, 0x004EABB0, uint8_t* owner);   // the owner's prop
GAME_FN(uint8_t, __cdecl, GetNodeTransform, 0x00467D80, uint8_t* entity, int node, NodeTransform* transform, int flag);

// a name key field set to none raw (the temporary's value copied, not assigned)
inline void SetNoneRaw(uint8_t* self, int at) {
    uint32_t name;
    actc::NameKey_FromValue(&name, 0);
    At<uint32_t>(self, at) = name;
    actc::NameKey_Destruct(&name);
}

} // namespace proptr
