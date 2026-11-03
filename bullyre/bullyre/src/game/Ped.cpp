// CPed (0x470110 .. 0x49AD40): money, stats, health, the punishment points, gifts,
// blended anims, and the small accessors.

#include "game/Ped.h"

#include <cmath>

#include "game/Pools.h"
#include "reversed.h"

namespace {
GAME_VAR(uint32_t, PED_timeInMs, 0x00C1A9B4);      // CTimer::m_snTimeInMilliseconds
GAME_VAR(float, PED_timeStep, 0x00C1A9A4);         // CTimer::ms_fTimeStep
GAME_VAR(float, PED_stepSeconds, 0x00A3396C);      // 0.02: a step's seconds at time step 1
GAME_VAR(CPed*, PED_player, 0x00C1AEA8);           // FindPlayerPed
inline void* const PED_playerInfo = reinterpret_cast<void*>(0x00C1AEA8);  // (its first field is the ped)
GAME_VAR(int32_t, PED_currArea, 0x00BD1008);       // CGame::currArea
GAME_VAR(uint8_t*, PED_aiButes, 0x020C889C);       // +0x84, +0x8C .. +0x98: punishment settings
GAME_VAR(uint8_t*, PED_punishButes, 0x00C0F78C);   // +0x34: above it, the points drop later
GAME_VAR(void*, PED_hud, 0x00C674D4);              // told when the player's money changes (vf7)
GAME_VAR(void*, PED_radar, 0x00C674CC);
GAME_VAR(void*, PED_vehicleSomething, 0x00CF4274);
inline void* const PED_objCF4378 = reinterpret_cast<void*>(0x00CF4378);
inline void* const PED_giftMgr = reinterpret_cast<void*>(0x020C3CA0);
inline void* const PED_animMgr = reinterpret_cast<void*>(0x020C4910);
GAME_VAR(float, PED_floatCF311C, 0x00CF311C);
GAME_VAR(uint32_t, PED_valueCF312C, 0x00CF312C);
GAME_VAR(uint32_t, PED_valueCF3130, 0x00CF3130);
GAME_VAR(uint32_t, PED_valueCF3134, 0x00CF3134);
GAME_VAR(int32_t, PED_divisorA35054, 0x00A35054);
GAME_VAR(void*, PED_attitudeDefaults, 0x00C2AF9C);  // the attitudes by ped type
// the attitude GetAttitude found last (a function static; its guard bit is set on first use)
GAME_VAR(uint32_t, PED_attitudeGuard, 0x00C2B1B0);
GAME_VAR(int32_t, PED_attitudeLast, 0x00C2B1AC);
GAME_VAR(CPed*, PED_pedC2B118, 0x00C2B118);
GAME_ARRAY(void*, PED_modelInfoPtrs, 0x00C67738);
// the gifts: a 0x14-byte entry each, its requirement at +0x1A from 0x20C3680
inline int16_t& GiftRequirement(int index) {
    return *reinterpret_cast<int16_t*>(0x020C369A + index * 0x14);
}
GAME_ARRAY(int32_t, PED_giftRequests, 0x020C3B9C);
// the pickups (0x48 bytes each)
inline uint8_t& PickupState(int index) {
    return *reinterpret_cast<uint8_t*>(0x00C0A144 + index * 0x48);
}
inline re::Ptr32<CEntity>& PickupEntity(int index) {
    return *reinterpret_cast<re::Ptr32<CEntity>*>(0x00C0A11C + index * 0x48);
}

// model ids (CModelInfo's, looked up at start)
GAME_VAR(int32_t, PED_miWeapon4330, 0x00BF4330);
GAME_VAR(int32_t, PED_miBF42CC, 0x00BF42CC);
GAME_VAR(int32_t, PED_miBF42D0, 0x00BF42D0);
GAME_VAR(int32_t, PED_miBF42D4, 0x00BF42D4);
GAME_VAR(int32_t, PED_miBF42D8, 0x00BF42D8);
GAME_VAR(int32_t, PED_miBF42DC, 0x00BF42DC);
// the weapons stat 6 is 0 with
inline const uintptr_t kStat6Weapons[] = {0x00BF444C, 0x00BF4448, 0x00BF4438, 0x00BF4444, 0x00BF443C, 0x00BF4440,
                                          0x00BF442C, 0x00BF4424, 0x00BF4420, 0x00BF44D8, 0x00BF4410, 0x00BF43AC};

GAME_FN(void, __cdecl, PED_IncrementStat, 0x00458480, int stat, int value);
GAME_FN(void, __cdecl, PED_SetNodeAlpha, 0x005F2B00, void* node, int alpha);
GAME_FN(void, __cdecl, PED_RegisterReference, 0x004657D0, void* entity, void* ref);
GAME_FN(void, __cdecl, PED_CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(ped::CVector*, __cdecl, PED_VectorSub, 0x00414B70, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(float, __thiscall, PED_VectorNormalise, 0x00414E10, ped::CVector* v);
GAME_FN(void, __thiscall, PED_PlacementGetRight, 0x00473600, ped::CVector* placement, ped::CVector* out);
GAME_FN(void, __thiscall, PED_GetForward, 0x004154A0, CPed* ped, ped::CVector* out);
GAME_FN(void, __thiscall, PED_MatrixSetRotateZ, 0x00412C20, void* matrix, float heading);
GAME_FN(void, __thiscall, PED_Notify4475D0, 0x004475D0, void* object, CPed* ped);
GAME_FN(void, __thiscall, PED_AttitudeReset, 0x0046F2B0, void* attitudeSet);
GAME_FN(int*, __thiscall, PED_AttitudeDefault, 0x0046F830, void* table, int* out, int pedType, int otherType);
GAME_FN(void, __thiscall, PED_Vehicle4CE960, 0x004CE960, void* part);
GAME_FN(void, __thiscall, PED_Fn5A59D0, 0x005A59D0, void* object, CPed* ped, int a, int b);
GAME_FN(void, __thiscall, PED_Fn5924E0, 0x005924E0, void* object, CPed* ped, int a);
GAME_FN(uint8_t, __thiscall, PED_GiftMgr6AA7E0, 0x006AA7E0, void* mgr);
GAME_FN(void, __thiscall, PED_RadarAddBlip, 0x0054A890, void* radar, int kind, int handle);
GAME_FN(uint8_t, __thiscall, PED_Fn48D470, 0x0048D470, void* object, CPed* ped);
GAME_FN(void, __thiscall, PED_AnimAdd, 0x006BD010, void* mgr, void* clump, int id);
GAME_FN(void, __thiscall, PED_AnimRemove, 0x006BD030, void* mgr, void* clump, int id, int);
GAME_FN(void, __thiscall, PED_ClumpStopAnim, 0x006B3DA0, void* clump, int id, int);
GAME_FN(void, __thiscall, PED_Player49BBD0, 0x0049BBD0, CPed* player, int model, uint32_t* out);
GAME_FN(uint32_t, __thiscall, PED_PlayerInfo4466F0, 0x004466F0, void* info, int flag);
GAME_FN(Objective*, __cdecl, PED_AsObjective17, 0x00474EC0, Objective* objective);
GAME_FN(int, __cdecl, PED_FindPickup, 0x004438B0, int id);
GAME_FN(CEntity*, __thiscall, PED_Targeting4D7780, 0x004D7780, void* targeting);
GAME_FN(CEntity*, __thiscall, PED_Targeting4D62C0, 0x004D62C0, void* targeting);
GAME_FN(CEntity*, __thiscall, PED_Targeting4D5AD0, 0x004D5AD0, void* targeting);
GAME_FN(CEntity*, __thiscall, PED_Fn48C070, 0x0048C070, void* object);
GAME_FN(uint8_t, __thiscall, PED_Chain485320, 0x00485320, void* chain);
GAME_FN(uint8_t, __thiscall, PED_Chain485330, 0x00485330, void* chain);
// the objective queue's (where modes apply)
GAME_FN(Objective*, __thiscall, PED_QueueGetCurrent, 0x00470340, ObjectiveQueue* queue);
GAME_FN(uint8_t, __thiscall, PED_QueueRemoveFirst, 0x004702C0, ObjectiveQueue* queue);
// CPed's own (where modes apply)
GAME_FN(uint8_t, __thiscall, PED_Is1D24Kind3, 0x00473220, CPed* ped);
GAME_FN(void, __thiscall, PED_Update1D44, 0x004745D0, CPed* ped);
GAME_FN(void, __thiscall, PED_SetPunishment, 0x00477520, CPed* ped, int points);
GAME_FN(void, __thiscall, PED_UpdatePunishment, 0x00477680, CPed* ped);
GAME_FN(void, __thiscall, PED_SubPunishment, 0x0049A830, CPed* ped, int points);
GAME_FN(void, __thiscall, PED_ResetPunishment, 0x0049A840, CPed* ped);
GAME_FN(void, __thiscall, PED_SetVehicle, 0x00476180, CPed* ped, CVehicle* vehicle);
GAME_FN(int32_t, __thiscall, PED_GetStat, 0x00477060, CPed* ped, int stat);
GAME_FN(CEntity*, __thiscall, PED_GetTarget, 0x00497100, CPed* ped);
GAME_FN(CEntity*, __thiscall, PED_Fn496CB0, 0x00496CB0, CPed* ped, int* out);
GAME_FN(uint8_t, __thiscall, PED_InVehicle1558, 0x004CE690, CPed* ped);
GAME_FN(void, __thiscall, PED_Set1570, 0x004771F0, CPed* ped, int state, int kind, int a, int b);
GAME_FN(uint8_t, __thiscall, PED_Fn47B800, 0x0047B800, CPed* ped, float a, uintptr_t b);
GAME_FN(int*, __thiscall, PED_GetAttitude, 0x00475380, CPed* ped, int* out, int pedType);

GAME_FN(int*, __thiscall, PED_VehicleKind, 0x004CAF30, CVehicle* vehicle, int* out);
GAME_FN(void, __thiscall, PED_AnimMgr6C2200, 0x006C2200, void* mgr, void* animBlend, uintptr_t anim);
GAME_FN(void, __thiscall, PED_Targeting4D7800, 0x004D7800, void* targeting);
GAME_FN(void, __thiscall, PED_Fn72EEF0, 0x0072EEF0, void* object);
GAME_FN(void, __thiscall, PED_SetTarget497E70, 0x00497E70, CPed* ped, CEntity* target, int, ped::CVector* offset);
GAME_FN(void, __thiscall, PED_Intel484E60, 0x00484E60, uint8_t* intelligence, CPed* ped);
GAME_FN(void, __thiscall, PED_Intel4843E0, 0x004843E0, uint8_t* intelligence, uintptr_t a, uintptr_t b);
GAME_FN(void, __thiscall, PED_Intel4849B0, 0x004849B0, uint8_t* intelligence, uintptr_t a, uintptr_t b);
GAME_FN(uint8_t, __thiscall, PED_Intel484A70, 0x00484A70, uint8_t* intelligence, uintptr_t a, uintptr_t b);
GAME_FN(uint8_t, __thiscall, PED_Intel484AF0, 0x00484AF0, uint8_t* intelligence, uintptr_t a, uintptr_t b);
GAME_FN(void, __thiscall, PED_Intel484480, 0x00484480, uint8_t* intelligence);

inline uint8_t* Bytes(void* p) {
    return reinterpret_cast<uint8_t*>(p);
}
using re::At;
const ped::CVector* PosOf(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<ped::CVector*>(matrix + 0x30) : reinterpret_cast<ped::CVector*>(Bytes(entity) + 4);
}
// the clump its anims play on
void* Clump(CPed* ped) {
    return At<void*>(At<void*>(ped->animBlend, 0x94), 0x10);
}
// Some models don't take punishment points
bool IgnoresPunishment(CPed* ped) {
    int model = ped->modelIndex;
    return model == PED_miBF42D4 || model == PED_miBF42D0 || model == PED_miBF42CC || model == PED_miBF42DC ||
           model == PED_miBF42D8;
}
// CTimer's time in seconds, as the game computes it (unsigned, times 0.001f)
float NowSeconds() {
    return static_cast<float>(static_cast<double>(PED_timeInMs) * static_cast<double>(0.001f));
}
constexpr uint32_t kMoneyCap = 10000000;
constexpr int kPedTypePlayer = 13;
} // namespace

// ---------------------------------------------------------------------------------
// Static helpers

float __cdecl CPed::Distance2D(const ped::CVector* a, const ped::CVector* b) {
    ped::CVector d;
    PED_VectorSub(&d, a, b);
    float sq = d.y * d.y + d.x * d.x + 0.0f * 0.0f;
    return static_cast<float>(std::sqrt(std::fabs(sq)));
}

ped::CVector* __cdecl CPed::NormalisedCopy(ped::CVector* out, const ped::CVector* in) {
    *out = *in;
    PED_VectorNormalise(out);
    return out;
}

float __cdecl CPed::DistanceSquared(const ped::CVector* a, const ped::CVector* b) {
    ped::CVector d;
    PED_VectorSub(&d, a, b);
    return d.y * d.y + d.x * d.x + d.z * d.z;
}

CVehicle* __cdecl CPed::AsVehicle(CEntity* entity) {
    if (!entity) {
        return nullptr;
    }
    return (At<uint8_t>(entity, 0x108) & 7) == 2 ? reinterpret_cast<CVehicle*>(entity) : nullptr;
}

uint8_t __cdecl CPed::IsPedWith1580(CEntity* entity) {
    return entity && (At<uint8_t>(entity, 0x108) & 7) == 3 && At<int32_t>(entity, 0x1580) > 0;
}

// ---------------------------------------------------------------------------------
// Money

uint8_t CPed::CanAfford(uint32_t amount) {
    return amount <= money;
}

void CPed::TakeMoney(uint32_t amount) {
    amount -= amount % 25;
    if (money < amount) {
        money = 0;
        return;
    }
    money -= amount;
}

void CPed::SetMoney(uint32_t amount) {
    amount -= amount % 25;
    if (amount > kMoneyCap) {
        money = kMoneyCap;
    } else {
        if (pedType == kPedTypePlayer && amount > money) {
            PED_IncrementStat(0, static_cast<int>(amount - money));
        }
        money = amount;
    }
    if (this == PED_player) {
        re::Ptr32<void>* vt = *re::PtrCast<re::Ptr32<re::Ptr32<void>>*>(PED_hud);
        re::PtrCast<void(__thiscall*)(void*)>(vt[0x1C / 4])(PED_hud);
    }
}

void CPed::GiveMoney(uint32_t amount) {
    amount -= amount % 25;
    uint32_t total = money + amount;
    if (total > kMoneyCap) {
        money = kMoneyCap;
        return;
    }
    money = total;
    if (pedType == kPedTypePlayer) {
        PED_IncrementStat(0, static_cast<int>(amount));
    }
}

// ---------------------------------------------------------------------------------
// Stats

int32_t CPed::GetStat(int stat) {
    int32_t value = stats[stat];
    if (stat == 6) {
        int16_t model = At<int16_t>(weapon, 0x10E);
        if (model != -1) {
            for (uintptr_t address : kStat6Weapons) {
                if (model == *reinterpret_cast<int32_t*>(address)) {
                    return 0;
                }
            }
        }
    }
    return value;
}

void CPed::SetStat(int stat, int value) {
    stats[stat] = value;
    switch (stat) {
    case 2:
        unknownA68 = static_cast<float>(value * 3.1415927410125732 / 180.0);
        break;
    case 3:
        unknownA6C = static_cast<float>(value);
        break;
    case 4: {
        float max = static_cast<float>(value);
        maxHealth = max;
        if (health > max) {
            health = max;
            unknown1C98 = 0;
            unknown1CBC = max;
        }
        break;
    }
    case 5:
        healthRegen = static_cast<float>(value);
        break;
    case 0x10:
        unknown18D0 = static_cast<float>(value);
        break;
    case 0x16:
        unknown18C0 = static_cast<float>(value);
        break;
    case 0x17:
        unknown18C8 = value;
        break;
    default:
        break;
    }
}

// ---------------------------------------------------------------------------------
// Health

void CPed::RegenerateHealth() {
    if (0.0f < health && maxHealth >= health && maxHealth > health) {
        float step = PED_stepSeconds * PED_timeStep;
        float h = static_cast<float>(step * (healthRegen / 100.0) + health);
        health = h;
        if (maxHealth < h) {
            health = maxHealth;
        }
    }
}

void CPed::AddHealth(float amount) {
    if (!unknown1C98 && maxHealth >= health) {
        float h = amount + health;
        health = h;
        if (maxHealth < h) {
            health = maxHealth;
        }
    }
}

// ---------------------------------------------------------------------------------
// Punishment

void CPed::SetPunishment(int points) {
    if (IgnoresPunishment(this)) {
        return;
    }
    punishment = points;
    if (points <= 0) {
        punishment = 0;
    }
    if (pedType == kPedTypePlayer) {
        PED_ResetPunishment(PED_player);
        int more = points - punishment;
        if (more > 0) {
            punishment += more;
        }
    }
    uint32_t delay = points > At<int32_t>(PED_punishButes, 0x34) ? 30000 : 5000;
    punishmentTime = PED_timeInMs + delay;
}

void CPed::AddPunishment(int points) {
    if (IgnoresPunishment(this)) {
        return;
    }
    if (pedType == kPedTypePlayer && points > 0) {
        PED_IncrementStat(0x9A, points);
    }
    if (points >= At<int32_t>(PED_aiButes, 0x90)) {
        unknown137C = 0;
        unknown1D44 = 0;
    } else {
        PED_Update1D44(this);
    }
    punishment += points;
    if (punishment < 0) {
        punishment = 0;
    }
    PED_SetPunishment(this, punishment);
}

void CPed::UpdatePunishment() {
    int drop = At<int32_t>(PED_aiButes, 0x94);
    if (pedType == kPedTypePlayer && (PED_player->unknown1E91 || PED_currArea == 14)) {
        drop *= At<int32_t>(PED_aiButes, 0x98);
        punishmentTime = 0;
    }
    bool seen = false;
    if (unknown14A8) {
        // (still seen by an authority figure: the points don't drop)
        unknown14A8 = 0;
        CPool<CPed>* pool = CPools::ms_pPedPool;
        for (int slot = pool->size; slot != 0;) {
            slot--;
            if (pool->flags[slot] & 0x80) {
                continue;
            }
            CPed* other = reinterpret_cast<CPed*>(Bytes(pool->objects) + pool->elemSize * slot);
            if (!other) {
                continue;
            }
            int kind = other->pedType;
            if ((kind == 0 || kind == 8 || kind == 7) && PED_Fn48D470(other->unknownA64, this)) {
                unknown14A8 = 1;
                break;
            }
        }
        seen = unknown14A8 != 0;
    }
    if (!seen && punishment > 0 && punishmentTime < PED_timeInMs) {
        if (pedType == kPedTypePlayer) {
            PED_SubPunishment(PED_player, drop);
        } else {
            punishment -= drop;
        }
        if (punishment < 0) {
            punishment = 0;
        }
    }
    if (unknown137C && punishment <= At<int32_t>(PED_aiButes, 0x84)) {
        unknown137C = 0;
    }
}

void CPed::Update1D44() {
    int max = At<int32_t>(PED_aiButes, 0x8C);
    if (unknown137C) {
        unknown1D44 = static_cast<int8_t>(unknown1D44 + 1);
        if (unknown137C && unknown1D44 > max) {
            unknown137C = 0;
        }
    }
    if (unknown1D44 < 0 || unknown1D44 > max) {
        unknown1D44 = 0;
    }
    if (unknown1D44 < 0) {
        unknown1D44 = 0;
    }
}

void CPed::SetPunishmentTime(int msFromNow) {
    punishmentTime = PED_timeInMs + msFromNow;
}

void CPed::SubPunishment(int points) {
    punishment -= points;
}

void CPed::ResetPunishment() {
    punishment = 0;
    minPunishment = 0;
}

void CPed::PlayerUpdatePunishment() {
    PED_UpdatePunishment(this);
    if (static_cast<uint32_t>(punishment) < static_cast<uint32_t>(minPunishment)) {
        punishment = minPunishment;
    }
}

float CPed::Player49AD40() {
    int32_t left = static_cast<int32_t>(unknown1F40 - PED_timeInMs);
    // (as unsigned)
    double value = static_cast<double>(left);
    if (left < 0) {
        value += 4294967296.0;
    }
    float f = static_cast<float>(value / PED_divisorA35054);
    if (f < 0.0f) {
        return 0.0f;
    }
    if (1.0f < f) {
        return 1.0f;
    }
    return f;
}

// ---------------------------------------------------------------------------------
// Gifts

uint8_t CPed::WantsGift() {
    if (unknown1E27) {
        return 0;
    }
    if (unknown1E28) {
        return 1;
    }
    if (seatState == 2) {
        return 0;
    }
    int index = giftIndex;
    if (index == -1 || PED_Is1D24Kind3(this)) {
        return 0;
    }
    int16_t requirement = GiftRequirement(index);
    if (requirement == 0) {
        return 0;
    }
    if (requirement < 0x16) {
        return 1;
    }
    return PED_GiftMgr6AA7E0(PED_giftMgr) ? 0 : 1;
}

int* CPed::GetGift(int* out) {
    if (unknown1E28) {
        *out = unknown1E28;
        return out;
    }
    if (seatState != 2 && giftIndex != -1 && !(unknown1D24 && At<int32_t>(unknown1D24, 0x70) == 3) &&
        !unknown14CC) {
        if (GiftRequirement(giftIndex) < 0x16 || !PED_GiftMgr6AA7E0(PED_giftMgr)) {
            *out = GiftRequirement(giftIndex);
            return out;
        }
    }
    *out = 0;
    return out;
}

void CPed::Set1E28(int value) {
    unknown1E28 = value;
}

void CPed::SetGiftIndex(int index) {
    if (giftIndex == -1 && index != -1 && GiftRequirement(index) == 0) {
        PED_giftRequests[index]++;
    }
    giftIndex = index;
}

uint8_t CPed::Has0GiftRequirement() {
    return seatState != 2 && giftIndex != -1 && !(unknown1D24 && At<int32_t>(unknown1D24, 0x70) == 3) &&
           !unknown14CC && GiftRequirement(giftIndex) == 0;
}

void CPed::Set1E26(int value) {
    unknown1E26 = static_cast<uint8_t>(value);
}

void CPed::Set1E27(int value) {
    uint8_t v = static_cast<uint8_t>(value);
    unknown1E27 = v;
    if (v == 1 && giftIndex != -1) {
        PED_giftRequests[giftIndex]++;
        CPool<CPed>* pool = CPools::ms_pPedPool;
        int slot = static_cast<int>((Bytes(this) - Bytes(pool->objects)) / pool->elemSize);
        PED_RadarAddBlip(PED_radar, 2, (slot << 8) + pool->flags[slot]);
    }
}

// ---------------------------------------------------------------------------------
// Accessors and the rest

void CPed::Push1D24(uint8_t* value) {
    if (value) {
        unknown1D28 = unknown1D24;
        unknown1D24 = value;
    }
}

uint8_t CPed::Is1D24Kind3() {
    return (unknown1D24 && At<int32_t>(unknown1D24, 0x70) == 3) || unknown14CC != 0;
}

uint8_t CPed::Clear1D24(int notify) {
    uint8_t cleared = 0;
    if (uint8_t* old = unknown1D24) {
        unknown1D24 = nullptr;
        if (re::Flag(notify)) {
            PED_Notify4475D0(old, this);
        }
        cleared = 1;
    }
    if (uint8_t* old = unknown1D28) {
        unknown1D28 = nullptr;
        if (re::Flag(notify)) {
            PED_Notify4475D0(old, this);
        }
        cleared = 1;
    }
    return cleared;
}

void CPed::SetAlpha(int value, int flag) {
    PED_SetNodeAlpha(rwObject, value);
    alpha = static_cast<uint8_t>(value);
    unknown1384 = static_cast<uint8_t>(flag);
}

void CPed::SetDesiredHeading(float value) {
    desiredHeading = value;
}

void CPed::SetHeading(float value) {
    desiredHeading = value;
    if (matrix) {
        PED_MatrixSetRotateZ(matrix, value);
    }
    heading = value;
    ped::CVector forward;
    PED_GetForward(this, &forward);
    *re::PtrCast<ped::CVector*>(intelligence) = forward;
    *reinterpret_cast<ped::CVector*>(intelligence + 0xC) = forward;
}

void CPed::UpdateMoveDirection() {
    ped::CVector forward;
    PED_GetForward(this, &forward);
    forward.z = 0.0f;
    PED_VectorNormalise(&forward);
    *re::PtrCast<ped::CVector*>(intelligence) = forward;
}

uint8_t* CPed::Get1630() {
    return unknown1630;
}

ped::CVector* CPed::GetRight(ped::CVector* out) {
    if (matrix) {
        *out = *re::PtrCast<ped::CVector*>(matrix);
        return out;
    }
    PED_PlacementGetRight(&position, out);
    return out;
}

ped::CVector* CPed::GetPosition(ped::CVector* out, int) {
    *out = *PosOf(this);
    return out;   // (returns the output pointer, as the game's eax does: 2026-10-30)
}

float CPed::DistanceToPlayer() {
    ped::CVector d;
    PED_VectorSub(&d, PosOf(PED_player), PosOf(this));
    float sq = d.y * d.y + d.x * d.x + d.z * d.z;
    return static_cast<float>(std::sqrt(std::fabs(sq)));
}

void CPed::SetAttitude(int otherType, int attitude) {
    if (attitudeSet) {
        re::Ptr32<void>* vt = *re::PtrCast<re::Ptr32<re::Ptr32<void>>*>(attitudeSet);
        re::PtrCast<void(__thiscall*)(void*, int, int)>(vt[1])(attitudeSet, otherType, attitude);
    }
}

void CPed::ResetAttitudes() {
    PED_AttitudeReset(attitudeSet);
}

int* CPed::GetAttitude(int* out, int otherType) {
    if (!attitudeSet) {
        *out = 2;
        return out;
    }
    if (!(PED_attitudeGuard & 1)) {
        PED_attitudeGuard |= 1;
    }
    int tmp;
    re::Ptr32<void>* vt = *re::PtrCast<re::Ptr32<re::Ptr32<void>>*>(attitudeSet);
    int* got = re::PtrCast<int*(__thiscall*)(void*, int*, int)>(vt[0])(attitudeSet, &tmp, otherType);
    PED_attitudeLast = *got;
    if (PED_attitudeLast == 5) {
        // (5: the ped type's default)
        int tmp2;
        PED_attitudeLast = *PED_AttitudeDefault(PED_attitudeDefaults, &tmp2, pedType, otherType);
    }
    *out = PED_attitudeLast;
    return out;
}

int* CPed::GetAttitudeTo(int* out, CPed* other) {
    PED_GetAttitude(this, out, other->pedType);
    return out;
}

uint8_t CPed::Has1470OrWeapon4330() {
    return unknown1470 != 0 || At<int16_t>(weapon, 0x10E) == PED_miWeapon4330;
}

void CPed::Vf24() {
    unknown78 = 1;
    if (vehicle) {
        PED_Vehicle4CE960(Bytes(vehicle) + 0x1C4);
    }
}

uint32_t CPed::Get473F80() {
    if (unknown131C) {
        return PED_valueCF3130;
    }
    return unknown1320 ? PED_valueCF3134 : PED_valueCF312C;
}

uint8_t CPed::Is473F10() {
    uint32_t bits = 0;
    PED_Player49BBD0(PED_player, modelIndex, &bits);
    if (PED_PlayerInfo4466F0(PED_playerInfo, 0x1B) && (bits & PED_PlayerInfo4466F0(PED_playerInfo, 0x1B))) {
        return 1;
    }
    return 0;
}

void CPed::Set18C4(float value) {
    unknown18C4 = value;
}

// (0 .. 5, truncated to int by the CRT's _ftol)
static int ClampTo5(float f) {
    if (f < 0.0f) {
        f = 0.0f;
    } else if (5.0f < f) {
        f = 5.0f;
    }
    return static_cast<int>(f);
}

void CPed::Set18C0And18C8(int value) {
    int hundreds = value / 200;
    unknown18C0 = static_cast<float>(value - hundreds * 200);
    unknown18C8 = ClampTo5(static_cast<float>(hundreds));
}

void CPed::Add18C8(int value) {
    unknown18C8 += value;
    unknown18C8 = ClampTo5(static_cast<float>(unknown18C8));
}

void CPed::Set18D4(float value) {
    unknown18D4 = value;
}

void CPed::Set1559(int value, int notify) {
    unknown1559 = static_cast<uint8_t>(value);
    if (!re::Flag(value) || !partner) {
        return;
    }
    if (re::Flag(notify)) {
        if (!unknown1348) {
            PED_Fn5A59D0(PED_objCF4378, this, 1, 0x19);
        }
        if (unknown143C) {
            PED_Fn5924E0(PED_vehicleSomething, this, 1);
        }
    }
    partner->unknown1559 = 0;
}

float CPed::Vf28() {
    return PED_floatCF311C + PED_floatCF311C;
}

float CPed::Vf29() {
    if (!inVehicleSeat) {
        return PED_floatCF311C;
    }
    re::Ptr32<void>* vt = *re::PtrCast<re::Ptr32<re::Ptr32<void>>*>(vehicle);
    float value = re::PtrCast<float(__thiscall*)(CVehicle*)>(vt[0x70 / 4])(vehicle);
    return static_cast<float>(value * 0.5);
}

uint8_t CPed::Vf21() {
    return unknown1624 != 0 || unknown148C != 0;
}

void CPed::Set1E0C(float seconds) {
    unknown1E0C = NowSeconds() + seconds;
}

uint8_t CPed::Is1E0CAhead() {
    return unknown1E0C > NowSeconds();
}

void CPed::Set1E18(const ped::CVector* value) {
    unknown1E14 = 1;
    unknown1E18 = *value;
}

uint8_t* CPed::GetIntelligence300() {
    return intelligence + 0x300;
}

uint8_t* CPed::GetIntelligence408() {
    return intelligence + 0x408;
}

uint8_t CPed::HasWeapon() {
    return At<int16_t>(weapon, 0x10E) != -1;
}

void CPed::SetFlags1E24(int set, int second) {
    if (re::Flag(set)) {
        if (re::Flag(second)) {
            flags1E24 |= 2;
            return;
        }
        flags1E24 |= 1;
    } else {
        if (re::Flag(second)) {
            flags1E24 &= 0xFD;
            return;
        }
        flags1E24 &= 0xFE;
    }
    unknown15E8[0] = 1000.0f;
    unknown15E8[1] = 1000.0f;
    unknown15E8[2] = 1000.0f;
}

void CPed::SetVehicle(CVehicle* v) {
    if (v) {
        if (lastVehicle) {
            PED_CleanUpOldReference(lastVehicle, &lastVehicle);
        }
        lastVehicle = v;
        PED_RegisterReference(v, &lastVehicle);
    }
    vehicle = v;
    if (v) {
        PED_RegisterReference(v, &vehicle);
        CEntity* entity = reinterpret_cast<CEntity*>(v);
        if (unknown430 != entity) {
            if (unknown430) {
                PED_CleanUpOldReference(unknown430, &unknown430);
            }
            unknown430 = entity;
            PED_RegisterReference(entity, &unknown430);
        }
    }
}

void CPed::EnterVehicle(CVehicle* v) {
    uint8_t* ai = intelligence + 0x408;
    ai[0x68] = 0;
    *reinterpret_cast<int32_t*>(ai + 0x64) = 0;
    ai[0x34] = 0;
    PED_SetVehicle(this, v);
    unknown132C = 1;
    Objective* current = PED_QueueGetCurrent(&objectives);
    if (current && current->type == 5) {
        PED_QueueRemoveFirst(&objectives);
    }
}

uint8_t CPed::Is476200(CEntity* entity) {
    if (unknown1E25) {
        return 1;
    }
    if ((At<uint8_t>(entity, 0x108) & 7) != 3) {
        return 0;
    }
    CPed* other = reinterpret_cast<CPed*>(entity);
    if (other == this || unknown1520 || unknown1C98 || !unknown133C || other->unknown1E25 || other->unknown1C98 ||
        !other->unknown133C || other->vehicle || other == PED_pedC2B118) {
        return 1;
    }
    return partner && !unknown1500 && other->partner && !other->unknown1500 && partner == other &&
           other->partner == this;
}

uint8_t CPed::IsInVehicle(CEntity* entity) {
    if (re::PtrCast<CEntity*>(vehicle) == entity) {
        return 1;
    }
    return entity && (At<uint8_t>(entity, 0x108) & 7) == 2 && At<CPed*>(entity, 0x450) == this &&
           At<int8_t>(entity, 0x454) > 0;
}

uint8_t CPed::Is4765C0() {
    if (vehicle || pedType == kPedTypePlayer) {
        return 0;
    }
    return seatState != 2 && seatState != 3;
}

uint8_t CPed::Is476600() {
    if (pedType == kPedTypePlayer) {
        return 0;
    }
    return seatState != 2 && seatState != 3 && seatState != 4;
}

void CPed::RecordPosition() {
    const ped::CVector* pos = PosOf(this);
    if (historyCount == 4) {
        history[historyIndex] = *pos;
        historyIndex = (historyIndex + 1) % 4;
        return;
    }
    history[historyCount] = *pos;
    historyCount++;
    if (historyCount == 4) {
        historyIndex = 0;
    }
}

void CPed::ResetPositionHistory() {
    historyIndex = -1;
    historyCount = 0;
}

uint8_t CPed::HasntMoved() {
    if (historyIndex != 3) {
        return 0;
    }
    ped::CVector d;
    PED_VectorSub(&d, &history[3], &history[0]);
    float sq = d.x * d.x + d.y * d.y;
    float length = static_cast<float>(std::sqrt(std::fabs(sq)));
    return length < static_cast<double>(0.2f);
}

void* CPed::GetModelInfo() {
    return PED_modelInfoPtrs[modelIndex];
}

CEntity* CPed::GetEntity(int kind, int* out) {
    switch (kind) {
    case 0:
        return PED_GetTarget(this);
    case 1:
        return re::PtrCast<CEntity*>(partner);
    case 2:
        return PED_Fn48C070(unknown3BC);
    case 3:
        return unknown430;
    case 4:
        if (out) {
            *out = entityRef438.value;
        }
        return entityRef438.entity;
    case 6: {
        // what the current objective (a pickup one) is after
        Objective* current = PED_QueueGetCurrent(&objectives);
        if (current->type != 0x17) {
            return nullptr;
        }
        Objective* pickup = PED_AsObjective17(current);
        if (!pickup) {
            return nullptr;
        }
        int index = PED_FindPickup(At<int32_t>(pickup, 0x10));
        if (index == -1 || PickupState(index) != 1) {
            return nullptr;
        }
        return PickupEntity(index);
    }
    case 7:
        return unknown544;
    case 8:
        return PED_Targeting4D7780(targeting);
    case 9:
        return re::PtrCast<CEntity*>(unknown4F0[unknown504]);
    case 10:
        return PED_Targeting4D62C0(targeting);
    case 12:
        return unknownD70;
    case 13: {
        // (the game passes the argument's own slot)
        int slot = kind;
        return PED_Fn496CB0(this, &slot);
    }
    case 15:
        return unknown11EC;
    case 16:
        if (PED_Chain485330(&unknown2F0)) {
            return re::PtrCast<CEntity*>(unknown2F8);
        }
        if (PED_Chain485320(&unknown2F0)) {
            return re::PtrCast<CEntity*>(unknown2F4);
        }
        return nullptr;
    default:
        return nullptr;
    }
}

CEntity* CPed::GetTarget() {
    if (pedType != kPedTypePlayer) {
        if (unknownBB0 && unknownBB4 && (!unknown1DBC || !(5.0f > unknown1DC0))) {
            return unknownBB4;
        }
        if (unknown538) {
            return unknown538;
        }
    }
    if (!PED_Targeting4D5AD0(targeting)) {
        return nullptr;
    }
    return PED_Targeting4D5AD0(targeting);
}

float CPed::Get478180(float value) {
    if (value < 0.0f) {
        if (PED_InVehicle1558(this)) {
            return static_cast<float>(PED_GetStat(this, 0x18) / 100.0);
        }
        value = 1.1f;
    }
    return value;
}

void CPed::Set1570(int state, int a, int b) {
    PED_Set1570(this, state, 10, a, b);
}

uint8_t CPed::Is47B940(float a, int b) {
    return PED_Fn47B800(this, a, b) != 0;
}

// A vehicle a ped can have a partner in: none, one with +0x46C set, or of kind 6 or 7
static bool PartnerVehicleOk(CVehicle* vehicle) {
    if (!vehicle || At<int32_t>(vehicle, 0x46C)) {
        return true;
    }
    int kind;
    if (*PED_VehicleKind(vehicle, &kind) == 7) {
        return true;
    }
    return *PED_VehicleKind(vehicle, &kind) == 6;
}

void CPed::SetPartner(CPed* other) {
    if (partner) {
        PED_CleanUpOldReference(partner, &partner);
    }
    if (!PartnerVehicleOk(vehicle) || (other && !PartnerVehicleOk(other->vehicle))) {
        partner = nullptr;
        return;
    }
    partner = other;
    if (other) {
        PED_RegisterReference(other, &partner);
    }
}

void CPed::ClearPartner() {
    if (unknown161C) {
        PED_AnimMgr6C2200(PED_animMgr, animBlend, unknown161C);
        unknown161C = 0;
    }
    if (partner) {
        PED_CleanUpOldReference(partner, &partner);
    }
    if (vehicle && !At<int32_t>(vehicle, 0x46C)) {
        // (the kind asked twice, neither answer used)
        int kind;
        if (*PED_VehicleKind(vehicle, &kind) != 7) {
            PED_VehicleKind(vehicle, &kind);
        }
    }
    partner = nullptr;
    unknown1500 = 0;
    PED_Targeting4D7800(targeting);
    if (!unknown548) {
        PED_Fn72EEF0(unknown2EC);
    }
}

void CPed::SetTargetEntity(CEntity* target) {
    ped::CVector offset = {0.0f, 0.0f, 0.0f};
    PED_SetTarget497E70(this, target, 0, &offset);
}

void CPed::Intelligence484E60() {
    PED_Intel484E60(intelligence, this);
}
void CPed::Intelligence4843E0(int a, int b) {
    PED_Intel4843E0(intelligence, a, b);
}
void CPed::Intelligence4849B0(int a, int b) {
    PED_Intel4849B0(intelligence, a, b);
}
uint8_t CPed::Intelligence484A70(int a, int b) {
    return PED_Intel484A70(intelligence, a, b);
}
uint8_t CPed::Intelligence484AF0(int a, int b) {
    return PED_Intel484AF0(intelligence, a, b);
}
void CPed::Intelligence484480() {
    PED_Intel484480(intelligence);
}

// ---------------------------------------------------------------------------------
// Blended anims

void CPed::AddBlend(int id) {
    for (PedBlendSlot& slot : blendSlots) {
        if (slot.id == id) {
            return;
        }
    }
    for (PedBlendSlot& slot : blendSlots) {
        if (slot.id == -1) {
            void* clump = Clump(this);
            slot.time = 20.0f;
            slot.id = id;
            PED_AnimAdd(PED_animMgr, clump, id);
            return;
        }
    }
}

void CPed::RemoveBlend(int id) {
    for (PedBlendSlot& slot : blendSlots) {
        if (slot.id == id) {
            PED_AnimRemove(PED_animMgr, Clump(this), slot.id, 0);
            slot.time = 0.0f;
            slot.id = -1;
            return;
        }
    }
}

void CPed::ClearBlends() {
    for (PedBlendSlot& slot : blendSlots) {
        if (slot.id != -1) {
            void* anim = At<void*>(animBlend, 0x94);
            if (anim && At<void*>(anim, 0x10)) {
                PED_ClumpStopAnim(At<void*>(anim, 0x10), slot.id, 0);
            }
            slot.id = -1;
            slot.time = 0.0f;
        }
    }
}

void CPed::UpdateBlends() {
    for (PedBlendSlot& slot : blendSlots) {
        if (slot.id == -1 || slot.time == 0.0f) {
            continue;
        }
        float step = PED_stepSeconds * PED_timeStep;
        float t = slot.time - step;
        if (t < 0.0f) {
            t = 0.0f;
        }
        slot.time = t;
        if (t == 0.0f) {
            PED_AnimRemove(PED_animMgr, Clump(this), slot.id, 0);
            slot.time = 0.0f;
            slot.id = -1;
        }
    }
}

// ---------------------------------------------------------------------------------

#define PED_REVERSED(name, address, fn) \
    REVERSED(name, address, re::MemberAddress(fn), nullptr, nullptr, re::Mode::Original)
#define PED_REVERSED_STATIC(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PED_REVERSED_STATIC("CPed::Distance2D", 0x00470110, &CPed::Distance2D);
PED_REVERSED_STATIC("CPed::AsVehicle", 0x00474EE0, &CPed::AsVehicle);
PED_REVERSED_STATIC("CPed::IsPedWith1580", 0x00477D90, &CPed::IsPedWith1580);
PED_REVERSED("CPed::CanAfford", 0x004730E0, &CPed::CanAfford);
PED_REVERSED("CPed::TakeMoney", 0x00473100, &CPed::TakeMoney);
PED_REVERSED("CPed::SetMoney", 0x004752A0, &CPed::SetMoney);
PED_REVERSED("CPed::GiveMoney", 0x00475320, &CPed::GiveMoney);
PED_REVERSED("CPed::GetStat", 0x00477060, &CPed::GetStat);
PED_REVERSED("CPed::SetStat", 0x004770F0, &CPed::SetStat);
PED_REVERSED("CPed::RegenerateHealth", 0x00473140, &CPed::RegenerateHealth);
PED_REVERSED("CPed::AddHealth", 0x00474CC0, &CPed::AddHealth);
PED_REVERSED("CPed::SetPunishment", 0x00477520, &CPed::SetPunishment);
PED_REVERSED("CPed::AddPunishment", 0x004775D0, &CPed::AddPunishment);
PED_REVERSED("CPed::UpdatePunishment", 0x00477680, &CPed::UpdatePunishment);
PED_REVERSED("CPed::Update1D44", 0x004745D0, &CPed::Update1D44);
PED_REVERSED("CPed::SetPunishmentTime", 0x004745B0, &CPed::SetPunishmentTime);
PED_REVERSED("CPlayerPed::SubPunishment", 0x0049A830, &CPed::SubPunishment);
PED_REVERSED("CPlayerPed::ResetPunishment", 0x0049A840, &CPed::ResetPunishment);
PED_REVERSED("CPlayerPed::UpdatePunishment", 0x0049A7F0, &CPed::PlayerUpdatePunishment);
PED_REVERSED("CPlayerPed::Get49AD40", 0x0049AD40, &CPed::Player49AD40);
PED_REVERSED("CPed::WantsGift", 0x00474A30, &CPed::WantsGift);
PED_REVERSED("CPed::GetGift", 0x00474AA0, &CPed::GetGift);
PED_REVERSED("CPed::Set1E28", 0x00474B30, &CPed::Set1E28);
PED_REVERSED("CPed::SetGiftIndex", 0x00474B40, &CPed::SetGiftIndex);
PED_REVERSED("CPed::Has0GiftRequirement", 0x00474B80, &CPed::Has0GiftRequirement);
PED_REVERSED("CPed::Set1E26", 0x00474A90, &CPed::Set1E26);
PED_REVERSED("CPed::Set1E27", 0x0047D1B0, &CPed::Set1E27);
PED_REVERSED("CPed::Push1D24", 0x00473200, &CPed::Push1D24);
PED_REVERSED("CPed::Is1D24Kind3", 0x00473220, &CPed::Is1D24Kind3);
PED_REVERSED("CPed::Clear1D24", 0x00473250, &CPed::Clear1D24);
PED_REVERSED("CPed::SetAlpha", 0x004732A0, &CPed::SetAlpha);
PED_REVERSED("CPed::SetDesiredHeading", 0x004734C0, &CPed::SetDesiredHeading);
PED_REVERSED("CPed::SetHeading", 0x00475730, &CPed::SetHeading);
PED_REVERSED("CPed::UpdateMoveDirection", 0x00474570, &CPed::UpdateMoveDirection);
PED_REVERSED("CPed::Get1630", 0x004734D0, &CPed::Get1630);
PED_REVERSED("CPed::GetRight", 0x004736F0, &CPed::GetRight);
PED_REVERSED("CPed::GetPosition", 0x00474870, &CPed::GetPosition);
PED_REVERSED("CPed::DistanceToPlayer", 0x004777B0, &CPed::DistanceToPlayer);
PED_REVERSED("CPed::SetAttitude", 0x004731D0, &CPed::SetAttitude);
PED_REVERSED("CPed::ResetAttitudes", 0x004731F0, &CPed::ResetAttitudes);
PED_REVERSED("CPed::GetAttitude", 0x00475380, &CPed::GetAttitude);
PED_REVERSED("CPed::GetAttitudeTo", 0x00475400, &CPed::GetAttitudeTo);
PED_REVERSED("CPed::Has1470OrWeapon4330", 0x00473E10, &CPed::Has1470OrWeapon4330);
PED_REVERSED("CPed::Vf24", 0x00473F60, &CPed::Vf24);
PED_REVERSED("CPed::Get473F80", 0x00473F80, &CPed::Get473F80);
PED_REVERSED("CPed::Is473F10", 0x00473F10, &CPed::Is473F10);
PED_REVERSED("CPed::Set18C4", 0x00474340, &CPed::Set18C4);
PED_REVERSED("CPed::Set18C0And18C8", 0x00474350, &CPed::Set18C0And18C8);
PED_REVERSED("CPed::Add18C8", 0x004743D0, &CPed::Add18C8);
PED_REVERSED("CPed::Set18D4", 0x00474430, &CPed::Set18D4);
PED_REVERSED("CPed::Set1559", 0x00474440, &CPed::Set1559);
PED_REVERSED("CPed::Vf28", 0x00474920, &CPed::Vf28);
PED_REVERSED("CPed::Vf29", 0x00474940, &CPed::Vf29);
PED_REVERSED("CPed::Vf21", 0x00474970, &CPed::Vf21);
PED_REVERSED("CPed::Set1E0C", 0x00474990, &CPed::Set1E0C);
PED_REVERSED("CPed::Is1E0CAhead", 0x004749C0, &CPed::Is1E0CAhead);
PED_REVERSED("CPed::Set1E18", 0x00474A00, &CPed::Set1E18);
PED_REVERSED("CPed::GetIntelligence300", 0x00474BD0, &CPed::GetIntelligence300);
PED_REVERSED("CPed::GetIntelligence408", 0x00474BE0, &CPed::GetIntelligence408);
PED_REVERSED("CPed::HasWeapon", 0x00474BF0, &CPed::HasWeapon);
PED_REVERSED("CPed::SetFlags1E24", 0x00474C10, &CPed::SetFlags1E24);
PED_REVERSED("CPed::SetVehicle", 0x00476180, &CPed::SetVehicle);
PED_REVERSED("CPed::EnterVehicle", 0x00476640, &CPed::EnterVehicle);
PED_REVERSED("CPed::Is476200", 0x00476200, &CPed::Is476200);
PED_REVERSED("CPed::IsInVehicle", 0x004762B0, &CPed::IsInVehicle);
PED_REVERSED("CPed::Is4765C0", 0x004765C0, &CPed::Is4765C0);
PED_REVERSED("CPed::Is476600", 0x00476600, &CPed::Is476600);
PED_REVERSED("CPed::RecordPosition", 0x00477AC0, &CPed::RecordPosition);
PED_REVERSED("CPed::ResetPositionHistory", 0x00477B70, &CPed::ResetPositionHistory);
PED_REVERSED("CPed::HasntMoved", 0x00477B90, &CPed::HasntMoved);
PED_REVERSED("CPed::GetModelInfo", 0x00477DC0, &CPed::GetModelInfo);
PED_REVERSED("CPed::GetEntity", 0x00477C00, &CPed::GetEntity);
PED_REVERSED("CPed::GetTarget", 0x00497100, &CPed::GetTarget);
PED_REVERSED("CPed::Get478180", 0x00478180, &CPed::Get478180);
PED_REVERSED("CPed::Set1570", 0x0047B7E0, &CPed::Set1570);
PED_REVERSED("CPed::Is47B940", 0x0047B940, &CPed::Is47B940);
PED_REVERSED("CPed::AddBlend", 0x004739E0, &CPed::AddBlend);
PED_REVERSED("CPed::RemoveBlend", 0x00473A50, &CPed::RemoveBlend);
PED_REVERSED("CPed::ClearBlends", 0x00473AC0, &CPed::ClearBlends);
PED_REVERSED("CPed::UpdateBlends", 0x00473B10, &CPed::UpdateBlends);
PED_REVERSED_STATIC("CPed::NormalisedCopy", 0x00472D40, &CPed::NormalisedCopy);
PED_REVERSED_STATIC("CPed::DistanceSquared", 0x00472D00, &CPed::DistanceSquared);
PED_REVERSED("CPed::SetPartner", 0x004744B0, &CPed::SetPartner);
PED_REVERSED("CPed::ClearPartner", 0x00477470, &CPed::ClearPartner);
PED_REVERSED("CPed::SetTargetEntity", 0x00498B10, &CPed::SetTargetEntity);
PED_REVERSED("CPed::Intelligence484E60", 0x00474090, &CPed::Intelligence484E60);
PED_REVERSED("CPed::Intelligence4843E0", 0x004740C0, &CPed::Intelligence4843E0);
PED_REVERSED("CPed::Intelligence4849B0", 0x004740D0, &CPed::Intelligence4849B0);
PED_REVERSED("CPed::Intelligence484A70", 0x004740E0, &CPed::Intelligence484A70);
PED_REVERSED("CPed::Intelligence484AF0", 0x004740F0, &CPed::Intelligence484AF0);
PED_REVERSED("CPed::Intelligence484480", 0x00474110, &CPed::Intelligence484480);
