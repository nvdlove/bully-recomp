#pragma once

// CPools: the pools the entities (and their helpers) live in, all made at start by
// Initialise. (GTA's CPools, with Bully's many more; the pool names are the game's,
// from its pool usage dump, 0x44C330.)
//
// A pool's elements are got at by slot, or by a handle (a "ref"): slot << 8 | the
// slot's flag byte, which a slot's reuse changes, so a stale handle finds nothing.

#include <cstddef>
#include <cstdint>

#include "game/Pool.h"
#include "reversed.h"

struct CEntity;
struct CPtrNode;
struct CEntryInfoNode;
struct CPed;
struct CVehicle;
struct CBuilding;
struct CTreadable;
struct CDummy;
struct CColModel;
struct CPropAnim;
struct CAccessory;
struct CStimulus;
struct CObject;
struct CProjectile;
struct CCutsceneObject;
struct SFXItem;
struct CEntityEffectKeeper;
struct CAccessoryContainer;
struct CAttitudeSet;
struct CPedActionTree;
struct CTargetingSystem;
struct CWeaponAccessoryContainer;
struct CMotionController;
struct JointConstraint;
struct FloorMotionDriver;
struct ReachDriver;
struct CLuaScript;

// What GetEntityInPool and GetEntityPoolAndSlot call the pools
enum ePoolType : int32_t {
    POOL_PED = 0,
    POOL_VEHICLE = 1,
    POOL_OBJECT = 2,
    POOL_PROJECTILE = 3,
    POOL_CUTSCENE = 4,
    POOL_DUMMY = 5,
    POOL_PROPANIM = 6,
    POOL_BUILDING = 7,
    POOL_TREADABLE = 8,
    POOL_ACCESSORY = 9,
    POOL_NONE = -1,
};

