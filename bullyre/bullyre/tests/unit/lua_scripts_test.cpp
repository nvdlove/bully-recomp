// Lua from its source (third_party/lua-5.0.2, the game's configuration) against the game's own
// scripts: every .lur in Scripts/Scripts.img is loaded (lundump checks the header: float
// numbers, instruction layout) and its main chunk run in a fresh state with the libraries the
// game opens, every global it doesn't have answering as a stub (the natives and the game's
// tables: see PushStub). A main chunk can still fail on what a stub gives back (a native's
// result used as a number: 5 of the 515): those are listed, but only a script that doesn't
// load fails the test.
//
// bullyre_luatest.exe [<game folder> [-v]]  prints a line per failure and a summary; exit code 1
//                                            when a script didn't load (-v: each name first)

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

// bully_lua.h's hooks (in the game: its allocator and C runtime)
extern "C" void* bully_lua_realloc(void* block, size_t, size_t size) {
    return std::realloc(block, size);
}
extern "C" void bully_lua_free(void* block, size_t) {
    std::free(block);
}
extern "C" int bully_lua_rand() {
    return std::rand();
}
extern "C" void bully_lua_srand(unsigned int seed) {
    std::srand(seed);
}

namespace {

int Nothing(lua_State*) {
    return 0;
}

// An unknown global (a native, or one of the game's tables): a table that can be called (doing
// nothing) and indexed (giving another of these)
int PushStub(lua_State* L) {
    lua_newtable(L);
    lua_newtable(L);
    lua_pushstring(L, "__call");
    lua_pushcclosure(L, &Nothing, 0);
    lua_settable(L, -3);
    lua_pushstring(L, "__index");
    lua_pushcclosure(L, &PushStub, 0);
    lua_settable(L, -3);
    lua_setmetatable(L, -2);
    return 1;
}

struct Entry {
    std::string name;
    std::vector<char> data;
};

std::vector<Entry> ReadScripts(const std::string& folder) {
    std::vector<Entry> out;
    FILE* dir = std::fopen((folder + "\\Scripts\\Scripts.dir").c_str(), "rb");
    FILE* img = std::fopen((folder + "\\Scripts\\Scripts.img").c_str(), "rb");
    if (dir == nullptr || img == nullptr) {
        std::printf("can't open %s\\Scripts\\Scripts.dir/.img\n", folder.c_str());
        std::exit(2);
    }
    struct {
        uint32_t offset, size;  // in 2048-byte sectors
        char name[24];
    } e;
    while (std::fread(&e, sizeof(e), 1, dir) == 1) {
        Entry entry;
        entry.name.assign(e.name, strnlen(e.name, sizeof(e.name)));
        entry.data.resize(size_t(e.size) * 2048);
        std::fseek(img, long(e.offset) * 2048, SEEK_SET);
        entry.data.resize(std::fread(entry.data.data(), 1, entry.data.size(), img));
        out.push_back(std::move(entry));
    }
    std::fclose(dir);
    std::fclose(img);
    return out;
}

} // namespace

int main(int argc, char** argv) {
    std::string folder = argc > 1 ? argv[1] : R"(C:\Program Files (x86)\Steam\steamapps\common\Bully Scholarship Edition)";
    bool verbose = argc > 2;
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::vector<Entry> scripts = ReadScripts(folder);
    int loaded = 0, ran = 0;
    for (const Entry& s : scripts) {
        if (verbose) {
            std::printf("%s\n", s.name.c_str());
        }
        lua_State* L = lua_open();
        luaopen_base(L);
        luaopen_table(L);
        luaopen_string(L);
        luaopen_math(L);
        luaopen_debug(L);
        lua_settop(L, 0);
        lua_newtable(L);  // the globals' metatable
        lua_pushstring(L, "__index");
        lua_pushcclosure(L, &PushStub, 0);
        lua_settable(L, -3);
        lua_setmetatable(L, LUA_GLOBALSINDEX);

        int status = luaL_loadbuffer(L, s.data.data(), s.data.size(), s.name.c_str());
        if (status != 0) {
            std::printf("%-24s load: %s\n", s.name.c_str(), lua_tostring(L, -1));
        } else {
            loaded++;
            status = lua_pcall(L, 0, 0, 0);
            if (status != 0) {
                std::printf("%-24s run: %s\n", s.name.c_str(), lua_tostring(L, -1));
            } else {
                ran++;
            }
        }
        lua_close(L);
    }
    std::printf("%zu scripts: %d loaded, %d ran their main chunk\n", scripts.size(), loaded, ran);
    return loaded == int(scripts.size()) ? 0 : 1;
}
