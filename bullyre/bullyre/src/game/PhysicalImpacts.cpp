// CPhysical's collision responses (GTA San Andreas' names where they match): the sphere
// sweep against a ped's spheres (0x46A0C0), what a collision does to each side (0x46AEE0:
// projectile damage, stopping on the ground, the collision records, damage to objects,
// decals on buildings), the pair of them (0x46B870), the impulse against a static surface
// (0x46B9D0) and between two bodies (0x46BC60, ApplyCollision), and the suspension
// springs (ApplySpringCollision 0x46C160 / 0x46C260, ApplySpringDampening 0x46C3C0).

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

template <typename T>
re::Field<T>& Field(void* object, size_t offset) {
    return *reinterpret_cast<re::Field<T>*>(reinterpret_cast<uint8_t*>(object) + offset);
}

CVector& PosOf(CPlaceable* p) {
    return p->GetPosition();
}
const CVector& MatrixPos(const CMatrix* m) {
    return *reinterpret_cast<const CVector*>(&m->m[12]);
}

constexpr uint8_t TYPE_BUILDING = 1;
constexpr uint8_t TYPE_VEHICLE = 2;
constexpr uint8_t TYPE_PED = 3;
constexpr uint8_t TYPE_OBJECT = 4;
constexpr uint8_t TYPE_PROPANIM = 6;

uint8_t TypeOf(const CEntity* e) {
    return e->typeAndStatus & 7;
}

// A sphere as the sweep test takes it
struct Sphere {
    CVector centre;
    float radius;
};

GAME_VAR(float, PI_timeStep, 0x00C1A9A4);
GAME_VAR(uint32_t, PI_timeInMs, 0x00C1A9B4);
GAME_VAR(float, PI_fltMax, 0x009009B8);
GAME_VAR(CVector, PI_zeroVector, 0x00BCC564);
GAME_VAR(CEntity*, PI_playerPed, 0x00C1AEA8);          // CWorld::Players[0].ped
GAME_VAR(uint8_t, PI_noTurnForces, 0x00C1AE76);
GAME_VAR(int32_t, PI_modelBF432C, 0x00BF432C);         // (a projectile that hits again)
GAME_VAR(int32_t, PI_weaponBF4434, 0x00BF4434);
GAME_VAR(uint32_t, PI_decalModelsFirst, 0x00A136B0);
GAME_VAR(uint32_t, PI_decalModelsLast, 0x00A136B4);
GAME_ARRAY(uint8_t*, PI_modelInfos, 0x00C67738);
inline uint8_t* const PI_projectileInfos = reinterpret_cast<uint8_t*>(0x00C3A054);  // 0x34 each: +0 its damage
inline void* const PI_sounds = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const PI_decals = reinterpret_cast<void*>(0x00CF1F60);

GAME_FN(CMatrix*, __cdecl, PI_Matrix_Invert, 0x00413770, const CMatrix* in, CMatrix* out);
GAME_FN(CMatrix*, __cdecl, PI_Matrix_Multiply, 0x004138D0, CMatrix* out, const CMatrix* a, const CMatrix* b);
// (0x0045D500 stores 16 bytes through `out4`: called through re::CallVec16)
static inline void PI_Matrix_MultiplyPoint4(float* out4, const CMatrix* m, const CVector* v) { re::CallVec16<0x0045D500, void>(out4, m, v); }
GAME_FN(uint8_t, __cdecl, PI_SweepSphereSphere, 0x00576B50, const Sphere* a, const Sphere* b, float f, const CVector* speedA,
        const CVector* speedB, float* t, CVector* point);
GAME_FN(CVector*, __cdecl, PI_VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, PI_VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, PI_CrossProduct, 0x00414730, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, PI_VectorNormalise, 0x00414E10, CVector* v);
GAME_FN(void, __thiscall, PI_FlattenDownward, 0x0046A610, CPhysical* self, CVector* v);
GAME_FN(void, __thiscall, PI_Physical_ApplyMoveForce, 0x0046A7E0, CPhysical* self, float x, float y, float z);
GAME_FN(void, __thiscall, PI_Physical_ApplyTurnForce, 0x0046A950, CPhysical* self, float jx, float jy, float jz, float px,
        float py, float pz);
