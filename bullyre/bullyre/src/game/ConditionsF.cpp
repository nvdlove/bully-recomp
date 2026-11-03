// The action tree's conditions (continued): the ped's vehicle and bike race, its path,
// which pad controls it, its group, ranges to its target / focus / vehicle, the bike's
// pedalling and sprint, random rolls and the scan of the ped pool for someone of an
// attitude to attack. 0x600720..0x6010B0.
//
// Arithmetic is float in the game's order (its x87 runs in single precision, D3D9's
// default).

#include <cmath>
#include <cstdint>

#include "game/ActionTree.h"

#include "game/Crt.h"
#include "reversed.h"
using namespace acttree;

namespace {

GAME_VAR(uint8_t*, bikeRace_20C8704, 0x020C8704);
GAME_VAR(uint8_t, twoPads_A45A4C, 0x00A45A4C);
GAME_VAR(uint8_t*, padPed1_C2B124, 0x00C2B124);
GAME_VAR(uint8_t*, padPed0_C2B120, 0x00C2B120);
GAME_VAR(uint8_t*, player_C1AEA8f, 0x00C1AEA8);
GAME_VAR(int32_t, leftAloneModel_BF42DC, 0x00BF42DC);
GAME_VAR(double, randMax_900CE8f, 0x00900CE8);   // 32767
GAME_ARRAY(uint32_t, aiBusyName_1B65288f, 0x01B65288);

uint8_t* const kPedPoolSlot = reinterpret_cast<uint8_t*>(0x00C0F5F0);
uint8_t* const kSchedules = reinterpret_cast<uint8_t*>(0x00C2AB70);

GAME_FN(uint32_t, __cdecl, CompareFloatsF, 0x005FB8B0, float a, float b, int op);
GAME_FN(uint32_t, __cdecl, CompareIntsF, 0x005FBCB0, int32_t a, int32_t b, int op);
GAME_FN(uint32_t, __thiscall, Ctl_IsPlaying_5F4800f, 0x005F4800, uint8_t* controller, const uint32_t* name, uint32_t ancestors,
        int nested);
GAME_FN(uint8_t, __thiscall, Ped_IsSeatedInVehicle_4CE690, 0x004CE690, uint8_t* ped);
GAME_FN(int32_t*, __thiscall, Vehicle_GetVehicleClassF, 0x004CAF30, uint8_t* vehicle, int32_t* out);
GAME_FN(uint8_t, __thiscall, BikeRace_HasRacer_70D3E0, 0x0070D3E0, uint8_t* race, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, ObjectiveQueue_GetCurrent_470340, 0x00470340, uint8_t* queue);
GAME_FN(uint32_t, __thiscall, PathNodeRef_IsValid_46FD20, 0x0046FD20, uint8_t* ref);
GAME_FN(int32_t, __thiscall, PedGroup_CountMembers_489700, 0x00489700, uint8_t* group);
GAME_FN(uint8_t*, __thiscall, Ped_GetFightTarget_497100f, 0x00497100, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, Targeting_GetLookTarget_4D7780f, 0x004D7780, uint8_t* targeting);
GAME_FN(uint8_t*, __thiscall, Scanner_Get184_48C060, 0x0048C060, uint8_t* scanner);
GAME_FN(uint8_t*, __thiscall, Scanner_GetThreat_48C070f, 0x0048C070, uint8_t* scanner);
GAME_FN(float*, __cdecl, VectorSubF, 0x00414B70, float* out, const float* a, const float* b);
GAME_FN(int32_t, __thiscall, Bike_GetTimeSince724_4BCCA0, 0x004BCCA0, uint8_t* bike);
inline int __cdecl Rand_85C6FAf() { return crt::Rand(); }
GAME_FN(uint8_t, __thiscall, PedCombat_HoldsUnflaggedSlot_4868B0, 0x004868B0, uint8_t* combat, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Ped_IsScenarioPed_473220f, 0x00473220, uint8_t* ped);
GAME_FN(uint8_t, __cdecl, Nis_IsSet_5BB380, 0x005BB380);
GAME_FN(uint8_t, __thiscall, Ped_IsAmbient_4765C0, 0x004765C0, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Ally_HasFollower_485330, 0x00485330, uint8_t* ally);
GAME_FN(uint8_t, __thiscall, Ally_FollowsPlayer_485530, 0x00485530, uint8_t* ally);
GAME_FN(int32_t*, __thiscall, Ped_GetAttitude_475380f, 0x00475380, uint8_t* ped, int32_t* out, int32_t pedType);
GAME_FN(uint8_t, __thiscall, Social_CanTalkTo_48D470f, 0x0048D470, uint8_t* social, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, PedCombat_HasRemembered_486300f, 0x00486300, uint8_t* combat);
GAME_FN(uint8_t, __thiscall, Ped_CanDrive_41AA50f, 0x0041AA50, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Schedules_Fn46EFB0, 0x0046EFB0, uint8_t* self, uint8_t* ped, const float* pos, int);
GAME_FN(void, __thiscall, Ped_SetTarget_497E70f, 0x00497E70, uint8_t* ped, uint8_t* target, int kind, const float* offset);
GAME_FN(void, __thiscall, Scanner_Alert_48C480f, 0x0048C480, uint8_t* scanner, uint8_t* ped);

uint8_t* Ped(uint8_t* owner) {
    return vc::At<uint8_t*>(owner, 8);
}

// sqrt (the C runtime's)
float CrtSqrtF(float value) {
    return crt::Sqrtf(value);
}

const float* EntityPos(uint8_t* entity) {
    uint8_t* matrix = vc::At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<float*>(matrix + 0x30) : reinterpret_cast<float*>(entity + 4);
}

bool IsPed(uint8_t* entity) {
    return (entity[0x108] & 7) == 3;
}

// the 2D distance from b to a compared (+4 against, +8 the op)
uint32_t CompareRange2D(uint8_t* self, const float* a, const float* b) {
    float d[3];
    float* delta = VectorSubF(d, a, b);
    float lengthSq = std::fabs(delta[0] * delta[0] + delta[1] * delta[1]);
    float length = CrtSqrtF(lengthSq);
    return CompareFloatsF(length, vc::At<float>(self, 4), vc::At<int32_t>(self, 8));
}

// 0x600720 (thiscall): ConditionInCar: seated in a car
uint32_t __fastcall InCar_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    if (!Ped_IsSeatedInVehicle_4CE690(Ped(owner))) {
        return 0;
    }
    return vc::At<int32_t>(vc::At<uint8_t*>(Ped(owner), 0x1554), 0x46C) == 0 ? 1u : 0u;
}

