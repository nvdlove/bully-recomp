// The music stream (0xCF42D4: the one stream channel the music plays on, with a request
// that waits until the current one has stopped) and the interactive music on top of it
// (0xCF3B28: picks the entry whose condition holds, crossfades between its levels with
// the transitions of 0xCF3B0C's sound table, and keeps the fight stats it reads from the
// ped pool), 0x58FF50..0x591B00.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Pool.h"
#include "reversed.h"

namespace {

using re::At;

struct CVector {
    float x, y, z;
};

// a stream request (0x20 bytes: the playing one, and the one queued behind it)
struct StreamRequest {
    int32_t state;        // +0 0 idle, 1 stopping, 2 preloaded, 3 playing
    int32_t priority;     // +4 (5 the interactive music, 6 and 7 the missions')
    int32_t soundId;      // +8
    float volume;         // +0xC
    uint8_t flag;         // +0x10
    uint8_t pad11[3];
    uint32_t startTime;   // +0x14
    uint16_t fadeOut;     // +0x18 ms
    uint16_t fadeIn;      // +0x1A ms
    int32_t bank;         // +0x1C the stream sounds' index of the sound (to the channel's +0x94)
};
static_assert(sizeof(StreamRequest) == 0x20);

struct MusicStream {
    StreamRequest current;   // +0
    StreamRequest pending;   // +0x20 (state 0: none)
    int32_t channelIndex;    // +0x40 into the stream channels (0xCF4288)
    uint32_t playTime;       // +0x44
    uint8_t hasChannel;      // +0x48
    uint8_t missionActive;   // +0x49 (the mission manager's last answer)
};
static_assert(offsetof(MusicStream, pending) == 0x20);
static_assert(offsetof(MusicStream, channelIndex) == 0x40);
static_assert(offsetof(MusicStream, missionActive) == 0x49);

// a crossfade slot of the interactive music (0x30 bytes)
struct MusicSlot {
    re::Ptr32<uint8_t> transition;  // +0 (58E3E0's: +2 random sounds, +4 streamed, +5, +9, +0xA, +0x10 u16,
                          //  +0x12 u16 fade, +0x18 volume)
    int32_t soundFrom;    // +4
    int32_t soundTo;      // +8
    int32_t soundMid;     // +0xC (a random one of the sound table played between)
    uint8_t flags;        // +0x10 1 active, 2 the transition's +4
    uint8_t pad11[3];
    int32_t bankFrom;     // +0x14
    int32_t bankTo;       // +0x18
    int32_t bankMid;      // +0x1C
    uint8_t typeFrom;     // +0x20
    uint8_t typeTo;       // +0x21
    uint8_t pad22[2];
    float volume;         // +0x24 \ the level's (copied from a MusicLevelInfo)
    int32_t levelTime;    // +0x28 |
    int32_t levelRandom;  // +0x2C /
};
static_assert(sizeof(MusicSlot) == 0x30);

struct InteractiveMusic {
    MusicSlot slots[3];   // +0
    int32_t current;      // +0x90
    int32_t unknown94;    // +0x94
    uint32_t waitUntil;   // +0x98
};
static_assert(offsetof(InteractiveMusic, current) == 0x90);

// what the update hands the slot (a copy of three dwords)
struct MusicLevelInfo {
    float volume;
    int32_t time;
    uint8_t random;
    uint8_t pad[3];
};
static_assert(sizeof(MusicLevelInfo) == 0xC);

// The music entries (0x40 bytes each, 0xCF3B04; count 0xCF3AF4): +0 type (the condition
// function's index into 0xAE6288), +1 priority (lower wins), +4 u16 fade in, +6 u16 fade
// out, +0x10 u16 the time until the next update, +0x12 current level, +0x13 wanted level,
// +0x1C volume, +0x1D volume follows the stream, +0x1E, +0x20 the levels (10 bytes each:
// +0 random min, +1 random range, +2 count, +3 volume, +4 u16 sound table index, +6 u16
// time, +8 u16 fade).
constexpr int kEntrySize = 0x40;
constexpr int kNoEntry = 0x1F;

inline MusicStream& stream = *reinterpret_cast<MusicStream*>(0x00CF42D4);
inline InteractiveMusic& music = *reinterpret_cast<InteractiveMusic*>(0x00CF3B28);
inline void* const streamChannels = reinterpret_cast<void*>(0x00CF4284);   // +4 the channels
inline void* const missionMgr = reinterpret_cast<void*>(0x020C3CA0);
inline void* const theZones = reinterpret_cast<void*>(0x020C7E98);
inline void* const currentCandidate = reinterpret_cast<void*>(0x00AE5F84);  // the ambient stream's
GAME_ARRAY(uint8_t*, channels, 0x00CF4288);
GAME_ARRAY(uint8_t, musicStats, 0x00CF3A30);        // 0x68 bytes (see GatherStats)
GAME_ARRAY(uint8_t, musicStatsPrev, 0x00CF39C8);    // last update's
using EntryCondition = uint8_t(__cdecl*)(uint8_t* entry);
GAME_ARRAY(EntryCondition, entryConditions, 0x00AE6288);   // by the entry's type
GAME_VAR(uint8_t, candidateTypeBits, 0x00AE5F98);   // (the low four bits, signed)
GAME_VAR(uint8_t*, curEntry, 0x00CF3A98);
GAME_VAR(int32_t, curSound, 0x00CF3A9C);
GAME_VAR(uint32_t, levelTime, 0x00CF3AA0);
GAME_VAR(uint8_t, levelRandom, 0x00CF3AA4);
GAME_VAR(int16_t, forcedEntry, 0x00CF3ADE);         // (0x1F none)
GAME_VAR(uint8_t, musicBusy, 0x00CF3AE6);
GAME_VAR(uint8_t, interactiveMusicDisabled, 0x00CF3AE7);
GAME_VAR(uint32_t, nextUpdate, 0x00CF3AE8);
GAME_VAR(int32_t, interactiveMusicState, 0x00CF3AEC);  // updates left holding the current one
GAME_VAR(uint8_t, carnivalVolume, 0x00CF3AF0);
GAME_VAR(uint8_t, lastChange, 0x00CF3AF1);          // 1 a new entry, 2 a level change, 3 a random one
GAME_VAR(uint8_t, forcedEntrySet, 0x00CF3AF2);
GAME_VAR(uint8_t, stopRequested, 0x00CF3AF3);
GAME_VAR(int32_t, numEntries, 0x00CF3AF4);
GAME_VAR(uint8_t*, entries, 0x00CF3B04);
GAME_VAR(int32_t*, soundTable, 0x00CF3B0C);
GAME_VAR(uint16_t, forcedPriority, 0x00CF3B10);
GAME_VAR(uint8_t, missionMusicPaused, 0x00CF3B12);
GAME_VAR(uint8_t, missionMusicType, 0x00AE6304);    // (0x1B while a mission's music is forced)
GAME_VAR(int32_t, currentGoKartRace, 0x00AE6284);
GAME_VAR(float, lastStreamVolume, 0x020C8AD8);
GAME_VAR(void*, streamSounds, 0x00CF70D4);
GAME_VAR(uint32_t, m_snTimeInMilliseconds, 0x00C1A9B4);
GAME_VAR(uint32_t, realTimeMs, 0x00A14A50);
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);
GAME_VAR(uint8_t*, hudHealth, 0x00C674D0);          // +0xAC, +0xB0 the peds whose bars show
GAME_VAR(uint8_t*, punishment, 0x00C0F78C);
GAME_VAR(CPool<uint8_t>*, pedPool, 0x00C0F5F0);
GAME_VAR(CPool<uint8_t>*, triggerPool, 0x020C7C48);
GAME_VAR(void*, ambienceEmitters, 0x00CF4278);
GAME_VAR(uint8_t, flag20C5BE6, 0x020C5BE6);         // (a cutscene runs)
GAME_VAR(uint8_t, flag20C5BE9, 0x020C5BE9);
GAME_VAR(uint8_t, flag20C870A, 0x020C870A);
GAME_VAR(uint8_t, musicDuringCutscenes, 0x00CF3DA4);
GAME_VAR(int32_t, ignoredModel0, 0x00BF42DC);
GAME_VAR(int32_t, ignoredModel1, 0x00BF42D4);
GAME_VAR(int32_t, ignoredModel2, 0x00BF42D0);
GAME_VAR(int32_t, ignoredModel3, 0x00BF42CC);

