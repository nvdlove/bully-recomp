// Script natives of the minigames and classes (0x680000-0x71FFFF): most work on the
// running minigame (currentMinigame, whose class depends on which one it is), with
// thiscalls on it or its fields.

#include <cmath>
#include <cstring>

#include "game/NativesC.h"

namespace {

// ---- the minigame in general ---------------------------------------------------------

GAME_VAR(uint8_t, minigameShowingGrades, 0x020C8708);
GAME_VAR(uint8_t, minigameHoldCompletion, 0x020C870B);
GAME_VAR(uint8_t, minigameUberCompletion, 0x020C87A8);
GAME_VAR(uint8_t*, minigameProgress, 0x020CE75C);
GAME_FN(uint8_t, __cdecl, Minigame_IsFadingCompletion, 0x00705520);
GAME_FN(void, __cdecl, Minigame_SetElapsedGameTime, 0x00704860, int, int);
GAME_FN(int, __thiscall, PlayerInfo_GetErrandCompletion, 0x004467E0, void* info, int errand);

// (the minigame's virtual at +0x14, when one runs and +4 is set)
int __cdecl Native_MinigameEnd(lua_State*) {
    uint8_t* mg = currentMinigame;
    if (mg != nullptr && mg[4] != 0) {
        VCall<void>(mg, 0x14);
    }
    return 0;
}

int __cdecl Native_MinigameIsReady(lua_State* L) {
    Script::PushBool(L, currentMinigame != nullptr);
    return 1;
}

int __cdecl Native_MinigameIsActive(lua_State* L) {
    uint8_t* mg = currentMinigame;
    Script::PushBool(L, mg != nullptr && mg[4] != 0);
    return 1;
}

int __cdecl Native_MinigameIsSuccess(lua_State* L) {
    uint8_t* mg = currentMinigame;
    Script::PushBool(L, mg != nullptr && mg[6] != 0);
    return 1;
}

int __cdecl Native_MinigameEnableHUD(lua_State* L) {
    uint8_t enable = Script::GetBool(L, 0);
    if (currentMinigame != nullptr) {
        currentMinigame[5] = enable;
    }
    return 0;
}

int __cdecl Native_MinigameIsShowingGrades(lua_State* L) {
    Script::PushBool(L, minigameShowingGrades);
    return 1;
}

int __cdecl Native_MinigameIsFadingCompletion(lua_State* L) {
    Script::PushBool(L, Minigame_IsFadingCompletion());
    return 1;
}

int __cdecl Native_MinigameHoldCompletion(lua_State*) {
    minigameHoldCompletion = 1;
    return 0;
}

int __cdecl Native_MinigameReleaseCompletion(lua_State*) {
    minigameHoldCompletion = 0;
    return 0;
}

int __cdecl Native_MinigameSetUberCompletion(lua_State*) {
    if (At<uint8_t>(minigameProgress, 0x270) == 0) {
        minigameUberCompletion = 1;
    }
    return 0;
}

int __cdecl Native_MinigameSetElapsedGameTime(lua_State* L) {
    int a = Script::GetInt(L, 0);
    Minigame_SetElapsedGameTime(a, Script::GetInt(L, 1));
    return 0;
}

int __cdecl Native_MinigameGetErrandCompletion(lua_State* L) {
    Script::PushInt(L, PlayerInfo_GetErrandCompletion(playerInfo, Script::GetInt(L, 0)));
    return 1;
}

// ---- dodgeball -----------------------------------------------------------------------

GAME_FN(void, __cdecl, Dodgeball_EndGame, 0x00683B90);
GAME_FN(void, __cdecl, Dodgeball_Init, 0x00683A70);
GAME_FN(void, __cdecl, Dodgeball_Term, 0x00683C00);
GAME_FN(uint8_t, __cdecl, Dodgeball_IsActive, 0x00683A30);
GAME_FN(int, __cdecl, Dodgeball_GetWinner, 0x00683A60);
GAME_FN(void, __cdecl, Dodgeball_EnableCamera, 0x00683AD0, uint8_t);
GAME_FN(void, __cdecl, Dodgeball_EnableHud, 0x00683AA0, uint8_t);
GAME_FN(void, __cdecl, Dodgeball_Pause, 0x00683B20, uint8_t);
GAME_FN(void, __cdecl, Dodgeball_StartGame, 0x006839F0, const char* name, uint8_t);

int __cdecl Native_DodgeballEndGame(lua_State*) {
    Dodgeball_EndGame();
    return 0;
}

int __cdecl Native_DodgeballInit(lua_State*) {
    Dodgeball_Init();
    return 0;
}

int __cdecl Native_DodgeballTerm(lua_State*) {
    Dodgeball_Term();
    return 0;
}

int __cdecl Native_DodgeballIsActive(lua_State* L) {
    Script::PushBool(L, Dodgeball_IsActive());
    return 1;
}

int __cdecl Native_DodgeballGetWinner(lua_State* L) {
    Script::PushInt(L, Dodgeball_GetWinner());
    return 1;
}

int __cdecl Native_DodgeballEnableCamera(lua_State* L) {
    Dodgeball_EnableCamera(Script::GetBool(L, 0));
    return 0;
}

int __cdecl Native_DodgeballEnableHud(lua_State* L) {
    Dodgeball_EnableHud(Script::GetBool(L, 0));
    return 0;
}

int __cdecl Native_DodgeballPause(lua_State* L) {
    Dodgeball_Pause(Script::GetBool(L, 0));
    return 0;
}

int __cdecl Native_DodgeballStartGame(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    Dodgeball_StartGame(name, Script::GetBool(L, 1));
    return 0;
}

// ---- carnival dunk tank ----------------------------------------------------------------

GAME_FN(void, __thiscall, DunkTank_StartAiming, 0x006D8290, void* mg);

int __cdecl Native_CarnivalDunkTankIncCount(lua_State*) {
    At<int32_t>(currentMinigame, 0xE4) += 1;
    return 0;
}

int __cdecl Native_CarnivalDunkTankDecCount(lua_State*) {
    int32_t& count = At<int32_t>(currentMinigame, 0xE4);
    if (count != 0) {
        count = count - 1;
    }
    return 0;
}

int __cdecl Native_CarnivalDunkTankGetCount(lua_State* L) {
    Script::PushInt(L, At<int32_t>(currentMinigame, 0xE4));
    return 1;
}

int __cdecl Native_CarnivalDunkTankSetCount(lua_State* L) {
    uint8_t* mg = currentMinigame;
    int count = Script::GetInt(L, 0);
    if (count >= 0) {
        At<int32_t>(mg, 0xE4) = count;
    }
    return 0;
}

int __cdecl Native_CarnivalDunkTankStartAiming(lua_State*) {
    DunkTank_StartAiming(currentMinigame);
    return 0;
}

// ---- the arcade game (MGCA_) -------------------------------------------------------------

GAME_FN(void, __thiscall, MGCA_SetLocaleStringKey, 0x006D9D30, void* mg, int, const char*);
GAME_FN(void, __thiscall, MGCA_SetTextureBank, 0x006D9D60, void* mg, int);
GAME_FN(void, __thiscall, MGCA_SetCompletionThreshold, 0x006D9D70, void* mg, float);
GAME_FN(void, __thiscall, MGCA_SetPlayerSpeed, 0x006D9D80, void* mg, float);
GAME_FN(void, __thiscall, MGCA_SetPathSpeedupFactor, 0x006D9D90, void* mg, float);
GAME_FN(void, __thiscall, MGCA_SetEnemySpeed, 0x006D9DA0, void* mg, int, float);
GAME_FN(void, __thiscall, MGCA_SetEnemyInitialPeriod, 0x006D9DC0, void* mg, int, float);
GAME_FN(void, __thiscall, MGCA_SetEnemyPeriod, 0x006D9DE0, void* mg, int, float);
GAME_FN(void, __thiscall, MGCA_SetEnemyCompletionSpawnThreshold, 0x006D9E00, void* mg, int, int);
GAME_FN(void, __thiscall, MGCA_SetEnemyInstanceMax, 0x006D9E20, void* mg, int, int);
GAME_FN(void, __thiscall, MGCA_SetEnemyTotalMax, 0x006D9E40, void* mg, int);
GAME_FN(void, __thiscall, MGCA_SetExploderTimeOut, 0x006D9E50, void* mg, float);
GAME_FN(void, __thiscall, MGCA_SetExploderDebrisCount, 0x006D9E60, void* mg, int);
GAME_FN(void, __thiscall, MGCA_SetExploderDebrisSpeed, 0x006D9E70, void* mg, float);
GAME_FN(void, __thiscall, MGCA_SetPowerUpSpeed, 0x006D9E80, void* mg, int, float);
GAME_FN(void, __thiscall, MGCA_SetPowerUpInitialPeriod, 0x006D9EA0, void* mg, int, float);
GAME_FN(void, __thiscall, MGCA_SetPowerUpPeriod, 0x006D9EC0, void* mg, int, float);
GAME_FN(void, __thiscall, MGCA_SetPowerUpInstanceMax, 0x006D9EE0, void* mg, int, int);
GAME_FN(void, __thiscall, MGCA_SetPowerUpTotalMax, 0x006D9F00, void* mg, int);
GAME_FN(void, __thiscall, MGCA_SetPowerUpCompletionSpawnThreshold, 0x006D9F10, void* mg, int, int);
GAME_FN(void, __thiscall, MGCA_SetPowerUpDuration, 0x006D9F30, void* mg, int, float);
GAME_FN(void, __thiscall, MGCA_SetPowerUpSpeedBoostFactor, 0x006D9F50, void* mg, float);
GAME_FN(void, __thiscall, MGCA_SetPowerUpFreezeFactor, 0x006D9F60, void* mg, float);
GAME_FN(void, __thiscall, MGCA_SetPowerUpClearRadius, 0x006D9F70, void* mg, float);
GAME_FN(void, __thiscall, MGCA_SetLevelTimeout, 0x006D9F80, void* mg, float);
GAME_FN(void, __thiscall, MGCA_SetLives, 0x006D9F90, void* mg, int);
GAME_FN(void, __thiscall, MGCA_SetDeckResolution, 0x006D9FA0, void* mg, int);
GAME_FN(int, __thiscall, MGCA_GetFailureCode, 0x006DA000, void* mg);

// The setters' shapes: (int), (float), (int, int), (int, float)
template <void(__thiscall* const& Set)(void*, int)>
int __cdecl SetInt(lua_State* L) {
    int value = Script::GetInt(L, 0);
    Set(currentMinigame, value);
    return 0;
}
template <void(__thiscall* const& Set)(void*, float)>
int __cdecl SetFloat(lua_State* L) {
    float value = Script::GetFloat(L, 0);
    Set(currentMinigame, value);
    return 0;
}
template <void(__thiscall* const& Set)(void*, int, int)>
int __cdecl SetIntInt(lua_State* L) {
    int a = Script::GetInt(L, 0);
    int b = Script::GetInt(L, 1);
    Set(currentMinigame, a, b);
    return 0;
}
template <void(__thiscall* const& Set)(void*, int, float)>
int __cdecl SetIntFloat(lua_State* L) {
    int a = Script::GetInt(L, 0);
    float b = Script::GetFloat(L, 1);
    Set(currentMinigame, a, b);
    return 0;
}

int __cdecl Native_MGCA_SetLocaleStringKey(lua_State* L) {
    int index = Script::GetInt(L, 0);
    const char* key = Script::GetString(L, 1);
    MGCA_SetLocaleStringKey(currentMinigame, index, key);
    return 0;
}

constexpr auto Native_MGCA_SetTextureBank = &SetInt<MGCA_SetTextureBank>;
constexpr auto Native_MGCA_SetCompletionThreshold = &SetFloat<MGCA_SetCompletionThreshold>;
constexpr auto Native_MGCA_SetPlayerSpeed = &SetFloat<MGCA_SetPlayerSpeed>;
constexpr auto Native_MGCA_SetPathSpeedupFactor = &SetFloat<MGCA_SetPathSpeedupFactor>;
constexpr auto Native_MGCA_SetEnemySpeed = &SetIntFloat<MGCA_SetEnemySpeed>;
constexpr auto Native_MGCA_SetEnemyInitialPeriod = &SetIntFloat<MGCA_SetEnemyInitialPeriod>;
constexpr auto Native_MGCA_SetEnemyPeriod = &SetIntFloat<MGCA_SetEnemyPeriod>;
constexpr auto Native_MGCA_SetEnemyCompletionSpawnThreshold = &SetIntInt<MGCA_SetEnemyCompletionSpawnThreshold>;
constexpr auto Native_MGCA_SetEnemyInstanceMax = &SetIntInt<MGCA_SetEnemyInstanceMax>;
constexpr auto Native_MGCA_SetEnemyTotalMax = &SetInt<MGCA_SetEnemyTotalMax>;
constexpr auto Native_MGCA_SetExploderTimeOut = &SetFloat<MGCA_SetExploderTimeOut>;
constexpr auto Native_MGCA_SetExploderDebrisCount = &SetInt<MGCA_SetExploderDebrisCount>;
constexpr auto Native_MGCA_SetExploderDebrisSpeed = &SetFloat<MGCA_SetExploderDebrisSpeed>;
constexpr auto Native_MGCA_SetPowerUpSpeed = &SetIntFloat<MGCA_SetPowerUpSpeed>;
constexpr auto Native_MGCA_SetPowerUpInitialPeriod = &SetIntFloat<MGCA_SetPowerUpInitialPeriod>;
constexpr auto Native_MGCA_SetPowerUpPeriod = &SetIntFloat<MGCA_SetPowerUpPeriod>;
constexpr auto Native_MGCA_SetPowerUpInstanceMax = &SetIntInt<MGCA_SetPowerUpInstanceMax>;
constexpr auto Native_MGCA_SetPowerUpTotalMax = &SetInt<MGCA_SetPowerUpTotalMax>;
constexpr auto Native_MGCA_SetPowerUpCompletionSpawnThreshold = &SetIntInt<MGCA_SetPowerUpCompletionSpawnThreshold>;
constexpr auto Native_MGCA_SetPowerUpDuration = &SetIntFloat<MGCA_SetPowerUpDuration>;
constexpr auto Native_MGCA_SetPowerUpSpeedBoostFactor = &SetFloat<MGCA_SetPowerUpSpeedBoostFactor>;
constexpr auto Native_MGCA_SetPowerUpFreezeFactor = &SetFloat<MGCA_SetPowerUpFreezeFactor>;
constexpr auto Native_MGCA_SetSetPowerUpClearRadius = &SetFloat<MGCA_SetPowerUpClearRadius>;
constexpr auto Native_MGCA_SetLevelTimeout = &SetFloat<MGCA_SetLevelTimeout>;
constexpr auto Native_MGCA_SetLives = &SetInt<MGCA_SetLives>;
constexpr auto Native_MGCA_SetDeckResolution = &SetInt<MGCA_SetDeckResolution>;

int __cdecl Native_MGCA_GetLives(lua_State* L) {
    Script::PushInt(L, At<int32_t>(currentMinigame, 0x454));
    return 1;
}

int __cdecl Native_MGCA_GetFailureCode(lua_State* L) {
    Script::PushInt(L, MGCA_GetFailureCode(currentMinigame));
    return 1;
}

// ---- classes: the shared parts (biology, geography, math share one base) ---------------

GAME_FN(void, __thiscall, Class_Success, 0x00705790, void* mg);
GAME_FN(void, __thiscall, Class_SubtractTime, 0x006E4110, void* mg, float);
GAME_FN(void, __thiscall, Class_SetScoreMsg, 0x006FA280, void* mg, float score, const char* msg);

// (also ClassBiologySuccess)
int __cdecl Native_ClassGeographySuccess(lua_State*) {
    Class_Success(currentMinigame);
    return 0;
}

// (also ClassGeographySetWarnTime and ClassMathSetWarnTime)
int __cdecl Native_ClassBiologySetWarnTime(lua_State* L) {
    uint8_t* mg = currentMinigame;
    At<float>(mg, 0x24) = Script::GetFloat(L, 0);
    return 0;
}

// (also ClassMathSetGameType)
int __cdecl Native_ClassBiologySetGameType(lua_State* L) {
    uint8_t* mg = currentMinigame;
    At<int32_t>(mg, 0x40) = Script::GetInt(L, 0) - 1;
    return 0;
}

// (also ClassGeographySubtractTime and ClassMathSubtractTime)
int __cdecl Native_ClassBiologySubtractTime(lua_State* L) {
    uint8_t* mg = currentMinigame;
    Class_SubtractTime(mg, Script::GetFloat(L, 0));
    return 0;
}

// (also ClassMathSetScoreMsg)
int __cdecl Native_ClassEnglishSetScoreMsg(lua_State* L) {
    float score = float(Script::GetInt(L, 0));
    const char* msg = Script::GetString(L, 1);
    Class_SetScoreMsg(currentMinigame, score, msg);
    return 0;
}

// (also ClassMathSetLevel: 1-based, clamped to 0..4)
int __cdecl Native_ClassEnglishSetLevel(lua_State* L) {
    uint8_t* mg = currentMinigame;
    int level = Script::GetInt(L, 0) - 1;
    if (level < 0) {
        At<int32_t>(mg, 0xC4) = 0;
        return 0;
    }
    if (level >= 5) {
        level = 4;
    }
    At<int32_t>(mg, 0xC4) = level;
    return 0;
}

// The trig function (a float) and its callback
int SetTrigFunc(lua_State* L, uint32_t callback) {
    uint8_t* mg = currentMinigame;
    At<float>(mg, 0x2C) = Script::GetFloat(L, 0);
    StoreCallback(L, 1, currentMinigame + callback);
    return 0;
}

int __cdecl Native_ClassBiologySetTrigFunc(lua_State* L) {
    return SetTrigFunc(L, 0xF4);
}

int __cdecl Native_ClassGeographySetTrigFunc(lua_State* L) {
    return SetTrigFunc(L, 0x37C);
}

int __cdecl Native_ClassMathSetTrigFunc(lua_State* L) {
    return SetTrigFunc(L, 0x380);
}

// ---- biology ---------------------------------------------------------------------------

GAME_FN(uint8_t, __thiscall, Biology_ValidOperation, 0x006E40B0, void* mg);
GAME_FN(uint8_t, __thiscall, Biology_InvalidOperation, 0x006E40D0, void* mg);
GAME_FN(void, __thiscall, Biology_SetAnimal, 0x006E4080, void* mg, int);

int __cdecl Native_ClassBiologyResetBadMove(lua_State*) {
    At<uint8_t>(currentMinigame, 0x119) = 0;
    return 0;
}

int __cdecl Native_ClassBiologyResetGrossOut(lua_State*) {
    At<uint8_t>(currentMinigame, 0x11B) = 0;
    return 0;
}

int __cdecl Native_ClassBiologyIsBadMove(lua_State* L) {
    Script::PushBool(L, At<uint8_t>(currentMinigame, 0x119));
    return 1;
}

int __cdecl Native_ClassBiologyIsGrossOut(lua_State* L) {
    Script::PushBool(L, At<uint8_t>(currentMinigame, 0x11B));
    return 1;
}

int __cdecl Native_ClassBiologyValidOperation(lua_State* L) {
    Script::PushBool(L, Biology_ValidOperation(currentMinigame));
    return 1;
}

int __cdecl Native_ClassBiologyInvalidOperation(lua_State* L) {
    Script::PushBool(L, Biology_InvalidOperation(currentMinigame));
    return 1;
}

int __cdecl Native_ClassBiologySetAnimal(lua_State* L) {
    uint8_t* mg = currentMinigame;
    Biology_SetAnimal(mg, Script::GetInt(L, 0));
    return 0;
}

// (a percentage, times 0.01f in double)
int __cdecl Native_ClassBiologySetScorePercentage(lua_State* L) {
    int percent = Script::GetInt(L, 0);
    At<float>(currentMinigame, 0x100) = float(percent * double(0.01f));
    return 0;
}

// ---- chemistry -------------------------------------------------------------------------

GAME_FN(int, __thiscall, Chem_GetLastAction, 0x006EB460, void* mg);
GAME_FN(uint8_t, __thiscall, Chem_GetActionJustFailed, 0x006EB3F0, void* mg, int action);
GAME_FN(void, __thiscall, Chem_SetActiveActions, 0x006E8130, void* mg, int);

int __cdecl Native_ClassChemGetLastAction(lua_State* L) {
    Script::PushInt(L, Chem_GetLastAction(currentMinigame));
    return 1;
}

int __cdecl Native_ClassChemGetActionJustFailed(lua_State* L) {
    int action = Script::GetInt(L, 0);
    Script::PushBool(L, Chem_GetActionJustFailed(currentMinigame, action));
    return 1;
}

int __cdecl Native_ClassChemGetPerformance(lua_State* L) {
    Script::PushFloat(L, At<float>(currentMinigame, 0xCB0));
    return 1;
}

int __cdecl Native_ClassChemSetActiveActions(lua_State* L) {
    uint8_t* mg = currentMinigame;
    Chem_SetActiveActions(mg, Script::GetInt(L, 0));
    return 0;
}

int __cdecl Native_ClassChemSetSpeedScale(lua_State* L) {
    uint8_t* mg = currentMinigame;
    At<float>(mg, 0xCD4) = Script::GetFloat(L, 0);
    return 0;
}

int __cdecl Native_ClassChemSetPixPerSec(lua_State* L) {
    uint8_t* mg = currentMinigame;
    At<float>(mg, 0xCD8) = Script::GetFloat(L, 0);
    return 0;
}

int __cdecl Native_ClassChemSetScrollyVisible(lua_State* L) {
    uint8_t* mg = currentMinigame;
    At<uint8_t>(mg, 0xCD0) = Script::GetBool(L, 0);
    return 0;
}

// (scrolly-only makes it visible too)
int __cdecl Native_ClassChemSetScrollyOnly(lua_State* L) {
    uint8_t* mg = currentMinigame;
    uint8_t only = Script::GetBool(L, 0);
    At<uint8_t>(mg, 0xCD1) = only;
    if (only != 0) {
        At<uint8_t>(mg, 0xCD0) = 1;
    }
    return 0;
}

// ---- English ---------------------------------------------------------------------------

GAME_FN(void, __thiscall, English_SetMinScore, 0x006EC380, void* mg, int);

int __cdecl Native_ClassEnglishSetWarnTime(lua_State* L) {
    uint8_t* mg = currentMinigame;
    At<float>(mg, 0x4B4) = Script::GetFloat(L, 0);
    return 0;
}

int __cdecl Native_ClassEnglishSetPenaltyTime(lua_State* L) {
    uint8_t* mg = currentMinigame;
    At<float>(mg, 0x4C0) = Script::GetFloat(L, 0);
    return 0;
}

int __cdecl Native_ClassEnglishSetMinScore(lua_State* L) {
    uint8_t* mg = currentMinigame;
    English_SetMinScore(mg, Script::GetInt(L, 0));
    return 0;
}

int __cdecl Native_ClassEnglishSetTrigFunc(lua_State* L) {
    uint8_t* mg = currentMinigame;
    At<float>(mg, 0x4B8) = Script::GetFloat(L, 0);
    StoreCallback(L, 1, currentMinigame + 0x4BC);
    return 0;
}

int __cdecl Native_ClassEnglishSetScorePercentage(lua_State* L) {
    int percent = Script::GetInt(L, 0);
    At<float>(currentMinigame, 0x4E8) = float(percent * double(0.01f));
    return 0;
}

// (copied to a 32-byte buffer first)
int __cdecl Native_ClassEnglishGetLastSubmittedWord(lua_State* L) {
    char word[32];
    strcpy(word, reinterpret_cast<const char*>(currentMinigame + 0x4DE));
    Script::PushString(L, word);
    return 1;
}

int __cdecl Native_ClassEnglishGetLastSubmittedWordLength(lua_State* L) {
    Script::PushInt(L, int(strlen(reinterpret_cast<const char*>(currentMinigame + 0x4DE))));
    return 1;
}

// The word flags: read once (cleared as they are read)
int TakeWordFlag(lua_State* L, uint32_t offset) {
    uint8_t* mg = currentMinigame;
    uint8_t flag = At<uint8_t>(mg, offset);
    At<uint8_t>(mg, offset) = 0;
    Script::PushBool(L, flag);
    return 1;
}

int __cdecl Native_ClassEnglishWordWasValid(lua_State* L) {
    return TakeWordFlag(L, 0x4D9);
}

int __cdecl Native_ClassEnglishWordWasNotValid(lua_State* L) {
    return TakeWordFlag(L, 0x4DA);
}

int __cdecl Native_ClassEnglishWordWasDuplicate(lua_State* L) {
    return TakeWordFlag(L, 0x4DB);
}

int __cdecl Native_ClassEnglishWordWasNaughty(lua_State* L) {
    return TakeWordFlag(L, 0x4DC);
}

int __cdecl Native_ClassEnglishWordWasTooShort(lua_State* L) {
    return TakeWordFlag(L, 0x4DD);
}

// ---- geography -------------------------------------------------------------------------

GAME_FN(uint8_t, __thiscall, Geography_ValidOperation, 0x006F2E20, void* mg);
GAME_FN(uint8_t, __thiscall, Geography_InvalidOperation, 0x006F2E40, void* mg);
GAME_FN(void, __thiscall, Geography_SetScoreMsg, 0x006F71B0, void* mg, float score, const char* msg);

int __cdecl Native_ClassGeographyValidOperation(lua_State* L) {
    Script::PushBool(L, Geography_ValidOperation(currentMinigame));
    return 1;
}

int __cdecl Native_ClassGeographyInvalidOperation(lua_State* L) {
    Script::PushBool(L, Geography_InvalidOperation(currentMinigame));
    return 1;
}

// (1-based, unclamped)
int __cdecl Native_ClassGeographySetLevel(lua_State* L) {
    uint8_t* mg = currentMinigame;
    At<int32_t>(mg, 0xC4) = Script::GetInt(L, 0) - 1;
    return 0;
}

// (times 0.01f and back times 100: a float round trip)
int __cdecl Native_ClassGeographySetScorePercentage(lua_State* L) {
    int percent = Script::GetInt(L, 0);
    float fraction = float(percent * double(0.01f));
    At<float>(currentMinigame, 0x388) = float(fraction * 100.0);
    return 0;
}

int __cdecl Native_ClassGeographySetScoreMsg(lua_State* L) {
    float score = float(Script::GetInt(L, 0));
    const char* msg = Script::GetString(L, 1);
    Geography_SetScoreMsg(currentMinigame, score, msg);
    return 0;
}

// ---- math ------------------------------------------------------------------------------

GAME_FN(void, __thiscall, Math_Finished, 0x00700DE0, void* mg);   // (also darts' StopAiming)
GAME_FN(uint8_t, __thiscall, Math_AnswerGiven, 0x006F8CB0, void* mg);
GAME_FN(uint8_t, __thiscall, Math_ValidAnswer, 0x006F8CD0, void* mg);
GAME_FN(uint8_t, __thiscall, Math_InvalidAnswer, 0x006F8CF0, void* mg);
GAME_FN(void, __thiscall, Math_SetNumQuestions, 0x006F8D10, void* mg, int);
GAME_FN(int, __thiscall, Math_GetScore, 0x006F8D20, void* mg);
GAME_FN(int, __thiscall, Math_GetScorePercentage, 0x006F8D30, void* mg);

// (also MGDartsStopAiming)
int __cdecl Native_ClassMathFinished(lua_State*) {
    Math_Finished(currentMinigame);
    return 0;
}

int __cdecl Native_ClassMathAnswerGiven(lua_State* L) {
    Script::PushBool(L, Math_AnswerGiven(currentMinigame));
    return 1;
}

int __cdecl Native_ClassMathValidAnswer(lua_State* L) {
    Script::PushBool(L, Math_ValidAnswer(currentMinigame));
    return 1;
}

int __cdecl Native_ClassMathInvalidAnswer(lua_State* L) {
    Script::PushBool(L, Math_InvalidAnswer(currentMinigame));
    return 1;
}

int __cdecl Native_ClassMathSetNumQuestions(lua_State* L) {
    uint8_t* mg = currentMinigame;
    Math_SetNumQuestions(mg, Script::GetInt(L, 0));
    return 0;
}

// (through a float and truncated back: exact to 2^24)
int __cdecl Native_ClassMathSetScorePercentage(lua_State* L) {
    float percent = float(Script::GetInt(L, 0));
    At<int32_t>(currentMinigame, 0x38C) = int32_t(percent);
    return 0;
}

int __cdecl Native_ClassMathGetScorePercentage(lua_State* L) {
    float percent = float(Math_GetScorePercentage(currentMinigame));
    Script::PushInt(L, int32_t(percent));
    return 1;
}

int __cdecl Native_ClassMathGetScore(lua_State* L) {
    float score = float(Math_GetScore(currentMinigame));
    Script::PushInt(L, int32_t(score));
    return 1;
}

// ---- music -----------------------------------------------------------------------------

GAME_FN(void, __thiscall, Music_SetInstrument, 0x006FB440, void* mg, int player, const char* name);
GAME_FN(void, __thiscall, Music_SetPlayers, 0x006FB1A0, void* mg, int);
GAME_FN(void, __thiscall, Music_StartSeq, 0x006FB2F0, void* mg, int);
GAME_FN(int, __thiscall, Music_GetWinner, 0x006FAD70, void* mg);
GAME_FN(int, __thiscall, Music_GetScore, 0x006FAD80, void* mg, int player);
GAME_FN(uint8_t, __cdecl, Fn_42E430, 0x0042E430);

int __cdecl Native_ClassMusicInstrument(lua_State* L) {
    int player = Script::GetInt(L, 0);
    const char* name = Script::GetString(L, 1);
    Music_SetInstrument(currentMinigame, player - 1, name);
    return 0;
}

int __cdecl Native_ClassMusicSetPlayers(lua_State* L) {
    uint8_t* mg = currentMinigame;
    Music_SetPlayers(mg, Script::GetInt(L, 0));
    return 0;
}

// (spins until 0x42E430 says so)
int __cdecl Native_ClassMusicStartSeq(lua_State* L) {
    int seq = Script::GetInt(L, 0);
    while (!Fn_42E430()) {
    }
    Music_StartSeq(currentMinigame, seq);
    return 0;
}

int __cdecl Native_ClassMusicFeedbackCallback(lua_State* L) {
    StoreCallbackOpt(L, 0, currentMinigame + 0xC4);
    return 0;
}

int __cdecl Native_ClassMusicGetWinner(lua_State* L) {
    int winner = Music_GetWinner(currentMinigame);
    int score = Music_GetScore(currentMinigame, winner);
    Script::PushInt(L, winner);
    Script::PushInt(L, score);
    return 2;
}

// ---- the arcade cabinet (MGArcade_) ------------------------------------------------------

GAME_FN(void, __thiscall, Arcade_LoadTextures, 0x006FD470, void* mg, const char*);
GAME_FN(void, __thiscall, Arcade_LoadSounds, 0x006FD1E0, void* mg, const char*);
GAME_FN(int, __thiscall, Arcade_GetSoundID, 0x006FD350, void* mg, const char*);

int __cdecl Native_MGArcade_SetRasteryEffect(lua_State* L) {
    uint8_t* mg = currentMinigame;
    At<uint8_t>(mg, 0xC4) = Script::GetBool(L, 0);
    return 0;
}

int __cdecl Native_MGArcade_SetCancelConfirm(lua_State* L) {
    uint8_t* mg = currentMinigame;
    At<uint8_t>(mg, 0xC5) = Script::GetBool(L, 0);
    return 0;
}

int __cdecl Native_MGArcade_GetDialogActive(lua_State* L) {
    Script::PushBool(L, At<uint8_t>(currentMinigame, 0xA0));
    return 1;
}

int __cdecl Native_MGArcade_LoadTextures(lua_State* L) {
    uint8_t* mg = currentMinigame;
    Arcade_LoadTextures(mg, Script::GetString(L, 0));
    return 0;
}

int __cdecl Native_MGArcade_LoadSounds(lua_State* L) {
    uint8_t* mg = currentMinigame;
    Arcade_LoadSounds(mg, Script::GetString(L, 0));
    return 0;
}

int __cdecl Native_MGArcade_GetSoundID(lua_State* L) {
    uint8_t* mg = currentMinigame;
    Script::PushInt(L, Arcade_GetSoundID(mg, Script::GetString(L, 0)));
    return 1;
}

// (the CRT's cos / sin, in double)
int __cdecl Native_MGArcade_Cos(lua_State* L) {
    float angle = Script::GetFloat(L, 0);
    Script::PushFloat(L, float(std::cos(double(angle))));
    return 1;
}

int __cdecl Native_MGArcade_Sin(lua_State* L) {
    float angle = Script::GetFloat(L, 0);
    Script::PushFloat(L, float(std::sin(double(angle))));
    return 1;
}

// ---- darts, keep-ups, soccer -------------------------------------------------------------

GAME_FN(void, __thiscall, Darts_StartAiming, 0x007015D0, void* mg);
GAME_FN(uint8_t, __thiscall, Darts_IsAiming, 0x00700DF0, void* mg);
GAME_FN(int, __thiscall, Darts_GetTargetPoints, 0x00701880, void* mg);
GAME_FN(int, __thiscall, KeepUps_GetLastAction, 0x00703430, void* mg);
GAME_FN(void, __thiscall, Soccer_StartAiming, 0x00715B40, void* mg);
GAME_FN(int, __thiscall, Soccer_GetHitTarget, 0x00714160, void* mg);

int __cdecl Native_MGDartsStartAiming(lua_State*) {
    Darts_StartAiming(currentMinigame);
    return 0;
}

int __cdecl Native_MGDartsIsAiming(lua_State* L) {
    Script::PushBool(L, Darts_IsAiming(currentMinigame));
    return 1;
}

int __cdecl Native_MGDartsGetTargetPoints(lua_State* L) {
    Script::PushInt(L, Darts_GetTargetPoints(currentMinigame));
    return 1;
}

int __cdecl Native_MGDartsSetGame(lua_State* L) {
    uint8_t* mg = currentMinigame;
    At<int32_t>(mg, 0xEC) = Script::GetInt(L, 0);
    return 0;
}

// (no bounds check on the index)
int __cdecl Native_MGDartsSetPoints(lua_State* L) {
    uint8_t* mg = currentMinigame;
    int points = Script::GetInt(L, 1);
    int index = Script::GetInt(L, 0);
    At<int32_t>(mg, 0xF0 + index * 4) = points;
    return 0;
}

int __cdecl Native_MGKeepUpsGetLastAction(lua_State* L) {
    Script::PushInt(L, KeepUps_GetLastAction(currentMinigame));
    return 1;
}

int __cdecl Native_MGKeepUpsSetSpeedScale(lua_State* L) {
    uint8_t* mg = currentMinigame;
    At<float>(mg, 0x804) = Script::GetFloat(L, 0);
    return 0;
}

int __cdecl Native_SoccerPStartAiming(lua_State*) {
    Soccer_StartAiming(currentMinigame);
    return 0;
}

int __cdecl Native_SoccerPStopAiming(lua_State*) {
    At<uint8_t>(currentMinigame, 4) = 0;
    return 0;
}

int __cdecl Native_SoccerPSetBallsToKick(lua_State* L) {
    uint8_t* mg = currentMinigame;
    At<int32_t>(mg, 0xE0) = Script::GetInt(L, 0);
    return 0;
}

// (1-based for the script)
int __cdecl Native_SoccerPGetHitTarget(lua_State* L) {
    Script::PushInt(L, Soccer_GetHitTarget(currentMinigame) + 1);
    return 1;
}

// ---- lawn mowing, races ------------------------------------------------------------------

GAME_FN(void, __thiscall, LawnMowing_SetMinScore, 0x00709540, void* mg, int);
GAME_FN(void, __thiscall, Race_CleanUp, 0x0070B640, void* mg);
GAME_FN(void, __thiscall, Race_DestroyTransition, 0x0070B830, void* mg);
GAME_FN(void, __thiscall, Race_SetArcade, 0x0070B630, void* mg);
GAME_FN(void, __thiscall, Race_Start, 0x00711BC0, void* mg);
GAME_FN(float, __thiscall, Race_GetDuration, 0x0070D480, void* mg);
GAME_FN(void, __thiscall, Race_AddNodeToHighlight, 0x0070F400, void* mg, int node);
GAME_FN(void, __thiscall, Race_AddNodeToBottleneck, 0x0070F670, void* mg, int node);
GAME_FN(void, __thiscall, Race_AddJumpNode, 0x0070F7F0, void* mg, int, int);
GAME_FN(void, __thiscall, RaceHud_ClearResults, 0x005467B0, void* hud);
GAME_FN(void, __thiscall, RaceHud_Start, 0x005467C0, void* hud);
GAME_VAR(uint8_t, raceTransitionDisplayed, 0x020C8824);

int __cdecl Native_LawnMowingSetMinScore(lua_State* L) {
    int score = Script::GetInt(L, 0);
    LawnMowing_SetMinScore(currentMinigame, score);
    return 0;
}

// (reads its argument and does nothing with it)
int __cdecl Native_LawnMowingSetTimer(lua_State* L) {
    Script::GetFloat(L, 0);
    return 0;
}

int __cdecl Native_RaceCleanUpRace(lua_State*) {
    Race_CleanUp(currentMinigame);
    return 0;
}

int __cdecl Native_RaceClearResults(lua_State*) {
    RaceHud_ClearResults(raceHud);
    return 0;
}

int __cdecl Native_RaceDestroyTransition(lua_State*) {
    Race_DestroyTransition(currentMinigame);
    return 0;
}

int __cdecl Native_RaceSetArcade(lua_State*) {
    Race_SetArcade(currentMinigame);
    return 0;
}

int __cdecl Native_RaceStartRace(lua_State*) {
    Race_Start(currentMinigame);
    RaceHud_Start(raceHud);
    return 0;
}

int __cdecl Native_RaceIsTransitionDisplayed(lua_State* L) {
    Script::PushBool(L, raceTransitionDisplayed);
    return 1;
}

int __cdecl Native_RaceShowFinishPointOnly(lua_State* L) {
    uint8_t only = Script::GetBool(L, 0);
    At<uint8_t>(currentMinigame, 0x155) = only;
    return 0;
}

int __cdecl Native_RaceGetDuration(lua_State* L) {
    Script::PushFloat(L, Race_GetDuration(currentMinigame));
    return 1;
}

int __cdecl Native_RaceAddNodeToHighlight(lua_State* L) {
    int node = Script::GetInt(L, 0);
    if (currentMinigame != nullptr) {
        Race_AddNodeToHighlight(currentMinigame, node);
    }
    return 0;
}

int __cdecl Native_RaceAddNodeToBottleneck(lua_State* L) {
    int node = Script::GetInt(L, 0);
    if (currentMinigame != nullptr) {
        Race_AddNodeToBottleneck(currentMinigame, node);
    }
    return 0;
}

int __cdecl Native_RaceAddJumpNode(lua_State* L) {
    int a = Script::GetInt(L, 0);
    int b = Script::GetInt(L, 1);
    Race_AddJumpNode(currentMinigame, a, b);
    return 0;
}

// (the race HUD's virtual at +0x14)
int __cdecl Native_RaceHUDVisible(lua_State* L) {
    void* hud = raceHud;
    uint8_t visible = Script::GetBool(L, 0);
    VCall<void>(hud, 0x14, visible);
    return 0;
}

} // namespace

