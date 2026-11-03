// AreaTransitionManager, continued: the player placed at the target, the area's prop
// anims (trigger objects) removed and made, the world cleared for the new area, and
// the two state machines (the transition, +0x864; the loading screen one, +0x868).
// (AreaTransitionManager.cpp has the fields' list.)

#include <cmath>
#include <cstdint>
#include <cstdio>

#include "reversed.h"
#include "runflags.h"

namespace c01atm2 {

struct CVector {
    float x, y, z;
};
// GTA's CColPoint, as the game builds it on the stack (0x415FA0's layout)
struct CColPoint {
    CVector point;       // +0
    re::Ptr32<void> entity;        // +0xC a reference (let go of after)
    CVector normal;      // +0x10
    uint8_t unknown1C[8];
    float depth;         // +0x24
};
struct CMatrix {
    float m[16];
    re::Ptr32<void> attached;
};
static_assert(sizeof(CMatrix) == 0x44);
struct NiTransform {
    float rotate[9];
    CVector translate;
    float scale;
};
static_assert(sizeof(NiTransform) == 0x34);

struct AreaTransitionManager;

using re::At;

GAME_VAR(uint8_t*, PED_player, 0x00C1AEA8);
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(uint32_t*, ms_pPtrNodes, 0x00C0F788);
GAME_VAR(uint8_t*, ms_pTriggerPool, 0x020C7C48);    // CPool: objects +0, flags +4, element size +0xC
GAME_VAR(uint8_t*, ms_pPropAnimPool, 0x00C0F608);
GAME_VAR(uint8_t*, ms_pVehiclePool, 0x00C0F5F4);
GAME_VAR(int32_t, ms_numPendingProps, 0x00BD5C54);
GAME_VAR(uint8_t, ms_bInTransition, 0x00C3CF80);
GAME_VAR(uint8_t, lockButtonInputsExcept, 0x00C1AF8C);
GAME_ARRAY(uint8_t*, ms_modelInfoPtrs, 0x00C67738);
GAME_VAR(uint8_t, ms_currentChapter, 0x00A147E0);
GAME_ARRAY(int32_t, ms_chapterSeason, 0x00A147EC);
GAME_VAR(uint8_t*, g_transitionTimes, 0x020C8910);   // (+0x18 fade out, +0x1C fade in, +0x24 the alpha to go on at)
GAME_ARRAY(uint8_t, ms_fadeColour, 0x00C3CD2C);      // r, g, b, a
GAME_VAR(float, ms_fadeAlpha, 0x00C3CD28);
GAME_VAR(uint8_t, ms_bInputDisabled_A45A4F, 0x00A45A4F);
GAME_VAR(int32_t, ms_numModelsRequested, 0x00CA3988);
GAME_VAR(uint32_t, m_snTimeInMilliseconds, 0x00C1A9B4);
GAME_VAR(void*, ms_pAlwaysValidEntity, 0x00C2A8E8);

inline void* const TheCamera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const TheLurStore = reinterpret_cast<void*>(0x00D02850);
inline void* const userInputManager = reinterpret_cast<void*>(0x00A45A30);
inline void* const triggerPoolWrapper = reinterpret_cast<void*>(0x020C7E98);
inline void* const object_C2B300 = reinterpret_cast<void*>(0x00C2B300);

GAME_FN(void*, __cdecl, GetEntityInPool, 0x0044A3E0, uint32_t pool, int slot);
GAME_FN(void, __thiscall, SimpleTransform_UpdateMatrix, 0x0046DCA0, void* transform, CMatrix* out);
GAME_FN(CMatrix*, __thiscall, Entity_GetMatrix, 0x008B6120, void* entity);
GAME_FN(void, __thiscall, Matrix_CopyConstruct, 0x00412A10, CMatrix* self, const CMatrix* other);
GAME_FN(void*, __cdecl, GetColModel, 0x0051AF50, void* entity);
GAME_FN(uint8_t, __cdecl, ProcessColModels_56C8C0, 0x0056C8C0, CMatrix* matrixA, CMatrix* matrixA2, void* colA, CMatrix* matrixB,
        CMatrix* matrixB2, void* colB, CColPoint* points, int, int, int, int, int, int, uintptr_t, int);
GAME_FN(void, __cdecl, World_Remove, 0x0045DD70, void* entity);
GAME_FN(void, __cdecl, World_Add, 0x0045D6B0, void* entity);
GAME_FN(void, __cdecl, World_RemoveFromCollisionRecords, 0x0045E690, void* entity);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, re::Ptr32<void>* ref);
GAME_FN(void, __cdecl, ATM_PlaceVehicle, 0x004158F0);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(uint8_t, __cdecl, ProcessVerticalLine_462840, 0x00462840, const CVector* pos, float zLimit, CColPoint* point, re::Ptr32<void>* entity,
        int, int);
GAME_FN(float, __thiscall, Entity_GetDistanceFromCentreOfMassToBaseOfModel, 0x00466110, void* entity);
GAME_FN(void, __thiscall, Matrix_SetRotateZOnly, 0x00412C20, void* matrix, float angle);
GAME_FN(void, __thiscall, Ped_SetHeading, 0x00475730, void* ped, float heading);
GAME_FN(void, __thiscall, Fn_48BAC0, 0x0048BAC0, void* self);
GAME_FN(void, __thiscall, Fn_48B480, 0x0048B480, void* self);
GAME_FN(void, __thiscall, Entity_DeleteRwObject, 0x004667A0, void* entity);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(int16_t, __thiscall, TriggerPool_GetSize, 0x006D70A0, void* self);
GAME_FN(uint8_t*, __thiscall, ModelInfo_5201D0, 0x005201D0, void* modelInfo);
GAME_FN(NiTransform*, __cdecl, Transform_CombineRotation, 0x00415EB0, NiTransform* out, const CVector* axis, float degrees, int mode);
GAME_FN(NiTransform*, __cdecl, Transform_CombineTranslation, 0x00415F50, NiTransform* out, const CVector* translation, int mode);
GAME_FN(void*, __cdecl, CreateTriggerObject_44CE70, 0x0044CE70, int model, int area, NiTransform* transform, float, void* trigger,
        int, int);
