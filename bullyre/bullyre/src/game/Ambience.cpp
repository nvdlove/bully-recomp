// The ambience (the object at 0xCF4374): the streamed ambient sound of the area or the
// trigger the camera is in (audio\CONFIG\AreaAmbi.txt, audio\config\Triggers.txt: by time
// of day and season), or the one a script sets; played on the ambient stream (0xCF4324,
// priority 3); and the school's PA announcements. With the time and season conditions
// the audio's files share. 0x593D20..0x594B00.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Pool.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

struct CVector {
    float x, y, z;
};

// an area's ambience (0x18 bytes, six per area)
struct AreaAmbience {
    int32_t soundId;     // +0 (0: none)
    float volume;        // +4 (below 0: a free slot)
    uint8_t hourStart;   // +8
    uint8_t minStart;    // +9
    uint8_t hourEnd;     // +0xA
    uint8_t minEnd;      // +0xB
    int32_t fadeIn;      // +0xC
    int32_t fadeOut;     // +0x10
    uint8_t seasons;     // +0x14 (Condition_Parse's bits)
    uint8_t pad15[3];
};
static_assert(sizeof(AreaAmbience) == 0x18);

// a trigger's ambience (0x1C bytes)
struct TriggerAmbience {
    uint32_t hash;       // +0 the trigger
    int32_t soundId;     // +4
    float volume;        // +8
    uint8_t hourStart;   // +0xC
    uint8_t minStart;    // +0xD
    uint8_t hourEnd;     // +0xE
    uint8_t minEnd;      // +0xF
    int32_t fadeIn;      // +0x10
    int32_t fadeOut;     // +0x14
    uint8_t seasons;     // +0x18
    uint8_t pad19[3];
};
static_assert(sizeof(TriggerAmbience) == 0x1C);

struct Ambience {
    uint8_t stopped;            // +0
    uint8_t paStopped;          // +1
    uint8_t paAfternoon;        // +2 (the next announcement is the afternoon's)
    uint8_t paLunchDone;        // +3
    re::Ptr32<uint8_t> stream;            // +4 the ambient stream (0xCF4324, a MusicStream)
    uint32_t paTime;            // +8
    uint8_t inCutscene;         // +0xC (the camera's mode 3)
    uint8_t unknownD;           // +0xD
    uint8_t padE[2];
    float volume;               // +0x10 the playing one's
    int32_t scriptedSound;      // +0x14
    float scriptedVolume;       // +0x18
    uint8_t scripted;           // +0x1C
    uint8_t pad1D[3];
    int32_t scriptedFadeIn;     // +0x20
    int32_t scriptedFadeOut;    // +0x24
    uint8_t unknown28[4];
    uint32_t frame;             // +0x2C (updates every fourth)
    int32_t playing;            // +0x30 the sound
    int32_t stopFade;           // +0x34 (-1 none)
    uint8_t isPlaying;          // +0x38
    uint8_t pad39[7];
    AreaAmbience areas[64][6];  // +0x40
    TriggerAmbience triggers[200];  // +0x2440
    int32_t numTriggers;        // +0x3A20
    uint8_t unknown3A24[4];
    uint8_t queued;             // +0x3A28 (waits for the stream to stop)
    uint8_t pad3A29[3];
    int32_t queuedSound;        // +0x3A2C
    float queuedVolume;         // +0x3A30
    int32_t queuedFadeIn;       // +0x3A34
    int32_t queuedFadeOut;      // +0x3A38
    uint8_t enabled;            // +0x3A3C
    uint8_t paWaiting;          // +0x3A3D
    uint8_t paPlayed;           // +0x3A3E
    uint8_t pad3A3F;
    uint32_t paWaitStart;       // +0x3A40
};
static_assert(offsetof(Ambience, playing) == 0x30);
static_assert(offsetof(Ambience, areas) == 0x40);
static_assert(offsetof(Ambience, triggers) == 0x2440);
static_assert(offsetof(Ambience, numTriggers) == 0x3A20);
static_assert(offsetof(Ambience, enabled) == 0x3A3C);
static_assert(offsetof(Ambience, paWaitStart) == 0x3A40);

struct LineFile {
    re::Ptr32<char> data;     // +0 (freed after)
    int32_t left;   // +4
    int32_t end;    // +8
};
static_assert(sizeof(LineFile) == 0xC);

