// A ped's head look (CHeadLook, 0x3C bytes, a member of the ped): the head and spine
// turned towards a target entity, a point, where the camera looks, or where the ped's
// something at +0x2E0 points, within limits, blended by distance. The ped being worked
// on is the global 0xC2C13C, the tuning 0xC2C138 (from 0x20C8934).
//
// The bones are NiTransforms (0x34 bytes: a 3x3 rotation, the translation at +0x24,
// the scale at +0x30) in the ped's animation data (+0x114, its +0x14's +8 the array).
// Angles are in degrees on the look (+0x14.. / +0x20..), radians on the bones.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

struct Vec {
    float x, y, z;
};
struct Vec4 {
    float x, y, z, w;
};
struct NiTransform {
    float rot[9];   // +0x0 (rows of 3)
    Vec trans;      // +0x24
    float scale;    // +0x30
};
static_assert(sizeof(NiTransform) == 0x34);

struct CHeadLook {
    re::Ptr32<uint8_t> target;        // +0x0 (a registered reference)
    Vec point;              // +0x4 (vecUnset: none)
    re::Ptr32<uint8_t> info;          // +0x10 the bone info (+0x18 yaw speed, +0x1C pitch speed, +0x20 five
                            // weights, +0x34 / +0x38 the spine's, +0x3C the head bone, +0x40 / +0x44
                            // the yaw's max / min)
    float yaw;              // +0x14
    float yawTarget;        // +0x18
    float yawSpeed;         // +0x1C
    float pitch;            // +0x20
    float pitchTarget;      // +0x24
    float pitchSpeed;       // +0x28
    float blend;            // +0x2C
    float lastHeading;      // +0x30 the body's, last frame (radians)
    int32_t targetBone;     // +0x34
    int8_t active;          // +0x38 (> 0: on)
    uint8_t flags;          // +0x39 1 camera, 2, 4 the spine too, 8 the ped's own target
                            // (when it can see it), 0x10 the target set from outside
    uint8_t unknown3A[2];
};
static_assert(offsetof(CHeadLook, info) == 0x10);
static_assert(offsetof(CHeadLook, yaw) == 0x14);
static_assert(offsetof(CHeadLook, pitch) == 0x20);
static_assert(offsetof(CHeadLook, blend) == 0x2C);
static_assert(offsetof(CHeadLook, lastHeading) == 0x30);
static_assert(offsetof(CHeadLook, targetBone) == 0x34);
static_assert(offsetof(CHeadLook, active) == 0x38);
static_assert(offsetof(CHeadLook, flags) == 0x39);
static_assert(sizeof(CHeadLook) == 0x3C);

GAME_VAR(uint8_t*, g_subject, 0x00C2C13C);    // the ped
GAME_VAR(uint8_t*, g_tuning, 0x00C2C138);     // +0x18 five limits (min yaw, max yaw, min pitch, max
                                              // pitch), +0x68 / +0x6C the blend's near / far, +0x70 /
                                              // +0x74 seeing near / far, +0x78 the field of view
GAME_VAR(uint8_t*, g_defaultTuning, 0x020C8934);
GAME_VAR(uint8_t*, g_defaultInfo, 0x020C8938);
GAME_VAR(float, g_pitchSum, 0x00C2C130);      // (radians, over the bones done this frame)
GAME_VAR(float, g_yawSum, 0x00C2C134);
GAME_VAR(Vec4, g_axisA, 0x00C2C140);
GAME_VAR(Vec4, g_axisB, 0x00C2C150);
GAME_VAR(Vec4, g_axis0, 0x00C2C160);          // the last bone's axes
GAME_VAR(Vec4, g_axis1, 0x00C2C170);
GAME_VAR(Vec4, g_axis2, 0x00C2C180);
GAME_VAR(Vec, vecUnset, 0x00BCC564);
GAME_VAR(float, g_halfPi, 0x00A351F0);
GAME_ARRAY(int32_t, g_neckBones, 0x00A351BC);   // 0xA.. 0xE (the loop can read the one before)
GAME_ARRAY(int32_t, g_spineBones, 0x00A351D0);  // two chains of four
GAME_VAR(void*, TheCamera, 0x00C3CCF4);
GAME_VAR(int32_t, g_cameraMode, 0x00C3CCF8);
GAME_VAR(int32_t, g_poiModel, 0x00BF42DC);
GAME_VAR(void*, g_boneRoot, 0x01B6551C);
inline void* const g_boneNames = reinterpret_cast<void*>(0x01B65520);

