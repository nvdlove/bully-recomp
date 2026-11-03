// The action tree's locomotion tracks (the small ones): the jump's end, liquid drag,
// retarding, the new locomotion animation, the loco sync animation, footstep effects and
// sounds (the ped's surface / water effects), gravity, goofy foot switching, the orient
// offset and target facing. 0x60E240..0x60F3D0.

#include <cmath>
#include <cstdint>
#include <cstring>

#include "game/ActionTracksCommon.h"
#include "reversed.h"

using namespace actc;

namespace {

constexpr uint32_t kVtLocomotionAnimationNewTrack = 0x00930EDC;

// an AE effect proxy
struct FxProxy {
    re::Ptr32<void> vtable;
    re::Ptr32<void> effect;
};
static_assert(sizeof(FxProxy) == 8);

// the transform an effect is placed with: GroundTransform's 3x4 and a scale
struct FxTransform {
    float m[12];
    float scale;
};
static_assert(sizeof(FxTransform) == 0x34);

inline void* const sounds_CF3DA0 = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const waterLevels = reinterpret_cast<void*>(0x00BC5DA8);
inline void* const surfaceFx = reinterpret_cast<void*>(0x020C8BA0);   // per surface footstep effects
GAME_VAR(void*, effectLibrary, 0x00B6F1E8);
GAME_VAR(void*, effectManager, 0x00B6F1D4);
GAME_VAR(const char*, defaultFootstepSound, 0x00B0B0F0);
GAME_VAR(uint8_t*, motionPool_20C9028, 0x020C9028);   // AM motion tracks: +0 used, +4 free, +8 size, +0x14 list
GAME_VAR(float, locoSyncBlend_B0B13C, 0x00B0B13C);
GAME_VAR(float, goofyFootDot_B0B140, 0x00B0B140);
GAME_VAR(float, targetFaceRate_B0BC3C, 0x00B0BC3C);
GAME_VAR(int32_t, noFootstepModelA, 0x00BF42D4);
GAME_VAR(int32_t, noFootstepModelB, 0x00BF42D0);
GAME_VAR(int32_t, noFootstepModelC, 0x00BF42CC);
GAME_VAR(int32_t, noFootstepModelD, 0x00BF42DC);
GAME_VAR(int32_t, noAimWeaponA_BF4430, 0x00BF4430);
GAME_VAR(int32_t, noAimWeaponB_BF433C, 0x00BF433C);
GAME_VAR(void*, eventManager_C2C1A0, 0x00C2C1A0);
GAME_VAR(uint8_t, currentChapter, 0x00A147E0);
GAME_ARRAY(int32_t, chapterSeason, 0x00A147EC);
inline const char* const treadWater = reinterpret_cast<const char*>(0x009316D0);   // "TreadWater"
inline const char* const treadAcid = reinterpret_cast<const char*>(0x009316C4);    // "TreadAcid"
inline const char* const treadMud = reinterpret_cast<const char*>(0x009316B8);     // "TreadMud"
inline const char* const leftFoot = reinterpret_cast<const char*>(0x00918310);     // "LeftFoot"
inline const char* const rightFoot = reinterpret_cast<const char*>(0x0091831C);    // "RightFoot"

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __thiscall, Track_Construct, 0x0061A880, uint8_t* track);
GAME_FN(uint32_t*, __thiscall, NameKey_Default, 0x0066BEA0, uint32_t* key);
GAME_FN(uint8_t*, __thiscall, Blend_GetAnimAt, 0x006B1FA0, uint8_t* blend, int32_t slot);
GAME_FN(void, __thiscall, AnimManager_Play, 0x006BCCD0, void* manager, uint8_t* blend, uint8_t* anim, int32_t slot);
GAME_FN(void, __thiscall, AnimManager_BlendOut, 0x006BD480, void* manager, uint8_t* blend, uint8_t* anim,
        float time);
