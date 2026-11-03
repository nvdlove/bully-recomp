// The action tree's conditions about gifts, stimuli, bikes, allies, targets, guarding
// and getting on / off vehicles: each one's Evaluate(owner) (vf1, thiscall).
// 0x6040F0..0x605570.

#include <cstdint>
#include <cstring>

#include "game/ActionTracksCommon.h"
#include "reversed.h"

using namespace actc;

namespace {

GAME_VAR(CPool<uint8_t>*, vehiclePool, 0x00C0F5F4);   // CPools::ms_pVehiclePool
GAME_VAR(uint8_t*, ignoreEntity, 0x00C1AE70);          // CWorld::pIgnoreEntity
GAME_VAR(float, pedRadius, 0x00CF311C);                // (0.35)
GAME_ARRAY(float, upVector, 0x009E33DC);               // (0, 0, 1)
GAME_VAR(int32_t, gameMode, 0x00C3CCF8);               // (1: the player's ground distance from the camera)
GAME_VAR(uint8_t*, modeCamera_C3CE98, 0x00C3CE98);
inline void* const giftList = reinterpret_cast<void*>(0x020C3698);
inline uint8_t* const giftRequirements = reinterpret_cast<uint8_t*>(0x020C369A);   // int16 each 0x14 bytes
inline void* const missionManager = reinterpret_cast<void*>(0x020C3CA0);
inline void* const conversations = reinterpret_cast<void*>(0x00B9DAB8);

GAME_FN(int32_t, __thiscall, Stimulus_GetPriority, 0x004A5CF0, uint8_t* stimulus);
GAME_FN(float, __thiscall, Vehicle_GetSlide, 0x004CB510, uint8_t* vehicle);
GAME_FN(uint32_t, __thiscall, Vehicle_GetSlideTime, 0x004CB520, uint8_t* vehicle);
GAME_FN(uint8_t, __thiscall, Bike_IsUpright, 0x005FD680, uint8_t* bike);
GAME_FN(int, __cdecl, GetModelIndexByHash, 0x0051AE20, uint32_t hash, int first, int last);
GAME_FN(uint8_t*, __thiscall, Ped_GetLastHitWeapon, 0x00474BE0, uint8_t* ped);   // +0x5E its model
GAME_FN(uint8_t, __thiscall, Ped_WantsGift, 0x00474A30, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, Targeting_GetLookTarget, 0x004D7780, uint8_t* targeting);
GAME_FN(int32_t*, __thiscall, Ped_GetGift, 0x00474AA0, uint8_t* ped, int32_t* out);
GAME_FN(uint8_t, __thiscall, Ped_HasItem, 0x0049A850, uint8_t* ped, int32_t item, int32_t count);
GAME_FN(uint8_t, __thiscall, Ped_IsStrafing, 0x00473CE0, uint8_t* ped);
GAME_FN(void, __thiscall, Ped_SetTargetEntity, 0x00498B10, uint8_t* ped, uint8_t* target);
GAME_FN(uint8_t*, __thiscall, Ped_FindByType, 0x00477C00, uint8_t* ped, int32_t type, int);
GAME_FN(int, __thiscall, Gifts_Find, 0x006A9C70, void* gifts, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Combat_CheckSemaphore, 0x004866C0, uint8_t* combat, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Combat_DetectGrenade, 0x0048A280, uint8_t* combat, V3* pos, float* radius);
GAME_FN(uint8_t, __thiscall, Combat_DetectProjectile, 0x0048A450, uint8_t* combat, V3* a, V3* b, V3* c);
GAME_FN(void, __thiscall, Ped_SetTarget, 0x00497E70, uint8_t* ped, uint8_t* target, int kind, const V3* offset);
GAME_FN(float, __cdecl, DistanceBeyond, 0x0041A4D0, const float* a, const float* b, float range);
GAME_FN(uint8_t*, __thiscall, Ally_GetTop, 0x00485560, uint8_t* ally);
GAME_FN(uint8_t, __thiscall, Conditions_Evaluate, 0x005FBB10, uint8_t* conditions, uint8_t* owner);
GAME_FN(uint8_t, __thiscall, MissionManager_Fn6AAD10, 0x006AAD10, void* manager);
GAME_FN(float, __thiscall, Bike_GetSteerLock, 0x004BCA60, uint8_t* bike);
GAME_FN(uint8_t, __thiscall, Ped_HasWeapon, 0x00474BF0, uint8_t* ped);
GAME_FN(float, __cdecl, DistanceSquaredBeyond, 0x00487E50, const float* a, const float* b, float range);
GAME_FN(V3*, __thiscall, Placeable_GetPosition, 0x00415460, uint8_t* placeable);
GAME_FN(uint8_t*, __thiscall, Ped_GetModelInfo, 0x00477DC0, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, ModelInfo_MatchesName, 0x0050E940, uint8_t* mi, const char* name);
GAME_FN(float, __cdecl, Distance, 0x0041A950, const float* a, const float* b);
GAME_FN(uint8_t, __thiscall, Conversations_IsInOne, 0x00683F40, void* conversations, uint8_t* ped);
GAME_FN(V3*, __thiscall, Ped_GetEyePosition, 0x0048D020, uint8_t* social, V3* out);   // (on the ped's +0xA64)
GAME_FN(uint8_t, __cdecl, ProcessLineOfSight, 0x00461EA0, const V3* start, const V3* end, ColPoint* colPoint,
        re::Ptr32<uint8_t>* entity, int flags, int, int);
GAME_FN(float, __thiscall, Camera_GetGroundDistance, 0x004FBFC0, uint8_t* camera);
GAME_FN(uint8_t*, __cdecl, Clump_GetHierarchy, 0x005EE1A0, void* clump);
GAME_FN(uint8_t*, __cdecl, Hierarchy_GetMatrices, 0x006C7710, uint8_t* hierarchy);
GAME_FN(float, __cdecl, FindGroundZFor3DCoord, 0x004635A0, float x, float y, float z, uint8_t* found, int flags);
GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_FN(int32_t*, __thiscall, Vehicle_GetClass, 0x004CAF30, uint8_t* vehicle, int32_t* out);
GAME_FN(V3*, __thiscall, Placement_GetRight, 0x00473600, uint8_t* placement, V3* out);
GAME_FN(V3*, __cdecl, VectorAdd, 0x004129A0, V3* out, const float* a, const float* b);
GAME_FN(uint8_t, __cdecl, ProcessSphereCast, 0x004616A0, const V3* start, const V3* end, float radius,
        ColPoint* colPoint, re::Ptr32<uint8_t>* entity, int flags, int lists, int);

uint8_t* AsPed(uint8_t* entity) {
    return entity && IsPedEntity(entity) ? entity : nullptr;
}
ColPoint EmptyColPoint() {
    ColPoint cp = {};
    cp.depth = 3.4028235e38f;
    return cp;
}

// 0x6040F0: the target ped's last dismisser (+0xC60) is the ped
uint8_t __fastcall HaveIDismissedTargetPed_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* target = Ped_GetTarget(ped);
    if (!target || !IsPedEntity(target)) {
        return 0;
    }
    return ped && ped == At<uint8_t*>(target, 0xC60) ? 1 : 0;
}

