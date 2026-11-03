// The action tree's conditions (continued): the ped's own node, water, the surface,
// trigger volumes and points, its kind (faction, sex, scenario / POI roles), what it
// senses, its stats and fight timers, and the stimuli it reacts to.
// 0x5FF8B0..0x600720.
//
// Arithmetic is float in the game's order (its x87 runs in single precision, D3D9's
// default); a dword loaded by fild is exact until the next operation.

#include <cstdint>

#include "game/ActionTree.h"

#include "game/Crt.h"
using namespace acttree;

namespace {

GAME_VAR(uint8_t*, player_C1AEA8, 0x00C1AEA8);
GAME_VAR(uint32_t, timeMs_C1A9B4, 0x00C1A9B4);
GAME_VAR(uint8_t*, poiManager_C0F5E4, 0x00C0F5E4);
GAME_VAR(uint8_t*, stimuli_C2C1A0, 0x00C2C1A0);
GAME_VAR(uint8_t*, volumePool_20C7C48, 0x020C7C48);
GAME_VAR(float, noAttack_9009AC, 0x009009AC);         // -1
GAME_VAR(double, msPerSecond_900138, 0x00900138);     // 1000
GAME_VAR(double, percent_900130, 0x00900130);         // 100
GAME_VAR(double, randMax_900CE8, 0x00900CE8);         // 32767
GAME_VAR(float, twoPow32_900AC8, 0x00900AC8);         // 2^32
GAME_ARRAY(uint32_t, neutralStateName_1B652F0, 0x01B652F0);
GAME_ARRAY(uint32_t, aiBusyName_1B65288, 0x01B65288);

uint8_t* const kSurfaceHardness = reinterpret_cast<uint8_t*>(0x00C1A7E8);
uint8_t* const kTriggerVolumes = reinterpret_cast<uint8_t*>(0x020C7E98);
uint8_t* const kPointLists = reinterpret_cast<uint8_t*>(0x00BC5E7C);

GAME_FN(uint32_t, __cdecl, CompareFloatsE, 0x005FB8B0, float a, float b, int op);
GAME_FN(uint32_t, __thiscall, Ctl_IsPlaying_5F4800e, 0x005F4800, uint8_t* controller, const uint32_t* name, uint32_t ancestors,
        int nested);
GAME_FN(void, __thiscall, Hardness_Get_45B2F0, 0x0045B2F0, uint8_t* table, int32_t* out, const int32_t* surface);
GAME_FN(int32_t, __thiscall, Volumes_FindByName_6D7A10, 0x006D7A10, uint8_t* volumes, uintptr_t name);
GAME_FN(uint8_t, __thiscall, Area_Contains_6D6FA0, 0x006D6FA0, uint8_t* area, const float* point);
GAME_FN(uint32_t, __thiscall, Volumes_ContainsOfType_6D7AB0, 0x006D7AB0, uint8_t* volumes, uint32_t type, const float* point);
GAME_FN(int32_t, __thiscall, PointLists_Find_6D63C0, 0x006D63C0, uint8_t* lists, uintptr_t name);
GAME_FN(uint8_t*, __thiscall, PointLists_Get_6D6450, 0x006D6450, uint8_t* lists, int32_t index);
GAME_FN(uint32_t, __thiscall, PointList_Count_6D5950, 0x006D5950, uint8_t* list);
GAME_FN(float*, __thiscall, PointList_First_6D59B0, 0x006D59B0, uint8_t* list);
GAME_FN(float*, __cdecl, VectorSubE, 0x00414B70, float* out, const float* a, const float* b);
GAME_FN(float, __thiscall, Vector_Magnitude_40DFB0, 0x0040DFB0, const float* v);
GAME_FN(uint8_t, __thiscall, Seats_Contains_4CEA70e, 0x004CEA70, uint8_t* seats, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, Ped_GetModelInfo_477DC0, 0x00477DC0, uint8_t* ped);
GAME_FN(uint32_t, __thiscall, Ped_IsScenarioPed_473220, 0x00473220, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, Ped_GetFightTarget_497100e, 0x00497100, uint8_t* ped);
GAME_FN(uint32_t, __thiscall, PedCombat_HasRemembered_486300, 0x00486300, uint8_t* combat);
GAME_FN(uint32_t, __thiscall, POIManager_FindPOIForPed_448B90, 0x00448B90, uint8_t* manager, uint8_t* ped, float range);
GAME_FN(uint8_t*, __thiscall, POIManager_FindByPed_448460, 0x00448460, uint8_t* manager, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Social_CanTalkTo_48D470, 0x0048D470, uint8_t* social, uint8_t* ped);
GAME_FN(void, __thiscall, Scanner_Alert_48C480, 0x0048C480, uint8_t* scanner, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, PedSight_CanSeeEntity_48D1F0, 0x0048D1F0, uint8_t* sight, uint8_t* entity);
GAME_FN(uint8_t*, __thiscall, Scanner_GetThreat_48C070, 0x0048C070, uint8_t* scanner);
GAME_FN(uint32_t, __thiscall, Ped_IsOnUsedEntitysLeft_477930, 0x00477930, uint8_t* ped);
GAME_FN(int32_t, __thiscall, Ped_GetStat_477060, 0x00477060, uint8_t* ped, int32_t stat);
inline int __cdecl Rand_85C6FA() { return crt::Rand(); }
GAME_FN(void, __thiscall, Scanner_SetFlag_48C190, 0x0048C190, uint8_t* scanner, int value);
GAME_FN(uint8_t*, __thiscall, ObjectiveQueue_FindOfType_4701D0, 0x004701D0, uint8_t* queue, int32_t type);
GAME_FN(uint32_t, __thiscall, Scanner_IsAroundAndSeen_48C140, 0x0048C140, uint8_t* scanner, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Stimulus_HasExpiredBy_4A5C50, 0x004A5C50, uint8_t* stimulus, uint32_t time);
GAME_FN(uint8_t*, __thiscall, Stimuli_FindStrongestRecent_4A61A0, 0x004A61A0, uint8_t* manager, uint8_t* ped, int32_t kind);
GAME_FN(uint8_t*, __thiscall, Stimulus_GetOffender_4A5CB0, 0x004A5CB0, uint8_t* stimulus);
GAME_FN(uint8_t*, __thiscall, Stimulus_GetSource_4A5CD0, 0x004A5CD0, uint8_t* stimulus);
GAME_FN(void, __thiscall, Ped_SetTarget_497E70, 0x00497E70, uint8_t* ped, uint8_t* target, int kind, const float* offset);
GAME_FN(void, __thiscall, Ped_SetTargetEntity_498B10, 0x00498B10, uint8_t* ped, uint8_t* target);

uint8_t* Ped(uint8_t* owner) {
    return vc::At<uint8_t*>(owner, 8);
}

const float* EntityPos(uint8_t* entity) {
    uint8_t* matrix = vc::At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<float*>(matrix + 0x30) : reinterpret_cast<float*>(entity + 4);
}

bool IsPed(uint8_t* entity) {
    return (entity[0x108] & 7) == 3;
}

// an unsigned dword on the x87: fild (exact), 2^32 added (rounded) when its top bit is set
double X87Unsigned(uint32_t value) {
    if (int32_t(value) >= 0) {
        return double(value);
    }
    return float(double(int32_t(value)) + double(twoPow32_900AC8));
}

// a time stamp plus seconds as ms: fld s; fmul 1000.0; fild t; faddp
float StampPlusSeconds(uint32_t stamp, float seconds) {
    float ms = float(double(seconds) * msPerSecond_900138);
    return float(X87Unsigned(stamp) + double(ms));
}

// 0x5FF8B0 (thiscall): ConditionMePlaying: the ped's own controller plays the node named
// at +4 (+8: or an ancestor). The Evaluates are byte bools: the callers test al (eax's
// upper bytes are whatever the last call left)
uint8_t __fastcall MePlaying_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* controller = vc::At<uint8_t*>(Ped(owner), 0x2E0) + 0x258;
    return uint8_t(Ctl_IsPlaying_5F4800e(controller, reinterpret_cast<uint32_t*>(self + 4), self[8], 1));
}

