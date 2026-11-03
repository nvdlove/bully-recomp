// The spawners' own functions 0x4A1150 .. 0x4A1F60 (Spawners.cpp has the rest and the
// layouts): CSpawner's (vtable 0x912308), CMissionSpawner's (0x912328, 0x174 bytes),
// CAmbientSpawner's (0x912340, 0x114 bytes), a spawn location's, the manager's
// (CSpawners, *0xC2C12C, 0x150 bytes: the mission spawners and the ambient ones) and two
// of their pools' New.
// A spawner's virtuals: vf0 the deleting destructor, vf1 (+4) a ped of its is gone, vf2
// (+8) add a location, vf3 (+0xC) a ped came out, vf4 (+0x10) may a location spawn.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/PedPopulation.h"
#include "game/Pool.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using Vec3 = pop::CVector;

template <typename R, typename... A>
R VCall(void* obj, int offset, A... args) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(obj))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(fn)(obj, args...);
}

struct CSpawner;

// A spawn location (CSpawnLocation 0x14 bytes; the mission one 0x3C, with its vehicle
// models at +0x14 and its ped models at +0x28)
struct CSpawnLocation {
    re::Ptr32<re::Ptr32<void>> vtable;       // +0x0
    re::Ptr32<CSpawner> spawner;   // +0x4
    int32_t routeId;     // +0x8 its route (point)'s id
    uint32_t trigger;    // +0xC its trigger's hash
    re::Ptr32<void> ped;           // +0x10 the ped it made last (nullptr: none about)
};
static_assert(sizeof(CSpawnLocation) == 0x14);

struct CSpawner {
    re::Ptr32<re::Ptr32<void>> vtable;                  // +0x00
    int32_t unknown04;              // +0x04
    int32_t delay;                  // +0x08 ms between peds
    int32_t nextTime;               // +0x0C CTimer ms (-1: not waiting)
    int32_t script;                 // +0x10 the owning script's id
    uint8_t flags;                  // +0x14 1, 2: which sexes; 4: off
    uint8_t unknown15[3];           // +0x15
    re::Ptr32<CSpawnLocation> locations[40];  // +0x18
    uint32_t numLocations;          // +0xB8
};
static_assert(offsetof(CSpawner, locations) == 0x18);
static_assert(sizeof(CSpawner) == 0xBC);

struct CMissionSpawner : CSpawner {
    int8_t maxAlive;          // +0xBC
    int8_t total;             // +0xBD
    uint8_t unknownBE[2];     // +0xBE
    uint32_t trigger;         // +0xC0 its trigger's hash (0: none)
    re::Ptr32<void> attackTarget;       // +0xC4 a registered reference (nullptr: the player)
    uint8_t missionFlags;     // +0xC8 1: on without a trigger; 2: attack (else go to) the target
    uint8_t unknownC9[3];     // +0xC9
    uint32_t callback;        // +0xCC the script's callback (a script function reference)
    re::Ptr32<void> peds[40];           // +0xD0 the peds out now
    uint32_t numPeds;         // +0x170
};
static_assert(offsetof(CMissionSpawner, maxAlive) == 0xBC);
static_assert(offsetof(CMissionSpawner, callback) == 0xCC);
static_assert(offsetof(CMissionSpawner, numPeds) == 0x170);
static_assert(sizeof(CMissionSpawner) == 0x174);

// minutes of the day, the end may be past midnight (then wrapped)
struct TimePeriod {
    int32_t start;  // +0
    int32_t end;    // +4
};

struct CAmbientSpawner : CSpawner {
    uint8_t ambientFlags;    // +0xBC 1: exclusive (a location may spawn with its ped about);
                             //        2: forced (then 4: on)
    uint8_t unknownBD[3];    // +0xBD
    TimePeriod periods[10];  // +0xC0
    uint32_t numPeriods;     // +0x110
};
static_assert(offsetof(CAmbientSpawner, periods) == 0xC0);
static_assert(sizeof(CAmbientSpawner) == 0x114);

