#include "game/Camera.h"

#include <cmath>
#include <initializer_list>
#include "reversed.h"

// VehicleCameraController (0x5098F0..0x50B2F0): the camera behind the player's
// vehicle (bike, go-kart, ...): its attribute set blended from the named ones by the
// vehicle's class, the look direction (the vehicle's heading or velocity, the look
// back/left/right buttons), the height and distance eased behind it, the fov eased
// by speed, and a shake for a moment after it starts. Thiscalls are __fastcall
// wrappers with the unused edx.
//
// Layout (VehicleCameraController):
//   +4 the last look direction, +0x10 the direction the camera looks along, +0x1C the
//   camera position, +0x28 its change this frame, +0x34 the look-at, +0x40 its change,
//   +0x4C/+0x50 eased shares (to the set's +0x24/+0x20), +0x54 the time step (/50,
//   at most 0.05), +0x58 when it started (ms), +0x5C the height (eased to the set's
//   +0x2C by +0x60), +0x64 high above the ground, +0x65/+0x66 looking back (now/last
//   frame), +0x67/+0x68 last frame's +0x69/+0x6A (looking left/right), +0x6B the
//   direction's been worked out this frame, +0x6C a step with a given time step,
//   +0x70 the speed smoothing (0x509A10: speed, low, high, share, last share,
//   smoothed, last smoothed), +0x8C the fov easing (0x50A0D0), +0xBC..+0xD0 the
//   distance easing (+0xD0 the distance), +0xD4..+0xF8 a snapshot (0x50AE90),
//   +0xFC a set by name (overrides), +0x100 the default set, +0x104 the set when
//   high above the ground, +0x108 the blended set (VehicleCamButes, 0x88 bytes),
//   +0x10C/+0x110/+0x114/+0x118/+0x11C the sets for vehicle classes 7, -, 6, 8, 10.
// The set: +0x18/+0x1C the distance (near/far), +0x20/+0x24 the share targets, +0x28
// the look-at height, +0x2C the camera height, +0x48 how high counts as high, +0x50
// how fast the direction follows, +0x60/+0x64 the fov range, +0x68/+0x6C the speed
// range, +0x70 no look buttons.

#define VEHICLECAM_REVERSED(...) REVERSED(__VA_ARGS__)

namespace {

using cam::CVector;
using cam::NiTransform;

GAME_VAR(void*, theNiCamera, 0x00C3CCF4);            // TheCamera.camera
GAME_VAR(float, cameraFov, 0x00C3CD04);              // TheCamera.fov
GAME_VAR(int32_t, g_widescreen, 0x00C3CD0C);         // TheCamera+0xA4
GAME_VAR(CVector, cameraUnknown180, 0x00C3CDE8);     // TheCamera+0x180
GAME_VAR(CVector, cameraUnknown18C, 0x00C3CDF4);     // TheCamera+0x18C
GAME_VAR(CVector, cameraLookAt, 0x00C3CE00);         // TheCamera+0x198
GAME_VAR(float, cameraFovOverride, 0x00C3CDD0);      // TheCamera.fovOverride
GAME_VAR(int32_t, cameraFovOverridden, 0x00C3CDD4);  // TheCamera.fovOverridden
GAME_VAR(uint8_t, cameraFovLocked, 0x00C3CF82);      // TheCamera+0x31A
GAME_VAR(uint8_t*, cameraTarget, 0x00C3CE14);        // TheCamera.targetPed
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);           // CWorld::Players[0].m_pPed
GAME_VAR(float, timeStep, 0x00C1A9A4);               // CTimer::ms_fTimeStep
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);            // CTimer::m_snTimeInMilliseconds
GAME_VAR(uint8_t*, cameraSettings, 0x020C88B8);
GAME_VAR(uint8_t, altSteering, 0x00A14925);
// the tunables
GAME_VAR(float, g_A45974, 0x00A45974);   // 0.3: the speed share's smoothing
GAME_VAR(float, g_A45978, 0x00A45978);   // 0.01: the height change that restarts the height easing
GAME_VAR(float, g_A459C4, 0x00A459C4);   // 4: the fov easing's rate at most
GAME_VAR(float, g_A459C8, 0x00A459C8);   // 1: ... at least
GAME_VAR(float, g_A459CC, 0x00A459CC);   // 0.5: the rate's fall
GAME_VAR(float, g_A459D0, 0x00A459D0);   // 1: its rise (above the range)
GAME_VAR(float, g_A459D4, 0x00A459D4);   // 6: its rise (below the range)
GAME_VAR(float, g_A459D8, 0x00A459D8);   // 0.12: the input's return to 0
GAME_VAR(float, g_A459DC, 0x00A459DC);   // 0.005: the input's range (high)
GAME_VAR(float, g_A459E0, 0x00A459E0);   // -0.015: ... (low)
GAME_VAR(float, g_A459E4, 0x00A459E4);   // 0.01: the height easing's rise per step
GAME_VAR(float, g_A459E8, 0x00A459E8);   // 25: the start shake's size
GAME_VAR(int32_t, g_A459EC, 0x00A459EC); // 1000: the start shake's length (ms)
GAME_VAR(float, g_A459F0, 0x00A459F0);   // 1: the speed's scale

inline void* const transition = reinterpret_cast<void*>(0x00A45500);
inline void* const theCamera = reinterpret_cast<void*>(0x00C3CC68);
// the butes' names and root (a set is found by {0x1B653B8, name})
inline const void* const name_1B653B8 = reinterpret_cast<const void*>(0x01B653B8);
inline const void* const root_1B65428 = reinterpret_cast<const void*>(0x01B65428);
inline const void* const nameDefault = reinterpret_cast<const void*>(0x01B65400);
inline const void* const nameHigh = reinterpret_cast<const void*>(0x01B653D4);
inline const void* const nameClass7 = reinterpret_cast<const void*>(0x01B653D8);
inline const void* const name110 = reinterpret_cast<const void*>(0x01B653DC);
inline const void* const nameClass6 = reinterpret_cast<const void*>(0x01B653E0);
inline const void* const nameClass8 = reinterpret_cast<const void*>(0x01B653E8);
inline const void* const nameClass10 = reinterpret_cast<const void*>(0x01B653E4);
inline void* const reloadCallback = reinterpret_cast<void*>(0x00509960);
constexpr uintptr_t kVehicleCameraVtable = 0x00914758;
constexpr uintptr_t kVehicleCamButesVtable = 0x00914740;

