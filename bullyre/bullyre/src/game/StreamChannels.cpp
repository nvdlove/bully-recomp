// The streamed-audio channels (0x5A5FF0..0x5A7DC9): the channel set at 0xCF4284 (+0 the
// speech channel's state, +2 paused, +4 sixteen channel pointers, +0x4C ready), the
// channels themselves (0x1A0 bytes each, an array at *0xCF70CC with its count at
// 0xCF70D0), the trigger loader at *0xCF70D4 (the playlists: 0xAC-byte entries after a
// count) with its hash tables (sorted {hash, a, b} records searched by bsearch), and the
// speech library's setup.
//
// A channel: +0 state (0/1/8 active, 2 idle, 3/4 paused, 5 loading, 6 preloading, 7
// ready), +8 its index, +0x14 a fader (+0x14 ms, +0x18 volume, +0x1C), +0x20 another,
// +0x30 fading out, +0x34 last volume, +0x38 positional (+0x3C distance, +0x40 entity
// reference, +0x44 position), +0x54/+0x58/+0x5C the sound engine's handles, +0x60,
// +0x64 the read buffer, +0x68 the file, +0x6C an OVERLAPPED, +0x80 entry, +0x84 load
// step, +0x88, +0x8C offset, +0x90 play, +0x91, +0x92 hold, +0x99 name, +0x19C hash.

#include <cmath>
#include <cstdint>
#include <cstring>

#include "reversed.h"
#include "sysabi.h"