GAME_FN(uint8_t*, __thiscall, MotionTrack_Construct, 0x007315C0, uint8_t* track);
GAME_FN(uint8_t*, __thiscall, Entries2C_Find, 0x006074A0, uint8_t* dict, const char* name, uint32_t* index);
GAME_FN(void, __thiscall, AE_Proxy_Construct, 0x00668E90, FxProxy* proxy);
GAME_FN(void, __thiscall, AE_Proxy_Destruct, 0x00668EB0, FxProxy* proxy);
GAME_FN(void, __thiscall, AE_Manager_Start, 0x00668290, void* manager, const int32_t* type, FxProxy* proxy, int);
GAME_FN(void, __thiscall, AE_Proxy_SetTransform, 0x00668EC0, FxProxy* proxy, const FxTransform* transform);
GAME_FN(void, __thiscall, AE_Proxy_SetPosition, 0x00669040, FxProxy* proxy, const float* pos, int);
GAME_FN(void, __thiscall, AE_Proxy_SetDirection, 0x00669360, FxProxy* proxy, const V3* dir);
GAME_FN(void, __thiscall, AE_Proxy_SetStrength_x64, 0x00668C40, FxProxy* proxy, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void AE_Proxy_SetStrength(FxProxy* a0, float a1) {
    return AE_Proxy_SetStrength_x64(a0, re::BitsOf(a1));
}
GAME_FN(void, __thiscall, AE_Proxy_SetRate_x64, 0x00668C10, FxProxy* proxy, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void AE_Proxy_SetRate(FxProxy* a0, float a1) {
    return AE_Proxy_SetRate_x64(a0, re::BitsOf(a1));
}
GAME_FN(void, __cdecl, GroundTransform, 0x00429520, FxTransform* out, float x, float y, float z, uint32_t w,
        float dirX, float dirY, float dirZ, uint32_t dirW, float zOffset);
GAME_FN(uint8_t, __thiscall, WaterLevels_Get, 0x006D4A20, void* levels, float* level, int32_t* kind, const float* pos);
GAME_FN(uint16_t, __thiscall, EffectLibrary_Find, 0x00667B60, void* library, const char* name);
GAME_FN(void, __thiscall, Timer_Reset, 0x00669FF0, uint8_t* timer);
GAME_FN(float, __thiscall, Timer_GetSeconds, 0x0066A010, uint8_t* timer);
GAME_FN(uint8_t, __thiscall, SurfaceFx_HasTrail, 0x0072E5E0, void* table, int32_t surface);
GAME_FN(uint16_t, __thiscall, SurfaceFx_Trail, 0x0072E600, void* table, int32_t surface);
GAME_FN(uint8_t, __thiscall, SurfaceFx_HasStep, 0x0072E580, void* table, int32_t surface);
GAME_FN(float, __thiscall, SurfaceFx_StepStrength, 0x0072E5A0, void* table, int32_t surface);
GAME_FN(uint16_t, __thiscall, SurfaceFx_Step, 0x0072E590, void* table, int32_t surface);
GAME_FN(uint8_t, __thiscall, SurfaceFx_HasWinter, 0x0072E5B0, void* table, int32_t surface);
GAME_FN(uint16_t, __thiscall, SurfaceFx_Winter, 0x0072E5D0, void* table, int32_t surface);
GAME_FN(uint8_t, __cdecl, IsCurrentAreaInterior, 0x0040DF80);
GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_FN(uint8_t*, __thiscall, Entity_GetMatrix, 0x008B6120, uint8_t* entity);
GAME_FN(void*, __thiscall, Nodes_FindByName, 0x004A6BB0, void* nodes, const char* name, int32_t* index);
GAME_FN(void, __thiscall, Bones_GetPosition, 0x006C1190, void* manager, uint8_t* anim, int32_t bone, V3* out, uintptr_t);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const V3* v);
GAME_FN(float, __cdecl, Clamp, 0x0041A470, float value, float low, float high);
GAME_FN(void, __thiscall, MotionIntention_SetEventA, 0x005F3770, uint8_t* intention, int set);
GAME_FN(void, __thiscall, MotionIntention_SetEventB, 0x005F37A0, uint8_t* intention, int set);
GAME_FN(void, __cdecl, Ped_FootstepEffects, 0x0060E730, uint8_t* ped, float* depth, int32_t* surface);
GAME_FN(void, __cdecl, SpawnFootEffect, 0x0060E680, uint32_t effect, const float* pos, float dirX, float dirY,
        float dirZ);
GAME_FN(int32_t, __thiscall, Sounds_GetId, 0x00595B80, void* sounds, const char* name);
GAME_FN(uint8_t, __thiscall, Sounds_IsFootstep, 0x005956A0, void* sounds, int32_t id);
GAME_FN(void, __thiscall, Footsteps_Play, 0x0059CC00, void* sounds, uint8_t* ped, int32_t id, uint32_t volume,
        float depth, int32_t surface);
GAME_FN(int32_t, __thiscall, Sounds_Play, 0x0059B8A0, void* sounds, int32_t sound, const float* pos, uint32_t, uintptr_t,
        int, int, int, int, uintptr_t, uintptr_t, int, int, float, int);
GAME_FN(float, __cdecl, AngleBetweenPoints, 0x00432D10, float x1, float y1, float x2, float y2);
GAME_FN(float, __cdecl, LimitAngle, 0x00432EE0, float angle);
GAME_FN(void, __thiscall, Matrix_SetRotateZOnly, 0x00412C20, uint8_t* matrix, float angle);
GAME_FN(void, __thiscall, Ped_SetHeading, 0x004734C0, uint8_t* ped, float heading);
GAME_FN(void, __thiscall, Ped_UpdateMoveDirection, 0x00474570, uint8_t* ped);
GAME_FN(float, __cdecl, TurnHeading, 0x00608380, float x, float y, float z, float toX, float toY, float toZ,
        float accel, float max, float rate, float* speed);
GAME_FN(uint8_t, __thiscall, Ped_EveryNthFrame, 0x00465A60, uint8_t* ped, uint32_t n);
GAME_FN(void, __thiscall, EventMgr_Send, 0x004A6670, void* self, int event, uint8_t* ped, uint8_t* other, uintptr_t,
        float, float);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, uint8_t* entity, re::Ptr32<uint8_t>* ref);
