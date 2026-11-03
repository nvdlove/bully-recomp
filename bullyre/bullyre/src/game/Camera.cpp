#include "game/Camera.h"

#include <cstring>
#include "reversed.h"

// Camera part 1: CCamera, the controller transition, the base controller and the
// small controllers' destructors, resets and setters. (Thiscalls on controllers not
// given a struct here are __fastcall wrappers with the unused edx.)

#define CAMERA_REVERSED(...) REVERSED(__VA_ARGS__)

namespace {

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);           // CTimer's
GAME_VAR(uint8_t*, cameraSettings, 0x020C88B8);     // +0x6C the default fov (int), +0x74.. aspect ratios
GAME_VAR(uint8_t, widescreen, 0x00C1870E);
GAME_VAR(float, cameraFov, 0x00C3CD04);             // TheCamera.fov, written as a global
GAME_VAR(void*, playerPed, 0x00C1AEA8);             // FindPlayerPed
GAME_VAR(void*, convCamCtrl, 0x00C3CFC0);           // the conversation camera, while there is one
GAME_VAR(void*, storeCameraController, 0x00C3D2A4); // the store camera, while there is one
GAME_VAR(uint32_t, arcadeDefault, 0x020C8930);
// (4F1160's and 4F1190's: grade display state)
GAME_VAR(float, g_1B65C10, 0x01B65C10);
GAME_VAR(int32_t, g_1B65C0C, 0x01B65C0C);
GAME_VAR(uint8_t, showingGrades, 0x020C8708);
GAME_VAR(uint8_t, g_20C87A8, 0x020C87A8);
GAME_VAR(int32_t, g_20C8730, 0x020C8730);
GAME_VAR(float, g_20C87A0, 0x020C87A0);
GAME_VAR(uint8_t, g_20C870B, 0x020C870B);
inline const cam::CVector& cutsceneDefault = *reinterpret_cast<const cam::CVector*>(0x00BCC564);

// the camera's Gamebryo node and its local transform
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(cam::NiTransform*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(void*, __cdecl, GetCameraFrame, 0x006C81D0, void* camera);
GAME_FN(void, __thiscall, Unknown48_Set, 0x00412A70, void* self, void* frame);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, cam::CVector* v);
GAME_FN(void, __thiscall, Camera_Unknown4F2390, 0x004F2390, CCamera* camera, int value);
// (an empty function: the profiler marker, shared by everything empty-bodied)
GAME_FN(void, __thiscall, EmptyThiscall, 0x0049A650, void* self);
// calls fn(object) (for the controllers' owned sub-controllers, on destruction)
GAME_FN(void, __cdecl, ForEachOwned, 0x0072B8B0, void* fn, void* object);
GAME_FN(uint32_t, __cdecl, Unknown738740, 0x00738740, int value);
GAME_FN(uint8_t, __thiscall, FollowCamera_Flag30, 0x004FB6C0, CameraController* follow);

// the base destructors of the "Butes" (attribute sets)
GAME_FN(void, __thiscall, CinematicShotButes_Destruct, 0x0072AA70, void* self);
GAME_FN(void, __thiscall, FollowCam3Butes_Destruct, 0x0072A600, void* self);
GAME_FN(void, __thiscall, VehicleCamButes_Destruct, 0x0072A5F0, void* self);
GAME_FN(void, __thiscall, CutsceneCameraController_Destruct, 0x004F8030, void* self);
GAME_FN(void, __thiscall, FightCamCtrl_Reset, 0x004F8C40, void* self);
GAME_FN(void, __thiscall, FollowCameraFight_Reset, 0x004FE640, void* self);
GAME_FN(void, __thiscall, FollowCameraBase_Destruct, 0x004FE0C0, void* self);
GAME_FN(void, __thiscall, ConvCamCtrl_Destruct, 0x004F4FC0, void* self);
GAME_FN(void, __thiscall, FollowCameraFight_Unknown500560, 0x00500560, void* self, int value);
GAME_FN(void, __thiscall, FollowCameraVehicle_Unknown4FE010, 0x004FE010, void* self, int value);
GAME_FN(void, __thiscall, FollowCameraVehicle_Unknown4FE000, 0x004FE000, void* self);
GAME_FN(void, __thiscall, FollowInterp_Unknown4FB4D0, 0x004FB4D0, void* self);
GAME_FN(void, __thiscall, FollowCameraController3_Unknown5044A0, 0x005044A0, void* self, cam::CVector a, cam::CVector b, int c);
GAME_FN(uint8_t, __thiscall, Ped_Unknown4CE690, 0x004CE690, void* ped);
GAME_FN(void, __thiscall, Ped_Unknown4732A0, 0x004732A0, void* ped, int a, int b);
GAME_FN(void, __thiscall, OffsetCameraController_Unknown5051E0, 0x005051E0, void* self);
GAME_FN(uint8_t, __thiscall, OffsetCameraController_Unknown5050D0, 0x005050D0, void* self, void* name);
GAME_FN(void, __thiscall, Name_Construct, 0x005FA910, void* self, uintptr_t value);
GAME_FN(void, __thiscall, VehicleCameraController_Unknown509C50, 0x00509C50, void* self, float value);
GAME_FN(void, __thiscall, VehicleCameraController_Unknown50B2F0, 0x0050B2F0, void* self);
GAME_FN(void, __thiscall, StoreCameraController_Reset, 0x005093D0, void* self);

