// Lua 5.0.2 from its source (third_party/lua-5.0.2, configured as the game's copy was: see its
// BULLY.md) in place of the game's copy at 0x73AD00..0x746470. Every function the game's code
// calls in its Lua is registered here with the source's, so in ours mode the game's copy
// never runs. tools/lua_map.txt pairs the rest of the game's copy with the source.
//
// Any mix of modes works: both copies are the same source with the same configuration (the
// same lua_State, TObject and table layouts) and both allocate through the game's Lua
// allocator (0x5D8010 / 0x5D8070, LuaMemory in ScriptLuaGlue.cpp), so a state one copy made
// can be used by the other.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "game/Lua502.h"
#include "reversed.h"

// lapi.c's own (lapi.h)
extern "C" void luaA_pushobject(lua_State* L, const void* o);

namespace {

GAME_FN(void*, __cdecl, LuaMemory_Realloc, 0x005D8010, void* block, uint32_t oldSize, uint32_t newSize);
GAME_FN(void, __cdecl, LuaMemory_Free, 0x005D8070, void* block);

} // namespace

// bully_lua.h's hooks
extern "C" void* bully_lua_realloc(void* block, size_t oldsize, size_t size) {
    return LuaMemory_Realloc(block, uint32_t(oldsize), uint32_t(size));
}
extern "C" void bully_lua_free(void* block, size_t) {
    LuaMemory_Free(block);
}
// (the game's rand sequence, Crt.h: math.random shares it)
extern "C" int bully_lua_rand() {
    return crt::Rand();
}
extern "C" void bully_lua_srand(unsigned int seed) {
    crt::Srand(seed);
}

