// CLurStore (TheLurStore, 0xD02850): the compiled scripts (.lur, streamed as model ids
// from 0x5DB9), the Lua state, the running scripts (CLuaScript) and the action trees
// and .act files they use. Its layout, as used here:
//   +0      trees[50] {refs, tree, name hash}      +0x258  count
//   +0x25C  act files[192], 0x8C each {name hash, path[128], tree, byte}   +0x6B5C count
//   +0x6B60 the lua_State
//   +0x6B64 scripts[8]    +0x6B84 count    +0x6B88 the current one (-1: none)
//   +0x6B8C lurs[75] (0x2C each: +0 size, +4 data, +0xC loaded, +0x10 index,
//           +0x14 name[20], +0x28 20000)    +0x6CB8 count

#include <cstdint>

#include "game/ScriptApi.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

template <typename T>
re::Field<T>& lurAt(void* base, int offset) {
    return *reinterpret_cast<re::Field<T>*>(static_cast<uint8_t*>(base) + offset);
}

template <typename R, typename... A>
R lurVCall(void* self, int offset, A... args) {
    using Fn = R(__thiscall*)(void*, re::Arg<A>...);
    Fn fn = re::GameFnAt<Fn>(*static_cast<re::Ptr32<uint8_t>*>(self) + offset);
    return fn(self, args...);
}

inline constexpr int LUR_SCRIPTS = 0x6B64;
inline constexpr int LUR_SCRIPT_COUNT = 0x6B84;
inline constexpr int LUR_CURRENT = 0x6B88;
inline constexpr int LUR_LURS = 0x6B8C;
inline constexpr int LUR_LUR_COUNT = 0x6CB8;
inline constexpr int LUR_ACT = 0x25C;
inline constexpr int LUR_ACT_COUNT = 0x6B5C;
inline constexpr int LUR_ACT_SIZE = 0x8C;
inline constexpr int LUR_LUA_GLOBALSINDEX = -10001;

lua_State* lurState(uint8_t* self) {
    return lurAt<lua_State*>(self, 0x6B60);
}
re::Ptr32<uint8_t>& lurScript(uint8_t* self, uint32_t i) {
    return lurAt<uint8_t*>(self, LUR_SCRIPTS + i * 4);
}
uint8_t* lurLur(uint8_t* self, int i) {
    return lurAt<uint8_t*>(self, LUR_LURS + i * 4);
}

// Lua
LIB_FN(void, __cdecl, lurLua_gettable, lua_gettable, lua_State* L, int idx);
LIB_FN(void, __cdecl, lurLua_settable, lua_settable, lua_State* L, int idx);
LIB_FN(void, __cdecl, lurLua_pushcclosure, lua_pushcclosure, lua_State* L, void* fn, int n);
LIB_FN(int, __cdecl, lurLua_setmetatable, lua_setmetatable, lua_State* L, int idx);
LIB_FN(int, __cdecl, lurLua_pcall, lua_pcall, lua_State* L, int nargs, int nresults, int errfunc);
LIB_FN(void, __cdecl, lurLua_setgcthreshold, lua_setgcthreshold, lua_State* L, int threshold);
LIB_FN(void*, __cdecl, lurLua_atpanic, lua_atpanic, lua_State* L, void* fn);
LIB_FN(lua_State*, __cdecl, lurLua_open, lua_open);
LIB_FN(void, __cdecl, lurLua_close, lua_close, lua_State* L);
LIB_FN(int, __cdecl, lurLuaopen_73DD20, luaopen_base, lua_State* L);
LIB_FN(int, __cdecl, lurLuaopen_743950, luaopen_table, lua_State* L);
LIB_FN(int, __cdecl, lurLuaopen_742510, luaopen_string, lua_State* L);
LIB_FN(int, __cdecl, lurLuaopen_741630, luaopen_math, lua_State* L);
LIB_FN(int, __cdecl, lurLuaopen_73E6E0, luaopen_debug, lua_State* L);
inline void* const lurErrorHandler = reinterpret_cast<void*>(0x00824EC0);

// the memory
GAME_FN(void*, __cdecl, lurMalloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, lurFree, 0x005EEAB0, void* p);
inline void* __cdecl lurMemcpy(void* dst, const void* src, uint32_t size) { return crt::Memcpy(dst, src, (size_t)size); }
inline const char* __cdecl lurStrRChr(const char* s, int c) { return (const char*)crt::Strrchr(s, c); }
inline const char* __cdecl lurStrStr(const char* s, const char* sub) { return (const char*)crt::Strstr(s, sub); }
inline int __cdecl lurStricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
GAME_FN(uint32_t, __cdecl, lurHashName, 0x00576ED0, const char* name);
GAME_FN(void, __cdecl, lurRequestModel, 0x00532020, int id, int flags);

