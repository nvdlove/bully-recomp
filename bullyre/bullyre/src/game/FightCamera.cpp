#include "game/Camera.h"

#include <cmath>
#include <cstring>
#include "reversed.h"

// The fight camera (FightCamCtrl, vtable 0x914510): frames the player and whoever he's
// locked on to, on springs (a velocity each for the position and the look-at point)
// tuned by an attribute set ("Butes", reloaded between interiors and exteriors), and
// pushed out of the world by a capsule collision query. Also the capsule it queries.

#define FIGHTCAM_REVERSED(...) REVERSED(__VA_ARGS__)

namespace {

struct FightCamCtrl {
    re::Ptr32<void> vtable;               // +0 (0x914510)
    cam::CVector position;      // +4
    cam::CVector velocity;      // +0x10
    cam::CVector lookAt;        // +0x1C
    cam::CVector lookVelocity;  // +0x28
    cam::CVector lockTarget;    // +0x34 where the target was last seen
    uint32_t lockTime;          // +0x40 CTimer ms when there was last a target
    uint8_t locked;             // +0x44 framing a target (still, for a while after losing it)
    uint8_t unknown45[3];       // +0x45
    float height;               // +0x48 raised with the distance to the target
    re::Ptr32<uint8_t> butes;             // +0x4C the attribute set
    uint8_t butesSet;           // +0x50 set by name (vtable[8]): not swapped per area
    uint8_t unknown51[3];       // +0x51
    int32_t buteSet;            // +0x54 0 exterior, 2 interior, -1 none
    uint8_t far;                // +0x58 the target is out of framing range
};
static_assert(offsetof(FightCamCtrl, lookVelocity) == 0x28);
static_assert(offsetof(FightCamCtrl, lockTime) == 0x40);
static_assert(offsetof(FightCamCtrl, height) == 0x48);
static_assert(offsetof(FightCamCtrl, buteSet) == 0x54);
static_assert(offsetof(FightCamCtrl, far) == 0x58);

// The attribute set's fields used here (float unless said)
enum : int {
    kButeCamOffset = 0x18,      // +0x18.. +0x20 the camera's offset from the player
    kButeLookHeight = 0x24,     // +0x24 (with a unit y: the look-at point's offset)
    kButeFlag28 = 0x28,         // +0x28 byte: unlock when the player has a weapon out
    kButeNearRange = 0x2C,      // +0x2C
    kButeFarRange = 0x30,       // +0x30
    kButeOrbit = 0x38,          // +0x38 .. +0x40
    kButeBlend = 0x44,          // +0x44 the player's share of the framed midpoint
    kButeLookRaise = 0x48,      // +0x48
    kButeDistanceScale = 0x4C,  // +0x4C
    kButeAngleMode = 0x50,      // +0x50 1.0: a fixed side angle
    kButeMaxAngle = 0x54,       // +0x54 degrees
    kButeAngleSpeed = 0x58,     // +0x58
    kButeSideAngle = 0x5C,      // +0x5C degrees
    kButePosVelScale = 0x60,
    kButeLookVelScale = 0x64,
    kButePosSpring = 0x68,
    kButeLookSpring = 0x6C,
    kButePosDamping = 0x70,
    kButeLookDamping = 0x74,
    kButeRadius = 0x78,
    kButeGroundPush = 0x80,
    kButeHeightFollow = 0x84,
    kButePosBlend = 0x88,
    kButeHeightScale = 0x8C,
    kButeHeightUpRate = 0x90,
    kButeHeightDownRate = 0x94,
    kButeFov = 0x98,
    kButeFovBlend = 0x9C,
    kButeSideMode = 0xA0,       // int: 1 always the right, 2 always the left
    kButeHeightStart = 0xA4,
    kButeHeightMax = 0xA8,
};

// The capsule a collision query takes (0x38 bytes)
struct CameraCapsule {
    cam::CVector middle;     // +0
    cam::CVector direction;  // +0xC (a to b, normalised)
    cam::CVector a;          // +0x18
    cam::CVector b;          // +0x24
    float length;            // +0x30
    float radius;            // +0x34
};
static_assert(sizeof(CameraCapsule) == 0x38);

// A collision query's hit (0x28 bytes, in the array at 0xC216C0)
struct CollisionHit {
    cam::CVector point;    // +0
    float unknownC;        // +0xC
    cam::CVector normal;   // +0x10
    float unknown1C[2];    // +0x1C
    float depth;           // +0x24 0: a hit along the capsule's line, not a penetration
};
static_assert(sizeof(CollisionHit) == 0x28);

struct ButesPath {
    int32_t count;
    re::Ptr32<const uint32_t> parts;
};

GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(cam::CVector*, __cdecl, VectorAdd, 0x004129A0, cam::CVector* out, const cam::CVector* a, const cam::CVector* b);
GAME_FN(cam::CVector*, __cdecl, VectorSub, 0x00414B70, cam::CVector* out, const cam::CVector* a, const cam::CVector* b);
GAME_FN(cam::CVector*, __cdecl, CrossProduct, 0x00414730, cam::CVector* out, const cam::CVector* a, const cam::CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, cam::CVector* v);
GAME_FN(cam::CVector*, __thiscall, Entity_GetForward, 0x004154A0, void* entity, cam::CVector* out);
GAME_FN(float*, __thiscall, Placeable_GetMatrix, 0x0046DD20, void* placeable);
// (0x0041A9A0 stores 16 bytes through `out`: called through re::CallVec16)
static inline cam::CVector* Matrix_TransformPoint(cam::CVector* out, const float* m, const cam::CVector* v) { return re::CallVec16<0x0041A9A0, cam::CVector*>(out, m, v); }
GAME_FN(float, __cdecl, FindGroundZ, 0x004635A0, float x, float y, float z, uint8_t* found, int arg);
GAME_FN(float, __cdecl, GetVectorHeading, 0x00433190, const cam::CVector* v);
GAME_FN(float, __cdecl, WrapAngle, 0x00432930, float angle);
GAME_FN(float, __cdecl, AngleDiff, 0x00432980, float a, float b);
GAME_FN(uint8_t, __cdecl, IsAreaInterior, 0x00430320, int area);
GAME_FN(void*, __thiscall, TargetingSystem_GetTarget, 0x004D5AD0, void* targeting);
GAME_FN(uint8_t, __thiscall, Ped_HasWeapon, 0x00474BF0, void* ped);
GAME_FN(float, __thiscall, Weapon_GetInfoByte68AsFloat, 0x004D96B0, void* weapon);
GAME_FN(void, __thiscall, CopyInt, 0x005FA880, uint32_t* dst, const uint32_t* src);
GAME_FN(void, __thiscall, Name_Construct, 0x005FA910, uint32_t* self, uintptr_t value);
GAME_FN(void, __thiscall, ButesPath_Construct, 0x005FA8C0, ButesPath* self, int count, const uint32_t* parts);
GAME_FN(uint8_t*, __cdecl, Butes_Find, 0x0072AEA0, const ButesPath* path, const uint32_t* root);
GAME_FN(void, __stdcall, DestructArray, 0x004017C0, void* array, size_t size, int count, void* destructor);
GAME_FN(void, __cdecl, RegisterOwned, 0x0072B870, void* fn, void* object);
GAME_FN(void, __thiscall, EmptyThiscall, 0x0049A650, void* self);
GAME_FN(void, __thiscall, CollisionQuery_Construct, 0x00562D80, void* query, const CameraCapsule* capsule, int flags, CollisionHit* hits, int maxHits);
GAME_FN(int, __thiscall, CollisionQuery_Run, 0x00563E80, void* query);
GAME_FN(void, __thiscall, Camera_SetView4F2C60, 0x004F2C60, CCamera* camera, const cam::CVector* position, const cam::CVector* lookAt);
// (0x4F0920, the arcade camera's offset setter: the vector at +0x180, no longer than 10)
GAME_FN(void, __thiscall, Camera_SetVelocity, 0x004F0920, void* self, const cam::CVector* velocity);
GAME_FN(void, __cdecl, Camera_SetNearClip, 0x006C8820, void* camera, float nearClip);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(cam::NiTransform*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);

// own functions, called through the game's address
GAME_FN(void, __thiscall, FightCam_ReloadButes, 0x004F8C40, FightCamCtrl* self);
GAME_FN(cam::CVector*, __thiscall, FightCam_GetTarget, 0x004F8DC0, FightCamCtrl* self, cam::CVector* out);
GAME_FN(void, __thiscall, FightCam_UpdateLock, 0x004F8EE0, FightCamCtrl* self);
GAME_FN(void, __thiscall, FightCam_UpdateRange, 0x004F8FA0, FightCamCtrl* self);
GAME_FN(void, __thiscall, FightCam_ComputeView, 0x004F90E0, FightCamCtrl* self, cam::CVector* position, cam::CVector* lookAt);
GAME_FN(void, __thiscall, FightCam_UpdateHeight, 0x004F9800, FightCamCtrl* self);
GAME_FN(void, __thiscall, FightCam_Follow, 0x004F9BD0, FightCamCtrl* self, cam::CVector position, cam::CVector lookAt);
GAME_FN(void, __thiscall, CameraCapsule_Set, 0x004F8AE0, CameraCapsule* self, const cam::CVector* a, const cam::CVector* b, float radius);

GAME_VAR(void*, playerPed, 0x00C1AEA8);          // FindPlayerPed
GAME_VAR(int32_t, currArea, 0x00BD1008);         // CGame::currArea
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);        // CTimer's
GAME_VAR(float, timeStep, 0x00C1A9A4);           // CTimer's
GAME_VAR(uint8_t*, cameraSettings, 0x020C88B8);  // +0x1C the lock's hold time (ms)
GAME_VAR(void*, secondTarget, 0x00C3CE18);       // TheCamera+0x1B0: a scripted target
GAME_VAR(int32_t, cameraUnknown1A4, 0x00C3CE0C); // TheCamera+0x1A4
GAME_VAR(uint8_t, fovLocked, 0x00C3CF82);        // TheCamera+0x31A: nothing touches the fov
GAME_VAR(float, springScale, 0x00A456B8);        // 0.02
GAME_VAR(float, maxStepScale, 0x00A456B4);       // 10
GAME_VAR(uint8_t, cameraQueryActive, 0x00C1AE8A);
// (4F8EE0's function-local static: the hold time, read once)
GAME_VAR(uint32_t, lockHoldGuard, 0x00C3CFCC);
GAME_VAR(uint32_t, lockHoldMs, 0x00C3CFC8);
inline CollisionHit* const collisionHits = reinterpret_cast<CollisionHit*>(0x00C216C0);
// the Butes names: the root, "fight camera", and the exterior/interior sets
inline const uint32_t* const butesRoot = reinterpret_cast<const uint32_t*>(0x01B653C4);
inline const uint32_t* const butesFightCam = reinterpret_cast<const uint32_t*>(0x01B653C8);
inline const uint32_t* const butesSet404 = reinterpret_cast<const uint32_t*>(0x01B65404);
inline const uint32_t* const butesSet408 = reinterpret_cast<const uint32_t*>(0x01B65408);
inline const uint32_t* const butesSet40C = reinterpret_cast<const uint32_t*>(0x01B6540C);
inline const uint32_t* const butesSet410 = reinterpret_cast<const uint32_t*>(0x01B65410);

