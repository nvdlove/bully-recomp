// The action tree's mission conditions (CMissionCond, CErrandCond, ConditionLastMission,
// CMissionUnlockedCond, CClassSkippedCond, CTimeCond, CPeriodCond, CDayCond, CLuaCond)
// and mission tracks (the char / vehicle / button / door triggers, the door lock, the
// tutorial's message / complete / pause / start), with their field initialisers (called
// after construction by the factories) and destructors. 0x6A71D0..0x6A8A8E.
//
// A condition: +0 vtable, its fields from +4; vf1 Evaluate(owner) -> byte.
// A track (ActionTracksCommon.h): fields from +0x18; vf2 Start(owner), vf3 Update(dt),
// vf4 End(), vf5 its name. Their "owner" here: +0x20 the mission slot (+8 its mission).

#include <cmath>
#include <cstring>

#include "game/ActionTracksCommon.h"

#include "game/Crt.h"
namespace mcond {

using vc::At;

namespace {

void* const kMissionMgr = reinterpret_cast<void*>(0x020C3CA0);
void* const kPlayerInfo = reinterpret_cast<void*>(0x00C1AEA8);
void* const kPeriods = reinterpret_cast<void*>(0x020C35E8);
void* const kRoutes = reinterpret_cast<void*>(0x00BC5E7C);
void* const kLurStore = reinterpret_cast<void*>(0x00D02850);
void* const kCharTriggers = reinterpret_cast<void*>(0x00BCD098);
void* const kTriggers = reinterpret_cast<void*>(0x020C7E98);
void* const kDoors = reinterpret_cast<void*>(0x00C3B808);
void* const kAreaTransition = reinterpret_cast<void*>(0x00BCC580);
void* const kUserInput = reinterpret_cast<void*>(0x00A45A30);
void* const kTheText = reinterpret_cast<void*>(0x020C2FA4);
GAME_VAR(uint8_t*, g_playerPed, 0x00C1AEA8);
GAME_VAR(void*, g_radar, 0x00C674CC);
GAME_VAR(void*, g_tutorialHud, 0x00C67540);
GAME_VAR(void*, g_hudText, 0x00C67548);
GAME_VAR(uint8_t*, g_classes, 0x00C0F78C);       // (+0xC classes attended)
GAME_VAR(uint8_t*, g_triggerPool, 0x020C7C48);   // CPool: +0 objects, +4 flags, +0xC size
GAME_VAR(uint8_t*, g_areaCounter, 0x009E4F4C);   // (+4 changes with the area)
GAME_VAR(uint8_t*, g_helpColour, 0x020C88A4);    // (+0x18.. r, g, b, a as ints)
GAME_VAR(uint8_t, g_tutorialComplete, 0x020C43FC);
GAME_VAR(uint8_t, g_tutorialPaused, 0x020C43FD);
GAME_VAR(uint8_t, g_buttonTriggered, 0x020C43EC);
GAME_ARRAY(uint8_t, g_fontColour, 0x0208B764);
GAME_ARRAY(float, g_zeroVector, 0x00BCC564);
// the lazily made keys of CMissionCond
GAME_VAR(uint32_t, g_missionCondKeysGuard, 0x020C3598);
GAME_VAR(uint32_t, g_keyCurrentMission, 0x020C3594);
GAME_VAR(uint32_t, g_keyCurrentClassMission, 0x020C3590);
GAME_FN(void, __thiscall, MC_NameKey_Destruct, 0x0049A650, uint32_t* key);

GAME_FN(uint8_t*, __thiscall, MC_TrackOrConditionConstruct, 0x0061A600, uint8_t* self);
GAME_FN(uint32_t*, __thiscall, MC_NameKeyDefault, 0x0066BEA0, uint32_t* key);
GAME_FN(uint32_t*, __thiscall, MC_NameKey, 0x005FA910, uint32_t* key, const char* name);
inline int __cdecl MC_Atexit(void* fn) { return crt::Atexit((void(__cdecl*)())fn); }
GAME_FN(void, __cdecl, MC_Free, 0x005EEAB0, void* p);
GAME_FN(int32_t, __thiscall, MC_MissionFind, 0x006A9FD0, void* mgr, uint32_t* key);
GAME_FN(uint8_t*, __thiscall, MC_MissionFlags, 0x006AA7C0, void* mgr, int32_t mission);
GAME_FN(uint8_t*, __thiscall, MC_MissionInfo, 0x006AA660, void* mgr, int32_t mission);
GAME_FN(uint8_t*, __thiscall, MC_MissionSlot0, 0x006AA680, void* mgr);
GAME_FN(uint8_t*, __thiscall, MC_MissionSlot1, 0x006AA690, void* mgr);
GAME_FN(int32_t*, __thiscall, MC_MissionCurrent, 0x006AA790, void* mgr);
GAME_FN(int32_t, __thiscall, MC_MissionRunningClass, 0x006AA030, void* mgr);
GAME_FN(uint8_t, __thiscall, MC_MissionAvailable, 0x006AA9A0, void* mgr, int32_t mission, int flag);
GAME_FN(void, __thiscall, MC_MissionStart, 0x006AD100, void* mgr, int32_t mission, int);
GAME_FN(void, __thiscall, MC_MissionUpdateClasses, 0x006AD290, void* mgr, int);
GAME_FN(int32_t, __thiscall, MC_ErrandCompletion, 0x004467E0, void* info, int errand);
GAME_FN(uint8_t, __cdecl, MC_IsTimeInRange, 0x00425E40, int fromHours, int toHours, int fromMinutes, int toMinutes);
GAME_FN(int32_t, __thiscall, MC_PeriodFind, 0x006A8C30, void* periods, const char* name);
GAME_FN(uint8_t, __thiscall, MC_PeriodNow, 0x006A8BB0, void* periods, int period, int before, int after);
GAME_FN(uint8_t, __thiscall, MC_PeriodIsDay, 0x006A8C90, void* periods, int day, int weekly);
GAME_FN(uint8_t*, __thiscall, MC_LurFindScript, 0x005DBCC0, void* store, const char* name);
GAME_FN(uint8_t*, __thiscall, MC_LurGetScript, 0x005DB8D0, void* store, int index);
GAME_FN(int32_t, __thiscall, MC_ScriptCall, 0x005D8200, uint8_t* script, const char* name, int args, int results);
GAME_FN(uint8_t*, __cdecl, MC_GetModelInfo, 0x0051C190, const char* name, int32_t* id);
GAME_FN(int32_t, __thiscall, MC_RoutesFind, 0x006D63C0, void* routes, uintptr_t name);
GAME_FN(uint8_t*, __thiscall, MC_RoutesGet, 0x006D6450, void* routes, int index);
GAME_FN(uint8_t*, __thiscall, MC_RoutesGetById, 0x006D64D0, void* routes, uint32_t id);
GAME_FN(float*, __thiscall, MC_RouteFirst, 0x006D59B0, uint8_t* route);
GAME_FN(void*, __cdecl, MC_VehicleTriggers, 0x0045C950);
GAME_FN(void, __thiscall, MC_VehicleTriggersAdd, 0x0045C2F0, void* triggers, int32_t mission, int32_t area,
        const float* pos, float heading, int32_t model, int32_t a, int b24, int b25, int32_t c, int b2c, uintptr_t d);
GAME_FN(void, __thiscall, MC_VehicleTriggersRemove, 0x0045C7B0, void* triggers, int32_t area, const float* pos);
GAME_FN(void, __thiscall, MC_CharTriggersAdd, 0x004246E0, void* triggers, int32_t model, int area, const float* pos,
        float heading, float radius, uintptr_t a24, uintptr_t a20, int32_t a30, int32_t a34, int32_t a38, int b3c,
        int b3d, int32_t a48, uintptr_t a70, uintptr_t a74, int b2c, int32_t mission, int b2d, int32_t a4c, uint8_t* node);
GAME_FN(void, __thiscall, MC_CharTriggersRemove, 0x00424530, void* triggers, int area, const float* pos);
GAME_FN(uint8_t*, __cdecl, MC_ActTreeFind, 0x005F5B10, const char* path, uint8_t* from);
GAME_FN(void, __thiscall, MC_RadarRemoveBlip, 0x0054A940, void* radar, int blip);
GAME_FN(int32_t, __thiscall, MC_RadarAddBlip, 0x0054A410, void* radar, int sprite, float x, float y, float z,
        uint32_t colour, int a, int32_t type, int flag);
GAME_FN(void, __thiscall, MC_RadarSetBlipPos, 0x0054A9C0, void* radar, int32_t blip, const float* pos);
GAME_FN(void, __thiscall, MC_RadarSetBlipSize, 0x00548BF0, void* radar, int32_t blip, int size);
GAME_FN(void, __thiscall, MC_RadarSetBlipMission, 0x00548A40, void* radar, int32_t blip, int32_t mission);
GAME_FN(void, __cdecl, MC_PlaceMarker, 0x0052A600, int type, int a, int b, const float* pos, float size,
        const float* rotation, int32_t mission, int c, int g, int bl, void* d, int e, int f);
GAME_FN(float*, __cdecl, MC_VectorSub, 0x00414B70, float* out, const float* a, const float* b);
GAME_FN(void, __cdecl, MC_FontReset, 0x00688E60);
GAME_FN(void, __cdecl, MC_ShowHelp, 0x00514340, uintptr_t key, float seconds);
GAME_FN(uint8_t, __thiscall, MC_PedInVehicle, 0x004CE690, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, MC_Ped473FD0, 0x00473FD0, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, MC_PedHasMoney, 0x004730E0, uint8_t* ped, int32_t amount);
GAME_FN(uint8_t*, __thiscall, MC_AreaTransitionTarget, 0x004152F0, void* transition);
GAME_FN(void, __thiscall, MC_AreaTransitionSetTarget, 0x004152B0, void* transition, void* target);
GAME_FN(int32_t, __thiscall, MC_TriggersFind, 0x006D79C0, void* triggers, uint32_t name);
GAME_FN(uint8_t*, __thiscall, MC_TriggersFromName, 0x00436C10, void* triggers, uintptr_t name);
GAME_FN(void, __thiscall, MC_TriggerSetLocked, 0x006D6570, uint8_t* trigger, int locked);
GAME_FN(void, __thiscall, MC_DoorsSetLocked, 0x004E99F0, void* doors, uint32_t name, int locked);
GAME_FN(uint8_t, __cdecl, MC_IsOnScreen, 0x00528750, const float* pos);
GAME_FN(uint8_t, __cdecl, MC_IsEntityInArea, 0x005C2A70, int is3D, int handle, uint32_t area0, float x, float y,
        float z, float rx, float ry, float rz, int32_t a20, const float* rotation, int32_t blip, int32_t mission,
        int flag, uint8_t* info, int);
GAME_FN(uint8_t, __thiscall, MC_InputEnabled, 0x0050DF50, void* input);
GAME_FN(uint8_t*, __cdecl, MC_FindVehicle, 0x004D33F0, uint8_t* ped, float range, int, int, int);
GAME_FN(void, __thiscall, MC_MissionDataCopy, 0x0044F390, void* self, uint8_t* from);
GAME_FN(void, __thiscall, MC_MissionDataDestruct, 0x0044E0D0, void* self);
GAME_FN(void, __thiscall, MC_HudTextAdd, 0x0055B070, void* hud, int slot, uintptr_t text);
GAME_FN(void, __cdecl, MC_ShowPrompt, 0x0068F580, uintptr_t key, float scale);
GAME_FN(float, __cdecl, MC_PadValue, 0x00738840, int control, int pad);
GAME_FN(uint8_t, __cdecl, MC_PadJustDown, 0x00738900, int button, int pad);
GAME_FN(void, __thiscall, MC_ControllerPlay, 0x005F4A50, uint8_t* controller, uint8_t* node);
GAME_FN(const uint16_t*, __thiscall, MC_TextGet, 0x0068FE80, void* text, uintptr_t key);
GAME_FN(int, __cdecl, MC_ActivePad, 0x00738070);
GAME_FN(uint8_t, __cdecl, MC_PadIsXInput, 0x00738500, int pad);
GAME_FN(uint8_t, __cdecl, MC_PadIsUsed, 0x00738520, int pad, int);
GAME_FN(void, __thiscall, MC_TutorialSetText, 0x0055B6E0, void* hud, uintptr_t key);
GAME_FN(void, __cdecl, MC_PadStopRumble, 0x00738780);
GAME_FN(void, __cdecl, MC_StartUserPause, 0x0045B810);
GAME_FN(void, __cdecl, MC_EndUserPause, 0x0045B820);
GAME_FN(void, __cdecl, MC_GenerateBoundingBoxes, 0x0045B960, int);

// the mission of a track's owner (+0x20 its slot, +8 in it)
int32_t OwnerMission(uint8_t* owner) {
    return At<int32_t>(At<uint8_t*>(owner, 0x20), 8);
}

// a string compared with the game's `repe cmpsb` (stops at the first difference)
bool SameString(const char* a, const char* b) {
    for (;; ++a, ++b) {
        if (*a != *b) {
            return false;
        }
        if (*b == 0) {
            return true;
        }
    }
}

// a position: the matrix's when there is one
const float* EntityPos(uint8_t* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix != nullptr ? reinterpret_cast<float*>(matrix + 0x30) : reinterpret_cast<float*>(entity + 4);
}

// a trigger by name (the pool's slot, none when free)
uint8_t* TriggerByName(uint32_t name) {
    int32_t index = MC_TriggersFind(kTriggers, name);
    if (index == -1) {
        return nullptr;
    }
    uint8_t* pool = g_triggerPool;
    if ((At<uint8_t*>(pool, 4)[index] & 0x80) != 0) {
        return nullptr;
    }
    return At<uint8_t*>(pool, 0) + At<int32_t>(pool, 0xC) * index;
}

// a trigger's position: its entity's when it has one
void TriggerPos(uint8_t* trigger, float* out) {
    out[0] = At<float>(trigger, 4);
    out[1] = At<float>(trigger, 8);
    out[2] = At<float>(trigger, 0xC);
    uint8_t* entity = At<uint8_t*>(trigger, 0x1C);
    if (entity != nullptr) {
        const float* pos = EntityPos(entity);
        out[0] = pos[0];
        out[1] = pos[1];
        out[2] = pos[2];
    }
}

float DegreesToRadians(float degrees) {
    float product = float(double(degrees) * 3.141592653589793);
    return float(double(product) / 180.0);
}

} // namespace

// ---- the conditions ----------------------------------------------------------------------------

// the conditions' deleting destructors (the vtable set, a name key at +4 destroyed first
// when `Key`)
template <uint32_t Vt, bool Key>
uint8_t* __fastcall CondDeletingDtor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = Vt;
    if constexpr (Key) {
        actc::NameKey_Destruct(self + 4);
    }
    if (flags & 1) {
        MC_Free(self);
    }
    return self;
}