constexpr double kPi = double(3.1415927f);

GAME_FN(float, __cdecl, AngleDiff, 0x00432980, float a, float b);
GAME_FN(float, __cdecl, VecHeading, 0x00433190, const Vec* v);
GAME_FN(float, __cdecl, VecPitch, 0x00433490, const Vec* v);
GAME_FN(float, __cdecl, VecHeadingLimited, 0x00433310, const Vec* v);
GAME_FN(float, __cdecl, LimitAngle, 0x00432EE0, float angle);
GAME_FN(Vec*, __cdecl, VecFromHeading, 0x00432DF0, Vec* out, float heading);
GAME_FN(Vec*, __cdecl, VectorAdd, 0x004129A0, Vec* out, const Vec* a, const Vec* b);
GAME_FN(Vec*, __cdecl, VectorSub, 0x00414B70, Vec* out, const Vec* a, const Vec* b);
GAME_FN(float, __thiscall, Vec_Normalise, 0x00414E10, Vec* v);
GAME_FN(Vec*, __thiscall, Entity_GetForward, 0x004154A0, void* entity, Vec* out);
GAME_FN(void, __thiscall, NiTransform_MakeIdentity, 0x00770920, NiTransform* t);
GAME_FN(NiTransform*, __thiscall, NiTransform_Mul, 0x00406830, const NiTransform* t, NiTransform* out, const NiTransform* other);
GAME_FN(void, __cdecl, RotateAboutAxis, 0x00415EB0, NiTransform* t, const Vec4* axis, float angle, int mode);
GAME_FN(void, __thiscall, AnimData_SetBone, 0x006B9C20, void* animData, int bone, const NiTransform* t, int, int, float, int);
GAME_FN(void, __cdecl, SmoothTowards, 0x004331C0, float* value, float* speed, float target, float maxSpeed, float accel);
GAME_FN(uint8_t, __thiscall, Ped_465A60, 0x00465A60, void* ped, int);
GAME_FN(uint8_t*, __thiscall, Ped_GetTarget, 0x00497100, void* ped);
GAME_FN(uint8_t, __thiscall, Ped_IsPoiModel, 0x004473E0, void* ped);
GAME_FN(Vec*, __thiscall, Targeting_GetTargetPos, 0x004D6000, void* targeting, Vec* out, void* entity, int);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, void* entity, void* ref);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(uint8_t*, __cdecl, FindBoneInfo, 0x0072AEA0, void* path, void* names);
GAME_FN(void, __thiscall, EmptyThiscall, 0x0049A650, void* self);
// (their own, called where modes apply)
GAME_FN(void, __thiscall, HL_RotateAboutPivot, 0x004A34B0, CHeadLook* self, NiTransform* t, const Vec* pivot, float yaw, float pitch);
GAME_FN(void, __thiscall, HL_TurnBone, 0x004A35E0, CHeadLook* self, void* animData, NiTransform* t, int bone,
        const float* limits, float weight, float* yaw, float* pitch, int flag);
GAME_FN(uint8_t, __thiscall, HL_CanSee, 0x004A30F0, CHeadLook* self, uint8_t* entity);
GAME_FN(void, __thiscall, HL_UpdateTarget, 0x004A3AE0, CHeadLook* self);
GAME_FN(void, __thiscall, HL_TurnSpine, 0x004A3BB0, CHeadLook* self, void* animData, const NiTransform* t, float yaw, float pitch);
GAME_FN(void, __thiscall, HL_ApplyToBones, 0x004A3CD0, CHeadLook* self);
GAME_FN(void, __thiscall, HL_UpdateBodyTurn, 0x004A2C80, CHeadLook* self);
GAME_FN(void, __thiscall, HL_AimAtPoint, 0x004A2A60, CHeadLook* self, const Vec* forward);
GAME_FN(void, __thiscall, HL_AimAtTarget, 0x004A2DE0, CHeadLook* self, const Vec* forward);
GAME_FN(void, __thiscall, HL_AimAlong2E0, 0x004A2F80, CHeadLook* self);
GAME_FN(void, __thiscall, HL_AimWithCamera, 0x004A3050, CHeadLook* self, const Vec* forward);

