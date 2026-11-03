#include "game/Camera.h"

#include <cfloat>
#include <cmath>
#include "reversed.h"

// OffsetCameraController, the rest (0x505300..0x506BB0; its constructor, butes and
// clear are in FollowCamera4.cpp): the over-the-shoulder camera placed at an offset
// from the target (or at a node of an entity the butes name), turned by the look
// input within the pitch range of its attribute set (or the camera's look limits),
// pulled in where a sweep hits, and the crosshair shown while the view to what's
// aimed at is clear. Thiscalls are __fastcall wrappers with the unused edx.
//
// Layout (OffsetCameraController):
//   +4 the heading, +8 the heading needs setting from the butes, +0xC the offset (the
//   butes' +0x4C, raised by +0x124), +0x18 the pivot (the butes' +0x34, raised), +0x24
//   the pitch (radians from straight up), +0x28 the camera position, +0x34 the heading
//   offset (degrees, eased to +0x38 the look input's x), +0x3C the pitch (0..1, eased
//   to +0x40), +0x44 the look input ignored until it's let go, +0x110 the attribute
//   set, +0x114/+0x118 the crosshairs (clear / blocked view), +0x11C/+0x11D placed /
//   aim again this frame, +0x120/+0x124 the raise (wanted / eased), +0x128 the heading
//   the turn range is around, +0x12C the heading's share of the range (0..1), +0x134
//   a turn running (+0x138 to, +0x13C from, +0x140 since, +0x144/+0x148 the share
//   from/to), +0x14C..+0x164 the look limits saved while turning, +0x168 the eased
//   distance to the camera, +0x16C, +0x16D the view is clear, +0x170 the entity the
//   camera is on.
//
// The attribute set (the butes): +0x18/+0x1C the heading/pitch easing, +0x20/+0x24
// the pitch range (degrees from straight down), +0x28 the default pitch, +0x2C the
// heading offset (degrees), +0x30 the pitch offset (degrees), +0x34 the pivot,
// +0x40/+0x44/+0x48 the raise (most, rate, easing), +0x4C the offset, +0x60 on an
// entity's node (+0x64 its name), +0x68, +0x6C the fov, +0x70, +0x74 the distance the
// target fades in over, +0x78/+0x79 the look mode, +0x7A the sweep starts at the
// camera, +0x84..+0x98 the look input's tuning.
//
// The x87 code keeps sums in extended precision between float stores; the values
// here are computed in double and rounded where the game stores them.

#define OFFSETCAM_REVERSED(...) REVERSED(__VA_ARGS__)

namespace {

using cam::CVector;
using cam::NiTransform;

// the game's CMatrix: right, forward, up, position (4 floats each), the attached
// NiTransform
struct Matrix {
    float m[16];
    re::Ptr32<void> attached;
};
static_assert(sizeof(Matrix) == 0x44);

// a colpoint (0x28): its entity is a registered reference (let go when it's done)
struct ColPoint {
    CVector point;          // +0
    re::Ptr32<void> entity;           // +0xC
    CVector normal;         // +0x10
    uint8_t unknown1C[8];   // +0x1C
    float depth;            // +0x24
};
static_assert(sizeof(ColPoint) == 0x28);
static_assert(offsetof(ColPoint, depth) == 0x24);

// the camera's look limits (TheCamera+0x208, 0xC3CE70; CameraTracks.cpp's)
struct LookLimits {
    CVector direction;     // +0
    float pitchMax;        // +0xC
    float headingMax;      // +0x10
    float pitchMin;        // +0x14
    float headingMin;      // +0x18
    uint8_t enabled;       // +0x1C
    uint8_t unknown1D[3];  // +0x1D
    re::Ptr32<void> entity;          // +0x20 (a registered reference)
};
static_assert(sizeof(LookLimits) == 0x24);

GAME_VAR(LookLimits, lookLimits, 0x00C3CE70);
GAME_VAR(void*, theNiCamera, 0x00C3CCF4);           // TheCamera.camera
GAME_VAR(int32_t, cameraMode94, 0x00C3CCFC);        // TheCamera+0x94 (2: first person)
GAME_VAR(float, cameraFov, 0x00C3CD04);             // TheCamera.fov
GAME_VAR(int32_t, lookMode, 0x00C3CD70);            // TheCamera+0x108
GAME_VAR(float, lookX, 0x00C3CD8C);                 // TheCamera+0x124 the look input
GAME_VAR(float, lookY, 0x00C3CD90);                 // TheCamera+0x128
GAME_VAR(float, lookTuning94, 0x00C3CD94);          // TheCamera+0x12C..+0x13C: the look input's tuning
GAME_VAR(float, lookTuning98, 0x00C3CD98);
GAME_VAR(float, lookTuning9C, 0x00C3CD9C);
GAME_VAR(float, lookTuningA0, 0x00C3CDA0);
GAME_VAR(float, lookTuningA4, 0x00C3CDA4);
GAME_VAR(int32_t, cameraFovOverridden, 0x00C3CDD4); // TheCamera.fovOverridden
GAME_VAR(uint8_t*, cameraTarget, 0x00C3CE14);       // TheCamera.targetPed
GAME_VAR(void*, firstPersonController, 0x00C3CE9C); // TheCamera.controllers[2]
GAME_VAR(uint8_t, cameraFlag284, 0x00C3CEEC);       // TheCamera+0x284
GAME_VAR(uint8_t, cameraFovLocked, 0x00C3CF82);     // TheCamera+0x31A
GAME_VAR(float, timeStep, 0x00C1A9A4);              // CTimer::ms_fTimeStep
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);           // CTimer::m_snTimeInMilliseconds
GAME_VAR(void*, ignoreEntity, 0x00C1AE70);          // CWorld::pIgnoreEntity
GAME_VAR(uint16_t, currentScanCode, 0x00C1AE88);    // CWorld::ms_nCurrentScanCode
GAME_VAR(uint8_t, g_C1AE8A, 0x00C1AE8A);            // set around the sweeps
GAME_VAR(void*, ms_pAlwaysValidEntity, 0x00C2A8E8); // (compared with before a reference is let go: both ways the same)
GAME_VAR(CVector, vec0, 0x00BCC564);                // (a zero vector)
GAME_VAR(int32_t, g_BF4440, 0x00BF4440);            // (a model index)
GAME_VAR(uint8_t*, cameraSettings, 0x020C88B8);
// the tunables
GAME_VAR(float, g_A458F0, 0x00A458F0);   // 3: how fast the distance may change (times the rate)
GAME_VAR(float, g_A458F4, 0x00A458F4);   // 1: how far the sweep starts back from the camera
GAME_VAR(float, g_A458F8, 0x00A458F8);   // 0.1: the distance the target is fully faded at
GAME_VAR(float, g_A458FC, 0x00A458FC);   // 0.02: the rate per time step
GAME_ARRAY(ColPoint, colPoints, 0x00C216C0);

