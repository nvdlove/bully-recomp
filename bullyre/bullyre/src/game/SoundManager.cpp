// The audio manager (cSCREAMAudioManager, TheSounds at 0xCF3DA0): its setup and
// shutdown, the fire alarm (at +8), the loop slots, the volume table, the triggers'
// sorted list (+0x2354) and the skateboard's surface sounds. 0x594DC0..0x596360.
//
// TheSounds: +0 the audio follows the player (else the camera), +1, +8 the fire alarm,
// +0x35 paused, +0x39 debug names, +0x3C/+0x40 a list of 256 ids, +0x440 the fight music,
// +0x4C4 the environments, +0x4C8/+0x4CC times, +0x4D0, +0x4D4 the loops, +0x4D8 the
// ambience emitters, +0x4DC the rats, +0x4E0, +0x4E4 the stream channels (0xCF4284),
// +0x534 the music stream (0xCF42D4), +0x584 the ambient stream (0xCF4324), +0x5D4 the
// ambience (0xCF4374), +0x5D8 the speech (0xCF4378), +0x1070, +0x2014 the volume mode,
// +0x2024 the banks (0xCF5DC4), +0x2338 the area, +0x2354 the triggers, +0x2B28, +0x2B2C
// the SFX queue, +0x2B30, +0x2B34, +0x2B38 initialised.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Pool.h"
#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

struct CVector {
    float x, y, z;
};

// the fire alarm (TheSounds+8)
struct FireAlarm {
    re::Ptr32<uint8_t> sounds[6];     // +0 (one inside, or one per bell outside)
    uint8_t active;         // +0x18
    uint8_t pad19[3];
    int32_t bank;           // +0x1C
    int32_t soundInterior;  // +0x20
    int32_t soundExterior;  // +0x24
    uint8_t interior;       // +0x28
    uint8_t wasInterior;    // +0x29
};
static_assert(offsetof(FireAlarm, bank) == 0x1C);
static_assert(offsetof(FireAlarm, interior) == 0x28);

// a trigger's entry in the sorted list (+0x2358)
struct TriggerEntry {
    uint32_t hash;
    int32_t value;   // (the trigger's index, -1 unknown)
};

// a loop slot (0xCF3C08, 48 of them)
struct LoopSlot {
    re::Ptr32<uint8_t> sound;
    uint8_t keep;
    uint8_t pad[3];
};

inline uint8_t* const theSounds = reinterpret_cast<uint8_t*>(0x00CF3DA0);
inline uint8_t* const streamChannels = reinterpret_cast<uint8_t*>(0x00CF4284);
inline uint8_t* const ambientStream = reinterpret_cast<uint8_t*>(0x00CF4324);
inline uint8_t* const soundBanks = reinterpret_cast<uint8_t*>(0x00CF5DC4);
inline void* const voiceEngine = reinterpret_cast<void*>(0x00CF7268);
inline void* const volumeModeLock = reinterpret_cast<void*>(0x00CF7148);   // a CRITICAL_SECTION
inline char* const scratchBuffer = reinterpret_cast<char*>(0x00BD7800);
GAME_ARRAY(LoopSlot, loopSlots, 0x00CF3C08);
GAME_ARRAY(CVector, alarmBells, 0x00AE6B88);        // six outside
GAME_VAR(uint16_t, alarmRange, 0x00AE7076);
GAME_VAR(uint8_t*, ambience, 0x00CF4374);
GAME_VAR(uint8_t, audioFlagCF4270, 0x00CF4270);
GAME_VAR(void*, speechChannel0, 0x00CF4290);
GAME_VAR(void*, speechChannel1, 0x00CF4294);
GAME_VAR(int32_t, volumeMode, 0x00AE80E4);
GAME_VAR(uint32_t, m_snTimeInMilliseconds, 0x00C1A9B4);
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);
GAME_VAR(void*, theCamera, 0x00C3CCF4);
GAME_VAR(float, simulationSpeed, 0x00C1A9A8);
GAME_VAR(uint32_t, audioTimeCF4268, 0x00CF4268);
GAME_VAR(uint32_t, audioTimeCF426C, 0x00CF426C);
GAME_VAR(uint32_t, audioTimeCF68DC, 0x00CF68DC);
GAME_VAR(uint32_t, audioTimeCF68E0, 0x00CF68E0);
GAME_VAR(uint32_t, audioTimeCF68E4, 0x00CF68E4);
GAME_VAR(uint8_t, interactiveMusicDisabled, 0x00CF3AE7);
GAME_VAR(uint8_t, forcedEntrySet, 0x00CF3AF2);
GAME_VAR(uint32_t, musicNextUpdate, 0x00CF3AE8);
GAME_VAR(uint8_t, speechDisabledByActionTree, 0x00CF3DA3);
GAME_VAR(uint8_t, everythingCloser, 0x00CF6C70);
GAME_VAR(CPool<uint8_t>*, pedPool, 0x00C0F5F0);
GAME_VAR(uint8_t, volumeSfx, 0x00C18720);           // the frontend's levels (0..10)
GAME_VAR(int32_t, volumeMusic, 0x00C1871C);
GAME_VAR(int32_t, volumeSpeech, 0x00C18718);
constexpr uintptr_t kSleep = 0x008FC16C;
constexpr uintptr_t kEnterCriticalSection = 0x008FC17C;
constexpr uintptr_t kLeaveCriticalSection = 0x008FC178;

GAME_FN(void, __thiscall, Sound_Set, 0x0059D110, void* soundRef, void* sound);
GAME_FN(void*, __thiscall, Sounds_PlayAt, 0x0059B8A0, void* sounds, int id, const CVector* pos, int a3,
        const CVector* pos2, int a5, int a6, int a7, int a8, void* callback, void* entity, int a11, int a12,
        float a13, int a14);
