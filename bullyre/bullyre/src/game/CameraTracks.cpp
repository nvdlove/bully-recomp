#include "game/Camera.h"

#include <cmath>
#include <cstring>
#include "reversed.h"
#include "runflags.h"
#include <cstdio>

// CCamera's own methods from 0x4F2A90 to 0x4F41A0: construction and init, the reset,
// the per-frame process (the transition blend, the saved view, the view offset and
// the look limits), the controller requests, the fov transition and look-at helpers.
// (All thiscalls on TheCamera; written as __fastcall wrappers taking CCamera*.)
//
// Found here, not in Camera.h: CCamera has 15 controllers (+0x22C..+0x264), then
// +0x268 an action-tree object (0x38 bytes, 0x5F3E50) and +0x26C its 0x58-byte
// context; +0x1B8 a CameraViewOffset (Camera.cpp), +0x208 the look limits below
// (0x24 bytes, ending in the registered entity at +0x228); +0x2D0 a second transform
// (the live view while a saved controller runs), +0x304..+0x310 fov/+0xA0 swaps.

#define CAMTRACKS_REVERSED(...) REVERSED(__VA_ARGS__)

namespace {

using cam::CVector;
using cam::NiTransform;

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);           // CTimer's
GAME_VAR(float, timeStep, 0x00C1A9A4);              // CTimer::ms_fTimeStep
GAME_VAR(uint8_t, m_UserPause, 0x00C1A99A);
GAME_VAR(uint8_t, m_CodePause, 0x00C1A999);
GAME_VAR(uint8_t, m_OtherPause, 0x00C1A998);
GAME_VAR(uint8_t*, cameraSettings, 0x020C88B8);     // +0x58, +0x5C stick look; +0x6C the default fov (int)
GAME_VAR(void*, playerPed, 0x00C1AEA8);             // FindPlayerPed
GAME_VAR(uint8_t, invertLook, 0x00C18736);          // the frontend's invert camera option
GAME_VAR(uint8_t, lockButtonInputs, 0x00C1AF8C);
GAME_VAR(uint8_t, g_C1AFA0, 0x00C1AFA0);            // (lets input through the lock)
GAME_VAR(uint8_t, rollActive, 0x00A455A0);          // the saved view's roll (with 0xA4559C)
GAME_VAR(float, rollScale, 0x00A4559C);
GAME_VAR(uint32_t, viewTransformGuard, 0x00C3CFB8); // 0x4F2E30's static's init flag
inline NiTransform& viewTransform = *reinterpret_cast<NiTransform*>(0x00C3CF84);
inline void* const userInputManager = reinterpret_cast<void*>(0x00A45A30);
inline CameraTransition* const transition = reinterpret_cast<CameraTransition*>(0x00A45500);
inline void* const g_C3CFBC = reinterpret_cast<void*>(0x00C3CFBC);  // (the camera context's +0x38)
constexpr uintptr_t kCameraControllerVtable = 0x00914160;