inline void* const transition = reinterpret_cast<void*>(0x00A45500);   // the camera transition
inline void* const theCamera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const sweepFilter = reinterpret_cast<void*>(0x0045EE90);
inline const char* const muzzleName = reinterpret_cast<const char*>(0x009146A8);    // "Muzzle"
inline const char* const fxNode1Name = reinterpret_cast<const char*>(0x009146A0);   // "FXNode1"

constexpr double kPi = 3.1415927410125732;       // (float pi, as a double)
constexpr double kHalfPi = 1.5707963705062866;   // (float pi / 2, as a double)
constexpr double kRadToDeg = 57.2957763671875;

// the camera's node
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(NiTransform*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(void, __cdecl, Node_Translate, 0x006CA180, void* node, const CVector* by, int mode);
GAME_FN(void, __cdecl, Node_Rotate, 0x006CA1E0, void* node, const CVector* axis, float degrees, int mode);
GAME_FN(void, __cdecl, Camera_SetNearClip, 0x006C8820, void* camera, float plane);
GAME_FN(CVector*, __thiscall, Camera_GetViewDirection, 0x0041AA70, void* camera, CVector* out);
// maths
GAME_FN(void, __thiscall, Matrix_FromTransform, 0x00412A70, Matrix* m, const void* transform);
GAME_FN(void, __thiscall, Matrix_SetRotateXOnly, 0x00412B60, Matrix* m, float angle);
GAME_FN(void, __thiscall, Matrix_SetRotateZOnly, 0x00412C20, Matrix* m, float angle);
// (0x004146B0 stores 16 bytes through `out`: called through re::CallVec16)
static inline void Matrix_TransformVector(CVector* out, const Matrix* m, const CVector* v) { re::CallVec16<0x004146B0, void>(out, m, v); }
// (0x0045D500 stores 16 bytes through `out`: called through re::CallVec16)
static inline void MultiplyMatrixVector4(float* out, const float* m, const CVector* v) { re::CallVec16<0x0045D500, void>(out, m, v); }
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const CVector* v);
GAME_FN(float, __cdecl, LimitAngle, 0x00432EE0, float angle);
GAME_FN(float, __cdecl, AngleDiff, 0x00432980, float a, float b);
GAME_FN(float, __cdecl, GetHeading, 0x00433310, const CVector* v);
GAME_FN(float, __cdecl, VecPitch, 0x00433490, const CVector* v);
// the world
GAME_FN(void, __thiscall, Sweep_Construct, 0x00563060, void* self, const CVector* from, const CVector* to, float radius, int flags, void* hit, void* filter, int a, int b, uintptr_t c);
GAME_FN(int32_t, __thiscall, Sweep_Test, 0x00564080, void* self);
GAME_FN(void, __thiscall, EmptyThiscall, 0x0049A650, void* self);   // (the sweep's destructor, the name's)
GAME_FN(uint8_t, __cdecl, ProcessLineOfSight, 0x00461EA0, const CVector* start, const CVector* end, ColPoint* colPoint, re::Ptr32<void>* entity, int flags, int a, int b);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, re::Ptr32<void>* ref);
GAME_FN(void, __thiscall, ColPoint_Destruct, 0x00415500, ColPoint* colPoint);
GAME_FN(float*, __thiscall, Placeable_GetMatrix, 0x0046DD20, void* placeable);
GAME_FN(uint8_t, __cdecl, GetNodeTransform, 0x00467D80, void* entity, int node, NiTransform* transform, int flag);
GAME_FN(void, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* self, const char* name);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(uint8_t*, __cdecl, UsePoint_GetEntity, 0x004EABB0, void* object);
// the camera's others
GAME_FN(int32_t, __thiscall, Transition_IsActive, 0x00856850, void* transition);
GAME_FN(float, __thiscall, Transition_GetWeight, 0x004F0AC0, void* transition, int controller);
GAME_FN(void, __thiscall, LookLimits_Get, 0x004F41A0, LookLimits* self, float* pitchMin, float* pitchMax, float* headingMin, float* headingMax, CVector* direction);
GAME_FN(void, __thiscall, LookLimits_Set_x64, 0x00504E90, LookLimits* self, float pitchMin, float pitchMax, float headingMin, float headingMax, uint32_t, uint32_t, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void LookLimits_Set(LookLimits* a0, float a1, float a2, float a3, float a4, CVector a5) {
    return LookLimits_Set_x64(a0, a1, a2, a3, a4, re::BitsOf(a5.x), re::BitsOf(a5.y), re::BitsOf(a5.z));
}
GAME_FN(CVector*, __thiscall, FirstPerson_GetEyePosition, 0x004FA890, void* self, CVector* out);
GAME_FN(void, __thiscall, FirstPerson_GetViewDirection, 0x004FA7C0, void* self, CVector* out);
GAME_FN(void, __cdecl, Clear_C3B708, 0x004DB5F0);
GAME_FN(void, __cdecl, Reticle_SetCurrentCrosshair, 0x004DB290, int index);
GAME_FN(void, __cdecl, Reticle_ShowCrosshair, 0x004DB660, float size, int r, int g, int b, int a);
// the target
GAME_FN(uint8_t*, __thiscall, Ped_GetTarget, 0x00497100, void* ped);
GAME_FN(void*, __thiscall, Targeting_GetLookTarget, 0x004D7780, void* targeting);
GAME_FN(CVector*, __thiscall, Targeting_GetTargetPos, 0x004D6000, void* targeting, CVector* out, void* entity, int flag);
GAME_FN(void, __thiscall, Ped_SetHeading, 0x00475730, void* ped, float heading);
GAME_FN(void, __thiscall, Ped_SetAlpha, 0x004732A0, void* ped, int alpha, int b);
// the pad
GAME_FN(void*, __cdecl, GetPlayerPadInput, 0x00738070);
GAME_FN(uint8_t, __cdecl, Pad_Fn738500, 0x00738500, void* pad);
// OffsetCameraController (through the game's addresses, so modes apply)
GAME_FN(void, __thiscall, OffsetCam_AddPitch, 0x00505300, void* self, float by);
GAME_FN(void, __thiscall, OffsetCam_ApplyPitch, 0x00505370, void* self);
GAME_FN(void, __thiscall, OffsetCam_MoveTo, 0x00505550, void* self, const CVector* position);
GAME_FN(void, __thiscall, OffsetCam_SetHeading, 0x00505600, void* self, float heading);
GAME_FN(void, __thiscall, OffsetCam_Collide, 0x00505750, void* self, CVector* position);
GAME_FN(void, __thiscall, OffsetCam_SetNearClip, 0x00505E60, void* self);
GAME_FN(uint8_t, __thiscall, OffsetCam_IsViewClear, 0x00506080, void* self);
GAME_FN(void, __thiscall, OffsetCam_AimAt, 0x005065B0, void* self, const CVector* point);
GAME_FN(void, __thiscall, OffsetCam_AimFromFirstPerson, 0x005067E0, void* self);
GAME_FN(void, __thiscall, OffsetCam_AimAtTarget, 0x00506990, void* self);
GAME_FN(void, __thiscall, OffsetCam_ApplyHeading, 0x00506A10, void* self);

using re::At;

template <typename R, typename... A>
R VCall(void* object, int index, A... args) {
    auto fn = reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(object))[index]));
    return fn(object, args...);
}