GAME_FN(CVector*, __thiscall, Placeable_GetForward, 0x004154A0, void* placeable, CVector* out);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(CMatrix*, __thiscall, Placeable_GetMatrix, 0x0046DD20, void* placeable);
GAME_FN(float, __cdecl, HeadingFromVector_433310, 0x00433310, const CVector* v);
GAME_FN(CVector*, __thiscall, Entity_GetBoundCentre, 0x00466C70, void* entity, CVector* out);
GAME_FN(void, __thiscall, CPed_ClearPartner, 0x00477470, void* ped);
GAME_FN(void, __thiscall, ActionController_Stop, 0x005F40A0, void* controller);
GAME_FN(void, __thiscall, PedIntelligence_5F3A40, 0x005F3A40, void* intelligence);
GAME_FN(void, __thiscall, CPed_UpdateMoveDirection, 0x00474570, void* ped);
GAME_FN(void, __thiscall, ATM_PlacePlayer, 0x00416440, AreaTransitionManager* self, const CVector* pos, float heading);
GAME_FN(void, __thiscall, Camera_SetShot, 0x004F2C20, void* camera, int, int, int, int, int);
GAME_FN(void, __thiscall, Camera_ResetController, 0x004F2B60, void* camera);
GAME_FN(void, __thiscall, Camera_ControllerLookAt, 0x004F2B90, void* camera, CVector direction);
GAME_FN(void, __thiscall, Camera_Fade, 0x004F1AD0, void* camera, float time, int, int);
GAME_FN(int, __thiscall, Camera_GetFadeState, 0x004F12F0, void* camera);
GAME_FN(void, __cdecl, Fn_45B800, 0x0045B800);
GAME_FN(void, __thiscall, EmptyThiscall, 0x0049A650, void* self);
GAME_FN(void*, __thiscall, VehicleSeats_GetOccupant, 0x004CE560, void* seats, int seat);
GAME_FN(void, __thiscall, ATM_RemoveAreaProps, 0x004165E0, AreaTransitionManager* self, const int32_t* area);
GAME_FN(void, __thiscall, Fn_596140, 0x00596140, void* self);
GAME_FN(void, __thiscall, Fn_4A7750, 0x004A7750, void* self);
GAME_FN(void, __cdecl, Fn_4D4A30, 0x004D4A30);
GAME_FN(void, __cdecl, Fn_534510, 0x00534510, const CVector* pos);
GAME_FN(void, __cdecl, ClearAllExplosions, 0x004D2680);
GAME_FN(void, __cdecl, Fn_444BB0, 0x00444BB0);
GAME_FN(void, __thiscall, Fn_59B190, 0x0059B190, void* self);
GAME_FN(void, __thiscall, Fn_5A6420, 0x005A6420, void* self, int);
GAME_FN(void, __cdecl, ATM_RemoveColliding, 0x00416290, uint32_t* node, void* entity);
GAME_FN(void, __cdecl, GenerateAllBoundingBoxes, 0x0045B960, int);
GAME_FN(void, __thiscall, LurStore_5DAF40, 0x005DAF40, void* self);
GAME_FN(void, __thiscall, ATM_CreateAreaProps, 0x00416690, AreaTransitionManager* self, const int32_t* area);
GAME_FN(uint8_t, __thiscall, LurStore_IsLoading_5DBC40, 0x005DBC40, void* self);
GAME_FN(void, __thiscall, LurStore_5DBC80, 0x005DBC80, void* self);
GAME_FN(int, __thiscall, LurStore_5DBCC0, 0x005DBCC0, void* self, const char* name);
GAME_FN(void, __thiscall, LurStore_Load_5DC180, 0x005DC180, void* self, const char* name, int);
GAME_FN(const char*, __cdecl, GetAreaName, 0x00415050, const int32_t* area);
GAME_FN(void, __cdecl, Fn_438820, 0x00438820);
GAME_FN(void, __cdecl, ATM_ResetPlayer, 0x00415330);
GAME_FN(void, __thiscall, ATM_SetArea, 0x004158A0, AreaTransitionManager* self, const int32_t* area);
GAME_FN(void, __thiscall, ATM_ClearWorldForArea, 0x00416C30, AreaTransitionManager* self, const CVector* pos);
GAME_FN(void, __thiscall, ATM_RequestTransition, 0x00415790, AreaTransitionManager* self, const CVector* pos, float heading,
        const int32_t* area, const CVector* direction);
