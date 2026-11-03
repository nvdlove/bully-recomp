#pragma once

// The ped population (0xC2C108, 0x79A4 bytes, made by 0x49DEC0): what mix of peds
// each area wants by time of day (Config\Dat\PedPop.dat), the population triggers
// that override it, how many peds of each type there are, and the spawning and
// culling of ambient peds around the player (the cull distances, the spawn points
// of 0xBF6228, the POI groups). 0x49C0C0 .. 0x49FCD0.
//
// The functions here are the population's own (thiscall on it, or on one of its
// parts) unless noted; several ignore their `this` (the game calls them on the
// population anyway).

#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace pop {
struct CVector {
    float x, y, z;
};
static_assert(sizeof(CVector) == 0xC);

using re::At;   // (reads through const objects too)

// A placeable's position: its matrix's (+0x14, the position at +0x30), else its own (+4)
inline CVector* PosOf(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return reinterpret_cast<CVector*>(matrix ? matrix + 0x30 : static_cast<uint8_t*>(entity) + 4);
}
} // namespace pop

// The objective to pick up the weapon a ped dropped (0x17). vtable 0x911DC4
struct PickupObjective {
    re::Ptr32<re::Ptr32<void>> vtable;      // +0x00
    re::Ptr32<void> ped;          // +0x04
    int32_t type;       // +0x08 0x17
    int32_t numRefs;    // +0x0C
    int32_t pickup;     // +0x10 the pickup's index (0x443FE0)

    // 0x49C0C0
    PickupObjective* Construct(void* ped, int pickup);
    // vf0 0x49C160: the (deleting) destructor
    PickupObjective* Destroy(uint8_t flags);
    // vf1 0x49C0F0: IsA (its own class id, or Objective's)
    uint8_t IsA(int id);
};
static_assert(sizeof(PickupObjective) == 0x14);

// One population mix (0x24 bytes, vtable 0x911E9C; 0x4A0C70 constructs, 0x4A0CA0
// destructs, 0x4645F0 parses a PedPop.dat line, 0x464680 sets from counts)
struct CPopulationInfo {
    re::Ptr32<re::Ptr32<void>> vtable;         // +0x00
    int32_t total;         // +0x04 how many ambient peds in all
    uint8_t counts[12];    // +0x08 how many of each ped type
    uint32_t numCounts;    // +0x14 how many of counts[] are set
    uint8_t unknown18[0xC];  // +0x18 (an IArchivable part, vtable 0x911E8C)
};
static_assert(offsetof(CPopulationInfo, numCounts) == 0x14);
static_assert(sizeof(CPopulationInfo) == 0x24);

// A population trigger an area has active
struct PopTriggerEntry {
    int32_t pedType;   // +0 0xF: none (AreaDeteriorateAttitudeForTriggerPedType sets it)
    re::Ptr32<uint8_t> trigger;  // +4 (+4 its position, +0x24 its CAreaPopulationInfo, +0x2C its id,
                       //     +0x3C its area code)
};
static_assert(sizeof(PopTriggerEntry) == 8);

// An area's populations (0x1D8 bytes, vtable 0x911DD8): one per time of day (0x6A8CC0
// of 0x20C35E8), and the triggers active in it
struct CAreaPopulationInfo {
    re::Ptr32<re::Ptr32<void>> vtable;                 // +0x000
    CPopulationInfo infos[4];      // +0x004
    PopTriggerEntry triggers[40];  // +0x094 (the array class's constructor: 0x66BEA0)
    uint32_t numTriggers;          // +0x1D4

    // 0x49C1C0: the destructor's body
    void Destruct();
    // 0x49CA20: a trigger active (not twice, by its id)
    void AddTrigger(uint8_t* trigger);
    // 0x49CA80: a trigger inactive (the last one takes its place)
    void RemoveTrigger(uint8_t* trigger);
    // 0x49CAF0: a new ped in a trigger that deteriorates attitudes: its attitude to
    // the other ped types (not its own, 0, 7, 8, 9) one worse, down to 0 (ignores `this`)
    void DeteriorateAttitudes(uint8_t* ped);
    // 0x49CBB0: the ped type of an active trigger
    void SetTriggerPedType(uint8_t* trigger, int pedType);
    // 0x49DD10: the population at a place: that of the last active trigger it's in,
    // else the area's own
    CPopulationInfo* GetInfoAt(int timeSlot, pop::CVector pos);
    // 0x49DE10: whether a place is in an active trigger set to a ped type
    uint8_t IsInTriggerForPedType(int pedType, pop::CVector* pos);
};
static_assert(offsetof(CAreaPopulationInfo, triggers) == 0x94);
static_assert(offsetof(CAreaPopulationInfo, numTriggers) == 0x1D4);
static_assert(sizeof(CAreaPopulationInfo) == 0x1D8);

