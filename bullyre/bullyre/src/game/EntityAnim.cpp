// CEntity: its animation updates (vf19, vf20: the animated object stepped by the
// animation manager, off screen at a lower rate), the full on-screen test (its box's
// corners against the camera's planes), the nearest point of its box (vf25) and a bone's
// transform.

#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "game/Entity.h"
#include "reversed.h"

namespace {

using col::CMatrix;
using col::CRect;
using col::CVector;

GAME_ARRAY(uint8_t*, s_modelInfoPtrs, 0x00C67738);
GAME_VAR(uint8_t, s_cutsceneRunning, 0x020C5BE4);
GAME_VAR(int8_t, s_offScreenOverride, 0x00C936F8);    // not -1: everything counts as on screen
GAME_VAR(float, s_animRate, 0x00A24010);
GAME_VAR(float, s_timeStepNonClipped, 0x00C1A9A0);
GAME_VAR(float, s_timeStep, 0x00C1A9A4);
GAME_VAR(float, s_cutsceneClock, 0x00C2A8F0);
GAME_VAR(int32_t, s_noSphereModelA, 0x00BF4488);       // prop anims without the anim's bounding sphere
GAME_VAR(int32_t, s_noSphereModelB, 0x00BF447C);
GAME_VAR(int32_t, s_noSphereModelC, 0x00BF4484);
GAME_VAR(CEntity*, s_playerPed, 0x00C1AEA8);
GAME_VAR(int32_t, s_cameraMode, 0x00C3CCF8);
GAME_VAR(void*, s_rwCamera, 0x00C3CCF4);
GAME_VAR(float*, s_cameraTransform, 0x00C3CCAC);
GAME_ARRAY(float, s_planeA, 0x00C3CD30);   // two side planes' (x, y) coefficients in camera space
GAME_ARRAY(float, s_planeB, 0x00C3CD40);
inline void* const s_animManager = reinterpret_cast<void*>(0x020C4910);
inline void* const s_camera = reinterpret_cast<void*>(0x00C3CC68);

GAME_FN(uint8_t, __cdecl, Entity_Fn477D90, 0x00477D90, CEntity* entity);
GAME_FN(uint8_t, __thiscall, Entity_GetIsOnScreen, 0x00467460, CEntity* entity);
GAME_FN(int, __thiscall, AnimObject_GetAnim, 0x006B1F50, void* animObject, int index);
GAME_FN(float, __cdecl, GetClockSeconds, 0x00465710);
GAME_FN(void, __thiscall, AnimManager_SetTime, 0x006C0E90, void* manager, void* animObject, uintptr_t anim, int a, float time, int b, int c);
GAME_FN(void, __thiscall, AnimManager_Update, 0x006C1910, void* manager, void* animObject, float dt, int visible, uintptr_t a);
GAME_FN(void, __thiscall, AnimManager_UpdateLate, 0x006C1840, void* manager, void* animObject, float dt, int visible, const CVector* move);
GAME_FN(void, __thiscall, AnimManager_UpdateMoving, 0x006C0710, void* manager, void* animObject, float dt, int visible, const CVector* move);
GAME_FN(void, __thiscall, AnimManager_SetBoundingSphere, 0x006BD460, void* manager, void* animObject, const float* sphere);
GAME_FN(CColModel*, __cdecl, GetColModel, 0x0051AF50, CEntity* entity);
GAME_FN(CVector*, __thiscall, Entity_GetBoundCentre, 0x00466C70, CEntity* entity, CVector* out);
GAME_FN(void, __thiscall, Entity_TransformFromObjectSpace, 0x00466200, CEntity* entity, CVector* out, const CVector* offset);
GAME_FN(int, __thiscall, Camera_IsSphereVisible, 0x004F13D0, void* camera, const CVector* center, float radius);
GAME_FN(void, __cdecl, NiTransform_MultiplyPoints, 0x00451410, float* out, const float* in, int count, const float* transform);
GAME_FN(float, __cdecl, Camera_GetNear, 0x006C82E0, void* camera);
GAME_FN(float, __cdecl, Camera_GetFar, 0x006C82D0, void* camera);
GAME_FN(CMatrix*, __thiscall, Placeable_GetMatrix, 0x0046DD20, CEntity* entity);
GAME_FN(CVector*, __cdecl, InverseTransformPoint, 0x00414D20, CVector* out, const CMatrix* m, const CVector* point);
// (0x0041A9A0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* Matrix_MultiplyPoint(CVector* out, const CMatrix* m, const CVector* v) { return re::CallVec16<0x0041A9A0, CVector*>(out, m, v); }
GAME_FN(void*, __thiscall, ElementTable_FindById, 0x00465580, void* table, int id, int* outIndex);
GAME_FN(void, __thiscall, AnimManager_GetBoneTransform, 0x006BF570, void* manager, void* animObject, int bone, void* out, void* element);
GAME_FN(void, __thiscall, Matrix_ToNiTransform, 0x00412AC0, const CMatrix* m, void* ni);
GAME_FN(uint8_t, __thiscall, PAnimModelInfo_GetBoneTransform, 0x00520BC0, void* mi, int bone, void* out);

template <typename T>
re::Field<T>& Field(void* object, size_t offset) {
    return *reinterpret_cast<re::Field<T>*>(reinterpret_cast<uint8_t*>(object) + offset);
}
template <typename R, typename... A>
R VCall(void* object, int slot, A... args) {
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[slot]))(object, args...);
}

