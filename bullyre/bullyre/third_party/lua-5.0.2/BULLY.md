# Lua 5.0.2, as Bully built it

The game's Lua (0x73AD00..0x746470 in Bully.unpacked.exe, version string "Lua 5.0.2" at
0x94F0E4) is stock Lua 5.0.2 with a configuration and a few edits, all read from its code.
This copy is that: the release from lua.org (`lua-5.0.2.tar.gz`, md5
dea74646b7e5c621fef7174df83c34b1), configured and edited the same way.
`src/game/LuaLibrary.cpp` registers it at the game's addresses.

## Configuration (`include/bully_lua.h`, LUA_USER_H)

- **Numbers are floats:**
  - LUA_NUMBER is float and LUA_NUMBER_FMT is "%.10g" (luaV_tostring 0x744510, luaO_pushvfstring).
  - The scan format is luser_number.h's for floats. Only liolib uses it, and the game doesn't have liolib.
- **Four bytes before every lua_State:**
  - lua_open (0x742010) and lua_newthread (0x741F30) allocate 0x5C bytes and use the block + 4, so EXTRASPACE is 4.
  - LUA_USERSTATE is set, and LUSER_ALIGNMENT_T is 4 bytes wide. Lua's default union, with its double, would make it 8.
- **Memory comes from the game's Lua allocator:**
  - l_realloc and l_free call 0x5D8010 and 0x5D8070, as luaM_realloc (0x7416F0) does.
  - The hooks are in LuaLibrary.cpp.
- **No parser:** the parsing modules (lcode, llex, lparser) are left out, and `src/noparser.c` from `etc/` stands in for them. The game's luaY_parser pushes "parser not loaded" (0x741CD0); scripts come precompiled.
- **Random numbers come from the game's C runtime:**
  - math.random and math.randomseed use the game's rand (0x85C6FA) and srand (0x85C6ED), as the game's copy did, so they share the game's sequence.
  - lmathlib.c is compiled with `rand=bully_lua_rand` and `srand=bully_lua_srand` (CMakeLists.txt).

## Source changes

Each is marked `Bully:` in the source.

- **lbaselib.c:** base_funcs has no `print`. The game's table is at 0x94F2C0.
- **lmathlib.c:** mathlib is the game's table (0x94FB98): no frexp, ldexp, log or log10, and in the game's order. The order sets the order the names go into the `math` table.
- **lstrlib.c:** strlib has only `find` and `format` (0x94FE00).
- **ltable.c:** computesizes's `(n == -1) ? 0 : twoto(n)` is now `twoto(n + 1) >> 1`. This is not the game's edit; it works around a compiler bug. VS 2026's /O2 drops the -1 case, and on x86 `1 << -1` is 1<<31, so the first table resize asked for 2^31 slots.

The other tables are stock: co_funcs (0x94F388), dblib (0x94F750) and tab_funcs (0x94FEB8).

Not reproduced: the game's luaM_realloc has no "block too big" check (lmem.c's `size >= MAX_SIZET`). The difference only shows for sizes near 4 GB.

## Files

- Core: lapi, ldebug, ldo, ldump, lfunc, lgc, lmem, lobject, lopcodes, lstate, lstring, ltable, ltm, lundump, lvm, lzio, and noparser.
- Libraries: lauxlib, lbaselib, ldblib, lmathlib, lstrlib and ltablib.
- liolib and loadlib aren't here: the game opens base, table, string, math and debug (LurStore.cpp).
- Headers are unchanged, apart from the new `include/bully_lua.h`.

`tools/lua_map.txt` pairs each of the game's Lua functions with this source.
