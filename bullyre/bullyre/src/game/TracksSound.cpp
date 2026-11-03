// The action tree's sound and speech tracks (0x618780..0x61A300): speech filters,
// ambience, music streams, timed bank loads, positional / 2D / looping / parametric
// sounds, speech events (with the five follow-up slots at 0x1B655D0), crowd reactions
// and social speech: their field initialisers, destructors and Start / Update virtuals.

#include <cstdint>

#include "game/ActionTracksCommon.h"

#include "game/Crt.h"
#include "reversed.h"
using namespace actc;

namespace {

constexpr uint32_t kVtSoundClearSpeechFilterTrack = 0x00932D5C;
constexpr uint32_t kVtSoundSetSpeechPriorityFilterTrack = 0x00932D74;
constexpr uint32_t kVtSoundAddSpeechPriorityFilterTrack = 0x00932D8C;
constexpr uint32_t kVtSoundRemoveSpeechPriorityFilterTrack = 0x00932DA4;
constexpr uint32_t kVtSoundAmbienceOnTrack = 0x00932DBC;
constexpr uint32_t kVtSoundAmbienceOffTrack = 0x00932DD4;
constexpr uint32_t kVtPlayPreloadStreamTrack = 0x00932DEC;
constexpr uint32_t kVtSoundStreamTrack = 0x00932E04;
constexpr uint32_t kVtSoundStreamFadeoutTrack = 0x00932E1C;
constexpr uint32_t kVtSoundTimedBankLoadTrack = 0x00932E34;
constexpr uint32_t kVtSoundTrack = 0x00932E90;
constexpr uint32_t kVtSoundLoopTrack = 0x00932EA8;
constexpr uint32_t kVtCleanupDeadSoundLoopsTrack = 0x00932EC0;
constexpr uint32_t kVtSpeechTrack = 0x00932ED8;
constexpr uint32_t kVtSpeechStartTrack = 0x00932EF0;
constexpr uint32_t kVtRemoveAllQueuedSpeechTrack = 0x00932F08;
constexpr uint32_t kVtBikeSoundStateTrack = 0x00932F20;
constexpr uint32_t kVtCrowdCheerTrack = 0x00932F38;
constexpr uint32_t kVtCrowdBooTrack = 0x00932F50;
constexpr uint32_t kVtCrowdExciteTrack = 0x00932F68;
constexpr uint32_t kVtCrowdRelaxTrack = 0x00932F80;
constexpr uint32_t kVtAISocialSpeechEventTrack = 0x00932F98;
constexpr uint32_t kVtPlayerSocialSpeechEventTrack = 0x00932FB0;

// a speech's follow-up (the owner stops or forgets the ped when its time is up)
struct SpeechSlot {
    int32_t kind;       // +0 (1: forget, 2: stop)
    uint8_t active;     // +4
    uint8_t started;    // +5
    uint8_t pad6[2];
    uint32_t until;     // +8 ms
    re::Ptr32<uint8_t> ped;       // +0xC a registered reference
};
static_assert(sizeof(SpeechSlot) == 0x10);
GAME_ARRAY(SpeechSlot, speechSlots, 0x01B655D0);   // [5]
constexpr int kSpeechSlots = 5;

// a stream emitter's queued sound
struct QueuedStream {
    V3 pos;            // +0
    float volume;      // +0xC
    uint8_t flags;     // +0x10 (bit 2: loop; bits 1, 3 cleared)
    uint8_t pad11[3];
    uint32_t flags2;   // +0x14 (bit 0 set, bits 1..3 cleared)
    int32_t hash;      // +0x18
};
static_assert(sizeof(QueuedStream) == 0x1C);

inline void* const speech = reinterpret_cast<void*>(0x00CF4378);
inline void* const musicStream = reinterpret_cast<void*>(0x00CF42D4);
inline void* const streamEmitters = reinterpret_cast<void*>(0x00AE5F60);
inline void* const soundBanks = reinterpret_cast<void*>(0x00CF5DC4);
inline void* const sounds = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const speechLib = reinterpret_cast<void*>(0x00CF4E10);
inline void* const texts = reinterpret_cast<void*>(0x020C2FA4);
inline void* const missionManager = reinterpret_cast<void*>(0x020C3CA0);
inline void* const areas = reinterpret_cast<void*>(0x00BCC580);
inline uint32_t* const shopDoorNode = reinterpret_cast<uint32_t*>(0x01B652D0);
inline const char* const bankWarning = reinterpret_cast<const char*>(0x00932E48);
inline const char* const missionKey = reinterpret_cast<const char*>(0x0091DFD4);   // "3_04"
inline const char* const shopBell = reinterpret_cast<const char*>(0x00932FC4);     // "ShopBell"
inline const char* const bikeCrash = reinterpret_cast<const char*>(0x0090E18C);    // "Bike_Crash"
inline const char* const diveSplash = reinterpret_cast<const char*>(0x0091CBFC);   // "Dive_Splash"
GAME_VAR(uint8_t*, ambience, 0x00CF4374);
GAME_VAR(uint8_t*, loopSounds, 0x00CF4274);
GAME_VAR(uint8_t*, ambienceEmitters, 0x00CF4278);
GAME_VAR(uint8_t*, boxingCrowd, 0x00CF4280);
GAME_VAR(uint8_t, musicDuringCutscenes, 0x00CF3DA4);
GAME_VAR(float, parametricSoundDelay, 0x00B0BCC4);
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(uint8_t*, dialogState, 0x01B8919C);
GAME_VAR(uint8_t, socialSubtitles, 0x00A33938);
GAME_VAR(uint32_t, aiSubtitleTime, 0x01B65568);
GAME_VAR(uint32_t, playerSubtitleTime, 0x01B6556C);
GAME_VAR(uint8_t*, hudText, 0x00C67548);
GAME_VAR(uint32_t, lastScriptedSpeech, 0x00CF4E00);

GAME_FN(void, __thiscall, Speech_ResetFilter, 0x005A09A0, void* speech);
GAME_FN(void, __thiscall, Speech_Nop8A20A0, 0x008A20A0, void* speech, int);
GAME_FN(void, __thiscall, MusicStream_PlayPreloaded, 0x005905E0, void* stream, uint32_t name);
GAME_FN(void, __thiscall, MusicStream_StopAny, 0x00591480, void* stream, int);
GAME_FN(void, __thiscall, MusicStream_PlayByName, 0x00591330, void* stream, const char* name, float volume, int flag,
        int priority);
GAME_FN(void, __thiscall, MusicStream_PreloadByName, 0x00591380, void* stream, const char* name, float volume,
        int flag, int priority, int fadeIn, int fadeOut);
GAME_FN(void, __thiscall, MusicStream_Stop, 0x005913D0, void* stream, int, int);
GAME_FN(int32_t, __cdecl, HashStreamName, 0x005A7D60, const char* name, int);
GAME_FN(void, __thiscall, StreamEmitter_Unqueue, 0x00587040, void* emitters, int32_t hash);
GAME_FN(void, __thiscall, StreamEmitter_Queue, 0x00587000, void* emitters, const QueuedStream* entry);
GAME_FN(void, __cdecl, SoundPreloadStream_6C3B10, 0x006C3B10, const char* name, float volume, int);
GAME_FN(int, __thiscall, Banks_Find, 0x00597FC0, void* banks, const char* name);
GAME_FN(void, __cdecl, DebugPrintf, 0x0049A650, int, const char* format, const char* name);
GAME_FN(void, __thiscall, Banks_Request, 0x0059A860, void* banks, int bank, int, int);
GAME_FN(int32_t, __thiscall, Audio_GetSoundId, 0x00595B80, void* audio, const char* name);
GAME_FN(void*, __thiscall, Sounds_PlayAt, 0x0059B8A0, void* sounds, int id, const V3* pos, int a3, const void* pos2,
        int a5, int a6, int a7, int a8, void* callback, void* entity, int a11, int a12, float a13, int a14);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void, __thiscall, AmbienceEmitters_Enable, 0x005873C0, uint8_t* emitters, uint32_t hash, int enable);
