#pragma once

// The script side: Lua 5.0.2 as the game built it (lua_Number is float: numbers are 8-byte
// TValues, a type int and a float; Lua502.h), and the helpers the script natives read their
// arguments and push their results with.
//
// A native is `int __cdecl Native_X(lua_State* L)`: it reads its arguments, pushes
// its results and returns how many it pushed. The game's helpers (namespace Script)
// take 0-based argument indices (the Lua index minus 1); the lua_* ones take Lua's.

#include <cstdint>

#include "Lua502.h"
#include "reversed.h"

// The vectors GetVector / PushVector take (a CVector; its own name here: the math
// types aren't shared yet)
struct ScriptVector {
    float x, y, z;
};
static_assert(sizeof(ScriptVector) == 12);

namespace lua {
// lapi.c, in its order (the source's, under the declarations the natives were written against)
LIB_FN(int, __cdecl, gettop, lua_gettop, lua_State* L);
LIB_FN(void, __cdecl, settop, lua_settop, lua_State* L, int idx);
LIB_FN(void, __cdecl, remove, lua_remove, lua_State* L, int idx);
LIB_FN(void, __cdecl, insert, lua_insert, lua_State* L, int idx);
LIB_FN(void, __cdecl, replace, lua_replace, lua_State* L, int idx);
LIB_FN(void, __cdecl, pushvalue, lua_pushvalue, lua_State* L, int idx);
LIB_FN(int, __cdecl, type, lua_type, lua_State* L, int idx);           // LUA_TNONE for no such index
LIB_FN(const char*, __cdecl, type_name, lua_typename, lua_State* L, int t);   // lua_typename
LIB_FN(int, __cdecl, isnumber, lua_isnumber, lua_State* L, int idx);       // a number or a string that converts
LIB_FN(int, __cdecl, isstring, lua_isstring, lua_State* L, int idx);
LIB_FN(float, __cdecl, tonumber, lua_tonumber, lua_State* L, int idx);     // 0 when not a number
LIB_FN(int, __cdecl, toboolean, lua_toboolean, lua_State* L, int idx);      // 0 for nil, false and none
LIB_FN(const char*, __cdecl, tostring, lua_tostring, lua_State* L, int idx); // nullptr when not a string or number
LIB_FN(size_t, __cdecl, strlen, lua_strlen, lua_State* L, int idx);
LIB_FN(void*, __cdecl, touserdata, lua_touserdata, lua_State* L, int idx);   // light or full userdata
LIB_FN(lua_State*, __cdecl, tothread, lua_tothread, lua_State* L, int idx);
LIB_FN(const void*, __cdecl, topointer, lua_topointer, lua_State* L, int idx);
LIB_FN(void, __cdecl, pushnil, lua_pushnil, lua_State* L);
LIB_FN(void, __cdecl, pushnumber, lua_pushnumber, lua_State* L, float n);
LIB_FN(void, __cdecl, pushlstring, lua_pushlstring, lua_State* L, const char* s, size_t len);
LIB_FN(void, __cdecl, pushstring, lua_pushstring, lua_State* L, const char* s);   // nil for nullptr
LIB_FN(const char*, __cdecl, pushvfstring, lua_pushvfstring, lua_State* L, const char* fmt, void* argp);
LIB_FN(void, __cdecl, pushboolean, lua_pushboolean, lua_State* L, int b);
LIB_FN(void, __cdecl, pushlightuserdata, lua_pushlightuserdata, lua_State* L, void* p);
LIB_FN(void, __cdecl, newtable, lua_newtable, lua_State* L);
LIB_FN(void, __cdecl, rawgeti, lua_rawgeti, lua_State* L, int idx, int n);
LIB_FN(void, __cdecl, rawseti, lua_rawseti, lua_State* L, int idx, int n);
} // namespace lua

