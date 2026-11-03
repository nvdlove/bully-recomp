// The action tree's conditions (end of the chunk): whom to fight for (a master's or a
// friend's attacker, the player's girlfriend), docks to go to, the target's faction and
// clothing, character sizes, combat zones, acquiring a focus / target, humiliation, who
// targets whom, and the condition groups run on another's action context.
// 0x6010B0..0x601FA8.
//
// Arithmetic is float in the game's order (its x87 runs in single precision, D3D9's
// default).

#include <cmath>
#include <cstdint>
#include <cstring>

#include "game/ActionTree.h"

#include "game/Crt.h"
#include "reversed.h"
using namespace acttree;

namespace {

GAME_VAR(uint8_t*, player_C1AEA8g, 0x00C1AEA8);
GAME_VAR(uint8_t*, schedules_C2AB68, 0x00C2AB68);
GAME_VAR(float, fleeDockRange_9009B8, 0x009009B8);     // FLT_MAX
GAME_VAR(uint8_t*, combatZones_20C889C, 0x020C889C);
GAME_VAR(float, outerZone_900490, 0x00900490);         // 100
GAME_VAR(float, punishRange_90D0C8, 0x0090D0C8);       // 70
GAME_VAR(uint8_t*, punishment_C0F78Cg, 0x00C0F78C);
GAME_VAR(double, percent_900130g, 0x00900130);         // 100
GAME_VAR(float, attackerTimeLimit_900188, 0x00900188); // 5
GAME_VAR(int32_t, clothingFactionCount_20C78F4, 0x020C78F4);

uint8_t* const kPedPoolSlotG = reinterpret_cast<uint8_t*>(0x00C0F5F0);
uint8_t* const kClothingSlots = reinterpret_cast<uint8_t*>(0x020C5F79);      // 6 x 0xC: a byte each
uint8_t* const kClothingSlotsEnd = reinterpret_cast<uint8_t*>(0x020C5FC1);
int32_t* const kClothingAttitudes = reinterpret_cast<int32_t*>(0x020C78FC);  // [faction][11]
int32_t* const kFactionAttitudes = reinterpret_cast<int32_t*>(0x020C7924);   // [faction] x 0x2C bytes

GAME_FN(uint32_t, __cdecl, CompareIntsG, 0x005FBCB0, int32_t a, int32_t b, int op);
GAME_FN(uint32_t, __thiscall, ConditionList_EvaluateG, 0x005FBB10, uint8_t* list, uint8_t* owner);
GAME_FN(uint8_t*, __thiscall, Stimulus_GetSource_4A5CD0g, 0x004A5CD0, uint8_t* stimulus);
GAME_FN(uint8_t*, __thiscall, Stimulus_GetOffender_4A5CB0g, 0x004A5CB0, uint8_t* stimulus);
GAME_FN(int32_t*, __thiscall, Ped_GetAttitude_475380g, 0x00475380, uint8_t* ped, int32_t* out, int32_t pedType);
GAME_FN(void, __thiscall, Ped_SetTargetEntity_498B10g, 0x00498B10, uint8_t* ped, uint8_t* target);
GAME_FN(void, __thiscall, Scanner_Alert_48C480g, 0x0048C480, uint8_t* scanner, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Ped_IsRegisteredAsGirlfriend_473F10, 0x00473F10, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Scanner_IsAroundAndSeen_48C140g, 0x0048C140, uint8_t* scanner, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Ped_Fn496F30, 0x00496F30, uint8_t* ped, uint8_t* other);
GAME_FN(uint8_t*, __thiscall, ObjectiveQueue_GetAttackObjective_485900, 0x00485900, uint8_t* queue);
GAME_FN(uint8_t*, __thiscall, PedScheduleManager_Pick_46EA10, 0x0046EA10, uint8_t* manager, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, PedScheduleManager_FindNearest_46EA90, 0x0046EA90, uint8_t* manager, uint8_t* ped,
        float* distance);
inline int __cdecl Rand_85C6FAg() { return crt::Rand(); }
GAME_FN(uint8_t*, __thiscall, Scanner_Get184_48C060g, 0x0048C060, uint8_t* scanner);
GAME_FN(uint8_t*, __thiscall, Scanner_GetThreat_48C070g, 0x0048C070, uint8_t* scanner);
GAME_FN(uint8_t*, __thiscall, Ped_GetFightTarget_497100g, 0x00497100, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, Ped_GetModelInfo_477DC0g, 0x00477DC0, uint8_t* ped);
GAME_FN(int32_t*, __thiscall, PedModelInfo_GetType_522380, 0x00522380, uint8_t* info, int32_t* out);
GAME_FN(uint8_t*, __thiscall, Ped_FindByType_477C00g, 0x00477C00, uint8_t* ped, int32_t type, int32_t* extra);
GAME_FN(uint8_t, __thiscall, PedCombat_JoinTarget_487BC0, 0x00487BC0, uint8_t* combat, uint8_t* target, int list,
        int dontCommit);
GAME_FN(float*, __cdecl, VectorSubG, 0x00414B70, float* out, const float* a, const float* b);
GAME_FN(uint8_t*, __thiscall, ObjectiveQueue_GetCurrent_470340g, 0x00470340, uint8_t* queue);
GAME_FN(uint8_t*, __thiscall, Objective_GetTarget_8B6120, 0x008B6120, uint8_t* objective);
GAME_FN(void, __thiscall, Scanner_SetFocus_48C4C0, 0x0048C4C0, uint8_t* scanner, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, Ped_ChooseTargetIfAsked_499470, 0x00499470, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Ped_CanAttackStanding_497F90, 0x00497F90, uint8_t* ped, uint8_t* other, int, int);
GAME_FN(float, __cdecl, Distance2D_470110, 0x00470110, const float* a, const float* b);
GAME_FN(void, __thiscall, Targeting_SetTarget_4D79E0, 0x004D79E0, uint8_t* targeting, uint8_t* target);
GAME_FN(int32_t, __thiscall, PedGroup_CountMembers_489700g, 0x00489700, uint8_t* group);
GAME_FN(int32_t, __thiscall, Ped_GetStat_477060g, 0x00477060, uint8_t* ped, int32_t stat);
GAME_FN(uint8_t*, __thiscall, Ped_Fn497020, 0x00497020, uint8_t* ped);
GAME_FN(float, __thiscall, Ped_GetAttackerTime_496990, 0x00496990, uint8_t* ped);
GAME_FN(int32_t*, __thiscall, Social_GetMood_48EBC0, 0x0048EBC0, uint8_t* social, int32_t* out, uint8_t* ped, int, uintptr_t, uintptr_t);
GAME_FN(uint8_t*, __thiscall, Targeting_GetTarget_4D5AD0g, 0x004D5AD0, uint8_t* targeting);
GAME_FN(uint8_t*, __thiscall, Targeting_GetLookTarget_4D7780g, 0x004D7780, uint8_t* targeting);
GAME_FN(uint8_t*, __thiscall, Ped_GetLastHitWeapon_474BE0, 0x00474BE0, uint8_t* ped);

uint8_t* Ped(uint8_t* owner) {
    return vc::At<uint8_t*>(owner, 8);
}

// sqrt (the C runtime's)
float CrtSqrtG(float value) {
    return crt::Sqrtf(value);
}

const float* EntityPos(uint8_t* entity) {
    uint8_t* matrix = vc::At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<float*>(matrix + 0x30) : reinterpret_cast<float*>(entity + 4);
}

bool IsPed(uint8_t* entity) {
    return (entity[0x108] & 7) == 3;
}

// the fight target if a ped, else null
uint8_t* FightTargetPed(uint8_t* ped) {
    uint8_t* target = Ped_GetFightTarget_497100g(ped);
    return target && IsPed(target) ? target : nullptr;
}

int32_t AttitudeTo(uint8_t* ped, uint8_t* other) {
    int32_t out;
    return *Ped_GetAttitude_475380g(ped, &out, vc::At<int32_t>(other, 0x1310));
}

// the one attacked back: made the target entity and alerted to
uint8_t TakeOn(uint8_t* ped, uint8_t* other) {
    Ped_SetTargetEntity_498B10g(ped, other);
    Scanner_Alert_48C480g(ped + 0x3BC, other);
    return 1;
}

// 0x6010B0 (thiscall): ConditionTargetMastersAttacker: of a stimulus 0x3C / 4 / 5, its
// offender unless hated (attitude 4), else its source unless hated
uint8_t __fastcall TargetMastersAttacker_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    uint8_t* stimulus = vc::At<uint8_t*>(ped, 0x1D30);
    if (!stimulus) {
        return 0;
    }
    int32_t kind = vc::At<int32_t>(stimulus, 0);
    if (kind != 0x3C && kind != 4 && kind != 5) {
        return 0;
    }
    uint8_t* source = Stimulus_GetSource_4A5CD0g(stimulus);
    uint8_t* offender = Stimulus_GetOffender_4A5CB0g(stimulus);
    if (!source || !offender) {
        return 0;
    }
    if (AttitudeTo(ped, offender) != 4) {
        return TakeOn(ped, offender);
    }
    if (AttitudeTo(ped, source) == 4) {
        return 0;
    }
    return TakeOn(ped, source);
}

