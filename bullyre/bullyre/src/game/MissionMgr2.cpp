// The mission manager's life (construction, loading the missions from the action tree,
// saving, the per-frame update of its missions and its two slots' state machines) and a
// mission slot's start, setup, run, pass / fail and end. 0x6AB6F0..0x6ADEF3.
// Layouts: MissionMgr.cpp. A slot's state machine (+8): 1 starting (fade out) -> 2 set
// up (MissionSetup) -> 3 fade in -> 4 running; 0xC failed -> 5 "restart?" (-> 6, 7) or
// 8 passed -> 9 (main ends) -> 0xA (MissionCleanup) -> 0xB done -> 0 idle.

#include <cstdint>
#include <cstring>
#include <initializer_list>

#include "reversed.h"

#include "game/Crt.h"
namespace msnmgr2 {

namespace {

using re::At;

template <typename R, typename... A>
R VCall(void* object, int offset, A... args) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(fn)(object, args...);
}

inline int Ftol(float value) {
    return static_cast<int>(value);
}

void* const kMissionMgr = reinterpret_cast<void*>(0x020C3CA0);
void* const kPeriods = reinterpret_cast<void*>(0x020C35E8);
void* const kCamera = reinterpret_cast<void*>(0x00C3CC68);
void* const kUserInput = reinterpret_cast<void*>(0x00A45A30);
void* const kLurStore = reinterpret_cast<void*>(0x00D02850);
void* const kTheText = reinterpret_cast<void*>(0x020C2FA4);
void* const kPlayerInfo = reinterpret_cast<void*>(0x00C1AEA8);
void* const kAudio = reinterpret_cast<void*>(0x00CF3DA0);
void* const kGameLogic = reinterpret_cast<void*>(0x00BD1078);
void* const kAreaTransition = reinterpret_cast<void*>(0x00BCC580);
void* const kEvents = reinterpret_cast<void*>(0x00CF4378);
void* const kSpeech = reinterpret_cast<void*>(0x00B9DAB8);
void* const kMissionTimer = reinterpret_cast<void*>(0x00BF4508);
void* const kMissionStats = reinterpret_cast<void*>(0x00CF5DC4);
void* const kObjectives = reinterpret_cast<void*>(0x020C4420);
void* const kFireAlarm = reinterpret_cast<void*>(0x00CF3DA8);
GAME_VAR(int32_t, g_missionCount, 0x020C43E4);
GAME_VAR(uint8_t*, g_missionData, 0x020C4158);
GAME_VAR(uint8_t*, g_missionRecords, 0x020C4328);
GAME_VAR(int32_t, g_missionsPassed, 0x020C43E8);
GAME_VAR(void*, g_currentController, 0x020C432C);
GAME_ARRAY(int32_t, g_slot0, 0x020C415C);
GAME_ARRAY(int32_t, g_slot1, 0x020C4240);
GAME_VAR(uint8_t*, g_playerPed, 0x00C1AEA8);
GAME_VAR(uint8_t, g_playerFlags, 0x00C1AF6C);
GAME_VAR(uint8_t*, g_radar, 0x00C674CC);
GAME_VAR(void*, g_hudObject, 0x00C674C0);
GAME_VAR(uint8_t, g_cameraC3CE8C, 0x00C3CE8C);
GAME_VAR(uint8_t*, g_cameraRefC3CE90, 0x00C3CE90);
GAME_VAR(int32_t, g_cameraC3CE0C, 0x00C3CE0C);
GAME_VAR(int32_t, g_cameraC3CED8, 0x00C3CED8);
GAME_VAR(uint8_t, g_cameraC3CF82, 0x00C3CF82);
GAME_VAR(float, g_fade, 0x00C3CD28);
GAME_VAR(uint8_t, g_fadeColourR, 0x00C3CD2C);
GAME_VAR(uint8_t, g_fadeColourG, 0x00C3CD2D);
GAME_VAR(uint8_t, g_fadeColourB, 0x00C3CD2E);
GAME_VAR(uint8_t, g_fadeColourA, 0x00C3CD2F);
GAME_VAR(uint8_t, g_pedProduction, 0x00C2C10C);
GAME_VAR(void*, g_objectC2AF9C, 0x00C2AF9C);
GAME_VAR(uint8_t*, g_population, 0x00C2C108);
GAME_VAR(uint8_t, g_ambientTransitions, 0x00C2B3A0);
GAME_VAR(void*, g_traffic, 0x00BCD080);
GAME_VAR(uint8_t*, g_systemC0F5E4, 0x00C0F5E4);
GAME_VAR(uint8_t*, g_classes, 0x00C0F78C);
GAME_VAR(uint8_t, g_missionFlag20C43ED, 0x020C43ED);
GAME_VAR(uint8_t, g_slotsChanged, 0x020C441C);
GAME_VAR(float, g_respawnHeading, 0x020C4418);
GAME_ARRAY(float, g_respawnPos, 0x020C440C);
GAME_VAR(int32_t, g_music_CF3AE8, 0x00CF3AE8);
GAME_VAR(uint8_t, g_fireAlarmOn, 0x00CF3DC0);
GAME_VAR(uint8_t, g_skipFlag20C870A, 0x020C870A);
GAME_VAR(uint8_t, g_menuFlag20C8708, 0x020C8708);
GAME_VAR(uint8_t, g_flag20C5BE6, 0x020C5BE6);
GAME_VAR(int32_t, g_menuAllowed20C87AC, 0x020C87AC);
GAME_VAR(uint8_t, g_flagBD0FDC, 0x00BD0FDC);
GAME_VAR(uint32_t, g_timeInMs, 0x00C1A9B4);
GAME_VAR(uint32_t, g_frontendTime, 0x00A14A50);
GAME_VAR(float, g_timeStep, 0x00C1A9A4);
GAME_VAR(float, g_controllerStepScale, 0x00BA6BBC);
GAME_VAR(uint8_t*, g_areaCounter, 0x009E4F4C);
GAME_ARRAY(float, g_zeroVector, 0x00BCC564);
GAME_ARRAY(uint8_t, g_fontColour, 0x0208B764);

GAME_FN(void*, __cdecl, M2_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, M2_Free, 0x005EEAB0, void* p);
inline void* __cdecl M2_Memset(void* p, int value, size_t size) { return crt::Memset(p, value, size); }
GAME_FN(void, __stdcall, M2_VectorConstruct, 0x00401240, void* array, size_t size, int count, void* ctor);
GAME_FN(void, __cdecl, M2_PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, M2_PopMemoryTag, 0x005EF100);
GAME_FN(void, __cdecl, M2_CleanUpOldReference, 0x004657E0, void* entity, void* reference);
GAME_FN(void, __cdecl, M2_RegisterReference, 0x004657D0, void* entity, void* reference);
GAME_FN(uint32_t*, __thiscall, M2_NameKey, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(void, __thiscall, M2_NameKeyDefault, 0x0066BEA0, uint32_t* key);
GAME_FN(void, __thiscall, M2_NameKeyDestruct, 0x0049A650, uint32_t* key);
GAME_FN(uint32_t, __cdecl, M2_HashName, 0x00576ED0, const char* name);
GAME_FN(int32_t, __thiscall, M2_Find, 0x006A9FD0, void* mgr, const uint32_t* key);
GAME_FN(uint8_t*, __thiscall, M2_Data, 0x006AA660, void* mgr, int32_t mission);
GAME_FN(uint8_t, __thiscall, M2_HasTag, 0x006AB290, void* mgr, int32_t mission, uint32_t hash);
GAME_FN(uint8_t, __thiscall, M2_IsArcade, 0x006AB380, void* mgr, int32_t mission);
GAME_FN(uint8_t, __thiscall, M2_IsRunning, 0x006AA940, void* mgr, int32_t mission);
GAME_FN(int32_t, __thiscall, M2_FindController, 0x006A9FA0, void* mgr, int32_t mission);
GAME_FN(void, __thiscall, M2_ControllerStop, 0x006A9E60, void* controller);
GAME_FN(uint8_t, __thiscall, M2_ControllerStart, 0x006AB430, void* controller, int32_t mission);
GAME_FN(void, __thiscall, M2_ControllerUpdate, 0x006AB580, void* controller, float dt);
GAME_FN(void, __thiscall, M2_ControllerPlayNode, 0x006AB670, void* controller, const char* name);
GAME_FN(void, __thiscall, M2_ControllerMissed, 0x006AC350, void* controller);
GAME_FN(void, __thiscall, M2_Collect, 0x006AB940, void* mgr, void* node, re::Ptr32<void>* list, int32_t* count);
GAME_FN(void, __thiscall, M2_Reset, 0x006AB7B0, void* mgr);
GAME_FN(void, __thiscall, M2_Destruct, 0x006AB6F0, void* mgr);
GAME_FN(uint8_t, __thiscall, M2_SlotIsRunning, 0x006AA430, int32_t* slot);
GAME_FN(void, __thiscall, M2_SlotReset, 0x006AA190, int32_t* slot);
GAME_FN(void, __thiscall, M2_SlotAbandon, 0x006AA450, int32_t* slot, int32_t next);
GAME_FN(void, __thiscall, M2_SlotRequest, 0x006AA4A0, int32_t* slot, int32_t mission);
GAME_FN(void, __thiscall, M2_SlotLoadTextures, 0x006AA510, int32_t* slot);
GAME_FN(void, __thiscall, M2_SlotReleaseTextures, 0x006AA620, int32_t* slot);
GAME_FN(void, __thiscall, M2_SlotStart, 0x006ACFD0, int32_t* slot, int32_t mission);
GAME_FN(void, __thiscall, M2_SlotSetup, 0x006ACD00, int32_t* slot);
GAME_FN(void, __thiscall, M2_SlotEnd, 0x006ACA70, int32_t* slot);
GAME_FN(void, __thiscall, M2_SlotRun, 0x006AC080, int32_t* slot);
GAME_FN(void, __thiscall, M2_SlotStopScript, 0x006ABFF0, int32_t* slot);
GAME_FN(void, __thiscall, M2_SlotCleanup, 0x006AC030, int32_t* slot);
GAME_FN(void, __thiscall, M2_SlotUpdate, 0x006AD540, int32_t* slot);
GAME_FN(void, __thiscall, M2_Start, 0x006AD100, void* mgr, int32_t mission, int abandon);
GAME_FN(void, __thiscall, M2_UpdateMissions, 0x006AD290, void* mgr, int all);
GAME_FN(void, __thiscall, M2_MarkClasses, 0x006AADD0, void* mgr);
GAME_FN(uint8_t*, __thiscall, M2_ScriptName, 0x006ABB90, void* mgr, int32_t mission, char* out, int size);
GAME_FN(uint8_t, __cdecl, M2_NotFading, 0x006AA080);
GAME_FN(void, __cdecl, M2_FadeOut, 0x006AA0A0);
GAME_FN(void, __cdecl, M2_FadeIn, 0x006AA0F0);
GAME_FN(void, __cdecl, M2_GenerateBoundingBoxes, 0x0045B960, int);
GAME_FN(void, __thiscall, M2_MissionDataCopy, 0x0044F390, void* data, const uint8_t* from);
GAME_FN(void, __thiscall, M2_MissionDataDestruct, 0x0044E0D0, void* data);
GAME_FN(void, __cdecl, M2_SkipToNextPeriod, 0x006AA1F0);
GAME_FN(void, __thiscall, M2_PeriodsReset, 0x006A8B40, void* periods);
GAME_FN(void, __thiscall, M2_PeriodsLoad, 0x006A8AA0, void* periods);
GAME_FN(void, __thiscall, M2_PeriodsUpdate, 0x006A8F70, void* periods);
GAME_FN(void, __thiscall, M2_PeriodsShutdown, 0x006A8E40, void* periods);
GAME_FN(uint8_t*, __cdecl, M2_ActTreeFind, 0x005F5B10, const char* path, uint8_t* from);
GAME_FN(uint8_t*, __cdecl, M2_ActFileLoad, 0x005F6DB0, const char* file, uintptr_t, uintptr_t);
GAME_FN(void, __thiscall, M2_TrackConstruct, 0x0061A880, void* track);
GAME_FN(void, __thiscall, M2_TrackDestruct, 0x0061A8B0, void* track);
GAME_FN(void, __thiscall, M2_TrackDescApply, 0x005FA940, void* desc, void* track);
GAME_FN(uint32_t*, __thiscall, M2_TrackDescKey, 0x005FA9B0, void* desc, uint32_t* out);
GAME_FN(void*, __thiscall, M2_ControllerConstruct, 0x005F3E50, void* controller);
GAME_FN(void, __thiscall, M2_ControllerDestruct, 0x005F40F0, void* controller);
GAME_FN(void, __thiscall, M2_ControllerReset, 0x005F40A0, void* controller);
GAME_FN(void, __thiscall, M2_ControllerRelease, 0x005F4070, void* controller);
GAME_FN(void, __thiscall, M2_ControllerTick, 0x005F4B00, void* controller, float dt);
GAME_FN(uint8_t, __thiscall, M2_NodeListContains, 0x005F48D0, void* list, void* node, int a, int b);
GAME_FN(void, __thiscall, M2_InputsUpdate, 0x005F3880, void* inputs, float dt);
GAME_FN(uint8_t*, __thiscall, M2_RGBA, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, M2_FontReset, 0x00688E60);
GAME_FN(void, __cdecl, M2_FontPrint, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __cdecl, M2_FontRender, 0x0068CDF0);
GAME_FN(const uint16_t*, __thiscall, M2_TextGet, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __thiscall, M2_TextLoadMission, 0x0068FCB0, void* text, const char* name);
GAME_FN(void, __thiscall, M2_TextLoadMinigame, 0x0068FD00, void* text, const char* name);
GAME_FN(void, __thiscall, M2_Tokenise, 0x0061A470, void* tokens, const char* text, const char* separators);
GAME_FN(void, __thiscall, M2_TokenNext, 0x0061A450, void* tokens);
GAME_FN(void, __thiscall, M2_Token4D0, 0x0061A4D0, void* tokens);
GAME_FN(void, __thiscall, M2_TokenPrevious, 0x0061A460, void* tokens);
GAME_FN(uint8_t, __thiscall, M2_TokenIs, 0x0061A350, void* tokens, const char* text);
GAME_FN(void, __thiscall, M2_TokenCopy, 0x0061A3B0, void* tokens, const char* from, char* out, int size, int,
        int);
GAME_FN(void, __thiscall, M2_RecordCopy, 0x00733330, void* to, const void* from);
GAME_FN(uint32_t, __cdecl, M2_Checksum, 0x00733E60, const void* data, int size);
GAME_FN(uint8_t, __cdecl, M2_SaveCheck, 0x00733320, int, int, int value, int expected);
GAME_FN(uint8_t, __cdecl, M2_SaveCheckBlock, 0x00733620, const char* name, void* data, int size);
GAME_FN(void, __thiscall, M2_RecordAssign, 0x006A9DC0, void* record, const void* from);
GAME_FN(uint8_t, __cdecl, M2_TimePaused, 0x00705500);
GAME_FN(void, __thiscall, M2_PopupInit, 0x00705070, void* popup);
GAME_FN(int, __thiscall, M2_PopupResult, 0x00705080, void* popup);
GAME_FN(void, __thiscall, M2_PopupShow, 0x00705220, void* popup, int, const char* title, const char* buttons, int a,
        int b);
GAME_FN(void, __thiscall, M2_PopupDraw, 0x00706000, void* popup);
GAME_FN(void, __thiscall, M2_InputSet50DFD0, 0x0050DFD0, void* input, int);
GAME_FN(void, __thiscall, M2_InputSetEnabled, 0x0050DFA0, void* input, int enabled);
GAME_FN(void, __thiscall, M2_PlayerSetUnlocks, 0x004466D0, void* info, int word, uint32_t flags);
GAME_FN(void, __thiscall, M2_PlayerSetPaused, 0x00445DA0, void* info, int paused);
GAME_FN(uint8_t*, __thiscall, M2_LurFindScript, 0x005DBCC0, void* store, const char* name);
GAME_FN(uint8_t*, __thiscall, M2_LurLaunch, 0x005DC0F0, void* store, const char* name, int a);
GAME_FN(void, __thiscall, M2_LurStoreUpdate, 0x005DBB40, void* store, int);
GAME_FN(void, __thiscall, M2_LurStore5DAF40, 0x005DAF40, void* store);
GAME_FN(void, __thiscall, M2_ScriptStop5D9050, 0x005D9050, uint8_t* script);
GAME_FN(void, __thiscall, M2_ScriptEnd, 0x005D9E40, uint8_t* script, int);
GAME_FN(void, __thiscall, M2_ScriptThread, 0x005D9E80, uint8_t* script, const char* function);
GAME_FN(uint8_t, __thiscall, M2_ScriptRunning, 0x005D8F10, uint8_t* script, const char* function);
inline char* __cdecl M2_Strstr(const char* text, const char* find) { return crt::Strstr(text, find); }
template <typename... A>
inline int __cdecl M2_Sprintf(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
GAME_FN(int, __cdecl, M2_AddTxdSlot, 0x005F26E0, const char* name);
GAME_FN(uint8_t, __cdecl, M2_LoadTxd, 0x005F2A80, int slot, const char* file);
GAME_FN(void, __cdecl, M2_AddRef, 0x005F1130, int slot);
GAME_FN(void, __cdecl, M2_RemoveTxdSlot, 0x005F2240, int slot);
GAME_FN(void, __thiscall, M2_SetRespawn, 0x00416B50, void* transition, const float* pos, float heading);
GAME_FN(uint8_t, __thiscall, M2_Events59FF80, 0x0059FF80, void* events, int32_t mission);
GAME_FN(void, __thiscall, M2_EventsAdd, 0x005A4920, void* events, void* out, uint8_t* ped, uintptr_t, int, int kind, int,
        int, int, int, int, uintptr_t);
GAME_FN(void, __thiscall, M2_CameraSetShot, 0x004F2C20, void* camera, int, int, int, int, int);
GAME_FN(int, __thiscall, M2_CameraFadeState, 0x004F12F0, void* camera);
GAME_FN(int32_t, __thiscall, M2_Camera43A590, 0x0043A590, void* camera);
GAME_FN(void, __thiscall, M2_MissionTimerStop, 0x0043F750, void* timer, int);
GAME_FN(void, __thiscall, M2_Stats59A600, 0x0059A600, void* stats, int32_t mission);
GAME_FN(void, __thiscall, M2_Stats59A490, 0x0059A490, void* stats, int32_t mission);
GAME_FN(void, __thiscall, M2_Object46FA20, 0x0046FA20, void* object);
GAME_FN(void, __thiscall, M2_PopulationRevert, 0x0049C6D0, void* population);
GAME_FN(void, __thiscall, M2_TrafficRevert, 0x0041A860, void* traffic);
GAME_FN(void, __thiscall, M2_PopulationClearDisabled, 0x0049C610, void* population);
GAME_FN(void, __thiscall, M2_RespawnReset, 0x004315B0, void* logic);
GAME_FN(void, __thiscall, M2_ObjectivesReset, 0x006AE020, void* objectives);
GAME_FN(void, __thiscall, M2_Objectives6AE5E0, 0x006AE5E0, void* objectives);
GAME_FN(void, __cdecl, M2_Fn5BB390, 0x005BB390);
GAME_FN(void, __thiscall, M2_SpeechStop, 0x00684CF0, void* speech);
GAME_FN(void, __thiscall, M2_AlliesDismiss, 0x00485A40, void* allies);
GAME_FN(void, __cdecl, M2_ClearArea, 0x0045EC50, const float* pos, float radius, int);
GAME_FN(void, __thiscall, M2_Radar548F80, 0x00548F80, void* radar, int, int);
GAME_FN(uint8_t, __thiscall, M2_Area415380, 0x00415380, void* area);
GAME_FN(float, __cdecl, M2_Max, 0x00414490, float a, float b);
GAME_FN(uint8_t, __cdecl, M2_Music58FD80, 0x0058FD80);
GAME_FN(void, __cdecl, M2_FadeWithCamera, 0x0059E5E0, int category, int follows);
GAME_FN(void, __cdecl, M2_ShowTitle, 0x006881B0, const char* key, float time, float scale);
GAME_FN(void, __cdecl, M2_Fn4261A0, 0x004261A0);
GAME_FN(void, __cdecl, M2_Fn425F80, 0x00425F80);
GAME_FN(void, __cdecl, M2_Fn425FB0, 0x00425FB0);
GAME_FN(void, __cdecl, M2_AddMinutes, 0x00425D80, uint32_t minutes);
GAME_FN(void, __cdecl, M2_SetClock, 0x00426070, int hours, int minutes);
GAME_FN(void, __thiscall, M2_RestorePlayer, 0x00432320, void* logic, int, float* pos);
GAME_FN(void, __thiscall, M2_Logic4314F0, 0x004314F0, void* logic, int busted, int knockedOut, int c);
GAME_FN(void, __thiscall, M2_SoundsReset, 0x00596270, void* audio);
GAME_FN(void, __thiscall, M2_FireAlarmStop, 0x005950B0, void* alarm);
GAME_FN(float, __cdecl, M2_PadValue, 0x00738840, int control, int pad);

int32_t* Controller(void* mgr, int index) {
    return &At<int32_t>(mgr, 8 + index * 0xC);
}

uint8_t* GlobalData(int32_t mission) {
    int32_t index = mission < 0 || mission >= g_missionCount ? 0 : mission;
    return g_missionData + index * 0x4C;
}

uint8_t* Data(void* mgr, int32_t mission) {
    int32_t index = mission < 0 || mission >= At<int32_t>(mgr, 0x744) ? 0 : mission;
    return At<uint8_t*>(mgr, 0x4B8) + index * 0x4C;
}

bool SlotRunning(const int32_t* slot) {
    return slot[0] != -1 && slot[2] != 0 && slot[2] != 0xB;
}

void MakeCurrent(void* controller) {
    At<void*>(controller, 0x18) = &g_currentController;
    g_currentController = controller;
}

void ReleaseReference(re::Ptr32<uint8_t>& reference) {
    if (reference != nullptr) {
        M2_CleanUpOldReference(reference, &reference);
    }
}

// the player's respawn and state set back (a mission's setup and end share it)
void ResetPlayerState(bool checkWeapon) {
    uint8_t* ped = g_playerPed;
    if (At<uint8_t>(ped, 0x1E91) != 0 && (!checkWeapon || At<int32_t>(ped, 0x13DC) == 0)) {
        M2_SetRespawn(kAreaTransition, g_respawnPos, g_respawnHeading);
    }
    At<int32_t>(ped, 0x13C0) = 1;
    At<int32_t>(ped, 0x1344) = 0;
    At<int32_t>(ped, 0x1348) = 0;
}

void ClearCameraReference() {
    g_cameraC3CE8C = 0;
    if (g_cameraRefC3CE90 != nullptr) {
        M2_CleanUpOldReference(g_cameraRefC3CE90, &g_cameraRefC3CE90);
        g_cameraRefC3CE90 = nullptr;
    }
}

// the player's controller out of "InvulnerableIdle"
void LeaveInvulnerableIdle() {
    uint8_t* node = M2_ActTreeFind("/Global/SimpleLoco/Mission/InvulnerableIdle", nullptr);
    uint8_t* controller = At<uint8_t*>(g_playerPed, 0x2E0) + 0x258;
    if (M2_NodeListContains(controller, node, 1, 0)) {
        M2_ControllerReset(At<uint8_t*>(g_playerPed, 0x2E0) + 0x258);
    }
}

// the mission's title shown on starting (story, kind 3 and classes with +0x24)
void ShowTitle(int32_t* slot, uint8_t* data) {
    int32_t kind = At<int32_t>(data, 0x28);
    if ((kind == 0 || kind == 3 || kind == 1) && At<uint8_t>(data, 0x24) != 0 &&
        At<uint8_t>(slot, 0x80) == 0) {
        const char* title = At<const char*>(data, 0x20);
        if (title != nullptr && *title != 0) {
            M2_ShowTitle(title, 5.0f, 2.0f);
        }
    }
}

// held back while the game's time is paused (unless 0x20C870A)
bool Held() {
    if (M2_TimePaused()) {
        if (!M2_TimePaused() || g_skipFlag20C870A == 0) {
            return true;
        }
    }
    return false;
}

} // namespace

// 0x6AB6F0: the destructor: its references released, the controllers deleted
void __fastcall MissionMgr_Destruct(uint8_t* self) {
    At<uint32_t>(self, 0) = 0x00941D50;
    At<uint8_t>(self, 0x740) = 0;
    ReleaseReference(At<uint8_t*>(self, 0x6F0));
    ReleaseReference(At<uint8_t*>(self, 0x6EC));
    ReleaseReference(At<uint8_t*>(self, 0x698));
    ReleaseReference(At<uint8_t*>(self, 0x694));
    for (int i = 99; i >= 0; --i) {
        int32_t* controller = Controller(self, i);
        void* a = reinterpret_cast<void*>(controller[0]);
        if (a != nullptr) {
            M2_ControllerDestruct(a);
            M2_Free(a);
        }
        void* b = reinterpret_cast<void*>(controller[1]);
        if (b != nullptr) {
            M2_ControllerDestruct(b);
            M2_Free(b);
        }
    }
}

// 0x6AB7B0 (vf5): Reset: the slots, the state, every controller released, the records
// cleared, the periods, the passed count
void __fastcall MissionMgr_Reset(uint8_t* self) {
    M2_SlotReset(&At<int32_t>(self, 0x4BC));
    M2_SlotReset(&At<int32_t>(self, 0x5A0));
    float count = float(At<int32_t>(self, 0x744));
    At<int32_t>(self, 0x760) = 0;
    At<uint8_t>(self, 0x77C) = 0;
    At<uint8_t>(self, 0x764) = 0;
    At<int32_t>(self, 0x750) = 0;
    At<uint8_t>(self, 0x74D) = 0;
    At<uint8_t>(self, 0x75C) = 1;
    At<int32_t>(self, 0x758) = -1;
    At<uint8_t>(self, 0x75D) = 0;
    int32_t n = Ftol(count < count ? count : count);  // (a min of the count with itself)
    for (int i = 0; i < 100; ++i) {
        int32_t* controller = Controller(self, i);
        if (controller[2] == -1) {
            continue;
        }
        g_missionRecords[controller[2] * 6 + 4] &= 0xFE;
        void* a = reinterpret_cast<void*>(controller[0]);
        MakeCurrent(a);
        M2_ControllerRelease(a);
        void* b = reinterpret_cast<void*>(controller[1]);
        MakeCurrent(b);
        M2_ControllerRelease(b);
        controller[2] = -1;
    }
    for (int i = 0; i < 2; ++i) {
        At<uint8_t>(self, 0x4D2 + i * 0xE4) = 0;
    }
    // (the game clears a stack temporary record per mission, Record_Clear inlined: its
    // flags are an uninitialised stack word ANDed with 0x90, so bits 0x10 / 0x80 and the
    // high byte come from whatever was on the stack; 0 here)
    uint16_t flags = 0;
    for (int i = 0; i < n; ++i) {
        uint8_t* record = At<uint8_t*>(self, 0x688) + i * 6;
        flags = uint16_t((flags & 0xFF00) | (flags & 0x90));
        At<uint32_t>(record, 0) = 0;
        At<uint16_t>(record, 4) = flags;
    }
    M2_PeriodsReset(kPeriods);
    At<int32_t>(self, 0x748) = 0;
    for (int i = 0; i < At<int32_t>(self, 0x744); ++i) {
        if (At<uint16_t>(At<uint8_t*>(self, 0x688), i * 6 + 2) != 0) {
            At<int32_t>(self, 0x748) += 1;
        }
    }
}

// 0x6AB940: the tree's mission nodes collected (their first track a "MissionData", read
// into a temporary to check it)
void __fastcall MissionMgr_Collect(uint8_t* self, re::Edx, void* node, re::Ptr32<void>* list, int32_t* count) {
    for (int i = 0; i < VCall<int32_t>(node, 0x2C); ++i) {
        void* child = VCall<void*>(node, 0x30, i);
        bool mission = false;
        if (VCall<int32_t>(child, 0x40) >= 1) {
            void* track = VCall<void*>(child, 0x48, 0);
            if (track != nullptr) {
                uint32_t key;
                uint32_t* name = M2_TrackDescKey(track, &key);
                mission = *name != 0 && M2_HashName("MissionData") == *name;
                M2_NameKeyDestruct(&key);
            }
            if (mission) {
                alignas(4) uint8_t data[0x4C];
                M2_TrackConstruct(data);
                At<uint32_t>(data, 0) = 0x009063D8;
                M2_NameKeyDefault(&At<uint32_t>(data, 0x18));
                M2_NameKeyDefault(&At<uint32_t>(data, 0x1C));
                M2_NameKeyDefault(&At<uint32_t>(data, 0x20));
                M2_NameKeyDefault(&At<uint32_t>(data, 0x44));
                M2_TrackDescApply(track, data);
                list[*count] = child;
                *count += 1;
                At<uint32_t>(data, 0) = 0x009063D8;
                M2_NameKeyDestruct(&At<uint32_t>(data, 0x44));
                M2_NameKeyDestruct(&At<uint32_t>(data, 0x20));
                M2_NameKeyDestruct(&At<uint32_t>(data, 0x1C));
                M2_NameKeyDestruct(&At<uint32_t>(data, 0x18));
                M2_TrackDestruct(data);
                continue;
            }
        }
        M2_Collect(self, child, list, count);
    }
}

// 0x6ABAC0 (vf4): the "restart / continue" prompt (red, faded in)
void __fastcall MissionMgr_DrawRestartPrompt(void*) {
    M2_FontReset();
    int alpha = Ftol(g_fade);
    uint8_t colour[4];
    uint8_t* c = M2_RGBA(colour, 0xFF, 0, 0, uint8_t(alpha));
    std::memcpy(g_fontColour, c, 4);
    M2_FontPrint(20.0f, 100.0f, M2_TextGet(kTheText, "MSN_RESTART"), 0);
    M2_FontPrint(20.0f, 250.0f, M2_TextGet(kTheText, "MSN_CONT"), 0);
    M2_FontRender();
}

// 0x6ABB90: a mission's script's file name (its path's last part before ".lua"); no
// script: its name
char* __fastcall MissionMgr_ScriptName(uint8_t* self, re::Edx, int32_t mission, char* out, int size) {
    if (mission == -1) {
        return nullptr;
    }
    uint8_t* data = At<uint8_t*>(self, 0x4B8) + mission * 0x4C;
    const char* script = At<const char*>(data, 0x1C);
    if (script == nullptr) {
        return At<char*>(data, 0x18);
    }
    alignas(4) uint8_t tokens[0x3C];
    M2_Tokenise(tokens, script, "\\/.");
    while (*At<const char*>(tokens, 0) != 0) {
        M2_TokenNext(tokens);
        M2_Token4D0(tokens);
    }
    M2_TokenPrevious(tokens);
    if (!M2_TokenIs(tokens, "lua")) {
        return nullptr;
    }
    M2_TokenPrevious(tokens);
    M2_TokenCopy(tokens, At<const char*>(tokens, 0), out, size, 0, 1);
    return out;
}

// 0x6ABC40: saved: the records (debug missions left out) and a checksum of the names
void __fastcall MissionMgr_Save(uint8_t* self, re::Edx, int32_t* block) {
    uint32_t names[0x1FE];
    M2_Memset(names, 0, 0x7F8);
    block[0] = 1;
    block[1] = 0xC84;
    block[2] = 0x1FE;
    int32_t count = At<int32_t>(self, 0x744);
    for (int i = 0; i < count; ++i) {
        uint32_t debug = M2_HashName("Debug");
        bool isDebug = false;
        for (void* track = re::PtrCast<re::Ptr32<void>*>(At<uint8_t*>(self, 0x684))[i]; track != nullptr;
             track = VCall<void*>(track, 0x34, -1)) {
            if (*VCall<uint32_t*>(track, 8) == debug) {
                isDebug = true;
                break;
            }
        }
        if (isDebug) {
            continue;
        }
        M2_RecordCopy(reinterpret_cast<uint8_t*>(block + 4) + i * 6, At<uint8_t*>(self, 0x688) + i * 6);
        names[i] = *VCall<uint32_t*>(re::PtrCast<re::Ptr32<void>*>(At<uint8_t*>(self, 0x684))[i], 8);
    }
    block[3] = int32_t(M2_Checksum(names, 0x7F8));
}

// 0x6ABD50: loaded when the block and the names' checksum match: the controllers stopped,
// the records copied (not running)
void __fastcall MissionMgr_Load(uint8_t* self, re::Edx, int32_t* block) {
    if (!M2_SaveCheck(0, 0, block[0], 1) || !M2_SaveCheck(0, 0, block[1], 0xC84) ||
        !M2_SaveCheck(0, 0, block[2], 0x1FE) || !M2_SaveCheckBlock("MissionSaveData", block + 0x301, 0x20)) {
        return;
    }
    float count = float(At<int32_t>(self, 0x744));
    int32_t n = Ftol(count < count ? count : count);
    uint32_t names[0x1FE];
    M2_Memset(names, 0, 0x7F8);
    for (int i = 0; i < n; ++i) {
        if (!M2_HasTag(kMissionMgr, i, M2_HashName("Debug"))) {
            names[i] = *VCall<uint32_t*>(re::PtrCast<re::Ptr32<void>*>(At<uint8_t*>(self, 0x684))[i], 8);
        }
    }
    if (M2_Checksum(names, 0x7F8) != uint32_t(block[3])) {
        return;
    }
    for (int i = 0; i < 100; ++i) {
        int32_t* controller = Controller(self, i);
        if (controller[2] != -1) {
            M2_ControllerStop(controller);
        }
    }
    const uint8_t* from = reinterpret_cast<const uint8_t*>(block + 4);
    for (int i = 0; i < n; ++i, from += 6) {
        M2_RecordAssign(At<uint8_t*>(self, 0x688) + i * 6, from);
        At<uint8_t*>(self, 0x688)[i * 6 + 4] &= 0xFE;
    }
    At<int32_t>(self, 0x748) = 0;
    for (int i = 0; i < At<int32_t>(self, 0x744); ++i) {
        if (At<uint16_t>(At<uint8_t*>(self, 0x688), i * 6 + 2) != 0) {
            At<int32_t>(self, 0x748) += 1;
        }
    }
}

// 0x6ABF20: a running slot's exit popup drawn (when up)
void __fastcall Slot_DrawPopup(int32_t* self) {
    if (self[0] != -1 && self[2] != 0 && self[2] != 0xB && At<uint8_t>(self, 0x70) != 0) {
        M2_PopupDraw(reinterpret_cast<uint8_t*>(self) + 0x20);
    }
}

// 0x6ABF40: a running mission passed (faded out, or not)
void __fastcall Slot_Pass(int32_t* self, re::Edx, int a, int fade, int) {
    g_flagBD0FDC = 0;
    At<uint8_t>(self, 0xE) = uint8_t(a);
    if (self[1] == 0 || self[2] != 4) {
        return;
    }
    At<uint8_t>(self, 0xC) = 1;
    if (uint8_t(fade) != 0) {
        M2_FadeOut();
    } else {
        At<uint8_t>(self, 0x13) = 1;
    }
    self[2] = 8;
    if (At<uint8_t>(GlobalData(self[0]), 0x4A) != 0) {
        M2_PlayerSetUnlocks(kPlayerInfo, 0x1C, uint32_t(self[0]));
    }
    uint8_t* script = M2_LurFindScript(kLurStore, At<const char*>(GlobalData(self[0]), 0x1C));
    if (script != nullptr) {
        M2_ScriptStop5D9050(script);
    }
}

// 0x6ABFF0: the mission's script ended
void __fastcall Slot_StopScript(int32_t* self) {
    uint8_t* script = M2_LurFindScript(kLurStore, At<const char*>(GlobalData(self[0]), 0x1C));
    At<uint8_t>(script, 0x1150) = 0;
    M2_ScriptEnd(script, 0);
}

// 0x6AC030: its MissionCleanup run
void __fastcall Slot_Cleanup(int32_t* self) {
    uint8_t* script = M2_LurLaunch(kLurStore, At<const char*>(GlobalData(self[0]), 0x1C), 1);
    M2_ScriptThread(script, "MissionCleanup");
    At<uint8_t>(script, 0x1150) = 1;
}

// 0x6AC080: its main run (the fire alarm stopped), its controller's "Run" node played
void __fastcall Slot_Run(int32_t* self) {
    if (self[1] == 0) {
        return;
    }
    if (g_fireAlarmOn != 0) {
        M2_FireAlarmStop(kFireAlarm);
    }
    uint8_t* script = M2_LurLaunch(kLurStore, At<const char*>(GlobalData(self[0]), 0x1C), 1);
    M2_ScriptThread(script, "main");
    M2_ControllerPlayNode(reinterpret_cast<void*>(self[1]), "Run");
}

// 0x6AC0F0: may the running mission be left: 2 nothing runs (or it can't be: the
// arcade's 2_G2), 1 yes, 0 asked (the exit popup shown when `ask`); the popup's answer
// once up
int __fastcall MissionMgr_ExitPrompt(uint8_t* self, re::Edx, int ask) {
    int32_t* slot = &At<int32_t>(self, 0x4BC);
    bool running = false;
    if (slot[0] != -1 && slot[2] != 0 && slot[2] != 0xB) {
        running = true;
        if (At<uint8_t>(self, 0x4DA) != 0) {
            uint32_t key;
            M2_NameKey(&key, "2_G2");
            running = M2_IsRunning(kMissionMgr, M2_Find(kMissionMgr, &key)) != 0;
            M2_NameKeyDestruct(&key);
        }
    }
    if (!running) {
        return 2;
    }
    if (At<uint8_t>(self, 0x52C) == 0 && At<uint8_t>(self, 0x52D) == 0) {
        if (uint8_t(ask) != 0) {
            M2_PopupShow(self + 0x4DC, 1, reinterpret_cast<const char*>(0x00941D04),
                         reinterpret_cast<const char*>(0x00941D14), 7, 8);  // ("EXIT_MISSION", "EXIT_BUTTONS")
            At<uint8_t>(self, 0x52D) = 1;
            return 0;
        }
        return 1;
    }
    return M2_PopupResult(self + 0x4DC);
}

// 0x6AC1E0: a slot runs a gang mission ("_G" in its script's name)
uint8_t __fastcall MissionMgr_InGangMission(uint8_t* self) {
    char name[0x80];
    for (int i = 0; i < 2; ++i) {
        int32_t mission = At<int32_t>(self, 0x4BC + i * 0xE4);
        if (mission < 0) {
            continue;
        }
        char* file = reinterpret_cast<char*>(M2_ScriptName(self, mission, name, 0x80));
        if (file != nullptr && M2_Strstr(file, reinterpret_cast<const char*>(0x00941D98)) != nullptr) {
            return 1;
        }
    }
    return 0;
}

// 0x6AC270: the tutorials' controllers stepped by the frontend's clock (20 .. 100 ms),
// the player's action inputs too (while paused)
void __fastcall MissionMgr_UpdateWhilePaused(uint8_t* self) {
    uint32_t now = g_frontendTime;
    float elapsed = float(int32_t(now - At<uint32_t>(self, 0x760)));
    float clamped = elapsed < 20.0f ? 20.0f : (elapsed > 100.0f ? 100.0f : elapsed);
    int32_t ms = Ftol(clamped);
    At<uint32_t>(self, 0x760) = now;
    float dt = float(ms) / 1000.0f;
    for (int i = 0; i < 100; ++i) {
        int32_t* controller = Controller(self, i);
        int32_t mission = controller[2];
        if (mission != -1 && At<int32_t>(Data(self, mission), 0x28) == 4) {
            M2_ControllerUpdate(controller, dt);
        }
    }
    M2_InputsUpdate(At<void*>(At<uint8_t*>(self, 0x694), 0x2E0), dt);
}

// 0x6AC350: a controller stopped as missed: its "Missed" node (unless a class already
// counted; a class marks the classes running), then stopped
void __fastcall Controller_StopMissed(int32_t* self) {
    if ((g_missionRecords[self[2] * 6 + 4] & 0x10) == 0) {
        M2_ControllerPlayNode(self, "Missed");
        if (At<int32_t>(GlobalData(self[2]), 0x28) == 1) {
            for (int i = 0; i < 100; ++i) {
                int32_t mission = At<int32_t>(kMissionMgr, 0x10 + i * 0xC);
                if (mission != -1 && At<int32_t>(GlobalData(mission), 0x28) == 1) {
                    g_missionRecords[mission * 6 + 4] |= 0x10;
                }
            }
        }
    }
    g_missionRecords[self[2] * 6 + 4] &= 0xFE;
    void* a = reinterpret_cast<void*>(self[0]);
    MakeCurrent(a);
    M2_ControllerReset(a);
    void* b = reinterpret_cast<void*>(self[1]);
    MakeCurrent(b);
    M2_ControllerReset(b);
    self[2] = -1;
}

// 0x6AC520: the constructor
uint8_t* __fastcall MissionMgr_Construct(uint8_t* self) {
    At<uint32_t>(self, 0) = 0x00941D50;
    for (int i = 0; i < 100; ++i) {
        int32_t* controller = Controller(self, i);
        controller[0] = 0;
        controller[1] = 0;
        controller[2] = -1;
    }
    for (int i = 0; i < 2; ++i) {
        uint8_t* slot = self + 0x4BC + i * 0xE4;
        M2_PopupInit(slot + 0x20);
        At<int32_t>(slot, 0) = -1;
        At<int32_t>(slot, 4) = 0;
        At<int32_t>(slot, 8) = 0;
        for (int b = 0xC; b <= 0x16; ++b) {
            slot[b] = 0;
        }
        At<int32_t>(slot, 0x18) = -1;
        slot[0x1C] = 0;
        slot[0x1D] = 0;
        slot[0x1E] = 0;
        slot[0x70] = 0;
        slot[0x80] = 0;
        At<int32_t>(slot, 0x84) = -1;
        At<int32_t>(slot, 0x88) = 0;
        At<int32_t>(slot, 0x8C) = -1;
        At<int32_t>(slot, 0xE0) = 0;
    }
    for (int offset : {0x68C, 0x690, 0x694, 0x698, 0x69C, 0x6A0, 0x6A4, 0x6AC, 0x6C0, 0x6C4, 0x6CC, 0x6D0, 0x6DC,
                       0x6E0, 0x6E4, 0x6E8, 0x6EC, 0x6F0, 0x6F4, 0x6F8, 0x6FC, 0x704, 0x718, 0x71C, 0x724, 0x728,
                       0x734, 0x738}) {
        At<int32_t>(self, offset) = 0;
    }
    for (int offset : {0x6A8, 0x6A9, 0x6C8, 0x700, 0x701, 0x720, 0x77D, 0x740}) {
        At<uint8_t>(self, offset) = 0;
    }
    At<int32_t>(self, 0x6D4) = -1;
    At<int32_t>(self, 0x72C) = -1;
    for (int offset : {0x6B0, 0x6B4, 0x6B8, 0x6BC, 0x708, 0x70C, 0x710, 0x714, 0x76C, 0x770, 0x774}) {
        At<float>(self, offset) = 0.0f;
    }
    At<int32_t>(self, 0x748) = 0;
    M2_Reset(self);
    return self;
}

// 0x6AC710 (vf0): the deleting destructor
uint8_t* __fastcall MissionMgr_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    M2_Destruct(self);
    if (flags & 1) {
        M2_Free(self);
    }
    return self;
}

// 0x6AC730 (vf1): Init: the missions from the action tree (the node at `path`, else the
// file loaded), their records, tracks and data, the story's count, the player's
// controller, the periods
void __fastcall MissionMgr_Init(uint8_t* self, re::Edx, const char* path, const char* file) {
    if (At<uint8_t>(self, 0x740) != 0) {
        return;
    }
    int32_t& count = At<int32_t>(self, 0x744);
    count = 0;
    At<int32_t>(self, 0x754) = 0;
    At<uint8_t*>(self, 0x688) = nullptr;
    At<int32_t>(self, 0x748) = 0;
    At<uint8_t>(self, 0x74C) = 0;
    At<uint8_t*>(self, 0x4B8) = nullptr;
    At<int32_t>(self, 0x750) = 0;
    At<uint8_t*>(self, 4) = M2_ActTreeFind(path, nullptr);
    if (At<uint8_t*>(self, 4) == nullptr) {
        At<uint8_t*>(self, 4) = M2_ActFileLoad(file, 0, 0);
    }
    if (At<uint8_t*>(self, 4) == nullptr) {
        return;
    }
    re::Ptr32<void> list[1000];
    count = 0;
    M2_Collect(self, At<void*>(self, 4), list, &count);
    auto size = [](uint32_t n, uint32_t each) {
        uint64_t bytes = uint64_t(n) * each;
        return (bytes >> 32) != 0 ? 0xFFFFFFFFu : uint32_t(bytes);
    };
    uint32_t n = uint32_t(count);
    void* records = M2_Malloc(size(n, 6));
    if (records != nullptr) {
        M2_VectorConstruct(records, 6, int(n), reinterpret_cast<void*>(0x006A9DA0));
    }
    At<void*>(self, 0x688) = records;
    At<void*>(self, 0x684) = M2_Malloc(size(uint32_t(count), 4));
    n = uint32_t(count);
    uint32_t dataBytes = size(n, 0x4C);
    uint32_t total = dataBytes + 4;
    if (total < dataBytes) {
        total = 0xFFFFFFFF;
    }
    uint8_t* block = static_cast<uint8_t*>(M2_Malloc(total));
    uint8_t* data = nullptr;
    if (block != nullptr) {
        data = block + 4;
        At<uint32_t>(block, 0) = n;
        M2_VectorConstruct(data, 0x4C, int(n), reinterpret_cast<void*>(0x006322D0));
    }
    At<uint8_t*>(self, 0x4B8) = data;
    for (int i = 0; i < count; ++i) {
        uint8_t* entry = At<uint8_t*>(self, 0x4B8) + i * 0x4C;
        void* desc = VCall<void*>(list[i], 0x48, 0);
        M2_TrackDescApply(desc, entry);
        re::PtrCast<re::Ptr32<void>*>(At<uint8_t*>(self, 0x684))[i] = list[i];
        uint8_t* d = At<uint8_t*>(self, 0x4B8) + i * 0x4C;
        int32_t kind = At<int32_t>(d, 0x28);
        if ((kind == 0 || kind == 3) && At<uint8_t>(d, 0x25) == 0 && At<uint8_t>(d, 0x4A) != 0) {
            At<int32_t>(self, 0x754) += 1;
        }
    }
    re::Ptr32<uint8_t>& ped = At<uint8_t*>(self, 0x694);
    if (ped != nullptr) {
        M2_CleanUpOldReference(ped, &ped);
        ped = nullptr;
    }
    At<int32_t>(self, 0x68C) = 0;
    re::Ptr32<uint8_t>& other = At<uint8_t*>(self, 0x6EC);
    if (other != nullptr) {
        M2_CleanUpOldReference(other, &other);
        other = nullptr;
    }
    M2_PushMemoryTag(0x26);
    void* memory = M2_Malloc(0x38);
    At<void*>(self, 0x73C) = memory != nullptr ? M2_ControllerConstruct(memory) : nullptr;
    M2_PopMemoryTag();
    At<void*>(At<void*>(self, 0x73C), 0x18) = self + 0x6E4;
    At<void*>(self, 0x6E4) = At<void*>(self, 0x73C);
    At<uint8_t>(self, 0x740) = At<void*>(self, 0x688) != nullptr && At<void*>(self, 0x4B8) != nullptr;
    M2_PeriodsLoad(kPeriods);
}

// 0x6AC990 (vf2): Shutdown: the periods' callback off, the controllers stopped (missed)
// and deleted, the arrays freed
void __fastcall MissionMgr_Shutdown(uint8_t* self) {
    if (At<uint8_t>(self, 0x740) == 0) {
        return;
    }
    M2_PeriodsShutdown(kPeriods);
    for (int i = 0; i < 100; ++i) {
        int32_t* controller = Controller(self, i);
        if (controller[2] != -1) {
            M2_ControllerMissed(controller);
        }
        for (int k = 0; k < 2; ++k) {
            void* c = reinterpret_cast<void*>(controller[k]);
            if (c != nullptr) {
                M2_ControllerDestruct(c);
                M2_Free(c);
                controller[k] = 0;
            }
        }
    }
    M2_Free(At<void*>(self, 0x688));
    uint8_t* data = At<uint8_t*>(self, 0x4B8);
    At<void*>(self, 0x688) = nullptr;
    At<int32_t>(self, 0x744) = 0;
    if (data != nullptr) {
        if (At<int32_t>(data, -4) != 0) {
            VCall<void>(data, 4, 3);
        } else {
            M2_Free(data - 4);
        }
    }
    At<void*>(self, 0x4B8) = nullptr;
    M2_Free(At<void*>(self, 0x684));
    At<void*>(self, 0x684) = nullptr;
}

// 0x6ACA70: a slot's mission ended: the player and the world set back, its controller's
// "Succeed" (counted) or "Fail", its dictionary removed
void __fastcall Slot_End(int32_t* self) {
    if (self[1] == 0) {
        return;
    }
    uint8_t* ped = g_playerPed;
    g_playerFlags &= 0xF7;
    if (ped != nullptr) {
        ResetPlayerState(true);
    }
    ClearCameraReference();
    M2_LurStore5DAF40(kLurStore);
    if (At<uint8_t>(self, 0xC) != 0) {
        if (M2_Events59FF80(kEvents, self[0])) {
            uint8_t out[0x30];
            M2_EventsAdd(kEvents, out, g_playerPed, 0, 0, 0x167, 0, 0, 0, 0, -1, 0);
        }
        At<int32_t>(self, 0x84) = self[0];
        At<uint16_t>(g_missionRecords, self[0] * 6 + 2) += 1;
        if (At<uint16_t>(g_missionRecords, self[0] * 6 + 2) == 1) {
            g_missionsPassed += 1;
        }
        M2_ControllerPlayNode(reinterpret_cast<void*>(self[1]), "Succeed");
    } else {
        At<int32_t>(self, 0x84) = -1;
        M2_ControllerPlayNode(reinterpret_cast<void*>(self[1]), "Fail");
    }
    g_cameraC3CE0C = 1;
    g_cameraC3CED8 = 0;
    M2_CameraSetShot(kCamera, 1, 0, 1, 0, 0);
    ClearCameraReference();
    M2_PlayerSetPaused(kPlayerInfo, 0);
    M2_InputSetEnabled(kUserInput, 1);
    VCall<void>(g_hudObject, 0x1C);
    M2_MissionTimerStop(kMissionTimer, 0);
    g_pedProduction = 0;
    M2_Stats59A600(kMissionStats, self[0]);
    M2_Object46FA20(g_objectC2AF9C);
    g_ambientTransitions = 1;
    M2_PopulationRevert(g_population);
    M2_TrafficRevert(g_traffic);
    At<uint8_t>(g_systemC0F5E4, 0xA4) = 1;
    At<uint8_t>(g_population, 0x7898) |= 0x10;
    M2_PopulationClearDisabled(g_population);
    At<uint8_t>(g_population, 0x7898) &= 0xFE;
    g_classes[0] = 0;
    g_classes[1] = 0;
    M2_RespawnReset(kGameLogic);
    if (self == g_slot0) {
        M2_ObjectivesReset(kObjectives);
    }
    g_missionFlag20C43ED = 0;
    g_cameraC3CF82 = 0;
    if (At<int32_t>(self, 0x8C) >= 0) {
        M2_SlotReleaseTextures(self);
        M2_RemoveTxdSlot(At<int32_t>(self, 0x8C));
        At<int32_t>(self, 0x8C) = -1;
    }
    At<uint8_t>(g_radar, 0xE844) = 1;
    At<int32_t>(self, 0xE0) = 0;
    At<uint8_t>(self, 0x1E) = 0;
    M2_Fn5BB390();
}

// 0x6ACD00: a slot's mission set up: its text, its dictionary (TXD/Missions/<name>.NFT)
// and textures, the world cleared for it, its MissionSetup run (waited on up to 5 times)
void __fastcall Slot_Setup(int32_t* self) {
    if (self[1] == 0) {
        return;
    }
    char name[0x40];
    M2_ScriptName(kMissionMgr, self[0], name, 0x40);
    if (At<int32_t>(GlobalData(self[0]), 0x28) == 5) {
        M2_TextLoadMinigame(kTheText, name);
    } else {
        M2_TextLoadMission(kTheText, name);
    }
    if (At<int32_t>(self, 0x8C) >= 0) {
        M2_RemoveTxdSlot(At<int32_t>(self, 0x8C));
    }
    if (At<uint8_t>(GlobalData(self[0]), 0x49) != 0) {
        char path[0x80];
        M2_Sprintf(path, "TXD/Missions/%s.NFT", name);
        int slot = M2_AddTxdSlot(name);
        At<int32_t>(self, 0x8C) = slot;
        if (M2_LoadTxd(slot, path)) {
            M2_AddRef(At<int32_t>(self, 0x8C));
        }
        if (At<int32_t>(self, 0x8C) >= 0) {
            M2_SlotLoadTextures(self);
        }
    }
    M2_SpeechStop(kSpeech);
    M2_Object46FA20(g_objectC2AF9C);
    uint8_t* data = GlobalData(self[0]);
    if (At<uint8_t>(data, 0x36) == 0 && g_playerPed != nullptr) {
        M2_AlliesDismiss(g_playerPed + 0x2F0);
    }
    ResetPlayerState(false);
    ClearCameraReference();
    if (At<uint8_t>(data, 0x34) == 0) {
        M2_ClearArea(g_zeroVector, 4000.0f, 1);
    }
    At<uint8_t>(self, 0x11) = 0;
    At<uint8_t>(self, 0x13) = 0;
    At<uint8_t>(self, 0x12) = 0;
    uint8_t* script = M2_LurLaunch(kLurStore, At<const char*>(data, 0x1C), 0);
    At<uint8_t>(script, 0x1150) = 0;
    M2_ScriptThread(script, "MissionSetup");
    int tries = 5;
    while (M2_ScriptRunning(script, "MissionSetup")) {
        int left = tries;
        tries -= 1;
        if (left <= 0) {
            break;
        }
        M2_GenerateBoundingBoxes(0);
        M2_LurStoreUpdate(kLurStore, 0);
    }
    At<uint8_t>(self, 0xF) = 0;
    At<uint8_t>(self, 0x14) = 0;
    At<uint8_t>(self, 0x10) = 0;
    At<int32_t>(self, 0x84) = -1;
    M2_Stats59A490(kMissionStats, self[0]);
}

// 0x6ACF60: the player (the two references at +0x6EC and +0x694)
void __fastcall MissionMgr_SetPlayer(uint8_t* self, re::Edx, uint8_t* ped) {
    for (int offset : {0x6EC, 0x694}) {
        re::Ptr32<uint8_t>& reference = At<uint8_t*>(self, offset);
        if (ped != reference) {
            if (reference != nullptr) {
                M2_CleanUpOldReference(reference, &reference);
            }
            reference = ped;
            if (ped != nullptr) {
                M2_RegisterReference(ped, &reference);
            }
        }
    }
}

// 0x6ACFD0: a mission started in a slot (its controller running it): attempted once more,
// marked a class, the fade out (unless +0x34)
void __fastcall Slot_Start(int32_t* self, re::Edx, int32_t mission) {
    if (mission < 0 || mission >= g_missionCount || self[1] != 0) {
        return;
    }
    int32_t controller = M2_FindController(kMissionMgr, mission);
    if (controller == -1) {
        return;
    }
    char name[0x40];
    M2_ScriptName(kMissionMgr, mission, name, 0x40);
    self[0] = mission;
    At<int32_t>(self, 0x84) = -1;
    At<uint16_t>(g_missionRecords, mission * 6) += 1;
    g_missionRecords[mission * 6 + 4] |= 0x10;
    if (At<int32_t>(GlobalData(self[0]), 0x28) == 1) {
        M2_MarkClasses(kMissionMgr);
    }
    self[1] = int32_t(0x020C3CA8 + controller * 0xC);
    alignas(4) uint8_t data[0x4C];  // (a CMissionData copy)
    M2_MissionDataCopy(data, GlobalData(self[0]));
    self[2] = 1;
    At<uint8_t>(self, 0xD) = 0;
    if (At<uint8_t>(data, 0x34) == 0 && M2_CameraFadeState(kCamera) != 1) {
        M2_FadeOut();
    }
    At<uint8_t>(self, 0x80) = 0;
    g_playerFlags &= 0xF7;
    M2_MissionDataDestruct(data);
}

// 0x6AD100: a mission started: a class or kind 5 while something runs in slot 0 asks
// (or abandons it), else kind 5 goes in slot 1 (unless slot 0 is starting / passing),
// the rest in slot 0
void __fastcall MissionMgr_Start(uint8_t* self, re::Edx, int32_t mission, int abandon) {
    uint8_t* radar = g_radar;
    uint8_t* data = Data(self, mission);
    int32_t kind = At<int32_t>(data, 0x28);
    if (kind == 5 || kind == 1) {
        int32_t* slot = &At<int32_t>(self, 0x4BC);
        bool ask = false;
        if (SlotRunning(slot)) {
            bool check = true;
            if (At<uint8_t>(self, 0x4DA) != 0 && kind != 1) {
                check = false;
                if (M2_IsArcade(self, mission)) {
                    uint32_t key;
                    M2_NameKey(&key, "2_G2");
                    check = M2_IsRunning(self, M2_Find(kMissionMgr, &key)) != 0;
                    M2_NameKeyDestruct(&key);
                }
            }
            if (check && slot[0] != mission && At<uint8_t>(Data(self, slot[0]), 0x33) == 0) {
                ask = true;
            }
        }
        if (ask) {
            if (uint8_t(abandon) != 0) {
                M2_SlotAbandon(slot, mission);
            } else {
                M2_SlotRequest(slot, mission);
            }
            return;
        }
    }
    At<int32_t>(radar, 0xE82C) = 0;
    if (At<uint8_t>(data, 0x48) != 0) {
        M2_Radar548F80(radar, 9, 1);
    }
    if (At<int32_t>(data, 0x28) == 5) {
        int32_t* slot0 = &At<int32_t>(self, 0x4BC);
        if (slot0[0] != -1 && (slot0[2] == 1 || slot0[2] == 8)) {
            return;
        }
        M2_SlotStart(&At<int32_t>(self, 0x5A0), mission);
        return;
    }
    M2_SlotStart(&At<int32_t>(self, 0x4BC), mission);
}

// 0x6AD290: three missions (all: every one) looked at in turn: their controllers started
// when their tracks want (or flag 2), the story ones auto-started, the rest missed
void __fastcall MissionMgr_UpdateMissions(uint8_t* self, re::Edx, int all) {
    float next = float(At<int32_t>(self, 0x750) + 3);
    float count = float(At<int32_t>(self, 0x744));
    int32_t end = Ftol(count > next ? next : count);
    if (uint8_t(all) != 0) {
        At<int32_t>(self, 0x750) = 0;
        end = At<int32_t>(self, 0x744);
    }
    int32_t* slot0 = &At<int32_t>(self, 0x4BC);
    for (int32_t i = At<int32_t>(self, 0x750); i < end; ++i) {
        uint8_t* data = At<uint8_t*>(self, 0x4B8) + i * 0x4C;
        void* track = re::PtrCast<re::Ptr32<void>*>(At<uint8_t*>(self, 0x684))[i];
        bool wanted = false;
        if (At<uint16_t>(At<uint8_t*>(self, 0x688), i * 6 + 2) == 0 || At<uint8_t>(data, 0x2C) != 0) {
            wanted = VCall<int32_t>(track, 0x4C, self + 0x68C) != 0;
        }
        uint8_t flags = At<uint8_t*>(self, 0x688)[i * 6 + 4];
        if (wanted || (flags & 2) != 0) {
            uint8_t* record = At<uint8_t*>(self, 0x688) + i * 6;
            if ((record[4] & 1) == 0) {
                int32_t free = M2_FindController(self, -1);
                if (free == -1) {
                    continue;
                }
                if (At<int32_t>(At<uint8_t*>(self, 0x4B8), i * 0x4C + 0x28) == 4) {
                    record[4] |= 2;
                }
                M2_ControllerStart(Controller(self, free), i);
                At<uint8_t*>(self, 0x688)[i * 6 + 4] |= 1;
                At<uint8_t*>(self, 0x688)[i * 6 + 4] &= 0xEF;
                if (!SlotRunning(slot0) && At<uint8_t>(At<uint8_t*>(self, 0x4B8), i * 0x4C + 0x33) != 0 &&
                    !M2_Area415380(g_areaCounter)) {
                    M2_Start(self, i, 0);
                }
                uint8_t* missions = At<uint8_t*>(self, 0x4B8);
                if (At<int32_t>(missions, i * 0x4C + 0x28) == 1 && SlotRunning(slot0) &&
                    At<int32_t>(missions, slot0[0] * 0x4C + 0x28) == 1) {
                    At<uint8_t*>(self, 0x688)[i * 6 + 4] |= 0x10;
                }
            } else if (!SlotRunning(slot0) && At<uint8_t>(At<uint8_t*>(self, 0x4B8), i * 0x4C + 0x33) != 0 &&
                       !M2_Area415380(g_areaCounter) && At<uint8_t>(self, 0x77D) != 0) {
                M2_Start(self, i, 0);
            }
        } else if ((flags & 1) != 0) {
            int32_t* slot1 = &At<int32_t>(self, 0x5A0);
            bool in0 = slot0[0] == i && slot0[2] != 0 && slot0[2] != 0xB;
            bool in1 = slot1[0] == i && slot1[2] != 0 && slot1[2] != 0xB;
            if (!in0 && !in1) {
                M2_ControllerMissed(Controller(self, M2_FindController(self, i)));
            }
        }
    }
    At<int32_t>(self, 0x750) = end >= At<int32_t>(self, 0x744) ? 0 : end;
}

// 0x6AD540: a slot's state machine (the exit popup's answer first)
void __fastcall Slot_Update(int32_t* self) {
    uint8_t* s = reinterpret_cast<uint8_t*>(self);
    float spot[3] = {0.0f, 0.0f, 0.0f};
    if (s[0x1D] != 0 && s[0x1C] == 0) {
        if (s[0x70] != 0 && At<int32_t>(s, 0x20) == 1) {
            if (M2_PopupResult(s + 0x20) != 2) {
                return;
            }
            s[0x1C] = 1;
            s[0x1D] = 0;
            s[0x16] = 0;
            if (self[1] == 0) {
                return;
            }
            s[0xF] = s[0xF] != 0;
            s[0x14] = s[0x14] != 0;
            s[0x10] = s[0x10] != 0;
            if (self[2] == 4) {
                s[0xC] = 0;
                s[0xE] = 0;
                s[0x13] = 0;
                self[2] = 0xC;
            }
            return;
        }
        s[0x1C] = 0;
        s[0x1D] = 0;
        s[0x16] = 0;
        M2_InputSet50DFD0(kUserInput, 1);
        LeaveInvulnerableIdle();
        return;
    }
    if (s[0x16] != 0 || g_menuAllowed20C87AC != 0) {
        return;
    }
    switch (self[2]) {
    case 1: {
        uint8_t* data = M2_Data(kMissionMgr, self[0]);
        if (!M2_NotFading()) {
            return;
        }
        if (M2_CameraFadeState(kCamera) == 1) {
            float current = float(g_music_CF3AE8);
            float wanted = float(uint32_t(g_timeInMs + 500));
            g_music_CF3AE8 = Ftol(M2_Max(wanted, current));
            M2_Music58FD80();
            M2_FadeWithCamera(4, 1);
            M2_FadeWithCamera(3, 1);
            M2_FadeWithCamera(2, 1);
            M2_FadeWithCamera(1, 1);
        }
        self[2] = 2;
        M2_SlotSetup(self);
        ShowTitle(self, data);
        return;
    }
    case 2: {
        uint8_t* data = M2_Data(kMissionMgr, self[0]);
        ShowTitle(self, data);
        if (At<uint8_t>(data, 0x2E) != 0 && At<uint8_t>(data, 0x26) != 0) {
            M2_Fn4261A0();
        }
        M2_Fn425F80();
        uint8_t* script = M2_LurFindScript(kLurStore, At<const char*>(data, 0x1C));
        if (script != nullptr && M2_ScriptRunning(script, "MissionSetup")) {
            return;
        }
        LeaveInvulnerableIdle();
        if (At<uint8_t>(data, 0x35) == 0 && At<uint8_t>(data, 0x34) == 0 && s[0x11] == 0) {
            self[2] = 3;
            M2_FadeIn();
            s[0x11] = 0;
            return;
        }
        M2_SlotRun(self);
        s[0x11] = 0;
        self[2] = 4;
        return;
    }
    case 3: {
        uint8_t* data = M2_Data(kMissionMgr, self[0]);
        ShowTitle(self, data);
        if (At<uint8_t>(data, 0x35) == 0 && At<uint8_t>(data, 0x34) == 0 && !M2_NotFading()) {
            return;
        }
        M2_SlotRun(self);
        self[2] = 4;
        return;
    }
    case 7:
        if (!M2_NotFading()) {
            return;
        }
        M2_SlotRun(self);
        self[2] = 4;
        return;
    case 5: {
        if (!M2_NotFading() || Held()) {
            return;
        }
        uint8_t* script = M2_LurFindScript(kLurStore, At<const char*>(M2_Data(kMissionMgr, self[0]), 0x1C));
        if (script != nullptr) {
            M2_ScriptStop5D9050(script);
        }
        M2_SlotEnd(self);
        uint8_t knockedOut = s[0x10];
        uint8_t busted = s[0xF];
        if (self[1] != 0) {
            s[0xF] = (busted != 0 || s[0x14] != 0) ? 1 : 0;
            s[0x14] = (s[0x14] != 0 || knockedOut != 0) ? 1 : 0;
            s[0x10] = knockedOut != 0;
            if (self[2] == 4) {
                s[0xC] = 0;
                self[2] = 0xC;
                s[0xE] = busted;
                s[0x13] = 0;
            }
        }
        self[2] = 6;
        g_slotsChanged = 1;
        return;
    }
    case 6:
        if (M2_PadValue(7, 0) > 0.0f) {
            s[0xD] = 1;
            M2_RestorePlayer(kGameLogic, 1, spot);
            M2_SlotSetup(self);
            M2_FadeIn();
            self[2] = 7;
            return;
        }
        if (M2_PadValue(8, 0) > 0.0f) {
            s[0xD] = 0;
            M2_RestorePlayer(kGameLogic, 0, spot);
            M2_FadeIn();
            self[2] = 0xB;
        }
        return;
    case 8: {
        if (Held()) {
            return;
        }
        if ((s[0x15] != 0 || s[0x13] == 0) && !M2_NotFading()) {
            return;
        }
        uint8_t* script = M2_LurFindScript(kLurStore, At<const char*>(M2_Data(kMissionMgr, self[0]), 0x1C));
        if (script != nullptr) {
            M2_ScriptStop5D9050(script);
        }
        At<int32_t>(s, 0x88) = 0;
        M2_SlotStopScript(self);
        self[2] = 9;
        return;
    }
    case 9: {
        uint8_t* script = M2_LurFindScript(kLurStore, At<const char*>(M2_Data(kMissionMgr, self[0]), 0x1C));
        if (script != nullptr && M2_ScriptRunning(script, "main")) {
            At<int32_t>(s, 0x88) += 1;
            return;
        }
        At<int32_t>(s, 0x88) = 0;
        M2_SlotCleanup(self);
        self[2] = 0xA;
        return;
    }
    case 0xA: {
        uint8_t* data = M2_Data(kMissionMgr, self[0]);
        if (self == g_slot0 || (!M2_SlotIsRunning(g_slot0) && self == g_slot1)) {
            M2_SoundsReset(kAudio);
        }
        uint8_t* script = M2_LurFindScript(kLurStore, At<const char*>(data, 0x1C));
        if (script != nullptr && M2_ScriptRunning(script, "MissionCleanup")) {
            At<int32_t>(s, 0x88) += 1;
            return;
        }
        M2_SlotEnd(self);
        if (s[0x15] == 0) {
            if (s[0xC] == 0 && At<uint8_t>(data, 0x2D) == 0) {
                M2_Logic4314F0(kGameLogic, s[0xF], s[0x14], s[0x10]);
                if (s[0xF] != 0 || s[0x14] != 0 || s[0x10] != 0) {
                    M2_RestorePlayer(kGameLogic, At<uint8_t>(M2_Data(kMissionMgr, self[0]), 0x33), spot);
                }
            }
            if (At<uint8_t>(data, 0x2E) != 0) {
                if (s[0xC] == 0 && At<int32_t>(data, 0x28) != 1) {
                    M2_Fn425FB0();
                } else {
                    if (At<uint8_t>(data, 0x30) != 0) {
                        M2_SetClock(At<uint8_t>(data, 0x31), At<uint8_t>(data, 0x32));
                    }
                    if (At<int32_t>(data, 0x38) > 0) {
                        M2_AddMinutes(uint32_t(At<int32_t>(data, 0x38)));
                    }
                    if (At<uint8_t>(data, 0x2F) != 0) {
                        M2_SkipToNextPeriod();
                    }
                }
            }
            if (s[0x12] == 0 && s[0xF] == 0 && s[0x14] == 0 && s[0x10] == 0 && s[0x1C] == 0) {
                M2_FadeIn();
            }
        }
        int32_t mission = self[0];
        self[2] = 0xB;
        self[1] = 0;
        s[0xD] = 0;
        s[0x13] = 0;
        s[0x15] = 0;
        s[0x12] = 0;
        if ((g_missionRecords[mission * 6 + 4] & 2) != 0) {
            g_missionRecords[mission * 6 + 4] &= 0xFD;
            g_missionRecords[self[0] * 6 + 4] &= 0xFB;
        }
        g_slotsChanged = 1;
        s[0x1D] = 0;
        if (s[0x1C] != 0) {
            self[2] = 0;
            self[0] = -1;
            s[0x1E] = 0;
            self[1] = 0;
            s[0x1C] = 0;
            M2_InputSet50DFD0(kUserInput, 1);
            M2_Start(kMissionMgr, self[6], 0);
        }
        return;
    }
    case 0xB: {
        uint8_t* data = M2_Data(kMissionMgr, self[0]);
        if (At<uint8_t>(data, 0x34) == 0 && !M2_NotFading()) {
            return;
        }
        self[2] = 0;
        self[0] = -1;
        s[0x1E] = 0;
        self[1] = 0;
        return;
    }
    case 0xC: {
        if (Held()) {
            return;
        }
        if (s[0x13] == 0) {
            M2_FadeOut();
        }
        if (At<uint8_t>(M2_Data(kMissionMgr, self[0]), 0x2D) != 0) {
            g_fadeColourR = 0;
            g_fadeColourG = 0;
            g_fadeColourB = 0;
            g_fadeColourA = 0x80;
            self[2] = 5;
            return;
        }
        self[2] = 8;
        return;
    }
    default:
        return;
    }
}

// 0x6ADDD0 (vf3): each frame: the periods, the objectives (when nothing's in the way),
// three missions looked at, the controllers stepped (more than 3 tutorials: +0x764), the
// slots, everything again when asked (+0x77C), the player's controller
void __fastcall MissionMgr_Update(uint8_t* self) {
    At<uint8_t>(self, 0x74C) = 0;
    M2_PeriodsUpdate(kPeriods);
    if (!Held() && g_menuFlag20C8708 == 0 && At<uint8_t>(self, 0x52C) == 0 && At<uint8_t>(self, 0x610) == 0 &&
        M2_Camera43A590(kCamera) == 0 && g_flag20C5BE6 == 0) {
        M2_Objectives6AE5E0(kObjectives);
    }
    M2_UpdateMissions(self, 0);
    float dt = g_controllerStepScale * g_timeStep;
    int tutorials = 0;
    for (int i = 0; i < 100; ++i) {
        int32_t* controller = Controller(self, i);
        M2_ControllerUpdate(controller, dt);
        int32_t mission = controller[2];
        if (mission >= 0 && At<int32_t>(Data(self, mission), 0x28) == 4) {
            ++tutorials;
        }
    }
    if (tutorials > 3) {
        At<uint8_t>(self, 0x764) = 1;
    }
    M2_SlotUpdate(&At<int32_t>(self, 0x5A0));
    M2_SlotUpdate(&At<int32_t>(self, 0x4BC));
    if (At<uint8_t>(self, 0x77C) != 0) {
        M2_UpdateMissions(self, 1);
        At<uint8_t>(self, 0x77C) = 0;
    }
    void* controller = At<void*>(self, 0x73C);
    if (At<void*>(controller, 0x10) != nullptr) {
        M2_ControllerTick(controller, dt);
    }
}

} // namespace msnmgr2

