// The action tree's tracks of 0x611E50..0x6135D5: target clear, spine control (the
// head look at ped +0xD70), the pelvis health meter, falling damage, attack timer
// reset, and the animation tracks' Start / Update virtuals (AnimationTrack, its dialog,
// hit, joint driver and parametric kinds), the AI aim and the spud cannon's fire
// controller.
// An animation track: +0x18 the animation's name, +0x1C its group entry's name, +0x20
// its mode, +0x24 the slot, +0x28 start time, +0x30 speed, +0x34 blend in, +0x3C
// "hold the ped", +0x3D "match the ped's speed", +0x40 the owner, +0x44 playing,
// +0x48 the animation's index, +0x4C the blend it plays into, +0x50 the ped's held
// flag saved, +0x54 the speed played at.

#include <cmath>
#include <cstdint>

#include "game/ActionTracksCommon.h"
#include "game/TrackMath.h"
#include "reversed.h"

using namespace actc;

namespace {

// a NiTransform: rotation, translation, scale
struct NiTransformT {
    float rot[9];     // +0x0
    float trans[3];   // +0x24
    float scale;      // +0x30
};
static_assert(sizeof(NiTransformT) == 0x34);

GAME_VAR(int32_t, ratModel, 0x00BF42DC);          // the rat's model index
GAME_VAR(float, sprintRate, 0x00B0B194);
GAME_VAR(int32_t, boneTransformFlag, 0x00B0BC44);
GAME_VAR(uint8_t*, dialogState, 0x01B8919C);      // +0x59: lines finished
GAME_VAR(uint8_t*, loopSounds, 0x00CF4274);
inline void* const audioFocus = reinterpret_cast<void*>(0x00CF3DA0);
inline const char* const ratBaseAnim = reinterpret_cast<const char*>(0x0090C038);      // "RAT_PED\BASE"
inline const char* const playerBaseAnim = reinterpret_cast<const char*>(0x0090C048);   // "C_PLAYER\BASE"
inline const char* const rootName = reinterpret_cast<const char*>(0x0090B8F4);         // "ROOT"
inline const char* const spudMisfire = reinterpret_cast<const char*>(0x00931650);      // "SpudCanMisfire"
inline const char* const spudDryFire = reinterpret_cast<const char*>(0x00931660);      // "SpudCanDryFire"
inline const char* const spudFire = reinterpret_cast<const char*>(0x00931670);         // "SPUDCANNONFIRE3"

GAME_FN(void, __thiscall, Ped_SetTargetEntity, 0x00498B10, uint8_t* ped, uint8_t* target);
GAME_FN(void, __thiscall, HeadLook_SetBoneInfo, 0x004A2B40, uint8_t* headLook, uint8_t* info);
GAME_FN(void, __thiscall, HeadLook_SetTarget, 0x004A3B60, uint8_t* headLook, uint8_t* target);
GAME_FN(uint8_t*, __thiscall, Ped_GetEntity, 0x00477C00, uint8_t* ped, int32_t type, int);
GAME_FN(void, __thiscall, Ped_TakeDamage, 0x0047B940, uint8_t* ped, float amount, uint8_t* attacker);
GAME_FN(void, __thiscall, Scanner_SetTimeSince, 0x0048C190, uint8_t* scanner, int32_t ms);
GAME_FN(uint8_t, __cdecl, Anim_FindNode, 0x0060AE00, void* outA, void* outB, const char* name, uint8_t* model);
GAME_FN(void, __thiscall, AnimManager_GetNodePosition, 0x006C1190, void* manager, void* a, void* b, V3* out, uintptr_t);
GAME_FN(int, __thiscall, AnimDict_FindByName, 0x006B1D60, void* dict, int, const char* name, int32_t* index);
GAME_FN(int32_t, __thiscall, Ped_GetStat, 0x00477060, uint8_t* ped, int32_t stat);
GAME_FN(void, __thiscall, AnimManager_Blend, 0x006C0D40, void* manager, uint8_t* blend, uintptr_t index, uint8_t* slot,
        int flags, float speed, float blendIn);
GAME_FN(void, __thiscall, AnimManager_Play, 0x006C0E90, void* manager, uint8_t* blend, uint8_t* anim, int, float time,
        int, int);
GAME_FN(void, __thiscall, AnimManager_Update, 0x006C0DC0, void* manager, uint8_t* blend, uint8_t* anim, int);
GAME_FN(void, __thiscall, AnimManager_Stop, 0x006C0E10, void* manager, uint8_t* blend, uint8_t* anim, int);
GAME_FN(uint8_t*, __thiscall, Anim_GetSlot, 0x006B9380, void* self, int32_t type);
GAME_FN(uint8_t*, __thiscall, Entries2C_Find, 0x006074A0, uint8_t* dict, const char* name, uint32_t* index);
GAME_FN(uint8_t*, __thiscall, Entries2C_GetAt, 0x00607480, uint8_t* dict, uint32_t i);
GAME_FN(int32_t, __cdecl, AnimDict_IndexOf, 0x00607770, const uint32_t* name, uint8_t* dict);
GAME_FN(uint8_t*, __thiscall, Array64_Get, 0x00465560, uint8_t* array, int32_t i);
GAME_FN(void, __thiscall, AnimManager_GetBoneTransform, 0x006BE3B0, void* manager, uint8_t* bone, float time,
        int32_t flag, uint8_t* instance, V3* pos, float* rot);
GAME_FN(uint8_t, __cdecl, NearlyEqual, 0x00432A60, float a, float b, float epsilon);
GAME_FN(void, __thiscall, Vector_DivideBy, 0x0041A4A0, V3* v, float d);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const void* v);
GAME_FN(uint8_t*, __thiscall, Blend_GetAnim, 0x006B1F50, uint8_t* blend, int32_t slot);
GAME_FN(uint8_t*, __thiscall, Blend_GetAnimAt, 0x006B1FA0, uint8_t* blend, int32_t slot);
GAME_FN(int, __thiscall, Anim_GetBoneTravel, 0x00607500, uint8_t* dict, int32_t bone, V3* out);
GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_FN(float, __cdecl, AngleBetweenPoints, 0x00432D10, float x1, float y1, float x2, float y2);
GAME_FN(void, __thiscall, Matrix_SetRotateZOnly, 0x00412C20, uint8_t* matrix, float angle);
GAME_FN(void, __thiscall, Ped_SetHeading, 0x004734C0, uint8_t* ped, float heading);
GAME_FN(void, __thiscall, Entity_UpdateRwFrame, 0x00443AA0, uint8_t* entity);
GAME_FN(void, __cdecl, Node_Update, 0x006C9AF0, void* node);
GAME_FN(uint8_t, __thiscall, JointDriverTrack_FindTarget, 0x0060B320, uint8_t* self, uint8_t* ped, uint8_t* other);
GAME_FN(uint8_t*, __cdecl, JointConstraint_Alloc, 0x007307C0, uintptr_t size);
GAME_FN(uint8_t*, __thiscall, JointConstraint_Construct, 0x007332C0, uint8_t* self);
GAME_FN(float, __thiscall, JointDriverTrack_GetLength, 0x006077A0, uint8_t* self);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(uint8_t*, __thiscall, Skeleton_FindBone, 0x00465580, void* skeleton, uint32_t hash, int32_t* bone);
GAME_FN(uint8_t*, __thiscall, Nodes_FindByName, 0x004A6BB0, void* nodes, const char* name, int32_t* index);
GAME_FN(void, __thiscall, Constraints_Add, 0x0072EE50, void* constraints, uint8_t* constraint, int32_t kind);
GAME_FN(uint8_t*, __thiscall, Ped_GetMatrix, 0x004734D0, uint8_t* ped);
GAME_FN(void, __cdecl, Entity_GetNodeTransform, 0x00467D80, uint8_t* entity, uint32_t hash, NiTransformT* out, int);
GAME_FN(void, __thiscall, Matrix_FromTransform, 0x00412A70, uint8_t* matrix, const NiTransformT* t);
GAME_FN(int, __thiscall, AnimManager_Attach, 0x006C2070, void* manager, uint8_t* clump, int32_t index, uintptr_t,
        float* time, uint8_t* bone, uint8_t* blend, uintptr_t);
