#include "game/Camera.h"

#include <cmath>
#include "reversed.h"

// FirstPersonCameraController: the look-around view from the target ped's eyes. Yaw
// and pitch are kept as the rotations still to apply to the camera's node (the node
// is turned about its own position each frame), pitch as 0..1 between the attribute
// set's limits (degrees). The attribute set ("butes") is looked up by name.

#define FPCAMERA_REVERSED(...) REVERSED(__VA_ARGS__)

namespace {

using cam::CVector;

GAME_VAR(void*, playerPed, 0x00C1AEA8);             // FindPlayerPed
GAME_VAR(void*, ignoreEntity, 0x00C1AE70);          // CWorld::pIgnoreEntity (line tests skip it)
GAME_VAR(int32_t, cameraMode94, 0x00C3CCFC);        // TheCamera+0x94 (13: the offset camera)
GAME_VAR(void*, offsetCamera, 0x00C3CEC8);          // TheCamera.controllers[13]
GAME_VAR(uint8_t, cameraFlag284, 0x00C3CEEC);       // TheCamera+0x284
GAME_VAR(uint8_t, cameraFlag224, 0x00C3CE8C);       // TheCamera+0x224
GAME_VAR(uint8_t, cameraFlag225, 0x00C3CE8D);       // TheCamera+0x225
GAME_VAR(float, lookX, 0x00C3CD8C);                 // TheCamera+0x124 the look input's x
GAME_VAR(float, lookY, 0x00C3CD90);                 // TheCamera+0x128 and y
// the attribute set names ("FirstPerson" root, the default, the table type)
inline void* const kButesRoot = reinterpret_cast<void*>(0x01B65528);
inline void* const kButesDefault = reinterpret_cast<void*>(0x01B65400);
inline void* const kButesType = reinterpret_cast<void*>(0x01B6552C);

// names and paths into the attribute tables
GAME_FN(void, __thiscall, Name_Copy, 0x005FA880, void* self, const void* from);
GAME_FN(void, __thiscall, Name_Construct, 0x005FA910, void* self, uintptr_t value);
GAME_FN(void, __thiscall, Path_Build, 0x005FA8C0, void* self, int count, const void* names);
GAME_FN(void*, __cdecl, Config_Find, 0x0072AEA0, const void* path, const void* type);
GAME_FN(void, __thiscall, EmptyThiscall, 0x0049A650, void* self);   // (the names' destructor)
GAME_FN(void, __cdecl, AddLoadCallback, 0x0072B870, void* fn, void* object);
// math
GAME_FN(CVector*, __cdecl, Vector_FromAngle, 0x00432DF0, CVector* out, float angle);   // (-sin, cos, 0)
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __cdecl, GetHeading, 0x00433310, const CVector* v);
GAME_FN(float, __cdecl, GetPitch, 0x00433490, const CVector* v);
// entities
GAME_FN(CVector*, __thiscall, Entity_GetForward, 0x004154A0, void* entity, CVector* out);
GAME_FN(CVector*, __thiscall, Placement_GetRight, 0x00473600, void* placement, CVector* out);
GAME_FN(void*, __thiscall, Ped_GetTarget, 0x00497100, void* ped);
GAME_FN(void*, __cdecl, Fn_5EE1A0, 0x005EE1A0, void* object);
GAME_FN(uint8_t*, __cdecl, Fn_6C7710, 0x006C7710, void* object);   // (+0x2FC: a position)
GAME_FN(uint8_t, __cdecl, ProcessLineOfSight, 0x00461EA0, const CVector* from, const CVector* to, void* point, re::Ptr32<void>* entity,
        uint32_t flags, int a, int b);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, void* ref);
