#include "game/Camera.h"

#include <cstring>
#include "reversed.h"

// Camera part 2: the follow camera's pieces, the script camera and its splines, the
// store camera, and the input side (UserInputManager, PlayerUserInputHandler).

#define CAMERA_REVERSED(...) REVERSED(__VA_ARGS__)

namespace {

GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);           // CTimer's
GAME_VAR(uint8_t*, cameraSettings, 0x020C88B8);     // +0x6C the default fov (int)
GAME_VAR(void*, playerPed, 0x00C1AEA8);             // FindPlayerPed
GAME_VAR(void*, storeCameraController, 0x00C3D2A4); // the store camera, while there is one
GAME_VAR(uint8_t, g_20C32E9, 0x020C32E9);
GAME_VAR(uint8_t, g_20C32EA, 0x020C32EA);
GAME_VAR(uint8_t*, g_C674E0, 0x00C674E0);
// the button mapping (+0: the XInput masks of the 16 buttons)
inline uint8_t* const buttonMap = reinterpret_cast<uint8_t*>(0x00A14810);
inline const int32_t* const buttonMasks = reinterpret_cast<const int32_t*>(0x00A14810);
// 0x50BCC0's table: 1 for the keys (1..0xD3) that can't be bound
inline const uint8_t* const unbindableKeys = reinterpret_cast<const uint8_t*>(0x0050BCF4);

GAME_FN(void, __thiscall, EmptyThiscall, 0x0049A650, void* self);
GAME_FN(void, __cdecl, ForEachOwned, 0x0072B8B0, void* fn, void* object);
GAME_FN(uint8_t, __thiscall, Ped_Unknown4CE690, 0x004CE690, void* ped);
GAME_FN(void, __thiscall, FollowCameraFight_Unknown500560, 0x00500560, void* self, int value);
GAME_FN(void, __thiscall, FollowCameraFight_Reset, 0x004FE640, void* self);
GAME_FN(void, __thiscall, FollowCameraVehicle_Unknown4FE010, 0x004FE010, void* self, int value);
GAME_FN(void, __thiscall, FollowCameraVehicle_Unknown4FE000, 0x004FE000, void* self);
GAME_FN(void, __thiscall, FollowInterp_Unknown4FB4D0, 0x004FB4D0, void* self);
GAME_FN(void, __thiscall, FollowCameraBase_Destruct, 0x004FE0C0, void* self);
GAME_FN(void, __thiscall, FollowCameraController3_Unknown5044A0, 0x005044A0, void* self, cam::CVector a, cam::CVector b, int c);
GAME_FN(void, __thiscall, StoreCameraController_Reset, 0x005093D0, void* self);
GAME_FN(void, __thiscall, SplineInterpolater_Destruct, 0x005083E0, SplineInterpolater* self);
GAME_FN(void, __thiscall, SplineInterpolater_Clear, 0x00508420, SplineInterpolater* self);
GAME_FN(void, __thiscall, SplineInterpolater_Unknown508930, 0x00508930, SplineInterpolater* self, uintptr_t value);
GAME_FN(void, __thiscall, SplineInterpolater_Set, 0x00509240, SplineInterpolater* self, int value);
GAME_FN(void, __thiscall, SplineInterpolater_Set2C, 0x005083A0, SplineInterpolater* self, float value);
GAME_FN(void, __thiscall, SplineInterpolater_Set30, 0x005083C0, SplineInterpolater* self, float value);
GAME_FN(void, __thiscall, SplineInterpolater_Set34, 0x005083D0, SplineInterpolater* self, float value);
GAME_FN(void, __thiscall, ScriptCameraController_Clear, 0x00507670, void* self);
GAME_FN(void, __thiscall, ScriptCameraController_Unknown507970, 0x00507970, void* self, uintptr_t a, int b);
// input
GAME_FN(void, __cdecl, InputHandler_Release, 0x005B0900, void* handler);
GAME_FN(uint8_t, __cdecl, IsGamePaused43A680, 0x0043A680);
GAME_FN(float, __cdecl, Input_Unknown738840, 0x00738840, int a, int b);
GAME_FN(float, __cdecl, Input_Unknown738AD0, 0x00738AD0, int a, int b);
GAME_FN(uint8_t, __cdecl, Input_Unknown738880, 0x00738880, int a, int b, int c);
GAME_FN(const int32_t*, __thiscall, ButtonMap_GetTable, 0x00454CD0, void* map, int flag);
GAME_FN(float, __thiscall, PlayerInput_GetValue, 0x0050C030, void* self, int key, float* axes);
GAME_FN(void, __thiscall, PlayerInput_SetValue, 0x0050C3C0, void* self, int key, float value, float* axes);
// (XInputSetState's import thunk)
GAME_FN(uint32_t, __stdcall, XInputSetState_, 0x0085AB8A, uint32_t user, void* vibration);

constexpr uintptr_t kCameraControllerVtable = 0x00914160;

using re::At;

template <typename R, typename... A>
R VCall(void* object, int index, A... args) {
    auto fn = reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(object))[index]));
    return fn(object, args...);
}

int DefaultFov() {
    return At<int32_t>(cameraSettings, 0x6C);
}

// ---- the follow camera's value easing (0x18 bytes)

struct FollowInterp {
    float from;          // +0
    float current;       // +4
    float target;        // +8
    uint32_t startTime;  // +0xC (CTimer ms; 0: not moving)
    int32_t duration;    // +0x10 ms
    uint8_t unknown14;   // +0x14
    uint8_t pad[3];
};
static_assert(sizeof(FollowInterp) == 0x18);