// own functions, called through the game's address
GAME_FN(void, __cdecl, ResetGradeDisplay, 0x004F1190);
GAME_FN(void, __thiscall, SplineInterpolater_Destruct, 0x005083E0, void* self);
GAME_FN(void, __thiscall, SplineInterpolater_Clear, 0x00508420, void* self);
GAME_FN(void, __thiscall, SplineInterpolater_Unknown508930, 0x00508930, void* self, uintptr_t value);
GAME_FN(void, __thiscall, SplineInterpolater_Set, 0x00509240, void* self, int value);
GAME_FN(void, __thiscall, SplineInterpolater_Set34, 0x005083D0, void* self, float value);
GAME_FN(void, __thiscall, SplineInterpolater_Set2C, 0x005083A0, void* self, float value);
GAME_FN(void, __thiscall, SplineInterpolater_Set30, 0x005083C0, void* self, float value);
GAME_FN(void, __thiscall, ScriptCameraController_Clear, 0x00507670, void* self);
GAME_FN(void, __thiscall, ScriptCameraController_Unknown507970, 0x00507970, void* self, uintptr_t a, int b);

constexpr uintptr_t kCameraControllerVtable = 0x00914160;

using re::At;

// vtable[index] of an object, as a thiscall
template <typename R, typename... A>
R VCall(void* object, int index, A... args) {
    auto fn = reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(object))[index]));
    return fn(object, args...);
}

int DefaultFov() {
    return At<int32_t>(cameraSettings, 0x6C);
}

cam::NiTransform* CameraTransform(void* camera) {
    return GetLocalTransform(GetCameraNode(camera));
}

} // namespace

// ---- CameraTransition

float CameraTransition::GetWeight(int controller) {
    if (!active) {
        return 1.0f;
    }
    if (controller == ends[0].controller) {
        return 1.0f - t;
    }
    if (controller == ends[1].controller) {
        return t;
    }
    return 0.0f;
}
CAMERA_REVERSED("CameraTransition::GetWeight", 0x004F0AC0, re::MemberAddress(&CameraTransition::GetWeight), nullptr, nullptr, re::Mode::Original);

int32_t CameraTransition::GetDirection() {
    return active ? forwards : 0;
}
CAMERA_REVERSED("CameraTransition::GetDirection", 0x004F0B00, re::MemberAddress(&CameraTransition::GetDirection), nullptr, nullptr, re::Mode::Original);

int32_t* CameraTransition::GetController(int32_t* out, int end) {
    *out = ends[end].controller;
    return out;   // (returns the output pointer, as the game's eax does: 2026-10-30)
}
CAMERA_REVERSED("CameraTransition::GetController", 0x004F0B10, re::MemberAddress(&CameraTransition::GetController), nullptr, nullptr, re::Mode::Original);

void CameraTransition::Stop() {
    active = 0;
    t = 0.0f;
}
CAMERA_REVERSED("CameraTransition::Stop", 0x004F0B30, re::MemberAddress(&CameraTransition::Stop), nullptr, nullptr, re::Mode::Original);

void CameraTransition::Start(int from, int to, int ms) {
    t = 0.0f;
    ends[0].controller = from;
    ends[1].controller = to;
    active = 0;
    if (ms != 0) {
        duration = ms;
        startTime = timeInMs;
        active = 1;
        forwards = 1;
    }
}
CAMERA_REVERSED("CameraTransition::Start", 0x004F0B40, re::MemberAddress(&CameraTransition::Start), nullptr, nullptr, re::Mode::Original);

float CameraTransition::GetProgress() {
    float progress;
    if (duration == 0) {
        progress = 0.0f;
    } else {
        // (the elapsed time as unsigned, over the duration in the FPU's precision)
        progress = float(double(uint32_t(timeInMs - startTime)) / double(duration));
    }
    if (progress < 0.0f) {
        progress = 0.0f;
    } else if (progress >= 1.0f) {
        progress = 1.0f;
    }
    if (forwards == 0) {
        progress = 1.0f - progress;
    }
    return progress;
}
CAMERA_REVERSED("CameraTransition::GetProgress", 0x004F0B80, re::MemberAddress(&CameraTransition::GetProgress), nullptr, nullptr, re::Mode::Original);

void CameraTransition::SetEnd(int end, void* camera, float a, float b) {
    ends[end].transform = *CameraTransform(camera);
    ends[end].unknown34 = a;
    ends[end].unknown3C = b;
}
CAMERA_REVERSED("CameraTransition::SetEnd", 0x004F0C00, re::MemberAddress(&CameraTransition::SetEnd), nullptr, nullptr, re::Mode::Original);

