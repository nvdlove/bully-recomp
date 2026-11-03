#include "game/Camera.h"
#include "game/Pool.h"

#include <cmath>
#include <cstddef>
#include "reversed.h"

// The follow camera's parts (0x4FE590..0x500650): FollowCameraBase's smoothing and
// the peds-in-the-way check, FollowCameraFight (at FollowCameraBase+0x160: the view
// while fighting: its attribute set by name, the target it frames, how far to pull
// back, where to put the camera), FollowCameraVehicle's blend (at +0x278) and a
// small timed fade. Thiscalls are __fastcall wrappers with the unused edx.
//
// FollowCameraFight, as used here: +4 the view (+4 position, +0x10 the second
// vector), +0x1C the point framed, +0x28 when there last was a target (CTimer ms),
// +0x2C framing a target, +0x30/+0x34 height offsets (+0x34 eased), +0x38 to the
// target, +0x44 its perpendicular, +0x50 its attribute set, +0x54 set by name,
// +0x58 its mode (0 outside, 2 inside; -1 none), +0x5C/+0x68 the positions without
// a target, +0x7C, +0x84/+0x90 where the camera goes and looks, +0x9C..+0xA8 the
// angle limits, +0xAC the centre, +0xB8/+0xC4 the side and up axes, +0xD0 the
// distance, +0xD4 the spot found by 0x500650 (+0xE0 the camera then, +0xF0 when,
// +0xF4 in use).

#define FOLLOWCAMBASE_REVERSED(...) REVERSED(__VA_ARGS__)

namespace {

using cam::CVector;

GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);   // CTimer's
GAME_VAR(int32_t, currArea, 0x00BD1008);    // CGame::currArea
GAME_VAR(CPool<uint8_t>*, pedPool, 0x00C0F5F0);
GAME_VAR(uint8_t*, cameraSettings, 0x020C88B8);
GAME_VAR(int32_t, g_BF40F4, 0x00BF40F4);
GAME_VAR(uint32_t, g_A45814, 0x00A45814);   // how long 0x500650's spot is kept (ms)
GAME_VAR(float, g_A45818, 0x00A45818);
GAME_VAR(float, g_A4576C, 0x00A4576C);      // 2 pi
GAME_VAR(float, g_A457E0, 0x00A457E0);
GAME_VAR(float, g_A457E4, 0x00A457E4);
GAME_VAR(float, g_C3D278, 0x00C3D278);
GAME_VAR(float, g_C3D27C, 0x00C3D27C);
// 0x4FF460's tuning
GAME_VAR(float, g_A457E8, 0x00A457E8);
GAME_VAR(float, g_A457EC, 0x00A457EC);
GAME_VAR(float, g_A457F0, 0x00A457F0);
GAME_VAR(float, g_A457F4, 0x00A457F4);
GAME_VAR(float, g_A457F8, 0x00A457F8);
GAME_VAR(float, g_A457FC, 0x00A457FC);
GAME_VAR(float, g_A45800, 0x00A45800);
GAME_VAR(float, g_A45804, 0x00A45804);
GAME_VAR(float, g_A45808, 0x00A45808);
GAME_VAR(float, g_A4580C, 0x00A4580C);
GAME_VAR(float, g_A45810, 0x00A45810);
GAME_VAR(float, g_A4581C, 0x00A4581C);
// 0x4FE8A0's static: cameraSettings +0x1C, read once
GAME_VAR(uint32_t, fightTargetHoldTime, 0x00C3D270);
GAME_VAR(uint32_t, fightTargetHoldTimeGuard, 0x00C3D274);
// 0x4FFF50's static: the up vector (0, 1, 0)
GAME_VAR(CVector, orbitAxis, 0x00C3D280);
GAME_VAR(uint32_t, orbitAxisGuard, 0x00C3D28C);
// the fight attribute sets' names (hashes) and root
inline const uint32_t* const butesName_1B653C8 = reinterpret_cast<const uint32_t*>(0x01B653C8);
inline const uint32_t* const butesName_1B65404 = reinterpret_cast<const uint32_t*>(0x01B65404);
inline const uint32_t* const butesName_1B65408 = reinterpret_cast<const uint32_t*>(0x01B65408);
inline const uint32_t* const butesName_1B6540C = reinterpret_cast<const uint32_t*>(0x01B6540C);
inline const uint32_t* const butesName_1B65410 = reinterpret_cast<const uint32_t*>(0x01B65410);
inline const uint32_t* const butesRoot_1B653C4 = reinterpret_cast<const uint32_t*>(0x01B653C4);