GAME_FN(void, __thiscall, HeadLook_SetTarget, 0x004A3B60, uint8_t* headLook, uint8_t* target);

// a copy of a track (the locomotion animation's 0x34 bytes) for the blend to own
uint8_t* CopyLocoAnimTrack(uint8_t* self) {
    uint8_t* copy = static_cast<uint8_t*>(Malloc(0x34));
    if (copy) {
        Track_Construct(copy);
        At<uint32_t>(copy, 0) = kVtLocomotionAnimationNewTrack;
        NameKey_Default(&At<uint32_t>(copy, 0x18));
    }
    // (the game copies over it unchecked: a failed allocation writes to null)
    std::memcpy(copy, self, 0x34);
    return copy;
}

// 0x60E240 (thiscall): LocomotionJumpTrack::End: the ped lands (+0x1334) and its jump
// state (+0x1474) clears
uint8_t __fastcall LocomotionJump_End(uint8_t* self, re::Edx) {
    At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x30)), 0x1334) = 1;
    At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x30)), 0x1474) = 0;
    return 1;
}

// 0x60E270 (thiscall): LocomotionLiquidTrack::Update: under the surface the ped's vertical
// speed (+0x1D58 the rise) is dragged by depth (+0x18 the deepest counted, +0x20 the scale,
// +0x24 the drag) and applied to its move speed and velocity z
uint8_t __fastcall LocomotionLiquid_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x28));
    ped[0x1BC] &= 0xFD;
    float deepest = At<float>(self, 0x18);
    float z = Pos(ped)[2];
    float scale = At<float>(self, 0x20);
    float v = At<float>(ped, 0x1D58);
    if (z < 0.0f) {
        deepest = -deepest;
        if (deepest > z) {
            z = deepest;
        }
        float t = z / scale * v;
        v = t - v - At<float>(OwnerPed(At<uint8_t*>(self, 0x28)), 0x12C) * At<float>(self, 0x24);
    }
    float dv = timeStep * v;
    uint8_t* p = OwnerPed(At<uint8_t*>(self, 0x28));
    At<float>(p, 0x1710) += dv;
    At<float>(OwnerPed(At<uint8_t*>(self, 0x28)), 0x12C) += dv;
    return 1;
}

// 0x60E360 (thiscall): LocomotionRetardTrack::Update: the motion's speed (+0x1C) and / or
// the ped's move speed scaled by +0x18
uint8_t __fastcall LocomotionRetard_Update(uint8_t* self, re::Edx, float) {
    if (self[0x1C]) {
        uint8_t* m = At<uint8_t*>(At<uint8_t*>(self, 0x20), 0x14);
        At<float>(m, 0x1C) = At<float>(self, 0x18) * At<float>(m, 0x1C);
    }
    if (self[0x1D]) {
        uint8_t* m = At<uint8_t*>(OwnerPed(At<uint8_t*>(self, 0x20)), 0x2E0);
        At<float>(m, 0x1C) = At<float>(self, 0x18) * At<float>(m, 0x1C);
    }
    return 1;
}

// 0x60E3A0 (thiscall): LocomotionAnimationNewTrack::Start: a copy of this track is kept in
// the locomotion's first free slot (+0x54, ten); the locomotion animation replaces the
// one playing in its slot (an animation of that type restarts at full weight)
uint8_t __fastcall LocomotionAnimationNew_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x30) = owner;
    uint8_t* loco = At<uint8_t*>(owner, 0x18);
    uint8_t* blend = At<uint8_t*>(OwnerPed(owner), 0x114);
    if (!loco) {
        return 1;
    }
    for (int i = 0; i < 10; ++i) {
        if (At<uint8_t*>(loco, 0x54 + i * 4) == nullptr) {
            At<uint8_t*>(loco, 0x54 + i * 4) = CopyLocoAnimTrack(self);
            break;
        }
    }
    int32_t slot = At<int32_t>(At<uint8_t*>(loco, 0x4C), 0x40);
    uint8_t* anim = Blend_GetAnimAt(blend, slot);
    if (anim != loco) {
        At<uint8_t>(At<uint8_t*>(self, 0x30), 0x1C) = 1;
        if (anim) {
            int16_t id = acttree::TypeIdAt<0x01B65254, 0x01B65250>();
            auto isA = reinterpret_cast<uint8_t(__thiscall*)(uint8_t*, int)>(static_cast<void*>(At<re::Ptr32<void>*>(anim, 0)[0]));
            if (isA(anim, uint16_t(id))) {
                At<float>(anim, 0xC) = 1.0f;
                At<float>(anim, 0x10) = 0.0f;
            }
        }
        AnimManager_Play(animManager, blend, loco, slot);
    }
    return 1;
}

