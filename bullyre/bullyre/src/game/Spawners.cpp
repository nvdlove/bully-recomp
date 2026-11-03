// The spawners (areas that keep peds coming out of their spawn locations while the
// player is inside a trigger), their spawn locations, their pools and their manager
// (0xC2C12C).
//
// A spawner (CSpawner, vtable 0x9122F0 family; CMissionSpawner 0x912328, 0x174 bytes,
// pool 0x20C7C7C; CAmbientSpawner 0x114 bytes, pool 0x20C7C78):
//   +0x0 vtable (vf2 AddLocation, vf3 (+0xC) a ped came out, vf4 (+0x10) can a location
//   spawn), +0x4 the delay (ms), +0xC when to spawn next (-1: not waiting), +0x14 flags
//   (4: off), +0x18 the locations (40), +0xB8 how many, +0xBC how many may be alive
//   (signed byte), +0xBD how many in all (signed byte), +0xC0 the trigger's hash (0:
//   none), +0xC4 a registered reference, +0xC8 flags (1: on without a trigger), +0xCC an
//   action handle, +0x170 how many came out so far.
// A spawn location (CSpawnLocation 0x912300, 0x14 bytes, pool 0x20C7C74;
// CMissionSpawnLocation 0x912358, 0x3C bytes, pool 0x20C7C80): +0x4 its spawner, +0x8
// its route (point)'s id, +0xC its trigger's hash, +0x10 the ped it made last; the
// mission one has its vehicle models at +0x14 (CVehicleModelsInfo, +0x10 how many) and
// its ped models at +0x28 (CPedModelsInfo, +0x4 how many), +0x2C / +0x18 the counts the
// spawner sums.

#include <cstdint>

#include "game/Pool.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;
template <typename R, typename... A>
R VCall(void* obj, int offset, A... args) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(obj))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(fn)(obj, args...);
}

// the pools (CPool<> of each kind)
GAME_VAR(CPool<uint8_t>*, pool20C7C6C, 0x020C7C6C);
GAME_VAR(CPool<uint8_t>*, spawnLocationPool, 0x020C7C74);
GAME_VAR(CPool<uint8_t>*, ambientSpawnerPool, 0x020C7C78);
GAME_VAR(CPool<uint8_t>*, missionSpawnerPool, 0x020C7C7C);
GAME_VAR(CPool<uint8_t>*, missionSpawnLocationPool, 0x020C7C80);
GAME_VAR(CPool<uint8_t>*, triggerPool, 0x020C7C48);
GAME_VAR(CPool<uint8_t>*, pedPool, 0x00C0F5F0);   // CPools::ms_pPedPool
inline void* const TheTriggers = reinterpret_cast<void*>(0x020C7E98);   // the triggers' hashes
inline void* const TheRoutes = reinterpret_cast<void*>(0x00BC5E7C);     // the route points
inline void* const propAnims = reinterpret_cast<void*>(0x00C3B808);

GAME_VAR(void*, g_spawners, 0x00C2C12C);   // the manager (0x150 bytes)
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);

inline void* __cdecl Memset(void* p, int value, size_t n) { return crt::Memset(p, value, n); }
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void, __thiscall, ActionHandle_Release, 0x005D9C50, void* handle);
GAME_FN(int, __thiscall, Triggers_FindByHash, 0x006D79C0, void* triggers, uint32_t hash);
GAME_FN(uint8_t, __thiscall, Trigger_IsPointInside, 0x006D6FA0, void* trigger, const float* pos);
GAME_FN(uint8_t, __thiscall, Trigger_IsPointInside2D, 0x006D6EB0, void* trigger, const float* xy);
GAME_FN(void*, __cdecl, Trigger_GetPropAnim, 0x004EA3C0, void* trigger);
GAME_FN(void*, __thiscall, Routes_Get, 0x006D6450, void* routes, int slot);
GAME_FN(void*, __thiscall, Routes_GetById, 0x006D64D0, void* routes, int id);
GAME_FN(float*, __thiscall, Route_First, 0x006D59B0, void* route);
GAME_FN(void*, __thiscall, SpawnLocationPool_New, 0x005EC730, CPool<uint8_t>* pool);
GAME_FN(void*, __thiscall, MissionSpawnLocationPool_New, 0x004A1F60, CPool<uint8_t>* pool);
GAME_FN(void*, __thiscall, MissionSpawnerPool_New, 0x004A1EE0, CPool<uint8_t>* pool);
GAME_FN(void*, __thiscall, AmbientSpawnerPool_New, 0x004A1FE0, CPool<uint8_t>* pool);
GAME_FN(void, __thiscall, Spawner_Destruct, 0x004A1260, void* spawner);
GAME_FN(uint32_t, __thiscall, Spawner_PickLocation, 0x004A12B0, void* spawner);   // -1: none
GAME_FN(void, __thiscall, Spawners_Destruct, 0x004A1940, void* spawners);
GAME_FN(void*, __thiscall, SpawnLocation_Construct, 0x004A21F0, void* location, int routeId, uint32_t triggerHash, void* spawner);
GAME_FN(void*, __thiscall, MissionSpawnLocation_Construct, 0x004A2310, void* location, int routeId, uint32_t triggerHash,
        void* spawner);
