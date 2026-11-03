// CLuaScript (0x2B0C bytes, the pool at CPools' 0xC0F650): a running script, its Lua
// threads, and what it created or requested (peds, objects, vehicles, blips, models,
// action trees, animation groups), all released when it ends (Cleanup, 0x5DAA20).
// And the script callbacks: 16-byte entries at 0xCFC9E0 (1510 of them) holding a Lua
// function reference for its owner (a pointer to the entry, cleared on release).
//
// The script's layout, as far as it's used here:
//   +0      state (Init 0; 2 or 3 when ending)
//   +4      name (a path, slashes)          +0x44   id (from the counter 0xCFC9C0)
//   +0x48   threads[64], 0x44 bytes each    +0x1148 their count   +0x114C the current (-1)
//   +0x1150 byte (1)                        +0x1154
//   +0x115C action trees[50]                +0x1224 count
//   +0x1228 blips[180] {handle, flag, float, float heading}   +0x1D68 count
//   +0x1D6C peds[180]                       +0x203C count
//   +0x2040 objects[50]                     +0x2108 count
//   +0x210C vehicles[50]                    +0x21D4 count
//   +0x21D8 0x21D8 entries[50]              +0x22A0 count
//   +0x22A4 models[256]                     +0x26A4 count
//   +0x26A8 agr groups[16]                  +0x26E8 count
//   +0x26EC anim groups[120]                +0x28CC count
//   +0x28D0 {index, action file}[50]        +0x2A60 count
//   +0x2A64 {index, action file}[20]        +0x2B04 count
//   +0x2B08, +0x2B09 bytes
// A thread (0x44): +0 its lua_State, +4 its registry reference, +8 the state it was
// made from, +0xC the script, +0x10 state (4: finished, 3: being killed), +0x14 byte,
// +0x18, +0x1C, +0x20 id, +0x24 name[32].

#include <cstdint>
#include <cstdio>

#include "game/ScriptApi.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

template <typename T>
re::Field<T>& lsAt(void* base, int offset) {
    return *reinterpret_cast<re::Field<T>*>(static_cast<uint8_t*>(base) + offset);
}

// A virtual (thiscall) at a byte offset in the object's vtable
template <typename R, typename... A>
R lsVCall(void* self, int offset, A... args) {
    using Fn = R(__thiscall*)(void*, re::Arg<A>...);
    Fn fn = re::GameFnAt<Fn>(*static_cast<re::Ptr32<uint8_t>*>(self) + offset);
    return fn(self, args...);
}

inline constexpr int LS_THREADS = 0x48;
inline constexpr int LS_THREAD_SIZE = 0x44;

// Lua
GAME_VAR(lua_State*, lsLuaState, 0x00D093B0);
LIB_FN(void, __cdecl, lsLua_gettable, lua_gettable, lua_State* L, int idx);
LIB_FN(int, __cdecl, lsLua_pcall, lua_pcall, lua_State* L, int nargs, int nresults, int errfunc);
LIB_FN(void, __cdecl, lsLua_setgcthreshold, lua_setgcthreshold, lua_State* L, int threshold);
LIB_FN(int, __cdecl, lsLua_getstack, lua_getstack, lua_State* L, int level, void* ar);
LIB_FN(int, __cdecl, lsLua_resume, lua_resume, lua_State* L, int nargs);  // (the game's; non-zero: an error)
LIB_FN(int, __cdecl, lsLuaL_ref, luaL_ref, lua_State* L, int t);
LIB_FN(void, __cdecl, lsLuaL_unref, luaL_unref, lua_State* L, int t, int ref);
inline constexpr int LS_LUA_GLOBALSINDEX = -10001;
inline void* __cdecl lsMemset(void* p, int value, uint32_t size) { return crt::Memset(p, value, (size_t)size); }

// the memory statistic the loaders keep
GAME_FN(uint32_t, __thiscall, lsHeap_GetMemoryUsed, 0x005EEDD0, void* heap, int memId);
GAME_VAR(uint32_t, lsLuaMemoryUsed, 0x00CFC9C8);
inline void lsUpdateLuaMemory() {
    uint32_t simple = lsHeap_GetMemoryUsed(reinterpret_cast<void*>(0x00D14240), 0x23);
    lsLuaMemoryUsed = simple + lsHeap_GetMemoryUsed(reinterpret_cast<void*>(0x00D14170), 0x23);
}

// TheLurStore (0xD02850): its first member, the action trees scripts use (50 of
// {refs, tree, ?}, the count at 0xD02AA8)
inline void* const lsLurStore = reinterpret_cast<void*>(0x00D02850);
inline int32_t* const lsTrees = reinterpret_cast<int32_t*>(0x00D02850);
GAME_VAR(uint32_t, lsTreeCount, 0x00D02AA8);
GAME_FN(uint8_t*, __thiscall, lsLurStore_FindScript, 0x005DB8F0, void* store, int id);
GAME_FN(uint8_t*, __thiscall, lsLurStore_GetScript, 0x005DBD50, void* store, lua_State* L);
GAME_FN(uint32_t, __cdecl, lsActFile_HashName, 0x005DB830, const char* path);
GAME_FN(void, __cdecl, lsTree_Release, 0x005F59D0, void* tree, int a);
GAME_VAR(void*, lsTreeRoot, 0x01B649DC);

// the script's own (called through the game's addresses)
GAME_FN(lua_State*, __thiscall, lsScript_GetThreadState, 0x005D8B50, void* script, int id);
GAME_FN(uint8_t*, __thiscall, lsScript_AddThread, 0x005D8DD0, void* script, lua_State* L, const char* name);
GAME_FN(void, __thiscall, lsScript_CreateNamespace, 0x005D8EA0, void* script, lua_State* L, const char* name);
GAME_FN(uint8_t*, __thiscall, lsScript_CreateThreadIn, 0x005D8FA0, void* script, int idx, const char* function);
GAME_FN(void, __thiscall, lsScript_SetName, 0x005D8100, void* script, const char* name);
GAME_FN(uint8_t, __thiscall, lsScript_PedUsesNode, 0x005D86F0, void* script, void* ped, void* node);
GAME_FN(void, __thiscall, lsThreads_RemoveAt, 0x005D8820, void* threads, uint32_t index);
GAME_FN(void, __thiscall, lsBlips_RemoveAt, 0x005D8870, void* blips, uint32_t index);
GAME_FN(void, __thiscall, lsPeds_RemoveAt, 0x005D88D0, void* peds, uint32_t index);
GAME_FN(void, __thiscall, lsVehicles_RemoveAt, 0x005D8910, void* vehicles, uint32_t index);
GAME_FN(void, __thiscall, lsModels_RemoveAt, 0x005D8950, void* models, uint32_t index);
GAME_FN(void, __thiscall, lsScript_AddBlip, 0x005D9380, void* script, int handle, int flag, float heading, float value);
GAME_FN(void, __thiscall, lsScript_KillThread, 0x005D9E90, void* script, uint32_t index);
GAME_FN(void, __thiscall, lsCallback_Release, 0x005D9BC0, void* callback);
GAME_FN(void, __cdecl, lsCallbacks_ReleaseFor, 0x005D9C60, int script, int thread);
GAME_FN(void, __cdecl, lsCallbacks_ReleaseScript, 0x005D9C90, int script);
GAME_FN(void, __thiscall, lsScript_ReleaseTrees, 0x005DA170, void* script);
GAME_FN(void, __thiscall, lsScript_ReleaseActionFiles, 0x005DA4D0, void* script);
GAME_FN(void, __thiscall, lsScript_RemoveBlips, 0x005D9330, void* script);
GAME_FN(void, __thiscall, lsScript_RemovePeds, 0x005D9750, void* script);
GAME_FN(void, __thiscall, lsScript_RemoveObjects, 0x005D9860, void* script);
GAME_FN(void, __thiscall, lsScript_RemoveVehicles, 0x005DA860, void* script);
GAME_FN(void, __thiscall, lsScript_Remove21D8, 0x005D9990, void* script);
GAME_FN(void, __thiscall, lsScript_ReleaseModels, 0x005DA7F0, void* script);
GAME_FN(void, __thiscall, lsScript_ReleaseAgrGroups, 0x005D94F0, void* script);
GAME_FN(void, __thiscall, lsScript_ReleaseAnimGroups, 0x005D9A80, void* script);
GAME_FN(void, __thiscall, lsScript_Cleanup, 0x005DAA20, void* script);
GAME_FN(void, __cdecl, lsScriptId_5B27E0, 0x005B27E0, int32_t* id);