// 0x601170 (thiscall): ConditionTargetFriendsAttacker: of a stimulus 0x3C / 4 / 3 /
// 0x40 / 5 (5 with a hated offender: the two swapped), the victim a friend (attitude 4,
// or the player's girlfriend) seen around, the attacker not already dealt with, and the
// victim attacking back or the player (or a stimulus 0x40)
uint8_t __fastcall TargetFriendsAttacker_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    uint8_t* stimulus = vc::At<uint8_t*>(ped, 0x1D30);
    if (!stimulus) {
        return 0;
    }
    int32_t kind = vc::At<int32_t>(stimulus, 0);
    if (kind != 0x3C && kind != 4 && kind != 3 && kind != 0x40 && kind != 5) {
        return 0;
    }
    uint8_t* victim = Stimulus_GetSource_4A5CD0g(stimulus);
    uint8_t* attacker = Stimulus_GetOffender_4A5CB0g(stimulus);
    if (!victim || !attacker || victim == attacker) {
        return 0;
    }
    if (vc::At<int32_t>(stimulus, 0) == 5 && AttitudeTo(ped, attacker) == 4) {
        victim = Stimulus_GetOffender_4A5CB0g(stimulus);
        attacker = Stimulus_GetSource_4A5CD0g(stimulus);
    }
    bool girlfriend = vc::At<int32_t>(victim, 0x1310) == 0xD && Ped_IsRegisteredAsGirlfriend_473F10(ped);
    if (AttitudeTo(ped, victim) != 4 && !girlfriend) {
        return 0;
    }
    if (!Scanner_IsAroundAndSeen_48C140g(ped + 0x3BC, victim)) {
        return 0;
    }
    if (Ped_Fn496F30(ped, attacker)) {
        return 0;
    }
    if (!ObjectiveQueue_GetAttackObjective_485900(victim + 0x5F0) && vc::At<int32_t>(victim, 0x1310) != 0xD &&
        vc::At<int32_t>(stimulus, 0) != 0x40) {
        return 0;
    }
    return TakeOn(ped, attacker);
}