GAME_FN(void, __thiscall, SpawnLocation_Update, 0x004A2240, void* location);
GAME_FN(uint8_t, __thiscall, Spawner_IsTimeToSpawn, 0x004A2100, void* spawner);
GAME_FN(uint8_t*, __thiscall, SpawnLocation_SpawnPed, 0x004A25D0, void* location, int model);
GAME_FN(void, __thiscall, MissionSpawnLocation_Spawn, 0x004A2860, void* location);
GAME_FN(uint8_t, __thiscall, MissionSpawner_Update, 0x004A28E0, void* spawner);
// the model lists (CModelsInfo, 0x464E00 their constructor)
GAME_FN(void, __thiscall, ModelsInfo_Construct, 0x00464E00, void* info);
GAME_FN(int, __thiscall, ModelsInfo_Pick, 0x00464B70, void* info, int, int);
GAME_FN(int, __thiscall, PedModelsInfo_Pick, 0x00464E70, void* info, int flagA, int flagB, int);
GAME_FN(void, __thiscall, ModelsInfo_Used, 0x00464AB0, void* info, int model);
// streaming, peds, the world
GAME_FN(uint8_t*, __cdecl, GetStreamingInfo, 0x0052DD30, int id);   // (+4: the load state, 1 loaded)
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int id, int flags);
GAME_FN(float, __cdecl, FindGroundZ, 0x004636B0, float x, float y, float z, uint8_t* found, int);
GAME_FN(void*, __cdecl, Ped_New, 0x004789E0, uintptr_t size);
GAME_FN(uint8_t*, __thiscall, CivilianPed_Construct, 0x0046DFF0, void* ped, int model, int, int);
GAME_FN(void, __thiscall, Entity_SetPosition, 0x0041C4E0, void* entity, float x, float y, float z);
GAME_FN(void, __thiscall, Ped_SetHeading, 0x00475730, void* ped, float heading);
GAME_FN(void, __cdecl, World_Add, 0x0045D6B0, void* entity);
GAME_FN(void, __thiscall, PropAnims_AttachPed, 0x004E9CB0, void* propAnims, void* ped, void* propAnim);

constexpr uintptr_t vtblSpawnLocation = 0x00912300;
constexpr uintptr_t vtblMissionSpawnLocation = 0x00912358;
constexpr uintptr_t vtblMissionSpawner = 0x00912328;
constexpr uintptr_t vtblVehicleModelsInfo = 0x00900CF4;
constexpr uintptr_t vtblPedModelsInfo = 0x00911DBC;

// a pool's slot (nullptr when free), and one of the trigger pool's by slot
uint8_t* PoolSlot(CPool<uint8_t>* pool, int slot) {
    if (pool->flags[slot] & 0x80) {
        return nullptr;
    }
    return pool->objects + pool->elemSize * slot;
}
uint8_t* FindTrigger(uint32_t hash) {
    int slot = Triggers_FindByHash(TheTriggers, hash);
    if (slot == -1) {
        return nullptr;
    }
    return PoolSlot(triggerPool, slot);
}
const float* EntityPosition(uint8_t* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<float*>(matrix + 0x30) : reinterpret_cast<float*>(entity + 4);
}

