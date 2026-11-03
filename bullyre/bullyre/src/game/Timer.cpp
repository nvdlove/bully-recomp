// CTimer (0x45B7B0..0x45B960): the game clock from the performance counter, its pauses
// and suspends, and the time step.

#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace {

GAME_VAR(uint32_t, m_snTimeInMilliseconds, 0x00C1A9B4);
GAME_VAR(uint32_t, m_snPreviousTimeInMilliseconds, 0x00C1A9AC);
GAME_VAR(float, ms_fTimeStep, 0x00C1A9A4);
GAME_VAR(float, ms_fOldTimeStep, 0x00C1A9A0);   // (the step before its 3.0 cap)
GAME_VAR(float, ms_fTimeScale, 0x00C1A9A8);
GAME_VAR(float, ms_fTimeStepNonClipped, 0x00C1A9B0);   // (the ms added up, as a float)
GAME_VAR(float, ms_fStepOf1, 0x00A14A54);       // 0.02: the ms of a step of 1, over 1000
GAME_VAR(uint32_t, m_FrameCounter, 0x00C1A99C);
GAME_VAR(uint8_t, m_UserPause, 0x00C1A99A);
GAME_VAR(uint8_t, m_CodePause, 0x00C1A999);
GAME_VAR(uint8_t, m_OtherPause, 0x00C1A998);
GAME_VAR(uint32_t, suspendDepth, 0x00C1A9B8);
GAME_VAR(uint8_t, suspended, 0x00C1A9E8);
GAME_VAR(uint8_t, capTimeStep, 0x00A14A4C);
GAME_VAR(int64_t, oldCounter, 0x00C1A9C8);
GAME_VAR(int64_t, suspendCounter, 0x00C1A9D8);
GAME_VAR(int64_t, counterNow, 0x00C1A9D0);
GAME_VAR(int64_t, counterDelta, 0x00C1A9C0);
GAME_VAR(int64_t, frequency, 0x00C1A9E0);
GAME_VAR(int32_t, realMs, 0x00A14A50);          // (unscaled ms, added up)
GAME_VAR(uint8_t, pauseCountdownOn, 0x00C1A9E9);
GAME_VAR(int32_t, pauseCountdown, 0x00C1AA00);
// the play clock
GAME_VAR(int32_t, clockMs, 0x00C1A9FC);
GAME_VAR(int32_t, clockSeconds, 0x00C1A9F8);
GAME_VAR(int32_t, clockMinutes, 0x00C1A9F4);
GAME_VAR(int32_t, clockHours, 0x00C1A9F0);

GAME_FN(void, __cdecl, Fn738780, 0x00738780);

// the performance counter through the game's import slots
int QueryCounter(int64_t* out) {
    return re::PtrCast<int(__stdcall*)(int64_t*)>(*reinterpret_cast<re::Ptr32<void>*>(0x008FC040))(out);
}
int QueryFrequency(int64_t* out) {
    return re::PtrCast<int(__stdcall*)(int64_t*)>(*reinterpret_cast<re::Ptr32<void>*>(0x008FC044))(out);
}

// 0x45B7B0 (cdecl)
void __cdecl Timer_Suspend() {
    if (++suspendDepth <= 1) {
        suspended = 1;
    }
}

// 0x45B7D0 (cdecl)
void __cdecl Timer_Resume() {
    if (--suspendDepth == 0) {
        suspended = 0;
    }
}

// 0x45B7F0 (cdecl)
int64_t __cdecl Timer_GetFrequency() {
    return frequency;
}

// 0x45B800 (cdecl)
void __cdecl Timer_UpdatePrevious() {
    m_snPreviousTimeInMilliseconds = m_snTimeInMilliseconds;
}

// 0x45B810 (cdecl)
void __cdecl Timer_StartUserPause() {
    m_UserPause = 1;
}

// 0x45B820 (cdecl)
void __cdecl Timer_EndUserPause() {
    m_UserPause = 0;
}

// 0x45B830 (cdecl): ms since the last frame
int32_t __cdecl Timer_GetTimeStepInMilliseconds() {
    return int32_t(m_snTimeInMilliseconds - m_snPreviousTimeInMilliseconds);
}

// 0x45B840 (cdecl): seconds since the last frame
float __cdecl Timer_GetTimeStepInSeconds() {
    // (the difference taken as unsigned, as the game does: 2^32 added when it's negative)
    return float(double(uint32_t(m_snTimeInMilliseconds - m_snPreviousTimeInMilliseconds)) / 1000.0);
}

