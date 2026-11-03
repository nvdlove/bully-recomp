// The action tree's conditions about props, snow, allies and socializing (the ped's
// social block at +0xB44: +0xBB0 its social state, +0xBB4 the partner): each one's
// Evaluate(owner) (vf1, thiscall). 0x6034B0..0x6040F0.

#include <cstdint>

#include "game/ActionTracksCommon.h"

using namespace actc;

namespace {

GAME_VAR(float, runSpeedScale_B0AAC0, 0x00B0AAC0);

GAME_FN(uint8_t, __thiscall, PropAnim_CanSetUsageState, 0x004EAD40, uint8_t* prop, uint8_t* ped, int32_t state);
GAME_FN(uint8_t, __thiscall, Ped_EveryNthFrame, 0x00465A60, uint8_t* ped, uint32_t n);
GAME_FN(uint8_t, __thiscall, Ped_IsActionTimerAhead, 0x004749C0, uint8_t* ped);
GAME_FN(float*, __cdecl, FindPlayerCoors, 0x00446900, float* out);
GAME_FN(uint8_t, __thiscall, Ped_HasntMoved, 0x00477B90, uint8_t* ped);
GAME_FN(uint8_t, __cdecl, IsOnSnow, 0x00457F50, uint8_t* ped);
GAME_FN(uint8_t, __cdecl, FindSnow, 0x00457FC0, uint8_t* ped, float radius, float* out);
GAME_FN(float, __cdecl, Distance, 0x0041A950, const float* a, const float* b);
GAME_FN(uint8_t, __cdecl, FindSurfaceNear, 0x00457B10, uint8_t* ped, int32_t surface, float radius, float* out,
        int flag);
GAME_FN(V3*, __cdecl, VectorAdd, 0x004129A0, V3* out, const float* a, const float* b);
GAME_FN(uint8_t, __thiscall, Ally_HasFollower, 0x00485330, uint8_t* ally);
GAME_FN(uint8_t, __thiscall, Ally_IsFollower, 0x00485320, uint8_t* ally);
GAME_FN(uint8_t, __thiscall, Ally_IsFollowedBy, 0x00485500, uint8_t* ally, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Ally_KeepsFollower, 0x004854B0, uint8_t* ally, uint8_t* ped, int keep);
GAME_FN(int32_t*, __thiscall, Social_GetMood, 0x0048EBC0, uint8_t* social, int32_t* out, uint8_t* ped, int, uintptr_t, uintptr_t);
GAME_FN(uint8_t, __thiscall, Social_PlayAnswer, 0x0048F3C0, uint8_t* social, int, int);

// the social block's yes/no questions (thiscall on ped + 0xB44)
template <uintptr_t Fn>
uint8_t __fastcall SocialQuery_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return reinterpret_cast<uint8_t(__thiscall*)(uint8_t*)>(Fn)(OwnerPed(owner) + 0xB44);
}
// the ally block's (thiscall on ped + 0x2F0)
template <uintptr_t Fn>
uint8_t __fastcall AllyQuery_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return reinterpret_cast<uint8_t(__thiscall*)(uint8_t*)>(Fn)(OwnerPed(owner) + 0x2F0);
}
// a dword of the ped's equal to +4
template <int Offset>
uint8_t __fastcall PedDwordIs_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    return At<int32_t>(OwnerPed(owner), Offset) == At<int32_t>(self, 4) ? 1 : 0;
}
// the target as a ped (a non-ped target counts as none)
uint8_t* TargetPed(uint8_t* ped) {
    uint8_t* target = Ped_GetTarget(ped);
    if (target && !IsPedEntity(target)) {
        return nullptr;
    }
    return target;
}
bool IsSocializing(uint8_t* ped) {
    return At<int32_t>(ped, 0xBB0) != 0 && At<uint8_t*>(ped, 0xBB4) != nullptr;
}

// 0x6034B0: the target is an object (entity type 6)
uint8_t __fastcall IsTargetProp_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* target = Ped_GetTarget(OwnerPed(owner));
    if (!target) {
        return 0;
    }
    return (target[0x108] & 7) == 6 ? 1 : 0;
}

// 0x6034E0: the ped's prop (+0x438) can be set to usage state +4
uint8_t __fastcall CanPropSetUsageState_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* prop = At<uint8_t*>(ped, 0x438);
    if (!prop) {
        return 0;
    }
    return PropAnim_CanSetUsageState(prop + 0x398, ped, At<int32_t>(self, 4));
}

