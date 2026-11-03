// CAutomobile (vtable 0x905D04): its collisions with the world and other entities,
// being put on the ground, its creation as a copy, and its per-frame processing (vf13).
// Fields are reached by offset: CAutomobile's layout (Vehicle.h) only has a few named.

#include "game/Vehicle.h"
#include "game/World.h"

#include <cmath>
#include <cstring>
#include "reversed.h"

namespace {

using veh::CVector;

using re::At;

// a virtual by its byte offset in the vtable
template <typename R, typename... A>
R Virtual(void* object, int offset, A... args) {
    re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object);
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>(vtable[offset / 4]))(object, args...);
}

GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);     // CTimer's
GAME_VAR(float, timeStep, 0x00C1A9A4);        // CTimer's
GAME_VAR(CPed*, playerPed, 0x00C1AEA8);       // (FindPlayerPed's)
GAME_VAR(CVector, vec0, 0x00BCC564);          // (a zero vector)
GAME_VAR(CEntity*, entityC2A8E8, 0x00C2A8E8); // (compared with, both ways the same)
GAME_VAR(uint16_t, currentScanCode, 0x00C1AE88);   // CWorld::ms_nCurrentScanCode
GAME_VAR(uint8_t, flagA35739, 0x00A35739);
GAME_ARRAY(uint8_t*, modelInfoPtrs, 0x00C67738);   // CModelInfo::ms_modelInfoPtrs (+0xC the col model)

GAME_FN(void, __cdecl, ClearScanCodes, 0x0045D840);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, CEntity* entity, re::Ptr32<CEntity>* ref);
GAME_FN(CColPoint*, __thiscall, ColPoint_Assign, 0x0046AA70, CColPoint* self, const CColPoint* other);
GAME_FN(float*, __thiscall, Entity_GetMatrix, 0x008B6120, void* entity);
GAME_FN(void, __thiscall, Physical_ProcessCollisionWith, 0x0046C600, void* physical, const float* matrix, void* other,
        CColPoint* colPoint, int arg);
GAME_FN(void*, __thiscall, Matrix_Copy, 0x00412A10, veh::CMatrix* m, const float* from);
GAME_FN(void, __thiscall, Matrix_SetUp, 0x00413600, veh::CMatrix* m, float x, float y, float z, float unused);
GAME_FN(void, __thiscall, ColLine_Set, 0x00564950, void* line, const CVector* start, const CVector* end);
GAME_FN(uint8_t, __cdecl, Col_ProcessLineOfSight, 0x0056B4D0, const void* line, veh::CMatrix* m, void* col, CColPoint* point,
        float* dist, uint32_t flags, int surfaces, uintptr_t g);
GAME_FN(uint8_t, __cdecl, Col_ProcessSweep, 0x0056C8C0, veh::CMatrix* to, veh::CMatrix* from, void* colModel, veh::CMatrix* matrixA,
        veh::CMatrix* matrixB, void* colModel2, CColPoint* colPoint, int, int, int, int, int, int lists, uintptr_t, int);
GAME_FN(float, __cdecl, Fn4737D0, 0x004737D0, float normalZ);   // ((1 - z) times a constant)
GAME_FN(void, __thiscall, Entity_SetPosition, 0x0041C4E0, void* entity, float x, float y, float z);
GAME_FN(void, __thiscall, Physical_RemoveAndAdd, 0x00469470, void* entity);

// an empty collision point as the game's code makes one (its piece and surfaces left
// as the stack had them)
void InitColPoint(CColPoint& p) {
    p.point = {0.0f, 0.0f, 0.0f};
    p.entity = nullptr;
    p.normal = {0.0f, 0.0f, 0.0f};
    p.depth = 3.4028234663852886e+38f;
}

void ReleaseColPoint(CColPoint& p) {
    if (p.entity) {
        // (the game compares it with 0xC2A8E8 first; both ways do this)
        CleanUpOldReference(p.entity, &p.entity);
    }
}

const CVector* Position(void* entity) {
    CVehicle* e = static_cast<CVehicle*>(entity);
    return e->matrix ? reinterpret_cast<const CVector*>(&e->matrix->m[12]) : &e->placementPos;
}
GAME_FN(int32_t*, __thiscall, Vehicle_GetVehicleClass, 0x004CAF30, void* vehicle, int32_t* out);
// (0x0045D500 stores 16 bytes through `out4`: called through re::CallVec16)
static inline void Matrix_MultiplyPoint4(float* out4, const float* m, const float* v) { re::CallVec16<0x0045D500, void>(out4, m, v); }   // (writes 4 floats)
GAME_FN(uint8_t, __cdecl, ProcessVerticalLine, 0x004629C0, const CVector* point, float zStop, CColPoint* colPoint,
        re::Ptr32<CEntity>* entity, int lists, int arg);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_VAR(CVehicle*, ignoreEntity, 0x00C1AE70);   // CWorld::pIgnoreEntity
GAME_VAR(CVector, vec9E33DC, 0x009E33DC);        // (a direction: gravity's?)
GAME_FN(CVehicle*, __thiscall, Vehicle_Construct, 0x004CB730, CVehicle* vehicle, int createdBy);
GAME_FN(void, __thiscall, DamageManager_Construct, 0x004C7D50, void* damage);
GAME_FN(void, __thiscall, DamageManager_SetDoorStatus, 0x004C7DF0, void* damage, int door, int status);
GAME_FN(void, __thiscall, Effect_Construct, 0x00668E90, void* effect);
GAME_FN(void, __thiscall, Vehicle_SetModelIndex, 0x004CB190, CVehicle* vehicle, int model, int arg);
GAME_FN(void, __thiscall, Vehicle_SetStatus, 0x004CB2F0, CVehicle* vehicle, int status);
GAME_FN(void, __thiscall, Automobile_FindFrames, 0x004AAAA0, CAutomobile* automobile);
GAME_FN(void, __thiscall, Automobile_ResetEffects, 0x004A84E0, CAutomobile* automobile);
GAME_FN(void, __thiscall, Automobile_SetupSuspensionLines, 0x004AAB00, CAutomobile* automobile);
GAME_FN(void, __thiscall, VehicleModel_ChooseVehicleColour, 0x00535D90, void* modelInfo, uint8_t* colour1, uint8_t* colour2);
GAME_FN(void, __thiscall, Seats_Init, 0x004CE470, CVehicleSeats* seats, CVehicle* vehicle);
GAME_FN(void*, __cdecl, MemoryMgr_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void*, __cdecl, MemoryMgr_Malloc2, 0x005EEA50, uint32_t size, int arg);
GAME_FN(void*, __thiscall, CollisionData_Construct, 0x0056DAD0, void* data);
GAME_ARRAY(uint8_t, handlingData, 0x00C2DEE4);   // mod_HandlingManager's entries (0xDC each)
inline void* const automobileVtable = reinterpret_cast<void*>(0x00905D04);
GAME_FN(CEntity*, __cdecl, GetEntityFromNode, 0x0044A3E0, int type, int index);
GAME_VAR(uint32_t*, ptrNodes, 0x00C0F788);        // the sector lists' nodes (packed: type, index, next)
GAME_VAR(int32_t, modelBF42DC, 0x00BF42DC);       // (a ped model left alone)
GAME_VAR(uint8_t, chapter, 0x00A147E0);
GAME_FN(float, __thiscall, Entity_GetBoundRadius, 0x00465E10, void* entity);
GAME_FN(void, __thiscall, Entity_GetBoundCentre, 0x00466CA0, void* entity, CVector* out);
GAME_FN(uint8_t, __thiscall, Entity_GetIsTouchingMoving, 0x00465D10, void* entity, const CVector* centre, float radius);
GAME_FN(uint8_t, __thiscall, Ped_4763C0, 0x004763C0, void* ped, CVehicle* vehicle);
GAME_FN(uint8_t, __thiscall, Ped_ProcessVehicleHit, 0x00483690, void* ped, CVehicle* vehicle, void* colModel,
        const float* matrix, const CVector* move, float length);
