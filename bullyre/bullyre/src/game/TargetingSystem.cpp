// CTargetingSystem (0xE0 bytes, in its own pool; one per ped at CPed+0x1C8): who a ped
// aims at. It keeps a list of candidates (0x1C bytes each: the entity, when its line
// of sight and screen visibility were last checked and found, what kind of target it
// is and a score) and picks the best visible one; the player's lock-on button feeds
// it. 0x4D58E0..0x4D6F10 (the rest is TargetingSystem2.cpp and Weapon.cpp).
//
// Entities are reached by raw offsets (peds are type 3 at +0x108, the model at +0x10E);
// references are the game's registered ones (CEntity::RegisterReference and
// CleanUpOldReference).

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

struct CEntity;

namespace targeting_c12b {

using vc::At;

struct CVector {
    float x, y, z;
};
static_assert(sizeof(CVector) == 0xC);

// GTA's CColLine: two points, each padded to 16 bytes
struct CColLine {
    CVector start;       // +0
    float unknownC;      // +0xC
    CVector end;         // +0x10
    float unknown1C;     // +0x1C
};
static_assert(sizeof(CColLine) == 0x20);

// GTA's CColPoint as built here (the constructor 0x415FA0 inlined)
struct CColPoint {
    CVector point;       // +0
    re::Ptr32<CEntity> entity;     // +0xC a registered reference (released by the caller)
    CVector normal;      // +0x10
    uint8_t unknown1C[8];
    float depth;         // +0x24
};
static_assert(sizeof(CColPoint) == 0x28);

// one entity the system considers
struct TargetCandidate {
    re::Ptr32<CEntity> entity;          // +0 registered reference
    uint32_t lastChecked;     // +4 when the line of sight was last tested
    uint32_t lastVisible;     // +8 when it was last clear
    uint32_t lastOnScreen;    // +0xC when it was last on screen
    int32_t kind;             // +0x10 ClassifyTarget's
    float score;              // +0x14 ScoreTarget's (-1e11: none)
    uint32_t flags;           // +0x18 bit 0 visible, bit 1 on screen, bit 2 (copied only)
};
static_assert(sizeof(TargetCandidate) == 0x1C);

struct CandidateList {
    re::Ptr32<TargetCandidate> data;    // +0
    uint32_t unknown4;        // +4 (the capacity)
    uint32_t count;           // +8
};
static_assert(sizeof(CandidateList) == 0xC);

struct CTargetingSystem {
    re::Ptr32<re::Ptr32<void>> vtable;               // +0 (0x4D83F0 the destructor)
    re::Ptr32<uint8_t> owner;              // +4 the CPed
    int32_t state;               // +8 2: a target set
    uint32_t unknownC;           // +0xC
    CVector eye;                 // +0x10 where the aim line starts
    CVector losStart;            // +0x1C where line of sight tests start
    CVector aimDir;              // +0x28
    float heading;               // +0x34
    float pitch;                 // +0x38
    uint32_t unknown3C;          // +0x3C
    float unknown40;             // +0x40 (-1 on reset)
    uint8_t unknown44[0xC];      // +0x44
    float distanceWeight;        // +0x50
    float angleWeight;           // +0x54
    float range;                 // +0x58
    uint8_t unknown5C[0x10];     // +0x5C
    uint8_t unknown6C;           // +0x6C (kind 8 allowed)
    uint8_t unknown6D[3];        // +0x6D
    CVector aim;                 // +0x70 SetAim's
    uint8_t useCameraWeights;    // +0x7C (SetAim's flag: the weights from the camera settings)
    uint8_t unknown7D;           // +0x7D
    uint8_t lockOn;              // +0x7E the lock-on button held
    uint8_t unknown7F;           // +0x7F
    float lockTime;              // +0x80
    uint8_t unknown84;           // +0x84
    uint8_t unknown85;           // +0x85
    uint8_t unknown86[2];        // +0x86
    float rangeScale;            // +0x88 for the weapon model below
    int32_t rangeScaleModel;     // +0x8C (-1: none)
    CandidateList candidates;    // +0x90
    re::Ptr32<CEntity> unknown9C;          // +0x9C registered references from here to +0xB0
    re::Ptr32<CEntity> target;             // +0xA0
    re::Ptr32<CEntity> unknownA4;          // +0xA4
    re::Ptr32<CEntity> unknownA8;          // +0xA8
    re::Ptr32<CEntity> unknownAC;          // +0xAC
    re::Ptr32<CEntity> unknownB0;          // +0xB0
    uint8_t unknownB4;           // +0xB4
    uint8_t unknownB5[3];        // +0xB5
    int32_t unknownB8;           // +0xB8
    int32_t unknownBC;           // +0xBC
    int32_t unknownC0;           // +0xC0
    int32_t unknownC4;           // +0xC4
    uint32_t aimTime;            // +0xC8 when SetAim got a real direction
    int32_t unknownCC;           // +0xCC
    int32_t unknownD0;           // +0xD0
    int32_t unknownD4;           // +0xD4
    int32_t unknownD8;           // +0xD8
    int32_t unknownDC;           // +0xDC
};
static_assert(sizeof(CTargetingSystem) == 0xE0);
static_assert(offsetof(CTargetingSystem, range) == 0x58);
static_assert(offsetof(CTargetingSystem, aim) == 0x70);
static_assert(offsetof(CTargetingSystem, lockTime) == 0x80);
static_assert(offsetof(CTargetingSystem, candidates) == 0x90);
static_assert(offsetof(CTargetingSystem, target) == 0xA0);
static_assert(offsetof(CTargetingSystem, aimTime) == 0xC8);

constexpr float kNoScore = -99999997952.0f;   // -1e11 as a float

GAME_VAR(int32_t, g_cameraMode, 0x00C3CCF8);        // 2, 0xD: aiming modes
GAME_VAR(uint8_t*, cameraSettings, 0x020C88B8);
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);
GAME_VAR(uint16_t, ms_nCurrentScanCode, 0x00C1AE88);
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);          // CWorld::Players[0].ped
GAME_VAR(uint8_t*, controlledPed, 0x00C2B124);
GAME_VAR(uint8_t, lockButtonInputsExcept, 0x00C1AF8C);
GAME_VAR(uint8_t, unknownC1AF97, 0x00C1AF97);
GAME_VAR(CEntity*, lastPlayerCandidate, 0x00C3A938);
// model indices resolved by name at load
GAME_VAR(int32_t, modelBF40EC, 0x00BF40EC);
GAME_VAR(int32_t, modelBF40F0, 0x00BF40F0);
GAME_VAR(int32_t, modelBF40F4, 0x00BF40F4);
GAME_VAR(int32_t, modelBF4184, 0x00BF4184);
GAME_VAR(int32_t, modelBF42DC, 0x00BF42DC);
GAME_VAR(int32_t, modelBF433C, 0x00BF433C);
GAME_VAR(int32_t, modelBF43F0, 0x00BF43F0);
GAME_VAR(int32_t, modelBF4414, 0x00BF4414);
GAME_VAR(int32_t, modelBF4430, 0x00BF4430);
inline void* const TheCamera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const userInputManager = reinterpret_cast<void*>(0x00A45A30);

GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, CEntity* entity, re::Ptr32<CEntity>* ref);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, CEntity* entity, re::Ptr32<CEntity>* ref);
GAME_FN(float, __cdecl, AngleDiff, 0x00432980, float a, float b);
GAME_FN(uint8_t*, __cdecl, WeaponInfo_Get, 0x0051B450, int model);
GAME_FN(uint8_t, __thiscall, Ped_CanAttack, 0x004971D0, uint8_t* ped, CEntity* target, int kind, int checkRange, int notIfDown);
GAME_FN(uint8_t, __thiscall, Ped_IsSeatedInVehicle, 0x004CE690, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Ped_473FB0, 0x00473FB0, CEntity* ped);
GAME_FN(uint8_t*, __thiscall, Ped_46ECE0, 0x0046ECE0, CEntity* ped);   // (its attack target)
GAME_FN(uint8_t, __thiscall, Ped_41AA50, 0x0041AA50, CEntity* ped);
GAME_FN(uint8_t, __thiscall, Ped_496F30, 0x00496F30, uint8_t* ped, CEntity* other);
GAME_FN(uint8_t, __thiscall, Fn_485590, 0x00485590, uint8_t* object2F0, CEntity* other);
GAME_FN(uint8_t*, __thiscall, Ped_GetModelInfo, 0x00477DC0, CEntity* ped);   // +0x50 female
GAME_FN(uint8_t, __thiscall, Ped_44E400, 0x0044E400, CEntity* ped);
GAME_FN(float, __thiscall, Camera_GetZoom, 0x004F1250, void* camera);
GAME_FN(int, __thiscall, Camera_IsSphereVisible, 0x004F13D0, void* camera, const CVector* center, float radius);
GAME_FN(uint8_t, __thiscall, UserInput_50DF50, 0x0050DF50, void* input);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(uint8_t, __thiscall, Ped_GetColSphere, 0x00474630, CEntity* ped, int index, CVector* center, float* radius);
GAME_FN(float, __thiscall, ColLine_DistanceToPoint, 0x005E7250, const CColLine* line, const CVector* point);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorScale, 0x00412350, CVector* out, const CVector* v, float s);
GAME_FN(float, __cdecl, VecHeading, 0x00433190, const CVector* v);
GAME_FN(float, __cdecl, GetATanOfXY, 0x00432FE0, float x, float y);
GAME_FN(void*, __thiscall, EntityRef_Set, 0x00478550, void* ref, CEntity* entity);
GAME_FN(float, __cdecl, RandomFloatInRange, 0x0041A5B0, float lo, float hi);
GAME_FN(uint8_t, __cdecl, ProcessLineOfSight, 0x00461EA0, const CVector* start, const CVector* end, CColPoint* colPoint,
        re::Ptr32<CEntity>* hit, int flags, int, int);
