// A ped's ally links (CPed +0x2F0, 0x54 bytes): the ped it follows (+4, whose own +0x2F4
// goes on up to the leader at the top) and the one following it (+8, on down through
// +0x2F8), how the follower keeps up (the state at +0x28), the health bar and radar blip
// shown for it. 0x4852E0 .. 0x485BE0.
//
// +0 the ped, +4 / +8 registered pointers (0x5D7F10 sets them), +0xC a flag, +0x10 a
// time the leader waits until, +0x14 / +0x15 flags, +0x18 / +0x1C times, +0x20 the
// health bar, +0x24 the radar blip, +0x28 the state (0 idle, 1 following, 2 waiting,
// 3 attacking), +0x2C, +0x30 a sub-object (+0 the ped, +4 a registered pointer).

#include <cstdint>

#include "reversed.h"

namespace {

struct V3 {
    float x, y, z;
};

using re::At;

V3* EntityPos(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<V3*>(matrix + 0x30) : &At<V3>(entity, 4);
}

// The ped the leader chain ends at (up through +0x2F4), nullptr without one
uint8_t* TopOf(uint8_t* first) {
    uint8_t* ped = first;
    while (ped && At<uint8_t*>(ped, 0x2F4)) {
        ped = At<uint8_t*>(ped, 0x2F4);
    }
    return ped;
}

GAME_VAR(uint32_t, PAL_timeInMs, 0x00C1A9B4);        // CTimer::m_snTimeInMilliseconds
GAME_VAR(uint8_t*, PAL_playerPed, 0x00C1AEA8);
GAME_VAR(void*, PAL_radar, 0x00C674CC);
GAME_VAR(float, PAL_pedRadius, 0x00CF311C);
GAME_VAR(V3, PAL_zeroOffset, 0x00BCC564);
// the offsets beside the leader (made on first use: +x, -x, +y)
GAME_VAR(uint32_t, PAL_offsetsMade, 0x00C2B1FC);
GAME_VAR(V3, PAL_offsetRight, 0x00C2B1F0);
GAME_VAR(V3, PAL_offsetLeft, 0x00C2B1E4);
GAME_VAR(V3, PAL_offsetAhead, 0x00C2B1D8);
// the action nodes checked (names of the player's moves)
inline void* const PAL_nodeBlock = reinterpret_cast<void*>(0x01B65530);
inline void* const PAL_nodeDismiss1 = reinterpret_cast<void*>(0x01B654FC);
inline void* const PAL_nodeDismiss2 = reinterpret_cast<void*>(0x01B65500);
inline void* const PAL_nodeWait = reinterpret_cast<void*>(0x01B654F0);
inline void* const PAL_actFiles = reinterpret_cast<void*>(0x00D02850);
inline const char* const PAL_allyDamagedNode =
    reinterpret_cast<const char*>(0x0090D2D0);   // "/Global/AI/Ally/AllyAttacker/AllyAttackerDamaged/AllyAttackerDamaged"
inline const char* const PAL_aiActFile = reinterpret_cast<const char*>(0x0090D318);   // "AI/AI.act"

GAME_FN(uint8_t, __thiscall, PAL_RouteIsValid, 0x005E7AF0, void* route);
GAME_FN(uint8_t, __thiscall, PAL_HandleIsValid, 0x005D80E0, void* handle);
GAME_FN(void*, __thiscall, PAL_HandleGetFile, 0x005D80B0, void* handle);
GAME_FN(void*, __thiscall, PAL_HandleGetNode, 0x005D9C20, void* handle);
GAME_FN(void*, __thiscall, PAL_Handle5D80D0, 0x005D80D0, void* handle);
GAME_FN(void*, __thiscall, PAL_ActFilesCurrent, 0x005DB960, void* files);
GAME_FN(void*, __thiscall, PAL_ActFilesFind, 0x005DB8F0, void* files, void* file);
GAME_FN(void, __thiscall, PAL_ActFilesSetCurrent, 0x005DBDA0, void* files, void* file);
GAME_FN(void, __cdecl, PAL_Node5D8270, 0x005D8270, void* node);
GAME_FN(void, __cdecl, PAL_Node5D8670, 0x005D8670, void* node, void*);
GAME_FN(void, __cdecl, PAL_Node5D85C0, 0x005D85C0, void* node, void*);
GAME_FN(void, __cdecl, PAL_Node5D82A0, 0x005D82A0, void* node, int, int);
GAME_FN(void, __thiscall, PAL_RefSet, 0x005D7F10, void* ref, void* entity);
GAME_FN(void, __thiscall, PAL_RadarRemoveBlip, 0x0054A940, void* radar, int blip);
GAME_FN(void, __thiscall, PAL_HealthBarSet, 0x005537E0, void* bar, void* a, void* b);
GAME_FN(int*, __thiscall, PAL_Ped475400, 0x00475400, void* ped, int* out, void* other);
GAME_FN(void*, __thiscall, PAL_QueueGetCurrent, 0x00470340, void* queue);
GAME_FN(uint8_t, __thiscall, PAL_Ped4765C0, 0x004765C0, void* ped);
GAME_FN(uint8_t, __thiscall, PAL_Ped496F30, 0x00496F30, void* ped, void* other);
GAME_FN(void, __thiscall, PAL_PedPlayNode, 0x004740E0, void* ped, const char* node, const char* file);
GAME_FN(void, __thiscall, PAL_Tether499A40, 0x00499A40, void* tether);
GAME_FN(void, __thiscall, PAL_Tether499A60, 0x00499A60, void* tether, const V3* pos, float radius);
GAME_FN(void, __thiscall, PAL_Tether499AB0, 0x00499AB0, void* tether, void* entity, float radius, const V3* offset);
GAME_FN(uint8_t, __thiscall, PAL_Tether4998F0, 0x004998F0, void* tether, float);
GAME_FN(uint8_t, __cdecl, PAL_IsPointClear5EAE60, 0x005EAE60, const V3* pos, float radius, uintptr_t);
GAME_FN(void, __thiscall, PAL_Obj4A4710, 0x004A4710, void* self);
GAME_FN(void, __thiscall, PAL_Sub489290, 0x00489290, void* self, void* ped);
GAME_FN(uint8_t, __thiscall, PAL_ControllerIsPlaying, 0x005F4800, void* controller, void* node, int, int);
GAME_FN(V3*, __thiscall, PAL_GetRight, 0x004736F0, void* ped, V3* out);
GAME_FN(float, __thiscall, PAL_Normalise, 0x00414E10, V3* v);
GAME_FN(V3*, __cdecl, PAL_VectorSub, 0x00414B70, V3* out, const V3* a, const V3* b);
GAME_FN(void*, __thiscall, PAL_GetMatrix, 0x0046DD20, void* entity);
// (0x0041A9A0 stores 16 bytes through `out`: called through re::CallVec16)
static inline V3* PAL_MultiplyPoint(V3* out, const void* matrix, const V3* v) { return re::CallVec16<0x0041A9A0, V3*>(out, matrix, v); }
GAME_FN(uint8_t, __cdecl, PAL_Route5E9F50, 0x005E9F50, void* route, V3* pos, int, uintptr_t, int, int, uintptr_t);
// (this file's, through the game's address)
GAME_FN(uint8_t, __thiscall, PAL_CopyRoute, 0x004852E0, void* self, void* out);
GAME_FN(void, __thiscall, PAL_Reapply485340, 0x00485340, void* self);
GAME_FN(void, __thiscall, PAL_RemoveBars, 0x00485430, void* self, void* ped);
GAME_FN(uint8_t, __thiscall, PAL_KeepsFollower, 0x004854B0, void* self, void* ped, int keep);
GAME_FN(uint8_t*, __thiscall, PAL_GetTop, 0x00485560, void* self);
GAME_FN(void, __thiscall, PAL_Release, 0x00485970, void* self, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, PAL_RemoveFollower, 0x004859D0, void* self, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, PAL_Leave, 0x00485A50, void* self);
GAME_FN(void, __thiscall, PAL_Bumped, 0x00485A80, void* self, uint8_t* ped);

GAME_FN(int, __thiscall, PAL_RadarAddBlip, 0x0054A5B0, void* radar, int kind, int handle, uint32_t colour, int, int, int);
GAME_FN(void, __thiscall, PAL_RadarSetBlipSprite, 0x00548BF0, void* radar, int blip, int sprite);
GAME_FN(void, __thiscall, PAL_RadarSetBlip548B60, 0x00548B60, void* radar, int blip, int on);
GAME_FN(void, __thiscall, PAL_HealthBar5514D0, 0x005514D0, void* bar, void* name);
GAME_FN(void, __thiscall, PAL_HealthBar551500, 0x00551500, void* bar, void* name);
GAME_FN(uint8_t*, __thiscall, PAL_PedGetModelInfo, 0x00477DC0, void* ped);
GAME_FN(void, __thiscall, PAL_PedB44_48F1D0, 0x0048F1D0, void* part, void* other, int, int, int);
GAME_FN(void, __thiscall, PAL_PedSetAttitude, 0x004731D0, void* ped, int pedType, int attitude);
GAME_VAR(void*, PAL_healthBar, 0x00C674D0);
GAME_VAR(uint8_t*, PAL_pedPoolObjects, 0x00C0F5F0);   // (CPools::ms_pPedPool: +0 objects, +4 flags, +0xC size)
GAME_FN(void, __thiscall, PAL_ShowFollower, 0x00485F90, void* self, uint8_t* ped);

uint8_t* Ally(uint8_t* ped) {
    return ped + 0x2F0;
}

void* IntelController(uint8_t* ped, uint32_t offset) {
    return At<uint8_t*>(ped, 0x2E0) + offset;
}

} // namespace