GAME_FN(void, __thiscall, Automobile_4AA160, 0x004AA160, CAutomobile* automobile, void* ped);
inline void* const eventsCF4378 = reinterpret_cast<void*>(0x00CF4378);
GAME_FN(void, __thiscall, Events_Add, 0x005A4920, void* events, void* out, void* ped, uintptr_t a, int b, int kind, int c, int d,
        int e, int f, int g, uintptr_t h);
GAME_FN(CVector*, __thiscall, Entity_GetForward, 0x004154A0, void* entity, CVector* out);
GAME_FN(CVector*, __thiscall, Placeable_GetPosition, 0x00415460, void* placeable);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(uint8_t, __thiscall, Game_PlaceEntityOnTop, 0x004B6C50, CAutomobile* automobile, void* entity);
GAME_FN(float*, __thiscall, Placeable_GetMatrix, 0x0046DD20, void* placeable);   // (a temporary one without a matrix)
GAME_FN(void*, __cdecl, GetEntityColModel, 0x0051AF50, void* entity);
GAME_FN(void, __cdecl, Col_ProcessWheels, 0x00569DC0, const float* matrix, void* colModel, veh::CMatrix* otherMatrix,
        void* otherColModel, void* wheelColPoints, float* wheelCompressions);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, void* entity, void* ref);
GAME_FN(void, __thiscall, Automobile_SetDamage, 0x004A8410, void* automobile, int piece, float impulse, void* entity,
        const CVector* pos);
GAME_FN(void, __thiscall, Physical_ApplyCollision, 0x0046B870, void* physical, void* entity, CColPoint* colPoint, float arg);
GAME_FN(void, __thiscall, Physical_46C9D0, 0x0046C9D0, void* physical, const float* matrix, CColPoint* colPoint, int arg);
GAME_FN(void, __thiscall, Physical_SetPositionV, 0x0041EE20, void* entity, const CVector* pos);
GAME_FN(void, __thiscall, Matrix_UpdateRW, 0x00413A80, veh::CMatrix* m);
GAME_FN(float, __thiscall, Vector_DotProduct, 0x00468CD0, const CVector* a, const CVector* b);
GAME_FN(void, __thiscall, Vector_Scale, 0x0088A480, CVector* v, float s);
GAME_FN(float, __cdecl, MaxFloat, 0x00414490, float a, float b);   // (b unless it's under a)
GAME_FN(CVector*, __cdecl, VectorScale, 0x00412350, CVector* out, const CVector* v, float s);
GAME_FN(float, __thiscall, Vector_MagnitudeSqr, 0x0040D9F0, const CVector* v);
GAME_FN(uint8_t, __thiscall, Seats_IsOccupied, 0x004CE660, CVehicleSeats* seats, uint32_t seat);
GAME_FN(CPed*, __thiscall, Seats_GetOccupant, 0x004CE560, CVehicleSeats* seats, uint32_t seat);
GAME_VAR(int32_t, modelSiren2, 0x00BF431C);
GAME_VAR(uint8_t, flagC2D160, 0x00C2D160);        // (a collision happened this frame)
inline uint8_t* const colModelC1AEAC = reinterpret_cast<uint8_t*>(0x00C1AEAC);   // (the one used with flags33A 0x80)
inline void* const objC3B808 = reinterpret_cast<void*>(0x00C3B808);
GAME_FN(void, __thiscall, Obj_4E9DF0, 0x004E9DF0, void* object, int on);
GAME_FN(void, __thiscall, Matrix_SetTranslate, 0x004127C0, veh::CMatrix* m, const CVector* pos);
GAME_FN(veh::CMatrix*, __cdecl, Matrix_Multiply, 0x004138D0, veh::CMatrix* out, const float* a, const veh::CMatrix* b);
GAME_FN(void, __thiscall, Matrix_CopyRotPos, 0x00412720, veh::CMatrix* m, const veh::CMatrix* from);
GAME_FN(veh::CMatrix*, __cdecl, Matrix_Invert, 0x0041A890, veh::CMatrix* out, const veh::CMatrix* m);
// (0x00414C80 stores 16 bytes through `out`: called through re::CallVec16)
static inline void TransformVector2(CVector* out, const veh::CMatrix* m, const CVector* v) { re::CallVec16<0x00414C80, void>(out, m, v); }
GAME_FN(int, __thiscall, Vehicle_4CDC30, 0x004CDC30, CVehicle* vehicle, veh::CMatrix* before, float radius, float* arg3,
        CColPoint* colPoint, int kind, int arg6, CVector* offset);
GAME_FN(void, __thiscall, Vehicle_4CCD30, 0x004CCD30, CVehicle* vehicle, veh::CMatrix* before, float* arg2, int hasDriver,
        CColPoint* colPoint, int kind, int arg6);
GAME_FN(void, __thiscall, Entity_443AA0, 0x00443AA0, void* entity);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const CVector* v);
GAME_FN(void, __thiscall, Vector_SubAssign, 0x00415230, CVector* v, const CVector* other);
GAME_FN(CVector*, __thiscall, Physical_GetUp, 0x00473730, void* vehicle, CVector* out);
GAME_FN(void, __thiscall, Placeable_SetMatrix, 0x0046DE90, void* placeable, const veh::CMatrix* m, int force);
GAME_FN(void, __thiscall, Physical_SetMoveSpeed, 0x0041A440, void* physical, const CVector* speed);
GAME_FN(void, __thiscall, PathFollower_SetSpeed, 0x00417C50, void* follower, float speed);
GAME_FN(void, __cdecl, UpdateCarOnRails, 0x00422A10, CVehicle* vehicle);
GAME_FN(void, __cdecl, UpdateCarOnRailsFollowingPoints, 0x00422EB0, CVehicle* vehicle);
GAME_FN(void, __thiscall, SetRef, 0x005D7F10, void* ref, void* entity);
GAME_FN(uint8_t, __thiscall, Game_ProcessPedCollisions, 0x004B8110, CAutomobile* automobile, const veh::CMatrix* matrix);
GAME_FN(uint8_t, __thiscall, Game_CheckVehicleCollision, 0x004B6B00, CAutomobile* automobile, const CColPoint* colPoint,
        const veh::CMatrix* matrix);
