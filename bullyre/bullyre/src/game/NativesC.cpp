// Script natives: the ones at 0x5D0000-0x5DFFFF (player, shop, sound, stats, system,
// text, vehicles) and the minigame / class ones (0x680000-0x71FFFF, 0x820000-). Each is
// `int __cdecl Native_X(lua_State* L)`, returning how many results it pushed.
// Registered under their script names (lua_natives.csv); where several script names
// share one function, under the first and the others in a comment.
//
// Game objects touched here only for a field or two are read through At() with the
// field's offset; their layouts belong to the files reversing those classes.

#include <cstring>

#include "game/NativesC.h"

namespace {

// ---- the player ----------------------------------------------------------------------

GAME_FN(void, __thiscall, PlayerInfo_SetPhysicalState, 0x00445E30, void* info, int);
GAME_FN(void, __thiscall, PlayerInfo_RefreshPhysicalState, 0x00446B40, void* info);
GAME_FN(void, __thiscall, PlayerPed_SetMoney, 0x004752A0, void* ped, int money);
GAME_FN(void, __thiscall, PlayerPed_SetPunishmentPoints, 0x00477520, void* ped, int points);
GAME_FN(void, __thiscall, Punishment_Scale, 0x0044E310, void* punishment, float scale);
GAME_FN(void, __thiscall, WeaponHud_Lock, 0x0055BC70, void* hud, uint8_t lock);

int __cdecl Native_PlayerResetPhysicalState(lua_State*) {
    PlayerInfo_SetPhysicalState(playerInfo, 0);
    return 0;
}

int __cdecl Native_PlayerRefreshPhysicalState(lua_State*) {
    PlayerInfo_RefreshPhysicalState(playerInfo);
    return 0;
}

int __cdecl Native_PlayerSuppressFailDisplayOnDeath(lua_State*) {
    At<uint8_t>(playerInfo, 0xC4) |= 8;
    return 0;
}

int __cdecl Native_PlayerGetMoney(lua_State* L) {
    Script::PushInt(L, At<int32_t>(playerPed, 0x1CA0));
    return 1;
}

int __cdecl Native_PlayerSetMoney(lua_State* L) {
    int money = Script::GetInt(L, 0);
    if (money >= 0) {
        PlayerPed_SetMoney(playerPed, money);
    }
    return 0;
}

int __cdecl Native_PlayerSetClassSkipCount(lua_State* L) {
    At<int32_t>(punishment, 0xC) = Script::GetInt(L, 0);
    return 0;
}

// (reads its argument and does nothing with it)
int __cdecl Native_PlayerSetEnableFirstPerson(lua_State* L) {
    Script::GetBool(L, 0);
    return 0;
}

int __cdecl Native_PlayerGetPunishmentPoints(lua_State* L) {
    Script::PushInt(L, At<int32_t>(playerPed, 0x1D40));
    return 1;
}

int __cdecl Native_PlayerSetPunishmentPoints(lua_State* L) {
    void* ped = playerPed;
    PlayerPed_SetPunishmentPoints(ped, Script::GetInt(L, 0));
    return 0;
}

int __cdecl Native_PlayerScalePunishmentPoints(lua_State* L) {
    Punishment_Scale(punishment, Script::GetFloat(L, 0));
    return 0;
}

// (the ped's virtual at +0xB4)
int __cdecl Native_PlayerIncPunishmentPoints(lua_State* L) {
    void* ped = playerPed;
    VCall<void>(ped, 0xB4, Script::GetInt(L, 0));
    return 0;
}

int __cdecl Native_PlayerGetNumTimesBusted(lua_State* L) {
    int count = 0;
    if (playerPed != nullptr) {
        count = At<int32_t>(playerPed, 0x1ED4);
    }
    Script::PushInt(L, count);
    return 1;
}

int __cdecl Native_PlayerSetMinPunishmentPoints(lua_State* L) {
    void* ped = playerPed;
    if (ped != nullptr) {
        At<int32_t>(ped, 0x1EBC) = Script::GetInt(L, 0);
    }
    return 0;
}

// (the argument is read and ignored)
int __cdecl Native_PlayerDoneLoading(lua_State* L) {
    Script::GetBool(L, 0);
    if (playerPed != nullptr) {
        At<uint8_t>(playerPed, 0x1E8C) = 1;
    }
    return 0;
}

int __cdecl Native_PlayerIgnoreTargeting(lua_State* L) {
    At<uint8_t>(targeting, 0x20) = Script::GetBool(L, 0);
    return 0;
}

// (the ped's virtual at +0xA8, with 0, 0)
int __cdecl Native_PlayerKilled(lua_State*) {
    VCall<void>(playerPed, 0xA8, 0, 0);
    At<uint8_t>(playerInfo, 0x58) = 1;
    return 0;
}

int __cdecl Native_PlayerGetPhysicalState(lua_State* L) {
    Script::PushInt(L, At<int8_t>(playerInfo, 0xA4));
    return 1;
}

int __cdecl Native_PlayerIsInStealthProp(lua_State* L) {
    Script::PushBool(L, At<uint8_t>(playerPed, 0x1E91));
    return 1;
}

int __cdecl Native_PlayerClearLastVehicle(lua_State*) {
    if (playerPed != nullptr) {
        At<void*>(playerPed, 0x1560) = nullptr;
    }
    return 0;
}

int __cdecl Native_PlayerGetHealth(lua_State* L) {
    Script::PushFloat(L, At<float>(GetPedFromHandle(0, PED_HANDLE_PLAYER), 0x1CB8));
    return 1;
}

int __cdecl Native_PlayerActivateCombatSphere(lua_State* L) {
    void* ped = GetPedFromHandle(0, PED_HANDLE_PLAYER);
    At<int32_t>(ped, 0x151C) = Script::GetBool(L, 0);
    return 0;
}

// (a non-zero low word)
int __cdecl Native_PlayerSetHighRisk(lua_State* L) {
    int value = Script::GetInt(L, 0);
    At<uint8_t>(punishment, 2) = uint16_t(value) != 0;
    At<int32_t>(punishment, 4) = 0;
    return 0;
}

int __cdecl Native_PlayerWeaponHudLock(lua_State* L) {
    WeaponHud_Lock(weaponHud, Script::GetBool(L, 0));
    return 0;
}

// (the meter's virtual at +0x14)
int __cdecl Native_DetentionMeterSetVisibility(lua_State* L) {
    uint8_t visible = Script::GetBool(L, 0);
    VCall<void>(detentionMeter, 0x14, visible);
    return 0;
}

// (no bounds check on the index)
int __cdecl Native_PlayerSetScriptSavedData(lua_State* L) {
    int index = Script::GetInt(L, 0);
    int value = Script::GetInt(L, 1);
    At<int32_t>(playerInfo, 0x110 + index * 4) = value;
    return 0;
}

int __cdecl Native_PlayerGetScriptSavedData(lua_State* L) {
    Script::PushInt(L, At<int32_t>(playerInfo, 0x110 + Script::GetInt(L, 0) * 4));
    return 1;
}

int __cdecl Native_PlayerSetGrade(lua_State* L) {
    int grade = Script::GetInt(L, 1);
    int index = Script::GetInt(L, 0);
    At<int32_t>(playerInfo, 0x184 + index * 4) = grade;
    return 0;
}

GAME_FN(uint8_t, __thiscall, Ped_IsOnBike, 0x004CE690, void* ped);
GAME_FN(void, __thiscall, PedAttachment_Detach, 0x005F3A90, void* attachment, int, int, float);
GAME_FN(void, __thiscall, Ped_SetHealth, 0x004782E0, void* ped, float health);
GAME_FN(int, __cdecl, GetVehicleRef, 0x0044CAC0, void* vehicle);
GAME_FN(void, __thiscall, Upgrades_Unlock, 0x0045C170, void* upgrades, const char* name);
GAME_VAR(void*, upgrades, 0x00C1AA04);

int __cdecl Native_PlayerIsInAnyVehicle(lua_State* L) {
    uint8_t* ped = GetPedFromHandle(0, PED_HANDLE_PLAYER);
    Script::PushBool(L, At<void*>(ped, 0x1554) != nullptr);
    return 1;
}

// (+0x2E0's detach with 3, 1, 0.0f, when on a bike)
int __cdecl Native_PlayerDismountBike(lua_State*) {
    uint8_t* ped = GetPedFromHandle(0, PED_HANDLE_PLAYER);
    if (Ped_IsOnBike(ped)) {
        PedAttachment_Detach(At<void*>(ped, 0x2E0), 3, 1, 0.0f);
    }
    return 0;
}

int __cdecl Native_PlayerSetHealth(lua_State* L) {
    float health = Script::GetFloat(L, 0);
    Ped_SetHealth(GetPedFromHandle(0, PED_HANDLE_PLAYER), health);
    return 0;
}

int __cdecl Native_PlayerSetInvulnerable(lua_State* L) {
    uint8_t on = Script::GetBool(L, 0);
    At<int32_t>(GetPedFromHandle(0, PED_HANDLE_PLAYER), 0x138C) = on;
    return 0;
}

int __cdecl Native_PlayerSetTakeDamageWhenAttachedToVehicle(lua_State* L) {
    uint8_t on = Script::GetBool(L, 0);
    At<int32_t>(GetPedFromHandle(0, PED_HANDLE_PLAYER), 0x1454) = on;
    return 0;
}

// The handle of the vehicle at a ped field, -1 for none
int PushPlayerVehicle(lua_State* L, uint32_t offset) {
    int handle = -1;
    if (playerPed != nullptr && At<void*>(playerPed, offset) != nullptr) {
        handle = GetVehicleRef(At<void*>(playerPed, offset));
    }
    Script::PushInt(L, handle);
    return 1;
}

int __cdecl Native_PlayerGetBikeId(lua_State* L) {
    return PushPlayerVehicle(L, 0x1554);
}

int __cdecl Native_PlayerGetLastBikeId(lua_State* L) {
    return PushPlayerVehicle(L, 0x1560);
}

// (the model of the weapon held, +0x1D0; no check there is one)
int __cdecl Native_PlayerHasWeapon(lua_State* L) {
    int model = Script::GetInt(L, 0);
    uint8_t* weapon = At<uint8_t*>(playerPed, 0x1D0);
    Script::PushBool(L, At<int16_t>(weapon, 0x10E) == model);
    return 1;
}

int __cdecl Native_PlayerFellAsleep(lua_State* L) {
    uint8_t state = At<uint8_t>(playerInfo, 0x58);
    Script::PushBool(L, state == 7 || state == 6);
    return 1;
}

// (one physical state up or down, in a byte)
int __cdecl Native_PlayerChangePhysicalState(lua_State* L) {
    int8_t state = At<int8_t>(playerInfo, 0xA4);
    if (Script::GetBool(L, 0)) {
        state = int8_t(state + 1);
    } else {
        state = int8_t(state - 1);
    }
    PlayerInfo_SetPhysicalState(playerInfo, state);
    return 0;
}

// (copied to a 24-byte buffer first: a longer name overruns it, as in the game)
int __cdecl Native_PlayerUnlockUpgrade(lua_State* L) {
    char name[24];
    strcpy(name, Script::GetString(L, 0));
    Upgrades_Unlock(upgrades, name);
    return 0;
}

int __cdecl Native_PlayerClearRewardStore(lua_State*) {
    At<uint8_t>(playerInfo, 0x10C) = 0;
    At<uint8_t>(playerInfo, 0x10D) = 0;
    At<uint8_t>(playerInfo, 0x10E) = 0;
    At<int32_t>(playerInfo, 0x100) = -1;
    At<int32_t>(playerInfo, 0x104) = 15;
    At<int32_t>(playerInfo, 0x108) = 6;
    At<uint8_t>(playerInfo, 0x10F) = 1;
    return 0;
}

// (reads its argument and does nothing: the achievements aren't on PC)
int __cdecl Native_AwardAchievement(lua_State* L) {
    Script::GetString(L, 0);
    return 0;
}

// ---- POIs, pickups -----------------------------------------------------------------------

GAME_VAR(uint8_t*, poiManager, 0x00C0F5E4);
GAME_VAR(uint8_t*, poiGroups, 0x00C2C108);
GAME_VAR(uint8_t, ignoreRespawnDistance, 0x00C0A0AC);
GAME_FN(void, __cdecl, Pickup_RemoveAllOfType, 0x00444B50, int type);
GAME_FN(void, __cdecl, Pickup_RemoveAll, 0x00444BB0);

int __cdecl Native_POISetSystemEnabled(lua_State* L) {
    uint8_t* manager = poiManager;
    At<uint8_t>(manager, 0xA4) = Script::GetBool(L, 0);
    return 0;
}

// (bit 4 of +0x7898)
int __cdecl Native_POIGroupsEnabled(lua_State* L) {
    uint8_t* groups = poiGroups;
    uint8_t enabled = Script::GetBool(L, 0);
    uint8_t& flags = At<uint8_t>(groups, 0x7898);
    flags ^= (uint8_t(enabled << 4) ^ flags) & 0x10;
    return 0;
}

// (reads its arguments and does nothing with them)
int __cdecl Native_PickupMakeTargetable(lua_State* L) {
    Script::GetInt(L, 0);
    Script::GetBool(L, 1);
    return 0;
}

int __cdecl Native_PickupSetIgnoreRespawnDistance(lua_State* L) {
    ignoreRespawnDistance = Script::GetBool(L, 0);
    return 0;
}

GAME_FN(uint8_t*, __thiscall, POIManager_GetPOI, 0x00448DC0, void* manager, int index);

// (+0x7C: 10 when set, 9 when not)
int __cdecl Native_POISetScriptedPedExitObjective(lua_State* L) {
    uint8_t* poi = POIManager_GetPOI(poiManager, Script::GetInt(L, 0));
    int exit = Script::GetInt(L, 1);
    if (poi != nullptr) {
        At<int32_t>(poi, 0x7C) = (exit != 0) + 9;
    }
    return 0;
}

int __cdecl Native_POISetDisablePedProduction(lua_State* L) {
    uint8_t* poi = POIManager_GetPOI(poiManager, Script::GetInt(L, 0));
    if (poi != nullptr) {
        At<uint8_t>(poi, 0x83) = Script::GetBool(L, 1);
    }
    return 0;
}

int __cdecl Native_PickupRemoveAll(lua_State* L) {
    if (lua::gettop(L) > 0) {
        Pickup_RemoveAllOfType(Script::GetInt(L, 0));
        return 0;
    }
    Pickup_RemoveAll();
    return 0;
}

// ---- shops -------------------------------------------------------------------------------

inline uint8_t* const shop = reinterpret_cast<uint8_t*>(0x00C18748);
inline void* const shopStopCallback = reinterpret_cast<void*>(0x00C18A20);
GAME_VAR(uint8_t, playerInShop, 0x00C1921C);
GAME_FN(void, __thiscall, Shop_End, 0x00457560, void* shop);
GAME_FN(void*, __thiscall, Shop_GetClerk, 0x0077E8D0, void* shop);
GAME_FN(int, __cdecl, GetPedHandle, 0x005C6230, void* ped);

GAME_FN(void, __thiscall, Shop_Start, 0x004574A0, void* shop, int);

int __cdecl Native_ShopStart(lua_State* L) {
    int kind = 0;
    if (lua::gettop(L) >= 1) {
        kind = Script::GetInt(L, 0);
    }
    Shop_Start(shop, kind);
    return 0;
}

int __cdecl Native_ShopEnd(lua_State*) {
    Shop_End(shop);
    return 0;
}

int __cdecl Native_ShopIsActive(lua_State* L) {
    Script::PushBool(L, At<uint8_t>(shop, 0xC));
    return 1;
}

int __cdecl Native_ShopSetIsPlayerInShop(lua_State* L) {
    playerInShop = Script::GetBool(L, 0);
    return 0;
}

int __cdecl Native_ShopSetCameraZoomPercentage(lua_State* L) {
    At<float>(shop, 0x54) = Script::GetFloat(L, 0);
    return 0;
}

int __cdecl Native_ShopSetCameraAngleOffset(lua_State* L) {
    At<float>(shop, 0x58) = Script::GetFloat(L, 0);
    return 0;
}

int __cdecl Native_ShopSetOnStopShoppingCB(lua_State* L) {
    StoreCallback(L, 0, shopStopCallback);
    Callback_IsSet(shopStopCallback);
    return 0;
}

int __cdecl Native_ShopGetClerkID(lua_State* L) {
    Script::PushInt(L, GetPedHandle(Shop_GetClerk(shop)));
    return 1;
}

// ---- sound -------------------------------------------------------------------------------

inline void* const speechFilter = reinterpret_cast<void*>(0x00CF4378);
inline void* const speech = reinterpret_cast<void*>(0x00CF4E10);
inline void* const soundManager = reinterpret_cast<void*>(0x00CF3DA0);   // also the screamer
inline void* const fireAlarm = reinterpret_cast<void*>(0x00CF3DA8);
inline void* const streamPlayer = reinterpret_cast<void*>(0x00CF42D4);
inline void* const ambienceStream = reinterpret_cast<void*>(0x00CF4324);
inline void* const soundBanks = reinterpret_cast<void*>(0x00CF5DC4);
GAME_VAR(uint8_t*, ambience, 0x00CF4374);       // +0 stopped, +1 the PA stopped
GAME_VAR(uint8_t*, ambienceEmitters, 0x00CF4278);
GAME_VAR(void*, loopSounds, 0x00CF4274);
GAME_VAR(void*, boxingCrowd, 0x00CF4280);
GAME_VAR(uint8_t, soundPaused, 0x00CF3DD7);
GAME_VAR(uint8_t, soundPaused2, 0x00CF6C71);
GAME_VAR(uint8_t, playPreloadedSpeech, 0x00CFB3BC);
GAME_VAR(uint8_t, speechPreloaded, 0x00CFB3BD);
GAME_VAR(uint8_t, speechDisabledByActionTree, 0x00CF3DA3);
GAME_VAR(uint8_t, audioFocusPlayer, 0x00CF3DA0);
GAME_VAR(uint8_t, audioFocusCamera, 0x00CF3DA1);
GAME_VAR(uint8_t, musicDuringCutscenes, 0x00CF3DA4);
GAME_VAR(uint8_t, everythingCloser, 0x00CF6C70);
GAME_VAR(int32_t, currentGoKartRace, 0x00AE6284);
GAME_VAR(uint8_t, interactiveMusicDisabled, 0x00CF3AE7);
GAME_VAR(int32_t, interactiveMusicState, 0x00CF3AEC);

GAME_FN(void, __thiscall, SpeechFilter_Clear, 0x005A09A0, void* filter);
GAME_FN(void, __thiscall, SpeechFilter_SetPriority, 0x008A20A0, void* filter, int priority);
GAME_FN(void, __thiscall, Ambience_ClearScripted, 0x00593DC0, void* ambience);
GAME_FN(void, __thiscall, Speech_Disable, 0x0059FC90, void* speech);
GAME_FN(void, __thiscall, Speech_Enable, 0x0059E8D0, void* speech);
GAME_FN(void, __thiscall, Screamer_Init, 0x00595D30, void* self);
GAME_FN(void, __thiscall, FireAlarm_Start, 0x00595920, void* alarm);
GAME_FN(void, __thiscall, FireAlarm_Stop, 0x005950B0, void* alarm);
GAME_FN(void, __thiscall, Stream_PlayPreloaded, 0x005905E0, void* stream, int);
GAME_FN(void, __thiscall, Stream_Stop, 0x00591480, void* stream, int);
GAME_FN(void, __thiscall, Stream_Fadeout, 0x005913D0, void* stream, int, int time);
GAME_FN(void, __cdecl, InteractiveStream_Stop, 0x00591A70, int);
GAME_FN(void, __thiscall, BoxingCrowd_Start, 0x0058DA80, void* crowd);
GAME_FN(void, __thiscall, BoxingCrowd_Kill, 0x0058DAC0, void* crowd);
GAME_FN(void, __thiscall, BoxingCrowd_PlayBoo, 0x0058D900, void* crowd);
GAME_FN(void, __thiscall, BoxingCrowd_PlayCheer, 0x0058D940, void* crowd);
GAME_FN(void, __thiscall, BoxingCrowd_SetExcitement, 0x0058DA60, void* crowd, int level);
GAME_FN(int, __thiscall, SoundBanks_Find, 0x00597FC0, void* banks, const char* name);
GAME_FN(void, __thiscall, SoundBanks_Load, 0x0059A3E0, void* banks, int bank, int, int);
GAME_FN(void, __thiscall, SoundBanks_Unload, 0x0059A420, void* banks, int bank, int, int);
GAME_FN(int, __thiscall, Sound_FindLoop, 0x00595B80, void* manager, const char* name);
GAME_FN(void, __thiscall, LoopSounds_Stop, 0x00592400, void* loops, uintptr_t, uintptr_t, int sound);
GAME_FN(void, __cdecl, FadeWithCamera, 0x0059E5E0, int music, uint8_t fade);

int __cdecl Native_SoundClearSpeechFilter(lua_State*) {
    SpeechFilter_Clear(speechFilter);
    return 0;
}

// (also SoundRemoveSpeechPriorityFilter and SoundSetSpeechPriorityFilter)
int __cdecl Native_SoundAddSpeechPriorityFilter(lua_State* L) {
    SpeechFilter_SetPriority(speechFilter, Script::GetInt(L, 0));
    return 0;
}

int __cdecl Native_SoundClearScriptedAmbience(lua_State*) {
    Ambience_ClearScripted(ambience);
    return 0;
}

int __cdecl Native_SoundDisableSpeech(lua_State*) {
    Speech_Disable(speech);
    return 0;
}

int __cdecl Native_SoundEnableSpeech(lua_State*) {
    Speech_Enable(speech);
    return 0;
}

int __cdecl Native_SoundDisableSpeech_ActionTree(lua_State*) {
    uint8_t* amb = ambience;
    speechDisabledByActionTree = 1;
    if (amb != nullptr) {
        amb[1] = 1;
    }
    return 0;
}

int __cdecl Native_SoundEnableSpeech_ActionTree(lua_State*) {
    uint8_t* amb = ambience;
    speechDisabledByActionTree = 0;
    if (amb != nullptr) {
        amb[1] = 0;
    }
    return 0;
}

int __cdecl Native_SoundInitScreamer(lua_State*) {
    Screamer_Init(soundManager);
    return 0;
}

int __cdecl Native_SoundPause(lua_State*) {
    soundPaused = 1;
    soundPaused2 = 1;
    return 0;
}

int __cdecl Native_SoundContinue(lua_State*) {
    soundPaused = 0;
    soundPaused2 = 0;
    return 0;
}

int __cdecl Native_SoundPlayPreloadedSpeech(lua_State*) {
    playPreloadedSpeech = 1;
    return 0;
}

int __cdecl Native_SoundIsSpeechPreloaded(lua_State* L) {
    Script::PushBool(L, speechPreloaded);
    return 1;
}

int __cdecl Native_SoundStartFireAlarm(lua_State*) {
    FireAlarm_Start(fireAlarm);
    return 0;
}

int __cdecl Native_SoundStopFireAlarm(lua_State*) {
    FireAlarm_Stop(fireAlarm);
    return 0;
}

int __cdecl Native_SoundPlayPreloadedStream(lua_State*) {
    Stream_PlayPreloaded(streamPlayer, 0);
    return 0;
}

int __cdecl Native_SoundStopStream(lua_State*) {
    Stream_Stop(streamPlayer, 0);
    return 0;
}

int __cdecl Native_SoundStopInteractiveStream(lua_State* L) {
    InteractiveStream_Stop(Script::GetIntOpt(L, 0, -1));
    return 0;
}

int __cdecl Native_SoundFadeoutStream(lua_State* L) {
    Stream_Fadeout(streamPlayer, 0, Script::GetIntOpt(L, 0, -1));
    return 0;
}

int __cdecl Native_SoundFadeoutAmbience(lua_State* L) {
    Stream_Fadeout(ambienceStream, 0, Script::GetIntOpt(L, 0, -1));
    return 0;
}

int __cdecl Native_SoundStopPA(lua_State*) {
    ambience[1] = 1;
    return 0;
}

int __cdecl Native_SoundRestartPA(lua_State*) {
    ambience[1] = 0;
    return 0;
}

int __cdecl Native_SoundStopAmbiences(lua_State*) {
    ambience[0] = 1;
    At<uint8_t>(ambienceEmitters, 0xA8) = 1;
    return 0;
}

int __cdecl Native_SoundRestartAmbiences(lua_State*) {
    ambience[0] = 0;
    At<uint8_t>(ambienceEmitters, 0xA8) = 0;
    return 0;
}

int __cdecl Native_SoundSetAudioFocusPlayer(lua_State*) {
    audioFocusPlayer = 1;
    audioFocusCamera = 0;
    return 0;
}

int __cdecl Native_SoundSetAudioFocusCamera(lua_State*) {
    audioFocusPlayer = 0;
    audioFocusCamera = 1;
    return 0;
}

int __cdecl Native_SoundBoxingCrowdSoundStart(lua_State*) {
    BoxingCrowd_Start(boxingCrowd);
    return 0;
}

int __cdecl Native_SoundBoxingCrowdSoundKill(lua_State*) {
    BoxingCrowd_Kill(boxingCrowd);
    return 0;
}

int __cdecl Native_SoundBoxingCrowdSoundPlayBoo(lua_State*) {
    BoxingCrowd_PlayBoo(boxingCrowd);
    return 0;
}

int __cdecl Native_SoundBoxingCrowdSoundPlayCheer(lua_State*) {
    BoxingCrowd_PlayCheer(boxingCrowd);
    return 0;
}

int __cdecl Native_SoundBoxingCrowdSoundSetExcitementLevel(lua_State* L) {
    int level = Script::GetInt(L, 0);
    BoxingCrowd_SetExcitement(boxingCrowd, level);
    return 0;
}

int __cdecl Native_SoundSetCurrentGoKartRace(lua_State* L) {
    currentGoKartRace = Script::GetIntOpt(L, 0, -1);
    return 0;
}

int __cdecl Native_SoundMakeEverythingCloser(lua_State* L) {
    everythingCloser = Script::GetBoolOpt(L, 0, 0);
    return 0;
}

int __cdecl Native_MusicAllowPlayDuringCutscenes(lua_State* L) {
    musicDuringCutscenes = Script::GetBoolOpt(L, 0, 0);
    return 0;
}

int __cdecl Native_MusicFadeWithCamera(lua_State* L) {
    FadeWithCamera(1, Script::GetBoolOpt(L, 0, 1));
    return 0;
}

int __cdecl Native_CutsceneFadeWithCamera(lua_State* L) {
    FadeWithCamera(0, Script::GetBoolOpt(L, 0, 1));
    return 0;
}

int __cdecl Native_SoundEnableInteractiveMusic(lua_State* L) {
    uint8_t disabled = Script::GetBoolOpt(L, 0, 1) == 0;
    interactiveMusicDisabled = disabled;
    if (!disabled) {
        interactiveMusicState = 2;
    }
    return 0;
}

int __cdecl Native_SoundLoadBank(lua_State* L) {
    int bank = SoundBanks_Find(soundBanks, Script::GetString(L, 0));
    if (bank >= 0) {
        SoundBanks_Load(soundBanks, bank, 1, 1);
    }
    return 0;
}

// (bank 0 is never unloaded: > 0, where loading takes >= 0)
int __cdecl Native_SoundUnLoadBank(lua_State* L) {
    int bank = SoundBanks_Find(soundBanks, Script::GetString(L, 0));
    if (bank > 0) {
        SoundBanks_Unload(soundBanks, bank, 1, 1);
    }
    return 0;
}

int __cdecl Native_SoundLoopStop3D(lua_State* L) {
    int sound = Sound_FindLoop(soundManager, Script::GetString(L, 0));
    LoopSounds_Stop(loopSounds, 0, 0, sound);
    return 0;
}

GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void, __thiscall, AmbienceEmitters_Enable, 0x005873C0, void* emitters, uint32_t hash, uint8_t on);
GAME_FN(int, __thiscall, Stream_GetState, 0x0058EDC0, void* stream);
GAME_FN(uint8_t, __thiscall, Stream_IsPreloaded, 0x0058EE40, void* stream);