namespace c07pal {

// 0x4852E0 (a route at CPed +0xDAC, thiscall): +0x4BC's 5 dwords copied out, and whether
// they make a valid route
uint8_t __fastcall CopyRoute(uint8_t* self, re::Edx, uint32_t* out) {
    uint32_t* route = &At<uint32_t>(self, 0x4BC);
    out[0] = route[0];
    out[1] = route[1];
    out[2] = route[2];
    out[3] = route[3];
    out[4] = route[4];
    return PAL_RouteIsValid(route);
}

// 0x485320: follows someone
uint32_t __fastcall HasLeader(uint8_t* self, re::Edx) {
    return At<void*>(self, 4) != nullptr ? 1 : 0;
}

// 0x485330: someone follows it
uint32_t __fastcall HasFollower(uint8_t* self, re::Edx) {
    return At<void*>(self, 8) != nullptr ? 1 : 0;
}

// 0x485340: the ped's current action node (+0x18E8, with +0x141C) played again in its own
// act file
void __fastcall Reapply(uint8_t* self, re::Edx) {
    uint8_t* ped = At<uint8_t*>(self, 0);
    if (At<int32_t>(ped, 0x141C) == 0 || !PAL_HandleIsValid(ped + 0x18E8)) {
        return;
    }
    void* saved = PAL_ActFilesCurrent(PAL_actFiles);
    uint8_t* handle = At<uint8_t*>(self, 0) + 0x18E8;
    void* file = PAL_ActFilesFind(PAL_actFiles, PAL_HandleGetFile(handle));
    if (!file) {
        return;
    }
    PAL_ActFilesSetCurrent(PAL_actFiles, file);
    void* node = PAL_HandleGetNode(handle);
    PAL_Node5D8270(node);
    PAL_Node5D8670(node, PAL_Handle5D80D0(handle));
    uint8_t* owner = At<uint8_t*>(self, 0);
    void* value = reinterpret_cast<void*(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(owner))[0xAC / 4]))(owner);
    PAL_Node5D85C0(node, value);
    PAL_Node5D82A0(node, 1, 0);
    PAL_ActFilesSetCurrent(PAL_actFiles, saved);
}