#define LUA_REVERSED(fn, address) \
    REVERSED("Lua::" #fn, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

// lapi.c (0x73AD00 negindex takes its arguments in registers: only lapi.c calls it)
RE_UNHOOKED("Lua::negindex", 0x0073AD00);
LUA_REVERSED(luaA_pushobject, 0x0073AD50);
LUA_REVERSED(lua_checkstack, 0x0073AD90);
LUA_REVERSED(lua_xmove, 0x0073ADF0);
LUA_REVERSED(lua_atpanic, 0x0073AE40);
LUA_REVERSED(lua_newthread, 0x0073AE60);
LUA_REVERSED(lua_gettop, 0x0073AEA0);
LUA_REVERSED(lua_settop, 0x0073AEB0);
LUA_REVERSED(lua_remove, 0x0073AF00);
LUA_REVERSED(lua_insert, 0x0073AF40);
LUA_REVERSED(lua_replace, 0x0073AF90);
LUA_REVERSED(lua_pushvalue, 0x0073AFE0);
LUA_REVERSED(lua_type, 0x0073B030);
LUA_REVERSED(lua_typename, 0x0073B060);
LUA_REVERSED(lua_iscfunction, 0x0073B080);
LUA_REVERSED(lua_isnumber, 0x0073B0C0);
LUA_REVERSED(lua_isstring, 0x0073B110);
LUA_REVERSED(lua_rawequal, 0x0073B150);
LUA_REVERSED(lua_lessthan, 0x0073B1B0);
LUA_REVERSED(lua_tonumber, 0x0073B220);
LUA_REVERSED(lua_toboolean, 0x0073B270);
LUA_REVERSED(lua_tostring, 0x0073B2B0);
LUA_REVERSED(lua_strlen, 0x0073B320);
LUA_REVERSED(lua_touserdata, 0x0073B380);
LUA_REVERSED(lua_tothread, 0x0073B3D0);
LUA_REVERSED(lua_topointer, 0x0073B410);
LUA_REVERSED(lua_pushnil, 0x0073B480);
LUA_REVERSED(lua_pushnumber, 0x0073B4A0);
LUA_REVERSED(lua_pushlstring, 0x0073B4C0);
LUA_REVERSED(lua_pushstring, 0x0073B500);
LUA_REVERSED(lua_pushvfstring, 0x0073B570);
LUA_REVERSED(lua_pushfstring, 0x0073B5A0);
LUA_REVERSED(lua_pushcclosure, 0x0073B5D0);
LUA_REVERSED(lua_pushboolean, 0x0073B650);
LUA_REVERSED(lua_pushlightuserdata, 0x0073B670);
LUA_REVERSED(lua_gettable, 0x0073B690);
LUA_REVERSED(lua_rawget, 0x0073B6E0);
LUA_REVERSED(lua_rawgeti, 0x0073B730);
LUA_REVERSED(lua_newtable, 0x0073B780);
LUA_REVERSED(lua_getmetatable, 0x0073B7C0);
LUA_REVERSED(lua_getfenv, 0x0073B830);
LUA_REVERSED(lua_settable, 0x0073B890);
LUA_REVERSED(lua_rawset, 0x0073B8D0);
LUA_REVERSED(lua_rawseti, 0x0073B920);
LUA_REVERSED(lua_setmetatable, 0x0073B970);
LUA_REVERSED(lua_setfenv, 0x0073B9D0);
LUA_REVERSED(lua_call, 0x0073BA20);
LUA_REVERSED(lua_pcall, 0x0073BA70);
LUA_REVERSED(lua_load, 0x0073BAE0);
LUA_REVERSED(lua_getgcthreshold, 0x0073BB30);
LUA_REVERSED(lua_getgccount, 0x0073BB40);
LUA_REVERSED(lua_setgcthreshold, 0x0073BB50);
LUA_REVERSED(lua_error, 0x0073BB90);
LUA_REVERSED(lua_next, 0x0073BBA0);
LUA_REVERSED(lua_concat, 0x0073BBF0);
LUA_REVERSED(lua_newuserdata, 0x0073BC70);
LUA_REVERSED(lua_getupvalue, 0x0073BCB0);
LUA_REVERSED(lua_setupvalue, 0x0073BD50);

// lauxlib.c
LUA_REVERSED(luaL_where, 0x0073BDF0);
LUA_REVERSED(luaL_error, 0x0073BE60);
LUA_REVERSED(luaL_checkstack, 0x0073BE90);
LUA_REVERSED(luaL_getmetafield, 0x0073BEC0);
LUA_REVERSED(luaL_callmeta, 0x0073BF20);
LUA_REVERSED(luaL_openlib, 0x0073BF80);
LUA_REVERSED(luaL_setn, 0x0073C0F0);
LUA_REVERSED(luaL_getn, 0x0073C1C0);
LUA_REVERSED(luaL_addlstring, 0x0073C360);
LUA_REVERSED(luaL_pushresult, 0x0073C3C0);
LUA_REVERSED(luaL_addvalue, 0x0073C400);
LUA_REVERSED(luaL_buffinit, 0x0073C490);
LUA_REVERSED(luaL_ref, 0x0073C4B0);
LUA_REVERSED(luaL_unref, 0x0073C580);
LUA_REVERSED(luaL_loadfile, 0x0073C660);
LUA_REVERSED(luaL_loadbuffer, 0x0073C7F0);
LUA_REVERSED(lua_dostring, 0x0073C8F0);
LUA_REVERSED(luaL_argerror, 0x0073C920);
LUA_REVERSED(luaL_checkany, 0x0073CA10);
LUA_REVERSED(luaL_checklstring, 0x0073CA40);
LUA_REVERSED(luaL_optlstring, 0x0073CAB0);
LUA_REVERSED(luaL_checknumber, 0x0073CB10);
LUA_REVERSED(luaL_optnumber, 0x0073CB90);

// the libraries the game opens (lbaselib.c, ldblib.c, lmathlib.c, lstrlib.c, ltablib.c)
LUA_REVERSED(luaopen_base, 0x0073DD20);
LUA_REVERSED(luaopen_debug, 0x0073E6E0);
LUA_REVERSED(luaopen_math, 0x00741630);
LUA_REVERSED(luaopen_string, 0x00742510);
LUA_REVERSED(luaopen_table, 0x00743950);

// ldebug.c, ldo.c, lstate.c
LUA_REVERSED(lua_getstack, 0x0073E820);
LUA_REVERSED(lua_getlocal, 0x0073E8A0);
LUA_REVERSED(lua_setlocal, 0x0073E920);
LUA_REVERSED(lua_getinfo, 0x0073F370);
LUA_REVERSED(lua_yield, 0x0073FEF0);
LUA_REVERSED(lua_resume, 0x00740110);
LUA_REVERSED(lua_open, 0x00742010);
LUA_REVERSED(lua_close, 0x007420B0);