// 0x604140: the ped's stimulus's (+0x1D30) priority compared (+8) with +4
uint8_t __fastcall IsStimulusPriority_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* stimulus = At<uint8_t*>(OwnerPed(owner), 0x1D30);
    if (!stimulus) {
        return 0;
    }
    int32_t priority = Stimulus_GetPriority(stimulus);
    return CompareOp(float(priority), float(At<int32_t>(self, 4)), At<int32_t>(self, 8));
}

// 0x604190: power sliding: the bike slides (or did within 250 ms); on foot the ped's
// slide (+0x1474, +0x1D5C, within 500 ms of +0x1D60)
uint8_t __fastcall IsPowerSliding_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
    if (vehicle && At<int32_t>(vehicle, 0x46C) == 1) {
        if (!(Vehicle_GetSlide(vehicle) > 0.0f)) {
            uint32_t now = timeInMs;
            if (Vehicle_GetSlideTime(At<uint8_t*>(ped, 0x1554)) + 250 <= now) {
                return 0;
            }
        }
        return 1;
    }
    if (At<int32_t>(ped, 0x1474) == 0) {
        return 0;
    }
    if (!(At<float>(ped, 0x1D5C) > 0.0f) && At<uint32_t>(ped, 0x1D60) + 500 <= timeInMs) {
        return 0;
    }
    return 1;
}