// 0x4FB360
FollowInterp* __fastcall FollowInterp_Construct(FollowInterp* self, re::Edx) {
    self->from = 0.0f;
    self->startTime = 0;
    self->current = 0.0f;
    self->duration = 200;
    self->target = 0.0f;
    self->unknown14 = 1;
    return self;
}
CAMERA_REVERSED("FollowInterp::FollowInterp", 0x004FB360, RE_BODY(&FollowInterp_Construct), nullptr, nullptr, re::Mode::Original);

// 0x4FB380: jumps to a value
void __fastcall FollowInterp_Set(FollowInterp* self, re::Edx, float value) {
    self->startTime = 0;
    self->current = value;
}
CAMERA_REVERSED("FollowInterp::Set", 0x004FB380, RE_BODY(&FollowInterp_Set), nullptr, nullptr, re::Mode::Original);

// 0x4FB3B0
uint8_t __fastcall FollowInterp_IsRising(FollowInterp* self, re::Edx) {
    if (self->startTime == 0) {
        return 0;
    }
    return self->current < self->target;
}
CAMERA_REVERSED("FollowInterp::IsRising", 0x004FB3B0, RE_BODY(&FollowInterp_IsRising), nullptr, nullptr, re::Mode::Original);

// 0x4FB3E0
int32_t __fastcall FollowInterp_IsMoving(FollowInterp* self, re::Edx) {
    return self->current != self->target ? 1 : 0;
}
CAMERA_REVERSED("FollowInterp::IsMoving", 0x004FB3E0, RE_BODY(&FollowInterp_IsMoving), nullptr, nullptr, re::Mode::Original);

// 0x4FB4A0: eases from where it is to `target` over `duration` ms
void __fastcall FollowInterp_Start(FollowInterp* self, re::Edx, float target, int duration, int flag) {
    self->from = self->current;
    self->startTime = timeInMs;
    self->target = target;
    self->duration = duration;
    self->unknown14 = uint8_t(flag);
}
CAMERA_REVERSED("FollowInterp::Start", 0x004FB4A0, RE_BODY(&FollowInterp_Start), nullptr, nullptr, re::Mode::Original);

// ---- FollowCameraBase / FollowCameraController3
// FollowCameraBase: +0x160 a FollowCameraFight (vtable 0x9145EC), +0x278 a
// FollowCameraVehicle (0x9145F4: +4 the controller it drives, +8 a flag).
// FollowCameraController3 adds +0x518 its attribute set, +0x520/+0x524.

// 0x4FB570 (vtable[2])
void __fastcall FollowCameraController3_Vf2(void* self, re::Edx) {
    At<int32_t>(self, 0x520) = 0;
    At<int32_t>(self, 0x524) = 0;
    EmptyThiscall(static_cast<uint8_t*>(self) + 0x160);
    FollowCameraVehicle_Unknown4FE000(static_cast<uint8_t*>(self) + 0x278);
}
CAMERA_REVERSED("FollowCameraController3::vf2", 0x004FB570, RE_BODY(&FollowCameraController3_Vf2), nullptr, nullptr, re::Mode::Original);

// 0x4FB6D0: the fight part's 0x500560
void __fastcall FollowCameraBase_Fight500560(void* self, re::Edx, int value) {
    FollowCameraFight_Unknown500560(static_cast<uint8_t*>(self) + 0x160, value);
}
CAMERA_REVERSED("FollowCameraBase::Fight500560", 0x004FB6D0, RE_BODY(&FollowCameraBase_Fight500560), nullptr, nullptr, re::Mode::Original);

// 0x4FB6E0: the vehicle part's 0x4FE010
void __fastcall FollowCameraBase_Vehicle4FE010(void* self, re::Edx, int value) {
    FollowCameraVehicle_Unknown4FE010(static_cast<uint8_t*>(self) + 0x278, value);
}
CAMERA_REVERSED("FollowCameraBase::Vehicle4FE010", 0x004FB6E0, RE_BODY(&FollowCameraBase_Vehicle4FE010), nullptr, nullptr, re::Mode::Original);

// 0x4FB6F0: what the follow camera's destructor does to what it owns
void __cdecl FollowCameraController3_ReleaseOwned(void* object) {
    VCall<void>(object, 2);
    VCall<void>(object, 1);
    FollowInterp_Unknown4FB4D0(object);
}
CAMERA_REVERSED("FollowCameraController3::ReleaseOwned", 0x004FB6F0, &FollowCameraController3_ReleaseOwned, nullptr, nullptr, re::Mode::Original);

// 0x4FBCE0: its attribute set's +0x24
float __fastcall FollowCameraController3_GetButes24(void* self, re::Edx) {
    return At<float>(At<void*>(self, 0x518), 0x24);
}
CAMERA_REVERSED("FollowCameraController3::GetButes24", 0x004FBCE0, RE_BODY(&FollowCameraController3_GetButes24), nullptr, nullptr, re::Mode::Original);

// 0x4FD170 (vtable[0])
void* __fastcall FollowCameraController3_Delete(void* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = 0x00914598;
    ForEachOwned(reinterpret_cast<void*>(0x004FB6F0), self);
    if (void* butes = At<void*>(self, 0x518)) {
        VCall<void>(butes, 1, 1);
    }
    FollowCameraBase_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}