// 0x6A71D0: CMissionData's fields (after construction)
void __fastcall MissionData_Init(uint8_t* self) {
    At<uint8_t>(self, 8) = 1;
    actc::ClearName(self, 0x1C);
    actc::ClearName(self, 0x18);
    actc::ClearName(self, 0x20);
    At<uint8_t>(self, 0x24) = 1;
    At<uint8_t>(self, 0x25) = 0;
    At<int32_t>(self, 0x28) = 0;
    for (int i = 0x2C; i <= 0x35; ++i) {
        At<uint8_t>(self, i) = 0;
    }
    At<uint8_t>(self, 0x36) = 1;
    At<int32_t>(self, 0x38) = 0;
    At<uint8_t>(self, 0x2F) = 0;
    At<uint8_t>(self, 0x26) = 0;
    At<uint8_t>(self, 0x3C) = 0;
    At<int32_t>(self, 0x40) = 0;
    actc::ClearName(self, 0x44);
    At<uint8_t>(self, 0x48) = 0;
    At<uint8_t>(self, 0x49) = 0;
    At<uint8_t>(self, 0x4A) = 0;
}

// 0x6A72F0: CErrandCond constructed
uint8_t* __fastcall ErrandCond_Construct(uint8_t* self) {
    MC_TrackOrConditionConstruct(self);
    At<uint32_t>(self, 0) = 0x00941750;
    return self;
}