GAME_FN(void*, __thiscall, MissionMgr_Get0, 0x006AA680, void* mgr);
GAME_FN(void*, __thiscall, MissionMgr_Get1, 0x006AA690, void* mgr);
GAME_FN(uint8_t, __thiscall, MissionSlot_IsActive, 0x006AA3F0, void* slot);
GAME_FN(uint8_t, __thiscall, MissionMgr_IsRunning, 0x006AA940, void* mgr, int32_t mission);
GAME_FN(int32_t, __thiscall, MissionMgr_FindMission, 0x006A9FD0, void* mgr, uint32_t* key);
GAME_FN(uint32_t*, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(uint8_t, __thiscall, MissionMgr_Fn6AA7E0, 0x006AA7E0, void* mgr);
GAME_FN(uint8_t, __cdecl, IsTimePaused, 0x00705500);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void, __thiscall, AmbienceEmitters_Enable, 0x005873C0, void* emitters, uint32_t hash, int on);
GAME_FN(int32_t, __thiscall, Zones_FindByName, 0x006D7A10, void* zones, const char* name);
GAME_FN(uint8_t, __thiscall, Area_Contains, 0x006D6FA0, void* area, const CVector* point);
GAME_FN(void, __cdecl, Emitter_SetVolume, 0x0058EFB0, void* emitter, int on, float volume);
GAME_FN(int32_t, __cdecl, HashStreamName, 0x005A7D60, const char* name, int arg);
GAME_FN(int32_t, __thiscall, StreamSounds_GetBank, 0x005A6DD0, void* sounds, int32_t soundId);
GAME_FN(int32_t, __cdecl, RandomInt, 0x0040DA80, int min, int max);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Magnitude2D, 0x004246B0, const CVector* v);
GAME_FN(uint8_t, __thiscall, Ped_Fn473FB0, 0x00473FB0, void* ped);
GAME_FN(uint8_t, __thiscall, Ped_Fn473E40, 0x00473E40, void* ped);
GAME_FN(uint8_t*, __thiscall, Ped_GetTarget46ECE0, 0x0046ECE0, void* ped);
GAME_FN(uint8_t*, __thiscall, Ped_GetModelInfo477DC0, 0x00477DC0, void* ped);
GAME_FN(int32_t*, __thiscall, PedModelInfo_GetType, 0x00522380, void* info, int32_t* out);
GAME_FN(float, __cdecl, Music_ModelThreat, 0x0058E8E0, int model);
// the stream channels
GAME_FN(void, __thiscall, StreamChannels_Play, 0x005A61F0, void* channels, int index);
GAME_FN(int32_t, __thiscall, Channel_Fn58E060, 0x0058E060, void* channel);
GAME_FN(void, __thiscall, Channel_Preload, 0x005A7880, void* channel, uintptr_t, int32_t soundId, float volume, int,
        int flag, int);
GAME_FN(void, __thiscall, Channel_Play, 0x005A7A00, void* channel, uintptr_t, int32_t soundId, float volume, int mode,
        int flag);
GAME_FN(void, __thiscall, Channel_Fn5A6D20, 0x005A6D20, void* channel, uintptr_t, int);
GAME_FN(void, __thiscall, Channel_SetCandidate, 0x005A6CE0, void* channel, void* candidate, int type);
GAME_FN(void, __thiscall, Channel_Fn5A6EA0, 0x005A6EA0, void* channel);
GAME_FN(void, __thiscall, Channel_Stop, 0x005A7140, void* channel, int fadeMs);
GAME_FN(int32_t, __thiscall, Channel_Fn5A6D80, 0x005A6D80, void* channel);
GAME_FN(void, __thiscall, Channel_Fn5A6E70, 0x005A6E70, void* channel);
GAME_FN(void, __thiscall, Channel_SetVolume5A6ED0, 0x005A6ED0, void* channel, float volume, int);
GAME_FN(void, __thiscall, Channel_Queue5A6BC0, 0x005A6BC0, void* channel, int32_t soundId, int, int, int, int);
GAME_FN(void, __thiscall, Channel_FadeTo, 0x0058E360, void* channel, float from, float to, uint32_t time, int ms);
GAME_FN(void, __thiscall, Channel_FadeIn, 0x0058E3A0, void* channel, float volume, uint32_t time, int ms);
GAME_FN(void, __thiscall, Channel_Empty8A20A0, 0x008A20A0, void* channel, int);   // (just a ret 4)
GAME_FN(void, __thiscall, Fader_Start59E5B0, 0x0059E5B0, void* fader, void* target, float from, float to,
        uint32_t time, int ms);
GAME_FN(uint8_t, __cdecl, Fn5AF560, 0x005AF560, int32_t handle, int32_t* out);
// the stream
GAME_FN(void*, __thiscall, Stream_GetChannel, 0x0058EE00, void* stream);
GAME_FN(void, __thiscall, Stream_SetState, 0x00590620, void* stream, int state);
GAME_FN(void, __thiscall, Stream_Play, 0x00590470, void* stream, int32_t soundId, int32_t bank, float volume,
        int flag, int priority, int fadeIn, int fadeOut);
GAME_FN(void, __thiscall, Stream_Preload, 0x00590570, void* stream, int32_t soundId, int32_t bank, float volume,
        int flag, int priority, int fadeIn, int fadeOut);
GAME_FN(void, __thiscall, Stream_Stop, 0x005913D0, void* stream, int priority, int fadeOut);
GAME_FN(void, __thiscall, Stream_StopAny, 0x00591480, void* stream, int priority);
// the interactive music
GAME_FN(uint8_t*, __thiscall, Music_FindTransition, 0x0058E3E0, void* music, int32_t from, int32_t to, int typeFrom,
        int typeTo);
GAME_FN(uint8_t*, __thiscall, Music_FindTransition58E760, 0x0058E760, void* music, int32_t from, int32_t to,
        int typeFrom, int typeTo);
GAME_FN(int32_t, __thiscall, Music_PickSound, 0x0058E520, void* music, re::Ptr32<uint8_t>* transition, int mode, int32_t from,
        int typeFrom, int typeTo, int level);
