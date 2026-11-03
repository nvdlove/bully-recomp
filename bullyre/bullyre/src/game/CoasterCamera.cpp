#include "game/Camera.h"

#include <cmath>
#include <cstring>
#include "reversed.h"

// The roller coaster camera (CoasterCameraController, 0x70 bytes, vtable 0x91433C):
// rides the nearest of the three special props (the coaster cars, 0xC3B7F0) on the
// node 0x1B65514, smoothing the view towards it, and rumbles the pad by the ride's
// speed. The settings are at *0x20C887C. After it, 0x4F4E00..0x4F4F20: the
// conversation camera's shot blending helpers (the shot record: two pairs of vectors
// at +0xC..+0x38, the previous ones at +0x3C..+0x68).

#define COASTERCAM_REVERSED(...) REVERSED(__VA_ARGS__)

namespace {

using cam::CVector;
using cam::NiTransform;

GAME_VAR(float, timeStep, 0x00C1A9A4);           // CTimer::ms_fTimeStep
GAME_VAR(void*, playerPed, 0x00C1AEA8);          // FindPlayerPed
GAME_VAR(uint8_t*, coasterSettings, 0x020C887C);
GAME_VAR(int32_t, coasterNode, 0x01B65514);      // the node the camera rides on
GAME_VAR(int32_t, coasterModel, 0x00BF4488);     // the first special prop's model
GAME_VAR(float, coasterTimerRate, 0x00A45600);
GAME_ARRAY(uint8_t*, specialProps, 0x00C3B7F0);  // three
// (0xA455A8: eleven ranges of the ride timer, low and high, that give a jolt)
GAME_ARRAY(const float, rumbleRanges, 0x00A455A8);
constexpr float kNearPlane = 0.05f;              // (0x9068B8)

GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(NiTransform*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(void, __thiscall, NiTransform_MakeIdentity, 0x00770920, NiTransform* self);
GAME_FN(void, __cdecl, SetNearPlane, 0x006C8820, void* camera, float plane);
GAME_FN(uint8_t, __cdecl, GetNodeTransform, 0x00467D80, void* entity, int node, NiTransform* transform, int flag);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __cdecl, AngleDiff, 0x00432980, float a, float b);
GAME_FN(float, __cdecl, LimitAngle0To2Pi, 0x00432930, float angle);
GAME_FN(float, __cdecl, VecHeading, 0x00433190, const CVector* v);
GAME_FN(float, __cdecl, VecPitch, 0x00433490, const CVector* v);
GAME_FN(void, __cdecl, Pad_Rumble, 0x00738680, int pad, int duration, int strength, int arg);
GAME_FN(int, __cdecl, RandomInt, 0x0040DA80, int min, int max);
GAME_FN(void, __thiscall, Camera_LookFromAngles, 0x004F35E0, CCamera* self, const CVector* position, float heading, float pitch);
// its own
GAME_FN(uint8_t, __thiscall, Coaster_IsInRumbleRange, 0x004F4250, void* self);
GAME_FN(void, __thiscall, Coaster_FindNearest, 0x004F42D0, void* self, re::Ptr32<void>* car, NiTransform* transform);
GAME_FN(void, __thiscall, Coaster_UpdateRumble, 0x004F43D0, void* self, const NiTransform* transform);
GAME_FN(void, __thiscall, Coaster_UpdateView, 0x004F4670, void* self, const NiTransform* transform);

using re::At;

template <typename T>
T Setting(size_t offset) {
    return At<T>(coasterSettings, offset);
}

struct CoasterCameraController {
    re::Ptr32<void> vtable;          // +0 (0x91433C)
    re::Ptr32<void> car;             // +4 the prop ridden
    NiTransform last;      // +8 the node's transform last frame
    int32_t samples;       // +0x3C
    float distance;        // +0x40 moved over the samples
    float speed;           // +0x44 (per sample, times 30)
    float rideTime;        // +0x48
    int32_t joltCooldown;  // +0x4C
    float pitch;           // +0x50
    float heading;         // +0x54
    float roll;            // +0x58
    CVector position;      // +0x5C
    int32_t lowCount;      // +0x68
    int32_t highCount;     // +0x6C
};
static_assert(sizeof(CoasterCameraController) == 0x70);
static_assert(offsetof(CoasterCameraController, samples) == 0x3C);
static_assert(offsetof(CoasterCameraController, position) == 0x5C);

// 0x4F4250: the ride time inside one of the jolt ranges (both ends excluded)
uint8_t __fastcall Coaster_IsInRumbleRangeOurs(CoasterCameraController* self, re::Edx) {
    for (int i = 0; i < 11; i++) {
        if (rumbleRanges[i * 2] < self->rideTime && self->rideTime < rumbleRanges[i * 2 + 1]) {
            return 1;
        }
    }
    return 0;
}
COASTERCAM_REVERSED("CoasterCameraController::IsInRumbleRange", 0x004F4250, RE_BODY(&Coaster_IsInRumbleRangeOurs), nullptr, nullptr, re::Mode::Original);

// 0x4F42D0: the car (and its node's transform) nearest the player (this unused)
void __fastcall Coaster_FindNearestOurs(void*, re::Edx, re::Ptr32<void>* car, NiTransform* transform) {
    float best = 1000000.0f;  // (0x903498)
    NiTransform node;
    node.scale = 1.0f;
    for (int i = 0; i < 3; i++) {
        if (!specialProps[i]) {
            continue;
        }
        At<int32_t>(specialProps[i], 0xA4) = 0;
        if (!GetNodeTransform(specialProps[i], coasterNode, &node, 0)) {
            continue;
        }
        uint8_t* player = static_cast<uint8_t*>(playerPed);
        void* matrix = At<void*>(player, 0x14);
        const CVector* position = matrix ? &At<CVector>(matrix, 0x30) : &At<CVector>(player, 4);
        CVector temp;
        CVector* d = VectorSub(&temp, &node.translate, position);
        double sum = double(d->y) * d->y + double(d->x) * d->x;
        float lengthSq = float(sum + double(d->z) * d->z);
        lengthSq = std::fabs(lengthSq);
        float distance = float(std::sqrt(double(lengthSq)));
        if (distance < best) {
            best = distance;
            *car = specialProps[i];
            *transform = node;
        }
    }
}
COASTERCAM_REVERSED("CoasterCameraController::FindNearest", 0x004F42D0, RE_BODY(&Coaster_FindNearestOurs), nullptr, nullptr, re::Mode::Original);

// 0x4F43D0: the ride's speed (averaged over settings+0x18 frames) and time, and the
// pad's rumble from them
void __fastcall Coaster_UpdateRumbleOurs(CoasterCameraController* self, re::Edx, const NiTransform* transform) {
    CVector old = self->last.translate;
    CVector now = transform->translate;
    float dx = float(double(old.x) - now.x);
    float dy = float(double(old.y) - now.y);
    float dz = float(double(old.z) - now.z);
    double sum = double(dy) * dy + double(dx) * dx;
    float lengthSq = float(sum + double(dz) * dz);
    lengthSq = std::fabs(lengthSq);
    float moved = float(std::sqrt(double(lengthSq)));
    self->samples++;
    float distance = float(double(moved) + self->distance);
    self->distance = distance;
    self->rideTime = float(double(float(double(coasterTimerRate) * timeStep)) + self->rideTime);
    // (the rumble strength below is pushed from the argument's stack slot with only
    // its low byte set: the rest is what was last stored there)
    uint32_t slot;
    int32_t perAverage = Setting<int32_t>(0x18);
    slot = uint32_t(perAverage);
    if (self->samples == perAverage) {
        float average = float(double(distance) / perAverage);
        std::memcpy(&slot, &average, 4);
        self->samples = 0;
        self->speed = float(double(average) * 30.0);
        self->distance = 0.0f;
    }
    if (At<int16_t>(self->car, 0x10E) != coasterModel) {
        return;
    }
    float time = self->rideTime;
    if (time < Setting<float>(0x1C)) {
        if (self->lowCount == Setting<int32_t>(0x28)) {
            self->lowCount = 0;
            Pad_Rumble(0, 100, Setting<uint8_t>(0x38), Setting<uint8_t>(0x3C));
        }
        if (Setting<int32_t>(0x28) == 0) {
            self->lowCount = 0;
        } else {
            self->lowCount++;
        }
    } else if (time < Setting<float>(0x20)) {
        Pad_Rumble(0, 0x28, 0xFF, 0);
    } else if (time < Setting<float>(0x24)) {
        if (self->highCount == Setting<int32_t>(0x2C)) {
            self->highCount = 0;
            Pad_Rumble(0, 100, Setting<uint8_t>(0x30), Setting<uint8_t>(0x34));
        }
        if (Setting<int32_t>(0x2C) == 0) {
            self->highCount = 0;
        } else {
            self->highCount++;
        }
    }
    if (self->joltCooldown != 0) {
        self->joltCooldown--;
        return;
    }
    if (Coaster_IsInRumbleRange(self)) {
        Pad_Rumble(0, 100, 0xFF, 0);
        self->joltCooldown = Setting<int32_t>(0x40);
        return;
    }
    int strength;
    if (self->speed < 10.0f) {
        strength = RandomInt(Setting<int32_t>(0x44), Setting<int32_t>(0x48));
    } else if (self->speed > 25.0) {
        strength = RandomInt(Setting<int32_t>(0x4C), Setting<int32_t>(0x50));
    } else {
        strength = RandomInt(Setting<int32_t>(0x54), Setting<int32_t>(0x58));
    }
    slot = (slot & 0xFFFFFF00u) | uint8_t(strength);
    Pad_Rumble(0, 500, int(slot), 0);
}
COASTERCAM_REVERSED("CoasterCameraController::UpdateRumble", 0x004F43D0, RE_BODY(&Coaster_UpdateRumbleOurs), nullptr, nullptr, re::Mode::Original);

// (float(a * k) for each of a vector's components)
CVector Scaled(const CVector& v, float k) {
    return {float(double(v.x) * k), float(double(v.y) * k), float(double(v.z) * k)};
}

// 0x4F4670: the view eased towards the car's node: the angles by settings +0x74/+0x70
// /+0x78, the position through a spring along the node's own axes (+0x64 right, +0x68
// up, +0x6C forward, the step by +0x60, forward kept within +-settings+0x84)
void __fastcall Coaster_UpdateViewOurs(CoasterCameraController* self, re::Edx, const NiTransform* t) {
    CVector forward = {t->rotate[2], t->rotate[5], t->rotate[8]};
    CVector copy = forward;
    float pitch = VecPitch(&copy);
    copy = forward;
    float heading = VecHeading(&copy);
    float pitchDiff = AngleDiff(self->pitch, pitch);
    float headingDiff = AngleDiff(self->heading, heading);
    float rollDiff = AngleDiff(self->roll, 0.0f);
    self->pitch = LimitAngle0To2Pi(float(double(Setting<float>(0x74)) * pitchDiff + self->pitch));
    self->heading = LimitAngle0To2Pi(float(double(Setting<float>(0x70)) * headingDiff + self->heading));
    self->roll = LimitAngle0To2Pi(float(double(Setting<float>(0x78)) * rollDiff + self->roll));

    CVector right = {t->rotate[0], t->rotate[3], t->rotate[6]};
    CVector up = {t->rotate[1], t->rotate[4], t->rotate[7]};
    CVector d;
    VectorSub(&d, &t->translate, &self->position);
    float forwardDot = float((double(d.x) * forward.x + double(d.y) * forward.y) + double(forward.z) * d.z);
    float rightDot = float((double(right.x) * d.x + double(right.y) * d.y) + double(right.z) * d.z);
    float upDot = float((double(d.y) * up.y + double(d.x) * up.x) + double(d.z) * up.z);
    CVector upPart = Scaled(Scaled(up, upDot), Setting<float>(0x68));
    CVector rightPart = Scaled(Scaled(right, rightDot), Setting<float>(0x64));
    CVector forwardPart = Scaled(Scaled(forward, forwardDot), Setting<float>(0x6C));
    CVector total;
    total.x = float(float(double(forwardPart.x) + rightPart.x) + double(upPart.x));
    total.y = float(float(double(forwardPart.y) + rightPart.y) + double(upPart.y));
    total.z = float(float(double(forwardPart.z) + rightPart.z) + double(upPart.z));
    CVector step = Scaled(total, Setting<float>(0x60));
    CVector moved;
    VectorAdd(&moved, &self->position, &step);

    // the new position relative to the node, the forward part clamped
    VectorSub(&d, &moved, &t->translate);
    forwardDot = float((double(d.x) * forward.x + double(d.y) * forward.y) + double(d.z) * forward.z);
    rightDot = float((double(d.x) * right.x + double(d.y) * right.y) + double(d.z) * right.z);
    upDot = float((double(d.y) * up.y + double(d.x) * up.x) + double(d.z) * up.z);
    float limit = Setting<float>(0x84);
    if (limit < forwardDot) {
        forwardDot = limit;
    }
    float negative = -Setting<float>(0x84);
    if (forwardDot < negative) {
        forwardDot = negative;
    }
    upPart = Scaled(Scaled(up, upDot), Setting<float>(0x68));
    rightPart = Scaled(Scaled(right, rightDot), Setting<float>(0x64));
    forwardPart = Scaled(Scaled(forward, forwardDot), Setting<float>(0x6C));
    CVector offset;
    offset.x = float(float(double(forwardPart.x) + rightPart.x) + double(upPart.x));
    offset.y = float(float(double(forwardPart.y) + rightPart.y) + double(upPart.y));
    offset.z = float(float(double(forwardPart.z) + rightPart.z) + double(upPart.z));
    CVector result;
    self->position = *VectorAdd(&result, &t->translate, &offset);
    Camera_LookFromAngles(&TheCamera, &self->position, self->heading, self->pitch);
}
COASTERCAM_REVERSED("CoasterCameraController::UpdateView", 0x004F4670, RE_BODY(&Coaster_UpdateViewOurs), nullptr, nullptr, re::Mode::Original);

// 0x4F4C40 (vtable[7], process): the car's node this frame; the camera eased towards
// it (settings+0x5C) or put right on it
void __fastcall Coaster_Process(CoasterCameraController* self, re::Edx) {
    NiTransform node;
    node.scale = 1.0f;
    if (!GetNodeTransform(self->car, coasterNode, &node, 0)) {
        return;
    }
    Coaster_UpdateRumble(self, &node);
    if (Setting<uint8_t>(0x5C) != 0) {
        Coaster_UpdateView(self, &node);
        self->last = node;
        return;
    }
    *GetLocalTransform(GetCameraNode(TheCamera.camera)) = node;
    self->last = node;
}
COASTERCAM_REVERSED("CoasterCameraController::Process", 0x004F4C40, RE_BODY(&Coaster_Process), nullptr, nullptr, re::Mode::Original);

// 0x4F4CE0 (vtable[3], reset): the near plane brought in and the nearest car taken
void __fastcall Coaster_Reset(CoasterCameraController* self, re::Edx) {
    self->distance = 0.0f;
    self->speed = 0.0f;
    self->rideTime = 0.0f;
    self->samples = 0;
    self->joltCooldown = 0;
    NiTransform_MakeIdentity(&self->last);
    self->lowCount = 0;
    self->highCount = 0;
    if (TheCamera.camera) {
        SetNearPlane(TheCamera.camera, kNearPlane);
    }
    NiTransform node;
    node.scale = 1.0f;
    re::Ptr32<void> car = nullptr;
    Coaster_FindNearest(self, &car, &node);
    if (!car) {
        return;
    }
    self->roll = 0.0f;
    self->last = node;
    CVector forward = {node.rotate[2], node.rotate[5], node.rotate[8]};
    self->car = car;
    CVector copy = forward;
    self->pitch = VecPitch(&copy);
    copy = forward;
    self->heading = VecHeading(&copy);
    self->position = node.translate;
}
COASTERCAM_REVERSED("CoasterCameraController::Reset", 0x004F4CE0, RE_BODY(&Coaster_Reset), nullptr, nullptr, re::Mode::Original);

// ---- the conversation camera's shot record (callers: 0x4F58E0, 0x4F6D40)

// 0x4F4E00: the current vectors kept as the previous ones
void __fastcall CinematicShot_SaveAsPrevious(void* self, re::Edx) {
    for (size_t offset = 0; offset < 0x30; offset += 4) {
        At<uint32_t>(self, 0x3C + offset) = At<uint32_t>(self, 0xC + offset);
    }
    for (size_t offset = 0; offset < 0x18; offset += 4) {
        At<uint32_t>(self, 0x84 + offset) = At<uint32_t>(self, 0x6C + offset);
    }
}
COASTERCAM_REVERSED("CinematicShot::SaveAsPrevious", 0x004F4E00, RE_BODY(&CinematicShot_SaveAsPrevious), nullptr, nullptr, re::Mode::Original);

// a * (1 - w) + b * w, w being blend+0x1C or +0x20 by `index`
CVector* ShotLerp(void* self, CVector* out, void* blend, int index, size_t from, size_t to) {
    float weights[2] = {At<float>(blend, 0x1C), At<float>(blend, 0x20)};
    float w = weights[index];
    const CVector& b = At<CVector>(self, to + index * 0xC);
    CVector bPart = {float(double(b.x) * w), float(double(b.y) * w), float(double(b.z) * w)};
    float rest = float(1.0 - weights[index]);
    const CVector& a = At<CVector>(self, from + index * 0xC);
    CVector aPart = {float(double(a.x) * rest), float(double(a.y) * rest), float(double(a.z) * rest)};
    VectorAdd(out, &aPart, &bPart);
    return out;
}

// 0x4F4E80: between the current pairs (+0xC.. and +0x24..)
CVector* __fastcall CinematicShot_LerpCurrent(void* self, re::Edx, CVector* out, void* blend, int index) {
    return ShotLerp(self, out, blend, index, 0xC, 0x24);
}
COASTERCAM_REVERSED("CinematicShot::LerpCurrent", 0x004F4E80, RE_BODY(&CinematicShot_LerpCurrent), nullptr, nullptr, re::Mode::Original);

// 0x4F4F20: between the previous pairs (+0x3C.. and +0x54..)
CVector* __fastcall CinematicShot_LerpPrevious(void* self, re::Edx, CVector* out, void* blend, int index) {
    return ShotLerp(self, out, blend, index, 0x3C, 0x54);
}
COASTERCAM_REVERSED("CinematicShot::LerpPrevious", 0x004F4F20, RE_BODY(&CinematicShot_LerpPrevious), nullptr, nullptr, re::Mode::Original);

} // namespace