CAMERA_REVERSED("FollowCameraController3::vf0", 0x004FD170, RE_BODY(&FollowCameraController3_Delete), nullptr, nullptr, re::Mode::Original);

// 0x4FE0C0
void __fastcall FollowCameraBase_DestructOurs(void* self, re::Edx) {
    At<uint32_t>(self, 0) = 0x00914600;
    At<uint32_t>(self, 0x278) = 0x009145F4;
    if (void* driven = At<void*>(self, 0x27C)) {
        VCall<void>(driven, 0, 1);
    }
    At<void*>(self, 0x27C) = nullptr;
    At<uint32_t>(self, 0x160) = 0x009145EC;
    At<uint32_t>(self, 0) = kCameraControllerVtable;
}
CAMERA_REVERSED("FollowCameraBase::~FollowCameraBase", 0x004FE0C0, RE_BODY(&FollowCameraBase_DestructOurs), nullptr, nullptr, re::Mode::Original);

// 0x500360 (vtable[0])
void* __fastcall FollowCameraBase_Delete(void* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = 0x00914600;
    At<uint32_t>(self, 0x278) = 0x009145F4;
    if (void* driven = At<void*>(self, 0x27C)) {
        VCall<void>(driven, 0, 1);
    }
    At<void*>(self, 0x27C) = nullptr;
    At<uint32_t>(self, 0x160) = 0x009145EC;
    At<uint32_t>(self, 0) = kCameraControllerVtable;
    if (flags & 1) {
        Free(self);
    }
    return self;
}
CAMERA_REVERSED("FollowCameraBase::vf0", 0x00500360, RE_BODY(&FollowCameraBase_Delete), nullptr, nullptr, re::Mode::Original);

// 0x4FDEF0: +0x90 (a vector; out returned)
cam::CVector* __fastcall FollowCameraController3_Get90(void* self, re::Edx, cam::CVector* out) {
    *out = At<cam::CVector>(self, 0x90);
    return out;
}
CAMERA_REVERSED("FollowCameraController3::Get90", 0x004FDEF0, RE_BODY(&FollowCameraController3_Get90), nullptr, nullptr, re::Mode::Original);

// 0x504A00: 0x5044A0 with the same arguments
void __fastcall FollowCameraController3_Call5044A0(void* self, re::Edx, cam::CVector a, cam::CVector b, int c) {
    FollowCameraController3_Unknown5044A0(self, a, b, c);
}
CAMERA_REVERSED("FollowCameraController3::Call5044A0", 0x00504A00, RE_BODY(&FollowCameraController3_Call5044A0), nullptr, nullptr, re::Mode::Original);

// 0x504E90: a 0x20-byte record's fields (whose: not pinned down)
void __fastcall Unknown504E90_Set(void* self, re::Edx, float a, float b, float c, float d, int e, int f, int g) {
    At<float>(self, 0x14) = a;
    At<int32_t>(self, 0) = e;
    At<float>(self, 0x18) = c;
    At<int32_t>(self, 4) = f;
    At<float>(self, 0xC) = b;
    At<uint8_t>(self, 0x1C) = 1;
    At<int32_t>(self, 8) = g;
    At<float>(self, 0x10) = d;
}
CAMERA_REVERSED("Unknown504E90::Set", 0x00504E90, RE_BODY(&Unknown504E90_Set), nullptr, nullptr, re::Mode::Original);

// ---- FollowCameraVehicle / FollowCameraFight

// 0x4FE000: the driven controller's vtable[2]
void __fastcall FollowCameraVehicle_Vf2Driven(void* self, re::Edx) {
    VCall<void>(At<void*>(self, 4), 2);
}
CAMERA_REVERSED("FollowCameraVehicle::Vf2Driven", 0x004FE000, RE_BODY(&FollowCameraVehicle_Vf2Driven), nullptr, nullptr, re::Mode::Original);

// 0x4FE010: the driven controller's vtable[8]
void __fastcall FollowCameraVehicle_Vf8Driven(void* self, re::Edx, int value) {
    VCall<void>(At<void*>(self, 4), 8, value);
}
CAMERA_REVERSED("FollowCameraVehicle::Vf8Driven", 0x004FE010, RE_BODY(&FollowCameraVehicle_Vf8Driven), nullptr, nullptr, re::Mode::Original);

// 0x4FE020: starts the driven controller (its vtable[4]) once the player is in a
// vehicle of kind 1 (or kind 0, when 0x4CE690 says so)
void __fastcall FollowCameraVehicle_Update(void* self, re::Edx) {
    void* ped = playerPed;
    if (ped) {
        void* vehicle = At<void*>(ped, 0x1554);
        if (vehicle) {
            int kind = At<int32_t>(vehicle, 0x46C);
            if (kind == 1 || (kind == 0 && Ped_Unknown4CE690(ped))) {
                if (At<uint8_t>(self, 8) != 0) {
                    return;
                }
                At<uint8_t>(self, 8) = 1;
                VCall<void>(At<void*>(self, 4), 4);
                return;
            }
        }
        if (At<uint8_t>(self, 8) == 0) {
            return;
        }
    }
    At<uint8_t>(self, 8) = 0;
}
CAMERA_REVERSED("FollowCameraVehicle::Update", 0x004FE020, RE_BODY(&FollowCameraVehicle_Update), nullptr, nullptr, re::Mode::Original);

