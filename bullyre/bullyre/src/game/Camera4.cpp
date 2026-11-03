#include "game/Camera.h"

#include <cmath>
#include <cstdint>
#include "reversed.h"
#include "runflags.h"
#include <cstdio>

// Camera part 4 (0x4F0890..0x4F1890): the arcade camera's butes and process, the
// transition's reverse and blend, and CCamera's heading, sphere test, zoom input, fov
// transition and controller requests. (Thiscalls as __fastcall wrappers.)
//
// Found here: CCamera +4 is its view matrix (a CMatrix, the camera's inverse, set by
// 0x4F1F10), +0x94 the previous controller, +0xA0 the heading, +0xC8..+0x104 four
// view-space frustum planes (left, right, bottom, top: normal and an unset d; 0x4F1D10),
// +0x158..+0x164 the fov transition (start ms, duration, from, to) running while +0x16C
// is 2 (back to the default) or 3 (to the override), +0x170 a fov kick that decays by
// +0x178 (from below while +0x174), +0x108 the stick mode (1..3), +0x10C its scale,
// +0x118 its dead zone.

#define C13E_REVERSED(name, address, fn) REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

namespace {

using cam::CVector;
using cam::NiTransform;

constexpr double kPi = 3.1415927410125732;  // (float pi, as a double)

using re::At;

// vtable[index] of an object, as a thiscall
template <typename R, typename... A>
R VCall(void* object, int index, A... args) {
    auto fn = reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(object))[index]));
    return fn(object, args...);
}

// The request 0x4F1890 carries out (CCamera +0x270 is one, 0x4F2C20 fills it)
struct CameraRequest {
    int32_t controller;  // +0
    int32_t duration;    // +4 ms of transition (0: none)
    uint8_t reset;       // +8 reset (vtable[3]) rather than vtable[4]
    uint8_t unknown9[3];
    int32_t value;       // +0xC for vtable[8] (0: not called)
    int32_t kind;        // +0x10 1 and 2 go through while +0x1A8
};
static_assert(sizeof(CameraRequest) == 0x14);

GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);        // CTimer's
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);       // FindPlayerPed
GAME_VAR(uint32_t, arcadeDefault, 0x020C8930);   // the arcade camera's default butes
GAME_VAR(uint8_t, lockButtonInputsExcept, 0x00C1AF8C);
GAME_VAR(uint8_t, allowStickX, 0x00C1AF9F);
GAME_VAR(uint8_t, g_C3B700, 0x00C3B700);
GAME_VAR(int32_t, g_C3B6FC, 0x00C3B6FC);
GAME_VAR(int32_t, g_C3B6F8, 0x00C3B6F8);
inline const uint32_t& butesArcadeName = *reinterpret_cast<const uint32_t*>(0x01B653FC);
inline const uint32_t* const butesArcadeRoot = reinterpret_cast<const uint32_t*>(0x01B653F8);
inline void* const userInputManager = reinterpret_cast<void*>(0x00A45A30);
inline CameraTransition* const transition = reinterpret_cast<CameraTransition*>(0x00A45500);

