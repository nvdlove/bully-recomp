// CWorld::TriggerExplosionSectorList: an explosion's push and damage on the entities of
// a sector list. Peds are hit through the nearest of their six collision spheres the
// explosion can see; other entities through their bounding box.

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
GAME_VAR(uint8_t, s_chapter, 0x00A147E0);
GAME_VAR(CEntity*, s_playerPed, 0x00C1AEA8);
GAME_VAR(int32_t, s_specialPedModel, 0x00BF42DC);

GAME_FN(CEntity*, __cdecl, GetEntityInPool, 0x0044A3E0, uint32_t pool, uint32_t slot);
GAME_FN(void, __thiscall, Entity_GetBoundCentreNoClear, 0x00466CA0, CEntity* entity, CVector* out);
GAME_FN(CVector*, __thiscall, Entity_GetBoundCentre, 0x00466C70, CEntity* entity, CVector* out);
GAME_FN(float, __thiscall, Entity_GetBoundRadius, 0x00465E10, CEntity* entity);
GAME_FN(void, __thiscall, Ped_GetColSphere, 0x00474630, CEntity* ped, int index, CVector* center, float* radius);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(uint8_t, __cdecl, ProcessLineOfSight, 0x00461EA0, const CVector* from, const CVector* to, CColPoint* point, re::Ptr32<CEntity>* entity,
        uint32_t flags, int pedsOwnModel, int lineTestFlag);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, CEntity* entity, re::Ptr32<CEntity>* ref);
GAME_FN(CMatrix*, __thiscall, Placeable_GetMatrix, 0x0046DD20, CEntity* entity);
GAME_FN(CVector*, __cdecl, InverseTransformPoint, 0x00414D20, CVector* out, const CMatrix* m, const CVector* point);
GAME_FN(void, __thiscall, ColSphere_Set, 0x0056EB80, CColSphere* sphere, float radius, const CVector* center);
GAME_FN(uint8_t, __cdecl, SphereTouchesBox, 0x00564B50, const CColSphere* sphere, const void* boxMin, const void* boxMax);
GAME_FN(void, __thiscall, Physical_AddToMovingList, 0x004697D0, CEntity* entity);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(void, __thiscall, Physical_ApplyMoveForce, 0x0046A7E0, CEntity* entity, float x, float y, float z);
GAME_FN(void, __thiscall, Physical_ApplyTurnForce, 0x0045D410, CEntity* entity, const CVector* force, const CVector* offset);
GAME_FN(float, __cdecl, Max, 0x00414490, float a, float b);
GAME_FN(float, __cdecl, Min, 0x00414470, float a, float b);
GAME_FN(void, __thiscall, Vehicle_InflictDamage_x64, 0x004CC4A0, CEntity* vehicle, CEntity* creator, int type, float amount, float, float, float, uintptr_t a, uint8_t b);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Vehicle_InflictDamage(CEntity* a0, CEntity* a1, int a2, float a3, CVector a4, int a5, uint8_t a6) {
    return Vehicle_InflictDamage_x64(a0, a1, a2, a3, a4.x, a4.y, a4.z, a5, a6);
}
GAME_FN(int, __thiscall, Ped_GetQuadrant, 0x004737F0, CEntity* ped, const float* dir);
GAME_FN(uint8_t, __thiscall, Ped_Fn4473E0, 0x004473E0, CEntity* ped);

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
template <typename T>
re::Field<T>& Field(void* object, size_t offset) {
    return *reinterpret_cast<re::Field<T>*>(reinterpret_cast<uint8_t*>(object) + offset);
}
float Length(const CVector* v) {
    float sq = float(double(v->x) * v->x + double(v->y) * v->y + double(v->z) * v->z);
    return float(std::sqrt(double(std::fabs(sq))));
}

// vf15: the damage an entity takes (18 arguments; what most mean isn't known)
void InflictDamage(CEntity* e, float amount, CEntity* creator, int type, int a10, int bone, int last) {
    using Fn = void(__thiscall*)(CEntity*, float, float, CEntity*, int, int, int, int, int, int, int, float, float, int, int, int, int,
                                 float, int);
    re::PtrCast<Fn>(e->vtable[15])(e, amount, 0.0f, creator, type, 0, 0, 0, 0, 0, a10, 0.0f, 0.0f, 0, 1, 0, bone, 0.0f, last);
}