GAME_FN(uint8_t, __cdecl, IsAreaInterior, 0x00430320, int area);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, re::Ptr32<void>* ref);
GAME_FN(CVector*, __thiscall, Entity_GetForward, 0x004154A0, void* entity, CVector* out);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, CrossProduct, 0x00414730, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(float, __cdecl, VecHeading, 0x00433190, const CVector* v);
GAME_FN(float, __cdecl, VecPitch, 0x00433490, const CVector* v);
GAME_FN(float, __cdecl, AngleDiff, 0x00432980, float a, float b);
GAME_FN(float, __cdecl, LimitAngle, 0x00432930, float angle);
GAME_FN(float, __cdecl, Clamp, 0x0041A470, float value, float low, float high);
GAME_FN(CVector*, __cdecl, VectorFromAngles, 0x00412530, CVector* out, CVector axis, float heading, float pitch);
GAME_FN(void*, __thiscall, Unknown4D5AD0, 0x004D5AD0, void* self);
GAME_FN(float, __thiscall, Unknown4D96B0, 0x004D96B0, void* self);
GAME_FN(uint8_t, __thiscall, Ped_Unknown474BF0, 0x00474BF0, void* ped);
GAME_FN(uint8_t*, __thiscall, Ped_GetTarget, 0x00497100, void* ped);
GAME_FN(uint8_t*, __thiscall, Ped_Unknown46ECE0, 0x0046ECE0, void* ped);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int pad, int);
GAME_FN(uint8_t, __cdecl, World_TestLine, 0x00461EA0, const CVector* from, const CVector* to, void* colPoint, re::Ptr32<void>* entity, int flags, int a, int b);
// the butes (names are 4-byte hashes; a path is {count, names})
GAME_FN(void, __thiscall, Name_Copy, 0x005FA880, void* self, const void* from);
GAME_FN(void, __thiscall, Name_Construct, 0x005FA910, void* self, uintptr_t value);
GAME_FN(void, __thiscall, Path_Build, 0x005FA8C0, void* self, int count, const void* names);
GAME_FN(void*, __cdecl, Butes_Find, 0x0072AEA0, const void* path, const void* root);
GAME_FN(void, __stdcall, DestructArray, 0x004017C0, void* array, size_t size, int count, void* dtor);
GAME_FN(void, __thiscall, VehicleCameraController_Unknown50AE90, 0x0050AE90, void* self, float value);
GAME_FN(float, __stdcall, EaseToLimit, 0x004FDF20, float value, float limit, float start, float t);
GAME_FN(cam::NiTransform*, __thiscall, Camera_GetViewTransform, 0x004F2E30, CCamera* self);
GAME_FN(float, __thiscall, Camera_GetFov, 0x004F1390, CCamera* self);
GAME_FN(float, __cdecl, GetAspectRatio, 0x004F1360);
GAME_FN(CVector*, __thiscall, Matrix_Multiply, 0x00768A70, const void* matrix, CVector* out, const CVector* v);
// own functions, through the game's address
GAME_FN(void, __thiscall, FollowCameraFight_Reset, 0x004FE640, void* self);
GAME_FN(CVector*, __thiscall, FollowCameraFight_GetTarget, 0x004FE730, void* self, CVector* out);
GAME_FN(uint32_t, __thiscall, FollowCameraFight_CheckTarget, 0x004FE8A0, void* self, uint8_t* started);
GAME_FN(void, __thiscall, FollowCameraFight_UpdatePullBack, 0x004FE980, void* self);
GAME_FN(void, __thiscall, FollowCameraFight_Place, 0x004FEB90, void* self, CVector* camera, CVector* lookAt);
GAME_FN(void, __thiscall, FollowCameraFight_Unknown4FF460, 0x004FF460, void* self, int started);

constexpr double kPi = 3.1415927410125732;   // (float pi, as a double)

using re::At;

template <typename R, typename... A>
R VCall(void* object, int index, A... args) {
    auto fn = reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(object))[index]));
    return fn(object, args...);
}

uint8_t* TargetPed() {
    return static_cast<uint8_t*>(TheCamera.targetPed);
}
uint8_t* SecondTarget() {
    return At<uint8_t*>(&TheCamera, 0x1B0);
}
uint8_t* CloseEntity() {
    return At<uint8_t*>(&TheCamera, 0x1B4);
}

int EntityType(void* entity) {
    return At<uint8_t>(entity, 0x108) & 7;
}

const CVector* Position(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<const CVector*>(matrix + 0x30) : &At<CVector>(entity, 4);
}

uint8_t* Butes(void* fight) {
    return At<uint8_t*>(fight, 0x50);
}

float LengthSq(const CVector& v) {
    return float((double(v.x) * v.x + double(v.y) * v.y) + double(v.z) * v.z);
}

// the game's sqrt of |x| (x87 fsqrt through _CIsqrt), to a float
float SqrtAbs(float x) {
    return float(std::sqrt(double(std::fabs(x))));
}

// the ped's focus (+0x1C8)'s target, when it has one
void* FocusTarget(uint8_t* ped) {
    return Unknown4D5AD0(At<void*>(ped, 0x1C8));
}

} // namespace

namespace {

// 0x4FE590 (FollowCameraBase): +0x90 eased towards a point (by +0xA0 across, +0x9C
// up; at once when +0x115)
void __fastcall FollowCameraBase_EasePosition(uint8_t* self, re::Edx, float x, float y, float z) {
    float across;
    float up;
    if (At<uint8_t>(self, 0x115)) {
        across = 1.0f;
        up = 1.0f;
    } else {
        across = At<float>(self, 0xA0);
        up = At<float>(self, 0x9C);
    }
    At<float>(self, 0x90) = float((double(x) - At<float>(self, 0x90)) * across + At<float>(self, 0x90));
    At<float>(self, 0x94) = float((double(y) - At<float>(self, 0x94)) * across + At<float>(self, 0x94));
    At<float>(self, 0x98) = float((double(z) - At<float>(self, 0x98)) * up + At<float>(self, 0x98));
}
FOLLOWCAMBASE_REVERSED("FollowCameraBase::EasePosition4FE590", 0x004FE590, RE_BODY(&FollowCameraBase_EasePosition), nullptr, nullptr, re::Mode::Original);

// 0x4FE640: the attribute set for outside or inside, when that changed (not while
// one set by name is in use). (Eight names built, two used: pair 0 or 2.)
void __fastcall FollowCameraFight_ResetOurs(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 0x54)) {
        return;
    }
    int32_t mode = IsAreaInterior(currArea) ? 2 : 0;
    if (At<int32_t>(self, 0x58) == mode) {
        return;
    }
    At<int32_t>(self, 0x58) = mode;
    uint32_t names[8];
    Name_Copy(&names[0], butesName_1B653C8);
    Name_Copy(&names[1], butesName_1B65404);
    Name_Copy(&names[2], butesName_1B653C8);
    Name_Copy(&names[3], butesName_1B65408);
    Name_Copy(&names[4], butesName_1B653C8);
    Name_Copy(&names[5], butesName_1B6540C);
    Name_Copy(&names[6], butesName_1B653C8);
    Name_Copy(&names[7], butesName_1B65410);
    uint32_t path[2];
    Path_Build(path, 2, &names[mode * 2]);
    At<void*>(self, 0x50) = Butes_Find(path, butesRoot_1B653C4);
    DestructArray(names, 4, 8, reinterpret_cast<void*>(0x0049A650));
}
FOLLOWCAMBASE_REVERSED("FollowCameraFight::Reset", 0x004FE640, RE_BODY(&FollowCameraFight_ResetOurs), nullptr, nullptr, re::Mode::Original);