inline uint8_t* const ambientStream = reinterpret_cast<uint8_t*>(0x00CF4324);
inline void* const streamChannels = reinterpret_cast<void*>(0x00CF4284);
inline void* const theSounds = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const speech = reinterpret_cast<void*>(0x00CF4378);
GAME_VAR(uint8_t*, ambience, 0x00CF4374);
GAME_VAR(int32_t, chapterMode, 0x00C0F6B8);        // (2, 3: the season flag 0x10)
GAME_VAR(uint8_t, chapter, 0x00A147E0);
GAME_ARRAY(int32_t, chapterSeason, 0x00A147EC);     // 0 summer, 1 fall, 2 winter, 3 spring
GAME_VAR(int32_t, winterVariant, 0x00C1AFFC);       // (1: the 0x20 flag instead of 2)
GAME_VAR(uint8_t, clockHours, 0x00BCEB4B);
GAME_VAR(uint8_t, clockMinutes, 0x00BCEB4A);
GAME_VAR(uint32_t, m_snTimeInMilliseconds, 0x00C1A9B4);
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(void*, population, 0x00C2C108);
GAME_VAR(uint8_t, paAnnounced, 0x00CF3BC8);
GAME_VAR(CPool<uint8_t>*, triggerPool, 0x020C7C48);
GAME_VAR(int32_t, cameraMode, 0x00C3CCF8);
GAME_VAR(uint8_t, audioFlagCF3DD5, 0x00CF3DD5);
GAME_VAR(uint8_t, audioFlagCF4270, 0x00CF4270);
GAME_VAR(void*, streamSounds, 0x00CF70D4);

GAME_FN(int32_t, __thiscall, Channel_Fn5A6D60, 0x005A6D60, void* channel);
GAME_FN(int32_t, __thiscall, Channel_Fn5A6D80, 0x005A6D80, void* channel);
GAME_FN(int32_t, __thiscall, Channel_Fn58E060, 0x0058E060, void* channel);
GAME_FN(void, __thiscall, StreamChannels_SetVolume, 0x005A6530, void* channels, int index, float volume, int ms);
GAME_FN(void*, __thiscall, Stream_GetChannel, 0x0058EE00, void* stream);
GAME_FN(void, __thiscall, Stream_Play, 0x00590470, void* stream, int32_t soundId, int32_t bank, float volume,
        int flag, int priority, int fadeIn, int fadeOut);
GAME_FN(void, __thiscall, Stream_Stop, 0x005913D0, void* stream, int priority, int fadeOut);
GAME_FN(void, __thiscall, Stream_StopAny, 0x00591480, void* stream, int priority);
GAME_FN(int32_t, __thiscall, StreamSounds_GetBank, 0x005A6DD0, void* sounds, int32_t soundId);
GAME_FN(int32_t, __cdecl, HashStreamName, 0x005A7D60, const char* name, int arg);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(uint8_t, __cdecl, IsHourBetween, 0x00425E40, int start, int end, int, int);
inline int __cdecl Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
inline char* __cdecl Strchr(const char* s, int c) { return crt::Strchr(s, c); }
template <typename... A>
inline int __cdecl Sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
GAME_FN(uint8_t, __thiscall, LineFile_Load, 0x00426610, LineFile* file, const char* path);
GAME_FN(uint8_t, __cdecl, LineFile_ReadLine, 0x0042D600, LineFile* file, char* line, int size);
GAME_FN(const char*, __cdecl, GetAreaName, 0x004305B0, int area);
GAME_FN(void, __thiscall, Sounds_AddTrigger, 0x005960A0, void* sounds, uint32_t hash);
GAME_FN(int, __thiscall, Sounds_FindTrigger, 0x00596120, void* sounds, uint32_t hash);
GAME_FN(uint8_t, __thiscall, Area_Contains, 0x006D6FA0, void* area, const CVector* point);
GAME_FN(CVector*, __cdecl, GetCameraPos, 0x00595630, CVector* out);
GAME_FN(int, __thiscall, Population_CountPedsWithName, 0x0049E960, void* population, uint32_t hash, int max);
GAME_FN(void, __thiscall, Speech_Add, 0x005A4920, void* speech, void* out, uintptr_t, uintptr_t, int, int kind, int, int, int,
        int, int, uintptr_t);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
// its own
GAME_FN(uint8_t, __cdecl, Condition_IsMet, 0x00593DF0, int seasons);
GAME_FN(uint8_t, __cdecl, IsTimeBetween, 0x00593E80, int hourStart, int hourEnd, int minStart, int minEnd);
GAME_FN(int32_t, __thiscall, Ambience_GetScripted, 0x00593D50, Ambience* self, float* volume, int32_t* fadeIn,
        int32_t* fadeOut);
