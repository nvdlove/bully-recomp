// CCarCtrl: the random traffic and the AI cars' steering (GTA's CCarCtrl; Bully's
// takes the same shape: the weaving through traffic, the physics steering towards a
// point, the random cars made near the player). 0x420630..0x424280.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Entity.h"
#include "game/Pools.h"
#include "game/Vehicle.h"
#include "game/World.h"
#include "reversed.h"

#include "game/Crt.h"
#define CARCTRL_REVERSED(name, address, fn)                                                      static void* RE_CONCAT(Original_, __LINE__);                                                 REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace {
GAME_VAR(uint32_t, cc_frameCounter, 0x00C1A99C);    // CTimer's frame counter
GAME_VAR(float, cc_timeStep, 0x00C1A9A4);           // CTimer's time step
GAME_VAR(float, cc_slowDownRate, 0x009E4F84);       // (3: the traffic's braking per time step)
GAME_VAR(float, cc_twoPi, 0x009E4F8C);              // (data, not a constant: 2 pi)
GAME_VAR(float, cc_halfPi, 0x009E4F88);             // (pi / 2)
GAME_VAR(void*, cc_camera, 0x00C3CCF4);             // the Gamebryo camera
GAME_VAR(uint8_t, cc_chapter, 0x00A147E0);          // the current chapter (a model's +0xB bits)
GAME_ARRAY(uint8_t*, cc_modelInfoPtrs, 0x00C67738); // CModelInfo::ms_modelInfoPtrs

GAME_FN(uint8_t, __thiscall, CC_GetIsOnScreen, 0x00467460, void* entity);
GAME_FN(uint8_t, __thiscall, CC_CanBeRemoved, 0x004CB7F0, CVehicle* vehicle);
GAME_FN(void*, __cdecl, CC_CameraObject, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, CC_WorldTransform, 0x006C9A90, void* object);  // (+0x24: the translation)
GAME_FN(veh::CVector*, __cdecl, CC_Sub, 0x00414BA0, veh::CVector* out, const veh::CVector* a, const veh::CVector* b);
GAME_FN(veh::CVector*, __thiscall, CC_GetBoundCentre, 0x00466C70, void* entity, veh::CVector* out);
GAME_FN(void, __cdecl, CC_SlowCarDownForOtherCar, 0x0041F0C0, void* other, CVehicle* vehicle, float* speed, float curSpeed);
GAME_FN(void, __cdecl, CC_ClearScanCodes, 0x0045D840);
GAME_FN(void, __cdecl, CC_WeaveThroughCarsSectorList, 0x0041F400, CPtrList* list, CVehicle* vehicle, void* target, float x1, float y1, float x2, float y2, float* left, float* right);
GAME_FN(void, __cdecl, CC_WeaveThroughPedsSectorList, 0x0041F5A0, CPtrList* list, CVehicle* vehicle, void* target, float x1, float y1, float x2, float y2, float* left, float* right);
GAME_FN(void, __cdecl, CC_WeaveThroughObjectsSectorList, 0x0041F740, CPtrList* list, CVehicle* vehicle, float x1, float y1, float x2, float y2, float* left, float* right);
GAME_FN(float, __cdecl, CC_FindAngleToWeaveThroughTraffic, 0x004208B0, CVehicle* vehicle, void* target, float angleToTarget, float angleForward);
GAME_FN(veh::CVector*, __thiscall, CC_GetForward, 0x004154A0, CVehicle* vehicle, veh::CVector* out);
GAME_FN(float, __cdecl, CC_GetATanOfXY, 0x00432FE0, float x, float y);
GAME_FN(float, __cdecl, CC_SpeedFactorForSteer, 0x0041A600, float angle, float a, float b, float c);
// (GTA's CTrafficLights::ShouldCarStopForLight, on 0xC1A7DC here, and CCarAI::CarHasReasonToStop)
GAME_FN(uint8_t, __thiscall, CC_ShouldCarStop, 0x0045B060, void* lights, CVehicle* vehicle);
GAME_FN(void, __cdecl, CC_CarHasReasonToStop, 0x0041A150, CVehicle* vehicle);
GAME_FN(void, __cdecl, CC_SlowCarDownForCarsSectorList, 0x00420740, CPtrList* list, CVehicle* vehicle, float x1, float y1, float x2, float y2, float* speed, float curSpeed);
GAME_FN(void, __cdecl, CC_SlowCarDownForPedsSectorList, 0x0041C5F0, CPtrList* list, CVehicle* vehicle, float x1, float y1, float x2, float y2, float* speed, float curSpeed);
GAME_FN(float, __cdecl, CC_FindMaximumSpeedForThisCarInTraffic, 0x004219E0, CVehicle* vehicle);
GAME_FN(void, __thiscall, CC_AutoPilot_ModifySpeed, 0x00417C50, uint8_t* autoPilot, float speed);
// the autopilot's path: the current and next node's coordinates and directions
GAME_FN(veh::CVector*, __thiscall, CC_AutoPilot_GetCurNodeCoors, 0x00418010, uint8_t* autoPilot, veh::CVector* out);
GAME_FN(veh::CVector*, __thiscall, CC_AutoPilot_GetNextNodeCoors, 0x00418080, uint8_t* autoPilot, veh::CVector* out);
GAME_FN(void, __thiscall, CC_AutoPilot_GetCurDirection, 0x004180F0, uint8_t* autoPilot, float* x, float* y);
GAME_FN(void, __thiscall, CC_AutoPilot_GetNextDirection, 0x00418130, uint8_t* autoPilot, float* x, float* y);
GAME_FN(int16_t*, __thiscall, CC_AutoPilot_GetTargetNode, 0x00417920, uint8_t* autoPilot);  // (+8: x, y, z in eighths)
GAME_FN(uint8_t, __cdecl, CC_PickNextNodeAccordingStrategy, 0x0041F8D0, CVehicle* vehicle);
GAME_FN(float, __cdecl, CC_LimitRadianAngle, 0x00432EE0, float angle);
GAME_FN(veh::CVector*, __thiscall, CC_AutoPilot_GetPoint, 0x00417A30, uint8_t* autoPilot, veh::CVector* out);
GAME_FN(uint8_t, __thiscall, CC_AutoPilot_NextPoint, 0x00417980, uint8_t* autoPilot);
GAME_FN(void, __thiscall, CC_AutoPilot_Fn418170, 0x00418170, uint8_t* autoPilot, int32_t arg, int flag, int arg3);
// GenerateRandomCars'
GAME_VAR(uint8_t, cc_cutsceneRunning, 0x020C5BE4);
GAME_VAR(uint8_t*, cc_popLimits, 0x020C8914);        // (+0x18: the most random cars)
GAME_VAR(uint8_t, cc_flagC2D1A9, 0x00C2D1A9);
GAME_VAR(int32_t, cc_carToUpdate, 0x00BCD094);
GAME_FN(int, __cdecl, CC_CountRandomCars, 0x0041C320);
GAME_FN(void, __thiscall, CC_GenerateOneRandomCar, 0x00421370, uint8_t* self);
GAME_FN(void, __thiscall, CC_UpdateCarPath, 0x0041EC70, uint8_t* self, CVehicle* vehicle);
GAME_FN(void, __cdecl, CC_RemoveCarsIfThePoolGetsFull, 0x00420630);
// UpdateCarOnRails'
GAME_FN(void, __cdecl, CC_SlowCarOnRailsDownForTrafficAndLights, 0x00421D20, CVehicle* vehicle);
GAME_FN(void, __thiscall, CC_SetMoveSpeed, 0x0041A440, CVehicle* vehicle, const veh::CVector* speed);
GAME_FN(void, __thiscall, CC_VectorDiv, 0x0041A4A0, veh::CVector* v, float d);
GAME_FN(void, __cdecl, CC_CalcCurvePoint, 0x00428E70, const veh::CVector* start, const veh::CVector* end, const veh::CVector* startDir, const veh::CVector* endDir, float between, uint32_t timeOnCurve, veh::CVector* outPos, veh::CVector* outSpeed);
GAME_FN(void, __cdecl, CC_FindCarInTheWay, 0x0041FA20, CVehicle* vehicle, veh::CVector* pos, re::Ptr32<CEntity>* found);
GAME_ARRAY(int32_t, cc_modelsStopFor, 0x00BF3E88);   // [4] the models a car on rails waits behind
GAME_FN(veh::CVector*, __thiscall, CC_AutoPilot_GetPrevPoint, 0x004179C0, uint8_t* autoPilot, veh::CVector* out);
GAME_FN(void, __thiscall, CC_AutoPilot_GetPrevPointDirection, 0x00417AA0, uint8_t* autoPilot, float* x, float* y);
GAME_FN(void, __thiscall, CC_AutoPilot_GetPointDirection, 0x00417B10, uint8_t* autoPilot, float* x, float* y);
// SteerAICarWithPhysics'
GAME_FN(veh::CVector*, __cdecl, CC_FindPlayerCoors, 0x00446900, veh::CVector* out);
GAME_FN(CVehicle*, __cdecl, CC_FindPlayerVehicle, 0x00446970, uintptr_t player);
GAME_FN(veh::CVector*, __cdecl, CC_FindPlayerSpeed, 0x00445D60);
GAME_FN(veh::CVector*, __thiscall, CC_GetRight, 0x008B6120, void* entity);
GAME_FN(float, __cdecl, CC_DotProduct, 0x00414640, const veh::CVector* a, const veh::CVector* b);
GAME_FN(float, __thiscall, CC_Magnitude, 0x0040DFB0, const veh::CVector* v);
GAME_FN(float, __cdecl, CC_Sqrt, 0x0040DF90, float v);
GAME_FN(veh::CVector*, __cdecl, CC_Scale, 0x00412350, veh::CVector* out, const veh::CVector* v, float s);
GAME_FN(veh::CVector*, __cdecl, CC_Add, 0x004129A0, veh::CVector* out, const veh::CVector* a, const veh::CVector* b);
GAME_FN(void, __cdecl, CC_SteerAICarWithPhysicsTryingToBlockTarget, 0x00421240, CVehicle* vehicle, float x, float y, float speedX, float speedY, float* swerve, float* accel, float* brake, uint8_t* handbrake);
GAME_FN(void, __cdecl, CC_SteerAICarWithPhysicsTryingToBlockTarget_Stop, 0x0041BB60, CVehicle* vehicle, float x, float y, float speedX, float speedY, float* swerve, float* accel, float* brake, uint8_t* handbrake);
GAME_FN(void, __cdecl, CC_SteerAICarWithPhysicsFollowPath, 0x00421E50, CVehicle* vehicle, float* swerve, float* accel, float* brake, uint8_t* handbrake);
GAME_FN(void, __cdecl, CC_SteerAICarWithPhysicsFollowPoints, 0x00422800, CVehicle* vehicle, float* swerve, float* accel, float* brake, uint8_t* handbrake);
GAME_FN(void, __cdecl, CC_SteerAICarWithPhysics_Mission15, 0x0041BBF0, CVehicle* vehicle, float* swerve, float* accel, float* brake, uint8_t* handbrake);
GAME_FN(void*, __thiscall, CC_Paths_GetNode, 0x00417850, void* paths, int32_t index);
GAME_FN(veh::CVector*, __thiscall, CC_Vehicle_Fn4CCC00, 0x004CCC00, CVehicle* vehicle, veh::CVector* out);
GAME_FN(void, __cdecl, CC_SteerAICarWithPhysics_OnlyMission, 0x00423460, CVehicle* vehicle, float* swerve, float* accel, float* brake, uint8_t* handbrake);
GAME_FN(float, __cdecl, CC_CalcSpeedScaleFactor, 0x00428DA0, const veh::CVector* start, const veh::CVector* end, float startDirX, float startDirY, float endDirX, float endDirY);
GAME_FN(void, __cdecl, CC_SteerAICarWithPhysicsHeadingForTarget, 0x00420E20, CVehicle* vehicle, void* target, float x, float y, float* swerve, float* accel, float* brake, uint8_t* handbrake);
inline void* const cc_stuckCarList = reinterpret_cast<void*>(0x00C1A7DC);
// GenerateOneRandomCar's
GAME_VAR(uint8_t, cc_noRandomCars, 0x00C2C10C);
GAME_VAR(uint8_t*, cc_playerPed, 0x00C1AEA8);
GAME_VAR(void*, cc_population, 0x00C2C108);          // the ped population (makes the drivers)
GAME_VAR(uint32_t, cc_gameTimeMs, 0x00C1A9B4);
GAME_VAR(int32_t, cc_modelCop, 0x00BF431C);          // the models with a timer of their own (+0x1D6C)
GAME_VAR(int32_t, cc_modelCop2, 0x00BF4318);         // (+0x1D70)
GAME_VAR(int32_t, cc_modelNever, 0x00BF4454);
GAME_ARRAY(int32_t, cc_modelsDriver2, 0x00BF42F8);   // [5]: 0xBF42F8..0xBF4308, drivers of kind 2
GAME_VAR(uint8_t*, cc_punishment, 0x00C0F78C);
GAME_VAR(veh::CVector, cc_zeroVector, 0x00BCC564);
GAME_VAR(int32_t, cc_currArea, 0x00BD1008);
inline void* const cc_thePaths = reinterpret_cast<void*>(0x00BF6228);
inline void* const cc_theCamera = reinterpret_cast<void*>(0x00C3CC68);
GAME_FN(uint8_t, __thiscall, CC_Pop_CanMakeCars, 0x0049E9E0, void* population);
GAME_FN(uint8_t, __thiscall, CC_IsPointOkForCar, 0x0041FF20, uint8_t* self, veh::CVector* pos);
GAME_FN(void, __thiscall, CC_ChooseGroup, 0x0041C240, uint8_t* self, int32_t* out, int a, int b);
GAME_FN(int32_t, __thiscall, CC_Group_Choose, 0x00464B70, void* group, int32_t model, int32_t model2);
GAME_FN(int32_t, __thiscall, CC_Pop_ChooseCarModel, 0x0049C330, void* population, int32_t choice);
GAME_FN(veh::CVector*, __thiscall, CC_Camera_GetForward, 0x0041AA70, void* camera, veh::CVector* out);
GAME_FN(uint8_t, __thiscall, CC_Paths_NewGenerateCarCreationCoors_x64, 0x00440870, void* paths, float, float, float, float, float, float, float range, float minRange, int flag, veh::CVector* out, re::Ptr32<void>* node1, re::Ptr32<void>* node2, float* positionBetween, uintptr_t a, uintptr_t b);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t CC_Paths_NewGenerateCarCreationCoors(void* a0, veh::CVector a1, veh::CVector a2, float a3, float a4, int a5, veh::CVector* a6, re::Ptr32<void>* a7, re::Ptr32<void>* a8, float* a9, uintptr_t a10, uintptr_t a11) {
    return CC_Paths_NewGenerateCarCreationCoors_x64(a0, a1.x, a1.y, a1.z, a2.x, a2.y, a2.z, a3, a4, a5, a6, a7, a8, a9, a10, a11);
}
GAME_FN(float, __cdecl, CC_LanesChance, 0x00414470, float lanes1, float lanes2);
GAME_FN(uint8_t, __cdecl, CC_IsPointInArea, 0x00570330, float* xy, int32_t* area);
GAME_FN(uint8_t, __cdecl, CC_IsPointInsideSomething_x64, 0x0045EAE0, float, float, float);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t CC_IsPointInsideSomething(veh::CVector a0) {
    return CC_IsPointInsideSomething_x64(a0.x, a0.y, a0.z);
}
GAME_FN(float, __cdecl, CC_FindGroundZFor3DCoord, 0x004635A0, float x, float y, float z, uint8_t* found, int flag);
GAME_FN(float, __cdecl, CC_GetRandomNumberInRange, 0x0041A5B0, float lo, float hi);
GAME_FN(void, __cdecl, CC_FindObjectsInRange, 0x0045E3C0, const veh::CVector* pos, float range, int twoD, int16_t* count, int16_t maxCount, re::Ptr32<void>* out, uint32_t lists);
GAME_FN(void*, __thiscall, CC_Pop_AddBikeDriver, 0x0049FCF0, void* population, veh::CVector* pos, int32_t model);
GAME_FN(int32_t, __thiscall, CC_Pop_ChooseDriverModel, 0x0049C230, void* population, int32_t kind, int32_t a, int32_t b);
GAME_FN(void*, __thiscall, CC_Pop_AddDriver, 0x0049F560, void* population, int32_t kind, int32_t model);
GAME_FN(void*, __cdecl, CC_VehicleNew, 0x004CC490, uintptr_t size);
GAME_FN(CVehicle*, __thiscall, CC_Bike_Construct, 0x004C6F70, void* mem, int32_t model, int createdBy);
GAME_FN(CVehicle*, __thiscall, CC_Automobile_Construct, 0x004B7B80, void* mem, int32_t model, int createdBy);
GAME_FN(void, __cdecl, CC_World_Add, 0x0045D6B0, CVehicle* vehicle);
GAME_FN(void, __thiscall, CC_ChooseVehicleColour, 0x00535060, uint8_t* modelInfo, uint8_t* colour1, uint8_t* colour2);
GAME_FN(int32_t, __thiscall, CC_Node_GetIndex, 0x0043F8A0, void* node);
GAME_FN(void, __thiscall, CC_AutoPilot_SetNextNode, 0x005AB1A0, uint8_t* autoPilot, uintptr_t node);
inline int __cdecl CC_rand() { return crt::Rand(); }
GAME_FN(void, __cdecl, CC_JoinCarWithRoadSystem, 0x0041BF30, CVehicle* vehicle);
GAME_FN(void, __cdecl, CC_PlaceCarBetweenNodes, 0x0041FF70, CVehicle* vehicle, float between, float z, int flag);
GAME_FN(veh::CVector*, __thiscall, CC_Node_GetPosition, 0x00417C00, void* node, veh::CVector* out);
GAME_FN(veh::CVector*, __cdecl, CC_Sub2, 0x00414B70, veh::CVector* out, const veh::CVector* a, const veh::CVector* b);
GAME_FN(float, __thiscall, CC_Normalise, 0x00414E10, veh::CVector* v);
GAME_FN(float, __cdecl, CC_HeadingOf, 0x00433310, const veh::CVector* v);
GAME_FN(void, __thiscall, CC_SetOwnerRef, 0x005D7F10, re::Ptr32<CEntity>* ref, void* ped);
GAME_FN(void, __cdecl, CC_SetObjectVisible, 0x005F2B00, void* rwObject, int visible);
GAME_FN(void, __cdecl, CC_Pop_PutDriverIn, 0x0049A3B0, void* ped, CVehicle* vehicle);