GAME_FN(void, __thiscall, ATM_ClearFadeState, 0x00415D80, AreaTransitionManager* self);
GAME_FN(void, __thiscall, ATM_SetDoorTarget, 0x004168A0, AreaTransitionManager* self, const int32_t* area, void* ped, void* trigger);
GAME_FN(void, __cdecl, Pad_StopRumble, 0x00738780);
GAME_FN(void, __thiscall, UserInputManager_SetEnabled3, 0x0050DFD0, void* self, int enabled);
GAME_FN(void, __cdecl, Fn_55E850, 0x0055E850);
GAME_FN(void, __cdecl, Game_TidyUpMemory, 0x004302A0, int, int);
GAME_FN(uint8_t, __cdecl, HasCollisionLoadedExcept, 0x00570260, const float* xy);
GAME_FN(CVector*, __thiscall, Placeable_GetPosition, 0x00415460, void* placeable);
GAME_FN(uint8_t, __thiscall, Trigger_IsPointInside_6D6FA0, 0x006D6FA0, void* trigger, const CVector* point);
GAME_FN(uint8_t, __cdecl, Fn_42E6C0, 0x0042E6C0, int area);
GAME_FN(uint8_t*, __thiscall, Route_First, 0x006D59B0, void* route);
GAME_FN(uint8_t*, __thiscall, Route_Next, 0x006D59F0, void* route);

CVector* PositionOf(void* placeable) {
    uint8_t* matrix = At<uint8_t*>(placeable, 0x14);
    return matrix ? reinterpret_cast<CVector*>(matrix + 0x30) : reinterpret_cast<CVector*>(static_cast<uint8_t*>(placeable) + 4);
}
float HeadingOf(void* placeable) {
    uint8_t* matrix = At<uint8_t*>(placeable, 0x14);
    if (matrix) {
        return static_cast<float>(std::atan2(-static_cast<double>(At<float>(matrix, 0x10)), static_cast<double>(At<float>(matrix, 0x14))));
    }
    return At<float>(placeable, 0x10);
}
uint8_t* PoolAt(uint8_t* pool, int slot) {
    if (At<uint8_t*>(pool, 4)[slot] & 0x80) {
        return nullptr;
    }
    return At<uint8_t*>(pool, 0) + At<int32_t>(pool, 0xC) * slot;
}
// (both of the game's branches on ms_pAlwaysValidEntity do the same)
void ReleaseColPointEntity(CColPoint* point) {
    if (point->entity) {
        CleanUpOldReference(point->entity, &point->entity);
    }
}
void ResetPlayerInline() {
    uint8_t* ped = PED_player;
    CPed_ClearPartner(ped);
    ActionController_Stop(At<uint8_t*>(ped, 0x2E0) + 0x258);
    PedIntelligence_5F3A40(At<uint8_t*>(ped, 0x2E0));
    CPed_UpdateMoveDirection(ped);
    At<int32_t>(ped, 0x13AC) = 0;
    At<int32_t>(ped, 0x13A8) = 0;
    lockButtonInputsExcept = 0;
}
// the camera looking along (pos - from), raised by 0.8
void LookFrom(const CVector* pos, const CVector* from) {
    CVector look;
    VectorSub(&look, pos, from);
    look.z = static_cast<float>(look.z + 0.800000011920929);
    Vector_Normalise(&look);
    Camera_ControllerLookAt(TheCamera, look);
}

// 0x416290: the objects and dummies in a sector list that collide with `entity`
// (raised by 1) deleted
void __cdecl RemoveColliding(uint32_t* node, void* entity) {
    while (node) {
        uint32_t bits = *node;
        uint8_t* other = static_cast<uint8_t*>(GetEntityInPool(bits & 0xF, (bits >> 4) & 0x3FFF));
        node = (bits >> 18) == 0x3FFF ? nullptr : ms_pPtrNodes + (bits >> 18);
        if (!other) {
            continue;
        }
        CMatrix dummyMatrix;
        CMatrix* otherMatrix;
        uint8_t kind = other[0x108] & 7;
        if (kind == 5) {
            SimpleTransform_UpdateMatrix(other + 4, &dummyMatrix);
            otherMatrix = &dummyMatrix;
        } else if (kind == 4) {
            otherMatrix = Entity_GetMatrix(other);
        } else {
            continue;
        }
        CMatrix raised;
        Matrix_CopyConstruct(&raised, Entity_GetMatrix(entity));
        raised.m[12] = raised.m[12] + 0.0f;
        raised.m[13] = 0.0f + raised.m[13];
        raised.m[14] = static_cast<float>(raised.m[14] + 1.0);
        CColPoint point;
        point.point = {0.0f, 0.0f, 0.0f};
        point.entity = nullptr;
        point.normal = {0.0f, 0.0f, 0.0f};
        point.depth = 3.4028234663852886e+38f;
        void* colEntity = GetColModel(entity);
        void* colOther = GetColModel(other);
        if (ProcessColModels_56C8C0(Entity_GetMatrix(entity), &raised, colEntity, otherMatrix, otherMatrix, colOther, &point, 0, 0, 1, 1, 0,
                                    0xF, 0, 1)) {
            World_Remove(other);
            reinterpret_cast<void(__thiscall*)(void*, int)>(static_cast<void*>(At<re::Ptr32<void>*>(other, 0)[0]))(other, 1);
        }
        ReleaseColPointEntity(&point);
    }
}