// 0x604220: the ped's bike (+0x430) isn't locked (+0x6B8 bit 3)
uint8_t __fastcall BikeIsLocked_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* bike = At<uint8_t*>(OwnerPed(owner), 0x430);
    return bike && At<int32_t>(bike, 0x46C) == 1 && (bike[0x6B8] & 8) == 0 ? 1 : 0;
}

// 0x604250: the ped's bike stands upright
uint8_t __fastcall BikeIsUpright_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* bike = At<uint8_t*>(OwnerPed(owner), 0x430);
    return bike && At<int32_t>(bike, 0x46C) == 1 && Bike_IsUpright(bike) ? 1 : 0;
}

// 0x604280: the ped was last hit with weapon model +4 (a name hash)
uint8_t __fastcall WeaponLastHitBy_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    int model = GetModelIndexByHash(At<uint32_t>(self, 4), 0, 0x2FA7);
    return model == At<int16_t>(Ped_GetLastHitWeapon(ped), 0x5E) ? 1 : 0;
}

// 0x6042C0: the ped would take a gift
uint8_t __fastcall WillAcceptGift_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return Ped_WantsGift(OwnerPed(owner)) != 0 ? 1 : 0;
}

// 0x6042E0: the player has +4 of the gift the ped (+8: the player's look target, or the
// ped's +0x538) wants
uint8_t __fastcall PlayerHasRequiredGift_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* player = playerPed;
    if (self[8] != 0) {
        if (At<int32_t>(ped, 0x1310) == 13) {
            ped = Targeting_GetLookTarget(At<uint8_t*>(ped, 0x1C8));
            if (!ped || !IsPedEntity(ped)) {
                return 0;
            }
        } else {
            ped = AsPed(At<uint8_t*>(ped, 0x538));
        }
    }
    if (!ped) {
        return 0;
    }
    int32_t gift;
    return Ped_HasItem(player, *Ped_GetGift(ped, &gift), At<int32_t>(self, 4));
}

// 0x604370: the ped is strafing
uint8_t __fastcall IsStrafing_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return Ped_IsStrafing(OwnerPed(owner));
}

// 0x604380: the owner's attack record (+0x10: +4 the attacker, +0x2C its time, -1:
// none) is recent: the attacker (a ped) becomes the target
uint8_t __fastcall TargetAttacker_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* record = At<uint8_t*>(owner, 0x10);
    float time = At<float>(record, 0x2C);
    bool recent = time != -1.0f && time <= 1.0f;
    uint8_t* attacker = AsPed(At<uint8_t*>(record, 4));
    if (recent && attacker) {
        Ped_SetTargetEntity(OwnerPed(owner), attacker);
        return 1;
    }
    return 0;
}

// 0x6043F0: a ped of type +4 around (+8: it becomes the target)
uint8_t __fastcall TargetByType_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* found = Ped_FindByType(ped, At<int32_t>(self, 4), 0);
    if (!found) {
        return 0;
    }
    if (self[8] != 0) {
        Ped_SetTargetEntity(ped, found);
    }
    return 1;
}

// 0x604430: the gift the ped wants is +4
uint8_t __fastcall TestRequiredGiftType_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!ped) {
        return 0;
    }
    int32_t gift;
    return *Ped_GetGift(ped, &gift) == At<int32_t>(self, 4) ? 1 : 0;
}

// 0x604460: the gift the ped will want (the gift list's entry) is +4
uint8_t __fastcall TestFutureGiftType_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!ped) {
        return 0;
    }
    int i = Gifts_Find(giftList, ped);
    return i != -1 && int32_t(At<int16_t>(giftRequirements, i * 0x14)) == At<int32_t>(self, 4) ? 1 : 0;
}

// 0x6044A0: the target ped's combat semaphore lets the ped in
uint8_t __fastcall CheckTargetSemaphore_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* target = Ped_GetTarget(ped);
    if (!target || !IsPedEntity(target)) {
        return 0;
    }
    return Combat_CheckSemaphore(target + 0x668, ped) != 0 ? 1 : 0;
}

// 0x6044E0: a grenade whose blast (its radius + 1) reaches the ped
uint8_t __fastcall DetectGrenade_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    V3 pos = {0.0f, 0.0f, 0.0f};
    float radius;
    if (!Combat_DetectGrenade(ped + 0x668, &pos, &radius)) {
        return 0;
    }
    return DistanceBeyond(&pos.x, Pos(ped), radius + 1.0f) <= 0.0f ? 1 : 0;
}

