#pragma once

// The value-holding kinds of NiExtraData (src/gamebryo/NiExtraData.h), from their constructors
// and CreateClones (src/gamebryo/NiExtraData2.cpp):
//   NiFloatExtraData    (vtable 0x8FFA8C, 0x10 bytes)
//   NiIntegerExtraData  (vtable 0x8FFAE4, 0x10 bytes)
//   NiColorExtraData    (vtable 0x9524F0, 0x1C bytes)
//   NiFloatsExtraData   (vtable 0x952860, 0x14 bytes)
//   NiIntegersExtraData (vtable 0x952B88, 0x14 bytes)
// The arrays are NiMalloc'd and owned.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiExtraData.h"
#include "reversed.h"

namespace ni {

struct NiFloatExtraData : NiExtraData {
    float value;          // +0xC
};
static_assert(sizeof(NiFloatExtraData) == 0x10);

struct NiIntegerExtraData : NiExtraData {
    int32_t value;        // +0xC
};
static_assert(sizeof(NiIntegerExtraData) == 0x10);

struct NiColorExtraData : NiExtraData {
    float color[4];       // +0xC (NiColorA: r, g, b, a)
};
static_assert(sizeof(NiColorExtraData) == 0x1C);

struct NiFloatsExtraData : NiExtraData {
    uint32_t size;        // +0xC
    re::Ptr32<float> values;        // +0x10
};
static_assert(offsetof(NiFloatsExtraData, values) == 0x10);
static_assert(sizeof(NiFloatsExtraData) == 0x14);

struct NiIntegersExtraData : NiExtraData {
    uint32_t size;        // +0xC
    re::Ptr32<int32_t> values;      // +0x10
};
static_assert(sizeof(NiIntegersExtraData) == 0x14);

} // namespace ni