// the camera's Gamebryo node and its local transform
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(NiTransform*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(void, __cdecl, Node_Update6C9AF0, 0x006C9AF0, void* node);
GAME_FN(void, __cdecl, Node_Update6C9E50, 0x006C9E50, void* node);
GAME_FN(NiTransform*, __thiscall, NiTransform_Multiply, 0x00406830, const NiTransform* self, NiTransform* out, const NiTransform* other);
GAME_FN(void, __thiscall, NiTransform_MakeIdentity, 0x00770920, NiTransform* self);
GAME_FN(void, __cdecl, CombineRotation, 0x00415EB0, NiTransform* t, const CVector* axis, float degrees, int mode);
// vectors
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, CrossProduct, 0x00414730, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(CVector*, __cdecl, Multiply3x3, 0x0041C490, CVector* out, const void* m, const CVector* v);
// (0x004146B0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* Multiply3x3b(CVector* out, const void* m, const CVector* v) { return re::CallVec16<0x004146B0, CVector*>(out, m, v); }
GAME_FN(void*, __thiscall, Placeable_GetMatrix, 0x0046DD20, void* placeable);
GAME_FN(float, __cdecl, AngleDiff, 0x00432980, float a, float b);
GAME_FN(float, __cdecl, VecHeading, 0x00433190, const CVector* v);
GAME_FN(float, __cdecl, VecPitch, 0x00433490, const CVector* v);
// references
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, re::Ptr32<void>* ref);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, void* entity, re::Ptr32<void>* ref);
// input
GAME_FN(uint8_t, __thiscall, UserInput_IsEnabled, 0x0050DF50, void* manager);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int key, int pad);
GAME_FN(float, __cdecl, ApplyDeadZone, 0x004F1520, float value, float deadZone);
// the action tree
GAME_FN(void*, __cdecl, ActionTree_Find, 0x005F5B10, const char* name, uintptr_t);
GAME_FN(void, __cdecl, ActFile_Load, 0x005F6DB0, const char* file, uintptr_t, uintptr_t);
GAME_FN(void, __thiscall, ActionContext_SetNode, 0x005F3830, void* context, void* node);
GAME_FN(void, __thiscall, ActionTree_Play, 0x005F45F0, void* tree, void* instance);
GAME_FN(void*, __thiscall, ActionTree_Construct, 0x005F3E50, void* self);
GAME_FN(uint8_t, __cdecl, Ped_Unknown472E60, 0x00472E60, void* ped);
// the controllers' constructors
GAME_FN(void*, __thiscall, FollowCameraController3_Construct, 0x004FD110, void* self);
GAME_FN(void*, __thiscall, FirstPersonCameraController_Construct, 0x004FA860, void* self);
GAME_FN(void*, __thiscall, CutsceneCameraController_Construct, 0x004F87B0, void* self);
GAME_FN(void*, __thiscall, ScriptCameraController_Construct, 0x005078C0, void* self);
GAME_FN(void*, __thiscall, StoreCameraController_Construct, 0x00509570, void* self);
GAME_FN(void*, __thiscall, VehicleCameraController_Construct, 0x0050AF60, void* self);
GAME_FN(void*, __thiscall, FightCamCtrl_Construct, 0x004F8BF0, void* self);
GAME_FN(void*, __thiscall, ConvCamCtrl_Construct, 0x004F6730, void* self);
GAME_FN(void*, __thiscall, CoasterCameraController_Construct, 0x004F4290, void* self);
GAME_FN(void*, __thiscall, OffsetCameraController_Construct, 0x00504ED0, void* self);
GAME_FN(void*, __thiscall, ArcadeCameraController_Construct, 0x004F0870, void* self);
// the transition (the global one at 0xA45500)
GAME_FN(int32_t, __thiscall, Transition_IsActive, 0x00856850, CameraTransition* self);  // (a shared getter of +8)
GAME_FN(int32_t*, __thiscall, Transition_GetController, 0x004F0B10, CameraTransition* self, int32_t* out, int end);
GAME_FN(void, __thiscall, Transition_SetEnd, 0x004F0C00, CameraTransition* self, int end, void* camera, float a, float b);
GAME_FN(void, __thiscall, Transition_GetEnd, 0x004F0C40, CameraTransition* self, int end, void* camera, float* a, float* b);
GAME_FN(void, __thiscall, Transition_Blend, 0x004F1130, CameraTransition* self, void* camera, float* a, float* b);
// CCamera's own
GAME_FN(void, __thiscall, Camera_Unknown4F2390, 0x004F2390, CCamera* self, int value);
GAME_FN(void, __thiscall, Camera_Unknown4F1670, 0x004F1670, CCamera* self);
GAME_FN(void, __thiscall, Camera_Unknown4F26E0, 0x004F26E0, CCamera* self);
GAME_FN(void, __thiscall, Camera_Unknown4F1EC0, 0x004F1EC0, CCamera* self);
GAME_FN(float, __thiscall, Camera_Unknown43A570, 0x0043A570, CCamera* self);
GAME_FN(void, __thiscall, Camera_ProcessController, 0x004F2200, CCamera* self);
GAME_FN(void, __thiscall, Camera_Unknown4F1F10, 0x004F1F10, CCamera* self);
GAME_FN(void, __thiscall, Camera_Unknown4F1D10, 0x004F1D10, CCamera* self);
GAME_FN(void, __thiscall, Camera_Unknown4F2090, 0x004F2090, CCamera* self);
GAME_FN(void, __thiscall, Camera_SetControllerRequest, 0x004F2C20, CCamera* self, int a, int b, int c, int d, int e);
GAME_FN(void, __thiscall, Camera_LookAtPoint, 0x004F2C60, CCamera* self, const CVector* position, const CVector* target);
GAME_FN(void, __thiscall, Camera_LookFromAngles, 0x004F35E0, CCamera* self, const CVector* position, float heading, float pitch);
GAME_FN(void, __thiscall, Camera_SetTargetPed, 0x004F3680, CCamera* self, void* ped);
GAME_FN(void, __thiscall, ViewOffset_Restore, 0x004F3D50, void* self);
GAME_FN(void, __thiscall, ViewOffset_Clear, 0x004F3D80, void* self);
GAME_FN(void, __thiscall, ViewOffset_Apply, 0x004F3E80, void* self);

using re::At;

template <typename R, typename... A>
R VCall(void* object, int index, A... args) {
    auto fn = reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(object))[index]));
    return fn(object, args...);
}

NiTransform* CameraTransform(void* camera) {
    return GetLocalTransform(GetCameraNode(camera));
}

// (inlined everywhere: a registered entity reference changed)
void SetReference(re::Ptr32<void>* slot, void* entity) {
    void* old = *slot;
    if (entity != old) {
        if (old) {
            CleanUpOldReference(old, slot);
        }
        *slot = entity;
        if (entity) {
            RegisterReference(entity, slot);
        }
    }
}

// a bare CameraController (4 bytes: the base vtable)
void* NewBaseController() {
    void* p = Malloc(4);
    if (p) {
        At<uint32_t>(p, 0) = kCameraControllerVtable;
        return p;
    }
    return nullptr;
}