// CPool<T>::New, for T of `size` bytes (the slot zeroed)
void* PoolNew(CPool<uint8_t>* pool, size_t size) {
    uint8_t* flags = pool->flags;
    bool wrapped = false;
    if (!(flags[pool->lastUsed] & 0x80)) {
        do {
            pool->lastUsed++;
            if (pool->lastUsed >= pool->size) {
                if (wrapped) {
                    return nullptr;
                }
                pool->lastUsed = 0;
                wrapped = true;
            }
        } while (!(flags[pool->lastUsed] & 0x80));
    }
    flags[pool->lastUsed] &= 0x7F;
    uint8_t& flag = pool->flags[pool->lastUsed];
    flag ^= (uint8_t(flag + 1) ^ flag) & 0x7F;   // the reuse count up, the free bit kept
    void* object = pool->objects + pool->elemSize * pool->lastUsed;
    Memset(object, 0, size);
    return object;
}

// CPool<T>::Delete as an operator delete
void PoolDelete(CPool<uint8_t>* pool, void* object) {
    int slot = int((static_cast<uint8_t*>(object) - pool->objects) / pool->elemSize);
    pool->flags[slot] |= 0x80;
    if (slot < pool->lastUsed) {
        pool->lastUsed = slot;
    }
}

} // namespace

namespace c09spawners {

// 0x4A1FE0: CPool<CAmbientSpawner>::New (0x114 bytes)
void* __fastcall AmbientSpawnerPoolNew(CPool<uint8_t>* pool, re::Edx) {
    return PoolNew(pool, 0x114);
}

// 0x4A2060: CMissionSpawner's destructor body: its peds deleted, then CSpawner's
void __fastcall MissionSpawnerDestruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = vtblMissionSpawner;
    if (At<void*>(self, 0xC4)) {
        CleanUpOldReference(At<void*>(self, 0xC4), self + 0xC4);
    }
    CPool<uint8_t>* pool = pedPool;
    for (int slot = pool->size; slot != 0;) {
        slot--;
        uint8_t* ped = PoolSlot(pool, slot);
        if (ped && At<void*>(ped, 0x1B8) == self) {   // (the ped's spawner)
            At<void*>(ped, 0x1B8) = nullptr;
            VCall<void>(ped, 0, 1);   // deleted
        }
    }
    At<uint32_t>(self, 0x170) = 0;
    ActionHandle_Release(self + 0xCC);
    Spawner_Destruct(self);
}

// 0x4A20F0: CMissionSpawner::operator new (from its pool; the size unused)
void* __cdecl MissionSpawnerNew(uint32_t) {
    return MissionSpawnerPool_New(missionSpawnerPool);
}

// 0x4A2100: time for the spawner to spawn: on (by its trigger holding the player, or
// flag 1 without one) for its delay since it came on
uint8_t __fastcall SpawnerIsTimeToSpawn(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 0x14) & 4) {
        return 0;
    }
    uint8_t* player = playerPed;
    if (At<uint32_t>(self, 0xC0) != 0) {
        uint8_t* trigger = FindTrigger(At<uint32_t>(self, 0xC0));
        if (!trigger || !Trigger_IsPointInside(trigger, EntityPosition(player))) {
            At<int32_t>(self, 0xC) = -1;
            return 0;
        }
    } else if (!(At<uint8_t>(self, 0xC8) & 1)) {
        At<int32_t>(self, 0xC) = -1;
        return 0;
    }
    if (At<int32_t>(self, 0xC) == -1) {
        At<int32_t>(self, 0xC) = At<int32_t>(self, 4) + int32_t(timeInMs);
    }
    return int32_t(timeInMs) >= At<int32_t>(self, 0xC);
}

// 0x4A21B0: the peds its locations have out (each one's +0x2C)
int32_t __fastcall SpawnerCountAlive(uint8_t* self, re::Edx) {
    int32_t count = 0;
    for (int i = At<int32_t>(self, 0xB8) - 1; i >= 0; i--) {
        count += At<int32_t>(At<void*>(self, 0x18 + i * 4), 0x2C);
    }
    return count;
}

// 0x4A21E0: CAmbientSpawner::operator new (from its pool; the size unused)
void* __cdecl AmbientSpawnerNew(uint32_t) {
    return AmbientSpawnerPool_New(ambientSpawnerPool);
}