int __cdecl Native_SoundEmitterEnable(lua_State* L) {
    uint8_t on = Script::GetBool(L, 1);
    uint32_t hash = HashName(Script::GetString(L, 0));
    AmbienceEmitters_Enable(ambienceEmitters, hash, on);
    return 0;
}

int __cdecl Native_SoundMusicJimmyComeToTheOfficePA(lua_State* L) {
    uint8_t on = Script::GetBool(L, 0);
    ambience[2] = on;
    ambience[3] = 0;
    At<int32_t>(ambience, 8) = 0;
    return 0;
}

int __cdecl Native_SoundIsPreloadReady(lua_State* L) {
    uint8_t ready = 0;
    if (Stream_GetState(streamPlayer) == 6 && Stream_IsPreloaded(streamPlayer)) {
        ready = 1;
    }
    Script::PushBool(L, ready);
    return 1;
}

int __cdecl Native_SoundFadeWithCamera(lua_State* L) {
    uint8_t fade = Script::GetBoolOpt(L, 0, 1);
    FadeWithCamera(4, fade);
    FadeWithCamera(3, fade);
    FadeWithCamera(2, fade);
    return 0;
}

// ---- stats -------------------------------------------------------------------------------

GAME_FN(void, __cdecl, Stat_SetAsFloat, 0x00458470, int stat, float value);
GAME_FN(void, __cdecl, Stat_AddToInt, 0x00458480, int stat, int amount);
GAME_FN(float, __cdecl, Stat_GetAsFloat, 0x0045A900, int stat);