// Untested: the game's code runs until each is checked.
#define NATC_REVERSED(name, address)       \
    static void* Original_Native_##name;   \
    REVERSED("Native_" #name, address, RE_BODY(Native_##name), nullptr, &Original_Native_##name, re::Mode::Original)

NATC_REVERSED(MinigameEnd, 0x00708CB0);
NATC_REVERSED(MinigameIsReady, 0x00708CD0);
NATC_REVERSED(MinigameIsActive, 0x00708CF0);
NATC_REVERSED(MinigameIsSuccess, 0x00708D20);
NATC_REVERSED(MinigameEnableHUD, 0x00708D50);
NATC_REVERSED(MinigameIsShowingGrades, 0x00708DF0);
NATC_REVERSED(MinigameIsFadingCompletion, 0x00709120);
NATC_REVERSED(MinigameHoldCompletion, 0x00709140);
NATC_REVERSED(MinigameReleaseCompletion, 0x00709150);
NATC_REVERSED(MinigameSetUberCompletion, 0x00709160);
NATC_REVERSED(MinigameSetElapsedGameTime, 0x00708D70);
NATC_REVERSED(MinigameGetErrandCompletion, 0x007090B0);
NATC_REVERSED(DodgeballEndGame, 0x00680190);
NATC_REVERSED(DodgeballInit, 0x006801A0);
NATC_REVERSED(DodgeballTerm, 0x006801B0);
NATC_REVERSED(DodgeballIsActive, 0x00680170);
NATC_REVERSED(DodgeballGetWinner, 0x006801C0);
NATC_REVERSED(DodgeballEnableCamera, 0x006801E0);
NATC_REVERSED(DodgeballEnableHud, 0x00680210);
NATC_REVERSED(DodgeballPause, 0x00680240);
NATC_REVERSED(DodgeballStartGame, 0x00680060);
NATC_REVERSED(CarnivalDunkTankIncCount, 0x006D8820);
NATC_REVERSED(CarnivalDunkTankDecCount, 0x006D8830);
NATC_REVERSED(CarnivalDunkTankGetCount, 0x006D8850);
NATC_REVERSED(CarnivalDunkTankSetCount, 0x006D87F0);
NATC_REVERSED(CarnivalDunkTankStartAiming, 0x006D8870);
NATC_REVERSED(MGCA_SetLocaleStringKey, 0x006E3130);
NATC_REVERSED(MGCA_SetTextureBank, 0x006E3160);
NATC_REVERSED(MGCA_SetCompletionThreshold, 0x006E3180);
NATC_REVERSED(MGCA_SetPlayerSpeed, 0x006E31B0);
NATC_REVERSED(MGCA_SetPathSpeedupFactor, 0x006E31E0);
NATC_REVERSED(MGCA_SetEnemySpeed, 0x006E3210);
NATC_REVERSED(MGCA_SetEnemyInitialPeriod, 0x006E3250);
NATC_REVERSED(MGCA_SetEnemyPeriod, 0x006E3290);
NATC_REVERSED(MGCA_SetEnemyInstanceMax, 0x006E32D0);
NATC_REVERSED(MGCA_SetEnemyTotalMax, 0x006E3300);
NATC_REVERSED(MGCA_SetEnemyCompletionSpawnThreshold, 0x006E3320);
NATC_REVERSED(MGCA_SetExploderTimeOut, 0x006E3350);
NATC_REVERSED(MGCA_SetExploderDebrisCount, 0x006E3380);
NATC_REVERSED(MGCA_SetExploderDebrisSpeed, 0x006E33A0);
NATC_REVERSED(MGCA_SetPowerUpSpeed, 0x006E34A0);
NATC_REVERSED(MGCA_SetPowerUpInitialPeriod, 0x006E34E0);
NATC_REVERSED(MGCA_SetPowerUpPeriod, 0x006E3520);
NATC_REVERSED(MGCA_SetPowerUpInstanceMax, 0x006E3560);
NATC_REVERSED(MGCA_SetPowerUpTotalMax, 0x006E3590);
NATC_REVERSED(MGCA_SetPowerUpCompletionSpawnThreshold, 0x006E35B0);
NATC_REVERSED(MGCA_SetPowerUpDuration, 0x006E35E0);
NATC_REVERSED(MGCA_SetLevelTimeout, 0x006E3620);
NATC_REVERSED(MGCA_SetLives, 0x006E3650);
NATC_REVERSED(MGCA_GetLives, 0x006E3670);
NATC_REVERSED(MGCA_SetDeckResolution, 0x006E3690);
NATC_REVERSED(MGCA_SetPowerUpSpeedBoostFactor, 0x006E36B0);
NATC_REVERSED(MGCA_SetPowerUpFreezeFactor, 0x006E36E0);
NATC_REVERSED(MGCA_SetSetPowerUpClearRadius, 0x006E3710);
NATC_REVERSED(MGCA_GetFailureCode, 0x006E37D0);
NATC_REVERSED(ClassGeographySuccess, 0x006E80A0);
NATC_REVERSED(ClassBiologySetWarnTime, 0x006FA3D0);
NATC_REVERSED(ClassBiologySetGameType, 0x006FA520);
NATC_REVERSED(ClassBiologySubtractTime, 0x006E7FA0);
NATC_REVERSED(ClassEnglishSetScoreMsg, 0x006EE240);
NATC_REVERSED(ClassEnglishSetLevel, 0x006EE3E0);
NATC_REVERSED(ClassBiologySetTrigFunc, 0x006E7F60);
NATC_REVERSED(ClassGeographySetTrigFunc, 0x006F72D0);
NATC_REVERSED(ClassMathSetTrigFunc, 0x006FA3F0);
NATC_REVERSED(ClassBiologyResetBadMove, 0x006E80D0);
NATC_REVERSED(ClassBiologyResetGrossOut, 0x006E8100);
NATC_REVERSED(ClassBiologyIsBadMove, 0x006E80B0);
NATC_REVERSED(ClassBiologyIsGrossOut, 0x006E80E0);
NATC_REVERSED(ClassBiologyValidOperation, 0x006E7E90);
NATC_REVERSED(ClassBiologyInvalidOperation, 0x006E7EC0);
NATC_REVERSED(ClassBiologySetAnimal, 0x006E8070);
NATC_REVERSED(ClassBiologySetScorePercentage, 0x006E8030);
NATC_REVERSED(ClassChemGetLastAction, 0x006EB710);
NATC_REVERSED(ClassChemGetActionJustFailed, 0x006EB6D0);
NATC_REVERSED(ClassChemGetPerformance, 0x006EB730);
NATC_REVERSED(ClassChemSetActiveActions, 0x006EB760);
NATC_REVERSED(ClassChemSetSpeedScale, 0x006EB790);
NATC_REVERSED(ClassChemSetPixPerSec, 0x006EB7B0);
NATC_REVERSED(ClassChemSetScrollyVisible, 0x006EB7D0);
NATC_REVERSED(ClassChemSetScrollyOnly, 0x006EB7F0);
NATC_REVERSED(ClassEnglishSetWarnTime, 0x006EE0F0);
NATC_REVERSED(ClassEnglishSetPenaltyTime, 0x006EE150);
NATC_REVERSED(ClassEnglishSetMinScore, 0x006EE170);
NATC_REVERSED(ClassEnglishSetTrigFunc, 0x006EE110);
NATC_REVERSED(ClassEnglishSetScorePercentage, 0x006EE200);
NATC_REVERSED(ClassEnglishGetLastSubmittedWord, 0x006EE280);
NATC_REVERSED(ClassEnglishGetLastSubmittedWordLength, 0x006EE2C0);
NATC_REVERSED(ClassEnglishWordWasValid, 0x006EE2F0);
NATC_REVERSED(ClassEnglishWordWasNotValid, 0x006EE320);
NATC_REVERSED(ClassEnglishWordWasDuplicate, 0x006EE350);
NATC_REVERSED(ClassEnglishWordWasNaughty, 0x006EE380);
NATC_REVERSED(ClassEnglishWordWasTooShort, 0x006EE3B0);
NATC_REVERSED(ClassGeographyValidOperation, 0x006F7200);
NATC_REVERSED(ClassGeographyInvalidOperation, 0x006F7230);
NATC_REVERSED(ClassGeographySetLevel, 0x006F73F0);
NATC_REVERSED(ClassGeographySetScorePercentage, 0x006F7370);
NATC_REVERSED(ClassGeographySetScoreMsg, 0x006F73B0);
NATC_REVERSED(ClassMathFinished, 0x006FA540);
NATC_REVERSED(ClassMathAnswerGiven, 0x006FA2D0);
NATC_REVERSED(ClassMathValidAnswer, 0x006FA300);
NATC_REVERSED(ClassMathInvalidAnswer, 0x006FA330);
NATC_REVERSED(ClassMathSetNumQuestions, 0x006FA430);
NATC_REVERSED(ClassMathSetScorePercentage, 0x006FA460);
NATC_REVERSED(ClassMathGetScorePercentage, 0x006FA4A0);
NATC_REVERSED(ClassMathGetScore, 0x006FA4E0);
NATC_REVERSED(ClassMusicInstrument, 0x006FCF10);
NATC_REVERSED(ClassMusicSetPlayers, 0x006FCFA0);
NATC_REVERSED(ClassMusicStartSeq, 0x006FCFD0);
NATC_REVERSED(ClassMusicFeedbackCallback, 0x006FD000);
NATC_REVERSED(ClassMusicGetWinner, 0x006FD020);
NATC_REVERSED(MGArcade_SetRasteryEffect, 0x00700560);
NATC_REVERSED(MGArcade_SetCancelConfirm, 0x00700580);
NATC_REVERSED(MGArcade_GetDialogActive, 0x007005A0);
NATC_REVERSED(MGArcade_LoadTextures, 0x006FF310);
NATC_REVERSED(MGArcade_LoadSounds, 0x006FF3A0);
NATC_REVERSED(MGArcade_GetSoundID, 0x006FF3D0);
NATC_REVERSED(MGArcade_Cos, 0x00700A80);
NATC_REVERSED(MGArcade_Sin, 0x00700AC0);
NATC_REVERSED(MGDartsStartAiming, 0x00701FE0);
NATC_REVERSED(MGDartsIsAiming, 0x00701FF0);
NATC_REVERSED(MGDartsGetTargetPoints, 0x00702080);
NATC_REVERSED(MGDartsSetGame, 0x00702110);
NATC_REVERSED(MGDartsSetPoints, 0x007020A0);
NATC_REVERSED(MGKeepUpsGetLastAction, 0x007035B0);
NATC_REVERSED(MGKeepUpsSetSpeedScale, 0x007035D0);
NATC_REVERSED(SoccerPStartAiming, 0x00715BA0);
NATC_REVERSED(SoccerPStopAiming, 0x00715BB0);
NATC_REVERSED(SoccerPSetBallsToKick, 0x00715D00);
NATC_REVERSED(SoccerPGetHitTarget, 0x00715CD0);
NATC_REVERSED(LawnMowingSetMinScore, 0x0070B4B0);
NATC_REVERSED(LawnMowingSetTimer, 0x0070B540);
NATC_REVERSED(RaceCleanUpRace, 0x007138A0);
NATC_REVERSED(RaceClearResults, 0x00713C50);
NATC_REVERSED(RaceDestroyTransition, 0x00713D20);
NATC_REVERSED(RaceSetArcade, 0x00713DF0);
NATC_REVERSED(RaceStartRace, 0x00713880);
NATC_REVERSED(RaceIsTransitionDisplayed, 0x00713D30);
NATC_REVERSED(RaceShowFinishPointOnly, 0x00713E00);
NATC_REVERSED(RaceGetDuration, 0x007139C0);
NATC_REVERSED(RaceAddNodeToHighlight, 0x00713B90);
NATC_REVERSED(RaceAddNodeToBottleneck, 0x00713BC0);
NATC_REVERSED(RaceAddJumpNode, 0x00713BF0);
NATC_REVERSED(RaceHUDVisible, 0x00713C20);