// 0x60E4B0 (thiscall): AnimationLocoSyncTrack::Start: a motion track from the pool
// (+0x44) for the ped, blending in over +0x3C (negative: the default; zero: at once), a
// copy of this track, the animation named +0x34 (else the dictionary's first) when a
// slot (+0x38) is given, played in that slot
uint8_t __fastcall AnimationLocoSync_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x30) = owner;
    uint8_t* blend = At<uint8_t*>(OwnerPed(owner), 0x114);
    uint8_t* pool = motionPool_20C9028;
    bool room = At<int32_t>(pool, 8) >= 0xA8;
    uint8_t* motion = nullptr;
    if (At<int32_t>(pool, 4) > 0 && room) {
        At<int32_t>(pool, 4) += -1;
        At<int32_t>(pool, 0) += 1;
        uint8_t* item = At<re::Ptr32<uint8_t>*>(pool, 0x14)[At<int32_t>(pool, 4)];
        if (item) {
            motion = MotionTrack_Construct(item);
        }
    }
    At<uint8_t*>(self, 0x44) = motion;
    At<uint8_t*>(motion, 0x48) = OwnerPed(At<uint8_t*>(self, 0x30));
    At<int32_t>(At<uint8_t*>(self, 0x44), 0x4C) = 0;
    float t = At<float>(self, 0x3C);
    uint8_t* m = At<uint8_t*>(self, 0x44);
    if (!(t >= 0.0f)) {   // (negative or NaN: the game tests the sign flag)
        t = locoSyncBlend_B0B13C;
    }
    if (t == 0.0f) {
        At<float>(m, 0x10) = 0.0f;
        At<float>(m, 0x14) = 0.0f;
        At<float>(m, 0xC) = 1.0f;
    } else {
        float blendIn = -t;
        At<float>(m, 0x10) = blendIn;
        At<float>(m, 0x14) = blendIn;
        At<float>(m, 0xC) = 0.0f;
    }
    if (At<uint8_t*>(self, 0x44)) {
        At<uint8_t*>(At<uint8_t*>(self, 0x44), 0x54) = CopyLocoAnimTrack(self);
    }
    int32_t slot = At<int32_t>(self, 0x38);
    if (slot > 0) {
        uint8_t* dict = At<uint8_t*>(At<uint8_t*>(blend, 0x94), 0x10);
        uint8_t* entry = nullptr;
        if (At<const char*>(self, 0x34)) {
            entry = Entries2C_Find(dict, At<const char*>(self, 0x34), nullptr);
        }
        if (!entry && At<uint32_t>(dict, 0xC) != 0) {
            entry = At<uint8_t*>(dict, 8);
        }
        if (entry) {
            At<uint8_t*>(At<uint8_t*>(self, 0x44), 8) = entry;
        }
    }
    AnimManager_Play(animManager, blend, At<uint8_t*>(self, 0x44), slot);
    return 1;
}

// 0x60E620 (thiscall): AnimationLocoSyncTrack::End: its motion blended out over +0x40
// (negative: the default, -1)
uint8_t __fastcall AnimationLocoSync_End(uint8_t* self, re::Edx) {
    uint8_t* blend = At<uint8_t*>(OwnerPed(At<uint8_t*>(self, 0x30)), 0x114);
    if (At<int32_t>(self, 0x38) > 0 && At<uint8_t*>(self, 0x44)) {
        float t = At<float>(self, 0x40);
        if (t < 0.0f) {
            t = -1.0f;
        }
        AnimManager_BlendOut(animManager, blend, At<uint8_t*>(self, 0x44), t);
    }
    return 1;
}

// 0x60E680 (cdecl): a footstep effect (its low 16 bits) placed on the ground at pos
// facing dir
void __cdecl SpawnFootEffectAt(uint32_t effect, const float* pos, float dirX, float dirY, float dirZ) {
    FxProxy proxy;
    AE_Proxy_Construct(&proxy);
    int32_t type = int32_t(effect & 0xFFFF);
    AE_Manager_Start(effectManager, &type, &proxy, 0);
    FxTransform transform;
    transform.scale = 1.0f;
    // (the 4th floats of both vectors are left unset on the game's stack; unused)
    GroundTransform(&transform, pos[0], pos[1], pos[2], 0, dirX, dirY, dirZ, 0, 0.05f);
    AE_Proxy_SetTransform(&proxy, &transform);
    AE_Proxy_Destruct(&proxy);
}