// 0x416440 (thiscall; `this` unused): the player (or its vehicle) put on the ground at
// `pos`, facing `heading`
void __fastcall PlacePlayer(AreaTransitionManager*, re::Edx, const CVector* pos, float heading) {
    uint8_t* ped = PED_player;
    ATM_PlaceVehicle();
    CColPoint point;
    point.point = {0.0f, 0.0f, 0.0f};
    point.normal = {0.0f, 0.0f, 0.0f};
    point.entity = nullptr;
    point.depth = 3.4028234663852886e+38f;
    CVector up = {0.0f, 0.0f, 1.0f};
    CVector probe;
    VectorAdd(&probe, pos, &up);
    float zLimit = static_cast<float>(pos->z - 1.0);
    re::Ptr32<void> hitEntity;
    if (ProcessVerticalLine_462840(&probe, zLimit, &point, &hitEntity, 1, 0)) {
        probe.z = point.point.z;
    } else {
        probe.z = static_cast<float>(probe.z + 0.5);
    }
    uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
    if (vehicle) {
        probe.z = Entity_GetDistanceFromCentreOfMassToBaseOfModel(vehicle) + probe.z;
        reinterpret_cast<void(__thiscall*)(void*, float, float, float)>(static_cast<void*>(At<re::Ptr32<void>*>(vehicle, 0)[0x38 / 4]))(
            vehicle, probe.x, probe.y, probe.z);   // (Teleport: the vector as its floats, as the bodies take it)
        if (At<void*>(vehicle, 0x14)) {
            Matrix_SetRotateZOnly(At<void*>(vehicle, 0x14), heading);
        }
        At<float>(vehicle, 0x10) = heading;
    } else {
        probe.z = Entity_GetDistanceFromCentreOfMassToBaseOfModel(ped) + probe.z;
        At<int32_t>(ped, 0x14D4) = 1;
        reinterpret_cast<void(__thiscall*)(void*, float, float, float)>(static_cast<void*>(At<re::Ptr32<void>*>(ped, 0)[0x38 / 4]))(
            ped, probe.x, probe.y, probe.z);   // (Teleport: the vector as its floats, as the bodies take it)
        Ped_SetHeading(ped, heading);
    }
    Fn_48BAC0(object_C2B300);
    ReleaseColPointEntity(&point);
}

// 0x4165E0 (thiscall; `this` unused): the prop anims of triggers not in `area` removed
// (not the first trigger's, not flagged ones, not kind 5)
void __fastcall RemoveAreaProps(AreaTransitionManager*, re::Edx, const int32_t* area) {
    uint8_t* pool = ms_pPropAnimPool;
    int slot = At<int32_t>(pool, 8);
    uint8_t* firstTrigger = (At<uint8_t*>(ms_pTriggerPool, 4)[0] & 0x80) ? nullptr : At<uint8_t*>(ms_pTriggerPool, 0);
    while (slot != 0) {
        slot--;
        uint8_t* prop = PoolAt(pool, slot);
        if (!prop) {
            continue;
        }
        uint8_t* trigger = At<uint8_t*>(prop, 0x23C);
        if (!trigger || trigger == firstTrigger || (trigger[0x3F] & 1) || static_cast<int8_t>(trigger[0x3C]) == *area || trigger[0x3D] == 5) {
            continue;
        }
        if (At<void*>(prop, 0x18)) {
            Entity_DeleteRwObject(prop);
        }
        reinterpret_cast<void(__thiscall*)(void*)>(static_cast<void*>(At<re::Ptr32<void>*>(prop, 0)[3]))(prop);
        World_Remove(prop);
        World_RemoveFromCollisionRecords(prop);
        reinterpret_cast<void(__thiscall*)(void*, int)>(static_cast<void*>(At<re::Ptr32<void>*>(prop, 0)[0]))(prop, 1);
        At<void*>(trigger, 0x1C) = nullptr;
    }
}

// 0x416690 (thiscall; `this` unused): the prop anims of `area`'s triggers made (not
// the first trigger's; those whose model is for the chapter's season)
void __fastcall CreateAreaProps(AreaTransitionManager*, re::Edx, const int32_t* area) {
    PushMemoryTag(2);
    int count = TriggerPool_GetSize(triggerPoolWrapper);
    for (int slot = 1; slot < count; slot++) {
        uint8_t* trigger = PoolAt(ms_pTriggerPool, slot);
        if (!trigger || At<void*>(trigger, 0x1C) != nullptr || (trigger[0x3F] & 1)) {
            continue;
        }
        int model = At<int32_t>(At<uint8_t*>(trigger, 0x20), 0x10);
        if (model == -1 || static_cast<int8_t>(trigger[0x3C]) != *area || trigger[0x3D] == 5) {
            continue;
        }
        uint8_t* seasons = ModelInfo_5201D0(ms_modelInfoPtrs[model]);
        int season = ms_chapterSeason[ms_currentChapter];
        if (!((seasons[0x74] && season == 0) || (seasons[0x75] && season == 1) || (seasons[0x77] && season == 3) ||
              (seasons[0x76] && season == 2))) {
            continue;
        }
        NiTransform transform;
        transform.scale = 1.0f;
        CVector axisZ = {0.0f, 0.0f, 1.0f};
        CVector axisY = {0.0f, 1.0f, 0.0f};
        CVector axisX = {1.0f, 0.0f, 0.0f};
        float rotZ = At<float>(trigger, 0x10);
        float rotY = At<float>(trigger, 0x14);
        float rotX = At<float>(trigger, 0x18);
        float degrees = static_cast<float>(rotZ - At<float>(trigger, 0x38) * 180.0 / 3.1415927410125732);
        Transform_CombineRotation(&transform, &axisZ, degrees, 0);
        Transform_CombineRotation(&transform, &axisY, rotY, 2);
        Transform_CombineRotation(&transform, &axisX, rotX, 2);
        Transform_CombineTranslation(&transform, reinterpret_cast<CVector*>(trigger + 4), 2);
        void* object = CreateTriggerObject_44CE70(model, static_cast<int8_t>(trigger[0x3C]), &transform, 0.0f, trigger, 1, 0);
        World_Add(object);
    }
    PopMemoryTag();
}

