// Script natives 0x5C6340..0x5C7A10: model and weapon requests, action trees and their
// overrides, AI butes, cover, random peds, and the first ped ones (delete, stop, face a
// heading / point / entity) with their helpers (0x5C7380 the ped of a script handle,
// 0x5C73E0 deleting one, 0x5C7440 / 0x5C7520 facing, 0x5C7600 warping). Each native is
// `int __cdecl Native_X(lua_State* L)`, returning how many results it pushed.

#include <cmath>
#include <cstdint>
#include <cstring>

#include "game/NativesMPP.h"

#include "game/Crt.h"
namespace {

GAME_FN(void*, __cdecl, PS_GetStreamingInfo, 0x0052DD30, int id);
GAME_FN(void, __cdecl, PS_RequestModel, 0x00532020, int model, int flags);
GAME_FN(void, __thiscall, PS_Script_AddModel, 0x005D9450, void* script, int id);
GAME_FN(void, __thiscall, Script_RemoveModel, 0x005D94A0, void* script, int id);
GAME_FN(void, __thiscall, Script_RemovePed, 0x005D9680, void* script, int handle);
GAME_FN(void, __thiscall, Script_AddSocialOverride, 0x005D9290, void* script, int id, void* node);
GAME_FN(void*, __thiscall, Scripts_GetCurrentOrNull, 0x005DB960, void* store);
GAME_FN(uint8_t*, __thiscall, LurStore_GetScript, 0x005DBD50, void* store, lua_State* L);
GAME_VAR(int32_t, ms_firstWeaponModel, 0x00A136B0);
GAME_VAR(int32_t, ms_lastWeaponModel, 0x00A136B4);
GAME_FN(uint8_t, __cdecl, RequestWeaponModels, 0x004D9790, int weapon, int flags);
GAME_FN(float*, __thiscall, Entity_GetForward, 0x004154A0, void* entity, float* out);
GAME_FN(float, __thiscall, PS_Normalise, 0x00414E10, float* v);
GAME_FN(float*, __cdecl, PS_VectorSub, 0x00414B70, float* out, const float* a, const float* b);
GAME_VAR(void*, g_population, 0x00C2C108);
GAME_FN(uint8_t*, __thiscall, Population_FindPed, 0x0049E860, void* pop, int, int, int, void* near, float range, int);
inline void* const ps_routes = reinterpret_cast<void*>(0x00BC5E7C);
GAME_FN(void*, __thiscall, PS_Routes_Get, 0x006D6450, void* routes, int index);
GAME_FN(float*, __thiscall, PS_Route_First, 0x006D59B0, void* route);
GAME_FN(float*, __thiscall, PS_Route_Next, 0x006D59F0, void* route);
inline void* const charTriggers = reinterpret_cast<void*>(0x00BCD098);
GAME_FN(void, __thiscall, CharTriggers_Add, 0x004246E0, void* self, int id, int area, const float* point, float heading,
        float radius, const char* a, const char* b, int, int, int, int, int, int, uintptr_t, uintptr_t, int, int, int, int, int);
GAME_FN(uint8_t, __thiscall, ActFiles_IsLoaded, 0x005DBFC0, void* store, const char* name);
GAME_FN(void*, __cdecl, ActFiles_Find, 0x005F6DB0, const char* name, uintptr_t, uintptr_t);
GAME_FN(void, __thiscall, ActFiles_Load, 0x005DC1A0, void* store, uintptr_t, uintptr_t, const char* name, void* file);
GAME_FN(void, __thiscall, ActFiles_Add, 0x005DBEB0, void* store, const char* name, void* file, int);
GAME_FN(void, __cdecl, PS_LoadingScreen, 0x0043C4F0, const char* title, const char* message, uintptr_t);
inline void __cdecl PS_Memset(void* p, int value, uint32_t size) { crt::Memset(p, value, (size_t)size); }
GAME_FN(void*, __cdecl, PS_FindActionNode, 0x005F5B10, const char* path, uintptr_t);
GAME_FN(int16_t, __cdecl, ActionFileReference_GetClassId, 0x005C6190);
GAME_FN(void, __thiscall, ActionFileRef_5F6670, 0x005F6670, void* node);
GAME_FN(void, __thiscall, ActionFileRef_SetFile, 0x005F56B0, void* node, const char* file);
GAME_FN(void, __thiscall, ActionFileRef_5F6CE0, 0x005F6CE0, void* node);
inline int __cdecl PS_Strncmp(const char* a, const char* b, uint32_t n) { return crt::Strncmp(a, b, (size_t)n); }
GAME_FN(void, __thiscall, PS_ButesPath_Construct, 0x005FA8C0, void* path, int count, const uint32_t* parts);
GAME_FN(uint32_t, __cdecl, PS_Config_GetHash, 0x0072AEA0, const void* path, const void* type);
GAME_FN(void, __thiscall, PS_CopyInt, 0x005FA880, uint32_t* dst, const void* src);
GAME_FN(uint32_t*, __thiscall, PS_NameKey_Construct, 0x005FA910, uint32_t* key, const char* name);
GAME_VAR(uint32_t, g_aiButes, 0x020C889C);
GAME_FN(float, __cdecl, AngleToHeading, 0x004333A0, float angle);
inline void* const coverPoints = reinterpret_cast<void*>(0x00C2B210);
GAME_FN(uint8_t*, __thiscall, CoverPoints_New, 0x004883A0, void* self, void* script);
GAME_FN(void*, __thiscall, CoverPoints_GetProfile, 0x00487F70, void* self, const char* name);
GAME_FN(void, __cdecl, SetModelNotNeeded, 0x0052FF40, int model);
GAME_ARRAY(float, g_poiDefaultPosition, 0x00BCC564);
GAME_FN(uint8_t, __thiscall, Population_FindSpawnPoint, 0x0049D860, void* pop, const float* from, float* out, int,
        int32_t* out2, float, float);
GAME_FN(int, __thiscall, Population_GetRandomModel, 0x0049C230, void* pop, int type, int, int);
GAME_FN(uint8_t*, __thiscall, Population_AddPedAtModel, 0x0049F6C0, void* pop, int model, const float* pos, int, int32_t);   // (a float; the game passes FindSpawnPoint's out2 node pointer bits)
GAME_FN(uint8_t, __cdecl, IsAreaClear, 0x0048C990, const float* pos, float radius, int, uintptr_t);
GAME_FN(int, __cdecl, PedHandleToSlot, 0x0046DF10, int handle);
GAME_FN(void, __thiscall, Fn_4CE960, 0x004CE960, void*);
GAME_FN(void, __cdecl, PS_Entity_45E690, 0x0045E690, void* entity);
GAME_FN(void, __thiscall, Ped_SetHeading, 0x00475730, void* ped, float heading);
GAME_FN(void, __thiscall, PS_Entity_443AA0, 0x00443AA0, void* entity);
GAME_FN(void, __cdecl, PS_UpdateFrame, 0x006C9AF0, void* frame);
GAME_FN(void*, __cdecl, PS_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void*, __thiscall, FaceHeadingObjective_Construct, 0x00470B20, void* self, void* ped, float heading, int flag);
GAME_FN(void*, __thiscall, FacePositionObjective_Construct, 0x00470B50, void* self, void* ped, const float* pos);
GAME_FN(void*, __thiscall, FaceEntityObjective_Construct, 0x00470C10, void* self, void* ped, void* target, int flag);
GAME_FN(float, __thiscall, FaceEntityObjective_GetHeading, 0x00470C90, void* self);
GAME_FN(void, __thiscall, FaceEntityObjective_Destruct, 0x00470C60, void* self);
GAME_FN(void*, __thiscall, IdleObjective_Construct, 0x0046FC60, void* self, void* ped);
GAME_FN(uint8_t, __thiscall, Objectives_Add, 0x00471390, void* manager, void* objective);
GAME_FN(void, __thiscall, Objectives_Clear, 0x00470350, void* manager);
GAME_FN(void, __thiscall, Objectives_SetDefault, 0x00470300, void* manager, void* objective);
GAME_FN(void, __thiscall, ActionController_Stop, 0x005F40A0, void* controller);
GAME_FN(float, __cdecl, GetATanOfXY, 0x00432FE0, float x, float y);
GAME_FN(float, __cdecl, PS_FindGroundZ2D, 0x004634A0, float x, float y, int);
GAME_VAR(float, ps_groundZThreshold, 0x00A14A8C);
GAME_VAR(uint8_t*, g_areaTransitions, 0x009E4F4C);
GAME_FN(void, __thiscall, AreaTransitions_Request, 0x00415880, void* self, const float* pos, float heading, int* area);
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_FN(float, __thiscall, PS_Entity_DistanceToBase, 0x00466110, void* entity);
GAME_FN(uint8_t*, __cdecl, PS_GetAreaEntity, 0x005C28D0, int handle, int kind, int noVehicle, int cutscene);
GAME_FN(void, __cdecl, DeletePedByHandle, 0x005C73E0, int handle, int kind);
GAME_FN(void, __cdecl, PedFaceHeadingHelper, 0x005C7440, void* ped, int flag, float heading);
GAME_FN(void, __cdecl, PedFacePointHelper, 0x005C7520, void* ped, int flag, const float* pos);

void* PS_CurrentScript() {
    return M_Scripts_GetCurrent(m_lurStore);
}

// The streaming state (+4): 1 loaded, 2..4 on its way
uint8_t LoadState(int model) {
    return At<uint8_t>(PS_GetStreamingInfo(model), 4);
}

// (model, [priority]) -> it was loaded; else it's requested (for the script unless
// priority, which asks with flags 0x15 and adds nothing)
int __cdecl Native_RequestModel(lua_State* L) {
    int model = Script::GetInt(L, 0);
    uint8_t priority = Script::GetBoolOpt(L, 1, 0);
    uint8_t loaded = LoadState(model) == 1;
    if (LoadState(model) != 2 && LoadState(model) != 3 && LoadState(model) != 4) {
        if (priority == 0) {
            PS_RequestModel(model, 0x16);
            PS_Script_AddModel(PS_CurrentScript(), model);
            Script::PushBool(L, loaded);
            return 1;
        }
        PS_RequestModel(model, 0x15);
    }
    Script::PushBool(L, loaded);
    return 1;
}

// (weapon model, [priority]) -> its models are loaded (0 outside the weapon range)
int __cdecl Native_WeaponRequestModel(lua_State* L) {
    int model = Script::GetInt(L, 0);
    uint8_t priority = Script::GetBoolOpt(L, 1, 0);
    uint8_t loaded = 0;
    if (model >= ms_firstWeaponModel && model <= ms_lastWeaponModel) {
        if (priority == 0) {
            loaded = RequestWeaponModels(model, 2);
            if (Scripts_GetCurrentOrNull(m_lurStore) != nullptr) {
                PS_Script_AddModel(PS_CurrentScript(), model);
            }
        } else {
            loaded = RequestWeaponModels(model, 1);
        }
    }
    Script::PushBool(L, loaded);
    return 1;
}

// 0x5C65C0: the point is within the entity's field of view (fov degrees, flat)
uint8_t __cdecl IsPointInFront(void* entity, float fov, float x, float y, float z) {
    float forward[3];
    Entity_GetForward(entity, forward);
    forward[2] = 0.0f;
    PS_Normalise(forward);
    float point[3] = {x, y, z};
    float d[3];
    PS_VectorSub(d, point, EntityPos(static_cast<uint8_t*>(entity)));
    d[2] = 0.0f;
    PS_Normalise(d);
    float dot = forward[1] * d[1] + forward[0] * d[0] + d[2] * forward[2];
    float half = float(double(fov) * 0.5 * 3.1415927410125732 / 180.0);
    float limit = float(std::cos(half));
    return dot > limit;
}

// ([a = -1], [b = -1], [c = -1], [range = 50]) -> an ambient ped near the player, -1
int __cdecl Native_PedFindRandomPed(lua_State* L) {
    int a = Script::GetIntOpt(L, 0, -1);
    int b = Script::GetIntOpt(L, 1, -1);
    int c = Script::GetIntOpt(L, 2, -1);
    float range = Script::GetFloatOpt(L, 3, 50.0f);
    uint8_t* ped = Population_FindPed(g_population, a, b, c, m_playerPed, range, 0);
    if (ped == nullptr) {
        Script::PushInt(L, -1);
        return 1;
    }
    Script::PushInt(L, VCall<int>(ped, 0xAC));
    return 1;
}

// (id, route, radius, a, b): a character trigger at the route's first point (its
// heading, +0x1C, in degrees). (Also registered as PedCharacterTrigger)
int __cdecl Native_PedCharacterTriggerDelete(lua_State* L) {
    int id = Script::GetInt(L, 0);
    uint8_t* route = static_cast<uint8_t*>(PS_Routes_Get(ps_routes, Script::GetInt(L, 1)));
    float* point = PS_Route_First(route);
    float degrees = PS_Route_First(route)[7];
    int area = At<int8_t>(route, 0xC);
    float heading = float(double(degrees) * 3.1415927410125732 / 180.0);
    float radius = Script::GetFloat(L, 2);
    const char* a = Script::GetString(L, 3);
    const char* b = Script::GetString(L, 4);
    CharTriggers_Add(charTriggers, id, area, point, heading, radius, a, b, 0, 5, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0);
    return 0;
}

// (name): the action file loaded for the script (found first unless already loaded)
int __cdecl Native_LoadActionTree(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    void* file = nullptr;
    if (ActFiles_IsLoaded(m_lurStore, name) == 0) {
        file = ActFiles_Find(name, 0, 0);
    }
    ActFiles_Load(m_lurStore, 0, 0, name, file);
    return 0;
}

// (name): an action file loaded now. (Copied unbounded into 256 bytes)
int __cdecl Native_Load(lua_State* L) {
    char name[256];
    std::strcpy(name, Script::GetString(L, 0));
    PS_LoadingScreen("CmdLoad", nullptr, 0);
    void* file = ActFiles_Find(name, 0, 0);
    if (file != nullptr) {
        ActFiles_Add(m_lurStore, name, file, 0);
    }
    return 0;
}

// A file reference node's file replaced (when the node is one)
bool OverrideFileReference(void* node, const char* file) {
    if (node == nullptr) {
        return false;
    }
    int16_t id = ActionFileReference_GetClassId();
    if (VCall<uint8_t>(node, 4, int(id)) == 0) {
        return false;
    }
    ActionFileRef_5F6670(node);
    ActionFileRef_SetFile(node, file);
    ActionFileRef_5F6CE0(node);
    return true;
}

// (node path, file): the file reference node points at another file. (The buffers are
// zeroed for 255 bytes; copies unbounded)
int __cdecl Native_PedOverrideActionTreeReference(lua_State* L) {
    char path[256];
    char file[256];
    path[0] = '\0';
    path[1] = '\0';
    PS_Memset(path + 2, 0, 0xFD);
    file[0] = '\0';
    file[1] = '\0';
    PS_Memset(file + 2, 0, 0xFD);
    if (*Script::GetString(L, 0) != '\0') {
        std::strcpy(path, Script::GetString(L, 0));
    }
    if (*Script::GetString(L, 1) != '\0') {
        std::strcpy(file, Script::GetString(L, 1));
    }
    OverrideFileReference(PS_FindActionNode(path, 0), file);
    return 0;
}

// (social anim, file): its override node (the table at 0x90FC50, 0x40 each) points at
// the file; the script remembers it
int __cdecl Native_PedSocialOverrideLoad(lua_State* L) {
    int id = Script::GetInt(L, 0);
    char file[256];
    file[0] = '\0';
    file[1] = '\0';
    PS_Memset(file + 2, 0, 0xFD);
    if (*Script::GetString(L, 1) != '\0') {
        std::strcpy(file, Script::GetString(L, 1));
    }
    void* node = PS_FindActionNode(reinterpret_cast<const char*>(0x0090FC50 + id * 0x40), 0);
    if (OverrideFileReference(node, file)) {
        Script_AddSocialOverride(PS_CurrentScript(), id, node);
    }
    return 0;
}

// (name): the AI butes used ("Default": the root's)
int __cdecl Native_PedSetAIButes(lua_State* L) {
    char name[200];
    std::strcpy(name, Script::GetString(L, 0));
    if (PS_Strncmp(name, "Default", 8) == 0) {
        uint8_t path[8];
        PS_ButesPath_Construct(path, 0, nullptr);
        uint32_t hash = PS_Config_GetHash(path, reinterpret_cast<void*>(0x01B6532C));
        if (hash != 0) {
            g_aiButes = hash;
        }
        return 0;
    }
    uint32_t parts[2];
    PS_CopyInt(&parts[0], reinterpret_cast<void*>(0x01B6532C));
    PS_NameKey_Construct(&parts[1], name);
    uint8_t path[8];
    PS_ButesPath_Construct(path, 2, parts);
    uint32_t hash = PS_Config_GetHash(path, reinterpret_cast<void*>(0x01B6532C));
    if (hash != 0) {
        g_aiButes = hash;
    }
    return 0;
}

// 0x5C6E90 (argument index in eax, L in ecx: not hooked): a route's n-th point (the
// first when n <= 1) and its heading
[[maybe_unused]] uint8_t RoutePointArg(int i, lua_State* L, float* pos, float* heading, int n) {
    void* route = PS_Routes_Get(ps_routes, Script::GetInt(L, i));
    float* p = PS_Route_First(route);
    for (int k = 1; k < n;) {
        float* next = PS_Route_Next(route);
        if (next == nullptr) {
            break;
        }
        k++;
        p = next;
    }
    pos[0] = p[0];
    pos[1] = p[1];
    pos[2] = p[2];
    *heading = AngleToHeading(float(double(p[7]) * 3.1415927410125732 / 180.0));
    return 1;
}

// (route, profile name, [scale = 1]) or (route, 7 floats, 4 ints, flag, [scale = 1]):
// a cover point at the route's first point (facing its second, or its own heading)
int __cdecl Native_PedAddCover(lua_State* L) {
    uint8_t* route = static_cast<uint8_t*>(PS_Routes_Get(ps_routes, uint16_t(Script::GetInt(L, 0))));
    if (route == nullptr) {
        return 0;
    }
    uint8_t* cover = CoverPoints_New(coverPoints, LurStore_GetScript(m_lurStore, L));
    float* p = PS_Route_First(route);
    At<float>(cover, 8) = p[0];
    At<float>(cover, 0xC) = p[1];
    At<float>(cover, 0x10) = p[2];
    At<float>(cover, 0x20) = AngleToHeading(float(double(p[7]) * 3.1415927410125732 / 180.0));
    float* next = PS_Route_Next(route);
    if (next == nullptr) {
        At<float>(cover, 0x14) = At<float>(cover, 8);
        At<int32_t>(cover, 4) = 1;
        At<float>(cover, 0x18) = At<float>(cover, 0xC);
        At<float>(cover, 0x1C) = At<float>(cover, 0x10);
    } else {
        At<int32_t>(cover, 4) = 2;
        At<float>(cover, 0x14) = next[0];
        At<float>(cover, 0x18) = next[1];
        At<float>(cover, 0x1C) = next[2];
    }
    if (Script::IsString(L, 1) != 0) {
        const uint32_t* profile = static_cast<const uint32_t*>(CoverPoints_GetProfile(coverPoints, Script::GetString(L, 1)));
        for (int i = 0; i < 0x12; i++) {
            At<uint32_t>(cover, 0x24 + i * 4) = profile[i];
        }
        At<float>(cover, 0x6C) = Script::GetFloatOpt(L, 2, 1.0f);
        return 0;
    }
    for (int i = 0; i < 7; i++) {
        At<float>(cover, 0x3C + i * 4) = Script::GetFloat(L, 1 + i);
    }
    for (int i = 0; i < 4; i++) {
        At<int32_t>(cover, 0x58 + i * 4) = Script::GetInt(L, 8 + i);
    }
    At<uint8_t>(cover, 0x68) = Script::GetBool(L, 12);
    At<float>(cover, 0x6C) = Script::GetFloatOpt(L, 13, 1.0f);
    return 0;
}

int __cdecl Native_ModelNotNeeded(lua_State* L) {
    int model = Script::GetInt(L, 0);
    SetModelNotNeeded(model);
    Script_RemoveModel(PS_CurrentScript(), model);
    return 0;
}

// ([type = -1], [a = 0], [b = 1], [x, y, z]) -> a ped spawned near the point, -1
int __cdecl Native_PedSpawnRandomPed(lua_State* L) {
    int type = Script::GetIntOpt(L, 0, -1);
    int a = Script::GetIntOpt(L, 1, 0);
    int b = Script::GetIntOpt(L, 2, 1);
    float found[3] = {g_poiDefaultPosition[0], g_poiDefaultPosition[1], g_poiDefaultPosition[2]};
    ScriptVector from;
    Script::GetVectorOpt(&from, L, 3, reinterpret_cast<const ScriptVector*>(g_poiDefaultPosition));
    int32_t spot = 0;
    int handle = -1;
    if (Population_FindSpawnPoint(g_population, &from.x, found, 1, &spot, 0.0f, 0.0f) != 0) {
        int model = Population_GetRandomModel(g_population, type, a, b);
        if (model != -1) {
            uint8_t* ped = Population_AddPedAtModel(g_population, model, found, 1, spot);
            if (ped != nullptr) {
                handle = VCall<int>(ped, 0xAC);
            }
        }
    }
    Script::PushInt(L, handle);
    return 1;
}

// ([x, y, z]) -> no peds within 0.2 (false for the default point)
int __cdecl Native_PedAreaClearOfPedsInXYZ(lua_State* L) {
    ScriptVector pos;
    Script::GetVectorOpt(&pos, L, 0, reinterpret_cast<const ScriptVector*>(g_poiDefaultPosition));
    uint8_t clear = 0;
    if (!(pos.x == g_poiDefaultPosition[0]) || !(pos.y == g_poiDefaultPosition[1]) || !(pos.z == g_poiDefaultPosition[2])) {
        clear = IsAreaClear(&pos.x, 0.2f, -1, 0);
    }
    Script::PushBool(L, clear);
    return 1;
}

// 0x5C7380: the ped of a script handle: -1 none; kind 3 the player; else the ped pool's
// slot (0x46DF10; below 24) when its handle matches
uint8_t* __cdecl GetPedFromHandle(int handle, int kind) {
    if (handle == -1) {
        return nullptr;
    }
    if (kind == 3) {
        return m_playerPed;
    }
    int slot = PedHandleToSlot(handle);
    if (slot >= 0x18) {
        return nullptr;
    }
    CPool<uint8_t>* pool = m_pedPool;
    if ((pool->flags[slot] & 0x80) != 0) {
        return nullptr;
    }
    uint8_t* ped = pool->objects + pool->elemSize * slot;
    if (ped == nullptr || VCall<int>(ped, 0xAC) != handle) {
        return nullptr;
    }
    return ped;
}

// 0x5C73E0: deletes the ped of a handle (not the player's), out of its vehicle first
void __cdecl DeletePedByHandleImpl(int handle, int kind) {
    if (kind == 3) {
        return;
    }
    uint8_t* ped = M_GetPedFromHandle(handle, 2);
    if (ped == nullptr || VCall<int>(ped, 0xAC) != handle) {
        return;
    }
    uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
    if (vehicle != nullptr) {
        Fn_4CE960(vehicle + 0x1C4);
    }
    PS_Entity_45E690(ped);
    VCall<void>(ped, 0, 1);
}

// 0x5C7440: the ped faces the heading now, or (flag) as an objective
void __cdecl PedFaceHeadingImpl(uint8_t* ped, int flag, float heading) {
    if (flag == 0) {
        Ped_SetHeading(ped, heading);
        PS_Entity_443AA0(ped);
        PS_UpdateFrame(At<void*>(At<uint8_t*>(ped, 0x114), 0xC));
        return;
    }
    void* memory = PS_Malloc(0x18);
    void* objective = memory != nullptr ? FaceHeadingObjective_Construct(memory, ped, heading, 0) : nullptr;
    Objectives_Add(ped + 0x5F0, objective);
}

int __cdecl Native_PlayerFaceHeadingNow(lua_State* L) {
    float heading = float(double(Script::GetFloat(L, 0)) * 3.1415927410125732 / 180.0);
    uint8_t* ped = m_playerPed;
    Ped_SetHeading(ped, heading);
    PS_Entity_443AA0(ped);
    PS_UpdateFrame(At<void*>(At<uint8_t*>(ped, 0x114), 0xC));
    return 0;
}

// 0x5C7520: the ped faces the point now (flat), or (flag) as an objective
void __cdecl PedFacePointImpl(uint8_t* ped, int flag, const float* pos) {
    if (flag == 0) {
        float d[3];
        PS_VectorSub(d, pos, EntityPos(ped));
        d[2] = 0.0f;
        PS_Normalise(d);
        float heading = float(GetATanOfXY(d[0], d[1]) - 1.5707963705062866);
        if (heading < 0.0f) {
            heading = float(heading + 6.2831854820251465);
        }
        Ped_SetHeading(ped, heading);
        return;
    }
    void* memory = PS_Malloc(0x20);
    void* objective = memory != nullptr ? FacePositionObjective_Construct(memory, ped, pos) : nullptr;
    Objectives_Add(ped + 0x5F0, objective);
}

// 0x5C7600: puts the ped (its vehicle when in one) at the point, on its base (the ground
// for z at or below -20); the player through an area transition
void __cdecl WarpPed(uint8_t* ped, const float* pos) {
    if (ped == nullptr) {
        return;
    }
    float p[3] = {pos[0], pos[1], pos[2]};
    if (ps_groundZThreshold >= p[2]) {
        p[2] = PS_FindGroundZ2D(p[0], p[1], 1);
    }
    if (At<int32_t>(ped, 0x1310) == 0xD) {
        At<uint8_t>(g_areaTransitions, 0x8DC) = 1;
        float heading;
        uint8_t* matrix = At<uint8_t*>(ped, 0x14);
        if (matrix != nullptr) {
            heading = float(std::atan2(-At<float>(matrix, 0x10), At<float>(matrix, 0x14)));
        } else {
            heading = At<float>(ped, 0x10);
        }
        AreaTransitions_Request(reinterpret_cast<void*>(0x00BCC580), p, heading, &currArea);
        return;
    }
    uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
    if (vehicle != nullptr) {
        if (Scripts_GetCurrentOrNull(m_lurStore) != nullptr) {
            Scripts_GetCurrentOrNull(m_lurStore);
        }
        p[2] = PS_Entity_DistanceToBase(vehicle) + p[2];
        VCall<void>(vehicle, 0x38, p[0], p[1], p[2]);
        return;
    }
    p[2] = PS_Entity_DistanceToBase(ped) + p[2];
    VCall<void>(ped, 0x38, p[0], p[1], p[2]);
}

int __cdecl Native_PedDelete(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    DeletePedByHandle(handle, 2);
    Script_RemovePed(PS_CurrentScript(), handle);
    return 0;
}

// (ped): its action controller stopped, objectives cleared, idle
int __cdecl Native_PedStop(lua_State* L) {
    uint8_t* ped = M_GetPedFromHandle(Script::GetInt(L, 0), 2);
    if (ped == nullptr) {
        return 0;
    }
    ActionController_Stop(At<uint8_t*>(ped, 0x2E0) + 0x290);
    Objectives_Clear(ped + 0x5F0);
    void* memory = PS_Malloc(0x10);
    void* idle = memory != nullptr ? IdleObjective_Construct(memory, ped) : nullptr;
    Objectives_SetDefault(ped + 0x5F0, idle);
    return 0;
}

// (ped, heading, as objective)
int __cdecl Native_PedFaceHeading(lua_State* L) {
    uint8_t* ped = M_GetPedFromHandle(Script::GetInt(L, 0), 2);
    if (ped != nullptr) {
        float degrees = Script::GetFloat(L, 1);
        int flag = Script::GetInt(L, 2);
        PedFaceHeadingHelper(ped, flag, float(double(degrees) * 3.1415927410125732 / 180.0));
    }
    return 0;
}

// (ped, x, y, z, as objective)
int __cdecl Native_PedFaceXYZ(lua_State* L) {
    uint8_t* ped = M_GetPedFromHandle(Script::GetInt(L, 0), 2);
    if (ped != nullptr) {
        float x = Script::GetFloat(L, 1);
        float y = Script::GetFloat(L, 2);
        float z = Script::GetFloat(L, 3);
        int flag = Script::GetInt(L, 4);
        float pos[3] = {x, y, z};
        PedFacePointHelper(ped, flag, pos);
    }
    return 0;
}

// (ped, handle, kind, as objective, [flag = 1])
int __cdecl Native_PedFaceObject(lua_State* L) {
    uint8_t* ped = M_GetPedFromHandle(Script::GetInt(L, 0), 2);
    if (ped == nullptr) {
        return 0;
    }
    int handle = Script::GetInt(L, 1);
    int kind = Script::GetInt(L, 2);
    int asObjective = Script::GetInt(L, 3);
    uint8_t* target = PS_GetAreaEntity(handle, kind, 0, 0);
    if (target == nullptr) {
        return 0;
    }
    uint8_t flag = 1;
    if (lua::gettop(L) >= 5) {
        flag = Script::GetBool(L, 4);
    }
    if (asObjective == 0) {
        PedFacePointHelper(ped, 0, EntityPos(target));
        return 0;
    }
    void* memory = PS_Malloc(0x1C);
    void* objective = memory != nullptr ? FaceEntityObjective_Construct(memory, ped, target, flag) : nullptr;
    Objectives_Add(ped + 0x5F0, objective);
    return 0;
}

// (ped, handle, kind): faces it now (through a FaceEntityObjective on the stack)
int __cdecl Native_PedFaceObjectNow(lua_State* L) {
    uint8_t* ped = M_GetPedFromHandle(Script::GetInt(L, 0), 2);
    if (ped == nullptr) {
        return 0;
    }
    int handle = Script::GetInt(L, 1);
    int kind = Script::GetInt(L, 2);
    uint8_t* target = PS_GetAreaEntity(handle, kind, 0, 0);
    alignas(4) uint8_t objective[0x1C];
    FaceEntityObjective_Construct(objective, ped, target, 1);
    Ped_SetHeading(ped, FaceEntityObjective_GetHeading(objective));
    PS_Entity_443AA0(ped);
    PS_UpdateFrame(At<void*>(At<uint8_t*>(ped, 0x114), 0xC));
    FaceEntityObjective_Destruct(objective);
    return 0;
}

} // namespace