GAME_FN(int32_t, __thiscall, Music_PickRandom, 0x0058EE70, void* music, re::Ptr32<uint8_t>* transition, int32_t from, int type,
        int level);
GAME_FN(int32_t, __cdecl, Music_EntrySound, 0x0058EBA0, uint8_t* entry);
GAME_FN(int32_t, __cdecl, Music_EntryRandomSound, 0x0058EC60, uint8_t* entry, int32_t from);
GAME_FN(void, __cdecl, Music_ResetEntries, 0x0058ECD0);
GAME_FN(void, __thiscall, Music_EndSlot, 0x0058EE90, void* music);
GAME_FN(void, __thiscall, Music_Fn58EEE0, 0x0058EEE0, void* music);
GAME_FN(void, __thiscall, Music_UpdateSlot, 0x00590950, void* music);
GAME_FN(uint8_t, __thiscall, Music_StartSlot, 0x00591590, void* music, MusicLevelInfo* info, int32_t from,
        int32_t to, int typeFrom, int typeTo);
GAME_FN(uint8_t, __cdecl, Music_IsAllowed, 0x0058FF50);
GAME_FN(void, __cdecl, Music_GatherStats, 0x00590C90);
GAME_FN(void, __cdecl, Music_ForceEntryFn, 0x00591900, int32_t soundId, float volume, int fadeIn, int fadeOut,
        int missionType);

// the channel a stream plays on (inlined everywhere: 0x58EE00)
uint8_t* ChannelOf(const MusicStream& s) {
    return s.hasChannel ? channels[s.channelIndex] : nullptr;
}

int32_t ChannelState(void* channel) {
    return *static_cast<int32_t*>(channel);   // (0x773DB0)
}

int32_t PriorityOf(const MusicStream& s) {
    if (s.pending.state != 0) {
        return s.pending.priority;
    }
    return s.current.state != 0 ? s.current.priority : 0;
}

uint8_t* EntryAt(int index) {
    return entries + index * kEntrySize;
}

// a mission by its name running (the name key's destructor is the empty function)
uint8_t MissionRunning(const char* name) {
    uint32_t key;
    int32_t mission = MissionMgr_FindMission(missionMgr, NameKey_Construct(&key, name));
    return MissionMgr_IsRunning(missionMgr, mission);
}

// the stats' nibble counters (a byte holds two)
void IncLow(uint8_t& b) {
    b = uint8_t(b ^ ((uint8_t(b + 1) ^ b) & 0xF));
}
void IncHigh(uint8_t& b) {
    b = uint8_t((uint8_t((b & 0xF0) + 0x10)) ^ (b & 0xF));
}

float Clamp01(float v) {
    if (v < 0.0f) {
        return 0.0f;
    }
    if (v > 1.0f) {
        return 1.0f;
    }
    return v;
}

const CVector* PositionOf(uint8_t* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return reinterpret_cast<const CVector*>(matrix ? matrix + 0x30 : entity + 4);
}

// ---- the stream ------------------------------------------------------------------------------

// 0x590470 (thiscall): play a sound, or queue it if the stream is busy with one of a
// higher priority (the same sound playing: just its volume and fades change)
void __fastcall MusicStream_Play(MusicStream* self, re::Edx, int32_t soundId, int32_t bank, float volume, int flag,
                                 int priority, int fadeIn, int fadeOut) {
    if (PriorityOf(*self) == priority && self->current.state == 3 && self->current.soundId == soundId) {
        self->current.volume = volume;
        self->current.fadeIn = uint16_t(fadeIn);
        self->current.fadeOut = uint16_t(fadeOut);
        // (the global stream's channel, whichever stream this is)
        uint8_t* channel = ChannelOf(stream);
        Channel_FadeTo(channel, At<float>(channel, 0x18), volume, realTimeMs, 500);
        return;
    }
    if (PriorityOf(*self) <= priority) {
        self->pending.priority = priority;
        self->pending.volume = volume;
        self->pending.flag = uint8_t(flag);
        self->pending.fadeOut = uint16_t(fadeOut);
        self->pending.state = 3;
        self->pending.soundId = soundId;
        self->pending.fadeIn = uint16_t(fadeIn);
        self->pending.startTime = m_snTimeInMilliseconds;
        self->pending.bank = bank;
        self->playTime = m_snTimeInMilliseconds;
    }
}

// 0x590570 (thiscall): the same, preloaded to start later
void __fastcall MusicStream_Preload(MusicStream* self, re::Edx, int32_t soundId, int32_t bank, float volume, int flag,
                                    int priority, int fadeIn, int fadeOut) {
    if (PriorityOf(*self) <= priority) {
        self->pending.priority = priority;
        self->pending.volume = volume;
        self->pending.flag = uint8_t(flag);
        self->pending.soundId = soundId;
        self->pending.fadeOut = uint16_t(fadeOut);
        self->pending.state = 2;
        self->pending.fadeIn = uint16_t(fadeIn);
        self->pending.startTime = m_snTimeInMilliseconds;
        self->pending.bank = bank;
    }
}

// 0x5905E0 (thiscall): start the preloaded sound (priority 0: whichever)
void __fastcall MusicStream_PlayPreloaded(MusicStream* self, re::Edx, int priority) {
    if ((priority == 0 || priority == self->current.priority) && self->current.state == 2) {
        StreamChannels_Play(streamChannels, self->channelIndex);
        self->current.state = 3;
        self->current.startTime = m_snTimeInMilliseconds;
        self->playTime = m_snTimeInMilliseconds;
    }
}

// what the stop functions share: fade out (half as long when a higher priority waits)
void StopCurrent(MusicStream* self, uint8_t* channel) {
    int fade = self->current.fadeOut;
    if (self->pending.state != 0 && self->current.priority < self->pending.priority) {
        fade >>= 1;
    }
    if (self->current.state == 2) {
        if (ChannelState(channel) == 7) {
            Channel_Fn5A6EA0(channel);
        }
    } else {
        Channel_Stop(channel, fade);
    }
    self->current.state = 1;
}

// 0x590620 (thiscall): go to a state (1 stops, 2 preloads, 3 plays the current request)
void __fastcall MusicStream_SetState(MusicStream* self, re::Edx, int state) {
    uint8_t* channel = ChannelOf(*self);
    switch (state) {
    case 0:
        self->current.state = 0;
        return;
    case 1:
        StopCurrent(self, channel);
        return;
    case 2:
        if (self->current.volume > 1.0f) {
            self->current.volume = 1.0f;
        }
        if (self->current.volume < 0.0f) {
            self->current.volume = 0.0f;
        }
        lastStreamVolume = 0.0f;
        Channel_Preload(channel, 0, self->current.soundId, self->current.volume, 1, self->current.flag, 0);
        At<int32_t>(channel, 0x94) = self->current.bank;
        At<uint8_t>(channel, 0x92) = 1;
        self->current.state = 2;
        return;
    case 3: {
        Channel_Fn5A6D20(channel, 0, 1);
        int mode;
        if (self->current.priority == 2) {
            // (the ambient stream: its current candidate, the type sign-extended from four bits)
            Channel_SetCandidate(channel, currentCandidate, int8_t(candidateTypeBits << 4) >> 4);
            mode = 3;
        } else if (self->current.priority == 3) {
            mode = 3;
        } else {
            mode = 1;
        }
        if (self->current.volume > 1.0f) {
            self->current.volume = 1.0f;
        }
        if (self->current.volume < 0.0f) {
            self->current.volume = 0.0f;
        }
        lastStreamVolume = 0.0f;
        Channel_Play(channel, 0, self->current.soundId, self->current.volume, mode, self->current.flag);
        At<int32_t>(channel, 0x94) = self->current.bank;
        if (self->current.fadeIn != 0) {
            Channel_FadeIn(channel, self->current.volume, realTimeMs, self->current.fadeIn);
        }
        self->current.startTime = m_snTimeInMilliseconds;
        self->current.state = 3;
        return;
    }
    default:
        return;
    }
}