// 0x6A7310 (vf1): the errand (+4) completed
uint8_t __fastcall ErrandCond_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    return MC_ErrandCompletion(kPlayerInfo, At<int32_t>(self, 4)) > 0;
}

// 0x6A7330: ConditionLastMission constructed
uint8_t* __fastcall LastMission_Construct(uint8_t* self) {
    MC_TrackOrConditionConstruct(self);
    At<uint32_t>(self, 0) = 0x00941764;
    MC_NameKeyDefault(&At<uint32_t>(self, 4));
    At<int32_t>(self, 0xC) = -2;
    return self;
}

// 0x6A7360 (vf1): the mission (+4, looked up once) is the last one of slot 0 or 1 (+8)
uint8_t __fastcall LastMission_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    if (At<int32_t>(self, 0xC) == -2 && At<uint32_t>(self, 4) != 0) {
        At<int32_t>(self, 0xC) = MC_MissionFind(kMissionMgr, &At<uint32_t>(self, 4));
    }
    if (At<int32_t>(self, 0xC) < 0) {
        return 0;
    }
    uint8_t* slot = At<uint8_t>(self, 8) != 0 ? MC_MissionSlot0(kMissionMgr) : MC_MissionSlot1(kMissionMgr);
    return At<int32_t>(slot, 0x84) == At<int32_t>(self, 0xC);
}

// 0x6A73D0: CMissionUnlockedCond constructed
uint8_t* __fastcall Unlocked_Construct(uint8_t* self) {
    MC_TrackOrConditionConstruct(self);
    At<uint32_t>(self, 0) = 0x00941778;
    MC_NameKeyDefault(&At<uint32_t>(self, 4));
    At<int32_t>(self, 8) = -2;
    return self;
}

// 0x6A7400 (vf1): the mission done or unlocked (flag 8)
uint8_t __fastcall Unlocked_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    if (At<int32_t>(self, 8) == -2 && At<uint32_t>(self, 4) != 0) {
        At<int32_t>(self, 8) = MC_MissionFind(kMissionMgr, &At<uint32_t>(self, 4));
    }
    if (At<uint16_t>(MC_MissionFlags(kMissionMgr, At<int32_t>(self, 8)), 2) > 0) {
        return 1;
    }
    return (At<uint8_t>(MC_MissionFlags(kMissionMgr, At<int32_t>(self, 8)), 4) & 8) != 0 ? 1 : 0;
}

// 0x6A7460: CClassSkippedCond's fields
void __fastcall ClassSkipped_Init(uint8_t* self) {
    At<int32_t>(self, 4) = 1;
    At<uint8_t>(self, 8) = 0;
}

// 0x6A7470: its destructor (the vtable set)
void __fastcall ClassSkipped_Destruct(uint8_t* self) {
    At<uint32_t>(self, 0) = 0x00936BA0;
}

// 0x6A7480 (vf1): classes attended below (+8 set) / at least (+8 clear) the count (+4)
uint8_t __fastcall ClassSkipped_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    int32_t attended = At<int32_t>(g_classes, 0xC);
    if (At<uint8_t>(self, 8) != 0) {
        return attended < At<int32_t>(self, 4);
    }
    return attended >= At<int32_t>(self, 4);
}

// 0x6A74B0: CTimeCond's fields
void __fastcall TimeCond_Init(uint8_t* self) {
    At<int32_t>(self, 4) = 0;
    At<int32_t>(self, 8) = 0;
    At<int32_t>(self, 0xC) = 0;
    At<int32_t>(self, 0x10) = 0;
}

// 0x6A74C0: its destructor
void __fastcall TimeCond_Destruct(uint8_t* self) {
    At<uint32_t>(self, 0) = 0x009372D4;
}

// 0x6A74D0 (vf4) / 0x6A7620: the conditions' names
template <uintptr_t Name>
const char* __fastcall NameOf(void*) {
    return reinterpret_cast<const char*>(Name);
}

// 0x6A74E0 (vf1): the clock in the range
uint8_t __fastcall TimeCond_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    return MC_IsTimeInRange(At<int32_t>(self, 4), At<int32_t>(self, 8), At<int32_t>(self, 0xC),
                            At<int32_t>(self, 0x10));
}

// 0x6A7500: CPeriodCond constructed
uint8_t* __fastcall PeriodCond_Construct(uint8_t* self) {
    MC_TrackOrConditionConstruct(self);
    At<uint32_t>(self, 0) = 0x0094178C;
    MC_NameKeyDefault(&At<uint32_t>(self, 4));
    At<int32_t>(self, 0x10) = -1;
    return self;
}

// 0x6A7530: CPeriodCond's fields
void __fastcall PeriodCond_Init(uint8_t* self) {
    actc::ClearName(self, 4);
    At<int32_t>(self, 8) = 0;
    At<int32_t>(self, 0xC) = -1;
}

// 0x6A7570 (vf1): now in the named period (looked up once), stretched by +8 / +0xC
uint8_t __fastcall PeriodCond_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    const char* name = At<const char*>(self, 4);
    if (name == nullptr) {
        return 0;
    }
    if (At<int32_t>(self, 0x10) == -1) {
        At<int32_t>(self, 0x10) = MC_PeriodFind(kPeriods, name);
    }
    if (At<int32_t>(self, 0x10) == -1) {
        return 0;
    }
    return MC_PeriodNow(kPeriods, At<int32_t>(self, 0x10), At<int32_t>(self, 8), At<int32_t>(self, 0xC));
}

// 0x6A75C0: CDayCond's fields
void __fastcall DayCond_Init(uint8_t* self) {
    At<int32_t>(self, 4) = 0;
    At<uint8_t>(self, 8) = 1;
}

// 0x6A75D0: its destructor
void __fastcall DayCond_Destruct(uint8_t* self) {
    At<uint32_t>(self, 0) = 0x009372EC;
}

// 0x6A75E0 (vf1): the day (weekly when +8)
uint8_t __fastcall DayCond_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    return MC_PeriodIsDay(kPeriods, At<int32_t>(self, 4), At<uint8_t>(self, 8));
}

// 0x6A7600: CLuaCond's destructor (its two name keys)
void __fastcall LuaCond_Destruct(uint8_t* self) {
    At<uint32_t>(self, 0) = 0x00937300;
    actc::NameKey_Destruct(self + 0x10);
    actc::NameKey_Destruct(self + 0xC);
}

