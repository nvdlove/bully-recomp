// The script VM's glue (0x5D7EC0..0x5D8C40): Lua's allocator (the "LuaCommonBlock"
// common blocks, else the heap under tag 0x28), calling a script's function through
// the Lua side's __onerror / NSCall, the argument helpers the natives use (namespace
// Script in ScriptApi.h: 0-based argument indices), and loading a script's chunk.

#include <cstdint>

#include "game/ScriptApi.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

// lapi.c / lauxlib.c beyond ScriptApi.h's
LIB_FN(void, __cdecl, slgLua_gettable, lua_gettable, lua_State* L, int idx);
LIB_FN(int, __cdecl, slgLua_pcall, lua_pcall, lua_State* L, int nargs, int nresults, int errfunc);
LIB_FN(int, __cdecl, slgLuaL_loadbuffer, luaL_loadbuffer, lua_State* L, const void* buffer, uint32_t size, const char* name);
inline constexpr int SLG_LUA_GLOBALSINDEX = -10001;

// The Lua state the game runs its scripts in
GAME_VAR(lua_State*, slgLuaState, 0x00D093B0);

// The common blocks Lua's small allocations come from (0xCFC9B8)
inline void* const slgLuaBlocks = reinterpret_cast<void*>(0x00CFC9B8);
GAME_FN(void, __thiscall, slgBlocks_Init, 0x005EEAE0, void* blocks, const char* name, int32_t* params);
GAME_FN(uint8_t, __thiscall, slgBlocks_IsInitialised, 0x005EEB50, void* blocks);
GAME_FN(void*, __thiscall, slgBlocks_Malloc, 0x005EEB60, void* blocks, uint32_t size);
GAME_FN(void, __thiscall, slgBlocks_Free, 0x005EEB80, void* blocks, void* p);
GAME_FN(uint8_t, __thiscall, slgBlocks_Owns, 0x005EEBA0, void* blocks, void* p);
GAME_FN(void, __cdecl, slgPushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, slgPopMemoryTag, 0x005EF100);
GAME_FN(void*, __cdecl, slgMalloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, slgFree, 0x005EEAB0, void* p);
inline void* __cdecl slgMemcpy(void* dst, const void* src, uint32_t size) { return crt::Memcpy(dst, src, (size_t)size); }
// CMemoryHeap::GetMemoryUsed on the general and the main heap
GAME_FN(uint32_t, __thiscall, slgHeap_GetMemoryUsed, 0x005EEDD0, void* heap, int memId);
inline void* const slgSimpleHeap = reinterpret_cast<void*>(0x00D14240);
inline void* const slgMainHeap = reinterpret_cast<void*>(0x00D14170);
// the Lua memory in use (tag 0x23 on both heaps), kept by the loaders below
GAME_VAR(uint32_t, slgLuaMemoryUsed, 0x00CFC9C8);

// TheLurStore (0xD02850): the running script
inline void* const slgLurStore = reinterpret_cast<void*>(0x00D02850);
GAME_FN(void*, __thiscall, slgLurStore_GetCurrentChecked, 0x005DB960, void* store);
GAME_FN(void, __thiscall, slgLurStore_SetCurrent, 0x005DBDA0, void* store, void* script);
GAME_FN(int, __thiscall, slgLurStore_Find, 0x005DAFF0, void* store, const char* name);
GAME_FN(const void*, __thiscall, slgLurStore_GetData, 0x005DB0A0, void* store, int lur);
GAME_FN(uint32_t, __thiscall, slgLurStore_GetSize, 0x005DB0C0, void* store, int lur);

GAME_FN(void, __thiscall, slgArrayElement_Construct, 0x00669E20, void* element);

// ped / prop side of the "uses node" checks
GAME_FN(uint8_t, __thiscall, slgNodeList_Contains, 0x005F48D0, void* list, void* node, int a, int b);
GAME_FN(void, __thiscall, slgNodeList_5F3830, 0x005F3830, void* list, uintptr_t value);
GAME_FN(void, __thiscall, slgPedActions_4840A0, 0x004840A0, void* actions);
GAME_FN(void, __thiscall, slgPed_474090, 0x00474090, void* ped);
GAME_FN(void, __thiscall, slgProp_UpdateAnim, 0x004ED940, void* prop);