GAME_FN(void, __thiscall, BoxingCrowd_PlayCheer, 0x0058D940, uint8_t* crowd);
GAME_FN(void, __thiscall, BoxingCrowd_PlayBoo, 0x0058D900, uint8_t* crowd);
GAME_FN(void, __thiscall, BoxingCrowd_Raise, 0x0058DA20, uint8_t* crowd);
GAME_FN(void, __thiscall, BoxingCrowd_Lower, 0x0058DA50, uint8_t* crowd);
GAME_FN(void, __cdecl, Sfx_SetCategoryFollowsFade, 0x0059E5E0, int category, int follows);
GAME_FN(uint8_t*, __cdecl, UsePoint_GetEntity, 0x004EABB0, uint8_t* owner);
GAME_FN(void, __thiscall, LoopSounds_PlayOn, 0x00592F30, uint8_t* sounds, uint8_t* ped, int32_t id, uint32_t volume,
        uint8_t* entity);
GAME_FN(void, __thiscall, LoopSounds_PlayAtEntity, 0x00592FA0, uint8_t* sounds, uint8_t* entity, int32_t id,
        uint32_t volume, uint8_t* owner);
GAME_FN(void, __thiscall, LoopSounds_PlayAt, 0x00593010, uint8_t* sounds, int32_t id, const float* pos, uint32_t volume,
        uint8_t* owner);
GAME_FN(void, __thiscall, LoopSounds_Stop, 0x00592400, uint8_t* sounds, uint8_t* ped, uint8_t* entity, int32_t id);
GAME_FN(void, __thiscall, LoopSounds_StopForPed, 0x005924E0, uint8_t* sounds, uint8_t* ped, int);
GAME_FN(void, __thiscall, Speech_StopPed, 0x005A59D0, void* speech, uint8_t* ped, int32_t reason, uint32_t priority);
GAME_FN(void, __thiscall, Speech_ForgetPed, 0x005A0AB0, void* speech, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Speech_PassesPedFlag, 0x005A21B0, void* speech, uint8_t* ped, int id);
GAME_FN(void*, __thiscall, Speech_Add, 0x005A4920, void* speech, void* out, uint8_t* ped, void* target, int, int id,
        const void* callback, SpeechSlot* slot, int flag8, int hasPosition, int32_t variant, const V3* position);
GAME_FN(void*, __thiscall, Speech_AddScripted, 0x005A4AD0, void* speech, void* out, int id, uint32_t variant,
        uint8_t* ped, uint32_t distance, const void* callback, SpeechSlot* slot, int urgent, int flag9, int calm);