GAME_FN(int32_t, __thiscall, Ambience_StreamLoading, 0x00593F50, Ambience* self);
GAME_FN(void, __thiscall, Ambience_Play, 0x00593F90, Ambience* self, int32_t soundId, float volume, int fadeIn,
        int stopFade);
GAME_FN(void, __thiscall, Ambience_StopStream, 0x00593FE0, Ambience* self);
GAME_FN(void, __thiscall, Ambience_Stop, 0x00594000, Ambience* self, int fadeOut);
GAME_FN(int32_t, __thiscall, Ambience_StreamActive, 0x00594030, Ambience* self);
GAME_FN(void, __cdecl, Condition_Parse, 0x00594070, const char* text, uint8_t* out);
GAME_FN(uint8_t, __cdecl, IsPAArea, 0x00594170, int area);
GAME_FN(void, __thiscall, Ambience_UpdatePA, 0x005941D0, Ambience* self);
GAME_FN(int32_t, __thiscall, Ambience_FindArea, 0x00594390, Ambience* self, float* volume, int32_t* fadeIn,
        int32_t* fadeOut);
GAME_FN(int, __stdcall, Ambience_ParseLine, 0x00594480, const char* line, char* sound, float* volume,
        uint8_t* hourStart, uint8_t* minStart, uint8_t* hourEnd, uint8_t* minEnd, int32_t* fadeIn, int32_t* fadeOut,
        uint8_t* seasons);
GAME_FN(void, __thiscall, Ambience_LoadAreas, 0x005945A0, Ambience* self);
GAME_FN(void, __thiscall, Ambience_LoadTriggers, 0x00594760, Ambience* self);
GAME_FN(int32_t, __thiscall, Ambience_FindTrigger, 0x005949D0, Ambience* self, float* volume, int32_t* fadeIn,
        int32_t* fadeOut);

const CVector* PositionOf(uint8_t* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return reinterpret_cast<const CVector*>(matrix ? matrix + 0x30 : entity + 4);
}

// ---- conditions ---------------------------------------------------------------------------------

// 0x593D20 (thiscall, a stream channel's): loaded, not playing, and with its +0x1C
int32_t __fastcall Channel_IsLoadedIdle(uint8_t* self) {
    if (Channel_Fn5A6D60(self) != 0 && Channel_Fn5A6D80(self) == 0 && At<int32_t>(self, 0x1C) != 0) {
        return 1;
    }
    return 0;
}

// 0x593DE0: all four seasons (and the others)
void __cdecl Condition_SetAll(uint8_t* flags) {
    *flags |= 0x3F;
}

// 0x593DF0: the season flags hold now (1 fall, 2 winter, 4 spring, 8 summer, 0x10 in
// chapter modes 2 and 3, 0x20 the other winter)
uint8_t __cdecl Condition_IsMetImpl(int flags) {
    uint8_t f = uint8_t(flags);
    if (chapterMode == 2 || chapterMode == 3) {
        return (f >> 4) & 1;
    }
    switch (chapterSeason[chapter]) {
    case 0:
        return (f & 8) ? 1 : 0;
    case 1:
        return (f & 1) ? 1 : 0;
    case 2:
        if (winterVariant == 1) {
            return (f & 0x20) ? 1 : 0;
        }
        return (f & 2) ? 1 : 0;
    case 3:
        return (f & 4) ? 1 : 0;
    default:
        return 0;
    }
}

// 0x593E80: the clock between two times (across midnight when the end is earlier)
uint8_t __cdecl IsTimeBetweenImpl(int hourStart, int hourEnd, int minStart, int minEnd) {
    uint8_t h1 = uint8_t(hourStart);
    uint8_t h2 = uint8_t(hourEnd);
    if (h1 > 0x17 || h2 > 0x17) {
        return 0;
    }
    if (h1 > h2) {
        if (IsTimeBetween(h1, 0x17, minStart, 0x3B) != 0 || IsTimeBetween(0, h2, 0, minEnd) != 0) {
            return 1;
        }
        return 0;
    }
    uint8_t hour = clockHours;
    uint8_t minute = clockMinutes;
    if (h1 > hour || h2 < hour) {
        return 0;
    }
    if (h1 == hour) {
        return minute >= uint8_t(minStart) ? 1 : 0;
    }
    if (h2 == hour) {
        return uint8_t(minEnd) >= minute ? 1 : 0;
    }
    return 1;
}