// the image file the .lur scripts are in
GAME_FN(void*, __cdecl, slgFileMgr_Open, 0x0042D260, const char* name, const char* mode, int a);
GAME_FN(int, __cdecl, slgFileMgr_Seek, 0x0042D300, void* file, int offset, int origin);
GAME_FN(int, __cdecl, slgFileMgr_Read, 0x0042D2A0, void* file, void* buffer, int size);
GAME_FN(int, __cdecl, slgFileMgr_Close, 0x0042D450, void* file);
GAME_VAR(void*, slgScriptImgDir, 0x00D02848);
GAME_FN(uint8_t, __thiscall, slgImgDir_Find, 0x004290E0, void* dir, const char* name, int32_t* offset, int32_t* size);
inline uint8_t* const slgScriptBuffer = reinterpret_cast<uint8_t*>(0x00BD7800);

template <typename T>
re::Field<T>& slgAt(void* base, int offset) {
    return *reinterpret_cast<re::Field<T>*>(static_cast<uint8_t*>(base) + offset);
}

// A number rounded half away from zero, as the game does (x87 in extended precision,
// then _ftol's truncation)
inline int slgRound(float f) {
    return 0.0f <= f ? int(double(f) + 0.5) : int(double(f) - 0.5);
}

// 0x5D7EC0: the constructor of the 100 0x30-byte entries at 0xCFB6E8 (a static
// initialiser's): each constructed with +0x2C cleared, then +0x2C cleared again
void* __fastcall ScriptArray_Construct(uint8_t* self, re::Edx) {
    uint8_t* element = self;
    for (int i = 99; i >= 0; i--) {
        slgArrayElement_Construct(element);
        slgAt<int32_t>(element, 0x2C) = 0;
        element += 0x30;
    }
    uint32_t* field = reinterpret_cast<uint32_t*>(self + 0x2C);
    for (int i = 100; i != 0; i--) {
        *field = 0;
        field += 0xC;
    }
    return self;
}

// 0x5D7F10: an entity reference set (GTA's RegisterReference / CleanUpOldReference:
// the old entity's reference dropped, the new one's registered)
GAME_FN(void, __cdecl, slgCleanUpOldReference, 0x004657E0, void* entity, re::Ptr32<void>* ref);
GAME_FN(void, __cdecl, slgRegisterReference, 0x004657D0, void* entity, re::Ptr32<void>* ref);
void __fastcall RegisteredPointerAssign(re::Ptr32<void>* self, re::Edx, void* object) {
    void* old = *self;
    if (old != object) {
        if (old != nullptr) {
            slgCleanUpOldReference(old, self);
        }
        *self = object;
        if (object != nullptr) {
            slgRegisterReference(object, self);
        }
    }
}

// 0x5D7F50: the common blocks Lua allocates from
void __cdecl LuaMemory_Init() {
    int32_t params[6];
    params[2] = 4;
    params[4] = 4;
    params[0] = 0x400;
    params[1] = 0x3E000;
    params[3] = 0x60;
    params[5] = 0;
    slgBlocks_Init(slgLuaBlocks, "LuaCommonBlock", params);
}

// 0x5D7FA0 (size in eax): up to 0x60 bytes from the common blocks when they're up
// and have room, else the heap under tag 0x28
void* LuaMemory_Malloc(int size) {
    if (size <= 0x60 && slgBlocks_IsInitialised(slgLuaBlocks)) {
        void* p = slgBlocks_Malloc(slgLuaBlocks, uint32_t(size));
        if (p != nullptr) {
            return p;
        }
    }
    slgPushMemoryTag(0x28);
    void* p = slgMalloc(uint32_t(size));
    slgPopMemoryTag();
    return p;
}
RE_UNHOOKED_FN("LuaMemory::Malloc", 0x005D7FA0, &LuaMemory_Malloc);

