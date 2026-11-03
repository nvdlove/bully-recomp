// The Minigame base class (vtable 0x948F10) and the minigame manager around it
// (0x704810..0x705FB0, 0x7074E0, the Minigame* script natives 0x708A30..0x709360): the
// running minigame (one at a time, 0x20C8704) created by kind, its texture and sound sets,
// the exit / info popup, the timer warnings, the high score tables (10 of 5: a three
// letter name each), the grades and completion screens' state (drawn in
// MinigameScreens.cpp) and the small effect slots the minigames spawn.
//
// The Minigame: +0x4 active, +0x5 HUD shown, +0x6 success, +0xC / +0x10 the info box's
// textures, +0x14 / +0x18 / +0x1C the texture set (count, txd, textures), +0x20 / +0x28 /
// +0x30 the timer warnings played (+0x24 / +0x2C their times), +0x34 the time left,
// +0x38 / +0x3C the kinds, +0x44 / +0x48 / +0x4C the sound set (count, bank, ids), +0x50
// the popup (0x60 bytes), +0xB0..+0xC0 sounds playing.
// The popup: +0x0 its kind, +0x4 / +0x8 the buttons (-1: none), +0xC the choice (1 / 2),
// +0x10 its title's text key, +0x30 its footer's, +0x50 showing, +0x51 a result waiting,
// +0x54 the result, +0x58 / +0x5C its textures (4:3, widescreen).

#include <cstdint>
#include <cstring>

#include "game/SchoolClasses.h"
#include "game/ScriptApi.h"
#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using school::At;
using school::GameString;

// ---- the game's state -------------------------------------------------------------------------

GAME_VAR(uint8_t*, mbCurrent, 0x020C8704);
GAME_VAR(int32_t, mbUserStartResponse, 0x020C87AC);   // popups up (the pause's count)
GAME_VAR(uint8_t*, mbPunishment, 0x00C0F78C);
GAME_VAR(uint8_t, mbHoldCompletionC3CF82, 0x00C3CF82);
GAME_VAR(uint8_t, mbSoundsPaused, 0x00CF3DD8);
GAME_VAR(uint8_t, mbPauseA, 0x020C32E9);
GAME_VAR(uint8_t, mbPauseB, 0x020C32EA);
GAME_VAR(uint8_t, mbPauseC, 0x020C32EB);
GAME_VAR(uint8_t, mbPauseD, 0x020C32F2);
// the grades screen
GAME_VAR(int32_t, mbGradesClass, 0x020C86F8);
GAME_VAR(int32_t, mbGradesGrade, 0x020C8654);
GAME_ARRAY(char, mbGradesText, 0x020C8710);   // [0x20]
GAME_VAR(uint8_t, mbShowingGrades, 0x020C8708);
GAME_VAR(float, mbGradesTime, 0x020C870C);
GAME_ARRAY(int32_t, mbGradeNeeded, 0x00C1B02C);   // the player info's pass grade per class
// the completion screen
GAME_ARRAY(char, mbCompletionTitle, 0x020C8738);      // [0x20]
GAME_ARRAY(char, mbCompletionSubtitle, 0x020C8758);   // [0x20]
GAME_ARRAY(char, mbCompletionExtra, 0x020C8778);      // [0x20]
GAME_VAR(uint8_t, mbCompletionFading, 0x020C8709);
GAME_VAR(uint8_t, mbCompletionColoured, 0x020C870A);
GAME_VAR(uint8_t, mbHoldCompletion, 0x020C870B);
GAME_VAR(uint8_t, mbUberCompletion, 0x020C87A8);
GAME_VAR(uint8_t, mbCompletion87A9, 0x020C87A9);
GAME_VAR(uint8_t, mbErrandDone, 0x020C87AA);
GAME_ARRAY(uint8_t, mbCompletionColour, 0x020C87F0);
GAME_ARRAY(uint8_t, mbCompletionDefaultColour, 0x020C87F8);
GAME_VAR(float, mbCompletionMoney, 0x020C8734);
GAME_VAR(int32_t, mbCompletion87A4, 0x020C87A4);
GAME_VAR(float, mbCompletionTime, 0x020C87A0);
GAME_VAR(int32_t, mbCompletionMsgCount, 0x020C8798);
GAME_VAR(float, mbCompletionMsgTime, 0x020C879C);
GAME_ARRAY(char, mbCompletionMsgs, 0x020C8658);       // [5][0x20]
GAME_ARRAY(int32_t, mbCompletionMsgValues, 0x020C8640);
GAME_VAR(int32_t, mbCrest, 0x020C8730);   // the crest texture (0: not loaded)
GAME_VAR(int32_t, mbCrestTxd, 0x00BC6C68);
GAME_VAR(int32_t, mbWidescreenC3CD0C, 0x00C3CD0C);
GAME_VAR(float, mbC3CD14, 0x00C3CD14);
GAME_VAR(uint8_t*, mbProgress, 0x020CE75C);
GAME_VAR(uint8_t*, mbStatsColours, 0x00C67558);
// the high scores
GAME_ARRAY(uint32_t, mbHighScores, 0x020C83E8);   // [10][5]
GAME_ARRAY(uint16_t, mbHighNames, 0x020C84B0);    // [10][5][4]
GAME_ARRAY(uint32_t, mbDefaultScores, 0x00BC6E00);
GAME_ARRAY(uint32_t, mbDefaultNames, 0x00BC6C70);
// the effect slots: 4 per minigame index, each a proxy of 8 bytes
GAME_ARRAY(int32_t, mbEffectNext, 0x020C86FC);
inline uint8_t* const mbEffects = reinterpret_cast<uint8_t*>(0x020C87B0);
GAME_VAR(float, mbEffectZ, 0x00BC6C6C);
GAME_VAR(void*, mbEffectLibrary, 0x00B6F1E8);
GAME_VAR(void*, mbEffectManager, 0x00B6F1D4);
GAME_VAR(void*, mbHud, 0x00C67540);
GAME_VAR(void*, mbHudText, 0x00C67548);
GAME_VAR(uint8_t*, mbPopulation, 0x00C2C108);
GAME_VAR(void*, mbTraffic, 0x00BCD080);
GAME_VAR(uint8_t*, mbSystem, 0x00C0F5E4);
GAME_VAR(uint8_t*, mbPlayerPed, 0x00C1AEA8);
inline void* const mbPlayerInfo = reinterpret_cast<void*>(0x00C1AEA8);
inline void* const mbSounds = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const mbSoundBanks = reinterpret_cast<void*>(0x00CF5DC4);
inline void* const mbCamera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const mbObjectives = reinterpret_cast<void*>(0x020C4420);
inline void* const mbTheText = reinterpret_cast<void*>(0x020C2FA4);

// ---- the game's calls ------------------------------------------------------------------------

GAME_FN(void*, __cdecl, MB_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, MB_Free, 0x005EEAB0, void* p);
inline char* __cdecl MB_Strncpy(char* dst, const char* src, size_t n) { return crt::Strncpy(dst, src, n); }
inline void* __cdecl MB_Memset(void* p, int value, size_t size) { return crt::Memset(p, value, size); }
GAME_FN(void, __cdecl, MB_SetGameClock, 0x00426070, int hours, int minutes);
GAME_FN(int, __cdecl, MB_FindTxdSlot, 0x005F22D0, const char* name, int);
GAME_FN(void, __cdecl, MB_RequestModel, 0x00532020, int id, int flags);
GAME_FN(void, __cdecl, MB_LoadAllRequestedModels, 0x00533EE0, int);
GAME_FN(void, __cdecl, MB_RemoveModel, 0x0052F640, int id);
GAME_FN(void, __cdecl, MB_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, MB_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void*, __cdecl, MB_FindTexture, 0x005F1770, const char* name, const char* mask);
GAME_FN(void, __cdecl, MB_PopCurrentTxd, 0x005F0AA0);
GAME_FN(void, __cdecl, MB_NiRelease, 0x005F0F90, void* object);
GAME_FN(int32_t, __thiscall, MB_SoundBanks_Find, 0x00597FC0, void* banks, const char* name);
GAME_FN(void, __thiscall, MB_SoundBanks_Load, 0x0059A3E0, void* banks, int32_t bank, int, int);
GAME_FN(void, __thiscall, MB_SoundBanks_Unload, 0x0059A420, void* banks, int32_t bank, int, int);
GAME_FN(int32_t, __thiscall, MB_Sounds_GetId, 0x00595B80, void* sounds, const char* name);
GAME_FN(void*, __thiscall, MB_Sounds_Play, 0x0059B8A0, void* sounds, int32_t id, const float* pos, int, uintptr_t, int,
        int, int, int, uintptr_t, uintptr_t, int, int, float, int);