GAME_FN(float*, __thiscall, Placeable_GetMatrix, 0x0046DD20, CEntity* entity);
// (these files', called where modes apply)
GAME_FN(void, __thiscall, ColLine_Set, 0x004D5A60, CColLine* line, float sx, float sy, float sz, float ex, float ey, float ez);
GAME_FN(TargetCandidate*, __thiscall, Candidate_Copy, 0x004D5D20, TargetCandidate* self, const TargetCandidate* other);
GAME_FN(TargetCandidate*, __thiscall, Candidate_Construct, 0x004D63A0, TargetCandidate* self, CEntity* entity);
GAME_FN(TargetCandidate*, __thiscall, Candidate_Assign, 0x004D6400, TargetCandidate* self, const TargetCandidate* other);
GAME_FN(uint8_t, __thiscall, TS_WeaponHasFlag68, 0x004D59E0, CTargetingSystem* self);
GAME_FN(float, __thiscall, TS_GetLockProgress, 0x004D5E20, CTargetingSystem* self);
GAME_FN(void, __thiscall, TS_SortCandidates, 0x004D68A0, CTargetingSystem* self, int lo, int hi);
GAME_FN(void, __thiscall, TS_UpdateVisibility, 0x004D6A00, CTargetingSystem* self, TargetCandidate* candidate, float spread);
GAME_FN(uint8_t, __thiscall, TS_IsCandidateVisible, 0x004D6DA0, CTargetingSystem* self, TargetCandidate* candidate, int onScreenToo);

uint8_t EntityType(const void* entity) {
    return At<uint8_t>(const_cast<void*>(entity), 0x108) & 7;
}
int16_t ModelIndex(const void* entity) {
    return At<int16_t>(const_cast<void*>(entity), 0x10E);
}
bool IsPlayer(const uint8_t* ped) {
    return At<int32_t>(const_cast<uint8_t*>(ped), 0x1310) == 13;
}
// the model of the object in the ped's hand (-1: none)
int WeaponModel(const uint8_t* ped) {
    return ModelIndex(At<void*>(const_cast<uint8_t*>(ped), 0x1D0));
}
// CPlaceable's position: the matrix's when it has one
const CVector* Position(CEntity* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<const CVector*>(matrix + 0x30) : &At<CVector>(entity, 4);
}
float EntityHeight(CEntity* entity) {
    return vc::Call<float>(entity, 0x6C);
}
float EntityWidth(CEntity* entity) {
    return vc::Call<float>(entity, 0x70);
}
// (the inlined candidate search: the index or -1)
uint32_t FindIndex(const CTargetingSystem* self, const CEntity* entity) {
    for (uint32_t i = 0; i < self->candidates.count; i++) {
        if (self->candidates.data[i].entity == entity) {
            return i;
        }
    }
    return 0xFFFFFFFF;
}
void ReleaseRef(re::Ptr32<CEntity>* ref) {
    if (*ref) {
        CleanUpOldReference(*ref, ref);
        *ref = nullptr;
    }
}

// 0x4D58E0: a candidate's score (higher is better): out of range or outside the heading
// and pitch cone, -1e11
float __fastcall ScoreTarget(CTargetingSystem* self, re::Edx, float range, float maxHeading, float maxPitch, float distance,
                             float heading, float pitch) {
    if (range < distance) {
        return kNoScore;
    }
    float headingOff = std::fabs(AngleDiff(self->heading, heading));
    float headingRatio = headingOff / maxHeading;
    float pitchOff = std::fabs(AngleDiff(self->pitch, pitch));
    float pitchRatio = pitchOff / maxPitch;
    if (headingRatio > 1.0f || pitchRatio > 1.0f) {
        return kNoScore;
    }
    float distanceWeight = self->useCameraWeights ? At<float>(cameraSettings, 0x3C) : self->distanceWeight;
    float angleWeight = self->useCameraWeights ? At<float>(cameraSettings, 0x40) : self->angleWeight;
    float byDistance = -distance * distanceWeight;
    return byDistance - headingRatio * angleWeight * 5.0f;
}

// 0x4D59E0: in an aiming camera mode that ignores targets: mode 2, or mode 0xD with a
// weapon whose info has +0x68 set
uint8_t __fastcall WeaponHasFlag68(CTargetingSystem* self) {
    int mode = g_cameraMode;
    if (mode != 0xD) {
        return mode == 2;
    }
    int model = WeaponModel(self->owner);
    if (model != -1 && WeaponInfo_Get(model) && WeaponInfo_Get(model)[0x68]) {
        return 1;
    }
    return 0;
}

// 0x4D5A40: the entity if it's an object (type 4)
CEntity* __cdecl AsObject(CEntity* entity) {
    if (!entity) {
        return nullptr;
    }
    return EntityType(entity) == 4 ? entity : nullptr;
}

// 0x4D5A60: CColLine from two points; returns this (the pads get whatever is on the
// game's stack there: left alone here)
CColLine* __fastcall ColLineSet(CColLine* self, re::Edx, float sx, float sy, float sz, float ex, float ey, float ez) {
    self->start = {sx, sy, sz};
    self->end = {ex, ey, ez};
    return self;
}