// 0x4A21F0: CSpawnLocation's constructor (the lookups of its trigger and route are
// left in, their results unused)
void* __fastcall SpawnLocationConstruct(uint8_t* self, re::Edx, int routeId, uint32_t triggerHash, void* spawner) {
    At<void*>(self, 4) = spawner;
    At<int32_t>(self, 8) = routeId;
    At<uint32_t>(self, 0) = vtblSpawnLocation;
    At<uint32_t>(self, 0xC) = triggerHash;
    At<void*>(self, 0x10) = nullptr;
    Triggers_FindByHash(TheTriggers, triggerHash);
    Route_First(Routes_GetById(TheRoutes, At<int32_t>(self, 8)));
    return self;
}

// 0x4A2240: the last ped forgotten once it's out of the location's trigger
void __fastcall SpawnLocationUpdate(uint8_t* self, re::Edx) {
    uint8_t* ped = At<uint8_t*>(self, 0x10);
    if (!ped || At<uint32_t>(self, 0xC) == 0) {
        return;
    }
    uint8_t* trigger = FindTrigger(At<uint32_t>(self, 0xC));
    if (!trigger) {
        return;
    }
    const float* pos = EntityPosition(ped);
    float xy[2] = {pos[0], pos[1]};
    if (!Trigger_IsPointInside2D(trigger, xy)) {
        Trigger_GetPropAnim(trigger);   // (its result unused)
        At<void*>(self, 0x10) = nullptr;
    }
}

// 0x4A22D0: the manager made (once)
void __cdecl SpawnersInit() {
    if (g_spawners) {
        return;
    }
    uint8_t* spawners = static_cast<uint8_t*>(Malloc(0x150));
    if (spawners) {
        At<uint32_t>(spawners, 0) = 0;
        At<uint32_t>(spawners, 4) = 0;
        At<uint32_t>(spawners, 0xA8) = 0;
        At<uint32_t>(spawners, 0x14C) = 0;
        g_spawners = spawners;
        return;
    }
    g_spawners = nullptr;
}

// 0x4A2310: CMissionSpawnLocation's constructor (CSpawnLocation's inlined, then its
// vehicle and ped model lists)
void* __fastcall MissionSpawnLocationConstruct(uint8_t* self, re::Edx, int routeId, uint32_t triggerHash, void* spawner) {
    At<void*>(self, 4) = spawner;
    At<int32_t>(self, 8) = routeId;
    At<uint32_t>(self, 0) = vtblSpawnLocation;
    At<uint32_t>(self, 0xC) = triggerHash;
    At<void*>(self, 0x10) = nullptr;
    Triggers_FindByHash(TheTriggers, triggerHash);
    Route_First(Routes_GetById(TheRoutes, At<int32_t>(self, 8)));
    At<uint32_t>(self, 0) = vtblMissionSpawnLocation;
    ModelsInfo_Construct(self + 0x14);
    At<uint32_t>(self, 0x14) = vtblVehicleModelsInfo;
    ModelsInfo_Construct(self + 0x28);
    At<uint32_t>(self, 0x28) = vtblPedModelsInfo;
    return self;
}

// 0x4A2390 / 0x4A2440 / 0x4A2540 / 0x4A2570 / 0x4A25A0: operator delete of each pool
void __cdecl Pool20C7C6CDelete(void* object) {
    PoolDelete(pool20C7C6C, object);
}
void __cdecl MissionSpawnerDelete(void* object) {
    PoolDelete(missionSpawnerPool, object);
}
void __cdecl MissionSpawnLocationDelete(void* object) {
    PoolDelete(missionSpawnLocationPool, object);
}
void __cdecl AmbientSpawnerDelete(void* object) {
    PoolDelete(ambientSpawnerPool, object);
}
void __cdecl SpawnLocationDelete(void* object) {
    PoolDelete(spawnLocationPool, object);
}