// 0x4FE070
void* __fastcall FollowCameraVehicle_GetDrivenD4(void* self, re::Edx) {
    return static_cast<uint8_t*>(At<void*>(self, 4)) + 0xD4;
}
CAMERA_REVERSED("FollowCameraVehicle::GetDrivenD4", 0x004FE070, RE_BODY(&FollowCameraVehicle_GetDrivenD4), nullptr, nullptr, re::Mode::Original);

// 0x4FFDE0 (vtable[0])
void* __fastcall FollowCameraVehicle_Delete(void* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = 0x009145F4;
    if (void* driven = At<void*>(self, 4)) {
        VCall<void>(driven, 0, 1);
    }
    At<void*>(self, 4) = nullptr;
    if (flags & 1) {
        Free(self);
    }
    return self;
}
CAMERA_REVERSED("FollowCameraVehicle::vf0", 0x004FFDE0, RE_BODY(&FollowCameraVehicle_Delete), nullptr, nullptr, re::Mode::Original);

// 0x4FE620 (vtable[0])
void* __fastcall FollowCameraFight_Delete(void* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = 0x009145EC;
    if (flags & 1) {
        Free(self);
    }
    return self;
}
CAMERA_REVERSED("FollowCameraFight::vf0", 0x004FE620, RE_BODY(&FollowCameraFight_Delete), nullptr, nullptr, re::Mode::Original);

// 0x500630
void __fastcall FollowCameraFight_Clear(void* self, re::Edx) {
    At<int32_t>(self, 0x50) = 0;
    At<uint8_t>(self, 0x54) = 0;
    At<int32_t>(self, 0x58) = -1;
    FollowCameraFight_Reset(self);
}
CAMERA_REVERSED("FollowCameraFight::Clear", 0x00500630, RE_BODY(&FollowCameraFight_Clear), nullptr, nullptr, re::Mode::Original);

// 0x4FE080: 32 vectors and four fields cleared (a position history; whose: not
// pinned down)
void* __fastcall PositionHistory_Construct(void* self, re::Edx) {
    for (int i = 0; i < 32; i++) {
        At<cam::CVector>(self, i * 0xC) = {0.0f, 0.0f, 0.0f};
    }
    At<float>(self, 0x208) = 0.0f;
    At<int32_t>(self, 0x200) = 0;
    At<int32_t>(self, 0x204) = 0;
    At<int32_t>(self, 0x20C) = 0;
    return self;
}
CAMERA_REVERSED("PositionHistory::PositionHistory", 0x004FE080, RE_BODY(&PositionHistory_Construct), nullptr, nullptr, re::Mode::Original);

// ---- ScriptCameraController

struct ScriptCameraController {
    re::Ptr32<void> vtable;                    // +0 (0x9146D0)
    int32_t unknown4;                // +4
    float unknown8;                  // +8
    float unknownC;                  // +0xC
    int32_t unknown10;               // +0x10
    uint8_t unknown14[0x28];         // +0x14
    SplineInterpolater splineA;      // +0x3C
    int32_t hasTarget;               // +0x8C
    cam::CVector target;             // +0x90
    int32_t unknown9C;               // +0x9C
    int32_t unknownA0;               // +0xA0
    SplineInterpolater splineB;      // +0xA4
    int32_t unknownF4;               // +0xF4
    float unknownF8;                 // +0xF8
    float unknownFC;                 // +0xFC
    uint8_t unknown100;              // +0x100
    uint8_t unknown101[3];           // +0x101
    float fov;                       // +0x104
};
static_assert(offsetof(ScriptCameraController, splineA) == 0x3C);
static_assert(offsetof(ScriptCameraController, target) == 0x90);
static_assert(offsetof(ScriptCameraController, splineB) == 0xA4);
static_assert(offsetof(ScriptCameraController, fov) == 0x104);

// 0x507660
void __fastcall ScriptCameraController_ResetFov(ScriptCameraController* self, re::Edx) {
    self->fov = float(DefaultFov());
}
CAMERA_REVERSED("ScriptCameraController::ResetFov", 0x00507660, RE_BODY(&ScriptCameraController_ResetFov), nullptr, nullptr, re::Mode::Original);

// 0x507670
void __fastcall ScriptCameraController_ClearOurs(ScriptCameraController* self, re::Edx) {
    self->unknownC = 0.0f;
    self->unknownF8 = 0.0f;
    self->unknown4 = 1;
    self->unknownF4 = 0;
    self->target = {0.0f, 0.0f, 0.0f};
    self->hasTarget = 0;
    self->unknown9C = -1;
    self->unknownA0 = -1;
    SplineInterpolater_Clear(&self->splineA);
    self->unknownFC = 0.0f;
    self->unknown100 = 0;
}
CAMERA_REVERSED("ScriptCameraController::Clear", 0x00507670, RE_BODY(&ScriptCameraController_ClearOurs), nullptr, nullptr, re::Mode::Original);

// 0x507700: looks at a point
void __fastcall ScriptCameraController_SetTarget(ScriptCameraController* self, re::Edx, const cam::CVector* target, int value) {
    ScriptCameraController_Clear(self);
    self->hasTarget = 1;
    self->target.x = target->x;
    self->target.y = target->y;
    self->unknownC = 0.0f;
    self->unknownFC = 0.0f;
    self->target.z = target->z;
    self->unknown4 = value;
    self->unknown100 = 0;
}
CAMERA_REVERSED("ScriptCameraController::SetTarget", 0x00507700, RE_BODY(&ScriptCameraController_SetTarget), nullptr, nullptr, re::Mode::Original);