// the camera's node
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(cam::NiTransform*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(void, __cdecl, Node_Translate, 0x006CA180, void* node, const CVector* by, int mode);
GAME_FN(void, __cdecl, Node_Rotate, 0x006CA1E0, void* node, const CVector* axis, float angle, int mode);
GAME_FN(void, __cdecl, Camera_SetNearClip, 0x006C8820, void* camera, float plane);
GAME_FN(void, __thiscall, CameraController_MoveTo, 0x00505550, void* self, const CVector* position);   // (self unused)
GAME_FN(CVector*, __thiscall, OffsetCamera_GetDirection, 0x00505270, void* self, CVector* out);
GAME_FN(void, __thiscall, Camera_Unknown4F35E0, 0x004F35E0, CCamera* camera, const CVector* position, float angle, float c);
GAME_FN(void, __thiscall, Camera_ResetFov, 0x004F1660, CCamera* camera);

constexpr double kPi = 3.1415927410125732;       // (the float pi, as the game's double)
constexpr double kHalfPi = 1.5707963705062866;
constexpr double kTenth = 0.10000000149011612;   // (0.1f as a double)

struct FirstPersonCameraController;
// own functions, called through the game's address
GAME_FN(CVector*, __thiscall, Game_GetEyePosition, 0x004FA890, FirstPersonCameraController* self, CVector* out);
GAME_FN(void, __thiscall, Game_LookAtPoint, 0x004FA9C0, FirstPersonCameraController* self, const CVector* point);
GAME_FN(void, __thiscall, Game_LookAtTarget, 0x004FAAB0, FirstPersonCameraController* self);
GAME_FN(void, __thiscall, Game_ApplyYaw, 0x004FAB30, FirstPersonCameraController* self);
GAME_FN(void, __thiscall, Game_ApplyPitch, 0x004FAC60, FirstPersonCameraController* self);
GAME_FN(void, __thiscall, Game_StartFromOffsetCamera, 0x004FADB0, FirstPersonCameraController* self);
GAME_FN(void, __thiscall, Game_UpdateNearClip, 0x004FAF30, FirstPersonCameraController* self);

using re::At;

// an entity's position (its matrix's, or the placement's without one)
const CVector* EntityPosition(void* entity) {
    void* matrix = At<void*>(entity, 0x14);
    return matrix ? &At<CVector>(matrix, 0x30) : &At<CVector>(entity, 4);
}

// the collision point the line tests fill (0x28, as World.h's CColPoint)
struct ColPoint {
    CVector point;       // +0
    re::Ptr32<void> entity;        // +0xC (a registered reference)
    CVector normal;      // +0x10
    int32_t piece;       // +0x1C (left uninitialised)
    uint16_t surfaceA;   // +0x20 (left uninitialised)
    uint16_t surfaceB;   // +0x22
    float depth;         // +0x24
};
static_assert(sizeof(ColPoint) == 0x28);

void InitColPoint(ColPoint& p) {
    p.point = {0.0f, 0.0f, 0.0f};
    p.entity = nullptr;
    p.normal = {0.0f, 0.0f, 0.0f};
    p.depth = 3.4028234663852886e+38f;
}

// (both of the game's branches release it the same way)
void ReleaseColPoint(ColPoint& p) {
    if (p.entity) {
        CleanUpOldReference(p.entity, &p.entity);
    }
}

// the attribute set (partial)
struct FirstPersonButes {
    uint8_t unknown0[0x18];  // +0
    float yawEase;           // +0x18 how much of the yaw left is applied each frame
    float pitchEase;         // +0x1C
    float pitchMin;          // +0x20 degrees
    float pitchMax;          // +0x24 degrees
    float pitchDefault;      // +0x28 (0..1)
    float eyeHeight;         // +0x2C
    float nearClip;          // +0x30
    float nearClipBlocked;   // +0x34 (something within `probeLength` in front)
    float probeLength;       // +0x38
};
static_assert(offsetof(FirstPersonButes, probeLength) == 0x38);

struct FirstPersonCameraController {
    re::Ptr32<void> vtable;                // +0 (0x914540)
    float yaw;                   // +4 degrees
    float targetYaw;             // +8 degrees
    int32_t yawChanged;          // +0xC targetYaw to apply
    float pitchAngle;            // +0x10 radians from the vertical
    float yawLeft;               // +0x14 radians still to turn
    float yawInput;              // +0x18
    float pitch;                 // +0x1C 0..1
    float targetPitch;           // +0x20 0..1
    uint8_t waitForCentredStick; // +0x24
    uint8_t justReset;           // +0x25
    uint8_t unknown26[2];
    re::Ptr32<FirstPersonButes> butes;     // +0x28
};
static_assert(sizeof(FirstPersonCameraController) == 0x2C);

float PitchMin(const FirstPersonButes* b) {
    return float(double(b->pitchMin) * kPi / 180.0);
}
float PitchMax(const FirstPersonButes* b) {
    return float(double(b->pitchMax) * kPi / 180.0);
}

// 0..1 (NaN kept)
float Clamp01(float t) {
    if (t < 0.0f) {
        return 0.0f;
    }
    if (t > 1.0f) {
        return 1.0f;
    }
    return t;
}

// ---- FirstPersonCameraController

// 0x4FA6E0 (vtable[8]): the attribute set by name (the default one when there's no
// name or it isn't found)
uint8_t __fastcall FirstPersonCameraController_Vf8(FirstPersonCameraController* self, re::Edx, int name) {
    uint32_t parts[2];
    alignas(8) uint8_t path[8];
    void* butes;
    if (name != 0) {
        Name_Copy(&parts[0], kButesRoot);
        Name_Construct(&parts[1], name);
        Path_Build(path, 2, parts);
        butes = Config_Find(path, kButesType);
        for (int i = 1; i >= 0; i--) {
            EmptyThiscall(&parts[i]);
        }
        if (butes) {
            self->butes = static_cast<FirstPersonButes*>(butes);
            return 1;
        }
    }
    Name_Copy(&parts[0], kButesRoot);
    Name_Copy(&parts[1], kButesDefault);
    Path_Build(path, 2, parts);
    butes = Config_Find(path, kButesType);
    for (int i = 1; i >= 0; i--) {
        EmptyThiscall(&parts[i]);
    }
    self->butes = static_cast<FirstPersonButes*>(butes);
    return 1;
}
FPCAMERA_REVERSED("FirstPersonCameraController::vf8", 0x004FA6E0, RE_BODY(&FirstPersonCameraController_Vf8), nullptr, nullptr, re::Mode::Original);

// 0x4FA7C0: the view direction from the yaw and the pitch (normalised)
CVector* __fastcall FirstPersonCameraController_GetViewDirection(FirstPersonCameraController* self, re::Edx, CVector* out) {
    Vector_FromAngle(out, float(double(self->yaw) * kPi / 180.0));
    float min = PitchMin(self->butes);
    float max = PitchMax(self->butes);
    float pitch = float((double(max) - min) * self->pitch + min);
    float fromVertical = float(kHalfPi - pitch);
    out->z = float(std::tan(double(fromVertical)));
    Vector_Normalise(out);
    return out;
}
FPCAMERA_REVERSED("FirstPersonCameraController::GetViewDirection", 0x004FA7C0, RE_BODY(&FirstPersonCameraController_GetViewDirection), nullptr, nullptr, re::Mode::Original);

// 0x4FA860 (the vtable, its reload callback, no attribute set yet)
FirstPersonCameraController* __fastcall FirstPersonCameraController_Construct(FirstPersonCameraController* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(0x00914540);
    AddLoadCallback(reinterpret_cast<void*>(0x00509960), self);
    self->butes = nullptr;
    return self;
}
FPCAMERA_REVERSED("FirstPersonCameraController::FirstPersonCameraController", 0x004FA860, RE_BODY(&FirstPersonCameraController_Construct), nullptr, nullptr, re::Mode::Original);

// 0x4FA890: the eye: the camera target's position, its height from 0x6C7710's
// position (+0x304) plus the attribute set's eye height. (The forward and right
// offsets are scaled by 0: kept, but they add nothing)
CVector* __fastcall FirstPersonCameraController_GetEyePosition(FirstPersonCameraController* self, re::Edx, CVector* out) {
    float eyeHeight = self->butes->eyeHeight;
    void* ped = TheCamera.targetPed;
    void* matrix = At<void*>(ped, 0x14);
    *out = *EntityPosition(ped);
    CVector forward;
    const CVector* f = Entity_GetForward(ped, &forward);
    CVector forward0 = {float(f->x * 0.0), float(f->y * 0.0), float(f->z * 0.0)};
    CVector right;
    if (matrix) {
        right = At<CVector>(matrix, 0);
    } else {
        Placement_GetRight(static_cast<uint8_t*>(ped) + 4, &right);
    }
    CVector right0 = {float(right.x * 0.0), float(right.y * 0.0), float(right.z * 0.0)};
    CVector sum;
    const CVector* s = VectorAdd(&sum, &right0, &forward0);
    out->x = s->x + out->x;
    out->y = s->y + out->y;
    out->z = s->z + out->z;
    uint8_t* object = Fn_6C7710(Fn_5EE1A0(At<void*>(ped, 0x18)));
    out->z = eyeHeight + At<float>(object, 0x304);
    return out;
}
FPCAMERA_REVERSED("FirstPersonCameraController::GetEyePosition", 0x004FA890, RE_BODY(&FirstPersonCameraController_GetEyePosition), nullptr, nullptr, re::Mode::Original);

// 0x4FA9C0: turns towards a point (yaw at once on the next process, pitch both now
// and as the target)
void __fastcall FirstPersonCameraController_LookAtPoint(FirstPersonCameraController* self, re::Edx, const CVector* point) {
    CVector eye;
    Game_GetEyePosition(self, &eye);
    CVector dir;
    VectorSub(&dir, point, &eye);
    float heading = GetHeading(&dir);
    self->yawChanged = 1;
    self->targetYaw = float(double(heading) * 180.0 / kPi);
    float fromVertical = float(kHalfPi - GetPitch(&dir));
    float min = PitchMin(self->butes);
    float max = PitchMax(self->butes);
    float t = float((double(fromVertical) - min) / (double(max) - min));
    t = Clamp01(t);
    self->pitch = t;
    self->targetPitch = t;
}
FPCAMERA_REVERSED("FirstPersonCameraController::LookAtPoint", 0x004FA9C0, RE_BODY(&FirstPersonCameraController_LookAtPoint), nullptr, nullptr, re::Mode::Original);

// 0x4FAAB0: at the camera target's target (a ped: 0x2C8 into its model's +8, the head
// presumably)
void __fastcall FirstPersonCameraController_LookAtTarget(FirstPersonCameraController* self, re::Edx) {
    uint8_t* target = static_cast<uint8_t*>(Ped_GetTarget(TheCamera.targetPed));
    CVector point = {0.0f, 0.0f, 0.0f};
    const CVector* p;
    if ((At<uint8_t>(target, 0x108) & 7) == 3) {
        p = &At<CVector>(At<void*>(At<void*>(At<void*>(target, 0x114), 0x14), 8), 0x2C8);
    } else {
        p = EntityPosition(target);
    }
    point = *p;
    Game_LookAtPoint(self, &point);
}
FPCAMERA_REVERSED("FirstPersonCameraController::LookAtTarget", 0x004FAAB0, RE_BODY(&FirstPersonCameraController_LookAtTarget), nullptr, nullptr, re::Mode::Original);

// 0x4FAB30: eases the yaw input in and turns the node about the world's up axis
// through its own position; the yaw back from the node's facing (its third column)
void __fastcall FirstPersonCameraController_ApplyYaw(FirstPersonCameraController* self, re::Edx) {
    if (self->yawInput == 0.0f) {
        float d = std::fabs(self->yawInput - self->yawLeft);
        if (d < kTenth) {
            self->yawLeft = self->yawInput;
        }
    }
    self->yawLeft = float((double(self->yawInput) - self->yawLeft) * self->butes->yawEase + self->yawLeft);
    void* node = GetCameraNode(TheCamera.camera);
    cam::NiTransform* t = GetLocalTransform(node);
    CVector position = t->translate;
    CVector back = {-position.x, -position.y, -position.z};
    Node_Translate(node, &back, 2);
    CVector up = {0.0f, 0.0f, 1.0f};
    Node_Rotate(node, &up, self->yawLeft, 2);
    Node_Translate(node, &position, 2);
    self->yawInput = 0.0f;
    CVector facing = {t->rotate[2], t->rotate[5], t->rotate[8]};
    self->yaw = float(double(GetHeading(&facing)) * 180.0 / kPi);
}
FPCAMERA_REVERSED("FirstPersonCameraController::ApplyYaw", 0x004FAB30, RE_BODY(&FirstPersonCameraController_ApplyYaw), nullptr, nullptr, re::Mode::Original);

// 0x4FAC60: eases the pitch towards its target and tilts the node about its own
// right axis (first column) from where it is to there
void __fastcall FirstPersonCameraController_ApplyPitch(FirstPersonCameraController* self, re::Edx) {
    self->pitch = float((double(self->targetPitch) - self->pitch) * self->butes->pitchEase + self->pitch);
    void* node = GetCameraNode(TheCamera.camera);
    cam::NiTransform* t = GetLocalTransform(node);
    float up = t->rotate[8];
    CVector position = t->translate;
    // (above 1, or NaN: 0)
    self->pitchAngle = up <= 1.0f ? float(std::acos(double(up))) : 0.0f;
    const FirstPersonButes* b = self->butes;
    float wanted = float((double(b->pitchMax) - b->pitchMin) * self->pitch + b->pitchMin);
    float current = float(double(self->pitchAngle) * 57.2957763671875);
    float angle = wanted - current;
    CVector back = {-position.x, -position.y, -position.z};
    Node_Translate(node, &back, 2);
    CVector axis = {t->rotate[0], t->rotate[3], t->rotate[6]};
    Node_Rotate(node, &axis, angle, 2);
    Node_Translate(node, &position, 2);
}
FPCAMERA_REVERSED("FirstPersonCameraController::ApplyPitch", 0x004FAC60, RE_BODY(&FirstPersonCameraController_ApplyPitch), nullptr, nullptr, re::Mode::Original);

// 0x4FADB0: from the offset camera (controllers[13]): looks at what's up to 50 ahead
// of it, or takes its heading and pitch when nothing is
void __fastcall FirstPersonCameraController_StartFromOffsetCamera(FirstPersonCameraController* self, re::Edx) {
    uint8_t* offset = static_cast<uint8_t*>(offsetCamera);
    CVector from = At<CVector>(offset, 0x28);
    CVector dir;
    OffsetCamera_GetDirection(offset, &dir);
    CVector ahead = {float(dir.x * 50.0), float(dir.y * 50.0), float(dir.z * 50.0)};
    CVector to;
    VectorAdd(&to, &from, &ahead);
    ColPoint point;
    InitColPoint(point);
    re::Ptr32<void> entity;   // (written by the test)
    if (ProcessLineOfSight(&from, &to, &point, &entity, 0x10F, 1, 1)) {
        Game_LookAtPoint(self, &point.point);
    } else {
        float min = PitchMin(self->butes);
        float max = PitchMax(self->butes);
        float t = float((double(At<float>(offset, 0x24)) - min) / (double(max) - min));
        t = Clamp01(t);
        self->targetYaw = float(double(At<float>(offset, 4)) * 180.0 / kPi);
        self->pitch = t;
        self->targetPitch = t;
    }
    ReleaseColPoint(point);
}
FPCAMERA_REVERSED("FirstPersonCameraController::StartFromOffsetCamera", 0x004FADB0, RE_BODY(&FirstPersonCameraController_StartFromOffsetCamera), nullptr, nullptr, re::Mode::Original);

// 0x4FAF30: a nearer clip plane while something (not the player) is within the probe
// length in front
void __fastcall FirstPersonCameraController_UpdateNearClip(FirstPersonCameraController* self, re::Edx) {
    ColPoint point;
    InitColPoint(point);
    cam::NiTransform* t = GetLocalTransform(GetCameraNode(TheCamera.camera));
    CVector position = t->translate;
    float length = self->butes->probeLength;
    t = GetLocalTransform(GetCameraNode(TheCamera.camera));
    CVector facing = {t->rotate[2], t->rotate[5], t->rotate[8]};
    CVector ahead = {facing.x * length, facing.y * length, facing.z * length};
    CVector end;
    VectorAdd(&end, &position, &ahead);
    ignoreEntity = playerPed;
    re::Ptr32<void> entity = nullptr;
    uint8_t hit = ProcessLineOfSight(&end, &position, &point, &entity, 0x8F, 0, 0);
    ignoreEntity = nullptr;
    Camera_SetNearClip(TheCamera.camera, hit ? self->butes->nearClipBlocked : self->butes->nearClip);
    ReleaseColPoint(point);
}
FPCAMERA_REVERSED("FirstPersonCameraController::UpdateNearClip", 0x004FAF30, RE_BODY(&FirstPersonCameraController_UpdateNearClip), nullptr, nullptr, re::Mode::Original);

// 0x4FB0A0 (vtable[3]): the view reset: from the offset camera, at the target's
// target, or along the player's heading at the default pitch
void __fastcall FirstPersonCameraController_Reset(FirstPersonCameraController* self, re::Edx) {
    self->yawChanged = 1;
    uint8_t* ped = static_cast<uint8_t*>(playerPed);
    self->yawLeft = 0.0f;
    self->yawInput = 0.0f;
    if (cameraMode94 == 0xD) {
        Game_StartFromOffsetCamera(self);
    } else if (ped && cameraFlag284 && Ped_GetTarget(ped)) {
        Game_LookAtTarget(self);
    } else {
        float yaw;
        if (!ped) {
            yaw = 0.0f;
        } else {
            float heading;
            if (void* matrix = At<void*>(ped, 0x14)) {
                // (the forward row: atan2(-x, y))
                heading = float(std::atan2(-double(At<float>(matrix, 0x10)), double(At<float>(matrix, 0x14))));
            } else {
                heading = At<float>(ped, 0x10);
            }
            yaw = float(double(heading) * 180.0 / kPi);
        }
        self->targetYaw = yaw;
        self->pitch = self->butes->pitchDefault;
        self->targetPitch = self->butes->pitchDefault;
    }
    self->waitForCentredStick = 1;
    self->justReset = 1;
    Camera_ResetFov(&TheCamera);
}
FPCAMERA_REVERSED("FirstPersonCameraController::Reset", 0x004FB0A0, RE_BODY(&FirstPersonCameraController_Reset), nullptr, nullptr, re::Mode::Original);

// 0x4FB150 (vtable[7], process)
void __fastcall FirstPersonCameraController_Process(FirstPersonCameraController* self, re::Edx) {
    if (!self->justReset) {
        // the pitch back from the node (when not just reset)
        cam::NiTransform* t = GetLocalTransform(GetCameraNode(TheCamera.camera));
        float up = t->rotate[8];
        float angle = std::fabs(up) <= 1.0f ? float(std::acos(double(up))) : 0.0f;
        float min = PitchMin(self->butes);
        float max = PitchMax(self->butes);
        float pitch = float((double(angle) - min) / (double(max) - min));
        self->pitch = pitch;
        if (cameraFlag224 && cameraFlag225) {
            self->targetPitch = pitch;
        }
    }
    if (self->yawChanged) {
        cam::NiTransform* t = GetLocalTransform(GetCameraNode(TheCamera.camera));
        CVector position = t->translate;
        float angle = float((double(self->targetYaw) - 180.0) * kPi / 180.0);
        Camera_Unknown4F35E0(&TheCamera, &position, angle, 0.0f);
        self->yaw = self->targetYaw;
        self->yawChanged = 0;
    }
    if (!self->waitForCentredStick) {
        self->yawInput = lookX;
        float pitch = self->targetPitch - lookY;
        self->targetPitch = pitch;
        if (pitch < 0.0f) {
            self->targetPitch = 0.0f;
        } else if (pitch > 1.0f) {
            self->targetPitch = 1.0f;
        }
    } else if (std::fabs(lookX) < kTenth && std::fabs(lookY) < kTenth) {
        self->waitForCentredStick = 0;
    }
    Game_ApplyYaw(self);
    Game_ApplyPitch(self);
    Game_UpdateNearClip(self);
    CVector eye;
    Game_GetEyePosition(self, &eye);
    CameraController_MoveTo(self, &eye);
    self->justReset = 0;
}
FPCAMERA_REVERSED("FirstPersonCameraController::Process", 0x004FB150, RE_BODY(&FirstPersonCameraController_Process), nullptr, nullptr, re::Mode::Original);

} // namespace