// 0x5D7FE0 (the block in esi)
void LuaMemory_FreeEsi(void* p) {
    if (slgBlocks_Owns(slgLuaBlocks, p)) {
        slgBlocks_Free(slgLuaBlocks, p);
        return;
    }
    slgPushMemoryTag(0x28);
    slgFree(p);
    slgPopMemoryTag();
}
RE_UNHOOKED_FN("LuaMemory::Free_Esi", 0x005D7FE0, &LuaMemory_FreeEsi);

// 0x5D8010: Lua's l_realloc (grows by copying; the old block freed through
// 0x5D7FE0, whose esi here is the caller's block: the same one)
void* __cdecl LuaMemory_Realloc(void* block, uint32_t oldSize, uint32_t newSize) {
    if (oldSize == 0) {
        if (newSize != 0) {
            return LuaMemory_Malloc(int(newSize));
        }
        return nullptr;
    }
    if (newSize == 0) {
        return nullptr;
    }
    if (oldSize == newSize) {
        return block;
    }
    void* p = LuaMemory_Malloc(int(newSize));
    uint32_t copy = int(oldSize) < int(newSize) ? oldSize : newSize;
    slgMemcpy(p, block, copy);
    LuaMemory_FreeEsi(block);
    return p;
}

// 0x5D8070: Lua's l_free
void __cdecl LuaMemory_Free(void* p) {
    if (slgBlocks_Owns(slgLuaBlocks, p)) {
        slgBlocks_Free(slgLuaBlocks, p);
        return;
    }
    slgPushMemoryTag(0x28);
    slgFree(p);
    slgPopMemoryTag();
}

// 0x5D80B0..0x5D80E0: a script callback reference (a pointer to {script, ?, function,
// set}): its fields, -1 without one
int __fastcall ScriptRef_GetScript(re::Ptr32<int32_t>* self, re::Edx) {
    return *self != nullptr ? (*self)[0] : -1;
}
int __fastcall ScriptRef_Get4(re::Ptr32<int32_t>* self, re::Edx) {
    return *self != nullptr ? (*self)[1] : -1;
}
int __fastcall ScriptRef_GetFunction(re::Ptr32<int32_t>* self, re::Edx) {
    return *self != nullptr ? (*self)[2] : -1;
}
uint8_t __fastcall ScriptRef_IsSet(re::Ptr32<int32_t>* self, re::Edx) {
    return *self != nullptr && (*self)[3] != 0;
}

// 0x5D8100: the script's name (+4) from a path, backslashes made slashes (the loop
// bound is the name's length, recounted each time)
void __fastcall LuaScript_SetName(uint8_t* self, re::Edx, const char* name) {
    char* dst = reinterpret_cast<char*>(self + 4);
    char c;
    do {
        c = *name++;
        *dst++ = c;
    } while (c != '\0');
    char* s = reinterpret_cast<char*>(self + 4);
    uint32_t length = 0;
    while (s[length] != '\0') {
        length++;
    }
    if (length != 0) {
        uint32_t i = 0;
        do {
            if (s[i] == '\\') {
                s[i] = '/';
            }
            i++;
            length = 0;
            while (s[length] != '\0') {
                length++;
            }
        } while (i < length);
    }
}

// 0x5D8160: calls the script's function `function` through NSCall (in its
// namespace, +4) with the `nargs` values on top of the stack copied as arguments,
// __onerror handling errors
void __fastcall LuaScript_CallInNamespace(uint8_t* self, re::Edx, const char* function, int nargs, int nresults, int flag) {
    lua_State* L = slgLuaState;
    lua::pushlstring(L, "__onerror", 9);
    slgLua_gettable(L, SLG_LUA_GLOBALSINDEX);
    lua::pushstring(L, "NSCall");
    slgLua_gettable(L, SLG_LUA_GLOBALSINDEX);
    lua::pushstring(L, reinterpret_cast<const char*>(self + 4));
    lua::pushstring(L, function);
    lua::pushboolean(L, uint8_t(flag));
    for (uint32_t i = uint32_t(nargs); i != 0; i--) {
        lua::pushvalue(L, -5 - nargs);
    }
    slgLua_pcall(L, nargs + 3, nresults, -5 - nargs);
}

