// CClock: the game's time of day (GTA's CClock): minutes advance every msPerMinute
// (0x9E51A4, from 0x9E51A8, or 0x9E51AC while the mission manager says so), paused in
// cutscenes, menus, some missions. 0x425D60..0x4260D0.

#include <cstdint>

#include "reversed.h"

#define CLOCK_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace {
GAME_VAR(uint8_t, ms_nGameClockMinutes, 0x00BCEB4A);
GAME_VAR(uint8_t, ms_nGameClockHours, 0x00BCEB4B);
GAME_VAR(int32_t, ms_nGameClockDays, 0x00BCEB4C);
GAME_VAR(uint16_t, ms_nGameClockSeconds, 0x00BCEB48);
GAME_VAR(uint32_t, ms_nLastClockTick, 0x00BCEB44);
GAME_VAR(uint32_t, ms_nMillisecondsPerGameMinute, 0x009E51A4);
GAME_VAR(uint32_t, ms_nMsPerMinuteNormal, 0x009E51A8);
GAME_VAR(uint32_t, ms_nMsPerMinuteOther, 0x009E51AC);
GAME_VAR(uint8_t, ms_bBigJump, 0x00BCEB3C);          // the last step was more than 5 minutes
GAME_VAR(uint8_t, ms_bStopped, 0x00BCEB3F);          // stopped by the script
GAME_VAR(uint8_t, ms_bPaused, 0x00BCEB42);
GAME_VAR(uint8_t, ms_bStored, 0x00BCEB43);
GAME_VAR(uint8_t, ms_Stored_nGameClockMinutes, 0x00BCEB3D);
GAME_VAR(uint8_t, ms_Stored_nGameClockHours, 0x00BCEB3E);
GAME_VAR(uint16_t, ms_Stored_nGameClockSeconds, 0x00BCEB40);
GAME_VAR(int32_t, ms_nPauseMission, 0x009E51A0);     // a mission whose flags' +2 0 pauses the clock (-1: none)
GAME_VAR(uint32_t, ck_timeMs, 0x00C1A9B4);
GAME_VAR(uint8_t, ck_inCutscene, 0x00B9DB1C);
GAME_VAR(int32_t, ck_widescreen, 0x00C3CD0C);
GAME_VAR(uint8_t, ck_cutsceneRunning, 0x020C5BE4);
inline void* const ck_missionMgr = reinterpret_cast<void*>(0x020C3CA0);
inline void* const ck_statsMgr = reinterpret_cast<void*>(0x020C35E8);
inline uint8_t* const ck_objC18748 = reinterpret_cast<uint8_t*>(0x00C18748);

GAME_FN(void, __cdecl, CK_TimeChanged, 0x00513030);
GAME_FN(uint8_t*, __thiscall, CK_MissionMgr_GetTaskFlags, 0x006AA7C0, void* mgr, int32_t mission);
GAME_FN(uint8_t, __cdecl, CK_Fn513FE0, 0x00513FE0);
GAME_FN(uint8_t, __thiscall, CK_IsStateBusy, 0x00425D60, uint8_t* obj);
GAME_FN(uint8_t, __thiscall, CK_MissionMgr_Fn6AA7E0, 0x006AA7E0, void* mgr);
GAME_FN(int32_t*, __thiscall, CK_MissionMgr_Current, 0x006AA790, void* mgr);
GAME_FN(uint8_t*, __thiscall, CK_MissionMgr_Get, 0x006AA660, void* mgr, int32_t index);
GAME_FN(int32_t, __thiscall, CK_Stats_Find, 0x006A8C30, void* mgr, const char* name);
GAME_FN(uint8_t, __thiscall, CK_Stats_Test, 0x006A8BB0, void* mgr, int32_t stat, int a, int b);
GAME_FN(void, __cdecl, CK_AddMinutes, 0x00425D80, uint32_t minutes);
GAME_FN(void, __cdecl, CK_UpdatePaused, 0x00425EB0);
} // namespace

