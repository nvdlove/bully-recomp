// Script natives, 0x5C0000-0x5CFFFF (missions, objectives, photos, newspapers, prop
// anims, objects, peds): `int __cdecl Native_<ScriptName>(lua_State*)`, returning how
// many values they pushed.
//
// They reach Lua through the game's own helpers (ScriptApi.h: 0-based argument indices,
// a float lua_Number).
//
// The ped fields have no CPed struct yet: they are read at their offsets (At<T>), with
// what the natives say they are.

#include <cstddef>
#include <cstdint>

#include "game/ModelInfo.h"
#include "game/Pools.h"
#include "game/ScriptApi.h"
#include "reversed.h"

namespace {

// ---- script helpers ----
// 0x5C7380: a ped by script handle; kind 3 is the player (the handle unused), 2 a pool
// ped whose handle (vf 0xAC) still matches. nullptr for -1 or a stale handle
GAME_FN(uint8_t*, __cdecl, Script_GetPed, 0x005C7380, int handle, int kind);

// The ped the first argument's handle names (kind 2)
uint8_t* PedArg(lua_State* L) {
    return Script_GetPed(Script::GetInt(L, 0), 2);
}

// A field of a struct not written up yet
using re::At;

// A virtual function (thiscall) by vtable index
template <typename R, typename... A>
R CallVirtual(void* object, int index, A... args) {
    using Fn = R(__thiscall*)(void*, re::Arg<A>...);
    return re::GameVirtual<Fn>(object, index)(object, args...);
}

// ---- missions (CMissionMgr at 0x20C3CA0) ----
inline void* const missionMgr = reinterpret_cast<void*>(0x020C3CA0);
GAME_FN(uint8_t*, __thiscall, MissionMgr_GetCurrent, 0x006AA710, void* mgr);          // the running mission's state
GAME_FN(uint8_t, __thiscall, MissionState_IsActive, 0x006AA3F0, uint8_t* state);
GAME_FN(uint8_t*, __thiscall, MissionMgr_GetInfo, 0x006AA660, void* mgr, int id);      // +0x18 its name
GAME_FN(int, __thiscall, MissionMgr_GetCount, 0x006AA140, void* mgr);
GAME_FN(uint8_t, __thiscall, MissionMgr_InClass, 0x006AA840, void* mgr);
GAME_FN(void, __thiscall, MissionMgr_SetAutoRestart, 0x006AB010, void* mgr, uint8_t on);
GAME_FN(int, __thiscall, MissionMgr_AskUserToStart, 0x006AC0F0, void* mgr, int ask);
GAME_FN(void, __thiscall, GiftRequirements_Reset, 0x006A99D0, void* self);
inline void* const giftRequirements = reinterpret_cast<void*>(0x020C3698);
// the respawn overrides (0xBD1078)
inline void* const respawnOverrides = reinterpret_cast<void*>(0x00BD1078);
GAME_FN(void, __thiscall, RespawnOverrides_Reset, 0x004315B0, void* self);
// the ped type manager
GAME_VAR(void*, pedTypeMgr, 0x00C2C108);
GAME_FN(void, __thiscall, PedTypeMgr_ClearDisabled, 0x0049C610, void* self);
GAME_FN(void, __thiscall, PedTypeMgr_SetDisabled, 0x0049C5F0, void* self, int type, uint8_t disabled);
// the ped type attitudes
GAME_VAR(void*, attitudes, 0x00C2AF9C);
GAME_FN(void, __thiscall, Attitudes_ResetToDefault, 0x0046F4A0, void* self);
// the mission timer (0xBF4508)
inline void* const missionTimer = reinterpret_cast<void*>(0x00BF4508);
GAME_FN(void, __thiscall, MissionTimer_Stop, 0x0043F750, void* self, int);
GAME_FN(void, __thiscall, MissionTimer_Pause, 0x0043F6B0, void* self, uint8_t paused);
// the objectives (0x20C4420)
inline void* const objectives = reinterpret_cast<void*>(0x020C4420);
GAME_FN(void, __thiscall, Objectives_Complete, 0x006AE450, void* self, uint8_t id, int);
GAME_FN(void, __thiscall, Objectives_Remove, 0x006AE480, void* self, uint8_t id);
GAME_FN(void, __thiscall, Objectives_SetReminderTime, 0x006AE580, void* self, float time);
// the player's info (GTA's CPlayerInfo): faction respect, mini objectives
inline void* const playerInfo = reinterpret_cast<void*>(0x00C1AEA8);   // +0 the player's ped
GAME_FN(int, __thiscall, PlayerInfo_GetRespect, 0x00446780, void* self, int faction);
GAME_FN(uint8_t, __thiscall, PlayerInfo_GetMiniObjectiveComplete, 0x00446860, void* self, int id);
// the mission's action controller
GAME_VAR(void*, missionActionController, 0x020C43DC);
GAME_FN(void, __thiscall, ActionController_Stop, 0x005F40A0, void* self);
GAME_FN(void, __thiscall, ActionController_Update, 0x005F4B00, void* self, float dt);
// Lua callbacks kept by the game (cdecl: the state, the argument, where the ref goes)
GAME_FN(void, __cdecl, Script_SetCallback, 0x005DA060, lua_State* L, int index, void* slot);
GAME_FN(void, __cdecl, Script_SetCallback2, 0x005D9FD0, lua_State* L, int index, void* slot);

GAME_FN(int, __cdecl, PedRequestModelImpl, 0x005C6340, lua_State* L);
GAME_FN(void, __cdecl, Stimulus_RemoveBroadcast, 0x004A62E0, int id);
GAME_FN(void, __cdecl, Projectile_Destroy, 0x004D3F10, int id, int);
inline void* const world = reinterpret_cast<void*>(0x00A147E0);   // +0 the chapter
GAME_FN(void, __thiscall, World_SetSeasonalModels, 0x00454540, void* self, int season);
GAME_FN(void, __cdecl, Script_PedMakeAmbient, 0x005CA910, lua_State* L, int handle, int freeResources);
inline void* const fires = reinterpret_cast<void*>(0x00BD0CF8);
GAME_FN(void, __thiscall, Fires_Destroy, 0x0042D880, void* self, int id);
GAME_FN(int, __cdecl, Photo_GetEntity, 0x005C1680, int start);
GAME_FN(int, __cdecl, ModelInfo_GetIndexByName, 0x0051C330, const char* name);
GAME_VAR(uint8_t*, photo, 0x00C674E0);
// the script's prop anims (0x20C7E98)
inline void* const propAnims = reinterpret_cast<void*>(0x020C7E98);
GAME_FN(uint8_t*, __thiscall, PropAnims_Get, 0x006D7B10, void* self, int id);
// 0x5C4570: the prop anim the arguments from `first` name: an id, a light userdata, a
// model and a position... `*next` gets the argument after them
GAME_FN(uint8_t*, __cdecl, Script_GetPropAnimArg, 0x005C4570, lua_State* L, int first, int* next);
GAME_FN(void, __thiscall, Attitudes_SetTypeToType, 0x0046F8B0, void* self, int from, int to, int attitude);
GAME_FN(void, __thiscall, Attitudes_SetDefaultTypeToType, 0x0046F8F0, void* self, int from, int to, int attitude);
GAME_FN(int*, __thiscall, Attitudes_GetTypeToType, 0x0046F830, void* self, int* out, int from, int to);
GAME_FN(void, __thiscall, Fires_SetScale, 0x0042D800, void* self, int id, float scale);
GAME_FN(void, __thiscall, Fires_SetDamageRadius, 0x0042D7E0, void* self, int id, float radius);
GAME_FN(int16_t, __thiscall, PedModelInfo_GetUniqueStatus, 0x005228D0, void* info);
GAME_FN(void, __thiscall, Ped_ClearPOI, 0x00473250, void* ped, uint8_t);
GAME_FN(float, __thiscall, MissionTimer_GetTimeRemaining, 0x0043F6F0, void* self);
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);
// 0x5C28D0: an entity by script handle and kind (0 object, 1 vehicle, 2 ped, 3 the
// player); a ped's vehicle instead unless `pedItself`
GAME_FN(uint8_t*, __cdecl, Script_GetEntity, 0x005C28D0, int handle, int kind, uint8_t pedItself, uint8_t cutscene);
GAME_FN(uint8_t*, __cdecl, Pools_GetObject, 0x0044A5B0, int handle);   // CPools::GetObjectFromRef
GAME_FN(void, __thiscall, PlayerInfo_SetMiniObjectiveComplete, 0x004467F0, void* self, int id, uint8_t complete);
GAME_FN(int, __thiscall, PedTypeMgr_GetRandomModel, 0x0049C230, void* self, int type, int, int);
inline void* const pathMgr = reinterpret_cast<void*>(0x00D13730);
GAME_FN(uint8_t*, __thiscall, PathMgr_FindForEntity, 0x005EB9A0, void* self, void* entity);
GAME_FN(uint8_t*, __cdecl, WeaponInfo_Get, 0x0051B450, int model);
GAME_FN(void, __thiscall, RespawnOverrides_SetArrestPoint, 0x00431530, void* self, const ScriptVector* pos, float heading, int area);
GAME_FN(void, __thiscall, RespawnOverrides_SetKOPoint, 0x00431570, void* self, const ScriptVector* pos, float heading, int area);
GAME_VAR(uint8_t*, combatantDefaults, 0x020C889C);   // +0x18..+0x20 the ped's default combatant counts
GAME_VAR(uint8_t*, controllerPed0, 0x00C2B120);
GAME_VAR(uint8_t*, controllerPed1, 0x00C2B124);
GAME_FN(void, __cdecl, MemoryHistory_Dump, 0x0049A650);
GAME_FN(void, __cdecl, E3Dodgeball_CleanObjects1, 0x00444BB0);
GAME_FN(void, __cdecl, E3Dodgeball_CleanObjects2, 0x004D4A90);
// the running scripts (0xD02850)
inline void* const scriptMgr = reinterpret_cast<void*>(0x00D02850);
GAME_FN(void*, __thiscall, ScriptMgr_GetCurrent, 0x005DB940, void* self);
GAME_FN(void, __thiscall, Script_DeleteMissionPeds, 0x005D9750, void* script);
GAME_FN(void, __thiscall, HealthBar_Set, 0x005537E0, void* self, uintptr_t, uintptr_t);
GAME_FN(uint8_t, __thiscall, MissionMgr_IsRunning, 0x006AA6A0, void* mgr);
GAME_FN(uint8_t*, __thiscall, MissionMgr_GetStats, 0x006AA7C0, void* mgr, int id);   // +0 attempts, +2 successes (16 bits), +4 flags
GAME_FN(int, __thiscall, MissionMgr_GetNextClass, 0x006AA030, void* mgr);
GAME_FN(void, __thiscall, MissionMgr_Start, 0x006AD100, void* mgr, int id, uint8_t);
GAME_FN(void, __thiscall, PropAnim_Reset, 0x004EF570, void* prop);
GAME_FN(void, __thiscall, PropAnim_CloseDoor, 0x004EEEC0, void* prop, uintptr_t);
GAME_FN(void, __thiscall, PropAnim_SetFlag, 0x004EA450, void* prop, int flag, int on);
GAME_FN(void, __thiscall, PropAnim_OpenDoor, 0x004EED10, void* prop, int, uintptr_t, int immediate);
GAME_FN(void, __thiscall, PathMgr_SetClearWhenDone, 0x005EBE30, void* self, void* entity, int);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void, __thiscall, World_SetChapter, 0x004545C0, void* self, int chapter);
GAME_VAR(void*, chapterHud, 0x00C674CC);
GAME_FN(void, __thiscall, ChapterHud_Update, 0x0054AD80, void* self);
GAME_FN(void, __thiscall, Ped_ResetAttitudes, 0x004731F0, void* ped);
GAME_FN(void, __thiscall, Ped_WarpOutOfCar, 0x0049A3F0, void* ped);
GAME_FN(void, __thiscall, PedAllies_DismissAll, 0x00485A40, void* allies);             // ped +0x2F0
GAME_FN(void, __thiscall, PedSocial_Stop, 0x004910A0, void* social);                   // ped +0xB44
GAME_FN(void, __thiscall, WeaponInventory_SaveSnapshot, 0x0045CBE0, void* inventory);    // ped +0x1C4
GAME_FN(void, __thiscall, WeaponInventory_RestoreSnapshot, 0x0045D200, void* inventory);
GAME_FN(void, __thiscall, WeaponInventory_DeleteSnapshot, 0x0045C9B0, void* inventory);
GAME_FN(float, __thiscall, Fires_GetHealth, 0x0042D840, void* self, int id);
GAME_FN(void, __thiscall, MissionTimer_Start, 0x0043F690, void* self, uint16_t seconds);
GAME_FN(void, __thiscall, PedTether_Clear, 0x00499A40, void* tether);                  // ped +0x358
GAME_FN(void, __thiscall, PedTether_ClearAnti, 0x00499B70, void* tether);
GAME_FN(void, __thiscall, PedObjectives_RemoveCurrent, 0x004702C0, void* objectives);  // ped +0x5F0
GAME_FN(void, __thiscall, PedModelInfo_SetPickupListOverride, 0x00520530, void* info, const char* list);
GAME_FN(void, __cdecl, Streaming_SetModelNotNeededAmbient, 0x0052FE40, int model);
GAME_FN(void, __cdecl, Streaming_SetPedModelNotNeededAmbient, 0x004D98E0, int model);
GAME_FN(void, __thiscall, PedFocus_Clear, 0x0048C4C0, void* focus, uintptr_t);              // ped +0x3BC
GAME_FN(void, __thiscall, POI_Release, 0x004475D0, void* poi, void* ped);
GAME_FN(void, __thiscall, Ped_SetPunishmentPoints, 0x00477520, void* ped, int points);
GAME_FN(void, __thiscall, Ped_SetSpecial, 0x00474350, void* ped, int special);
GAME_FN(void, __thiscall, Ped_SetSpecialRate, 0x00474340, void* ped, float rate);
GAME_FN(uint8_t*, __thiscall, Ped_GetLastHitWeapon, 0x00474BE0, void* ped);   // +0x5E its model
GAME_FN(float, __thiscall, PropAnim_GetHealth, 0x004EDD80, void* prop);
GAME_FN(void, __thiscall, Ped_SetMoney, 0x004752A0, void* ped, int money);
// 0x5C6280: argument i (0-4, default -1) as a tether speed
GAME_FN(float, __cdecl, Script_GetTetherSpeed, 0x005C6280, lua_State* L, int i);
GAME_FN(void, __thiscall, Ped_SetEffectedByGravity, 0x00474C10, void* ped, uint8_t on, int);
GAME_FN(void, __thiscall, PedTether_SetTrigger, 0x00499B30, void* tether, int trigger);
GAME_FN(void, __thiscall, PedTether_SetAntiTrigger, 0x00499C40, void* tether, int trigger);
GAME_FN(void*, __cdecl, ActionTree_Find, 0x005F5B10, const char* name, uintptr_t);
GAME_FN(void, __thiscall, ActionController_Play, 0x005F4A50, void* self, void* node);
GAME_FN(void, __thiscall, Ped_OverrideStat, 0x004770F0, void* ped, int stat, int value);
GAME_FN(void, __thiscall, Ped_SetPedToTypeAttitude, 0x004731D0, void* ped, int type, int attitude);
GAME_FN(uint8_t, __thiscall, Ped_IsOnScreen, 0x00467460, void* ped);
GAME_FN(void, __thiscall, Object_Break, 0x004D0450, void* object, uint8_t);
GAME_FN(uint8_t*, __thiscall, PedObjectives_GetCurrent, 0x00470340, void* objectives);   // +8 its type
GAME_FN(int, __cdecl, Pools_GetVehicleRef, 0x0044CAC0, void* vehicle);   // CPools::GetVehicleRef
GAME_FN(uint8_t*, __thiscall, Ped_GetModelInfo, 0x00477DC0, void* ped);   // +0x50 female
GAME_FN(void, __thiscall, PedCombatZones_Set, 0x00486260, void* zones, int zone, uint8_t on);   // ped +0x668
GAME_FN(void, __thiscall, Ped_SetAlpha, 0x004732A0, void* ped, int alpha, uint8_t);
GAME_FN(int, __thiscall, WeaponInventory_GetAmmo, 0x0045CF40, void* inventory, int model);
GAME_FN(void, __thiscall, WeaponInventory_Remove, 0x0045D0C0, void* inventory, int model, int);
GAME_FN(void, __cdecl, Stimulus_AddBroadcast, 0x004A6220, int id, float);
GAME_FN(uint8_t, __thiscall, Conversations_IsInOne, 0x00683F40, void* self, void* ped);
GAME_FN(void, __cdecl, Script_SetPedPos, 0x005C7600, void* ped, const ScriptVector* pos);
GAME_FN(uint8_t, __cdecl, Ped_IsSpotted, 0x00602F20, void* ped, int by);
GAME_FN(uint8_t, __thiscall, ActionController_IsPlaying, 0x005F3D50, void* self, const char* node, uint8_t);

