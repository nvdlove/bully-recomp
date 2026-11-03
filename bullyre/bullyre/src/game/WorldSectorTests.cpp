// CWorld's tests over a sector's lists: line of sight, sphere casts, spheres against the
// world, the nearest entity of a model, and pushing peds out of a moving entity's way.
// (GTA's CWorld::ProcessLineOfSightSector, TestSphereAgainstWorld and friends.)

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "game/Entity.h"
#include "game/World.h"
#include "reversed.h"

namespace {

using col::CMatrix;
using col::CVector;

GAME_VAR(CPtrNode*, s_ptrNodes, 0x00C0F788);
GAME_ARRAY(CSector, s_sectors, 0x00C1B178);
GAME_VAR(uint16_t, s_scanCode, 0x00C1AE88);
GAME_VAR(uint8_t, s_testingSphere, 0x00C1AE8A);        // while TestSphereAgainstSectorList runs
GAME_VAR(uint8_t, s_includeDriverPeds, 0x00C1AE74);
GAME_VAR(uint8_t, s_includeFlag1C98Peds, 0x00C1AE75);
GAME_VAR(CColModel*, s_sphereColModel, 0x00C1AE90);    // a one-sphere model, made on first use
GAME_VAR(uint8_t, s_chapter, 0x00A147E0);
GAME_VAR(CVector, s_vecUnset, 0x00BCC564);
GAME_ARRAY(CEntity*, s_foundEntities, 0x00C1AA80);     // [250]
inline void* const s_tempSpherePoints = reinterpret_cast<void*>(0x00C216C0);

GAME_FN(CEntity*, __cdecl, GetEntityInPool, 0x0044A3E0, uint32_t pool, uint32_t slot);
GAME_FN(void, __cdecl, ClearScanCodes, 0x0045D840);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(void, __cdecl, FindObjectsOfTypeInRange, 0x0045E0D0, int model, const CVector* pos, float range, int16_t* count,
        int16_t maxCount, re::Ptr32<CEntity>* out, uint32_t lists);
GAME_FN(CColModel*, __cdecl, ColModel_New, 0x0056E7C0, size_t size);
GAME_FN(CColModel*, __thiscall, ColModel_Construct, 0x0056E7A0, CColModel* col);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(CCollisionData*, __thiscall, CollisionData_Construct, 0x0056DAD0, CCollisionData* data);
GAME_FN(void, __thiscall, Matrix_SetTranslate, 0x004127C0, CMatrix* m, const float* pos);
GAME_FN(uint8_t, __thiscall, Entity_GetIsTouchingDistance, 0x00465C50, CEntity* entity, const CVector* center, float radius,
        float* distance);
GAME_FN(CColModel*, __cdecl, GetColModel, 0x0051AF50, CEntity* entity);
GAME_FN(CMatrix*, __thiscall, Placeable_GetMatrix, 0x0046DD20, CEntity* entity);
GAME_FN(int, __cdecl, Col_ProcessColModels, 0x0056C880, CMatrix* matA, CColModel* colA, CMatrix* matB, CColModel* colB, void* points,
        void* b, void* c);
// the sector list tests (WorldProcess.cpp's, called where they are)
GAME_FN(uint8_t, __cdecl, ProcessSphereCastSectorList, 0x0045EEC0, CPtrList* list, const CVector* start, const CVector* end, float radius,
        float touchRadius, CColPoint* point, float* dist, re::Ptr32<CEntity>* entity, uint32_t flags, int e, int f);
GAME_FN(void, __cdecl, ForPedsInSectorList, 0x0045F0E0, CPtrList* list, int flags, CEntity* entity, CColModel* col, CMatrix* matrix,
        CVector* offset, float radius);
GAME_FN(uint8_t, __cdecl, ProcessLineOfSightSectorList, 0x0045F670, CPtrList* list, const void* line, CColPoint* point, float* dist,
        re::Ptr32<CEntity>* entity, uint32_t flags, int pedsOwnModel);
GAME_FN(uint8_t, __cdecl, ProcessVerticalLineSectorList, 0x0045F940, CPtrList* list, const void* line, CColPoint* point, float* dist,
        re::Ptr32<CEntity>* entity, uint32_t flags, int g);
GAME_FN(uint8_t, __cdecl, GetIsLineOfSightSectorListClear, 0x00460670, CPtrList* list, const void* line, uint32_t flags,
        int ignoreSomeObjects);
GAME_FN(CEntity*, __cdecl, TestSphereAgainstSectorList_, 0x004608F0, CPtrList* list, float x, float y, float z, float radius,
        CEntity* ignore, int ignoreSomeObjects);
GAME_FN(uint8_t, __cdecl, ProcessSphereCastSector_, 0x00460C20, CSector* sector, const CVector* start, const CVector* end, float radius,
        CColPoint* point, re::Ptr32<CEntity>* entity, uint32_t flags, int e, int f);

inline CEntity* NodeEntity(const CPtrNode* node) {
    return GetEntityInPool(node->bits & 0xF, (node->bits >> 4) & 0x3FFF);
}
inline CPtrNode* NodeNext(const CPtrNode* node) {
    uint32_t next = node->bits >> 18;
    return next == 0x3FFF ? nullptr : &s_ptrNodes[next];
}
uint8_t* ModelInfo(const CEntity* e) {
    return *reinterpret_cast<re::Ptr32<uint8_t>*>(0x00C67738 + e->modelIndex * 4);
}

// the sector a coordinate is in (through the game's float round trip), clamped
int SectorIndex(double coord) {
    return int(coord / 50.0 + 18.0);
}
int SectorMin(double coord) {
    float f = float(SectorIndex(coord));
    return int(f > 0.0f ? f : 0.0f);
}
int SectorMax(double coord) {
    float f = float(SectorIndex(coord));
    return int(f < 35.0f ? f : 35.0f);
}

void NextScanCode() {
    if (s_scanCode >= 0xFFFF) {
        ClearScanCodes();
        s_scanCode = 1;
    } else {
        s_scanCode++;
    }
}

float Length(const CVector* v) {
    float sq = float(double(v->x) * v->x + double(v->y) * v->y + double(v->z) * v->z);
    return float(std::sqrt(double(std::fabs(sq))));
}

// 0x4607A0: the nearest entity of the model within range (FindObjectsOfTypeInRange's
// first 250), or nullptr
CEntity* __cdecl FindNearestObjectOfTypeInRange(int model, const CVector* pos, float range, uint32_t lists) {
    int32_t count = -1;  // (a whole word of -1, read as a short)
    FindObjectsOfTypeInRange(model, pos, range, reinterpret_cast<int16_t*>(&count), 250, s_foundEntities, lists);
    int n = int16_t(count);
    float nearest = 10000.0f;
    CEntity* found = nullptr;
    CVector p = *pos;
    for (int i = 0; i < n; i++) {
        CEntity* e = s_foundEntities[i];
        CVector ep = e->GetPosition();
        CVector d{ep.x - p.x, ep.y - p.y, ep.z - p.z};
        float dist = Length(&d);
        if (nearest > dist) {
            nearest = dist;
            found = e;
        }
    }
    return found;
}

// 0x4608F0: GTA's TestSphereAgainstSectorList: the first entity (not `ignore`) the
// sphere collides with, or nullptr
CEntity* __cdecl TestSphereAgainstSectorList(CPtrList* list, float x, float y, float z, float radius, CEntity* ignore,
                                             int ignoreSomeObjects) {
    CPtrNode* node = list->first;
    if (!node) {
        return nullptr;
    }
    if (!s_sphereColModel) {
        CColModel* col = ColModel_New(0x40);
        s_sphereColModel = col ? ColModel_Construct(col) : nullptr;
        void* mem = Malloc(0x24);
        CCollisionData* data = mem ? CollisionData_Construct(static_cast<CCollisionData*>(mem)) : nullptr;
        s_sphereColModel->data = data;
        CColSphere* sphere = static_cast<CColSphere*>(Malloc(0x14));
        if (sphere) {
            sphere->center = {0.0f, 0.0f, 0.0f};
        }
        data->numSpheres = 1;
        data->spheres = sphere;
        data->unknown20 = 0;
        data->numBoxes = 0;
        data->numTriangles = 0;
    }
    CColModel* col = s_sphereColModel;
    col->sphereCenter = s_vecUnset;
    col->sphereRadius = radius;
    float neg = -radius;
    col->boxMin = {neg, neg, neg};
    col->boxMax = {radius, radius, radius};
    CColSphere* sphere = col->data->spheres;
    sphere->center = s_vecUnset;
    sphere->radius = radius;
    // (SetTranslate copies four floats: the game's fourth is whatever follows on its stack)
    float pos[4] = {x, y, z, 0.0f};
    CMatrix sphereMatrix;
    Matrix_SetTranslate(&sphereMatrix, pos);
    uint16_t scanCode = s_scanCode;
    s_testingSphere = 1;
    do {
        CEntity* e = NodeEntity(node);
        node = NodeNext(node);
        if (!e->bUsesCollision || !(e->flag1C || e->flag60) || !(ModelInfo(e)[0xB] & uint8_t(1 << s_chapter)) || e->flagF4 ||
            e->scanCode == scanCode) {
            continue;
        }
        e->scanCode = scanCode;
        if (e == ignore) {
            continue;
        }
        if (re::Flag(ignoreSomeObjects)) {
            uint8_t* mi = ModelInfo(e);
            if (mi[8] == 1 && ((*reinterpret_cast<uint32_t*>(mi + 0x28) >> 19) & 1)) {
                continue;
            }
        }
        CVector center{x, y, z};
        float dist;
        if (!Entity_GetIsTouchingDistance(e, &center, radius, &dist)) {
            continue;
        }
        CColModel* entityCol = GetColModel(e);
        if (Col_ProcessColModels(&sphereMatrix, s_sphereColModel, Placeable_GetMatrix(e), entityCol, s_tempSpherePoints, nullptr,
                                 nullptr)) {
            s_testingSphere = 0;
            return e;
        }
        // vf5: (a building-like entity) its box's max x grown by the radius
        if (reinterpret_cast<uint8_t(__thiscall*)(CEntity*)>(static_cast<void*>(e->vtable[5]))(e)) {
            if (!(dist >= entityCol->boxMax.x + radius)) {
                s_testingSphere = 0;
                return e;
            }
        }
    } while (node);
    s_testingSphere = 0;
    return nullptr;
}

// 0x460C20: a sphere cast from start to end against a sector's lists (flags: 1
// buildings, 2 vehicles, 4 peds, 8 objects, 0x10 dummies): hit before the end
uint8_t __cdecl ProcessSphereCastSector(CSector* sector, const CVector* start, const CVector* end, float radius, CColPoint* point,
                                        re::Ptr32<CEntity>* entity, uint32_t flags, int e, int f) {
    float dist = 1.0f;
    NextScanCode();
    CVector dir;
    VectorSub(&dir, end, start);
    float touch = Length(&dir);
    touch = float(double(touch) + radius);
    if (flags & 1) {
        ProcessSphereCastSectorList(&sector->lists[SECTOR_BUILDINGS], start, end, radius, touch, point, &dist, entity, flags, e, f);
    }
    if (flags & 2) {
        ProcessSphereCastSectorList(&sector->lists[SECTOR_VEHICLES], start, end, radius, touch, point, &dist, entity, flags, e, f);
    }
    if (flags & 4) {
        ProcessSphereCastSectorList(&sector->lists[SECTOR_PEDS], start, end, radius, touch, point, &dist, entity, flags, e, f);
    }
    if (flags & 8) {
        ProcessSphereCastSectorList(&sector->lists[SECTOR_OBJECTS], start, end, radius, touch, point, &dist, entity, flags, e, f);
    }
    if (flags & 0x10) {
        ProcessSphereCastSectorList(&sector->lists[SECTOR_DUMMIES], start, end, radius, touch, point, &dist, entity, flags, e, f);
    }
    return 1.0f > dist;
}

// 0x460E10: an entity moving from one matrix to another: the peds in the sectors its
// sweep covers pushed off it (CPed::MoveAgainstVehicle; the vehicles' call does
// nothing, flag 2)
void __cdecl MovePedsAgainstEntity(CEntity* entity, CMatrix* from, CMatrix* to) {
    CVector toPos = *reinterpret_cast<CVector*>(&to->m[12]);
    CVector fromPos = *reinterpret_cast<CVector*>(&from->m[12]);
    CVector offset;
    VectorSub(&offset, &toPos, &fromPos);
    CColModel* col = GetColModel(entity);
    float radius = float(double(Length(&offset)) + col->sphereRadius);
    float right = float(double(toPos.x) + radius);
    float bottom = float(double(toPos.y) - radius);
    float top = float(double(toPos.y) + radius);
    float left = float(double(toPos.x) - radius);
    int minX = SectorMin(left);
    int minY = SectorMin(bottom);
    int maxX = SectorMax(right);
    int maxY = SectorMax(top);
    NextScanCode();
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            CSector* s = &s_sectors[y * 36 + x];
            ForPedsInSectorList(&s->lists[SECTOR_VEHICLES], 2, entity, col, to, &offset, radius);
            ForPedsInSectorList(&s->lists[SECTOR_PEDS], 4, entity, col, to, &offset, radius);
        }
    }
}

