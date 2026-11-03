#pragma once

// What the NativesC*.cpp files share: the game objects the natives reach for and the
// handle lookups (in an anonymous namespace, so nothing clashes with other passes'
// names). Each file defines its registering macro, NATC_REVERSED, itself.

#include <cstdint>

#include "game/ScriptApi.h"
#include "reversed.h"

namespace {

// A field of a game object at a byte offset (for objects only a field or two of
// which are touched here)
using re::At;

// A virtual (thiscall) at a byte offset in the object's vtable
template <typename R, typename... A>
R VCall(void* self, uint32_t offset, A... args) {
    using Fn = R(__thiscall*)(void*, re::Arg<A>...);
    Fn fn = re::GameFnAt<Fn>(*static_cast<re::Ptr32<uint8_t>*>(self) + offset);
    return fn(self, args...);
}

// The player's info (GTA's CPlayerInfo: +0 the ped); the ped itself
inline uint8_t* const playerInfo = reinterpret_cast<uint8_t*>(0x00C1AEA8);
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);
// punishment (trouble meter) state
GAME_VAR(uint8_t*, punishment, 0x00C0F78C);
GAME_VAR(uint8_t*, targeting, 0x00C674B4);
GAME_VAR(void*, weaponHud, 0x00C674F0);
GAME_VAR(void*, detentionMeter, 0x00C674A0);
GAME_VAR(void*, raceHud, 0x00C674C8);
// the minigame (or class) running, nullptr for none
GAME_VAR(uint8_t*, currentMinigame, 0x020C8704);

// 0x5C7380: the ped of a script handle (nullptr for -1 or a stale one); kind 3 is
// the player, whatever the handle
inline constexpr int PED_HANDLE_PLAYER = 3;
GAME_FN(uint8_t*, __cdecl, GetPedFromHandle, 0x005C7380, int handle, int kind);
// 0x44A580: CPools::GetVehicle
GAME_FN(uint8_t*, __cdecl, GetVehicleFromHandle, 0x0044A580, int handle);

// 0x5D9FD0: argument i (a function) to a registry reference in the callback (always 1)
GAME_FN(uint8_t, __cdecl, StoreCallback, 0x005D9FD0, lua_State* L, int i, void* callback);
// 0x5DA060: the same, when there is such an argument
GAME_FN(void, __cdecl, StoreCallbackOpt, 0x005DA060, lua_State* L, int i, void* callback);
// 0x5D80E0: the callback holds a live reference
GAME_FN(uint8_t, __thiscall, Callback_IsSet, 0x005D80E0, void* callback);

} // namespace