GAME_VAR(uint16_t, statNumLayoutItems, 0x00C1A748);
GAME_VAR(uint8_t, statTrackingEnabled, 0x00C193E4);
GAME_FN(void, __cdecl, Stat_UpdatePrincipalStats, 0x0045A9D0);
GAME_FN(void, __cdecl, Stat_ResetAll, 0x004593F0);
GAME_FN(void, __cdecl, StatLayout_AddGroup, 0x00459150, const char* name);
GAME_FN(void, __cdecl, StatLayout_AddHeading, 0x004591A0, const char* name);
GAME_FN(void, __cdecl, StatLayout_AddStat, 0x004591F0, const char* name, int stat);
GAME_FN(void, __cdecl, Stat_SetAsInt, 0x00458460, int stat, int value);
GAME_FN(int, __cdecl, Stat_GetAsInt, 0x00459A00, int stat);
GAME_FN(int, __cdecl, Stat_GetPrincipalDiffAsInt, 0x0045A630, int stat);
GAME_FN(float, __cdecl, Stat_GetGameCompletion, 0x006A98C0);

int __cdecl Native_StatUpdatePrincipalStats(lua_State*) {
    Stat_UpdatePrincipalStats();
    return 0;
}

int __cdecl Native_StatResetAll(lua_State*) {
    Stat_ResetAll();
    return 0;
}