// 0x4168A0 (thiscall): the stored target for going out through a door: 2.5 behind the
// ped facing back, or (with the trigger's door object) in front of the door
void __fastcall SetDoorTarget(AreaTransitionManager* self, re::Edx, const int32_t* area, void* ped, uint8_t* trigger) {
    CVector pos = *PositionOf(ped);
    CVector forward;
    Placeable_GetForward(ped, &forward);
    Vector_Normalise(&forward);
    CVector back = {static_cast<float>(forward.x * 2.5), static_cast<float>(forward.y * 2.5), static_cast<float>(forward.z * 2.5)};
    pos.x = pos.x - back.x;
    pos.y = pos.y - back.y;
    pos.z = pos.z - back.z;
    float heading = static_cast<float>(HeadingOf(ped) - 3.1415927410125732);
    float z = pos.z;
    uint8_t* door = At<uint8_t*>(trigger, 0x1C);
    if (door) {
        uint8_t* colModel = At<uint8_t*>(ms_modelInfoPtrs[At<int16_t>(door, 0x10E)], 0xC);
        CMatrix matrix;
        Matrix_CopyConstruct(&matrix, Placeable_GetMatrix(door));
        CVector boxMin = {At<float>(colModel, 0x10), At<float>(colModel, 0x14), At<float>(colModel, 0x18)};
        CVector boxMax = {At<float>(colModel, 0x20), At<float>(colModel, 0x24), At<float>(colModel, 0x28)};
        CVector size;
        CVector* s = VectorSub(&size, &boxMax, &boxMin);
        CVector half = {static_cast<float>(s->x * 0.5), static_cast<float>(s->y * 0.5), static_cast<float>(s->z * 0.5)};
        // (computed and not used)
        CVector tmp, centre;
        VectorAdd(&centre, VectorAdd(&tmp, PositionOf(door), &boxMin), &half);
        CVector doorForward;
        CVector* f = Placeable_GetForward(door, &doorForward);
        CVector out = {-f->x, -f->y, -f->z};
        heading = HeadingFromVector_433310(&out);
        CVector offset = {static_cast<float>(out.x * 2.5), static_cast<float>(out.y * 2.5), static_cast<float>(out.z * 2.5)};
        CVector bound;
        CVector target;
        CVector* t = VectorAdd(&target, Entity_GetBoundCentre(door, &bound), &offset);
        pos.x = t->x;
        pos.y = t->y;
    }
    At<float>(self, 0x8A0) = pos.x;
    At<float>(self, 0x8AC) = heading;
    At<float>(self, 0x8A4) = pos.y;
    At<uint8_t>(self, 0x89C) = 1;
    At<float>(self, 0x8A8) = z;
    At<int32_t>(self, 0x8B0) = *area;
    At<uint8_t>(self, 0x8B4) = 0;
}

// 0x416B50 (thiscall): the player moved to `pos` at once (camera reset)
void __fastcall TeleportPlayer(AreaTransitionManager* self, re::Edx, const CVector* pos, float heading) {
    ms_bInTransition = 1;
    ResetPlayerInline();
    At<CVector>(self, 0x8C4) = *pos;
    At<CVector>(self, 0x8D0) = *PositionOf(PED_player);
    ATM_PlacePlayer(self, pos, heading);
    Camera_SetShot(TheCamera, 1, 0, 1, 0, 0);
    Camera_ResetController(TheCamera);
    ms_bInTransition = 0;
}

// 0x416C30 (thiscall): the world emptied of the old area (empty parked vehicles,
// other areas' props, effects, explosions, pickups...) and objects in the way of
// parked vehicles removed
void __fastcall ClearWorldForArea(AreaTransitionManager* self, re::Edx, const CVector* pos) {
    Fn_45B800();
    EmptyThiscall(reinterpret_cast<void*>(0x00CF5DC4));
    At<uint8_t>(self, 0x85D) = 1;
    uint8_t* pool = ms_pVehiclePool;
    ms_numPendingProps = 0;
    for (int slot = At<int32_t>(pool, 8); slot != 0;) {
        slot--;
        uint8_t* vehicle = PoolAt(pool, slot);
        if (vehicle && At<int32_t>(vehicle, 0x46C) == 1 && !VehicleSeats_GetOccupant(vehicle + 0x1C4, 0)) {
            reinterpret_cast<void(__thiscall*)(void*, int)>(static_cast<void*>(At<re::Ptr32<void>*>(vehicle, 0)[0x1C / 4]))(vehicle, 1);
        }
    }
    ATM_RemoveAreaProps(self, &currArea);
    Fn_596140(reinterpret_cast<void*>(0x00CF3DA0));
    EmptyThiscall(reinterpret_cast<void*>(0x00CF5DC4));
    Fn_4A7750(reinterpret_cast<void*>(0x00C2C730));
    Fn_4D4A30();
    Fn_534510(pos);
    ClearAllExplosions();
    Fn_444BB0();
    Fn_59B190(reinterpret_cast<void*>(0x00CF5DC4));
    Fn_5A6420(reinterpret_cast<void*>(0x00CF4284), 0);
    for (int slot = At<int32_t>(pool, 8); slot != 0;) {
        slot--;
        uint8_t* vehicle = PoolAt(pool, slot);
        if (!vehicle || At<int32_t>(vehicle, 0x46C) != 1 || VehicleSeats_GetOccupant(vehicle + 0x1C4, 0)) {
            continue;
        }
        for (uint8_t* entry = At<uint8_t*>(vehicle, 0x178); entry; entry = At<uint8_t*>(entry, 0x10)) {
            ATM_RemoveColliding(At<uint32_t*>(At<uint8_t*>(entry, 8), 4), vehicle);
            ATM_RemoveColliding(At<uint32_t*>(At<uint8_t*>(entry, 8), 0x10), vehicle);
        }
    }
    GenerateAllBoundingBoxes(0);
    LurStore_5DAF40(TheLurStore);
    EmptyThiscall(reinterpret_cast<void*>(0x00CF5DC4));
    At<int32_t>(self, 4)++;
    At<uint8_t>(self, 0x85D) = 0;
}

