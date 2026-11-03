// The action tree's conditions about the playing ped and its target (combat, reach,
// surfaces, speech, stealth, tethers): each one's Evaluate(owner) (vf1, thiscall, the
// owner's ped at +8). 0x601FB0..0x6034B0.

#include <cmath>
#include <cstdint>

#include "game/ActionTracksCommon.h"
#include "reversed.h"

using namespace actc;

namespace {

GAME_ARRAY(uint8_t*, projectiles, 0x00C3A6B8);   // CProjectileInfo's entities
GAME_VAR(uint8_t*, ignoreEntity, 0x00C1AE70);    // CWorld::pIgnoreEntity
GAME_VAR(uint8_t, skipFlagC4, 0x00C1AE6C);
GAME_VAR(uint8_t, includeFlag1C98Peds, 0x00C1AE75);
GAME_VAR(int32_t, speechSlotIndex, 0x00CF4644);  // the current of the two slots
inline uint8_t* const speechSlots = reinterpret_cast<uint8_t*>(0x00CF458C);   // 0x5C bytes each
inline void* const speechLib = reinterpret_cast<void*>(0x00CF4E10);
inline void* const charTriggers = reinterpret_cast<void*>(0x00BCD098);

GAME_FN(int, __cdecl, Projectile_FindNearest, 0x004D4140, float x, float y, float z, float radius, float maxSpeed,
        int model, uintptr_t, int);
GAME_FN(uint8_t, __thiscall, Combat_HasToken, 0x004868B0, uint8_t* combat, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Combat_InAttackCone, 0x004866F0, uint8_t* combat, uint8_t* target);
GAME_FN(uint8_t*, __thiscall, Ped_GetModelInfo, 0x00477DC0, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, ModelInfo_MatchesName, 0x0050E940, uint8_t* mi, uint32_t name);
GAME_FN(uint8_t, __thiscall, Ped_HasWeapon, 0x00474BF0, uint8_t* ped);
GAME_FN(float, __thiscall, Weapon_GetRangeFactor, 0x004D96B0, uint8_t* weapon);
GAME_FN(uint8_t, __thiscall, CharTriggers_Contains, 0x004245E0, void* triggers, uint8_t* ped);
GAME_FN(float, __cdecl, SafeAcos, 0x00413D60, float x);
GAME_FN(uint8_t*, __thiscall, AnimGroup_Find, 0x0042E350, uint8_t* table, uintptr_t name, uint32_t* out);
GAME_FN(void, __thiscall, Ped_AddAnimGroupRef, 0x004739E0, uint8_t* ped, uint32_t group);
GAME_FN(int, __cdecl, AnimGroup_StreamId, 0x0052DBD0, uint8_t* table, uint32_t group);
GAME_FN(uint8_t, __cdecl, Streaming_IsLoaded, 0x004151E0, int id);
GAME_FN(uint8_t, __cdecl, Streaming_IsRequested, 0x004151C0, int id);
GAME_FN(uint8_t, __cdecl, Streaming_IsLoading, 0x00450C00, int id);
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int id, int flags);
GAME_FN(void, __thiscall, AnimManager_AddRef, 0x006BD010, void* manager, uint8_t* table, uint32_t group);
GAME_FN(void, __thiscall, AnimGroup_SetRequested, 0x006B56B0, uint8_t* table, uint32_t group, int time);
GAME_FN(uint8_t, __cdecl, IsSpeechBusy, 0x005A2900, uint8_t* ped, int, int);
GAME_FN(uint8_t, __thiscall, SpeechSlot_IsPreloaded, 0x005A0E10, uint8_t* slot, uint32_t a, uint32_t b, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, SpeechLib_Has, 0x0059F3E0, void* lib, uint32_t event, int model);
GAME_FN(uint8_t, __cdecl, ProcessVerticalLine, 0x004629C0, const float* point, float zStop, ColPoint* colPoint,
        re::Ptr32<uint8_t>* entity, int lists, int);
GAME_FN(uint8_t*, __thiscall, Placeable_GetMatrix, 0x0046DD20, uint8_t* placeable);
GAME_FN(uint8_t*, __thiscall, Ped_GetMatrix, 0x004734D0, uint8_t* ped);
// (0x0045D500 stores 16 bytes through `out4`: called through re::CallVec16)
static inline void Matrix_MultiplyPoint4(float* out4, const uint8_t* m, const float* v) { re::CallVec16<0x0045D500, void>(out4, m, v); }
GAME_FN(uint8_t, __cdecl, ProcessSphereCast, 0x004616A0, const float* start, const float* end, float radius,
        ColPoint* colPoint, re::Ptr32<uint8_t>* entity, int flags, int lists, int);