constexpr double kPi = 3.1415927410125732;   // (float pi, as a double)

GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(NiTransform*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(CVector*, __thiscall, Entity_GetForward, 0x004154A0, void* entity, CVector* out);
GAME_FN(CVector*, __thiscall, Vehicle_Fn4736F0, 0x004736F0, void* vehicle, CVector* out);
GAME_FN(int32_t*, __thiscall, Vehicle_GetVehicleClass, 0x004CAF30, void* vehicle, int32_t* out);
GAME_FN(uint8_t*, __cdecl, FindPlayerVehicle, 0x00446970, uintptr_t player);
GAME_FN(float, __cdecl, FindGroundZ, 0x004635A0, float x, float y, float z, uint8_t* found, int arg);
GAME_FN(uint8_t*, __thiscall, Ped_GetTarget, 0x00497100, void* ped);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(void*, __cdecl, GetPlayerPadInput, 0x00738070);
GAME_FN(uint8_t, __cdecl, Pad_Fn738500, 0x00738500, void* pad);
GAME_FN(int32_t, __thiscall, Transition_IsActive, 0x00856850, void* transition);
GAME_FN(void, __thiscall, Camera_LookAtPoint, 0x004F2C60, void* camera, const CVector* position, const CVector* target);
GAME_FN(void, __thiscall, Camera_Fn4F0920, 0x004F0920, void* camera, const CVector* v);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __thiscall, Track_Construct, 0x0061A880, void* self);
GAME_FN(void, __thiscall, VehicleCamButes_SetDefaults, 0x007287A0, void* self);
GAME_FN(void, __cdecl, AddLoadCallback, 0x0072B870, void* fn, void* object);
// the butes
GAME_FN(void, __thiscall, Name_Copy, 0x005FA880, void* self, const void* from);
GAME_FN(void, __thiscall, NameKey_Construct, 0x005FA910, void* self, const char* name);
GAME_FN(void, __thiscall, Path_Build, 0x005FA8C0, void* self, int count, const void* names);
GAME_FN(void*, __cdecl, Butes_Find, 0x0072AEA0, const void* path, const void* root);
GAME_FN(void, __thiscall, EmptyThiscall, 0x0049A650, void* self);   // (the name's destructor)
// VehicleCameraController (through the game's addresses, so modes apply)
GAME_FN(void, __cdecl, VehicleCam_FindButes, 0x005098F0, re::Ptr32<void>* out, const void* name);
GAME_FN(void, __thiscall, SpeedShare_Update, 0x00509A10, void* self);
GAME_FN(void, __stdcall, VehicleCamButes_Blend, 0x00509A80, void* out, const void* a, const void* b, float weightA, float weightB);
GAME_FN(void, __thiscall, VehicleCam_BlendButes, 0x00509C50, void* self, float blend);
GAME_FN(void, __thiscall, FovEasing_Update, 0x0050A0D0, void* self);
GAME_FN(void, __thiscall, VehicleCam_UpdateLookAt, 0x0050A250, void* self);
GAME_FN(CVector*, __thiscall, VehicleCam_GetDirection, 0x0050A400, void* self, CVector* out);
GAME_FN(void, __thiscall, VehicleCam_UpdatePosition, 0x0050A5C0, void* self);
GAME_FN(void, __thiscall, VehicleCam_UpdateDirection, 0x0050A790, void* self);
GAME_FN(void, __thiscall, VehicleCam_ReadInput, 0x0050A970, void* self);
GAME_FN(void, __thiscall, VehicleCam_UpdateHeight, 0x0050AAA0, void* self);
GAME_FN(void, __thiscall, VehicleCam_Apply, 0x0050AB70, void* self);
GAME_FN(void, __thiscall, VehicleCam_UpdateEasing, 0x0050ABF0, void* self);

using re::At;

uint8_t* Set(void* self) {
    return At<uint8_t*>(self, 0x108);
}

// (the game: the float sum, its fabs, then the CRT's sqrt; each step rounded to float, the
// main thread's x87 running at 24 bits)
float Length(const CVector& v) {
    float sum = (v.x * v.x + v.y * v.y) + v.z * v.z;
    return float(std::sqrt(double(std::fabs(sum))));
}

const CVector* EntityPosition(uint8_t* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<const CVector*>(matrix + 0x30) : reinterpret_cast<const CVector*>(entity + 4);
}

// a set by the name path {0x1B653B8, second}
void* FindButes(const uint32_t* second) {
    uint32_t names[2];
    uint32_t path[2];
    Name_Copy(&names[0], name_1B653B8);
    Name_Copy(&names[1], second);
    Path_Build(path, 2, names);
    void* found = Butes_Find(path, root_1B65428);
    for (int i = 1; i >= 0; i--) {
        EmptyThiscall(&names[i]);
    }
    return found;
}

// ---- 0x5098F0 (cdecl): *out = the set named `name`
void __cdecl VehicleCam_FindButesOurs(re::Ptr32<void>* out, const void* name) {
    *out = FindButes(static_cast<const uint32_t*>(name));
}
VEHICLECAM_REVERSED("VehicleCameraController::FindButes_5098F0", 0x005098F0, &VehicleCam_FindButesOurs, nullptr, nullptr, re::Mode::Original);