// 0x5D8200: the same as the running script, its one result as an int (truncated)
int __fastcall LuaScript_CallForInt(uint8_t* self, re::Edx, const char* function, int nargs, int flag) {
    void* previous = slgLurStore_GetCurrentChecked(slgLurStore);
    slgLurStore_SetCurrent(slgLurStore, self);
    lua_State* L = slgLuaState;
    LuaScript_CallInNamespace(self, nullptr, function, nargs, 1, flag);
    int result = int(lua::tonumber(L, -1));
    lua::settop(L, -3 - nargs);
    slgLurStore_SetCurrent(slgLurStore, previous);
    return result;
}

// 0x5D8270: pushes the __onerror handler (before a function and its arguments)
void __cdecl Lua_PushErrorHandler(lua_State* L) {
    lua::pushlstring(L, "__onerror", 9);
    slgLua_gettable(L, SLG_LUA_GLOBALSINDEX);
}

// 0x5D82A0: calls the function under its arguments with the handler below it, then
// drops the handler (and one result)
void __cdecl Lua_CallProtected(lua_State* L, int nargs, int nresults) {
    slgLua_pcall(L, nargs, nresults, -2 - nargs);
    lua::settop(L, -2);
}

// 0x5D82D0 (a jmp to lua_gettop)
RE_UNHOOKED("Script::GetArgCount", 0x005D82D0);

uint8_t __cdecl Script_IsNil(lua_State* L, int i) {
    return lua::type(L, i + 1) == LUA_TNIL;
}
uint8_t __cdecl Script_IsBoolean(lua_State* L, int i) {
    return lua::type(L, i + 1) == LUA_TBOOLEAN;
}
uint8_t __cdecl Script_IsNumber(lua_State* L, int i) {
    return lua::isnumber(L, i + 1) != 0;
}
uint8_t __cdecl Script_IsString(lua_State* L, int i) {
    return lua::type(L, i + 1) == LUA_TSTRING;
}
int __cdecl Script_GetInt(lua_State* L, int i) {
    return slgRound(lua::tonumber(L, i + 1));
}
int __cdecl Script_GetIntOpt(lua_State* L, int i, int def) {
    if (lua::gettop(L) < i + 1) {
        return def;
    }
    return slgRound(lua::tonumber(L, i + 1));
}
float __cdecl Script_GetFloat(lua_State* L, int i) {
    return lua::tonumber(L, i + 1);
}
float __cdecl Script_GetFloatOpt(lua_State* L, int i, float def) {
    if (lua::gettop(L) < i + 1) {
        return def;
    }
    return lua::tonumber(L, i + 1);
}
const char* __cdecl Script_GetString(lua_State* L, int i) {
    return lua::tostring(L, i + 1);
}
const char* __cdecl Script_GetStringOpt(lua_State* L, int i, const char* def) {
    if (lua::gettop(L) < i + 1) {
        return def;
    }
    return lua::tostring(L, i + 1);
}
ScriptVector* __cdecl Script_GetVector(ScriptVector* out, lua_State* L, int i) {
    out->x = 0.0f;
    out->y = 0.0f;
    out->z = 0.0f;
    out->x = lua::tonumber(L, i + 1);
    out->y = lua::tonumber(L, i + 2);
    out->z = lua::tonumber(L, i + 3);
    return out;
}
// (out is cleared first: a `def` that is `out` reads zeros)
ScriptVector* __cdecl Script_GetVectorOpt(ScriptVector* out, lua_State* L, int i, const ScriptVector* def) {
    out->x = 0.0f;
    out->y = 0.0f;
    out->z = 0.0f;
    float d = def->x;
    out->x = lua::gettop(L) < i + 1 ? d : lua::tonumber(L, i + 1);
    d = def->y;
    out->y = lua::gettop(L) < i + 2 ? d : lua::tonumber(L, i + 2);
    d = def->z;
    out->z = lua::gettop(L) < i + 3 ? d : lua::tonumber(L, i + 3);
    return out;
}
void __cdecl Script_PushBool(lua_State* L, int b) {
    lua::pushboolean(L, uint8_t(b));
}
void __cdecl Script_PushInt(lua_State* L, int n) {
    lua::pushnumber(L, float(n));
}
void __cdecl Script_PushFloat(lua_State* L, float n) {
    lua::pushnumber(L, n);
}
void __cdecl Script_PushVector(lua_State* L, const ScriptVector* v) {
    lua::pushnumber(L, v->x);
    lua::pushnumber(L, v->y);
    lua::pushnumber(L, v->z);
}
uint8_t __cdecl Script_IsFunction(lua_State* L, int i) {
    return lua::type(L, i + 1) == LUA_TFUNCTION;
}
void __cdecl Script_PushRef(lua_State* L, int ref) {
    lua::rawgeti(L, LUA_REGISTRYINDEX, ref);
}
uint8_t __cdecl Script_IsUserdata(lua_State* L, int i) {
    return lua::type(L, i + 1) == LUA_TLIGHTUSERDATA;
}
void* __cdecl Script_GetUserdata(lua_State* L, int i) {
    return lua::touserdata(L, i + 1);
}

