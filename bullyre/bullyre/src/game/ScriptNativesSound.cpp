// Script natives for sound, speech, stats, the system, text and credits
// (0x5D3530..0x5D5DB0). Each is `int __cdecl Native_X(lua_State* L)`, returning how many
// results it pushed; registered under its script name (lua_natives.csv).

#include <cstdint>
#include <cstring>

#include "game/ScriptNatives.h"

#include "game/Crt.h"
#include "reversed.h"
namespace {

// ---- sounds ---------------------------------------------------------------------------------

// The sounds (0xCF3DA0): an id by name, playing one, the frontend loop
inline void* const snsSounds = reinterpret_cast<void*>(0x00CF3DA0);
GAME_FN(int, __thiscall, snsSounds_GetId, 0x00595B80, void* sounds, const char* name);
GAME_FN(void*, __thiscall, snsSounds_PlayAt, 0x0059B8A0, void* sounds, int id, const SnVec* pos, int curve, const SnVec* pos2,
        int a5, int a6, int a7, int a8, void* callback, void* entity, int a11, int a12, float volume, int a14);
GAME_FN(void, __thiscall, snsSounds_PlayFrontend, 0x0059C0A0, void* sounds, int id);
GAME_FN(void, __thiscall, snsSounds_StopFrontend, 0x0059B860, void* sounds);
// 0x59E170: a curve by name
GAME_FN(int, __cdecl, snsCurveFromName, 0x0059E170, const char* name);
// The ambience emitters (0xCF4278)
GAME_VAR(void*, snsEmitters, 0x00CF4278);
GAME_FN(void, __thiscall, snsEmitters_AddMission, 0x00586820, void* emitters, float x, float y, float z, int soundId, int type,
        int condition);
GAME_FN(void, __thiscall, snsEmitters_RemoveMission, 0x00587190, void* emitters, float x, float y, float z, int soundId);
GAME_FN(void, __cdecl, snsCondition_SetAll, 0x00593DE0, uint32_t* flags);
// The ambience (0xCF4374)
GAME_VAR(void*, snsAmbience, 0x00CF4374);
GAME_FN(void, __thiscall, snsAmbience_SetScripted, 0x00593D80, void* ambience, const char* name, float volume, int fadeIn, int fadeOut);
// The loop sounds (0xCF4274)
GAME_VAR(void*, snsLoopSounds, 0x00CF4274);
GAME_FN(void, __thiscall, snsLoopSounds_PlayAt, 0x00593010, void* loops, int id, const SnVec* pos, int curve, uintptr_t);
GAME_FN(void, __thiscall, snsLoopSounds_PlayOn, 0x00592F30, void* loops, void* entity, int id, int curve, uintptr_t);
GAME_FN(void, __thiscall, snsLoopSounds_Stop, 0x00592400, void* loops, void* entity, void* entity2, int id);
GAME_FN(void*, __cdecl, snsGetEntity, 0x0044A660, int handle, int kind);

// x, y, z, sound, type
int __cdecl Native_SoundEmitterStart(lua_State* L) {
    float x = Script::GetFloat(L, 0);
    float y = Script::GetFloat(L, 1);
    float z = Script::GetFloat(L, 2);
    int id = snsSounds_GetId(snsSounds, Script::GetString(L, 3));
    int type = snsCurveFromName(Script::GetString(L, 4));
    uint32_t condition = 0; // (the game only clears the low byte; the rest is uninitialised)
    snsCondition_SetAll(&condition);
    snsEmitters_AddMission(snsEmitters, x, y, z, id, type, int(condition));
    return 0;
}

int __cdecl Native_SoundEmitterStop(lua_State* L) {
    float x = Script::GetFloat(L, 0);
    float y = Script::GetFloat(L, 1);
    float z = Script::GetFloat(L, 2);
    int id = snsSounds_GetId(snsSounds, Script::GetString(L, 3));
    snsEmitters_RemoveMission(snsEmitters, x, y, z, id);
    return 0;
}

int __cdecl Native_SoundSetScriptedAmbience(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    float volume = Script::GetFloat(L, 1);
    int fadeIn = Script::GetInt(L, 2);
    int fadeOut = Script::GetInt(L, 3);
    snsAmbience_SetScripted(snsAmbience, name, volume, fadeIn, fadeOut);
    return 0;
}

// ---- speech -----------------------------------------------------------------------------------

// The speech events' names (0xAE74E8, up to "NUM_SPEECH_EVENTS")
inline const re::Ptr32<const char>* const snsSpeechEventNames = reinterpret_cast<const re::Ptr32<const char>*>(0x00AE74E8);
// The speech manager (0xCF4378) and the last scripted event's result (0xCF4E00; -2: the
// model has no such event)
inline void* const snsSpeech = reinterpret_cast<void*>(0x00CF4378);
GAME_VAR(int32_t, snsLastScriptedSpeech, 0x00CF4E00);
struct SnsSpeechEvent {
    uint8_t unknown0[0x30];
};
GAME_FN(void, __thiscall, snsSpeech_Add, 0x005A4920, void* speech, SnsSpeechEvent* out, uint8_t* ped, uintptr_t, int, int event,
        int, int, int, int, int, uintptr_t);
GAME_FN(SnsSpeechEvent*, __thiscall, snsSpeech_AddScripted, 0x005A4AD0, void* speech, SnsSpeechEvent* out, int event,
        int comment, uint8_t* ped, int curve, int, int, int, int flag4, int flag5);
GAME_FN(void, __thiscall, snsSpeech_StopPed, 0x005A59D0, void* speech, uint8_t* ped, int, int);
GAME_FN(void, __cdecl, snsSpeech_StopAll, 0x005A5DB0);
GAME_FN(uint8_t, __cdecl, snsIsSpeechBusy, 0x005A2900, uint8_t* ped, int event, int comment);
// The speech library (0xCF4E10)
inline void* const snsSpeechLib = reinterpret_cast<void*>(0x00CF4E10);
GAME_FN(void, __thiscall, snsSpeechLib_RemoveModel, 0x0059F8D0, void* lib, int model, int, int);
GAME_FN(void, __cdecl, snsDebugPrint, 0x0049A650, int, const char* format, ...);   // (empty)

// A speech event's index by name, -1 for none
int snsFindSpeechEvent(const char* name) {
    if (snStricmp("NUM_SPEECH_EVENTS", snsSpeechEventNames[0]) == 0) {
        return -1;
    }
    for (int i = 0;; i++) {
        if (snStricmp(name, snsSpeechEventNames[i]) == 0) {
            return i;
        }
        if (snStricmp("NUM_SPEECH_EVENTS", snsSpeechEventNames[i + 1]) == 0) {
            return -1;
        }
    }
}

// ped, event (by name; nothing for an unknown one)
int __cdecl Native_SoundPlayAmbientSpeechEvent(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    if (ped != nullptr) {
        int event = snsFindSpeechEvent(Script::GetString(L, 1));
        if (event != -1) {
            SnsSpeechEvent result;
            snsSpeech_Add(snsSpeech, &result, ped, 0, 0, event, 0, 0, 0, 0, -1, 0);
        }
    }
    return 0;
}

// ped, event, comment[, curve (4), flag, flag]: the result (nothing for an unknown event,
// though the function says 1)
int __cdecl Native_SoundPlayScriptedSpeechEvent(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    const char* name = Script::GetString(L, 1);
    int comment = Script::GetInt(L, 2);
    uint8_t flag4 = Script::GetBoolOpt(L, 4, 0);
    uint8_t flag5 = Script::GetBoolOpt(L, 5, 0);
    int curve = 4;
    if (lua::gettop(L) >= 4) {
        curve = snsCurveFromName(Script::GetString(L, 3));
    }
    int event = snsFindSpeechEvent(name);
    if (event == -1) {
        return 1;
    }
    SnsSpeechEvent result;
    // (the flags go as dwords with uninitialised upper bytes)
    snsSpeech_AddScripted(snsSpeech, &result, event, comment, ped, curve, 0, 0, 0, flag4, flag5);
    if (snsLastScriptedSpeech == -2) {
        snsDebugPrint(0,
                      "\n*************** ATTENTION SCRIPTER ****************\n\nYou tried to play %s (COMMENT#: %d) on ped model "
                      "id %d- this model does not have this speech event!\n\n======================================================\n\n",
                      name, comment, int(snAt<int16_t>(ped, 0x10E)));
    }
    Script::PushInt(L, snsLastScriptedSpeech);
    return 1;
}

// event, comment: with no ped
int __cdecl Native_SoundPlayScriptedSpeechEvent_2D(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    int comment = Script::GetInt(L, 1);
    int event = snsFindSpeechEvent(name);
    if (event == -1) {
        return 1;
    }
    SnsSpeechEvent result;
    snsSpeech_AddScripted(snsSpeech, &result, event, comment, nullptr, 4, 0, 0, 0, 0, 0);
    Script::PushInt(L, snsLastScriptedSpeech);
    return 1;
}

// [ped]: its speech stopped, else all
int __cdecl Native_SoundStopCurrentSpeechEvent(lua_State* L) {
    if (lua::gettop(L) > 0) {
        snsSpeech_StopPed(snsSpeech, snPedArg(L, 0), 0, 0x19);
        return 0;
    }
    snsSpeech_StopAll();
    return 0;
}

// [ped (-1), event name, comment (-1)]
int __cdecl Native_SoundSpeechPlaying(lua_State* L) {
    int event = -1;
    int handle = Script::GetIntOpt(L, 0, -1);
    uint8_t* ped = handle == -1 ? nullptr : snGetPed(handle, SN_HANDLE_PED);
    int comment = -1;
    if (lua::gettop(L) > 1) {
        event = snsFindSpeechEvent(Script::GetStringOpt(L, 1, nullptr));
        if (lua::gettop(L) > 2) {
            comment = Script::GetIntOpt(L, 2, -1);
        }
    }
    Script::PushBool(L, snsIsSpeechBusy(ped, event, comment));
    return 1;
}

// [ped (-1), stop now (false)]: the ped's speech stopped, or its model's queued speech
// removed
int __cdecl Native_SoundRemoveAllQueuedSpeech(lua_State* L) {
    int handle = Script::GetIntOpt(L, 0, -1);
    uint8_t* ped = handle == -1 ? nullptr : snGetPed(handle, SN_HANDLE_PED);
    uint8_t now = Script::GetBoolOpt(L, 1, 0);
    if (ped != nullptr) {
        if (now) {
            snsSpeech_StopPed(snsSpeech, ped, 0, 0x19);
            return 0;
        }
        snsSpeechLib_RemoveModel(snsSpeechLib, snAt<int16_t>(ped, 0x10E), 0, -1);
    }
    return 0;
}

// ---- streams ----------------------------------------------------------------------------------

// The music stream (0xCF42D4), the ambience stream (0xCF4324), the stream sounds (0xCF70D4)
inline void* const snsMusicStream = reinterpret_cast<void*>(0x00CF42D4);
inline void* const snsAmbienceStream = reinterpret_cast<void*>(0x00CF4324);
GAME_VAR(void*, snsStreamSounds, 0x00CF70D4);
GAME_FN(void, __thiscall, snsStream_PreloadByName, 0x00591380, void* stream, const char* name, float volume, int loop, int,
        int fadeIn, int fadeOut);
GAME_FN(void, __thiscall, snsStream_Play, 0x00590470, void* stream, int32_t id, int32_t bank, float volume, int loop, int,
        int fadeIn, int fadeOut);
GAME_FN(int32_t, __thiscall, snsStreamSounds_GetBank, 0x005A6DD0, void* sounds, int32_t id);
GAME_FN(int32_t, __cdecl, snsHashStreamName, 0x005A7D60, const char* name, int);
GAME_FN(void, __cdecl, snsMusic_ForceEntry, 0x00591900, int32_t id, float volume, int fadeIn, int fadeOut, int locked);
GAME_FN(void, __cdecl, snsMusic_SetForcedLevel, 0x005922B0, int level, int32_t id, float volume);

// name, volume[, fade in (500), fade out (2000)]
int snsPreloadStream(lua_State* L, int loop) {
    const char* name = Script::GetString(L, 0);
    float volume = Script::GetFloat(L, 1);
    int fadeIn = Script::GetIntOpt(L, 2, 500);
    int fadeOut = Script::GetIntOpt(L, 3, 2000);
    snsStream_PreloadByName(snsMusicStream, name, volume, loop, 6, fadeIn, fadeOut);
    return 0;
}

int __cdecl Native_SoundPreloadStreamNoLoop(lua_State* L) {
    return snsPreloadStream(L, 0);
}

int __cdecl Native_SoundPreloadStream(lua_State* L) {
    return snsPreloadStream(L, 1);
}

// name, volume[, fade in (2500), fade out (2000)]
int snsPlayStream(lua_State* L, void* stream, int loop) {
    const char* name = Script::GetString(L, 0);
    float volume = Script::GetFloat(L, 1);
    int fadeIn = Script::GetIntOpt(L, 2, 2500);
    int fadeOut = Script::GetIntOpt(L, 3, 2000);
    int32_t id = snsHashStreamName(name, 1);
    int32_t bank = snsStreamSounds_GetBank(snsStreamSounds, id);
    snsStream_Play(stream, id, bank, volume, loop, 6, fadeIn, fadeOut);
    return 0;
}

int __cdecl Native_SoundPlayStreamNoLoop(lua_State* L) {
    return snsPlayStream(L, snsMusicStream, 0);
}

int __cdecl Native_SoundPlayStream(lua_State* L) {
    return snsPlayStream(L, snsMusicStream, 1);
}

int __cdecl Native_SoundPlayAmbience(lua_State* L) {
    return snsPlayStream(L, snsAmbienceStream, 1);
}

int snsPlayInteractive(lua_State* L, int locked) {
    const char* name = Script::GetString(L, 0);
    float volume = Script::GetFloat(L, 1);
    int fadeIn = Script::GetIntOpt(L, 2, 2500);
    int fadeOut = Script::GetIntOpt(L, 3, 2000);
    snsMusic_ForceEntry(snsHashStreamName(name, 1), volume, fadeIn, fadeOut, locked);
    return 0;
}

int __cdecl Native_SoundPlayInteractiveStream(lua_State* L) {
    return snsPlayInteractive(L, 0);
}

int __cdecl Native_SoundPlayInteractiveStreamLocked(lua_State* L) {
    return snsPlayInteractive(L, 1);
}

int __cdecl Native_SoundSetMidIntensityStream(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    float volume = Script::GetFloat(L, 1);
    snsMusic_SetForcedLevel(1, snsHashStreamName(name, 1), volume);
    return 0;
}

int __cdecl Native_SoundSetHighIntensityStream(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    float volume = Script::GetFloat(L, 1);
    snsMusic_SetForcedLevel(2, snsHashStreamName(name, 1), volume);
    return 0;
}

// ---- one-shots and loops ----------------------------------------------------------------------

int __cdecl Native_SoundPlay2D(lua_State* L) {
    int id = snsSounds_GetId(snsSounds, Script::GetString(L, 0));
    snsSounds_PlayAt(snsSounds, id, nullptr, 1, nullptr, 0, 0x78, 0, 0, nullptr, nullptr, 6, 1, 1.0f, 0);
    return 0;
}

// name, on
int __cdecl Native_SoundLoopPlay2D(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    uint8_t on = Script::GetBool(L, 1);
    int id = snsSounds_GetId(snsSounds, name);
    if (on) {
        snsSounds_PlayFrontend(snsSounds, id);
        return 0;
    }
    snsSounds_StopFrontend(snsSounds);
    return 0;
}

// x, y, z, name[, curve (1)]
int __cdecl Native_SoundLoopPlay3D(lua_State* L) {
    SnVec pos = {0.0f, 0.0f, 0.0f};
    pos.x = Script::GetFloat(L, 0);
    pos.y = Script::GetFloat(L, 1);
    pos.z = Script::GetFloat(L, 2);
    int id = snsSounds_GetId(snsSounds, Script::GetString(L, 3));
    int curve = 1;
    if (lua::gettop(L) > 4) {
        curve = snsCurveFromName(Script::GetString(L, 4));
    }
    snsLoopSounds_PlayAt(snsLoopSounds, id, &pos, curve, 0);
    return 0;
}

// handle, kind, name, on[, curve (1)]
int __cdecl Native_SoundLoopPlayOnProp(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    int kind = Script::GetInt(L, 1);
    void* entity = snsGetEntity(handle, kind);
    const char* name = Script::GetString(L, 2);
    uint8_t on = Script::GetBool(L, 3);
    int id = snsSounds_GetId(snsSounds, name);
    int curve = 1;
    if (lua::gettop(L) == 5) {
        curve = snsCurveFromName(Script::GetString(L, 4));
    }
    if (on) {
        snsLoopSounds_PlayOn(snsLoopSounds, entity, id, curve, 0);
        return 0;
    }
    snsLoopSounds_Stop(snsLoopSounds, entity, nullptr, id);
    return 0;
}

// ped, name, on[, curve (1)]
int __cdecl Native_SoundLoopPlayOnPed(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    const char* name = Script::GetString(L, 1);
    uint8_t on = Script::GetBool(L, 2);
    int id = snsSounds_GetId(snsSounds, name);
    int curve = 1;
    if (lua::gettop(L) == 4) {
        curve = snsCurveFromName(Script::GetString(L, 3));
    }
    if (on) {
        snsLoopSounds_PlayOn(snsLoopSounds, ped, id, curve, 0);
        return 0;
    }
    snsLoopSounds_Stop(snsLoopSounds, ped, nullptr, id);
    return 0;
}

// x, y, z, name[, curve (1)]
int __cdecl Native_SoundPlay3D(lua_State* L) {
    SnVec pos = {0.0f, 0.0f, 0.0f};
    pos.x = Script::GetFloat(L, 0);
    pos.y = Script::GetFloat(L, 1);
    pos.z = Script::GetFloat(L, 2);
    int id = snsSounds_GetId(snsSounds, Script::GetString(L, 3));
    if (lua::gettop(L) == 5) {
        int curve = snsCurveFromName(Script::GetString(L, 4));
        snsSounds_PlayAt(snsSounds, id, &pos, curve, nullptr, 0, 0x78, 0, 0, nullptr, nullptr, 6, 1, 1.0f, 0);
        return 0;
    }
    snsSounds_PlayAt(snsSounds, id, &pos, 1, nullptr, 0, 0x78, 0, 0, nullptr, nullptr, 6, 1, 1.0f, 0);
    return 0;
}

int __cdecl Native_SoundPlay3DIgnoreFade(lua_State* L) {
    SnVec pos = {0.0f, 0.0f, 0.0f};
    pos.x = Script::GetFloat(L, 0);
    pos.y = Script::GetFloat(L, 1);
    pos.z = Script::GetFloat(L, 2);
    int id = snsSounds_GetId(snsSounds, Script::GetString(L, 3));
    if (lua::gettop(L) == 5) {
        int curve = snsCurveFromName(Script::GetString(L, 4));
        snsSounds_PlayAt(snsSounds, id, &pos, curve, nullptr, 0, 0, 0, 0, nullptr, nullptr, 5, 0, 1.0f, 0);
        return 0;
    }
    snsSounds_PlayAt(snsSounds, id, &pos, 1, nullptr, 0, 0, 0, 0, nullptr, nullptr, 5, 0, 1.0f, 0);
    return 0;
}

GAME_FN(void, __thiscall, snsStream_PlayByName, 0x00591330, void* stream, const char* name, float volume, int flag, int);

// passed, kind: the mission end stinger (kinds 0..3, 5, 6 on foot, 4 nerds, 7 romance,
// 8 bikes, 9 classes, 10 generic; for another kind the game plays an uninitialised name)
int __cdecl Native_SoundPlayMissionEndMusic(lua_State* L) {
    int kind = Script::GetInt(L, 1);
    char name[0x34];
    name[0] = '\0'; // (uninitialised in the game)
    static const char* const passed[11] = {"MS_OnFootVictory.rsm", "MS_OnFootVictory.rsm", "MS_OnFootVictory.rsm",
                                           "MS_OnFootVictory.rsm", "MS_NerdPassStinger.rsm", "MS_OnFootVictory.rsm",
                                           "MS_OnFootVictory.rsm", "MS_RomancePassStinger.rsm", "MS_BikeVictory.rsm",
                                           "MS_ClassSuccessStinger.rsm", "MS_GenericStingSuccess.rsm"};
    static const char* const failed[11] = {"MS_OnFootFailure.rsm", "MS_OnFootFailure.rsm", "MS_OnFootFailure.rsm",
                                           "MS_OnFootFailure.rsm", "MS_NerdFailStinger.rsm", "MS_OnFootFailure.rsm",
                                           "MS_OnFootFailure.rsm", "MS_RomanceFailStinger.rsm", "MS_BikeFailure.rsm",
                                           "MS_ClassFailureStinger.rsm", "MS_GenericStingFail.rsm"};
    const char* const* names = Script::GetBool(L, 0) ? passed : failed;
    if (uint32_t(kind) <= 10) {
        strcpy(name, names[kind]);
    }
    snsStream_PlayByName(snsMusicStream, name, 1.0f, 0, 7);
    return 0;
}

GAME_FN(void, __cdecl, snsConversation_Cancel, 0x006842C0);

int __cdecl Native_SoundCancelConversation(lua_State*) {
    snsConversation_Cancel();
    return 0;
}

// The preloaded speech (0xCFB3B4): +0 waiting, +4 its time-out, +8 play it, +9 ready,
// +0xC the ped (a reference)
struct SnsPreloadedSpeech {
    uint8_t waiting;     // +0
    uint8_t unknown1[3];
    uint32_t timeOut;    // +4
    uint8_t play;        // +8
    uint8_t ready;       // +9
    uint8_t unknownA[2];
    re::Ptr32<void> ped;           // +0xC
};
static_assert(sizeof(SnsPreloadedSpeech) == 0x10);
inline SnsPreloadedSpeech* const snsPreloadedSpeech = reinterpret_cast<SnsPreloadedSpeech*>(0x00CFB3B4);
GAME_VAR(uint32_t, snsTimeInMilliseconds, 0x00C1A9B4);
GAME_FN(void, __thiscall, snsRef_Set, 0x005D7F10, re::Ptr32<void>* ref, void* entity);

// 0x5D4B30 (not in Ghidra's list; the preload's callback): ready; once played (or no
// longer waiting) done (1), else waiting on (0) and stopped past the time-out
uint8_t __cdecl PreloadedSpeechCallback(int event, SnsPreloadedSpeech* state) {
    if (event == 0 || state == nullptr) {
        return 1;
    }
    state->ready = 1;
    if (state->waiting == 0 || state->play != 0) {
        state->waiting = 0;
        return 1;
    }
    if (state->timeOut < snsTimeInMilliseconds) {
        state->waiting = 0;
        snsSpeech_StopPed(snsSpeech, static_cast<uint8_t*>(state->ped), 3, 0x19);
    }
    return 0;
}

// SoundPlayScriptedSpeechEvent held back until played (30 s at most)
int __cdecl Native_SoundPreloadSpeech(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    const char* name = Script::GetString(L, 1);
    int comment = Script::GetInt(L, 2);
    uint8_t flag4 = Script::GetBoolOpt(L, 4, 0);
    uint8_t flag5 = Script::GetBoolOpt(L, 5, 0);
    int curve = 4;
    if (lua::gettop(L) >= 4) {
        curve = snsCurveFromName(Script::GetString(L, 3));
    }
    int event = snsFindSpeechEvent(name);
    if (event == -1) {
        return 1;
    }
    snsPreloadedSpeech->timeOut = snsTimeInMilliseconds + 30000;
    snsRef_Set(&snsPreloadedSpeech->ped, ped);
    snsPreloadedSpeech->ready = 0;
    snsPreloadedSpeech->waiting = 1;
    snsPreloadedSpeech->play = 0;
    SnsSpeechEvent result;
    snsSpeech_AddScripted(snsSpeech, &result, event, comment, ped, curve, 0x005D4B30, 0x00CFB3B4, 0, flag4, flag5);
    if (snsLastScriptedSpeech == -2) {
        snsDebugPrint(0,
                      "\n*************** ATTENTION SCRIPTER ****************\n\nYou tried to play %s (COMMENT#: %d) on ped model "
                      "id %d- this model does not have this speech event!\n\n======================================================\n\n",
                      name, comment, int(snAt<int16_t>(ped, 0x10E)));
    }
    Script::PushInt(L, snsLastScriptedSpeech);
    return 1;
}

// ---- ambient spawners (natives the dump names no script name for) ---------------------------

// The spawners (0xC2C12C)
GAME_VAR(void*, snsSpawners, 0x00C2C12C);
GAME_FN(uint8_t*, __thiscall, snsSpawners_GetAmbient, 0x004A1A80, void* spawners, void* userdata);
GAME_FN(void, __thiscall, snsSpawners_SwitchAmbientNear, 0x004A1D90, void* spawners, const ScriptVector* pos, float radius, int on);

// 0x5D4D20: spawner (userdata), on: +0xBC bit 2 from `on`, bit 1 set
int __cdecl Native_SpawnerSetEnabled_5D4D20(lua_State* L) {
    uint8_t* spawner = snsSpawners_GetAmbient(snsSpawners, Script::GetUserdata(L, 0));
    uint8_t on = Script::GetBool(L, 1);
    uint8_t flags = snAt<uint8_t>(spawner, 0xBC);
    snAt<uint8_t>(spawner, 0xBC) = uint8_t((((on << 2) ^ flags) & 4) ^ flags) | 2;
    return 0;
}

// 0x5D4D70: bit 1 cleared
int __cdecl Native_SpawnerClearOverride_5D4D70(lua_State* L) {
    uint8_t* spawner = snsSpawners_GetAmbient(snsSpawners, Script::GetUserdata(L, 0));
    snAt<uint8_t>(spawner, 0xBC) &= 0xFD;
    return 0;
}

// 0x5D4DA0: x, y, z, radius, on (the flag goes as a dword with uninitialised upper bytes)
int __cdecl Native_SpawnerSwitchAmbientNear_5D4DA0(lua_State* L) {
    ScriptVector pos;
    Script::GetVector(&pos, L, 0);
    float radius = Script::GetFloat(L, 3);
    uint8_t on = Script::GetBool(L, 4);
    snsSpawners_SwitchAmbientNear(snsSpawners, &pos, radius, on);
    return 0;
}

// ---- stats ----------------------------------------------------------------------------------------

GAME_FN(int32_t, __thiscall, snsPed_GetStat, 0x00477060, void* ped, int stat);
GAME_FN(void, __thiscall, snsPed_OverrideStat, 0x004770F0, void* ped, int stat, int value);
GAME_FN(void, __cdecl, snsStats_IncrementFloat, 0x004585B0, int stat, float amount);

// (16 bits; 0 for no ped)
int __cdecl Native_GameGetPedStat(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    if (ped == nullptr) {
        Script::PushInt(L, 0);
        return 1;
    }
    Script::PushInt(L, uint16_t(snsPed_GetStat(ped, Script::GetInt(L, 1))));
    return 1;
}

int __cdecl Native_GameSetPedStat(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    if (ped != nullptr) {
        int stat = Script::GetInt(L, 1);
        int value = uint16_t(Script::GetInt(L, 2));
        snsPed_OverrideStat(ped, stat, value);
    }
    return 0;
}

// stat[, amount (1)]
int __cdecl Native_StatAddToFloat(lua_State* L) {
    float amount = 1.0f;
    if (lua::gettop(L) >= 2) {
        amount = Script::GetFloat(L, 1);
    }
    snsStats_IncrementFloat(Script::GetInt(L, 0), amount);
    return 0;
}

// ---- the system ---------------------------------------------------------------------------------

// The saved position (0xBD0FD8 heading, 0xBD0FDC a flag, 0xBD0FE0 x, y, z; 0x9F4760 the area)
GAME_VAR(float, snsSavedHeading, 0x00BD0FD8);
GAME_VAR(uint8_t, snsSavedFlag, 0x00BD0FDC);
GAME_ARRAY(float, snsSavedPosition, 0x00BD0FE0);
GAME_VAR(int32_t, snsSavedArea, 0x009F4760);

// x, y, z, heading (degrees), area, flag
int __cdecl Native_SystemGetSavedPositionInformation(lua_State* L) {
    Script::PushFloat(L, snsSavedPosition[0]);
    Script::PushFloat(L, snsSavedPosition[1]);
    Script::PushFloat(L, snsSavedPosition[2]);
    Script::PushFloat(L, float(double(snsSavedHeading) * 180.0 / double(3.1415927f)));
    Script::PushInt(L, snsSavedArea);
    Script::PushBool(L, snsSavedFlag);
    return 6;
}

GAME_VAR(uint8_t, snsInTitleMenu0, 0x020C32EB);
GAME_VAR(uint8_t, snsInTitleMenu1, 0x020C32F2);

int __cdecl Native_IsInTitleMenu(lua_State* L) {
    if (snsInTitleMenu0 == 0 && snsInTitleMenu1 == 0) {
        Script::PushBool(L, 0);
        return 1;
    }
    Script::PushBool(L, 1);
    return 1;
}

// ---- tagging ------------------------------------------------------------------------------------

// The tagging (0x20C8A00): the tag a ped just finished (its trigger's userdata, the ped)
inline void* const snsTagging = reinterpret_cast<void*>(0x020C8A00);
GAME_FN(uint8_t, __thiscall, snsTagging_JustFinished, 0x0072C100, void* tagging, re::Ptr32<void>* userdata, int32_t* ped);
inline void* const snsTriggerNames = reinterpret_cast<void*>(0x020C7E98);
GAME_FN(int, __thiscall, snsTriggerNames_Find, 0x006D79C0, void* names, void* userdata);

// [ped]: finished (by that ped), the tag's trigger slot
int __cdecl Native_TaggingPedHasJustFinished(lua_State* L) {
    uint8_t finished = 0;
    int slot = 0;
    re::Ptr32<void> userdata;
    int32_t ped;
    if (snsTagging_JustFinished(snsTagging, &userdata, &ped)) {
        int found = snsTriggerNames_Find(snsTriggerNames, userdata);
        if (lua::gettop(L) != 1 || ped == Script::GetInt(L, 0)) {
            slot = found;
            finished = 1;
        }
    }
    Script::PushBool(L, finished);
    Script::PushInt(L, slot);
    return 2;
}

// ---- text -----------------------------------------------------------------------------------------

// The text parameters (up to 3 at 0xCFB3C8, 0x104 bytes each: +0 the kind (1 an int,
// 2 a float, 3 a text key, 4 a wide string), +4 the value; the count at 0xCFB6D4)
struct SnsTextParam {
    int32_t kind;           // +0
    uint8_t value[0x100];   // +4
};
static_assert(sizeof(SnsTextParam) == 0x104);
inline SnsTextParam* const snsTextParams = reinterpret_cast<SnsTextParam*>(0x00CFB3C8);
GAME_VAR(int32_t, snsNumTextParams, 0x00CFB6D4);
// The big message's parameters (0x514240..) and the help box's (0x5142E0..)
GAME_FN(void, __cdecl, snsBig_AddNumber, 0x00514240, float value);
GAME_FN(void, __cdecl, snsBig_AddKey, 0x00514260, const void* key);
GAME_FN(void, __cdecl, snsBig_AddString, 0x00514280, const void* text);
GAME_FN(void, __cdecl, snsHelp_AddNumber, 0x005142E0, float value);
GAME_FN(void, __cdecl, snsHelp_AddKey, 0x00514300, const void* key);
GAME_FN(void, __cdecl, snsHelp_AddString, 0x00514320, const void* text);
GAME_FN(void, __cdecl, snsBig_Clear, 0x005142C0);
GAME_FN(void, __cdecl, snsBig_Show, 0x005142A0, const char* key, float time);
GAME_FN(void, __cdecl, snsHelp_Show, 0x00514340, const char* key, float time);
GAME_FN(void, __cdecl, snsHelp_Show3, 0x00514420, const char* key, float time);
GAME_FN(void, __cdecl, snsHelp_Show4, 0x00514470, const char* key, float time);
GAME_FN(void, __cdecl, snsHelp_Show5, 0x005144C0, const char* key, float time);
// The generic text HUD (0xC674C4) and the text table (0x20C2FA4)
GAME_VAR(void*, snsGenericText, 0x00C674C4);
GAME_FN(void, __thiscall, snsGenericText_AddLine, 0x00540250, void* hud, const uint16_t* text, float time, int);
inline void* const snsTheText = reinterpret_cast<void*>(0x020C2FA4);
GAME_FN(const uint16_t*, __thiscall, snsText_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __cdecl, snsAsciiToText, 0x00690090, const char* ascii, uint16_t* text);
GAME_VAR(int32_t, snsWidescreen, 0x00C3CD0C);

// The parameters handed on (to the help box's, else the big message's) and dropped
void snsFlushTextParams(bool help) {
    for (int i = 0; i < snsNumTextParams; i++) {
        SnsTextParam* p = &snsTextParams[i];
        switch (p->kind) {
        case 1:
            (help ? snsHelp_AddNumber : snsBig_AddNumber)(float(*reinterpret_cast<int32_t*>(p->value)));
            break;
        case 2:
            (help ? snsHelp_AddNumber : snsBig_AddNumber)(*reinterpret_cast<float*>(p->value));
            break;
        case 3:
            (help ? snsHelp_AddKey : snsBig_AddKey)(p->value);
            break;
        case 4:
            (help ? snsHelp_AddString : snsBig_AddString)(p->value);
            break;
        default:
            break;
        }
    }
    snsNumTextParams = 0;
}

// 0x5D5510 (cdecl, a byte): snsFlushTextParams
void __cdecl FlushTextParams(int help) {
    snsFlushTextParams(uint8_t(help) != 0);
}

// key, time[, style (2 in widescreen, else 0)]: 0 a generic line, 1 the big message, 2
// the help box, 3..5 other help boxes (the parameters go only with 1 and 2)
int __cdecl Native_TextPrintBig(lua_State* L) {
    const char* key = Script::GetString(L, 0);
    int style = snsWidescreen != 0 ? 2 : 0;
    if (lua::gettop(L) == 3) {
        style = Script::GetInt(L, 2);
    }
    if (style == 0) {
        void* hud = snsGenericText;
        float time = Script::GetFloat(L, 1);
        snsGenericText_AddLine(hud, snsText_Get(snsTheText, key), time, -1);
        return 0;
    }
    if (style == 1) {
        snsFlushTextParams(false);
        snsBig_Show(key, Script::GetFloat(L, 1));
        return 0;
    }
    if (style == 2) {
        snsFlushTextParams(true);
        snsHelp_Show(key, Script::GetFloat(L, 1));
        return 0;
    }
    if (style == 3) {
        snsHelp_Show3(key, Script::GetFloat(L, 1));
    } else if (style == 4) {
        snsHelp_Show4(key, Script::GetFloat(L, 1));
    } else if (style == 5) {
        snsHelp_Show5(key, Script::GetFloat(L, 1));
    }
    return 0;
}
RE_UNHOOKED_FN("Native_TextPrintF", 0x005D57E0, &Native_TextPrintBig); // (TextPrint / TextPrintF: a jmp to TextPrintBig)

// text (not a key), time[, style[, 6's time]]: as TextPrintBig, the text as the one
// parameter of "UNLOCALIZED_HACK"
int __cdecl Native_TextPrintString(lua_State* L) {
    const char* ascii = Script::GetString(L, 0);
    int style = snsWidescreen != 0 ? 2 : 0;
    uint16_t text[256];
    snsAsciiToText(ascii, text);
    if (lua::gettop(L) == 3 || lua::gettop(L) == 4) {
        style = Script::GetInt(L, 2);
    }
    if (style == 0) {
        void* hud = snsGenericText;
        float time = Script::GetFloat(L, 1);
        snsGenericText_AddLine(hud, text, time, -1);
        return 0;
    }
    if (style == 6) {
        void* hud = snsGenericText;
        int value = Script::GetInt(L, 3);
        float time = Script::GetFloat(L, 1);
        snsGenericText_AddLine(hud, text, time, value);
        return 0;
    }
    if (style == 1) {
        snsBig_Clear();
        snsBig_AddString(text);
        snsBig_Show("UNLOCALIZED_HACK", Script::GetFloat(L, 1));
        return 0;
    }
    if (style == 2) {
        snsHelp_AddString(text);
        snsHelp_Show("UNLOCALIZED_HACK", Script::GetFloat(L, 1));
    }
    return 0;
}

inline char* __cdecl snsStrncpy(char* to, const char* from, size_t count) { return crt::Strncpy(to, from, count); }

int __cdecl Native_TextAddParamNum(lua_State* L) {
    if (snsNumTextParams < 3) {
        float value = Script::GetFloat(L, 0);
        SnsTextParam* p = &snsTextParams[snsNumTextParams];
        p->kind = 2;
        snsNumTextParams++;
        *reinterpret_cast<float*>(p->value) = value;
    }
    return 0;
}

int __cdecl Native_TextAddParamString(lua_State* L) {
    if (snsNumTextParams < 3) {
        const char* key = Script::GetString(L, 0);
        SnsTextParam* p = &snsTextParams[snsNumTextParams];
        p->kind = 3;
        snsStrncpy(reinterpret_cast<char*>(p->value), key, 0x7F);
        snsNumTextParams++;
    }
    return 0;
}

int __cdecl Native_TextAddNonLocalizedString(lua_State* L) {
    if (snsNumTextParams < 3) {
        const char* ascii = Script::GetString(L, 0);
        SnsTextParam* p = &snsTextParams[snsNumTextParams];
        p->kind = 4;
        snsAsciiToText(ascii, reinterpret_cast<uint16_t*>(p->value));
        snsNumTextParams++;
    }
    return 0;
}

// ---- credits --------------------------------------------------------------------------------------

GAME_FN(void, __cdecl, snsCredits_PrintText, 0x0043A810, float x, float y, const char* text, int flag);
GAME_FN(void, __cdecl, snsCredits_Setup, 0x0043A7C0, uint32_t color, float scale, int font);

// x, y, flag, text (the flag goes as a dword with uninitialised upper bytes)
int __cdecl Native_CreditPrintText(lua_State* L) {
    float x = Script::GetFloat(L, 0);
    float y = Script::GetFloat(L, 1);
    uint8_t flag = Script::GetBool(L, 2);
    const char* text = Script::GetString(L, 3);
    snsCredits_PrintText(x, y, text, flag);
    return 0;
}

// r, g, b, a (bytes of one colour), scale, font
int __cdecl Native_CreditSetup(lua_State* L) {
    uint32_t color = uint8_t(Script::GetInt(L, 0));
    color |= uint32_t(uint8_t(Script::GetInt(L, 1))) << 8;
    color |= uint32_t(uint8_t(Script::GetInt(L, 2))) << 16;
    color |= uint32_t(uint8_t(Script::GetInt(L, 3))) << 24;
    float scale = Script::GetFloat(L, 4);
    snsCredits_Setup(color, scale, Script::GetInt(L, 5));
    return 0;
}

// ---- missions and the tutorial box (natives the dump names no script name for) ------------------

// The mission manager (0x20C3CA0)
inline void* const snsMissionMgr = reinterpret_cast<void*>(0x020C3CA0);
GAME_FN(int32_t, __thiscall, snsMissionMgr_Find, 0x006A9FD0, void* mgr, const uint32_t* name);
GAME_FN(uint8_t*, __thiscall, snsMissionMgr_GetStats, 0x006AA7C0, void* mgr, int32_t mission);
GAME_FN(void, __thiscall, snsMissionMgr_Fn6AD290, 0x006AD290, void* mgr, int);
GAME_FN(void*, __thiscall, snsNameKey_Construct, 0x005FA910, uint32_t* self, const char* name);
GAME_FN(void, __thiscall, snsNameKey_Destruct, 0x0049A650, uint32_t* self);   // (empty)
GAME_VAR(uint8_t, snsMissionFlag20C43FC, 0x020C43FC);

// 0x5D5CA0: mission (name): its stats' +4 bit 1 set, then 0x6AD290(1)
int __cdecl Native_MissionSetFlag_5D5CA0(lua_State* L) {
    uint32_t key;
    snsNameKey_Construct(&key, Script::GetString(L, 0));
    int32_t mission = snsMissionMgr_Find(snsMissionMgr, &key);
    snsMissionMgr_GetStats(snsMissionMgr, mission)[4] |= 2;
    snsMissionMgr_Fn6AD290(snsMissionMgr, 1);
    snsNameKey_Destruct(&key);
    return 0;
}

// 0x5D5CF0: mission (name): looked up (unused), 0x20C43FC set
int __cdecl Native_MissionSetFlag_5D5CF0(lua_State* L) {
    uint32_t key;
    snsNameKey_Construct(&key, Script::GetString(L, 0));
    snsMissionMgr_Find(snsMissionMgr, &key);
    snsMissionFlag20C43FC = 1;
    snsNameKey_Destruct(&key);
    return 0;
}

// The tutorial box (0xC67540)
GAME_VAR(uint8_t*, snsTutorial, 0x00C67540);
GAME_FN(void, __thiscall, snsTutorial_SetText, 0x0055B6E0, void* hud, const char* key);
GAME_FN(void, __thiscall, snsTutorial_Set38, 0x0055B720, void* hud, int value);

// 0x5D5D30: key[, value (-1), flag (+0x3C)]: shown (virtual +0x14, with 1)
int __cdecl Native_TutorialShow_5D5D30(lua_State* L) {
    const char* key = Script::GetString(L, 0);
    uint8_t* hud = snsTutorial;
    snsTutorial_SetText(hud, key);
    int value = -1;
    if (Script::GetArgCount(L) >= 2) {
        value = Script::GetInt(L, 1);
    }
    if (Script::GetArgCount(L) >= 3) {
        snAt<uint8_t>(hud, 0x3C) = Script::GetBool(L, 2);
    }
    snsTutorial_Set38(hud, value);
    snVCall<void>(hud, 0x14, 1);
    return 0;
}

// 0x5D5DB0: hidden (virtual +0x1C)
int __cdecl Native_TutorialHide_5D5DB0(lua_State*) {
    snVCall<void>(snsTutorial, 0x1C);
    return 0;
}

} // namespace

