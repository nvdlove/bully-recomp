// The animation manager's lookups: an animation group's entry by name, and a
// hierarchy file cached ("CacheHie..." at start). 0x42E350, 0x42E3C0.

#include <cstdint>
#include <cstring>
#include <malloc.h>

#include "reversed.h"

#include "game/Crt.h"
#define ANIMLOOKUP_REVERSED(name, address, fn)                                               \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace {

inline int __cdecl AL_stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
GAME_FN(int, __thiscall, AL_Anim_FindFile, 0x006BD0D0, void* manager, const char* path, char* out, int kind);
GAME_FN(void, __thiscall, AL_Anim_LoadHierarchy, 0x006C1950, void* manager, char* name, int a, int b, uintptr_t c);

} // namespace

namespace CAnimManager {

// 0x42E350 (thiscall, on a group set: +0x68 the count, +0x6C the entries, 0x3C each with
// the name at +4): the entry named so (any case), its index in *index (-1: none)
uint8_t* __fastcall FindGroupEntry(uint8_t* set, re::Edx, const char* name, uint32_t* index) {
    uint32_t count = *reinterpret_cast<uint32_t*>(set + 0x68);
    for (uint32_t i = 0; i < count; i++) {
        uint8_t* entries = *reinterpret_cast<re::Ptr32<uint8_t>*>(set + 0x6C);
        if (AL_stricmp(reinterpret_cast<const char*>(entries + i * 0x3C + 4), name) == 0) {
            if (index) {
                *index = i;
            }
            return *reinterpret_cast<re::Ptr32<uint8_t>*>(set + 0x6C) + i * 0x3C;
        }
        count = *reinterpret_cast<uint32_t*>(set + 0x68);
    }
    if (index) {
        *index = 0xFFFFFFFF;
    }
    return nullptr;
}

// 0x42E3C0 (thiscall): a hierarchy loaded unless 0x6BD0D0 finds it (kind 2) already;
// the name copied onto the stack first (the manager's calls write into the copy)
void __fastcall CacheHierarchy(void* manager, re::Edx, const char* name) {
    size_t size = std::strlen(name) + 1;
    char* copy = static_cast<char*>(_alloca(size));
    std::memcpy(copy, name, size);
    if (AL_Anim_FindFile(manager, name, copy, 2) == 0) {
        AL_Anim_LoadHierarchy(manager, copy, 0, 1, 0);
    }
}

ANIMLOOKUP_REVERSED("CAnimManager::FindGroupEntry", 0x0042E350, &FindGroupEntry);
ANIMLOOKUP_REVERSED("CAnimManager::CacheHierarchy", 0x0042E3C0, &CacheHierarchy);

} // namespace CAnimManager