const Vec* EntityPos(uint8_t* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<Vec*>(matrix + 0x30) : reinterpret_cast<Vec*>(entity + 4);
}
// GTA's GetHeading: atan2(-forward.x, forward.y), else the placement's
float EntityHeading(uint8_t* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    if (matrix) {
        return float(std::atan2(-double(At<float>(matrix, 0x10)), double(At<float>(matrix, 0x14))));
    }
    return At<float>(entity, 0x10);
}
NiTransform* Bones(uint8_t* ped) {
    return At<NiTransform*>(At<uint8_t*>(At<uint8_t*>(ped, 0x114), 0x14), 8);
}
float Deg(float radians) {
    return float(double(radians) * 180.0 / kPi);
}
bool IsUnset(const Vec& v) {
    return v.x == vecUnset.x && v.y == vecUnset.y && v.z == vecUnset.z;
}

} // namespace

namespace c09headlook {

// 0x4A2A60: aimed at its point, from the head bone: the yaw and pitch between
// `forward` and there
void __fastcall AimAtPoint(CHeadLook* self, re::Edx, const Vec* forward) {
    Vec bone = Bones(g_subject)[At<int32_t>(self->info, 0x3C)].trans;
    Vec to;
    VectorSub(&to, &self->point, &bone);
    float fromHeading = VecHeading(forward);
    float fromPitch = VecPitch(forward);
    float toHeading = VecHeading(&to);
    float toPitch = VecPitch(&to);
    self->yawTarget = Deg(AngleDiff(fromHeading, toHeading));
    self->pitchTarget = Deg(AngleDiff(fromPitch, toPitch));
}

// 0x4A2B40: its bone info by name (the default one when there's no such)
void __fastcall SetBoneInfo(CHeadLook* self, re::Edx, re::Ptr32<const char>* name) {
    self->info = nullptr;
    if (*name) {
        // a path of two names, the root's and this one
        struct {
            re::Ptr32<const char> names[2];
            struct {
                int32_t count;
                re::Ptr32<re::Ptr32<const char>> names;
            } path;
        } locals;
        locals.names[0] = static_cast<const char*>(g_boneRoot);
        locals.names[1] = *name;
        locals.path.count = 2;
        locals.path.names = locals.names;
        self->info = FindBoneInfo(&locals.path, g_boneNames);
        for (int i = 1; i >= 0; i--) {
            EmptyThiscall(&locals.names[i]);   // (the names' destructors)
        }
    }
    if (!self->info) {
        self->info = g_defaultInfo;
    }
}

// 0x4A2BD0: the constructor (for its ped; the tuning set to the default)
CHeadLook* __fastcall Construct(CHeadLook* self, re::Edx, uint8_t* ped) {
    self->target = nullptr;
    self->point = {0.0f, 0.0f, 0.0f};
    g_tuning = g_defaultTuning;
    self->yaw = 0.0f;
    self->flags &= 0xE8;
    self->yawTarget = 0.0f;
    self->yawSpeed = 0.0f;
    self->info = g_defaultInfo;
    self->pitch = 0.0f;
    self->active = 0;
    self->pitchTarget = 0.0f;
    self->pitchSpeed = 0.0f;
    self->blend = 1.0f;
    self->point = vecUnset;
    self->lastHeading = EntityHeading(ped);
    return self;
}

// 0x4A2C80: the look turned back by as much as the body turned since last frame (so
// the head keeps looking where it did), not past zero
void __fastcall UpdateBodyTurn(CHeadLook* self, re::Edx) {
    float heading = EntityHeading(g_subject);
    float turned = Deg(AngleDiff(heading, self->lastHeading));
    int sign = 0.0f <= turned ? 1 : -1;
    float lookTurn = Deg(AngleDiff(float(double(self->yaw) * kPi / 180.0), float(double(self->yawTarget) * kPi / 180.0)));
    if (turned != 0.0f) {
        int lookSign = 0.0f <= lookTurn ? 1 : -1;
        if (sign == lookSign) {
            float a = std::fabs(self->yaw);
            float b = std::fabs(turned);
            float step = a <= b ? a : b;
            self->yaw = float(float(sign) * step) + self->yaw;
        }
    }
    self->lastHeading = heading;
}

// 0x4A2DE0: aimed at its target (a ped's bone when it has one set and isn't a POI,
// else the entity's position; not a ped: where the targeting puts it)
void __fastcall AimAtTarget(CHeadLook* self, re::Edx, const Vec* forward) {
    int32_t targetBone = self->targetBone;
    Vec bone = Bones(g_subject)[At<int32_t>(self->info, 0x3C)].trans;
    Vec pos = {0.0f, 0.0f, 0.0f};
    uint8_t* target = self->target;
    if (targetBone == 0 || (At<uint8_t>(target, 0x108) & 7) != 3) {
        Vec out;
        pos = *Targeting_GetTargetPos(At<void*>(g_subject, 0x1C8), &out, self->target, 0);
    } else {
        bool useBone = true;
        if (At<int16_t>(target, 0x10E) == g_poiModel || Ped_IsPoiModel(target)) {
            useBone = false;
        }
        if (At<void*>(target, 0x114) && useBone) {
            pos = Bones(target)[targetBone].trans;
        } else {
            pos = *EntityPos(target);
        }
    }
    Vec to;
    VectorSub(&to, &pos, &bone);
    float fromHeading = VecHeading(forward);
    float fromPitch = VecPitch(forward);
    float toHeading = VecHeading(&to);
    float toPitch = VecPitch(&to);
    self->yawTarget = Deg(AngleDiff(fromHeading, toHeading));
    self->pitchTarget = Deg(AngleDiff(fromPitch, toPitch));
}

// 0x4A2F80: turned (yaw only) the way the ped's +0x2E0 vector points (its first when
// its state is 0xD, else its second); nothing when that's unset
void __fastcall AimAlong2E0(CHeadLook* self, re::Edx) {
    float* p = At<float*>(g_subject, 0x2E0);
    if (At<int32_t>(g_subject, 0x1310) != 0xD) {
        p += 3;
    }
    Vec v = {p[0], p[1], p[2]};
    self->pitchTarget = 0.0f;
    float yaw = 0.0f;
    if (!IsUnset(v)) {
        Vec forward;
        Entity_GetForward(g_subject, &forward);
        float fromHeading = VecHeading(&forward);
        float toHeading = VecHeading(&v);
        yaw = Deg(AngleDiff(fromHeading, toHeading));
    }
    self->yawTarget = yaw;
}

// 0x4A3050: pitched the way the camera looks (no yaw)
void __fastcall AimWithCamera(CHeadLook* self, re::Edx, const Vec* forward) {
    uint8_t* transform = GetLocalTransform(GetCameraNode(TheCamera));
    Vec look = {At<float>(transform, 8), At<float>(transform, 0x14), At<float>(transform, 0x20)};
    float fromPitch = VecPitch(forward);
    float toPitch = VecPitch(&look);
    self->yawTarget = 0.0f;
    self->pitchTarget = Deg(AngleDiff(fromPitch, toPitch));
}

// 0x4A30F0: the ped can see the entity: not a ped in state 0xD that's busy; close
// enough (seeing near: always), and within the field of view: not in the blind cone
// behind, and in front of the view's side planes. (A null entity is read through,
// as the game does)
uint8_t __fastcall CanSee(CHeadLook*, re::Edx, uint8_t* entity) {
    if (entity && (At<uint8_t>(entity, 0x108) & 7) == 3 && At<int32_t>(entity, 0x1310) == 0xD &&
        (At<uint8_t>(entity, 0x1E91) != 0 || At<int32_t>(entity, 0x1E94) != 0)) {
        return 0;
    }
    float seeFar = At<float>(g_tuning, 0x74);
    float seeNear = At<float>(g_tuning, 0x70);
    float fov = At<float>(g_tuning, 0x78);
    Vec entityPos = *EntityPos(entity);
    Vec subjectPos = *EntityPos(g_subject);
    Vec to;
    VectorSub(&to, &entityPos, &subjectPos);
    float dist = float(std::sqrt(std::fabs(float(double(to.y) * to.y + double(to.x) * to.x))));
    if (!(seeNear < dist || seeNear == dist)) {
        return 1;
    }
    if (!(seeFar > dist)) {
        return 0;
    }
    Vec forward;
    Entity_GetForward(g_subject, &forward);
    float forwardHeading = VecHeadingLimited(&forward);
    float toHeading = VecHeadingLimited(&to);
    float diff = AngleDiff(forwardHeading, toHeading);
    float half = float(double(fov) * 0.5 * kPi / 180.0);
    float rest = float(double(g_halfPi) - half);   // the blind cone's half angle
    if (0.0f < rest) {
        float behind = LimitAngle(float(double(diff) + kPi));
        if (-rest < behind && behind < rest) {
            return 0;
        }
    }
    // the view's two sides: planes through the points seeing-near out along them
    Vec side, offset, point, rel;
    float sideA = LimitAngle(float(double(half) + forwardHeading + g_halfPi));
    VecFromHeading(&side, sideA);
    offset = {seeNear * side.x, seeNear * side.y, seeNear * side.z};
    VectorAdd(&point, &subjectPos, &offset);
    VectorSub(&rel, &entityPos, &point);
    float dotA = float(double(rel.y) * side.y + double(side.x) * rel.x + double(rel.z) * side.z);
    bool behindA = dotA < 0.0f;
    float sideB = LimitAngle(float(double(forwardHeading) - half - g_halfPi));
    VecFromHeading(&side, sideB);
    offset = {seeNear * side.x, seeNear * side.y, seeNear * side.z};
    VectorAdd(&point, &subjectPos, &offset);
    VectorSub(&rel, &entityPos, &point);
    float dotB = float(double(rel.y) * side.y + double(side.x) * rel.x + double(rel.z) * side.z);
    bool behindB = 0.0f > dotB;
    if (0.0f < rest) {
        return behindA && behindB;
    }
    return behindA || behindB;
}

// 0x4A34B0: the transform made a rotation about the pivot (yaw about axis A, then
// pitch about axis B). (It's reset to identity first: what it held is lost)
void __fastcall RotateAboutPivot(CHeadLook*, re::Edx, NiTransform* t, const Vec* pivot, float yaw, float pitch) {
    Vec minus = {-pivot->x, -pivot->y, -pivot->z};
    NiTransform_MakeIdentity(t);
    NiTransform shift;
    shift.scale = 1.0f;
    NiTransform_MakeIdentity(&shift);
    shift.trans = minus;
    NiTransform product;
    *t = *NiTransform_Mul(&shift, &product, t);
    Vec4 axisB = g_axisB;
    RotateAboutAxis(t, &axisB, pitch, 2);
    Vec4 axisA = g_axisA;
    RotateAboutAxis(t, &axisA, yaw, 2);
    Vec plus = *pivot;
    shift.scale = 1.0f;
    NiTransform_MakeIdentity(&shift);
    shift.trans = plus;
    *t = *NiTransform_Mul(&shift, &product, t);
}

// 0x4A35E0: one bone turned by its share (weight) of what's left of the yaw and
// pitch, within the limits (min yaw, max yaw, min pitch, max pitch: none, weight 1);
// what it took off the two, and added to the frame's sums; the bone set on the
// animation data
void __fastcall TurnBone(CHeadLook* self, re::Edx, void* animData, NiTransform* t, int bone, const float* limits, float weight,
                         float* yawLeft, float* pitchLeft, int flag) {
    NiTransform* b = &At<NiTransform*>(At<uint8_t*>(animData, 0x14), 8)[bone];
    bool noLimits = limits == nullptr;
    Vec column = {b->rot[1], b->rot[4], b->rot[7]};
    float a = float(double(column.x) * g_axis2.x + double(column.y) * g_axis2.y + double(column.z) * g_axis2.z);
    float c = float(double(column.x) * g_axis0.x + double(column.y) * g_axis0.y + double(column.z) * g_axis0.z);
    float yaw = -float(double(float(std::atan2(double(a), double(c)))) * 180.0 / kPi);
    float d = float(double(column.x) * g_axis1.x + double(column.y) * g_axis1.y + double(column.z) * g_axis1.z);
    float cc = float(double(c) * c);
    float aa = float(double(a) * a);
    float h = float(std::sqrt(std::fabs(float(double(cc) + aa))));
    float pitch = -float(double(float(std::atan2(double(d), double(h)))) * 180.0 / kPi);
    float columnPitch = float(double(VecPitch(&column)) + g_pitchSum);
    float yawWant = float(double(yaw) + float(double(weight) * *yawLeft));
    float pitchWant = float(double(pitch) + float(double(weight) * *pitchLeft));
    if (!noLimits) {
        if (limits[0] > yawWant) {
            yawWant = limits[0];
        } else if (limits[1] < yawWant) {
            yawWant = limits[1];
        }
        float minPitch = -limits[3];
        float maxPitch = -limits[2];
        if (minPitch > pitchWant) {
            pitchWant = minPitch;
        } else if (maxPitch < pitchWant) {
            pitchWant = maxPitch;
        }
    }
    float w = 1.0f;
    if (!noLimits) {
        w = c > 0.0f ? c : 0.0f;
    }
    float pitchTurn = float((double(pitchWant) - pitch) * w);
    float yawTurn = float(double(yawWant) - yaw);
    if (!noLimits) {
        float cosine = float(std::cos(double(float(double(pitchTurn) * kPi / 180.0 + columnPitch))));
        yawTurn = cosine < 0.0f ? 0.0f : float(double(cosine) * yawTurn);
    }
    *yawLeft -= yawTurn;
    *pitchLeft -= pitchTurn;
    g_yawSum = float(double(g_yawSum) + double(yawTurn) * kPi / 180.0);
    g_pitchSum = float(double(pitchTurn) * kPi / 180.0 + g_pitchSum);
    Vec pivot = b->trans;
    // the bone's axes kept for the next (their fourth floats are stack garbage in the
    // game; left as they are here)
    g_axis0.x = b->rot[1];
    g_axis0.y = b->rot[4];
    g_axis0.z = b->rot[7];
    g_axis1.x = b->rot[0];
    g_axis1.y = b->rot[3];
    g_axis1.z = b->rot[6];
    g_axis2.x = -b->rot[2];
    g_axis2.y = -b->rot[5];
    g_axis2.z = -b->rot[8];
    NiTransform turn;
    turn.scale = 1.0f;
    HL_RotateAboutPivot(self, &turn, &pivot, yawTurn, pitchTurn);
    NiTransform product;
    NiTransform result = *NiTransform_Mul(t, &product, &turn);
    AnimData_SetBone(animData, bone, &result, flag, flag, -1.0f, 2);
    *t = result;
}

// 0x4A3AE0: the target updated (unless set from outside): the ped's own (its +0x155C
// without one), dropped when flag 8 wants it seen and it can't be
void __fastcall UpdateTarget(CHeadLook* self, re::Edx) {
    if (self->flags & 0x10) {
        return;
    }
    uint8_t* target = Ped_GetTarget(g_subject);
    if (!target) {
        target = At<uint8_t*>(g_subject, 0x155C);
    }
    if ((self->flags & 8) && target && !HL_CanSee(self, target)) {
        if (self->target) {
            CleanUpOldReference(self->target, &self->target);
            self->target = nullptr;
        }
        return;
    }
    if (target == self->target) {
        return;
    }
    if (self->target) {
        CleanUpOldReference(self->target, &self->target);
    }
    self->target = target;
    if (target) {
        RegisterReference(target, &self->target);
    }
}

// 0x4A3B60: the target set from outside (nullptr: back to the ped's own)
void __fastcall SetTarget(CHeadLook* self, re::Edx, uint8_t* entity) {
    if (!entity) {
        self->flags &= 0xEF;
        HL_UpdateTarget(self);
        return;
    }
    if (entity != self->target) {
        if (self->target) {
            CleanUpOldReference(self->target, &self->target);
        }
        self->target = entity;
        RegisterReference(entity, &self->target);
    }
    self->flags |= 0x10;
}

// 0x4A3BB0: the two spine chains (bones at 0xA351D0) turned with the neck's transform
void __fastcall TurnSpine(CHeadLook* self, re::Edx, void* animData, const NiTransform* t, float yaw, float pitch) {
    float scales[2] = {At<float>(self->info, 0x34), At<float>(self->info, 0x38)};
    float yawLeftStart = -yaw;
    float pitchLeft = -pitch;
    for (int chain = 0; chain < 8; chain += 4) {
        float yawLeft = yawLeftStart;
        NiTransform m = *t;
        float pitchLeftNow = pitchLeft;
        AnimData_SetBone(animData, g_spineBones[chain], &m, 0, 0, -1.0f, 2);
        for (int i = 1; i < 3; i++) {
            HL_TurnBone(self, animData, &m, g_spineBones[i + chain], nullptr, scales[i - 1], &yawLeft, &pitchLeftNow, i == 2);
            if (i == 1) {
                AnimData_SetBone(animData, g_spineBones[chain + 3], &m, 0, 0, -1.0f, 2);
            }
        }
    }
}

// 0x4A3CD0: the neck bones turned by the look, each by its weight of what's left (the
// weights made fractions of the rest; the zero ones skipped), the spine with them (flag
// 4) at the third
void __fastcall ApplyToBones(CHeadLook* self, re::Edx) {
    // (laid out as the game's: the loop can start at -1 and read `last` as a weight)
    int32_t locals[6];
    int32_t& last = locals[0];
    float* w = reinterpret_cast<float*>(&locals[1]);
    uint8_t* info = self->info;
    for (int i = 0; i < 5; i++) {
        w[i] = At<float>(info, 0x20 + i * 4);
    }
    int first = -1;
    last = 0;
    float total = float(((((double(w[1]) + w[0]) + w[2]) + w[3]) + w[4]));
    const float eps = 1e-05f;
    if (eps < std::fabs(w[0])) {
        first = 0;
        last = 0;
        float cur = w[0];
        w[0] = float(double(cur) / total);
        total = float(double(total) - cur);
    }
    for (int i = 1; i < 4; i++) {
        float cur = w[i];
        if (!(std::fabs(cur) <= eps)) {
            w[i] = float(double(cur) / total);
            total = float(double(total) - cur);
            if (first == -1) {
                first = i;
            }
            last = i;
        }
    }
    if (!(std::fabs(w[4]) <= eps)) {
        w[4] = float(double(w[4]) / total);
        if (first == -1) {
            first = 4;
        }
        last = 4;
    }
    float yawLeft = self->yaw;
    uint8_t* animData = At<uint8_t*>(g_subject, 0x114);
    NiTransform* bones = At<NiTransform*>(At<uint8_t*>(animData, 0x14), 8);
    float pitchLeft = self->pitch;
    // (the fourth floats of these are stack garbage in the game)
    g_axisA.x = 0.0f;
    g_axisA.z = 1.0f;
    g_axisA.y = 0.0f;
    Vec v = {bones[9].rot[0], bones[9].rot[3], 0.0f};
    Vec_Normalise(&v);
    g_axisB.x = v.x;
    g_axisB.y = v.y;
    g_axisB.z = v.z;
    g_axis0.x = bones[2].rot[1];
    g_axis0.y = bones[2].rot[4];
    g_axis0.z = bones[2].rot[7];
    g_axis1.x = bones[2].rot[0];
    g_axis1.y = bones[2].rot[3];
    g_axis1.z = bones[2].rot[6];
    g_axis2.x = -bones[2].rot[2];
    g_axis2.y = -bones[2].rot[5];
    g_axis2.z = -bones[2].rot[8];
    g_yawSum = 0.0f;
    g_pitchSum = 0.0f;
    NiTransform t;
    t.scale = 1.0f;
    NiTransform_MakeIdentity(&t);
    for (int i = first; i <= last; i++) {
        const float* limits = reinterpret_cast<const float*>(g_tuning + 0x18 + i * 0x10);
        HL_TurnBone(self, animData, &t, g_neckBones[i], limits, w[i], &yawLeft, &pitchLeft, i == 4);
        if (i != 2) {
            continue;
        }
        if (self->flags & 4) {
            Vec4 axis0 = g_axis0, axis1 = g_axis1, axis2 = g_axis2;
            float yawSum = g_yawSum, pitchSum = g_pitchSum;
            HL_TurnSpine(self, animData, &t, self->yaw - yawLeft, self->pitch - pitchLeft);
            g_yawSum = yawSum;
            g_pitchSum = pitchSum;
            g_axis0 = axis0;
            g_axis1 = axis1;
            g_axis2 = axis2;
        } else {
            for (int j = 0; j < 8; j += 4) {
                AnimData_SetBone(animData, g_spineBones[j], &t, 1, 0, -1.0f, 2);
            }
        }
    }
    if (last < 4) {
        AnimData_SetBone(animData, g_neckBones[last + 1], &t, 1, 1, -1.0f, 2);
    }
    self->yaw -= yawLeft;
    self->pitch -= pitchLeft;
}

// 0x4A4310: a frame of the ped's head look: the target updated, the aim picked
// (camera, target, +0x2E0's way, or the point), blended out with the camera's
// distance, the yaw kept within the info's range, both smoothed, and the bones turned
// when either is past a degree
void __fastcall Update(CHeadLook* self, re::Edx, uint8_t* ped) {
    g_subject = ped;
    if (Ped_465A60(ped, 8) ||
        (self->target && (self->flags & 8) && !(self->flags & 0x10) && Ped_465A60(g_subject, 4))) {
        HL_UpdateTarget(self);
    }
    uint8_t* subject = g_subject;
    bool camera = At<int32_t>(subject, 0x1310) == 0xD && (g_cameraMode == 0xD || g_cameraMode == 2) && (self->flags & 1);
    bool unset = IsUnset(self->point);
    if (self->active <= 0 || (!self->target && !camera && !(self->flags & 2) && unset)) {
        self->yawTarget = 0.0f;
        self->pitchTarget = 0.0f;
    } else {
        NiTransform* bones = Bones(subject);
        Vec forward = {bones[9].rot[1], bones[9].rot[4], bones[9].rot[7]};
        if (camera) {
            if (unset) {
                HL_AimWithCamera(self, &forward);
            } else {
                HL_AimAtPoint(self, &forward);
            }
        } else if (!(self->flags & 2) || self->target) {
            if (unset) {
                HL_AimAtTarget(self, &forward);
            } else {
                HL_AimAtPoint(self, &forward);
            }
        } else if (unset) {
            HL_AimAlong2E0(self);
        } else {
            HL_AimAtPoint(self, &forward);
        }
    }
    float factor = 1.0f;
    uint8_t* transform = GetLocalTransform(GetCameraNode(TheCamera));
    Vec cameraPos = {At<float>(transform, 0x24), At<float>(transform, 0x28), At<float>(transform, 0x2C)};
    Vec pedPos = *EntityPos(g_subject);
    Vec to;
    Vec* d = VectorSub(&to, &cameraPos, &pedPos);
    float distSq = float((double(d->x) * d->x + double(d->y) * d->y) + double(d->z) * d->z);
    float nearDist = At<float>(g_tuning, 0x68);
    float farDist = At<float>(g_tuning, 0x6C);
    float nearSq = float(double(nearDist) * nearDist);
    if (nearSq < distSq) {
        float farSq = float(double(farDist) * farDist);
        if (farSq < distSq) {
            factor = 0.0f;
        } else {
            float dist = float(std::sqrt(std::fabs(distSq)));
            factor = float((double(dist) - nearDist) / (double(farDist) - nearDist));
        }
    }
    factor = float(double(self->blend) * factor);
    float yawSpeed = float(double(At<float>(self->info, 0x18)) * factor);
    float pitchSpeed = float(double(At<float>(self->info, 0x1C)) * factor);
    self->yawTarget = float(double(self->yawTarget) * factor);
    self->pitchTarget = float(double(self->pitchTarget) * factor);
    HL_UpdateBodyTurn(self);
    float minYaw = At<float>(self->info, 0x44);
    float maxYaw = At<float>(self->info, 0x40);
    if (minYaw > self->yawTarget) {
        self->yawTarget = minYaw;
    } else if (maxYaw < self->yawTarget) {
        self->yawTarget = maxYaw;
    }
    SmoothTowards(&self->yaw, &self->yawSpeed, self->yawTarget, yawSpeed, 0.01f);
    SmoothTowards(&self->pitch, &self->pitchSpeed, self->pitchTarget, pitchSpeed, 0.01f);
    self->yaw = float(double(self->yaw) * factor);
    self->pitch = float(double(factor) * self->pitch);
    if (At<int32_t>(g_subject, 0xA4) == 0) {
        if (1.0f < std::fabs(self->yaw) || 1.0f < std::fabs(self->pitch)) {
            HL_ApplyToBones(self);
        }
    }
}

} // namespace c09headlook