GAME_FN(uint8_t*, __thiscall, Targeting_GetTarget, 0x004D5AD0, uint8_t* targeting);
GAME_FN(void, __thiscall, AIAim_GetAimVector, 0x0060BBD0, uint8_t* self, V3* out, uint8_t* ped, uint8_t* target);
GAME_FN(uint8_t*, __thiscall, ActionInputs_GetRequest, 0x005F3720, uint8_t* inputs, int button);
GAME_FN(void, __thiscall, ActionInputs_SetRequest, 0x005F3A90, uint8_t* inputs, int button, int kind, float time);
GAME_FN(uint8_t*, __cdecl, UsePoint_GetEntity, 0x004EABB0, uint8_t* owner);
GAME_FN(void, __thiscall, SpudCannon_StopSounds, 0x0060BFB0, uint8_t* self, uintptr_t, uint8_t* entity);
GAME_FN(int32_t, __thiscall, Audio_GetSoundId, 0x00595B80, void* audio, const char* name);
GAME_FN(void, __thiscall, LoopSounds_Stop, 0x00592400, uint8_t* sounds, uintptr_t, uint8_t* entity, int32_t id);
GAME_FN(void, __thiscall, LoopSounds_PlayAtEntity, 0x00592FA0, uint8_t* sounds, uint8_t* entity, int32_t id, int, uintptr_t);
GAME_FN(void, __thiscall, LoopSounds_PlayAt, 0x00593010, uint8_t* sounds, int32_t id, uintptr_t, int, uintptr_t);
GAME_FN(float, __cdecl, RemapRange, 0x0072F500, float value, float from, float to, float outFrom, float outTo);
// the animation track's virtuals (by address, so their modes apply)
GAME_FN(void, __thiscall, AnimationTrack_PlayBase, 0x006123D0, uint8_t* self, uint8_t* slot);
GAME_FN(uint8_t, __thiscall, AnimationTrack_Start, 0x006124C0, uint8_t* self, uint8_t* owner);
GAME_FN(uint8_t, __thiscall, AnimationTrack_Update, 0x0060AF60, uint8_t* self, float dt);
GAME_FN(uint8_t, __thiscall, JointDriverTrack_Start, 0x00612C60, uint8_t* self, uint8_t* owner);

constexpr float kEpsilon = 1e-5f;
constexpr float kPi = 3.14159274f;

uint8_t* HeadLook(uint8_t* ped) {
    return ped + 0xD70;
}
// a ref-counted object (+4 its count) released: deleted at 0
void Release(uint8_t* object) {
    At<int32_t>(object, 4) += -1;
    if (At<int32_t>(object, 4) <= 0) {
        vc::Call<void>(object, 4, 1);
    }
}

// 0x611E50 (thiscall): TargetClearTrack::Start: the ped's target cleared
uint8_t __fastcall TargetClearTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    if (OwnerPed(owner)) {
        Ped_SetTargetEntity(OwnerPed(owner), nullptr);
    }
    return 1;
}

// 0x611E70 (thiscall): SpineControlTrack::Start: the head look's bones (+0x18), its four
// axis flags (+0x1C..+0x1F) and its rate (+0x20); the ped's spine users counted (+0xDA8)
uint8_t __fastcall SpineControlTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    uint8_t* ped = OwnerPed(owner);
    ped[0xDA8] += 1;
    uint8_t* look = HeadLook(ped);
    HeadLook_SetBoneInfo(look, self + 0x18);
    uint8_t bits = uint8_t((self[0x1F] & 1) << 3 | (self[0x1E] & 1) << 2 | (self[0x1D] & 1) << 1 | (self[0x1C] & 1));
    look[0x39] = uint8_t(bits | (look[0x39] & 0xF0));
    At<uint32_t>(look, 0x34) = At<uint32_t>(self, 0x20);
    return 1;
}

// 0x611EE0 (thiscall): SpineControlTrack::End
uint8_t __fastcall SpineControlTrack_End(uint8_t* self, re::Edx) {
    OwnerPed(At<uint8_t*>(self, 0x24))[0xDA8] += 0xFF;
    return 1;
}

// 0x611F00 (thiscall): SpineControlDisableTrack::Start: the spine's weight (ped +0xD9C)
// faded out over +0x18 seconds (none: at once)
uint8_t __fastcall SpineControlDisableTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    At<uint32_t>(self, 0x20) = timeInMs;
    if (std::fabs(At<float>(self, 0x18)) <= kEpsilon) {
        At<float>(OwnerPed(owner), 0xD9C) = 0.0f;
    }
    return 1;
}

