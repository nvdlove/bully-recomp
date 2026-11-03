// The action tree's tracks of 0x61EAC0..0x621660: punishment points to the player, the
// wander / circulate / ride-around tracks (path nodes, the goal ahead, the ride-around
// passing and chasing), halting a vehicle, fleeing, the POI tracks (hanging out with the
// group's chatter, spectating, dynamic POIs, moving to, sitting down), facing (a
// direction, the stimulus, the target, the vehicle's door, the dive entity, the hit), the
// look-around track, moving to a stimulus and investigating an area.

#include <cmath>
#include <cstdint>

#include "game/ActionTracksCommon.h"
#include "game/TrackMath.h"

#include "game/Crt.h"
#include "reversed.h"
using namespace actc;

namespace {

// A line segment (0x20: start and end, each padded)
struct Segment {
    V3 start;   // +0x0
    float w0;   // +0xC
    V3 end;     // +0x10
    float w1;   // +0x1C
};
static_assert(sizeof(Segment) == 0x20);

// A nav mesh point (0x14)
struct NavPoint {
    re::Ptr32<uint8_t> col;   // +0x0
    int32_t tri;    // +0x4
    V3 pos;         // +0x8
};
static_assert(sizeof(NavPoint) == 0x14);

// A speech event (0x30): +0 bits 0..8 the line, 9..16 its speaker
struct SpeechEvent {
    uint32_t w0;         // +0x0
    uint32_t w1;         // +0x4 bits 0..10 its variant
    uint8_t pad8[8];     // +0x8
    uint32_t w10;        // +0x10
    uint8_t pad14[0x1C]; // +0x14
};
static_assert(sizeof(SpeechEvent) == 0x30);

GAME_FN(uint8_t*, __thiscall, Targeting_GetLookTarget, 0x004D7780, uint8_t* targeting);
GAME_FN(V3*, __cdecl, VectorAdd, 0x004129A0, V3* out, const float* a, const float* b);
GAME_FN(V3*, __cdecl, VectorScale, 0x00412350, V3* out, const V3* v, float s);
GAME_FN(V3*, __cdecl, Normalised, 0x00472D40, V3* out, const V3* v);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const V3* a, const V3* b);
GAME_FN(float, __cdecl, DistanceSquared, 0x00472D00, const V3* a, const V3* b);
GAME_FN(float, __cdecl, Distance, 0x0041A950, const float* a, const float* b);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const V3* v);
GAME_FN(float, __thiscall, Vector_MagnitudeSqr, 0x0040D9F0, const V3* v);
GAME_FN(V3*, __cdecl, FindPlayerSpeed, 0x00445D60);
GAME_FN(uint8_t*, __thiscall, Entity_GetMatrix, 0x008B6120, uint8_t* entity);
GAME_FN(V3*, __thiscall, Entity_GetRight, 0x004736F0, uint8_t* entity, V3* out);
GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_FN(float, __thiscall, Entity_GetHeading, 0x00415470, uint8_t* entity);
GAME_FN(void, __thiscall, Ped_SetHeading, 0x00475730, uint8_t* ped, float heading);
GAME_FN(uint8_t, __cdecl, GetIsLineOfSightClear, 0x004617D0, const V3* a, const V3* b, int flags);
GAME_FN(int32_t, __thiscall, Ped_GetStat, 0x00477060, uint8_t* ped, int32_t stat);
GAME_FN(int32_t*, __thiscall, Ped_GetAttitude, 0x00475380, uint8_t* ped, int32_t* out, int32_t pedType);
GAME_FN(uint8_t*, __thiscall, Ped_FindByType, 0x00477C00, uint8_t* ped, int32_t type, int);
GAME_FN(uint8_t, __thiscall, Ped_EveryNthFrame, 0x00465A60, uint8_t* ped, uint32_t n);
GAME_FN(uint8_t*, __thiscall, Ped_GetLastHitWeapon, 0x00474BE0, uint8_t* ped);
GAME_FN(float, __cdecl, HeadingFromVector, 0x00433310, const V3* v);
GAME_FN(float, __cdecl, LimitAngle, 0x00432EE0, float angle);
GAME_FN(V3*, __cdecl, Vector_FromAngle, 0x00432DF0, V3* out, float angle);
GAME_FN(V3*, __cdecl, AngleToVector, 0x00432E30, V3* out, float angle);
GAME_FN(uint8_t, __thiscall, Steering_GoTo, 0x004A5000, uint8_t* steering, float x, float y, float z, uint32_t flags,
        float speed, float radius, uint8_t* entity, int flag);
GAME_FN(uint8_t, __thiscall, Steering_GoToWith, 0x004A4910, uint8_t* steering, float x, float y, float z, int unused,
        uint32_t flags, float speed, float radius, float a, float b);
GAME_FN(uint8_t, __thiscall, Steering_IsDD4One, 0x004A48D0, uint8_t* steering);
GAME_FN(void, __thiscall, ColLine_Set, 0x004D5A60, Segment* line, float sx, float sy, float sz, float ex, float ey,
        float ez);
GAME_FN(V3*, __thiscall, Segment_PointAhead, 0x005E76C0, const Segment* self, V3* out, const V3* p, float radius,
        float* t);
GAME_FN(uint8_t*, __thiscall, Paths_FindNodeInDirection, 0x00441B60, void* paths, uint8_t* ped, float x, float y,
        float z, float dx, float dy, float dz, float angle, int kind, float maxDist, int, int, int, int);
GAME_FN(uint8_t*, __thiscall, Paths_FindNodeClosestToCoors, 0x00441950, void* paths, uint8_t* ped, float x, float y,
        float z, int kind, float maxDist, int, int, int, int);
GAME_FN(V3*, __thiscall, Node_GetPosition, 0x00417C00, const uint8_t* node, V3* out);
GAME_FN(uint8_t*, __thiscall, Node_GetLinkNode, 0x0043F7E0, uint8_t* node, int link);
GAME_FN(uint8_t*, __thiscall, Node_FindNextNode, 0x0043FE80, uint8_t* node, uint8_t* previous, uint8_t* ped);
GAME_FN(V3*, __cdecl, GetPointOnNode, 0x004422C0, V3* out, const uint8_t* node, int packed);
GAME_FN(uint8_t, __cdecl, RouteBetweenPositions, 0x005E26A0, const V3* start, const V3* goal, int flags,
        uint8_t* route, float maxDist);
GAME_FN(uint8_t, __cdecl, RouteBetween, 0x005E2620, const NavPoint* start, const NavPoint* goal, int flags,
        uint8_t* route, float maxDist);
GAME_FN(uint8_t, __cdecl, FindNavPoint, 0x005EB020, const V3* pos, NavPoint* out, void* entity, uint8_t* offMesh);
GAME_FN(uint8_t, __thiscall, NavPoint_IsValid, 0x005E7AF0, const uint8_t* point);
GAME_FN(uint8_t, __thiscall, Navigator_FindCurrent, 0x005E2B20, uint8_t* navigator);
GAME_FN(uint8_t*, __thiscall, Queue_GetCurrent, 0x00470340, uint8_t* queue);
GAME_FN(void, __thiscall, Queue_RemoveFirst, 0x004702C0, uint8_t* queue);
GAME_FN(uint8_t, __thiscall, Queue_Prepend, 0x00471470, uint8_t* queue, uint8_t* objective);
GAME_FN(void, __thiscall, EntityRef_Set, 0x00478550, re::Ptr32<uint8_t>* ref, uint8_t* entity);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, uint8_t* entity, re::Ptr32<uint8_t>* ref);
GAME_FN(void, __thiscall, SetRef, 0x005D7F10, re::Ptr32<uint8_t>* ref, uint8_t* entity);
GAME_FN(void, __thiscall, EventMgr_Send, 0x004A6670, void* self, int event, uint8_t* ped, uint8_t* other,
        const V3* pos, float, float);
GAME_FN(uint8_t*, __thiscall, POIManager_AddDynamic, 0x00448E00, void* manager, float radius, float x, float y,
        float z, float heading);