// 0x416DC0 (thiscall): the current area's props made
void __fastcall CreateCurrentAreaProps(AreaTransitionManager* self, re::Edx) {
    ATM_CreateAreaProps(self, &currArea);
}

// 0x416DD0 (thiscall): the loading-screen transition's state machine (+0x868); true
// while nothing moved on
uint32_t __fastcall UpdateLoadingTransition(AreaTransitionManager* self, re::Edx) {
    switch (At<int32_t>(self, 0x868)) {
    case 1:
        if (LurStore_IsLoading_5DBC40(TheLurStore)) {
            return 1;
        }
        {
            const char* name = GetAreaName(&At<int32_t>(self, 0x888));
            Fn_438820();
            LurStore_Load_5DC180(TheLurStore, name, 1);
        }
        At<int32_t>(self, 0x868) = 2;
        return 0;
    case 2:
        if (At<uint8_t>(self, 0x870) == 1) {
            At<uint8_t>(self, 0x870) = 0;
            At<int32_t>(self, 0x868) = 3;
        }
        return 1;
    case 3: {
        At<int32_t>(self, 0x868) = 4;
        ATM_ResetPlayer();
        ATM_SetArea(self, &At<int32_t>(self, 0x888));
        Fn_48B480(object_C2B300);
        CVector* target = &At<CVector>(self, 0x878);
        ATM_ClearWorldForArea(self, target);
        At<CVector>(self, 0x8C4) = *target;
        At<CVector>(self, 0x8D0) = *PositionOf(PED_player);
        ATM_PlacePlayer(self, target, At<float>(self, 0x884));
        Camera_ResetController(TheCamera);
        ms_bInTransition = 0;
        return 0;
    }
    case 4:
        At<int32_t>(self, 0x868) = 0;
        At<uint8_t>(self, 0x86D) = 0;
        At<uint8_t>(self, 0x870) = 0;
        return 0;
    default:
        if (!At<uint8_t>(self, 0x86D)) {
            return 1;
        }
        At<uint8_t>(self, 0x86D) = 0;
        if (At<uint8_t>(self, 0x86E)) {
            LurStore_5DBC80(TheLurStore);
            At<int32_t>(self, 0x868) = 1;
            return 0;
        }
        At<int32_t>(self, 0x868) = 3;
        return 0;
    }
}

// the fade to black (a = 255)
void FadeOut(float time, int direction, int flags) {
    ms_fadeColour[0] = 0;
    ms_fadeColour[1] = 0;
    ms_fadeColour[2] = 0;
    ms_fadeColour[3] = 0xFF;
    Camera_Fade(TheCamera, time, direction, flags);
}
double TimeMs() {
    double now = static_cast<double>(static_cast<int32_t>(m_snTimeInMilliseconds));
    if (static_cast<int32_t>(m_snTimeInMilliseconds) < 0) {
        now += 4294967296.0;
    }
    return now;
}