GAME_FN(uint8_t*, __thiscall, Scanner_GetThreat, 0x0048C070, uint8_t* scanner);
GAME_FN(uint8_t, __thiscall, Combat_IsLineOfFireClear, 0x0048AD90, uint8_t* combat, uint8_t* target, int flags);
GAME_FN(uint8_t, __thiscall, Combat_IsOutOfReach, 0x00487AE0, uint8_t* combat, uint8_t* target, float a, float b);
GAME_FN(uint8_t*, __thiscall, Ped_GetAttacker, 0x00496D80, uint8_t* ped, uint8_t* of);
GAME_FN(V3*, __thiscall, Placeable_GetPosition, 0x00415460, uint8_t* placeable);
GAME_FN(uint8_t, __thiscall, Social_CanTalkTo, 0x0048D470, uint8_t* social, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Ped_IsStateIdle, 0x0041AA50, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Ped_Fn44E3D0, 0x0044E3D0, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Ped_Fn473FB0, 0x00473FB0, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, Ped_Fn46ECE0, 0x0046ECE0, uint8_t* ped);
GAME_FN(float, __cdecl, Stimuli_FootstepRange, 0x004A5A90);
GAME_FN(void, __thiscall, Scanner_Alert, 0x0048C480, uint8_t* scanner, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Tether_IsInside, 0x004998F0, uint8_t* tether, float margin);
GAME_FN(float, __thiscall, ConditionRange_Get, 0x005FCDF0, uint8_t* condition, int range);
GAME_FN(float, __cdecl, DistanceBeyond, 0x0041A4D0, const float* a, const float* b, float range);
GAME_FN(uint8_t*, __thiscall, Targeting_GetLookTarget, 0x004D7780, uint8_t* targeting);
GAME_FN(uint8_t*, __cdecl, GetColModel, 0x0051AF50, uint8_t* entity);

uint8_t* Target(uint8_t* ped) {
    return Ped_GetTarget(ped);
}
uint8_t* Combat(uint8_t* ped) {
    return ped + 0x668;
}

// 0x601FB0: a projectile within 20 of the ped (the ped's +0x354 set) flying at it
uint8_t __fastcall Projectile_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    float* pos = Pos(OwnerPed(owner));
    int i = Projectile_FindNearest(pos[0], pos[1], pos[2], 20.0f, 1000.0f, -1, 0, 0);
    if (i == -1) {
        return 0;
    }
    uint8_t* ped = OwnerPed(owner);
    if (ped[0x354] == 0) {
        return 0;
    }
    uint8_t* projectile = projectiles[i];
    V3 toPed;
    VectorSub(&toPed, Pos(ped), Pos(projectile));
    V3 velocity = {At<float>(projectile, 0x124), At<float>(projectile, 0x128), At<float>(projectile, 0x12C)};
    toPed.z = 0.0f;
    Vector_Normalise(&toPed);
    velocity.z = 0.0f;
    Vector_Normalise(&velocity);
    float dot = velocity.x * toPed.x + velocity.y * toPed.y + velocity.z * toPed.z;
    return dot > double(0.96f) ? 1 : 0;
}

// 0x6020D0: the target (a ped) gave us the fight token, or any non-ped target
uint8_t __fastcall FightToken_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* target = Target(ped);
    if (target && IsPedEntity(target)) {
        return Combat_HasToken(Combat(target), ped) ? 1 : 0;
    }
    return Target(ped) ? 1 : 0;
}

// 0x602120: the ped's attack phase is +4 and (for a time +8 > 0) its attack ends later
// than +8 seconds from now
uint8_t __fastcall AttackPhase_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (At<uint32_t>(self, 4) != At<uint32_t>(ped, 0x71C)) {
        return 0;
    }
    float time = At<float>(self, 8);
    if (!(0.0f < time)) {
        return 1;
    }
    uint32_t end = At<uint32_t>(ped, 0x720);
    if (end == 0) {
        return 0;
    }
    uint32_t until = uint32_t(int64_t(double(time) * 1000.0 + double(timeInMs)));
    return until < end ? 1 : 0;
}

// 0x6021B0: the ped's model is named +4
uint8_t __fastcall PedModelID_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* mi = Ped_GetModelInfo(OwnerPed(owner));
    return ModelInfo_MatchesName(mi, At<uint32_t>(self, 4));
}