void* NewController(size_t size, void* (__thiscall* construct)(void*)) {
    void* p = Malloc(size);
    return p ? construct(p) : nullptr;
}

// The look limits (CCamera +0x208): the view kept within pitch and heading ranges
// around a direction (in an entity's space when there is one)
struct CameraLookLimits {
    CVector direction;     // +0
    float pitchMax;        // +0xC
    float headingMax;      // +0x10
    float pitchMin;        // +0x14
    float headingMin;      // +0x18
    uint8_t enabled;       // +0x1C
    uint8_t pitchClamped;  // +0x1D
    uint8_t headingClamped; // +0x1E
    uint8_t unknown1F;     // +0x1F
    re::Ptr32<void> entity;          // +0x20 (a registered reference)
};
static_assert(sizeof(CameraLookLimits) == 0x24);

// ---- CCamera

// 0x4F2A90: the stick's look speed this frame (0 while input's locked; the axis by the
// look mode at +0x108, through the dead zone at +0x114, scaled by +0x110)
float __fastcall Camera_GetStickLookSpeed(CCamera* self, re::Edx) {
    if (!UserInput_IsEnabled(userInputManager)) {
        return 0.0f;
    }
    if (lockButtonInputs && !g_C1AFA0) {
        return 0.0f;
    }
    float value = 0.0f;
    int mode = At<int32_t>(self, 0x108);
    if (mode == 3) {
        value = Pad_GetValue(0x11, 0);
        if (value == 0.0f) {
            value = Pad_GetValue(0x13, 0);
        }
    } else if (mode == 2) {
        value = Pad_GetValue(0x11, 0);
    } else if (mode == 1) {
        value = Pad_GetValue(0x13, 0);
    }
    value = ApplyDeadZone(value, At<float>(self, 0x114));
    if (!invertLook) {
        value = float(double(value) * -1.0);
    }
    return float(double(At<float>(self, 0x110)) * value * timeStep);
}
CAMTRACKS_REVERSED("CCamera::GetStickLookSpeed", 0x004F2A90, RE_BODY(&Camera_GetStickLookSpeed), nullptr, nullptr, re::Mode::Original);

// 0x4F2C20: a controller change for 0x4F2390 to carry out
void __fastcall Camera_SetControllerRequestOurs(CCamera* self, re::Edx, int controller, int b, int c, int d, int e) {
    if (re::DoorReport()) {
        std::fprintf(stderr, "doors: camera controller request %d (%d %d %d %d) at %u ms\n", controller, b, c, d, e,
                     *reinterpret_cast<const uint32_t*>(0x00C1A9B4));
    }
    self->unknown270 = controller;
    self->unknown274 = b;
    self->unknown278 = uint8_t(c);
    self->unknown27C = d;
    self->unknown280 = e;
    Camera_Unknown4F2390(self, 0);
}
CAMTRACKS_REVERSED("CCamera::SetControllerRequest", 0x004F2C20, RE_BODY(&Camera_SetControllerRequestOurs), nullptr, nullptr, re::Mode::Original);

// 0x4F2C60: the camera (TheCamera's, whichever this is) at `position` looking at
// `target` (up is z, or y when looking nearly straight up or down)
void __fastcall Camera_LookAtPointOurs(CCamera*, re::Edx, const CVector* position, const CVector* target) {
    NiTransform* t = CameraTransform(TheCamera.camera);
    CVector difference;
    CVector dir = *VectorSub(&difference, target, position);
    Vector_Normalise(&dir);
    CVector right = {0.0f, 0.0f, 0.0f};
    CVector up;
    up.x = 0.0f;
    float dot = float((double(dir.y) * 0.0 + 0.0 * double(dir.x)) + dir.z);
    dot = std::fabs(dot);
    if (!(dot < 0.999)) {
        up.y = 1.0f;
        up.z = 0.0f;
    } else {
        up.y = 0.0f;
        up.z = 1.0f;
    }
    CVector temp;
    right = *CrossProduct(&temp, &dir, &up);
    CVector realUp = *CrossProduct(&temp, &right, &dir);
    t->rotate[1] = realUp.x;
    t->rotate[4] = realUp.y;
    t->rotate[7] = realUp.z;
    t->rotate[2] = dir.x;
    t->rotate[5] = dir.y;
    t->rotate[8] = dir.z;
    t->rotate[0] = -right.x;
    t->rotate[3] = -right.y;
    t->rotate[6] = -right.z;
    t->translate = *position;
}
CAMTRACKS_REVERSED("CCamera::LookAtPoint", 0x004F2C60, RE_BODY(&Camera_LookAtPointOurs), nullptr, nullptr, re::Mode::Original);

// 0x4F2E30: the view (the saved-controller's live one at +0x2D0 while there is one), in
// a static copy
NiTransform* __fastcall Camera_GetViewTransform(CCamera* self, re::Edx) {
    if ((viewTransformGuard & 1) == 0) {
        viewTransformGuard |= 1;
        viewTransform.scale = 1.0f;
    }
    if (self->savedController == 0) {
        viewTransform = *CameraTransform(TheCamera.camera);
        return &viewTransform;
    }
    viewTransform = At<NiTransform>(self, 0x2D0);
    return &viewTransform;
}
CAMTRACKS_REVERSED("CCamera::GetViewTransform", 0x004F2E30, RE_BODY(&Camera_GetViewTransform), nullptr, nullptr, re::Mode::Original);