// 0x600760 (thiscall): ConditionOnBike: seated on a bike
uint32_t __fastcall OnBike_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    if (!Ped_IsSeatedInVehicle_4CE690(Ped(owner))) {
        return 0;
    }
    return vc::At<int32_t>(vc::At<uint8_t*>(Ped(owner), 0x1554), 0x46C) == 1 ? 1u : 0u;
}

// 0x600790 (thiscall): ConditionOnVehicleOfClass: the vehicle it is on of the class +4
uint8_t __fastcall OnVehicleOfClass_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* vehicle = vc::At<uint8_t*>(Ped(owner), 0x1554);
    if (!vehicle || (vehicle[0x108] & 7) != 2) {
        return 0;
    }
    int32_t kind;
    return *Vehicle_GetVehicleClassF(vehicle, &kind) == vc::At<int32_t>(self, 4);
}

// 0x6007D0 (thiscall): ConditionInBikeRace: a race of kind 0xE running with the ped in it
uint8_t __fastcall InBikeRace_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* race = bikeRace_20C8704;
    if (!race || !race[4] || vc::At<int32_t>(race, 0x3C) != 0xE) {
        return 0;
    }
    return BikeRace_HasRacer_70D3E0(race, Ped(owner)) ? 1 : 0;
}

// 0x600810 (thiscall): ConditionHavePath: the current objective (kind 0xC / 0xD) has a
// valid path node
uint32_t __fastcall HavePath_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* objective = ObjectiveQueue_GetCurrent_470340(Ped(owner) + 0x5F0);
    if (!objective) {
        return 0;
    }
    int32_t kind = vc::At<int32_t>(objective, 8);
    if (kind != 0xC && kind != 0xD) {
        return 0;
    }
    return PathNodeRef_IsValid_46FD20(objective + 0x10);
}

// 0x600850 (thiscall): ConditionController: in two-pad play, the ped is pad +4's (0 or
// 1) or (+5) either's
uint32_t __fastcall Controller_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    if (!twoPads_A45A4C) {
        return 0;
    }
    uint8_t* ped = Ped(owner);
    if (self[5]) {
        return padPed1_C2B124 == ped || padPed0_C2B120 == ped ? 1u : 0u;
    }
    uint8_t* padPed = self[4] ? padPed1_C2B124 : padPed0_C2B120;
    return padPed == ped ? 1u : 0u;
}