// 0x603510: every +4th frame (the ped's own frame offset when there's a ped)
uint8_t __fastcall Interval_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    if (owner && OwnerPed(owner)) {
        return Ped_EveryNthFrame(OwnerPed(owner), At<uint32_t>(self, 4));
    }
    return frameCounter % At<uint32_t>(self, 4) == 0 ? 1 : 0;
}

// 0x603540: the ped's action timer has run out
uint8_t __fastcall CheckActionTimer_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return Ped_IsActionTimerAhead(OwnerPed(owner)) == 0 ? 1 : 0;
}

// 0x603560: the player within +4 of the ped (or the owner's +0xC entity)
uint8_t __fastcall IsPlayerNearby_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* entity = OwnerPed(owner);
    if (!entity) {
        entity = At<uint8_t*>(owner, 0xC);
        if (!entity) {
            return 0;
        }
    }
    float* p = Pos(entity);
    float pos[3] = {p[0], p[1], p[2]};
    float player[3];
    FindPlayerCoors(player);
    float range = At<float>(self, 4);
    V3 d;
    V3* v = VectorSub(&d, pos, player);
    float left = v->x * v->x + v->y * v->y + v->z * v->z - range * range;
    return left <= 0.0f ? 1 : 0;
}

// 0x603640: the ped hasn't moved
uint8_t __fastcall IsHungUp_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return Ped_HasntMoved(OwnerPed(owner));
}

// 0x603670: the ped stands on snow
uint8_t __fastcall IsOnSnow_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return IsOnSnow(OwnerPed(owner));
}

// 0x603690: snow within +4 (found: the ped's +0x50C, where: +0x510)
uint8_t __fastcall FindSnow_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t found = FindSnow(ped, At<float>(self, 4), reinterpret_cast<float*>(ped + 0x510));
    ped[0x50C] = found;
    return found;
}

// 0x6036C0: a surface +0xC within +4 (+8: a flag of the search); the last one found is
// kept while it's within range
uint8_t __fastcall FindSurface_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    float* found = reinterpret_cast<float*>(ped + 0x510);
    if (ped[0x50C] == 0 || At<float>(self, 4) < Distance(found, Pos(ped))) {
        ped[0x50C] = FindSurfaceNear(ped, At<int32_t>(self, 0xC), At<float>(self, 4), found, self[8]);
    }
    return ped[0x50C];
}

// 0x603730: the target ped, +4 seconds ahead at its speed, within +8 of the ped (2D)
uint8_t __fastcall RunningAttack_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    float* p = Pos(OwnerPed(owner));
    float pedPos[3] = {p[0], p[1], p[2]};
    uint8_t* target = Ped_GetTarget(OwnerPed(owner));
    if (!target || !IsPedEntity(target)) {
        return 0;
    }
    float* q = Pos(target);
    float targetPos[3] = {q[0], q[1], q[2]};
    V3 speed;
    const float* velocity;
    if (IsPedEntity(target)) {
        float scale = 1.0f / (runSpeedScale_B0AAC0 * timeStep);
        speed.x = scale * At<float>(target, 0x1714);
        speed.y = At<float>(target, 0x1718) * scale;
        speed.z = scale * At<float>(target, 0x171C);
        velocity = &speed.x;
    } else {
        velocity = zeroVector;
    }
    float time = At<float>(self, 4);
    float ahead[3] = {time * velocity[0], velocity[1] * time, time * velocity[2]};
    V3 predicted;
    VectorAdd(&predicted, targetPos, ahead);
    V3 d;
    VectorSub(&d, &predicted.x, pedPos);
    float range = At<float>(self, 8);
    float left = d.x * d.x + d.y * d.y - range * range;
    return left <= 0.0f ? 1 : 0;
}

// 0x603930: the target ped follows the ped
uint8_t __fastcall IsTargetAlly_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!Ped_GetTarget(ped)) {
        return Ally_IsFollowedBy(ped + 0x2F0, nullptr);
    }
    return Ally_IsFollowedBy(ped + 0x2F0, TargetPed(ped));
}

// 0x603980: the ped can take the target ped on as a follower
uint8_t __fastcall CanRecruitTarget_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* target = TargetPed(ped);
    return Ally_KeepsFollower(ped + 0x2F0, target, 0) != 0 ? 1 : 0;
}