// 0x5FF8E0 (thiscall): ConditionMePlayingDefaultNeutralState: in the neutral state node
// with no +0x142C
uint8_t __fastcall MePlayingDefaultNeutralState_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* controller = vc::At<uint8_t*>(Ped(owner), 0x2E0) + 0x258;
    uint8_t playing = uint8_t(Ctl_IsPlaying_5F4800e(controller, neutralStateName_1B652F0, 1, 1));
    uint8_t none = vc::At<int32_t>(Ped(owner), 0x142C) == 0;
    return uint8_t(playing & none);   // (sete / and on al)
}

// 0x5FF920 (thiscall): ConditionAIIsPlaying: the AI's controller (+0x290) plays it
uint8_t __fastcall AIIsPlaying_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* controller = vc::At<uint8_t*>(Ped(owner), 0x2E0) + 0x290;
    return uint8_t(Ctl_IsPlaying_5F4800e(controller, reinterpret_cast<uint32_t*>(self + 4), self[8], 1));
}

// 0x5FF950 (thiscall): ConditionLiquidLevel: in water, the ped's height over its surface
// compared
uint8_t __fastcall LiquidLevel_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    if (!ped[0x16CC]) {
        return 0;
    }
    float level = vc::At<float>(ped, 0x16D0);
    float depth = EntityPos(ped)[2] - level;
    return uint8_t(CompareFloatsE(depth, vc::At<float>(self, 4), vc::At<int32_t>(self, 8)));
}