// 0x6008B0 (thiscall): ConditionSizeOfGroup: its group's members counted (0 without)
uint32_t __fastcall SizeOfGroup_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* group = vc::At<uint8_t*>(Ped(owner), 0x1D20);
    int32_t count = group ? PedGroup_CountMembers_489700(group) : 0;
    return CompareIntsF(count, vc::At<int32_t>(self, 4), vc::At<int32_t>(self, 8));
}

// 0x6008E0 (thiscall): ConditionRangeToTarget: the 2D range to its +0x434 (else fight
// target, looked-at one, scanner's +0x184)
uint32_t __fastcall RangeToTarget_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    uint8_t* target = vc::At<uint8_t*>(ped, 0x434);
    if (!target) {
        target = Ped_GetFightTarget_497100f(ped);
        if (!target) {
            target = Targeting_GetLookTarget_4D7780f(vc::At<uint8_t*>(ped, 0x1C8));
            if (!target) {
                target = Scanner_Get184_48C060(ped + 0x3BC);
                if (!target) {
                    return 0;
                }
            }
        }
    }
    return CompareRange2D(self, EntityPos(target), EntityPos(ped));
}

// 0x6009B0 (thiscall): ConditionTimeSincePedalling: the bike's time since compared (a
// byte bool: the callers test al; its xor al, al path leaves eax's upper bytes)
uint8_t __fastcall TimeSincePedalling_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    if (!ped || !vc::At<uint8_t*>(ped, 0x1554)) {
        return 0;
    }
    int32_t time = Bike_GetTimeSince724_4BCCA0(vc::At<uint8_t*>(ped, 0x1554));
    return uint8_t(CompareIntsF(time, vc::At<int32_t>(self, 4), vc::At<int32_t>(self, 8)));
}

// 0x6009F0 (thiscall): ConditionBikeSprintAmount: the bike's sprint (+0x778) compared
uint32_t __fastcall BikeSprintAmount_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    if (!ped || !vc::At<uint8_t*>(ped, 0x1554)) {
        return 0;
    }
    float sprint = vc::At<float>(vc::At<uint8_t*>(ped, 0x1554), 0x778);
    return CompareFloatsF(sprint, vc::At<float>(self, 4), vc::At<int32_t>(self, 8));
}

// 0x600A40 (thiscall): ConditionRangeToFocus: the 2D range to the scanner's threat
uint32_t __fastcall RangeToFocus_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    if (!Scanner_GetThreat_48C070f(Ped(owner) + 0x3BC)) {
        return 0;
    }
    uint8_t* ped = Ped(owner);
    const float* pedPos = EntityPos(ped);
    uint8_t* threat = Scanner_GetThreat_48C070f(ped + 0x3BC);
    return CompareRange2D(self, EntityPos(threat), pedPos);
}

// 0x600B00 (thiscall): ConditionRangeToObjectiveVehicle: the 2D range to its vehicle (a
// byte bool, as above)
uint8_t __fastcall RangeToObjectiveVehicle_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    if (!vc::At<uint8_t*>(ped, 0x430)) {
        return 0;
    }
    const float* pedPos = EntityPos(ped);
    return uint8_t(CompareRange2D(self, EntityPos(vc::At<uint8_t*>(ped, 0x430)), pedPos));
}

// the AI's controller busy (the node the game names at 0x1B65288)
bool AiBusy(uint8_t* owner) {
    uint8_t* controller = vc::At<uint8_t*>(Ped(owner), 0x2E0) + 0x290;
    return uint8_t(Ctl_IsPlaying_5F4800f(controller, aiBusyName_1B65288f, 1, 1)) != 0;
}

// a roll in [0, 1]: rand / 32767, times (1 - 0), plus 0
float RollUnit() {
    int roll = Rand_85C6FAf();
    float fraction = float(double(roll) / randMax_900CE8f);
    return fraction * 1.0f + 0.0f;
}