// 0x509980 (vtable[8]): the set to use over the others by name (nullptr: none); 1
uint8_t __fastcall VehicleCam_SetButesByNameOurs(uint8_t* self, re::Edx, const char* name) {
    if (!name) {
        At<void*>(self, 0xFC) = nullptr;
        return 1;
    }
    uint32_t names[2];
    uint32_t path[2];
    Name_Copy(&names[0], name_1B653B8);
    NameKey_Construct(&names[1], name);
    Path_Build(path, 2, names);
    At<void*>(self, 0xFC) = Butes_Find(path, root_1B65428);
    for (int i = 1; i >= 0; i--) {
        EmptyThiscall(&names[i]);
    }
    return 1;
}
VEHICLECAM_REVERSED("VehicleCameraController::SetButesByName_509980", 0x00509980, RE_BODY(&VehicleCam_SetButesByNameOurs), nullptr, nullptr, re::Mode::Original);

// 0x509A10: a value's share of its range (clamped to 0..1) and that smoothed (the
// last of each kept). Floats: +0 the value, +4 low, +8 high, +0xC the share, +0x10
// last, +0x14 smoothed, +0x18 last
void __fastcall SpeedShare_UpdateOurs(float* self, re::Edx) {
    float range = self[2] - self[1];
    float share = float((double(self[0]) - self[1]) / range);
    if (share > 1.0f) {
        share = 1.0f;
    } else if (0.0f > share) {
        share = 0.0f;
    }
    self[4] = self[3];
    self[3] = share;
    self[6] = self[5];
    double k = g_A45974;
    self[5] = float((1.0 - k) * self[5] + k * share);
}
VEHICLECAM_REVERSED("VehicleCameraController::SpeedShare_509A10", 0x00509A10, RE_BODY(&SpeedShare_UpdateOurs), nullptr, nullptr, re::Mode::Original);

// 0x509A80 (stdcall): out = a * weightA + b * weightB, field by field (+0x18..+0x84;
// the flag at +0x70 from b)
void __stdcall VehicleCamButes_BlendOurs(uint8_t* out, const uint8_t* a, const uint8_t* b, float weightA, float weightB) {
    for (size_t offset = 0x18; offset <= 0x84; offset += 4) {
        if (offset == 0x70) {
            out[0x70] = b[0x70];
            continue;
        }
        const float fa = *reinterpret_cast<const float*>(a + offset);
        const float fb = *reinterpret_cast<const float*>(b + offset);
        At<float>(out, offset) = float(double(fb) * weightB + double(fa) * weightA);
    }
}
VEHICLECAM_REVERSED("VehicleCameraController::BlendButes_509A80", 0x00509A80, &VehicleCamButes_BlendOurs, nullptr, nullptr, re::Mode::Original);

// 0x509C50: the used set eased towards the one that fits by `blend` (0: by the time
// step times the settings' +0x50, or +0x54 for a named set or a class's, +0x4C when
// high above the ground): the named one, or with the player on a bike or kart (the
// vehicle's +0x46C 0 or 1) its class's, or high above the ground (not on one), or the
// default
void __fastcall VehicleCam_BlendButesOurs(uint8_t* self, re::Edx, float blend) {
    uint8_t* vehicle = FindPlayerVehicle(0);
    void* target;
    size_t rateOffset;
    bool riding = vehicle && (At<int32_t>(vehicle, 0x46C) == 0 || At<int32_t>(vehicle, 0x46C) == 1);
    if (riding) {
        int32_t kind;
        if (At<void*>(self, 0xFC)) {
            target = At<void*>(self, 0xFC);
            rateOffset = 0x54;
        } else if (At<void*>(self, 0x10C) && *Vehicle_GetVehicleClass(vehicle, &kind) == 7) {
            target = At<void*>(self, 0x10C);
            rateOffset = 0x54;
        } else if (At<void*>(self, 0x114) && *Vehicle_GetVehicleClass(vehicle, &kind) == 6) {
            target = At<void*>(self, 0x114);
            rateOffset = 0x54;
        } else if (At<void*>(self, 0x118) && *Vehicle_GetVehicleClass(vehicle, &kind) == 8) {
            target = At<void*>(self, 0x118);
            rateOffset = 0x54;
        } else if (At<void*>(self, 0x11C) && *Vehicle_GetVehicleClass(vehicle, &kind) == 10) {
            target = At<void*>(self, 0x11C);
            rateOffset = 0x54;
        } else {
            target = At<void*>(self, 0x100);
            rateOffset = 0x50;
        }
    } else if (At<void*>(self, 0xFC)) {
        target = At<void*>(self, 0xFC);
        rateOffset = 0x54;
    } else if (At<uint8_t>(self, 0x64)) {
        target = At<void*>(self, 0x104);
        rateOffset = 0x4C;
    } else {
        target = At<void*>(self, 0x100);
        rateOffset = 0x50;
    }
    if (blend == 0.0f) {
        blend = At<float>(cameraSettings, rateOffset) * At<float>(self, 0x54);
    }
    VehicleCamButes_Blend(Set(self), Set(self), target, 1.0f - blend, blend);
}
VEHICLECAM_REVERSED("VehicleCameraController::BlendButes_509C50", 0x00509C50, RE_BODY(&VehicleCam_BlendButesOurs), nullptr, nullptr, re::Mode::Original);

// 0x509F50: the distance (+0xD0), and with hits the height easing restarted when the
// height between the two points moved by 0.01 or more (from the follow camera's
// collision pass)
void __fastcall VehicleCam_SetFromFollowOurs(uint8_t* self, re::Edx, CVector position, CVector lookAt, float distance, int hits) {
    At<float>(self, 0xD0) = distance;
    if (hits != 0) {
        float height = position.z - lookAt.z;
        float moved = std::fabs(float(height - At<float>(self, 0x5C)));
        if (!(g_A45978 > moved)) {
            At<float>(self, 0x60) = 0.0f;
            At<float>(self, 0x5C) = height;
        }
    }
}
VEHICLECAM_REVERSED("VehicleCameraController::SetFromFollow_509F50", 0x00509F50, RE_BODY(&VehicleCam_SetFromFollowOurs), nullptr, nullptr, re::Mode::Original);