// 0x604570: a projectile coming at the ped
uint8_t __fastcall DetectProjectile_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    V3 a = {0.0f, 0.0f, 0.0f};
    V3 b = {0.0f, 0.0f, 0.0f};
    V3 c = {0.0f, 0.0f, 0.0f};
    return Combat_DetectProjectile(OwnerPed(owner) + 0x668, &a, &b, &c) != 0 ? 1 : 0;
}

// 0x6045D0: the nearest vehicle within +4 (2D; +8: cars too, +9: bikes too); +0xA: it
// becomes the target
uint8_t __fastcall IsVehicleInRange_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    CPool<uint8_t>* pool = vehiclePool;
    float best = 3.4028235e38f;
    uint8_t* nearest = nullptr;
    if (pool->size == 0) {
        return 0;
    }
    for (uint32_t i = 0; i < uint32_t(pool->size); i++) {
        if (pool->flags[i] & 0x80) {
            continue;
        }
        uint8_t* vehicle = pool->objects + pool->elemSize * i;
        if (!vehicle) {
            continue;
        }
        if (self[8] == 0 && At<int32_t>(vehicle, 0x46C) == 0) {
            continue;
        }
        if (self[9] == 0 && At<int32_t>(vehicle, 0x46C) == 1) {
            continue;
        }
        V3 d;
        V3* v = VectorSub(&d, Pos(vehicle), Pos(OwnerPed(owner)));
        float dist = v->x * v->x + v->y * v->y;
        float range = At<float>(self, 4);
        if (!(dist > range * range) && best > dist) {
            best = dist;
            nearest = vehicle;
        }
    }
    if (!nearest) {
        return 0;
    }
    if (self[0xA] != 0) {
        V3 offset = {0.0f, 0.0f, 0.0f};
        Ped_SetTarget(OwnerPed(owner), nearest, 0, &offset);
    }
    return 1;
}

// 0x604710: within +4 of what the ped guards (+0x1E08)
uint8_t __fastcall InGuardRadius_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* guarded = At<uint8_t*>(ped, 0x1E08);
    if (!guarded) {
        return 0;
    }
    return DistanceBeyond(Pos(ped), Pos(guarded), At<float>(self, 4)) <= 0.0f ? 1 : 0;
}

// 0x604770: the conditions +4 hold for the ally chain's leader (its owner, vf 0x40)
uint8_t __fastcall AllyLeaderConditionGroup_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    if (!OwnerPed(owner)) {
        return 0;
    }
    uint8_t* leader = Ally_GetTop(OwnerPed(owner) + 0x2F0);
    if (!leader) {
        return 0;
    }
    uint8_t* leaderOwner = vc::Call<uint8_t*>(leader, 0x40);
    if (!leaderOwner || !At<uint8_t*>(self, 4)) {
        return 0;
    }
    return Conditions_Evaluate(At<uint8_t*>(self, 4), leaderOwner);
}

// 0x6047C0: the conditions +4 hold for the ped's leader (+0x2F8)
uint8_t __fastcall AllyFollowerConditionGroup_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* leader = At<uint8_t*>(OwnerPed(owner), 0x2F8);
    if (!leader) {
        return 0;
    }
    uint8_t* leaderOwner = vc::Call<uint8_t*>(leader, 0x40);
    if (!leaderOwner || !At<uint8_t*>(self, 4)) {
        return 0;
    }
    return Conditions_Evaluate(At<uint8_t*>(self, 4), leaderOwner);
}

// 0x604800: allies can be taken on (not for the player (+4: always the player) while the
// mission manager says no)
uint8_t __fastcall CanAcquireAlly_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = self[4] != 0 ? static_cast<uint8_t*>(playerPed) : OwnerPed(owner);
    if (ped && At<int32_t>(ped, 0x1310) == 13 && MissionManager_Fn6AAD10(missionManager)) {
        return 0;
    }
    return 1;
}

// 0x604840: the bike's steering (+0x2A0, in degrees, negated) compared (+8) with +4
uint8_t __fastcall Steering_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* vehicle = At<uint8_t*>(OwnerPed(owner), 0x1554);
    if (!vehicle) {
        return 0;
    }
    float steer = At<float>(vehicle, 0x2A0);
    float degrees = float(-steer * 180.0 / double(3.14159274f));
    return CompareOp(degrees, At<float>(self, 4), At<int32_t>(self, 8));
}