// 0x4853F0: the constructor for a ped
uint8_t* __fastcall Construct(uint8_t* self, re::Edx, void* ped) {
    At<float>(self, 0x3C) = 0.0f;
    At<float>(self, 0x40) = 0.0f;
    At<void*>(self, 0) = ped;
    At<float>(self, 0x44) = 0.0f;
    At<void*>(self, 4) = nullptr;
    At<float>(self, 0x48) = 0.0f;
    At<void*>(self, 8) = nullptr;
    At<float>(self, 0x4C) = 0.0f;
    At<void*>(self, 0x34) = nullptr;
    At<float>(self, 0x50) = 0.0f;
    At<void*>(self, 0x30) = ped;
    PAL_RefSet(self + 0x34, nullptr);
    return self;
}

// 0x485430: the radar blip removed and `ped` taken off the health bar (gone when empty)
void __fastcall RemoveBars(uint8_t* self, re::Edx, void* ped) {
    PAL_RadarRemoveBlip(PAL_radar, At<int32_t>(self, 0x24));
    uint8_t* bar = At<uint8_t*>(self, 0x20);
    if (!bar) {
        return;
    }
    if (At<void*>(bar, 0xAC) == ped) {
        PAL_HealthBarSet(bar, nullptr, At<void*>(bar, 0xB0));
    }
    bar = At<uint8_t*>(self, 0x20);
    if (At<void*>(bar, 0xB0) == ped) {
        PAL_HealthBarSet(bar, At<void*>(bar, 0xAC), nullptr);
    }
    bar = At<uint8_t*>(self, 0x20);
    if (At<void*>(bar, 0xAC) == nullptr && At<void*>(bar, 0xB0) == nullptr) {
        reinterpret_cast<void(__thiscall*)(void*, int)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(bar))[0x14 / 4]))(bar, 0);
        At<void*>(self, 0x20) = nullptr;
    }
}

// 0x4854B0: whether the follower stays (`keep`, or a seated ped of a type other than 0, 7
// and 8 that 0x475400 rates 4 against this one's)
uint8_t __fastcall KeepsFollower(uint8_t* self, re::Edx, uint8_t* ped, int keep) {
    if (!ped) {
        return 0;
    }
    if (re::Flag(keep)) {
        return 1;
    }
    if (At<uint8_t>(ped, 0x1570) != 1) {
        return 0;
    }
    int type = At<int32_t>(ped, 0x1310);
    if (type == 0 || type == 8 || type == 7) {
        return 0;
    }
    int rating;
    return *PAL_Ped475400(ped, &rating, At<void*>(self, 0)) == 4 ? 1 : 0;
}