// 0x507750
void __fastcall ScriptCameraController_Set9C(ScriptCameraController* self, re::Edx, int a, int b, int value, float fc) {
    ScriptCameraController_Clear(self);
    self->unknownC = 0.0f;
    self->unknownFC = fc;
    self->unknown9C = a;
    self->unknownA0 = b;
    self->unknown4 = value;
    self->unknown100 = 0;
}
CAMERA_REVERSED("ScriptCameraController::Set9C", 0x00507750, RE_BODY(&ScriptCameraController_Set9C), nullptr, nullptr, re::Mode::Original);

// 0x507790: along a spline
void __fastcall ScriptCameraController_SetSpline(ScriptCameraController* self, re::Edx, int spline, int value) {
    ScriptCameraController_Clear(self);
    SplineInterpolater_Set(&self->splineA, spline);
    self->unknown4 = value;
    self->unknown100 = 0;
}
CAMERA_REVERSED("ScriptCameraController::SetSpline", 0x00507790, RE_BODY(&ScriptCameraController_SetSpline), nullptr, nullptr, re::Mode::Original);

// 0x5077C0 / 0x5077E0 / 0x507800: the first spline's setters
void __fastcall ScriptCameraController_SplineASet34(ScriptCameraController* self, re::Edx, float value) {
    SplineInterpolater_Set34(&self->splineA, value);
}
CAMERA_REVERSED("ScriptCameraController::SplineASet34", 0x005077C0, RE_BODY(&ScriptCameraController_SplineASet34), nullptr, nullptr, re::Mode::Original);

void __fastcall ScriptCameraController_SplineASet2C(ScriptCameraController* self, re::Edx, float value) {
    SplineInterpolater_Set2C(&self->splineA, value);
}
CAMERA_REVERSED("ScriptCameraController::SplineASet2C", 0x005077E0, RE_BODY(&ScriptCameraController_SplineASet2C), nullptr, nullptr, re::Mode::Original);

void __fastcall ScriptCameraController_SplineASet30(ScriptCameraController* self, re::Edx, float value) {
    SplineInterpolater_Set30(&self->splineA, value);
}
CAMERA_REVERSED("ScriptCameraController::SplineASet30", 0x00507800, RE_BODY(&ScriptCameraController_SplineASet30), nullptr, nullptr, re::Mode::Original);

// 0x507820
void __fastcall ScriptCameraController_Set8(ScriptCameraController* self, re::Edx, float value) {
    self->unknown8 = value;
}
CAMERA_REVERSED("ScriptCameraController::Set8", 0x00507820, RE_BODY(&ScriptCameraController_Set8), nullptr, nullptr, re::Mode::Original);

// 0x507830 / 0x507850 / 0x507870: the second spline's
void __fastcall ScriptCameraController_SplineBSet34(ScriptCameraController* self, re::Edx, float value) {
    SplineInterpolater_Set34(&self->splineB, value);
}
CAMERA_REVERSED("ScriptCameraController::SplineBSet34", 0x00507830, RE_BODY(&ScriptCameraController_SplineBSet34), nullptr, nullptr, re::Mode::Original);

void __fastcall ScriptCameraController_SplineBSet2C(ScriptCameraController* self, re::Edx, float value) {
    SplineInterpolater_Set2C(&self->splineB, value);
}
CAMERA_REVERSED("ScriptCameraController::SplineBSet2C", 0x00507850, RE_BODY(&ScriptCameraController_SplineBSet2C), nullptr, nullptr, re::Mode::Original);

void __fastcall ScriptCameraController_SplineBSet30(ScriptCameraController* self, re::Edx, float value) {
    SplineInterpolater_Set30(&self->splineB, value);
}
CAMERA_REVERSED("ScriptCameraController::SplineBSet30", 0x00507870, RE_BODY(&ScriptCameraController_SplineBSet30), nullptr, nullptr, re::Mode::Original);

// 0x507890: 0 for the default
void __fastcall ScriptCameraController_SetFov(ScriptCameraController* self, re::Edx, float fov) {
    if (fov == 0.0f) {
        fov = float(DefaultFov());
    }
    self->fov = fov;
}
CAMERA_REVERSED("ScriptCameraController::SetFov", 0x00507890, RE_BODY(&ScriptCameraController_SetFov), nullptr, nullptr, re::Mode::Original);

// 0x507930 (vtable[0])
void* __fastcall ScriptCameraController_Delete(ScriptCameraController* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(0x009146D0);
    SplineInterpolater_Destruct(&self->splineB);
    SplineInterpolater_Destruct(&self->splineA);
    self->vtable = reinterpret_cast<void*>(kCameraControllerVtable);
    if (flags & 1) {
        Free(self);
    }
    return self;
}
CAMERA_REVERSED("ScriptCameraController::vf0", 0x00507930, RE_BODY(&ScriptCameraController_Delete), nullptr, nullptr, re::Mode::Original);

// 0x507A80
void __fastcall ScriptCameraController_Start(ScriptCameraController* self, re::Edx, int a, int b, float c, int d, int e) {
    ScriptCameraController_Unknown507970(self, a, d);
    self->unknownF8 = c;
    self->unknown10 = d;
    self->unknownF4 = b;
    self->unknown100 = uint8_t(e);
}
CAMERA_REVERSED("ScriptCameraController::Start", 0x00507A80, RE_BODY(&ScriptCameraController_Start), nullptr, nullptr, re::Mode::Original);