// 0x4610B0: GTA's ProcessLineOfSightSector: the nearest hit in a sector's lists, below
// *dist (which it lowers). The peds' switches (0xC1AE74/75) apply to the peds only
uint8_t __cdecl ProcessLineOfSightSector(CSector* sector, const void* line, CColPoint* point, float* dist, re::Ptr32<CEntity>* entity,
                                         uint32_t flags, int pedsOwnModel) {
    float localDist = *dist;
    uint8_t save75 = s_includeFlag1C98Peds;
    uint8_t save74 = s_includeDriverPeds;
    s_includeFlag1C98Peds = 0;
    s_includeDriverPeds = 0;
    if (flags & 1) {
        ProcessLineOfSightSectorList(&sector->lists[SECTOR_BUILDINGS], line, point, &localDist, entity, flags, 0);
    }
    if (flags & 2) {
        ProcessLineOfSightSectorList(&sector->lists[SECTOR_VEHICLES], line, point, &localDist, entity, flags, 0);
    }
    if (flags & 4) {
        if (save75) {
            s_includeFlag1C98Peds = 1;
        }
        if (save74) {
            s_includeDriverPeds = 1;
        }
        ProcessLineOfSightSectorList(&sector->lists[SECTOR_PEDS], line, point, &localDist, entity, flags, pedsOwnModel);
        s_includeFlag1C98Peds = 0;
        s_includeDriverPeds = 0;
    }
    if (flags & 8) {
        ProcessLineOfSightSectorList(&sector->lists[SECTOR_OBJECTS], line, point, &localDist, entity, flags, 0);
    }
    if (flags & 0x10) {
        ProcessLineOfSightSectorList(&sector->lists[SECTOR_DUMMIES], line, point, &localDist, entity, flags, 0);
    }
    s_includeDriverPeds = save74;
    s_includeFlag1C98Peds = save75;
    if (*dist > localDist) {
        *dist = localDist;
        return 1;
    }
    return 0;
}