// 0x60E730 (cdecl): a ped's footstep effects: in water (depth below the surface, the
// liquid's kind as the surface) its tread effect; else its surface's trail, step
// (strength by speed) and, in a winter interior, winter effects at the lower foot;
// +0x1BF remembers the liquid kind, +0x162C the trail's surface (+0x1628 its timer)
void __cdecl FootstepEffects(uint8_t* ped, float* depth, int32_t* surface) {
    int32_t model = *reinterpret_cast<int16_t*>(ped + 0x10E);
    if (model == noFootstepModelA || model == noFootstepModelB || model == noFootstepModelC ||
        model == noFootstepModelD) {
        return;
    }
    if (At<int32_t>(ped, 0xA4) != 0) {
        return;
    }
    *surface = 4;
    *depth = 0.0f;
    float level;
    int32_t kind;
    if (WaterLevels_Get(waterLevels, &level, &kind, Pos(ped))) {
        *surface = kind;
        float* p = Pos(ped);
        float pos[3] = {p[0], p[1], p[2]};
        *depth = pos[2] - level;
        pos[2] = level;
        float under = Pos(ped)[2] - At<float>(ped, 0x16D0);
        if (!(under > -1.2f)) {
            ped[0x1BF] = uint8_t(kind);
            return;
        }
        uint32_t effect = 0xFFFFFFFF;
        if (kind == 1) {
            effect = EffectLibrary_Find(effectLibrary, treadWater);
            At<int32_t>(ped, 0x162C) = 0x1F;
            Timer_Reset(ped + 0x1628);
        } else if (kind == 3) {
            effect = EffectLibrary_Find(effectLibrary, treadAcid);
        } else if (kind == 2) {
            effect = EffectLibrary_Find(effectLibrary, treadMud);
            At<int32_t>(ped, 0x162C) = 5;
            Timer_Reset(ped + 0x1628);
        }
        FxProxy proxy;
        AE_Proxy_Construct(&proxy);
        int32_t type = int32_t(effect & 0xFFFF);
        AE_Manager_Start(effectManager, &type, &proxy, 0);
        AE_Proxy_SetPosition(&proxy, pos, 0);
        V3 up = {0.0f, 0.0f, 1.0f};
        AE_Proxy_SetDirection(&proxy, &up);
        AE_Proxy_Destruct(&proxy);
        ped[0x1BF] = uint8_t(kind);
        return;
    }
    // (not in water: kind is -1 as the lookup leaves it)
    uint32_t trail = 0xFFFFFFFF;
    float strength = 0.0f;
    uint32_t step = 0xFFFFFFFF;
    uint32_t winter = 0xFFFFFFFF;
    if (SurfaceFx_HasTrail(surfaceFx, At<int32_t>(ped, 0x162C))) {
        trail = SurfaceFx_Trail(surfaceFx, At<int32_t>(ped, 0x162C));
    }
    bool leaveTrail = false;
    if (At<int32_t>(ped, 0x162C) == int32_t(ped[0x1BE])) {
        if (trail != 0xFFFFFFFF) {
            Timer_Reset(ped + 0x1628);
        }
    } else {
        if (trail != 0xFFFFFFFF) {
            if (Timer_GetSeconds(ped + 0x1628) < 5.0f) {
                if (SurfaceFx_HasTrail(surfaceFx, At<int32_t>(ped, 0x162C))) {
                    leaveTrail = true;
                }
            } else {
                At<int32_t>(ped, 0x162C) = -1;
            }
        }
        if (SurfaceFx_HasTrail(surfaceFx, ped[0x1BE])) {
            At<int32_t>(ped, 0x162C) = ped[0x1BE];
        }
    }
    if (SurfaceFx_HasTrail(surfaceFx, ped[0x1BE])) {
        Timer_Reset(ped + 0x1628);
        At<int32_t>(ped, 0x162C) = ped[0x1BE];
    }
    if (SurfaceFx_HasStep(surfaceFx, ped[0x1BE])) {
        strength = SurfaceFx_StepStrength(surfaceFx, ped[0x1BE]);
        step = SurfaceFx_Step(surfaceFx, ped[0x1BE]);
    }
    if (SurfaceFx_HasWinter(surfaceFx, ped[0x1BE])) {
        winter = SurfaceFx_Winter(surfaceFx, ped[0x1BE]);
    }
    bool snow;
    if (chapterSeason[currentChapter] == 2 && IsCurrentAreaInterior()) {
        snow = true;
        if (winter == 0xFFFFFFFF) {
            ped[0x1BF] = uint8_t(kind);
            return;
        }
    } else {
        snow = false;
        if (step == 0xFFFFFFFF) {
            ped[0x1BF] = uint8_t(kind);
            return;
        }
    }
    float* p = Pos(ped);
    float pos[3] = {p[0], p[1], p[2]};
    V3 fwd;
    Placeable_GetForward(ped, &fwd);
    Entity_GetMatrix(ped);
    pos[2] = float(pos[2] + 0.1);
    float ox = float(fwd.x * 0.2);
    float oy = float(fwd.y * 0.2);
    float oz = float(fwd.z * 0.2);
    pos[0] = ox + pos[0];
    pos[1] = pos[1] + oy;
    pos[2] = oz + pos[2];
    int32_t leftIndex = 0;
    int32_t rightIndex = 0;
    V3 left = {0.0f, 0.0f, 0.0f};
    V3 right = {0.0f, 0.0f, 0.0f};
    uint8_t* blend = At<uint8_t*>(ped, 0x114);
    bool hasLeft = Nodes_FindByName(At<void*>(blend, 0x94), leftFoot, &leftIndex) != nullptr;
    if (hasLeft) {
        Bones_GetPosition(animManager, At<uint8_t*>(ped, 0x114), leftIndex, &left, 0);
    }
    bool hasRight = Nodes_FindByName(At<void*>(At<uint8_t*>(ped, 0x114), 0x94), rightFoot, &rightIndex) != nullptr;
    if (hasRight) {
        Bones_GetPosition(animManager, At<uint8_t*>(ped, 0x114), rightIndex, &right, 0);
    }
    if (hasLeft) {
        if (!hasRight || left.z < right.z) {
            pos[0] = left.x;
            pos[1] = left.y;
            pos[2] = left.z;
        } else {
            pos[0] = right.x;
            pos[1] = right.y;
            pos[2] = right.z;
        }
    } else if (hasRight) {
        pos[0] = right.x;
        pos[1] = right.y;
        pos[2] = right.z;
    }
    if (snow) {
        SpawnFootEffect(winter, pos, fwd.x, fwd.y, fwd.z);
        ped[0x1BF] = uint8_t(kind);
        return;
    }
    if (leaveTrail && trail != 0xFFFFFFFF) {
        SpawnFootEffect(trail, pos, fwd.x, fwd.y, fwd.z);
    }
    FxProxy proxy;
    AE_Proxy_Construct(&proxy);
    int32_t type = int32_t(step & 0xFFFF);
    AE_Manager_Start(effectManager, &type, &proxy, 0);
    AE_Proxy_SetPosition(&proxy, pos, 0);
    V3 velocity = {At<float>(ped, 0x124), At<float>(ped, 0x128), At<float>(ped, 0x12C)};
    float speed = float(double(float(double(Vector_Magnitude(&velocity)) - 0.029999999329447746)) / 0.07899999804794788);
    float c = Clamp(speed, 0.0f, 1.0f);
    float value = strength + (1.0f - strength) * c;
    AE_Proxy_SetStrength(&proxy, value);
    AE_Proxy_SetRate(&proxy, Clamp(value, 0.25f, 1.0f));
    AE_Proxy_Destruct(&proxy);
    ped[0x1BF] = uint8_t(kind);
}