// 0x4FE730: what the fight camera frames: the second target, the ped's focus, the
// ped close by (lowered by half its height when not a ped), or with none the last
// point (+0x2C) or a step in front of the ped
CVector* __fastcall FollowCameraFight_GetTargetOurs(uint8_t* self, re::Edx, CVector* out) {
    uint8_t* ped = TargetPed();
    CVector pedPosition = *Position(ped);
    *out = {0.0f, 0.0f, 0.0f};
    uint8_t* target = SecondTarget();
    if (!target) {
        if (FocusTarget(ped)) {
            target = static_cast<uint8_t*>(FocusTarget(ped));
        } else {
            target = CloseEntity();
        }
        if (!target) {
            if (At<uint8_t>(self, 0x2C)) {
                *out = At<CVector>(self, 0x1C);
                return out;
            }
            CVector forward;
            CVector* f = Entity_GetForward(ped, &forward);
            CVector step{f->x, f->y, f->z};
            CVector sum;
            *out = *VectorAdd(&sum, &pedPosition, &step);
            return out;
        }
    }
    *out = *Position(target);
    if (EntityType(target) != 3 && int32_t(At<int16_t>(target, 0x10E)) != g_BF40F4) {
        double z = out->z;
        // vtable[27]: its height
        out->z = float(z - double(VCall<float>(target, 27)) * 0.5);
    }
    return out;
}
FOLLOWCAMBASE_REVERSED("FollowCameraFight::GetTarget", 0x004FE730, RE_BODY(&FollowCameraFight_GetTargetOurs), nullptr, nullptr, re::Mode::Original);

// 0x4FE8A0: whether there is a target (kept for cameraSettings +0x1C ms after the
// last one; dropped while the ped's +0x1D0 is moving with butes +0x28); *started
// (and eax) 1 when it just began
uint32_t __fastcall FollowCameraFight_CheckTargetOurs(uint8_t* self, re::Edx, uint8_t* started) {
    uint8_t* ped = TargetPed();
    bool hasTarget;
    if (SecondTarget()) {
        hasTarget = true;
    } else if (FocusTarget(ped)) {
        hasTarget = true;
    } else {
        hasTarget = CloseEntity() != nullptr;
    }
    if (!(fightTargetHoldTimeGuard & 1)) {
        fightTargetHoldTime = At<uint32_t>(cameraSettings, 0x1C);
        fightTargetHoldTimeGuard |= 1;
    }
    if (hasTarget) {
        At<uint32_t>(self, 0x28) = timeInMs;
    }
    uint8_t recent = uint32_t(timeInMs - At<uint32_t>(self, 0x28)) < fightTargetHoldTime;
    if (At<int32_t>(&TheCamera, 0x1A4) == 0 && At<uint8_t>(Butes(self), 0x28) != 0 && Ped_Unknown474BF0(ped) &&
        !(Unknown4D96B0(At<void*>(ped, 0x1D0)) == 0.0f)) {
        recent = 0;
    }
    if (At<uint8_t>(self, 0x2C) == 0 && recent) {
        *started = 1;
        At<uint8_t>(self, 0x2C) = recent;
        return 1;
    }
    *started = 0;
    At<uint8_t>(self, 0x2C) = recent;
    return 0;
}
FOLLOWCAMBASE_REVERSED("FollowCameraFight::CheckTarget", 0x004FE8A0, RE_BODY(&FollowCameraFight_CheckTargetOurs), nullptr, nullptr, re::Mode::Original);

// 0x4FE980: +0x34, how far to pull back, eased towards the square of the distance
// to the target past butes +0xA4 (times +0x8C, at most +0xA8)
void __fastcall FollowCameraFight_UpdatePullBackOurs(uint8_t* self, re::Edx) {
    if (!TheCamera.targetPed) {
        return;
    }
    CVector target{0.0f, 0.0f, 0.0f};
    if (uint8_t* second = SecondTarget()) {
        target = *Position(second);
    } else if (FocusTarget(TargetPed())) {
        target = *Position(FocusTarget(TargetPed()));
    } else if (uint8_t* close = CloseEntity()) {
        target = *Position(close);
    } else {
        uint8_t* ped = TargetPed();
        CVector forward;
        CVector* f = Entity_GetForward(ped, &forward);
        CVector step{f->x, f->y, f->z};
        CVector sum;
        target = *VectorAdd(&sum, Position(ped), &step);
    }
    CVector d;
    VectorSub(&d, &target, Position(TargetPed()));
    uint8_t* butes = Butes(self);
    float distance = SqrtAbs(LengthSq(d));
    float past = float(double(distance) - At<float>(butes, 0xA4));
    float pull = 0.0f;
    if (past > 0.0f) {
        pull = float(double(past) * past * At<float>(butes, 0x8C));
    }
    if (At<float>(butes, 0xA8) < pull) {
        pull = At<float>(butes, 0xA8);
    }
    float current = At<float>(self, 0x34);
    float rate = current < pull ? At<float>(butes, 0x90) : At<float>(butes, 0x94);
    At<float>(self, 0x34) = float((double(pull) - current) * rate + current);
    if (At<float>(Butes(self), 0xA8) < At<float>(self, 0x34)) {
        At<float>(self, 0x34) = At<float>(Butes(self), 0xA8);
    }
}
FOLLOWCAMBASE_REVERSED("FollowCameraFight::UpdatePullBack", 0x004FE980, RE_BODY(&FollowCameraFight_UpdatePullBackOurs), nullptr, nullptr, re::Mode::Original);