// 0x611F50 (thiscall): SpineControlDisableTrack::Update: the weight along half a cosine
uint8_t __fastcall SpineControlDisableTrack_Update(uint8_t* self, re::Edx, float) {
    uint32_t elapsed = timeInMs - At<uint32_t>(self, 0x20);
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x1C));
    float duration = At<float>(self, 0x18);
    if (std::fabs(duration) <= kEpsilon) {
        At<float>(ped, 0xD9C) = 0.0f;
        return 1;
    }
    // (fild: the elapsed time exact, not rounded to float first; 2^32 added (rounded) when
    // it reads negative; over the duration's milliseconds, then rounded)
    double since = int32_t(elapsed) < 0 ? double(float(double(int32_t(elapsed)) + 4294967296.0)) : double(elapsed);
    float t = float(since / double(duration * 1000.0f));
    if (t < 1.0f) {
        float c = trk::CrtCos(t * kPi);
        At<float>(ped, 0xD9C) = c * 0.5f + 0.5f;
        return 1;
    }
    At<float>(ped, 0xD9C) = 0.0f;
    return 1;
}

// 0x612020 (thiscall): SpineControlDisableTrack::End: the weight back to 1
uint8_t __fastcall SpineControlDisableTrack_End(uint8_t* self, re::Edx) {
    At<float>(OwnerPed(At<uint8_t*>(self, 0x1C)), 0xD9C) = 1.0f;
    return 1;
}

// 0x612040 (thiscall): SpineControlSetTargetTrack::Start: the head look's target (the
// ped's entity of type +0x18)
uint8_t __fastcall SpineControlSetTargetTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    uint8_t* ped = OwnerPed(owner);
    HeadLook_SetTarget(HeadLook(ped), Ped_GetEntity(ped, At<int32_t>(self, 0x18), 0));
    return 1;
}

// 0x612070 (thiscall): SpineControlSetTargetTrack::Update: again while +0x1C
uint8_t __fastcall SpineControlSetTargetTrack_Update(uint8_t* self, re::Edx, float) {
    if (self[0x1C] != 0) {
        uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x20));
        HeadLook_SetTarget(HeadLook(ped), Ped_GetEntity(ped, At<int32_t>(self, 0x18), 0));
    }
    return 1;
}

// 0x6120B0 (thiscall): SpineControlSetTargetTrack::End
uint8_t __fastcall SpineControlSetTargetTrack_End(uint8_t* self, re::Edx) {
    HeadLook_SetTarget(HeadLook(OwnerPed(At<uint8_t*>(self, 0x20))), nullptr);
    return 1;
}

// 0x6120D0 (thiscall): usePelvisForHealthMeterTrack::Start: the meter's pelvis blend (ped
// +0x1E84 weight, +0x1E88 rate: 1 / +0x18); mode +0x20 (1: stops at once, 2: nothing)
uint8_t __fastcall PelvisHealthMeterTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (At<int32_t>(self, 0x20) != 2) {
        if (At<float>(self, 0x18) < 0.0f) {
            At<float>(ped, 0x1E84) = 1.0f;
            At<float>(ped, 0x1E88) = 0.0f;
            return At<int32_t>(self, 0x20) != 1 ? 1 : 0;
        }
        At<float>(ped, 0x1E84) = 0.0f;
        At<float>(ped, 0x1E88) = 1.0f / At<float>(self, 0x18);
    }
    return At<int32_t>(self, 0x20) != 1 ? 1 : 0;
}

// 0x612130 (thiscall): usePelvisForHealthMeterTrack::End: blended back at 1 / +0x1C
uint8_t __fastcall PelvisHealthMeterTrack_End(uint8_t* self, re::Edx) {
    if (At<int32_t>(self, 0x20) == 1) {
        return 1;
    }
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x24));
    if (At<float>(self, 0x1C) < 0.0f) {
        At<float>(ped, 0x1E84) = 0.0f;
        At<float>(ped, 0x1E88) = 0.0f;
        return 1;
    }
    At<float>(ped, 0x1E84) = 0.0f;
    At<float>(ped, 0x1E88) = 1.0f / At<float>(self, 0x1C);
    return 1;
}

// 0x612170 (thiscall): FallingDamageTrack::Start: the fall's start (+0x3C the ped's
// +0x18B0, +0x40 its position) and the damage so far (+0x30: none when +0x18, else the
// ped's; plus +0x1C when not negative)
uint8_t __fastcall FallingDamageTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x2C) = owner;
    uint8_t* ped = vc::Call<uint8_t*>(self, acttree::kTrackResolve);
    At<float>(self, 0x3C) = At<float>(ped, 0x18B0);
    const float* pos = Pos(ped);
    At<uint32_t>(self, 0x40) = reinterpret_cast<const uint32_t*>(pos)[0];
    At<uint32_t>(self, 0x44) = reinterpret_cast<const uint32_t*>(pos)[1];
    At<uint32_t>(self, 0x48) = reinterpret_cast<const uint32_t*>(pos)[2];
    float base = self[0x18] != 0 ? 0.0f : At<float>(ped, 0x1CC4);
    At<float>(self, 0x30) = base;
    float extra = 0.0f;
    if (!(0.0f > At<float>(self, 0x1C))) {
        extra = At<float>(self, 0x1C);
    }
    At<float>(self, 0x30) = base + extra;
    return 1;
}

// 0x6121F0 (thiscall): FallingDamageTrack::Update: while it has its ped
uint8_t __fastcall FallingDamageTrack_Update(uint8_t* self, re::Edx, float) {
    return vc::Call<uint8_t*>(self, acttree::kTrackResolve) != nullptr ? 1 : 0;
}

// 0x612200 (thiscall): FallingDamageTrack::End: the damage (+0x30, plus the height fallen
// times +0x24 unless mode +0x28 is 1; within 0..+0x20 when that's positive) left on the
// ped (+0x1CC4) with the owner's ped as its cause (+0x1CC8)
uint8_t __fastcall FallingDamageTrack_End(uint8_t* self, re::Edx) {
    uint8_t* cause = OwnerPed(At<uint8_t*>(self, 0x2C));
    uint8_t* ped = vc::Call<uint8_t*>(self, acttree::kTrackResolve);
    if (!ped) {
        return 0;
    }
    const float* pos = Pos(ped);
    float damage = At<float>(self, 0x30);
    if (At<int32_t>(self, 0x28) != 1) {
        float fallen = At<float>(self, 0x3C) - pos[2];
        damage = fallen * At<float>(self, 0x24) + damage;
    }
    if (0.0f < At<float>(self, 0x20)) {
        float limit = At<float>(self, 0x20);
        if (damage < 0.0f) {
            damage = 0.0f;
        } else if (limit < damage) {
            damage = limit;
        }
    }
    At<uint8_t*>(ped, 0x1CC8) = cause;
    At<float>(ped, 0x1CC4) = damage;
    return 1;
}