// 0x485500: `ped` is somewhere down the follower chain
uint8_t __fastcall IsFollowedBy(uint8_t* self, re::Edx, uint8_t* ped) {
    if (ped) {
        for (uint8_t* follower = At<uint8_t*>(self, 8); follower; follower = At<uint8_t*>(follower, 0x2F8)) {
            if (follower == ped) {
                return 1;
            }
        }
    }
    return 0;
}

// 0x485530: not the player itself, it follows the player somewhere up the chain
uint8_t __fastcall FollowsPlayer(uint8_t* self, re::Edx) {
    for (uint8_t* leader = At<uint8_t*>(self, 4); leader; leader = At<uint8_t*>(leader, 0x2F4)) {
        if (At<int32_t>(At<uint8_t*>(self, 0), 0x1310) != 13 && At<int32_t>(leader, 0x1310) == 13) {
            return 1;
        }
    }
    return 0;
}

// 0x485560: the leader at the top of the chain
uint8_t* __fastcall GetTop(uint8_t* self, re::Edx) {
    return TopOf(At<uint8_t*>(self, 4));
}

// 0x485590: `ped` is this one, or up or down its chain
uint8_t __fastcall IsInChain(uint8_t* self, re::Edx, uint8_t* ped) {
    if (!ped) {
        return 0;
    }
    if (ped == At<uint8_t*>(self, 0)) {
        return 1;
    }
    for (uint8_t* leader = At<uint8_t*>(self, 4); leader; leader = At<uint8_t*>(leader, 0x2F4)) {
        if (leader == ped) {
            return 1;
        }
    }
    for (uint8_t* follower = At<uint8_t*>(self, 8); follower; follower = At<uint8_t*>(follower, 0x2F8)) {
        if (follower == ped) {
            return 1;
        }
    }
    return 0;
}

// 0x4855E0: where to follow the top leader: beside it (on the side the follower is, or
// ahead when +0x358 says so), the route's point for that when there is one, else the
// leader's position. False with no leader or with +0x14EC set.
uint8_t __fastcall GetFollowPoint(uint8_t* self, re::Edx, V3* out) {
    if (!(PAL_offsetsMade & 1)) {
        PAL_offsetsMade |= 1;
        PAL_offsetRight = {PAL_pedRadius + 1.0f, 0.0f, 0.0f};
    }
    if (!(PAL_offsetsMade & 2)) {
        PAL_offsetsMade |= 2;
        PAL_offsetLeft = {-(1.0f + PAL_pedRadius), 0.0f, 0.0f};
    }
    if (!(PAL_offsetsMade & 4)) {
        PAL_offsetAhead.x = 0.0f;
        PAL_offsetsMade |= 4;
        PAL_offsetAhead.y = 1.0f;
        PAL_offsetAhead.z = 0.0f;
    }
    uint8_t* owner = At<uint8_t*>(self, 0);
    uint8_t* top = TopOf(At<uint8_t*>(owner, 0x2F4));
    if (!owner || !top || At<int32_t>(owner, 0x14EC) != 0) {
        return 0;
    }
    struct { V3 v; float pad; } worldPad16;   // (the game's transform writes a 4th float)
    V3& world = worldPad16.v;
    world = {0.0f, 0.0f, 0.0f};
    V3 offset = PAL_zeroOffset;
    uint8_t* from = owner;
    if (top != At<uint8_t*>(owner, 0x2F4)) {
        from = At<uint8_t*>(owner, 0x2F4);
    }
    V3 right;
    PAL_GetRight(top, &right);
    right.z = 0.0f;
    PAL_Normalise(&right);
    V3 toTop;
    PAL_VectorSub(&toTop, EntityPos(top), EntityPos(from));
    toTop.z = 0.0f;
    PAL_Normalise(&toTop);
    float side = toTop.y * right.y + right.x * toTop.x + toTop.z * right.z;
    if (At<uint8_t*>(self, 0) != from) {
        side = -side;
        uint8_t* first = At<uint8_t*>(self, 0);
        if (At<int32_t>(first, 0x360) != 0 && !PAL_Tether4998F0(first + 0x358, 0.0f)) {
            offset = PAL_offsetAhead;
        }
    }
    const V3& beside = (0.0f > side) ? PAL_offsetRight : PAL_offsetLeft;
    offset.x = offset.x + beside.x;
    offset.y = offset.y + beside.y;
    offset.z = offset.z + beside.z;
    PAL_MultiplyPoint(&world, PAL_GetMatrix(top), &offset);
    struct {
        uint32_t a;
        int32_t b;
        float c, d, e;
    } route = {0, -1, 0.0f, 0.0f, 0.0f};
    if (PAL_CopyRoute(At<uint8_t*>(self, 0) + 0xDAC, &route) && PAL_Route5E9F50(&route, &world, 0, 0, 0, 1, 0)) {
        *out = world;
        return 1;
    }
    *out = *EntityPos(top);
    return 1;
}