// 0x591330 (thiscall): play a stream by name (200 ms in, 2 s out)
void __fastcall MusicStream_PlayByName(MusicStream* self, re::Edx, const char* name, float volume, int flag,
                                       int priority) {
    int32_t soundId = HashStreamName(name, 1);
    int32_t bank = StreamSounds_GetBank(streamSounds, soundId);
    Stream_Play(self, soundId, bank, volume, flag, priority, 200, 2000);
}

// 0x591380 (thiscall): preload a stream by name
void __fastcall MusicStream_PreloadByName(MusicStream* self, re::Edx, const char* name, float volume, int flag,
                                          int priority, int fadeIn, int fadeOut) {
    int32_t soundId = HashStreamName(name, 1);
    int32_t bank = StreamSounds_GetBank(streamSounds, soundId);
    Stream_Preload(self, soundId, bank, volume, flag, priority, fadeIn, fadeOut);
}

// 0x5913D0 (thiscall): stop what plays at that priority (0: whatever plays), dropping the
// queued request; fadeOut -1 keeps the request's own
void __fastcall MusicStream_Stop(MusicStream* self, re::Edx, int priority, int fadeOut) {
    if (priority != 0 && PriorityOf(*self) != priority) {
        return;
    }
    if (self->pending.state != 0) {
        self->pending.state = 0;
    }
    int32_t state = self->current.state;
    if (state == 3 || state == 2) {
        if (fadeOut != -1) {
            self->current.fadeOut = uint16_t(fadeOut);
        }
        StopCurrent(self, ChannelOf(*self));
    }
}

// 0x591480 (thiscall): the same with the request's fade
void __fastcall MusicStream_StopAny(MusicStream* self, re::Edx, int priority) {
    if (priority != 0 && PriorityOf(*self) != priority) {
        return;
    }
    if (self->pending.state != 0) {
        self->pending.state = 0;
    }
    int32_t state = self->current.state;
    if (state != 0 && state != 1) {
        StopCurrent(self, ChannelOf(*self));
    }
}

// 0x591520 (thiscall; its second half is at 0x5907E0): the per-frame update
void __fastcall MusicStream_Update(MusicStream* self) {
    if (flag20C5BE6 != 0 && musicDuringCutscenes == 0) {
        return;
    }
    uint8_t was = self->missionActive;
    if (was != MissionMgr_Fn6AA7E0(missionMgr)) {
        // (a mission starting stops what plays, unless it's a mission's own music)
        if (was == 0 && PriorityOf(*self) != 6) {
            Stream_StopAny(self, 0);
        }
        self->missionActive = self->missionActive == 0;
    }
    // (0x5907E0)
    if (ChannelState(ChannelOf(*self)) == 5) {
        return;
    }
    if (self->pending.state != 0) {
        int32_t state = self->current.state;
        if (state == 1) {
            if (ChannelState(ChannelOf(*self)) == 7) {
                Channel_Fn5A6EA0(ChannelOf(*self));
                return;
            }
            if (Channel_Fn58E060(ChannelOf(*self)) == 0) {
                void* channel = Stream_GetChannel(self);
                if (ChannelState(channel) != 5) {
                    Channel_Stop(Stream_GetChannel(self), 500);
                }
                return;
            }
        } else if (state == 2 || state == 3) {
            Stream_SetState(self, 1);
            return;
        }
        // the queued request becomes the current one
        self->current = self->pending;
        self->pending.state = 0;
        Stream_SetState(self, self->current.state);
        return;
    }
    if (self->current.state == 1) {
        if (Channel_Fn58E060(ChannelOf(*self)) != 0) {
            self->current.state = 0;
            return;
        }
        if (ChannelState(ChannelOf(*self)) != 5) {
            Channel_Stop(Stream_GetChannel(self), 500);
        }
    } else if (self->current.state == 3) {
        if (Channel_Fn58E060(ChannelOf(*self)) != 0 && m_snTimeInMilliseconds - self->current.startTime > 400) {
            self->current.state = 0;
        }
    }
}

// ---- the interactive music -----------------------------------------------------------------

// 0x58FF50: whether the interactive music may play (and the radios of the janitor's room
// and the gym, which follow their missions)
uint8_t __cdecl Music_IsAllowedImpl() {
    uint8_t result = 1;
    MissionSlot_IsActive(MissionMgr_Get0(missionMgr));
    uint8_t secondActive = MissionSlot_IsActive(MissionMgr_Get1(missionMgr));
    if (PriorityOf(stream) == 7) {
        if (IsTimePaused() != 0 && flag20C870A == 0) {
            missionMusicPaused = 1;
        } else {
            missionMusicPaused = 0;
        }
    }
    uint8_t gymMission = 0;       // [esp+8]
    uint8_t janitorMission = 0;   // [esp+9]
    if (currArea == 8) {
        janitorMission = MissionRunning(reinterpret_cast<const char*>(0x0091D388));   // "1_10"
        int on = janitorMission == 0;
        AmbienceEmitters_Enable(ambienceEmitters, HashName("JanitorRadio"), on);
        AmbienceEmitters_Enable(ambienceEmitters, HashName("JanitorRadioXMAS"), on);
    } else if (currArea == 0x1B) {
        static const uintptr_t kGymMissions[] = {
            0x0091D3E8,   // "2_R11_Chad"
            0x0091D3D8,   // "2_R11_Justin"
            0x0091D3C8,   // "2_R11_Parker"
            0x0091D3BC,   // "2_R11_Bryce"
            0x0091D3AC,   // "2_R11_Random"
            0x0090A368,   // "3_R09_P3"
            0x0091D3A8,   // "2_B"
            0x0091D3A0,   // "2_09"
        };
        for (uintptr_t name : kGymMissions) {
            if (MissionRunning(reinterpret_cast<const char*>(name)) != 0) {
                gymMission = 1;
                break;
            }
        }
        AmbienceEmitters_Enable(ambienceEmitters, HashName("ExceriseRadio"), gymMission == 0);
    }
    if (flag20C5BE6 != 0 && flag20C5BE9 == 0) {
        return 0;
    }
    if (interactiveMusicDisabled != 0) {
        return 0;
    }
    if (PriorityOf(stream) > 5) {
        return 0;
    }
    if (missionMusicPaused != 0) {
        return 0;
    }
    switch (currArea) {
    case 8:
        if (janitorMission == 0) {
            return 0;
        }
        break;
    case 0x1B:
        if (gymMission == 0) {
            return 0;
        }
        break;
    case 0x10: case 0x1A: case 0x1C: case 0x1D: case 0x1E: case 0x21: case 0x22: case 0x27:
    case 0x2E: case 0x32: case 0x37: case 0x38: case 0x39: case 0x3B: case 0x3C: case 0x3D:
        return 0;
    default:
        break;
    }
    if (secondActive == 0) {
        return result;
    }
    // (the carnival rides have their own music)
    uint8_t ride = MissionRunning("Coaster") != 0 || MissionRunning("Squid") != 0 ||
                   MissionRunning("FerrisWheel") != 0;
    if (ride) {
        result = 0;
    }
    return result;
}