// 0x60ED90 (thiscall): LocomotionEventTrack::Update: on the motion intention's step events
// (+0x30 left, +0x31 right) the footstep effects and the step's sound (+0x18 / +0x24
// its name, else the default; +0x20 / +0x2C its volume; +0x38 the side, -1 / 1)
uint8_t __fastcall LocomotionEvent_Update(uint8_t* self, re::Edx, float) {
    float* p = Pos(OwnerPed(At<uint8_t*>(self, 0x34)));
    float pos[3] = {p[0], p[1], p[2]};
    pos[2] = float(pos[2] + 0.5);
    // (the game leaves these unset when FootstepEffects returns early for the ped)
    int32_t surface = 0;
    float depth = 0.0f;
    if (At<uint8_t*>(At<uint8_t*>(self, 0x34), 0x14)[0x30]) {
        MotionIntention_SetEventA(At<uint8_t*>(At<uint8_t*>(self, 0x34), 0x14), 0);
        Ped_FootstepEffects(OwnerPed(At<uint8_t*>(self, 0x34)), &depth, &surface);
        const char* name = At<const char*>(self, 0x18) ? At<const char*>(self, 0x18) : defaultFootstepSound;
        int32_t id = Sounds_GetId(sounds_CF3DA0, name);
        At<float>(self, 0x38) = -1.0f;
        if (Sounds_IsFootstep(sounds_CF3DA0, id)) {
            Footsteps_Play(sounds_CF3DA0, OwnerPed(At<uint8_t*>(self, 0x34)), id, At<uint32_t>(self, 0x20), depth,
                           surface);
        } else {
            Sounds_Play(sounds_CF3DA0, id, pos, At<uint32_t>(self, 0x20), 0, 0, 0, 0, 0, 0, 0, 6, 1, 1.0f, 0);
        }
    }
    if (At<uint8_t*>(At<uint8_t*>(self, 0x34), 0x14)[0x31]) {
        MotionIntention_SetEventB(At<uint8_t*>(At<uint8_t*>(self, 0x34), 0x14), 0);
        Ped_FootstepEffects(OwnerPed(At<uint8_t*>(self, 0x34)), &depth, &surface);
        const char* name = At<const char*>(self, 0x24) ? At<const char*>(self, 0x24) : defaultFootstepSound;
        int32_t id = Sounds_GetId(sounds_CF3DA0, name);
        At<float>(self, 0x38) = 1.0f;
        if (Sounds_IsFootstep(sounds_CF3DA0, id)) {
            Footsteps_Play(sounds_CF3DA0, OwnerPed(At<uint8_t*>(self, 0x34)), id, At<uint32_t>(self, 0x2C), depth,
                           surface);
            return 1;
        }
        Sounds_Play(sounds_CF3DA0, id, pos, At<uint32_t>(self, 0x2C), 0, 0, 0, 0, 0, 0, 0, 6, 1, 1.0f, 0);
    }
    return 1;
}

