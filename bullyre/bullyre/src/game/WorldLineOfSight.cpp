// CWorld's line tests over the sectors a line crosses (GTA's GetIsLineOfSightClear,
// ProcessLineOfSight, ProcessVerticalLine): the sectors are walked column by column,
// following the line's slope across each column's edge.

#include <cstdint>

#include "game/Collision.h"
#include "game/Entity.h"
#include "game/World.h"
#include "reversed.h"

namespace {

using col::CMatrix;
using col::CVector;

GAME_ARRAY(CSector, s_sectors, 0x00C1B178);
GAME_VAR(uint16_t, s_scanCode, 0x00C1AE88);
GAME_VAR(uint8_t, s_lineTestFlag, 0x00C1AE82);
GAME_VAR(CEntity*, s_playerPed, 0x00C1AEA8);

GAME_FN(void, __cdecl, ClearScanCodes, 0x0045D840);
GAME_FN(uint8_t, __cdecl, IsInWorld, 0x0045D450, float x, float y);
GAME_FN(void, __thiscall, ColLine_Set, 0x00564950, void* line, const CVector* start, const CVector* end);
GAME_FN(CColModel*, __cdecl, GetColModel, 0x0051AF50, CEntity* entity);
GAME_FN(CMatrix*, __thiscall, Placeable_GetMatrix, 0x0046DD20, CEntity* entity);
GAME_FN(uint8_t, __cdecl, Col_TestLineOfSight, 0x0056ACD0, const void* line, CMatrix* m, CColModel* col, uint32_t flags, uint8_t g);
GAME_FN(uint8_t, __cdecl, GetIsLineOfSightSectorClear, 0x004612E0, CSector* sector, const void* line, uint32_t flags);
GAME_FN(uint8_t, __cdecl, ProcessLineOfSightSector, 0x004610B0, CSector* sector, const void* line, CColPoint* point, float* dist,
        re::Ptr32<CEntity>* entity, uint32_t flags, int pedsOwnModel);
GAME_FN(uint8_t, __cdecl, ProcessVerticalLineSector, 0x00461200, CSector* sector, const void* line, CColPoint* point,
        re::Ptr32<CEntity>* entity, uint32_t flags, int g);
GAME_FN(float, __cdecl, Fn4737D0, 0x004737D0, float x);

// a CColLine: two 16-byte vectors
struct ColLine {
    float start[4];
    float end[4];
};

void NextScanCode() {
    if (s_scanCode >= 0xFFFF) {
        ClearScanCodes();
        s_scanCode = 1;
    } else {
        s_scanCode++;
    }
}

int SectorIndex(float coord) {
    return int(coord / 50.0 + 18.0);
}
// (through the game's float round trip) clamped to 0..35, the top first
int SectorIndexClamped(float coord) {
    float f = float(SectorIndex(coord));
    f = 35.0 < f ? 35.0f : f;
    f = 0.0f > f ? 0.0f : f;
    return int(f);
}

CSector* Sector(int x, int y) {
    return &s_sectors[y * 36 + x];
}

// one column of sectors, from y0 to y1 (both included, either way); false as soon as
// `test` says stop
template <typename F>
bool Column(int x, int y0, int y1, F test) {
    if (y0 < y1) {
        for (int y = y0; y <= y1; y++) {
            if (!test(Sector(x, y))) {
                return false;
            }
        }
    } else {
        for (int y = y0; y >= y1; y--) {
            if (!test(Sector(x, y))) {
                return false;
            }
        }
    }
    return true;
}

// where the line crosses the edge at `edgeX` (a multiple of 50 from the map's corner),
// as a sector row
int RowAtEdge(int edge, const CVector* from, float slope) {
    float y = float((double(edge) * 50.0 - from->x) * slope + from->y);
    return int(y / 50.0 + 18.0);
}

// the sectors a line crosses, not clamped to the map (a diagonal line: column by column)
template <typename F>
bool WalkDiagonal(const CVector* from, const CVector* to, F test) {
    float dy = to->y - from->y;
    float slope = float(dy / (double(to->x) - from->x));
    int x1 = SectorIndex(from->x);
    int x2 = SectorIndex(to->x);
    int y1 = SectorIndex(from->y);
    int y2 = SectorIndex(to->y);
    if (from->x < to->x) {
        int y = RowAtEdge(x1 - 17, from, slope);  // the column's right edge
        if (!Column(x1, y1, y, test)) {
            return false;
        }
        for (int x = x1 + 1; x < x2; x++) {
            int next = RowAtEdge(x - 17, from, slope);
            if (!Column(x, y, next, test)) {
                return false;
            }
            y = next;
        }
        return Column(x2, y, y2, test);
    }
    int y = RowAtEdge(x1 - 18, from, slope);  // the column's left edge
    if (!Column(x1, y1, y, test)) {
        return false;
    }
    for (int x = x1 - 1; x > x2; x--) {
        int next = RowAtEdge(x - 18, from, slope);
        if (!Column(x, y, next, test)) {
            return false;
        }
        y = next;
    }
    return Column(x2, y, y2, test);
}

// 0x4617D0: GTA's GetIsLineOfSightClear (flags: the sector lists, 0x20 the player too)
uint8_t __cdecl GetIsLineOfSightClear(const CVector* from, const CVector* to, uint32_t flags) {
    NextScanCode();
    int x1 = SectorIndex(from->x);
    int y1 = SectorIndex(from->y);
    int x2 = SectorIndex(to->x);
    int y2 = SectorIndex(to->y);
    ColLine line;
    ColLine_Set(&line, from, to);
    if (flags & 0x20) {
        CEntity* player = s_playerPed;
        CColModel* col = GetColModel(player);
        if (Col_TestLineOfSight(&line, Placeable_GetMatrix(player), col, flags & ~0x100u, s_lineTestFlag)) {
            return 0;
        }
    }
    auto test = [&](CSector* s) { return GetIsLineOfSightSectorClear(s, &line, flags) != 0; };
    if (x1 == x2) {
        if (y1 == y2) {
            return GetIsLineOfSightSectorClear(Sector(x1, y1), &line, flags);
        }
        return Column(x1, y1, y2, test);
    }
    if (y1 == y2) {
        if (x1 < x2) {
            for (int x = x1; x <= x2; x++) {
                if (!test(Sector(x, y1))) {
                    return 0;
                }
            }
        } else {
            for (int x = x1; x >= x2; x--) {
                if (!test(Sector(x, y1))) {
                    return 0;
                }
            }
        }
        return 1;
    }
    return WalkDiagonal(from, to, test);
}

// 0x461EA0: GTA's ProcessLineOfSight: the nearest hit along the line (both ends in the
// world). (The straight cases clamp the sectors to the map; the diagonal walk doesn't)
uint8_t __cdecl ProcessLineOfSight(const CVector* from, const CVector* to, CColPoint* point, re::Ptr32<CEntity>* entity, uint32_t flags,
                                   int pedsOwnModel, uint8_t lineTestFlag) {
    NextScanCode();
    *entity = nullptr;
    if (!IsInWorld(from->x, from->y)) {
        return 0;
    }
    if (!IsInWorld(to->x, to->y)) {
        return 0;
    }
    s_lineTestFlag = lineTestFlag;
    int x1 = SectorIndexClamped(from->x);
    int y1 = SectorIndexClamped(from->y);
    int x2 = SectorIndexClamped(to->x);
    int y2 = SectorIndexClamped(to->y);
    float dist = 1.0f;
    ColLine line;
    ColLine_Set(&line, from, to);
    auto test = [&](CSector* s) {
        ProcessLineOfSightSector(s, &line, point, &dist, entity, flags, pedsOwnModel);
        return true;
    };
    if (x1 == x2) {
        if (y1 == y2) {
            uint8_t hit = ProcessLineOfSightSector(Sector(x1, y1), &line, point, &dist, entity, flags, pedsOwnModel);
            s_lineTestFlag = 0;
            return hit;
        }
        Column(x1, y1, y2, test);
    } else if (y1 == y2) {
        if (x1 < x2) {
            for (int x = x1; x <= x2; x++) {
                test(Sector(x, y1));
            }
        } else {
            for (int x = x1; x >= x2; x--) {
                test(Sector(x, y1));
            }
        }
    } else {
        WalkDiagonal(from, to, test);
    }
    s_lineTestFlag = 0;
    return 1.0f > dist;
}

// 0x462840: GTA's ProcessVerticalLine: from pos down (or up) to z, in pos's sector; the
// hit's z corrected by 0x4737D0 of its normal's z
uint8_t __cdecl ProcessVerticalLine(const CVector* pos, float z, CColPoint* point, re::Ptr32<CEntity>* entity, uint32_t flags, int g) {
    NextScanCode();
    int x = SectorIndexClamped(pos->x);
    int y = SectorIndexClamped(pos->y);
    CVector end{pos->x, pos->y, z};
    ColLine line;
    ColLine_Set(&line, pos, &end);
    uint8_t hit = ProcessVerticalLineSector(Sector(x, y), &line, point, entity, flags, g);
    if (hit) {
        point->point.z = float(double(Fn4737D0(point->normal.z)) + point->point.z);
    }
    return hit;
}

// 0x4629C0: the same without the correction
uint8_t __cdecl ProcessVerticalLineRaw(const CVector* pos, float z, CColPoint* point, re::Ptr32<CEntity>* entity, uint32_t flags, int g) {
    NextScanCode();
    int x = SectorIndexClamped(pos->x);
    int y = SectorIndexClamped(pos->y);
    CVector end{pos->x, pos->y, z};
    ColLine line;
    ColLine_Set(&line, pos, &end);
    return ProcessVerticalLineSector(Sector(x, y), &line, point, entity, flags, g);
}

} // namespace

#define WORLDLOS_C05_REVERSED(name, address, fn) REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

WORLDLOS_C05_REVERSED("CWorld::GetIsLineOfSightClear", 0x004617D0, &GetIsLineOfSightClear);
WORLDLOS_C05_REVERSED("CWorld::ProcessLineOfSight", 0x00461EA0, &ProcessLineOfSight);
WORLDLOS_C05_REVERSED("CWorld::ProcessVerticalLine", 0x00462840, &ProcessVerticalLine);
WORLDLOS_C05_REVERSED("CWorld::ProcessVerticalLineRaw", 0x004629C0, &ProcessVerticalLineRaw);