// The peds of one ped type (0x14 bytes, vtable 0x911DBC; its base's constructor
// 0x464E00, destructor 0x4647A0; vf0: whether a model may be used)
struct CPedModelsInfo {
    re::Ptr32<re::Ptr32<void>> vtable;          // +0x00
    int32_t numPeds;        // +0x04 how many there are
    uint8_t unknown08[0xC]; // +0x08
};
static_assert(sizeof(CPedModelsInfo) == 0x14);

// The models the population may spawn (0x10 bytes; 0x49BFD0 constructs)
struct PedModelList {
    re::Ptr32<int32_t> models;    // +0x0 (-1: none)
    int32_t count;      // +0x4
    int32_t unknown8;   // +0x8
    int32_t cursor;     // +0xC

    // 0x49C700: the ones not loaded requested
    void RequestAll();
    // 0x49F0C0: a model from a random place on, that its ped type accepts; -1 if none
    int ChooseModel(int arg);
};
static_assert(sizeof(PedModelList) == 0x10);

// A ped model and an entity, with when (the ring at +0x7998)
struct PedSpawnRecord {
    int32_t model;      // +0
    re::Ptr32<uint8_t> entity;    // +4 (0: a free one)
    uint32_t time;      // +8 CTimer ms
};
static_assert(sizeof(PedSpawnRecord) == 0xC);

struct PedSpawnRing {
    re::Ptr32<PedSpawnRecord> entries;  // +0x0
    int32_t size;             // +0x4 (21)
    int32_t unknown8;         // +0x8
    int32_t cursor;           // +0xC

    // 0x49C800: sorted by time, newest first (a selection sort)
    void SortByTime();
    // 0x49C990: into the first free one before the cursor, else at the cursor
    // (which moves on round the ring). (Returns what's left in eax.)
    int Add(int model, uint8_t* entity);
};
static_assert(sizeof(PedSpawnRing) == 0x10);

// The POI groups made for the points of interest around the player
struct POIGroupList {
    re::Ptr32<uint8_t> groups[60];  // +0x00
    uint32_t count;       // +0xF0

    // 0x49C8F0: one out (the rest move down)
    void RemoveAt(uint32_t index);
};
static_assert(sizeof(POIGroupList) == 0xF4);

struct CPedPopulation {
    CAreaPopulationInfo areas[64];     // +0x0000 by area code (CGame::currArea)
    CPedModelsInfo pedModels[12];      // +0x7600 by ped type
    re::Ptr32<CPopulationInfo> current;          // +0x76F0 where the player is (0x49EF00)
    re::Ptr32<CPopulationInfo> override_;        // +0x76F4 AreaOverridePopulation's
    int32_t typeDisabled[15];          // +0x76F8
    uint32_t typeBlockedUntil[15];     // +0x7734 CTimer ms (a minute after one is used up)
    uint8_t spawnPoints[0xA0];         // +0x7770 (0xBF6228's: 0x440E10 finds, 0x440FB0 uses)
    pop::CVector lastFocus;            // +0x7810 where the spawn points were found
    int32_t numSpawnPoints;            // +0x781C
    int32_t unknown7820;               // +0x7820
    int32_t unknown7824;               // +0x7824
    // +0x7828 the cull distances: [0] far in view, [1] near in view (no spawning
    // closer), [2], [3] far out of view, [4] near out of view, [5]
    float cull[6];
    float cullExterior[6];             // +0x7840 PedPop.dat's first line
    float cullInterior[6];             // +0x7858 its second
    float interiorHeight;              // +0x7870 (how far above or below in interiors)
    int32_t unknown7874;               // +0x7874 (PedPop.dat)
    int32_t unknown7878;               // +0x7878 (PedPop.dat)
    int32_t unknown787C;               // +0x787C
    int32_t forcedModel;               // +0x7880 (-1: none)
    int32_t forcedPedType;             // +0x7884
    int32_t unknown7888;               // +0x7888
    int32_t pendingModel;              // +0x788C (-1: none)
    int32_t pendingTries;              // +0x7890
    int32_t unknown7894;               // +0x7894
    // +0x7898 bit 0 fast culling, 1 males, 2 females (a model's +0x50 is "female"), 3 (flips when the model list
    // comes round), 4, 5 triggers tested in 3D
    uint8_t flags;
    uint8_t unknown7899[3];
    uint32_t type7AllowedAt;           // +0x789C CTimer ms (ped type 7: 20 s apart)
    POIGroupList poiGroups;            // +0x78A0
    re::Ptr32<PedModelList> modelList;           // +0x7994
    re::Ptr32<PedSpawnRing> spawnRing;           // +0x7998
    re::Ptr32<uint8_t> nearEntities[2];          // +0x799C (0x45E3C0 fills them)

