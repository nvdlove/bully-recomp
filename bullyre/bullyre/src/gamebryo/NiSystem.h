#pragma once

// What the files of Gamebryo's system and streaming layer share (NiMemory, NiFile, NiPath,
// NiGlobalStringTable, NiObject, NiRenderer, NiShader*, NiShadowManager, NiStream): byte
// access, virtual calls by slot, the Win32 imports through the game's slots, and the stateful
// pieces of the game's C runtime it uses (from src/game/Crt.h). Everything in
// ni::sys so nothing clashes with the scene-graph files' helpers (NiSceneCommon.h).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni::sys {

using re::At;

// a virtual call by slot (thiscall), as the library's code makes them
template <typename R = void, typename... A>
inline R Virtual(const void* object, int slot, A... args) {
    return reinterpret_cast<R(__thiscall*)(const void*, re::Arg<A>...)>(static_cast<void*>(VtableOf(object)[slot]))(object, args...);
}

// ---- Win32, through the game's import slots (read at call time)

template <typename R, typename... A>
inline R Import(uintptr_t slot, A... args) {
    return reinterpret_cast<R(__stdcall*)(re::Arg<A>...)>(static_cast<void*>(*reinterpret_cast<re::Ptr32<void>*>(slot)))(args...);
}
constexpr uintptr_t kQueryPerformanceCounter = 0x008FC040;
constexpr uintptr_t kQueryPerformanceFrequency = 0x008FC044;
constexpr uintptr_t kCloseHandle = 0x008FC050;
constexpr uintptr_t kInitializeCriticalSection = 0x008FC084;
constexpr uintptr_t kDeleteCriticalSection = 0x008FC088;
constexpr uintptr_t kCreateSemaphoreA = 0x008FC0BC;
constexpr uintptr_t kGetSystemInfo = 0x008FC0E4;
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

// NiCriticalSection: the Win32 one, its owner thread and how many times it holds it
struct NiCriticalSection {
    uint8_t section[0x18];   // +0x0 (CRITICAL_SECTION)
    uint32_t owner;          // +0x18
    uint32_t lockCount;      // +0x1C
};
static_assert(sizeof(NiCriticalSection) == 0x20);

inline void Lock(NiCriticalSection& cs) {
    Import<void>(kEnterCriticalSection, &cs.section);
    cs.owner = Import<uint32_t>(kGetCurrentThreadId);
    cs.lockCount++;
}
inline void Unlock(NiCriticalSection& cs) {
    if (--cs.lockCount == 0) {
        cs.owner = 0;
    }
    Import<void>(kLeaveCriticalSection, &cs.section);
}

// ---- NiFixedString's reference counting as the library inlines it (the count 8 bytes before
// the characters, the bucket 2 bytes before them)

GAME_VAR(char*, kNullFixedString, 0x00951028);   // the empty handle the code compares with
GAME_VAR(void*, kGlobalStringTable, 0x020E216C);
GAME_FN(void, __thiscall, GlobalStringTable_RemoveString, 0x00754550, void* table, re::Ptr32<char>* handle, uint32_t hash);
GAME_FN(char*, __cdecl, GlobalStringTable_AddString, 0x00754760, const char* text);

// a handle let go
inline void FixedStringRelease(re::Ptr32<char>* handle) {
    char* s = *handle;
    if (s != kNullFixedString) {
        uint32_t hash = *reinterpret_cast<uint16_t*>(s - 2);
        if (Decrement(s - 8) == 1) {
            GlobalStringTable_RemoveString(kGlobalStringTable, handle, hash);
        }
    }
}

// a handle copied (the reference counted)
inline char* FixedStringAddRef(char* s) {
    if (s != kNullFixedString) {
        Increment(s - 8);
    }
    return s;
}

// ---- the game's C runtime pieces that keep state (its heap, its FILE table): Crt.h's
using crt::AlignedFree;
using crt::AlignedMalloc;
using crt::AlignedRealloc;
using crt::FopenS;
using crt::Fread;
using crt::Fseek;
using crt::Ftell;
using crt::Fwrite;
using crt::Realloc;

} // namespace ni::sys