// 0x5D86F0 (thiscall on a script, unused): whether the ped runs the action node (in
// one of its three node lists, or as one of three current nodes: then only while the
// ped's byte +0x1C98 is clear; set, 0x5F3830 runs and it's false)
uint8_t __fastcall LuaScript_PedUsesNode(void*, re::Edx, uint8_t* ped, void* node) {
    if (ped == nullptr || node == nullptr) {
        return 0;
    }
    if (slgNodeList_Contains(slgAt<uint8_t*>(ped, 0x2E0) + 0x258, node, 1, 0)
        || slgNodeList_Contains(slgAt<uint8_t*>(ped, 0x2E0) + 0x290, node, 1, 0)
        || slgNodeList_Contains(slgAt<uint8_t*>(ped, 0x2E0) + 0x2C8, node, 1, 0)) {
        return 1;
    }
    uint8_t* actions = slgAt<uint8_t*>(ped, 0x2E0);
    if (slgAt<void*>(actions, 0x398) == node || slgAt<void*>(actions, 0x340) == node || slgAt<void*>(actions, 0x3F0) == node) {
        if (slgAt<uint8_t>(ped, 0x1C98) == 0) {
            return 1;
        }
        slgNodeList_5F3830(actions + 0x300, slgAt<int32_t>(actions, 0x4B0));
    }
    return 0;
}

// 0x5D87B0 (thiscall on a script, unused): the ped's actions reset
void __fastcall LuaScript_ResetPedActions(void*, re::Edx, uint8_t* ped) {
    if (ped != nullptr) {
        slgPedActions_4840A0(slgAt<void*>(ped, 0x2E0));
        slgPed_474090(ped);
    }
}

// 0x5D87D0 (thiscall on a script, unused): whether the prop (with an animation, +0x228)
// runs the node
uint8_t __fastcall LuaScript_PropUsesNode(void*, re::Edx, uint8_t* prop, void* node) {
    if (prop != nullptr && node != nullptr && slgAt<int32_t>(prop, 0x228) != 0) {
        slgProp_UpdateAnim(prop);
        if (slgNodeList_Contains(prop + 0x128, node, 1, 0)) {
            return 1;
        }
    }
    return 0;
}

// 0x5D8990: a free slot of the 16-byte table at 0xCFC9E0 (+0 and +8 -1, +0xC clear)
// takes the four values; `*slotOut` (when given) the slot
uint8_t __cdecl ScriptCallbacks_Add(int32_t a, int32_t b, int32_t c, re::Ptr32<int32_t>* slotOut) {
    int32_t* table = reinterpret_cast<int32_t*>(0x00CFC9E0);
    int index = 0;
    int32_t* entry = table;
    for (;;) {
        if (entry[0] == -1 && entry[2] == -1 && entry[3] == 0) {
            break;
        }
        entry += 4;
        index++;
        if (reinterpret_cast<uintptr_t>(entry + 2) >= 0x00D02848) {
            return 0;
        }
    }
    if (index == -1) {
        return 0;
    }
    entry = table + index * 4;
    entry[0] = a;
    entry[2] = c;
    entry[1] = b;
    entry[3] = reinterpret_cast<int32_t>(slotOut);
    if (slotOut != nullptr) {
        *slotOut = entry;
    }
    return 1;
}