// 0x6A7630 (vf1): a script's function (+0x10) returns more than 0. The script (+0xC,
// "mission": the running mission's) found or made; a function without a script name
// compares a null string (as the game does).
uint8_t __fastcall LuaCond_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    const char* script = At<const char*>(self, 0xC);
    const char* function = At<const char*>(self, 0x10);
    if (function == nullptr && script == nullptr) {
        return 0;
    }
    if (SameString(script, "mission")) {
        int32_t current = *MC_MissionCurrent(kMissionMgr);
        if (current == -1) {
            return 0;
        }
        script = At<const char*>(MC_MissionInfo(kMissionMgr, current), 0x1C);
    }
    uint8_t* found = MC_LurFindScript(kLurStore, script);
    At<uint8_t*>(self, 4) = found;
    if (found == nullptr) {
        At<uint8_t*>(self, 4) = MC_LurGetScript(kLurStore, 0);
    }
    if (function == nullptr || At<uint8_t*>(self, 4) == nullptr) {
        return 0;
    }
    return MC_ScriptCall(At<uint8_t*>(self, 4), function, 0, 0) > 0;
}

// 0x6A7EB0: CMissionCond constructed
uint8_t* __fastcall MissionCond_Construct(uint8_t* self) {
    MC_TrackOrConditionConstruct(self);
    At<uint32_t>(self, 0) = 0x0094173C;
    MC_NameKeyDefault(&At<uint32_t>(self, 4));
    At<int32_t>(self, 8) = -2;
    return self;
}

// 0x6A7F10 (vf1): the mission (+4: a name, "CurrentMission" or "CurrentClassMission") done
uint8_t __fastcall MissionCond_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    if (!(g_missionCondKeysGuard & 1)) {
        g_missionCondKeysGuard |= 1;
        MC_NameKey(&g_keyCurrentMission, "CurrentMission");
        MC_Atexit(reinterpret_cast<void*>(0x008F7820));
    }
    if (!(g_missionCondKeysGuard & 2)) {
        g_missionCondKeysGuard |= 2;
        MC_NameKey(&g_keyCurrentClassMission, "CurrentClassMission");
        MC_Atexit(reinterpret_cast<void*>(0x008F7810));
    }
    uint32_t name = At<uint32_t>(self, 4);
    if (name == g_keyCurrentClassMission) {
        At<int32_t>(self, 8) = MC_MissionRunningClass(kMissionMgr);
    } else if (name == g_keyCurrentMission) {
        At<int32_t>(self, 8) = *MC_MissionCurrent(kMissionMgr);
    } else if (At<int32_t>(self, 8) == -2 && name != 0) {
        At<int32_t>(self, 8) = MC_MissionFind(kMissionMgr, &At<uint32_t>(self, 4));
    }
    int32_t mission = At<int32_t>(self, 8);
    if (mission < 0) {
        return 0;
    }
    return At<uint16_t>(MC_MissionFlags(kMissionMgr, mission), 2) > 0 ? 1 : 0;
}

// ---- the tracks --------------------------------------------------------------------------------

// 0x6A76D0: CMissionCharTriggerTrack's fields
void __fastcall CharTrigger_Init(uint8_t* self) {
    At<uint8_t>(self, 8) = 1;
    actc::ClearName(self, 0x1C);
    actc::ClearName(self, 0x18);
    actc::ClearName(self, 0x20);
    actc::ClearName(self, 0x24);
    At<float>(self, 0x28) = 5.0f;
    At<uint8_t>(self, 0x2C) = 1;
    At<uint8_t>(self, 0x2D) = 1;
    At<int32_t>(self, 0x30) = 0;
    At<int32_t>(self, 0x34) = 5;
    At<int32_t>(self, 0x38) = 0;
    At<uint8_t>(self, 0x3C) = 0;
    At<uint8_t>(self, 0x3D) = 0;
    At<int32_t>(self, 0x48) = 0;
    actc::ClearName(self, 0x40);
    actc::ClearName(self, 0x44);
    At<int32_t>(self, 0x4C) = -1;
    actc::ClearName(self, 0x50);
}

// 0x6A7850: CMissionVehicleTriggerTrack's fields
void __fastcall VehicleTrigger_Init(uint8_t* self) {
    At<uint8_t>(self, 8) = 1;
    actc::StringRef_Assign(&At<uint32_t>(self, 0x18), 0);
    actc::StringRef_Assign(&At<uint32_t>(self, 0x1C), 0);
    At<int32_t>(self, 0x20) = 0;
    At<uint8_t>(self, 0x24) = 1;
    At<uint8_t>(self, 0x25) = 1;
    At<int32_t>(self, 0x28) = 3;
    At<uint8_t>(self, 0x2C) = 1;
    actc::StringRef_Assign(&At<uint32_t>(self, 0x30), 0);
}

// 0x6A78A0 (vf2): Start: a vehicle trigger at the route's first point (heading - 90)
uint8_t __fastcall VehicleTrigger_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    int32_t model;  // (left as the lookup leaves it)
    MC_GetModelInfo(At<const char*>(self, 0x1C), &model);
    uint8_t* route = MC_RoutesGet(kRoutes, MC_RoutesFind(kRoutes, At<uint32_t>(self, 0x18)));
    float* first = MC_RouteFirst(route);
    At<int32_t>(self, 0x34) = At<int8_t>(route, 0xC);
    At<float>(self, 0x38) = first[0];
    At<float>(self, 0x3C) = first[1];
    At<float>(self, 0x40) = first[2];
    float heading = DegreesToRadians(first[7] - 90.0f);
    MC_VehicleTriggersAdd(MC_VehicleTriggers(), OwnerMission(owner), At<int32_t>(self, 0x34), &At<float>(self, 0x38), heading, model,
                          At<int32_t>(self, 0x20), At<uint8_t>(self, 0x24), At<uint8_t>(self, 0x25),
                          At<int32_t>(self, 0x28), At<uint8_t>(self, 0x2C), At<int32_t>(self, 0x30));
    return 1;
}

// 0x6A7980 (vf4): End: the trigger removed
uint8_t __fastcall VehicleTrigger_End(uint8_t* self) {
    int32_t area = At<int32_t>(self, 0x34);
    MC_VehicleTriggersRemove(MC_VehicleTriggers(), area, &At<float>(self, 0x38));
    return 1;
}

// 0x6A79A0: CMissionButtonTriggerTrack's fields
void __fastcall ButtonTrigger_Init(uint8_t* self) {
    At<uint8_t>(self, 8) = 1;
    actc::ClearName(self, 0x18);
    actc::ClearName(self, 0x30);
    At<float>(self, 0x1C) = 1.0f;
    At<uint8_t>(self, 0x2C) = 0;
    At<float>(self, 0x34) = 1.0f;
    At<int32_t>(self, 0x20) = 1;
    At<uint8_t>(self, 0x24) = 0;
    At<int32_t>(self, 0x28) = 0;
    At<uint8_t>(self, 0x2D) = 1;
    At<int32_t>(self, 0x38) = 0x5F;
    At<int32_t>(self, 0x3C) = 0x51;
}