// 0x509FB0: *esi moved towards *eax by `t` (esi and eax are register arguments:
// not hookable)
RE_UNHOOKED("VehicleCameraController::MoveTowards_509FB0", 0x00509FB0);
void MoveTowards(CVector* v, const CVector* to, float t) {
    CVector d;
    VectorSub(&d, to, v);
    CVector part{d.x * t, d.y * t, t * d.z};
    v->x = v->x + part.x;
    v->y = part.y + v->y;
    v->z = v->z + part.z;
}

// 0x50A020 (vtable[1]): init: the sets looked up by name, the flags cleared, the used
// set snapped to the default
void __fastcall VehicleCam_InitOurs(uint8_t* self, re::Edx) {
    At<uint8_t>(self, 0x6C) = 0;
    At<void*>(self, 0xFC) = nullptr;
    VehicleCam_FindButes(&At<void*>(self, 0x100), nameDefault);
    VehicleCam_FindButes(&At<void*>(self, 0x104), nameHigh);
    VehicleCam_FindButes(&At<void*>(self, 0x10C), nameClass7);
    VehicleCam_FindButes(&At<void*>(self, 0x110), name110);
    VehicleCam_FindButes(&At<void*>(self, 0x114), nameClass6);
    VehicleCam_FindButes(&At<void*>(self, 0x118), nameClass8);
    VehicleCam_FindButes(&At<void*>(self, 0x11C), nameClass10);
    At<uint8_t>(self, 0x64) = 0;
    At<uint8_t>(self, 0x65) = 0;
    At<uint8_t>(self, 0x69) = 0;
    At<uint8_t>(self, 0x6A) = 0;
    At<uint8_t>(self, 0x66) = 0;
    At<uint8_t>(self, 0x67) = 0;
    At<uint8_t>(self, 0x68) = 0;
    At<uint8_t>(self, 0x6B) = 0;
    VehicleCam_BlendButes(self, 1.0f);
}
VEHICLECAM_REVERSED("VehicleCameraController::Init_50A020", 0x0050A020, RE_BODY(&VehicleCam_InitOurs), nullptr, nullptr, re::Mode::Original);

// 0x50A0D0: the fov easing. Floats: [0] the fov, [1] the speed share, [2] the speed
// change, [3]/[4] the fov range, [5] the time step, [6] the change (smoothed back to 0
// inside the range), [7] the rate, [8] the result, [9] the target, [0xB] the step
void __fastcall FovEasing_UpdateOurs(float* self, re::Edx) {
    float target = float((1.0 - self[1]) * self[3] + double(self[4]) * self[1]);
    self[9] = target;
    float low = g_A459E0;
    float high = g_A459DC;
    float change = self[2];
    if (low > change) {
        if (self[6] > change) {
            self[6] = self[2];
        }
    } else if (change > high) {
        if (self[6] < change) {
            self[6] = self[2];
        }
    } else if (0.0f > self[6]) {
        float back = float(double(self[5]) * g_A459D8 + self[6]);
        self[6] = back;
        if (back > 0.0f) {
            self[6] = 0.0f;
        }
    } else if (0.0f < self[6]) {
        float back = float(double(self[6]) - double(self[5]) * g_A459D8);
        self[6] = back;
        if (back < 0.0f) {
            self[6] = 0.0f;
        }
    }
    if (self[6] < low) {
        self[7] = float(double(self[5]) * g_A459D4 + self[7]);
    } else if (self[6] > high) {
        self[7] = float(double(self[5]) * g_A459D0 + self[7]);
    } else {
        self[7] = float(double(self[7]) - double(self[5]) * g_A459CC);
    }
    if (g_A459C8 > self[7]) {
        self[7] = g_A459C8;
    }
    if (g_A459C4 < self[7]) {
        self[7] = g_A459C4;
    }
    float step = self[7] * self[5];
    self[0xB] = step;
    float fov = float((double(target) - self[0]) * step + self[0]);
    self[0] = fov;
    self[8] = fov;
}
VEHICLECAM_REVERSED("VehicleCameraController::FovEasing_50A0D0", 0x0050A0D0, RE_BODY(&FovEasing_UpdateOurs), nullptr, nullptr, re::Mode::Original);

// 0x50A250: the look-at eased to above the player (its change kept in +0x40)
void __fastcall VehicleCam_UpdateLookAtOurs(uint8_t* self, re::Edx) {
    At<float>(self, 0x4C) = float((double(At<float>(Set(self), 0x24)) - At<float>(self, 0x4C)) * At<float>(self, 0x54) +
                                  At<float>(self, 0x4C));
    CVector up{0.0f, 0.0f, At<float>(Set(self), 0x28)};
    CVector sum;
    CVector above = *VectorAdd(&sum, EntityPosition(playerPed), &up);
    CVector* lookAt = &At<CVector>(self, 0x34);
    CVector* change = &At<CVector>(self, 0x40);
    CVector toAbove;
    VectorSub(&toAbove, &above, lookAt);
    float share = float(double(At<float>(self, 0x4C)) * 90.0 * At<float>(self, 0x54));
    if (!(share < 1.0f)) {
        share = 1.0f;
    }
    CVector off;
    const CVector* d = VectorSub(&off, &toAbove, change);
    CVector part{d->x * share, d->y * share, d->z * share};
    change->x = change->x + part.x;
    change->y = change->y + part.y;
    change->z = change->z + part.z;
    CVector towards{above.x * share, above.y * share, above.z * share};
    float keep = 1.0f - share;
    CVector kept{lookAt->x * keep, lookAt->y * keep, keep * lookAt->z};
    CVector next;
    VectorAdd(&next, &kept, &towards);
    CVector moved;
    *change = *VectorSub(&moved, &next, lookAt);
    *lookAt = next;
}
VEHICLECAM_REVERSED("VehicleCameraController::UpdateLookAt_50A250", 0x0050A250, RE_BODY(&VehicleCam_UpdateLookAtOurs), nullptr, nullptr, re::Mode::Original);