// 0x593F10: class time (9 to 11, 13 to 15)
uint8_t __cdecl IsClassTime() {
    uint8_t morning = IsHourBetween(9, 0xB, 0, 0) != 0 ? 1 : 0;
    if (IsHourBetween(0xD, 0xF, 0, 0) != 0) {
        return 1;
    }
    return morning;
}

// 0x594070: the season letters (F W P S R X; "all" or "": the first six)
void __cdecl Condition_ParseImpl(const char* text, uint8_t* out) {
    *out = 0;
    if (Stricmp("all", text) == 0 || Stricmp("", text) == 0) {
        *out |= 0x3F;
    }
    if (Strchr(text, 'F') != nullptr || Strchr(text, 'f') != nullptr) {
        *out |= 1;
    }
    if (Strchr(text, 'W') != nullptr || Strchr(text, 'w') != nullptr) {
        *out |= 2;
    }
    if (Strchr(text, 'P') != nullptr || Strchr(text, 'p') != nullptr) {
        *out |= 4;
    }
    if (Strchr(text, 'S') != nullptr || Strchr(text, 's') != nullptr) {
        *out |= 8;
    }
    if (Strchr(text, 'R') != nullptr || Strchr(text, 'r') != nullptr) {
        *out |= 0x10;
    }
    if (Strchr(text, 'X') != nullptr || Strchr(text, 'x') != nullptr) {
        *out |= 0x20;
    }
}

// 0x594170: the areas the PA is heard in (the school's)
uint8_t __cdecl IsPAAreaImpl(int area) {
    switch (area) {
    case 2: case 4: case 5: case 6: case 9: case 0xE: case 0xF: case 0x11: case 0x12: case 0x17: case 0x26:
        return 1;
    default:
        return 0;
    }
}

// 0x594740 (thiscall, ecx unused): area 0, 0x2A or 0x2B
uint8_t __stdcall IsOutsideArea(int area) {
    if (area != 0 && (area < 0x2A || area > 0x2B)) {
        return 0;
    }
    return 1;
}

// ---- the scripted ambience and the stream ---------------------------------------------------

// 0x593D50 (thiscall): the script's ambience (0: none)
int32_t __fastcall Ambience_GetScriptedImpl(Ambience* self, re::Edx, float* volume, int32_t* fadeIn, int32_t* fadeOut) {
    if (self->scripted == 0) {
        return 0;
    }
    *volume = self->scriptedVolume;
    *fadeIn = self->scriptedFadeIn;
    *fadeOut = self->scriptedFadeOut;
    return self->scriptedSound;
}

// 0x593D80 (thiscall)
void __fastcall Ambience_SetScripted(Ambience* self, re::Edx, const char* name, float volume, int32_t fadeIn,
                                     int32_t fadeOut) {
    int32_t soundId = HashStreamName(name, 1);
    self->scriptedVolume = volume;
    self->scriptedSound = soundId;
    self->scripted = 1;
    self->scriptedFadeIn = fadeIn;
    self->scriptedFadeOut = fadeOut;
}

// 0x593DC0 (thiscall)
void __fastcall Ambience_ClearScripted(Ambience* self) {
    self->scriptedVolume = -1.0f;
    self->scriptedSound = 0;
    self->scripted = 0;
}

// 0x593F50 (thiscall): the stream's channel still loading its sound
int32_t __fastcall Ambience_StreamLoadingImpl(Ambience* self) {
    return Channel_IsLoadedIdle(static_cast<uint8_t*>(Stream_GetChannel(self->stream)));
}

// 0x593F90 (thiscall)
void __fastcall Ambience_PlayImpl(Ambience* self, re::Edx, int32_t soundId, float volume, int fadeIn, int stopFade) {
    int32_t bank = StreamSounds_GetBank(streamSounds, soundId);
    Stream_Play(self->stream, soundId, bank, volume, 1, 3, fadeIn, 500);
    self->volume = volume;
    self->playing = soundId;
    self->isPlaying = 1;
    self->stopFade = stopFade;
}

// 0x593FE0 (thiscall; missing from the function lists: only jumped to)
void __fastcall Ambience_StopStreamImpl(Ambience* self) {
    Stream_StopAny(self->stream, 3);
    self->playing = 0;
    self->stopFade = -1;
    self->isPlaying = 0;
}

// 0x594000 (thiscall)
void __fastcall Ambience_StopImpl(Ambience* self, re::Edx, int fadeOut) {
    Stream_Stop(self->stream, 3, fadeOut);
    self->playing = 0;
    self->stopFade = -1;
    self->isPlaying = 0;
}