constexpr uintptr_t kFightCamCtrlVtable = 0x00914510;
constexpr double kPi = 3.1415927410125732;    // (the float pi, as the game's double)
constexpr double kHalfPi = 1.5707963705062866;
constexpr double kEpsilon = 9.999999747378752e-05;

using re::At;

float Bute(const FightCamCtrl* self, int offset) {
    return *reinterpret_cast<const float*>(self->butes + offset);
}

// an entity's position: its matrix's, or its placement's without one
const cam::CVector* EntityPosition(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<const cam::CVector*>(matrix + 0x30) : &At<cam::CVector>(entity, 4);
}

void* PlayerTarget(void* player) {
    return TargetingSystem_GetTarget(At<void*>(player, 0x1C8));
}

cam::CVector Scaled(const cam::CVector& v, float s) {
    return {float(double(v.x) * s), float(double(v.y) * s), float(double(v.z) * s)};
}

float Length(const cam::CVector& v) {
    float sq = float(double(v.x) * v.x + double(v.y) * v.y + double(v.z) * v.z);
    return float(std::sqrt(double(std::fabs(sq))));
}

float Length2D(const cam::CVector& v) {
    float sq = float(double(v.x) * v.x + double(v.y) * v.y);
    return float(std::sqrt(double(std::fabs(sq))));
}

} // namespace

