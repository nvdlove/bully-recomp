// AM_AnimTrack (vtable 0x941F90): its constructor, type id, copying, the root motion a
// player's anim moved (the root joint's translation now against where it started, over a
// loop's wrap too) and setting a player's anim (keeping its group's use count).
// 0x6B45F0..0x6B4ACD, 0x6B5600.

#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

struct CVector {
    float x, y, z;
};

// a player (0x14): +0 the anim, +4 the time, +8 last frame's, +0xC the rate, +0x10 flags
// an anim: +8 its HXD file, +0x18 flags, +0x20 its group, +0x32 its index

GAME_VAR(int16_t, am_nextTypeId, 0x00BF3830);
GAME_VAR(uint32_t, am_trackTypeIdSet, 0x020C48BC);
GAME_VAR(int16_t, am_trackTypeId, 0x020C48B8);
GAME_ARRAY(uint32_t, am_zeroVector, 0x00BCC564);
inline void* const kAnimPose = reinterpret_cast<void*>(0x020C4910);

GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Hxd_GetTranslation, 0x006B3E20, uint8_t* hxd, int animIndex, int joint, CVector* out,
        int key);
GAME_FN(int, __thiscall, Hxd_ReleaseGroup, 0x006B3DA0, uint8_t* hxd, uint32_t group, int keep);
// (the pose sampler at 0x20C4910: an anim's joint at a time)
GAME_FN(void, __thiscall, Pose_SampleJoint, 0x006BE3B0, void* sampler, uint8_t* anim, float time, int joint,
        uint8_t* hierarchy, CVector* translation, float* rotation);
GAME_FN(CVector*, __thiscall, Track_RootMotion, 0x006B4730, void* self, CVector* out, uint8_t* model,
        uint8_t* player, CVector start);
GAME_FN(CVector*, __thiscall, Track_RootMotionNow, 0x006B4980, void* self, CVector* out, uint8_t* model,
        uint8_t* player);

inline int AnimKind(uint8_t* anim) {
    return (At<uint32_t>(anim, 0x18) & 0x200) ? 2 : 1;
}

// the root joint an anim moves: the skeleton's last with flag 0x40, else 1 or 2
inline int RootJoint(uint8_t* model, uint8_t* anim) {
    if (At<uint32_t>(anim, 0x18) & 0x40) {
        return int(At<uint32_t>(At<uint8_t*>(At<uint8_t*>(model, 0x94), 0x10), 0x5C)) - 1;
    }
    return AnimKind(anim);
}

inline void AddGroupUser(uint8_t* anim) {
    if (anim == nullptr) {
        return;
    }
    uint8_t* hxd = At<uint8_t*>(anim, 8);
    if (hxd && At<uint32_t>(anim, 0x20) < At<uint32_t>(hxd, 0x68)) {
        At<uint16_t>(At<uint8_t*>(hxd, 0x6C) + At<uint32_t>(anim, 0x20) * 0x3C, 0x2C) += 1;
    }
}

// 0x6B45F0: AM_AnimTrack's constructor (both players' rates 1)
void* __fastcall Construct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 4) = 0;
    At<void*>(self, 0) = reinterpret_cast<void*>(0x00941F90);
    At<float>(self, 0x1C) = 0.0f;
    At<float>(self, 0x20) = 0.0f;
    At<uint32_t>(self, 0x18) = 0;
    At<uint32_t>(self, 0x28) = 0;
    At<float>(self, 0x24) = 1.0f;
    At<uint32_t>(self, 0x2C) = 0;
    At<uint32_t>(self, 0x3C) = 0;
    At<float>(self, 0x30) = 0.0f;
    At<float>(self, 0x34) = 0.0f;
    At<float>(self, 0x38) = 1.0f;
    return self;
}

// 0x6B4630: IsType (the class's id handed out on first use)
uint8_t __fastcall IsType(void*, re::Edx, int16_t id) {
    if (!(am_trackTypeIdSet & 1)) {
        am_trackTypeIdSet |= 1;
        am_trackTypeId = am_nextTypeId;
        am_nextTypeId = int16_t(am_nextTypeId + 1);
    }
    return id == am_trackTypeId ? 1 : 0;
}

// 0x6B4670: copies another track (its anims gain a user each; the old ones aren't released)
uint8_t* __fastcall CopyFrom(uint8_t* self, re::Edx, uint8_t* other) {
    At<uint32_t>(self, 8) = At<uint32_t>(other, 8);
    At<float>(self, 0xC) = At<float>(other, 0xC);
    At<float>(self, 0x10) = At<float>(other, 0x10);
    At<float>(self, 0x14) = At<float>(other, 0x14);
    for (int p = 0; p < 2; p++) {
        int base = 0x18 + p * 0x14;
        uint8_t* anim = At<uint8_t*>(other, base);
        At<uint8_t*>(self, base) = anim;
        AddGroupUser(anim);
        At<float>(self, base + 4) = At<float>(other, base + 4);
        At<float>(self, base + 8) = At<float>(other, base + 8);
        At<float>(self, base + 0xC) = At<float>(other, base + 0xC);
        At<uint32_t>(self, base + 0x10) = At<uint32_t>(other, base + 0x10);
    }
    At<float>(self, 0x40) = At<float>(other, 0x40);
    At<uint32_t>(self, 0x44) = At<uint32_t>(other, 0x44);
    return self;
}