GAME_FN(uint8_t, __thiscall, Photo_IsSetValid, 0x00545290, void* self);
inline void* const conversations = reinterpret_cast<void*>(0x00B9DAB8);
GAME_FN(void, __thiscall, Conversations_SetMovePeds, 0x00683EB0, void* self, uint8_t on);
inline void* const memoryMgr = reinterpret_cast<void*>(0x00D14170);
GAME_FN(void, __thiscall, MemoryMgr_Check, 0x005EEF90, void* self);

GAME_VAR(uint8_t*, newspaper, 0x00C674BC);   // +0x4C max papers, +0x50 delivered, +0x64 paper visible, +0x65 throw
GAME_VAR(uint8_t*, healthBar, 0x00C674D0);   // the HUD's general health bar
GAME_VAR(uint8_t*, areaTransitions, 0x009E4F4C);
GAME_VAR(CPool<uint8_t>*, scriptPropAnimPool, 0x020C7C48);   // the script's handles: prop anims, triggers (+0x1C the entity)

GAME_VAR(int32_t, currentDay, 0x020C3640);
GAME_VAR(uint8_t, chapter, 0x00A147E0);
GAME_VAR(int32_t, totalMissionSuccessCount, 0x020C43E8);
GAME_VAR(uint8_t, gameRestarted, 0x00CFB35C);
GAME_VAR(uint8_t, lockFps30, 0x00A135DB);
GAME_VAR(uint8_t, allowAmbientTransitions, 0x00C2B3A0);
GAME_VAR(uint8_t, globalSleep, 0x00C2B108);
GAME_VAR(uint8_t, socialSubtitles, 0x00A33938);
GAME_VAR(uint8_t, globalAttitudeRumble, 0x00C2AFA0);
GAME_VAR(uint8_t, globalAttitudeRumble2, 0x00C2AFA1);
GAME_VAR(uint8_t, globalAttitudeIgnoreTruants, 0x00C2AFA2);
GAME_VAR(int32_t, photoEntityStart, 0x00CFB354);
GAME_VAR(int32_t, photoEntity358, 0x00CFB358);
GAME_VAR(int32_t, photoEntity370, 0x00CFB370);
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);    // CTimer's
GAME_ARRAY(uint8_t, objectiveCompleteCallback, 0x00C1B174);
GAME_ARRAY(uint8_t, bballCollisionCallback, 0x01B89178);

} // namespace