veh::CVector& Pos(CVehicle* v) {
    return v->matrix ? *reinterpret_cast<veh::CVector*>(&v->matrix->m[12]) : v->placementPos;
}
veh::CVector& EntityPos(void* e) {
    return Pos(static_cast<CVehicle*>(e));  // (the CPlaceable layout is shared)
}
// (the game: the magnitude's fabs before the square root, 0x85C500)
float SqrtAbs(float v) {
    return std::sqrt(std::fabs(v));
}
float Magnitude(const veh::CVector& v) {
    float sq = v.x * v.x + v.y * v.y + v.z * v.z;
    return SqrtAbs(sq);
}
// LimitRadianAngle (inlined; the game stores the float every step)
float LimitAngle(float a) {
    while (a < -3.1415927410125732) {
        a += cc_twoPi;
    }
    while (a > 3.1415927410125732) {
        a -= cc_twoPi;
    }
    return a;
}
} // namespace

namespace CCarCtrl {

// 0x420630: every fourth frame (of 8), with the vehicle pool full: the random car
// farthest from the camera, not on screen, deleted. (GTA's RemoveCarsIfThePoolGetsFull
// takes the nearest; Bully the farthest, and deletes it without CWorld::Remove)
void __cdecl RemoveCarsIfThePoolGetsFull() {
    if ((cc_frameCounter & 7) != 3) {
        return;
    }
    CPool<CVehicle>* pool = CPools::ms_pVehiclePool;
    int size = pool->size;
    int used = pool->GetNoOfUsedSpaces();
    if (size != used || size == 0) {
        return;
    }
    float farthest = 0.0f;
    CVehicle* found = nullptr;
    for (int i = size; i != 0;) {
        i--;
        CVehicle* vehicle = pool->GetAt(i);
        if (!vehicle || CC_GetIsOnScreen(vehicle) || !CC_CanBeRemoved(vehicle)) {
            continue;
        }
        veh::CVector* pos = &Pos(vehicle);
        uint8_t* transform = CC_WorldTransform(CC_CameraObject(cc_camera));
        veh::CVector d;
        veh::CVector* diff = CC_Sub(&d, reinterpret_cast<veh::CVector*>(transform + 0x24), pos);
        float distSq = diff->x * diff->x + diff->y * diff->y;
        if (farthest < distSq) {
            farthest = distSq;
            found = vehicle;
        }
    }
    if (found) {
        reinterpret_cast<void(__thiscall*)(CVehicle*, int)>(static_cast<void*>(found->vtable[0]))(found, 1);
    }
}

// 0x420740: the cars of a sector list in the rectangle (by their bounding centres),
// within 5 units of height, slowing the car down (GTA's SlowCarDownForCarsSectorList;
// Bully skips the car itself, those without collision, those not in this chapter
// and those with +0xF4)
void __cdecl SlowCarDownForCarsSectorList(CPtrList* list, CVehicle* vehicle, float x1, float y1, float x2, float y2, float* speed, float curSpeed) {
    CPtrNode* node = list->first;
    if (!node) {
        return;
    }
    float z = Pos(vehicle).z;
    uint16_t scanCode = CWorld::ms_nCurrentScanCode;
    do {
        CEntity* entity = node->GetItem();
        node = node->GetNext();
        if (reinterpret_cast<void*>(entity) == vehicle || entity->scanCode == scanCode || !entity->bUsesCollision) {
            continue;
        }
        if (!entity->flag1C && !entity->flag60) {
            continue;
        }
        if (!(cc_modelInfoPtrs[entity->modelIndex][0xB] & uint8_t(1 << (cc_chapter & 0x1F))) || entity->flagF4) {
            continue;
        }
        entity->scanCode = scanCode;
        veh::CVector centre;
        CC_GetBoundCentre(entity, &centre);
        if (x1 < centre.x && centre.x < x2 && y1 < centre.y && centre.y < y2) {
            float dz = std::fabs(centre.z - z);
            if (dz < 5.0f) {
                CC_SlowCarDownForOtherCar(entity, vehicle, speed, curSpeed);
            }
        }
    } while (node);
}

// 0x4208B0: the heading to steer for, weaving through the cars, peds and objects in
// the sectors ahead (GTA's FindAngleToWeaveThroughTraffic)
float __cdecl FindAngleToWeaveThroughTraffic(CVehicle* vehicle, void* target, float angleToTarget, float angleForward) {
    veh::CVector speed = vehicle->moveSpeed;
    float dist = SqrtAbs(speed.x * speed.x + speed.y * speed.y);
    dist = float(dist / double(0.4f) + 1.0);
    if (2.0f <= dist) {
        dist = 2.0f;
    }
    veh::CVector pos = Pos(vehicle);
    dist = float(dist * 12.0);
    float left = pos.x - dist;
    float right = dist + pos.x;
    float bottom = pos.y - dist;
    float top = pos.y + dist;
    // (the sector indices: truncated, back to float, clamped, truncated again)
    float f = float(int(left / 50.0 + 18.0));
    int xstart = int(f > 0.0f ? f : 0.0f);
    f = float(int(bottom / 50.0 + 18.0));
    int ystart = int(f > 0.0f ? f : 0.0f);
    f = float(int(right / 50.0 + 18.0));
    int xend = int(f < 35.0f ? f : 35.0f);
    f = float(int(top / 50.0 + 18.0));
    int yend = int(f < 35.0f ? f : 35.0f);

    float angleLeft = angleToTarget;
    float angleRight = angleToTarget;
    if (CWorld::ms_nCurrentScanCode >= 0xFFFF) {
        CC_ClearScanCodes();
        CWorld::ms_nCurrentScanCode = 1;
    } else {
        CWorld::ms_nCurrentScanCode++;
    }
    float lastLeft = -9999.9f;
    float lastRight = -9999.9f;
    while (angleLeft != lastLeft || angleRight != lastRight) {
        lastLeft = angleLeft;
        lastRight = angleRight;
        for (int y = ystart; y <= yend; y++) {
            for (int x = xstart; x <= xend; x++) {
                CSector* sector = &CWorld::ms_aSectors[y * 36 + x];
                CPtrList* lists = reinterpret_cast<CPtrList*>(sector);
                CC_WeaveThroughCarsSectorList(&lists[SECTOR_VEHICLES], vehicle, target, left, bottom, right, top, &angleLeft, &angleRight);
                CC_WeaveThroughPedsSectorList(&lists[SECTOR_PEDS], vehicle, target, left, bottom, right, top, &angleLeft, &angleRight);
                CC_WeaveThroughObjectsSectorList(&lists[SECTOR_OBJECTS], vehicle, left, bottom, right, top, &angleLeft, &angleRight);
            }
        }
    }
    float diff = LimitAngle(angleForward - angleToTarget);
    float bisect = LimitAngle(float(diff * 0.5 + angleToTarget));
    float diffLeft = std::fabs(LimitAngle(angleLeft - bisect));
    float diffRight = std::fabs(LimitAngle(angleRight - bisect));
    if (diffLeft > cc_halfPi && diffRight > cc_halfPi) {
        return bisect;
    }
    if (std::fabs(diffLeft - diffRight) < 0.08f) {
        return angleRight;
    }
    return diffLeft < diffRight ? angleLeft : angleRight;
}

// 0x420E20: steering, throttle and brake towards a point (GTA's
// SteerAICarWithPhysicsHeadingForTarget)
void __cdecl SteerAICarWithPhysicsHeadingForTarget(CVehicle* vehicle, void* target, float x, float y, float* swerve, float* accel, float* brake, uint8_t* handbrake) {
    *handbrake = 0;
    veh::CVector tmp;
    float fx = CC_GetForward(vehicle, &tmp)->x;
    float fy = CC_GetForward(vehicle, &tmp)->y;
    float len = SqrtAbs(fx * fx + fy * fy);
    if (len == 0.0f) {
        fx = 1.0f;
    } else {
        fx = fx / len;
        fy = fy / len;
    }
    veh::CVector pos = Pos(vehicle);
    float dy = y - pos.y;
    float dx = x - pos.x;
    float angleToTarget = CC_GetATanOfXY(dx, dy);
    float angleForward = CC_GetATanOfXY(fx, fy);
    if (vehicle->drivingStyle == 2) {
        angleToTarget = CC_FindAngleToWeaveThroughTraffic(vehicle, target, angleToTarget, angleForward);
    }
    float steer = LimitAngle(angleToTarget - angleForward);
    if (Magnitude(vehicle->moveSpeed) > 0.3f && std::fabs(steer) > 0.7f) {
        *handbrake = 1;
    }
    if (steer < -0.5f) {
        steer = -0.5f;
    }
    if (0.5f < steer) {
        steer = 0.5f;
    }
    // (the speed factor from the straight angle to the target, not the weaving one)
    float straight = CC_GetATanOfXY(dx, dy);
    float factor = CC_SpeedFactorForSteer(straight - angleForward, 0.4f, 1.2f, 0.4f);
    int8_t style = int8_t(vehicle->drivingStyle);
    if (style >= 0 && (style <= 1 || style == 5) && CC_ShouldCarStop(cc_stuckCarList, vehicle)) {
        CC_CarHasReasonToStop(vehicle);
        factor = 0.0f;
    }
    veh::CVector speed = vehicle->moveSpeed;
    float speed30 = float(Magnitude(speed) * 30.0);
    float wanted = float(vehicle->cruiseSpeed) * factor;
    float diffSpeed = wanted - speed30;
    *brake = 0.0f;
    if (diffSpeed <= 0.0f) {
        *accel = 0.0f;
        float b = float(-diffSpeed / 20.0);
        if (b > 0.5) {
            b = 0.5f;
        }
        *brake = b;
        *swerve = steer;
        return;
    }
    if (speed30 >= 25.0) {
        *accel = 1.0f;
        *swerve = steer;
        return;
    }
    float a = float(diffSpeed / 10.0);
    if (a > 1.0) {
        a = 1.0f;
    }
    *accel = a;
    *swerve = steer;
}

// 0x421240: heading for where the target will be (its speed, capped at 0.13, over 30
// frames), then the mission's close stage once within 3 units (GTA's
// SteerAICarWithPhysicsTryingToBlockTarget; missions 0x11 -> 0x12, else 6)
void __cdecl SteerAICarWithPhysicsTryingToBlockTarget(CVehicle* vehicle, float x, float y, float speedX, float speedY, float* swerve, float* accel, float* brake, uint8_t* handbrake) {
    float len = SqrtAbs(speedY * speedY + speedX * speedX);
    if (len > 0.13f) {
        double k = 0.13 / len;
        speedX = float(speedX * k);
        speedY = float(k * speedY);
    }
    x = float(x + speedX * 30.0);
    vehicle->drivingStyle = 2;
    y = float(speedY * 30.0 + y);
    CC_SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, x, y, swerve, accel, brake, handbrake);
    veh::CVector pos = Pos(vehicle);
    float dy = y - pos.y;
    float dx = x - pos.x;
    if (dx * dx + dy * dy < 9.0f) {
        vehicle->mission = vehicle->mission == 0x11 ? 0x12 : 6;
    }
}

