// AM_ models: their constructor, moving the whole skeleton (translating every joint, or
// re-rooting it on one joint's new transform), and RV::BaseList's deleting destructor.
// 0x6B9DA0..0x6BA374.

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

GAME_VAR(float, mt_timeScale, 0x00BB612C);
GAME_VAR(float, mt_timeStep, 0x00C1A9A4);
GAME_VAR(uint32_t, mt_timeInMs, 0x00C1A9B4);

GAME_FN(void, __cdecl, Skeleton_Propagate, 0x006B6310, uint8_t* skeleton);
GAME_FN(void, __cdecl, Transform_Invert, 0x00522440, Xform* out, const Xform* in);
GAME_FN(Xform*, __thiscall, NiTransform_Multiply, 0x00406830, const Xform* self, Xform* out, const Xform* other);
GAME_FN(void, __thiscall, NiMatrix3_Fn768CD0, 0x00768CD0, Xform* self);
GAME_FN(float*, __thiscall, NiMatrix3_MultiplyVector, 0x00768A70, const Xform* self, CVector* out, const CVector* v);
GAME_FN(void, __thiscall, Model_UpdateAttachments, 0x006B93A0, uint8_t* model, float dt, int withSkeleton);
GAME_FN(void, __thiscall, Model_CentreSphereOnRoot, 0x006B4530, uint8_t* model);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);

inline void UpdateIfChanged(uint8_t* self, float dt) {
    if (At<uint32_t>(self, 0x8C) != 0 || At<uint32_t>(self, 0x90) != 0) {
        Model_UpdateAttachments(self, dt, 1);
        Model_CentreSphereOnRoot(self);
    }
}

// 0x6B9DA0: joint `joint` takes transform `t` and the other joints move rigidly with it
// (update: then the attachments; dt < 0: a frame's time)
void __fastcall Model_ReRoot(uint8_t* self, re::Edx, uint32_t joint, const Xform* t, int update, float dt) {
    if (At<uint8_t>(self, 0x98) & 0x80) {
        Skeleton_Propagate(At<uint8_t*>(self, 0x14));
        At<uint32_t>(self, 0x98) &= ~0x80u;
    }
    uint8_t* skeleton = At<uint8_t*>(self, 0x14);
    Xform* transforms = At<Xform*>(skeleton, 8);
    Xform* j = &transforms[joint];
    Xform inverse;  // F-0x68
    inverse.scale = 1.0f;
    Xform turn;     // F-0x144
    turn.scale = 1.0f;
    CVector oldPos = j->translation;
    j->translation = CVector{0.0f, 0.0f, 0.0f};
    Transform_Invert(&inverse, j);
    Xform product;
    turn = *NiTransform_Multiply(t, &product, &inverse);
    Xform orthonormal = turn;  // F-0x9C
    NiMatrix3_Fn768CD0(&orthonormal);
    turn = orthonormal;
    turn.translation = CVector{0.0f, 0.0f, 0.0f};
    CVector newPos = t->translation;
    uint32_t count = At<uint32_t>(skeleton, 4);
    *j = *t;
    for (uint32_t i = 0; i < count; i++) {
        if (i == joint) {
            continue;
        }
        Xform* other = &transforms[i];
        CVector d;
        d.x = other->translation.x - oldPos.x;
        d.y = other->translation.y - oldPos.y;
        d.z = other->translation.z - oldPos.z;
        other->translation = CVector{0.0f, 0.0f, 0.0f};
        CVector turned;
        const float* r = NiMatrix3_MultiplyVector(&turn, &turned, &d);
        CVector rotated{r[0], r[1], r[2]};
        Xform moved = *NiTransform_Multiply(&turn, &product, other);  // F-0x110
        CVector pos{rotated.x + newPos.x, newPos.y + rotated.y, newPos.z + rotated.z};
        moved.translation = pos;
        Xform fixed = moved;  // F-0xD0
        NiMatrix3_Fn768CD0(&fixed);
        *other = fixed;
    }
    At<int32_t>(self, 0x90) = -1;
    At<int32_t>(self, 0x8C) = -1;
    if (!uint8_t(update)) {
        return;
    }
    if (dt < 0.0f) {
        dt = mt_timeScale * mt_timeStep;
    }
    UpdateIfChanged(self, dt);
}

// 0x6BA090: every joint moved by (x, y, z) (update: then the attachments; dt < 0: the
// clock's seconds)
void __fastcall Model_Translate(uint8_t* self, re::Edx, float x, float y, float z, int update, float dt) {
    uint8_t* skeleton = At<uint8_t*>(self, 0x14);
    uint32_t count = At<uint32_t>(skeleton, 4);
    Xform* transforms = At<Xform*>(skeleton, 8);
    for (uint32_t i = 0; i < count; i++) {
        transforms[i].translation.x = transforms[i].translation.x + x;
        transforms[i].translation.y = transforms[i].translation.y + y;
        transforms[i].translation.z = transforms[i].translation.z + z;
    }
    At<int32_t>(self, 0x90) = -1;
    At<int32_t>(self, 0x8C) = -1;
    if (!uint8_t(update)) {
        return;
    }
    if (dt < 0.0f) {
        dt = float(double(mt_timeInMs) * 0.0010000000474974513);
    }
    UpdateIfChanged(self, dt);
}

// 0x6BA220: RV::BaseList's deleting destructor
void* __fastcall BaseList_DeletingDestruct(uint8_t* self, re::Edx, int flags) {
    At<void*>(self, 0) = reinterpret_cast<void*>(0x0092D0AC);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x6BA240: AM_Model's constructor (all cleared; its attachment and IK effector lists)
uint8_t* __fastcall Model_Construct(uint8_t* self, re::Edx) {
    for (int off = 0; off <= 0x14; off += 4) {
        At<uint32_t>(self, off) = 0;
    }
    for (int off = 0x34; off <= 0xA8; off += 4) {
        At<uint32_t>(self, off) = 0;
    }
    for (int off = 0xB0; off <= 0xD4; off += 4) {
        At<uint32_t>(self, off) = 0;
    }
    At<void*>(self, 0xAC) = reinterpret_cast<void*>(0x00941FDC);  // RV::List<AM_Attachment*>
    for (int off = 0xDC; off <= 0x100; off += 4) {
        At<uint32_t>(self, off) = 0;
    }
    At<void*>(self, 0xD8) = reinterpret_cast<void*>(0x00941FE4);  // RV::List<AM_IKEffector*>
    for (int off = 0x18; off <= 0x30; off += 4) {
        At<uint32_t>(self, off) = 0;
    }
    return self;
}

} // namespace

#define AMMODELT_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMMODELT_REVERSED("AM_Model::ReRoot", 0x006B9DA0, &Model_ReRoot);
AMMODELT_REVERSED("AM_Model::Translate", 0x006BA090, &Model_Translate);
AMMODELT_REVERSED("RV::BaseList::DeletingDestructor", 0x006BA220, &BaseList_DeletingDestruct);
AMMODELT_REVERSED("AM_Model::AM_Model", 0x006BA240, &Model_Construct);
