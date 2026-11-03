#pragma once

// The camera: CCamera (TheCamera, 0xC3CC68), its controllers (one per kind of view:
// follow, fight, vehicle, first person, conversation, cutscene, script, store, ...,
// each a CameraController with the same virtual interface), their helpers (the
// transition between two controllers, the spline interpolater the script camera
// runs on), and the input manager beside them.
//
// The camera itself is a Gamebryo NiCamera: its node's local transform (NiTransform:
// 3x3 rotation, translation, scale) is where the controllers put the view.
//
// Controller vtable, as far as it's used here:
//   [0] +0x00 scalar deleting destructor
//   [3] +0x0C reset (what [4], [5] and [6] fall back to)
//   [5] +0x14 (CVector, int) / [6] +0x18 (CVector, int, int): look-at variants
//   [7] +0x1C process
//   [8] +0x20 (int)

#include <cstddef>
#include <cstdint>

#include "reversed.h"

// (in a namespace: the math types aren't shared yet, other files have their own)
namespace cam {

struct CVector {
    float x, y, z;
};
static_assert(sizeof(CVector) == 0xC);

// Gamebryo's NiTransform (what 0x6C9A90 returns for a node: its local transform)
struct NiTransform {
    float rotate[9];     // +0
    CVector translate;   // +0x24
    float scale;         // +0x30
};
static_assert(sizeof(NiTransform) == 0x34);

} // namespace cam

// The base of every controller (4 bytes: CCamera::SetController makes bare ones)
struct CameraController {
    re::Ptr32<void> vtable;  // +0 (0x914160 for the base)
};
static_assert(sizeof(CameraController) == 4);

// The blend between two controllers' views (where it sits in CCamera: not pinned
// down). Two ends of 0x40 bytes, from +0x18.
struct CameraTransitionEnd {
    cam::NiTransform transform;  // +0
    float unknown34;             // +0x34
    int32_t controller;          // +0x38 which controller this end is
    float unknown3C;             // +0x3C
};
static_assert(sizeof(CameraTransitionEnd) == 0x40);

struct CameraTransition {
    uint32_t startTime;          // +0 (CTimer ms)
    int32_t duration;            // +4 ms
    int32_t active;              // +8
    int32_t forwards;            // +0xC 0: the blend runs from 1 down to 0
    float t;                     // +0x10
    uint8_t unknown14[4];        // +0x14
    CameraTransitionEnd ends[2]; // +0x18

    // 0x4F0AC0 (thiscall): the weight of a controller in the blend (1 without one)
    float GetWeight(int controller);
    // 0x4F0B00 (thiscall)
    int32_t GetDirection();
    // 0x4F0B10 (thiscall): *out = ends[end].controller
    int32_t* GetController(int32_t* out, int end);   // (returns out)
    // 0x4F0B30 (thiscall)
    void Stop();
    // 0x4F0B40 (thiscall): from one controller to another over `duration` ms (0: no
    // blend running)
    void Start(int from, int to, int duration);
    // 0x4F0B80 (thiscall): how far along, 0..1 (reversed when not forwards)
    float GetProgress();
    // 0x4F0C00 (thiscall): an end from a camera's transform
    void SetEnd(int end, void* camera, float a, float b);
    // 0x4F0C40 (thiscall): an end back to a camera's transform
    void GetEnd(int end, void* camera, float* a, float* b);
};
static_assert(offsetof(CameraTransition, ends) == 0x18);
static_assert(offsetof(CameraTransition, ends[1].controller) == 0x90);

// CCamera (partial: the size isn't known)
struct CCamera {
    uint8_t unknown0[0x48];          // +0
    uint8_t unknown48[0x40];         // +0x48 (0x412A70 sets it from the frame)
    re::Ptr32<void> frame;                     // +0x88 the camera's node (0x6C81D0 of camera)
    re::Ptr32<void> camera;                    // +0x8C the NiCamera
    int32_t activeController;        // +0x90 index into controllers
    uint8_t unknown94[8];            // +0x94
    float fov;                       // +0x9C
    float unknownA0;                 // +0xA0
    uint8_t unknownA4[0xC];          // +0xA4
    int32_t fadeState;               // +0xB0 0 faded in, 1 faded out, 2 fading
    int32_t fadeTarget;              // +0xB4
    float fadeDuration;              // +0xB8 seconds (less a frame)
    float fadeStart;                 // +0xBC seconds
    float fadeAlpha;                 // +0xC0
    uint8_t fadeColour[4];           // +0xC4 RGBA: [3] the alpha faded to
    uint8_t unknownC8[0xA0];         // +0xC8
    float fovOverride;               // +0x168
    int32_t fovOverridden;           // +0x16C
    uint8_t unknown170[0x28];        // +0x170
    cam::CVector unknown198;         // +0x198
    uint8_t unknown1A4[4];           // +0x1A4 (tested as a dword)
    uint8_t unknown1A8;              // +0x1A8
    uint8_t unknown1A9[3];           // +0x1A9
    re::Ptr32<void> targetPed;                 // +0x1AC (a CPed)
    uint8_t unknown1B0[0x7C];        // +0x1B0
    re::Ptr32<CameraController> controllers[17]; // +0x22C (count: up to +0x270)
    int32_t unknown270;              // +0x270
    int32_t unknown274;              // +0x274
    uint8_t unknown278;              // +0x278
    uint8_t unknown279[3];           // +0x279
    int32_t unknown27C;              // +0x27C
    int32_t unknown280;              // +0x280
    uint8_t unknown284[0x14];        // +0x284
    int32_t savedController;         // +0x298 0: none
    cam::NiTransform savedTransform; // +0x29C the view when it was saved
    uint8_t unknown2D0[0x40];        // +0x2D0
    float savedFov;                  // +0x310