// the scripts' (through the game's addresses)
GAME_FN(void, __cdecl, lurUpdateLuaMemory, 0x005D8A00);  // (called with two strings it ignores)
GAME_FN(void, __cdecl, lurLoadChunk, 0x005D8C40, lua_State* L, const char* name);
GAME_FN(uint8_t*, __thiscall, lurScript_Init, 0x005D8AB0, void* script, const char* name);
GAME_FN(void, __thiscall, lurScript_UseActionTree, 0x005D9150, void* script, void* node, const char* name);
GAME_FN(void, __thiscall, lurScript_RemoveBlip, 0x005D9400, void* script, int handle);
GAME_FN(uint8_t*, __thiscall, lurScript_FindThread, 0x005D9A40, void* script, lua_State* L);
GAME_FN(void, __thiscall, lurScript_RemovePed, 0x005D96C0, void* script, void* ped);
GAME_FN(void, __thiscall, lurScript_Start, 0x005D9DB0, void* script, lua_State* L);
GAME_FN(void, __thiscall, lurScript_End, 0x005D9E40, void* script, int flag);
GAME_FN(uint8_t*, __thiscall, lurScript_CreateThread, 0x005D9E80, void* script, const char* function);
GAME_FN(void, __thiscall, lurScript_Update, 0x005DAAC0, void* script, int loading);
GAME_FN(void, __thiscall, lurScript_Destruct, 0x005DAAA0, void* script);
GAME_FN(void*, __cdecl, lurScript_New, 0x005D9DA0, uintptr_t size);
GAME_FN(void, __cdecl, lurScript_Delete, 0x005DA9F0, void* script);
GAME_FN(void, __cdecl, lurCallbacks_ReleaseAll, 0x005D9D90);  // (a thunk to 0x5D9D00)
// the store's own
GAME_FN(void, __thiscall, lurStore_RegisterFunctions, 0x005DB140, void* store, const void* functions);
GAME_FN(void, __thiscall, lurStore_RegisterEnum, 0x005DB1A0, void* store, const char* name, void* index);
GAME_FN(void, __thiscall, lurStore_CreateLuaState, 0x005DB3C0, void* store);
GAME_FN(void, __thiscall, lurStore_Shutdown, 0x005DB990, void* store);
GAME_FN(void, __thiscall, lurStore_RemoveCurrent, 0x005DBAA0, void* store, int keepMissionState);
GAME_FN(uint8_t*, __thiscall, lurStore_FindScriptByName, 0x005DBCC0, void* store, const char* name);
GAME_FN(void, __thiscall, lurStore_SetCurrent, 0x005DBDA0, void* store, void* script);
GAME_FN(uint8_t*, __thiscall, lurStore_LaunchScript, 0x005DC0F0, void* store, const char* name, int mainThread);
GAME_FN(void, __thiscall, lurActFiles_RemoveAt, 0x005DB870, void* files, uint32_t index);

// elsewhere
GAME_FN(void, __thiscall, lurMission_6D2210, 0x006D2210, void* self, int a);
inline void* const lurMission20C7C48 = reinterpret_cast<void*>(0x020C7C48);
GAME_VAR(int32_t, lurSavePointLocation, 0x020CECE0);
GAME_FN(void, __cdecl, lurReset_56EDE0, 0x0056EDE0);
GAME_FN(void, __cdecl, lurKeepLoadingScreenAlive, 0x0043C060, int a, int b);
GAME_VAR(uint8_t, lurUpdating, 0x00D0950C);
GAME_VAR(uint8_t*, lurManagerC0F5E4, 0x00C0F5E4);
GAME_FN(void, __thiscall, lurManager_449020, 0x00449020, void* manager, uintptr_t a);
GAME_FN(int32_t, __thiscall, lurTriggers_Find, 0x006D7A10, void* triggers, const char* name);
GAME_FN(int32_t, __thiscall, lurPointLists_Find, 0x006D63C0, void* lists, const char* name);
GAME_FN(int32_t, __thiscall, lurPaths_Find, 0x005ED310, void* paths, const char* name);
GAME_VAR(void*, lurPaths, 0x00D13CB4);
GAME_FN(void*, __cdecl, lurActionTree_Find, 0x005F5B10, void* name, uintptr_t a);
GAME_VAR(void*, lurTreeRoot, 0x01B649DC);
GAME_ARRAY(uint8_t*, lurModelInfoPtrs, 0x00C67738);
GAME_FN(int16_t, __cdecl, lurAnimFile_Find, 0x00520960, const char* name, uintptr_t a);
// the script image's directory
GAME_VAR(uint8_t*, lurScriptImgDir, 0x00D02848);
GAME_FN(void, __thiscall, lurDirectory_Construct, 0x004291B0, void* dir, int capacity);
GAME_FN(void, __thiscall, lurDirectory_ReadDir, 0x004291F0, void* dir, const char* name);
GAME_FN(void*, __cdecl, lurFileMgr_Open, 0x0042D260, const char* name, const char* mode, int a);
GAME_FN(int, __cdecl, lurFileMgr_Close, 0x0042D450, void* file);

// ---- the lurs

// 0x5DAF40: a full garbage collection
void __fastcall LurStore_CollectGarbage(uint8_t* self, re::Edx) {
    lurLua_setgcthreshold(lurState(self), 0);
    lurLua_setgcthreshold(lurState(self), 1000000000);
}

// 0x5DAF70: a new lur of that name (not loaded); its index. (The name isn't bounded;
// a failed allocation is written through anyway.)
int __fastcall LurStore_Add(uint8_t* self, re::Edx, const char* name) {
    uint8_t* lur = static_cast<uint8_t*>(lurMalloc(0x2C));
    if (lur != nullptr) {
        lurAt<int32_t>(lur, 0) = 0;
        lurAt<int32_t>(lur, 4) = 0;
        lurAt<int32_t>(lur, 8) = 0;
        lurAt<uint8_t>(lur, 0xC) = 0;
        lurAt<uint8_t>(lur, 0xD) = 0;
        lurAt<int32_t>(lur, 0x10) = 0;
        lurAt<uint8_t>(lur, 0x14) = 0;
    }
    lurAt<uint8_t>(lur, 0xC) = 0;
    lurAt<int32_t>(lur, 8) = 0;
    lurAt<uint8_t>(lur, 0xD) = 0;
    lurAt<int32_t>(lur, 0x10) = lurAt<int32_t>(self, LUR_LUR_COUNT);
    lurAt<int32_t>(lur, 0x28) = 20000;
    char* d = reinterpret_cast<char*>(lur + 0x14);
    char c;
    do {
        c = *name++;
        *d++ = c;
    } while (c != '\0');
    lurAt<uint8_t*>(self, LUR_LURS + lurAt<int32_t>(self, LUR_LUR_COUNT) * 4) = lur;
    lurAt<int32_t>(self, LUR_LUR_COUNT)++;
    return lurAt<int32_t>(lur, 0x10);
}

