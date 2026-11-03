// The ped population's spawning and culling: the cull distances, the spawn band
// around the player, the POIs, the objects turned into dummies and back, and the
// ambient peds added (with a partner to talk to) and culled (0x49C930 .. 0x49FCD0;
// the population itself is in PedPopulation.cpp).

#include "game/PedPopulation.h"

#include <cfloat>
#include <cmath>
#include <cstdio>

#include "game/Pool.h"

#include "game/Crt.h"
#include "reversed.h"
#include "runflags.h"
namespace {
using pop::At;
using pop::CVector;
using pop::PosOf;

GAME_VAR(CPedPopulation*, P5S_population, 0x00C2C108);
GAME_VAR(int32_t, P5S_currArea, 0x00BD1008);            // CGame::currArea
GAME_VAR(uint32_t, P5S_timeInMs, 0x00C1A9B4);            // CTimer::m_snTimeInMilliseconds
GAME_VAR(uint32_t, P5S_frameCounter, 0x00C1A99C);        // CTimer::m_FrameCounter
GAME_VAR(uint8_t*, P5S_player, 0x00C1AEA8);              // FindPlayerPed()
GAME_ARRAY(uint8_t*, P5S_modelInfoPtrs, 0x00C67738);     // CModelInfo::ms_modelInfoPtrs
GAME_VAR(CPool<uint8_t>*, P5S_pedPool, 0x00C0F5F0);
GAME_VAR(CPool<uint8_t>*, P5S_objectPool, 0x00C0F614);   // (chained on, Next())
GAME_VAR(CPool<uint8_t>*, P5S_triggerPool, 0x020C7C48);
GAME_VAR(void*, P5S_camera, 0x00C3CCF4);                 // its Gamebryo camera (+0x140 its frame)
GAME_VAR(void*, P5S_poiManager, 0x00C0F5E4);
GAME_VAR(void*, P5S_pedCheck, 0x009E4F4C);               // (0x415380's; 0xBCC580 is another)
inline void* const P5S_otherPedCheck = reinterpret_cast<void*>(0x00BCC580);
inline void* const P5S_navigation = reinterpret_cast<void*>(0x00C3CC68);   // 0x4F12F0: its mode
inline void* const P5S_spawnPoints = reinterpret_cast<void*>(0x00BF6228);
inline void* const P5S_triggerNames = reinterpret_cast<void*>(0x020C7E98);
inline void* const P5S_pickups = reinterpret_cast<void*>(0x00BCEB50);
inline void* const P5S_c2b300 = reinterpret_cast<void*>(0x00C2B300);
inline void* const P5S_c2ab70 = reinterpret_cast<void*>(0x00C2AB70);
// the five weapons peds go to pick up; 0xBF4344 the one given most
GAME_VAR(int32_t, P5S_weapon0, 0x00BF4158);
GAME_VAR(int32_t, P5S_weapon1, 0x00BF4154);
GAME_VAR(int32_t, P5S_weapon2, 0x00BF4150);
GAME_VAR(int32_t, P5S_weapon3, 0x00BF414C);
GAME_VAR(int32_t, P5S_weapon4, 0x00BF4148);
GAME_VAR(int32_t, P5S_commonWeapon, 0x00BF4344);
GAME_VAR(int32_t, P5S_noPartnerModel, 0x00BF42DC);
// the world's sectors (36 x 20, 0x14 bytes each: +0 the head of a list of entry nodes)
inline uint8_t* const P5S_sectors = reinterpret_cast<uint8_t*>(0x00C1B180);
GAME_VAR(uint8_t*, P5S_sectorNodes, 0x00C0F788);
GAME_VAR(int16_t, P5S_scanCode, 0x00C1AE88);             // CWorld::ms_nCurrentScanCode
GAME_VAR(uint8_t, P5S_inSectorScan, 0x00C1AE8A);
inline void* const P5S_colPoints = reinterpret_cast<void*>(0x00C216C0);
// 0x49D770 by its address (a callback for 0x440FB0)
inline void* const P5S_SpawnPointTestAddress = reinterpret_cast<void*>(0x0049D770);

GAME_FN(void*, __cdecl, P5S_Malloc, 0x005EEAA0, size_t size);
inline int __cdecl P5S_rand() { return crt::Rand(); }
GAME_FN(int, __cdecl, P5S_Random, 0x0040DA80, int min, int max);
// vectors
GAME_FN(CVector*, __cdecl, P5S_Sub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, P5S_Add, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, P5S_Normalise, 0x00414E10, CVector* v);
GAME_FN(float, __cdecl, P5S_Distance, 0x00470110, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, P5S_FindPlayerCentreOfWorld, 0x004469E0, uintptr_t);
// the camera's Gamebryo transform: 3x3 rotation, then the translation (+0x24)
GAME_FN(void*, __cdecl, P5S_CameraFrame, 0x006C82F0, void* camera);
GAME_FN(float*, __cdecl, P5S_FrameTransform, 0x006C9A90, void* frame);
// 0xC3CC68
GAME_FN(int, __thiscall, P5S_Navigation_GetMode, 0x004F12F0, void* self);
GAME_FN(int, __thiscall, P5S_Navigation_IsValid, 0x004F13D0, void* self, CVector* pos, float radius);
// triggers
GAME_FN(uint8_t, __thiscall, P5S_Trigger_Contains, 0x006D6FA0, uint8_t* trigger, CVector* pos);
GAME_FN(uint8_t, __thiscall, P5S_Trigger_Contains2D, 0x006D6EB0, uint8_t* trigger, float* pos);
GAME_FN(int, __thiscall, P5S_TriggerNames_Find, 0x006D7A10, void* self, const char* name);
// the world
GAME_FN(void, __cdecl, P5S_World_Add, 0x0045D6B0, uint8_t* entity);
GAME_FN(void, __cdecl, P5S_World_Remove, 0x0045DD70, uint8_t* entity);
GAME_FN(void, __cdecl, P5S_SetAreaCode, 0x00438840, int area, uint8_t* entity);
GAME_FN(void, __cdecl, P5S_FindObjectsInRange, 0x0045E3C0, CVector* centre, float radius, int, int16_t* count,
        int max, re::Ptr32<uint8_t>* found, int);
GAME_FN(uint8_t, __cdecl, P5S_ProcessLineOfSphere, 0x004616A0, CVector* from, CVector* to, float radius, void* colPoint,
        re::Ptr32<uint8_t>* hit, int, int, int);
GAME_FN(uint8_t*, __cdecl, P5S_GetEntityFromNode, 0x0044A3E0, int type, int index);
GAME_FN(int, __cdecl, P5S_ProcessColModels, 0x0056C880, void* matrixA, void* colA, void* matrixB, void* colB,
        void* colPoints, void*, void*);
GAME_FN(uint8_t, __cdecl, P5S_FindPathNear, 0x005EB7D0, CVector* pos, int, uintptr_t);
GAME_FN(float, __cdecl, P5S_FindGroundZ, 0x004636B0, float x, float y, float z, uint8_t* found, int);
// entities
GAME_FN(uint8_t*, __cdecl, P5S_GetColModel, 0x0051AF50, uint8_t* entity);
GAME_FN(float, __thiscall, P5S_Entity_GetBoundRadius, 0x00465E10, uint8_t* entity);
GAME_FN(void, __thiscall, P5S_Entity_GetBoundCentre, 0x00466CA0, uint8_t* entity, CVector* out);
GAME_FN(uint8_t, __thiscall, P5S_Entity_IsTouchingSphere, 0x004673B0, uint8_t* entity, CVector* centre, float radius);
GAME_FN(void*, __thiscall, P5S_Entity_GetMatrix, 0x0046DD20, uint8_t* entity);
GAME_FN(uint8_t, __thiscall, P5S_Entity_IsVisible, 0x00467460, uint8_t* entity);
GAME_FN(CVector*, __thiscall, P5S_Entity_GetPosition, 0x00415460, uint8_t* entity);
GAME_FN(void, __thiscall, P5S_Entity_SetPosition, 0x0041C4E0, uint8_t* entity, float x, float y, float z);
GAME_FN(void, __thiscall, P5S_Entity_SetOrientation, 0x0046A690, void* entity, float x, float y, float z);
GAME_FN(void, __thiscall, P5S_Entity_SetMatrix, 0x0046DE90, uint8_t* entity, void* matrix, int);
GAME_FN(void, __thiscall, P5S_Entity_443AA0, 0x00443AA0, uint8_t* entity);
GAME_FN(void, __thiscall, P5S_Entity_465760, 0x00465760, uint8_t* entity);
GAME_FN(uint8_t, __cdecl, P5S_IsEntityPointerValid, 0x004657F0, uint8_t* entity);
GAME_FN(void, __cdecl, P5S_CleanUpOldReference, 0x004657E0, uint8_t* entity, re::Ptr32<uint8_t>* ref);
GAME_FN(int, __cdecl, P5S_GetAlpha, 0x005F2B10, void* node);
GAME_FN(void, __cdecl, P5S_SetAlpha, 0x005F2B00, void* node, int alpha);
// objects and dummies
GAME_FN(void*, __cdecl, P5S_DummyPool_New, 0x004653A0, size_t size);
GAME_FN(uint8_t*, __thiscall, P5S_DummyObject_Construct, 0x004CF220, void* self, uint8_t* object);
GAME_FN(void*, __cdecl, P5S_ObjectPool_New, 0x004CFCD0, size_t size);
GAME_FN(uint8_t*, __thiscall, P5S_Object_ConstructFromDummy, 0x004D05E0, void* self, uint8_t* dummy);
GAME_FN(uint8_t, __thiscall, P5S_Object_CanBeDummy, 0x004CF2B0, uint8_t* object);
GAME_FN(void, __cdecl, P5S_Object_5B34B0, 0x005B34B0, uint8_t* object);
GAME_FN(void, __thiscall, P5S_Object_ResetHeld, 0x004D9410, uint8_t* object);
GAME_FN(void, __thiscall, P5S_Object_4DA190, 0x004DA190, uint8_t* object, int);
GAME_FN(int, __thiscall, P5S_Pool_ChainSize, 0x00435BB0, CPool<uint8_t>* pool);
GAME_FN(uint8_t*, __thiscall, P5S_Pool_ChainAt, 0x00435BD0, CPool<uint8_t>* pool, int slot, int);
// pickups (0xBCEB50)
GAME_FN(int16_t, __thiscall, P5S_Pickups_Find, 0x004263C0, void* self, int id);
GAME_FN(uint8_t, __thiscall, P5S_Pickups_426340, 0x00426340, void* self, int index);
GAME_FN(uint8_t, __thiscall, P5S_Pickups_426360, 0x00426360, void* self, int index);
GAME_FN(uint8_t, __thiscall, P5S_Pickups_426380, 0x00426380, void* self, int index);
GAME_FN(uint8_t, __thiscall, P5S_Pickups_4263A0, 0x004263A0, void* self, int index);
GAME_FN(void, __thiscall, P5S_Pickups_426410, 0x00426410, void* self, int index, int value);
GAME_FN(int, __cdecl, P5S_Pickup_445790, 0x00445790, int index, uint8_t* dummy);
GAME_FN(int, __cdecl, P5S_FindPickupAt, 0x00443FE0, float x, float y, float z, int model, int, uintptr_t, uintptr_t, int);
// POIs (0xC0F5E4)
GAME_FN(uint32_t, __thiscall, P5S_POIs_GetNumGroups, 0x004484B0, void* self);
GAME_FN(uint8_t*, __thiscall, P5S_POIs_GetGroup, 0x00448AD0, void* self, uint32_t index);
GAME_FN(uint8_t, __thiscall, P5S_POIs_IsOutOfRange_x64, 0x004487E0, void* self, uint8_t* poi, float, float, float);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t P5S_POIs_IsOutOfRange(void* a0, uint8_t* a1, CVector a2) {
    return P5S_POIs_IsOutOfRange_x64(a0, a1, a2.x, a2.y, a2.z);
}
GAME_FN(uint8_t, __thiscall, P5S_POIs_IsInRange_x64, 0x00448780, void* self, uint8_t* poi, float, float, float);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t P5S_POIs_IsInRange(void* a0, uint8_t* a1, CVector a2) {
    return P5S_POIs_IsInRange_x64(a0, a1, a2.x, a2.y, a2.z);
}
GAME_FN(uint8_t*, __thiscall, P5S_POIGroup_First, 0x00447B90, uint8_t* group);
GAME_FN(uint8_t*, __thiscall, P5S_POIGroup_Next, 0x00447BD0, uint8_t* group);
GAME_FN(void, __thiscall, P5S_POI_SetActive, 0x004478F0, uint8_t* poi, int active);
GAME_FN(void, __thiscall, P5S_POI_SetGroup, 0x00447870, uint8_t* poi, uint8_t* group);
GAME_FN(void*, __cdecl, P5S_POIGroupPool_New, 0x0048A100, size_t size);
GAME_FN(uint8_t*, __thiscall, P5S_POIGroup_Construct, 0x0048A080, void* self, CVector* pos, float heading, int,
        int, int, float);
// spawn points (0xBF6228)
GAME_FN(void, __thiscall, P5S_SpawnPoints_Collect, 0x00440E10, void* self, float x, float y, float z, float far_,
        float near_, float range, int, int max, void* points, int32_t* count);
GAME_FN(uint8_t, __thiscall, P5S_SpawnPoints_Find_x64, 0x00440FB0, void* self, uint32_t, uint32_t, uint32_t, uint32_t, int, uintptr_t, int32_t*, void* points, int count, void* test, float* context, int, uintptr_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t P5S_SpawnPoints_Find(void* a0, float a1, float a2, float a3, float a4, int a5, uintptr_t a6, int32_t* a7, void* a8, int a9, void* a10, float* a11, int a12, uintptr_t a13) {
    return P5S_SpawnPoints_Find_x64(a0, re::BitsOf(a1), re::BitsOf(a2), re::BitsOf(a3), re::BitsOf(a4), a5, a6, a7, a8, a9, a10, a11, a12, a13);
}
GAME_FN(float, __cdecl, P5S_SpawnPoints_Beyond, 0x0041A4D0, float* context, CVector* pos, float radius);
// peds
GAME_FN(void*, __cdecl, P5S_PedPool_New, 0x004789E0, size_t size);
GAME_FN(uint8_t*, __thiscall, P5S_CivilianPed_Construct, 0x0046DFF0, void* self, int model, int, int);
GAME_FN(uint8_t, __thiscall, P5S_Ped_IsAvailable, 0x004765C0, uint8_t* ped);   // (not in a vehicle, not the player)
GAME_FN(uint8_t*, __thiscall, P5S_Ped_GetModelInfo, 0x00477DC0, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, P5S_Ped_473220, 0x00473220, uint8_t* ped);
GAME_FN(void, __thiscall, P5S_Ped_4732A0, 0x004732A0, uint8_t* ped, int, int);
GAME_FN(void, __thiscall, P5S_Ped_SetAttitude, 0x004731D0, uint8_t* ped, int pedType, int attitude);
GAME_FN(uint8_t, __thiscall, P5S_Ped_473F10, 0x00473F10, uint8_t* ped);
GAME_FN(CVector*, __thiscall, P5S_Ped_GetForward, 0x004736F0, uint8_t* ped, CVector* out);
GAME_FN(uint8_t*, __thiscall, P5S_Ped_GetIntelligence408, 0x00474BE0, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, P5S_Ped_474BF0, 0x00474BF0, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, P5S_Ped_4473E0, 0x004473E0, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, P5S_Ped_4CE690, 0x004CE690, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, P5S_Ped_41AA50, 0x0041AA50, uint8_t* ped);
GAME_FN(void, __thiscall, P5S_Ped_41EE20, 0x0041EE20, uint8_t* ped, CVector* pos);
GAME_FN(int, __thiscall, P5S_Ped2F0_485560, 0x00485560, uint8_t* part);
GAME_FN(void, __thiscall, P5S_Ped2F0_4860F0, 0x004860F0, uint8_t* part, uint8_t* other, int);
GAME_FN(void, __thiscall, P5S_PedB44_48F1D0, 0x0048F1D0, uint8_t* part, uint8_t* other, int, int, int);
GAME_FN(uint8_t, __thiscall, P5S_PedA64_48D470, 0x0048D470, uint8_t* part, uint8_t* player);
GAME_FN(uint8_t, __thiscall, P5S_C2B300_48AEF0, 0x0048AEF0, void* self, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, P5S_C2AB70_46EFB0, 0x0046EFB0, void* self, uint8_t* ped, CVector* pos, int);
GAME_FN(uint8_t, __thiscall, P5S_PedCheck, 0x00415380, void* self);
GAME_FN(void, __thiscall, P5S_Intelligence300_5F3830, 0x005F3830, uint8_t* part, uintptr_t);
GAME_FN(int*, __thiscall, P5S_ModelInfo_522380, 0x00522380, uint8_t* modelInfo, int* out);
GAME_FN(int16_t, __thiscall, P5S_ModelInfo_5228D0, 0x005228D0, uint8_t* modelInfo);
GAME_FN(uint32_t, __cdecl, P5S_HashName, 0x00576ED0, const char* name);
GAME_FN(uint8_t, __cdecl, P5S_IsHourBetween, 0x00425E40, int, int, int, int);
// objectives
GAME_FN(void, __thiscall, P5S_Queue_Clear, 0x00470350, uint8_t* queue);
GAME_FN(uint8_t*, __thiscall, P5S_Queue_GetCurrent, 0x00470340, uint8_t* queue);
GAME_FN(uint8_t, __thiscall, P5S_Queue_Append, 0x00471390, uint8_t* queue, void* objective);
GAME_FN(uint8_t*, __thiscall, P5S_Objective_GetTarget, 0x008B6120, uint8_t* objective);
GAME_FN(void*, __thiscall, P5S_AttackPed_Construct, 0x00470D60, void* self, uint8_t* ped, void* target, int);
// streaming
GAME_FN(uint8_t*, __cdecl, P5S_GetStreamingInfo, 0x0052DD30, int model);   // +4 the load state (1 loaded, 2 requested)
GAME_FN(void, __cdecl, P5S_RequestModel, 0x00532020, int model, int flags);
// our own
GAME_FN(void*, __thiscall, P5S_PickupObjective_Construct, 0x0049C0C0, void* self, void* ped, int pickup);
GAME_FN(int, __thiscall, P5S_IsPlaceValid, 0x0049C930, CPedPopulation* self, CVector* pos);
GAME_FN(uint8_t, __thiscall, P5S_IsInSpawnRange, 0x0049D040, CPedPopulation* self, CVector* pos, int, int, float);
GAME_FN(uint8_t, __thiscall, P5S_IsInTrigger, 0x0049D340, CPedPopulation* self, CVector* pos, uint8_t* trigger);
GAME_FN(void, __thiscall, P5S_ConvertObjectToDummy, 0x0049D390, CPedPopulation* self, uint8_t* object);
GAME_FN(uint8_t, __thiscall, P5S_IsObjectPlaceClear, 0x0049D420, CPedPopulation* self, uint8_t* object);
GAME_FN(uint8_t, __thiscall, P5S_PickUpWeapon, 0x0049D950, CPedPopulation* self, uint8_t* ped, int);
GAME_FN(uint8_t, __thiscall, P5S_IsPlaceFreeForPed, 0x0049E2C0, CPedPopulation* self, CVector* pos, uint8_t* ignore);
GAME_FN(uint8_t, __thiscall, P5S_IsEntityFree, 0x0049E4E0, CPedPopulation* self, uint8_t* entity);
GAME_FN(uint8_t, __thiscall, P5S_ShouldArmPed, 0x0049E7D0, CPedPopulation* self, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, P5S_FindPed, 0x0049E860, CPedPopulation* self, int, int, int, uint8_t*, float, int);
GAME_FN(int, __thiscall, P5S_CountPedsWithName, 0x0049E960, CPedPopulation* self, uint32_t hash, int max);
GAME_FN(int, __thiscall, P5S_CanAddPed, 0x0049E9E0, CPedPopulation* self);
GAME_FN(uint8_t, __thiscall, P5S_IsPedKept, 0x0049EA30, CPedPopulation* self, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, P5S_RemoveFarPed, 0x0049EB80, CPedPopulation* self, int force);
GAME_FN(int, __thiscall, P5S_GetTotalNumPeds, 0x0049C2F0, CPedPopulation* self);
GAME_FN(uint8_t, __thiscall, P5S_GiveWeapon, 0x0049C280, CPedPopulation* self, uint8_t* ped, int weapon);
GAME_FN(int, __thiscall, P5S_ModelList_ChooseModel, 0x0049F0C0, PedModelList* self, int arg);
GAME_FN(uint8_t, __thiscall, P5S_Area_IsInTriggerForPedType, 0x0049DE10, CAreaPopulationInfo* self, int pedType,
        CVector* pos);
GAME_FN(void, __thiscall, P5S_Area_DeteriorateAttitudes, 0x0049CAF0, CAreaPopulationInfo* self, uint8_t* ped);

// vf0 of the game's classes: the deleting destructor
void DeleteObject(void* object) {
    using Fn = void*(__thiscall*)(void*, int);
    re::GameVirtual<Fn>(object, 0)(object, 1);
}

// |(x, y)| as the game works it out (the sum of squares stored as a float)
float Length2D(const CVector& v) {
    float squared = static_cast<float>(static_cast<double>(v.x) * v.x + static_cast<double>(v.y) * v.y);
    return static_cast<float>(std::sqrt(std::fabs(static_cast<double>(squared))));
}

float Length3D(const CVector& v) {
    float squared = static_cast<float>(static_cast<double>(v.x) * v.x + static_cast<double>(v.y) * v.y +
                                       static_cast<double>(v.z) * v.z);
    return static_cast<float>(std::sqrt(std::fabs(static_cast<double>(squared))));
}

float Dot(const CVector& a, const CVector& b) {
    return static_cast<float>(static_cast<double>(a.y) * b.y + static_cast<double>(b.x) * a.x +
                              static_cast<double>(a.z) * b.z);
}

float* CameraTransform() {
    return P5S_FrameTransform(P5S_CameraFrame(P5S_camera));
}

// The world sector a coordinate is in, clamped (0 .. 35)
int SectorLow(double value) {
    float sector = static_cast<float>(static_cast<int>(value / 50.0 + 18.0));
    if (sector <= 0.0f) {
        sector = 0.0f;
    }
    return static_cast<int>(sector);
}
int SectorHigh(double value) {
    float sector = static_cast<float>(static_cast<int>(value / 50.0 + 18.0));
    if (!(sector < 35.0f)) {
        sector = 35.0f;
    }
    return static_cast<int>(sector);
}

uint8_t* PoolAt(CPool<uint8_t>* pool, int slot) {
    return pool->objects + pool->elemSize * slot;
}

// A CColPoint as the game sets it up on the stack for 0x4616A0
struct ColPointLocal {
    CVector pos;         // +0x00
    re::Ptr32<uint8_t> entity;     // +0x0C (registered by the callee)
    float normal[3];     // +0x10
    uint32_t unknown1C;  // +0x1C (left uninitialised by the game)
    uint32_t unknown20;  // +0x20 (left uninitialised by the game)
    float distance;      // +0x24
};
static_assert(sizeof(ColPointLocal) == 0x28);
} // namespace

// ---------------------------------------------------------------------------------
// Where peds may be

int CPedPopulation::IsPlaceValid(CVector* pos) {
    CVector at = *pos;
    if (P5S_Navigation_GetMode(P5S_navigation) == 0 && P5S_Navigation_IsValid(P5S_navigation, &at, 2.0f) != 0) {
        return 1;
    }
    return 0;
}

void CPedPopulation::UpdateCullDistances() {
    if (P5S_currArea != 0) {
        for (int i = 0; i < 6; i++) {
            cull[i] = cullInterior[i];
        }
        return;
    }
    for (int i = 0; i < 6; i++) {
        cull[i] = cullExterior[i];
    }
    uint8_t* vehicle = At<uint8_t*>(P5S_player, 0x1554);
    if (!vehicle) {
        return;
    }
    CVector speed = {At<float>(vehicle, 0x124), At<float>(vehicle, 0x128), 0.0f};
    float stretch = Length2D(speed);
    if (stretch < 0.0f) {
        stretch = 0.0f;
    } else if (0.7f < stretch) {
        stretch = 0.7f;
    }
    for (int i = 0; i < 6; i++) {
        cull[i] = cullExterior[i] * stretch + cullExterior[i];
    }
}

uint8_t CPedPopulation::IsInSpawnRange(CVector* pos, int outOfRange, int useMin, float minDistance) {
    CVector focus = *P5S_FindPlayerCentreOfWorld(0);
    CVector offset;
    P5S_Sub(&offset, pos, &focus);
    float distance = Length2D(offset);
    float* camera = CameraTransform();
    CVector cameraPos = {camera[9], camera[10], camera[11]};
    camera = CameraTransform();
    CVector look = {camera[2], camera[5], camera[8]};
    P5S_Normalise(&look);
    CVector toPlace;
    P5S_Sub(&toPlace, pos, &cameraPos);
    P5S_Normalise(&toPlace);
    int mode = P5S_Navigation_GetMode(P5S_navigation);
    bool inView = false;
    if (P5S_IsPlaceValid(this, pos) && Dot(toPlace, look) > -0.3f) {
        inView = true;
    }
    float height = std::fabs(offset.z);

    if (re::Flag(outOfRange)) {
        // (the remaster's second pass, runflags.h: its peds are made out of the doorway's view as far
        // as cull[3]; kept as far as one in view is, so one a step further isn't gone at once)
        float limit = inView || re::PopulationPass() == 2 ? cull[0] : cull[3];
        if (limit < distance) {
            return 1;
        }
        if (P5S_currArea == 0) {
            return 0;
        }
        return static_cast<double>(interiorHeight) + 4.0 < height ? 1 : 0;
    }
    if (mode != 0) {
        if (P5S_Navigation_GetMode(P5S_navigation) == 2 && static_cast<double>(cull[1]) * 0.5 > distance) {
            return 0;
        }
        // (the global population's, not this one's)
        if (P5S_population->cull[3] < distance) {
            return 0;
        }
    } else if (re::Flag(useMin)) {
        if (minDistance > distance) {
            return 0;
        }
        if (inView ? cull[0] < distance : cull[3] < distance) {
            return 0;
        }
    } else if (inView) {
        if (cull[1] > distance || cull[0] < distance) {
            return 0;
        }
    } else {
        if (cull[4] > distance || cull[3] < distance) {
            return 0;
        }
    }
    if (P5S_currArea != 0 && interiorHeight < height) {
        return 0;
    }
    return 1;
}

uint8_t CPedPopulation::IsInTrigger(CVector* pos, uint8_t* trigger) {
    if (!trigger) {
        return 0;
    }
    if (flags & 0x20) {
        return P5S_Trigger_Contains(trigger, pos);
    }
    float pos2D[2] = {pos->x, pos->y};
    return P5S_Trigger_Contains2D(trigger, pos2D);
}

// ---------------------------------------------------------------------------------
// Objects and dummies

void CPedPopulation::ConvertObjectToDummy(uint8_t* object) {
    void* memory = P5S_DummyPool_New(0x128);
    uint8_t* dummy = memory ? P5S_DummyObject_Construct(memory, object) : nullptr;
    P5S_Entity_SetMatrix(dummy, object + 0x1C4, 0);
    P5S_Entity_443AA0(dummy);
    P5S_Entity_465760(dummy);
    uint8_t area = object[0x110];
    P5S_World_Remove(object);
    DeleteObject(object);
    dummy[0x110] = area;
    if (area != 0xFF) {
        P5S_SetAreaCode(area, dummy);
    }
    P5S_World_Add(dummy);
}

uint8_t CPedPopulation::IsObjectPlaceClear(uint8_t* object) {
    CVector at = At<CVector>(object, 0x1F4);
    float radius = At<float>(P5S_GetColModel(object), 0xC);
    int16_t count;
    P5S_FindObjectsInRange(&at, radius, 0, &count, 2, nullptr, 6);
    return count == 0;
}

void CPedPopulation::CullObjects(CVector centre) {
    CPool<uint8_t>* pool = P5S_objectPool;
    int size = pool->size;
    if (pool->Next()) {
        size += P5S_Pool_ChainSize(pool->Next());
    }
    int frame = static_cast<int>(P5S_frameCounter & 0x1F);
    int first = size * frame / 32;
    int end = (frame + 1) * size / 32;
    for (int slot = first; slot < end; slot++) {
        uint8_t* object = nullptr;
        if (slot < pool->size) {
            object = PoolAt(pool, slot);
        } else if (pool->Next()) {
            object = P5S_Pool_ChainAt(pool->Next(), slot - pool->size, 1);
        }
        // (the flags read for slots past this pool too)
        if ((pool->flags[slot] & 0x80) != 0 || !object || !P5S_Object_CanBeDummy(object)) {
            continue;
        }
        uint8_t kind = object[0x208];
        if (kind == 3) {
            CVector offset;
            float distance = Length2D(*P5S_Sub(&offset, PosOf(object), &centre));
            if (cull[0] < distance || (cull[3] < distance && !P5S_Entity_IsVisible(object))) {
                P5S_World_Remove(object);
                DeleteObject(object);
            }
        } else if (kind != 4) {
            CVector offset;
            if (Length2D(*P5S_Sub(&offset, PosOf(object), &centre)) > 80.0f &&
                Length2D(*P5S_Sub(&offset, reinterpret_cast<CVector*>(object + 0x1F4), &centre)) > 80.0f &&
                P5S_IsObjectPlaceClear(this, object)) {
                P5S_ConvertObjectToDummy(this, object);
            }
        }
    }
}

void CPedPopulation::ConvertAllObjectsToDummies() {
    CPool<uint8_t>* pool = P5S_objectPool;
    int slot = pool->size;
    if (pool->Next()) {
        slot += P5S_Pool_ChainSize(pool->Next());
    }
    while (slot != 0) {
        slot--;
        uint8_t* object = nullptr;
        if (slot < pool->size) {
            object = PoolAt(pool, slot);
        } else if (pool->Next()) {
            object = P5S_Pool_ChainAt(pool->Next(), slot - pool->size, 1);
        }
        if ((pool->flags[slot] & 0x80) == 0 && object) {
            if (P5S_Object_CanBeDummy(object)) {
                P5S_ConvertObjectToDummy(this, object);
            }
            pool = P5S_objectPool;
        }
    }
}

uint8_t CPedPopulation::IsEntityFree(uint8_t* entity) {
    uint8_t* colModel = At<uint8_t*>(P5S_modelInfoPtrs[At<int16_t>(entity, 0x10E)], 0xC);
    float bound = At<float>(colModel, 0xC);
    int minX = SectorLow(static_cast<double>(PosOf(entity)->x) - bound);
    int minY = SectorLow(static_cast<double>(PosOf(entity)->y) - bound);
    int maxX = SectorHigh(static_cast<double>(bound) + PosOf(entity)->x);
    int maxY = SectorHigh(static_cast<double>(PosOf(entity)->y) + bound);
    // (the centre starts as the 0 left on the x87 stack)
    CVector centre = {0.0f, 0.0f, 0.0f};
    float radius = P5S_Entity_GetBoundRadius(entity);
    P5S_Entity_GetBoundCentre(entity, &centre);
    P5S_inSectorScan = 1;
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            uint32_t* node = *reinterpret_cast<re::Ptr32<uint32_t>*>(P5S_sectors + (y * 36 + x) * 0x14);
            while (node) {
                uint32_t value = *node;
                uint8_t* other = P5S_GetEntityFromNode(value & 0xF, (value >> 4) & 0x3FFF);
                node = (value >> 18) == 0x3FFF ? nullptr
                                               : reinterpret_cast<uint32_t*>(P5S_sectorNodes + (value >> 18) * 4);
                if (At<int16_t>(other, 0x10A) == P5S_scanCode || !P5S_Entity_IsTouchingSphere(other, &centre, radius)) {
                    continue;
                }
                At<int16_t>(other, 0x10A) = P5S_scanCode;
                uint8_t* otherCol = At<uint8_t*>(P5S_modelInfoPtrs[At<int16_t>(other, 0x10E)], 0xC);
                void* otherMatrix = P5S_Entity_GetMatrix(other);
                void* matrix = P5S_Entity_GetMatrix(entity);
                if (P5S_ProcessColModels(matrix, colModel, otherMatrix, otherCol, P5S_colPoints, nullptr, nullptr) > 0) {
                    P5S_inSectorScan = 0;
                    return 0;
                }
            }
        }
    }
    P5S_inSectorScan = 0;
    return 1;
}