// 0x5FF9B0 (thiscall): ConditionLiquidType: in water of the type +4 (0: any)
uint8_t __fastcall LiquidType_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    if (!ped[0x16CC]) {
        return 0;
    }
    if (vc::At<int32_t>(self, 4) == 0) {
        return 1;
    }
    return vc::At<int32_t>(self, 4) == vc::At<int32_t>(ped, 0x1620);
}

// 0x5FF9F0 (thiscall): ConditionSurfaceHardness: the hardness of the surface the ped
// stands on
uint8_t __fastcall SurfaceHardness_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    int32_t surface = Ped(owner)[0x1BE];
    int32_t hardness;
    // (the game reads the result through the returned pointer: the out)
    Hardness_Get_45B2F0(kSurfaceHardness, &hardness, &surface);
    return hardness == vc::At<int32_t>(self, 4);
}

// the trigger volume named at +4, or null (none / a free pool slot)
uint8_t* NamedVolume(uint8_t* self) {
    int32_t index = Volumes_FindByName_6D7A10(kTriggerVolumes, vc::At<uint32_t>(self, 4));
    if (index == -1 || index < 0) {
        return nullptr;
    }
    uint8_t* pool = volumePool_20C7C48;
    if (vc::At<uint8_t*>(pool, 4)[index] & 0x80) {
        return nullptr;
    }
    return vc::At<uint8_t*>(pool, 0) + vc::At<int32_t>(pool, 0xC) * index;
}

// 0x5FFA30 (thiscall): ConditionInTriggerVolume: the ped inside the volume named at +4
uint8_t __fastcall InTriggerVolume_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* volume = NamedVolume(self);
    if (!volume) {
        return 0;
    }
    return Area_Contains_6D6FA0(volume, EntityPos(Ped(owner))) ? 1 : 0;
}

// 0x5FFAA0 (thiscall): ConditionPlayerInTriggerVolume: the player inside it
uint8_t __fastcall PlayerInTriggerVolume_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    uint8_t* volume = NamedVolume(self);
    if (!volume) {
        return 0;
    }
    return Area_Contains_6D6FA0(volume, EntityPos(player_C1AEA8)) ? 1 : 0;
}

// 0x5FFB00 (thiscall): ConditionInTriggerVolumeOfType: the ped inside any volume of the
// type +4
uint8_t __fastcall InTriggerVolumeOfType_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    if (!ped) {
        return 0;
    }
    return uint8_t(Volumes_ContainsOfType_6D7AB0(kTriggerVolumes, vc::At<uint32_t>(self, 4), EntityPos(ped)));
}

// 0x5FFB70 (thiscall): ConditionNearPoint: the first point of the list named at +4 (its
// index cached at +0xC) within +8 of the ped
uint8_t __fastcall NearPoint_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    if (vc::At<int32_t>(self, 0xC) == -1) {
        vc::At<int32_t>(self, 0xC) = PointLists_Find_6D63C0(kPointLists, vc::At<uint32_t>(self, 4));
    }
    int32_t index = vc::At<int32_t>(self, 0xC);
    if (index == -1) {
        return 0;
    }
    uint8_t* list = PointLists_Get_6D6450(kPointLists, index);
    if (!list || PointList_Count_6D5950(list) < 1) {
        return 0;
    }
    float* point = PointList_First_6D59B0(list);
    float d[3];
    float* delta = VectorSubE(d, EntityPos(Ped(owner)), point);
    float distance = Vector_Magnitude_40DFB0(delta);
    return vc::At<float>(self, 8) > distance ? 1 : 0;
}

// 0x5FFC20 (thiscall): ConditionPedType: the ped's faction
uint8_t __fastcall PedType_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    return vc::At<int32_t>(self, 4) == vc::At<int32_t>(Ped(owner), 0x1310);
}

// 0x5FFC40 (thiscall): ConditionPedIsDriver: seated in the vehicle it is on
uint8_t __fastcall PedIsDriver_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    uint8_t* vehicle = vc::At<uint8_t*>(ped, 0x1554);
    if (!vehicle || (vehicle[0x108] & 7) != 2) {
        return 0;
    }
    return Seats_Contains_4CEA70e(vehicle + 0x1C4, ped);
}

// 0x5FFC80 (thiscall): ConditionIsAuthority: prefect (0), cop (8) or 7
uint32_t __fastcall IsAuthority_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    int32_t faction = vc::At<int32_t>(Ped(owner), 0x1310);
    return faction == 0 || faction == 8 || faction == 7 ? 1u : 0u;
}

// 0x5FFCB0 (thiscall): ConditionIsFemale: the model info's flag
uint8_t __fastcall IsFemale_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return Ped_GetModelInfo_477DC0(Ped(owner))[0x50];
}