// 0x603A00: the social state is 3 or 4 (being the target)
uint8_t __fastcall IsSocialTarget_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    int32_t state = At<int32_t>(OwnerPed(owner), 0xBB0);
    return state == 4 || state == 3 ? 1 : 0;
}

// 0x603A30: the social override +4 (bytes at +0xC80) is on
uint8_t __fastcall UseSocialOverride_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    return OwnerPed(owner)[0xC80 + At<int32_t>(self, 4)] == 1 ? 1 : 0;
}

// 0x603A50: the player (faction 13) targets a ped whose social override +4 is on
uint8_t __fastcall PlayerUseSocialOverride_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!ped || At<int32_t>(ped, 0x1310) != 13) {
        return 0;
    }
    uint8_t* target = Ped_GetTarget(ped);
    if (!target || !IsPedEntity(target)) {
        return 0;
    }
    return target[At<int32_t>(self, 4) + 0xB43] == 1 ? 1 : 0;
}

// 0x603AA0: the social animation (+0xC24) is +4 while acting (states 1 and 3); else +4
// is 25 (none)
uint8_t __fastcall SocialAnimIs_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    int32_t state = At<int32_t>(OwnerPed(owner), 0xBB0);
    if (state != 1 && state != 3) {
        return At<int32_t>(self, 4) == 25 ? 1 : 0;
    }
    return At<int32_t>(OwnerPed(owner), 0xC24) == At<int32_t>(self, 4) ? 1 : 0;
}

// 0x603B30: the ped is socializing; for the player (+4) any ped socializing with it
uint8_t __fastcall IsSocializing_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (self[4] == 0 || At<int32_t>(ped, 0x1310) != 13) {
        return IsSocializing(ped) ? 1 : 0;
    }
    CPool<uint8_t>* pool = pedPool;
    for (int i = pool->size; i != 0;) {
        i--;
        if (pool->flags[i] & 0x80) {
            continue;
        }
        uint8_t* other = pool->objects + pool->elemSize * i;
        if (other && other != ped && IsSocializing(other) && At<uint8_t*>(other, 0xBB4) == ped &&
            (other[0xD68] & 1) == 0) {
            return 1;
        }
    }
    return 0;
}

// 0x603BD0: socializing with the player
uint8_t __fastcall IsSocializingWithPlayer_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    return IsSocializing(ped) && At<uint8_t*>(ped, 0xBB4) == playerPed ? 1 : 0;
}

// 0x603C10: wanting to socialize (+0xD68 bit 4) with someone (+0xBB8)
uint8_t __fastcall IsWantingToSocialize_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    return (ped[0xD68] & 0x10) != 0 && At<int32_t>(ped, 0xBB8) != 0 ? 1 : 0;
}

// 0x603C40: the ped's mood towards the target ped is +4
uint8_t __fastcall IsEmotionTowardsTarget_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* target = TargetPed(ped);
    int32_t mood;
    return *Social_GetMood(ped + 0xB44, &mood, target, 0, 0, 0) == At<int32_t>(self, 4) ? 1 : 0;
}

// 0x603CA0: the ped's mood towards the player is +4
uint8_t __fastcall IsEmotionTowardsPlayer_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    int32_t mood;
    return *Social_GetMood(OwnerPed(owner) + 0xB44, &mood, playerPed, 0, 0, 0) == At<int32_t>(self, 4) ? 1 : 0;
}

// 0x603CE0 / 0x603D20: socializing in a waiting (2, 4, 6) / acting (3, 1, 5) state
uint8_t __fastcall IsSocialStateWait_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!IsSocializing(ped)) {
        return 0;
    }
    int32_t state = At<int32_t>(ped, 0xBB0);
    return state == 2 || state == 4 || state == 6 ? 1 : 0;
}
uint8_t __fastcall IsSocialStateAct_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!IsSocializing(ped)) {
        return 0;
    }
    int32_t state = At<int32_t>(ped, 0xBB0);
    return state == 3 || state == 1 || state == 5 ? 1 : 0;
}

// 0x603D60: the initial reaction flag (+0xD69 bit 7)
uint8_t __fastcall IsSocialInitialReaction_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return uint8_t(OwnerPed(owner)[0xD69] >> 7);
}