// 0x6048A0: the bike's steering as a fraction of its steering lock compared (+8) with +4
uint8_t __fastcall SteeringFraction_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* bike = At<uint8_t*>(OwnerPed(owner), 0x1554);
    if (!bike || At<int32_t>(bike, 0x46C) != 1) {
        return 0;
    }
    double steer = -double(At<float>(bike, 0x2A0));
    float lock = float(Bike_GetSteerLock(bike) * double(3.14159274f) / 180.0);
    float fraction = float(steer / lock);
    return CompareOp(fraction, At<float>(self, 4), At<int32_t>(self, 8));
}

// 0x604930: the ped's ally (+0x2F4, else its leader +0x2F8) becomes the target
uint8_t __fastcall TargetAlly_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!ped) {
        return 0;
    }
    uint8_t* ally = At<uint8_t*>(ped, 0x2F4);
    if (!ally) {
        ally = At<uint8_t*>(ped, 0x2F8);
        if (!ally) {
            return 0;
        }
    }
    Ped_SetTargetEntity(ped, ally);
    return 1;
}

// 0x604960: the target is within the ped's weapon's range (+0x1D0, its +0x120)
uint8_t __fastcall InWeaponRange_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!ped) {
        return 0;
    }
    uint8_t* target = Ped_GetTarget(ped);
    if (!target || !Ped_HasWeapon(ped)) {
        return 0;
    }
    float range = At<float>(At<uint8_t*>(ped, 0x1D0), 0x120);
    return DistanceSquaredBeyond(Pos(ped), Pos(target), range) < 0.0f ? 1 : 0;
}

// 0x6049F0: the nearest other ped within +8 (named +4 when set) becomes the target
uint8_t __fastcall TargetPed_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    CPool<uint8_t>* pool = pedPool;
    uint8_t* ped = OwnerPed(owner);
    if (!ped) {
        return 0;
    }
    float best = At<float>(self, 8);
    const char* name = At<const char*>(self, 4);
    uint8_t* nearest = nullptr;
    if (name && std::strlen(name) == 0) {
        name = nullptr;
    }
    if (pool->size <= 0) {
        return 0;
    }
    for (int i = 0; i < pool->size; i++) {
        if (pool->flags[i] & 0x80) {
            continue;
        }
        uint8_t* other = pool->objects + pool->elemSize * i;
        if (!other || other == ped) {
            continue;
        }
        V3* pos = Placeable_GetPosition(ped);
        if (!(DistanceSquaredBeyond(&pos->x, Pos(other), best) <= 0.0f)) {
            continue;
        }
        if (name && !ModelInfo_MatchesName(Ped_GetModelInfo(other), name)) {
            continue;
        }
        best = Distance(&Placeable_GetPosition(ped)->x, Pos(other));
        nearest = other;
    }
    if (!nearest) {
        return 0;
    }
    Ped_SetTargetEntity(ped, nearest);
    return 1;
}

// 0x604B20: the player's balance meter (+0x1ED8: +0x20 of half +8) compared (+8) with +4
uint8_t __fastcall BalanceMeterPosition_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (At<int32_t>(ped, 0x1310) != 13) {
        return 0;
    }
    uint8_t* meter = At<uint8_t*>(ped, 0x1ED8);
    float position = At<float>(meter, 0x20) / (At<float>(meter, 8) * 0.5f);
    return CompareOp(position, At<float>(self, 4), At<int32_t>(self, 8));
}

// 0x604B70: a target, and the ped is colliding (+0x1DE6)
uint8_t __fastcall TargetColliding_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!Ped_GetTarget(ped)) {
        return 0;
    }
    return ped[0x1DE6] != 0 ? 1 : 0;
}

// 0x604BA0: the ped is in a conversation
uint8_t __fastcall PedInConversation_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return Conversations_IsInOne(conversations, OwnerPed(owner)) != 0 ? 1 : 0;
}

// 0x604BC0 / 0x604BE0: the conversation's status (+0x1E70) / mood (+0x1E74) is +4
template <int Offset>
uint8_t __fastcall PedDwordIs_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    return At<int32_t>(OwnerPed(owner), Offset) == At<int32_t>(self, 4) ? 1 : 0;
}