// 0x6122D0 (thiscall): FallingDamageHandleTrack::Start: the fall's damage taken (times
// +0x18) unless the ped's +0x14D4 blocks it; the fall cleared
uint8_t __fastcall FallingDamageHandleTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (ped) {
        if (At<int32_t>(ped, 0x14D4) == 0) {
            Ped_TakeDamage(ped, At<float>(ped, 0x1CC4) * At<float>(self, 0x18), At<uint8_t*>(ped, 0x1CC8));
        }
        At<int32_t>(ped, 0x14D4) = 0;
        At<float>(ped, 0x1CC4) = 0.0f;
        At<uint32_t>(ped, 0x1CC8) = 0;
    }
    return 0;
}

// 0x612330 (thiscall): AttackTimerResetTrack::Start: the ped's scanner's time since
// (+0x3BC) set to +0x18 seconds
uint8_t __fastcall AttackTimerResetTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (ped) {
        Scanner_SetTimeSince(ped + 0x3BC, int32_t(int64_t(At<float>(self, 0x18) * 1000.0f)));
    }
    return 1;
}

// 0x612380 (cdecl): a model's dummy (named node) position
uint8_t __cdecl Model_FindDummy(V3* out, const char* name, uint8_t* model) {
    void* b;
    void* a;
    uint8_t found = Anim_FindNode(&a, &b, name, model);
    if (found) {
        AnimManager_GetNodePosition(animManager, a, b, out, 0);
    }
    return found;
}

// 0x6123D0 (thiscall): AnimationTrack's fallback: the ped's base animation (the rat's
// or the player's) when it plays into the ped, else the group's first; at the ped's
// animation speed stat (0x14)
void __fastcall AnimationTrack_PlayBaseImpl(uint8_t* self, re::Edx, uint8_t* slot) {
    self[0x44] = 1;
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x40));
    if (!ped || At<uint8_t*>(self, 0x4C) != At<uint8_t*>(ped, 0x114)) {
        At<int32_t>(self, 0x48) = 0;
    } else {
        int16_t model = At<int16_t>(ped, 0x10E);
        void* dict = At<void*>(At<uint8_t*>(modelInfoPtrs[model], 0x48), 0x10);
        int32_t index = -1;
        if (model == ratModel) {
            AnimDict_FindByName(dict, 0, ratBaseAnim, &index);
        } else {
            AnimDict_FindByName(dict, 0, playerBaseAnim, &index);
        }
        At<int32_t>(self, 0x48) = index;
    }
    float speed = 1.0f;
    if (OwnerPed(At<uint8_t*>(self, 0x40))) {
        speed = float(Ped_GetStat(OwnerPed(At<uint8_t*>(self, 0x40)), 0x14)) / 100.0f;
    }
    AnimManager_Blend(animManager, At<uint8_t*>(self, 0x4C), At<int32_t>(self, 0x48), slot, 0, speed, -1.0f);
}

// 0x6124C0 (thiscall): AnimationTrack::Start: its animation blended in (the group's
// entry +0x1C, or the first, into the slot +0x24); its speed matched to the ped's
// movement when +0x3D (at least 0.8); the ped held (+0xE4) when +0x3C
uint8_t __fastcall AnimationTrack_StartImpl(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x40) = owner;
    self[0x44] = 0;
    At<int32_t>(self, 0x48) = -1;
    At<uint8_t*>(self, 0x4C) = nullptr;
    vc::Call<void>(self, acttree::kTrackResolve);
    uint8_t* blend = At<uint8_t*>(self, 0x4C);
    if (!blend || !At<uint8_t*>(blend, 0x94)) {
        return 0;
    }
    uint8_t* dict = At<uint8_t*>(At<uint8_t*>(blend, 0x94), 0x10);
    if (!dict) {
        return 0;
    }
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x40));
    int flags = 0;
    if (At<int32_t>(self, 0x20) == 2) {
        flags = 2;
    }
    int32_t slotType = At<int32_t>(self, 0x24);
    uint8_t* slot = Anim_GetSlot(nullptr, slotType);
    if (slotType > 0) {
        uint8_t* entry = nullptr;
        const char* group = At<const char*>(self, 0x1C);
        if (group) {
            entry = Entries2C_Find(dict, group, nullptr);
        }
        if (entry || (entry = Entries2C_GetAt(dict, 0)) != nullptr) {
            At<uint8_t*>(slot, 8) = entry;
        }
    }
    int32_t index = AnimDict_IndexOf(&At<uint32_t>(self, 0x18), dict);
    At<int32_t>(self, 0x48) = index;
    if (index == -1) {
        AnimationTrack_PlayBase(self, slot);
    } else {
        float blendIn = At<float>(self, 0x34);
        if (blendIn == 0.0f) {
            flags |= 1;
        } else if (blendIn < 0.0f) {
            blendIn = -1.0f;
        }
        At<float>(self, 0x54) = At<float>(self, 0x30);
        self[0x44] = 1;
        if (ped) {
            At<float>(self, 0x54) = float(Ped_GetStat(ped, 0x14)) / 100.0f * At<float>(self, 0x54);
        }
        float dt = timeStep;
        if (self[0x3D] != 0 && !NearlyEqual(dt, 0.0f, kEpsilon)) {
            // the animation's root speed over this frame against the ped's
            uint8_t* bone = Array64_Get(dict, At<int32_t>(self, 0x48));
            float t0 = At<float>(self, 0x28);
            float step = sprintRate * timeStep;
            uint8_t* instance = At<uint8_t*>(At<uint8_t*>(self, 0x4C), 0x94);
            float t1 = t0 + step;
            V3 p0;
            V3 p1;
            float r0[4];
            float r1[4];
            AnimManager_GetBoneTransform(animManager, bone, t0, boneTransformFlag, instance, &p0, r0);
            AnimManager_GetBoneTransform(animManager, bone, t1, boneTransformFlag, instance, &p1, r1);
            V3 a = p0;
            V3 b = p1;
            V3 d;
            VectorSub(&d, &b.x, &a.x);
            Vector_DivideBy(&d, dt);
            float animSpeed = Vector_Magnitude(&d);
            float pedSpeed = Vector_Magnitude(ped + 0x124);
            if (!NearlyEqual(pedSpeed, 0.0f, kEpsilon) && !NearlyEqual(animSpeed, 0.0f, kEpsilon)) {
                float ratio = pedSpeed / animSpeed;
                At<float>(self, 0x54) = ratio;
                if (ratio < 0.8f) {
                    At<float>(self, 0x54) = 0.8f;
                }
            }
        }
        AnimManager_Blend(animManager, At<uint8_t*>(self, 0x4C), At<int32_t>(self, 0x48), slot, flags,
                          At<float>(self, 0x54), blendIn);
        AnimManager_Play(animManager, At<uint8_t*>(self, 0x4C), slot, 0, At<float>(self, 0x28), 0, 0);
    }
    if (ped && self[0x3C] != 0) {
        uint8_t* owner2 = At<uint8_t*>(self, 0x40);
        self[0x50] = At<int32_t>(OwnerPed(owner2), 0xE4) != 0 ? 1 : 0;
        At<int32_t>(OwnerPed(owner2), 0xE4) = 1;
    }
    return 1;
}