#define NB_REVERSED(name, address)                     \
    static void* RE_CONCAT(Original_, __LINE__);       \
    REVERSED("Native_" #name, address, RE_BODY(&Native_##name), nullptr, \
             &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

// ---- missions ----------------------------------------------------------------------

// (with no mission running it pushes nil but says it pushed nothing)
int __cdecl Native_MissionGetCurrentName(lua_State* L) {
    if (!MissionState_IsActive(MissionMgr_GetCurrent(missionMgr))) {
        Script::PushNil(L);
        return 0;
    }
    int id = *reinterpret_cast<int*>(MissionMgr_GetCurrent(missionMgr));
    Script::PushString(L, At<const char*>(MissionMgr_GetInfo(missionMgr, id), 0x18));
    return 1;
}
NB_REVERSED(MissionGetCurrentName, 0x005C0000);

int __cdecl Native_GetMissionCount(lua_State* L) {
    Script::PushInt(L, MissionMgr_GetCount(missionMgr));
    return 1;
}
NB_REVERSED(GetMissionCount, 0x005C0380);

int __cdecl Native_IsMissionFromRestart(lua_State* L) {
    Script::PushBool(L, MissionMgr_GetCurrent(missionMgr)[0xD]);
    return 1;
}
NB_REVERSED(IsMissionFromRestart, 0x005C0440);

int __cdecl Native_GetTotalMissionSuccessCount(lua_State* L) {
    Script::PushInt(L, totalMissionSuccessCount);
    return 1;
}
NB_REVERSED(GetTotalMissionSuccessCount, 0x005C05F0);

int __cdecl Native_MissionDontFadeIn(lua_State*) {
    MissionMgr_GetCurrent(missionMgr)[0x11] = 1;
    return 0;
}
NB_REVERSED(MissionDontFadeIn, 0x005C08F0);

int __cdecl Native_MissionDontFadeInAfterCompetion(lua_State*) {
    MissionMgr_GetCurrent(missionMgr)[0x12] = 1;
    return 0;
}
NB_REVERSED(MissionDontFadeInAfterCompetion, 0x005C0910);

int __cdecl Native_MissionStopActionController(lua_State* L) {
    ActionController_Stop(missionActionController);
    Script::PushBool(L, 1);
    return 1;
}
NB_REVERSED(MissionStopActionController, 0x005C0980);

int __cdecl Native_MissionUpdateActionController(lua_State* L) {
    ActionController_Update(missionActionController, 0.0f);
    Script::PushBool(L, 1);
    return 1;
}
NB_REVERSED(MissionUpdateActionController, 0x005C09A0);

// (reading it clears it)
int __cdecl Native_GameRestarted(lua_State* L) {
    Script::PushBool(L, gameRestarted);
    gameRestarted = 0;
    return 1;
}
NB_REVERSED(GameRestarted, 0x005C09D0);

int __cdecl Native_ChapterGet(lua_State* L) {
    Script::PushInt(L, chapter);
    return 1;
}
NB_REVERSED(ChapterGet, 0x005C0D00);

int __cdecl Native_AllowAreaTransitions(lua_State* L) {
    uint8_t allow = Script::GetBool(L, 0);
    areaTransitions[0x85C] = allow;
    return 0;
}
NB_REVERSED(AllowAreaTransitions, 0x005C0D40);

// the day of the week (0-4) when the argument is true, else the day count
int __cdecl Native_GetCurrentDay(lua_State* L) {
    uint8_t weekday = Script::GetBool(L, 0);
    int day = currentDay;
    if (weekday) {
        day %= 5;
    }
    Script::PushInt(L, day);
    return 1;
}
NB_REVERSED(GetCurrentDay, 0x005C0D60);

int __cdecl Native_SetCurrentDay(lua_State* L) {
    int day = Script::GetInt(L, 0);
    currentDay = day;
    if (day > 4 || day < 0) {
        currentDay = 0;
    }
    return 0;
}
NB_REVERSED(SetCurrentDay, 0x005C0DA0);

int __cdecl Native_DestroyProjectile(lua_State* L) {
    Projectile_Destroy(Script::GetInt(L, 0), 0);
    return 0;
}
NB_REVERSED(DestroyProjectile, 0x005C0D20);

int __cdecl Native_ResetGiftRequirements(lua_State*) {
    GiftRequirements_Reset(giftRequirements);
    return 0;
}
NB_REVERSED(ResetGiftRequirements, 0x005C0F80);

int __cdecl Native_LockFPS30(lua_State* L) {
    lockFps30 = Script::GetBool(L, 0);
    return 0;
}
NB_REVERSED(LockFPS30, 0x005C0F90);

int __cdecl Native_MissionSetAutoRestart(lua_State* L) {
    MissionMgr_SetAutoRestart(missionMgr, Script::GetBool(L, 0));
    return 0;
}
NB_REVERSED(MissionSetAutoRestart, 0x005C0FB0);

int __cdecl Native_MissionAllowAmbientTransitions(lua_State* L) {
    allowAmbientTransitions = Script::GetBool(L, 0);
    return 0;
}
NB_REVERSED(MissionAllowAmbientTransitions, 0x005C1070);

int __cdecl Native_MissionInClass(lua_State* L) {
    Script::PushBool(L, MissionMgr_InClass(missionMgr));
    return 1;
}
NB_REVERSED(MissionInClass, 0x005C1110);

int __cdecl Native_MissionDisablePedType(lua_State* L) {
    int type = Script::GetInt(L, 0);
    void* mgr = pedTypeMgr;
    uint8_t disabled = Script::GetBool(L, 1);
    PedTypeMgr_SetDisabled(mgr, type, disabled);
    return 0;
}
NB_REVERSED(MissionDisablePedType, 0x005C1130);

int __cdecl Native_MissionClearDisablePedTypes(lua_State*) {
    PedTypeMgr_ClearDisabled(pedTypeMgr);
    return 0;
}
NB_REVERSED(MissionClearDisablePedTypes, 0x005C1170);

int __cdecl Native_MissionResetRespawnOverrides(lua_State*) {
    RespawnOverrides_Reset(respawnOverrides);
    return 0;
}
NB_REVERSED(MissionResetRespawnOverrides, 0x005C1280);

int __cdecl Native_GetFactionRespect(lua_State* L) {
    Script::PushInt(L, PlayerInfo_GetRespect(playerInfo, Script::GetInt(L, 0)));
    return 1;
}
NB_REVERSED(GetFactionRespect, 0x005C1310);

// true when the prompt is up (0 from the mission manager)
int __cdecl Native_MissionAskUserToStart(lua_State* L) {
    Script::PushBool(L, MissionMgr_AskUserToStart(missionMgr, 1) == 0);
    return 1;
}
NB_REVERSED(MissionAskUserToStart, 0x005C1340);

int __cdecl Native_MiniObjectiveOnCompletedCallback(lua_State* L) {
    Script_SetCallback(L, 0, objectiveCompleteCallback);
    return 0;
}
NB_REVERSED(MiniObjectiveOnCompletedCallback, 0x005C13C0);

int __cdecl Native_MiniObjectiveGetIsComplete(lua_State* L) {
    Script::PushBool(L, PlayerInfo_GetMiniObjectiveComplete(playerInfo, Script::GetInt(L, 0)));
    return 1;
}
NB_REVERSED(MiniObjectiveGetIsComplete, 0x005C1430);

int __cdecl Native_MissionObjectiveComplete(lua_State* L) {
    Objectives_Complete(objectives, uint8_t(Script::GetInt(L, 0)), 1);
    return 0;
}
NB_REVERSED(MissionObjectiveComplete, 0x005C2090);

int __cdecl Native_MissionObjectiveRemove(lua_State* L) {
    Objectives_Remove(objectives, uint8_t(Script::GetInt(L, 0)));
    return 0;
}
NB_REVERSED(MissionObjectiveRemove, 0x005C20C0);

int __cdecl Native_MissionObjectiveReminderTime(lua_State* L) {
    Objectives_SetReminderTime(objectives, Script::GetFloat(L, 0));
    return 0;
}
NB_REVERSED(MissionObjectiveReminderTime, 0x005C20F0);

int __cdecl Native_MissionTimerPause(lua_State* L) {
    MissionTimer_Pause(missionTimer, Script::GetBool(L, 0));
    return 0;
}
NB_REVERSED(MissionTimerPause, 0x005C21F0);

int __cdecl Native_MissionTimerStop(lua_State*) {
    MissionTimer_Stop(missionTimer, 0);
    return 0;
}
NB_REVERSED(MissionTimerStop, 0x005C2220);

// ---- fires, photos -----------------------------------------------------------------

int __cdecl Native_FireDestroy(lua_State* L) {
    int id = Script::GetInt(L, 0);
    if (id >= 0) {
        Fires_Destroy(fires, id);
    }
    return 0;
}
NB_REVERSED(FireDestroy, 0x005C0B30);

// two results: taken (cleared by reading it), and +0x34
int __cdecl Native_PhotoHasBeenTaken(lua_State* L) {
    uint8_t* p = photo;
    uint8_t taken = p[0x40];
    uint8_t valid = p[0x34];
    p[0x40] = 0;
    Script::PushBool(L, taken);
    Script::PushBool(L, valid);
    return 2;
}
NB_REVERSED(PhotoHasBeenTaken, 0x005C0BB0);

int __cdecl Native_PhotoShowExitString(lua_State* L) {
    uint8_t show = Script::GetBool(L, 0);
    photo[0x20] = show;
    return 0;
}
NB_REVERSED(PhotoShowExitString, 0x005C0C00);

int __cdecl Native_PhotoIsSetValid(lua_State* L) {
    Script::PushBool(L, Photo_IsSetValid(photo));
    return 1;
}
NB_REVERSED(PhotoIsSetValid, 0x005C0CA0);

int __cdecl Native_PhotoGetEntityStart(lua_State*) {
    photoEntity370 = 0;
    photoEntity358 = 0;
    photoEntityStart = Photo_GetEntity(0);
    return 0;
}
NB_REVERSED(PhotoGetEntityStart, 0x005C1980);

// ---- weapons LOD -------------------------------------------------------------------

int __cdecl Native_GetWeaponLOD(lua_State* L) {
    int model = Script::GetInt(L, 0);
    Script::PushFloat(L, re::PtrCast<CWeaponModelInfo*>(CModelInfo::ms_modelInfoPtrs[model])->unknown50);
    return 1;
}
NB_REVERSED(GetWeaponLOD, 0x005C1B30);

// (the game reads argument 0 for the distance too: the model's number as a float)
int __cdecl Native_SetWeaponLOD(lua_State* L) {
    int model = Script::GetInt(L, 0);
    float distance = Script::GetFloat(L, 0);
    re::PtrCast<CWeaponModelInfo*>(CModelInfo::ms_modelInfoPtrs[model])->unknown50 = distance;
    return 0;
}
NB_REVERSED(SetWeaponLOD, 0x005C1B60);

// ---- newspapers --------------------------------------------------------------------

int __cdecl Native_NewspaperMakeHUDVisible(lua_State* L) {
    uint8_t* hud = newspaper;
    CallVirtual<void>(hud, 5, int(Script::GetBool(L, 0)));
    return 0;
}
NB_REVERSED(NewspaperMakeHUDVisible, 0x005C22D0);

int __cdecl Native_NewspaperSetPaperVisible(lua_State* L) {
    uint8_t* hud = newspaper;
    hud[0x64] = Script::GetBool(L, 0);
    return 0;
}
NB_REVERSED(NewspaperSetPaperVisible, 0x005C2300);

int __cdecl Native_NewspaperThrowPaper(lua_State*) {
    newspaper[0x65] = 1;
    return 0;
}
NB_REVERSED(NewspaperThrowPaper, 0x005C2320);

int __cdecl Native_NewspaperSetMaxPapers(lua_State* L) {
    uint8_t* hud = newspaper;
    At<int32_t>(hud, 0x4C) = Script::GetInt(L, 0);
    return 0;
}
NB_REVERSED(NewspaperSetMaxPapers, 0x005C2330);

int __cdecl Native_NewspaperSetDeliveredPapers(lua_State* L) {
    uint8_t* hud = newspaper;
    At<int32_t>(hud, 0x50) = Script::GetInt(L, 0);
    return 0;
}
NB_REVERSED(NewspaperSetDeliveredPapers, 0x005C2350);

int __cdecl Native_NewspaperAddDeliverdPaper(lua_State*) {
    At<int32_t>(newspaper, 0x50) += 1;
    return 0;
}
NB_REVERSED(NewspaperAddDeliverdPaper, 0x005C2370);

int __cdecl Native_NewspaperGetMaxPapers(lua_State* L) {
    Script::PushInt(L, At<int32_t>(newspaper, 0x4C));
    return 1;
}
NB_REVERSED(NewspaperGetMaxPapers, 0x005C2380);

int __cdecl Native_NewspaperGetNumDeliveredPapers(lua_State* L) {
    Script::PushInt(L, At<int32_t>(newspaper, 0x50));
    return 1;
}
NB_REVERSED(NewspaperGetNumDeliveredPapers, 0x005C23A0);

// ---- objects -----------------------------------------------------------------------

// (also LawnMowingSetLawnAreaP2: a no-op that reads its argument)
int __cdecl Native_ObjectDontCleanup(lua_State* L) {
    Script::GetInt(L, 0);
    return 0;
}
NB_REVERSED(ObjectDontCleanup, 0x005C2420);

int __cdecl Native_ObjectGetModelIndex(lua_State* L) {
    Script::PushInt(L, ModelInfo_GetIndexByName(Script::GetString(L, 0)));
    return 1;
}
NB_REVERSED(ObjectGetModelIndex, 0x005C2440);

int __cdecl Native_SetBBallCollisionCallback(lua_State* L) {
    Script_SetCallback2(L, 0, bballCollisionCallback);
    return 0;
}
NB_REVERSED(SetBBallCollisionCallback, 0x005C2470);

// ---- prop anims --------------------------------------------------------------------

int __cdecl Native_PAnimExists(lua_State* L) {
    uint8_t exists = 0;
    int slot = Script::GetInt(L, 0);
    if (slot >= 0) {
        uint8_t* handle = scriptPropAnimPool->GetAt(slot);
        if (handle) {
            uint8_t* entity = At<uint8_t*>(handle, 0x1C);
            if (entity) {
                uint8_t* p = At<uint8_t*>(entity, 0x114);
                if (p && At<int32_t>(p, 0x94) != 0) {
                    exists = 1;
                }
            }
        }
    }
    Script::PushBool(L, exists);
    return 1;
}
NB_REVERSED(PAnimExists, 0x005C4A20);

int __cdecl Native_PAnimEnableStreaming(lua_State* L) {
    int slot = Script::GetInt(L, 0);
    if (slot >= 0) {
        uint8_t* handle = scriptPropAnimPool->GetAt(slot);
        if (handle) {
            uint8_t on = Script::GetBool(L, 1);
            handle[0x3F] = uint8_t((handle[0x3F] & ~0x10) | ((on << 4) & 0x10));
        }
    }
    return 0;
}
NB_REVERSED(PAnimEnableStreaming, 0x005C5820);

// ---- peds: globals -----------------------------------------------------------------

int __cdecl Native_PedRequestModel(lua_State* L) {
    Script::GetInt(L, 0);
    return PedRequestModelImpl(L);
}
NB_REVERSED(PedRequestModel, 0x005C6480);

int __cdecl Native_ConversationMovePeds(lua_State* L) {
    Conversations_SetMovePeds(conversations, Script::GetBool(L, 0));
    return 0;
}
NB_REVERSED(ConversationMovePeds, 0x005C68A0);

int __cdecl Native_PedShowHealthBarInFPmode(lua_State* L) {
    uint8_t* bar = healthBar;
    bar[0xBC] = Script::GetBool(L, 0);
    return 0;
}
NB_REVERSED(PedShowHealthBarInFPmode, 0x005C6900);

int __cdecl Native_PedSetHealthBarQuiet(lua_State* L) {
    uint8_t quiet = 1;
    if (Script::GetArgCount(L) >= 1) {
        quiet = Script::GetBool(L, 0);
    }
    healthBar[0xBD] = quiet;
    return 0;
}
NB_REVERSED(PedSetHealthBarQuiet, 0x005C6920);

int __cdecl Native_HideGeneralHealthBar(lua_State*) {
    uint8_t* bar = healthBar;
    bar[0x174] = 0;
    CallVirtual<void>(bar, 5, 0);
    return 0;
}
NB_REVERSED(HideGeneralHealthBar, 0x005C6970);

int __cdecl Native_SetGeneralHealthBar(lua_State* L) {
    uint8_t* bar = healthBar;
    At<float>(bar, 0x17C) = float(Script::GetInt(L, 0));
    return 0;
}
NB_REVERSED(SetGeneralHealthBar, 0x005C6990);

int __cdecl Native_PedResetTypeAttitudesToDefault(lua_State*) {
    Attitudes_ResetToDefault(attitudes);
    return 0;
}
NB_REVERSED(PedResetTypeAttitudesToDefault, 0x005C6CD0);

int __cdecl Native_PedSetGlobalAttitude_Rumble(lua_State* L) {
    uint8_t a = Script::GetBoolOpt(L, 0, 0);
    uint8_t b = Script::GetBoolOpt(L, 1, 0);
    globalAttitudeRumble = a;
    globalAttitudeRumble2 = b;
    return 0;
}
NB_REVERSED(PedSetGlobalAttitude_Rumble, 0x005C6D30);

int __cdecl Native_PedSetGlobalAttitude_IgnoreTruants(lua_State* L) {
    globalAttitudeIgnoreTruants = Script::GetBoolOpt(L, 0, 0);
    return 0;
}
NB_REVERSED(PedSetGlobalAttitude_IgnoreTruants, 0x005C6D60);

// (reads its name twice around the check, for a debug build's messages)
int __cdecl Native_DebugMemCheck(lua_State* L) {
    Script::GetString(L, 0);
    MemoryMgr_Check(memoryMgr);
    Script::GetString(L, 0);
    return 0;
}
NB_REVERSED(DebugMemCheck, 0x005C6E60);

int __cdecl Native_PedRemoveBroadcastStimulus(lua_State* L) {
    Stimulus_RemoveBroadcast(Script::GetInt(L, 0));
    return 0;
}
NB_REVERSED(PedRemoveBroadcastStimulus, 0x005C6F80);

int __cdecl Native_PedSetGlobalSleep(lua_State* L) {
    globalSleep = Script::GetBool(L, 0);
    return 0;
}
NB_REVERSED(PedSetGlobalSleep, 0x005C6FA0);

int __cdecl Native_PedSetSocialSubtitlesEnable(lua_State* L) {
    socialSubtitles = Script::GetBool(L, 0);
    return 0;
}
NB_REVERSED(PedSetSocialSubtitlesEnable, 0x005C6FC0);

int __cdecl Native_PedSetSeasonalModels(lua_State* L) {
    World_SetSeasonalModels(world, Script::GetInt(L, 0));
    return 0;
}
NB_REVERSED(PedSetSeasonalModels, 0x005C71D0);

int __cdecl Native_PedMakeAmbient(lua_State* L) {
    Script_PedMakeAmbient(L, Script::GetInt(L, 0), 1);
    return 0;
}
NB_REVERSED(PedMakeAmbient, 0x005CA9B0);

int __cdecl Native_PedMakeAmbientKeepResources(lua_State* L) {
    Script_PedMakeAmbient(L, Script::GetInt(L, 0), 0);
    return 0;
}
NB_REVERSED(PedMakeAmbientKeepResources, 0x005CA9D0);

int __cdecl Native_PedSetUniqueModelStatus(lua_State* L) {
    int model = Script::GetInt(L, 0);
    if (model != -1) {
        CBaseModelInfo* info = CModelInfo::ms_modelInfoPtrs[model];
        if (info) {
            At<int16_t>(info, 0x5C) = int16_t(Script::GetInt(L, 1));
        }
    }
    return 0;
}
NB_REVERSED(PedSetUniqueModelStatus, 0x005CE010);

// peds (not the ones +0x1C98 marks) with the model
int __cdecl Native_PedGetPedCountWithModel(lua_State* L) {
    int model = Script::GetInt(L, 0);
    CPool<CPed>* pool = CPools::ms_pPedPool;
    int count = 0;
    for (int slot = pool->size; slot != 0;) {
        slot--;
        uint8_t* ped = reinterpret_cast<uint8_t*>(pool->GetAt(slot));
        if (ped && ped[0x1C98] == 0 && At<int16_t>(ped, 0x10E) == model) {
            count++;
        }
    }
    Script::PushInt(L, count);
    return 1;
}
NB_REVERSED(PedGetPedCountWithModel, 0x005CEBE0);

// ---- peds --------------------------------------------------------------------------

// (also PedDontCleanup: looks the ped up and nothing more)
int __cdecl Native_PedSetAnim(lua_State* L) {
    Script_GetPed(Script::GetInt(L, 0), 2);
    return 0;
}
NB_REVERSED(PedSetAnim, 0x005CA180);

// (always false)
int __cdecl Native_PedIsCurrentAnim(lua_State* L) {
    Script_GetPed(Script::GetInt(L, 0), 2);
    Script::PushBool(L, 0);
    return 1;
}
NB_REVERSED(PedIsCurrentAnim, 0x005C8990);

// (handle 0: the player)
int __cdecl Native_PedSocialKeepAlive(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    uint8_t* ped = Script_GetPed(handle, handle != 0 ? 2 : 3);
    At<uint32_t>(ped, 0xC38) = timeInMs;
    return 0;
}
NB_REVERSED(PedSocialKeepAlive, 0x005CB100);

int __cdecl Native_PedClearHasAggressed(lua_State* L) {
    uint8_t* ped = Script_GetPed(Script::GetInt(L, 0), 2);
    if (ped) {
        ped[0x5D8] = 0;
    }
    return 0;
}
NB_REVERSED(PedClearHasAggressed, 0x005CE1A0);

int __cdecl Native_PedHasReceivedGift(lua_State* L) {
    uint8_t* ped = Script_GetPed(Script::GetInt(L, 0), 2);
    Script::PushBool(L, ped[0x1E27]);
    return 1;
}
NB_REVERSED(PedHasReceivedGift, 0x005CDE00);

int __cdecl Native_PedCanTeleportOnAreaTransition(lua_State* L) {
    uint8_t* ped = Script_GetPed(Script::GetInt(L, 0), 2);
    ped[0x1C99] = Script::GetBool(L, 1);
    return 0;
}
NB_REVERSED(PedCanTeleportOnAreaTransition, 0x005CD1E0);

int __cdecl Native_PedSetAlwaysStrafe(lua_State* L) {
    uint8_t* ped = Script_GetPed(Script::GetInt(L, 0), 2);
    ped[0x718] = Script::GetBool(L, 1);
    return 0;
}
NB_REVERSED(PedSetAlwaysStrafe, 0x005CD280);

// (no ped check)
int __cdecl Native_PedOnBikePathSmoothing(lua_State* L) {
    uint8_t* ped = PedArg(L);
    At<float>(ped, 0x524) = Script::GetFloat(L, 1);
    return 0;
}
NB_REVERSED(PedOnBikePathSmoothing, 0x005CDAD0);

int __cdecl Native_PedSetFightingSystem(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    int system = Script::GetInt(L, 1);
    uint8_t* ped = Script_GetPed(handle, 2);
    if (ped) {
        At<int32_t>(ped, 0x16DC) = system;
    }
    return 0;
}
NB_REVERSED(PedSetFightingSystem, 0x005CA6F0);

int __cdecl Native_PedSetAllyBehaviour(lua_State* L) {
    uint8_t* ped = PedArg(L);
    int behaviour = Script::GetIntOpt(L, 1, 1);
    if (ped) {
        At<int32_t>(ped, 0x318) = behaviour;
    }
    return 0;
}
NB_REVERSED(PedSetAllyBehaviour, 0x005CDA50);

// A ped's byte flag from the second argument (when the ped exists)
static int SetPedByte(lua_State* L, uint32_t offset) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        ped[offset] = Script::GetBool(L, 1);
    }
    return 0;
}