// 0x590B70 (cdecl): the carnival's ambience emitter, on while the player is in its area
// (off during "2_G2" while the forced entry wants it)
void __cdecl Music_UpdateCarnival(void* emitter) {
    float volume = float(double(carnivalVolume) / 3.0);
    uint8_t on = 0;   // (the upper bytes of the game's argument are stack garbage)
    if (currArea == 0 && playerPed != nullptr) {
        int32_t index = Zones_FindByName(theZones, "AMB_CARNIVAL_AREA");
        void* area = nullptr;
        if (index != -1 && (triggerPool->flags[index] & 0x80) == 0) {
            area = triggerPool->objects + triggerPool->elemSize * index;
        }
        if (Area_Contains(area, PositionOf(playerPed)) != 0) {
            uint8_t running = 0;
            uint8_t* entry = forcedEntry == kNoEntry ? nullptr : EntryAt(forcedEntry);
            if (entry != nullptr && entry[0x1E] != 0) {
                running = MissionRunning(reinterpret_cast<const char*>(0x0091D3F4));   // "2_G2"
            }
            on = running == 0;
        }
    }
    Emitter_SetVolume(emitter, on, volume);
}

// 0x590950 (thiscall): the slot being crossfaded: when its transition has played (or at
// once when it isn't streamed), the stream goes on with the new sound
void __fastcall Music_UpdateSlotImpl(InteractiveMusic* self) {
    MusicSlot* slot = &self->slots[self->current];
    if ((slot->flags & 1) == 0) {
        return;
    }
    if (slot->transition[4] == 0) {
        if (self->waitUntil > m_snTimeInMilliseconds) {
            return;
        }
        Music_Fn58EEE0(self);
        if (curEntry != nullptr) {
            slot = &self->slots[self->current];
            Stream_Play(&stream, slot->soundTo, slot->bankTo, slot->volume, 1, 5, At<uint16_t>(curEntry, 4),
                        At<uint16_t>(curEntry, 6));
        }
        Music_EndSlot(self);
        self->slots[0].flags &= 0xFE;
        self->slots[1].flags &= 0xFE;
        self->slots[2].flags &= 0xFE;
        return;
    }
    if (slot->bankTo != At<int32_t>(ChannelOf(stream), 0x94)) {
        return;
    }
    Channel_Empty8A20A0(ChannelOf(stream), 1);
    if (curEntry != nullptr) {
        uint16_t fade;
        slot = &self->slots[self->current];
        if (slot->transition[5] != 0) {
            slot->volume = float(slot->transition[0x18] / 255.0);
            fade = At<uint16_t>(self->slots[self->current].transition, 0x12);
        } else {
            fade = At<uint16_t>(curEntry, 0x28 + curEntry[0x12] * 10);
        }
        float from = stream.current.volume;
        slot = &self->slots[self->current];
        if (slot->volume != from && fade != 0) {
            Channel_FadeTo(Stream_GetChannel(&stream), from, slot->volume, realTimeMs + 2500, fade);
        }
        Music_Fn58EEE0(self);
        curSound = self->slots[self->current].soundTo;
        stream.current.fadeIn = At<uint16_t>(curEntry, 4);
        stream.current.fadeOut = At<uint16_t>(curEntry, 6);
        stream.current.soundId = self->slots[self->current].soundTo;
        stream.current.volume = self->slots[self->current].volume;
        nextUpdate = At<uint16_t>(curEntry, 0x10) + m_snTimeInMilliseconds;
    }
    self->slots[0].flags &= 0xFE;
    self->slots[1].flags &= 0xFE;
    self->slots[2].flags &= 0xFE;
}

