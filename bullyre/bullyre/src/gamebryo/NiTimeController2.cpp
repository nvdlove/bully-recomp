// NiTimeController's update gate (DontDoUpdate), Stop and ComputeScaledTime (the controller's
// time through its frequency and phase, then its cycle type: loop, reverse or clamp, between
// its key times; the last result cached under a lock for the next controller asking with the
// same values), and NiTimeSyncController's slots (its target's time extra data: Update,
// SetTarget, TargetIsRequiredType).
//
// Main-thread float maths at the game's 24-bit x87 precision: float operations in its order.

#include <cmath>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiSystem.h"
#include "gamebryo/NiTimeController.h"
#include "reversed.h"

namespace ni {
namespace {

using ni::scene::At;
using ni::scene::FixedStringRelease;
using ni::scene::NullFixedString;
using ni::scene::StringTable_AddString;
using ni::scene::Virtual;
using ni::sys::Import;

GAME_VAR(const char*, ms_pcTimeExtraData, 0x00BC8B20);      // the time extra data's name
GAME_VAR(uint32_t, kNiFloatExtraDataClassIndex, 0x020E2924);
GAME_VAR(uint32_t, kTargetClassIndex, 0x020E25BC);          // (the class a sync controller drives)
GAME_VAR(double, kMinusFltMax, 0x00953E90);
GAME_VAR(double, kFltMax, 0x0092CD00);
GAME_VAR(float, kMinusFltMaxF, 0x009009BC);
// the last ComputeScaledTime's inputs and result
GAME_VAR(float, ms_fLastResult, 0x020E32AC);
GAME_VAR(float, ms_fLastTime, 0x020E32B0);
GAME_VAR(float, ms_fLastLoKeyTime, 0x020E32B4);
GAME_VAR(float, ms_fLastHiKeyTime, 0x020E32B8);
GAME_VAR(uint32_t, ms_uiLastCycleType, 0x00BC8AD0);
constexpr uintptr_t ms_kScaledTimeLock = 0x020E32F0;   // a CRITICAL_SECTION

constexpr uintptr_t NiFloatExtraData_vtable = 0x008FFA8C;

GAME_FN(void, __thiscall, Controller_SetTarget, 0x0077E050, NiTimeController* self, NiObjectNET* target);
GAME_FN(NiObject*, __thiscall, ObjectNET_GetExtraData, 0x00764250, NiObjectNET* self, const re::Ptr32<const char>* name);
GAME_FN(uint8_t, __thiscall, ObjectNET_AddExtraData, 0x00764990, NiObjectNET* self, NiObject* data);
GAME_FN(void*, __thiscall, ExtraData_Construct, 0x00765640, void* self);
GAME_FN(void, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<NiObject>* self, NiObject* value);
GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<const char>* self, const re::Ptr32<const char>* other);
// (this file's, called where modes apply)
GAME_FN(uint8_t, __thiscall, Controller_DontDoUpdate, 0x0077D890, NiTimeController* self, float time);

inline uint8_t IsKindOf(void* object, uint32_t classIndex) {
    const uint32_t* rtti = Virtual<const uint32_t*>(object, 2);
    return (rtti[3 + (classIndex >> 5)] & (1u << (classIndex & 31))) != 0 ? 1 : 0;
}

// the time extra data's name (none: the empty handle)
inline const char* TimeExtraDataName() {
    return ms_pcTimeExtraData != nullptr ? StringTable_AddString(ms_pcTimeExtraData) : static_cast<const char*>(NullFixedString);
}

} // namespace

// 0x77D890: DontDoUpdate(time): true when inactive, or the time (scaled, when it computes scaled
// time) hasn't changed and no update is forced; else the forced update cleared (the scaled time
// kept)
uint8_t __fastcall NiTimeController_DontDoUpdate(NiTimeController* self, re::Edx, float time) {
    uint8_t flags = uint8_t(self->flags);
    if (((flags >> 3) & 1) == 0) {
        return 1;
    }
    if (time == self->lastTime && ((flags >> 7) & 1) == 0) {
        return 1;
    }
    if (((flags >> 6) & 1) == 0) {
        self->flags &= 0xFF7F;
        return 0;
    }
    float scaled = Virtual<float>(self, 24, time);   // ComputeScaledTime
    if (scaled == self->scaledTime && ((uint8_t(self->flags) >> 7) & 1) == 0) {
        return 1;
    }
    self->flags &= 0xFF7F;
    self->scaledTime = scaled;
    return 0;
}

// 0x77D920: NiTimeSyncController::Update(time): the time extra data set to the time
void __fastcall NiTimeSyncController_Update(NiTimeSyncController* self, re::Edx, float time) {
    if (self->target == nullptr || self->unknown34 == nullptr) {
        return;
    }
    if (!Controller_DontDoUpdate(self, time)) {
        At<float>(self->unknown34, 0xC) = time;
    }
}