// The game's helpers over it (0x5D82D0..): argument `i` is Lua's i + 1
namespace Script {
// 0x5D82D0: the number of arguments (lua_gettop)
GAME_FN(int, __cdecl, GetArgCount, 0x005D82D0, lua_State* L);
// 0x5D82E0: argument i is nil (not none: that is -1)
GAME_FN(uint8_t, __cdecl, IsNil, 0x005D82E0, lua_State* L, int i);
// 0x5D8300: argument i is a boolean
GAME_FN(uint8_t, __cdecl, IsBoolean, 0x005D8300, lua_State* L, int i);
// 0x5D8320: lua_isnumber
GAME_FN(uint8_t, __cdecl, IsNumber, 0x005D8320, lua_State* L, int i);
// 0x5D8340: argument i is a string (type 4 only)
GAME_FN(uint8_t, __cdecl, IsString, 0x005D8340, lua_State* L, int i);
// 0x5D8360: the number rounded half away from zero (in double, then truncated)
GAME_FN(int, __cdecl, GetInt, 0x005D8360, lua_State* L, int i);
// 0x5D83A0: GetInt, or `def` when there are not that many arguments
GAME_FN(int, __cdecl, GetIntOpt, 0x005D83A0, lua_State* L, int i, int def);
// 0x5D8400: lua_tonumber
GAME_FN(float, __cdecl, GetFloat, 0x005D8400, lua_State* L, int i);
// 0x5D8410: GetFloat, or `def` when there are not that many arguments
GAME_FN(float, __cdecl, GetFloatOpt, 0x005D8410, lua_State* L, int i, float def);
// 0x5D8440: lua_tostring
GAME_FN(const char*, __cdecl, GetString, 0x005D8440, lua_State* L, int i);
// 0x5D8450: GetString, or `def` when there are not that many arguments
GAME_FN(const char*, __cdecl, GetStringOpt, 0x005D8450, lua_State* L, int i, const char* def);
// 0x5D8480: arguments i..i+2 as a vector (returns `out`)
GAME_FN(ScriptVector*, __cdecl, GetVector, 0x005D8480, ScriptVector* out, lua_State* L, int i);
// 0x5D84D0: the same, each missing one from `def`
GAME_FN(ScriptVector*, __cdecl, GetVectorOpt, 0x005D84D0, ScriptVector* out, lua_State* L, int i, const ScriptVector* def);
// 0x5D85A0: lua_pushlightuserdata
GAME_FN(void, __cdecl, PushUserdata, 0x005D85A0, lua_State* L, void* p);
// 0x5D85B0: pushes a boolean (the byte's non-zero)
GAME_FN(void, __cdecl, PushBool, 0x005D85B0, lua_State* L, uint8_t b);
// 0x5D85C0: pushes an int (as a float)
GAME_FN(void, __cdecl, PushInt, 0x005D85C0, lua_State* L, int n);
// 0x5D85E0: pushes a float
GAME_FN(void, __cdecl, PushFloat, 0x005D85E0, lua_State* L, float n);
// 0x5D8600: lua_pushstring
GAME_FN(void, __cdecl, PushString, 0x005D8600, lua_State* L, const char* s);
// 0x5D8610: pushes x, y, z (three results)
GAME_FN(void, __cdecl, PushVector, 0x005D8610, lua_State* L, const ScriptVector* v);
// 0x5D8650: argument i is a function
GAME_FN(uint8_t, __cdecl, IsFunction, 0x005D8650, lua_State* L, int i);
// 0x5D8670: pushes registry[ref] (lua_getref)
GAME_FN(void, __cdecl, PushRef, 0x005D8670, lua_State* L, int ref);
// 0x5D8690: lua_pushnil
GAME_FN(void, __cdecl, PushNil, 0x005D8690, lua_State* L);
// 0x5D86A0: lua_newtable
GAME_FN(void, __cdecl, NewTable, 0x005D86A0, lua_State* L);
// 0x5D86B0: lua_rawseti
GAME_FN(void, __cdecl, RawSetI, 0x005D86B0, lua_State* L, int idx, int n);
// 0x5D86C0: argument i is a light userdata
GAME_FN(uint8_t, __cdecl, IsUserdata, 0x005D86C0, lua_State* L, int i);
// 0x5D86E0: lua_touserdata
GAME_FN(void*, __cdecl, GetUserdata, 0x005D86E0, lua_State* L, int i);
// 0x5D9080: a number (rounded as GetInt) != 0, else lua_toboolean
GAME_FN(uint8_t, __cdecl, GetBool, 0x005D9080, lua_State* L, int i);
// 0x5D9100: GetBool if argument i is a boolean or a number, else `def`
GAME_FN(uint8_t, __cdecl, GetBoolOpt, 0x005D9100, lua_State* L, int i, int def);
// 0x5D9FD0: argument i (a function) into the registry, its reference in *ref (an
// older one there released first)
GAME_FN(void, __cdecl, StoreFunctionRef, 0x005D9FD0, lua_State* L, int i, void* ref);
// 0x5DA060: StoreFunctionRef, when there are that many arguments
GAME_FN(void, __cdecl, StoreFunctionRefOpt, 0x005DA060, lua_State* L, int i, void* ref);
// 0x5DA110: argument i as a vehicle handle (GetInt): the vehicle, nullptr for a stale one
GAME_FN(void*, __cdecl, GetVehicle, 0x005DA110, lua_State* L, int i);
} // namespace Script

// Used by many natives (not Script helpers)
namespace ScriptGame {
// 0x5C7380: the ped of a handle (-1: nullptr; kind 3: the player; else the ped pool
// slot handle & 0xFF, when its virtual +0xAC gives the handle back)
GAME_FN(void*, __cdecl, GetPedFromHandle, 0x005C7380, int handle, int kind);
// 0x5DB940 (thiscall on TheLurStore, 0xD02850): the running script, nullptr outside one
GAME_FN(void*, __thiscall, GetCurrentScript, 0x005DB940, void* lurStore);
} // namespace ScriptGame