// 0x6012B0 (thiscall): ConditionNearbyDockLocation: a dock (+0x1D1C) kept, or one picked
// whose chance (+0x100 of its +8) the roll 0..99 is within
uint8_t __fastcall NearbyDockLocation_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    if (vc::At<int32_t>(ped, 0x1D1C) != 0) {
        return 1;
    }
    uint8_t* dock = PedScheduleManager_Pick_46EA10(schedules_C2AB68, ped);
    if (!dock) {
        return 0;
    }
    uint8_t* info = vc::At<uint8_t*>(dock, 8);
    if (!info) {
        return 0;
    }
    int32_t roll = int32_t(uint32_t(Rand_85C6FAg()) * 99u) / 32767;
    if (roll > int32_t(int8_t(info[0x100]))) {
        return 0;
    }
    vc::At<uint8_t*>(ped, 0x1D1C) = dock;
    return 1;
}

// 0x601330 (thiscall): ConditionNearbyFleeDockLocation: the nearest dock within +4
uint8_t __fastcall NearbyFleeDockLocation_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    float distance = fleeDockRange_9009B8;
    uint8_t* ped = Ped(owner);
    uint8_t* dock = PedScheduleManager_FindNearest_46EA90(schedules_C2AB68, ped, &distance);
    if (!dock || !(vc::At<float>(self, 4) > distance)) {
        return 0;
    }
    vc::At<uint8_t*>(ped, 0x1D1C) = dock;
    return 1;
}

// 0x601380 (thiscall): ConditionTargetFaction: the attitude towards the scanner's +0x184
// (else the fight target, a ped) is +4
uint8_t __fastcall TargetFaction_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    uint8_t* scanner = ped + 0x3BC;
    uint8_t* target;
    if (Scanner_Get184_48C060g(scanner)) {
        target = Scanner_Get184_48C060g(scanner);
    } else {
        target = Ped_GetFightTarget_497100g(ped);
        if (!target || !IsPed(target)) {
            return 0;
        }
        target = FightTargetPed(ped);
    }
    if (!target) {
        return 0;
    }
    int32_t attitude;
    Ped_GetAttitude_475380g(ped, &attitude, vc::At<int32_t>(target, 0x1310));
    return attitude == vc::At<int32_t>(self, 4);
}