uint8_t* Butes(void* self) {
    return At<uint8_t*>(self, 0x110);
}

// (the game: the float sum, its fabs, then the CRT's sqrt)
float Root(double sumOfSquares) {
    float sum = float(sumOfSquares);
    return float(std::sqrt(double(std::fabs(sum))));
}

float Length(const CVector& v) {
    return Root((double(v.x) * v.x + double(v.y) * v.y) + double(v.z) * v.z);
}

// where an entity is (its matrix's position, or its own without a matrix)
const CVector* EntityPosition(uint8_t* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<const CVector*>(matrix + 0x30) : reinterpret_cast<const CVector*>(entity + 4);
}

// a colpoint as the game's constructor leaves it
void InitColPoint(ColPoint* cp) {
    cp->point = {0.0f, 0.0f, 0.0f};
    cp->entity = nullptr;
    cp->normal = {0.0f, 0.0f, 0.0f};
    cp->depth = FLT_MAX;
}

// the colpoint's entity let go (the game compares it with ms_pAlwaysValidEntity
// first; both ways do this)
void ReleaseColPoint(ColPoint* cp) {
    if (cp->entity) {
        CleanUpOldReference(cp->entity, &cp->entity);
    }
}

// the camera's target (and what it's in) marked as scanned, so the sweep skips them
void MarkTargetScanned() {
    At<uint16_t>(cameraTarget, 0x10A) = uint16_t(currentScanCode + 1);
    uint8_t* target = cameraTarget;
    if ((At<uint8_t>(target, 0x108) & 7) == 3 && At<uint8_t*>(target, 0x1554)) {
        At<uint16_t>(At<uint8_t*>(target, 0x1554), 0x10A) = uint16_t(currentScanCode + 1);
    }
}

// a pitch (radians from straight up) to the share of the butes' pitch range (0..1),
// set as both the pitch and the wanted pitch
float PitchShare(uint8_t* self, float fromUp) {
    uint8_t* butes = Butes(self);
    float low = float(double(At<float>(butes, 0x20)) * kPi / 180.0);
    float high = float(double(At<float>(butes, 0x24)) * kPi / 180.0);
    float share = float(1.0 - (double(fromUp) - low) / (double(high) - low));
    if (share < 0.0f) {
        return 0.0f;
    }
    if (share > 1.0f) {
        return 1.0f;
    }
    return share;
}

// ---- 0x505300: the pitch (0..1) moved by `by` (the target's +0x157C follows it with
// the butes' +0x70)
void __fastcall OffsetCam_AddPitchOurs(uint8_t* self, re::Edx, float by) {
    float pitch = by + At<float>(self, 0x40);
    At<float>(self, 0x40) = pitch;
    if (pitch < 0.0f) {
        At<float>(self, 0x40) = 0.0f;
    } else if (pitch > 1.0f) {
        At<float>(self, 0x40) = 1.0f;
    }
    float now = At<float>(self, 0x40);
    if (At<uint8_t>(Butes(self), 0x70)) {
        At<float>(cameraTarget, 0x157C) = now;
        now = At<float>(self, 0x40);
    }
    At<float>(self, 0x130) = now;
}
OFFSETCAM_REVERSED("OffsetCameraController::AddPitch_505300", 0x00505300, RE_BODY(&OffsetCam_AddPitchOurs), nullptr, nullptr, re::Mode::Original);

// 0x505370: the pitch eased to the wanted one and the camera's node turned to it
// (about its own right axis), within the butes' range or the look limits' when those
// are on (and no transition runs)
void __fastcall OffsetCam_ApplyPitchOurs(uint8_t* self, re::Edx) {
    At<float>(self, 0x3C) = float((double(At<float>(self, 0x40)) - At<float>(self, 0x3C)) * At<float>(Butes(self), 0x1C) +
                                  At<float>(self, 0x3C));
    void* node = GetCameraNode(theNiCamera);
    NiTransform* t = GetLocalTransform(node);
    float up = t->rotate[8];
    CVector position = t->translate;
    At<float>(self, 0x24) = !(up <= 1.0f) ? 0.0f : float(std::acos(double(up)));
    float current = float(double(At<float>(self, 0x24)) * kRadToDeg);
    float low, high;
    if (lookLimits.enabled && Transition_IsActive(transition) == 0) {
        low = float(90.0 - double(lookLimits.pitchMin) * 180.0 / kPi);
        high = float(90.0 - double(lookLimits.pitchMax) * 180.0 / kPi);
    } else {
        low = float(180.0 - At<float>(Butes(self), 0x20));
        high = float(180.0 - At<float>(Butes(self), 0x24));
    }
    float wanted = float(double(At<float>(self, 0x3C)) * (double(high) - low) + low);
    if (wanted > low) {
        wanted = low;
        At<float>(self, 0x3C) = 0.0f;
        At<float>(self, 0x40) = 0.0f;
    } else if (wanted < high) {
        wanted = high;
        At<float>(self, 0x3C) = 1.0f;
        At<float>(self, 0x40) = 1.0f;
    }
    CVector back{-position.x, -position.y, -position.z};
    Node_Translate(node, &back, 2);
    CVector axis{t->rotate[0], t->rotate[3], t->rotate[6]};
    Node_Rotate(node, &axis, float(double(wanted) - current), 2);
    Node_Translate(node, &position, 2);
    At<float>(self, 0x24) = float(double(wanted) * kPi / 180.0);
}
OFFSETCAM_REVERSED("OffsetCameraController::ApplyPitch_505370", 0x00505370, RE_BODY(&OffsetCam_ApplyPitchOurs), nullptr, nullptr, re::Mode::Original);