// 0x5DAFF0: the lur of that name (any case; -1: none)
int __fastcall LurStore_Find(uint8_t* self, re::Edx, const char* name) {
    for (int i = 0; i < lurAt<int32_t>(self, LUR_LUR_COUNT); i++) {
        uint8_t* lur = lurLur(self, i);
        if (lur != nullptr && lurStricmp(name, reinterpret_cast<const char*>(lur + 0x14)) == 0) {
            return lurAt<int32_t>(lur, 0x10);
        }
    }
    return -1;
}

// 0x5DB050: every lur requested
void __fastcall LurStore_RequestAll(uint8_t* self, re::Edx) {
    for (int i = 0; i < lurAt<int32_t>(self, LUR_LUR_COUNT); i++) {
        lurRequestModel(i + 0x5DB9, 9);
    }
}

// 0x5DB080 / 0x5DB0A0 / 0x5DB0C0: a lur's name, data, size
const char* __fastcall LurStore_GetName(uint8_t* self, re::Edx, int i) {
    return reinterpret_cast<const char*>(lurLur(self, i) + 0x14);
}
void* __fastcall LurStore_GetData(uint8_t* self, re::Edx, int i) {
    return lurAt<void*>(lurLur(self, i), 4);
}
uint32_t __fastcall LurStore_GetSize(uint8_t* self, re::Edx, int i) {
    return lurAt<uint32_t>(lurLur(self, i), 0);
}

// 0x5DB0D0: a lur's data streamed in (copied)
uint8_t __fastcall LurStore_Load(uint8_t* self, re::Edx, int i, const void* data, uint32_t size) {
    uint8_t* lur = lurLur(self, i);
    void* copy = lurMalloc(size);
    lurMemcpy(copy, data, size);
    lurAt<uint32_t>(lur, 0) = size;
    lurAt<void*>(lur, 4) = copy;
    lurAt<uint8_t>(lur, 0xC) = 1;
    return 1;
}

// 0x5DB110: a lur's data freed
uint8_t __fastcall LurStore_Remove(uint8_t* self, re::Edx, int i) {
    uint8_t* lur = lurLur(self, i);
    lurFree(lurAt<void*>(lur, 4));
    lurAt<void*>(lur, 4) = nullptr;
    lurAt<uint8_t>(lur, 0xC) = 0;
    lurAt<uint32_t>(lur, 0) = 0;
    return 1;
}

// ---- the Lua state

// 0x5DB140: natives made globals ({name, function} up to a null function)
void __fastcall LurStore_RegisterFunctions(uint8_t* self, re::Edx, const re::Ptr32<void>* functions) {
    int i = 0;
    void* fn = functions[1];
    const re::Ptr32<void>* entry = functions;
    while (fn != nullptr) {
        lua::pushstring(lurState(self), static_cast<const char*>(entry[0]));
        lurLua_pushcclosure(lurState(self), fn, 0);
        lurLua_settable(lurState(self), LUR_LUA_GLOBALSINDEX);
        i++;
        fn = functions[i * 2 + 1];
        entry = functions + i * 2;
    }
}

// 0x5DB1A0: a global table whose missing keys go to `index` (its metatable's __index)
void __fastcall LurStore_RegisterEnum(uint8_t* self, re::Edx, const char* name, void* index) {
    lua::newtable(lurState(self));
    lua::pushstring(lurState(self), name);
    lua::pushvalue(lurState(self), -2);
    lurLua_settable(lurState(self), LUR_LUA_GLOBALSINDEX);
    lua::newtable(lurState(self));
    lua::pushstring(lurState(self), "__index");
    lurLua_pushcclosure(lurState(self), index, 0);
    lurLua_settable(lurState(self), -3);
    lurLua_setmetatable(lurState(self), -2);
    lua::settop(lurState(self), -2);
}

// The __index functions of TRIGGER, POINTLIST and PATH: the key (a leading '_'
// skipped) looked up by name
const char* lurKey(lua_State* L) {
    const char* name = Script::GetString(L, 1);
    if (*name == '_') {
        name++;
    }
    return name;
}
// 0x5DB250
int __cdecl Lur_TriggerIndex(lua_State* L) {
    const char* name = lurKey(L);
    Script::PushInt(L, lurTriggers_Find(reinterpret_cast<void*>(0x020C7E98), name));
    return 1;
}
// 0x5DB290
int __cdecl Lur_PointListIndex(lua_State* L) {
    const char* name = lurKey(L);
    Script::PushInt(L, lurPointLists_Find(reinterpret_cast<void*>(0x00BC5E7C), name));
    return 1;
}
// 0x5DB380
int __cdecl Lur_PathIndex(lua_State* L) {
    const char* name = Script::GetString(L, 1);
    void* paths = lurPaths;
    if (*name == '_') {
        name++;
    }
    Script::PushInt(L, lurPaths_Find(paths, name));
    return 1;
}