// ---- SplineInterpolater

// 0x508380: +4 (a vector; out returned: the script camera uses it, and the 64-bit build
// crashed in chemistry class while this was void, 2026-11-01)
cam::CVector* __fastcall SplineInterpolater_Get4(SplineInterpolater* self, re::Edx, cam::CVector* out) {
    *out = *reinterpret_cast<const cam::CVector*>(self->unknown4);
    return out;
}
CAMERA_REVERSED("SplineInterpolater::Get4", 0x00508380, RE_BODY(&SplineInterpolater_Get4), nullptr, nullptr, re::Mode::Original);

// 0x5083A0
void __fastcall SplineInterpolater_Set2COurs(SplineInterpolater* self, re::Edx, float value) {
    self->unknown2C = value;
    if (value == 0.0f) {
        self->unknown38 = self->unknown34;
    }
}
CAMERA_REVERSED("SplineInterpolater::Set2C", 0x005083A0, RE_BODY(&SplineInterpolater_Set2COurs), nullptr, nullptr, re::Mode::Original);

// 0x5083C0
void __fastcall SplineInterpolater_Set30Ours(SplineInterpolater* self, re::Edx, float value) {
    self->unknown30 = value;
}
CAMERA_REVERSED("SplineInterpolater::Set30", 0x005083C0, RE_BODY(&SplineInterpolater_Set30Ours), nullptr, nullptr, re::Mode::Original);

// 0x5083D0
void __fastcall SplineInterpolater_Set34Ours(SplineInterpolater* self, re::Edx, float value) {
    self->unknown34 = value;
}
CAMERA_REVERSED("SplineInterpolater::Set34", 0x005083D0, RE_BODY(&SplineInterpolater_Set34Ours), nullptr, nullptr, re::Mode::Original);

// 0x5083E0
void __fastcall SplineInterpolater_DestructOurs(SplineInterpolater* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(0x009146FC);
    if (self->points) {
        Free(self->points);
        self->points = nullptr;
    }
}
CAMERA_REVERSED("SplineInterpolater::~SplineInterpolater", 0x005083E0, RE_BODY(&SplineInterpolater_DestructOurs), nullptr, nullptr, re::Mode::Original);

// 0x508420
void __fastcall SplineInterpolater_ClearOurs(SplineInterpolater* self, re::Edx) {
    if (self->points) {
        Free(self->points);
        self->points = nullptr;
    }
    self->unknown4C = 0;
    self->unknown48 = 0;
}
CAMERA_REVERSED("SplineInterpolater::Clear", 0x00508420, RE_BODY(&SplineInterpolater_ClearOurs), nullptr, nullptr, re::Mode::Original);

// 0x5084F0 (vtable[0])
void* __fastcall SplineInterpolater_Delete(SplineInterpolater* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(0x009146FC);
    if (self->points) {
        Free(self->points);
        self->points = nullptr;
    }
    if (flags & 1) {
        Free(self);
    }
    return self;
}
CAMERA_REVERSED("SplineInterpolater::vf0", 0x005084F0, RE_BODY(&SplineInterpolater_Delete), nullptr, nullptr, re::Mode::Original);

// 0x509240: starts over on a spline
void __fastcall SplineInterpolater_SetOurs(SplineInterpolater* self, re::Edx, int spline) {
    if (self->points) {
        Free(self->points);
        self->points = nullptr;
    }
    self->unknown48 = 0;
    self->unknown4C = 0;
    self->unknown3C = 0.0f;
    self->unknown38 = 0.0f;
    self->unknown40 = 0.0f;
    self->unknown28 = 0;
    SplineInterpolater_Unknown508930(self, spline);
}
CAMERA_REVERSED("SplineInterpolater::Set", 0x00509240, RE_BODY(&SplineInterpolater_SetOurs), nullptr, nullptr, re::Mode::Original);

// ---- StoreCameraController

// 0x509480
void __fastcall StoreCameraController_Set10(void* self, re::Edx, const cam::CVector* v) {
    At<cam::CVector>(self, 0x10) = *v;
}
CAMERA_REVERSED("StoreCameraController::Set10", 0x00509480, RE_BODY(&StoreCameraController_Set10), nullptr, nullptr, re::Mode::Original);

// 0x509570 (the game returns ecx as left by the reset: taken to be this)
void* __fastcall StoreCameraController_Construct(void* self, re::Edx) {
    At<uint32_t>(self, 0) = 0x00914708;
    for (size_t offset = 0x10; offset <= 0x48; offset += 4) {
        At<float>(self, offset) = 0.0f;
    }
    storeCameraController = self;
    StoreCameraController_Reset(self);
    return self;
}
CAMERA_REVERSED("StoreCameraController::StoreCameraController", 0x00509570, RE_BODY(&StoreCameraController_Construct), nullptr, nullptr, re::Mode::Original);

// 0x5095C0 (vtable[0])
void* __fastcall StoreCameraController_Delete(void* self, re::Edx, int flags) {
    storeCameraController = nullptr;
    At<uint32_t>(self, 0) = kCameraControllerVtable;
    if (flags & 1) {
        Free(self);
    }
    return self;
}
CAMERA_REVERSED("StoreCameraController::vf0", 0x005095C0, RE_BODY(&StoreCameraController_Delete), nullptr, nullptr, re::Mode::Original);