// A ped's int flag (0/1) from the second argument
static int SetPedIntFlag(lua_State* L, uint32_t offset) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        At<int32_t>(ped, offset) = Script::GetBool(L, 1);
    }
    return 0;
}

// A ped's int flag set when the second argument is false
static int SetPedIntFlagNot(lua_State* L, uint32_t offset) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        At<int32_t>(ped, offset) = Script::GetBool(L, 1) ? 0 : 1;
    }
    return 0;
}

// A ped's float from the second argument
static int SetPedFloat(lua_State* L, uint32_t offset) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        At<float>(ped, offset) = Script::GetFloat(L, 1);
    }
    return 0;
}

int __cdecl Native_PedSetStationary(lua_State* L) { return SetPedByte(L, 0x4E4); }
NB_REVERSED(PedSetStationary, 0x005C7AB0);
int __cdecl Native_PedSetAllyAutoEngage(lua_State* L) { return SetPedByte(L, 0x304); }
NB_REVERSED(PedSetAllyAutoEngage, 0x005CD810);
int __cdecl Native_PedSetAllyJump(lua_State* L) { return SetPedByte(L, 0x305); }
NB_REVERSED(PedSetAllyJump, 0x005CD850);
int __cdecl Native_PedSetCheap(lua_State* L) { return SetPedByte(L, 0x1E25); }
NB_REVERSED(PedSetCheap, 0x005CE410);

int __cdecl Native_PedSetMaxHealth(lua_State* L) { return SetPedFloat(L, 0x1CA4); }
NB_REVERSED(PedSetMaxHealth, 0x005C8F40);
int __cdecl Native_PedSetMinHealth(lua_State* L) { return SetPedFloat(L, 0x1CA8); }
NB_REVERSED(PedSetMinHealth, 0x005C9050);
int __cdecl Native_PedSetMinEngage(lua_State* L) { return SetPedFloat(L, 0x42C); }
NB_REVERSED(PedSetMinEngage, 0x005CB8F0);
int __cdecl Native_PedSetStealthVisionHeight(lua_State* L) { return SetPedFloat(L, 0xA70); }
NB_REVERSED(PedSetStealthVisionHeight, 0x005CE110);

int __cdecl Native_PedAllowCollisionResponse(lua_State* L) { return SetPedIntFlag(L, 0x60); }
NB_REVERSED(PedAllowCollisionResponse, 0x005CE4A0);
int __cdecl Native_PedSetInvulnerable(lua_State* L) { return SetPedIntFlag(L, 0x138C); }
NB_REVERSED(PedSetInvulnerable, 0x005C92C0);
int __cdecl Native_PedSetRemoveOwnedProj(lua_State* L) { return SetPedIntFlag(L, 0x1394); }
NB_REVERSED(PedSetRemoveOwnedProj, 0x005C9310);
int __cdecl Native_PedSetInvulnerableToPlayer(lua_State* L) { return SetPedIntFlag(L, 0x1364); }
NB_REVERSED(PedSetInvulnerableToPlayer, 0x005C9360);
int __cdecl Native_PedSetCheering(lua_State* L) { return SetPedIntFlag(L, 0x136C); }
NB_REVERSED(PedSetCheering, 0x005C93B0);
int __cdecl Native_PedIgnoreAttacks(lua_State* L) { return SetPedIntFlag(L, 0x1360); }
NB_REVERSED(PedIgnoreAttacks, 0x005CA480);
int __cdecl Native_PedIgnoreStimuli(lua_State* L) { return SetPedIntFlag(L, 0x1370); }
NB_REVERSED(PedIgnoreStimuli, 0x005CA4D0);
int __cdecl Native_PedSetAsleep(lua_State* L) { return SetPedIntFlag(L, 0x1358); }
NB_REVERSED(PedSetAsleep, 0x005CAB90);
int __cdecl Native_PedAlwaysUpdateAnimation(lua_State* L) { return SetPedIntFlag(L, 0xE4); }
NB_REVERSED(PedAlwaysUpdateAnimation, 0x005CBD30);
int __cdecl Native_PedIgnoreAttackCone(lua_State* L) { return SetPedIntFlag(L, 0x1380); }
NB_REVERSED(PedIgnoreAttackCone, 0x005CBD80);
int __cdecl Native_PedSetInfiniteSprint(lua_State* L) { return SetPedIntFlag(L, 0x13C8); }
NB_REVERSED(PedSetInfiniteSprint, 0x005CDFC0);
int __cdecl Native_PedSetScenarioObjFlag(lua_State* L) { return SetPedIntFlag(L, 0x13F8); }
NB_REVERSED(PedSetScenarioObjFlag, 0x005CE790);
int __cdecl Native_PedSetImmortalFlag(lua_State* L) { return SetPedIntFlag(L, 0x13FC); }
NB_REVERSED(PedSetImmortalFlag, 0x005CE840);
int __cdecl Native_PedDisableMoveOutOfWay(lua_State* L) { return SetPedIntFlag(L, 0x1584); }
NB_REVERSED(PedDisableMoveOutOfWay, 0x005CF110);
int __cdecl Native_PedSetNoDamageNextFall(lua_State* L) { return SetPedIntFlag(L, 0x14D4); }
NB_REVERSED(PedSetNoDamageNextFall, 0x005CF560);

// (+0x1C / +0xC0 are "off" flags: set when the argument is false)
int __cdecl Native_PedSetUsesCollisionScripted(lua_State* L) { return SetPedIntFlagNot(L, 0x1C); }
NB_REVERSED(PedSetUsesCollisionScripted, 0x005CE450);
int __cdecl Native_PedMakeTargetable(lua_State* L) { return SetPedIntFlagNot(L, 0xC0); }
NB_REVERSED(PedMakeTargetable, 0x005CAAE0);

// (no ped check)
int __cdecl Native_PedPathNodeReachedDistance(lua_State* L) {
    uint8_t* ped = PedArg(L);
    At<float>(ped, 0x51C) = Script::GetFloat(L, 1);
    ped[0x520] = 1;
    return 0;
}
NB_REVERSED(PedPathNodeReachedDistance, 0x005CDA90);

int __cdecl Native_PedGetFaction(lua_State* L) {
    uint8_t* ped = PedArg(L);
    Script::PushInt(L, ped ? At<int32_t>(ped, 0x1310) : -1);
    return 1;
}
NB_REVERSED(PedGetFaction, 0x005CB720);

// (faction 13 is refused)
int __cdecl Native_PedSetFaction(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        int faction = Script::GetInt(L, 1);
        if (faction != 13) {
            At<int32_t>(ped, 0x1310) = faction;
        }
    }
    return 0;
}
NB_REVERSED(PedSetFaction, 0x005CB6D0);

int __cdecl Native_PedClearPOIForAllPeds(lua_State*) {
    CPool<CPed>* pool = CPools::ms_pPedPool;
    for (int slot = pool->size; slot != 0;) {
        slot--;
        uint8_t* ped = reinterpret_cast<uint8_t*>(pool->GetAt(slot));
        if (ped && At<void*>(ped, 0x1D24) != nullptr) {
            Ped_ClearPOI(ped, 1);
        }
    }
    return 0;
}
NB_REVERSED(PedClearPOIForAllPeds, 0x005CF790);

// (no ped check: +0x2E0 is its action tree, +0x260 the node's time)
int __cdecl Native_PedGetNodeTime(lua_State* L) {
    uint8_t* ped = PedArg(L);
    Script::PushFloat(L, At<float>(At<uint8_t*>(ped, 0x2E0), 0x260));
    return 1;
}
NB_REVERSED(PedGetNodeTime, 0x005CBAE0);

// -2 for no such model
int __cdecl Native_PedGetUniqueModelStatus(lua_State* L) {
    int model = Script::GetInt(L, 0);
    int status = -2;
    if (model != -1) {
        CBaseModelInfo* info = CModelInfo::ms_modelInfoPtrs[model];
        if (info) {
            status = PedModelInfo_GetUniqueStatus(info);
        }
    }
    Script::PushInt(L, status);
    return 1;
}
NB_REVERSED(PedGetUniqueModelStatus, 0x005CE050);

// (nothing pushed for no ped)
int __cdecl Native_PedIsOnVehicle(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (!ped) {
        return 0;
    }
    Script::PushBool(L, At<void*>(ped, 0x1554) != nullptr);
    return 1;
}
NB_REVERSED(PedIsOnVehicle, 0x005C8D70);

int __cdecl Native_PedGetHeading(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (!ped) {
        return 0;
    }
    Script::PushFloat(L, At<float>(ped, 0x1700));
    return 1;
}
NB_REVERSED(PedGetHeading, 0x005CCF90);

int __cdecl Native_PedSetProjectileLifetime(lua_State* L) {
    uint8_t* ped = PedArg(L);
    float lifetime = Script::GetFloat(L, 1);
    if (ped) {
        At<float>(ped, 0x1CC0) = lifetime;
    }
    return 0;
}
NB_REVERSED(PedSetProjectileLifetime, 0x005CCDE0);

// ---- attitudes ---------------------------------------------------------------------

int __cdecl Native_PedSetTypeToTypeAttitude(lua_State* L) {
    int from = Script::GetInt(L, 0);
    int to = Script::GetInt(L, 1);
    int attitude = Script::GetInt(L, 2);
    Attitudes_SetTypeToType(attitudes, from, to, attitude);
    return 0;
}
NB_REVERSED(PedSetTypeToTypeAttitude, 0x005C6C50);

int __cdecl Native_PedSetDefaultTypeToTypeAttitude(lua_State* L) {
    int from = Script::GetInt(L, 0);
    int to = Script::GetInt(L, 1);
    int attitude = Script::GetInt(L, 2);
    Attitudes_SetDefaultTypeToType(attitudes, from, to, attitude);
    return 0;
}
NB_REVERSED(PedSetDefaultTypeToTypeAttitude, 0x005C6C90);

int __cdecl Native_PedGetTypeToTypeAttitude(lua_State* L) {
    int from = Script::GetInt(L, 0);
    int to = Script::GetInt(L, 1);
    int attitude;
    Attitudes_GetTypeToType(attitudes, &attitude, from, to);
    Script::PushInt(L, attitude);
    return 1;
}
NB_REVERSED(PedGetTypeToTypeAttitude, 0x005C6CE0);

// ---- fires, timer ------------------------------------------------------------------

int __cdecl Native_FireSetScale(lua_State* L) {
    int id = Script::GetInt(L, 0);
    float scale = Script::GetFloat(L, 1);
    Fires_SetScale(fires, id, scale);
    return 0;
}
NB_REVERSED(FireSetScale, 0x005C0AB0);

int __cdecl Native_FireSetDamageRadius(lua_State* L) {
    int id = Script::GetInt(L, 0);
    float radius = Script::GetFloat(L, 1);
    Fires_SetDamageRadius(fires, id, radius);
    return 0;
}
NB_REVERSED(FireSetDamageRadius, 0x005C0AF0);