// 0x505550: the camera's node moved to `position` (Ghidra puts it with the base
// CameraController)
void __fastcall OffsetCam_MoveToOurs(void*, re::Edx, const CVector* position) {
    void* node = GetCameraNode(theNiCamera);
    NiTransform* t = GetLocalTransform(node);
    CVector by{position->x - t->translate.x, position->y - t->translate.y, position->z - t->translate.z};
    Node_Translate(node, &by, 2);
}
OFFSETCAM_REVERSED("CameraController::MoveTo_505550", 0x00505550, RE_BODY(&OffsetCam_MoveToOurs), nullptr, nullptr, re::Mode::Original);

// 0x505600: the heading (+4) from the camera's node, then the node turned about the
// vertical to `heading` (when it's more than 0.0007 off)
void __fastcall OffsetCam_SetHeadingOurs(uint8_t* self, re::Edx, float heading) {
    void* node = GetCameraNode(theNiCamera);
    NiTransform* t = GetLocalTransform(node);
    Matrix m;
    Matrix_FromTransform(&m, t);
    float angle = float(std::atan2(double(m.m[8]), double(m.m[9])));
    float current = LimitAngle(float(0.0 - angle));
    At<float>(self, 4) = current;
    float diff = AngleDiff(current, heading);
    float off = std::fabs(float(double(diff) - 0.0));
    if (!(off <= 0.0007f)) {
        CVector position = t->translate;
        CVector back{-position.x, -position.y, -position.z};
        Node_Translate(node, &back, 2);
        CVector axis{0.0f, 0.0f, 1.0f};
        Node_Rotate(node, &axis, float(double(diff) * 180.0 / kPi), 2);
        Node_Translate(node, &position, 2);
        At<float>(self, 4) = heading;
    }
}
OFFSETCAM_REVERSED("OffsetCameraController::SetHeading_505600", 0x00505600, RE_BODY(&OffsetCam_SetHeadingOurs), nullptr, nullptr, re::Mode::Original);

// one sweep from `start` to `end`: how far along it gets (the whole way without a hit)
float SweepDistance(void* sweep, const CVector& start, const CVector& end, float radius) {
    g_C1AE8A = 1;
    Sweep_Construct(sweep, &start, &end, radius, 0x8F, colPoints, sweepFilter, 0, 0, 0);
    MarkTargetScanned();
    ignoreEntity = nullptr;
    CVector along;
    if (Sweep_Test(sweep) != 0) {
        float hit = At<float>(sweep, 0x3C);
        VectorSub(&along, &end, &start);
        CVector part{hit * along.x, along.y * hit, along.z * hit};
        CVector at;
        VectorAdd(&at, &start, &part);
        VectorSub(&along, &at, &start);
    } else {
        VectorSub(&along, &end, &start);
    }
    return Length(along);
}

// 0x505750: the camera's place: the offset turned by the pitch and the heading from
// the pivot above `position`, then a sweep from the pivot (the butes' +0x7A: from
// beyond the camera, back, and again the other way) for how far it can go; the
// distance eased to that (by at most 3 * 0.02 per time step) and the camera put that
// far along from the sweep's start
void __fastcall OffsetCam_CollideOurs(uint8_t* self, re::Edx, CVector* position) {
    Matrix m;
    m.attached = nullptr;
    Matrix_SetRotateXOnly(&m, float(kHalfPi - At<float>(self, 0x24)));
    CVector offset;
    VectorSub(&offset, &At<CVector>(self, 0xC), &At<CVector>(self, 0x18));
    CVector v = offset;
    struct { CVector v; float pad; } turnedPad16;   // (the game's transform writes a 4th float)
    CVector& turned = turnedPad16.v;
    Matrix_TransformVector(&turned, &m, &v);
    Matrix_SetRotateZOnly(&m, At<float>(self, 4));
    v = turned;
    Matrix_TransformVector(&turned, &m, &v);
    CVector arm = turned;
    CVector start;
    VectorAdd(&start, position, &At<CVector>(self, 0x18));
    CVector pivot;
    CVector end;
    VectorAdd(&end, VectorAdd(&pivot, position, &At<CVector>(self, 0x18)), &arm);
    float radius = 0.3f;
    if (At<uint8_t>(Butes(self), 0x7A)) {
        CVector dir = arm;
        Vector_Normalise(&dir);
        float back = g_A458F4;
        CVector by{dir.x * back, dir.y * back, back * dir.z};
        CVector from;
        start = *VectorSub(&from, &end, &by);
        radius = 0.2f;
    }
    alignas(4) uint8_t sweep1[0x48];   // (+0x3C how far along the hit is)
    float distance = SweepDistance(sweep1, start, end, radius);
    g_C1AE8A = 0;
    if (At<uint8_t>(Butes(self), 0x7A)) {
        CVector dir = arm;
        Vector_Normalise(&dir);
        float back = g_A458F4;
        CVector by{dir.x * back, dir.y * back, back * dir.z};
        CVector from;
        start = *VectorAdd(&from, &end, &by);
        alignas(4) uint8_t sweep2[0x48];
        float distance2 = SweepDistance(sweep2, start, end, radius);
        if (double(distance2) - 0.20000000298023224 > double(distance)) {
            distance = float(double(Length(arm)) + distance2);
        }
        g_C1AE8A = 0;
        EmptyThiscall(sweep2);
    }
    float change = float(double(distance) - At<float>(self, 0x168));
    float rate = g_A458FC * timeStep;
    float most = g_A458F0;
    float low = float(double(-most) * rate);
    if (!(low < change)) {
        change = low;
    }
    float high = float(double(rate) * most);
    if (!(high > change)) {
        change = high;
    }
    At<float>(self, 0x168) = At<float>(self, 0x168) + change;
    CVector dir;
    VectorSub(&dir, &end, &start);
    Vector_Normalise(&dir);
    float along = At<float>(self, 0x168);
    CVector by{along * dir.x, dir.y * along, along * dir.z};
    CVector out;
    *position = *VectorAdd(&out, &start, &by);
    EmptyThiscall(sweep1);
}
OFFSETCAM_REVERSED("OffsetCameraController::Collide_505750", 0x00505750, RE_BODY(&OffsetCam_CollideOurs), nullptr, nullptr, re::Mode::Original);

// 0x505E60: the near clip plane to 0.05. (The game also works out a point 2 along the
// node's third column from the camera and never uses it, and lets go of a colpoint's
// entity that is never set)
void __fastcall OffsetCam_SetNearClipOurs(void*, re::Edx) {
    Camera_SetNearClip(theNiCamera, 0.05f);
}
OFFSETCAM_REVERSED("OffsetCameraController::SetNearClip_505E60", 0x00505E60, RE_BODY(&OffsetCam_SetNearClipOurs), nullptr, nullptr, re::Mode::Original);