// 0x4A23C0 (CSpawner vf2) / 0x4A2470 (CMissionSpawner vf2): a location added, at a
// route point and a trigger (by slots). (A negative trigger slot reads through
// nullptr, as the game does). Returns the location (scripts keep it)
void* __fastcall SpawnerAddLocation(uint8_t* self, re::Edx, int routeSlot, int triggerSlot) {
    uint8_t* route = static_cast<uint8_t*>(Routes_Get(TheRoutes, routeSlot));
    uint8_t* trigger = triggerSlot < 0 ? nullptr : PoolSlot(triggerPool, triggerSlot);
    uint32_t hash = At<uint32_t>(trigger, 0x2C);
    int routeId = At<int32_t>(route, 8);
    void* location = SpawnLocationPool_New(spawnLocationPool);
    location = location ? SpawnLocation_Construct(location, routeId, hash, self) : nullptr;
    At<void*>(self, 0x18 + At<int32_t>(self, 0xB8) * 4) = location;
    At<int32_t>(self, 0xB8)++;
    return location;
}
void* __fastcall MissionSpawnerAddLocation(uint8_t* self, re::Edx, int routeSlot, int triggerSlot) {
    uint8_t* route = static_cast<uint8_t*>(Routes_Get(TheRoutes, routeSlot));
    uint8_t* trigger = triggerSlot < 0 ? nullptr : PoolSlot(triggerPool, triggerSlot);
    uint32_t hash = At<uint32_t>(trigger, 0x2C);
    int routeId = At<int32_t>(route, 8);
    void* location = MissionSpawnLocationPool_New(missionSpawnLocationPool);
    location = location ? MissionSpawnLocation_Construct(location, routeId, hash, self) : nullptr;
    At<void*>(self, 0x18 + At<int32_t>(self, 0xB8) * 4) = location;
    At<int32_t>(self, 0xB8)++;
    return location;
}

// 0x4A24F0: every ped it may have is out, and none has come out (+0x170)
uint8_t __fastcall MissionSpawnerIsExhausted(uint8_t* self, re::Edx) {
    int32_t count = 0;
    for (int i = At<int32_t>(self, 0xB8) - 1; i >= 0; i--) {
        count += At<int32_t>(At<void*>(self, 0x18 + i * 4), 0x2C);
    }
    return count == At<int8_t>(self, 0xBC) && At<int32_t>(self, 0x170) == 0;
}

// 0x4A25D0: a ped of the model made at the location's route point (on the ground),
// facing its way; nullptr while the model loads (requested) or the location's prop is
// in use
uint8_t* __fastcall SpawnLocationSpawnPed(uint8_t* self, re::Edx, int model) {
    if (model == -1) {
        return nullptr;
    }
    if (GetStreamingInfo(model)[4] != 1) {
        RequestModel(model, 0x14);
        return nullptr;
    }
    uint8_t* trigger = FindTrigger(At<uint32_t>(self, 0xC));
    uint8_t* propAnim = static_cast<uint8_t*>(Trigger_GetPropAnim(trigger));
    if (propAnim && (At<int32_t>(propAnim, 0x184) != 0 || At<int32_t>(propAnim, 0x18) == 0)) {
        return nullptr;
    }
    void* route = Routes_GetById(TheRoutes, At<int32_t>(self, 8));
    const float* point = Route_First(route);
    float x = point[0];
    float y = point[1];
    float z = point[2];
    uint8_t found = 0;
    float groundZ = FindGroundZ(x, y, float(double(z) + 1.0), &found, 1);
    if (found) {
        z = float(double(groundZ) + double(0.01f));
    }
    void* memory = Ped_New(0x1E90);
    if (!memory) {
        return nullptr;
    }
    uint8_t* ped = CivilianPed_Construct(memory, model, 1, 1);
    if (ped) {
        Entity_SetPosition(ped, x, y, z);
        float heading = float((double(Route_First(route)[7]) - 90.0) * double(3.1415927f) / 180.0);
        Ped_SetHeading(ped, heading);
        At<uint8_t*>(self, 0x10) = ped;
        At<int32_t>(ped, 0x1390) = 1;
        VCall<void>(At<void*>(self, 4), 0xC, ped);   // the spawner told
        World_Add(ped);
        if (At<uint32_t>(self, 0xC) != 0 && propAnim) {
            PropAnims_AttachPed(propAnims, ped, propAnim);
        }
    }
    return ped;
}

// 0x4A27D0: the manager gone
void __cdecl SpawnersShutdown() {
    void* spawners = g_spawners;
    if (spawners) {
        Spawners_Destruct(spawners);
        Free(spawners);
        g_spawners = nullptr;
    }
}