GAME_FN(uint8_t*, __thiscall, POIManager_FindByPed, 0x00448460, uint8_t* manager, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, POI_Join, 0x00447410, uint8_t* poi, uint8_t* ped, int flag);
GAME_FN(void, __thiscall, POI_Release, 0x004475D0, uint8_t* poi, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Group_ContainsPed, 0x00448300, uint8_t* group, uint8_t* ped);
GAME_FN(int32_t, __thiscall, PedGroup_CountMembers, 0x00489700, uint8_t* group);
GAME_FN(uint8_t*, __thiscall, PedGroup_GetMember, 0x00489730, uint8_t* group, int n);
GAME_FN(SpeechEvent*, __thiscall, SpeechEvent_Reset, 0x004894D0, SpeechEvent* event);
GAME_FN(uint8_t, __thiscall, Speech_IsPedSayingEvent, 0x005A08F0, void* speech, uint8_t* ped, uint32_t id,
        uint32_t variant);
GAME_FN(uint8_t, __thiscall, Speech_PassesPedFlag, 0x005A21B0, void* speech, uint8_t* ped, int id);
GAME_FN(SpeechEvent*, __thiscall, Speech_Add, 0x005A4920, void* speech, SpeechEvent* out, uint8_t* ped,
        uint8_t* group, int, int event, int, int, int, int, int, uintptr_t);
GAME_FN(uint8_t, __cdecl, IsSpeechBusy, 0x005A2900, uint8_t* ped, int event, int comment);
inline void* __cdecl memset_(void* p, int value, size_t size) { return crt::Memset(p, value, size); }
GAME_FN(float, __cdecl, RandomFloat, 0x0041A5B0, float low, float high);
GAME_FN(int, __cdecl, RandomRange, 0x0040DA80, int least, int most);
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(uint8_t*, __thiscall, MoveToStimulusObjective_Construct, 0x004707D0, void* self, uint8_t* ped,
        uint8_t* stimulus);
GAME_FN(int32_t, __thiscall, Stimulus_GetPriority, 0x004A5CF0, uint8_t* stimulus);
GAME_FN(uint8_t, __thiscall, Damage_GetDoorStatus, 0x004C7E00, void* damage, int door);
GAME_FN(uint8_t, __thiscall, ActionInputs_IsHeld, 0x005F3700, uint8_t* inputs, int button);
GAME_FN(void, __thiscall, ActionInputs_SetRequest, 0x005F3A90, uint8_t* inputs, int button, int kind, float time);
GAME_FN(uint32_t*, __thiscall, NameKey_Copy, 0x005FA880, uint32_t* key, const uint32_t* from);
GAME_FN(void, __thiscall, HashPath_Set, 0x005FA8C0, int32_t* path, int32_t count, uint32_t* hashes);
GAME_FN(uint8_t*, __cdecl, FindBoneInfo, 0x0072AEA0, int32_t* path, void* names);
GAME_FN(void, __stdcall, DestructArray, 0x004017C0, void* array, size_t size, int count, void* destructor);
GAME_FN(void, __thiscall, HUDDetention_SetText, 0x0053D650, void* hud, const char* text, int value, int force);

GAME_VAR(void*, events, 0x00C2C1A0);
GAME_VAR(uint8_t*, poiManager, 0x00C0F5E4);
GAME_ARRAY(uint8_t*, hudComponents, 0x00C674A0);
GAME_VAR(int32_t, noAimWeaponA, 0x00BF4430);
GAME_VAR(int32_t, noAimWeaponB, 0x00BF433C);
GAME_VAR(uint32_t, lookAroundInit, 0x01B656A0);
GAME_ARRAY(float, lookAroundDirs, 0x01B65678);   // 5 (cos, sin) pairs
inline void* const thePaths = reinterpret_cast<void*>(0x00BF6228);
inline void* const speech = reinterpret_cast<void*>(0x00CF4378);
inline uint32_t* const punishmentName = reinterpret_cast<uint32_t*>(0x01B652F8);   // "Punishment"
inline void* const punishmentButesName = reinterpret_cast<void*>(0x01B652FC);      // "PunishmentButes"
inline void* const nameKeyDestruct = reinterpret_cast<void*>(0x0049A650);

constexpr float kHalfPi = 1.57079637f;
constexpr float kPi = 3.14159274f;

inline V3 Pos3(uint8_t* e) {
    return *reinterpret_cast<V3*>(Pos(e));
}
inline uint8_t* LocoOut(uint8_t* ped) {   // +0 heading, +0xC facing, +0x1C speed
    return At<uint8_t*>(ped, 0x2E0);
}
// a path node's position (shorts in eighths)
inline V3 NodePos(const uint8_t* node) {
    return V3{float(At<int16_t>(const_cast<uint8_t*>(node), 8)) * 0.125f,
              float(At<int16_t>(const_cast<uint8_t*>(node), 0xA)) * 0.125f,
              float(At<int16_t>(const_cast<uint8_t*>(node), 0xC)) * 0.125f};
}
// an entity's heading: its matrix's (atan2(-up.x... the forward row), else its own
inline float HeadingOf(uint8_t* e) {
    uint8_t* m = At<uint8_t*>(e, 0x14);
    if (m) {
        return trk::CrtAtan2(-At<float>(m, 0x10), At<float>(m, 0x14));
    }
    return At<float>(e, 0x10);
}
// an entity reference moved to another entity (registered)
inline void MoveRef(re::Ptr32<uint8_t>* ref, uint8_t* e) {
    if (e == *ref) {
        return;
    }
    if (*ref) {
        Entity_CleanUpOldReference(*ref, ref);
    }
    *ref = e;
    if (e) {
        RegisterReference(e, ref);
    }
}
// the PunishmentButes level of that name (nullptr: none)
uint8_t* FindPunishmentLevel(const char* name) {
    uint32_t keys[2];
    int32_t path[2];   // (the game doesn't destroy it)
    NameKey_Copy(&keys[0], punishmentName);
    NameKey_Construct(&keys[1], name);
    HashPath_Set(path, 2, keys);
    uint8_t* level = FindBoneInfo(path, punishmentButesName);
    DestructArray(keys, 4, 2, nameKeyDestruct);
    return level;
}

// 0x61EAC0 (thiscall): GivePunishmentPointsToPlayerTrack::Update: the player punished:
// by the named level (+0x1C; its HUD text and points), else by the points (+0x18)
uint8_t __fastcall GivePunishmentPointsToPlayerTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* player = playerPed;
    if (!player) {
        return 0;
    }
    int32_t points = At<int32_t>(self, 0x18);
    if (At<const char*>(self, 0x1C) != nullptr) {
        uint8_t* level = FindPunishmentLevel(At<const char*>(self, 0x1C));
        if (level) {
            HUDDetention_SetText(hudComponents[0], At<const char*>(level, 0x2C), At<int32_t>(level, 0x28), 1);
            points = At<int32_t>(level, 0x28);
        }
    }
    vc::Call<void>(player, 0xB4, points);
    return 0;
}

// WanderTrack / CirculateTrack / RideAroundTrack fields: +0x18 the owner, +0x1C the next
// node, +0x20 the node, +0x24 its reach, +0x28 failures, +0x2C speed (ride-around: +0x30
// its speed). vf5 picks the next node, vf6 goes to a point, vf7 the node kind, vf8 the
// goal, vf9 a node's point, vf10 sets the node, vf11 the lookahead.