GAME_FN(void, __thiscall, MB_FrontendSound, 0x0059C150, void* sounds, int sound);
GAME_FN(uint8_t, __cdecl, MB_Pad_JustDown, 0x00738900, int control, int pad);
GAME_FN(float, __cdecl, MB_Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(void, __cdecl, MB_PadsUpdate, 0x00738610);
GAME_FN(void, __cdecl, MB_Pad_StopRumble, 0x00738780);
GAME_FN(void, __cdecl, MB_Pause_45B810, 0x0045B810);
GAME_FN(void, __cdecl, MB_Unpause_45B820, 0x0045B820);
GAME_FN(void, __cdecl, MB_Fn45B960, 0x0045B960, int);
GAME_FN(void, __thiscall, MB_Camera_Fade, 0x004F1AD0, void* camera, float time, int, int);
GAME_FN(uint8_t*, __thiscall, MB_RGBA_Set, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __thiscall, MB_PlayerInfo_SetClassPassed, 0x00446730, void* info, int cls, int passed);
GAME_FN(uint8_t, __thiscall, MB_PlayerInfo_IsMiniObjectiveComplete, 0x00446860, void* info, int which);
GAME_FN(void, __thiscall, MB_PlayerInfo_SetMiniObjectiveComplete, 0x004467F0, void* info, int which, int done);
GAME_FN(int32_t, __thiscall, MB_PlayerInfo_GetErrands, 0x004467B0, void* info, int errand);
GAME_FN(void, __thiscall, MB_PlayerInfo_SetErrands, 0x00446790, void* info, int errand, int count);
GAME_FN(int32_t, __thiscall, MB_PlayerInfo_GetErrandCompletion, 0x004467E0, void* info, int errand);
GAME_FN(void, __thiscall, MB_PlayerInfo_SetErrandCompletion, 0x004467C0, void* info, int errand, int count);
GAME_FN(void, __thiscall, MB_Ped_SetMoney, 0x004752A0, void* ped, int money);
GAME_FN(void, __thiscall, MB_Objectives_6AE260, 0x006AE260, void* objectives);
GAME_FN(void, __thiscall, MB_Objectives_SetReminderTime, 0x006AE580, void* objectives, float time);
GAME_FN(void, __thiscall, MB_HUDText_SetText, 0x0055B130, void* text, int, uintptr_t);
GAME_FN(void, __thiscall, MB_PopulationRevert, 0x0049C6D0, void* population);
GAME_FN(void, __thiscall, MB_TrafficRevert, 0x0041A860, void* traffic);
GAME_FN(void, __thiscall, MB_PopulationClearDisabled, 0x0049C610, void* population);
GAME_FN(void, __thiscall, MB_EffectProxy_Kill, 0x00668BA0, void* proxy);
GAME_FN(uint16_t, __thiscall, MB_EffectLibrary_Find, 0x00667B60, void* library, const char* name);
GAME_FN(void, __thiscall, MB_EffectManager_Start, 0x00668290, void* manager, uint32_t* id, void* proxy, int);
GAME_FN(void, __thiscall, MB_EffectProxy_SetPosition, 0x00669040, void* proxy, const float* pos, int);
GAME_FN(void, __cdecl, MB_WStrNCopy, 0x0068FFD0, uint16_t* dst, const uint16_t* src, int n);
GAME_FN(int, __cdecl, MB_WStrNCmp, 0x00690040, const uint16_t* a, const uint16_t* b, int n);
GAME_FN(uint8_t, __cdecl, MB_Save_CheckValue, 0x00733320, int, int, int32_t value, int32_t expected);
GAME_FN(uint8_t, __cdecl, MB_Save_CheckBlock, 0x00733620, const char* name, void* data, int size);
GAME_FN(int32_t, __cdecl, MB_Stat_GetInt, 0x00459A00, int stat);
GAME_FN(void, __cdecl, MB_Stat_SetInt, 0x00458460, int stat, int32_t value);
GAME_FN(void, __cdecl, MB_Stat_AddInt, 0x00458480, int stat, int32_t value);
GAME_FN(uint8_t, __cdecl, MB_Fn43AC10, 0x0043AC10);
GAME_FN(void, __cdecl, MB_Fn43BE40, 0x0043BE40, int);
GAME_FN(void, __cdecl, MB_Font_Format, 0x00688ED0, uint16_t* out, const uint16_t* text, int size);
GAME_FN(const uint16_t*, __thiscall, MB_Text_Get, 0x0068FE80, void* text, const char* key);
// the minigames' constructors, by kind 1..18
GAME_FN(void*, __thiscall, MB_ClassEnglish_Construct, 0x006EB880, void* mg);
GAME_FN(void*, __thiscall, MB_ClassMath_Construct, 0x006F8D70, void* mg);
GAME_FN(void*, __thiscall, MB_ClassGeography_Construct, 0x006F2EA0, void* mg);
GAME_FN(void*, __thiscall, MB_ClassBiology_Construct, 0x006E4140, void* mg);
GAME_FN(void*, __thiscall, MB_ClassChem_Construct, 0x006E81E0, void* mg);
GAME_FN(void*, __thiscall, MB_ClassArt_Construct, 0x006E0F80, void* mg);
GAME_FN(void*, __thiscall, MB_Striker_Construct, 0x006D9B80, void* mg);
GAME_FN(void*, __thiscall, MB_BallToss_Construct, 0x006D7CD0, void* mg);
GAME_FN(void*, __thiscall, MB_Darts_Construct, 0x00701660, void* mg);
GAME_FN(void*, __thiscall, MB_Lock_Construct, 0x00703A40, void* mg);
GAME_FN(void*, __thiscall, MB_LawnMowing_Construct, 0x00709550, void* mg);
GAME_FN(void*, __thiscall, MB_SoccerP_Construct, 0x00714530, void* mg);
GAME_FN(void*, __thiscall, MB_DunkTank_Construct, 0x006D82E0, void* mg);
GAME_FN(void*, __thiscall, MB_Race_Construct, 0x00711340, void* mg);
GAME_FN(void*, __thiscall, MB_Arcade_Construct, 0x006FE4B0, void* mg);
GAME_FN(void*, __thiscall, MB_Shooting_Construct, 0x006D8CA0, void* mg);
GAME_FN(void*, __thiscall, MB_KeepUps_Construct, 0x00702350, void* mg);
GAME_FN(void*, __thiscall, MB_ClassMusic_Construct, 0x006FC790, void* mg);
// (this file's)
GAME_FN(int, __thiscall, MB_Popup_Result, 0x00705080, void* popup);
GAME_FN(void, __thiscall, MB_UnloadTextures, 0x00704DE0, void* mg);
GAME_FN(void, __thiscall, MB_EndSounds, 0x00704FD0, void* mg);
GAME_FN(void, __thiscall, MB_LoadInfoTextures, 0x007057B0, void* mg);
GAME_FN(void, __thiscall, MB_UnloadInfoTextures, 0x00705850, void* mg);
GAME_FN(uint8_t, __cdecl, MB_Create, 0x00704890, int kind);
GAME_FN(void, __cdecl, MB_Destroy, 0x00704C60, int restoreWorld);
GAME_FN(void, __cdecl, MB_ShowGrades, 0x007052B0, int cls, int grade, const char* text);
GAME_FN(void, __cdecl, MB_LoadCrest, 0x00705340);
GAME_FN(void, __cdecl, MB_AddCompletionMsg, 0x007054B0, const char* text, int value);
GAME_FN(int32_t, __cdecl, MB_IsShowingCompletion, 0x00705500);
GAME_FN(uint8_t, __cdecl, MB_IsHighScore, 0x00705570, int table, uint32_t score);
GAME_FN(void, __cdecl, MB_GetHighScore, 0x007055A0, int table, int slot, int32_t* score, re::Ptr32<uint16_t>* name);
GAME_FN(int32_t, __cdecl, MB_HasTopScore, 0x007055E0, int table, const uint16_t* name);
GAME_FN(void, __cdecl, MB_SetCompletion, 0x00705AE0, const char* title, const char* subtitle, int fading,
        float money, const char* extra, const uint8_t* colour, int32_t p7);
GAME_FN(int32_t, __cdecl, MB_SetHighScore, 0x00705CE0, int table, uint32_t score, const uint16_t* name);
GAME_FN(void, __cdecl, MB_SetCompletion2, 0x007074E0, const char* title, int fading, float money, const char* extra,
        const uint8_t* colour, int32_t p6);

inline uint8_t* PopupOf(uint8_t* mg) {
    return mg + 0x50;
}
// a sound item stopped (+0x5C: done) and forgotten
inline void StopSoundItem(uint8_t* mg, int offset) {
    re::Ptr32<uint8_t>& item = At<uint8_t*>(mg, offset);
    if (item != nullptr) {
        item[0x5C] = 1;
        item = nullptr;
    }
}
inline void* PlayNamed(const char* name) {
    int32_t id = MB_Sounds_GetId(mbSounds, name);
    return MB_Sounds_Play(mbSounds, id, nullptr, 1, 0, 0, 0x78, 0, 0, 0, 0, 6, 1, 1.0f, 0);
}

// ---- the natives' helpers and the base ---------------------------------------------------------

// 0x704810: MGLockSetCombo(a, b, c): the lock's three numbers
int __cdecl Native_MGLockSetCombo(lua_State* L) {
    uint8_t* mg = mbCurrent;
    int c = Script::GetInt(L, 2);
    int b = Script::GetInt(L, 1);
    int a = Script::GetInt(L, 0);
    At<int32_t>(mg, 0xC8) = a;
    At<int32_t>(mg, 0xCC) = b;
    At<int32_t>(mg, 0xD0) = c;
    return 0;
}

// 0x704860 (cdecl): the clock moved on by (hours, minutes) (the bytes wrap)
void __cdecl MinigameAddGameTime(int hours, int minutes) {
    uint8_t h = uint8_t(reinterpret_cast<uint8_t*>(0x00BCEB4B)[0] + uint8_t(hours));
    uint8_t m = uint8_t(reinterpret_cast<uint8_t*>(0x00BCEB4A)[0] + uint8_t(minutes));
    MB_SetGameClock(h, m);
}

// 0x704890 (cdecl): creates the minigame of a kind (1..18; 0 for others); a failed
// allocation leaves none (but still says yes)
uint8_t __cdecl MinigameCreateKind(int kind) {
    struct Kind {
        uint32_t size;
        void*(__thiscall* construct)(void*);
    };
    static const Kind kKinds[18] = {
            {0xD60, MB_ClassEnglish_Construct},   {0xACC, MB_ClassMath_Construct},
            {0x830, MB_ClassGeography_Construct}, {0x1544, MB_ClassBiology_Construct},
            {0xD1C, MB_ClassChem_Construct},      {0x658, MB_ClassArt_Construct},
            {0x4C0, MB_Striker_Construct},        {0xC8, MB_BallToss_Construct},
            {0x3E0, MB_Darts_Construct},          {0x18C, MB_Lock_Construct},
            {0x15588, MB_LawnMowing_Construct},   {0xAD0, MB_SoccerP_Construct},
            {0xF0, MB_DunkTank_Construct},        {0x510, MB_Race_Construct},
            {0x5E0, MB_Arcade_Construct},         {0x178, MB_Shooting_Construct},
            {0x808, MB_KeepUps_Construct},        {0x105A8, MB_ClassMusic_Construct},
    };
    if (uint32_t(kind - 1) > 0x11) {
        return 0;
    }
    void* memory = MB_Malloc(kKinds[kind - 1].size);
    if (memory != nullptr) {
        mbCurrent = static_cast<uint8_t*>(kKinds[kind - 1].construct(memory));
    } else {
        mbCurrent = nullptr;
    }
    mbPunishment[0] = 1;
    mbPunishment[1] = 1;
    return 1;
}

// 0x704C60 (cdecl): the minigame ended and deleted; the world's population and traffic
// back when asked
void __cdecl MinigameDestroy(int restoreWorld) {
    uint8_t* mg = mbCurrent;
    if (mg != nullptr) {
        if (mg[4] != 0) {
            vc::Call<void>(mg, 0x14);
        }
        if (mbCurrent != nullptr) {
            vc::Call<void*>(mbCurrent, 0x0, 1);
        }
        mbCurrent = nullptr;
    }
    if (re::Flag(restoreWorld)) {
        MB_PopulationRevert(mbPopulation);
        MB_TrafficRevert(mbTraffic);
        mbSystem[0xA4] = 1;
        mbPopulation[0x7898] |= 0x10;
        MB_PopulationClearDisabled(mbPopulation);
    }
    mbHoldCompletionC3CF82 = 0;
    mbPunishment[0] = 0;
    mbPunishment[1] = 0;
    MB_Objectives_SetReminderTime(mbObjectives, -1.0f);
}

// 0x704D00 (cdecl): a minigame index's four effect slots killed
void __cdecl MinigameKillEffects(int index) {
    uint8_t* proxy = mbEffects + index * 0x20;
    for (int i = 0; i < 4; i++) {
        MB_EffectProxy_Kill(proxy);
        proxy += 8;
    }
}

// 0x704D30: the texture set from a txd (entries: index, name, mask); 0 when any is missing
uint8_t __fastcall MinigameLoadTextures(uint8_t* mg, re::Edx, int count, const school::ClassTextureEntry* entries,
                                        const char* txd) {
    uint8_t ok = 1;
    int slot = MB_FindTxdSlot(txd, 0);
    At<int32_t>(mg, 0x18) = slot;
    MB_RequestModel(slot + 0x2FA8, 9);
    MB_LoadAllRequestedModels(1);
    uint64_t bytes = uint64_t(uint32_t(count)) * 4;
    uint32_t size = (bytes >> 32) != 0 ? 0xFFFFFFFFu : uint32_t(bytes);
    At<re::Ptr32<void>*>(mg, 0x1C) = static_cast<re::Ptr32<void>*>(MB_Malloc(size));
    At<int32_t>(mg, 0x14) = count;
    MB_PushCurrentTxd();
    MB_SetCurrentTxd(At<int32_t>(mg, 0x18));
    for (int i = 0; i < count; i++) {
        At<re::Ptr32<void>*>(mg, 0x1C)[entries[i].id] = MB_FindTexture(entries[i].name, entries[i].mask);
        if (At<re::Ptr32<void>*>(mg, 0x1C)[entries[i].id] == nullptr) {
            ok = 0;
        }
    }
    MB_PopCurrentTxd();
    return ok;
}

// 0x704DE0: the texture set released (and its txd)
void __fastcall MinigameUnloadTextures(uint8_t* mg, re::Edx) {
    if (At<re::Ptr32<void>*>(mg, 0x1C) == nullptr) {
        return;
    }
    for (int i = 0; i < At<int32_t>(mg, 0x14); i++) {
        MB_NiRelease(At<re::Ptr32<void>*>(mg, 0x1C)[i]);
    }
    MB_Free(At<re::Ptr32<void>*>(mg, 0x1C));
    MB_RemoveModel(At<int32_t>(mg, 0x18) + 0x2FA8);
    At<re::Ptr32<void>*>(mg, 0x1C) = nullptr;
    At<int32_t>(mg, 0x14) = 0;
    At<int32_t>(mg, 0x18) = -1;
}

// 0x704E40 (ecx unused): a texture set released and its txd removed
void __fastcall MinigameUnloadTextureSet(void*, re::Edx, re::Ptr32<void>* textures, int count, int txdSlot) {
    if (textures == nullptr) {
        return;
    }
    for (int i = 0; i < count; i++) {
        MB_NiRelease(textures[i]);
    }
    MB_Free(textures);
    MB_RemoveModel(txdSlot + 0x2FA8);
}

// 0x704E90: the sound set from MINIGAME\<bank>.bnk (entries: index, name); 0 without memory
uint8_t __fastcall MinigameLoadSounds(uint8_t* mg, re::Edx, int count, const school::ClassSoundEntry* entries,
                                      const char* bank) {
    char path[0x100];
    std::memcpy(path, GameString(0x009481E8), 10);   // "MINIGAME\"
    MB_Memset(path + 10, 0, 0xF6);
    std::strcat(path, bank);
    std::strcat(path, GameString(0x009481E0));   // ".bnk"
    int32_t id = MB_SoundBanks_Find(mbSoundBanks, path);
    At<int32_t>(mg, 0x48) = id;
    MB_SoundBanks_Load(mbSoundBanks, id, 1, 1);
    uint64_t bytes = uint64_t(uint32_t(count)) * 4;
    uint32_t size = (bytes >> 32) != 0 ? 0xFFFFFFFFu : uint32_t(bytes);
    int32_t* ids = static_cast<int32_t*>(MB_Malloc(size));
    At<int32_t*>(mg, 0x4C) = ids;
    if (ids == nullptr) {
        return 0;
    }
    At<int32_t>(mg, 0x44) = count;
    for (int i = 0; i < count; i++) {
        int32_t sound = MB_Sounds_GetId(mbSounds, entries[i].name);
        At<int32_t*>(mg, 0x4C)[entries[i].id] = sound;
    }
    return 1;
}

// 0x704FD0: the sounds playing stopped and the sound set unloaded
void __fastcall MinigameEndSounds(uint8_t* mg, re::Edx) {
    StopSoundItem(mg, 0xB0);
    StopSoundItem(mg, 0xB4);
    StopSoundItem(mg, 0xB8);
    StopSoundItem(mg, 0xBC);
    StopSoundItem(mg, 0xC0);
    if (At<int32_t*>(mg, 0x4C) != nullptr) {
        MB_SoundBanks_Unload(mbSoundBanks, At<int32_t>(mg, 0x48), 1, 1);
        MB_Free(At<int32_t*>(mg, 0x4C));
        At<int32_t*>(mg, 0x4C) = nullptr;
        At<int32_t>(mg, 0x44) = 0;
        At<int32_t>(mg, 0x48) = 0;
    }
}

// 0x705070: the popup hidden
uint8_t* __fastcall PopupReset(uint8_t* popup, re::Edx) {
    popup[0x50] = 0;
    popup[0x51] = 0;
    return popup;
}

// the popup closes: one fewer pause, unpaused when it was the last
void PopupClose(uint8_t* popup) {
    popup[0x50] = 0;
    mbUserStartResponse -= 1;
    if (mbUserStartResponse == 0) {
        MB_Unpause_45B820();
        MB_Fn45B960(0);
        mbSoundsPaused = 0;
    }
}

// 0x705080: the popup's tick: its result (1 cancel / no, 2 yes) once chosen, else 0; the
// stick and d-pad move the choice
int __fastcall PopupResult(uint8_t* popup, re::Edx) {
    if (mbPauseA == 1 || mbPauseB == 1 || mbPauseC != 0 || mbPauseD != 0) {
        return 0;
    }
    if (popup[0x50] == 0) {
        if (popup[0x51] == 0) {
            return 0;
        }
        popup[0x51] = 0;
        return At<int32_t>(popup, 0x54);
    }
    int32_t cancel = At<int32_t>(popup, 8);
    if (cancel != -1 && MB_Pad_JustDown(cancel, 0) != 0) {
        MB_FrontendSound(mbSounds, 0xC);
        MB_PadsUpdate();
        popup[0x50] = 0;
        At<int32_t>(popup, 0x54) = 1;
        mbUserStartResponse -= 1;
        if (mbUserStartResponse == 0) {
            MB_Unpause_45B820();
            MB_Fn45B960(0);
            mbSoundsPaused = 0;
        }
        return 1;
    }
    int32_t accept = At<int32_t>(popup, 4);
    if (accept != -1 && MB_Pad_JustDown(accept, 0) != 0) {
        MB_FrontendSound(mbSounds, 0xD);
        MB_PadsUpdate();
        PopupClose(popup);
        if (At<int32_t>(popup, 0xC) == 2) {
            At<int32_t>(popup, 0x54) = 2;
            return 2;
        }
        At<int32_t>(popup, 0x54) = 1;
        return At<int32_t>(popup, 0x54);
    }
    if (At<int32_t>(popup, 8) == -1) {
        return 0;
    }
    if ((MB_Pad_JustDown(2, 0) != 0 || MB_Pad_GetValue(0x11, 0) > 0.5f) && At<int32_t>(popup, 0xC) != 2) {
        MB_FrontendSound(mbSounds, 0xC);
        At<int32_t>(popup, 0xC) = 2;
        return 0;
    }
    if (MB_Pad_JustDown(3, 0) != 0 || MB_Pad_GetValue(0x11, 0) < -0.5f) {
        if (At<int32_t>(popup, 0xC) != 1) {
            MB_FrontendSound(mbSounds, 0xD);
            At<int32_t>(popup, 0xC) = 1;
        }
    }
    return 0;
}

// 0x705220: shows the popup (the game pauses; the choice starts on "yes" when there's no
// cancel button)
void __fastcall PopupShow(uint8_t* popup, re::Edx, int kind, const char* title, const char* footer, int accept,
                          int cancel) {
    MB_Pad_StopRumble();
    MB_Pause_45B810();
    if (popup[0x50] == 0) {
        mbUserStartResponse += 1;
    }
    MB_FrontendSound(mbSounds, 0xD);
    MB_Strncpy(reinterpret_cast<char*>(popup + 0x10), title, 0x20);
    MB_Strncpy(reinterpret_cast<char*>(popup + 0x30), footer, 0x20);
    At<int32_t>(popup, 0) = kind;
    popup[0x50] = 1;
    At<int32_t>(popup, 4) = accept;
    At<int32_t>(popup, 8) = cancel;
    At<int32_t>(popup, 0xC) = (cancel == -1 ? 1 : 0) + 1;
    MB_PadsUpdate();
    MB_Camera_Fade(mbCamera, 0.0f, 0, 0);
}

// 0x7052B0 (cdecl): the grades screen for a class (its result recorded)
void __cdecl MinigameShowGrades(int cls, int grade, const char* text) {
    mbGradesClass = cls;
    mbGradesGrade = grade;
    if (text != nullptr) {
        MB_Strncpy(mbGradesText, text, 0x20);
    } else {
        mbGradesText[0] = 0;
    }
    mbShowingGrades = 1;
    MB_PlayerInfo_SetClassPassed(mbPlayerInfo, cls, grade >= mbGradeNeeded[cls] ? 1 : 0);
    mbGradesTime = 2.0f;
    MB_Objectives_6AE260(mbObjectives);
    MB_HUDText_SetText(mbHudText, 1, 0);
    vc::Call<void>(mbHud, 0x1C);
}

// 0x705340 (cdecl): the crest texture, once
void __cdecl MinigameLoadCrest() {
    if (mbCrest != 0) {
        return;
    }
    mbCrestTxd = MB_FindTxdSlot(GameString(0x00948EA0), 0);   // crest
    MB_RequestModel(mbCrestTxd + 0x2FA8, 9);
    MB_LoadAllRequestedModels(1);
    MB_PushCurrentTxd();
    MB_SetCurrentTxd(mbCrestTxd);
    mbCrest = int32_t(re::Addr32(MB_FindTexture(GameString(0x00948EA0), GameString(0x00948E98))));
    MB_PopCurrentTxd();
}

// 0x7053A0 (cdecl): an effect started in a minigame index's next slot (of 4) at a screen
// position (centred coordinates); its proxy
uint8_t* __cdecl MinigameStartEffect(const char* name, const float* pos, int index) {
    int next = mbEffectNext[index];
    MB_EffectProxy_Kill(mbEffects + (next + index * 4) * 8);
    next = mbEffectNext[index];
    uint8_t* proxy = mbEffects + (next + index * 4) * 8;
    mbEffectNext[index] = next + 1;
    if (next + 1 == 4) {
        mbEffectNext[index] = 0;
    }
    uint32_t id = MB_EffectLibrary_Find(mbEffectLibrary, name);
    MB_EffectManager_Start(mbEffectManager, &id, proxy, 1);
    float at[3] = {float(double(pos[0]) + 320.0), float(double(pos[1]) + 240.0), mbEffectZ};
    MB_EffectProxy_SetPosition(proxy, at, 1);
    return proxy;
}

// 0x705470 (cdecl): an effect moved to a screen position
void __cdecl MinigameMoveEffect(void* proxy, const float* pos) {
    float at[3] = {float(double(pos[0]) + 320.0), float(double(pos[1]) + 240.0), mbEffectZ};
    MB_EffectProxy_SetPosition(proxy, at, 1);
}

// 0x7054B0 (cdecl): a line on the completion screen (five at most)
void __cdecl MinigameAddCompletionMsg(const char* text, int value) {
    if (text == nullptr || text[0] == 0 || mbCompletionMsgCount >= 5) {
        return;
    }
    MB_Strncpy(mbCompletionMsgs + mbCompletionMsgCount * 0x20, text, 0x20);
    mbCompletionMsgTime = 2.0f;
    mbCompletionMsgValues[mbCompletionMsgCount] = value;
    mbCompletionMsgCount += 1;
}

// 0x705500 (cdecl): the completion screen is up
int32_t __cdecl MinigameIsShowingCompletion() {
    return 0.0f < mbCompletionTime ? 1 : 0;
}

// 0x705520 (cdecl): the completion screen is fading (in its last 2 seconds, 0.5 when it
// fades fast). A byte bool: its time-0 path sets al only (ah: the fnstsw status), and its
// one caller, 0x709120, pushes eax to 0x5D85B0, which reads the low byte
uint8_t __cdecl MinigameIsFadingCompletion() {
    float time = mbCompletionTime;
    if (time == 0.0f) {
        return 0;
    }
    float limit = mbCompletionFading != 0 ? 0.5f : 2.0f;
    return time <= limit ? 1 : 0;
}

// 0x705570 (cdecl): a score makes a table (unsigned)
uint8_t __cdecl MinigameIsHighScore(int table, uint32_t score) {
    for (int i = 0; i < 5; i++) {
        if (score >= mbHighScores[table * 5 + i]) {
            return 1;
        }
    }
    return 0;
}

// 0x7055A0 (cdecl): a table entry's score and name
void __cdecl MinigameGetHighScore(int table, int slot, int32_t* score, re::Ptr32<uint16_t>* name) {
    if (score != nullptr) {
        *score = int32_t(mbHighScores[table * 5 + slot]);
    }
    if (name != nullptr) {
        *name = mbHighNames + (table * 5 + slot) * 4;
    }
}

// 0x7055E0 (cdecl): the table's top name is this one (three letters)
int32_t __cdecl MinigameHasTopScore(int table, const uint16_t* name) {
    return MB_WStrNCmp(mbHighNames + table * 5 * 4, name, 3) == 0 ? 1 : 0;
}

// 0x705610 (cdecl): the tables into the save block
void __cdecl MinigameSaveHighScores(int32_t* data) {
    data[0] = 1;
    data[1] = 0x3EC;
    data[2] = 10;
    data[3] = 5;
    data[4] = 3;
    uint16_t* names = reinterpret_cast<uint16_t*>(data + 5);
    for (int i = 0; i < 50; i++) {
        MB_WStrNCopy(names + i * 4, mbHighNames + i * 4, 4);
        data[0x69 + i] = int32_t(mbHighScores[i]);
    }
}

// 0x705690 (cdecl): the tables from the save block, when its header matches
void __cdecl MinigameLoadHighScores(int32_t* data) {
    if (MB_Save_CheckValue(0, 0, data[0], 1) == 0 || MB_Save_CheckValue(0, 0, data[1], 0x3EC) == 0 ||
        MB_Save_CheckValue(0, 0, data[2], 10) == 0 || MB_Save_CheckValue(0, 0, data[3], 5) == 0 ||
        MB_Save_CheckValue(0, 0, data[4], 3) == 0 ||
        MB_Save_CheckBlock(GameString(0x00948EA8), data + 0x9B, 0x60) == 0) {   // MinigameSaveData
        return;
    }
    const uint16_t* names = reinterpret_cast<const uint16_t*>(data + 5);
    for (int i = 0; i < 50; i++) {
        MB_WStrNCopy(mbHighNames + i * 4, names + i * 4, 4);
        mbHighScores[i] = uint32_t(data[0x69 + i]);
    }
}

// 0x705790: success (the warning loop stopped)
void __fastcall MinigameSucceed(uint8_t* mg, re::Edx) {
    mg[6] = 1;
    mg[4] = 0;
    StopSoundItem(mg, 0xB0);
}

// 0x7057B0: the info box's textures (MG_Dialog: 4:3 and widescreen)
void __fastcall MinigameLoadInfoTextures(uint8_t* mg, re::Edx) {
    const school::ClassTextureEntry entries[2] = {
            {0, GameString(0x00948ECC), nullptr},   // info_screen
            {1, GameString(0x00948EBC), nullptr},   // info_screen_w
    };
    int slot = MB_FindTxdSlot(GameString(0x00948ED8), 0);   // MG_Dialog
    MB_RequestModel(slot + 0x2FA8, 9);
    MB_LoadAllRequestedModels(1);
    MB_PushCurrentTxd();
    MB_SetCurrentTxd(slot);
    for (int i = 0; i < 2; i++) {
        At<void*>(mg, 0xC + i * 4) = MB_FindTexture(entries[i].name, entries[i].mask);
    }
    MB_PopCurrentTxd();
}

// 0x705850: the info box's textures released
void __fastcall MinigameUnloadInfoTextures(uint8_t* mg, re::Edx) {
    int slot = MB_FindTxdSlot(GameString(0x00948ED8), 0);
    for (int i = 0; i < 2; i++) {
        MB_NiRelease(At<void*>(mg, 0xC + i * 4));
    }
    MB_RemoveModel(slot + 0x2FA8);
}

// 0x7058A0: the timer's sounds as it runs down: the bell at a second left, the warning
// loop and the warning at their times (+1 second)
void __fastcall MinigameTimerSounds(uint8_t* mg, re::Edx, float time) {
    if (mg[0x30] == 0 && !(1.0f < time)) {
        mg[0x30] = 1;
        StopSoundItem(mg, 0xB0);
        At<void*>(mg, 0xB8) = PlayNamed(GameString(0x00948F00));   // TimerBell
    }
    if (mg[0x28] == 0 && !(At<float>(mg, 0x2C) + 1.0f < time)) {
        mg[0x28] = 1;
        At<void*>(mg, 0xB0) = PlayNamed(GameString(0x00948EF0));   // TimerWarnLoop
    }
    if (!(At<float>(mg, 0x24) + 1.0f < time) && mg[0x20] == 0) {
        At<void*>(mg, 0xB4) = PlayNamed(GameString(0x00948EE4));   // TimerWarn
        mg[0x20] = 1;
    }
}

// 0x7059D0: the constructor
uint8_t* __fastcall MinigameConstruct(uint8_t* mg, re::Edx) {
    At<uint32_t>(mg, 0) = 0x00948F10;
    mg[0xA0] = 0;
    mg[0xA1] = 0;
    At<int32_t>(mg, 0x38) = 0;
    At<int32_t>(mg, 0x3C) = 0;
    mg[4] = 0;
    mg[5] = 0;
    mg[6] = 0;
    At<int32_t>(mg, 0x14) = 0;
    At<int32_t>(mg, 0x18) = -1;
    At<int32_t>(mg, 0x1C) = 0;
    At<int32_t>(mg, 0x44) = 0;
    At<int32_t>(mg, 0x48) = 0;
    At<int32_t>(mg, 0x4C) = 0;
    At<int32_t>(mg, 0x40) = 0;
    mg[7] = 0;
    mg[8] = 0;
    mbEffectNext[0] = 0;
    mbEffectNext[1] = 0;
    MB_LoadInfoTextures(mg);
    At<int32_t>(mg, 0xA8) = At<int32_t>(mg, 0xC);
    At<int32_t>(mg, 0xAC) = At<int32_t>(mg, 0x10);
    mg[0x20] = 0;
    mg[0x28] = 0;
    mg[0x29] = 0;
    mg[0x30] = 0;
    return mg;
}

// the destructor's body (shared by 0x705A50 and 0x705FB0)
void MinigameDestructBody(uint8_t* mg) {
    At<uint32_t>(mg, 0) = 0x00948F10;
    MB_UnloadInfoTextures(mg);
    MB_UnloadTextures(mg);
    MB_EndSounds(mg);
    for (uint8_t* proxy = mbEffects; proxy < mbEffects + 0x20; proxy += 8) {
        MB_EffectProxy_Kill(proxy);
    }
}

// 0x705A50: the destructor
void __fastcall MinigameDestruct(uint8_t* mg, re::Edx) {
    MinigameDestructBody(mg);
}

// 0x705A90 (cdecl): the screens' state and the high score tables reset
void __cdecl MinigameResetAll() {
    mbShowingGrades = 0;
    if (mbCrest == 0) {
        mbUberCompletion = 0;
        mbCompletionTime = 0.0f;
        mbHoldCompletion = 0;
    }
    std::memcpy(mbHighNames, mbDefaultNames, 100 * 4);
    std::memcpy(mbHighScores, mbDefaultScores, 50 * 4);
}

// 0x705AE0 (cdecl): the completion screen: its title, subtitle, money (added to the
// player's), extra line and colour (the gold of a fading one by default)
void __cdecl MinigameSetCompletion(const char* title, const char* subtitle, int fading, float money, const char* extra,
                                   const uint8_t* colour, int32_t p7) {
    if (title != nullptr && title[0] != 0) {
        MB_Strncpy(mbCompletionTitle, title, 0x20);
    } else {
        mbCompletionTitle[0] = 0;
    }
    if (subtitle != nullptr && subtitle[0] != 0) {
        MB_Strncpy(mbCompletionSubtitle, subtitle, 0x20);
    } else {
        mbCompletionSubtitle[0] = 0;
    }
    uint8_t fade = uint8_t(fading);
    mbCompletionFading = fade;
    mbCompletionColoured = 0;
    mbHoldCompletion = 0;
    mbUberCompletion = 0;
    mbCompletion87A9 = 0;
    if (colour != nullptr) {
        mbCompletionColour[0] = colour[0];
        mbCompletionColour[1] = colour[1];
        mbCompletionColour[2] = colour[2];
        mbCompletionColour[3] = colour[3];
        mbCompletionColoured = 1;
    } else {
        if (fade != 0) {
            uint8_t gold[4];
            uint8_t* set = MB_RGBA_Set(gold, 0xA6, 0x93, 0x3F, 0xD2);
            mbCompletionColour[0] = set[0];
            mbCompletionColour[1] = set[1];
            mbCompletionColour[2] = set[2];
            mbCompletionColour[3] = set[3];
        } else {
            mbCompletionColour[0] = mbCompletionDefaultColour[0];
            mbCompletionColour[1] = mbCompletionDefaultColour[1];
            mbCompletionColour[2] = mbCompletionDefaultColour[2];
            mbCompletionColour[3] = mbCompletionDefaultColour[3];
        }
        if (mbCompletionColoured == 0 && mbCompletionFading == 0) {
            mbWidescreenC3CD0C = 1;
            mbC3CD14 = 0.0f;
        }
    }
    mbCompletionMoney = money;
    mbCompletion87A4 = p7;
    if (money != 0.0f) {
        uint8_t* ped = mbPlayerPed;
        int total = int(float(double(money) + double(At<int32_t>(ped, 0x1CA0))));   // (fiadd, __ftol2)
        if (total < 0) {
            total = 0;
        }
        MB_Ped_SetMoney(ped, total);
    }
    mbCompletionMsgTime = 0.0f;
    mbCompletionMsgCount = 0;
    if (extra != nullptr && extra[0] != 0) {
        MB_Strncpy(mbCompletionExtra, extra, 0x20);
    } else {
        mbCompletionExtra[0] = 0;
    }
    mbCompletionTime = 2.0f;
    if (mbCompletionColoured == 0) {
        MB_Objectives_6AE260(mbObjectives);
        MB_HUDText_SetText(mbHudText, 1, 0);
    }
    vc::Call<void>(mbHud, 0x1C);
    mbErrandDone = 0;
}

// 0x705CE0 (cdecl): a score into a table: its place (-1: none). Jim's arcade scores
// count for the stats, and his name on top of all four arcade tables is an achievement
int32_t __cdecl MinigameSetHighScore(int table, uint32_t score, const uint16_t* name) {
    uint16_t jim[16];   // (FormatText writes the whole text: it has no size)
    MB_Font_Format(jim, MB_Text_Get(mbTheText, GameString(0x00941B98)), 0xF);   // ARCADE_JIM
    jim[3] = 0;
    if (MB_WStrNCmp(name, jim, 3) == 0 && uint32_t(table) <= 5) {
        int stat = -1;
        if (table == 0 || table == 5) {
            stat = 0xDA;
        } else if (table == 1) {
            stat = 0xDD;
        } else if (table == 2) {
            stat = 0xE0;
        }
        if (stat != -1) {
            double value = double(int32_t(score));
            if (int32_t(score) < 0) {
                value += 4294967296.0;
            }
            float now = float(value);
            float best = float(MB_Stat_GetInt(stat));
            float top = best < now ? now : best;
            MB_Stat_SetInt(stat, int(top));   // (__ftol2)
        }
    }
    for (int slot = 0; slot < 5; slot++) {
        if (score < mbHighScores[table * 5 + slot]) {
            continue;
        }
        if (slot < 4) {
            for (int k = 4; k > slot; k--) {
                MB_WStrNCopy(mbHighNames + (table * 5 + k) * 4, mbHighNames + (table * 5 + k - 1) * 4, 4);
                mbHighScores[table * 5 + k] = mbHighScores[table * 5 + k - 1];
            }
        }
        int entry = slot + table * 5;
        MB_WStrNCopy(mbHighNames + entry * 4, name, 4);
        mbHighScores[entry] = score;
        if (MB_PlayerInfo_IsMiniObjectiveComplete(mbPlayerInfo, 0) == 0 && MB_WStrNCmp(name, jim, 3) == 0 &&
            (MB_WStrNCmp(mbHighNames, jim, 3) == 0 || MB_WStrNCmp(mbHighNames + 5 * 5 * 4, jim, 3) == 0) &&
            MB_WStrNCmp(mbHighNames + 1 * 5 * 4, jim, 3) == 0 && MB_WStrNCmp(mbHighNames + 2 * 5 * 4, jim, 3) == 0) {
            MB_PlayerInfo_SetMiniObjectiveComplete(mbPlayerInfo, 0, 1);
        }
        return slot;
    }
    return -1;
}

// 0x705FB0 (virtual 0): the deleting destructor
uint8_t* __fastcall MinigameDeletingDestructor(uint8_t* mg, re::Edx, int flags) {
    MinigameDestructBody(mg);
    if ((flags & 1) != 0) {
        MB_Free(mg);
    }
    return mg;
}

// 0x7074E0 (cdecl): the completion screen without a subtitle
void __cdecl MinigameSetCompletionShort(const char* title, int fading, float money, const char* extra,
                                        const uint8_t* colour, int32_t p6) {
    MB_SetCompletion(title, nullptr, fading, money, extra, colour, p6);
}

// ---- the natives --------------------------------------------------------------------------------

// MinigameCreate(name [, start]): ENGLISH, MATH, GEOGRAPHY, BIOLOGY, CHEM, ART, STRIKER,
// BALLTOSS, DARTS, LOCK, LAWNMOWING, SOCCERP, DUNKTANK, RACE, ARCADE, SHOOT, KEEPUPS, MUSIC
int __cdecl Native_MinigameCreate(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    uint8_t start = Script::GetBool(L, 1);
    static const uintptr_t kNames[18] = {
            0x009208B4, 0x0094936C, 0x00949360, 0x00949358, 0x00920C88, 0x00921094, 0x00949350, 0x00949344, 0x0094933C,
            0x0092E490, 0x00949330, 0x00949328, 0x0094931C, 0x0092BFC8, 0x00949314, 0x0094930C, 0x009204CC, 0x00921F44,
    };
    int kind = 0;
    for (int i = 0; i < 18; i++) {
        if (std::strcmp(name, GameString(kNames[i])) == 0) {
            kind = i + 1;
            break;
        }
    }
    MB_Create(kind);
    if (start != 0) {
        uint8_t saved = MB_Fn43AC10();
        vc::Call<void>(mbCurrent, 0x10);
        MB_Fn43BE40(saved);
    }
    return 0;
}

// MinigameDestroy([restoreWorld = true])
int __cdecl Native_MinigameDestroy(lua_State* L) {
    uint8_t* mg = mbCurrent;
    if (mg != nullptr && mg[4] != 0) {
        vc::Call<void>(mg, 0x14);
    }
    MB_Destroy(Script::GetBoolOpt(L, 0, 1));
    return 0;
}

// MinigameStart() (the saved byte passed on with the push's other bytes undefined)
int __cdecl Native_MinigameStart(lua_State*) {
    uint8_t* mg = mbCurrent;
    if (mg != nullptr && mg[4] == 0) {
        uint8_t saved = MB_Fn43AC10();
        vc::Call<void>(mbCurrent, 0x10);
        MB_Fn43BE40(saved);
    }
    return 0;
}

// MinigameSetGrades(class, grade [, text])
int __cdecl Native_MinigameSetGrades(lua_State* L) {
    const char* text = nullptr;
    if (Script::GetArgCount(L) >= 3) {
        text = Script::GetString(L, 2);
    }
    int grade = Script::GetInt(L, 1);
    int cls = Script::GetInt(L, 0);
    MB_ShowGrades(cls, grade, text);
    return 0;
}

// MinigameSetCompletion(title, fading [, money [, extra]])
int __cdecl Native_MinigameSetCompletion(lua_State* L) {
    const char* title = Script::GetString(L, 0);
    uint8_t fading = Script::GetBool(L, 1);
    int money = 0;
    const char* extra = nullptr;
    if (Script::GetArgCount(L) >= 3) {
        money = Script::GetInt(L, 2);
        if (Script::GetArgCount(L) >= 4) {
            extra = Script::GetString(L, 3);
        }
    }
    MB_SetCompletion2(title, fading, float(money), extra, nullptr, 0);
    return 0;
}

// MinigameSetChapterCompletion(title, subtitle, fading [, money [, extra]]) (the money
// read when there are three arguments: the fourth is then none)
int __cdecl Native_MinigameSetChapterCompletion(lua_State* L) {
    const char* extra = nullptr;
    const char* title = Script::GetString(L, 0);
    const char* subtitle = Script::GetString(L, 1);
    uint8_t fading = Script::GetBool(L, 2);
    int money = 0;
    if (Script::GetArgCount(L) >= 3) {
        money = Script::GetInt(L, 3);
        if (Script::GetArgCount(L) >= 5) {
            extra = Script::GetString(L, 4);
        }
    }
    MB_SetCompletion(title, subtitle, fading, float(money), extra, nullptr, 0);
    MB_LoadCrest();
    return 0;
}

// MinigameAddCompletionMsg(text [, value])
int __cdecl Native_MinigameAddCompletionMsg(lua_State* L) {
    const char* text = Script::GetString(L, 0);
    int value = 0;
    if (Script::GetArgCount(L) >= 2) {
        value = Script::GetInt(L, 1);
    }
    MB_AddCompletionMsg(text, value);
    return 0;
}

// MinigameSetErrandCompletion(errand, title, fading [, money [, extra [, p]]]): the
// errand counted (and its completions when it went well)
int __cdecl Native_MinigameSetErrandCompletion(lua_State* L) {
    int p = 0;
    int errand = Script::GetInt(L, 0);
    const char* title = Script::GetString(L, 1);
    uint8_t fading = Script::GetBool(L, 2);
    int money = 0;
    const char* extra = nullptr;
    if (Script::GetArgCount(L) >= 4) {
        money = Script::GetInt(L, 3);
        if (Script::GetArgCount(L) >= 5) {
            extra = Script::GetString(L, 4);
            if (Script::GetArgCount(L) >= 6) {
                p = Script::GetInt(L, 5);
            }
        }
    }
    if (fading != 0) {
        MB_Stat_AddInt(0xD8, money);
    }
    MB_SetCompletion2(title, fading, float(money), extra, mbStatsColours + 0x38, p);
    if (mbProgress[0x270] != 0 && mbCrest == 0) {
        mbUberCompletion = 0;
        mbCompletionTime = 0.0f;
        mbHoldCompletion = 0;
    }
    if (errand >= 0) {
        MB_PlayerInfo_SetErrands(mbPlayerInfo, errand, MB_PlayerInfo_GetErrands(mbPlayerInfo, errand) + 1);
        if (fading != 0) {
            MB_PlayerInfo_SetErrandCompletion(mbPlayerInfo, errand,
                                              MB_PlayerInfo_GetErrandCompletion(mbPlayerInfo, errand) + 1);
        }
        mbErrandDone = 1;
    }
    return 0;
}

// MinigameIsShowingCompletion() (not the coloured kind)
int __cdecl Native_MinigameIsShowingCompletion(lua_State* L) {
    if (uint8_t(MB_IsShowingCompletion()) != 0 && mbCompletionColoured == 0) {
        Script::PushBool(L, 1);
        return 1;
    }
    Script::PushBool(L, 0);
    return 1;
}

// MinigameIsHighScore(table, score)
int __cdecl Native_MinigameIsHighScore(lua_State* L) {
    uint32_t score = uint32_t(Script::GetInt(L, 1));
    int table = Script::GetInt(L, 0);
    Script::PushBool(L, MB_IsHighScore(table, score));
    return 1;
}

// MinigameSetHighScoreFromID(table, score, textKey): the place
int __cdecl Native_MinigameSetHighScoreFromID(lua_State* L) {
    uint16_t name[4];
    MB_Font_Format(name, MB_Text_Get(mbTheText, Script::GetString(L, 2)), 4);
    name[3] = 0;
    uint32_t score = uint32_t(Script::GetInt(L, 1));
    int table = Script::GetInt(L, 0);
    Script::PushInt(L, MB_SetHighScore(table, score, name));
    return 1;
}

// MinigameSetHighScoreFromASCII(table, score, name): the place
int __cdecl Native_MinigameSetHighScoreFromASCII(lua_State* L) {
    uint16_t name[4];
    const char* text = Script::GetString(L, 2);
    int n = 0;
    while (text[n] != 0 && n < 3) {
        name[n] = uint16_t(int16_t(int8_t(text[n])));
        n += 1;
    }
    name[n] = 0;
    uint32_t score = uint32_t(Script::GetInt(L, 1));
    int table = Script::GetInt(L, 0);
    Script::PushInt(L, MB_SetHighScore(table, score, name));
    return 1;
}

// MinigameGetHighScore(table, slot): the score
int __cdecl Native_MinigameGetHighScore(lua_State* L) {
    int32_t score;
    re::Ptr32<uint16_t> name;
    int slot = Script::GetInt(L, 1);
    int table = Script::GetInt(L, 0);
    MB_GetHighScore(table, slot, &score, &name);
    Script::PushInt(L, score);
    return 1;
}

// MinigameHasTopScore(table, textKey)
int __cdecl Native_MinigameHasTopScore(lua_State* L) {
    uint16_t name[4];
    MB_Font_Format(name, MB_Text_Get(mbTheText, Script::GetString(L, 1)), 3);
    name[3] = 0;
    int table = Script::GetInt(L, 0);
    Script::PushBool(L, uint8_t(MB_HasTopScore(table, name)));
    return 1;
}

// MinigameSetAnnouncement(text, fading): the completion screen in gold, no money
int __cdecl Native_MinigameSetAnnouncement(lua_State* L) {
    const char* text = Script::GetString(L, 0);
    uint8_t fading = Script::GetBool(L, 1);
    uint8_t gold[4];
    MB_RGBA_Set(gold, 0xA6, 0x93, 0x3F, 0xD2);
    MB_SetCompletion2(text, fading, 0.0f, nullptr, gold, 0);
    return 0;
}

} // namespace