int __cdecl Native_StatLayoutAddGroup(lua_State* L) {
    StatLayout_AddGroup(Script::GetString(L, 0));
    return 0;
}

int __cdecl Native_StatLayoutAddHeading(lua_State* L) {
    StatLayout_AddHeading(Script::GetString(L, 0));
    return 0;
}

int __cdecl Native_StatLayoutAddStat(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    StatLayout_AddStat(name, Script::GetInt(L, 1));
    return 0;
}

int __cdecl Native_StatGetNumLayoutItems(lua_State* L) {
    Script::PushInt(L, statNumLayoutItems);
    return 1;
}

int __cdecl Native_StatSetIsTrackingEnabled(lua_State* L) {
    statTrackingEnabled = Script::GetBool(L, 0);
    return 0;
}

int __cdecl Native_StatGetGameCompletion(lua_State* L) {
    Script::PushFloat(L, Stat_GetGameCompletion());
    return 1;
}

int __cdecl Native_StatSetAsInt(lua_State* L) {
    int stat = Script::GetInt(L, 0);
    Stat_SetAsInt(stat, Script::GetInt(L, 1));
    return 0;
}

int __cdecl Native_StatGetAsInt(lua_State* L) {
    Script::PushInt(L, Stat_GetAsInt(Script::GetInt(L, 0)));
    return 1;
}