/// 0x4FEB90: where the fight camera goes (*camera) and looks (*lookAt): around the
// middle of the ped and what it frames (+0xAC), pulled back and up by the attribute
// set as the two get further apart (w: 0 at butes +0xAC, 1 at +0xB0), turned towards
// the side it's on, within angle limits (+0x9C..+0xA8)
void __fastcall FollowCameraFight_PlaceOurs(uint8_t* self, re::Edx, CVector* camera, CVector* lookAt) {
    CVector pedPosition = *Position(TargetPed());
    CVector target = At<CVector>(self, 0x1C);
    CVector* toTarget = &At<CVector>(self, 0x38);
    CVector d;
    *toTarget = *VectorSub(&d, &target, &pedPosition);
    float distance = SqrtAbs(LengthSq(*toTarget));
    CVector up{0.0f, 0.0f, 1.0f};
    CVector* across = &At<CVector>(self, 0x44);
    CVector cross;
    *across = *CrossProduct(&cross, toTarget, &up);
    Vector_Normalise(toTarget);
    Vector_Normalise(across);
    uint8_t* butes = Butes(self);
    float nearDistance = At<float>(butes, 0xAC);
    float w = float((double(distance) - nearDistance) / (double(At<float>(butes, 0xB0)) - nearDistance));
    if (w < 0.0f) {
        w = 0.0f;
    } else if (1.0f < w) {
        w = 1.0f;
    }
    CVector perpendicular = *across;
    CVector side{-toTarget->x, -toTarget->y, 0.0f};
    float sideSq = float((double(side.y) * side.y + double(side.x) * side.x) + 0.0 * 0.0);
    if (sideSq < 1e-5f) {
        side = {0.0f, 0.0f, 0.0f};
    } else {
        Vector_Normalise(&side);
    }
    At<CVector>(self, 0xB8) = side;
    At<CVector>(self, 0xC4) = perpendicular;
    CVector sum;
    CVector* both = VectorAdd(&sum, &target, &pedPosition);
    CVector middle{float(double(both->x) * 0.5), float(double(both->y) * 0.5), float(0.5 * double(both->z))};
    CVector towardsPed{float(double(w) * pedPosition.x), float(double(pedPosition.y) * w), float(double(pedPosition.z) * w)};
    double inverse = 1.0 - double(w);
    float inverseF = float(inverse);
    CVector towardsMiddle{float(double(inverseF) * middle.x), float(double(middle.y) * inverseF),
                          float(double(inverseF) * middle.z)};
    CVector centre;
    VectorAdd(&centre, &towardsMiddle, &towardsPed);
    At<CVector>(self, 0xAC) = centre;

    // how far the look-at point is drawn to the ped (more when the target is far,
    // or while locked on with the button held)
    float toPed = At<float>(butes, 0x44);
    float farDistance = At<float>(butes, 0xC0);
    float farToPed = At<float>(butes, 0xC8);
    if (farDistance < distance) {
        float u = float((double(distance) - farDistance) / (double(At<float>(butes, 0xC4)) - farDistance));
        if (!(u < 1.0f)) {
            u = 1.0f;
        }
        uint8_t pressed = Pad_GetValue(10, 0) > 0.0f;
        uint8_t* ped = TargetPed();
        if (ped && EntityType(ped) == 3 && At<int32_t>(ped, 0x1310) == 0xD && (pressed || At<uint8_t>(ped, 0x1EC0) != 0)) {
            uint8_t* pedTarget = Ped_GetTarget(ped);
            if (pedTarget && EntityType(pedTarget) == 3) {
                uint8_t* second = SecondTarget();
                if (Ped_Unknown46ECE0(ped) == second) {
                    u = Clamp(float(double(At<float>(Butes(self), 0xCC)) * u), 0.0f, 1.0f);
                }
            }
        }
        toPed = float((1.0 - u) * toPed + double(farToPed) * u);
    }
    CVector pedPart{float(double(toPed) * pedPosition.x), float(double(pedPosition.y) * toPed),
                    float(double(pedPosition.z) * toPed)};
    float rest = float(1.0 - toPed);
    CVector targetPart{float(double(target.x) * rest), float(double(rest) * target.y), float(double(rest) * target.z)};
    CVector look;
    *lookAt = *VectorAdd(&look, &targetPart, &pedPart);
    lookAt->z = At<float>(Butes(self), 0x48) + lookAt->z;

    // the camera's side of the line, flat
    CVector e;
    VectorSub(&e, &At<CVector>(self, 4), &centre);
    e.z = 0.0f;
    CVector flat{float((double(e.x) * side.x + double(e.y) * side.y) + double(side.z) * 0.0),
                 float((double(e.x) * perpendicular.x + double(e.y) * perpendicular.y) + 0.0 * perpendicular.z), 0.0f};
    Vector_Normalise(&flat);
    float g = g_A457E4;
    float scaled = float(double(g) * w);
    float extra;
    if (scaled < g) {
        extra = g;
    } else if (0.0f < scaled) {
        extra = 0.0f;
    } else {
        extra = scaled;
    }
    float limit = float(double(At<float>(Butes(self), 0x54)) * kPi / 180.0);
    float a0 = float((double(extra) + 180.0) * kPi / 180.0);
    float a1 = float(double(a0) - 1.0471975803375244);
    float a2 = float(double(a1) - limit);
    if (a2 > 0.0) {
        a2 = 0.0f;
    }
    float a3 = float(double(limit) + a2);
    At<float>(self, 0x9C) = a0;
    At<float>(self, 0xA0) = a1;
    At<float>(self, 0xA4) = a2;
    At<float>(self, 0xA8) = a3;
    float h = float(double(LimitAngle(VecHeading(&flat))) - 1.5707963705062866);
    if (double(h) > kPi) {
        h = float(double(h) - g_A4576C);
    }
    uint8_t* butes2 = Butes(self);
    float absH = std::fabs(h);
    float sign = float(double(h) / absH);
    float rate = At<float>(butes2, 0x58);
    float angle = EaseToLimit(EaseToLimit(absH, a2, a3, rate), a0, a1, rate);
    if (angle < 0.0) {
        angle = 0.0f;
    }
    float turn = float(double(sign) * angle);

    // the distance
    float low = At<float>(butes2, 0xB4);
    float high = At<float>(butes2, 0xB8);
    float range = float(double(At<float>(butes2, 0x38)) * distance / At<float>(butes2, 0x4C));
    if (low > range) {
        range = low;
    } else if (high < range) {
        range = high;
    }
    range = float(double(At<float>(butes2, 0xBC)) * w + double(range) * inverse);
    At<float>(self, 0xD0) = range;
    // (the game's _CIcos/_CIsin, on the x87 stack: the C library's here)
    float c = float(std::cos(double(turn)));
    float alongSide = float(double(c) * range);
    float s = float(std::sin(double(turn)));
    float alongPerpendicular = float(double(s) * range);
    float height = At<float>(butes2, 0x40);
    alongPerpendicular = float(double(w) * (double(sign) * g_A457E0) + alongPerpendicular);
    height = float(double(w) * g_C3D278 + height);
    float heightZero = float(0.0 * height);
    CVector lift{heightZero, heightZero, height};
    CVector perpendicularPart{float(double(alongPerpendicular) * perpendicular.x),
                              float(double(perpendicular.y) * alongPerpendicular),
                              float(double(alongPerpendicular) * perpendicular.z)};
    CVector sidePart{float(double(alongSide) * side.x), float(double(side.y) * alongSide), float(double(alongSide) * side.z)};
    CVector r1;
    CVector r2;
    CVector r3;
    CVector* p1 = VectorAdd(&r1, &centre, &sidePart);
    CVector* p2 = VectorAdd(&r2, p1, &perpendicularPart);
    *camera = *VectorAdd(&r3, p2, &lift);
    float z = float(double(At<float>(self, 0x34)) + camera->z);
    camera->z = float(double(z) + At<float>(self, 0x30));
    float dz = float(double(At<float>(self, 0x7C)) - pedPosition.z);
    lookAt->z = float(double(lookAt->z) + dz);
    camera->z = float(double(dz) + camera->z);
}
FOLLOWCAMBASE_REVERSED("FollowCameraFight::Place", 0x004FEB90, RE_BODY(&FollowCameraFight_PlaceOurs), nullptr, nullptr, re::Mode::Original);