// 0x601410 (cdecl): the ped's faction's attitudes to the six clothing slots worn and
// the faction's own: with flag 0 any one equal to want; with the flag, the count of 0s
// (want 0) or of 2s (want 2) at least the other's; only while it fights a ped
uint8_t __cdecl ClothingAttitudeMatches(uint8_t* ped, int32_t want, uint32_t flagArg) {
    uint8_t flag = uint8_t(flagArg);
    int32_t zeros = 0, twos = 0;
    if (!ped) {
        return 0;
    }
    if (!FightTargetPed(ped)) {
        return 0;
    }
    int32_t faction = vc::At<int32_t>(ped, 0x1310);
    for (uint8_t* slot = kClothingSlots; slot < kClothingSlotsEnd; slot += 0xC) {
        int32_t attitude;
        if (*slot == 0xFF || faction == -1) {
            attitude = 1;
        } else {
            attitude = kClothingAttitudes[faction * 0xB + int32_t(int8_t(*slot))];
        }
        if (!flag && attitude == want) {
            return 1;
        }
        if (attitude == 0) {
            zeros++;
        } else if (attitude == 2) {
            twos++;
        }
    }
    int32_t attitude;
    if (clothingFactionCount_20C78F4 > 0 && faction != -1) {
        attitude = *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(kFactionAttitudes) + faction * 0x2C);
    } else {
        attitude = 1;
    }
    if (!flag && attitude == want) {
        return 1;
    }
    if (attitude == 0) {
        zeros++;
    } else if (attitude == 2) {
        twos++;
    }
    if (!flag) {
        return 0;
    }
    if (want == 0) {
        return zeros >= twos ? 1 : 0;
    }
    if (want == 2) {
        return twos >= zeros ? 1 : 0;
    }
    return 0;
}

// 0x601520 (thiscall): ConditionTargetClothing: against the player (the fight target):
// the clothing attitudes (+4 wanted, +8 counted); the flag byte goes in the owner
// argument's slot, its other bytes staying the owner's address
uint8_t __fastcall TargetClothing_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    uint8_t* target = Ped_GetFightTarget_497100g(ped);
    if (!target || !IsPed(target) || vc::At<int32_t>(target, 0x1310) != 0xD) {
        return 0;
    }
    uint32_t flagDword = (uint32_t(reinterpret_cast<uintptr_t>(owner)) & 0xFFFFFF00u) | self[8];
    return ClothingAttitudeMatches(ped, vc::At<int32_t>(self, 4), flagDword);
}

// 0x601580 (thiscall): ConditionAttackRole: never (its test of +0x538 is unused)
uint8_t __fastcall AttackRole_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    return 0;
}

// 0x6015A0 (thiscall): ConditionFightClass
uint8_t __fastcall FightClass_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    return vc::At<int32_t>(Ped(owner), 0x714) == vc::At<int32_t>(self, 4);
}

// 0x6015C0 (thiscall): ConditionCharacterSize: the model's size type
uint8_t __fastcall CharacterSize_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* info = Ped_GetModelInfo_477DC0g(Ped(owner));
    int32_t size;
    PedModelInfo_GetType_522380(info, &size);
    return size == vc::At<int32_t>(self, 4);
}

// 0x6015F0 (thiscall): ConditionTargetRelativeCharacterSize: the ped's size against the
// target's of kind +8 (0..2 or 8..10), op +4
uint32_t __fastcall TargetRelativeCharacterSize_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    uint32_t kind = vc::At<uint32_t>(self, 8);
    if (kind > 10 || (kind >= 3 && kind <= 7)) {
        return 0;
    }
    uint8_t* target = Ped_FindByType_477C00g(ped, int32_t(kind), nullptr);
    if (!target || !IsPed(target)) {
        return 0;
    }
    uint8_t* mine = Ped_GetModelInfo_477DC0g(ped);
    uint8_t* theirs = Ped_GetModelInfo_477DC0g(target);
    if (!mine || !theirs) {
        return 0;
    }
    int32_t a, b;
    PedModelInfo_GetType_522380(mine, &a);
    PedModelInfo_GetType_522380(theirs, &b);
    return CompareIntsG(a, b, vc::At<int32_t>(self, 4));
}

// 0x6016B0 (thiscall): ConditionJoinCombatZone: joining the +0x538 ped's combat in the
// zone list +8 (above 2: the first of 0..2 that takes)
uint8_t __fastcall JoinCombatZone_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    if (!ped) {
        return 0;
    }
    uint8_t* target = vc::At<uint8_t*>(ped, 0x538);
    if (!target || !IsPed(target)) {
        return 0;
    }
    uint32_t zone = vc::At<uint32_t>(self, 8);
    if (zone <= 2) {
        return PedCombat_JoinTarget_487BC0(ped + 0x668, target, int(zone), 1) != 0 ? 1 : 0;
    }
    for (int i = 0; i < 3; i++) {
        if (PedCombat_JoinTarget_487BC0(ped + 0x668, target, i, 1)) {
            return 1;
        }
    }
    return 0;
}