    // 0x4F11C0 (thiscall): replaces a controller (deleting the old one; nullptr: a
    // bare CameraController)
    void SetController(int index, CameraController* controller);
    // 0x4F1250 (thiscall): the default fov over the current one (a zoom factor)
    float GetZoom();
    // 0x4F12A0 (thiscall)
    uint8_t GetUnknown1A8();
    // 0x4F12C0 (thiscall): controller vtable[8], when +0x1A4 is set
    void CallController20(int index, int value);
    // 0x4F12F0 (thiscall): the fade's state (2 while one runs)
    int32_t GetFadeState();
    // 0x4F1390 (thiscall)
    float GetFov();
    // 0x4F1640 (thiscall): the follow camera's +0x30 when it's the active one
    uint8_t IsFollowCameraFlag30();
    // 0x4F1660 (thiscall)
    void ResetFov();
    // 0x4F1800 (thiscall)
    void SetCamera(void* camera);
    // 0x4F1830 (thiscall): saves the view and resets the controller given
    void SaveView(int controller);
    // 0x4F1AD0 (thiscall): fades in (0) or out (1) over `seconds` (the third
    // argument is unused)
    void Fade(float seconds, int direction, int unused);
    // 0x4F2B60 (thiscall): resets the active controller (and the fov)
    void ResetController();
    // 0x4F2B90 (thiscall): the active controller's vtable[5]
    void ControllerLookAt(cam::CVector target);
    // 0x4F2BD0 (thiscall): the active controller's vtable[6]
    void ControllerLookAt2(cam::CVector target, int value);
};
static_assert(offsetof(CCamera, frame) == 0x88);
static_assert(offsetof(CCamera, fov) == 0x9C);
static_assert(offsetof(CCamera, fadeState) == 0xB0);
static_assert(offsetof(CCamera, fadeColour) == 0xC4);
static_assert(offsetof(CCamera, fovOverride) == 0x168);
static_assert(offsetof(CCamera, unknown198) == 0x198);
static_assert(offsetof(CCamera, targetPed) == 0x1AC);
static_assert(offsetof(CCamera, controllers) == 0x22C);
static_assert(offsetof(CCamera, unknown280) == 0x280);
static_assert(offsetof(CCamera, savedController) == 0x298);
static_assert(offsetof(CCamera, savedFov) == 0x310);

GAME_VAR(CCamera, TheCamera, 0x00C3CC68);

// The spline the script camera moves along (0x50 bytes; two in ScriptCameraController)
struct SplineInterpolater {
    re::Ptr32<void> vtable;                // +0 (0x9146FC)
    uint8_t unknown4[0x24];      // +4
    int32_t unknown28;           // +0x28
    float unknown2C;             // +0x2C
    float unknown30;             // +0x30
    float unknown34;             // +0x34
    float unknown38;             // +0x38
    float unknown3C;             // +0x3C
    float unknown40;             // +0x40
    re::Ptr32<void> points;                // +0x44 (allocated)
    int32_t unknown48;           // +0x48
    int32_t unknown4C;           // +0x4C
};
static_assert(sizeof(SplineInterpolater) == 0x50);

// The input manager (0x50DE30 builds it): six input handlers, flags that each let
// input through, DirectInput
struct UserInputManager {
    uint8_t unknown0[4];       // +0
    re::Ptr32<void> handlers[6];         // +4
    uint8_t enabled[5];        // +0x1C all must be set for input
    uint8_t unknown21[3];      // +0x21
    re::Ptr32<void> directInput;         // +0x24 IDirectInput8
};
static_assert(offsetof(UserInputManager, directInput) == 0x24);