// 0x600BB0 (thiscall): ConditionRandom: +0xD rolls the owner's parameter +0x10; that
// (+0xE) or the ped's +0x1748 compared (+4, op +8); +0xC: not while the AI is busy
uint32_t __fastcall Random_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    if (self[0xC] && AiBusy(owner)) {
        return 0;
    }
    if (self[0xD]) {
        vc::At<float>(owner, 0x28 + vc::At<int32_t>(self, 0x10) * 4) = RollUnit();
    }
    float value;
    if (self[0xE]) {
        value = vc::At<float>(owner, 0x28 + vc::At<int32_t>(self, 0x10) * 4);
    } else {
        value = vc::At<float>(Ped(owner), 0x1748);
    }
    return CompareFloatsF(value, vc::At<float>(self, 4), vc::At<int32_t>(self, 8));
}

// 0x600C60 (thiscall): ConditionWeightedRandom: the owner's roll (+0x24) at most +8,
// then rerolled; +0xD: only while the fight target holds an unflagged slot for the ped
uint8_t __fastcall WeightedRandom_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    if (self[0xC] && AiBusy(owner)) {
        return 0;
    }
    if (self[0xD]) {
        uint8_t* target = Ped_GetFightTarget_497100f(Ped(owner));
        if (target && IsPed(target) && !PedCombat_HoldsUnflaggedSlot_4868B0(target + 0x668, Ped(owner))) {
            return 0;
        }
    }
    if (!(vc::At<float>(self, 8) >= vc::At<float>(owner, 0x24))) {
        return 0;
    }
    vc::At<float>(owner, 0x24) = RollUnit();
    return 1;
}

// the pool's filters shared by both passes of the faction scan
bool ScanCandidate(uint8_t* me, uint8_t* other) {
    if (other == me) {
        return false;
    }
    if (int32_t(vc::At<int16_t>(other, 0x10E)) == leftAloneModel_BF42DC) {
        return false;
    }
    if (vc::At<int32_t>(other, 0xC0) != 0) {
        return false;
    }
    return true;
}

// the authority / cutscene filters (true: skip)
bool ScanExcluded(uint8_t* me, uint8_t* other) {
    int32_t faction = vc::At<int32_t>(me, 0x1310);
    if (faction == 0 || faction == 8 || faction == 7) {
        if (vc::At<int32_t>(other, 0x14C4) != 0 || vc::At<int32_t>(other, 0x14E8) == 0) {
            return true;
        }
        if (Ped_IsScenarioPed_473220f(other)) {
            return true;
        }
    }
    if (Nis_IsSet_5BB380() && Ped_IsAmbient_4765C0(me) && other[0x1570] == 2) {
        return true;
    }
    return false;
}

// the one found made the target (kind 1) and alerted to
uint8_t ScanFound(uint8_t* me, uint8_t* other) {
    float offset[3] = {0.0f, 0.0f, 0.0f};
    Ped_SetTarget_497E70f(me, other, 1, offset);
    Scanner_Alert_48C480f(me + 0x3BC, other);
    return 1;
}