// 0x5FFCD0 (thiscall): ConditionIsMissionSpecific
uint32_t __fastcall IsMissionSpecific_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return Ped(owner)[0x1570] == 2 ? 1u : 0u;
}

// the ped's POI's kind (+0x70), or -1 without one
bool PoiKindIs(uint8_t* ped, int32_t kind) {
    uint8_t* poi = vc::At<uint8_t*>(ped, 0x1D24);
    return poi && vc::At<int32_t>(poi, 0x70) == kind;
}

// 0x5FFCF0 (thiscall): ConditionIsHangingOut: its POI of kind 2
uint8_t __fastcall IsHangingOut_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return PoiKindIs(Ped(owner), 2) ? 1 : 0;
}

// 0x5FFD20 (thiscall): ConditionIsSpectator: its POI of kind 1
uint8_t __fastcall IsSpectator_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return PoiKindIs(Ped(owner), 1) ? 1 : 0;
}

// 0x5FFD50 (thiscall): ConditionIsScenarioPed
uint32_t __fastcall IsScenarioPed_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return Ped_IsScenarioPed_473220(Ped(owner));
}

// 0x5FFD60 (thiscall): ConditionHasCompletedScenarioObj
uint8_t __fastcall HasCompletedScenarioObj_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    if (vc::At<int32_t>(ped, 0x14CC) != 0 && vc::At<int32_t>(ped, 0x13F8) != 0) {
        return 1;
    }
    if (!uint8_t(Ped_IsScenarioPed_473220(ped))) {
        return 0;
    }
    return vc::At<int32_t>(Ped(owner), 0x13F8) != 0;
}

// 0x5FFDA0 (thiscall): ConditionIsScriptedAmbient: its POI of a kind 3..15
uint32_t __fastcall IsScriptedAmbient_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* poi = vc::At<uint8_t*>(Ped(owner), 0x1D24);
    if (!poi) {
        return 0;
    }
    int32_t kind = vc::At<int32_t>(poi, 0x70);
    return kind > 2 && kind < 0x10 ? 1u : 0u;
}

// 0x5FFDD0 (thiscall): ConditionIsTargetAttackRightsTaken: the fight target (a ped)
// remembers it
uint8_t __fastcall IsTargetAttackRightsTaken_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* target = Ped_GetFightTarget_497100e(Ped(owner));
    if (!target || !IsPed(target)) {
        return 0;
    }
    return uint8_t(PedCombat_HasRemembered_486300(target + 0x668));
}

// 0x5FFE10 (thiscall): ConditionAcquirePOI: a POI within +4 found for the ped
uint32_t __fastcall AcquirePOI_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    return POIManager_FindPOIForPed_448B90(poiManager_C0F5E4, Ped(owner), vc::At<float>(self, 4));
}

// 0x5FFE30 (thiscall): ConditionHavePOI: a POI or a dynamic one (+0x1D20)
uint8_t __fastcall HavePOI_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    if (vc::At<int32_t>(ped, 0x1D24) != 0) {
        return 1;
    }
    return vc::At<int32_t>(ped, 0x1D20) != 0;
}

// 0x5FFE60 (thiscall): ConditionHavePOIOfType: its POI of the kind +4
uint8_t __fastcall HavePOIOfType_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    return PoiKindIs(Ped(owner), vc::At<int32_t>(self, 4)) ? 1 : 0;
}

// 0x5FFE90 (thiscall): ConditionHaveDynamicPOI: the manager has one for the ped
uint8_t __fastcall HaveDynamicPOI_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return POIManager_FindByPed_448460(poiManager_C0F5E4, Ped(owner)) != nullptr;
}

// 0x5FFEB0 (thiscall): ConditionSensePlayer: the player noticed: alerted to
uint8_t __fastcall SensePlayer_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* player = player_C1AEA8;
    if (!Social_CanTalkTo_48D470(Ped(owner) + 0xA64, player)) {
        return 0;
    }
    Scanner_Alert_48C480(Ped(owner) + 0x3BC, player);
    return 1;
}

// 0x5FFEF0 (thiscall): ConditionSensePlayerDuration: the player noticed for +4 seconds
// (the start kept at +0x4D8, cleared when not)
uint8_t __fastcall SensePlayerDuration_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* player = player_C1AEA8;
    if (!Social_CanTalkTo_48D470(Ped(owner) + 0xA64, player)) {
        vc::At<uint32_t>(Ped(owner), 0x4D8) = 0;
        return 0;
    }
    uint8_t* ped = Ped(owner);
    uint32_t start = vc::At<uint32_t>(ped, 0x4D8);
    if (start == 0) {
        vc::At<uint32_t>(ped, 0x4D8) = timeMs_C1A9B4;
        return 0;
    }
    double now = X87Unsigned(timeMs_C1A9B4);
    float until = StampPlusSeconds(start, vc::At<float>(self, 4));
    if (!(until < now)) {
        return 0;
    }
    Scanner_Alert_48C480(ped + 0x3BC, player);
    return 1;
}

