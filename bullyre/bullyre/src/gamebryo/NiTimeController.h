#pragma once

// NiTimeController (vtable 0x953E98, 0x34 bytes), NiShaderTimeController (vtable 0x953DE8, 0x34)
// and NiTimeSyncController (vtable 0x9503E8, 0x38), from the constructors at 0x77DFA0 and 0x77D770.
// The controllers of an object chain through `next` (its first is NiObjectNET's controller).
//
// Virtual slots past NiObject's: 18 Start(time), 23 IsVertexController, 26 IsStreamable (its
// chain is saved from the first streamable one).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni {

struct NiTimeController : NiObject {
    uint16_t flags;            // +0x8 (bit 0 anim type APP_INIT, bits 1..2 cycle type (loop,
                               //  reverse, clamp), bit 3 active, bit 4 play backwards, bit 5
                               //  manager controlled, bit 6 compute scaled time; 0x4C by default)
    uint16_t padA;
    float frequency;           // +0xC (1)
    float phase;               // +0x10 (0)
    float loKeyTime;           // +0x14 (FLT_MAX; NiShaderTimeController's: -FLT_MAX)
    float hiKeyTime;           // +0x18 (-FLT_MAX; NiShaderTimeController's: FLT_MAX)
    float startTime;           // +0x1C (-FLT_MAX)
    float lastTime;            // +0x20 (-FLT_MAX)
    float weightedLastTime;    // +0x24 (0)
    float scaledTime;          // +0x28 (-FLT_MAX)
    re::Ptr32<NiObjectNET> target;       // +0x2C (not a reference)
    re::Ptr32<NiTimeController> next;    // +0x30 (NiTimeControllerPtr)
};
static_assert(offsetof(NiTimeController, frequency) == 0xC);
static_assert(offsetof(NiTimeController, target) == 0x2C);
static_assert(offsetof(NiTimeController, next) == 0x30);
static_assert(sizeof(NiTimeController) == 0x34);

struct NiTimeSyncController : NiTimeController {
    re::Ptr32<NiObject> unknown34;       // +0x34 (a reference)
};
static_assert(sizeof(NiTimeSyncController) == 0x38);

} // namespace ni