// 0x603D80: the initial action (+0xD6A bit 0) or reaction flag
uint8_t __fastcall IsSocialInitialAction_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if ((ped[0xD6A] & 1) == 0 && (ped[0xD69] & 0x80) == 0) {
        return 0;
    }
    return 1;
}

// 0x603ED0: the script's social callback (the answer 25)
uint8_t __fastcall SocialScriptCallback_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return Social_PlayAnswer(OwnerPed(owner) + 0xB44, 0, 25);
}

// 0x603F10: the kiss count (+0xD60) compared (+8) with +4
uint8_t __fastcall IsKissCount_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    return CompareOp(float(At<int32_t>(OwnerPed(owner), 0xD60)), float(At<int32_t>(self, 4)), At<int32_t>(self, 8));
}

// 0x603F40: the milliseconds since the last social action (+0xC38) compared (+8) with +4
uint8_t __fastcall TimeOfLastSocialAction_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint32_t since = timeInMs - At<uint32_t>(OwnerPed(owner), 0xC38);
    return CompareOp(float(since), float(At<int32_t>(self, 4)), At<int32_t>(self, 8));
}

// 0x603F90: the target ped is socializing with someone else (for the player: anyone but
// us socializing with it)
uint8_t __fastcall IsTargetBusySocializing_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* target = Ped_GetTarget(ped);
    if (!target || !IsPedEntity(target)) {
        return 0;
    }
    if (At<int32_t>(target, 0x1310) != 13) {
        return IsSocializing(target) && (At<uint8_t*>(target, 0xBB4)[0xD68] & 1) == 0 ? 1 : 0;
    }
    CPool<uint8_t>* pool = pedPool;
    for (int i = pool->size; i != 0;) {
        i--;
        if (pool->flags[i] & 0x80) {
            continue;
        }
        uint8_t* other = pool->objects + pool->elemSize * i;
        if (other && other != ped && other != target && IsSocializing(other) && At<uint8_t*>(other, 0xBB4) == target &&
            (other[0xD68] & 1) == 0) {
            return 1;
        }
    }
    return 0;
}

// 0x604050 / 0x6040A0 / 0x6040F0: the target ped's last assaulter (+0xC68) / harasser
// (+0xC64) / dismisser is the ped
template <int Offset>
uint8_t __fastcall TargetPedRecorded_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* target = Ped_GetTarget(ped);
    if (!target || !IsPedEntity(target)) {
        return 0;
    }
    return ped && ped == At<uint8_t*>(target, Offset) ? 1 : 0;
}

} // namespace

#define ACTCONDSOCIAL_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