// 0x505F70: a turn to face the other way starts: the base heading (+0x128) flipped
// round, the turn's ends and start time kept, and the look limits (when on) saved
// with their direction reversed, and turned off
void __fastcall OffsetCam_StartTurnOurs(uint8_t* self, re::Edx) {
    float before = AngleDiff(At<float>(self, 0x128), At<float>(self, 4));
    float base = At<float>(self, 0x128);
    if (kPi > double(base)) {
        At<float>(self, 0x128) = float(kPi + base);
    } else {
        At<float>(self, 0x128) = float(double(base) - kPi);
    }
    At<uint8_t>(self, 0x134) = 1;
    At<float>(self, 0x13C) = At<float>(self, 4);
    At<float>(self, 0x138) = At<float>(self, 0x128) - before;
    At<uint32_t>(self, 0x140) = timeInMs;
    At<float>(self, 0x144) = At<float>(self, 0x12C);
    At<float>(self, 0x148) = float(1.0 - At<float>(self, 0x12C));
    if (lookLimits.enabled) {
        LookLimits_Get(&lookLimits, &At<float>(self, 0x14C), &At<float>(self, 0x150), &At<float>(self, 0x154),
                       &At<float>(self, 0x158), &At<CVector>(self, 0x15C));
        At<float>(self, 0x15C) = -At<float>(self, 0x15C);
        At<float>(self, 0x160) = -At<float>(self, 0x160);
        lookLimits.enabled = 0;
        if (lookLimits.entity) {
            CleanUpOldReference(lookLimits.entity, &lookLimits.entity);
            lookLimits.entity = nullptr;
        }
    }
}
OFFSETCAM_REVERSED("OffsetCameraController::StartTurn_505F70", 0x00505F70, RE_BODY(&OffsetCam_StartTurnOurs), nullptr, nullptr, re::Mode::Original);

// 0x506080: whether the view is clear to what's aimed at: from the weapon's muzzle
// (or its placing's up, or with none, the entity's "FXNode1" node when the entity
// is model 0xBF4440's) a line to the locked-on target (clear when nothing's in the way
// or it's the target), or else one along the view (as far as the weapon's range; 100
// from the node) and then from the muzzle to where that stopped: clear when it
// reaches a ped, or something whose vtable[31] says so and whose vtable[32] is under
// 30000
uint8_t __fastcall OffsetCam_IsViewClearOurs(uint8_t* self, re::Edx) {
    uint8_t* target = cameraTarget;
    CVector start{0.0f, 0.0f, 0.0f};
    CVector end{0.0f, 0.0f, 0.0f};
    ColPoint cp;
    InitColPoint(&cp);
    bool fromNode = false;
    uint8_t* weapon = At<uint8_t*>(target, 0x1D0);
    CVector from;
    uint32_t key;
    NiTransform t;   // (from the node; uninitialised when that fails, apart from the scale)
    if (weapon && At<int16_t>(weapon, 0x10E) != -1) {
        NameKey_Construct(&key, muzzleName);
        t.scale = 1.0f;
        if (GetNodeTransform(weapon, int32_t(key), &t, 0)) {
            from = t.translate;
        } else {
            CVector up{0.0f, 0.0f, 1.0f};
            float out[4];
            MultiplyMatrixVector4(out, Placeable_GetMatrix(target), &up);
            from = {out[0], out[1], out[2]};
        }
        ignoreEntity = target;
    } else {
        uint8_t* entity = At<uint8_t*>(self, 0x170);
        if (!entity || int32_t(At<int16_t>(entity, 0x10E)) != g_BF4440) {
            return 0;
        }
        fromNode = true;
        NameKey_Construct(&key, fxNode1Name);
        t.scale = 1.0f;
        GetNodeTransform(At<void*>(self, 0x170), int32_t(key), &t, 0);
        from = t.translate;
        ignoreEntity = At<void*>(self, 0x170);
    }
    EmptyThiscall(&key);
    re::Ptr32<void> hit = nullptr;
    void* lockOn = Targeting_GetLookTarget(At<uint8_t*>(target, 0x1C8));
    if (lockOn) {
        start = from;
        CVector at;
        end = *Targeting_GetTargetPos(At<uint8_t*>(target, 0x1C8), &at, lockOn, 0);
        uint8_t clear = ProcessLineOfSight(&start, &end, &cp, &hit, 0x18B, 1, 0) ? uint8_t(hit == lockOn) : uint8_t(1);
        ReleaseColPoint(&cp);
        return clear;
    }
    start = GetLocalTransform(GetCameraNode(theNiCamera))->translate;
    float range;
    if (!fromNode) {
        float weaponRange = At<float>(weapon, 0x120);
        float share = At<float>(weapon, 0x128);
        float clamped = share < 0.0f ? 0.0f : (share > 1.0f ? 1.0f : share);
        double half = double(weaponRange) * 0.5;
        range = float(double(clamped) * half + half);
        if (0.01f > range) {
            range = 0.01f;
        }
    } else {
        range = 100.0f;
    }
    CVector view;
    const CVector* dir = Camera_GetViewDirection(theCamera, &view);
    CVector ahead{dir->x * range, dir->y * range, range * dir->z};
    CVector sum;
    end = *VectorAdd(&sum, &start, &ahead);
    if (ProcessLineOfSight(&start, &end, &cp, &hit, 0x10F, 1, 1)) {
        end = cp.point;
    }
    re::Ptr32<void> hit2 = nullptr;
    start = from;
    uint8_t* entity;
    if (ProcessLineOfSight(&start, &end, &cp, &hit2, 0x10F, 1, 1)) {
        entity = static_cast<uint8_t*>(hit2);
    } else {
        entity = static_cast<uint8_t*>(hit);
    }
    if (entity) {
        bool flagC0 = At<int32_t>(entity, 0xC0) != 0;
        uint8_t type = At<uint8_t>(entity, 0x108) & 7;
        bool pedFlag = type == 3 && At<uint8_t>(entity, 0x1C98) != 0;
        if (!(pedFlag || flagC0) || At<int32_t>(entity, 0xF8) == 0) {
            if (type == 3) {
                ReleaseColPoint(&cp);
                return 1;
            }
            if (VCall<uint8_t>(entity, 31) && VCall<float>(entity, 32) < 30000.0) {
                ColPoint_Destruct(&cp);
                return 1;
            }
        }
    }
    ReleaseColPoint(&cp);
    return 0;
}
OFFSETCAM_REVERSED("OffsetCameraController::IsViewClear_506080", 0x00506080, RE_BODY(&OffsetCam_IsViewClearOurs), nullptr, nullptr, re::Mode::Original);