GAME_FN(float, __thiscall, Sounds_DistanceSq, 0x0059B540, void* sounds, const CVector* pos);
GAME_FN(int32_t, __cdecl, HashStreamName, 0x005A7D60, const char* name, int arg);
GAME_FN(int, __thiscall, SoundBanks_Find, 0x00597FC0, void* banks, const char* name);
GAME_FN(void, __thiscall, SoundBanks_Request, 0x0059A860, void* banks, int bank, int time, int a);
GAME_FN(uint8_t, __thiscall, SoundBanks_IsLoaded, 0x005995F0, void* banks, int bank);
GAME_FN(float, __cdecl, VolumeTable_Get, 0x0059E4E0, int index);
GAME_FN(void, __cdecl, VolumeTable_Set, 0x0059E5F0, int index, float volume);
GAME_FN(void, __cdecl, VolumeTable_Init, 0x0059E490);
GAME_FN(void, __thiscall, Fn597E60, 0x00597E60, void* self);
GAME_FN(void, __thiscall, Fn59E7F0, 0x0059E7F0, void* self);
GAME_FN(void, __thiscall, Fn5A0570, 0x005A0570, void* self);
GAME_FN(void, __cdecl, Voices_Pause, 0x005AF9B0);
GAME_FN(void, __cdecl, Voices_Resume, 0x005AF9E0);
GAME_FN(void, __thiscall, StreamChannels_Pause, 0x005A60D0, void* channels);
GAME_FN(void, __thiscall, StreamChannels_Resume, 0x005A6100, void* channels);
GAME_FN(void, __thiscall, VoiceEngine_Pause, 0x005A96D0, void* engine, int on);
GAME_FN(void, __thiscall, Stream_StopAny, 0x00591480, void* stream, int priority);
GAME_FN(void, __cdecl, Speech_StopAll5A5DB0, 0x005A5DB0);
GAME_FN(void, __cdecl, Speech_Fn5A6070, 0x005A6070);
GAME_FN(void, __thiscall, Emitters_StopAll, 0x00587250, void* emitters);
GAME_FN(void, __thiscall, StreamChannels_Fn5A6250, 0x005A6250, void* channels, int);
GAME_FN(void, __thiscall, StreamChannels_Update, 0x005A64D0, void* channels);
GAME_FN(void, __thiscall, StreamChannels_Fn5A6170, 0x005A6170, void* channels, uintptr_t a, float b, int c, int d);
GAME_FN(void, __thiscall, StreamChannels_Fn5A6210, 0x005A6210, void* channels, int);
GAME_FN(void, __thiscall, StreamChannels_Fn5A6420, 0x005A6420, void* channels, int);
GAME_FN(void, __thiscall, StreamChannels_Play, 0x005A61F0, void* channels, int index);
GAME_FN(void, __thiscall, StreamChannels_StopAll, 0x005A6140, void* channels);
GAME_FN(void, __thiscall, StreamChannels_Init, 0x005A6640, void* channels);
GAME_FN(void, __thiscall, StreamChannels_Reset, 0x005A60C0, void* channels);
GAME_FN(void, __thiscall, Channel_Fn5A6D20, 0x005A6D20, void* channel, uintptr_t, int);
GAME_FN(void, __thiscall, Channel_Fn5A6EA0, 0x005A6EA0, void* channel);
GAME_FN(int32_t, __thiscall, Channel_Fn5A6DA0, 0x005A6DA0, void* channel);
inline char* __cdecl Strncpy(char* dst, const char* src, size_t n) { return crt::Strncpy(dst, src, n); }
GAME_FN(void, __cdecl, VolumeMode_Apply, 0x005A8FA0, int mode);
GAME_FN(uint32_t, __cdecl, Timer_GetCurrentTime, 0x0045BCA0);
GAME_FN(void*, __cdecl, Camera_Object, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, Node_WorldTransform, 0x006C9A90, void* node);
GAME_FN(void, __cdecl, LoadingScreen_Message, 0x0043C4F0, const char* title, const char* text, uintptr_t);
GAME_FN(void, __thiscall, Banks_InitArray, 0x00599780, void* banks);
GAME_FN(void*, __thiscall, AmbienceEmitters_Construct, 0x00586ED0, void* self, int, int, int, int);
GAME_FN(void, __thiscall, AmbienceEmitters_Reload, 0x00586FE0, void* self);
GAME_FN(void, __thiscall, AmbienceEmitters_Shutdown, 0x00587520, void* self);
GAME_FN(void, __thiscall, Banks_LoadTriggerBankFile, 0x00598AA0, void* banks);
GAME_FN(void*, __thiscall, Construct_Trivial, 0x0066BEA0, void* self);   // (returns this)
GAME_FN(void, __thiscall, Environment_Load, 0x00593800, void* environment);
GAME_FN(void, __thiscall, Environment_Init, 0x005937D0, void* environment);
GAME_FN(void, __thiscall, Environment_FreeAreasThunk, 0x00593770, void* environment);
GAME_FN(uint8_t, __thiscall, Ambience_Init, 0x00594900, void* ambience);
GAME_FN(void, __cdecl, InteractiveMusic_Load, 0x0058FDC0);
GAME_FN(int, __cdecl, LoadFile, 0x0042D180, const char* path, char* buffer, int size, const char* mode);
GAME_FN(uint8_t, __cdecl, Xact_Init, 0x005AF130, void* settings, int size);
GAME_FN(void, __cdecl, ShowSystemMessage, 0x0043B0F0, const char* key);
GAME_FN(void, __thiscall, Stream_Init, 0x0058EE20, void* stream, int channel);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void*, __thiscall, SfxQueue_Construct, 0x0059DEF0, void* self, int, int);
GAME_FN(void, __thiscall, SfxQueue_StopAll, 0x0059DE80, void* self);
GAME_FN(void, __thiscall, SfxQueue_DestructThunk, 0x0059DF40, void* self);
GAME_FN(void*, __thiscall, List58DE60_Construct, 0x0058DE60, void* self, int, int);
GAME_FN(void, __thiscall, List58DE60_StopAll, 0x0058DEC0, void* self);
GAME_FN(void, __thiscall, List58DE60_Destruct, 0x0058DFE0, void* self);
GAME_FN(void*, __thiscall, List58C3A0_Construct, 0x0058C3A0, void* self, int, int);
GAME_FN(void, __thiscall, List58C3A0_StopAll, 0x0058C400, void* self);
GAME_FN(void, __thiscall, List58C3A0_Destruct, 0x0058C470, void* self);
GAME_FN(void*, __thiscall, Obj58D870_Construct, 0x0058D870, void* self);
GAME_FN(void, __thiscall, Obj58D870_Destruct, 0x0058D8E0, void* self);
GAME_FN(void*, __thiscall, LoopSounds_Init, 0x00592600, void* self, int count, int32_t value);
GAME_FN(void, __thiscall, LoopSounds_StopAll, 0x00592390, void* self);
GAME_FN(void, __thiscall, LoopSounds_Shutdown, 0x00592810, void* self);
GAME_FN(void*, __thiscall, RatSounds_Init, 0x00593540, void* self, int count, int32_t value);
GAME_FN(void, __thiscall, RatSounds_StopAll, 0x00593510, void* self);
GAME_FN(void, __thiscall, RatSounds_Shutdown, 0x005935E0, void* self);
GAME_FN(void, __thiscall, Banks_LoadStatic, 0x0059AA30, void* banks);
GAME_FN(void, __thiscall, Banks_LoadAreaBankFile, 0x00599030, void* banks);
GAME_FN(void, __thiscall, Banks_LoadMissionBankFile, 0x00599210, void* banks);
GAME_FN(void, __thiscall, Banks_LoadWeatherBankFile, 0x00598CF0, void* banks);
GAME_FN(void, __thiscall, Banks_Reset, 0x00598970, void* banks);
GAME_FN(void, __thiscall, Banks_Shutdown, 0x0059A9A0, void* banks);
GAME_FN(void, __thiscall, SpeechLib_Setup, 0x005A5FF0, void* speech);
GAME_FN(void, __thiscall, Speech_Reset5A4D10, 0x005A4D10, void* speech);
GAME_FN(void, __thiscall, SpeechLib_Fn59FD50, 0x0059FD50, void* lib, int);
GAME_FN(void, __thiscall, FightMusic_Clear49, 0x0058E090, void* self);
GAME_FN(void, __thiscall, FightMusic_Construct, 0x0058E0B0, void* self);
GAME_FN(void, __cdecl, Music_StopForced, 0x00591A70, int fadeOut);
// its own
GAME_FN(int, __thiscall, Sounds_GetVolume, 0x00594E60, void* self, int category);
GAME_FN(void, __thiscall, Sounds_SetVolume, 0x00594ED0, void* self, int category, int level);
GAME_FN(void, __thiscall, FireAlarm_PlayOutside, 0x00595000, FireAlarm* self);
GAME_FN(void, __thiscall, FireAlarm_Stop, 0x005950B0, FireAlarm* self);
GAME_FN(int, __thiscall, Sounds_FindTriggerIndex, 0x00595310, void* self, uint32_t hash);
GAME_FN(void, __thiscall, Sounds_ResetTime, 0x00595A70, void* self);
GAME_FN(void, __thiscall, FireAlarm_Construct, 0x005958B0, FireAlarm* self);
GAME_FN(void, __thiscall, Sounds_Reset, 0x00596270, void* self);