// (vf19 and vf20's start) an animated clump; +0xA4 becomes "off screen"
bool PrepareAnimUpdate(CEntity* e) {
    uint8_t* object = static_cast<uint8_t*>(e->rwObject);
    if (!object || *object != 2 || !e->animObject) {
        return false;
    }
    if (Entity_Fn477D90(e)) {
        e->flagA4 = 0;
    } else if (e->flagA4 == 0) {
        e->flagA4 = Entity_GetIsOnScreen(e) ? 0 : 1;
    }
    if (e->flagA4 != 0 && s_offScreenOverride != -1) {
        e->flagA4 = 0;
    }
    if (e->flagD4) {
        e->flagA4 = 0;
    }
    return true;
}

// (prop anims) the animation's bounding sphere: the entity's, twice its radius
void SetAnimSphere(CEntity* e, CColModel* col) {
    CVector centre;
    Entity_GetBoundCentre(e, &centre);
    float radius = col->sphereRadius;
    float sphere[4] = {centre.x, centre.y, centre.z, radius + radius};
    AnimManager_SetBoundingSphere(s_animManager, e->animObject, sphere);
}

bool HasNoAnimSphere(CEntity* e) {
    int model = e->modelIndex;
    return model == s_noSphereModelA || model == s_noSphereModelB || model == s_noSphereModelC;
}

// 0x467EB0 vf19 (thiscall)
void __fastcall UpdateAnim(CEntity* self, re::Edx) {
    if (!PrepareAnimUpdate(self)) {
        return;
    }
    uint8_t type = self->GetType();
    if (type == ENTITY_TYPE_OBJECT) {
        float dt;
        AnimObject_GetAnim(self->animObject, 0);
        if (s_cutsceneRunning && Field<uint8_t>(self, 0x208) == 4) {
            float time = GetClockSeconds();
            AnimManager_SetTime(s_animManager, self->animObject, AnimObject_GetAnim(self->animObject, 0), 0, time, 1, 0);
            int second = AnimObject_GetAnim(self->animObject, 1);
            if (second) {
                AnimManager_SetTime(s_animManager, self->animObject, second, 0, time, 1, 0);
            }
            dt = 0.0f;
            s_cutsceneClock = time;
        } else {
            dt = s_animRate * s_timeStepNonClipped;
        }
        AnimManager_Update(s_animManager, self->animObject, dt, 1, 0);
        return;
    }
    if (type == ENTITY_TYPE_PROPANIM) {
        int visible = (self->flagA4 == 0 || self->flagE4 != 0) ? 1 : 0;
        AnimManager_Update(s_animManager, self->animObject, s_timeStep * s_animRate, visible, 0);
        if (HasNoAnimSphere(self)) {
            return;
        }
        SetAnimSphere(self, GetColModel(self));
        return;
    }
    if (VCall<uint8_t>(self, 4)) {
        CVector move{0.0f, 0.0f, -Field<float>(self, 0x35C)};
        AnimManager_UpdateMoving(s_animManager, self->animObject, s_timeStep * s_animRate, 1, &move);
        return;
    }
    uint8_t visible = VCall<uint8_t>(self, 22);
    AnimManager_UpdateMoving(s_animManager, self->animObject, s_timeStep * s_animRate, visible, nullptr);
}

