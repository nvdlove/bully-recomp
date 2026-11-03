#pragma once

// NiExtraData (vtable 0x952010, 0xC bytes) and NiBooleanExtraData (vtable 0x8FFA24, 0x10 bytes),
// from the constructor at 0x765640 and NiBooleanExtraData::CreateClone (0x765810). Extra data is
// kept on its NiObjectNET by name (the handle's value orders the array).
//
// Virtual slots past NiObject's: 18 IsStreamable, 19 IsCloneable.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni {

struct NiExtraData : NiObject {
    re::Ptr32<const char> name;   // +0x8
};
static_assert(sizeof(NiExtraData) == 0xC);

struct NiBooleanExtraData : NiExtraData {
    uint8_t value;        // +0xC
    uint8_t padD[3];
};
static_assert(sizeof(NiBooleanExtraData) == 0x10);

} // namespace ni