GAME_FN(uint8_t, __cdecl, Dodgeball_IsActive, 0x00683A30);
GAME_FN(uint32_t*, __thiscall, NameKey_ConstructFrom, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(int32_t, __thiscall, Missions_Find, 0x006A9FD0, void* manager, uint32_t* key);
GAME_FN(uint8_t*, __thiscall, Missions_GetController1, 0x006AA690, void* manager);
GAME_FN(uint8_t, __thiscall, MissionSlot_Is, 0x006AA410, uint8_t* slot, int32_t id);
GAME_FN(uint8_t*, __thiscall, Ped_GetTarget, 0x00497100, uint8_t* ped);
GAME_FN(void, __thiscall, EntityRef_Set, 0x005D7F10, re::Ptr32<uint8_t>* ref, uint8_t* entity);
GAME_FN(void, __thiscall, SpeechLib_RemoveModel, 0x0059F8D0, void* lib, int model, int, int);
GAME_FN(uint8_t, __thiscall, Social_PlayAnswer, 0x0048F3C0, uint8_t* social, int, int);
GAME_FN(uint8_t, __thiscall, Social_OtherPlaysAnswer, 0x0048F5D0, uint8_t* social, int, int);
GAME_FN(const void*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __thiscall, HUDText_SetText, 0x0055B130, uint8_t* hud, uint32_t a, uintptr_t key);
GAME_FN(uint8_t, __thiscall, ModelInfo_IsDoor, 0x00520440, uint8_t* modelInfo);
GAME_FN(void, __thiscall, PropAnim_SetCurrent, 0x004ED940, uint8_t* prop);
GAME_FN(uint8_t, __thiscall, Controller_IsPlaying, 0x005F4800, uint8_t* controller, const uint32_t* name, int, int);
GAME_FN(uint8_t, __thiscall, Areas_GetTriggerRouteArea, 0x004153B0, void* areas, uintptr_t prop);
GAME_FN(uint8_t, __thiscall, Areas_IsShop, 0x004153F0, void* areas, int area);
GAME_FN(uint8_t, __thiscall, Sounds_IsFootstep, 0x005956A0, void* sounds, int32_t id);
GAME_FN(void, __thiscall, Footsteps_Play, 0x0059CC00, void* sounds, uint8_t* ped, int32_t id, uint32_t a, float,
        int);
GAME_FN(uint8_t*, __thiscall, Blend_GetAnim, 0x006B1F50, uint8_t* blend, int32_t slot);
GAME_FN(void, __thiscall, SoundTrack_Play, 0x00619E10, uint8_t* self);

int32_t SoundId(const char* name) {
    return Audio_GetSoundId(sounds, name);
}
// ms from now + seconds (the x87's single rounding of the sum, then _ftol)
uint32_t TimeAfter(float seconds) {
    float t = float(double(seconds * 1000.0f) + double(timeInMs));
    return uint32_t(int64_t(t));
}

// ---- field initialisers ----
// 0x618850 / 0x618930 / 0x618FD0 / 0x619010 / 0x6192F0 (thiscall)
void __fastcall SoundSpeechPriorityTrack_InitFields(uint8_t* self, re::Edx) {
    At<int32_t>(self, 0x18) = -1;
    self[8] = 0;
}
void __fastcall SoundAmbienceTrack_InitFields(uint8_t* self, re::Edx) {
    self[8] = 0;
    At<uint32_t>(self, 0x18) = 0;
}
void __fastcall SpeechStartTrack_InitFields(uint8_t* self, re::Edx) {
    self[0x18] = 0;
}
void __fastcall BikeSoundStateTrack_InitFields(uint8_t* self, re::Edx) {
    At<int32_t>(self, 0x18) = 1;
    self[8] = 0;
}
void __fastcall SoundFadeWithCameraTrack_InitFields(uint8_t* self, re::Edx) {
    self[0x18] = 1;
    self[0x19] = 0;
    self[0x1A] = 0;
}
// 0x618990 (thiscall): SoundPreloadStreamTrack's fields
void __fastcall SoundPreloadStreamTrack_InitFields(uint8_t* self, re::Edx) {
    ClearName(self, 0x20);
    At<float>(self, 0x18) = 0.0f;
    self[0x1C] = 0;
    self[8] = 0;
    self[0x24] = 0;
    At<int32_t>(self, 0x28) = 1;
}
// 0x6189D0 (thiscall): SoundStreamTrack's fields
void __fastcall SoundStreamTrack_InitFields(uint8_t* self, re::Edx) {
    ClearName(self, 0x28);
    At<float>(self, 0x1C) = 0.0f;
    At<int32_t>(self, 0x18) = 0;
    self[0x20] = 0;
    self[8] = 0;
    At<int32_t>(self, 0x24) = 6;
}
// 0x618B20 (thiscall): SoundTimedBankLoadTrack's fields
void __fastcall SoundTimedBankLoadTrack_InitFields(uint8_t* self, re::Edx) {
    ClearName(self, 0x18);
    At<int32_t>(self, 0x1C) = 0;
}
// 0x618BE0 (thiscall): a sound track's fields
void __fastcall SoundTrack_InitFields_618BE0(uint8_t* self, re::Edx) {
    ClearName(self, 0x1C);
    At<float>(self, 0x18) = 1.0f;
    At<int32_t>(self, 0x20) = 1;
    self[8] = 0;
    self[0x24] = 0;
    self[0x25] = 1;
}
// 0x618C50 / 0x618CA0 (thiscall): two sound tracks' fields
void __fastcall SoundTrack_InitFields_618C50(uint8_t* self, re::Edx) {
    ClearName(self, 0x1C);
    At<float>(self, 0x18) = 1.0f;
    At<int32_t>(self, 0x20) = 1;
    At<float>(self, 0x2C) = 0.0f;
    self[8] = 0;
    self[0x24] = 0;
    self[0x25] = 1;
    At<int32_t>(self, 0x30) = 0;
}
void __fastcall SoundTrack_InitFields_618CA0(uint8_t* self, re::Edx) {
    ClearName(self, 0x1C);
    At<float>(self, 0x18) = 1.0f;
    At<int32_t>(self, 0x20) = 1;
    At<float>(self, 0x2C) = 0.0f;
    self[8] = 0;
    self[0x24] = 0;
    self[0x25] = 1;
}
// 0x618D30 (thiscall): SoundLoopTrack's fields
void __fastcall SoundLoopTrack_InitFields(uint8_t* self, re::Edx) {
    ClearName(self, 0x28);
    At<float>(self, 0x18) = 1.0f;
    At<int32_t>(self, 0x1C) = 0;
    self[8] = 0;
    self[0x25] = 0;
    self[0x24] = 0;
    At<int32_t>(self, 0x20) = 1;
    self[0x26] = 1;
}
// 0x618DE0 (thiscall): SoundParametricTrack's fields
void __fastcall SoundParametricTrack_InitFields(uint8_t* self, re::Edx) {
    ClearName(self, 0x1C);
    At<float>(self, 0x18) = 1.0f;
    self[8] = 0;
    At<float>(self, 0x2C) = 0.0f;
    At<float>(self, 0x30) = 1.0f;
}
// 0x618E90 (thiscall): EmitterEnableTrack's fields
void __fastcall EmitterEnableTrack_InitFields(uint8_t* self, re::Edx) {
    ClearName(self, 0x18);
    self[0x1C] = 0;
}
// 0x618F00 (thiscall): SpeechTrack's fields
void __fastcall SpeechTrack_InitFields(uint8_t* self, re::Edx) {
    ClearName(self, 0x44);
    At<float>(self, 0x2C) = 1.5f;
    At<int32_t>(self, 0x18) = 0;
    self[8] = 0;
    At<float>(self, 0x34) = 666.666015625f;
    self[0x1C] = 0;
    At<float>(self, 0x38) = 666.666015625f;
    At<int32_t>(self, 0x28) = 0;
    self[0x30] = 0;
    At<float>(self, 0x3C) = 666.666015625f;
    self[0x40] = 0;
    At<int32_t>(self, 0x24) = 4;
    At<int32_t>(self, 0x20) = -1;
}
// 0x619190 / 0x619240 (thiscall): the social speech event tracks' fields (+0x2C their
// answer: 0x19 the AI's, 0x25 the player's)
template <int32_t Answer>
void __fastcall SocialSpeechTrack_InitFields(uint8_t* self, re::Edx) {
    ClearName(self, 0x30);
    At<int32_t>(self, 0x18) = 0;
    self[8] = 0;
    self[0x1C] = 0;
    At<int32_t>(self, 0x20) = -1;
    ClearName(self, 0x24);
    At<int32_t>(self, 0x28) = 0;
    self[0x34] = 0;
    At<int32_t>(self, 0x2C) = Answer;
}

// ---- Start / Update ----
// 0x6187B0 (thiscall): SoundClearSpeechFilterTrack::Start
uint8_t __fastcall SoundClearSpeechFilterTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    Speech_ResetFilter(speech);
    return 1;
}
// 0x618830 (thiscall): SoundSetSpeechPriorityFilterTrack::Start (its call does nothing)
uint8_t __fastcall SoundSetSpeechPriorityFilterTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    Speech_Nop8A20A0(speech, self[0x18]);
    return 1;
}
// 0x6188C0 / 0x618910 (thiscall): SoundAmbienceOn / OffTrack::Start: the ambience's
// muted flag
uint8_t __fastcall SoundAmbienceOnTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    ambience[0] = 0;
    return 1;
}
uint8_t __fastcall SoundAmbienceOffTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    ambience[0] = 1;
    return 1;
}
// 0x618970 (thiscall): PlayPreloadStreamTrack::Start
uint8_t __fastcall PlayPreloadStreamTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    MusicStream_PlayPreloaded(musicStream, At<uint32_t>(self, 0x18));
    return 1;
}
// 0x618A50 (thiscall): SoundStreamTrack::Start: the stream +0x28 played (+0x18 0; volume
// +0x1C, +0x20, priority +0x24) or stopped (1); priority 2: its emitter's queue left
uint8_t __fastcall SoundStreamTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    float volume = At<float>(self, 0x1C);
    uint8_t flag = self[0x20];
    At<uint8_t*>(self, 0x2C) = owner;
    const char* name = At<const char*>(self, 0x28);
    if (!name) {
        return 1;
    }
    int32_t priority = At<int32_t>(self, 0x24);
    if (priority == 2) {
        StreamEmitter_Unqueue(streamEmitters, HashStreamName(name, 1));
        return 1;
    }
    int32_t kind = At<int32_t>(self, 0x18);
    if (kind == 0) {
        // (the flag's upper bytes: the owner argument's)
        MusicStream_PlayByName(musicStream, name, volume, flag, priority);
    } else if (kind == 1) {
        MusicStream_StopAny(musicStream, 0);
    }
    return 1;
}
// 0x618B00 (thiscall): SoundStreamFadeoutTrack::Start
uint8_t __fastcall SoundStreamFadeoutTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    MusicStream_Stop(musicStream, 0, -1);
    return 1;
}
// 0x618B80 (thiscall): SoundTimedBankLoadTrack::Start: the bank +0x18 requested for
// +0x1C
uint8_t __fastcall SoundTimedBankLoadTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    const char* name = At<const char*>(self, 0x18);
    if (name) {
        int bank = Banks_Find(soundBanks, name);
        DebugPrintf(bank != -1 ? 1 : 0, bankWarning, name);
        if (bank != -1) {
            Banks_Request(soundBanks, bank, At<int32_t>(self, 0x1C), 0);
        }
    }
    return 1;
}
// 0x618D10 (thiscall): DialogSoundFXTrack::Start: waiting for the dialog (+0x30)
uint8_t __fastcall DialogSoundFXTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    self[0x30] = 1;
    At<uint32_t>(self, 0x34) = 0;
    At<uint8_t*>(self, 0x28) = owner;
    return 1;
}
// 0x618E20 (thiscall): SoundParametricTrack::Start
uint8_t __fastcall SoundParametricTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<float>(self, 0x34) = -1.0f;
    At<uint8_t*>(self, 0x28) = owner;
    At<float>(self, 0x38) = parametricSoundDelay;
    return 1;
}
// 0x618E40 (thiscall): SoundTrack2D::Start: the sound +0x18 played without a position
uint8_t __fastcall SoundTrack2D_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    const char* name = At<const char*>(self, 0x18);
    if (name) {
        Sounds_PlayAt(sounds, SoundId(name), nullptr, 1, nullptr, 0, 0x78, 0, 0, nullptr, nullptr, 6, 1, 1.0f, 0);
    }
    return 1;
}
// 0x618EC0 (thiscall): EmitterEnableTrack::Start: the ambience emitter +0x18 on / off
// (+0x1C)
uint8_t __fastcall EmitterEnableTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    const char* name = At<const char*>(self, 0x18);
    if (name) {
        // (the flag's upper bytes: the owner argument's)
        AmbienceEmitters_Enable(ambienceEmitters, HashName(name), self[0x1C]);
    }
    return 1;
}
// 0x619080 / 0x6190D0 / 0x619120 / 0x619170 (thiscall): the boxing crowd's cheer, boo,
// excitement up and down
template <uintptr_t Fn>
uint8_t __fastcall CrowdTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    reinterpret_cast<void(__thiscall*)(uint8_t*)>(Fn)(boxingCrowd);
    return 1;
}
// 0x619300 (thiscall): SoundFadeWithCameraTrack::Start: the categories that follow the
// camera's fade (+0x19: 2..4, +0x1A: 1) set to +0x18
uint8_t __fastcall SoundFadeWithCameraTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    if (!owner) {
        return 0;
    }
    if (self[0x19] != 0) {
        Sfx_SetCategoryFollowsFade(4, self[0x18]);
        Sfx_SetCategoryFollowsFade(3, self[0x18]);
        Sfx_SetCategoryFollowsFade(2, self[0x18]);
    }
    if (self[0x1A] != 0) {
        Sfx_SetCategoryFollowsFade(1, self[0x18]);
    }
    return 1;
}
// 0x619360 (thiscall): MusicAllowPlayDuringCutscenesTrack::Start
uint8_t __fastcall MusicAllowPlayDuringCutscenesTrack_Start(uint8_t* self, re::Edx, uint8_t*) {
    musicDuringCutscenes = self[0x18];
    return 1;
}
// 0x619370 (thiscall): SoundPreloadStreamTrack::Start: the stream +0x20 preloaded
// (volume +0x18, looping +0x1C, priority +0x28; +0x24 the other preload); priority 2:
// queued on its use point's emitter
uint8_t __fastcall SoundPreloadStreamTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    float volume = At<float>(self, 0x18);
    uint8_t loop = self[0x1C];
    const char* name = At<const char*>(self, 0x20);
    At<uint8_t*>(self, 0x2C) = owner;
    if (!name) {
        return 1;
    }
    int32_t priority = At<int32_t>(self, 0x28);
    if (priority != 2) {
        if (self[0x24] != 0) {
            SoundPreloadStream_6C3B10(name, volume, 0);
            return 1;
        }
        MusicStream_PreloadByName(musicStream, name, volume, loop != 0 ? 1 : 0, priority, 200, 2000);
        return 1;
    }
    uint8_t* entity = UsePoint_GetEntity(owner);
    if (!entity) {
        return 1;
    }
    // (the flags' other bits: whatever the stack held)
    QueuedStream entry;
    entry.pos = {0.0f, 0.0f, 0.0f};
    entry.volume = volume;
    entry.flags = uint8_t(((loop & 1) << 2) | (entry.flags & 0xF9));
    const float* pos = Pos(entity);
    entry.pos = {pos[0], pos[1], pos[2]};
    entry.hash = HashStreamName(name, 1);
    entry.flags &= 0xF7;
    entry.flags2 = (entry.flags2 & 0xFFFFFFF1u) | 1;
    StreamEmitter_Queue(streamEmitters, &entry);
    return 1;
}
// 0x6194A0 (thiscall): SoundRecycleTrack::Start: the blend to watch (+0x34: the owner's
// +4, else its ped's or entity's)
uint8_t __fastcall SoundRecycleTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x28) = owner;
    At<uint8_t*>(self, 0x34) = At<uint8_t*>(owner, 4);
    if (!At<uint8_t*>(owner, 4)) {
        if (At<uint8_t*>(owner, 8)) {
            At<uint8_t*>(self, 0x34) = At<uint8_t*>(At<uint8_t*>(owner, 8), 0x114);
            return 1;
        }
        if (At<uint8_t*>(owner, 0xC)) {
            At<uint8_t*>(self, 0x34) = At<uint8_t*>(At<uint8_t*>(owner, 0xC), 0x114);
        }
    }
    return 1;
}
// 0x6194E0 (thiscall): SoundLoopTrack::Start: the loop +0x28 played (+0x1C 0) or
// stopped (1): on the ped (+0x25), at the use point's entity (+0x24; else at the ped),
// or anywhere; +0x26: owned by the ped; volume +0x20
uint8_t __fastcall SoundLoopTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    int32_t mode = At<int32_t>(self, 0x1C);
    uint32_t volume = At<uint32_t>(self, 0x20);
    uint8_t* by = nullptr;
    const char* name = At<const char*>(self, 0x28);
    At<uint8_t*>(self, 0x2C) = owner;
    if (self[0x26] != 0) {
        by = OwnerPed(owner);
    }
    if (self[0x25] != 0) {
        if (mode == 0) {
            uint8_t* ped = OwnerPed(owner);
            LoopSounds_PlayOn(loopSounds, ped, SoundId(name), volume, by);
        } else if (mode == 1) {
            uint8_t* ped = OwnerPed(owner);
            LoopSounds_Stop(loopSounds, ped, nullptr, SoundId(name));
        }
        return 1;
    }
    if (self[0x24] == 0) {
        if (mode == 0) {
            LoopSounds_PlayAt(loopSounds, SoundId(name), nullptr, 2, by);
        } else if (mode == 1) {
            LoopSounds_Stop(loopSounds, nullptr, nullptr, SoundId(name));
        }
        return 1;
    }
    uint8_t* entity = UsePoint_GetEntity(owner);
    if (entity) {
        if (mode == 0) {
            LoopSounds_PlayAtEntity(loopSounds, entity, SoundId(name), volume, by);
        } else if (mode == 1) {
            LoopSounds_Stop(loopSounds, nullptr, entity, SoundId(name));
        }
        return 1;
    }
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x2C));
    if (!ped) {
        return 1;
    }
    if (mode == 0) {
        LoopSounds_PlayAt(loopSounds, SoundId(name), Pos(ped), volume, by);
    } else if (mode == 1) {
        LoopSounds_Stop(loopSounds, nullptr, nullptr, SoundId(name));
    }
    return 1;
}
// 0x619680 (thiscall): CleanupDeadSoundLoopsTrack::Start: a dead ped's loops stopped
uint8_t __fastcall CleanupDeadSoundLoopsTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (ped && At<int32_t>(ped, 0x143C) != 0) {
        LoopSounds_StopForPed(loopSounds, ped, 0);
    }
    return 1;
}

