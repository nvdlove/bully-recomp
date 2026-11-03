#pragma once

// The vehicles' engine and tyre sounds (0x5884B0..0x58C500): one VehicleAudio a car
// (0x74 bytes) in the manager at 0xCF68D4, and the shared declarations of
// VehicleAudio.cpp / VehicleAudio2.cpp. The game's sound calls are declared here once.

#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace vaudio {

struct CVector {
    float x, y, z;
};

// a CEntity reference passed by value (callee-destroyed): SetRef(&ref, nullptr) at the end
struct EntityRef {
    re::Ptr32<void> entity;
};

struct VehicleAudio {
    re::Ptr32<void> vehicle;          // +0 (a reference)
    re::Ptr32<void> sound4;           // +4 the idle / idle loop
    re::Ptr32<void> sound8;           // +8 the cruise / engine loop
    re::Ptr32<void> soundC;           // +0xC reverse / stop
    re::Ptr32<void> sound10;          // +0x10 passby, tyre skid, button sounds
    re::Ptr32<void> sound14;          // +0x14 tyre turns / engine stop
    re::Ptr32<void> sound18;          // +0x18 the radio beats / burn-out / stop
    re::Ptr32<void> sound1C;          // +0x1C the siren / horn
    uint32_t nextCurbBump;  // +0x20 (time)
    int32_t pitchOffset;    // +0x24
    int32_t idleSoundId;    // +0x28
    int32_t cruiseSoundId;  // +0x2C
    uint8_t bikeBanks;      // +0x30 the bike banks were unloaded for it
    uint8_t pad31[3];
    float distance;         // +0x34 from the camera, last update
    int32_t index;          // +0x38 its slot (-1 free)
    int32_t unknown3C;      // +0x3C
    int32_t surface[4];     // +0x40 each wheel's last surface
    float turn;             // +0x50 (the scooter's smoothed values)
    float lean;             // +0x54
    uint32_t startTime;     // +0x58
    float lastF5C;          // +0x5C
    float maxF60;           // +0x60
    int32_t bank;           // +0x64 its own bank (-1: Engine.bnk)
    uint8_t bankLoaded;     // +0x68
    uint8_t pad69[3];
    int32_t unknown6C;      // +0x6C
    uint8_t unknown70;      // +0x70
    uint8_t pad71[3];
};
static_assert(offsetof(VehicleAudio, nextCurbBump) == 0x20);
static_assert(offsetof(VehicleAudio, surface) == 0x40);
static_assert(offsetof(VehicleAudio, bank) == 0x64);
static_assert(sizeof(VehicleAudio) == 0x74);

struct VehicleAudioManager {
    re::Ptr32<VehicleAudio> entries[7];  // +0
    int32_t freeHint;          // +0x1C
    int32_t count;             // +0x20
    int32_t unknown24;         // +0x24
};
static_assert(sizeof(VehicleAudioManager) == 0x28);

struct VoiceList {
    re::Ptr32<int> data;
    uint32_t unknown4;
    uint32_t count;
};

using re::At;

// the entity's position (its matrix's, else its placement's)
inline CVector* EntityPos(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return reinterpret_cast<CVector*>(matrix ? matrix + 0x30 : static_cast<uint8_t*>(entity) + 4);
}

template <typename R, typename... A>
inline R VCall(void* object, int offset, A... args) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(fn)(object, args...);
}

inline void* const TheSounds = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const soundBanks = reinterpret_cast<void*>(0x00CF5DC4);
GAME_VAR(void*, playerPed, 0x00C1AEA8);
GAME_VAR(void*, sfxManager, 0x00CF68CC);
GAME_VAR(VehicleAudioManager*, vehicleAudios, 0x00CF68D4);
GAME_VAR(uint32_t, timeMs, 0x00C1A9B4);
GAME_VAR(uint8_t, transitionDisplayed, 0x020C8824);
GAME_VAR(float, soundTimeStep, 0x00CF60F0);
GAME_VAR(float, passbyScale, 0x00C1A9A4);

