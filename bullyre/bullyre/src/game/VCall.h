#pragma once

// Small helpers for code that calls through vtables and the game's import slots.

#include <cstdint>
#include "reversed.h"

namespace vc {

// obj's virtual at byte offset `offset` in its vtable (thiscall). The game's layout in both
// builds: a 4-byte vtable pointer, 4-byte slots (re::Ptr32)
template <typename R, typename... A>
inline R Call(const void* obj, int offset, A... args) {
    const re::Ptr32<void>* vtable = *static_cast<const re::Ptr32<re::Ptr32<void>>*>(obj);
    void* fn = vtable[offset / 4];
    return reinterpret_cast<R(__thiscall*)(const void*, re::Arg<A>...)>(fn)(obj, args...);
}

// a Win32 import through the game's slot (read at call time)
template <typename R, typename... A>
inline R Import(uintptr_t slot, A... args) {
    return reinterpret_cast<R(__stdcall*)(re::Arg<A>...)>(static_cast<void*>(*reinterpret_cast<re::Ptr32<void>*>(slot)))(args...);
}

// the slots used here
constexpr uintptr_t kInterlockedDecrement = 0x008FC170;
constexpr uintptr_t kInterlockedIncrement = 0x008FC174;
constexpr uintptr_t kLeaveCriticalSection = 0x008FC178;
constexpr uintptr_t kEnterCriticalSection = 0x008FC17C;
constexpr uintptr_t kGetCurrentThreadId = 0x008FC180;

inline long Increment(void* p) {
    return Import<long>(kInterlockedIncrement, p);
}
inline long Decrement(void* p) {
    return Import<long>(kInterlockedDecrement, p);
}

using re::At;

} // namespace vc