// 0x5D8A00
void __cdecl LuaMemory_UpdateUsed() {
    uint32_t simple = slgHeap_GetMemoryUsed(slgSimpleHeap, 0x23);
    slgLuaMemoryUsed = simple + slgHeap_GetMemoryUsed(slgMainHeap, 0x23);
}

// 0x5D8C40: loads a script's chunk: a name not ending ".lua" from TheLurStore (its
// streamed data), else the .lur of that name read from Scripts\Scripts.IMG into the
// buffer at 0xBD7800 (size 0 when it isn't there)
void __cdecl Script_LoadChunk(lua_State* L, const char* name) {
    size_t length = 0;
    while (name[length] != '\0') {
        length++;
    }
    const char* end = name + length - 4;
    const char* lua = ".lua";
    bool isLua = true;
    for (int i = 0; i < 5; i++) {
        if (end[i] != lua[i]) {
            isLua = false;
            break;
        }
    }
    if (!isLua) {
        int lur = slgLurStore_Find(slgLurStore, name);
        const void* data = slgLurStore_GetData(slgLurStore, lur);
        uint32_t size = slgLurStore_GetSize(slgLurStore, lur);
        slgLuaL_loadbuffer(L, data, size, name);
        return;
    }
    uint32_t simple = slgHeap_GetMemoryUsed(slgSimpleHeap, 0x23);
    slgLuaMemoryUsed = simple + slgHeap_GetMemoryUsed(slgMainHeap, 0x23);
    char path[64];
    char* d = path;
    const char* s = name;
    char c;
    do {
        c = *s++;
        *d++ = c;
    } while (c != '\0');
    size_t pathLength = 0;
    while (path[pathLength] != '\0') {
        pathLength++;
    }
    path[pathLength - 1] = 'r';
    void* file = slgFileMgr_Open("Scripts\\Scripts.IMG", "r", 1);
    int size = 0;
    if (file != nullptr) {
        char* base = path;
        if (path[0] != '\0') {
            for (char* p = path; *p != '\0'; p++) {
                if (*p == '/' || *p == '\\') {
                    base = p;
                }
            }
            if (base != path) {
                base++;
            }
        }
        int32_t offset;
        int32_t sectors;
        if (slgImgDir_Find(slgScriptImgDir, base, &offset, &sectors)) {
            size = sectors << 11;
            slgFileMgr_Seek(file, offset << 11, 0);
            slgFileMgr_Read(file, slgScriptBuffer, size);
        }
        slgFileMgr_Close(file);
    }
    slgLuaL_loadbuffer(L, slgScriptBuffer, uint32_t(size), path);
    simple = slgHeap_GetMemoryUsed(slgSimpleHeap, 0x23);
    slgLuaMemoryUsed = simple + slgHeap_GetMemoryUsed(slgMainHeap, 0x23);
}

// 0x5D9080: a number (rounded as GetInt) != 0, else lua_toboolean
uint32_t __cdecl Script_GetBool(lua_State* L, int i) {
    if (lua::isnumber(L, i + 1) == 0) {
        return lua::toboolean(L, i + 1) != 0;
    }
    return slgRound(lua::tonumber(L, i + 1)) != 0;
}

// 0x5D9100: GetBool for a boolean or a number, else `def`
uint8_t __cdecl Script_GetBoolOpt(lua_State* L, int i, int def) {
    if (lua::type(L, i + 1) != LUA_TBOOLEAN && lua::isnumber(L, i + 1) == 0) {
        return uint8_t(def);
    }
    return Script::GetBool(L, i);
}