#include "game/Crt.h"
namespace {

struct CVector {
    float x, y, z;
};

using re::At;

inline uint8_t* const TheSpeechLib = reinterpret_cast<uint8_t*>(0x00CF4E10);
inline void* const TheMissions = reinterpret_cast<void*>(0x020C3CA0);
inline void* const TheSounds = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const speechHistory = reinterpret_cast<void*>(0x0208B9F0);
inline void* const playlistSettings = reinterpret_cast<void*>(0x00A45A30);
GAME_ARRAY(uint8_t, speechSlotsBase, 0x00CF458C);        // the speech slots (0x5C each)
GAME_VAR(int32_t, speechCurrentSlot, 0x00CF4644);
GAME_VAR(uint8_t*, channelArray, 0x00CF70CC);
GAME_VAR(int32_t, channelCount, 0x00CF70D0);
GAME_VAR(int32_t, channelArrayUnknown, 0x00CF70C8);
GAME_VAR(uint8_t*, triggerLoader, 0x00CF70D4);
GAME_VAR(uint32_t, m_snTimeInMilliseconds, 0x00C1A9B4);
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(int32_t, streamsLastArea, 0x00CF60D8);
GAME_VAR(int32_t, streamsResumeTime, 0x00CF3AE8);
GAME_VAR(uint8_t, streamsResumePending, 0x00CF5DC8);
GAME_VAR(uint8_t, speechBlockedA, 0x020C5BE6);
GAME_VAR(uint8_t, speechBlockedB, 0x020C5BE7);
GAME_VAR(uint8_t, musicKeepsPlaying, 0x00CF3DA4);
GAME_VAR(uint8_t, speechMuted, 0x00CF3DD5);
GAME_VAR(uint8_t, speechDebugNames, 0x00CF3DD9);
GAME_VAR(uint8_t, speechSubtitles, 0x00CF4429);
GAME_VAR(uint8_t, disableChannel1, 0x00CF3BF3);
GAME_VAR(uint8_t, disableChannel2, 0x00CF3BF2);
GAME_VAR(uint8_t, disableChannel0, 0x00CF3BF1);
GAME_VAR(uint8_t*, ambienceObject, 0x00CF4374);
GAME_VAR(void*, streamSounds, 0x00CF70D4);
GAME_VAR(int32_t, fadeDefault, 0x00A14A50);
GAME_VAR(uint32_t, playlistSpeech, 0x00AE80CC);
GAME_VAR(uint32_t, playlistMusic, 0x00AE80D0);
GAME_VAR(uint32_t, playlistCuts, 0x00AE80D4);
GAME_VAR(uint32_t, playlistAmbs, 0x00AE80D8);
inline char* const playlistSpeechPath = reinterpret_cast<char*>(0x00CF6FC8);
inline char* const playlistMusicPath = reinterpret_cast<char*>(0x00CF6EC8);
inline char* const playlistCutsPath = reinterpret_cast<char*>(0x00CF6DC8);
inline char* const playlistAmbsPath = reinterpret_cast<char*>(0x00CF6CC8);

// the imports, read at call time
using CreateFileAFn = uintptr_t(__stdcall*)(const char*, uint32_t, uint32_t, void*, uint32_t, uint32_t, void*);
using ReadFileFn = int(__stdcall*)(uintptr_t, void*, uint32_t, uint32_t*, void*);
using GetLastErrorFn = uint32_t(__stdcall*)();
using GetOverlappedResultFn = int(__stdcall*)(uintptr_t, void*, uint32_t*, int);
using CreateEventAFn = uintptr_t(__stdcall*)(void*, int, int, const char*);
using SleepFn = void(__stdcall*)(uint32_t);
inline CreateFileAFn ImportCreateFileA() { return re::GameFnAt<CreateFileAFn>(uintptr_t(0x008FC064)); }
inline ReadFileFn ImportReadFile() { return re::GameFnAt<ReadFileFn>(uintptr_t(0x008FC06C)); }
inline GetLastErrorFn ImportGetLastError() { return re::GameFnAt<GetLastErrorFn>(uintptr_t(0x008FC060)); }
inline GetOverlappedResultFn ImportGetOverlappedResult() {
    return re::GameFnAt<GetOverlappedResultFn>(uintptr_t(0x008FC090));
}
inline CreateEventAFn ImportCreateEventA() { return re::GameFnAt<CreateEventAFn>(uintptr_t(0x008FC08C)); }
inline SleepFn ImportSleep() { return re::GameFnAt<SleepFn>(uintptr_t(0x008FC16C)); }

inline void* __cdecl Memset(void* p, int value, uint32_t size) { return crt::Memset(p, value, (size_t)size); }
GAME_FN(void*, __cdecl, New, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __stdcall, ConstructArray, 0x00401240, void* p, uint32_t size, int count, void* ctor);
inline int __cdecl Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
inline int __cdecl Strncmp(const char* a, const char* b, uint32_t n) { return crt::Strnicmp(a, b, (size_t)n); }
inline char* __cdecl StrRChr(const char* s, int c) { return crt::Strrchr(s, c); }
inline void* __cdecl Bsearch(const void* key, const void* base, uint32_t count, uint32_t size, void* compare) { return crt::Bsearch(key, base, (size_t)count, (size_t)size, (int(__cdecl*)(const void*, const void*))compare); }
GAME_FN(const char*, __cdecl, FixPath, 0x00829900, const char* path);
GAME_FN(void*, __cdecl, File_Open, 0x0042D260, const char* path, const char* mode, int);
GAME_FN(int, __cdecl, File_Read, 0x0042D2A0, void* file, void* buffer, int size);
GAME_FN(int, __cdecl, File_Close, 0x0042D450, void* file);
GAME_FN(void*, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* self, const char* name);
GAME_FN(void, __thiscall, NameKey_Destruct, 0x0049A650, uint32_t* self);
GAME_FN(int32_t, __thiscall, Missions_Find, 0x006A9FD0, void* missions, uint32_t* key);
GAME_FN(uint8_t, __thiscall, Missions_Fn6AA940, 0x006AA940, void* missions, int32_t mission);
GAME_FN(void, __thiscall, SetRef, 0x005D7F10, void* ref, void* entity);
GAME_FN(CVector*, __cdecl, GetCameraPos, 0x00595630, CVector* out);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(int, __cdecl, SFX_Fn59E3E0, 0x0059E3E0, float distance, int);
GAME_FN(float, __cdecl, SFX_Fn59E4F0, 0x0059E4F0, float volume, int ms);
GAME_FN(int, __thiscall, Sounds_Fn59B740, 0x0059B740, void* sounds, const CVector* position, uintptr_t);
GAME_FN(void, __thiscall, Fader_Start, 0x0059E5B0, void* fader, void* target, float from, float to, int, int);
GAME_FN(void, __thiscall, Fader_Start59E6B0, 0x0059E6B0, void* fader, float from, float to, int, int);
GAME_FN(void, __thiscall, Fader_Update, 0x0059E710, void* fader);
GAME_FN(void, __thiscall, MusicLayer_FadeTo, 0x0058E3A0, void* channel, float to, int, int);
GAME_FN(uint8_t, __thiscall, Channel_IsLoadedIdle, 0x00593D20, void* channel);
GAME_FN(uint8_t, __cdecl, Music_Fn58FD80, 0x0058FD80);
GAME_FN(uint8_t, __thiscall, Settings_Fn50DF50, 0x0050DF50, void* settings);
GAME_FN(void, __cdecl, Sound_Fn5AFA90, 0x005AFA90);
GAME_FN(void, __thiscall, Screamer_Fn8283D0, 0x008283D0, void* sounds);
GAME_FN(void, __cdecl, Sound_Lock, 0x005EF0A0, int);
GAME_FN(void, __cdecl, Sound_Unlock, 0x005EF100);
GAME_FN(uint8_t, __thiscall, History_Fn68E200, 0x0068E200, void* history, int32_t slot, int32_t line, int32_t* entry,
        int32_t* offset, int32_t* extra);
// the sound engine (0x5AF...)
GAME_FN(int32_t, __cdecl, Engine_Fn5AF150, 0x005AF150, void* data, int32_t size, int, uint32_t flags);
GAME_FN(void, __cdecl, Engine_Fn5AF1D0, 0x005AF1D0, int32_t handle);
GAME_FN(int32_t, __cdecl, Engine_Fn5AF210, 0x005AF210, int32_t handle);
GAME_FN(int32_t, __cdecl, Engine_Fn5AF2A0, 0x005AF2A0, void* description);
GAME_FN(void, __cdecl, Engine_Fn5AF2F0, 0x005AF2F0, int32_t handle);
GAME_FN(void, __cdecl, Engine_Fn5AF330, 0x005AF330, int32_t handle, int32_t* state);
GAME_FN(int32_t, __cdecl, Engine_Fn5AF340, 0x005AF340, int32_t handle);
GAME_FN(int32_t, __cdecl, Engine_Fn5AF3B0, 0x005AF3B0, int32_t handle, int);
GAME_FN(void, __cdecl, Engine_Fn5AF420, 0x005AF420, int32_t bank, int32_t handle, int);
GAME_FN(void, __cdecl, Engine_Fn5AF470, 0x005AF470, int32_t bank, int32_t handle);
GAME_FN(uint8_t, __cdecl, Engine_Fn5AF510, 0x005AF510, int32_t handle);
GAME_FN(uint8_t, __cdecl, Engine_Fn5AF550, 0x005AF550, int32_t handle, uint32_t* state);
GAME_FN(void, __cdecl, Engine_Fn5AF570, 0x005AF570, int32_t handle, int, int);
GAME_FN(void, __cdecl, Engine_Pause, 0x005AF5C0, int32_t handle, int pause);
GAME_FN(void, __cdecl, Engine_Fn5AF660, 0x005AF660, int32_t handle, float);
GAME_FN(void, __cdecl, Engine_SetPan, 0x005AF6B0, int32_t handle, float pan);
GAME_FN(void, __cdecl, Engine_SetVolume, 0x005AF760, int32_t handle, int volume, int);
GAME_FN(void, __cdecl, Engine_SetVolumePan, 0x005AF7B0, int32_t handle, int volume, int pan, int);
GAME_FN(int32_t, __cdecl, Engine_Fn5AF9A0, 0x005AF9A0, int32_t handle);
// the speech library's
GAME_FN(uint8_t, __thiscall, SpeechLib_Load, 0x0059F210, void* lib, const char* path);
GAME_FN(void, __thiscall, SpeechLib_Fn59F440, 0x0059F440, void* lib);
GAME_FN(void, __thiscall, SpeechLib_Fn59F8A0, 0x0059F8A0, void* lib);
GAME_FN(void, __thiscall, SpeechLib_Fn59E820, 0x0059E820, void* lib);
GAME_FN(void, __thiscall, SpeechSlot_SetState, 0x005A5060, void* slot, int32_t state);
// this file's
GAME_FN(uint8_t, __thiscall, Fader_IsActive, 0x005A66D0, void* fader);
GAME_FN(uint8_t, __thiscall, TriggerLoader_Add, 0x005A66F0, void* loader, const char* name, int noTable);
GAME_FN(uint8_t, __thiscall, TriggerLoader_FindByName, 0x005A6790, void* loader, const char* name, int32_t* index,
        int32_t* a, int32_t* b);
GAME_FN(uint8_t, __thiscall, TriggerLoader_Find, 0x005A6800, void* loader, uint32_t hash, int32_t* index,
        int32_t* a, int32_t* b);
GAME_FN(int32_t, __thiscall, TriggerLoader_GetEntry, 0x005A6860, void* loader, int32_t index, int, char* name);
GAME_FN(uint8_t*, __cdecl, GetStreamChannel, 0x005A68A0, int index);
GAME_FN(uint8_t, __cdecl, TriggerLoader_AddGlobal, 0x005A68C0, const char* name, int noTable);
GAME_FN(void, __cdecl, StreamChannels_OpenFiles, 0x005A68E0);
GAME_FN(void, __stdcall, Playlist_Handle, 0x005A6B10, const char* path, uint32_t* out);
GAME_FN(void, __thiscall, Channel_Release, 0x005A6C40, void* channel);
GAME_FN(int32_t, __thiscall, Channel_IsActive, 0x005A6D60, void* channel);
GAME_FN(int32_t, __thiscall, Channel_IsPaused, 0x005A6D80, void* channel);
GAME_FN(int32_t, __thiscall, Channel_IsReady, 0x005A6DA0, void* channel);
GAME_FN(void, __thiscall, Channel_Play, 0x005A6DB0, void* channel);
GAME_FN(void, __thiscall, Channel_Pause, 0x005A6E40, void* channel);
GAME_FN(void, __thiscall, Channel_Resume, 0x005A6E70, void* channel);
GAME_FN(void, __thiscall, Channel_Reset, 0x005A6EA0, void* channel);
GAME_FN(void, __thiscall, Channel_SetVolume, 0x005A6ED0, void* channel, float volume, int ms);
GAME_FN(void, __cdecl, StreamChannels_Allocate, 0x005A7200, int count);
GAME_FN(void, __thiscall, Channel_SetLoadStep, 0x005A72B0, void* channel, int32_t step);
GAME_FN(void, __thiscall, Channel_UpdateLoading, 0x005A74F0, void* channel);
GAME_FN(uint8_t, __thiscall, Channel_Preload, 0x005A7880, void* channel, const char* name, uint32_t hash,
        float volume, int ms, int, int useSpeech);
GAME_FN(void, __thiscall, Channel_Update, 0x005A7B70, void* channel);
GAME_FN(void*, __thiscall, HashTable_Construct, 0x005A7C00, void* table, void* file);
GAME_FN(void*, __cdecl, HashTable_Load, 0x005A7C50, const char* path);
GAME_FN(uint32_t, __cdecl, HashStreamName, 0x005A7D60, const char* name, int);
GAME_FN(uint8_t, __thiscall, HashTable_Find, 0x005A7DD0, void* table, uint32_t hash, int32_t* a, int32_t* b);
GAME_FN(void, __thiscall, StreamChannels_Refresh, 0x005A64D0, void* self);
GAME_FN(uint8_t, __thiscall, StreamChannels_LoadPlaylist, 0x005A6570, void* self, const char* name, int noTable);

inline uint8_t* ChannelAt(uint8_t* self, int index) {
    return At<uint8_t*>(self, 4 + index * 4);
}

// ---- the speech library's setup ------------------------------------------------------------

// 0x5A5FF0: the speech library's callbacks and database
void __cdecl Speech_LoadDatabase() {
    char path[0x80];
    std::memcpy(path, reinterpret_cast<const void*>(0x009219DC), 0x1A);   // "audio\CONFIG\SpeechDB.bin"
    At<uint32_t>(TheSpeechLib, 0x4B0) = 0x005A25A0;
    At<uint32_t>(TheSpeechLib, 0x4AC) = 0x005A0A40;
    At<uint32_t>(TheSpeechLib, 0x4B4) = 0x005A5DF0;
    At<uint32_t>(TheSpeechLib, 0x4A8) = 0x005A0D80;
    At<uint32_t>(TheSpeechLib, 0x4C0) = 0x005A0D20;
    At<uint32_t>(TheSpeechLib, 0x4B8) = 0x005A5CD0;
    uint8_t loaded = SpeechLib_Load(TheSpeechLib, path);
    if (int32_t(loaded) > -1) {   // (always)
        SpeechLib_Fn59F440(TheSpeechLib);
    }
}

// 0x5A6070: stops all speech and resets the library
void __cdecl Speech_ResetAll() {
    SpeechLib_Fn59F8A0(TheSpeechLib);
    SpeechSlot_SetState(speechSlotsBase + speechCurrentSlot * 0x5C, 0);
    SpeechSlot_SetState(speechSlotsBase + ((speechCurrentSlot - 1) & 1) * 0x5C, 0);
    SpeechLib_Fn59E820(TheSpeechLib);
}

// ---- the channel set (0xCF4284) --------------------------------------------------------------

// 0x5A60C0 (thiscall): not paused, not ready
uint8_t* __fastcall StreamChannels_Clear(uint8_t* self) {
    At<uint8_t>(self, 2) = 0;
    At<uint8_t>(self, 0x4C) = 0;
    return self;
}

// 0x5A60D0 (thiscall): pauses every channel
void __fastcall StreamChannels_PauseAll(uint8_t* self) {
    if (At<uint8_t>(self, 0x4C) == 0) {
        return;
    }
    for (int i = 0; i < 16; ++i) {
        if (ChannelAt(self, i) != nullptr) {
            Channel_Pause(ChannelAt(self, i));
        }
    }
}

// 0x5A6100 (thiscall): resumes every channel (only the speech one while speech is blocked)
void __fastcall StreamChannels_ResumeAll(uint8_t* self) {
    if (At<uint8_t>(self, 0x4C) == 0) {
        return;
    }
    if (speechBlockedA != 0) {
        Channel_Resume(ChannelAt(self, 2));
        return;
    }
    for (int i = 0; i < 16; ++i) {
        Channel_Resume(ChannelAt(self, i));
    }
}

// 0x5A6140 (thiscall): resets every channel
void __fastcall StreamChannels_ResetAll(uint8_t* self) {
    for (int i = 0; i < 16; ++i) {
        Channel_Reset(ChannelAt(self, i));
    }
}

// 0x5A6170 (thiscall): preloads a stream on a channel (the volume at most 1)
uint8_t __fastcall StreamChannels_Preload(uint8_t* self, re::Edx, const char* name, float volume, int index,
                                          int flag) {
    if (!(1.0f >= volume || volume != volume)) {
        volume = 1.0f;
    }
    int mode = (index == 2 && speechBlockedA != 0) ? 0 : 2;   // (a debug print before)
    return Channel_Preload(ChannelAt(self, index), name, 0, volume, mode, uint8_t(flag) != 0 ? 1 : 0, 0);
}

// 0x5A61F0 (thiscall): plays a channel (clearing its hold)
void __fastcall StreamChannels_Play(uint8_t* self, re::Edx, int index) {
    At<uint8_t>(ChannelAt(self, index), 0x92) = 0;
    Channel_Play(ChannelAt(self, index));
}

// 0x5A6210 (thiscall): resets a channel unless it's idle
void __fastcall StreamChannels_StopChannel(uint8_t* self, re::Edx, int index) {
    uint8_t* channel = ChannelAt(self, index);
    if (Channel_IsActive(channel) != 0 || Channel_IsPaused(channel) != 0 || Channel_IsReady(channel) != 0) {
        Channel_Reset(ChannelAt(self, index));
    }
}

// the resume time pushed out to at least now + ms (as floats, like the game)
void DelayResume(uint32_t ms) {
    float until = float(m_snTimeInMilliseconds + ms);
    float last = float(streamsResumeTime);
    float later = (last >= until || last != last) ? last : until;
    streamsResumeTime = int32_t(later);
}

// 0x5A6250 (thiscall): pauses for a menu or an area change: the music (channel 0) keeps
// playing in the arcade races, the ambience switches to the interior one in area 0x37
void __fastcall StreamChannels_Pause(uint8_t* self, re::Edx, int force) {
    if (uint8_t(force) == 0 && streamsLastArea == currArea) {
        return;
    }
    DelayResume(1000);
    if (!(speechBlockedA != 0 && speechBlockedB != 0 && uint8_t(force) == 0)) {
        for (int i = 0; i < 16; ++i) {
            uint8_t* channel = ChannelAt(self, i);
            if (channel == nullptr) {
                continue;
            }
            if (i == 0) {
                uint32_t race;
                uint32_t race3D;
                int constructed = 0;
                NameKey_Construct(&race, reinterpret_cast<const char*>(0x0091E0B4));   // "ArcadeRace"
                constructed |= 1;
                bool arcade;
                if (Missions_Fn6AA940(TheMissions, Missions_Find(TheMissions, &race)) != 0) {
                    arcade = true;
                } else {
                    NameKey_Construct(&race3D, reinterpret_cast<const char*>(0x0091E0A4));   // "ArcadeRace3D"
                    constructed |= 2;
                    arcade = Missions_Fn6AA940(TheMissions, Missions_Find(TheMissions, &race3D)) != 0;
                }
                if ((constructed & 2) != 0) {
                    NameKey_Destruct(&race3D);
                }
                if ((constructed & 1) != 0) {
                    NameKey_Destruct(&race);
                }
                if (!(musicKeepsPlaying != 0 && uint8_t(force) != 0) && !arcade) {
                    Channel_Pause(ChannelAt(self, 0));
                }
            } else if (i == 1) {
                Channel_Reset(ChannelAt(self, 1));
            } else {
                Channel_Reset(channel);
            }
        }
        At<uint8_t>(self, 2) = 1;
    }
    At<uint32_t>(ambienceObject, 4) = currArea == 0x37 ? 0x00CF42D4 : 0x00CF4324;
}

// 0x5A6420 (thiscall): resumes after a pause (the music unless another plays)
void __fastcall StreamChannels_Resume(uint8_t* self, re::Edx, int force) {
    if (uint8_t(force) == 0 && streamsResumePending == 0) {
        return;
    }
    DelayResume(500);
    if (!(speechBlockedA != 0 && uint8_t(force) == 0)) {
        if (!(musicKeepsPlaying != 0 && uint8_t(force) != 0) && Music_Fn58FD80() == 0) {
            Channel_Resume(ChannelAt(self, 0));
        }
        At<uint8_t>(self, 2) = 0;
    }
    streamsResumePending = 0;
}

// 0x5A64D0 (thiscall): updates every channel; +0 the speech channel's activity
void __fastcall StreamChannels_RefreshImpl(uint8_t* self) {
    for (int i = 0; i < 16; ++i) {
        if (ChannelAt(self, i) != nullptr) {
            Channel_Update(ChannelAt(self, i));
        }
    }
    if (speechBlockedA != 0 || Settings_Fn50DF50(playlistSettings) == 0) {
        Sound_Fn5AFA90();
    }
    At<uint8_t>(self, 0) = Channel_IsActive(ChannelAt(self, 2)) != 0 ? 1 : 0;
}

// 0x5A6530 (thiscall): fades a channel's volume
void __fastcall StreamChannels_SetVolume(uint8_t* self, re::Edx, int index, float volume, int ms) {
    uint8_t* channel = ChannelAt(self, index);
    Fader_Start(channel + 0x14, channel + 0x20, At<float>(channel, 0x18), volume, fadeDefault, ms);
    At<uint8_t>(channel, 0x30) = 0;
}

// 0x5A6570 (thiscall): loads a playlist (audio\PLAYLIST\<name>.BIN) into the trigger loader
uint8_t __fastcall StreamChannels_LoadPlaylistImpl(uint8_t* self, re::Edx, const char* name, int noTable) {
    char path[0x100];
    std::memcpy(path, reinterpret_cast<const void*>(0x00921F34), 16);   // "audio\PLAYLIST\"
    std::strcat(path, name);
    std::strcat(path, reinterpret_cast<const char*>(0x00921F2C));       // ".BIN"
    Sound_Lock(0x2E);
    TriggerLoader_AddGlobal(path, noTable);
    StreamChannels_Refresh(self);
    ImportSleep()(100);
    At<uint8_t>(self, 1) = 1;
    Sound_Unlock();
    return 1;
}

// 0x5A6640 (thiscall): creates the 16 channels and loads the playlists
void __fastcall StreamChannels_Init(uint8_t* self) {
    StreamChannels_Allocate(0x10);
    StreamChannels_OpenFiles();
    for (int i = 0; i < 16; ++i) {
        At<uint8_t*>(self, 4 + i * 4) = GetStreamChannel(i);
    }
    StreamChannels_LoadPlaylist(self, reinterpret_cast<const char*>(0x00921F5C), 1);   // "SPEECH"
    StreamChannels_LoadPlaylist(self, reinterpret_cast<const char*>(0x00921F54), 0);   // "AMBS"
    StreamChannels_LoadPlaylist(self, reinterpret_cast<const char*>(0x00921F4C), 0);   // "CUTS"
    StreamChannels_LoadPlaylist(self, reinterpret_cast<const char*>(0x00921F44), 0);   // "MUSIC"
    if (speechDebugNames != 0) {
        Screamer_Fn8283D0(streamSounds);
    }
    At<uint8_t>(self, 0x4C) = 1;
}

// 0x5A66D0 (thiscall, a fader): running (+8 > 0) and at 0 (+4)
int32_t __fastcall Fader_IsActiveImpl(uint8_t* self) {
    if (At<int32_t>(self, 8) > 0 && At<float>(self, 4) == 0.0f) {
        return 1;
    }
    return 0;
}

// ---- the trigger loader (count, then 0xAC-byte entries: +0 table, +4 name, +0x84, +0x88) --

// 0x5A66F0 (thiscall): adds a playlist (its hash table loaded unless noTable)
uint8_t __fastcall TriggerLoader_AddImpl(uint8_t* self, re::Edx, const char* name, int noTable) {
    uint8_t* entry = self + 4 + At<int32_t>(self, 0) * 0xAC;
    if (uint8_t(noTable) == 0) {
        At<void*>(entry, 0) = HashTable_Load(name);
        if (At<void*>(self + 4 + At<int32_t>(self, 0) * 0xAC, 0) == nullptr) {
            return 0;
        }
    } else {
        At<void*>(entry, 0) = nullptr;
    }
    std::strcpy(reinterpret_cast<char*>(self + 8 + At<int32_t>(self, 0) * 0xAC), name);
    At<int32_t>(self, 0x88 + At<int32_t>(self, 0) * 0xAC) = 0;
    At<int32_t>(self, 0x8C + At<int32_t>(self, 0) * 0xAC) = -1;
    At<int32_t>(self, 0) += 1;
    return 1;
}

// 0x5A6790 (thiscall): finds a stream by name in the playlists
uint8_t __fastcall TriggerLoader_FindByNameImpl(uint8_t* self, re::Edx, const char* name, int32_t* index, int32_t* a,
                                                int32_t* b) {
    for (int32_t i = 0; i < At<int32_t>(self, 0); ++i) {
        void* table = At<void*>(self, 4 + i * 0xAC);
        if (table != nullptr && HashTable_Find(table, HashStreamName(name, 1), a, b) != 0) {
            *index = i;
            return 1;
        }
    }
    return 0;
}

// 0x5A6800 (thiscall): finds a stream by hash in the playlists
uint8_t __fastcall TriggerLoader_FindImpl(uint8_t* self, re::Edx, uint32_t hash, int32_t* index, int32_t* a,
                                          int32_t* b) {
    for (int32_t i = 0; i < At<int32_t>(self, 0); ++i) {
        void* table = At<void*>(self, 4 + i * 0xAC);
        if (table != nullptr && HashTable_Find(table, hash, a, b) != 0) {
            *index = i;
            return 1;
        }
    }
    return 0;
}

// 0x5A6860 (thiscall): a playlist's name (copied if asked) and its +0x88
int32_t __fastcall TriggerLoader_GetEntryImpl(uint8_t* self, re::Edx, int32_t index, int, char* name) {
    if (name != nullptr) {
        std::strcpy(name, reinterpret_cast<const char*>(self + 8 + index * 0xAC));
    }
    return At<int32_t>(self, 0x88 + index * 0xAC);
}

// 0x5A68A0: a channel
uint8_t* __cdecl GetStreamChannelImpl(int index) {
    return channelArray + index * 0x1A0;
}

// 0x5A68C0: adds a playlist to the global loader
uint8_t __cdecl TriggerLoader_AddGlobalImpl(const char* name, int noTable) {
    return TriggerLoader_Add(triggerLoader, name, noTable);
}

// 0x5A68E0: opens the four playlist files (overlapped, unbuffered) and sets up the channels
void __cdecl StreamChannels_OpenFilesImpl() {
    CreateFileAFn createFile = ImportCreateFileA();
    struct {
        uintptr_t name;
        char* path;
        uint32_t* handle;   // (a 4-byte game variable: a handle's low 32 bits are all of it)
    } files[4] = {{0x00922064, playlistSpeechPath, &playlistSpeech},   // \Audio\PLAYLIST\Speech.bin
                  {0x00922048, playlistMusicPath, &playlistMusic},     // Music.bin
                  {0x0092202C, playlistCutsPath, &playlistCuts},       // Cuts.bin
                  {0x00922010, playlistAmbsPath, &playlistAmbs}};      // Ambs.bin
    for (auto& file : files) {
        std::strcpy(file.path, FixPath(reinterpret_cast<const char*>(file.name)));
        *file.handle = uint32_t((createFile(file.path, 0x80000000, 1, nullptr, 3, 0x50000001, nullptr)));
    }
    for (int32_t i = 0; i < channelCount; ++i) {
        uint8_t* channel = channelArray + i * 0x1A0;
        At<int32_t>(channel, 8) = i;
        At<int32_t>(channel, 0) = 2;
        At<int32_t>(channel, 0x10) = 0;
        At<int32_t>(channel, 0xC) = 0;
        At<uint8_t>(channel, 0x30) = 0;
        At<float>(channel, 0x34) = -1.0f;
        At<int32_t>(channel, 4) = 0;
        At<int32_t>(channel, 0x94) = 0;
        At<uint8_t>(channel, 0x98) = 0;
        At<int32_t>(channel, 0x19C) = 0;
        At<int32_t>(channel, 0x80) = -1;
        At<int32_t>(channel, 0x8C) = -1;
        At<int32_t>(channel, 0x84) = i + 8;
        At<int32_t>(channel, 0x58) = -1;
        At<int32_t>(channel, 0x5C) = -1;
        At<int32_t>(channel, 0x54) = -1;
        At<int32_t>(channel, 0x60) = 0;
        At<uint8_t>(channel, 0x90) = 1;
        At<int32_t>(channel, 0x64) = 0;
        At<int32_t>(channel, 0x68) = -1;
        At<uint8_t>(channel, 0x91) = 0;
        At<uint8_t>(channel, 0x92) = 0;
    }
}

// 0x5A6B10: the open playlist file for a path (-1 unknown; nothing for null)
void __stdcall Playlist_HandleImpl(const char* path, uint32_t* out) {
    if (path == nullptr) {
        return;
    }
    if (Stricmp(playlistSpeechPath, path) == 0) {
        *out = playlistSpeech;
    } else if (Stricmp(playlistMusicPath, path) == 0) {
        *out = playlistMusic;
    } else if (Stricmp(playlistCutsPath, path) == 0) {
        *out = playlistCuts;
    } else if (Stricmp(playlistAmbsPath, path) == 0) {
        *out = playlistAmbs;
    } else {
        *out = uint32_t(-1);
    }
}

// ---- a channel ---------------------------------------------------------------------------------

bool ChannelDisabled(uint8_t* channel) {
    int32_t index = At<int32_t>(channel, 8);
    return (disableChannel1 != 0 && index == 1) || (disableChannel2 != 0 && index == 2) ||
           (disableChannel0 != 0 && index == 0);
}

// 0x5A6BC0 (thiscall): looks a stream up (what it found is dropped; +0xC cleared)
void __fastcall Channel_Lookup(uint8_t* self, re::Edx, uint32_t hash, int, int, int, int) {
    if (ChannelDisabled(self)) {
        return;
    }
    At<int32_t>(self, 0xC) = 0;
    if (hash != 0) {
        int32_t index;
        int32_t a;
        int32_t b;
        TriggerLoader_Find(triggerLoader, hash, &index, &a, &b);
    }
}

// 0x5A6C40 (thiscall): frees its sound engine handles and read buffer
void __fastcall Channel_ReleaseImpl(uint8_t* self) {
    if (At<int32_t>(self, 0x5C) != -1) {
        Engine_Fn5AF420(At<int32_t>(self, 0x58), At<int32_t>(self, 0x5C), 1);
        Engine_Fn5AF470(At<int32_t>(self, 0x58), At<int32_t>(self, 0x5C));
        At<int32_t>(self, 0x5C) = -1;
    }
    if (At<int32_t>(self, 0x58) != -1) {
        Engine_Fn5AF1D0(At<int32_t>(self, 0x58));
        At<int32_t>(self, 0x58) = -1;
    }
    if (At<int32_t>(self, 0x54) != -1) {
        Engine_Fn5AF2F0(At<int32_t>(self, 0x54));
        At<int32_t>(self, 0x54) = -1;
    }
    if (At<int32_t>(self, 0x68) != -1) {
        At<int32_t>(self, 0x68) = -1;
    }
    if (At<void*>(self, 0x64) != nullptr) {
        Free(At<void*>(self, 0x64));
        At<void*>(self, 0x64) = nullptr;
    }
    At<int32_t>(self, 0x6C) = 0;
    At<int32_t>(self, 0x70) = 0;
    At<int32_t>(self, 0x74) = 0;
    At<int32_t>(self, 0x78) = 0;
    At<int32_t>(self, 0x7C) = 0;
    At<int32_t>(self, 0x80) = -1;
    At<uint8_t>(self, 0x90) = 0;
    At<uint8_t>(self, 0x91) = 0;
    At<uint8_t>(self, 0x92) = 0;
}

// 0x5A6CE0 (thiscall): positional at a point
void __fastcall Channel_SetPosition(uint8_t* self, re::Edx, const CVector* position, int distance) {
    At<CVector>(self, 0x44) = *position;
    SetRef(self + 0x40, nullptr);
    At<int32_t>(self, 0x3C) = distance;
    if (At<uint8_t>(self, 0x38) == 0) {
        At<uint16_t>(self, 0x50) = 0xFFFF;
    }
    At<uint8_t>(self, 0x38) = 1;
}

// 0x5A6D20 (thiscall): positional at an entity (none: not positional)
void __fastcall Channel_SetEntity(uint8_t* self, re::Edx, void* entity, int distance) {
    if (entity != nullptr) {
        At<CVector>(self, 0x44) = {0.0f, 0.0f, 0.0f};
        SetRef(self + 0x40, entity);
        At<int32_t>(self, 0x3C) = distance;
        if (At<uint8_t>(self, 0x38) == 0) {
            At<uint16_t>(self, 0x50) = 0xFFFF;
        }
    }
    At<uint8_t>(self, 0x38) = entity != nullptr ? 1 : 0;
}

// 0x5A6D60 (thiscall): playing (0, 1, 8)
int32_t __fastcall Channel_IsActiveImpl(uint8_t* self) {
    int32_t state = At<int32_t>(self, 0);
    return (state == 0 || state == 1 || state == 8) ? 1 : 0;
}

// 0x5A6D80 (thiscall): paused (3, 4)
int32_t __fastcall Channel_IsPausedImpl(uint8_t* self) {
    int32_t state = At<int32_t>(self, 0);
    return (state == 3 || state == 4) ? 1 : 0;
}

// 0x5A6DA0 (thiscall): loaded, ready to play (7)
int32_t __fastcall Channel_IsReadyImpl(uint8_t* self) {
    return At<int32_t>(self, 0) == 7 ? 1 : 0;
}

// 0x5A6DB0 (thiscall): plays it unless held
void __fastcall Channel_PlayImpl(uint8_t* self) {
    if (At<uint8_t>(self, 0x92) == 0) {
        At<uint8_t>(self, 0x90) = 1;
    }
}

// 0x5A6DD0 (thiscall, the trigger loader): a sound's playlist index (with the found
// record's first field's upper bytes, like the game)
int32_t __fastcall StreamSounds_GetBank(uint8_t* self, re::Edx, uint32_t hash) {
    int32_t index;
    int32_t a = int32_t(hash);
    int32_t b;
    if (TriggerLoader_Find(self, hash, &index, &a, &b) != 0) {
        return int32_t((uint32_t(a) & 0xFFFFFF00) + uint32_t(index));
    }
    return 0;
}

// 0x5A6E40 (thiscall): pauses it
void __fastcall Channel_PauseImpl(uint8_t* self) {
    if (At<int32_t>(self, 0x5C) != -1) {
        Engine_Pause(At<int32_t>(self, 0x5C), 1);
        if (At<int32_t>(self, 0) == 0) {
            At<int32_t>(self, 0) = 3;
        }
    }
}

// 0x5A6E70 (thiscall): resumes it
void __fastcall Channel_ResumeImpl(uint8_t* self) {
    if (At<int32_t>(self, 0x5C) != -1) {
        Engine_Pause(At<int32_t>(self, 0x5C), 0);
        if (At<int32_t>(self, 0) == 3) {
            At<int32_t>(self, 0) = 0;
        }
    }
}

void ResetChannel(uint8_t* self) {
    At<int32_t>(self, 0) = 2;
    Channel_Release(self);
    At<float>(self, 0x18) = 0.0f;
    At<int32_t>(self, 0x1C) = 0;
    At<int32_t>(self, 0x14) = 6;
    At<uint8_t>(self, 0x30) = 0;
    At<uint8_t>(self, 0x38) = 0;
    At<uint16_t>(self, 0x50) = 0xFFFF;
}

// 0x5A6EA0 (thiscall): back to idle
void __fastcall Channel_ResetImpl(uint8_t* self) {
    ResetChannel(self);
}

// 0x5A6ED0 (thiscall): the volume (and for positional ones the distance fall-off and
// pan); sent to the engine when it changed by more than 0.0001
void __fastcall Channel_SetVolumeImpl(uint8_t* self, re::Edx, float volume, int ms) {
    float pan = 0.0f;
    uint8_t* options = *reinterpret_cast<re::Ptr32<uint8_t>*>(0x00CF4264);
    if (At<int32_t>(self, 8) >= 2 && (At<int32_t>(options, 0x28) != 0 || speechBlockedA == 0)) {
        pan = 1.0f;
    }
    At<float>(self, 0x18) = volume;
    if (ms != -1) {
        At<int32_t>(self, 0x14) = ms;
    }
    float level = SFX_Fn59E4F0(At<float>(self, 0x18), At<int32_t>(self, 0x14));
    if (At<uint8_t>(self, 0x38) != 0) {
        CVector position = {0.0f, 0.0f, 0.0f};
        uint8_t* entity = At<uint8_t*>(self, 0x40);
        if (entity != nullptr) {
            uint8_t* matrix = At<uint8_t*>(entity, 0x14);
            position = *reinterpret_cast<CVector*>(matrix != nullptr ? matrix + 0x30 : entity + 4);
        } else {
            position = At<CVector>(self, 0x44);
        }
        CVector cameraOut;
        CVector camera = *GetCameraPos(&cameraOut);
        CVector delta;
        VectorSub(&delta, &position, &camera);
        float length2 = delta.y * delta.y + delta.x * delta.x + delta.z * delta.z;
        float distance = std::sqrt(std::fabs(length2));
        uint32_t falloff = uint32_t(SFX_Fn59E3E0(distance, At<int32_t>(self, 0x3C)));
        level = float(double(falloff) * level);
        level = float(double(level) * 0.0009765625);
        if (level < 0.0f) {
            level = 0.0f;
        } else if (!(1.0f >= level || level != level)) {
            level = 1.0f;
        }
        int positionPan = Sounds_Fn59B740(TheSounds, &position, 0);
        Engine_SetVolumePan(At<int32_t>(self, 0x5C), int(float(double(level) * 1024.0)), positionPan, 0);
    } else if (At<int32_t>(self, 0x5C) != -1) {
        Engine_SetPan(At<int32_t>(self, 0x5C), pan);
    }
    float change = std::fabs(float(level - At<float>(self, 0x34)));
    if (double(change) <= 9.999999747378752e-05 || change != change) {
        return;
    }
    At<float>(self, 0x34) = level;
    if (At<uint8_t>(self, 0x38) != 0) {
        return;
    }
    Engine_SetVolume(At<int32_t>(self, 0x5C), int(float(double(level) * 1024.0)), 0);
}

// 0x5A7140 (thiscall): stops it, fading out over ms when it's playing
void __fastcall Channel_Stop(uint8_t* self, re::Edx, int ms) {
    int32_t state = At<int32_t>(self, 0);
    if (state != 0 && state != 1) {   // (8, 5, 3, 4 and the others: at once)
        ResetChannel(self);
        return;
    }
    if (Channel_IsLoadedIdle(self) != 0) {
        uint8_t* fader = self + 0x20;
        if (Fader_IsActive(fader) == 0) {
            Fader_Start59E6B0(fader, 1.0f, 0.0f, fadeDefault, ms);
        }
    } else {
        MusicLayer_FadeTo(self, 0.0f, fadeDefault, ms);
    }
    At<uint8_t>(self, 0x30) = 1;
}

// 0x5A7200: allocates the channels and the trigger loader
void __cdecl StreamChannels_AllocateImpl(int count) {
    uint64_t bytes = uint64_t(uint32_t(count)) * 0x1A0;
    uint32_t size = bytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(bytes);
    size = size + 4 < size ? 0xFFFFFFFFu : size + 4;
    uint8_t* block = static_cast<uint8_t*>(New(size));
    if (block != nullptr) {
        At<int32_t>(block, 0) = count;
        ConstructArray(block + 4, 0x1A0, count, reinterpret_cast<void*>(0x005A6E10));
        channelArray = block + 4;
    } else {
        channelArray = nullptr;
    }
    channelCount = count;
    channelArrayUnknown = 0;
    uint8_t* loader = static_cast<uint8_t*>(New(0xD74));
    if (loader != nullptr) {
        Memset(loader + 4, 0, 0xD70);
        At<int32_t>(loader, 0) = 0;
        triggerLoader = loader;
    } else {
        triggerLoader = nullptr;
    }
}

// 0x5A72B0 (thiscall): a step of loading a stream from its playlist file (the step is
// kept only when it worked)
void __fastcall Channel_SetLoadStepImpl(uint8_t* self, re::Edx, int32_t step) {
    switch (step) {
    case 0:
        Channel_Release(self);
        break;
    case 1: {
        char name[0x74];
        At<int32_t>(self, 0x88) = TriggerLoader_GetEntry(triggerLoader, At<int32_t>(self, 0x80), At<int32_t>(self, 0x8C),
                                                         name);
        Playlist_Handle(FixPath(name), &At<uint32_t>(self, 0x68));
        int32_t file = At<int32_t>(self, 0x68);
        if (file == -1) {
            return;
        }
        struct {
            int32_t file;
            int32_t offset;
            int32_t unknown8;
            int32_t kind;
        } description = {file, At<int32_t>(self, 0x8C) + 0x800, 0, 9};
        Sound_Lock(0x2E);
        At<int32_t>(self, 0x54) = Engine_Fn5AF2A0(&description);
        Sound_Unlock();
        if (At<int32_t>(self, 0x54) == -1) {
            return;
        }
        break;
    }
    case 2: {
        At<void*>(self, 0x64) = New(0x800);
        At<int32_t>(self, 0x74) = At<int32_t>(self, 0x8C);
        At<uint32_t>(self, 0x7C) = ImportCreateEventA()(nullptr, 0, 0, nullptr);
        if (ImportReadFile()(At<uint32_t>(self, 0x68), At<void*>(self, 0x64), 0x800, nullptr, re::SystemOverlapped(self + 0x6C)) == 0) {
            ImportGetLastError()();
        }
        break;
    }
    case 3:
        At<int32_t>(self, 0x84) = step;
        At<int32_t>(self, 0) = 6;
        return;
    case 4:
        Sound_Lock(0x2E);
        At<int32_t>(self, 0x5C) = Engine_Fn5AF3B0(At<int32_t>(self, 0x58), 0);
        Sound_Unlock();
        if (At<int32_t>(self, 0x5C) == -1) {
            return;
        }
        break;
    case 5:
        At<int32_t>(self, 0) = 7;
        if (At<uint8_t>(self, 0x91) == 0 && At<uint8_t>(self, 0x92) == 0) {
            At<uint8_t>(self, 0x90) = 1;
        }
        if (At<int32_t>(self, 0x5C) != -1 && Engine_Fn5AF510(At<int32_t>(self, 0x5C)) != 0) {
            At<int32_t>(self, 0x84) = step;
            At<int32_t>(self, 0) = 3;
            return;
        }
        break;
    case 6:
        At<int32_t>(self, 0x84) = step;
        At<int32_t>(self, 0) = 0;
        return;
    case 7:
        ResetChannel(self);
        break;
    default:
        break;
    }
    At<int32_t>(self, 0x84) = step;
}

// 0x5A74F0 (thiscall): advances the load steps (any engine error resets it)
void __fastcall Channel_UpdateLoadingImpl(uint8_t* self) {
    if ((At<int32_t>(self, 0x54) != -1 && Engine_Fn5AF340(At<int32_t>(self, 0x54)) == 1) ||
        (At<int32_t>(self, 0x58) != -1 && Engine_Fn5AF210(At<int32_t>(self, 0x58)) == 1) ||
        (At<int32_t>(self, 0x5C) != -1 && Engine_Fn5AF9A0(At<int32_t>(self, 0x5C)) == 1)) {
        ResetChannel(self);
        return;
    }
    switch (At<int32_t>(self, 0x84)) {
    case 0:
        Channel_SetLoadStep(self, 1);
        return;
    case 1: {
        if (At<int32_t>(self, 0x54) == -1) {
            return;
        }
        int32_t state = 0;
        Engine_Fn5AF330(At<int32_t>(self, 0x54), &state);
        if (state != 4) {
            return;
        }
        At<void*>(self, 0x64) = New(0x800);
        At<int32_t>(self, 0x74) = At<int32_t>(self, 0x8C);
        At<uint32_t>(self, 0x7C) = ImportCreateEventA()(nullptr, 0, 0, nullptr);
        if (ImportReadFile()(At<uint32_t>(self, 0x68), At<void*>(self, 0x64), 0x800, nullptr, re::SystemOverlapped(self + 0x6C)) == 0) {
            ImportGetLastError()();
        }
        At<int32_t>(self, 0x84) = 2;
        return;
    }
    case 2:
        if (At<void*>(self, 0x64) == nullptr) {
            return;
        }
        At<int32_t>(self, 0) = 6;
        At<int32_t>(self, 0x84) = 3;
        return;
    case 3: {
        uint32_t read = 0;
        int got = ImportGetOverlappedResult()(At<uint32_t>(self, 0x68), re::SystemOverlapped(self + 0x6C), &read, 0);
        re::SyncOverlapped(self + 0x6C);
        if (got == 0) {
            return;
        }
        if (At<void*>(self, 0x64) == nullptr || read != 0x800) {
            return;
        }
        if (At<int32_t>(self, 0) != 6) {
            if (At<int32_t>(self, 0x5C) == -1) {
                return;
            }
            Engine_Fn5AF570(At<int32_t>(self, 0x5C), 0, 0);
            At<int32_t>(self, 0x84) = 6;
            At<int32_t>(self, 0) = 0;
            return;
        }
        Sound_Lock(0x2E);
        Sound_Lock(0x2F);
        uint8_t* header = static_cast<uint8_t*>(New(0x800));
        Sound_Unlock();
        std::memcpy(header, At<void*>(self, 0x64), 0x800);
        At<int32_t>(self, 0x58) = Engine_Fn5AF150(header, At<int32_t>(header, 0x7FC), 0, 0x9A020004);
        Sound_Unlock();
        if (At<int32_t>(self, 0x58) == -1) {
            return;
        }
        Sound_Lock(0x2E);
        At<int32_t>(self, 0x5C) = Engine_Fn5AF3B0(At<int32_t>(self, 0x58), 0);
        Sound_Unlock();
        if (At<int32_t>(self, 0x5C) == -1) {
            return;
        }
        At<int32_t>(self, 0x84) = 4;
        return;
    }
    case 4: {
        if (At<int32_t>(self, 0x5C) == -1) {
            return;
        }
        uint32_t state = 0;
        if (Engine_Fn5AF550(At<int32_t>(self, 0x5C), &state) == 0 || (state & 4) == 0) {
            return;
        }
        At<int32_t>(self, 0) = 7;
        if (At<uint8_t>(self, 0x91) == 0 && At<uint8_t>(self, 0x92) == 0) {
            At<uint8_t>(self, 0x90) = 1;
        }
        if (At<int32_t>(self, 0x5C) != -1 && Engine_Fn5AF510(At<int32_t>(self, 0x5C)) != 0) {
            At<int32_t>(self, 0) = 3;
        }
        At<int32_t>(self, 0x84) = 5;
        return;
    }
    case 5:
        if (At<uint8_t>(self, 0x90) == 0 || At<int32_t>(self, 0x5C) == -1 ||
            Engine_Fn5AF510(At<int32_t>(self, 0x5C)) != 0) {
            return;
        }
        Channel_SetVolume(self, At<float>(self, 0x18), At<int32_t>(self, 0x14));
        Engine_Fn5AF660(At<int32_t>(self, 0x5C), 0.0f);
        Engine_Fn5AF570(At<int32_t>(self, 0x5C), 0, 0);
        At<int32_t>(self, 0x84) = 6;
        At<int32_t>(self, 0) = 0;
        return;
    case 6: {
        if (At<int32_t>(self, 0x5C) == -1) {
            return;
        }
        uint32_t state = 0;
        if (Engine_Fn5AF550(At<int32_t>(self, 0x5C), &state) == 0 || (state & 0x20) == 0) {
            return;
        }
        Channel_Reset(self);
        At<int32_t>(self, 0x84) = 7;
        return;
    }
    default:
        return;
    }
}

// 0x5A7880 (thiscall): preloads a stream by name, hash, or the speech slot's line
// (useSpeech): true when found (or the channel is disabled)
uint8_t __fastcall Channel_PreloadImpl(uint8_t* self, re::Edx, const char* name, uint32_t hash, float volume, int ms,
                                       int, int useSpeech) {
    if (ChannelDisabled(self)) {
        return 1;
    }
    int32_t offset;
    int32_t extra;
    int32_t entry;
    uint8_t found;
    if (name != nullptr) {
        found = TriggerLoader_FindByName(triggerLoader, name, &entry, &offset, &extra);
    } else if (hash != 0) {
        found = TriggerLoader_Find(triggerLoader, hash, &entry, &offset, &extra);
    } else if (uint8_t(useSpeech) != 0) {
        int32_t index = At<int32_t>(self, 8);
        uint8_t* slot = speechSubtitles != 0 ? reinterpret_cast<uint8_t*>(0x00CF44D4) + index * 0x5C
                                             : speechSlotsBase + speechCurrentSlot * 0x5C;
        found = History_Fn68E200(speechHistory, index - 2, At<int32_t>(slot, 0x50), &entry, &offset, &extra);
    } else {
        return 0;
    }
    if (found == 0) {
        return 0;
    }
    At<float>(self, 0x34) = -1.0f;
    At<uint16_t>(self, 0x50) = 0xFFFF;
    At<int32_t>(self, 0x1C) = 0;
    At<float>(self, 0x18) = 0.0f;
    At<int32_t>(self, 0x14) = 6;
    At<int32_t>(self, 0x14) = ms;
    At<float>(self, 0x18) = volume;
    Channel_Release(self);
    At<int32_t>(self, 0x84) = 0;
    At<int32_t>(self, 0x80) = entry;
    At<int32_t>(self, 0x8C) = offset;
    At<int32_t>(self, 0) = 6;
    return 1;
}

// 0x5A7A00 (thiscall): loads a stream by name or hash to play: true when found (or the
// channel is disabled)
uint8_t __fastcall Channel_Load(uint8_t* self, re::Edx, const char* name, uint32_t hash, float volume, int ms, int) {
    if (ChannelDisabled(self)) {
        return 1;
    }
    int32_t state = At<int32_t>(self, 0);
    if (state == 0 || state == 1 || state == 8 || state == 3 || state == 4) {
        At<uint8_t>(self, 0x38) = 0;
    }
    At<float>(self, 0x18) = 0.0f;
    At<int32_t>(self, 0x1C) = 0;
    At<int32_t>(self, 0x14) = 6;
    At<float>(self, 0x18) = volume;
    At<uint8_t>(self, 0x30) = 0;
    At<int32_t>(self, 0x14) = ms;
    At<int32_t>(self, 0xC) = 0;
    int32_t entry;
    int32_t offset;
    int32_t extra;
    uint8_t found;
    if (name != nullptr) {
        found = TriggerLoader_FindByName(triggerLoader, name, &entry, &offset, &extra);
        std::strcpy(reinterpret_cast<char*>(self + 0x99), name);
    } else if (hash != 0) {
        found = TriggerLoader_Find(triggerLoader, hash, &entry, &offset, &extra);
        At<uint32_t>(self, 0x19C) = hash;
    } else {
        return 0;
    }
    if (found == 0) {
        return 0;
    }
    At<float>(self, 0x34) = -1.0f;
    At<uint16_t>(self, 0x50) = 0xFFFF;
    Channel_Release(self);
    At<int32_t>(self, 0x84) = 0;
    At<int32_t>(self, 0x80) = entry;
    At<int32_t>(self, 0x8C) = offset;
    At<int32_t>(self, 0x60) = extra;
    At<int32_t>(self, 0) = 5;
    return 1;
}

// 0x5A7B70 (thiscall): the per-frame update: loading, the faders, the volume
void __fastcall Channel_UpdateImpl(uint8_t* self) {
    if (At<int32_t>(self, 0) != 2) {
        Channel_UpdateLoading(self);
    }
    int32_t state = At<int32_t>(self, 0);
    if (state == 0 || state == 1 || state == 8) {
        Fader_Update(self + 0x14);
        if (At<uint8_t>(self, 0x30) != 0 && At<int32_t>(self, 0x1C) == 0 && double(At<float>(self, 0x18)) < 1e-06) {
            Channel_Reset(self);
            return;
        }
        Channel_SetVolume(self, At<float>(self, 0x18), At<int32_t>(self, 0x14));
        return;
    }
    if (speechMuted != 0 && (state == 3 || state == 4)) {
        Channel_SetVolume(self, At<float>(self, 0x18), At<int32_t>(self, 0x14));
        return;
    }
    At<uint16_t>(self, 0x50) = 0xFFFF;
}

// ---- the hash tables ({count, records}: 0xC-byte {hash, a, b} records) -------------------

// 0x5A7C00 (thiscall): reads a table from a file
void* __fastcall HashTable_ConstructImpl(uint8_t* self, re::Edx, void* file) {
    File_Read(file, self + 4, 4);
    uint64_t bytes = uint64_t(At<uint32_t>(self, 4)) * 0xC;
    void* records = New(bytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(bytes));
    At<void*>(self, 0) = records;
    File_Read(file, records, At<int32_t>(self, 4) * 0xC);
    return self;
}

// 0x5A7C50: loads a table ("cdrom0:" and anything from ';' dropped; the magic "Hash")
void* __cdecl HashTable_LoadImpl(const char* path) {
    char name[0x100];
    if (Strncmp(path, reinterpret_cast<const char*>(0x00922134), 7) == 0) {   // "cdrom0:"
        std::strcpy(name, path + 7);
    } else {
        std::strcpy(name, path);
    }
    char* semicolon = StrRChr(name, ';');
    if (semicolon != nullptr) {
        *semicolon = 0;
    }
    void* file = File_Open(name, reinterpret_cast<const char*>(0x008FF548), 1);   // "r"
    if (file == nullptr) {
        return nullptr;
    }
    uint32_t magic;
    File_Read(file, &magic, 4);
    if (magic != 0x68736148) {
        return nullptr;   // (the file stays open)
    }
    void* table = New(8);
    if (table != nullptr) {
        void* constructed = HashTable_Construct(table, file);
        File_Close(file);
        return constructed;
    }
    File_Close(file);
    return nullptr;
}

// 0x5A7D60: a stream name's hash (quotes stripped, lower case, '\' as '/')
uint32_t __cdecl HashStreamNameImpl(const char* name, int) {
    uint32_t hash = 0;
    bool quoted = *name == '"';
    if (quoted) {
        ++name;
    }
    while (*name != 0) {
        char c = *name;
        if (quoted && c == '"') {
            break;
        }
        ++name;
        if (uint8_t(c - 'A') <= 0x19) {
            c = char(c + 0x20);
        } else if (c == '\\') {
            c = '/';
        }
        hash = (uint32_t(int32_t(c)) + hash) * 0x401;
        hash ^= hash >> 6;
    }
    hash *= 9;
    hash ^= hash >> 11;
    return hash * 0x8001;
}

// 0x5A7DD0 (thiscall): a hash's record
uint8_t __fastcall HashTable_FindImpl(uint8_t* self, re::Edx, uint32_t hash, int32_t* a, int32_t* b) {
    uint32_t key[3];
    key[0] = hash;
    uint8_t* record = static_cast<uint8_t*>(
        Bsearch(key, At<void*>(self, 0), At<uint32_t>(self, 4), 0xC, reinterpret_cast<void*>(0x005A7D40)));
    if (record == nullptr) {
        return 0;
    }
    *a = At<int32_t>(record, 4);
    *b = At<int32_t>(record, 8);
    return 1;
}

// ---- registration ---------------------------------------------------------------------------

#define STREAMCH_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(streamch_original_, __LINE__);                                        \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(streamch_original_, __LINE__), \
             re::Mode::Original)