// 0x461200: ProcessVerticalLineSectorList over a sector's lists (the distance starts at 1)
uint8_t __cdecl ProcessVerticalLineSector(CSector* sector, const void* line, CColPoint* point, re::Ptr32<CEntity>* entity, uint32_t flags, int g) {
    float dist = 1.0f;
    if (flags & 1) {
        ProcessVerticalLineSectorList(&sector->lists[SECTOR_BUILDINGS], line, point, &dist, entity, flags, g);
    }
    if (flags & 2) {
        ProcessVerticalLineSectorList(&sector->lists[SECTOR_VEHICLES], line, point, &dist, entity, flags, g);
    }
    if (flags & 4) {
        ProcessVerticalLineSectorList(&sector->lists[SECTOR_PEDS], line, point, &dist, entity, flags, g);
    }
    if (flags & 8) {
        ProcessVerticalLineSectorList(&sector->lists[SECTOR_OBJECTS], line, point, &dist, entity, flags, g);
    }
    if (flags & 0x10) {
        ProcessVerticalLineSectorList(&sector->lists[SECTOR_DUMMIES], line, point, &dist, entity, flags, g);
    }
    return 1.0f > dist;
}

// 0x4612E0: GTA's GetIsLineOfSightSectorClear (objects: flags bit 7 skips some)
uint8_t __cdecl GetIsLineOfSightSectorClear(CSector* sector, const void* line, uint32_t flags) {
    if ((flags & 1) && !GetIsLineOfSightSectorListClear(&sector->lists[SECTOR_BUILDINGS], line, flags, 0)) {
        return 0;
    }
    if ((flags & 2) && !GetIsLineOfSightSectorListClear(&sector->lists[SECTOR_VEHICLES], line, flags, 0)) {
        return 0;
    }
    if ((flags & 4) && !GetIsLineOfSightSectorListClear(&sector->lists[SECTOR_PEDS], line, flags, 0)) {
        return 0;
    }
    if ((flags & 8) &&
        !GetIsLineOfSightSectorListClear(&sector->lists[SECTOR_OBJECTS], line, flags, int((flags >> 7) & 0xFFFFFF01))) {
        return 0;
    }
    if ((flags & 0x10) && !GetIsLineOfSightSectorListClear(&sector->lists[SECTOR_DUMMIES], line, flags, 0)) {
        return 0;
    }
    return 1;
}

