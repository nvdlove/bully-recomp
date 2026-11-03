#pragma once

// What the ScriptNatives*.cpp files share (the script natives at 0x5CD5D0..0x5D7E40:
// peds, pickups, the player, POIs, shops, sound, stats, text, vehicles): field access,
// virtual calls, the handle lookups. In an anonymous namespace (names prefixed sn) so
// nothing clashes with other files. Each file defines its own registering macro.

#include <cstdint>

#include "game/Pool.h"
#include "game/ScriptApi.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

// A field of a game object at a byte offset (for objects only a field or two of which
// are touched here)
template <typename T>
re::Field<T>& snAt(void* base, int offset) {
    return *reinterpret_cast<re::Field<T>*>(static_cast<uint8_t*>(base) + offset);
}

// A virtual (thiscall) at a byte offset in the object's vtable
template <typename R, typename... A>
R snVCall(void* self, int offset, A... args) {
    using Fn = R(__thiscall*)(void*, re::Arg<A>...);
    Fn fn = re::GameFnAt<Fn>(*static_cast<re::Ptr32<uint8_t>*>(self) + offset);
    return fn(self, args...);
}

// The vector the game passes by value (CVector)
struct SnVec {
    float x, y, z;
};
static_assert(sizeof(SnVec) == 12);

// The script handle kinds (GetPedFromHandle's second argument)
inline constexpr int SN_HANDLE_PED = 2;
inline constexpr int SN_HANDLE_PLAYER = 3;

// 0x5C7380: the ped of a script handle (nullptr for -1 or a stale one; kind 3: the player)
GAME_FN(uint8_t*, __cdecl, snGetPed, 0x005C7380, int handle, int kind);
// 0x5EEAA0: CMemoryMgr::Malloc (operator new)
GAME_FN(void*, __cdecl, snMalloc, 0x005EEAA0, uint32_t size);
// 0x5EEAB0: CMemoryMgr::Free (operator delete)
GAME_FN(void, __cdecl, snFree, 0x005EEAB0, void* p);
// 0x471390: the ped's objective manager (ped + 0x5F0) takes a new objective (nullptr
// too: the game hands on a failed allocation)
GAME_FN(uint8_t, __thiscall, snObjectives_Add, 0x00471390, void* manager, void* objective);
// 0x8B6120: the entity's matrix (CPlaceable::GetMatrix)
GAME_FN(float*, __thiscall, snEntity_GetMatrix, 0x008B6120, void* entity);
// 0x85F5DF: _stricmp
inline int __cdecl snStricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }

// The player's info (GTA's CPlayerInfo, +0 the ped) and the player ped
inline uint8_t* const snPlayerInfo = reinterpret_cast<uint8_t*>(0x00C1AEA8);
GAME_VAR(uint8_t*, snPlayerPed, 0x00C1AEA8);

// The pools
GAME_VAR(CPool<uint8_t>*, snPedPool, 0x00C0F5F0);
GAME_VAR(CPool<uint8_t>*, snVehiclePool, 0x00C0F5F4);
GAME_VAR(CPool<uint8_t>*, snObjectPool, 0x00C0F614);
GAME_ARRAY(uint8_t*, snModelInfoPtrs, 0x00C67738);

// The running script (TheLurStore 0xD02850's current)
inline void* const snLurStore = reinterpret_cast<void*>(0x00D02850);
GAME_FN(uint8_t*, __thiscall, snCurrentScript, 0x005DB940, void* store);

// The ped handle rule most natives use: the ped (kind 2)
inline uint8_t* snPedArg(lua_State* L, int i) {
    return snGetPed(Script::GetInt(L, i), SN_HANDLE_PED);
}

// A pool's object from a script handle, checked the game's inlined way: the slot's flag
// byte must equal the handle's low byte (nullptr otherwise; a negative handle reads
// flags[-1] as the game does)
inline uint8_t* snPoolAt(CPool<uint8_t>* pool, int handle) {
    int slot = handle >> 8;
    if (pool->flags[slot] != uint8_t(handle)) {
        return nullptr;
    }
    return pool->objects + pool->elemSize * slot;
}

// An object's handle: slot * 0x100 + its flag byte
inline int snPoolHandle(CPool<uint8_t>* pool, uint8_t* object) {
    int slot = int(object - pool->objects) / pool->elemSize;
    return pool->flags[slot] + slot * 0x100;
}

// The entity's position (its matrix's, else the placeable's own)
inline float* snEntityPos(uint8_t* entity) {
    uint8_t* matrix = snAt<uint8_t*>(entity, 0x14);
    return matrix != nullptr ? reinterpret_cast<float*>(matrix + 0x30) : reinterpret_cast<float*>(entity + 4);
}

} // namespace