// 0x61EB70 (thiscall): WanderTrack::GetGoal: the goal (the node, or the point ahead on
// the way from the next node); 1 while under way, else vf5's pick
uint8_t __fastcall WanderTrack_GetGoal(uint8_t* self, re::Edx, V3* out) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    V3 node;
    vc::Call<V3*>(self, 0x24, &node, At<uint8_t*>(self, 0x20));
    V3 pedPos = Pos3(ped);
    V3 d;
    VectorSub(&d, &node.x, &pedPos.x);
    float width = float(At<uint8_t>(At<uint8_t*>(self, 0x20), 0x12)) * 0.1f;
    uint8_t* next = At<uint8_t*>(self, 0x1C);
    if (!next) {
        *out = node;
        float r = At<float>(self, 0x24) + width;
        float distSq = d.x * d.x + d.y * d.y;
        float rSq = r * r;
        if (!(rSq > distSq)) {
            return 1;
        }
    } else {
        V3 nextPos;
        vc::Call<V3*>(self, 0x24, &nextPos, next);
        float nextWidth = float(At<uint8_t>(At<uint8_t*>(self, 0x1C), 0x12)) * 0.1f;
        Segment seg;
        ColLine_Set(&seg, nextPos.x, nextPos.y, nextPos.z, node.x, node.y, node.z);
        float k = 3.0f;
        uint8_t* veh = At<uint8_t*>(ped, 0x1554);
        if (veh && At<int32_t>(veh, 0x46C) == 1) {
            k = 12.0f;
        }
        float look = vc::Call<float>(self, 0x2C);
        float ahead = At<float>(self, 0x24) + 1.0f + (nextWidth + width) * 0.5f + look * k;
        float t = 0.0f;
        V3 p = pedPos;
        V3 tmp;
        *out = *Segment_PointAhead(&seg, &tmp, &p, ahead, &t);
        if (!(t >= 1.0f)) {
            return 1;
        }
    }
    return vc::Call<uint8_t>(self, 0x14) != 0 ? 1 : 0;
}

// 0x61EDA0 (thiscall): CirculateTrack::PickNode: the ped's path node (+0x534; else one
// ahead within 45 degrees and 30, 120 degrees and 70, else the closest) routed to: when
// the nav mesh has no route, heading away from it (vf7 set) or to the closest node
// (0: failed, the failures counted); then the linked node most in the ped's direction as
// the next one
uint8_t __fastcall CirculateTrack_PickNode(uint8_t* self, re::Edx) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    V3 pedPos = Pos3(ped);
    uint8_t* node = At<uint8_t*>(ped, 0x534);
    V3 heading = At<V3>(ped, 0x528);
    bool found = true;
    if (!node) {
        int kind = vc::Call<uint8_t>(self, 0x1C);
        node = Paths_FindNodeInDirection(thePaths, ped, pedPos.x, pedPos.y, pedPos.z, heading.x, heading.y, heading.z,
                                         0.785398185f, kind, 30.0f, 0, 0, 0, 0);
        if (!node) {
            kind = vc::Call<uint8_t>(self, 0x1C);
            node = Paths_FindNodeInDirection(thePaths, ped, pedPos.x, pedPos.y, pedPos.z, heading.x, heading.y,
                                             heading.z, 2.09439516f, kind, 70.0f, 0, 0, 0, 0);
            if (!node) {
                kind = vc::Call<uint8_t>(self, 0x1C);
                node = Paths_FindNodeClosestToCoors(thePaths, ped, pedPos.x, pedPos.y, pedPos.z, kind, 999999.875f, 0,
                                                    0, 0, 0);
                found = node != nullptr;
            }
        }
    }
    if (found) {
        V3 nodePos = NodePos(node);
        V3 tmp;
        V3 away = *VectorSub(&tmp, &pedPos.x, &nodePos.x);
        Vector_Normalise(&away);
        uint8_t* route = ped + 0xDB0;
        uint8_t* chosen = node;
        if (!RouteBetweenPositions(&pedPos, &nodePos, 0, route, 0.3f)) {
            if (vc::Call<uint8_t>(self, 0x1C)) {
                At<V3>(ped, 0x528) = away;
                At<uint8_t*>(ped, 0x534) = nullptr;
                At<uint8_t>(self, 0x28) += 1;
                return 0;
            }
            chosen = Paths_FindNodeClosestToCoors(thePaths, ped, pedPos.x, pedPos.y, pedPos.z, 1, 999999.875f, 0, 0, 0,
                                                  0);
            if (chosen) {
                V3 closest;
                Node_GetPosition(chosen, &closest);
                V3 dir;
                Normalised(&dir, VectorSub(&tmp, &pedPos.x, &closest.x));
                if (!RouteBetweenPositions(&pedPos, &closest, 0, route, 0.3f)) {
                    At<V3>(ped, 0x528) = dir;
                    At<uint8_t>(self, 0x28) += 1;
                    At<uint8_t*>(ped, 0x534) = nullptr;
                    return 0;
                }
            }
        }
        vc::Call<void>(self, 0x28, chosen);
        At<uint8_t>(self, 0x28) = 0;
    }
    if (At<uint8_t*>(self, 0x1C) == nullptr) {
        uint8_t* cur = At<uint8_t*>(self, 0x20);
        float best = -1.0f;
        int links = At<uint8_t>(cur, 0x14) & 0xF;
        V3 curPos = NodePos(cur);
        V3 tmp;
        V3 toPed = *VectorSub(&tmp, &pedPos.x, &curPos.x);
        Vector_Normalise(&toPed);
        for (int i = 0; i < links; ++i) {
            uint8_t* link = Node_GetLinkNode(At<uint8_t*>(self, 0x20), i);
            V3 linkPos = NodePos(link);
            V3 d = *VectorSub(&tmp, &linkPos.x, &curPos.x);
            Vector_Normalise(&d);
            float dot = d.y * toPed.y + toPed.x * d.x + d.z * toPed.z;
            if (best < dot) {
                best = dot;
                At<uint8_t*>(self, 0x1C) = link;
            }
        }
    }
    return 1;
}

// 0x61F250 (thiscall): WanderTrack::PickNext: the node after this one (or the one after
// that) the nav mesh routes to from the ped's point: set (vf10); else the failures
// counted (0)
uint8_t __fastcall WanderTrack_PickNext(uint8_t* self, re::Edx) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    uint8_t* next = Node_FindNextNode(At<uint8_t*>(self, 0x20), At<uint8_t*>(self, 0x1C), ped);
    NavPoint goal{nullptr, -1, {0.0f, 0.0f, 0.0f}};
    uint8_t* current = ped + 0x1268;
    NavPoint start = *reinterpret_cast<NavPoint*>(current);
    if (!NavPoint_IsValid(current)) {
        Navigator_FindCurrent(ped + 0xDAC);
        start = *reinterpret_cast<NavPoint*>(current);
        if (!NavPoint_IsValid(current)) {
            return 0;
        }
    }
    V3 pos = NodePos(next);
    if (!FindNavPoint(&pos, &goal, nullptr, nullptr) || !RouteBetween(&start, &goal, 0, nullptr, 0.3f)) {
        next = Node_FindNextNode(At<uint8_t*>(self, 0x20), next, ped);
        pos = NodePos(next);
        if (!FindNavPoint(&pos, &goal, nullptr, nullptr) || !RouteBetween(&start, &goal, 0, nullptr, 0.3f)) {
            At<uint8_t>(self, 0x28) += 1;
            return 0;
        }
    }
    At<uint8_t>(self, 0x28) = 0;
    vc::Call<void>(self, 0x28, next);
    return 1;
}

// 0x61F450 (thiscall): WanderTrack::GoTo: steered to the point; under way, the ped's
// flat heading to it and its node kept (+0x528, +0x534)
uint8_t __fastcall WanderTrack_GoTo(uint8_t* self, re::Edx, float x, float y, float z) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    uint8_t ok = Steering_GoTo(ped + 0x654, x, y, z, 0x21C, At<float>(self, 0x2C), 0.3f, nullptr, 0);
    if (ok) {
        V3 p{x, y, z};
        V3 tmp;
        V3 dir = *VectorSub(&tmp, &p.x, Pos(ped));
        dir.z = 0.0f;
        Vector_Normalise(&dir);
        At<V3>(ped, 0x528) = dir;
        At<uint8_t*>(ped, 0x534) = At<uint8_t*>(self, 0x20);
    }
    return ok;
}

// 0x61F520 (thiscall): WanderTrack::NodePoint: the ped's own point on a node (by its
// random seed +0x10C)
V3* __fastcall WanderTrack_NodePoint(uint8_t* self, re::Edx, V3* out, uint8_t* node) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    GetPointOnNode(out, node, At<uint16_t>(ped, 0x10C));
    return out;
}

