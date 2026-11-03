// A ped's navigator (the ped's +0xDAC, 0x560 bytes; PedSteering.cpp's "path follower"):
// it keeps a route over the nav meshes to a target (or away from a position), walks it
// (the next point, looking ahead along it, opening doors on the way) and gives the ped
// its move direction and speed. 0x5E2750..0x5E4DC0.
//
// Its +0x438 is the ped's avoidance of others (PedAvoidance.cpp; +0x494 the entities
// in the way, +0x4A4 their count).

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

namespace {

struct CVector {
    float x, y, z;
};
struct NavPoint {
    re::Ptr32<uint8_t> col;  // +0x0
    int32_t tri;   // +0x4
    CVector pos;   // +0x8
};
static_assert(sizeof(NavPoint) == 0x14);

// The route (PedNavRoute.cpp)
struct CPedRoute {
    uint8_t unknown0[0x24];
    int32_t status;            // +0x24
    uint8_t unknown28[0x42C - 0x28];
    int32_t count;             // +0x42C
    uint32_t unknown430;
};
static_assert(sizeof(CPedRoute) == 0x434);

struct CPedNavigator {
    uint8_t dirty;               // +0x0 (update again)
    uint8_t pad1[3];
    CPedRoute route;             // +0x4 (the last point is the next to walk to)
    re::Ptr32<uint8_t> avoidancePed;       // +0x438 (the avoidance's ped)
    uint32_t avoidance43C;       // +0x43C
    uint32_t avoidance440;       // +0x440
    uint8_t avoidance444[0x450 - 0x444];
    float avoidanceSpeed;        // +0x450
    uint8_t avoidance454[0x494 - 0x454];
    re::Ptr32<uint8_t> inTheWay[4];        // +0x494
    uint8_t inTheWayCount;       // +0x4A4
    uint8_t avoidanceFlags;      // +0x4A5
    uint8_t avoidance4A6[0x4B4 - 0x4A6];
    re::Ptr32<uint8_t> ped;                // +0x4B4
    int32_t fleeing;             // +0x4B8 (1: away from fleeFrom)
    NavPoint current;            // +0x4BC (where the ped is on the meshes)
    NavPoint door;               // +0x4D0 (a door on the way)
    CVector target;              // +0x4E4
    CVector lastTarget;          // +0x4F0 (what the route was made for)
    CVector fleeFrom;            // +0x4FC
    CVector override;            // +0x508 (unset: none)
    uint8_t currentEntity[8];    // +0x514 ({entity, value} of the current mesh)
    uint8_t targetEntity[8];     // +0x51C
    uint8_t retarget;            // +0x524
    uint8_t pad525[3];
    float arriveRadius;          // +0x528 (0.3)
    uint32_t planFrame;          // +0x52C
    uint32_t updateFrame;        // +0x530
    float speed;                 // +0x534
    uint8_t unknown538[5];
    uint8_t noTether;            // +0x53D
    uint8_t blocked;             // +0x53E (the target is outside the ped's tether)
    uint8_t useRoute;            // +0x53F
    uint8_t unknown540;          // +0x540
    uint8_t unknown541;          // +0x541
    uint8_t pad542[2];
    float fleeGiveUp;            // +0x544
    float fleeWant;              // +0x548
    float lookAhead;             // +0x54C
    uint8_t unknown550;
    uint8_t atDoor;              // +0x551
    uint8_t navFlags;            // +0x552 (1 doors)
    uint8_t navArea;             // +0x553
    uint8_t frontAdds;           // +0x554
    uint8_t pad555[3];
    re::Ptr32<uint8_t> doorRefs[2];        // +0x558 (registered)
};
static_assert(offsetof(CPedNavigator, avoidancePed) == 0x438);
static_assert(offsetof(CPedNavigator, avoidanceSpeed) == 0x450);
static_assert(offsetof(CPedNavigator, inTheWay) == 0x494);
static_assert(offsetof(CPedNavigator, ped) == 0x4B4);
static_assert(offsetof(CPedNavigator, current) == 0x4BC);
static_assert(offsetof(CPedNavigator, target) == 0x4E4);
static_assert(offsetof(CPedNavigator, override) == 0x508);
static_assert(offsetof(CPedNavigator, arriveRadius) == 0x528);
static_assert(offsetof(CPedNavigator, speed) == 0x534);
static_assert(offsetof(CPedNavigator, fleeGiveUp) == 0x544);
static_assert(offsetof(CPedNavigator, navFlags) == 0x552);
static_assert(sizeof(CPedNavigator) == 0x560);

template <typename T>
re::Field<T>& F(const void* base, int offset) {
    return *reinterpret_cast<re::Field<T>*>(reinterpret_cast<uintptr_t>(base) + offset);
}

GAME_VAR(CVector, nv_vecUnset, 0x00BCC564);
GAME_VAR(uint32_t, nv_frameCounter, 0x00C1A99C);
GAME_VAR(float, nv_pedRadius, 0x00AEB940);
inline CPedRoute* const nv_finderRoute = reinterpret_cast<CPedRoute*>(0x00D09660);
GAME_VAR(int32_t, nv_finderRouteStatus, 0x00D09684);
inline void* const nv_doorManager = reinterpret_cast<void*>(0x00C3B808);

GAME_FN(CVector*, __cdecl, NV_VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, NV_VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, NV_Normalise, 0x00414E10, CVector* v);
GAME_FN(float, __thiscall, NV_Normalise2, 0x004294A0, CVector* v);
GAME_FN(CVector*, __cdecl, NV_Flat2DScaled, 0x0041A520, CVector* out, const CVector* v, float scale);
GAME_FN(float, __cdecl, NV_Heading, 0x00433310, const CVector* v);
GAME_FN(float, __cdecl, NV_AngleDiff, 0x00432980, float a, float b);
GAME_FN(CVector*, __cdecl, NV_VecFromHeading, 0x00432DF0, CVector* out, float heading);
GAME_FN(float, __cdecl, NV_Distance41A950, 0x0041A950, const CVector* a, const CVector* b);
GAME_FN(float, __cdecl, NV_Distance, 0x005E28D0, const CVector* a, const CVector* b);
GAME_FN(float, __cdecl, NV_DistanceCopy, 0x005E2940, const CVector* a, const CVector* b);
GAME_FN(float, __cdecl, NV_SqrtAbs, 0x0040DF90, float v);
GAME_FN(float, __cdecl, NV_Max, 0x00414490, float a, float b);
GAME_FN(uint8_t, __cdecl, NV_VectorNotEqual, 0x004305F0, const CVector* a, const CVector* b);
GAME_FN(CVector*, __thiscall, NV_GetForward, 0x004154A0, void* placeable, CVector* out);
GAME_FN(CVector*, __thiscall, NV_GetBoundCentre, 0x00466C70, void* entity, CVector* out);
GAME_FN(uint8_t, __thiscall, NV_IsFrameForRandomUpdate, 0x00465A60, void* entity, int);
GAME_FN(void*, __cdecl, NV_GetColModel, 0x0051AF50, void* entity);
GAME_FN(void, __cdecl, NV_CleanUpOldReference, 0x004657E0, void* entity, re::Ptr32<uint8_t>* ref);
GAME_FN(void, __thiscall, NV_EntityRefSet, 0x00478550, re::Ptr32<uint8_t>* ref, void* entity);
GAME_FN(void, __thiscall, NV_DoorManager_AddPush, 0x004E9CB0, void* manager, void* ped, void* door);
GAME_FN(void, __thiscall, NV_PropAnim_GetFacing, 0x004EA8C0, void* prop, void* ped, uint8_t* inFront, uint8_t* behind);
GAME_FN(uint8_t, __thiscall, NV_Tether_IsPosAllowed, 0x00499840, void* tether, const CVector* pos, float range);
GAME_FN(uint8_t, __thiscall, NV_Ped_473CE0, 0x00473CE0, void* ped);
GAME_FN(void, __thiscall, NV_Ped_477B70, 0x00477B70, void* ped);
GAME_FN(void, __thiscall, NV_MoveState_5F3A90, 0x005F3A90, void* moveState, int, int, float);
GAME_FN(uint8_t, __thiscall, NV_Avoidance_5DDC90_x64, 0x005DDC90, void* avoidance, CVector* out, re::Float3, float radius);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t NV_Avoidance_5DDC90(void* a0, CVector* a1, float a2, float a3, float a4, float a5) {
    return NV_Avoidance_5DDC90_x64(a0, a1, re::Float3{a2, a3, a4}, a5);
}
GAME_FN(uint8_t, __thiscall, NV_Avoidance_IsBlocked, 0x005E28B0, void* avoidance);
GAME_FN(uint8_t, __cdecl, NV_FindNavPoint, 0x005EB020, const CVector* pos, NavPoint* out, void* entity, uint8_t* offMesh);
GAME_FN(uint8_t, __cdecl, NV_FindNavPointNear, 0x005EAEB0, const CVector* pos, NavPoint* out, float, void* entity,
        uint8_t* offMesh, float);
GAME_FN(uint8_t, __cdecl, NV_WalkStraight, 0x005E9C30, NavPoint* from, NavPoint* to, int flags, int area, int check,
        NavPoint* door);
GAME_FN(uint8_t, __cdecl, NV_WalkToward, 0x005E9F50, NavPoint* from, const CVector* to, int flags, NavPoint* out,
        int area, int doorWanted, uintptr_t);
GAME_FN(uint8_t, __cdecl, NV_SearchPositions, 0x005E2460, CPedRoute* route, const CVector* start, const CVector* goal,
        int flags, void* startEntity, uint32_t maxDepth, int area, int b, NavPoint* door);
GAME_FN(uint8_t, __cdecl, NV_SearchPoints, 0x005E2500, CPedRoute* route, const NavPoint* start, const NavPoint* goal,
        int flags, uint32_t maxDepth, int area, int b, NavPoint* door);
GAME_FN(uint8_t, __cdecl, NV_FleePositions, 0x005E25B0, CPedRoute* route, const CVector* start, const CVector* from,
        float giveUpDist, float wantDist, int flags, NavPoint* door);
GAME_FN(void, __thiscall, NV_Route_Construct, 0x005E0F00, CPedRoute* self);
GAME_FN(void, __thiscall, NV_Route_Clear, 0x005E0E60, CPedRoute* self);
GAME_FN(void, __thiscall, NV_Route_Add, 0x005E0D10, CPedRoute* self, const NavPoint* point);
GAME_FN(uint8_t, __thiscall, NV_Route_AddFront, 0x005E0D70, CPedRoute* self, const NavPoint* point);
GAME_FN(void, __thiscall, NV_Route_Append, 0x005E0F80, CPedRoute* self, const CPedRoute* other);
GAME_FN(NavPoint*, __thiscall, NV_Route_Last, 0x005E0DA0, CPedRoute* self);
GAME_FN(NavPoint*, __thiscall, NV_Route_BeforeLast, 0x005E0DC0, CPedRoute* self);
GAME_FN(NavPoint*, __thiscall, NV_Route_First, 0x005E0DE0, CPedRoute* self);
GAME_FN(void, __thiscall, NV_Route_RemoveFirst, 0x005E0DF0, CPedRoute* self);
GAME_FN(NavPoint*, __thiscall, NV_Route_Second, 0x005E0E10, CPedRoute* self);
GAME_FN(NavPoint*, __thiscall, NV_Route_Point, 0x005E0E20, CPedRoute* self, int index);
GAME_FN(void, __thiscall, NV_Route_RemoveLast, 0x005E0E40, CPedRoute* self);
GAME_FN(void, __thiscall, NV_Point_Set, 0x005E7970, NavPoint* self, uint8_t* col, int32_t tri);
GAME_FN(uint8_t, __thiscall, NV_Point_IsValid, 0x005E7AF0, const NavPoint* self);
GAME_FN(int, __thiscall, NV_Point_DoorLine, 0x005E7CA0, const NavPoint* self, void* line);
GAME_FN(uint8_t, __thiscall, NV_Point_IsDoorOpen, 0x005E7B90, const NavPoint* self, int line);
GAME_FN(CVector*, __thiscall, NV_Line_Normal, 0x005E7220, const void* line, CVector* out);
// the navigator's own
GAME_FN(void, __thiscall, NV_Reset, 0x005E2750, CPedNavigator* self);
GAME_FN(void, __thiscall, NV_UpdateOverride, 0x005E27A0, CPedNavigator* self);
GAME_FN(uint8_t, __thiscall, NV_NextAfterPos, 0x005E2880, CPedNavigator* self, CVector* out);
GAME_FN(void, __thiscall, NV_Clear, 0x005E2970, CPedNavigator* self);
GAME_FN(uint8_t, __thiscall, NV_FindCurrent, 0x005E2B20, CPedNavigator* self);
GAME_FN(uint8_t, __thiscall, NV_CheckTether, 0x005E2B70, CPedNavigator* self);
GAME_FN(uint8_t, __thiscall, NV_Advance, 0x005E2BD0, CPedNavigator* self);
GAME_FN(void, __thiscall, NV_SetLookAhead, 0x005E2D10, CPedNavigator* self, const CVector* from, const CVector* to);
GAME_FN(uint8_t, __thiscall, NV_Retarget, 0x005E2DD0, CPedNavigator* self);
GAME_FN(uint8_t, __thiscall, NV_GetTarget, 0x005E3110, CPedNavigator* self, CVector* out);
GAME_FN(uint8_t, __thiscall, NV_ReachedNext, 0x005E3410, CPedNavigator* self);
GAME_FN(void, __thiscall, NV_AdvanceAndLook, 0x005E3630, CPedNavigator* self);
GAME_FN(void, __thiscall, NV_LookFurther, 0x005E3690, CPedNavigator* self);
GAME_FN(uint8_t, __thiscall, NV_Replan, 0x005E3A10, CPedNavigator* self);
GAME_FN(uint8_t, __thiscall, NV_Plan, 0x005E3AF0, CPedNavigator* self);
GAME_FN(void, __thiscall, NV_CheckDoorRef, 0x005E3CC0, CPedNavigator* self, re::Ptr32<uint8_t>* ref);
GAME_FN(void, __thiscall, NV_ThroughDoor, 0x005E3DE0, CPedNavigator* self, CVector* outPos, CVector* outDir);
GAME_FN(void, __thiscall, NV_NextPoint, 0x005E4450, CPedNavigator* self);
GAME_FN(void, __thiscall, NV_CheckDoors, 0x005E4540, CPedNavigator* self);
GAME_FN(void, __thiscall, NV_Update, 0x005E4920, CPedNavigator* self);
GAME_FN(uint8_t, __thiscall, NV_GetMoveDir, 0x005E4AD0, CPedNavigator* self, CVector* out);
GAME_FN(float, __thiscall, NV_GetSpeed, 0x005E4BC0, CPedNavigator* self, const CVector* dir);

const CVector& EntityPos(const void* entity) {
    const uint8_t* matrix = F<const uint8_t*>(entity, 0x14);
    return matrix ? *reinterpret_cast<const CVector*>(matrix + 0x30) : F<const CVector>(entity, 4);
}
uint8_t TriArea(const NavPoint& p) {
    uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(p.col + 0x38);
    return (*reinterpret_cast<re::Ptr32<uint8_t>*>(data + 0x10))[p.tri * 8 + 6];
}
float FlatLength(const CVector& d) {
    float lengthSq = std::fabs(float(double(d.x) * d.x + double(d.y) * d.y));
    return float(std::sqrt(double(lengthSq)));
}
NavPoint* DoorArg(CPedNavigator* self) {
    return (self->navFlags & 1) ? &self->door : nullptr;
}
bool IsUnset(const CVector& v) {
    return !(nv_vecUnset.x != v.x || nv_vecUnset.y != v.y || nv_vecUnset.z != v.z);
}

// 0x5E2750 (thiscall): empties the route, forgets the door and the override
void __fastcall Reset(CPedNavigator* self) {
    NV_Route_Clear(&self->route);
    self->frontAdds = 0;
    NV_Point_Set(&self->door, nullptr, -1);
    self->atDoor = 0;
    self->override = nv_vecUnset;
}

// 0x5E27A0 (thiscall): the avoidance's override when the route has one point left
void __fastcall UpdateOverride(CPedNavigator* self) {
    if (!NV_Route_BeforeLast(&self->route)) {
        float radius = vc::Call<float>(self->ped, 0x74);
        if (NV_Avoidance_5DDC90(&self->avoidancePed, &self->override, self->target.x, self->target.y,
                                self->target.z, radius)) {
            return;
        }
    }
    self->override = nv_vecUnset;
}

// 0x5E2820 (thiscall): flee from a position
void __fastcall SetFlee(CPedNavigator* self, re::Edx, const CVector* from, float giveUpDist, float wantDist) {
    if (self->fleeing == 0) {
        NV_Reset(self);
    }
    self->fleeFrom = *from;
    self->fleeGiveUp = giveUpDist;
    self->fleeWant = wantDist;
    self->fleeing = 1;
}

// 0x5E2880 (thiscall): the route point after the next one
uint8_t __fastcall NextAfterPos(CPedNavigator* self, re::Edx, CVector* out) {
    NavPoint* point = NV_Route_BeforeLast(&self->route);
    if (point) {
        *out = point->pos;
        return 1;
    }
    return 0;
}

// 0x5E2970 (thiscall): back to no target
void __fastcall Clear(CPedNavigator* self) {
    self->fleeing = 0;
    self->target = nv_vecUnset;
    self->lastTarget = nv_vecUnset;
    self->arriveRadius = 0.3f;
    self->lookAhead = 0.0f;
    self->retarget = 0;
    self->updateFrame = 0;
    self->noTether = 0;
    self->useRoute = 1;
    self->unknown541 = 0;
    NV_Reset(self);
}

// 0x5E2A00 (thiscall): a new target (and its entity): replanned when there's no route
// and the ped isn't there yet, retargeted when it moved
void __fastcall SetTarget(CPedNavigator* self, re::Edx, const CVector* target, const uint32_t* entity) {
    if (entity) {
        F<uint32_t>(self->targetEntity, 0) = entity[0];
        F<uint32_t>(self->targetEntity, 4) = entity[1];
    }
    self->target = *target;
    if (self->fleeing == 1) {
        NV_Reset(self);
        self->fleeing = 0;
    }
    if (self->route.count == 0) {
        CVector d;
        NV_VectorSub(&d, target, &EntityPos(self->ped));
        float distSq = float(double(d.x) * d.x + double(d.y) * d.y);
        float radiusSq = self->arriveRadius * self->arriveRadius;
        if (radiusSq < distSq) {
            self->lastTarget = self->target;
            self->dirty = 1;
        }
    } else if (NV_VectorNotEqual(target, &self->lastTarget)) {
        self->retarget = 1;
    }
}

// 0x5E2B20 (thiscall): where the ped is on the meshes
uint8_t __fastcall FindCurrent(CPedNavigator* self) {
    CVector pos = EntityPos(self->ped);
    uint8_t offMesh;
    return NV_FindNavPoint(&pos, &self->current, self->currentEntity, &offMesh);
}

// 0x5E2B70 (thiscall): is the target outside the ped's tether?
uint8_t __fastcall CheckTether(CPedNavigator* self) {
    self->blocked = 0;
    if (!self->noTether && F<uint32_t>(self->ped, 0x360) != 0) {
        if (!NV_Tether_IsPosAllowed(self->ped + 0x358, &self->target, 0.1f)) {
            self->blocked = 1;
        }
    }
    return self->blocked;
}

// 0x5E2BD0 (thiscall): drops the next points while the one after can be walked to
// straight (not into the area when the flags ask for it)
uint8_t __fastcall Advance(CPedNavigator* self) {
    uint8_t advanced = 0;
    NavPoint* next = NV_Route_BeforeLast(&self->route);
    if (!next) {
        return 0;
    }
    for (;;) {
        if (!NV_Point_IsValid(next)) {
            break;
        }
        uint8_t flags = self->navFlags;
        if ((flags & 4) && TriArea(*next) == self->navArea && TriArea(self->current) != self->navArea) {
            return 1;
        }
        NavPoint door = {nullptr, -1, {0.0f, 0.0f, 0.0f}};
        if (!NV_WalkStraight(&self->current, next, flags, self->navArea, 1, &door)) {
            break;
        }
        NV_Route_RemoveLast(&self->route);
        advanced = 1;
        if (self->navFlags & 1) {
            self->door = door;
        }
        next = NV_Route_BeforeLast(&self->route);
        if (!next) {
            NV_UpdateOverride(self);
            return 1;
        }
    }
    if (advanced) {
        NV_UpdateOverride(self);
    }
    return advanced;
}

// 0x5E2D10 (thiscall): how far to look ahead: 0.6 of the flat distance (0 under 0.5),
// at least 2
void __fastcall SetLookAhead(CPedNavigator* self, re::Edx, const CVector* from, const CVector* to) {
    CVector d;
    NV_VectorSub(&d, to, from);
    float dist = FlatLength(d);
    self->lookAhead = dist;
    float value = 0.5f < dist ? float(dist * double(0.6f)) : 0.0f;
    self->lookAhead = value;
    self->lookAhead = value > 2.0f ? value : 2.0f;
}

// 0x5E2DD0 (thiscall): the target moved: the route's end point moved to it where the
// meshes allow
uint8_t __fastcall Retarget(CPedNavigator* self) {
    NavPoint* next = NV_Route_Second(&self->route);
    NavPoint* first = NV_Route_First(&self->route);
    NavPoint point = {nullptr, -1, {0.0f, 0.0f, 0.0f}};
    uint8_t isEnd = 0;
    if (!next) {
        next = &self->current;
        isEnd = 1;
    }
    self->retarget = 0;
    uint8_t offMesh = 0;
    NV_FindNavPointNear(&self->target, &point, 3.5f, self->targetEntity, &offMesh, 50.0f);
    if (offMesh) {
        if (next && next->tri == point.tri && next->col == point.col) {
            first->pos = self->target;
            return 1;
        }
        return 0;
    }
    uint8_t wasEnd1 = 0;
    uint8_t wasEnd2 = 0;
    if (isEnd) {
        self->frontAdds = 0;
        wasEnd2 = 1;
        wasEnd1 = 1;
    }
    if (!NV_Point_IsValid(next)) {
        return 0;
    }
    if (!isEnd) {
        NavPoint* third = NV_Route_Point(&self->route, 2);
        isEnd = 0;
        if (!third) {
            third = &self->current;
            isEnd = 1;
        }
        if (NV_Point_IsValid(third)) {
            NavPoint door = {nullptr, -1, {0.0f, 0.0f, 0.0f}};
            if (NV_WalkStraight(third, &point, self->navFlags, self->navArea, 1, &door)) {
                NV_Route_RemoveFirst(&self->route);
                *NV_Route_First(&self->route) = point;
                if (isEnd) {
                    if (self->navFlags & 1) {
                        self->door = door;
                    }
                    self->frontAdds = 0;
                    NV_UpdateOverride(self);
                }
                return 1;
            }
        }
    }
    NavPoint* doorArg = ((self->navFlags & 1) && wasEnd1) ? &self->door : nullptr;
    if (NV_WalkStraight(next, &point, self->navFlags, self->navArea, 1, doorArg)) {
        *first = point;
        if (wasEnd2) {
            CVector firstPos = {first->pos.x, first->pos.y, point.pos.z};
            CVector currentPos = self->current.pos;
            NV_SetLookAhead(self, &currentPos, &firstPos);
        }
        NV_UpdateOverride(self);
        return 1;
    }
    if (NV_Point_IsValid(first) && self->frontAdds < 1) {
        if (NV_WalkStraight(first, &point, self->navFlags, self->navArea, 1, nullptr)) {
            NV_Route_AddFront(&self->route, &point);
            self->frontAdds++;
            return 1;
        }
    }
    return 0;
}

// 0x5E3110 (thiscall): where to head for: the override, the next route point (or the
// current mesh point), or the target while the ped isn't within the arrive radius
uint8_t __fastcall GetTarget(CPedNavigator* self, re::Edx, CVector* out) {
    CVector pos = EntityPos(self->ped);
    if (!IsUnset(self->override)) {
        *out = self->override;
        return 1;
    }
    if (!self->useRoute) {
        CVector d;
        NV_VectorSub(&d, &self->target, &pos);
        float distSq = float(double(d.x) * d.x + double(d.y) * d.y);
        float radiusSq = self->arriveRadius * self->arriveRadius;
        if (!(radiusSq < distSq)) {
            return 0;
        }
        *out = self->target;
        return 1;
    }
    NavPoint* last = NV_Route_Last(&self->route);
    if (last) {
        if (last->col == nullptr) {
            last = &self->current;
        }
        *out = last->pos;
        return 1;
    }
    *out = self->target;
    return 0;
}

// 0x5E32A0 (thiscall): the constructor
CPedNavigator* __fastcall Construct(CPedNavigator* self, re::Edx, uint8_t* ped) {
    NV_Route_Construct(&self->route);
    self->avoidancePed = ped;
    self->avoidance43C = 0;
    self->avoidance440 = 0;
    self->inTheWay[0] = nullptr;
    self->inTheWay[1] = nullptr;
    self->inTheWay[2] = nullptr;
    self->inTheWay[3] = nullptr;
    self->ped = ped;
    self->fleeing = 0;
    self->current = {nullptr, -1, {0.0f, 0.0f, 0.0f}};
    self->door = {nullptr, -1, {0.0f, 0.0f, 0.0f}};
    self->target = nv_vecUnset;
    self->lastTarget = nv_vecUnset;
    self->fleeFrom = {0.0f, 0.0f, 0.0f};
    self->override = nv_vecUnset;
    F<uint32_t>(self->currentEntity, 0) = 0;
    F<int32_t>(self->currentEntity, 4) = -1;
    F<uint32_t>(self->targetEntity, 0) = 0;
    F<int32_t>(self->targetEntity, 4) = -1;
    self->arriveRadius = 0.3f;
    self->retarget = 0;
    self->updateFrame = 0;
    self->lookAhead = 0.0f;
    self->unknown541 = 0;
    self->atDoor = 0;
    self->frontAdds = 0;
    self->noTether = 1;
    self->useRoute = 1;
    self->doorRefs[0] = nullptr;
    self->doorRefs[1] = nullptr;
    return self;
}

// 0x5E3410 (thiscall): has the ped reached the next point (within the radius, 0.35 at
// least with more to go, and the one after walkable), or gone past it?
uint8_t __fastcall ReachedNext(CPedNavigator* self) {
    CVector pos = EntityPos(self->ped);
    CVector target = {0.0f, 0.0f, 0.0f};
    if (!NV_GetTarget(self, &target)) {
        return 0;
    }
    CVector after = {0.0f, 0.0f, 0.0f};
    NavPoint* afterPoint = NV_Route_BeforeLast(&self->route);
    bool hasAfter = afterPoint != nullptr;
    if (hasAfter) {
        after = afterPoint->pos;
    }
    CVector d;
    NV_VectorSub(&d, &target, &pos);
    float radius = self->arriveRadius;
    if (hasAfter) {
        radius = radius > 0.35f ? radius : 0.35f;
    }
    float distSq = float(double(d.x) * d.x + double(d.y) * d.y);
    float radiusSq = radius * radius;
    if (!(radiusSq < distSq)) {
        NavPoint* next = NV_Route_BeforeLast(&self->route);
        if (!next || !NV_Point_IsValid(next)) {
            return 1;
        }
        if (NV_WalkStraight(&self->current, next, self->navFlags, self->navArea, 0, nullptr)) {
            return 1;
        }
    }
    if (!hasAfter) {
        return 0;
    }
    CVector toAfter;
    NV_VectorSub(&toAfter, &after, &pos);
    CVector afterDir;
    NV_Flat2DScaled(&afterDir, &toAfter, 1.0f);
    d.z = 0.0f;
    NV_Normalise(&d);
    float dot = float(double(afterDir.y) * d.y + double(afterDir.x) * d.x + double(afterDir.z) * d.z);
    if (dot < 0.0f) {
        NavPoint* next = NV_Route_BeforeLast(&self->route);
        if (NV_Point_IsValid(next)
            && NV_WalkStraight(&self->current, next, self->navFlags, self->navArea, 0, nullptr)) {
            return 1;
        }
    }
    return 0;
}

// 0x5E3630 (thiscall): advances, then sets the look-ahead from where the ped was
void __fastcall AdvanceAndLook(CPedNavigator* self) {
    CVector from = self->current.pos;
    NV_Advance(self);
    CVector target = {0.0f, 0.0f, 0.0f};
    if (NV_GetTarget(self, &target)) {
        NV_SetLookAhead(self, &from, &target);
    }
}

// 0x5E3690 (thiscall): when the next point is within the look-ahead: advances, or moves
// it along the way to the one after (doubling the step while the heading stays within
// 30 degrees and the meshes allow)
void __fastcall LookFurther(CPedNavigator* self) {
    if (self->route.count <= 1) {
        return;
    }
    CVector last = NV_Route_Last(&self->route)->pos;
    CVector current = self->current.pos;
    CVector toLast;
    NV_VectorSub(&toLast, &last, &current);
    float dist = FlatLength(toLast);
    if (!(self->lookAhead > dist)) {
        return;
    }
    if (NV_Advance(self)) {
        last = NV_Route_Last(&self->route)->pos;
    } else {
        float step = 0.5f;
        CVector after = NV_Route_BeforeLast(&self->route)->pos;
        CVector dir;
        NV_VectorSub(&dir, &after, &last);
        float length = FlatLength(dir);
        NavPoint best = {nullptr, -1, {0.0f, 0.0f, 0.0f}};
        NavPoint found = {nullptr, -1, {0.0f, 0.0f, 0.0f}};
        bool any = false;
        CVector bestPos = {};
        float heading = NV_Heading(&toLast);
        double lengthD = length;
        if (length > 0.5) {
            do {
                float t = float(step / length);
                CVector along = {t * dir.x, dir.y * t, t * dir.z};
                CVector candidate;
                NV_VectorAdd(&candidate, &last, &along);
                CVector toCandidate;
                NV_VectorSub(&toCandidate, &candidate, &current);
                float diff = std::fabs(NV_AngleDiff(heading, NV_Heading(&toCandidate)));
                if (!(diff < 0.5235988f)) {
                    break;
                }
                int doorWanted = (self->navFlags & 1) ? 1 : 0;
                if (!NV_WalkToward(&self->current, &candidate, self->navFlags, &found, self->navArea, doorWanted, 0)) {
                    break;
                }
                step = step + step;
                bestPos = candidate;
                best = found;
                any = true;
            } while (step < lengthD);
            if (any) {
                NV_Route_RemoveLast(&self->route);
                NV_Route_Add(&self->route, &best);
                last = bestPos;
            }
        }
    }
    NV_SetLookAhead(self, &current, &last);
}

// 0x5E3A10 (thiscall): the next point unreachable straight: a search from the current
// point to it, spliced in (with maxDepth the area byte: the game's argument order)
uint8_t __fastcall Replan(CPedNavigator* self) {
    self->planFrame = nv_frameCounter;
    NavPoint* last = NV_Route_Last(&self->route);
    if (!NV_Point_IsValid(last)) {
        return 0;
    }
    uint8_t area = self->navArea;
    if (area != 0 && TriArea(self->current) != area) {
        return 0;
    }
    if (NV_WalkStraight(&self->current, last, self->navFlags, area, 0, DoorArg(self))) {
        return 1;
    }
    if (!NV_SearchPoints(nv_finderRoute, &self->current, last, 4, self->navArea, 0, 1, nullptr)
        || nv_finderRouteStatus != 2) {
        return 0;
    }
    NV_Route_Append(&self->route, nv_finderRoute);
    NV_Ped_477B70(self->ped);
    NV_AdvanceAndLook(self);
    return 1;
}

// 0x5E3AF0 (thiscall): a new route to the target (or away from fleeFrom); not when the
// ped is within the radius and 3.5 in height of it
uint8_t __fastcall Plan(CPedNavigator* self) {
    CVector pos = EntityPos(self->ped);
    NV_Reset(self);
    uint8_t ok;
    if (self->fleeing == 1) {
        self->lastTarget = nv_vecUnset;
        ok = NV_FleePositions(&self->route, &pos, &self->fleeFrom, self->fleeGiveUp, self->fleeWant, self->navFlags,
                              DoorArg(self));
    } else {
        self->lastTarget = self->target;
        CVector d;
        NV_VectorSub(&d, &self->lastTarget, &pos);
        float distSq = float(double(d.x) * d.x + double(d.y) * d.y);
        float radiusSq = self->arriveRadius * self->arriveRadius;
        if (radiusSq > distSq) {
            float height = std::fabs(d.z);
            if (height < 3.5) {
                self->retarget = 0;
                return 0;
            }
        }
        ok = NV_SearchPositions(&self->route, &pos, &self->target, self->navFlags, self->currentEntity, 50,
                                self->navArea, self->unknown540, DoorArg(self));
    }
    if (ok) {
        NV_Ped_477B70(self->ped);
        NV_AdvanceAndLook(self);
        NV_UpdateOverride(self);
        self->planFrame = nv_frameCounter;
    }
    self->retarget = 0;
    return ok;
}

// 0x5E3CC0 (thiscall): a door the ped is near (2.1) is pushed open, else forgotten
void __fastcall CheckDoorRef(CPedNavigator* self, re::Edx, re::Ptr32<uint8_t>* ref) {
    uint8_t* hinge = F<uint8_t*>(*ref, 0x23C);
    CVector centre;
    NV_GetBoundCentre(*ref, &centre);
    centre.z = EntityPos(*ref).z;
    CVector pos = EntityPos(self->ped);
    CVector d;
    NV_VectorSub(&d, &pos, &centre);
    float lengthSq = float(double(d.x) * d.x + double(d.y) * d.y + double(d.z) * d.z);
    float dist = float(std::sqrt(double(std::fabs(lengthSq))));
    if (!(dist < 2.1f)) {
        if (!(NV_Distance41A950(&pos, reinterpret_cast<CVector*>(hinge + 4)) < 2.1f)) {
            if (*ref) {
                NV_CleanUpOldReference(*ref, ref);
                *ref = nullptr;
            }
            return;
        }
    }
    NV_DoorManager_AddPush(nv_doorManager, self->ped, *ref);
}

// 0x5E3DE0 (thiscall): steering through the door on the route: which side of its line
// the ped is on, and where along it to pass (0.35 in from either end); outDir comes in
// as the move direction
void __fastcall ThroughDoor(CPedNavigator* self, re::Edx, CVector* outPos, CVector* outDir) {
    uint8_t line[0x20];
    NV_Point_DoorLine(&self->door, line);
    const CVector& a = *reinterpret_cast<CVector*>(line);
    const CVector& b = *reinterpret_cast<CVector*>(line + 0x10);
    CVector n;
    CVector* normal = NV_Line_Normal(line, &n);
    CVector nn = *normal;
    NV_Normalise2(&nn);
    CVector away = {-nn.x, -nn.y, -nn.z};
    CVector offset = {float(away.x * double(0.4f)), float(away.y * double(0.4f)), float(double(0.4f) * away.z)};
    CVector pos = EntityPos(self->ped);
    CVector toA = {a.x - pos.x, a.y - pos.y, 0.0f};
    float dot = float(double(away.y) * toA.y + double(away.x) * toA.x + away.z * 0.0);
    float side = -dot;
    if (!(0.0f < side)) {
        NV_Point_Set(&self->door, nullptr, -1);
        self->atDoor = 0;
        F<uint32_t>(self->ped, 0x1514) = 0;
        return;
    }
    CVector along = {b.x - a.x, b.y - a.y, 0.0f};
    float length = FlatLength(along);
    NV_Normalise2(&along);
    float alongDot = float(double(along.x) * toA.x + double(along.y) * toA.y + along.z * 0.0);
    float u = float(-double(alongDot) / length);
    float moveHeading = NV_Heading(outDir);
    float fromB = float(length * (1.0 - u) - double(0.35f));
    float fromA = float(length * double(u) - double(0.35f));
    float heading;
    float scale;
    if (!(0.0f > fromB) && !(0.0f > fromA)) {
        F<uint32_t>(self->ped, 0x1514) = 1;
        *outDir = {-away.x, -away.y, -away.z};
    } else {
        CVector v;
        float distV = 0.0f;
        if (u > 0.5f) {
            // (past the middle: pass by B)
            if (side > double(0.4f)) {
                CVector q = {b.x + offset.x, b.y + offset.y, b.z + offset.z};
                v = {q.x - pos.x, q.y - pos.y, q.z - pos.z};
                distV = FlatLength(v);
                if (0.35f < distV) {
                    float h = NV_Heading(&v);
                    float s = float(std::asin(double(float(double(0.35f) / distV))));
                    float w = float(h - s);
                    if (!(NV_AngleDiff(w, moveHeading) > 0.0f)) {
                        goto doorDone;
                    }
                    heading = w;
                    scale = distV;
                    goto steer;
                }
            }
            CVector back = {-along.x, -along.y, -along.z};
            heading = NV_Heading(&back);
            scale = -fromB;
        } else {
            if (side > double(0.4f)) {
                CVector q = {a.x + offset.x, a.y + offset.y, a.z + offset.z};
                v = {q.x - pos.x, q.y - pos.y, q.z - pos.z};
                distV = FlatLength(v);
                if (0.35f < distV) {
                    float h = NV_Heading(&v);
                    float s = float(std::asin(double(float(double(0.35f) / distV))));
                    float w = float(s + h);
                    if (!(NV_AngleDiff(w, moveHeading) < 0.0f)) {
                        goto doorDone;
                    }
                    heading = w;
                    scale = distV;
                    goto steer;
                }
            }
            heading = NV_Heading(&along);
            scale = -fromA;
        }
    steer: {
        CVector dir;
        *outDir = *NV_VecFromHeading(&dir, heading);
        CVector step = {outDir->x * scale, outDir->y * scale, scale * outDir->z};
        CVector result;
        *outPos = *NV_VectorAdd(&result, &pos, &step);
        return;
    }
    }
doorDone:
    if (side < double(0.4f)) {
        self->speed = 0.0f;
    }
    if (side < 2.5f) {
        const uint8_t* matrix = F<const uint8_t*>(self->ped, 0x14);
        float pedHeading;
        if (matrix) {
            pedHeading = float(std::atan2(double(-F<float>(matrix, 0x10)), double(F<float>(matrix, 0x14))));
        } else {
            pedHeading = F<float>(self->ped, 0x10);
        }
        CVector facing = {-away.x, -away.y, -away.z};
        float doorHeading = NV_Heading(&facing);
        float diff = std::fabs(NV_AngleDiff(pedHeading, doorHeading));
        if (diff < 0.39269909262657166) {
            self->atDoor = 1;
        }
    }
}

// 0x5E4450 (thiscall): the next point reached: dropped (planning again when it was the
// last), and the door taken from a door triangle ahead
void __fastcall NextPoint(CPedNavigator* self) {
    self->planFrame = nv_frameCounter;
    NV_Route_RemoveLast(&self->route);
    if (self->route.count == 0) {
        if (self->route.status == 1) {
            NV_Plan(self);
        } else {
            NV_Clear(self);
        }
    }
    NavPoint* next = NV_Route_Last(&self->route);
    if (!next) {
        return;
    }
    if ((self->navFlags & 1) && NV_Point_IsValid(next) && TriArea(*next) == 5 && TriArea(self->current) != 5) {
        int line = NV_Point_DoorLine(next, nullptr);
        if (NV_Point_IsDoorOpen(next, line)) {
            self->door = *next;
        }
        NV_UpdateOverride(self);
        return;
    }
    NV_Point_Set(&self->door, nullptr, -1);
    NV_UpdateOverride(self);
}

// 0x5E4540 (thiscall): doors in the way (the avoidance's entities the ped and its target
// are on either side of, within 2.1) are pushed open (two at most)
void __fastcall CheckDoors(CPedNavigator* self) {
    if (0.0f == self->speed) {
        return;
    }
    if (F<int32_t>(self->ped, 0x1310) == 0xD) {
        return;
    }
    re::Ptr32<uint8_t>* door1 = &self->doorRefs[0];
    if (*door1) {
        NV_CheckDoorRef(self, door1);
    }
    re::Ptr32<uint8_t>* door2 = &self->doorRefs[1];
    if (*door2) {
        NV_CheckDoorRef(self, door2);
    }
    if (*door1 && *door2) {
        return;
    }
    if (!NV_IsFrameForRandomUpdate(self->ped, 2)) {
        return;
    }
    CVector pos = EntityPos(self->ped);
    CVector target = {0.0f, 0.0f, 0.0f};
    NV_GetTarget(self, &target);
    CVector toTarget = {target.x - pos.x, target.y - pos.y, 0.0f};
    float distSq = float(double(toTarget.x) * toTarget.x + double(toTarget.y) * toTarget.y);
    if (distSq < 1.0f) {
        NV_NextAfterPos(self, &toTarget);
    }
    int count = self->inTheWayCount;
    for (int i = 0; i < count; i++) {
        uint8_t* entity = self->inTheWay[i];
        if (!entity) {
            continue;
        }
        CVector at = EntityPos(entity);
        CVector a;
        CVector t = target;
        NV_VectorSub(&a, &t, &at);
        a.z = 0.0f;
        NV_Normalise(&a);
        CVector forward;
        NV_GetForward(entity, &forward);
        forward.z = 0.0f;
        NV_Normalise(&forward);
        CVector p = pos;
        CVector b;
        NV_VectorSub(&b, &p, &at);
        b.z = 0.0f;
        NV_Normalise(&b);
        float dotA = float(double(a.y) * forward.y + double(forward.x) * a.x + double(forward.z) * a.z);
        float dotB = float(double(forward.y) * b.y + double(forward.x) * b.x + double(forward.z) * b.z);
        if ((dotA > 0.0f && dotB > 0.0f) || (dotB < 0.0f && dotA < 0.0f)) {
            continue;
        }
        uint8_t inFront = 0;
        uint8_t behind = 0;
        CVector centre;
        NV_GetBoundCentre(entity, &centre);
        centre.z = at.z;
        NV_PropAnim_GetFacing(entity, self->ped, &inFront, &behind);
        if (behind) {
            CVector atCopy = at;
            CVector p2 = pos;
            if (NV_Distance(&p2, &atCopy) < 2.1f) {
                if (*door1 == nullptr) {
                    NV_EntityRefSet(door1, entity);
                } else if (*door2 == nullptr) {
                    NV_EntityRefSet(door2, entity);
                }
                if (*door1 && *door2) {
                    return;
                }
                continue;
            }
        }
        if (inFront) {
            CVector p2 = pos;
            if (NV_DistanceCopy(&p2, &centre) < 2.1f
                && *door1 == nullptr) {
                NV_EntityRefSet(door1, entity);
                return;
            }
        }
    }
}

// 0x5E4920 (thiscall): the per-frame update (once a frame, or when dirty or retargeted)
void __fastcall Update(CPedNavigator* self) {
    if (!self->dirty && !self->retarget && self->updateFrame == nv_frameCounter) {
        return;
    }
    uint8_t* vehicle = F<uint8_t*>(self->ped, 0x1554);
    float radius;
    if (vehicle && F<int32_t>(vehicle, 0x46C) == 1) {
        uint8_t* col = static_cast<uint8_t*>(NV_GetColModel(vehicle));
        radius = std::fabs(float(F<float>(col, 0x10) * 1.25));
    } else {
        radius = vc::Call<float>(self->ped, 0x74);
    }
    nv_pedRadius = radius;
    if (!NV_FindCurrent(self)) {
        return;
    }
    if (NV_CheckTether(self)) {
        return;
    }
    if (self->useRoute) {
        CVector target;
        if (!NV_GetTarget(self, &target) && !NV_Plan(self)) {
            return;
        }
        if (nv_frameCounter - self->planFrame > 32 || NV_Avoidance_IsBlocked(&self->avoidancePed)) {
            float saved = nv_pedRadius;
            nv_pedRadius = 0.0f;
            uint8_t replanned = NV_Replan(self);
            nv_pedRadius = saved;
            if (replanned) {
                NV_Advance(self);
            } else if (!NV_Plan(self)) {
                return;
            }
        }
        if (self->retarget) {
            if (NV_Retarget(self)) {
                self->lastTarget = self->target;
            } else if (!NV_Plan(self)) {
                return;
            }
        }
        if (NV_ReachedNext(self)) {
            NV_NextPoint(self);
            NV_AdvanceAndLook(self);
        } else {
            NV_LookFurther(self);
        }
        NV_CheckDoors(self);
    }
    self->updateFrame = nv_frameCounter;
    self->dirty = 0;
}

// 0x5E4AD0 (thiscall): the direction to move in (normalised; through a door when there's
// one) and the avoidance's steering
uint8_t __fastcall GetMoveDir(CPedNavigator* self, re::Edx, CVector* out) {
    NV_Update(self);
    if (self->blocked) {
        return 0;
    }
    CVector target = {0.0f, 0.0f, 0.0f};
    if (!NV_GetTarget(self, &target)) {
        return 0;
    }
    CVector d;
    *out = *NV_VectorSub(&d, &target, &EntityPos(self->ped));
    NV_Normalise(out);
    if ((self->navFlags & 1) && NV_Point_IsValid(&self->door)) {
        NV_ThroughDoor(self, &target, out);
    }
    using Avoid = void(__thiscall*)(void*, float, float, float, CVector*, void*, float, int);
    reinterpret_cast<Avoid>(0x005E0B20)(&self->avoidancePed, target.x, target.y, target.z, out, &self->current,
                                         self->speed, self->unknown541);
    return 1;
}

// 0x5E4BC0 (thiscall): the speed: the avoidance's, slowed near the next point by the
// turn to it (cosine) and when very near (0.1 at least)
float __fastcall GetSpeed(CPedNavigator* self, re::Edx, const CVector* dir) {
    if (F<uint8_t>(self->ped, 0x4E4) || self->blocked) {
        return 0.0f;
    }
    NV_Update(self);
    CVector target = {0.0f, 0.0f, 0.0f};
    if (!NV_GetTarget(self, &target)) {
        return 0.0f;
    }
    float speed = self->avoidanceSpeed;
    if (speed < double(0.1f)) {
        return 0.0f;
    }
    if (NV_Ped_473CE0(self->ped) || NV_NextAfterPos(self, &target)) {
        return speed;
    }
    CVector pos = EntityPos(self->ped);
    CVector d;
    NV_VectorSub(&d, &target, &pos);
    float distSq = float(double(d.x) * d.x + double(d.y) * d.y);
    if (!(9.0f > distSq)) {
        return speed;
    }
    CVector forward;
    NV_GetForward(self->ped, &forward);
    float facing = NV_Heading(&forward);
    float want = NV_Heading(dir);
    float turn = std::fabs(NV_AngleDiff(facing, want));
    float result;
    if (turn > 3.1415927410125732) {
        result = 0.0f;
    } else {
        float c = float(std::cos(double(turn)));
        speed = c * speed;
        if (distSq < 0.09f) {
            float near = float(NV_SqrtAbs(distSq) / 0.30000001192092896);
            speed = near * speed;
        }
        result = NV_Max(speed, 0.1f);
    }
    speed = result;
    F<uint32_t>(self->ped, 0x1514) = 1;
    return speed;
}

// 0x5E4DC0 (thiscall): into the ped's move state (+0x2E0): direction twice, speed
uint8_t __fastcall Process(CPedNavigator* self) {
    CVector dir = {0.0f, 0.0f, 0.0f};
    if (!NV_GetMoveDir(self, &dir)) {
        return 0;
    }
    uint8_t* moveState = F<uint8_t*>(self->ped, 0x2E0);
    float speed = NV_GetSpeed(self, &dir);
    if ((self->navFlags & 1) && NV_Point_IsValid(&self->door) && self->atDoor) {
        NV_MoveState_5F3A90(moveState, 4, 1, 0.0f);
        self->atDoor = 0;
        F<uint32_t>(self->ped, 0x1514) = 0;
        if (speed < double(0.1f)) {
            speed = 0.1f;
        }
    }
    F<CVector>(moveState, 0) = dir;
    F<CVector>(moveState, 0xC) = dir;
    F<float>(moveState, 0x1C) = speed;
    return 1;
}

#define NAVIGATOR_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(NavigatorOriginal_, __LINE__);                                        \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(NavigatorOriginal_, __LINE__), \
             re::Mode::Original)