// 0x4F8AE0 (thiscall): a capsule from a to b
void __fastcall CameraCapsule_SetOurs(CameraCapsule* self, re::Edx, const cam::CVector* a, const cam::CVector* b, float radius) {
    cam::CVector tmp;
    const cam::CVector* sum = VectorAdd(&tmp, a, b);
    self->middle = {float(double(sum->x) * 0.5), float(double(sum->y) * 0.5), float(double(sum->z) * 0.5)};
    self->direction = *VectorSub(&tmp, b, a);
    self->length = Length(self->direction);
    if (self->length != 0.0f) {
        float inverse = float(1.0 / self->length);
        self->direction.x = float(double(self->direction.x) * inverse);
        self->direction.y = float(double(self->direction.y) * inverse);
        self->direction.z = float(double(self->direction.z) * inverse);
    }
    self->radius = radius;
    self->a = *a;
    self->b = *b;
}
FIGHTCAM_REVERSED("CameraCapsule::Set", 0x004F8AE0, RE_BODY(&CameraCapsule_SetOurs), nullptr, nullptr, re::Mode::Original);

// 0x4F8BF0 (thiscall)
FightCamCtrl* __fastcall FightCamCtrl_Construct(FightCamCtrl* self, re::Edx) {
    self->position = {0.0f, 0.0f, 0.0f};
    self->velocity = {0.0f, 0.0f, 0.0f};
    self->vtable = reinterpret_cast<void*>(kFightCamCtrlVtable);
    self->lookAt = {0.0f, 0.0f, 0.0f};
    self->lookVelocity = {0.0f, 0.0f, 0.0f};
    self->lockTarget = {0.0f, 0.0f, 0.0f};
    RegisterOwned(reinterpret_cast<void*>(0x00509960), self);
    return self;
}
FIGHTCAM_REVERSED("FightCamCtrl::FightCamCtrl", 0x004F8BF0, RE_BODY(&FightCamCtrl_Construct), nullptr, nullptr, re::Mode::Original);