// 0x4A2800: the manager made anew
void __cdecl SpawnersReinit() {
    void* old = g_spawners;
    if (old) {
        Spawners_Destruct(old);
        Free(old);
        g_spawners = nullptr;
    }
    uint8_t* spawners = static_cast<uint8_t*>(Malloc(0x150));
    if (spawners) {
        At<uint32_t>(spawners, 0) = 0;
        At<uint32_t>(spawners, 4) = 0;
        At<uint32_t>(spawners, 0xA8) = 0;
        At<uint32_t>(spawners, 0x14C) = 0;
        g_spawners = spawners;
        return;
    }
    g_spawners = nullptr;
}

// 0x4A2860: one spawned: from the vehicle models just the pick marked (no vehicle is
// made), else a ped of the spawner's kind (its flags 1 and 2)
void __fastcall MissionSpawnLocationSpawn(uint8_t* self, re::Edx) {
    if (At<int32_t>(self, 0x24) != 0) {
        int model = ModelsInfo_Pick(self + 0x14, -1, -1);
        ModelsInfo_Used(self + 0x14, model);
        return;
    }
    uint8_t flags = At<uint8_t>(At<uint8_t*>(self, 4), 0x14);
    // (the game passes the two flags as bytes in dwords whose upper bytes are stack
    // garbage)
    int flagB = (flags >> 1) & 1;
    int flagA = flags & 1;
    int model = PedModelsInfo_Pick(self + 0x28, flagA, flagB, 0);
    uint8_t* ped = SpawnLocation_SpawnPed(self, model);
    if (ped) {
        At<int32_t>(ped, 0x1390) = 1;
        ModelsInfo_Used(self + 0x28, At<int16_t>(ped, 0x10E));
    }
}

// 0x4A28E0: the spawner's locations updated, and one ped more when it's time and
// fewer are out than it may have (0: it had nothing left to pick from)
uint8_t __fastcall MissionSpawnerUpdate(uint8_t* self, re::Edx) {
    int32_t numLocations = At<int32_t>(self, 0xB8);
    if (numLocations == 0) {
        return 1;
    }
    for (int i = numLocations - 1; i >= 0; i--) {
        SpawnLocation_Update(At<void*>(self, 0x18 + i * 4));
    }
    if (!Spawner_IsTimeToSpawn(self)) {
        return 1;
    }
    int last = At<int32_t>(self, 0xB8) - 1;
    int32_t alive = 0;
    for (int i = last; i >= 0; i--) {
        alive += At<int32_t>(At<void*>(self, 0x18 + i * 4), 0x2C);
    }
    int32_t maxAlive = At<int8_t>(self, 0xBC);
    if (alive < maxAlive) {
        int32_t vehicles = 0;
        for (int i = last; i >= 0; i--) {
            vehicles += At<int32_t>(At<void*>(self, 0x18 + i * 4), 0x18);
        }
        if (vehicles < maxAlive) {
            if (At<uint32_t>(self, 0x170) < uint32_t(int32_t(At<int8_t>(self, 0xBD)))) {
                uint32_t pick = Spawner_PickLocation(self);
                if (pick != 0xFFFFFFFF) {
                    MissionSpawnLocation_Spawn(At<void*>(self, 0x18 + pick * 4));
                }
            }
            return 1;
        }
    }
    return Spawner_PickLocation(self) != 0xFFFFFFFF;
}

// 0x4A29B0: one mission spawner updated a frame, and one ambient spawner's locations
void __fastcall SpawnersUpdate(uint8_t* self, re::Edx) {
    if (At<uint32_t>(self, 0xA8) != 0) {
        MissionSpawner_Update(At<void*>(self, 8 + At<uint32_t>(self, 0) * 4));
        At<uint32_t>(self, 0)++;
        if (At<uint32_t>(self, 0) >= At<uint32_t>(self, 0xA8)) {
            At<uint32_t>(self, 0) = 0;
        }
    }
    if (At<uint32_t>(self, 0x14C) != 0) {
        uint8_t* spawner = At<uint8_t*>(self, 0xAC + At<uint32_t>(self, 4) * 4);
        for (int i = At<int32_t>(spawner, 0xB8) - 1; i >= 0; i--) {
            SpawnLocation_Update(At<void*>(spawner, 0x18 + i * 4));
        }
        At<uint32_t>(self, 4)++;
        if (At<uint32_t>(self, 4) >= At<uint32_t>(self, 0x14C)) {
            At<uint32_t>(self, 4) = 0;
        }
    }
}