// 0x6021D0: the target within the engage distance of the ped's combat state (+0x66C;
// the AI butes' +0x28.. distances; +4 halfway ones, +5 always the far one), or the
// weapon's range for a ranged weapon
uint8_t __fastcall EngageDistance_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    int32_t state = At<int32_t>(ped, 0x66C);
    if (state == 4) {
        return 0;
    }
    float range;
    if (self[5] != 0) {
        range = At<float>(aiButes, 0x34) + 1.0f;
    } else if (self[4] != 0) {
        if (state == 0) {
            range = (At<float>(aiButes, 0x2C) + At<float>(aiButes, 0x28)) * 0.5f;
        } else if (state == 1) {
            range = (At<float>(aiButes, 0x30) + At<float>(aiButes, 0x2C)) * 0.5f;
        } else if (state == 2) {
            range = At<float>(aiButes, 0x34);
        } else {
            return 0;
        }
    } else {
        if (state == 0) {
            range = At<float>(aiButes, 0x2C) + 1.0f;
        } else if (state == 1) {
            range = At<float>(aiButes, 0x30) + 1.0f;
        } else if (state == 2) {
            range = At<float>(aiButes, 0x34) + 1.0f;
        } else {
            return 0;
        }
    }
    if (Ped_HasWeapon(ped)) {
        uint8_t* weapon = At<uint8_t*>(OwnerPed(owner), 0x1D0);
        if (Weapon_GetRangeFactor(weapon) != 0.0f) {
            range = At<float>(OwnerPed(owner), 0x42C);
            if (range == 0.0f) {
                range = At<float>(weapon, 0x120);
            }
        }
    }
    uint8_t* target = Target(ped);
    if (!target) {
        return 0;
    }
    V3 d;
    VectorSub(&d, Pos(target), Pos(ped));
    float dist = SqrtAbs(d.y * d.y + d.x * d.x);
    return range > dist ? 1 : 0;
}

// 0x602360: the ped's target is in its attack cone (a non-ped target counts as none)
uint8_t __fastcall InAttackCone_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!Target(ped)) {
        return 0;
    }
    uint8_t* target = Target(ped);
    if (target && !IsPedEntity(target)) {
        target = nullptr;
    }
    return Combat_InAttackCone(Combat(ped), target);
}

// 0x6023C0 / 0x6023E0 / 0x6025D0 / 0x602D50 / 0x602E40 / 0x602E50 / 0x6031A0: a flag of
// the ped's
template <int Offset>
uint8_t __fastcall PedByteSet_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return OwnerPed(owner)[Offset] != 0 ? 1 : 0;
}
template <int Offset>
uint8_t __fastcall PedByteIsOne_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return OwnerPed(owner)[Offset] == 1 ? 1 : 0;
}
template <int Offset>
uint8_t __fastcall PedByte_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return OwnerPed(owner)[Offset];
}
template <int Offset>
uint8_t __fastcall PedDwordSet_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return At<int32_t>(OwnerPed(owner), Offset) != 0 ? 1 : 0;
}

// 0x602400: the first nearby ped (+0x3C0 count, +0x3C4 list) of the ped's faction
// (+0x1310) is within +4 (2D)
uint8_t __fastcall FactionNear_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    for (int i = 0; i < At<int32_t>(ped, 0x3C0); i++) {
        uint8_t* other = At<uint8_t*>(ped, 0x3C4 + i * 4);
        if (other && At<int32_t>(ped, 0x1310) == At<int32_t>(other, 0x1310)) {
            V3 d;
            V3* p = VectorSub(&d, Pos(ped), Pos(other));
            float range = At<float>(self, 4);
            return p->x * p->x + p->y * p->y < range * range ? 1 : 0;
        }
    }
    return 0;
}

// 0x6024B0: the ped is one of the character triggers
uint8_t __fastcall PedIsTrigger_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return CharTriggers_Contains(charTriggers, OwnerPed(owner));
}

// 0x6024D0: the target (or +0x438, or +0x434) is nearer than +4 (its own distance virtual)
uint8_t __fastcall DistToTarget_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* target = Target(ped);
    if (!target) {
        target = At<uint8_t*>(ped, 0x438);
    }
    if (!target) {
        target = At<uint8_t*>(ped, 0x434);
    }
    if (!target) {
        return 0;
    }
    float* p = Pos(ped);
    V3 pos = {p[0], p[1], p[2]};
    float dist = vc::Call<float>(target, 0x68, &pos);
    return At<float>(self, 4) > dist ? 1 : 0;
}

// 0x602560: the surface the ped last stood on (+0x1BE)
uint8_t __fastcall LastSurfaceOn_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    return uint32_t(OwnerPed(owner)[0x1BE]) == At<uint32_t>(self, 4) ? 1 : 0;
}