GAME_FN(void, __thiscall, PI_Physical_ApplyTurnForceV, 0x0045D410, CPhysical* self, const CVector* force, const CVector* point);
GAME_FN(void, __cdecl, PI_ReactToCollision, 0x0046AEE0, CPhysical* self, CPhysical* other, CColPoint* point, float impulse);
GAME_FN(uint8_t*, __cdecl, PI_GetProjectileInstance, 0x004D34B0, int index);
GAME_FN(CEntity*, __cdecl, PI_Fn4D3DA0, 0x004D3DA0, int index);
GAME_FN(void, __cdecl, PI_Projectile_OnHit, 0x004D39B0, CPhysical* self, CPhysical* other, CColPoint* point, int damage);
GAME_FN(uint8_t, __thiscall, PI_Ped_Fn4737F0, 0x004737F0, CEntity* ped, const float* offset2d);
GAME_FN(CEntity*, __thiscall, PI_Ped_Fn497100, 0x00497100, CEntity* ped);
GAME_FN(CEntity*, __thiscall, PI_Seats_GetOccupant, 0x004CE560, void* seats, int seat);
GAME_FN(void, __thiscall, PI_Vehicle_Fn4CC4A0_x64, 0x004CC4A0, CEntity* vehicle, CEntity* attacker, int weapon, float damage, float, float, float, CColPoint* colPoint, int a);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void PI_Vehicle_Fn4CC4A0(CEntity* a0, CEntity* a1, int a2, float a3, CVector a4, CColPoint* a5, int a6) {
    return PI_Vehicle_Fn4CC4A0_x64(a0, a1, a2, a3, a4.x, a4.y, a4.z, a5, a6);
}
GAME_FN(void, __thiscall, PI_Vehicle_Fn4CB4E0, 0x004CB4E0, CEntity* vehicle);
GAME_FN(int32_t*, __thiscall, PI_Vehicle_GetVehicleClass, 0x004CAF30, CEntity* vehicle, int32_t* out);
GAME_FN(void, __thiscall, PI_Vehicle_Fn4AA530, 0x004AA530, CEntity* vehicle);
GAME_FN(uint8_t, __thiscall, PI_ModelInfo_Fn520440, 0x00520440, uint8_t* modelInfo);
GAME_FN(void, __thiscall, PI_Fn7282A0, 0x007282A0, uint8_t* object);
GAME_FN(uint16_t, __thiscall, PI_Fn468D00, 0x00468D00, uint8_t* object);
GAME_FN(uint16_t, __thiscall, PI_Fn468D20, 0x00468D20, uint8_t* object);
GAME_FN(void, __thiscall, PI_Decals_Add, 0x005610C0, void* decals, int id, CColPoint* point, const CVector* normal, int surface);
GAME_FN(uint8_t, __thiscall, PI_Entity_Fn512760, 0x00512760, CEntity* entity);
GAME_FN(void, __cdecl, PI_SpawnDestructionEffects, 0x0046AAF0, CEntity* entity);
GAME_FN(void, __cdecl, PI_Stats_Increment, 0x00458480, int stat, int by);
GAME_FN(int, __thiscall, PI_Sounds_GetId, 0x00595B80, void* sounds, const char* name);
GAME_FN(void*, __thiscall, PI_Sounds_PlayAt, 0x0059B8A0, void* sounds, int id, const CVector* pos, int a, const CVector* pos2,
        int b, int c, int d, int e, uintptr_t f, uintptr_t g, int h, int i, float volume, int j);

// Damage through vf15 (0x3C) of a ped, vehicle or object: its 18 arguments
void InflictDamage(CEntity* target, float damage, float a2, CEntity* attacker, int weapon, void* a5, float a11, float a12,
                   int a16, float a17, int a18) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(target))[0x3C / 4];
    reinterpret_cast<void(__thiscall*)(CEntity*, float, float, CEntity*, int, void*, int, int, int, int, int, float, float, int,
                                       int, int, int, float, int)>(fn)(target, damage, a2, attacker, weapon, a5, 0, 0, 0, 0, 0,
                                                                        a11, a12, 0, 1, 0, a16, a17, a18);
}

template <typename R>
R VCall(void* object, uint32_t offset) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*)>(fn)(object);
}

