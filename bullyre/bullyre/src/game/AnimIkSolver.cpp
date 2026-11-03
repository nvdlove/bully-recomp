// AM_ models: the two-bone IK solver for one effector (AM_IKEffector: +0 the chain, +4 the
// target, +0x10 the wanted end rotation (w, x, y, z), +0x20.. its socket follower, +0x28 the
// fade time, +0x2C the fade length, +0x38 flags; the chain: +0x20 the upper joint, +0x24 the
// middle one (the end joint follows it), +0x28 / +0x2C the reach's least / most scale).
// Optionally bends the upper joint's parent first when the target's out of reach, then turns
// the upper joint to face the goal, bends the middle joint to fit the bone lengths, and places
// the end joint. 0x6BA380..0x6BB56E.

#include <cmath>
#include <cstdint>
#include <cstring>

#include "game/TrackMath.h"
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
struct Quat {
    float x, y, z, w;
};
static_assert(sizeof(Quat) == 0x10);

// a subtree visitor (only its vtable's first entry is used)
struct Visitor {
    re::Ptr32<void> vtable;
    re::Ptr32<uint8_t> model;
    re::Ptr32<void> a;
    re::Ptr32<void> b;
};

constexpr float kPi = 3.14159274f;

inline void* const kAdjustJointVisitorVtbl = reinterpret_cast<void*>(0x00941F88);  // +8 offset, +0xC transforms
inline void* const kIKAdjustMidBonesVtbl = reinterpret_cast<void*>(0x00941F54);    // +8 parent, +0xC one to leave

GAME_VAR(uint32_t, ik_alwaysBendParent, 0x020C48D0);
GAME_VAR(uint32_t, ik_allowBendParent, 0x00BB62F0);
GAME_VAR(uint32_t, ik_parentOnly, 0x020C48C4);
GAME_VAR(uint32_t, ik_parentLimitGuard, 0x020C48CC);
GAME_VAR(float, ik_parentLimit, 0x020C48C8);
GAME_ARRAY(uint32_t, ik_zeroVector, 0x00BCC564);

GAME_FN(void, __thiscall, Follower_Update, 0x006B9AA0, uint8_t* self, uint8_t* model, float dt);
GAME_FN(uint32_t, __thiscall, Attachment_HasOffset, 0x006B5650, uint8_t* self);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, ClampedOffset, 0x006B43D0, CVector* out, const CVector* from, const CVector* to,
        float maxScale, float minScale, float scale);
GAME_FN(uint32_t, __cdecl, Vec_WithinTolerance, 0x006B2D50, const CVector* a, const CVector* b, float tolerance);
GAME_FN(float, __cdecl, Distance, 0x0041A950, const CVector* a, const CVector* b);
GAME_FN(float, __cdecl, TriangleAngle, 0x006B20F0, float a, float b, float c);
GAME_FN(uint8_t, __cdecl, NearlyEqual, 0x00432A60, float a, float b, float epsilon);
GAME_FN(uint8_t, __thiscall, Vector_ClampToCone, 0x00414E70, CVector* self, const CVector* axis, float cosLimit);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(void, __cdecl, RotationBetween, 0x006B5BB0, Xform* out, const CVector* a, const CVector* b, int mode);
GAME_FN(void, __cdecl, Transform_Invert, 0x00522440, Xform* out, const Xform* in);
GAME_FN(void, __cdecl, Transform_PreMultiply, 0x00415E00, Xform* out, const Xform* a, const Xform* b);
GAME_FN(void, __thiscall, Model_SetJointTransform, 0x006B9C20, uint8_t* model, uint32_t joint, const Xform* t,
        int subtree, int update, float dt, int mode);
GAME_FN(void, __thiscall, Quaternion_Set, 0x00465620, Quat* q, const Xform* rotation);
GAME_FN(void, __thiscall, Quaternion_Slerp, 0x004143C0, Quat* self, const Quat* a, const Quat* b, float t);
GAME_FN(CVector*, __cdecl, CrossProduct, 0x00414730, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __cdecl, SafeAcos, 0x00413D60, float x);
GAME_FN(void, __cdecl, Transform_SetRotation, 0x00415EB0, Xform* t, const CVector* axis, float degrees, int mode);
GAME_FN(CVector*, __cdecl, RotateAboutAxis, 0x006B5B40, CVector* out, const CVector* point, const CVector* axis,
        float radians);