// 0x4F8C40 (thiscall): the exterior or interior attribute set, when the area changed
// kind (not when one was set by name). (Of the four name pairs built, the first and
// third are used.)
void __fastcall FightCamCtrl_ReloadButes(FightCamCtrl* self, re::Edx) {
    if (self->butesSet) {
        return;
    }
    int32_t set = IsAreaInterior(currArea) ? 2 : 0;
    if (self->buteSet == set) {
        return;
    }
    self->buteSet = set;
    uint32_t parts[8];
    CopyInt(&parts[0], butesFightCam);
    CopyInt(&parts[1], butesSet404);
    CopyInt(&parts[2], butesFightCam);
    CopyInt(&parts[3], butesSet408);
    CopyInt(&parts[4], butesFightCam);
    CopyInt(&parts[5], butesSet40C);
    CopyInt(&parts[6], butesFightCam);
    CopyInt(&parts[7], butesSet410);
    ButesPath path;
    ButesPath_Construct(&path, 2, &parts[set * 2]);
    self->butes = Butes_Find(&path, butesRoot);
    DestructArray(parts, 4, 8, reinterpret_cast<void*>(0x0049A650));
}
FIGHTCAM_REVERSED("FightCamCtrl::ReloadButes", 0x004F8C40, RE_BODY(&FightCamCtrl_ReloadButes), nullptr, nullptr, re::Mode::Original);

// 0x4F8D30 (vtable[4]): takes over from where the camera is
void __fastcall FightCamCtrl_TakeOver(FightCamCtrl* self, re::Edx) {
    cam::NiTransform* transform = GetLocalTransform(GetCameraNode(TheCamera.camera));
    self->height = 0.0f;
    self->position = transform->translate;
    self->lookAt = TheCamera.unknown198;
    self->velocity = At<cam::CVector>(&TheCamera, 0x180);
    self->lookVelocity = At<cam::CVector>(&TheCamera, 0x18C);
    self->far = 0;
}
FIGHTCAM_REVERSED("FightCamCtrl::vf4", 0x004F8D30, RE_BODY(&FightCamCtrl_TakeOver), nullptr, nullptr, re::Mode::Original);

// 0x4F8DC0 (thiscall): what to frame: the scripted target, the player's target, where
// the last one was (while still locked), or a metre ahead of the player
cam::CVector* __fastcall FightCamCtrl_GetTarget(FightCamCtrl* self, re::Edx, cam::CVector* out) {
    void* player = playerPed;
    cam::CVector position = *EntityPosition(player);
    *out = {0.0f, 0.0f, 0.0f};
    if (secondTarget) {
        *out = *EntityPosition(secondTarget);
        return out;
    }
    if (PlayerTarget(player)) {
        *out = *EntityPosition(PlayerTarget(player));
        return out;
    }
    if (self->locked) {
        *out = self->lockTarget;
        return out;
    }
    cam::CVector forward;
    cam::CVector* f = Entity_GetForward(player, &forward);
    cam::CVector ahead = *f;
    cam::CVector tmp;
    *out = *VectorAdd(&tmp, &position, &ahead);
    return out;
}
FIGHTCAM_REVERSED("FightCamCtrl::GetTarget", 0x004F8DC0, RE_BODY(&FightCamCtrl_GetTarget), nullptr, nullptr, re::Mode::Original);

// 0x4F8EE0 (thiscall): stays locked for a while after the last target (not while the
// player holds a weapon out, when the attribute set says so)
void __fastcall FightCamCtrl_UpdateLock(FightCamCtrl* self, re::Edx) {
    uint8_t hasTarget = (secondTarget != nullptr || PlayerTarget(playerPed) != nullptr) ? 1 : 0;
    if ((lockHoldGuard & 1) == 0) {
        lockHoldMs = At<uint32_t>(cameraSettings, 0x1C);
        lockHoldGuard |= 1;
    }
    if (hasTarget) {
        self->lockTime = timeInMs;
    }
    uint8_t recent = (timeInMs - self->lockTime) < lockHoldMs ? 1 : 0;
    if (cameraUnknown1A4 == 0 && self->butes[kButeFlag28] != 0) {
        void* player = playerPed;
        if (Ped_HasWeapon(player) && !(Weapon_GetInfoByte68AsFloat(At<void*>(player, 0x1D0)) == 0.0f)) {
            self->locked = 0;
            return;
        }
    }
    self->locked = recent;
}
FIGHTCAM_REVERSED("FightCamCtrl::UpdateLock", 0x004F8EE0, RE_BODY(&FightCamCtrl_UpdateLock), nullptr, nullptr, re::Mode::Original);