GAME_FN(void, __thiscall, Game_PushOutOfGround, 0x004B6F80, CAutomobile* automobile);
// the sounds
GAME_VAR(void*, soundBank, 0x00B6F1E8);
GAME_VAR(void*, soundManager, 0x00B6F1D4);
GAME_VAR(uint16_t, soundGoKartHit, 0x020C9004);   // (class 0xA's)
GAME_FN(uint16_t, __thiscall, SoundBank_GetId, 0x00667B60, void* bank, const char* name);
GAME_FN(void, __thiscall, SoundManager_Play, 0x00668290, void* manager, uint32_t* id, void* sound, int arg);
GAME_FN(void, __thiscall, Sound_Construct, 0x00668E90, void* sound);
GAME_FN(void, __thiscall, Sound_Destruct, 0x00668EB0, void* sound);
GAME_FN(void, __thiscall, Sound_SetPosition, 0x00669040, void* sound, const CVector* pos, int arg);
GAME_FN(void, __thiscall, Sound_SetVolume_x64, 0x00668C40, void* sound, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Sound_SetVolume(void* a0, float a1) {
    return Sound_SetVolume_x64(a0, re::BitsOf(a1));
}

// the world position of a point halfway between two of its suspension lines' starts,
// times `scale`, by its matrix (vf13's)
CVector LinesMidpoint(CAutomobile* self, const float* a, const float* b, float scale, veh::CMatrix* translate,
                      veh::CMatrix* moved) {
    CVector sum;
    VectorAdd(&sum, reinterpret_cast<const CVector*>(a), reinterpret_cast<const CVector*>(b));
    CVector mid = {float(sum.x * 0.5), float(sum.y * 0.5), float(sum.z * 0.5)};
    CVector s = {mid.x * scale, mid.y * scale, mid.z * scale};
    Matrix_SetTranslate(translate, &s);
    veh::CMatrix product;
    Matrix_Multiply(&product, Entity_GetMatrix(self), translate);
    Matrix_CopyRotPos(moved, &product);
    return {moved->m[12], moved->m[13], moved->m[14]};
}
CVehicleSeats* Seats(void* vehicle) {
    return &static_cast<CVehicle*>(vehicle)->seats;
}

// 0x4B6B00: its collisions processed (with `colPoint` given, just that one; else each
// entity in its collision list, against `matrix` or its own); a vehicle hit makes it
// wait (+0x2DB, +0x2DC 3 s from now) and returns 1
uint8_t __fastcall Automobile_CheckVehicleCollision(CAutomobile* self, re::Edx, const CColPoint* colPoint, const float* matrix) {
    if (currentScanCode >= 0xFFFF) {
        ClearScanCodes();
        currentScanCode = 1;
    } else {
        currentScanCode++;
    }
    CColPoint point;
    InitColPoint(point);
    flagA35739 = 0;
    if (colPoint) {
        ColPoint_Assign(&point, colPoint);
    } else {
        uint8_t* node = static_cast<uint8_t*>(self->collisionList);
        if (!node) {
            return 0;
        }
        do {
            const float* m = matrix ? matrix : Entity_GetMatrix(self);
            Physical_ProcessCollisionWith(self, m, At<uint8_t*>(node, 8) + 8, &point, 0);
            node = At<uint8_t*>(node, 0x10);
        } while (node);
    }
    if (!point.entity) {
        return 0;
    }
    if ((re::PtrCast<CVehicle*>(point.entity)->typeAndStatus & 7) == 2) {
        self->unknown2DB = 1;
        At<uint32_t>(self, 0x2DC) = timeInMs + 0xBB8;
        ReleaseColPoint(point);
        return 1;
    }
    ReleaseColPoint(point);
    return 0;
}

// 0x4B6C50: an entity put on top of it: a line from 5 above the entity to 5 below
// through its collision; when it hits, the entity stood upright (up (0, 0, 1)) and swept
// from 1 above the hit down, and put where the sweep stopped (or on the hit)
uint8_t __fastcall Automobile_PlaceEntityOnTop(CAutomobile* self, re::Edx, CVehicle* entity) {
    CVector pos = *Position(entity);
    pos.z = float(pos.z + 5.0);
    CVector end = {pos.x, pos.y, float(pos.z - 10.0)};
    alignas(4) uint8_t line[0x20];
    ColLine_Set(line, &pos, &end);
    void* colModel = At<void*>(modelInfoPtrs[self->modelIndex], 0xC);
    CColPoint hit;
    InitColPoint(hit);
    float dist = 1.0f;
    veh::CMatrix own;
    Matrix_Copy(&own, Entity_GetMatrix(self));
    if (!Col_ProcessLineOfSight(line, &own, colModel, &hit, &dist, 0, 10, 0)) {
        ReleaseColPoint(hit);
        return 0;
    }
    pos.z = Fn4737D0(hit.normal.z) + hit.point.z;
    veh::CMatrix from;
    Matrix_Copy(&from, Entity_GetMatrix(entity));
    Matrix_SetUp(&from, 0.0f, 0.0f, 1.0f, 0.0f);   // (the 4th float is whatever the stack had)
    CVector start = {pos.x, pos.y, float(pos.z + 1.0)};
    veh::CMatrix to;
    Matrix_Copy(&to, from.m);
    to.m[12] = start.x;
    to.m[13] = start.y;
    to.m[14] = start.z;
    // (to.m[15] gets an uninitialised stack float in the game; ours keeps from's)
    CColPoint stop;
    InitColPoint(stop);
    uint8_t found = Col_ProcessSweep(&to, &from, At<void*>(modelInfoPtrs[entity->modelIndex], 0xC), &own, &own, colModel,
                                     &stop, 0, 0, 0, 0, 1, 10, 0, 1);
    if (found) {
        Entity_SetPosition(entity, stop.point.x, stop.point.y, stop.point.z);
    } else {
        Entity_SetPosition(entity, pos.x, pos.y, pos.z);
    }
    Physical_RemoveAndAdd(entity);
    ReleaseColPoint(stop);
    ReleaseColPoint(hit);
    return 1;
}

// 0x4B6F80: for vehicle classes 6 and 7 (upright): its four suspension lines' starts
// (the collision's lines) cast down to 6 below; where one is in the ground past 3/4 of
// its length it's pushed out, along the ground's normal (straight along 0x9E33DC when
// the ground is steeper than 45 degrees or flatter than about 22), each by its share
void __fastcall Automobile_PushOutOfGround(CAutomobile* self) {
    int32_t cls;
    if (*Vehicle_GetVehicleClass(self, &cls) != 7 && *Vehicle_GetVehicleClass(self, &cls) != 6) {
        return;
    }
    if (self->matrix && 0.0f > self->matrix->m[10]) {
        return;
    }
    uint8_t* col = At<uint8_t*>(At<uint8_t*>(modelInfoPtrs[self->modelIndex], 0xC), 0x38);
    float points[4][4];
    for (int i = 0; i < 4; i++) {
        const float* start = &At<float>(At<uint8_t*>(col, 4), i * 0x20);
        points[i][0] = start[0];
        points[i][1] = start[1];
        points[i][2] = start[2];
        points[i][3] = 0.0f;   // (the game copies an uninitialised float: overwritten below)
    }
    static const int transformOrder[4] = {0, 2, 1, 3};
    for (int i : transformOrder) {
        Matrix_MultiplyPoint4(points[i], Entity_GetMatrix(self), points[i]);
    }
    float depth[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float rest[4] = {0.0f, 0.0f, 0.0f, 0.0f};   // (uninitialised in the game while a line misses; never read then)
    ignoreEntity = self;
    CColPoint hit[4];
    re::Ptr32<CEntity> hitEntity = nullptr;
    for (int i = 0; i < 4; i++) {
        InitColPoint(hit[i]);
        float s = float(std::fabs(self->moveSpeed.z) * timeStep + 0.10000000149011612);
        CVector along = {s * vec9E33DC.x, s * vec9E33DC.y, s * vec9E33DC.z};
        float zStop = float(points[i][2] - 6.0);
        CVector startPos;
        const CVector* from = VectorAdd(&startPos, reinterpret_cast<const CVector*>(points[i]), &along);
        if (ProcessVerticalLine(from, zStop, &hit[i], &hitEntity, 0x19, 0)) {
            const float* line = &At<float>(At<uint8_t*>(col, 4), i * 0x20);
            rest[i] = float(points[i][2] - (line[2] - line[6]) * 0.75);
            float d = hit[i].point.z - rest[i];
            depth[i] = d < 0.0 ? 0.0f : d;
        }
    }
    ignoreEntity = nullptr;
    if (0.001f < depth[0] || 0.001f < depth[1] || 0.001f < depth[2] || 0.001f < depth[3]) {
        for (int i = 0; i < 4; i++) {
            float nz = hit[i].normal.z;
            if (0.9300000071525574 < nz || 0.707f > nz) {
                hit[i].normal = {vec9E33DC.x, vec9E33DC.y, vec9E33DC.z};
            } else {
                float d = float((1.0 - nz) * 0.3799999952316284 + hit[i].point.z - rest[i]);
                depth[i] = d < 0.0 ? 0.0f : d;
            }
        }
        float along[4], weight[4];
        for (int i = 0; i < 4; i++) {
            along[i] = depth[i] * hit[i].normal.z;
            weight[i] = std::fabs(along[i]);
        }
        float sum = weight[1] + weight[0] + weight[2] + weight[3];
        CVector push[4];
        static const int order[4] = {3, 2, 1, 0};
        for (int i : order) {
            CVector n = {hit[i].normal.x * depth[i], hit[i].normal.y * depth[i], along[i]};
            float k = weight[i] / sum;
            push[i] = {n.x * k, n.y * k, n.z * k};
        }
        CVector a, b, c, d;
        const CVector* p = VectorAdd(&a, Position(self), &push[0]);
        p = VectorAdd(&b, p, &push[1]);
        p = VectorAdd(&c, p, &push[2]);
        p = VectorAdd(&d, p, &push[3]);
        Entity_SetPosition(self, p->x, p->y, p->z);
    }
    for (int i = 3; i >= 0; i--) {
        ReleaseColPoint(hit[i]);
    }
}

// 0x4B7B80: the constructor (CVehicle's first; thiscall, returns this): its damage
// manager (+0x580), its six doors (+0x59C, 0x24 each: open and shut angles, direction,
// axis; by the handling's flags 4, 8 and 0x200), its four wheels' collision points
// (+0x6C4, 0x28 each), effects, handling (+0x2A4 from the model's +0x5E), colours,
// masses, suspension lines, seats
CAutomobile* __fastcall Automobile_Construct(CAutomobile* self, re::Edx, int model, int createdBy) {
    Vehicle_Construct(self, createdBy);
    self->vtable = static_cast<re::Ptr32<void>*>(automobileVtable);
    uint8_t* damage = &At<uint8_t>(self, 0x580);
    DamageManager_Construct(damage);
    At<float>(damage, 0) = 0.5f;
    At<uint8_t>(damage, 0x18) = 1;
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 9; j++) {
            At<int32_t>(self, 0x59C + i * 0x24 + j * 4) = 0;
        }
    }
    // the wheels' collision points: no point, no entity, no normal, the most depth
    for (int i = 0; i < 4; i++) {
        int p = 0x6C4 + i * 0x28;
        At<float>(self, p) = 0.0f;
        At<float>(self, p + 4) = 0.0f;
        At<float>(self, p + 8) = 0.0f;
        At<int32_t>(self, p + 0xC) = 0;
        At<float>(self, p + 0x10) = 0.0f;
        At<float>(self, p + 0x14) = 0.0f;
        At<float>(self, p + 0x18) = 0.0f;
        At<float>(self, p + 0x24) = 3.4028234663852886e+38f;
    }
    for (int i = 0; i < 12; i++) {
        At<float>(self, 0x80C + i * 4) = 0.0f;
    }
    At<float>(self, 0x864) = 0.0f;
    At<float>(self, 0x868) = 0.0f;
    At<float>(self, 0x86C) = 0.0f;
    for (int i = 0; i < 4; i++) {
        Effect_Construct(&self->effects[i * 8]);
    }
    for (int i = 0; i < 8; i++) {
        At<int32_t>(self, 0x8A0 + i * 4) = 0;
    }
    uint8_t* mi = modelInfoPtrs[model];
    int handlingId = At<int16_t>(mi, 0x5E);
    self->vehicleType = 0;
    At<int16_t>(self, 0x7D6) = 0;
    At<int32_t>(self, 0x800) = 0;
    At<int32_t>(self, 0x804) = 0;
    Vehicle_SetModelIndex(self, model, 1);
    Automobile_FindFrames(self);
    Virtual<void>(self, 0x9C, Entity_GetMatrix(self));
    Automobile_ResetEffects(self);
    for (int i = 0; i < 4; i++) {
        self->entities[i] = nullptr;
    }
    self->handling = &handlingData[uint8_t(handlingId) * 0xDC];
    VehicleModel_ChooseVehicleColour(mi, &self->colour1, &self->colour2);
    // the doors: +0 the open angle, +4 the shut one, +8 the direction, +9 the axis
    const float openRear = -1.2566370964050293f;
    const float openFront = 1.2566370964050293f;
    At<uint8_t>(self, 0x5EC) = 0;
    At<uint8_t>(self, 0x5ED) = 2;
    At<float>(self, 0x5E8) = openRear;
    At<float>(self, 0x5E4) = 0.0f;
    At<float>(self, 0x60C) = 0.0f;
    At<uint8_t>(self, 0x610) = 1;
    At<uint8_t>(self, 0x611) = 2;
    At<float>(self, 0x608) = openFront;
    At<uint8_t>(self, 0x634) = 0;
    At<float>(self, 0x630) = openRear;
    At<uint8_t>(self, 0x635) = 2;
    At<float>(self, 0x62C) = 0.0f;
    At<float>(self, 0x654) = 0.0f;
    At<uint8_t>(self, 0x658) = 1;
    At<uint8_t>(self, 0x659) = 2;
    At<float>(self, 0x650) = openFront;
    // the bonnet and the boot
    At<uint8_t>(self, 0x5A4) = 1;
    At<uint8_t>(self, 0x5A5) = 0;
    if (At<uint32_t>(self->handling, 0xCC) & 4) {
        At<float>(self, 0x5A0) = -0.9424778819084167f;
        At<float>(self, 0x59C) = 0.0f;
    } else {
        At<float>(self, 0x5A0) = 0.0f;
        At<float>(self, 0x59C) = 0.9424778819084167f;
    }
    uint32_t flags = At<uint32_t>(self->handling, 0xCC);
    if (flags & 8) {
        At<uint8_t>(self, 0x5C8) = 0;
        At<float>(self, 0x5C4) = openRear;
        At<uint8_t>(self, 0x5C9) = 0;
        At<float>(self, 0x5C0) = 0.0f;
    } else if (flags & 0x200) {
        At<uint8_t>(self, 0x5C8) = 1;
        At<uint8_t>(self, 0x5C9) = 0;
        At<float>(self, 0x5C4) = 0.0f;
        At<float>(self, 0x5C0) = 1.5707963705062866f;
    } else {
        At<uint8_t>(self, 0x5C8) = 1;
        At<float>(self, 0x5C4) = -0.9424778819084167f;
        At<uint8_t>(self, 0x5C9) = 0;
        At<float>(self, 0x5C0) = 0.0f;
    }
    if (At<uint32_t>(self->handling, 0xCC) & 0x10) {
        for (int door = 2; door <= 5; door++) {
            DamageManager_SetDoorStatus(damage, door, 3);
        }
    }
    uint8_t* h = self->handling;
    At<float>(self, 0x158) = At<float>(h, 4);                       // the mass
    At<float>(self, 0x15C) = At<float>(h, 0xC);                     // the turn mass
    self->centreOfMass = At<CVector>(h, 0x1C);
    At<float>(self, 0x160) = At<float>(h, 0x18) * At<float>(h, 0x10) / At<float>(self, 0x158);
    At<float>(self, 0x164) = 0.05f;
    At<float>(self, 0x168) = At<float>(h, 0x2C);
    At<uint8_t>(self, 0x7CC) &= 0xFE;
    self->steerAngle = 0.0f;
    At<float>(self, 0x294) = float(double(self->unknown29C) * 0.0);   // (keeps a NaN)
    At<float>(self, 0x298) = 0.0f;
    At<float>(self, 0x808) = 0.0f;
    float angle = float(0.0 * 3.1415927410125732 / 180.0);
    for (int i = 0; i < 4; i++) {
        self->effectIds[i] = 0xFFFF;
        At<CVector>(self, 0x80C + i * 0xC) = vec0;
        At<float>(self, 0x764 + i * 4) = 1.0f;
        At<float>(self, 0x774 + i * 4) = 1.0f;
        At<int32_t>(self, 0x850 + i * 4) = 0;
        At<float>(self, 0x784 + i * 4) = 0.0f;
        At<float>(self, 0x7BC + i * 4) = 0.0f;
        At<float>(self, 0x79C + i * 4) = angle;
        At<uint8_t>(self, 0x794 + i) = 0;
    }
    At<float>(self, 0x7F8) = 0.0f;
    At<uint8_t>(self, 0x84E) = 0;
    At<uint8_t>(self, 0x84F) = 0;
    At<float>(self, 0x7FC) = 1.0f;
    Entity_GetMatrix(self);
    uint8_t* colModel = At<uint8_t*>(mi, 0xC);
    uint8_t* data = At<uint8_t*>(colModel, 0x38);
    if (!data) {
        void* mem = MemoryMgr_Malloc(0x24);
        data = mem ? static_cast<uint8_t*>(CollisionData_Construct(mem)) : nullptr;
        At<uint8_t*>(colModel, 0x38) = data;
    }
    if (At<void*>(data, 4) == nullptr) {
        At<uint8_t>(data, 0x20) = 4;
        At<void*>(data, 4) = MemoryMgr_Malloc2(0x80, 0);   // (four suspension lines)
    }
    Automobile_SetupSuspensionLines(self);
    if (At<uint8_t>(data, 0x1E) == 0) {
        Vehicle_SetStatus(self, 2);
    }
    At<int32_t>(self, 0x3C) = 1;
    At<uint8_t>(self, 0x84C) = 0;
    At<float>(self, 0x7D0) = 0.0f;
    At<uint8_t>(self, 0x84D) = 0;
    At<int32_t>(self, 0x860) = 0;
    At<uint8_t>(self, 0x7D4) = 0;
    Seats_Init(Seats(self), self);
    return self;
}