int __cdecl Native_MissionTimerHasFinished(lua_State* L) {
    Script::PushBool(L, MissionTimer_GetTimeRemaining(missionTimer) <= 0.0f);
    return 1;
}
NB_REVERSED(MissionTimerHasFinished, 0x005C2230);

// (truncated through a 64-bit integer, as the game's fistp does)
int __cdecl Native_MissionTimerGetTimeRemaining(lua_State* L) {
    Script::PushInt(L, int(int64_t(MissionTimer_GetTimeRemaining(missionTimer))));
    return 1;
}
NB_REVERSED(MissionTimerGetTimeRemaining, 0x005C2280);

// ---- prop anims --------------------------------------------------------------------

int __cdecl Native_PAnimOverrideDamage(lua_State* L) {
    uint8_t* prop = PropAnims_Get(propAnims, Script::GetInt(L, 0));
    if (prop) {
        At<int16_t>(prop, 0x120) = int16_t(Script::GetInt(L, 1));
    }
    return 0;
}
NB_REVERSED(PAnimOverrideDamage, 0x005C3BD0);

int __cdecl Native_PAnimOverrideMinDamage(lua_State* L) {
    uint8_t* prop = PropAnims_Get(propAnims, Script::GetInt(L, 0));
    if (prop) {
        At<int16_t>(prop, 0x122) = int16_t(Script::GetInt(L, 1));
    }
    return 0;
}
NB_REVERSED(PAnimOverrideMinDamage, 0x005C3C10);

// (+0xF8 set when not targetable)
int __cdecl Native_PAnimMakeVisuallyTargetable(lua_State* L) {
    uint8_t* prop = PropAnims_Get(propAnims, Script::GetInt(L, 0));
    if (prop) {
        At<int32_t>(prop, 0xF8) = Script::GetBool(L, 1) ? 0 : 1;
    }
    return 0;
}
NB_REVERSED(PAnimMakeVisuallyTargetable, 0x005C3C50);

int __cdecl Native_PAnimSetAlpha(lua_State* L) {
    int next = -1;
    uint8_t* prop = Script_GetPropAnimArg(L, 1, &next);
    if (prop) {
        At<int16_t>(prop, 0x250) = int16_t(Script::GetInt(L, next));
    }
    return 0;
}
NB_REVERSED(PAnimSetAlpha, 0x005C57E0);

// (no prop anim check)
int __cdecl Native_PAnimSetPulsateLight(lua_State* L) {
    int next = -1;
    uint8_t* prop = Script_GetPropAnimArg(L, 1, &next);
    uint8_t on = Script::GetBool(L, next);
    prop[0x258] = on;
    if (!on) {
        At<float>(prop, 0x25C) = 1.0f;
    }
    return 0;
}
NB_REVERSED(PAnimSetPulsateLight, 0x005C5BC0);

int __cdecl Native_PAnimMakeTargetable(lua_State* L) {
    int next = -1;
    uint8_t* prop = Script_GetPropAnimArg(L, 1, &next);
    if (prop) {
        At<int32_t>(prop, 0xC0) = Script::GetBool(L, next) ? 0 : 1;
    }
    return 0;
}
NB_REVERSED(PAnimMakeTargetable, 0x005C4F60);

// ---- peds (2) ----------------------------------------------------------------------

// (normalised to 0/1)
int __cdecl Native_PedSetTetherMoveToTarget(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        ped[0x3B4] = Script::GetBool(L, 1) != 0;
    }
    return 0;
}
NB_REVERSED(PedSetTetherMoveToTarget, 0x005CC750);

int __cdecl Native_PedSetTetherMoveToCenter(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        ped[0x3B5] = Script::GetBool(L, 1) != 0;
    }
    return 0;
}
NB_REVERSED(PedSetTetherMoveToCenter, 0x005CC7A0);

// (no ped check; handle 0 is the player)
int __cdecl Native_PedOverrideSocialResponseToStimulus(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    uint8_t* ped = Script_GetPed(handle, handle != 0 ? 2 : 3);
    int response = Script::GetInt(L, 1);
    int stimulus = Script::GetInt(L, 2);
    ped[stimulus + 0xC9A] = uint8_t(response);
    return 0;
}
NB_REVERSED(PedOverrideSocialResponseToStimulus, 0x005CB050);

int __cdecl Native_PedUseSocialOverride(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    uint8_t* ped = Script_GetPed(handle, handle != 0 ? 2 : 3);
    int stimulus = Script::GetInt(L, 1);
    ped[stimulus + 0xC80] = Script::GetBoolOpt(L, 2, 1);
    return 0;
}
NB_REVERSED(PedUseSocialOverride, 0x005CB000);

// (+0x1C: the entity flags, ints)
int __cdecl Native_PedSetEntityFlag(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        int flag = Script::GetInt(L, 1);
        At<int32_t>(ped, 0x1C + flag * 4) = Script::GetBool(L, 2);
    }
    return 0;
}
NB_REVERSED(PedSetEntityFlag, 0x005CC2D0);

int __cdecl Native_PedHasAggressed(lua_State* L) {
    uint8_t* ped = PedArg(L);
    uint8_t aggressed = 0;
    if (ped) {
        aggressed = ped[0x5D8];
    }
    Script::PushBool(L, aggressed);
    return 1;
}
NB_REVERSED(PedHasAggressed, 0x005CE150);

int __cdecl Native_PedSetDamageGivenMultiplier(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        int kind = Script::GetInt(L, 1);
        At<float>(ped, 0x1CCC + kind * 4) = Script::GetFloat(L, 2);
    }
    return 0;
}
NB_REVERSED(PedSetDamageGivenMultiplier, 0x005CF460);

int __cdecl Native_PedSetDamageTakenMultiplier(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        int kind = Script::GetInt(L, 1);
        At<float>(ped, 0x1CF4 + kind * 4) = Script::GetFloat(L, 2);
    }
    return 0;
}
NB_REVERSED(PedSetDamageTakenMultiplier, 0x005CF510);

// (nothing pushed for no ped)
int __cdecl Native_PedGetDamageGivenMultiplier(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (!ped) {
        return 0;
    }
    int kind = Script::GetInt(L, 1);
    Script::PushFloat(L, At<float>(ped, 0x1CCC + kind * 4));
    return 1;
}
NB_REVERSED(PedGetDamageGivenMultiplier, 0x005CF400);

int __cdecl Native_PedGetDamageTakenMultiplier(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (!ped) {
        return 0;
    }
    int kind = Script::GetInt(L, 1);
    Script::PushFloat(L, At<float>(ped, 0x1CF4 + kind * 4));
    return 1;
}
NB_REVERSED(PedGetDamageTakenMultiplier, 0x005CF4B0);

int __cdecl Native_PedGetMoney(lua_State* L) {
    uint8_t* ped = PedArg(L);
    Script::PushInt(L, ped ? At<int32_t>(ped, 0x1CA0) : 0);
    return 1;
}
NB_REVERSED(PedGetMoney, 0x005C9270);

// A ped's pointer field is set (false for no ped)
static int PushPedFieldSet(lua_State* L, uint32_t offset) {
    uint8_t* ped = PedArg(L);
    uint8_t set = 0;
    if (ped && At<int32_t>(ped, offset) != 0) {
        set = 1;
    }
    Script::PushBool(L, set);
    return 1;
}

int __cdecl Native_PedHasAllyLeader(lua_State* L) { return PushPedFieldSet(L, 0x2F4); }
NB_REVERSED(PedHasAllyLeader, 0x005CD8F0);
int __cdecl Native_PedHasAllyFollower(lua_State* L) { return PushPedFieldSet(L, 0x2F8); }
NB_REVERSED(PedHasAllyFollower, 0x005CD940);
int __cdecl Native_PedIsUsingProp(lua_State* L) { return PushPedFieldSet(L, 0x548); }
NB_REVERSED(PedIsUsingProp, 0x005CEDB0);
int __cdecl Native_PedGetScenarioObjFlag(lua_State* L) { return PushPedFieldSet(L, 0x13F8); }
NB_REVERSED(PedGetScenarioObjFlag, 0x005CE7E0);
int __cdecl Native_PedGetImmortalFlag(lua_State* L) { return PushPedFieldSet(L, 0x13FC); }
NB_REVERSED(PedGetImmortalFlag, 0x005CE890);
int __cdecl Native_PedIsInAnyVehicle(lua_State* L) { return PushPedFieldSet(L, 0x1554); }
NB_REVERSED(PedIsInAnyVehicle, 0x005C8930);

// (the flag argument is read and nothing checked: always false)
int __cdecl Native_PedIsAnimFlagSet(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (!ped) {
        Script::PushBool(L, 0);
        return 1;
    }
    Script::GetInt(L, 1);
    Script::PushBool(L, 0);
    return 1;
}
NB_REVERSED(PedIsAnimFlagSet, 0x005C89D0);

// its model's name (nil for no ped)
int __cdecl Native_PedGetName(lua_State* L) {
    uint8_t* ped = PedArg(L);
    const char* name = nullptr;
    if (ped) {
        CBaseModelInfo* info = CModelInfo::ms_modelInfoPtrs[At<int16_t>(ped, 0x10E)];
        if (info) {
            name = reinterpret_cast<const char*>(info) + 0x98;
        }
    }
    Script::PushString(L, name);
    return 1;
}
NB_REVERSED(PedGetName, 0x005CF0C0);

// (nothing pushed for no ped)
int __cdecl Native_PedHasPOI(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (!ped) {
        return 0;
    }
    Script::PushBool(L, At<void*>(ped, 0x1D24) != nullptr);
    return 1;
}
NB_REVERSED(PedHasPOI, 0x005CC0F0);

int __cdecl Native_PedRestoreNumCombatants(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        At<int32_t>(ped, 0x670) = At<int32_t>(combatantDefaults, 0x18);
        At<int32_t>(ped, 0x674) = At<int32_t>(combatantDefaults, 0x1C);
        At<int32_t>(ped, 0x678) = At<int32_t>(combatantDefaults, 0x20);
    }
    return 0;
}
NB_REVERSED(PedRestoreNumCombatants, 0x005CBB20);

int __cdecl Native_PedSetNumCombatants(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        At<int32_t>(ped, 0x670) = Script::GetInt(L, 1);
        At<int32_t>(ped, 0x674) = Script::GetInt(L, 2);
        At<int32_t>(ped, 0x678) = Script::GetInt(L, 3);
    }
    return 0;
}
NB_REVERSED(PedSetNumCombatants, 0x005CB930);

// which controller (0 or 1, as 16 bits) the ped is on
int __cdecl Native_PedSetControllerID(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        uint16_t id = uint16_t(Script::GetInt(L, 1));
        if (id == 0) {
            controllerPed0 = ped;
        } else if (id == 1) {
            controllerPed1 = ped;
        }
    }
    return 0;
}
NB_REVERSED(PedSetControllerID, 0x005CA120);

// (+0x1314: the ped flags, ints; nothing pushed for no ped)
int __cdecl Native_PedGetFlag(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (!ped) {
        return 0;
    }
    int flag = Script::GetInt(L, 1);
    Script::PushBool(L, At<int32_t>(ped, 0x1314 + flag * 4) != 0);
    return 1;
}
NB_REVERSED(PedGetFlag, 0x005CC270);

// (flag 12 and those past 0x88 are refused)
int __cdecl Native_PedSetFlag(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        int flag = Script::GetInt(L, 1);
        uint8_t on = Script::GetBool(L, 2);
        if (flag != 12 && uint32_t(flag) <= 0x88) {
            At<int32_t>(ped, 0x1314 + flag * 4) = on;
        }
    }
    return 0;
}
NB_REVERSED(PedSetFlag, 0x005CC210);

// -100 for no ped
int __cdecl Native_PedGetMaxHealth(lua_State* L) {
    uint8_t* ped = PedArg(L);
    Script::PushFloat(L, ped ? At<float>(ped, 0x1CA4) : -100.0f);
    return 1;
}
NB_REVERSED(PedGetMaxHealth, 0x005C8EE0);

int __cdecl Native_PedGetHealth(lua_State* L) {
    uint8_t* ped = PedArg(L);
    Script::PushFloat(L, ped ? At<float>(ped, 0x1CB8) : -100.0f);
    return 1;
}
NB_REVERSED(PedGetHealth, 0x005C8F80);

int __cdecl Native_PedIsValid(lua_State* L) {
    uint8_t valid = 0;
    if (Script::IsNumber(L, 0)) {
        int handle = Script::GetInt(L, 0);
        if (handle != -1 && Script_GetPed(handle, 2)) {
            valid = 1;
        }
    }
    Script::PushBool(L, valid);
    return 1;
}
NB_REVERSED(PedIsValid, 0x005C99C0);

// (true for no such ped)
int __cdecl Native_PedIsDead(lua_State* L) {
    uint8_t dead = 1;
    if (Script::IsNumber(L, 0)) {
        int handle = Script::GetInt(L, 0);
        if (handle != -1) {
            uint8_t* ped = Script_GetPed(handle, 2);
            if (ped) {
                dead = ped[0x1C98];
            }
        }
    }
    Script::PushBool(L, dead);
    return 1;
}
NB_REVERSED(PedIsDead, 0x005C9090);

int __cdecl Native_PedHasAlly(lua_State* L) {
    uint8_t* ped = PedArg(L);
    uint8_t has = 0;
    if (ped) {
        if (At<int32_t>(ped, 0x2F4) != 0) {
            has = 1;
        }
        if (At<int32_t>(ped, 0x2F8) != 0) {
            has = 1;
        }
    }
    Script::PushBool(L, has);
    return 1;
}
NB_REVERSED(PedHasAlly, 0x005CD890);