// 0x4F8FA0 (thiscall): whether the target is too far to frame (with hysteresis)
void __fastcall FightCamCtrl_UpdateRange(FightCamCtrl* self, re::Edx) {
    cam::CVector position = *EntityPosition(playerPed);
    if (!self->locked) {
        self->far = 0;
        return;
    }
    cam::CVector target;
    cam::CVector tmp;
    if (self->far) {
        FightCam_GetTarget(self, &target);
        self->lockTarget = target;
        float distance = Length2D(*VectorSub(&tmp, &position, &target));
        if (Bute(self, kButeNearRange) > distance) {
            self->far = 0;
        }
    } else {
        FightCam_GetTarget(self, &target);
        self->lockTarget = target;
        float distance = Length2D(*VectorSub(&tmp, &position, &target));
        if (Bute(self, kButeFarRange) < distance) {
            self->far = 1;
        }
    }
}
FIGHTCAM_REVERSED("FightCamCtrl::UpdateRange", 0x004F8FA0, RE_BODY(&FightCamCtrl_UpdateRange), nullptr, nullptr, re::Mode::Original);

// 0x4F90E0 (thiscall): where the camera wants to be and look. Locked (and in range):
// beside the line from the player to the target, swung to a side; otherwise at the
// attribute set's offset behind the player.
void __fastcall FightCamCtrl_ComputeView(FightCamCtrl* self, re::Edx, cam::CVector* outPosition, cam::CVector* outLookAt) {
    void* player = playerPed;
    if (!player) {
        *outPosition = {0.0f, 0.0f, 0.0f};
        *outLookAt = {0.0f, 1.0f, 0.0f};
        return;
    }
    cam::CVector playerPos = *EntityPosition(player);
    struct { cam::CVector v; float pad; } tmpPad16;   // (the game's transform writes a 4th float)
    cam::CVector& tmp = tmpPad16.v;
    if (!self->locked || self->far) {
        cam::CVector offset = {Bute(self, kButeCamOffset), Bute(self, kButeCamOffset + 4), Bute(self, kButeCamOffset + 8)};
        cam::CVector ahead = {0.0f, 1.0f, Bute(self, kButeLookHeight)};
        *outPosition = *Matrix_TransformPoint(&tmp, Placeable_GetMatrix(player), &offset);
        cam::CVector aheadWorld = *Matrix_TransformPoint(&tmp, Placeable_GetMatrix(player), &ahead);
        float scale = -offset.y;
        cam::CVector toAhead;
        const cam::CVector* d = VectorSub(&toAhead, &aheadWorld, &playerPos);
        cam::CVector along = {float(double(scale) * d->x), float(double(d->y) * scale), float(double(d->z) * scale)};
        *outLookAt = *VectorAdd(&tmp, outPosition, &along);
        return;
    }

    // both ends on the ground (from 2 m up)
    uint8_t found;
    float ground = FindGroundZ(playerPos.x, playerPos.y, float(double(playerPos.z) + 2.0), &found, 1);
    if (found) {
        playerPos.z = ground;
    }
    cam::CVector target;
    FightCam_GetTarget(self, &target);
    ground = FindGroundZ(target.x, target.y, float(double(target.z) + 2.0), &found, 1);
    if (found) {
        target.z = ground;
    }
    // the framed point, between the two
    float blend = Bute(self, kButeBlend);
    cam::CVector playerShare = Scaled(playerPos, blend);
    float targetBlend = float(1.0 - blend);
    cam::CVector targetShare = Scaled(target, targetBlend);
    cam::CVector middle = *VectorAdd(&tmp, &targetShare, &playerShare);
    cam::CVector line = *VectorSub(&tmp, &target, &playerPos);
    cam::CVector up = {0.0f, 0.0f, 1.0f};
    cam::CVector side = *CrossProduct(&tmp, &line, &up);
    float distance = Length(line);
    float stretch = float(double(distance) * (1.0 / Bute(self, kButeDistanceScale)));
    if (!(stretch > 1.0f)) {
        stretch = 1.0f;
    }
    Vector_Normalise(&line);
    Vector_Normalise(&side);
    float raise = Bute(self, kButeLookRaise);
    float raise0 = float(0.0 * raise);
    cam::CVector lift = {raise0, raise0, raise};
    *outLookAt = *VectorAdd(&tmp, &middle, &lift);

    float angle;
    if (1.0f == Bute(self, kButeAngleMode)) {
        // a fixed angle, on the side the camera is on (or the one the set says)
        cam::CVector fromMiddle = *VectorSub(&tmp, &self->position, &middle);
        float dot = float(double(fromMiddle.y) * side.y + double(side.x) * fromMiddle.x + double(fromMiddle.z) * side.z);
        float sign = 1.0f;
        if (dot < 0.0f) {
            sign = -1.0f;
        }
        int32_t mode = *reinterpret_cast<const int32_t*>(self->butes + kButeSideMode);
        if (mode == 1) {
            sign = 1.0f;
        } else if (mode == 2) {
            sign = -1.0f;
        }
        angle = float((double(Bute(self, kButeSideAngle)) * sign - 90.0) * kPi / 180.0);
    } else {
        // the camera's current angle around the line, eased back inside the limits
        // either side of it
        cam::CVector fromLook = *VectorSub(&tmp, &self->position, outLookAt);
        fromLook.z = 0.0f;
        cam::CVector local;
        local.x = float(double(fromLook.x) * side.x + double(fromLook.y) * side.y + double(side.z) * 0.0);
        local.y = float(double(fromLook.x) * line.x + double(fromLook.y) * line.y + 0.0 * line.z);
        local.z = 0.0f;
        Vector_Normalise(&local);
        float heading = float(double(GetVectorHeading(&local)) - kHalfPi);
        angle = WrapAngle(heading);
        float maxAngle = float(double(Bute(self, kButeMaxAngle)) * kPi / 180.0);
        float centres[2];
        centres[0] = float(double(angle) - kHalfPi);
        centres[1] = float(double(angle) - 4.71238911151886);
        for (int i = 0; i < 2; i++) {
            float diff = AngleDiff(0.0f, centres[i]);
            float absDiff = std::fabs(diff);
            if (maxAngle > absDiff) {
                float limit = maxAngle;
                if (0.0f > diff) {
                    limit = -maxAngle;
                }
                float over = float(double(limit) - diff);
                float step = float(double(Bute(self, kButeAngleSpeed)) / maxAngle * over);
                angle = float(double(step) + angle);
            }
        }
    }
    float orbitX = Bute(self, kButeOrbit);
    float orbitY = Bute(self, kButeOrbit + 4);
    float orbitZ = Bute(self, kButeOrbit + 8);
    float sinK = float(std::sin(double(angle)));
    sinK = float(double(sinK) * stretch);
    float alongLine = float(double(sinK) * orbitX);
    float cosK = float(std::cos(double(angle)));
    cosK = float(double(cosK) * stretch);
    float alongSide = float(double(cosK) * orbitY);
    float orbitZ0 = float(0.0 * orbitZ);
    cam::CVector rise = {orbitZ0, orbitZ0, orbitZ};
    cam::CVector sideOffset = Scaled(side, alongSide);
    cam::CVector lineOffset = Scaled(line, alongLine);
    cam::CVector tmp2, tmp3;
    const cam::CVector* p = VectorAdd(&tmp, &middle, &lineOffset);
    p = VectorAdd(&tmp2, p, &sideOffset);
    *outPosition = *VectorAdd(&tmp3, p, &rise);
    outPosition->z = float(double(self->height) + outPosition->z);
}
FIGHTCAM_REVERSED("FightCamCtrl::ComputeView", 0x004F90E0, RE_BODY(&FightCamCtrl_ComputeView), nullptr, nullptr, re::Mode::Original);