void CPedPopulation::ConvertDummyToObject(uint8_t* dummy) {
    if (!P5S_IsEntityFree(this, dummy)) {
        return;
    }
    uint8_t fromPickup = 0;
    int pickupId = At<int32_t>(dummy, 0x124);
    if (pickupId != 0) {
        int16_t index = P5S_Pickups_Find(P5S_pickups, pickupId);
        if (index >= 0) {
            if (!P5S_Pickups_426360(P5S_pickups, index) && !P5S_Pickups_426380(P5S_pickups, index) &&
                !P5S_Pickups_4263A0(P5S_pickups, index)) {
                P5S_Pickups_426410(P5S_pickups, index, P5S_Pickup_445790(index, dummy));
                P5S_World_Remove(dummy);
                DeleteObject(dummy);
                return;
            }
            if (P5S_Pickups_426340(P5S_pickups, index)) {
                P5S_World_Remove(dummy);
                DeleteObject(dummy);
                return;
            }
            fromPickup = 1;
        }
    }
    void* memory = P5S_ObjectPool_New(0x22C);
    if (!memory) {
        return;
    }
    uint8_t* object = P5S_Object_ConstructFromDummy(memory, dummy);
    if (!object) {
        return;
    }
    uint8_t area = dummy[0x110];
    P5S_World_Remove(dummy);
    DeleteObject(dummy);
    object[0x110] = area;
    if (area != 0xFF) {
        P5S_SetAreaCode(area, object);
    }
    P5S_World_Add(object);
    if (fromPickup) {
        object[0x209] |= 0x10;
    }
    P5S_Object_5B34B0(object);
}

