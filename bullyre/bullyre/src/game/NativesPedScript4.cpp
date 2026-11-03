// Script natives 0x5CA520..0x5CD4B0 for peds: rectangles, health bars, mission
// characters, objectives, AI and action trees, branches, sight and targets, stimuli,
// cover, points of interest, damage, tethers, alertness, vehicles, pickups, fleeing,
// jumping and climbing. Each is `int __cdecl Native_X(lua_State* L)`, returning how
// many results it pushed. The ped's parts reached here: +0x2E0 its action part, +0x358
// its tether, +0x3BC its scanner, +0x5F0 its objectives, +0xA64 its AI.

#include <cmath>
#include <cstdint>
#include <cstring>

#include "game/NativesMPP.h"

#include "game/Crt.h"
namespace {

GAME_FN(void*, __cdecl, P4_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(uint8_t, __thiscall, P4_Objectives_Add, 0x00471390, void* manager, void* objective);
GAME_FN(void, __thiscall, P4_Objectives_Clear, 0x00470350, void* manager);
GAME_FN(uint8_t, __thiscall, Objectives_RemoveAt, 0x00470200, void* manager, int index);
GAME_FN(uint8_t*, __thiscall, Objectives_GetCurrent, 0x00470340, void* manager);
GAME_FN(void*, __thiscall, P4_Objective_Construct, 0x0046FA90, void* self, int type, void* ped);
GAME_FN(void, __thiscall, P4_ActionController_Stop, 0x005F40A0, void* controller);
GAME_VAR(uint8_t*, p4_healthBar, 0x00C674D0);
GAME_FN(void, __thiscall, HealthBar_SetPeds, 0x005537E0, void* bar, void* ped, void* other);
GAME_FN(void, __thiscall, P4_HealthBar_SetName, 0x005514D0, void* bar, const char* name);
GAME_FN(void, __thiscall, HealthBar_551500, 0x00551500, void* bar, const char* name);
GAME_FN(void, __thiscall, P4_Script_RemovePed, 0x005D9680, void* script, int handle);
GAME_FN(void, __thiscall, Script_AddPed, 0x005D9630, void* script, int handle);
GAME_FN(void, __thiscall, Ped_Set1570, 0x0047B7E0, void* ped, int state, int a, int b);
GAME_FN(void, __thiscall, Ped_474C10, 0x00474C10, void* ped, int, int);
GAME_FN(int, __thiscall, Gifts_For, 0x006A9A20, void* gifts, void* ped);
GAME_FN(void, __thiscall, Ped_SetGiftIndex, 0x00474B40, void* ped, int index);
GAME_FN(void, __thiscall, Ped_497520, 0x00497520, void* ped, void* attacker);
GAME_FN(void, __thiscall, P4_Scanner_ClearFocus, 0x0048C4C0, void* scanner, void* ped);
GAME_FN(void, __thiscall, Scanner_Ignore, 0x0048C0D0, void* scanner, void* ped);
GAME_FN(void, __thiscall, Scanner_Unignore, 0x0048C0F0, void* scanner, void* ped);
GAME_FN(void, __thiscall, Scanner_Alert, 0x0048C480, void* scanner, void* ped);
GAME_FN(void, __thiscall, Ped_SetAITree, 0x004740C0, void* ped, const char* a, const char* b);
GAME_FN(void, __thiscall, Ped_SetActionTree, 0x004740D0, void* ped, const char* a, const char* b);
GAME_FN(void, __thiscall, P4_ActFiles_Load, 0x005DC1A0, void* store, void* a, const void* b, const void* c, void* d);
inline void __cdecl P4_Memset(void* p, int value, uint32_t size) { crt::Memset(p, value, (size_t)size); }
GAME_VAR(void*, p4_pedStatsLike, 0x00C2C084);
GAME_FN(int, __thiscall, P4_Fn_49A0F0, 0x0049A0F0, void* object, const char*);
GAME_FN(void, __thiscall, Ped_SetStats, 0x00478F80, void* ped, uintptr_t stats, int);
GAME_FN(void*, __cdecl, P4_FindActionNode, 0x005F5B10, const char* path, uintptr_t);
GAME_FN(uint8_t, __thiscall, Script_UsesPropNode, 0x005D87D0, void* script, void* prop, void* node);
GAME_FN(void, __thiscall, Script_ReleasePropNode, 0x005D9230, void* script, void* prop);
GAME_FN(uint8_t, __thiscall, Script_UsesPedNode, 0x005D86F0, void* script, void* ped, void* node);
GAME_FN(void, __thiscall, Script_ReleasePedNode, 0x005D87B0, void* script, void* ped);
GAME_VAR(void*, g_actionTreeRoot, 0x01B649DC);
GAME_FN(void, __thiscall, ActFiles_Unload, 0x005DBF80, void* store, void* node);
GAME_FN(void, __cdecl, ActionFiles_Remove, 0x005F6500, const char* name);
GAME_FN(uint8_t*, __cdecl, P4_GetAreaEntity, 0x005C28D0, int handle, int kind, int noVehicle, int cutscene);
GAME_FN(uint8_t, __thiscall, Ai_CanSee, 0x0048D470, void* ai, void* entity);
GAME_FN(int32_t*, __cdecl, P4_MakeAttackKind, 0x00496AF0, int32_t* out, int first, int second);
GAME_FN(void, __thiscall, Ped_ClearTarget, 0x00498B10, void* ped, void*);
GAME_FN(void, __thiscall, Ped_SetTarget, 0x00497E70, void* ped, void* target, int kind, const float* offset);
GAME_FN(uint8_t*, __cdecl, P4_Object_GetProp, 0x004EA3C0, void* object);
GAME_FN(void*, __thiscall, BreakPropObjective_Construct, 0x00470E50, void* self, void* ped, void* prop, int value,
        float f10, float f14, float f18);
GAME_FN(uint8_t, __thiscall, P4_Objectives_Prepend, 0x00471470, void* manager, void* objective);
GAME_FN(float*, __thiscall, Placement_GetRight, 0x00473600, void* placeable, float* out);
GAME_FN(float*, __cdecl, P4_VectorAdd, 0x004129A0, float* out, const float* a, const float* b);
GAME_FN(float*, __cdecl, P4_VectorSub, 0x00414B70, float* out, const float* a, const float* b);
GAME_FN(int*, __thiscall, Ped_GetAttitude, 0x00475380, void* ped, int* out, int type);
GAME_FN(uint8_t*, __thiscall, Ped_GetTarget, 0x00497100, void* ped);
GAME_FN(uint8_t, __thiscall, ActionController_IsPlaying, 0x005F3D50, void* controller, const char* node, int);
GAME_FN(uint8_t, __thiscall, ActionController_IsPlayingNode, 0x005F49A0, void* controller, const char* node, int,
        int);
GAME_VAR(void*, g_stimuli, 0x00C2C1A0);
GAME_VAR(int32_t, g_stimulusTime, 0x00C1A9AC);
GAME_FN(uint8_t*, __thiscall, Stimuli_Find, 0x004A6150, void* self, void* ped, int type, int time);
GAME_FN(uint8_t*, __thiscall, Stimulus_GetSource, 0x004A5CD0, void* stimulus);
GAME_FN(uint8_t, __thiscall, Stimuli_RemoveAll, 0x004A67D0, void* self, void* ped);
GAME_FN(uint8_t, __thiscall, Stimuli_Remove, 0x004A6880, void* self, void* ped, int type);
GAME_FN(void*, __thiscall, Stimuli_4A61A0, 0x004A61A0, void* self, void* ped, int time);   // (the stimulus found, or null)
GAME_FN(float, __cdecl, Stimuli_4A5A90, 0x004A5A90);
inline void* const p4_routes = reinterpret_cast<void*>(0x00BC5E7C);
GAME_FN(void*, __thiscall, P4_Routes_Get, 0x006D6450, void* routes, int index);
GAME_FN(float*, __thiscall, P4_Route_First, 0x006D59B0, void* route);
GAME_FN(float*, __thiscall, P4_Route_Next, 0x006D59F0, void* route);
GAME_FN(uint8_t*, __thiscall, CoverObjective_Construct, 0x004710B0, void* self, void* ped, void* target, int value,
        void* route);
inline void* const p4_coverPoints = reinterpret_cast<void*>(0x00C2B210);
GAME_FN(void*, __thiscall, P4_CoverPoints_GetProfile, 0x00487F70, void* self, const char* name);
GAME_FN(uint8_t, __cdecl, P4_StoreCallback, 0x005D9FD0, lua_State* L, int i, void* callback);
GAME_FN(void, __cdecl, P4_StoreCallbackOpt, 0x005DA060, lua_State* L, int i, void* callback);
GAME_FN(void, __thiscall, P4_Ref_Release, 0x005D9C50, void* ref);
GAME_VAR(void*, g_poiManager, 0x00C0F5E4);
GAME_FN(uint8_t*, __thiscall, POIManager_GetPOI, 0x00448DC0, void* manager, int id);
GAME_FN(uint8_t, __thiscall, POI_447410, 0x00447410, void* poi, void* ped, int flag);
GAME_FN(uint8_t, __thiscall, Ped_TakeDamage, 0x0047B800, void* ped, float damage, uintptr_t);
GAME_FN(void, __thiscall, Tether_SetPoint, 0x00499A60, void* tether, const float* point, float radius);
GAME_FN(void, __thiscall, Tether_SetEntity, 0x00499AB0, void* tether, void* entity, float radius, const float* def);
GAME_FN(void, __thiscall, AntiTether_SetPoint, 0x00499B90, void* tether, const float* point, float radius);
GAME_FN(void, __thiscall, AntiTether_SetEntity, 0x00499BD0, void* tether, void* entity, float radius, const float* def);
GAME_ARRAY(float, p4_defaultPosition, 0x00BCC564);
GAME_VAR(uint32_t, g_timeInMs, 0x00C1A9B4);
GAME_FN(uint8_t*, __cdecl, P4_GetVehicle, 0x0044A580, int handle);
GAME_FN(void, __thiscall, Ped_WarpIntoCar, 0x0049A470, void* ped, void* vehicle, int seat);
GAME_FN(void, __thiscall, Targeting_SetVehicle, 0x004D79E0, void* targeting, void* vehicle);
GAME_FN(void*, __thiscall, FleeObjective_Construct, 0x0048E700, void* self, void* ped, void* from);
GAME_FN(void*, __thiscall, P4_JumpObjective_Construct, 0x005C61D0, void* self, void* ped, const float* target, float a,
        float b);
GAME_FN(float, __cdecl, P4_AngleToHeading, 0x004333A0, float angle);

uint8_t* P4_Ped(lua_State* L, int i) {
    return M_GetPedFromHandle(Script::GetInt(L, i), 2);
}

void* P4_CurrentScript() {
    return M_Scripts_GetCurrent(m_lurStore);
}

// The damage the game deals through a ped's virtual +0x3C
void DealDamage(uint8_t* ped, float damage) {
    VCall<void>(ped, 0x3C, damage, 0.0f, 0, -1, 0, 0, 0, 0, 0, 0, 0.0f, 0.0f, 0, 1, 0, -1, 0.0f, 0);
}

// 0x5C6E90's work (its registers can't be called from here): the route (argument i) n-th
// point and heading
void P4_RoutePoint(lua_State* L, int i, float* pos, float* heading, int n) {
    void* route = P4_Routes_Get(p4_routes, Script::GetInt(L, i));
    float* p = P4_Route_First(route);
    for (int k = 1; k < n;) {
        float* next = P4_Route_Next(route);
        if (next == nullptr) {
            break;
        }
        k++;
        p = next;
    }
    pos[0] = p[0];
    pos[1] = p[1];
    pos[2] = p[2];
    *heading = P4_AngleToHeading(float(double(p[7]) * 3.1415927410125732 / 180.0));
}

// (ped (0 the player), x1, y1, x2, y2): the ped is in the rectangle, remembered at
// 0xCFB38C (min x, min y, 0, max x, max y, 0)
int __cdecl Native_PedInRectangle(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    uint8_t* ped = M_GetPedFromHandle(handle, handle != 0 ? 2 : 3);
    float* p = EntityPos(ped);
    float pos[3] = {p[0], p[1], p[2]};
    float a = Script::GetFloat(L, 1);
    float b = Script::GetFloat(L, 3);
    float minX = b > a ? a : b;
    a = Script::GetFloat(L, 2);
    b = Script::GetFloat(L, 4);
    float minY = b > a ? a : b;
    a = Script::GetFloat(L, 1);
    b = Script::GetFloat(L, 3);
    float maxX = b < a ? a : b;
    a = Script::GetFloat(L, 2);
    b = Script::GetFloat(L, 4);
    float maxY = b < a ? a : b;
    float* rect = reinterpret_cast<float*>(0x00CFB38C);
    rect[2] = 0.0f;
    rect[5] = 0.0f;
    rect[0] = minX;
    rect[1] = minY;
    rect[3] = maxX;
    rect[4] = maxY;
    uint8_t in = minX <= pos[0] && maxX >= pos[0] && minY <= pos[1] && maxY >= pos[1];
    Script::PushBool(L, in);
    return 1;
}

// (ped, show, [name], [flag], [other ped], [name 2])
int __cdecl Native_PedShowHealthBar(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    uint8_t* other = nullptr;
    if (lua::gettop(L) > 4) {
        other = P4_Ped(L, 4);
    }
    if (ped == nullptr) {
        return 0;
    }
    if (Script::GetBool(L, 1) == 0) {
        At<int32_t>(ped, 0x135C) = 1;
        if (other != nullptr) {
            At<int32_t>(other, 0x135C) = 1;
        }
        return 0;
    }
    At<int32_t>(ped, 0x135C) = 0;
    uint8_t* bar = p4_healthBar;
    HealthBar_SetPeds(bar, ped, other);
    VCall<void>(bar, 0x14, 1);
    P4_HealthBar_SetName(bar, lua::gettop(L) < 3 ? nullptr : Script::GetString(L, 2));
    if (lua::gettop(L) < 4) {
        At<uint8_t>(bar, 0xBA) = 0;
        At<uint8_t>(bar, 0xBB) = 0;
    } else {
        uint8_t flag = Script::GetBool(L, 3);
        At<uint8_t>(bar, 0xBA) = flag;
        if (flag == 0) {
            At<uint8_t>(bar, 0xBB) = 0;
        }
    }
    if (lua::gettop(L) > 5) {
        HealthBar_551500(bar, Script::GetString(L, 5));
        return 0;
    }
    HealthBar_551500(bar, nullptr);
    return 0;
}

// (value, name, [flag]): a health bar showing a number (+0x178)
int __cdecl Native_ShowGeneralHealthBar(lua_State* L) {
    int value = Script::GetInt(L, 0);
    uint8_t* bar = p4_healthBar;
    At<float>(bar, 0x178) = float(value);
    At<uint8_t>(bar, 0x174) = 1;
    VCall<void>(bar, 0x14, 1);
    P4_HealthBar_SetName(bar, Script::GetString(L, 1));
    if (lua::gettop(L) < 3) {
        At<uint8_t>(bar, 0xBA) = 0;
        At<uint8_t>(bar, 0xBB) = 0;
    } else {
        uint8_t flag = Script::GetBool(L, 2);
        At<uint8_t>(bar, 0xBA) = flag;
        if (flag == 0) {
            At<uint8_t>(bar, 0xBB) = 0;
        }
    }
    HealthBar_551500(bar, nullptr);
    return 0;
}

// 0x5CA910 (L, ped handle, a): the ped handed back to the population (no longer the
// script's; its gift, argument 1 (default 1), from the gifts)
void __cdecl ReleasePedToAmbient(lua_State* L, int handle, int a) {
    uint8_t* ped = M_GetPedFromHandle(handle, 2);
    if (ped == nullptr) {
        return;
    }
    P4_Script_RemovePed(P4_CurrentScript(), handle);
    uint8_t gift = Script::GetBoolOpt(L, 1, 1);
    Ped_Set1570(ped, 1, a, gift);
    Ped_474C10(ped, 1, 0);
    Ped_SetGiftIndex(ped, gift != 0 ? Gifts_For(reinterpret_cast<void*>(0x020C3698), ped) : -1);
    At<int32_t>(ped, 0x1B8) = 0;
}

int __cdecl Native_PedMakeMissionChar(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    uint8_t* ped = M_GetPedFromHandle(handle, 2);
    if (ped != nullptr) {
        Ped_Set1570(ped, 2, 1, 1);
        Script_AddPed(P4_CurrentScript(), handle);
    }
    return 0;
}

int __cdecl Native_PedClearObjectives(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    if (ped != nullptr) {
        P4_Objectives_Clear(ped + 0x5F0);
        Ped_497520(ped, nullptr);
        P4_ActionController_Stop(At<uint8_t*>(ped, 0x2E0) + 0x290);
    }
    return 0;
}

// (ped, index)
int __cdecl Native_PedClearObjective(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    int index = Script::GetInt(L, 1);
    uint8_t* ped = M_GetPedFromHandle(handle, 2);
    if (ped != nullptr) {
        Objectives_RemoveAt(ped + 0x5F0, index);
        Ped_497520(ped, nullptr);
    }
    return 0;
}

// (ped, focus ped): the ped's scanner loses the other
int __cdecl Native_PedSetFocus(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    int other = Script::GetInt(L, 1);
    uint8_t* ped = M_GetPedFromHandle(handle, 2);
    if (ped != nullptr) {
        uint8_t* focus = M_GetPedFromHandle(other, 2);
        if (focus != nullptr) {
            P4_Scanner_ClearFocus(ped + 0x3BC, focus);
        }
    }
    return 0;
}

// (ped, tree, file). (Copied unbounded into 256 bytes each)
int __cdecl Native_PedSetAITree(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    char tree[256];
    char file[256];
    std::strcpy(tree, Script::GetString(L, 1));
    std::strcpy(file, Script::GetString(L, 2));
    Ped_SetAITree(ped, tree, file);
    if (std::strlen(tree) != 0 && std::strlen(file) != 0) {
        P4_ActFiles_Load(m_lurStore, ped, tree, file, nullptr);
    }
    return 0;
}

// (ped, stats name)
int __cdecl Native_PedSetStatsType(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    if (ped != nullptr) {
        char name[256];
        std::strcpy(name, Script::GetString(L, 1));
        Ped_SetStats(ped, P4_Fn_49A0F0(p4_pedStatsLike, name), 0);
    }
    return 0;
}

// (ped (0 the player), tree, file). (A missing ped is called through)
int __cdecl Native_PedSetActionTree(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    uint8_t* ped = M_GetPedFromHandle(handle, handle != 0 ? 2 : 3);
    char tree[256];
    char file[256];
    tree[0] = '\0';
    tree[1] = '\0';
    P4_Memset(tree + 2, 0, 0xFD);
    file[0] = '\0';
    file[1] = '\0';
    P4_Memset(file + 2, 0, 0xFD);
    if (*Script::GetString(L, 1) != '\0') {
        std::strcpy(tree, Script::GetString(L, 1));
    }
    if (*Script::GetString(L, 2) != '\0') {
        std::strcpy(file, Script::GetString(L, 2));
    }
    Ped_SetActionTree(ped, tree, file);
    if (std::strlen(tree) != 0 && std::strlen(file) != 0) {
        P4_ActFiles_Load(m_lurStore, ped, tree, file, nullptr);
    }
    return 0;
}

// (node path): props and peds of the script using it let go; the branch (up to below
// the root) unloaded
int __cdecl Native_UnLoadBranch(lua_State* L) {
    char path[256];
    std::strcpy(path, Script::GetString(L, 0));
    uint8_t* node = static_cast<uint8_t*>(P4_FindActionNode(path, 0));
    if (node == nullptr) {
        return 0;
    }
    CPool<uint8_t>* props = m_propAnimPool;
    int n = props->size;
    for (int i = 0; i < n; i++) {
        if ((props->flags[i] & 0x80) != 0) {
            continue;
        }
        uint8_t* prop = props->objects + props->elemSize * i;
        if (prop == nullptr) {
            continue;
        }
        if (Script_UsesPropNode(P4_CurrentScript(), prop, node) != 0) {
            Script_ReleasePropNode(P4_CurrentScript(), prop);
        }
    }
    CPool<uint8_t>* peds = m_pedPool;
    n = peds->size;
    for (int i = 0; i < n; i++) {
        if ((peds->flags[i] & 0x80) != 0) {
            continue;
        }
        uint8_t* ped = peds->objects + peds->elemSize * i;
        if (ped == nullptr) {
            continue;
        }
        if (Script_UsesPedNode(P4_CurrentScript(), ped, node) != 0) {
            Script_ReleasePedNode(P4_CurrentScript(), ped);
        }
    }
    uint8_t* top = node;
    while (node != g_actionTreeRoot) {
        top = node;
        node = VCall<uint8_t*>(node, 0x34, -1);
    }
    ActFiles_Unload(m_lurStore, top);
    ActionFiles_Remove(path);
    return 0;
}

// (ped, handle, kind) -> its AI can see it; nothing for no ped
int __cdecl Native_PedCanSeeObject(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    int handle = Script::GetInt(L, 1);
    int kind = Script::GetInt(L, 2);
    uint8_t* e = P4_GetAreaEntity(handle, kind, 0, 0);
    Script::PushBool(L, Ai_CanSee(ped + 0xA64, e));
    return 1;
}

// (ped, target (-1 none), kind | [second])
int __cdecl Native_PedLockTarget(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    int target = Script::GetInt(L, 1);
    int kind;
    if (Script::IsNumber(L, 2) != 0) {
        kind = Script::GetInt(L, 2);
    } else {
        uint8_t second = Script::GetBoolOpt(L, 2, 1);
        int32_t made;
        kind = *P4_MakeAttackKind(&made, 1, second);
    }
    uint8_t* ped = M_GetPedFromHandle(handle, 2);
    if (ped == nullptr) {
        return 0;
    }
    if (target == -1) {
        Ped_ClearTarget(ped, nullptr);
        return 0;
    }
    uint8_t* other = M_GetPedFromHandle(target, 2);
    if (other != nullptr) {
        float offset[3] = {0.0f, 0.0f, 0.0f};
        Ped_SetTarget(ped, other, kind, offset);
    }
    return 0;
}

// (ped, trigger, [value = 3], [first], [f10], [f11], [f12]): breaks the trigger's prop
int __cdecl Native_PedAttackProp(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    uint8_t* prop = P4_Object_GetProp(PoolSlot(m_triggerPool, Script::GetInt(L, 1)));
    if (prop == nullptr) {
        return 0;
    }
    uint8_t first = 0;
    int value = 3;
    if (Script::IsNumber(L, 2) != 0) {
        value = Script::GetInt(L, 2);
        first = Script::GetBoolOpt(L, 3, 0);
    }
    void* memory = P4_Malloc(0x28);
    void* objective = nullptr;
    if (memory != nullptr) {
        float f6 = Script::GetFloatOpt(L, 6, 0.0f);
        float f5 = Script::GetFloatOpt(L, 5, 0.0f);
        float f4 = Script::GetFloatOpt(L, 4, 0.0f);
        objective = BreakPropObjective_Construct(memory, ped, prop, value, f4, f5, f6);
    }
    if (first != 0) {
        P4_Objectives_Prepend(ped + 0x5F0, objective);
        return 0;
    }
    P4_Objectives_Add(ped + 0x5F0, objective);
    return 0;
}

// (ped, trigger, right, up): breaks the prop aiming at the ped's right * a + up * b
int __cdecl Native_PedAttackPropOffset(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    uint8_t* prop = P4_Object_GetProp(PoolSlot(m_triggerPool, Script::GetInt(L, 1)));
    if (prop == nullptr) {
        return 0;
    }
    float a = Script::GetFloat(L, 2);
    float b = Script::GetFloat(L, 3);
    float axis[3];
    float* matrix = At<float*>(ped, 0x14);
    if (matrix == nullptr) {
        Placement_GetRight(ped + 4, axis);
    } else {
        axis[0] = matrix[0];
        axis[1] = matrix[1];
        axis[2] = matrix[2];
    }
    float right[3] = {a * axis[0], axis[1] * a, a * axis[2]};
    matrix = At<float*>(ped, 0x14);
    if (matrix == nullptr) {
        axis[0] = 0.0f;
        axis[1] = 0.0f;
        axis[2] = 1.0f;
    } else {
        axis[0] = matrix[8];
        axis[1] = matrix[9];
        axis[2] = matrix[10];
    }
    float up[3] = {b * axis[0], axis[1] * b, b * axis[2]};
    float offset[3];
    P4_VectorAdd(offset, right, up);
    void* memory = P4_Malloc(0x28);
    void* objective = memory != nullptr ? BreakPropObjective_Construct(memory, ped, prop, 3, offset[0], offset[1], offset[2])
                                        : nullptr;
    P4_Objectives_Add(ped + 0x5F0, objective);
    return 0;
}

// (ped, ped type) -> its attitude (2 without a ped)
int __cdecl Native_PedGetPedToTypeAttitude(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    if (ped == nullptr) {
        Script::PushInt(L, 2);
        return 1;
    }
    int type = Script::GetInt(L, 1);
    int attitude;
    Ped_GetAttitude(ped, &attitude, type);
    Script::PushInt(L, attitude);
    return 1;
}

// (ped) -> its target ped, -1
int __cdecl Native_PedGetTargetPed(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    int handle = -1;
    uint8_t* target = Ped_GetTarget(ped);
    if (target != nullptr && (At<uint8_t>(target, 0x108) & 7) == 3) {
        handle = VCall<int>(target, 0xAC);
    }
    Script::PushInt(L, handle);
    return 1;
}

int __cdecl Native_PedAddPedToIgnoreList(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    int other = Script::GetInt(L, 1);
    uint8_t* ped = M_GetPedFromHandle(handle, 2);
    uint8_t* ignored = M_GetPedFromHandle(other, 2);
    if (ped != nullptr && ignored != nullptr) {
        Scanner_Ignore(ped + 0x3BC, ignored);
    }
    return 0;
}

int __cdecl Native_PedRemovePedFromIgnoreList(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    int other = Script::GetInt(L, 1);
    uint8_t* ped = M_GetPedFromHandle(handle, 2);
    uint8_t* ignored = M_GetPedFromHandle(other, 2);
    if (ped != nullptr && ignored != nullptr) {
        Scanner_Unignore(ped + 0x3BC, ignored);
    }
    return 0;
}

// (ped (0 the player), node, flag). (A missing ped is read through)
int __cdecl Native_PedIsDoingTask(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    uint8_t* part = At<uint8_t*>(M_GetPedFromHandle(handle, handle != 0 ? 2 : 3), 0x2E0);
    uint8_t flag = Script::GetBool(L, 2);
    const char* node = Script::GetString(L, 1);
    Script::PushBool(L, ActionController_IsPlaying(part + 0x290, node, flag));
    return 1;
}

int __cdecl Native_PedMePlaying(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    uint8_t* part = At<uint8_t*>(M_GetPedFromHandle(handle, handle != 0 ? 2 : 3), 0x2E0);
    uint8_t flag = Script::GetBool(L, 2);
    const char* node = Script::GetString(L, 1);
    Script::PushBool(L, ActionController_IsPlayingNode(part + 0x258, node, 1, flag));
    return 1;
}

// (ped, ped type): it just killed one (a stimulus 7 now, from a ped of the type)
int __cdecl Native_PedHasJustKilledPedOfType(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    int type = Script::GetInt(L, 1);
    uint8_t* stimulus = Stimuli_Find(g_stimuli, ped, 7, g_stimulusTime);
    if (stimulus != nullptr) {
        uint8_t* source = Stimulus_GetSource(stimulus);
        if (source != nullptr && At<int32_t>(source, 0x1310) == type) {
            Script::PushBool(L, 1);
            return 1;
        }
    }
    Script::PushBool(L, 0);
    return 1;
}

// (ped, type) -> (generated, its source's handle)
int __cdecl Native_PedHasGeneratedStimulusOfType(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    int type = Script::GetInt(L, 1);
    uint8_t* stimulus = Stimuli_Find(g_stimuli, ped, type, -1);
    if (stimulus == nullptr) {
        Script::PushBool(L, 0);
        return 1;
    }
    uint8_t* source = Stimulus_GetSource(stimulus);
    Script::PushBool(L, 1);
    if (source != nullptr) {
        // (0x5C6250, the source in ecx)
        Script::PushInt(L, VCall<int>(source, 0xAC));
        return 2;
    }
    return 1;
}

// (ped, [type]) -> removed
int __cdecl Native_PedRemoveStimulus(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    uint8_t removed;
    if (lua::gettop(L) < 2) {
        removed = Stimuli_RemoveAll(g_stimuli, ped);
    } else {
        removed = Stimuli_Remove(g_stimuli, ped, Script::GetInt(L, 1));
    }
    Script::PushBool(L, removed);
    return 1;
}

// The cover target of argument 1 (a number: 0 the player), else none; profile: -1 none
uint8_t* CoverTarget(lua_State* L, bool negativeIsNone) {
    if (Script::IsNumber(L, 1) == 0) {
        return nullptr;
    }
    int handle = Script::GetInt(L, 1);
    if (negativeIsNone && handle < 0) {
        return nullptr;
    }
    return handle != 0 ? M_GetPedFromHandle(handle, 2) : static_cast<uint8_t*>(m_playerPed);
}

// (ped, target, route, 7 floats, 4 ints, flag). (A failed allocation is written through)
int __cdecl Native_PedCoverSet(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    void* route = P4_Routes_Get(p4_routes, uint16_t(Script::GetInt(L, 2)));
    if (route == nullptr) {
        return 0;
    }
    uint8_t* target = CoverTarget(L, false);
    void* memory = P4_Malloc(0x6C);
    uint8_t* objective = memory != nullptr ? CoverObjective_Construct(memory, ped, target, 3, route) : nullptr;
    for (int i = 0; i < 7; i++) {
        At<float>(objective, 0x38 + i * 4) = Script::GetFloat(L, 3 + i);
    }
    for (int i = 0; i < 4; i++) {
        At<int32_t>(objective, 0x54 + i * 4) = Script::GetInt(L, 10 + i);
    }
    At<uint8_t>(objective, 0x64) = Script::GetBool(L, 14);
    P4_Objectives_Add(ped + 0x5F0, objective);
    return 0;
}

// (ped, target, route, profile name). (Copied unbounded into 24 bytes)
int __cdecl Native_PedCoverSetFromProfile(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    void* route = P4_Routes_Get(p4_routes, uint16_t(Script::GetInt(L, 2)));
    if (route == nullptr) {
        return 0;
    }
    uint8_t* target = CoverTarget(L, true);
    char name[24];
    std::strcpy(name, Script::GetString(L, 3));
    void* memory = P4_Malloc(0x6C);
    uint8_t* objective = memory != nullptr ? CoverObjective_Construct(memory, ped, target, 3, route) : nullptr;
    const uint32_t* profile = static_cast<const uint32_t*>(P4_CoverPoints_GetProfile(p4_coverPoints, name));
    for (int i = 0; i < 0x12; i++) {
        At<uint32_t>(objective, 0x20 + i * 4) = profile[i];
    }
    P4_Objectives_Add(ped + 0x5F0, objective);
    return 0;
}

// (ped, function): the current cover objective's (type 0x10) callback (+0x68)
int __cdecl Native_PedCoverSetOnCoverCallback(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    if (ped != nullptr) {
        uint8_t* current = Objectives_GetCurrent(ped + 0x5F0);
        if (current != nullptr && At<int32_t>(current, 8) == 0x10) {
            P4_StoreCallback(L, 1, current + 0x68);
        }
    }
    return 0;
}

// (ped, poi, [flag = 1]): only for points of interest in modes -1 < m < 3, 3, m >= 16
int __cdecl Native_PedSetPOI(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    uint8_t* poi = POIManager_GetPOI(g_poiManager, Script::GetInt(L, 1));
    int mode = At<int32_t>(poi, 0x70);
    if (mode != -1 && (mode <= 2 || mode >= 0x10 || mode == 3)) {
        uint8_t flag = Script::GetBoolOpt(L, 2, 1);
        POI_447410(poi, ped, flag);
    }
    return 0;
}

// (ped, damage): killed (100 more) when down to 1e-6 health
int __cdecl Native_PedApplyDamage(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    float damage = Script::GetFloat(L, 1);
    uint8_t* ped = M_GetPedFromHandle(handle, 2);
    if (ped != nullptr) {
        Ped_TakeDamage(ped, damage, 0);
        if (!(9.9999997e-07f < At<float>(ped, 0x1CB8))) {
            DealDamage(ped, 100.0f);
        }
    }
    return 0;
}

int __cdecl Native_PedPlayHitReaction(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    float damage = Script::GetFloat(L, 1);
    uint8_t* ped = M_GetPedFromHandle(handle, 2);
    if (ped != nullptr) {
        Ped_TakeDamage(ped, damage, 0);
        DealDamage(ped, damage);
    }
    return 0;
}

// (ped, x, y, z, radius)
int __cdecl Native_PedSetTetherToXYZ(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    ScriptVector pos;
    Script::GetVector(&pos, L, 1);
    float radius = Script::GetFloat(L, 4);
    if (ped != nullptr) {
        Tether_SetPoint(ped + 0x358, &pos.x, radius);
    }
    return 0;
}

// (ped, route, [point], radius): to the route's point
template <typename F>
int TetherToPoint(lua_State* L, F set) {
    uint8_t* ped = P4_Ped(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    uint16_t index = uint16_t(Script::GetInt(L, 1));
    bool numbered = lua::gettop(L) == 4;
    int n = 0;   // (uninitialised in the game when not given; unused then)
    if (numbered) {
        n = Script::GetInt(L, 2);
    }
    void* route = P4_Routes_Get(p4_routes, index);
    if (route == nullptr) {
        return 0;
    }
    float* p = P4_Route_First(route);
    if (numbered) {
        for (int i = 1; i < n;) {
            float* next = P4_Route_Next(route);
            if (next == nullptr) {
                break;
            }
            i++;
            p = next;
        }
    }
    if (p == nullptr) {
        return 0;
    }
    float pos[3] = {p[0], p[1], p[2]};
    float radius = Script::GetFloat(L, numbered ? 3 : 2);
    set(ped + 0x358, pos, radius);
    return 0;
}

int __cdecl Native_PedSetTetherToPoint(lua_State* L) {
    return TetherToPoint(L, Tether_SetPoint);
}

// (ped, other ped, radius)
int __cdecl Native_PedSetTetherToPed(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    if (ped != nullptr) {
        uint8_t* other = P4_Ped(L, 1);
        if (other != nullptr) {
            Tether_SetEntity(ped + 0x358, other, Script::GetFloat(L, 2), p4_defaultPosition);
        }
    }
    return 0;
}

// (ped, object, radius)
int __cdecl Native_PedSetTetherToProp(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    if (ped != nullptr) {
        uint8_t* prop = P4_GetAreaEntity(Script::GetInt(L, 1), 0, 0, 0);
        if (prop != nullptr) {
            Tether_SetEntity(ped + 0x358, prop, Script::GetFloat(L, 2), p4_defaultPosition);
        }
    }
    return 0;
}

int __cdecl Native_PedSetAntiTetherToXYZ(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    ScriptVector pos;
    Script::GetVector(&pos, L, 1);
    float radius = Script::GetFloat(L, 4);
    if (ped != nullptr) {
        AntiTether_SetPoint(ped + 0x358, &pos.x, radius);
    }
    return 0;
}

int __cdecl Native_PedSetAntiTetherToPoint(lua_State* L) {
    return TetherToPoint(L, AntiTether_SetPoint);
}

int __cdecl Native_PedSetAntiTetherToPed(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    if (ped != nullptr) {
        uint8_t* other = P4_Ped(L, 1);
        if (other != nullptr) {
            AntiTether_SetEntity(ped + 0x358, other, Script::GetFloat(L, 2), p4_defaultPosition);
        }
    }
    return 0;
}

int __cdecl Native_PedSetAntiTetherToProp(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    if (ped != nullptr) {
        uint8_t* prop = P4_GetAreaEntity(Script::GetInt(L, 1), 0, 0, 0);
        if (prop != nullptr) {
            AntiTether_SetEntity(ped + 0x358, prop, Script::GetFloat(L, 2), p4_defaultPosition);
        }
    }
    return 0;
}

// (ped, critical, [callback], [flag]): +0x141C, +0x18E8, +0x1420 (the callback released
// when not critical)
int __cdecl Native_PedSetMissionCritical(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    if (ped != nullptr) {
        At<uint32_t>(ped, 0x141C) = Script::GetBool(L, 1);
        P4_StoreCallbackOpt(L, 2, ped + 0x18E8);
        At<uint32_t>(ped, 0x1420) = Script::GetBoolOpt(L, 3, 0);
        if (At<uint32_t>(ped, 0x141C) == 0) {
            P4_Ref_Release(ped + 0x18E8);
        }
    }
    return 0;
}

// (ped, [delay], [time]) -> alerted by the player: seen for the delay (ms, +0x4D8 when
// first seen), or nearer than 0x4A5A90's range (head to head), or a stimulus. (The time
// is argument 1 again; with no ped it pushes false and returns 0)
int __cdecl Native_PedIsAlerted(lua_State* L) {
    int delay = 0;
    uint8_t* ped = P4_Ped(L, 0);
    uint8_t* player = m_playerPed;
    if (ped == nullptr) {
        Script::PushBool(L, 0);
        return 0;
    }
    if (lua::gettop(L) >= 2) {
        delay = Script::GetInt(L, 1);
    }
    uint8_t alerted;
    if (Ai_CanSee(ped + 0xA64, player) != 0) {
        if (At<uint32_t>(ped, 0x4D8) == 0) {
            At<uint32_t>(ped, 0x4D8) = g_timeInMs;
        }
        if (g_timeInMs >= At<uint32_t>(ped, 0x4D8) + uint32_t(delay)) {
            Scanner_Alert(ped + 0x3BC, player);
            Script::PushBool(L, 1);
            return 1;
        }
    } else {
        At<uint32_t>(ped, 0x4D8) = 0;
    }
    player = m_playerPed;
    float* p = EntityPos(ped);
    float head[3] = {p[0], p[1], float(p[2] + 2.0)};
    p = EntityPos(player);
    float playerHead[3] = {p[0], p[1], float(p[2] + 1.0)};
    float d[3];
    P4_VectorSub(d, playerHead, head);
    float distance = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
    distance = std::sqrt(std::fabs(distance));
    float range = Stimuli_4A5A90();
    if (distance < range) {
        Scanner_Alert(ped + 0x3BC, player);
        alerted = 1;
    } else {
        int time = 200;
        if (lua::gettop(L) >= 3) {
            time = Script::GetInt(L, 1);
        }
        alerted = Stimuli_4A61A0(g_stimuli, ped, time) != nullptr;
    }
    Script::PushBool(L, alerted);
    return 1;
}

// (ped, vehicle, [seat = 0])
int __cdecl Native_PedWarpIntoCar(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    if (ped != nullptr) {
        uint8_t* vehicle = P4_GetVehicle(Script::GetInt(L, 1));
        int seat = Script::GetIntOpt(L, 2, 0);
        Ped_WarpIntoCar(ped, vehicle, seat);
    }
    return 0;
}

// (ped, pickup): a pickup objective (type 0x17, vtable 0x911DC4)
int __cdecl Native_PedPickup(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    int pickup = Script::GetInt(L, 1);
    if (ped == nullptr) {
        return 0;
    }
    uint8_t* objective = static_cast<uint8_t*>(P4_Malloc(0x14));
    if (objective != nullptr) {
        P4_Objective_Construct(objective, 0x17, ped);
        At<uint32_t>(objective, 0) = 0x00911DC4;
        At<int32_t>(objective, 0x10) = pickup;
    }
    P4_Objectives_Add(ped + 0x5F0, objective);
    return 0;
}

// (ped, vehicle (-1 none), kind | [second])
int __cdecl Native_PedTargetVehicle(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    int target = Script::GetInt(L, 1);
    int kind;
    if (Script::IsNumber(L, 2) != 0) {
        kind = Script::GetInt(L, 2);
    } else {
        uint8_t second = Script::GetBoolOpt(L, 2, 1);
        int32_t made;
        kind = *P4_MakeAttackKind(&made, 1, second);
    }
    uint8_t* ped = M_GetPedFromHandle(handle, 2);
    if (ped == nullptr) {
        return 0;
    }
    uint8_t* vehicle = nullptr;
    if (target != -1) {
        vehicle = P4_GetVehicle(target);
        if (vehicle == nullptr) {
            return 0;
        }
        float offset[3] = {0.0f, 0.0f, 0.0f};
        Ped_SetTarget(ped, vehicle, kind, offset);
    }
    Targeting_SetVehicle(At<void*>(ped, 0x1C8), vehicle);
    return 0;
}

// (ped, vehicle (-1 any)) -> it stands on it (+0x1758)
int __cdecl Native_PedIsStandingOnVehicle(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    int target = Script::GetInt(L, 1);
    uint8_t on = 0;
    uint8_t* ped = M_GetPedFromHandle(handle, 2);
    if (ped != nullptr) {
        uint8_t* standing = At<uint8_t*>(ped, 0x1758);
        if (target == -1) {
            on = standing != nullptr && (At<uint8_t>(standing, 0x108) & 7) == 2;
        } else {
            uint8_t* vehicle = P4_GetVehicle(target);
            if (vehicle != nullptr) {
                on = standing != nullptr && standing == vehicle;
            }
        }
    }
    Script::PushBool(L, on);
    return 1;
}

// (ped, from ped)
int __cdecl Native_PedFlee(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    uint8_t* ped = M_GetPedFromHandle(handle, 2);
    uint8_t* from = P4_Ped(L, 1);
    if (ped != nullptr && from != nullptr) {
        void* memory = P4_Malloc(0x14);
        void* objective = memory != nullptr ? FleeObjective_Construct(memory, ped, from) : nullptr;
        P4_Objectives_Add(ped + 0x5F0, objective);
    }
    return 0;
}

// (ped, route, [height = 1.5], [point = 1]): jumps to the route's point. (No ped check)
int __cdecl Native_PedJump(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    float pos[3] = {0.0f, 0.0f, 0.0f};
    float heading;
    int n = Script::GetIntOpt(L, 3, 1);
    P4_RoutePoint(L, 1, pos, &heading, n);
    float height = Script::GetFloatOpt(L, 2, 1.5f);
    void* memory = P4_Malloc(0x34);
    void* objective = memory != nullptr ? P4_JumpObjective_Construct(memory, ped, pos, heading, height) : nullptr;
    P4_Objectives_Add(ped + 0x5F0, objective);
    return 0;
}

// (ped, route, [point = 1]): a jump of 1.5
int __cdecl Native_PedClimbWall(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    float pos[3] = {0.0f, 0.0f, 0.0f};
    float heading;
    int n = Script::GetIntOpt(L, 2, 1);
    P4_RoutePoint(L, 1, pos, &heading, n);
    void* memory = P4_Malloc(0x34);
    void* objective = memory != nullptr ? P4_JumpObjective_Construct(memory, ped, pos, heading, 1.5f) : nullptr;
    P4_Objectives_Add(ped + 0x5F0, objective);
    return 0;
}

// (ped, route): a ladder objective (type 0x1C, vtable 0x928A8C) to the route's first
// point
int __cdecl Native_PedClimbLadder(lua_State* L) {
    uint8_t* ped = P4_Ped(L, 0);
    float pos[3] = {0.0f, 0.0f, 0.0f};
    float heading;
    P4_RoutePoint(L, 1, pos, &heading, 1);
    uint8_t* objective = static_cast<uint8_t*>(P4_Malloc(0x20));
    if (objective != nullptr) {
        P4_Objective_Construct(objective, 0x1C, ped);
        At<uint32_t>(objective, 0) = 0x00928A8C;
        At<float>(objective, 0x10) = pos[0];
        At<float>(objective, 0x14) = pos[1];
        At<float>(objective, 0x18) = pos[2];
        At<float>(objective, 0x1C) = heading;
    }
    P4_Objectives_Add(ped + 0x5F0, objective);
    return 0;
}

} // namespace

#define PEDSCRIPT4_NATIVE(name, address)                                               \
    static decltype(&Native_##name) Original_P4_##name;                                  \
    REVERSED("Native_" #name, address, RE_BODY(&Native_##name), nullptr, \
             reinterpret_cast<void**>(&Original_P4_##name), re::Mode::Original)

PEDSCRIPT4_NATIVE(PedInRectangle, 0x005CA520);
PEDSCRIPT4_NATIVE(PedShowHealthBar, 0x005CA730);
PEDSCRIPT4_NATIVE(ShowGeneralHealthBar, 0x005CA870);
static void* Original_P4_ReleasePedToAmbient;
REVERSED("ScriptNatives::ReleasePedToAmbient_5CA910", 0x005CA910, RE_BODY(&ReleasePedToAmbient), nullptr,
         &Original_P4_ReleasePedToAmbient, re::Mode::Original);
PEDSCRIPT4_NATIVE(PedMakeMissionChar, 0x005CA9F0);
PEDSCRIPT4_NATIVE(PedClearObjectives, 0x005CAA40);
PEDSCRIPT4_NATIVE(PedClearObjective, 0x005CAA90);
PEDSCRIPT4_NATIVE(PedSetFocus, 0x005CAB30);
PEDSCRIPT4_NATIVE(PedSetAITree, 0x005CAC20);
PEDSCRIPT4_NATIVE(PedSetStatsType, 0x005CAD00);
PEDSCRIPT4_NATIVE(PedSetActionTree, 0x005CAD80);
PEDSCRIPT4_NATIVE(UnLoadBranch, 0x005CAEB0);
PEDSCRIPT4_NATIVE(PedCanSeeObject, 0x005CB140);
PEDSCRIPT4_NATIVE(PedLockTarget, 0x005CB1C0);
PEDSCRIPT4_NATIVE(PedAttackProp, 0x005CB290);
PEDSCRIPT4_NATIVE(PedAttackPropOffset, 0x005CB3D0);
PEDSCRIPT4_NATIVE(PedGetPedToTypeAttitude, 0x005CB630);
PEDSCRIPT4_NATIVE(PedGetTargetPed, 0x005CB760);
PEDSCRIPT4_NATIVE(PedAddPedToIgnoreList, 0x005CB830);
PEDSCRIPT4_NATIVE(PedRemovePedFromIgnoreList, 0x005CB890);
PEDSCRIPT4_NATIVE(PedIsDoingTask, 0x005CBA00);
PEDSCRIPT4_NATIVE(PedMePlaying, 0x005CBA70);
PEDSCRIPT4_NATIVE(PedHasJustKilledPedOfType, 0x005CBB80);
PEDSCRIPT4_NATIVE(PedHasGeneratedStimulusOfType, 0x005CBC20);
PEDSCRIPT4_NATIVE(PedRemoveStimulus, 0x005CBCB0);
PEDSCRIPT4_NATIVE(PedCoverSet, 0x005CBDD0);
PEDSCRIPT4_NATIVE(PedCoverSetFromProfile, 0x005CBF30);
PEDSCRIPT4_NATIVE(PedCoverSetOnCoverCallback, 0x005CC040);
PEDSCRIPT4_NATIVE(PedSetPOI, 0x005CC150);
PEDSCRIPT4_NATIVE(PedApplyDamage, 0x005CC320);
PEDSCRIPT4_NATIVE(PedPlayHitReaction, 0x005CC3D0);
PEDSCRIPT4_NATIVE(PedSetTetherToXYZ, 0x005CC460);
PEDSCRIPT4_NATIVE(PedSetTetherToPoint, 0x005CC4D0);
PEDSCRIPT4_NATIVE(PedSetTetherToPed, 0x005CC5E0);
PEDSCRIPT4_NATIVE(PedSetTetherToProp, 0x005CC660);
PEDSCRIPT4_NATIVE(PedSetAntiTetherToXYZ, 0x005CC880);
PEDSCRIPT4_NATIVE(PedSetAntiTetherToPoint, 0x005CC8F0);
PEDSCRIPT4_NATIVE(PedSetAntiTetherToPed, 0x005CCA00);
PEDSCRIPT4_NATIVE(PedSetAntiTetherToProp, 0x005CCA80);
PEDSCRIPT4_NATIVE(PedSetMissionCritical, 0x005CCB70);
PEDSCRIPT4_NATIVE(PedIsAlerted, 0x005CCBF0);
PEDSCRIPT4_NATIVE(PedWarpIntoCar, 0x005CCE30);
PEDSCRIPT4_NATIVE(PedPickup, 0x005CCEC0);
PEDSCRIPT4_NATIVE(PedTargetVehicle, 0x005CCFE0);
PEDSCRIPT4_NATIVE(PedIsStandingOnVehicle, 0x005CD0B0);
PEDSCRIPT4_NATIVE(PedFlee, 0x005CD150);
PEDSCRIPT4_NATIVE(PedJump, 0x005CD330);
PEDSCRIPT4_NATIVE(PedClimbWall, 0x005CD400);
PEDSCRIPT4_NATIVE(PedClimbLadder, 0x005CD4B0);