// 0x4F9800 (thiscall): the height offset: up with the distance to the target past a
// start, capped, eased (faster up than down, or as the set says)
void __fastcall FightCamCtrl_UpdateHeight(FightCamCtrl* self, re::Edx) {
    void* player = playerPed;
    if (!player) {
        return;
    }
    cam::CVector target = {0.0f, 0.0f, 0.0f};
    cam::CVector tmp;
    if (secondTarget) {
        target = *EntityPosition(secondTarget);
    } else if (PlayerTarget(player)) {
        target = *EntityPosition(PlayerTarget(playerPed));
    } else {
        cam::CVector forward;
        cam::CVector ahead = *Entity_GetForward(player, &forward);
        target = *VectorAdd(&tmp, EntityPosition(player), &ahead);
    }
    float distance = Length(*VectorSub(&tmp, &target, EntityPosition(playerPed)));
    float past = float(double(distance) - Bute(self, kButeHeightStart));
    float wanted = 0.0f;
    if (past > 0.0f) {
        wanted = float(double(past) * past * Bute(self, kButeHeightScale));
    }
    if (Bute(self, kButeHeightMax) < wanted) {
        wanted = Bute(self, kButeHeightMax);
    }
    float height = self->height;
    double rate = height < wanted ? Bute(self, kButeHeightUpRate) : Bute(self, kButeHeightDownRate);
    self->height = float((double(wanted) - height) * rate + height);
    if (Bute(self, kButeHeightMax) < self->height) {
        self->height = Bute(self, kButeHeightMax);
    }
}
FIGHTCAM_REVERSED("FightCamCtrl::UpdateHeight", 0x004F9800, RE_BODY(&FightCamCtrl_UpdateHeight), nullptr, nullptr, re::Mode::Original);