// 0x45B870 (thiscall): value in [start, end], the range wrapping when end < start
int32_t __fastcall Range_Contains(const int32_t* range, re::Edx, int32_t value) {
    int32_t start = range[0], end = range[1];
    if (start < end) {
        return (value >= start && value <= end) ? 1 : 0;
    }
    if (value >= start) {
        return 1;
    }
    return value > end ? 0 : 1;
}

// 0x45B8A0 (cdecl)
void __cdecl Timer_Initialise() {
    ms_fTimeStepNonClipped = 0.0f;
    ms_fTimeScale = 1.0f;
    ms_fTimeStep = 1.0f;
    m_UserPause = 0;
    m_CodePause = 0;
    m_OtherPause = 0;
    m_snPreviousTimeInMilliseconds = 0;
    suspendDepth = 0;
    capTimeStep = 1;
    m_snTimeInMilliseconds = 0;
    int64_t counter;
    if (QueryCounter(&counter)) {
        oldCounter = counter;
        suspendCounter = counter;
    }
    int64_t freq;
    if (QueryFrequency(&freq)) {
        frequency = freq;
    }
    counterDelta = 0;
    counterNow = 0;
    m_FrameCounter = 0;
}

// 0x45B960 (cdecl): once a frame. Patched in place by bullygfx (0x45BB88, the time
// step's 0.5 floor): written here, not hooked
[[maybe_unused]] void __cdecl Timer_Update(int keepPaused) {
    if (pauseCountdownOn) {
        if (pauseCountdown > 0) {
            pauseCountdown--;
            m_CodePause = 0;
        } else {
            m_CodePause = 1;
            pauseCountdown = 0;
        }
    }
    if (suspendDepth) {
        return;
    }
    m_snPreviousTimeInMilliseconds = m_snTimeInMilliseconds;
    if (!suspended) {
        int64_t counter;
        if (QueryCounter(&counter)) {
            counterNow = counter;
        }
    }
    counterDelta = counterNow - oldCounter;
    oldCounter = counterNow;
    // (the frequency's low dword only)
    uint32_t perMs = uint32_t(frequency) / 1000;
    realMs += int32_t(counterDelta / int64_t(perMs));
    if ((m_UserPause || m_CodePause || m_OtherPause) && !uint8_t(keepPaused)) {
        counterDelta = 0;
        Fn738780();
    }
    double scaled = double(counterDelta) * ms_fTimeScale;
    float perMsF = float(perMs);
    float ms = float(scaled / perMsF);
    m_snTimeInMilliseconds += uint32_t(int64_t(ms));
    ms_fTimeStepNonClipped = ms_fTimeStepNonClipped + ms;
    ms_fTimeStep = float(scaled / (perMsF * (ms_fStepOf1 * 1000.0)));
    int32_t total = int32_t(ms + double(clockMs));
    clockSeconds += total / 1000;
    clockMs = total % 1000;
    clockMinutes += clockSeconds / 60;
    clockSeconds %= 60;
    clockHours += clockMinutes / 60;
    clockMinutes %= 60;
    float step = ms_fTimeStep;
    if (step < 0.5f && !m_UserPause && !m_CodePause && !m_OtherPause && ms_fTimeScale >= 1.0f) {
        ms_fTimeStep = 0.5f;
        step = ms_fTimeStep;
    }
    ms_fOldTimeStep = step;
    if (!capTimeStep) {
        m_FrameCounter++;
        return;
    }
    step = step < 3.0f ? step : 3.0f;
    ms_fTimeStep = step;
    // no more than 60 ms a frame (through floats: exact only while the times fit 24 bits)
    float now = float(double(m_snTimeInMilliseconds));
    float limit = float(double(m_snPreviousTimeInMilliseconds + 60));
    float time = limit <= now ? limit : now;
    m_FrameCounter++;
    m_snTimeInMilliseconds = uint32_t(int64_t(time));
}
RE_UNHOOKED_FN("CTimer::Update", 0x0045B960, &Timer_Update);


// 0x45BCA0 (cdecl): ms since Initialise, by the counter (1 when it can't be read)
uint32_t __cdecl Timer_GetCurrentTimeInMs() {
    int64_t counter;
    if (!QueryCounter(&counter)) {
        return 1;
    }
    int64_t elapsed = counter - suspendCounter;
    return uint32_t(int64_t(double(elapsed) / (double(frequency) / 1000.0)));
}

// ---- countdowns: {running, start, duration} -------------------------------------------------