// 0x612830 (thiscall): DialogAnimationTrack::Start: started at once when +0x5D (and
// paused: its animation's update stopped), else on the dialog's cue (+0x5E started)
uint8_t __fastcall DialogAnimationTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    self[0x44] = 0;
    At<int32_t>(self, 0x48) = -1;
    At<uint8_t*>(self, 0x4C) = nullptr;
    At<uint8_t*>(self, 0x40) = owner;
    self[0x5E] = 0;
    if (self[0x5D] != 0) {
        AnimationTrack_Start(self, owner);
        if (At<uint8_t*>(self, 0x4C)) {
            uint8_t* anim = Blend_GetAnimAt(At<uint8_t*>(self, 0x4C), At<int32_t>(self, 0x24));
            AnimManager_Update(animManager, At<uint8_t*>(self, 0x4C), anim, -1);
        }
    }
    return 1;
}

// 0x612890 (thiscall): DialogAnimationTrack::Update: waits for the dialog's lines (unless
// +0x5C), then starts (or resumes) the animation
uint8_t __fastcall DialogAnimationTrack_Update(uint8_t* self, re::Edx, float dt) {
    if (self[0x5E] != 0) {
        return AnimationTrack_Update(self, dt);
    }
    if (self[0x5C] != 0 || !dialogState || dialogState[0x59] != 0) {
        if (self[0x5D] != 0) {
            if (At<uint8_t*>(self, 0x4C)) {
                uint8_t* anim = Blend_GetAnimAt(At<uint8_t*>(self, 0x4C), At<int32_t>(self, 0x24));
                AnimManager_Stop(animManager, At<uint8_t*>(self, 0x4C), anim, -1);
                self[0x5E] = 1;
                return 1;
            }
        } else {
            AnimationTrack_Start(self, At<uint8_t*>(self, 0x40));
        }
        self[0x5E] = 1;
    }
    return 1;
}

// 0x612910 (thiscall): HitAnimationTrack::Start: the animation, the hit's timing (+0x5D:
// +0x60 / +0x68 / +0x64 into the hit state, owner +0x10), its blend out rate (+0x70)
// and the distance still to travel (+0x7C, +0x78 its time, +0x74 its rate); the entity
// turned to face the hit's direction (+0x20)
uint8_t __fastcall HitAnimationTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<float>(self, 0x6C) = -1.0f;
    At<float>(self, 0x7C) = 0.0f;
    self[0x44] = 0;
    At<int32_t>(self, 0x48) = -1;
    At<uint8_t*>(self, 0x4C) = nullptr;
    uint8_t result = AnimationTrack_Start(self, owner);
    if (self[0x5D] != 0) {
        At<float>(At<uint8_t*>(At<uint8_t*>(self, 0x40), 0x10), 0x4C) = At<float>(self, 0x60);
        At<float>(At<uint8_t*>(At<uint8_t*>(self, 0x40), 0x10), 0x54) = At<float>(self, 0x68);
        At<float>(At<uint8_t*>(At<uint8_t*>(self, 0x40), 0x10), 0x58) = At<float>(self, 0x64);
    }
    At<float>(self, 0x6C) = At<float>(At<uint8_t*>(At<uint8_t*>(self, 0x40), 0x10), 0x4C);
    uint8_t* anim = Blend_GetAnim(At<uint8_t*>(self, 0x4C), At<int32_t>(self, 0x24));
    float length = At<float>(At<uint8_t*>(anim, 0x18), 0xC);
    float rate = 1.0f;
    if (self[0x5C] != 0 && 0.0f < At<float>(self, 0x6C)) {
        float after = At<float>(self, 0x58);
        float left = length - after;
        float span = At<float>(self, 0x6C) - after;
        if (0.0f < left && 0.0f < span) {
            rate = left / span;
        }
    }
    uint8_t* hitOwner = At<uint8_t*>(self, 0x40);
    At<float>(self, 0x70) = rate;
    uint8_t* entity = nullptr;
    if (At<uint8_t*>(hitOwner, 8)) {
        entity = At<uint8_t*>(hitOwner, 8);
    } else if (At<uint8_t*>(hitOwner, 0xC)) {
        entity = At<uint8_t*>(hitOwner, 0xC);
    }
    re::Ptr32<uint8_t>* hit = &At<uint8_t*>(hitOwner, 0x10);
    float distance = At<float>(*hit, 0x54);
    V3 v;
    if (0.0f < distance) {
        v.x = 0.0f;
        v.y = 0.0f;
        Anim_GetBoneTravel(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x4C), 0x94), 0x10), At<int32_t>(self, 0x48),
                           &v);
        float travel = std::sqrt(std::fabs(v.x * v.x + v.y * v.y));
        hit = &At<uint8_t*>(At<uint8_t*>(self, 0x40), 0x10);
        float left = distance - travel;
        At<float>(self, 0x7C) = left;
        float time = At<float>(*hit, 0x58);
        At<float>(self, 0x78) = time;
        if (std::fabs(time) <= kEpsilon) {
            At<float>(self, 0x78) = 0.0f;
            At<float>(self, 0x74) = 1.0f;
            if (left < 0.0f) {
                At<float>(self, 0x74) = -1.0f;
                At<float>(self, 0x7C) = left * -1.0f;
            }
        } else {
            float limit = length;
            if (self[0x5C] != 0 && 0.0f < At<float>(self, 0x6C)) {
                limit = At<float>(self, 0x6C);
            }
            if (time < 0.0f || limit < time) {
                At<float>(self, 0x78) = limit;
            }
            At<float>(self, 0x74) = left / At<float>(self, 0x78);
            if (left < 0.0f) {
                At<float>(self, 0x7C) = left * -1.0f;
            }
        }
    } else {
        At<float>(self, 0x7C) = 0.0f;
    }
    Placeable_GetForward(entity, &v);
    const float* dir = &At<float>(*hit, 0x20);
    float x = dir[0];
    float y = dir[1];
    float dot = dir[1] * v.y + dir[0] * v.x + dir[2] * v.z;
    if (dot < 0.0f) {
        x = x * -1.0f;
        y = y * -1.0f;
    }
    float heading = AngleBetweenPoints(x, y, 0.0f, 0.0f);
    if (At<uint8_t*>(entity, 0x14)) {
        Matrix_SetRotateZOnly(At<uint8_t*>(entity, 0x14), heading);
    }
    At<float>(entity, 0x10) = heading;
    if ((entity[0x108] & 7) == 3) {
        Ped_SetHeading(entity, heading);
    }
    if ((entity[0x108] & 7) == 6) {
        Entity_UpdateRwFrame(entity);
        Node_Update(At<void*>(At<uint8_t*>(entity, 0x114), 0xC));
    }
    return result;
}