NAVIGATOR_REVERSED("CPedNavigator::Reset", 0x005E2750, &Reset);
NAVIGATOR_REVERSED("CPedNavigator::UpdateOverride", 0x005E27A0, &UpdateOverride);
NAVIGATOR_REVERSED("CPedNavigator::SetFlee", 0x005E2820, &SetFlee);
NAVIGATOR_REVERSED("CPedNavigator::NextAfterPos", 0x005E2880, &NextAfterPos);
NAVIGATOR_REVERSED("CPedNavigator::Clear", 0x005E2970, &Clear);
NAVIGATOR_REVERSED("CPedNavigator::SetTarget", 0x005E2A00, &SetTarget);
NAVIGATOR_REVERSED("CPedNavigator::FindCurrent", 0x005E2B20, &FindCurrent);
NAVIGATOR_REVERSED("CPedNavigator::CheckTether", 0x005E2B70, &CheckTether);
NAVIGATOR_REVERSED("CPedNavigator::Advance", 0x005E2BD0, &Advance);
NAVIGATOR_REVERSED("CPedNavigator::SetLookAhead", 0x005E2D10, &SetLookAhead);
NAVIGATOR_REVERSED("CPedNavigator::Retarget", 0x005E2DD0, &Retarget);
NAVIGATOR_REVERSED("CPedNavigator::GetTarget", 0x005E3110, &GetTarget);
NAVIGATOR_REVERSED("CPedNavigator::Construct", 0x005E32A0, &Construct);
NAVIGATOR_REVERSED("CPedNavigator::ReachedNext", 0x005E3410, &ReachedNext);
NAVIGATOR_REVERSED("CPedNavigator::AdvanceAndLook", 0x005E3630, &AdvanceAndLook);
NAVIGATOR_REVERSED("CPedNavigator::LookFurther", 0x005E3690, &LookFurther);
NAVIGATOR_REVERSED("CPedNavigator::Replan", 0x005E3A10, &Replan);
NAVIGATOR_REVERSED("CPedNavigator::Plan", 0x005E3AF0, &Plan);
NAVIGATOR_REVERSED("CPedNavigator::CheckDoorRef", 0x005E3CC0, &CheckDoorRef);
NAVIGATOR_REVERSED("CPedNavigator::ThroughDoor", 0x005E3DE0, &ThroughDoor);
NAVIGATOR_REVERSED("CPedNavigator::NextPoint", 0x005E4450, &NextPoint);
NAVIGATOR_REVERSED("CPedNavigator::CheckDoors", 0x005E4540, &CheckDoors);
NAVIGATOR_REVERSED("CPedNavigator::Update", 0x005E4920, &Update);
NAVIGATOR_REVERSED("CPedNavigator::GetMoveDir", 0x005E4AD0, &GetMoveDir);
NAVIGATOR_REVERSED("CPedNavigator::GetSpeed", 0x005E4BC0, &GetSpeed);
NAVIGATOR_REVERSED("CPedNavigator::Process", 0x005E4DC0, &Process);

} // namespace