// 0x6196B0 (cdecl): a speech's callback for its slot: once said, the slot's follow-up
// is due (kind 2: the ped stopped now); 0 when it acted
uint8_t __cdecl SpeechSlot_Callback(int said, SpeechSlot* slot) {
    if (!said || !slot) {
        return 1;
    }
    if (slot->active == 0) {
        slot->active = 0;
        return 1;
    }
    if (slot->started != 0) {
        slot->active = 0;
        return 1;
    }
    if (slot->until >= timeInMs) {
        return 0;
    }
    slot->active = 0;
    if (slot->kind == 1) {
        return 1;
    }
    if (slot->kind != 2) {
        return 1;
    }
    Speech_StopPed(speech, slot->ped, 3, 0x19);
    return 0;
}

// 0x619700 (thiscall): SpeechTrack::Start: the speech +0x18 said by the ped (priority
// +0x24; 10 in dodgeball, area 0x2D or mission 3_04 on a bike); scripted with variant
// +0x20 when +0x1C; without a ped (+0x30): by a toilet / urinal / sink prop's position or
// +0x34; +0x28: a follow-up slot (+0x2C seconds, or 12 s at the player's target)
uint8_t __fastcall SpeechTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    V3 position = {0.0f, 0.0f, 0.0f};
    At<uint8_t*>(self, 0x48) = owner;
    uint8_t* ped = OwnerPed(owner);
    int32_t type = At<int32_t>(self, 0x18);
    int32_t variant = -1;
    uint8_t* speaker = nullptr;
    if (ped) {
        speaker = (ped[0x108] & 7) == 3 ? ped : nullptr;
    }
    if (self[0x40] == 0 && !Speech_PassesPedFlag(speech, ped, type)) {
        return 0;
    }
    int32_t priority = At<int32_t>(self, 0x24);
    uint8_t urgent;
    if (type == 0x6D || Dodgeball_IsActive() || currArea == 0x2D) {
        urgent = 1;
    } else {
        // (the key lives in the owner argument's slot)
        uint32_t key;
        NameKey_ConstructFrom(&key, missionKey);
        int32_t id = Missions_Find(missionManager, &key);
        uint8_t* controller = Missions_GetController1(missionManager);
        urgent = MissionSlot_Is(controller, id) && At<int32_t>(speaker, 0x1554) != 0 ? 1 : 0;
        NameKey_Destruct(&key);
    }
    if (urgent) {
        priority = 10;
    }
    if (!speaker || !At<uint8_t*>(self, 0x48)) {
        if (self[0x30] == 0) {
            return 0;
        }
        uint8_t* prop = At<uint8_t*>(At<uint8_t*>(self, 0x48), 0xC);
        if (prop) {
            int16_t model = At<int16_t>(prop, 0x10E);
            if (model == 0x27EF) {
                variant = 1;
            } else if (model == 0x27EE) {
                variant = 2;
            } else if (model == 0x27EC) {
                variant = 3;
            } else {
                return 0;
            }
            const float* p = Pos(prop);
            position = {p[0], p[1], p[2]};
        } else {
            position = {At<float>(self, 0x34), At<float>(self, 0x38), At<float>(self, 0x3C)};
        }
    }
    uint8_t scripted = self[0x1C];
    SpeechSlot* slot = nullptr;
    const void* callback = nullptr;
    if (At<int32_t>(self, 0x28) != 0) {
        for (int i = 0; i < kSpeechSlots; i++) {
            SpeechSlot& s = speechSlots[i];
            if (s.active != 0 && s.ped == speaker) {
                s.active = 0;
                if (s.ped) {
                    Speech_StopPed(speech, s.ped, 3, 0x19);
                }
            }
        }
        for (int i = 0; i < kSpeechSlots; i++) {
            if (speechSlots[i].active == 0) {
                slot = &speechSlots[i];
                slot->active = 1;
                slot->started = 0;
                slot->kind = At<int32_t>(self, 0x28);
                uint32_t until;
                uint8_t* target = speaker ? Ped_GetTarget(speaker) : nullptr;
                if (target && (target[0x108] & 7) != 3) {
                    target = nullptr;
                }
                if (speaker && target == playerPed) {
                    until = timeInMs + 12000;
                } else {
                    until = TimeAfter(At<float>(self, 0x2C));
                }
                slot->until = until;
                EntityRef_Set(&slot->ped, speaker);
                callback = reinterpret_cast<const void*>(0x006196B0);
                break;
            }
        }
    }
    alignas(4) uint8_t event[0x30];
    if (scripted == 0) {
        if (self[0x30] != 0) {
            Speech_Add(speech, event, speaker, nullptr, 0, type, callback, slot, 0, 1, variant, &position);
        } else {
            Speech_AddScripted(speech, event, type, 0, speaker, uint32_t(priority), callback, slot, 0, 0, 0);
        }
    } else {
        Speech_AddScripted(speech, event, type, At<uint32_t>(self, 0x20), speaker, uint32_t(priority), callback, slot,
                           0, 0, 0);
    }
    if (lastScriptedSpeech != 0 && slot) {
        slot->active = 0;
    }
    return 1;
}