#define MSNMGR2_REVERSED(name, address, fn)                                                          \
    static void* RE_CONCAT(msnmgr2_original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(msnmgr2_original_, __LINE__), \
             re::Mode::Original)

MSNMGR2_REVERSED("CMissionMgr::~CMissionMgr", 0x006AB6F0, &msnmgr2::MissionMgr_Destruct);
MSNMGR2_REVERSED("CMissionMgr::Reset", 0x006AB7B0, &msnmgr2::MissionMgr_Reset);
MSNMGR2_REVERSED("CMissionMgr::Collect", 0x006AB940, &msnmgr2::MissionMgr_Collect);
MSNMGR2_REVERSED("CMissionMgr::DrawRestartPrompt", 0x006ABAC0, &msnmgr2::MissionMgr_DrawRestartPrompt);
MSNMGR2_REVERSED("CMissionMgr::ScriptName", 0x006ABB90, &msnmgr2::MissionMgr_ScriptName);
MSNMGR2_REVERSED("CMissionMgr::Save", 0x006ABC40, &msnmgr2::MissionMgr_Save);
MSNMGR2_REVERSED("CMissionMgr::Load", 0x006ABD50, &msnmgr2::MissionMgr_Load);
MSNMGR2_REVERSED("CMissionSlot::DrawPopup", 0x006ABF20, &msnmgr2::Slot_DrawPopup);
MSNMGR2_REVERSED("CMissionSlot::Pass", 0x006ABF40, &msnmgr2::Slot_Pass);
MSNMGR2_REVERSED("CMissionSlot::StopScript", 0x006ABFF0, &msnmgr2::Slot_StopScript);
MSNMGR2_REVERSED("CMissionSlot::Cleanup", 0x006AC030, &msnmgr2::Slot_Cleanup);
MSNMGR2_REVERSED("CMissionSlot::Run", 0x006AC080, &msnmgr2::Slot_Run);
MSNMGR2_REVERSED("CMissionMgr::ExitPrompt", 0x006AC0F0, &msnmgr2::MissionMgr_ExitPrompt);
MSNMGR2_REVERSED("CMissionMgr::InGangMission", 0x006AC1E0, &msnmgr2::MissionMgr_InGangMission);
MSNMGR2_REVERSED("CMissionMgr::UpdateWhilePaused", 0x006AC270, &msnmgr2::MissionMgr_UpdateWhilePaused);
MSNMGR2_REVERSED("CMissionController::StopMissed", 0x006AC350, &msnmgr2::Controller_StopMissed);
MSNMGR2_REVERSED("CMissionMgr::CMissionMgr", 0x006AC520, &msnmgr2::MissionMgr_Construct);
MSNMGR2_REVERSED("CMissionMgr::DeletingDestruct", 0x006AC710, &msnmgr2::MissionMgr_DeletingDestruct);
MSNMGR2_REVERSED("CMissionMgr::Init", 0x006AC730, &msnmgr2::MissionMgr_Init);
MSNMGR2_REVERSED("CMissionMgr::Shutdown", 0x006AC990, &msnmgr2::MissionMgr_Shutdown);
MSNMGR2_REVERSED("CMissionSlot::End", 0x006ACA70, &msnmgr2::Slot_End);
MSNMGR2_REVERSED("CMissionSlot::Setup", 0x006ACD00, &msnmgr2::Slot_Setup);
MSNMGR2_REVERSED("CMissionMgr::SetPlayer", 0x006ACF60, &msnmgr2::MissionMgr_SetPlayer);
MSNMGR2_REVERSED("CMissionSlot::Start", 0x006ACFD0, &msnmgr2::Slot_Start);
MSNMGR2_REVERSED("CMissionMgr::Start", 0x006AD100, &msnmgr2::MissionMgr_Start);
MSNMGR2_REVERSED("CMissionMgr::UpdateMissions", 0x006AD290, &msnmgr2::MissionMgr_UpdateMissions);
MSNMGR2_REVERSED("CMissionSlot::Update", 0x006AD540, &msnmgr2::Slot_Update);
MSNMGR2_REVERSED("CMissionMgr::Update", 0x006ADDD0, &msnmgr2::MissionMgr_Update);