// 0x485900 (the objective queue, thiscall): the current objective if it's an attack (7)
void* __fastcall GetAttackObjective(void* self, re::Edx) {
    uint8_t* objective = static_cast<uint8_t*>(PAL_QueueGetCurrent(self));
    if (!objective) {
        return nullptr;
    }
    return At<int32_t>(objective, 8) == 7 ? objective : nullptr;
}

// 0x485920: back to following, nobody linked
void __fastcall Reset(uint8_t* self, re::Edx) {
    PAL_RemoveBars(self, At<void*>(self, 0));
    At<uint8_t>(self, 0xC) = 0;
    At<uint32_t>(self, 0x10) = 0;
    At<uint8_t>(self, 0x14) = 1;
    At<uint8_t>(self, 0x15) = 1;
    At<int32_t>(self, 0x28) = 1;
    At<int32_t>(self, 0x2C) = 0;
    At<uint32_t>(self, 0x18) = 0;
    At<uint32_t>(self, 0x1C) = 0;
    PAL_RefSet(self + 4, nullptr);
    PAL_RefSet(self + 8, nullptr);
    At<void*>(self, 0x20) = nullptr;
    At<int32_t>(self, 0x24) = -1;
}

// 0x485970: `ped` let go: off the bars, its node played again, its flags back, its tether
// (+0x358) dropped
void __fastcall Release(uint8_t* self, re::Edx, uint8_t* ped) {
    if (!ped) {
        return;
    }
    PAL_RemoveBars(self, ped);
    PAL_Reapply485340(Ally(ped));
    At<int32_t>(ped, 0x318) = 1;
    At<int32_t>(ped, 0x13C8) = 1;
    At<int32_t>(ped, 0x14D0) = 1;
    At<int32_t>(ped, 0x14EC) = 0;
    At<int32_t>(ped, 0x153C) = 0;
    if (At<int32_t>(ped, 0x360) != 0) {
        PAL_Tether499A40(ped + 0x358);
    }
}

// 0x4859D0: `ped` (and whoever follows it, first) unlinked when it's this one's follower
uint8_t __fastcall RemoveFollower(uint8_t* self, re::Edx, uint8_t* ped) {
    if (!ped) {
        return 0;
    }
    if (At<uint8_t*>(ped, 0x2F8)) {
        PAL_RemoveFollower(Ally(ped), At<uint8_t*>(ped, 0x2F8));
    }
    re::Ptr32<uint8_t>& follower = At<uint8_t*>(self, 8);
    if (follower != ped) {
        return 0;
    }
    PAL_RefSet(follower + 0x2F4, nullptr);
    PAL_Release(self, follower);
    PAL_RefSet(&follower, nullptr);
    return 1;
}

// 0x485A40: the follower (and the rest of the chain) unlinked
void __fastcall RemoveFollowers(uint8_t* self, re::Edx) {
    PAL_RemoveFollower(self, At<uint8_t*>(self, 8));
}

// 0x485A50: this one let go, and off its leader
uint8_t __fastcall Leave(uint8_t* self, re::Edx) {
    PAL_Release(self, At<uint8_t*>(self, 0));
    uint8_t* leader = At<uint8_t*>(self, 4);
    if (leader) {
        return PAL_RemoveFollower(Ally(leader), At<uint8_t*>(self, 0));
    }
    return 0;
}

// 0x485A80: bumped by `ped`: when that's the top leader, +0x2C 1; a second bump between
// 0.5 and 10 seconds after the last (of a ped 0x4765C0 accepts) makes it leave
void __fastcall Bumped(uint8_t* self, re::Edx, uint8_t* ped) {
    uint8_t* owner = At<uint8_t*>(self, 0);
    uint8_t* top = TopOf(At<uint8_t*>(owner, 0x2F4));
    if (!ped || !top || top != ped) {
        return;
    }
    At<int32_t>(self, 0x2C) = 1;
    if (PAL_Ped4765C0(owner) && PAL_timeInMs > At<uint32_t>(self, 0x18) + 500 &&
        PAL_timeInMs < At<uint32_t>(self, 0x18) + 10000) {
        At<int32_t>(self, 0x2C) = 2;
        PAL_Leave(self);
    }
    At<uint32_t>(self, 0x18) = PAL_timeInMs;
}

