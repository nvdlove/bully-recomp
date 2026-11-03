// AM_ models: applying both of a track's players (the second blended in from a scratch
// pose), a socket follower's position and rotation (+4 position, +0x10 rotation, +0x20 the
// socket, +0x24 its model, +0x30 a second socket of the other model), and setting a joint's
// transform (and its subtree's) directly. 0x6B99F0..0x6B9A75, 0x6B9AA0..0x6B9D9C.

#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

struct CVector {
    float x, y, z;
};
struct Xform {
    float rotation[9];
    CVector translation;
    float scale;
};

inline void* const kAnimPose = reinterpret_cast<void*>(0x020C4910);
GAME_VAR(uint8_t*, mj_scratchPose, 0x020C48A0);
GAME_VAR(uint32_t, mj_timeInMs, 0x00C1A9B4);

GAME_FN(uint8_t, __thiscall, Track_ApplyPlayer, 0x006B6FB0, uint8_t* track, uint8_t* model, uint8_t* starts,
        void* travel, float* rootRotation, int player);
GAME_FN(void, __thiscall, Pose_Blend, 0x006BCDC0, void* sampler, uint8_t* out, uint8_t* a, uint8_t* b, float t,
        uintptr_t);
GAME_FN(void, __thiscall, Model_UpdateChild, 0x006B9550, uint8_t* model, uint8_t* child, float dt, int);
GAME_FN(void, __thiscall, Pose_UpdateModel, 0x006C1910, void* sampler, uint8_t* model, float dt, int, uintptr_t);
GAME_FN(void, __thiscall, Pose_GetBoneTransform, 0x006BF570, void* sampler, uint8_t* model, int32_t bone, Xform* out,
        void* socket);
GAME_FN(float*, __thiscall, Quat_FromWFirst, 0x00435B70, float* out, const float* in);
GAME_FN(void, __thiscall, Quat_ToNiTransform, 0x00413E60, const float* q, Xform* out);
GAME_FN(void, __cdecl, Transform_Invert, 0x00522440, Xform* out, const Xform* in);
GAME_FN(Xform*, __thiscall, NiTransform_Multiply, 0x00406830, const Xform* self, Xform* out, const Xform* other);
GAME_FN(void, __thiscall, NiQuaternion_FromRotation, 0x00776040, void* q, const Xform* rotation);
GAME_FN(void*, __cdecl, Transform_Combine, 0x00415E30, void* out, const void* in, int mode);
GAME_FN(void, __thiscall, Model_UpdateAttachments, 0x006B93A0, uint8_t* model, float dt, int withSkeleton);
GAME_FN(void, __thiscall, Model_CentreSphereOnRoot, 0x006B4530, uint8_t* model);

inline void SetDirty(uint8_t* model, uint32_t joint) {
    uint32_t* bits = &At<uint32_t>(model, 0x8C);
    if (int32_t(joint) >= 32) {
        bits += joint >> 5;
        joint -= (joint >> 5) << 5;
    }
    *bits |= 1u << joint;
}

// 0x6B99F0: AM_AnimTrack: both players applied (the second into the scratch pose, then
// blended over the first by its share); false without an HXD
uint8_t __fastcall Track_ApplyBoth(uint8_t* self, re::Edx, uint8_t* model, uint8_t* starts, void* travel,
                                   float* rootRotation) {
    if (At<void*>(At<uint8_t*>(model, 0x94), 0x10) == nullptr) {
        return 0;
    }
    uint8_t* target = starts;
    for (uint32_t i = 0; i < 2; i++) {
        float share = i == 0 ? 1.0f - At<float>(self, 0x40) : At<float>(self, 0x40);
        if (Track_ApplyPlayer(self, model, target, travel, rootRotation, int(i))) {
            if (starts == target) {
                target = mj_scratchPose;
            } else {
                Pose_Blend(kAnimPose, starts, starts, target, share, 0);
            }
        }
    }
    return 1;
}