// 0x421370 (thiscall, on the car control's state): one random car on a path node near
// the player, ahead of the camera: its model and driver from the population, placed
// between the two nodes, on the ground, with nothing in the way. (GTA's
// GenerateOneRandomCar.) Two models (police-like, 0xBF431C and 0xBF4318) wait 20 s
// between each; model 0x111 is never made (its driver is made first, and left)
void __fastcall GenerateOneRandomCar(uint8_t* self, re::Edx) {
    if (cc_noRandomCars) {
        return;
    }
    veh::CVector playerPos = EntityPos(cc_playerPed);
    CPool<CVehicle>* pool = CPools::ms_pVehiclePool;
    if (pool->size - pool->GetNoOfUsedSpaces() <= 0) {
        return;
    }
    if (!CC_Pop_CanMakeCars(cc_population) || !CC_IsPointOkForCar(self, &playerPos)) {
        return;
    }
    uint32_t& copTimer = *reinterpret_cast<uint32_t*>(self + 0x1D6C);
    uint32_t& cop2Timer = *reinterpret_cast<uint32_t*>(self + 0x1D70);
    int32_t model2 = cc_modelCop2;
    int kind = cc_gameTimeMs > copTimer ? 3 : 2;
    int32_t allowed2 = cc_gameTimeMs > cop2Timer ? -1 : model2;
    int32_t group;
    CC_ChooseGroup(self, &group, 2, kind);
    if (group == -1) {
        return;
    }
    int32_t driverKind = 9;
    int32_t driverModel = -1;
    int32_t model = CC_Pop_ChooseCarModel(cc_population, CC_Group_Choose(self + 0x1D00 + group * 0x14, model2, allowed2));
    if ((model == cc_modelCop && cc_gameTimeMs > copTimer) || (model == cc_modelCop2 && cc_gameTimeMs > cop2Timer)) {
        uint8_t* p = *reinterpret_cast<re::Ptr32<uint8_t>*>(cc_punishment + 0x10);
        if (p && *reinterpret_cast<int32_t*>(p + 0x1310) == 0xD) {
            return;
        }
        driverKind = 7;
    }
    if (model == -1 || model == cc_modelNever) {
        return;
    }
    float between = 0.0f;
    veh::CVector pos = cc_zeroVector;
    re::Ptr32<void> node1 = nullptr;
    re::Ptr32<void> node2 = nullptr;
    veh::CVector camTmp;
    veh::CVector* camForward = CC_Camera_GetForward(cc_theCamera, &camTmp);
    veh::CVector dir = {camForward->x, camForward->y, 0.0f};
    if (!CC_Paths_NewGenerateCarCreationCoors(cc_thePaths, playerPos, dir, 65.0f, 0.5f, 1, &pos, &node1, &node2, &between, 0, 0)) {
        return;
    }
    int lanes1 = re::PtrCast<uint8_t*>(node1)[0x16] & 0xF;
    int lanes2 = re::PtrCast<uint8_t*>(node2)[0x16] & 0xF;
    if (int(CC_LanesChance(float(lanes1), float(lanes2))) == 0) {
        return;
    }
    float xy[2] = {pos.x, pos.y};
    int32_t area = cc_currArea;
    if (!CC_IsPointInArea(xy, &area)) {
        return;
    }
    if (CC_IsPointInsideSomething(pos)) {
        return;
    }
    uint8_t found;
    float groundZ = CC_FindGroundZFor3DCoord(pos.x, pos.y, float(pos.z + 2.0), &found, 1);
    if (!found) {
        return;
    }
    uint8_t* colModel = *reinterpret_cast<re::Ptr32<uint8_t>*>(cc_modelInfoPtrs[model] + 0xC);
    float boxZ = *reinterpret_cast<float*>(colModel + 0x18);
    pos.z = float(boxZ * -1.0 + groundZ);
    float boxY = *reinterpret_cast<float*>(colModel + 0xC);
    float speed = *reinterpret_cast<float*>(self + 0x1D68);
    if (!(0.001f < speed)) {
        speed = CC_GetRandomNumberInRange(7.0f, 8.5f);
    }
    int16_t count = 0;
    re::Ptr32<void> inTheWay[8];
    float range = boxY + boxY + speed;
    CC_FindObjectsInRange(&pos, range, 1, &count, 8, inTheWay, 6);
    if (count > 0) {
        return;
    }
    void* driver;
    CVehicle* vehicle;
    if (*reinterpret_cast<int32_t*>(cc_modelInfoPtrs[model] + 0x50) == 1) {
        driver = CC_Pop_AddBikeDriver(cc_population, &pos, model);
        if (!driver) {
            return;
        }
        void* mem = CC_VehicleNew(0x7A4);
        if (!mem) {
            return;
        }
        vehicle = CC_Bike_Construct(mem, model, 1);
    } else {
        int32_t pick = driverModel;
        if (driverKind != 7) {
            int a = 1;
            if (model == cc_modelsDriver2[0] || model == cc_modelsDriver2[4] || model == cc_modelsDriver2[3] || model == cc_modelsDriver2[2] ||
                model == cc_modelsDriver2[1]) {
                a = 2;
            }
            pick = CC_Pop_ChooseDriverModel(cc_population, driverKind, a, 5);
            if (pick == -1) {
                return;
            }
        }
        driver = CC_Pop_AddDriver(cc_population, driverKind, pick);
        if (!driver) {
            return;
        }
        if (driverKind != 7) {
            *reinterpret_cast<int32_t*>(static_cast<uint8_t*>(driver) + 0x1358) = 1;
        }
        if (model == 0x111) {
            return;
        }
        void* mem = CC_VehicleNew(0x8C0);
        if (!mem) {
            return;
        }
        vehicle = CC_Automobile_Construct(mem, model, 1);
    }
    if (!vehicle) {
        return;
    }
    CC_World_Add(vehicle);
    CC_ChooseVehicleColour(cc_modelInfoPtrs[vehicle->modelIndex], &vehicle->colour1, &vehicle->colour2);
    uint8_t* autoPilot = &vehicle->autoPilot;
    *reinterpret_cast<int32_t*>(autoPilot + 8) = 0;
    reinterpret_cast<uint8_t*>(vehicle)[0x2D7] = 0;
    autoPilot[0x30] = 0;
    *reinterpret_cast<int32_t*>(autoPilot) = CC_Node_GetIndex(node1);
    CC_AutoPilot_SetNextNode(autoPilot, CC_Node_GetIndex(node2));
    int8_t cruise = int8_t(int(speed));
    vehicle->cruiseSpeed = cruise;
    vehicle->mission = 1;
    vehicle->unknown2DB = 0;
    vehicle->cruiseSpeedNow = float(int(cruise));
    vehicle->drivingStyle = (CC_rand() & 3) ? 0 : 5;
    CC_JoinCarWithRoadSystem(vehicle);
    CC_PlaceCarBetweenNodes(vehicle, between, pos.z, 1);
    if (vehicle->vehicleType == 1) {
        veh::CVector a, b, dirNodes;
        veh::CVector* p1 = CC_Node_GetPosition(node1, &a);
        veh::CVector* p2 = CC_Node_GetPosition(node2, &b);
        CC_Sub2(&dirNodes, p2, p1);
        dirNodes.z = 0.0f;
        CC_Normalise(&dirNodes);
        float heading = CC_HeadingOf(&dirNodes);
        reinterpret_cast<void(__thiscall*)(CVehicle*, float, float, float, float)>(static_cast<void*>(vehicle->vtable[0xD4 / 4]))(vehicle, pos.x, pos.y, pos.z, heading);
    }
    CC_SetOwnerRef(&vehicle->owner, driver);
    CC_SetObjectVisible(vehicle->rwObject, 1);
    CC_Pop_PutDriverIn(driver, vehicle);
    if (model == cc_modelCop) {
        copTimer = cc_gameTimeMs + 20000;
    }
    if (model == cc_modelCop2) {
        cop2Timer = cc_gameTimeMs + 20000;
    }
}