// 0x602580: the ground's grade (acos of the ped's +0x1764) compared (+8) with +4
uint8_t __fastcall SurfaceGrade_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    float grade = SafeAcos(At<float>(OwnerPed(owner), 0x1764));
    return CompareOp(grade, At<float>(self, 4), At<int32_t>(self, 8));
}

// 0x6025F0: the bike's (+0x1554) drive force (+0x698) compared (+8) with +4
uint8_t __fastcall DriveForce_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* vehicle = At<uint8_t*>(OwnerPed(owner), 0x1554);
    if (!vehicle) {
        return 0;
    }
    return CompareOp(At<float>(vehicle, 0x698), At<float>(self, 4), At<int32_t>(self, 8));
}

// 0x602630: the animation group +4 is loaded (and added to the ped); +8: request it when
// it isn't (+0xC the time it's wanted for)
uint8_t __fastcall AnimationGroupLoaded_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    if (At<uint32_t>(self, 4) == 0) {
        return 0;
    }
    uint8_t result = 0;
    uint8_t* ped = OwnerPed(owner);
    if (!ped || !At<uint8_t*>(ped, 0x114)) {
        return result;
    }
    uint8_t* table = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(ped, 0x114), 0x94), 0x10);
    if (!table) {
        return result;
    }
    uint32_t group;
    uint8_t* found = AnimGroup_Find(table, At<uint32_t>(self, 4), &group);
    if (!found) {
        return result;
    }
    if (found[0x34] != 0) {
        result = 1;
        if (OwnerPed(owner)) {
            Ped_AddAnimGroupRef(OwnerPed(owner), group);
        }
        return result;
    }
    if (self[8] == 0) {
        return result;
    }
    int id = AnimGroup_StreamId(table, group);
    if (Streaming_IsLoaded(id)) {
        return 0;
    }
    if (!Streaming_IsRequested(id) && !Streaming_IsLoading(id)) {
        RequestModel(id, 0);
        AnimManager_AddRef(animManager, table, group);
        AnimGroup_SetRequested(table, group, static_cast<int>(At<float>(self, 0xC)));
    }
    return 0;
}

// 0x602740: the speech system is busy (for the ped when +4)
uint8_t __fastcall IsSpeechBusy_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    if (self[4] != 0 && owner) {
        return IsSpeechBusy(OwnerPed(owner), -1, -1);
    }
    return IsSpeechBusy(nullptr, -1, -1);
}

// 0x602780: speech +4/+8 preloaded in either slot (for the ped when +0xC)
uint8_t __fastcall IsSpeechPreloaded_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = self[0xC] ? OwnerPed(owner) : nullptr;
    if (SpeechSlot_IsPreloaded(speechSlots + speechSlotIndex * 0x5C, At<uint32_t>(self, 4), At<uint32_t>(self, 8),
                               ped)) {
        return 1;
    }
    ped = self[0xC] ? OwnerPed(owner) : nullptr;
    if (SpeechSlot_IsPreloaded(speechSlots + ((speechSlotIndex - 1) & 1) * 0x5C, At<uint32_t>(self, 4),
                               At<uint32_t>(self, 8), ped)) {
        return 1;
    }
    return 0;
}

// 0x602800: the ped's tree (+0x2E0 +0x4B0) has the node named +4
uint8_t __fastcall HasActionNode_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    if (At<uint32_t>(self, 4) == 0) {
        return 0;
    }
    uint8_t* tree = At<uint8_t*>(At<uint8_t*>(OwnerPed(owner), 0x2E0), 0x4B0);
    return vc::Call<void*>(tree, acttree::kNodeFindChild, self + 4) != nullptr ? 1 : 0;
}

// 0x602840: the speech library has event +4 for the ped's model
uint8_t __fastcall HasSpeechEvent_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    if (!owner || !OwnerPed(owner)) {
        return 0;
    }
    return SpeechLib_Has(speechLib, At<uint32_t>(self, 4), At<int16_t>(OwnerPed(owner), 0x10E));
}