// 0x604C00: nothing between the ped's eyes and the player's
uint8_t __fastcall HasLineOfSightToPlayer_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    V3 playerEyes;
    Ped_GetEyePosition(playerPed + 0xA64, &playerEyes);
    V3 eyes;
    Ped_GetEyePosition(OwnerPed(owner) + 0xA64, &eyes);
    ColPoint cp = EmptyColPoint();
    re::Ptr32<uint8_t> hit;
    uint8_t blocked = ProcessLineOfSight(&eyes, &playerEyes, &cp, &hit, 0xB, 0, 0);
    uint8_t result = blocked == 0 ? 1 : 0;
    ReleaseColPoint(cp);
    return result;
}

// 0x604CD0: the height of the ped's root bone above the ground (the camera's for the
// player in game mode 1; 1000 with no ground) compared (+8) with +4
uint8_t __fastcall PedGroundDist_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    float height = 1000.0f;
    uint8_t* ped = OwnerPed(owner);
    if (At<int32_t>(ped, 0x1310) == 13 && gameMode == 1) {
        height = Camera_GetGroundDistance(modeCamera_C3CE98);
    } else {
        uint8_t* matrices = Hierarchy_GetMatrices(Clump_GetHierarchy(At<void*>(ped, 0x18)));
        float x = At<float>(matrices, 0x128);
        float y = At<float>(matrices, 0x12C);
        float z = At<float>(matrices, 0x130);
        uint8_t found;
        float ground = FindGroundZFor3DCoord(x, y, z + 1.0f, &found, 0xB);
        if (found) {
            height = z - ground;
        }
    }
    return CompareOp(height, At<float>(self, 4), At<int32_t>(self, 8));
}

// 0x604DB0: the ped's (or its vehicle's) velocity compared (+8) with +4: +0xC its speed
// (+0xD: 3D; +0xE: along the ped's forward instead), else its vertical speed; negated
// when a vehicle moves backwards (unless +0xE)
uint8_t __fastcall PedVelocity_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
    float v[3] = {At<float>(ped, 0x124), At<float>(ped, 0x128), At<float>(ped, 0x12C)};
    if (vehicle) {
        v[0] = At<float>(vehicle, 0x124);
        v[1] = At<float>(vehicle, 0x128);
        v[2] = At<float>(vehicle, 0x12C);
    }
    float value;
    V3 forward;
    if (self[0xC] == 0) {
        value = v[2];
    } else if (self[0xD] != 0) {
        if (self[0xE] != 0) {
            V3* f = Placeable_GetForward(ped, &forward);
            value = f->y * v[1] + f->x * v[0] + f->z * v[2];
        } else {
            value = SqrtAbs(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        }
    } else if (self[0xE] != 0) {
        v[2] = 0.0f;
        V3* f = Placeable_GetForward(ped, &forward);
        value = float(double(f->y * v[1] + f->x * v[0]) + double(f->z) * 0.0);
    } else {
        value = SqrtAbs(v[0] * v[0] + v[1] * v[1]);
    }
    if (vehicle) {
        V3* f = Placeable_GetForward(vehicle, &forward);
        float along = f->y * v[1] + f->x * v[0] + f->z * v[2];
        if (along < 0.0f && self[0xE] == 0) {
            value = -value;
        }
    }
    return CompareOp(value, At<float>(self, 4), At<int32_t>(self, 8));
}