const CVector* PositionOf(uint8_t* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return reinterpret_cast<const CVector*>(matrix ? matrix + 0x30 : entity + 4);
}

int32_t ReadVolumeMode() {
    vc::Import<void>(kEnterCriticalSection, volumeModeLock);
    int32_t mode = volumeMode;
    vc::Import<void>(kLeaveCriticalSection, volumeModeLock);
    return mode;
}

// ---- the loop slots and the volumes ----------------------------------------------------------

// 0x594DC0 (thiscall, ecx unused): the slots not kept this frame stopped
void __fastcall Sounds_UpdateLoopSlots(void*) {
    for (int i = 0; i < 48; i++) {
        LoopSlot& slot = loopSlots[i];
        if (slot.sound == nullptr) {
            continue;
        }
        if (slot.keep == 0) {
            slot.sound[0x5C] = 1;
            Sound_Set(&slot.sound, nullptr);
        } else {
            slot.keep = 0;
        }
    }
}

// 0x594E00 (thiscall): the members' sounds released
void __fastcall Sounds_ReleaseAll(uint8_t* self) {
    Sound_Set(self + 0x2B28, nullptr);
    Fn597E60(self + 0x2024);
    Fn59E7F0(self + 0x1070);
    Fn5A0570(self + 0x5D8);
    Sound_Set(self + 0x4AC, nullptr);
    for (int i = 5; i >= 0; i--) {
        Sound_Set(self + 8 + i * 4, nullptr);   // (the alarm's, last first)
    }
}

// 0x594E60 (thiscall, ecx unused): a category's volume as the menu's level (0..10)
int __fastcall Sounds_GetVolumeImpl(void*, re::Edx, int category) {
    VolumeTable_Get(category);   // (read and dropped)
    int index;
    switch (category) {
    case 1:
        index = 1;
        break;
    case 3:
    case 4:
    case 5:
        index = 4;
        break;
    default:
        index = 2;
        break;
    }
    float volume = VolumeTable_Get(index);
    return int32_t((volume + 0.05f) * 10.0f);   // (the x87 in single precision)
}

// 0x594ED0 (thiscall, ecx unused): a category's volume from the menu's level
void __fastcall Sounds_SetVolumeImpl(void*, re::Edx, int category, int level) {
    float volume = float(level) / 10.0f;
    switch (category) {
    case 0:
    case 2:
        VolumeTable_Set(2, volume);
        VolumeTable_Set(0, volume);
        volumeSfx = uint8_t(level);
        return;
    case 1:
        VolumeTable_Set(1, volume);
        volumeMusic = level;
        return;
    case 3:
    case 4:
    case 5:
        VolumeTable_Set(4, volume);
        VolumeTable_Set(5, volume);
        VolumeTable_Set(3, volume);
        volumeSpeech = level;
        return;
    default:
        return;
    }
}

// 0x594FA0 (thiscall)
void __fastcall Sounds_VolumeUp(void* self, re::Edx, int category) {
    int level = Sounds_GetVolume(self, category);
    if (level < 10) {
        Sounds_SetVolume(self, category, level + 1);
    }
}

// 0x594FD0 (thiscall)
void __fastcall Sounds_VolumeDown(void* self, re::Edx, int category) {
    int level = Sounds_GetVolume(self, category);
    if (level > 0) {
        Sounds_SetVolume(self, category, level - 1);
    }
}

// ---- the fire alarm ---------------------------------------------------------------------------------