// the butes
GAME_FN(void, __thiscall, CopyInt, 0x005FA880, uint32_t* dst, const uint32_t* src);
GAME_FN(void, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* self, const char* name);
GAME_FN(void, __thiscall, Path_Build, 0x005FA8C0, void* path, int count, const uint32_t* names);
GAME_FN(uint8_t*, __cdecl, Butes_Find, 0x0072AEA0, const void* path, const uint32_t* root);
GAME_FN(void, __thiscall, EmptyThiscall, 0x0049A650, void* self);
// maths
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __cdecl, VecHeading, 0x00433190, const CVector* v);
GAME_FN(float, __cdecl, VecPitch, 0x00433490, const CVector* v);
GAME_FN(float, __cdecl, AngleDifference, 0x00432980, float a, float b);
GAME_FN(float, __thiscall, Vector_DotProduct, 0x00468CD0, const CVector* a, const CVector* b);
// (0x0045D500 stores 16 bytes through `out4`: called through re::CallVec16)
static inline float* MultiplyPoint4(float* out4, const void* m, const CVector* v) { return re::CallVec16<0x0045D500, float*>(out4, m, v); }  // (writes four floats)
GAME_FN(NiTransform*, __thiscall, NiTransform_Multiply, 0x00406830, const NiTransform* self, NiTransform* out, const NiTransform* other);
GAME_FN(void, __thiscall, NiTransform_MakeIdentity, 0x00770920, NiTransform* self);
GAME_FN(void, __cdecl, CombineRotation, 0x00415EB0, NiTransform* t, const CVector* axis, float degrees, int mode);
GAME_FN(void, __thiscall, NiQuaternion_FromRotation, 0x00776040, float* q, const void* rotation);
GAME_FN(float*, __cdecl, NiQuaternion_Slerp, 0x007766E0, float* out, float t, const float* p, const float* q);
// the camera
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(NiTransform*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(float, __cdecl, Camera_GetNear, 0x006C82E0, void* camera);
GAME_FN(float, __cdecl, Camera_GetFar, 0x006C82D0, void* camera);
GAME_FN(void, __thiscall, Camera_LookAtPoint, 0x004F2C60, CCamera* self, const CVector* position, const CVector* target);
// (registered as ArcadeCameraController::SetOffset: it is TheCamera's +0x180)
GAME_FN(void, __thiscall, Camera_SetOffset180, 0x004F0920, CCamera* self, const CVector* offset);
GAME_FN(float, __thiscall, Camera_GetFovOrOverride, 0x0043A570, CCamera* self);
GAME_FN(CVector*, __thiscall, FollowCamera_GetLookDirection, 0x004FBF10, void* follow, CVector* out);
GAME_FN(void, __cdecl, LockOn_Clear, 0x004DB5F0);
GAME_FN(void, __thiscall, Ped_SetAlpha, 0x004732A0, void* ped, int alpha, int b);
// the input
GAME_FN(uint8_t, __thiscall, UserInput_IsEnabled, 0x0050DF50, void* input);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(float, __cdecl, ApplyDeadZone, 0x004F1520, float value, float deadZone);
// the transition
GAME_FN(int32_t, __thiscall, Transition_IsActive, 0x00856850, CameraTransition* self);  // (a shared getter of +8)
GAME_FN(int32_t, __thiscall, Transition_GetDirection, 0x004F0B00, CameraTransition* self);
GAME_FN(int32_t*, __thiscall, Transition_GetController, 0x004F0B10, CameraTransition* self, int32_t* out, int end);
GAME_FN(void, __thiscall, Transition_Stop, 0x004F0B30, CameraTransition* self);
GAME_FN(void, __thiscall, Transition_Start, 0x004F0B40, CameraTransition* self, int from, int to, int duration);
GAME_FN(float, __thiscall, Transition_GetProgress, 0x004F0B80, CameraTransition* self);
GAME_FN(void, __thiscall, Transition_SetEnd, 0x004F0C00, CameraTransition* self, int end, void* camera, float a, float b);
// (ours, through their addresses)
GAME_FN(void, __thiscall, Transition_Reverse, 0x004F0C90, CameraTransition* self, int duration);
GAME_FN(void, __thiscall, Transition_Blend, 0x004F0D10, CameraTransition* self, NiTransform* t, float* heading, float* fov);

const CVector& Position(uint8_t* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? *reinterpret_cast<CVector*>(matrix + 0x30) : At<CVector>(entity, 4);
}

// ---- ArcadeCameraController (vtable 0x91418C; +4 its butes: +0x18 the height looked
// at over the player, +0x1C the camera's height over that, +0x20 its distance behind)

// 0x4F0890 (vtable[8]): the butes by name (none, or not found: the default); always 1
uint8_t __fastcall ArcadeCameraController_SetButes(uint8_t* self, re::Edx, const char* name) {
    if (name == nullptr) {
        At<uint32_t>(self, 4) = arcadeDefault;
        return 1;
    }
    uint32_t names[2];
    uint32_t path[2];
    CopyInt(&names[0], &butesArcadeName);
    NameKey_Construct(&names[1], name);
    Path_Build(path, 2, names);
    uint8_t* found = Butes_Find(path, butesArcadeRoot);
    At<uint8_t*>(self, 4) = found;
    if (found == nullptr) {
        At<uint32_t>(self, 4) = arcadeDefault;
    }
    for (int i = 1; i >= 0; i--) {
        EmptyThiscall(&names[i]);
    }
    return 1;
}
C13E_REVERSED("ArcadeCameraController::SetButes", 0x004F0890, &ArcadeCameraController_SetButes);

// 0x4F09A0 (vtable[7], process): behind and above the player along -y, looking at a
// point over the player's head
void __fastcall ArcadeCameraController_Process(uint8_t* self, re::Edx) {
    uint8_t* butes = At<uint8_t*>(self, 4);
    CVector zero = {0.0f, 0.0f, 0.0f};
    CVector up = {0.0f, 0.0f, At<float>(butes, 0x18)};
    CVector target;
    VectorAdd(&target, &Position(playerPed), &up);
    float back = -At<float>(butes, 0x20);
    // (the x and z are back * 0.0, the constant at 0x9009A0)
    float side = float(0.0 * double(back));
    CVector offset = {side, back, side};
    CVector position;
    VectorAdd(&position, &target, &offset);
    position.z = float(double(At<float>(butes, 0x1C)) + double(position.z));
    CVector noOffset = {0.0f, 0.0f, 0.0f};
    Camera_LookAtPoint(&TheCamera, &position, &target);
    Camera_SetOffset180(&TheCamera, &noOffset);
    At<CVector>(&TheCamera, 0x18C) = zero;
    TheCamera.unknown198 = target;
}
C13E_REVERSED("ArcadeCameraController::Process", 0x004F09A0, &ArcadeCameraController_Process);

// ---- CameraTransition

// 0x4F0C90: turns a running blend around, from where it stands, over `duration` ms
// (-1: the same duration)
void __fastcall Transition_ReverseOurs(CameraTransition* self, re::Edx, int duration) {
    if (self->active == 0) {
        return;
    }
    float progress = Transition_GetProgress(self);
    if (self->forwards != 0) {
        progress = float(1.0 - double(progress));
    }
    if (duration != -1) {
        self->duration = duration;
    }
    // (truncated, as a 64-bit integer)
    int64_t elapsed = int64_t(double(progress) * double(self->duration));
    self->startTime = timeInMs - uint32_t(elapsed);
    self->t = progress;
    self->forwards = self->forwards == 0 ? 1 : 0;
}
C13E_REVERSED("CameraTransition::Reverse", 0x004F0C90, &Transition_ReverseOurs);

// 0x4F0D10: the blend applied to a transform: the ends' positions eased (a half sine)
// from the first to the second, and, with +0x14, the first end's rotation turned by the
// eased pitch and heading differences; the heading of the result and the ends' +0x3C
// (the fov) eased come out. The blend stops at its end.
void __fastcall Transition_BlendOurs(CameraTransition* self, re::Edx, NiTransform* t, float* heading, float* fov) {
    float progress = Transition_GetProgress(self);
    if ((progress <= 0.0f && self->forwards == 0) || (progress >= 1.0f && self->forwards != 0)) {
        self->active = 0;
    }
    float angle = float((double(progress) - 0.5) * kPi);
    float s = float(std::sin(double(angle)));   // (0x85AEF0: the CRT's x87 sin)
    float w = float((double(s) + 1.0) * 0.5);
    CVector from = self->ends[0].transform.translate;
    CVector to = self->ends[1].transform.translate;
    self->t = progress;
    CVector diff;
    CVector* d = VectorSub(&diff, &to, &from);
    CVector step;
    step.y = float(double(w) * d->y);
    step.z = float(double(d->z) * w);
    step.x = float(double(w) * d->x);
    CVector position;
    VectorAdd(&position, &from, &step);
    if (At<int32_t>(self, 0x14) == 0) {
        // (the slerped rotation is dropped: `t` keeps the rotation it came with)
        float q0[4];
        float q1[4];
        float q[4];
        NiQuaternion_FromRotation(q0, self->ends[0].transform.rotate);
        NiQuaternion_FromRotation(q1, self->ends[1].transform.rotate);
        NiQuaternion_Slerp(q, w, q0, q1);
    } else {
        const float* r0 = self->ends[0].transform.rotate;
        const float* r1 = self->ends[1].transform.rotate;
        CVector at0 = {r0[2], r0[5], r0[8]};
        CVector at1 = {r1[2], r1[5], r1[8]};
        float pitch0 = VecPitch(&at0);
        float heading0 = VecHeading(&at0);
        float pitch1 = VecPitch(&at1);
        float heading1 = VecHeading(&at1);
        float pitchStep = float(double(AngleDifference(pitch0, pitch1)) * w);
        float headingStep = float(double(AngleDifference(heading0, heading1)) * w);
        CVector right = {r0[0], r0[3], r0[6]};
        CVector upAxis = {0.0f, 0.0f, 1.0f};
        CVector back = {-self->ends[0].transform.translate.x, -self->ends[0].transform.translate.y,
                        -self->ends[0].transform.translate.z};
        NiTransform m = self->ends[0].transform;
        NiTransform move;
        move.scale = 1.0f;
        NiTransform_MakeIdentity(&move);
        move.translate = back;
        NiTransform product;
        m = *NiTransform_Multiply(&move, &product, &m);
        CombineRotation(&m, &right, float(-(double(pitchStep) * 180.0 / kPi)), 2);
        CombineRotation(&m, &upAxis, float(double(headingStep) * 180.0 / kPi), 2);
        *t = m;
    }
    NiTransform place;
    place.scale = 1.0f;
    NiTransform_MakeIdentity(&place);
    place.translate = position;
    NiTransform product;
    *t = *NiTransform_Multiply(&place, &product, t);
    float a = self->ends[0].unknown3C;
    *fov = float((double(self->ends[1].unknown3C) - double(a)) * w + double(a));
    // (0x85CF1A: the CRT's x87 atan2)
    *heading = float(std::atan2(double(t->rotate[2]), double(t->rotate[5])));
}
C13E_REVERSED("CameraTransition::Blend", 0x004F0D10, &Transition_BlendOurs);

// 0x4F1130: the blend applied to a camera's local transform
void __fastcall Transition_BlendCamera(CameraTransition* self, re::Edx, void* camera, float* heading, float* fov) {
    void* node = GetCameraNode(camera);
    Transition_Blend(self, GetLocalTransform(node), heading, fov);
}
C13E_REVERSED("CameraTransition::BlendCamera", 0x004F1130, &Transition_BlendCamera);

// ---- CCamera

// 0x4F1220: |round(+0x17C * +0x170)| (as the game does it: +0.5 and truncated, then
// through a float). The product and the sum in float (the main thread's x87 runs at
// 24 bits); each _ftol is the CRT's SSE2 one (cvttsd2si: 0x80000000 out of range), as
// the casts from float compile here.
int32_t __fastcall Camera_GetKickScaled(CCamera* self, re::Edx) {
    float product = float(double(At<int32_t>(self, 0x17C)) * double(At<float>(self, 0x170)));
    int32_t rounded = int32_t(product + 0.5f);
    float magnitude = float(std::fabs(double(rounded)));
    return int32_t(magnitude);
}
C13E_REVERSED("CCamera::GetKickScaled", 0x004F1220, &Camera_GetKickScaled);

// 0x4F1310: the heading: the follow camera's look direction while it runs unsaved,
// otherwise +0xA0
float __fastcall Camera_GetHeading(CCamera* self, re::Edx) {
    if (self->activeController == 1 && self->savedController == 0) {
        CVector direction;
        FollowCamera_GetLookDirection(self->controllers[1], &direction);
        return float(std::atan2(double(direction.x), double(direction.y)));
    }
    return self->unknownA0;
}
C13E_REVERSED("CCamera::GetHeading", 0x004F1310, &Camera_GetHeading);

// 0x4F13D0: a sphere in the view (in view space: y is the depth, between the near and
// far planes, then the four side planes)
int32_t __fastcall Camera_IsSphereVisible(CCamera* self, re::Edx, const CVector* centre, float radius) {
    float v[4];
    MultiplyPoint4(v, reinterpret_cast<uint8_t*>(self) + 4, centre);
    double nearDepth = double(v[1]) + double(radius);
    if (double(Camera_GetNear(self->camera)) > nearDepth) {
        return 0;
    }
    double farDepth = double(v[1]) - double(radius);
    if (double(Camera_GetFar(self->camera)) < farDepth) {
        return 0;
    }
    float* p = &At<float>(self, 0xC8);
    float d = float((double(p[1]) * v[1] + double(p[0]) * v[0]) + double(p[2]) * v[2]);
    if (radius < d) {
        return 0;
    }
    d = float((double(p[5]) * v[1] + double(p[4]) * v[0]) + double(p[6]) * v[2]);
    if (d > radius) {
        return 0;
    }
    d = float((double(p[8]) * v[0] + double(p[9]) * v[1]) + double(v[2]) * p[10]);
    if (d > radius) {
        return 0;
    }
    if (radius < Vector_DotProduct(reinterpret_cast<const CVector*>(v), reinterpret_cast<const CVector*>(&p[12]))) {
        return 0;
    }
    if (re::PopulationPass() == 2) {
        // (the remaster's second population pass, runflags.h: the camera is the main one moved
        // through a seamless door, and what it sees of the area there is what the doorway shows)
        return re::SeamlessSeenThroughDoorway(&centre->x, radius) ? 1 : 0;
    }
    return 1;
}
C13E_REVERSED("CCamera::IsSphereVisible", 0x004F13D0, &Camera_IsSphereVisible);

// 0x4F1580: a stick axis by the stick mode (+0x108: 3 control 0x10, or 0x12 when that
// is 0; 2 0x10; 1 0x12), past the dead zone (+0x118), scaled (+0x10C); 0 without input
float __fastcall Camera_GetStickZoom(CCamera* self, re::Edx) {
    if (!UserInput_IsEnabled(userInputManager)) {
        return 0.0f;
    }
    if (lockButtonInputsExcept != 0 && allowStickX == 0) {
        return 0.0f;
    }
    float value = 0.0f;
    int32_t mode = At<int32_t>(self, 0x108);
    if (mode == 3) {
        value = Pad_GetValue(0x10, 0);
        if (value == 0.0f) {
            value = Pad_GetValue(0x12, 0);
        }
    } else if (mode == 2) {
        value = Pad_GetValue(0x10, 0);
    } else if (mode == 1) {
        value = Pad_GetValue(0x12, 0);
    }
    return float(double(ApplyDeadZone(value, At<float>(self, 0x118))) * At<float>(self, 0x10C));
}
C13E_REVERSED("CCamera::GetStickZoom", 0x004F1580, &Camera_GetStickZoom);

// 0x4F1670: the fov transition (eased: a half sine), with the kick (+0x170) at least
// the ease; without one the kick decays to 0 by +0x178
void __fastcall Camera_UpdateFovTransition(CCamera* self, re::Edx) {
    int32_t state = self->fovOverridden;
    if (state == 0 || state == 1) {
        if (At<int32_t>(self, 0x174) != 0) {
            float kick = float(double(At<float>(self, 0x178)) + double(At<float>(self, 0x170)));
            At<float>(self, 0x170) = kick;
            if (kick >= 0.0f) {
                At<float>(self, 0x170) = 0.0f;
            }
        } else {
            float kick = float(double(At<float>(self, 0x170)) - double(At<float>(self, 0x178)));
            At<float>(self, 0x170) = kick;
            if (kick <= 0.0f) {
                At<float>(self, 0x170) = 0.0f;
            }
        }
        return;
    }
    int32_t duration = At<int32_t>(self, 0x15C);
    float t;
    if (duration != 0) {
        // (the elapsed time as unsigned)
        uint32_t elapsed = timeInMs - At<uint32_t>(self, 0x158);
        t = float(double(elapsed) / double(duration));
    } else {
        t = 1.0f;
    }
    if (t < 1.0f) {
        float angle = float((double(t) - 0.5) * kPi);
        float s = float(std::sin(double(angle)));
        float w = float((double(s) + 1.0) * 0.5);
        float from = At<float>(self, 0x160);
        self->fovOverride = float((double(At<float>(self, 0x164)) - double(from)) * w + double(from));
        float kick = float(std::fabs(double(At<float>(self, 0x170))));
        if (kick < w) {
            At<float>(self, 0x170) = w;
        }
        return;
    }
    self->fovOverride = At<float>(self, 0x164);
    At<int32_t>(self, 0x158) = 0;
    At<int32_t>(self, 0x174) = 0;
    if (state == 2) {
        self->fovOverridden = 0;
    } else if (state == 3) {
        self->fovOverridden = 1;
    }
}
C13E_REVERSED("CCamera::UpdateFovTransition", 0x004F1670, &Camera_UpdateFovTransition);

// 0x4F1890: carries out a controller request (while +0x1A4, or for kinds 1 and 2 while
// +0x1A8): the same controller only gets vtable[8] (and a reset); another becomes the
// active one, blended over the duration (a running blend between the two is turned
// around, another stopped); the request is cleared (+0x270)
void __fastcall Camera_ApplyRequest(CCamera* self, re::Edx, const CameraRequest* request) {
    if (re::DoorReport()) {
        std::fprintf(stderr, "doors: camera request: controller %d value %d kind %d for %d ms at %u ms (active %d)\n", request->controller,
                     request->value, request->kind, request->duration, *reinterpret_cast<const uint32_t*>(0x00C1A9B4),
                     self->activeController);
    }
    // (the remaster's seamless doors: a door's own camera, controller 4, left out while its other
    // side is drawn through it, runflags.h)
    if (request->controller == 4 && re::SeamlessDoorHoldsCamera("the door camera (controller 4)")) {
        self->unknown270 = 0;
        return;
    }
    bool special = (request->kind == 1 || request->kind == 2) && self->unknown1A8 != 0;
    if (At<int32_t>(self, 0x1A4) == 0 && !special) {
        self->unknown270 = 0;
        return;
    }
    g_C3B700 = 0;
    g_C3B6FC = -1;
    g_C3B6F8 = -1;
    int32_t active = self->activeController;
    if (active == request->controller) {
        if (request->value != 0) {
            VCall<void>(self->controllers[active], 8, request->value);
            if (request->reset != 0) {
                VCall<void>(self->controllers[self->activeController], 3);
            }
        }
        self->unknown270 = 0;
        return;
    }
    At<int32_t>(self, 0x94) = active;
    self->activeController = request->controller;
    if (request->value != 0) {
        VCall<void>(self->controllers[request->controller], 8, request->value);
    }
    CameraController* controller = self->controllers[request->controller];
    VCall<void>(controller, request->reset != 0 ? 3 : 4);
    LockOn_Clear();
    if (request->duration <= 0 || request->controller == 2 || self->activeController == 2) {
        Transition_Stop(transition);
        void* ped = self->targetPed;
        if (ped != nullptr && At<int32_t>(ped, 0x1384) == 0) {
            Ped_SetAlpha(TheCamera.targetPed, 0xFF, 0);
        }
        self->unknown270 = 0;
        return;
    }
    if (Transition_IsActive(transition) != 0) {
        int32_t end;
        if ((*Transition_GetController(transition, &end, 0) == request->controller && Transition_GetDirection(transition) != 0) ||
            (*Transition_GetController(transition, &end, 1) == request->controller && Transition_GetDirection(transition) == 0)) {
            Transition_Reverse(transition, request->duration);
        } else {
            Transition_Stop(transition);
        }
        self->unknown270 = 0;
        return;
    }
    float fov = Camera_GetFovOrOverride(self);
    Transition_SetEnd(transition, 0, self->camera, self->unknownA0, fov);
    fov = Camera_GetFovOrOverride(self);
    Transition_SetEnd(transition, 1, self->camera, self->unknownA0, fov);
    Transition_Start(transition, At<int32_t>(self, 0x94), self->activeController, request->duration);
    self->unknown270 = 0;
}
C13E_REVERSED("CCamera::ApplyRequest", 0x004F1890, &Camera_ApplyRequest);

} // namespace