// 0x612C60 (thiscall): JointDriverTrack::Start: its target found (0x60B320), a joint
// constraint (+0x74) from the animation's +0x34..+0x38 (its length where negative) tying
// the ped's ROOT to the target's node +0x64; the animation started, and the target
// (+0x78) played along from its node +0x5C (or the ped put at that node's transform)
uint8_t __fastcall JointDriverTrack_StartImpl(uint8_t* self, re::Edx, uint8_t* owner) {
    self[0x44] = 0;
    At<int32_t>(self, 0x48) = -1;
    At<uint8_t*>(self, 0x4C) = nullptr;
    At<uint8_t*>(self, 0x40) = owner;
    uint8_t* ownerPed = OwnerPed(owner);
    if (!JointDriverTrack_FindTarget(self, ownerPed, At<uint8_t*>(ownerPed, 0x155C))) {
        return 0;
    }
    At<int32_t>(self, 0x24) = 0;
    if (At<uint8_t*>(self, 0x6C)) {
        uint8_t* constraint = JointConstraint_Alloc(0x80);
        if (constraint) {
            constraint = JointConstraint_Construct(constraint);
        } else {
            constraint = nullptr;
        }
        At<uint8_t*>(self, 0x74) = constraint;
        if (constraint) {
            At<int32_t>(constraint, 4) += 1;
            float length = JointDriverTrack_GetLength(self);
            float from = At<float>(self, 0x34);
            float to = At<float>(self, 0x38);
            if (from < 0.0f) {
                from = length;
            }
            if (to < 0.0f) {
                to = length;
            }
            vc::Call<void>(At<uint8_t*>(self, 0x74), 0x30, from);
            vc::Call<void>(At<uint8_t*>(self, 0x74), 0x38, to);
            At<uint8_t*>(self, 0x74)[0x7C] = self[0x68];
            uint8_t* c = At<uint8_t*>(self, 0x74);
            if (!At<uint8_t*>(At<uint8_t*>(c, 0x2C), 0x94)) {
                Release(c);
                At<uint8_t*>(self, 0x74) = nullptr;
            }
        }
    }
    uint8_t result = AnimationTrack_Start(self, owner);
    if (!At<uint8_t*>(self, 0x74) || !At<uint8_t*>(self, 0x6C)) {
        return result;
    }
    uint8_t* instance = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x74), 0x2C), 0x94);
    int32_t index;
    uint8_t* root = Skeleton_FindBone(instance, HashName(rootName), &index);
    if (!root) {
        Release(At<uint8_t*>(self, 0x74));
        At<uint8_t*>(self, 0x74) = nullptr;
        return result;
    }
    uint8_t* ped = At<uint8_t*>(self, 0x6C);
    int32_t node = 1;
    uint8_t* clump = At<uint8_t*>(ped, 0x114);
    if (clump) {
        uint8_t* found = Nodes_FindByName(At<void*>(clump, 0x94), At<const char*>(self, 0x64), &index);
        if (found) {
            node = At<int32_t>(found, 8);
        }
    }
    Constraints_Add(At<void*>(ped, 0x2EC), At<uint8_t*>(self, 0x74), node);
    At<uint8_t*>(self, 0x6C)[0x1624] = 1;
    self[0x70] = 1;
    vc::Call<void>(At<uint8_t*>(self, 0x74), 0x1C, Ped_GetMatrix(At<uint8_t*>(self, 0x6C)));
    uint8_t* target = At<uint8_t*>(self, 0x78);
    if (!target) {
        return result;
    }
    uint8_t* targetClump = At<uint8_t*>(target, 0x114);
    uint32_t hash = HashName(At<const char*>(self, 0x5C));
    if (targetClump && At<uint8_t*>(targetClump, 0x94)) {
        int32_t targetBone;
        if (Skeleton_FindBone(At<uint8_t*>(targetClump, 0x94), hash, &targetBone)) {
            float zero = 0.0f;
            AnimManager_Attach(animManager, targetClump, targetBone, 0, &zero, root,
                               At<uint8_t*>(At<uint8_t*>(self, 0x74), 0x2C), 0);
        }
        return result;
    }
    NiTransformT t;   // (rotation and translation from the node; scale 1)
    t.scale = 1.0f;
    Entity_GetNodeTransform(At<uint8_t*>(self, 0x78), hash, &t, 1);
    Matrix_FromTransform(Ped_GetMatrix(At<uint8_t*>(self, 0x6C)), &t);
    vc::Call<void>(At<uint8_t*>(self, 0x74), 0x1C, Ped_GetMatrix(At<uint8_t*>(self, 0x6C)));
    return result;
}

// 0x612F10 (thiscall): DialogJointDriverTrack::Start (the dialog animation's, +0x7C /
// +0x7D its flags)
uint8_t __fastcall DialogJointDriverTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    self[0x44] = 0;
    At<int32_t>(self, 0x48) = -1;
    At<uint8_t*>(self, 0x4C) = nullptr;
    At<uint8_t*>(self, 0x40) = owner;
    self[0x7D] = 0;
    if (self[0x7C] != 0) {
        JointDriverTrack_Start(self, owner);
        if (At<uint8_t*>(self, 0x4C)) {
            uint8_t* anim = Blend_GetAnimAt(At<uint8_t*>(self, 0x4C), 0);
            AnimManager_Update(animManager, At<uint8_t*>(self, 0x4C), anim, -1);
        }
    }
    return 1;
}

