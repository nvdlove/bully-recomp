// Script natives for pickups, the player, POIs and shops (0x5D0CF0..0x5D3320). Each is
// `int __cdecl Native_X(lua_State* L)`, returning how many results it pushed; registered
// under its script name (lua_natives.csv).

#include <cmath>
#include <cstdint>
#include <cstring>

#include "game/ScriptNatives.h"
#include "reversed.h"

namespace {

// ---- pickups ------------------------------------------------------------------------------

// The pickups (0xC0A100, 0x48 bytes each: +8 the position, +0x18 the script owning it,
// +0x30 the model, +0x44 1 while it's there); the ones FindPickupsInRange found
inline uint8_t* const snlPickups = reinterpret_cast<uint8_t*>(0x00C0A100);
GAME_VAR(uint32_t, snlNumPickupsInRange, 0x00C0A0A8);
GAME_ARRAY(int32_t, snlPickupsInRange, 0x00C0A0B0);
inline uint8_t* snlPickup(int index) {
    return snlPickups + index * 0x48;
}

GAME_FN(int, __cdecl, snlPickups_GetActualIndex, 0x004438B0, int handle);
GAME_FN(int32_t, __cdecl, snlPickups_GenerateFacing, 0x00443FE0, float x, float y, float z, int model, int flag, void* butes,
        void* entity, int spread);
GAME_FN(int32_t, __cdecl, snlPickups_GenerateForEntity, 0x00443EE0, uint8_t* entity, int model, void* butes);
GAME_FN(void, __cdecl, snlPickups_FindInRange, 0x004440F0, const SnVec* pos, float radius, int sort);
GAME_FN(void, __cdecl, snlPickups_Remove, 0x004454F0, int handle, int);
GAME_FN(void, __thiscall, snlPickup_Collect, 0x00444A10, void* pickup, int deleteObject);
// 0x5D0C80: the butes of a name under the pickups' group
GAME_FN(void*, __cdecl, snlFindGroupButes, 0x005D0C80, const uint32_t* name);
GAME_FN(void*, __thiscall, snlNameKey_Construct, 0x005FA910, uint32_t* self, const char* name);
GAME_FN(void, __thiscall, snlNameKey_Destruct, 0x0049A650, uint32_t* self);   // (empty)
// The running script keeps a pickup / a model / drops a pickup
GAME_FN(void, __thiscall, snlScript_AddPickup, 0x005D98C0, void* script, int handle);
GAME_FN(void, __thiscall, snlScript_AddModel, 0x005D9450, void* script, int model);
GAME_FN(void, __thiscall, snlScript_RemovePickup, 0x005D9900, void* script, int handle, int);
GAME_FN(uint8_t*, __cdecl, snlGetStreamingInfo, 0x0052DD30, int model);
GAME_FN(void, __cdecl, snlRequestModel, 0x00532020, int model, int flags);
// The routes (0xBC5E7C)
inline void* const snlRoutes = reinterpret_cast<void*>(0x00BC5E7C);
GAME_FN(uint8_t*, __thiscall, snlRoutes_Get, 0x006D6450, void* routes, int id);
GAME_FN(uint8_t*, __thiscall, snlRoute_First, 0x006D59B0, void* route);
GAME_FN(uint8_t*, __thiscall, snlRoute_Next, 0x006D59F0, void* route);
GAME_FN(SnVec*, __cdecl, snlAngleToVector, 0x00432E30, SnVec* out, float angle);
// the population's "area clear of peds" point (0xBCC564)
inline SnVec* const snlAreaClearPoint = reinterpret_cast<SnVec*>(0x00BCC564);

// The butes named by argument i
void* snlButesArg(lua_State* L, int i) {
    uint32_t key;
    snlNameKey_Construct(&key, Script::GetString(L, i));
    void* butes = snlFindGroupButes(&key);
    snlNameKey_Destruct(&key);
    return butes;
}

// model, x, y, z[, butes]
int __cdecl Native_PickupCreateXYZ(lua_State* L) {
    int model = Script::GetInt(L, 0);
    float x = Script::GetFloat(L, 1);
    float y = Script::GetFloat(L, 2);
    float z = Script::GetFloat(L, 3);
    void* butes = nullptr;
    if (lua::gettop(L) == 5) {
        butes = snlButesArg(L, 4);
    }
    int32_t handle = snlPickups_GenerateFacing(x, y, z, model, 1, butes, nullptr, 0);
    snlScript_AddPickup(snCurrentScript(snLurStore), handle);
    Script::PushInt(L, handle);
    return 1;
}

int __cdecl Native_PickupGetXYZ(lua_State* L) {
    int index = snlPickups_GetActualIndex(Script::GetInt(L, 0));
    if (index != -1) {
        Script::PushVector(L, reinterpret_cast<ScriptVector*>(snlPickup(index) + 8));
        return 3;
    }
    Script::PushVector(L, reinterpret_cast<ScriptVector*>(snlAreaClearPoint));
    return 3;
}

// model, ped[, butes]
int __cdecl Native_PickupCreateFromPed(lua_State* L) {
    int model = Script::GetInt(L, 0);
    uint8_t* ped = snPedArg(L, 1);
    void* butes = nullptr;
    if (lua::gettop(L) == 3) {
        butes = snlButesArg(L, 2);
    }
    int32_t handle = snlPickups_GenerateForEntity(ped, model, butes);
    snlScript_AddPickup(snCurrentScript(snLurStore), handle);
    Script::PushInt(L, handle);
    return 1;
}

int __cdecl Native_PickupDelete(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    int index = snlPickups_GetActualIndex(handle);
    if (index >= 0) {
        uint8_t* pickup = snlPickup(index);
        if (snAt<void*>(pickup, 0x18) != nullptr) {
            snlScript_RemovePickup(snAt<void*>(pickup, 0x18), handle, 1);
        }
        snlPickups_Remove(handle, 1);
    }
    return 0;
}

// (reads its arguments and answers nothing)
int __cdecl Native_PickupIsInAreaXYZ(lua_State* L) {
    Script::GetInt(L, 0);
    Script::GetFloat(L, 1);
    Script::GetFloat(L, 2);
    Script::GetFloat(L, 3);
    Script::GetFloat(L, 4);
    Script::GetInt(L, 5);
    return 0;
}

// model, x, y, z, radius: a pickup of the model in range
int __cdecl Native_PickupFindInArea(lua_State* L) {
    int model = Script::GetInt(L, 0);
    SnVec pos;
    pos.x = Script::GetFloat(L, 1);
    pos.y = Script::GetFloat(L, 2);
    pos.z = Script::GetFloat(L, 3);
    float radius = Script::GetFloat(L, 4);
    snlPickups_FindInRange(&pos, radius, 0);
    for (uint32_t i = 0; i < snlNumPickupsInRange; i++) {
        if (snAt<int32_t>(snlPickup(snlPickupsInRange[i]), 0x30) == model) {
            Script::PushBool(L, 1);
            return 1;
        }
    }
    Script::PushBool(L, 0);
    return 1;
}

// Loaded (true), or being loaded or requested now (false; the running script keeps it).
// The model is taken as 16 bits
int __cdecl Native_PickupRequestModel(lua_State* L) {
    int model = uint16_t(Script::GetInt(L, 0));
    if (snlGetStreamingInfo(model)[4] == 1) {
        Script::PushBool(L, 1);
        return 1;
    }
    if (snlGetStreamingInfo(model)[4] != 2 && snlGetStreamingInfo(model)[4] != 3 && snlGetStreamingInfo(model)[4] != 4) {
        snlRequestModel(model, 0x16);
        snlScript_AddModel(snCurrentScript(snLurStore), model);
    }
    Script::PushBool(L, 0);
    return 1;
}

// (true for an unknown handle)
int __cdecl Native_PickupIsPickedUp(lua_State* L) {
    int index = snlPickups_GetActualIndex(Script::GetInt(L, 0));
    if (index != -1 && snlPickup(index)[0x44] == 1) {
        Script::PushBool(L, 0);
        return 1;
    }
    Script::PushBool(L, 1);
    return 1;
}

// model, route[, point index, spread (360), butes]: facing the point's heading, turned
// by up to half the spread (the game hands the heading's direction vector over as the
// entity whose heading is used)
int __cdecl Native_PickupCreatePoint(lua_State* L) {
    int spread = 360;
    void* butes = nullptr;
    int index = 0; // (only read with the argument)
    bool hasIndex = false;
    if (lua::gettop(L) >= 3) {
        hasIndex = true;
        index = Script::GetInt(L, 2);
    }
    if (lua::gettop(L) >= 4) {
        spread = Script::GetInt(L, 3);
    }
    if (lua::gettop(L) == 5) {
        butes = snlButesArg(L, 4);
    }
    int model = uint16_t(Script::GetInt(L, 0));
    uint8_t* route = snlRoutes_Get(snlRoutes, uint16_t(Script::GetInt(L, 1)));
    uint8_t* point = snlRoute_First(route);
    if (hasIndex) {
        for (int i = 1; i < index; i++) {
            uint8_t* next = snlRoute_Next(route);
            if (next == nullptr) {
                break;
            }
            point = next;
        }
    }
    float x = snAt<float>(point, 0);
    float y = snAt<float>(point, 4);
    float z = snAt<float>(point, 8);
    SnVec direction;
    snlAngleToVector(&direction, snAt<float>(point, 0x1C));
    int32_t handle = snlPickups_GenerateFacing(x, y, z, model, 0, butes, &direction, spread);
    snlScript_AddPickup(snCurrentScript(snLurStore), handle);
    Script::PushInt(L, handle);
    return 1;
}

// x, y, z, radius[, model (-1: any)]
int __cdecl Native_PickupDestroyTypeInAreaXYZ(lua_State* L) {
    SnVec pos;
    pos.x = Script::GetFloat(L, 0);
    pos.y = Script::GetFloat(L, 1);
    pos.z = Script::GetFloat(L, 2);
    float radius = Script::GetFloat(L, 3);
    int model = -1;
    if (lua::gettop(L) > 4) {
        model = Script::GetInt(L, 4);
    }
    snlPickups_FindInRange(&pos, radius, 0);
    for (uint32_t i = 0; i < snlNumPickupsInRange; i++) {
        uint8_t* pickup = snlPickup(snlPickupsInRange[i]);
        if (model == -1 || snAt<int32_t>(pickup, 0x30) == model) {
            snlPickup_Collect(pickup, 1);
        }
    }
    return 0;
}

// ---- the player ---------------------------------------------------------------------------

// The area transitions (0xBCC580): a transition to a point, with the player's heading
inline void* const snlAreaTransitions = reinterpret_cast<void*>(0x00BCC580);
GAME_FN(void, __thiscall, snlAreaTransitions_RequestHere, 0x00416100, void* manager, const SnVec* pos);
// (an object whose +0x8DC says a scripted move is under way)
GAME_VAR(uint8_t*, snlObj9E4F4C, 0x009E4F4C);

int __cdecl Native_PlayerSetPosXYZ(lua_State* L) {
    SnVec pos;
    pos.x = Script::GetFloat(L, 0);
    pos.y = Script::GetFloat(L, 1);
    pos.z = Script::GetFloat(L, 2);
    snAt<uint8_t>(snlObj9E4F4C, 0x8DC) = 1;
    snlAreaTransitions_RequestHere(snlAreaTransitions, &pos);
    return 0;
}

GAME_FN(void, __thiscall, snlPlayerInfo_SetPaused, 0x00445DA0, void* info, int paused);
// The user input (0xA45A30)
inline void* const snlUserInput = reinterpret_cast<void*>(0x00A45A30);
GAME_FN(void, __thiscall, snlUserInput_SetEnabled0, 0x0050DFA0, void* input, int enabled);
GAME_FN(void, __thiscall, snlUserInput_SetEnabled3, 0x0050DFD0, void* input, int enabled);
GAME_FN(void, __thiscall, snlPedIntelligence_Fn5F3A40, 0x005F3A40, void* intelligence);
GAME_FN(void, __thiscall, snlPed_UpdateMoveDirection, 0x00474570, void* ped);
GAME_FN(void, __thiscall, snlActionController_Stop, 0x005F40A0, void* controller);

// 0: control off (paused, the input off, the intelligence (+0x2E0) reset and its +0..8
// copied to +0xC..0x14); 1: on
int __cdecl Native_PlayerSetControl(lua_State* L) {
    int on = Script::GetInt(L, 0);
    if (on == 0) {
        snlPlayerInfo_SetPaused(snPlayerInfo, 1);
        snlUserInput_SetEnabled0(snlUserInput, 0);
        snlPedIntelligence_Fn5F3A40(snAt<void*>(snPlayerPed, 0x2E0));
        snlPed_UpdateMoveDirection(snPlayerPed);
        uint8_t* intelligence = snAt<uint8_t*>(snPlayerPed, 0x2E0);
        snAt<int32_t>(intelligence, 0xC) = snAt<int32_t>(intelligence, 0);
        snAt<int32_t>(intelligence, 0x10) = snAt<int32_t>(intelligence, 4);
        snAt<int32_t>(intelligence, 0x14) = snAt<int32_t>(intelligence, 8);
        return 0;
    }
    if (on == 1) {
        snAt<int32_t>(snPlayerPed, 0x1370) = 0;
        snlPlayerInfo_SetPaused(snPlayerInfo, 0);
        snlUserInput_SetEnabled0(snlUserInput, 1);
        snlUserInput_SetEnabled3(snlUserInput, 1);
    }
    return 0;
}

// The three action controllers of the player's intelligence (+0x290, +0x258, +0x2C8)
int __cdecl Native_PlayerStopAllActionControllers(lua_State*) {
    snlActionController_Stop(snAt<uint8_t*>(snPlayerPed, 0x2E0) + 0x290);
    snlActionController_Stop(snAt<uint8_t*>(snPlayerPed, 0x2E0) + 0x258);
    snlActionController_Stop(snAt<uint8_t*>(snPlayerPed, 0x2E0) + 0x2C8);
    return 0;
}

// 0x5C2A70: an entity (by handle and kind, or as below) in a box or sphere
GAME_FN(uint8_t, __cdecl, snlIsEntityInArea, 0x005C2A70, int is3D, int handle, int kind, float x, float y, float z, float rx,
        float ry, float rz, int blip, const SnVec* rotation, int blip2, int a13, int a14, int a15, int a16);
GAME_FN(void*, __cdecl, snlGetEntityFromHandle, 0x005C28D0, int handle, int kind, int pedItself, int cutscene);
GAME_FN(SnVec*, __cdecl, snlGetHandlePosition, 0x005C2970, SnVec* out, int handle, int kind);

// x, y, z, radius, blip[, heading (degrees)]
int __cdecl Native_PlayerIsInAreaXYZ(lua_State* L) {
    float x = Script::GetFloat(L, 0);
    float y = Script::GetFloat(L, 1);
    float z = Script::GetFloat(L, 2);
    float r = Script::GetFloat(L, 3);
    int blip = Script::GetInt(L, 4);
    float heading = 0.0f;
    if (Script::GetArgCount(L) >= 6) {
        heading = float(double(Script::GetFloat(L, 5)) * double(3.1415927f) / 180.0);
    }
    SnVec rotation = {0.0f, 0.0f, heading};
    Script::PushBool(L, snlIsEntityInArea(1, 0, 3, x, y, z, r, r, r, blip, &rotation, -1, -1, 0, 0, 1));
    return 1;
}

// handle, kind, radius, blip[, blip2, flag, heading]: around the entity
int __cdecl Native_PlayerIsInAreaObject(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    int kind = Script::GetInt(L, 1);
    float r = Script::GetFloat(L, 2);
    int blip = Script::GetInt(L, 3);
    uint8_t inArea = 0;
    if (snlGetEntityFromHandle(handle, kind, 0, 0) != nullptr) {
        SnVec pos;
        snlGetHandlePosition(&pos, handle, kind);
        int blip2 = -1;
        uint8_t flag = 0;
        if (Script::GetArgCount(L) >= 5) {
            blip2 = Script::GetInt(L, 4);
        }
        if (Script::GetArgCount(L) >= 6) {
            flag = Script::GetBool(L, 5);
        }
        float heading = 0.0f;
        if (Script::GetArgCount(L) >= 7) {
            heading = float(double(Script::GetFloat(L, 6)) * double(3.1415927f) / 180.0);
        }
        SnVec rotation = {0.0f, 0.0f, heading};
        // (the flag goes as a dword whose upper bytes the game leaves uninitialised)
        inArea = snlIsEntityInArea(1, 0, 3, pos.x, pos.y, pos.z, r, r, r, blip, &rotation, blip2, -1, flag, 0, 1);
    }
    Script::PushBool(L, inArea);
    return 1;
}

GAME_FN(uint8_t*, __cdecl, snlGetVehicle, 0x0044A580, int handle);

// (the handle taken as 16 bits)
int __cdecl Native_PlayerIsInVehicle(lua_State* L) {
    int handle = uint16_t(Script::GetInt(L, 0));
    uint8_t inVehicle = 0;
    uint8_t* player = snGetPed(0, SN_HANDLE_PLAYER);
    if (snAt<void*>(player, 0x1554) != nullptr) {
        inVehicle = snAt<uint8_t*>(player, 0x1554) == snlGetVehicle(handle);
    }
    Script::PushBool(L, inVehicle);
    return 1;
}

GAME_FN(void, __thiscall, snlVehicleSeats_RemoveDriver, 0x004CE960, void* seats);
// The cutscene (0xB9DAB8; +0x64 running) forgets a ped
inline void* const snlCutscene = reinterpret_cast<void*>(0x00B9DAB8);
GAME_VAR(uint8_t, snlCutsceneRunning, 0x00B9DB1C);
GAME_FN(void, __thiscall, snlCutscene_ForgetPed, 0x00683EF0, void* cutscene, uint8_t* ped);

int __cdecl Native_PlayerDetachFromVehicle(lua_State*) {
    uint8_t* player = snGetPed(0, SN_HANDLE_PLAYER);
    if (snAt<void*>(player, 0x1554) != nullptr) {
        snlActionController_Stop(snAt<uint8_t*>(player, 0x2E0) + 0x258);
        snlActionController_Stop(snAt<uint8_t*>(player, 0x2E0) + 0x290);
        snlVehicleSeats_RemoveDriver(snAt<uint8_t*>(player, 0x1554) + 0x1C4);
        if (snlCutsceneRunning) {
            snlCutscene_ForgetPed(snlCutscene, player);
        }
    }
    return 0;
}

GAME_FN(int, __thiscall, snlInventory_Find, 0x0045CF40, void* inventory, int weapon);
GAME_FN(void, __thiscall, snlInventory_Give, 0x0045D030, void* inventory, int weapon, int ammo, int flag, int);
GAME_FN(void, __thiscall, snlInventory_SetCount, 0x0045D240, void* inventory, int weapon, int ammo, int flag);
// The weapons HUD (0xC674F0) selects one
GAME_VAR(void*, snlWeaponsHud, 0x00C674F0);
GAME_FN(void, __thiscall, snlWeaponsHud_Select, 0x0055C840, void* hud, int weapon);

// weapon, ammo (16 bits)[, flag (true)]: given or its count set, then selected
int __cdecl Native_PlayerSetWeapon(lua_State* L) {
    uint8_t flag = 1; // (a byte the game passes as a dword with uninitialised upper bytes)
    if (lua::gettop(L) >= 3) {
        flag = Script::GetBool(L, 2);
    }
    int weapon = Script::GetInt(L, 0);
    int ammo = uint16_t(Script::GetInt(L, 1));
    uint8_t* player = snPlayerPed;
    if (snlInventory_Find(snAt<void*>(player, 0x1C4), weapon) == 0) {
        snlInventory_Give(snAt<void*>(player, 0x1C4), weapon, ammo, flag, 0);
    } else {
        snlInventory_SetCount(snAt<void*>(player, 0x1C4), weapon, ammo, flag);
    }
    snAt<int32_t>(snAt<uint8_t*>(player, 0x2E0), 0x18) = weapon;
    snlWeaponsHud_Select(snlWeaponsHud, weapon);
    return 0;
}

GAME_FN(void*, __cdecl, snlGetModelInfo, 0x0051C190, const char* name, int32_t* index);

// The player's model by name (the ped's virtual +0x20, with 1)
int __cdecl Native_PlayerSwapModel(lua_State* L) {
    char name[256];
    strcpy(name, Script::GetString(L, 0));
    uint8_t* player = snGetPed(0, SN_HANDLE_PLAYER);
    int32_t index = -1;
    snlGetModelInfo(name, &index);
    snVCall<void>(player, 0x20, index, 1);
    return 0;
}

GAME_FN(void, __thiscall, snlPed_TakeMoney, 0x00473100, void* ped, int amount);
GAME_FN(void, __thiscall, snlPed_AddMoney, 0x00475320, void* ped, int amount);

// (the second argument is read and ignored)
int __cdecl Native_PlayerAddMoney(lua_State* L) {
    int amount = Script::GetInt(L, 0);
    if (lua::gettop(L) >= 2) {
        Script::GetBool(L, 1);
    }
    if (amount < 0) {
        snlPed_TakeMoney(snPlayerPed, -amount);
        return 0;
    }
    snlPed_AddMoney(snPlayerPed, amount);
    return 0;
}

// The button lock (0xC1AF8C on, 0xC1AF8D.. 25 buttons still allowed; the buttons come
// from the script unchecked, and 4 and 5 are always allowed)
GAME_VAR(uint8_t, snlButtonLock, 0x00C1AF8C);
GAME_ARRAY(uint8_t, snlButtonLockAllowed, 0x00C1AF8D);

int __cdecl Native_PlayerLockButtonInputsExcept(lua_State* L) {
    snlButtonLock = Script::GetBool(L, 0);
    if (snlButtonLock != 0) {
        memset(snlButtonLockAllowed, 0, 25);
        for (int i = 1; i < lua::gettop(L); i++) {
            snlButtonLockAllowed[Script::GetInt(L, i)] = 1;
        }
        snlButtonLockAllowed[4] = 1;
        snlButtonLockAllowed[5] = 1;
    }
    return 0;
}

// The unlocks (0xC1AA04)
GAME_VAR(void*, snlUnlocks, 0x00C1AA04);
GAME_FN(uint8_t, __thiscall, snlUnlocks_IsUnlocked, 0x0045BF20, void* unlocks, const char* name);

// (the name is copied into 24 bytes, unchecked)
int __cdecl Native_PlayerHasUnlockedUpgrade(lua_State* L) {
    char name[24];
    strcpy(name, Script::GetString(L, 0));
    Script::PushBool(L, snlUnlocks_IsUnlocked(snlUnlocks, name));
    return 1;
}

// The social action overrides: 128-byte action paths from 0x910950, by social action
// 0x17 on
inline const char (*const snlSocialOverridePaths)[0x80] = reinterpret_cast<const char (*)[0x80]>(0x00910950);
GAME_FN(void*, __cdecl, snlActionTree_Find, 0x005F5B10, const char* path, uintptr_t);
// 0x5C6190: the action file reference's type (the node's virtual +4 tests for it)
GAME_FN(int, __cdecl, snlActionFileReference_Type, 0x005C6190);
GAME_FN(void, __thiscall, snlActionFileReference_Fn5F6670, 0x005F6670, void* node);
GAME_FN(void, __thiscall, snlActionFileReference_SetFile, 0x005F56B0, void* node, const char* file);
GAME_FN(void, __thiscall, snlActionFileReference_Fn5F6CE0, 0x005F6CE0, void* node);
GAME_FN(void, __thiscall, snlScript_AddSocialOverride, 0x005D92E0, void* script, int index, void* node);

// action, file: the override node of that social action gets the file (an empty name
// leaves the buffer zeroed; its last byte is never initialised), kept by the script
int __cdecl Native_PlayerSocialOverrideLoad(lua_State* L) {
    int action = Script::GetInt(L, 0);
    char file[256];
    memset(file, 0, 255);
    if (*Script::GetString(L, 1) != '\0') {
        strcpy(file, Script::GetString(L, 1));
    }
    int index = action - 0x17;
    void* node = snlActionTree_Find(snlSocialOverridePaths[index], 0);
    if (node != nullptr && snVCall<uint8_t>(node, 4, snlActionFileReference_Type())) {
        snlActionFileReference_Fn5F6670(node);
        snlActionFileReference_SetFile(node, file);
        snlActionFileReference_Fn5F6CE0(node);
        snlScript_AddSocialOverride(snCurrentScript(snLurStore), index, node);
    }
    return 0;
}

GAME_FN(SnVec*, __thiscall, snlSteering_DirectionOf, 0x004A4720, void* steering, SnVec* out, float heading);
GAME_FN(void*, __thiscall, snlFaceHeadingObjective_Construct, 0x00470B20, void* self, uint8_t* ped, float heading, int flag);

// 0x5D2300 (the ped in edi, then cdecl heading in degrees and a flag): the
// intelligence's (+0x2E0) move direction and its copy set to the heading's (the
// steering at +0x654), then a face-heading objective (a failed allocation hands on
// nullptr)
void snlFaceHeading(uint8_t* ped, float heading, int flag) {
    uint8_t* intelligence = snAt<uint8_t*>(ped, 0x2E0);
    SnVec direction;
    SnVec* d = snlSteering_DirectionOf(ped + 0x654, &direction, heading);
    snAt<SnVec>(intelligence, 0) = *d;
    intelligence = snAt<uint8_t*>(ped, 0x2E0);
    snAt<SnVec>(intelligence, 0xC) = snAt<SnVec>(intelligence, 0);
    void* mem = snMalloc(0x18);
    void* objective = nullptr;
    if (mem != nullptr) {
        objective = snlFaceHeadingObjective_Construct(mem, ped, float(double(heading) * double(3.1415927f) / 180.0), flag);
    }
    snObjectives_Add(ped + 0x5F0, objective);
}
RE_UNHOOKED_FN("Script::FaceHeading_5D2300", 0x005D2300, &snlFaceHeading); // (the ped in edi)

// area transitions: to a point with a heading (radians) in an area
GAME_FN(void, __thiscall, snlAreaTransitions_Request, 0x00415880, void* manager, const SnVec* pos, float heading, const int32_t* area);
GAME_VAR(int32_t, snlCurrArea, 0x00BD1008);
GAME_FN(float, __cdecl, snlAngleToHeading, 0x004333A0, float angle);

// The player's heading (the matrix's, else the placeable's own)
float snlHeadingOf(uint8_t* entity) {
    uint8_t* matrix = snAt<uint8_t*>(entity, 0x14);
    if (matrix == nullptr) {
        return snAt<float>(entity, 0x10);
    }
    return float(std::atan2(-double(snAt<float>(matrix, 0x10)), double(snAt<float>(matrix, 0x14))));
}

// x, y, z, area: with the player's heading
int __cdecl Native_PlayerSetPosXYZArea(lua_State* L) {
    SnVec pos;
    pos.x = Script::GetFloat(L, 0);
    pos.y = Script::GetFloat(L, 1);
    pos.z = Script::GetFloat(L, 2);
    int32_t area = Script::GetInt(L, 3);
    float heading = 0.0f;
    if (snPlayerPed != nullptr) {
        heading = snlHeadingOf(snPlayerPed);
    }
    snAt<uint8_t>(snlObj9E4F4C, 0x8DC) = 1;
    snlAreaTransitions_Request(snlAreaTransitions, &pos, heading, &area);
    return 0;
}

// The point (and heading) of a route (the id as 16 bits) and its index (1-based, stopping
// at the last)
uint8_t* snlRoutePoint(uint8_t* route, bool hasIndex, int index) {
    uint8_t* point = snlRoute_First(route);
    if (hasIndex) {
        for (int i = 1; i < index; i++) {
            uint8_t* next = snlRoute_Next(route);
            if (next == nullptr) {
                break;
            }
            point = next;
        }
    }
    return point;
}

// route[, point]: in the route's area (+0xC; -1: the current one)
int __cdecl Native_PlayerSetPosPoint(lua_State* L) {
    bool hasIndex = false;
    int index = 0; // (only read with the argument)
    if (lua::gettop(L) == 2) {
        hasIndex = true;
        index = Script::GetInt(L, 1);
    }
    uint8_t* route = snlRoutes_Get(snlRoutes, uint16_t(Script::GetInt(L, 0)));
    uint8_t* point = snlRoutePoint(route, hasIndex, index);
    SnVec pos = {snAt<float>(point, 0), snAt<float>(point, 4), snAt<float>(point, 8)};
    float heading = snlAngleToHeading(float(double(snAt<float>(point, 0x1C)) * double(3.1415927f) / 180.0));
    int32_t area = snAt<int8_t>(route, 0xC);
    if (area == -1) {
        area = snlCurrArea;
    }
    snAt<uint8_t>(snlObj9E4F4C, 0x8DC) = 1;
    snlAreaTransitions_Request(snlAreaTransitions, &pos, heading, &area);
    return 0;
}

// area, route[, point]
int __cdecl Native_PlayerSetPosPointArea(lua_State* L) {
    int32_t area = Script::GetInt(L, 0);
    uint8_t* route = snlRoutes_Get(snlRoutes, uint16_t(Script::GetInt(L, 1)));
    bool hasIndex = false;
    int index = area; // (unused without the argument)
    if (lua::gettop(L) == 3) {
        hasIndex = true;
        index = Script::GetInt(L, 2);
    }
    uint8_t* point = snlRoutePoint(route, hasIndex, index);
    SnVec pos = {snAt<float>(point, 0), snAt<float>(point, 4), snAt<float>(point, 8)};
    float heading = snlAngleToHeading(float(double(snAt<float>(point, 0x1C)) * double(3.1415927f) / 180.0));
    snAt<uint8_t>(snlObj9E4F4C, 0x8DC) = 1;
    snlAreaTransitions_Request(snlAreaTransitions, &pos, heading, &area);
    return 0;
}

// The player's vehicle's position when in one
int __cdecl Native_PlayerGetPosXYZ(lua_State* L) {
    uint8_t* player = snGetPed(0, SN_HANDLE_PLAYER);
    snCurrentScript(snLurStore); // (the result is unused)
    uint8_t* vehicle = snAt<uint8_t*>(player, 0x1554);
    const float* pos = snEntityPos(vehicle != nullptr ? vehicle : player);
    float x = pos[0], y = pos[1], z = pos[2];
    Script::PushFloat(L, x);
    Script::PushFloat(L, y);
    Script::PushFloat(L, z);
    return 3;
}

GAME_FN(void, __thiscall, snlPed_SetHeading, 0x00475730, void* ped, float heading);

// heading (degrees? the game passes it on as is), mode: 0 set at once, else turn to it
int __cdecl Native_PlayerFaceHeading(lua_State* L) {
    float heading = Script::GetFloat(L, 0);
    int mode = Script::GetInt(L, 1);
    uint8_t* player = snGetPed(0, SN_HANDLE_PLAYER);
    if (mode == 0) {
        snlPed_SetHeading(player, heading);
        return 0;
    }
    snlFaceHeading(player, heading, 0); // (0x5D2300: the ped in edi, not hookable)
    return 0;
}

GAME_FN(int32_t*, __cdecl, snlMakeAttackKind, 0x00496AF0, int32_t* out, int first, int second);
GAME_FN(void*, __thiscall, snlAttackPedObjective_Construct, 0x00470D60, void* self, uint8_t* ped, uint8_t* target, int kind);
GAME_FN(uint8_t, __thiscall, snlObjectives_Prepend, 0x00471470, void* manager, void* objective);

// ped, kind (a number, then "in front" (false)) or two flags (true, true) the kind is made
// of (the game passes them as dwords with uninitialised upper bytes)
int __cdecl Native_PedAttackPlayer(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    uint8_t inFront = 0;
    int32_t kind;
    if (Script::IsNumber(L, 1)) {
        kind = Script::GetInt(L, 1);
        inFront = Script::GetBoolOpt(L, 2, 0);
    } else {
        uint8_t first = Script::GetBoolOpt(L, 1, 1);
        uint8_t second = Script::GetBoolOpt(L, 2, 1);
        int32_t made;
        kind = *snlMakeAttackKind(&made, first, second);
    }
    uint8_t* ped = snGetPed(handle, SN_HANDLE_PED);
    uint8_t* player = snGetPed(0, SN_HANDLE_PLAYER);
    void* mem = snMalloc(0x1C);
    void* objective = mem != nullptr ? snlAttackPedObjective_Construct(mem, ped, player, kind) : nullptr;
    if (inFront) {
        snlObjectives_Prepend(ped + 0x5F0, objective);
        return 0;
    }
    snObjectives_Add(ped + 0x5F0, objective);
    return 0;
}

GAME_FN(void, __cdecl, snlRegisterReference, 0x004657D0, void* entity, re::Ptr32<void>* ref);
GAME_FN(void*, __thiscall, snlFPPassengerObjective_Construct, 0x0046FDD0, void* self, uint8_t* ped, uint8_t* vehicle);
GAME_FN(void, __thiscall, snlObjectives_Clear, 0x00470350, void* manager);
GAME_FN(void, __thiscall, snlPed_Fn4732A0, 0x004732A0, void* ped, int, int);
inline void* const snlTheCamera = reinterpret_cast<void*>(0x00C3CC68);
GAME_FN(void, __thiscall, snlCamera_SetShot, 0x004F2C20, void* camera, int shot, int, int, int, int);

// vehicle (16 bits), on: while the player is in state 0xD (+0x1310), riding on the back
// of it (+0x430 the vehicle, set without releasing an older one; a passenger objective
// in front; the player's virtual +0x38 moves it to the vehicle), or off again
int __cdecl Native_PlayerFixToBackOfVehicle(lua_State* L) {
    int handle = uint16_t(Script::GetInt(L, 0));
    uint8_t on = Script::GetBool(L, 1);
    uint8_t* player = snGetPed(0, SN_HANDLE_PLAYER);
    uint8_t* vehicle = snlGetVehicle(handle);
    if (snAt<int32_t>(player, 0x1310) != 0xD) {
        return 0;
    }
    if (on) {
        re::Ptr32<void>* ref = &snAt<void*>(player, 0x430);
        *ref = vehicle;
        snlRegisterReference(vehicle, ref);
        void* mem = snMalloc(0x14);
        snlObjectives_Prepend(player + 0x5F0, mem != nullptr ? snlFPPassengerObjective_Construct(mem, player, vehicle) : nullptr);
        const float* pos = snEntityPos(vehicle);
        SnVec at = {pos[0], pos[1], pos[2]};
        snVCall<void>(player, 0x38, at.x, at.y, at.z);   // (Teleport takes the floats)
        return 0;
    }
    snAt<int32_t>(player, 0x1E94) = 0;
    snlObjectives_Clear(player + 0x5F0);
    snAt<int32_t>(player, 0x20) = 1;
    snlPed_Fn4732A0(player, 0xFF, 0);
    snlCamera_SetShot(snlTheCamera, 1, 0, 1, 0, 0);
    snAt<int32_t>(player, 0x1318) = 0;
    return 0;
}

// ped, social action (from 0x17), on: the ped's per-action flags (+0xB5A overrides,
// +0xB4C disabled actions; the action unchecked)
int __cdecl Native_PlayerSocialEnableOverrideAgainstPed(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    int action = Script::GetInt(L, 1);
    snAt<uint8_t>(ped, 0xB5A + action - 0x17) = Script::GetBool(L, 2);
    return 0;
}

int __cdecl Native_PlayerSocialDisableActionAgainstPed(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    int action = Script::GetInt(L, 1);
    snAt<uint8_t>(ped, 0xB4C + action - 0x17) = Script::GetBool(L, 2);
    return 0;
}

// ped, action, callback: into +0xB68 (per action), flagged at +0xBA0
int __cdecl Native_PlayerRegisterSocialCallbackVsPed(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    if (ped != nullptr) {
        int action = Script::GetInt(L, 1) - 0x17;
        Script::StoreFunctionRefOpt(L, 2, ped + 0xB68 + action * 4);
        snAt<uint8_t>(ped, 0xBA0 + action) = 1;
    }
    return 0;
}

GAME_FN(void, __thiscall, snlScanner_RecordAttackTime, 0x0048C190, void* scanner, int time);

// [time]: into the player's scanner (+0x3BC)
int __cdecl Native_PlayerRecordAttackTime(lua_State* L) {
    int time = 0;
    if (lua::gettop(L) != 0 && lua::gettop(L) == 1) {
        time = Script::GetInt(L, 0);
    }
    if (snPlayerPed != nullptr) {
        snlScanner_RecordAttackTime(snPlayerPed + 0x3BC, time);
    }
    return 0;
}

GAME_FN(void, __cdecl, snlLoadingScreenMessage, 0x0043C4F0, const char* a, const char* b, uintptr_t);
GAME_FN(void, __cdecl, snlLoadAllRequestedModels, 0x00533EE0, int onlyPriority);
GAME_FN(void, __cdecl, snlPlayerPed_SetupPlayerPed, 0x0049B570);
GAME_FN(void, __cdecl, snlPlayerPed_RemoveFromWorld, 0x0049A660);
GAME_FN(void, __cdecl, snlPlayerPed_AddToWorld, 0x0049A670);
GAME_FN(void, __thiscall, snlPed_Fn47B7E0, 0x0047B7E0, void* ped, int state, int, int);
GAME_FN(float, __cdecl, snlFindGroundZ2D, 0x004634A0, float x, float y, int);
GAME_FN(float, __thiscall, snlEntity_GetDistanceFromCentreOfMassToBase, 0x00466110, void* entity);
GAME_FN(void, __thiscall, snlEntity_SetPosition, 0x0041C4E0, void* entity, float x, float y, float z);
GAME_FN(void, __thiscall, snlCamera_Init, 0x004F3720, void* camera);
// the lowest z taken as given when creating the player (below: the ground's)
GAME_VAR(float, snlPlayerCreateMinZ, 0x00A14A8C);

// 0x5D2AC0: the player (model 0, loaded at once) set up and placed at x, y and z (the
// ground's at or below the minimum z, or unordered), its base on it; the camera reset.
// Returns 0
int __cdecl CreatePlayer(float x, float y, float z) {
    if (snlGetStreamingInfo(0)[4] != 1) {
        snlRequestModel(0, 5);
        snlLoadingScreenMessage("LuaCreatePlayer()", "CStreaming::LoadAllRequestedModels()", 0);
        snlLoadAllRequestedModels(0);
    }
    snlPlayerPed_SetupPlayerPed();
    snlPed_Fn47B7E0(snPlayerPed, 2, 1, 1);
    snlPlayerPed_RemoveFromWorld();
    if (!(snlPlayerCreateMinZ < z)) {
        z = snlFindGroundZ2D(x, y, 1);
    }
    z = snlEntity_GetDistanceFromCentreOfMassToBase(snPlayerPed) + z;
    snlEntity_SetPosition(snPlayerPed, x, y, z);
    snlPlayerPed_AddToWorld();
    snlCamera_Init(snlTheCamera);
    return 0;
}

GAME_FN(int, __cdecl, snlCreatePlayer, 0x005D2AC0, float x, float y, float z);
GAME_FN(uint8_t, __thiscall, snlTrigger_Contains, 0x006D6FA0, void* trigger, const float* pos);
GAME_VAR(CPool<uint8_t>*, snlTriggerPool, 0x020C7C48);

// trigger slot (16 bits)[, a boolean: then no answer at all while the player is +0x1C98
// flagged or has +0x155C]
int __cdecl Native_PlayerIsInTrigger(lua_State* L) {
    int16_t slot = int16_t(Script::GetInt(L, 0));
    uint8_t* player = snGetPed(0, SN_HANDLE_PLAYER);
    if (lua::gettop(L) == 2 && Script::IsBoolean(L, 1) &&
        (snAt<uint8_t>(player, 0x1C98) != 0 || snAt<int32_t>(player, 0x155C) != 0)) {
        return 0;
    }
    uint8_t inside = 0;
    if (slot >= 0) {
        CPool<uint8_t>* pool = snlTriggerPool;
        uint8_t* trigger = (pool->flags[slot] & 0x80) != 0 ? nullptr : pool->objects + pool->elemSize * slot;
        if (snlTrigger_Contains(trigger, snEntityPos(player))) {
            inside = 1;
        }
    }
    Script::PushBool(L, inside);
    return 1;
}

// The weapon and item model ranges (0xA136B0..)
GAME_VAR(int32_t, snlFirstWeapon, 0x00A136B0);
GAME_VAR(int32_t, snlLastWeapon, 0x00A136B4);
GAME_VAR(int32_t, snlFirstItem, 0x00A136B8);
GAME_VAR(int32_t, snlLastItem, 0x00A136BC);
GAME_FN(int, __cdecl, snlItems_GetCount, 0x00541E00, int item);

// model: its limit (model info +0x18 -> +0x34) and whether the player has that many
int __cdecl Native_PlayerGetNumberOfItemsAllowed(lua_State* L) {
    int model = Script::GetInt(L, 0);
    int allowed = snAt<int32_t>(snAt<uint8_t*>(snModelInfoPtrs[model], 0x18), 0x34);
    int count = 0;
    if (model >= snlFirstWeapon && model <= snlLastWeapon) {
        count = snlInventory_Find(snAt<void*>(snPlayerPed, 0x1C4), model);
    } else if (model >= snlFirstItem && model <= snlLastItem) {
        count = snlItems_GetCount(model);
    }
    Script::PushInt(L, allowed);
    Script::PushBool(L, count >= allowed);
    return 2;
}

// (0x5D2AC0's 0 as the result)
int __cdecl Native_PlayerCreateXYZ(lua_State* L) {
    float z = Script::GetFloat(L, 2);
    float y = Script::GetFloat(L, 1);
    float x = Script::GetFloat(L, 0);
    Script::PushInt(L, snlCreatePlayer(x, y, z));
    return 1;
}

GAME_FN(void, __thiscall, snlEntity_Fn443AA0, 0x00443AA0, void* entity);
GAME_FN(void, __thiscall, snlEntity_UpdateRwFrame, 0x00465760, void* entity);

// Out of the world (virtual +0xC), moved, back in (+8)
int __cdecl Native_PlayerSetPosSimple(lua_State* L) {
    float x = Script::GetFloat(L, 0);
    float y = Script::GetFloat(L, 1);
    float z = Script::GetFloat(L, 2);
    snVCall<void>(snPlayerPed, 0xC);
    snlEntity_SetPosition(snPlayerPed, x, y, z);
    snlEntity_Fn443AA0(snPlayerPed);
    snlEntity_UpdateRwFrame(snPlayerPed);
    snVCall<void>(snPlayerPed, 8);
    return 0;
}

GAME_FN(void, __thiscall, snlBike_SetRider, 0x004BDF80, void* bike, uint8_t* ped);
GAME_FN(void, __thiscall, snlPed_EnterVehicle, 0x00476640, void* ped, uint8_t* vehicle);
GAME_FN(int32_t*, __thiscall, snlVehicle_GetClass, 0x004CAF30, void* vehicle, int32_t* out);
GAME_FN(void, __thiscall, snlPed_PlayAction, 0x004740F0, void* ped, const char* node, const char* file);

// PedPutOnBike for the player (the handle as 16 bits, nothing checked but the type)
int __cdecl Native_PlayerPutOnBike(lua_State* L) {
    int handle = uint16_t(Script::GetInt(L, 0));
    uint8_t* player = snGetPed(0, SN_HANDLE_PLAYER);
    uint8_t* bike = snlGetVehicle(handle);
    if (snAt<int32_t>(bike, 0x46C) != 1) {
        return 0;
    }
    snlBike_SetRider(bike, player);
    snlPed_EnterVehicle(player, bike);
    const float* pos = snEntityPos(bike);
    snlEntity_SetPosition(player, pos[0], pos[1], pos[2]);
    int32_t kind;
    if (*snlVehicle_GetClass(bike, &kind) == 3) {
        snlPed_PlayAction(player, "/Global/Vehicles/Motorcycle/Vehicles_Ride", "Act/Vehicles.act");
        return 0;
    }
    if (*snlVehicle_GetClass(bike, &kind) == 8) {
        snlPed_PlayAction(player, "/Global/Vehicles/Scooter/Vehicles_Ride", "Act/Vehicles.act");
        return 0;
    }
    snlPed_PlayAction(player, "/Global/Vehicles/Bikes/Vehicles_Ride", "Act/Vehicles.act");
    return 0;
}

// ---- POIs -----------------------------------------------------------------------------------

// The POIs (0xC0F5E4; +0x14C set: the kind-3 ones count as valid) and their pool
GAME_VAR(uint8_t*, snlPOIManager, 0x00C0F5E4);
GAME_FN(uint8_t*, __thiscall, snlPOIManager_Get, 0x00448DC0, void* manager, int index);
GAME_VAR(CPool<uint8_t>*, snlPOIPool, 0x020C7C84);
GAME_FN(uint8_t, __cdecl, snlIsAreaClear, 0x0048C990, const float* pos, float radius, int, uintptr_t);

// (FLT_MAX for no such POI)
int __cdecl Native_POIGetPosXYZ(lua_State* L) {
    uint8_t* poi = snlPOIManager_Get(snlPOIManager, Script::GetInt(L, 0));
    float none = *reinterpret_cast<const float*>(0x009009B8);
    float x = none, y = none, z = none;
    if (poi != nullptr) {
        const float* pos = snEntityPos(poi);
        x = pos[0];
        y = pos[1];
        z = pos[2];
    }
    Script::PushFloat(L, x);
    Script::PushFloat(L, y);
    Script::PushFloat(L, z);
    return 3;
}

// poi[, radius (0)]: no peds around it
int __cdecl Native_POIClearForPeds(lua_State* L) {
    uint8_t* poi = snlPOIManager_Get(snlPOIManager, Script::GetInt(L, 0));
    uint8_t clear = 0;
    if (poi != nullptr) {
        const float* pos = snEntityPos(poi);
        clear = snlIsAreaClear(pos, float(Script::GetIntOpt(L, 1, 0)), -1, 0);
    }
    Script::PushBool(L, clear);
    return 1;
}

// A POI of kind 3 while the manager's +0x14C is set, else one that's an object of the pool
int __cdecl Native_POIIsValid(lua_State* L) {
    uint8_t valid = 0;
    uint8_t* poi = snlPOIManager_Get(snlPOIManager, Script::GetInt(L, 0));
    if (poi != nullptr) {
        if (snAt<int32_t>(poi, 0x70) == 3 && snAt<int32_t>(snlPOIManager, 0x14C) != 0) {
            valid = 1;
        } else {
            CPool<uint8_t>* pool = snlPOIPool;
            int slot = int(poi - pool->objects) / pool->elemSize;
            if (slot < 0 || slot >= pool->size) {
                valid = 0;
            } else {
                valid = poi == pool->objects + pool->elemSize * slot;
            }
        }
    }
    Script::PushBool(L, valid);
    return 1;
}

// ---- shops ------------------------------------------------------------------------------------

// The shop (0xC18748); its positions from a route's first point, the rotation (heading, 0, 0)
inline void* const snlShop = reinterpret_cast<void*>(0x00C18748);
GAME_FN(void, __thiscall, snlShop_SetClerk_x64, 0x00455060, void* shop, int model, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void snlShop_SetClerk(void* a0, int a1, SnVec a2, SnVec a3) {
    return snlShop_SetClerk_x64(a0, a1, re::BitsOf(a2.x), re::BitsOf(a2.y), re::BitsOf(a2.z), re::BitsOf(a3.x), re::BitsOf(a3.y), re::BitsOf(a3.z));
}
GAME_FN(void, __thiscall, snlShop_SetPlayerPos_x64, 0x004550A0, void* shop, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void snlShop_SetPlayerPos(void* a0, SnVec a1, SnVec a2) {
    return snlShop_SetPlayerPos_x64(a0, re::BitsOf(a1.x), re::BitsOf(a1.y), re::BitsOf(a1.z), re::BitsOf(a2.x), re::BitsOf(a2.y), re::BitsOf(a2.z));
}
GAME_FN(void, __thiscall, snlShop_SetCameraTarget_x64, 0x004550D0, void* shop, uint32_t, uint32_t, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void snlShop_SetCameraTarget(void* a0, SnVec a1) {
    return snlShop_SetCameraTarget_x64(a0, re::BitsOf(a1.x), re::BitsOf(a1.y), re::BitsOf(a1.z));
}
GAME_FN(void, __thiscall, snlShop_SetSpeech, 0x004550F0, void* shop, const char* name, const char* s0, const char* s1,
        const char* s2, const char* s3, const char* s4, const char* s5, const char* s6);
GAME_FN(uint8_t*, __thiscall, snlShop_AddItem, 0x004553E0, void* shop, int shelf, int model, SnVec pos, SnVec rotation, int stock,
        int price, int flag);
GAME_FN(uint8_t, __thiscall, snlCallback_IsSet, 0x005D80E0, void* callback);

SnVec snlPointPos(uint8_t* point) {
    return SnVec{snAt<float>(point, 0), snAt<float>(point, 4), snAt<float>(point, 8)};
}

int __cdecl Native_ShopSetShopKeepInfo(lua_State* L) {
    int model = Script::GetInt(L, 0);
    uint8_t* point = snlRoute_First(snlRoutes_Get(snlRoutes, Script::GetInt(L, 1)));
    SnVec rotation = {snAt<float>(point, 0x1C), 0.0f, 0.0f};
    snlShop_SetClerk(snlShop, model, snlPointPos(point), rotation);
    return 0;
}

int __cdecl Native_ShopSetPlayerPos(lua_State* L) {
    uint8_t* point = snlRoute_First(snlRoutes_Get(snlRoutes, Script::GetInt(L, 0)));
    SnVec rotation = {snAt<float>(point, 0x1C), 0.0f, 0.0f};
    snlShop_SetPlayerPos(snlShop, snlPointPos(point), rotation);
    return 0;
}

int __cdecl Native_ShopSetCameraPos(lua_State* L) {
    uint8_t* point = snlRoute_First(snlRoutes_Get(snlRoutes, Script::GetInt(L, 0)));
    snlShop_SetCameraTarget(snlShop, snlPointPos(point));
    return 0;
}

int __cdecl Native_ShopSetConversationTree(lua_State* L) {
    const char* s[8];
    for (int i = 0; i < 8; i++) {
        s[i] = Script::GetString(L, i);
    }
    snlShop_SetSpeech(snlShop, s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7]);
    return 0;
}

// shelf, model, route, stock, price, callback, flag
int __cdecl Native_ShopAddItem(lua_State* L) {
    int shelf = Script::GetInt(L, 0);
    int model = Script::GetInt(L, 1);
    uint8_t* point = snlRoute_First(snlRoutes_Get(snlRoutes, Script::GetInt(L, 2)));
    int stock = Script::GetInt(L, 3);
    int price = Script::GetInt(L, 4);
    int flag = Script::GetInt(L, 6);
    SnVec rotation = {snAt<float>(point, 0x1C), 0.0f, 0.0f};
    uint8_t* item = snlShop_AddItem(snlShop, shelf, model, snlPointPos(point), rotation, stock, price, flag);
    Script::StoreFunctionRef(L, 5, item + 0x40);
    snlCallback_IsSet(item + 0x40); // (the result is unused)
    return 0;
}

} // namespace

#define SNL_REVERSED(name, address)       \
    static void* Original_Native_##name; \
    REVERSED("Native_" #name, address, RE_BODY(Native_##name), nullptr, &Original_Native_##name, re::Mode::Original)

SNL_REVERSED(PickupCreateXYZ, 0x005D0CF0);
SNL_REVERSED(PickupGetXYZ, 0x005D0DD0);
SNL_REVERSED(PickupCreateFromPed, 0x005D0E20);
SNL_REVERSED(PickupDelete, 0x005D0EC0);
SNL_REVERSED(PickupIsInAreaXYZ, 0x005D0F10);
SNL_REVERSED(PickupFindInArea, 0x005D0F80);
SNL_REVERSED(PickupRequestModel, 0x005D1050);
SNL_REVERSED(PickupIsPickedUp, 0x005D1110);
SNL_REVERSED(PickupCreatePoint, 0x005D1160);
SNL_REVERSED(PickupDestroyTypeInAreaXYZ, 0x005D12E0);
SNL_REVERSED(PlayerSetPosXYZ, 0x005D13E0);
SNL_REVERSED(PlayerSetControl, 0x005D1450);
SNL_REVERSED(PlayerStopAllActionControllers, 0x005D1500);
SNL_REVERSED(PlayerIsInAreaXYZ, 0x005D1550);
SNL_REVERSED(PlayerIsInAreaObject, 0x005D1650);
SNL_REVERSED(PlayerIsInVehicle, 0x005D17A0);
SNL_REVERSED(PlayerDetachFromVehicle, 0x005D1850);
SNL_REVERSED(PlayerSetWeapon, 0x005D1A20);
SNL_REVERSED(PlayerSwapModel, 0x005D1AD0);
SNL_REVERSED(PlayerAddMoney, 0x005D1B50);
SNL_REVERSED(PlayerLockButtonInputsExcept, 0x005D1D10);
SNL_REVERSED(PlayerHasUnlockedUpgrade, 0x005D2110);
SNL_REVERSED(PlayerSocialOverrideLoad, 0x005D2170);
SNL_REVERSED(PlayerSetPosXYZArea, 0x005D23B0);
SNL_REVERSED(PlayerSetPosPoint, 0x005D2470);
SNL_REVERSED(PlayerSetPosPointArea, 0x005D2570);
SNL_REVERSED(PlayerGetPosXYZ, 0x005D2670);
SNL_REVERSED(PlayerFaceHeading, 0x005D2710);
SNL_REVERSED(PedAttackPlayer, 0x005D2780);
SNL_REVERSED(PlayerFixToBackOfVehicle, 0x005D2860);
SNL_REVERSED(PlayerSocialEnableOverrideAgainstPed, 0x005D2970);
SNL_REVERSED(PlayerSocialDisableActionAgainstPed, 0x005D29C0);
SNL_REVERSED(PlayerRegisterSocialCallbackVsPed, 0x005D2A10);
SNL_REVERSED(PlayerRecordAttackTime, 0x005D2A70);
static void* Original_CreatePlayer;
REVERSED("Script::CreatePlayer", 0x005D2AC0, RE_BODY(CreatePlayer), nullptr, &Original_CreatePlayer, re::Mode::Original);
SNL_REVERSED(PlayerIsInTrigger, 0x005D2B90);
SNL_REVERSED(PlayerGetNumberOfItemsAllowed, 0x005D2C60);
SNL_REVERSED(PlayerCreateXYZ, 0x005D2CE0);
SNL_REVERSED(PlayerSetPosSimple, 0x005D2D30);
SNL_REVERSED(PlayerPutOnBike, 0x005D2DC0);
SNL_REVERSED(POIGetPosXYZ, 0x005D2F70);
SNL_REVERSED(POIClearForPeds, 0x005D3010);
SNL_REVERSED(POIIsValid, 0x005D3090);
SNL_REVERSED(ShopSetShopKeepInfo, 0x005D3130);
SNL_REVERSED(ShopSetPlayerPos, 0x005D31C0);
SNL_REVERSED(ShopSetCameraPos, 0x005D3240);
SNL_REVERSED(ShopSetConversationTree, 0x005D3290);
SNL_REVERSED(ShopAddItem, 0x005D3320);