// 0x4D5AE0: what kind of target an entity is for the owner (0: none). Peds: 1/3
// attacking the owner (3 if also special), 2 plain, 4/5 special with the lock-on held,
// 8 for +0x1C98 peds; 6 from a vehicle; 7 objects that allow it
int __fastcall ClassifyTarget(CTargetingSystem* self, re::Edx, CEntity* target) {
    uint8_t* owner = self->owner;
    if (!Ped_CanAttack(owner, target, 0, 0, 0)) {
        return 0;
    }
    if (At<int32_t>(target, 0xC0) != 0) {
        return 0;
    }
    int weapon = WeaponModel(self->owner);
    if (weapon != modelBF43F0 && ModelIndex(target) == modelBF40EC) {
        return 0;
    }
    if (weapon == modelBF4414 && Ped_IsSeatedInVehicle(self->owner)) {
        uint8_t* vehicle = At<uint8_t*>(self->owner, 0x1554);
        if (vehicle && At<int32_t>(vehicle, 0x46C) == 1) {
            if (ModelIndex(target) == modelBF4184) {
                return 6;
            }
            return self->lockOn ? 4 : 0;
        }
    }
    uint8_t type = EntityType(target);
    if (type != 3) {
        if (type != 6 && !vc::Call<uint8_t>(target, 0x7C)) {
            return 0;
        }
        return vc::Call<uint8_t>(target, 0x78) ? 0 : 7;
    }
    if (At<uint8_t>(target, 0x1C98)) {
        return self->unknown6C ? 8 : 0;
    }
    if (weapon == modelBF4430 || weapon == modelBF433C) {
        return 2;
    }
    bool attackingOwner = Ped_473FB0(target) && Ped_46ECE0(target) == self->owner;
    uint8_t gang = Ped_41AA50(target);
    bool has1424 = At<int32_t>(target, 0x1424) != 0;
    uint8_t a = Ped_496F30(self->owner, target);
    uint8_t b = Fn_485590(self->owner + 0x2F0, target);
    uint8_t female = Ped_GetModelInfo(target)[0x50];
    uint8_t c = Ped_44E400(target);
    int32_t pedType = At<int32_t>(target, 0x1310);
    bool special = a || b || female || c || pedType == 9 || pedType == 10;
    if (attackingOwner) {
        return (!gang && !has1424 && !special) ? 1 : 3;
    }
    if (!self->lockOn) {
        if (gang || has1424 || special) {
            return 0;
        }
        return 2;
    }
    if (has1424) {
        return 4;
    }
    if (gang || special) {
        return 5;
    }
    return 2;
}

// 0x4D5D20: TargetCandidate's copy constructor (bits 3.. of the flags stay as they were)
TargetCandidate* __fastcall CandidateCopy(TargetCandidate* self, re::Edx, const TargetCandidate* other) {
    self->entity = other->entity;
    if (other->entity) {
        RegisterReference(other->entity, &self->entity);
    }
    self->lastChecked = other->lastChecked;
    self->lastVisible = other->lastVisible;
    self->lastOnScreen = other->lastOnScreen;
    self->kind = other->kind;
    self->score = other->score;
    self->flags = (self->flags & ~7u) | (other->flags & 7u);
    return self;
}

// 0x4D5D90: the range scale: the camera's zoom for the player, times rangeScale with
// its weapon in hand
float __fastcall GetRangeScale(CTargetingSystem* self) {
    float scale = IsPlayer(self->owner) ? Camera_GetZoom(TheCamera) : 1.0f;
    if (self->rangeScaleModel != -1 && WeaponModel(self->owner) == self->rangeScaleModel) {
        scale = self->rangeScale * scale;
    }
    return scale;
}

// 0x4D5DF0: the camera's zoom for the player, else 1
float __fastcall GetZoom(CTargetingSystem* self) {
    if (IsPlayer(self->owner)) {
        return Camera_GetZoom(TheCamera);
    }
    return 1.0f;
}

// 0x4D5E20: how far the lock-on has come, 0..1 (the lock time over the settings' +0x108
// seconds; the player's +0x1EC4 added)
float __fastcall GetLockProgress(CTargetingSystem* self) {
    float full = At<float>(cameraSettings, 0x108) * 1000.0f;
    float progress = self->lockTime;
    if (progress > full) {
        progress = full;
    }
    progress = progress / full;
    if (IsPlayer(self->owner)) {
        progress = At<float>(self->owner, 0x1EC4) + progress;
    }
    if (progress < 1.0f) {
        return progress;
    }
    return 1.0f;
}

// 0x4D5EA0: the lock-on button (control 10) into lockOn
void __fastcall UpdateLockButton(CTargetingSystem* self) {
    uint8_t held;
    if (UserInput_50DF50(userInputManager) && (!lockButtonInputsExcept || unknownC1AF97) &&
        (Pad_GetValue(10, 0) != 0.0f || At<int32_t>(self->owner, 0x1408) != 0)) {
        held = 1;
    } else {
        held = 0;
    }
    if (controlledPed == self->owner) {
        held = (Pad_GetValue(10, 1) != 0.0f || At<int32_t>(self->owner, 0x1408) != 0) ? 1 : 0;
    }
    uint8_t* owner = self->owner;
    if (IsPlayer(owner) && At<uint8_t>(owner, 0x1E91) && At<int32_t>(owner, 0x13DC) == 0) {
        held = 0;
    }
    if (TS_WeaponHasFlag68(self)) {
        self->lockOn = 0;
        return;
    }
    self->lockOn = held;
}