// the world
GAME_VAR(uint8_t*, lsPlayerPed, 0x00C1AEA8);
struct LsPool {
    re::Ptr32<uint8_t> objects;
    re::Ptr32<uint8_t> flags;
    int32_t size;
    int32_t elemSize;
};
GAME_VAR(LsPool*, lsPedPool, 0x00C0F5F0);
GAME_VAR(LsPool*, lsVehiclePool, 0x00C0F5F4);
GAME_VAR(LsPool*, lsPropPool, 0x00C0F608);
GAME_ARRAY(uint8_t*, lsModelInfoPtrs, 0x00C67738);
GAME_FN(uint8_t*, __cdecl, lsGetPed, 0x005C7380, int handle, int kind);
GAME_FN(void, __cdecl, lsDeletePed, 0x005C73E0, int handle, int kind);
GAME_FN(uint8_t*, __cdecl, lsGetObject, 0x005C23C0, int handle);
GAME_FN(void, __cdecl, lsDeleteObject, 0x005C2A10, int handle16);
GAME_FN(void, __cdecl, lsDeleteVehicle, 0x005D6180, int handle16);
GAME_FN(void, __cdecl, lsSetModelNotNeeded, 0x0052FF40, int model);
GAME_FN(void, __cdecl, lsModel_4D9850, 0x004D9850, int model);
GAME_FN(void, __cdecl, lsRemoveModel, 0x0052F640, int id);
GAME_FN(int32_t, __cdecl, lsGetAgrGroup, 0x0052DB50, int32_t index, int32_t* out);
GAME_FN(void, __thiscall, lsAgr_Release, 0x006BD030, void* self, uintptr_t group, int32_t b, int c);
inline void* const lsAgrManager = reinterpret_cast<void*>(0x020C4910);
GAME_FN(void, __thiscall, lsVehicle_4CB230, 0x004CB230, void* vehicle, int a);
GAME_FN(void, __thiscall, lsRadar_RemoveBlip, 0x0054A940, void* radar, int handle);
GAME_VAR(void*, lsRadar, 0x00C674CC);
GAME_FN(int, __cdecl, lsEntry_4438B0, 0x004438B0, int32_t value);
GAME_FN(void, __cdecl, lsEntry_4454F0, 0x004454F0, int32_t value, int a);
// the animation groups (0xC0F7B4)
inline void* const lsAnimGroups = reinterpret_cast<void*>(0x00C0F7B4);
GAME_FN(void, __thiscall, lsAnimGroups_Release, 0x00450720, void* groups, int group);
GAME_FN(int, __thiscall, lsAnimGroups_RefCount, 0x00450760, void* groups, int group);
GAME_FN(void, __thiscall, lsAnimGroups_AddRef, 0x004506F0, void* groups, int group);
GAME_FN(void, __cdecl, lsAnim_Remove2, 0x0051B980, int group);
// props / peds and their action nodes
GAME_FN(void, __thiscall, lsProp_UpdateAnim, 0x004ED940, void* prop);
GAME_FN(uint8_t, __thiscall, lsNodeList_Contains, 0x005F48D0, void* list, void* node, int a, int b);
GAME_FN(void, __thiscall, lsNodeList_5F40A0, 0x005F40A0, void* list);
GAME_FN(void, __thiscall, lsEntity_4EF2A0, 0x004EF2A0, void* entity);
GAME_FN(uint8_t, __thiscall, lsModelInfo_520430, 0x00520430, void* modelInfo);
GAME_FN(void, __thiscall, lsPedActions_4840A0, 0x004840A0, void* actions);
GAME_FN(void, __thiscall, lsPed_474090, 0x00474090, void* ped);
GAME_FN(void, __thiscall, lsPed_4740A0, 0x004740A0, void* ped);
GAME_FN(void, __thiscall, lsPed_4740B0, 0x004740B0, void* ped);
GAME_FN(void, __thiscall, lsActionFile_5F6670, 0x005F6670, void* file);
GAME_FN(void, __thiscall, lsActionFile_5F56B0, 0x005F56B0, void* file, const char* name);
GAME_FN(void, __thiscall, lsActionFile_5F6CE0, 0x005F6CE0, void* file);
// the class id the action files are checked against (a function-local static)
GAME_VAR(uint32_t, lsActionFileTypeGuard, 0x00CFB378);
GAME_VAR(uint16_t, lsActionFileType, 0x00CFB374);
GAME_VAR(uint16_t, lsNextTypeId, 0x00BF3830);
// the managers a script's end is passed to
GAME_FN(void, __thiscall, lsManager_488420, 0x00488420, void* manager, void* script);
GAME_FN(void, __cdecl, lsCleanup_49A650, 0x0049A650);  // (an empty function)
GAME_FN(void, __thiscall, lsManager_4A1C00, 0x004A1C00, void* manager, void* script);
GAME_FN(void, __thiscall, lsManager_46EBC0, 0x0046EBC0, void* manager, void* script);
GAME_FN(void, __thiscall, lsManager_4729D0, 0x004729D0, void* manager, void* script);
GAME_VAR(void*, lsManagerC2C12C, 0x00C2C12C);
GAME_VAR(void*, lsManagerC2AB68, 0x00C2AB68);
GAME_VAR(void*, lsManagerC2B034, 0x00C2B034);

uint8_t* lsThread(uint8_t* script, uint32_t index) {
    return script + LS_THREADS + index * LS_THREAD_SIZE;
}

// ---- array removal (thiscall on the array; its count after it)

// 0x5D8820: the threads (count at +0x1100 from the array)
void __fastcall Threads_RemoveAt(uint8_t* self, re::Edx, uint32_t index) {
    for (; index < lsAt<uint32_t>(self, 0x1100) - 1; index++) {
        uint32_t* dst = reinterpret_cast<uint32_t*>(self + index * 0x44);
        for (int i = 0; i < 0x11; i++) {
            dst[i] = dst[i + 0x11];
        }
    }
    lsAt<int32_t>(self, 0x1100)--;
}
// 0x5D8870: the blips (16 bytes, count at +0xB40)
void __fastcall Blips_RemoveAt(uint8_t* self, re::Edx, uint32_t index) {
    for (; index < lsAt<uint32_t>(self, 0xB40) - 1; index++) {
        uint32_t* dst = reinterpret_cast<uint32_t*>(self + index * 0x10);
        dst[0] = dst[4];
        dst[1] = dst[5];
        dst[2] = dst[6];
        dst[3] = dst[7];
    }
    lsAt<int32_t>(self, 0xB40)--;
}
// 0x5D88D0 / 0x5D8910 / 0x5D8950: dword arrays with the count at +0x2D0, +0xC8, +0x400
template <int COUNT>
void __fastcall Dwords_RemoveAt(uint8_t* self, re::Edx, uint32_t index) {
    for (; index < lsAt<uint32_t>(self, COUNT) - 1; index++) {
        lsAt<uint32_t>(self, index * 4) = lsAt<uint32_t>(self, 4 + index * 4);
    }
    lsAt<int32_t>(self, COUNT)--;
}

// ---- CLuaScript

// 0x5D8AB0: set up for a script of that name
uint8_t* __fastcall LuaScript_Init(uint8_t* self, re::Edx, const char* name) {
    lsAt<int32_t>(self, 0) = 0;
    lsAt<int32_t>(self, 0x1148) = 0;
    lsAt<int32_t>(self, 0x1154) = 0;
    lsAt<int32_t>(self, 0x114C) = -1;
    lsAt<int32_t>(self, 0x1224) = 0;
    lsAt<int32_t>(self, 0x1D68) = 0;
    lsAt<int32_t>(self, 0x203C) = 0;
    lsAt<int32_t>(self, 0x2108) = 0;
    lsAt<int32_t>(self, 0x21D4) = 0;
    lsAt<int32_t>(self, 0x22A0) = 0;
    lsAt<int32_t>(self, 0x26A4) = 0;
    lsAt<int32_t>(self, 0x26E8) = 0;
    lsAt<int32_t>(self, 0x28CC) = 0;
    lsAt<int32_t>(self, 0x2A60) = 0;
    lsAt<int32_t>(self, 0x2B04) = 0;
    lsAt<uint8_t>(self, 0x2B08) = 0;
    lsAt<uint8_t>(self, 0x2B09) = 0;
    lsAt<uint8_t>(self, 0x1150) = 1;
    lsScript_SetName(self, name);
    int32_t& counter = *reinterpret_cast<int32_t*>(0x00CFC9C0);
    lsAt<int32_t>(self, 0x44) = counter;
    counter++;
    return self;
}

// 0x5D8B50: the lua_State of the thread with that id (nullptr: none)
lua_State* __fastcall LuaScript_GetThreadState(uint8_t* self, re::Edx, int id) {
    for (uint32_t i = 0; i < lsAt<uint32_t>(self, 0x1148); i++) {
        if (lsAt<int32_t>(lsThread(self, i), 0x20) == id) {
            return lsAt<lua_State*>(lsThread(self, i), 0);
        }
    }
    return nullptr;
}

// 0x5D8B90: the current thread (nullptr: none)
uint8_t* __fastcall LuaScript_GetCurrentThread(uint8_t* self, re::Edx) {
    uint32_t index = lsAt<uint32_t>(self, 0x114C);
    if (int32_t(index) > -1 && index < lsAt<uint32_t>(self, 0x1148)) {
        return lsThread(self, index);
    }
    return nullptr;
}

// 0x5D8BC0: the current thread's id (-1: none)
int __fastcall LuaScript_GetCurrentThreadId(uint8_t* self, re::Edx) {
    uint32_t index = lsAt<uint32_t>(self, 0x114C);
    if (int32_t(index) > -1 && index < lsAt<uint32_t>(self, 0x1148)) {
        uint8_t* thread = lsThread(self, index);
        if (thread != nullptr) {
            return lsAt<int32_t>(thread, 0x20);
        }
    }
    return -1;
}