// 0x4B8110: the peds in the sectors it's in that it runs into (moved by `matrix` minus
// where it is, when given): those it hits (0x483690) get an event (0x1E) and it waits
// (+0x2DB, +0x2DC 3 s); it waits too for a ped 0xD walking into it in front. Classes 6
// and 7 barely moving skip it all (0)
uint8_t __fastcall Automobile_ProcessPedCollisions(CAutomobile* self, re::Edx, const float* matrix) {
    int32_t cls;
    if (*Vehicle_GetVehicleClass(self, &cls) == 6 || *Vehicle_GetVehicleClass(self, &cls) == 7) {
        const CVector& v = self->moveSpeed;
        float speed = float(std::sqrt(double(std::fabs(v.x * v.x + v.y * v.y))));
        if (speed < 0.003f) {
            return 0;
        }
    }
    if (currentScanCode >= 0xFFFF) {
        ClearScanCodes();
        currentScanCode = 1;
    } else {
        currentScanCode++;
    }
    flagA35739 = 0;
    CVector move = vec0;
    if (matrix) {
        const float* m = Entity_GetMatrix(self);
        move = {m[12] - matrix[12], m[13] - matrix[13], m[14] - matrix[14]};
    }
    uint16_t scanCode = currentScanCode;
    const CVector& v = self->moveSpeed;
    float speed = float(std::sqrt(double(std::fabs(v.x * v.x + v.y * v.y + v.z * v.z))));
    float step = timeStep;
    float radius = Entity_GetBoundRadius(self) + step * speed;
    CVector centre;
    Entity_GetBoundCentre(self, &centre);
    uint8_t* colModel = At<uint8_t*>(modelInfoPtrs[self->modelIndex], 0xC);
    bool hitOne = false;
    float length = float(std::sqrt(double(std::fabs(move.x * move.x + move.y * move.y + move.z * move.z))));
    float reach = At<float>(colModel, 0xC) + length;   // (the bounding sphere's radius)
    if (0.0f < length) {
        CVector dir = move;
        Vector_Normalise(&dir);
        move = {dir.x * reach, dir.y * reach, dir.z * reach};
    }
    for (uint8_t* node = static_cast<uint8_t*>(self->collisionList); node; node = At<uint8_t*>(node, 0x10)) {
        uint32_t* link = At<uint32_t*>(At<uint8_t*>(node, 8), 0xC);
        while (link) {
            uint32_t packed = *link;
            uint8_t* entity = reinterpret_cast<uint8_t*>(GetEntityFromNode(packed & 0xF, (packed >> 4) & 0x3FFF));
            link = (packed >> 0x12) == 0x3FFF ? nullptr : &ptrNodes[packed >> 0x12];
            if (At<uint16_t>(entity, 0x10A) == scanCode) {
                continue;
            }
            At<uint16_t>(entity, 0x10A) = scanCode;
            // (only peds: anything else reads through nullptr, as the game's code does)
            uint8_t* ped = (At<uint8_t>(entity, 0x108) & 7) == 3 ? entity : nullptr;
            if (At<int16_t>(ped, 0x10E) == modelBF42DC || At<uint8_t>(ped, 0x1C98) || At<void*>(ped, 0x1554) ||
                Ped_4763C0(ped, self) || At<int32_t>(ped, 0x20) == 0 ||
                (At<int32_t>(ped, 0x1C) == 0 && At<int32_t>(ped, 0x60) == 0)) {
                continue;
            }
            if (!(At<uint8_t>(modelInfoPtrs[At<int16_t>(ped, 0x10E)], 0xB) & uint8_t(1u << (chapter & 0x1F))) ||
                At<int32_t>(ped, 0xF4) != 0) {
                continue;
            }
            if (!Entity_GetIsTouchingMoving(ped, &centre, radius) || Game_PlaceEntityOnTop(self, ped)) {
                continue;
            }
            if (Ped_ProcessVehicleHit(ped, self, colModel, Entity_GetMatrix(self), &move, reach)) {
                hitOne = true;
                Automobile_4AA160(self, ped);
                uint8_t event[0x30];
                Events_Add(eventsCF4378, event, ped, 0, 0, 0x1E, 0, 0, 0, 0, -1, 0);
                continue;
            }
            if (At<int32_t>(ped, 0x1310) != 0xD) {
                continue;
            }
            CVector tmp;
            if (!(DotProduct(&At<CVector>(ped, 0x124), Entity_GetForward(self, &tmp)) < 0.0f)) {
                continue;
            }
            const CVector* pedPos = Position(ped);
            CVector tmp2, tmp3;
            const CVector* forward = Entity_GetForward(self, &tmp2);
            const CVector* offset = VectorSub(&tmp3, pedPos, Placeable_GetPosition(self));
            if (DotProduct(offset, forward) > 0.0f) {
                self->unknown2DB = 1;
                At<uint32_t>(self, 0x2DC) = timeInMs + 0xBB8;
            }
        }
    }
    if (!hitOne) {
        return 0;
    }
    self->unknown2DB = 1;
    At<uint32_t>(self, 0x2DC) = timeInMs + 0xBB8;
    return 1;
}