// 0x5FFFA0 (thiscall): ConditionIsPlayer
uint32_t __fastcall IsPlayer_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return vc::At<int32_t>(Ped(owner), 0x1310) == 0xD ? 1u : 0u;
}

// 0x5FFFC0 (thiscall): ConditionIsPatrolPed
uint32_t __fastcall IsPatrolPed_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return Ped(owner)[0x1570] == 4 ? 1u : 0u;
}

// 0x5FFFE0 (thiscall): ConditionNeedsToDive: something to dive from (+0x16D4 > 0 and
// +0x16D8), on the used entity's left side or not (+4)
uint8_t __fastcall NeedsToDive_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    if (int8_t(ped[0x16D4]) <= 0 || vc::At<int32_t>(ped, 0x16D8) == 0) {
        return 0;
    }
    if (Ped_IsOnUsedEntitysLeft_477930(ped) == 0) {
        return self[4];
    }
    return self[4] == 0;
}

// the hit's time since (+0x2C) set (not -1) and not in the future
bool HitTimeSet(float time) {
    if (noAttack_9009AC == time) {
        return false;
    }
    return 0.0f >= time;
}

// 0x600020 (thiscall): ConditionSenseAttacker: the one who hit the ped seen: alerted to
uint8_t __fastcall SenseAttacker_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* hit = vc::At<uint8_t*>(owner, 0x10);
    if (!HitTimeSet(vc::At<float>(hit, 0x2C))) {
        return 0;
    }
    uint8_t* attacker = vc::At<uint8_t*>(hit, 4);
    if (!attacker || !PedSight_CanSeeEntity_48D1F0(Ped(owner) + 0xA64, attacker)) {
        return 0;
    }
    Scanner_Alert_48C480(Ped(owner) + 0x3BC, vc::At<uint8_t*>(vc::At<uint8_t*>(owner, 0x10), 4));
    return 1;
}

// 0x600080 (thiscall): ConditionSenseTargetNow: the fight target (a ped) seen: alerted to
uint8_t __fastcall SenseTargetNow_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    if (!Ped(owner)) {
        return 0;
    }
    uint8_t* target = Ped_GetFightTarget_497100e(Ped(owner));
    if (!target || !IsPed(target)) {
        return 0;
    }
    if (!PedSight_CanSeeEntity_48D1F0(Ped(owner) + 0xA64, target)) {
        return 0;
    }
    Scanner_Alert_48C480(Ped(owner) + 0x3BC, target);
    return 1;
}

// 0x6000E0 (thiscall): ConditionFocusAttacked: the threat's last hit set and at most +4
uint8_t __fastcall FocusAttacked_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    if (!Scanner_GetThreat_48C070(Ped(owner) + 0x3BC)) {
        return 0;
    }
    uint8_t* threat = Scanner_GetThreat_48C070(Ped(owner) + 0x3BC);
    float limit = vc::At<float>(self, 4);
    uint8_t* action = vc::Call<uint8_t*>(threat, 0x40);
    float time = vc::At<float>(vc::At<uint8_t*>(action, 0x10), 0x2C);
    if (noAttack_9009AC == time) {
        return 0;
    }
    return limit >= time ? 1 : 0;
}

// 0x600150 (thiscall): ConditionStat: a stat (+0xC) against +4, or (+8) above a random
// percentage (+0x10: the owner's parameter +0x14 instead); +9: not while the AI is busy
uint32_t __fastcall Stat_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    if (self[9]) {
        uint8_t* controller = vc::At<uint8_t*>(Ped(owner), 0x2E0) + 0x290;
        if (uint8_t(Ctl_IsPlaying_5F4800e(controller, aiBusyName_1B65288, 1, 1))) {
            return 0;
        }
    }
    float value = float(Ped_GetStat_477060(Ped(owner), vc::At<int32_t>(self, 0xC)));
    if (!self[8]) {
        return value >= vc::At<float>(self, 4) ? 1u : 0u;
    }
    float chance;
    if (self[0x10]) {
        float parameter = vc::At<float>(owner, 0x28 + vc::At<int32_t>(self, 0x14) * 4);
        chance = float(double(parameter) * percent_900130);
    } else {
        int roll = Rand_85C6FA();
        float fraction = float(double(roll) / randMax_900CE8);
        chance = float(double(fraction) * percent_900130) + 0.0f;
    }
    return value > chance ? 1u : 0u;
}

