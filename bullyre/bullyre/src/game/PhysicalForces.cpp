// CPhysical's forces (GTA's ApplyMoveForce, ApplyTurnForce, GetSpeed), a rotation about a
// point, its destructor, and small helpers from around them.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "game/Entity.h"
#include "reversed.h"

namespace {

using col::CMatrix;
using col::CVector;

struct Quaternion {
    float x, y, z, w;
};

inline re::Ptr32<void>* const s_physicalVtable = reinterpret_cast<re::Ptr32<void>*>(0x0090C654);

GAME_FN(CMatrix*, __thiscall, Placeable_GetMatrix, 0x0046DD20, CEntity* entity);
GAME_FN(CMatrix*, __thiscall, Entity_GetMatrixLink, 0x008B6120, CEntity* entity);
// (0x004146B0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* Multiply3x3(CVector* out, const CMatrix* m, const CVector* v) { return re::CallVec16<0x004146B0, CVector*>(out, m, v); }
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, CrossProduct, 0x00414730, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CMatrix*, __cdecl, Matrix_Invert, 0x00413770, const CMatrix* in, CMatrix* out);
GAME_FN(CMatrix*, __thiscall, Matrix_Copy, 0x00412A10, CMatrix* m, const CMatrix* from);
GAME_FN(void, __thiscall, Matrix_UpdateRW, 0x00413A70, CMatrix* m);
GAME_FN(void, __thiscall, Matrix_UpdateRWFull, 0x00413A80, CMatrix* m);
GAME_FN(void, __thiscall, Matrix_FromNode, 0x00412A70, CMatrix* m, void* transform);
GAME_FN(void, __thiscall, Quaternion_Set, 0x00465620, Quaternion* q, const void* rotation);
GAME_FN(void, __thiscall, Quaternion_Multiply, 0x004654C0, Quaternion* out, const Quaternion* a, const Quaternion* b);
GAME_FN(void, __thiscall, Quaternion_ToNiTransform, 0x00413E60, const Quaternion* q, void* transform);
GAME_FN(void, __thiscall, SimpleTransform_UpdateMatrix, 0x0046DCA0, const CSimpleTransform* t, CMatrix* m);
GAME_FN(void, __thiscall, Entity_Fn443AA0, 0x00443AA0, CEntity* entity);
GAME_FN(void, __thiscall, Entity_SetPosition, 0x0041C4E0, CEntity* entity, float x, float y, float z);
GAME_FN(void, __thiscall, Physical_ApplyMoveForce, 0x0046A7E0, CPhysical* physical, float x, float y, float z);
GAME_FN(void, __thiscall, Physical_ApplyTurnForce, 0x0046A950, CPhysical* physical, float jx, float jy, float jz, float px, float py,
        float pz);
GAME_FN(void, __thiscall, EntryInfoList_Flush, 0x00429840, CEntryInfoList* list);
GAME_FN(void, __thiscall, Entity_Destruct, 0x00467E60, CEntity* entity);
GAME_FN(void, __thiscall, ProjectileData_Resolve, 0x00728430, void* data);
GAME_FN(uint8_t*, __cdecl, GetProjectileInstance, 0x004D34B0, int index);
GAME_FN(CEntity*, __thiscall, Seats_GetOccupant, 0x004CE560, void* seats, int seat);

template <typename T>
re::Field<T>& Field(void* object, size_t offset) {
    return *reinterpret_cast<re::Field<T>*>(reinterpret_cast<uint8_t*>(object) + offset);
}

// 0x45D3E0 (thiscall): ApplyMoveForce of a vector
void __fastcall ApplyMoveForceVector(CPhysical* self, re::Edx, const CVector* force) {
    Physical_ApplyMoveForce(self, force->x, force->y, force->z);
}

// 0x45D410 (thiscall): ApplyTurnForce of vectors
void __fastcall ApplyTurnForceVectors(CPhysical* self, re::Edx, const CVector* force, const CVector* point) {
    Physical_ApplyTurnForce(self, force->x, force->y, force->z, point->x, point->y, point->z);
}