// vf41 0x4B63C0: its collision with an entity while moving to `matrix` (GTA's
// ProcessEntityCollision): the wheels' lines first (a vehicle or object under a wheel
// kept with where it touches), then the swept test; on a hit both take damage by the
// speed (x1000), the collision is applied and it's put back out: along the normal (the
// first hit against something level, kept from sinking by 0.005), else by the hit's
// depth less 0.1 a hit so far (`keepHeight`: only its height moved back). Hitting a
// building or a static object throws its occupants about (-6). 1: hit
uint8_t __fastcall Automobile_ProcessEntityCollision(CAutomobile* self, re::Edx, const float* matrix, uint8_t* entity,
                                                       CColPoint* colPoint, int keepHeight) {
    if ((self->typeAndStatus & 0xF8) != 0x10) {
        self->flags33B |= 2;
    }
    uint8_t type = At<uint8_t>(entity, 0x108) & 7;
    if (type == 7) {
        return 0;
    }
    if (type == 3 && (At<void*>(entity, 0x1554) || Ped_4763C0(entity, self))) {
        return 0;
    }
    uint8_t* colModel = (self->flags33A & 0x80) ? colModelC1AEAC : static_cast<uint8_t*>(At<uint8_t*>(modelInfoPtrs[self->modelIndex], 0xC));
    if (!At<void*>(colModel, 0x38)) {
        return 0;
    }
    uint8_t result = 0;
    veh::CMatrix entityMatrix;
    Matrix_Copy(&entityMatrix, Placeable_GetMatrix(entity));
    if (flagA35739 && (At<uint8_t>(entity, 0x108) & 7) != 3) {
        float before[4];
        for (int i = 0; i < 4; i++) {
            before[i] = At<float>(self, 0x764 + i * 4);
        }
        void* entityCol = GetEntityColModel(entity);
        Col_ProcessWheels(Entity_GetMatrix(self), colModel, &entityMatrix, entityCol, &At<uint8_t>(self, 0x6C4),
                          &At<float>(self, 0x764));
        if (!(At<uint8_t>(self, 0x1BD) & 4) && !(At<uint8_t>(self, 0x1BC) & 0x80)) {
            for (int i = 0; i < 4; i++) {
                float now = At<float>(self, 0x764 + i * 4);
                if (!(1.0f > now) || !(before[i] > now)) {
                    continue;
                }
                uint8_t kind = At<uint8_t>(entity, 0x108) & 7;
                if (kind == 2 || kind == 4) {
                    self->entities[i] = reinterpret_cast<CEntity*>(entity);
                    RegisterReference(entity, &self->entities[i]);
                    CVector tmp;
                    const CVector* d = VectorSub(&tmp, &At<CVector>(self, 0x6C4 + i * 0x28), Placeable_GetPosition(entity));
                    At<CVector>(self, 0x80C + i * 0xC) = *d;
                }
                At<uint8_t>(self, 0x1BE) = At<uint8_t>(self, 0x6E6 + i * 0x28);   // (the surface under it)
            }
        }
    }
    int sirenFlag = ((self->typeAndStatus & 0xF8) == 0x18 && self->modelIndex == modelSiren2) ? 0 : 1;
    void* entityCol = GetEntityColModel(entity);
    if (!Col_ProcessSweep(reinterpret_cast<veh::CMatrix*>(const_cast<float*>(matrix)),
                          reinterpret_cast<veh::CMatrix*>(Entity_GetMatrix(self)), colModel, &entityMatrix, &entityMatrix,
                          entityCol, colPoint, 0, 1, 0, 0, sirenFlag, 0xF, 0, 1)) {
        return result;
    }
    const CVector& v = self->moveSpeed;
    float impulse = float((v.y * v.y + v.x * v.x + v.z * v.z) * 1000.0);
    const CVector* normal = reinterpret_cast<const CVector*>(&colPoint->normal);
    if ((At<uint8_t>(entity, 0x108) & 7) == 2) {
        float other = float(Vector_MagnitudeSqr(&At<CVector>(entity, 0x124)) * 1000.0 + impulse);
        Automobile_SetDamage(entity, reinterpret_cast<uint8_t*>(colPoint)[0x23], other, self, normal);
    }
    Automobile_SetDamage(self, reinterpret_cast<uint8_t*>(colPoint)[0x21], impulse, entity, normal);
    bool hitBefore = At<int32_t>(self, 0x1C0) > 0;
    result = 1;
    Physical_ApplyCollision(self, entity, colPoint, 0.0f);
    CVector tmp, tmp2;
    if (!re::Flag(keepHeight)) {
        At<int32_t>(self, 0x1C0)++;
        CVector target = {matrix[12], matrix[13], matrix[14]};
        float nz = colPoint->normal.z;
        if (!hitBefore && nz < 0.9999989867210388 && nz > -0.9999989867210388) {
            // against something level: slid back along the ground and out along the
            // normal's flat part
            CVector back;
            VectorSub(&back, Position(self), &target);
            float left = MaxFloat(0.0f, float(colPoint->depth - 0.004999999888241291));
            CVector part;
            VectorScale(&part, &back, left);
            back = {back.x - part.x, back.y - part.y, back.z - part.z};
            CVector flat = {normal->x, normal->y, 0.0f};
            Vector_Normalise(&flat);
            CVector away = {-back.x, -back.y, -back.z};
            Vector_Scale(&flat, float(Vector_DotProduct(&away, &flat) + 0.05000000074505806));
            const CVector* c = VectorAdd(&tmp, &back, &flat);
            const CVector* e = VectorAdd(&tmp2, &target, &part);
            CVector pos;
            VectorAdd(&pos, e, c);
            Physical_SetPositionV(self, &pos);
            colPoint->depth = MaxFloat(0.0f, float(colPoint->depth - 0.004999999888241291));
        } else {
            CVector back;
            VectorSub(&back, Position(self), &target);
            float k = float(colPoint->depth - float(At<int32_t>(self, 0x1C0)) * 0.10000000149011612);
            if (k < 0.0) {
                k = 0.0f;
            }
            CVector part = {back.x * k, back.y * k, back.z * k};
            CVector pos;
            VectorAdd(&pos, &target, &part);
            Physical_SetPositionV(self, &pos);
            k = float(colPoint->depth - float(At<int32_t>(self, 0x1C0)) * 0.10000000149011612);
            if (k < 0.0) {
                k = 0.0f;
            }
            colPoint->depth = k;
        }
    } else {
        float k = float(colPoint->depth - 0.004999999888241291);
        if (k < 0.0) {
            k = 0.0f;
        }
        colPoint->depth = k;
        float z = matrix[14];
        float up = Position(self)->z - z;
        const_cast<CVector*>(Position(self))->z = z + up * k;
    }
    Physical_46C9D0(self, matrix, colPoint, 0);
    if (self->matrix) {
        Matrix_UpdateRW(self->matrix);
    }
    flagC2D160 = 1;
    uint8_t kind = At<uint8_t>(entity, 0x108) & 7;
    if (kind == 1 || (kind == 4 && (At<uint8_t>(entity, 0x1BC) & 4))) {
        At<int32_t>(self, 0x7C) = 1;
        for (uint32_t seat = 0; seat < 4; seat++) {
            if (!Seats_IsOccupied(Seats(self), seat)) {
                continue;
            }
            CVector point = {colPoint->point.x, colPoint->point.y, colPoint->point.z};
            CVector n = {colPoint->normal.x, colPoint->normal.y, colPoint->normal.z};
            CPed* ped = Seats_GetOccupant(Seats(self), seat);
            // (CPed's vf15 CPed_ApplyHit: the attacker and the node source pointers, full width)
            using Hit = void(__thiscall*)(CPed*, float, float, uintptr_t, int, CVector*, CVector*, uintptr_t, int, int, int, float,
                                          float, int, int, int, int, float, int);
            Hit hit = re::PtrCast<Hit>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(ped))[0x3C / 4]);
            hit(ped, 0.0f, 0.0f, 0, -6, &point, &n, 0, 0, 0, 0, 0.0f, 0.0f, 0, 1, 0, -1, 0.0f, 0);
        }
    }
    return result;
}