// ---- UserInputManager

// 0x50BCC0: whether a key can't be bound (keys from 0x100 on can)
uint8_t __cdecl IsKeyUnbindable(int key) {
    if (key >= 0x100) {
        return 0;
    }
    if (uint32_t(key - 1) > 0xD2) {
        return 1;
    }
    return unbindableKeys[key - 1];
}
CAMERA_REVERSED("UserInputManager::IsKeyUnbindable", 0x0050BCC0, &IsKeyUnbindable, nullptr, nullptr, re::Mode::Original);

// 0x50DEB0: releases the six handlers
void __fastcall UserInputManager_ReleaseHandlers(UserInputManager* self, re::Edx) {
    for (int i = 0; i < 6; i++) {
        InputHandler_Release(self->handlers[i]);
        self->handlers[i] = nullptr;
    }
}
CAMERA_REVERSED("UserInputManager::ReleaseHandlers", 0x0050DEB0, RE_BODY(&UserInputManager_ReleaseHandlers), nullptr, nullptr, re::Mode::Original);

// 0x50DEE0: IDirectInput8::CreateDevice (nullptr when it fails)
void* __fastcall UserInputManager_CreateDevice(UserInputManager* self, re::Edx, const void* guid) {
    void* device = nullptr;
    if (self->directInput) {
        void* di = self->directInput;
        auto createDevice = reinterpret_cast<long(__stdcall*)(void*, const void*, void**, void*)>(static_cast<void*>(re::ComVtable(di)[3]));
        if (createDevice(di, guid, &device, nullptr) == 0) {
            return device;
        }
    }
    return nullptr;
}
CAMERA_REVERSED("UserInputManager::CreateDevice", 0x0050DEE0, RE_BODY(&UserInputManager_CreateDevice), nullptr, nullptr, re::Mode::Original);

// 0x50DF20: the handlers in turn until one takes it (vtable[1]; returns the last
// one's answer)
uint8_t __fastcall UserInputManager_Dispatch(UserInputManager* self, re::Edx, int value) {
    uint8_t taken = 0;
    for (int i = 0; i < 6; i++) {
        taken = VCall<uint8_t>(self->handlers[i], 1, value);
        if (taken) {
            break;
        }
    }
    return taken;
}
CAMERA_REVERSED("UserInputManager::Dispatch", 0x0050DF20, RE_BODY(&UserInputManager_Dispatch), nullptr, nullptr, re::Mode::Original);

// 0x50DF50: input goes through (every flag set, nothing else holding it)
uint8_t __fastcall UserInputManager_IsEnabled(UserInputManager* self, re::Edx) {
    for (int i = 0; i < 5; i++) {
        if (!self->enabled[i]) {
            return 0;
        }
    }
    if (g_20C32E9 || g_20C32EA || At<int32_t>(g_C674E0, 0x2C) != 0) {
        return 0;
    }
    if (IsGamePaused43A680()) {
        return 0;
    }
    return 1;
}
CAMERA_REVERSED("UserInputManager::IsEnabled", 0x0050DF50, RE_BODY(&UserInputManager_IsEnabled), nullptr, nullptr, re::Mode::Original);

// 0x50DFA0 / 0x50DFB0 / 0x50DFC0 / 0x50DFD0
void __fastcall UserInputManager_SetEnabled0(UserInputManager* self, re::Edx, int value) {
    self->enabled[0] = uint8_t(value);
}
CAMERA_REVERSED("UserInputManager::SetEnabled0", 0x0050DFA0, RE_BODY(&UserInputManager_SetEnabled0), nullptr, nullptr, re::Mode::Original);

void __fastcall UserInputManager_SetEnabled1(UserInputManager* self, re::Edx, int value) {
    self->enabled[1] = uint8_t(value);
}
CAMERA_REVERSED("UserInputManager::SetEnabled1", 0x0050DFB0, RE_BODY(&UserInputManager_SetEnabled1), nullptr, nullptr, re::Mode::Original);

void __fastcall UserInputManager_SetEnabled2(UserInputManager* self, re::Edx, int value) {
    self->enabled[2] = uint8_t(value);
}
CAMERA_REVERSED("UserInputManager::SetEnabled2", 0x0050DFC0, RE_BODY(&UserInputManager_SetEnabled2), nullptr, nullptr, re::Mode::Original);

void __fastcall UserInputManager_SetEnabled3(UserInputManager* self, re::Edx, int value) {
    self->enabled[3] = uint8_t(value);
}
CAMERA_REVERSED("UserInputManager::SetEnabled3", 0x0050DFD0, RE_BODY(&UserInputManager_SetEnabled3), nullptr, nullptr, re::Mode::Original);

// 0x50DFE0: whether a control is held (one argument; ecx unused)
uint8_t __fastcall IsControlHeld(void*, re::Edx, int control) {
    if (Input_Unknown738840(0, control) != 0.0f) {
        return 1;
    }
    if (Input_Unknown738AD0(0x10, control) > 0.0f) {
        return 1;
    }
    if (Input_Unknown738880(0xCB, 0, 1)) {
        return 1;
    }
    return 0;
}
CAMERA_REVERSED("UserInputManager::IsControlHeld", 0x0050DFE0, RE_BODY(&IsControlHeld), nullptr, nullptr, re::Mode::Original);

