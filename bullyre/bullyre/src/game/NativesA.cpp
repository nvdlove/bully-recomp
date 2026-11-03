// Script natives 0x5B6050..0x5BFFFF (areas, POIs, spawners, dockers, triggers and
// doors, blips and the radar, the camera, clock and weather, clothing and shops,
// effects, event handlers, garages, HUD, items, restart points, threads, missions):
// what the Lua scripts call, registered with the game's Lua under their script names
// (the name here is Native_<script name>). Each reads its arguments with the Script
// helpers (0-based) and returns how many results it pushed (a few return 1 with
// nothing pushed, as the game does). See ScriptApi.h.
//
// Not here: HasBallTouchedSocketRadius (0x5BCD70) and CreatePersistentEntity
// (0x5BD1F0), both long.

#include <cmath>
#include <cstdint>

#include "game/Pool.h"
#include "game/ScriptApi.h"

#include "game/Crt.h"
#include "reversed.h"
namespace {

// A field of a game object not reversed yet
using re::At;

// ---- globals
GAME_VAR(void*, g_population, 0x00C2C108);      // the population manager (ambient peds)
GAME_VAR(void*, g_patrolPaths, 0x00C2B034);     // the patrol path manager
GAME_VAR(void*, g_patrolPathPoolOwner, 0x00D13CB4);  // (passed as `this` to PatrolPath_Get, unused there)
GAME_VAR(void*, g_spawners, 0x00C2C12C);        // the (ambient and mission) spawner manager
GAME_VAR(void*, g_dockers, 0x00C2AB68);         // the docker manager (peds going into buildings)
GAME_VAR(void*, g_poiManager, 0x00C0F5E4);      // the points of interest
GAME_VAR(void*, g_punishment, 0x00C0F78C);      // the punishment (alert level) settings
inline void* const TheLurStore = reinterpret_cast<void*>(0x00D02850);   // the scripts
inline void* const g_specialEntities = reinterpret_cast<void*>(0x00BA6C4C);
inline void* const g_areaTransition = reinterpret_cast<void*>(0x00BCC580);
GAME_VAR(int32_t, g_streamingState, 0x00CA399C);     // (busy above -200)
GAME_VAR(uint8_t, g_streamingBusy, 0x00A957D4);
GAME_VAR(int32_t, ms_numModelsRequested, 0x00CA3988);
GAME_VAR(uint8_t, g_areaTransitionReadyToLoad, 0x00BCCDF0);
GAME_VAR(uint8_t, g_disableCameraControlForTransition, 0x00BCCDDF);
GAME_VAR(uint8_t, g_forceLoadAreaByAreaTransition, 0x00BCCDE0);
GAME_VAR(void*, g_transitionTimes, 0x020C8910);    // +0x18 / +0x1C: seconds, fade out / in
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_ARRAY(float, g_poiDefaultPosition, 0x00BCC564);   // 3 floats
GAME_ARRAY(float, g_playerPositionBeforeStore, 0x00BCCE20);   // x, y, z, heading (+0xC)
GAME_VAR(int32_t, g_playerAreaBeforeStore, 0x00BCCE30);

// ---- the game's functions
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
// TheLurStore: the script now running (nullptr outside one)
GAME_FN(void*, __thiscall, LurStore_GetCurrentScript, 0x005DB940, void* store);
// Script::: argument i (a function) as a registry reference in *ref (the old one released)
GAME_FN(void, __cdecl, Script_StoreFunctionRef, 0x005D9FD0, lua_State* L, int i, int* ref);

// population
GAME_FN(void, __thiscall, Population_ActivateTrigger, 0x0049EF60, void* pop, int trigger);
GAME_FN(void, __thiscall, Population_DeactivateTrigger, 0x0049EFA0, void* pop, int trigger);
GAME_FN(void, __thiscall, Population_DeteriorateAttitudeForTriggerPedType, 0x0049EFE0, void* pop, int trigger, int pedType);
GAME_FN(void, __thiscall, Population_Override, 0x0049C5B0, void* pop, int* counts);   // [0] the total, [1..12] per ped type
GAME_FN(void, __thiscall, Population_RevertToDefault, 0x0049C6D0, void* pop);
GAME_FN(void, __thiscall, Population_49EF00, 0x0049EF00, void* pop, ScriptVector* position);
// GTA's FindPlayerCoors: the player's (or its vehicle's) position into out; returns out
GAME_FN(ScriptVector*, __cdecl, FindPlayerCoors, 0x00446900, ScriptVector* out);

// patrol paths
GAME_FN(void*, __thiscall, PatrolPath_Get, 0x005ED3A0, void* unused, int slot);   // (its own pool, 0x20C7C50)
GAME_FN(void, __thiscall, PatrolPaths_Disable, 0x004723A0, void* paths, void* path);
GAME_FN(void, __thiscall, PatrolPaths_Enable, 0x004720D0, void* paths, void* path);
GAME_FN(void, __thiscall, PatrolPaths_ClearForScript, 0x004729D0, void* paths, void* script);
GAME_FN(void, __thiscall, PatrolPaths_ClearAll, 0x00472AF0, void* paths);
GAME_FN(void, __thiscall, PatrolPaths_SetActiveHours, 0x00471F40, void* paths, void* path, int from, int to);
GAME_FN(void, __thiscall, PatrolPaths_Init, 0x00472120, void* paths, void* path, int a, int b, int c, uint8_t d);
GAME_FN(void, __thiscall, PatrolPaths_DisableAll, 0x00472860, void* paths);
GAME_FN(void, __thiscall, PatrolPaths_EnableAll, 0x004722F0, void* paths);
GAME_FN(void, __thiscall, PatrolPaths_ShowVisionCones, 0x00471E00, void* paths, uint8_t show);

// spawners
GAME_FN(void, __thiscall, Spawners_ClearForScript, 0x004A1C00, void* spawners, void* script);
GAME_FN(void, __thiscall, Spawners_ClearAll, 0x004A1980, void* spawners);
GAME_FN(void*, __cdecl, AmbientSpawner_New, 0x004A21E0, uint32_t size);   // from its pool
GAME_FN(void*, __thiscall, AmbientSpawner_Construct, 0x004A1700, void* spawner, int a, int b, int c, int d, int scriptId);
GAME_FN(void, __thiscall, Spawners_Add, 0x004A1A60, void* spawners, void* spawner);
GAME_FN(void*, __thiscall, Spawners_GetAmbient, 0x004A1A80, void* spawners, void* userdata);
GAME_FN(void*, __thiscall, Spawners_GetSpawner, 0x004A1AC0, void* spawners, void* userdata);
GAME_FN(void*, __thiscall, Spawners_GetMission, 0x004A1A00, void* spawners, void* userdata);
GAME_FN(void, __thiscall, Spawners_Remove, 0x004A1B20, void* spawners, void* userdata);
GAME_FN(void, __thiscall, AmbientSpawner_AddPeriod, 0x004A17D0, void* spawner, int startMinute, int minutes);
GAME_FN(void, __thiscall, SpawnLocation_AddPedModelId, 0x004A1200, void* location, int model);
GAME_FN(void, __thiscall, SpawnLocation_AddPedType, 0x004A1210, void* location, int pedType);
GAME_FN(uint8_t, __thiscall, MissionSpawner_IsExhausted, 0x004A24F0, void* spawner);
GAME_FN(uint8_t, __thiscall, MissionSpawner_4A21B0, 0x004A21B0, void* spawner);
GAME_FN(void, __thiscall, MissionSpawner_SetAttackTarget, 0x004A1160, void* spawner, void* ped, uint8_t flag);
// a ped from a script handle (kind 3: the player; nullptr for -1 or a stale handle)
GAME_FN(void*, __cdecl, GetPedFromHandle, 0x005C7380, int handle, int kind);

// dockers
GAME_FN(void, __thiscall, Dockers_ClearForScript, 0x0046EBC0, void* dockers, void* script);
GAME_FN(void, __thiscall, Dockers_ClearAll, 0x0046EC30, void* dockers);
GAME_FN(void*, __thiscall, Dockers_Add, 0x0046E980, void* dockers, int a, int b, void* script);
GAME_FN(void*, __thiscall, Dockers_Get, 0x0046E9E0, void* dockers, void* userdata);
GAME_FN(void, __thiscall, Docker_AddLocation, 0x0046EB20, void* docker, int a, int b);
GAME_FN(void, __thiscall, Docker_AddPeriod, 0x0046E280, void* docker, int startMinute, int minutes);

// points of interest
GAME_FN(int, __thiscall, POIManager_GetScriptedPointToActivate, 0x00449140, void* mgr, int type);
GAME_FN(void*, __thiscall, POIManager_Get, 0x00448DC0, void* mgr, int index);   // -2: its own one
GAME_FN(void, __thiscall, POI_SetActivation, 0x004478F0, void* poi, uint8_t active);

// punishment
GAME_FN(void, __thiscall, Punishment_SetAlertLevels, 0x0044E1E0, void* p, int* levels);
GAME_FN(void, __thiscall, Punishment_ResetAlertLevels, 0x0044E210, void* p);
GAME_FN(void, __thiscall, Punishment_SetPedType, 0x0044E200, void* p, int a, int b);
GAME_FN(void, __thiscall, Punishment_ResetPedTypes, 0x0044E230, void* p);

// areas
GAME_FN(void, __cdecl, RemoveExtraScene, 0x00429940);
GAME_FN(void, __cdecl, RegisterAnimProps, 0x00438850);
GAME_FN(void, __thiscall, SpecialEntities_Load, 0x006AE730, void* se, uint32_t hash);
GAME_FN(void, __thiscall, SpecialEntities_Unload, 0x006AE760, void* se, uint32_t hash);
GAME_FN(void, __thiscall, SpecialEntities_EnsureCreated, 0x006AEA10, void* se);
GAME_FN(void, __thiscall, SpecialEntities_SetOverride, 0x006AE6F0, void* se, const char* name, int value);
GAME_FN(void, __thiscall, AreaTransition_DoorFix, 0x00415290, void* t);
GAME_FN(void, __thiscall, AreaTransition_CancelStored, 0x00415D80, void* t);
GAME_FN(uint8_t, __thiscall, AreaTransition_IsLoading, 0x00415380, void* t);
GAME_FN(void, __cdecl, SetPathableInRadius, 0x005EAB30, ScriptVector* centre, float a, float b, uint8_t pathable);
GAME_FN(uint8_t, __cdecl, SavePoint_737DD0, 0x00737DD0, void* savePoint);
GAME_FN(void, __cdecl, SavePoint_SetLocation, 0x00737F30, void* savePoint, ScriptVector* position, float heading, int area, ScriptVector* v);
GAME_FN(void, __cdecl, ClearAllExplosions, 0x004D2680);
GAME_FN(void, __cdecl, ClearProjectilesOfType, 0x004D49F0, int type);
GAME_FN(void, __cdecl, ClearAllProjectiles, 0x004D4A90);
GAME_FN(void, __cdecl, LoadCollision_570080, 0x00570080, float* xy);
GAME_FN(void, __cdecl, LoadScene_439770, 0x00439770, float* xyz, int flag);
GAME_FN(void, __cdecl, AddExtraScene, 0x004298E0, float x, float y, float z, uint8_t a, uint8_t b, uint8_t c, int d, float e);
GAME_FN(void, __cdecl, LoadScene_439CF0_x64, 0x00439CF0, re::Float3, int flag);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void LoadScene_439CF0(float a0, float a1, float a2, int a3) {
    return LoadScene_439CF0_x64(re::Float3{a0, a1, a2}, a3);
}
GAME_FN(void, __cdecl, LoadCollision_56FE00, 0x0056FE00, float* xy);
GAME_FN(void, __cdecl, ExtraScene_429990, 0x00429990);
GAME_FN(uint8_t, __cdecl, IsCollisionLoaded, 0x005701C0, float* xy);
GAME_FN(void, __cdecl, RegisterAreaScript, 0x00415060, int* area, const char* name);

} // namespace