// 0x595000 (thiscall): the bells outside within their range of the listener
void __fastcall FireAlarm_PlayOutsideImpl(FireAlarm* self) {
    for (int i = 0; i < 6; i++) {
        if (self->sounds[i] != nullptr) {
            continue;
        }
        uint32_t range = alarmRange;
        float distanceSq = Sounds_DistanceSq(theSounds, &alarmBells[i]);
        if (!(double(distanceSq) < double(int32_t(range * range)))) {
            continue;
        }
        void* sound = Sounds_PlayAt(theSounds, self->soundExterior, &alarmBells[i], 5, nullptr, 0, 0x78, 0, 0,
                                    nullptr, nullptr, 6, 1, 1.0f, 0);
        Sound_Set(&self->sounds[i], sound);
        if (self->sounds[i] != nullptr) {
            vc::Call<void>(self->sounds[i], 0x80, 0);
            vc::Call<void>(self->sounds[i], 0x98, 0);
        }
    }
}

// 0x5950B0 (thiscall)
void __fastcall FireAlarm_StopImpl(FireAlarm* self) {
    for (int i = 0; i < 6; i++) {
        if (self->sounds[i] != nullptr) {
            self->sounds[i][0x5C] = 1;
        }
        Sound_Set(&self->sounds[i], nullptr);
    }
    ambience[0x3A3D] = 0;
    ambience[0x3A3E] = 0;
    self->active = 0;
}

// 0x5958B0 (thiscall): the constructor
FireAlarm* __fastcall FireAlarm_ConstructImpl(FireAlarm* self) {
    for (int i = 0; i < 6; i++) {
        self->sounds[i] = nullptr;
    }
    for (int i = 0; i < 6; i++) {
        Sound_Set(&self->sounds[i], nullptr);
    }
    self->active = 0;
    self->bank = -1;
    self->soundInterior = HashStreamName("F_Alrm_On_Int", 1);
    self->soundExterior = HashStreamName("F_Alrm_On_Ext", 1);
    return self;
}

// 0x595920 (thiscall): the alarm on (and the PA waits for it)
void __fastcall FireAlarm_Start(FireAlarm* self) {
    if (self->active == 0) {
        self->bank = SoundBanks_Find(soundBanks, "SB_Int.bnk");
        if (self->interior != 0) {
            void* sound = Sounds_PlayAt(theSounds, self->soundInterior, nullptr, 1, nullptr, 0, 0x78, 0, 0, nullptr,
                                        nullptr, 6, 1, 1.0f, 0);
            Sound_Set(&self->sounds[0], sound);
        } else {
            FireAlarm_PlayOutside(self);
        }
    }
    ambience[0x3A3D] = 1;
    At<uint32_t>(ambience, 0x3A40) = m_snTimeInMilliseconds;
    self->active = 1;
}

// 0x5959B0 (thiscall): its bank kept, the sounds changed when the player goes in or out
void __fastcall FireAlarm_Update(FireAlarm* self) {
    self->interior = currArea != 0 ? 1 : 0;
    if (self->active == 0) {
        self->wasInterior = self->interior;
        return;
    }
    SoundBanks_Request(soundBanks, self->bank, 10000, 0);
    if (SoundBanks_IsLoaded(soundBanks, self->bank) == 0) {
        return;
    }
    if (self->interior != self->wasInterior) {
        FireAlarm_Stop(self);
        self->active = 1;
    }
    if (self->interior == 0) {
        FireAlarm_PlayOutside(self);
    } else if (self->sounds[0] == nullptr) {
        void* sound = Sounds_PlayAt(theSounds, self->soundInterior, nullptr, 1, nullptr, 0, 0x78, 0, 0, nullptr,
                                    nullptr, 6, 1, 1.0f, 0);
        Sound_Set(&self->sounds[0], sound);
    }
    self->wasInterior = self->interior;
}

// ---- pause, the speech stream, ids and triggers ------------------------------------------

// 0x595100 (thiscall)
void __fastcall Sounds_Pause(uint8_t* self) {
    Voices_Pause();
    StreamChannels_Pause(self + 0x4E4);
    VoiceEngine_Pause(voiceEngine, 1);
    self[0x35] = 1;
}

// 0x595130 (thiscall)
void __fastcall Sounds_Resume(uint8_t* self) {
    Voices_Resume();
    StreamChannels_Resume(self + 0x4E4);
    VoiceEngine_Pause(voiceEngine, 0);
    self[0x35] = 0;
}

// 0x595160 (thiscall): an id's index in the list (added when new; -1 when full)
int __fastcall Sounds_AddId(uint8_t* self, re::Edx, int32_t id) {
    int32_t& count = At<int32_t>(self, 0x3C);
    int32_t* ids = &At<int32_t>(self, 0x40);
    int i = 0;
    if (count > 0) {
        for (; i < count; i++) {
            if (ids[i] == id) {
                return i;
            }
        }
        if (i == 0x100) {
            return -1;
        }
    }
    ids[i] = id;
    count++;
    return i;
}

// 0x5951A0 (thiscall)
int32_t __fastcall Sounds_GetIdAt(uint8_t* self, re::Edx, int index) {
    if (index == -1) {
        return 0;
    }
    return At<int32_t>(self, 0x40 + index * 4);
}

// 0x5951C0 (thiscall): everything else stopped for a cutscene's speech stream (waits
// for both speech channels to be free)
void __fastcall Sounds_StartSpeechStream(uint8_t* self, re::Edx, int a, float b) {
    Stream_StopAny(ambientStream, 0);
    audioFlagCF4270 = 1;
    Speech_StopAll5A5DB0();
    FireAlarm_Stop(reinterpret_cast<FireAlarm*>(self + 8));
    Emitters_StopAll(At<void*>(self, 0x4D8));
    At<uint8_t>(At<uint8_t*>(self, 0x4D8), 0xA8) = 1;
    StreamChannels_Fn5A6250(self + 0x4E4, 1);
    while (!(*static_cast<int32_t*>(speechChannel0) == 2 && *static_cast<int32_t*>(speechChannel1) == 2)) {
        Channel_Fn5A6EA0(speechChannel0);
        Channel_Fn5A6EA0(speechChannel1);
        StreamChannels_Update(streamChannels);
        vc::Import<void>(kSleep, 5);
    }
    Channel_Fn5A6D20(speechChannel0, 0, 1);
    StreamChannels_Fn5A6170(streamChannels, a, b, 2, 0);
}

// 0x595290 (thiscall)
void __fastcall Sounds_EndSpeechStream(uint8_t* self) {
    StreamChannels_Fn5A6210(streamChannels, 2);
    StreamChannels_Fn5A6420(streamChannels, 1);
    self[0x4D0] = 0;
    At<uint8_t>(At<uint8_t*>(self, 0x4D8), 0xA8) = 0;
}