// vf13 0x4B85F0: its update in the world (GTA's ProcessControl's collision part; the
// argument unused): parked (status 4) it stops; on rails (2) it follows them and is put
// back when it runs into someone; else vf37 and vf38 move it, it's kept where its
// wheels that were on the ground stood (a class 6 mower pivots on its middle), pushed
// out of the ground, its collisions processed (0x4CDC30, 0x4CCD30; a go-kart's and a
// mower's sparks and sounds), its speed from how far it went, and put back when it hit
// a ped or a vehicle
void __fastcall Automobile_ProcessCollisions(CAutomobile* self, re::Edx, int) {
    self->autoPilotFlags &= 0xFC;
    At<float>(self, 0x19C) = 0.0f;
    re::Ptr32<CEntity>* ref450 = &self->unknown450;
    if (*ref450 && int8_t(self->unknown454) > 0) {
        self->unknown454--;
    }
    if (At<int32_t>(self, 0xB0) != 0 || self->entityFlags == 0) {
        return;
    }
    veh::CMatrix before;
    Matrix_Copy(&before, Entity_GetMatrix(self));
    uint8_t status = self->typeAndStatus >> 3;
    if (status == 2) {
        At<int32_t>(self, 0x34) = 0;
        At<int32_t>(self, 0x38) = 1;
        if (self->mission == 0x16 || self->mission == 0x17) {
            UpdateCarOnRailsFollowingPoints(self);
        } else {
            UpdateCarOnRails(self);
        }
        Physical_RemoveAndAdd(self);
        if (Game_ProcessPedCollisions(self, &before) || Game_CheckVehicleCollision(self, nullptr, &before)) {
            Placeable_SetMatrix(self, &before, 0);
            self->moveSpeed = vec0;
            PathFollower_SetSpeed(&self->autoPilot, 0.0f);
            Physical_RemoveAndAdd(self);
        }
        return;
    }
    if (status == 4) {
        self->moveSpeed = vec0;
        self->turnSpeed = vec0;
        self->moveSpeed = vec0;
        PathFollower_SetSpeed(&self->autoPilot, 0.0f);
        return;
    }
    int32_t cls;
    uint8_t pedalling = 0;
    CPed* driver = Seats_GetOccupant(Seats(self), 0);
    if (driver && At<int32_t>(Seats_GetOccupant(Seats(self), 0), 0x1310) == 0xD &&
        (*Vehicle_GetVehicleClass(self, &cls) == 7 || *Vehicle_GetVehicleClass(self, &cls) == 6)) {
        pedalling = 1;
        Obj_4E9DF0(objC3B808, 1);
    }
    uint8_t* data = At<uint8_t*>(At<uint8_t*>(modelInfoPtrs[self->modelIndex], 0xC), 0x38);
    veh::CMatrix beforeMove;
    Matrix_Copy(&beforeMove, Entity_GetMatrix(self));
    Virtual<void>(self, 0x94);
    float scale = 1.15f;   // (a mower pivots on its middle)
    if (*Vehicle_GetVehicleClass(self, &cls) == 6) {
        scale = 0.0f;
    }
    CVector offset = {0.0f, 0.0f, 0.0f};
    veh::CMatrix translate, moved;
    translate.attached = nullptr;
    moved.attached = nullptr;
    const float* lines = At<float*>(data, 4);   // (four lines, 8 floats each: the start first)
    CVector frontBefore = LinesMidpoint(self, lines + 0, lines + 16, scale, &translate, &moved);
    CVector rearBefore = LinesMidpoint(self, lines + 8, lines + 24, scale, &translate, &moved);
    Virtual<void>(self, 0x98);
    if (self->matrix) {
        Matrix_UpdateRW(self->matrix);
    }
    CVector frontAfter = LinesMidpoint(self, lines + 0, lines + 16, scale, &translate, &moved);
    CVector rearAfter = LinesMidpoint(self, lines + 8, lines + 24, scale, &translate, &moved);
    // (+0x774.. the wheels' compressions, +0x850.. their states: 0 or 3 is gripping)
    auto grips = [&](int a, int b) {
        int32_t sa = At<int32_t>(self, 0x850 + (a - 0x774));
        int32_t sb = At<int32_t>(self, 0x850 + (b - 0x774));
        return 1.0f > At<float>(self, a) && 1.0f > At<float>(self, b) && (sa == 0 || sa == 3) && (sb == 0 || sb == 3);
    };
    bool frontDown = grips(0x774, 0x77C);
    bool rearDown = grips(0x778, 0x780);
    bool useFront;
    if (rearDown) {
        useFront = frontDown && (0.0f < At<float>(self, 0x470) || At<float>(self, 0x298) > 0.10000000149011612) &&
                   At<float>(self, 0x298) > 0.10000000149011612 && Vector_MagnitudeSqr(&self->moveSpeed) > 0.01f;
    } else {
        useFront = true;
    }
    CVector tmp, tmp2;
    if (rearDown && !useFront) {
        const CVector* pos = Position(self);
        VectorSub(&tmp, &rearBefore, &rearAfter);
        Physical_SetPositionV(self, VectorAdd(&tmp2, pos, &tmp));
        offset = *VectorSub(&tmp, &rearAfter, Position(self));
    } else if (frontDown) {
        const CVector* pos = Position(self);
        VectorSub(&tmp, &frontBefore, &frontAfter);
        Physical_SetPositionV(self, VectorAdd(&tmp2, pos, &tmp));
        offset = *VectorSub(&tmp, &frontAfter, Position(self));
    } else {
        offset = vec0;
    }
    Game_PushOutOfGround(self);
    float radius = 0.0f;
    if (*Vehicle_GetVehicleClass(self, &cls) != 7) {
        radius = 0.2f;
    } else if (Physical_GetUp(self, &tmp)->z < 0.20000000298023224) {
        radius = 0.1f;
    }
    float arg3 = 0.0f;
    CColPoint hit;
    InitColPoint(hit);
    flagA35739 = 0;
    flagC2D160 = 0;
    int kind = *Vehicle_GetVehicleClass(self, &cls) == 0xA ? 7 : 0;
    int result = Vehicle_4CDC30(self, &beforeMove, radius, &arg3, &hit, kind, 0, &offset);
    if (*Vehicle_GetVehicleClass(self, &cls) == 0xA) {
        self->turnSpeed.y = 0.0f;
        self->turnSpeed.x = 0.0f;
        if (result == 1) {
            alignas(4) uint8_t sound[8];
            // (only the id's low 16 bits written: the rest are the radius float's)
            uint32_t radiusBits;
            static_assert(sizeof(radiusBits) == sizeof(radius));
            std::memcpy(&radiusBits, &radius, 4);
            uint32_t id = (radiusBits & 0xFFFF0000u) | soundGoKartHit;
            Sound_Construct(sound);
            SoundManager_Play(soundManager, &id, sound, 0);
            Sound_SetPosition(sound, reinterpret_cast<const CVector*>(&hit.point), 0);
            Sound_Destruct(sound);
        }
    }
    if (*Vehicle_GetVehicleClass(self, &cls) == 6) {
        self->turnSpeed.y = 0.0f;
        self->turnSpeed.x = 0.0f;
        if (result == 1) {
            uint32_t id = SoundBank_GetId(soundBank, "MowerSpark");
            alignas(4) uint8_t sound[8];
            Sound_Construct(sound);
            SoundManager_Play(soundManager, &id, sound, 0);
            Sound_SetPosition(sound, reinterpret_cast<const CVector*>(&hit.point), 0);
            Sound_SetVolume(sound, 0.01f);
            self->unknown458 = {hit.point.x, hit.point.y, hit.point.z};
            veh::CMatrix m;
            Matrix_Copy(&m, Placeable_GetMatrix(self));
            veh::CMatrix inverse, inverted;
            inverse.attached = nullptr;
            Matrix_CopyRotPos(&inverse, Matrix_Invert(&inverted, &m));
            CVector point = {hit.point.x, hit.point.y, hit.point.z};
            CVector local = self->unknown458;
            TransformVector2(&local, &inverse, &point);
            self->unknown458 = local;   // (the spark where it hit, in the mower's space)
            self->unknown464 = uint8_t(result);
            Sound_Destruct(sound);
        } else {
            self->unknown464 = 0;
        }
    }
    flagA35739 = 1;
    Vehicle_4CCD30(self, &beforeMove, &arg3, Seats_GetOccupant(Seats(self), 0) != nullptr, &hit, kind, 0);
    Entity_443AA0(self);
    CVector oldPos = {before.m[12], before.m[13], before.m[14]};
    At<int32_t>(self, 0x34) = 0;
    At<int32_t>(self, 0x38) = 1;
    At<float>(self, 0x19C) = Vector_Magnitude(VectorSub(&tmp, Position(self), &oldPos));
    const CVector* now = Position(self);
    CVector flat = {now->x, now->y, 0.0f};
    if (Seats_GetOccupant(Seats(self), 0) && (result == 1 || result == 2)) {
        uint8_t* other = re::PtrCast<uint8_t*>(hit.entity);
        bool keep = false;
        if (other && Virtual<uint8_t>(other, 0x7C)) {
            double otherValue = Virtual<float>(other, 0x80);
            keep = Virtual<float>(self, 0xE4) > otherValue;
        }
        if (!keep) {
            // its speed across the ground from how far it went (z: from 0)
            Vector_SubAssign(&flat, reinterpret_cast<const CVector*>(&before.m[12]));
            Vector_Scale(&flat, 1.0f / timeStep);
            self->moveSpeed.x = flat.x;
            self->moveSpeed.y = flat.y;
        }
    }
    Physical_RemoveAndAdd(self);
    bool putBack;
    if ((self->typeAndStatus & 0xF8) == 0x18 && int8_t(At<uint8_t>(data, 0x1F)) > 0 &&
        *Vehicle_GetVehicleClass(self, &cls) != 7 && *Vehicle_GetVehicleClass(self, &cls) != 0xA &&
        *Vehicle_GetVehicleClass(self, &cls) != 6) {
        putBack = Game_ProcessPedCollisions(self, &before) || Game_CheckVehicleCollision(self, &hit, nullptr);
    } else {
        putBack = Game_ProcessPedCollisions(self, &before) && *Vehicle_GetVehicleClass(self, &cls) == 6;
    }
    if (putBack) {
        Placeable_SetMatrix(self, &before, 0);
        Physical_SetMoveSpeed(self, &vec0);
        PathFollower_SetSpeed(&self->autoPilot, 0.0f);
        Physical_RemoveAndAdd(self);
    }
    if (int8_t(self->unknown454) <= 0) {
        SetRef(ref450, nullptr);
    }
    if (pedalling) {
        Obj_4E9DF0(objC3B808, 0);
    }
    ReleaseColPoint(hit);
}

} // namespace

// ---- registration ---------------------------------------------------------------------

#define AUTOB_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)
AUTOB_REVERSED("CAutomobile::CheckVehicleCollision", 0x004B6B00, &Automobile_CheckVehicleCollision);
AUTOB_REVERSED("CAutomobile::PlaceEntityOnTop", 0x004B6C50, &Automobile_PlaceEntityOnTop);
AUTOB_REVERSED("CAutomobile::PushOutOfGround", 0x004B6F80, &Automobile_PushOutOfGround);
AUTOB_REVERSED("CAutomobile::CAutomobile", 0x004B7B80, &Automobile_Construct);
AUTOB_REVERSED("CAutomobile::ProcessPedCollisions", 0x004B8110, &Automobile_ProcessPedCollisions);
AUTOB_REVERSED("CAutomobile::ProcessEntityCollision", 0x004B63C0, &Automobile_ProcessEntityCollision);
AUTOB_REVERSED("CAutomobile::ProcessCollisions", 0x004B85F0, &Automobile_ProcessCollisions);