// 0x590C90: the fight stats the entries' conditions read (0xCF3A30, 0x68 bytes; last
// update's at 0xCF39C8): per faction (13) four bytes of two four-bit counters (+0 low:
// hostile to the player or an ally, high: after the player; +1 low: +0x136C set, high:
// hostile with +0x14A0; +2 low: hostile, high: the HUD shows its health; +3 low: hostile
// within 10 m), their totals (+0x34..+0x37), then +0x38 the hostiles' mean distance, +0x3C
// their mean health, +0x40 their mean weapon, +0x44 the lowest health shown, +0x48 the
// hostiles' mean threat, +0x4C the shown ones' threat, +0x50 the player's punishment,
// +0x54..+0x57 counts of peds with +0x1398 (all, within 20, 10, 5 m), +0x58..+0x64 times
void __cdecl Music_GatherStatsImpl() {
    std::memcpy(musicStatsPrev, musicStats, 0x68);
    std::memset(musicStats, 0, 0x68);
    uint8_t* player = playerPed;
    if (player == nullptr) {
        return;
    }
    uint8_t* hud = hudHealth;
    uint8_t* counts = musicStats;
    uint8_t* totals = musicStats + 0x34;
    float& meanDistance = At<float>(musicStats, 0x38);
    float& meanHealth = At<float>(musicStats, 0x3C);
    float& meanWeapon = At<float>(musicStats, 0x40);
    float& lowestHealth = At<float>(musicStats, 0x44);
    float& meanThreat = At<float>(musicStats, 0x48);
    float& shownThreat = At<float>(musicStats, 0x4C);
    float& punish = At<float>(musicStats, 0x50);

    lowestHealth = Clamp01(At<float>(player, 0x1CB8) / At<float>(player, 0x1CA4));
    shownThreat = Music_ModelThreat(At<int16_t>(At<uint8_t*>(player, 0x1D0), 0x10E));
    punish = float(At<int32_t>(player, 0x1D40)) / float(At<int32_t>(punishment, 0x38));
    punish = Clamp01(punish);
    totals[2] = uint8_t((totals[2] & 0xF) | 0x10);   // (the player's own bar)

    CPool<uint8_t>* pool = pedPool;
    for (int slot = pool->size; slot != 0;) {
        slot--;
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        uint8_t* ped = pool->objects + pool->elemSize * slot;
        if (ped == nullptr || ped == player || At<uint8_t>(ped, 0x1C98) != 0) {
            continue;
        }
        int32_t model = At<int16_t>(ped, 0x10E);
        if (model == ignoredModel0 || model == ignoredModel1 || model == ignoredModel2 || model == ignoredModel3) {
            continue;
        }
        uint8_t hostile = 0;
        uint8_t shown = 0;
        uint8_t* target = player;
        float threat = Music_ModelThreat(At<int16_t>(At<uint8_t*>(ped, 0x1D0), 0x10E));
        float health = Clamp01(At<float>(ped, 0x1CB8) / At<float>(ped, 0x1CA4));
        if (ped == At<uint8_t*>(hud, 0xAC) || ped == At<uint8_t*>(hud, 0xB0)) {
            shown = 1;
        }
        float distance;
        CVector delta;
        if (Ped_Fn473FB0(ped) == 0 || Ped_Fn473E40(ped) != 0) {
            distance = Vector_Magnitude2D(VectorSub(&delta, PositionOf(ped), PositionOf(player)));
        } else {
            if (Ped_GetTarget46ECE0(ped) == player) {
                IncHigh(counts[At<int32_t>(ped, 0x1310) * 4]);
                IncHigh(totals[0]);
                hostile = 1;
            } else if (Ped_GetTarget46ECE0(ped) != nullptr) {
                if (Ped_GetTarget46ECE0(ped) == At<uint8_t*>(hud, 0xAC) ||
                    Ped_GetTarget46ECE0(ped) == At<uint8_t*>(hud, 0xB0)) {
                    target = Ped_GetTarget46ECE0(ped);
                    hostile = 1;
                }
            }
            distance = Vector_Magnitude2D(VectorSub(&delta, PositionOf(ped), PositionOf(target)));
            if (hostile) {
                int32_t faction = At<int32_t>(ped, 0x1310);
                IncLow(counts[faction * 4]);
                IncLow(counts[faction * 4 + 2]);
                IncLow(totals[0]);
                IncLow(totals[2]);
                shown = 0;
                if (distance < 10.0f) {
                    IncLow(counts[At<int32_t>(ped, 0x1310) * 4 + 3]);
                    IncLow(totals[3]);
                }
            }
        }
        if (At<int32_t>(ped, 0x1398) != 0) {
            musicStats[0x54]++;
            if (distance < 5.0f) {
                musicStats[0x57]++;
            }
            if (distance < 10.0f) {
                musicStats[0x56]++;
            }
            if (distance < 20.0f) {
                musicStats[0x55]++;
            }
        }
        if (shown) {
            if (health < lowestHealth) {
                lowestHealth = health;
            }
            shownThreat = shownThreat + threat;
            IncHigh(counts[At<int32_t>(ped, 0x1310) * 4 + 2]);
            IncHigh(totals[2]);
        }
        if (hostile) {
            meanDistance = meanDistance + distance;
            if (At<int32_t>(ped, 0x14A0) != 0) {
                IncHigh(counts[At<int32_t>(ped, 0x1310) * 4 + 1]);
                IncHigh(totals[1]);
            }
            meanHealth = health + meanHealth;
            float weapon = 0.0f;
            uint8_t* info = Ped_GetModelInfo477DC0(ped);
            if (info != nullptr) {
                int32_t type;
                switch (*PedModelInfo_GetType(info, &type)) {
                case 0: weapon = 0.0f; break;
                case 1: weapon = 0.3f; break;
                case 2: weapon = 0.75f; break;
                case 3: weapon = 0.6f; break;
                case 4: weapon = 1.0f; break;
                default: break;
                }
                meanWeapon = meanWeapon + weapon;
            }
            meanThreat = meanThreat + threat;
        }
        if (At<int32_t>(ped, 0x136C) != 0) {
            int32_t f = At<int32_t>(ped, 0x1310);
            if (f < 13) {
                IncLow(counts[f * 4 + 1]);
                IncLow(totals[1]);
            }
        }
    }
    uint32_t now = m_snTimeInMilliseconds;
    if ((totals[3] & 0xF) == 0) {
        At<uint32_t>(musicStats, 0x60) = now;
    } else {
        At<uint32_t>(musicStats, 0x5C) = now;
    }
    uint8_t hostiles = totals[2] & 0xF;
    if (hostiles == 0) {
        At<uint32_t>(musicStats, 0x64) = now;
    } else {
        At<uint32_t>(musicStats, 0x58) = now;
    }
    if (hostiles != 0) {
        float n = float(int32_t(hostiles));
        meanDistance = meanDistance / n;
        meanHealth = meanHealth / n;
        meanWeapon = meanWeapon / n;
        meanThreat = meanThreat / n;
    }
    uint8_t shownCount = totals[2] >> 4;
    if (shownCount != 0) {
        shownThreat = shownThreat / float(int32_t(shownCount));
    }
}

// 0x591590 (thiscall): crossfade to `to` in the next slot (the transition from `from`);
// no transition: the same sound goes on (1), else nothing (0)
uint8_t __fastcall Music_StartSlotImpl(InteractiveMusic* self, re::Edx, MusicLevelInfo* info, int32_t from, int32_t to,
                                       int typeFrom, int typeTo) {
    if (PriorityOf(stream) != 5) {
        return 0;
    }
    int32_t next = self->current;
    if (self->slots[next].flags & 1) {
        next = self->current + 1;
        if (next >= 3) {
            next = 0;
        }
    }
    uint8_t* transition = Music_FindTransition(self, from, to, typeFrom, typeTo);
    self->slots[next].transition = transition;
    if (transition == nullptr) {
        if (from != to) {
            return 0;
        }
        Channel_Empty8A20A0(ChannelOf(stream), 1);
        return 1;
    }
    self->current = next;
    self->slots[next].flags |= 1;
    MusicSlot* slot = &self->slots[self->current];
    slot->flags = uint8_t(slot->flags ^ ((uint8_t(slot->transition[4] * 2) ^ slot->flags) & 2));
    self->slots[self->current].soundFrom = from;
    self->slots[self->current].bankFrom = StreamSounds_GetBank(streamSounds, from);
    self->slots[self->current].soundTo = to;
    self->slots[self->current].bankTo = StreamSounds_GetBank(streamSounds, to);
    self->slots[self->current].typeFrom = uint8_t(typeFrom);
    self->slots[self->current].typeTo = uint8_t(typeTo);
    self->slots[self->current].soundMid = 0;
    self->slots[self->current].bankMid = 0;
    slot = &self->slots[self->current];
    std::memcpy(&slot->volume, info, 0xC);
    uint8_t* t = slot->transition;
    if (t[4] == 0) {
        // (not streamed: the stream fades out, the next one starts after the delay)
        uint8_t* found = Music_FindTransition58E760(&music, from, to, typeFrom, typeTo);
        int fade = -1;
        if (found != nullptr && At<uint16_t>(found, 0x12) != 0) {
            fade = At<uint16_t>(found, 0x12);
        }
        Stream_Stop(&stream, 5, fade);
        self->waitUntil = At<uint16_t>(self->slots[self->current].transition, 0x10) + m_snTimeInMilliseconds;
        self->unknown94 = 0;
        return 1;
    }
    if (t[2] != 0) {
        slot = &self->slots[self->current];
        int32_t r = RandomInt(0, slot->transition[2]);
        slot->soundMid = soundTable[r + At<uint16_t>(slot->transition, 0x10)];
        self->slots[self->current].bankMid = StreamSounds_GetBank(streamSounds, self->slots[self->current].soundMid);
        slot = &self->slots[self->current];
        Channel_Queue5A6BC0(ChannelOf(stream), slot->soundMid, 1, 1, slot->transition[9], 1);
        slot = &self->slots[self->current];
        Channel_Queue5A6BC0(ChannelOf(stream), slot->soundTo, 1, 0, 0, slot->transition[0xA]);
        self->unknown94 = 0;
        return 1;
    }
    Channel_Queue5A6BC0(ChannelOf(stream), slot->soundTo, 1, 1, t[9], t[0xA]);
    self->unknown94 = 0;
    return 1;
}