// 0x5D8BF0: makes the thread with that id current (-1: none; an unknown id: unchanged)
void __fastcall LuaScript_SetCurrentThread(uint8_t* self, re::Edx, int id) {
    if (id == -1) {
        lsAt<int32_t>(self, 0x114C) = -1;
        return;
    }
    for (uint32_t i = 0; i < lsAt<uint32_t>(self, 0x1148); i++) {
        if (lsAt<int32_t>(lsThread(self, i), 0x20) == id) {
            lsAt<uint32_t>(self, 0x114C) = i;
            return;
        }
    }
}

// 0x5D8DD0: a new thread from the one on top of L's stack (referenced in the registry,
// popped with the value under it); the new entry. (Its +0xC is set to the script just
// before the copy; its name is copied without a bound.)
uint8_t* __fastcall LuaScript_AddThread(uint8_t* self, re::Edx, lua_State* L, const char* name) {
    uint32_t thread[0x11];
    thread[1] = uint32_t(lsLuaL_ref(L, LUA_REGISTRYINDEX));
    thread[2] = reinterpret_cast<uint32_t>(L);
    lua::rawgeti(L, LUA_REGISTRYINDEX, int(thread[1]));
    thread[0] = reinterpret_cast<uint32_t>(lua::tothread(L, -1));
    int32_t& counter = *reinterpret_cast<int32_t*>(0x00CFC9C0);
    thread[8] = uint32_t(counter);
    counter++;
    thread[7] = 0;
    thread[4] = 0;
    thread[5] = 0;  // (a byte; the rest of the dword is stack)
    thread[6] = 0;
    if (name == nullptr) {
        name = "<none>";
    }
    char* d = reinterpret_cast<char*>(&thread[9]);
    char c;
    do {
        c = *name++;
        *d++ = c;
    } while (c != '\0');
    lua::settop(L, -2);
    thread[3] = reinterpret_cast<uint32_t>(self);
    uint32_t* dst = reinterpret_cast<uint32_t*>(lsThread(self, lsAt<uint32_t>(self, 0x1148)));
    for (int i = 0; i < 0x11; i++) {
        dst[i] = thread[i];
    }
    lsAt<int32_t>(self, 0x1148)++;
    return self + 4 + lsAt<int32_t>(self, 0x1148) * LS_THREAD_SIZE;
}

// 0x5D8EA0: the script's namespace (Lua's CreateNameSpace(name), which returns its
// main thread) and its first thread
void __fastcall LuaScript_CreateNamespace(uint8_t* self, re::Edx, lua_State* L, const char* name) {
    lua::pushlstring(L, "__onerror", 9);
    lsLua_gettable(L, LS_LUA_GLOBALSINDEX);
    lua::pushstring(L, "CreateNameSpace");
    lsLua_gettable(L, LS_LUA_GLOBALSINDEX);
    lua::pushstring(L, name);
    int failed = lsLua_pcall(L, 1, 1, -3);
#if defined(_M_X64)   // (bring-up: what Lua said)
    std::fprintf(stderr, "x64 lua: CreateNameSpace(%s) -> %d, top %s %s\n", name, failed, lua::type_name(L, lua::type(L, -1)),
                 lua::isstring(L, -1) ? lua::tostring(L, -1) : "");
#endif
    (void)failed;
    lsScript_AddThread(self, L, name);
    lua::settop(L, -2);
}

// 0x5D8F10: whether a thread (not the first) of that name runs (state not 4)
uint8_t __fastcall LuaScript_IsThreadRunning(uint8_t* self, re::Edx, const char* name) {
    uint32_t count = lsAt<uint32_t>(self, 0x1148);
    for (uint32_t i = 1; i < count; i++) {
        const char* a = reinterpret_cast<const char*>(lsThread(self, i) + 0x24);
        const char* b = name;
        while (*a == *b && *a != '\0') {
            a++;
            b++;
        }
        if (*a == *b) {
            return lsAt<int32_t>(lsThread(self, i), 0x10) != 4;
        }
    }
    return 0;
}

// 0x5D8FA0: a new thread (Lua's ThreadNameSpace(namespace, function)): the function by
// name (idx 0) or the value at idx; the new entry
uint8_t* __fastcall LuaScript_CreateThreadIn(uint8_t* self, re::Edx, int idx, const char* function) {
    lua_State* L = lsLuaState;
    lua::pushlstring(L, "__onerror", 9);
    lsLua_gettable(L, LS_LUA_GLOBALSINDEX);
    lua::pushstring(L, "ThreadNameSpace");
    lsLua_gettable(L, LS_LUA_GLOBALSINDEX);
    lua::pushstring(L, reinterpret_cast<const char*>(self + 4));
    if (idx == 0) {
        lua::pushstring(L, function);
    } else {
        if (idx < 0) {
            idx -= 3;
        }
        lua::pushvalue(L, idx);
    }
    lsLua_pcall(L, 2, 1, -4);
    lsScript_AddThread(self, L, function);
    lua::settop(L, -2);
    return self + 4 + lsAt<int32_t>(self, 0x1148) * LS_THREAD_SIZE;
}

// 0x5D9050: every thread but the first: +0x1C = -1
void __fastcall LuaScript_ResetThreads1C(uint8_t* self, re::Edx) {
    for (uint32_t i = 1; i < lsAt<uint32_t>(self, 0x1148); i++) {
        lsAt<int32_t>(lsThread(self, i), 0x1C) = -1;
    }
}

// 0x5D9150: the action tree the node is in (its top node under the root) is used by
// the script: counted in TheLurStore's table (added with its name's hash when new)
void __fastcall LuaScript_UseActionTree(uint8_t* self, re::Edx, void* node, const char* name) {
    void* tree = node;
    if (node != lsTreeRoot) {
        void* parent;
        do {
            tree = node;
            parent = lsVCall<void*>(node, 0x34, -1);
            node = parent;
        } while (parent != lsTreeRoot);
    }
    for (uint32_t i = 0; i < lsAt<uint32_t>(self, 0x1224); i++) {
        if (lsAt<void*>(self, 0x115C + i * 4) == tree) {
            return;
        }
    }
    for (uint32_t i = 0; i < lsTreeCount; i++) {
        if (reinterpret_cast<void*>(lsTrees[i * 3 + 1]) == tree) {
            lsAt<void*>(self, 0x115C + lsAt<int32_t>(self, 0x1224) * 4) = tree;
            lsAt<int32_t>(self, 0x1224)++;
            lsTrees[i * 3]++;
            return;
        }
    }
    int32_t value = int32_t(lsActFile_HashName(name));
    uint32_t n = lsTreeCount;
    lsTrees[n * 3] = 1;
    lsTrees[n * 3 + 1] = reinterpret_cast<int32_t>(tree);
    lsTrees[n * 3 + 2] = value;
    lsTreeCount++;
    lsAt<void*>(self, 0x115C + lsAt<int32_t>(self, 0x1224) * 4) = tree;
    lsAt<int32_t>(self, 0x1224)++;
}

// 0x5D9230: the prop's animation stopped (its group released unless its model keeps
// it)
void __fastcall LuaScript_StopPropAnim(void*, re::Edx, uint8_t* prop) {
    if (prop == nullptr) {
        return;
    }
    uint8_t* modelInfo = lsModelInfoPtrs[lsAt<int16_t>(prop, 0x10E)];
    int group = lsAt<int16_t>(prop, 0x22C);
    lsProp_UpdateAnim(prop);
    lsNodeList_5F40A0(prop + 0x128);
    lsEntity_4EF2A0(prop);
    if (group != -1 && !lsModelInfo_520430(modelInfo)) {
        lsAnimGroups_Release(lsAnimGroups, group);
    }
}

// 0x5D9290 / 0x5D92E0: an action file (index, file) to release at the end, once
template <int ARRAY, int COUNT>
void __fastcall LuaScript_AddActionFile(uint8_t* self, re::Edx, int32_t index, int32_t file) {
    uint32_t count = lsAt<uint32_t>(self, COUNT);
    for (uint32_t i = 0; i < count; i++) {
        if (lsAt<int32_t>(self, ARRAY + i * 8 + 4) != 0 && lsAt<int32_t>(self, ARRAY + i * 8) == index) {
            return;
        }
    }
    lsAt<int32_t>(self, ARRAY + count * 8) = index;
    lsAt<int32_t>(self, ARRAY + 4 + count * 8) = file;
    lsAt<int32_t>(self, COUNT)++;
}

// 0x5D9330: the blips removed from the radar (which takes each out of the list: the
// first entry each time, while any are left)
void __fastcall LuaScript_RemoveBlips(uint8_t* self, re::Edx) {
    void* radar = lsRadar;
    int32_t n = lsAt<int32_t>(self, 0x1D68);
    if (n != 0) {
        do {
            n--;
            if (n < 0) {
                break;
            }
            if (radar != nullptr) {
                lsRadar_RemoveBlip(radar, lsAt<int32_t>(self, 0x1228));
            }
        } while (lsAt<uint32_t>(self, 0x1D68) > 0);
    }
    lsAt<int32_t>(self, 0x1D68) = 0;
}