ACTCONDSOCIAL_REVERSED("ConditionIsTargetProp::Evaluate", 0x006034B0, &IsTargetProp_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionCanPropSetUsageState::Evaluate", 0x006034E0, &CanPropSetUsageState_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionInterval::Evaluate", 0x00603510, &Interval_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionCheckActionTimer::Evaluate", 0x00603540, &CheckActionTimer_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionIsPlayerNearby::Evaluate", 0x00603560, &IsPlayerNearby_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionIsHungUp::Evaluate", 0x00603640, &IsHungUp_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionStealthBehavior::Evaluate", 0x00603650, &PedDwordIs_Evaluate<0x18DC>);
ACTCONDSOCIAL_REVERSED("ConditionIsOnSnow::Evaluate", 0x00603670, &IsOnSnow_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionFindSnow::Evaluate", 0x00603690, &FindSnow_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionFindSurface::Evaluate", 0x006036C0, &FindSurface_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionRunningAttack::Evaluate", 0x00603730, &RunningAttack_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionIsAllyLeader::Evaluate", 0x006038F0, &AllyQuery_Evaluate<0x00485330>);
ACTCONDSOCIAL_REVERSED("ConditionIsAllyFollower::Evaluate", 0x00603910, &AllyQuery_Evaluate<0x00485320>);
ACTCONDSOCIAL_REVERSED("ConditionIsTargetAlly::Evaluate", 0x00603930, &IsTargetAlly_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionCanRecruitTarget::Evaluate", 0x00603980, &CanRecruitTarget_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionShouldSocialize::Evaluate", 0x006039C0, &SocialQuery_Evaluate<0x00492AE0>);
ACTCONDSOCIAL_REVERSED("ConditionShouldAllowApology::Evaluate", 0x006039E0, &SocialQuery_Evaluate<0x00494290>);
ACTCONDSOCIAL_REVERSED("ConditionIsSocialTarget::Evaluate", 0x00603A00, &IsSocialTarget_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionUseSocialOverride::Evaluate", 0x00603A30, &UseSocialOverride_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionPlayerUseSocialOverride::Evaluate", 0x00603A50, &PlayerUseSocialOverride_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionSocialAnimIs::Evaluate", 0x00603AA0, &SocialAnimIs_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionSocialEmotionIs::Evaluate", 0x00603AF0, &PedDwordIs_Evaluate<0xC30>);
ACTCONDSOCIAL_REVERSED("ConditionHumiliationGroupIs::Evaluate", 0x00603B10, &PedDwordIs_Evaluate<0xD44>);
ACTCONDSOCIAL_REVERSED("ConditionIsSocializing::Evaluate", 0x00603B30, &IsSocializing_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionIsSocializingWithPlayer::Evaluate", 0x00603BD0, &IsSocializingWithPlayer_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionIsWantingToSocialize::Evaluate", 0x00603C10, &IsWantingToSocialize_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionIsEmotionTowardsTarget::Evaluate", 0x00603C40, &IsEmotionTowardsTarget_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionIsEmotionTowardsPlayer::Evaluate", 0x00603CA0, &IsEmotionTowardsPlayer_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionIsSocialStateWait::Evaluate", 0x00603CE0, &IsSocialStateWait_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionIsSocialStateAct::Evaluate", 0x00603D20, &IsSocialStateAct_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionIsSocialInitialReaction::Evaluate", 0x00603D60, &IsSocialInitialReaction_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionIsSocialInitialAction::Evaluate", 0x00603D80, &IsSocialInitialAction_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionIsReadyToBeKissed::Evaluate", 0x00603DB0, &SocialQuery_Evaluate<0x0048FB40>);
ACTCONDSOCIAL_REVERSED("ConditionSocialTypeFAT::Evaluate", 0x00603DD0, &SocialQuery_Evaluate<0x0048E660>);
ACTCONDSOCIAL_REVERSED("ConditionSocialTypeGIRL::Evaluate", 0x00603DF0, &SocialQuery_Evaluate<0x0048FDD0>);
ACTCONDSOCIAL_REVERSED("ConditionSocialTypeBULLY::Evaluate", 0x00603E10, &SocialQuery_Evaluate<0x0048FE40>);
ACTCONDSOCIAL_REVERSED("ConditionSocialTypeNERD::Evaluate", 0x00603E30, &SocialQuery_Evaluate<0x0048FEA0>);
ACTCONDSOCIAL_REVERSED("ConditionSocialTypeAUTH::Evaluate", 0x00603E50, &SocialQuery_Evaluate<0x0048FEF0>);
ACTCONDSOCIAL_REVERSED("ConditionSocialTypeGEN::Evaluate", 0x00603E70, &SocialQuery_Evaluate<0x0048FF60>);
ACTCONDSOCIAL_REVERSED("ConditionSocialTypeOLD::Evaluate", 0x00603E90, &SocialQuery_Evaluate<0x0048E6A0>);
ACTCONDSOCIAL_REVERSED("ConditionSocialTypeNERDGIRL::Evaluate", 0x00603EB0, &SocialQuery_Evaluate<0x0048FFF0>);
ACTCONDSOCIAL_REVERSED("ConditionSocialScriptCallback::Evaluate", 0x00603ED0, &SocialScriptCallback_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionSocialActionInterrupted::Evaluate", 0x00603EF0, &SocialQuery_Evaluate<0x004967A0>);
ACTCONDSOCIAL_REVERSED("ConditionIsKissCount::Evaluate", 0x00603F10, &IsKissCount_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionTimeOfLastSocialAction::Evaluate", 0x00603F40, &TimeOfLastSocialAction_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionIsTargetBusySocializing::Evaluate", 0x00603F90, &IsTargetBusySocializing_Evaluate);
ACTCONDSOCIAL_REVERSED("ConditionHaveIAssaultedTargetPed::Evaluate", 0x00604050, &TargetPedRecorded_Evaluate<0xC68>);
ACTCONDSOCIAL_REVERSED("ConditionHaveIHarassedTargetPed::Evaluate", 0x006040A0, &TargetPedRecorded_Evaluate<0xC64>);