// 0x4219E0: the speed the traffic around lets the car go (GTA's
// FindMaximumSpeedForThisCarInTraffic: the cars and peds within 14 units); flags33A
// 0x10 set. Styles 2 and 3 ignore the traffic; styles other than 0, 4 and 5 go halfway
float __cdecl FindMaximumSpeedForThisCarInTraffic(CVehicle* vehicle) {
    float mult = *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(vehicle) + 0x2E8);
    uint8_t style = vehicle->drivingStyle;
    if (style == 2 || style == 3) {
        return float(int(vehicle->cruiseSpeed)) * mult;
    }
    veh::CVector pos = Pos(vehicle);
    float left = float(pos.x - 14.0);
    float right = float(pos.x + 14.0);
    float bottom = float(pos.y - 14.0);
    float top = float(pos.y + 14.0);
    float f = float(int(left / 50.0 + 18.0));
    int xstart = int(f > 0.0f ? f : 0.0f);
    f = float(int(bottom / 50.0 + 18.0));
    int ystart = int(f > 0.0f ? f : 0.0f);
    f = float(int(right / 50.0 + 18.0));
    int xend = int(f < 35.0f ? f : 35.0f);
    f = float(int(top / 50.0 + 18.0));
    int yend = int(f < 35.0f ? f : 35.0f);
    if (CWorld::ms_nCurrentScanCode >= 0xFFFF) {
        CC_ClearScanCodes();
        CWorld::ms_nCurrentScanCode = 1;
    } else {
        CWorld::ms_nCurrentScanCode++;
    }
    float maxSpeed = float(int(vehicle->cruiseSpeed)) * mult;
    for (int y = ystart; y <= yend; y++) {
        for (int x = xstart; x <= xend; x++) {
            CPtrList* lists = reinterpret_cast<CPtrList*>(&CWorld::ms_aSectors[y * 36 + x]);
            float cur = float(int(vehicle->cruiseSpeed)) * *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(vehicle) + 0x2E8);
            CC_SlowCarDownForCarsSectorList(&lists[SECTOR_VEHICLES], vehicle, left, bottom, right, top, &maxSpeed, cur);
            cur = float(int(vehicle->cruiseSpeed)) * *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(vehicle) + 0x2E8);
            CC_SlowCarDownForPedsSectorList(&lists[SECTOR_PEDS], vehicle, left, bottom, right, top, &maxSpeed, cur);
        }
    }
    style = vehicle->drivingStyle;
    vehicle->flags33A |= 0x10;
    if (style != 0 && style != 4 && style != 5) {
        float cruise = float(int(vehicle->cruiseSpeed)) * *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(vehicle) + 0x2E8);
        return float((cruise + maxSpeed) * 0.5);
    }
    return maxSpeed;
}