// The native tables CreateLuaState registers, in its order
constexpr uintptr_t lurNativeTables[] = {
    0x00AE9DA8, 0x00AEAFA8, 0x00AEA2C8, 0x00AE9AA8, 0x00AE96F8, 0x00AE9418, 0x00AE9530, 0x00AE9100, 0x00AE9958, 0x00AEB510,
    0x00AEB200, 0x00AEB770, 0x00AEB6A8, 0x00AEA580, 0x00AE9B20, 0x00AE9890, 0x00AEA28C, 0x00AE9D40, 0x00AEB530, 0x00AE9990,
    0x00AEA310, 0x00AEAF38, 0x00AEA3C8, 0x00AEB240, 0x00AE9F78, 0x00AE9918, 0x00AE94E0, 0x00AEB2B8, 0x00B9DA18, 0x00BC6ED8,
    0x00AE9A78, 0x00AE9518, 0x00BC6598, 0x00BC6700, 0x00BC6660, 0x00BC63D8, 0x00BC64F8, 0x00BC6298, 0x00BC6040, 0x00BC5F54,
    0x00BC6AF0, 0x00BC6C4C, 0x00BC6FF8, 0x00BC7170, 0x00BC5FAC, 0x00BC7068, 0x00BC68E8, 0x00BC6008, 0x00BC6B5C, 0x00BC6890,
    0x00AE9B90, 0x00AEB674, 0x00AE9778, 0x00AEA25C, 0x00AE9858, 0x00AEB748, 0x00AE9A28, 0x00AEB5C8,
};
static_assert(sizeof(lurNativeTables) / sizeof(lurNativeTables[0]) == 58);

// 0x5DB3C0: the Lua state made: the error handler (__onerror, _ALERT, the panic), the
// libraries, every native, the enum tables, then util.lua run
void __fastcall LurStore_CreateLuaState(uint8_t* self, re::Edx) {
    reinterpret_cast<void(__cdecl*)(const char*, const char*)>(0x005D8A00)("before * opening * root * lua * state", "CreateLuaState");
    lurAt<lua_State*>(self, 0x6B60) = lurLua_open();
    reinterpret_cast<void(__cdecl*)(const char*, const char*)>(0x005D8A00)("before registration of anything", "CreateLuaState");
    lua::pushstring(lurState(self), "__onerror");
    lurLua_pushcclosure(lurState(self), lurErrorHandler, 0);
    lurLua_settable(lurState(self), LUR_LUA_GLOBALSINDEX);
    lua::pushstring(lurState(self), "_ALERT");
    lurLua_pushcclosure(lurState(self), lurErrorHandler, 0);
    lurLua_settable(lurState(self), LUR_LUA_GLOBALSINDEX);
    lurLua_atpanic(lurState(self), lurErrorHandler);
    lurLuaopen_73DD20(lurState(self));
    lurLuaopen_743950(lurState(self));
    lurLuaopen_742510(lurState(self));
    lurLuaopen_741630(lurState(self));
    lurLuaopen_73E6E0(lurState(self));
    lurLua_setgcthreshold(lurState(self), 1000000000);
    reinterpret_cast<void(__cdecl*)(const char*, const char*)>(0x005D8A00)("after reging libs %s -", "CreateLuaState");
    for (uintptr_t table : lurNativeTables) {
        lurStore_RegisterFunctions(self, reinterpret_cast<const void*>(table));
    }
    reinterpret_cast<void(__cdecl*)(const char*, const char*)>(0x005D8A00)("after reging funcs %s -", "CreateLuaState");
    lurStore_RegisterEnum(self, "PATH", reinterpret_cast<void*>(0x005DB380));
    lurStore_RegisterEnum(self, "TRIGGER", reinterpret_cast<void*>(0x005DB250));
    lurStore_RegisterEnum(self, "POINTLIST", reinterpret_cast<void*>(0x005DB290));
    lurStore_RegisterEnum(self, "MODELENUM", reinterpret_cast<void*>(0x005DB330));
    lurStore_RegisterEnum(self, "POI", reinterpret_cast<void*>(0x005DB2D0));
    lurLoadChunk(lurState(self), "util.lua");
    lurLua_pcall(lurState(self), 0, 0, 0);
    lurLua_setgcthreshold(lurState(self), 0);
    lurLua_setgcthreshold(lurState(self), 1000000000);
}

// ---- the action files and trees

// 0x5DB830: a path's name hash (after its last slash or backslash)
uint32_t __cdecl ActFile_HashName(const char* path) {
    const char* slash = lurStrRChr(path, '/');
    if (slash == nullptr) {
        slash = lurStrRChr(path, '\\');
        if (slash == nullptr) {
            return lurHashName(path);
        }
    }
    return lurHashName(slash + 1);
}

// 0x5DB870 (on +0x25C): an act file entry removed
void __fastcall ActFiles_RemoveAt(uint8_t* self, re::Edx, uint32_t index) {
    for (; index < lurAt<uint32_t>(self, 0x6900) - 1; index++) {
        uint32_t* dst = reinterpret_cast<uint32_t*>(self + index * LUR_ACT_SIZE);
        for (int i = 0; i < 0x23; i++) {
            dst[i] = dst[i + 0x23];
        }
    }
    lurAt<int32_t>(self, 0x6900)--;
}

// ---- the scripts

// 0x5DB8D0: script i (nullptr with none running)
uint8_t* __fastcall LurStore_GetScript(uint8_t* self, re::Edx, int i) {
    if (lurAt<int32_t>(self, LUR_SCRIPT_COUNT) == 0) {
        return nullptr;
    }
    return lurScript(self, uint32_t(i));
}

// 0x5DB8F0: the script with that id (nullptr: none)
uint8_t* __fastcall LurStore_FindScript(uint8_t* self, re::Edx, int id) {
    for (uint32_t i = 0; i < lurAt<uint32_t>(self, LUR_SCRIPT_COUNT); i++) {
        if (lurAt<int32_t>(lurScript(self, i), 0x44) == id) {
            return lurScript(self, i);
        }
    }
    return nullptr;
}