// ---------------------------------------------------------------------------------
// POIs and spawn points

void CPedPopulation::UpdatePOIs() {
    CVector at = *PosOf(P5S_player);
    for (uint32_t i = 0; i < P5S_POIs_GetNumGroups(P5S_poiManager); i++) {
        uint8_t* group = P5S_POIs_GetGroup(P5S_poiManager, i);
        if (!group) {
            continue;
        }
        for (uint8_t* poi = P5S_POIGroup_First(group); poi;) {
            uint8_t* next = P5S_POIGroup_Next(group);
            if (At<int32_t>(poi, 0x6C) != 0 && P5S_POIs_IsOutOfRange(P5S_poiManager, poi, at)) {
                P5S_POI_SetActive(poi, 0);
            }
            poi = next;
        }
    }
    for (uint32_t i = 0; i < P5S_POIs_GetNumGroups(P5S_poiManager); i++) {
        uint8_t* group = P5S_POIs_GetGroup(P5S_poiManager, i);
        if (!group) {
            continue;
        }
        for (uint8_t* poi = P5S_POIGroup_First(group); poi;) {
            uint8_t* next = P5S_POIGroup_Next(group);
            int type = At<int32_t>(poi, 0x70);
            if ((type == 2 || type == 1) && P5S_POIs_IsInRange(P5S_poiManager, poi, at)) {
                CVector poiPos = *PosOf(poi);
                if (P5S_FindPathNear(&poiPos, 1, 0)) {
                    float value = 0.0f;
                    if (At<int32_t>(poi, 0x70) == 1 && 0.0f < At<float>(poi, 0x9C)) {
                        value = At<float>(poi, 0x9C);
                    }
                    void* memory = P5S_POIGroupPool_New(0x110);
                    uint8_t* poiGroup = nullptr;
                    if (memory) {
                        uint8_t* matrix = At<uint8_t*>(poi, 0x14);
                        float heading = matrix ? static_cast<float>(std::atan2(-static_cast<double>(At<float>(matrix, 0x10)),
                                                                               static_cast<double>(At<float>(matrix, 0x14))))
                                               : At<float>(poi, 0x10);
                        poiGroup = P5S_POIGroup_Construct(memory, PosOf(poi), heading, At<int8_t>(poi, 0x81),
                                                          At<int32_t>(poi, 0x78), At<int32_t>(poi, 0x74), value);
                    }
                    // (no check against the 60 there's room for, nor for a failed allocation)
                    poiGroups.groups[poiGroups.count] = poiGroup;
                    poiGroups.count++;
                    At<uint8_t*>(poiGroup, 0x40) = poi;
                    P5S_POI_SetGroup(poi, poiGroup);
                    P5S_POI_SetActive(poi, 1);
                }
            }
            poi = next;
        }
    }
}
// 0x49E7C0: a lone jmp to UpdatePOIs
RE_UNHOOKED_FN("CPedPopulation::UpdatePOIs (thunk)", 0x0049E7C0, &CPedPopulation::UpdatePOIs);

