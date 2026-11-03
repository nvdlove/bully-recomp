// The day's periods (0x20C35E8: the school day's parts, from the config; each with a
// start and end time and the script functions run when it starts and ends) and the game
// completion: its ten conditions (chapters, races, errands, jobs, minigames, classes,
// mini-objectives, collectibles, arcade high scores, clothing), each worth points, and the
// percentage. 0x6A8AA0..0x6A99CA.
//
// A period (from the config): +0x18 its name, +0x1C / +0x20 the functions called when it
// starts / ends, +0x24 / +0x28 start hour / minute, +0x2C / +0x30 end, +0x34 its kind.
// The manager: +0 [20] periods, +0x50 the current one, +0x54 their count, +0x58 the day.
// A completion condition: +0 vtable {deleting destructor, Points, MaxPoints}, +4 its
// maximum, +8.. its weights.

#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace gcomp {

namespace {

using re::At;

void* const kMissionMgr = reinterpret_cast<void*>(0x020C3CA0);
void* const kPlayerInfo = reinterpret_cast<void*>(0x00C1AEA8);
void* const kLurStore = reinterpret_cast<void*>(0x00D02850);
void* const kCollectibles = reinterpret_cast<void*>(0x00BCEB50);
void* const kTheText = reinterpret_cast<void*>(0x020C2FA4);
GAME_VAR(uint8_t, g_clockHours, 0x00BCEB4B);
GAME_VAR(uint8_t, g_clockMinutes, 0x00BCEB4A);
GAME_VAR(uint8_t, g_clockSeconds, 0x00BCEB48);
GAME_VAR(uint8_t*, g_schoolConfig, 0x020C88AC);  // (+0x18 days in a week, +0x1C the periods' script)
GAME_VAR(void*, g_periodListener, 0x00C2C108);
GAME_ARRAY(uint32_t, g_periodKeys, 0x01B653AC);  // the config's path ([2]) and the entries' key
GAME_VAR(uint32_t, g_hashRacesA, 0x020C3670);
GAME_VAR(uint32_t, g_hashRacesB, 0x020C3674);
GAME_VAR(uint32_t, g_hashJobsA, 0x020C3678);
GAME_VAR(uint32_t, g_hashJobsB, 0x020C367C);
GAME_ARRAY(uint32_t, g_hashMinigames, 0x020C3680);  // [5]
GAME_ARRAY(int32_t, g_grades, 0x00C1B02C);          // [10]
GAME_VAR(int32_t, g_clothesOwned, 0x020C6B70);
GAME_VAR(int32_t, g_clothesStart, 0x020C7BA4);
GAME_VAR(int32_t, g_clothesTotal, 0x020C7BA8);
GAME_VAR(int32_t, g_clothesBase, 0x020C7BAC);
GAME_ARRAY(void*, g_conditions, 0x020C3644);  // [10]
GAME_VAR(uint8_t, g_conditionsMade, 0x020C366C);

GAME_FN(void, __thiscall, GC_NameKeyCopy, 0x005FA880, uint32_t* key, const uint32_t* from);
GAME_FN(void, __thiscall, GC_NameKeyDestruct, 0x0049A650, uint32_t* key);
GAME_FN(void, __thiscall, GC_PathBuild, 0x005FA8C0, void* path, int count, const uint32_t* names);
GAME_FN(int32_t, __cdecl, GC_ConfigCount, 0x0072B7D0, const void* path);
GAME_FN(uint8_t*, __cdecl, GC_ConfigGet, 0x0072B810, const void* path, int index, const uint32_t* key);
GAME_FN(void, __cdecl, GC_AddLoadCallback, 0x0072B870, void* fn, void* object);
GAME_FN(void, __cdecl, GC_RemoveLoadCallback, 0x0072B8B0, void* fn, void* object);
GAME_FN(void, __thiscall, GC_PeriodsLoad, 0x006A8AA0, void* self);
GAME_FN(uint8_t, __thiscall, GC_InRange, 0x006A8B50, void* self, int from, int to);
GAME_FN(void, __thiscall, GC_PeriodsAdvance, 0x006A8E50, void* self);
GAME_FN(uint8_t*, __thiscall, GC_LurLaunch, 0x005DC0F0, void* store, const char* name, int a);
GAME_FN(void, __thiscall, GC_LurRelease, 0x005DC0B0, void* store, uint8_t* script);
GAME_FN(int32_t, __thiscall, GC_ScriptCall, 0x005D8200, uint8_t* script, const char* name, int args, int results);
GAME_FN(void, __thiscall, GC_ListenerNotify, 0x0049E7C0, void* listener);
GAME_FN(void*, __cdecl, GC_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, GC_Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, GC_Delete, 0x005B0900, void* object);
GAME_FN(void*, __cdecl, GC_CreateCondition, 0x006A8FD0, int index);
GAME_FN(int32_t, __thiscall, GC_MissionCount, 0x006AA140, void* mgr);
GAME_FN(uint8_t, __thiscall, GC_MissionIsKind, 0x006AB290, void* mgr, int32_t mission, uint32_t kind);
GAME_FN(uint8_t*, __thiscall, GC_MissionInfo, 0x006AA660, void* mgr, int32_t mission);
GAME_FN(uint8_t*, __thiscall, GC_MissionFlags, 0x006AA7C0, void* mgr, int32_t mission);
GAME_FN(int32_t, __thiscall, GC_ErrandCompletion, 0x004467E0, void* info, int errand);
GAME_FN(uint8_t, __thiscall, GC_MiniObjectiveDone, 0x00446860, void* info, int id);
GAME_FN(int16_t, __thiscall, GC_Collected, 0x00426440, void* collectibles, int kind);
GAME_FN(int16_t, __thiscall, GC_Collectable, 0x00426430, void* collectibles, int kind);
GAME_FN(uint8_t, __cdecl, GC_HighScoreBy, 0x007055E0, int game, const uint16_t* initials);
GAME_FN(uint32_t, __cdecl, GC_HashName, 0x00576ED0, const char* name);
template <typename... A>
inline int __cdecl GC_Sprintf(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
GAME_FN(const uint16_t*, __thiscall, GC_TextGet, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __cdecl, GC_FontFormat, 0x00688ED0, uint16_t* out, const uint16_t* text, int size);

inline int Ftol(float value) {
    return static_cast<int>(value);
}

// points: the fraction done of the weight (the game's float order)
int Points(int done, int total, float weight) {
    float fraction = float(done) / float(total);
    return Ftol(fraction * weight);
}

uint8_t* Period(void* self, int index) {
    return At<uint8_t*>(self, index * 4);
}

// a clock time in seconds from hours and minutes
int Seconds(int hours, int minutes) {
    return (hours * 60 + minutes) * 60;
}

int NowSeconds() {
    return (g_clockHours * 60 + g_clockMinutes) * 60 + g_clockSeconds;
}

bool MissionDone(int32_t mission) {
    return At<uint16_t>(GC_MissionFlags(kMissionMgr, mission), 2) > 0;
}

} // namespace

// ---- the periods -------------------------------------------------------------------------------

// 0x6A8AA0: the periods read from the config, reloaded with it
void __fastcall Periods_Load(uint8_t* self) {
    uint32_t keys[2];
    GC_NameKeyCopy(&keys[0], &g_periodKeys[0]);
    GC_NameKeyCopy(&keys[1], &g_periodKeys[1]);
    uint32_t path[2];  // (not destroyed, as in the game)
    GC_PathBuild(path, 2, keys);
    At<int32_t>(self, 0x54) = GC_ConfigCount(path);
    for (int i = 0; i < At<int32_t>(self, 0x54); ++i) {
        At<uint8_t*>(self, i * 4) = GC_ConfigGet(path, i, &g_periodKeys[2]);
    }
    GC_AddLoadCallback(reinterpret_cast<void*>(0x006A8E20), self);
    GC_NameKeyDestruct(&keys[1]);
    GC_NameKeyDestruct(&keys[0]);
}

// 0x6A8B40: back to the first period of day 0
void __fastcall Periods_Reset(uint8_t* self) {
    At<int32_t>(self, 0x50) = 0;
    At<int32_t>(self, 0x58) = 0;
}

// 0x6A8B50: now (in seconds) from `from` to `to` (wrapping past midnight)
uint8_t __fastcall Periods_NowBetween(void*, re::Edx, int from, int to) {
    int now = NowSeconds();
    if (from > to) {
        return now >= from || now < to;
    }
    return now >= from && now < to;
}

// 0x6A8BB0: now within a period started `before` minutes early (and `after` minutes
// long, -1: to its end)
uint8_t __fastcall Periods_IsNow(uint8_t* self, re::Edx, int index, int before, int after) {
    if (index < 0 || index >= At<int32_t>(self, 0x54)) {
        return 0;
    }
    uint8_t* period = Period(self, index);
    int start = At<int32_t>(period, 0x24) * 60;
    int from = (start - before + At<int32_t>(period, 0x28)) * 60;
    int to;
    if (after == -1) {
        to = At<int32_t>(period, 0x2C) * 60 + At<int32_t>(period, 0x30);
    } else {
        to = start + At<int32_t>(period, 0x28) + after;
    }
    return GC_InRange(self, from, to * 60);
}

// 0x6A8C30: a period by name (-1: none)
int32_t __fastcall Periods_Find(uint8_t* self, re::Edx, const char* name) {
    for (int i = 0; i < At<int32_t>(self, 0x54); ++i) {
        const char* a = name;
        const char* b = At<const char*>(Period(self, i), 0x18);
        while (*a == *b && *a != 0) {
            ++a;
            ++b;
        }
        if (*a == *b) {
            return i;
        }
    }
    return -1;
}

// 0x6A8C90: the day (or the day of the week, when `weekly`)
uint8_t __fastcall Periods_IsDay(uint8_t* self, re::Edx, int day, int weekly) {
    if (uint8_t(weekly) == 0) {
        return day == At<int32_t>(self, 0x58);
    }
    return At<int32_t>(self, 0x58) % At<int32_t>(g_schoolConfig, 0x18) == day;
}

// 0x6A8CC0: a period's kind (out of range: the current one's)
int32_t __fastcall Periods_Kind(uint8_t* self, re::Edx, int index) {
    if (index >= 0 && index < At<int32_t>(self, 0x54)) {
        return At<int32_t>(Period(self, index), 0x34);
    }
    return At<int32_t>(Period(self, At<int32_t>(self, 0x50)), 0x34);
}

// 0x6A8CF0: the next period (from the current one) of kind 1 or 3 (-1: none)
int32_t __fastcall Periods_NextClass(uint8_t* self) {
    for (int i = At<int32_t>(self, 0x50); i < At<int32_t>(self, 0x54); ++i) {
        int32_t kind = At<int32_t>(Period(self, i), 0x34);
        if (kind == 1 || kind == 3) {
            return i;
        }
    }
    return -1;
}

// 0x6A8D20: the current period's kind is 3
uint8_t __fastcall Periods_CurrentIsKind3(uint8_t* self) {
    return At<int32_t>(Period(self, At<int32_t>(self, 0x50)), 0x34) == 3;
}

// 0x6A8D30: a period's length in minutes (over midnight: + 24h)
int32_t __fastcall Periods_Length(uint8_t* self, re::Edx, int index) {
    uint8_t* period = Period(self, index);
    int start = At<int32_t>(period, 0x24) * 60 + At<int32_t>(period, 0x28);
    int end = At<int32_t>(period, 0x2C) * 60 + At<int32_t>(period, 0x30);
    if (start > end) {
        return end - start + 0x5A0;
    }
    return end - start;
}

// 0x6A8D70: minutes left in a period
int32_t __fastcall Periods_MinutesLeft(uint8_t* self, re::Edx, int index) {
    uint8_t* period = Period(self, index);
    int end = At<int32_t>(period, 0x2C) * 60 + At<int32_t>(period, 0x30);
    int now = g_clockHours * 60 + g_clockMinutes;
    if (now > end) {
        return end - now + 0x5A0;
    }
    return end - now;
}

// 0x6A8DC0: a period's start (hours, minutes)
void __fastcall Periods_GetStart(uint8_t* self, re::Edx, int index, int32_t* hours, int32_t* minutes) {
    *hours = At<int32_t>(Period(self, index), 0x24);
    *minutes = At<int32_t>(Period(self, index), 0x28);
}

// 0x6A8DF0: and its end
void __fastcall Periods_GetEnd(uint8_t* self, re::Edx, int index, int32_t* hours, int32_t* minutes) {
    *hours = At<int32_t>(Period(self, index), 0x2C);
    *minutes = At<int32_t>(Period(self, index), 0x30);
}

// 0x6A8E20: the config reloaded: the callback off and the periods read again
void __cdecl Periods_OnReload(uint8_t* self) {
    GC_RemoveLoadCallback(reinterpret_cast<void*>(0x006A8E20), self);
    GC_PeriodsLoad(self);
}

// 0x6A8E40: the reload callback removed
void __fastcall Periods_Shutdown(uint8_t* self) {
    GC_RemoveLoadCallback(reinterpret_cast<void*>(0x006A8E20), self);
}

// 0x6A8E50: the period that's now: the old one's end function, the new one's start
// function (in the periods' script, launched for it)
void __fastcall Periods_Advance(uint8_t* self) {
    const char* scriptName = At<const char*>(g_schoolConfig, 0x1C);
    const char* ending = At<const char*>(Period(self, At<int32_t>(self, 0x50)), 0x20);
    uint8_t* script = GC_LurLaunch(kLurStore, scriptName, 1);
    if (ending != nullptr) {
        GC_ScriptCall(script, ending, 0, 0);
        GC_LurRelease(kLurStore, script);
    }
    int now = NowSeconds();
    for (int i = 0; i < At<int32_t>(self, 0x54); ++i) {
        uint8_t* period = Period(self, i);
        int start = Seconds(At<int32_t>(period, 0x24), At<int32_t>(period, 0x28));
        int end = Seconds(At<int32_t>(period, 0x2C), At<int32_t>(period, 0x30));
        bool in = start > end ? (now >= start || now < end) : (now >= start && now < end);
        if (in) {
            At<int32_t>(self, 0x50) = i;
            break;
        }
    }
    GC_ListenerNotify(g_periodListener);
    const char* starting = At<const char*>(Period(self, At<int32_t>(self, 0x50)), 0x1C);
    if (starting != nullptr) {
        GC_ScriptCall(script, starting, 0, 0);
        GC_LurRelease(kLurStore, script);
    }
}

// 0x6A8F70: the current period over: the next one (back to the first: a new day)
void __fastcall Periods_Update(uint8_t* self) {
    uint8_t* period = Period(self, At<int32_t>(self, 0x50));
    int end = Seconds(At<int32_t>(period, 0x2C), At<int32_t>(period, 0x30));
    int start = Seconds(At<int32_t>(period, 0x24), At<int32_t>(period, 0x28));
    if (!GC_InRange(self, start, end)) {
        GC_PeriodsAdvance(self);
        if (At<int32_t>(self, 0x50) == 0) {
            At<int32_t>(self, 0x58) += 1;
        }
    }
}

// ---- the completion conditions ------------------------------------------------------------------

// 0x6A8FD0: a condition made by index
void* __cdecl CreateCondition(uint32_t index) {
    struct Kind {
        uint32_t size;
        uintptr_t constructor;
    };
    static const Kind kKinds[10] = {{0x30, 0x006A9120}, {0x10, 0x006A9260}, {8, 0x006A9380}, {8, 0x006A93F0},
                                    {8, 0x006A94A0},    {8, 0x006A95A0},    {8, 0x006A9620}, {8, 0x006A9690},
                                    {8, 0x006A9720},    {8, 0x006A9800}};
    if (index > 9) {
        return nullptr;
    }
    void* condition = GC_Malloc(kKinds[index].size);
    if (condition == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<void*(__thiscall*)(void*)>(kKinds[index].constructor)(condition);
}

// 0x6A9100 (vf0): the deleting destructor
void* __fastcall Condition_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = 0x00941AF0;
    if (flags & 1) {
        GC_Free(self);
    }
    return self;
}

// the constructors: the vtable and the maximum (+4), then the weights
template <uintptr_t Vt, int32_t Max>
void* __fastcall Condition_Construct(uint8_t* self) {
    At<uint32_t>(self, 0) = uint32_t(Vt);
    At<int32_t>(self, 4) = Max;
    return self;
}

// 0x6A9120: CGCC_Chapters (5000: 600 .. 1400 by chapter)
void* __fastcall Chapters_Construct(uint8_t* self) {
    At<uint32_t>(self, 0) = 0x00941B00;
    At<int32_t>(self, 4) = 5000;
    At<int32_t>(self, 8) = 600;
    At<int32_t>(self, 0xC) = 800;
    At<int32_t>(self, 0x10) = 1000;
    At<int32_t>(self, 0x14) = 1200;
    At<int32_t>(self, 0x18) = 1400;
    for (int i = 0x1C; i <= 0x2C; i += 4) {
        At<int32_t>(self, i) = -1;
    }
    return self;
}

// 0x6A9170 (vf1): each chapter's counted missions done, by its weight
int __fastcall Chapters_Points(uint8_t* self) {
    int points = 0;
    int32_t* weight = &At<int32_t>(self, 8);
    for (int chapter = 1; chapter < 6; ++chapter, ++weight) {
        char name[0x20];
        GC_Sprintf(name, "Chapter%d", chapter);
        uint32_t hash = GC_HashName(name);
        int done = 0;
        int total = 0;
        for (int i = 0; i < GC_MissionCount(kMissionMgr); ++i) {
            if (GC_MissionIsKind(kMissionMgr, i, hash) && At<uint8_t>(GC_MissionInfo(kMissionMgr, i), 0x4A) != 0) {
                ++total;
                if (MissionDone(i)) {
                    ++done;
                }
            }
        }
        points += Points(done, total, float(*weight));
    }
    return points;
}

// 0x6A9250 (vf2): the chapters' weights summed
int __fastcall Chapters_MaxPoints(uint8_t* self) {
    return At<int32_t>(self, 0x18) + At<int32_t>(self, 0x14) + At<int32_t>(self, 0x10) + At<int32_t>(self, 0xC) +
           At<int32_t>(self, 8);
}

// 0x6A9260: CGCC_Races (1000: 600 + 400)
void* __fastcall Races_Construct(uint8_t* self) {
    At<uint32_t>(self, 0) = 0x00941B1C;
    At<int32_t>(self, 4) = 1000;
    At<int32_t>(self, 8) = 600;
    At<int32_t>(self, 0xC) = 400;
    return self;
}

// 0x6A9280 (vf1): the two kinds of races done, by their weights
int __fastcall Races_Points(uint8_t* self) {
    int doneA = 0;
    int totalA = 0;
    int doneB = 0;
    int totalB = 0;
    for (int i = 0; i < GC_MissionCount(kMissionMgr); ++i) {
        if (GC_MissionIsKind(kMissionMgr, i, g_hashRacesA)) {
            if (MissionDone(i)) {
                ++doneA;
            }
            ++totalA;
        } else if (GC_MissionIsKind(kMissionMgr, i, g_hashRacesB)) {
            if (MissionDone(i)) {
                ++doneB;
            }
            ++totalB;
        }
    }
    int b = Points(doneB, totalB, float(At<int32_t>(self, 0xC)));
    int a = Points(doneA, totalA, float(At<int32_t>(self, 8)));
    return a + b;
}

// 0x6A9370 / 0x6A93E0 / 0x6A9680 / 0x6A9710 / 0x6A97F0 (vf2): the fixed maxima
template <int32_t Max>
int __fastcall FixedMaxPoints(void*) {
    return Max;
}

// 0x6A9390 (vf1): the 50 errands done, out of 200
int __fastcall Errands_Points(void*) {
    int done = 0;
    for (int i = 0; i < 0x32; ++i) {
        if (GC_ErrandCompletion(kPlayerInfo, i) > 0) {
            ++done;
        }
    }
    float fraction = float(done) / 50.0f;
    return Ftol(fraction * 200.0f);
}

// 0x6A9400 (vf1): the jobs done, out of 200
int __fastcall Jobs_Points(void*) {
    int done = 0;
    int total = 0;
    for (int i = 0; i < GC_MissionCount(kMissionMgr); ++i) {
        if (GC_MissionIsKind(kMissionMgr, i, g_hashJobsA) || GC_MissionIsKind(kMissionMgr, i, g_hashJobsB)) {
            if (MissionDone(i)) {
                ++done;
            }
            ++total;
        }
    }
    return Points(done, total, 200.0f);
}

// 0x6A94B0 (vf1): the minigames (five kinds) done, out of 500
int __fastcall Minigames_Points(void*) {
    int done = 0;
    int total = 0;
    for (int i = 0; i < GC_MissionCount(kMissionMgr); ++i) {
        bool kind = false;
        for (int k = 0; k < 5 && !kind; ++k) {
            kind = GC_MissionIsKind(kMissionMgr, i, g_hashMinigames[k]) != 0;
        }
        if (kind) {
            if (MissionDone(i)) {
                ++done;
            }
            ++total;
        }
    }
    return Points(done, total, 500.0f);
}

// 0x6A95B0 (vf1): the ten classes' grades (up to 5 each) out of 50, of 1000
int __fastcall Classes_Points(void*) {
    int sum = 0;
    for (int i = 0; i < 10; ++i) {
        int grade = g_grades[i];
        int points = grade > 0 ? 1 : 0;
        points += grade > 1;
        points += grade > 2;
        points += grade > 3;
        points += grade > 4;
        sum += points;
    }
    float fraction = float(sum) / 50.0f;
    return Ftol(fraction * 1000.0f);
}

// 0x6A9630 (vf1): the 21 mini-objectives, out of 500
int __fastcall MiniObjectives_Points(void*) {
    int done = 0;
    for (int i = 0; i < 0x15; ++i) {
        if (GC_MiniObjectiveDone(kPlayerInfo, i)) {
            ++done;
        }
    }
    float fraction = float(done) / 21.0f;
    return Ftol(fraction * 500.0f);
}

// 0x6A96A0 (vf1): the collectibles (7 kinds) found, out of 800
int __fastcall Collectibles_Points(void*) {
    int found = 0;
    int total = 0;
    for (int i = 0; i < 7; ++i) {
        found += GC_Collected(kCollectibles, i);
        total += GC_Collectable(kCollectibles, i);
    }
    return Points(found, total, 800.0f);
}

// 0x6A9730 (vf1): the arcade games (0, 1, 2, 5) with Jimmy's high score, out of 300 (games
// 5 and 0 count once together)
int __fastcall ArcadeHighScores_Points(void*) {
    uint16_t initials[0x10];
    GC_FontFormat(initials, GC_TextGet(kTheText, "ARCADE_JIM"), 0xF);
    initials[3] = 0;
    int done = 0;
    int total = 0;
    for (int game = 0; game < 7; ++game) {
        if (game == 0 || game == 1 || game == 2 || game == 5) {
            if (GC_HighScoreBy(game, initials)) {
                ++done;
            }
            ++total;
        }
    }
    total -= 1;
    if (GC_HighScoreBy(5, initials) && GC_HighScoreBy(0, initials)) {
        done -= 1;
    }
    return Points(done, total, 300.0f);
}

// 0x6A9810 (vf1): the clothes owned beyond the start, out of 500
int __fastcall Clothing_Points(void*) {
    int owned = g_clothesOwned - g_clothesStart - g_clothesBase;
    int total = g_clothesTotal - g_clothesBase;
    return Points(owned, total, 500.0f);
}

// 0x6A9850: the conditions made (once)
void __cdecl Completion_Initialise() {
    if (g_conditionsMade == 0) {
        for (int i = 0; i < 10; ++i) {
            g_conditions[i] = GC_CreateCondition(i);
        }
        g_conditionsMade = 1;
    }
}

// 0x6A9890: and deleted
void __cdecl Completion_Shutdown() {
    if (g_conditionsMade != 0) {
        for (int i = 0; i < 10; ++i) {
            GC_Delete(g_conditions[i]);
        }
        g_conditionsMade = 0;
    }
}

// 0x6A98C0: the completion in percent (all points / 100; 80 more when chapters, races,
// jobs, minigames, classes, collectibles and arcade are all at their maxima), 0 .. 100
float __cdecl Completion_Percent() {
    static const int kFull[7] = {0, 1, 3, 4, 5, 7, 8};
    int total = 0;
    bool full = true;
    for (int i = 0; i < 10; ++i) {
        void* condition = g_conditions[i];
        int points = reinterpret_cast<int(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(condition))[1]))(condition);
        for (int j = 0; j < 7; ++j) {
            if (kFull[j] == i) {
                int max = reinterpret_cast<int(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(condition))[2]))(
                    condition);
                full = full && points == max;
            }
        }
        total += points;
    }
    if (full) {
        total += 0x50;
    }
    float percent = float(total) / 100.0f;
    if (percent < 0.0f) {
        return 0.0f;
    }
    if (percent > 100.0f) {
        return 100.0f;
    }
    return percent;
}

} // namespace gcomp