GAME_FN(float*, __thiscall, NiMatrix3_MultiplyVector, 0x00768A70, const Xform* self, CVector* out, const CVector* v);
GAME_FN(Xform*, __thiscall, NiTransform_Multiply, 0x00406830, const Xform* self, Xform* out, const Xform* other);
GAME_FN(void, __thiscall, Quat_ToNiTransform, 0x00413E60, const Quat* q, Xform* out);
GAME_FN(void, __thiscall, Model_VisitSubtree, 0x006B21A0, uint8_t* model, int joint, Visitor* visitor,
        int visitFirst);

inline float Length(const CVector& v) {
    return std::sqrt(std::fabs((v.x * v.x + v.y * v.y) + v.z * v.z));
}

inline void SetDirty(uint8_t* model, uint32_t joint) {
    uint32_t* bits = &At<uint32_t>(model, 0x8C);
    if (int32_t(joint) >= 32) {
        bits += joint >> 5;
        joint -= (joint >> 5) << 5;
    }
    *bits |= 1u << joint;
}

inline void ZeroTranslation(Xform* t) {
    std::memcpy(&t->translation, ik_zeroVector, sizeof(CVector));
}

// 0x6BA380: solves one IK effector on the model
void __fastcall Model_SolveIKEffector(uint8_t* self, re::Edx, uint8_t* effector, float dt) {
    uint8_t* skeleton = At<uint8_t*>(self, 0x14);
    Follower_Update(effector, self, dt);
    uint32_t hasOffset = Attachment_HasOffset(effector);
    Quat wanted;  // F-0x1E0
    wanted.x = At<float>(effector, 0x14);
    wanted.y = At<float>(effector, 0x18);
    wanted.z = At<float>(effector, 0x1C);
    wanted.w = At<float>(effector, 0x10);
    CVector target = At<CVector>(effector, 4);  // F-0x20C
    uint8_t* chain = At<uint8_t*>(effector, 0);
    Xform* transforms = At<Xform*>(skeleton, 8);
    Xform* upper = &transforms[At<uint32_t>(chain, 0x20)];
    Xform* middle = &transforms[At<uint32_t>(chain, 0x24)];
    Xform* end = middle + 1;
    CVector* upperPos = &upper->translation;

    CVector a;
    CVector b;
    CVector tmp;
    a = middle->translation;
    b = *upperPos;
    float upperLength = Length(*VectorSub(&tmp, &b, &a));  // F-0x268
    a = end->translation;
    b = middle->translation;
    float lowerLength = Length(*VectorSub(&tmp, &b, &a));  // F-0x22C
    float reach = lowerLength + upperLength;               // F-0x2CC (and F-0x174)

    a = *upperPos;
    CVector offset = *ClampedOffset(&tmp, &a, &target, At<float>(chain, 0x2C), At<float>(chain, 0x28), reach);
    CVector endPos = end->translation;  // F-0x244
    CVector goal;                       // F-0x264
    VectorAdd(&goal, upperPos, &offset);
    if (Vec_WithinTolerance(&endPos, &goal, 0.001f)) {
        return;
    }

    if (ik_alwaysBendParent != 0 || (ik_allowBendParent != 0 && (At<uint8_t>(effector, 0x38) & 4))) {
        a = *upperPos;
        float distance = Distance(&a, &target);  // F-0x200
        if (distance > reach) {
            int32_t parentJoint = At<int32_t>(At<uint8_t*>(skeleton, 0xC) + At<uint32_t>(chain, 0x20) * 0x14, 0);
            Xform* parent = &transforms[parentJoint];
            CVector upperCopy = *upperPos;
            CVector parentCopy = parent->translation;
            float parentLength = Distance(&parentCopy, &upperCopy);  // F-0x2D4
            CVector parentPos = parent->translation;                 // F-0x2AC
            CVector toTarget;                                        // F-0x2C8
            toTarget.x = target.x - parentPos.x;
            toTarget.y = target.y - parentPos.y;
            toTarget.z = target.z - parentPos.z;
            float targetDistance = Length(toTarget);  // F-0x2B8
            Vector_Normalise(&toTarget);
            CVector toUpper;  // F-0x2A0
            toUpper.x = upperPos->x - parentPos.x;
            toUpper.y = upperPos->y - parentPos.y;
            toUpper.z = upperPos->z - parentPos.z;
            Vector_Normalise(&toUpper);
            if (!(ik_parentLimitGuard & 1)) {
                ik_parentLimitGuard |= 1;
                ik_parentLimit = 0.34906587f;
            }
            float turn = ik_parentLimit;  // F-0x2B0
            float armReach = std::fabs(parentLength + reach);
            if (armReach - targetDistance > 0.001f) {
                float now = TriangleAngle(parentLength, targetDistance, distance);
                if (0.0f > now) {
                    now = 0.0f;
                }
                float fits = TriangleAngle(parentLength, targetDistance, reach);
                if (fits > 0.0f && now > 0.0f) {
                    float difference = std::fabs(now - fits);
                    if (ik_parentLimit < difference) {
                        turn = ik_parentLimit;
                    } else {
                        if (NearlyEqual(difference, 0.0f, 9.99999975e-06f)) {
                            goto fade;
                        }
                        turn = difference;
                    }
                }
            }
            {
                float cosTurn = trk::CrtCos(turn);
                Vector_ClampToCone(&toTarget, &toUpper, cosTurn);
                Vector_Normalise(&toTarget);
                Xform rotation = *parent;  // F-0x1B4
                ZeroTranslation(&rotation);
                RotationBetween(&rotation, &toUpper, &toTarget, 2);
                Xform inverse;   // F-0x138
                Xform combined;  // F-0x16C
                inverse.scale = 1.0f;
                combined.scale = 1.0f;
                rotation.translation = parentPos;
                Transform_Invert(&inverse, parent);
                Transform_PreMultiply(&combined, &inverse, &rotation);
                Model_SetJointTransform(self, uint32_t(parentJoint), &combined, 1, 0, -1.0f, 2);
            }
            if (ik_parentOnly != 0) {
                return;
            }
            CVector from = *upperPos;
            offset = *ClampedOffset(&tmp, &from, &target, At<float>(chain, 0x2C), At<float>(chain, 0x28), reach);
            endPos = end->translation;
            goal = *VectorAdd(&tmp, upperPos, &offset);
        }
    }

fade: {
    float fade = 0.0f;  // F-0x2CC
    float fadeTime = At<float>(effector, 0x28);
    if (!(0.0f == fadeTime)) {
        float left = 0.0f < fadeTime ? fadeTime : At<float>(effector, 0x2C) + fadeTime;
        fade = 1.0f - left / At<float>(effector, 0x2C);
    }
    if (!(std::fabs(fade) < 0.001f)) {
        if (hasOffset) {
            Quat current;
            Quaternion_Set(&current, end);
            Quaternion_Slerp(&wanted, &current, &wanted, fade);
        }
        CVector delta;
        VectorSub(&delta, &goal, &endPos);
        CVector scaled;
        scaled.x = delta.x * fade;
        scaled.y = delta.y * fade;
        scaled.z = fade * delta.z;
        goal = *VectorAdd(&tmp, &endPos, &scaled);
        a = *upperPos;
        offset = *VectorSub(&tmp, &goal, &a);
    }
}
    float goalDistance = Length(offset);  // F-0x2B8
    Vector_Normalise(&offset);            // now the goal's direction
    CVector root = *upperPos;             // F-0x2A0
    CVector middleRel = middle->translation;  // F-0x258
    CVector bent{0.0f, 0.0f, 0.0f};          // F-0x238
    CVector endRel;                           // F-0x2AC
    endRel.x = endPos.x - root.x;
    endRel.y = endPos.y - root.y;
    endRel.z = endPos.z - root.z;
    CVector goalRel;  // F-0x2C8
    goalRel.x = goal.x - root.x;
    goalRel.y = goal.y - root.y;
    goalRel.z = goal.z - root.z;
    middleRel.x = middleRel.x - root.x;
    middleRel.y = middleRel.y - root.y;
    middleRel.z = middleRel.z - root.z;
    CVector endDir = endRel;    // F-0x1F8
    CVector goalDir = goalRel;  // F-0x1EC
    Vector_Normalise(&endDir);
    Vector_Normalise(&goalDir);
    float dot = (endDir.y * goalDir.y + goalDir.x * endDir.x) + endDir.z * goalDir.z;
    if (1.0f < dot) {
        return;
    }
    CVector hinge;  // F-0x224: the middle joint's third axis
    hinge.x = middle->rotation[2];
    hinge.y = middle->rotation[5];
    hinge.z = middle->rotation[8];
    Vector_Normalise(&hinge);
    {
        Xform swing;  // F-0x104 (built and dropped)
        swing.scale = 1.0f;
        CVector normal;  // F-0x180
        CrossProduct(&normal, &endDir, &goalDir);
        Vector_Normalise(&normal);
        float degrees = SafeAcos(dot) * 180.0f / kPi;
        Transform_SetRotation(&swing, &normal, degrees, 0);
        ZeroTranslation(&swing);
    }
    float bend = 0.0f;   // F-0x2CC
    float knee = kPi;    // F-0x2B0
    bool bentDone = false;
    if (std::fabs(reach - goalDistance) > 0.001f) {
        bend = TriangleAngle(goalDistance, upperLength, lowerLength);
        if (0.0f > bend) {
            bend = 0.0f;
        }
        knee = TriangleAngle(upperLength, lowerLength, goalDistance);
        if (0.0f > knee) {
            knee = 0.0f;
        }
        if (bend > 0.0f) {
            RotateAboutAxis(&bent, &offset, &hinge, bend);
            bent.x = bent.x * upperLength;
            bent.y = bent.y * upperLength;
            bent.z = upperLength * bent.z;
            bentDone = true;
        }
    }
    if (!bentDone) {
        bent.x = offset.x * upperLength;
        bent.y = offset.y * upperLength;
        bent.z = upperLength * offset.z;
    }
    Vector_Normalise(&middleRel);
    Vector_Normalise(&bent);  // (unused after)
    CVector middleDir = middleRel;  // F-0x1D0
    Vector_Normalise(&middleDir);
    CVector lower;  // F-0x218 (unused after)
    lower.x = endRel.x - middleRel.x;
    lower.y = endRel.y - middleRel.y;
    lower.z = endRel.z - middleRel.z;
    Vector_Normalise(&lower);
    Xform aim;  // F-0x9C
    aim.scale = 1.0f;
    RotationBetween(&aim, &middleDir, &offset, 0);

    upperPos->x = upperPos->x - root.x;
    upperPos->y = upperPos->y - root.y;
    upperPos->z = upperPos->z - root.z;
    middle->translation.x = middle->translation.x - root.x;
    middle->translation.y = middle->translation.y - root.y;
    middle->translation.z = middle->translation.z - root.z;
    Xform inverse;  // F-0x68
    inverse.scale = 1.0f;
    Transform_Invert(&inverse, upper);
    Xform turn = aim;  // F-0xD0
    const float* r = NiMatrix3_MultiplyVector(&turn, &tmp, &hinge);
    hinge = CVector{r[0], r[1], r[2]};
    Transform_SetRotation(&turn, &hinge, bend * 180.0f / kPi, 2);
    Xform product;  // F-0x16C
    *upper = *NiTransform_Multiply(&turn, &product, upper);
    CVector middlePos = middle->translation;  // F-0x278
    r = NiMatrix3_MultiplyVector(&inverse, &tmp, &middlePos);
    middlePos = CVector{r[0], r[1], r[2]};
    r = NiMatrix3_MultiplyVector(upper, &tmp, &middlePos);
    middlePos = CVector{r[0], r[1], r[2]};
    *middle = *upper;
    middle->translation = middlePos;
    Transform_SetRotation(middle, &hinge, -(kPi - knee) * 180.0f / kPi, 2);
    middle->translation = middlePos;
    upperPos->x = root.x + upperPos->x;
    upperPos->y = root.y + upperPos->y;
    upperPos->z = root.z + upperPos->z;
    middle->translation.x = root.x + middle->translation.x;
    middle->translation.y = root.y + middle->translation.y;
    middle->translation.z = root.z + middle->translation.z;

    uint32_t endJoint = At<uint32_t>(chain, 0x24) + 1;
    if (hasOffset) {
        Xform placed;  // F-0x1B4
        Xform inverseEnd;  // F-0x16C
        Xform moved;  // F-0x138
        placed.scale = 1.0f;
        inverseEnd.scale = 1.0f;
        moved.scale = 1.0f;
        Quat_ToNiTransform(&wanted, &placed);
        placed.translation = goal;
        Transform_Invert(&inverseEnd, end);
        moved = *NiTransform_Multiply(&placed, &product, &inverseEnd);
        Model_SetJointTransform(self, endJoint, &moved, 1, 0, -1.0f, 2);
    } else {
        CVector shift;  // F-0x288
        VectorSub(&shift, &goal, &endPos);
        Visitor adjust{kAdjustJointVisitorVtbl, self, &shift, transforms};
        Model_VisitSubtree(self, int(endJoint), &adjust, 1);
    }
    Visitor midBones{kIKAdjustMidBonesVtbl, self, reinterpret_cast<void*>(uintptr_t(At<uint32_t>(chain, 0x20))),
                     reinterpret_cast<void*>(uintptr_t(At<uint32_t>(chain, 0x24)))};
    Model_VisitSubtree(self, At<int32_t>(chain, 0x20), &midBones, 1);
    SetDirty(self, At<uint32_t>(chain, 0x20));
    SetDirty(self, At<uint32_t>(chain, 0x24));
    SetDirty(self, At<uint32_t>(chain, 0x24) + 1);
}

} // namespace

#define AMIKSOLVE_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMIKSOLVE_REVERSED("AM_Model::SolveIKEffector", 0x006BA380, &Model_SolveIKEffector);