// 0x61F550 (thiscall): RideAroundTrack::GoTo: riding to the point (speed from stat 0x18):
// a non-player-hostile rider near its (ped, not on a bike) target that is ahead and
// coming towards it passes it on the clear side; a good rider near the moving player
// heading its way cuts across the player's path; else just there
uint8_t __fastcall RideAroundTrack_GoTo(uint8_t* self, re::Edx, float x, float y, float z) {
    V3 p{x, y, z};
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    At<float>(self, 0x30) = float(Ped_GetStat(ped, 0x18)) * 0.01f;
    uint8_t* target = Ped_GetTarget(ped);
    if (target && !IsPedEntity(target)) {
        target = nullptr;
    }
    V3 vel{0.0f, 0.0f, 0.0f};
    int32_t attitude = 0;   // (the game's is stale without a target: unused then)
    if (target) {
        uint8_t* veh = At<uint8_t*>(target, 0x1554);
        vel = At<V3>(veh ? veh : target, 0x124);
        int32_t tmp;
        attitude = *Ped_GetAttitude(ped, &tmp, At<int32_t>(target, 0x1310));
    }
    V3 pedPos = Pos3(ped);
    float radius = 1.5f;
    if (target && target != ped &&
        !(At<uint8_t*>(target, 0x1554) && At<int32_t>(At<uint8_t*>(target, 0x1554), 0x46C) == 0)) {
        V3 tpos = Pos3(target);
        uint8_t* tm = Entity_GetMatrix(target);
        float sx = At<float>(tm, 0);
        float sy = At<float>(tm, 4);
        V3 t;
        if ((Ped_GetStat(ped, 0xE) < 26 && attitude <= 1) || (Ped_GetStat(ped, 0xE) < 11 && attitude == 2)) {
            if (DistanceSquared(&tpos, &pedPos) < 10.0f && vel.x * vel.x + vel.y * vel.y + vel.z * vel.z > 0.01f &&
                DotProduct(&vel, VectorSub(&t, &p.x, &pedPos.x)) > 0.0f &&
                DotProduct(&vel, VectorSub(&t, &tpos.x, &pedPos.x)) < 0.0f) {
                V3 toTarget, toGoal;
                Normalised(&toTarget, VectorSub(&t, &tpos.x, &pedPos.x));
                Normalised(&toGoal, VectorSub(&t, &p.x, &pedPos.x));
                if (DotProduct(&toGoal, &toTarget) > -0.8f) {
                    Normalised(&toTarget, VectorSub(&t, &tpos.x, &pedPos.x));
                    Normalised(&toGoal, VectorSub(&t, &p.x, &pedPos.x));
                    if (DotProduct(&toGoal, &toTarget) < 0.8f) {
                        // pass it
                        float dist = Distance(&p.x, &pedPos.x);
                        V3 cand = p;
                        uint8_t* pm = Entity_GetMatrix(ped);
                        V3 right{At<float>(pm, 0), At<float>(pm, 4), At<float>(pm, 8)};
                        V3 r, scaled, probe;
                        bool placed = false;
                        if (DotProduct(&right, VectorSub(&t, &tpos.x, &pedPos.x)) > 0.0f) {
                            VectorScale(&scaled, Entity_GetRight(ped, &r), 2.0f);
                            if (GetIsLineOfSightClear(&pedPos, VectorSub(&probe, &pedPos.x, &scaled.x), 1)) {
                                cand.x = x - dist * sx * 0.3f;
                                cand.y = y - dist * sy * 0.3f;
                                placed = true;
                            }
                        }
                        if (!placed) {
                            VectorScale(&scaled, Entity_GetRight(ped, &r), 2.0f);
                            if (GetIsLineOfSightClear(&pedPos, VectorAdd(&probe, &pedPos.x, &scaled.x), 1)) {
                                cand.x = dist * sx * 0.3f + x;
                                cand.y = dist * sy * 0.3f + y;
                            }
                        }
                        V3 n, s3, ahead;
                        VectorAdd(&ahead, &pedPos.x, &VectorScale(&s3, Normalised(&n, VectorSub(&t, &cand.x, &pedPos.x)), 3.0f)->x);
                        if (GetIsLineOfSightClear(&pedPos, &ahead, 1)) {
                            p = cand;
                        }
                        return Steering_GoTo(ped + 0x654, p.x, p.y, p.z, 0x1018, At<float>(self, 0x30), 0.3f,
                                             nullptr, 0);
                    }
                }
            }
        }
        if ((Ped_GetStat(ped, 0xE) > 0x4A && attitude <= 1) || (Ped_GetStat(ped, 0xE) > 0x59 && attitude == 2)) {
            if (DistanceSquared(&tpos, &pedPos) < 25.0f && Vector_MagnitudeSqr(FindPlayerSpeed()) > 0.01f) {
                V3 moving, toGoal;
                Normalised(&moving, FindPlayerSpeed());
                if (DotProduct(Normalised(&toGoal, VectorSub(&t, &p.x, &pedPos.x)), &moving) > 0.707f) {
                    V3 toTarget;
                    Normalised(&toTarget, VectorSub(&t, &tpos.x, &pedPos.x));
                    Normalised(&toGoal, VectorSub(&t, &p.x, &pedPos.x));
                    if (DotProduct(&toGoal, &toTarget) > -0.1f) {
                        // cut across ahead of it, to its side
                        V3 lead{vel.x * 30.0f, vel.y * 30.0f, vel.z * 30.0f};
                        V3 goal;
                        VectorAdd(&goal, &tpos.x, &lead.x);
                        p = goal;
                        uint8_t* pm = Entity_GetMatrix(ped);
                        V3 right{At<float>(pm, 0), At<float>(pm, 4), At<float>(pm, 8)};
                        if (DotProduct(&right, VectorSub(&t, &tpos.x, &pedPos.x)) > 0.0f) {
                            p.x = p.x - sx * 0.75f;
                            p.y = p.y - sy * 0.75f;
                        } else {
                            p.x = sx * 0.75f + p.x;
                            p.y = 0.75f * sy + p.y;
                        }
                        return Steering_GoTo(ped + 0x654, p.x, p.y, p.z, 8, At<float>(self, 0x30) * 1.2f, 1.5f,
                                             nullptr, 0);
                    }
                }
            }
        }
    }
    return Steering_GoTo(ped + 0x654, p.x, p.y, p.z, 0x1018, At<float>(self, 0x30), radius, nullptr, 0);
}

// 0x61FD10 (thiscall): HaltVehicleTrack::Update: the ped stopped and its vehicle braking
// (a bike: +0x2DA 11; else no gas, full brake); once still, a queued objective 6 dropped
uint8_t __fastcall HaltVehicleTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    At<V3>(LocoOut(ped), 0) = V3{zeroVector[0], zeroVector[1], zeroVector[2]};
    At<float>(LocoOut(ped), 0x1C) = 0.0f;
    uint8_t* veh = At<uint8_t*>(ped, 0x1554);
    if (!veh) {
        return 0;
    }
    if (At<int32_t>(veh, 0x46C) == 0) {
        At<uint8_t>(veh, 0x2DA) = 0xB;
    } else {
        At<float>(veh, 0x69C) = 0.0f;
        At<float>(veh, 0x298) = 1.0f;
    }
    V3& v = At<V3>(veh, 0x124);
    float speedSq = v.x * v.x + v.y * v.y + v.z * v.z;
    if (!(speedSq < 0.005f)) {
        return 1;
    }
    uint8_t* cur = Queue_GetCurrent(ped + 0x5F0);
    if (cur && At<int32_t>(cur, 8) == 6) {
        Queue_RemoveFirst(ped + 0x5F0);
    }
    return 0;
}

// FleeTrack fields: +0x18 / +0x1C the steering's, +0x20 from the target, +0x21 send the
// stimulus, +0x24 the owner, +0x28 the entity fled (registered), +0x2C the point, +0x38
// first frame