// 0x462B20: returns whether a ped that counts (not the special model, not 0x4473E0's,
// no +0x1C98) was hurt
uint8_t __cdecl TriggerExplosionSectorList(int type, CPtrList* list, const CVector* pos, float radius, float innerRadius, float force,
                                           float damage, CEntity* creator, int includeCreator) {
    uint8_t hurtPed = 0;
    for (CPtrNode* node = list->first; node;) {
        CEntity* e = NodeEntity(node);
        node = NodeNext(node);
        if (!e->bUsesCollision || !(e->flag1C || e->flag60) || !(ModelInfo(e)[0xB] & uint8_t(1 << s_chapter)) || e->flagF4) {
            continue;
        }
        if (!re::Flag(includeCreator) && creator == e) {
            continue;
        }
        CVector dir{0.0f, 0.0f, 0.0f};
        CVector entityPos = e->GetPosition();
        int bone = -1;
        float dist;
        if (e->bIsPhysical && e->GetType() == ENTITY_TYPE_PED) {
            if (Field<int32_t>(e, 0x1344)) {
                continue;
            }
            CVector centre{0.0f, 0.0f, 0.0f};
            Entity_GetBoundCentreNoClear(e, &centre);
            centre.x -= pos->x;
            centre.y -= pos->y;
            centre.z -= pos->z;
            float d = Length(&centre);
            if (!(d <= Entity_GetBoundRadius(e) + radius)) {
                continue;
            }
            CVector sphereCentre{0.0f, 0.0f, 0.0f};
            float sphereRadius;
            CColPoint point;
            point.point = {0.0f, 0.0f, 0.0f};
            point.entity = nullptr;
            point.normal = {0.0f, 0.0f, 0.0f};
            point.depth = 3.40282347e+38f;
            re::Ptr32<CEntity> hit;
            float nearest = 1.0e7f;
            for (int i = 0; i < 6; i++) {
                Ped_GetColSphere(e, i, &sphereCentre, &sphereRadius);
                CVector diff;
                VectorSub(&diff, &sphereCentre, pos);
                float dd = Length(&diff);
                if (dd < sphereRadius + radius && !ProcessLineOfSight(pos, &sphereCentre, &point, &hit, 3, 0, 0) && nearest > dd) {
                    nearest = dd;
                    dir = diff;
                    bone = i;
                }
            }
            if (bone == -1) {
                if (point.entity) {
                    CleanUpOldReference(point.entity, &point.entity);
                }
                continue;
            }
            float diameter = reinterpret_cast<float(__thiscall*)(CEntity*)>(static_cast<void*>(e->vtable[27]))(e);
            entityPos.z = float(diameter * 0.6 + entityPos.z);
            if (point.entity) {
                CleanUpOldReference(point.entity, &point.entity);
            }
            dist = nearest;
        } else {
            CColSphere sphere;
            sphere.center = {0.0f, 0.0f, 0.0f};
            CVector local;
            ColSphere_Set(&sphere, radius, InverseTransformPoint(&local, Placeable_GetMatrix(e), pos));
            CColModel* col = *reinterpret_cast<re::Ptr32<CColModel>*>(ModelInfo(e) + 0xC);
            if (!SphereTouchesBox(&sphere, &col->boxMin, &col->boxMax)) {
                continue;
            }
            if (!e->bIsPhysical) {
                InflictDamage(e, damage, nullptr, type, 0, -1, 0);
                continue;
            }
            CVector centre;
            CVector diff;
            dist = Length(VectorSub(&diff, Entity_GetBoundCentre(e, &centre), pos));
            // (dir stays zero here: the push below has no direction)
        }

        float q = float((double(radius) - dist) * 2.0 / radius);
        q = q < 1.0f ? q : 1.0f;
        if (e->flag44) {
            continue;
        }
        uint8_t kind = e->GetType();
        if (kind == ENTITY_TYPE_PED) {
            uint8_t* driving = Field<uint8_t*>(e, 0x1554);
            if (driving && Field<int32_t>(driving, 0x46C) == 0) {
                continue;
            }
        }
        if (e->bIsStatic || e->bIsStaticWaitingForCollision) {
            if (kind == ENTITY_TYPE_OBJECT) {
                InflictDamage(e, float(q * damage), nullptr, type, 0, -1, 0);
            } else {
                reinterpret_cast<void(__thiscall*)(CEntity*, int)>(static_cast<void*>(e->vtable[7]))(e, 0);  // SetIsStatic
                Physical_AddToMovingList(e);
            }
        }
        if (e->bIsStatic || e->bIsStaticWaitingForCollision) {
            continue;
        }
        float falloff = 1.0f;
        if (!(innerRadius > dist)) {
            falloff = float(1.0 - (double(dist) - innerRadius) / (double(radius) - innerRadius));
            if (!(falloff <= 1.0f)) {
                falloff = 1.0f;
            } else if (0.0f > falloff) {
                falloff = 0.0f;
            }
        }
        CVector push = dir;
        Vector_Normalise(&push);
        CVector a{push.x * falloff, push.y * falloff, falloff * push.z};
        CVector b{a.x * force, a.y * force, force * a.z};
        float massScale = float(Field<float>(e, 0x158) / 1000.0);
        CVector impulse{b.x * massScale, b.y * massScale, b.z * massScale};
        if (0.0f > impulse.z) {
            impulse.z = 0.0f;
        }
        if (e == s_playerPed) {
            impulse.z = impulse.z < 1.0f ? impulse.z : 1.0f;
        }
        Physical_ApplyMoveForce(e, impulse.x, impulse.y, impulse.z);
        if (e->flag30 == 0) {
            CVector offset{0.0f, 0.0f, 0.0f};
            float r1 = Entity_GetBoundRadius(e);
            float r2 = Entity_GetBoundRadius(e);
            offset.z = Min(Max(pos->z - entityPos.z, -r2), r1);
            Physical_ApplyTurnForce(e, &impulse, &offset);
        }
        kind = e->GetType();
        if (kind == ENTITY_TYPE_VEHICLE) {
            Vehicle_InflictDamage(e, creator, -4, float(double(q) * damage * 0.2), *pos, 0, 1);
        } else if (kind == ENTITY_TYPE_PED) {
            float away[2] = {-impulse.x, -impulse.y};
            Ped_GetQuadrant(e, away);
            InflictDamage(e, float(falloff * damage), creator, type, 1, bone, 4);
            if (e->modelIndex != s_specialPedModel && !Ped_Fn4473E0(e) && !Field<uint8_t>(e, 0x1C98)) {
                hurtPed = 1;
            }
        } else if (kind == ENTITY_TYPE_OBJECT) {
            InflictDamage(e, float(q * damage), nullptr, type, 0, -1, 0);
        }
    }
    return hurtPed;
}