// 0x5DB940: the current script (unchecked index; nullptr with none running)
uint8_t* __fastcall LurStore_GetCurrentScript(uint8_t* self, re::Edx) {
    if (lurAt<int32_t>(self, LUR_SCRIPT_COUNT) == 0) {
        return nullptr;
    }
    return lurScript(self, lurAt<uint32_t>(self, LUR_CURRENT));
}

// 0x5DB960: the current script, nullptr unless there's a valid one
uint8_t* __fastcall LurStore_GetCurrentScriptChecked(uint8_t* self, re::Edx) {
    uint32_t current = lurAt<uint32_t>(self, LUR_CURRENT);
    if (current != 0xFFFFFFFF && current < lurAt<uint32_t>(self, LUR_SCRIPT_COUNT) && lurAt<uint32_t>(self, LUR_SCRIPT_COUNT) != 0) {
        return lurScript(self, current);
    }
    return nullptr;
}

// 0x5DB990: every script ended and freed, the Lua state closed
void __fastcall LurStore_Shutdown(uint8_t* self, re::Edx) {
    if (lurState(self) == nullptr) {
        return;
    }
    lurMission_6D2210(lurMission20C7C48, 2);
    lurMission_6D2210(lurMission20C7C48, 0);
    lurSavePointLocation = 0;
    lurReset_56EDE0();
    lurCallbacks_ReleaseAll();
    for (uint32_t i = 0; i < lurAt<uint32_t>(self, LUR_SCRIPT_COUNT); i++) {
        lurAt<uint32_t>(self, LUR_CURRENT) = i;
        lurScript_End(lurScript(self, i), 1);
        uint8_t* script = lurScript(self, i);
        if (script != nullptr) {
            lurScript_Destruct(script);
            lurScript_Delete(script);
        }
        lurScript(self, i) = nullptr;
    }
    lurAt<int32_t>(self, LUR_SCRIPT_COUNT) = 0;
    lurLua_setgcthreshold(lurState(self), 0);
    lurLua_setgcthreshold(lurState(self), 1000000000);
    lurLua_close(lurState(self));
    lurAt<lua_State*>(self, 0x6B60) = nullptr;
}

// 0x5DBA60: a ped out of every script's list
void __fastcall LurStore_RemovePed(uint8_t* self, re::Edx, void* ped) {
    if (lurState(self) == nullptr) {
        return;
    }
    for (uint32_t i = 0; i < lurAt<uint32_t>(self, LUR_SCRIPT_COUNT); i++) {
        lurScript_RemovePed(lurScript(self, i), ped);
    }
}

// 0x5DBAA0: the current script freed and taken out of the list (the mission state
// reset first unless `keep`)
void __fastcall LurStore_RemoveCurrent(uint8_t* self, re::Edx, int keep) {
    if (!re::Flag(keep)) {
        lurMission_6D2210(lurMission20C7C48, 2);
        lurMission_6D2210(lurMission20C7C48, 0);
    }
    uint8_t* script = lurScript(self, lurAt<uint32_t>(self, LUR_CURRENT));
    if (script != nullptr) {
        lurScript_Destruct(script);
        lurScript_Delete(script);
    }
    for (uint32_t i = lurAt<uint32_t>(self, LUR_CURRENT); i < lurAt<uint32_t>(self, LUR_SCRIPT_COUNT) - 1; i++) {
        lurScript(self, i) = lurScript(self, i + 1);
    }
    lurAt<int32_t>(self, LUR_SCRIPT_COUNT)--;
}

// 0x5DBB40: a frame of every script (not re-entered); finished ones (state 4) removed,
// the manager at 0xC0F5E4 dropping its +0x14C when it was theirs (then the mission
// state kept)
void __fastcall LurStore_Update(uint8_t* self, re::Edx, int loading) {
    if (lurUpdating != 0) {
        return;
    }
    lurUpdating = 1;
    for (uint32_t i = 0; i < lurAt<uint32_t>(self, LUR_SCRIPT_COUNT); i++) {
        lurAt<uint32_t>(self, LUR_CURRENT) = i;
        if (re::Flag(loading)) {
            lurKeepLoadingScreenAlive(1, 0);
        }
        lurScript_Update(lurScript(self, i), loading);
        if (re::Flag(loading)) {
            lurKeepLoadingScreenAlive(1, 0);
        }
        if (lurAt<int32_t>(lurScript(self, i), 0) != 4) {
            continue;
        }
        uint32_t hash = lurHashName(reinterpret_cast<const char*>(lurScript(self, i) + 4));
        uint8_t keep = 0;
        if (hash > 0) {
            uint8_t* manager = lurManagerC0F5E4;
            if (hash == lurAt<uint32_t>(manager, 0x14C)) {
                lurManager_449020(manager, 0);
                lurAt<uint32_t>(manager, 0x14C) = 0;
                keep = 1;
            }
        }
        if (re::Flag(loading)) {
            lurKeepLoadingScreenAlive(1, 0);
        }
        lurStore_RemoveCurrent(self, keep);
        i--;
    }
    lurAt<int32_t>(self, LUR_CURRENT) = -1;
    lurUpdating = 0;
}

// 0x5DBC40: whether a script has byte +0x2B09 (launched by 0x5DC180)
uint8_t __fastcall LurStore_Any2B09(uint8_t* self, re::Edx) {
    for (uint32_t i = 0; i < lurAt<uint32_t>(self, LUR_SCRIPT_COUNT); i++) {
        uint8_t* script = lurScript(self, i);
        if (script != nullptr && lurAt<uint8_t>(script, 0x2B09) != 0) {
            return 1;
        }
    }
    return 0;
}