// 0x50A400: the direction behind the vehicle (once a frame; kept in +4): the player's
// flat forward (flipped when the player is upside down), or for a bike (not +0x6EC)
// or with no forward to speak of, the vehicle's flat velocity (the last one when
// that's too small too)
CVector* __fastcall VehicleCam_GetDirectionOurs(uint8_t* self, re::Edx, CVector* out) {
    if (At<uint8_t>(self, 0x6B) || !FindPlayerVehicle(0)) {
        *out = At<CVector>(self, 4);
        return out;
    }
    uint8_t* ped = playerPed;
    CVector dir;
    Entity_GetForward(ped, &dir);
    uint8_t* matrix = At<uint8_t*>(ped, 0x14);
    if (matrix && 0.0f > At<float>(matrix, 0x28)) {
        dir.x = -dir.x;
        dir.y = -dir.y;
        dir.z = -dir.z;
    }
    dir.z = 0.0f;
    uint8_t* vehicle = FindPlayerVehicle(0);
    bool useVelocity = vehicle && At<int32_t>(FindPlayerVehicle(0), 0x46C) == 1 && At<uint8_t>(FindPlayerVehicle(0), 0x6EC) == 0;
    if (!useVelocity) {
        float sq = float((double(dir.x) * dir.x + double(dir.y) * dir.y) + double(dir.z) * dir.z);
        useVelocity = !(sq > 0.001f);
    }
    if (useVelocity) {
        uint8_t* v = FindPlayerVehicle(0);
        dir = At<CVector>(v, 0x124);
        dir.z = 0.0f;
        float sq = float((double(dir.x) * dir.x + double(dir.y) * dir.y) + 0.0 * 0.0);
        if (!(sq > 0.001f)) {
            dir = At<CVector>(self, 4);
        } else {
            Vector_Normalise(&dir);
        }
    } else {
        Vector_Normalise(&dir);
    }
    At<CVector>(self, 4) = dir;
    *out = dir;
    At<uint8_t>(self, 0x6B) = 1;
    return out;
}
VEHICLECAM_REVERSED("VehicleCameraController::GetDirection_50A400", 0x0050A400, RE_BODY(&VehicleCam_GetDirectionOurs), nullptr, nullptr, re::Mode::Original);

// 0x50A5C0: the camera position: the height eased in (+0x60 rising by 0.01 a step),
// then behind the look-at along the direction at the distance, raised by the height
// (snapped there when looking back changed), eased (its change kept in +0x28)
void __fastcall VehicleCam_UpdatePositionOurs(uint8_t* self, re::Edx) {
    if (1.0f > At<float>(self, 0x60)) {
        float rise = g_A459E4 + At<float>(self, 0x60);
        At<float>(self, 0x60) = rise;
        if (rise > 1.0f) {
            At<float>(self, 0x60) = 1.0f;
        }
    }
    // (in float, step by step: the main thread's x87 rounds each operation to 24 bits)
    float target = At<float>(Set(self), 0x2C);
    float share = At<float>(self, 0x60) * 90.0f * At<float>(self, 0x54);
    if (!(share < 1.0f)) {
        share = 1.0f;
    }
    At<float>(self, 0x5C) = (target - At<float>(self, 0x5C)) * share + At<float>(self, 0x5C);
    At<float>(self, 0x50) = (At<float>(Set(self), 0x20) - At<float>(self, 0x50)) * At<float>(self, 0x54) + At<float>(self, 0x50);
    float back = -At<float>(self, 0xD0);
    CVector behind{At<float>(self, 0x10) * back, At<float>(self, 0x14) * back, back * At<float>(self, 0x18)};
    CVector wanted;
    VectorAdd(&wanted, &At<CVector>(self, 0x34), &behind);
    wanted.z = wanted.z + At<float>(self, 0x5C);
    if (At<uint8_t>(self, 0x65) != At<uint8_t>(self, 0x66)) {
        At<CVector>(self, 0x1C) = wanted;
    }
    float ease = At<float>(self, 0x50) * 90.0f * At<float>(self, 0x54);
    if (!(ease < 1.0f)) {
        ease = 1.0f;
    }
    CVector* position = &At<CVector>(self, 0x1C);
    CVector unused;
    VectorSub(&unused, &wanted, position);   // (not used)
    CVector towards{wanted.x * ease, wanted.y * ease, ease * wanted.z};
    float keep = 1.0f - ease;
    CVector kept{position->x * keep, position->y * keep, keep * position->z};
    CVector next;
    VectorAdd(&next, &kept, &towards);
    CVector moved;
    At<CVector>(self, 0x28) = *VectorSub(&moved, &next, position);
    *position = next;
}
VEHICLECAM_REVERSED("VehicleCameraController::UpdatePosition_50A5C0", 0x0050A5C0, RE_BODY(&VehicleCam_UpdatePositionOurs), nullptr, nullptr, re::Mode::Original);