struct Countdown {
    uint8_t running;     // +0
    uint8_t unknown1[3];
    uint32_t start;      // +4 ms
    uint32_t duration;   // +8 ms
};
static_assert(sizeof(Countdown) == 0xC);

inline int __cdecl Rand() { return crt::Rand(); }

// 0x45BD10 (thiscall)
void __fastcall Countdown_Clear(Countdown* self) {
    self->running = 0;
    self->start = 0;
    self->duration = 0;
}

// 0x45BD20 (thiscall): the constructor
Countdown* __fastcall Countdown_Construct(Countdown* self) {
    self->running = 0;
    self->start = 0;
    self->duration = 0;
    return self;
}

// 0x45BD30 (thiscall)
void __fastcall Countdown_Start(Countdown* self, re::Edx, uint32_t duration) {
    self->running = 1;
    self->start = m_snTimeInMilliseconds;
    self->duration = duration;
}

// 0x45BD50 (thiscall): unless running
void __fastcall Countdown_StartIfStopped(Countdown* self, re::Edx, uint32_t duration) {
    if (self->running) {
        return;
    }
    self->running = 1;
    self->start = m_snTimeInMilliseconds;
    self->duration = duration;
}

// 0x45BD70 (thiscall): in seconds
void __fastcall Countdown_StartSeconds(Countdown* self, re::Edx, float seconds) {
    self->running = 1;
    self->start = m_snTimeInMilliseconds;
    self->duration = uint32_t(int64_t(seconds * 1000.0));
}

// 0x45BDB0 (thiscall): running and not done
uint8_t __fastcall Countdown_IsRunning(const Countdown* self) {
    if (!self->running) {
        return 0;
    }
    return m_snTimeInMilliseconds < self->duration + self->start;
}

// 0x45BDD0 (thiscall): running and done
uint8_t __fastcall Countdown_IsDone(const Countdown* self) {
    if (!self->running) {
        return 0;
    }
    return m_snTimeInMilliseconds >= self->duration + self->start;
}

// 0x45BDF0 (thiscall): a random duration between the two, in seconds
void __fastcall Countdown_StartRandom(Countdown* self, re::Edx, float minSeconds, float maxSeconds) {
    int r = Rand();
    self->running = 1;
    self->start = m_snTimeInMilliseconds;
    float seconds = float(minSeconds + (double(maxSeconds) - minSeconds) * (double(r) / 32767.0));
    self->duration = uint32_t(int64_t(seconds * 1000.0));
}

#define C04E_TIMER(name, address, fn)                                                          \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04E_TIMER("CTimer::Suspend", 0x0045B7B0, &Timer_Suspend);
C04E_TIMER("CTimer::Resume", 0x0045B7D0, &Timer_Resume);
C04E_TIMER("CTimer::GetFrequency", 0x0045B7F0, &Timer_GetFrequency);
C04E_TIMER("CTimer::UpdatePrevious", 0x0045B800, &Timer_UpdatePrevious);
C04E_TIMER("CTimer::StartUserPause", 0x0045B810, &Timer_StartUserPause);
C04E_TIMER("CTimer::EndUserPause", 0x0045B820, &Timer_EndUserPause);
C04E_TIMER("CTimer::GetTimeStepInMilliseconds", 0x0045B830, &Timer_GetTimeStepInMilliseconds);
C04E_TIMER("CTimer::GetTimeStepInSeconds", 0x0045B840, &Timer_GetTimeStepInSeconds);
C04E_TIMER("TimeRange::Contains", 0x0045B870, &Range_Contains);
C04E_TIMER("CTimer::Initialise", 0x0045B8A0, &Timer_Initialise);
C04E_TIMER("CTimer::GetCurrentTimeInMs", 0x0045BCA0, &Timer_GetCurrentTimeInMs);
C04E_TIMER("Countdown::Clear", 0x0045BD10, &Countdown_Clear);
C04E_TIMER("Countdown::Construct", 0x0045BD20, &Countdown_Construct);
C04E_TIMER("Countdown::Start", 0x0045BD30, &Countdown_Start);
C04E_TIMER("Countdown::StartIfStopped", 0x0045BD50, &Countdown_StartIfStopped);
C04E_TIMER("Countdown::StartSeconds", 0x0045BD70, &Countdown_StartSeconds);
C04E_TIMER("Countdown::IsRunning", 0x0045BDB0, &Countdown_IsRunning);
C04E_TIMER("Countdown::IsDone", 0x0045BDD0, &Countdown_IsDone);
C04E_TIMER("Countdown::StartRandom", 0x0045BDF0, &Countdown_StartRandom);

} // namespace