// 0x5DBC80: those scripts get byte +0x2B08
void __fastcall LurStore_Set2B08(uint8_t* self, re::Edx) {
    for (uint32_t i = 0; i < lurAt<uint32_t>(self, LUR_SCRIPT_COUNT); i++) {
        uint8_t* script = lurScript(self, i);
        if (script != nullptr && lurAt<uint8_t>(script, 0x2B09) != 0) {
            lurAt<uint8_t>(script, 0x2B08) = 1;
        }
    }
}

// 0x5DBCC0: the script of that name (backslashes as slashes, any case; the name goes
// through the buffer at 0xC220A8)
uint8_t* __fastcall LurStore_FindScriptByName(uint8_t* self, re::Edx, const char* name) {
    char* buffer = reinterpret_cast<char*>(0x00C220A8);
    int n = 0;
    if (*name != '\0') {
        const char* p = name;
        do {
            buffer[p - name] = *p == '\\' ? '/' : *p;
            p++;
            n++;
        } while (*p != '\0');
    }
    buffer[n] = name[n];
    for (uint32_t i = 0; i < lurAt<uint32_t>(self, LUR_SCRIPT_COUNT); i++) {
        uint8_t* script = lurScript(self, i);
        if (lurStricmp(buffer, reinterpret_cast<const char*>(script + 4)) == 0) {
            return script;
        }
    }
    return nullptr;
}

// 0x5DBD50: the script running that lua_State (nullptr: none)
uint8_t* __fastcall LurStore_GetScriptOfState(uint8_t* self, re::Edx, lua_State* L) {
    for (uint32_t i = 0; i < lurAt<uint32_t>(self, LUR_SCRIPT_COUNT); i++) {
        uint8_t* script = lurScript(self, i);
        if (lurScript_FindThread(script, L) != nullptr) {
            return script;
        }
    }
    return nullptr;
}

// 0x5DBDA0: the current script (nullptr or not running: none)
void __fastcall LurStore_SetCurrent(uint8_t* self, re::Edx, uint8_t* script) {
    lurAt<int32_t>(self, LUR_CURRENT) = -1;
    if (script == nullptr) {
        return;
    }
    for (uint32_t i = 0; i < lurAt<uint32_t>(self, LUR_SCRIPT_COUNT); i++) {
        if (lurScript(self, i) == script) {
            lurAt<uint32_t>(self, LUR_CURRENT) = i;
            return;
        }
    }
}

// 0x5DBDF0: the running script of lur i's name
uint8_t* __fastcall LurStore_GetStreamedScript(uint8_t* self, re::Edx, int lur) {
    uint8_t* entry = lurLur(self, lur);
    for (uint32_t i = 0; i < lurAt<uint32_t>(self, LUR_SCRIPT_COUNT); i++) {
        uint8_t* script = lurScript(self, i);
        if (lurStricmp(reinterpret_cast<const char*>(entry + 0x14), reinterpret_cast<const char*>(script + 4)) == 0) {
            return script;
        }
    }
    return nullptr;
}

// 0x5DBE60: a blip out of every script's list
void __fastcall LurStore_RemoveBlip(uint8_t* self, re::Edx, int handle) {
    for (uint32_t i = 0; i < lurAt<uint32_t>(self, LUR_SCRIPT_COUNT); i++) {
        uint8_t* script = lurScript(self, i);
        if (script != nullptr) {
            lurScript_RemoveBlip(script, handle);
        }
    }
}

// 0x5DBEB0: an act file (once per name hash) with its tree. (The byte goes in a stack
// dword whose other three bytes the game leaves unset: zero here.)
void __fastcall LurStore_AddActFile(uint8_t* self, re::Edx, const char* path, int32_t tree, int flag) {
    const char* slash = lurStrRChr(path, '/');
    const char* name = path;
    if (slash != nullptr || (slash = lurStrRChr(path, '\\')) != nullptr) {
        name = slash + 1;
    }
    uint32_t hash = lurHashName(name);
    for (uint32_t i = 0; i < lurAt<uint32_t>(self, LUR_ACT_COUNT); i++) {
        if (lurAt<uint32_t>(self, LUR_ACT + i * LUR_ACT_SIZE) == hash) {
            return;
        }
    }
    uint32_t entry[0x23];
    entry[0] = hash;
    char* d = reinterpret_cast<char*>(&entry[1]);
    char c;
    do {
        c = *path++;
        *d++ = c;
    } while (c != '\0');
    entry[0x21] = uint32_t(tree);
    entry[0x22] = uint8_t(flag);
    uint32_t* dst = reinterpret_cast<uint32_t*>(self + LUR_ACT + lurAt<int32_t>(self, LUR_ACT_COUNT) * LUR_ACT_SIZE);
    for (int i = 0; i < 0x23; i++) {
        dst[i] = entry[i];
    }
    lurAt<int32_t>(self, LUR_ACT_COUNT)++;
}

// 0x5DBF80: the act file of that tree removed
void __fastcall LurStore_RemoveActFile(uint8_t* self, re::Edx, int32_t tree) {
    for (uint32_t i = 0; i < lurAt<uint32_t>(self, LUR_ACT_COUNT); i++) {
        if (lurAt<int32_t>(self, 0x2E0 + i * LUR_ACT_SIZE) == tree) {
            lurActFiles_RemoveAt(self + LUR_ACT, i);
            return;
        }
    }
}

// 0x5DBFC0: whether an act file of that name is loaded (as an act file, or a tree in use)
uint8_t __fastcall LurStore_IsActFileLoaded(uint8_t* self, re::Edx, const char* path) {
    const char* slash = lurStrRChr(path, '/');
    if (slash != nullptr || (slash = lurStrRChr(path, '\\')) != nullptr) {
        path = slash + 1;
    }
    uint32_t hash = lurHashName(path);
    for (uint32_t i = 0; i < lurAt<uint32_t>(self, LUR_ACT_COUNT); i++) {
        if (lurAt<uint32_t>(self, LUR_ACT + i * LUR_ACT_SIZE) == hash) {
            return 1;  // (its flag byte is tested, both ways true)
        }
    }
    uint8_t result = 0;
    if (lurAt<uint32_t>(self, 0x258) != 0) {
        for (uint32_t i = 0;; i++) {
            if (i >= lurAt<uint32_t>(self, 0x258)) {
                return 0;
            }
            if (hash == lurAt<uint32_t>(self, 8 + i * 12)) {
                break;
            }
        }
        result = 1;
    }
    return result;
}