// 0x60EF50 (thiscall): GravityTrack::Start: the ped's gravity flag (+0x1BC bit 1) from
// +0x18 and its rise speed (+0x1D58) from +0x1C
uint8_t __fastcall Gravity_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (ped) {
        ped[0x1BC] ^= uint8_t(((self[0x18] * 2) ^ ped[0x1BC]) & 2);
        At<float>(ped, 0x1D58) = At<float>(self, 0x1C);
    }
    return 1;
}

// 0x60EF80 (thiscall): AllowSwitchGoofyFootTrack::Start: the goofy foot flag (+0x1488)
// clears once moving forward faster than the threshold, sets once slower than its
// negative
uint8_t __fastcall AllowSwitchGoofyFoot_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    uint8_t* ped = OwnerPed(owner);
    V3 f;
    V3* fwd = Placeable_GetForward(ped, &f);
    float d = At<float>(ped, 0x128) * fwd->y + At<float>(ped, 0x124) * fwd->x + At<float>(ped, 0x12C) * fwd->z;
    float g = goofyFootDot_B0B140;
    if (At<int32_t>(ped, 0x1488) != 0) {
        if (-g < d) {
            At<int32_t>(ped, 0x1488) = 0;
        }
        return 1;
    }
    if (g > d) {
        At<int32_t>(ped, 0x1488) = 1;
    }
    return 1;
}

// 0x60F020 (thiscall): OrientOffSetTrack::Start: the ped turned by +0x18 from its heading
// (kept at +0x40)
uint8_t __fastcall OrientOffSet_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x3C) = owner;
    uint8_t* ped = OwnerPed(owner);
    V3 fwd;
    Placeable_GetForward(ped, &fwd);
    float heading = AngleBetweenPoints(fwd.x, fwd.y, 0.0f, 0.0f);
    At<float>(self, 0x40) = heading;
    float h = LimitAngle(heading + At<float>(self, 0x18));
    if (At<uint8_t*>(ped, 0x14)) {
        Matrix_SetRotateZOnly(At<uint8_t*>(ped, 0x14), h);
    }
    At<float>(ped, 0x10) = h;
    Ped_SetHeading(ped, h);
    Ped_UpdateMoveDirection(ped);
    return 1;
}

// 0x60F0C0 (thiscall): TargetFaceTrack::Start: the turn state cleared, the ped's turn
// limit (+0x1C8 +0x40) from +0x2C when set (not -1), then a first tiny update
uint8_t __fastcall TargetFace_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<float>(self, 0x44) = 0.0f;
    At<uint8_t*>(self, 0x40) = nullptr;
    self[0x48] = 0;
    At<uint8_t*>(self, 0x3C) = owner;
    if (At<float>(self, 0x2C) != -1.0f) {
        At<float>(At<uint8_t*>(OwnerPed(owner), 0x1C8), 0x40) = At<float>(self, 0x2C);
    }
    auto update = reinterpret_cast<uint8_t(__thiscall*)(uint8_t*, float)>(static_cast<void*>(At<re::Ptr32<void>*>(self, 0)[3]));
    update(self, 1e-6f);
    return 1;
}

// 0x60F110 (thiscall): TargetFaceTrack's target set: a player aiming a weapon at a ped
// sends the aim event (every 8th frame); the reference moves (+0x40), with the head
// look (+0x37)
void __fastcall TargetFace_SetTarget(uint8_t* self, re::Edx, uint8_t* target) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x3C));
    if (At<int32_t>(ped, 0x1310) == 0xD && At<int32_t>(ped, 0x142C) == 1 && Ped_EveryNthFrame(ped, 8)) {
        uint8_t* weapon = At<uint8_t*>(ped, 0x1D0);
        int16_t model = At<int16_t>(weapon, 0x10E);
        if (model != -1 && model != noAimWeaponA_BF4430 && model != noAimWeaponB_BF433C && target &&
            IsPedEntity(target)) {
            EventMgr_Send(eventManager_C2C1A0, 0xC, ped, target, 0, -1.0f, -1.0f);
        }
    }
    re::Ptr32<uint8_t>* ref = &At<uint8_t*>(self, 0x40);
    if (target != *ref) {
        if (*ref) {
            Entity_CleanUpOldReference(*ref, ref);
        }
        *ref = target;
        if (target) {
            RegisterReference(target, ref);
            if (self[0x37]) {
                HeadLook_SetTarget(ped + 0xD70, *ref);
            }
        }
    }
}