// 0x46A7E0 (thiscall): GTA's ApplyMoveForce: the speed changed by force / mass. (Objects
// (vf5) then move their centre of mass by the force in their own space times 0: nothing,
// unless it's not finite)
void __fastcall ApplyMoveForce(CPhysical* self, re::Edx, float x, float y, float z) {
    float invMass = float(1.0 / self->mass);
    CVector f{invMass * x, invMass * y, invMass * z};
    self->moveSpeed.x = f.x + self->moveSpeed.x;
    self->moveSpeed.y = self->moveSpeed.y + f.y;
    self->moveSpeed.z = f.z + self->moveSpeed.z;
    if (!reinterpret_cast<uint8_t(__thiscall*)(CPhysical*)>(static_cast<void*>(self->vtable[5]))(self)) {
        return;
    }
    CMatrix inverse;
    inverse.attached = nullptr;
    CMatrix local;
    Matrix_Copy(&local, Matrix_Invert(Entity_GetMatrixLink(self), &inverse));
    CVector v = f;
    struct { CVector v; float pad; } outPad16;   // (the game's transform writes a 4th float)
    CVector& out = outPad16.v;
    Multiply3x3(&out, &local, &v);
    CVector shift = out;
    if (std::fabs(shift.x) < 0.05) {
        shift.x = 0.0f;
    }
    float sx = shift.x * 0.0f;
    float sy = shift.y * 0.0f;
    float sz = 0.0f * 0.0f;
    self->centreOfMass.x -= sx;
    self->centreOfMass.y -= sy;
    self->centreOfMass.z -= sz;
}

// 0x46A950 (thiscall): GTA's ApplyTurnForce: the turn speed changed by
// (point - centre of mass) x force / turn mass
void __fastcall ApplyTurnForce(CPhysical* self, re::Edx, float jx, float jy, float jz, float px, float py, float pz) {
    CVector com = self->centreOfMass;
    struct { CVector v; float pad; } worldComPad16;   // (the game's transform writes a 4th float)
    CVector& worldCom = worldComPad16.v;
    Multiply3x3(&worldCom, Placeable_GetMatrix(self), &com);
    CVector j{jx, jy, jz};
    CVector p{px, py, pz};
    CVector arm;
    CVector cross;
    CrossProduct(&cross, VectorSub(&arm, &p, &worldCom), &j);
    float inv = float(1.0 / self->turnMass);
    CVector t{cross.x * inv, cross.y * inv, inv * cross.z};
    self->turnSpeed.x += t.x;
    self->turnSpeed.y += t.y;
    self->turnSpeed.z = t.z + self->turnSpeed.z;
}

// 0x468E30 (thiscall): GTA's GetSpeed: the speed of a point (relative to it)
CVector* __fastcall GetSpeed(CPhysical* self, re::Edx, CVector* out, const CVector* point) {
    CVector p = *point;
    CVector cross;
    return VectorAdd(out, CrossProduct(&cross, &self->turnSpeed, &p), &self->moveSpeed);
}

// 0x468450 (thiscall): turned by a quaternion about a point (offset in its own space;
// the offset's z is added straight)
void __fastcall RotateAbout(CPhysical* self, re::Edx, const Quaternion* rotation, const CVector* offset) {
    CVector pos = self->GetPosition();
    CVector local{offset->x, offset->y, 0.0f};
    struct { CVector v; float pad; } rotatedPad16;   // (the game's transform writes a 4th float)
    CVector& rotated = rotatedPad16.v;
    Multiply3x3(&rotated, Placeable_GetMatrix(self), &local);
    pos.x = rotated.x + pos.x;
    pos.y = rotated.y + pos.y;
    float z = rotated.z + pos.z;
    pos.z = z + offset->z;
    Entity_Fn443AA0(self);
    Quaternion current;
    Quaternion turned;
    if (self->matrix) {
        CMatrix* m = Entity_GetMatrixLink(self);
        Matrix_UpdateRW(m);
        Quaternion_Set(&current, m->attached);
        Quaternion_Multiply(&turned, &current, rotation);
        Quaternion copy = turned;
        CMatrix* m2 = Entity_GetMatrixLink(self);
        Quaternion_ToNiTransform(&copy, m2->attached);
        Matrix_FromNode(m2, m2->attached);
        Matrix_UpdateRWFull(Entity_GetMatrixLink(self));
    } else {
        // (the temporary matrix has no Gamebryo transform: the game reads through null here)
        CMatrix temp;
        temp.attached = nullptr;
        SimpleTransform_UpdateMatrix(&self->placement, &temp);
        Matrix_UpdateRW(&temp);
        Quaternion_Set(&current, temp.attached);
        Quaternion_Multiply(&turned, &current, rotation);
        Quaternion copy = turned;
        Quaternion_ToNiTransform(&copy, temp.attached);
        Matrix_FromNode(&temp, temp.attached);
        self->placement.heading = float(std::atan2(-double(temp.m[4]), double(temp.m[5])));
    }
    Entity_SetPosition(self, pos.x, pos.y, pos.z);
    Entity_Fn443AA0(self);
}