#define PEDSCRIPT_NATIVE(name, address)                                                \
    static decltype(&Native_##name) Original_PS_##name;                                  \
    REVERSED("Native_" #name, address, RE_BODY(&Native_##name), nullptr, \
             reinterpret_cast<void**>(&Original_PS_##name), re::Mode::Original)
#define PEDSCRIPT_REVERSED(name, address, fn)                                       \
    static void* RE_CONCAT(Original_PS_, address);                                  \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &RE_CONCAT(Original_PS_, address), \
             re::Mode::Original)

PEDSCRIPT_NATIVE(RequestModel, 0x005C6340);
PEDSCRIPT_NATIVE(WeaponRequestModel, 0x005C64A0);
PEDSCRIPT_REVERSED("ScriptNatives::IsPointInFront_5C65C0", 0x005C65C0, IsPointInFront);
PEDSCRIPT_NATIVE(PedFindRandomPed, 0x005C6710);
PEDSCRIPT_NATIVE(PedCharacterTriggerDelete, 0x005C67C0);
PEDSCRIPT_NATIVE(LoadActionTree, 0x005C69C0);
PEDSCRIPT_NATIVE(Load, 0x005C6A10);
PEDSCRIPT_NATIVE(PedOverrideActionTreeReference, 0x005C6A80);
PEDSCRIPT_NATIVE(PedSocialOverrideLoad, 0x005C6B80);
PEDSCRIPT_NATIVE(PedSetAIButes, 0x005C6D80);
RE_UNHOOKED("ScriptNatives::RoutePointArg_5C6E90", 0x005C6E90);
PEDSCRIPT_NATIVE(PedAddCover, 0x005C6FE0);
PEDSCRIPT_NATIVE(ModelNotNeeded, 0x005C71A0);
PEDSCRIPT_NATIVE(PedSpawnRandomPed, 0x005C71F0);
PEDSCRIPT_NATIVE(PedAreaClearOfPedsInXYZ, 0x005C72E0);
PEDSCRIPT_REVERSED("Script::GetPedFromHandle_5C7380", 0x005C7380, GetPedFromHandle);
PEDSCRIPT_REVERSED("Script::DeletePedByHandle_5C73E0", 0x005C73E0, DeletePedByHandleImpl);
PEDSCRIPT_REVERSED("Script::PedFaceHeading_5C7440", 0x005C7440, PedFaceHeadingImpl);
PEDSCRIPT_NATIVE(PlayerFaceHeadingNow, 0x005C74C0);
PEDSCRIPT_REVERSED("Script::PedFacePoint_5C7520", 0x005C7520, PedFacePointImpl);
PEDSCRIPT_REVERSED("Script::WarpPed_5C7600", 0x005C7600, WarpPed);
PEDSCRIPT_NATIVE(PedDelete, 0x005C7750);
PEDSCRIPT_NATIVE(PedStop, 0x005C7790);
PEDSCRIPT_NATIVE(PedFaceHeading, 0x005C7810);
PEDSCRIPT_NATIVE(PedFaceXYZ, 0x005C7880);
PEDSCRIPT_NATIVE(PedFaceObject, 0x005C7910);
PEDSCRIPT_NATIVE(PedFaceObjectNow, 0x005C7A10);