// 0x77D9A0: NiTimeSyncController::SetTarget: the target's float extra data of the time name, or a
// new one added to it
void __fastcall NiTimeSyncController_SetTarget(NiTimeSyncController* self, re::Edx, NiObjectNET* target) {
    Controller_SetTarget(self, target);
    if (target == nullptr) {
        return;
    }
    re::Ptr32<const char> name = TimeExtraDataName();
    NiObject* data = ObjectNET_GetExtraData(target, &name);
    FixedStringRelease(&name);
    if (data != nullptr && IsKindOf(data, kNiFloatExtraDataClassIndex)) {
        Pointer_Assign(&self->unknown34, data);
        return;
    }
    void* created = NiNew(0x10);
    if (created != nullptr) {
        ExtraData_Construct(created);
        At<float>(created, 0xC) = 0.0f;
        *static_cast<re::Ptr32<void>*>(created) = reinterpret_cast<void*>(NiFloatExtraData_vtable);
    } else {
        created = nullptr;
    }
    Pointer_Assign(&self->unknown34, static_cast<NiObject*>(created));
    name = TimeExtraDataName();
    FixedString_Assign(&At<const char*>(self->unknown34, 8), &name);
    FixedStringRelease(&name);
    ObjectNET_AddExtraData(target, self->unknown34);
}

// 0x77DB00: NiTimeSyncController::TargetIsRequiredType
uint8_t __fastcall NiTimeSyncController_TargetIsRequiredType(NiTimeSyncController* self, re::Edx) {
    if (self->target != nullptr && IsKindOf(self->target, kTargetClassIndex)) {
        return 1;
    }
    return 0;
}

// 0x77DC50: Stop (slot 19): inactive, the last time forgotten (and the start time, for
// APP_INIT)
void __fastcall NiTimeController_Stop(NiTimeController* self, re::Edx) {
    self->flags &= 0xFFF7;
    float none = kMinusFltMaxF;
    self->lastTime = none;
    if ((self->flags & 1) != 0) {
        self->startTime = none;
    }
}

// 0x77DC70: ComputeScaledTime (slot 24). (REVERSE takes the time modulo two spans without the low
// key time subtracted, as the game has it; the result is clamped to the key times, then mirrored
// when playing backwards)
float __fastcall NiTimeController_ComputeScaledTime(NiTimeController* self, re::Edx, float time) {
    if (double(self->startTime) == kMinusFltMax) {
        self->startTime = time;
    }
    float delta;
    if (double(self->lastTime) == kMinusFltMax) {
        self->weightedLastTime = 0.0f;
        delta = (self->flags & 1) != 0 ? 0.0f : time;
    } else {
        delta = time - self->lastTime;
    }
    uint32_t cycle = (uint8_t(self->flags) >> 1) & 3;
    float weighted = self->frequency * delta + self->weightedLastTime;
    self->weightedLastTime = weighted;
    float scaled = weighted + self->phase;
    self->lastTime = time;
    Import<void>(sys::kEnterCriticalSection, reinterpret_cast<void*>(ms_kScaledTimeLock));
    if (self->hiKeyTime == ms_fLastHiKeyTime && self->loKeyTime == ms_fLastLoKeyTime && scaled == ms_fLastTime &&
        ms_uiLastCycleType == cycle) {
        scaled = ms_fLastResult;
    } else {
        ms_uiLastCycleType = cycle;
        ms_fLastHiKeyTime = self->hiKeyTime;
        ms_fLastLoKeyTime = self->loKeyTime;
        ms_fLastTime = scaled;
        float lo = self->loKeyTime;
        if (double(self->hiKeyTime) != kMinusFltMax && double(lo) != kFltMax) {
            if (cycle == 0) {   // LOOP
                float span = self->hiKeyTime - self->loKeyTime;
                if (span == 0.0f) {
                    scaled = self->loKeyTime;
                } else {
                    float offset = scaled - self->loKeyTime;
                    scaled = float(std::fmod(double(offset), double(span)));
                    scaled = scaled + self->loKeyTime;
                    if (self->loKeyTime > scaled) {
                        scaled = scaled + span;
                    }
                }
            } else if (cycle == 1) {   // REVERSE
                float span = self->hiKeyTime - self->loKeyTime;
                if (span == 0.0f) {
                    scaled = self->loKeyTime;
                } else {
                    float twoSpan = span + span;
                    float r = float(std::fmod(double(scaled), double(twoSpan)));
                    if (r < 0.0f) {
                        r = r + twoSpan;
                    }
                    if (span < r) {
                        r = twoSpan - r;
                    }
                    scaled = r + self->loKeyTime;
                }
            }
        }
        if (self->hiKeyTime < scaled) {
            scaled = self->hiKeyTime;
        } else if (self->loKeyTime > scaled) {
            scaled = self->loKeyTime;
        }
        if (((uint8_t(self->flags) >> 4) & 1) != 0) {
            scaled = self->hiKeyTime - (scaled - self->loKeyTime);
        }
        ms_fLastResult = scaled;
    }
    Import<void>(sys::kLeaveCriticalSection, reinterpret_cast<void*>(ms_kScaledTimeLock));
    return scaled;
}

#define NITIMECTRL2_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NITIMECTRL2_REVERSED("NiTimeController::DontDoUpdate", 0x0077D890, NiTimeController_DontDoUpdate);
NITIMECTRL2_REVERSED("NiTimeSyncController::Update", 0x0077D920, NiTimeSyncController_Update);
NITIMECTRL2_REVERSED("NiTimeSyncController::SetTarget", 0x0077D9A0, NiTimeSyncController_SetTarget);
NITIMECTRL2_REVERSED("NiTimeSyncController::TargetIsRequiredType", 0x0077DB00, NiTimeSyncController_TargetIsRequiredType);
NITIMECTRL2_REVERSED("NiTimeController::Stop", 0x0077DC50, NiTimeController_Stop);
NITIMECTRL2_REVERSED("NiTimeController::ComputeScaledTime", 0x0077DC70, NiTimeController_ComputeScaledTime);

} // namespace ni