// 0x601740 (thiscall): ConditionInCombatZone: the fight target's 2D range within the
// ring +4 (0: inside the first radius, 1: between the two, 2: between the second and
// 100), widened by +8. (Another ring reads the owner argument's bits as its inner
// radius and an unset local as its outer: taken as 0 here.)
uint32_t __fastcall InCombatZone_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* target = Ped_GetFightTarget_497100g(Ped(owner));
    if (!target) {
        return 0;
    }
    float inner;
    float outer = 0.0f;
    std::memcpy(&inner, &owner, sizeof(inner));
    int32_t ring = vc::At<int32_t>(self, 4);
    uint8_t* zones = combatZones_20C889C;
    if (ring == 0) {
        inner = 0.0f;
        outer = vc::At<float>(zones, 0x2C);
    } else if (ring == 1) {
        inner = vc::At<float>(zones, 0x2C);
        outer = vc::At<float>(zones, 0x30);
    } else if (ring == 2) {
        inner = vc::At<float>(zones, 0x30);
        outer = outerZone_900490;
    }
    inner = inner - vc::At<float>(self, 8);
    outer = vc::At<float>(self, 8) + outer;
    float d[3];
    float* delta = VectorSubG(d, EntityPos(target), EntityPos(Ped(owner)));
    float lengthSq = std::fabs(delta[0] * delta[0] + delta[1] * delta[1]);
    float length = CrtSqrtG(lengthSq);
    if (!(inner <= length)) {
        return 0;
    }
    return outer >= length ? 1u : 0u;
}

// 0x601860 (thiscall): ConditionAcquireFocus: the current objective's entity (kind 7:
// its target, 0x11: +0x10, 0xD: +0x3C) made the focus
uint8_t __fastcall AcquireFocus_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    uint8_t* objective = ObjectiveQueue_GetCurrent_470340g(ped + 0x5F0);
    if (!objective) {
        return 0;
    }
    uint8_t* entity;
    int32_t kind = vc::At<int32_t>(objective, 8);
    if (kind == 7) {
        entity = Objective_GetTarget_8B6120(objective);
    } else if (kind == 0x11) {
        entity = vc::At<uint8_t*>(objective, 0x10);
    } else if (kind == 0xD) {
        entity = vc::At<uint8_t*>(objective, 0x3C);
    } else {
        return 0;
    }
    if (!entity || entity[0x1C98]) {
        return 0;
    }
    Scanner_SetFocus_48C4C0(ped + 0x3BC, entity);
    return 1;
}

// 0x6018C0 (thiscall): ConditionAcquireTarget
uint8_t __fastcall AcquireTarget_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return Ped_ChooseTargetIfAsked_499470(Ped(owner)) != nullptr;
}

// 0x6018E0 (thiscall): ConditionAcquirePunishableTarget: of the peds it may attack, the
// one within 70 with the most punishment points (at least 1; not the player while
// +0x1E91), else the nearest already punished (+0x14E0); made the target, marked
// punished once over the limit
uint8_t __fastcall AcquirePunishableTarget_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    float nearestDistance = punishRange_90D0C8;
    uint8_t* me = Ped(owner);
    uint8_t* pool = *reinterpret_cast<re::Ptr32<uint8_t>*>(kPedPoolSlotG);
    uint8_t* found = nullptr;
    uint8_t* nearest = nullptr;
    int32_t mostPoints = 1;
    for (int32_t i = vc::At<int32_t>(pool, 8); i != 0;) {
        i--;
        if (vc::At<uint8_t*>(pool, 4)[i] & 0x80) {
            continue;
        }
        uint8_t* other = vc::At<uint8_t*>(pool, 0) + vc::At<int32_t>(pool, 0xC) * i;
        if (!other || other == me) {
            continue;
        }
        if (!Ped_CanAttackStanding_497F90(me, other, 0, 0)) {
            continue;
        }
        if (vc::At<int32_t>(other, 0x14E0) != 0) {
            float distance = Distance2D_470110(EntityPos(me), EntityPos(other));
            if (nearestDistance > distance) {
                nearestDistance = distance;
                nearest = other;
            }
        }
        if (vc::At<int32_t>(other, 0x1D40) < mostPoints) {
            continue;
        }
        if (!(Distance2D_470110(EntityPos(me), EntityPos(other)) < punishRange_90D0C8)) {
            continue;
        }
        if (vc::At<int32_t>(other, 0x1310) == 0xD && player_C1AEA8g[0x1E91]) {
            continue;
        }
        mostPoints = vc::At<int32_t>(other, 0x1D40);
        found = other;
    }
    if (!found) {
        if (!nearest) {
            return 0;
        }
        found = nearest;
    }
    Scanner_Alert_48C480g(me + 0x3BC, found);
    Ped_SetTargetEntity_498B10g(me, found);
    Targeting_SetTarget_4D79E0(vc::At<uint8_t*>(me, 0x1C8), found);
    if (vc::At<int32_t>(found, 0x1D40) >= vc::At<int32_t>(punishment_C0F78Cg, 0x30)) {
        vc::At<int32_t>(found, 0x14E0) = 1;
    }
    return 1;
}