// 0x600220 (thiscall): ConditionAttackTimerPlayTime: the time now against the last
// attack (+0x550) plus +4 seconds (not in the first 2 s); +8: the scanner's flag cleared
uint8_t __fastcall AttackTimerPlayTime_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint32_t last = vc::At<uint32_t>(Ped(owner), 0x550);
    uint32_t now = timeMs_C1A9B4;
    if (last == 0 && now <= 2000) {
        return 0;
    }
    float until = StampPlusSeconds(last, vc::At<float>(self, 4));
    uint8_t result = uint8_t(CompareFloatsE(float(double(now)), until, vc::At<int32_t>(self, 0xC)));
    if (result && self[8]) {
        Scanner_SetFlag_48C190(Ped(owner) + 0x3BC, 0);
    }
    return result;
}

// the block / evade timers: the time now against the stamp plus +4 seconds; +8 restamps
template <int Offset>
uint32_t __fastcall StampTimer_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    float until = StampPlusSeconds(vc::At<uint32_t>(Ped(owner), Offset), vc::At<float>(self, 4));
    uint32_t result = CompareFloatsE(float(double(timeMs_C1A9B4)), until, vc::At<int32_t>(self, 0xC));
    if (uint8_t(result) && self[8]) {
        vc::At<uint32_t>(Ped(owner), Offset) = timeMs_C1A9B4;
    }
    return result;
}

// 0x6003C0 (thiscall): ConditionTargetsPunishmentPoints: the fight target's points
uint32_t __fastcall TargetsPunishmentPoints_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* target = Ped_GetFightTarget_497100e(Ped(owner));
    if (!target || !IsPed(target)) {
        return 0;
    }
    float points = float(vc::At<int32_t>(target, 0x1D40));
    return CompareFloatsE(points, vc::At<float>(self, 4), vc::At<int32_t>(self, 8));
}

// 0x600420 (thiscall): ConditionObjectiveOfType: an objective of the type +4 queued
uint32_t __fastcall ObjectiveOfType_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    return ObjectiveQueue_FindOfType_4701D0(Ped(owner) + 0x5F0, vc::At<int32_t>(self, 4)) != nullptr ? 1u : 0u;
}

// 0x600440 (thiscall): ConditionTargetIsVisible: the fight target (a ped; else the
// current threat slot) around and seen
uint32_t __fastcall TargetIsVisible_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    uint8_t* target = Ped_GetFightTarget_497100e(ped);
    if (target && !IsPed(target)) {
        target = nullptr;
    }
    if (!target) {
        target = vc::At<uint8_t*>(ped, 0x4F0 + vc::At<int32_t>(ped, 0x504) * 4);
        if (!target) {
            return 0;
        }
    }
    return Scanner_IsAroundAndSeen_48C140(ped + 0x3BC, target);
}

// 0x6004A0 (thiscall): ConditionStimulus: the ped's stimulus of the kind +4, live, and
// older than +8 seconds plus a per-ped offset (its id modulo +0xC seconds)
uint8_t __fastcall Stimulus_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    uint8_t* stimulus = vc::At<uint8_t*>(ped, 0x1D30);
    if (!stimulus || vc::At<int32_t>(stimulus, 0) != vc::At<int32_t>(self, 4)) {
        return 0;
    }
    uint32_t now = timeMs_C1A9B4;
    if (stimulus[0x2A] || Stimulus_HasExpiredBy_4A5C50(stimulus, now)) {
        return 0;
    }
    float spreadMs = float(double(vc::At<float>(self, 0xC)) * msPerSecond_900138);
    int32_t spread = int32_t(uint32_t(int64_t(spreadMs)));   // (__ftol2: truncated, eax)
    int32_t offset = 0;
    if (spread > 0) {
        offset = int32_t(vc::At<uint16_t>(ped, 0x10C)) % spread;
    }
    double age = X87Unsigned(now - vc::At<uint32_t>(stimulus, 0x18));
    float minMs = float(double(vc::At<float>(self, 8)) * msPerSecond_900138);
    float threshold = float(double(minMs) + double(offset));
    return age >= threshold ? 1 : 0;
}

// 0x600560 (thiscall): ConditionHasNewAudibleStimulus: the strongest recent one of the
// kind +4: its position kept at +0x1D34
uint8_t __fastcall HasNewAudibleStimulus_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    if (!ped) {
        return 0;
    }
    uint8_t* stimulus = Stimuli_FindStrongestRecent_4A61A0(stimuli_C2C1A0, ped, vc::At<int32_t>(self, 4));
    if (!stimulus) {
        return 0;
    }
    vc::At<uint32_t>(ped, 0x1D34) = vc::At<uint32_t>(stimulus, 0xC);
    vc::At<uint32_t>(ped, 0x1D38) = vc::At<uint32_t>(stimulus, 0x10);
    vc::At<uint32_t>(ped, 0x1D3C) = vc::At<uint32_t>(stimulus, 0x14);
    return 1;
}

