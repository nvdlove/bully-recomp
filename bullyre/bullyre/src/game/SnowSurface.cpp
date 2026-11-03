// The peds' snow conditions (winter: the chapter whose area props are 2): the snow
// surfaces, the snow zones, a random reachable point near a ped on a surface.
// 0x457AC0..0x457FC0.

#include <cstdint>

#include "game/Collision.h"
#include "game/World.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using col::CVector;

template <typename T>
re::Field<T>& Field(void* base, int offset) {
    return *reinterpret_cast<re::Field<T>*>(static_cast<uint8_t*>(base) + offset);
}

// ---- game data ----
GAME_VAR(int32_t, currArea, 0x00BD1008);          // CGame::currArea
GAME_VAR(uint8_t, currentChapter, 0x00A147E0);
GAME_ARRAY(int32_t, chapterAreaProps, 0x00A147EC);  // by chapter (2: snow)
GAME_VAR(float, twoPi, 0x00A149B4);
GAME_ARRAY(const char* const, snowZoneNames, 0x00A149A8);  // (to a null)
inline void* const zones = reinterpret_cast<void*>(0x020C7E98);
GAME_VAR(CPool<uint8_t>*, zonePool, 0x020C7C48);
GAME_VAR(void*, ignoreEntity, 0x00C1AE70);        // (the line tests skip it)

// ---- the game's ----
GAME_FN(uint8_t, __cdecl, Area_430320, 0x00430320, int area);   // (Game.cpp's CGame::IsAreaInterior)
inline int __cdecl GameRand() { return crt::Rand(); }
GAME_FN(CVector*, __cdecl, Vector_FromAngle, 0x00432DF0, CVector* out, float angle);   // (-sin, cos, 0)
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
// the ped's (+0x358) test that it can get to a point
GAME_FN(uint8_t, __thiscall, PedNav_CanReach, 0x00499840, void* self, const CVector* point, float);
// a line between two points: false, or `blocked` set, when something's in the way
GAME_FN(uint8_t, __cdecl, World_TestLine, 0x005EB1B0, const CVector* from, const CVector* to, uint8_t* blocked, int, int);
GAME_FN(uint8_t, __cdecl, ProcessVerticalLine, 0x004629C0, const CVector* point, float z, CColPoint* colPoint,
        re::Ptr32<void>* entity, int lists, int);
GAME_FN(void, __cdecl, CleanUpReference, 0x004657E0, void* entity, void* ref);
GAME_FN(int, __thiscall, Zones_FindByName, 0x006D7A10, void* zones, const char* name);
GAME_FN(uint8_t, __thiscall, Trigger_IsPointInside, 0x006D6FA0, void* trigger, const CVector* pos);

// ours, through the game
GAME_FN(uint8_t, __cdecl, IsSnowSurfaceCall, 0x00457AC0, int surface);
GAME_FN(uint8_t, __cdecl, IsSnowAtCall, 0x00457D90, const CVector* pos, void* ignore);
GAME_FN(uint8_t, __cdecl, IsInSnowZoneCall, 0x00457EE0, const CVector* pos);

bool IsWinter() {
    return Area_430320(currArea) && chapterAreaProps[currentChapter] == 2;
}

const CVector* Position(uint8_t* entity) {
    if (uint8_t* matrix = Field<uint8_t*>(entity, 0x14)) {
        return reinterpret_cast<const CVector*>(matrix + 0x30);
    }
    return reinterpret_cast<const CVector*>(entity + 4);
}

// rand() over 0..range (as the game: in double, then a float)
float RandomUpTo(float range) {
    return float(double(GameRand()) / 32767.0 * (double(range) - 0.0) + 0.0);
}

// a random point within `radius` of the ped, that the ped could walk to
bool RandomPointNear(uint8_t* ped, float radius, CVector* out) {
    float distance = RandomUpTo(radius);
    float angle = RandomUpTo(twoPi);
    CVector direction;
    const CVector* d = Vector_FromAngle(&direction, angle);
    CVector offset = {d->x * distance, d->y * distance, d->z * distance};
    CVector sum;
    *out = *VectorAdd(&sum, Position(ped), &offset);
    return PedNav_CanReach(ped + 0x358, out, 0.0f) != 0;
}

// the vertical line through `pos` from `height` above to `height` below: its surface
// (the colour point as the game sets it up; the piece and surfaces left to the test)
bool SurfaceUnder(const CVector* pos, float height, void* ignore, uint8_t* surface) {
    CVector up = {0.0f, 0.0f, height};
    CVector top, bottom;
    VectorAdd(&top, pos, &up);
    VectorSub(&bottom, pos, &up);
    CColPoint colPoint;
    colPoint.point = {0.0f, 0.0f, 0.0f};
    colPoint.entity = nullptr;
    colPoint.normal = {0.0f, 0.0f, 0.0f};
    colPoint.depth = 3.4028234663852886e+38f;
    re::Ptr32<void> entity = nullptr;
    ignoreEntity = ignore;
    bool hit = ProcessVerticalLine(&top, bottom.z, &colPoint, &entity, 0x1F, 0) != 0;
    ignoreEntity = nullptr;
    *surface = uint8_t(colPoint.surfaceB);
    // (the colour point's end: the game tests for the always-valid entity but does the
    // same either way)
    if (colPoint.entity) {
        CleanUpReference(colPoint.entity, &colPoint.entity);
    }
    return hit;
}

} // namespace