// 0x602870: the opening branch (the ped's +0x2E0 +0x270 +0x40) has a child named +4 with
// a child named +8
uint8_t __fastcall OpeningBranchChildHasChild_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    using namespace acttree;
    if (At<uint32_t>(self, 4) == 0 || At<uint32_t>(self, 8) == 0) {
        return 0;
    }
    uint8_t* branch = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(OwnerPed(owner), 0x2E0), 0x270), 0x40);
    if (!branch) {
        return 0;
    }
    int n = vc::Call<int>(branch, kNodeNumChildren);
    for (int i = 0; i < n; i++) {
        uint8_t* child = vc::Call<uint8_t*>(branch, kNodeGetChild, i);
        if (*vc::Call<uint32_t*>(child, kNodeGetName) == At<uint32_t>(self, 4)) {
            uint8_t* node = vc::Call<uint8_t*>(branch, kNodeGetChild, i);
            if (!node) {
                return 0;
            }
            int m = vc::Call<int>(node, kNodeNumChildren);
            if (m < 1) {
                return 0;
            }
            for (int j = 0; j < m; j++) {
                uint8_t* grandchild = vc::Call<uint8_t*>(node, kNodeGetChild, j);
                if (*vc::Call<uint32_t*>(grandchild, kNodeGetName) == At<uint32_t>(self, 8)) {
                    return 1;
                }
            }
            return 0;
        }
    }
    return 0;
}

// 0x602940: the height above the ground (a line down to -100) compared (+8) with +4
uint8_t __fastcall DistToGround_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    float* p = Pos(OwnerPed(owner));
    float pos[3] = {p[0], p[1], p[2]};
    ColPoint cp = {};
    cp.depth = 3.4028235e38f;
    re::Ptr32<uint8_t> hit;
    ProcessVerticalLine(pos, -100.0f, &cp, &hit, 1, 0);
    uint8_t result = CompareOp(pos[2] - cp.pos.z, At<float>(self, 4), At<int32_t>(self, 8));
    ReleaseColPoint(cp);
    return result;
}

// 0x602A30: a sphere (+4 radius) swept from +8 towards +0x14 (the fraction +0x28), in the
// ped's space (+0x25: its animated matrix) hits something (+0x20: not the ped; +0x21:
// ..; +0x22 / +0x23: which lists; +0x24: skip +0xC4 entities)
uint8_t __fastcall HasSweptSphereCollision_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    float start[4] = {At<float>(self, 8), At<float>(self, 0xC), At<float>(self, 0x10), 0.0f};
    float t = At<float>(self, 0x28);
    float dx = t * (At<float>(self, 0x14) - start[0]);
    float dy = (At<float>(self, 0x18) - start[1]) * t;
    float dz = t * (At<float>(self, 0x1C) - start[2]);
    float end[4] = {dx + start[0], start[1] + dy, start[2] + dz, 0.0f};
    uint8_t* m = Placeable_GetMatrix(OwnerPed(owner));
    if (self[0x25] != 0) {
        m = Ped_GetMatrix(OwnerPed(owner));
    }
    Matrix_MultiplyPoint4(start, m, start);
    Matrix_MultiplyPoint4(end, m, end);
    int flags = self[0x23] != 0 ? 8 : 10;
    if (self[0x22] == 0) {
        flags |= 1;
    }
    if (self[0x20] != 0) {
        ignoreEntity = OwnerPed(owner);
        flags |= 4;
        includeFlag1C98Peds = self[0x21] == 0 ? 1 : 0;
    } else {
        ignoreEntity = nullptr;
    }
    skipFlagC4 = self[0x24];
    ColPoint cp = {};
    cp.depth = 3.4028235e38f;
    re::Ptr32<uint8_t> hit;
    uint8_t result = ProcessSphereCast(start, end, At<float>(self, 4), &cp, &hit, flags, 0xF, 1);
    ignoreEntity = nullptr;
    includeFlag1C98Peds = 0;
    skipFlagC4 = 0;
    ReleaseColPoint(cp);
    return result;
}

// 0x602C40: ConditionMissionSuccessCount's deleting destructor (its name key at +0xC)
uint8_t* __fastcall MissionSuccessCount_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = 0x0092E7E0;
    NameKey_Destruct(self + 0xC);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x602C70: the ped's +0x538 is a ped on a bike
uint8_t __fastcall IsTargetOnBike_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* target = At<uint8_t*>(OwnerPed(owner), 0x538);
    if (target && IsPedEntity(target)) {
        return At<uint8_t*>(target, 0x1554) != nullptr ? 1 : 0;
    }
    return 0;
}

// 0x602CB0: the scanner's threat is on a bike
uint8_t __fastcall IsFocusOnBike_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* threat = Scanner_GetThreat(OwnerPed(owner) + 0x3BC);
    if (!threat) {
        return 0;
    }
    return At<uint8_t*>(threat, 0x1554) != nullptr ? 1 : 0;
}