// 0x61FDE0 (thiscall): FleeTrack::Start: what to flee: the target (+0x20), a queued flee
// objective's (0x16) entity, the attacker, the stimulus' entity or point, else the player
uint8_t __fastcall FleeTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    At<uint8_t>(self, 0x38) = 1;
    uint8_t* target = Ped_GetTarget(OwnerPed(owner));
    re::Ptr32<uint8_t>* ref = &At<uint8_t*>(self, 0x28);
    if (At<uint8_t>(self, 0x20) != 0 && target) {
        MoveRef(ref, target);
    } else {
        uint8_t* cur = Queue_GetCurrent(OwnerPed(At<uint8_t*>(self, 0x24)) + 0x5F0);
        if (cur && At<int32_t>(cur, 8) == 0x16 && At<uint8_t*>(cur, 0x10)) {
            EntityRef_Set(ref, At<uint8_t*>(cur, 0x10));
        } else {
            uint8_t* attacker = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x24), 0x10), 4);
            if (attacker) {
                MoveRef(ref, attacker);
            } else {
                uint8_t* stim = At<uint8_t*>(OwnerPed(At<uint8_t*>(self, 0x24)), 0x1D30);
                if (!stim) {
                    EntityRef_Set(ref, playerPed);
                } else if (At<uint8_t*>(stim, 8)) {
                    EntityRef_Set(ref, At<uint8_t*>(stim, 8));
                } else {
                    At<V3>(self, 0x2C) = At<V3>(stim, 0xC);
                }
            }
        }
    }
    At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x24)), 0x1550) = 1;
    return 1;
}

// 0x61FEE0 (thiscall): FleeTrack::Update: steered away; done, stimulus 0x2A sent (+0x21)
uint8_t __fastcall FleeTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x24));
    uint8_t* from = At<uint8_t*>(self, 0x28);
    V3 pos = from ? Pos3(from) : At<V3>(self, 0x2C);
    uint8_t ok = Steering_GoToWith(ped + 0x654, pos.x, pos.y, pos.z, At<uint8_t>(self, 0x38), 0x254, 1.5f, 2.0f,
                                   At<float>(self, 0x18), At<float>(self, 0x1C));
    At<uint8_t>(self, 0x38) = 0;
    if (ok) {
        return 1;
    }
    if (At<uint8_t>(self, 0x21) != 0 && events) {
        EventMgr_Send(events, 0x2A, At<uint8_t*>(self, 0x28), ped, nullptr, -1.0f, -1.0f);
    }
    return 0;
}

// 0x61FFD0 (thiscall): FleeTrack::End: the ped no longer fleeing
uint8_t __fastcall FleeTrack_End(uint8_t* self, re::Edx) {
    At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x24)), 0x1550) = 0;
    return 1;
}

// the POI tracks' time limit passed: its lower (+0x18) or upper (+0x1C) bound, and a
// random one between them (twice the lower when they're the wrong way round)
bool LimitPassed(uint8_t* self, uint8_t* owner) {
    float limit;
    if (At<float>(self, 0x1C) < At<float>(self, 0x18)) {
        limit = At<float>(self, 0x18) + At<float>(self, 0x18);
    } else {
        limit = RandomFloat(At<float>(self, 0x18), At<float>(self, 0x1C));
    }
    float t = At<float>(At<uint8_t*>(owner, 0), 8);
    return (At<float>(self, 0x18) < t || At<float>(self, 0x1C) < t) && limit < t;
}

// the group's chatter stopped (+0x44) and its line cleared
void StopChatter(uint8_t* group) {
    At<uint8_t>(group, 0x44) = 0;
    SpeechEvent line;
    SpeechEvent_Reset(&line);
    line.w0 = (line.w0 & 0xFE0001FF) | 0x1FF;
    line.w1 &= 0xFFFFF800;
    line.w10 = 0;
    At<SpeechEvent>(group, 0x48) = line;
}

// 0x61FFF0 (thiscall): HangOutTrack::Update: at its hang-out POI (kind 2; +0x20 set when
// it isn't, or its time is up and no one is talking): the group updated, and near the
// player its chatter run: a line (0x66) started when no one talks, the talker replying
// (0x4F) or handing over to a random member
uint8_t __fastcall HangOutTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x24));
    uint8_t* poi = At<uint8_t*>(ped, 0x1D24);
    uint8_t* group = At<uint8_t*>(ped, 0x1D20);
    SpeechEvent line;
    memset_(&line, 0, 0x30);
    if (!poi || !group || At<int32_t>(poi, 0x70) != 2) {
        At<uint8_t>(self, 0x20) = 1;
        return 0;
    }
    if (At<uint8_t>(ped, 0x1570) != 2 && !(At<float>(self, 0x18) == 0.0f && At<float>(self, 0x1C) == 0.0f) &&
        LimitPassed(self, At<uint8_t*>(self, 0x24))) {
        if (At<uint8_t>(group, 0x44) == 0 ||
            !Speech_IsPedSayingEvent(speech, At<uint8_t*>(group, 0x7C), line.w0 & 0x1FF, line.w1 & 0x7FF)) {
            At<uint8_t>(self, 0x20) = 1;
            return 0;
        }
    }
    vc::Call<void>(group, 0xC, ped);
    uint32_t now = timeInMs;
    if (At<int32_t>(ped, 0x1378) == 0) {
        return 1;
    }
    V3 t;
    float dist = Vector_Magnitude(VectorSub(&t, Pos(ped), Pos(playerPed)));
    int32_t members = PedGroup_CountMembers(group);
    re::Ptr32<uint8_t>* talker = &At<uint8_t*>(group, 0x7C);
    if (10.0f < dist) {
        if (members >= 2 && At<uint8_t>(group, 0x44) == 0) {
            return 1;
        }
    }
    if (members < 2) {
        if (At<uint8_t>(group, 0x44) != 0) {
            StopChatter(group);
        }
        return 1;
    }
    if (At<uint8_t>(group, 0x44) == 0) {
        if (*talker && !Group_ContainsPed(group, *talker)) {
            SetRef(talker, nullptr);
        }
        uint8_t* who = *talker;
        if ((!who || who == ped) && now > At<uint32_t>(group, 0x78) + 3000 && Speech_PassesPedFlag(speech, ped, 0x66)) {
            SpeechEvent out;
            line = *Speech_Add(speech, &out, ped, group, 0, 0x66, 0, 0, 0, 0, -1, 0);
            if ((line.w0 & 0x1FF) != 0x66) {
                At<SpeechEvent>(group, 0x48) = line;
                At<uint32_t>(group, 0x78) = now;
                At<uint8_t>(group, 0x44) = 1;
                SetRef(talker, ped);
            }
            return 1;
        }
        if (At<uint8_t>(group, 0x44) == 0) {
            return 1;
        }
    }
    if (IsSpeechBusy(*talker, -1, -1)) {
        return 1;
    }
    line = At<SpeechEvent>(group, 0x48);
    bool reply = false;
    bool otherSpeaker = false;
    switch (line.w0 & 0x1FF) {
    case 0x4E:
        otherSpeaker = true;
        break;
    case 0x5A:
    case 0x5C:
    case 0x61:
        if (RandomRange(0, 100) < 50) {
            reply = true;
            break;
        }
        [[fallthrough]];
    case 0x64:
        At<uint8_t>(group, 0x44) = 0;
        SetRef(talker, PedGroup_GetMember(group, RandomRange(0, members)));
        return 1;
    default:
        break;
    }
    if (!reply && (int32_t(At<int16_t>(ped, 0x10E)) != int32_t((line.w0 >> 9) & 0xFF)) == otherSpeaker) {
        return 1;
    }
    if (IsSpeechBusy(*talker, -1, -1)) {
        return 1;
    }
    SpeechEvent out;
    SpeechEvent* said = Speech_Add(speech, &out, ped, group, 0, 0x4F, 0, 0, 0, 0, -1, 0);
    line = *said;
    At<SpeechEvent>(group, 0x48) = *said;
    // (the same whether the reply was said or not)
    At<uint32_t>(group, 0x78) = now;
    SetRef(talker, ped);
    return 1;
}

// 0x620470 (thiscall): HangOutTrack::End: the chatter stopped (0 without a player); the
// ped leaves its POI (+0x20 set, or it's in a pair), else its leave time set; no longer
// hanging out
uint8_t __fastcall HangOutTrack_End(uint8_t* self, re::Edx) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x24));
    uint8_t* poi = At<uint8_t*>(ped, 0x1D24);
    uint8_t* group = At<uint8_t*>(ped, 0x1D20);
    if (poi) {
        if (group && At<uint8_t>(group, 0x44) != 0) {
            if (!playerPed) {
                return 0;
            }
            StopChatter(group);
        }
        if (At<uint8_t>(self, 0x20) != 0 || (At<int32_t>(ped, 0xBB0) != 0 && At<int32_t>(ped, 0xBB4) != 0)) {
            uint8_t* spot = At<uint8_t*>(poi, 0x6C);
            if (spot) {
                At<uint8_t>(spot, 0x90) = 1;
            }
            POI_Release(poi, ped);
        }
        if (!(At<int32_t>(ped, 0xBB0) != 0 && At<int32_t>(ped, 0xBB4) != 0)) {
            At<uint32_t>(ped, 0xC38) = timeInMs;
        }
    }
    At<int32_t>(ped, 0x1378) = 0;
    At<int32_t>(ped, 0x1340) = 0;
    return 1;
}