struct CSpawners {
    uint32_t missionCursor;          // +0x000
    uint32_t ambientCursor;          // +0x004
    re::Ptr32<CMissionSpawner> mission[40];    // +0x008
    uint32_t numMission;             // +0x0A8
    re::Ptr32<CAmbientSpawner> ambient[40];    // +0x0AC
    uint32_t numAmbient;             // +0x14C
};
static_assert(offsetof(CSpawners, numMission) == 0xA8);
static_assert(offsetof(CSpawners, ambient) == 0xAC);
static_assert(sizeof(CSpawners) == 0x150);

constexpr uintptr_t vtblSpawner = 0x00912308;
constexpr uintptr_t vtblMissionSpawner = 0x00912328;
constexpr uintptr_t vtblAmbientSpawner = 0x00912340;

GAME_VAR(CPedPopulation*, ThePopulation, 0x00C2C108);
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);
GAME_VAR(int32_t, timeInMs, 0x00C1A9B4);      // CTimer's (compared signed here)
GAME_VAR(uint32_t, frameCounter, 0x00C1A99C);
GAME_VAR(uint8_t, clockMinutes, 0x00BCEB4A);  // CClock's
GAME_VAR(uint8_t, clockHours, 0x00BCEB4B);
inline void* const TheRoutes = reinterpret_cast<void*>(0x00BC5E7C);     // the route points
inline void* const TheCamera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const TheLurStore = reinterpret_cast<void*>(0x00D02850);   // the scripts

GAME_FN(void*, __thiscall, Routes_GetById, 0x006D64D0, void* routes, int id);
GAME_FN(Vec3*, __thiscall, Route_First, 0x006D59B0, void* route);
GAME_FN(Vec3*, __cdecl, VecSub, 0x00414B70, Vec3* out, const Vec3* a, const Vec3* b);   // a - b
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, void* entity, void* ref);
GAME_FN(int, __thiscall, PedModelsInfo_Pick, 0x00464E70, void* info, int flagA, int flagB, int);
GAME_FN(void, __thiscall, PedModelsInfo_AddModel, 0x00464A50, void* info, int model);
GAME_FN(void, __thiscall, PedModelsInfo_AddPedType, 0x00464800, void* info, int pedType);
GAME_FN(uint8_t, __thiscall, TimePeriod_Contains, 0x0045B870, const TimePeriod* period, int minute);
GAME_FN(int, __thiscall, Camera_IsSphereVisible, 0x004F13D0, void* camera, const Vec3* centre, float radius);
inline void* __cdecl Memset(void* p, int value, size_t n) { return crt::Memset(p, value, n); }
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
// the scripts
GAME_FN(void*, __thiscall, LurStore_FindScript, 0x005DB8F0, void* store, int script);
GAME_FN(void, __thiscall, LurStore_SetCurrentScript, 0x005DBDA0, void* store, void* script);
GAME_FN(uint8_t, __thiscall, ScriptRef_IsSet, 0x005D80E0, uint32_t* ref);
GAME_FN(int, __thiscall, ScriptRef_GetFunction, 0x005D80D0, uint32_t* ref);
GAME_FN(void*, __thiscall, ScriptRef_GetState, 0x005D9C20, uint32_t* ref);
GAME_FN(void, __cdecl, Lua_Fn_5D8270, 0x005D8270, void* state);
GAME_FN(void, __cdecl, Lua_Fn_5D8670, 0x005D8670, void* state, int function);
GAME_FN(void, __cdecl, Lua_PushInt, 0x005D85C0, void* state, int value);
GAME_FN(void, __cdecl, Lua_PushPointer, 0x005D85A0, void* state, void* value);
GAME_FN(void, __cdecl, Lua_Call, 0x005D82A0, void* state, int args, int results);
// a ped's objective to go to (1) or attack (3) a target (0x1C bytes) and the ped's list
GAME_FN(void*, __thiscall, TargetObjective_Construct, 0x00470D60, void* objective, void* ped, void* target, int mode);
GAME_FN(void, __thiscall, PedObjectives_Add, 0x00471390, void* objectives, void* objective);
// (this file's, called where modes apply)
GAME_FN(uint8_t, __thiscall, GameFn_AmbientSpawner_IsActive, 0x004A1750, void* spawner);
GAME_FN(CSpawnLocation*, __thiscall, GameFn_Spawner_FindNearLocation, 0x004A1360, void* spawner, const Vec3* pos);