// 0x594030 (thiscall): the stream's channel playing or loading
int32_t __fastcall Ambience_StreamActiveImpl(Ambience* self) {
    if (Channel_Fn5A6D80(Stream_GetChannel(self->stream)) != 0) {
        return 1;
    }
    if (Channel_Fn5A6D60(Stream_GetChannel(self->stream)) != 0) {
        return 1;
    }
    return 0;
}

// ---- the PA -------------------------------------------------------------------------------------

// 0x5941D0 (thiscall): the school's announcements (7 to 17 o'clock, in the PA's areas;
// one 90 to 160 s after the game starts, then every 160 s; none while Miss Winston is
// around)
void __fastcall Ambience_UpdatePAImpl(Ambience* self) {
    uint32_t now = m_snTimeInMilliseconds;
    if (self->paStopped != 0) {
        self->paTime = now;
        return;
    }
    if (IsPAArea(currArea) == 0) {
        self->paTime = now - 20000;
    }
    if (Population_CountPedsWithName(population, HashName("N_MissWinston"), 1) != 0) {
        self->paTime = now - 5000;
        return;
    }
    uint8_t out[0x30];
    if (ambience[0xC] == 0 && uint8_t(clockHours - 7) <= 10 && IsPAArea(currArea) != 0) {
        if (self->paWaiting == 1) {
            if (self->paWaitStart > m_snTimeInMilliseconds - 2500) {
                return;
            }
            if (self->paPlayed == 0) {
                Speech_Add(speech, out, 0, 0, 0, 0x125, 0, 0, 0, 0, -1, 0);
            }
            self->paWaiting = 0;
            self->paPlayed = 1;
        }
        if ((now - 90001 > 70000 || paAnnounced != 0) && now <= self->paTime + 160000) {
            return;
        }
        paAnnounced = 1;
        int kind;
        if (self->paAfternoon == 0) {
            kind = 0x128;
        } else if (clockHours < 0xD && self->paLunchDone == 0) {
            kind = 0x129;
        } else {
            self->paLunchDone = 1;
            kind = 0x12A;
        }
        Speech_Add(speech, out, 0, 0, 0, kind, 0, 0, 0, 0, -1, 0);
    }
    self->paTime = now;
}

// ---- the ambience's tables -------------------------------------------------------------------

// 0x594390 (thiscall): the area's ambience for now (0: none)
int32_t __fastcall Ambience_FindAreaImpl(Ambience* self, re::Edx, float* volume, int32_t* fadeIn, int32_t* fadeOut) {
    int32_t area = currArea;
    if (area == 0 || self->enabled == 0) {
        *volume = 0.0f;
        return 0;
    }
    for (int i = 0; i < 6; i++) {
        AreaAmbience& a = self->areas[area][i];
        if (IsTimeBetween(a.hourStart, a.hourEnd, a.minStart, a.minEnd) != 0 && Condition_IsMet(a.seasons) != 0) {
            *volume = self->areas[area][i].volume;
            *fadeIn = self->areas[currArea][i].fadeIn;
            *fadeOut = self->areas[currArea][i].fadeOut;
            return self->areas[currArea][i].soundId;
        }
    }
    *volume = 0.0f;
    return 0;
}

// 0x594480 (thiscall, ecx unused): a line of AreaAmbi.txt ("area sound volume h:m h:m
// in:ms out:ms seasons"); its area's index (-1: none). (The parsed numbers are the
// game's uninitialised locals when the line is short: zeroes here)
int __stdcall Ambience_ParseLineImpl(const char* line, char* sound, float* volume, uint8_t* hourStart,
                                     uint8_t* minStart, uint8_t* hourEnd, uint8_t* minEnd, int32_t* fadeIn,
                                     int32_t* fadeOut, uint8_t* seasons) {
    char area[128];
    char seasonText[32];
    uint32_t h1 = 0, m1 = 0, h2 = 0, m2 = 0;
    int32_t in = 0, out = 0;
    area[0] = 0;
    seasonText[0] = 0;
    Sscanf(line, "%s\t%s\t%f\t%u:%u\t%u:%u\tin:%d\tout:%d\t%s\n", area, sound, volume, &h1, &m1, &h2, &m2, &in,
           &out, seasonText);
    *hourStart = uint8_t(h1);
    *minStart = uint8_t(m1);
    *hourEnd = uint8_t(h2);
    *minEnd = uint8_t(m2);
    *fadeIn = in;
    *fadeOut = out;
    Condition_Parse(seasonText, seasons);
    for (int i = 0; i < 0x40; i++) {
        if (std::strcmp(area, GetAreaName(i)) == 0) {
            return i;
        }
    }
    return -1;
}