GAME_ARRAY(CSector, s_sectors, 0x00C1B178);
GAME_VAR(int32_t, s_explosionTypeA, 0x00BF442C);  // explosion types that leave vehicles and objects alone
GAME_VAR(int32_t, s_explosionTypeB, 0x00BF44D0);
GAME_VAR(int32_t, s_explosionTypeC, 0x00BF43E8);
GAME_FN(uint8_t, __cdecl, TriggerExplosionSectorList_, 0x00462B20, int type, CPtrList* list, const CVector* pos, float radius,
        float innerRadius, float force, float damage, CEntity* creator, int includeCreator);

int SectorMin(double coord) {
    float f = float(int(coord / 50.0 + 18.0));
    return int(f > 0.0f ? f : 0.0f);
}
int SectorMax(double coord) {
    float f = float(int(coord / 50.0 + 18.0));
    return int(f < 35.0f ? f : 35.0f);
}

// 0x463CC0: GTA's TriggerExplosion: the sectors within radius; the vehicles and objects
// too, except for three types
uint8_t __cdecl TriggerExplosion(int type, const CVector* pos, float radius, float innerRadius, float force, float damage,
                                 CEntity* creator, int includeCreator) {
    bool vehicles = true;
    bool objects = true;
    uint8_t result = 0;
    if (type == s_explosionTypeA || type == s_explosionTypeB || type == s_explosionTypeC) {
        vehicles = false;
        objects = false;
    }
    int minX = SectorMin(double(pos->x) - radius);
    int minY = SectorMin(double(pos->y) - radius);
    int maxX = SectorMax(double(pos->x) + radius);
    int maxY = SectorMax(double(pos->y) + radius);
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            CSector* s = &s_sectors[y * 36 + x];
            if (vehicles) {
                result |= TriggerExplosionSectorList_(type, &s->lists[SECTOR_VEHICLES], pos, radius, innerRadius, force, damage, creator,
                                                      includeCreator);
            }
            result |= TriggerExplosionSectorList_(type, &s->lists[SECTOR_PEDS], pos, radius, innerRadius, force, damage, creator,
                                                  includeCreator);
            if (objects) {
                result |= TriggerExplosionSectorList_(type, &s->lists[SECTOR_OBJECTS], pos, radius, innerRadius, force, damage, creator,
                                                      includeCreator);
            }
        }
    }
    return result;
}

} // namespace

#define EXPL_C05_REVERSED(name, address, fn) REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

EXPL_C05_REVERSED("CWorld::TriggerExplosionSectorList", 0x00462B20, &TriggerExplosionSectorList);
EXPL_C05_REVERSED("CWorld::TriggerExplosion", 0x00463CC0, &TriggerExplosion);
