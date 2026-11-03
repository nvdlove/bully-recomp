#pragma once

// Helpers for the transcribed static initializers (StaticInit*.cpp): the game's globals by
// address, and calls through game addresses with dword arguments, as the code pushes them.

#include <cstdint>
#include <intrin.h>

#include "game/Crt.h"
#include "reversed.h"

namespace staticinit {

inline uint32_t& Dword(uintptr_t address) {
    return *reinterpret_cast<uint32_t*>(address);
}
inline float& Float(uintptr_t address) {
    return *reinterpret_cast<float*>(address);
}
inline const double& Double(uintptr_t address) {
    return *reinterpret_cast<const double*>(address);
}
inline uint8_t& Byte(uintptr_t address) {
    return *reinterpret_cast<uint8_t*>(address);
}
inline uint16_t& Word(uintptr_t address) {
    return *reinterpret_cast<uint16_t*>(address);
}
// a double copied through the x87 (fld/fstp qword), possibly unaligned: its bits
inline uint64_t& Qword(uintptr_t address) {
    return *reinterpret_cast<uint64_t*>(address);
}
// rep stosd
inline void FillDwords(uintptr_t address, uint32_t value, uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        Dword(address + 4 * i) = value;
    }
}
template <typename F>
F Fn(uintptr_t address) {
    return reinterpret_cast<F>(address);
}
// atexit with an exit function by its game address
inline void AtexitGame(uintptr_t fn) {
    crt::Atexit(Fn<void(__cdecl*)()>(fn));
}

template <typename... A>
uint32_t Cdecl(uintptr_t fn, A... args) {
    return reinterpret_cast<uint32_t(__cdecl*)(re::Arg<A>...)>(fn)(args...);
}
template <typename... A>
uint32_t Stdcall(uintptr_t fn, A... args) {
    return reinterpret_cast<uint32_t(__stdcall*)(re::Arg<A>...)>(fn)(args...);
}
template <typename... A>
uint32_t Thiscall(uintptr_t fn, uint32_t self, A... args) {
    return reinterpret_cast<uint32_t(__thiscall*)(uint32_t, re::Arg<A>...)>(fn)(self, args...);
}

// a global NiFixedString's destructor, as the exit functions inline it: unless it's the empty
// string (0x951028's), its count (8 bytes before the characters) drops, and the string table
// (0x20E216C) lets it go when only the table's reference is left
GAME_FN(void, __thiscall, StringTable_Release, 0x00754550, void* table, re::Ptr32<const char>* string, uint32_t length);
inline void ReleaseFixedString(uintptr_t variable) {
    char* s = *reinterpret_cast<re::Ptr32<char>*>(variable);
    if (s == *reinterpret_cast<re::Ptr32<char>*>(0x00951028)) {
        return;
    }
    uint32_t length = *reinterpret_cast<uint16_t*>(s - 2);
    if (_InterlockedDecrement(reinterpret_cast<volatile long*>(s - 8)) == 1) {
        StringTable_Release(*reinterpret_cast<re::Ptr32<void>*>(0x020E216C), reinterpret_cast<re::Ptr32<const char>*>(variable), length);
    }
}

// a global NiPointer's destructor, as the exit functions inline it: the reference dropped,
// DeleteThis (vtable slot 1) when it was the last (the global isn't cleared)
inline void ReleaseObject(uintptr_t variable) {
    uint8_t* p = *reinterpret_cast<re::Ptr32<uint8_t>*>(variable);
    if (p != nullptr && _InterlockedDecrement(reinterpret_cast<volatile long*>(p + 4)) == 0) {
        reinterpret_cast<void(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(p))[1]))(p);
    }
}

} // namespace staticinit