// 0x461380: GTA's TestSphereAgainstWorld: the first entity the sphere collides with in
// the sectors it covers
CEntity* __cdecl TestSphereAgainstWorld(CVector center, float radius, CEntity* ignore, uint32_t flags) {
    int minX = SectorMin(double(center.x) - radius);
    int minY = SectorMin(double(center.y) - radius);
    int maxX = SectorMax(double(center.x) + radius);
    int maxY = SectorMax(double(center.y) + radius);
    NextScanCode();
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            CSector* s = &s_sectors[y * 36 + x];
            CEntity* e;
            if ((flags & 1) &&
                (e = TestSphereAgainstSectorList_(&s->lists[SECTOR_BUILDINGS], center.x, center.y, center.z, radius, ignore, 0))) {
                return e;
            }
            if ((flags & 2) &&
                (e = TestSphereAgainstSectorList_(&s->lists[SECTOR_VEHICLES], center.x, center.y, center.z, radius, ignore, 0))) {
                return e;
            }
            if ((flags & 4) &&
                (e = TestSphereAgainstSectorList_(&s->lists[SECTOR_PEDS], center.x, center.y, center.z, radius, ignore, 0))) {
                return e;
            }
            if ((flags & 8) && (e = TestSphereAgainstSectorList_(&s->lists[SECTOR_OBJECTS], center.x, center.y, center.z, radius, ignore,
                                                                  int((flags >> 7) & 0xFFFFFF01)))) {
                return e;
            }
            if ((flags & 0x10) &&
                (e = TestSphereAgainstSectorList_(&s->lists[SECTOR_DUMMIES], center.x, center.y, center.z, radius, ignore, 0))) {
                return e;
            }
        }
    }
    return nullptr;
}