// 0x421D20: the autopilot's speed towards what the traffic (and the lights) allow:
// up by 0.05 a time step, down by 3 (0xE4F84) a time step, never under 0.01; to 0 at
// once under 0.1 (GTA's SlowCarOnRailsDownForTrafficAndLights)
void __cdecl SlowCarOnRailsDownForTrafficAndLights(CVehicle* vehicle) {
    float maxSpeed;
    if (CC_ShouldCarStop(cc_stuckCarList, vehicle)) {
        CC_CarHasReasonToStop(vehicle);
        maxSpeed = 0.0f;
    } else {
        maxSpeed = CC_FindMaximumSpeedForThisCarInTraffic(vehicle);
    }
    float cur = vehicle->cruiseSpeedNow;
    float speed;
    if (cur <= maxSpeed) {
        if (cur >= maxSpeed) {
            return;
        }
        float up = float(cc_timeStep * 0.05 + vehicle->cruiseSpeedNow);
        speed = up < maxSpeed ? up : maxSpeed;
    } else {
        if (vehicle->cruiseSpeedNow == 0.0f) {
            return;
        }
        if (vehicle->cruiseSpeedNow < 0.1) {
            if (vehicle->cruiseSpeedNow == 0.0f) {
                return;
            }
            CC_AutoPilot_ModifySpeed(&vehicle->autoPilot, 0.0f);
            return;
        }
        float down = vehicle->cruiseSpeedNow - cc_timeStep * cc_slowDownRate;
        speed = down > maxSpeed ? down : maxSpeed;
    }
    if (!(0.01f < speed)) {
        speed = 0.01f;
    }
    CC_AutoPilot_ModifySpeed(&vehicle->autoPilot, speed);
}

// 0x421E50: following the path: towards the current node (or a point before it, 0.4 of
// the way back along the link, within 40 units), the speed limited by the traffic, the
// bend ahead and the angle (GTA's SteerAICarWithPhysicsFollowPath). Missions 8 and 0xC
// steer for the autopilot's target node while heading for it, and become 9 / 0xD once
// it's farther from +0x2F0 than the car is
void __cdecl SteerAICarWithPhysicsFollowPath(CVehicle* vehicle, float* swerve, float* accel, float* brake, uint8_t* handbrake) {
    uint8_t* v = reinterpret_cast<uint8_t*>(vehicle);
    veh::CVector tmp;
    float fx = CC_GetForward(vehicle, &tmp)->x;
    float fy = CC_GetForward(vehicle, &tmp)->y;
    float len = SqrtAbs(fx * fx + fy * fy);
    if (len == 0.0f) {
        fx = 1.0f;
    } else {
        fx = fx / len;
        fy = fy / len;
    }
    uint8_t* autoPilot = &vehicle->autoPilot;
    veh::CVector cur;
    CC_AutoPilot_GetCurNodeCoors(autoPilot, &cur);
    veh::CVector next;
    CC_AutoPilot_GetNextNodeCoors(autoPilot, &next);
    veh::CVector toCar;
    CC_Sub2(&toCar, &Pos(vehicle), &cur);
    float dist = SqrtAbs(toCar.x * toCar.x + toCar.y * toCar.y);
    veh::CVector link;
    CC_Sub2(&link, &next, &cur);
    float linkLen = SqrtAbs(link.x * link.x + link.y * link.y);
    toCar.z = 0.0f;
    link.z = 0.0f;
    float dot = link.y * toCar.y + link.x * toCar.x + 0.0f * 0.0f;
    bool atNode = dist < 5.0f || (dot > 0.0f && dist < 8.0f);
    if (!atNode) {
        atNode = double(dot) / (double(dist) * linkLen) > 0.7 ||
                 *reinterpret_cast<int32_t*>(v + 0x2C4) == *reinterpret_cast<int32_t*>(v + 0x2C0);
    }
    if (atNode) {
        if (CC_PickNextNodeAccordingStrategy(vehicle)) {
            veh::CVector* base = reinterpret_cast<veh::CVector*>(v + 0x2F0);
            veh::CVector carFromBase;
            CC_Sub2(&carFromBase, &Pos(vehicle), base);
            int16_t* node = CC_AutoPilot_GetTargetNode(autoPilot);
            veh::CVector target;
            target.x = float(node[4] * 0.125);
            target.y = float(node[5] * 0.125);
            target.z = float(node[6] * 0.125);
            veh::CVector targetFromBase;
            CC_Sub2(&targetFromBase, &target, base);
            uint8_t mission = vehicle->mission;
            if (mission == 8 || mission == 0xC) {
                float carSq = carFromBase.x * carFromBase.x + carFromBase.y * carFromBase.y + carFromBase.z * carFromBase.z;
                float targetSq = targetFromBase.x * targetFromBase.x + targetFromBase.y * targetFromBase.y + targetFromBase.z * targetFromBase.z;
                if (targetSq > carSq) {
                    vehicle->mission = mission == 8 ? 9 : 0xD;
                    return;
                }
                CC_SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, target.x, target.y, swerve, accel, brake, handbrake);
                return;
            }
        }
        veh::CVector node;
        cur = *CC_AutoPilot_GetCurNodeCoors(autoPilot, &node);
        veh::CVector d;
        veh::CVector* diff = CC_Sub2(&d, &cur, &Pos(vehicle));
        dist = SqrtAbs(diff->x * diff->x + diff->y * diff->y);
    }
    veh::CVector target;
    CC_AutoPilot_GetCurNodeCoors(autoPilot, &target);
    if (dist < 40.0f) {
        float dx, dy;
        CC_AutoPilot_GetCurDirection(autoPilot, &dx, &dy);
        target.x = float(target.x - dx * dist * 0.4);
        target.y = float(target.y - dy * dist * 0.4);
    }
    veh::CVector pos = Pos(vehicle);
    float angleToTarget = CC_GetATanOfXY(target.x - pos.x, target.y - pos.y);
    float angleForward = CC_GetATanOfXY(fx, fy);
    if (vehicle->drivingStyle == 2) {
        angleToTarget = CC_FindAngleToWeaveThroughTraffic(vehicle, nullptr, angleToTarget, angleForward);
    }
    float steer = CC_LimitRadianAngle(angleToTarget - angleForward);
    if (Magnitude(vehicle->moveSpeed) > 0.45) {
        if (steer < -0.2) {
            steer = -0.2f;
        }
        if (steer > 0.2) {
            steer = 0.2f;
        }
    }
    veh::CVector* forward = CC_GetForward(vehicle, &tmp);
    float forwardSpeed = vehicle->moveSpeed.y * forward->y + vehicle->moveSpeed.x * forward->x + vehicle->moveSpeed.z * forward->z;
    float speedNow = float(forwardSpeed * 30.0);
    float trafficFactor;
    switch (vehicle->drivingStyle) {
    case 0:
    case 1:
    case 4:
    case 5:
        trafficFactor = float(double(CC_FindMaximumSpeedForThisCarInTraffic(vehicle)) / int(vehicle->cruiseSpeed));
        break;
    default:
        trafficFactor = 1.0f;
        break;
    }
    int8_t style = int8_t(vehicle->drivingStyle);
    if (style >= 0 && (style <= 1 || style == 5) && CC_ShouldCarStop(cc_stuckCarList, vehicle)) {
        CC_CarHasReasonToStop(vehicle);
        trafficFactor = 0.0f;
    }
    veh::CVector nodeTmp;
    veh::CVector toNode;
    CC_Sub2(&toNode, CC_AutoPilot_GetCurNodeCoors(autoPilot, &nodeTmp), &Pos(vehicle));
    float angleToNode = CC_GetATanOfXY(toNode.x, toNode.y);
    float angleFactor = CC_SpeedFactorForSteer(angleToNode - angleForward, 0.4f, 1.2f, 0.4f);
    float curX, curY, nextX, nextY;
    CC_AutoPilot_GetCurDirection(autoPilot, &curX, &curY);
    CC_AutoPilot_GetNextDirection(autoPilot, &nextX, &nextY);
    float curAngle = CC_GetATanOfXY(curX, curY);
    float nextAngle = CC_GetATanOfXY(nextX, nextY);
    // (the bend ahead slows the car as it nears the node, from 40 units, at cruise speeds of 12 up)
    float bendFactor;
    if (40.0f < dist || vehicle->cruiseSpeed < 12) {
        bendFactor = 1.0f;
    } else {
        float f = CC_SpeedFactorForSteer(curAngle - nextAngle, 0.1f, 1.2f, 0.4f);
        bendFactor = float(1.0 - (1.0 - f) * (1.0 - dist / 40.0));
    }
    float factor = bendFactor <= angleFactor ? bendFactor : angleFactor;
    factor = trafficFactor <= factor ? trafficFactor : factor;
    float wanted = float(int(vehicle->cruiseSpeed)) * factor;
    float diffSpeed = wanted - speedNow;
    *brake = 0.0f;
    if (wanted < 0.05 && diffSpeed < 0.03) {
        *brake = 1.0f;
        *accel = 0.0f;
    } else if (diffSpeed <= 0.0f) {
        *accel = 0.0f;
        float b = float(-diffSpeed / 20.0);
        if (b > 0.5) {
            b = 0.5f;
        }
        *brake = b;
    } else if (speedNow < 2.0f) {
        float a = float(diffSpeed * 0.25);
        *accel = a <= 1.0f ? a : 1.0f;
    } else {
        float a = float(diffSpeed * 0.125);
        *accel = a <= 1.0f ? a : 1.0f;
    }
    *swerve = steer;
    *handbrake = 0;
}

// 0x422800: following the autopilot's points: the next once within 5 x speed + 1.5
// units; out of points, the mission ends (0x17: the autopilot's 0x418170 instead)
void __cdecl SteerAICarWithPhysicsFollowPoints(CVehicle* vehicle, float* swerve, float* accel, float* brake, uint8_t* handbrake) {
    float reach = float(Magnitude(vehicle->moveSpeed) * 5.0 + 1.5);
    uint8_t* autoPilot = &vehicle->autoPilot;
    veh::CVector target;
    CC_AutoPilot_GetPoint(autoPilot, &target);
    veh::CVector d;
    CC_Sub2(&d, &Pos(vehicle), &target);
    float sq = d.y * d.y + d.x * d.x + 0.0f * 0.0f;
    if (sq - reach * reach < 0.0f) {
        if (!CC_AutoPilot_NextPoint(autoPilot)) {
            if (vehicle->mission == 0x17) {
                // (the flag is pushed as a whole register, only its low byte set)
                CC_AutoPilot_Fn418170(autoPilot, *reinterpret_cast<int32_t*>(autoPilot + 0x84), (vehicle->typeAndStatus & 0xF8) == 0x10, 1);
                return;
            }
            vehicle->mission = 0;
            return;
        }
        veh::CVector tmp;
        target = *CC_AutoPilot_GetPoint(autoPilot, &tmp);
    }
    CC_SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, target.x, target.y, swerve, accel, brake, handbrake);
}