// 0x620560 (thiscall): SpectatorTrack::Update: at its spectator POI (kind 1) until its
// time is up: the group updated, the ped looking ahead of the POI (+0xD74)
uint8_t __fastcall SpectatorTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x20));
    uint8_t* group = At<uint8_t*>(ped, 0x1D20);
    uint8_t* poi = At<uint8_t*>(ped, 0x1D24);
    if (!poi || !group || At<int32_t>(poi, 0x70) != 1) {
        return 0;
    }
    if (LimitPassed(self, At<uint8_t*>(self, 0x20))) {
        return 0;
    }
    vc::Call<void>(group, 0xC, ped);
    float* pos = Pos(poi);
    V3 fwd, look;
    VectorAdd(&look, pos, &Placeable_GetForward(ped, &fwd)->x);
    At<V3>(ped, 0xD74) = look;
    V3 t;
    VectorSub(&t, Pos(ped), Pos(playerPed));   // (unused)
    return 1;
}

// 0x6206B0 (thiscall): SpectatorTrack::End: not looking, the POI left
uint8_t __fastcall SpectatorTrack_End(uint8_t* self, re::Edx) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x20));
    At<V3>(ped, 0xD74) = V3{zeroVector[0], zeroVector[1], zeroVector[2]};
    if (At<uint8_t*>(ped, 0x1D24)) {
        POI_Release(At<uint8_t*>(ped, 0x1D24), ped);
    }
    At<int32_t>(ped, 0x13D4) = 0;
    return 1;
}

// 0x620700 (thiscall): GenerateDynamicPOITrack::Start: a dynamic POI (radius +0x18) made
// just ahead of the ped, facing its way, and joined
uint8_t __fastcall GenerateDynamicPOITrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* ped = OwnerPed(owner);
    V3 fwd;
    Placeable_GetForward(ped, &fwd);
    Vector_Normalise(&fwd);
    V3 spot;
    VectorAdd(&spot, Pos(ped), &fwd.x);
    float heading = HeadingOf(ped);
    uint8_t* poi = POIManager_AddDynamic(poiManager, At<float>(self, 0x18), spot.x, spot.y, spot.z, heading);
    POI_Join(poi, ped, 1);
    return 1;
}

// 0x6207C0 (thiscall): JoinDynamicPOITrack::Start: not at a POI: the one of its (ped)
// target joined
uint8_t __fastcall JoinDynamicPOITrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (At<uint8_t*>(ped, 0x1D24)) {
        return 0;
    }
    uint8_t* target = Ped_GetTarget(ped);
    if (target && !IsPedEntity(target)) {
        target = nullptr;
    }
    uint8_t* poi = POIManager_FindByPed(poiManager, target);
    if (!poi) {
        return 0;
    }
    POI_Join(poi, ped, 1);
    return 1;
}

// 0x620820 (thiscall): FaceDirectionTrack::Face: unless the ped already faces within
// minDot of it, stopped facing the direction (and heading there unless nearly so)
void __fastcall FaceDirection_Face(uint8_t* self, re::Edx, float x, float y, float z, float minDot) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    V3 fwd;
    V3* f = Placeable_GetForward(ped, &fwd);
    float facing = f->y * y + f->x * x + f->z * z;
    if (!(minDot >= facing)) {   // (unordered too)
        return;
    }
    uint8_t* out = LocoOut(ped);
    At<float>(out, 0x1C) = 0.0f;
    float along = x * At<float>(out, 0) + y * At<float>(out, 4) + z * At<float>(out, 8);
    if (along < 0.999f) {
        At<V3>(out, 0) = V3{x, y, z};
    }
    At<V3>(out, 0xC) = V3{x, y, z};
}

// the flat direction from the ped to a point
inline V3 FlatDirection(uint8_t* ped, const V3& to) {
    V3 d;
    VectorSub(&d, &to.x, Pos(ped));
    d.z = 0.0f;
    Vector_Normalise(&d);
    return d;
}

// 0x6208E0 (thiscall): FaceStimulusTrack::Update: facing its stimulus' point
uint8_t __fastcall FaceStimulusTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* stim = At<uint8_t*>(OwnerPed(At<uint8_t*>(self, 0x18)), 0x1D30);
    if (!stim) {
        return 0;
    }
    V3 pos = At<V3>(stim, 0xC);
    V3 d = FlatDirection(OwnerPed(At<uint8_t*>(self, 0x18)), pos);
    FaceDirection_Face(self, nullptr, d.x, d.y, d.z, 0.998f);
    return 1;
}

// 0x620980 (thiscall): LookAroundTrack::LookAt: a point 1.5 up and the ped's look
// distance (+0xA6C) away, its forward turned by one of 5 angles (by the time +0x1C)
void __fastcall LookAroundTrack_LookAt(uint8_t* self, re::Edx, V3* out) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    if (!(lookAroundInit & 1)) {
        lookAroundInit |= 1;
        static const float angles[5] = {0.0f, -0.698131740f, 0.261799395f, -0.261799395f, 0.698131740f};
        for (int i = 0; i < 5; ++i) {
            lookAroundDirs[i * 2] = trk::CrtCos(angles[i]);
            lookAroundDirs[i * 2 + 1] = trk::CrtSin(angles[i]);
        }
    }
    uint32_t i = uint32_t(int64_t(At<float>(self, 0x1C))) % 5;
    V3 fwd;
    Placeable_GetForward(ped, &fwd);
    float c = lookAroundDirs[i * 2];
    float s = lookAroundDirs[i * 2 + 1];
    float x = c * fwd.x - s * fwd.y;
    out->x = x;
    float y = s * fwd.x + c * fwd.y;
    out->y = y;
    out->z = 0.0f;
    float k = At<float>(ped, 0xA6C);
    x = k * x;
    out->x = x;
    y = k * y;
    out->y = y;
    float z = k * 0.0f;
    out->z = z;
    float* pos = Pos(ped);
    out->x = pos[0] + x;
    out->y = pos[1] + y;
    float up = z + pos[2];
    out->z = up + 1.5f;
}

// 0x620BB0 (thiscall): LookAroundTrack::Update: the ped looking round (+0xD74)
uint8_t __fastcall LookAroundTrack_Update(uint8_t* self, re::Edx, float dt) {
    At<float>(self, 0x1C) = dt + At<float>(self, 0x1C);
    V3 look;
    LookAroundTrack_LookAt(self, nullptr, &look);
    At<V3>(OwnerPed(At<uint8_t*>(self, 0x18)), 0xD74) = look;
    return 1;
}

// 0x620C00 (thiscall): LookAroundTrack::End: not looking
uint8_t __fastcall LookAroundTrack_End(uint8_t* self, re::Edx) {
    At<V3>(OwnerPed(At<uint8_t*>(self, 0x18)), 0xD74) = V3{zeroVector[0], zeroVector[1], zeroVector[2]};
    return 1;
}

// 0x620C30 (thiscall): MoveToPOITrack::Update: walking to its POI (1 while not there);
// there, with +0x18 set, turning to the POI's heading; done (+0x20 set, 0) once facing it
uint8_t __fastcall MoveToPOITrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x1C));
    uint8_t* poi = At<uint8_t*>(ped, 0x1D24);
    if (!poi) {
        return 0;
    }
    float* pp = Pos(poi);
    if (Steering_GoTo(ped + 0x654, pp[0], pp[1], pp[2], 0x14, 0.4f, 0.3f, nullptr, 0)) {
        return Steering_IsDD4One(ped + 0x654) == 0;
    }
    if (At<uint8_t>(self, 0x18) != 0) {
        V3 fwd;
        Placeable_GetForward(ped, &fwd);
        fwd.z = 0.0f;
        Vector_Normalise(&fwd);
        float heading = HeadingOf(poi);
        V3 dir;
        AngleToVector(&dir, heading);
        dir.z = 0.0f;
        Vector_Normalise(&dir);
        float dot = dir.y * fwd.y + fwd.x * dir.x + dir.z * fwd.z;
        if (dot < 0.998f) {
            At<V3>(LocoOut(ped), 0) = dir;
            At<V3>(LocoOut(ped), 0xC) = dir;
            return 1;
        }
    }
    At<uint8_t>(self, 0x20) = 1;
    return 0;
}