// 0x4F2EA0 (TheCamera's atexit destructor): its entity references dropped
void __fastcall Camera_Destruct(CCamera* self, re::Edx) {
    static const size_t refs[] = {0x228, 0x1B4, 0x1B0, 0x1AC};
    for (size_t offset : refs) {
        re::Ptr32<void>& ref = At<void*>(self, offset);
        if (ref) {
            CleanUpOldReference(ref, &ref);
        }
    }
}
CAMTRACKS_REVERSED("CCamera::~CCamera", 0x004F2EA0, RE_BODY(&Camera_Destruct), nullptr, nullptr, re::Mode::Original);

// 0x4F2F10 (TheCamera's static constructor)
CCamera* __fastcall Camera_Construct(CCamera* self, re::Edx) {
    At<float>(self, 0x180) = 0.0f;
    At<float>(self, 0x184) = 0.0f;
    At<int32_t>(self, 0x44) = 0;
    At<float>(self, 0x188) = 0.0f;
    self->frame = nullptr;
    At<float>(self, 0x18C) = 0.0f;
    self->targetPed = nullptr;
    At<float>(self, 0x190) = 0.0f;
    At<void*>(self, 0x1B0) = nullptr;
    At<float>(self, 0x194) = 0.0f;
    self->unknown198 = {0.0f, 0.0f, 0.0f};
    At<void*>(self, 0x1B4) = nullptr;
    At<float>(self, 0x1A0) = 0.0f;
    ViewOffset_Clear(reinterpret_cast<uint8_t*>(self) + 0x1B8);
    CameraLookLimits& limits = At<CameraLookLimits>(self, 0x208);
    limits.direction = {0.0f, 0.0f, 0.0f};
    limits.entity = nullptr;
    limits.enabled = 0;
    // (just cleared: never taken)
    if (limits.entity) {
        CleanUpOldReference(limits.entity, &limits.entity);
        limits.entity = nullptr;
    }
    self->unknown274 = 0;
    self->unknown278 = 0;
    self->unknown27C = 0;
    self->unknown280 = 0;
    self->unknown270 = 1;
    self->savedTransform.scale = 1.0f;
    At<NiTransform>(self, 0x2D0).scale = 1.0f;
    self->savedController = 0;
    self->unknown0[0] = 0;
    self->camera = nullptr;
    self->unknown1A9[0] = 0;
    return self;
}
CAMTRACKS_REVERSED("CCamera::CCamera", 0x004F2F10, RE_BODY(&Camera_Construct), nullptr, nullptr, re::Mode::Original);