namespace PedPopulation {
// (--doors, the remaster's second population pass: what its spawn point tests found, once a second)
void NoteSecondPassTest(bool seen, float distance, uint8_t result) {
    static int calls = 0, seenCount = 0, passed = 0;
    static float nearest = 1e9f, farthest = 0.0f;
    static uint32_t said = 0;
    calls++;
    seenCount += seen;
    passed += result;
    nearest = distance < nearest ? distance : nearest;
    farthest = distance > farthest ? distance : farthest;
    uint32_t now = *reinterpret_cast<uint32_t*>(0x00C1A9B4);
    if (now - said >= 1000) {
        said = now;
        std::fprintf(stderr, "doors:   second pass spawn tests: %d (%d seen through the doorway), %d passed, %.1f to %.1f m away\n", calls,
                     seenCount, passed, nearest, farthest);
        calls = seenCount = passed = 0;
        nearest = 1e9f;
        farthest = 0.0f;
    }
}

uint8_t __cdecl SpawnPointTest(CVector* point, float* context) {
    CVector at = *point;
    CVector place = *point;
    uint8_t result;
    bool seen = P5S_IsPlaceValid(P5S_population, &at) != 0;
    if (seen) {
        result = P5S_SpawnPoints_Beyond(context, &place, context[3]) < 0.0f        ? 0
                 : P5S_SpawnPoints_Beyond(context, &place, context[4]) <= 0.0f ? 1
                                                                                 : 0;
    } else {
        result = P5S_SpawnPoints_Beyond(context, &place, context[5]) < 0.0f        ? 0
                 : P5S_SpawnPoints_Beyond(context, &place, context[6]) <= 0.0f ? 1
                                                                                 : 0;
    }
    if (re::PopulationPass() == 2 && re::DoorReport()) {
        float dx = point->x - context[0], dy = point->y - context[1], dz = point->z - context[2];
        NoteSecondPassTest(seen, std::sqrt(dx * dx + dy * dy + dz * dz), result);
    }
    return result;
}
} // namespace PedPopulation