// 0x50A790: the direction the camera looks along: at the player's target (with the
// pad's control 10 held), or back (looking back), left/right (the vehicle's
// 0x4736F0, its reverse), otherwise eased towards the direction behind by the set's
// +0x50 (flat, normalised when set outright)
void __fastcall VehicleCam_UpdateDirectionOurs(uint8_t* self, re::Edx) {
    CVector behind;
    VehicleCam_GetDirection(self, &behind);
    CVector* dir = &At<CVector>(self, 0x10);
    if (Ped_GetTarget(cameraTarget) && !(Pad_GetValue(10, 0) == 0.0f)) {
        const CVector* from = EntityPosition(FindPlayerVehicle(0));
        const CVector* to = EntityPosition(Ped_GetTarget(cameraTarget));
        CVector d;
        *dir = *VectorSub(&d, to, from);
        dir->z = 0.0f;
        Vector_Normalise(dir);
        return;
    }
    uint8_t back = At<uint8_t>(self, 0x65);
    if (back == At<uint8_t>(self, 0x66) && back == 0) {
        if (At<uint8_t>(self, 0x69)) {
            CVector side;
            *dir = *Vehicle_Fn4736F0(FindPlayerVehicle(0), &side);
            dir->z = 0.0f;
            Vector_Normalise(dir);
        } else if (At<uint8_t>(self, 0x6A)) {
            CVector side;
            const CVector* s = Vehicle_Fn4736F0(FindPlayerVehicle(0), &side);
            *dir = {-s->x, -s->y, -s->z};
            dir->z = 0.0f;
            Vector_Normalise(dir);
        } else {
            MoveTowards(dir, &behind, At<float>(Set(self), 0x50) * At<float>(self, 0x54));
        }
        return;
    }
    if (back == 0) {
        *dir = behind;
    } else {
        *dir = {-behind.x, -behind.y, -behind.z};
    }
}
VEHICLECAM_REVERSED("VehicleCameraController::UpdateDirection_50A790", 0x0050A790, RE_BODY(&VehicleCam_UpdateDirectionOurs), nullptr, nullptr, re::Mode::Original);

// 0x50A970: the time step (unless given), last frame's buttons kept, then (not in
// widescreen, no transition, look buttons allowed) the pad: control 14 looks back,
// control 18 (only with the alternative steering off or pad 0x738500) left/right
void __fastcall VehicleCam_ReadInputOurs(uint8_t* self, re::Edx) {
    if (!At<uint8_t>(self, 0x6C)) {
        float step = float(double(timeStep) / 50.0);
        At<float>(self, 0x54) = step;
        if (!(double(step) < 0.05000000074505806)) {
            step = 0.05f;
        }
        At<float>(self, 0x54) = step;
    }
    At<uint8_t>(self, 0x66) = At<uint8_t>(self, 0x65);
    At<uint8_t>(self, 0x67) = At<uint8_t>(self, 0x69);
    At<uint8_t>(self, 0x68) = At<uint8_t>(self, 0x6A);
    At<uint8_t>(self, 0x65) = 0;
    if (g_widescreen != 0 || Transition_IsActive(transition) != 0 || At<uint8_t>(Set(self), 0x70)) {
        return;
    }
    float side = 0.0f;
    if (!altSteering || Pad_Fn738500(GetPlayerPadInput())) {
        side = Pad_GetValue(0x12, 0);
    }
    At<uint8_t>(self, 0x65) = Pad_GetValue(0xE, 0) >= 0.01f ? 1 : 0;   // (NaN: not)
    At<uint8_t>(self, 0x69) = side < -0.5f ? 1 : 0;
    At<uint8_t>(self, 0x6A) = side > 0.5f ? 1 : 0;
}
VEHICLECAM_REVERSED("VehicleCameraController::ReadInput_50A970", 0x0050A970, RE_BODY(&VehicleCam_ReadInputOurs), nullptr, nullptr, re::Mode::Original);

// 0x50AAA0: high above the ground: the player more than the settings' +0x48 above the
// ground under it (-1e11 when there's none)
void __fastcall VehicleCam_UpdateHeightOurs(uint8_t* self, re::Edx) {
    if (!playerPed) {
        return;
    }
    CVector position = *EntityPosition(playerPed);
    float height = -99999997952.0f;
    uint8_t found = 0;
    float ground = FindGroundZ(position.x, position.y, float(double(position.z) + 1.0), &found, 1);
    if (found) {
        height = position.z - ground;
    }
    At<uint8_t>(self, 0x64) = At<float>(cameraSettings, 0x48) < height ? 1 : 0;
}
VEHICLECAM_REVERSED("VehicleCameraController::UpdateHeight_50AAA0", 0x0050AAA0, RE_BODY(&VehicleCam_UpdateHeightOurs), nullptr, nullptr, re::Mode::Original);

// 0x50AB70: the camera put there looking at the look-at, the changes and look-at noted
// in TheCamera, the fov (unless locked)
void __fastcall VehicleCam_ApplyOurs(uint8_t* self, re::Edx) {
    Camera_LookAtPoint(theCamera, &At<CVector>(self, 0x1C), &At<CVector>(self, 0x34));
    Camera_Fn4F0920(theCamera, &At<CVector>(self, 0x28));
    cameraUnknown18C = At<CVector>(self, 0x40);
    cameraLookAt = At<CVector>(self, 0x34);
    if (!cameraFovLocked) {
        cameraFovOverridden = 0;
        cameraFov = At<float>(self, 0xAC);
    }
}
VEHICLECAM_REVERSED("VehicleCameraController::Apply_50AB70", 0x0050AB70, RE_BODY(&VehicleCam_ApplyOurs), nullptr, nullptr, re::Mode::Original);