// 0x4F3000: makes the controllers (each then gets its vtable[1]) and the camera's action
// tree context
void __fastcall Camera_Init(CCamera* self, re::Edx) {
    At<uint8_t>(self, 0x319) = 0;
    *reinterpret_cast<int32_t*>(self->unknown1A4) = 1;
    self->unknown1A8 = 0;
    re::Ptr32<CameraController>* controllers = self->controllers;
    // (in the game's allocation order)
    controllers[0] = static_cast<CameraController*>(NewBaseController());
    controllers[1] = static_cast<CameraController*>(NewController(0x53C, FollowCameraController3_Construct));
    controllers[2] = static_cast<CameraController*>(NewController(0x2C, FirstPersonCameraController_Construct));
    controllers[3] = static_cast<CameraController*>(NewController(0xBC, CutsceneCameraController_Construct));
    controllers[4] = static_cast<CameraController*>(NewController(0x108, ScriptCameraController_Construct));
    controllers[7] = static_cast<CameraController*>(NewController(0x60, StoreCameraController_Construct));
    controllers[8] = static_cast<CameraController*>(NewController(0x120, VehicleCameraController_Construct));
    controllers[9] = static_cast<CameraController*>(NewController(0x5C, FightCamCtrl_Construct));
    controllers[10] = static_cast<CameraController*>(NewController(0x180, ConvCamCtrl_Construct));
    controllers[11] = static_cast<CameraController*>(NewBaseController());
    controllers[12] = static_cast<CameraController*>(NewController(0x70, CoasterCameraController_Construct));
    controllers[13] = static_cast<CameraController*>(NewController(0x174, OffsetCameraController_Construct));
    controllers[14] = static_cast<CameraController*>(NewController(8, ArcadeCameraController_Construct));
    controllers[5] = static_cast<CameraController*>(NewBaseController());
    controllers[6] = static_cast<CameraController*>(NewBaseController());
    for (int i = 0; i < 15; i++) {
        VCall<void>(controllers[i], 1);
    }
    self->fadeDuration = 0.0f;
    self->fadeState = 0;
    self->fadeStart = 0.0f;
    self->fadeTarget = 0;
    self->fadeAlpha = 0.0f;
    for (size_t offset = 0x180; offset <= 0x194; offset += 4) {
        At<float>(self, offset) = 0.0f;
    }
    PushMemoryTag(0x26);
    At<void*>(self, 0x268) = NewController(0x38, ActionTree_Construct);
    uint8_t* context = static_cast<uint8_t*>(Malloc(0x58));
    if (context) {
        At<int32_t>(context, 0) = 0;
        At<int32_t>(context, 4) = 0;
        At<int32_t>(context, 8) = 0;
        At<int32_t>(context, 0xC) = 0;
        At<float>(context, 0x24) = 0.0f;
        At<int32_t>(context, 0x10) = 0;
        At<int32_t>(context, 0x14) = 0;
        At<int32_t>(context, 0x18) = 0;
        At<uint8_t>(context, 0x1C) = 0;
        At<uint8_t>(context, 0x1D) = 0;
        At<int32_t>(context, 0x20) = 0;
        At<int32_t>(context, 0x34) = 0;
        At<int32_t>(context, 0x38) = 0;
        At<uint8_t>(context, 0x3C) = 0;
        At<int32_t>(context, 0x40) = 0;
        At<int32_t>(context, 0x44) = 0;
        At<int32_t>(context, 0x50) = 0;
        At<int32_t>(context, 0x54) = 0;
        At<int32_t>(context, 0x48) = -1;
        At<float>(context, 0x28) = 0.0f;
        At<float>(context, 0x2C) = 0.0f;
        At<float>(context, 0x30) = 0.0f;
    } else {
        context = nullptr;
    }
    At<uint8_t*>(self, 0x26C) = context;
    PopMemoryTag();
    // (the context's +8 is the ped it acts for; no null check)
    SetReference(&At<void*>(At<uint8_t*>(self, 0x26C), 8), playerPed);
    At<void*>(At<uint8_t*>(self, 0x26C), 0x38) = g_C3CFBC;
    std::memset(self->unknown284, 0, 0x12);
    At<float>(self, 0x314) = 0.0f;
    At<uint8_t>(self, 0x31A) = 0;
    self->unknown0[0] = 1;
}
CAMTRACKS_REVERSED("CCamera::Init", 0x004F3000, RE_BODY(&Camera_Init), nullptr, nullptr, re::Mode::Original);

// 0x4F33A0: a fov change: mode (+0x16C) 0 back to normal, 1 to `value` as the override,
// 2 towards the default, 3 towards `value`, over `duration` from now (0x4F1670 runs it)
void __fastcall Camera_SetFovTransition(CCamera* self, re::Edx, float value, int duration, int mode, float speed, int flag) {
    float from = self->fovOverridden == 0 ? self->fov : self->fovOverride;
    self->fovOverridden = mode;
    At<float>(self, 0x160) = from;
    At<uint32_t>(self, 0x158) = timeInMs;
    At<float>(self, 0x178) = speed;
    At<int32_t>(self, 0x15C) = duration;
    At<int32_t>(self, 0x17C) = flag;
    if (flag == 0) {
        At<float>(self, 0x170) = 0.0f;
    }
    switch (mode) {
    case 0:
        At<float>(self, 0x170) = 0.0f;
        break;
    case 1:
        self->fovOverride = value;
        At<float>(self, 0x170) = 0.0f;
        break;
    case 2:
        At<float>(self, 0x164) = float(At<int32_t>(cameraSettings, 0x6C));
        break;
    case 3:
        At<float>(self, 0x164) = value;
        break;
    default:
        break;
    }
    Camera_Unknown4F1670(self);
}
CAMTRACKS_REVERSED("CCamera::SetFovTransition", 0x004F33A0, RE_BODY(&Camera_SetFovTransition), nullptr, nullptr, re::Mode::Original);

// 0x4F3490: the active controller's vtable[8]; while blending, run on each end whose
// controller is the active one (the camera set to that end's view first, and the
// end updated after)
void __fastcall Camera_CallActiveController20(CCamera* self, re::Edx, int value) {
    if (*reinterpret_cast<int32_t*>(self->unknown1A4) == 0) {
        return;
    }
    if (Transition_IsActive(transition) == 0) {
        VCall<void>(self->controllers[self->activeController], 8, value);
        return;
    }
    for (int end = 0; end < 2; end++) {
        int32_t controller;
        if (*Transition_GetController(transition, &controller, end) != self->activeController) {
            continue;
        }
        // (the fov comes back in the argument's own stack slot)
        float fov;
        Transition_GetEnd(transition, end, self->camera, &self->unknownA0, &fov);
        if (At<uint8_t>(self, 0x31A) == 0) {
            self->fovOverridden = 0;
            self->fov = fov;
        }
        VCall<void>(self->controllers[self->activeController], 8, value);
        Camera_Unknown4F1EC0(self);
        float b = Camera_Unknown43A570(self);
        Transition_SetEnd(transition, end, self->camera, self->unknownA0, b);
    }
}
CAMTRACKS_REVERSED("CCamera::CallActiveController20", 0x004F3490, RE_BODY(&Camera_CallActiveController20), nullptr, nullptr, re::Mode::Original);