int __cdecl Native_PedIsSocializing(lua_State* L) {
    uint8_t* ped = PedArg(L);
    uint8_t socializing = 0;
    if (ped) {
        socializing = At<int32_t>(ped, 0xBB0) != 0 && At<int32_t>(ped, 0xBB4) != 0;
    }
    Script::PushBool(L, socializing);
    return 1;
}
NB_REVERSED(PedIsSocializing, 0x005CDB10);

int __cdecl Native_PedIsWantingToSocialize(lua_State* L) {
    uint8_t* ped = PedArg(L);
    uint8_t wanting = 0;
    if (ped) {
        wanting = (ped[0xD68] & 0x10) != 0 && At<int32_t>(ped, 0xBB8) != 0;
    }
    Script::PushBool(L, wanting);
    return 1;
}
NB_REVERSED(PedIsWantingToSocialize, 0x005CDB70);

// (faction 13 is the player's)
int __cdecl Native_PedIsPlayer(lua_State* L) {
    uint8_t* ped = PedArg(L);
    uint8_t player = 0;
    if (ped && At<int32_t>(ped, 0x1310) == 13) {
        player = 1;
    }
    Script::PushBool(L, player);
    return 1;
}
NB_REVERSED(PedIsPlayer, 0x005C9A20);

// its weapon's model (+0x1D0 the weapon, no check on it), -1 for no ped; handle 0
// is the player
int __cdecl Native_PedGetWeapon(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    uint8_t* ped = handle == 0 ? static_cast<uint8_t*>(playerPed) : Script_GetPed(handle, 2);
    if (ped) {
        Script::PushInt(L, At<int16_t>(At<uint8_t*>(ped, 0x1D0), 0x10E));
        return 1;
    }
    Script::PushInt(L, -1);
    return 1;
}
NB_REVERSED(PedGetWeapon, 0x005CE1D0);

int __cdecl Native_PedHasWeapon(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    int model = Script::GetInt(L, 1);
    uint8_t has = 0;
    uint8_t* ped = Script_GetPed(handle, 2);
    if (ped) {
        has = At<int16_t>(At<uint8_t*>(ped, 0x1D0), 0x10E) == model;
    }
    Script::PushBool(L, has);
    return 1;
}
NB_REVERSED(PedHasWeapon, 0x005CD2C0);

// a flag (+0x26C, ints) of the ped's weapon, when it is that model
int __cdecl Native_PedSetWeaponFlag(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    int model = Script::GetInt(L, 1);
    uint8_t* ped = Script_GetPed(handle, 2);
    if (ped && At<int16_t>(At<uint8_t*>(ped, 0x1D0), 0x10E) == model) {
        int flag = Script::GetInt(L, 2);
        uint8_t on = Script::GetBool(L, 3);
        At<int32_t>(At<uint8_t*>(ped, 0x1D0), 0x26C + flag * 4) = on;
    }
    return 0;
}
NB_REVERSED(PedSetWeaponFlag, 0x005CF1F0);

// (handle 0: the player's weapon)
int __cdecl Native_WeaponSetRangeMultiplier(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    uint8_t* ped = Script_GetPed(handle, handle != 0 ? 2 : 3);
    if (ped) {
        int kind = Script::GetInt(L, 1);
        float multiplier = Script::GetFloat(L, 2);
        uint8_t* weapon = At<uint8_t*>(ped, 0x1C8);
        At<float>(weapon, 0x88) = multiplier;
        At<int32_t>(weapon, 0x8C) = kind;
    }
    return 0;
}
NB_REVERSED(WeaponSetRangeMultiplier, 0x005C1000);

// the type argument, then two optional ones (-1: 0, and 5)
int __cdecl Native_PedGetRandomModelId(lua_State* L) {
    int type = Script::GetIntOpt(L, 0, -1);
    int b = Script::GetIntOpt(L, 1, -1);
    if (b == -1) {
        b = 0;
    }
    int c = Script::GetIntOpt(L, 2, -1);
    if (c == -1) {
        c = 5;
    }
    Script::PushInt(L, PedTypeMgr_GetRandomModel(pedTypeMgr, type, b, c));
    return 1;
}
NB_REVERSED(PedGetRandomModelId, 0x005C66A0);

// ---- missions (2) ------------------------------------------------------------------

// -1 declined, 1 accepted, 0 not answered
int __cdecl Native_MissionGetUserStartResponse(lua_State* L) {
    int response = 0;
    switch (MissionMgr_AskUserToStart(missionMgr, 0)) {
    case 1:
        response = -1;
        break;
    case 2:
        Script::PushInt(L, 1);
        return 1;
    }
    Script::PushInt(L, response);
    return 1;
}
NB_REVERSED(MissionGetUserStartResponse, 0x005C1370);

// (complete unless a second argument says otherwise)
int __cdecl Native_MiniObjectiveSetIsComplete(lua_State* L) {
    int id = Script::GetInt(L, 0);
    uint8_t complete = 1;
    if (lua::gettop(L) >= 2) {
        complete = Script::GetBool(L, 1);
    }
    PlayerInfo_SetMiniObjectiveComplete(playerInfo, id, complete);
    return 0;
}
NB_REVERSED(MiniObjectiveSetIsComplete, 0x005C13E0);

// x, y, z, heading, area
int __cdecl Native_MissionOverrideArrestPoint(lua_State* L) {
    ScriptVector pos;
    pos.x = Script::GetFloat(L, 0);
    pos.y = Script::GetFloat(L, 1);
    pos.z = Script::GetFloat(L, 2);
    float heading = Script::GetFloat(L, 3);
    int area = Script::GetInt(L, 4);
    RespawnOverrides_SetArrestPoint(respawnOverrides, &pos, heading, area);
    return 0;
}
NB_REVERSED(MissionOverrideArrestPoint, 0x005C1180);

int __cdecl Native_MissionOverrideKOPoint(lua_State* L) {
    ScriptVector pos;
    pos.x = Script::GetFloat(L, 0);
    pos.y = Script::GetFloat(L, 1);
    pos.z = Script::GetFloat(L, 2);
    float heading = Script::GetFloat(L, 3);
    int area = Script::GetInt(L, 4);
    RespawnOverrides_SetKOPoint(respawnOverrides, &pos, heading, area);
    return 0;
}
NB_REVERSED(MissionOverrideKOPoint, 0x005C1200);

// ---- objects, prop anims (2) -------------------------------------------------------

// (handles as 16 bits; no object check)
int __cdecl Native_ObjectMakeTargetable(lua_State* L) {
    uint16_t handle = uint16_t(Script::GetInt(L, 0));
    uint8_t on = Script::GetBool(L, 1);
    uint8_t* object = Script_GetEntity(handle, 0, 1, 0);
    At<int32_t>(object, 0xC0) = on;
    return 0;
}
NB_REVERSED(ObjectMakeTargetable, 0x005C2F30);

// (handle and model as 16 bits; the model compared unsigned; no object check)
int __cdecl Native_ObjectIsModel(lua_State* L) {
    uint16_t handle = uint16_t(Script::GetInt(L, 0));
    uint16_t model = uint16_t(Script::GetInt(L, 1));
    uint8_t* object = Pools_GetObject(handle);
    Script::PushBool(L, At<int16_t>(object, 0x10E) == int(model));
    return 1;
}
NB_REVERSED(ObjectIsModel, 0x005C23D0);

// The object pool slot with the hash id (+0x228), -1 for none. (It looks each slot
// up as a handle: slot >> 8 with the flag byte, so past slot 0 it gets nullptr and
// reads +0x228 through it, as the game does)
int __cdecl Native_ObjectHashIDToPoolIndex(lua_State* L) {
    lua::gettop(L);
    int id = int(reinterpret_cast<uintptr_t>(Script::GetUserdata(L, 0)));
    CPool<CObject>* pool = CPools::ms_pObjectPool;
    int found = -1;
    for (int slot = 0; slot < pool->size; slot++) {
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        int index = slot >> 8;
        uint8_t* object = nullptr;
        if (pool->flags[index] == uint8_t(slot)) {
            object = re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * index;
        }
        if (At<int32_t>(object, 0x228) == id) {
            found = slot;
            break;
        }
    }
    Script::PushInt(L, found);
    return 1;
}
NB_REVERSED(ObjectHashIDToPoolIndex, 0x005C34B0);

// A prop anim's pointer field is set (false for no prop anim)
static int PushPropAnimFieldSet(lua_State* L, uint32_t offset) {
    uint8_t* prop = PropAnims_Get(propAnims, Script::GetInt(L, 0));
    Script::PushBool(L, prop ? At<int32_t>(prop, offset) != 0 : 0);
    return 1;
}

int __cdecl Native_PAnimIsUsed(lua_State* L) { return PushPropAnimFieldSet(L, 0x170); }
NB_REVERSED(PAnimIsUsed, 0x005C39C0);
int __cdecl Native_PAnimIsOpen(lua_State* L) { return PushPropAnimFieldSet(L, 0x184); }
NB_REVERSED(PAnimIsOpen, 0x005C3A10);

// (+0x160: its flags, ints)
int __cdecl Native_PAnimGetPropFlag(lua_State* L) {
    uint8_t set = 0;
    int next = -1;
    uint8_t* prop = Script_GetPropAnimArg(L, 1, &next);
    if (prop) {
        int flag = Script::GetInt(L, next);
        set = At<int32_t>(prop, 0x160 + flag * 4) != 0;
    }
    Script::PushBool(L, set);
    return 1;
}
NB_REVERSED(PAnimGetPropFlag, 0x005C5770);

int __cdecl Native_IsPathDone(lua_State* L) {
    int slot = Script::GetInt(L, 0);
    uint8_t* handle = slot < 0 ? nullptr : scriptPropAnimPool->GetAt(slot);
    uint8_t done = 0;
    if (handle) {
        uint8_t* entity = At<uint8_t*>(handle, 0x1C);
        if (entity) {
            uint8_t* follower = PathMgr_FindForEntity(pathMgr, entity);
            if (follower) {
                uint8_t* path = At<uint8_t*>(follower, 8);
                if (path && path[0xC]) {
                    done = 1;
                }
            }
        }
    }
    Script::PushBool(L, done);
    return 1;
}
NB_REVERSED(IsPathDone, 0x005C5260);

// 1 (+0x69 and +0x68), 2 (+0x69), 0 (+0x68). (With neither the game pushes a stack
// slot it never set, the caller's ecx: 0 here)
int __cdecl Native_WeaponGetType(lua_State* L) {
    uint8_t* info = WeaponInfo_Get(Script::GetInt(L, 0));
    int type = 0;
    if (info[0x69]) {
        type = info[0x68] ? 1 : 2;
    } else if (info[0x68]) {
        type = 0;
    }
    Script::PushInt(L, type);
    return 1;
}
NB_REVERSED(WeaponGetType, 0x005C6530);

// ---- misc (3) ----------------------------------------------------------------------

// (the model argument as a float; the ped's model, sign-extended, as an unsigned int)
int __cdecl Native_PedIsModel(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (!ped) {
        Script::PushBool(L, 0);
        return 1;
    }
    float model = float(Script::GetInt(L, 1));
    uint32_t pedModel = uint32_t(int32_t(At<int16_t>(ped, 0x10E)));
    Script::PushBool(L, double(model) == double(pedModel));
    return 1;
}
NB_REVERSED(PedIsModel, 0x005C9400);

int __cdecl Native_DumpMemoryHistory(lua_State*) {
    MemoryHistory_Dump();
    MemoryHistory_Dump();
    return 0;
}
NB_REVERSED(DumpMemoryHistory, 0x005C0BA0);

int __cdecl Native_E3DodgeballHackCleanObjects(lua_State*) {
    E3Dodgeball_CleanObjects1();
    E3Dodgeball_CleanObjects2();
    return 0;
}
NB_REVERSED(E3DodgeballHackCleanObjects, 0x005C1B20);

int __cdecl Native_DeleteAllMissionPeds(lua_State*) {
    void* script = ScriptMgr_GetCurrent(scriptMgr);
    if (script) {
        Script_DeleteMissionPeds(script);
    }
    return 0;
}
NB_REVERSED(DeleteAllMissionPeds, 0x005C0E80);

int __cdecl Native_PedHideHealthBar(lua_State*) {
    uint8_t* bar = healthBar;
    HealthBar_Set(bar, 0, 0);
    CallVirtual<void>(bar, 5, 0);
    return 0;
}
NB_REVERSED(PedHideHealthBar, 0x005C68D0);

int __cdecl Native_MissionSurpressMissionNameText(lua_State*) {
    if (MissionMgr_IsRunning(missionMgr)) {
        MissionMgr_GetCurrent(missionMgr)[0x80] = 1;
    }
    return 0;
}
NB_REVERSED(MissionSurpressMissionNameText, 0x005C0FD0);

int __cdecl Native_GetMissionCurrentAttemptCount(lua_State* L) {
    int id = *reinterpret_cast<int*>(MissionMgr_GetCurrent(missionMgr));
    Script::PushInt(L, At<uint16_t>(MissionMgr_GetStats(missionMgr, id), 0));
    return 1;
}
NB_REVERSED(GetMissionCurrentAttemptCount, 0x005C0590);

int __cdecl Native_GetMissionCurrentSuccessCount(lua_State* L) {
    int id = *reinterpret_cast<int*>(MissionMgr_GetCurrent(missionMgr));
    Script::PushInt(L, At<uint16_t>(MissionMgr_GetStats(missionMgr, id), 2));
    return 1;
}
NB_REVERSED(GetMissionCurrentSuccessCount, 0x005C05C0);