const Vec3* EntityPosition(uint8_t* entity) {
    return pop::PosOf(entity);
}

// a spawn location's position (its route point's)
Vec3 LocationPosition(const CSpawnLocation* location) {
    return *Route_First(Routes_GetById(TheRoutes, location->routeId));
}

// rand() scaled to 0..n (the game's multiply and divide by 32767)
uint32_t RandomUpTo(uint32_t n) {
    return uint32_t(int(Rand() * int(n)) / 32767);
}

// |x² + y²| (2D) and its root, as the game rounds them
float Length2D(const Vec3& v) {
    float sq = std::fabs(float(double(v.x) * v.x + double(v.y) * v.y));
    return float(std::sqrt(double(sq)));
}

void Delete(void* object) {
    VCall<void>(object, 0, 1);
}

} // namespace

namespace spawner_manager {

// ---- CSpawner

// 0x4A1150 (vf4): a location may spawn when its last ped isn't about
uint8_t __fastcall Spawner_CanUseLocation(CSpawner*, re::Edx, CSpawnLocation* location) {
    return location->ped == nullptr;
}

// 0x4A1220: the constructor (the first argument unused)
CSpawner* __fastcall Spawner_Construct(CSpawner* self, re::Edx, int, int unknown04, int delay, int script) {
    self->vtable = reinterpret_cast<re::Ptr32<void>*>(vtblSpawner);
    self->unknown04 = unknown04;
    self->delay = delay;
    self->nextTime = -1;
    self->script = script;
    self->flags = (self->flags & 0xFB) | 3;
    self->numLocations = 0;
    return self;
}

// 0x4A1260: the destructor's body (the locations deleted)
void __fastcall Spawner_Destruct(CSpawner* self, re::Edx) {
    self->vtable = reinterpret_cast<re::Ptr32<void>*>(vtblSpawner);
    for (uint32_t i = 0; i < self->numLocations; i++) {
        if (self->locations[i] != nullptr) {
            Delete(self->locations[i]);
        }
    }
    self->numLocations = 0;
}

// 0x4A12B0: a location that may spawn, from a random one round (-1: none)
int __fastcall Spawner_PickLocation(CSpawner* self, re::Edx) {
    if (self->numLocations == 0) {
        return -1;
    }
    uint32_t start = RandomUpTo(self->numLocations - 1);
    uint32_t i = start;
    do {
        if (VCall<uint8_t>(self, 0x10, self->locations[i])) {
            return int(i);
        }
        if (++i >= self->numLocations) {
            i = 0;
        }
    } while (i != start);
    return -1;
}

// 0x4A1320 (vf3): a ped came out: the next one after the delay
void __fastcall Spawner_PedSpawned(CSpawner* self, re::Edx, void*) {
    self->nextTime = self->delay + timeInMs;
}

// 0x4A1330 (vf1): a ped is gone: the locations that made it may spawn again
void __fastcall Spawner_PedGone(CSpawner* self, re::Edx, void* ped) {
    for (int i = int(self->numLocations) - 1; i >= 0; i--) {
        if (self->locations[i]->ped == ped) {
            self->locations[i]->ped = nullptr;
        }
    }
}

// 0x4A1360: of the locations level with pos (within 2 up or down), the nearest or the
// second nearest (2D; frame by frame in turn). The first two start at 500000/500001.
CSpawnLocation* __fastcall Spawner_FindNearLocation(CSpawner* self, re::Edx, const Vec3* pos) {
    float nearest = 500000.0f;
    float second = 500001.0f;
    CSpawnLocation* found[2] = {nullptr, nullptr};   // the nearest, the second
    for (uint32_t i = 0; i < self->numLocations; i++) {
        CSpawnLocation* location = self->locations[i];
        if (Routes_GetById(TheRoutes, location->routeId) == nullptr) {
            continue;
        }
        Vec3 at = LocationPosition(location);
        Vec3 d;
        VecSub(&d, &at, pos);
        float dist = Length2D(d);
        if (!(std::fabs(d.z) < 2.0f)) {
            continue;
        }
        if (second > dist) {
            if (nearest > dist) {
                second = nearest;
                nearest = dist;
                found[1] = found[0];
                found[0] = location;
            } else {
                second = dist;
                found[1] = location;
            }
        }
    }
    return found[frameCounter & 1];
}

// ---- spawn locations

// 0x4A1190: where it is (its route point's position)
Vec3* __fastcall SpawnLocation_GetPosition(CSpawnLocation* self, re::Edx, Vec3* out) {
    *out = LocationPosition(self);
    return out;
}

// 0x4A11C0: a ped model from a type's list, of the sexes its spawner allows
int __fastcall SpawnLocation_PickModel(CSpawnLocation* self, re::Edx, void* pedModels, int arg) {
    uint8_t flags = self->spawner->flags;
    // (the game builds the two flags as bytes in uninitialised dwords)
    return PedModelsInfo_Pick(pedModels, flags & 1, (flags >> 1) & 1, arg);
}

// 0x4A1200 / 0x4A1210: a mission location's ped models (its list at +0x28)
void __fastcall MissionSpawnLocation_AddPedModelId(uint8_t* self, re::Edx, int model) {
    PedModelsInfo_AddModel(self + 0x28, model);
}

void __fastcall MissionSpawnLocation_AddPedType(uint8_t* self, re::Edx, int pedType) {
    PedModelsInfo_AddPedType(self + 0x28, pedType);
}

// ---- CMissionSpawner

// 0x4A1160: the target its peds go for (flag: attack it; nullptr: the player)
void __fastcall MissionSpawner_SetAttackTarget(CMissionSpawner* self, re::Edx, void* ped, int attack) {
    self->missionFlags = uint8_t((self->missionFlags & ~2) | ((attack << 1) & 2));
    self->attackTarget = ped;
    RegisterReference(ped, &self->attackTarget);
}

// 0x4A14C0: the constructor (CSpawner's inlined; the fifth argument unused)
CMissionSpawner* __fastcall MissionSpawner_Construct(CMissionSpawner* self, re::Edx, int maxAlive, int total, int script,
                                                     uint32_t trigger, int, int unknown04, int delay) {
    self->delay = delay;
    self->unknown04 = unknown04;
    self->flags = (self->flags & 0xFB) | 3;
    self->script = script;
    self->nextTime = -1;
    self->numLocations = 0;
    self->missionFlags &= 0xFC;
    self->maxAlive = int8_t(maxAlive);
    self->total = int8_t(total);
    self->vtable = reinterpret_cast<re::Ptr32<void>*>(vtblMissionSpawner);
    self->trigger = trigger;
    self->attackTarget = nullptr;
    self->callback = 0;
    self->numPeds = 0;
    return self;
}

// 0x4A1540 (vf1): a ped of its is gone (the delay restarts when all were out)
void __fastcall MissionSpawner_PedGone(CMissionSpawner* self, re::Edx, void* ped) {
    for (uint32_t i = 0; i < self->numPeds; i++) {
        if (self->peds[i] == ped) {
            if (self->numPeds == uint32_t(int(self->total))) {
                self->nextTime = self->delay + timeInMs;
            }
            self->peds[i] = self->peds[self->numPeds - 1];
            self->numPeds--;
            break;
        }
    }
    for (int i = int(self->numLocations) - 1; i >= 0; i--) {
        if (self->locations[i]->ped == ped) {
            self->locations[i]->ped = nullptr;
        }
    }
}

// 0x4A15D0 (vf3): a ped came out: the script's callback called with it (its handle
// and the spawner), else it's sent at the target
void __fastcall MissionSpawner_PedSpawned(CMissionSpawner* self, re::Edx, uint8_t* ped) {
    pop::At<CMissionSpawner*>(ped, 0x1B8) = self;
    self->peds[self->numPeds] = ped;
    self->numPeds++;
    void* script = LurStore_FindScript(TheLurStore, self->script);
    if (script != nullptr && ScriptRef_IsSet(&self->callback)) {
        LurStore_SetCurrentScript(TheLurStore, script);
        void* state = ScriptRef_GetState(&self->callback);
        Lua_Fn_5D8270(state);
        Lua_Fn_5D8670(state, ScriptRef_GetFunction(&self->callback));
        Lua_PushInt(state, VCall<int>(ped, 0xAC));
        Lua_PushPointer(state, self);
        Lua_Call(state, 2, 0);
        LurStore_SetCurrentScript(TheLurStore, nullptr);
        self->nextTime = self->delay + timeInMs;
        return;
    }
    void* target = self->attackTarget ? self->attackTarget : playerPed;
    int mode = (self->missionFlags & 2) ? 3 : 1;
    void* memory = Malloc(0x1C);
    void* objective = memory ? TargetObjective_Construct(memory, ped, target, mode) : nullptr;
    PedObjectives_Add(ped + 0x5F0, objective);
    self->nextTime = self->delay + timeInMs;
}

// ---- CAmbientSpawner

// 0x4A1700: the constructor (CSpawner's inlined; the first two arguments unused)
CAmbientSpawner* __fastcall AmbientSpawner_Construct(CAmbientSpawner* self, re::Edx, int, int, int unknown04, int delay,
                                                     int script) {
    self->delay = delay;
    self->unknown04 = unknown04;
    self->script = script;
    self->nextTime = -1;
    self->flags = (self->flags & 0xFB) | 3;
    self->numLocations = 0;
    self->ambientFlags &= 0xFE;
    self->vtable = reinterpret_cast<re::Ptr32<void>*>(vtblAmbientSpawner);
    self->numPeriods = 0;
    return self;
}

// 0x4A1750: on (not off; forced, or past its delay and in one of its periods)
uint8_t __fastcall AmbientSpawner_IsActive(CAmbientSpawner* self, re::Edx) {
    if (self->flags & 4) {
        return 0;
    }
    if (self->ambientFlags & 2) {
        return (self->ambientFlags >> 2) & 1;
    }
    if (timeInMs <= self->nextTime) {
        return 0;
    }
    int minute = clockHours * 60 + clockMinutes;
    for (int i = int(self->numPeriods) - 1; i >= 0; i--) {
        if (TimePeriod_Contains(&self->periods[i], minute)) {
            return 1;
        }
    }
    return 0;
}

// 0x4A17D0: a period of the day (minutes from midnight; the end wraps past 1440)
void __fastcall AmbientSpawner_AddPeriod(CAmbientSpawner* self, re::Edx, int start, int minutes) {
    int end = start + minutes;
    if (end > 1440) {
        end -= 1440;
    }
    self->periods[self->numPeriods].start = start;
    self->periods[self->numPeriods].end = end;
    self->numPeriods++;
}

// 0x4A1810 (vf4): a location may spawn when it's level enough with the player and
// within the population's range (the in-view one if the camera sees it)
uint8_t __fastcall AmbientSpawner_CanUseLocation(CAmbientSpawner*, re::Edx, CSpawnLocation* location) {
    uint8_t* player = playerPed;
    Vec3 at = LocationPosition(location);
    Vec3 d;
    VecSub(&d, EntityPosition(player), &at);
    CPedPopulation* population = ThePopulation;
    if (population == nullptr) {
        return 0;
    }
    if (population->interiorHeight < std::fabs(d.z)) {
        return 0;
    }
    float sq = std::fabs(float(double(d.x) * d.x + double(d.y) * d.y + double(d.z) * d.z));
    float dist = float(std::sqrt(double(sq)));
    Vec3 centre = at;
    float range = Camera_IsSphereVisible(TheCamera, &centre, 2.0f) ? population->cull[2] : population->cull[5];
    return range > dist ? 1 : 0;
}

// ---- CSpawners

// 0x4A1940: the mission spawners deleted
void __fastcall Spawners_DeleteMission(CSpawners* self, re::Edx) {
    for (int i = int(self->numMission) - 1; i >= 0; i--) {
        if (self->mission[i] != nullptr) {
            Delete(self->mission[i]);
        }
    }
    self->numMission = 0;
}

// 0x4A1980: all deleted
void __fastcall Spawners_ClearAll(CSpawners* self, re::Edx) {
    for (int i = int(self->numMission) - 1; i >= 0; i--) {
        if (self->mission[i] != nullptr) {
            Delete(self->mission[i]);
        }
    }
    self->numMission = 0;
    for (int i = int(self->numAmbient) - 1; i >= 0; i--) {
        if (self->ambient[i] != nullptr) {
            Delete(self->ambient[i]);
        }
    }
    self->numAmbient = 0;
    self->missionCursor = 0;
    self->ambientCursor = 0;
}

// 0x4A1A00 / 0x4A1A80: a spawner by pointer (a script's userdata), if it's one of these
CMissionSpawner* __fastcall Spawners_GetMission(CSpawners* self, re::Edx, CMissionSpawner* spawner) {
    for (uint32_t i = 0; i < self->numMission; i++) {
        if (self->mission[i] == spawner) {
            return spawner;
        }
    }
    return nullptr;
}

CAmbientSpawner* __fastcall Spawners_GetAmbient(CSpawners* self, re::Edx, CAmbientSpawner* spawner) {
    for (uint32_t i = 0; i < self->numAmbient; i++) {
        if (self->ambient[i] == spawner) {
            return spawner;
        }
    }
    return nullptr;
}

// 0x4A1AC0: either kind
CSpawner* __fastcall Spawners_GetSpawner(CSpawners* self, re::Edx, CSpawner* spawner) {
    for (uint32_t i = 0; i < self->numMission; i++) {
        if (self->mission[i] == spawner) {
            if (spawner != nullptr) {
                return spawner;
            }
            break;
        }
    }
    for (uint32_t i = 0; i < self->numAmbient; i++) {
        if (self->ambient[i] == spawner) {
            return spawner;
        }
    }
    return nullptr;
}

// 0x4A1A30 / 0x4A1A60
void __fastcall Spawners_AddMission(CSpawners* self, re::Edx, CMissionSpawner* spawner) {
    self->mission[self->numMission] = spawner;
    self->numMission++;
    if (int(self->missionCursor) < 0) {
        self->missionCursor = 0;
    }
}

void __fastcall Spawners_AddAmbient(CSpawners* self, re::Edx, CAmbientSpawner* spawner) {
    self->ambient[self->numAmbient] = spawner;
    self->numAmbient++;
}

// 0x4A1B20: one deleted (the last takes its place)
void __fastcall Spawners_Remove(CSpawners* self, re::Edx, CSpawner* spawner) {
    for (int i = int(self->numMission) - 1; i >= 0; i--) {
        if (self->mission[i] == spawner) {
            if (self->mission[i] != nullptr) {
                Delete(self->mission[i]);
            }
            self->mission[i] = self->mission[self->numMission - 1];
            self->numMission--;
            if (self->missionCursor >= self->numMission) {
                self->missionCursor = 0;
            }
            return;
        }
    }
    for (int i = int(self->numAmbient) - 1; i >= 0; i--) {
        if (self->ambient[i] == spawner) {
            if (self->ambient[i] != nullptr) {
                Delete(self->ambient[i]);
            }
            self->ambient[i] = self->ambient[self->numAmbient - 1];
            self->numAmbient--;
            if (self->ambientCursor >= self->numAmbient) {
                self->ambientCursor = 0;
            }
            return;
        }
    }
}

// 0x4A1C00: a script's spawners deleted (its id at +0x44)
void __fastcall Spawners_ClearForScript(CSpawners* self, re::Edx, uint8_t* script) {
    if (script != nullptr) {
        for (uint32_t i = 0; i < self->numMission;) {
            if (self->mission[i]->script == pop::At<int32_t>(script, 0x44)) {
                Delete(self->mission[i]);
                self->mission[i] = self->mission[self->numMission - 1];
                self->numMission--;
            } else {
                i++;
            }
        }
        for (uint32_t i = 0; i < self->numAmbient;) {
            if (self->ambient[i]->script == pop::At<int32_t>(script, 0x44)) {
                Delete(self->ambient[i]);
                self->ambient[i] = self->ambient[self->numAmbient - 1];
                self->numAmbient--;
            } else {
                i++;
            }
        }
    }
    if (self->missionCursor >= self->numMission) {
        self->missionCursor = 0;
    }
    if (self->ambientCursor >= self->numAmbient) {
        self->ambientCursor = 0;
    }
}

// 0x4A1CE0: an ambient spawner's location near the player to spawn at, from a random
// spawner on (to the list's end; 32 tries of active ones at most). Exclusive spawners
// don't mind the location's last ped still being about.
CSpawnLocation* __fastcall Spawners_PickAmbientLocation(CSpawners* self, re::Edx) {
    if (self->numAmbient == 0) {
        return nullptr;
    }
    uint32_t i = RandomUpTo(self->numAmbient - 1);
    int tries = 0;
    for (; i < self->numAmbient; i++) {
        CAmbientSpawner* spawner = self->ambient[i];
        if (!GameFn_AmbientSpawner_IsActive(spawner)) {
            continue;
        }
        CSpawnLocation* location = GameFn_Spawner_FindNearLocation(spawner, EntityPosition(playerPed));
        if (location != nullptr && ((self->ambient[i]->ambientFlags & 1) || location->ped == nullptr)) {
            return location;
        }
        if (tries++ > 30) {
            break;
        }
    }
    return nullptr;
}

// 0x4A1D90: the ambient spawners with a location within radius of pos (2D) turned on
// (on) or off
void __fastcall Spawners_SwitchAmbientNear(CSpawners* self, re::Edx, const Vec3* pos, float radius, int on) {
    for (uint32_t i = 0; i < self->numAmbient; i++) {
        CAmbientSpawner* spawner = self->ambient[i];
        for (int j = 0; j < int(spawner->numLocations); j++) {
            CSpawnLocation* location = self->ambient[i]->locations[j];
            if (Routes_GetById(TheRoutes, location->routeId) == nullptr) {
                continue;
            }
            Vec3 at = LocationPosition(location);
            Vec3 d;
            VecSub(&d, pos, &at);
            if (radius > Length2D(d)) {
                uint8_t off = re::Flag(on) ? 0 : 4;
                location->spawner->flags = (location->spawner->flags & ~4) | off;
            }
        }
    }
}

// ---- pools

// 0x4A1EE0 / 0x4A1F60: CPool<CMissionSpawner> / CPool<CMissionSpawnLocation>::New: the
// next free slot from the last used on (once round), its reuse count bumped, zeroed
template <size_t Size>
void* PoolNew(CPool<uint8_t>* pool) {
    uint8_t wrapped = 0;
    if (!(pool->flags[pool->lastUsed] & 0x80)) {
        do {
            if (++pool->lastUsed >= pool->size) {
                if (wrapped) {
                    return nullptr;
                }
                pool->lastUsed = 0;
                wrapped = 1;
            }
        } while (!(pool->flags[pool->lastUsed] & 0x80));
    }
    uint8_t& flag = pool->flags[pool->lastUsed];
    flag &= 0x7F;
    flag = uint8_t((flag & 0x80) | ((flag + 1) & 0x7F));
    uint8_t* object = pool->objects + pool->elemSize * pool->lastUsed;
    Memset(object, 0, Size);
    return object;
}

void* __fastcall MissionSpawnerPool_New(CPool<uint8_t>* self, re::Edx) {
    return PoolNew<0x174>(self);
}

void* __fastcall MissionSpawnLocationPool_New(CPool<uint8_t>* self, re::Edx) {
    return PoolNew<0x3C>(self);
}

} // namespace spawner_manager