// 0x4F3590: a controller, requesting a switch to it when it isn't the active one
CameraController* __fastcall Camera_SwitchToController(CCamera* self, re::Edx, int index) {
    if (self->activeController != index) {
        self->unknown270 = index;
        self->unknown274 = 0;
        self->unknown278 = 1;
        self->unknown27C = 0;
        self->unknown280 = 0;
        Camera_Unknown4F2390(self, 0);
    }
    return self->controllers[index];
}
CAMTRACKS_REVERSED("CCamera::SwitchToController", 0x004F3590, RE_BODY(&Camera_SwitchToController), nullptr, nullptr, re::Mode::Original);

// 0x4F35E0: looks from `position` along a heading and pitch (radians)
void __fastcall Camera_LookFromAnglesOurs(CCamera* self, re::Edx, const CVector* position, float heading, float pitch) {
    float cosPitch = float(std::cos(double(pitch)));
    float sinPitch = float(std::sin(double(pitch)));
    float sinHeading = float(std::sin(double(heading)));
    CVector target;
    target.x = float(double(sinHeading) * cosPitch);
    float cosHeading = float(std::cos(double(heading)));
    target.y = float(-double(cosHeading) * cosPitch);
    target.z = sinPitch;
    target.x = float(double(position->x) + target.x);
    target.y = float(double(position->y) + target.y);
    target.z = float(double(position->z) + target.z);
    Camera_LookAtPoint(self, position, &target);
}
CAMTRACKS_REVERSED("CCamera::LookFromAngles", 0x004F35E0, RE_BODY(&Camera_LookFromAnglesOurs), nullptr, nullptr, re::Mode::Original);

// 0x4F3680: the ped the camera follows (the player when none), also the action
// context's
void __fastcall Camera_SetTargetPedOurs(CCamera* self, re::Edx, void* ped) {
    re::Ptr32<void>* slot = &self->targetPed;
    SetReference(slot, ped);
    if (*slot == nullptr) {
        SetReference(slot, playerPed);
    }
    if (uint8_t* context = At<uint8_t*>(self, 0x26C)) {
        SetReference(&At<void*>(context, 8), ped);
    }
}
CAMTRACKS_REVERSED("CCamera::SetTargetPed", 0x004F3680, RE_BODY(&Camera_SetTargetPedOurs), nullptr, nullptr, re::Mode::Original);

// 0x4F3720: back to the defaults (the follow camera, the player, every controller
// reset) and the camera action tree started
void __fastcall Camera_Restart(CCamera* self, re::Edx) {
    At<float>(self, 0x98) = 1.0f;
    At<int32_t>(self, 0x94) = 0;
    At<int32_t>(self, 0xA4) = 0;
    self->unknownA0 = 0.0f;
    self->fadeColour[0] = 0;
    self->fadeColour[1] = 0;
    self->fadeColour[2] = 0;
    self->fadeColour[3] = 0xFF;
    self->activeController = 1;
    self->unknown270 = 1;
    At<int32_t>(self, 0x108) = 1;
    At<float>(self, 0x10C) = At<float>(cameraSettings, 0x58);
    At<float>(self, 0x110) = At<float>(cameraSettings, 0x5C);
    At<float>(self, 0x114) = 0.0f;
    At<float>(self, 0x118) = 0.0f;
    self->fovOverride = float(At<int32_t>(cameraSettings, 0x6C));
    self->fovOverridden = 0;
    At<uint32_t>(self, 0x158) = 0;
    At<int32_t>(self, 0x15C) = 0;
    At<float>(self, 0x164) = float(At<int32_t>(cameraSettings, 0x6C));
    At<int32_t>(self, 0x17C) = 0x80;
    At<uint8_t>(self, 0x296) = 1;
    At<float>(self, 0x170) = 0.0f;
    At<float>(self, 0x178) = *reinterpret_cast<const float*>(0x00900170);  // 0.1
    At<float>(self, 0x314) = 0.0f;
    CameraLookLimits& limits = At<CameraLookLimits>(self, 0x208);
    limits.enabled = 0;
    if (limits.entity) {
        CleanUpOldReference(limits.entity, &limits.entity);
        limits.entity = nullptr;
    }
    Camera_SetTargetPed(self, playerPed);
    re::Ptr32<void>& ref1B0 = At<void*>(self, 0x1B0);
    if (ref1B0) {
        CleanUpOldReference(ref1B0, &ref1B0);
        ref1B0 = nullptr;
    }
    for (int i = 0; i < 15; i++) {
        VCall<void>(self->controllers[i], 3);
    }
    *reinterpret_cast<int32_t*>(self->unknown1A4) = 1;
    self->fov = float(At<int32_t>(cameraSettings, 0x6C));
    self->unknown1A8 = 0;
    void* node = ActionTree_Find("/Global/Camera/CameraActions", 0);
    if (!node) {
        ActFile_Load("/Act/CameraActions.act", 0, 0);
        node = ActionTree_Find("/Global/Camera/CameraActions", 0);
    }
    uint8_t* tree = At<uint8_t*>(self, 0x268);
    uint8_t* context = At<uint8_t*>(self, 0x26C);
    At<void*>(tree, 0x18) = context;
    At<void*>(context, 0) = tree;
    ActionContext_SetNode(At<void*>(self, 0x26C), node);
    if (playerPed) {
        context = At<uint8_t*>(self, 0x26C);
        void* owner = At<void*>(context, 0x40);
        if (owner) {
            At<int32_t>(context, 0x44) = 0;
            void* instance = VCall<void*>(owner, 0x4C / 4, context);
            ActionTree_Play(At<void*>(context, 0), instance);
        }
    }
}
CAMTRACKS_REVERSED("CCamera::Restart", 0x004F3720, RE_BODY(&Camera_Restart), nullptr, nullptr, re::Mode::Original);