// 0x5945A0 (thiscall): AreaAmbi.txt read (into each area's first free slot)
void __fastcall Ambience_LoadAreasImpl(Ambience* self) {
    // (kept from line to line)
    float volume = 0.0f;
    uint8_t hourEnd = 0, hourStart = 0, minEnd = 0, minStart = 0, seasons = 0;
    int32_t fadeIn = 0, fadeOut = 0;
    LineFile file{nullptr, 0, 0};
    char sound[64];
    char line[512];
    LineFile_Load(&file, "audio\\CONFIG\\AreaAmbi.txt");
    while (LineFile_ReadLine(&file, line, 0x200) != 0) {
        if (line[0] == '#' || Strchr(line, '\t') == nullptr) {
            continue;
        }
        int area = Ambience_ParseLine(line, sound, &volume, &hourStart, &minStart, &hourEnd, &minEnd, &fadeIn,
                                      &fadeOut, &seasons);
        if (area == -1) {
            continue;
        }
        for (int i = 0; i < 6; i++) {
            AreaAmbience& a = self->areas[area][i];
            if (a.volume < 0.0f) {   // (a free slot)
                a.soundId = HashStreamName(sound, 1);
                a.volume = volume;
                a.hourStart = hourStart;
                a.minEnd = minEnd;
                a.minStart = minStart;
                a.hourEnd = hourEnd;
                a.fadeIn = fadeIn;
                a.fadeOut = fadeOut;
                a.seasons = seasons;
                break;
            }
        }
    }
    if (file.data != nullptr) {
        Free(file.data);
    }
}

// 0x594760 (thiscall): Triggers.txt read ("trigger sound volume h:m h:m in:ms out:ms
// seasons"; no check against the 200 slots)
void __fastcall Ambience_LoadTriggersImpl(Ambience* self) {
    int count = 0;
    LineFile file{nullptr, 0, 0};
    // (the game's locals: uninitialised before the first line that parses; zeroes here)
    char trigger[64] = {};
    char sound[64] = {};
    char seasonText[64] = {};
    float volume = 0.0f;
    uint32_t h1 = 0, m1 = 0, h2 = 0, m2 = 0;
    int32_t in = 0, out = 0;
    char line[512];
    LineFile_Load(&file, "audio\\config\\Triggers.txt");
    if (LineFile_ReadLine(&file, line, 0x200) != 0) {
        TriggerAmbience* t = self->triggers;
        do {
            if (line[0] != '#' && Strchr(line, '\t') != nullptr && std::strlen(line) >= 5) {
                Sscanf(line, "%s\t%s\t%f\t%u:%u\t%u:%u\tin:%d\tout:%d\t%s\n", trigger, sound, &volume, &h1, &m1, &h2,
                       &m2, &in, &out, seasonText);
                uint32_t hash = HashName(trigger);
                t->hash = hash;
                Sounds_AddTrigger(theSounds, hash);
                int32_t soundId = HashStreamName(sound, 1);
                t->volume = volume;
                t->hourStart = uint8_t(h1);
                t->minStart = uint8_t(m1);
                t->soundId = soundId;
                t->minEnd = uint8_t(m2);
                t->fadeIn = in;
                t->hourEnd = uint8_t(h2);
                t->fadeOut = out;
                Condition_Parse(seasonText, &t->seasons);
                count++;
                t++;
            }
        } while (LineFile_ReadLine(&file, line, 0x200) != 0);
    }
    self->numTriggers = count;
    if (file.data != nullptr) {
        Free(file.data);
    }
}

