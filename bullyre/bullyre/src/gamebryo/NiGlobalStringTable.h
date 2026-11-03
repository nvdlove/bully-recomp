#pragma once

// NiGlobalStringTable: the one table every NiFixedString's characters live in. A string is
// NiMalloc'd with an 8-byte header before its characters: the reference count (-8; the table
// holds one), then the length (-4, 16 bits) and the bucket (-2, 16 bits). Buckets: 513
// NiTPrimitiveArray<char*>, chosen by a hash of the characters.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"

namespace ni {

struct NiGlobalStringTable {
    NiTArray<char*> buckets[513];      // +0x0
    sys::NiCriticalSection lock;       // +0x2010
};
static_assert(offsetof(NiGlobalStringTable, lock) == 0x2010);
static_assert(sizeof(NiGlobalStringTable) == 0x2030);

GAME_VAR(NiGlobalStringTable*, ms_pkGlobalStringTable, 0x020E216C);
GAME_VAR(uint32_t, ms_uiNumStrings, 0x020E2170);      // NiGlobalStringTable's string count
GAME_VAR(uint32_t, ms_uiNumCollisions, 0x020E2174);   // strings added to a bucket already used

} // namespace ni