// 0x4D5F60
CEntity* __fastcall GetUnknownA8(CTargetingSystem* self) {
    return self->unknownA8;
}

// 0x4D5F70: the aim direction; aimTime is when it was last longer than 0.1
void __fastcall SetAim(CTargetingSystem* self, re::Edx, float x, float y, float z, int useCameraWeights) {
    self->useCameraWeights = static_cast<uint8_t>(useCameraWeights);
    self->aim = {x, y, z};
    float lengthSq = self->aim.x * self->aim.x + self->aim.y * self->aim.y + self->aim.z * self->aim.z;
    float length = std::sqrt(std::fabs(lengthSq));
    self->aimTime = length > 0.1f ? timeInMs : 0;
}

// 0x4D6000: the point to aim at on an entity. Not a ped: its position. The player in an
// aiming camera: the collision sphere closest to the aim line. Else sphere 2 (`head`),
// or sphere 1; with the lock-on on the current target, sphere 1 lerped towards sphere 1
// by the lock progress (the same sphere twice: no change). A null entity crashes here
// as in the game.
CVector* __fastcall GetTargetPos(CTargetingSystem* self, re::Edx, CVector* out, CEntity* entity, int head) {
    CEntity* ped = entity && EntityType(entity) == 3 ? entity : nullptr;
    *out = {0.0f, 0.0f, 0.0f};
    float radius;   // (the game uses the `head` argument's slot)
    if (!ped) {
        *out = *Position(entity);
        return out;
    }
    if (IsPlayer(self->owner) && (g_cameraMode == 2 || g_cameraMode == 0xD)) {
        float range = self->range;
        CVector along{range * self->aimDir.x, self->aimDir.y * range, range * self->aimDir.z};
        CVector end;
        VectorAdd(&end, &self->eye, &along);
        CColLine line;
        ColLine_Set(&line, self->eye.x, self->eye.y, self->eye.z, end.x, end.y, end.z);
        float best = 99999997952.0f;
        int bestSphere = 0;
        CVector center{0.0f, 0.0f, 0.0f};
        for (int sphere = 0; sphere < 6; sphere++) {
            if (Ped_GetColSphere(ped, sphere, &center, &radius)) {
                CVector point = center;
                float distance = ColLine_DistanceToPoint(&line, &point);
                if (best > distance) {
                    best = distance;
                    bestSphere = sphere;
                }
            }
        }
        Ped_GetColSphere(ped, bestSphere, out, &radius);
        return out;
    }
    if (ModelIndex(ped) == modelBF42DC) {
        *out = *Position(entity);
        return out;
    }
    if (static_cast<uint8_t>(head)) {
        Ped_GetColSphere(ped, 2, out, &radius);
        return out;
    }
    if (self->lockOn && entity == self->target) {
        CVector from{0.0f, 0.0f, 0.0f};
        CVector to{0.0f, 0.0f, 0.0f};
        Ped_GetColSphere(ped, 1, &from, &radius);
        Ped_GetColSphere(ped, 1, &to, &radius);
        float t = TS_GetLockProgress(self);
        CVector delta, scaled, result;
        VectorSub(&delta, &to, &from);
        VectorScale(&scaled, &delta, t);
        VectorAdd(&result, &from, &scaled);
        *out = result;
        return out;
    }
    Ped_GetColSphere(ped, 1, out, &radius);
    return out;
}

// 0x4D62C0
CEntity* __fastcall GetUnknown9C(CTargetingSystem* self) {
    return self->unknown9C;
}

// 0x4D62D0: the index of the candidate for the same entity, or -1
uint32_t __fastcall CandidateListFind(CandidateList* self, re::Edx, const TargetCandidate* candidate) {
    for (uint32_t i = 0; i < self->count; i++) {
        if (self->data[i].entity == candidate->entity) {
            return i;
        }
    }
    return 0xFFFFFFFF;
}

// 0x4D63A0: TargetCandidate's constructor
TargetCandidate* __fastcall CandidateConstruct(TargetCandidate* self, re::Edx, CEntity* entity) {
    self->entity = nullptr;
    if (entity) {
        self->entity = entity;
        RegisterReference(entity, &self->entity);
    }
    self->flags &= ~3u;
    self->kind = 0;
    self->score = kNoScore;
    self->lastChecked = 0;
    self->lastVisible = 0;
    self->lastOnScreen = 0;
    return self;
}

// 0x4D63E0: TargetCandidate's destructor
void __fastcall CandidateDestruct(TargetCandidate* self) {
    ReleaseRef(&self->entity);
}

// 0x4D6400: TargetCandidate's assignment
TargetCandidate* __fastcall CandidateAssign(TargetCandidate* self, re::Edx, const TargetCandidate* other) {
    CEntity* entity = other->entity;
    if (entity != self->entity) {
        if (self->entity) {
            CleanUpOldReference(self->entity, &self->entity);
        }
        self->entity = entity;
        if (entity) {
            RegisterReference(entity, &self->entity);
        }
    }
    self->lastChecked = other->lastChecked;
    self->lastVisible = other->lastVisible;
    self->lastOnScreen = other->lastOnScreen;
    self->kind = other->kind;
    self->score = other->score;
    self->flags = (self->flags & ~7u) | (other->flags & 7u);
    return self;
}

