#include "game/Camera.h"

#include <cmath>
#include <cstdint>
#include "reversed.h"
#include "runflags.h"

// Camera part 5 (0x4F1BE0..0x4F26E0): CCamera's letterbox bars, frustum planes, heading
// and view matrix from the camera node, the fade, input events, the controller's
// process, the projection to the screen, the buttons and the stick look. (Thiscalls
// on TheCamera as __fastcall wrappers.)
//
// Found here: +0xA4 the letterbox wanted, +0xA8 shown, +0xAC its slide (0..1);
// +0x11C the stick look's speed, +0x120 its heading, +0x124/+0x128 the turn and tilt
// this frame, +0x12C..+0x13C its tuning; +0x284..+0x295 nine button pairs (this frame,
// last frame); +0x319 set while the action tree (+0x268) updates.

#define C13F_REVERSED(name, address, fn) REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

namespace {

using cam::CVector;
using cam::NiTransform;

constexpr double kDegToRad = 0.017453293005625408;  // (float pi / 180, as a double)

using re::At;

// vtable[index] of an object, as a thiscall
template <typename R, typename... A>
R VCall(void* object, int index, A... args) {
    auto fn = reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(object))[index]));
    return fn(object, args...);
}

// GTA's CMatrix: a RwMatrix and the transform it's attached to
struct CamMatrix {
    CVector right;     // +0
    uint32_t flags;    // +0xC
    CVector up;        // +0x10
    uint32_t pad1;     // +0x1C
    CVector at;        // +0x20
    uint32_t pad2;     // +0x2C
    CVector pos;       // +0x30
    uint32_t pad3;     // +0x3C
    re::Ptr32<void> attached;    // +0x40
};
static_assert(sizeof(CamMatrix) == 0x44);

GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);          // CTimer's
GAME_VAR(float, timeStep, 0x00C1A9A4);             // CTimer::ms_fTimeStep
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);         // FindPlayerPed
GAME_VAR(uint8_t*, cameraSettings, 0x020C88B8);    // +0x74.. aspect ratios
GAME_VAR(uint8_t, widescreen, 0x00C1870E);
GAME_VAR(uint8_t, invertLook, 0x00C18736);         // the frontend's invert camera option
GAME_VAR(float, cameraStepScale, 0x00A455A4);      // (0.02)
GAME_VAR(float, stickLimitNoPad, 0x00A1491C);      // the stick's reach without 0x738500 (0)
inline void* const userInputManager = reinterpret_cast<void*>(0x00A45A30);
inline CameraTransition* const transition = reinterpret_cast<CameraTransition*>(0x00A45500);