// 0x612F70 (thiscall): DialogJointDriverTrack::Update
uint8_t __fastcall DialogJointDriverTrack_Update(uint8_t* self, re::Edx, float dt) {
    if (self[0x7D] != 0) {
        return AnimationTrack_Update(self, dt);
    }
    if (!dialogState || dialogState[0x59] != 0) {
        if (self[0x7C] != 0) {
            if (At<uint8_t*>(self, 0x4C)) {
                uint8_t* anim = Blend_GetAnimAt(At<uint8_t*>(self, 0x4C), 0);
                AnimManager_Stop(animManager, At<uint8_t*>(self, 0x4C), anim, -1);
                self[0x7D] = 1;
                return 1;
            }
        } else {
            JointDriverTrack_Start(self, At<uint8_t*>(self, 0x40));
        }
        self[0x7D] = 1;
    }
    return 1;
}

// 0x612FE0 (thiscall): JointDriverParametricTrack::Start (always running)
uint8_t __fastcall JointDriverParametricTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    JointDriverTrack_Start(self, owner);
    return 1;
}

// 0x612FF0 (thiscall): AIAimParametricTrack::Update: the owner's parameter (+0x38) set
// from the aim at the ped's target: its pitch (or with +0x18 its yaw against the ped's
// +0x438, in degrees 0..360) clamped to +0x24..+0x28, mapped onto +0x1C..+0x20 (+0x4C
// keeps the last)
uint8_t __fastcall AIAimParametricTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x44));
    uint8_t* target = At<uint8_t*>(ped, 0x538);
    if (!target) {
        target = Targeting_GetTarget(At<uint8_t*>(ped, 0x1C8));
        if (!target) {
            *At<float*>(At<uint8_t*>(self, 0x44), 0x38) = At<float>(self, 0x4C);
            return 1;
        }
    }
    float lo;
    float hi;
    uint8_t ascending;
    if (At<float>(self, 0x28) < At<float>(self, 0x24)) {
        ascending = 0;
        hi = At<float>(self, 0x24);
        lo = At<float>(self, 0x28);
    } else {
        ascending = 1;
        hi = At<float>(self, 0x28);
        lo = At<float>(self, 0x24);
    }
    V3 aim;
    AIAim_GetAimVector(self, &aim, ped, target);
    float angle;
    if (self[0x18] == 0) {
        float flat = std::sqrt(std::fabs(aim.y * aim.y + aim.x * aim.x));
        float pitch = trk::CrtAtan2(flat, aim.z);
        angle = (1.57079637f - pitch) * 180.0f / kPi;
    } else {
        float base = 0.0f;
        uint8_t* other = At<uint8_t*>(OwnerPed(At<uint8_t*>(self, 0x44)), 0x438);
        if (other) {
            uint8_t* matrix = At<uint8_t*>(other, 0x14);
            if (matrix) {
                base = trk::CrtAtan2(-At<float>(matrix, 0x10), At<float>(matrix, 0x14));
            } else {
                base = At<float>(other, 0x10);
            }
        }
        float yaw = trk::CrtAtan2(aim.x, aim.y);
        angle = (base + kPi + yaw) * 180.0f / kPi;
        while (angle < 0.0f) {
            angle = angle + 360.0f;
        }
        while (360.0f < angle) {
            angle = angle - 360.0f;
        }
    }
    float clamped = angle;
    if (lo > angle) {
        clamped = lo;
    } else if (hi < angle) {
        clamped = hi;
    }
    float t = (clamped - lo) / (hi - lo);
    if (!ascending) {
        t = 1.0f - t;
    }
    float value = (At<float>(self, 0x20) - At<float>(self, 0x1C)) * t + At<float>(self, 0x1C);
    At<float>(self, 0x4C) = value;
    *At<float*>(At<uint8_t*>(self, 0x44), 0x38) = value;
    return 1;
}

int32_t SoundId(const char* name) {
    return Audio_GetSoundId(audioFocus, name);
}
// the cannon's use point's owner's ped type: 0xD (the player) plays unplaced
bool IsPlayerCannon(uint8_t* self) {
    return At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x3C)), 0x1310) == 0xD;
}

// 0x613230 (thiscall): SpudCannonFireControllerTrack::Update: pressure (+0x28, up by dt
// to +0x2C) spent +0x20 a shot every +0x1C ms (+0x30 the last) while fire (button 8) is
// held: fire's loop sound (+0x36) and request (0x16); out of pressure (+0x35 empty):
// the dry fire (+0x37) or misfire (+0x38) loop; +0x34 firing
uint8_t __fastcall SpudCannonFireControllerTrack_Update(uint8_t* self, re::Edx, float dt) {
    uint8_t* inputs = At<uint8_t*>(OwnerPed(At<uint8_t*>(self, 0x3C)), 0x2E0);
    uint8_t held = 0;
    if ((ActionInputs_GetRequest(inputs, 8)[0] & 3) != 0) {
        held = 1;
    }
    uint8_t* entity = UsePoint_GetEntity(At<uint8_t*>(self, 0x3C));
    float pressure = At<float>(self, 0x28) + dt;
    At<float>(self, 0x28) = pressure;
    float most = At<float>(self, 0x2C);
    if (pressure > most) {
        pressure = most;
    }
    At<float>(self, 0x28) = pressure;
    if (!held) {
        SpudCannon_StopSounds(self, 0, entity);
        self[0x34] = 0;
        return 1;
    }
    if (self[0x35] != 0) {
        if (At<float>(self, 0x20) < pressure) {
            self[0x35] = 0;
        }
        if (!entity) {
            self[0x34] = 0;
            return 1;
        }
        if (self[0x37] == 0 && self[0x38] == 0) {
            if (self[0x36] != 0) {
                if (IsPlayerCannon(self)) {
                    LoopSounds_PlayAt(loopSounds, SoundId(spudDryFire), 0, 3, 0);
                } else {
                    LoopSounds_PlayAtEntity(loopSounds, entity, SoundId(spudDryFire), 3, 0);
                }
                self[0x37] = 1;
            } else {
                if (IsPlayerCannon(self)) {
                    LoopSounds_PlayAt(loopSounds, SoundId(spudMisfire), 0, 3, 0);
                } else {
                    LoopSounds_PlayAtEntity(loopSounds, entity, SoundId(spudMisfire), 3, 0);
                }
                self[0x38] = 1;
            }
        }
        if (self[0x36] != 0) {
            LoopSounds_Stop(loopSounds, 0, entity, SoundId(spudFire));
            self[0x36] = 0;
            self[0x34] = 0;
            return 1;
        }
        self[0x34] = 0;
        return 1;
    }
    uint32_t now = timeInMs;
    if (!(now > uint32_t(At<int32_t>(self, 0x1C) + At<int32_t>(self, 0x30)))) {
        self[0x34] = 0;
        return 1;
    }
    if (!(At<float>(self, 0x20) < pressure)) {
        self[0x35] = 1;
        self[0x34] = 0;
        return 1;
    }
    if (entity) {
        if (self[0x37] != 0) {
            LoopSounds_Stop(loopSounds, 0, entity, SoundId(spudDryFire));
            self[0x37] = 0;
        }
        if (self[0x38] != 0) {
            LoopSounds_Stop(loopSounds, 0, entity, SoundId(spudMisfire));
            self[0x38] = 0;
        }
        if (self[0x36] == 0) {
            if (IsPlayerCannon(self)) {
                LoopSounds_PlayAt(loopSounds, SoundId(spudFire), 0, 3, 0);
            } else {
                LoopSounds_PlayAtEntity(loopSounds, entity, SoundId(spudFire), 3, 0);
            }
            self[0x36] = 1;
        }
    }
    if (self[0x34] != 0) {
        At<int32_t>(self, 0x30) += At<int32_t>(self, 0x1C);
    } else {
        At<uint32_t>(self, 0x30) = now;
    }
    At<float>(self, 0x28) = At<float>(self, 0x28) - At<float>(self, 0x20);
    ActionInputs_SetRequest(inputs, 0x16, 1, 0.0f);
    self[0x34] = 1;
    return 1;
}