// 0x416F80 (thiscall): the transition's state machine (+0x864): idle (a route
// trigger the player is in starts one), fade out, load the area, move the player,
// wait for the models, fade in. True while nothing moved on.
uint32_t __fastcall UpdateTransition(AreaTransitionManager* self, re::Edx) {
    uint8_t* player = PED_player;
    uint32_t result = 1;
    switch (At<int32_t>(self, 0x864)) {
    case 1:
        Pad_StopRumble();
        UserInputManager_SetEnabled3(userInputManager, 0);
        At<int32_t>(PED_player, 0x1344) = 1;
        At<int32_t>(PED_player, 0x1348) = 1;
        At<int32_t>(PED_player, 0x133C) = 0;
        if (currArea == At<int32_t>(self, 0x888)) {
            const char* name = GetAreaName(&At<int32_t>(self, 0x888));
            At<int32_t>(self, 0x864) = LurStore_5DBCC0(TheLurStore, name) ? 7 : 5;
            return 0;
        }
        if (At<uint8_t>(self, 0x85F) == 0 && Camera_GetFadeState(TheCamera) != 1) {
            At<int32_t>(self, 0x864) = 2;
            return 1;
        }
        At<int32_t>(self, 0x864) = 5;
        return 0;
    case 2:
        if (At<uint8_t>(self, 0x8DC) != 1) {
            At<int32_t>(self, 0x8EC) = At<int32_t>(self, 0x8E0);
            At<int32_t>(self, 0x864) = 4;
            return 1;
        }
        FadeOut(At<float>(g_transitionTimes, 0x18), 1, 0);
        At<int32_t>(self, 0x864) = 3;
        return 1;
    case 3:
        if (Camera_GetFadeState(TheCamera) != 1) {
            return 1;
        }
        if (At<uint8_t>(self, 0x85E) == 0) {
            Fn_55E850();
            At<uint8_t>(self, 0x85E) = 1;
            return 1;
        }
        At<int32_t>(self, 0x864) = 5;
        At<uint8_t>(self, 0x85E) = 0;
        return 0;
    case 4:
        At<int32_t>(self, 0x8EC)--;
        if (At<int32_t>(self, 0x8EC) > 0 && !re::SeamlessDoorCrossed()) {   // (the remaster's: runflags.h)
            return 1;
        }
        if (re::SeamlessDoorSkipsFade(At<int32_t>(self, 0x888))) {
            // (the remaster's seamless doors: the area is loaded already, so no black; what state
            // 3 does once the screen is black: SeamlessDoors.cpp)
            Fn_55E850();
            At<int32_t>(self, 0x864) = 5;
            return 0;
        }
        FadeOut(0.0f, 1, 1);
        At<int32_t>(self, 0x864) = 3;
        return 1;
    case 5:
        At<uint8_t>(self, 0x8DC) = 0;
        At<int32_t>(self, 0x8E0) = 0xE;
        At<int32_t>(self, 0x8E4) = 0;
        At<int32_t>(self, 0x8E8) = 0xB4;
        LurStore_5DBC80(TheLurStore);
        At<int32_t>(self, 0x864) = 6;
        return 0;
    case 6: {
        if (LurStore_IsLoading_5DBC40(TheLurStore)) {
            return 1;
        }
        const char* name = GetAreaName(&At<int32_t>(self, 0x888));
        Fn_438820();
        LurStore_Load_5DC180(TheLurStore, name, 1);
        At<int32_t>(self, 0x864) = 8;
        return 0;
    }
    case 7:
        At<int32_t>(self, 0x864) = 10;
        return 0;
    case 8:
        if (At<uint8_t>(self, 0x870) != 1) {
            return 1;
        }
        At<uint8_t>(self, 0x870) = 0;
        At<int32_t>(self, 0x864) = 9;
        return 0;
    case 9: {
        // (the remaster's seamless doors, runflags.h: the player carries on from just inside the
        // door, where they were seen going, with the camera looking the same way)
        CVector through;
        CVector look;
        float throughHeading = 0.0f;
        bool seamless = re::SeamlessDoorPlacement(&through.x, &throughHeading, &look.x);
        ATM_ResetPlayer();
        ATM_SetArea(self, &At<int32_t>(self, 0x888));
        if (!seamless || !re::TwoAreas()) {
            Fn_48B480(object_C2B300);   // (the peds stopped: the seamless door's go on, both sides')
        }
        CVector* target = &At<CVector>(self, 0x878);
        if (seamless) {
            re::SeamlessDoorSwitch();   // (the area left kept: before the world is cleared for the new one)
        }
        ATM_ClearWorldForArea(self, target);
        if (seamless) {
            ATM_PlacePlayer(self, &through, throughHeading);
            Camera_ControllerLookAt(TheCamera, look);
        } else {
            ATM_PlacePlayer(self, target, At<float>(self, 0x884));
            if (At<uint8_t>(self, 0x88C)) {
                LookFrom(target, &At<CVector>(self, 0x890));
            } else {
                Camera_ResetController(TheCamera);
            }
        }
        re::SeamlessDoorArrived();   // (the remaster's: runflags.h)
        if (At<uint8_t>(self, 0x85F)) {
            At<int32_t>(self, 0x864) = 0xD;
            Game_TidyUpMemory(1, 1);
            return 0;
        }
        int wanted = static_cast<int>(ms_numModelsRequested * 0.5);
        At<int32_t>(self, 0x854) = wanted;
        if (wanted < 0) {
            At<int32_t>(self, 0x854) = 0;
        } else if (wanted > ms_numModelsRequested) {
            At<int32_t>(self, 0x854) = ms_numModelsRequested;
        }
        At<int32_t>(self, 0x864) = 0xB;
        At<float>(self, 0x850) = static_cast<float>(TimeMs() + 20000.0);
        Game_TidyUpMemory(1, 1);
        return 0;
    }
    case 10: {
        ATM_ResetPlayer();
        bool clear = true;
        if (At<uint8_t>(self, 0x860) == 0) {
            float xy[2] = {At<float>(self, 0x878), At<float>(self, 0x87C)};
            if (HasCollisionLoadedExcept(xy)) {
                clear = false;
            }
        }
        Fn_48B480(object_C2B300);
        if (clear) {
            ATM_ClearWorldForArea(self, &At<CVector>(self, 0x878));
        }
        CVector* target = &At<CVector>(self, 0x878);
        ATM_PlacePlayer(self, target, At<float>(self, 0x884));
        if (At<uint8_t>(self, 0x88C)) {
            LookFrom(target, &At<CVector>(self, 0x890));
        } else {
            Camera_ResetController(TheCamera);
        }
        At<int32_t>(self, 0x864) = 0xD;
        return 0;
    }
    case 0xB:
        if (static_cast<double>(At<float>(self, 0x850)) < TimeMs() || ms_numModelsRequested <= At<int32_t>(self, 0x854)) {
            Camera_Fade(TheCamera, At<float>(g_transitionTimes, 0x1C), 0, 0);
            At<int32_t>(self, 0x864) = 0xC;
            return 0;
        }
        return 1;
    case 0xC: {
        if (ms_bInputDisabled_A45A4F == 1) {
            At<int32_t>(self, 0x864) = 0xD;
            return 0;
        }
        float alpha = static_cast<float>(ms_fadeAlpha / 255.0);
        if (alpha < At<float>(g_transitionTimes, 0x24)) {
            UserInputManager_SetEnabled3(userInputManager, 1);
            At<int32_t>(self, 0x864) = 0xD;
            return 0;
        }
        return 1;
    }
    case 0xD:
        UserInputManager_SetEnabled3(userInputManager, 1);
        At<int32_t>(PED_player, 0x1344) = 0;
        At<int32_t>(PED_player, 0x1348) = 0;
        At<int32_t>(PED_player, 0x133C) = 1;
        At<int32_t>(self, 0x394) = 0;
        At<uint8_t>(self, 0x86C) = 0;
        At<uint8_t>(self, 0x870) = 0;
        At<int32_t>(self, 0x864) = 0;
        return 0;
    default:
        break;
    }
    // idle (0, and anything past 0xD)
    if (At<uint8_t>(self, 0x86C)) {
        At<int32_t>(self, 0x864) = 1;
        At<uint8_t>(self, 0x86C) = 0;
        return 0;
    }
    for (int i = 0; i < At<int32_t>(self, 8); i++) {
        void* trigger = At<void*>(self, 0x10 + i * 8);
        uint8_t* route = At<uint8_t*>(self, 0x14 + i * 8);
        if (!trigger || !route) {
            continue;
        }
        float* first = reinterpret_cast<float*>(Route_First(route));
        float* next = reinterpret_cast<float*>(Route_Next(route));
        CVector direction = {0.0f, 0.0f, 0.0f};
        CVector pos = {first[0], first[1], first[2]};
        float heading = static_cast<float>((first[7] - 90.0) * 3.1415927410125732 / 180.0);
        int32_t area = static_cast<int8_t>(route[0xC]);
        bool hasNext = next != nullptr;
        if (next) {
            direction = {next[0], next[1], next[2]};
        }
        int8_t flag = static_cast<int8_t>(At<uint8_t>(self, 0x330 + i));
        if (flag == 0) {
            if (!Trigger_IsPointInside_6D6FA0(trigger, Placeable_GetPosition(player))) {
                continue;
            }
        } else if (flag == 1) {
            if (At<void*>(self, 0x10 + i * 8) != At<void*>(self, 0x394)) {
                continue;
            }
            if (At<uint8_t>(self, 0x86F)) {
                ATM_RequestTransition(self, &At<CVector>(self, 0x8A0), At<float>(self, 0x8AC), &At<int32_t>(self, 0x8B0),
                                      At<uint8_t>(self, 0x8B4) ? &At<CVector>(self, 0x8B8) : nullptr);
                ATM_ClearFadeState(self);
                At<uint8_t>(self, 0x86F) = 0;
                At<int32_t>(self, 0xC) = i;
                At<uint8_t>(self, 0x86C) = 1;
                return 0;
            }
            if (Fn_42E6C0(area)) {
                ATM_SetDoorTarget(self, &currArea, player, At<void*>(self, 0x394));
                At<uint8_t>(self, 0x86F) = 1;
            }
        } else {
            continue;
        }
        ATM_RequestTransition(self, &pos, heading, &area, hasNext ? &direction : nullptr);
        At<int32_t>(self, 0xC) = i;
        At<uint8_t>(self, 0x86C) = 1;
        return 0;
    }
    return result;
}