int __cdecl Native_StatGetPrincipalDiffAsInt(lua_State* L) {
    Script::PushInt(L, Stat_GetPrincipalDiffAsInt(Script::GetInt(L, 0)));
    return 1;
}

int __cdecl Native_StatSetAsFloat(lua_State* L) {
    int stat = Script::GetInt(L, 0);
    Stat_SetAsFloat(stat, Script::GetFloat(L, 1));
    return 0;
}

int __cdecl Native_StatAddToInt(lua_State* L) {
    int amount = 1;
    if (lua::gettop(L) >= 2) {
        amount = Script::GetInt(L, 1);
    }
    Stat_AddToInt(Script::GetInt(L, 0), amount);
    return 0;
}

int __cdecl Native_StatGetAsFloat(lua_State* L) {
    Script::PushFloat(L, Stat_GetAsFloat(Script::GetInt(L, 0)));
    return 1;
}

int __cdecl Native_StatGetPrincipalDiffAsFloat(lua_State* L) {
    Script::PushFloat(L, float(Stat_GetPrincipalDiffAsInt(Script::GetInt(L, 0))));
    return 1;
}

// ---- system, demo build, credits, text -----------------------------------------------

inline void* const lurStore = reinterpret_cast<void*>(0x00D02850);
GAME_VAR(void*, textHud, 0x00C674C4);
GAME_VAR(void*, textBox1, 0x00C674A8);
GAME_VAR(void*, textBox2, 0x00C674A4);
// (thiscall on the script store: the running script of a Lua state)
GAME_FN(uint8_t*, __thiscall, LurStore_GetScript, 0x005DBD50, void* store, lua_State* L);
GAME_FN(void, __thiscall, TextHud_Clear, 0x005400B0, void* hud, int line);
GAME_FN(void, __thiscall, TextBox_Clear, 0x005539D0, void* box);
GAME_FN(void, __cdecl, Credit_GetInteger, 0x0043A890, const char* text, int32_t* value);