// 0x5D9380: a blip (up to 179); heading in degrees, kept in radians. (The flag goes
// in as a byte through a stack dword whose upper bytes the game doesn't set: zero here.)
void __fastcall LuaScript_AddBlip(uint8_t* self, re::Edx, int handle, int flag, float heading, float value) {
    if (handle == 0) {
        return;
    }
    float radians = float(double(heading) * double(3.1415927f) / 180.0);
    if (lsAt<uint32_t>(self, 0x1D68) + 1 >= 0xB4) {
        return;
    }
    uint8_t* entry = self + 0x1228 + lsAt<int32_t>(self, 0x1D68) * 0x10;
    lsAt<int32_t>(entry, 0) = handle;
    lsAt<uint32_t>(entry, 4) = uint8_t(flag);
    lsAt<float>(entry, 8) = value;
    lsAt<float>(entry, 0xC) = radians;
    lsAt<int32_t>(self, 0x1D68)++;
}

// 0x5D9400: a blip taken out of the list (the first of that handle)
void __fastcall LuaScript_RemoveBlip(uint8_t* self, re::Edx, int handle) {
    bool removed = false;
    for (uint32_t i = 0; i < lsAt<uint32_t>(self, 0x1D68); i++) {
        if (removed) {
            return;
        }
        if (lsAt<int32_t>(self, 0x1228 + i * 0x10) == handle) {
            lsBlips_RemoveAt(self + 0x1228, i);
            removed = true;
        }
    }
}

// A value added to a dword list once (0x5D9450 models, 0x5D9550 agr groups, 0x5D9630
// peds)
template <int ARRAY, int COUNT>
void __fastcall LuaScript_AddOnce(uint8_t* self, re::Edx, int32_t value) {
    uint32_t count = lsAt<uint32_t>(self, COUNT);
    for (uint32_t i = 0; i < count; i++) {
        if (lsAt<int32_t>(self, ARRAY + i * 4) == value) {
            return;
        }
    }
    lsAt<int32_t>(self, ARRAY + count * 4) = value;
    lsAt<int32_t>(self, COUNT)++;
}

// 0x5D94A0: a model no longer needed (and out of the list)
void __fastcall LuaScript_ReleaseModel(uint8_t* self, re::Edx, int model) {
    lsSetModelNotNeeded(model);
    for (uint32_t i = 0; i < lsAt<uint32_t>(self, 0x26A4); i++) {
        if (lsAt<int32_t>(self, 0x22A4 + i * 4) == model) {
            if (i != 0xFFFFFFFF) {
                lsModels_RemoveAt(self + 0x22A4, i);
            }
            return;
        }
    }
}

// 0x5D94F0: every agr group released
void __fastcall LuaScript_ReleaseAgrGroups(uint8_t* self, re::Edx) {
    int32_t out;
    for (uint32_t i = 0; i < lsAt<uint32_t>(self, 0x26E8); i++) {
        int32_t group = lsGetAgrGroup(lsAt<int32_t>(self, 0x26A8 + i * 4), &out);
        lsAgr_Release(lsAgrManager, group, out, 0);
    }
    lsAt<int32_t>(self, 0x26E8) = 0;
}

// 0x5D95A0: one agr group released
void __fastcall LuaScript_ReleaseAgrGroup(uint8_t* self, re::Edx, int32_t index) {
    uint32_t i = 0;
    if (lsAt<uint32_t>(self, 0x26E8) == 0) {
        return;
    }
    while (lsAt<int32_t>(self, 0x26A8 + i * 4) != index) {
        i++;
        if (i >= lsAt<uint32_t>(self, 0x26E8)) {
            return;
        }
    }
    if (i == 0xFFFFFFFF) {
        return;
    }
    int32_t out = index;
    int32_t group = lsGetAgrGroup(index, &out);
    lsAgr_Release(lsAgrManager, group, out, 0);
    for (; i < lsAt<uint32_t>(self, 0x26E8) - 1; i++) {
        lsAt<int32_t>(self, 0x26A8 + i * 4) = lsAt<int32_t>(self, 0x26AC + i * 4);
    }
    lsAt<int32_t>(self, 0x26E8)--;
}

// 0x5D9680: a ped handle out of the list
void __fastcall LuaScript_RemovePedHandle(uint8_t* self, re::Edx, int handle) {
    for (uint32_t i = 0; i < lsAt<uint32_t>(self, 0x203C); i++) {
        if (lsAt<int32_t>(self, 0x1D6C + i * 4) == handle) {
            if (i != 0xFFFFFFFF) {
                lsPeds_RemoveAt(self + 0x1D6C, i);
            }
            return;
        }
    }
}

// 0x5D96C0: a ped out of the list (by the ped: the first handle giving it)
void __fastcall LuaScript_RemovePed(uint8_t* self, re::Edx, uint8_t* ped) {
    uint32_t count = lsAt<uint32_t>(self, 0x203C);
    if (count == 0) {
        return;
    }
    uint32_t i = 0;
    for (;;) {
        uint8_t* p = lsGetPed(lsAt<int32_t>(self, 0x1D6C + i * 4), 2);
        if (p != nullptr && p == ped) {
            break;
        }
        i++;
        if (i >= lsAt<uint32_t>(self, 0x203C)) {
            return;
        }
    }
    int32_t handle = lsAt<int32_t>(self, 0x1D6C + i * 4);
    for (uint32_t j = 0; j < lsAt<uint32_t>(self, 0x203C); j++) {
        if (lsAt<int32_t>(self, 0x1D6C + j * 4) == handle) {
            if (j != 0xFFFFFFFF) {
                lsPeds_RemoveAt(self + 0x1D6C, j);
            }
            return;
        }
    }
}

// 0x5D9750: the script's peds deleted (their models released)
void __fastcall LuaScript_RemovePeds(uint8_t* self, re::Edx) {
    for (uint32_t i = 0; i < lsAt<uint32_t>(self, 0x203C); i++) {
        int32_t handle = lsAt<int32_t>(self, 0x1D6C + i * 4);
        uint8_t* ped = lsGetPed(handle, 2);
        if (ped != nullptr) {
            lsSetModelNotNeeded(lsAt<int16_t>(ped, 0x10E));
            lsDeletePed(lsAt<int32_t>(self, 0x1D6C + i * 4), 2);
        }
    }
    lsAt<int32_t>(self, 0x203C) = 0;
}

// 0x5D97D0: an object handle
void __fastcall LuaScript_AddObject(uint8_t* self, re::Edx, int32_t handle) {
    lsAt<int32_t>(self, 0x2040 + lsAt<int32_t>(self, 0x2108) * 4) = handle;
    lsAt<int32_t>(self, 0x2108)++;
}

// 0x5D97F0: an object handle out of the list (the count drops even when it isn't there)
void __fastcall LuaScript_RemoveObject(uint8_t* self, re::Edx, int32_t handle) {
    uint32_t count = lsAt<uint32_t>(self, 0x2108);
    uint32_t index = 0xFFFFFFFF;
    for (uint32_t i = 0; i < count; i++) {
        if (lsAt<int32_t>(self, 0x2040 + i * 4) == handle) {
            index = i;
            break;
        }
    }
    for (; index < lsAt<uint32_t>(self, 0x2108) - 1; index++) {
        lsAt<int32_t>(self, 0x2040 + index * 4) = lsAt<int32_t>(self, 0x2044 + index * 4);
    }
    lsAt<int32_t>(self, 0x2108)--;
}

// 0x5D9860: the script's objects deleted (their models released)
void __fastcall LuaScript_RemoveObjects(uint8_t* self, re::Edx) {
    for (uint32_t i = 0; i < lsAt<uint32_t>(self, 0x2108); i++) {
        uint8_t* object = lsGetObject(lsAt<int32_t>(self, 0x2040 + i * 4));
        lsSetModelNotNeeded(lsAt<int16_t>(object, 0x10E));
        lsDeleteObject(lsAt<uint16_t>(self, 0x2040 + i * 4));
    }
    lsAt<int32_t>(self, 0x2108) = 0;
}

// 0x5D98C0: a 0x21D8 entry (its 72-byte record at 0xC0A100 + 0x48 * index gets the
// script at +0x18)
void __fastcall LuaScript_Add21D8(uint8_t* self, re::Edx, int32_t value) {
    int index = lsEntry_4438B0(value);
    uint8_t* record = reinterpret_cast<uint8_t*>(0x00C0A100) + index * 0x48;
    if (record != nullptr) {
        lsAt<uint8_t*>(record, 0x18) = self;
    }
    lsAt<int32_t>(self, 0x21D8 + lsAt<int32_t>(self, 0x22A0) * 4) = value;
    lsAt<int32_t>(self, 0x22A0)++;
}

