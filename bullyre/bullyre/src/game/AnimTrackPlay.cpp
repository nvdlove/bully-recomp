// AM_AnimTrack: starting an anim on one of its players (fading it in, its rate and
// flags), and moving a model's stored root start (per-joint records: +0x24 the stride,
// +0x54 a rotation, +0x64 a position) by an anim's root travel. 0x6B66D0..0x6B6A4F.

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
static_assert(sizeof(Xform) == 0x34);

GAME_FN(int, __thiscall, Hxd_ReleaseGroup, 0x006B3DA0, uint8_t* hxd, uint32_t group, int keep);
GAME_FN(float, __thiscall, Hxd_GetTranslation, 0x006B3E20, uint8_t* hxd, int animIndex, int joint, CVector* out,
        int key);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(void, __thiscall, Quat_ToTransform, 0x00413C50, const void* quat, Xform* out);
GAME_FN(void, __cdecl, Transform_Invert, 0x00522440, Xform* out, const Xform* in);
GAME_FN(Xform*, __thiscall, NiTransform_Multiply, 0x00406830, const Xform* self, Xform* out, const Xform* other);
GAME_FN(void, __thiscall, NiMatrix3_Fn768CD0, 0x00768CD0, Xform* self);
GAME_FN(void, __thiscall, NiQuaternion_FromRotation, 0x00776040, void* q, const Xform* rotation);

inline int AnimKind(uint8_t* anim) {
    return (At<uint32_t>(anim, 0x18) & 0x200) ? 2 : 1;
}
inline uint8_t* Record(uint8_t* starts, int joint) {
    return starts + At<int32_t>(starts, 0x24) * joint;
}

// 0x6B66D0: plays anim `animIndex` of `hxd` on player `player` (flags: 1 no fade, 2 loop,
// 8 a player flag; a negative fade takes the anim's own, a negative rate plays it
// backwards from the end); player 1 also sets the blend
void __fastcall PlayAnim(uint8_t* self, re::Edx, uint8_t* hxd, uint32_t animIndex, int flagsArg, float blend,
                         int player, float rate, float fade) {
    uint8_t* anim = animIndex < At<uint32_t>(hxd, 4) ? At<uint8_t*>(hxd, 0) + (animIndex << 6) : nullptr;
    uint8_t flags = uint8_t(flagsArg);
    uint8_t* p = self + 0x18 + player * 0x14;
    if (flags & 1) {
        fade = 0.0f;
    }
    if (anim) {
        uint8_t* owner = At<uint8_t*>(anim, 8);
        if (owner && At<uint32_t>(anim, 0x20) < At<uint32_t>(owner, 0x68)) {
            At<uint16_t>(At<uint8_t*>(owner, 0x6C) + At<uint32_t>(anim, 0x20) * 0x3C, 0x2C) += 1;
        }
    }
    uint8_t* old = At<uint8_t*>(p, 0);
    if (old && At<uint8_t*>(old, 8)) {
        Hxd_ReleaseGroup(At<uint8_t*>(old, 8), At<uint32_t>(old, 0x20), 0);
    }
    At<uint8_t*>(p, 0) = anim;
    if (fade < 0.0f) {
        fade = At<float>(anim, 0x10);
    }
    if (0.0f == fade) {
        At<float>(self, 0x10) = 0.0f;
        At<float>(self, 0x14) = 0.0f;
        At<float>(self, 0xC) = 1.0f;
    } else {
        float fadeIn = -fade;
        At<float>(self, 0x10) = fadeIn;
        At<float>(self, 0x14) = fadeIn;
        At<float>(self, 0xC) = 0.0f;
    }
    At<float>(p, 4) = rate < 0.0f ? At<float>(At<uint8_t*>(p, 0), 0xC) : 0.0f;
    At<float>(p, 0xC) = rate;
    At<uint32_t>(p, 0x10) = 1;
    if (flags & 2) {
        At<uint32_t>(p, 0x10) = 3;
    }
    if (flags & 8) {
        At<uint32_t>(p, 0x10) |= 8;
    }
    if (player == 1) {
        At<float>(self, 0x40) = blend;
    }
}

// 0x6B67F0: the anim's root start moved by its root's first key (the extra root, when the
// anim has one: all three axes, else x and y)
void __stdcall MoveStartByFirstKey(uint8_t* hxd, uint8_t* anim, uint8_t* starts) {
    CVector first{0.0f, 0.0f, 0.0f};
    int kind = AnimKind(anim);
    CVector* start = reinterpret_cast<CVector*>(Record(starts, kind) + 0x64);
    CVector d;
    if (At<uint32_t>(anim, 0x18) & 0x40) {
        int joint = int(At<uint32_t>(hxd, 0x5C)) - 1;
        Hxd_GetTranslation(hxd, At<uint16_t>(anim, 0x32), joint, &first, 0);
        CVector* r = VectorSub(&d, &first, reinterpret_cast<CVector*>(Record(starts, joint) + 0x64));
        CVector delta = *r;
        start->x = start->x + delta.x;
        start->y = start->y + delta.y;
        start->z = start->z + delta.z;
        return;
    }
    Hxd_GetTranslation(hxd, At<uint16_t>(anim, 0x32), kind, &first, 0);
    CVector* r = VectorSub(&d, &first, start);
    CVector delta = *r;
    start->x = start->x + delta.x;
    start->y = start->y + delta.y;
}

// 0x6B6910: the kind's root record re-expressed relative to the extra root's (the last
// joint's) transform
void __stdcall MakeRootRelative(uint8_t* hxd, uint8_t* anim, uint8_t* starts) {
    uint8_t* last = Record(starts, int(At<uint32_t>(hxd, 0x5C)) - 1);
    Xform lastT;   // F-0x104
    Quat_ToTransform(last + 0x54, &lastT);
    lastT.translation = *reinterpret_cast<CVector*>(last + 0x64);
    lastT.scale = 1.0f;
    Xform inverse;  // F-0x9C
    inverse.scale = 1.0f;
    Transform_Invert(&inverse, &lastT);
    uint8_t* root = Record(starts, AnimKind(anim));
    Xform rootT;    // F-0xD0
    Quat_ToTransform(root + 0x54, &rootT);
    rootT.scale = 1.0f;
    CVector* rootPos = reinterpret_cast<CVector*>(root + 0x64);
    rootT.translation = *rootPos;
    Xform product;
    lastT = *NiTransform_Multiply(&inverse, &product, &rootT);
    Xform rotation = lastT;  // F-0x68
    NiMatrix3_Fn768CD0(&rotation);
    NiQuaternion_FromRotation(root + 0x54, &rotation);
    *rootPos = lastT.translation;
}

} // namespace

#define AMPLAY_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMPLAY_REVERSED("AM_AnimTrack::PlayAnim", 0x006B66D0, &PlayAnim);
AMPLAY_REVERSED("AM::MoveStartByFirstKey", 0x006B67F0, &MoveStartByFirstKey);
AMPLAY_REVERSED("AM::MakeRootRelative", 0x006B6910, &MakeRootRelative);