// 0x485B00: the followers `attacker` hurt play the damaged node; one bumped twice
// between 0.5 and 10 seconds leaves
void __fastcall FollowersAttacked(uint8_t* self, re::Edx, void* attacker) {
    if (!attacker) {
        return;
    }
    for (uint8_t* follower = At<uint8_t*>(self, 8); follower; follower = At<uint8_t*>(follower, 0x2F8)) {
        if (!PAL_Ped496F30(follower, attacker)) {
            continue;
        }
        At<int32_t>(follower, 0x31C) = 1;
        if (PAL_Ped4765C0(follower) && PAL_timeInMs > At<uint32_t>(self, 0x1C) + 500 &&
            PAL_timeInMs < At<uint32_t>(self, 0x1C) + 10000) {
            uint8_t* ally = Ally(follower);
            At<int32_t>(follower, 0x31C) = 2;
            PAL_Release(ally, At<uint8_t*>(ally, 0));
            if (At<uint8_t*>(ally, 4)) {
                PAL_RemoveFollower(Ally(At<uint8_t*>(ally, 4)), At<uint8_t*>(ally, 0));
            }
        }
        PAL_PedPlayNode(follower, PAL_allyDamagedNode, PAL_aiActFile);
        At<uint32_t>(self, 0x1C) = PAL_timeInMs;
    }
}

// 0x485BC0: the chain dropped both ways
void __fastcall Clear(uint8_t* self, re::Edx) {
    PAL_RemoveFollower(self, At<uint8_t*>(self, 8));
    PAL_RemoveBars(self, At<void*>(self, 0));
}

// 0x485BE0: a step of following the top leader
void __fastcall Update(uint8_t* self, re::Edx) {
    uint8_t* owner = At<uint8_t*>(self, 0);
    if (At<int32_t>(owner, 0x1310) != 13 && PAL_ControllerIsPlaying(IntelController(owner, 0x290), PAL_nodeBlock, 1, 1)) {
        return;
    }
    owner = At<uint8_t*>(self, 0);
    int32_t* action = At<int32_t*>(owner, 0x1D30);
    if (action) {
        if (*action == 0 || *action == 3) {
            return;
        }
        if (*action == 0x3E) {
            if (At<int32_t>(self, 0x28) == 2) {
                At<int32_t>(self, 0x2C) = 0;
            }
            At<int32_t>(self, 0x28) = 1;
        }
    }
    uint8_t* top = TopOf(At<uint8_t*>(owner, 0x2F4));
    if (top) {
        if (At<uint8_t*>(top, 0x155C) == owner) {
            if (PAL_Ped4765C0(owner) &&
                (PAL_ControllerIsPlaying(IntelController(At<uint8_t*>(self, 0), 0x258), PAL_nodeDismiss1, 1, 1) ||
                 PAL_ControllerIsPlaying(IntelController(At<uint8_t*>(self, 0), 0x258), PAL_nodeDismiss2, 1, 1))) {
                PAL_Leave(self);
                return;
            }
            PAL_Bumped(self, top);
        }
        bool attacking = false;
        if (At<int32_t>(self, 0x2C) != 2) {
            uint8_t* objective = static_cast<uint8_t*>(PAL_QueueGetCurrent(At<uint8_t*>(self, 0) + 0x5F0));
            if (objective && At<int32_t>(objective, 8) == 7) {
                attacking = true;
                if (At<int32_t>(self, 0x28) != 3) {
                    At<int32_t>(self, 0x28) = 3;
                }
            }
        }
        uint8_t* ped = At<uint8_t*>(self, 0);
        if (At<int32_t>(ped, 0x153C) != 0 && At<int32_t>(ped, 0x14D0) != 0) {
            At<int32_t>(ped, 0x14D0) = 0;
        }
        switch (At<uint32_t>(self, 0x28)) {
        case 0:
            ped = At<uint8_t*>(self, 0);
            if (At<int32_t>(ped, 0x360) != 1) {
                PAL_Tether499A60(ped + 0x358, EntityPos(ped), 4.0f);
            }
            break;
        case 1:
            if (!PAL_IsPointClear5EAE60(EntityPos(top), 0.4f, 0)) {
                ped = At<uint8_t*>(self, 0);
                if (At<int32_t>(ped, 0x360) != 0) {
                    PAL_Tether499A40(ped + 0x358);
                }
                PAL_Obj4A4710(At<uint8_t*>(self, 0) + 0x654);
                return;
            }
            ped = At<uint8_t*>(self, 0);
            if (At<int32_t>(ped, 0x360) != 2) {
                PAL_Tether499AB0(ped + 0x358, top, 4.0f, &PAL_zeroOffset);
            }
            if (At<uint8_t>(self, 0x15) && At<int32_t>(top, 0x548) == 0) {
                PAL_Sub489290(self + 0x30, PAL_GetTop(At<uint8_t*>(self, 0) + 0x2F0));
            }
            if (PAL_ControllerIsPlaying(IntelController(top, 0x258), PAL_nodeWait, 1, 1) ||
                (At<int32_t>(top, 0x1310) == 13 && At<uint8_t>(PAL_playerPed, 0x1E91) != 0)) {
                ped = At<uint8_t*>(self, 0);
                At<int32_t>(self, 0x28) = 2;
                At<uint32_t>(self, 0x10) = PAL_timeInMs + 30000;
                if (At<int32_t>(ped, 0x360) != 0) {
                    PAL_Tether499A40(ped + 0x358);
                }
                ped = At<uint8_t*>(self, 0);
                PAL_Tether499A60(ped + 0x358, EntityPos(ped), 20.0f);
            }
            break;
        case 2:
            if (!PAL_ControllerIsPlaying(IntelController(top, 0x258), PAL_nodeWait, 1, 1) &&
                (At<int32_t>(top, 0x1310) != 13 || At<uint8_t>(PAL_playerPed, 0x1E91) == 0)) {
                At<int32_t>(self, 0x2C) = 0;
                At<int32_t>(self, 0x28) = 1;
            } else if (PAL_Ped4765C0(At<uint8_t*>(self, 0)) && At<uint32_t>(self, 0x10) < PAL_timeInMs) {
                PAL_Leave(self);
            }
            break;
        case 3:
            if (attacking) {
                ped = At<uint8_t*>(self, 0);
                if (At<int32_t>(ped, 0x360) != 0) {
                    PAL_Tether499A40(ped + 0x358);
                }
            } else {
                At<int32_t>(self, 0x28) = 1;
                PAL_Tether499AB0(At<uint8_t*>(self, 0) + 0x358, top, 4.0f, &PAL_zeroOffset);
            }
            break;
        }
    }
    uint8_t* follower = At<uint8_t*>(self, 8);
    if (follower && !PAL_KeepsFollower(self, follower, At<uint8_t>(self, 0xC))) {
        PAL_RemoveFollower(self, At<uint8_t*>(self, 8));
    }
}