void CameraTransition::GetEnd(int end, void* camera, float* a, float* b) {
    *CameraTransform(camera) = ends[end].transform;
    *a = ends[end].unknown34;
    *b = ends[end].unknown3C;
}
CAMERA_REVERSED("CameraTransition::GetEnd", 0x004F0C40, re::MemberAddress(&CameraTransition::GetEnd), nullptr, nullptr, re::Mode::Original);

// ---- CCamera

void CCamera::SetController(int index, CameraController* controller) {
    if (controllers[index]) {
        VCall<void*>(controllers[index], 0, 1);
    }
    if (!controller) {
        controller = static_cast<CameraController*>(Malloc(4));
        if (controller) {
            controller->vtable = reinterpret_cast<void*>(kCameraControllerVtable);
        }
    }
    controllers[index] = controller;
}
CAMERA_REVERSED("CCamera::SetController", 0x004F11C0, re::MemberAddress(&CCamera::SetController), nullptr, nullptr, re::Mode::Original);

float CCamera::GetZoom() {
    float current = fovOverridden ? fovOverride : fov;
    return float(double(DefaultFov()) / current);
}
CAMERA_REVERSED("CCamera::GetZoom", 0x004F1250, re::MemberAddress(&CCamera::GetZoom), nullptr, nullptr, re::Mode::Original);

uint8_t CCamera::GetUnknown1A8() {
    if (unknown280 != 2 && unknown280 != 1) {
        return 0;
    }
    return unknown1A8;
}
CAMERA_REVERSED("CCamera::GetUnknown1A8", 0x004F12A0, re::MemberAddress(&CCamera::GetUnknown1A8), nullptr, nullptr, re::Mode::Original);

void CCamera::CallController20(int index, int value) {
    if (*reinterpret_cast<int32_t*>(unknown1A4) != 0) {
        VCall<void>(controllers[index], 8, value);
    }
}
CAMERA_REVERSED("CCamera::CallController20", 0x004F12C0, re::MemberAddress(&CCamera::CallController20), nullptr, nullptr, re::Mode::Original);

int32_t CCamera::GetFadeState() {
    return fadeTarget == fadeState ? fadeState : 2;
}
CAMERA_REVERSED("CCamera::GetFadeState", 0x004F12F0, re::MemberAddress(&CCamera::GetFadeState), nullptr, nullptr, re::Mode::Original);

float CCamera::GetFov() {
    if (fovOverridden) {
        return fovOverride;
    }
    return savedController ? savedFov : fov;
}
CAMERA_REVERSED("CCamera::GetFov", 0x004F1390, re::MemberAddress(&CCamera::GetFov), nullptr, nullptr, re::Mode::Original);

uint8_t CCamera::IsFollowCameraFlag30() {
    if (activeController != 1) {
        return 0;
    }
    return FollowCamera_Flag30(controllers[1]);
}
CAMERA_REVERSED("CCamera::IsFollowCameraFlag30", 0x004F1640, re::MemberAddress(&CCamera::IsFollowCameraFlag30), nullptr, nullptr, re::Mode::Original);

void CCamera::ResetFov() {
    fov = float(DefaultFov());
}
CAMERA_REVERSED("CCamera::ResetFov", 0x004F1660, re::MemberAddress(&CCamera::ResetFov), nullptr, nullptr, re::Mode::Original);

void CCamera::SetCamera(void* newCamera) {
    camera = newCamera;
    frame = GetCameraFrame(newCamera);
    Unknown48_Set(unknown48, frame);
}
CAMERA_REVERSED("CCamera::SetCamera", 0x004F1800, re::MemberAddress(&CCamera::SetCamera), nullptr, nullptr, re::Mode::Original);

void CCamera::SaveView(int controller) {
    savedController = controller;
    if (controller) {
        savedTransform = *CameraTransform(camera);
        VCall<void>(controllers[savedController], 3);
    }
}
CAMERA_REVERSED("CCamera::SaveView", 0x004F1830, re::MemberAddress(&CCamera::SaveView), nullptr, nullptr, re::Mode::Original);

void CCamera::Fade(float seconds, int direction, int) {
    if (direction == fadeState || direction == fadeTarget) {
        return;
    }
    fadeTarget = direction;
    if (!(seconds > 0.0f)) {
        fadeState = direction;
        if (direction == 1) {
            fadeAlpha = float(fadeColour[3]);
            ResetGradeDisplay();
        } else {
            fadeAlpha = 0.0f;
        }
        return;
    }
    fadeState = 2;
    // (a frame less: 0.033 as a float, subtracted as a double)
    fadeDuration = float(double(seconds) - double(0.033f));
    fadeStart = float(double(timeInMs) / 1000.0);
    if (direction == 1) {
        // (already close to faded out: done at once)
        if (double(int(fadeColour[3]) - 20) <= double(fadeAlpha)) {
            fadeState = direction;
            fadeAlpha = float(fadeColour[3]);
        }
    } else if (fadeAlpha <= 20.0f) {
        fadeAlpha = 0.0f;
        fadeState = direction;
    }
}
CAMERA_REVERSED("CCamera::Fade", 0x004F1AD0, re::MemberAddress(&CCamera::Fade), nullptr, nullptr, re::Mode::Original);