// 0x4FF460: the fight camera's height offset (+0x30): on a new target, what puts the
// camera at +0x64; while framing one, raised or lowered (by up to g_A4580C, scaled)
// when the framed point (raised by g_A45804) is near the top or bottom of the view
// (the frustum's side planes from the fov less g_A45808 degrees), then held within
// g_A457F0..g_A457EC; otherwise eased back to 0. +0xEC: when the point was last well
// inside the view.
void __fastcall FollowCameraFight_UpdateHeight(uint8_t* self, re::Edx, int started) {
    float bump = 0.0f;
    float drop = 0.0f;
    uint8_t settle = 0;
    if (uint8_t(started)) {
        At<float>(self, 0x30) = 0.0f;
        CVector camera{0.0f, 0.0f, 0.0f};
        CVector lookAt{0.0f, 0.0f, 0.0f};
        FollowCameraFight_Place(self, &camera, &lookAt);
        At<float>(self, 0x30) = float(double(At<float>(self, 0x64)) - camera.z);
    }
    if (At<uint8_t>(self, 0x2C) == 0) {
        drop = float(double(g_A45810) * g_A4580C);
        At<uint32_t>(self, 0xEC) = timeInMs;
        settle = 1;
    } else {
        // the view with its y and z axes swapped
        cam::NiTransform view = *Camera_GetViewTransform(&TheCamera);
        for (int row = 0; row < 3; row++) {
            float y = view.rotate[row * 3 + 1];
            view.rotate[row * 3 + 1] = view.rotate[row * 3 + 2];
            view.rotate[row * 3 + 2] = y;
        }
        float halfFov = float(double(Camera_GetFov(&TheCamera)) * 0.017453293005625408 * 0.5);
        float across = float(double(halfFov) - double(g_A45808) * kPi / 180.0);
        // (the game's _CIcos/_CIsin, on the x87 stack: the C library's here)
        float c1 = float(std::cos(double(across)));
        float s1 = float(std::sin(double(across)));
        float upDown = float(double(halfFov) / GetAspectRatio() - double(g_A45808) * kPi / 180.0);
        float c2 = float(std::cos(double(upDown)));
        float s2 = float(std::sin(double(upDown)));
        CVector normals[4] = {{c1, -s1, 0.0f}, {0.0f, -s2, c2}, {-c1, -s1, 0.0f}, {0.0f, -s2, -c2}};
        CVector position = At<CVector>(self, 0x5C);
        float planes[4][4];
        for (int i = 0; i < 4; i++) {
            CVector rotated;
            CVector* n = Matrix_Multiply(&view, &rotated, &normals[i]);
            CVector normal = *n;
            normals[i] = normal;
            float dot = float((double(position.x) * normal.x + double(position.y) * normal.y) + double(position.z) * normal.z);
            planes[i][0] = normal.x;
            planes[i][1] = normal.y;
            planes[i][2] = normal.z;
            planes[i][3] = float(double(dot) - 0.0);
        }
        CVector point = At<CVector>(self, 0x1C);
        point.z = float(double(point.z) + g_A45804);
        float margin = g_C3D27C;
        // how far inside each plane the point is, plus the margin
        auto inside = [&](int i) {
            float dot = float((double(point.x) * planes[i][0] + double(point.y) * planes[i][1]) + double(point.z) * planes[i][2]);
            float past = float(double(dot) - planes[i][3]);
            return float(double(past) + margin);
        };
        float top = inside(1);
        float bottom = inside(3);
        float left = inside(0);
        if (0.3f > left) {
            float right = inside(2);
            if (0.3f > right) {
                At<uint32_t>(self, 0xEC) = timeInMs;
            }
        }
        float h = g_A4580C;
        float x = bottom;
        if (top > 0.0f) {
            x = -h;
        } else if (bottom > 0.0f && !(-top > bottom)) {
            x = -top;
        }
        if (x >= 0.0f) {
            // (less when the framed point is close to the ped)
            CVector d;
            VectorSub(&d, &At<CVector>(self, 0x1C), Position(TargetPed()));
            float distance = SqrtAbs(LengthSq(d));
            float farDistance = g_A45800;
            if (farDistance > distance) {
                float nearD = g_A457FC;
                float r = float((double(distance) - nearD) / (double(farDistance) - nearD));
                if (!(r > 0.0f)) {
                    r = 0.0f;
                }
                x = float(double(r) * x + (1.0 - double(r)) * -double(h));
            }
        }
        if (top > 0.0f) {
            bump = -top;
        } else if (x > 0.0f) {
            bump = x;
        }
        float most = top > x ? top : x;
        if (most > 0.0f) {
            float offset = At<float>(self, 0x30);
            if (0.0f < offset) {
                if (top > x) {
                    bump = -h;
                }
            } else if (0.0f > offset && top < x) {
                bump = h;
            }
        }
        bump = float(double(g_A457F8) * bump);
        if (most <= 0.0f) {
            drop = -most;
            float offset = At<float>(self, 0x30);
            if (0.0f < offset) {
                if (top > x) {
                    drop = h;
                }
            } else if (0.0f > offset && top < x) {
                drop = h;
            }
        }
        drop = float(double(g_A45810) * drop);
        if (!(g_A457F4 > drop)) {
            drop = g_A457F4;
        }
        if (bump == 0.0f) {
            settle = 1;
        } else {
            At<float>(self, 0x30) = float(double(bump) + At<float>(self, 0x30));
        }
    }
    float offset = At<float>(self, 0x30);
    if (offset < g_A457F0) {
        offset = g_A457F0;
    }
    At<float>(self, 0x30) = offset;
    if (g_A457EC < offset) {
        offset = g_A457EC;
    }
    At<float>(self, 0x30) = offset;
    if (!settle) {
        return;
    }
    float slow = g_A457E8;
    if (offset > 0.0f) {
        if (slow > offset) {
            drop = float(double(offset) / slow * drop);
        }
        float settled = float(double(offset) - drop);
        At<float>(self, 0x30) = settled;
        if (settled < 0.0f) {
            At<float>(self, 0x30) = 0.0f;
        }
    } else if (offset < 0.0f) {
        float negative = -slow;
        if (negative < offset) {
            drop = float(double(offset) / negative * drop);
        }
        float settled = float(double(At<float>(self, 0x30)) + drop);
        At<float>(self, 0x30) = settled;
        if (settled > 0.0f) {
            At<float>(self, 0x30) = 0.0f;
        }
    }
}
FOLLOWCAMBASE_REVERSED("FollowCameraFight::UpdateHeight", 0x004FF460, RE_BODY(&FollowCameraFight_UpdateHeight), nullptr, nullptr, re::Mode::Original);