// 0x619A50 (cdecl): the speech slots whose time is up: their ped forgotten (1) or
// stopped (2)
void __cdecl SpeechSlots_Update() {
    for (int i = 0; i < kSpeechSlots; i++) {
        SpeechSlot& s = speechSlots[i];
        if (s.active != 0 && s.until < timeInMs) {
            uint8_t* ped = s.ped;
            s.active = 0;
            if (ped) {
                if (s.kind == 1) {
                    Speech_ForgetPed(speech, ped);
                } else if (s.kind == 2) {
                    Speech_StopPed(speech, ped, 3, 0x19);
                }
            }
        }
    }
}

// 0x619AB0 (thiscall): SpeechStartTrack::Start: the ped's pending slots started (kind 1)
uint8_t __fastcall SpeechStartTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    if (!OwnerPed(owner)) {
        return 1;
    }
    for (int i = 0; i < kSpeechSlots; i++) {
        SpeechSlot& s = speechSlots[i];
        if (s.active != 0 && s.ped == OwnerPed(At<uint8_t*>(self, 0x18))) {
            s.started = 1;
            s.kind = 1;
        }
    }
    return 1;
}
// 0x619B00 (thiscall): RemoveAllQueuedSpeechTrack::Start: the ped's speech stopped (with
// +0x18) or its model's queued lines removed
uint8_t __fastcall RemoveAllQueuedSpeechTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (ped) {
        if (self[0x18] != 0) {
            Speech_StopPed(speech, ped, 0, 0x19);
            return 1;
        }
        SpeechLib_RemoveModel(speechLib, At<int16_t>(ped, 0x10E), 0, -1);
    }
    return 1;
}
// 0x619B50 (thiscall): BikeSoundStateTrack::Start: the ped's bike's sound state (+0x714)
uint8_t __fastcall BikeSoundStateTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* bike = At<uint8_t*>(OwnerPed(owner), 0x1554);
    if (bike) {
        At<int32_t>(bike, 0x714) = At<int32_t>(self, 0x18);
    }
    return 1;
}
// 0x619B80 / 0x619CD0 (thiscall): the AI's / player's social speech event: the answer
// played (+0x34 then: nothing said), else the speech +0x18 (scripted with +0x1C)
template <uintptr_t Answer>
uint8_t __fastcall SocialSpeechEventTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x38) = owner;
    if (!owner) {
        return 0;
    }
    uint8_t* ped = OwnerPed(owner);
    if (!ped || (ped[0x108] & 7) != 3) {
        return 0;
    }
    if (reinterpret_cast<uint8_t(__thiscall*)(uint8_t*, int, int)>(Answer)(ped + 0xB44, 1, At<int32_t>(self, 0x2C))) {
        self[0x34] = 1;
        return 0;
    }
    if (At<const char*>(self, 0x24)) {
        Text_Get(texts, At<const char*>(self, 0x24));
    }
    int32_t type = At<int32_t>(self, 0x18);
    if (!Speech_PassesPedFlag(speech, OwnerPed(At<uint8_t*>(self, 0x38)), type)) {
        return 0;
    }
    alignas(4) uint8_t event[0x30];
    if (self[0x1C] == 0) {
        Speech_Add(speech, event, ped, nullptr, 0, type, nullptr, nullptr, 0, 0, -1, nullptr);
        return 1;
    }
    Speech_AddScripted(speech, event, type, At<uint32_t>(self, 0x20), ped, 4, nullptr, nullptr, 0, 0, 0);
    return 1;
}
// 0x619C70 (thiscall): AISocialSpeechEventTrack::Update: its subtitle (+0x24, +0x28) shown
// once a frame while it talks to the player
uint8_t __fastcall AISocialSpeechEventTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x38));
    if (self[0x34] == 0 && socialSubtitles != 0) {
        uint8_t* other = At<uint8_t*>(ped, 0xBB4);
        if (other && At<int32_t>(other, 0x1310) == 0xD) {
            if (aiSubtitleTime != timeInMs && At<uint32_t>(self, 0x24) != 0) {
                aiSubtitleTime = timeInMs;
                HUDText_SetText(hudText, At<uint32_t>(self, 0x28), At<uint32_t>(self, 0x24));
            }
            return 1;
        }
    }
    return 0;
}
// 0x619DC0 (thiscall): PlayerSocialSpeechEventTrack::Update
uint8_t __fastcall PlayerSocialSpeechEventTrack_Update(uint8_t* self, re::Edx, float) {
    if (self[0x34] != 0 || socialSubtitles == 0) {
        return 0;
    }
    if (playerSubtitleTime != timeInMs && At<uint32_t>(self, 0x24) != 0) {
        playerSubtitleTime = timeInMs;
        HUDText_SetText(hudText, At<uint32_t>(self, 0x28), At<uint32_t>(self, 0x24));
    }
    return 1;
}