// 0x468190 vf20 (thiscall): the same through the late update (the player's always
// visible in camera mode 2)
void __fastcall UpdateAnimLate(CEntity* self, re::Edx) {
    if (!PrepareAnimUpdate(self)) {
        return;
    }
    uint8_t type = self->GetType();
    if (type == ENTITY_TYPE_OBJECT) {
        float dt;
        AnimObject_GetAnim(self->animObject, 0);
        if (s_cutsceneRunning && Field<uint8_t>(self, 0x208) == 4) {
            float time = GetClockSeconds();
            AnimManager_SetTime(s_animManager, self->animObject, AnimObject_GetAnim(self->animObject, 0), 0, time, 1, 0);
            dt = 0.0f;
            s_cutsceneClock = time;
        } else {
            dt = s_animRate * s_timeStepNonClipped;
        }
        AnimManager_UpdateLate(s_animManager, self->animObject, dt, 1, nullptr);
        return;
    }
    if (type == ENTITY_TYPE_PROPANIM) {
        int visible = (self->flagA4 == 0 || self->flagE4 != 0) ? 1 : 0;
        AnimManager_UpdateLate(s_animManager, self->animObject, s_timeStep * s_animRate, visible, nullptr);
        if (HasNoAnimSphere(self)) {
            return;
        }
        SetAnimSphere(self, *reinterpret_cast<re::Ptr32<CColModel>*>(s_modelInfoPtrs[self->modelIndex] + 0xC));
        return;
    }
    if (VCall<uint8_t>(self, 4)) {
        CVector move{0.0f, 0.0f, -Field<float>(self, 0x35C)};
        AnimManager_UpdateLate(s_animManager, self->animObject, s_timeStep * s_animRate, 1, &move);
        return;
    }
    bool playerInMode2 = self == s_playerPed && s_cameraMode == 2;
    int visible = (self->flagA4 == 0 || self->flagE4 != 0 || playerInMode2) ? 1 : 0;
    AnimManager_UpdateLate(s_animManager, self->animObject, s_timeStep * s_animRate, visible, nullptr);
}

// 0x467590 (thiscall): GTA's GetIsOnScreenComplex: its sphere's centre visible, or its
// box's eight corners not all beyond one of the camera's planes (near, far, two sides)
uint8_t __fastcall GetIsOnScreenComplex(CEntity* self, re::Edx) {
    GetColModel(self);
    CColModel* col = GetColModel(self);
    CVector centre;
    Entity_TransformFromObjectSpace(self, &centre, &col->sphereCenter);
    if (Camera_IsSphereVisible(s_camera, &centre, 0.0f)) {
        return 1;
    }
    CRect rect;
    self->CallGetBoundRect(&rect);
    float zMin = self->GetPosition().z + GetColModel(self)->boxMin.z;
    float zMax = self->GetPosition().z + GetColModel(self)->boxMax.z;
    int outside[6] = {0, 0, 0, 0, 0, 0};
    float corners[8][3] = {
        {rect.left, rect.top, zMin},  {rect.left, rect.bottom, zMin},  {rect.right, rect.top, zMin},  {rect.right, rect.bottom, zMin},
        {rect.left, rect.top, zMax},  {rect.left, rect.bottom, zMax},  {rect.right, rect.top, zMax},  {rect.right, rect.bottom, zMax},
    };
    NiTransform_MultiplyPoints(&corners[0][0], &corners[0][0], 8, s_cameraTransform);
    int sideA = 0;
    int sideB = 0;
    for (int i = 0; i < 8; i++) {
        float x = corners[i][0];
        float depth = corners[i][1];
        if (Camera_GetNear(s_rwCamera) > depth) {
            outside[0]++;
        }
        if (Camera_GetFar(s_rwCamera) < depth) {
            outside[1]++;
        }
        if (0.0 < double(depth) * s_planeA[1] + double(x) * s_planeA[0]) {
            sideA++;
        }
        if (double(depth) * s_planeB[1] + double(x) * s_planeB[0] > 0.0) {
            sideB++;
        }
    }
    outside[2] = sideA;
    outside[3] = sideB;
    for (int i = 0; i < 6; i++) {
        if (outside[i] == 8) {
            return 0;
        }
    }
    return 1;
}