// 0x4D6480: a direction's heading and pitch
void __cdecl HeadingAndPitch(float x, float y, float z, float* heading, float* pitch) {
    CVector v{x, y, z};
    *heading = VecHeading(&v);
    float horizontal = std::sqrt(std::fabs(x * x + y * y));
    *pitch = GetATanOfXY(horizontal, z);
}

// 0x4D64F0: back to no target
void __fastcall Reset(CTargetingSystem* self) {
    self->state = 0;
    self->losStart = {0.0f, 0.0f, 0.0f};
    self->aimDir = {0.0f, 0.0f, 0.0f};
    self->unknown7D = 0;
    self->aim = {0.0f, 0.0f, 0.0f};
    self->unknown40 = -1.0f;
    self->lockOn = 0;
    self->unknownC4 = 0;
    self->unknownBC = 0;
    self->aimTime = 0;
    self->unknownC0 = 0;
    ReleaseRef(&self->target);
    ReleaseRef(&self->unknownA4);
    ReleaseRef(&self->unknownA8);
    ReleaseRef(&self->unknownAC);
    ReleaseRef(&self->unknownB0);
    self->unknownB4 = 0;
    self->unknownB8 = 0;
    self->unknown84 = 0;
    self->unknown85 = 1;
    ReleaseRef(&self->unknown9C);
    self->rangeScale = 1.0f;
    self->unknownCC = 0;
    self->unknownD4 = 0;
    self->unknownD8 = 0;
    self->lockTime = 0.0f;
    self->rangeScaleModel = -1;
}

// 0x4D6660: the target (the lock time restarts); true when it is one of the candidates
// (or none)
uint8_t __fastcall SetTargetEntity(CTargetingSystem* self, re::Edx, CEntity* entity) {
    if (self->target == entity) {
        return 1;
    }
    self->lockTime = 0.0f;
    if (entity != self->target) {
        if (self->target) {
            CleanUpOldReference(self->target, &self->target);
        }
        self->target = entity;
        if (!entity) {
            return 1;
        }
        RegisterReference(entity, &self->target);
    }
    if (!entity) {
        return 1;
    }
    TargetCandidate probe;
    Candidate_Construct(&probe, entity);
    uint32_t index = FindIndex(self, probe.entity);
    if (probe.entity) {
        CleanUpOldReference(probe.entity, &probe.entity);
    }
    if (index != 0xFFFFFFFF && EntityType(entity) == 3) {
        self->unknown85 = At<uint8_t>(entity, 0x1C98) == 0;
    }
    if (IsPlayer(self->owner) && index != 0xFFFFFFFF && self->lockOn) {
        EntityRef_Set(&self->unknownAC, entity);
    }
    return index != 0xFFFFFFFF;
}

// 0x4D6770: whether the target is a candidate with a score
uint8_t __fastcall IsTargetScored(CTargetingSystem* self) {
    if (!self->target) {
        return 0;
    }
    // (a candidate built inline)
    re::Ptr32<CEntity> probe = self->target;
    RegisterReference(probe, &probe);
    uint32_t index = FindIndex(self, probe);
    if (probe) {
        CleanUpOldReference(probe, &probe);
        probe = nullptr;
    }
    if (index == 0xFFFFFFFF) {
        return 0;
    }
    return self->candidates.data[index].score != kNoScore;
}

// 0x4D6860: no target
void __fastcall ClearTarget(CTargetingSystem* self) {
    if (self->target) {
        self->lockTime = 0.0f;
        ReleaseRef(&self->target);
    }
    self->state = 0;
}

// 0x4D68A0: quicksort of the candidates lo..hi by score, best first
void __fastcall SortCandidates(CTargetingSystem* self, re::Edx, int lo, int hi) {
    while (lo < hi) {
        float pivot = self->candidates.data[(lo + hi) / 2].score;
        int i = lo;
        int j = hi;
        do {
            while (self->candidates.data[i].score > pivot) {
                i++;
            }
            while (self->candidates.data[j].score < pivot) {
                j--;
            }
            if (i > j) {
                break;
            }
            TargetCandidate swap;
            Candidate_Copy(&swap, &self->candidates.data[i]);
            Candidate_Assign(&self->candidates.data[i], &self->candidates.data[j]);
            Candidate_Assign(&self->candidates.data[j], &swap);
            i++;
            j--;
            if (swap.entity) {
                CleanUpOldReference(swap.entity, &swap.entity);
            }
        } while (i <= j);
        TS_SortCandidates(self, lo, j);
        lo = i;
    }
}