// 0x5D9900: a 0x21D8 entry out of the list (released first unless `keep`; not there,
// the dword before the list is released and the count still drops)
void __fastcall LuaScript_Remove21D8(uint8_t* self, re::Edx, int32_t value, int keep) {
    uint32_t index = 0xFFFFFFFF;
    for (uint32_t i = 0; i < lsAt<uint32_t>(self, 0x22A0); i++) {
        if (lsAt<int32_t>(self, 0x21D8 + i * 4) == value) {
            index = i;
            break;
        }
    }
    if (!re::Flag(keep)) {
        lsEntry_4454F0(lsAt<int32_t>(self, 0x21D8 + index * 4), 1);
    }
    for (; index < lsAt<uint32_t>(self, 0x22A0) - 1; index++) {
        lsAt<int32_t>(self, 0x21D8 + index * 4) = lsAt<int32_t>(self, 0x21DC + index * 4);
    }
    lsAt<int32_t>(self, 0x22A0)--;
}

// 0x5D9990: every 0x21D8 entry released
void __fastcall LuaScript_Remove21D8All(uint8_t* self, re::Edx) {
    for (uint32_t i = 0; i < lsAt<uint32_t>(self, 0x22A0); i++) {
        lsEntry_4454F0(lsAt<int32_t>(self, 0x21D8 + i * 4), 1);
    }
    lsAt<int32_t>(self, 0x22A0) = 0;
}

// 0x5D99E0: a vehicle handle
void __fastcall LuaScript_AddVehicle(uint8_t* self, re::Edx, int32_t handle) {
    lsAt<int32_t>(self, 0x210C + lsAt<int32_t>(self, 0x21D4) * 4) = handle;
    lsAt<int32_t>(self, 0x21D4)++;
}

// 0x5D9A00: a vehicle handle out of the list
void __fastcall LuaScript_RemoveVehicle(uint8_t* self, re::Edx, int32_t handle) {
    for (uint32_t i = 0; i < lsAt<uint32_t>(self, 0x21D4); i++) {
        if (lsAt<int32_t>(self, 0x210C + i * 4) == handle) {
            if (i != 0xFFFFFFFF) {
                lsVehicles_RemoveAt(self + 0x210C, i);
            }
            return;
        }
    }
}

// 0x5D9A40: the thread entry running that lua_State (nullptr: none)
uint8_t* __fastcall LuaScript_FindThread(uint8_t* self, re::Edx, lua_State* L) {
    for (uint32_t i = 0; i < lsAt<uint32_t>(self, 0x1148); i++) {
        if (lsAt<lua_State*>(lsThread(self, i), 0) == L) {
            return lsThread(self, i);
        }
    }
    return nullptr;
}

// 0x5D9A80: every animation group released (removed when nothing else holds it)
void __fastcall LuaScript_ReleaseAnimGroups(uint8_t* self, re::Edx) {
    for (uint32_t i = 0; i < lsAt<uint32_t>(self, 0x28CC); i++) {
        int group = lsAt<int32_t>(self, 0x26EC + i * 4);
        lsAnimGroups_Release(lsAnimGroups, group);
        if (lsAnimGroups_RefCount(lsAnimGroups, group) == 0) {
            lsRemoveModel(group + 0x5D41);
            lsAnim_Remove2(group);
        }
    }
    lsAt<int32_t>(self, 0x28CC) = 0;
}

// 0x5D9AF0: an animation group held (once)
void __fastcall LuaScript_AddAnimGroup(uint8_t* self, re::Edx, int32_t group) {
    uint32_t count = lsAt<uint32_t>(self, 0x28CC);
    for (uint32_t i = 0; i < count; i++) {
        if (lsAt<int32_t>(self, 0x26EC + i * 4) == group) {
            if (i != 0xFFFFFFFF) {
                return;
            }
            break;
        }
    }
    lsAnimGroups_AddRef(lsAnimGroups, group);
    lsAt<int32_t>(self, 0x26EC + lsAt<int32_t>(self, 0x28CC) * 4) = group;
    lsAt<int32_t>(self, 0x28CC)++;
}

// 0x5D9DB0: the script started: its namespace, then a garbage collection (the main
// thread's gc threshold 0)
void __fastcall LuaScript_Start(uint8_t* self, re::Edx, lua_State* L) {
    lsUpdateLuaMemory();
    lsScript_CreateNamespace(self, L, reinterpret_cast<const char*>(self + 4));
    lsUpdateLuaMemory();
    lsLua_setgcthreshold(lsAt<lua_State*>(self, 0x48), 0);
    lsUpdateLuaMemory();
}

// 0x5D9E40: ending (state 3 when `flag`, else 2); every thread but the first: +0x1C = -1
void __fastcall LuaScript_End(uint8_t* self, re::Edx, int flag) {
    for (uint32_t i = 1; i < lsAt<uint32_t>(self, 0x1148); i++) {
        lsAt<int32_t>(lsThread(self, i), 0x1C) = -1;
    }
    lsAt<int32_t>(self, 0) = (uint8_t(flag) != 0) + 2;
}

// 0x5D9E80: a thread running the named function
uint8_t* __fastcall LuaScript_CreateThread(uint8_t* self, re::Edx, const char* function) {
    return lsScript_CreateThreadIn(self, 0, function);
}

// 0x5D9E90: a thread killed: its callbacks released; the main thread (0) also ends
// the script (its id's things, every callback of it, a last resume, Lua's
// KillNameSpace; it's finished when it has no stack left); its reference dropped
void __fastcall LuaScript_KillThread(uint8_t* self, re::Edx, uint32_t index) {
    uint8_t* thread = lsThread(self, index);
    lua_State* L = lsAt<lua_State*>(thread, 8);
    lsCallbacks_ReleaseFor(lsAt<int32_t>(self, 0x44), lsAt<int32_t>(thread, 0x20));
    if (index == 0) {
        lsScriptId_5B27E0(&lsAt<int32_t>(self, 0x44));
        lsCallbacks_ReleaseScript(lsAt<int32_t>(self, 0x44));
        lsLua_resume(lsAt<lua_State*>(thread, 0), 0);
        int32_t ar[0x18];  // lua_Debug
        ar[0] = int32_t(index);
        lsMemset(&ar[1], 0, 0x5C);
        if (lsLua_getstack(lsAt<lua_State*>(thread, 0), 0, ar) == 0 && lua::gettop(lsAt<lua_State*>(thread, 0)) == 0) {
            lsAt<int32_t>(thread, 0x10) = 4;
        }
        lua::pushlstring(L, "KillNameSpace", 0xD);
        lsLua_gettable(L, LS_LUA_GLOBALSINDEX);
        lua::pushstring(L, reinterpret_cast<const char*>(self + 4));
        lsLua_pcall(L, 1, 0, 0);
    }
    lsLuaL_unref(L, LUA_REGISTRYINDEX, lsAt<int32_t>(thread, 4));
    lsAt<int32_t>(thread, 4) = -1;
    lsThreads_RemoveAt(self + LS_THREADS, index);
}

// 0x5D9F80: the thread(s) with that id killed (state 3 first; the one after each is
// skipped, as the list closes up); true
uint8_t __fastcall LuaScript_KillThreadWithId(uint8_t* self, re::Edx, int32_t id) {
    for (uint32_t i = 0; i < lsAt<uint32_t>(self, 0x1148); i++) {
        uint8_t* thread = lsThread(self, i);
        if (lsAt<int32_t>(thread, 0x20) == id) {
            lsAt<int32_t>(thread, 0x10) = 3;
            lsScript_KillThread(self, i);
        }
    }
    return 1;
}