GAME_FN(void, __thiscall, Sound_Set, 0x0059D110, re::Ptr32<void>* sound, void* value);
GAME_FN(void, __thiscall, Sound_Construct, 0x0042D6D0, re::Ptr32<void>* sound, void* value);
GAME_FN(uint8_t, __thiscall, Sound_IsActive, 0x0059CF10, void* sound);
GAME_FN(void, __thiscall, SetRef, 0x005D7F10, re::Ptr32<void>* ref, void* entity);
GAME_FN(void*, __thiscall, Sounds_PlayAt, 0x0059B8A0, void* sounds, int id, const CVector* pos, int a3, const CVector* pos2,
        int a5, int a6, int a7, int a8, void* callback, void* entity, int a11, int a12, float a13, int a14);
GAME_FN(void*, __thiscall, Sounds_Play, 0x0059BAB0, void* sounds, int id, int volume, int, int pitch);
GAME_FN(int, __thiscall, Sounds_GetId, 0x00595B80, void* sounds, const char* name);
GAME_FN(void*, __thiscall, Sounds_Fn59B740, 0x0059B740, void* sounds, const CVector* pos, uintptr_t);
GAME_FN(void*, __thiscall, Sfx_Play, 0x0059E060, void* sfx, int id, int volume, void* at, int, int, uintptr_t, int, int, int, int,
        uintptr_t, uintptr_t, int one, int six);  // (at: -1 or from Sounds_Fn59B740)
GAME_FN(int, __thiscall, SoundBanks_Find, 0x00597FC0, void* banks, const char* name);
GAME_FN(void, __thiscall, SoundBanks_Request, 0x0059A860, void* banks, int bank, int timeMs, int arg);
GAME_FN(uint8_t, __thiscall, SoundBanks_IsLoaded, 0x005995F0, void* banks, int bank);
GAME_FN(uint8_t, __thiscall, SoundBanks_Fn598A50, 0x00598A50, void* banks, int bank);
GAME_FN(void, __thiscall, SoundBanks_Load, 0x0059A3E0, void* banks, int bank, int, int);
GAME_FN(void, __thiscall, SoundBanks_Unload, 0x0059A420, void* banks, int bank, int, int);
GAME_FN(void, __cdecl, SoundSlot_Play597010, 0x00597010, re::Ptr32<void>* sound, int id, const CVector* pos, int, int volume,
        int pitch, int, int);
GAME_FN(void, __cdecl, SoundSlot_Play597310, 0x00597310, re::Ptr32<void>* sound, int id, const CVector* pos, int, int volume, int,
        int);
GAME_FN(void, __cdecl, SoundSlot_Play597400, 0x00597400, re::Ptr32<void>* sound, int id, int, int, int volume, int pitch, int);
GAME_FN(uint8_t, __cdecl, Audio_Approach, 0x0059CF30, int* value, int target, int step);
GAME_FN(uint8_t, __cdecl, Audio_ApproachF, 0x0059CFC0, float* value, float target, float step);
GAME_FN(int, __cdecl, SFX_Fn59E3E0, 0x0059E3E0, float distance, int);
GAME_FN(float, __cdecl, Audio_Fn59E4F0, 0x0059E4F0, float value, int);
GAME_FN(float, __cdecl, Audio_Fn5A7F00, 0x005A7F00, float, float);
GAME_FN(float, __cdecl, Audio_Fn5A7EA0, 0x005A7EA0, int);
GAME_FN(void, __thiscall, Sound_GetVoices, 0x0059CF20, void* sound, VoiceList* list);
GAME_FN(uint8_t, __cdecl, Voice_Fn5AF530, 0x005AF530, int voice);
GAME_FN(void, __cdecl, Voice_Fn5AF870, 0x005AF870, int voice, int volume, float);
GAME_FN(void, __cdecl, Voice_Fn5AF7B0, 0x005AF7B0, int voice, int volume, int, int);
GAME_FN(void, __cdecl, Voice_Fn5AF660, 0x005AF660, int voice, float);
GAME_FN(CVector*, __cdecl, GetCameraPos, 0x00595630, CVector* out);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorScale, 0x00412350, CVector* out, const CVector* v, float s);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Magnitude2D, 0x004246B0, const CVector* v);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const CVector* v);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(float, __cdecl, MinF, 0x00414470, float a, float b);
GAME_FN(float, __cdecl, MaxF, 0x00414490, float a, float b);
GAME_FN(float*, __thiscall, Entity_GetMatrix, 0x008B6120, void* entity);
GAME_FN(int32_t*, __thiscall, Vehicle_GetVehicleClass, 0x004CAF30, void* vehicle, int32_t* out);
GAME_FN(uint8_t, __thiscall, Vehicle_HasSirenModel, 0x004CB1F0, void* vehicle);
GAME_FN(void*, __cdecl, Vehicle_AsAutomobile, 0x004864B0, void* entity);
GAME_FN(void*, __thiscall, Seats_GetOccupant, 0x004CE560, void* seats, int seat);
GAME_FN(uint8_t, __thiscall, Seats_IsDriver, 0x004CEA70, void* seats, void* ped);
GAME_FN(int, __cdecl, RandomInt, 0x0040DA80, int low, int high);
GAME_FN(uint8_t, __cdecl, IsPlayerInFastVehicle, 0x004469A0);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int input, int pad);
GAME_FN(uint8_t, __cdecl, Pad_JustPressed, 0x00738900, int input, int pad);
GAME_FN(uint8_t, __cdecl, Pad_JustReleased, 0x00738940, int input, int pad);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);