int __cdecl Native_SystemShouldEndScript(lua_State* L) {
    uint8_t* script = LurStore_GetScript(lurStore, L);
    uint8_t end = 0;
    if (script != nullptr) {
        end = At<uint8_t>(script, 0x2B08);
    }
    Script::PushBool(L, end);
    return 1;
}

int __cdecl Native_TextClear(lua_State*) {
    void* hud = textHud;
    for (int line = 0; line < 5; line++) {
        TextHud_Clear(hud, line);
    }
    TextBox_Clear(textBox1);
    TextBox_Clear(textBox2);
    return 0;
}

int __cdecl Native_CreditGetInteger(lua_State* L) {
    const char* text = Script::GetString(L, 0);
    int32_t value = 0;
    Credit_GetInteger(text, &value);
    Script::PushInt(L, value);
    return 1;
}

GAME_VAR(uint8_t, allowMissionManagerToForceRun, 0x020C441D);
GAME_VAR(uint8_t, bustedAreaTransitionHandled, 0x00BD3E6C);
GAME_VAR(uint8_t, readyToLoad, 0x00BD0FD0);
GAME_VAR(uint8_t, systemReady, 0x00BD0FCB);
GAME_VAR(int32_t, shouldMovePlayerArea, 0x00BD0FD4);
GAME_VAR(uint8_t, frontEndAndSelectScreensEnabled, 0x00BA688D);
GAME_VAR(uint8_t, demoReturnToMain, 0x020C32EC);
GAME_VAR(uint8_t, storyModeSelected, 0x020C32F1);
GAME_VAR(int32_t, creditPosition, 0x00BF381C);
GAME_VAR(int32_t, language, 0x00C18724);
inline void* const creditDB = reinterpret_cast<void*>(0x020C2FA4);
GAME_FN(void, __cdecl, ResetShouldMovePlayer, 0x0042E720);
GAME_FN(uint8_t, __cdecl, ShouldMovePlayer, 0x0042E700);
GAME_FN(void, __cdecl, MarkCollisionForExclusion, 0x0056EDB0, const char* name);
GAME_FN(void, __cdecl, DemoBuild_ReturnToMain, 0x00458450);
GAME_FN(void, __thiscall, CreditDB_Unload, 0x0068FA40, void* db);
GAME_FN(uint8_t, __thiscall, CreditDB_Load, 0x0068FA70, void* db);
GAME_FN(uint8_t, __cdecl, Credit_FindText, 0x0043A860, const char* text);
GAME_FN(void, __cdecl, Text_SetClipRange, 0x0043A7A0, int from, int to);
GAME_FN(void, __cdecl, FontStyle_Load, 0x00687BC0, int style);
GAME_FN(void, __cdecl, FontStyle_Unload, 0x00687D10, int style);
GAME_FN(void, __cdecl, PushTxdMemoryTag, 0x0052E700);
GAME_FN(void, __cdecl, PopTxdMemoryTag, 0x0052E710);

int __cdecl Native_SystemResetShouldMovePlayer(lua_State*) {
    ResetShouldMovePlayer();
    return 0;
}

int __cdecl Native_SystemShouldMovePlayer(lua_State* L) {
    Script::PushBool(L, ShouldMovePlayer());
    Script::PushInt(L, shouldMovePlayerArea);
    return 2;
}

int __cdecl Native_SystemAllowMissionManagerToForceRunMissions(lua_State*) {
    allowMissionManagerToForceRun = 1;
    return 0;
}

int __cdecl Native_SystemBustedAreaTransitionHandled(lua_State*) {
    bustedAreaTransitionHandled = 1;
    return 0;
}

int __cdecl Native_ReadyToLoad(lua_State*) {
    readyToLoad = 1;
    return 0;
}

int __cdecl Native_SystemIsReady(lua_State* L) {
    Script::PushBool(L, systemReady);
    return 1;
}

int __cdecl Native_SystemMarkCollisionForExclusion(lua_State* L) {
    MarkCollisionForExclusion(Script::GetString(L, 0));
    return 0;
}

int __cdecl Native_SystemEnableFrontEndAndSelectScreens(lua_State* L) {
    frontEndAndSelectScreensEnabled = Script::GetBool(L, 0);
    return 0;
}

// (also ClassMathBothInvalid: pushes false)
int __cdecl Native_IsDemoBuildEnabled(lua_State* L) {
    Script::PushBool(L, 0);
    return 1;
}

int __cdecl Native_DemoBuildGetSelectedMission(lua_State* L) {
    Script::PushInt(L, -1);
    return 1;
}

int __cdecl Native_DemoBuildReturnToMain(lua_State*) {
    demoReturnToMain = 1;
    DemoBuild_ReturnToMain();
    Stat_ResetAll();
    return 0;
}

// (also PlayerGetPedIndex: pushes 0)
int __cdecl Native_GetRequestedSubMPGame(lua_State* L) {
    Script::PushInt(L, 0);
    return 1;
}

int __cdecl Native_HasStoryModeBeenSelected(lua_State* L) {
    Script::PushBool(L, storyModeSelected);
    return 1;
}

int __cdecl Native_CreditReset(lua_State*) {
    creditPosition = 0;
    return 0;
}

int __cdecl Native_CreditLoadDB(lua_State* L) {
    creditPosition = 0;
    Script::PushBool(L, CreditDB_Load(creditDB));
    return 1;
}

int __cdecl Native_CreditUnLoadDB(lua_State*) {
    creditPosition = 0;
    CreditDB_Unload(creditDB);
    return 0;
}

int __cdecl Native_CreditFindText(lua_State* L) {
    Script::PushBool(L, Credit_FindText(Script::GetString(L, 0)));
    return 1;
}

int __cdecl Native_SetClipRange(lua_State* L) {
    int from = Script::GetInt(L, 0);
    Text_SetClipRange(from, Script::GetInt(L, 1));
    return 0;
}

int __cdecl Native_LoadFontStyle(lua_State* L) {
    int style = Script::GetInt(L, 0);
    PushTxdMemoryTag();
    FontStyle_Load(style);
    PopTxdMemoryTag();
    return 0;
}

int __cdecl Native_UnloadFontStyle(lua_State* L) {
    FontStyle_Unload(Script::GetInt(L, 0));
    return 0;
}

int __cdecl Native_GetLanguage(lua_State* L) {
    Script::PushInt(L, language);
    return 1;
}

// ---- tagging, vehicles -----------------------------------------------------------------