// 0x5DA170: the action trees released: the props running one of them stop, props
// animating one of the script's animation groups (with no group of their model) drop
// it, peds running a node of them reset (or lose it as their current ones); each tree's
// count in TheLurStore drops (released at 0), unused entries removed
void __fastcall LuaScript_ReleaseTrees(uint8_t* self, re::Edx) {
    if (lsAt<int32_t>(self, 0x1224) == 0) {
        return;
    }
    LsPool* props = lsPropPool;
    int32_t propCount = props->size;
    for (int32_t slot = 0; slot < propCount; slot++) {
        if ((props->flags[slot] & 0x80) != 0) {
            continue;
        }
        uint8_t* prop = props->objects + props->elemSize * slot;
        if (prop == nullptr) {
            continue;
        }
        int32_t n = lsAt<int32_t>(self, 0x1224);
        if (n > 0) {
            for (int32_t i = 0; i < n; i++) {
                void* tree = lsAt<void*>(self, 0x115C + i * 4);
                if (tree == nullptr || lsAt<int32_t>(prop, 0x228) == 0) {
                    continue;
                }
                lsProp_UpdateAnim(prop);
                if (lsNodeList_Contains(prop + 0x128, tree, 1, 0)) {
                    uint8_t* modelInfo = lsModelInfoPtrs[lsAt<int16_t>(prop, 0x10E)];
                    int group = lsAt<int16_t>(prop, 0x22C);
                    lsProp_UpdateAnim(prop);
                    lsNodeList_5F40A0(prop + 0x128);
                    lsEntity_4EF2A0(prop);
                    if (group != -1 && !lsModelInfo_520430(modelInfo)) {
                        lsAnimGroups_Release(lsAnimGroups, group);
                    }
                }
            }
        }
        for (uint32_t i = 0; i < lsAt<uint32_t>(self, 0x28CC); i++) {
            if (lsAt<int16_t>(prop, 0x22C) == lsAt<int32_t>(self, 0x26EC + i * 4)) {
                uint8_t* modelInfo = lsModelInfoPtrs[lsAt<int16_t>(prop, 0x10E)];
                if (lsVCall<int16_t>(modelInfo, 0x2C) == -1) {
                    lsEntity_4EF2A0(prop);
                    lsAt<int16_t>(prop, 0x22C) = -1;
                }
            }
        }
    }
    LsPool* peds = lsPedPool;
    int32_t pedCount = peds->size;
    for (int32_t slot = 0; slot < pedCount; slot++) {
        if ((peds->flags[slot] & 0x80) != 0) {
            continue;
        }
        uint8_t* ped = peds->objects + peds->elemSize * slot;
        if (ped == nullptr) {
            continue;
        }
        int32_t n = lsAt<int32_t>(self, 0x1224);
        if (n <= 0) {
            continue;
        }
        for (int32_t i = 0; i < n; i++) {
            void* tree = lsAt<void*>(self, 0x115C + i * 4);
            if (lsScript_PedUsesNode(self, ped, tree)) {
                lsPedActions_4840A0(lsAt<void*>(ped, 0x2E0));
                lsPed_474090(ped);
            } else {
                if (lsAt<void*>(lsAt<uint8_t*>(ped, 0x2E0), 0x4B0) == tree) {
                    lsPed_4740A0(ped);
                }
                if (lsAt<void*>(lsAt<uint8_t*>(ped, 0x2E0), 0x4B4) == tree) {
                    lsPed_4740B0(ped);
                }
            }
        }
    }
    for (int32_t i = lsAt<int32_t>(self, 0x1224) - 1; i >= 0; i--) {
        void* tree = lsAt<void*>(self, 0x115C + i * 4);
        for (int32_t t = int32_t(lsTreeCount) - 1; t >= 0; t--) {
            int32_t* entry = lsTrees + t * 3;
            if (reinterpret_cast<void*>(entry[1]) != tree) {
                continue;
            }
            entry[0]--;
            for (uint32_t j = uint32_t(i); j < lsAt<uint32_t>(self, 0x1224) - 1; j++) {
                lsAt<int32_t>(self, 0x115C + j * 4) = lsAt<int32_t>(self, 0x1160 + j * 4);
            }
            lsAt<int32_t>(self, 0x1224)--;
            if (entry[0] == 0) {
                lsTree_Release(reinterpret_cast<void*>(entry[1]), 1);
                entry[1] = 0;
                entry[2] = 0;
            }
            break;
        }
    }
    for (int32_t t = int32_t(lsTreeCount) - 1; t >= 0; t--) {
        int32_t* entry = lsTrees + t * 3;
        if (entry[1] != 0) {
            continue;
        }
        int32_t* e = entry;
        for (uint32_t j = uint32_t(t); j < lsTreeCount - 1; j++) {
            e[0] = e[3];
            e[1] = e[4];
            e[2] = e[5];
            e += 3;
        }
        lsTreeCount--;
    }
}

// The action files' class id (a static set on first use)
uint16_t lsGetActionFileType() {
    if ((lsActionFileTypeGuard & 1) == 0) {
        lsActionFileTypeGuard |= 1;
        lsActionFileType = lsNextTypeId;
        lsNextTypeId = uint16_t(lsNextTypeId + 1);
    }
    return lsActionFileType;
}

// Whether the ped runs the node (its three node lists or its three current nodes)
bool lsPedRunsNode(uint8_t* ped, void* node) {
    return lsNodeList_Contains(lsAt<uint8_t*>(ped, 0x2E0) + 0x258, node, 1, 0)
           || lsNodeList_Contains(lsAt<uint8_t*>(ped, 0x2E0) + 0x290, node, 1, 0)
           || lsNodeList_Contains(lsAt<uint8_t*>(ped, 0x2E0) + 0x2C8, node, 1, 0)
           || lsAt<void*>(lsAt<uint8_t*>(ped, 0x2E0), 0x398) == node || lsAt<void*>(lsAt<uint8_t*>(ped, 0x2E0), 0x340) == node
           || lsAt<void*>(lsAt<uint8_t*>(ped, 0x2E0), 0x3F0) == node;
}

void lsReleaseActionFiles(uint8_t* self, int array, int count, const char* names) {
    for (uint32_t i = 0; i < lsAt<uint32_t>(self, count); i++) {
        uint8_t* entry = self + array + i * 8;
        if (lsAt<void*>(entry, 4) == nullptr) {
            continue;
        }
        uint16_t type = lsGetActionFileType();
        if (lsVCall<uint8_t>(lsAt<void*>(entry, 4), 4, uint32_t(type))) {
            void* file = lsAt<void*>(entry, 4);
            if (file != nullptr) {
                lsActionFile_5F6670(file);
                lsActionFile_5F56B0(file, names + lsAt<int32_t>(entry, 0) * 0x40);
                lsActionFile_5F6CE0(file);
            }
        }
        lsAt<int32_t>(entry, 4) = 0;
        lsAt<int32_t>(entry, 0) = -1;
    }
}

// 0x5DA4D0: the action files released: peds running one reset, then each file (still an
// action file) reloaded under its name (the tables at 0x9102D0 and 0x911050)
void __fastcall LuaScript_ReleaseActionFiles(uint8_t* self, re::Edx) {
    LsPool* peds = lsPedPool;
    int32_t pedCount = peds->size;
    for (int32_t slot = 0; slot < pedCount; slot++) {
        if ((peds->flags[slot] & 0x80) != 0) {
            continue;
        }
        uint8_t* ped = peds->objects + peds->elemSize * slot;
        if (ped == nullptr) {
            continue;
        }
        for (uint32_t i = 0; i < lsAt<uint32_t>(self, 0x2A60); i++) {
            void* node = lsAt<void*>(self, 0x28D4 + i * 8);
            if (node != nullptr && lsPedRunsNode(ped, node)) {
                lsPed_474090(ped);
            }
        }
        for (uint32_t i = 0; i < lsAt<uint32_t>(self, 0x2B04); i++) {
            void* node = lsAt<void*>(self, 0x2A68 + i * 8);
            if (node != nullptr && lsPedRunsNode(ped, node)) {
                lsPed_474090(ped);
            }
        }
    }
    lsReleaseActionFiles(self, 0x28D0, 0x2A60, reinterpret_cast<const char*>(0x009102D0));
    lsReleaseActionFiles(self, 0x2A64, 0x2B04, reinterpret_cast<const char*>(0x00911050));
}

// 0x5DA7D0: a blip with no flag (-1), heading 0
void __fastcall LuaScript_AddBlipSimple(uint8_t* self, re::Edx, int handle) {
    lsScript_AddBlip(self, handle, -1, 0.0f, 0.0f);
}

// 0x5DA7F0: every model released (type 4 models their own way)
void __fastcall LuaScript_ReleaseModels(uint8_t* self, re::Edx) {
    for (uint32_t i = 0; i < lsAt<uint32_t>(self, 0x26A4); i++) {
        int32_t model = lsAt<int32_t>(self, 0x22A4 + i * 4);
        if (lsAt<uint8_t>(lsModelInfoPtrs[model], 8) == 4) {
            lsModel_4D9850(model);
        } else {
            lsSetModelNotNeeded(model);
        }
    }
    lsAt<int32_t>(self, 0x26A4) = 0;
}

// 0x5DA860: the script's vehicles deleted (the player's own kept, handed to 0x4CB230)
void __fastcall LuaScript_RemoveVehicles(uint8_t* self, re::Edx) {
    uint8_t* playerVehicle = nullptr;
    if (lsPlayerPed != nullptr) {
        playerVehicle = lsAt<uint8_t*>(lsPlayerPed, 0x1560);
    }
    if (lsAt<uint32_t>(self, 0x21D4) == 0) {
        lsAt<int32_t>(self, 0x21D4) = 0;
        return;
    }
    LsPool* pool = lsVehiclePool;
    for (uint32_t i = 0; i < lsAt<uint32_t>(self, 0x21D4); i++) {
        int32_t handle = lsAt<int32_t>(self, 0x210C + i * 4);
        int slot = handle >> 8;
        if (pool->flags[slot] != uint8_t(handle)) {
            continue;
        }
        uint8_t* vehicle = pool->objects + pool->elemSize * slot;
        if (vehicle == nullptr) {
            continue;
        }
        if (playerVehicle == vehicle) {
            lsVehicle_4CB230(vehicle, 1);
        } else {
            uint8_t* v = pool->flags[slot] == uint8_t(handle) ? pool->objects + pool->elemSize * slot : nullptr;
            lsSetModelNotNeeded(lsAt<int16_t>(v, 0x10E));
            lsDeleteVehicle(lsAt<uint16_t>(self, 0x210C + i * 4));
        }
        pool = lsVehiclePool;
    }
    lsAt<int32_t>(self, 0x21D4) = 0;
}