// 0x4A2A30: the value kept within [min, max]
void __cdecl ClampFloat(float* value, float min, float max) {
    if (min <= *value) {
        if (*value <= max) {
            return;
        }
        *value = max;
        return;
    }
    *value = min;
}

} // namespace c09spawners

// ---- registration ---------------------------------------------------------------------

#define SPAWNERS_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

SPAWNERS_REVERSED("CPool<CAmbientSpawner>::New", 0x004A1FE0, &c09spawners::AmbientSpawnerPoolNew);
SPAWNERS_REVERSED("CMissionSpawner::Destruct", 0x004A2060, &c09spawners::MissionSpawnerDestruct);
SPAWNERS_REVERSED("CMissionSpawner::operator new", 0x004A20F0, &c09spawners::MissionSpawnerNew);
SPAWNERS_REVERSED("CSpawner::IsTimeToSpawn", 0x004A2100, &c09spawners::SpawnerIsTimeToSpawn);
SPAWNERS_REVERSED("CSpawner::CountAlive", 0x004A21B0, &c09spawners::SpawnerCountAlive);
SPAWNERS_REVERSED("CAmbientSpawner::operator new", 0x004A21E0, &c09spawners::AmbientSpawnerNew);
SPAWNERS_REVERSED("CSpawnLocation::Construct", 0x004A21F0, &c09spawners::SpawnLocationConstruct);
SPAWNERS_REVERSED("CSpawnLocation::Update", 0x004A2240, &c09spawners::SpawnLocationUpdate);
SPAWNERS_REVERSED("CSpawners::Init", 0x004A22D0, &c09spawners::SpawnersInit);
SPAWNERS_REVERSED("CMissionSpawnLocation::Construct", 0x004A2310, &c09spawners::MissionSpawnLocationConstruct);
SPAWNERS_REVERSED("CPool<0x20C7C6C>::operator delete", 0x004A2390, &c09spawners::Pool20C7C6CDelete);
SPAWNERS_REVERSED("CSpawner::AddLocation", 0x004A23C0, &c09spawners::SpawnerAddLocation);
SPAWNERS_REVERSED("CMissionSpawner::operator delete", 0x004A2440, &c09spawners::MissionSpawnerDelete);
SPAWNERS_REVERSED("CMissionSpawner::AddLocation", 0x004A2470, &c09spawners::MissionSpawnerAddLocation);
SPAWNERS_REVERSED("CMissionSpawner::IsExhausted", 0x004A24F0, &c09spawners::MissionSpawnerIsExhausted);
SPAWNERS_REVERSED("CMissionSpawnLocation::operator delete", 0x004A2540, &c09spawners::MissionSpawnLocationDelete);
SPAWNERS_REVERSED("CAmbientSpawner::operator delete", 0x004A2570, &c09spawners::AmbientSpawnerDelete);
SPAWNERS_REVERSED("CSpawnLocation::operator delete", 0x004A25A0, &c09spawners::SpawnLocationDelete);
SPAWNERS_REVERSED("CSpawnLocation::SpawnPed", 0x004A25D0, &c09spawners::SpawnLocationSpawnPed);
SPAWNERS_REVERSED("CSpawners::Shutdown", 0x004A27D0, &c09spawners::SpawnersShutdown);
SPAWNERS_REVERSED("CSpawners::Reinit", 0x004A2800, &c09spawners::SpawnersReinit);
SPAWNERS_REVERSED("CMissionSpawnLocation::Spawn", 0x004A2860, &c09spawners::MissionSpawnLocationSpawn);
SPAWNERS_REVERSED("CMissionSpawner::Update", 0x004A28E0, &c09spawners::MissionSpawnerUpdate);
SPAWNERS_REVERSED("CSpawners::Update", 0x004A29B0, &c09spawners::SpawnersUpdate);
SPAWNERS_REVERSED("ClampFloat", 0x004A2A30, &c09spawners::ClampFloat);