// 0x50ABF0: the speed's share, the fov easing (with the start shake for the first
// second), and the distance eased between the set's near and far by the speed share
void __fastcall VehicleCam_UpdateEasingOurs(uint8_t* self, re::Edx) {
    float speed = 0.0f;
    if (playerPed && At<uint8_t*>(playerPed, 0x1554)) {
        speed = Length(At<CVector>(At<uint8_t*>(playerPed, 0x1554), 0x124)) * g_A459F0;
    }
    float* share = &At<float>(self, 0x70);
    share[0] = speed;
    share[1] = At<float>(Set(self), 0x68);
    share[2] = At<float>(Set(self), 0x6C);
    SpeedShare_Update(share);
    float shake = 1.0f;
    uint32_t started = At<uint32_t>(self, 0x58);
    if (timeInMs < started + uint32_t(g_A459EC)) {
        // (the x87 at 24 bits: the elapsed time loaded signed, 2^32 added (rounded to float)
        // when negative, divided by the int; then each step rounded to float)
        int32_t elapsed = int32_t(timeInMs - started);
        double since = elapsed < 0 ? double(float(double(elapsed) + 4294967296.0)) : double(elapsed);
        float t = float(since / double(g_A459EC));
        float angle = (t - 0.5f) * float(kPi);
        float s = float(std::sin(double(angle)));
        float half = (s + 1.0f) * 0.5f;
        shake = half * g_A459E8;
    }
    float* fov = &At<float>(self, 0x8C);
    fov[0] = cameraFovOverridden == 0 ? cameraFov : cameraFovOverride;
    fov[1] = At<float>(self, 0x7C);
    fov[2] = At<float>(self, 0x84) - At<float>(self, 0x88);
    fov[3] = At<float>(Set(self), 0x60);
    fov[4] = At<float>(Set(self), 0x64);
    fov[5] = At<float>(self, 0x54) * shake;
    FovEasing_Update(fov);
    At<float>(self, 0xBC) = At<float>(self, 0x7C);
    At<float>(self, 0xC0) = At<float>(self, 0xA8);
    At<float>(self, 0xC4) = At<float>(Set(self), 0x18);
    At<float>(self, 0xC8) = At<float>(Set(self), 0x1C);
    At<float>(self, 0xCC) = At<float>(self, 0x54);
    float wanted = (1.0f - At<float>(self, 0xBC)) * At<float>(self, 0xC4) + At<float>(self, 0xC8) * At<float>(self, 0xBC);
    float rate = At<float>(self, 0xCC) * At<float>(self, 0xC0);
    At<float>(self, 0xD0) = (wanted - At<float>(self, 0xD0)) * rate + At<float>(self, 0xD0);
}
VEHICLECAM_REVERSED("VehicleCameraController::UpdateEasing_50ABF0", 0x0050ABF0, RE_BODY(&VehicleCam_UpdateEasingOurs), nullptr, nullptr, re::Mode::Original);

// the frame's steps (0x50AE30, 0x50AE90)
void Step(uint8_t* self) {
    VehicleCam_ReadInput(self);
    VehicleCam_UpdateHeight(self);
    VehicleCam_BlendButes(self, 0.0f);
    VehicleCam_UpdateEasing(self);
    VehicleCam_UpdateDirection(self);
    VehicleCam_UpdateLookAt(self);
    VehicleCam_UpdatePosition(self);
}

// 0x50AE30 (vtable[7]): the frame (with the player in a vehicle)
void __fastcall VehicleCam_ProcessOurs(uint8_t* self, re::Edx) {
    if (!playerPed || !At<uint8_t*>(playerPed, 0x1554)) {
        return;
    }
    At<uint8_t>(self, 0x6B) = 0;
    At<uint8_t>(self, 0x6C) = 0;
    Step(self);
    VehicleCam_Apply(self);
}
VEHICLECAM_REVERSED("VehicleCameraController::Process_50AE30", 0x0050AE30, RE_BODY(&VehicleCam_ProcessOurs), nullptr, nullptr, re::Mode::Original);

// 0x50AE90: a frame with the time step given, without applying it; the result kept
// in +0xD4..+0xF8
void __fastcall VehicleCam_StepOurs(uint8_t* self, re::Edx, float step) {
    if (!playerPed || !At<uint8_t*>(playerPed, 0x1554)) {
        return;
    }
    At<uint8_t>(self, 0x6B) = 0;
    At<float>(self, 0x54) = step;
    At<uint8_t>(self, 0x6C) = 1;
    Step(self);
    At<float>(self, 0xEC) = At<float>(self, 0xAC);
    At<float>(self, 0xF0) = At<float>(self, 0xD0);
    At<CVector>(self, 0xD4) = At<CVector>(self, 0x1C);
    At<float>(self, 0xF4) = At<float>(self, 0x7C);
    At<CVector>(self, 0xE0) = At<CVector>(self, 0x34);
    At<uint8_t>(self, 0xF8) = At<uint8_t>(self, 0x65);
}
VEHICLECAM_REVERSED("VehicleCameraController::Step_50AE90", 0x0050AE90, RE_BODY(&VehicleCam_StepOurs), nullptr, nullptr, re::Mode::Original);

// 0x50AF60: the constructor (its blended set allocated, set to the defaults, a
// reload callback registered)
void* __fastcall VehicleCam_ConstructOurs(uint8_t* self, re::Edx) {
    for (size_t offset = 4; offset <= 0x48; offset += 4) {
        At<float>(self, offset) = 0.0f;
    }
    At<uint32_t>(self, 0) = kVehicleCameraVtable;
    for (size_t offset : {0x100, 0x104, 0xFC, 0x10C, 0x110, 0x114, 0x118}) {
        At<int32_t>(self, offset) = 0;
    }
    for (size_t offset = 0xD4; offset <= 0xE8; offset += 4) {
        At<float>(self, offset) = 0.0f;
    }
    void* butes = Malloc(0x88);
    if (butes) {
        Track_Construct(butes);
        At<uint32_t>(butes, 0) = kVehicleCamButesVtable;
    }
    At<void*>(self, 0x108) = butes;
    VehicleCamButes_SetDefaults(butes);   // (with nullptr when the allocation failed)
    AddLoadCallback(reloadCallback, self);
    return self;
}
VEHICLECAM_REVERSED("VehicleCameraController::VehicleCameraController_50AF60", 0x0050AF60, RE_BODY(&VehicleCam_ConstructOurs), nullptr, nullptr, re::Mode::Original);