// (VehicleAudio's own, where modes apply; Init 0x588820 and PlayCollision 0x589B00 take a
// reference by value: called through VehicleAudio2.cpp's asm helpers)
GAME_FN(void*, __thiscall, SoundPtr_Construct, 0x005884B0, re::Ptr32<void>* self, void* sound);
GAME_FN(void*, __thiscall, SoundPtr_Assign, 0x005884D0, re::Ptr32<void>* self, const re::Ptr32<void>* other);
GAME_FN(float, __thiscall, Minigame_Fn5884F0, 0x005884F0, void* minigame);
GAME_FN(VehicleAudio*, __thiscall, VehicleAudio_Construct, 0x00588520, VehicleAudio* self);
GAME_FN(void, __thiscall, VehicleAudio_Destruct, 0x005885E0, VehicleAudio* self);
GAME_FN(void, __thiscall, VehicleAudio_Reset, 0x00588640, VehicleAudio* self);
GAME_FN(void, __thiscall, VehicleAudio_UpdateArcade, 0x00588D00, VehicleAudio* self);
GAME_FN(void, __thiscall, VehicleAudio_UpdateEngine, 0x00589390, VehicleAudio* self, float delta);
GAME_FN(void, __thiscall, VehicleAudio_UpdateSmallEngine, 0x00589790, VehicleAudio* self);
GAME_FN(void, __thiscall, VehicleAudio_UpdateTyres, 0x0058AB90, VehicleAudio* self);
GAME_FN(void, __thiscall, VehicleAudio_UpdateGoKart, 0x0058AE70, VehicleAudio* self);
GAME_FN(void, __thiscall, VehicleAudio_UpdateScooter, 0x0058B610, VehicleAudio* self);
GAME_FN(void, __thiscall, VehicleAudio_Update, 0x0058BF90, VehicleAudio* self);
GAME_FN(VehicleAudio*, __thiscall, VehicleAudios_Find, 0x0058DEF0, VehicleAudioManager* self, void* vehicle);
GAME_FN(int, __thiscall, VehicleAudios_FindFree, 0x0058DF60, VehicleAudioManager* self);
GAME_FN(void, __thiscall, VehicleAudios_Remove, 0x0058C430, VehicleAudioManager* self, void* vehicle);

constexpr uintptr_t kTyreTurnsCallback = 0x0058A740;
constexpr uintptr_t kDozerCallback = 0x0058A9D0;

// a sound told to stop (when it's playing or says so)
inline void StopSound(void* sound) {
    if (sound && (Sound_IsActive(sound) || VCall<uint8_t>(sound, 0x50))) {
        At<uint8_t>(sound, 0x5C) = 1;
    }
}

// a sound cut (its method 0x34 while it plays) and let go
inline void CutSound(re::Ptr32<void>* sound) {
    if (*sound && Sound_IsActive(*sound)) {
        VCall<void>(*sound, 0x34);
    }
    Sound_Set(sound, nullptr);
}

inline int VehicleClass(void* vehicle) {
    int32_t out;
    return *Vehicle_GetVehicleClass(vehicle, &out);
}

inline float UnsignedToFloat(int value) {
    return float(uint32_t(value));
}

// (x * n) / 1024 rounded toward zero, as the game's shifts do
inline int Scale1024(int x, int n) {
    int v = x * n;
    return (v + ((v >> 31) & 0x3FF)) >> 10;
}

} // namespace vaudio