namespace CClock {

// 0x425D60 (thiscall): the state at 0xC18748 busy (+0xC set, and +0 1 or 3)
uint32_t __fastcall IsStateBusy(uint8_t* obj, re::Edx) {
    if (obj[0xC] != 0) {
        int32_t state = *reinterpret_cast<int32_t*>(obj);
        if (state == 1 || state == 3) {
            return 1;
        }
    }
    return 0;
}

// 0x425D80: the clock moved on by minutes (days counted)
void __cdecl AddMinutes(uint32_t minutes) {
    int32_t m = int32_t(ms_nGameClockMinutes + minutes);
    ms_bBigJump = minutes > 5;
    ms_nGameClockMinutes = uint8_t(m);
    if (m >= 60) {
        ms_nGameClockMinutes = uint8_t(m % 60);
        int32_t h = (m - m % 60) / 60 + ms_nGameClockHours;
        ms_nGameClockHours = uint8_t(h);
        if (h >= 24) {
            ms_nGameClockHours = uint8_t(h % 24);
            ms_nGameClockDays += (h - h % 24) / 24;
        }
    }
    CK_TimeChanged();
}

// 0x425E40: the time between h1:m1 and h2:m2 (over midnight when the start is later)
uint8_t __cdecl IsTimeInRange(int h1, int h2, int m1, int m2) {
    int now = ms_nGameClockMinutes + ms_nGameClockHours * 60;
    int start = m1 + h1 * 60;
    int end = m2 + h2 * 60;
    if (start > end) {
        return now >= start || now < end;
    }
    return !(now < start || now >= end);
}

// 0x425EA0
void __cdecl SetStopped(uint8_t stopped) {
    ms_bStopped = stopped;
}

// 0x425EB0: whether the clock is paused: cutscenes, widescreen, a paused mission, the
// script's stop, the menus; a mission playing (0x2E set, or kind 5 while the player
// is "PER_TIRED")
void __cdecl UpdatePaused() {
    uint8_t paused = 0;
    if (ck_inCutscene || ck_widescreen != 0 || ck_cutsceneRunning) {
        paused = 1;
    } else if (ms_nPauseMission != -1 && *reinterpret_cast<int16_t*>(CK_MissionMgr_GetTaskFlags(ck_missionMgr, ms_nPauseMission) + 2) == 0) {
        paused = 1;
    } else if (ms_bStopped || CK_Fn513FE0() || CK_IsStateBusy(ck_objC18748)) {
        paused = 1;
    } else if (CK_MissionMgr_Fn6AA7E0(ck_missionMgr)) {
        int32_t current = *CK_MissionMgr_Current(ck_missionMgr);
        if (current >= 0) {
            uint8_t* mission = CK_MissionMgr_Get(ck_missionMgr, current);
            if (mission[0x2E] != 0) {
                paused = 1;
            } else if (*reinterpret_cast<int32_t*>(mission + 0x28) == 5) {
                int32_t stat = CK_Stats_Find(ck_statsMgr, "PER_TIRED");
                if (CK_Stats_Test(ck_statsMgr, stat, 0, -1)) {
                    paused = 1;
                }
            }
        }
    }
    ms_bPaused = paused;
}

// 0x425F80: the time kept (GTA's StoreClock)
void __cdecl StoreClock() {
    ms_Stored_nGameClockHours = ms_nGameClockHours;
    ms_Stored_nGameClockMinutes = ms_nGameClockMinutes;
    ms_Stored_nGameClockSeconds = ms_nGameClockSeconds;
    ms_bStored = 1;
}

// 0x425FB0: the kept time back (GTA's RestoreClock)
void __cdecl RestoreClock() {
    ms_nGameClockHours = ms_Stored_nGameClockHours;
    ms_nGameClockMinutes = ms_Stored_nGameClockMinutes;
    ms_nGameClockSeconds = ms_Stored_nGameClockSeconds;
    CK_TimeChanged();
}

// 0x425FE0: 6:00..18:59 (all of eax 0 or 1)
uint32_t __cdecl IsDayTime() {
    return uint8_t(ms_nGameClockHours - 6) <= 12 ? 1 : 0;
}

// 0x426000
uint8_t __cdecl IsNightTime() {
    return uint8_t(ms_nGameClockHours - 6) <= 12 ? 0 : 1;
}

// 0x426020: noon, running (GTA's Initialise)
void __cdecl Initialise(uint32_t msPerMinute) {
    ms_nMillisecondsPerGameMinute = msPerMinute;
    ms_nLastClockTick = ck_timeMs;
    ms_nGameClockHours = 12;
    ms_nGameClockMinutes = 0;
    ms_nGameClockSeconds = 0;
    ms_bStored = 0;
    ms_bPaused = 0;
    ms_bStopped = 0;
    ms_nPauseMission = -1;
    ms_bBigJump = 0;
}

// 0x426070: the clock set forward to h:m (never back: past midnight instead)
void __cdecl SetGameClock(uint8_t h, uint8_t m) {
    ms_nLastClockTick = ck_timeMs;
    uint32_t now = ms_nGameClockMinutes + ms_nGameClockHours * 60u;
    uint32_t then = m + h * 60u;
    ms_nGameClockSeconds = 0;
    if (then < now) {
        then += 1440;
    }
    CK_AddMinutes(then - now);
}

// 0x4260D0: the clock's frame (GTA's Update); the seconds from the time since the
// last minute
void __cdecl Update() {
    CK_UpdatePaused();
    if (ms_bPaused) {
        ms_nLastClockTick = ck_timeMs;
        return;
    }
    uint32_t msPerMinute = ms_nMsPerMinuteNormal;
    if (!CK_MissionMgr_Fn6AA7E0(ck_missionMgr)) {
        msPerMinute = ms_nMsPerMinuteOther;
    }
    uint32_t now = ck_timeMs;
    uint32_t last = ms_nLastClockTick;
    ms_nMillisecondsPerGameMinute = msPerMinute;
    if (now - last > msPerMinute) {
        uint32_t minutes = (now - last) / msPerMinute;
        last = now;
        ms_nLastClockTick = last;
        uint8_t m = uint8_t(ms_nGameClockMinutes + minutes);
        ms_nGameClockMinutes = m;
        ms_bBigJump = int32_t(minutes) > 5;
        if (m >= 60) {
            uint8_t h = uint8_t(ms_nGameClockHours + 1);
            ms_nGameClockMinutes = 0;
            ms_nGameClockHours = h;
            if (h >= 24) {
                ms_nGameClockDays++;
                ms_nGameClockHours = 0;
            }
        }
    }
    ms_nGameClockSeconds = uint16_t(((now - last) * 60 / msPerMinute) % 60);
}

CLOCK_REVERSED("CClock::IsStateBusy", 0x00425D60, &IsStateBusy);
CLOCK_REVERSED("CClock::AddMinutes", 0x00425D80, &AddMinutes);
CLOCK_REVERSED("CClock::IsTimeInRange", 0x00425E40, &IsTimeInRange);
CLOCK_REVERSED("CClock::SetStopped", 0x00425EA0, &SetStopped);
CLOCK_REVERSED("CClock::UpdatePaused", 0x00425EB0, &UpdatePaused);
CLOCK_REVERSED("CClock::StoreClock", 0x00425F80, &StoreClock);
CLOCK_REVERSED("CClock::RestoreClock", 0x00425FB0, &RestoreClock);
CLOCK_REVERSED("CClock::IsDayTime", 0x00425FE0, &IsDayTime);
CLOCK_REVERSED("CClock::IsNightTime", 0x00426000, &IsNightTime);
CLOCK_REVERSED("CClock::Initialise", 0x00426020, &Initialise);
CLOCK_REVERSED("CClock::SetGameClock", 0x00426070, &SetGameClock);
CLOCK_REVERSED("CClock::Update", 0x004260D0, &Update);

} // namespace CClock