// 0x6A7A50 (vf2): Start: the route's first point (none: 1e11s, fails)
uint8_t __fastcall ButtonTrigger_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x40) = owner;
    At<float>(self, 0x44) = 1e11f;
    At<float>(self, 0x48) = 1e11f;
    At<float>(self, 0x4C) = 1e11f;
    int32_t index = MC_RoutesFind(kRoutes, At<uint32_t>(self, 0x18));
    if (index == -1) {
        return 0;
    }
    float* first = MC_RouteFirst(MC_RoutesGet(kRoutes, index));
    At<float>(self, 0x44) = first[0];
    At<float>(self, 0x48) = first[1];
    At<float>(self, 0x4C) = first[2];
    At<float>(self, 0x50) = first[7];
    if (At<uint32_t>(self, 0x30) != 0) {
        MC_TextGet(kTheText, At<uint32_t>(self, 0x30));
    }
    return 1;
}

// 0x6A7AF0: CMissionDoorTriggerTrack's fields
void __fastcall DoorTrigger_Init(uint8_t* self) {
    At<uint8_t>(self, 8) = 1;
    actc::ClearName(self, 0x28);
    At<int32_t>(self, 0x1C) = -1;
    At<uint8_t>(self, 0x20) = 1;
    At<uint8_t>(self, 0x21) = 0;
    At<uint8_t>(self, 0x22) = 0;
    At<int32_t>(self, 0x24) = -1;
}

// 0x6A7B80 (vf2): Start: the door's route point and heading (none: zeros)
uint8_t __fastcall DoorTrigger_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x30) = owner;
    uint8_t* route = MC_RoutesGetById(kRoutes, At<uint32_t>(self, 0x2C));
    if (route == nullptr) {
        At<float>(self, 0x6C) = 0.0f;
        At<float>(self, 0x70) = 0.0f;
        At<float>(self, 0x74) = 0.0f;
        At<float>(self, 0x78) = 0.0f;
    } else {
        float* first = MC_RouteFirst(route);
        At<float>(self, 0x6C) = first[0];
        At<float>(self, 0x70) = first[1];
        At<float>(self, 0x74) = first[2];
        At<float>(self, 0x78) = first[7];
    }
    if (At<uint32_t>(self, 0x28) != 0) {
        MC_TextGet(kTheText, At<uint32_t>(self, 0x28));
    }
    At<int32_t>(self, 0x68) = -1;
    return 1;
}

// 0x6A7C10 (vf4): End: its blip removed
uint8_t __fastcall DoorTrigger_End(uint8_t* self) {
    if (At<int32_t>(self, 0x68) > 0) {
        MC_RadarRemoveBlip(g_radar, At<int32_t>(self, 0x68));
    }
    return 1;
}

// 0x6A7C30: CMissionDoorLockTrack's fields
void __fastcall DoorLock_Init(uint8_t* self) {
    At<uint8_t>(self, 8) = 1;
    At<uint8_t>(self, 0x18) = 0;
    At<int32_t>(self, 0x1C) = -1;
    At<uint8_t>(self, 0x20) = 1;
    At<uint8_t>(self, 0x21) = 1;
    At<int32_t>(self, 0x24) = -1;
    At<uint8_t>(self, 0x19) = 0;
    At<int32_t>(self, 0x38) = -1;
}

// 0x6A7C80: CTutorialShowMessageTrack's fields
void __fastcall ShowMessage_Init(uint8_t* self) {
    At<uint8_t>(self, 8) = 1;
    actc::ClearName(self, 0x18);
    At<uint8_t>(self, 0x1C) = 0;
}

// 0x6A7CD0 (vf3): Update: the message shown (once the tutorial HUD is free): the skate
// and weapon-wheel messages' keyboard versions without an XInput pad, the bike-trick
// ones not at all with a used pad
uint8_t __fastcall ShowMessage_Update(uint8_t* self, re::Edx, float) {
    void* hud = g_tutorialHud;
    if (At<uint8_t>(hud, 0x15) != 0 || vc::Call<uint8_t>(hud, 0x10) != 0) {
        return 1;
    }
    if (!MC_PadIsXInput(MC_ActivePad())) {
        const char* key = At<const char*>(self, 0x18);
        if (SameString(key, "TUT_SKATE01")) {
            actc::StringRef_Assign(&At<uint32_t>(self, 0x18), 0x009417D8);  // ("TUT_SKATE01_DI")
        } else if (SameString(key, "TUT_WW1")) {
            actc::StringRef_Assign(&At<uint32_t>(self, 0x18), 0x009417CC);  // ("TUT_WW1_DI")
        } else if (MC_PadIsUsed(MC_ActivePad(), 0)) {
            key = At<const char*>(self, 0x18);
            if (SameString(key, "TUT_BKTR1") || SameString(key, "TUT_BKTR2")) {
                return 1;
            }
        }
    }
    MC_TutorialSetText(hud, At<uint32_t>(self, 0x18));
    vc::Call<void>(hud, 0x14, 1);
    At<uint8_t>(hud, 0x3C) = At<uint8_t>(self, 0x1C);
    return 1;
}

// 0x6A7DC0 (vf4): End: the message cleared
uint8_t __fastcall ShowMessage_End(void*) {
    vc::Call<void>(g_tutorialHud, 0x1C);
    return 1;
}

// 0x6A7DE0 (vf2): the tutorial completed
uint8_t __fastcall CompleteActive_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    g_tutorialComplete = 1;
    return 1;
}

// 0x6A7E10 (vf2): the game paused for the tutorial (+0x18) or resumed
uint8_t __fastcall TutorialPause_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t pause = At<uint8_t>(self, 0x18);
    At<uint8_t*>(self, 0x1C) = owner;
    if (pause != 0) {
        g_tutorialPaused = 1;
        MC_PadStopRumble();
        MC_StartUserPause();
        return 1;
    }
    g_tutorialPaused = 0;
    MC_EndUserPause();
    MC_GenerateBoundingBoxes(0);
    return 1;
}

// 0x6A7E70 (vf2): a tutorial (+0x18) marked started, the classes updated
uint8_t __fastcall TutorialStart_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    int32_t mission = MC_MissionFind(kMissionMgr, &At<uint32_t>(self, 0x18));
    At<uint8_t>(MC_MissionFlags(kMissionMgr, mission), 4) |= 2;
    MC_MissionUpdateClasses(kMissionMgr, 1);
    return 1;
}

// 0x6A8090 (vf2): Start: a character trigger at the route's first point
uint8_t __fastcall CharTrigger_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x54) = owner;
    const char* model = At<const char*>(self, 0x18);
    uint32_t routeName = At<uint32_t>(self, 0x1C);
    At<int32_t>(self, 0x68) = -1;
    if (model == nullptr || routeName == 0) {
        return 0;
    }
    MC_GetModelInfo(model, &At<int32_t>(self, 0x58));
    int32_t index = MC_RoutesFind(kRoutes, routeName);
    if (index == -1) {
        return 0;
    }
    uint8_t* route = MC_RoutesGet(kRoutes, index);
    float* first = MC_RouteFirst(route);
    At<float>(self, 0x5C) = first[0];
    At<float>(self, 0x60) = first[1];
    At<float>(self, 0x64) = first[2];
    At<int32_t>(self, 0x68) = At<int8_t>(route, 0xC);
    At<int32_t>(self, 0x74) = At<int32_t>(self, 0x44);
    At<int32_t>(self, 0x70) = At<int32_t>(self, 0x40);
    At<float>(self, 0x6C) = DegreesToRadians(first[7]);
    float radius = At<float>(self, 0x28);
    uint8_t* node = nullptr;
    if (At<const char*>(self, 0x50) != nullptr) {
        node = MC_ActTreeFind(At<const char*>(self, 0x50), nullptr);
    }
    MC_CharTriggersAdd(kCharTriggers, At<int32_t>(self, 0x58), At<uint8_t>(self, 0x68), &At<float>(self, 0x5C),
                       At<float>(self, 0x6C), radius, At<int32_t>(self, 0x24), At<int32_t>(self, 0x20),
                       At<int32_t>(self, 0x30), At<int32_t>(self, 0x34), At<int32_t>(self, 0x38),
                       At<uint8_t>(self, 0x3C), At<uint8_t>(self, 0x3D), At<int32_t>(self, 0x48),
                       At<int32_t>(self, 0x70), At<int32_t>(self, 0x74), At<uint8_t>(self, 0x2C),
                       OwnerMission(owner), At<uint8_t>(self, 0x2D), At<int32_t>(self, 0x4C),
                       node);
    return 1;
}