float SquaredLength(const CVector& v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

// 0x46A0C0 (thiscall): its collision spheres swept (from `previous` along its move)
// against the other's (a ped's); the earliest hit into `points` (+0x1C: the other's
// sphere). Once one hits the other's first sphere, only its fourth is tried after it,
// along the move normalised and made 100 long (kept for the next spheres). Any hit
uint8_t __fastcall ProcessPedSpheres(CPhysical*, re::Edx, CMatrix* previous, CMatrix* matrix, CColModel* col, CMatrix* otherMatrix,
                                     CColModel* otherCol, CColPoint* points) {
    CCollisionData* otherData = otherCol->data;
    CCollisionData* data = col->data;
    if (!data || !otherData) {
        return 0;
    }
    CVector move = {matrix->m[12] - previous->m[12], matrix->m[13] - previous->m[13], matrix->m[14] - previous->m[14]};
    CMatrix inverse;
    inverse.attached = nullptr;
    PI_Matrix_Invert(otherMatrix, &inverse);
    CMatrix relative;  // (computed, unused)
    PI_Matrix_Multiply(&relative, &inverse, previous);
    points->depth = PI_fltMax;
    uint8_t result = 0;
    CVector hitPoint = {0.0f, 0.0f, 0.0f};
    Sphere mine = {{0.0f, 0.0f, 0.0f}, 0.0f};
    Sphere theirs = {{0.0f, 0.0f, 0.0f}, 0.0f};
    for (int i = 0; i < int8_t(data->numSpheres); i++) {
        const CColSphere& s = data->spheres[i];
        CVector centre = s.center;
        float world[4];
        PI_Matrix_MultiplyPoint4(world, previous, &centre);
        mine.centre = {world[0], world[1], world[2]};
        mine.radius = s.radius;
        int j = 0;
        uint8_t found = 0;
        if (int8_t(otherData->numSpheres) > 0) {
            for (;;) {
                if (found) {
                    j = 3;
                    PI_VectorNormalise(&move);
                    move = {float(move.x * 100.0), float(move.y * 100.0), float(move.z * 100.0)};
                }
                const CColSphere& o = otherData->spheres[j];
                centre = o.center;
                PI_Matrix_MultiplyPoint4(world, otherMatrix, &centre);
                theirs.centre = {world[0], world[1], world[2]};
                theirs.radius = o.radius;
                float t = points->depth;
                if (PI_SweepSphereSphere(&mine, &theirs, 1.0f, &move, &PI_zeroVector, &t, &hitPoint) && points->depth > t) {
                    if (!found) {
                        points->depth = t;
                    }
                    points->point = hitPoint;
                    CVector moved = {move.x * t, move.y * t, t * move.z};
                    points->surfaceA = *reinterpret_cast<const uint16_t*>(&s.surface);
                    points->surfaceB = *reinterpret_cast<const uint16_t*>(&o.surface);
                    result = 1;
                    CVector n;
                    PI_VectorAdd(&n, &moved, &mine.centre);
                    n = {n.x - theirs.centre.x, n.y - theirs.centre.y, n.z - theirs.centre.z};
                    PI_VectorNormalise(&n);
                    points->normal = n;
                    points->piece = j;
                    if (j == 0) {
                        found = 1;
                        j++;
                        if (!(j < int8_t(otherData->numSpheres))) {
                            break;
                        }
                        continue;
                    }
                }
                if (found) {
                    break;
                }
                j++;
                if (!(j < int8_t(otherData->numSpheres))) {
                    break;
                }
            }
        }
    }
    return result;
}

// 0x46AEE0: what a collision with `other` does to `self`: a vehicle's time of it (+0x120);
// a projectile's damage to a ped or vehicle (to its driver, or the vehicle's own), its
// hit count (+0x22C), its hit handled (0x4D39B0), stopped when resting on the ground;
// the collision records; damage to objects and prop anims (a projectile's, or a
// vehicle's by its vf57 speed; the mower's grinding); a projectile's decals on buildings,
// buildings' destruction effects; the player's projectile's hit stat (once)
void __cdecl ReactToCollision(CPhysical* self, CPhysical* other, CColPoint* cp, float impulse) {
    uint8_t canDamage = 1;  // (a byte in a dword whose upper bytes the game leaves uninitialised)
    self->flag4C = 1;
    if (TypeOf(self) == TYPE_VEHICLE) {
        self->unknown120 = PI_timeInMs;
    } else {
        uint8_t otherType = TypeOf(other);
        if ((otherType == TYPE_PED || otherType == TYPE_VEHICLE) && self->flagC4 != 0) {
            canDamage = 0;
            if (Field<uint8_t>(self, 0x22C) < 1 || self->modelIndex == PI_modelBF432C) {
                canDamage = 1;
                const CVector& a = PosOf(self);
                const CVector& b = PosOf(other);
                float offset[2];
                offset[0] = a.x - b.x;
                offset[1] = a.y - b.y;
                uint8_t side = otherType == TYPE_PED ? PI_Ped_Fn4737F0(other, offset) : 0;
                uint8_t* projectile = PI_GetProjectileInstance(Field<int32_t>(self, 0x238));
                if (TypeOf(other) == TYPE_PED) {
                    CEntity* attacker = Field<CEntity*>(projectile, 4);
                    if (attacker == other && PI_timeInMs - Field<uint32_t>(projectile, 8) < 200) {
                        return;
                    }
                    uint8_t* info = Field<uint8_t*>(projectile, 0x30);
                    InflictDamage(other, Field<float>(projectile, 0x1C), Field<float>(self, 0x244), attacker,
                                  Field<int32_t>(projectile, 0), nullptr, Field<float>(info, 0xE8), Field<float>(info, 0xEC),
                                  cp->piece, impulse, 2);
                } else if (VCall<uint8_t>(other, 0x10) && PI_Seats_GetOccupant(reinterpret_cast<uint8_t*>(other) + 0x1C4, 0)) {
                    uint8_t* info = Field<uint8_t*>(projectile, 0x30);
                    CEntity* driver = PI_Seats_GetOccupant(reinterpret_cast<uint8_t*>(other) + 0x1C4, 0);
                    InflictDamage(driver, Field<float>(projectile, 0x1C), Field<float>(self, 0x244), Field<CEntity*>(projectile, 4),
                                  Field<int32_t>(projectile, 0), nullptr, Field<float>(info, 0xE8), Field<float>(info, 0xEC), -1,
                                  float(int(side)), 0);
                } else if (VCall<uint8_t>(other, 0x14)) {
                    PI_Vehicle_Fn4CC4A0(other, Field<CEntity*>(projectile, 4), Field<int32_t>(projectile, 0),
                                        Field<float>(projectile, 0x1C), cp->point, cp, 1);
                    if (Field<int32_t>(projectile, 0) == PI_weaponBF4434 && Field<CEntity*>(projectile, 4) == PI_playerPed) {
                        PI_Vehicle_Fn4CB4E0(other);
                    }
                    int32_t vehicleClass;
                    if (*PI_Vehicle_GetVehicleClass(other, &vehicleClass) == 10) {
                        PI_Vehicle_Fn4AA530(other);
                    }
                }
            }
        }
    }
    if (TypeOf(other) == TYPE_BUILDING || other->bIsStatic != 0 || other->bIsStaticWaitingForCollision != 0) {
        self->flag7C = 1;
    }
    if (self->flagC4 != 0) {
        // (0x85C500: the CRT's x87 sqrt)
        float speed = float(std::sqrt(double(std::fabs(SquaredLength(self->moveSpeed)))));
        if (speed < double(0.03f)) {
            canDamage = 0;
        }
        Field<uint8_t>(self, 0x22C) = uint8_t(Field<uint8_t>(self, 0x22C) + 1);
        PI_Projectile_OnHit(self, other, cp, canDamage);
        CEntity* hit = cp->entity;
        if (!hit || (TypeOf(hit) != TYPE_PED && hit->flagC4 == 0)) {
            // resting on the ground: stopped and made static
            float sq = SquaredLength(self->moveSpeed);
            double limit = Field<uint8_t>(self, 0x20A) == 7 ? double(1e-10f) : double(1e-4f);
            if (sq < limit && cp->normal.z > double(0.8f)) {
                self->moveSpeed = {0.0f, 0.0f, 0.0f};
                self->turnSpeed = {0.0f, 0.0f, 0.0f};
                self->stuckCount = 10;
                self->CallSetIsStatic(1);
            }
        }
    }
    if (self->bIsPhysical != 0 && self->flag3C != 0) {
        if (TypeOf(self) == TYPE_PED && PI_Ped_Fn497100(self) && PI_Ped_Fn497100(self) == other) {
            Field<uint8_t>(self, 0x1DE6) = 1;
        }
        if (TypeOf(other) == TYPE_PED && PI_Ped_Fn497100(other) && PI_Ped_Fn497100(other) == self) {
            Field<uint8_t>(other, 0x1DE6) = 1;
        }
        uint8_t count = self->numCollisionRecords;
        for (int i = 0; i < int(self->numCollisionRecords); i++) {
            if (self->collisionRecords[i] == other) {
                return;
            }
        }
        if (count < 6) {
            self->collisionRecords[count] = other;
            self->numCollisionRecords = uint8_t(self->numCollisionRecords + 1);
        }
    }
    uint8_t otherType = TypeOf(other);
    bool grind = false;
    if (otherType == TYPE_OBJECT) {
        if (self->flagC4 != 0) {
            int index = Field<int32_t>(self, 0x238);
            float damage = Field<float>(PI_projectileInfos, index * 0x34);
            uint8_t* projectile = PI_GetProjectileInstance(index);
            CEntity* attacker = nullptr;
            if (projectile && Field<CEntity*>(projectile, 4)) {
                attacker = Field<CEntity*>(projectile, 4);
                if (TypeOf(attacker) != TYPE_PED) {
                    attacker = nullptr;
                }
            }
            InflictDamage(other, damage, 0.0f, attacker, self->modelIndex, cp, 0.0f, 0.0f, -1, 0.0f, 0);
        } else if (TypeOf(self) == TYPE_VEHICLE) {
            float speed = VCall<float>(self, 0xE4);
            if (0.5f < speed) {
                CEntity* driver = PI_Seats_GetOccupant(reinterpret_cast<uint8_t*>(self) + 0x1C4, 0);
                InflictDamage(other, speed, 0.0f, driver, -1, cp, 0.0f, 0.0f, -1, 0.0f, 0);
                int32_t vehicleClass;
                grind = *PI_Vehicle_GetVehicleClass(self, &vehicleClass) == 6 && VCall<uint8_t>(other, 0x78);
            }
        }
    } else if (otherType == TYPE_PROPANIM) {
        CEntity* attacker = nullptr;
        bool done = false;
        if (self->flagC4 != 0) {
            int index = Field<int32_t>(self, 0x238);
            float damage = Field<float>(PI_projectileInfos, index * 0x34);
            attacker = PI_Fn4D3DA0(index);
            if (damage > 0.0f) {
                InflictDamage(other, damage, 0.0f, attacker, self->modelIndex, cp, 0.0f, 0.0f, -1, 0.0f, 0);
                done = true;
            }
        }
        if (!done && TypeOf(self) == TYPE_VEHICLE) {
            float speed = VCall<float>(self, 0xE4);
            if (0.5f < speed) {
                if (!PI_ModelInfo_Fn520440(PI_modelInfos[other->modelIndex])) {
                    InflictDamage(other, speed, 0.0f, attacker, self->modelIndex, cp, 0.0f, 0.0f, -1, 0.0f, 0);
                }
                if (VCall<uint8_t>(other, 0x78)) {
                    int32_t vehicleClass;
                    grind = *PI_Vehicle_GetVehicleClass(self, &vehicleClass) == 6;
                }
            }
        }
    }
    if (grind) {
        // (the mower)
        CVector* pos = &PosOf(self);
        PI_Sounds_PlayAt(PI_sounds, PI_Sounds_GetId(PI_sounds, "MowerGrind"), pos, 3, pos, 0, 0, 0, 0, 0, 0, 6, 1, 1.0f, 0);
    }
    if (TypeOf(other) == TYPE_BUILDING) {
        if (self->flagC4 != 0) {
            uint32_t model = uint32_t(int32_t(self->modelIndex));
            if (model >= PI_decalModelsFirst && model <= PI_decalModelsLast) {
                uint8_t* object = Field<uint8_t*>(PI_modelInfos[model], 0x18);
                if (object) {
                    PI_Fn7282A0(object);
                    if (Field<uint8_t*>(object, 0x80)) {
                        float up = std::fabs(cp->normal.z);
                        uint16_t id;
                        PI_Fn7282A0(object);
                        if (up < 0.3f) {
                            id = PI_Fn468D20(Field<uint8_t*>(object, 0x80));
                        } else {
                            id = PI_Fn468D00(Field<uint8_t*>(object, 0x80));
                        }
                        if (id != 0xFFFF) {
                            PI_Decals_Add(PI_decals, id, cp, &cp->normal, reinterpret_cast<uint8_t*>(cp)[0x22]);
                        }
                    }
                }
            }
        }
        if (PI_Entity_Fn512760(other) && other->flagCC == 0 && TypeOf(self) != TYPE_PED) {
            PI_SpawnDestructionEffects(other);
        }
    }
    if (self->flagC4 != 0) {
        uint8_t* projectile = PI_GetProjectileInstance(Field<int32_t>(self, 0x238));
        uint8_t flags = Field<uint8_t>(self, 0x240);
        if (Field<CEntity*>(projectile, 4) == PI_playerPed && !(flags & 1)) {
            uint8_t type = TypeOf(other);
            if (type == TYPE_VEHICLE || type == TYPE_PED || type == TYPE_OBJECT || type == TYPE_PROPANIM) {
                Field<uint8_t>(self, 0x240) = uint8_t(flags | 1);
                PI_Stats_Increment(0x6C, 1);
            }
        }
    }
}

// 0x46B870 (thiscall): a collision's effect on both (the other first). A projectile
// doesn't hit the player who threw it, nor a ped it's resting against (less than half a
// metre over its base, nearly still)
void __fastcall ApplyCollisionPair(CPhysical* self, re::Edx, CPhysical* other, CColPoint* cp, float impulse) {
    if (other->flagB0 != 0) {
        other->flagB0 = 0;
    }
    if (other->flagC4 != 0 || self->flagC4 != 0) {
        CPhysical* projectile = other->flagC4 != 0 ? other : self;
        CPhysical* target = other->flagC4 != 0 ? self : other;
        if (TypeOf(target) == TYPE_PED && Field<int32_t>(target, 0x1310) == 0xD &&
            target == Field<CEntity*>(PI_GetProjectileInstance(Field<int32_t>(projectile, 0x238)), 4)) {
            return;
        }
        if (TypeOf(target) == TYPE_PED) {
            float height = PosOf(projectile).z - PosOf(target).z;
            height = float(height - 0.1847944);
            if (height < 0.5f) {
                // (0x85C500: the CRT's x87 sqrt)
                float speed = float(std::sqrt(double(std::fabs(SquaredLength(projectile->moveSpeed)))));
                if (speed < double(0.1f)) {
                    return;
                }
            }
        }
    }
    PI_ReactToCollision(other, self, cp, impulse);
    PI_ReactToCollision(self, other, cp, impulse);
}

// 0x46B9D0 (thiscall): the impulse against something immovable at the point (its
// position offset by `offset` when given): none unless moving into it; the impulse's
// size into *impulse; applied as a move force (times moveFactor; a vehicle's vertical
// part cut to 0.3 on steep ground) and a turn force (x, y times turnFactor) unless
// physicalFlags 4 (the turn force not while 0xC1AE76). 1 when moving into it
uint8_t __fastcall ApplyCollisionImpulse(CPhysical* self, re::Edx, CColPoint* cp, float* impulse, float moveFactor, float turnFactor,
                                         const CVector* offset) {
    CVector r = {0.0f, 0.0f, 0.0f};
    CVector tmp;
    if (offset) {
        CVector at;
        PI_VectorAdd(&at, &PosOf(self), offset);
        r = *PI_VectorSub(&tmp, &cp->point, &at);
    } else {
        r = *PI_VectorSub(&tmp, &cp->point, &PosOf(self));
    }
    CVector arm = r;
    CVector spin;
    CVector v;
    PI_VectorAdd(&v, PI_CrossProduct(&spin, &self->turnSpeed, &arm), &self->moveSpeed);
    CVector n = cp->normal;
    float dot = (v.y * n.y + n.x * v.x) + v.z * n.z;
    if (!(0.0f > dot)) {
        return 0;
    }
    CVector rn;
    PI_CrossProduct(&rn, &r, &n);
    float lever = SquaredLength(rn);
    float inverseMass = float(1.0f / (lever / self->turnMass + 1.0f / self->mass));
    float j = -((Field<float>(self, 0x164) + 1.0f) * dot * inverseMass);
    *impulse = j;
    CVector force = {n.x * j, n.y * j, n.z * j};
    if (TypeOf(self) == TYPE_VEHICLE && n.z < 0.7f) {
        force.z = float(force.z * double(0.3f));
    }
    if (!(self->physicalFlags & 4)) {
        force = {force.x * moveFactor, force.y * moveFactor, moveFactor * force.z};
        PI_Physical_ApplyMoveForce(self, force.x, force.y, force.z);
        if (PI_noTurnForces == 0) {
            force.x = force.x * turnFactor;
            force.y = turnFactor * force.y;
            PI_Physical_ApplyTurnForceV(self, &force, &r);
        }
    }
    return 1;
}

// 0x46BC60 (thiscall): GTA's ApplyCollision between two bodies: none unless closing on
// each other along the normal; both brought to their common (mass-weighted) speed and
// bounced apart by the average elasticity (not those with +0x7C); the impulses into
// *impulseA / *impulseB. A vehicle's vertical part cut on steep ground, a ped's against a
// vehicle cut to 1%, flattened downwards when `flatten`; applied (times moveFactor, the
// turn force's x, y also times turnFactor) to those without physicalFlags 4. 1 when so
uint8_t __fastcall ApplyCollision(CPhysical* self, re::Edx, CPhysical* other, CColPoint* cp, float* impulseA, float* impulseB,
                                  int flatten, float moveFactor, float turnFactor) {
    CVector rA;
    PI_VectorSub(&rA, &cp->point, &PosOf(self));
    CVector rB;
    PI_VectorSub(&rB, &cp->point, &PosOf(other));
    CVector arm = rA;
    CVector spin;
    CVector vA;
    PI_VectorAdd(&vA, PI_CrossProduct(&spin, &self->turnSpeed, &arm), &self->moveSpeed);
    arm = rB;
    CVector vB;
    PI_VectorAdd(&vB, PI_CrossProduct(&spin, &other->turnSpeed, &arm), &other->moveSpeed);
    const CVector& n = cp->normal;
    float dA = (n.y * vA.y + n.x * vA.x) + n.z * vA.z;
    float dB = (n.y * vB.y + n.x * vB.x) + n.z * vB.z;
    CVector x;
    PI_CrossProduct(&x, &rA, &n);
    float lever = SquaredLength(x);
    float kA = float(1.0f / (lever / self->turnMass + 1.0f / self->mass));
    PI_CrossProduct(&x, &rB, &n);
    lever = SquaredLength(x);
    float kB = float(1.0f / (lever / other->turnMass + 1.0f / other->mass));
    float common = float((kB * dB + kA * dA) / (kB + kA));
    if (!(dA - common < 0.0f)) {
        return 0;
    }
    float elasticity = float((double(Field<float>(other, 0x164)) + Field<float>(self, 0x164)) * 0.5);
    float newA = common;
    if (self->flag7C == 0) {
        newA = float(common - elasticity * (dA - common));
    }
    float newB = common;
    if (other->flag7C == 0) {
        newB = float(common - (dB - common) * elasticity);
    }
    *impulseA = kA * (newA - dA);
    float jB = -(kB * (newB - dB));
    *impulseB = jB;
    float jA = *impulseA;
    CVector fA = {jA * n.x, n.y * jA, jA * n.z};
    float mjB = -jB;
    CVector fB = {n.x * mjB, n.y * mjB, mjB * n.z};
    uint8_t typeA = TypeOf(self);
    if (typeA == TYPE_VEHICLE && 0.7f > n.z) {
        fA.z = float(fA.z * double(0.3f));
    }
    uint8_t typeB = TypeOf(other);
    if (typeB == TYPE_VEHICLE && n.z * -1.0 < double(0.7f)) {
        fB.z = float(double(0.3f) * fB.z);
    }
    if (typeA == TYPE_PED && typeB == TYPE_VEHICLE) {
        fB = {float(fB.x * double(0.01f)), float(fB.y * double(0.01f)), float(double(0.01f) * fB.z)};
    } else if (typeB == TYPE_PED && typeA == TYPE_VEHICLE) {
        fA = {float(fA.x * double(0.01f)), float(fA.y * double(0.01f)), float(double(0.01f) * fA.z)};
    }
    if (re::Flag(flatten)) {
        PI_FlattenDownward(self, &fA);
        PI_FlattenDownward(self, &fB);
    }
    if (!(self->physicalFlags & 4)) {
        fA = {fA.x * moveFactor, fA.y * moveFactor, fA.z * moveFactor};
        PI_Physical_ApplyMoveForce(self, fA.x, fA.y, fA.z);
        fA.x = turnFactor * fA.x;
        fA.y = fA.y * turnFactor;
        PI_Physical_ApplyTurnForce(self, fA.x, fA.y, fA.z, rA.x, rA.y, rA.z);
    }
    if (other->physicalFlags & 4) {
        return 1;
    }
    fB = {fB.x * moveFactor, fB.y * moveFactor, fB.z * moveFactor};
    PI_Physical_ApplyMoveForce(other, fB.x, fB.y, fB.z);
    fB.x = turnFactor * fB.x;
    fB.y = fB.y * turnFactor;
    PI_Physical_ApplyTurnForce(other, fB.x, fB.y, fB.z, rB.x, rB.y, rB.z);
    return 1;
}

// The spring's time step (at most 3; NaN: 3)
float SpringStep() {
    float ts = PI_timeStep;
    return ts < 3.0f ? ts : 3.0f;
}

// 0x46C160 (thiscall): GTA's ApplySpringCollision: compressed (springRatio under 1), a
// force of the spring constant times twice 0.008 of its mass, the compression, the time
// step and the bias, against `dir`, at `point`. 1
uint8_t __fastcall ApplySpringCollision(CPhysical* self, re::Edx, float springConst, const CVector* dir, const CVector* point,
                                        float springRatio, float bias) {
    float compression = 1.0f - springRatio;
    if (compression < 0.0f || compression == 0.0f) {
        return 1;
    }
    float step = SpringStep();
    double mass = double(self->mass) * double(0.008f);
    float f = float(-((mass + mass) * springConst * compression * step * bias));
    CVector force = {dir->x * f, dir->y * f, f * dir->z};
    PI_Physical_ApplyMoveForce(self, force.x, force.y, force.z);
    PI_Physical_ApplyTurnForce(self, force.x, force.y, force.z, point->x, point->y, point->z);
    return 1;
}

// 0x46C260 (thiscall): the same along `normal` (turned to face against `dir`: the
// caller's vector is flipped), 0.75 of it with physicalFlags 1. 1
uint8_t __fastcall ApplySpringCollisionPos(CPhysical* self, re::Edx, float springConst, const CVector* dir, const CVector* point,
                                           float springRatio, float bias, CVector* normal) {
    float compression = 1.0f - springRatio;
    if (compression < 0.0f || compression == 0.0f) {
        return 1;
    }
    float dot = (dir->y * normal->y + dir->x * normal->x) + dir->z * normal->z;
    if (dot > 0.0f) {
        normal->x = float(normal->x * -1.0);
        normal->y = float(normal->y * -1.0);
        normal->z = float(normal->z * -1.0);
    }
    float step = SpringStep();
    double mass = double(self->mass) * double(0.008f);
    float f = float((mass + mass) * springConst * compression * step * bias);
    if (self->physicalFlags & 1) {
        f = float(f * 0.75);
    }
    CVector force = {normal->x * f, normal->y * f, f * normal->z};
    PI_Physical_ApplyMoveForce(self, force.x, force.y, force.z);
    PI_Physical_ApplyTurnForce(self, force.x, force.y, force.z, point->x, point->y, point->z);
    return 1;
}

// 0x46C3C0 (thiscall): GTA's ApplySpringDampening: the spring's speed along `dir` (the
// average of `speed`'s and the point's own) damped by 0.53 of damping * mass * step
// (doubled with physicalFlags 1), limited so as not to reverse it (by the turn mass
// over the point's lever). 1
uint8_t __fastcall ApplySpringDampening(CPhysical* self, re::Edx, float damping, const CVector* dir, const CVector* point,
                                        const CVector* speed) {
    float dot1 = (speed->y * dir->y + dir->x * speed->x) + speed->z * dir->z;
    CVector arm = *point;
    CVector spin;
    CVector v;
    PI_VectorAdd(&v, PI_CrossProduct(&spin, &self->turnSpeed, &arm), &self->moveSpeed);
    float dot2 = (dir->x * v.x + v.y * dir->y) + dir->z * v.z;
    float average = float((dot2 + dot1) * 0.5);
    float step = SpringStep();
    float f = float(average * -damping * self->mass * step * double(0.53f));
    if (self->physicalFlags & 1) {
        f = float(f * 2.0);
    }
    float lever = SquaredLength(*point);
    float limit = float(self->turnMass / ((lever + 1.0) * 2.0 * self->mass));
    if (limit > 1.0f) {
        limit = 1.0f;
    }
    if (std::fabs(average) > double(1e-4f)) {
        float ratio = std::fabs(float(f / (average * self->mass)));
        if (ratio > limit) {
            f = float(f * limit / ratio);
        }
    }
    CVector force = {dir->x * f, f * dir->y, f * dir->z};
    PI_Physical_ApplyMoveForce(self, force.x, force.y, force.z);
    PI_Physical_ApplyTurnForce(self, force.x, force.y, force.z, point->x, point->y, point->z);
    return 1;
}

} // namespace

#define PHYSI_REVERSED(name, address, fn) REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PHYSI_REVERSED("CPhysical::ProcessPedSpheres", 0x0046A0C0, &ProcessPedSpheres);
PHYSI_REVERSED("CPhysical::ReactToCollision", 0x0046AEE0, &ReactToCollision);
PHYSI_REVERSED("CPhysical::ApplyCollisionPair", 0x0046B870, &ApplyCollisionPair);
PHYSI_REVERSED("CPhysical::ApplyCollisionImpulse", 0x0046B9D0, &ApplyCollisionImpulse);
PHYSI_REVERSED("CPhysical::ApplyCollision", 0x0046BC60, &ApplyCollision);
PHYSI_REVERSED("CPhysical::ApplySpringCollision", 0x0046C160, &ApplySpringCollision);
PHYSI_REVERSED("CPhysical::ApplySpringCollisionPos", 0x0046C260, &ApplySpringCollisionPos);
PHYSI_REVERSED("CPhysical::ApplySpringDampening", 0x0046C3C0, &ApplySpringDampening);