// 0x620DB0 (thiscall): ClearPOITrack::Update: the POI left
uint8_t __fastcall ClearPOITrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    if (At<uint8_t*>(ped, 0x1D24)) {
        POI_Release(At<uint8_t*>(ped, 0x1D24), ped);
    }
    return 0;
}

// 0x620DD0 (thiscall): SitDownTrack::Start: the sitting time picked (+0x18..+0x1C), the
// sit button (0x14) requested at a POI
uint8_t __fastcall SitDownTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    float lo = At<float>(self, 0x18);
    At<uint8_t*>(self, 0x20) = owner;
    At<float>(self, 0x24) = lo;
    if (At<float>(self, 0x1C) > lo) {
        float range = At<float>(self, 0x1C) - lo;
        int r = Rand();
        float add = 0.0f + float(r) / 32767.0f * (range - 0.0f);
        At<float>(self, 0x24) = At<float>(self, 0x24) + add;
    }
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x20));
    if (!At<uint8_t*>(ped, 0x1D24)) {
        return 0;
    }
    ActionInputs_SetRequest(LocoOut(ped), 0x14, 2, 0.0f);
    return 1;
}

// 0x620E60 (thiscall): SitDownTrack::Update: the sit button kept held at the POI; once
// seated (+0x1570 1) until the time is up, unless paired or (+0xD68 bit 4) with +0xBB8
uint8_t __fastcall SitDownTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x20));
    if (!At<uint8_t*>(ped, 0x1D24)) {
        return 0;
    }
    if (!ActionInputs_IsHeld(LocoOut(ped), 0x14)) {
        ActionInputs_SetRequest(LocoOut(ped), 0x14, 2, 0.0f);
    }
    if (At<uint8_t>(ped, 0x1570) == 1) {
        float t = At<float>(At<uint8_t*>(At<uint8_t*>(self, 0x20), 0), 8);
        if (At<float>(self, 0x24) < t) {
            return 0;
        }
        if (At<int32_t>(ped, 0xBB0) != 0 && At<int32_t>(ped, 0xBB4) != 0) {
            return 0;
        }
        if ((At<uint8_t>(ped, 0xD68) & 0x10) && At<int32_t>(ped, 0xBB8) != 0) {
            return 0;
        }
    }
    return 1;
}

// 0x620EF0 (thiscall): SitDownTrack::End: the POI left, the sit button released (3)
uint8_t __fastcall SitDownTrack_End(uint8_t* self, re::Edx) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x20));
    if (At<uint8_t*>(ped, 0x1D24)) {
        POI_Release(At<uint8_t*>(ped, 0x1D24), ped);
    }
    ActionInputs_SetRequest(LocoOut(ped), 0x14, 3, 0.0f);
    return 1;
}

// 0x620F20 (thiscall): FaceTargetTrack::Update: facing its target of a kind (+0x20),
// turned by +0x28 degrees (0: already within +0x2C degrees, +0x1C set); as the player
// aiming a weapon, stimulus 0xC sent to it every 8th frame; moving (+0x30), 1 at once;
// no target: +0x24
uint8_t __fastcall FaceTargetTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    uint8_t* target = Ped_FindByType(ped, At<int32_t>(self, 0x20), 0);
    if (At<int32_t>(ped, 0x1310) == 0xD) {
        if (Ped_EveryNthFrame(ped, 8) && At<int32_t>(ped, 0x142C) == 1 && At<uint8_t>(ped, 0x1DE5) == 0) {
            int32_t weapon = At<int16_t>(At<uint8_t*>(ped, 0x1D0), 0x10E);
            if (weapon != -1 && weapon != noAimWeaponA && weapon != noAimWeaponB) {
                uint8_t* other = nullptr;
                if (target) {
                    if (IsPedEntity(target)) {
                        other = target;
                    }
                } else if (At<uint8_t>(ped, 0x1EC0) != 0) {
                    uint8_t* look = Targeting_GetLookTarget(At<uint8_t*>(ped, 0x1C8));
                    if (look && IsPedEntity(look)) {
                        other = look;
                    }
                }
                if (other) {
                    EventMgr_Send(events, 0xC, ped, other, nullptr, -1.0f, -1.0f);
                }
            }
        }
        if (At<uint8_t>(self, 0x30) != 0 && 0.3f < At<float>(LocoOut(ped), 0x1C)) {
            return 1;
        }
    }
    if (!target) {
        return At<uint8_t>(self, 0x24) != 0;
    }
    V3 tpos = Pos3(target);
    V3 d;
    VectorSub(&d, &tpos.x, Pos(ped));
    float a = HeadingFromVector(&d);
    float turn = At<float>(self, 0x28);
    float angle = LimitAngle(turn * kPi / 180.0f + a);
    V3 tmp;
    V3 dir = *Vector_FromAngle(&tmp, angle);
    if (At<uint8_t>(self, 0x1C) != 0) {
        V3 fwd;
        Placeable_GetForward(ped, &fwd);
        fwd.z = 0.0f;
        Vector_Normalise(&fwd);
        float dot = dir.y * fwd.y + fwd.x * dir.x + dir.z * fwd.z;
        float s = trk::CrtSin(At<float>(self, 0x2C) * kPi / 180.0f);
        if (dot > 1.0f - s) {
            return 0;
        }
    }
    FaceDirection_Face(self, nullptr, dir.x, dir.y, dir.z, 0.998f);
    return 1;
}

// 0x6211D0 (thiscall): FaceVehicleForEntryTrack::Update: in a car (not a bike), the ped
// turned to its heading, or across it at an open door (+0x1C; status 3); 0 when it
// already faced that way (+0x20 set: within 5 degrees)
uint8_t __fastcall FaceVehicleForEntryTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    uint8_t* veh = At<uint8_t*>(ped, 0x430);
    if (!veh || At<int32_t>(veh, 0x46C) != 0) {
        return 0;
    }
    uint8_t check = At<uint8_t>(self, 0x20);
    int32_t door = At<int32_t>(self, 0x1C);
    // (the game's 5 degrees: a double that is a float)
    const float fiveDegrees = float(*reinterpret_cast<const double*>(0x00933680));
    if (Damage_GetDoorStatus(veh + 0x580, door) != 3) {
        V3 vf;
        Placeable_GetForward(veh, &vf);
        vf.z = 0.0f;
        Vector_Normalise(&vf);
        if (check) {
            V3 pf;
            Placeable_GetForward(ped, &pf);
            pf.z = 0.0f;
            Vector_Normalise(&pf);
            float dot = vf.y * pf.y + pf.x * vf.x + pf.z * vf.z;
            float c = trk::CrtCos(fiveDegrees);
            if (dot > 1.0f - c) {
                Ped_SetHeading(ped, Entity_GetHeading(veh));
                return 0;
            }
        }
        Ped_SetHeading(ped, Entity_GetHeading(veh));
        return 1;
    }
    float off = door == 2 ? -kHalfPi : kHalfPi;
    if (check) {
        float current = At<float>(ped, 0x1700);
        float diff = std::fabs(current - (Entity_GetHeading(veh) + off));
        // (the game compares the angle with a cosine)
        float c = trk::CrtCos(fiveDegrees);
        if (diff <= c) {
            Ped_SetHeading(ped, Entity_GetHeading(veh) + off);
            return 0;
        }
    }
    Ped_SetHeading(ped, Entity_GetHeading(veh) + off);
    return 1;
}

