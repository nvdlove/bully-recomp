// CWorld's ground height queries (GTA's FindGroundZForCoord, FindGroundZFor3DCoord,
// FindRoofZFor3DCoord's cousin) and RepositionOneObject: two models put back on the
// ground under them.

#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "game/Entity.h"
#include "game/Pool.h"
#include "game/World.h"
#include "reversed.h"

namespace {

using col::CMatrix;
using col::CVector;

GAME_VAR(int32_t, s_groundedModelA, 0x00BF4500);   // put down by its bounding box
GAME_VAR(int32_t, s_groundedModelB, 0x00BF44F4);   // put down by its lowest sphere (or its box)
GAME_VAR(CPool<uint8_t>*, s_dummyPool, 0x00C0F600);

GAME_FN(uint8_t, __cdecl, ProcessVerticalLineRaw, 0x004629C0, const CVector* pos, float z, CColPoint* point, re::Ptr32<CEntity>* entity,
        uint32_t flags, int g);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, CEntity* entity, re::Ptr32<CEntity>* ref);
GAME_FN(float, __cdecl, Fn4737D0, 0x004737D0, float x);
GAME_FN(float, __cdecl, FindGroundZFor3DCoord_, 0x004635A0, float x, float y, float z, uint8_t* found, uint32_t flags);
GAME_FN(void, __thiscall, Entity_Fn443AA0, 0x00443AA0, CEntity* entity);
GAME_FN(void, __thiscall, Entity_UpdateRwFrame, 0x00465760, CEntity* entity);
GAME_FN(CMatrix*, __thiscall, Placeable_GetMatrix, 0x0046DD20, CEntity* entity);
// (0x0045D500 stores 16 bytes through `out`: called through re::CallVec16)
static inline float* MultiplyMatrixVector4(float* out, const CMatrix* m, const float* v) { return re::CallVec16<0x0045D500, float*>(out, m, v); }
GAME_FN(CVector*, __cdecl, MultiplyMatrixVector, 0x0045D4A0, CVector* out, const CMatrix* m, const CVector* v);
GAME_FN(void, __cdecl, RepositionOneObject_, 0x004637D0, CEntity* entity);

void InitColPoint(CColPoint* point) {
    point->point = {0.0f, 0.0f, 0.0f};
    point->entity = nullptr;
    point->normal = {0.0f, 0.0f, 0.0f};
    point->depth = 3.40282347e+38f;
}
// (CColPoint's destructor)
void DestroyColPoint(CColPoint* point) {
    if (point->entity) {
        CleanUpOldReference(point->entity, &point->entity);
    }
}

// 0x4634A0: the ground under (x, y) from z 100 down to -100 (20 when there's none).
// (The entity found goes into the x argument's slot: the game's `hit` is its own stack)
float __cdecl FindGroundZForCoord(float x, float y, uint32_t flags) {
    CVector pos{x, y, 100.0f};
    CColPoint point;
    InitColPoint(&point);
    re::Ptr32<CEntity> hit;
    if (ProcessVerticalLineRaw(&pos, -100.0f, &point, &hit, flags, 0)) {
        float z = point.point.z;
        DestroyColPoint(&point);
        return z;
    }
    DestroyColPoint(&point);
    return 20.0f;
}

// 0x4635A0: the ground under (x, y, z), down to 100 below (0 when there's none)
float __cdecl FindGroundZFor3DCoord(float x, float y, float z, uint8_t* found, uint32_t flags) {
    CVector pos{x, y, z};
    CColPoint point;
    InitColPoint(&point);
    re::Ptr32<CEntity> hit;
    if (ProcessVerticalLineRaw(&pos, float(double(z) - 100.0), &point, &hit, flags, 0)) {
        if (found) {
            *found = 1;
        }
        float ground = point.point.z;
        DestroyColPoint(&point);
        return ground;
    }
    if (found) {
        *found = 0;
    }
    DestroyColPoint(&point);
    return 0.0f;
}