    // ---- PedPopulation.cpp
    // 0x49C1F0: a random model of a ped type (of the sexes allowed)
    int ChooseModelForType(int pedType);
    // 0x49C230: a model of a ped type (-1: a random type)
    int GetModelForType(int pedType, int a, int b);
    // 0x49C280: a weapon given to a ped that has none (ignores `this`)
    uint8_t GiveWeapon(uint8_t* ped, int weapon);
    // 0x49C2D0
    int GetNumPedsOfType(int pedType);
    // 0x49C2F0
    int GetTotalNumPeds();
    // 0x49C330: a model to load and use next (tried three times); -1 while loading
    int PickPendingModel(int model);
    // 0x49C3D0: Config\Dat\PedPop.dat
    void LoadPedPop();
    // 0x49C5B0
    void Override(int* counts);
    // 0x49C5F0
    void SetTypeDisabled(uint32_t pedType, uint8_t disabled);
    // 0x49C610
    void ClearTypeDisabled();
    // 0x49C670
    void ClearTypeBlocked();
    // 0x49C6D0
    void RevertToDefault();
    // 0x49CBF0: the constructor
    CPedPopulation* Construct();
    // 0x49CC90: the destructor's body
    void Destruct();
    // 0x49CD50
    void Reset();
    // 0x49CDB0: the forced model loaded? (requests it and the list's while not)
    uint8_t UpdateForcedModel();
    // 0x49D480
    void RemovePOIGroup(uint8_t* group);
    // 0x49DB10
    void ResetPedModels();
    // 0x49DB50 / 0x49DB70: the trigger's area's
    void AddTrigger(uint8_t* trigger);
    void RemoveTrigger(uint8_t* trigger);
    // 0x49DC80: an entity a ped was spawned for, remembered (in area 0)
    void RecordSpawn(uint8_t* ped, uint8_t* entity);
    // 0x49EE40
    void SetForcedModel(int model);
    // 0x49EE70: a ped type blocked for a minute (after a ped of it that was in a trigger)
    void BlockPedTypeOf(uint8_t* ped);
    // 0x49EEB0: the population in effect at a place (the override, if any)
    CPopulationInfo* GetInfoAt(pop::CVector* pos);
    // 0x49EF00: that, as the current one
    void UpdateCurrent(pop::CVector* pos);
    // 0x49EF60 / 0x49EFA0 / 0x49EFE0: by the trigger's index
    void ActivateTrigger(int trigger);
    void DeactivateTrigger(int trigger);
    void DeteriorateAttitudeForTriggerPedType(int trigger, int pedType);
    // 0x49F610: a ped type to spawn at a place (what the mix wants most); -1 for none
    int ChoosePedType(pop::CVector* pos, uint32_t exclude);