// 0x602CE0: the ped owns its objective vehicle (+0x430, its +0x358)
uint8_t __fastcall IsObjectiveVehicleOwner_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!ped || !At<uint8_t*>(ped, 0x430)) {
        return 0;
    }
    return At<uint8_t*>(At<uint8_t*>(ped, 0x430), 0x358) == ped ? 1 : 0;
}

// 0x602D10: the line of fire to the target is clear (+4: flag 4)
uint8_t __fastcall LineOfFireClear_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* target = Target(ped);
    if (!target) {
        return 0;
    }
    return Combat_IsLineOfFireClear(Combat(ped), target, self[4] != 0 ? 4 : 0) != 0 ? 1 : 0;
}

// 0x602D70: the target is a crouching ped (+0x131C)
uint8_t __fastcall IsTargetCrouching_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* target = Target(OwnerPed(owner));
    if (target && IsPedEntity(target)) {
        return At<int32_t>(target, 0x131C) != 0 ? 1 : 0;
    }
    return 0;
}

// 0x602DA0: the target (a ped) is out of reach (+4 / +8), and so is its attacker
uint8_t __fastcall IsTargetOutOfReach_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* target = Target(ped);
    if (!target || !IsPedEntity(target)) {
        return 0;
    }
    if (!Combat_IsOutOfReach(Combat(ped), target, At<float>(self, 4), At<float>(self, 8))) {
        return 0;
    }
    uint8_t* other = Ped_GetAttacker(ped, target);
    if (other && IsPedEntity(other) &&
        !Combat_IsOutOfReach(Combat(ped), other, At<float>(self, 4), At<float>(self, 8))) {
        return 0;
    }
    return 1;
}

// 0x602E70: a stealth mission character (+0x1398) among the nearby peds within +4 (2D)
uint8_t __fastcall WantToBeStealthy_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    for (int i = 0; i < At<int32_t>(ped, 0x3C0); i++) {
        uint8_t* other = At<uint8_t*>(ped, 0x3C4 + i * 4);
        if (other && At<int32_t>(other, 0x1398) != 0) {
            V3 d;
            V3* p = VectorSub(&d, reinterpret_cast<float*>(Placeable_GetPosition(ped)), Pos(other));
            float range = At<float>(self, 4);
            if (p->x * p->x + p->y * p->y < range * range) {
                return 1;
            }
        }
    }
    return 0;
}

// 0x602F20 (cdecl): a nearby ped of a kind that can talk to the ped (2: the player)
uint8_t __cdecl FindSocialCandidate(uint8_t* ped, int kind) {
    if (kind == 2) {
        V3 d;
        VectorSub(&d, Pos(ped), Pos(playerPed));
        return Social_CanTalkTo(playerPed + 0xA64, ped);
    }
    for (int i = 0; i < At<int32_t>(ped, 0x3C0); i++) {
        uint8_t* other = At<uint8_t*>(ped, 0x3C4 + i * 4);
        if (!other) {
            continue;
        }
        bool candidate = false;
        switch (kind) {
        case 0:
            candidate = true;
            break;
        case 1:
            candidate = At<int32_t>(other, 0x1398) != 0;
            break;
        case 3:
            candidate = Ped_IsStateIdle(other) != 0;
            break;
        case 4:
            candidate = Ped_GetModelInfo(other)[0x50] != 0 && Ped_Fn44E3D0(other) != 0;
            break;
        case 5:
            candidate = At<int32_t>(ped, 0x1310) != At<int32_t>(other, 0x1310);
            break;
        case 6:
            candidate = At<int32_t>(ped, 0x1310) == At<int32_t>(other, 0x1310);
            break;
        case 7:
            candidate = Ped_Fn473FB0(other) != 0 && Ped_Fn46ECE0(other) == ped;
            break;
        case 8:
            candidate = At<int32_t>(other, 0x1398) != 0 && At<int32_t>(other, 0x18DC) == 1;
            break;
        default:
            break;
        }
        if (candidate && Social_CanTalkTo(other + 0xA64, ped)) {
            return 1;
        }
    }
    return 0;
}

// 0x603090: the player (1 up) is within the footstep hearing range of the ped (2 up);
// alerts the ped's scanner
uint8_t __fastcall HearPlayerFootsteps_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* player = playerPed;
    float* p = Pos(OwnerPed(owner));
    float ear[3] = {p[0], p[1], p[2] + 2.0f};
    float* q = Pos(player);
    float feet[3] = {q[0], q[1], q[2] + 1.0f};
    V3 d;
    V3* v = VectorSub(&d, feet, ear);
    float dist = SqrtAbs(v->y * v->y + v->x * v->x + v->z * v->z);
    if (dist < Stimuli_FootstepRange()) {
        Scanner_Alert(OwnerPed(owner) + 0x3BC, player);
        return 1;
    }
    return 0;
}