// 0x5952D0 (thiscall): (a debug name copied to a local and dropped: the game copies
// sixteen bytes from the argument's own stack slot)
void __fastcall Sounds_DebugName(uint8_t* self, re::Edx, const char* name) {
    if (self[0x39] != 0) {
        char buffer[0x14] = {};
        const char* slot = reinterpret_cast<const char*>(&name);
        std::memcpy(buffer, slot, sizeof(name));
        (void)buffer;
    }
}

// 0x595310 (thiscall): a trigger's index in the sorted list (-1: none). (The game's
// callers 0x595370 and 0x596120 rely on ecx surviving this call)
int __fastcall Sounds_FindTriggerIndexImpl(uint8_t* self, re::Edx, uint32_t hash) {
    int32_t count = At<int32_t>(self, 0x2354);
    if (count == 0) {
        return -1;
    }
    TriggerEntry* entries = &At<TriggerEntry>(self, 0x2358);
    int32_t low = 0;
    int32_t half = count;
    do {
        half >>= 1;
        int32_t mid = half + low;
        uint32_t h = entries[mid].hash;
        if (h == hash) {
            return mid;
        }
        if (h < hash) {
            low = mid + 1;
        }
    } while (half != 0 && low < count);
    return -1;
}

// 0x595370 (thiscall)
void __fastcall Sounds_SetTriggerValue(uint8_t* self, re::Edx, uint32_t hash, int32_t value) {
    int index = Sounds_FindTriggerIndex(self, hash);
    if (index != -1) {
        At<TriggerEntry>(self, 0x2358 + index * 8).value = value;
    }
}

// 0x5960A0 (thiscall): a trigger added to the sorted list
void __fastcall Sounds_AddTrigger(uint8_t* self, re::Edx, uint32_t hash) {
    if (Sounds_FindTriggerIndex(self, hash) != -1) {
        return;
    }
    int32_t count = At<int32_t>(self, 0x2354);
    TriggerEntry* entries = &At<TriggerEntry>(self, 0x2358);
    int i = 0;
    for (; i < count; i++) {
        if (entries[i].hash > hash) {
            std::memmove(&entries[i + 1], &entries[i], (count - i) * 8);
            break;
        }
    }
    entries[i].hash = hash;
    entries[i].value = -1;
    At<int32_t>(self, 0x2354)++;
}

// 0x596120 (thiscall): a trigger's value (-1: none)
int32_t __fastcall Sounds_FindTrigger(uint8_t* self, re::Edx, uint32_t hash) {
    int index = Sounds_FindTriggerIndex(self, hash);
    if (index == -1) {
        return -1;
    }
    return At<TriggerEntry>(self, 0x2358 + index * 8).value;
}

// ---- the skateboard ------------------------------------------------------------------------

// 0x595390 (thiscall; *this is the skater): the rolling sound for the surface under
// it and, turning, its turn sound ("TURNL_" left)
void __fastcall Sounds_SkateSounds(re::Ptr32<uint8_t>* self, re::Edx, int32_t* roll, int32_t* turnSound, float turn) {
    uint8_t surface = (*self)[0x1BE];
    *roll = 0;
    *turnSound = 0;
    char name[0x14];
    switch (surface) {
    case 7:
        *roll = HashStreamName("SKATE_SIDEWALK", 1);
        std::strcpy(name, "TURNR_SIDEWLK");
        break;
    case 8:
        *roll = HashStreamName("SKATE_CEMSTAIR", 1);
        std::strcpy(name, "TURNR_ROAD");
        break;
    case 10: case 0x26: case 0x27: case 0x28: case 0x29: case 0x2A: case 0x39: case 0x3A: case 0x3C:
        *roll = HashStreamName("SKATE_FLOOR", 1);
        std::strcpy(name, "TURNR_FLOOR");
        break;
    case 0xE: case 0x13: case 0x15: case 0x18: case 0x35: case 0x38:
        *roll = HashStreamName("SKATE_METAL", 1);
        std::strcpy(name, "TURNR_METAL");
        break;
    case 0x20: case 0x22: case 0x25:
        *roll = HashStreamName("SKATE_WOOD", 1);
        std::strcpy(name, "TURNR_WOOD");
        break;
    case 0x23:
        *roll = HashStreamName("SKATE_WOODSTAIR", 1);
        std::strcpy(name, "TURNR_WOOD");
        break;
    case 0x3B:
        *roll = HashStreamName("SKATE_DOCK", 1);
        std::strcpy(name, "TURNR_DOCK");
        break;
    default:
        *roll = HashStreamName("SKATE_ROAD", 1);
        std::strcpy(name, "TURNR_ROAD");
        break;
    }
    float amount = turn < 0.0f ? -turn : turn;
    if (!(double(amount) > 1e-5)) {
        return;
    }
    if (0.0f < turn) {
        name[4] = 'L';
    }
    *turnSound = HashStreamName(name, 1);
}

// ---- the listener -------------------------------------------------------------------------------

// 0x595630: where the audio listens from (the player, else the camera)
CVector* __cdecl GetCameraPosImpl(CVector* out) {
    if (theSounds[0] != 0 && playerPed != nullptr) {
        *out = *PositionOf(playerPed);
        return out;
    }
    const float* pos = reinterpret_cast<const float*>(Node_WorldTransform(Camera_Object(theCamera)) + 0x24);
    out->x = pos[0];
    out->y = pos[1];
    out->z = pos[2];
    return out;
}

// 0x5956A0 (thiscall, ecx unused): a footstep-like sound (the ones that follow the
// surface)
uint8_t __fastcall Sounds_IsFootstep(void*, re::Edx, int32_t soundId) {
    static const char* const kNames[] = {
        "Walk", "S_Walk", "P_Walk", "F_Walk", "Run", "P_Run", "Fast_Run", "Run_Stop", "Jump", "SkateOllieUp",
        "SkateOllieLand", "SkateLand", "SkateFall", "Bodyfall", "BikeJumpLand", "BikeJumpLndSoft",
        "SledgeHamrHit", "Scuff",
    };
    for (const char* name : kNames) {
        if (soundId == HashStreamName(name, 1)) {
            return 1;
        }
    }
    return 0;
}

// 0x595840 (thiscall): the volume mode (applied when it changes)
void __fastcall Sounds_SetVolumeMode(uint8_t* self, re::Edx, int32_t mode) {
    int32_t current = ReadVolumeMode();
    if (current != mode) {
        VolumeMode_Apply(mode);
    }
    At<int32_t>(self, 0x2014) = mode;
}