// 0x613500 (thiscall): SpudCannonFireControllerTrack::End: its sounds stopped
uint8_t __fastcall SpudCannonFireControllerTrack_End(uint8_t* self, re::Edx) {
    SpudCannon_StopSounds(self, 0, UsePoint_GetEntity(At<uint8_t*>(self, 0x3C)));
    return 1;
}

// 0x613520 (thiscall): AnimationParametricTrack::Start: the animation, then its time set
// from the owner's parameter (+0x58..+0x5C onto +0x28..+0x2C, the animation's length
// where that's negative)
uint8_t __fastcall AnimationParametricTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x40) = owner;
    AnimationTrack_Start(self, owner);
    float from = At<float>(self, 0x58);
    float to = At<float>(self, 0x5C);
    float outFrom = At<float>(self, 0x28);
    float outTo = At<float>(self, 0x2C);
    uint8_t* anim = Blend_GetAnim(At<uint8_t*>(self, 0x4C), At<int32_t>(self, 0x24));
    if (!anim) {
        return 0;
    }
    if (outTo < 0.0f) {
        outTo = At<float>(At<uint8_t*>(anim, 0x18), 0xC);
    }
    float time = RemapRange(*At<float*>(At<uint8_t*>(self, 0x40), 0x38), from, to, outFrom, outTo);
    AnimManager_Play(animManager, At<uint8_t*>(self, 0x4C), anim, 0, time, 0, 1);
    return 1;
}

} // namespace

#define TRSPINE_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

TRSPINE_REVERSED("TargetClearTrack::Start", 0x00611E50, &TargetClearTrack_Start);
TRSPINE_REVERSED("SpineControlTrack::Start", 0x00611E70, &SpineControlTrack_Start);
TRSPINE_REVERSED("SpineControlTrack::End", 0x00611EE0, &SpineControlTrack_End);
TRSPINE_REVERSED("SpineControlDisableTrack::Start", 0x00611F00, &SpineControlDisableTrack_Start);
TRSPINE_REVERSED("SpineControlDisableTrack::Update", 0x00611F50, &SpineControlDisableTrack_Update);
TRSPINE_REVERSED("SpineControlDisableTrack::End", 0x00612020, &SpineControlDisableTrack_End);
TRSPINE_REVERSED("SpineControlSetTargetTrack::Start", 0x00612040, &SpineControlSetTargetTrack_Start);
TRSPINE_REVERSED("SpineControlSetTargetTrack::Update", 0x00612070, &SpineControlSetTargetTrack_Update);
TRSPINE_REVERSED("SpineControlSetTargetTrack::End", 0x006120B0, &SpineControlSetTargetTrack_End);
TRSPINE_REVERSED("usePelvisForHealthMeterTrack::Start", 0x006120D0, &PelvisHealthMeterTrack_Start);
TRSPINE_REVERSED("usePelvisForHealthMeterTrack::End", 0x00612130, &PelvisHealthMeterTrack_End);
TRSPINE_REVERSED("FallingDamageTrack::Start", 0x00612170, &FallingDamageTrack_Start);
TRSPINE_REVERSED("FallingDamageTrack::Update", 0x006121F0, &FallingDamageTrack_Update);
TRSPINE_REVERSED("FallingDamageTrack::End", 0x00612200, &FallingDamageTrack_End);
TRSPINE_REVERSED("FallingDamageHandleTrack::Start", 0x006122D0, &FallingDamageHandleTrack_Start);
TRSPINE_REVERSED("AttackTimerResetTrack::Start", 0x00612330, &AttackTimerResetTrack_Start);
TRSPINE_REVERSED("Model::FindDummy_612380", 0x00612380, &Model_FindDummy);
TRSPINE_REVERSED("AnimationTrack::PlayBase", 0x006123D0, &AnimationTrack_PlayBaseImpl);
TRSPINE_REVERSED("AnimationTrack::Start", 0x006124C0, &AnimationTrack_StartImpl);
TRSPINE_REVERSED("DialogAnimationTrack::Start", 0x00612830, &DialogAnimationTrack_Start);
TRSPINE_REVERSED("DialogAnimationTrack::Update", 0x00612890, &DialogAnimationTrack_Update);
TRSPINE_REVERSED("HitAnimationTrack::Start", 0x00612910, &HitAnimationTrack_Start);
TRSPINE_REVERSED("JointDriverTrack::Start", 0x00612C60, &JointDriverTrack_StartImpl);
TRSPINE_REVERSED("DialogJointDriverTrack::Start", 0x00612F10, &DialogJointDriverTrack_Start);
TRSPINE_REVERSED("DialogJointDriverTrack::Update", 0x00612F70, &DialogJointDriverTrack_Update);
TRSPINE_REVERSED("JointDriverParametricTrack::Start", 0x00612FE0, &JointDriverParametricTrack_Start);
TRSPINE_REVERSED("AIAimParametricTrack::Update", 0x00612FF0, &AIAimParametricTrack_Update);
TRSPINE_REVERSED("SpudCannonFireControllerTrack::Update", 0x00613230, &SpudCannonFireControllerTrack_Update);
TRSPINE_REVERSED("SpudCannonFireControllerTrack::End", 0x00613500, &SpudCannonFireControllerTrack_End);
TRSPINE_REVERSED("AnimationParametricTrack::Start", 0x00613520, &AnimationParametricTrack_Start);