#define GCOMP_REVERSED(name, address, fn)                                                          \
    static void* RE_CONCAT(gcomp_original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(gcomp_original_, __LINE__), \
             re::Mode::Original)

GCOMP_REVERSED("CPeriodMgr::Load", 0x006A8AA0, &gcomp::Periods_Load);
GCOMP_REVERSED("CPeriodMgr::Reset", 0x006A8B40, &gcomp::Periods_Reset);
GCOMP_REVERSED("CPeriodMgr::NowBetween", 0x006A8B50, &gcomp::Periods_NowBetween);
GCOMP_REVERSED("CPeriodMgr::IsNow", 0x006A8BB0, &gcomp::Periods_IsNow);
GCOMP_REVERSED("CPeriodMgr::Find", 0x006A8C30, &gcomp::Periods_Find);
GCOMP_REVERSED("CPeriodMgr::IsDay", 0x006A8C90, &gcomp::Periods_IsDay);
GCOMP_REVERSED("CPeriodMgr::Kind", 0x006A8CC0, &gcomp::Periods_Kind);
GCOMP_REVERSED("CPeriodMgr::NextClass", 0x006A8CF0, &gcomp::Periods_NextClass);
GCOMP_REVERSED("CPeriodMgr::CurrentIsKind3", 0x006A8D20, &gcomp::Periods_CurrentIsKind3);
GCOMP_REVERSED("CPeriodMgr::Length", 0x006A8D30, &gcomp::Periods_Length);
GCOMP_REVERSED("CPeriodMgr::MinutesLeft", 0x006A8D70, &gcomp::Periods_MinutesLeft);
GCOMP_REVERSED("CPeriodMgr::GetStart", 0x006A8DC0, &gcomp::Periods_GetStart);
GCOMP_REVERSED("CPeriodMgr::GetEnd", 0x006A8DF0, &gcomp::Periods_GetEnd);
GCOMP_REVERSED("CPeriodMgr::OnReload", 0x006A8E20, &gcomp::Periods_OnReload);
GCOMP_REVERSED("CPeriodMgr::Shutdown", 0x006A8E40, &gcomp::Periods_Shutdown);
GCOMP_REVERSED("CPeriodMgr::Advance", 0x006A8E50, &gcomp::Periods_Advance);
GCOMP_REVERSED("CPeriodMgr::Update", 0x006A8F70, &gcomp::Periods_Update);
GCOMP_REVERSED("CGameCompletionMgr::CreateCondition", 0x006A8FD0, &gcomp::CreateCondition);
GCOMP_REVERSED("CGameCompletionCondition::DeletingDestruct", 0x006A9100, &gcomp::Condition_DeletingDestruct);
GCOMP_REVERSED("CGCC_Chapters::CGCC_Chapters", 0x006A9120, &gcomp::Chapters_Construct);
GCOMP_REVERSED("CGCC_Chapters::Points", 0x006A9170, &gcomp::Chapters_Points);
GCOMP_REVERSED("CGCC_Chapters::MaxPoints", 0x006A9250, &gcomp::Chapters_MaxPoints);
GCOMP_REVERSED("CGCC_Races::CGCC_Races", 0x006A9260, &gcomp::Races_Construct);
GCOMP_REVERSED("CGCC_Races::Points", 0x006A9280, &gcomp::Races_Points);
GCOMP_REVERSED("CGCC_Races::MaxPoints", 0x006A9370, &gcomp::FixedMaxPoints<1000>);
GCOMP_REVERSED("CGCC_Errands::CGCC_Errands", 0x006A9380, (&gcomp::Condition_Construct<0x00941B2C, 200>));
GCOMP_REVERSED("CGCC_Errands::Points", 0x006A9390, &gcomp::Errands_Points);
GCOMP_REVERSED("CGCC_Jobs::MaxPoints", 0x006A93E0, &gcomp::FixedMaxPoints<200>);
GCOMP_REVERSED("CGCC_Jobs::CGCC_Jobs", 0x006A93F0, (&gcomp::Condition_Construct<0x00941B3C, 200>));
GCOMP_REVERSED("CGCC_Jobs::Points", 0x006A9400, &gcomp::Jobs_Points);
GCOMP_REVERSED("CGCC_Minigames::CGCC_Minigames", 0x006A94A0, (&gcomp::Condition_Construct<0x00941B4C, 500>));
GCOMP_REVERSED("CGCC_Minigames::Points", 0x006A94B0, &gcomp::Minigames_Points);
GCOMP_REVERSED("CGCC_Classes::CGCC_Classes", 0x006A95A0, (&gcomp::Condition_Construct<0x00941B5C, 1000>));
GCOMP_REVERSED("CGCC_Classes::Points", 0x006A95B0, &gcomp::Classes_Points);
GCOMP_REVERSED("CGCC_MiniObjectives::CGCC_MiniObjectives", 0x006A9620,
               (&gcomp::Condition_Construct<0x00941B6C, 500>));