void CCamera::ResetController() {
    // (the global camera's fov, whichever camera this is)
    cameraFov = float(DefaultFov());
    VCall<void>(controllers[activeController], 3);
}
CAMERA_REVERSED("CCamera::ResetController", 0x004F2B60, re::MemberAddress(&CCamera::ResetController), nullptr, nullptr, re::Mode::Original);

void CCamera::ControllerLookAt(cam::CVector target) {
    cameraFov = float(DefaultFov());
    // (vf5 takes the vector by value, as passed here: not through VCall's argument check)
    void* controller = controllers[activeController];
    re::GameVirtual<void(__thiscall*)(void*, cam::CVector)>(controller, 5)(controller, target);
}
CAMERA_REVERSED("CCamera::ControllerLookAt", 0x004F2B90, re::MemberAddress(&CCamera::ControllerLookAt), nullptr, nullptr, re::Mode::Original);

void CCamera::ControllerLookAt2(cam::CVector target, int value) {
    cameraFov = float(DefaultFov());
    // (vf6 takes the vector by value, as passed here: not through VCall's argument check)
    void* controller = controllers[activeController];
    re::GameVirtual<void(__thiscall*)(void*, cam::CVector, int)>(controller, 6)(controller, target, value);
}
CAMERA_REVERSED("CCamera::ControllerLookAt2", 0x004F2BD0, re::MemberAddress(&CCamera::ControllerLookAt2), nullptr, nullptr, re::Mode::Original);