// the camera
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(NiTransform*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(void*, __cdecl, GetCameraFrame, 0x006C81D0, void* camera);
GAME_FN(float, __cdecl, Camera_GetNear, 0x006C82E0, void* camera);
GAME_FN(uint8_t*, __cdecl, Camera_GetRaster, 0x006C8740, void* camera);  // +0xC width, +0x10 height, +0x1C/+0x1E the left and top
GAME_FN(CVector*, __thiscall, Transform_MultiplyPoint, 0x00450B10, const void* m, CVector* out, const CVector* v);
GAME_FN(void, __thiscall, Camera_SaveView, 0x004F1830, CCamera* self, int controller);
GAME_FN(void, __thiscall, LookLimits_Apply, 0x004F3FA0, void* limits);
GAME_FN(int32_t, __thiscall, Transition_IsActive, 0x00856850, CameraTransition* self);  // (a shared getter of +8)
GAME_FN(void, __cdecl, ResetGradeDisplay, 0x004F1190);
GAME_FN(int32_t, __cdecl, IsGradeDisplayIdle, 0x004F1160);
// matrices
GAME_FN(CamMatrix*, __cdecl, Matrix_Invert, 0x00413770, const CamMatrix* in, CamMatrix* out);
GAME_FN(void, __thiscall, Matrix_CopyConstruct, 0x00412A10, CamMatrix* self, const CamMatrix* other);
GAME_FN(void, __thiscall, Matrix_Assign, 0x00412720, void* self, const CamMatrix* other);
// drawing
GAME_FN(uint8_t*, __thiscall, RGBA_Construct, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, DrawRect, 0x0052C470, const float* rect, const uint8_t* colour);
// input
GAME_FN(uint8_t, __thiscall, UserInput_IsEnabled, 0x0050DF50, void* input);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(uint8_t, __cdecl, Pad_ButtonJustUp, 0x00738940, int button, int pad);
GAME_FN(uint8_t, __cdecl, Pad_JustPressed, 0x00738900, int control, int pad);
GAME_FN(int, __cdecl, GetActivePad, 0x00738070);
GAME_FN(uint8_t, __cdecl, Fn_738500, 0x00738500, int pad);
GAME_FN(void*, __thiscall, Ped_GetTarget, 0x00497100, void* ped);
// the action tree
GAME_FN(void, __thiscall, Controller_Play, 0x005F45F0, void* controller, void* node);
GAME_FN(void, __thiscall, ActionController_Update, 0x005F4B00, void* self, float dt);

// 0x4F1BE0: the letterbox bars (black, 640x480 space), sliding in or out by 0.15 a
// call while the shown state isn't the wanted one
void __fastcall Camera_DrawLetterbox(CCamera* self, re::Edx) {
    int32_t wanted = At<int32_t>(self, 0xA4);
    int32_t shown = At<int32_t>(self, 0xA8);
    float height;
    float bottom;
    if (wanted != shown) {
        float slide = float(double(At<float>(self, 0xAC)) + 0.15000000596046448);
        At<float>(self, 0xAC) = slide;
        if (!(slide < 1.0f)) {
            slide = 1.0f;
        }
        At<float>(self, 0xAC) = slide;
        double f = shown != 0 ? 1.0 - double(slide) : double(slide);
        height = float(f * 50.0);
        if (slide == 1.0f) {
            shown = wanted;
        }
        At<int32_t>(self, 0xA8) = shown;
        bottom = float(480.0 - double(height));
    } else {
        height = 50.0f;
        bottom = 430.0f;
    }
    float rect[4] = {0.0f, 0.0f, 640.0f, height};
    uint8_t colour[4];
    DrawRect(rect, RGBA_Construct(colour, 0, 0, 0, 0xFF));
    rect[0] = 0.0f;
    rect[1] = bottom;
    rect[2] = 640.0f;
    rect[3] = 480.0f;
    DrawRect(rect, RGBA_Construct(colour, 0, 0, 0, 0xFF));
}
C13F_REVERSED("CCamera::DrawLetterbox", 0x004F1BE0, &Camera_DrawLetterbox);

// 0x4F1D10: the view-space side planes (+0xC8: left, right, bottom, top) from the fov
// and the aspect ratio (the widescreen one when set)
void __fastcall Camera_UpdateFrustumPlanes(CCamera* self, re::Edx) {
    float* p = &At<float>(self, 0xC8);
    float half = float(double(self->fov) * kDegToRad * 0.5);
    float side = half;
    if (widescreen != 0 && re::WiderThan169() > 1.0f) {
        side = float(std::atan(std::tan(double(half)) * re::WiderThan169()));   // (wider than 16:9: runflags.h)
    }
    float c = float(std::cos(double(side)));   // (0x85AD80, 0x85AEF0: the CRT's x87 cos, sin)
    float s = float(std::sin(double(side)));
    // (each plane's fourth float is the game's uninitialised stack)
    float unset = 0.0f;
    p[0] = c;
    p[1] = -s;
    p[2] = 0.0f;
    p[3] = unset;
    p[4] = -c;
    p[5] = -s;
    p[6] = 0.0f;
    p[7] = unset;
    float aspect = float(double(At<float>(cameraSettings, 0x74)) / double(At<float>(cameraSettings, 0x78)));
    if (widescreen != 0) {
        aspect = float(double(At<float>(cameraSettings, 0x7C)) / double(At<float>(cameraSettings, 0x80)));
    }
    float vertical = float(double(aspect) * half);
    float cv = float(std::cos(double(vertical)));
    float sv = float(std::sin(double(vertical)));
    p[8] = 0.0f;
    p[9] = -sv;
    p[10] = -cv;
    p[11] = unset;
    p[12] = 0.0f;
    p[13] = -sv;
    p[14] = cv;
    p[15] = unset;
}
C13F_REVERSED("CCamera::UpdateFrustumPlanes", 0x004F1D10, &Camera_UpdateFrustumPlanes);

// 0x4F1EC0: the heading (+0xA0) from the camera node's rotation
void __fastcall Camera_UpdateHeading(CCamera* self, re::Edx) {
    void* node = GetCameraNode(self->camera);
    NiTransform* t = GetLocalTransform(node);
    // (0x85CF1A: the CRT's x87 atan2)
    self->unknownA0 = float(std::atan2(double(t->rotate[2]), double(t->rotate[5])));
}
C13F_REVERSED("CCamera::UpdateHeading", 0x004F1EC0, &Camera_UpdateHeading);

// 0x4F1F10: the view matrix (+4): the camera node's transform as a CMatrix (the
// rotation's columns as right, at and up swapped), inverted
void __fastcall Camera_UpdateViewMatrix(CCamera* self, re::Edx) {
    CamMatrix m;
    CamMatrix inverse;
    m.attached = nullptr;
    void* node = GetCameraNode(self->camera);
    NiTransform* t = GetLocalTransform(node);
    inverse.attached = nullptr;
    const float* r = t->rotate;
    m.right = {r[0], r[3], r[6]};
    m.up = {r[2], r[5], r[8]};
    m.at = {r[1], r[4], r[7]};
    m.pos = t->translate;
    // (the flags and pads are the game's uninitialised stack)
    m.flags = 0;
    m.pad1 = 0;
    m.pad2 = 0;
    m.pad3 = 0;
    CamMatrix* result = Matrix_Invert(&m, &inverse);
    CamMatrix copy;
    Matrix_CopyConstruct(&copy, result);
    Matrix_Assign(reinterpret_cast<uint8_t*>(self) + 4, &copy);
}
C13F_REVERSED("CCamera::UpdateViewMatrix", 0x004F1F10, &Camera_UpdateViewMatrix);

// 0x4F2090: the fade's alpha by time (in: 255 down to 0; out: up to the colour's alpha,
// then the grade display is reset unless +0x1A9)
void __fastcall Camera_UpdateFade(CCamera* self, re::Edx) {
    int32_t target = self->fadeTarget;
    if (self->fadeState == target) {
        return;
    }
    double now = double(timeInMs);
    double elapsed = now - double(self->fadeStart) * 1000.0;
    double duration = double(self->fadeDuration) * 1000.0;
    if (target == 0) {
        float alpha = float((1.0 - elapsed / duration) * 255.0);
        self->fadeAlpha = alpha;
        if (alpha <= 0.0f) {
            self->fadeAlpha = 0.0f;
            self->fadeState = 0;
        }
        return;
    }
    float alpha = float(elapsed / duration * 255.0);
    self->fadeAlpha = alpha;
    int32_t full = self->fadeColour[3];
    if (double(full) <= double(alpha)) {
        self->fadeState = target;
        self->fadeAlpha = float(full);
        if (self->unknown1A9[0] == 0) {
            ResetGradeDisplay();
        }
    }
}
C13F_REVERSED("CCamera::UpdateFade", 0x004F2090, &Camera_UpdateFade);

// 0x4F2190: an input event to the controllers (vtable[9]): the saved one (taking it
// gives the view back), then the active one; 1 when taken, or when the event's +8 is
// not above 0
int32_t __fastcall Camera_HandleEvent(CCamera* self, re::Edx, void* event) {
    if (self->unknown0[0] == 0) {
        return 0;
    }
    if (self->savedController != 0) {
        if (VCall<uint8_t>(self->controllers[self->savedController], 9, event) != 0) {
            Camera_SaveView(self, 0);
        }
    }
    if (VCall<uint8_t>(self->controllers[self->activeController], 9, event) != 0) {
        return 1;
    }
    if (At<float>(event, 8) <= 0.0f) {
        return 1;
    }
    return 0;
}
C13F_REVERSED("CCamera::HandleEvent", 0x004F2190, &Camera_HandleEvent);

// 0x4F2200: with a target ped, the active controller's process, then the look limits
// (not while a transition runs)
void __fastcall Camera_ProcessController(CCamera* self, re::Edx) {
    if (self->targetPed == nullptr) {
        return;
    }
    VCall<void>(self->controllers[self->activeController], 7);
    if (Transition_IsActive(transition) == 0) {
        LookLimits_Apply(reinterpret_cast<uint8_t*>(self) + 0x208);
    }
}
C13F_REVERSED("CCamera::ProcessController", 0x004F2200, &Camera_ProcessController);

// 0x4F2240: a point to the screen: 0 behind the near plane or off the raster; `out` is
// (x, y, 1) over the depth (in raster pixels unless `normalised`)
int32_t __fastcall Camera_ProjectToScreen(CCamera* self, re::Edx, const CVector* point, float* out, int normalised) {
    void* frame = GetCameraFrame(self->camera);
    CVector projected;
    CVector v = *Transform_MultiplyPoint(frame, &projected, point);
    double depth = double(v.z);
    if (!(double(Camera_GetNear(self->camera)) < depth)) {
        return 0;
    }
    uint8_t* raster = Camera_GetRaster(self->camera);
    int32_t width = At<int32_t>(raster, 0xC);
    int32_t height = At<int32_t>(raster, 0x10);
    int32_t left = At<int16_t>(raster, 0x1C);
    int32_t top = At<int16_t>(raster, 0x1E);
    float x = float(double(v.x) / double(v.z));
    out[0] = x;
    float y = float(double(v.y) / double(v.z));
    out[1] = y;
    out[2] = 1.0f;
    if (!(double(left) <= double(x))) {
        return 0;
    }
    if (!(x < 1.0f)) {
        return 0;
    }
    if (!(double(top) <= double(y))) {
        return 0;
    }
    if (!(y < 1.0f)) {
        return 0;
    }
    if (normalised == 0) {
        out[0] = float(double(width) * x);
        out[1] = float(double(y) * height);
    }
    return 1;
}
C13F_REVERSED("CCamera::ProjectToScreen", 0x004F2240, &Camera_ProjectToScreen);

// the buttons' last-frame copies (+0x285, +0x287, ... from +0x284, +0x286, ...)
void KeepLastButtons(uint8_t* p) {
    for (int i = 0x284; i <= 0x294; i += 2) {
        p[i + 1] = p[i];
    }
}

float ActionStep() {
    float step = float(double(cameraStepScale) * double(timeStep));
    return float(double(step) * 0.5);
}

// 0x4F2390: the camera's buttons (when `readPad` and input is on), then, with a target
// ped, the action tree started when idle and updated twice by half steps (the buttons
// moved to last frame's between)
void __fastcall Camera_UpdateButtons(CCamera* self, re::Edx, int readPad) {
    uint8_t* p = reinterpret_cast<uint8_t*>(self);
    if (re::Flag(readPad) && UserInput_IsEnabled(userInputManager)) {
        KeepLastButtons(p);
        p[0x284] = (Ped_GetTarget(playerPed) != nullptr && Pad_GetValue(10, 0) != 0.0f) ? 1 : 0;
        p[0x286] = Pad_ButtonJustUp(2, 0);
        p[0x288] = (Pad_GetValue(9, 0) != 0.0f || Pad_GetValue(6, 0) != 0.0f || Pad_GetValue(7, 0) != 0.0f ||
                    Pad_GetValue(8, 0) != 0.0f || (Pad_JustPressed(3, 0) != 0 && re::Flag(IsGradeDisplayIdle())))
                       ? 1
                       : 0;
        p[0x28A] = Pad_GetValue(6, 0) != 0.0f ? 1 : 0;
        p[0x28C] = Pad_GetValue(7, 0) != 0.0f ? 1 : 0;
        p[0x28E] = 0;
        p[0x290] = Pad_GetValue(9, 0) != 0.0f ? 1 : 0;
        p[0x292] = Pad_GetValue(2, 0) != 0.0f ? 1 : 0;
        p[0x294] = Pad_GetValue(3, 0) != 0.0f ? 1 : 0;
    }
    if (self->targetPed == nullptr) {
        return;
    }
    if (At<int32_t>(At<uint8_t*>(p, 0x268), 0x10) == 0) {
        uint8_t* context = At<uint8_t*>(p, 0x26C);
        void* tree = At<void*>(context, 0x40);
        if (tree != nullptr) {
            At<int32_t>(context, 0x44) = 0;
            void* node = VCall<void*>(tree, 0x4C / 4, context);
            Controller_Play(At<void*>(context, 0), node);
        }
    }
    p[0x319] = 1;
    ActionController_Update(At<void*>(p, 0x268), ActionStep());
    KeepLastButtons(p);
    ActionController_Update(At<void*>(p, 0x268), ActionStep());
    p[0x319] = 0;
}
C13F_REVERSED("CCamera::UpdateButtons", 0x004F2390, &Camera_UpdateButtons);

// the stick's length (as the game rounds it)
float StickLength(float x, float y) {
    float xx = float(double(x) * x);
    float yy = float(double(y) * y);
    float sum = float(double(xx) + double(yy));
    sum = float(std::fabs(double(sum)));
    return float(std::sqrt(double(sum)));   // (0x85C500: the CRT's x87 sqrt)
}

// 0x4F26E0: the stick look by the stick mode (+0x108: 3 both sticks added, limited; 2
// controls 0x10/0x11; 1 0x12/0x13; others the raw values): the speed (+0x11C) eased by
// the push along the last heading and capped by a curve of the push, the heading
// (+0x120), and this frame's turn (+0x124) and tilt (+0x128, inverted by the option)
void __fastcall Camera_UpdateStickLook(CCamera* self, re::Edx) {
    float p10 = Pad_GetValue(0x10, 0);
    float p12 = Pad_GetValue(0x12, 0);
    float p11 = Pad_GetValue(0x11, 0);
    float p13 = Pad_GetValue(0x13, 0);
    int32_t mode = At<int32_t>(self, 0x108);
    float x = p12;
    float y = p11;
    float length = p13;
    if (mode == 3) {
        x = float(double(p12) + double(p10));
        y = float(double(p13) + double(p11));
        length = StickLength(x, y);
        float limit = 1.0f;
        if (!Fn_738500(GetActivePad())) {
            limit = stickLimitNoPad;
        }
        if (limit < length) {
            x = float(double(limit) * x / double(length));
            y = float(double(limit) * y / double(length));
            length = limit;
        }
    } else if (mode == 2) {
        x = p10;
        y = p11;
        length = StickLength(x, y);
    } else if (mode == 1) {
        x = p12;
        y = p13;
        length = StickLength(x, y);
    }
    if (!(double(length) > 9.999999747378752e-05)) {
        At<float>(self, 0x11C) = 0.0f;
        At<float>(self, 0x124) = 0.0f;
        At<float>(self, 0x128) = 0.0f;
        return;
    }
    float a = float(double(length) * x);
    float b = float(double(length) * y);
    float s = float(std::sin(double(At<float>(self, 0x120))));
    double sb = double(s) * double(b);
    float c = float(std::cos(double(At<float>(self, 0x120))));
    float along = float(double(c) * a + sb);
    if (along < 0.0f) {
        At<float>(self, 0x11C) = 0.0f;
    } else {
        // (0x85CB70: the CRT's x87 pow)
        float power = float(std::pow(2.0, double(-At<float>(self, 0x134))));
        float eased = float(std::pow(double(along), double(power)));
        At<float>(self, 0x11C) = float(double(eased) * At<float>(self, 0x11C));
    }
    float step = float(double(cameraStepScale) * double(timeStep));
    At<float>(self, 0x11C) =
        float(double(step) * At<float>(self, 0x130) * At<float>(self, 0x12C) + double(At<float>(self, 0x11C)));
    float power = float(std::pow(2.0, double(-At<float>(self, 0x138))));
    float cap = float(std::pow(double(length), double(power)));
    cap = float(double(cap) * At<float>(self, 0x12C));
    if (Fn_738500(GetActivePad())) {
        float speed = At<float>(self, 0x11C);
        if (cap > speed) {
            cap = speed;
        }
    }
    At<float>(self, 0x11C) = cap;
    At<float>(self, 0x120) = float(std::atan2(double(b), double(a)));
    float dt = timeStep;
    At<float>(self, 0x124) = float(double(x) * At<float>(self, 0x11C) * dt);
    float tilt = float(double(y) * At<float>(self, 0x11C) * At<float>(self, 0x13C) * dt);
    At<float>(self, 0x128) = tilt;
    if (invertLook != 0) {
        At<float>(self, 0x128) = float(double(tilt) * -1.0);
    }
}
C13F_REVERSED("CCamera::UpdateStickLook", 0x004F26E0, &Camera_UpdateStickLook);

} // namespace