// 0x5DAA20: everything the script made or holds released
void __fastcall LuaScript_Cleanup(uint8_t* self, re::Edx) {
    lsScript_ReleaseTrees(self);
    lsScript_ReleaseActionFiles(self);
    lsScript_RemoveBlips(self);
    lsScript_RemovePeds(self);
    lsScript_RemoveObjects(self);
    lsScript_RemoveVehicles(self);
    lsScript_Remove21D8(self);
    lsScript_ReleaseModels(self);
    lsScript_ReleaseAgrGroups(self);
    lsScript_ReleaseAnimGroups(self);
    lsCleanup_49A650();
    lsManager_4A1C00(lsManagerC2C12C, self);
    lsManager_46EBC0(lsManagerC2AB68, self);
    lsManager_4729D0(lsManagerC2B034, self);
}

// 0x5DAAA0: the destructor (0xC2B210 told first)
void __fastcall LuaScript_Destruct(uint8_t* self, re::Edx) {
    lsManager_488420(reinterpret_cast<void*>(0x00C2B210), self);
    lsScript_Cleanup(self);
}

// ---- the callbacks (16 bytes: script id, thread id, function reference, owner)

void lsCallbackRelease(int32_t* callback) {
    uint8_t* script = lsLurStore_FindScript(lsLurStore, callback[0]);
    if (script != nullptr) {
        lsLuaL_unref(lsScript_GetThreadState(script, callback[1]), LUA_REGISTRYINDEX, callback[2]);
    }
    callback[0] = -1;
    callback[1] = -1;
    callback[2] = -1;
    *reinterpret_cast<int32_t*>(callback[3]) = 0;
    callback[3] = 0;
}

// 0x5D9BC0: released (the reference dropped in its thread's state; the owner cleared)
void __fastcall Callback_Release(int32_t* self, re::Edx) {
    if (self[3] != 0) {
        lsCallbackRelease(self);
    }
}

// 0x5D9C20 (on the owner): the callback's thread's lua_State (nullptr: none)
lua_State* __fastcall CallbackRef_GetState(re::Ptr32<int32_t>* self, re::Edx) {
    if (*self != nullptr) {
        uint8_t* script = lsLurStore_FindScript(lsLurStore, (*self)[0]);
        if (script != nullptr) {
            return lsScript_GetThreadState(script, (*self)[1]);
        }
    }
    return nullptr;
}

// 0x5D9C50 (on the owner): the callback released
void __fastcall CallbackRef_Release(re::Ptr32<int32_t>* self, re::Edx) {
    if (*self != nullptr) {
        lsCallback_Release(*self);
    }
}

// 0x5D9C60: the callbacks of a script's thread released
void __cdecl Callbacks_ReleaseFor(int script, int thread) {
    int32_t* callback = reinterpret_cast<int32_t*>(0x00CFC9E0);
    do {
        if (callback[0] == script && callback[1] == thread) {
            lsCallback_Release(callback);
        }
        callback += 4;
    } while (reinterpret_cast<uintptr_t>(callback) < 0x00D02840);
}

// 0x5D9C90: the callbacks of a script released
void __cdecl Callbacks_ReleaseScript(int script) {
    int32_t* callback = reinterpret_cast<int32_t*>(0x00CFC9E0);
    do {
        if (callback[0] == script && callback[3] != 0) {
            lsCallbackRelease(callback);
        }
        callback += 4;
    } while (reinterpret_cast<uintptr_t>(callback + 2) < 0x00D02848);
}

// 0x5D9D00: every callback released
void __cdecl Callbacks_ReleaseAll() {
    int32_t* callback = reinterpret_cast<int32_t*>(0x00CFC9E0);
    do {
        if (callback[3] != 0) {
            lsCallbackRelease(callback);
        }
        callback += 4;
    } while (reinterpret_cast<uintptr_t>(callback) <= 0x00D0283F);
}

// 0x5DA950 (thiscall on the table, an atexit's): every callback released, the last first
void __fastcall Callbacks_Destruct(uint8_t* self, re::Edx) {
    int32_t* callback = reinterpret_cast<int32_t*>(self + 0x5E60);
    for (int n = 0x5E5; n >= 0; n--) {
        callback -= 4;
        if (callback[3] != 0) {
            lsCallbackRelease(callback);
        }
    }
}

// ---- the update

GAME_VAR(uint32_t, lsTimeInMs, 0x00C1A9B4);
LIB_FN(void, __cdecl, lsLua_settable, lua_settable, lua_State* L, int idx);
GAME_FN(void, __cdecl, lsProfileMark, 0x0049A650);  // (empty; given a name on the way in)
GAME_FN(void, __cdecl, lsKeepLoadingScreenAlive, 0x0043C060, int a, int b);
GAME_FN(uint8_t, __thiscall, lsRadar_GetBlipPos, 0x0054AC30, void* radar, int handle, ScriptVector* pos);
GAME_FN(void, __cdecl, lsMarker_52A600, 0x0052A600, int type, int a, int b, const ScriptVector* pos, float size, const ScriptVector* v,
        int c, int d, int e, int f, int g, int h, int i);
GAME_VAR(uint32_t, lsPedCheckIndex, 0x00D02840);

// 0x5DAAC0: a frame of the script: each thread but the first that is due (or ending)
// resumed with the global Alive set (false when ending): one that stops or finishes is
// killed, and an ending one too unless it's MissionCleanup; one ped handle checked
// (round robin) and dropped when stale; with byte +0x1150 and only the main thread left
// the script ends (state 4); its blips' markers drawn. `loading`: the loading screen
// kept alive in between.
void __fastcall LuaScript_Update(uint8_t* self, re::Edx, int loading) {
    reinterpret_cast<void(__cdecl*)(const char*)>(0x0049A650)("LuaScript_Update");
    if (lsAt<int32_t>(self, 0) == 0) {
        lsAt<int32_t>(self, 0) = 1;
    }
    for (uint32_t i = 1; i < lsAt<uint32_t>(self, 0x1148); i++) {
        uint8_t* thread = lsThread(self, i);
        if (re::Flag(loading)) {
            lsKeepLoadingScreenAlive(1, 0);
        }
        if (lsAt<uint8_t>(self, 0x2B08) != 0 && lsAt<uint8_t>(thread, 0x14) != 0 && lsAt<int32_t>(thread, 0x18) < 5) {
            lsAt<int32_t>(thread, 0x18)++;
        }
        int32_t state = lsAt<int32_t>(thread, 0x10);
        if ((state == 1 || state == 0)
            && (lsAt<int32_t>(thread, 0x1C) < 0 || lsAt<int32_t>(self, 0) == 2 || lsAt<int32_t>(thread, 0x18) >= 5)) {
            lsAt<int32_t>(thread, 0x10) = 2;
        }
        if (!(lsTimeInMs > lsAt<uint32_t>(thread, 0x1C)) && lsAt<int32_t>(thread, 0x10) != 2) {
            continue;
        }
        if (lsAt<int32_t>(thread, 0x10) == 0) {
            lsAt<int32_t>(thread, 0x10) = 1;
        }
        lua_State* L = lsAt<lua_State*>(thread, 0);
        lua::pushlstring(L, "Alive", 5);
        lua::pushboolean(L, lsAt<int32_t>(thread, 0x10) != 2);
        lsLua_settable(L, LS_LUA_GLOBALSINDEX);
        if (re::Flag(loading)) {
            lsKeepLoadingScreenAlive(1, 0);
        }
        lsAt<uint32_t>(self, 0x114C) = i;
        int result = lsLua_resume(L, 0);
        lsAt<int32_t>(self, 0x114C) = -1;
        if (re::Flag(loading)) {
            lsKeepLoadingScreenAlive(1, 0);
        }
        bool kill = false;
        if (result != 0) {
            kill = true;
        } else {
            int32_t ar[0x18];  // lua_Debug
            ar[0] = 0;
            lsMemset(&ar[1], 0, 0x5C);
            if (lsLua_getstack(L, 0, ar) == 0 && lua::gettop(L) == 0) {
                kill = true;
            } else if (lsAt<int32_t>(thread, 0x10) == 2) {
                const char* a = reinterpret_cast<const char*>(thread + 0x24);
                const char* b = "MissionCleanup";
                bool same = true;
                for (int k = 0; k < 15; k++) {
                    if (a[k] != b[k]) {
                        same = false;
                        break;
                    }
                }
                if (!same) {
                    lsAt<int32_t>(thread, 0x10) = 3;
                    lsScript_KillThread(self, i);
                    i--;
                }
                continue;
            }
        }
        if (kill) {
            lsAt<int32_t>(thread, 0x10) = 4;
            lsScript_KillThread(self, i);
            i--;
        }
    }
    if (re::Flag(loading)) {
        lsKeepLoadingScreenAlive(1, 0);
    }
    uint32_t pedCount = lsAt<uint32_t>(self, 0x203C);
    if (pedCount != 0) {
        if (lsPedCheckIndex >= pedCount) {
            lsPedCheckIndex = 0;
        }
        if (lsGetPed(lsAt<int32_t>(self, 0x1D6C + lsPedCheckIndex * 4), 2) == nullptr) {
            uint32_t last = lsAt<uint32_t>(self, 0x203C);
            lsAt<int32_t>(self, 0x1D6C + lsPedCheckIndex * 4) = lsAt<int32_t>(self, 0x1D68 + last * 4);
            lsAt<int32_t>(self, 0x203C)--;
        } else {
            lsPedCheckIndex++;
        }
    }
    if (lsAt<uint8_t>(self, 0x1150) != 0 && lsAt<uint32_t>(self, 0x1148) < 2) {
        lsScript_KillThread(self, 0);
        lsAt<int32_t>(self, 0) = 4;
    }
    void* radar = lsRadar;
    ScriptVector pos = {0.0f, 0.0f, 0.0f};
    for (uint32_t i = 0; i < lsAt<uint32_t>(self, 0x1D68); i++) {
        uint8_t* blip = self + 0x1228 + i * 0x10;
        if (lsAt<uint8_t>(blip, 4) == 0xFF) {
            continue;
        }
        if (lsRadar_GetBlipPos(radar, lsAt<int32_t>(blip, 0), &pos)) {
            ScriptVector v = {lsAt<float>(blip, 0xC), 0.0f, 0.0f};
            lsMarker_52A600(lsAt<uint8_t>(blip, 4), -1, 0, &pos, lsAt<float>(blip, 8), &v, -1, 0, 0x72, 0xBC, 0, 1, 5);
        }
    }
    lsProfileMark();
}

} // namespace

