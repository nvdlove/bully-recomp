#pragma once

// What NativesMissions.cpp, NativesObjects.cpp and NativesPedScript*.cpp share (the
// script natives at 0x5B8F10..0x5CD4B0): field access, virtual calls and the handle
// lookups, in an anonymous namespace so nothing clashes with other files' names.

#include <cstdint>

#include "game/Pool.h"
#include "game/ScriptApi.h"
#include "reversed.h"

namespace {

// A field of a game object at a byte offset (for objects only a field or two of which
// are touched here)
using re::At;

// A virtual (thiscall) at a byte offset in the object's vtable
template <typename R, typename... A>
R VCall(void* self, int offset, A... args) {
    using Fn = R(__thiscall*)(void*, re::Arg<A>...);
    Fn fn = re::GameFnAt<Fn>(*static_cast<re::Ptr32<uint8_t>*>(self) + offset);
    return fn(self, args...);
}

// The script handle kinds (the second result of some natives, GetPedFromHandle's kind)
inline constexpr int HANDLE_OBJECT = 0;
inline constexpr int HANDLE_VEHICLE = 1;
inline constexpr int HANDLE_PED = 2;
inline constexpr int HANDLE_PLAYER = 3;
inline constexpr int HANDLE_BUILDING = 4;
inline constexpr int HANDLE_PROPANIM = 5;

// 0x5C7380: the ped of a script handle
GAME_FN(uint8_t*, __cdecl, M_GetPedFromHandle, 0x005C7380, int handle, int kind);
// The player's info (+0 the ped)
GAME_VAR(uint8_t*, m_playerPed, 0x00C1AEA8);
inline uint8_t* const m_playerInfo = reinterpret_cast<uint8_t*>(0x00C1AEA8);

// The pools
GAME_VAR(CPool<uint8_t>*, m_pedPool, 0x00C0F5F0);
GAME_VAR(CPool<uint8_t>*, m_vehiclePool, 0x00C0F5F4);
GAME_VAR(CPool<uint8_t>*, m_buildingPool, 0x00C0F5F8);
GAME_VAR(CPool<uint8_t>*, m_propAnimPool, 0x00C0F608);
GAME_VAR(CPool<uint8_t>*, m_objectPool, 0x00C0F614);
GAME_VAR(CPool<uint8_t>*, m_projectilePool, 0x00C0F618);
// the triggers (slot 0 is the world)
GAME_VAR(CPool<uint8_t>*, m_triggerPool, 0x020C7C48);
inline void* const m_triggerPoolWrapper = reinterpret_cast<void*>(0x020C7E98);
GAME_ARRAY(uint8_t*, m_modelInfoPtrs, 0x00C67738);

inline void* const m_lurStore = reinterpret_cast<void*>(0x00D02850);
GAME_FN(void*, __thiscall, M_Scripts_GetCurrent, 0x005DB940, void* store);

// A pool's object at a slot (nullptr when free or negative)
inline uint8_t* PoolSlot(CPool<uint8_t>* pool, int slot) {
    if (slot < 0 || (pool->flags[slot] & 0x80) != 0) {
        return nullptr;
    }
    return pool->objects + pool->elemSize * slot;
}

// An object's handle: slot * 0x100 + its flag byte
inline int PoolHandle(CPool<uint8_t>* pool, uint8_t* object) {
    int slot = int(object - pool->objects) / pool->elemSize;
    return pool->flags[slot] + slot * 0x100;
}

// The entity's position (its matrix's, else the placeable's own)
inline float* EntityPos(uint8_t* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix != nullptr ? reinterpret_cast<float*>(matrix + 0x30) : reinterpret_cast<float*>(entity + 4);
}

} // namespace