// 0x5065B0: turned to look at `point` from the pivot above the target (the heading
// corrected for the offset's sideways part, the pitch for its height)
void __fastcall OffsetCam_AimAtOurs(uint8_t* self, re::Edx, const CVector* point) {
    CVector pivot;
    VectorAdd(&pivot, EntityPosition(cameraTarget), &At<CVector>(self, 0x18));
    CVector toPoint;
    VectorSub(&toPoint, point, &pivot);
    double flatSum = double(toPoint.x) * toPoint.x + double(toPoint.y) * toPoint.y;
    float flat = std::fabs(float(flatSum));
    float distance = Root(flatSum + double(toPoint.z) * toPoint.z);
    CVector offset;
    VectorSub(&offset, &At<CVector>(self, 0xC), &At<CVector>(self, 0x18));
    float heading = GetHeading(&toPoint);
    float sideways = float(double(offset.x) / float(std::sqrt(double(flat))));
    float yaw = heading;
    if (sideways >= -1.0f && sideways <= 1.0f) {
        yaw = float(double(float(std::asin(double(sideways)))) + heading);
    }
    float up = float(double(offset.z) / distance);
    if (up < -1.0f) {
        up = -1.0f;
    } else if (up > 1.0f) {
        up = 1.0f;
    }
    float pitch = VecPitch(&toPoint);
    float correction = float(std::asin(double(up)));
    float fromUp = float(kHalfPi - float(double(pitch) - correction));
    OffsetCam_SetHeading(self, yaw);
    float share = PitchShare(self, fromUp);
    At<uint8_t>(self, 8) = 0;
    At<float>(self, 0x3C) = share;
    At<float>(self, 0x40) = share;
}
OFFSETCAM_REVERSED("OffsetCameraController::AimAt_5065B0", 0x005065B0, RE_BODY(&OffsetCam_AimAtOurs), nullptr, nullptr, re::Mode::Original);

// 0x5067E0: turned to where the first-person camera looks (at what's 50 along its
// view when something is there, otherwise its direction)
void __fastcall OffsetCam_AimFromFirstPersonOurs(uint8_t* self, re::Edx) {
    void* firstPerson = firstPersonController;
    CVector eye;
    FirstPerson_GetEyePosition(firstPerson, &eye);
    CVector dir;
    FirstPerson_GetViewDirection(firstPerson, &dir);
    CVector ahead{float(dir.x * 50.0), float(dir.y * 50.0), float(50.0 * dir.z)};
    CVector end;
    VectorAdd(&end, &eye, &ahead);
    ColPoint cp;
    InitColPoint(&cp);
    re::Ptr32<void> hit;
    if (ProcessLineOfSight(&eye, &end, &cp, &hit, 0x10F, 1, 1)) {
        OffsetCam_AimAt(self, &cp.point);
    } else {
        float fromUp = float(kHalfPi - VecPitch(&dir));
        float share = PitchShare(self, fromUp);
        At<float>(self, 0x3C) = share;
        At<float>(self, 0x40) = share;
        OffsetCam_SetHeading(self, GetHeading(&dir));
    }
    ReleaseColPoint(&cp);
}
OFFSETCAM_REVERSED("OffsetCameraController::AimFromFirstPerson_5067E0", 0x005067E0, RE_BODY(&OffsetCam_AimFromFirstPersonOurs), nullptr, nullptr, re::Mode::Original);

// 0x506990: turned to the target's target (a ped: its +0x114 -> +0x14 -> +8, +0x2C8)
void __fastcall OffsetCam_AimAtTargetOurs(uint8_t* self, re::Edx) {
    uint8_t* aimed = Ped_GetTarget(cameraTarget);
    const CVector* at;
    if ((At<uint8_t>(aimed, 0x108) & 7) == 3) {
        at = &At<CVector>(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(aimed, 0x114), 0x14), 8), 0x2C8);
    } else {
        at = EntityPosition(aimed);
    }
    CVector point = *at;
    OffsetCam_AimAt(self, &point);
}
OFFSETCAM_REVERSED("OffsetCameraController::AimAtTarget_506990", 0x00506990, RE_BODY(&OffsetCam_AimAtTargetOurs), nullptr, nullptr, re::Mode::Original);

// 0x506A10: the heading offset eased to the look input's (snapped when the input is
// back at 0 and it's within 0.1), and the heading set from it, kept within the look
// limits' heading range around +0x128 when those are on (at a limit with the input
// pushing on, to the other one)
void __fastcall OffsetCam_ApplyHeadingOurs(uint8_t* self, re::Edx) {
    if (0.0f == At<float>(self, 0x38)) {
        float off = std::fabs(float(At<float>(self, 0x38) - At<float>(self, 0x34)));
        if (off < 0.10000000149011612) {
            At<float>(self, 0x34) = At<float>(self, 0x38);
        }
    }
    At<float>(self, 0x34) = float((double(At<float>(self, 0x38)) - At<float>(self, 0x34)) * At<float>(Butes(self), 0x18) +
                                  At<float>(self, 0x34));
    float heading = float(double(At<float>(self, 0x34)) * kPi / 180.0 + At<float>(self, 4));
    if (lookLimits.enabled && Transition_IsActive(transition) == 0) {
        float wanted = AngleDiff(At<float>(self, 0x128), heading);
        float now = AngleDiff(At<float>(self, 0x128), At<float>(self, 4));
        float low = lookLimits.headingMin;
        float high = lookLimits.headingMax;
        if (low > wanted) {
            float atOther = std::fabs(float(now - high));
            if (atOther < 0.001f && 0.0f < lookX) {
                heading = high + At<float>(self, 0x128);
            } else {
                heading = low + At<float>(self, 0x128);
            }
        } else if (high < wanted) {
            float atOther = std::fabs(float(now - low));
            if (atOther < 0.001f && 0.0f > lookX) {
                heading = low + At<float>(self, 0x128);
            } else {
                heading = high + At<float>(self, 0x128);
            }
        }
    }
    OffsetCam_SetHeading(self, heading);
    At<float>(self, 0x38) = 0.0f;
}
OFFSETCAM_REVERSED("OffsetCameraController::ApplyHeading_506A10", 0x00506A10, RE_BODY(&OffsetCam_ApplyHeadingOurs), nullptr, nullptr, re::Mode::Original);