// 0x601A80 (thiscall): ConditionAcquireRandomTargetInGroup: from a random member of its
// group on, the first other member made the target
uint8_t __fastcall AcquireRandomTargetInGroup_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    uint8_t* group = vc::At<uint8_t*>(ped, 0x1D20);
    if (!group) {
        return 0;
    }
    int32_t count = PedGroup_CountMembers_489700g(group);
    if (count <= 1) {
        return 0;
    }
    int32_t start = int32_t(uint32_t(Rand_85C6FAg()) * uint32_t(count - 1)) / 32767;
    int32_t i = start;
    do {
        uint8_t* member = uint32_t(i) < vc::At<uint32_t>(group, 0x2C) ? vc::At<uint8_t*>(group, 4 + i * 4) : nullptr;
        if (member && member != ped) {
            Scanner_Alert_48C480g(ped + 0x3BC, member);
            Ped_SetTargetEntity_498B10g(ped, member);
            Targeting_SetTarget_4D79E0(vc::At<uint8_t*>(ped, 0x1C8), member);
            return 1;
        }
        i++;
        if (i >= count) {
            i = 0;
        }
    } while (i != start);
    return 0;
}

// 0x601B30 (thiscall): ConditionAcquireAuthorityFocus: the first authority (faction 0 /
// 8 / 7) the scanner lists made the focus
uint8_t __fastcall AcquireAuthorityFocus_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* scanner = Ped(owner) + 0x3BC;
    int32_t count = vc::At<int32_t>(scanner, 4);
    for (int32_t i = 0; i < count; i++) {
        uint8_t* other = vc::At<uint8_t*>(scanner, 8 + i * 4);
        if (!other) {
            continue;
        }
        int32_t faction = vc::At<int32_t>(other, 0x1310);
        if (faction == 0 || faction == 8 || faction == 7) {
            Scanner_SetFocus_48C4C0(scanner, other);
            return 1;
        }
    }
    return 0;
}

// 0x601B90 (thiscall): ConditionAcquireFactionFocus: the first of its own faction
uint8_t __fastcall AcquireFactionFocus_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    uint8_t* scanner = ped + 0x3BC;
    int32_t count = vc::At<int32_t>(ped, 0x3C0);
    for (int32_t i = 0; i < count; i++) {
        uint8_t* other = vc::At<uint8_t*>(scanner, 8 + i * 4);
        if (ped && other && vc::At<int32_t>(ped, 0x1310) == vc::At<int32_t>(other, 0x1310)) {
            Scanner_SetFocus_48C4C0(scanner, other);
            return 1;
        }
    }
    return 0;
}

// 0x601BF0 (thiscall): ConditionIsHealthAtHumiliationLevel: a male ped with the stat
// 0x3F whose health fraction is under it (a percentage), or recently attacked (under
// 5 s) by one it's in mood 4 with
uint8_t __fastcall IsHealthAtHumiliationLevel_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    if (Ped_GetModelInfo_477DC0g(Ped(owner))[0x50]) {
        return 0;
    }
    if (Ped_GetStat_477060g(Ped(owner), 0x3F) <= 0) {
        return 0;
    }
    int32_t stat = Ped_GetStat_477060g(Ped(owner), 0x3F);
    uint8_t* ped = Ped(owner);
    float fraction = vc::At<float>(ped, 0x1CB8) / vc::At<float>(ped, 0x1CA4);
    float level = float(double(stat) / percent_900130g);
    if (level > fraction) {
        return 1;
    }
    uint8_t* attacker = Ped_Fn497020(Ped(owner));
    if (!attacker) {
        return 0;
    }
    if (!(Ped_GetAttackerTime_496990(Ped(owner)) < attackerTimeLimit_900188)) {
        return 0;
    }
    int32_t mood;
    return *Social_GetMood_48EBC0(Ped(owner) + 0xB44, &mood, attacker, 0, 0, 0) == 4 ? 1 : 0;
}

// 0x601CC0 (thiscall): ConditionTargetedByTarget: the fight target (a ped) fights or
// targets the ped
uint32_t __fastcall TargetedByTarget_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    uint8_t* target = Ped_GetFightTarget_497100g(ped);
    if (!target || !IsPed(target)) {
        return 0;
    }
    uint8_t* theirs = FightTargetPed(target);
    uint8_t* locked = Targeting_GetTarget_4D5AD0g(vc::At<uint8_t*>(target, 0x1C8));
    return ped == theirs || ped == locked ? 1u : 0u;
}