// 0x594900 (thiscall): the constructor's work (both files read)
uint8_t __fastcall Ambience_Init(Ambience* self) {
    self->paWaiting = 0;
    self->paPlayed = 0;
    self->paTime = 0;
    self->paAfternoon = 0;
    self->paLunchDone = 0;
    self->stream = ambientStream;
    for (int i = 0; i < 200; i++) {
        self->triggers[i].soundId = 0;
        self->triggers[i].hash = 0;
    }
    self->numTriggers = 0;
    self->queuedVolume = 0.0f;
    self->enabled = 1;
    self->isPlaying = 0;
    self->frame = 0;
    self->queued = 0;
    self->queuedSound = 0;
    self->unknownD = 0;
    for (int area = 0; area < 0x40; area++) {
        for (int i = 0; i < 6; i++) {
            AreaAmbience& a = self->areas[area][i];
            a.volume = -1000.0f;
            a.hourStart = 0xFF;
            a.hourEnd = 0xFF;
            a.minStart = 0xFF;
            a.minEnd = 0xFF;
            a.soundId = 0;
            a.fadeIn = 0;
            a.fadeOut = 0;
        }
    }
    Ambience_LoadAreas(self);
    Ambience_LoadTriggers(self);
    self->scriptedVolume = 0.0f;
    self->paStopped = 0;
    self->paAfternoon = 0;
    self->stopped = 0;
    self->scriptedSound = 0;
    self->scripted = 0;
    return 1;
}

// 0x5949D0 (thiscall): the ambience of the trigger the camera is in (0: none)
int32_t __fastcall Ambience_FindTriggerImpl(Ambience* self, re::Edx, float* volume, int32_t* fadeIn, int32_t* fadeOut) {
    CVector camera;
    GetCameraPos(&camera);
    if (self->enabled == 0) {
        *volume = 0.0f;
        return 0;
    }
    for (int i = 0; i < self->numTriggers; i++) {
        TriggerAmbience& t = self->triggers[i];
        if (t.hash == 0) {
            continue;
        }
        int index = Sounds_FindTrigger(theSounds, t.hash);
        if (index == -1 || index < 0) {
            continue;
        }
        CPool<uint8_t>* pool = triggerPool;
        if (pool->flags[index] & 0x80) {
            continue;
        }
        uint8_t* trigger = pool->objects + pool->elemSize * index;
        if (trigger == nullptr || int32_t(int8_t(trigger[0x3C])) != currArea) {
            continue;
        }
        if (Area_Contains(trigger, &camera) == 0) {
            continue;
        }
        if (IsTimeBetween(t.hourStart, t.hourEnd, t.minStart, t.minEnd) == 0) {
            continue;
        }
        if (Condition_IsMet(t.seasons) == 0) {
            continue;
        }
        *volume = self->triggers[i].volume;
        *fadeIn = self->triggers[i].fadeIn;
        *fadeOut = self->triggers[i].fadeOut;
        return self->triggers[i].soundId;
    }
    return 0;
}

// 0x594B00 (thiscall): the per-frame update (every fourth frame): the script's, the
// trigger's or the area's ambience on the stream, a new one after the old one stops
void __fastcall Ambience_Update(Ambience* self) {
    self->inCutscene = cameraMode == 3 ? 1 : 0;
    bool stop = self->stopped == 1;
    if (!stop) {
        if (audioFlagCF3DD5 != 0) {
            return;
        }
        stop = audioFlagCF4270 != 0 || self->inCutscene != 0;
    }
    if (stop) {
        if (self->isPlaying == 0) {
            return;
        }
        Stream_StopAny(self->stream, 3);
        self->playing = 0;
        self->isPlaying = 0;
        self->stopFade = -1;
        return;
    }
    if (self->enabled == 0) {
        if (self->isPlaying == 0) {
            return;
        }
        Ambience_StopStream(self);
        return;
    }
    self->frame++;
    if (self->frame <= 3) {
        return;
    }
    self->frame = 0;
    Ambience_UpdatePA(self);
    if (uint8_t(Ambience_StreamLoading(self)) != 0) {
        return;
    }
    float volume = 0.0f;
    int32_t fadeOut = 0;   // [esp+0x10]
    int32_t fadeIn = 0;    // [esp+0x14]
    if (self->queued != 0) {
        if (Channel_Fn58E060(Stream_GetChannel(self->stream)) == 0) {
            Stream_Stop(self->stream, 3, 500);
            return;
        }
        Ambience_Play(self, self->queuedSound, self->queuedVolume, self->queuedFadeIn, self->queuedFadeOut);
        self->queued = 0;
        self->queuedSound = 0;
        self->queuedFadeIn = -1;
        self->queuedFadeOut = -1;
        return;
    }
    int32_t sound = Ambience_GetScripted(self, &volume, &fadeIn, &fadeOut);
    if (sound == 0) {
        sound = Ambience_FindTrigger(self, &volume, &fadeIn, &fadeOut);
        if (sound == 0) {
            sound = Ambience_FindArea(self, &volume, &fadeIn, &fadeOut);
        }
    }
    if (sound == 0) {
        if (uint8_t(Ambience_StreamActive(self)) == 0 || uint8_t(Ambience_StreamLoading(self)) != 0) {
            return;
        }
        if (self->stopFade > -1) {
            Ambience_Stop(self, self->stopFade);
            return;
        }
        Ambience_StopStream(self);
        return;
    }
    if (self->playing != sound) {
        if (uint8_t(Ambience_StreamActive(self)) != 0) {
            // (the new one waits for the old one to fade)
            Ambience_Stop(self, self->stopFade > -1 ? self->stopFade : 5000);
            self->queuedVolume = volume;
            self->queuedSound = sound;
            self->queued = 1;
            self->queuedFadeIn = fadeIn;
            self->queuedFadeOut = fadeOut;
            return;
        }
        Ambience_Play(self, sound, volume, fadeIn, fadeOut);
        return;
    }
    if (uint8_t(Ambience_StreamActive(self)) == 0 && sound != 0 && uint8_t(Ambience_StreamLoading(self)) == 0) {
        Ambience_Play(self, sound, volume, fadeIn, fadeOut);
    }
    if (uint8_t(Ambience_StreamLoading(self)) != 0) {
        return;
    }
    if (!(self->volume == volume)) {
        StreamChannels_SetVolume(streamChannels, At<int32_t>(self->stream, 0x40), volume, 500);
        self->volume = volume;
    }
}