// 0x6031C0: the ped is inside its tether (+0x358)
uint8_t __fastcall IsInsideTether_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return Tether_IsInside(OwnerPed(owner) + 0x358, 0.0f);
}

// 0x6031E0: the target is between range classes +4 and +8 (4: unbounded)
uint8_t __fastcall IsTargetInRange_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* target = Target(ped);
    if (!target) {
        return 0;
    }
    if (At<int32_t>(self, 4) != 4) {
        float range = ConditionRange_Get(self, At<int32_t>(self, 4) - 1) - 0.5f;
        if (range < 0.0f) {
            range = 0.0f;
        }
        float d = DistanceBeyond(Pos(ped), Pos(target), range);
        if (d < 0.0f) {
            return 0;
        }
    }
    if (At<int32_t>(self, 8) != 4) {
        float range = ConditionRange_Get(self, At<int32_t>(self, 8)) + 0.5f;
        float d = DistanceBeyond(Pos(ped), Pos(target), range);
        if (d > 0.0f) {
            return 0;
        }
    }
    return 1;
}

// 0x6032D0: the target's (+4: the ped's +0x155C; else the target or the look target)
// height overlaps the ped's between fractions +8 and +0xC of its collision height
uint8_t __fastcall TargetRelativeHeight_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!ped) {
        return 0;
    }
    uint8_t* target;
    if (self[4] == 0) {
        target = Target(ped);
        if (!target) {
            target = Targeting_GetLookTarget(At<uint8_t*>(ped, 0x1C8));
            if (!target) {
                return 0;
            }
        }
    } else {
        target = At<uint8_t*>(ped, 0x155C);
        if (!target) {
            return 0;
        }
    }
    uint8_t* mi = modelInfoPtrs[At<int16_t>(ped, 0x10E)];
    if (!mi) {
        return 0;
    }
    uint8_t* col = At<uint8_t*>(mi, 0xC);
    if (!col) {
        return 0;
    }
    uint8_t* targetMi = modelInfoPtrs[At<int16_t>(target, 0x10E)];
    if (!targetMi || !At<uint8_t*>(targetMi, 0xC)) {
        return 0;
    }
    uint8_t* targetCol = GetColModel(target);
    float z = Pos(ped)[2];
    float minZ = z + At<float>(col, 0x18);
    float maxZ = z + At<float>(col, 0x28);
    float targetZ = Pos(target)[2];
    float height = maxZ - minZ;
    float targetMaxZ = targetZ + At<float>(targetCol, 0x28);
    float low = At<float>(self, 8) * height + minZ;
    if (low > targetMaxZ) {
        return 0;
    }
    float targetMinZ = targetZ + At<float>(targetCol, 0x18);
    float high = At<float>(self, 0xC) * height + minZ;
    return !(high < targetMinZ) ? 1 : 0;
}

} // namespace

#define ACTCONDPED_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