// 0x6005B0 (thiscall): ConditionStimulusRepeated: repeated at least +4 times
uint8_t __fastcall StimulusRepeated_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* stimulus = vc::At<uint8_t*>(Ped(owner), 0x1D30);
    if (!stimulus) {
        return 0;
    }
    return vc::At<int32_t>(stimulus, 0x24) >= vc::At<int32_t>(self, 4);
}

// 0x6005E0 (thiscall): ConditionTargetStimulusSubject: its offender made the target
uint8_t __fastcall TargetStimulusSubject_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    uint8_t* stimulus = vc::At<uint8_t*>(ped, 0x1D30);
    if (!stimulus) {
        return 0;
    }
    uint8_t* offender = Stimulus_GetOffender_4A5CB0(stimulus);
    if (!offender) {
        return 0;
    }
    float offset[3] = {0.0f, 0.0f, 0.0f};
    Ped_SetTarget_497E70(ped, offender, 3, offset);
    Scanner_Alert_48C480(ped + 0x3BC, offender);
    return 1;
}

// 0x600650 (thiscall): ConditionTargetStimulusObject: its source made the target entity
uint8_t __fastcall TargetStimulusObject_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    uint8_t* stimulus = vc::At<uint8_t*>(ped, 0x1D30);
    if (!stimulus) {
        return 0;
    }
    uint8_t* source = Stimulus_GetSource_4A5CD0(stimulus);
    if (!source) {
        return 0;
    }
    Ped_SetTargetEntity_498B10(ped, source);
    Scanner_Alert_48C480(ped + 0x3BC, source);
    return 1;
}

// 0x600690 (thiscall): ConditionIsStimulusObject: the ped is its source
uint8_t __fastcall IsStimulusObject_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    uint8_t* stimulus = vc::At<uint8_t*>(ped, 0x1D30);
    return stimulus && Stimulus_GetSource_4A5CD0(stimulus) == ped ? 1 : 0;
}

// 0x6006C0 (thiscall): ConditionIsInFactionWithStimulusObject
uint8_t __fastcall IsInFactionWithStimulusObject_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    uint8_t* stimulus = vc::At<uint8_t*>(ped, 0x1D30);
    if (!stimulus) {
        return 0;
    }
    uint8_t* source = Stimulus_GetSource_4A5CD0(stimulus);
    return source && vc::At<int32_t>(source, 0x1310) == vc::At<int32_t>(ped, 0x1310) ? 1 : 0;
}

// 0x600700 (thiscall): ConditionIgnoreAttacks
uint8_t __fastcall IgnoreAttacks_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return vc::At<int32_t>(Ped(owner), 0x1360) != 0;
}

} // namespace

#define CONDE_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