// 0x6A81D0 (vf4): End: the trigger removed
uint8_t __fastcall CharTrigger_End(uint8_t* self) {
    MC_CharTriggersRemove(kCharTriggers, At<uint8_t>(self, 0x68), &At<float>(self, 0x5C));
    return 1;
}

// 0x6A81F0 (vf3): Update: while the mission's available: its help near the door, the
// marker, the blip (off once the mission's flag 0x40 is set); walking through the area
// transition to the door (+0x18) starts the mission. Unavailable: the blip removed.
uint8_t __fastcall DoorTrigger_Update(uint8_t* self, re::Edx, float) {
    void* radar = g_radar;
    int32_t mission = -1;
    uint8_t* slot = At<uint8_t*>(At<uint8_t*>(self, 0x30), 0x20);
    if (slot != nullptr) {
        mission = At<int32_t>(slot, 8);
    }
    if (!MC_MissionAvailable(kMissionMgr, mission, At<uint8_t>(self, 0x20))) {
        if (At<int32_t>(self, 0x68) > 0) {
            MC_RadarRemoveBlip(radar, At<int32_t>(self, 0x68));
            At<int32_t>(self, 0x68) = -1;
        }
        return 1;
    }
    const float* playerPos = EntityPos(g_playerPed);
    float player[3] = {playerPos[0], playerPos[1], playerPos[2]};
    float* door = &At<float>(self, 0x6C);
    float delta[3];
    float* d = MC_VectorSub(delta, door, player);
    float sum = (d[0] * d[0] + d[1] * d[1]) + d[2] * d[2];
    float distance = std::sqrt(std::fabs(sum));
    if (distance < 2.0f && At<uint32_t>(self, 0x28) != 0) {
        int32_t colour[4];
        std::memcpy(colour, g_helpColour + 0x18, sizeof(colour));
        MC_FontReset();
        g_fontColour[0] = uint8_t(colour[0]);
        g_fontColour[1] = uint8_t(colour[1]);
        g_fontColour[2] = uint8_t(colour[2]);
        g_fontColour[3] = uint8_t(colour[3]);
        MC_ShowHelp(At<uint32_t>(self, 0x28), 0.1f);
    }
    float rotation[3] = {At<float>(self, 0x78), 0.0f, 0.0f};
    MC_PlaceMarker(At<int32_t>(self, 0x1C), -1, 0, door, 1.0f, rotation, mission, 0, 0x72, 0xBC, nullptr, 1, 5);
    if (At<int32_t>(self, 0x68) != -1) {
        if (mission >= 0 && (At<uint8_t>(MC_MissionFlags(kMissionMgr, mission), 4) & 0x40) != 0) {
            MC_RadarRemoveBlip(radar, At<int32_t>(self, 0x68));
            At<int32_t>(self, 0x68) = -1;
        }
    }
    if (At<int32_t>(self, 0x68) == -1 && At<uint8_t>(self, 0x21) != 0 &&
        !(mission >= 0 && (At<uint8_t>(MC_MissionFlags(kMissionMgr, mission), 4) & 0x40) != 0)) {
        uint32_t colour = 0xFFBC7200;  // (0, 0x72, 0xBC, 0xFF)
        int32_t blip = MC_RadarAddBlip(radar, 4, door[0], door[1], door[2], colour, 1, At<int32_t>(self, 0x24),
                                       At<uint8_t>(self, 0x22));
        At<int32_t>(self, 0x68) = blip;
        MC_RadarSetBlipPos(radar, blip, door);
        MC_RadarSetBlipSize(radar, At<int32_t>(self, 0x68), 5);
        MC_RadarSetBlipMission(radar, At<int32_t>(self, 0x68), mission);
    }
    if (!MC_PedInVehicle(g_playerPed)) {
        uint8_t* target = MC_AreaTransitionTarget(kAreaTransition);
        if (target != nullptr && At<uint32_t>(target, 0x2C) == At<uint32_t>(self, 0x18)) {
            MC_AreaTransitionSetTarget(kAreaTransition, nullptr);
            uint8_t* owner = At<uint8_t*>(self, 0x30);
            if (At<uint8_t*>(owner, 0x20) != nullptr) {
                MC_MissionStart(kMissionMgr, OwnerMission(owner), 0);
            }
        }
    }
    return 1;
}

// 0x6A84A0 (vf2): Start: the door (+0x28) locked / unlocked (+0x18) and its blip added
uint8_t __fastcall DoorLock_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x2C) = owner;
    At<int32_t>(self, 0x30) = -1;
    uint32_t name = At<uint32_t>(self, 0x28);
    if (name == 0) {
        return 0;
    }
    uint8_t* trigger = TriggerByName(name);
    At<uint8_t*>(self, 0x34) = trigger;
    At<int32_t>(self, 0x38) = At<int32_t>(g_areaCounter, 4);
    if (trigger == nullptr) {
        MC_DoorsSetLocked(kDoors, name, At<uint8_t>(self, 0x18));
        return 1;
    }
    MC_TriggerSetLocked(trigger, At<uint8_t>(self, 0x18));
    if (At<uint8_t>(self, 0x20) == 0) {
        return 1;
    }
    float pos[3];
    TriggerPos(At<uint8_t*>(self, 0x34), pos);
    int32_t type = At<int32_t>(self, 0x24);
    void* radar = g_radar;
    int size = 5;
    if (type == 9 || type == 0xA || type == 0xB || type == 0xC || type == 0xD || type == 0xE || type == 0x16) {
        size = 8;
    }
    uint8_t flag = At<uint8_t>(self, 0x21);
    uint8_t colour[4];
    uint32_t packed = *reinterpret_cast<uint32_t*>(actc::Thiscall<0x00510DE0, uint8_t*>(colour, 0, 0x72, 0xBC, 0xFF));
    int32_t blip = MC_RadarAddBlip(radar, size, pos[0], pos[1], pos[2], packed, 1, type, flag);
    At<int32_t>(self, 0x30) = blip;
    uint8_t* slot = At<uint8_t*>(At<uint8_t*>(self, 0x2C), 0x20);
    if (slot != nullptr) {
        MC_RadarSetBlipMission(radar, blip, At<int32_t>(slot, 8));
    }
    return 1;
}

// 0x6A8630 (vf3): Update: the trigger looked up again after an area change; an unlocked
// door on screen gets its marker
uint8_t __fastcall DoorLock_Update(uint8_t* self, re::Edx, float) {
    uint32_t name = At<uint32_t>(self, 0x28);
    if (name != 0 && At<int32_t>(self, 0x38) != At<int32_t>(g_areaCounter, 4)) {
        At<uint8_t*>(self, 0x34) = TriggerByName(name);
        At<int32_t>(self, 0x38) = At<int32_t>(g_areaCounter, 4);
    }
    uint8_t* trigger = At<uint8_t*>(self, 0x34);
    if (trigger != nullptr && At<uint8_t>(self, 0x18) == 0 && At<uint8_t>(self, 0x20) != 0) {
        float pos[3];
        TriggerPos(trigger, pos);
        if (MC_IsOnScreen(pos)) {
            MC_PlaceMarker(At<uint8_t>(self, 0x1C), -1, 1, pos, 1.0f, g_zeroVector, -1, 0, 0x72, 0xBC,
                           At<uint8_t*>(self, 0x34), 1, 5);
        }
    }
    return 1;
}