// 0x5DC070: the store shut down and the script image's directory deleted
void __fastcall LurStore_Destruct(uint8_t* self, re::Edx) {
    lurStore_Shutdown(self);
    uint8_t* dir = lurScriptImgDir;
    if (dir == nullptr) {
        return;
    }
    if (lurAt<int32_t>(dir, -4) != 0) {
        lurVCall<void*>(dir, 0, 3);
        lurScriptImgDir = nullptr;
        return;
    }
    lurFree(dir - 4);
    lurScriptImgDir = nullptr;
}

// 0x5DC0B0: one frame of a script on its own (removed when it finishes)
void __fastcall LurStore_UpdateScript(uint8_t* self, re::Edx, uint8_t* script) {
    lurStore_SetCurrent(self, script);
    lurScript_Update(script, 0);
    if (lurAt<int32_t>(script, 0) == 4) {
        lurStore_RemoveCurrent(self, 0);
    }
    lurAt<int32_t>(self, LUR_CURRENT) = -1;
}

// 0x5DC0F0: a script launched (unless one of that name runs: that one); its main
// thread run too when asked
uint8_t* __fastcall LurStore_LaunchScript(uint8_t* self, re::Edx, const char* name, int mainThread) {
    uint8_t* script = lurStore_FindScriptByName(self, name);
    if (script != nullptr) {
        return script;
    }
    void* memory = lurScript_New(0x2B0C);
    script = memory != nullptr ? lurScript_Init(memory, name) : nullptr;
    lurScript(self, lurAt<uint32_t>(self, LUR_SCRIPT_COUNT)) = script;
    lurAt<int32_t>(self, LUR_SCRIPT_COUNT)++;
    int32_t previous = lurAt<int32_t>(self, LUR_CURRENT);
    lurAt<int32_t>(self, LUR_CURRENT) = lurAt<int32_t>(self, LUR_SCRIPT_COUNT) - 1;
    lurScript_Start(script, lurState(self));
    if (re::Flag(mainThread)) {
        lurScript_CreateThread(script, "main");
    }
    lurAt<int32_t>(self, LUR_CURRENT) = previous;
    return script;
}

// 0x5DC180: the same, marked (+0x2B09)
void __fastcall LurStore_LaunchMarkedScript(uint8_t* self, re::Edx, const char* name, int mainThread) {
    uint8_t* script = lurStore_LaunchScript(self, name, mainThread);
    if (script != nullptr) {
        lurAt<uint8_t>(script, 0x2B09) = 1;
    }
}

// 0x5DC1A0: an action tree the current script now uses (by the entity's model's act
// name, else by `treeName` or the node given): nothing when the model's own act list
// has the name, the tree is an act file's, or an animation file of the name exists
void __fastcall LurStore_UseActionTree(uint8_t* self, re::Edx, uint8_t* entity, void* treeName, const char* name, void* node) {
    if (entity != nullptr) {
        const char* acts = lurAt<const char*>(lurModelInfoPtrs[lurAt<int16_t>(entity, 0x10E)], 0x68);
        if (acts != nullptr && lurStrStr(acts, name) != nullptr) {
            return;
        }
    }
    if (treeName != nullptr) {
        node = lurActionTree_Find(treeName, 0);
    }
    if (node == nullptr) {
        return;
    }
    bool known = false;
    const char* slash = lurStrRChr(name, '/');
    const char* file = name;
    if (slash != nullptr || (slash = lurStrRChr(name, '\\')) != nullptr) {
        file = slash + 1;
    }
    uint32_t hash = lurHashName(file);
    uint32_t count = lurAt<uint32_t>(self, LUR_ACT_COUNT);
    for (uint32_t i = 0; i < count; i++) {
        if (lurAt<uint32_t>(self, LUR_ACT + i * LUR_ACT_SIZE) == hash) {
            known = true;
            break;
        }
    }
    void* tree = node;
    if (node != lurTreeRoot) {
        void* parent;
        do {
            tree = node;
            parent = lurVCall<void*>(node, 0x34, -1);
            node = parent;
        } while (parent != lurTreeRoot);
    }
    for (uint32_t i = 0; i < count; i++) {
        if (lurAt<void*>(self, 0x2E0 + i * LUR_ACT_SIZE) == tree) {
            known = true;
            break;
        }
    }
    if (lurAnimFile_Find(name, 0) != -1 || known) {
        return;
    }
    uint32_t current = lurAt<uint32_t>(self, LUR_CURRENT);
    if (current == 0xFFFFFFFF || current >= lurAt<uint32_t>(self, LUR_SCRIPT_COUNT) || lurAt<uint32_t>(self, LUR_SCRIPT_COUNT) == 0) {
        return;
    }
    uint8_t* script = lurScript(self, current);
    if (script != nullptr) {
        lurScript_UseActionTree(script, tree, name);
    }
}