// 0x4FFD10: the fight camera's update: the view from the positions without a target
// (or 0x500650's spot while it's kept), then the attribute set, the pull-back, the
// target, and where the camera goes (+0x84) and looks (+0x90)
void __fastcall FollowCameraFight_Update(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 0xF4)) {
        At<CVector>(self, 4) = At<CVector>(self, 0xD4);
    } else {
        At<CVector>(self, 4) = At<CVector>(self, 0x5C);
    }
    At<CVector>(self, 0x10) = At<CVector>(self, 0x68);
    // (a dword whose low byte is cleared: 0x4FF460 gets the other three bytes as they
    // were on the stack)
    uint32_t started = 0;
    FollowCameraFight_Reset(self);
    FollowCameraFight_UpdatePullBack(self);
    FollowCameraFight_CheckTarget(self, reinterpret_cast<uint8_t*>(&started));
    CVector target;
    At<CVector>(self, 0x1C) = *FollowCameraFight_GetTarget(self, &target);
    FollowCameraFight_Unknown4FF460(self, int(started));
    FollowCameraFight_Place(self, &At<CVector>(self, 0x84), &At<CVector>(self, 0x90));
    if (At<uint8_t>(self, 0xF4) && uint32_t(timeInMs - At<uint32_t>(self, 0xF0)) > g_A45814) {
        At<uint8_t>(self, 0xF4) = 0;
    }
}
FOLLOWCAMBASE_REVERSED("FollowCameraFight::Update", 0x004FFD10, RE_BODY(&FollowCameraFight_Update), nullptr, nullptr, re::Mode::Original);

// 0x4FFE20 (FollowCameraVehicle): its blend (+0x1C) eased towards the driven
// controller's +0xF4 (not in the first 200 ms after +0x20), the two values it mixes
// (+0x14, +0x18), then the driven controller's 0x50AE90
void __fastcall FollowCameraVehicle_Process(uint8_t* self, re::Edx, float value) {
    if (uint32_t(timeInMs - At<uint32_t>(self, 0x20)) > 200) {
        float target = At<float>(At<uint8_t*>(self, 4), 0xF4);
        float rate = float(double(g_A45818) * 90.0 * value);
        if (!(rate < 1.0f)) {
            rate = 1.0f;
        }
        At<float>(self, 0x1C) = float((double(target) - At<float>(self, 0x1C)) * rate + At<float>(self, 0x1C));
    }
    double blend = At<float>(self, 0x1C);
    double inverse = 1.0 - blend;
    At<float>(self, 0x18) = float(At<float>(self, 0x28) * blend + At<float>(self, 0x24) * inverse);
    At<float>(self, 0x14) = float(At<float>(self, 0x30) * blend + At<float>(self, 0x2C) * inverse);
    VehicleCameraController_Unknown50AE90(At<void*>(self, 4), value);
}
FOLLOWCAMBASE_REVERSED("FollowCameraVehicle::Process", 0x004FFE20, RE_BODY(&FollowCameraVehicle_Process), nullptr, nullptr, re::Mode::Original);

// 0x4FFEC0: a timed fade (+0 the value: +0x10 times how far along; +4 running, +5
// repeat, +6 rising, +8 start and +0xC duration in seconds). A rising one that
// repeats turns to falling for another duration.
void __fastcall TimedFade_Update(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 4) == 0) {
        return;
    }
    float now = float(double(timeInMs) * double(0.001f));
    float t = float((double(now) - At<float>(self, 8)) / At<float>(self, 0xC));
    if (1.0f < t) {
        if (At<uint8_t>(self, 6) == 0) {
            t = 1.0f;
            At<uint8_t>(self, 4) = 0;
        } else if (At<uint8_t>(self, 5) != 0) {
            At<uint8_t>(self, 6) = 0;
            At<float>(self, 8) = At<float>(self, 0xC) + At<float>(self, 8);
        }
    }
    if (At<uint8_t>(self, 6) == 0) {
        t = float(1.0 - t);
    }
    At<float>(self, 0) = float(double(At<float>(self, 0x10)) * t);
}
FOLLOWCAMBASE_REVERSED("TimedFade::Update", 0x004FFEC0, RE_BODY(&TimedFade_Update), nullptr, nullptr, re::Mode::Original);