#define C08P6_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

using namespace spawner_manager;
C08P6_REVERSED("CSpawner::CanUseLocation", 0x004A1150, &Spawner_CanUseLocation);
C08P6_REVERSED("CMissionSpawner::SetAttackTarget", 0x004A1160, &MissionSpawner_SetAttackTarget);
C08P6_REVERSED("CSpawnLocation::GetPosition", 0x004A1190, &SpawnLocation_GetPosition);
C08P6_REVERSED("CSpawnLocation::PickModel", 0x004A11C0, &SpawnLocation_PickModel);
C08P6_REVERSED("CMissionSpawnLocation::AddPedModelId", 0x004A1200, &MissionSpawnLocation_AddPedModelId);
C08P6_REVERSED("CMissionSpawnLocation::AddPedType", 0x004A1210, &MissionSpawnLocation_AddPedType);
C08P6_REVERSED("CSpawner::CSpawner", 0x004A1220, &Spawner_Construct);
C08P6_REVERSED("CSpawner::~CSpawner", 0x004A1260, &Spawner_Destruct);
C08P6_REVERSED("CSpawner::PickLocation", 0x004A12B0, &Spawner_PickLocation);
C08P6_REVERSED("CSpawner::PedSpawned", 0x004A1320, &Spawner_PedSpawned);
C08P6_REVERSED("CSpawner::PedGone", 0x004A1330, &Spawner_PedGone);
C08P6_REVERSED("CSpawner::FindNearLocation", 0x004A1360, &Spawner_FindNearLocation);
C08P6_REVERSED("CMissionSpawner::CMissionSpawner", 0x004A14C0, &MissionSpawner_Construct);
C08P6_REVERSED("CMissionSpawner::PedGone", 0x004A1540, &MissionSpawner_PedGone);
C08P6_REVERSED("CMissionSpawner::PedSpawned", 0x004A15D0, &MissionSpawner_PedSpawned);
C08P6_REVERSED("CAmbientSpawner::CAmbientSpawner", 0x004A1700, &AmbientSpawner_Construct);
C08P6_REVERSED("CAmbientSpawner::IsActive", 0x004A1750, &AmbientSpawner_IsActive);
C08P6_REVERSED("CAmbientSpawner::AddPeriod", 0x004A17D0, &AmbientSpawner_AddPeriod);
C08P6_REVERSED("CAmbientSpawner::CanUseLocation", 0x004A1810, &AmbientSpawner_CanUseLocation);
C08P6_REVERSED("CSpawners::DeleteMission", 0x004A1940, &Spawners_DeleteMission);
C08P6_REVERSED("CSpawners::ClearAll", 0x004A1980, &Spawners_ClearAll);
C08P6_REVERSED("CSpawners::GetMission", 0x004A1A00, &Spawners_GetMission);
C08P6_REVERSED("CSpawners::AddMission", 0x004A1A30, &Spawners_AddMission);
C08P6_REVERSED("CSpawners::AddAmbient", 0x004A1A60, &Spawners_AddAmbient);
C08P6_REVERSED("CSpawners::GetAmbient", 0x004A1A80, &Spawners_GetAmbient);
C08P6_REVERSED("CSpawners::GetSpawner", 0x004A1AC0, &Spawners_GetSpawner);
C08P6_REVERSED("CSpawners::Remove", 0x004A1B20, &Spawners_Remove);
C08P6_REVERSED("CSpawners::ClearForScript", 0x004A1C00, &Spawners_ClearForScript);
C08P6_REVERSED("CSpawners::PickAmbientLocation", 0x004A1CE0, &Spawners_PickAmbientLocation);
C08P6_REVERSED("CSpawners::SwitchAmbientNear", 0x004A1D90, &Spawners_SwitchAmbientNear);
C08P6_REVERSED("CPool<CMissionSpawner>::New", 0x004A1EE0, &MissionSpawnerPool_New);
C08P6_REVERSED("CPool<CMissionSpawnLocation>::New", 0x004A1F60, &MissionSpawnLocationPool_New);