// 0x595880 (thiscall, ecx unused)
int32_t __fastcall Sounds_GetVolumeMode(void*) {
    return ReadVolumeMode();
}

// ---- time ----------------------------------------------------------------------------------

// 0x595A70 (thiscall)
void __fastcall Sounds_ResetTimeImpl(uint8_t* self) {
    uint32_t t = Timer_GetCurrentTime();
    uint32_t seconds = t / 1000;
    uint32_t ms = t - seconds * 1000;
    audioTimeCF4268 = (ms * 1000) / 1000 + seconds * 1000;
    audioTimeCF68DC = At<uint32_t>(self, 0x4C8);
    audioTimeCF426C = 0;
    audioTimeCF68E0 = 0;
    audioTimeCF68E4 = 0;
}

// 0x595AD0 (thiscall): waits (30 updates at most) for stream channel 2, starts it
void __fastcall Sounds_StartChannel2(uint8_t* self) {
    int tries = 0;
    int32_t ready = Channel_Fn5A6DA0(At<void*>(self, 0x4F0));
    while (ready == 0 && tries < 0x1E) {
        StreamChannels_Update(streamChannels);
        tries++;
        ready = Channel_Fn5A6DA0(At<void*>(self, 0x4F0));
    }
    StreamChannels_Play(streamChannels, 2);
    StreamChannels_Update(streamChannels);
    vc::Import<void>(kSleep, 10);
    Sounds_ResetTime(theSounds);
}

// 0x595B30 (thiscall): the audio's time (frozen while the game is)
int32_t __fastcall Sounds_UpdateTime(uint8_t* self) {
    if (simulationSpeed < 1e-6f) {
        At<int32_t>(self, 0x4C8) = At<int32_t>(self, 0x2B3C) - At<int32_t>(self, 0x2B40) - At<int32_t>(self, 0x4CC);
        return At<int32_t>(self, 0x4CC);
    }
    int32_t t = At<int32_t>(self, 0x2B3C) - At<int32_t>(self, 0x2B40) - At<int32_t>(self, 0x4C8);
    At<int32_t>(self, 0x4CC) = t;
    return t;
}

// 0x595B80 (thiscall, ecx unused): a sound's id by name
int32_t __fastcall Sounds_GetIdImpl(void*, re::Edx, const char* name) {
    return HashStreamName(name, 1);
}

// ---- setup and shutdown ----------------------------------------------------------------------

// 0x595BA0 (thiscall): the banks, the emitters, the environments, the ambience and the
// interactive music (the profiler's marks are the empty function)
void __fastcall Sounds_InitialiseBankManager(uint8_t* self) {
    const char* title = "cSCREAMAudioManager::InitialiseBankManager()";
    LoadingScreen_Message(title, "VolumeTable::Init()", 0);
    VolumeTable_Init();
    LoadingScreen_Message(title, "BankManager.InitBankArray()", 0);
    Banks_InitArray(self + 0x2024);
    LoadingScreen_Message(title, "EmitterQueue", 0);
    void* p = Malloc(0x13DC);
    void* emitters = p != nullptr ? AmbienceEmitters_Construct(p, 0x19, 0x38, 10, 0x18) : nullptr;
    At<void*>(self, 0x4D8) = emitters;
    AmbienceEmitters_Reload(emitters);
    LoadingScreen_Message(title, "BankManager.LoadTriggerBankFile()", 0);
    Banks_LoadTriggerBankFile(self + 0x2024);
    LoadingScreen_Message(title, "ReverbManager", 0);
    p = Malloc(0x34);
    void* environment = p != nullptr ? Construct_Trivial(p) : nullptr;
    At<void*>(self, 0x4C4) = environment;
    Environment_Load(environment);
    LoadingScreen_Message(title, "cSCREAMAmbienceManager", 0);
    p = Malloc(0x3A44);
    void* ambienceMgr = p != nullptr ? Construct_Trivial(p) : nullptr;
    At<void*>(self, 0x5D4) = ambienceMgr;
    Ambience_Init(ambienceMgr);
    LoadingScreen_Message(title, "InteractiveMusic::Init()", 0);
    InteractiveMusic_Load();
}

// 0x595D30 (thiscall): XACT's settings, the streams, the queues and lists, the banks
void __fastcall Sounds_Initialise(uint8_t* self) {
    if (self[0x2B38] != 0) {
        return;
    }
    char path[0x100];
    std::strcpy(path, "audio\\xact\\");
    std::strcat(path, "ALRMCLK.xgs");
    int size = LoadFile(path, scratchBuffer, 0x1C000, "r");
    if (size > 0 && Xact_Init(scratchBuffer, size) == 0) {
        ShowSystemMessage("SOUND_INIT_FAILED_MSG");
    }
    self[0] = 1;
    self[1] = 0;
    Sound_Set(self + 0x2B28, nullptr);
    for (int i = 0; i < 48; i++) {
        Sound_Set(&loopSlots[i].sound, nullptr);
        loopSlots[i].keep = 0;
    }
    const char* title = "cSCREAMAudioManager::Initialise()";
    LoadingScreen_Message(title, "m_StreamManager.Init()", 0);
    self[3] = 0;
    self[4] = 0;
    self[5] = 0;
    StreamChannels_Init(self + 0x4E4);
    LoadingScreen_Message(title, "m_MusicPlayer.Init(BULLY_MUSIC_STREAM)", 0);
    Stream_Init(self + 0x534, 0);
    Stream_Init(self + 0x584, 1);
    LoadingScreen_Message(title, "SFXQueue", 0);
    PushMemoryTag(0x2E);
    void* p = Malloc(0xAC);
    At<void*>(self, 0x2B2C) = p != nullptr ? SfxQueue_Construct(p, 0x28, 4) : nullptr;
    p = Malloc(0x28);
    At<void*>(self, 0x2B30) = p != nullptr ? List58DE60_Construct(p, 7, 0x40) : nullptr;
    p = Malloc(0x28);
    At<void*>(self, 0x2B34) = p != nullptr ? List58C3A0_Construct(p, 7, 0x74) : nullptr;
    p = Malloc(0x34);
    At<void*>(self, 0x4D4) = p != nullptr ? LoopSounds_Init(p, 10, 0x24) : nullptr;
    p = Malloc(0x34);
    At<void*>(self, 0x4DC) = p != nullptr ? RatSounds_Init(p, 10, 0x1C) : nullptr;
    p = Malloc(0x24);
    At<void*>(self, 0x4E0) = p != nullptr ? Obj58D870_Construct(p) : nullptr;
    PopMemoryTag();
    LoadingScreen_Message(title, "DumpAreaList()", 0);
    Environment_Init(At<void*>(self, 0x4C4));
    Banks_LoadStatic(self + 0x2024);
    Banks_LoadAreaBankFile(self + 0x2024);
    Banks_LoadMissionBankFile(self + 0x2024);
    Banks_LoadWeatherBankFile(self + 0x2024);
    SpeechLib_Setup(self + 0x5D8);
    FightMusic_Clear49(self + 0x440);
    if (ReadVolumeMode() != 1) {
        VolumeMode_Apply(1);
    }
    At<int32_t>(self, 0x2014) = 1;
    self[0x2B38] = 1;
}