// 0x485F90: a follower of the player shown: a radar blip, and on the health bar's free
// side (with its model's name)
void __fastcall ShowFollower(uint8_t* self, re::Edx, uint8_t* ped) {
    if (!ped) {
        return;
    }
    // (the game reads the top's type through a null top when `ped` has no leader)
    uint8_t* top = At<uint8_t*>(ped, 0x2F4);
    while (top && At<uint8_t*>(top, 0x2F4)) {
        top = At<uint8_t*>(top, 0x2F4);
    }
    if (At<int32_t>(top, 0x1310) != 13) {
        return;
    }
    void* radar = PAL_radar;
    uint8_t* bar = static_cast<uint8_t*>(PAL_healthBar);
    PAL_RadarRemoveBlip(radar, At<int32_t>(self, 0x24));
    uint8_t* pool = PAL_pedPoolObjects;
    int slot = static_cast<int>((ped - At<uint8_t*>(pool, 0)) / At<int32_t>(pool, 0xC));
    int handle = (slot << 8) + At<uint8_t*>(pool, 4)[slot];
    At<int32_t>(self, 0x24) = PAL_RadarAddBlip(radar, 2, handle, 0, 1, 2, 0);
    PAL_RadarSetBlipSprite(radar, At<int32_t>(self, 0x24), 5);
    PAL_RadarSetBlip548B60(radar, At<int32_t>(self, 0x24), 1);
    if (!bar) {
        return;
    }
    uint8_t* info = PAL_PedGetModelInfo(At<void*>(self, 8));
    if (At<void*>(bar, 0xAC) == nullptr) {
        At<uint8_t*>(self, 0x20) = bar;
        At<int32_t>(ped, 0x135C) = 0;
        PAL_HealthBarSet(At<void*>(self, 0x20), At<void*>(self, 8), At<void*>(bar, 0xB0));
        uint8_t* shown = At<uint8_t*>(self, 0x20);
        reinterpret_cast<void(__thiscall*)(void*, int)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(shown))[0x14 / 4]))(shown, 1);
        PAL_HealthBar5514D0(At<void*>(self, 0x20), info + 0x98);
    } else {
        if (At<void*>(bar, 0xB0) != nullptr) {
            return;
        }
        At<uint8_t*>(self, 0x20) = bar;
        At<int32_t>(ped, 0x135C) = 0;
        PAL_HealthBarSet(At<void*>(self, 0x20), At<void*>(bar, 0xAC), ped);
        uint8_t* shown = At<uint8_t*>(self, 0x20);
        reinterpret_cast<void(__thiscall*)(void*, int)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(shown))[0x14 / 4]))(shown, 1);
        PAL_HealthBar551500(At<void*>(self, 0x20), info + 0x98);
    }
    uint8_t* shown = At<uint8_t*>(self, 0x20);
    At<uint8_t>(shown, 0xBB) = 0;
    At<uint8_t>(shown, 0xBA) = 0;
}