GCOMP_REVERSED("CGCC_MiniObjectives::Points", 0x006A9630, &gcomp::MiniObjectives_Points);
GCOMP_REVERSED("CGCC_Minigames::MaxPoints", 0x006A9680, &gcomp::FixedMaxPoints<500>);
GCOMP_REVERSED("CGCC_Collectibles::CGCC_Collectibles", 0x006A9690, (&gcomp::Condition_Construct<0x00941B7C, 800>));
GCOMP_REVERSED("CGCC_Collectibles::Points", 0x006A96A0, &gcomp::Collectibles_Points);
GCOMP_REVERSED("CGCC_Collectibles::MaxPoints", 0x006A9710, &gcomp::FixedMaxPoints<800>);
GCOMP_REVERSED("CGCC_ArcadeHighScores::CGCC_ArcadeHighScores", 0x006A9720,
               (&gcomp::Condition_Construct<0x00941B8C, 300>));
GCOMP_REVERSED("CGCC_ArcadeHighScores::Points", 0x006A9730, &gcomp::ArcadeHighScores_Points);
GCOMP_REVERSED("CGCC_ArcadeHighScores::MaxPoints", 0x006A97F0, &gcomp::FixedMaxPoints<300>);
GCOMP_REVERSED("CGCC_Clothing::CGCC_Clothing", 0x006A9800, (&gcomp::Condition_Construct<0x00941BA8, 500>));
GCOMP_REVERSED("CGCC_Clothing::Points", 0x006A9810, &gcomp::Clothing_Points);
GCOMP_REVERSED("CGameCompletionMgr::Initialise", 0x006A9850, &gcomp::Completion_Initialise);
GCOMP_REVERSED("CGameCompletionMgr::Shutdown", 0x006A9890, &gcomp::Completion_Shutdown);
GCOMP_REVERSED("CGameCompletionMgr::GetPercent", 0x006A98C0, &gcomp::Completion_Percent);