CONDE_REVERSED("ConditionMePlaying::Evaluate", 0x005FF8B0, &MePlaying_Evaluate);
CONDE_REVERSED("ConditionMePlayingDefaultNeutralState::Evaluate", 0x005FF8E0, &MePlayingDefaultNeutralState_Evaluate);
CONDE_REVERSED("ConditionAIIsPlaying::Evaluate", 0x005FF920, &AIIsPlaying_Evaluate);
CONDE_REVERSED("ConditionLiquidLevel::Evaluate", 0x005FF950, &LiquidLevel_Evaluate);
CONDE_REVERSED("ConditionLiquidType::Evaluate", 0x005FF9B0, &LiquidType_Evaluate);
CONDE_REVERSED("ConditionSurfaceHardness::Evaluate", 0x005FF9F0, &SurfaceHardness_Evaluate);
CONDE_REVERSED("ConditionInTriggerVolume::Evaluate", 0x005FFA30, &InTriggerVolume_Evaluate);
CONDE_REVERSED("ConditionPlayerInTriggerVolume::Evaluate", 0x005FFAA0, &PlayerInTriggerVolume_Evaluate);
CONDE_REVERSED("ConditionInTriggerVolumeOfType::Evaluate", 0x005FFB00, &InTriggerVolumeOfType_Evaluate);
CONDE_REVERSED("ConditionNearPoint::Evaluate", 0x005FFB70, &NearPoint_Evaluate);
CONDE_REVERSED("ConditionPedType::Evaluate", 0x005FFC20, &PedType_Evaluate);
CONDE_REVERSED("ConditionPedIsDriver::Evaluate", 0x005FFC40, &PedIsDriver_Evaluate);
CONDE_REVERSED("ConditionIsAuthority::Evaluate", 0x005FFC80, &IsAuthority_Evaluate);
CONDE_REVERSED("ConditionIsFemale::Evaluate", 0x005FFCB0, &IsFemale_Evaluate);
CONDE_REVERSED("ConditionIsMissionSpecific::Evaluate", 0x005FFCD0, &IsMissionSpecific_Evaluate);
CONDE_REVERSED("ConditionIsHangingOut::Evaluate", 0x005FFCF0, &IsHangingOut_Evaluate);
CONDE_REVERSED("ConditionIsSpectator::Evaluate", 0x005FFD20, &IsSpectator_Evaluate);
CONDE_REVERSED("ConditionIsScenarioPed::Evaluate", 0x005FFD50, &IsScenarioPed_Evaluate);
CONDE_REVERSED("ConditionHasCompletedScenarioObj::Evaluate", 0x005FFD60, &HasCompletedScenarioObj_Evaluate);
CONDE_REVERSED("ConditionIsScriptedAmbient::Evaluate", 0x005FFDA0, &IsScriptedAmbient_Evaluate);
CONDE_REVERSED("ConditionIsTargetAttackRightsTaken::Evaluate", 0x005FFDD0, &IsTargetAttackRightsTaken_Evaluate);
CONDE_REVERSED("ConditionAcquirePOI::Evaluate", 0x005FFE10, &AcquirePOI_Evaluate);
CONDE_REVERSED("ConditionHavePOI::Evaluate", 0x005FFE30, &HavePOI_Evaluate);
CONDE_REVERSED("ConditionHavePOIOfType::Evaluate", 0x005FFE60, &HavePOIOfType_Evaluate);
CONDE_REVERSED("ConditionHaveDynamicPOI::Evaluate", 0x005FFE90, &HaveDynamicPOI_Evaluate);
CONDE_REVERSED("ConditionSensePlayer::Evaluate", 0x005FFEB0, &SensePlayer_Evaluate);
CONDE_REVERSED("ConditionSensePlayerDuration::Evaluate", 0x005FFEF0, &SensePlayerDuration_Evaluate);
CONDE_REVERSED("ConditionIsPlayer::Evaluate", 0x005FFFA0, &IsPlayer_Evaluate);
CONDE_REVERSED("ConditionIsPatrolPed::Evaluate", 0x005FFFC0, &IsPatrolPed_Evaluate);
CONDE_REVERSED("ConditionNeedsToDive::Evaluate", 0x005FFFE0, &NeedsToDive_Evaluate);
CONDE_REVERSED("ConditionSenseAttacker::Evaluate", 0x00600020, &SenseAttacker_Evaluate);
CONDE_REVERSED("ConditionSenseTargetNow::Evaluate", 0x00600080, &SenseTargetNow_Evaluate);
CONDE_REVERSED("ConditionFocusAttacked::Evaluate", 0x006000E0, &FocusAttacked_Evaluate);
CONDE_REVERSED("ConditionStat::Evaluate", 0x00600150, &Stat_Evaluate);
CONDE_REVERSED("ConditionAttackTimerPlayTime::Evaluate", 0x00600220, &AttackTimerPlayTime_Evaluate);
CONDE_REVERSED("ConditionBlockTimerPlayTime::Evaluate", 0x006002C0, &StampTimer_Evaluate<0x4D0>);
CONDE_REVERSED("ConditionEvadeTimerPlayTime::Evaluate", 0x00600340, &StampTimer_Evaluate<0x4D4>);
CONDE_REVERSED("ConditionTargetsPunishmentPoints::Evaluate", 0x006003C0, &TargetsPunishmentPoints_Evaluate);
CONDE_REVERSED("ConditionObjectiveOfType::Evaluate", 0x00600420, &ObjectiveOfType_Evaluate);
CONDE_REVERSED("ConditionTargetIsVisible::Evaluate", 0x00600440, &TargetIsVisible_Evaluate);
CONDE_REVERSED("ConditionStimulus::Evaluate", 0x006004A0, &Stimulus_Evaluate);
CONDE_REVERSED("ConditionHasNewAudibleStimulus::Evaluate", 0x00600560, &HasNewAudibleStimulus_Evaluate);
CONDE_REVERSED("ConditionStimulusRepeated::Evaluate", 0x006005B0, &StimulusRepeated_Evaluate);
CONDE_REVERSED("ConditionTargetStimulusSubject::Evaluate", 0x006005E0, &TargetStimulusSubject_Evaluate);
CONDE_REVERSED("ConditionTargetStimulusObject::Evaluate", 0x00600650, &TargetStimulusObject_Evaluate);
CONDE_REVERSED("ConditionIsStimulusObject::Evaluate", 0x00600690, &IsStimulusObject_Evaluate);
CONDE_REVERSED("ConditionIsInFactionWithStimulusObject::Evaluate", 0x006006C0, &IsInFactionWithStimulusObject_Evaluate);
CONDE_REVERSED("ConditionIgnoreAttacks::Evaluate", 0x00600700, &IgnoreAttacks_Evaluate);