    // ---- PedPopulationSpawn.cpp
    // 0x49C930: whether a place is somewhere 0xC3CC68 accepts (ignores `this`)
    int IsPlaceValid(pop::CVector* pos);
    // 0x49CE90: the cull distances for the area (interiors', or exterior ones
    // stretched by up to 70% by the speed of the player's vehicle)
    void UpdateCullDistances();
    // 0x49D040: `outOfRange` set: whether a place is beyond the cull distances; clear:
    // whether it's in the band to spawn in (past `minDistance` if `useMin`)
    uint8_t IsInSpawnRange(pop::CVector* pos, int outOfRange, int useMin, float minDistance);
    // 0x49D340: whether a place is in a trigger (0: no trigger)
    uint8_t IsInTrigger(pop::CVector* pos, uint8_t* trigger);
    // 0x49D390: an object back into a dummy (ignores `this`)
    void ConvertObjectToDummy(uint8_t* object);
    // 0x49D420: nothing else where an object was (ignores `this`)
    uint8_t IsObjectPlaceClear(uint8_t* object);
    // 0x49D4C0 (and its thunk 0x49E7C0): the POIs around the player on or off
    void UpdatePOIs();
    // 0x49D860: a spawn point (0xBF6228's)
    uint8_t FindSpawnPoint(pop::CVector* pos, int a2, int a3, int a4, float minFront, float minBehind);
    // 0x49D950: a ped goes to pick up the weapon it holds (or attack with it) (ignores `this`)
    uint8_t PickUpWeapon(uint8_t* ped, int giveObjective);
    // 0x49DAA0: for peds holding one of five weapons
    uint8_t PickUpSpecialWeapon(uint8_t* ped);
    // 0x49DB90: the spawn points found again after the focus moved 10 m
    uint8_t UpdateSpawnPoints();
    // 0x49E080: a 32nd of the objects culled or turned back into dummies
    void CullObjects(pop::CVector centre);
    // 0x49E2C0: room for a ped at a place (none there but `ignore`, and a clear line)
    uint8_t IsPlaceFreeForPed(pop::CVector* pos, uint8_t* ignore);
    // 0x49E410
    uint8_t IsPedInTriggerForItsType(uint8_t* ped);
    // 0x49E460: every object that can be, back into a dummy
    void ConvertAllObjectsToDummies();
    // 0x49E4E0: whether an entity (a dummy about to be an object) collides with
    // nothing in its sectors (ignores `this`)
    uint8_t IsEntityFree(uint8_t* entity);
    // 0x49E7D0: whether a ped is one to get a weapon here (the school's or area 2)
    uint8_t ShouldArmPed(uint8_t* ped);
    // 0x49E860: a ped (from the pool's end) matching; with `near`, within `maxDistance`
    // of it (ignores `this`)
    uint8_t* FindPed(int pedType, int sex, int x3, uint8_t* near, float maxDistance, int notPlayer);
    // 0x49E960: how many peds (up to `max`) have a model name's hash (ignores `this`)
    int CountPedsWithName(uint32_t hash, int max);
    // 0x49E9E0: room for another ambient ped
    int CanAddPed();
    // 0x49EA30: whether a ped is kept (it's after the player, near enough)
    uint8_t IsPedKept(uint8_t* ped);
    // 0x49EB80: a ped removed to make room (one behind the camera, or with `force`
    // the farthest)
    uint8_t RemoveFarPed(int force);
    // 0x49F020: the peds within a radius removed (ignores `this`)
    void RemovePedsInRadius(pop::CVector centre, float radius);
    // 0x49F210: the peds faded in, and culled out of range
    void CullPeds(pop::CVector centre);
    // 0x49F400: whether the ped pool is full (fewer than 4 free, none could go)
    uint8_t IsPedPoolFull();
    // 0x49F440: a dummy made an object, when it collides with nothing (or given to
    // its pickup) (ignores `this`)
    void ConvertDummyToObject(uint8_t* dummy);
    // 0x49F560: an ambient ped of a model (-1: chosen)
    uint8_t* AddPed(int arg, int model);
    // 0x49F6C0: an ambient ped at a place, maybe with a partner to talk to and a weapon
    uint8_t* AddPedAt(int model, pop::CVector* pos, int arg, float value);
};
static_assert(offsetof(CPedPopulation, pedModels) == 0x7600);
static_assert(offsetof(CPedPopulation, current) == 0x76F0);
static_assert(offsetof(CPedPopulation, typeDisabled) == 0x76F8);
static_assert(offsetof(CPedPopulation, typeBlockedUntil) == 0x7734);
static_assert(offsetof(CPedPopulation, spawnPoints) == 0x7770);
static_assert(offsetof(CPedPopulation, lastFocus) == 0x7810);
static_assert(offsetof(CPedPopulation, numSpawnPoints) == 0x781C);
static_assert(offsetof(CPedPopulation, cull) == 0x7828);
static_assert(offsetof(CPedPopulation, cullExterior) == 0x7840);
static_assert(offsetof(CPedPopulation, cullInterior) == 0x7858);
static_assert(offsetof(CPedPopulation, interiorHeight) == 0x7870);
static_assert(offsetof(CPedPopulation, forcedModel) == 0x7880);
static_assert(offsetof(CPedPopulation, flags) == 0x7898);
static_assert(offsetof(CPedPopulation, type7AllowedAt) == 0x789C);
static_assert(offsetof(CPedPopulation, poiGroups) == 0x78A0);
static_assert(offsetof(CPedPopulation, modelList) == 0x7994);
static_assert(offsetof(CPedPopulation, spawnRing) == 0x7998);
static_assert(offsetof(CPedPopulation, nearEntities) == 0x799C);
static_assert(sizeof(CPedPopulation) == 0x79A4);

// The static ones (cdecl, no arguments)
namespace PedPopulation {
// 0x49DEC0: made (with its model list and spawn ring) if not yet
void __cdecl Create();
// 0x49DF80: made, and everything reset
void __cdecl Init();
// 0x49F190: made, PedPop.dat loaded, the ped types' models set up, reset
void __cdecl Load();
// 0x49F1E0
void __cdecl Shutdown();
// 0x49D770 (cdecl): 0x440FB0's test of a spawn point against its context (in range
// in front, or behind)
uint8_t __cdecl SpawnPointTest(pop::CVector* point, float* context);
} // namespace PedPopulation

// 0x49F160 (thiscall on a placeable): 0x46A690 with a vector's x, y, z
struct PlaceableByVector {
    void SetOrientation(const pop::CVector* v);
};