// The callbacks' table (0x5D8990) and the script of a state
GAME_FN(uint8_t, __cdecl, slgCallbacks_Add, 0x005D8990, int32_t script, int32_t thread, int32_t ref, void* owner);
GAME_FN(void, __thiscall, slgCallback_Release, 0x005D9BC0, void* callback);
GAME_FN(uint8_t*, __thiscall, slgLurStore_GetScript, 0x005DBD50, void* store, lua_State* L);
LIB_FN(int, __cdecl, slgLuaL_ref, luaL_ref, lua_State* L, int t);

// The current thread's id of a script (-1: none), inlined in the two below
int slgCurrentThreadId(uint8_t* script) {
    uint32_t index = slgAt<uint32_t>(script, 0x114C);
    if (int32_t(index) > -1 && index < slgAt<uint32_t>(script, 0x1148)) {
        uint8_t* thread = script + 0x48 + index * 0x44;
        if (thread != nullptr) {
            return slgAt<int32_t>(thread, 0x20);
        }
    }
    return -1;
}

// 0x5D9FD0: argument i (a function) into the registry, as a callback of the running
// script's current thread owned by *owner (an older one released first); true
uint8_t __cdecl Script_StoreFunctionRef(lua_State* L, int i, re::Ptr32<void>* owner) {
    lua::pushvalue(L, i + 1);
    int ref = slgLuaL_ref(L, LUA_REGISTRYINDEX);
    if (*owner != nullptr && slgAt<int32_t>(*owner, 0xC) != 0) {
        slgCallback_Release(*owner);
    }
    uint8_t* script = slgLurStore_GetScript(slgLurStore, L);
    int thread = slgCurrentThreadId(script);
    slgCallbacks_Add(slgAt<int32_t>(script, 0x44), thread, ref, owner);
    return 1;
}

// 0x5DA060: the same when there are that many arguments (and the reference is good).
// (The game's al is left from whatever ran last: returned as nothing here.)
void __cdecl Script_StoreFunctionRefOpt(lua_State* L, int i, re::Ptr32<void>* owner) {
    if (lua::gettop(L) < i + 1) {
        return;
    }
    lua::pushvalue(L, i + 1);
    int ref = slgLuaL_ref(L, LUA_REGISTRYINDEX);
    if (*owner != nullptr && slgAt<int32_t>(*owner, 0xC) != 0) {
        slgCallback_Release(*owner);
    }
    if (ref > -1) {
        uint8_t* script = slgLurStore_GetScript(slgLurStore, L);
        int thread = slgCurrentThreadId(script);
        slgCallbacks_Add(slgAt<int32_t>(script, 0x44), thread, ref, owner);
    }
}

// 0x5DA110: argument i as a vehicle handle: the vehicle (nullptr for a stale one)
struct SlgPool {
    re::Ptr32<uint8_t> objects;
    re::Ptr32<uint8_t> flags;
    int32_t size;
    int32_t elemSize;
};
GAME_VAR(SlgPool*, slgVehiclePool, 0x00C0F5F4);
void* __cdecl Script_GetVehicle(lua_State* L, int i) {
    int handle = slgRound(lua::tonumber(L, i + 1));
    SlgPool* pool = slgVehiclePool;
    int slot = handle >> 8;
    if (pool->flags[slot] == uint8_t(handle)) {
        return pool->objects + pool->elemSize * slot;
    }
    return nullptr;
}

} // namespace

#define SLG_REVERSED(name, address, fn)                   \
    static void* RE_CONCAT(slgOriginal_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(slgOriginal_, __LINE__), re::Mode::Original)