// 0x6A8740 (vf4): End: the lock undone (when +0x19), the blip removed
uint8_t __fastcall DoorLock_End(uint8_t* self) {
    if (At<uint8_t>(self, 0x19) != 0 && At<uint32_t>(self, 0x28) != 0) {
        uint32_t name = At<uint32_t>(self, 0x28);
        uint8_t* trigger = At<uint8_t*>(self, 0x34);
        if (trigger != nullptr && At<uint32_t>(trigger, 0x2C) != name) {
            At<uint8_t*>(self, 0x34) = MC_TriggersFromName(kTriggers, name);
        }
        if (At<uint8_t*>(self, 0x34) != nullptr) {
            MC_TriggerSetLocked(At<uint8_t*>(self, 0x34), At<uint8_t>(self, 0x18) == 0);
        } else {
            MC_DoorsSetLocked(kDoors, At<uint32_t>(self, 0x28), At<uint8_t>(self, 0x18) == 0);
        }
    }
    int32_t blip = At<int32_t>(self, 0x30);
    if (blip >= 0) {
        MC_RadarRemoveBlip(g_radar, blip);
    }
    return 1;
}

// 0x6A87B0 (vf3): Update: in the button's area (the mission available, the player on
// foot, no vehicle near): the prompt; the button (pad 10 not held, 9 pressed) pays the
// mission's cost (else its "no money" help) and starts it
uint8_t __fastcall ButtonTrigger_Update(uint8_t* self, re::Edx, float) {
    if (At<float>(self, 0x44) == 1e11f && At<float>(self, 0x48) == 1e11f && At<float>(self, 0x4C) == 1e11f) {
        return 0;
    }
    int32_t mission = OwnerMission(At<uint8_t*>(self, 0x40));
    uint8_t* flags = MC_MissionFlags(kMissionMgr, mission);
    uint8_t flags4 = flags[4];
    uint8_t* info = MC_MissionInfo(kMissionMgr, mission);
    if (!MC_MissionAvailable(kMissionMgr, mission, At<uint8_t>(self, 0x2D))) {
        return 1;
    }
    float radius = At<float>(self, 0x1C);
    int32_t blip = -1;
    if (At<uint8_t>(self, 0x24) != 0 && (flags4 & 0x40) == 0) {
        blip = At<int32_t>(self, 0x28);
    }
    float rotation[3] = {0.0f, 0.0f, DegreesToRadians(At<float>(self, 0x50))};
    if (!MC_IsEntityInArea(1, 0, 3, At<float>(self, 0x44), At<float>(self, 0x48), At<float>(self, 0x4C), radius,
                           radius, radius, At<int32_t>(self, 0x20), rotation, blip, mission,
                           At<uint8_t>(self, 0x2C), info, 0)) {
        return 1;
    }
    if (!MC_InputEnabled(kUserInput) || MC_Ped473FD0(g_playerPed) || MC_PedInVehicle(g_playerPed)) {
        return 1;
    }
    if (MC_FindVehicle(g_playerPed, 3.01f, 0, 0, 0) != nullptr ||
        MC_FindVehicle(g_playerPed, 2.01f, 1, 0, 0) != nullptr) {
        return 1;
    }
    alignas(4) uint8_t data[0x4C];  // (a CMissionData copy)
    MC_MissionDataCopy(data, MC_MissionInfo(kMissionMgr, OwnerMission(At<uint8_t*>(self, 0x40))));
    if (At<uint32_t>(self, 0x30) != 0) {
        if (At<uint32_t>(data, 0x20) != 0) {
            MC_HudTextAdd(g_hudText, 3, At<uint32_t>(data, 0x20));
        }
        MC_ShowPrompt(At<uint32_t>(self, 0x30), 1.0f);
    }
    if (MC_PadValue(0xA, 0) <= 0.0f && float(MC_PadJustDown(9, 0)) > 0.0f) {
        if (At<uint8_t>(data, 0x3C) != 0 && !MC_PedHasMoney(g_playerPed, At<int32_t>(data, 0x40))) {
            MC_ShowHelp(At<uint32_t>(data, 0x44), 2.0f);
            MC_MissionDataDestruct(data);
            return 1;
        }
        uint8_t* owner = At<uint8_t*>(self, 0x40);
        if (At<uint8_t*>(owner, 0x20) != nullptr) {
            g_buttonTriggered = 1;
            MC_MissionStart(kMissionMgr, OwnerMission(At<uint8_t*>(self, 0x40)), 0);
            uint8_t* node = MC_ActTreeFind("/Global/SimpleLoco/Mission/InvulnerableIdle", nullptr);
            MC_ControllerPlay(At<uint8_t*>(g_playerPed, 0x2E0) + 0x258, node);
        }
    }
    MC_MissionDataDestruct(data);
    return 1;
}

// 0x8F7810, 0x8F7820 (exit): the "CurrentClassMission" and "CurrentMission" keys' destructors
void __cdecl CurrentClassMissionKey_Exit() {
    MC_NameKey_Destruct(&g_keyCurrentClassMission);
}
void __cdecl CurrentMissionKey_Exit() {
    MC_NameKey_Destruct(&g_keyCurrentMission);
}

} // namespace mcond

#define MCOND_REVERSED(name, address, fn)                                                          \
    static void* RE_CONCAT(mcond_original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(mcond_original_, __LINE__), \
             re::Mode::Original)

MCOND_REVERSED("CMissionData::Init", 0x006A71D0, &mcond::MissionData_Init);
MCOND_REVERSED("MissionConditions::CurrentClassMissionKeyExit", 0x008F7810, &mcond::CurrentClassMissionKey_Exit);
MCOND_REVERSED("MissionConditions::CurrentMissionKeyExit", 0x008F7820, &mcond::CurrentMissionKey_Exit);
MCOND_REVERSED("CMissionCond::IsA", 0x006A72A0, (&actc::IsA<0x020C358C, 0x020C3588, 0x005FC700>));
MCOND_REVERSED("CErrandCond::CErrandCond", 0x006A72F0, &mcond::ErrandCond_Construct);
MCOND_REVERSED("CErrandCond::Evaluate", 0x006A7310, &mcond::ErrandCond_Evaluate);
MCOND_REVERSED("ConditionLastMission::ConditionLastMission", 0x006A7330, &mcond::LastMission_Construct);
MCOND_REVERSED("ConditionLastMission::Evaluate", 0x006A7360, &mcond::LastMission_Evaluate);
MCOND_REVERSED("CMissionUnlockedCond::CMissionUnlockedCond", 0x006A73D0, &mcond::Unlocked_Construct);
MCOND_REVERSED("CMissionUnlockedCond::Evaluate", 0x006A7400, &mcond::Unlocked_Evaluate);
MCOND_REVERSED("CClassSkippedCond::Init", 0x006A7460, &mcond::ClassSkipped_Init);
MCOND_REVERSED("CClassSkippedCond::~CClassSkippedCond", 0x006A7470, &mcond::ClassSkipped_Destruct);
MCOND_REVERSED("CClassSkippedCond::Evaluate", 0x006A7480, &mcond::ClassSkipped_Evaluate);
MCOND_REVERSED("CTimeCond::Init", 0x006A74B0, &mcond::TimeCond_Init);
MCOND_REVERSED("CTimeCond::~CTimeCond", 0x006A74C0, &mcond::TimeCond_Destruct);
MCOND_REVERSED("CTimeCond::GetName", 0x006A74D0, (&mcond::NameOf<0x0093917C>));
MCOND_REVERSED("CTimeCond::Evaluate", 0x006A74E0, &mcond::TimeCond_Evaluate);
MCOND_REVERSED("CPeriodCond::CPeriodCond", 0x006A7500, &mcond::PeriodCond_Construct);
MCOND_REVERSED("CPeriodCond::Init", 0x006A7530, &mcond::PeriodCond_Init);
MCOND_REVERSED("CPeriodCond::Evaluate", 0x006A7570, &mcond::PeriodCond_Evaluate);
MCOND_REVERSED("CDayCond::Init", 0x006A75C0, &mcond::DayCond_Init);
MCOND_REVERSED("CDayCond::~CDayCond", 0x006A75D0, &mcond::DayCond_Destruct);
MCOND_REVERSED("CDayCond::Evaluate", 0x006A75E0, &mcond::DayCond_Evaluate);
MCOND_REVERSED("CLuaCond::~CLuaCond", 0x006A7600, &mcond::LuaCond_Destruct);
MCOND_REVERSED("CLuaCond::GetName", 0x006A7620, (&mcond::NameOf<0x00939160>));
MCOND_REVERSED("CLuaCond::Evaluate", 0x006A7630, &mcond::LuaCond_Evaluate);
MCOND_REVERSED("CMissionCharTriggerTrack::Init", 0x006A76D0, &mcond::CharTrigger_Init);
MCOND_REVERSED("CMissionCharTriggerTrack::~CMissionCharTriggerTrack", 0x006A77F0,
               (&actc::TrackDtor<0x00934D44, 0x50, 0x44, 0x40, 0x24, 0x20, 0x1C, 0x18>));