#define LS_REVERSED(name, address, fn)                  \
    static void* RE_CONCAT(lsOriginal_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(lsOriginal_, __LINE__), re::Mode::Original)

LS_REVERSED("CLuaScript::Threads_RemoveAt", 0x005D8820, &Threads_RemoveAt);
LS_REVERSED("CLuaScript::Blips_RemoveAt", 0x005D8870, &Blips_RemoveAt);
LS_REVERSED("CLuaScript::Peds_RemoveAt", 0x005D88D0, &Dwords_RemoveAt<0x2D0>);
LS_REVERSED("CLuaScript::Vehicles_RemoveAt", 0x005D8910, &Dwords_RemoveAt<0xC8>);
LS_REVERSED("CLuaScript::Models_RemoveAt", 0x005D8950, &Dwords_RemoveAt<0x400>);
LS_REVERSED("CLuaScript::Init", 0x005D8AB0, &LuaScript_Init);
LS_REVERSED("CLuaScript::GetThreadState", 0x005D8B50, &LuaScript_GetThreadState);
LS_REVERSED("CLuaScript::GetCurrentThread", 0x005D8B90, &LuaScript_GetCurrentThread);
LS_REVERSED("CLuaScript::GetCurrentThreadId", 0x005D8BC0, &LuaScript_GetCurrentThreadId);
LS_REVERSED("CLuaScript::SetCurrentThread", 0x005D8BF0, &LuaScript_SetCurrentThread);
LS_REVERSED("CLuaScript::AddThread", 0x005D8DD0, &LuaScript_AddThread);
LS_REVERSED("CLuaScript::CreateNamespace", 0x005D8EA0, &LuaScript_CreateNamespace);
LS_REVERSED("CLuaScript::IsThreadRunning", 0x005D8F10, &LuaScript_IsThreadRunning);
LS_REVERSED("CLuaScript::CreateThreadIn", 0x005D8FA0, &LuaScript_CreateThreadIn);
LS_REVERSED("CLuaScript::ResetThreads1C", 0x005D9050, &LuaScript_ResetThreads1C);
LS_REVERSED("CLuaScript::UseActionTree", 0x005D9150, &LuaScript_UseActionTree);
LS_REVERSED("CLuaScript::StopPropAnim", 0x005D9230, &LuaScript_StopPropAnim);
LS_REVERSED("CLuaScript::AddActionFile_5D9290", 0x005D9290, (&LuaScript_AddActionFile<0x28D0, 0x2A60>));
LS_REVERSED("CLuaScript::AddActionFile_5D92E0", 0x005D92E0, (&LuaScript_AddActionFile<0x2A64, 0x2B04>));
LS_REVERSED("CLuaScript::RemoveBlips", 0x005D9330, &LuaScript_RemoveBlips);
LS_REVERSED("CLuaScript::AddBlip", 0x005D9380, &LuaScript_AddBlip);
LS_REVERSED("CLuaScript::RemoveBlip", 0x005D9400, &LuaScript_RemoveBlip);
LS_REVERSED("CLuaScript::AddModel", 0x005D9450, (&LuaScript_AddOnce<0x22A4, 0x26A4>));
LS_REVERSED("CLuaScript::ReleaseModel", 0x005D94A0, &LuaScript_ReleaseModel);
LS_REVERSED("CLuaScript::ReleaseAgrGroups", 0x005D94F0, &LuaScript_ReleaseAgrGroups);
LS_REVERSED("CLuaScript::AddAgrGroup", 0x005D9550, (&LuaScript_AddOnce<0x26A8, 0x26E8>));
LS_REVERSED("CLuaScript::ReleaseAgrGroup", 0x005D95A0, &LuaScript_ReleaseAgrGroup);
LS_REVERSED("CLuaScript::AddPed", 0x005D9630, (&LuaScript_AddOnce<0x1D6C, 0x203C>));
LS_REVERSED("CLuaScript::RemovePedHandle", 0x005D9680, &LuaScript_RemovePedHandle);
LS_REVERSED("CLuaScript::RemovePed", 0x005D96C0, &LuaScript_RemovePed);
LS_REVERSED("CLuaScript::RemovePeds", 0x005D9750, &LuaScript_RemovePeds);
LS_REVERSED("CLuaScript::AddObject", 0x005D97D0, &LuaScript_AddObject);
LS_REVERSED("CLuaScript::RemoveObject", 0x005D97F0, &LuaScript_RemoveObject);
LS_REVERSED("CLuaScript::RemoveObjects", 0x005D9860, &LuaScript_RemoveObjects);
LS_REVERSED("CLuaScript::Add21D8", 0x005D98C0, &LuaScript_Add21D8);
LS_REVERSED("CLuaScript::Remove21D8", 0x005D9900, &LuaScript_Remove21D8);
LS_REVERSED("CLuaScript::Remove21D8All", 0x005D9990, &LuaScript_Remove21D8All);
LS_REVERSED("CLuaScript::AddVehicle", 0x005D99E0, &LuaScript_AddVehicle);
LS_REVERSED("CLuaScript::RemoveVehicle", 0x005D9A00, &LuaScript_RemoveVehicle);
LS_REVERSED("CLuaScript::FindThread", 0x005D9A40, &LuaScript_FindThread);
LS_REVERSED("CLuaScript::ReleaseAnimGroups", 0x005D9A80, &LuaScript_ReleaseAnimGroups);
LS_REVERSED("CLuaScript::AddAnimGroup", 0x005D9AF0, &LuaScript_AddAnimGroup);
LS_REVERSED("ScriptCallback::Release", 0x005D9BC0, &Callback_Release);
LS_REVERSED("ScriptCallbackRef::GetState", 0x005D9C20, &CallbackRef_GetState);
LS_REVERSED("ScriptCallbackRef::Release", 0x005D9C50, &CallbackRef_Release);
LS_REVERSED("ScriptCallbacks::ReleaseFor", 0x005D9C60, &Callbacks_ReleaseFor);
LS_REVERSED("ScriptCallbacks::ReleaseScript", 0x005D9C90, &Callbacks_ReleaseScript);
LS_REVERSED("ScriptCallbacks::ReleaseAll", 0x005D9D00, &Callbacks_ReleaseAll);
LS_REVERSED("CLuaScript::Start", 0x005D9DB0, &LuaScript_Start);
LS_REVERSED("CLuaScript::End", 0x005D9E40, &LuaScript_End);
LS_REVERSED("CLuaScript::CreateThread", 0x005D9E80, &LuaScript_CreateThread);
LS_REVERSED("CLuaScript::KillThread", 0x005D9E90, &LuaScript_KillThread);
LS_REVERSED("CLuaScript::KillThreadWithId", 0x005D9F80, &LuaScript_KillThreadWithId);
LS_REVERSED("CLuaScript::ReleaseTrees", 0x005DA170, &LuaScript_ReleaseTrees);
LS_REVERSED("CLuaScript::ReleaseActionFiles", 0x005DA4D0, &LuaScript_ReleaseActionFiles);
LS_REVERSED("CLuaScript::AddBlipSimple", 0x005DA7D0, &LuaScript_AddBlipSimple);
LS_REVERSED("CLuaScript::ReleaseModels", 0x005DA7F0, &LuaScript_ReleaseModels);
LS_REVERSED("CLuaScript::RemoveVehicles", 0x005DA860, &LuaScript_RemoveVehicles);
LS_REVERSED("ScriptCallbacks::Destruct", 0x005DA950, &Callbacks_Destruct);
LS_REVERSED("CLuaScript::Cleanup", 0x005DAA20, &LuaScript_Cleanup);
LS_REVERSED("CLuaScript::Destruct", 0x005DAAA0, &LuaScript_Destruct);
LS_REVERSED("CLuaScript::Update", 0x005DAAC0, &LuaScript_Update);