// the shop bell (a shop's door opened by the ped)
void PlayShopBell(const V3* pos, uint32_t volume) {
    Sounds_PlayAt(sounds, SoundId(shopBell), pos, 2, nullptr, 0, int(volume), 0, 0, nullptr, nullptr, 6, 0, 1.0f, 0);
}

// 0x619E10 (thiscall): SoundTrack's sound (+0x1C, volume +0x18 of 127, +0x20, +0x25):
// at the ped (half a metre up) or the entity (a shop door: the bell too), else without
// a position; footsteps (+0x24) through the ped's footsteps; a bike crash in water: a
// splash
void __fastcall SoundTrack_PlayImpl(uint8_t* self, re::Edx) {
    const char* name = At<const char*>(self, 0x1C);
    if (!name) {
        return;
    }
    uint32_t flags = At<uint32_t>(self, 0x20);
    uint8_t volume = uint8_t(int32_t(At<float>(self, 0x18) * 127.0f));
    if (volume > 0x7F) {
        volume = 0x7F;
    }
    int32_t id = SoundId(name);
    V3 pos = {0.0f, 0.0f, 0.0f};
    uint8_t* owner = At<uint8_t*>(self, 0x28);
    uint8_t placed = 1;
    if (At<uint8_t*>(owner, 8)) {
        const float* p = Pos(At<uint8_t*>(owner, 8));
        pos = {p[0], p[1], p[2]};
        pos.z = pos.z + 0.5f;
    } else if (At<uint8_t*>(owner, 0xC)) {
        uint8_t* entity = At<uint8_t*>(owner, 0xC);
        const float* p = Pos(entity);
        pos = {p[0], p[1], p[2]};
        if (ModelInfo_IsDoor(modelInfoPtrs[At<int16_t>(At<uint8_t*>(owner, 0xC), 0x10E)])) {
            uint8_t* door = At<uint8_t*>(At<uint8_t*>(self, 0x28), 0xC);
            PropAnim_SetCurrent(door);
            if (Controller_IsPlaying(door + 0x128, shopDoorNode, 1, 1)) {
                uint8_t area = Areas_GetTriggerRouteArea(areas, At<uint32_t>(At<uint8_t*>(At<uint8_t*>(self, 0x28), 0xC),
                                                                              0x23C));
                if (area != 0xFF && Areas_IsShop(areas, int8_t(area))) {
                    PlayShopBell(&pos, volume);
                } else if (Areas_IsShop(areas, int8_t(currArea))) {
                    PlayShopBell(&pos, volume);
                }
            }
        }
    } else {
        placed = 0;
    }
    if (self[0x24] != 0) {
        if (Sounds_IsFootstep(sounds, id) && At<uint8_t*>(At<uint8_t*>(self, 0x28), 8)) {
            Footsteps_Play(sounds, At<uint8_t*>(At<uint8_t*>(self, 0x28), 8), id, flags, 0.0f, 4);
            return;
        }
        uint8_t* ped = At<uint8_t*>(At<uint8_t*>(self, 0x28), 8);
        if (ped && At<uint8_t*>(ped, 0x1554) && At<uint8_t*>(ped, 0x1554)[0x1BF] == 1 && id == SoundId(bikeCrash)) {
            id = SoundId(diveSplash);
        }
    }
    // (the volume's upper bytes: stale stack in the game)
    Sounds_PlayAt(sounds, id, placed ? &pos : nullptr, int(flags), nullptr, 0, volume, 0, 0, nullptr, nullptr, 6,
                  self[0x25], 1.0f, 0);
}