uint8_t CPedPopulation::FindSpawnPoint(CVector* pos, int a2, int a3, int a4, float minFront, float minBehind) {
    // the context for 0x49D770: the place, the near distances (at least the global
    // population's) in and out of view, then its [4] and [5]
    float context[7] = {pos->x, pos->y, pos->z};
    CPedPopulation* population = P5S_population;
    context[3] = minFront < population->cull[1] ? population->cull[1] : minFront;
    context[4] = minBehind < population->cull[2] ? population->cull[2] : minBehind;
    context[5] = population->cull[4];
    // (the remaster's second pass, runflags.h: out of the doorway's view as far as such a ped is
    // kept, cull[3], not only cull[5]: what is near the doorway there is mostly in its view)
    context[6] = re::PopulationPass() == 2 ? population->cull[3] : population->cull[5];
    return P5S_SpawnPoints_Find(P5S_spawnPoints, pos->x, pos->y, pos->z, context[4], unknown7878, a2, &unknown7894,
                                spawnPoints, numSpawnPoints, P5S_SpawnPointTestAddress, context, a3, a4) != 0;
}

uint8_t CPedPopulation::UpdateSpawnPoints() {
    CVector focus = *P5S_FindPlayerCentreOfWorld(0);
    CVector moved;
    if (Length2D(*P5S_Sub(&moved, &focus, &lastFocus)) < 10.0f) {
        return 0;
    }
    lastFocus = focus;
    P5S_SpawnPoints_Collect(P5S_spawnPoints, focus.x, focus.y, focus.z, cull[0], cull[4], 100.0f, 1, 40, spawnPoints,
                            &numSpawnPoints);
    return 1;
}

// ---------------------------------------------------------------------------------
// Peds

uint8_t CPedPopulation::PickUpWeapon(uint8_t* ped, int giveObjective) {
    if (!ped || At<int32_t>(ped, 0x1368) == 0) {
        return 0;
    }
    uint8_t* weapon = At<uint8_t*>(ped, 0x1D0);
    int16_t model = At<int16_t>(weapon, 0x10E);
    if (model <= 0) {
        return 0;
    }
    uint8_t* anim = At<uint8_t*>(weapon, 0x114);
    if (!anim || At<int32_t>(anim, 0x94) == 0) {
        return 0;
    }
    CVector* at = PosOf(weapon);
    int pickup = P5S_FindPickupAt(at->x, at->y, at->z, model, 1, 0, 0, 0);
    if (re::Flag(giveObjective)) {
        uint8_t* queue = ped + 0x5F0;
        P5S_Queue_Clear(queue);
        void* objective;
        if (pickup >= 0) {
            void* memory = P5S_Malloc(0x14);
            objective = memory ? P5S_PickupObjective_Construct(memory, ped, pickup) : nullptr;
        } else {
            P5S_Object_4DA190(At<uint8_t*>(ped, 0x1D0), 0);
            void* memory = P5S_Malloc(0x1C);
            objective = memory ? P5S_AttackPed_Construct(memory, ped, At<void*>(P5S_Ped_GetIntelligence408(ped), 4), 1)
                               : nullptr;
        }
        P5S_Queue_Append(queue, objective);
    }
    P5S_Object_ResetHeld(At<uint8_t*>(ped, 0x1D0));
    uint8_t* intelligence = At<uint8_t*>(ped, 0x2E0);
    if (intelligence[0x33C]) {
        P5S_Intelligence300_5F3830(intelligence + 0x300, At<int32_t>(intelligence, 0x4B0));
        intelligence[0x33C] = 0;
    }
    return 1;
}