#define SNS_REVERSED(name, address)       \
    static void* Original_Native_##name; \
    REVERSED("Native_" #name, address, RE_BODY(Native_##name), nullptr, &Original_Native_##name, re::Mode::Original)

SNS_REVERSED(SoundEmitterStart, 0x005D3530);
SNS_REVERSED(SoundEmitterStop, 0x005D35D0);
SNS_REVERSED(SoundSetScriptedAmbience, 0x005D36F0);
SNS_REVERSED(SoundPlayAmbientSpeechEvent, 0x005D3840);
SNS_REVERSED(SoundPlayScriptedSpeechEvent, 0x005D3910);
SNS_REVERSED(SoundPlayScriptedSpeechEvent_2D, 0x005D3A70);
SNS_REVERSED(SoundStopCurrentSpeechEvent, 0x005D3B60);
SNS_REVERSED(SoundSpeechPlaying, 0x005D3BB0);
SNS_REVERSED(SoundRemoveAllQueuedSpeech, 0x005D3CB0);
SNS_REVERSED(SoundPreloadStreamNoLoop, 0x005D3D50);
SNS_REVERSED(SoundPreloadStream, 0x005D3DB0);
SNS_REVERSED(SoundPlay2D, 0x005D3E60);
SNS_REVERSED(SoundLoopPlay2D, 0x005D3EB0);
SNS_REVERSED(SoundLoopPlay3D, 0x005D3F00);
SNS_REVERSED(SoundLoopPlayOnProp, 0x005D3FD0);
SNS_REVERSED(SoundLoopPlayOnPed, 0x005D4070);
SNS_REVERSED(SoundPlay3D, 0x005D4110);
SNS_REVERSED(SoundPlay3DIgnoreFade, 0x005D41F0);
SNS_REVERSED(SoundPlayStreamNoLoop, 0x005D42D0);
SNS_REVERSED(SoundPlayStream, 0x005D4350);
SNS_REVERSED(SoundPlayAmbience, 0x005D43D0);
SNS_REVERSED(SoundPlayInteractiveStream, 0x005D4450);
SNS_REVERSED(SoundPlayInteractiveStreamLocked, 0x005D44C0);
SNS_REVERSED(SoundSetMidIntensityStream, 0x005D4530);
SNS_REVERSED(SoundSetHighIntensityStream, 0x005D4580);
SNS_REVERSED(SoundPlayMissionEndMusic, 0x005D4850);
SNS_REVERSED(SoundCancelConversation, 0x005D4B20);
static void* Original_PreloadedSpeechCallback;
REVERSED("Script::PreloadedSpeechCallback", 0x005D4B30, RE_BODY(PreloadedSpeechCallback), nullptr,
         &Original_PreloadedSpeechCallback, re::Mode::Original);
SNS_REVERSED(SoundPreloadSpeech, 0x005D4B80);
SNS_REVERSED(SpawnerSetEnabled_5D4D20, 0x005D4D20);
SNS_REVERSED(SpawnerClearOverride_5D4D70, 0x005D4D70);
SNS_REVERSED(SpawnerSwitchAmbientNear_5D4DA0, 0x005D4DA0);
SNS_REVERSED(GameGetPedStat, 0x005D4E00);
SNS_REVERSED(GameSetPedStat, 0x005D4E70);
SNS_REVERSED(StatAddToFloat, 0x005D4F80);
SNS_REVERSED(SystemGetSavedPositionInformation, 0x005D51E0);
SNS_REVERSED(IsInTitleMenu, 0x005D52E0);
SNS_REVERSED(TaggingPedHasJustFinished, 0x005D5430);
static void* Original_FlushTextParams;
REVERSED("Script::FlushTextParams", 0x005D5510, RE_BODY(FlushTextParams), nullptr, &Original_FlushTextParams,
         re::Mode::Original);
SNS_REVERSED(TextPrintBig, 0x005D55C0);
SNS_REVERSED(TextPrintString, 0x005D57F0);
SNS_REVERSED(TextAddParamNum, 0x005D5930);
SNS_REVERSED(TextAddParamString, 0x005D5980);
SNS_REVERSED(TextAddNonLocalizedString, 0x005D59D0);
SNS_REVERSED(CreditPrintText, 0x005D5A60);
SNS_REVERSED(CreditSetup, 0x005D5C30);
SNS_REVERSED(MissionSetFlag_5D5CA0, 0x005D5CA0);
SNS_REVERSED(MissionSetFlag_5D5CF0, 0x005D5CF0);
SNS_REVERSED(TutorialShow_5D5D30, 0x005D5D30);
SNS_REVERSED(TutorialHide_5D5DB0, 0x005D5DB0);