// 0x4F3900: the camera's process: the active controller (both ends of a blend), the
// view offset and look limits, then the saved controller's own view (+0x298) kept
// apart at +0x2D0 while the camera shows the saved one; the position kept within
// +-900 in x and y
void __fastcall Camera_Process(CCamera* self, re::Edx) {
    if (self->savedController != 0) {
        *CameraTransform(self->camera) = At<NiTransform>(self, 0x2D0);
        At<float>(self, 0x304) = self->unknownA0;
        self->unknownA0 = At<float>(self, 0x308);
        At<float>(self, 0x30C) = self->fov;
        self->fov = self->savedFov;
    }
    if (!m_UserPause && !m_CodePause && !m_OtherPause) {
        if (self->targetPed == nullptr) {
            Camera_SetTargetPed(self, playerPed);
        }
        void* ped = self->targetPed;
        if (ped && (At<uint8_t>(ped, 0x108) & 7) == 3 && Ped_Unknown472E60(ped)) {
            int active = self->activeController;
            if (active == 2 || active == 13 || active == 10 || active == 4) {
                Camera_SetControllerRequest(self, 1, 0, 1, 0, 0);
            }
        }
        Camera_Unknown4F2390(self, 1);
        Camera_Unknown4F1670(self);
        Camera_Unknown4F26E0(self);
        ViewOffset_Restore(reinterpret_cast<uint8_t*>(self) + 0x1B8);
        if (Transition_IsActive(transition) != 0) {
            int32_t active = self->activeController;
            for (int end = 0; end < 2; end++) {
                int32_t controller;
                self->activeController = *Transition_GetController(transition, &controller, end);
                float fov;
                Transition_GetEnd(transition, end, self->camera, &self->unknownA0, &fov);
                if (At<uint8_t>(self, 0x31A) == 0) {
                    self->fovOverridden = 0;
                    self->fov = fov;
                }
                Camera_ProcessController(self);
                Camera_Unknown4F1EC0(self);
                float b = Camera_Unknown43A570(self);
                Transition_SetEnd(transition, end, self->camera, self->unknownA0, b);
            }
            float fov;
            Transition_Blend(transition, self->camera, &self->unknownA0, &fov);
            if (At<uint8_t>(self, 0x31A) == 0) {
                self->fovOverridden = 0;
                self->fov = fov;
            }
            self->activeController = active;
        } else {
            Camera_ProcessController(self);
            Camera_Unknown4F1EC0(self);
        }
        ViewOffset_Apply(reinterpret_cast<uint8_t*>(self) + 0x1B8);
    }
    if (self->savedController != 0) {
        NiTransform* t = CameraTransform(self->camera);
        At<NiTransform>(self, 0x2D0) = *t;
        *t = self->savedTransform;
        At<float>(self, 0x308) = self->unknownA0;
        self->unknownA0 = At<float>(self, 0x304);
        self->savedFov = self->fov;
        self->fov = At<float>(self, 0x30C);
        VCall<void>(self->controllers[self->savedController], 7);
        self->savedTransform = *t;
        float forwardX = t->rotate[2];
        float forwardY = t->rotate[5];
        self->unknownA0 = float(std::atan2(double(forwardX), double(forwardY)));
        if (rollActive) {
            CVector axis = {t->rotate[2], t->rotate[5], t->rotate[8]};
            CVector translate = t->translate;
            float degrees = float(double(At<float>(self, 0x314)) * rollScale);
            CombineRotation(t, &axis, degrees, 2);
            t->translate = translate;
        }
    }
    NiTransform* t = CameraTransform(self->camera);
    const float kMin = *reinterpret_cast<const float*>(0x0091432C);  // -899
    const float kMax = *reinterpret_cast<const float*>(0x00914328);  // 899
    if (t->translate.x <= -900.0) {
        t->translate.x = kMin;
    } else if (t->translate.x >= 900.0) {
        t->translate.x = kMax;
    }
    if (t->translate.y <= -900.0) {
        t->translate.y = kMin;
    } else if (t->translate.y >= 900.0) {
        t->translate.y = kMax;
    }
    Camera_Unknown4F1F10(self);
    // (TheCamera's node, whichever camera this is)
    void* node = GetCameraNode(TheCamera.camera);
    GetLocalTransform(node);
    Node_Update6C9AF0(node);
    Node_Update6C9E50(node);
    Camera_Unknown4F1D10(self);
    Camera_Unknown4F2090(self);
}
CAMTRACKS_REVERSED("CCamera::Process", 0x004F3900, RE_BODY(&Camera_Process), nullptr, nullptr, re::Mode::Original);