// 0x4FFF50: an orbit (+0 running, +4 start and +8 duration in seconds, +0xC the
// distance across, +0x10 flip the axis, +0x14 the axis, +0x20..+0x30 the heading and
// pitch limits around it): +0x38 the point `distance` from `a` towards `c` moved
// across, with its heading and pitch held within the limits (`b` is unused)
void __fastcall Orbit_Update(uint8_t* self, re::Edx, CVector a, CVector, CVector c) {
    if (At<uint8_t>(self, 0) == 0) {
        return;
    }
    float now = float(double(timeInMs) * double(0.001f));
    float t = float((double(now) - At<float>(self, 4)) / At<float>(self, 8));
    if (t > 1.0f) {
        At<uint8_t>(self, 0) = 0;
    }
    CVector d;
    VectorSub(&d, &c, &a);
    float cz = c.z;
    CVector flat{d.x, d.y, 0.0f};
    Vector_Normalise(&flat);
    float across = std::fabs(At<float>(self, 0xC));
    CVector offset{float(double(flat.x) * across), float(double(flat.y) * across), float(double(across) * flat.z)};
    CVector sum;
    CVector* moved = VectorAdd(&sum, &a, &offset);
    c.x = moved->x;
    c.y = moved->y;
    c.z = cz;
    CVector dir;
    VectorSub(&dir, &c, &a);
    CVector n = dir;
    Vector_Normalise(&n);
    CVector axis = At<CVector>(self, 0x14);
    float dot = float((double(n.y) * axis.y + double(axis.x) * n.x) + double(n.z) * axis.z);
    if (dot < 0.0f && At<uint8_t>(self, 0x10) != 0) {
        axis.x = -axis.x;
        axis.y = -axis.y;
        axis.z = -axis.z;
    }
    float heading = VecHeading(&n);
    float pitch = VecPitch(&n);
    float axisHeading = VecHeading(&axis);
    float axisPitch = VecPitch(&axis);
    float headingLow = At<float>(self, 0x24);
    float headingHigh = At<float>(self, 0x30);
    float pitchLow = At<float>(self, 0x20);
    float pitchHigh = At<float>(self, 0x2C);
    float headingDiff = AngleDiff(axisHeading, heading);
    if (headingLow > headingDiff) {
        heading = headingLow + axisHeading;
    } else if (headingHigh < headingDiff) {
        heading = headingHigh + axisHeading;
    }
    float pitchDiff = AngleDiff(axisPitch, pitch);
    if (pitchLow > pitchDiff) {
        pitch = pitchLow + axisPitch;
    } else if (pitchHigh < pitchDiff) {
        pitch = pitchHigh + axisPitch;
    }
    float distance = SqrtAbs(float((double(dir.x) * dir.x + double(dir.y) * dir.y) + double(dir.z) * dir.z));
    if (!(orbitAxisGuard & 1)) {
        orbitAxisGuard |= 1;
        orbitAxis = {0.0f, 1.0f, 0.0f};
    }
    CVector r;
    CVector* fromAngles = VectorFromAngles(&r, orbitAxis, heading, pitch);
    CVector unit{fromAngles->x, -fromAngles->y, fromAngles->z};
    CVector out{float(double(distance) * unit.x), float(double(distance) * unit.y), float(double(distance) * unit.z)};
    CVector result;
    At<CVector>(self, 0x38) = *VectorAdd(&result, &a, &out);
}
FOLLOWCAMBASE_REVERSED("Orbit::Update4FFF50", 0x004FFF50, RE_BODY(&Orbit_Update), nullptr, nullptr, re::Mode::Original);

// 0x5003C0 (FollowCameraBase): every 200 ms (not in a sub-step, with butes +0xAC),
// whether a ped (not +0x1310 == 0xD) is within butes +0xAC of +0xA4 (its top: its
// height added): +0x5C for butes +0xA8 seconds; +0x58 the closest distance
void __fastcall FollowCameraBase_CheckPedsNear(uint8_t* self, re::Edx) {
    if (At<int32_t>(self, 0x2C) != 0) {
        return;
    }
    uint8_t* butes = At<uint8_t*>(self, 0x518);
    if (!(0.0f < At<float>(butes, 0xAC))) {
        return;
    }
    if (timeInMs > At<uint32_t>(self, 0x60)) {
        At<uint32_t>(self, 0x60) = timeInMs + 200;
        CPool<uint8_t>* pool = pedPool;
        float radiusSq = float(double(At<float>(butes, 0xAC)) * At<float>(butes, 0xAC));
        float best = 99999997952.0f;
        for (int slot = 0; slot < pool->size; slot++) {
            if (pool->flags[slot] & 0x80) {
                continue;
            }
            uint8_t* ped = pool->objects + pool->elemSize * slot;
            if (!ped || At<int32_t>(ped, 0x1310) == 0xD) {
                continue;
            }
            CVector d;
            VectorSub(&d, Position(ped), &At<CVector>(self, 0xA4));
            // vtable[27]: its height
            d.z = float(double(VCall<float>(ped, 27)) + d.z);
            float distanceSq = LengthSq(d);
            if (radiusSq > distanceSq && best > distanceSq) {
                best = distanceSq;
                At<uint8_t>(self, 0x5C) = 1;
                // (fistp, truncating: the ms as a 64-bit integer, low half used)
                int32_t ms = int32_t(int64_t(double(At<float>(At<uint8_t*>(self, 0x518), 0xA8)) * -1000.0));
                At<uint32_t>(self, 0x64) = timeInMs - uint32_t(ms);
            }
        }
        At<float>(self, 0x58) = SqrtAbs(best);
    }
    if (At<uint8_t>(self, 0x5C) && timeInMs > At<uint32_t>(self, 0x64)) {
        At<uint8_t>(self, 0x5C) = 0;
    }
}
FOLLOWCAMBASE_REVERSED("FollowCameraBase::CheckPedsNear", 0x005003C0, RE_BODY(&FollowCameraBase_CheckPedsNear), nullptr, nullptr, re::Mode::Original);

// the colpoint the line test fills (0x2C bytes on the stack)
struct LineHit {
    CVector position;       // +0
    re::Ptr32<void> entity;           // +0xC (a registered reference)
    float unknown10[3];     // +0x10
    float unknown1C[2];     // +0x1C (not set by the game before the test: zeroed here)
    float depth;            // +0x24
    uint8_t unknown28[4];   // +0x28
};
static_assert(offsetof(LineHit, entity) == 0xC);
static_assert(offsetof(LineHit, depth) == 0x24);