namespace CPools {

// (the element sizes; the slot counts are Initialise's)
GAME_VAR(CPool<CPtrNode>*, ms_pPtrNodePool, 0x00C0F5E8);                // 4, 15000
GAME_VAR(CPool<CEntryInfoNode>*, ms_pEntryInfoNodePool, 0x00C0F5EC);    // 0x14, 2000
GAME_VAR(CPool<CPed>*, ms_pPedPool, 0x00C0F5F0);                        // 0x1F44, 24
GAME_VAR(CPool<CVehicle>*, ms_pVehiclePool, 0x00C0F5F4);                // 0x8C0, 15
GAME_VAR(CPool<CBuilding>*, ms_pBuildingPool, 0x00C0F5F8);              // 0x120, 2250
GAME_VAR(CPool<CTreadable>*, ms_pTreadablePool, 0x00C0F5FC);            // 0x120, 1 (symbols.txt: "ms_pObjectPool")
GAME_VAR(CPool<CDummy>*, ms_pDummyPool, 0x00C0F600);                    // 0x128, 300
GAME_VAR(CPool<CColModel>*, ms_pColModelPool, 0x00C0F604);              // 0x40, 4150
GAME_VAR(CPool<CPropAnim>*, ms_pPropAnimPool, 0x00C0F608);              // 0x3B8, 220
GAME_VAR(CPool<CAccessory>*, ms_pAccessoryPool, 0x00C0F60C);            // 0x2E8, 48
GAME_VAR(CPool<CStimulus>*, ms_pStimulusPool, 0x00C0F610);              // 0x2C, 87
GAME_VAR(CPool<CObject>*, ms_pObjectPool, 0x00C0F614);                  // 0x22C, 275 (chained: projectiles, cutscene objects)
GAME_VAR(CPool<CProjectile>*, ms_pProjectilePool, 0x00C0F618);          // 0x24C, 35
GAME_VAR(CPool<CCutsceneObject>*, ms_pCutScenePool, 0x00C0F61C);        // 0x33C, 30
GAME_VAR(CPool<SFXItem>*, ms_pSFXItemPool, 0x00C0F620);                 // 0x74, 48
GAME_VAR(CPool<uint8_t>*, ms_pPool624, 0x00C0F624);                     // 0x64, 200 (ctor 0x5E57A0; not in the dump)
GAME_VAR(CPool<CEntityEffectKeeper>*, ms_pEntityEffectKeeperPool, 0x00C0F628);  // 0x74, 24
GAME_VAR(CPool<CAccessoryContainer>*, ms_pAccessoryContainerPool, 0x00C0F62C);  // 8, 24
GAME_VAR(CPool<CAttitudeSet>*, ms_pAttitudeSetPool, 0x00C0F630);        // 0x38, 24
GAME_VAR(CPool<CPedActionTree>*, ms_pPedActionTreePool, 0x00C0F634);    // 0x5C0, 24
GAME_VAR(CPool<CTargetingSystem>*, ms_pTargetingSystemPool, 0x00C0F638);  // 0xE0, 24
GAME_VAR(CPool<CWeaponAccessoryContainer>*, ms_pWeaponAccessoryContainerPool, 0x00C0F63C);  // 0x20, 24
GAME_VAR(CPool<CMotionController>*, ms_pMotionControllerPool, 0x00C0F640);  // 0x200, 24
GAME_VAR(CPool<JointConstraint>*, ms_pJointConstraintPool, 0x00C0F644);   // 0x80, 48
GAME_VAR(CPool<FloorMotionDriver>*, ms_pFloorMotionDriverPool, 0x00C0F648);  // 0x40, 24
GAME_VAR(CPool<ReachDriver>*, ms_pReachDriverPool, 0x00C0F64C);         // 0x50, 48
GAME_VAR(CPool<CLuaScript>*, ms_pLuaScriptPool, 0x00C0F650);            // 0x2B0C, 8
GAME_VAR(CPool<uint8_t>*, ms_pWeaponItemPool, 0x00C0F654);              // 0x28, 57 (CWeaponInventory's; name a guess)

// The pointer-node pool's array: the entity lists link their nodes by index into it
GAME_VAR(CPtrNode*, ms_pPtrNodes, 0x00C0F788);

// 0x44D320: makes every pool
void __cdecl Initialise();
// 0x44D090: flushes and frees them (not the projectile, cutscene and SFX item pools,
// and the pointers are left as they were)
void __cdecl ShutDown();
// 0x44C330: "size, used, element size, bytes" of each pool, to a debug file
void __cdecl PrintUsage(void* file);

// 0x44A3E0: the element at a slot of a pool (free or not; nullptr for no such pool)
void* __cdecl GetEntityInPool(uint32_t pool, int slot);
// 0x44C930: which pool an entity is in, and its slot (-1: not on a slot)
void __cdecl GetEntityPoolAndSlot(CEntity* entity, int32_t* pool, int32_t* slot);
// 0x44A4D0: removes every entity of a pool (and its chain) from the world and deletes
// it; nothing unless `keep` is 0
void __cdecl DeleteAll(CPool<uint8_t>* pool, int keep);

// Handles (nullptr: not a live one)
// 0x44A550
CPed* __cdecl GetPed(int handle);
// 0x44A580
CVehicle* __cdecl GetVehicle(int handle);
// 0x44A5B0 (GTA's GetObject: windows.h has a macro by that name)
CObject* __cdecl GetObjectFromRef(int handle);
// 0x44A5E0
CPropAnim* __cdecl GetPropAnim(int handle);
// 0x44A660: kind 0 an object, 4 a building, 5 a prop anim
CEntity* __cdecl GetEntity(int handle, int kind);
// 0x44CAC0
int __cdecl GetVehicleRef(CVehicle* vehicle);
// 0x44CAF0
int __cdecl GetObjectRef(CObject* object);
// 0x44CB20
int __cdecl GetPropAnimRef(CPropAnim* prop);
// 0x44A610: the prop anim with the id (+0x3AC). (The game looks the slots up as
// handles: only slot 0 can match; see there)
CPropAnim* __cdecl FindPropAnim(int id);
// 0x44CB50: the SFX item's slot, -1 if it isn't one
int __cdecl GetSFXItemIndex(SFXItem* item);

// Pointer checks
// 0x4CEDB0: in the building (or treadable) pool's range
uint8_t __cdecl IsBuildingPointerValid(CEntity* building);
// 0x4CE390: in the vehicle pool's range, and +0x178 set
uint8_t __cdecl IsVehiclePointerValid(CVehicle* vehicle);
// 0x4ED800: in the prop anim pool's range
uint8_t __cdecl IsPropAnimPointerValid(CPropAnim* prop);

} // namespace CPools