uint8_t CPedPopulation::PickUpSpecialWeapon(uint8_t* ped) {
    if (!ped || !P5S_Ped_IsAvailable(ped) || At<int32_t>(ped, 0x1368) == 0) {
        return 0;
    }
    int model = At<int16_t>(At<uint8_t*>(ped, 0x1D0), 0x10E);
    if (model == P5S_weapon0 || model == P5S_weapon1 || model == P5S_weapon2 || model == P5S_weapon3 ||
        model == P5S_weapon4) {
        return P5S_PickUpWeapon(this, ped, 1);
    }
    return 0;
}

uint8_t CPedPopulation::IsPlaceFreeForPed(CVector* pos, uint8_t* ignore) {
    nearEntities[0] = nullptr;
    nearEntities[1] = nullptr;
    // (a short count in a zeroed int, which the game then adjusts as an int)
    int32_t count = 0;
    P5S_FindObjectsInRange(pos, 0.3f, 1, reinterpret_cast<int16_t*>(&count), 2, nearEntities, 6);
    if (ignore) {
        if (nearEntities[0] == ignore) {
            count--;
        }
        if (nearEntities[1] == ignore) {
            count--;
        }
    }
    if (static_cast<int16_t>(count) == 0) {
        // (a sphere down from 2.55 to 0.55: heights, not offsets from the place)
        CVector from = {pos->x, pos->y, 2.55f};
        CVector to = {pos->x, pos->y, 0.55f};
        ColPointLocal colPoint;
        colPoint.pos = {0.0f, 0.0f, 0.0f};
        colPoint.entity = nullptr;
        colPoint.normal[0] = 0.0f;
        colPoint.normal[1] = 0.0f;
        colPoint.normal[2] = 0.0f;
        colPoint.unknown1C = 0;
        colPoint.unknown20 = 0;
        colPoint.distance = FLT_MAX;
        re::Ptr32<uint8_t> hit;
        if (P5S_ProcessLineOfSphere(&from, &to, 0.3f, &colPoint, &hit, 9, 0xF, 1)) {
            count++;
        }
        if (colPoint.entity) {
            P5S_CleanUpOldReference(colPoint.entity, &colPoint.entity);
        }
    }
    return static_cast<int16_t>(count) == 0;
}

uint8_t CPedPopulation::IsPedInTriggerForItsType(uint8_t* ped) {
    if (!ped || At<int32_t>(ped, 0x1400) == 0) {
        return 0;
    }
    return P5S_Area_IsInTriggerForPedType(&areas[P5S_currArea], At<int32_t>(ped, 0x1310), PosOf(ped));
}

uint8_t CPedPopulation::ShouldArmPed(uint8_t* ped) {
    if (!ped) {
        return 0;
    }
    int index = P5S_TriggerNames_Find(P5S_triggerNames, "AMB_SCHOOL_AREA");
    uint8_t* trigger = nullptr;
    if (index != -1 && (P5S_triggerPool->flags[index] & 0x80) == 0) {
        trigger = PoolAt(P5S_triggerPool, index);
    }
    if (P5S_currArea != 2 && !P5S_IsInTrigger(this, PosOf(ped), trigger)) {
        return 0;
    }
    int type = At<int32_t>(ped, 0x1310);
    return type == 1 || type == 5 || type == 6;
}

uint8_t* CPedPopulation::FindPed(int pedType, int sex, int x3, uint8_t* near, float maxDistance, int notPlayer) {
    CPool<uint8_t>* pool = P5S_pedPool;
    for (int slot = pool->size; slot != 0;) {
        slot--;
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        uint8_t* ped = PoolAt(pool, slot);
        if (!ped || (near && ped == near)) {
            continue;
        }
        if (re::Flag(notPlayer) && At<int32_t>(ped, 0x1310) == 13) {
            continue;
        }
        if (pedType != -1 && At<int32_t>(ped, 0x1310) != pedType) {
            continue;
        }
        if (sex != -1) {
            uint8_t* modelInfo = P5S_Ped_GetModelInfo(ped);
            if (modelInfo[0x50] != 0 ? sex == 1 : sex == 2) {
                continue;
            }
        }
        if (x3 != -1) {
            int value;
            if (*P5S_ModelInfo_522380(P5S_Ped_GetModelInfo(ped), &value) != x3) {
                continue;
            }
        }
        if (!near) {
            return ped;
        }
        float distance = P5S_Distance(P5S_Entity_GetPosition(near), PosOf(ped));
        if (!(maxDistance < distance)) {
            return ped;
        }
    }
    return nullptr;
}

int CPedPopulation::CountPedsWithName(uint32_t hash, int max) {
    CPool<uint8_t>* pool = P5S_pedPool;
    int slot = pool->size;
    int count = 0;
    if (slot == 0) {
        return 0;
    }
    do {
        slot--;
        if ((pool->flags[slot] & 0x80) == 0) {
            uint8_t* ped = PoolAt(pool, slot);
            if (ped && ped[0x1C98] == 0) {
                uint8_t* modelInfo = P5S_Ped_GetModelInfo(ped);
                if (modelInfo && P5S_HashName(reinterpret_cast<const char*>(modelInfo + 0x98)) == hash) {
                    count++;
                }
            }
        }
    } while (count < max && slot != 0);
    return count;
}

int CPedPopulation::CanAddPed() {
    if (P5S_GetTotalNumPeds(this) >= current->total) {
        return 0;
    }
    CPool<uint8_t>* pool = P5S_pedPool;
    int size = pool->size;
    int used = 0;
    if (size > 0) {
        for (int slot = 0; slot < size; slot++) {
            if ((pool->flags[slot] & 0x80) == 0) {
                used++;
            }
        }
        if (used >= 19) {
            return 0;
        }
    }
    return 1;
}

uint8_t CPedPopulation::IsPedKept(uint8_t* ped) {
    if (!ped) {
        return 0;
    }
    if (ped[0x1C98] == 0) {
        uint8_t* objective = P5S_Queue_GetCurrent(ped + 0x5F0);
        if (!objective || At<int32_t>(objective, 8) != 7 || !P5S_Objective_GetTarget(objective) ||
            At<int32_t>(P5S_Objective_GetTarget(objective), 0x1310) != 13) {
            return 0;
        }
        // attacking the player
        if (P5S_C2B300_48AEF0(P5S_c2b300, ped)) {
            return 1;
        }
        float limit = cull[2];
        if (limit > P5S_Distance(PosOf(ped), PosOf(P5S_player))) {
            return 1;
        }
        return P5S_PedA64_48D470(ped + 0xA64, P5S_player) ? 1 : 0;
    }
    if (flags & 1) {
        return 0;
    }
    uint8_t* other = At<uint8_t*>(ped, 0x4F0 + At<int32_t>(ped, 0x504) * 4);
    if (!other || At<int32_t>(other, 0x1310) != 13) {
        return 0;
    }
    float limit = cull[3];
    return limit > P5S_Distance(PosOf(ped), PosOf(P5S_player)) ? 1 : 0;
}