// 0x500650: a new spot for the fight camera: six around the centre, at -150..150
// degrees (30 apart) off its side, each eased into the angle limits; the closest to
// +0x5C with a clear line from +0x68, or else the one whose blocked line got the
// furthest. +0xD4, kept from now (+0xF0; +0xE0 the camera then, +0xF4 in use).
void __fastcall FollowCameraFight_FindView(uint8_t* self, re::Edx) {
    CVector pedPosition = *Position(TargetPed());
    CVector toTarget;
    VectorSub(&toTarget, &At<CVector>(self, 0x1C), &pedPosition);   // (unused)
    float bestBlocked = 99999997952.0f;
    float bestClear = 99999997952.0f;
    CVector blockedSpot{0.0f, 0.0f, 0.0f};
    CVector clearSpot{0.0f, 0.0f, 0.0f};
    for (int i = -5; i < 7; i += 2) {
        float angle = float(double(i) * 0.5235987901687622);
        float absAngle = std::fabs(angle);
        float sign = float(double(angle) / absAngle);
        float rate = g_A4581C;
        float eased = EaseToLimit(EaseToLimit(absAngle, At<float>(self, 0xA4), At<float>(self, 0xA8), rate),
                                  At<float>(self, 0x9C), At<float>(self, 0xA0), rate);
        if (0.0f > eased) {
            eased = 0.0f;
        }
        float turn = float(double(sign) * eased);
        float dz = float(double(At<float>(self, 0x7C)) - pedPosition.z);
        float height = float(((double(dz) + At<float>(Butes(self), 0x40)) + At<float>(self, 0x34)) + At<float>(self, 0x30));
        float heightZero = float(0.0 * height);
        CVector lift{heightZero, heightZero, height};
        // (the game's _CIsin/_CIcos, on the x87 stack: the C library's here)
        float s = float(std::sin(double(turn)));
        float alongPerpendicular = float(double(s) * At<float>(self, 0xD0));
        CVector perpendicularPart{float(double(At<float>(self, 0xC4)) * alongPerpendicular),
                                  float(double(At<float>(self, 0xC8)) * alongPerpendicular),
                                  float(double(alongPerpendicular) * At<float>(self, 0xCC))};
        float c = float(std::cos(double(turn)));
        float alongSide = float(double(c) * At<float>(self, 0xD0));
        CVector sidePart{float(double(alongSide) * At<float>(self, 0xB8)), float(double(alongSide) * At<float>(self, 0xBC)),
                         float(double(alongSide) * At<float>(self, 0xC0))};
        CVector r1;
        CVector r2;
        CVector spot;
        VectorAdd(&spot, VectorAdd(&r2, VectorAdd(&r1, &At<CVector>(self, 0xAC), &sidePart), &perpendicularPart), &lift);

        LineHit hit{};
        hit.depth = 3.4028234663852886e38f;   // FLT_MAX
        re::Ptr32<void> hitEntity = nullptr;
        CVector dir;
        VectorSub(&dir, &spot, &At<CVector>(self, 0x68));
        Vector_Normalise(&dir);
        float reach = At<float>(Butes(self), 0x78);
        CVector step{float(double(reach) * dir.x), float(double(dir.y) * reach), float(double(reach) * dir.z)};
        CVector end;
        VectorAdd(&end, &spot, &step);
        if (World_TestLine(&At<CVector>(self, 0x68), &end, &hit, &hitEntity, 0x89, 0, 0)) {
            CVector d;
            VectorSub(&d, &end, &hit.position);
            float distance = SqrtAbs(LengthSq(d));
            if (bestBlocked > distance) {
                bestBlocked = distance;
                blockedSpot = spot;
            }
        } else {
            CVector d;
            VectorSub(&d, &spot, &At<CVector>(self, 0x5C));
            float distance = SqrtAbs(LengthSq(d));
            if (bestClear > distance) {
                bestClear = distance;
                clearSpot = spot;
            }
        }
        if (hit.entity) {
            CleanUpOldReference(hit.entity, &hit.entity);
        }
    }
    At<CVector>(self, 0xD4) = bestClear != 99999997952.0f ? clearSpot : blockedSpot;
    At<uint32_t>(self, 0xF0) = timeInMs;
    At<CVector>(self, 0xE0) = At<CVector>(self, 0x84);
    At<uint8_t>(self, 0xF4) = 1;
}
FOLLOWCAMBASE_REVERSED("FollowCameraFight::FindView", 0x00500650, RE_BODY(&FollowCameraFight_FindView), nullptr, nullptr, re::Mode::Original);

// 0x500560: an attribute set by name (0: back to the default); 0 when not found
uint8_t __fastcall FollowCameraFight_SetButes(uint8_t* self, re::Edx, int name) {
    if (name == 0) {
        At<void*>(self, 0x50) = nullptr;
    } else {
        uint32_t names[2];
        uint32_t path[2];
        Name_Copy(&names[0], butesName_1B653C8);
        Name_Construct(&names[1], name);
        Path_Build(path, 2, names);
        void* found = Butes_Find(path, butesRoot_1B653C4);
        At<void*>(self, 0x50) = found;
        if (!found) {
            At<void*>(self, 0x50) = nullptr;
            At<uint8_t>(self, 0x54) = 0;
            At<int32_t>(self, 0x58) = -1;
            FollowCameraFight_Reset(self);
            DestructArray(names, 4, 2, reinterpret_cast<void*>(0x0049A650));
            return 0;
        }
        DestructArray(names, 4, 2, reinterpret_cast<void*>(0x0049A650));
    }
    uint8_t named = At<void*>(self, 0x50) != nullptr;
    At<uint8_t>(self, 0x54) = named;
    if (!named) {
        At<int32_t>(self, 0x58) = -1;
    }
    FollowCameraFight_Reset(self);
    return 1;
}
FOLLOWCAMBASE_REVERSED("FollowCameraFight::SetButes", 0x00500560, RE_BODY(&FollowCameraFight_SetButes), nullptr, nullptr, re::Mode::Original);

} // namespace