// 0x6B9AA0: the follower's position from its socket (updating the socket's model first
// when it isn't `model`), and with a second socket its rotation relative to that one
void __fastcall Follower_Update(uint8_t* self, re::Edx, uint8_t* model, float dt) {
    Xform t;  // F-0xE0
    t.scale = 1.0f;
    void* socket = At<void*>(self, 0x20);
    if (socket) {
        uint8_t* owner = At<uint8_t*>(self, 0x24);
        if (owner) {
            if (owner != model) {
                uint8_t* parent = At<uint8_t*>(owner, 0x9C);
                if (parent == model) {
                    Model_UpdateChild(model, owner, dt, 1);
                } else {
                    Pose_UpdateModel(kAnimPose, parent ? parent : owner, dt, 1, 0);
                }
            }
            Pose_GetBoneTransform(kAnimPose, At<uint8_t*>(self, 0x24), -1, &t, At<void*>(self, 0x20));
            At<CVector>(self, 4) = t.translation;
        } else {
            using GetFloats = const float*(__thiscall*)(void*);
            const float* pos = re::PtrCast<GetFloats>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(socket))[0])(socket);
            At<float>(self, 4) = pos[0];
            At<float>(self, 8) = pos[1];
            At<float>(self, 0xC) = pos[2];
        }
    }
    if (At<void*>(self, 0x30) == nullptr || At<void*>(self, 0x20) == nullptr) {
        return;
    }
    Xform inverse;  // F-0x68
    inverse.scale = 1.0f;
    if (At<void*>(self, 0x24) == nullptr) {
        void* s = At<void*>(self, 0x20);
        using GetFloats = const float*(__thiscall*)(void*);
        const float* rotation = re::PtrCast<GetFloats>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(s))[1])(s);
        float q[4];  // F-0x78
        Quat_ToNiTransform(Quat_FromWFirst(q, rotation), &t);
        t.translation = At<CVector>(self, 4);
    }
    Xform other;  // F-0xAC
    other.scale = 1.0f;
    Pose_GetBoneTransform(kAnimPose, model, -1, &other, At<void*>(self, 0x30));
    Transform_Invert(&inverse, &other);
    Xform product;
    other = *NiTransform_Multiply(&t, &product, &inverse);
    NiQuaternion_FromRotation(self + 0x10, &other);
}

// 0x6B9C20: sets joint `joint`'s transform (combined by `mode`; with `subtree` its
// descendants' too), marking them changed; `update` then places the attachments (dt < 0:
// the clock's seconds)
void __fastcall Model_SetJointTransform(uint8_t* self, re::Edx, uint32_t joint, const void* transform, int subtree,
                                        int update, float dt, int mode) {
    uint8_t* skeleton = At<uint8_t*>(self, 0x14);
    uint8_t* j = At<uint8_t*>(skeleton, 8) + joint * 0x34;
    Transform_Combine(j, transform, mode);
    SetDirty(self, joint);
    if (uint8_t(subtree)) {
        int depth = 1;
        bool first = true;
        const uint32_t* flags = reinterpret_cast<const uint32_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x14), 0xC) +
                                                                  joint * 0x14 + 8);
        for (;;) {
            uint32_t kind = *flags & 3;
            if (kind == 1) {
                if (--depth == 0) {
                    break;
                }
            } else if (kind == 2 && !first) {
                depth++;
            }
            joint++;
            j += 0x34;
            flags += 5;
            first = false;
            SetDirty(self, joint);
            Transform_Combine(j, transform, mode);
        }
    }
    if (!uint8_t(update)) {
        return;
    }
    if (dt < 0.0f) {
        dt = float(double(mj_timeInMs) * 0.0010000000474974513);
    }
    if (At<uint32_t>(self, 0x8C) != 0 || At<uint32_t>(self, 0x90) != 0) {
        Model_UpdateAttachments(self, dt, 1);
        Model_CentreSphereOnRoot(self);
    }
}

} // namespace

#define AMMODELJ_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMMODELJ_REVERSED("AM_AnimTrack::ApplyBoth", 0x006B99F0, &Track_ApplyBoth);
AMMODELJ_REVERSED("AM_Follower::Update", 0x006B9AA0, &Follower_Update);
AMMODELJ_REVERSED("AM_Model::SetJointTransform", 0x006B9C20, &Model_SetJointTransform);