// 0x506BB0 (vtable[7]): the frame: the raise eased (up while the target's +0x131C),
// the offset and pivot from the butes, the look input's tuning, then either on an
// entity's node (the butes' +0x60: the camera placed and turned by the node) or
// turned by the look input around the target (a turn running, the heading and pitch
// eased, the collision), the target faded when the camera is close, the fov, the
// crosshair by whether the view is clear
void __fastcall OffsetCam_ProcessOurs(uint8_t* self, re::Edx) {
    uint8_t* target = cameraTarget;
    if (!target) {
        return;
    }
    uint8_t* matrix = At<uint8_t*>(target, 0x14);
    const CVector* position = EntityPosition(target);
    float targetHeading;
    if (matrix) {
        targetHeading = float(std::atan2(-double(At<float>(matrix, 0x10)), double(At<float>(matrix, 0x14))));
    } else {
        targetHeading = At<float>(target, 0x10);
    }
    uint8_t* butes = Butes(self);
    Matrix node;
    node.attached = nullptr;
    float rate = g_A458FC * timeStep;
    if (At<int32_t>(target, 0x131C) != 0) {
        float raise = float(double(rate) * At<float>(butes, 0x44) + At<float>(self, 0x120));
        At<float>(self, 0x120) = raise;
        if (At<float>(butes, 0x40) > raise) {
            At<float>(self, 0x120) = At<float>(butes, 0x40);
        }
    } else {
        float raise = float(double(At<float>(self, 0x120)) - double(rate) * At<float>(butes, 0x44));
        At<float>(self, 0x120) = raise;
        if (raise > 0.0f) {
            At<float>(self, 0x120) = 0.0f;
        }
    }
    float off = std::fabs(float(At<float>(self, 0x124) - At<float>(self, 0x120)));
    if (off < 0.01f) {
        At<float>(self, 0x124) = At<float>(self, 0x120);
    } else {
        At<float>(self, 0x124) = float((double(At<float>(self, 0x120)) - At<float>(self, 0x124)) * At<float>(butes, 0x48) +
                                       At<float>(self, 0x124));
    }
    butes = Butes(self);
    At<float>(self, 0xC) = At<float>(butes, 0x4C);
    At<float>(self, 0x10) = At<float>(butes, 0x50);
    At<float>(self, 0x14) = At<float>(butes, 0x54) + At<float>(self, 0x124);
    if (At<uint8_t>(butes, 0x60)) {
        At<CVector>(self, 0x18) = vec0;
    } else {
        At<float>(self, 0x18) = At<float>(butes, 0x34);
        At<float>(self, 0x1C) = At<float>(butes, 0x38);
        At<float>(self, 0x20) = At<float>(butes, 0x3C) + At<float>(self, 0x124);
    }
    if (!At<uint8_t>(self, 0x16C) && Transition_IsActive(transition) == 0) {
        butes = Butes(self);
        int32_t mode = At<uint8_t>(butes, 0x78) ? int32_t(At<uint8_t>(butes, 0x79) != 0) + 2 : 1;
        float scale = At<uint8_t>(self, 0x16D) ? At<float>(butes, 0x98) : 0.4f;
        lookMode = mode;
        if (Pad_Fn738500(GetPlayerPadInput())) {
            lookTuning98 = At<float>(Butes(self), 0x84);
            lookTuning9C = At<float>(Butes(self), 0x88);
            lookTuning94 = At<float>(Butes(self), 0x8C) * scale;
            lookTuningA0 = At<float>(Butes(self), 0x90);
            lookTuningA4 = At<float>(Butes(self), 0x94);
        } else {
            lookTuning98 = 1.0f;
            lookTuning9C = -0.0f;
            lookTuning94 = 1.5f;
            lookTuningA0 = -0.0f;
            lookTuningA4 = At<float>(Butes(self), 0x94);
        }
    }
    if (At<uint8_t>(Butes(self), 0x60)) {
        uint8_t* entity = UsePoint_GetEntity(VCall<void*>(cameraTarget, 16));
        if (!entity) {
            entity = At<uint8_t*>(self, 0x170);
            if (!entity) {
                return;
            }
        }
        NiTransform t;   // (uninitialised apart from the scale when the node isn't found)
        t.scale = 1.0f;
        At<uint8_t*>(self, 0x170) = entity;
        uint32_t key = HashName(At<const char*>(Butes(self), 0x64));
        if (!GetNodeTransform(entity, int32_t(key), &t, 0) && At<uint8_t>(self, 0x16C)) {
            return;
        }
        Matrix_FromTransform(&node, &t);
        void* cameraNode = GetCameraNode(theNiCamera);
        GetLocalTransform(cameraNode);
        butes = Butes(self);
        CVector* cameraPos = &At<CVector>(self, 0x28);
        *cameraPos = {node.m[12], node.m[13], node.m[14]};
        if (At<uint8_t>(butes, 0x68) || At<uint8_t>(self, 0x11C)) {
            CVector forward{node.m[4], node.m[5], node.m[6]};
            float flat = Root(double(forward.y) * forward.y + double(forward.x) * forward.x);
            float pitch = float(std::atan2(double(forward.z), double(flat)));
            float aboutX = float((kHalfPi - pitch) * 180.0 / kPi + At<float>(butes, 0x30));
            float yaw = float(std::atan2(double(forward.x), double(forward.y)));
            float aboutZ = float((kPi - yaw) * 180.0 / kPi + At<float>(butes, 0x2C));
            CVector axisX{1.0f, 0.0f, 0.0f};
            CVector axisZ{0.0f, 0.0f, 1.0f};
            Node_Rotate(cameraNode, &axisX, aboutX, 0);
            Node_Rotate(cameraNode, &axisZ, aboutZ, 2);
            Node_Translate(cameraNode, cameraPos, 2);
            if (At<uint8_t>(self, 8)) {
                float offset = At<float>(Butes(self), 0x2C);
                float heading = GetHeading(&forward);
                float base = AngleDiff(float(double(offset) * kPi / 180.0), heading);
                At<float>(self, 0x128) = base;
                OffsetCam_SetHeading(self, base);
                At<uint8_t>(self, 8) = 0;
                OffsetCam_Collide(self, cameraPos);
                float pitchShare = At<float>(Butes(self), 0x28);
                At<float>(self, 0x40) = pitchShare;
                At<float>(self, 0x3C) = pitchShare;
            }
        }
    }
    if (At<uint8_t>(self, 0x11D) && !At<uint8_t>(Butes(self), 0x60)) {
        if (cameraMode94 == 2) {
            OffsetCam_AimFromFirstPerson(self);
        } else if (cameraFlag284 && Ped_GetTarget(target)) {
            OffsetCam_AimAtTarget(self);
        } else {
            CVector arm;
            VectorSub(&arm, &At<CVector>(self, 0xC), &At<CVector>(self, 0x18));
            At<float>(self, 0x168) = Vector_Magnitude(&arm);
            butes = Butes(self);
            At<float>(self, 0x3C) = At<float>(butes, 0x28);
            At<float>(self, 0x40) = At<float>(butes, 0x28);
        }
    }
    butes = Butes(self);
    if (!At<uint8_t>(butes, 0x60) || (!At<uint8_t>(butes, 0x68) && !At<uint8_t>(self, 0x11C))) {
        if (At<uint8_t>(self, 8)) {
            float base = AngleDiff(float(double(At<float>(butes, 0x2C)) * kPi / 180.0), targetHeading);
            At<float>(self, 0x128) = base;
            OffsetCam_SetHeading(self, base);
            At<uint8_t>(self, 8) = 0;
        }
        if (!At<uint8_t>(self, 0x44)) {
            if (At<uint8_t>(self, 0x134)) {
                float t = float(double(timeInMs - At<uint32_t>(self, 0x140)) / 1000.0);
                if (1.0f < t) {
                    // the turn is done: its end, the look limits back
                    OffsetCam_SetHeading(self, At<float>(self, 0x138));
                    At<uint8_t>(self, 0x134) = 0;
                    LookLimits_Set(&lookLimits, At<float>(self, 0x14C), At<float>(self, 0x150), At<float>(self, 0x154),
                                   At<float>(self, 0x158), At<CVector>(self, 0x15C));
                    At<float>(self, 0x12C) = At<float>(self, 0x148);
                } else {
                    float turn = AngleDiff(At<float>(self, 0x13C), At<float>(self, 0x138));
                    At<float>(self, 0x12C) = float(double(t) * (double(At<float>(self, 0x148)) - At<float>(self, 0x144)) +
                                                   At<float>(self, 0x144));
                    OffsetCam_SetHeading(self, float(double(t) * turn + At<float>(self, 0x13C)));
                    At<float>(self, 0x38) = 0.0f;
                }
            } else {
                At<float>(self, 0x38) = lookX;
            }
            OffsetCam_AddPitch(self, lookY);
        } else {
            bool xLetGo = 0.10000000149011612 > double(std::fabs(lookX));
            bool yLetGo = std::fabs(lookY) < 0.10000000149011612;
            if (xLetGo && yLetGo) {
                At<uint8_t>(self, 0x44) = 0;
            }
        }
        OffsetCam_ApplyHeading(self);
        OffsetCam_ApplyPitch(self);
        OffsetCam_SetNearClip(self);
        if (At<uint8_t>(Butes(self), 0x60)) {
            OffsetCam_Collide(self, &At<CVector>(self, 0x28));
        }
        uint8_t* ped = cameraTarget;
        if (At<int32_t>(ped, 0x13DC) == 0 && At<int32_t>(ped, 0x548) == 0 && Transition_IsActive(transition) == 0) {
            Ped_SetHeading(cameraTarget, float(double(At<float>(Butes(self), 0x2C)) * kPi / 180.0 + At<float>(self, 4)));
        }
    }
    if (!At<uint8_t>(Butes(self), 0x60)) {
        // (the game also sets up a colpoint here that nothing uses)
        At<CVector>(self, 0x28) = *position;
        OffsetCam_Collide(self, &At<CVector>(self, 0x28));
        float weight = 1.0f;
        if (Transition_IsActive(transition) != 0) {
            weight = Transition_GetWeight(transition, 13);
        }
        float alpha = 255.0f;
        butes = Butes(self);
        if (At<float>(butes, 0x74) > At<float>(self, 0x168)) {
            double near = g_A458F8;
            alpha = float((double(At<float>(self, 0x168)) - near) / (double(At<float>(butes, 0x74)) - near) * 255.0);
            if (!(0.0f < alpha)) {
                alpha = 0.0f;
            }
            alpha = float(double(alpha) * weight + (1.0 - weight) * 255.0);
        }
        Ped_SetAlpha(cameraTarget, int32_t(alpha), 0);   // (_ftol: truncated)
    }
    if (!At<uint8_t>(self, 0x16C) && !cameraFovLocked) {
        cameraFovOverridden = 0;
        cameraFov = At<float>(Butes(self), 0x6C);
    }
    OffsetCam_MoveTo(self, &At<CVector>(self, 0x28));
    At<uint8_t>(self, 0x16D) = OffsetCam_IsViewClear(self);
    Clear_C3B708();
    if (Transition_IsActive(transition) == 0) {
        uint8_t* crosshair;
        int r, g, b, a;
        if (At<uint8_t>(self, 0x16D)) {
            crosshair = At<uint8_t*>(self, 0x114);
            if (crosshair) {
                EmptyThiscall(crosshair);
                Reticle_SetCurrentCrosshair(At<int32_t>(crosshair, 0x40));
                crosshair = At<uint8_t*>(self, 0x114);
                r = At<uint8_t>(crosshair, 0x30);
                g = At<uint8_t>(crosshair, 0x34);
                b = At<uint8_t>(crosshair, 0x38);
                a = At<uint8_t>(crosshair, 0x3C);
                Reticle_ShowCrosshair(At<float>(crosshair, 0x1C), r, g, b, a);
            }
        } else {
            crosshair = At<uint8_t*>(self, 0x118);
            if (crosshair) {
                EmptyThiscall(crosshair);
                Reticle_SetCurrentCrosshair(At<int32_t>(crosshair, 0x40));
                crosshair = At<uint8_t*>(self, 0x118);
                r = At<uint8_t>(crosshair, 0x20);
                g = At<uint8_t>(crosshair, 0x24);
                b = At<uint8_t>(crosshair, 0x28);
                a = At<uint8_t>(crosshair, 0x2C);
                Reticle_ShowCrosshair(At<float>(crosshair, 0x1C), r, g, b, a);
            }
        }
    }
    At<uint8_t>(self, 0x11C) = 0;
    At<uint8_t>(self, 0x11D) = 0;
    if (!At<uint8_t>(self, 0x134)) {
        float diff = AngleDiff(At<float>(self, 0x128), At<float>(self, 4));
        At<float>(self, 0x12C) = float((double(diff) + kHalfPi) / kPi);
    }
}
OFFSETCAM_REVERSED("OffsetCameraController::Process_506BB0", 0x00506BB0, RE_BODY(&OffsetCam_ProcessOurs), nullptr, nullptr, re::Mode::Original);

} // namespace