uint8_t CPedPopulation::RemoveFarPed(int force) {
    float farthest = 0.0f;
    uint8_t* farthestPed = nullptr;
    uint8_t* behind = nullptr;
    CPool<uint8_t>* pool = P5S_pedPool;
    float* camera = CameraTransform();
    CVector cameraPos = {camera[9], camera[10], camera[11]};
    int slot = pool->size;
    camera = CameraTransform();
    CVector look = {camera[2], camera[5], camera[8]};
    P5S_Normalise(&look);
    while (slot != 0) {
        slot--;
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        uint8_t* ped = PoolAt(pool, slot);
        if (!ped || !P5S_Ped_IsAvailable(ped) || At<void*>(ped, 0x1554)) {
            continue;
        }
        // (the other side's: not this pass's to make room with, unless the pool is nearly full (force)
        // in the current area's pass: then they go first, the current area's peds and the scripts' come
        // before them)
        if (re::PopulationSkipsPed(ped) && !(re::Flag(force) && re::PopulationPass() == 1)) {
            continue;
        }
        uint8_t* lastVehicle = At<uint8_t*>(ped, 0x1560);
        if (lastVehicle && P5S_IsEntityPointerValid(lastVehicle)) {
            continue;
        }
        uint8_t* group = At<uint8_t*>(ped, 0x1D24);
        if (group) {
            int state = At<int32_t>(group, 0x70);
            if (state > 2 && state < 0x10) {
                continue;
            }
        }
        if (ped[0x1570] == 4 || P5S_IsPedKept(this, ped)) {
            continue;
        }
        CVector offset;
        float distance = Length3D(*P5S_Sub(&offset, &cameraPos, PosOf(ped)));
        CVector toPed;
        P5S_Sub(&toPed, PosOf(ped), &cameraPos);
        P5S_Normalise(&toPed);
        if (distance > 20.0f && Dot(toPed, look) < -0.3f) {
            behind = ped;
        }
        if (farthest < distance) {
            farthest = distance;
            farthestPed = ped;
        }
    }
    if (behind) {
        DeleteObject(behind);
        return 1;
    }
    if (re::Flag(force) && farthestPed) {
        DeleteObject(farthestPed);
        return 1;
    }
    return 0;
}

void CPedPopulation::RemovePedsInRadius(CVector centre, float radius) {
    float radiusSquared = radius * radius;
    CPool<uint8_t>* pool = P5S_pedPool;
    for (int slot = pool->size; slot != 0;) {
        slot--;
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        uint8_t* ped = PoolAt(pool, slot);
        if (!ped || !P5S_Ped_IsAvailable(ped)) {
            continue;
        }
        CVector offset;
        CVector* d = P5S_Sub(&offset, &centre, PosOf(ped));
        float squared = static_cast<float>(static_cast<double>(d->x) * d->x + static_cast<double>(d->y) * d->y);
        if (!(radiusSquared < squared)) {
            DeleteObject(ped);
        }
    }
}

void PlaceableByVector::SetOrientation(const CVector* v) {
    P5S_Entity_SetOrientation(this, v->x, v->y, v->z);
}

void CPedPopulation::CullPeds(CVector centre) {
    if (P5S_Navigation_GetMode(P5S_navigation) != 0) {
        return;
    }
    CPool<uint8_t>* pool = P5S_pedPool;
    for (int slot = pool->size; slot != 0;) {
        slot--;
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        uint8_t* ped = PoolAt(pool, slot);
        if (!ped || At<int32_t>(ped, 0x1310) == 13) {
            continue;
        }
        if (re::PopulationSkipsPed(ped)) {
            continue;   // (the remaster's two areas live: the other side's, its own pass's, runflags.h)
        }
        int alpha = P5S_GetAlpha(At<void*>(ped, 0x18));
        if (At<int32_t>(ped, 0x1338) != 0) {
            // fading out: gone when it's gone
            if (alpha == 0) {
                DeleteObject(ped);
                continue;
            }
        } else if (alpha != 0xFF) {
            // fading in (at once when near)
            if (alpha < 0xFF) {
                alpha += 0x10;
            }
            if (alpha > 0xFF) {
                alpha = 0xFF;
            } else {
                CVector offset;
                float distance = Length2D(*P5S_Sub(&offset, PosOf(ped), &centre));
                if (static_cast<double>(cull[1]) * 0.5 > distance) {
                    alpha = 0xFF;
                }
            }
            P5S_SetAlpha(At<void*>(ped, 0x18), alpha);
        }
        if (At<void*>(ped, 0x1554)) {
            continue;
        }
        uint8_t* lastVehicle = At<uint8_t*>(ped, 0x1560);
        if (lastVehicle && P5S_IsEntityPointerValid(lastVehicle)) {
            continue;
        }
        if (P5S_IsPedKept(this, ped)) {
            continue;
        }
        uint8_t* group = At<uint8_t*>(ped, 0x1D24);
        if (group) {
            int state = At<int32_t>(group, 0x70);
            if (state > 2 && state < 0x10 && !P5S_Ped_473220(ped)) {
                continue;
            }
        }
        if (!P5S_Ped_IsAvailable(ped) || ped[0x1570] == 4 || At<int32_t>(ped, 0x1B8) != 0) {
            continue;
        }
        if (!P5S_IsInSpawnRange(this, PosOf(ped), 1, 0, 10.0f)) {
            continue;
        }
        // out of range: faded out if seen, else gone
        if (P5S_Entity_IsVisible(ped)) {
            At<int32_t>(ped, 0x1338) = 1;
            continue;
        }
        DeleteObject(ped);
    }
}

uint8_t CPedPopulation::IsPedPoolFull() {
    CPool<uint8_t>* pool = P5S_pedPool;
    int size = pool->size;
    int used = 0;
    for (int slot = 0; slot < size; slot++) {
        if ((pool->flags[slot] & 0x80) == 0) {
            used++;
        }
    }
    if (size - used < 4 && !P5S_RemoveFarPed(this, 1)) {
        return 1;
    }
    return 0;
}

uint8_t* CPedPopulation::AddPed(int arg, int model) {
    if (!P5S_CanAddPed(this)) {
        return nullptr;
    }
    if (model == -1) {
        model = P5S_ModelList_ChooseModel(modelList, arg);
        if (model == -1) {
            return nullptr;
        }
    }
    if (P5S_GetStreamingInfo(model)[4] == 2) {
        return nullptr;
    }
    if (P5S_GetStreamingInfo(model)[4] != 1) {
        P5S_RequestModel(model, 0x10);
        return nullptr;
    }
    void* memory = P5S_PedPool_New(0x1E90);
    uint8_t* ped = memory ? P5S_CivilianPed_Construct(memory, model, 1, 0) : nullptr;
    P5S_Entity_SetOrientation(ped, 0.0f, 0.0f, 0.0f);
    P5S_World_Add(ped);
    return ped;
}