MCOND_REVERSED("CMissionCharTriggerTrack::GetName", 0x006A7840, (&mcond::NameOf<0x0093B5BC>));
MCOND_REVERSED("CMissionVehicleTriggerTrack::Init", 0x006A7850, &mcond::VehicleTrigger_Init);
MCOND_REVERSED("CMissionVehicleTriggerTrack::Start", 0x006A78A0, &mcond::VehicleTrigger_Start);
MCOND_REVERSED("CMissionVehicleTriggerTrack::End", 0x006A7980, &mcond::VehicleTrigger_End);
MCOND_REVERSED("CMissionButtonTriggerTrack::Init", 0x006A79A0, &mcond::ButtonTrigger_Init);
MCOND_REVERSED("CMissionButtonTriggerTrack::~CMissionButtonTriggerTrack", 0x006A7A20,
               (&actc::TrackDtor<0x00934DB0, 0x30, 0x18>));
MCOND_REVERSED("CMissionButtonTriggerTrack::Start", 0x006A7A50, &mcond::ButtonTrigger_Start);
MCOND_REVERSED("CMissionDoorTriggerTrack::Init", 0x006A7AF0, &mcond::DoorTrigger_Init);
MCOND_REVERSED("CMissionDoorTriggerTrack::~CMissionDoorTriggerTrack", 0x006A7B40,
               (&actc::TrackDtor<0x00934D94, 0x2C, 0x28, 0x18>));
MCOND_REVERSED("CMissionDoorTriggerTrack::GetName", 0x006A7B70, (&mcond::NameOf<0x0093B574>));
MCOND_REVERSED("CMissionDoorTriggerTrack::Start", 0x006A7B80, &mcond::DoorTrigger_Start);
MCOND_REVERSED("CMissionDoorTriggerTrack::End", 0x006A7C10, &mcond::DoorTrigger_End);
MCOND_REVERSED("CMissionDoorLockTrack::Init", 0x006A7C30, &mcond::DoorLock_Init);
MCOND_REVERSED("CMissionDoorLockTrack::~CMissionDoorLockTrack", 0x006A7C50, (&actc::TrackDtor<0x00934D78, 0x28>));
MCOND_REVERSED("CMissionDoorLockTrack::GetName", 0x006A7C70, (&mcond::NameOf<0x0094179C>));
MCOND_REVERSED("CTutorialShowMessageTrack::Init", 0x006A7C80, &mcond::ShowMessage_Init);
MCOND_REVERSED("CTutorialShowMessageTrack::~CTutorialShowMessageTrack", 0x006A7CB0,
               (&actc::TrackDtor<0x00935950, 0x18>));
MCOND_REVERSED("CTutorialShowMessageTrack::Update", 0x006A7CD0, &mcond::ShowMessage_Update);
MCOND_REVERSED("CTutorialShowMessageTrack::End", 0x006A7DC0, &mcond::ShowMessage_End);
MCOND_REVERSED("CTutorialCompleteActiveTrack::~CTutorialCompleteActiveTrack", 0x006A7DD0,
               (&actc::TrackDtor<0x00935968>));
MCOND_REVERSED("CTutorialCompleteActiveTrack::Start", 0x006A7DE0, &mcond::CompleteActive_Start);
MCOND_REVERSED("CTutorialPauseTrack::~CTutorialPauseTrack", 0x006A7E00, (&actc::TrackDtor<0x00935980>));
MCOND_REVERSED("CTutorialPauseTrack::Start", 0x006A7E10, &mcond::TutorialPause_Start);
MCOND_REVERSED("CTutorialStartTrack::~CTutorialStartTrack", 0x006A7E50, (&actc::TrackDtor<0x00935998, 0x18>));
MCOND_REVERSED("CTutorialStartTrack::Start", 0x006A7E70, &mcond::TutorialStart_Start);
MCOND_REVERSED("CMissionCond::CMissionCond", 0x006A7EB0, &mcond::MissionCond_Construct);
MCOND_REVERSED("CMissionCond::DeletingDestruct", 0x006A7EE0, (&mcond::CondDeletingDtor<0x0094173C, true>));
MCOND_REVERSED("CMissionCond::Evaluate", 0x006A7F10, &mcond::MissionCond_Evaluate);
MCOND_REVERSED("CErrandCond::DeletingDestruct", 0x006A7FE0, (&mcond::CondDeletingDtor<0x00941750, false>));
MCOND_REVERSED("ConditionLastMission::DeletingDestruct", 0x006A8000, (&mcond::CondDeletingDtor<0x00941764, true>));
MCOND_REVERSED("CMissionUnlockedCond::DeletingDestruct", 0x006A8030, (&mcond::CondDeletingDtor<0x00941778, true>));
MCOND_REVERSED("CPeriodCond::DeletingDestruct", 0x006A8060, (&mcond::CondDeletingDtor<0x0094178C, true>));
MCOND_REVERSED("CMissionCharTriggerTrack::Start", 0x006A8090, &mcond::CharTrigger_Start);
MCOND_REVERSED("CMissionCharTriggerTrack::End", 0x006A81D0, &mcond::CharTrigger_End);
MCOND_REVERSED("CMissionDoorTriggerTrack::Update", 0x006A81F0, &mcond::DoorTrigger_Update);
MCOND_REVERSED("CMissionDoorLockTrack::Start", 0x006A84A0, &mcond::DoorLock_Start);
MCOND_REVERSED("CMissionDoorLockTrack::Update", 0x006A8630, &mcond::DoorLock_Update);
MCOND_REVERSED("CMissionDoorLockTrack::End", 0x006A8740, &mcond::DoorLock_End);
MCOND_REVERSED("CMissionButtonTriggerTrack::Update", 0x006A87B0, &mcond::ButtonTrigger_Update);