// 0x604F70 / 0x605270: room to get off the vehicle on the right / left: a ped-sized
// sphere swept from the other side (half a ped up) out past the side (1.45, 1.15 on a
// bike) hits nothing (the vehicle ignored, then the ped)
template <bool Left>
uint8_t __fastcall CanGetOffVehicleHere_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!At<uint8_t*>(ped, 0x1554)) {
        return 1;
    }
    float side = Left ? -1.45f : 1.45f;
    float up = pedRadius * 0.5f;
    if (At<int32_t>(At<uint8_t*>(ped, 0x1554), 0x46C) == 1) {
        side = Left ? -1.15f : 1.15f;
    }
    int32_t kind;
    if (*Vehicle_GetClass(At<uint8_t*>(ped, 0x1554), &kind) == 8 ||
        *Vehicle_GetClass(At<uint8_t*>(ped, 0x1554), &kind) == 3 ||
        *Vehicle_GetClass(At<uint8_t*>(ped, 0x1554), &kind) == 7) {
        up = pedRadius * 1.5f;
    }
    float lift[3] = {up * upVector[0], upVector[1] * up, up * upVector[2]};
    V3 right;
    if (float* m = At<float*>(ped, 0x14)) {
        right = {m[0], m[1], m[2]};
    } else {
        Placement_GetRight(ped + 4, &right);
    }
    float width = Left ? -pedRadius : pedRadius;
    float across[3] = {right.x * width, right.y * width, width * right.z};
    V3 t;
    V3 start;
    VectorAdd(&start, &VectorSub(&t, Pos(ped), across)->x, lift);
    float lift2[3] = {up * upVector[0], upVector[1] * up, up * upVector[2]};
    if (float* m = At<float*>(ped, 0x14)) {
        right = {m[0], m[1], m[2]};
    } else {
        Placement_GetRight(ped + 4, &right);
    }
    float out[3] = {side * right.x, right.y * side, side * right.z};
    V3 end;
    VectorAdd(&end, &VectorAdd(&t, Pos(ped), out)->x, lift2);
    ignoreEntity = At<uint8_t*>(ped, 0x1554);
    ColPoint cp = EmptyColPoint();
    re::Ptr32<uint8_t> hit;
    uint8_t blocked = ProcessSphereCast(&start, &end, pedRadius, &cp, &hit, 0xB, 0xF, 1);
    if (!blocked) {
        ignoreEntity = ped;
        blocked = ProcessSphereCast(&start, &end, pedRadius, &cp, &hit, 4, 0xF, 1);
    }
    uint8_t result = blocked == 0 ? 1 : 0;
    ignoreEntity = nullptr;
    ReleaseColPoint(cp);
    return result;
}

// 0x605570: nothing between the ped and its bike (+0x430), half a unit up
uint8_t __fastcall CanGetOnVehicleHere_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* bike = At<uint8_t*>(ped, 0x430);
    if (!bike) {
        return 1;
    }
    float lift[3] = {upVector[0] * 0.5f, upVector[1] * 0.5f, upVector[2] * 0.5f};
    V3 from;
    VectorAdd(&from, Pos(ped), lift);
    float lift2[3] = {upVector[0] * 0.5f, upVector[1] * 0.5f, upVector[2] * 0.5f};
    V3 to;
    VectorAdd(&to, Pos(bike), lift2);
    ColPoint cp = EmptyColPoint();
    re::Ptr32<uint8_t> hit;
    uint8_t blocked = ProcessLineOfSight(&from, &to, &cp, &hit, 9, 0, 0);
    uint8_t result = blocked == 0 ? 1 : 0;
    ReleaseColPoint(cp);
    return result;
}

} // namespace

#define ACTCONDWORLD_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