#define MGBASE44_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

MGBASE44_REVERSED("Native_MGLockSetCombo", 0x00704810, &Native_MGLockSetCombo);
MGBASE44_REVERSED("Minigame::AddGameTime_704860", 0x00704860, &MinigameAddGameTime);
MGBASE44_REVERSED("Minigame::Create_704890", 0x00704890, &MinigameCreateKind);
MGBASE44_REVERSED("Minigame::Destroy_704C60", 0x00704C60, &MinigameDestroy);
MGBASE44_REVERSED("Minigame::KillEffects_704D00", 0x00704D00, &MinigameKillEffects);
MGBASE44_REVERSED("Minigame::LoadTextures_704D30", 0x00704D30, &MinigameLoadTextures);
MGBASE44_REVERSED("Minigame::UnloadTextures_704DE0", 0x00704DE0, &MinigameUnloadTextures);
MGBASE44_REVERSED("Minigame::UnloadTextureSet_704E40", 0x00704E40, &MinigameUnloadTextureSet);
MGBASE44_REVERSED("Minigame::LoadSounds_704E90", 0x00704E90, &MinigameLoadSounds);
MGBASE44_REVERSED("Minigame::EndSounds_704FD0", 0x00704FD0, &MinigameEndSounds);
MGBASE44_REVERSED("MinigamePopup::Reset_705070", 0x00705070, &PopupReset);
MGBASE44_REVERSED("MinigamePopup::Result_705080", 0x00705080, &PopupResult);
MGBASE44_REVERSED("MinigamePopup::Show_705220", 0x00705220, &PopupShow);
MGBASE44_REVERSED("Minigame::ShowGrades_7052B0", 0x007052B0, &MinigameShowGrades);
MGBASE44_REVERSED("Minigame::LoadCrest_705340", 0x00705340, &MinigameLoadCrest);
MGBASE44_REVERSED("Minigame::StartEffect_7053A0", 0x007053A0, &MinigameStartEffect);
MGBASE44_REVERSED("Minigame::MoveEffect_705470", 0x00705470, &MinigameMoveEffect);
MGBASE44_REVERSED("Minigame::AddCompletionMsg_7054B0", 0x007054B0, &MinigameAddCompletionMsg);
MGBASE44_REVERSED("Minigame::IsShowingCompletion_705500", 0x00705500, &MinigameIsShowingCompletion);
MGBASE44_REVERSED("Minigame::IsFadingCompletion_705520", 0x00705520, &MinigameIsFadingCompletion);
MGBASE44_REVERSED("Minigame::IsHighScore_705570", 0x00705570, &MinigameIsHighScore);
MGBASE44_REVERSED("Minigame::GetHighScore_7055A0", 0x007055A0, &MinigameGetHighScore);
MGBASE44_REVERSED("Minigame::HasTopScore_7055E0", 0x007055E0, &MinigameHasTopScore);
MGBASE44_REVERSED("Minigame::SaveHighScores_705610", 0x00705610, &MinigameSaveHighScores);
MGBASE44_REVERSED("Minigame::LoadHighScores_705690", 0x00705690, &MinigameLoadHighScores);
MGBASE44_REVERSED("Minigame::Succeed_705790", 0x00705790, &MinigameSucceed);
MGBASE44_REVERSED("Minigame::LoadInfoTextures_7057B0", 0x007057B0, &MinigameLoadInfoTextures);
MGBASE44_REVERSED("Minigame::UnloadInfoTextures_705850", 0x00705850, &MinigameUnloadInfoTextures);
MGBASE44_REVERSED("Minigame::TimerSounds_7058A0", 0x007058A0, &MinigameTimerSounds);
MGBASE44_REVERSED("Minigame::Minigame_7059D0", 0x007059D0, &MinigameConstruct);
MGBASE44_REVERSED("Minigame::~Minigame_705A50", 0x00705A50, &MinigameDestruct);
MGBASE44_REVERSED("Minigame::ResetAll_705A90", 0x00705A90, &MinigameResetAll);
MGBASE44_REVERSED("Minigame::SetCompletion_705AE0", 0x00705AE0, &MinigameSetCompletion);
MGBASE44_REVERSED("Minigame::SetHighScore_705CE0", 0x00705CE0, &MinigameSetHighScore);
MGBASE44_REVERSED("Minigame::DeletingDestructor_705FB0", 0x00705FB0, &MinigameDeletingDestructor);
MGBASE44_REVERSED("Minigame::SetCompletionShort_7074E0", 0x007074E0, &MinigameSetCompletionShort);
MGBASE44_REVERSED("Native_MinigameCreate", 0x00708A30, &Native_MinigameCreate);
MGBASE44_REVERSED("Native_MinigameDestroy", 0x00708C30, &Native_MinigameDestroy);
MGBASE44_REVERSED("Native_MinigameStart", 0x00708C70, &Native_MinigameStart);
MGBASE44_REVERSED("Native_MinigameSetGrades", 0x00708DA0, &Native_MinigameSetGrades);
MGBASE44_REVERSED("Native_MinigameSetCompletion", 0x00708E10, &Native_MinigameSetCompletion);
MGBASE44_REVERSED("Native_MinigameSetChapterCompletion", 0x00708EA0, &Native_MinigameSetChapterCompletion);
MGBASE44_REVERSED("Native_MinigameAddCompletionMsg", 0x00708F30, &Native_MinigameAddCompletionMsg);
MGBASE44_REVERSED("Native_MinigameSetErrandCompletion", 0x00708F70, &Native_MinigameSetErrandCompletion);
MGBASE44_REVERSED("Native_MinigameIsShowingCompletion", 0x007090E0, &Native_MinigameIsShowingCompletion);
MGBASE44_REVERSED("Native_MinigameIsHighScore", 0x00709180, &Native_MinigameIsHighScore);
MGBASE44_REVERSED("Native_MinigameSetHighScoreFromID", 0x007091C0, &Native_MinigameSetHighScoreFromID);
MGBASE44_REVERSED("Native_MinigameSetHighScoreFromASCII", 0x00709230, &Native_MinigameSetHighScoreFromASCII);
MGBASE44_REVERSED("Native_MinigameGetHighScore", 0x007092B0, &Native_MinigameGetHighScore);
MGBASE44_REVERSED("Native_MinigameHasTopScore", 0x00709300, &Native_MinigameHasTopScore);
MGBASE44_REVERSED("Native_MinigameSetAnnouncement", 0x00709360, &Native_MinigameSetAnnouncement);