inline void* const tagging = reinterpret_cast<void*>(0x020C8A00);
GAME_VAR(uint8_t, taggingOnlyMissionTags, 0x020C8B9C);
GAME_VAR(uint8_t, taggingTVsState, 0x020C8AC0);
GAME_VAR(uint8_t*, ambientTraffic, 0x00BCD080);
GAME_FN(void, __thiscall, Tagging_StartPersistentTag, 0x0072C8F0, void* tagging);
GAME_FN(void, __thiscall, Tagging_StopPersistentTag, 0x0072C950, void* tagging);
GAME_FN(void, __thiscall, AmbientTraffic_RevertToDefault, 0x0041A860, void* traffic);
GAME_FN(void, __cdecl, BikeGenerator_Toggle, 0x004C7200, uint8_t on);

int __cdecl Native_TaggingStartPersistentTag(lua_State*) {
    Tagging_StartPersistentTag(tagging);
    return 0;
}

int __cdecl Native_TaggingStopPersistentTag(lua_State*) {
    Tagging_StopPersistentTag(tagging);
    return 0;
}

int __cdecl Native_TaggingOnlyShowMissionTags(lua_State* L) {
    taggingOnlyMissionTags = Script::GetBool(L, 0);
    return 0;
}

int __cdecl Native_TaggingSetTVsState(lua_State* L) {
    taggingTVsState = Script::GetBool(L, 0);
    return 0;
}

int __cdecl Native_VehicleRevertToDefaultAmbient(lua_State*) {
    AmbientTraffic_RevertToDefault(ambientTraffic);
    return 0;
}

int __cdecl Native_VehicleOverrideAmbientCruiseSpeed(lua_State* L) {
    At<float>(ambientTraffic, 0x1D68) = Script::GetFloat(L, 0);
    return 0;
}

// (returns 1 without pushing anything)
int __cdecl Native_VehicleBikeGeneratorToggle(lua_State* L) {
    BikeGenerator_Toggle(Script::GetBool(L, 0));
    return 1;
}

} // namespace

