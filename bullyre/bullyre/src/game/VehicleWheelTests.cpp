// A vehicle's wheel line tests' nearest hit (0x46CAC0) and a teleport to its own position
// through a rotation (0x46C9D0).

#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "game/Entity.h"
#include "game/World.h"
#include "reversed.h"

namespace {

using col::CMatrix;
using col::CVector;

struct Quaternion {
    float x, y, z, w;
};

GAME_FN(void, __thiscall, Matrix_ToNiTransform, 0x00412AC0, const CMatrix* m, void* ni);
GAME_FN(void, __thiscall, Quaternion_Set, 0x00465620, Quaternion* q, const void* rotation);
GAME_FN(CMatrix*, __thiscall, Entity_GetMatrixLink, 0x008B6120, CEntity* entity);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, CrossProduct, 0x00414730, CVector* out, const CVector* a, const CVector* b);
GAME_FN(void, __thiscall, Entity_SetPosition, 0x0041C4E0, CEntity* entity, float x, float y, float z);
GAME_FN(uint8_t, __thiscall, Automobile_ProcessWheel, 0x0046C600, CEntity* self, void* a, void* wheel, CColPoint* point, int b);
GAME_FN(CColPoint*, __thiscall, ColPoint_Assign, 0x0046AA70, CColPoint* point, const CColPoint* other);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, CEntity* entity, re::Ptr32<CEntity>* ref);

// 0x46C9D0 (thiscall): the rotations of the matrix and its own taken (as quaternions),
// the point's offset crossed with its +0x10 vector; all unused: it's set back where it
// is. (A shell of something the game stripped)
void __fastcall TeleportInPlace(CEntity* self, re::Edx, const CMatrix* matrix, const uint8_t* target, int) {
    CVector pos = self->GetPosition();
    float transform[16];
    Quaternion q;
    Matrix_ToNiTransform(matrix, transform);
    Quaternion_Set(&q, transform);
    Matrix_ToNiTransform(Entity_GetMatrixLink(self), transform);
    Quaternion_Set(&q, transform);
    CVector offset;
    CVector diff = *VectorSub(&offset, reinterpret_cast<const CVector*>(target), &pos);
    CVector cross;
    CrossProduct(&cross, &diff, reinterpret_cast<const CVector*>(target + 0x10));
    Entity_SetPosition(self, pos.x, pos.y, pos.z);
}

// 0x46CAC0 (thiscall): the nearest hit of the four wheels' tests (the fourth only with
// allFour) into point; any hit
uint8_t __fastcall NearestWheelHit(CEntity* self, re::Edx, void* a, re::Ptr32<void>* wheels, CColPoint* point, int b, int allFour) {
    CColPoint candidate;
    candidate.point = {0.0f, 0.0f, 0.0f};
    candidate.normal = {0.0f, 0.0f, 0.0f};
    candidate.depth = 3.40282347e+38f;
    candidate.entity = nullptr;
    uint8_t hit = 0;
    for (int i = 0; i < 4; i++) {
        if (uint8_t(allFour) != 1 && i == 3) {
            continue;
        }
        if (Automobile_ProcessWheel(self, a, &wheels[i], &candidate, b) && point->depth > candidate.depth) {
            hit = 1;
            ColPoint_Assign(point, &candidate);
        }
    }
    if (candidate.entity) {
        CleanUpOldReference(candidate.entity, &candidate.entity);
    }
    return hit;
}

} // namespace

#define WHEEL_C05_REVERSED(name, address, fn) REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

WHEEL_C05_REVERSED("CVehicle::TeleportInPlace", 0x0046C9D0, &TeleportInPlace);
WHEEL_C05_REVERSED("CPhysical::NearestWheelHit", 0x0046CAC0, &NearestWheelHit);