STREAMCH_REVERSED("Speech::LoadDatabase", 0x005A5FF0, &Speech_LoadDatabase);
STREAMCH_REVERSED("Speech::ResetAll", 0x005A6070, &Speech_ResetAll);
STREAMCH_REVERSED("StreamChannels::Clear", 0x005A60C0, &StreamChannels_Clear);
STREAMCH_REVERSED("StreamChannels::PauseAll", 0x005A60D0, &StreamChannels_PauseAll);
STREAMCH_REVERSED("StreamChannels::ResumeAll", 0x005A6100, &StreamChannels_ResumeAll);
STREAMCH_REVERSED("StreamChannels::ResetAll", 0x005A6140, &StreamChannels_ResetAll);
STREAMCH_REVERSED("StreamChannels::Preload", 0x005A6170, &StreamChannels_Preload);
STREAMCH_REVERSED("StreamChannels::Play", 0x005A61F0, &StreamChannels_Play);
STREAMCH_REVERSED("StreamChannels::StopChannel", 0x005A6210, &StreamChannels_StopChannel);
STREAMCH_REVERSED("StreamChannels::Pause", 0x005A6250, &StreamChannels_Pause);
STREAMCH_REVERSED("StreamChannels::Resume", 0x005A6420, &StreamChannels_Resume);
STREAMCH_REVERSED("StreamChannels::Refresh", 0x005A64D0, &StreamChannels_RefreshImpl);
STREAMCH_REVERSED("StreamChannels::SetVolume", 0x005A6530, &StreamChannels_SetVolume);
STREAMCH_REVERSED("StreamChannels::LoadPlaylist", 0x005A6570, &StreamChannels_LoadPlaylistImpl);
STREAMCH_REVERSED("StreamChannels::Init", 0x005A6640, &StreamChannels_Init);
STREAMCH_REVERSED("Fader::IsActive", 0x005A66D0, &Fader_IsActiveImpl);
STREAMCH_REVERSED("TriggerLoader::Add", 0x005A66F0, &TriggerLoader_AddImpl);
STREAMCH_REVERSED("TriggerLoader::FindByName", 0x005A6790, &TriggerLoader_FindByNameImpl);
STREAMCH_REVERSED("TriggerLoader::Find", 0x005A6800, &TriggerLoader_FindImpl);
STREAMCH_REVERSED("TriggerLoader::GetEntry", 0x005A6860, &TriggerLoader_GetEntryImpl);
STREAMCH_REVERSED("StreamChannels::GetChannel", 0x005A68A0, &GetStreamChannelImpl);
STREAMCH_REVERSED("TriggerLoader::AddGlobal", 0x005A68C0, &TriggerLoader_AddGlobalImpl);
STREAMCH_REVERSED("StreamChannels::OpenFiles", 0x005A68E0, &StreamChannels_OpenFilesImpl);
STREAMCH_REVERSED("Playlist::Handle", 0x005A6B10, &Playlist_HandleImpl);
STREAMCH_REVERSED("StreamChannel::Lookup", 0x005A6BC0, &Channel_Lookup);
STREAMCH_REVERSED("StreamChannel::Release", 0x005A6C40, &Channel_ReleaseImpl);
STREAMCH_REVERSED("StreamChannel::SetPosition", 0x005A6CE0, &Channel_SetPosition);
STREAMCH_REVERSED("StreamChannel::SetEntity", 0x005A6D20, &Channel_SetEntity);
STREAMCH_REVERSED("StreamChannel::IsActive", 0x005A6D60, &Channel_IsActiveImpl);
STREAMCH_REVERSED("StreamChannel::IsPaused", 0x005A6D80, &Channel_IsPausedImpl);
STREAMCH_REVERSED("StreamChannel::IsReady", 0x005A6DA0, &Channel_IsReadyImpl);
STREAMCH_REVERSED("StreamChannel::Play", 0x005A6DB0, &Channel_PlayImpl);
STREAMCH_REVERSED("StreamSounds::GetBank", 0x005A6DD0, &StreamSounds_GetBank);
STREAMCH_REVERSED("StreamChannel::Pause", 0x005A6E40, &Channel_PauseImpl);
STREAMCH_REVERSED("StreamChannel::Resume", 0x005A6E70, &Channel_ResumeImpl);
STREAMCH_REVERSED("StreamChannel::Reset", 0x005A6EA0, &Channel_ResetImpl);
STREAMCH_REVERSED("StreamChannel::SetVolume", 0x005A6ED0, &Channel_SetVolumeImpl);
STREAMCH_REVERSED("StreamChannel::Stop", 0x005A7140, &Channel_Stop);
STREAMCH_REVERSED("StreamChannels::Allocate", 0x005A7200, &StreamChannels_AllocateImpl);
STREAMCH_REVERSED("StreamChannel::SetLoadStep", 0x005A72B0, &Channel_SetLoadStepImpl);
STREAMCH_REVERSED("StreamChannel::UpdateLoading", 0x005A74F0, &Channel_UpdateLoadingImpl);
STREAMCH_REVERSED("StreamChannel::Preload", 0x005A7880, &Channel_PreloadImpl);
STREAMCH_REVERSED("StreamChannel::Load", 0x005A7A00, &Channel_Load);
STREAMCH_REVERSED("StreamChannel::Update", 0x005A7B70, &Channel_UpdateImpl);
STREAMCH_REVERSED("StreamHashTable::StreamHashTable", 0x005A7C00, &HashTable_ConstructImpl);
STREAMCH_REVERSED("StreamHashTable::Load", 0x005A7C50, &HashTable_LoadImpl);
STREAMCH_REVERSED("StreamHashTable::HashName", 0x005A7D60, &HashStreamNameImpl);
STREAMCH_REVERSED("StreamHashTable::Find", 0x005A7DD0, &HashTable_FindImpl);

} // namespace