// 0x5DC2F0: the store (re)started: the script image's directory read once, the Lua
// state made, main.lua launched and its gamemain thread started
void __fastcall LurStore_Init(uint8_t* self, re::Edx) {
    if (lurState(self) != nullptr) {
        lurStore_Shutdown(self);
    }
    if (lurScriptImgDir == nullptr) {
        uint8_t* dir = static_cast<uint8_t*>(lurMalloc(0x10));
        if (dir != nullptr) {
            lurDirectory_Construct(dir, 0x300);
            lurAt<uint32_t>(dir, 0) = 0x009186D0;  // CDirectory's vtable
        } else {
            dir = nullptr;
        }
        lurScriptImgDir = dir;
        lurAt<int32_t>(dir, 0xC) = 0;
        void* file = lurFileMgr_Open("Scripts\\Scripts.DIR", "rb", 0);
        if (file != nullptr) {
            lurDirectory_ReadDir(lurScriptImgDir, "Scripts\\Scripts.DIR");
            lurFileMgr_Close(file);
        }
    }
    lurStore_CreateLuaState(self);
    uint8_t* script = lurStore_LaunchScript(self, "main.lua", 0);
    lurScript_CreateThread(script, "gamemain");
}

} // namespace

#define LUR_REVERSED(name, address, fn)                   \
    static void* RE_CONCAT(lurOriginal_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(lurOriginal_, __LINE__), re::Mode::Original)

LUR_REVERSED("CLurStore::CollectGarbage", 0x005DAF40, &LurStore_CollectGarbage);
LUR_REVERSED("CLurStore::Add", 0x005DAF70, &LurStore_Add);
LUR_REVERSED("CLurStore::Find", 0x005DAFF0, &LurStore_Find);
LUR_REVERSED("CLurStore::RequestAll", 0x005DB050, &LurStore_RequestAll);
LUR_REVERSED("CLurStore::GetName", 0x005DB080, &LurStore_GetName);
LUR_REVERSED("CLurStore::GetData", 0x005DB0A0, &LurStore_GetData);
LUR_REVERSED("CLurStore::GetSize", 0x005DB0C0, &LurStore_GetSize);
LUR_REVERSED("CLurStore::Load", 0x005DB0D0, &LurStore_Load);
LUR_REVERSED("CLurStore::Remove", 0x005DB110, &LurStore_Remove);
LUR_REVERSED("CLurStore::RegisterFunctions", 0x005DB140, &LurStore_RegisterFunctions);
LUR_REVERSED("CLurStore::RegisterEnum", 0x005DB1A0, &LurStore_RegisterEnum);
LUR_REVERSED("Lur::TriggerIndex", 0x005DB250, &Lur_TriggerIndex);
LUR_REVERSED("Lur::PointListIndex", 0x005DB290, &Lur_PointListIndex);
LUR_REVERSED("Lur::PathIndex", 0x005DB380, &Lur_PathIndex);
LUR_REVERSED("CLurStore::CreateLuaState", 0x005DB3C0, &LurStore_CreateLuaState);
LUR_REVERSED("ActFile::HashName", 0x005DB830, &ActFile_HashName);
LUR_REVERSED("CLurStore::ActFiles_RemoveAt", 0x005DB870, &ActFiles_RemoveAt);
LUR_REVERSED("CLurStore::GetScript", 0x005DB8D0, &LurStore_GetScript);
LUR_REVERSED("CLurStore::FindScript", 0x005DB8F0, &LurStore_FindScript);
LUR_REVERSED("CLurStore::GetCurrentScript", 0x005DB940, &LurStore_GetCurrentScript);
LUR_REVERSED("CLurStore::GetCurrentScriptChecked", 0x005DB960, &LurStore_GetCurrentScriptChecked);
LUR_REVERSED("CLurStore::Shutdown", 0x005DB990, &LurStore_Shutdown);
LUR_REVERSED("CLurStore::RemovePed", 0x005DBA60, &LurStore_RemovePed);
LUR_REVERSED("CLurStore::RemoveCurrent", 0x005DBAA0, &LurStore_RemoveCurrent);
LUR_REVERSED("CLurStore::Update", 0x005DBB40, &LurStore_Update);
LUR_REVERSED("CLurStore::Any2B09", 0x005DBC40, &LurStore_Any2B09);
LUR_REVERSED("CLurStore::Set2B08", 0x005DBC80, &LurStore_Set2B08);
LUR_REVERSED("CLurStore::FindScriptByName", 0x005DBCC0, &LurStore_FindScriptByName);
LUR_REVERSED("CLurStore::GetScriptOfState", 0x005DBD50, &LurStore_GetScriptOfState);
LUR_REVERSED("CLurStore::SetCurrent", 0x005DBDA0, &LurStore_SetCurrent);
LUR_REVERSED("CLurStore::GetStreamedScript", 0x005DBDF0, &LurStore_GetStreamedScript);
LUR_REVERSED("CLurStore::RemoveBlip", 0x005DBE60, &LurStore_RemoveBlip);
LUR_REVERSED("CLurStore::AddActFile", 0x005DBEB0, &LurStore_AddActFile);
LUR_REVERSED("CLurStore::RemoveActFile", 0x005DBF80, &LurStore_RemoveActFile);
LUR_REVERSED("CLurStore::IsActFileLoaded", 0x005DBFC0, &LurStore_IsActFileLoaded);
LUR_REVERSED("CLurStore::Destruct", 0x005DC070, &LurStore_Destruct);
LUR_REVERSED("CLurStore::UpdateScript", 0x005DC0B0, &LurStore_UpdateScript);
LUR_REVERSED("CLurStore::LaunchScript", 0x005DC0F0, &LurStore_LaunchScript);
LUR_REVERSED("CLurStore::LaunchMarkedScript", 0x005DC180, &LurStore_LaunchMarkedScript);
LUR_REVERSED("CLurStore::UseActionTree", 0x005DC1A0, &LurStore_UseActionTree);
LUR_REVERSED("CLurStore::Init", 0x005DC2F0, &LurStore_Init);