// 0x591900: a mission's music forced into the forced entry (its first level)
void __cdecl Music_ForceEntry(int32_t soundId, float volume, int fadeIn, int fadeOut, int missionType) {
    if (stream.pending.state != 0) {
        stream.pending.state = 0;
    }
    int32_t state = stream.current.state;
    if (state == 3 || state == 2) {
        StopCurrent(&stream, ChannelOf(stream));
    }
    uint8_t* entry = forcedEntry == kNoEntry ? nullptr : EntryAt(forcedEntry);
    if (missionMusicType == 0x1B) {
        missionMusicType = 0x1F;
    }
    if (entry == nullptr) {
        return;
    }
    interactiveMusicDisabled = 0;
    if (forcedEntrySet == 0) {
        forcedPriority = entry[1];
    } else {
        entry[1] = uint8_t(forcedPriority);
    }
    soundTable[At<uint16_t>(entry, 0x24)] = soundId;
    entry[0x23] = uint8_t(int32_t(double(volume) * 255.0));   // (truncated)
    At<uint16_t>(entry, 4) = uint16_t(fadeIn);
    At<uint16_t>(entry, 6) = uint16_t(fadeOut);
    if (uint8_t(missionType) != 0) {
        missionMusicType = 0x1B;
    }
    soundTable[At<uint16_t>(entry, 0x38)] = 0;
    soundTable[At<uint16_t>(entry, 0x2E)] = 0;
    forcedEntrySet = 1;
}

// 0x591A70: the forced music stopped (returns the slot's address in eax: unused)
void __cdecl Music_StopForced(int fadeOut) {
    Stream_Stop(&stream, 0, fadeOut);
    forcedEntrySet = 0;
    if (missionMusicType == 0x1B) {
        missionMusicType = 0x1F;
    }
    curEntry = nullptr;
    Music_ResetEntries();
    if (music.slots[music.current].flags & 1) {
        Channel_Empty8A20A0(ChannelOf(stream), 1);
        music.slots[music.current].flags &= 0xFE;
    }
}

// what the update does to drop the current entry
void DropEntry(int fadeOut) {
    if (PriorityOf(stream) == 5) {
        Stream_Stop(&stream, 5, fadeOut);
    }
    curEntry = nullptr;
    Music_ResetEntries();
    Music_EndSlot(&music);
}

// 0x591B00: the interactive music's update: the best entry whose condition holds (the
// lowest priority byte) takes over; else the current one changes level or picks another
// of its level's sounds
void __cdecl Music_UpdateImpl() {
    int32_t from = 0;                  // [esp+0x18]
    int32_t to = 0;                    // [esp+0x14]
    re::Ptr32<uint8_t> transition = nullptr;     // [esp+0x1C]
    Music_UpdateSlot(&music);
    if (PriorityOf(stream) != 5 && curEntry != nullptr) {
        curEntry = nullptr;
    }
    if (Music_IsAllowed() == 0) {
        if (curEntry != nullptr) {
            DropEntry(-1);
        }
        currentGoKartRace = -1;
        nextUpdate = m_snTimeInMilliseconds + 1100;
        return;
    }
    if (m_snTimeInMilliseconds < nextUpdate) {
        return;
    }
    Music_GatherStats();
    uint8_t* best = nullptr;   // ebp
    uint8_t finished = 0;
    if (curEntry != nullptr) {
        from = curSound;
        if (entryConditions[curEntry[0]](curEntry) == 0) {
            finished = 1;
        }
    }
    if (numEntries > 0) {
        for (int i = 0; i < numEntries; i++) {
            uint8_t* entry = EntryAt(i);
            uint8_t holds = 0;
            if (curEntry == nullptr || curEntry[0] != entry[0]) {
                holds = entryConditions[entry[0]](entry);
                entry = EntryAt(i);
            }
            if ((curEntry == nullptr || finished || entry[1] < curEntry[1]) && (best == nullptr || entry[1] < best[1]) &&
                holds) {
                best = entry;
            }
        }
    }
    if (best != nullptr) {
        // a new entry
        MusicSlot* slot = &music.slots[music.current];
        uint8_t fromType = kNoEntry;
        if (slot->flags & 1) {
            fromType = slot->typeFrom;
        } else if (curEntry != nullptr) {
            fromType = curEntry[0];
        }
        int32_t sound;
        if (fromType == best[0]) {
            to = from;
            Music_EndSlot(&music);
            sound = from;
        } else {
            to = Music_PickSound(&music, &transition, 1, from, fromType, best[0], best[0x13]);
            sound = to;
            if (to == 0) {
                to = Music_EntrySound(best);
                sound = to;
            }
        }
        if (sound != 0) {
            lastChange = 1;
            best[0x12] = best[0x13];
        } else {
            best = nullptr;
        }
    } else if (curEntry != nullptr && finished == 0) {
        // the current entry: its wanted level, or another sound of its level
        uint8_t* entry = curEntry;
        if (entry[0x13] != entry[0x12]) {
            to = Music_PickSound(&music, &transition, 2, from, entry[0], entry[0], entry[0x13]);
            if (to == 0) {
                to = Music_EntrySound(curEntry);
            }
            if (to != 0) {
                if (curEntry[0x13] == curEntry[0x12]) {
                    to = 0;
                } else {
                    lastChange = 2;
                    best = curEntry;
                    best[0x12] = best[0x13];
                }
            }
        }
        // the stream's markers (a byte: past which the level may change)
        uint8_t marker = 0;
        if (stream.hasChannel) {
            uint8_t* channel = channels[stream.channelIndex];
            if (channel != nullptr && reinterpret_cast<uintptr_t>(channel) + 0x54 != 0) {
                bool playing = At<int32_t>(channel, 0x84) == 5;
                if (!playing) {
                    int32_t state = At<int32_t>(Stream_GetChannel(&stream), 0x84);
                    playing = state == 6 || state == 7;
                }
                if (playing && At<int32_t>(channel, 0x5C) != -1) {
                    PushMemoryTag(0x2E);
                    int32_t value = 0;
                    if (Fn5AF560(At<int32_t>(static_cast<uint8_t*>(Stream_GetChannel(&stream)) + 0x54, 8), &value) !=
                        0) {
                        marker = uint8_t(value);
                    }
                    PopMemoryTag();
                }
            }
        }
        entry = curEntry;
        if (to == 0 && entry[0x22 + entry[0x12] * 10] > 1 &&
            ((levelRandom != 0 && levelRandom <= marker) ||
             (levelTime != 0 && levelTime <= m_snTimeInMilliseconds)) &&
            (music.slots[music.current].flags & 1) == 0) {
            to = Music_PickRandom(&music, &transition, from, entry[0], entry[0x12]);
            if (to == 0) {
                to = Music_EntryRandomSound(curEntry, from);
            }
            if (to != 0) {
                lastChange = 3;
                best = curEntry;
            }
        }
    }
    if (stopRequested != 0) {
        stopRequested = 0;
        if ((curEntry != nullptr && finished) || best != nullptr) {
            if (Channel_Fn5A6D80(channels[0]) != 0) {
                Channel_Fn5A6EA0(channels[0]);
            }
            if (curEntry != nullptr) {
                DropEntry(0);
            }
            currentGoKartRace = -1;
            nextUpdate = m_snTimeInMilliseconds + 500;
            best = nullptr;
        } else if (Channel_Fn5A6D80(channels[0]) != 0) {
            uint8_t* channel = channels[0];
            Fader_Start59E5B0(channel + 0x14, channel + 0x20, 0.0f, stream.current.volume, realTimeMs, 1000);
            channel[0x30] = 0;
            Channel_Fn5A6E70(channels[0]);
        }
    }
    if (interactiveMusicState <= 0 && best != nullptr) {
        // start the best entry's level
        uint8_t* level = best + 0x20 + best[0x12] * 10;
        int typeFrom = kNoEntry;   // (a byte in a dword whose upper bytes are stack garbage)
        if (curEntry != nullptr) {
            typeFrom = curEntry[0];
        }
        curEntry = best;
        MusicLevelInfo info;
        info.volume = float(level[3] / 255.0);
        info.time = 0;
        uint8_t random = 0;
        if (level[2] > 1) {
            if (transition != nullptr && transition[6] != 0) {
                if (At<uint16_t>(transition, 0x14) != 0) {
                    info.time = At<uint16_t>(transition, 0x14) + m_snTimeInMilliseconds;
                }
                if (transition[0x17] != 0) {
                    random = uint8_t(RandomInt(transition[0x16], transition[0x17] + 1));
                }
            } else {
                if (At<uint16_t>(level, 6) != 0) {
                    info.time = At<uint16_t>(level, 6) + m_snTimeInMilliseconds;
                }
                if (level[1] != 0) {
                    random = uint8_t(RandomInt(level[0], level[1] + 1));
                }
            }
        }
        info.random = random;
        info.pad[0] = info.pad[1] = info.pad[2] = 0;
        if (Music_StartSlot(&music, &info, from, to, typeFrom, best[0]) != 0) {
            musicBusy = 0;
            return;
        }
        levelTime = uint32_t(info.time);
        levelRandom = random;
        uint8_t* found = Music_FindTransition58E760(&music, from, to, typeFrom, best[0]);
        if (found != nullptr && At<uint16_t>(found, 0x12) != 0) {
            stream.current.fadeOut = At<uint16_t>(found, 0x12);
        }
        Stream_Play(&stream, to, StreamSounds_GetBank(streamSounds, to), info.volume, 1, 5, At<uint16_t>(best, 4),
                    At<uint16_t>(best, 6));
        nextUpdate = At<uint16_t>(best, 0x10) + m_snTimeInMilliseconds;
        Music_EndSlot(&music);
        curSound = to;
        musicBusy = 0;
        return;
    }
    if (interactiveMusicState > 0) {
        interactiveMusicState--;
    }
    if (curEntry != nullptr) {
        if (finished) {
            uint8_t* found = Music_FindTransition58E760(&music, curSound, 0, curEntry[0], kNoEntry);
            int fade = -1;
            if (found != nullptr && At<uint16_t>(found, 0x12) != 0) {
                fade = At<uint16_t>(found, 0x12);
            }
            Stream_Stop(&stream, 5, fade);
            curEntry = nullptr;
            Music_ResetEntries();
            Music_EndSlot(&music);
            nextUpdate = m_snTimeInMilliseconds + 1100;
            musicBusy = 0;
            return;
        }
        if (curEntry[0x1D] != 0) {
            float streamVolume = stream.current.state != 0 ? stream.current.volume : 0.0f;
            float volume = float(double(float(curEntry[0x1C]) * streamVolume) / 255.0);
            Channel_SetVolume5A6ED0(ChannelOf(stream), volume, -1);
        }
    }
    musicBusy = 0;
}

