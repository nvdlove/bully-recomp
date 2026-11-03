// Condition field setters Ghidra's function list missed (found by tools/sweep: called
// from the conditions' factories, ConditionFactories.cpp, right after the vtable is set).

#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

} // namespace

namespace sweepconditions {

// 0x5FBF90 (thiscall): ConditionSetPedFlags' fields (+0x4 set, +0x8 the flags). (The
// factory calls it on a null `this` when the allocation fails, as the game does)
void __fastcall SetPedFlags_Fields(uint8_t* self, re::Edx) {
    self[4] = 1;
    At<int32_t>(self, 8) = 0;
}

// 0x5FC540 (thiscall): ConditionHitTime's fields: the time (+0x4) 0, compared with
// operator 4 (+0x8), the hit's own time (+0xC clear; see ConditionHitTime::Evaluate)
void __fastcall HitTime_Fields(uint8_t* self, re::Edx) {
    At<int32_t>(self, 8) = 4;
    At<float>(self, 4) = 0.0f;
    self[0xC] = 0;
}

} // namespace sweepconditions

// ---- registration ---------------------------------------------------------------------

#define SWEEPCOND_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

SWEEPCOND_REVERSED("ConditionSetPedFlags::Fields", 0x005FBF90, &sweepconditions::SetPedFlags_Fields);
SWEEPCOND_REVERSED("ConditionHitTime::Fields", 0x005FC540, &sweepconditions::HitTime_Fields);