// 0x61A0F0 (thiscall): SoundRecycleTrack::Update: the sound played again each time the
// watched animation (+0x30 in +0x34) passes +0x2C (wrapping round too)
uint8_t __fastcall SoundRecycleTrack_Update(uint8_t* self, re::Edx, float) {
    if (!At<uint8_t*>(self, 0x34)) {
        return 0;
    }
    uint8_t* anim = Blend_GetAnim(At<uint8_t*>(self, 0x34), At<int32_t>(self, 0x30));
    if (!anim) {
        return 0;
    }
    uint8_t* info = At<uint8_t*>(anim, 0x18);
    if (!info) {
        return 1;
    }
    float length = At<float>(info, 0xC);
    if (!(length >= 0.0f)) {
        return 1;
    }
    float now = At<float>(anim, 0x20);
    float before = At<float>(anim, 0x1C);
    float at = At<float>(self, 0x2C);
    if (at >= now && at < before) {
        SoundTrack_Play(self);
        return 1;
    }
    if (!(now - before > 0.2f)) {
        return 1;
    }
    length = At<float>(At<uint8_t*>(anim, 0x18), 0xC);
    if (at >= 0.0f && at <= before) {
        SoundTrack_Play(self);
        return 1;
    }
    if (!(at >= now)) {
        return 1;
    }
    if (!(length >= at)) {
        return 1;
    }
    SoundTrack_Play(self);
    return 1;
}
// 0x61A1F0 (thiscall): DialogSoundFXTrack::Update: once the dialog's lines are done, the
// sound after +0x2C seconds (+0x34 when); ends then
uint8_t __fastcall DialogSoundFXTrack_Update(uint8_t* self, re::Edx, float) {
    if (self[0x30] != 0) {
        if (!dialogState || dialogState[0x59] != 0) {
            self[0x30] = 0;
            At<uint32_t>(self, 0x34) = TimeAfter(At<float>(self, 0x2C));
        }
        return 1;
    }
    if (timeInMs < At<uint32_t>(self, 0x34)) {
        return 1;
    }
    SoundTrack_Play(self);
    return 0;
}
// 0x61A260 (thiscall): SoundParametricTrack::Update: played when the owner's parameter
// rises past +0x2C / +0x30 (+0x34 the last), at most every 0xB0BCC4 seconds (+0x38)
uint8_t __fastcall SoundParametricTrack_Update(uint8_t* self, re::Edx, float dt) {
    float waited = dt + At<float>(self, 0x38);
    At<float>(self, 0x38) = waited;
    float value = *At<float*>(At<uint8_t*>(self, 0x28), 0x38);
    float mark = At<float>(self, 0x2C) / At<float>(self, 0x30);
    if (At<float>(self, 0x34) < mark && value >= mark && parametricSoundDelay <= waited) {
        At<float>(self, 0x38) = 0.0f;
        SoundTrack_Play(self);
        At<float>(self, 0x34) = value;
        return 1;
    }
    At<float>(self, 0x34) = value;
    return 1;
}
// 0x61A2F0 (thiscall): SoundTrack::Start: played once
uint8_t __fastcall SoundTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x28) = owner;
    SoundTrack_Play(self);
    return 1;
}

} // namespace

#define TRSOUND_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

TRSOUND_REVERSED("SoundClearSpeechFilterTrack::DeletingDestruct", 0x00618780,
                 &TrackDeletingDtor<kVtSoundClearSpeechFilterTrack>);
TRSOUND_REVERSED("SoundClearSpeechFilterTrack::Start", 0x006187B0, &SoundClearSpeechFilterTrack_Start);
TRSOUND_REVERSED("SoundSetSpeechPriorityFilterTrack::DeletingDestruct", 0x006187D0,
                 &TrackDeletingDtor<kVtSoundSetSpeechPriorityFilterTrack>);
TRSOUND_REVERSED("SoundAddSpeechPriorityFilterTrack::DeletingDestruct", 0x00618800,
                 &TrackDeletingDtor<kVtSoundAddSpeechPriorityFilterTrack>);
TRSOUND_REVERSED("SoundSetSpeechPriorityFilterTrack::Start", 0x00618830, &SoundSetSpeechPriorityFilterTrack_Start);
TRSOUND_REVERSED("SoundSpeechPriorityTrack::InitFields_618850", 0x00618850, &SoundSpeechPriorityTrack_InitFields);
TRSOUND_REVERSED("SoundRemoveSpeechPriorityFilterTrack::DeletingDestruct", 0x00618860,
                 &TrackDeletingDtor<kVtSoundRemoveSpeechPriorityFilterTrack>);
TRSOUND_REVERSED("SoundAmbienceOnTrack::DeletingDestruct", 0x00618890, &TrackDeletingDtor<kVtSoundAmbienceOnTrack>);
TRSOUND_REVERSED("SoundAmbienceOnTrack::Start", 0x006188C0, &SoundAmbienceOnTrack_Start);
TRSOUND_REVERSED("SoundAmbienceOffTrack::DeletingDestruct", 0x006188E0, &TrackDeletingDtor<kVtSoundAmbienceOffTrack>);
TRSOUND_REVERSED("SoundAmbienceOffTrack::Start", 0x00618910, &SoundAmbienceOffTrack_Start);
TRSOUND_REVERSED("SoundAmbienceTrack::InitFields_618930", 0x00618930, &SoundAmbienceTrack_InitFields);
TRSOUND_REVERSED("PlayPreloadStreamTrack::DeletingDestruct", 0x00618940, &TrackDeletingDtor<kVtPlayPreloadStreamTrack>);
TRSOUND_REVERSED("PlayPreloadStreamTrack::Start", 0x00618970, &PlayPreloadStreamTrack_Start);
TRSOUND_REVERSED("SoundPreloadStreamTrack::InitFields", 0x00618990, &SoundPreloadStreamTrack_InitFields);
TRSOUND_REVERSED("SoundStreamTrack::InitFields", 0x006189D0, &SoundStreamTrack_InitFields);
TRSOUND_REVERSED("SoundStreamTrack::DeletingDestruct", 0x00618A20, &TrackDeletingDtor<kVtSoundStreamTrack, 0x28>);
TRSOUND_REVERSED("SoundStreamTrack::Start", 0x00618A50, &SoundStreamTrack_Start);
TRSOUND_REVERSED("SoundStreamFadeoutTrack::DeletingDestruct", 0x00618AD0,
                 &TrackDeletingDtor<kVtSoundStreamFadeoutTrack>);
TRSOUND_REVERSED("SoundStreamFadeoutTrack::Start", 0x00618B00, &SoundStreamFadeoutTrack_Start);
TRSOUND_REVERSED("SoundTimedBankLoadTrack::InitFields", 0x00618B20, &SoundTimedBankLoadTrack_InitFields);
TRSOUND_REVERSED("SoundTimedBankLoadTrack::DeletingDestruct", 0x00618B50,
                 &TrackDeletingDtor<kVtSoundTimedBankLoadTrack, 0x18>);
TRSOUND_REVERSED("SoundTimedBankLoadTrack::Start", 0x00618B80, &SoundTimedBankLoadTrack_Start);
TRSOUND_REVERSED("SoundTrack::InitFields_618BE0", 0x00618BE0, &SoundTrack_InitFields_618BE0);
TRSOUND_REVERSED("SoundTrack::DeletingDestruct", 0x00618C20, &TrackDeletingDtor<kVtSoundTrack, 0x1C>);
TRSOUND_REVERSED("SoundTrack::InitFields_618C50", 0x00618C50, &SoundTrack_InitFields_618C50);
TRSOUND_REVERSED("SoundTrack::InitFields_618CA0", 0x00618CA0, &SoundTrack_InitFields_618CA0);
TRSOUND_REVERSED("SoundTrack::Destruct", 0x00618CF0, &TrackDtor<kVtSoundTrack, 0x1C>);
TRSOUND_REVERSED("DialogSoundFXTrack::Start", 0x00618D10, &DialogSoundFXTrack_Start);
TRSOUND_REVERSED("SoundLoopTrack::InitFields", 0x00618D30, &SoundLoopTrack_InitFields);
TRSOUND_REVERSED("SoundLoopTrack::DeletingDestruct", 0x00618D80, &TrackDeletingDtor<kVtSoundLoopTrack, 0x28>);
TRSOUND_REVERSED("CleanupDeadSoundLoopsTrack::DeletingDestruct", 0x00618DB0,
                 &TrackDeletingDtor<kVtCleanupDeadSoundLoopsTrack>);