#define NATIVE_A(name, address)                                                       \
    static decltype(&Native_##name) Original_Native_##name;                             \
    REVERSED("Native_" #name, address, RE_BODY(&Native_##name), nullptr, \
             reinterpret_cast<void**>(&Original_Native_##name), re::Mode::Original)

// ---- areas

int __cdecl Native_AreaRemoveExtraScene(lua_State*) {
    RemoveExtraScene();
    return 0;
}
NATIVE_A(AreaRemoveExtraScene, 0x005B6050);

// (reads its three arguments and does nothing: stubbed out)
int __cdecl Native_AreaLoad(lua_State* L) {
    Script::GetInt(L, 0);
    Script::GetInt(L, 1);
    Script::GetInt(L, 2);
    return 0;
}
NATIVE_A(AreaLoad, 0x005B6060);

int __cdecl Native_AreaActivatePopulationTrigger(lua_State* L) {
    Population_ActivateTrigger(g_population, Script::GetInt(L, 0));
    return 0;
}
NATIVE_A(AreaActivatePopulationTrigger, 0x005B6090);

int __cdecl Native_AreaDeactivatePopulationTrigger(lua_State* L) {
    Population_DeactivateTrigger(g_population, Script::GetInt(L, 0));
    return 0;
}
NATIVE_A(AreaDeactivatePopulationTrigger, 0x005B60B0);

int __cdecl Native_AreaDeteriorateAttitudeForPopTriggerPedType(lua_State* L) {
    int trigger = Script::GetInt(L, 0);
    int pedType = Script::GetInt(L, 1);
    Population_DeteriorateAttitudeForTriggerPedType(g_population, trigger, pedType);
    return 0;
}
NATIVE_A(AreaDeteriorateAttitudeForPopTriggerPedType, 0x005B60D0);

// (total, then up to 12 counts, 0 when missing)
int __cdecl Native_AreaOverridePopulation(lua_State* L) {
    int counts[13];
    counts[0] = Script::GetInt(L, 0);
    for (int i = 1; i < 13; i++) {
        counts[i] = Script::GetIntOpt(L, i, 0);
    }
    Population_Override(g_population, counts);
    return 0;
}
NATIVE_A(AreaOverridePopulation, 0x005B6100);

int __cdecl Native_AreaRevertToDefaultPopulation(lua_State*) {
    Population_RevertToDefault(g_population);
    return 0;
}
NATIVE_A(AreaRevertToDefaultPopulation, 0x005B6150);

// (females, males): +0x7898 bit 2 females, bit 1 males (a model's +0x50 is "female"; the girls'
// dorm's script calls it with (true, false))
int __cdecl Native_AreaSetPopulationSexGeneration(lua_State* L) {
    uint8_t females = Script::GetBool(L, 0);
    uint8_t males = Script::GetBool(L, 1);
    uint8_t& flags = At<uint8_t>(g_population, 0x7898);
    flags = uint8_t((flags & ~4) | ((females << 2) & 4));
    flags = uint8_t((flags & ~2) | ((males << 1) & 2));
    return 0;
}
NATIVE_A(AreaSetPopulationSexGeneration, 0x005B6160);

// ---- patrol paths

int __cdecl Native_AreaDisablePatrolPath(lua_State* L) {
    int slot = Script::GetInt(L, 0);
    void* path = nullptr;
    if (slot != -1) {
        path = PatrolPath_Get(g_patrolPathPoolOwner, slot);
        if (path != nullptr) {
            PatrolPaths_Disable(g_patrolPaths, path);
        }
    }
    Script::PushBool(L, path != nullptr);
    return 1;
}
NATIVE_A(AreaDisablePatrolPath, 0x005B61B0);

int __cdecl Native_AreaEnablePatrolPath(lua_State* L) {
    void* path = PatrolPath_Get(g_patrolPathPoolOwner, Script::GetInt(L, 0));
    if (path != nullptr) {
        PatrolPaths_Enable(g_patrolPaths, path);
    }
    return 0;
}
NATIVE_A(AreaEnablePatrolPath, 0x005B6200);

// (the running script's, or all of them outside one)
int __cdecl Native_AreaClearPatrolPaths(lua_State*) {
    void* script = LurStore_GetCurrentScript(TheLurStore);
    if (script != nullptr) {
        PatrolPaths_ClearForScript(g_patrolPaths, script);
        return 0;
    }
    PatrolPaths_ClearAll(g_patrolPaths);
    return 0;
}
NATIVE_A(AreaClearPatrolPaths, 0x005B6230);

int __cdecl Native_AreaSetActivePatrolPathHours(lua_State* L) {
    void* path = PatrolPath_Get(g_patrolPathPoolOwner, Script::GetInt(L, 0));
    if (path != nullptr) {
        void* paths = g_patrolPaths;
        int to = Script::GetInt(L, 2);
        int from = Script::GetInt(L, 1);
        PatrolPaths_SetActiveHours(paths, path, from, to);
    }
    return 0;
}
NATIVE_A(AreaSetActivePatrolPathHours, 0x005B6260);

int __cdecl Native_AreaInitPatrolPath(lua_State* L) {
    void* path = PatrolPath_Get(g_patrolPathPoolOwner, Script::GetInt(L, 0));
    if (path != nullptr) {
        int c = Script::GetInt(L, 3);
        int b = Script::GetInt(L, 2);
        int a = Script::GetInt(L, 1);
        void* paths = g_patrolPaths;
        uint8_t d = Script::GetBoolOpt(L, 4, 1);
        PatrolPaths_Init(paths, path, a, b, c, d);
    }
    return 0;
}
NATIVE_A(AreaInitPatrolPath, 0x005B62B0);

int __cdecl Native_AreaDisableAllPatrolPaths(lua_State*) {
    PatrolPaths_DisableAll(g_patrolPaths);
    return 0;
}
NATIVE_A(AreaDisableAllPatrolPaths, 0x005B6330);

int __cdecl Native_AreaEnableAllPatrolPaths(lua_State*) {
    PatrolPaths_EnableAll(g_patrolPaths);
    return 0;
}
NATIVE_A(AreaEnableAllPatrolPaths, 0x005B6340);

int __cdecl Native_AreaPatrolPathShowVisionCones(lua_State* L) {
    void* paths = g_patrolPaths;
    PatrolPaths_ShowVisionCones(paths, Script::GetBool(L, 0));
    return 0;
}
NATIVE_A(AreaPatrolPathShowVisionCones, 0x005B6350);

// ---- spawners

int __cdecl Native_AreaClearSpawners(lua_State*) {
    void* script = LurStore_GetCurrentScript(TheLurStore);
    if (script != nullptr) {
        Spawners_ClearForScript(g_spawners, script);
        return 0;
    }
    Spawners_ClearAll(g_spawners);
    return 0;
}
NATIVE_A(AreaClearSpawners, 0x005B6380);

// pushes the new spawner (a light userdata)
int __cdecl Native_AreaAddAmbientSpawner(lua_State* L) {
    int a = Script::GetIntOpt(L, 0, 1);
    int b = Script::GetIntOpt(L, 1, 1);
    int c = Script::GetIntOpt(L, 2, 0);
    int d = Script::GetIntOpt(L, 3, 0);
    void* script = LurStore_GetCurrentScript(TheLurStore);
    void* memory = AmbientSpawner_New(0x114);
    void* spawner = nullptr;
    if (memory != nullptr) {
        // (the script's id; reads through a null script outside one)
        spawner = AmbientSpawner_Construct(memory, a, b, c, d, At<int>(script, 0x44));
    }
    Spawners_Add(g_spawners, spawner);
    Script::PushUserdata(L, spawner);
    return 1;
}
NATIVE_A(AreaAddAmbientSpawner, 0x005B63B0);

// (spawner, hour, minute, minutes (default 1440: all day))
int __cdecl Native_AreaAddAmbientSpawnPeriod(lua_State* L) {
    void* spawner = Spawners_GetAmbient(g_spawners, Script::GetUserdata(L, 0));
    int hour = Script::GetIntOpt(L, 1, 0);
    int minute = Script::GetIntOpt(L, 2, 0);
    int minutes = Script::GetIntOpt(L, 3, 1440);
    AmbientSpawner_AddPeriod(spawner, minute + hour * 60, minutes);
    return 0;
}
NATIVE_A(AreaAddAmbientSpawnPeriod, 0x005B6450);

int __cdecl Native_AreaSetAmbientSpawnerExclusive(lua_State* L) {
    void* spawner = Spawners_GetAmbient(g_spawners, Script::GetUserdata(L, 0));
    uint8_t exclusive = Script::GetBool(L, 1);
    uint8_t& flags = At<uint8_t>(spawner, 0xBC);
    flags = uint8_t((flags & ~1) | (exclusive & 1));
    return 0;
}
NATIVE_A(AreaSetAmbientSpawnerExclusive, 0x005B64C0);

// (spawner, a, b): the spawner's virtual +8 makes the location; pushes it
int __cdecl Native_AreaAddSpawnLocation(lua_State* L) {
    void* spawner = Spawners_GetSpawner(g_spawners, Script::GetUserdata(L, 0));
    int a = Script::GetInt(L, 1);
    int b = Script::GetInt(L, 2);
    using AddLocationFn = void*(__thiscall*)(void*, int, int);
    void* location = re::GameVirtual<AddLocationFn>(spawner, 2)(spawner, a, b);
    Script::PushUserdata(L, location);
    return 1;
}
NATIVE_A(AreaAddSpawnLocation, 0x005B6500);

int __cdecl Native_AreaAddPedModelIdToSpawnLocation(lua_State* L) {
    void* spawner = Spawners_GetMission(g_spawners, Script::GetUserdata(L, 0));
    void* location = Script::GetUserdata(L, 1);
    if (spawner != nullptr) {
        SpawnLocation_AddPedModelId(location, Script::GetInt(L, 2));
    }
    return 0;
}
NATIVE_A(AreaAddPedModelIdToSpawnLocation, 0x005B6560);

int __cdecl Native_AreaAddPedTypeToSpawnLocation(lua_State* L) {
    void* spawner = Spawners_GetMission(g_spawners, Script::GetUserdata(L, 0));
    void* location = Script::GetUserdata(L, 1);
    if (spawner != nullptr) {
        SpawnLocation_AddPedType(location, Script::GetInt(L, 2));
    }
    return 0;
}
NATIVE_A(AreaAddPedTypeToSpawnLocation, 0x005B65B0);

int __cdecl Native_AreaRemoveSpawner(lua_State* L) {
    Spawners_Remove(g_spawners, Script::GetUserdata(L, 0));
    return 0;
}
NATIVE_A(AreaRemoveSpawner, 0x005B6600);

// (true for no such spawner)
int __cdecl Native_AreaMissionSpawnerExhausted(lua_State* L) {
    void* spawner = Spawners_GetMission(g_spawners, Script::GetUserdata(L, 0));
    if (spawner != nullptr && !MissionSpawner_IsExhausted(spawner)) {
        Script::PushBool(L, 0);
        return 1;
    }
    Script::PushBool(L, 1);
    return 1;
}
NATIVE_A(AreaMissionSpawnerExhausted, 0x005B6620);

int __cdecl Native_AreaMissionSpawnerSetExhausted(lua_State* L) {
    void* spawner = Spawners_GetMission(g_spawners, Script::GetUserdata(L, 0));
    if (spawner != nullptr) {
        At<uint8_t>(spawner, 0xBC) = MissionSpawner_4A21B0(spawner);
    }
    return 0;
}
NATIVE_A(AreaMissionSpawnerSetExhausted, 0x005B6670);

int __cdecl Native_AreaMissionSpawnerSetAttackTarget(lua_State* L) {
    void* spawner = Spawners_GetMission(g_spawners, Script::GetUserdata(L, 0));
    void* ped = GetPedFromHandle(Script::GetInt(L, 1), 2);
    if (ped != nullptr) {
        MissionSpawner_SetAttackTarget(spawner, ped, Script::GetBoolOpt(L, 2, 0));
    }
    return 0;
}
NATIVE_A(AreaMissionSpawnerSetAttackTarget, 0x005B66B0);

int __cdecl Native_AreaMissionSpawnerSetCallback(lua_State* L) {
    void* spawner = Spawners_GetMission(g_spawners, Script::GetUserdata(L, 0));
    Script_StoreFunctionRef(L, 1, &At<int>(spawner, 0xCC));
    return 0;
}
NATIVE_A(AreaMissionSpawnerSetCallback, 0x005B6720);

int __cdecl Native_AreaMissionSpawnerSetActivated(lua_State* L) {
    void* spawner = Spawners_GetMission(g_spawners, Script::GetUserdata(L, 0));
    uint8_t activated = Script::GetBool(L, 1);
    uint8_t& flags = At<uint8_t>(spawner, 0xC8);
    flags = uint8_t((flags & ~1) | (activated & 1));
    return 0;
}
NATIVE_A(AreaMissionSpawnerSetActivated, 0x005B6760);

// (spawner, females, males): +0x14 bit 0 females, bit 1 males (SpawnLocation_PickModel hands
// them to PedModelsInfo_Pick as (females, males))
int __cdecl Native_AreaSpawnerSetSexGeneration(lua_State* L) {
    void* spawner = Spawners_GetSpawner(g_spawners, Script::GetUserdata(L, 0));
    uint8_t females = Script::GetBool(L, 1);
    uint8_t males = Script::GetBool(L, 2);
    uint8_t& flags = At<uint8_t>(spawner, 0x14);
    flags = uint8_t(((males & 1) << 1) | (flags & 0xFC) | (females & 1));
    return 0;
}
NATIVE_A(AreaSpawnerSetSexGeneration, 0x005B67A0);

// ---- dockers

int __cdecl Native_AreaClearDockers(lua_State*) {
    void* script = LurStore_GetCurrentScript(TheLurStore);
    if (script != nullptr) {
        Dockers_ClearForScript(g_dockers, script);
        return 0;
    }
    Dockers_ClearAll(g_dockers);
    return 0;
}
NATIVE_A(AreaClearDockers, 0x005B67F0);

int __cdecl Native_AreaAddDocker(lua_State* L) {
    int a = Script::GetIntOpt(L, 0, 1);
    int b = Script::GetIntOpt(L, 1, 1);
    void* script = LurStore_GetCurrentScript(TheLurStore);
    Script::PushUserdata(L, Dockers_Add(g_dockers, a, b, script));
    return 1;
}
NATIVE_A(AreaAddDocker, 0x005B6820);

int __cdecl Native_AreaAddDockLocation(lua_State* L) {
    void* docker = Dockers_Get(g_dockers, Script::GetUserdata(L, 0));
    int a = Script::GetInt(L, 1);
    int b = Script::GetInt(L, 2);
    Docker_AddLocation(docker, a, b);
    return 0;
}
NATIVE_A(AreaAddDockLocation, 0x005B6870);

int __cdecl Native_AreaAddDockPeriod(lua_State* L) {
    void* docker = Dockers_Get(g_dockers, Script::GetUserdata(L, 0));
    int hour = Script::GetIntOpt(L, 1, 0);
    int minute = Script::GetIntOpt(L, 2, 0);
    int minutes = Script::GetIntOpt(L, 3, 1440);
    Docker_AddPeriod(docker, minute + hour * 60, minutes);
    return 0;
}
NATIVE_A(AreaAddDockPeriod, 0x005B68C0);

// (docker, females, males): +0xFC bit 2 females, bit 1 males (PedSchedules tests a model with
// +0x50 "female" against 4)
int __cdecl Native_AreaSetDockerSexReception(lua_State* L) {
    void* docker = Dockers_Get(g_dockers, Script::GetUserdata(L, 0));
    uint8_t& flags = At<uint8_t>(docker, 0xFC);
    uint8_t females = Script::GetBool(L, 1);
    flags = uint8_t((flags & ~4) | ((females << 2) & 4));
    uint8_t males = Script::GetBool(L, 2);
    flags = uint8_t((flags & ~2) | ((males << 1) & 2));
    return 0;
}
NATIVE_A(AreaSetDockerSexReception, 0x005B6930);

int __cdecl Native_AreaSetDockerPatrolPedReception(lua_State* L) {
    void* docker = Dockers_Get(g_dockers, Script::GetUserdata(L, 0));
    uint8_t on = Script::GetBool(L, 1);
    uint8_t& flags = At<uint8_t>(docker, 0xFC);
    flags = uint8_t((flags & ~8) | ((on << 3) & 8));
    return 0;
}
NATIVE_A(AreaSetDockerPatrolPedReception, 0x005B6990);

int __cdecl Native_AreaSetDockerKidReception(lua_State* L) {
    void* docker = Dockers_Get(g_dockers, Script::GetUserdata(L, 0));
    uint8_t on = Script::GetBool(L, 1);
    uint8_t& flags = At<uint8_t>(docker, 0xFC);
    flags = uint8_t((flags & ~0x10) | ((on << 4) & 0x10));
    return 0;
}
NATIVE_A(AreaSetDockerKidReception, 0x005B69E0);

int __cdecl Native_AreaSetDockerRunPercentage(lua_State* L) {
    void* docker = Dockers_Get(g_dockers, Script::GetUserdata(L, 0));
    At<uint8_t>(docker, 0xFF) = uint8_t(Script::GetInt(L, 1));
    return 0;
}
NATIVE_A(AreaSetDockerRunPercentage, 0x005B6A20);

int __cdecl Native_AreaSetDockerMinsToRunBeforeEnd(lua_State* L) {
    void* docker = Dockers_Get(g_dockers, Script::GetUserdata(L, 0));
    At<uint8_t>(docker, 0xFE) = uint8_t(Script::GetInt(L, 1));
    return 0;
}
NATIVE_A(AreaSetDockerMinsToRunBeforeEnd, 0x005B6A60);

int __cdecl Native_AreaSetDockerChanceToDock(lua_State* L) {
    void* docker = Dockers_Get(g_dockers, Script::GetUserdata(L, 0));
    At<uint8_t>(docker, 0x100) = uint8_t(Script::GetInt(L, 1));
    return 0;
}
NATIVE_A(AreaSetDockerChanceToDock, 0x005B6AA0);

// ---- points of interest

// (type: the one number argument, else 16)
int __cdecl Native_AreaGetScriptedPOIPointToActivate(lua_State* L) {
    int type = 16;
    if (lua::gettop(L) == 1 && !Script::IsNil(L, 0) && Script::IsNumber(L, 0)) {
        type = Script::GetInt(L, 0);
    }
    Script::PushInt(L, POIManager_GetScriptedPointToActivate(g_poiManager, type));
    return 1;
}
NATIVE_A(AreaGetScriptedPOIPointToActivate, 0x005B6AE0);

int __cdecl Native_AreaPOISetActivation(lua_State* L) {
    int index = Script::GetInt(L, 0);
    uint8_t active = Script::GetBool(L, 1);
    void* poi = POIManager_Get(g_poiManager, index);
    if (poi != nullptr) {
        POI_SetActivation(poi, active);
    }
    return 0;
}
NATIVE_A(AreaPOISetActivation, 0x005B6B50);

int __cdecl Native_AreaPOIGetActivation(lua_State* L) {
    void* poi = POIManager_Get(g_poiManager, Script::GetInt(L, 0));
    uint8_t active = 0;
    if (poi != nullptr) {
        active = At<uint8_t>(poi, 0x82);
    }
    Script::PushBool(L, active);
    return 1;
}
NATIVE_A(AreaPOIGetActivation, 0x005B6BA0);

int __cdecl Native_AreaPOICompareName(lua_State* L) {
    int index = Script::GetInt(L, 0);
    const char* name = Script::GetString(L, 1);
    void* poi = POIManager_Get(g_poiManager, index);
    uint8_t same = 0;
    if (poi != nullptr) {
        if (name == nullptr) {
            same = 0;
        } else {
            same = At<uint32_t>(poi, 0x84) == HashName(name);
        }
    }
    Script::PushBool(L, same);
    return 1;
}
NATIVE_A(AreaPOICompareName, 0x005B6BF0);

int __cdecl Native_AreaPOIGetInterestType(lua_State* L) {
    void* poi = POIManager_Get(g_poiManager, Script::GetInt(L, 0));
    int type = 15;
    if (poi != nullptr) {
        type = At<int>(poi, 0x70);
    }
    Script::PushInt(L, type);
    return 1;
}
NATIVE_A(AreaPOIGetInterestType, 0x005B6C60);

int __cdecl Native_AreaPOIGetFaction(lua_State* L) {
    void* poi = POIManager_Get(g_poiManager, Script::GetInt(L, 0));
    int faction = 12;
    if (poi != nullptr) {
        faction = At<int>(poi, 0x78);
    }
    Script::PushInt(L, faction);
    return 1;
}
NATIVE_A(AreaPOIGetFaction, 0x005B6CA0);

int __cdecl Native_AreaPOIGetMaxNumber(lua_State* L) {
    void* poi = POIManager_Get(g_poiManager, Script::GetInt(L, 0));
    int max = 0;
    if (poi != nullptr) {
        max = At<int8_t>(poi, 0x81);
    }
    Script::PushInt(L, max);
    return 1;
}
NATIVE_A(AreaPOIGetMaxNumber, 0x005B6CE0);

int __cdecl Native_AreaPOIDeactivated(lua_State* L) {
    void* poi = POIManager_Get(g_poiManager, Script::GetInt(L, 0));
    uint8_t active = 0;
    if (poi != nullptr) {
        active = At<uint8_t>(poi, 0x82);
    }
    Script::PushBool(L, active == 0);
    return 1;
}
NATIVE_A(AreaPOIDeactivated, 0x005B6D20);

int __cdecl Native_AreaPOIGetGender(lua_State* L) {
    void* poi = POIManager_Get(g_poiManager, Script::GetInt(L, 0));
    int gender = 0;
    if (poi != nullptr) {
        gender = At<int>(poi, 0x74);
    }
    Script::PushInt(L, gender);
    return 1;
}
NATIVE_A(AreaPOIGetGender, 0x005B6D60);

int __cdecl Native_IsStreamingBusy(lua_State* L) {
    uint8_t busy;
    if (g_streamingState > -200 && (g_streamingBusy != 0 || ms_numModelsRequested > 0)) {
        busy = 1;
    } else {
        busy = 0;
    }
    Script::PushBool(L, busy);
    return 1;
}
NATIVE_A(IsStreamingBusy, 0x005B6DA0);

// (argument 0 is read twice: once for nothing)
int __cdecl Native_AreaSetPunishmentAlertLevels(lua_State* L) {
    int levels[3];
    levels[0] = Script::GetInt(L, 0);
    for (int i = 0; i < 3; i++) {
        levels[i] = Script::GetInt(L, i);
    }
    Punishment_SetAlertLevels(g_punishment, levels);
    return 0;
}
NATIVE_A(AreaSetPunishmentAlertLevels, 0x005B6DE0);

int __cdecl Native_AreaResetPunishmentAlertLevels(lua_State*) {
    Punishment_ResetAlertLevels(g_punishment);
    return 0;
}
NATIVE_A(AreaResetPunishmentAlertLevels, 0x005B6E30);

int __cdecl Native_AreaSetPunishmentPedType(lua_State* L) {
    uint32_t a = uint32_t(Script::GetInt(L, 0));
    uint32_t b = uint32_t(Script::GetInt(L, 1));
    if (a < 64 && b < 15) {
        Punishment_SetPedType(g_punishment, int(a), int(b));
    }
    return 0;
}
NATIVE_A(AreaSetPunishmentPedType, 0x005B6E40);

int __cdecl Native_AreaResetPunishmentPedTypes(lua_State*) {
    Punishment_ResetPedTypes(g_punishment);
    return 0;
}
NATIVE_A(AreaResetPunishmentPedTypes, 0x005B6E80);

int __cdecl Native_AreaGetPopulationCullDistance(lua_State* L) {
    Script::PushFloat(L, At<float>(g_population, 0x7828));
    return 1;
}
NATIVE_A(AreaGetPopulationCullDistance, 0x005B6E90);

int __cdecl Native_AreaRegisterAnimProps(lua_State*) {
    RegisterAnimProps();
    return 0;
}
NATIVE_A(AreaRegisterAnimProps, 0x005B6EC0);

int __cdecl Native_AreaLoadSpecialEntities(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    uint8_t load = Script::GetBool(L, 1);
    uint32_t hash = HashName(name);
    if (load) {
        SpecialEntities_Load(g_specialEntities, hash);
        return 0;
    }
    SpecialEntities_Unload(g_specialEntities, hash);
    return 0;
}
NATIVE_A(AreaLoadSpecialEntities, 0x005B6ED0);

int __cdecl Native_AreaEnsureSpecialEntitiesAreCreated(lua_State*) {
    SpecialEntities_EnsureCreated(g_specialEntities);
    return 0;
}
NATIVE_A(AreaEnsureSpecialEntitiesAreCreated, 0x005B6F20);

int __cdecl Native_AreaEnsureSpecialEntitiesAreCreatedWithOverride(lua_State* L) {
    int value = Script::GetInt(L, 1);
    const char* name = Script::GetString(L, 0);
    SpecialEntities_SetOverride(g_specialEntities, name, value);
    SpecialEntities_EnsureCreated(g_specialEntities);
    SpecialEntities_SetOverride(g_specialEntities, nullptr, 0);
    return 0;
}
NATIVE_A(AreaEnsureSpecialEntitiesAreCreatedWithOverride, 0x005B6F30);

int __cdecl Native_AreaDoorTransitionFix(lua_State*) {
    AreaTransition_DoorFix(g_areaTransition);
    return 0;
}
NATIVE_A(AreaDoorTransitionFix, 0x005B6F80);

int __cdecl Native_AreaGetPopulationEvaluatesTriggersIn3D(lua_State* L) {
    Script::PushBool(L, (At<uint8_t>(g_population, 0x7898) >> 5) & 1);
    return 1;
}
NATIVE_A(AreaGetPopulationEvaluatesTriggersIn3D, 0x005B6F90);

int __cdecl Native_AreaSetPopulationEvaluatesTriggersIn3D(lua_State* L) {
    void* pop = g_population;
    uint8_t on = Script::GetBool(L, 0);
    uint8_t& flags = At<uint8_t>(pop, 0x7898);
    flags = uint8_t((flags & ~0x20) | ((on << 5) & 0x20));
    return 0;
}
NATIVE_A(AreaSetPopulationEvaluatesTriggersIn3D, 0x005B6FC0);

int __cdecl Native_AreaCancelStoredTransition(lua_State*) {
    AreaTransition_CancelStored(g_areaTransition);
    return 0;
}
NATIVE_A(AreaCancelStoredTransition, 0x005B6FF0);

// (x, y, z, a, b, pathable)
int __cdecl Native_AreaSetPathableInRadius(lua_State* L) {
    ScriptVector centre;
    centre.x = Script::GetFloat(L, 0);
    centre.y = Script::GetFloat(L, 1);
    centre.z = Script::GetFloat(L, 2);
    float a = Script::GetFloat(L, 3);
    float b = Script::GetFloat(L, 4);
    uint8_t pathable = Script::GetBool(L, 5);
    SetPathableInRadius(&centre, a, b, pathable);
    return 0;
}
NATIVE_A(AreaSetPathableInRadius, 0x005B7000);

int __cdecl Native_AreaSignalAreaTransitionReadyToLoad(lua_State*) {
    g_areaTransitionReadyToLoad = 1;
    return 0;
}
NATIVE_A(AreaSignalAreaTransitionReadyToLoad, 0x005B70A0);

int __cdecl Native_AreaIsLoading(lua_State* L) {
    Script::PushBool(L, AreaTransition_IsLoading(g_areaTransition));
    return 1;
}
NATIVE_A(AreaIsLoading, 0x005B70B0);

int __cdecl Native_AreaDisableCameraControlForTransition(lua_State* L) {
    g_disableCameraControlForTransition = Script::GetBool(L, 0);
    return 0;
}
NATIVE_A(AreaDisableCameraControlForTransition, 0x005B70D0);

int __cdecl Native_AreaForceLoadAreaByAreaTransition(lua_State* L) {
    g_forceLoadAreaByAreaTransition = Script::GetBool(L, 0);
    return 0;
}
NATIVE_A(AreaForceLoadAreaByAreaTransition, 0x005B70F0);

// (save point, x, y, z, heading in degrees, area, x, y, z); nothing when 0x737DD0 says so
int __cdecl Native_AreaSetSavePointLocation(lua_State* L) {
    void* savePoint = Script::GetUserdata(L, 0);
    if (!SavePoint_737DD0(savePoint)) {
        ScriptVector position;
        position.x = Script::GetFloat(L, 1);
        position.y = Script::GetFloat(L, 2);
        position.z = Script::GetFloat(L, 3);
        float heading = float(double(Script::GetFloat(L, 4)) * 3.1415927410125732 / 180.0);
        int area = Script::GetInt(L, 5);
        ScriptVector v;
        v.x = Script::GetFloat(L, 6);
        v.y = Script::GetFloat(L, 7);
        v.z = Script::GetFloat(L, 8);
        SavePoint_SetLocation(savePoint, &position, heading, area, &v);
    }
    return 0;
}
NATIVE_A(AreaSetSavePointLocation, 0x005B7110);

// (0: fade out, 1: fade in, in ms; nothing pushed for others, but still returns 1)
int __cdecl Native_AreaGetTransitionTime(lua_State* L) {
    int16_t which = int16_t(Script::GetInt(L, 0));
    if (which == 0) {
        Script::PushInt(L, int(double(At<float>(g_transitionTimes, 0x18)) * 1000.0));
    } else if (which == 1) {
        Script::PushInt(L, int(double(At<float>(g_transitionTimes, 0x1C)) * 1000.0));
    }
    return 1;
}
NATIVE_A(AreaGetTransitionTime, 0x005B7200);

int __cdecl Native_AreaClearAllExplosions(lua_State*) {
    ClearAllExplosions();
    return 0;
}
NATIVE_A(AreaClearAllExplosions, 0x005B7250);

int __cdecl Native_AreaClearAllProjectiles(lua_State* L) {
    if (lua::gettop(L) > 0) {
        ClearProjectilesOfType(Script::GetInt(L, 0));
        return 0;
    }
    ClearAllProjectiles();
    return 0;
}
NATIVE_A(AreaClearAllProjectiles, 0x005B7260);

// (the first call gets x, y and an unset z: uninitialised stack in the game)
int __cdecl Native_AreaLoadCollision(lua_State* L) {
    int x = Script::GetInt(L, 0);
    int y = Script::GetInt(L, 1);
    float xy[3];
    xy[0] = float(x);
    xy[1] = float(y);
    LoadCollision_570080(xy);
    float xyz[3] = {float(x), float(y), 0.0f};
    LoadScene_439770(xyz, 1);
    return 0;
}
NATIVE_A(AreaLoadCollision, 0x005B7290);

// (x, y, a, b, c, d, e): a is forced on with b or c
int __cdecl Native_AreaAddExtraScene(lua_State* L) {
    int x = Script::GetInt(L, 0);
    int y = Script::GetInt(L, 1);
    uint8_t a = Script::GetBool(L, 2);
    uint8_t b = Script::GetBool(L, 3);
    uint8_t c = Script::GetBool(L, 4);
    int d = Script::GetInt(L, 5);
    float e = Script::GetFloat(L, 6);
    if (b || c) {
        a = 1;
    }
    ScriptVector position = {float(x), float(y), 0.0f};
    AddExtraScene(position.x, position.y, position.z, a, b, c, d, e);
    if (a) {
        ScriptVector* p = FindPlayerCoors(&position);
        LoadScene_439CF0(p->x, p->y, p->z, 0);
    }
    if (b) {
        ScriptVector* p = FindPlayerCoors(&position);
        float xy[2] = {p->x, p->y};
        LoadCollision_56FE00(xy);
    }
    if (c) {
        ExtraScene_429990();
    }
    return 0;
}
NATIVE_A(AreaAddExtraScene, 0x005B7310);

int __cdecl Native_AreaGetVisible(lua_State* L) {
    Script::PushInt(L, currArea);
    return 1;
}
NATIVE_A(AreaGetVisible, 0x005B7460);

// (pedType, count): the current population with one ped type's count replaced
int __cdecl Native_AreaOverridePopulationPedType(lua_State* L) {
    ScriptVector position;
    Population_49EF00(g_population, FindPlayerCoors(&position));
    uint8_t* current = At<uint8_t*>(g_population, 0x76F0);
    uint32_t pedType = uint32_t(Script::GetInt(L, 0));
    int count = Script::GetInt(L, 1);
    if (current != nullptr) {
        int counts[13];
        int total = 0;
        for (uint32_t i = 0; i < 12; i++) {
            int n;
            if (i == pedType) {
                n = count;
            } else if (i < At<uint32_t>(current, 0x14)) {
                n = current[8 + i];
            } else {
                n = 0;
            }
            counts[1 + i] = n;
            total += n;
        }
        counts[0] = total;
        Population_Override(g_population, counts);
    }
    return 0;
}
NATIVE_A(AreaOverridePopulationPedType, 0x005B7480);

// (its entity's position (+0x30 of +0x14), else its own (+4); a default for none)
int __cdecl Native_AreaPOIGetPosition(lua_State* L) {
    void* poi = POIManager_Get(g_poiManager, Script::GetInt(L, 0));
    ScriptVector position = {g_poiDefaultPosition[0], g_poiDefaultPosition[1], g_poiDefaultPosition[2]};
    if (poi != nullptr) {
        void* entity = At<void*>(poi, 0x14);
        const float* p = entity != nullptr ? &At<float>(entity, 0x30) : &At<float>(poi, 4);
        position.x = p[0];
        position.y = p[1];
        position.z = p[2];
    }
    Script::PushVector(L, &position);
    return 3;
}
NATIVE_A(AreaPOIGetPosition, 0x005B7530);

int __cdecl Native_AreaIsCollisionLoaded(lua_State* L) {
    int x = Script::GetInt(L, 0);
    int y = Script::GetInt(L, 1);
    float xy[2] = {float(x), float(y)};
    Script::PushBool(L, IsCollisionLoaded(xy));
    return 1;
}
NATIVE_A(AreaIsCollisionLoaded, 0x005B75B0);

// (x, y, z, area, heading)
int __cdecl Native_AreaGetPlayerPositionBeforeStore(lua_State* L) {
    Script::PushFloat(L, g_playerPositionBeforeStore[0]);
    Script::PushFloat(L, g_playerPositionBeforeStore[1]);
    Script::PushFloat(L, g_playerPositionBeforeStore[2]);
    Script::PushInt(L, g_playerAreaBeforeStore);
    Script::PushFloat(L, g_playerPositionBeforeStore[3]);
    return 5;
}
NATIVE_A(AreaGetPlayerPositionBeforeStore, 0x005B7620);

int __cdecl Native_AreaRegisterAreaScript(lua_State* L) {
    int area = Script::GetInt(L, 0);
    const char* name = Script::GetString(L, 1);
    RegisterAreaScript(&area, name);
    return 0;
}
NATIVE_A(AreaRegisterAreaScript, 0x005B7690);

// ---- triggers (the area triggers pool; doors are triggers too)

namespace {
GAME_VAR(int32_t, g_transitionDestination, 0x00BCCE08);
GAME_VAR(CPool<uint8_t>*, g_triggerPool, 0x020C7C48);
inline void* const g_triggerNames = reinterpret_cast<void*>(0x020C7E98);
inline void* const g_doors = reinterpret_cast<void*>(0x00C3B808);   // the doors by name hash
inline void* const g_pathNodes = reinterpret_cast<void*>(0x00BF6228);
GAME_FN(int, __thiscall, TriggerNames_FindByName, 0x006D7A10, void* names, const char* name);   // -1: none
GAME_FN(int, __thiscall, TriggerNames_FindByUserdata, 0x006D79C0, void* names, void* userdata);
GAME_FN(uint8_t, __thiscall, Trigger_IsDoorLocked, 0x006D6550, void* trigger);
GAME_FN(void, __thiscall, Trigger_SetDoorLocked, 0x006D6570, void* trigger, uint8_t locked);
GAME_FN(uint8_t, __thiscall, Trigger_IsDoorLockedToPeds, 0x006D65A0, void* trigger);
GAME_FN(void, __thiscall, Trigger_SetDoorLockedToPeds, 0x006D65D0, void* trigger, uint8_t locked);
GAME_FN(uint8_t, __thiscall, Trigger_IsDoorOpen, 0x006D6620, void* trigger);
GAME_FN(void, __thiscall, Trigger_SetDoorOpen, 0x006D6650, void* trigger, uint8_t open);
// the doors by key: a name hash (or, the game's slip, a userdata pointer)
GAME_FN(uint8_t, __thiscall, Doors_IsLocked, 0x004E99A0, void* doors, uint32_t key);
GAME_FN(void, __thiscall, Doors_SetLocked, 0x004E99F0, void* doors, uint32_t key, uint8_t locked);
GAME_FN(uint8_t, __thiscall, Doors_IsLockedToPeds, 0x004E9A70, void* doors, uint32_t key);
GAME_FN(void, __thiscall, Doors_SetLockedToPeds, 0x004E9AC0, void* doors, uint32_t key, uint8_t locked);
GAME_FN(uint8_t, __thiscall, Doors_IsOpen, 0x004E9B40, void* doors, uint32_t key);
GAME_FN(void, __thiscall, Doors_SetOpen, 0x004E9B90, void* doors, uint32_t key, uint8_t open);
GAME_FN(void*, __cdecl, Trigger_GetPropAnim, 0x004EA3C0, void* trigger);
GAME_FN(void, __cdecl, Door_SetPathableToPeds, 0x005EB060, void* door, uint8_t pathable);
GAME_FN(void*, __cdecl, MissionSpawner_New, 0x004A20F0, uint32_t size);   // from its pool
GAME_FN(void*, __thiscall, MissionSpawner_Construct, 0x004A14C0, void* spawner, int a, int b, int scriptId, int triggerField, int c, int d, int e);
GAME_FN(void, __thiscall, Spawners_AddMission, 0x004A1A30, void* spawners, void* spawner);
GAME_VAR(CPool<uint8_t>*, ms_pPedPool, 0x00C0F5F0);
GAME_VAR(CPool<uint8_t>*, ms_pVehiclePool, 0x00C0F5F4);
GAME_VAR(uint8_t*, g_playerPed, 0x00C1AEA8);
inline void* const g_pedsKeptOnClear = reinterpret_cast<void*>(0x00BCD098);
GAME_FN(uint8_t, __thiscall, PedsKept_Contains, 0x004245E0, void* list, void* ped);
GAME_FN(uint8_t, __thiscall, Vehicle_CanBeCleared, 0x004CB7F0, void* vehicle);
GAME_FN(void*, __thiscall, VehicleOccupants_Get, 0x004CE560, void* occupants, int seat);
GAME_FN(void, __thiscall, VehicleOccupants_Remove, 0x004CEA60, void* occupants, void* ped);
GAME_FN(void, __thiscall, PathNodes_SwitchOffInTrigger, 0x00442090, void* nodes, void* trigger, uint8_t off);

// A trigger by slot (nullptr below 0 or for a free slot; no upper bound check)
uint8_t* TriggerAt(int slot) {
    if (slot < 0) {
        return nullptr;
    }
    return g_triggerPool->GetAt(slot);
}

// The door natives: argument 0 names a trigger (by name, userdata or slot); a name or
// userdata that isn't one goes to the doors by key
uint8_t QueryDoor(lua_State* L, uint8_t(__thiscall* ofTrigger)(void*), uint8_t(__thiscall* byKey)(void*, uint32_t)) {
    if (Script::IsString(L, 0)) {
        int slot = TriggerNames_FindByName(g_triggerNames, Script::GetString(L, 0));
        if (slot != -1) {
            uint8_t* trigger = g_triggerPool->GetAt(slot);
            if (trigger != nullptr) {
                return ofTrigger(trigger);
            }
        }
        return byKey(g_doors, HashName(Script::GetString(L, 0)));
    }
    if (Script::IsUserdata(L, 0)) {
        void* userdata = Script::GetUserdata(L, 0);
        int slot = TriggerNames_FindByUserdata(g_triggerNames, userdata);
        if (slot != -1) {
            uint8_t* trigger = g_triggerPool->GetAt(slot);
            if (trigger != nullptr) {
                return ofTrigger(trigger);
            }
        }
        return byKey(g_doors, uint32_t(reinterpret_cast<uintptr_t>(userdata)));
    }
    uint8_t* trigger = TriggerAt(Script::GetInt(L, 0));
    if (trigger != nullptr) {
        return ofTrigger(trigger);
    }
    return 0;
}

void SetDoor(lua_State* L, void(__thiscall* ofTrigger)(void*, uint8_t), void(__thiscall* byKey)(void*, uint32_t, uint8_t)) {
    uint8_t value = Script::GetBool(L, 1);
    if (Script::IsString(L, 0)) {
        int slot = TriggerNames_FindByName(g_triggerNames, Script::GetString(L, 0));
        if (slot != -1) {
            uint8_t* trigger = g_triggerPool->GetAt(slot);
            if (trigger != nullptr) {
                ofTrigger(trigger, value);
                return;
            }
        }
        byKey(g_doors, HashName(Script::GetString(L, 0)), value);
        return;
    }
    if (Script::IsUserdata(L, 0)) {
        void* userdata = Script::GetUserdata(L, 0);
        int slot = TriggerNames_FindByUserdata(g_triggerNames, userdata);
        if (slot != -1) {
            uint8_t* trigger = g_triggerPool->GetAt(slot);
            if (trigger != nullptr) {
                ofTrigger(trigger, value);
                return;
            }
        }
        byKey(g_doors, uint32_t(reinterpret_cast<uintptr_t>(userdata)), value);
        return;
    }
    uint8_t* trigger = TriggerAt(Script::GetInt(L, 0));
    if (trigger != nullptr) {
        ofTrigger(trigger, value);
    }
}
} // namespace

int __cdecl Native_AreaTransitionDestination(lua_State* L) {
    Script::PushInt(L, g_transitionDestination);
    return 1;
}
NATIVE_A(AreaTransitionDestination, 0x005B76C0);

// (+0x3F bit 0; reads through nullptr for no such trigger)
int __cdecl Native_GetIsMissionSpecific(lua_State* L) {
    uint8_t* trigger = TriggerAt(Script::GetInt(L, 0));
    Script::PushBool(L, trigger[0x3F] & 1);
    return 1;
}
NATIVE_A(GetIsMissionSpecific, 0x005B76E0);

// (+4; reads through nullptr for no such trigger)
int __cdecl Native_GetAnchorPosition(lua_State* L) {
    uint8_t* trigger = TriggerAt(Script::GetInt(L, 0));
    Script::PushFloat(L, At<float>(trigger, 4));
    Script::PushFloat(L, At<float>(trigger, 8));
    Script::PushFloat(L, At<float>(trigger, 0xC));
    return 3;
}
NATIVE_A(GetAnchorPosition, 0x005B7730);

int __cdecl Native_AreaIsDoorLocked(lua_State* L) {
    Script::PushBool(L, QueryDoor(L, Trigger_IsDoorLocked, Doors_IsLocked));
    return 1;
}
NATIVE_A(AreaIsDoorLocked, 0x005B77A0);

int __cdecl Native_AreaSetDoorLocked(lua_State* L) {
    SetDoor(L, Trigger_SetDoorLocked, Doors_SetLocked);
    return 0;
}
NATIVE_A(AreaSetDoorLocked, 0x005B78C0);

int __cdecl Native_AreaIsDoorLockedToPeds(lua_State* L) {
    Script::PushBool(L, QueryDoor(L, Trigger_IsDoorLockedToPeds, Doors_IsLockedToPeds));
    return 1;
}
NATIVE_A(AreaIsDoorLockedToPeds, 0x005B79E0);

int __cdecl Native_AreaSetDoorLockedToPeds(lua_State* L) {
    SetDoor(L, Trigger_SetDoorLockedToPeds, Doors_SetLockedToPeds);
    return 0;
}
NATIVE_A(AreaSetDoorLockedToPeds, 0x005B7B00);

int __cdecl Native_AreaIsDoorOpen(lua_State* L) {
    Script::PushBool(L, QueryDoor(L, Trigger_IsDoorOpen, Doors_IsOpen));
    return 1;
}
NATIVE_A(AreaIsDoorOpen, 0x005B7C20);

int __cdecl Native_AreaSetDoorOpen(lua_State* L) {
    SetDoor(L, Trigger_SetDoorOpen, Doors_SetOpen);
    return 0;
}
NATIVE_A(AreaSetDoorOpen, 0x005B7D40);

int __cdecl Native_AreaSetDoorPathableToPeds(lua_State* L) {
    void* door = Trigger_GetPropAnim(TriggerAt(Script::GetInt(L, 0)));
    if (door != nullptr) {
        Door_SetPathableToPeds(door, Script::GetBool(L, 1));
    }
    return 0;
}
NATIVE_A(AreaSetDoorPathableToPeds, 0x005B7E60);

// (a, b, trigger (-1: none), c, d, e): pushes the new spawner
int __cdecl Native_AreaAddMissionSpawner(lua_State* L) {
    int a = Script::GetInt(L, 0);
    int b = Script::GetInt(L, 1);
    int slot = Script::GetIntOpt(L, 2, -1);
    int c = Script::GetIntOpt(L, 3, 1);
    int d = Script::GetIntOpt(L, 4, 0);
    int e = Script::GetIntOpt(L, 5, 0);
    void* script = LurStore_GetCurrentScript(TheLurStore);
    int triggerField = 0;
    if (slot != -1) {
        // (reads through nullptr for no such trigger)
        triggerField = At<int>(TriggerAt(slot), 0x2C);
    }
    void* memory = MissionSpawner_New(0x174);
    void* spawner = nullptr;
    if (memory != nullptr) {
        spawner = MissionSpawner_Construct(memory, a, b, At<int>(script, 0x44), triggerField, c, d, e);
    }
    Spawners_AddMission(g_spawners, spawner);
    Script::PushUserdata(L, spawner);
    return 1;
}
NATIVE_A(AreaAddMissionSpawner, 0x005B7ED0);

// deletes every ped but the player (+0x1570 == 2), type 13 (+0x1310) and the kept ones
int __cdecl Native_AreaClearAllPeds(lua_State*) {
    CPool<uint8_t>* pool = ms_pPedPool;
    for (int slot = pool->size; slot != 0;) {
        slot--;
        uint8_t* ped = pool->GetAt(slot);
        if (ped != nullptr && ped[0x1570] != 2 && At<int>(ped, 0x1310) != 13 && !PedsKept_Contains(g_pedsKeptOnClear, ped)) {
            using DeleteFn = void(__thiscall*)(void*, int);
            re::GameVirtual<DeleteFn>(ped, 0)(ped, 1);
        }
    }
    return 0;
}
NATIVE_A(AreaClearAllPeds, 0x005B7FD0);

// deletes every vehicle that can go but the player's, its occupants taken out first
int __cdecl Native_AreaClearAllVehicles(lua_State*) {
    CPool<uint8_t>* pool = ms_pVehiclePool;
    for (int slot = pool->size; slot != 0;) {
        slot--;
        uint8_t* vehicle = pool->GetAt(slot);
        if (vehicle == nullptr || !Vehicle_CanBeCleared(vehicle) || At<uint8_t*>(g_playerPed, 0x1554) == vehicle) {
            continue;
        }
        if (At<int>(vehicle, 0x28C) > 0) {
            int seat = 0;
            do {
                seat++;
                void* ped = VehicleOccupants_Get(vehicle + 0x1C4, seat);
                if (ped != nullptr) {
                    VehicleOccupants_Remove(vehicle + 0x1C4, ped);
                }
            } while (seat < At<int>(vehicle, 0x28C));
        }
        using DeleteFn = void(__thiscall*)(void*, int);
        re::GameVirtual<DeleteFn>(vehicle, 0)(vehicle, 1);
    }
    return 0;
}
NATIVE_A(AreaClearAllVehicles, 0x005B8030);

// (+0x3F bit 2)
int __cdecl Native_AreaSetTriggerMonitoringRules(lua_State* L) {
    uint8_t* trigger = TriggerAt(Script::GetInt(L, 0));
    if (trigger != nullptr) {
        uint8_t on = Script::GetBool(L, 1);
        trigger[0x3F] = uint8_t((trigger[0x3F] & ~4) | ((on << 2) & 4));
    }
    return 0;
}
NATIVE_A(AreaSetTriggerMonitoringRules, 0x005B80C0);

int __cdecl Native_AreaSetNodesSwitchedOffInTrigger(lua_State* L) {
    uint8_t* trigger = TriggerAt(Script::GetInt(L, 0));
    uint8_t off = Script::GetBool(L, 1);
    PathNodes_SwitchOffInTrigger(g_pathNodes, trigger, off);
    return 0;
}
NATIVE_A(AreaSetNodesSwitchedOffInTrigger, 0x005B8110);

int __cdecl Native_AreaTriggerIsValid(lua_State* L) {
    uint8_t* trigger = nullptr;
    if (Script::IsString(L, 0)) {
        int slot = TriggerNames_FindByName(g_triggerNames, Script::GetString(L, 0));
        if (slot != -1) {
            trigger = g_triggerPool->GetAt(slot);
        }
    } else if (Script::IsUserdata(L, 0)) {
        int slot = TriggerNames_FindByUserdata(g_triggerNames, Script::GetUserdata(L, 0));
        if (slot != -1) {
            trigger = g_triggerPool->GetAt(slot);
        }
    } else {
        trigger = TriggerAt(Script::GetInt(L, 0));
    }
    Script::PushBool(L, trigger != nullptr);
    return 1;
}
NATIVE_A(AreaTriggerIsValid, 0x005B8170);

// ---- blips and the radar

namespace {
GAME_VAR(uint8_t*, g_radar, 0x00C674CC);         // the HUD radar
GAME_VAR(void*, g_hudC674B8, 0x00C674B8);        // (another HUD element shown with it)
GAME_VAR(uint8_t, g_blipAddedByScript, 0x00BF4520);
GAME_ARRAY(float, g_lastBlipPosition, 0x00BF4508);   // x, y, z
inline void* const g_routes = reinterpret_cast<void*>(0x00BC5E7C);
GAME_FN(void, __thiscall, Radar_SetBlipVisionConeAlwaysOn, 0x00548B10, void* radar, int blip, uint8_t on);
GAME_FN(void, __thiscall, Radar_SetBlipVisionConeCanSee, 0x00548B60, void* radar, int blip, uint8_t on);
GAME_FN(void, __thiscall, Radar_SetBlipSize, 0x00548BF0, void* radar, int blip, int size);
GAME_FN(void, __thiscall, Radar_SetBlipMarkerSize, 0x00548BB0, void* radar, int blip, float size);
GAME_FN(void, __thiscall, Radar_RemoveBlip, 0x0054A940, void* radar, int blip);
GAME_FN(void, __thiscall, Radar_SetBlipShortRanged, 0x00548C30, void* radar, int blip, uint8_t on);
GAME_FN(void, __thiscall, Radar_ChangeBlipIcon, 0x00548C80, void* radar, int blip, int icon);
GAME_FN(void, __thiscall, Radar_SetBlipFlashing, 0x0054ABB0, void* radar, int blip);
// a blip at a position: (kind, x, y, z, colour (RGBA bytes), a, icon, c)
GAME_FN(int, __thiscall, Radar_AddBlipAt, 0x0054A410, void* radar, int kind, float x, float y, float z, uint32_t colour, int a, int icon, int c);
// a blip on an entity (by handle): (kind, handle, colour, a, icon, c)
GAME_FN(int, __thiscall, Radar_AddBlipFor, 0x0054A5B0, void* radar, int kind, int handle, uint32_t colour, int a, int icon, int c);
GAME_FN(void, __thiscall, Radar_RemoveBlipsFor, 0x0054A890, void* radar, int kind, int handle);
// 13 args: a 3D marker (type, ..., position, size, direction, ..., colour-ish bytes, ...)
GAME_FN(void, __cdecl, Markers_Place, 0x0052A600, int type, int a, int b, ScriptVector* position, float size, ScriptVector* direction, int c, int d, int e, int f, int g, int h, int i);
// the running script's blips (removed with it)
GAME_FN(void, __thiscall, LuaScript_AddBlip, 0x005DA7D0, void* script, int blip);
GAME_FN(void, __thiscall, LuaScript_AddBlipEx, 0x005D9380, void* script, int blip, int a, float b, float c);
GAME_FN(void*, __thiscall, Routes_Get, 0x006D6450, void* routes, int slot);   // (its own pool, 0x20C7C54)
GAME_FN(float*, __thiscall, Route_First, 0x006D59B0, void* route);
GAME_FN(float*, __thiscall, Route_Next, 0x006D59F0, void* route);
GAME_FN(int, __cdecl, Pickups_Find_4438B0, 0x004438B0, int pickup);   // -1: none
// Script::: argument i as a vehicle
GAME_FN(void*, __cdecl, Script_GetVehicle, 0x005DA110, lua_State* L, int i);
GAME_FN(int, __cdecl, GetVehicleRef, 0x0044CAC0, void* vehicle);
GAME_FN(int, __cdecl, GetPropAnimRef, 0x0044CB20, void* prop);

constexpr uint32_t kBlipGreen = 0xFF007F00;   // (bytes 00 7F 00 FF)

// CPools::GetPedRef, inlined: slot << 8 | the slot's flag byte
int PedRef(uint8_t* ped) {
    CPool<uint8_t>* pool = ms_pPedPool;
    int slot = int(ped - pool->objects) / pool->elemSize;
    return pool->flags[slot] + (slot << 8);
}
} // namespace

int __cdecl Native_BlipSetVisionConeAlwaysOn(lua_State* L) {
    void* radar = g_radar;
    uint8_t on = Script::GetBool(L, 1);
    Radar_SetBlipVisionConeAlwaysOn(radar, Script::GetInt(L, 0), on);
    return 0;
}
NATIVE_A(BlipSetVisionConeAlwaysOn, 0x005B8200);

int __cdecl Native_BlipSetVisionConeCanSee(lua_State* L) {
    void* radar = g_radar;
    uint8_t on = Script::GetBool(L, 1);
    Radar_SetBlipVisionConeCanSee(radar, Script::GetInt(L, 0), on);
    return 0;
}
NATIVE_A(BlipSetVisionConeCanSee, 0x005B8230);

int __cdecl Native_BlipSetSize(lua_State* L) {
    void* radar = g_radar;
    int size = Script::GetInt(L, 1);
    Radar_SetBlipSize(radar, Script::GetInt(L, 0), size);
    return 0;
}
NATIVE_A(BlipSetSize, 0x005B8260);

int __cdecl Native_BlipSetMarkerSize(lua_State* L) {
    void* radar = g_radar;
    float size = Script::GetFloat(L, 1);
    Radar_SetBlipMarkerSize(radar, Script::GetInt(L, 0), size);
    return 0;
}
NATIVE_A(BlipSetMarkerSize, 0x005B8290);

int __cdecl Native_BlipRemove(lua_State* L) {
    if (Script::IsNumber(L, 0)) {
        int blip = Script::GetInt(L, 0);
        g_blipAddedByScript = 0;
        Radar_RemoveBlip(g_radar, blip);
    }
    return 0;
}
NATIVE_A(BlipRemove, 0x005B82D0);

// (x, y, z, size, height): a marker, not a blip
int __cdecl Native_BlipAddArrow(lua_State* L) {
    ScriptVector position;
    position.x = Script::GetFloat(L, 0);
    position.y = Script::GetFloat(L, 1);
    position.z = Script::GetFloat(L, 2);
    float size = Script::GetFloat(L, 3);
    ScriptVector direction = {0.0f, 0.0f, Script::GetFloat(L, 4)};
    Markers_Place(2, -1, 0, &position, size, &direction, -1, 0, 0x72, 0xBC, 0, 1, 5);
    return 0;
}
NATIVE_A(BlipAddArrow, 0x005B8310);

// (the byte goes in whole registers: the upper bytes are stack garbage in the game)
int __cdecl Native_RadarSetVisibility(lua_State* L) {
    uint8_t visible = Script::GetBool(L, 0);
    using SetVisibleFn = void(__thiscall*)(void*, uint8_t);
    void* hud = g_hudC674B8;
    re::GameVirtual<SetVisibleFn>(hud, 5)(hud, visible);
    void* radar = g_radar;
    re::GameVirtual<SetVisibleFn>(radar, 5)(radar, visible);
    return 0;
}
NATIVE_A(RadarSetVisibility, 0x005B83C0);

int __cdecl Native_BlipSetShortRanged(lua_State* L) {
    void* radar = g_radar;
    uint8_t on = Script::GetBool(L, 1);
    Radar_SetBlipShortRanged(radar, Script::GetInt(L, 0), on);
    return 0;
}
NATIVE_A(BlipSetShortRanged, 0x005B8400);

int __cdecl Native_BlipChangeIcon(lua_State* L) {
    void* radar = g_radar;
    int icon = Script::GetInt(L, 1);
    Radar_ChangeBlipIcon(radar, Script::GetInt(L, 0), icon);
    return 0;
}
NATIVE_A(BlipChangeIcon, 0x005B8430);

// (+0xE7D8.. the zoom range: min, mid, max)
int __cdecl Native_RadarSetMinMax(lua_State* L) {
    uint8_t* radar = g_radar;
    int c = Script::GetInt(L, 2);
    int b = Script::GetInt(L, 1);
    int a = Script::GetInt(L, 0);
    At<int>(radar, 0xE7D8) = a;
    At<int>(radar, 0xE7DC) = b;
    At<int>(radar, 0xE7E0) = c;
    return 0;
}
NATIVE_A(RadarSetMinMax, 0x005B8460);

int __cdecl Native_RadarSetIndoorRange(lua_State* L) {
    uint8_t* radar = g_radar;
    At<int>(radar, 0xE7E4) = Script::GetInt(L, 0);
    return 0;
}
NATIVE_A(RadarSetIndoorRange, 0x005B84B0);

// (back to the defaults at +0x1A4..)
int __cdecl Native_RadarRestoreMinMax(lua_State*) {
    uint8_t* radar = g_radar;
    At<int>(radar, 0xE7D8) = At<int>(radar, 0x1A4);
    At<int>(radar, 0xE7DC) = At<int>(radar, 0x1A8);
    At<int>(radar, 0xE7E0) = At<int>(radar, 0x1AC);
    At<int>(radar, 0xE7E4) = At<int>(radar, 0x1B0);
    return 0;
}
NATIVE_A(RadarRestoreMinMax, 0x005B84D0);

int __cdecl Native_BlipSetFlashing(lua_State* L) {
    void* radar = g_radar;
    Radar_SetBlipFlashing(radar, Script::GetInt(L, 0));
    return 0;
}
NATIVE_A(BlipSetFlashing, 0x005B8510);

int __cdecl Native_RadarBusBlipSetVisible(lua_State* L) {
    uint8_t visible = Script::GetBool(L, 0);
    At<uint8_t>(g_radar, 0xE844) = visible;
    return 0;
}
NATIVE_A(RadarBusBlipSetVisible, 0x005B8540);

// (x, y, z, icon, [a = 1], [b = -1], [c = 0], [d = 2]): pushes the blip
int __cdecl Native_BlipAddXYZ(lua_State* L) {
    float x = Script::GetFloat(L, 0);
    float y = Script::GetFloat(L, 1);
    float z = Script::GetFloat(L, 2);
    int icon = Script::GetInt(L, 3);
    float c = 0.0f;
    float d = 2.0f;
    int a = 1;
    int b = -1;
    if (lua::gettop(L) >= 5) {
        a = Script::GetInt(L, 4);
    }
    if (lua::gettop(L) >= 6) {
        b = Script::GetInt(L, 5);
    }
    if (lua::gettop(L) >= 7) {
        c = Script::GetFloat(L, 6);
    }
    if (lua::gettop(L) >= 8) {
        d = Script::GetFloat(L, 7);
    }
    g_blipAddedByScript = 1;
    float lastX = Script::GetFloat(L, 0);
    float lastY = Script::GetFloat(L, 1);
    float lastZ = Script::GetFloat(L, 2);
    void* radar = g_radar;
    g_lastBlipPosition[0] = lastX;
    g_lastBlipPosition[1] = lastY;
    g_lastBlipPosition[2] = lastZ;
    int blip = Radar_AddBlipAt(radar, 4, x, y, z, kBlipGreen, a, icon, 0);
    Radar_SetBlipSize(radar, blip, 5);
    LuaScript_AddBlipEx(LurStore_GetCurrentScript(TheLurStore), blip, b, c, d);
    Script::PushInt(L, blip);
    return 1;
}
NATIVE_A(BlipAddXYZ, 0x005B8560);

int __cdecl Native_BlipAddSave(lua_State* L) {
    float x = Script::GetFloat(L, 0);
    float y = Script::GetFloat(L, 1);
    float z = Script::GetFloat(L, 2);
    void* radar = g_radar;
    int blip = Radar_AddBlipAt(radar, 10, x, y, z, kBlipGreen, 1, 8, 0);
    Radar_SetBlipSize(radar, blip, 5);
    LuaScript_AddBlip(LurStore_GetCurrentScript(TheLurStore), blip);
    Script::PushInt(L, blip);
    return 1;
}
NATIVE_A(BlipAddSave, 0x005B8730);

// (route, icon, [point = the first], [a = 1], [b = -1], [c = 0], [d = 2])
int __cdecl Native_BlipAddPoint(lua_State* L) {
    void* route = Routes_Get(g_routes, Script::GetInt(L, 0));
    int icon = Script::GetInt(L, 1);
    float c = 0.0f;
    int point = -1;
    float d = 2.0f;
    int a = 1;
    int b = -1;
    uint8_t hasPoint = 0;
    if (lua::gettop(L) >= 3) {
        point = Script::GetInt(L, 2);
        hasPoint = 1;
    }
    if (lua::gettop(L) >= 4) {
        a = Script::GetInt(L, 3);
    }
    if (lua::gettop(L) >= 5) {
        b = Script::GetInt(L, 4);
    }
    if (lua::gettop(L) >= 6) {
        c = Script::GetFloat(L, 5);
    }
    if (lua::gettop(L) >= 7) {
        d = Script::GetFloat(L, 6);
    }
    float* p = Route_First(route);
    if (hasPoint && point > 1) {
        for (int i = 1; i < point;) {
            float* next = Route_Next(route);
            if (next == nullptr) {
                break;
            }
            i++;
            p = next;
        }
    }
    float x = p[0];
    float y = p[1];
    float z = p[2];
    void* radar = g_radar;
    g_lastBlipPosition[0] = x;
    g_blipAddedByScript = 1;
    g_lastBlipPosition[1] = y;
    g_lastBlipPosition[2] = z;
    int blip = Radar_AddBlipAt(radar, 4, x, y, z, kBlipGreen, a, icon, 0);
    Radar_SetBlipSize(radar, blip, 5);
    LuaScript_AddBlipEx(LurStore_GetCurrentScript(TheLurStore), blip, b, c, d);
    Script::PushInt(L, blip);
    return 1;
}
NATIVE_A(BlipAddPoint, 0x005B87F0);

// (handle, unused, icon, [a = 5])
int __cdecl Native_BlipAddObject(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    int a = 5;
    Script::GetInt(L, 1);
    int icon = Script::GetInt(L, 2);
    if (lua::gettop(L) >= 4) {
        a = Script::GetInt(L, 3);
    }
    void* radar = g_radar;
    int blip = Radar_AddBlipFor(radar, 3, handle, 0, a, icon, 0);
    Radar_SetBlipSize(radar, blip, 5);
    LuaScript_AddBlip(LurStore_GetCurrentScript(TheLurStore), blip);
    Script::PushInt(L, blip);
    return 1;
}
NATIVE_A(BlipAddObject, 0x005B89B0);

// (pickup, icon, [a = 5]): -1 for no such pickup
int __cdecl Native_AddBlipForPickup(lua_State* L) {
    int pickup = Script::GetInt(L, 0);
    int a = 5;
    if (Pickups_Find_4438B0(pickup) == -1) {
        Script::PushInt(L, -1);
        return 1;
    }
    int icon = Script::GetInt(L, 1);
    if (lua::gettop(L) >= 3) {
        a = Script::GetInt(L, 2);
    }
    void* radar = g_radar;
    int blip = Radar_AddBlipFor(radar, 11, pickup, 0x02000000, a, icon, 0);
    Radar_SetBlipSize(radar, blip, 5);
    LuaScript_AddBlip(LurStore_GetCurrentScript(TheLurStore), blip);
    Script::PushInt(L, blip);
    return 1;
}
NATIVE_A(AddBlipForPickup, 0x005B8A60);

int __cdecl Native_AddBlipForCar(lua_State* L) {
    void* vehicle = Script_GetVehicle(L, 0);
    int a = 5;
    int icon = Script::GetInt(L, 1);
    if (lua::gettop(L) >= 3) {
        a = Script::GetInt(L, 2);
    }
    int handle = GetVehicleRef(vehicle);
    void* radar = g_radar;
    int blip = Radar_AddBlipFor(radar, 1, handle, 0x02000000, a, icon, 0);
    Radar_SetBlipSize(radar, blip, 5);
    LuaScript_AddBlip(LurStore_GetCurrentScript(TheLurStore), blip);
    Script::PushInt(L, blip);
    return 1;
}
NATIVE_A(AddBlipForCar, 0x005B8B20);

// (trigger, icon, [a = 5]): the trigger's prop; false for none
int __cdecl Native_AddBlipForProp(lua_State* L) {
    int a = 5;
    uint8_t* trigger = TriggerAt(Script::GetInt(L, 0));
    if (trigger != nullptr) {
        void* prop = Trigger_GetPropAnim(trigger);
        if (prop != nullptr) {
            int icon = Script::GetInt(L, 1);
            if (lua::gettop(L) >= 3) {
                a = Script::GetInt(L, 2);
            }
            int handle = GetPropAnimRef(prop);
            void* radar = g_radar;
            int blip = Radar_AddBlipFor(radar, 7, handle, 0x02000000, a, icon, 0);
            Radar_SetBlipSize(radar, blip, 5);
            LuaScript_AddBlip(LurStore_GetCurrentScript(TheLurStore), blip);
            Script::PushInt(L, blip);
            return 1;
        }
    }
    Script::PushBool(L, 0);
    return 1;
}
NATIVE_A(AddBlipForProp, 0x005B8BC0);

int __cdecl Native_BlipRemoveFromChar(lua_State* L) {
    uint8_t* ped = static_cast<uint8_t*>(GetPedFromHandle(Script::GetInt(L, 0), 2));
    if (ped != nullptr) {
        int handle = PedRef(ped);
        void* radar = g_radar;
        Radar_RemoveBlipsFor(radar, 6, handle);
        Radar_RemoveBlipsFor(radar, 2, handle);
        Radar_RemoveBlipsFor(radar, 13, handle);
    }
    return 0;
}
NATIVE_A(BlipRemoveFromChar, 0x005B8CA0);

// (ped, unused, icon (17: kind 13), [a = 5], [b = -1], [c = 0], [d = 2]): -1 for no
// such ped; its old blips go first
int __cdecl Native_AddBlipForChar(lua_State* L) {
    uint8_t* ped = static_cast<uint8_t*>(GetPedFromHandle(Script::GetInt(L, 0), 2));
    int blip;
    if (ped == nullptr) {
        blip = -1;
    } else {
        int icon = Script::GetInt(L, 2);
        float c = 0.0f;
        float d = 2.0f;
        int a = 5;
        int b = -1;
        if (Script::GetArgCount(L) >= 4) {
            a = Script::GetInt(L, 3);
            if (Script::GetArgCount(L) >= 5) {
                b = Script::GetInt(L, 4);
                if (Script::GetArgCount(L) >= 6) {
                    c = Script::GetFloat(L, 5);
                    if (Script::GetArgCount(L) >= 7) {
                        d = Script::GetFloat(L, 6);
                    }
                }
            }
        }
        int handle = PedRef(ped);
        void* radar = g_radar;
        Radar_RemoveBlipsFor(radar, 6, handle);
        Radar_RemoveBlipsFor(radar, 2, handle);
        Radar_RemoveBlipsFor(radar, 13, handle);
        blip = Radar_AddBlipFor(radar, icon == 17 ? 13 : 2, handle, 0, a, icon, 0);
        Radar_SetBlipSize(radar, blip, 5);
        LuaScript_AddBlipEx(LurStore_GetCurrentScript(TheLurStore), blip, b, c, d);
    }
    Script::PushInt(L, blip);
    return 1;
}
NATIVE_A(AddBlipForChar, 0x005B8D10);

// ---- debug

namespace {
GAME_VAR(uint8_t, g_loadLogging, 0x00A135D8);
GAME_VAR(uint8_t, g_loadLoggingOn, 0x00A135D9);
inline int __cdecl game_stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }   // the CRT's _stricmp
GAME_FN(void, __cdecl, LogLoad_439D20, 0x00439D20);
GAME_FN(void, __cdecl, LogLoad_56FF90, 0x0056FF90);
GAME_FN(void, __cdecl, DebugLogMemStats, 0x0051AB30);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
} // namespace

// ("END": the end of the load log)
int __cdecl Native_LogLoad(lua_State* L) {
    const char* what = Script::GetString(L, 0);
    if (game_stricmp(what, "END") == 0 && g_loadLogging != 0) {
        if (g_loadLoggingOn != 0) {
            g_loadLogging = 0;
        }
        LogLoad_439D20();
        LogLoad_56FF90();
    }
    return 0;
}
NATIVE_A(LogLoad, 0x005B8EA0);

int __cdecl Native_DebugLogMemStats(lua_State*) {
    DebugLogMemStats();
    return 0;
}
NATIVE_A(DebugLogMemStats, 0x005B8EE0);

// (bsDumpMemUsage and bsDumpRunTimeMetrics both: stubbed out)
int __cdecl Native_bsDumpMemUsage(lua_State*) {
    PushMemoryTag(0x2C);
    PopMemoryTag();
    return 0;
}
NATIVE_A(bsDumpMemUsage, 0x005B8EF0);

// ---- the camera

namespace {
inline void* const TheCamera = reinterpret_cast<void*>(0x00C3CC68);
GAME_VAR(uint8_t, g_holdFadeout, 0x00C3CE11);
GAME_VAR(uint8_t, g_is169, 0x00C1870E);
GAME_ARRAY(uint8_t, g_fadeColour, 0x00C3CD2C);      // RGBA
GAME_VAR(int32_t, g_widescreen, 0x00C3CD0C);
GAME_VAR(int32_t, g_widescreenTarget, 0x00C3CD10);
GAME_VAR(float, g_widescreenAmount, 0x00C3CD14);
GAME_VAR(void*, g_rwCamera, 0x00C3CCF4);
GAME_VAR(int32_t, g_activeShot, 0x00C3CCF8);
GAME_VAR(int32_t, g_allowChange, 0x00C3CE0C);
GAME_VAR(uint8_t, g_allowScriptedChange, 0x00C3CE10);
GAME_VAR(int32_t, g_cameraCED8, 0x00C3CED8);
GAME_VAR(int32_t, g_fovOverridden, 0x00C3CDD4);
GAME_VAR(float, g_fovOverride, 0x00C3CDD0);
GAME_VAR(float, g_fov, 0x00C3CD04);
GAME_VAR(uint8_t, g_scriptFov, 0x00C3CF82);
GAME_VAR(int32_t, g_cameraDebug, 0x00C3CF00);
GAME_VAR(uint8_t*, g_cameraTargetEntity, 0x00C3CE14);
GAME_VAR(void*, g_secondTarget, 0x00C3CE18);
GAME_ARRAY(float, g_rotationLimit, 0x00C3CE70);     // 7 floats (see SetRotationLimit)
GAME_VAR(uint8_t, g_rotationLimited, 0x00C3CE8C);
GAME_VAR(void*, g_rotationLimitEntity, 0x00C3CE90);
GAME_ARRAY(float, g_playerRelativePathPosition, 0x00CFB2DC);
GAME_VAR(void*, g_convCamCtrl, 0x00C3CFC0);
GAME_ARRAY(float, g_returnToPlayerPosition, 0x00BD0FEC);
inline const float* const g_zeroVector = reinterpret_cast<const float*>(0x00BCC564);
inline const float* const g_vectorAE96C8 = reinterpret_cast<const float*>(0x00AE96C8);

GAME_FN(void, __cdecl, FadeToSplash_Setup, 0x0043BEE0, const char* name);
GAME_FN(void, __cdecl, FadeToSplash_End, 0x0043AD50);
GAME_FN(void, __thiscall, Camera_Fade, 0x004F1AD0, void* camera, float time, int direction, int c);
// the camera of a mode (1: follow, 4: scripted), switching to it
GAME_FN(uint8_t*, __thiscall, Camera_GetCam, 0x004F3590, void* camera, int mode);
GAME_FN(void, __thiscall, ScriptCam_SetPath, 0x00507970, void* cam, void* path, int flag);
GAME_FN(void, __thiscall, ScriptCam_LookAt, 0x00507750, void* cam, int handle, int kind, int flag, float f);
GAME_FN(void, __thiscall, ScriptCam_LookAtXYZ, 0x00507700, void* cam, ScriptVector* position, int flag);
GAME_FN(void, __thiscall, ScriptCam_SetSpeed0, 0x00507830, void* cam, float speed);
GAME_FN(void, __thiscall, ScriptCam_SetSpeed1, 0x00507850, void* cam, float speed);
GAME_FN(void, __thiscall, ScriptCam_SetSpeed2, 0x00507870, void* cam, float speed);
GAME_FN(void, __thiscall, ScriptCam_SetRotationSpeed, 0x00507820, void* cam, float speed);
GAME_FN(void, __thiscall, ScriptCam_LookAtPath, 0x00507790, void* cam, void* path, int flag);
GAME_FN(void, __thiscall, ScriptCam_SetLookAtPathSpeed0, 0x005077C0, void* cam, float speed);
GAME_FN(void, __thiscall, ScriptCam_SetLookAtPathSpeed1, 0x005077E0, void* cam, float speed);
GAME_FN(void, __thiscall, ScriptCam_SetLookAtPathSpeed2, 0x00507800, void* cam, float speed);
GAME_FN(void, __thiscall, ScriptCam_SetRelativePath, 0x00507A80, void* cam, void* path, float* point, float f, int flag, uint8_t flag2);
GAME_FN(void, __thiscall, ScriptCam_SetXYZ, 0x00507C60, void* cam, float x, float y, float z, float lookX, float lookY, float lookZ, int a, uint8_t b);
GAME_FN(void, __thiscall, FollowCam_SetFightShot, 0x004FB6D0, void* cam, const char* name);
GAME_FN(void, __thiscall, FollowCam_SetVehicleShot, 0x004FB6E0, void* cam, const char* name);
GAME_FN(void, __thiscall, Camera_SetShotName, 0x004F3490, void* camera, const char* name);
GAME_FN(void, __thiscall, Camera_SetShot, 0x004F2C20, void* camera, int shot, int a, uint8_t instant, int b, int c);
GAME_FN(void, __thiscall, Camera_ResetToPosition, 0x004F2BD0, void* camera, float x, float y, float z, uint8_t flag);
GAME_FN(void, __thiscall, Camera_Reset, 0x004F2B60, void* camera);
GAME_FN(void, __thiscall, Camera_ResetWithDirection, 0x004F2B90, void* camera, float x, float y, float z);
GAME_FN(void, __thiscall, Camera_Follow, 0x004F3680, void* camera, void* entity);
GAME_FN(void, __thiscall, Camera_SetShotOnly, 0x004F12C0, void* camera, int shot, const char* name);
GAME_FN(void, __thiscall, Camera_DefaultFOV, 0x004F1660, void* camera);
GAME_FN(void, __thiscall, Camera_MgrUpdate, 0x004F3900, void* camera);
GAME_FN(void, __cdecl, RwCameraSetNearPlane, 0x006C8820, void* camera, float plane);
GAME_FN(void*, __cdecl, RwCameraGetFrame, 0x006C82F0, void* camera);
GAME_FN(float*, __cdecl, RwFrameGetLTM, 0x006C9A90, void* frame);
// an entity from a script handle: (handle, kind, ..)
GAME_FN(void*, __cdecl, GetEntityFromHandle, 0x005C28D0, int handle, int kind, int a, int b);
GAME_FN(uint8_t, __cdecl, VectorNotEqual, 0x004305F0, const float* a, const float* b);
GAME_FN(float*, __cdecl, VectorSubtract, 0x00414B70, float* out, const float* a, const float* b);
// CEntity reference keeping (the pointer at `ref` cleared when the entity goes)
GAME_FN(void, __cdecl, Entity_CleanUpOldReference, 0x004657E0, void* entity, re::Ptr32<void>* ref);
GAME_FN(void, __cdecl, Entity_RegisterReference, 0x004657D0, void* entity, re::Ptr32<void>* ref);

// The conversation camera's settings (0x18 bytes, made by 0x4F4DE0)
struct ConvCamParams {
    re::Ptr32<void> ped1;           // +0
    re::Ptr32<void> ped2;           // +4
    int32_t shot;         // +8
    uint8_t flagC;        // +C
    uint8_t flagD;        // +D
    uint8_t unknownE[2];  // +E
    int32_t name;         // +10 (0x684000 of a name)
    int32_t name2;        // +14 (0x4F56B0 of a name)
};
static_assert(sizeof(ConvCamParams) == 0x18);
GAME_FN(void, __thiscall, ConvCamParams_Construct, 0x004F4DE0, ConvCamParams* params);
GAME_FN(int32_t, __cdecl, ConvCam_684000, 0x00684000, const char* name);
GAME_FN(int32_t, __cdecl, ConvCam_4F56B0, 0x004F56B0, const char* name);
GAME_FN(void, __thiscall, ConvCamCtrl_Set, 0x004F6D40, void* ctrl, ConvCamParams* params, int a, int b, int c);

// An entity's position: its matrix's (+0x14, +0x30 in it) or its own (+4)
const float* EntityPosition(uint8_t* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix != nullptr ? &At<float>(matrix, 0x30) : &At<float>(entity, 4);
}

// (angles in radians; the ranges from the camera's frame when all three are ~0)
void SetRotationLimit(float angle1, float angle2, float x, float y, float z) {
    float length = float(double(x) * x + double(y) * y + double(z) * z);
    length = float(std::sqrt(double(std::fabs(length))));
    if (length < 0.001f) {
        float* ltm = RwFrameGetLTM(RwCameraGetFrame(g_rwCamera));
        x = ltm[2];
        y = ltm[5];
        z = ltm[8];
    }
    g_rotationLimit[2] = z;
    g_rotationLimited = 1;
    g_rotationLimit[0] = x;
    g_rotationLimit[5] = -angle1;
    g_rotationLimit[1] = y;
    g_rotationLimit[6] = -angle2;
    g_rotationLimit[3] = angle1;
    g_rotationLimit[4] = angle2;
}

float DegreesToRadians(int degrees) {
    return float(double(degrees) * 3.1415927410125732 / 180.0);
}
} // namespace

int __cdecl Native_CameraHoldFadeout(lua_State* L) {
    g_holdFadeout = Script::GetBool(L, 0);
    return 0;
}
NATIVE_A(CameraHoldFadeout, 0x005B8F70);

int __cdecl Native_CameraFadeToSplashSetup(lua_State*) {
    if (g_is169 != 0) {
        FadeToSplash_Setup("end_ntw");
        return 0;
    }
    FadeToSplash_Setup("end_nt");
    return 0;
}
NATIVE_A(CameraFadeToSplashSetup, 0x005B8F90);

int __cdecl Native_CameraFadeToSplashStart(lua_State*) {
    g_fadeColour[0] = 2;
    g_fadeColour[1] = 2;
    g_fadeColour[2] = 2;
    g_fadeColour[3] = 0xFF;
    return 0;
}
NATIVE_A(CameraFadeToSplashStart, 0x005B8FC0);

int __cdecl Native_CameraFadeToSplashEnd(lua_State*) {
    FadeToSplash_End();
    g_fadeColour[0] = 0;
    g_fadeColour[1] = 0;
    g_fadeColour[2] = 0;
    g_fadeColour[3] = 0xFF;
    return 0;
}
NATIVE_A(CameraFadeToSplashEnd, 0x005B8FE0);

// (ms (negative: the transition's), 0 out / 1 in)
int __cdecl Native_CameraFade(lua_State* L) {
    float time = float(double(Script::GetFloat(L, 0)) / 1000.0);
    int16_t which = int16_t(Script::GetInt(L, 1));
    if (which == 0) {
        if (time < 0.0f) {
            time = At<float>(g_transitionTimes, 0x18);
        }
        Camera_Fade(TheCamera, time, 1, 0);
    } else if (which == 1) {
        if (time < 0.0f) {
            time = At<float>(g_transitionTimes, 0x1C);
        }
        Camera_Fade(TheCamera, time, 0, 0);
    }
    return 0;
}
NATIVE_A(CameraFade, 0x005B9000);

int __cdecl Native_CameraSetWidescreen(lua_State* L) {
    uint8_t on = Script::GetBool(L, 0);
    g_widescreenAmount = 0.0f;
    g_widescreen = on;
    if (Script::GetArgCount(L) == 2 && !Script::GetBool(L, 1)) {
        g_widescreenTarget = g_widescreen;
    }
    return 0;
}
NATIVE_A(CameraSetWidescreen, 0x005B9080);

int __cdecl Native_CameraSetPath(lua_State* L) {
    void* path = PatrolPath_Get(g_patrolPathPoolOwner, Script::GetInt(L, 0));
    if (path != nullptr) {
        int flag = Script::GetBool(L, 1);
        ScriptCam_SetPath(Camera_GetCam(TheCamera, 4), path, flag);
    }
    return 0;
}
NATIVE_A(CameraSetPath, 0x005B90D0);

namespace {
void CameraSetShotNoName(int instant) {
    Camera_SetShotName(TheCamera, nullptr);
    Camera_SetShot(TheCamera, 1, 0, instant != 0, 0, 0);
    Camera_SetShotName(TheCamera, nullptr);
}
} // namespace

// ([instant = true], [toStored = false], [x, y, z]): back to the follow camera, reset to
// the stored position (when set), the given one, or just reset
int __cdecl Native_CameraReturnToPlayer(lua_State* L) {
    float position[3] = {0.0f, 0.0f, 0.0f};
    int instant = 1;
    uint8_t toStored = 0;
    uint8_t hasPosition = 0;
    if (lua::gettop(L) >= 1) {
        instant = Script::GetBool(L, 0);
        toStored = Script::GetBoolOpt(L, 1, 0);
    }
    if (lua::gettop(L) == 5) {
        hasPosition = 1;
        ScriptVector v;
        ScriptVector* p = Script::GetVectorOpt(&v, L, 2, reinterpret_cast<const ScriptVector*>(g_zeroVector));
        position[0] = p->x;
        position[1] = p->y;
        position[2] = p->z;
    }
    if (toStored && VectorNotEqual(g_returnToPlayerPosition, g_zeroVector) && VectorNotEqual(g_returnToPlayerPosition, g_vectorAE96C8)) {
        CameraSetShotNoName(instant);
        Camera_ResetToPosition(TheCamera, g_returnToPlayerPosition[0], g_returnToPlayerPosition[1], g_returnToPlayerPosition[2], 0);
        return 0;
    }
    if (hasPosition && VectorNotEqual(position, g_zeroVector)) {
        CameraSetShotNoName(instant);
        Camera_ResetToPosition(TheCamera, position[0], position[1], position[2], 0);
        return 0;
    }
    uint8_t reset = instant != 0 && g_activeShot == 1;
    CameraSetShotNoName(instant);
    if (reset) {
        Camera_Reset(TheCamera);
    }
    return 0;
}
NATIVE_A(CameraReturnToPlayer, 0x005B9130);

int __cdecl Native_CameraLookAtPlayer(lua_State* L) {
    float f = 0.0f;
    if (lua::gettop(L) == 2) {
        f = Script::GetFloat(L, 1);
    }
    int flag = Script::GetBool(L, 0);
    ScriptCam_LookAt(Camera_GetCam(TheCamera, 4), 0, 3, flag, f);
    return 0;
}
NATIVE_A(CameraLookAtPlayer, 0x005B9340);

int __cdecl Native_CameraSetNearPlane(lua_State* L) {
    float plane = Script::GetFloat(L, 0);
    RwCameraSetNearPlane(g_rwCamera, plane);
    return 0;
}
NATIVE_A(CameraSetNearPlane, 0x005B93B0);

int __cdecl Native_CameraFollowPed(lua_State* L) {
    void* ped = GetPedFromHandle(Script::GetInt(L, 0), 2);
    if (ped != nullptr) {
        Camera_Follow(TheCamera, ped);
        Camera_Reset(TheCamera);
    }
    return 0;
}
NATIVE_A(CameraFollowPed, 0x005B93E0);

int __cdecl Native_CameraFollowEntity(lua_State* L) {
    void* entity = GetEntityFromHandle(Script::GetInt(L, 0), 0, 1, 1);
    if (entity != nullptr) {
        Camera_Follow(TheCamera, entity);
        Camera_Reset(TheCamera);
    }
    return 0;
}
NATIVE_A(CameraFollowEntity, 0x005B9420);

int __cdecl Native_CameraLookAtObject(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    int kind = Script::GetInt(L, 1);
    int flag = Script::GetBool(L, 2);
    float f = 0.0f;
    if (lua::gettop(L) == 4) {
        f = Script::GetFloat(L, 3);
    }
    ScriptCam_LookAt(Camera_GetCam(TheCamera, 4), handle, kind, flag, f);
    return 0;
}
NATIVE_A(CameraLookAtObject, 0x005B9470);

int __cdecl Native_CameraLookAtXYZ(lua_State* L) {
    ScriptVector position;
    position.x = Script::GetFloat(L, 0);
    position.y = Script::GetFloat(L, 1);
    position.z = Script::GetFloat(L, 2);
    int flag = Script::GetBool(L, 3);
    ScriptCam_LookAtXYZ(Camera_GetCam(TheCamera, 4), &position, flag);
    return 0;
}
NATIVE_A(CameraLookAtXYZ, 0x005B94F0);

int __cdecl Native_CameraSetSpeed(lua_State* L) {
    float speed = Script::GetFloat(L, 0);
    ScriptCam_SetSpeed0(Camera_GetCam(TheCamera, 4), speed);
    speed = Script::GetFloat(L, 1);
    ScriptCam_SetSpeed1(Camera_GetCam(TheCamera, 4), speed);
    speed = Script::GetFloat(L, 2);
    ScriptCam_SetSpeed2(Camera_GetCam(TheCamera, 4), speed);
    return 0;
}
NATIVE_A(CameraSetSpeed, 0x005B9570);

int __cdecl Native_CameraSetRotationSpeed(lua_State* L) {
    float speed = Script::GetFloat(L, 0);
    ScriptCam_SetRotationSpeed(Camera_GetCam(TheCamera, 4), speed);
    return 0;
}
NATIVE_A(CameraSetRotationSpeed, 0x005B95F0);

int __cdecl Native_CameraLookAtPath(lua_State* L) {
    void* path = PatrolPath_Get(g_patrolPathPoolOwner, Script::GetInt(L, 0));
    if (path != nullptr) {
        int flag = Script::GetBool(L, 1);
        ScriptCam_LookAtPath(Camera_GetCam(TheCamera, 4), path, flag);
    }
    return 0;
}
NATIVE_A(CameraLookAtPath, 0x005B9630);

int __cdecl Native_CameraLookAtPathSetSpeed(lua_State* L) {
    float speed = Script::GetFloat(L, 0);
    ScriptCam_SetLookAtPathSpeed0(Camera_GetCam(TheCamera, 4), speed);
    speed = Script::GetFloat(L, 1);
    ScriptCam_SetLookAtPathSpeed1(Camera_GetCam(TheCamera, 4), speed);
    speed = Script::GetFloat(L, 2);
    ScriptCam_SetLookAtPathSpeed2(Camera_GetCam(TheCamera, 4), speed);
    return 0;
}
NATIVE_A(CameraLookAtPathSetSpeed, 0x005B9690);

// (path, route, f, flag, [flag2 = false]): relative to the route's first point
int __cdecl Native_CameraSetRelativePath(lua_State* L) {
    void* path = PatrolPath_Get(g_patrolPathPoolOwner, Script::GetInt(L, 0));
    if (path != nullptr) {
        float* point = Route_First(Routes_Get(g_routes, Script::GetInt(L, 1)));
        float f = Script::GetFloat(L, 2);
        uint8_t flag = Script::GetBool(L, 3);
        uint8_t flag2 = 0;
        if (lua::gettop(L) == 5) {
            flag2 = Script::GetBool(L, 4);
        }
        ScriptCam_SetRelativePath(Camera_GetCam(TheCamera, 4), path, point, f, flag, flag2);
    }
    return 0;
}
NATIVE_A(CameraSetRelativePath, 0x005B9710);

// (shot, name (a number: none), [instant = false])
int __cdecl Native_CameraSetShot(lua_State* L) {
    int shot = Script::GetInt(L, 0);
    const char* name;
    if (Script::IsNumber(L, 1)) {
        name = nullptr;
    } else {
        name = Script::GetString(L, 1);
    }
    uint8_t instant = 0;
    if (lua::gettop(L) == 3) {
        instant = Script::GetBool(L, 2);
    }
    if (g_activeShot != shot) {
        Camera_SetShot(TheCamera, shot, 0, instant, 0, 0);
        Camera_SetShotName(TheCamera, name);
        return 0;
    }
    Camera_SetShotName(TheCamera, name);
    if (instant) {
        Camera_Reset(TheCamera);
    }
    return 0;
}
NATIVE_A(CameraSetShot, 0x005B97D0);

int __cdecl Native_CameraSetShotOnly(lua_State* L) {
    int shot = Script::GetInt(L, 0);
    const char* name = Script::GetString(L, 1);
    Camera_SetShotOnly(TheCamera, shot, name);
    return 0;
}
NATIVE_A(CameraSetShotOnly, 0x005B9880);

int __cdecl Native_FollowCamSetFightShot(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    FollowCam_SetFightShot(Camera_GetCam(TheCamera, 1), name);
    return 0;
}
NATIVE_A(FollowCamSetFightShot, 0x005B98B0);

int __cdecl Native_FollowCamDefaultFightShot(lua_State*) {
    FollowCam_SetFightShot(Camera_GetCam(TheCamera, 1), nullptr);
    return 0;
}
NATIVE_A(FollowCamDefaultFightShot, 0x005B98E0);

int __cdecl Native_FollowCamSetVehicleShot(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    FollowCam_SetVehicleShot(Camera_GetCam(TheCamera, 1), name);
    return 0;
}
NATIVE_A(FollowCamSetVehicleShot, 0x005B9900);

int __cdecl Native_FollowCamDefaultVehicleShot(lua_State*) {
    FollowCam_SetVehicleShot(Camera_GetCam(TheCamera, 1), nullptr);
    return 0;
}
NATIVE_A(FollowCamDefaultVehicleShot, 0x005B9930);

int __cdecl Native_CameraReset(lua_State*) {
    Camera_Reset(TheCamera);
    return 0;
}
NATIVE_A(CameraReset, 0x005B9950);

int __cdecl Native_CameraResetToPosition(lua_State* L) {
    float x = Script::GetFloat(L, 0);
    float y = Script::GetFloat(L, 1);
    float z = Script::GetFloat(L, 2);
    uint8_t flag = Script::GetBool(L, 3);
    Camera_ResetToPosition(TheCamera, x, y, z, flag);
    return 0;
}
NATIVE_A(CameraResetToPosition, 0x005B9960);

int __cdecl Native_CameraResetWithDirection2(lua_State* L) {
    float x = Script::GetFloat(L, 0);
    float y = Script::GetFloat(L, 1);
    float z = Script::GetFloat(L, 2);
    Camera_ResetWithDirection(TheCamera, x, y, z);
    return 0;
}
NATIVE_A(CameraResetWithDirection2, 0x005B99D0);

int __cdecl Native_CameraAllowChange(lua_State* L) {
    g_allowChange = Script::GetBool(L, 0);
    g_cameraCED8 = 0;
    return 0;
}
NATIVE_A(CameraAllowChange, 0x005B9A30);

int __cdecl Native_CameraAllowScriptedChange(lua_State* L) {
    g_allowScriptedChange = Script::GetBool(L, 0) != 0;
    g_cameraCED8 = 0;
    return 0;
}
NATIVE_A(CameraAllowScriptedChange, 0x005B9A60);

int __cdecl Native_CameraGetFOV(lua_State* L) {
    float fov = g_fovOverridden != 0 ? g_fovOverride : g_fov;
    Script::PushFloat(L, fov);
    return 1;
}
NATIVE_A(CameraGetFOV, 0x005B9A90);

// (returns 1 with nothing pushed: Lua takes the top of the stack, the argument)
int __cdecl Native_CameraSetFOV(lua_State* L) {
    g_fov = Script::GetFloat(L, 0);
    g_fovOverridden = 0;
    g_scriptFov = 1;
    return 1;
}
NATIVE_A(CameraSetFOV, 0x005B9AD0);

// (returns 1 with nothing pushed)
int __cdecl Native_CameraDefaultFOV(lua_State*) {
    g_scriptFov = 0;
    Camera_DefaultFOV(TheCamera);
    return 1;
}
NATIVE_A(CameraDefaultFOV, 0x005B9B00);

// (name, [shot], [name2 (0: none)], [ped1], [ped2], [flag], [flag2]); the peds default
// to the player
int __cdecl Native_CameraSetConvCam(lua_State* L) {
    ConvCamParams params;
    ConvCamParams_Construct(&params);
    void* player = g_playerPed;
    params.ped2 = player;
    params.ped1 = player;
    if (Script::GetString(L, 0) != nullptr) {
        params.name = ConvCam_684000(Script::GetString(L, 0));
    }
    if (lua::gettop(L) > 1) {
        params.shot = Script::GetInt(L, 1);
    }
    if (lua::gettop(L) > 2 && Script::GetInt(L, 2) != 0) {
        params.name2 = ConvCam_4F56B0(Script::GetString(L, 2));
    }
    if (lua::gettop(L) > 3 && Script::GetInt(L, 3) != 0) {
        params.ped1 = GetPedFromHandle(Script::GetInt(L, 3), 2);
    }
    if (lua::gettop(L) > 4 && Script::GetInt(L, 4) != 0) {
        params.ped2 = GetPedFromHandle(Script::GetInt(L, 4), 2);
    }
    if (lua::gettop(L) > 5) {
        params.flagC = Script::GetInt(L, 5) != 0;
    }
    if (lua::gettop(L) > 6) {
        params.flagD = Script::GetInt(L, 6) != 0;
    }
    ConvCamCtrl_Set(g_convCamCtrl, &params, 1, 1, 0);
    return 0;
}
NATIVE_A(CameraSetConvCam, 0x005B9B20);

// (shot, [time (truncated)], [instant = true])
int __cdecl Native_CameraSetActive(lua_State* L) {
    int shot = Script::GetInt(L, 0);
    float time = 0.0f;
    uint8_t instant = 1;
    if (lua::gettop(L) > 1) {
        time = Script::GetFloat(L, 1);
    }
    if (lua::gettop(L) > 2) {
        instant = Script::GetBool(L, 2);
    }
    Camera_SetShot(TheCamera, shot, int(time), instant, 0, 0);
    return 0;
}
NATIVE_A(CameraSetActive, 0x005B9C90);

int __cdecl Native_CameraGetActive(lua_State* L) {
    Script::PushInt(L, g_activeShot);
    return 1;
}
NATIVE_A(CameraGetActive, 0x005B9D10);

int __cdecl Native_CameraDebugActive(lua_State* L) {
    Script::PushBool(L, g_cameraDebug != 0);
    return 1;
}
NATIVE_A(CameraDebugActive, 0x005B9D30);

// (x, y, z, lookX, lookY, lookZ, [flag = false]): to the scripted camera first
int __cdecl Native_CameraSetXYZ(lua_State* L) {
    uint8_t flag = 0;
    if (lua::gettop(L) >= 7) {
        flag = Script::GetBool(L, 6);
    }
    if (g_activeShot != 4) {
        Camera_SetShot(TheCamera, 4, 0, 1, 0, 0);
    }
    float z = Script::GetFloat(L, 2);
    float y = Script::GetFloat(L, 1);
    float x = Script::GetFloat(L, 0);
    float lookZ = Script::GetFloat(L, 5);
    float lookY = Script::GetFloat(L, 4);
    float lookX = Script::GetFloat(L, 3);
    ScriptCam_SetXYZ(Camera_GetCam(TheCamera, 4), x, y, z, lookX, lookY, lookZ, 1, flag);
    return 0;
}
NATIVE_A(CameraSetXYZ, 0x005B9D50);

int __cdecl Native_CameraMgrUpdate(lua_State*) {
    Camera_MgrUpdate(TheCamera);
    return 0;
}
NATIVE_A(CameraMgrUpdate, 0x005B9E70);

int __cdecl Native_CameraGet169Mode(lua_State* L) {
    Script::PushBool(L, g_is169);
    return 1;
}
NATIVE_A(CameraGet169Mode, 0x005B9E80);

// (path, f, flag): relative to the player's position now
int __cdecl Native_CameraSetPlayerRelativePath(lua_State* L) {
    void* path = PatrolPath_Get(g_patrolPathPoolOwner, Script::GetInt(L, 0));
    if (path != nullptr) {
        const float* position = EntityPosition(g_playerPed);
        g_playerRelativePathPosition[0] = position[0];
        g_playerRelativePathPosition[1] = position[1];
        g_playerRelativePathPosition[2] = position[2];
        float f = Script::GetFloat(L, 1);
        int flag = Script::GetBool(L, 2);
        ScriptCam_SetRelativePath(Camera_GetCam(TheCamera, 4), path, g_playerRelativePathPosition, f, flag, 0);
    }
    return 0;
}
NATIVE_A(CameraSetPlayerRelativePath, 0x005B9EA0);

// (route): the direction from the camera target entity to the route's first point
int __cdecl Native_CameraResetWithDirection(lua_State* L) {
    float* p = Route_First(Routes_Get(g_routes, Script::GetInt(L, 0)));
    float point[3] = {p[0], p[1], p[2]};
    uint8_t* entity = g_cameraTargetEntity;
    if (entity != nullptr) {
        float direction[3];
        VectorSubtract(direction, point, EntityPosition(entity));
        Camera_ResetWithDirection(TheCamera, direction[0], direction[1], direction[2]);
    }
    return 0;
}
NATIVE_A(CameraResetWithDirection, 0x005B9F40);

// (angle1, angle2 (degrees), x, y, z)
int __cdecl Native_CameraSetRotationLimit(lua_State* L) {
    float angle1 = DegreesToRadians(Script::GetInt(L, 0));
    float angle2 = DegreesToRadians(Script::GetInt(L, 1));
    float x = float(Script::GetInt(L, 2));
    float y = float(Script::GetInt(L, 3));
    float z = float(Script::GetInt(L, 4));
    SetRotationLimit(angle1, angle2, x, y, z);
    return 0;
}
NATIVE_A(CameraSetRotationLimit, 0x005B9FD0);

// (ped handle; -1: none)
int __cdecl Native_CameraSetSecondTarget(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    if (handle == -1) {
        if (g_secondTarget != nullptr) {
            Entity_CleanUpOldReference(g_secondTarget, &g_secondTarget);
            g_secondTarget = nullptr;
            return 0;
        }
    } else {
        void* ped = GetPedFromHandle(handle, 2);
        if (ped != g_secondTarget) {
            if (g_secondTarget != nullptr) {
                Entity_CleanUpOldReference(g_secondTarget, &g_secondTarget);
            }
            g_secondTarget = ped;
            if (ped != nullptr) {
                Entity_RegisterReference(ped, &g_secondTarget);
            }
        }
    }
    return 0;
}
NATIVE_A(CameraSetSecondTarget, 0x005BA150);

// (angle1, angle2, x, y, z, entity handle, entity kind): relative to the entity
int __cdecl Native_CameraSetRotationLimitRel(lua_State* L) {
    int handle = Script::GetInt(L, 5);
    int kind = Script::GetInt(L, 6);
    void* entity = GetEntityFromHandle(handle, kind, 0, 0);
    float angle1 = DegreesToRadians(Script::GetInt(L, 0));
    float angle2 = DegreesToRadians(Script::GetInt(L, 1));
    float x = float(Script::GetInt(L, 2));
    float y = float(Script::GetInt(L, 3));
    float z = float(Script::GetInt(L, 4));
    SetRotationLimit(angle1, angle2, x, y, z);
    if (entity != g_rotationLimitEntity) {
        if (g_rotationLimitEntity != nullptr) {
            Entity_CleanUpOldReference(g_rotationLimitEntity, &g_rotationLimitEntity);
        }
        g_rotationLimitEntity = entity;
        if (entity != nullptr) {
            Entity_RegisterReference(entity, &g_rotationLimitEntity);
        }
    }
    return 0;
}
NATIVE_A(CameraSetRotationLimitRel, 0x005BA360);

int __cdecl Native_CameraClearRotationLimit(lua_State*) {
    g_rotationLimited = 0;
    if (g_rotationLimitEntity != nullptr) {
        Entity_CleanUpOldReference(g_rotationLimitEntity, &g_rotationLimitEntity);
        g_rotationLimitEntity = nullptr;
    }
    return 0;
}
NATIVE_A(CameraClearRotationLimit, 0x005BA530);

// ---- the clock, weather and simulation

namespace {
GAME_VAR(uint8_t, g_clockHours, 0x00BCEB4B);
GAME_VAR(uint8_t, g_clockMinutes, 0x00BCEB4A);
GAME_VAR(uint8_t, g_clockPaused, 0x00BCEB42);
GAME_VAR(int32_t, g_msPerGameMinute, 0x009E51A4);
GAME_VAR(int32_t, g_msPerGameMinuteA8, 0x009E51A8);
GAME_VAR(int32_t, g_msPerGameMinuteAC, 0x009E51AC);
GAME_VAR(int32_t, g_clockMission, 0x009E51A0);
GAME_VAR(uint16_t, g_forcedWeather, 0x00CE7578);
GAME_VAR(uint8_t, g_weatherCE754F, 0x00CE754F);
GAME_VAR(uint8_t, g_forceSnow, 0x00CE754E);
GAME_VAR(int16_t, NewWeatherType, 0x00CE756C);
GAME_VAR(uint8_t, g_normalizedTimeOverride, 0x00C66B24);
GAME_VAR(float, g_normalizedTimeOverrideValue, 0x00C66B28);
GAME_VAR(float, g_simulationSpeed, 0x00C1A9A8);
inline void* const g_playerInfo = reinterpret_cast<void*>(0x00C1AEA8);   // (starts with the player ped)
inline void* const g_simulationA45A30 = reinterpret_cast<void*>(0x00A45A30);
inline void* const g_missionNames = reinterpret_cast<void*>(0x020C3CA0);
GAME_FN(void, __cdecl, SetGameClock, 0x00426070, int hours, int minutes);
GAME_FN(void, __cdecl, PauseClock, 0x00425EA0, int pause);
GAME_FN(void, __cdecl, SetWeather, 0x005360E0, int weather, uint8_t flag);
// a hashed-name key (4 bytes: made from a string, and destroyed)
GAME_FN(void, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(void, __thiscall, NameKey_Destruct, 0x0049A650, uint32_t* key);
GAME_FN(int32_t, __thiscall, MissionNames_Find, 0x006A9FD0, void* names, uint32_t* key);
GAME_FN(void, __thiscall, PlayerInfo_SetPaused, 0x00445DA0, void* info, int paused);
GAME_FN(void, __thiscall, Simulation_50DFA0, 0x0050DFA0, void* p, int on);
GAME_FN(void, __thiscall, PedIntelligence_5F3A40, 0x005F3A40, void* p);
GAME_FN(void, __thiscall, Ped_474570, 0x00474570, void* ped);
} // namespace

int __cdecl Native_ClockSet(lua_State* L) {
    int hours = Script::GetInt(L, 0);
    int minutes = Script::GetInt(L, 1);
    SetGameClock(hours, minutes);
    return 0;
}
NATIVE_A(ClockSet, 0x005BA560);

int __cdecl Native_ClockGet(lua_State* L) {
    Script::PushInt(L, g_clockHours);
    Script::PushInt(L, g_clockMinutes);
    return 2;
}
NATIVE_A(ClockGet, 0x005BA590);

// (game minutes per real minute, [the same for 0x9E51A8]): as ms per game minute,
// truncated; a 0 there takes the first
int __cdecl Native_ClockSetTickRate(lua_State* L) {
    float rate = Script::GetFloat(L, 0);
    float rate2 = rate;
    if (Script::GetArgCount(L) >= 2) {
        rate2 = Script::GetFloat(L, 1);
    }
    int32_t ms = int32_t(int64_t(60000.0 / double(rate)));
    g_msPerGameMinuteAC = ms;
    g_msPerGameMinute = ms;
    int32_t ms2 = int32_t(int64_t(60000.0 / double(rate2)));
    g_msPerGameMinuteA8 = ms2;
    if (ms2 == 0) {
        g_msPerGameMinuteA8 = ms;
    }
    return 0;
}
NATIVE_A(ClockSetTickRate, 0x005BA5C0);

int __cdecl Native_ClockSetEnableMission(lua_State* L) {
    uint32_t key;
    NameKey_Construct(&key, Script::GetString(L, 0));
    g_clockMission = MissionNames_Find(g_missionNames, &key);
    NameKey_Destruct(&key);
    return 0;
}
NATIVE_A(ClockSetEnableMission, 0x005BA670);

int __cdecl Native_WeatherRelease(lua_State*) {
    g_forcedWeather = 0xFFFF;
    g_weatherCE754F = 0;
    return 0;
}
NATIVE_A(WeatherRelease, 0x005BA6B0);

int __cdecl Native_WeatherSet(lua_State* L) {
    uint8_t flag = 0;
    if (lua::gettop(L) == 2) {
        flag = Script::GetBool(L, 1);
    }
    SetWeather(Script::GetInt(L, 0), flag);
    return 0;
}
NATIVE_A(WeatherSet, 0x005BA6D0);

int __cdecl Native_WeatherForceSnow(lua_State* L) {
    g_forceSnow = Script::GetBool(L, 0);
    return 0;
}
NATIVE_A(WeatherForceSnow, 0x005BA720);

int __cdecl Native_WeatherGet(lua_State* L) {
    Script::PushInt(L, NewWeatherType);
    return 1;
}
NATIVE_A(WeatherGet, 0x005BA740);

int __cdecl Native_PauseGameClock(lua_State*) {
    PauseClock(1);
    return 0;
}
NATIVE_A(PauseGameClock, 0x005BA760);

int __cdecl Native_UnpauseGameClock(lua_State*) {
    PauseClock(0);
    return 0;
}
NATIVE_A(UnpauseGameClock, 0x005BA770);

int __cdecl Native_ClockIsPaused(lua_State* L) {
    Script::PushBool(L, g_clockPaused);
    return 1;
}
NATIVE_A(ClockIsPaused, 0x005BA780);

int __cdecl Native_NormalizedTimeOverride(lua_State* L) {
    g_normalizedTimeOverride = Script::GetBool(L, 0);
    return 0;
}
NATIVE_A(NormalizedTimeOverride, 0x005BA7A0);

int __cdecl Native_NormalizedTimeOverrideSet(lua_State* L) {
    g_normalizedTimeOverrideValue = Script::GetFloat(L, 0);
    return 0;
}
NATIVE_A(NormalizedTimeOverrideSet, 0x005BA7C0);

// (the player's +0x2E0 +0xC..: its +0..)
int __cdecl Native_PauseSimulation(lua_State*) {
    g_simulationSpeed = 0.0f;
    PlayerInfo_SetPaused(g_playerInfo, 1);
    Simulation_50DFA0(g_simulationA45A30, 0);
    PedIntelligence_5F3A40(At<void*>(g_playerPed, 0x2E0));
    Ped_474570(g_playerPed);
    float* p = At<float*>(g_playerPed, 0x2E0);
    p[3] = p[0];
    p[4] = p[1];
    p[5] = p[2];
    return 0;
}
NATIVE_A(PauseSimulation, 0x005BA7E0);

int __cdecl Native_UnpauseSimulation(lua_State*) {
    g_simulationSpeed = 1.0f;
    PlayerInfo_SetPaused(g_playerInfo, 0);
    Simulation_50DFA0(g_simulationA45A30, 1);
    return 0;
}
NATIVE_A(UnpauseSimulation, 0x005BA840);

// ---- clothing and the shops

namespace {
inline uint8_t* const g_clothing = reinterpret_cast<uint8_t*>(0x020C5F70);   // the player's clothing
GAME_VAR(int32_t, g_currentOutfit, 0x020C76FC);     // -1: none
GAME_ARRAY(uint8_t, g_outfits, 0x020C6B80);         // 0x54 bytes each
GAME_VAR(void*, g_playersHair, 0x020C6B74);
GAME_ARRAY(uint8_t, g_clothingLocks, 0x020C7B90);
GAME_VAR(uint8_t, g_outfitsLocked, 0x020C7B96);
GAME_ARRAY(uint8_t, g_clothingStoreLocks, 0x020C7B97);
GAME_VAR(uint8_t, g_outfitsStoreLocked, 0x020C7B9D);
GAME_VAR(uint8_t, g_clothingExitButton, 0x020C7B9E);
inline void* const g_clothingBackup = reinterpret_cast<void*>(0x00CFB2E8);

// a clothing item: (name hash, slot)
struct ClothingItem {
    uint32_t unknown0;
    uint32_t unknown4;
};
static_assert(sizeof(ClothingItem) == 8);
GAME_FN(void, __thiscall, ClothingItem_FromName, 0x006CB2F0, ClothingItem* item, const char* name, int slot);
GAME_FN(void, __thiscall, ClothingItem_FromHash, 0x006CA4F0, ClothingItem* item, uint32_t hash, int slot);
GAME_FN(void*, __cdecl, GetClothingStore, 0x0053CC70);
GAME_FN(void*, __cdecl, GetBarberShop, 0x0053A530);
GAME_FN(void*, __cdecl, GetTattooStore, 0x0055AEE0);
GAME_FN(void, __thiscall, ClothingStore_Add, 0x006CE5A0, void* store, ClothingItem* item, int price);
GAME_FN(void, __thiscall, BarberShop_Add, 0x006CD0B0, void* shop, int a, uint32_t hash, int b);
GAME_FN(void, __thiscall, BarberShop_SetGroupName, 0x006CCF10, void* shop, int a, uint32_t hash);
GAME_FN(void, __thiscall, TattooStore_Add, 0x006D0360, void* store, int a, int b, const char* name, int c, int d);
// Script::: StoreFunctionRef when there is an argument i
GAME_FN(void, __cdecl, Script_StoreFunctionRefOpt, 0x005DA060, lua_State* L, int i, void* ref);
GAME_FN(void, __thiscall, Clothing_DefineOutfit, 0x006CC0D0, void* clothing, const char* a, const char* b, const char* c, const char* d, const char* e, const char* f, const char* g, const char* h, uint8_t i, uint8_t j);
GAME_FN(void*, __cdecl, ClothingOverlay_Get, 0x006CA7C0);   // non-null while it's up
GAME_FN(void, __cdecl, ClothingOverlay_Begin, 0x006CB550);
GAME_FN(void, __cdecl, ClothingOverlay_End, 0x006CB620);
GAME_FN(void, __thiscall, Clothing_SetPlayer, 0x006CB670, void* clothing, ClothingItem* item);
GAME_FN(void, __thiscall, Clothing_SetPlayerOutfit, 0x006CB720, void* clothing, uint32_t key, uint8_t flag);
GAME_FN(void, __thiscall, Clothing_SetPlayersHair, 0x006CA900, void* clothing, uint32_t hash);
GAME_FN(int, __cdecl, Clothing_5F2450, 0x005F2450, void* item, int a);
GAME_FN(uint8_t, __thiscall, Clothing_PlayerOwns, 0x006CA850, void* clothing, ClothingItem* item);
GAME_FN(uint8_t, __thiscall, Clothing_IsWearingOutfit, 0x006CA9F0, void* clothing, uint32_t hash);
GAME_FN(void, __thiscall, Clothing_GivePlayer, 0x006CB6C0, void* clothing, ClothingItem* item, uint8_t flag);
GAME_FN(void, __thiscall, Clothing_RemovePlayer, 0x006CA7E0, void* clothing, ClothingItem* item);
GAME_FN(uint8_t*, __thiscall, Clothing_FindOutfit, 0x006CAA90, void* clothing, uint32_t key);
GAME_FN(void, __thiscall, Clothing_GivePlayerOutfit, 0x006CB7E0, void* clothing, uint8_t* outfit, uint8_t flag);
GAME_FN(int, __cdecl, Clothing_SlotFromName, 0x006CA4D0, const char* name);
GAME_FN(void, __thiscall, Clothing_Backup, 0x006CAE80, void* clothing, void* backup);
GAME_FN(void, __thiscall, Clothing_Restore, 0x006CB9B0, void* clothing, void* backup);
GAME_FN(void, __thiscall, ClothingOverlay_BuildPlayer, 0x006CC7C0, void* overlay, void* ped);

// (the game compares 7 bytes with repe cmpsb, stopping at the first difference)
bool IsOutfit(const char* name) {
    static const char outfit[7] = {'O', 'U', 'T', 'F', 'I', 'T', 0};
    for (int i = 0; i < 7; i++) {
        if (name[i] != outfit[i]) {
            return false;
        }
    }
    return true;
}

// argument i: a userdata, or a name (hashed)
uint32_t UserdataOrNameHash(lua_State* L, int i) {
    if (Script::IsUserdata(L, i)) {
        return uint32_t(reinterpret_cast<uintptr_t>(Script::GetUserdata(L, i)));
    }
    return HashName(Script::GetString(L, i));
}
} // namespace

int __cdecl Native_ClothingStoreAdd(lua_State* L) {
    int slot = Script::GetInt(L, 0);
    const char* name = Script::GetString(L, 1);
    int price = Script::GetInt(L, 2);
    ClothingItem item;
    ClothingItem_FromName(&item, name, slot);
    ClothingStore_Add(GetClothingStore(), &item, price);
    return 0;
}
NATIVE_A(ClothingStoreAdd, 0x005BA8B0);

int __cdecl Native_BarberShopAdd(lua_State* L) {
    int a = Script::GetInt(L, 0);
    const char* name = Script::GetString(L, 1);
    int b = Script::GetInt(L, 2);
    uint32_t hash = HashName(name);
    BarberShop_Add(GetBarberShop(), a, hash, b);
    return 0;
}
NATIVE_A(BarberShopAdd, 0x005BA910);

int __cdecl Native_BarberShopSetGroupName(lua_State* L) {
    int a = Script::GetInt(L, 0);
    uint32_t hash = HashName(Script::GetString(L, 1));
    BarberShop_SetGroupName(GetBarberShop(), a, hash);
    return 0;
}
NATIVE_A(BarberShopSetGroupName, 0x005BA960);

int __cdecl Native_TattooStoreAdd(lua_State* L) {
    int a = Script::GetInt(L, 0);
    int b = Script::GetInt(L, 1);
    const char* name = Script::GetString(L, 2);
    int c = Script::GetInt(L, 3);
    int d = Script::GetInt(L, 4);
    TattooStore_Add(GetTattooStore(), a, b, name, c, d);
    return 0;
}
NATIVE_A(TattooStoreAdd, 0x005BA9A0);

// (the store's first field is its callback reference)
int __cdecl Native_ClothingStoreRegisterFeedbackCallback(lua_State* L) {
    Script_StoreFunctionRefOpt(L, 0, GetClothingStore());
    return 0;
}
NATIVE_A(ClothingStoreRegisterFeedbackCallback, 0x005BAA00);

int __cdecl Native_BarberStoreRegisterFeedbackCallback(lua_State* L) {
    Script_StoreFunctionRefOpt(L, 0, GetBarberShop());
    return 0;
}
NATIVE_A(BarberStoreRegisterFeedbackCallback, 0x005BAA20);

int __cdecl Native_TattooStoreRegisterFeedbackCallback(lua_State* L) {
    Script_StoreFunctionRefOpt(L, 0, GetTattooStore());
    return 0;
}
NATIVE_A(TattooStoreRegisterFeedbackCallback, 0x005BAA40);

int __cdecl Native_ClothingDefineOutfit(lua_State* L) {
    const char* names[8];
    for (int i = 0; i < 8; i++) {
        names[i] = Script::GetString(L, i);
    }
    uint8_t i8 = Script::GetBool(L, 8);
    uint8_t i9 = Script::GetBool(L, 9);
    Clothing_DefineOutfit(g_clothing, names[0], names[1], names[2], names[3], names[4], names[5], names[6], names[7], i8, i9);
    return 0;
}
NATIVE_A(ClothingDefineOutfit, 0x005BAA60);

// (slot, name or userdata): with the overlay up for it
int __cdecl Native_ClothingSetPlayer(lua_State* L) {
    int slot = Script::GetInt(L, 0);
    uint32_t hash;
    if (Script::IsString(L, 1)) {
        hash = HashName(Script::GetString(L, 1));
    } else {
        hash = uint32_t(reinterpret_cast<uintptr_t>(Script::GetUserdata(L, 1)));
    }
    uint8_t wasUp = ClothingOverlay_Get() != nullptr;
    if (!wasUp) {
        ClothingOverlay_Begin();
    }
    ClothingItem item;
    ClothingItem_FromHash(&item, hash, slot);
    Clothing_SetPlayer(g_clothing, &item);
    if (!wasUp) {
        ClothingOverlay_End();
    }
    return 0;
}
NATIVE_A(ClothingSetPlayer, 0x005BAB20);

int __cdecl Native_ClothingSetPlayerOutfit(lua_State* L) {
    uint32_t key = UserdataOrNameHash(L, 0);
    uint8_t flag = 0;
    if (lua::gettop(L) >= 2) {
        flag = Script::GetBool(L, 1);
    }
    Clothing_SetPlayerOutfit(g_clothing, key, flag);
    return 0;
}
NATIVE_A(ClothingSetPlayerOutfit, 0x005BABA0);

int __cdecl Native_ClothingSetPlayersHair(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    uint8_t wasUp = ClothingOverlay_Get() != nullptr;
    if (!wasUp) {
        ClothingOverlay_Begin();
    }
    Clothing_SetPlayersHair(g_clothing, HashName(name));
    if (!wasUp) {
        ClothingOverlay_End();
    }
    return 0;
}
NATIVE_A(ClothingSetPlayersHair, 0x005BAC10);

// (slot): the item there (a userdata) and 0x5F2450 of it
int __cdecl Native_ClothingGetPlayer(lua_State* L) {
    re::Ptr32<void>* item = reinterpret_cast<re::Ptr32<void>*>(g_clothing + Script::GetInt(L, 0) * 12);
    Script::PushUserdata(L, *item);
    Script::PushInt(L, Clothing_5F2450(*item, -1));
    return 2;
}
NATIVE_A(ClothingGetPlayer, 0x005BAC60);

int __cdecl Native_ClothingGetPlayerOutfit(lua_State* L) {
    uint8_t* outfit = g_currentOutfit == -1 ? nullptr : g_outfits + g_currentOutfit * 0x54;
    void* name = nullptr;
    if (outfit != nullptr) {
        name = At<void*>(outfit, 0);
    }
    Script::PushUserdata(L, name);
    return 1;
}
NATIVE_A(ClothingGetPlayerOutfit, 0x005BACB0);

int __cdecl Native_ClothingGetPlayersHair(lua_State* L) {
    void* hair = g_playersHair;
    int n = Clothing_5F2450(hair, -1);
    Script::PushUserdata(L, hair);
    Script::PushInt(L, n);
    return 2;
}
NATIVE_A(ClothingGetPlayersHair, 0x005BACF0);

int __cdecl Native_ClothingPlayerOwns(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    int slot = int8_t(Script::GetInt(L, 1));
    ClothingItem item;
    ClothingItem_FromName(&item, name, slot);
    Script::PushBool(L, Clothing_PlayerOwns(g_clothing, &item));
    return 1;
}
NATIVE_A(ClothingPlayerOwns, 0x005BAD40);

int __cdecl Native_ClothingIsWearingOutfit(lua_State* L) {
    uint32_t hash = HashName(Script::GetString(L, 0));
    Script::PushBool(L, Clothing_IsWearingOutfit(g_clothing, hash));
    return 1;
}
NATIVE_A(ClothingIsWearingOutfit, 0x005BADA0);

int __cdecl Native_ClothingIsWearingAnyOutfit(lua_State* L) {
    uint8_t* outfit = g_currentOutfit == -1 ? nullptr : g_outfits + g_currentOutfit * 0x54;
    Script::PushBool(L, outfit != nullptr);
    return 1;
}
NATIVE_A(ClothingIsWearingAnyOutfit, 0x005BADE0);

int __cdecl Native_ClothingGivePlayer(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    int slot = Script::GetInt(L, 1);
    uint8_t flag = 1;
    if (lua::gettop(L) >= 3) {
        flag = Script::GetBool(L, 2);
    }
    ClothingItem item;
    ClothingItem_FromName(&item, name, slot);
    Clothing_GivePlayer(g_clothing, &item, flag);
    return 0;
}
NATIVE_A(ClothingGivePlayer, 0x005BAE10);

int __cdecl Native_ClothingRemovePlayer(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    int slot = Script::GetInt(L, 1);
    ClothingItem item;
    ClothingItem_FromName(&item, name, slot);
    Clothing_RemovePlayer(g_clothing, &item);
    return 0;
}
NATIVE_A(ClothingRemovePlayer, 0x005BAE80);

// (outfit, [a = true]: sets its +0x50 bit 3, [flag = false])
int __cdecl Native_ClothingGivePlayerOutfit(lua_State* L) {
    uint32_t key = UserdataOrNameHash(L, 0);
    uint8_t a = 1;
    if (lua::gettop(L) >= 2) {
        a = Script::GetBool(L, 1);
    }
    uint8_t flag = 0;
    if (lua::gettop(L) >= 3) {
        flag = Script::GetBool(L, 2);
    }
    uint8_t* outfit = Clothing_FindOutfit(g_clothing, key);
    if (outfit != nullptr) {
        if (a) {
            outfit[0x50] |= 8;
        }
        Clothing_GivePlayerOutfit(g_clothing, outfit, flag);
    }
    return 0;
}
NATIVE_A(ClothingGivePlayerOutfit, 0x005BAEC0);

// (slot name or "OUTFIT", locked)
int __cdecl Native_ClothingLock(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    uint8_t locked = Script::GetBool(L, 1);
    if (IsOutfit(name)) {
        g_outfitsLocked = locked;
        return 0;
    }
    g_clothingLocks[Clothing_SlotFromName(name)] = locked;
    return 0;
}
NATIVE_A(ClothingLock, 0x005BAF70);

int __cdecl Native_ClothingStoreLock(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    uint8_t locked = Script::GetBool(L, 1);
    if (IsOutfit(name)) {
        g_outfitsStoreLocked = locked;
        return 0;
    }
    g_clothingStoreLocks[Clothing_SlotFromName(name)] = locked;
    return 0;
}
NATIVE_A(ClothingStoreLock, 0x005BAFD0);

int __cdecl Native_ClothingExitButton(lua_State* L) {
    g_clothingExitButton = Script::GetBool(L, 0);
    return 0;
}
NATIVE_A(ClothingExitButton, 0x005BB030);

int __cdecl Native_ClothingBackup(lua_State*) {
    Clothing_Backup(g_clothing, g_clothingBackup);
    return 0;
}
NATIVE_A(ClothingBackup, 0x005BB050);

int __cdecl Native_ClothingRestore(lua_State*) {
    Clothing_Restore(g_clothing, g_clothingBackup);
    return 0;
}
NATIVE_A(ClothingRestore, 0x005BB070);

int __cdecl Native_ClothingBuildPlayer(lua_State*) {
    uint8_t wasUp = ClothingOverlay_Get() != nullptr;
    if (!wasUp) {
        ClothingOverlay_Begin();
    }
    void* player = g_playerPed;
    ClothingOverlay_BuildPlayer(ClothingOverlay_Get(), player);
    if (!wasUp) {
        ClothingOverlay_End();
    }
    return 0;
}
NATIVE_A(ClothingBuildPlayer, 0x005BB090);

int __cdecl Native_ClothingOverlay(lua_State* L) {
    if (Script::GetBool(L, 0)) {
        ClothingOverlay_Begin();
        return 0;
    }
    ClothingOverlay_End();
    return 0;
}
NATIVE_A(ClothingOverlay, 0x005BB0D0);

// ---- collectibles and cutscenes

namespace {
inline void* const g_collectibles = reinterpret_cast<void*>(0x00BCEB50);
inline void* const g_collectedCallback = reinterpret_cast<void*>(0x00BD09DC);
GAME_VAR(uint8_t, g_cutsceneRunning, 0x020C5BE4);
GAME_FN(void, __thiscall, Collectibles_SetTypeAvailable, 0x00426490, void* c, int type, uint8_t available);
GAME_FN(void, __thiscall, Collectibles_SetAllAsCollected, 0x00428CF0, void* c, int type, uint8_t collected);
GAME_FN(int16_t, __thiscall, Collectibles_GetNumCollectable, 0x00426430, void* c, int type);
GAME_FN(int16_t, __thiscall, Collectibles_GetNumCollected, 0x00426440, void* c, int type);
GAME_FN(void, __thiscall, Collectibles_OnMapEnable, 0x00428B70, void* c, int type, uint8_t enable);
GAME_FN(void, __cdecl, LoadCutscene, 0x006C45E0, const char* name, uint8_t flag);
GAME_FN(void, __cdecl, LoadCutsceneSound, 0x006C38B0, const char* name);
GAME_FN(void, __cdecl, StartCutscene, 0x006C6370);
GAME_FN(void, __cdecl, StopCutscene, 0x006C4030);
GAME_FN(int, __cdecl, GetCutsceneTime, 0x006C3AD0);
} // namespace

int __cdecl Native_CollectiblesSetTypeAvailable(lua_State* L) {
    uint8_t available = 1;
    if (lua::gettop(L) >= 2) {
        available = Script::GetBool(L, 1);
    }
    Collectibles_SetTypeAvailable(g_collectibles, Script::GetInt(L, 0), available);
    return 0;
}
NATIVE_A(CollectiblesSetTypeAvailable, 0x005BB100);

int __cdecl Native_CollectiblesOnCollectedCallback(lua_State* L) {
    Script_StoreFunctionRefOpt(L, 0, g_collectedCallback);
    return 0;
}
NATIVE_A(CollectiblesOnCollectedCallback, 0x005BB150);

int __cdecl Native_CollectiblesSetAllAsCollected(lua_State* L) {
    int type = Script::GetInt(L, 0);
    uint8_t collected = 1;
    if (lua::gettop(L) >= 2) {
        collected = Script::GetBool(L, 1);
    }
    Collectibles_SetAllAsCollected(g_collectibles, type, collected);
    return 0;
}
NATIVE_A(CollectiblesSetAllAsCollected, 0x005BB170);

int __cdecl Native_CollectiblesGetNumCollectable(lua_State* L) {
    Script::PushInt(L, Collectibles_GetNumCollectable(g_collectibles, Script::GetInt(L, 0)));
    return 1;
}
NATIVE_A(CollectiblesGetNumCollectable, 0x005BB1C0);

int __cdecl Native_CollectiblesGetNumCollected(lua_State* L) {
    Script::PushInt(L, Collectibles_GetNumCollected(g_collectibles, Script::GetInt(L, 0)));
    return 1;
}
NATIVE_A(CollectiblesGetNumCollected, 0x005BB1F0);

int __cdecl Native_CollectibleOnMapEnable(lua_State* L) {
    int type = Script::GetInt(L, 0);
    uint8_t enable = Script::GetBool(L, 1);
    Collectibles_OnMapEnable(g_collectibles, type, enable);
    return 0;
}
NATIVE_A(CollectibleOnMapEnable, 0x005BB220);

int __cdecl Native_LoadCutscene(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    uint8_t flag = Script::GetBoolOpt(L, 1, 1);
    LoadCutscene(name, flag);
    return 0;
}
NATIVE_A(LoadCutscene, 0x005BB260);

int __cdecl Native_LoadCutsceneSound(lua_State* L) {
    LoadCutsceneSound(Script::GetString(L, 0));
    return 0;
}
NATIVE_A(LoadCutsceneSound, 0x005BB290);

int __cdecl Native_StartCutscene(lua_State*) {
    StartCutscene();
    return 0;
}
NATIVE_A(StartCutscene, 0x005BB2B0);

int __cdecl Native_StopCutscene(lua_State*) {
    StopCutscene();
    return 0;
}
NATIVE_A(StopCutscene, 0x005BB2C0);

int __cdecl Native_GetCutsceneTime(lua_State* L) {
    Script::PushInt(L, GetCutsceneTime());
    return 1;
}
NATIVE_A(GetCutsceneTime, 0x005BB2D0);

int __cdecl Native_GetCutsceneRunning(lua_State* L) {
    Script::PushInt(L, g_cutsceneRunning);
    return 1;
}
NATIVE_A(GetCutsceneRunning, 0x005BB2F0);

namespace {
GAME_VAR(uint8_t, g_cutsceneLoaded, 0x020C5BE2);
GAME_VAR(uint8_t, g_inNIS, 0x00CFB340);
GAME_FN(uint8_t, __cdecl, Cutscene_SetActionNode, 0x006C3F00, const char* node, uintptr_t a);
GAME_FN(void, __cdecl, Cutscene_SetTableSize, 0x00425450, int size);
GAME_FN(void, __cdecl, Cutscene_SetName, 0x00425430, const char* name);
GAME_FN(void, __cdecl, Cutscene_AddPAnim, 0x006C3FD0, void* prop);
GAME_FN(uint8_t, __thiscall, Ped_IsAmbient, 0x004765C0, void* ped);
GAME_FN(void*, __thiscall, PedTasks_GetActive, 0x00470340, void* tasks);
GAME_FN(void*, __thiscall, Task_GetTarget, 0x008B6120, void* task);
GAME_FN(void, __thiscall, PedTasks_Abort, 0x00470260, void* tasks, int type);
GAME_FN(void, __thiscall, PedAttitude_48F380, 0x0048F380, void* p);
GAME_FN(void, __thiscall, PedAttitude_4910A0, 0x004910A0, void* p);
GAME_FN(void, __thiscall, Ped_498B10, 0x00498B10, void* ped, int a);
GAME_FN(void, __thiscall, Vehicle_SetInNIS, 0x004CB530, void* vehicle, int on);
} // namespace

int __cdecl Native_CutSceneSetActionNode(lua_State* L) {
    Script::PushBool(L, Cutscene_SetActionNode(Script::GetString(L, 0), 0));
    return 1;
}
NATIVE_A(CutSceneSetActionNode, 0x005BB310);

int __cdecl Native_CutSetCutsceneTableSize(lua_State* L) {
    Cutscene_SetTableSize(Script::GetInt(L, 0));
    return 0;
}
NATIVE_A(CutSetCutsceneTableSize, 0x005BB340);

int __cdecl Native_CutSetCutsceneName(lua_State* L) {
    Cutscene_SetName(Script::GetString(L, 0));
    return 0;
}
NATIVE_A(CutSetCutsceneName, 0x005BB360);

int __cdecl Native_IsCutsceneLoaded(lua_State* L) {
    Script::PushBool(L, g_cutsceneLoaded);
    return 1;
}
NATIVE_A(IsCutsceneLoaded, 0x005BB3A0);

// (trigger): its prop (+0x1C)
int __cdecl Native_AddPAnimToNextCutscene(lua_State* L) {
    if (Script::IsNumber(L, 0)) {
        uint8_t* trigger = TriggerAt(Script::GetInt(L, 0));
        if (trigger != nullptr) {
            void* prop = At<void*>(trigger, 0x1C);
            if (prop != nullptr) {
                Cutscene_AddPAnim(prop);
            }
        }
    }
    return 0;
}
NATIVE_A(AddPAnimToNextCutscene, 0x005BB3C0);

// (+0x1370 of every ambient ped)
int __cdecl Native_SetAmbientPedsIgnoreStimuli(lua_State* L) {
    uint8_t ignore = Script::GetBool(L, 0);
    CPool<uint8_t>* pool = ms_pPedPool;
    for (int slot = pool->size; slot != 0;) {
        slot--;
        uint8_t* ped = pool->GetAt(slot);
        if (ped != nullptr && Ped_IsAmbient(ped)) {
            At<int>(ped, 0x1370) = ignore;
        }
    }
    return 0;
}
NATIVE_A(SetAmbientPedsIgnoreStimuli, 0x005BB410);

// (ambient peds attacking (task 7) the player (+0x1570 == 2) or a type 13 ped stop)
int __cdecl Native_StopAmbientPedAttacks(lua_State*) {
    CPool<uint8_t>* pool = ms_pPedPool;
    for (int slot = pool->size; slot != 0;) {
        slot--;
        uint8_t* ped = pool->GetAt(slot);
        if (ped == nullptr || !Ped_IsAmbient(ped)) {
            continue;
        }
        void* task = PedTasks_GetActive(ped + 0x5F0);
        if (task == nullptr || At<int>(task, 8) != 7) {
            continue;
        }
        uint8_t* target = static_cast<uint8_t*>(Task_GetTarget(task));
        if (target == nullptr || (target[0x1570] != 2 && At<int>(target, 0x1310) != 13)) {
            continue;
        }
        PedTasks_Abort(ped + 0x5F0, 7);
        PedAttitude_48F380(ped + 0xB44);
        PedAttitude_4910A0(ped + 0xB44);
        Ped_498B10(ped, 0);
    }
    return 0;
}
NATIVE_A(StopAmbientPedAttacks, 0x005BB470);

namespace {
void SetVehiclesInNIS(int on) {
    CPool<uint8_t>* pool = ms_pVehiclePool;
    for (int slot = 0; slot < pool->size; slot++) {
        if (!pool->IsFreeSlot(slot)) {
            Vehicle_SetInNIS(pool->objects + pool->elemSize * slot, on);
        }
    }
}
} // namespace

int __cdecl Native_EnterNIS(lua_State*) {
    g_inNIS = 1;
    SetVehiclesInNIS(1);
    return 0;
}
NATIVE_A(EnterNIS, 0x005BB520);

int __cdecl Native_ExitNIS(lua_State*) {
    g_inNIS = 0;
    SetVehiclesInNIS(0);
    return 0;
}
NATIVE_A(ExitNIS, 0x005BB560);

int __cdecl Native_DockerSetMinimumRange(lua_State* L) {
    void* docker = Dockers_Get(g_dockers, Script::GetUserdata(L, 0));
    At<float>(docker, 0x104) = Script::GetFloat(L, 1);
    return 0;
}
NATIVE_A(DockerSetMinimumRange, 0x005BB5A0);

int __cdecl Native_DockerSetMaximumRange(lua_State* L) {
    void* docker = Dockers_Get(g_dockers, Script::GetUserdata(L, 0));
    At<float>(docker, 0x108) = Script::GetFloat(L, 1);
    return 0;
}
NATIVE_A(DockerSetMaximumRange, 0x005BB5E0);

// (+0xFC bit 6)
int __cdecl Native_DockerSetUseHeightCheck(lua_State* L) {
    void* docker = Dockers_Get(g_dockers, Script::GetUserdata(L, 0));
    uint8_t on = Script::GetBool(L, 1);
    uint8_t& flags = At<uint8_t>(docker, 0xFC);
    flags = uint8_t((flags & ~0x40) | ((on << 6) & 0x40));
    return 0;
}
NATIVE_A(DockerSetUseHeightCheck, 0x005BB620);

// (+0xFC bit 7)
int __cdecl Native_DockerSetUseFacingCheck(lua_State* L) {
    void* docker = Dockers_Get(g_dockers, Script::GetUserdata(L, 0));
    uint8_t on = Script::GetBool(L, 1);
    uint8_t& flags = At<uint8_t>(docker, 0xFC);
    flags = uint8_t((flags & 0x7F) | (on << 7));
    return 0;
}
NATIVE_A(DockerSetUseFacingCheck, 0x005BB660);

// (+0xFD bit 0: overridden, bit 1: the setting)
int __cdecl Native_DockerSetOverrideActiveSetting(lua_State* L) {
    void* docker = Dockers_Get(g_dockers, Script::GetUserdata(L, 0));
    uint8_t active = Script::GetBool(L, 1);
    uint8_t& flags = At<uint8_t>(docker, 0xFD);
    flags = uint8_t(((flags & ~2) | ((active << 1) & 2)) | 1);
    return 0;
}
NATIVE_A(DockerSetOverrideActiveSetting, 0x005BB6B0);

int __cdecl Native_DockerClearOverrideActiveSetting(lua_State* L) {
    void* docker = Dockers_Get(g_dockers, Script::GetUserdata(L, 0));
    At<uint8_t>(docker, 0xFD) &= 0xFE;
    return 0;
}
NATIVE_A(DockerClearOverrideActiveSetting, 0x005BB700);

// ---- effects

namespace {
inline void* const g_effects = reinterpret_cast<void*>(0x00C13FD8);
inline void* const g_leaves = reinterpret_cast<void*>(0x00BD5C90);
inline void* const g_windowGlow = reinterpret_cast<void*>(0x00CE7580);
GAME_VAR(float, g_gymFireAlpha, 0x00AC7634);
GAME_VAR(int32_t, g_gymFireOn, 0x00CF1090);
GAME_VAR(int32_t, g_blindedByMud, 0x00CF1094);
GAME_VAR(int32_t, g_blindedByMudTime, 0x00CF1098);
GAME_VAR(uint8_t, g_doublePedShadowDistance, 0x00C93768);
GAME_FN(void, __thiscall, Leaves_AddAt, 0x00439F00, void* leaves, float* point, int n);
GAME_FN(void, __thiscall, Effects_UnregisterInArea, 0x00454100, void* effects, int id);
GAME_FN(void, __thiscall, Effects_Kill, 0x00454070, void* effects, int id);
GAME_FN(void, __thiscall, Effects_SlowKill, 0x004540A0, void* effects, int id, float time, uint8_t flag);
GAME_FN(uint8_t, __thiscall, Effects_IsRunning, 0x00453E70, void* effects, int id);
GAME_FN(uint8_t, __thiscall, Effects_SetNightOnly, 0x00453FE0, void* effects, int id, uint8_t on);
GAME_FN(void, __thiscall, WindowGlow_Add, 0x00536B70, void* glow, ScriptVector* a, ScriptVector* b, ScriptVector* c, ScriptVector* d, int e, float f, float g, float h, uint8_t i, uint8_t j, int k);
GAME_FN(void, __thiscall, WindowGlow_TurnOn, 0x00537D00, void* glow, ScriptVector* centre, float radius);
GAME_FN(void, __thiscall, WindowGlow_TurnOff, 0x00537BF0, void* glow, ScriptVector* centre, float radius);
GAME_FN(int, __thiscall, Effects_RegisterInArea, 0x00454380, void* effects, const char* name, ScriptVector* position, int a);
GAME_FN(int, __thiscall, Effects_Create, 0x00453DD0, void* effects, const char* name, ScriptVector* position);
GAME_FN(uint8_t, __thiscall, Effects_SetPosition, 0x00453EC0, void* effects, int id, ScriptVector* position);
GAME_FN(uint8_t, __thiscall, Effects_SetDirection, 0x00453F60, void* effects, int id, float* direction);
// a direction from two angles (into out, 16 bytes)
GAME_FN(void, __cdecl, DirectionFromAngles, 0x00412380, float a, float b, float* out);
} // namespace

// (route, n): at every point of the route
int __cdecl Native_EffectAddLeavesInArea(lua_State* L) {
    int slot = Script::GetInt(L, 0);
    int n = Script::GetInt(L, 1);
    void* route = Routes_Get(g_routes, slot);
    float* point = Route_First(route);
    while (point != nullptr) {
        float* next = Route_Next(route);
        Leaves_AddAt(g_leaves, point, n);
        point = next;
    }
    return 0;
}
NATIVE_A(EffectAddLeavesInArea, 0x005BB730);

int __cdecl Native_EffectUnregisterInArea(lua_State* L) {
    Effects_UnregisterInArea(g_effects, Script::GetInt(L, 0));
    return 0;
}
NATIVE_A(EffectUnregisterInArea, 0x005BB7A0);

int __cdecl Native_EffectKill(lua_State* L) {
    Effects_Kill(g_effects, Script::GetInt(L, 0));
    return 0;
}
NATIVE_A(EffectKill, 0x005BB7C0);

// (id, [time = 1], [flag = false])
int __cdecl Native_EffectSlowKill(lua_State* L) {
    int id = Script::GetInt(L, 0);
    float time = 1.0f;
    uint8_t flag = 0;
    if (lua::gettop(L) > 1) {
        time = Script::GetFloat(L, 1);
        if (lua::gettop(L) > 2) {
            flag = Script::GetBool(L, 2);
        }
    }
    Effects_SlowKill(g_effects, id, time, flag);
    return 0;
}
NATIVE_A(EffectSlowKill, 0x005BB7E0);

int __cdecl Native_EffectIsRunning(lua_State* L) {
    Script::PushBool(L, Effects_IsRunning(g_effects, Script::GetInt(L, 0)));
    return 1;
}
NATIVE_A(EffectIsRunning, 0x005BB860);

int __cdecl Native_EffectSetNightOnly(lua_State* L) {
    int id = Script::GetInt(L, 0);
    uint8_t on = Script::GetBool(L, 1);
    Script::PushBool(L, Effects_SetNightOnly(g_effects, id, on));
    return 1;
}
NATIVE_A(EffectSetNightOnly, 0x005BB8A0);

// (returns 1 with nothing pushed)
int __cdecl Native_EffectSetGymnFireOn(lua_State* L) {
    uint8_t on = Script::GetBool(L, 0);
    g_gymFireAlpha = 1.0f;
    g_gymFireOn = on;
    return 1;
}
NATIVE_A(EffectSetGymnFireOn, 0x005BB8F0);

// (returns 1 with nothing pushed)
int __cdecl Native_EffectBlindedbyMud(lua_State*) {
    g_blindedByMud = 1;
    g_blindedByMudTime = 120;
    return 1;
}
NATIVE_A(EffectBlindedbyMud, 0x005BB920);

// (returns 1 with nothing pushed)
int __cdecl Native_DoublePedShadowDistance(lua_State* L) {
    g_doublePedShadowDistance = Script::GetBool(L, 0);
    return 1;
}
NATIVE_A(DoublePedShadowDistance, 0x005BB940);

// (id, 4 vectors, 3 floats, 2 flags, an int; returns 1 with nothing pushed)
int __cdecl Native_EffectAddWindowGlowInArea(lua_State* L) {
    int id = Script::GetInt(L, 0);
    ScriptVector v[4];
    for (int i = 0; i < 4; i++) {
        v[i].x = Script::GetFloat(L, 1 + i * 3);
        v[i].y = Script::GetFloat(L, 2 + i * 3);
        v[i].z = Script::GetFloat(L, 3 + i * 3);
    }
    float f = Script::GetFloat(L, 13);
    float g = Script::GetFloat(L, 14);
    float h = Script::GetFloat(L, 15);
    uint8_t i16 = Script::GetBool(L, 16);
    uint8_t i17 = Script::GetBool(L, 17);
    int k = Script::GetInt(L, 18);
    WindowGlow_Add(g_windowGlow, &v[0], &v[1], &v[2], &v[3], id, f, g, h, i16, i17, k);
    return 1;
}
NATIVE_A(EffectAddWindowGlowInArea, 0x005BB960);

int __cdecl Native_EffectTurnOnWindowGlowInArea(lua_State* L) {
    ScriptVector centre;
    centre.x = Script::GetFloat(L, 0);
    centre.y = Script::GetFloat(L, 1);
    centre.z = Script::GetFloat(L, 2);
    float radius = Script::GetFloat(L, 3);
    WindowGlow_TurnOn(g_windowGlow, &centre, radius);
    return 0;
}
NATIVE_A(EffectTurnOnWindowGlowInArea, 0x005BBAA0);

int __cdecl Native_EffectTurnOffWindowGlowInArea(lua_State* L) {
    ScriptVector centre;
    centre.x = Script::GetFloat(L, 0);
    centre.y = Script::GetFloat(L, 1);
    centre.z = Script::GetFloat(L, 2);
    float radius = Script::GetFloat(L, 3);
    WindowGlow_TurnOff(g_windowGlow, &centre, radius);
    return 0;
}
NATIVE_A(EffectTurnOffWindowGlowInArea, 0x005BBB10);

int __cdecl Native_EffectRegisterInArea(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    ScriptVector position;
    position.x = Script::GetFloat(L, 1);
    position.y = Script::GetFloat(L, 2);
    position.z = Script::GetFloat(L, 3);
    int a = Script::GetInt(L, 4);
    Script::PushInt(L, Effects_RegisterInArea(g_effects, name, &position, a));
    return 1;
}
NATIVE_A(EffectRegisterInArea, 0x005BBB80);

int __cdecl Native_EffectCreate(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    ScriptVector position;
    position.x = Script::GetFloat(L, 1);
    position.y = Script::GetFloat(L, 2);
    position.z = Script::GetFloat(L, 3);
    Script::PushInt(L, Effects_Create(g_effects, name, &position));
    return 1;
}
NATIVE_A(EffectCreate, 0x005BBC00);

int __cdecl Native_EffectSetPosition(lua_State* L) {
    int id = Script::GetInt(L, 0);
    ScriptVector position;
    position.x = Script::GetFloat(L, 1);
    position.y = Script::GetFloat(L, 2);
    position.z = Script::GetFloat(L, 3);
    Script::PushBool(L, Effects_SetPosition(g_effects, id, &position));
    return 1;
}
NATIVE_A(EffectSetPosition, 0x005BBC80);

int __cdecl Native_EffectSetSphereDirection(lua_State* L) {
    int id = Script::GetInt(L, 0);
    float a = Script::GetFloat(L, 1);
    float b = Script::GetFloat(L, 2);
    float direction[4];
    DirectionFromAngles(a, b, direction);
    Script::PushBool(L, Effects_SetDirection(g_effects, id, direction));
    return 1;
}
NATIVE_A(EffectSetSphereDirection, 0x005BBD00);

// (clamped to 0..1; returns 1 with nothing pushed)
int __cdecl Native_EffectSetGymnFireAlpha(lua_State* L) {
    float alpha = Script::GetFloat(L, 0);
    if (alpha < 0.0f) {
        alpha = 0.0f;
    } else if (1.0f < alpha) {
        alpha = 1.0f;
    }
    g_gymFireAlpha = alpha;
    return 1;
}
NATIVE_A(EffectSetGymnFireAlpha, 0x005BBD80);

int __cdecl Native_EffectSetDirection(lua_State* L) {
    int id = Script::GetInt(L, 0);
    float direction[3];
    direction[0] = Script::GetFloat(L, 1);
    direction[1] = Script::GetFloat(L, 2);
    direction[2] = Script::GetFloat(L, 3);
    Script::PushBool(L, Effects_SetDirection(g_effects, id, direction));
    return 1;
}
NATIVE_A(EffectSetDirection, 0x005BBDD0);

// ---- event handlers (argument 2 a function: registered; else unregistered)

namespace {
GAME_FN(void*, __cdecl, GlobalEvents_Register, 0x005B1DC0, int* event, int flag);
GAME_FN(void, __cdecl, GlobalEvents_Unregister, 0x005B1DF0, int* event, int flag);
GAME_FN(void, __cdecl, HashEvents_Prepare, 0x005B4040, void* object);
GAME_FN(void*, __cdecl, HashEvents_Register, 0x005B3990, void* object, int* event);
GAME_FN(void, __cdecl, HashEvents_Unregister, 0x005B3CE0, void* object, int* event);
GAME_FN(void, __cdecl, PedEvents_Prepare, 0x005B4760, void* ped);
GAME_FN(void*, __cdecl, PedEvents_Register, 0x005B4190, void* ped, int* event);
GAME_FN(void, __cdecl, PedEvents_Unregister, 0x005B4430, void* ped, int* event);
GAME_FN(void, __cdecl, HashEvents_SetNumberOfObjects, 0x005B40F0, int n, int scriptId);
GAME_FN(void, __cdecl, TriggerEvents_SetNumberOfObjects, 0x005B5FB0, int n, int scriptId, int a);
GAME_FN(void, __cdecl, PropEvents_SetNumberOfObjects, 0x005B5030, int n, int scriptId);
GAME_FN(void, __cdecl, PedEvents_SetNumberOfObjects, 0x005B47E0, int n, int scriptId);
GAME_FN(void, __cdecl, TriggerEvents_Prepare, 0x005B5E70, void* trigger, int a);
GAME_FN(void*, __cdecl, TriggerEvents_Register, 0x005B5150, void* trigger, int* event, int a);
GAME_FN(void, __cdecl, TriggerEvents_Unregister, 0x005B5850, void* trigger, int* event, int a);
GAME_FN(void, __cdecl, PropEvents_Prepare, 0x005B4F90, void* prop);
GAME_FN(void*, __cdecl, PropEvents_Register, 0x005B4880, void* prop, int* event);
GAME_FN(void, __cdecl, PropEvents_Unregister, 0x005B4BA0, void* prop, int* event);
} // namespace

// (event, function, [flag])
int __cdecl Native_RegisterGlobalEventHandler(lua_State* L) {
    int args = Script::GetArgCount(L);
    int event = Script::GetInt(L, 0);
    if (Script::IsFunction(L, 1)) {
        int flag;
        if (args == 2) {
            flag = 0;
        } else {
            flag = Script::GetInt(L, 2) != 0 ? 1 : 0;
        }
        Script_StoreFunctionRefOpt(L, 1, GlobalEvents_Register(&event, flag));
        return 0;
    }
    GlobalEvents_Unregister(&event, 0);
    return 0;
}
NATIVE_A(RegisterGlobalEventHandler, 0x005BBE50);

// (object, event, function)
int __cdecl Native_RegisterHashEventHandler(lua_State* L) {
    void* object = Script::GetUserdata(L, 0);
    int event = Script::GetInt(L, 1);
    if (Script::IsFunction(L, 2)) {
        HashEvents_Prepare(object);
        Script_StoreFunctionRefOpt(L, 2, HashEvents_Register(object, &event));
        return 0;
    }
    HashEvents_Unregister(object, &event);
    return 0;
}
NATIVE_A(RegisterHashEventHandler, 0x005BBEE0);

// (ped, event, function)
int __cdecl Native_RegisterPedEventHandler(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    int event = Script::GetInt(L, 1);
    void* ped = GetPedFromHandle(handle, 2);
    if (ped != nullptr) {
        if (Script::IsFunction(L, 2)) {
            PedEvents_Prepare(ped);
            Script_StoreFunctionRefOpt(L, 2, PedEvents_Register(ped, &event));
            return 0;
        }
        PedEvents_Unregister(ped, &event);
    }
    return 0;
}
NATIVE_A(RegisterPedEventHandler, 0x005BBF50);

// (the script's id: read through a null script outside one)
int __cdecl Native_SetNumberOfHandledHashEventObjects(lua_State* L) {
    int n = Script::GetInt(L, 0);
    void* script = LurStore_GetCurrentScript(TheLurStore);
    if (n > 0) {
        HashEvents_SetNumberOfObjects(n, At<int>(script, 0x44));
    }
    return 0;
}
NATIVE_A(SetNumberOfHandledHashEventObjects, 0x005BBFD0);

// (n, [a = 1]; the script's id is read first either way)
int __cdecl Native_SetNumberOfHandledTriggerEventObjects(lua_State* L) {
    int args = Script::GetArgCount(L);
    int n = Script::GetInt(L, 0);
    void* script = LurStore_GetCurrentScript(TheLurStore);
    int scriptId = At<int>(script, 0x44);
    if (n > 0) {
        if (args == 1) {
            TriggerEvents_SetNumberOfObjects(n, scriptId, 1);
            return 0;
        }
        if (args == 2) {
            TriggerEvents_SetNumberOfObjects(n, scriptId, Script::GetInt(L, 1));
        }
    }
    return 0;
}
NATIVE_A(SetNumberOfHandledTriggerEventObjects, 0x005BC010);

int __cdecl Native_SetNumberOfHandledPropEventObjects(lua_State* L) {
    int n = Script::GetInt(L, 0);
    void* script = LurStore_GetCurrentScript(TheLurStore);
    if (n > 0) {
        PropEvents_SetNumberOfObjects(n, At<int>(script, 0x44));
    }
    return 0;
}
NATIVE_A(SetNumberOfHandledPropEventObjects, 0x005BC080);

int __cdecl Native_SetNumberOfHandledPedEventObjects(lua_State* L) {
    int n = Script::GetInt(L, 0);
    void* script = LurStore_GetCurrentScript(TheLurStore);
    if (n > 0) {
        PedEvents_SetNumberOfObjects(n, At<int>(script, 0x44));
    }
    return 0;
}
NATIVE_A(SetNumberOfHandledPedEventObjects, 0x005BC0C0);

// (trigger, event, function, [a]): only with 3 or 4 arguments
int __cdecl Native_RegisterTriggerEventHandler(lua_State* L) {
    int args = Script::GetArgCount(L);
    int slot = Script::GetInt(L, 0);
    int event = Script::GetInt(L, 1);
    uint8_t* trigger = TriggerAt(slot);
    if (trigger == nullptr) {
        return 0;
    }
    if (Script::IsFunction(L, 2)) {
        if (args == 3) {
            TriggerEvents_Prepare(trigger, 1);
            Script_StoreFunctionRefOpt(L, 2, TriggerEvents_Register(trigger, &event, 1));
            return 0;
        }
        if (args == 4) {
            int a = Script::GetInt(L, 3);
            TriggerEvents_Prepare(trigger, a);
            Script_StoreFunctionRefOpt(L, 2, TriggerEvents_Register(trigger, &event, a));
            return 0;
        }
    } else {
        if (args == 3) {
            TriggerEvents_Unregister(trigger, &event, 1);
            return 0;
        }
        if (args == 4) {
            TriggerEvents_Unregister(trigger, &event, 0);
        }
    }
    return 0;
}
NATIVE_A(RegisterTriggerEventHandler, 0x005BC100);

// (trigger, event, function): the trigger's prop (+0x1C)
int __cdecl Native_RegisterPropEventHandler(lua_State* L) {
    int slot = Script::GetInt(L, 0);
    int event = Script::GetInt(L, 1);
    uint8_t* trigger = TriggerAt(slot);
    if (trigger == nullptr) {
        return 0;
    }
    void* prop = At<void*>(trigger, 0x1C);
    if (prop == nullptr) {
        return 0;
    }
    if (Script::IsFunction(L, 2)) {
        PropEvents_Prepare(prop);
        Script_StoreFunctionRefOpt(L, 2, PropEvents_Register(prop, &event));
        return 0;
    }
    PropEvents_Unregister(prop, &event);
    return 0;
}
NATIVE_A(RegisterPropEventHandler, 0x005BC200);

// ---- garages

namespace {
GAME_FN(void, __cdecl, Garages_ClearAll, 0x004C8B90);
GAME_FN(void, __cdecl, Garages_SetIsDeactivated, 0x004C8940, uint8_t deactivated);
GAME_FN(void, __cdecl, Garages_SetStoredVehicle, 0x004C8960, int a, int b);
GAME_FN(uint8_t, __cdecl, Garages_4C89F0, 0x004C89F0);
GAME_FN(float, __cdecl, LimitAngle_4333A0, 0x004333A0, float angle);
GAME_FN(int, __cdecl, Garages_Add, 0x004C8BA0, float* min, float* max, float* points, float* headings, int count);
} // namespace

int __cdecl Native_GarageClearAll(lua_State*) {
    Garages_ClearAll();
    return 0;
}
NATIVE_A(GarageClearAll, 0x005BC290);

int __cdecl Native_GarageSetIsDeactivated(lua_State* L) {
    Garages_SetIsDeactivated(Script::GetBool(L, 0));
    return 0;
}
NATIVE_A(GarageSetIsDeactivated, 0x005BC2A0);

int __cdecl Native_GarageSetStoredVehicle(lua_State* L) {
    int a = Script::GetInt(L, 0);
    int b = Script::GetInt(L, 1);
    Garages_SetStoredVehicle(a, b);
    return 0;
}
NATIVE_A(GarageSetStoredVehicle, 0x005BC2D0);

int __cdecl Native_GarageHasStoredVehicle(lua_State* L) {
    Script::PushBool(L, !Garages_4C89F0());
    return 1;
}
NATIVE_A(GarageHasStoredVehicle, 0x005BC300);

// (trigger, route): the trigger's box (+0x20: min +8.., max +0x20..) around its
// position (+4), and the route's points with their headings (+0x1C, degrees). Reads
// through nullptr for no such trigger. (The game keeps two points and headings on its
// stack; more overrun it: those aren't kept here)
int __cdecl Native_GarageAdd(lua_State* L) {
    int slot = Script::GetInt(L, 0);
    int routeSlot = Script::GetInt(L, 1);
    uint8_t* trigger = TriggerAt(slot);
    uint8_t* box = At<uint8_t*>(trigger, 0x20);
    void* route = Routes_Get(g_routes, routeSlot);
    // (as on the game's stack: headings, max, min, points)
    float frame[14];
    float* headings = &frame[0];
    float* max = &frame[2];
    float* min = &frame[5];
    float* points = &frame[8];
    min[1] = At<float>(box, 0x24) + At<float>(trigger, 8);
    min[2] = At<float>(trigger, 0xC) - At<float>(box, 0xC);
    min[0] = At<float>(box, 0x20) + At<float>(trigger, 4);
    max[1] = At<float>(box, 0x2C) + At<float>(trigger, 8);
    max[2] = At<float>(box, 8) + At<float>(trigger, 0xC);
    max[0] = At<float>(box, 0x28) + At<float>(trigger, 4);
    for (int i = 0; i < 6; i++) {
        points[i] = 0.0f;
    }
    int count = 0;
    for (float* p = Route_First(route); p != nullptr; p = Route_Next(route)) {
        if (count < 2) {
            points[count * 3] = p[0];
            points[count * 3 + 1] = p[1];
            points[count * 3 + 2] = p[2];
        }
        float heading = LimitAngle_4333A0(float(double(p[7]) * 3.1415927410125732 / 180.0));
        if (count < 8) {
            frame[count] = heading;   // (past two: over max and min, as in the game)
        }
        count++;
    }
    Script::PushInt(L, Garages_Add(min, max, points, headings, count));
    return 1;
}
NATIVE_A(GarageAdd, 0x005BC320);

// ---- the counter (HUD)

namespace {
GAME_VAR(uint8_t*, g_counter, 0x00C674C0);
inline char* __cdecl game_strncpy(char* dest, const char* source, size_t count) { return crt::Strncpy(dest, source, count); }   // the CRT's
GAME_FN(void, __thiscall, Counter_SetIcon, 0x0053F200, void* counter, const char* texture, const char* mask);
GAME_FN(void, __thiscall, Counter_SetTextXYOffset, 0x0053F120, void* counter, float x, float y);
GAME_FN(void, __thiscall, Counter_ClearIcon, 0x0053F270, void* counter);
GAME_FN(void, __thiscall, Counter_SetMax, 0x0053F1E0, void* counter, int max);
GAME_FN(void, __thiscall, Counter_SetCurrent, 0x0053F1F0, void* counter, int current);
GAME_FN(void, __thiscall, Counter_IncrementCurrent, 0x0053F1C0, void* counter, int by);
GAME_FN(void, __thiscall, Counter_EnableRoll, 0x0053F140, void* counter, uint8_t on, const char* a, const char* b, float f);
GAME_FN(void, __thiscall, CRGBA_Construct, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
} // namespace

// (visible (its virtual +0x14), [flag at +0x60])
int __cdecl Native_CounterMakeHUDVisible(lua_State* L) {
    uint8_t* counter = g_counter;
    using SetVisibleFn = void(__thiscall*)(void*, uint8_t);
    SetVisibleFn setVisible = re::GameVirtual<SetVisibleFn>(counter, 5);
    setVisible(counter, Script::GetBool(L, 0));
    counter = g_counter;
    if (lua::gettop(L) == 2) {
        counter[0x60] = Script::GetBool(L, 1);
    }
    return 0;
}
NATIVE_A(CounterMakeHUDVisible, 0x005BC480);

int __cdecl Native_CounterSetIcon(lua_State* L) {
    uint8_t* counter = g_counter;
    const char* mask = Script::GetString(L, 1);
    const char* texture = Script::GetString(L, 0);
    Counter_SetIcon(counter, texture, mask);
    return 0;
}
NATIVE_A(CounterSetIcon, 0x005BC4D0);

int __cdecl Native_CounterSetText(lua_State* L) {
    uint8_t* counter = g_counter;
    game_strncpy(reinterpret_cast<char*>(counter + 0x63), Script::GetString(L, 0), 0x40);
    return 0;
}
NATIVE_A(CounterSetText, 0x005BC500);

int __cdecl Native_CounterSetTextXYOffset(lua_State* L) {
    uint8_t* counter = g_counter;
    float x = Script::GetFloat(L, 0);
    float y = Script::GetFloat(L, 1);
    Counter_SetTextXYOffset(counter, x, y);
    return 0;
}
NATIVE_A(CounterSetTextXYOffset, 0x005BC530);

int __cdecl Native_CounterClearText(lua_State*) {
    uint8_t* counter = g_counter;
    At<float>(counter, 0xA4) = 0.0f;
    counter[0x63] = 0;
    At<float>(counter, 0xA8) = 0.0f;
    return 0;
}
NATIVE_A(CounterClearText, 0x005BC580);

int __cdecl Native_CounterClearIcon(lua_State*) {
    Counter_ClearIcon(g_counter);
    return 0;
}
NATIVE_A(CounterClearIcon, 0x005BC5A0);

int __cdecl Native_CounterSetMax(lua_State* L) {
    uint8_t* counter = g_counter;
    Counter_SetMax(counter, Script::GetInt(L, 0));
    return 0;
}
NATIVE_A(CounterSetMax, 0x005BC5B0);

int __cdecl Native_CounterGetMax(lua_State* L) {
    Script::PushInt(L, At<int>(g_counter, 0x4C));
    return 1;
}
NATIVE_A(CounterGetMax, 0x005BC5E0);

int __cdecl Native_CounterGetCurrent(lua_State* L) {
    Script::PushInt(L, At<int>(g_counter, 0x50));
    return 1;
}
NATIVE_A(CounterGetCurrent, 0x005BC600);

int __cdecl Native_CounterSetCurrent(lua_State* L) {
    uint8_t* counter = g_counter;
    Counter_SetCurrent(counter, Script::GetInt(L, 0));
    return 0;
}
NATIVE_A(CounterSetCurrent, 0x005BC620);

int __cdecl Native_CounterIncrementCurrent(lua_State* L) {
    uint8_t* counter = g_counter;
    Counter_IncrementCurrent(counter, Script::GetInt(L, 0));
    return 0;
}
NATIVE_A(CounterIncrementCurrent, 0x005BC650);

// (on, a, [b = nullptr], f)
int __cdecl Native_CounterEnableRoll(lua_State* L) {
    uint8_t* counter = g_counter;
    float f = Script::GetFloat(L, 3);
    const char* b = Script::GetStringOpt(L, 2, nullptr);
    const char* a = Script::GetString(L, 1);
    uint8_t on = Script::GetBool(L, 0);
    Counter_EnableRoll(counter, on, a, b, f);
    return 0;
}
NATIVE_A(CounterEnableRoll, 0x005BC680);

int __cdecl Native_CounterUseMeter(lua_State* L) {
    uint8_t* counter = g_counter;
    counter[0x61] = Script::GetBool(L, 0);
    return 0;
}
NATIVE_A(CounterUseMeter, 0x005BC6D0);

// (r, g, b, a) into +0x5C
int __cdecl Native_CounterSetTextColour(lua_State* L) {
    int a = Script::GetInt(L, 3);
    int b = Script::GetInt(L, 2);
    int g = Script::GetInt(L, 1);
    int r = Script::GetInt(L, 0);
    uint8_t rgba[4];
    CRGBA_Construct(rgba, r, g, b, a);
    uint8_t* counter = g_counter;
    counter[0x5D] = rgba[1];
    counter[0x5C] = rgba[0];
    counter[0x5E] = rgba[2];
    counter[0x5F] = rgba[3];
    return 0;
}
NATIVE_A(CounterSetTextColour, 0x005BC6F0);

// ---- paths, point lists and geometry

namespace {
GAME_FN(uint32_t, __thiscall, Path_GetNumNodes, 0x005EC670, void* path);
GAME_FN(float*, __thiscall, Path_GetNode, 0x005EC8C0, void* path, uint32_t node);
GAME_FN(int, __thiscall, Route_GetNumPoints, 0x006D5950, void* route);
GAME_FN(int, __cdecl, GetModelIndexByName, 0x0051C330, const char* name);
GAME_FN(void, __cdecl, Geometry_Instance, 0x0045E060, int model, ScriptVector* position, uint8_t a, uint8_t b);
GAME_FN(void*, __cdecl, Geometry_FindObject, 0x004607A0, int model, ScriptVector* position, float radius, int a);
GAME_FN(void, __thiscall, Entity_SetAlpha, 0x00466160, void* entity, int alpha);
GAME_FN(uint8_t*, __cdecl, Geometry_Find, 0x0045DE60, int model, float radius, ScriptVector* position);
GAME_FN(void*, __cdecl, GetEntity, 0x0044A660, int handle, int kind);
GAME_FN(void, __cdecl, PersistentEntity_45DD70, 0x0045DD70, void* entity);
GAME_FN(void, __cdecl, PersistentEntity_45E690, 0x0045E690, void* entity);

// argument i: a model index, or a model name
int ModelArgument(lua_State* L, int i) {
    if (Script::IsNumber(L, i)) {
        return Script::GetInt(L, i);
    }
    return GetModelIndexByName(Script::GetString(L, i));
}

// x, y, z, heading (+0x1C), 0, 0
void PushRoutePoint(lua_State* L, const float* p) {
    float x = p[0];
    float y = p[1];
    float z = p[2];
    Script::PushFloat(L, x);
    Script::PushFloat(L, y);
    Script::PushFloat(L, z);
    Script::PushFloat(L, p[7]);
    Script::PushFloat(L, 0.0f);
    Script::PushFloat(L, 0.0f);
}
} // namespace

// (path): its last node's index, nil for none
int __cdecl Native_PathGetLastNode(lua_State* L) {
    void* path = PatrolPath_Get(g_patrolPathPoolOwner, Script::GetInt(L, 0));
    if (path != nullptr && Path_GetNumNodes(path) != 0) {
        Script::PushInt(L, int(Path_GetNumNodes(path) - 1));
        return 1;
    }
    Script::PushNil(L);
    return 1;
}
NATIVE_A(PathGetLastNode, 0x005BC760);

// (route): every point (6 values each)
int __cdecl Native_GetPointList(lua_State* L) {
    int results = 0;
    void* route = Routes_Get(g_routes, Script::GetInt(L, 0));
    float* p = Route_First(route);
    if (p == nullptr) {
        return 0;
    }
    do {
        float* next = Route_Next(route);
        PushRoutePoint(L, p);
        results += 6;
        p = next;
    } while (p != nullptr);
    return results;
}
NATIVE_A(GetPointList, 0x005BC7C0);

int __cdecl Native_GetPointListSize(lua_State* L) {
    void* route = Routes_Get(g_routes, Script::GetInt(L, 0));
    if (route == nullptr) {
        Script::PushInt(L, 0);
        return 1;
    }
    Script::PushInt(L, Route_GetNumPoints(route));
    return 1;
}
NATIVE_A(GetPointListSize, 0x005BC8A0);

// (route, n): the n-th point (1-based; the last for past the end), zeros for no route
int __cdecl Native_GetPointFromPointList(lua_State* L) {
    int slot = Script::GetInt(L, 0);
    int n = Script::GetInt(L, 1);
    void* route = Routes_Get(g_routes, slot);
    if (route == nullptr) {
        for (int i = 0; i < 6; i++) {
            Script::PushFloat(L, 0.0f);
        }
        return 6;
    }
    float* p = Route_First(route);
    for (int i = 1; i < n;) {
        float* next = Route_Next(route);
        if (next == nullptr) {
            break;
        }
        i++;
        p = next;
    }
    PushRoutePoint(L, p);
    return 6;
}
NATIVE_A(GetPointFromPointList, 0x005BC8F0);

// (model, a, x, y, z, [b = false])
int __cdecl Native_GeometryInstance(lua_State* L) {
    int model = ModelArgument(L, 0);
    uint8_t a = Script::GetBool(L, 1);
    ScriptVector position;
    position.x = Script::GetFloat(L, 2);
    position.y = Script::GetFloat(L, 3);
    position.z = Script::GetFloat(L, 4);
    uint8_t b = 0;
    if (lua::gettop(L) == 6) {
        b = Script::GetBool(L, 5);
    }
    Geometry_Instance(model, &position, a, b);
    return 0;
}
NATIVE_A(GeometryInstance, 0x005BCA40);

// (model, alpha, x, y, z): the object within 10 of there
int __cdecl Native_GeometryAlpha(lua_State* L) {
    int model = ModelArgument(L, 0);
    int alpha = Script::GetInt(L, 1);
    ScriptVector position;
    position.x = Script::GetFloat(L, 2);
    position.y = Script::GetFloat(L, 3);
    position.z = Script::GetFloat(L, 4);
    void* object = Geometry_FindObject(model, &position, 10.0f, 1);
    if (object != nullptr) {
        Entity_SetAlpha(object, alpha);
    }
    return 0;
}
NATIVE_A(GeometryAlpha, 0x005BCB00);

int __cdecl Native_DeletePersistentEntity(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    int kind = Script::GetInt(L, 1);
    void* entity = GetEntity(handle, kind);
    if (entity != nullptr) {
        PersistentEntity_45DD70(entity);
        PersistentEntity_45E690(entity);
        using DeleteFn = void(__thiscall*)(void*, int);
        re::GameVirtual<DeleteFn>(entity, 0)(entity, 1);
    }
    return 0;
}
NATIVE_A(DeletePersistentEntity, 0x005BCBB0);

// (path, node): its position, truncated; zeros for none
int __cdecl Native_GetPointFromPath(lua_State* L) {
    int slot = Script::GetInt(L, 0);
    uint32_t node = uint32_t(Script::GetInt(L, 1));
    void* path = PatrolPath_Get(g_patrolPathPoolOwner, slot);
    if (path != nullptr && node < Path_GetNumNodes(path)) {
        float* p = Path_GetNode(path, node);
        Script::PushInt(L, int(p[0]));
        Script::PushInt(L, int(p[1]));
        Script::PushInt(L, int(p[2]));
        return 3;
    }
    Script::PushInt(L, 0);
    Script::PushInt(L, 0);
    Script::PushInt(L, 0);
    return 3;
}
NATIVE_A(GetPointFromPath, 0x005BCC00);

// (model, flag, x, y, z): the flag (+0x1C + 4 flag) of the geometry within 10 of there
int __cdecl Native_GetGeometryFlagState(lua_State* L) {
    int model = ModelArgument(L, 0);
    int flag = Script::GetInt(L, 1);
    ScriptVector position;
    position.x = Script::GetFloat(L, 2);
    position.y = Script::GetFloat(L, 3);
    position.z = Script::GetFloat(L, 4);
    uint8_t* geometry = Geometry_Find(model, 10.0f, &position);
    Script::PushBool(L, geometry != nullptr && At<int>(geometry, 0x1C + flag * 4) != 0);
    return 1;
}
NATIVE_A(GetGeometryFlagState, 0x005BCCB0);

// ---- HUD: the struggle button, components, the button history

namespace {
GAME_VAR(uint8_t*, g_struggleButton, 0x00C674E4);
GAME_ARRAY(uint8_t*, g_hudComponents, 0x00C674A0);
GAME_VAR(void*, g_hudC674D4, 0x00C674D4);
GAME_VAR(void*, g_buttonHistory, 0x00C674F4);
GAME_FN(void, __thiscall, HUD_542810, 0x00542810, void* p);
GAME_FN(void, __thiscall, ButtonHistory_SetTimeDisplayed, 0x0053ABB0, void* bh, int line, float time);
GAME_FN(void, __thiscall, ButtonHistory_SetNumButtonsDisplayed, 0x0053ABD0, void* bh, int line, int n);
GAME_FN(void, __thiscall, ButtonHistory_ClearLine, 0x0053AC70, void* bh, int line);
GAME_FN(void, __thiscall, ButtonHistory_AddButton, 0x0053B330, void* bh, int line, int button, uint8_t flag);
GAME_FN(void, __thiscall, ButtonHistory_IgnoreController, 0x0053B430, void* bh, uint8_t ignore);
GAME_FN(void, __thiscall, ButtonHistory_GetFromHistory, 0x0053ACC0, void* bh, int line, int n, int* button, int* flag);
GAME_FN(void*, __thiscall, ButtonHistory_CorrectButtonCallback, 0x0053AE00, void* bh);
GAME_FN(void*, __thiscall, ButtonHistory_PassedCallback, 0x008B5470, void* bh);
GAME_FN(void*, __thiscall, ButtonHistory_FailedCallback, 0x0053ADF0, void* bh);
GAME_FN(void, __thiscall, ButtonHistory_ClearSequence, 0x0053AD00, void* bh);
GAME_FN(void, __thiscall, ButtonHistory_SetSequenceTime, 0x0053ADC0, void* bh, int ms);
// (button, flag, ms, a, b)
GAME_FN(void, __thiscall, ButtonHistory_AddToSequence, 0x0053AEA0, void* bh, int button, uint8_t flag, int ms, int a, int b);
GAME_FN(void, __thiscall, ButtonHistory_AddChargeText, 0x0053AF10, void* bh, int n);
GAME_FN(void, __thiscall, ButtonHistory_IgnoreInSequence, 0x0053AF30, void* bh, int button);
GAME_FN(void, __thiscall, ButtonHistory_EnableActionTreeInput, 0x0053B0E0, void* bh, uint8_t on);
GAME_FN(void, __thiscall, ButtonHistory_AddSequenceLocalText, 0x0053AE70, void* bh, const char* text);

int SecondsToMs(float seconds) {
    return int(double(seconds) * 1000.0);
}
} // namespace

int __cdecl Native_StruggleButtonSetText(lua_State* L) {
    uint8_t* button = g_struggleButton;
    game_strncpy(reinterpret_cast<char*>(button + 0x44), Script::GetString(L, 0), 0xFF);
    return 0;
}
NATIVE_A(StruggleButtonSetText, 0x005BD5E0);

int __cdecl Native_StruggleButtonSetButton(lua_State* L) {
    uint8_t* button = g_struggleButton;
    int which = Script::GetInt(L, 0);
    At<float>(button, 0x164) = 0.0f;
    At<int>(button, 0x20) = which;
    return 0;
}
NATIVE_A(StruggleButtonSetButton, 0x005BD610);

int __cdecl Native_StruggleButtonSetDisplay(lua_State* L) {
    uint8_t* button = g_struggleButton;
    At<int>(button, 0x15C) = Script::GetInt(L, 0);
    return 0;
}
NATIVE_A(StruggleButtonSetDisplay, 0x005BD640);

// (component, visible (its virtual +0x14)); showing 13 also calls 0x542810
int __cdecl Native_ToggleHUDComponentVisibility(lua_State* L) {
    int component = Script::GetInt(L, 0);
    uint8_t visible = Script::GetBool(L, 1);
    uint8_t* hud = g_hudComponents[component];
    using SetVisibleFn = void(__thiscall*)(void*, uint8_t);
    re::GameVirtual<SetVisibleFn>(hud, 5)(hud, visible);
    if (component == 13 && visible) {
        HUD_542810(g_hudC674D4);
    }
    return 0;
}
NATIVE_A(ToggleHUDComponentVisibility, 0x005BD660);

int __cdecl Native_ToggleHUDComponentLocked(lua_State* L) {
    int component = Script::GetInt(L, 0);
    uint8_t locked = Script::GetBool(L, 1);
    g_hudComponents[component][0x15] = locked;
    return 0;
}
NATIVE_A(ToggleHUDComponentLocked, 0x005BD6C0);

int __cdecl Native_HUDComponentGetVisibility(lua_State* L) {
    uint8_t* hud = g_hudComponents[Script::GetInt(L, 0)];
    using IsVisibleFn = uint8_t(__thiscall*)(void*);
    Script::PushBool(L, re::GameVirtual<IsVisibleFn>(hud, 4)(hud));
    return 1;
}
NATIVE_A(HUDComponentGetVisibility, 0x005BD6F0);

int __cdecl Native_HUDComponentGetFlashing(lua_State* L) {
    Script::PushBool(L, g_hudComponents[Script::GetInt(L, 0)][0x14]);
    return 1;
}
NATIVE_A(HUDComponentGetFlashing, 0x005BD720);

int __cdecl Native_HUDComponentGetLocked(lua_State* L) {
    Script::PushBool(L, g_hudComponents[Script::GetInt(L, 0)][0x15]);
    return 1;
}
NATIVE_A(HUDComponentGetLocked, 0x005BD750);

int __cdecl Native_ButtonHistorySetTimeDisplayed(lua_State* L) {
    void* bh = g_buttonHistory;
    float time = Script::GetFloat(L, 1);
    ButtonHistory_SetTimeDisplayed(bh, Script::GetInt(L, 0), time);
    return 0;
}
NATIVE_A(ButtonHistorySetTimeDisplayed, 0x005BD780);

int __cdecl Native_ButtonHistorySetNumButtonsDisplayed(lua_State* L) {
    void* bh = g_buttonHistory;
    int n = Script::GetInt(L, 1);
    ButtonHistory_SetNumButtonsDisplayed(bh, Script::GetInt(L, 0), n);
    return 0;
}
NATIVE_A(ButtonHistorySetNumButtonsDisplayed, 0x005BD7C0);

int __cdecl Native_ButtonHistoryClearLine(lua_State* L) {
    void* bh = g_buttonHistory;
    ButtonHistory_ClearLine(bh, Script::GetInt(L, 0));
    return 0;
}
NATIVE_A(ButtonHistoryClearLine, 0x005BD7F0);

int __cdecl Native_ButtonHistoryAddButton(lua_State* L) {
    uint8_t flag = 0;
    if (lua::gettop(L) > 2) {
        flag = Script::GetBool(L, 2);
    }
    void* bh = g_buttonHistory;
    int button = Script::GetInt(L, 1);
    ButtonHistory_AddButton(bh, Script::GetInt(L, 0), button, flag);
    return 0;
}
NATIVE_A(ButtonHistoryAddButton, 0x005BD820);

int __cdecl Native_ButtonHistoryIgnoreController(lua_State* L) {
    void* bh = g_buttonHistory;
    ButtonHistory_IgnoreController(bh, Script::GetBool(L, 0));
    return 0;
}
NATIVE_A(ButtonHistoryIgnoreController, 0x005BD880);

// (line, n): the button and its flag (the game's locals aren't initialised)
int __cdecl Native_ButtonHistoryGetFromHistory(lua_State* L) {
    void* bh = g_buttonHistory;
    int button;
    int flag;
    int n = Script::GetInt(L, 1);
    ButtonHistory_GetFromHistory(bh, Script::GetInt(L, 0), n, &button, &flag);
    Script::PushInt(L, button);
    Script::PushBool(L, uint8_t(flag));
    return 2;
}
NATIVE_A(ButtonHistoryGetFromHistory, 0x005BD8B0);

int __cdecl Native_ButtonHistorySetCallbackCorrectButton(lua_State* L) {
    Script_StoreFunctionRefOpt(L, 0, ButtonHistory_CorrectButtonCallback(g_buttonHistory));
    return 0;
}
NATIVE_A(ButtonHistorySetCallbackCorrectButton, 0x005BD910);

int __cdecl Native_ButtonHistorySetCallbackPassed(lua_State* L) {
    Script_StoreFunctionRefOpt(L, 0, ButtonHistory_PassedCallback(g_buttonHistory));
    return 0;
}
NATIVE_A(ButtonHistorySetCallbackPassed, 0x005BD930);

int __cdecl Native_ButtonHistorySetCallbackFailed(lua_State* L) {
    Script_StoreFunctionRefOpt(L, 0, ButtonHistory_FailedCallback(g_buttonHistory));
    return 0;
}
NATIVE_A(ButtonHistorySetCallbackFailed, 0x005BD950);

int __cdecl Native_ButtonHistoryClearSequence(lua_State*) {
    ButtonHistory_ClearSequence(g_buttonHistory);
    return 0;
}
NATIVE_A(ButtonHistoryClearSequence, 0x005BD970);

// (seconds)
int __cdecl Native_ButtonHistorySetSequenceTime(lua_State* L) {
    void* bh = g_buttonHistory;
    ButtonHistory_SetSequenceTime(bh, SecondsToMs(Script::GetFloat(L, 0)));
    return 0;
}
NATIVE_A(ButtonHistorySetSequenceTime, 0x005BD980);

// (button, flag, button, flag, ...): 750 ms each
int __cdecl Native_ButtonHistoryAddSequence(lua_State* L) {
    int top = lua::gettop(L);
    void* bh = g_buttonHistory;
    for (int i = 0; i < top; i += 2) {
        int button = Script::GetInt(L, i);
        uint8_t flag = Script::GetBool(L, i + 1);
        ButtonHistory_AddToSequence(bh, button, flag, 750, -1, -1);
    }
    return 0;
}
NATIVE_A(ButtonHistoryAddSequence, 0x005BD9B0);

int __cdecl Native_ButtonHistoryAddChargeText(lua_State* L) {
    void* bh = g_buttonHistory;
    ButtonHistory_AddChargeText(bh, Script::GetInt(L, 0) - 1);
    return 0;
}
NATIVE_A(ButtonHistoryAddChargeText, 0x005BDA20);

// (button, flag, seconds, ...)
int __cdecl Native_ButtonHistoryAddSequenceTimeInterval(lua_State* L) {
    int top = lua::gettop(L);
    void* bh = g_buttonHistory;
    for (int i = 0; i < top; i += 3) {
        int button = Script::GetInt(L, i);
        uint8_t flag = Script::GetBool(L, i + 1);
        int ms = SecondsToMs(Script::GetFloat(L, i + 2));
        ButtonHistory_AddToSequence(bh, button, flag, ms, -1, -1);
    }
    return 0;
}
NATIVE_A(ButtonHistoryAddSequenceTimeInterval, 0x005BDA50);

// (button, seconds, seconds, seconds, ...)
int __cdecl Native_ButtonHistoryAddSequenceTimeIntervalCharge(lua_State* L) {
    int top = lua::gettop(L);
    void* bh = g_buttonHistory;
    for (int i = 2; i - 2 < top; i += 4) {
        int button = Script::GetInt(L, i - 2);
        int a = SecondsToMs(Script::GetFloat(L, i - 1));
        int b = SecondsToMs(Script::GetFloat(L, i));
        int c = SecondsToMs(Script::GetFloat(L, i + 1));
        ButtonHistory_AddToSequence(bh, button, 0, a, b, c);
    }
    return 0;
}
NATIVE_A(ButtonHistoryAddSequenceTimeIntervalCharge, 0x005BDAD0);

int __cdecl Native_ButtonHistoryIgnoreSequence(lua_State* L) {
    int top = lua::gettop(L);
    void* bh = g_buttonHistory;
    for (int i = 0; i < top; i++) {
        ButtonHistory_IgnoreInSequence(bh, Script::GetInt(L, i));
    }
    return 0;
}
NATIVE_A(ButtonHistoryIgnoreSequence, 0x005BDB80);

int __cdecl Native_ButtonHistoryEnableActionTreeInput(lua_State* L) {
    void* bh = g_buttonHistory;
    ButtonHistory_EnableActionTreeInput(bh, Script::GetBool(L, 0));
    return 0;
}
NATIVE_A(ButtonHistoryEnableActionTreeInput, 0x005BDBC0);

int __cdecl Native_ButtonHistoryAddSequenceLocalText(lua_State* L) {
    const char* text = Script::GetString(L, 0);
    ButtonHistory_AddSequenceLocalText(g_buttonHistory, text);
    return 0;
}
NATIVE_A(ButtonHistoryAddSequenceLocalText, 0x005BDBF0);

// ---- vibration and more HUD

namespace {
GAME_FN(void, __cdecl, Pad_StartVibration, 0x00738680, int pad, int a, uint8_t b, int c);
GAME_FN(void, __cdecl, Pad_StopVibration, 0x00738740, int pad);
GAME_FN(void, __cdecl, HUD_SaveVisibility, 0x005141D0, int which);
GAME_FN(void, __cdecl, HUD_RestoreVisibility, 0x00514210, int which);
GAME_VAR(void*, g_monitor, 0x00C6753C);
GAME_VAR(void*, g_screenFade, 0x00C6754C);
GAME_VAR(uint8_t*, g_objectiveTimer, 0x00C674AC);
GAME_VAR(void*, g_hudDiary, 0x00C67550);
GAME_VAR(void*, g_hudInventory, 0x00C67554);
GAME_VAR(void*, g_hudErrands, 0x00C67558);
GAME_VAR(uint8_t*, g_hudKeys, 0x00C67544);
GAME_VAR(uint8_t*, g_hudHealthBar, 0x00C674D0);
GAME_VAR(uint8_t, g_photographyColourUpgrade, 0x00C1B171);
GAME_VAR(uint8_t, g_photographySaveLevel1, 0x00C1B172);
GAME_VAR(uint8_t, g_photographySaveLevel2, 0x00C1B173);
GAME_VAR(uint8_t, g_hudFlashFromA14A50, 0x020C43FD);
GAME_VAR(int32_t, g_hudA14A50, 0x00A14A50);
GAME_VAR(int32_t, g_timeInMilliseconds, 0x00C1A9B4);
GAME_FN(void, __thiscall, Monitor_SetText, 0x00542FB0, void* monitor, uint8_t line, const char* text);
GAME_FN(void, __thiscall, Monitor_SetGreyed, 0x00542FE0, void* monitor, uint8_t line, uint8_t greyed);
GAME_FN(void, __thiscall, ScreenFade_SetValue, 0x00435A00, void* fade, int value, uint8_t flag);
GAME_FN(void, __cdecl, PushTxdMemoryTag, 0x0052E700);
GAME_FN(void, __cdecl, PopTxdMemoryTag, 0x0052E710);
GAME_FN(void, __cdecl, SplashScreen_Display, 0x004581D0, const char* texture, const char* mask);
GAME_FN(void, __cdecl, SplashScreen_Disable, 0x00458140);
GAME_FN(void, __thiscall, HUDInventory_DisplayChange, 0x00541110, void* inv, int a, int b, int c, int d);
GAME_FN(void, __thiscall, HUDErrands_Do, 0x00556C30, void* errands, uint8_t a, const char* text, uint8_t flag, float f);
GAME_FN(void, __thiscall, Radar_FlashAllObjectiveBlips, 0x0054C290, void* radar);
GAME_FN(void, __cdecl, HUD_SetEnableInputIgnoredDisplay, 0x0043A8D0, uint8_t on);
} // namespace

// (kind: 0 (a, b), 1 (a), 2 (a, b, looping) or 3 stop; a a 16-bit value, b a byte)
int __cdecl Native_StartVibration(lua_State* L) {
    int kind = Script::GetInt(L, 0);
    int a = uint16_t(Script::GetInt(L, 1));
    uint8_t b = uint8_t(Script::GetInt(L, 2));
    switch (kind) {
    case 0:
        Pad_StartVibration(0, a, b, 0);
        return 0;
    case 1:
        Pad_StartVibration(0, a, 0, 1);
        return 0;
    case 2:
        Pad_StartVibration(0, a, b, 1);
        return 0;
    case 3:
        Pad_StopVibration(0);
        break;
    }
    return 0;
}
NATIVE_A(StartVibration, 0x005BDC10);

int __cdecl Native_StopVibration(lua_State*) {
    Pad_StopVibration(0);
    return 0;
}
NATIVE_A(StopVibration, 0x005BDCC0);

int __cdecl Native_HUDSaveVisibility(lua_State*) {
    HUD_SaveVisibility(-1);
    return 0;
}
NATIVE_A(HUDSaveVisibility, 0x005BDCD0);

// (every component (48) hidden)
int __cdecl Native_HUDClearAllElements(lua_State*) {
    using SetVisibleFn = void(__thiscall*)(void*, uint8_t);
    for (int i = 0; i < 48; i++) {
        uint8_t* hud = g_hudComponents[i];
        re::GameVirtual<SetVisibleFn>(hud, 5)(hud, 0);
    }
    return 0;
}
NATIVE_A(HUDClearAllElements, 0x005BDCE0);

int __cdecl Native_HUDRestoreVisibility(lua_State*) {
    HUD_RestoreVisibility(-1);
    return 0;
}
NATIVE_A(HUDRestoreVisibility, 0x005BDD00);

int __cdecl Native_RadarLockOnFloor(lua_State* L) {
    int floor = Script::GetInt(L, 0);
    uint8_t* radar = g_radar;
    At<int>(radar, 0xE7FC) = floor;
    radar[0xE810] = 1;
    return 0;
}
NATIVE_A(RadarLockOnFloor, 0x005BDD10);

// (x, y, z, w)
int __cdecl Native_RadarLockOnPoint(lua_State* L) {
    float x = Script::GetFloat(L, 0);
    float y = Script::GetFloat(L, 1);
    float z = Script::GetFloat(L, 2);
    float w = Script::GetFloat(L, 3);
    uint8_t* radar = g_radar;
    At<float>(radar, 0xE80C) = w;
    At<float>(radar, 0xE800) = x;
    At<float>(radar, 0xE804) = y;
    radar[0xE811] = 1;
    At<float>(radar, 0xE808) = z;
    return 0;
}
NATIVE_A(RadarLockOnPoint, 0x005BDD40);

int __cdecl Native_RadarReleaseLock(lua_State*) {
    uint8_t* radar = g_radar;
    radar[0xE810] = 0;
    radar[0xE811] = 0;
    return 0;
}
NATIVE_A(RadarReleaseLock, 0x005BDDC0);

int __cdecl Native_RadarHidePlayerBlip(lua_State* L) {
    uint8_t* radar = g_radar;
    radar[0xE812] = Script::GetBool(L, 0);
    return 0;
}
NATIVE_A(RadarHidePlayerBlip, 0x005BDDE0);

int __cdecl Native_MonitorSetText(lua_State* L) {
    uint8_t line = uint8_t(Script::GetInt(L, 0));
    const char* text = Script::GetString(L, 1);
    Monitor_SetText(g_monitor, line, text);
    return 0;
}
NATIVE_A(MonitorSetText, 0x005BDE00);

int __cdecl Native_MonitorSetGreyed(lua_State* L) {
    uint8_t line = uint8_t(Script::GetInt(L, 0));
    uint8_t greyed = Script::GetBool(L, 1);
    Monitor_SetGreyed(g_monitor, line, greyed);
    return 0;
}
NATIVE_A(MonitorSetGreyed, 0x005BDE40);

int __cdecl Native_SetScreenFadeValue(lua_State* L) {
    int value = Script::GetInt(L, 0);
    uint8_t flag = Script::GetBool(L, 1);
    ScreenFade_SetValue(g_screenFade, value, flag);
    return 0;
}
NATIVE_A(SetScreenFadeValue, 0x005BDE80);

int __cdecl Native_SplashScreenDisplay(lua_State* L) {
    const char* texture = Script::GetString(L, 0);
    const char* mask = Script::GetString(L, 1);
    PushTxdMemoryTag();
    SplashScreen_Display(texture, mask);
    PopTxdMemoryTag();
    return 0;
}
NATIVE_A(SplashScreenDisplay, 0x005BDEC0);

int __cdecl Native_SplashScreenDisable(lua_State*) {
    SplashScreen_Disable();
    return 0;
}
NATIVE_A(SplashScreenDisable, 0x005BDF00);

int __cdecl Native_ObjTimerSetText(lua_State* L) {
    uint8_t* timer = g_objectiveTimer;
    game_strncpy(reinterpret_cast<char*>(timer + 0x44), Script::GetString(L, 0), 0x20);
    return 0;
}
NATIVE_A(ObjTimerSetText, 0x005BDF10);

int __cdecl Native_HUDDiaryVisible(lua_State* L) {
    void* diary = g_hudDiary;
    using SetVisibleFn = void(__thiscall*)(void*, uint8_t);
    SetVisibleFn setVisible = re::GameVirtual<SetVisibleFn>(diary, 5);
    setVisible(diary, Script::GetBool(L, 0));
    return 0;
}
NATIVE_A(HUDDiaryVisible, 0x005BDF40);

int __cdecl Native_HUDInventoryDisplayChange(lua_State* L) {
    void* inv = g_hudInventory;
    int c = Script::GetInt(L, 2);
    int b = Script::GetInt(L, 1);
    int a = Script::GetInt(L, 0);
    HUDInventory_DisplayChange(inv, a, b, c, 0);
    return 0;
}
NATIVE_A(HUDInventoryDisplayChange, 0x005BDF70);

// ([a = false], [text = nullptr], [f = 0]): the flag says there were three arguments
int __cdecl Native_DoSocialErrands(lua_State* L) {
    uint8_t a = Script::GetBoolOpt(L, 0, 0);
    const char* text = Script::GetStringOpt(L, 1, nullptr);
    float f = Script::GetFloatOpt(L, 2, 0.0f);
    uint8_t three = 0;
    if (lua::gettop(L) == 3) {
        three = 1;
    }
    void* errands = g_hudErrands;
    using SetVisibleFn = void(__thiscall*)(void*, uint8_t);
    re::GameVirtual<SetVisibleFn>(errands, 5)(errands, 1);
    HUDErrands_Do(errands, a, text, three, f);
    return 0;
}
NATIVE_A(DoSocialErrands, 0x005BDFB0);

int __cdecl Native_HUDSetNumGlobeKeys(lua_State* L) {
    uint8_t* keys = g_hudKeys;
    At<int>(keys, 0x114) = Script::GetInt(L, 0);
    return 0;
}
NATIVE_A(HUDSetNumGlobeKeys, 0x005BE040);

int __cdecl Native_HUDSetNumFortuneTellerKeys(lua_State* L) {
    uint8_t* keys = g_hudKeys;
    At<int>(keys, 0x104) = Script::GetInt(L, 0);
    return 0;
}
NATIVE_A(HUDSetNumFortuneTellerKeys, 0x005BE060);

int __cdecl Native_HUDFlashAllObjectiveBlips(lua_State*) {
    Radar_FlashAllObjectiveBlips(g_radar);
    return 0;
}
NATIVE_A(HUDFlashAllObjectiveBlips, 0x005BE080);

int __cdecl Native_HUDPhotographySetColourUpgrade(lua_State* L) {
    g_photographyColourUpgrade = Script::GetBool(L, 0);
    return 0;
}
NATIVE_A(HUDPhotographySetColourUpgrade, 0x005BE090);

int __cdecl Native_HUDPhotographySetSaveLevel1(lua_State* L) {
    g_photographySaveLevel1 = Script::GetBool(L, 0);
    return 0;
}
NATIVE_A(HUDPhotographySetSaveLevel1, 0x005BE0B0);

int __cdecl Native_HUDPhotographySetSaveLevel2(lua_State* L) {
    g_photographySaveLevel2 = Script::GetBool(L, 0);
    return 0;
}
NATIVE_A(HUDPhotographySetSaveLevel2, 0x005BE0D0);

// ([on = true]) into the health bar's +0xBB
int __cdecl Native_HUDSetFightStyle(lua_State* L) {
    if (Script::GetArgCount(L) >= 1) {
        uint8_t on = Script::GetBool(L, 0);
        g_hudHealthBar[0xBB] = on;
        return 0;
    }
    g_hudHealthBar[0xBB] = 1;
    return 0;
}
NATIVE_A(HUDSetFightStyle, 0x005BE0F0);

int __cdecl Native_HUDSetEnableInputIgnoredDisplay(lua_State* L) {
    HUD_SetEnableInputIgnoredDisplay(Script::GetBool(L, 0));
    return 0;
}
NATIVE_A(HUDSetEnableInputIgnoredDisplay, 0x005BE140);

// (component, flashing): +0x14, and +0x18 when it started
int __cdecl Native_ToggleHUDComponentFlashing(lua_State* L) {
    int component = Script::GetInt(L, 0);
    uint8_t flashing = Script::GetBool(L, 1);
    uint8_t* hud = g_hudComponents[component];
    hud[0x14] = flashing;
    if (flashing) {
        if (g_hudFlashFromA14A50 != 0) {
            At<int>(hud, 0x18) = g_hudA14A50;
        } else {
            At<int>(hud, 0x18) = g_timeInMilliseconds;
        }
    } else {
        At<int>(hud, 0x18) = 0;
    }
    if (component == 13 && flashing) {
        HUD_542810(g_hudC674D4);
    }
    return 0;
}
NATIVE_A(ToggleHUDComponentFlashing, 0x005BE170);

// ---- the player's items and weapons

namespace {
GAME_VAR(int32_t, g_firstWeapon, 0x00A136B0);
GAME_VAR(int32_t, g_lastWeapon, 0x00A136B4);
GAME_VAR(int32_t, g_firstItem, 0x00A136B8);
GAME_VAR(int32_t, g_lastItem, 0x00A136BC);
GAME_VAR(void*, g_hudWeapons, 0x00C674F0);
GAME_FN(void, __thiscall, WeaponInventory_GiveItem, 0x0045D030, void* inv, int item, int n, int a, int b);
GAME_FN(void, __thiscall, WeaponInventory_GiveWeapon, 0x0045CE00, void* inv, int weapon, uint8_t equip, int a);
GAME_FN(void, __thiscall, WeaponInventory_GiveAmmo, 0x0045CED0, void* inv, int weapon, int n, uint8_t a, int b);
GAME_FN(int, __thiscall, WeaponInventory_GetCount, 0x0045CF40, void* inv, int weapon);
GAME_FN(void, __thiscall, WeaponInventory_SetCount, 0x0045D240, void* inv, int weapon, int n, int a);
GAME_FN(void, __cdecl, Items_Give, 0x00541C30, int item, int n, int a, int b);
GAME_FN(uint8_t, __cdecl, Items_Has, 0x00541E90, int item);
GAME_FN(void, __cdecl, Items_Remove, 0x00541E40, int item);
GAME_FN(int, __cdecl, Items_GetCount, 0x00541E00, int item);
GAME_FN(void, __cdecl, Items_SetCount, 0x00541D60, int item, int n);
GAME_FN(void, __thiscall, HUDWeapons_55C840, 0x0055C840, void* hud, int a);
GAME_FN(void, __thiscall, HUDWeapons_Unequip, 0x0055C8D0, void* hud);

bool IsWeapon(int item) {
    return item >= g_firstWeapon && item <= g_lastWeapon;
}
bool IsItem(int item) {
    return item >= g_firstItem && item <= g_lastItem;
}
void* PlayerWeaponInventory() {
    return At<void*>(g_playerPed, 0x1C4);
}
int16_t PlayerCurrentWeapon() {
    return At<int16_t>(At<uint8_t*>(g_playerPed, 0x1D0), 0x10E);
}
} // namespace

// (item, [n = 1])
int __cdecl Native_GiveItemToPlayer(lua_State* L) {
    int item = Script::GetInt(L, 0);
    int n = 1;
    if (lua::gettop(L) == 2) {
        n = Script::GetInt(L, 1);
    }
    if (IsWeapon(item)) {
        WeaponInventory_GiveItem(PlayerWeaponInventory(), item, n, 1, 0);
        return 0;
    }
    if (IsItem(item)) {
        Items_Give(item, n, 1, 0);
    }
    return 0;
}
NATIVE_A(GiveItemToPlayer, 0x005BE1E0);

int __cdecl Native_GiveWeaponToPlayer(lua_State* L) {
    int weapon = Script::GetInt(L, 0);
    uint8_t equip = 1;
    if (lua::gettop(L) >= 2) {
        equip = Script::GetBool(L, 1);
    }
    WeaponInventory_GiveWeapon(PlayerWeaponInventory(), weapon, equip, 0);
    return 0;
}
NATIVE_A(GiveWeaponToPlayer, 0x005BE260);

int __cdecl Native_GiveAmmoToPlayer(lua_State* L) {
    int weapon = Script::GetInt(L, 0);
    int n = Script::GetInt(L, 1);
    uint8_t a = 1;
    if (lua::gettop(L) >= 3) {
        a = Script::GetBool(L, 2);
    }
    WeaponInventory_GiveAmmo(PlayerWeaponInventory(), weapon, n, a, 0);
    return 0;
}
NATIVE_A(GiveAmmoToPlayer, 0x005BE2C0);

int __cdecl Native_PlayerHasItem(lua_State* L) {
    int item = Script::GetInt(L, 0);
    uint8_t has = 0;
    if (IsWeapon(item)) {
        has = WeaponInventory_GetCount(PlayerWeaponInventory(), item) != 0;
    } else if (IsItem(item)) {
        has = Items_Has(item);
    }
    Script::PushBool(L, has);
    return 1;
}
NATIVE_A(PlayerHasItem, 0x005BE320);

// (a weapon in hand is put away first)
int __cdecl Native_RemovePlayerItem(lua_State* L) {
    int item = Script::GetInt(L, 0);
    if (IsWeapon(item)) {
        uint8_t* player = g_playerPed;
        if (At<int16_t>(At<uint8_t*>(player, 0x1D0), 0x10E) == item) {
            void* hud = g_hudWeapons;
            HUDWeapons_55C840(hud, -1);
            HUDWeapons_Unequip(hud);
        }
        WeaponInventory_SetCount(At<void*>(player, 0x1C4), item, 0, 1);
        return 0;
    }
    if (IsItem(item)) {
        Items_Remove(item);
    }
    return 0;
}
NATIVE_A(RemovePlayerItem, 0x005BE3A0);

int __cdecl Native_ItemGetCurrentNum(lua_State* L) {
    int item = Script::GetInt(L, 0);
    int n = 0;
    if (IsWeapon(item)) {
        Script::PushInt(L, WeaponInventory_GetCount(PlayerWeaponInventory(), item));
        return 1;
    }
    if (IsItem(item)) {
        n = Items_GetCount(item);
    }
    Script::PushInt(L, n);
    return 1;
}
NATIVE_A(ItemGetCurrentNum, 0x005BE430);

int __cdecl Native_ItemSetCurrentNum(lua_State* L) {
    int item = Script::GetInt(L, 0);
    int n = Script::GetInt(L, 1);
    if (IsWeapon(item)) {
        WeaponInventory_SetCount(PlayerWeaponInventory(), item, n, 1);
        return 0;
    }
    if (IsItem(item)) {
        Items_SetCount(item, n);
    }
    return 0;
}
NATIVE_A(ItemSetCurrentNum, 0x005BE4A0);

int __cdecl Native_IsItemAWeapon(lua_State* L) {
    int item = Script::GetInt(L, 0);
    uint8_t weapon = 0;
    if (IsWeapon(item)) {
        weapon = 1;
    }
    Script::PushBool(L, weapon);
    return 1;
}
NATIVE_A(IsItemAWeapon, 0x005BE510);

int __cdecl Native_PlayerUnequip(lua_State*) {
    HUDWeapons_Unequip(g_hudWeapons);
    return 0;
}
NATIVE_A(PlayerUnequip, 0x005BE560);

// ([weapon]): that one in hand, or any
int __cdecl Native_WeaponEquipped(lua_State* L) {
    if (lua::gettop(L) == 1) {
        int weapon = Script::GetInt(L, 0);
        Script::PushBool(L, PlayerCurrentWeapon() == weapon);
        return 1;
    }
    Script::PushBool(L, PlayerCurrentWeapon() != -1);
    return 1;
}
NATIVE_A(WeaponEquipped, 0x005BE570);

// ---- threads, input, scripts

namespace {
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int button, int pad);
GAME_FN(uint8_t, __cdecl, Pad_IsBeingPressed, 0x00738900, int button, int pad);
GAME_FN(uint8_t, __cdecl, Pad_IsBeingReleased, 0x00738940, int button, int pad);
GAME_FN(void, __cdecl, Script_Import, 0x005D8C40, lua_State* L, const char* name);
GAME_FN(void, __thiscall, LurStore_Launch, 0x005DC0F0, void* store, const char* name, int a);
} // namespace

// (returns lua_yield's result)
int __cdecl Native_ThreadYield(lua_State* L) {
    return lua_yield(L, 0);
}
NATIVE_A(ThreadYield, 0x005BE5E0);

// (button, pad): its value isn't 0 (NaN counts as pressed)
int __cdecl Native_IsButtonPressed(lua_State* L) {
    int button = Script::GetInt(L, 0);
    int pad = Script::GetInt(L, 1);
    uint8_t pressed = 0;
    if (!(double(Pad_GetValue(button, pad)) == 0.0)) {
        pressed = 1;
    }
    Script::PushBool(L, pressed);
    return 1;
}
NATIVE_A(IsButtonPressed, 0x005BE5F0);

int __cdecl Native_IsButtonBeingPressed(lua_State* L) {
    int button = Script::GetInt(L, 0);
    int pad = 0;
    if (lua::gettop(L) >= 2) {
        pad = Script::GetInt(L, 1);
    }
    Script::PushBool(L, Pad_IsBeingPressed(button, pad));
    return 1;
}
NATIVE_A(IsButtonBeingPressed, 0x005BE650);

int __cdecl Native_IsButtonBeingReleased(lua_State* L) {
    int button = Script::GetInt(L, 0);
    int pad = 0;
    if (lua::gettop(L) >= 2) {
        pad = Script::GetInt(L, 1);
    }
    Script::PushBool(L, Pad_IsBeingReleased(button, pad));
    return 1;
}
NATIVE_A(IsButtonBeingReleased, 0x005BE6B0);

int __cdecl Native_GetStickValue(lua_State* L) {
    int pad = Script::GetInt(L, 1);
    int stick = Script::GetInt(L, 0);
    Script::PushFloat(L, Pad_GetValue(stick, pad));
    return 1;
}
NATIVE_A(GetStickValue, 0x005BE710);

// (returns 1: what the import left on the stack)
int __cdecl Native_ImportScript(lua_State* L) {
    Script_Import(L, Script::GetString(L, 0));
    return 1;
}
NATIVE_A(ImportScript, 0x005BE750);

int __cdecl Native_LaunchScript(lua_State* L) {
    LurStore_Launch(TheLurStore, Script::GetString(L, 0), 1);
    return 0;
}
NATIVE_A(LaunchScript, 0x005BE770);

// ---- restart points (KO, arrest, asleep)

namespace {
inline void* const g_restartPoints = reinterpret_cast<void*>(0x00BD1078);
GAME_ARRAY(float, g_defaultRestartCameraPos, 0x00AE9F68);   // 3 floats

// (point, heading, area, a, b, camera position, camera heading, c)
using AddRestartPointFn = void(__thiscall*)(void* points, float* point, float heading, int area, int a, int b, float* cameraPos, float cameraHeading, int c);
// (interior, point, heading, area, a, b, camera position, camera heading, c)
using AddInteriorRestartPointFn = void(__thiscall*)(void* points, int interior, float* point, float heading, int area, int a, int b, float* cameraPos, float cameraHeading, int c);
GAME_FN(void, __thiscall, RestartPoints_AddKO, 0x00430840, void* points, float* point, float heading, int area, int a, int b, float* cameraPos, float cameraHeading, int c);
GAME_FN(void, __thiscall, RestartPoints_AddArrest, 0x00430AD0, void* points, float* point, float heading, int area, int a, int b, float* cameraPos, float cameraHeading, int c);
GAME_FN(void, __thiscall, RestartPoints_SetDefaultKO, 0x00431140, void* points, float* point, float heading, int area, int a, int b, float* cameraPos, float cameraHeading, int c);
GAME_FN(void, __thiscall, RestartPoints_SetDefaultArrest, 0x004311F0, void* points, float* point, float heading, int area, int a, int b, float* cameraPos, float cameraHeading, int c);
GAME_FN(void, __thiscall, RestartPoints_SetDefaultAsleep, 0x00431290, void* points, float* point, float heading, int area, int a, int b, float* cameraPos, float cameraHeading, int c);
GAME_FN(void, __thiscall, RestartPoints_AddInteriorKO, 0x00430920, void* points, int interior, float* point, float heading, int area, int a, int b, float* cameraPos, float cameraHeading, int c);
GAME_FN(void, __thiscall, RestartPoints_AddInteriorArrest, 0x00430E20, void* points, int interior, float* point, float heading, int area, int a, int b, float* cameraPos, float cameraHeading, int c);
GAME_FN(void, __thiscall, RestartPoints_AddInteriorAsleep, 0x00431030, void* points, int interior, float* point, float heading, int area, int a, int b, float* cameraPos, float cameraHeading, int c);
GAME_FN(void, __thiscall, RestartPoints_RemoveKO, 0x00430A00, void* points, float* point);
GAME_FN(void, __thiscall, RestartPoints_RemoveArrest, 0x00430F30, void* points, float* point);
GAME_FN(void, __thiscall, RestartPoints_SetKOCameraPos, 0x00430BE0, void* points, float* point, float* camera);
GAME_FN(void, __thiscall, RestartPoints_SetArrestCameraPos, 0x00430C70, void* points, float* point, float* camera);
GAME_FN(void, __thiscall, RestartPoints_SetAsleepCameraPos, 0x00430D00, void* points, float* point, float* camera);
GAME_FN(void, __thiscall, RestartPoints_SetDefaultKOCameraPos, 0x00430D90, void* points, float* camera);
GAME_FN(void, __thiscall, RestartPoints_SetDefaultArrestCameraPos, 0x00430DC0, void* points, float* camera);
GAME_FN(void, __thiscall, RestartPoints_SetDefaultAsleepCameraPos, 0x00430DF0, void* points, float* camera);

float* RoutePoint(int slot) {
    return Route_First(Routes_Get(g_routes, slot));
}

// headings are the route point's (+0x1C) less 90
float RouteHeading(void* route) {
    return float(double(At<float>(Route_First(route), 0x1C)) - 90.0);
}

// (route, area, [a, b], [camera route, c]): 2, 4 or 6 arguments. (A route's first point
// is got twice, as in the game)
void AddRestartPoint(lua_State* L, AddRestartPointFn add) {
    int area = Script::GetInt(L, 1);
    int a = 0;
    int c = -1;
    int b = 24;
    void* route = Routes_Get(g_routes, Script::GetInt(L, 0));
    float* point = Route_First(route);
    float heading = RouteHeading(route);
    float cameraPos[3] = {g_defaultRestartCameraPos[0], g_defaultRestartCameraPos[1], g_defaultRestartCameraPos[2]};
    float cameraHeading = 0.0f;
    if (lua::gettop(L) == 4 || lua::gettop(L) == 6) {
        a = Script::GetInt(L, 2);
        b = Script::GetInt(L, 3);
    }
    if (lua::gettop(L) == 6) {
        void* cameraRoute = Routes_Get(g_routes, Script::GetInt(L, 4));
        float* p = Route_First(cameraRoute);
        cameraPos[0] = p[0];
        cameraPos[1] = p[1];
        cameraPos[2] = p[2];
        cameraHeading = RouteHeading(cameraRoute);
        c = Script::GetInt(L, 5);
    }
    add(g_restartPoints, point, heading, area, a, b, cameraPos, cameraHeading, c);
}

// (interior, route, area, [a, b], [camera route, c]): 3, 5 or 7 arguments. (The
// camera route's slot is read again for c: argument 5 both times)
void AddInteriorRestartPoint(lua_State* L, AddInteriorRestartPointFn add) {
    int interior = Script::GetInt(L, 0);
    int area = Script::GetInt(L, 2);
    int c = -1;
    int a = 0;
    int b = 24;
    void* route = Routes_Get(g_routes, Script::GetInt(L, 1));
    float* point = Route_First(route);
    float heading = RouteHeading(route);
    float cameraPos[3] = {g_defaultRestartCameraPos[0], g_defaultRestartCameraPos[1], g_defaultRestartCameraPos[2]};
    float cameraHeading = 0.0f;
    if (lua::gettop(L) == 5 || lua::gettop(L) == 7) {
        a = Script::GetInt(L, 3);
        b = Script::GetInt(L, 4);
    }
    if (lua::gettop(L) == 7) {
        void* cameraRoute = Routes_Get(g_routes, Script::GetInt(L, 5));
        float* p = Route_First(cameraRoute);
        cameraPos[0] = p[0];
        cameraPos[1] = p[1];
        cameraPos[2] = p[2];
        cameraHeading = RouteHeading(cameraRoute);
        c = Script::GetInt(L, 5);
    }
    add(g_restartPoints, interior, point, heading, area, a, b, cameraPos, cameraHeading, c);
}
} // namespace

int __cdecl Native_AddKORestartPoint(lua_State* L) {
    AddRestartPoint(L, RestartPoints_AddKO);
    return 0;
}
NATIVE_A(AddKORestartPoint, 0x005BE790);

int __cdecl Native_AddInteriorKORestartPoint(lua_State* L) {
    AddInteriorRestartPoint(L, RestartPoints_AddInteriorKO);
    return 0;
}
NATIVE_A(AddInteriorKORestartPoint, 0x005BE8E0);

int __cdecl Native_RemoveKORestartPoint(lua_State* L) {
    float* point = RoutePoint(Script::GetInt(L, 0));
    RestartPoints_RemoveKO(g_restartPoints, point);
    return 0;
}
NATIVE_A(RemoveKORestartPoint, 0x005BEA40);

int __cdecl Native_SetKORestartPointCameraPos(lua_State* L) {
    float* point = RoutePoint(Script::GetInt(L, 0));
    float* camera = RoutePoint(Script::GetInt(L, 1));
    RestartPoints_SetKOCameraPos(g_restartPoints, point, camera);
    return 0;
}
NATIVE_A(SetKORestartPointCameraPos, 0x005BEA70);

int __cdecl Native_SetArrestRestartPointCameraPos(lua_State* L) {
    float* point = RoutePoint(Script::GetInt(L, 0));
    float* camera = RoutePoint(Script::GetInt(L, 1));
    RestartPoints_SetArrestCameraPos(g_restartPoints, point, camera);
    return 0;
}
NATIVE_A(SetArrestRestartPointCameraPos, 0x005BEAD0);

int __cdecl Native_SetAsleepRestartPointCameraPos(lua_State* L) {
    float* point = RoutePoint(Script::GetInt(L, 0));
    float* camera = RoutePoint(Script::GetInt(L, 1));
    RestartPoints_SetAsleepCameraPos(g_restartPoints, point, camera);
    return 0;
}
NATIVE_A(SetAsleepRestartPointCameraPos, 0x005BEB30);

int __cdecl Native_SetDefaultKORestartCameraPos(lua_State* L) {
    RestartPoints_SetDefaultKOCameraPos(g_restartPoints, RoutePoint(Script::GetInt(L, 0)));
    return 0;
}
NATIVE_A(SetDefaultKORestartCameraPos, 0x005BEB90);

int __cdecl Native_SetDefaultArrestRestartCameraPos(lua_State* L) {
    RestartPoints_SetDefaultArrestCameraPos(g_restartPoints, RoutePoint(Script::GetInt(L, 0)));
    return 0;
}
NATIVE_A(SetDefaultArrestRestartCameraPos, 0x005BEBC0);

int __cdecl Native_SetDefaultAsleepRestartCameraPos(lua_State* L) {
    RestartPoints_SetDefaultAsleepCameraPos(g_restartPoints, RoutePoint(Script::GetInt(L, 0)));
    return 0;
}
NATIVE_A(SetDefaultAsleepRestartCameraPos, 0x005BEBF0);

int __cdecl Native_AddArrestRestartPoint(lua_State* L) {
    AddRestartPoint(L, RestartPoints_AddArrest);
    return 0;
}
NATIVE_A(AddArrestRestartPoint, 0x005BEC20);

int __cdecl Native_AddInteriorArrestRestartPoint(lua_State* L) {
    AddInteriorRestartPoint(L, RestartPoints_AddInteriorArrest);
    return 0;
}
NATIVE_A(AddInteriorArrestRestartPoint, 0x005BED70);

int __cdecl Native_RemoveArrestRestartPoint(lua_State* L) {
    float* point = RoutePoint(Script::GetInt(L, 0));
    RestartPoints_RemoveArrest(g_restartPoints, point);
    return 0;
}
NATIVE_A(RemoveArrestRestartPoint, 0x005BEED0);

int __cdecl Native_AddInteriorAsleepRestartPoint(lua_State* L) {
    AddInteriorRestartPoint(L, RestartPoints_AddInteriorAsleep);
    return 0;
}
NATIVE_A(AddInteriorAsleepRestartPoint, 0x005BEF00);

int __cdecl Native_SetDefaultKOPoint(lua_State* L) {
    AddRestartPoint(L, RestartPoints_SetDefaultKO);
    return 0;
}
NATIVE_A(SetDefaultKOPoint, 0x005BF060);

int __cdecl Native_SetDefaultArrestPoint(lua_State* L) {
    AddRestartPoint(L, RestartPoints_SetDefaultArrest);
    return 0;
}
NATIVE_A(SetDefaultArrestPoint, 0x005BF1B0);

int __cdecl Native_SetDefaultAsleepPoint(lua_State* L) {
    AddRestartPoint(L, RestartPoints_SetDefaultAsleep);
    return 0;
}
NATIVE_A(SetDefaultAsleepPoint, 0x005BF300);

// ---- script threads, DAT files, streaming, yearbook, missions

namespace {
inline void* const g_dat = reinterpret_cast<void*>(0x020C7C48);   // (the trigger pool's owner)
inline void* const g_actionTrees = reinterpret_cast<void*>(0x00C0F7B4);
inline void* const g_bustedCallback = reinterpret_cast<void*>(0x00BD3E68);
inline void* const g_yearbook = reinterpret_cast<void*>(0x00C1AEA8);   // (the player info)
GAME_VAR(void*, g_yearbookHud, 0x00C1AA04);
GAME_VAR(void*, g_streamingContents, 0x00BD0FC4);
GAME_VAR(uint8_t, g_stopPedProduction, 0x00C2C10C);
inline const char* const g_emptyString8FEFBB = reinterpret_cast<const char*>(0x008FEFBB);
GAME_FN(uint8_t*, __thiscall, LuaScript_CreateThread, 0x005D9E80, void* script, const char* function);
GAME_FN(void, __thiscall, LuaScript_TerminateThread, 0x005D9F80, void* script, int thread);
GAME_FN(void, __thiscall, LuaScript_Terminate, 0x005D9E40, void* script, int a);
GAME_FN(uint8_t*, __thiscall, LuaScript_GetCurrentThread, 0x005D8B90, void* script);
GAME_FN(void, __cdecl, Command_43C4F0, 0x0043C4F0, const char* name, uintptr_t a, uintptr_t b);
GAME_FN(void, __thiscall, DAT_Load, 0x006D3C70, void* dat, const char* name, int a);
GAME_FN(void, __thiscall, DAT_Unload, 0x006D2210, void* dat, int a);
GAME_FN(void, __thiscall, DAT_Init, 0x006D2350, void* dat);
GAME_FN(void, __cdecl, StreamingContents_531930, 0x00531930, void* contents);
GAME_FN(void, __cdecl, StreamingContents_532330, 0x00532330, void* contents);
GAME_FN(void, __cdecl, EmptyFunction_49A650, 0x0049A650, void* p);   // (just a ret)
GAME_FN(void, __cdecl, RemoveUnusedModelsInLoadedList, 0x00530B50);
GAME_FN(void, __cdecl, Streaming_530A70, 0x00530A70, int a);
GAME_FN(int, __thiscall, ActionTrees_Find, 0x004507B0, void* trees, const char* name);
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int id, int flags);
GAME_FN(uint8_t*, __cdecl, GetStreamingInfo, 0x0052DD30, int id);   // (+4: the load state, 1 loaded)
GAME_FN(uint8_t*, __thiscall, LurStore_LaunchScript, 0x005DC0F0, void* store, const char* name, int a);
GAME_FN(int, __thiscall, LurStore_FindStreamed, 0x005DAFF0, void* store, const char* name);
GAME_FN(void*, __thiscall, LurStore_GetStreamedScript, 0x005DBDF0, void* store, int index);
GAME_FN(int, __cdecl, AnimGroups_GetStreamingId, 0x0052DAD0, const char* name);
GAME_FN(void, __thiscall, PlayerInfo_SetYearbookBits, 0x004466D0, void* info, int which, int bits);
GAME_FN(uint32_t, __thiscall, PlayerInfo_GetYearbookBits, 0x004466F0, void* info, int which);
GAME_FN(void, __cdecl, Yearbook_GetPictureBits, 0x006A2250, int picture, int* a, int* b, int* c);
GAME_FN(void, __thiscall, YearbookHud_Set, 0x0045BE70, void* hud, int a, int b, int c);
GAME_FN(uint8_t, __cdecl, Yearbook_IsPictureModel, 0x006A1350, int model);
GAME_FN(uint8_t, __thiscall, Ped_IsValidYearbookPhoto, 0x0049B680, void* player, void* ped, void* out);
GAME_FN(void, __thiscall, Ped_GetGirlfriendIndex, 0x0049BBD0, void* player, int model, int* out);
GAME_FN(void, __thiscall, YearbookHud_RegisterGirlfriend, 0x0045BEF0, void* hud, int index);
GAME_FN(uint8_t, __thiscall, Missions_IsActive, 0x006AA7E0, void* missions);
GAME_FN(int*, __thiscall, Missions_GetCurrent, 0x006AA680, void* missions);
GAME_FN(int*, __thiscall, Missions_GetCurrent2, 0x006AA690, void* missions);
GAME_FN(uint8_t, __thiscall, Mission_IsRunning, 0x006AA3F0, void* mission);
GAME_FN(uint8_t*, __thiscall, Missions_Get, 0x006AA660, void* missions, int index);

void SetLaunchedScriptPOI(uint8_t* script) {
    At<uint32_t>(g_poiManager, 0x14C) = HashName(reinterpret_cast<const char*>(script + 4));
}

// (the name's mission is the current one, as 0x6AA680 or 0x6AA690 has it)
uint8_t IsMissionActive(lua_State* L, int*(__thiscall* getCurrent)(void*)) {
    uint32_t key;
    NameKey_Construct(&key, Script::GetString(L, 0));
    int index = MissionNames_Find(g_missionNames, &key);
    uint8_t active = Mission_IsRunning(getCurrent(g_missionNames)) && *getCurrent(g_missionNames) == index;
    Script::PushBool(L, active);
    NameKey_Destruct(&key);
    return active;
}
} // namespace

// (function name): a thread of the running script; pushes its id (+0x20)
int __cdecl Native_CreateThread(lua_State* L) {
    const char* function = lua::tostring(L, 1);
    uint8_t* thread = LuaScript_CreateThread(LurStore_GetCurrentScript(TheLurStore), function);
    thread[0x14] = 1;
    Script::PushInt(L, At<int>(thread, 0x20));
    return 1;
}
NATIVE_A(CreateThread, 0x005BF450);

int __cdecl Native_TerminateThread(lua_State* L) {
    if (!Script::IsNil(L, 0)) {
        int thread = Script::GetInt(L, 0);
        LuaScript_TerminateThread(LurStore_GetCurrentScript(TheLurStore), thread);
    }
    return 0;
}
NATIVE_A(TerminateThread, 0x005BF490);

int __cdecl Native_TerminateCurrentScript(lua_State*) {
    LuaScript_Terminate(LurStore_GetCurrentScript(TheLurStore), 0);
    return 0;
}
NATIVE_A(TerminateCurrentScript, 0x005BF4D0);

// (name, [a = 2])
int __cdecl Native_DATLoad(lua_State* L) {
    int a = 2;
    if (lua::gettop(L) == 2) {
        a = Script::GetInt(L, 1);
    }
    Command_43C4F0("cmdDATLoad", 0, 0);
    DAT_Load(g_dat, Script::GetString(L, 0), a);
    return 0;
}
NATIVE_A(DATLoad, 0x005BF4F0);

int __cdecl Native_DATUnload(lua_State* L) {
    DAT_Unload(g_dat, Script::GetInt(L, 0));
    return 0;
}
NATIVE_A(DATUnload, 0x005BF550);

int __cdecl Native_DATInit(lua_State*) {
    DAT_Init(g_dat);
    return 0;
}
NATIVE_A(DATInit, 0x005BF570);

int __cdecl Native_FlushStreamingContents(lua_State*) {
    StreamingContents_531930(g_streamingContents);
    StreamingContents_532330(g_streamingContents);
    EmptyFunction_49A650(g_streamingContents);
    RemoveUnusedModelsInLoadedList();
    Streaming_530A70(0);
    return 0;
}
NATIVE_A(FlushStreamingContents, 0x005BF580);

// (name): pushes its index (-1 none), requesting it (streaming id 0x5D41 + index)
int __cdecl Native_RequestActionTree(lua_State* L) {
    int index = ActionTrees_Find(g_actionTrees, Script::GetString(L, 0));
    if (index != -1) {
        RequestModel(index + 0x5D41, 0x13);
    }
    Script::PushInt(L, index);
    return 1;
}
NATIVE_A(RequestActionTree, 0x005BF5C0);

int __cdecl Native_IsActionTreeLoaded(lua_State* L) {
    int index = Script::GetIntOpt(L, 0, -1);
    if (index != -1) {
        Script::PushBool(L, GetStreamingInfo(index + 0x5D41)[4] == 1);
        return 1;
    }
    Script::PushBool(L, 0);
    return 1;
}
NATIVE_A(IsActionTreeLoaded, 0x005BF610);

// (the POIs' +0x14C: the launched script's name hash)
int __cdecl Native_LaunchScenarioScript(lua_State* L) {
    uint8_t* script = LurStore_LaunchScript(TheLurStore, Script::GetString(L, 0), 1);
    if (script != nullptr) {
        SetLaunchedScriptPOI(script);
    }
    return 0;
}
NATIVE_A(LaunchScenarioScript, 0x005BF660);

// (streaming id 0x5DB9 + index)
int __cdecl Native_StreamedScriptRequest(lua_State* L) {
    int index = LurStore_FindStreamed(TheLurStore, Script::GetString(L, 0));
    if (index >= 0) {
        RequestModel(index + 0x5DB9, 0x1E);
    }
    return 0;
}
NATIVE_A(StreamedScriptRequest, 0x005BF6A0);

// (no check for an unknown one: -1 + 0x5DB9)
int __cdecl Native_StreamedScriptHasLoaded(lua_State* L) {
    int index = LurStore_FindStreamed(TheLurStore, Script::GetString(L, 0));
    Script::PushBool(L, GetStreamingInfo(index + 0x5DB9)[4] == 1);
    return 1;
}
NATIVE_A(StreamedScriptHasLoaded, 0x005BF6E0);

// (name, [scenario = false])
int __cdecl Native_StreamedScriptLaunch(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    LurStore_FindStreamed(TheLurStore, name);
    uint8_t* script = LurStore_LaunchScript(TheLurStore, name, 1);
    uint8_t scenario = Script::GetBoolOpt(L, 1, 0);
    if (script != nullptr && scenario) {
        SetLaunchedScriptPOI(script);
    }
    return 0;
}
NATIVE_A(StreamedScriptLaunch, 0x005BF730);

int __cdecl Native_StreamedScriptTerminate(lua_State* L) {
    int index = LurStore_FindStreamed(TheLurStore, Script::GetString(L, 0));
    void* script = LurStore_GetStreamedScript(TheLurStore, index);
    if (script != nullptr) {
        LuaScript_Terminate(script, 0);
    }
    return 0;
}
NATIVE_A(StreamedScriptTerminate, 0x005BF790);

int __cdecl Native_RegisterCallbackOnBusted(lua_State* L) {
    Script_StoreFunctionRefOpt(L, 0, g_bustedCallback);
    return 0;
}
NATIVE_A(RegisterCallbackOnBusted, 0x005BF7D0);

// ([name]: the game's empty string at 0x8FEFBB by default)
int __cdecl Native_RequestAnimGroup(lua_State* L) {
    int id = AnimGroups_GetStreamingId(Script::GetStringOpt(L, 0, g_emptyString8FEFBB));
    if (GetStreamingInfo(id)[4] != 2 && GetStreamingInfo(id)[4] != 1) {
        RequestModel(id, 0);
    }
    return 0;
}
NATIVE_A(RequestAnimGroup, 0x005BF7F0);

int __cdecl Native_HasAnimGroupLoaded(lua_State* L) {
    int id = AnimGroups_GetStreamingId(Script::GetStringOpt(L, 0, g_emptyString8FEFBB));
    Script::PushBool(L, GetStreamingInfo(id)[4] == 1);
    return 1;
}
NATIVE_A(HasAnimGroupLoaded, 0x005BF840);

int __cdecl Native_RegisterCallbackOnYearbookPhoto(lua_State* L) {
    Script_StoreFunctionRefOpt(L, 0, g_playerPed + 0x1F00);
    return 0;
}
NATIVE_A(RegisterCallbackOnYearbookPhoto, 0x005BF880);

int __cdecl Native_ResetYearbookPhotos(lua_State*) {
    PlayerInfo_SetYearbookBits(g_yearbook, 0x14, 0);
    PlayerInfo_SetYearbookBits(g_yearbook, 0x15, 0);
    PlayerInfo_SetYearbookBits(g_yearbook, 0x16, 0);
    int a = 0;
    int b = 0;
    int c = 0;
    Yearbook_GetPictureBits(0, &a, &b, &c);
    YearbookHud_Set(g_yearbookHud, a, b, c);
    return 0;
}
NATIVE_A(ResetYearbookPhotos, 0x005BF8A0);

// (all 61 picture bits set; returns 1 or 0 with nothing pushed)
int __cdecl Native_YearbookIsFull(lua_State*) {
    uint32_t bits0 = PlayerInfo_GetYearbookBits(g_yearbook, 0x14);
    uint32_t bits1 = PlayerInfo_GetYearbookBits(g_yearbook, 0x15);
    uint32_t bits2 = PlayerInfo_GetYearbookBits(g_yearbook, 0x16);
    uint32_t full0 = 0;
    uint32_t full1 = 0;
    uint32_t full2 = 0;
    for (int i = 0; i < 61; i++) {
        if (i < 32) {
            full0 |= 1u << i;
        } else if (i < 64) {
            full1 |= 1u << (i - 32);
        } else if (i < 96) {
            full2 |= 1u << (i - 64);
        }
    }
    if (bits0 == full0 && bits1 == full1 && bits2 == full2) {
        return 1;
    }
    return 0;
}
NATIVE_A(YearbookIsFull, 0x005BF920);

int __cdecl Native_IsValidYearbookPhotoOfPed(lua_State* L) {
    uint8_t* ped = static_cast<uint8_t*>(GetPedFromHandle(Script::GetInt(L, 0), 2));
    float out = 0.0f;
    uint8_t valid = 0;
    if (ped != nullptr && Yearbook_IsPictureModel(At<int16_t>(ped, 0x10E)) && Ped_IsValidYearbookPhoto(g_playerPed, ped, &out)) {
        valid = 1;
    }
    Script::PushBool(L, valid);
    return 1;
}
NATIVE_A(IsValidYearbookPhotoOfPed, 0x005BF9D0);

int __cdecl Native_DisablePunishmentSystem(lua_State* L) {
    uint8_t* punishment = static_cast<uint8_t*>(g_punishment);
    uint8_t disable = Script::GetBool(L, 0);
    punishment[0] = disable;
    punishment[1] = disable;
    return 0;
}
NATIVE_A(DisablePunishmentSystem, 0x005BFA50);

int __cdecl Native_PunishersRespondToPlayerOnly(lua_State* L) {
    uint8_t* punishment = static_cast<uint8_t*>(g_punishment);
    punishment[1] = Script::GetBool(L, 0);
    return 0;
}
NATIVE_A(PunishersRespondToPlayerOnly, 0x005BFA70);

int __cdecl Native_RegisterAsGirlfriend(lua_State* L) {
    uint8_t* ped = static_cast<uint8_t*>(GetPedFromHandle(Script::GetInt(L, 0), 2));
    int index = 0;
    if (ped != nullptr) {
        Ped_GetGirlfriendIndex(g_playerPed, At<int16_t>(ped, 0x10E), &index);
        YearbookHud_RegisterGirlfriend(g_yearbookHud, index);
    }
    return 0;
}
NATIVE_A(RegisterAsGirlfriend, 0x005BFA90);

int __cdecl Native_UnlockYearbookPicture(lua_State* L) {
    int a = 0;
    int b = 0;
    int c = 0;
    int picture = Script::GetInt(L, 0);
    if (Yearbook_IsPictureModel(picture)) {
        Yearbook_GetPictureBits(picture, &a, &b, &c);
        YearbookHud_Set(g_yearbookHud, a, b, c);
    }
    return 0;
}
NATIVE_A(UnlockYearbookPicture, 0x005BFAF0);

int __cdecl Native_StopPedProduction(lua_State* L) {
    g_stopPedProduction = Script::GetBool(L, 0);
    return 0;
}
NATIVE_A(StopPedProduction, 0x005BFB60);

// (+0x7898 bit 0)
int __cdecl Native_SetPopulationFastCulling(lua_State* L) {
    void* pop = g_population;
    uint8_t on = Script::GetBool(L, 0);
    uint8_t& flags = At<uint8_t>(pop, 0x7898);
    flags = uint8_t((flags & ~1) | (on & 1));
    return 0;
}
NATIVE_A(SetPopulationFastCulling, 0x005BFB80);

int __cdecl Native_GetTimer(lua_State* L) {
    Script::PushInt(L, g_timeInMilliseconds);
    return 1;
}
NATIVE_A(GetTimer, 0x005BFBB0);

// (ms): the running thread's wake time (+0x1C, unless -1), then yields (its result)
int __cdecl Native_Wait(lua_State* L) {
    int ms = Script::GetInt(L, 0);
    uint8_t* thread = LuaScript_GetCurrentThread(LurStore_GetCurrentScript(TheLurStore));
    if (At<int>(thread, 0x1C) != -1) {
        At<int>(thread, 0x1C) = g_timeInMilliseconds + ms;
    }
    return lua_yield(L, 0);
}
NATIVE_A(Wait, 0x005BFBD0);

int __cdecl Native_DistanceBetweenCoords3d(lua_State* L) {
    float a[3];
    float b[3];
    a[0] = Script::GetFloat(L, 0);
    a[1] = Script::GetFloat(L, 1);
    a[2] = Script::GetFloat(L, 2);
    b[0] = Script::GetFloat(L, 3);
    b[1] = Script::GetFloat(L, 4);
    b[2] = Script::GetFloat(L, 5);
    float d[3];
    VectorSubtract(d, a, b);
    float square = float(double(d[1]) * d[1] + double(d[0]) * d[0] + double(d[2]) * d[2]);
    Script::PushFloat(L, float(std::sqrt(double(std::fabs(square)))));
    return 1;
}
NATIVE_A(DistanceBetweenCoords3d, 0x005BFC20);

// (x1, y1, x2, y2)
int __cdecl Native_DistanceBetweenCoords2d(lua_State* L) {
    float x1 = Script::GetFloat(L, 0);
    float y1 = Script::GetFloat(L, 1);
    float x2 = Script::GetFloat(L, 2);
    float y2 = Script::GetFloat(L, 3);
    float dx = x1 - x2;
    float dy = y1 - y2;
    float square = float(double(dx) * dx + double(dy) * dy);
    Script::PushFloat(L, float(std::sqrt(double(std::fabs(square)))));
    return 1;
}
NATIVE_A(DistanceBetweenCoords2d, 0x005BFD70);

int __cdecl Native_MissionActive(lua_State* L) {
    Script::PushBool(L, Missions_IsActive(g_missionNames));
    return 1;
}
NATIVE_A(MissionActive, 0x005BFE60);

int __cdecl Native_MissionActiveSpecific(lua_State* L) {
    IsMissionActive(L, Missions_GetCurrent);
    return 1;
}
NATIVE_A(MissionActiveSpecific, 0x005BFE80);

int __cdecl Native_MissionActiveSpecific2(lua_State* L) {
    IsMissionActive(L, Missions_GetCurrent2);
    return 1;
}
NATIVE_A(MissionActiveSpecific2, 0x005BFF00);

// (name): nothing pushed (and 0 returned) for no name
int __cdecl Native_MissionGetIndex(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    if (name == nullptr) {
        return 0;
    }
    uint32_t key;
    NameKey_Construct(&key, name);
    int index = MissionNames_Find(g_missionNames, &key);
    NameKey_Destruct(&key);
    Script::PushInt(L, index);
    return 1;
}
NATIVE_A(MissionGetIndex, 0x005BFF80);

int __cdecl Native_MissionGetName(lua_State* L) {
    uint8_t* mission = Missions_Get(g_missionNames, Script::GetInt(L, 0));
    Script::PushString(L, At<const char*>(mission, 0x18));
    return 1;
}
NATIVE_A(MissionGetName, 0x005BFFD0);

// ---- (back to the camera)

namespace {
// GTA's CColPoint, as far as it's set here (0x28 bytes on the game's stack)
struct ScriptColPoint {
    float point[3];        // +0
    re::Ptr32<void> entity;          // +0xC (a registered reference, cleaned up after)
    float normal[3];       // +0x10
    uint8_t unknown1C[8];  // +0x1C (not set)
    float depth;           // +0x24 (FLT_MAX)
};
static_assert(sizeof(ScriptColPoint) == 0x28);
GAME_FN(int, __thiscall, Camera_IsSphereVisible, 0x004F13D0, void* camera, ScriptVector* centre, float radius);
GAME_FN(uint8_t, __cdecl, ProcessLineOfSight_461EA0, 0x00461EA0, ScriptVector* start, ScriptVector* end, ScriptColPoint* colPoint, void* out, int a, int b, int c);
} // namespace

// (unused, route): the route's first point is in view (a sphere of 2) and a line from
// the camera to it hits something. (The game's two identical branches on the hit entity
// being 0xC2A8E8's are one here)
int __cdecl Native_CameraIsPointVisible(lua_State* L) {
    float* p = Route_First(Routes_Get(g_routes, Script::GetInt(L, 1)));
    ScriptVector camera = {p[0], p[1], p[2]};
    ScriptVector point = {camera.x, camera.y, camera.z};
    if (Camera_IsSphereVisible(TheCamera, &point, 2.0f) != 0) {
        float* ltm = RwFrameGetLTM(RwCameraGetFrame(g_rwCamera));
        camera.x = ltm[9];
        camera.y = ltm[10];
        camera.z = ltm[11];
        ScriptColPoint colPoint;
        colPoint.point[0] = 0.0f;
        colPoint.point[1] = 0.0f;
        colPoint.point[2] = 0.0f;
        colPoint.entity = nullptr;
        colPoint.normal[0] = 0.0f;
        colPoint.normal[1] = 0.0f;
        colPoint.normal[2] = 0.0f;
        colPoint.depth = 3.402823466e+38f;
        int out;   // (not initialised in the game)
        if (ProcessLineOfSight_461EA0(&point, &camera, &colPoint, &out, 1, 0, 0)) {
            Script::PushBool(L, 1);
            if (colPoint.entity != nullptr) {
                Entity_CleanUpOldReference(colPoint.entity, &colPoint.entity);
            }
            return 1;
        }
        if (colPoint.entity != nullptr) {
            Entity_CleanUpOldReference(colPoint.entity, &colPoint.entity);
        }
    }
    Script::PushBool(L, 0);
    return 1;
}
NATIVE_A(CameraIsPointVisible, 0x005BA1E0);