ACTCONDWORLD_REVERSED("ConditionHaveIDismissedTargetPed::Evaluate", 0x006040F0, &HaveIDismissedTargetPed_Evaluate);
ACTCONDWORLD_REVERSED("ConditionIsStimulusPriority::Evaluate", 0x00604140, &IsStimulusPriority_Evaluate);
ACTCONDWORLD_REVERSED("ConditionIsPowerSliding::Evaluate", 0x00604190, &IsPowerSliding_Evaluate);
ACTCONDWORLD_REVERSED("ConditionBikeIsLocked::Evaluate", 0x00604220, &BikeIsLocked_Evaluate);
ACTCONDWORLD_REVERSED("ConditionBikeIsUpright::Evaluate", 0x00604250, &BikeIsUpright_Evaluate);
ACTCONDWORLD_REVERSED("ConditionWeaponLastHitBy::Evaluate", 0x00604280, &WeaponLastHitBy_Evaluate);
ACTCONDWORLD_REVERSED("ConditionWillAcceptGift::Evaluate", 0x006042C0, &WillAcceptGift_Evaluate);
ACTCONDWORLD_REVERSED("ConditionPlayerHasRequiredGift::Evaluate", 0x006042E0, &PlayerHasRequiredGift_Evaluate);
ACTCONDWORLD_REVERSED("ConditionIsStrafing::Evaluate", 0x00604370, &IsStrafing_Evaluate);
ACTCONDWORLD_REVERSED("ConditionTargetAttacker::Evaluate", 0x00604380, &TargetAttacker_Evaluate);
ACTCONDWORLD_REVERSED("ConditionTargetByType::Evaluate", 0x006043F0, &TargetByType_Evaluate);
ACTCONDWORLD_REVERSED("ConditionTestRequiredGiftType::Evaluate", 0x00604430, &TestRequiredGiftType_Evaluate);
ACTCONDWORLD_REVERSED("ConditionTestFutureGiftType::Evaluate", 0x00604460, &TestFutureGiftType_Evaluate);
ACTCONDWORLD_REVERSED("ConditionCheckTargetSemaphore::Evaluate", 0x006044A0, &CheckTargetSemaphore_Evaluate);
ACTCONDWORLD_REVERSED("ConditionDetectGrenade::Evaluate", 0x006044E0, &DetectGrenade_Evaluate);
ACTCONDWORLD_REVERSED("ConditionDetectProjectile::Evaluate", 0x00604570, &DetectProjectile_Evaluate);
ACTCONDWORLD_REVERSED("ConditionIsVehicleInRange::Evaluate", 0x006045D0, &IsVehicleInRange_Evaluate);
ACTCONDWORLD_REVERSED("ConditionInGuardRadius::Evaluate", 0x00604710, &InGuardRadius_Evaluate);
ACTCONDWORLD_REVERSED("ConditionAllyLeaderConditionGroup::Evaluate", 0x00604770, &AllyLeaderConditionGroup_Evaluate);
ACTCONDWORLD_REVERSED("ConditionAllyFollowerConditionGroup::Evaluate", 0x006047C0, &AllyFollowerConditionGroup_Evaluate);
ACTCONDWORLD_REVERSED("ConditionCanAcquireAlly::Evaluate", 0x00604800, &CanAcquireAlly_Evaluate);
ACTCONDWORLD_REVERSED("ConditionSteering::Evaluate", 0x00604840, &Steering_Evaluate);
ACTCONDWORLD_REVERSED("ConditionSteeringFraction::Evaluate", 0x006048A0, &SteeringFraction_Evaluate);
ACTCONDWORLD_REVERSED("ConditionTargetAlly::Evaluate", 0x00604930, &TargetAlly_Evaluate);
ACTCONDWORLD_REVERSED("ConditionInWeaponRange::Evaluate", 0x00604960, &InWeaponRange_Evaluate);
ACTCONDWORLD_REVERSED("ConditionTargetPed::Evaluate", 0x006049F0, &TargetPed_Evaluate);
ACTCONDWORLD_REVERSED("ConditionBalanceMeterPosition::Evaluate", 0x00604B20, &BalanceMeterPosition_Evaluate);
ACTCONDWORLD_REVERSED("ConditionTargetColliding::Evaluate", 0x00604B70, &TargetColliding_Evaluate);
ACTCONDWORLD_REVERSED("ConditionPedInConversation::Evaluate", 0x00604BA0, &PedInConversation_Evaluate);
ACTCONDWORLD_REVERSED("ConditionPedConversationStatus::Evaluate", 0x00604BC0, &PedDwordIs_Evaluate<0x1E70>);
ACTCONDWORLD_REVERSED("ConditionPedConversationMood::Evaluate", 0x00604BE0, &PedDwordIs_Evaluate<0x1E74>);
ACTCONDWORLD_REVERSED("ConditionHasLineOfSightToPlayer::Evaluate", 0x00604C00, &HasLineOfSightToPlayer_Evaluate);
ACTCONDWORLD_REVERSED("PedGroundDistCond::Evaluate", 0x00604CD0, &PedGroundDist_Evaluate);
ACTCONDWORLD_REVERSED("PedVelocityCond::Evaluate", 0x00604DB0, &PedVelocity_Evaluate);
ACTCONDWORLD_REVERSED("ConditionCanGetOffVehicleHere::Evaluate", 0x00604F70, &CanGetOffVehicleHere_Evaluate<false>);
ACTCONDWORLD_REVERSED("ConditionCanGetOffVehicleHereLeft::Evaluate", 0x00605270, &CanGetOffVehicleHere_Evaluate<true>);
ACTCONDWORLD_REVERSED("ConditionCanGetOnVehicleHere::Evaluate", 0x00605570, &CanGetOnVehicleHere_Evaluate);