// 0x4690B0 (thiscall): the destructor's body: its owned object (+0x1B8) told through its
// vf1, its entry infos freed, CEntity's
void __fastcall PhysicalDestruct(CPhysical* self, re::Edx) {
    self->vtable = s_physicalVtable;
    if (void* owned = self->unknown1B8) {
        reinterpret_cast<void(__thiscall*)(void*, CPhysical*)>(static_cast<void*>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(owned))[1]))(owned, self);
    }
    EntryInfoList_Flush(&self->entryInfoList);
    Entity_Destruct(self);
}

// 0x46A060: a vehicle driven by the projectile's thrower (e2's +0x238 instance), or one
// with +0xFC
uint8_t __cdecl IsVehicleOfThrower(CEntity* e1, CEntity* e2) {
    if (e1->GetType() != ENTITY_TYPE_VEHICLE || !e2->flagC4) {
        return 0;
    }
    CEntity* thrower = Field<CEntity*>(GetProjectileInstance(Field<int32_t>(e2, 0x238)), 4);
    if (thrower == Seats_GetOccupant(reinterpret_cast<uint8_t*>(e1) + 0x1C4, 0)) {
        return 1;
    }
    return e1->flagFC != 0;
}

// 0x468D00 / 0x468D20 (thiscall): a projectile data's two shorts, once resolved
int16_t __fastcall ProjectileData_Get150(uint8_t* self, re::Edx) {
    ProjectileData_Resolve(self);
    return Field<int16_t>(self, 0x150);
}
int16_t __fastcall ProjectileData_Get152(uint8_t* self, re::Edx) {
    ProjectileData_Resolve(self);
    return Field<int16_t>(self, 0x152);
}

} // namespace

#define PHYSF_C05_REVERSED(name, address, fn) REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PHYSF_C05_REVERSED("CPhysical::ApplyMoveForce(CVector)", 0x0045D3E0, &ApplyMoveForceVector);
PHYSF_C05_REVERSED("CPhysical::ApplyTurnForce(CVector, CVector)", 0x0045D410, &ApplyTurnForceVectors);
PHYSF_C05_REVERSED("CPhysical::RotateAbout", 0x00468450, &RotateAbout);
PHYSF_C05_REVERSED("ProjectileData::Get150", 0x00468D00, &ProjectileData_Get150);
PHYSF_C05_REVERSED("ProjectileData::Get152", 0x00468D20, &ProjectileData_Get152);
PHYSF_C05_REVERSED("CPhysical::GetSpeed", 0x00468E30, &GetSpeed);
PHYSF_C05_REVERSED("CPhysical::~CPhysical", 0x004690B0, &PhysicalDestruct);
PHYSF_C05_REVERSED("IsVehicleOfThrower", 0x0046A060, &IsVehicleOfThrower);
PHYSF_C05_REVERSED("CPhysical::ApplyMoveForce", 0x0046A7E0, &ApplyMoveForce);
PHYSF_C05_REVERSED("CPhysical::ApplyTurnForce", 0x0046A950, &ApplyTurnForce);