GAME_FN(uint32_t, __thiscall, ATM_UpdateLoadingTransition, 0x00416DD0, AreaTransitionManager* self);
GAME_FN(uint32_t, __thiscall, ATM_UpdateTransition, 0x00416F80, AreaTransitionManager* self);

// 0x417820 (thiscall): both state machines run until they settle (when enabled)
void __fastcall Update(AreaTransitionManager* self, re::Edx) {
    if (At<uint8_t>(self, 0x85C)) {
        re::SeamlessDoorsFrame();   // (the remaster's: runflags.h)
        while (ATM_UpdateLoadingTransition(self) == 0) {
        }
        int32_t before = At<int32_t>(self, 0x864);
        while (ATM_UpdateTransition(self) == 0) {
            if (re::DoorReport() && At<int32_t>(self, 0x864) != before) {
                std::fprintf(stderr, "doors: transition state %d -> %d at %u ms (area %d)\n", before, At<int32_t>(self, 0x864),
                             m_snTimeInMilliseconds, currArea);
                before = At<int32_t>(self, 0x864);
            }
        }
        if (re::DoorReport() && At<int32_t>(self, 0x864) != before) {
            std::fprintf(stderr, "doors: transition state %d -> %d at %u ms (area %d)\n", before, At<int32_t>(self, 0x864),
                         m_snTimeInMilliseconds, currArea);
        }
    }
}

} // namespace c01atm2

// ---- registration ----------------------------------------------------------------------

#define ATM2_REVERSED_C01(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace c01atm2 {
ATM2_REVERSED_C01("AreaTransitionManager::RemoveColliding", 0x00416290, &RemoveColliding);
ATM2_REVERSED_C01("AreaTransitionManager::PlacePlayer", 0x00416440, &PlacePlayer);
ATM2_REVERSED_C01("AreaTransitionManager::RemoveAreaProps", 0x004165E0, &RemoveAreaProps);
ATM2_REVERSED_C01("AreaTransitionManager::CreateAreaProps", 0x00416690, &CreateAreaProps);
ATM2_REVERSED_C01("AreaTransitionManager::SetDoorTarget", 0x004168A0, &SetDoorTarget);
ATM2_REVERSED_C01("AreaTransitionManager::TeleportPlayer", 0x00416B50, &TeleportPlayer);
ATM2_REVERSED_C01("AreaTransitionManager::ClearWorldForArea", 0x00416C30, &ClearWorldForArea);
ATM2_REVERSED_C01("AreaTransitionManager::CreateCurrentAreaProps", 0x00416DC0, &CreateCurrentAreaProps);
ATM2_REVERSED_C01("AreaTransitionManager::UpdateLoadingTransition", 0x00416DD0, &UpdateLoadingTransition);
ATM2_REVERSED_C01("AreaTransitionManager::UpdateTransition", 0x00416F80, &UpdateTransition);
ATM2_REVERSED_C01("AreaTransitionManager::Update", 0x00417820, &Update);
} // namespace c01atm2