// 0x4D6A00: tests a candidate's line of sight (from losStart to its middle, both moved
// by the same random offset within `spread` of its height) and whether it's on screen;
// once per frame
void __fastcall UpdateVisibility(CTargetingSystem* self, re::Edx, TargetCandidate* candidate, float spread) {
    if (timeInMs == candidate->lastChecked || !candidate->entity) {
        return;
    }
    CEntity* entity = candidate->entity;
    CVector start = self->losStart;
    CVector end = *Position(entity);
    float height = EntityHeight(entity);
    float offsetX, offsetY, offsetZ;
    if (EntityType(candidate->entity) == 3) {
        end.z = height * 0.8f + end.z;
        float s = height * spread / 9.0f;
        offsetX = RandomFloatInRange(-s, s);
        offsetY = RandomFloatInRange(-s, s);
        float s3 = s * 3.0f;
        float neg3 = 3.0f * -s;
        offsetZ = RandomFloatInRange(neg3, s3);
    } else {
        end.z = height * 0.5f + end.z;
        float s = height * spread / 3.0f;
        offsetX = RandomFloatInRange(-s, s);
        offsetY = RandomFloatInRange(-s, s);
        offsetZ = RandomFloatInRange(-s, s);
    }
    start.x = offsetX + start.x;
    start.y = offsetY + start.y;
    start.z = offsetZ + start.z;
    end.x = offsetX + end.x;
    end.y = offsetY + end.y;
    end.z = offsetZ + end.z;

    CColPoint colPoint;
    colPoint.point = {0.0f, 0.0f, 0.0f};
    colPoint.entity = nullptr;
    colPoint.normal = {0.0f, 0.0f, 0.0f};
    colPoint.depth = 3.4028234663852886e+38f;
    re::Ptr32<CEntity> hit = nullptr;
    // (both skipped by the test)
    At<uint16_t>(candidate->entity, 0x10A) = static_cast<uint16_t>(ms_nCurrentScanCode + 1);
    At<uint16_t>(self->owner, 0x10A) = static_cast<uint16_t>(ms_nCurrentScanCode + 1);
    uint8_t blocked = ProcessLineOfSight(&start, &end, &colPoint, &hit, 0x4B, 0, 0);
    candidate->flags = (candidate->flags & ~1u) | (blocked == 0 ? 1u : 0u);
    entity = candidate->entity;
    if (ModelIndex(entity) == modelBF40F4 && !(candidate->flags & 1) && hit && ModelIndex(hit) == modelBF40F0) {
        candidate->flags |= 1;
    }
    candidate->lastChecked = timeInMs;
    if (candidate->flags & 1) {
        candidate->lastVisible = timeInMs;
    }

    height = EntityHeight(entity);
    float width = EntityWidth(candidate->entity);
    const float* matrix = Placeable_GetMatrix(candidate->entity);
    CVector center{matrix[12], matrix[13], matrix[14]};
    float halfHeight = height * 0.5f;
    center.x = center.x + 0.0f;
    center.y = 0.0f + center.y;
    center.z = halfHeight + center.z;
    float radius = 0.5f * width;
    int onScreen = Camera_IsSphereVisible(TheCamera, &center, radius);
    candidate->flags = (candidate->flags & ~2u) | ((static_cast<uint32_t>(onScreen) * 2) & 2u);
    if (candidate->flags & 2) {
        candidate->lastOnScreen = timeInMs;
    }
    if (colPoint.entity) {
        CleanUpOldReference(colPoint.entity, &colPoint.entity);
    }
}

// 0x4D6DA0: visible (now or within 300 ms; tested again after 500 ms), and on screen
// too if asked
uint8_t __fastcall IsCandidateVisible(CTargetingSystem* self, re::Edx, TargetCandidate* candidate, int onScreenToo) {
    if (timeInMs - candidate->lastChecked > 500) {
        TS_UpdateVisibility(self, candidate, 0.3f);
    }
    uint8_t visible = candidate->flags & 1;
    if (timeInMs - candidate->lastVisible < 300) {
        visible = 1;
    }
    if (!static_cast<uint8_t>(onScreenToo)) {
        return visible;
    }
    uint8_t onScreen = (candidate->flags >> 1) & 1;
    if (timeInMs - candidate->lastOnScreen < 300) {
        onScreen = 1;
    }
    return visible && onScreen ? 1 : 0;
}

// 0x4D6E20: IsCandidateVisible for an entity's candidate (false when it isn't one)
uint8_t __fastcall IsEntityVisible(CTargetingSystem* self, re::Edx, CEntity* entity, int onScreenToo) {
    if (!entity) {
        return 0;
    }
    // (a candidate built inline)
    re::Ptr32<CEntity> probe = entity;
    RegisterReference(entity, &probe);
    uint32_t index = FindIndex(self, probe);
    if (probe) {
        CleanUpOldReference(probe, &probe);
        probe = nullptr;
    }
    if (index == 0xFFFFFFFF) {
        return 0;
    }
    return TS_IsCandidateVisible(self, &self->candidates.data[index], onScreenToo);
}

