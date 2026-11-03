#pragma once

// Lua 5.0.2, built from its source (third_party/lua-5.0.2) as the game's copy was: lua_Number
// is float. LuaLibrary.cpp puts it in place of the game's copy.

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}