namespace snow {

// 0x457AC0: the snow surfaces
uint8_t __cdecl IsSnowSurface(int surface) {
    switch (surface) {
    case 2:
    case 3:
    case 4:
    case 5:
    case 0x1B:
    case 0x1D:
        return 1;
    default:
        return 0;
    }
}

// 0x457B10: a random point within `radius` of the ped on `surface`, reachable (and in
// the ped's line of sight when `checkLine`)
uint8_t __cdecl FindSurfaceNear(uint8_t* ped, uint32_t surface, float radius, CVector* out, int checkLine) {
    if (!RandomPointNear(ped, radius, out)) {
        return 0;
    }
    if (re::Flag(checkLine)) {
        uint8_t blocked = 0;
        if (!World_TestLine(Position(ped), out, &blocked, 0, 0) || blocked) {
            return 0;
        }
    }
    uint8_t found;
    bool hit = SurfaceUnder(out, 15.0f, ped, &found);
    return hit && found == surface;
}

// 0x457D90: snow under a point (2 up to 2 down)
uint8_t __cdecl IsSnowAt(const CVector* pos, void* ignore) {
    uint8_t found;
    CVector up = {0.0f, 0.0f, 2.0f};
    CVector top, bottom;
    VectorAdd(&top, pos, &up);
    VectorSub(&bottom, pos, &up);
    CColPoint colPoint;
    colPoint.point = {0.0f, 0.0f, 0.0f};
    colPoint.entity = nullptr;
    colPoint.normal = {0.0f, 0.0f, 0.0f};
    colPoint.depth = 3.4028234663852886e+38f;
    re::Ptr32<void> entity = nullptr;
    ignoreEntity = ignore;
    bool hit = ProcessVerticalLine(&top, bottom.z, &colPoint, &entity, 0x1F, 0) != 0;
    ignoreEntity = nullptr;
    uint8_t result = 0;
    if (hit) {
        found = uint8_t(colPoint.surfaceB);
        result = IsSnowSurfaceCall(found);
    }
    if (colPoint.entity) {
        CleanUpReference(colPoint.entity, &colPoint.entity);
    }
    return result;
}

// 0x457EE0: in one of the snow zones
uint8_t __cdecl IsInSnowZone(const CVector* pos) {
    for (int i = 0; snowZoneNames[i]; i++) {
        int index = Zones_FindByName(zones, snowZoneNames[i]);
        if (index == -1 || (zonePool->flags[index] & 0x80)) {
            continue;
        }
        uint8_t* zone = re::PtrCast<uint8_t*>(zonePool->objects) + zonePool->elemSize * index;
        if (zone && Trigger_IsPointInside(zone, pos)) {
            return 1;
        }
    }
    return 0;
}

// 0x457F50: ConditionIsOnSnow: the ped's surface is snow, or it's in a snow zone
uint8_t __cdecl IsOnSnow(uint8_t* ped) {
    if (!IsWinter()) {
        return 0;
    }
    if (IsSnowSurfaceCall(ped[0x1BE])) {
        return 1;
    }
    return IsInSnowZoneCall(Position(ped)) != 0;
}

// 0x457FC0: ConditionFindSnow: a random reachable, visible point within `radius` with
// snow
uint8_t __cdecl FindSnow(uint8_t* ped, float radius, CVector* out) {
    if (!IsWinter()) {
        return 0;
    }
    if (!RandomPointNear(ped, radius, out)) {
        return 0;
    }
    uint8_t blocked = 0;
    if (!World_TestLine(Position(ped), out, &blocked, 0, 0) || blocked) {
        return 0;
    }
    if (IsSnowAtCall(out, ped)) {
        return 1;
    }
    return IsInSnowZoneCall(out) != 0;
}

} // namespace snow

// ---- registration ---------------------------------------------------------------------

#define C04D_SNOW_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04D_SNOW_REVERSED("Snow::IsSnowSurface", 0x00457AC0, &snow::IsSnowSurface);
C04D_SNOW_REVERSED("PedCondition::FindSurfaceNear", 0x00457B10, &snow::FindSurfaceNear);
C04D_SNOW_REVERSED("Snow::IsSnowAt", 0x00457D90, &snow::IsSnowAt);
C04D_SNOW_REVERSED("Snow::IsInSnowZone", 0x00457EE0, &snow::IsInSnowZone);
C04D_SNOW_REVERSED("ConditionIsOnSnow::Check", 0x00457F50, &snow::IsOnSnow);
C04D_SNOW_REVERSED("ConditionFindSnow::Check", 0x00457FC0, &snow::FindSnow);