// 0x422970 (thiscall): the traffic's frame: a random car made (under 30 of +0x1D60,
// and fewer cars than the population's limit; +0x1D64 frames of delay after a
// cutscene), one car's path updated in turn, and the pool kept from filling up
void __fastcall GenerateRandomCars(uint8_t* self, re::Edx) {
    int32_t& delay = *reinterpret_cast<int32_t*>(self + 0x1D64);
    if (cc_cutsceneRunning) {
        delay = 2;
        return;
    }
    if (*reinterpret_cast<int32_t*>(self + 0x1D60) < 30) {
        uint32_t limit = *reinterpret_cast<uint32_t*>(cc_popLimits + 0x18);
        if (uint32_t(CC_CountRandomCars()) < limit) {
            if (delay == 0) {
                CC_GenerateOneRandomCar(self);
            } else if (--delay == 0) {
                cc_flagC2D1A9 = 0x14;
            }
        }
    }
    CPool<CVehicle>* pool = CPools::ms_pVehiclePool;
    cc_carToUpdate++;
    if (cc_carToUpdate >= pool->size) {
        cc_carToUpdate = 0;
    }
    CVehicle* vehicle = pool->GetAt(cc_carToUpdate);
    if (vehicle) {
        CC_UpdateCarPath(self, vehicle);
    }
    CC_RemoveCarsIfThePoolGetsFull();
}

// 0x422A10: a car driven on rails (not physics): along the curve between its nodes by
// the time spent on it, its speed what that takes. Missions 0 and 0xA..0xB stop it;
// the temp action 1 waits (a second at a time) for the traffic to clear. +0x298 0.5:
// braking (GTA's UpdateCarOnRails)
void __cdecl UpdateCarOnRails(CVehicle* vehicle) {
    uint8_t* v = reinterpret_cast<uint8_t*>(vehicle);
    uint8_t* autoPilot = &vehicle->autoPilot;
    float& brakeLight = *reinterpret_cast<float*>(v + 0x298);
    uint8_t& tempAction = vehicle->unknown2DB;
    uint32_t& tempActionTime = *reinterpret_cast<uint32_t*>(v + 0x2DC);
    veh::CVector newSpeed = {0.0f, 0.0f, 0.0f};
    veh::CVector newPos = {0.0f, 0.0f, 0.0f};
    int8_t mission = int8_t(vehicle->mission);
    if (mission == 0 || (mission > 9 && mission <= 0xB)) {
        reinterpret_cast<void(__thiscall*)(CVehicle*, float)>(static_cast<void*>(vehicle->vtable[0xE0 / 4]))(vehicle, 0.0f);
        vehicle->flags33A |= 4;
        *reinterpret_cast<float*>(v + 0x294) = float(vehicle->unknown29C * 0.0);
        brakeLight = 0.5f;
        vehicle->moveSpeed = cc_zeroVector;
        CC_AutoPilot_ModifySpeed(autoPilot, 0.0f);
        return;
    }
    if (int8_t(tempAction) == 1) {
        float slower = vehicle->cruiseSpeedNow - cc_timeStep * cc_slowDownRate;
        CC_AutoPilot_ModifySpeed(autoPilot, slower >= 0.0 ? slower : 0.0f);
        if (cc_gameTimeMs > tempActionTime) {
            float maxSpeed = CC_FindMaximumSpeedForThisCarInTraffic(vehicle);
            if (0.0f < maxSpeed) {
                tempAction = 0;
                *reinterpret_cast<uint32_t*>(v + 0x2CC) = cc_gameTimeMs;
                *reinterpret_cast<uint32_t*>(v + 0x2D0) = cc_gameTimeMs;
            } else {
                tempAction = 1;
                tempActionTime = cc_gameTimeMs + 1000;
                if (0.0f == vehicle->cruiseSpeedNow) {
                    CC_SetMoveSpeed(vehicle, &cc_zeroVector);
                    brakeLight = 0.5f;
                    return;
                }
            }
        } else if (0.0f == vehicle->cruiseSpeedNow) {
            vehicle->moveSpeed = cc_zeroVector;
            brakeLight = 0.5f;
            return;
        }
    }
    CC_SlowCarOnRailsDownForTrafficAndLights(vehicle);
    uint32_t& timeEnteredCurve = *reinterpret_cast<uint32_t*>(v + 0x2B4);
    uint32_t& timeOnCurve = *reinterpret_cast<uint32_t*>(v + 0x2B8);
    if (timeEnteredCurve == 0xFFFFFFFF) {
        brakeLight = 0.5f;
        vehicle->moveSpeed = cc_zeroVector;
        return;
    }
    if (cc_gameTimeMs >= timeOnCurve + timeEnteredCurve) {
        CC_PickNextNodeAccordingStrategy(vehicle);
    }
    if ((vehicle->typeAndStatus & 0xF8) == 0x18) {
        return;
    }
    // (both as unsigned, through int with 2^32 added when negative)
    float elapsed = float(double(int32_t(cc_gameTimeMs - timeEnteredCurve)) + (int32_t(cc_gameTimeMs - timeEnteredCurve) < 0 ? 4294967296.0 : 0.0));
    float total = float(double(int32_t(timeOnCurve)) + (int32_t(timeOnCurve) < 0 ? 4294967296.0 : 0.0));
    float between = float(double(elapsed) / double(total));
    if (between < 0.0f) {
        between = 0.0f;
    } else if (1.0f < between) {
        between = 1.0f;
    }
    float cx, cy, nx, ny;
    CC_AutoPilot_GetCurDirection(autoPilot, &cx, &cy);
    CC_AutoPilot_GetNextDirection(autoPilot, &nx, &ny);
    veh::CVector dirCur = {cx, cy, 0.0f};
    veh::CVector dirNext = {nx, ny, 0.0f};
    CC_Normalise(&dirCur);
    CC_Normalise(&dirNext);
    veh::CVector curPos, nextPos;
    CC_AutoPilot_GetCurNodeCoors(autoPilot, &curPos);
    CC_AutoPilot_GetNextNodeCoors(autoPilot, &nextPos);
    CC_CalcCurvePoint(&curPos, &nextPos, &dirCur, &dirNext, between, timeOnCurve, &newPos, &newSpeed);
    newPos.z = 15.0f;
    re::Ptr32<CEntity> inTheWay = nullptr;
    CC_FindCarInTheWay(vehicle, &newPos, &inTheWay);
    if (inTheWay) {
        int32_t model = inTheWay->modelIndex;
        if (model == cc_modelsStopFor[3] || model == cc_modelsStopFor[2] || model == cc_modelsStopFor[1] || model == cc_modelsStopFor[0]) {
            tempAction = 1;
            tempActionTime = cc_gameTimeMs + 1000;
            if (0.0f == vehicle->cruiseSpeedNow) {
                CC_SetMoveSpeed(vehicle, &cc_zeroVector);
                brakeLight = 0.5f;
                return;
            }
        }
    }
    CC_VectorDiv(&newSpeed, 30.0f);
    veh::CVector& old = vehicle->moveSpeed;
    float newSq = float(newSpeed.x * newSpeed.x + newSpeed.y * newSpeed.y + newSpeed.z * newSpeed.z);
    float oldSq = float(old.y * old.y + old.x * old.x + old.z * old.z);
    brakeLight = oldSq > float(newSq + 0.002) ? 0.5f : 0.0f;
    vehicle->moveSpeed = newSpeed;
}