// 0x4636B0: the same, the height corrected by 0x4737D0 of the ground's normal z
float __cdecl FindGroundZFor3DCoordCorrected(float x, float y, float z, uint8_t* found, uint32_t flags) {
    CVector pos{x, y, z};
    CColPoint point;
    InitColPoint(&point);
    re::Ptr32<CEntity> hit;
    if (ProcessVerticalLineRaw(&pos, float(double(z) - 100.0), &point, &hit, flags, 0)) {
        if (found) {
            *found = 1;
        }
        float ground = float(double(Fn4737D0(point.normal.z)) + point.point.z);
        DestroyColPoint(&point);
        return ground;
    }
    if (found) {
        *found = 0;
    }
    DestroyColPoint(&point);
    return 0.0f;
}

// 0x4637D0: GTA's RepositionOneObject: the two models set on the ground (searched from
// their height, at least 2, above them)
void __cdecl RepositionOneObject(CEntity* e) {
    int model = e->modelIndex;
    CColModel* col = *reinterpret_cast<re::Ptr32<CColModel>*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(0x00C67738 + model * 4) + 0xC);
    if (model == s_groundedModelA) {
        float height = col->boxMax.z - col->boxMin.z;
        if (height < 2.0) {
            height = 2.0f;
        }
        CVector pos = e->GetPosition();
        float top = pos.z + height;
        float ground = FindGroundZFor3DCoord_(pos.x, pos.y, top, nullptr, 1);
        e->GetPosition().z = float(double(ground) - col->boxMin.z);
    } else if (model == s_groundedModelB) {
        float height = col->boxMax.z - col->boxMin.z;
        if (height < 2.0) {
            height = 2.0f;
        }
        CVector pos = e->GetPosition();
        // (the fourth float is the box's +0x1C; it's not looked at)
        float p[4] = {pos.x, pos.y, pos.z, col->unknown1C};
        CCollisionData* data = col->data;
        if (data) {
            if (data->numBoxes == 1) {
                MultiplyMatrixVector4(p, Placeable_GetMatrix(e), p);
            } else if (int8_t(data->numSpheres) > 0) {
                p[2] = 100.0f;
                for (int i = 0; i < int8_t(data->numSpheres); i++) {
                    CColSphere* s = &data->spheres[i];
                    if (p[2] > s->center.z) {
                        p[0] = s->center.x;
                        p[1] = s->center.y;
                        p[2] = s->center.z;
                    }
                }
                if (p[2] < 100.0) {
                    CVector local{p[0], p[1], p[2]};
                    CVector world;
                    MultiplyMatrixVector(&world, Placeable_GetMatrix(e), &local);
                    p[0] = world.x;
                    p[1] = world.y;
                    p[2] = world.z;
                }
            }
        }
        float top = e->GetPosition().z + height;
        float ground = FindGroundZFor3DCoord_(p[0], p[1], top, nullptr, 1);
        e->GetPosition().z = float(double(ground) - col->boxMin.z);
    } else {
        return;
    }
    Entity_Fn443AA0(e);
    Entity_UpdateRwFrame(e);
}

// 0x463F60: GTA's RepositionCertainDynamicObjects: every dummy
void __cdecl RepositionCertainDynamicObjects() {
    CPool<uint8_t>* pool = s_dummyPool;
    for (int i = pool->size; i != 0;) {
        i--;
        if (pool->IsFreeSlot(i)) {
            continue;
        }
        uint8_t* dummy = pool->objects + pool->elemSize * i;
        if (dummy) {
            RepositionOneObject_(reinterpret_cast<CEntity*>(dummy));
        }
    }
}

} // namespace

#define GROUNDZ_C05_REVERSED(name, address, fn) REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

GROUNDZ_C05_REVERSED("CWorld::FindGroundZForCoord", 0x004634A0, &FindGroundZForCoord);
GROUNDZ_C05_REVERSED("CWorld::FindGroundZFor3DCoord", 0x004635A0, &FindGroundZFor3DCoord);
GROUNDZ_C05_REVERSED("CWorld::FindGroundZFor3DCoordCorrected", 0x004636B0, &FindGroundZFor3DCoordCorrected);
GROUNDZ_C05_REVERSED("CWorld::RepositionOneObject", 0x004637D0, &RepositionOneObject);
GROUNDZ_C05_REVERSED("CWorld::RepositionCertainDynamicObjects", 0x00463F60, &RepositionCertainDynamicObjects);