ACTCONDPED_REVERSED("ConditionProjectile::Evaluate", 0x00601FB0, &Projectile_Evaluate);
ACTCONDPED_REVERSED("ConditionFightToken::Evaluate", 0x006020D0, &FightToken_Evaluate);
ACTCONDPED_REVERSED("ConditionAttackPhase::Evaluate", 0x00602120, &AttackPhase_Evaluate);
ACTCONDPED_REVERSED("ConditionPedModelID::Evaluate", 0x006021B0, &PedModelID_Evaluate);
ACTCONDPED_REVERSED("ConditionEngageDistance::Evaluate", 0x006021D0, &EngageDistance_Evaluate);
ACTCONDPED_REVERSED("ConditionInAttackCone::Evaluate", 0x00602360, &InAttackCone_Evaluate);
ACTCONDPED_REVERSED("ConditionCollisionStart::Evaluate", 0x006023C0, &PedByteSet_Evaluate<0x4E5>);
ACTCONDPED_REVERSED("ConditionHitReactionEnd::Evaluate", 0x006023E0, &PedByteSet_Evaluate<0x4EC>);
ACTCONDPED_REVERSED("ConditionFactionNear::Evaluate", 0x00602400, &FactionNear_Evaluate);
ACTCONDPED_REVERSED("ConditionPedIsTrigger::Evaluate", 0x006024B0, &PedIsTrigger_Evaluate);
ACTCONDPED_REVERSED("ConditionDistToTarget::Evaluate", 0x006024D0, &DistToTarget_Evaluate);
ACTCONDPED_REVERSED("ConditionLastSurfaceOn::Evaluate", 0x00602560, &LastSurfaceOn_Evaluate);
ACTCONDPED_REVERSED("ConditionSurfaceGrade::Evaluate", 0x00602580, &SurfaceGrade_Evaluate);
ACTCONDPED_REVERSED("ConditionHaveIAttacked::Evaluate", 0x006025D0, &PedByteIsOne_Evaluate<0x4EE>);
ACTCONDPED_REVERSED("ConditionDriveForce::Evaluate", 0x006025F0, &DriveForce_Evaluate);
ACTCONDPED_REVERSED("ConditionAnimationGroupLoaded::Evaluate", 0x00602630, &AnimationGroupLoaded_Evaluate);
ACTCONDPED_REVERSED("ConditionIsSpeechBusy::Evaluate", 0x00602740, &IsSpeechBusy_Evaluate);
ACTCONDPED_REVERSED("ConditionIsSpeechPreloaded::Evaluate", 0x00602780, &IsSpeechPreloaded_Evaluate);
ACTCONDPED_REVERSED("ConditionHasActionNode::Evaluate", 0x00602800, &HasActionNode_Evaluate);
ACTCONDPED_REVERSED("ConditionHasSpeechEvent::Evaluate", 0x00602840, &HasSpeechEvent_Evaluate);
ACTCONDPED_REVERSED("ConditionOpeningBranchChildHasChild::Evaluate", 0x00602870, &OpeningBranchChildHasChild_Evaluate);
ACTCONDPED_REVERSED("ConditionDistToGround::Evaluate", 0x00602940, &DistToGround_Evaluate);
ACTCONDPED_REVERSED("ConditionHasSweptSphereCollision::Evaluate", 0x00602A30, &HasSweptSphereCollision_Evaluate);
ACTCONDPED_REVERSED("ConditionMissionSuccessCount::DeletingDestruct", 0x00602C40, &MissionSuccessCount_DeletingDestruct);
ACTCONDPED_REVERSED("ConditionIsTargetOnBike::Evaluate", 0x00602C70, &IsTargetOnBike_Evaluate);
ACTCONDPED_REVERSED("ConditionIsFocusOnBike::Evaluate", 0x00602CB0, &IsFocusOnBike_Evaluate);
ACTCONDPED_REVERSED("ConditionIsObjectiveVehicleOwner::Evaluate", 0x00602CE0, &IsObjectiveVehicleOwner_Evaluate);
ACTCONDPED_REVERSED("ConditionLineOfFireClear::Evaluate", 0x00602D10, &LineOfFireClear_Evaluate);
ACTCONDPED_REVERSED("ConditionIsGuarding::Evaluate", 0x00602D50, &PedDwordSet_Evaluate<0x1E08>);
ACTCONDPED_REVERSED("ConditionIsTargetCrouching::Evaluate", 0x00602D70, &IsTargetCrouching_Evaluate);
ACTCONDPED_REVERSED("ConditionIsTargetOutOfReach::Evaluate", 0x00602DA0, &IsTargetOutOfReach_Evaluate);
ACTCONDPED_REVERSED("ConditionIsStationary::Evaluate", 0x00602E40, &PedByte_Evaluate<0x4E4>);
ACTCONDPED_REVERSED("ConditionIsStealthMissionChar::Evaluate", 0x00602E50, &PedDwordSet_Evaluate<0x1398>);
ACTCONDPED_REVERSED("ConditionWantToBeStealthy::Evaluate", 0x00602E70, &WantToBeStealthy_Evaluate);
ACTCONDPED_REVERSED("Condition::FindSocialCandidate_602F20", 0x00602F20, &FindSocialCandidate);
ACTCONDPED_REVERSED("ConditionHearPlayerFootsteps::Evaluate", 0x00603090, &HearPlayerFootsteps_Evaluate);
ACTCONDPED_REVERSED("ConditionIsTethered::Evaluate", 0x006031A0, &PedDwordSet_Evaluate<0x360>);
ACTCONDPED_REVERSED("ConditionIsInsideTether::Evaluate", 0x006031C0, &IsInsideTether_Evaluate);
ACTCONDPED_REVERSED("ConditionIsTargetInRange::Evaluate", 0x006031E0, &IsTargetInRange_Evaluate);
ACTCONDPED_REVERSED("ConditionTargetRelativeHeight::Evaluate", 0x006032D0, &TargetRelativeHeight_Evaluate);