// 0x422EB0: UpdateCarOnRails along the autopilot's points instead of path nodes: at
// each point the next curve's time from its length and the cruise speed; out of
// points, the mission ends (0x17: the autopilot's 0x418170 instead)
void __cdecl UpdateCarOnRailsFollowingPoints(CVehicle* vehicle) {
    uint8_t* v = reinterpret_cast<uint8_t*>(vehicle);
    uint8_t* autoPilot = &vehicle->autoPilot;
    float& brakeLight = *reinterpret_cast<float*>(v + 0x298);
    uint8_t& tempAction = vehicle->unknown2DB;
    uint32_t& tempActionTime = *reinterpret_cast<uint32_t*>(v + 0x2DC);
    veh::CVector p1 = {0.0f, 0.0f, 0.0f};
    veh::CVector p0 = {0.0f, 0.0f, 0.0f};
    int8_t mission = int8_t(vehicle->mission);
    if (mission == 0 || (mission > 9 && mission <= 0xB)) {
        reinterpret_cast<void(__thiscall*)(CVehicle*, float)>(static_cast<void*>(vehicle->vtable[0xE0 / 4]))(vehicle, 0.0f);
        vehicle->flags33A |= 4;
        *reinterpret_cast<float*>(v + 0x294) = float(vehicle->unknown29C * 0.0);
        brakeLight = 0.5f;
        vehicle->moveSpeed = cc_zeroVector;
        CC_AutoPilot_ModifySpeed(autoPilot, 0.0f);
        return;
    }
    if (int8_t(tempAction) == 1) {
        float slower = vehicle->cruiseSpeedNow - cc_timeStep * cc_slowDownRate;
        CC_AutoPilot_ModifySpeed(autoPilot, slower >= 0.0 ? slower : 0.0f);
        if (cc_gameTimeMs > tempActionTime) {
            float maxSpeed = CC_FindMaximumSpeedForThisCarInTraffic(vehicle);
            if (0.0f < maxSpeed) {
                tempAction = 0;
                *reinterpret_cast<uint32_t*>(v + 0x2CC) = cc_gameTimeMs;
                *reinterpret_cast<uint32_t*>(v + 0x2D0) = cc_gameTimeMs;
            } else {
                tempAction = 1;
                tempActionTime = cc_gameTimeMs + 1000;
                if (0.0f == vehicle->cruiseSpeedNow) {
                    CC_SetMoveSpeed(vehicle, &cc_zeroVector);
                    brakeLight = 0.5f;
                    return;
                }
            }
        } else if (0.0f == vehicle->cruiseSpeedNow) {
            vehicle->moveSpeed = cc_zeroVector;
            brakeLight = 0.5f;
            return;
        }
    }
    CC_SlowCarOnRailsDownForTrafficAndLights(vehicle);
    uint32_t& timeEnteredCurve = *reinterpret_cast<uint32_t*>(v + 0x2B4);
    uint32_t& timeOnCurve = *reinterpret_cast<uint32_t*>(v + 0x2B8);
    if (timeEnteredCurve == 0xFFFFFFFF) {
        brakeLight = 0.01f;
        vehicle->moveSpeed = cc_zeroVector;
        return;
    }
    if (cc_gameTimeMs >= timeOnCurve + timeEnteredCurve) {
        if (!CC_AutoPilot_NextPoint(autoPilot)) {
            if (vehicle->mission == 0x17) {
                // (the flag is pushed as a whole register, only its low byte set)
                CC_AutoPilot_Fn418170(autoPilot, *reinterpret_cast<int32_t*>(autoPilot + 0x84), (vehicle->typeAndStatus & 0xF8) == 0x10, 1);
                return;
            }
            vehicle->mission = 0;
            return;
        }
        timeEnteredCurve += timeOnCurve;
        veh::CVector tmp;
        p0 = *CC_AutoPilot_GetPrevPoint(autoPilot, &tmp);
        p1 = *CC_AutoPilot_GetPoint(autoPilot, &tmp);
        float x0, y0, x1, y1;
        CC_AutoPilot_GetPrevPointDirection(autoPilot, &x0, &y0);
        CC_AutoPilot_GetPointDirection(autoPilot, &x1, &y1);
        float length = CC_CalcSpeedScaleFactor(&p0, &p1, x0, y0, x1, y1);
        // (truncated through 64 bits, then as unsigned)
        uint32_t msPerUnit = uint32_t(int64_t(1000.0 / vehicle->cruiseSpeedNow));
        timeOnCurve = uint32_t(int64_t(double(msPerUnit) * length));
    }
    float elapsed = float(double(int32_t(cc_gameTimeMs - timeEnteredCurve)) + (int32_t(cc_gameTimeMs - timeEnteredCurve) < 0 ? 4294967296.0 : 0.0));
    float total = float(double(int32_t(timeOnCurve)) + (int32_t(timeOnCurve) < 0 ? 4294967296.0 : 0.0));
    float between = float(double(elapsed) / double(total));
    veh::CVector dirPrev = {0.0f, 0.0f, 0.0f};
    veh::CVector dirCur = {0.0f, 0.0f, 0.0f};
    CC_AutoPilot_GetPrevPointDirection(autoPilot, &dirPrev.x, &dirPrev.y);
    CC_AutoPilot_GetPointDirection(autoPilot, &dirCur.x, &dirCur.y);
    CC_Normalise(&dirPrev);
    CC_Normalise(&dirCur);
    veh::CVector tmp;
    p0 = *CC_AutoPilot_GetPrevPoint(autoPilot, &tmp);
    p1 = *CC_AutoPilot_GetPoint(autoPilot, &tmp);
    veh::CVector newPos = {0.0f, 0.0f, 0.0f};
    veh::CVector newSpeed = {0.0f, 0.0f, 0.0f};
    CC_CalcCurvePoint(&p0, &p1, &dirPrev, &dirCur, between, timeOnCurve, &newPos, &newSpeed);
    newPos.z = 15.0f;
    re::Ptr32<CEntity> inTheWay = nullptr;
    CC_FindCarInTheWay(vehicle, &newPos, &inTheWay);
    if (inTheWay) {
        int32_t model = inTheWay->modelIndex;
        if (model == cc_modelsStopFor[3] || model == cc_modelsStopFor[2] || model == cc_modelsStopFor[1] || model == cc_modelsStopFor[0]) {
            tempAction = 1;
            tempActionTime = cc_gameTimeMs + 1000;
            if (0.0f == vehicle->cruiseSpeedNow) {
                CC_SetMoveSpeed(vehicle, &cc_zeroVector);
                brakeLight = 0.5f;
                return;
            }
        }
    }
    const double perFrame = double(1.0f / 30.0f);
    newSpeed.x = float(newSpeed.x * perFrame);
    newSpeed.y = float(newSpeed.y * perFrame);
    newSpeed.z = float(perFrame * newSpeed.z);
    veh::CVector& old = vehicle->moveSpeed;
    float newSq = float(newSpeed.y * newSpeed.y + newSpeed.x * newSpeed.x + newSpeed.z * newSpeed.z);
    float oldSq = float(old.y * old.y + old.x * old.x + old.z * old.z);
    brakeLight = oldSq > float(newSq + 0.002) ? 0.5f : 0.0f;
    vehicle->moveSpeed = newSpeed;
}

// 0x423460: the steering, throttle, brake and handbrake for the car's mission (GTA's
// SteerAICarWithPhysics_OnlyMission). The target is the player's position unless the mission
// has its own (+0x2F0, the entity at +0x324, the path node)
void __cdecl SteerAICarWithPhysics_OnlyMission(CVehicle* vehicle, float* swerve, float* accel, float* brake, uint8_t* handbrake) {
    uint8_t* v = reinterpret_cast<uint8_t*>(vehicle);
    veh::CVector target;
    CC_FindPlayerCoors(&target);
    CVehicle* entity = *reinterpret_cast<re::Ptr32<CVehicle>*>(v + 0x324);
    veh::CVector pos = Pos(vehicle);
    veh::CVector tmp, tmp2;
    switch (int8_t(vehicle->mission)) {
    case 0:
    case 0xA:
    case 0xB:
        *swerve = 0.0f;
        *accel = 0.0f;
        *handbrake = 1;
        *brake = 0.5f;
        return;
    case 1:
    case 2:
    case 4:
    case 8:
    case 0xC:
    case 0xE:
    case 0x10:
        if (vehicle->autoPilotFlags & 0x10) {
            *swerve = 0.0f;
            *accel = 1.0f;
            *handbrake = 0;
            *brake = 0.0f;
            return;
        }
        CC_SteerAICarWithPhysicsFollowPath(vehicle, swerve, accel, brake, handbrake);
        return;
    case 3: {
        // chasing the player's car: level with it (a side by the seed), or offset by the seed
        CVehicle* player = CC_FindPlayerVehicle(0);
        if (player) {
            veh::CVector* right = CC_GetRight(player);
            float rx = right->x;
            uint16_t seed = *reinterpret_cast<uint16_t*>(v + 0x10C);
            float ry = right->y;
            bool alongside = false;
            if (seed & 1) {
                veh::CVector* a = CC_GetForward(vehicle, &tmp);
                veh::CVector* b = CC_GetForward(player, &tmp2);
                if (CC_DotProduct(b, a) > 0.5f) {
                    alongside = true;
                    uint8_t* col = *reinterpret_cast<re::Ptr32<uint8_t>*>(cc_modelInfoPtrs[player->modelIndex] + 0xC);
                    float playerWidth = *reinterpret_cast<float*>(col + 0x20);
                    col = *reinterpret_cast<re::Ptr32<uint8_t>*>(cc_modelInfoPtrs[vehicle->modelIndex] + 0xC);
                    float ownWidth = *reinterpret_cast<float*>(col + 0x20);
                    float w = float((ownWidth + playerWidth) * 0.35);
                    veh::CVector d;
                    veh::CVector toPlayer = *CC_Sub2(&d, &Pos(player), &pos);
                    if (CC_DotProduct(CC_GetRight(vehicle), &toPlayer) > 0.0f) {
                        target.x = target.x + rx * w;
                        target.y = w * ry + target.y;
                    } else {
                        target.x = target.x - rx * w;
                        target.y = target.y - ry * w;
                    }
                    veh::CVector rel;
                    CC_Sub2(&rel, &player->moveSpeed, &vehicle->moveSpeed);
                    float limit = float(CC_Magnitude(&rel) * 12.0 + 2.0);
                    float dy = pos.y - target.y;
                    float dx = pos.x - target.x;
                    float sq = dx * dx + dy * dy;
                    if (CC_Sqrt(sq) < limit && vehicle->unknown2DB == 0) {
                        toPlayer = *CC_Sub2(&d, &Pos(player), &pos);
                        vehicle->unknown2DB = CC_DotProduct(CC_GetRight(vehicle), &toPlayer) < 0.0f ? 6 : 7;
                        *reinterpret_cast<uint32_t*>(v + 0x2DC) = cc_gameTimeMs + 200;
                    }
                }
            }
            if (!alongside) {
                float f = float((int(uint8_t(seed)) - 0x80) * 0.00625);
                target.x = target.x + rx * f;
                target.y = f * ry + target.y;
            }
        }
        CC_SteerAICarWithPhysicsHeadingForTarget(vehicle, CC_FindPlayerVehicle(0), target.x, target.y, swerve, accel, brake, handbrake);
        return;
    }
    case 5: {
        veh::CVector* a = CC_FindPlayerSpeed();
        veh::CVector* b = CC_FindPlayerSpeed();
        CC_SteerAICarWithPhysicsTryingToBlockTarget(vehicle, target.x, target.y, a->x, b->y, swerve, accel, brake, handbrake);
        return;
    }
    case 6: {
        veh::CVector* a = CC_FindPlayerSpeed();
        veh::CVector* b = CC_FindPlayerSpeed();
        CC_SteerAICarWithPhysicsTryingToBlockTarget_Stop(vehicle, target.x, target.y, a->x, b->y, swerve, accel, brake, handbrake);
        return;
    }
    case 9:
    case 0xD:
    case 0x13:
        CC_SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, *reinterpret_cast<float*>(v + 0x2F0), *reinterpret_cast<float*>(v + 0x2F4), swerve, accel, brake,
                                                 handbrake);
        return;
    case 0xF: {
        veh::CVector& p = Pos(entity);
        CC_SteerAICarWithPhysicsHeadingForTarget(vehicle, entity, p.x, p.y, swerve, accel, brake, handbrake);
        return;
    }
    case 0x11: {
        veh::CVector& p = Pos(entity);
        CC_SteerAICarWithPhysicsTryingToBlockTarget(vehicle, p.x, p.y, entity->moveSpeed.x, entity->moveSpeed.y, swerve, accel, brake, handbrake);
        return;
    }
    case 0x12: {
        veh::CVector& p = Pos(entity);
        CC_SteerAICarWithPhysicsTryingToBlockTarget_Stop(vehicle, p.x, p.y, entity->moveSpeed.x, entity->moveSpeed.y, swerve, accel, brake, handbrake);
        return;
    }
    case 0x14:
        CC_SteerAICarWithPhysicsHeadingForTarget(vehicle, nullptr, target.x, target.y, swerve, accel, brake, handbrake);
        return;
    case 0x15:
        CC_SteerAICarWithPhysics_Mission15(vehicle, swerve, accel, brake, handbrake);
        return;
    case 0x16:
    case 0x17:
        CC_SteerAICarWithPhysicsFollowPoints(vehicle, swerve, accel, brake, handbrake);
        return;
    case 0x18: {
        // alongside the entity where it'll be in 30 frames, on the side the car is
        if (!entity) {
            return;
        }
        veh::CVector* p = &Pos(entity);
        veh::CVector ahead;
        CC_Add(&ahead, p, CC_Scale(&tmp, &entity->moveSpeed, 30.0f));
        uint8_t* col = *reinterpret_cast<re::Ptr32<uint8_t>*>(cc_modelInfoPtrs[entity->modelIndex] + 0xC);
        float entityWidth = *reinterpret_cast<float*>(col + 0x20);
        col = *reinterpret_cast<re::Ptr32<uint8_t>*>(cc_modelInfoPtrs[vehicle->modelIndex] + 0xC);
        float ownWidth = *reinterpret_cast<float*>(col + 0x20);
        veh::CVector* right = CC_GetRight(entity);
        float rx = right->x;
        float ry = right->y;
        float w = float(ownWidth + entityWidth + 0.1);
        veh::CVector ownRight = *CC_GetRight(vehicle);
        veh::CVector d;
        if (CC_DotProduct(&ownRight, CC_Sub2(&d, &Pos(entity), &pos)) > 0.0f) {
            ahead.x = ahead.x - rx * w;
            ahead.y = ahead.y - ry * w;
        } else {
            ahead.x = ahead.x + rx * w;
            ahead.y = w * ry + ahead.y;
        }
        CC_SteerAICarWithPhysicsHeadingForTarget(vehicle, entity, ahead.x, ahead.y, swerve, accel, brake, handbrake);
        return;
    }
    case 0x19: {
        if (!entity) {
            return;
        }
        veh::CVector ownRight = *CC_GetRight(vehicle);
        veh::CVector d;
        if (CC_DotProduct(&ownRight, CC_Sub2(&d, &Pos(entity), &pos)) > 0.0f) {
            vehicle->unknown2DB = 6;
        } else {
            vehicle->unknown2DB = 7;
        }
        *reinterpret_cast<uint32_t*>(v + 0x2DC) = cc_gameTimeMs + 100;
        return;
    }
    case 0x1A: {
        // the path, stopping when the autopilot's target is nearer than the node (then
        // the path again, as 0x1B)
        CC_SteerAICarWithPhysicsFollowPath(vehicle, swerve, accel, brake, handbrake);
        veh::CVector nodePos;
        CC_Node_GetPosition(CC_Paths_GetNode(cc_thePaths, *reinterpret_cast<int32_t*>(&vehicle->autoPilot)), &nodePos);
        veh::CVector targetPos = EntityPos(vehicle->autoPilotTarget);
        veh::CVector d;
        veh::CVector* toNode = CC_Sub2(&d, &nodePos, &pos);
        float nodeSq = toNode->x * toNode->x + toNode->y * toNode->y;
        veh::CVector d2;
        veh::CVector* toTarget = CC_Sub2(&d2, &targetPos, &pos);
        float targetSq = toTarget->x * toTarget->x + toTarget->y * toTarget->y;
        if (targetSq < nodeSq) {
            CC_CarHasReasonToStop(vehicle);
            *accel = 0.0f;
            *brake = 1.0f;
        }
    }
        [[fallthrough]];
    case 0x1B: {
        veh::CVector nodePos;
        veh::CVector* p = CC_Node_GetPosition(CC_Paths_GetNode(cc_thePaths, *reinterpret_cast<int32_t*>(&vehicle->autoPilot)), &nodePos);
        *reinterpret_cast<veh::CVector*>(v + 0x2F0) = *p;
        CC_SteerAICarWithPhysicsFollowPath(vehicle, swerve, accel, brake, handbrake);
        return;
    }
    case 0x1C: {
        // the car's own controls (its vf59 given 0x4CCC00's vector)
        veh::CVector* arg = CC_Vehicle_Fn4CCC00(vehicle, &tmp);
        reinterpret_cast<void(__thiscall*)(CVehicle*, float, float, float)>(static_cast<void*>(vehicle->vtable[0xEC / 4]))(
            vehicle, arg->x, arg->y, arg->z);   // (the vector as its floats, as the bodies take it)
        *swerve = vehicle->steerAngle;
        *accel = *reinterpret_cast<float*>(v + 0x294);
        *brake = *reinterpret_cast<float*>(v + 0x298);
        *handbrake = (vehicle->flags33A >> 2) & 1;
        return;
    }
    default:
        return;
    }
}