// 0x4F9A80 (vtable[8]): an attribute set by name (0: back to the per-area ones); 0 when
// there's no such set
uint8_t __fastcall FightCamCtrl_SetButes(FightCamCtrl* self, re::Edx, int name) {
    if (name == 0) {
        self->butes = nullptr;
    } else {
        uint32_t parts[2];
        CopyInt(&parts[0], butesFightCam);
        Name_Construct(&parts[1], name);
        ButesPath path;
        ButesPath_Construct(&path, 2, parts);
        self->butes = Butes_Find(&path, butesRoot);
        if (!self->butes) {
            self->butes = nullptr;
            self->butesSet = 0;
            FightCam_ReloadButes(self);
            DestructArray(parts, 4, 2, reinterpret_cast<void*>(0x0049A650));
            return 0;
        }
        DestructArray(parts, 4, 2, reinterpret_cast<void*>(0x0049A650));
    }
    self->butesSet = self->butes != nullptr ? 1 : 0;
    if (!self->butesSet) {
        self->buteSet = -1;
    }
    FightCam_ReloadButes(self);
    return 1;
}
FIGHTCAM_REVERSED("FightCamCtrl::SetButes", 0x004F9A80, RE_BODY(&FightCamCtrl_SetButes), nullptr, nullptr, re::Mode::Original);

// 0x4F9B40 (vtable[3])
void __fastcall FightCamCtrl_Reset(FightCamCtrl* self, re::Edx) {
    self->buteSet = -1;
    FightCam_ReloadButes(self);
    self->height = 0.0f;
    FightCam_UpdateHeight(self);
    FightCam_ComputeView(self, &self->position, &self->lookAt);
    self->velocity = {0.0f, 0.0f, 0.0f};
    self->lookVelocity = {0.0f, 0.0f, 0.0f};
    self->lockTime = 0;
    self->locked = 0;
    self->far = 0;
}
FIGHTCAM_REVERSED("FightCamCtrl::Reset", 0x004F9B40, RE_BODY(&FightCamCtrl_Reset), nullptr, nullptr, re::Mode::Original);

