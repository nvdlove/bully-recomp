// The script's mission timer (Script::MissionTimerStart and the like): a duration in ms
// counted from a start time, pausable (the elapsed time kept while paused).

#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace {

GAME_VAR(uint32_t, ms_timeInMs, 0x00C1A9B4);   // CTimer::m_snTimeInMilliseconds

struct CMissionTimer {
    uint8_t unknown0[0xC];   // +0
    uint32_t duration;       // +0xC ms
    uint32_t start;          // +0x10 (shifted on resuming)
    uint32_t elapsed;        // +0x14 (while paused or stopped)
    uint8_t unknown18;       // +0x18
    uint8_t running;         // +0x19
    uint8_t paused;          // +0x1A
};
static_assert(offsetof(CMissionTimer, duration) == 0xC);
static_assert(offsetof(CMissionTimer, running) == 0x19);
static_assert(offsetof(CMissionTimer, paused) == 0x1A);

} // namespace

namespace MissionTimer {

// 0x43F690 (thiscall)
static void __fastcall Start(CMissionTimer* self, re::Edx, int seconds) {
    self->duration = seconds * 1000;
    self->start = ms_timeInMs;
}

// 0x43F6B0 (thiscall): paused (the elapsed time kept, at most the duration) or resumed
static void __fastcall SetPaused(CMissionTimer* self, re::Edx, int pause) {
    uint8_t flag = uint8_t(pause);
    if (self->paused == flag || !self->running) {
        return;
    }
    uint32_t now = ms_timeInMs;
    self->paused = flag;
    if (flag) {
        self->elapsed = now - self->start;
        if (self->elapsed > self->duration) {
            self->elapsed = self->duration;
        }
    } else {
        self->start = now - self->elapsed;
    }
}

// 0x43F6F0 (thiscall): the seconds left (0 once over)
static float __fastcall GetSecondsLeft(CMissionTimer* self, re::Edx) {
    float left = 0.0f;
    if (!self->paused && self->running) {
        uint32_t elapsed = ms_timeInMs - self->start;
        if (elapsed > self->duration) {
            return float(double(left) * 0.0010000000474974513);
        }
        left = float(self->duration - elapsed);   // (unsigned)
    } else {
        left = float(self->duration - self->elapsed);
    }
    return float(double(left) * 0.0010000000474974513);
}

// 0x43F750 (thiscall): running or not (the elapsed time kept when it stops)
static void __fastcall SetRunning(CMissionTimer* self, re::Edx, int running) {
    uint8_t flag = uint8_t(running);
    if (flag == 0 && self->paused == 0 && self->running != 0) {
        self->elapsed = ms_timeInMs - self->start;
    }
    self->paused = 0;
    self->running = flag;
}

// 0x43F780 (thiscall)
static void __fastcall Stop(CMissionTimer* self, re::Edx) {
    self->unknown18 = 0;
    if (self->paused == 0 && self->running != 0) {
        self->elapsed = ms_timeInMs - self->start;
    }
    self->running = 0;
    self->paused = 0;
}

} // namespace MissionTimer

// ---- registration ----------------------------------------------------------------------

#define MTIMER_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace MissionTimer {
MTIMER_REVERSED("CMissionTimer::Start", 0x0043F690, &Start);
MTIMER_REVERSED("CMissionTimer::SetPaused", 0x0043F6B0, &SetPaused);
MTIMER_REVERSED("CMissionTimer::GetSecondsLeft", 0x0043F6F0, &GetSecondsLeft);
MTIMER_REVERSED("CMissionTimer::SetRunning", 0x0043F750, &SetRunning);
MTIMER_REVERSED("CMissionTimer::Stop", 0x0043F780, &Stop);
} // namespace MissionTimer