// 0x423F70: the car's pedals and steering: from its temp action (+0x2DB: reversing,
// turning, waiting... until +0x2DC's time), else from the mission. The steering goes
// through vf56, +0x294 the gas (times +0x29C), +0x298 the brake, flags33A 4 the
// handbrake (GTA's SteerAICarWithPhysics)
void __cdecl SteerAICarWithPhysics(CVehicle* vehicle) {
    uint8_t* v = reinterpret_cast<uint8_t*>(vehicle);
    uint32_t actionTime = *reinterpret_cast<uint32_t*>(v + 0x2DC);
    float swerve, accel, brake;
    uint8_t handbrake;
    uint8_t action = vehicle->unknown2DB;
    switch (int8_t(action)) {
    case 1:
        swerve = 0.0f;
        accel = 0.0f;
        handbrake = 0;
        brake = 0.2f;
        break;
    case 2: {
        // reversing: the mission's steering the other way
        uint8_t unused;
        CC_SteerAICarWithPhysics_OnlyMission(vehicle, &swerve, &accel, &brake, &unused);
        swerve = -swerve;
        handbrake = 0;
        veh::CVector tmp;
        if (CC_DotProduct(&vehicle->moveSpeed, CC_GetForward(vehicle, &tmp)) > 0.04f) {
            accel = 0.0f;
            brake = 0.5f;
        } else {
            accel = -0.5f;
            brake = 0.0f;
        }
        break;
    }
    case 3:
        swerve = 1.0f;
        handbrake = 1;
        accel = 0.0f;
        brake = 0.0f;
        break;
    case 4:
        swerve = -1.0f;
        handbrake = 1;
        accel = 0.0f;
        brake = 0.0f;
        break;
    case 5:
        swerve = 0.0f;
        accel = 0.0f;
        handbrake = 1;
        brake = 0.0f;
        break;
    case 6:
        swerve = 1.0f;
        handbrake = 0;
        accel = 1.0f;
        brake = 0.0f;
        break;
    case 7:
        swerve = -1.0f;
        accel = 1.0f;
        handbrake = 0;
        brake = 0.0f;
        break;
    case 8:
        swerve = 0.0f;
        handbrake = 0;
        accel = 0.5f;
        brake = 0.0f;
        break;
    case 9:
    case 10:
        swerve = -0.15f;
        handbrake = 0;
        if (action == 10) {
            swerve = 0.15f;
        }
        accel = vehicle->drivingStyle == 3 ? 0.5f : 0.0f;
        brake = 0.001f;
        if (cc_gameTimeMs > actionTime - 1000) {
            swerve = -swerve;
        }
        break;
    case 12:
        swerve = 0.0f;
        accel = 0.0f;
        handbrake = 1;
        brake = 1.0f;
        break;
    default:
        CC_SteerAICarWithPhysics_OnlyMission(vehicle, &swerve, &accel, &brake, &handbrake);
        goto apply;
    }
    if (cc_gameTimeMs > actionTime) {
        vehicle->unknown2DB = 0;
    }
apply:
    reinterpret_cast<void(__thiscall*)(CVehicle*, float)>(static_cast<void*>(vehicle->vtable[0xE0 / 4]))(vehicle, swerve);
    *reinterpret_cast<float*>(v + 0x294) = vehicle->unknown29C * accel;
    vehicle->flags33A ^= ((handbrake << 2) ^ vehicle->flags33A) & 4;
    *reinterpret_cast<float*>(v + 0x298) = brake;
}

CARCTRL_REVERSED("CCarCtrl::SteerAICarWithPhysics_OnlyMission", 0x00423460, &SteerAICarWithPhysics_OnlyMission);
CARCTRL_REVERSED("CCarCtrl::SteerAICarWithPhysics", 0x00423F70, &SteerAICarWithPhysics);
CARCTRL_REVERSED("CCarCtrl::UpdateCarOnRails", 0x00422A10, &UpdateCarOnRails);
CARCTRL_REVERSED("CCarCtrl::UpdateCarOnRailsFollowingPoints", 0x00422EB0, &UpdateCarOnRailsFollowingPoints);
CARCTRL_REVERSED("CCarCtrl::SteerAICarWithPhysicsFollowPoints", 0x00422800, &SteerAICarWithPhysicsFollowPoints);
CARCTRL_REVERSED("CCarCtrl::GenerateRandomCars", 0x00422970, &GenerateRandomCars);
CARCTRL_REVERSED("CCarCtrl::GenerateOneRandomCar", 0x00421370, &GenerateOneRandomCar);
CARCTRL_REVERSED("CCarCtrl::SteerAICarWithPhysicsFollowPath", 0x00421E50, &SteerAICarWithPhysicsFollowPath);
CARCTRL_REVERSED("CCarCtrl::FindMaximumSpeedForThisCarInTraffic", 0x004219E0, &FindMaximumSpeedForThisCarInTraffic);
CARCTRL_REVERSED("CCarCtrl::SlowCarOnRailsDownForTrafficAndLights", 0x00421D20, &SlowCarOnRailsDownForTrafficAndLights);
CARCTRL_REVERSED("CCarCtrl::RemoveCarsIfThePoolGetsFull", 0x00420630, &RemoveCarsIfThePoolGetsFull);
CARCTRL_REVERSED("CCarCtrl::SlowCarDownForCarsSectorList", 0x00420740, &SlowCarDownForCarsSectorList);
CARCTRL_REVERSED("CCarCtrl::FindAngleToWeaveThroughTraffic", 0x004208B0, &FindAngleToWeaveThroughTraffic);
CARCTRL_REVERSED("CCarCtrl::SteerAICarWithPhysicsHeadingForTarget", 0x00420E20, &SteerAICarWithPhysicsHeadingForTarget);
CARCTRL_REVERSED("CCarCtrl::SteerAICarWithPhysicsTryingToBlockTarget", 0x00421240, &SteerAICarWithPhysicsTryingToBlockTarget);

} // namespace CCarCtrl