// (bit 2 of the stats' flags)
int __cdecl Native_IsMissionFromDebug(lua_State* L) {
    int id = *reinterpret_cast<int*>(MissionMgr_GetCurrent(missionMgr));
    Script::PushBool(L, (MissionMgr_GetStats(missionMgr, id)[4] >> 2) & 1);
    return 1;
}
NB_REVERSED(IsMissionFromDebug, 0x005C0400);

int __cdecl Native_IsMissionRestartable(lua_State* L) {
    int id = *reinterpret_cast<int*>(MissionMgr_GetCurrent(missionMgr));
    Script::PushBool(L, MissionMgr_GetInfo(missionMgr, id)[0x2D]);
    return 1;
}
NB_REVERSED(IsMissionRestartable, 0x005C07C0);

int __cdecl Native_IsMissionSimpleFade(lua_State* L) {
    int id = *reinterpret_cast<int*>(MissionMgr_GetCurrent(missionMgr));
    Script::PushBool(L, MissionMgr_GetInfo(missionMgr, id)[0x35]);
    return 1;
}
NB_REVERSED(IsMissionSimpleFade, 0x005C07F0);

int __cdecl Native_MissionStartNextClass(lua_State* L) {
    int id = MissionMgr_GetNextClass(missionMgr);
    if (id != -1) {
        MissionMgr_Start(missionMgr, id, Script::GetBoolOpt(L, 0, 0));
    }
    return 0;
}
NB_REVERSED(MissionStartNextClass, 0x005C10D0);

// (the seconds as 16 bits)
int __cdecl Native_MissionTimerStart(lua_State* L) {
    uint16_t seconds = uint16_t(Script::GetInt(L, 0));
    MissionTimer_Stop(missionTimer, 1);
    MissionTimer_Start(missionTimer, seconds);
    return 0;
}
NB_REVERSED(MissionTimerStart, 0x005C2110);

// (truncated as _ftol does)
int __cdecl Native_FireGetHealth(lua_State* L) {
    Script::PushInt(L, int(int64_t(Fires_GetHealth(fires, Script::GetInt(L, 0)))));
    return 1;
}
NB_REVERSED(FireGetHealth, 0x005C0B60);

int __cdecl Native_ChapterSet(lua_State* L) {
    World_SetChapter(world, Script::GetInt(L, 0));
    if (chapterHud) {
        ChapterHud_Update(chapterHud);
    }
    return 0;
}
NB_REVERSED(ChapterSet, 0x005C0CD0);

// ---- prop anims (3) ----------------------------------------------------------------

// The prop anim the arguments name (from the first)
static uint8_t* PropAnimArg(lua_State* L) {
    int next;
    return Script_GetPropAnimArg(L, 0, &next);
}

int __cdecl Native_PAnimReset(lua_State* L) {
    uint8_t* prop = PropAnimArg(L);
    if (prop) {
        PropAnim_Reset(prop);
    }
    return 0;
}
NB_REVERSED(PAnimReset, 0x005C4D40);

int __cdecl Native_PAnimCloseDoor(lua_State* L) {
    uint8_t* prop = PropAnimArg(L);
    if (prop) {
        PropAnim_CloseDoor(prop, 0);
    }
    return 0;
}
NB_REVERSED(PAnimCloseDoor, 0x005C4CE0);

int __cdecl Native_PAnimDoorStayOpen(lua_State* L) {
    uint8_t* prop = PropAnimArg(L);
    if (prop) {
        PropAnim_SetFlag(prop, 0x10, 1);
    }
    return 0;
}
NB_REVERSED(PAnimDoorStayOpen, 0x005C4D10);

int __cdecl Native_PAnimOpenDoor(lua_State* L) {
    uint8_t* prop = PropAnimArg(L);
    if (prop) {
        PropAnim_OpenDoor(prop, 0, 0, 0);
    }
    return 0;
}
NB_REVERSED(PAnimOpenDoor, 0x005C4C80);

int __cdecl Native_PAnimOpenDoorImmediate(lua_State* L) {
    uint8_t* prop = PropAnimArg(L);
    if (prop) {
        PropAnim_OpenDoor(prop, 0, 0, 1);
    }
    return 0;
}
NB_REVERSED(PAnimOpenDoorImmediate, 0x005C4CB0);

int __cdecl Native_PAnimClearWhenDoneRotation(lua_State* L) {
    uint8_t* prop = PropAnimArg(L);
    if (prop) {
        PathMgr_SetClearWhenDone(pathMgr, prop, 1);
    }
    return 0;
}
NB_REVERSED(PAnimClearWhenDoneRotation, 0x005C5350);

// (no model check)
int __cdecl Native_PAnimTypeSetPickupListOverride(lua_State* L) {
    int model = ModelInfo_GetIndexByName(Script::GetString(L, 0));
    CBaseModelInfo* info = CModelInfo::ms_modelInfoPtrs[model];
    PedModelInfo_SetPickupListOverride(info, Script::GetString(L, 1));
    return 0;
}
NB_REVERSED(PAnimTypeSetPickupListOverride, 0x005C5C10);

// ---- objects (3) -------------------------------------------------------------------

// the name's hash, as a light userdata
int __cdecl Native_ObjectNameToHashID(lua_State* L) {
    lua::gettop(L);
    Script::PushUserdata(L, reinterpret_cast<void*>(uintptr_t(HashName(Script::GetString(L, 0)))));
    return 1;
}
NB_REVERSED(ObjectNameToHashID, 0x005C24F0);

// (vf 0x7C: whether it can be one)
int __cdecl Native_ObjectPedNoTarget(lua_State* L) {
    uint8_t* object = Pools_GetObject(Script::GetInt(L, 0));
    if (object && CallVirtual<uint8_t>(object, 0x7C / 4)) {
        At<int32_t>(object, 0x104) = 1;
    }
    return 0;
}
NB_REVERSED(ObjectPedNoTarget, 0x005C2560);

// (model info type 4 goes the other way; no model check)
int __cdecl Native_ModelNotNeededAmbient(lua_State* L) {
    int model = Script::GetInt(L, 0);
    uint8_t* info = re::PtrCast<uint8_t*>(CModelInfo::ms_modelInfoPtrs[model]);
    if (info[8] != 4) {
        Streaming_SetModelNotNeededAmbient(model);
    } else {
        Streaming_SetPedModelNotNeededAmbient(model);
    }
    return 0;
}
NB_REVERSED(ModelNotNeededAmbient, 0x005CE590);

// ---- peds (3) ----------------------------------------------------------------------

int __cdecl Native_PedResetAttitudes(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        Ped_ResetAttitudes(ped);
    }
    return 0;
}
NB_REVERSED(PedResetAttitudes, 0x005CB6A0);

int __cdecl Native_PedWarpOutOfCar(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        Ped_WarpOutOfCar(ped);
    }
    return 0;
}
NB_REVERSED(PedWarpOutOfCar, 0x005CCE90);

// (no ped check)
int __cdecl Native_PedDismissAllAllies(lua_State* L) {
    PedAllies_DismissAll(PedArg(L) + 0x2F0);
    return 0;
}
NB_REVERSED(PedDismissAllAllies, 0x005CD7E0);

// (no ped check)
int __cdecl Native_PedStopSocializing(lua_State* L) {
    PedSocial_Stop(PedArg(L) + 0xB44);
    return 0;
}
NB_REVERSED(PedStopSocializing, 0x005CDD50);

// (no ped check)
int __cdecl Native_PedSaveWeaponInventorySnapshot(lua_State* L) {
    WeaponInventory_SaveSnapshot(At<void*>(PedArg(L), 0x1C4));
    return 0;
}
NB_REVERSED(PedSaveWeaponInventorySnapshot, 0x005CF160);

int __cdecl Native_PedRestoreWeaponInventorySnapshot(lua_State* L) {
    WeaponInventory_RestoreSnapshot(At<void*>(PedArg(L), 0x1C4));
    return 0;
}
NB_REVERSED(PedRestoreWeaponInventorySnapshot, 0x005CF190);

int __cdecl Native_PedDeleteWeaponInventorySnapshot(lua_State* L) {
    WeaponInventory_DeleteSnapshot(At<void*>(PedArg(L), 0x1C4));
    return 0;
}
NB_REVERSED(PedDeleteWeaponInventorySnapshot, 0x005CF1C0);

// (also PedClearTether)
int __cdecl Native_PedFreeFromTrigger(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        PedTether_Clear(ped + 0x358);
    }
    return 0;
}
NB_REVERSED(PedFreeFromTrigger, 0x005C9B90);

int __cdecl Native_PedClearAntiTether(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        PedTether_ClearAnti(ped + 0x358);
    }
    return 0;
}
NB_REVERSED(PedClearAntiTether, 0x005CC840);

int __cdecl Native_PedRemoveCurrentObjective(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        PedObjectives_RemoveCurrent(ped + 0x5F0);
    }
    return 0;
}
NB_REVERSED(PedRemoveCurrentObjective, 0x005CB5A0);

int __cdecl Native_PedClearFocus(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        PedFocus_Clear(ped + 0x3BC, 0);
    }
    return 0;
}
NB_REVERSED(PedClearFocus, 0x005CABE0);

int __cdecl Native_PedClearPOI(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        void* poi = At<void*>(ped, 0x1D24);
        if (poi) {
            POI_Release(poi, ped);
        }
    }
    return 0;
}
NB_REVERSED(PedClearPOI, 0x005CC1D0);

int __cdecl Native_PedModelNotNeededAmbient(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        Streaming_SetModelNotNeededAmbient(At<int16_t>(ped, 0x10E));
    }
    return 0;
}
NB_REVERSED(PedModelNotNeededAmbient, 0x005CE5D0);

// ---- misc (4) ----------------------------------------------------------------------

int __cdecl Native_CompareHashIDs(lua_State* L) {
    lua::gettop(L);
    void* a = Script::GetUserdata(L, 0);
    void* b = Script::GetUserdata(L, 1);
    Script::PushBool(L, a == b);
    return 1;
}
NB_REVERSED(CompareHashIDs, 0x005C2520);

int __cdecl Native_IsMissionDebugFailure(lua_State* L) {
    uint8_t failure = 0;
    if (MissionMgr_GetCurrent(missionMgr)[0xE] && !MissionMgr_GetCurrent(missionMgr)[0xC]) {
        failure = 1;
    }
    Script::PushBool(L, failure);
    return 1;
}
NB_REVERSED(IsMissionDebugFailure, 0x005C0820);

int __cdecl Native_IsMissionDebugSuccess(lua_State* L) {
    uint8_t success = 0;
    if (MissionMgr_GetCurrent(missionMgr)[0xE] && MissionMgr_GetCurrent(missionMgr)[0xC]) {
        success = 1;
    }
    Script::PushBool(L, success);
    return 1;
}
NB_REVERSED(IsMissionDebugSuccess, 0x005C0860);

int __cdecl Native_MissionPlayActionNode(lua_State* L) {
    void* node = ActionTree_Find(Script::GetString(L, 0), 0);
    if (node) {
        ActionController_Play(missionActionController, node);
        Script::PushBool(L, 1);
        return 1;
    }
    Script::PushBool(L, 0);
    return 1;
}
NB_REVERSED(MissionPlayActionNode, 0x005C0930);

// (the break flag only read with exactly two arguments; no object check)
int __cdecl Native_ObjectBreak(lua_State* L) {
    int count = lua::gettop(L);
    int handle = Script::GetInt(L, 0);
    uint8_t flag = 1;
    if (count == 2) {
        flag = Script::GetBool(L, 1);
    }
    Object_Break(Pools_GetObject(handle), flag);
    return 0;
}
NB_REVERSED(ObjectBreak, 0x005C2490);

// (with two arguments the second is the strength, else 0)
int __cdecl Native_PedAddBroadcastStimulus(lua_State* L) {
    int id = Script::GetInt(L, 0);
    if (lua::gettop(L) == 2) {
        Stimulus_AddBroadcast(id, Script::GetFloat(L, 1));
        return 0;
    }
    Stimulus_AddBroadcast(id, 0.0f);
    return 0;
}
NB_REVERSED(PedAddBroadcastStimulus, 0x005C6F20);

// ---- prop anims (4) ----------------------------------------------------------------

// (vf 0x78)
int __cdecl Native_PAnimIsDestroyed(lua_State* L) {
    uint8_t* prop = PropAnimArg(L);
    uint8_t destroyed = 0;
    if (prop) {
        destroyed = CallVirtual<uint8_t>(prop, 0x78 / 4);
    }
    Script::PushBool(L, destroyed);
    return 1;
}
NB_REVERSED(PAnimIsDestroyed, 0x005C4B00);

int __cdecl Native_PAnimGetHealth(lua_State* L) {
    uint8_t* prop = PropAnimArg(L);
    float health = 0.0f;
    if (prop) {
        health = PropAnim_GetHealth(prop);
    }
    Script::PushFloat(L, health);
    return 1;
}
NB_REVERSED(PAnimGetHealth, 0x005C56C0);

// (flag 1)
int __cdecl Native_PAnimSetInvulnerableToPlayer(lua_State* L) {
    uint8_t* prop = PropAnims_Get(propAnims, Script::GetInt(L, 0));
    if (prop) {
        PropAnim_SetFlag(prop, 1, Script::GetBool(L, 1));
    }
    return 0;
}
NB_REVERSED(PAnimSetInvulnerableToPlayer, 0x005C3A60);

// the prop anim from the arguments from the third, then the flag and its value
int __cdecl Native_PAnimSetPropFlag(lua_State* L) {
    int next = -1;
    uint8_t* prop = Script_GetPropAnimArg(L, 2, &next);
    if (prop) {
        int flag = Script::GetInt(L, next);
        uint8_t on = Script::GetBool(L, next + 1);
        PropAnim_SetFlag(prop, flag, on);
    }
    return 0;
}
NB_REVERSED(PAnimSetPropFlag, 0x005C5710);