// 0x4D6F10: the best scored visible candidate after sorting. With the lock-on held the
// ones at +0xAC are skipped, and after the one at +0xB0 the special kinds (4, 5, 7);
// the player with +0x1EC0 set only takes visible and on-screen ones (or the last one
// taken so)
CEntity* __fastcall PickBestCandidate(CTargetingSystem* self) {
    if (self->candidates.count == 0) {
        return nullptr;
    }
    TS_SortCandidates(self, 0, static_cast<int>(self->candidates.count - 1));
    if (self->candidates.data[0].score == kNoScore) {
        return nullptr;
    }
    bool skipSpecial = false;
    for (uint32_t i = 0; i < self->candidates.count; i++) {
        TargetCandidate* candidate = &self->candidates.data[i];
        CEntity* entity = candidate->entity;
        if (!entity) {
            continue;
        }
        int kind = candidate->kind;
        bool special = kind == 4 || kind == 5 || kind == 7;
        if (skipSpecial && special) {
            continue;
        }
        if (At<uint8_t>(playerPed, 0x1EC0) && !self->lockOn) {
            if (!TS_IsCandidateVisible(self, candidate, 1) && self->candidates.data[i].entity != lastPlayerCandidate) {
                continue;
            }
            if (special) {
                continue;
            }
            candidate = &self->candidates.data[i];
            entity = candidate->entity;
            lastPlayerCandidate = entity;
        }
        if (self->lockOn) {
            if (self->unknownAC == entity) {
                continue;
            }
            if (self->unknownB0 == entity) {
                skipSpecial = true;
                continue;
            }
        }
        if (timeInMs - candidate->lastChecked > 500) {
            TS_UpdateVisibility(self, candidate, 0.3f);
        }
        if (timeInMs - candidate->lastVisible >= 300 && !(candidate->flags & 1)) {
            continue;
        }
        if (self->candidates.data[i].score != kNoScore) {
            return self->candidates.data[i].entity;
        }
    }
    return nullptr;
}

#define TARGETING_C12B_REVERSED(name, address, fn)  \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

TARGETING_C12B_REVERSED("CTargetingSystem::ScoreTarget", 0x004D58E0, &ScoreTarget);
TARGETING_C12B_REVERSED("CTargetingSystem::WeaponHasFlag68", 0x004D59E0, &WeaponHasFlag68);
TARGETING_C12B_REVERSED("CTargetingSystem::AsObject", 0x004D5A40, &AsObject);
TARGETING_C12B_REVERSED("CColLine::Set", 0x004D5A60, &ColLineSet);
TARGETING_C12B_REVERSED("CTargetingSystem::ClassifyTarget", 0x004D5AE0, &ClassifyTarget);
TARGETING_C12B_REVERSED("TargetCandidate::Copy", 0x004D5D20, &CandidateCopy);
TARGETING_C12B_REVERSED("CTargetingSystem::GetRangeScale", 0x004D5D90, &GetRangeScale);
TARGETING_C12B_REVERSED("CTargetingSystem::GetZoom", 0x004D5DF0, &GetZoom);
TARGETING_C12B_REVERSED("CTargetingSystem::GetLockProgress", 0x004D5E20, &GetLockProgress);
TARGETING_C12B_REVERSED("CTargetingSystem::UpdateLockButton", 0x004D5EA0, &UpdateLockButton);
TARGETING_C12B_REVERSED("CTargetingSystem::GetUnknownA8", 0x004D5F60, &GetUnknownA8);
TARGETING_C12B_REVERSED("CTargetingSystem::SetAim", 0x004D5F70, &SetAim);
TARGETING_C12B_REVERSED("CTargetingSystem::GetTargetPos", 0x004D6000, &GetTargetPos);
TARGETING_C12B_REVERSED("CTargetingSystem::GetUnknown9C", 0x004D62C0, &GetUnknown9C);
TARGETING_C12B_REVERSED("TargetCandidateList::Find", 0x004D62D0, &CandidateListFind);
TARGETING_C12B_REVERSED("TargetCandidate::TargetCandidate", 0x004D63A0, &CandidateConstruct);
TARGETING_C12B_REVERSED("TargetCandidate::~TargetCandidate", 0x004D63E0, &CandidateDestruct);
TARGETING_C12B_REVERSED("TargetCandidate::operator=", 0x004D6400, &CandidateAssign);
TARGETING_C12B_REVERSED("CTargetingSystem::HeadingAndPitch", 0x004D6480, &HeadingAndPitch);
TARGETING_C12B_REVERSED("CTargetingSystem::Reset", 0x004D64F0, &Reset);
TARGETING_C12B_REVERSED("CTargetingSystem::SetTargetEntity", 0x004D6660, &SetTargetEntity);
TARGETING_C12B_REVERSED("CTargetingSystem::IsTargetScored", 0x004D6770, &IsTargetScored);
TARGETING_C12B_REVERSED("CTargetingSystem::ClearTarget", 0x004D6860, &ClearTarget);
TARGETING_C12B_REVERSED("CTargetingSystem::SortCandidates", 0x004D68A0, &SortCandidates);
TARGETING_C12B_REVERSED("CTargetingSystem::UpdateVisibility", 0x004D6A00, &UpdateVisibility);
TARGETING_C12B_REVERSED("CTargetingSystem::IsCandidateVisible", 0x004D6DA0, &IsCandidateVisible);
TARGETING_C12B_REVERSED("CTargetingSystem::IsEntityVisible", 0x004D6E20, &IsEntityVisible);
TARGETING_C12B_REVERSED("CTargetingSystem::PickBestCandidate", 0x004D6F10, &PickBestCandidate);

} // namespace targeting_c12b