// 0x50B090 (vtable[4]): start from the view as it is (the camera, TheCamera's
// look-at and its change, dropped when over 2)
void __fastcall VehicleCam_StartFromViewOurs(uint8_t* self, re::Edx) {
    NiTransform* t = GetLocalTransform(GetCameraNode(theNiCamera));
    At<CVector>(self, 0x1C) = t->translate;
    At<CVector>(self, 0x28) = cameraUnknown180;
    At<CVector>(self, 0x40) = cameraUnknown18C;
    At<CVector>(self, 0x34) = cameraLookAt;
    if (Length(At<CVector>(self, 0x40)) > 2.0f) {
        At<CVector>(self, 0x40) = {0.0f, 0.0f, 0.0f};
    }
    At<float>(self, 0x4C) = 0.0f;
    At<uint8_t>(self, 0x6B) = 0;
    At<float>(self, 0x50) = 0.0f;
    At<uint32_t>(self, 0x58) = timeInMs;
    At<float>(self, 0x70) = 0.0f;
    At<float>(self, 0x80) = 0.0f;
    At<float>(self, 0x84) = 0.0f;
    At<float>(self, 0x88) = 0.0f;
    VehicleCam_UpdateHeight(self);
    VehicleCam_BlendButes(self, 1.0f);
    At<float>(self, 0x90) = 0.0f;
    At<float>(self, 0xA8) = 1.0f;
    At<float>(self, 0xA4) = 0.0f;
    At<float>(self, 0x5C) = At<float>(self, 0x24) - At<float>(self, 0x3C);
    At<float>(self, 0x60) = 1.0f;
    CVector d;
    CVector* dir = &At<CVector>(self, 0x10);
    *dir = *VectorSub(&d, &At<CVector>(self, 0x34), &At<CVector>(self, 0x1C));
    dir->z = 0.0f;
    float sq = std::fabs(float((double(dir->x) * dir->x + double(dir->y) * dir->y) + double(dir->z) * dir->z));
    Vector_Normalise(dir);
    At<float>(self, 0xBC) = 0.0f;
    At<float>(self, 0xC0) = 1.0f;
    At<float>(self, 0xC4) = At<float>(Set(self), 0x18);
    At<float>(self, 0xC8) = At<float>(Set(self), 0x1C);
    At<float>(self, 0xCC) = 0.0f;
    At<float>(self, 0xD0) = float(std::sqrt(double(sq)));
    At<float>(self, 0xC0) = 1.0f;
    At<float>(self, 0xBC) = 0.0f;
    float wanted = float(1.0 * At<float>(self, 0xC4) + double(At<float>(self, 0xC8)) * 0.0);
    At<float>(self, 0xD0) = float((double(wanted) - At<float>(self, 0xD0)) * At<float>(self, 0xCC) + At<float>(self, 0xD0));
    VehicleCam_ReadInput(self);
    At<uint8_t>(self, 0x66) = At<uint8_t>(self, 0x65);
}
VEHICLECAM_REVERSED("VehicleCameraController::StartFromView_50B090", 0x0050B090, RE_BODY(&VehicleCam_StartFromViewOurs), nullptr, nullptr, re::Mode::Original);

// 0x50B2F0: start behind the player's vehicle (the fov and distance snapped to their
// targets)
void __fastcall VehicleCam_StartBehindOurs(uint8_t* self, re::Edx) {
    if (!playerPed) {
        return;
    }
    At<uint8_t>(self, 0x6B) = 0;
    At<float>(self, 0x5C) = At<float>(Set(self), 0x2C);
    At<float>(self, 0x60) = 1.0f;
    At<CVector>(self, 0x28) = {0.0f, 0.0f, 0.0f};
    At<float>(self, 0x4C) = 0.1f;
    At<float>(self, 0x50) = 0.1f;
    At<CVector>(self, 0x40) = {0.0f, 0.0f, 0.0f};
    At<uint32_t>(self, 0x58) = timeInMs;
    At<float>(self, 0x70) = 0.0f;
    At<float>(self, 0x80) = 0.0f;
    At<float>(self, 0x84) = 0.0f;
    At<float>(self, 0x88) = 0.0f;
    VehicleCam_UpdateHeight(self);
    float* fov = &At<float>(self, 0x8C);
    fov[0] = cameraFovOverridden == 0 ? cameraFov : cameraFovOverride;
    fov[1] = At<float>(self, 0x7C);
    fov[2] = At<float>(self, 0x84) - At<float>(self, 0x88);
    fov[3] = At<float>(Set(self), 0x60);
    fov[4] = At<float>(Set(self), 0x64);
    fov[5] = 0.0001f;
    fov[1] = 0.0f;
    fov[7] = 1.0f;
    fov[6] = 0.0f;
    FovEasing_Update(fov);
    fov[0] = fov[9];
    fov[8] = fov[9];
    float nearDistance = At<float>(Set(self), 0x18);
    float farDistance = At<float>(Set(self), 0x1C);
    At<float>(self, 0xBC) = 0.0f;
    At<float>(self, 0xC0) = 1.0f;
    At<float>(self, 0xC4) = nearDistance;
    At<float>(self, 0xC8) = farDistance;
    At<float>(self, 0xCC) = 0.0f;
    At<float>(self, 0xBC) = 0.0f;
    At<float>(self, 0xC0) = 1.0f;
    At<float>(self, 0xD0) = At<float>(self, 0xC4);
    CVector up{0.0f, 0.0f, At<float>(Set(self), 0x28)};
    CVector sum;
    At<CVector>(self, 0x34) = *VectorAdd(&sum, EntityPosition(playerPed), &up);
    CVector behind;
    At<CVector>(self, 0x10) = *VehicleCam_GetDirection(self, &behind);
    float back = -At<float>(self, 0xD0);
    CVector part{At<float>(self, 0x10) * back, At<float>(self, 0x14) * back, back * At<float>(self, 0x18)};
    At<CVector>(self, 0x1C) = *VectorAdd(&sum, &At<CVector>(self, 0x34), &part);
    At<float>(self, 0x24) = At<float>(Set(self), 0x2C) + At<float>(self, 0x24);
}
VEHICLECAM_REVERSED("VehicleCameraController::StartBehind_50B2F0", 0x0050B2F0, RE_BODY(&VehicleCam_StartBehindOurs), nullptr, nullptr, re::Mode::Original);

} // namespace