// Untested: the game's code runs until each is checked.
#define NATC_REVERSED(name, address)       \
    static void* Original_Native_##name;   \
    REVERSED("Native_" #name, address, RE_BODY(Native_##name), nullptr, &Original_Native_##name, re::Mode::Original)

NATC_REVERSED(PlayerResetPhysicalState, 0x005D2060);
NATC_REVERSED(PlayerRefreshPhysicalState, 0x005D2070);
NATC_REVERSED(PlayerSuppressFailDisplayOnDeath, 0x005D2280);
NATC_REVERSED(PlayerGetMoney, 0x005D1BA0);
NATC_REVERSED(PlayerSetMoney, 0x005D1BC0);
NATC_REVERSED(PlayerSetClassSkipCount, 0x005D1C20);
NATC_REVERSED(PlayerSetEnableFirstPerson, 0x005D1C40);
NATC_REVERSED(PlayerGetPunishmentPoints, 0x005D1DA0);
NATC_REVERSED(PlayerSetPunishmentPoints, 0x005D1DC0);
NATC_REVERSED(PlayerScalePunishmentPoints, 0x005D1DF0);
NATC_REVERSED(PlayerIncPunishmentPoints, 0x005D1E20);
NATC_REVERSED(PlayerGetNumTimesBusted, 0x005D1E50);
NATC_REVERSED(PlayerSetMinPunishmentPoints, 0x005D20A0);
NATC_REVERSED(PlayerDoneLoading, 0x005D22D0);
NATC_REVERSED(PlayerIgnoreTargeting, 0x005D1EB0);
NATC_REVERSED(PlayerKilled, 0x005D1FA0);
NATC_REVERSED(PlayerGetPhysicalState, 0x005D2040);
NATC_REVERSED(PlayerIsInStealthProp, 0x005D2080);
NATC_REVERSED(PlayerClearLastVehicle, 0x005D2290);
NATC_REVERSED(PlayerGetHealth, 0x005D1900);
NATC_REVERSED(PlayerActivateCombatSphere, 0x005D19B0);
NATC_REVERSED(PlayerSetHighRisk, 0x005D1BF0);
NATC_REVERSED(PlayerWeaponHudLock, 0x005D1CE0);
NATC_REVERSED(DetentionMeterSetVisibility, 0x005D1E80);
NATC_REVERSED(PlayerSetScriptSavedData, 0x005D1F10);
NATC_REVERSED(PlayerGetScriptSavedData, 0x005D1F40);
NATC_REVERSED(PlayerSetGrade, 0x005D1F70);
NATC_REVERSED(AwardAchievement, 0x005D22B0);
NATC_REVERSED(POISetSystemEnabled, 0x005D2EE0);
NATC_REVERSED(POIGroupsEnabled, 0x005D2F00);
NATC_REVERSED(PickupMakeTargetable, 0x005D0F60);
NATC_REVERSED(PickupSetIgnoreRespawnDistance, 0x005D10F0);
NATC_REVERSED(PickupRemoveAll, 0x005D13B0);
NATC_REVERSED(ShopEnd, 0x005D3440);
NATC_REVERSED(ShopIsActive, 0x005D3450);
NATC_REVERSED(ShopSetIsPlayerInShop, 0x005D34D0);
NATC_REVERSED(ShopSetCameraZoomPercentage, 0x005D34F0);
NATC_REVERSED(ShopSetCameraAngleOffset, 0x005D3510);
NATC_REVERSED(ShopSetOnStopShoppingCB, 0x005D3470);
NATC_REVERSED(ShopGetClerkID, 0x005D34A0);
NATC_REVERSED(SoundClearSpeechFilter, 0x005D36C0);
NATC_REVERSED(SoundAddSpeechPriorityFilter, 0x005D36D0);
NATC_REVERSED(SoundClearScriptedAmbience, 0x005D3740);
NATC_REVERSED(SoundDisableSpeech, 0x005D37B0);
NATC_REVERSED(SoundEnableSpeech, 0x005D37C0);
NATC_REVERSED(SoundDisableSpeech_ActionTree, 0x005D37D0);
NATC_REVERSED(SoundEnableSpeech_ActionTree, 0x005D37F0);
NATC_REVERSED(SoundInitScreamer, 0x005D3810);
NATC_REVERSED(SoundPause, 0x005D3820);
NATC_REVERSED(SoundContinue, 0x005D3830);
NATC_REVERSED(SoundPlayPreloadedSpeech, 0x005D3B30);
NATC_REVERSED(SoundIsSpeechPreloaded, 0x005D3B40);
NATC_REVERSED(SoundStartFireAlarm, 0x005D3D30);
NATC_REVERSED(SoundStopFireAlarm, 0x005D3D40);
NATC_REVERSED(SoundPlayPreloadedStream, 0x005D3E10);
NATC_REVERSED(SoundStopStream, 0x005D45D0);
NATC_REVERSED(SoundStopInteractiveStream, 0x005D45E0);
NATC_REVERSED(SoundFadeoutStream, 0x005D4600);
NATC_REVERSED(SoundFadeoutAmbience, 0x005D4630);
NATC_REVERSED(SoundStopPA, 0x005D46A0);
NATC_REVERSED(SoundRestartPA, 0x005D46B0);
NATC_REVERSED(SoundStopAmbiences, 0x005D4660);
NATC_REVERSED(SoundRestartAmbiences, 0x005D4680);
NATC_REVERSED(SoundSetAudioFocusPlayer, 0x005D46C0);
NATC_REVERSED(SoundSetAudioFocusCamera, 0x005D46E0);
NATC_REVERSED(SoundBoxingCrowdSoundStart, 0x005D47A0);
NATC_REVERSED(SoundBoxingCrowdSoundKill, 0x005D47B0);
NATC_REVERSED(SoundBoxingCrowdSoundPlayBoo, 0x005D47E0);
NATC_REVERSED(SoundBoxingCrowdSoundPlayCheer, 0x005D47F0);
NATC_REVERSED(SoundBoxingCrowdSoundSetExcitementLevel, 0x005D47C0);
NATC_REVERSED(SoundSetCurrentGoKartRace, 0x005D4800);
NATC_REVERSED(SoundMakeEverythingCloser, 0x005D4AE0);
NATC_REVERSED(MusicAllowPlayDuringCutscenes, 0x005D4B00);
NATC_REVERSED(MusicFadeWithCamera, 0x005D4740);
NATC_REVERSED(CutsceneFadeWithCamera, 0x005D4770);
NATC_REVERSED(SoundEnableInteractiveMusic, 0x005D4820);
NATC_REVERSED(SoundLoadBank, 0x005D3750);
NATC_REVERSED(SoundUnLoadBank, 0x005D3780);
NATC_REVERSED(SoundLoopStop3D, 0x005D3FA0);
NATC_REVERSED(StatUpdatePrincipalStats, 0x005D5120);
NATC_REVERSED(StatResetAll, 0x005D5170);
NATC_REVERSED(StatLayoutAddGroup, 0x005D5040);
NATC_REVERSED(StatLayoutAddHeading, 0x005D5060);
NATC_REVERSED(StatLayoutAddStat, 0x005D5080);
NATC_REVERSED(StatGetNumLayoutItems, 0x005D5130);
NATC_REVERSED(StatSetIsTrackingEnabled, 0x005D5150);
NATC_REVERSED(StatGetGameCompletion, 0x005D5180);
NATC_REVERSED(StatSetAsInt, 0x005D4ED0);
NATC_REVERSED(StatGetAsInt, 0x005D4FD0);
NATC_REVERSED(StatGetPrincipalDiffAsInt, 0x005D50B0);
NATC_REVERSED(SystemResetShouldMovePlayer, 0x005D51D0);
NATC_REVERSED(SystemShouldMovePlayer, 0x005D51A0);
NATC_REVERSED(SystemAllowMissionManagerToForceRunMissions, 0x005D5270);
NATC_REVERSED(SystemBustedAreaTransitionHandled, 0x005D5330);
NATC_REVERSED(ReadyToLoad, 0x005D5400);
NATC_REVERSED(SystemIsReady, 0x005D5410);
NATC_REVERSED(SystemMarkCollisionForExclusion, 0x005D52C0);
NATC_REVERSED(SystemEnableFrontEndAndSelectScreens, 0x005D5340);
NATC_REVERSED(IsDemoBuildEnabled, 0x005D5360);
NATC_REVERSED(DemoBuildGetSelectedMission, 0x005D5380);
NATC_REVERSED(DemoBuildReturnToMain, 0x005D53A0);
NATC_REVERSED(GetRequestedSubMPGame, 0x005D53C0);
NATC_REVERSED(HasStoryModeBeenSelected, 0x005D53E0);
NATC_REVERSED(CreditReset, 0x005D5B30);
NATC_REVERSED(CreditLoadDB, 0x005D5B40);
NATC_REVERSED(CreditUnLoadDB, 0x005D5B70);
NATC_REVERSED(CreditFindText, 0x005D5AC0);
NATC_REVERSED(SetClipRange, 0x005D5B90);
NATC_REVERSED(LoadFontStyle, 0x005D5BC0);
NATC_REVERSED(UnloadFontStyle, 0x005D5BF0);
NATC_REVERSED(GetLanguage, 0x005D5C10);
NATC_REVERSED(TaggingStartPersistentTag, 0x005D54F0);
NATC_REVERSED(TaggingStopPersistentTag, 0x005D5500);
NATC_REVERSED(TaggingOnlyShowMissionTags, 0x005D54B0);
NATC_REVERSED(TaggingSetTVsState, 0x005D54D0);
NATC_REVERSED(VehicleRevertToDefaultAmbient, 0x005D5FD0);
NATC_REVERSED(VehicleOverrideAmbientCruiseSpeed, 0x005D5E40);
NATC_REVERSED(VehicleBikeGeneratorToggle, 0x005D60C0);
NATC_REVERSED(PlayerIsInAnyVehicle, 0x005D1810);
NATC_REVERSED(PlayerDismountBike, 0x005D18C0);
NATC_REVERSED(PlayerSetHealth, 0x005D1930);
NATC_REVERSED(PlayerSetInvulnerable, 0x005D1970);
NATC_REVERSED(PlayerSetTakeDamageWhenAttachedToVehicle, 0x005D19E0);
NATC_REVERSED(PlayerGetBikeId, 0x005D1C60);
NATC_REVERSED(PlayerGetLastBikeId, 0x005D1CA0);
NATC_REVERSED(PlayerHasWeapon, 0x005D1ED0);
NATC_REVERSED(PlayerFellAsleep, 0x005D1FC0);
NATC_REVERSED(PlayerChangePhysicalState, 0x005D2000);
NATC_REVERSED(PlayerUnlockUpgrade, 0x005D20D0);
NATC_REVERSED(PlayerClearRewardStore, 0x005D2240);
NATC_REVERSED(POISetScriptedPedExitObjective, 0x005D2EA0);
NATC_REVERSED(POISetDisablePedProduction, 0x005D2F30);
NATC_REVERSED(ShopStart, 0x005D3400);
NATC_REVERSED(SoundEmitterEnable, 0x005D3640);
NATC_REVERSED(SoundMusicJimmyComeToTheOfficePA, 0x005D3680);
NATC_REVERSED(SoundIsPreloadReady, 0x005D3E20);
NATC_REVERSED(SoundFadeWithCamera, 0x005D4700);
NATC_REVERSED(StatSetAsFloat, 0x005D4F00);
NATC_REVERSED(StatAddToInt, 0x005D4F40);
NATC_REVERSED(StatGetAsFloat, 0x005D5000);
NATC_REVERSED(StatGetPrincipalDiffAsFloat, 0x005D50E0);
NATC_REVERSED(SystemShouldEndScript, 0x005D5280);
NATC_REVERSED(TextClear, 0x005D5A20);
NATC_REVERSED(CreditGetInteger, 0x005D5AF0);