// 0x596140 (thiscall): the area changed: everything positional stopped
void __fastcall Sounds_AreaChanged(uint8_t* self) {
    if (At<int32_t>(self, 0x2338) == currArea) {
        return;
    }
    Speech_StopAll5A5DB0();
    StreamChannels_Fn5A6250(self + 0x4E4, 0);
    List58C3A0_StopAll(At<void*>(self, 0x2B34));
    List58DE60_StopAll(At<void*>(self, 0x2B30));
    LoopSounds_StopAll(At<void*>(self, 0x4D4));
    Emitters_StopAll(At<void*>(self, 0x4D8));
    RatSounds_StopAll(At<void*>(self, 0x4DC));
    SfxQueue_StopAll(At<void*>(self, 0x2B2C));
}

// 0x5961B0 (thiscall): the constructor
uint8_t* __fastcall Sounds_Construct(uint8_t* self) {
    FireAlarm_Construct(reinterpret_cast<FireAlarm*>(self + 8));
    self[0x34] &= 0xC;
    FightMusic_Construct(self + 0x440);
    StreamChannels_Reset(self + 0x4E4);
    self[0x57C] = 0;
    self[0x5CC] = 0;
    Speech_Reset5A4D10(self + 0x5D8);
    SpeechLib_Fn59FD50(self + 0x1070, 0);
    Banks_Reset(self + 0x2024);
    At<int32_t>(self, 0x2B28) = 0;
    self[0x2B38] = 0;
    self[0x35] = 0;
    self[0x4D0] = 0;
    At<int32_t>(self, 0x3C) = 0;
    std::memset(self + 0x40, 0, 0x400);
    std::memset(self + 0x1D04, 0, 0x300);
    self[0x39] = 0;
    At<int32_t>(self, 0x201C) = 0;
    At<int32_t>(self, 0x2020) = 0;
    At<int32_t>(self, 0x2354) = 0;
    self[0x2B48] = 0;
    return self;
}

// 0x596270 (thiscall): the scripts' audio settings back to their defaults
void __fastcall Sounds_ResetImpl(uint8_t* self) {
    interactiveMusicDisabled = 0;
    if (forcedEntrySet != 0) {
        Music_StopForced(0);
    }
    speechDisabledByActionTree = 0;
    everythingCloser = 0;
    self[0] = 1;
    self[1] = 0;
    CPool<uint8_t>* pool = pedPool;
    for (int slot = pool->size; slot != 0;) {
        slot--;
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        uint8_t* ped = pool->objects + pool->elemSize * slot;
        if (ped != nullptr) {
            At<int32_t>(ped, 0x1518) = 0;
        }
    }
}

// 0x5962E0 (thiscall): everything stopped
void __fastcall Sounds_StopAll(uint8_t* self) {
    FireAlarm_Stop(reinterpret_cast<FireAlarm*>(theSounds + 8));
    Speech_StopAll5A5DB0();
    StreamChannels_StopAll(self + 0x4E4);
    List58C3A0_StopAll(At<void*>(self, 0x2B34));
    List58DE60_StopAll(At<void*>(self, 0x2B30));
    LoopSounds_StopAll(At<void*>(self, 0x4D4));
    Emitters_StopAll(At<void*>(self, 0x4D8));
    RatSounds_StopAll(At<void*>(self, 0x4DC));
    SfxQueue_StopAll(At<void*>(self, 0x2B2C));
    Sounds_Reset(self);
    musicNextUpdate = 0;
}

// 0x596360 (thiscall): everything stopped and freed
void __fastcall Sounds_Shutdown(uint8_t* self) {
    StreamChannels_StopAll(self + 0x4E4);
    Speech_Fn5A6070();
    FireAlarm_Stop(reinterpret_cast<FireAlarm*>(theSounds + 8));
    List58C3A0_StopAll(At<void*>(self, 0x2B34));
    List58DE60_StopAll(At<void*>(self, 0x2B30));
    LoopSounds_StopAll(At<void*>(self, 0x4D4));
    Emitters_StopAll(At<void*>(self, 0x4D8));
    RatSounds_StopAll(At<void*>(self, 0x4DC));
    SfxQueue_StopAll(At<void*>(self, 0x2B2C));
    Sounds_Reset(self);
    StreamChannels_Fn5A6210(streamChannels, 2);
    StreamChannels_Fn5A6420(streamChannels, 1);
    self[0x4D0] = 0;
    At<uint8_t>(At<uint8_t*>(self, 0x4D8), 0xA8) = 0;
    musicNextUpdate = 0;
    if (void* p = At<void*>(self, 0x4C4); p != nullptr) {
        Environment_FreeAreasThunk(p);
        Free(p);
    }
    At<void*>(self, 0x4C4) = nullptr;
    if (void* p = At<void*>(self, 0x5D4); p != nullptr) {
        Free(p);   // (its destructor is the empty function)
    }
    At<void*>(self, 0x5D4) = nullptr;
    if (void* p = At<void*>(self, 0x2B2C); p != nullptr) {
        SfxQueue_DestructThunk(p);
        Free(p);
    }
    At<void*>(self, 0x2B2C) = nullptr;
    if (void* p = At<void*>(self, 0x2B30); p != nullptr) {
        List58DE60_Destruct(p);
        Free(p);
    }
    At<void*>(self, 0x2B30) = nullptr;
    if (void* p = At<void*>(self, 0x2B34); p != nullptr) {
        List58C3A0_Destruct(p);
        Free(p);
    }
    At<void*>(self, 0x2B34) = nullptr;
    if (void* p = At<void*>(self, 0x4E0); p != nullptr) {
        Obj58D870_Destruct(p);
        Free(p);
    }
    At<void*>(self, 0x4E0) = nullptr;
    if (void* p = At<void*>(self, 0x4D4); p != nullptr) {
        LoopSounds_Shutdown(p);
        Free(p);
    }
    At<void*>(self, 0x4D4) = nullptr;
    if (void* p = At<void*>(self, 0x4D8); p != nullptr) {
        AmbienceEmitters_Shutdown(p);
        Free(p);
    }
    At<void*>(self, 0x4D8) = nullptr;
    if (void* p = At<void*>(self, 0x4DC); p != nullptr) {
        RatSounds_Shutdown(p);
        Free(p);
    }
    At<void*>(self, 0x4DC) = nullptr;
    Banks_Shutdown(self + 0x2024);
}