// 0x4616A0: a sphere cast in the start's sector only (the point's depth reset first)
uint8_t __cdecl ProcessSphereCast(const CVector* start, const CVector* end, float radius, CColPoint* point, re::Ptr32<CEntity>* entity,
                                  uint32_t flags, int e, int f) {
    point->depth = 3.40282347e+38f;
    float fx = float(SectorIndex(start->x));
    fx = 35.0 < fx ? 35.0f : fx;
    fx = 0.0f > fx ? 0.0f : fx;
    int x = int(fx);
    float fy = float(SectorIndex(start->y));
    fy = fy > 35.0 ? 35.0f : fy;
    fy = fy < 0.0f ? 0.0f : fy;
    int y = int(fy);
    return ProcessSphereCastSector_(&s_sectors[y * 36 + x], start, end, radius, point, entity, flags, e, f);
}

} // namespace

#define WORLDSEC_C05_REVERSED(name, address, fn) REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

WORLDSEC_C05_REVERSED("CWorld::FindNearestObjectOfTypeInRange", 0x004607A0, &FindNearestObjectOfTypeInRange);
WORLDSEC_C05_REVERSED("CWorld::TestSphereAgainstSectorList", 0x004608F0, &TestSphereAgainstSectorList);
WORLDSEC_C05_REVERSED("CWorld::ProcessSphereCastSector", 0x00460C20, &ProcessSphereCastSector);
WORLDSEC_C05_REVERSED("CWorld::MovePedsAgainstEntity", 0x00460E10, &MovePedsAgainstEntity);
WORLDSEC_C05_REVERSED("CWorld::ProcessLineOfSightSector", 0x004610B0, &ProcessLineOfSightSector);
WORLDSEC_C05_REVERSED("CWorld::ProcessVerticalLineSector", 0x00461200, &ProcessVerticalLineSector);
WORLDSEC_C05_REVERSED("CWorld::GetIsLineOfSightSectorClear", 0x004612E0, &GetIsLineOfSightSectorClear);
WORLDSEC_C05_REVERSED("CWorld::TestSphereAgainstWorld", 0x00461380, &TestSphereAgainstWorld);
WORLDSEC_C05_REVERSED("CWorld::ProcessSphereCast", 0x004616A0, &ProcessSphereCast);