// ---- CameraViewOffset (CCamera +0x1B8; its struct is Camera.cpp's)

// 0x4F3E80: the camera's transform saved, then rotated about x, y and z (degrees) and
// moved by the offset in its own space
void __fastcall ViewOffset_ApplyOurs(void* self, re::Edx) {
    if (At<uint8_t>(self, 0) == 0) {
        return;
    }
    CVector offset = At<CVector>(self, 4);
    CVector rotation = At<CVector>(self, 0x10);
    NiTransform* t = CameraTransform(TheCamera.camera);
    CVector xAxis = {1.0f, 0.0f, 0.0f};
    At<NiTransform>(self, 0x1C) = *t;
    CVector yAxis = {0.0f, 1.0f, 0.0f};
    CVector zAxis = {0.0f, 0.0f, 1.0f};
    CombineRotation(t, &xAxis, rotation.x, 1);
    CombineRotation(t, &yAxis, rotation.y, 1);
    CombineRotation(t, &zAxis, rotation.z, 1);
    NiTransform move;
    move.scale = 1.0f;
    NiTransform_MakeIdentity(&move);
    move.translate = offset;
    NiTransform result;
    *t = *NiTransform_Multiply(t, &result, &move);
}
CAMTRACKS_REVERSED("CameraViewOffset::Apply", 0x004F3E80, RE_BODY(&ViewOffset_ApplyOurs), nullptr, nullptr, re::Mode::Original);

// ---- the look limits (CCamera +0x208)

// 0x4F3FA0: turns the camera back within the limits (not while the target ped is the
// player with +0x148C and +0x16C8 set)
void __fastcall LookLimits_Apply(CameraLookLimits* self, re::Edx) {
    if (!self->enabled) {
        return;
    }
    void* ped = TheCamera.targetPed;
    if (ped && ped == playerPed && At<int32_t>(playerPed, 0x148C) != 0 && At<int32_t>(playerPed, 0x16C8) != 0) {
        return;
    }
    NiTransform* t = CameraTransform(TheCamera.camera);
    CVector position = t->translate;
    CVector forward = {t->rotate[2], t->rotate[5], t->rotate[8]};
    CVector direction = self->direction;
    if (self->entity) {
        CVector temp;
        direction = *Multiply3x3(&temp, Placeable_GetMatrix(self->entity), &self->direction);
    }
    float pitch = VecPitch(&direction);
    float heading = VecHeading(&direction);
    float viewPitch = VecPitch(&forward);
    float viewHeading = VecHeading(&forward);
    self->pitchClamped = 0;
    self->headingClamped = 0;
    float pitchOffset = AngleDiff(pitch, viewPitch);
    if (pitchOffset < self->pitchMin) {
        pitchOffset = self->pitchMin;
        self->pitchClamped = 1;
    }
    if (self->pitchMax < pitchOffset) {
        pitchOffset = self->pitchMax;
        self->pitchClamped = 1;
    }
    float headingOffset = AngleDiff(heading, viewHeading);
    if (headingOffset < self->headingMin) {
        headingOffset = self->headingMin;
        self->headingClamped = 1;
    }
    if (self->headingMax < headingOffset) {
        headingOffset = self->headingMax;
        self->headingClamped = 1;
    }
    float newPitch = float(double(pitchOffset) + pitch);
    float newHeading = float(double(headingOffset) + heading);
    Camera_LookFromAngles(&TheCamera, &position, newHeading, newPitch);
}
CAMTRACKS_REVERSED("CameraLookLimits::Apply", 0x004F3FA0, RE_BODY(&LookLimits_Apply), nullptr, nullptr, re::Mode::Original);

// 0x4F41A0: the limits and the direction (in world space)
void __fastcall LookLimits_Get(CameraLookLimits* self, re::Edx, float* pitchMin, float* pitchMax, float* headingMin,
                               float* headingMax, CVector* direction) {
    *pitchMin = self->pitchMin;
    *headingMin = self->headingMin;
    *pitchMax = self->pitchMax;
    *headingMax = self->headingMax;
    *direction = self->direction;
    if (self->entity) {
        void* matrix = Placeable_GetMatrix(self->entity);
        CVector local = self->direction;
        struct { CVector v; float pad; } worldPad16;   // (the game's transform writes a 4th float)
        CVector& world = worldPad16.v;
        Multiply3x3b(&world, matrix, &local);
        *direction = world;
    }
}
CAMTRACKS_REVERSED("CameraLookLimits::Get", 0x004F41A0, RE_BODY(&LookLimits_Get), nullptr, nullptr, re::Mode::Original);

} // namespace