// ---- registration -----------------------------------------------------------------------------

#define SOUNDMGR_REVERSED(name, address, fn)                                                            \
    static void* RE_CONCAT(soundmgr_original_, __LINE__);                                                \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(soundmgr_original_, __LINE__), \
             re::Mode::Original)

SOUNDMGR_REVERSED("TheSounds::UpdateLoopSlots", 0x00594DC0, &Sounds_UpdateLoopSlots);
SOUNDMGR_REVERSED("TheSounds::ReleaseAll", 0x00594E00, &Sounds_ReleaseAll);
SOUNDMGR_REVERSED("TheSounds::GetVolume", 0x00594E60, &Sounds_GetVolumeImpl);
SOUNDMGR_REVERSED("TheSounds::SetVolume", 0x00594ED0, &Sounds_SetVolumeImpl);
SOUNDMGR_REVERSED("TheSounds::VolumeUp", 0x00594FA0, &Sounds_VolumeUp);
SOUNDMGR_REVERSED("TheSounds::VolumeDown", 0x00594FD0, &Sounds_VolumeDown);
SOUNDMGR_REVERSED("FireAlarm::PlayOutside", 0x00595000, &FireAlarm_PlayOutsideImpl);
SOUNDMGR_REVERSED("FireAlarm::Stop", 0x005950B0, &FireAlarm_StopImpl);
SOUNDMGR_REVERSED("TheSounds::Pause", 0x00595100, &Sounds_Pause);
SOUNDMGR_REVERSED("TheSounds::Resume", 0x00595130, &Sounds_Resume);
SOUNDMGR_REVERSED("TheSounds::AddId", 0x00595160, &Sounds_AddId);
SOUNDMGR_REVERSED("TheSounds::GetIdAt", 0x005951A0, &Sounds_GetIdAt);
SOUNDMGR_REVERSED("TheSounds::StartSpeechStream", 0x005951C0, &Sounds_StartSpeechStream);
SOUNDMGR_REVERSED("TheSounds::EndSpeechStream", 0x00595290, &Sounds_EndSpeechStream);
SOUNDMGR_REVERSED("TheSounds::DebugName", 0x005952D0, &Sounds_DebugName);
// callers 0x595370 and 0x596120 rely on ecx: turn on together with them
SOUNDMGR_REVERSED("TheSounds::FindTriggerIndex", 0x00595310, &Sounds_FindTriggerIndexImpl);
SOUNDMGR_REVERSED("TheSounds::SetTriggerValue", 0x00595370, &Sounds_SetTriggerValue);
SOUNDMGR_REVERSED("TheSounds::SkateSounds", 0x00595390, &Sounds_SkateSounds);
SOUNDMGR_REVERSED("TheSounds::GetListenerPos", 0x00595630, &GetCameraPosImpl);
SOUNDMGR_REVERSED("TheSounds::IsFootstep", 0x005956A0, &Sounds_IsFootstep);
SOUNDMGR_REVERSED("TheSounds::SetVolumeMode", 0x00595840, &Sounds_SetVolumeMode);
SOUNDMGR_REVERSED("TheSounds::GetVolumeMode", 0x00595880, &Sounds_GetVolumeMode);
SOUNDMGR_REVERSED("FireAlarm::FireAlarm", 0x005958B0, &FireAlarm_ConstructImpl);
SOUNDMGR_REVERSED("FireAlarm::Start", 0x00595920, &FireAlarm_Start);
SOUNDMGR_REVERSED("FireAlarm::Update", 0x005959B0, &FireAlarm_Update);
SOUNDMGR_REVERSED("TheSounds::ResetTime", 0x00595A70, &Sounds_ResetTimeImpl);
SOUNDMGR_REVERSED("TheSounds::StartChannel2", 0x00595AD0, &Sounds_StartChannel2);
SOUNDMGR_REVERSED("TheSounds::UpdateTime", 0x00595B30, &Sounds_UpdateTime);
SOUNDMGR_REVERSED("TheSounds::GetSoundId", 0x00595B80, &Sounds_GetIdImpl);
SOUNDMGR_REVERSED("TheSounds::InitialiseBankManager", 0x00595BA0, &Sounds_InitialiseBankManager);
SOUNDMGR_REVERSED("TheSounds::Initialise", 0x00595D30, &Sounds_Initialise);
SOUNDMGR_REVERSED("TheSounds::AddTrigger", 0x005960A0, &Sounds_AddTrigger);
SOUNDMGR_REVERSED("TheSounds::FindTrigger", 0x00596120, &Sounds_FindTrigger);
SOUNDMGR_REVERSED("TheSounds::AreaChanged", 0x00596140, &Sounds_AreaChanged);
SOUNDMGR_REVERSED("TheSounds::TheSounds", 0x005961B0, &Sounds_Construct);
SOUNDMGR_REVERSED("TheSounds::Reset", 0x00596270, &Sounds_ResetImpl);
SOUNDMGR_REVERSED("TheSounds::StopAll", 0x005962E0, &Sounds_StopAll);
SOUNDMGR_REVERSED("TheSounds::Shutdown", 0x00596360, &Sounds_Shutdown);
RE_UNHOOKED("AudioEnvironment::FreeAreasThunk", 0x00593770);   // (jmp 0x593700)
RE_UNHOOKED("SfxQueue::DestructThunk", 0x0059DF40);             // (jmp 0x59DE80)

} // namespace