uint8_t* CPedPopulation::AddPedAt(int model, CVector* pos, int arg, float value) {
    if (P5S_GetStreamingInfo(model)[4] == 2) {
        return nullptr;
    }
    if (P5S_GetStreamingInfo(model)[4] != 1) {
        P5S_RequestModel(model, 0x10);
        return nullptr;
    }
    CVector at = *pos;
    uint8_t* modelInfo = P5S_modelInfoPtrs[model];
    int pedType = At<int32_t>(modelInfo, 0x4C);
    uint8_t inTrigger = P5S_Area_IsInTriggerForPedType(&areas[P5S_currArea], pedType, &at);
    if (inTrigger && typeBlockedUntil[pedType] > P5S_timeInMs) {
        return nullptr;
    }
    if (model == forcedModel) {
        forcedModel = -1;
        forcedPedType = -1;
    }
    if (P5S_ModelInfo_5228D0(modelInfo) == -1 || P5S_ModelInfo_5228D0(modelInfo) == 1) {
        // (a unique ped: not twice)
        if (P5S_CountPedsWithName(this, P5S_HashName(reinterpret_cast<const char*>(modelInfo + 0x98)), 1) != 0) {
            forcedModel = -1;
            forcedPedType = -1;
            unknown7888 = 0;
            return nullptr;
        }
    }
    void* memory = P5S_PedPool_New(0x1E90);
    uint8_t* ped = memory ? P5S_CivilianPed_Construct(memory, model, arg, 0) : nullptr;
    P5S_Entity_SetPosition(ped, at.x, at.y, at.z);
    P5S_Entity_SetOrientation(ped, 0.0f, 0.0f, 0.0f);
    At<float>(ped, 0x534) = value;
    P5S_World_Add(ped);
    At<int32_t>(ped, 0x1338) = 0;
    P5S_SetAlpha(At<void*>(ped, 0x18), 1);
    if (P5S_Navigation_GetMode(P5S_navigation) == 1 || P5S_PedCheck(P5S_pedCheck)) {
        P5S_Ped_4732A0(ped, 0xFF, 0);
    }
    if (inTrigger) {
        P5S_Area_DeteriorateAttitudes(&areas[P5S_currArea], ped);
    }

    // one in four: a partner of the same type near, to stand and talk with
    if (P5S_rand() * 4 / 0x7FFF == 0) {
        int sex = modelInfo[0x50] != 0 ? 1 : 2;
        if (pedType == 9 && P5S_Random(0, 10) == 0) {
            sex = (modelInfo[0x50] != 0) + 1;
        }
        int value522380;
        int x3 = *P5S_ModelInfo_522380(modelInfo, &value522380);
        uint8_t* partner = P5S_FindPed(this, pedType, sex, x3, ped, 50.0f, 1);
        if (partner) {
            uint8_t found = 0;
            uint8_t busy = 1;
            if (!P5S_Ped_473F10(ped)) {
                busy = P5S_Ped_473F10(partner) ? 1 : 0;
            }
            uint8_t onGround = 0;
            CVector forward;
            CVector* direction = P5S_Ped_GetForward(partner, &forward);
            const double scale = 0.699999988079071;   // (0.7f as a double)
            CVector offset = {static_cast<float>(direction->x * scale), static_cast<float>(direction->y * scale),
                              static_cast<float>(direction->z * scale)};
            CVector target;
            P5S_Add(&target, PosOf(partner), &offset);
            float groundZ = P5S_FindGroundZ(target.x, target.y, static_cast<float>(target.z + 1.0), &onGround, 1);
            if (onGround) {
                target.z = groundZ;
            }
            if (P5S_IsInSpawnRange(this, &target, 0, 1, 15.0f) && !P5S_C2AB70_46EFB0(P5S_c2ab70, ped, &target, 1)) {
                if (P5S_IsPlaceFreeForPed(this, &target, partner)) {
                    found = 1;
                } else {
                    // the other side of the partner
                    CVector other;
                    target = *P5S_Sub(&other, PosOf(partner), &offset);
                    groundZ = P5S_FindGroundZ(target.x, target.y, static_cast<float>(target.z + 1.0), &onGround, 1);
                    if (onGround) {
                        target.z = groundZ;
                    }
                    if (P5S_IsPlaceFreeForPed(this, &target, partner)) {
                        found = 1;
                    }
                }
            }
            if (!P5S_PedCheck(P5S_otherPedCheck) && found && !busy && partner[0x1570] == 1) {
                int16_t partnerModel = At<int16_t>(partner, 0x10E);
                if (partnerModel != At<int16_t>(ped, 0x10E) && !P5S_Ped_4473E0(partner) &&
                    partnerModel != P5S_noPartnerModel && At<int32_t>(partner, 0x1310) != 13 &&
                    !P5S_Ped_4CE690(partner) && !P5S_Ped_474BF0(partner) && !P5S_Ped_41AA50(partner) &&
                    !P5S_Ped_473220(partner) && P5S_Ped2F0_485560(partner + 0x2F0) == 0 &&
                    At<void*>(partner, 0x2F8) == nullptr) {
                    At<int32_t>(At<uint8_t*>(ped, 0x2E0), 0x18) = -1;
                    At<int32_t>(At<uint8_t*>(partner, 0x2E0), 0x18) = -1;
                    P5S_Ped2F0_4860F0(ped + 0x2F0, partner, 1);
                    At<int32_t>(ped, 0x14EC) = 1;
                    At<int32_t>(partner, 0x14EC) = 1;
                    P5S_PedB44_48F1D0(ped + 0xB44, partner, 7, 0, 0);
                    P5S_Ped_SetAttitude(ped, At<int32_t>(partner, 0x1310), 4);
                    P5S_Ped_41EE20(ped, &target);
                }
            }
        }
    }

    // armed now and then, between 7:00 and 16:00 (only where ShouldArmPed says)
    if (At<int32_t>(ped, 0x14EC) == 0 && P5S_IsHourBetween(7, 0x10, 0, 0) && P5S_ShouldArmPed(this, ped) &&
        P5S_Random(0, 2) == 0) {
        int weapon;
        bool give = true;
        if (P5S_Random(0, 10) == 0) {
            weapon = P5S_commonWeapon;
        } else {
            switch (P5S_Random(0, 5)) {
            case 0: weapon = P5S_weapon0; break;
            case 1: weapon = P5S_weapon1; break;
            case 2: weapon = P5S_weapon2; break;
            case 3: weapon = P5S_weapon3; break;
            case 4: weapon = P5S_weapon4; break;
            default:
                weapon = 0;
                give = false;
                break;
            }
        }
        if (give) {
            P5S_GiveWeapon(this, ped, weapon);
        }
    }
    if (pedType == 7) {
        type7AllowedAt = P5S_timeInMs + 20000;
    }
    return ped;
}

// ---------------------------------------------------------------------------------

#define C08P5S_REVERSED(name, address, fn) \
    REVERSED(name, address, re::MemberAddress(fn), nullptr, nullptr, re::Mode::Original)
#define C08P5S_REVERSED_STATIC(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

C08P5S_REVERSED("CPedPopulation::IsPlaceValid", 0x0049C930, &CPedPopulation::IsPlaceValid);
C08P5S_REVERSED("CPedPopulation::UpdateCullDistances", 0x0049CE90, &CPedPopulation::UpdateCullDistances);
C08P5S_REVERSED("CPedPopulation::IsInSpawnRange", 0x0049D040, &CPedPopulation::IsInSpawnRange);
C08P5S_REVERSED("CPedPopulation::IsInTrigger", 0x0049D340, &CPedPopulation::IsInTrigger);
C08P5S_REVERSED("CPedPopulation::ConvertObjectToDummy", 0x0049D390, &CPedPopulation::ConvertObjectToDummy);
C08P5S_REVERSED("CPedPopulation::IsObjectPlaceClear", 0x0049D420, &CPedPopulation::IsObjectPlaceClear);
C08P5S_REVERSED("CPedPopulation::UpdatePOIs", 0x0049D4C0, &CPedPopulation::UpdatePOIs);
C08P5S_REVERSED_STATIC("CPedPopulation::SpawnPointTest", 0x0049D770, &PedPopulation::SpawnPointTest);
C08P5S_REVERSED("CPedPopulation::FindSpawnPoint", 0x0049D860, &CPedPopulation::FindSpawnPoint);
C08P5S_REVERSED("CPedPopulation::PickUpWeapon", 0x0049D950, &CPedPopulation::PickUpWeapon);
C08P5S_REVERSED("CPedPopulation::PickUpSpecialWeapon", 0x0049DAA0, &CPedPopulation::PickUpSpecialWeapon);
C08P5S_REVERSED("CPedPopulation::UpdateSpawnPoints", 0x0049DB90, &CPedPopulation::UpdateSpawnPoints);
C08P5S_REVERSED("CPedPopulation::CullObjects", 0x0049E080, &CPedPopulation::CullObjects);
C08P5S_REVERSED("CPedPopulation::IsPlaceFreeForPed", 0x0049E2C0, &CPedPopulation::IsPlaceFreeForPed);
C08P5S_REVERSED("CPedPopulation::IsPedInTriggerForItsType", 0x0049E410, &CPedPopulation::IsPedInTriggerForItsType);
C08P5S_REVERSED("CPedPopulation::ConvertAllObjectsToDummies", 0x0049E460, &CPedPopulation::ConvertAllObjectsToDummies);
C08P5S_REVERSED("CPedPopulation::IsEntityFree", 0x0049E4E0, &CPedPopulation::IsEntityFree);
C08P5S_REVERSED("CPedPopulation::ShouldArmPed", 0x0049E7D0, &CPedPopulation::ShouldArmPed);
C08P5S_REVERSED("CPedPopulation::FindPed", 0x0049E860, &CPedPopulation::FindPed);
C08P5S_REVERSED("CPedPopulation::CountPedsWithName", 0x0049E960, &CPedPopulation::CountPedsWithName);
C08P5S_REVERSED("CPedPopulation::CanAddPed", 0x0049E9E0, &CPedPopulation::CanAddPed);
C08P5S_REVERSED("CPedPopulation::IsPedKept", 0x0049EA30, &CPedPopulation::IsPedKept);
C08P5S_REVERSED("CPedPopulation::RemoveFarPed", 0x0049EB80, &CPedPopulation::RemoveFarPed);
C08P5S_REVERSED("CPedPopulation::RemovePedsInRadius", 0x0049F020, &CPedPopulation::RemovePedsInRadius);
C08P5S_REVERSED("CPlaceable::SetOrientation(CVector)", 0x0049F160, &PlaceableByVector::SetOrientation);
C08P5S_REVERSED("CPedPopulation::CullPeds", 0x0049F210, &CPedPopulation::CullPeds);
C08P5S_REVERSED("CPedPopulation::IsPedPoolFull", 0x0049F400, &CPedPopulation::IsPedPoolFull);
C08P5S_REVERSED("CPedPopulation::ConvertDummyToObject", 0x0049F440, &CPedPopulation::ConvertDummyToObject);
C08P5S_REVERSED("CPedPopulation::AddPed", 0x0049F560, &CPedPopulation::AddPed);
C08P5S_REVERSED("CPedPopulation::AddPedAt", 0x0049F6C0, &CPedPopulation::AddPedAt);