TRSOUND_REVERSED("SoundParametricTrack::InitFields", 0x00618DE0, &SoundParametricTrack_InitFields);
TRSOUND_REVERSED("SoundParametricTrack::Start", 0x00618E20, &SoundParametricTrack_Start);
TRSOUND_REVERSED("SoundTrack2D::Start", 0x00618E40, &SoundTrack2D_Start);
TRSOUND_REVERSED("EmitterEnableTrack::InitFields", 0x00618E90, &EmitterEnableTrack_InitFields);
TRSOUND_REVERSED("EmitterEnableTrack::Start", 0x00618EC0, &EmitterEnableTrack_Start);
TRSOUND_REVERSED("SpeechTrack::InitFields", 0x00618F00, &SpeechTrack_InitFields);
TRSOUND_REVERSED("SpeechTrack::DeletingDestruct", 0x00618F70, &TrackDeletingDtor<kVtSpeechTrack, 0x44>);
TRSOUND_REVERSED("SpeechStartTrack::DeletingDestruct", 0x00618FA0, &TrackDeletingDtor<kVtSpeechStartTrack>);
TRSOUND_REVERSED("SpeechStartTrack::InitFields", 0x00618FD0, &SpeechStartTrack_InitFields);
TRSOUND_REVERSED("RemoveAllQueuedSpeechTrack::DeletingDestruct", 0x00618FE0,
                 &TrackDeletingDtor<kVtRemoveAllQueuedSpeechTrack>);
TRSOUND_REVERSED("BikeSoundStateTrack::InitFields", 0x00619010, &BikeSoundStateTrack_InitFields);
TRSOUND_REVERSED("BikeSoundStateTrack::DeletingDestruct", 0x00619020, &TrackDeletingDtor<kVtBikeSoundStateTrack>);
TRSOUND_REVERSED("CrowdCheerTrack::DeletingDestruct", 0x00619050, &TrackDeletingDtor<kVtCrowdCheerTrack>);
TRSOUND_REVERSED("CrowdCheerTrack::Start", 0x00619080, &CrowdTrack_Start<0x0058D940>);
TRSOUND_REVERSED("CrowdBooTrack::DeletingDestruct", 0x006190A0, &TrackDeletingDtor<kVtCrowdBooTrack>);
TRSOUND_REVERSED("CrowdBooTrack::Start", 0x006190D0, &CrowdTrack_Start<0x0058D900>);
TRSOUND_REVERSED("CrowdExciteTrack::DeletingDestruct", 0x006190F0, &TrackDeletingDtor<kVtCrowdExciteTrack>);
TRSOUND_REVERSED("CrowdExciteTrack::Start", 0x00619120, &CrowdTrack_Start<0x0058DA20>);
TRSOUND_REVERSED("CrowdRelaxTrack::DeletingDestruct", 0x00619140, &TrackDeletingDtor<kVtCrowdRelaxTrack>);
TRSOUND_REVERSED("CrowdRelaxTrack::Start", 0x00619170, &CrowdTrack_Start<0x0058DA50>);
TRSOUND_REVERSED("AISocialSpeechEventTrack::InitFields", 0x00619190, &SocialSpeechTrack_InitFields<0x19>);
TRSOUND_REVERSED("AISocialSpeechEventTrack::DeletingDestruct", 0x00619200,
                 &TrackDeletingDtor<kVtAISocialSpeechEventTrack, 0x30, 0x24>);
TRSOUND_REVERSED("PlayerSocialSpeechEventTrack::InitFields", 0x00619240, &SocialSpeechTrack_InitFields<0x25>);
TRSOUND_REVERSED("PlayerSocialSpeechEventTrack::DeletingDestruct", 0x006192B0,
                 &TrackDeletingDtor<kVtPlayerSocialSpeechEventTrack, 0x30, 0x24>);
TRSOUND_REVERSED("SoundFadeWithCameraTrack::InitFields", 0x006192F0, &SoundFadeWithCameraTrack_InitFields);
TRSOUND_REVERSED("SoundFadeWithCameraTrack::Start", 0x00619300, &SoundFadeWithCameraTrack_Start);
TRSOUND_REVERSED("MusicAllowPlayDuringCutscenesTrack::Start", 0x00619360, &MusicAllowPlayDuringCutscenesTrack_Start);
TRSOUND_REVERSED("SoundPreloadStreamTrack::Start", 0x00619370, &SoundPreloadStreamTrack_Start);
TRSOUND_REVERSED("SoundRecycleTrack::Start", 0x006194A0, &SoundRecycleTrack_Start);
TRSOUND_REVERSED("SoundLoopTrack::Start", 0x006194E0, &SoundLoopTrack_Start);
TRSOUND_REVERSED("CleanupDeadSoundLoopsTrack::Start", 0x00619680, &CleanupDeadSoundLoopsTrack_Start);
TRSOUND_REVERSED("SpeechSlot::Callback_6196B0", 0x006196B0, &SpeechSlot_Callback);
TRSOUND_REVERSED("SpeechTrack::Start", 0x00619700, &SpeechTrack_Start);
TRSOUND_REVERSED("SpeechSlots::Update_619A50", 0x00619A50, &SpeechSlots_Update);
TRSOUND_REVERSED("SpeechStartTrack::Start", 0x00619AB0, &SpeechStartTrack_Start);
TRSOUND_REVERSED("RemoveAllQueuedSpeechTrack::Start", 0x00619B00, &RemoveAllQueuedSpeechTrack_Start);
TRSOUND_REVERSED("BikeSoundStateTrack::Start", 0x00619B50, &BikeSoundStateTrack_Start);
TRSOUND_REVERSED("AISocialSpeechEventTrack::Start", 0x00619B80, &SocialSpeechEventTrack_Start<0x0048F3C0>);
TRSOUND_REVERSED("AISocialSpeechEventTrack::Update", 0x00619C70, &AISocialSpeechEventTrack_Update);
TRSOUND_REVERSED("PlayerSocialSpeechEventTrack::Start", 0x00619CD0, &SocialSpeechEventTrack_Start<0x0048F5D0>);
TRSOUND_REVERSED("PlayerSocialSpeechEventTrack::Update", 0x00619DC0, &PlayerSocialSpeechEventTrack_Update);
TRSOUND_REVERSED("SoundTrack::Play", 0x00619E10, &SoundTrack_PlayImpl);
TRSOUND_REVERSED("SoundRecycleTrack::Update", 0x0061A0F0, &SoundRecycleTrack_Update);
TRSOUND_REVERSED("DialogSoundFXTrack::Update", 0x0061A1F0, &DialogSoundFXTrack_Update);
TRSOUND_REVERSED("SoundParametricTrack::Update", 0x0061A260, &SoundParametricTrack_Update);
TRSOUND_REVERSED("SoundTrack::Start", 0x0061A2F0, &SoundTrack_Start);