SLG_REVERSED("ScriptArray::Construct_5D7EC0", 0x005D7EC0, &ScriptArray_Construct);
SLG_REVERSED("RegisteredPointerAssign_5D7F10", 0x005D7F10, &RegisteredPointerAssign);
SLG_REVERSED("LuaMemory::Init", 0x005D7F50, &LuaMemory_Init);
SLG_REVERSED("LuaMemory::Realloc", 0x005D8010, &LuaMemory_Realloc);
SLG_REVERSED("LuaMemory::Free", 0x005D8070, &LuaMemory_Free);
SLG_REVERSED("ScriptRef::GetScript", 0x005D80B0, &ScriptRef_GetScript);
SLG_REVERSED("ScriptRef::Get4", 0x005D80C0, &ScriptRef_Get4);
SLG_REVERSED("ScriptRef::GetFunction", 0x005D80D0, &ScriptRef_GetFunction);
SLG_REVERSED("ScriptRef::IsSet", 0x005D80E0, &ScriptRef_IsSet);
SLG_REVERSED("CLuaScript::SetName", 0x005D8100, &LuaScript_SetName);
SLG_REVERSED("CLuaScript::CallInNamespace", 0x005D8160, &LuaScript_CallInNamespace);
SLG_REVERSED("CLuaScript::CallForInt", 0x005D8200, &LuaScript_CallForInt);
SLG_REVERSED("Lua::PushErrorHandler", 0x005D8270, &Lua_PushErrorHandler);
SLG_REVERSED("Lua::CallProtected", 0x005D82A0, &Lua_CallProtected);
SLG_REVERSED("Script::IsNil", 0x005D82E0, &Script_IsNil);
SLG_REVERSED("Script::IsBoolean", 0x005D8300, &Script_IsBoolean);
SLG_REVERSED("Script::IsNumber", 0x005D8320, &Script_IsNumber);
SLG_REVERSED("Script::IsString", 0x005D8340, &Script_IsString);
SLG_REVERSED("Script::GetInt", 0x005D8360, &Script_GetInt);
SLG_REVERSED("Script::GetIntOpt", 0x005D83A0, &Script_GetIntOpt);
SLG_REVERSED("Script::GetFloat", 0x005D8400, &Script_GetFloat);
SLG_REVERSED("Script::GetFloatOpt", 0x005D8410, &Script_GetFloatOpt);
SLG_REVERSED("Script::GetString", 0x005D8440, &Script_GetString);
SLG_REVERSED("Script::GetStringOpt", 0x005D8450, &Script_GetStringOpt);
SLG_REVERSED("Script::GetVector", 0x005D8480, &Script_GetVector);
SLG_REVERSED("Script::GetVectorOpt", 0x005D84D0, &Script_GetVectorOpt);
SLG_REVERSED("Script::PushBool", 0x005D85B0, &Script_PushBool);
SLG_REVERSED("Script::PushInt", 0x005D85C0, &Script_PushInt);
SLG_REVERSED("Script::PushFloat", 0x005D85E0, &Script_PushFloat);
SLG_REVERSED("Script::PushVector", 0x005D8610, &Script_PushVector);
SLG_REVERSED("Script::IsFunction", 0x005D8650, &Script_IsFunction);
SLG_REVERSED("Script::PushRef", 0x005D8670, &Script_PushRef);
SLG_REVERSED("Script::IsUserdata", 0x005D86C0, &Script_IsUserdata);
SLG_REVERSED("Script::GetUserdata", 0x005D86E0, &Script_GetUserdata);
SLG_REVERSED("CLuaScript::PedUsesNode", 0x005D86F0, &LuaScript_PedUsesNode);
SLG_REVERSED("CLuaScript::ResetPedActions", 0x005D87B0, &LuaScript_ResetPedActions);
SLG_REVERSED("CLuaScript::PropUsesNode", 0x005D87D0, &LuaScript_PropUsesNode);
SLG_REVERSED("ScriptCallbacks::Add", 0x005D8990, &ScriptCallbacks_Add);
SLG_REVERSED("LuaMemory::UpdateUsed", 0x005D8A00, &LuaMemory_UpdateUsed);
SLG_REVERSED("Script::LoadChunk", 0x005D8C40, &Script_LoadChunk);
SLG_REVERSED("Script::GetBool", 0x005D9080, &Script_GetBool);
SLG_REVERSED("Script::GetBoolOpt", 0x005D9100, &Script_GetBoolOpt);
SLG_REVERSED("Script::StoreFunctionRef", 0x005D9FD0, &Script_StoreFunctionRef);
SLG_REVERSED("Script::StoreFunctionRefOpt", 0x005DA060, &Script_StoreFunctionRefOpt);
SLG_REVERSED("Script::GetVehicle", 0x005DA110, &Script_GetVehicle);