// 0x5922B0: a level of the forced entry set (the running-low music forced first)
void __cdecl Music_SetForcedLevel(int level, int32_t soundId, float volume) {
    if (forcedEntry == kNoEntry) {
        return;
    }
    uint8_t* entry = EntryAt(forcedEntry);
    if (entry == nullptr) {
        return;
    }
    interactiveMusicDisabled = 0;
    if (forcedEntrySet == 0) {
        Music_ForceEntryFn(HashStreamName("MS_RunningLow.rsm", 1), 0.25f, 1000, 1000, 0);
    }
    int l = uint8_t(level);
    soundTable[At<uint16_t>(entry, 0x24 + l * 10)] = soundId;
    entry[0x23 + l * 10] = uint8_t(int32_t(double(volume) * 255.0));   // (truncated)
    if (soundTable[At<uint16_t>(entry, 0x38)] != 0) {
        entry[1] = 0xB;
    } else {
        entry[1] = uint8_t(forcedPriority);
    }
}

// ---- registration -----------------------------------------------------------------------------

#define MUSICSTREAM_REVERSED(name, address, fn)                                                          \
    static void* RE_CONCAT(musicstream_original_, __LINE__);                                              \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(musicstream_original_, __LINE__), \
             re::Mode::Original)

MUSICSTREAM_REVERSED("InteractiveMusic::IsAllowed", 0x0058FF50, &Music_IsAllowedImpl);
MUSICSTREAM_REVERSED("MusicStream::Play", 0x00590470, &MusicStream_Play);
MUSICSTREAM_REVERSED("MusicStream::Preload", 0x00590570, &MusicStream_Preload);
MUSICSTREAM_REVERSED("MusicStream::PlayPreloaded", 0x005905E0, &MusicStream_PlayPreloaded);
MUSICSTREAM_REVERSED("MusicStream::SetState", 0x00590620, &MusicStream_SetState);
MUSICSTREAM_REVERSED("InteractiveMusic::UpdateSlot", 0x00590950, &Music_UpdateSlotImpl);
MUSICSTREAM_REVERSED("InteractiveMusic::UpdateCarnival", 0x00590B70, &Music_UpdateCarnival);
MUSICSTREAM_REVERSED("InteractiveMusic::GatherStats", 0x00590C90, &Music_GatherStatsImpl);
MUSICSTREAM_REVERSED("MusicStream::PlayByName", 0x00591330, &MusicStream_PlayByName);
MUSICSTREAM_REVERSED("MusicStream::PreloadByName", 0x00591380, &MusicStream_PreloadByName);
MUSICSTREAM_REVERSED("MusicStream::Stop", 0x005913D0, &MusicStream_Stop);
MUSICSTREAM_REVERSED("MusicStream::StopAny", 0x00591480, &MusicStream_StopAny);
MUSICSTREAM_REVERSED("MusicStream::Update", 0x00591520, &MusicStream_Update);
MUSICSTREAM_REVERSED("InteractiveMusic::StartSlot", 0x00591590, &Music_StartSlotImpl);
MUSICSTREAM_REVERSED("InteractiveMusic::ForceEntry", 0x00591900, &Music_ForceEntry);
MUSICSTREAM_REVERSED("InteractiveMusic::StopForced", 0x00591A70, &Music_StopForced);
MUSICSTREAM_REVERSED("InteractiveMusic::Update", 0x00591B00, &Music_UpdateImpl);
MUSICSTREAM_REVERSED("InteractiveMusic::SetForcedLevel", 0x005922B0, &Music_SetForcedLevel);

} // namespace
