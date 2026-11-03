// AM_AnimTrack: applying one player's anim to a model (the pose sampled into the joints'
// records, the root motion taken out and added to the model's travel, weighted by the
// player's share of the blend). 0x6B6FB0..0x6B723B.

#include <cmath>
#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

struct CVector {
    float x, y, z;
};

inline void* const kAnimPose = reinterpret_cast<void*>(0x020C4910);
GAME_ARRAY(uint32_t, apply_zeroVector, 0x00BCC564);
GAME_VAR(int32_t, apply_noGroupScale, 0x020C48C0);

GAME_FN(void, __thiscall, Pose_SampleAll, 0x006BD970, void* sampler, uint8_t* starts, uint8_t* anim, float time,
        uint8_t* hierarchy);
GAME_FN(void, __stdcall, MakeRootRelative, 0x006B6910, uint8_t* hxd, uint8_t* anim, uint8_t* starts);
GAME_FN(void, __stdcall, RootRotation, 0x006B6A60, uint8_t* model, uint8_t* player, uint8_t* starts, float* quatOut,
        CVector* posOut);
GAME_FN(CVector*, __thiscall, Track_RootMotionFromStored, 0x006B4910, void* self, CVector* out, uint8_t* model,
        uint8_t* player, uint8_t* starts);
GAME_FN(void, __stdcall, MoveStartByFirstKey, 0x006B67F0, uint8_t* hxd, uint8_t* anim, uint8_t* starts);
GAME_FN(void, __thiscall, Vector_Scale, 0x0088A480, CVector* v, float s);
GAME_FN(void, __thiscall, Vector_Add, 0x00415200, CVector* v, const CVector* add);
GAME_FN(uint8_t*, __thiscall, Hxd_GetGroup, 0x00450B80, uint8_t* hxd, uint32_t index);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const CVector* v);

inline void ZeroObjectMotion(uint8_t* model) {
    uint8_t* object = At<uint8_t*>(model, 0xC);
    At<uint32_t>(object, 0x6C) = apply_zeroVector[0];
    At<uint32_t>(object, 0x70) = apply_zeroVector[1];
    At<uint32_t>(object, 0x74) = apply_zeroVector[2];
    object = At<uint8_t*>(model, 0xC);
    At<uint32_t>(object, 0x38) = apply_zeroVector[0];
    At<uint32_t>(object, 0x3C) = apply_zeroVector[1];
    At<uint32_t>(object, 0x40) = apply_zeroVector[2];
}

// 0x6B6FB0: applies player `player`'s anim to `model` (false without an HXD, an anim or
// weight; the player's flag 4 marks it applied)
uint8_t __fastcall ApplyPlayer(uint8_t* self, re::Edx, uint8_t* model, uint8_t* starts, CVector* travel,
                               float* rootRotation, int player) {
    uint8_t* hierarchy = At<uint8_t*>(model, 0x94);
    uint8_t* hxd = At<uint8_t*>(hierarchy, 0x10);
    if (hxd == nullptr) {
        return 0;
    }
    uint8_t* p = self + 0x18 + player * 0x14;
    uint8_t* anim = At<uint8_t*>(p, 0);
    if (anim == nullptr) {
        return 0;
    }
    float share = player == 0 ? 1.0f - At<float>(self, 0x40) : At<float>(self, 0x40);
    if (share < 0.001f) {
        At<uint32_t>(p, 0x10) &= ~4u;
        return 0;
    }
    if (!(0.0f < share)) {
        return 1;
    }
    uint32_t flags = At<uint32_t>(anim, 0x18);
    uint8_t extraRoot = uint8_t((flags >> 6) & 1);
    uint8_t moves = uint8_t((flags >> 2) & 1);
    uint8_t rotates = (int8_t(flags) < 0 && extraRoot) ? 1 : 0;
    uint8_t objectOnly = uint8_t((At<uint32_t>(model, 0x98) >> 2) & 1);
    Pose_SampleAll(kAnimPose, starts, anim, At<float>(p, 4), hierarchy);
    if (extraRoot) {
        if (objectOnly) {
            ZeroObjectMotion(model);
            At<uint32_t>(p, 0x10) |= 4;
            return 1;
        }
        MakeRootRelative(hxd, At<uint8_t*>(p, 0), starts);
    } else if (objectOnly) {
        ZeroObjectMotion(model);
        At<uint32_t>(p, 0x10) |= 4;
        return 1;
    }
    CVector motion{0.0f, 0.0f, 0.0f};
    if (rotates) {
        RootRotation(model, p, starts, rootRotation, &motion);
    } else if (moves) {
        CVector tmp;
        motion = *Track_RootMotionFromStored(self, &tmp, model, p, starts);
        if (!extraRoot) {
            MoveStartByFirstKey(hxd, At<uint8_t*>(p, 0), starts);
        }
    }
    if (share < 1.0f) {
        Vector_Scale(&motion, share);
    }
    Vector_Add(travel, &motion);
    if (apply_noGroupScale == 0) {
        uint8_t* h = At<uint8_t*>(At<uint8_t*>(model, 0x94), 0x10);
        if (h && At<void*>(h, 0x70)) {
            uint8_t* a = At<uint8_t*>(p, 0);
            float groupScale = At<float>(Hxd_GetGroup(h, At<uint32_t>(a, 0x20)), 0x28);
            if (!(std::fabs(groupScale) < 0.001f)) {
                // the root record's +0x20 scaled by the hierarchy's size over the group's
                int kind = (At<uint32_t>(a, 0x18) & 0x200) ? 2 : 1;
                uint8_t* record = starts + At<int32_t>(starts, 0x24) * kind + 0x4C;
                const CVector* size =
                    reinterpret_cast<const CVector*>(At<uint8_t*>(At<uint8_t*>(model, 0x94), 0xA8) + kind * 0xC);
                At<float>(record, 0x20) = Vector_Magnitude(size) / groupScale * At<float>(record, 0x20);
            }
        }
    }
    At<uint32_t>(p, 0x10) |= 4;
    return 1;
}

} // namespace

#define AMAPPLY_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMAPPLY_REVERSED("AM_AnimTrack::ApplyPlayer", 0x006B6FB0, &ApplyPlayer);