// 0x60F1F0 (thiscall): TargetFaceTrack's turn: towards the target (flat), else along the
// ped's move direction when moving (and +0x32 clear); +0x20 / +0x1C the turn limit per
// second, +0x44 the turn speed
void __fastcall TargetFace_Turn(uint8_t* self, re::Edx, float dt) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x3C));
    V3 f;
    Placeable_GetForward(ped, &f);
    V3 fwd = f;
    uint8_t* move = At<uint8_t*>(ped, 0x2E0);
    V3 dir = {At<float>(move, 0xC), At<float>(move, 0x10), At<float>(move, 0x14)};
    uint8_t* target = At<uint8_t*>(self, 0x40);
    if (target) {
        V3 d;
        V3* r = VectorSub(&d, Pos(target), Pos(ped));
        dir.x = r->x;
        dir.y = r->y;
        dir.z = r->z;
        dir.z = 0.0f;
        Vector_Normalise(&dir);
    } else {
        float lengthSq = float(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        lengthSq = float(lengthSq - 0.1);
        if (!(lengthSq >= 0.0f) || self[0x32]) {
            return;
        }
    }
    float rate = At<float>(self, 0x20) / At<float>(self, 0x1C);
    AngleBetweenPoints(fwd.x, fwd.y, 0.0f, 0.0f);
    float h = TurnHeading(fwd.x, fwd.y, fwd.z, dir.x, dir.y, dir.z, dt, rate, targetFaceRate_B0BC3C,
                          &At<float>(self, 0x44));
    h = LimitAngle(h);
    if (At<uint8_t*>(ped, 0x14)) {
        Matrix_SetRotateZOnly(At<uint8_t*>(ped, 0x14), h);
    }
    At<float>(ped, 0x10) = h;
    Ped_SetHeading(ped, h);
}

// 0x60F3D0 (thiscall): TargetFaceTrack::End: the target released (+0x36), the turn limit
// reset, the move direction refreshed when slow (+0x35), the ped's +0x1C8 +0x84 restored
// (+0x48 / +0x34) and the head look cleared (+0x37)
uint8_t __fastcall TargetFace_End(uint8_t* self, re::Edx) {
    if (self[0x36]) {
        uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x3C));
        if (At<int32_t>(ped, 0x1310) == 0xD && At<int32_t>(ped, 0x142C) == 1) {
            Ped_EveryNthFrame(ped, 8);
        }
        if (At<uint8_t*>(self, 0x40)) {
            Entity_CleanUpOldReference(At<uint8_t*>(self, 0x40), &At<uint8_t*>(self, 0x40));
            At<uint8_t*>(self, 0x40) = nullptr;
        }
    }
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x3C));
    if (At<float>(self, 0x2C) != -1.0f) {
        At<float>(At<uint8_t*>(ped, 0x1C8), 0x40) = -1.0f;
    }
    if (At<float>(At<uint8_t*>(ped, 0x2E0), 0x1C) < 0.2 && self[0x35]) {
        Ped_UpdateMoveDirection(ped);
    }
    if (self[0x48]) {
        At<uint8_t>(At<uint8_t*>(OwnerPed(At<uint8_t*>(self, 0x3C)), 0x1C8), 0x84) = self[0x34];
    }
    if (self[0x37]) {
        HeadLook_SetTarget(ped + 0xD70, nullptr);
    }
    return 1;
}

} // namespace

#define ACTTRACKSLOCO_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

ACTTRACKSLOCO_REVERSED("LocomotionJumpTrack::End", 0x0060E240, &LocomotionJump_End);
ACTTRACKSLOCO_REVERSED("LocomotionLiquidTrack::Update", 0x0060E270, &LocomotionLiquid_Update);
ACTTRACKSLOCO_REVERSED("LocomotionRetardTrack::Update", 0x0060E360, &LocomotionRetard_Update);
ACTTRACKSLOCO_REVERSED("LocomotionAnimationNewTrack::Start", 0x0060E3A0, &LocomotionAnimationNew_Start);
ACTTRACKSLOCO_REVERSED("AnimationLocoSyncTrack::Start", 0x0060E4B0, &AnimationLocoSync_Start);
ACTTRACKSLOCO_REVERSED("AnimationLocoSyncTrack::End", 0x0060E620, &AnimationLocoSync_End);
ACTTRACKSLOCO_REVERSED("ActionTracks::SpawnFootEffect", 0x0060E680, &SpawnFootEffectAt);
ACTTRACKSLOCO_REVERSED("ActionTracks::FootstepEffects", 0x0060E730, &FootstepEffects);
ACTTRACKSLOCO_REVERSED("LocomotionEventTrack::Update", 0x0060ED90, &LocomotionEvent_Update);
ACTTRACKSLOCO_REVERSED("GravityTrack::Start", 0x0060EF50, &Gravity_Start);
ACTTRACKSLOCO_REVERSED("AllowSwitchGoofyFootTrack::Start", 0x0060EF80, &AllowSwitchGoofyFoot_Start);
ACTTRACKSLOCO_REVERSED("OrientOffSetTrack::Start", 0x0060F020, &OrientOffSet_Start);
ACTTRACKSLOCO_REVERSED("TargetFaceTrack::Start", 0x0060F0C0, &TargetFace_Start);
ACTTRACKSLOCO_REVERSED("TargetFaceTrack::SetTarget", 0x0060F110, &TargetFace_SetTarget);
ACTTRACKSLOCO_REVERSED("TargetFaceTrack::Turn", 0x0060F1F0, &TargetFace_Turn);
ACTTRACKSLOCO_REVERSED("TargetFaceTrack::End", 0x0060F3D0, &TargetFace_End);
