/*
** Bully's configuration of Lua 5.0.2 (LUA_USER_H, included by lua.h): what the game's own
** copy was built with, read from its code. BULLY.md lists it with the source changes.
*/
#ifndef bully_lua_h
#define bully_lua_h

#include <stddef.h>

/* Numbers are floats (8-byte TObjects), printed with "%.10g" (luaV_tostring at 0x744510,
** lua_pushfstring's %f). The scan format is luser_number.h's for floats. */
#define LUA_NUMBER		float
#define LUA_NUMBER_SCAN		"%f"
#define LUA_NUMBER_FMT		"%.10g"

/* Four bytes before every lua_State: lua_open (0x742010) and lua_newthread (0x741F30)
** allocate 0x5C bytes and use the block + 4 (sizeof(lua_State) is 0x58). Lua's default
** alignment union would make it 8. */
#define LUSER_ALIGNMENT_T	long
#define LUA_USERSTATE		void *

/* The memory comes from the game's Lua allocator (0x5D8010 realloc, 0x5D8070 free, what
** luaM_realloc 0x7416F0 calls). */
#ifdef __cplusplus
extern "C" {
#endif
void *bully_lua_realloc (void *block, size_t oldsize, size_t size);
void bully_lua_free (void *block, size_t oldsize);
#ifdef __cplusplus
}
#endif
#define l_realloc(b,os,s)	bully_lua_realloc((b), (os), (s))
#define l_free(b,os)		bully_lua_free((b), (os))

#endif