// ---- peds (4) ----------------------------------------------------------------------

// (no ped check)
int __cdecl Native_PedSetPunishmentPoints(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    int points = Script::GetIntOpt(L, 1, 0);
    Ped_SetPunishmentPoints(Script_GetPed(handle, 2), points);
    return 0;
}
NB_REVERSED(PedSetPunishmentPoints, 0x005CCF50);

int __cdecl Native_PedSetSpecial(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        Ped_SetSpecial(ped, Script::GetInt(L, 1));
    }
    return 0;
}
NB_REVERSED(PedSetSpecial, 0x005C9140);

int __cdecl Native_PedSetSpecialRate(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        Ped_SetSpecialRate(ped, Script::GetFloat(L, 1));
    }
    return 0;
}
NB_REVERSED(PedSetSpecialRate, 0x005C90F0);

// (nothing pushed for no ped; truncated as _ftol does)
int __cdecl Native_PedGetSpecial(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (!ped) {
        return 0;
    }
    Script::PushInt(L, int(int64_t(At<float>(ped, 0x18C0))));
    return 1;
}
NB_REVERSED(PedGetSpecial, 0x005C9180);

// (1e-4 of a step of its action controller)
int __cdecl Native_PedActionControllerUpdate(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        ActionController_Update(At<uint8_t*>(ped, 0x2E0) + 0x258, 0.0001f);
    }
    return 0;
}
NB_REVERSED(PedActionControllerUpdate, 0x005CA2C0);

// (no checks; handle 0 is the player)
int __cdecl Native_PedGetLastHitWeapon(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    uint8_t* ped = Script_GetPed(handle, handle != 0 ? 2 : 3);
    Script::PushInt(L, At<int16_t>(Ped_GetLastHitWeapon(ped), 0x5E));
    return 1;
}
NB_REVERSED(PedGetLastHitWeapon, 0x005CD660);

// (negative amounts ignored)
int __cdecl Native_PedSetMoney(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        int money = Script::GetInt(L, 1);
        if (money >= 0) {
            Ped_SetMoney(ped, money);
        }
    }
    return 0;
}
NB_REVERSED(PedSetMoney, 0x005C9220);

int __cdecl Native_PedSetTetherSpeed(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        At<float>(ped, 0x3B8) = Script_GetTetherSpeed(L, 1);
    }
    return 0;
}
NB_REVERSED(PedSetTetherSpeed, 0x005CC7F0);

int __cdecl Native_PedSetEffectedByGravity(lua_State* L) {
    uint8_t* ped = PedArg(L);
    uint8_t on = Script::GetBool(L, 1);
    if (ped) {
        Ped_SetEffectedByGravity(ped, on, 0);
    }
    return 0;
}
NB_REVERSED(PedSetEffectedByGravity, 0x005CD580);

int __cdecl Native_PedSetObjectiveRemovedCallback(lua_State* L) {
    if (Script::IsNumber(L, 0)) {
        uint8_t* ped = PedArg(L);
        if (ped) {
            Script_SetCallback2(L, 1, ped + 0x650);
        }
    }
    return 0;
}
NB_REVERSED(PedSetObjectiveRemovedCallback, 0x005CE610);

// its hit record (vf 0x40): +0x10 -> +0x38 the damage (nothing pushed for no ped)
int __cdecl Native_PedGetHitRecordDamage(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (!ped) {
        return 0;
    }
    uint8_t* record = CallVirtual<uint8_t*>(ped, 0x40 / 4);
    Script::PushFloat(L, At<float>(At<uint8_t*>(record, 0x10), 0x38));
    return 1;
}
NB_REVERSED(PedGetHitRecordDamage, 0x005C9970);

int __cdecl Native_PedSetHitRecordDamage(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    float damage = Script::GetFloat(L, 1);
    uint8_t* ped = Script_GetPed(handle, 2);
    if (ped) {
        uint8_t* record = CallVirtual<uint8_t*>(ped, 0x40 / 4);
        At<float>(At<uint8_t*>(record, 0x10), 0x38) = damage;
    }
    return 0;
}
NB_REVERSED(PedSetHitRecordDamage, 0x005C9920);

// (the trigger as 16 bits)
int __cdecl Native_PedRestrictToTrigger(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        PedTether_SetTrigger(ped + 0x358, uint16_t(Script::GetInt(L, 1)));
    }
    return 0;
}
NB_REVERSED(PedRestrictToTrigger, 0x005C9B40);

int __cdecl Native_PedSetTetherToTrigger(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        int trigger = Script::GetInt(L, 1);
        if (trigger >= 0 && scriptPropAnimPool->GetAt(trigger)) {
            PedTether_SetTrigger(ped + 0x358, trigger);
        }
    }
    return 0;
}
NB_REVERSED(PedSetTetherToTrigger, 0x005CC6E0);

int __cdecl Native_PedSetAntiTetherToTrigger(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        int trigger = Script::GetInt(L, 1);
        if (trigger >= 0 && scriptPropAnimPool->GetAt(trigger)) {
            PedTether_SetAntiTrigger(ped + 0x358, trigger);
        }
    }
    return 0;
}
NB_REVERSED(PedSetAntiTetherToTrigger, 0x005CCB00);

int __cdecl Native_PedOverrideStat(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        int stat = Script::GetInt(L, 1);
        int value = Script::GetInt(L, 2);
        Ped_OverrideStat(ped, stat, value);
    }
    return 0;
}
NB_REVERSED(PedOverrideStat, 0x005C91D0);

int __cdecl Native_PedSetPedToTypeAttitude(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        int type = Script::GetInt(L, 1);
        int attitude = Script::GetInt(L, 2);
        Ped_SetPedToTypeAttitude(ped, type, attitude);
    }
    return 0;
}
NB_REVERSED(PedSetPedToTypeAttitude, 0x005CB5E0);

int __cdecl Native_PedIsOnScreen(lua_State* L) {
    uint8_t* ped = PedArg(L);
    Script::PushBool(L, ped ? Ped_IsOnScreen(ped) : 0);
    return 1;
}
NB_REVERSED(PedIsOnScreen, 0x005C94A0);

// the node of its flee-on-bike objective (type 13); nothing pushed otherwise
int __cdecl Native_PedFleeOnBikeGetNextNode(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        uint8_t* objective = PedObjectives_GetCurrent(ped + 0x5F0);
        if (objective && At<int32_t>(objective, 8) == 13) {
            Script::PushInt(L, At<int32_t>(objective, 0x14));
            return 1;
        }
    }
    return 0;
}
NB_REVERSED(PedFleeOnBikeGetNextNode, 0x005C7CE0);

// its current objective is an attack (7) or 16
int __cdecl Native_PedIsInCombat(lua_State* L) {
    uint8_t* ped = PedArg(L);
    uint8_t combat = 0;
    if (ped) {
        uint8_t* objective = PedObjectives_GetCurrent(ped + 0x5F0);
        if (objective) {
            int type = At<int32_t>(objective, 8);
            if (type == 7 || type == 16) {
                combat = 1;
            }
        }
    }
    Script::PushBool(L, combat);
    return 1;
}
NB_REVERSED(PedIsInCombat, 0x005CDE40);

// its last vehicle's handle, or nil (no ped check)
int __cdecl Native_PedGetLastVehicle(lua_State* L) {
    void* vehicle = At<void*>(PedArg(L), 0x1560);
    if (vehicle) {
        Script::PushInt(L, Pools_GetVehicleRef(vehicle));
        return 1;
    }
    Script::PushNil(L);
    return 1;
}
NB_REVERSED(PedGetLastVehicle, 0x005CEB80);

int __cdecl Native_PedIsFemale(lua_State* L) {
    uint8_t* ped = PedArg(L);
    Script::PushBool(L, ped ? Ped_GetModelInfo(ped)[0x50] : 0);
    return 1;
}
NB_REVERSED(PedIsFemale, 0x005CED50);

// the handle (vf 0xAC) of who it grapples, -1 for none; nothing pushed for no ped
int __cdecl Native_PedGetGrappleTargetPed(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (!ped) {
        return 0;
    }
    void* target = At<void*>(ped, 0x155C);
    Script::PushInt(L, target ? CallVirtual<int>(target, 0xAC / 4) : -1);
    return 1;
}
NB_REVERSED(PedGetGrappleTargetPed, 0x005CB7D0);

// A ped's ally (+0x2F4 leader, +0x2F8 follower) as a handle, 0 for none
static int PushPedAllyHandle(lua_State* L, uint32_t offset) {
    uint8_t* ped = PedArg(L);
    void* ally = ped ? At<void*>(ped, offset) : nullptr;
    Script::PushInt(L, ally ? CallVirtual<int>(ally, 0xAC / 4) : 0);
    return 1;
}

int __cdecl Native_PedGetAllyLeader(lua_State* L) { return PushPedAllyHandle(L, 0x2F4); }
NB_REVERSED(PedGetAllyLeader, 0x005CD990);
int __cdecl Native_PedGetAllyFollower(lua_State* L) { return PushPedAllyHandle(L, 0x2F8); }
NB_REVERSED(PedGetAllyFollower, 0x005CD9F0);

// the callback for a stimulus (+0xCB4, refs; +0xD1C cleared)
int __cdecl Native_PedRegisterSocialCallback(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        int stimulus = Script::GetInt(L, 1);
        Script_SetCallback(L, 2, ped + 0xCB4 + stimulus * 4);
        ped[stimulus + 0xD1C] = 0;
    }
    return 0;
}
NB_REVERSED(PedRegisterSocialCallback, 0x005CB0A0);

// three zones from arguments 2-4 (no ped check)
int __cdecl Native_PedSetCombatZoneMask(lua_State* L) {
    uint8_t* ped = PedArg(L);
    for (int zone = 0; zone < 3; zone++) {
        PedCombatZones_Set(ped + 0x668, zone, Script::GetBool(L, zone + 1));
    }
    return 0;
}
NB_REVERSED(PedSetCombatZoneMask, 0x005CD220);

int __cdecl Native_PedSetAlpha(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        int alpha = Script::GetInt(L, 1);
        uint8_t flag = Script::GetBool(L, 2);
        Ped_SetAlpha(ped, alpha, flag);
    }
    return 0;
}
NB_REVERSED(PedSetAlpha, 0x005CC090);

int __cdecl Native_PedGetAmmoCount(lua_State* L) {
    uint8_t* ped = PedArg(L);
    int ammo = 0;
    if (ped) {
        int model = Script::GetInt(L, 1);
        ammo = WeaponInventory_GetAmmo(At<void*>(ped, 0x1C4), model);
    }
    Script::PushInt(L, ammo);
    return 1;
}
NB_REVERSED(PedGetAmmoCount, 0x005CEE00);

// its model's name hashed, -1 for no ped
int __cdecl Native_PedGetNameHashID(lua_State* L) {
    uint8_t* ped = PedArg(L);
    int hash = -1;
    if (ped) {
        CBaseModelInfo* info = CModelInfo::ms_modelInfoPtrs[At<int16_t>(ped, 0x10E)];
        if (info) {
            hash = int(HashName(reinterpret_cast<const char*>(info) + 0x98));
        }
    }
    Script::PushInt(L, hash);
    return 1;
}
NB_REVERSED(PedGetNameHashID, 0x005CF060);

int __cdecl Native_PedInConversation(lua_State* L) {
    uint8_t* ped = PedArg(L);
    Script::PushBool(L, ped ? Conversations_IsInOne(conversations, ped) : 0);
    return 1;
}
NB_REVERSED(PedInConversation, 0x005CA420);

int __cdecl Native_PedSetPosXYZ(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    ScriptVector pos;
    pos.x = Script::GetFloat(L, 1);
    pos.y = Script::GetFloat(L, 2);
    pos.z = Script::GetFloat(L, 3);
    uint8_t* ped = Script_GetPed(handle, 2);
    if (ped) {
        Script_SetPedPos(ped, &pos);
    }
    return 0;
}
NB_REVERSED(PedSetPosXYZ, 0x005C83B0);

// (handle 0 is the player; no ped check)
int __cdecl Native_PedIsSpotted(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    uint8_t* ped = handle == 0 ? static_cast<uint8_t*>(playerPed) : Script_GetPed(handle, 2);
    int by = Script::GetInt(L, 1);
    Script::PushBool(L, Ped_IsSpotted(ped, by));
    return 1;
}
NB_REVERSED(PedIsSpotted, 0x005CE340);

// takes the weapon (1000 of its ammo); if it's the one in hand, its action tree's
// weapon (+0x18) goes to -1
int __cdecl Native_PedClearWeapon(lua_State* L) {
    uint8_t* ped = PedArg(L);
    if (ped) {
        int model = Script::GetInt(L, 1);
        WeaponInventory_Remove(At<void*>(ped, 0x1C4), model, 1000);
        if (At<int16_t>(At<uint8_t*>(ped, 0x1D0), 0x10E) == model) {
            At<int32_t>(At<uint8_t*>(ped, 0x2E0), 0x18) = -1;
        }
    }
    return 0;
}
NB_REVERSED(PedClearWeapon, 0x005C9720);

// (handle 0 is the player; no ped check)
int __cdecl Native_PedIsPlaying(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    uint8_t* ped = Script_GetPed(handle, handle != 0 ? 2 : 3);
    uint8_t* tree = At<uint8_t*>(ped, 0x2E0);
    uint8_t flag = Script::GetBool(L, 2);
    const char* node = Script::GetString(L, 1);
    Script::PushBool(L, ActionController_IsPlaying(tree + 0x258, node, flag));
    return 1;
}
NB_REVERSED(PedIsPlaying, 0x005CB990);