// 0x600D10 (thiscall): ConditionFactionScan: a ped in the pool (from the last slot) of
// an attitude (+4, op +8) towards the ped: first with every filter (+0xC talkable,
// +0xD the player, +0xE a follower of the player, +0xF not remembered), then again
// with only the basic ones
uint8_t __fastcall FactionScan_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* pool = *reinterpret_cast<re::Ptr32<uint8_t>*>(kPedPoolSlot);
    for (int32_t i = vc::At<int32_t>(pool, 8); i != 0;) {
        i--;
        if (vc::At<uint8_t*>(pool, 4)[i] & 0x80) {
            continue;
        }
        uint8_t* other = vc::At<uint8_t*>(pool, 0) + vc::At<int32_t>(pool, 0xC) * i;
        if (!other) {
            continue;
        }
        uint8_t* me = Ped(owner);
        if (!ScanCandidate(me, other)) {
            continue;
        }
        if (vc::At<int32_t>(other, 0x1310) == 0xD && player_C1AEA8f[0x1E91]) {
            continue;
        }
        if (other[0x1C98]) {
            continue;
        }
        if (ScanExcluded(me, other)) {
            continue;
        }
        uint8_t* ally = other + 0x2F0;
        int32_t attitude[2];
        int32_t* got;
        if (Ally_HasFollower_485330(ally) && vc::At<uint8_t*>(other, 0x2F8)) {
            got = Ped_GetAttitude_475380f(Ped(owner), &attitude[0],
                                          vc::At<int32_t>(vc::At<uint8_t*>(other, 0x2F8), 0x1310));
        } else {
            got = Ped_GetAttitude_475380f(Ped(owner), &attitude[1], vc::At<int32_t>(other, 0x1310));
        }
        if (!uint8_t(CompareIntsF(*got, vc::At<int32_t>(self, 4), vc::At<int32_t>(self, 8)))) {
            continue;
        }
        if (self[0xC] && !Social_CanTalkTo_48D470f(Ped(owner) + 0xA64, other)) {
            continue;
        }
        bool pass;
        if (self[0xD] && vc::At<int32_t>(other, 0x1310) == 0xD) {
            pass = true;
        } else if (self[0xE]) {
            pass = Ally_FollowsPlayer_485530(ally) != 0;
        } else {
            pass = !self[0xD];
        }
        if (!pass) {
            continue;
        }
        if (self[0xF] && PedCombat_HasRemembered_486300f(other + 0x668)) {
            continue;
        }
        uint8_t* ped = Ped(owner);
        if (Ped_CanDrive_41AA50f(ped)) {
            continue;
        }
        if (Schedules_Fn46EFB0(kSchedules, ped, EntityPos(other), 0)) {
            continue;
        }
        if (!self[0xC] || Social_CanTalkTo_48D470f(Ped(owner) + 0xA64, other)) {
            return ScanFound(Ped(owner), other);
        }
    }
    for (int32_t i = vc::At<int32_t>(pool, 8); i != 0;) {
        i--;
        if (vc::At<uint8_t*>(pool, 4)[i] & 0x80) {
            continue;
        }
        uint8_t* other = vc::At<uint8_t*>(pool, 0) + vc::At<int32_t>(pool, 0xC) * i;
        if (!other) {
            continue;
        }
        uint8_t* me = Ped(owner);
        if (!ScanCandidate(me, other) || other[0x1C98]) {
            continue;
        }
        if (ScanExcluded(me, other)) {
            continue;
        }
        int32_t attitude;
        int32_t* got = Ped_GetAttitude_475380f(Ped(owner), &attitude, vc::At<int32_t>(other, 0x1310));
        if (!uint8_t(CompareIntsF(*got, vc::At<int32_t>(self, 4), vc::At<int32_t>(self, 8)))) {
            continue;
        }
        if (Schedules_Fn46EFB0(kSchedules, Ped(owner), EntityPos(other), 0)) {
            continue;
        }
        if (!self[0xC] || Social_CanTalkTo_48D470f(Ped(owner) + 0xA64, other)) {
            return ScanFound(Ped(owner), other);
        }
    }
    return 0;
}

} // namespace

#define CONDF_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

CONDF_REVERSED("ConditionInCar::Evaluate", 0x00600720, &InCar_Evaluate);
CONDF_REVERSED("ConditionOnBike::Evaluate", 0x00600760, &OnBike_Evaluate);
CONDF_REVERSED("ConditionOnVehicleOfClass::Evaluate", 0x00600790, &OnVehicleOfClass_Evaluate);
CONDF_REVERSED("ConditionInBikeRace::Evaluate", 0x006007D0, &InBikeRace_Evaluate);
CONDF_REVERSED("ConditionHavePath::Evaluate", 0x00600810, &HavePath_Evaluate);
CONDF_REVERSED("ConditionController::Evaluate", 0x00600850, &Controller_Evaluate);
CONDF_REVERSED("ConditionSizeOfGroup::Evaluate", 0x006008B0, &SizeOfGroup_Evaluate);
CONDF_REVERSED("ConditionRangeToTarget::Evaluate", 0x006008E0, &RangeToTarget_Evaluate);
CONDF_REVERSED("ConditionTimeSincePedalling::Evaluate", 0x006009B0, &TimeSincePedalling_Evaluate);
CONDF_REVERSED("ConditionBikeSprintAmount::Evaluate", 0x006009F0, &BikeSprintAmount_Evaluate);
CONDF_REVERSED("ConditionRangeToFocus::Evaluate", 0x00600A40, &RangeToFocus_Evaluate);
CONDF_REVERSED("ConditionRangeToObjectiveVehicle::Evaluate", 0x00600B00, &RangeToObjectiveVehicle_Evaluate);
CONDF_REVERSED("ConditionRandom::Evaluate", 0x00600BB0, &Random_Evaluate);
CONDF_REVERSED("ConditionWeightedRandom::Evaluate", 0x00600C60, &WeightedRandom_Evaluate);
CONDF_REVERSED("ConditionFactionScan::Evaluate", 0x00600D10, &FactionScan_Evaluate);