// 0x467C00 vf25 (thiscall): the point of its box nearest to `point` (its position
// without a collision model). Returns `out`: the callers use it (vf26 passes it to
// CVector::Distance; a void here left a float's bits in eax: the 2026-10-30 crash)
CVector* __fastcall GetNearestPointOnBox(CEntity* self, re::Edx, CVector* out, const CVector* point) {
    CColModel* col = *reinterpret_cast<re::Ptr32<CColModel>*>(s_modelInfoPtrs[self->modelIndex] + 0xC);
    if (!col) {
        *out = self->GetPosition();
        return out;
    }
    CVector local;
    InverseTransformPoint(&local, Placeable_GetMatrix(self), point);
    float v[3] = {local.x, local.y, local.z};
    const float* mins = &col->boxMin.x;
    const float* maxs = &col->boxMax.x;
    for (int i = 0; i < 3; i++) {
        float c = mins[i] < v[i] ? v[i] : mins[i];
        v[i] = maxs[i] < c || maxs[i] == c ? maxs[i] : c;
    }
    CVector clamped{v[0], v[1], v[2]};
    struct { CVector v; float pad; } worldPad16;   // (the game's transform writes a 4th float)
    CVector& world = worldPad16.v;
    Matrix_MultiplyPoint(&world, Placeable_GetMatrix(self), &clamped);
    *out = world;
    return out;
}

// 0x467D80: a bone's transform (through its animated object's skeleton, or a prop
// anim's model); false when there's none. (hiddenOnly: not for an off-screen entity's
// bones with +8)
uint8_t __cdecl GetBoneTransform(CEntity* e, int bone, void* out, int skipOffScreen) {
    void* animObject = e->animObject;
    void* skeleton = animObject ? Field<void*>(animObject, 0x94) : nullptr;
    if (skeleton) {
        void* element = ElementTable_FindById(skeleton, bone, nullptr);
        if (!element) {
            return 0;
        }
        if (re::Flag(skipOffScreen) && e->flagA4 && Field<int32_t>(element, 8)) {
            return 0;
        }
        AnimManager_GetBoneTransform(s_animManager, e->animObject, -1, out, element);
        return 1;
    }
    if (e->GetType() != ENTITY_TYPE_PROPANIM) {
        return 0;
    }
    uint8_t* mi = s_modelInfoPtrs[e->modelIndex];
    Matrix_ToNiTransform(Placeable_GetMatrix(e), out);
    return PAnimModelInfo_GetBoneTransform(mi, bone, out);
}

} // namespace

#define ENTANIM_C05_REVERSED(name, address, fn) REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

ENTANIM_C05_REVERSED("CEntity::GetIsOnScreenComplex", 0x00467590, &GetIsOnScreenComplex);
ENTANIM_C05_REVERSED("CEntity::GetNearestPointOnBox", 0x00467C00, &GetNearestPointOnBox);
ENTANIM_C05_REVERSED("CEntity::GetBoneTransform", 0x00467D80, &GetBoneTransform);
ENTANIM_C05_REVERSED("CEntity::UpdateAnim", 0x00467EB0, &UpdateAnim);
ENTANIM_C05_REVERSED("CEntity::UpdateAnimLate", 0x00468190, &UpdateAnimLate);