// ---- registration ---------------------------------------------------------------------

#define HEADLOOK_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

HEADLOOK_REVERSED("CHeadLook::AimAtPoint", 0x004A2A60, &c09headlook::AimAtPoint);
HEADLOOK_REVERSED("CHeadLook::SetBoneInfo", 0x004A2B40, &c09headlook::SetBoneInfo);
HEADLOOK_REVERSED("CHeadLook::Construct", 0x004A2BD0, &c09headlook::Construct);
HEADLOOK_REVERSED("CHeadLook::UpdateBodyTurn", 0x004A2C80, &c09headlook::UpdateBodyTurn);
HEADLOOK_REVERSED("CHeadLook::AimAtTarget", 0x004A2DE0, &c09headlook::AimAtTarget);
HEADLOOK_REVERSED("CHeadLook::AimAlong2E0", 0x004A2F80, &c09headlook::AimAlong2E0);
HEADLOOK_REVERSED("CHeadLook::AimWithCamera", 0x004A3050, &c09headlook::AimWithCamera);
HEADLOOK_REVERSED("CHeadLook::CanSee", 0x004A30F0, &c09headlook::CanSee);
HEADLOOK_REVERSED("CHeadLook::RotateAboutPivot", 0x004A34B0, &c09headlook::RotateAboutPivot);
HEADLOOK_REVERSED("CHeadLook::TurnBone", 0x004A35E0, &c09headlook::TurnBone);
HEADLOOK_REVERSED("CHeadLook::UpdateTarget", 0x004A3AE0, &c09headlook::UpdateTarget);
HEADLOOK_REVERSED("CHeadLook::SetTarget", 0x004A3B60, &c09headlook::SetTarget);
HEADLOOK_REVERSED("CHeadLook::TurnSpine", 0x004A3BB0, &c09headlook::TurnSpine);
HEADLOOK_REVERSED("CHeadLook::ApplyToBones", 0x004A3CD0, &c09headlook::ApplyToBones);
HEADLOOK_REVERSED("CHeadLook::Update", 0x004A4310, &c09headlook::Update);