// 0x4860F0: `ped` made this one's follower (`keep`: whatever it is; else a seated one of
// a type other than 0, 7 and 8 rated 4): the old follower chain dropped, its links set,
// its tether on the top leader, attitudes to this one's type
uint8_t __fastcall AddFollower(uint8_t* self, re::Edx, uint8_t* ped, int keep) {
    if (!ped) {
        return 0;
    }
    if (!re::Flag(keep)) {
        if (At<uint8_t>(ped, 0x1570) != 1) {
            return 0;
        }
        int type = At<int32_t>(ped, 0x1310);
        if (type == 0 || type == 8 || type == 7) {
            return 0;
        }
        int rating;
        if (*PAL_Ped475400(ped, &rating, At<void*>(self, 0)) != 4) {
            return 0;
        }
    }
    for (uint8_t* follower = At<uint8_t*>(self, 8); follower; follower = At<uint8_t*>(follower, 0x2F8)) {
        if (follower == ped) {
            return 1;
        }
    }
    PAL_RemoveFollower(self, At<uint8_t*>(self, 8));
    PAL_RefSet(self + 8, ped);
    uint8_t* follower = At<uint8_t*>(self, 8);
    At<uint8_t>(follower, 0x2FC) = 0;
    At<uint32_t>(follower, 0x300) = 0;
    At<uint8_t>(follower, 0x304) = 1;
    At<uint8_t>(follower, 0x305) = 1;
    At<int32_t>(follower, 0x318) = 1;
    At<int32_t>(follower, 0x31C) = 0;
    At<uint32_t>(follower, 0x308) = 0;
    At<uint32_t>(follower, 0x30C) = 0;
    PAL_RefSet(At<uint8_t*>(self, 8) + 0x2F4, At<void*>(self, 0));
    follower = At<uint8_t*>(self, 8);
    uint8_t* top = TopOf(At<uint8_t*>(follower, 0x2F4));
    PAL_Tether499AB0(follower + 0x358, top, 4.0f, &PAL_zeroOffset);
    At<uint8_t>(At<uint8_t*>(self, 8), 0x3B4) = 1;
    At<uint8_t>(At<uint8_t*>(self, 8), 0x3B5) = 1;
    PAL_PedB44_48F1D0(At<uint8_t*>(self, 8) + 0xB44, At<void*>(self, 0), 7, 0, 0);
    PAL_PedSetAttitude(At<void*>(self, 8), At<int32_t>(At<uint8_t*>(self, 0), 0x1310), 4);
    At<int32_t>(At<uint8_t*>(self, 8), 0x13C8) = 1;
    At<uint8_t>(self, 0xC) = static_cast<uint8_t>(keep);
    PAL_ShowFollower(self, At<uint8_t*>(self, 8));
    return 1;
}

} // namespace c07pal

#define PAL_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PAL_REVERSED("PedRoute::CopyRoute", 0x004852E0, &c07pal::CopyRoute);
PAL_REVERSED("PedAlly::HasLeader", 0x00485320, &c07pal::HasLeader);
PAL_REVERSED("PedAlly::HasFollower", 0x00485330, &c07pal::HasFollower);
PAL_REVERSED("PedAlly::Reapply", 0x00485340, &c07pal::Reapply);
PAL_REVERSED("PedAlly::Construct", 0x004853F0, &c07pal::Construct);
PAL_REVERSED("PedAlly::RemoveBars", 0x00485430, &c07pal::RemoveBars);
PAL_REVERSED("PedAlly::KeepsFollower", 0x004854B0, &c07pal::KeepsFollower);
PAL_REVERSED("PedAlly::IsFollowedBy", 0x00485500, &c07pal::IsFollowedBy);
PAL_REVERSED("PedAlly::FollowsPlayer", 0x00485530, &c07pal::FollowsPlayer);
PAL_REVERSED("PedAlly::GetTop", 0x00485560, &c07pal::GetTop);
PAL_REVERSED("PedAlly::IsInChain", 0x00485590, &c07pal::IsInChain);
PAL_REVERSED("PedAlly::GetFollowPoint", 0x004855E0, &c07pal::GetFollowPoint);
PAL_REVERSED("ObjectiveQueue::GetAttackObjective", 0x00485900, &c07pal::GetAttackObjective);
PAL_REVERSED("PedAlly::Reset", 0x00485920, &c07pal::Reset);
PAL_REVERSED("PedAlly::Release", 0x00485970, &c07pal::Release);
PAL_REVERSED("PedAlly::RemoveFollower", 0x004859D0, &c07pal::RemoveFollower);
PAL_REVERSED("PedAlly::RemoveFollowers", 0x00485A40, &c07pal::RemoveFollowers);
PAL_REVERSED("PedAlly::Leave", 0x00485A50, &c07pal::Leave);
PAL_REVERSED("PedAlly::Bumped", 0x00485A80, &c07pal::Bumped);
PAL_REVERSED("PedAlly::FollowersAttacked", 0x00485B00, &c07pal::FollowersAttacked);
PAL_REVERSED("PedAlly::Clear", 0x00485BC0, &c07pal::Clear);
PAL_REVERSED("PedAlly::Update", 0x00485BE0, &c07pal::Update);
PAL_REVERSED("PedAlly::ShowFollower", 0x00485F90, &c07pal::ShowFollower);
PAL_REVERSED("PedAlly::AddFollower", 0x004860F0, &c07pal::AddFollower);