// 0x601D40 (thiscall): ConditionTargetedByPlayer: the player fights or targets the ped
uint32_t __fastcall TargetedByPlayer_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    if (!player_C1AEA8g) {
        return 0;
    }
    uint8_t* theirs = FightTargetPed(player_C1AEA8g);
    uint8_t* locked = Targeting_GetTarget_4D5AD0g(vc::At<uint8_t*>(player_C1AEA8g, 0x1C8));
    return ped == theirs || ped == locked ? 1u : 0u;
}

// 0x601DB0 (thiscall): ConditionTargetPlayersGF: the player's girlfriend (+0x2F8 while
// +0x153C / +0x14EC, else the grapple partner) made the target
uint8_t __fastcall TargetPlayersGF_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    uint8_t* player = player_C1AEA8g;
    if (!ped || !player) {
        return 0;
    }
    uint8_t* girlfriend = nullptr;
    if (vc::At<int32_t>(player, 0x153C) != 0 || vc::At<int32_t>(player, 0x14EC) != 0) {
        girlfriend = vc::At<uint8_t*>(player, 0x2F8);
    }
    if (!girlfriend) {
        girlfriend = vc::At<uint8_t*>(player, 0x155C);
        if (!girlfriend) {
            return 0;
        }
    }
    if (girlfriend == ped) {
        return 0;
    }
    return TakeOn(ped, girlfriend);
}

// the action context of an entity (its vfunc 0x40), or null
uint8_t* ActionContextOf(uint8_t* entity) {
    return vc::Call<uint8_t*>(entity, 0x40);
}

// 0x601E20 (thiscall): ConditionTargetConditionGroup: the list +4 run on the target's
// action context (the collider's +0x1C0 if any; +8: the grapple partner; else the
// fight target or looked-at one, a ped); its +0x34 cleared first
uint32_t __fastcall TargetConditionGroup_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* collider = vc::At<uint8_t*>(owner, 0xC);
    uint8_t* ped = Ped(owner);
    uint8_t* target;
    if (collider) {
        target = vc::At<uint8_t*>(collider, 0x1C0);
    } else if (self[8]) {
        if (!ped) {
            return 0;
        }
        target = vc::At<uint8_t*>(ped, 0x155C);
    } else {
        if (!ped) {
            return 0;
        }
        target = Ped_GetFightTarget_497100g(ped);
        if (!target) {
            target = Targeting_GetLookTarget_4D7780g(vc::At<uint8_t*>(ped, 0x1C8));
            if (!target) {
                return 0;
            }
        }
        if (!IsPed(target)) {
            return 0;
        }
    }
    if (!target) {
        return 0;
    }
    uint8_t* context = ActionContextOf(target);
    if (!context) {
        return 0;
    }
    vc::At<int32_t>(context, 0x34) = 0;
    uint8_t* list = vc::At<uint8_t*>(self, 4);
    if (!list) {
        return 0;
    }
    return ConditionList_EvaluateG(list, context);
}

// 0x601EB0 (thiscall): ConditionFocusConditionGroup: on the scanner's threat's context
uint32_t __fastcall FocusConditionGroup_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* threat = Scanner_GetThreat_48C070g(Ped(owner) + 0x3BC);
    if (!threat) {
        return 0;
    }
    uint8_t* context = ActionContextOf(threat);
    if (!context) {
        return 0;
    }
    return ConditionList_EvaluateG(vc::At<uint8_t*>(self, 4), context);
}

// 0x601EF0 (thiscall): ConditionPedConditionGroup: on the ped's own context (without
// one: true when there's a collider)
uint32_t __fastcall PedConditionGroup_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    if (ped) {
        uint8_t* context = ActionContextOf(ped);
        if (context) {
            return ConditionList_EvaluateG(vc::At<uint8_t*>(self, 4), context);
        }
    }
    return vc::At<int32_t>(owner, 0xC) != 0 ? 1u : 0u;
}

// 0x601F30 (thiscall): ConditionAIConditionGroup: on the AI's context (+0x2A8 of its
// action object)
uint32_t __fastcall AIConditionGroup_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    if (!ped) {
        return 0;
    }
    uint8_t* context = vc::At<uint8_t*>(vc::At<uint8_t*>(ped, 0x2E0), 0x2A8);
    if (!context) {
        return 0;
    }
    return ConditionList_EvaluateG(vc::At<uint8_t*>(self, 4), context);
}