// 0x6B4730: how far the root moved from `start` (the root's translation at last frame's
// time; over a loop's wrap, the anim's whole travel added); no height for the kind's own
// root
CVector* __fastcall RootMotion(void*, re::Edx, CVector* out, uint8_t* model, uint8_t* player, CVector start) {
    uint8_t* hierarchy = At<uint8_t*>(model, 0x94);
    uint8_t* hxd = At<uint8_t*>(hierarchy, 0x10);
    uint8_t* anim = At<uint8_t*>(player, 0);
    int joint = RootJoint(model, anim);
    out->x = 0.0f;
    out->y = 0.0f;
    out->z = 0.0f;
    CVector now{0.0f, 0.0f, 0.0f};
    float rotation[4];
    Pose_SampleJoint(kAnimPose, anim, At<float>(player, 8), joint, hierarchy, &now, rotation);
    CVector from = start;
    if (At<float>(player, 4) < At<float>(player, 8) && (At<uint8_t>(player, 0x10) & 2)) {
        uint8_t* a = At<uint8_t*>(player, 0);
        CVector end{0.0f, 0.0f, 0.0f};
        Hxd_GetTranslation(hxd, At<uint16_t>(a, 0x32), joint, &end, -1);
        if (joint != AnimKind(a)) {
            CVector first{0.0f, 0.0f, 0.0f};
            Hxd_GetTranslation(hxd, At<uint16_t>(a, 0x32), joint, &first, 0);
            CVector travel;
            VectorSub(&travel, &end, &first);
            end = travel;
        }
        CVector moved, total;
        VectorAdd(&total, VectorSub(&moved, &end, &now), &from);
        *out = total;
    } else {
        CVector moved;
        *out = *VectorSub(&moved, &from, &now);
    }
    if (joint == AnimKind(At<uint8_t*>(player, 0))) {
        out->z = 0.0f;
    }
    return out;
}

// 0x6B4910: RootMotion from the model's stored start for that joint (+0x64 + joint * +0x24)
CVector* __fastcall RootMotionFromStored(void* self, re::Edx, CVector* out, uint8_t* model, uint8_t* player,
                                         uint8_t* starts) {
    int joint = RootJoint(model, At<uint8_t*>(player, 0));
    const CVector* start = reinterpret_cast<const CVector*>(starts + At<int32_t>(starts, 0x24) * joint + 0x64);
    Track_RootMotion(self, out, model, player, *start);
    return out;
}

// 0x6B4980: RootMotion from where the root is now (sampled at the player's time)
CVector* __fastcall RootMotionNow(void* self, re::Edx, CVector* out, uint8_t* model, uint8_t* player) {
    uint8_t* anim = At<uint8_t*>(player, 0);
    int joint = RootJoint(model, anim);
    CVector start{0.0f, 0.0f, 0.0f};
    float rotation[4];
    Pose_SampleJoint(kAnimPose, anim, At<float>(player, 4), joint, At<uint8_t*>(model, 0x94), &start, rotation);
    Track_RootMotion(self, out, model, player, start);
    return out;
}

// 0x6B4A20: AM_AnimTrack's vtable slot 5 (shared with AM_MotionTrack): the root motion
// of the first player (flag 4 of the model's +0x98: the model's +0xC offsets cleared
// instead); false without an anim
uint8_t __fastcall GetRootMotion(uint8_t* self, re::Edx, uint8_t* model, CVector* out, int) {
    uint8_t* anim = At<uint8_t*>(self, 0x18);
    if (anim == nullptr) {
        return 0;
    }
    bool moves = ((At<uint32_t>(anim, 0x18) >> 2) & 1) != 0;
    if ((At<uint32_t>(model, 0x98) >> 2) & 1) {
        uint8_t* object = At<uint8_t*>(model, 0xC);
        At<uint32_t>(object, 0x6C) = am_zeroVector[0];
        At<uint32_t>(object, 0x70) = am_zeroVector[1];
        At<uint32_t>(object, 0x74) = am_zeroVector[2];
        object = At<uint8_t*>(model, 0xC);
        At<uint32_t>(object, 0x38) = am_zeroVector[0];
        At<uint32_t>(object, 0x3C) = am_zeroVector[1];
        At<uint32_t>(object, 0x40) = am_zeroVector[2];
        return 1;
    }
    if (moves) {
        CVector motion;
        CVector* m = Track_RootMotionNow(self, &motion, model, self + 0x18);
        *out = *m;
    }
    return 1;
}

// 0x6B5600: a player's anim set (the new one's group gains a user, the old one's loses one)
void __fastcall Player_SetAnim(uint8_t* self, re::Edx, uint8_t* anim) {
    AddGroupUser(anim);
    uint8_t* old = At<uint8_t*>(self, 0);
    if (old && At<uint8_t*>(old, 8)) {
        Hxd_ReleaseGroup(At<uint8_t*>(old, 8), At<uint32_t>(old, 0x20), 0);
    }
    At<uint8_t*>(self, 0) = anim;
}

} // namespace

#define AMROOT_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMROOT_REVERSED("AM_AnimTrack::AM_AnimTrack", 0x006B45F0, &Construct);
AMROOT_REVERSED("AM_AnimTrack::IsType", 0x006B4630, &IsType);
AMROOT_REVERSED("AM_AnimTrack::CopyFrom", 0x006B4670, &CopyFrom);
AMROOT_REVERSED("AM_AnimTrack::RootMotion", 0x006B4730, &RootMotion);
AMROOT_REVERSED("AM_AnimTrack::RootMotionFromStored", 0x006B4910, &RootMotionFromStored);
AMROOT_REVERSED("AM_AnimTrack::RootMotionNow", 0x006B4980, &RootMotionNow);
AMROOT_REVERSED("AM_AnimTrack::GetRootMotion", 0x006B4A20, &GetRootMotion);
AMROOT_REVERSED("AM_AnimPlayer::SetAnim", 0x006B5600, &Player_SetAnim);