// 0x6213A0 (thiscall): FaceDiveEntityTrack::Update: facing the entity dived at (+0x16D8);
// 1 while not facing it (dot 0.8)
uint8_t __fastcall FaceDiveEntityTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    uint8_t* e = At<uint8_t*>(ped, 0x16D8);
    if (!e) {
        return 0;
    }
    V3 epos = Pos3(e);
    V3 d = FlatDirection(ped, epos);
    FaceDirection_Face(self, nullptr, d.x, d.y, d.z, 0.998f);
    V3 fwd;
    Placeable_GetForward(ped, &fwd);
    fwd.z = 0.0f;
    Vector_Normalise(&fwd);
    float dot = fwd.x * d.x + d.y * fwd.y + fwd.z * d.z;
    return dot < 0.8f ? 1 : 0;
}

// 0x6214B0 (thiscall): FaceHitDirectionTrack::Update: after a hit, facing where it came
// from
uint8_t __fastcall FaceHitDirectionTrack_Update(uint8_t* self, re::Edx, float) {
    if (*Ped_GetLastHitWeapon(OwnerPed(At<uint8_t*>(self, 0x18))) == 0) {
        return 0;
    }
    uint8_t* hit = Ped_GetLastHitWeapon(OwnerPed(At<uint8_t*>(self, 0x18)));
    FaceDirection_Face(self, nullptr, -At<float>(hit, 0x20), -At<float>(hit, 0x24), -At<float>(hit, 0x28), 0.998f);
    return 1;
}

// 0x621530 (thiscall): MoveToStimulusTrack::Start: a move-to-stimulus objective (speed
// +0x1C) queued first; with +0x20, one already queued replaced unless of higher priority
uint8_t __fastcall MoveToStimulusTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x30) = owner;
    uint8_t* ped = OwnerPed(owner);
    uint8_t* stim = At<uint8_t*>(ped, 0x1D30);
    if (!stim) {
        return 0;
    }
    float speed = At<float>(self, 0x1C);
    if (At<uint8_t>(self, 0x20) != 0) {
        uint8_t* cur = Queue_GetCurrent(ped + 0x5F0);   // (not checked for none)
        if (At<int32_t>(cur, 8) == 0x18) {
            if (At<int32_t>(cur, 0x34) > Stimulus_GetPriority(stim)) {
                return 0;
            }
            Queue_RemoveFirst(ped + 0x5F0);
        }
    }
    void* mem = Malloc(0x38);
    uint8_t* objective = mem ? MoveToStimulusObjective_Construct(mem, ped, stim) : nullptr;
    At<float>(objective, 0x10) = speed;
    Queue_Prepend(ped + 0x5F0, objective);
    return 0;
}

// 0x6215C0 (thiscall): MoveToStimulusTrack::Update: its objective's point walked to
// (within +0x18); there, the objective dropped
uint8_t __fastcall MoveToStimulusTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x30));
    uint8_t* cur = Queue_GetCurrent(ped + 0x5F0);
    if (!cur || At<int32_t>(cur, 8) != 0x18) {
        return 0;
    }
    float radius = At<float>(self, 0x18);
    float speed = At<float>(cur, 0x10);
    V3* p = vc::Call<V3*>(cur, 0x10);
    if (Steering_GoTo(ped + 0x654, p->x, p->y, p->z, 0x14, speed, radius, nullptr, 0)) {
        return 1;
    }
    Queue_RemoveFirst(ped + 0x5F0);
    return 0;
}

// 0x621660 (thiscall): InvestigateAreaTrack::Start: the point to investigate (+0x28: 0
// the player's, 1 the ped's alert point +0x1D34, else none)
uint8_t __fastcall InvestigateAreaTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x2C) = owner;
    At<int32_t>(self, 0x30) = 0;
    uint8_t* ped = OwnerPed(owner);
    switch (At<int32_t>(self, 0x28)) {
    case 0:
        At<V3>(self, 0x38) = Pos3(playerPed);
        break;
    case 1:
        At<V3>(self, 0x38) = At<V3>(ped, 0x1D34);
        break;
    default:
        At<V3>(self, 0x38) = V3{0.0f, 0.0f, 0.0f};
        break;
    }
    return 0;
}

} // namespace

#define TRWANDER_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

TRWANDER_REVERSED("GivePunishmentPointsToPlayerTrack::Update", 0x0061EAC0, &GivePunishmentPointsToPlayerTrack_Update);
TRWANDER_REVERSED("WanderTrack::GetGoal_61EB70", 0x0061EB70, &WanderTrack_GetGoal);
TRWANDER_REVERSED("CirculateTrack::PickNode_61EDA0", 0x0061EDA0, &CirculateTrack_PickNode);
TRWANDER_REVERSED("WanderTrack::PickNext_61F250", 0x0061F250, &WanderTrack_PickNext);
TRWANDER_REVERSED("WanderTrack::GoTo_61F450", 0x0061F450, &WanderTrack_GoTo);
TRWANDER_REVERSED("WanderTrack::NodePoint_61F520", 0x0061F520, &WanderTrack_NodePoint);
TRWANDER_REVERSED("RideAroundTrack::GoTo_61F550", 0x0061F550, &RideAroundTrack_GoTo);
TRWANDER_REVERSED("HaltVehicleTrack::Update", 0x0061FD10, &HaltVehicleTrack_Update);
TRWANDER_REVERSED("FleeTrack::Start", 0x0061FDE0, &FleeTrack_Start);
TRWANDER_REVERSED("FleeTrack::Update", 0x0061FEE0, &FleeTrack_Update);
TRWANDER_REVERSED("FleeTrack::End", 0x0061FFD0, &FleeTrack_End);
TRWANDER_REVERSED("HangOutTrack::Update", 0x0061FFF0, &HangOutTrack_Update);
TRWANDER_REVERSED("HangOutTrack::End", 0x00620470, &HangOutTrack_End);
TRWANDER_REVERSED("SpectatorTrack::Update", 0x00620560, &SpectatorTrack_Update);
TRWANDER_REVERSED("SpectatorTrack::End", 0x006206B0, &SpectatorTrack_End);
TRWANDER_REVERSED("GenerateDynamicPOITrack::Start", 0x00620700, &GenerateDynamicPOITrack_Start);
TRWANDER_REVERSED("JoinDynamicPOITrack::Start", 0x006207C0, &JoinDynamicPOITrack_Start);
TRWANDER_REVERSED("FaceDirectionTrack::Face_620820", 0x00620820, &FaceDirection_Face);
TRWANDER_REVERSED("FaceStimulusTrack::Update", 0x006208E0, &FaceStimulusTrack_Update);
TRWANDER_REVERSED("LookAroundTrack::LookAt_620980", 0x00620980, &LookAroundTrack_LookAt);
TRWANDER_REVERSED("LookAroundTrack::Update", 0x00620BB0, &LookAroundTrack_Update);
TRWANDER_REVERSED("LookAroundTrack::End", 0x00620C00, &LookAroundTrack_End);
TRWANDER_REVERSED("MoveToPOITrack::Update", 0x00620C30, &MoveToPOITrack_Update);
TRWANDER_REVERSED("ClearPOITrack::Update", 0x00620DB0, &ClearPOITrack_Update);
TRWANDER_REVERSED("SitDownTrack::Start", 0x00620DD0, &SitDownTrack_Start);
TRWANDER_REVERSED("SitDownTrack::Update", 0x00620E60, &SitDownTrack_Update);
TRWANDER_REVERSED("SitDownTrack::End", 0x00620EF0, &SitDownTrack_End);
TRWANDER_REVERSED("FaceTargetTrack::Update", 0x00620F20, &FaceTargetTrack_Update);
TRWANDER_REVERSED("FaceVehicleForEntryTrack::Update", 0x006211D0, &FaceVehicleForEntryTrack_Update);
TRWANDER_REVERSED("FaceDiveEntityTrack::Update", 0x006213A0, &FaceDiveEntityTrack_Update);
TRWANDER_REVERSED("FaceHitDirectionTrack::Update", 0x006214B0, &FaceHitDirectionTrack_Update);
TRWANDER_REVERSED("MoveToStimulusTrack::Start", 0x00621530, &MoveToStimulusTrack_Start);
TRWANDER_REVERSED("MoveToStimulusTrack::Update", 0x006215C0, &MoveToStimulusTrack_Update);
TRWANDER_REVERSED("InvestigateAreaTrack::Start", 0x00621660, &InvestigateAreaTrack_Start);