// 0x601F60 (thiscall): ConditionAttackerConditionGroup: on the attacker's context (the
// collider's +0x1C0, else the last hit's +4)
uint32_t __fastcall AttackerConditionGroup_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* collider = vc::At<uint8_t*>(owner, 0xC);
    uint8_t* attacker;
    if (collider) {
        attacker = vc::At<uint8_t*>(collider, 0x1C0);
    } else {
        uint8_t* ped = Ped(owner);
        if (!ped) {
            return 0;
        }
        attacker = vc::At<uint8_t*>(Ped_GetLastHitWeapon_474BE0(ped), 4);
    }
    if (!attacker) {
        return 0;
    }
    uint8_t* context = ActionContextOf(attacker);
    if (!context) {
        return 0;
    }
    return ConditionList_EvaluateG(vc::At<uint8_t*>(self, 4), context);
}

} // namespace

#define CONDG_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

CONDG_REVERSED("ConditionTargetMastersAttacker::Evaluate", 0x006010B0, &TargetMastersAttacker_Evaluate);
CONDG_REVERSED("ConditionTargetFriendsAttacker::Evaluate", 0x00601170, &TargetFriendsAttacker_Evaluate);
CONDG_REVERSED("ConditionNearbyDockLocation::Evaluate", 0x006012B0, &NearbyDockLocation_Evaluate);
CONDG_REVERSED("ConditionNearbyFleeDockLocation::Evaluate", 0x00601330, &NearbyFleeDockLocation_Evaluate);
CONDG_REVERSED("ConditionTargetFaction::Evaluate", 0x00601380, &TargetFaction_Evaluate);
CONDG_REVERSED("Condition::ClothingAttitudeMatches", 0x00601410, &ClothingAttitudeMatches);
CONDG_REVERSED("ConditionTargetClothing::Evaluate", 0x00601520, &TargetClothing_Evaluate);
CONDG_REVERSED("ConditionAttackRole::Evaluate", 0x00601580, &AttackRole_Evaluate);
CONDG_REVERSED("ConditionFightClass::Evaluate", 0x006015A0, &FightClass_Evaluate);
CONDG_REVERSED("ConditionCharacterSize::Evaluate", 0x006015C0, &CharacterSize_Evaluate);
CONDG_REVERSED("ConditionTargetRelativeCharacterSize::Evaluate", 0x006015F0, &TargetRelativeCharacterSize_Evaluate);
CONDG_REVERSED("ConditionJoinCombatZone::Evaluate", 0x006016B0, &JoinCombatZone_Evaluate);
CONDG_REVERSED("ConditionInCombatZone::Evaluate", 0x00601740, &InCombatZone_Evaluate);
CONDG_REVERSED("ConditionAcquireFocus::Evaluate", 0x00601860, &AcquireFocus_Evaluate);
CONDG_REVERSED("ConditionAcquireTarget::Evaluate", 0x006018C0, &AcquireTarget_Evaluate);
CONDG_REVERSED("ConditionAcquirePunishableTarget::Evaluate", 0x006018E0, &AcquirePunishableTarget_Evaluate);
CONDG_REVERSED("ConditionAcquireRandomTargetInGroup::Evaluate", 0x00601A80, &AcquireRandomTargetInGroup_Evaluate);
CONDG_REVERSED("ConditionAcquireAuthorityFocus::Evaluate", 0x00601B30, &AcquireAuthorityFocus_Evaluate);
CONDG_REVERSED("ConditionAcquireFactionFocus::Evaluate", 0x00601B90, &AcquireFactionFocus_Evaluate);
CONDG_REVERSED("ConditionIsHealthAtHumiliationLevel::Evaluate", 0x00601BF0, &IsHealthAtHumiliationLevel_Evaluate);
CONDG_REVERSED("ConditionTargetedByTarget::Evaluate", 0x00601CC0, &TargetedByTarget_Evaluate);
CONDG_REVERSED("ConditionTargetedByPlayer::Evaluate", 0x00601D40, &TargetedByPlayer_Evaluate);
CONDG_REVERSED("ConditionTargetPlayersGF::Evaluate", 0x00601DB0, &TargetPlayersGF_Evaluate);
CONDG_REVERSED("ConditionTargetConditionGroup::Evaluate", 0x00601E20, &TargetConditionGroup_Evaluate);
CONDG_REVERSED("ConditionFocusConditionGroup::Evaluate", 0x00601EB0, &FocusConditionGroup_Evaluate);
CONDG_REVERSED("ConditionPedConditionGroup::Evaluate", 0x00601EF0, &PedConditionGroup_Evaluate);
CONDG_REVERSED("ConditionAIConditionGroup::Evaluate", 0x00601F30, &AIConditionGroup_Evaluate);
CONDG_REVERSED("ConditionAttackerConditionGroup::Evaluate", 0x00601F60, &AttackerConditionGroup_Evaluate);