// ---- PlayerUserInputHandler
//   +0 uses XInput (else DirectInput devices: +0x18, 0x30 apart, +0xFC of them)
//   +0x138 / +0x13C the XInput user, +0x144 the controls' values (floats),
//   +0x1E8 the XInput buttons, +0x8BA.., +0x8BC the rumble's countdown

// 0x50B590
void __fastcall PlayerUserInputHandler_Set138(void* self, re::Edx, int value) {
    At<int32_t>(self, 0x138) = value;
    At<uint8_t>(self, 0x8BA) = 0;
    At<uint8_t>(self, 0x8BF) = 0;
}
CAMERA_REVERSED("PlayerUserInputHandler::Set138", 0x0050B590, RE_BODY(&PlayerUserInputHandler_Set138), nullptr, nullptr, re::Mode::Original);

// 0x50B6F0
void __fastcall PlayerUserInputHandler_Clear140(void* self, re::Edx) {
    At<int32_t>(self, 0x140) = 0;
}
CAMERA_REVERSED("PlayerUserInputHandler::Clear140", 0x0050B6F0, RE_BODY(&PlayerUserInputHandler_Clear140), nullptr, nullptr, re::Mode::Original);

// 0x50B700
uint8_t __fastcall PlayerUserInputHandler_Get8BA(void* self, re::Edx) {
    return At<uint8_t>(self, 0x8BA);
}
CAMERA_REVERSED("PlayerUserInputHandler::Get8BA", 0x0050B700, RE_BODY(&PlayerUserInputHandler_Get8BA), nullptr, nullptr, re::Mode::Original);

// 0x50B900: counts the rumble down (5 a call), then stops it
void __fastcall PlayerUserInputHandler_UpdateRumble(void* self, re::Edx) {
    int16_t& countdown = At<int16_t>(self, 0x8BC);
    if (At<uint8_t>(self, 0x8BF) != 0 && countdown > 0) {
        countdown = int16_t(countdown - 5);
        return;
    }
    At<uint8_t>(self, 0x8BF) = 0;
    countdown = 0;
    At<uint8_t>(self, 0x8BE) = 0;
    if (At<uint8_t>(self, 0) != 0) {
        uint16_t vibration[2] = {0, 0};
        XInputSetState_(At<uint32_t>(self, 0x13C), vibration);
        return;
    }
    for (int i = 0; i < At<int32_t>(self, 0xFC); i++) {
        void* device = At<void*>(self, 0x18 + i * 0x30);
        if (device) {
            // IDirectInputDevice8::SendForceFeedbackCommand(DISFFC_STOPALL)
            auto send = reinterpret_cast<long(__stdcall*)(void*, uint32_t)>(static_cast<void*>(re::ComVtable(device)[0x58 / 4]));
            send(device, 2);
        }
    }
}
CAMERA_REVERSED("PlayerUserInputHandler::UpdateRumble", 0x0050B900, RE_BODY(&PlayerUserInputHandler_UpdateRumble), nullptr, nullptr, re::Mode::Original);

// 0x50C880: a control's value (buttons 1 or 0)
float __fastcall PlayerUserInputHandler_GetControl(void* self, re::Edx, int control) {
    float* values = &At<float>(self, 0x144);
    bool known = true;
    bool pressed = false;
    if (At<uint8_t>(self, 0) != 0) {
        if (control < 0 || control > 15) {
            known = false;
        } else {
            pressed = (buttonMasks[control] & At<uint16_t>(self, 0x1E8)) != 0;
        }
    } else if (control < 0 || (control > 15 && control != 24)) {
        known = false;
    } else {
        const int32_t* keys = ButtonMap_GetTable(buttonMap, 0);
        pressed = PlayerInput_GetValue(self, keys[control], values) > 0.0f;
    }
    // (read for any control, in range or not)
    float value = values[control];
    if (known) {
        return pressed ? 1.0f : 0.0f;
    }
    return value;
}
CAMERA_REVERSED("PlayerUserInputHandler::GetControl", 0x0050C880, RE_BODY(&PlayerUserInputHandler_GetControl), nullptr, nullptr, re::Mode::Original);

// 0x50C7D0: swaps two keys' states
void __fastcall PlayerUserInputHandler_SwapKeys(void* self, re::Edx, int a, int b) {
    if (At<uint8_t>(self, 0) != 0) {
        uint16_t& buttons = At<uint16_t>(self, 0x1E8);
        uint32_t held = buttons;
        bool aHeld = (uint32_t(a) & held) != 0;
        bool bHeld = (uint32_t(b) & held) != 0;
        uint32_t rest = ~uint32_t(b) & ~uint32_t(a) & held;
        buttons = uint16_t(rest);
        if (aHeld) {
            buttons = uint16_t(rest | uint32_t(b));
        }
        if (bHeld) {
            buttons = uint16_t(buttons | uint16_t(a));
        }
        return;
    }
    float* values = &At<float>(self, 0x144);
    float aValue = PlayerInput_GetValue(self, a, values);
    float bValue = PlayerInput_GetValue(self, b, values);
    PlayerInput_SetValue(self, a, bValue, values);
    PlayerInput_SetValue(self, b, aValue, values);
}
CAMERA_REVERSED("PlayerUserInputHandler::SwapKeys", 0x0050C7D0, RE_BODY(&PlayerUserInputHandler_SwapKeys), nullptr, nullptr, re::Mode::Original);

} // namespace