// ---- registration -----------------------------------------------------------------------------

#define AMBIENCE_REVERSED(name, address, fn)                                                            \
    static void* RE_CONCAT(ambience_original_, __LINE__);                                                \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(ambience_original_, __LINE__), \
             re::Mode::Original)

AMBIENCE_REVERSED("StreamChannel::IsLoadedIdle", 0x00593D20, &Channel_IsLoadedIdle);
AMBIENCE_REVERSED("Ambience::GetScripted", 0x00593D50, &Ambience_GetScriptedImpl);
AMBIENCE_REVERSED("Ambience::SetScripted", 0x00593D80, &Ambience_SetScripted);
AMBIENCE_REVERSED("Ambience::ClearScripted", 0x00593DC0, &Ambience_ClearScripted);
AMBIENCE_REVERSED("AudioCondition::SetAll", 0x00593DE0, &Condition_SetAll);
AMBIENCE_REVERSED("AudioCondition::IsMet", 0x00593DF0, &Condition_IsMetImpl);
AMBIENCE_REVERSED("AudioCondition::IsTimeBetween", 0x00593E80, &IsTimeBetweenImpl);
AMBIENCE_REVERSED("AudioCondition::IsClassTime", 0x00593F10, &IsClassTime);
AMBIENCE_REVERSED("Ambience::StreamLoading", 0x00593F50, &Ambience_StreamLoadingImpl);
AMBIENCE_REVERSED("Ambience::Play", 0x00593F90, &Ambience_PlayImpl);
AMBIENCE_REVERSED("Ambience::StopStream", 0x00593FE0, &Ambience_StopStreamImpl);
AMBIENCE_REVERSED("Ambience::Stop", 0x00594000, &Ambience_StopImpl);
AMBIENCE_REVERSED("Ambience::StreamActive", 0x00594030, &Ambience_StreamActiveImpl);
AMBIENCE_REVERSED("AudioCondition::Parse", 0x00594070, &Condition_ParseImpl);
AMBIENCE_REVERSED("AudioCondition::IsPAArea", 0x00594170, &IsPAAreaImpl);
AMBIENCE_REVERSED("Ambience::UpdatePA", 0x005941D0, &Ambience_UpdatePAImpl);
AMBIENCE_REVERSED("Ambience::FindArea", 0x00594390, &Ambience_FindAreaImpl);
AMBIENCE_REVERSED("Ambience::ParseLine", 0x00594480, &Ambience_ParseLineImpl);
AMBIENCE_REVERSED("Ambience::LoadAreas", 0x005945A0, &Ambience_LoadAreasImpl);
AMBIENCE_REVERSED("AudioCondition::IsOutsideArea", 0x00594740, &IsOutsideArea);
AMBIENCE_REVERSED("Ambience::LoadTriggers", 0x00594760, &Ambience_LoadTriggersImpl);
AMBIENCE_REVERSED("Ambience::Init", 0x00594900, &Ambience_Init);
AMBIENCE_REVERSED("Ambience::FindTrigger", 0x005949D0, &Ambience_FindTriggerImpl);
AMBIENCE_REVERSED("Ambience::Update", 0x00594B00, &Ambience_Update);

} // namespace