namespace {

// 0x4F1160: no grade display showing or pending
int32_t __cdecl IsGradeDisplayIdle() {
    if (g_1B65C10 <= 0.0f && g_1B65C0C == 0) {
        return 1;
    }
    return 0;
}
CAMERA_REVERSED("IsGradeDisplayIdle", 0x004F1160, &IsGradeDisplayIdle, nullptr, nullptr, re::Mode::Original);

// 0x4F1190: (called when the screen is faded out at once)
void __cdecl ResetGradeDisplayOurs() {
    showingGrades = 0;
    if (g_20C87A8 == 0 && g_20C8730 == 0) {
        g_20C87A8 = 0;
        g_20C87A0 = 0.0f;
        g_20C870B = 0;
    }
}
CAMERA_REVERSED("ResetGradeDisplay", 0x004F1190, &ResetGradeDisplayOurs, nullptr, nullptr, re::Mode::Original);

// 0x4F1360: the screen's aspect ratio (4:3 or 16:9, from the settings)
float __cdecl GetAspectRatio() {
    float ratio = At<float>(cameraSettings, 0x74) / At<float>(cameraSettings, 0x78);
    if (widescreen) {
        ratio = At<float>(cameraSettings, 0x7C) / At<float>(cameraSettings, 0x80);
    }
    return ratio;
}
CAMERA_REVERSED("CCamera::GetAspectRatio", 0x004F1360, &GetAspectRatio, nullptr, nullptr, re::Mode::Original);

// 0x4F1520: a stick axis past its dead zone, rescaled to 0..1 (0 inside it)
float __cdecl ApplyDeadZone(float value, float deadZone) {
    float range = 1.0f - deadZone;
    if (value < -deadZone) {
        return float((double(value) + deadZone) / range);
    }
    if (deadZone < value) {
        return float((double(value) - deadZone) / range);
    }
    return 0.0f;
}
CAMERA_REVERSED("ApplyDeadZone", 0x004F1520, &ApplyDeadZone, nullptr, nullptr, re::Mode::Original);

// 0x4F1630: 255 (one argument, unused)
float __fastcall GetMaxAlpha(void*, re::Edx, int) {
    return 255.0f;
}
CAMERA_REVERSED("CCamera::GetMaxAlpha", 0x004F1630, RE_BODY(&GetMaxAlpha), nullptr, nullptr, re::Mode::Original);

// ---- CameraController (the base)

// 0x4F0850 / 0x4F0860 / 0x4F4230 (vtable[5], [6], [4]): the reset, arguments unused
void __fastcall CameraController_LookAt(CameraController* self, re::Edx, cam::CVector) {
    VCall<void>(self, 3);
}
CAMERA_REVERSED("CameraController::LookAt", 0x004F0850, RE_BODY(&CameraController_LookAt), nullptr, nullptr, re::Mode::Original);

void __fastcall CameraController_LookAt2(CameraController* self, re::Edx, cam::CVector, int) {
    VCall<void>(self, 3);
}
CAMERA_REVERSED("CameraController::LookAt2", 0x004F0860, RE_BODY(&CameraController_LookAt2), nullptr, nullptr, re::Mode::Original);

void __fastcall CameraController_Vf4(CameraController* self, re::Edx) {
    VCall<void>(self, 3);
}
CAMERA_REVERSED("CameraController::vf4", 0x004F4230, RE_BODY(&CameraController_Vf4), nullptr, nullptr, re::Mode::Original);

// 0x4F42B0 (vtable[0])
CameraController* __fastcall CameraController_Delete(CameraController* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(kCameraControllerVtable);
    if (flags & 1) {
        Free(self);
    }
    return self;
}
CAMERA_REVERSED("CameraController::~CameraController", 0x004F42B0, RE_BODY(&CameraController_Delete), nullptr, nullptr, re::Mode::Original);

// 0x4F2E00 (vtable[7], process): TheCamera's defaults back
void __fastcall CameraController_Process(CameraController*, re::Edx) {
    TheCamera.unknown270 = 1;
    TheCamera.unknown278 = 1;
    TheCamera.unknown274 = 0;
    TheCamera.unknown27C = 0;
    TheCamera.unknown280 = 0;
    Camera_Unknown4F2390(&TheCamera, 0);
}
CAMERA_REVERSED("CameraController::Process", 0x004F2E00, RE_BODY(&CameraController_Process), nullptr, nullptr, re::Mode::Original);

// ---- ArcadeCameraController

// 0x4F0870: (the vtable only)
void* __fastcall ArcadeCameraController_Construct(void* self, re::Edx) {
    At<uint32_t>(self, 0) = 0x0091418C;
    return self;
}
CAMERA_REVERSED("ArcadeCameraController::ArcadeCameraController", 0x004F0870, RE_BODY(&ArcadeCameraController_Construct), nullptr, nullptr, re::Mode::Original);

// 0x4F0880 (vtable[3])
void __fastcall ArcadeCameraController_Reset(void* self, re::Edx) {
    At<uint32_t>(self, 4) = arcadeDefault;
}
CAMERA_REVERSED("ArcadeCameraController::Reset", 0x004F0880, RE_BODY(&ArcadeCameraController_Reset), nullptr, nullptr, re::Mode::Original);

// 0x4F0920: the offset (+0x180), no longer than 10
void __fastcall ArcadeCameraController_SetOffset(void* self, re::Edx, const cam::CVector* offset) {
    cam::CVector& v = At<cam::CVector>(self, 0x180);
    v = *offset;
    float lengthSq = float((double(v.y) * v.y + double(v.x) * v.x) + double(v.z) * v.z);
    if (lengthSq > 100.0) {
        Vector_Normalise(&v);
        v.x = float(v.x * 10.0);
        v.y = float(v.y * 10.0);
        v.z = float(v.z * 10.0);
    }
}
CAMERA_REVERSED("ArcadeCameraController::SetOffset", 0x004F0920, RE_BODY(&ArcadeCameraController_SetOffset), nullptr, nullptr, re::Mode::Original);

// ---- the view offset (a saved camera transform with an offset and rotation on top;
// 0x50 bytes; whose it is: not pinned down)

struct CameraViewOffset {
    uint8_t active;               // +0
    uint8_t unknown1[3];          // +1
    cam::CVector offset;          // +4
    cam::CVector rotation;        // +0x10
    cam::NiTransform saved;       // +0x1C
};
static_assert(sizeof(CameraViewOffset) == 0x50);

// 0x4F3D10
void __fastcall CameraViewOffset_SetOffset(CameraViewOffset* self, re::Edx, float x, float y, float z) {
    self->offset = {x, y, z};
}
CAMERA_REVERSED("CameraViewOffset::SetOffset", 0x004F3D10, RE_BODY(&CameraViewOffset_SetOffset), nullptr, nullptr, re::Mode::Original);

// 0x4F3D30
void __fastcall CameraViewOffset_SetRotation(CameraViewOffset* self, re::Edx, float x, float y, float z) {
    self->rotation = {x, y, z};
}
CAMERA_REVERSED("CameraViewOffset::SetRotation", 0x004F3D30, RE_BODY(&CameraViewOffset_SetRotation), nullptr, nullptr, re::Mode::Original);

// 0x4F3D50: the saved transform back on the camera
void __fastcall CameraViewOffset_Restore(CameraViewOffset* self, re::Edx) {
    if (self->active) {
        *CameraTransform(TheCamera.camera) = self->saved;
    }
}
CAMERA_REVERSED("CameraViewOffset::Restore", 0x004F3D50, RE_BODY(&CameraViewOffset_Restore), nullptr, nullptr, re::Mode::Original);

// 0x4F3D80
void __fastcall CameraViewOffset_Clear(CameraViewOffset* self, re::Edx) {
    self->offset = {0.0f, 0.0f, 0.0f};
    self->rotation = {0.0f, 0.0f, 0.0f};
    self->saved.scale = 1.0f;
    self->active = 0;
}
CAMERA_REVERSED("CameraViewOffset::Clear", 0x004F3D80, RE_BODY(&CameraViewOffset_Clear), nullptr, nullptr, re::Mode::Original);

// 0x4F3DA0: saves the camera's transform
void __fastcall CameraViewOffset_Start(CameraViewOffset* self, re::Edx, int active) {
    self->saved = *CameraTransform(TheCamera.camera);
    self->active = uint8_t(active);
    self->offset = {0.0f, 0.0f, 0.0f};
    self->rotation = {0.0f, 0.0f, 0.0f};
}
CAMERA_REVERSED("CameraViewOffset::Start", 0x004F3DA0, RE_BODY(&CameraViewOffset_Start), nullptr, nullptr, re::Mode::Original);

// 0x4F3E20
void __fastcall CameraViewOffset_AddOffset(CameraViewOffset* self, re::Edx, float x, float y, float z) {
    self->offset.x = self->offset.x + x;
    self->offset.y = self->offset.y + y;
    self->offset.z = self->offset.z + z;
}
CAMERA_REVERSED("CameraViewOffset::AddOffset", 0x004F3E20, RE_BODY(&CameraViewOffset_AddOffset), nullptr, nullptr, re::Mode::Original);

// 0x4F3E50
void __fastcall CameraViewOffset_AddRotation(CameraViewOffset* self, re::Edx, float x, float y, float z) {
    self->rotation.x = self->rotation.x + x;
    self->rotation.y = self->rotation.y + y;
    self->rotation.z = self->rotation.z + z;
}
CAMERA_REVERSED("CameraViewOffset::AddRotation", 0x004F3E50, RE_BODY(&CameraViewOffset_AddRotation), nullptr, nullptr, re::Mode::Original);

// ---- CoasterCameraController

// 0x4F4240 (vtable[2]) (returns what 0x738740 left in eax)
uint32_t __fastcall CoasterCameraController_Vf2(void*, re::Edx) {
    return Unknown738740(0);
}
CAMERA_REVERSED("CoasterCameraController::vf2", 0x004F4240, RE_BODY(&CoasterCameraController_Vf2), nullptr, nullptr, re::Mode::Original);

// 0x4F4290 (the vtable and its own fields only)
void* __fastcall CoasterCameraController_Construct(void* self, re::Edx) {
    At<float>(self, 0x38) = 1.0f;
    At<uint32_t>(self, 0) = 0x0091433C;
    At<float>(self, 0x5C) = 0.0f;
    At<float>(self, 0x60) = 0.0f;
    At<float>(self, 0x64) = 0.0f;
    return self;
}
CAMERA_REVERSED("CoasterCameraController::CoasterCameraController", 0x004F4290, RE_BODY(&CoasterCameraController_Construct), nullptr, nullptr, re::Mode::Original);

// 0x4F4DE0: a 0x18-byte record (whose: not pinned down)
void* __fastcall Unknown4F4DE0_Construct(void* self, re::Edx) {
    At<int32_t>(self, 0) = 0;
    At<int32_t>(self, 4) = 0;
    At<int32_t>(self, 8) = 0;
    At<int32_t>(self, 0x10) = 0;
    At<int32_t>(self, 0x14) = 0;
    At<uint8_t>(self, 0xC) = 1;
    At<uint8_t>(self, 0xD) = 0;
    At<uint8_t>(self, 0xE) = 1;
    return self;
}
CAMERA_REVERSED("Unknown4F4DE0::Construct", 0x004F4DE0, RE_BODY(&Unknown4F4DE0_Construct), nullptr, nullptr, re::Mode::Original);

// ---- ConvCamCtrl (the conversation camera)

// 0x4F4FC0: its sub-objects deleted (vtable[1] with 1: their deleting destructor)
void __fastcall ConvCamCtrl_DestructOurs(void* self, re::Edx) {
    At<uint32_t>(self, 0) = 0x0091436C;
    if (void* p = At<void*>(self, 0x24)) {
        VCall<void>(p, 1, 1);
    }
    if (void* p = At<void*>(self, 0x20)) {
        VCall<void>(p, 1, 1);
    }
    convCamCtrl = nullptr;
    if (void* p = At<void*>(self, 0xC8)) {
        VCall<void>(p, 1, 1);
    }
    if (void* p = At<void*>(self, 0x2C)) {
        VCall<void>(p, 1, 1);
    }
    At<uint32_t>(self, 0) = kCameraControllerVtable;
}
CAMERA_REVERSED("ConvCamCtrl::~ConvCamCtrl", 0x004F4FC0, RE_BODY(&ConvCamCtrl_DestructOurs), nullptr, nullptr, re::Mode::Original);

// 0x4F5EB0 (vtable[0])
void* __fastcall ConvCamCtrl_Delete(void* self, re::Edx, int flags) {
    ConvCamCtrl_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}
CAMERA_REVERSED("ConvCamCtrl::vf0", 0x004F5EB0, RE_BODY(&ConvCamCtrl_Delete), nullptr, nullptr, re::Mode::Original);

// 0x4F5020 (vtable[3])
void __fastcall ConvCamCtrl_Reset(void* self, re::Edx) {
    At<uint8_t>(self, 0x178) = 1;
}
CAMERA_REVERSED("ConvCamCtrl::Reset", 0x004F5020, RE_BODY(&ConvCamCtrl_Reset), nullptr, nullptr, re::Mode::Original);

// 0x4F56A0
int32_t __fastcall ConvCamCtrl_GetCC(void* self, re::Edx, int index) {
    return At<int32_t>(self, 0xCC + index * 4);
}
CAMERA_REVERSED("ConvCamCtrl::GetCC", 0x004F56A0, RE_BODY(&ConvCamCtrl_GetCC), nullptr, nullptr, re::Mode::Original);

// 0x4F5ED0 (vtable[4]): starts from where the camera is
void __fastcall ConvCamCtrl_Vf4(void* self, re::Edx) {
    cam::NiTransform* transform = CameraTransform(TheCamera.camera);
    At<cam::CVector>(self, 4) = transform->translate;
    At<cam::CVector>(self, 0x10) = TheCamera.unknown198;
    At<uint8_t>(self, 0x178) = 1;
}
CAMERA_REVERSED("ConvCamCtrl::vf4", 0x004F5ED0, RE_BODY(&ConvCamCtrl_Vf4), nullptr, nullptr, re::Mode::Original);

// ---- the attribute sets' deleting destructors (vtable[1])

void* __fastcall CinematicShotButes_Delete(void* self, re::Edx, int flags) {
    CinematicShotButes_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}
CAMERA_REVERSED("CinematicShotButes::vf1", 0x004F5780, RE_BODY(&CinematicShotButes_Delete), nullptr, nullptr, re::Mode::Original);

void* __fastcall FollowCam3Butes_Delete(void* self, re::Edx, int flags) {
    FollowCam3Butes_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}
CAMERA_REVERSED("FollowCam3Butes::vf1", 0x004FBFA0, RE_BODY(&FollowCam3Butes_Delete), nullptr, nullptr, re::Mode::Original);

void* __fastcall VehicleCamButes_Delete(void* self, re::Edx, int flags) {
    VehicleCamButes_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}
CAMERA_REVERSED("VehicleCamButes::vf1", 0x0050A000, RE_BODY(&VehicleCamButes_Delete), nullptr, nullptr, re::Mode::Original);

// ---- CutsceneCameraController

// 0x4F7460 (vtable[3])
void __fastcall CutsceneCameraController_Reset(void* self, re::Edx) {
    At<float>(self, 0x5C) = 0.0f;
    At<float>(self, 0x60) = 0.0f;
    At<int32_t>(self, 0x58) = 0;
    At<int32_t>(self, 0x40) = 0;
    At<int32_t>(self, 0x44) = 0;
    At<int32_t>(self, 0x48) = 0;
    At<int32_t>(self, 0x54) = -1;
    At<int32_t>(self, 0x50) = -1;
    At<int32_t>(self, 0x4C) = 0;
    At<cam::CVector>(self, 0x68) = cutsceneDefault;
    At<int32_t>(self, 0xB8) = 0;
}
CAMERA_REVERSED("CutsceneCameraController::Reset", 0x004F7460, RE_BODY(&CutsceneCameraController_Reset), nullptr, nullptr, re::Mode::Original);

// 0x4F7AC0: frees its four arrays ({data, count, capacity}-like)
void __fastcall CutsceneCameraController_FreeTracks(void* self, re::Edx) {
    if (void* p = At<void*>(self, 0x1C)) {
        Free(p);
        At<void*>(self, 0x1C) = nullptr;
    }
    At<int32_t>(self, 0x20) = 0;
    At<int32_t>(self, 0x24) = 0;
    if (void* p = At<void*>(self, 0x28)) {
        Free(p);
        At<void*>(self, 0x28) = nullptr;
    }
    At<int32_t>(self, 0x2C) = 0;
    At<int32_t>(self, 0x30) = 0;
    if (void* p = At<void*>(self, 4)) {
        Free(p);
        At<void*>(self, 4) = nullptr;
    }
    At<int32_t>(self, 8) = 0;
    At<int32_t>(self, 0xC) = 0;
    if (void* p = At<void*>(self, 0x10)) {
        Free(p);
        At<void*>(self, 0x10) = nullptr;
    }
    At<int32_t>(self, 0x18) = 0;
    At<int32_t>(self, 0x14) = 0;
}
CAMERA_REVERSED("CutsceneCameraController::FreeTracks", 0x004F7AC0, RE_BODY(&CutsceneCameraController_FreeTracks), nullptr, nullptr, re::Mode::Original);

// 0x4F8550 (vtable[0])
void* __fastcall CutsceneCameraController_Delete(void* self, re::Edx, int flags) {
    CutsceneCameraController_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}
CAMERA_REVERSED("CutsceneCameraController::vf0", 0x004F8550, RE_BODY(&CutsceneCameraController_Delete), nullptr, nullptr, re::Mode::Original);

// ---- the controllers owning sub-controllers (0x509960 on each, from 0x72B8B0)

// 0x509960: vtable[2], then vtable[1] (no arguments)
void __cdecl ReleaseOwned(void* object) {
    VCall<void>(object, 2);
    VCall<void>(object, 1);
}
CAMERA_REVERSED("CameraController::ReleaseOwned", 0x00509960, &ReleaseOwned, nullptr, nullptr, re::Mode::Original);

void* DeleteOwning(void* self, uintptr_t vtable, void* releaseFn, int flags) {
    At<uint32_t>(self, 0) = vtable;
    ForEachOwned(releaseFn, self);
    At<uint32_t>(self, 0) = kCameraControllerVtable;
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x4F9A20 (vtable[0])
void* __fastcall FightCamCtrl_Delete(void* self, re::Edx, int flags) {
    return DeleteOwning(self, 0x00914510, reinterpret_cast<void*>(0x00509960), flags);
}
CAMERA_REVERSED("FightCamCtrl::vf0", 0x004F9A20, RE_BODY(&FightCamCtrl_Delete), nullptr, nullptr, re::Mode::Original);

// 0x4FAD70 (vtable[0])
void* __fastcall FirstPersonCameraController_Delete(void* self, re::Edx, int flags) {
    return DeleteOwning(self, 0x00914540, reinterpret_cast<void*>(0x00509960), flags);
}
CAMERA_REVERSED("FirstPersonCameraController::vf0", 0x004FAD70, RE_BODY(&FirstPersonCameraController_Delete), nullptr, nullptr, re::Mode::Original);

// 0x50B040 (vtable[0]): also its attribute set (+0x108)
void* __fastcall VehicleCameraController_Delete(void* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = 0x00914758;
    ForEachOwned(reinterpret_cast<void*>(0x00509960), self);
    if (void* butes = At<void*>(self, 0x108)) {
        VCall<void>(butes, 1, 1);
    }
    At<uint32_t>(self, 0) = kCameraControllerVtable;
    if (flags & 1) {
        Free(self);
    }
    return self;
}
CAMERA_REVERSED("VehicleCameraController::vf0", 0x0050B040, RE_BODY(&VehicleCameraController_Delete), nullptr, nullptr, re::Mode::Original);

// 0x4F9A60 (vtable[1])
void __fastcall FightCamCtrl_Vf1(void* self, re::Edx) {
    At<int32_t>(self, 0x4C) = 0;
    At<uint8_t>(self, 0x50) = 0;
    At<int32_t>(self, 0x54) = -1;
    FightCamCtrl_Reset(self);
}
CAMERA_REVERSED("FightCamCtrl::vf1", 0x004F9A60, RE_BODY(&FightCamCtrl_Vf1), nullptr, nullptr, re::Mode::Original);

// 0x4FA6D0 (vtable[1])
void __fastcall FirstPersonCameraController_Vf1(void* self, re::Edx) {
    At<uint8_t>(self, 0x24) = 0;
    VCall<void>(self, 8, 0);
}
CAMERA_REVERSED("FirstPersonCameraController::vf1", 0x004FA6D0, RE_BODY(&FirstPersonCameraController_Vf1), nullptr, nullptr, re::Mode::Original);

// 0x4FA7B0 (vtable[2])
void __fastcall FirstPersonCameraController_Vf2(void* self, re::Edx) {
    At<int32_t>(self, 0x28) = 0;
}
CAMERA_REVERSED("FirstPersonCameraController::vf2", 0x004FA7B0, RE_BODY(&FirstPersonCameraController_Vf2), nullptr, nullptr, re::Mode::Original);

// 0x50B560 (vtable[3])
void __fastcall VehicleCameraController_Reset(void* self, re::Edx) {
    if (playerPed) {
        VehicleCameraController_Unknown509C50(self, 1.0f);
        VehicleCameraController_Unknown50B2F0(self);
    }
}
CAMERA_REVERSED("VehicleCameraController::Reset", 0x0050B560, RE_BODY(&VehicleCameraController_Reset), nullptr, nullptr, re::Mode::Original);

// 0x5052C0 (vtable[3])
void __fastcall OffsetCameraController_Reset(void* self, re::Edx) {
    At<uint8_t>(self, 0x16C) = 1;
    OffsetCameraController_Unknown5051E0(self);
    VCall<void>(self, 7);
    if (TheCamera.targetPed) {
        Ped_Unknown4732A0(TheCamera.targetPed, 0xFF, 0);
    }
    At<uint8_t>(self, 0x11D) = 1;
    At<uint8_t>(self, 0x16C) = 0;
}
CAMERA_REVERSED("OffsetCameraController::Reset", 0x005052C0, RE_BODY(&OffsetCameraController_Reset), nullptr, nullptr, re::Mode::Original);

// 0x5055D0 (vtable[8]): (the name object is built over the argument's own slot)
uint8_t __fastcall OffsetCameraController_Vf8(void* self, re::Edx, int value) {
    uint32_t name;
    Name_Construct(&name, value);
    uint8_t result = OffsetCameraController_Unknown5050D0(self, &name);
    EmptyThiscall(&name);
    return result;
}
CAMERA_REVERSED("OffsetCameraController::vf8", 0x005055D0, RE_BODY(&OffsetCameraController_Vf8), nullptr, nullptr, re::Mode::Original);

} // namespace