// 0x4F9BD0 (thiscall, the two vectors by value): springs towards where the camera
// wants to be and look, then out of the world: a capsule from the look-at point to the
// new position, its hits pushing the position out (and the velocity off the surfaces)
void __fastcall FightCamCtrl_Follow(FightCamCtrl* self, re::Edx, cam::CVector wantPosition, cam::CVector wantLookAt) {
    float step = float(double(springScale) * timeStep);
    float boost = (secondTarget != nullptr || PlayerTarget(playerPed) != nullptr) ? 1.0f : 1.3f;
    cam::CVector tmp;
    float posSpring = Bute(self, kButePosSpring);
    const cam::CVector* d = VectorSub(&tmp, &wantPosition, &self->position);
    cam::CVector posPull = {float(double(d->x) * posSpring), float(double(d->y) * posSpring), float(double(posSpring) * d->z)};
    posPull = Scaled(posPull, boost);
    float lookSpring = Bute(self, kButeLookSpring);
    d = VectorSub(&tmp, &wantLookAt, &self->lookAt);
    cam::CVector lookPull = {float(double(lookSpring) * d->x), float(double(d->y) * lookSpring), float(double(d->z) * lookSpring)};
    lookPull = Scaled(lookPull, boost);
    posPull = Scaled(posPull, step);
    float maxStep = float(double(Bute(self, kButeRadius)) * maxStepScale);
    float pull = Length(posPull);
    if (maxStep < pull) {
        posPull = Scaled(posPull, float(double(maxStep) / pull));
    }
    self->velocity.x = float(double(self->velocity.x) + posPull.x);
    self->velocity.y = float(double(posPull.y) + self->velocity.y);
    self->velocity.z = float(double(self->velocity.z) + posPull.z);
    lookPull = Scaled(lookPull, step);
    self->lookVelocity.x = float(double(lookPull.x) + self->lookVelocity.x);
    self->lookVelocity.y = float(double(lookPull.y) + self->lookVelocity.y);
    self->lookVelocity.z = float(double(self->lookVelocity.z) + lookPull.z);
    self->velocity = Scaled(self->velocity, Bute(self, kButePosDamping));
    self->lookVelocity = Scaled(self->lookVelocity, Bute(self, kButeLookDamping));
    cam::CVector posMove = Scaled(self->velocity, Bute(self, kButePosVelScale));
    cam::CVector newPosition = *VectorAdd(&tmp, &self->position, &posMove);
    cam::CVector lookMove = Scaled(self->lookVelocity, Bute(self, kButeLookVelScale));
    self->lookAt = *VectorAdd(&tmp, &self->lookAt, &lookMove);
    float radius = Bute(self, kButeRadius);

    CameraCapsule capsule;
    std::memset(&capsule, 0, 0x30);
    CameraCapsule_Set(&capsule, &self->lookAt, &newPosition, radius);
    uint8_t query[0x64];
    cameraQueryActive = 1;
    CollisionQuery_Construct(query, &capsule, 0x89, collisionHits, 0x20);
    int count = CollisionQuery_Run(query);
    float lineHit = 0.0f;
    // (the deepest penetration and the steepest normal against the earlier hits are
    // kept but not used)
    float steepest = -1.0f;
    float deepest = 0.0f;
    for (int i = 0; i < count; i++) {
        const CollisionHit& hit = collisionHits[i];
        float depth = hit.depth;
        cam::CVector point = hit.point;
        cam::CVector normal = hit.normal;
        if (double(std::fabs(depth)) < kEpsilon) {
            // a hit along the line: how far back along it (as a share of its length)
            cam::CVector toHit = *VectorSub(&tmp, &hit.point, &newPosition);
            float along = float(double(capsule.direction.x) * toHit.x + double(toHit.y) * capsule.direction.y +
                                double(toHit.z) * capsule.direction.z);
            float share = float(-double(along) / capsule.length);
            if (lineHit < share) {
                lineHit = share;
            }
            continue;
        }
        float push = float(double(capsule.radius) - depth);
        cam::CVector unused;
        VectorSub(&unused, &point, &capsule.a);
        cam::CVector out = {float(double(push) * normal.x), float(double(normal.y) * push), float(double(normal.z) * push)};
        newPosition.x = float(double(out.x) + newPosition.x);
        newPosition.y = float(double(newPosition.y) + out.y);
        newPosition.z = float(double(out.z) + newPosition.z);
        float into = float(double(self->velocity.y) * normal.y + double(self->velocity.x) * normal.x +
                           double(self->velocity.z) * normal.z);
        if (into < 0.0f) {
            float back = -into;
            cam::CVector bounce = {float(double(back) * normal.x), float(double(normal.y) * back), float(double(back) * normal.z)};
            self->velocity.x = float(double(bounce.x) + self->velocity.x);
            self->velocity.y = float(double(self->velocity.y) + bounce.y);
            self->velocity.z = float(double(bounce.z) + self->velocity.z);
        }
        for (int j = 0; j < i; j++) {
            const CollisionHit& other = collisionHits[j];
            if (double(std::fabs(other.depth)) < kEpsilon) {
                continue;
            }
            float facing = float(double(other.normal.y) * normal.y + double(other.normal.x) * normal.x +
                                 double(other.normal.z) * normal.z);
            facing = -facing;
            if (steepest < facing) {
                steepest = facing;
            }
        }
        if (!(deepest > push)) {
            deepest = push;
        }
        if (normal.z > -0.75f) {
            // not a ceiling: the height follows the wanted one, and the ground pushes up
            float rise = float(double(wantPosition.z) - newPosition.z);
            newPosition.z = float(double(newPosition.z) + (double(push) * Bute(self, kButeGroundPush) +
                                                           double(rise) * Bute(self, kButeHeightFollow)));
        }
    }
    (void)steepest;
    (void)deepest;
    cameraQueryActive = 0;
    if (!(lineHit == 0.0f)) {
        // in front of the nearest hit along the line, by half the radius
        float back = float(-double(lineHit) * capsule.length + double(capsule.radius) * 0.5);
        cam::CVector move = Scaled(capsule.direction, back);
        newPosition.x = float(double(move.x) + newPosition.x);
        newPosition.y = float(double(move.y) + newPosition.y);
        newPosition.z = float(double(move.z) + newPosition.z);
    }
    float blend = Bute(self, kButePosBlend);
    cam::CVector newShare = Scaled(newPosition, blend);
    float keep = float(1.0 - Bute(self, kButePosBlend));
    cam::CVector oldShare = Scaled(self->position, keep);
    self->position = *VectorAdd(&tmp, &oldShare, &newShare);
    EmptyThiscall(query);
}
FIGHTCAM_REVERSED("FightCamCtrl::Follow", 0x004F9BD0, RE_BODY(&FightCamCtrl_Follow), nullptr, nullptr, re::Mode::Original);

// 0x4FA580 (vtable[7], process)
void __fastcall FightCamCtrl_Process(FightCamCtrl* self, re::Edx) {
    FightCam_ReloadButes(self);
    FightCam_UpdateHeight(self);
    FightCam_UpdateLock(self);
    FightCam_UpdateRange(self);
    cam::CVector position = {0.0f, 0.0f, 0.0f};
    cam::CVector lookAt = {0.0f, 0.0f, 0.0f};
    FightCam_ComputeView(self, &position, &lookAt);
    FightCam_Follow(self, position, lookAt);
    Camera_SetView4F2C60(&TheCamera, &self->position, &self->lookAt);
    Camera_SetVelocity(&TheCamera, &self->velocity);
    At<cam::CVector>(&TheCamera, 0x18C) = self->lookVelocity;
    TheCamera.unknown198 = self->lookAt;
    Camera_SetNearClip(TheCamera.camera, 0.25f);
    float current = TheCamera.fovOverridden == 0 ? TheCamera.fov : TheCamera.fovOverride;
    if (fovLocked == 0) {
        float blend = Bute(self, kButeFovBlend);
        TheCamera.fovOverridden = 0;
        TheCamera.fov = float(double(Bute(self, kButeFov)) * blend + (1.0 - blend) * current);
    }
}
FIGHTCAM_REVERSED("FightCamCtrl::Process", 0x004FA580, RE_BODY(&FightCamCtrl_Process), nullptr, nullptr, re::Mode::Original);
