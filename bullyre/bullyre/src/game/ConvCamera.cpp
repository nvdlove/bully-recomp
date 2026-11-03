#include "game/Camera.h"

#include <cmath>
#include <cstring>

#include "game/Crt.h"
#include "reversed.h"
// The conversation camera (ConvCamCtrl, 0x4F5030..0x4F7280): two shots (the one it
// blends from and the one it goes to), each a pair of peds and the points the camera
// is placed from, and the "CinematicShotButes" (0x54-byte attribute sets: floats at
// +0x18..+0x50) that say where the camera sits relative to them. Then the cutscene
// camera's track helpers after it (0x4F74B0..0x4F7A40).
//
// x87: the game keeps sums of products in the FPU's precision and rounds to float where
// it stores; doubles stand in for the FPU registers here, floats where it stores.

#define CONVCAM_REVERSED(...) REVERSED(__VA_ARGS__)

namespace {

using cam::CVector;

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);            // CTimer's
GAME_VAR(float, timeStep, 0x00C1A9A4);               // CTimer's
GAME_VAR(void*, playerPed, 0x00C1AEA8);              // FindPlayerPed
GAME_VAR(void*, convCamCtrl, 0x00C3CFC0);            // the conversation camera, while there is one
GAME_VAR(uint8_t*, convSettings, 0x020C88DC);        // +0x40 radius, +0x44/+0x48 height pull, +0x4C smoothing
GAME_VAR(uint16_t, currentScanCode, 0x00C1AE88);     // CWorld's (entities with it are skipped)
GAME_VAR(uint8_t, testingSphere, 0x00C1AE8A);        // while a sphere test runs
GAME_VAR(uint8_t, missionRunning, 0x020C5BE6);
inline char* const missionName = reinterpret_cast<char*>(0x020C5B20);
// the sphere query's collision points (0x28 each, 32)
inline uint8_t* const sphereColPoints = reinterpret_cast<uint8_t*>(0x00C216C0);
// the camera's controller transition (a CameraTransition)
inline void* const cameraTransition = reinterpret_cast<void*>(0x00A45500);
// the butes tree: the conversation shots' path
GAME_VAR(const uint32_t, butesConvRoot, 0x01B6534C);
GAME_VAR(const uint32_t, butesConvPart1, 0x01B65350);
GAME_VAR(const uint32_t, butesConvPart2, 0x01B65354);
// (TheCamera's +0x31A: the fov is left alone while set)
GAME_VAR(uint8_t, cameraFovLocked, 0x00C3CF82);
inline void* const cameraAnimManager = reinterpret_cast<void*>(0x020C4910);

constexpr uintptr_t kConvCamCtrlVtable = 0x0091436C;
constexpr uintptr_t kCinematicShotButesVtable = 0x00914398;

// vector helpers (out = a + b / a - b / a x b; each returns out)
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, CrossProduct, 0x00414730, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(CVector*, __cdecl, RotateHeadingPitch, 0x00412530, CVector* out, CVector v, float heading, float pitch);
// the world
GAME_FN(uint8_t, __cdecl, GetIsLineOfSightClear, 0x004617D0, const CVector* a, const CVector* b, int flags);
GAME_FN(int32_t, __cdecl, TestSphereAgainstWorld, 0x00461380, CVector center, float radius, void* ignore, int flags);
GAME_FN(uint8_t, __cdecl, ProcessLineOfSight, 0x00461EA0, const CVector* start, const CVector* end, void* colPoint,
        re::Ptr32<void>* entity, int flags, int a, int b);
GAME_FN(void*, __thiscall, ColSphereQuery_Construct, 0x005707D0, void* query, const CVector* center, float radius,
        uint32_t lists, uint8_t* colPoints, int32_t maxPoints);
GAME_FN(int32_t, __thiscall, ColSphereQuery_Scan, 0x00570B40, void* query);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, re::Ptr32<void>* ref);
GAME_FN(CVector*, __thiscall, Entity_GetForward, 0x004154A0, void* entity, CVector* out);
// peds' bones
GAME_FN(void, __thiscall, Anim_6C0970, 0x006C0970, void* manager, void* anim, float, const float* offset, int, int);
GAME_FN(int32_t, __cdecl, Clump_FindBone, 0x0068E4E0, void* clump, const char* name);
// the butes
GAME_FN(void, __thiscall, CopyInt, 0x005FA880, uint32_t* dst, const uint32_t* src);
GAME_FN(void, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* self, const char* name);
GAME_FN(void, __thiscall, Path_Build, 0x005FA8C0, void* path, int count, const uint32_t* names);
GAME_FN(uint8_t*, __cdecl, Butes_Find, 0x0072AEA0, const void* path, const uint32_t* root);
GAME_FN(uint8_t*, __cdecl, CinematicShot_Find, 0x00684000, const void* name);
GAME_FN(void, __thiscall, Track_Construct, 0x0061A880, void* track);
GAME_FN(void, __thiscall, EmptyThiscall, 0x0049A650, void* self);
// the camera
GAME_FN(void, __thiscall, Camera_SetMode, 0x004F2C20, CCamera* camera, int controller, int a, int b, int c, int d);
GAME_FN(void, __thiscall, Camera_SetView, 0x004F2C60, CCamera* camera, const CVector* pos, const CVector* lookAt);
// (registered as ArcadeCameraController::SetOffset: it is TheCamera's +0x180)
GAME_FN(void, __thiscall, Camera_SetOffset180, 0x004F0920, CCamera* camera, const CVector* offset);
GAME_FN(void, __cdecl, Camera_SetNearClip, 0x006C8820, void* camera, float plane);
GAME_FN(int32_t, __thiscall, Transition_IsActive, 0x00856850, void* transition);
GAME_FN(int32_t*, __thiscall, Transition_GetController, 0x004F0B10, void* transition, int32_t* out, int end);
GAME_FN(void, __thiscall, Transition_Stop, 0x004F0B30, void* transition);
inline int __cdecl Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
GAME_FN(uint32_t, __cdecl, File_ReadItems, 0x0042D0E0, void* buffer, uint32_t size, uint32_t count, void* file);

using re::At;

template <typename R, typename... A>
R VCall(void* object, int index, A... args) {
    auto fn = reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(object))[index]));
    return fn(object, args...);
}

// ---- the structs

// A shot: two peds and their points (0x9C bytes; each array is per ped)
struct ConvShot {
    re::Ptr32<uint8_t> butes;          // +0 a CinematicShotButes of its own (0x54)
    re::Ptr32<void> peds[2];           // +4
    CVector pos[2];          // +0xC where the ped is
    CVector head[2];         // +0x24 its head bone (else its position)
    CVector unknown3C[2];    // +0x3C (0x4F4E00's)
    CVector unknown54[2];    // +0x54 (0x4F4E00's)
    CVector forward[2];      // +0x6C
    CVector unknown84[2];    // +0x84 (0x4F4E00's)
};
static_assert(sizeof(ConvShot) == 0x9C);

struct ConvCamCtrl {
    re::Ptr32<void> vtable;            // +0 (0x91436C)
    CVector pos;             // +4 the camera
    CVector lookAt;          // +0x10
    re::Ptr32<uint8_t> targetButes;    // +0x1C offsetButes eases towards it (a butes: 0x684000's)
    re::Ptr32<uint8_t> offsetButes;    // +0x20 own (added onto the shot's)
    re::Ptr32<uint8_t> baseButes;      // +0x24 own
    re::Ptr32<uint8_t> shotName;       // +0x28 the shot's butes (0x684000's)
    ConvShot from;           // +0x2C the shot blended from
    ConvShot to;             // +0xC8 the shot now
    uint32_t blendStart;     // +0x164 CTimer ms
    int32_t blendDuration;   // +0x168 ms, 0: no blend
    uint8_t smoothBlend;     // +0x16C the blend eased (a sine)
    uint8_t unknown16D[3];   // +0x16D
    re::Ptr32<uint8_t> shotList;       // +0x170 butes node: the shots to try (0x18..0x3C)
    int32_t snapCount;       // +0x174 moves the camera may still jump
    uint8_t reset;           // +0x178
    uint8_t unknown179[3];   // +0x179
    int32_t savedController; // +0x17C the camera's controller before this one
};
static_assert(offsetof(ConvCamCtrl, from) == 0x2C);
static_assert(offsetof(ConvCamCtrl, to) == 0xC8);
static_assert(offsetof(ConvCamCtrl, blendStart) == 0x164);
static_assert(offsetof(ConvCamCtrl, shotList) == 0x170);
static_assert(offsetof(ConvCamCtrl, savedController) == 0x17C);
static_assert(sizeof(ConvCamCtrl) == 0x180);

// What a script passes (as NativesA's ConvCamParams; 0x18 bytes)
struct ConvCamStart {
    re::Ptr32<void> peds[2];           // +0
    int32_t blendDuration;   // +8
    uint8_t smoothBlend;     // +0xC
    uint8_t offsetOnly;      // +0xD only sets the target offset butes
    uint8_t findClear;       // +0xE look for a shot with a clear view
    uint8_t unknownF;        // +0xF
    re::Ptr32<uint8_t> shot;           // +0x10 0x684000's
    re::Ptr32<uint8_t> shotList;       // +0x14 0x4F56B0's
};
static_assert(sizeof(ConvCamStart) == 0x18);

// CWorld's collision point (0x28)
struct ColPoint {
    CVector point;           // +0
    re::Ptr32<void> entity;            // +0xC (a registered reference)
    CVector normal;          // +0x10
    uint8_t unknown1C[8];    // +0x1C
    float depth;             // +0x24
};
static_assert(sizeof(ColPoint) == 0x28);

// the cutscene camera's arrays: {data, capacity, count}
struct TrackArray {
    re::Ptr32<uint8_t> data;           // +0
    uint32_t capacity;       // +4
    uint32_t count;          // +8
};
static_assert(sizeof(TrackArray) == 0xC);

// a cutscene camera key (0x30)
struct CutsceneKey {
    float time;              // +0 seconds
    CVector pos;             // +4
    CVector tangentIn;       // +0x10
    CVector tangentOut;      // +0x1C
    float easeIn;            // +0x28
    float easeOut;           // +0x2C
};
static_assert(sizeof(CutsceneKey) == 0x30);

// the shots' points (fork A's range: 0x4F4E80 between pos and head, 0x4F4F20 the other pair)
GAME_FN(CVector*, __thiscall, ConvShot_GetPointA, 0x004F4E80, ConvShot* shot, CVector* out, uint8_t* butes, int ped);
GAME_FN(CVector*, __thiscall, ConvShot_GetPointB, 0x004F4F20, ConvShot* shot, CVector* out, uint8_t* butes, int ped);
GAME_FN(void, __thiscall, ConvShot_Unknown4F4E00, 0x004F4E00, ConvShot* shot);

// own functions, called through the game's address
GAME_FN(uint8_t, __thiscall, ConvCamCtrl_IsShotClear, 0x004F5030, ConvCamCtrl* self, CVector pos, CVector lookAt);
GAME_FN(void, __thiscall, ConvCamCtrl_BlendButes, 0x004F55A0, ConvCamCtrl* self, uint8_t* out, uint8_t* a, uint8_t* b,
        float wa, float wb);
GAME_FN(uint8_t*, __cdecl, ConvCamCtrl_FindShotList, 0x004F56B0, const char* name);
GAME_FN(uint8_t, __cdecl, ConvCamCtrl_IsActive, 0x004F5740);
GAME_FN(void, __thiscall, ConvShot_UpdatePeds, 0x004F57A0, ConvShot* shot);
GAME_FN(void, __thiscall, ConvShot_GetCameraPoints, 0x004F58E0, ConvShot* shot, CVector* pos, CVector* lookAt, uint8_t* butes);
GAME_FN(float, __thiscall, ConvCamCtrl_GetBlend, 0x004F5F20, ConvCamCtrl* self);
GAME_FN(void, __thiscall, ConvCamCtrl_GetBlendedPoints, 0x004F5FF0, ConvCamCtrl* self, CVector* a, CVector* b, uint8_t* butes);
GAME_FN(void, __thiscall, ConvCamCtrl_PlaceCamera, 0x004F6240, ConvCamCtrl* self, CVector* pos, CVector* lookAt, CVector a,
        CVector b, uint8_t* butes);
GAME_FN(uint8_t*, __thiscall, ConvCamCtrl_FindClearShot, 0x004F6520, ConvCamCtrl* self, ConvShot* shot, uint8_t* butes, int keep);
GAME_FN(ConvShot*, __thiscall, ConvShot_Construct, 0x004F6670, ConvShot* shot);
GAME_FN(void, __thiscall, ConvCamCtrl_MoveTo, 0x004F6800, ConvCamCtrl* self, CVector pos, CVector lookAt);
GAME_FN(float, __cdecl, Cutscene_Ease, 0x004F74B0, float t, float a, float b);

float Butes(const uint8_t* butes, size_t offset) {
    return *reinterpret_cast<const float*>(butes + offset);
}

// a new CinematicShotButes (the Track base, its own vtable)
uint8_t* NewShotButes() {
    void* p = Malloc(0x54);
    if (!p) {
        return nullptr;
    }
    Track_Construct(p);
    At<uint32_t>(p, 0) = kCinematicShotButesVtable;
    return static_cast<uint8_t*>(p);
}

// an entity's position: its matrix's (+0x14, +0x30 in it) or its own (+4)
const CVector* EntityPosition(void* entity) {
    void* matrix = At<void*>(entity, 0x14);
    return matrix ? &At<CVector>(matrix, 0x30) : &At<CVector>(entity, 4);
}

CVector Scaled(const CVector& v, float s) {
    return {v.x * s, v.y * s, v.z * s};
}

float Length(const CVector& v) {
    // (the sum in the FPU's precision, stored, fabs, the CRT's sqrt, stored)
    float sq = float((double(v.x) * v.x + double(v.y) * v.y) + double(v.z) * v.z);
    return float(std::sqrt(double(std::fabs(sq))));
}

// ---- ConvCamCtrl

// 0x4F5030: a camera from `pos` looking at `lookAt` clear of the world (the line both
// ways, and four lines around it a radius off, all a radius longer at both ends; then
// a sphere of the radius at the camera)
uint8_t __fastcall ConvCamCtrl_IsShotClearOurs(ConvCamCtrl*, re::Edx, CVector pos, CVector lookAt) {
    float radius = Butes(convSettings, 0x40);
    CVector dir;
    VectorSub(&dir, &lookAt, &pos);
    Vector_Normalise(&dir);
    CVector up = {0.0f, 0.0f, 1.0f};
    CVector side, up2;
    CrossProduct(&side, &dir, &up);
    CrossProduct(&up2, &side, &dir);
    CVector ext = Scaled(dir, radius);
    pos = {pos.x - ext.x, pos.y - ext.y, pos.z - ext.z};
    lookAt = {ext.x + lookAt.x, lookAt.y + ext.y, lookAt.z + ext.z};

    CVector lines[5][2];
    lines[0][0] = pos;
    lines[0][1] = lookAt;
    CVector off = Scaled(up2, radius);
    VectorAdd(&lines[1][0], &pos, &off);
    VectorAdd(&lines[1][1], &lookAt, &off);
    VectorSub(&lines[2][0], &pos, &off);
    VectorSub(&lines[2][1], &lookAt, &off);
    off = Scaled(side, radius);
    VectorAdd(&lines[3][0], &pos, &off);
    VectorAdd(&lines[3][1], &lookAt, &off);
    VectorSub(&lines[4][0], &pos, &off);
    VectorSub(&lines[4][1], &lookAt, &off);
    for (int i = 0; i < 5; i++) {
        if (!GetIsLineOfSightClear(&lines[i][0], &lines[i][1], 0xB)) {
            return 0;
        }
        if (!GetIsLineOfSightClear(&lines[i][1], &lines[i][0], 0xB)) {
            return 0;
        }
    }
    return TestSphereAgainstWorld(pos, radius, nullptr, 0xB) == 0;
}
CONVCAM_REVERSED("ConvCamCtrl::IsShotClear", 0x004F5030, RE_BODY(&ConvCamCtrl_IsShotClearOurs), nullptr, nullptr, re::Mode::Original);

// 0x4F55A0: out = a * wa + b * wb over the butes' floats +0x18..+0x50 (any of them may
// be the same; ecx unused)
void __fastcall ConvCamCtrl_BlendButesOurs(ConvCamCtrl*, re::Edx, uint8_t* out, uint8_t* a, uint8_t* b, float wa, float wb) {
    for (size_t offset = 0x18; offset <= 0x50; offset += 4) {
        At<float>(out, offset) = float(double(Butes(a, offset)) * wa + double(Butes(b, offset)) * wb);
    }
}
CONVCAM_REVERSED("ConvCamCtrl::BlendButes", 0x004F55A0, RE_BODY(&ConvCamCtrl_BlendButesOurs), nullptr, nullptr, re::Mode::Original);

// 0x4F56B0: the butes node of a named shot list (under the conversation camera's path)
uint8_t* __cdecl ConvCamCtrl_FindShotListOurs(const char* name) {
    if (!name) {
        return nullptr;
    }
    uint32_t names[3];
    CopyInt(&names[0], &butesConvPart1);
    CopyInt(&names[1], &butesConvPart2);
    NameKey_Construct(&names[2], name);
    uint32_t path[2];
    Path_Build(path, 3, names);
    uint8_t* result = Butes_Find(path, &butesConvRoot);
    for (int i = 2; i >= 0; i--) {
        EmptyThiscall(&names[i]);
    }
    return result;
}
CONVCAM_REVERSED("ConvCamCtrl::FindShotList", 0x004F56B0, &ConvCamCtrl_FindShotListOurs, nullptr, nullptr, re::Mode::Original);

// 0x4F5740: the conversation camera is the active one (or the transition goes to it)
uint8_t __cdecl ConvCamCtrl_IsActiveOurs() {
    bool active = TheCamera.activeController == 10;
    if (Transition_IsActive(cameraTransition)) {
        int32_t controller;
        if (*Transition_GetController(cameraTransition, &controller, 1) == 10) {
            return 1;
        }
    }
    return active ? 1 : 0;
}
CONVCAM_REVERSED("ConvCamCtrl::IsActive", 0x004F5740, &ConvCamCtrl_IsActiveOurs, nullptr, nullptr, re::Mode::Original);

// 0x4F57A0: the peds' positions, heads (the "Head" bone of a ped's clump) and facing
void __fastcall ConvShot_UpdatePedsOurs(ConvShot* self, re::Edx) {
    for (int i = 0; i < 2; i++) {
        void* ped = self->peds[i];
        if (!ped) {
            continue;
        }
        const CVector* position = EntityPosition(ped);
        self->pos[i] = *position;
        self->head[i] = *position;
        void* matrix = At<void*>(ped, 0x14);
        if (matrix) {
            self->forward[i] = At<CVector>(matrix, 0x10);
        } else {
            float heading = At<float>(ped, 0x10);
            float s = float(std::sin(double(heading)));
            float c = float(std::cos(double(heading)));
            self->forward[i] = {-s, c, 0.0f};
        }
        void* clump = At<void*>(ped, 0x114);
        if (clump && (At<uint8_t>(ped, 0x108) & 7) == 3) {
            if (At<int32_t>(ped, 0xA4) != 0) {
                Anim_6C0970(cameraAnimManager, clump, 0.0f, nullptr, 1, 0);
            }
            int32_t bone = Clump_FindBone(At<void*>(ped, 0x114), "Head");
            uint8_t* bones = At<uint8_t*>(At<void*>(At<void*>(ped, 0x114), 0x14), 8);
            self->head[i] = *reinterpret_cast<CVector*>(bones + bone * 0x34 + 0x24);
        }
    }
}
CONVCAM_REVERSED("ConvShot::UpdatePeds", 0x004F57A0, RE_BODY(&ConvShot_UpdatePedsOurs), nullptr, nullptr, re::Mode::Original);

// 0x4F58E0: where the camera goes and looks for a shot's butes (+0x44/+0x48/+0x50: the
// mix between the two point pairs per axis, +0x24/+0x28: between the peds, +0x18: how
// much the peds' facing adds)
void __fastcall ConvShot_GetCameraPointsOurs(ConvShot* self, re::Edx, CVector* outPos, CVector* outLook, uint8_t* butes) {
    CVector tmp;
    CVector a0 = *ConvShot_GetPointA(self, &tmp, butes, 0);
    CVector a1 = *ConvShot_GetPointA(self, &tmp, butes, 1);
    CVector b0 = *ConvShot_GetPointB(self, &tmp, butes, 0);
    CVector b1 = *ConvShot_GetPointB(self, &tmp, butes, 1);
    double w50 = Butes(butes, 0x50), w4C = Butes(butes, 0x4C), w48 = Butes(butes, 0x48), w44 = Butes(butes, 0x44);
    double i50 = 1.0 - w50, i4C = 1.0 - w4C, i48 = 1.0 - w48, i44 = 1.0 - w44;

    CVector mixA0 = {float(b0.x * i50 + a0.x * w50), float(b0.y * i50 + a0.y * w50), float(b0.z * i4C + w4C * a0.z)};
    CVector mixB0 = {float(w48 * a0.x + b0.x * i48), float(b0.y * i48 + a0.y * w48), float(b0.z * i44 + w44 * a0.z)};
    CVector mixB1 = {float(b1.x * i48 + a1.x * w48), float(a1.y * w48 + b1.y * i48), float(i44 * b1.z + w44 * a1.z)};
    CVector mixA1 = {float(b1.x * i50 + a1.x * w50), float(a1.y * w50 + b1.y * i50), float(a1.z * w4C + b1.z * i4C)};

    float w28 = Butes(butes, 0x28);
    CVector p = Scaled(mixA1, w28);
    CVector q = Scaled(mixA0, float(1.0 - w28));
    VectorAdd(outPos, &q, &p);

    float w24 = Butes(butes, 0x24);
    p = Scaled(mixB1, w24);
    q = Scaled(mixB0, float(1.0 - w24));
    VectorAdd(outLook, &q, &p);

    if (Butes(butes, 0x18) != 0.0f) {
        CVector forward;
        Entity_GetForward(self->peds[0], &forward);
        CVector half = {float(forward.x * 0.5), float(forward.y * 0.5), float(forward.z * 0.5)};
        CVector ped0;
        VectorAdd(&ped0, &a0, &half);
        Entity_GetForward(self->peds[1], &forward);
        half = {float(forward.x * 0.5), float(forward.y * 0.5), float(forward.z * 0.5)};
        CVector ped1;
        VectorAdd(&ped1, &a1, &half);
        half = {float(self->unknown84[0].x * 0.5), float(self->unknown84[0].y * 0.5), float(self->unknown84[0].z * 0.5)};
        CVector other0;
        VectorAdd(&other0, &b0, &half);
        // (the second also from the first ped's point, as the game has it)
        half = {float(self->unknown84[1].x * 0.5), float(self->unknown84[1].y * 0.5), float(self->unknown84[1].z * 0.5)};
        CVector other1;
        VectorAdd(&other1, &b0, &half);

        w24 = Butes(butes, 0x24);
        p = Scaled(other1, w24);
        q = Scaled(other0, float(1.0 - Butes(butes, 0x24)));
        CVector others;
        VectorAdd(&others, &q, &p);
        p = Scaled(ped1, Butes(butes, 0x24));
        q = Scaled(ped0, float(1.0 - Butes(butes, 0x24)));
        CVector peds;
        VectorAdd(&peds, &q, &p);

        double j48 = 1.0 - Butes(butes, 0x48);
        double j44 = 1.0 - Butes(butes, 0x44);
        CVector mix = {float(peds.x * double(Butes(butes, 0x48)) + others.x * j48),
                       float(peds.y * double(Butes(butes, 0x48)) + others.y * j48),
                       float(j44 * others.z + double(Butes(butes, 0x44)) * peds.z)};
        float w18 = Butes(butes, 0x18);
        p = Scaled(mix, w18);
        q = Scaled(*outPos, float(1.0 - Butes(butes, 0x18)));
        VectorAdd(outPos, &q, &p);
    }
}
CONVCAM_REVERSED("ConvShot::GetCameraPoints", 0x004F58E0, RE_BODY(&ConvShot_GetCameraPointsOurs), nullptr, nullptr, re::Mode::Original);

// 0x4F5F20: how far the blend is, 0..1 (eased with a sine when smoothBlend)
float __fastcall ConvCamCtrl_GetBlendOurs(ConvCamCtrl* self, re::Edx) {
    float t = float(double(uint32_t(timeInMs - self->blendStart)) / double(self->blendDuration));
    if (t < 0.0f) {
        t = 0.0f;
    } else if (t >= 1.0f) {
        t = 1.0f;
    }
    if (self->smoothBlend) {
        float x = float((double(t) - 0.5) * 3.1415927410125732);
        float s = float(std::sin(double(x)));
        t = float((double(s) + 1.0) * 0.5);
        if (t < 1e-6f) {
            return 0.0f;
        }
        if (t >= 0.9999899864196777) {
            return 1.0f;
        }
    }
    return t;
}
CONVCAM_REVERSED("ConvCamCtrl::GetBlend", 0x004F5F20, RE_BODY(&ConvCamCtrl_GetBlendOurs), nullptr, nullptr, re::Mode::Original);

// 0x4F5FF0: the two points of the shot (blended from the old one while a blend runs);
// `butes` gets the shot's butes plus the offset
void __fastcall ConvCamCtrl_GetBlendedPointsOurs(ConvCamCtrl* self, re::Edx, CVector* outA, CVector* outB, uint8_t* butes) {
    if (self->blendDuration != 0) {
        float t = ConvCamCtrl_GetBlend(self);
        if (t == 1.0f) {
            self->blendDuration = 0;
        } else {
            float u = float(1.0 - t);
            ConvCamCtrl_BlendButes(self, butes, self->from.butes, self->to.butes, u, t);
            ConvCamCtrl_BlendButes(self, butes, butes, self->offsetButes, 1.0f, 1.0f);
            CVector fromA = {0.0f, 0.0f, 0.0f}, fromB = {0.0f, 0.0f, 0.0f};
            CVector toA = {0.0f, 0.0f, 0.0f}, toB = {0.0f, 0.0f, 0.0f};
            ConvShot_GetCameraPoints(&self->from, &fromA, &fromB, self->from.butes);
            ConvShot_GetCameraPoints(&self->to, &toA, &toB, self->to.butes);
            CVector p = Scaled(toA, t), q = Scaled(fromA, u);
            VectorAdd(outA, &q, &p);
            p = Scaled(toB, t);
            q = Scaled(fromB, u);
            VectorAdd(outB, &q, &p);
            return;
        }
    }
    if (self->blendDuration == 0) {
        ConvCamCtrl_BlendButes(self, butes, self->to.butes, self->to.butes, 1.0f, 0.0f);
        ConvCamCtrl_BlendButes(self, butes, butes, self->offsetButes, 1.0f, 1.0f);
        ConvShot_GetCameraPoints(&self->to, outA, outB, butes);
    }
}
CONVCAM_REVERSED("ConvCamCtrl::GetBlendedPoints", 0x004F5FF0, RE_BODY(&ConvCamCtrl_GetBlendedPointsOurs), nullptr, nullptr, re::Mode::Original);

// 0x4F6240: the camera from the shot's two points: looking from a to b turned by the
// butes' angles (+0x34 pitch, +0x38 heading, degrees), offset sideways (+0x2C) and up
// (+0x30), backed off the distance (+ butes +0x3C, at least 0.001) (ecx unused)
void __fastcall ConvCamCtrl_PlaceCameraOurs(ConvCamCtrl*, re::Edx, CVector* outPos, CVector* outLook, CVector a, CVector b,
                                            uint8_t* butes) {
    CVector d;
    VectorSub(&d, &b, &a);
    float distance = float(double(Length(d)) + Butes(butes, 0x3C));
    if (distance <= 0.001f) {
        distance = 0.001f;
    }
    CVector dir = {0.0f, 0.0f, 0.0f};
    VectorSub(&dir, &b, &a);
    Vector_Normalise(&dir);
    float heading = float(-((double(Butes(butes, 0x38)) * 3.1415927410125732) / 180.0));
    float pitch = float(-((double(Butes(butes, 0x34)) * 3.1415927410125732) / 180.0));
    CVector tmp;
    CVector forward = *RotateHeadingPitch(&tmp, dir, heading, pitch);
    CVector up = {0.0f, 0.0f, 1.0f};
    CVector side = *CrossProduct(&tmp, &forward, &up);
    Vector_Normalise(&side);
    CVector up2 = *CrossProduct(&tmp, &side, &forward);
    Vector_Normalise(&up2);
    CVector upOffset = Scaled(up2, Butes(butes, 0x30));
    CVector sideOffset = Scaled(side, Butes(butes, 0x2C));
    CVector sum, look;
    VectorAdd(&look, VectorAdd(&sum, &b, &sideOffset), &upOffset);
    *outLook = look;
    CVector back = Scaled(forward, distance);
    VectorSub(&tmp, outLook, &back);
    *outPos = tmp;
}
CONVCAM_REVERSED("ConvCamCtrl::PlaceCamera", 0x004F6240, RE_BODY(&ConvCamCtrl_PlaceCameraOurs), nullptr, nullptr, re::Mode::Original);

// 0x4F6520: the butes given if their camera is clear, else the first clear one of the
// shot list's (up to ten); none: 0 when `keep` is set, else the butes given
uint8_t* __fastcall ConvCamCtrl_FindClearShotOurs(ConvCamCtrl* self, re::Edx, ConvShot* shot, uint8_t* butes, int keep) {
    if (!butes) {
        return nullptr;
    }
    uint8_t* candidate = butes;
    for (int offset = 0x18;;) {
        CVector a = {0.0f, 0.0f, 0.0f}, b = {0.0f, 0.0f, 0.0f};
        ConvShot_GetCameraPoints(shot, &a, &b, candidate);
        CVector pos = {0.0f, 0.0f, 0.0f}, look = {0.0f, 0.0f, 0.0f};
        ConvCamCtrl_PlaceCamera(self, &pos, &look, a, b, candidate);
        if (ConvCamCtrl_IsShotClear(self, pos, look)) {
            return candidate;
        }
        if (!self->shotList) {
            break;
        }
        void* name = At<void*>(self->shotList, offset);
        if (!name) {
            break;
        }
        candidate = CinematicShot_Find(name);
        if (!candidate) {
            break;
        }
        offset += 4;
        if (offset >= 0x40) {
            break;
        }
    }
    return re::Flag(keep) ? nullptr : butes;
}
CONVCAM_REVERSED("ConvCamCtrl::FindClearShot", 0x004F6520, RE_BODY(&ConvCamCtrl_FindClearShotOurs), nullptr, nullptr, re::Mode::Original);

// 0x4F6670 (the old butes pointer is overwritten, not deleted)
ConvShot* __fastcall ConvShot_ConstructOurs(ConvShot* self, re::Edx) {
    std::memset(reinterpret_cast<uint8_t*>(self) + 0xC, 0, sizeof(ConvShot) - 0xC);
    self->butes = NewShotButes();
    self->peds[0] = nullptr;
    self->peds[1] = nullptr;
    return self;
}
CONVCAM_REVERSED("ConvShot::ConvShot", 0x004F6670, RE_BODY(&ConvShot_ConstructOurs), nullptr, nullptr, re::Mode::Original);

// 0x4F6730
ConvCamCtrl* __fastcall ConvCamCtrl_ConstructOurs(ConvCamCtrl* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kConvCamCtrlVtable);
    self->pos = {0.0f, 0.0f, 0.0f};
    self->lookAt = {0.0f, 0.0f, 0.0f};
    ConvShot_Construct(&self->from);
    ConvShot_Construct(&self->to);
    convCamCtrl = self;
    self->baseButes = NewShotButes();
    self->offsetButes = NewShotButes();
    ConvCamCtrl_BlendButes(self, self->offsetButes, self->offsetButes, self->offsetButes, 0.0f, 0.0f);
    self->smoothBlend = 1;
    self->savedController = 1;
    self->reset = 1;
    self->targetButes = nullptr;
    self->shotList = nullptr;
    self->snapCount = 0;
    self->shotName = nullptr;
    return self;
}
CONVCAM_REVERSED("ConvCamCtrl::ConvCamCtrl", 0x004F6730, RE_BODY(&ConvCamCtrl_ConstructOurs), nullptr, nullptr, re::Mode::Original);

// the shot's peds out of the line tests (the world's scan code)
void SkipShotPeds(ConvCamCtrl* self) {
    At<uint16_t>(self->to.peds[0], 0x10A) = uint16_t(currentScanCode + 1);
    if (self->to.peds[1] != self->to.peds[0]) {
        At<uint16_t>(self->to.peds[1], 0x10A) = uint16_t(currentScanCode + 1);
    }
}

// 0x4F6800: the camera to a new place: jumps there while it may (snapCount, outside a
// blend, when it moved over 0.5), else pulled in front of what's between it and the
// look-at point and out of the world's collision, smoothed
void __fastcall ConvCamCtrl_MoveToOurs(ConvCamCtrl* self, re::Edx, CVector pos, CVector lookAt) {
    if (self->blendDuration == 0) {
        CVector moved;
        VectorSub(&moved, &self->pos, &pos);
        if (Length(moved) > 0.5f && self->snapCount != 0) {
            self->snapCount--;
            self->pos = pos;
            self->lookAt = lookAt;
            return;
        }
    }
    ColPoint colPoint;
    std::memset(&colPoint, 0, sizeof(colPoint)); // (the game sets only point, entity, normal, depth)
    CVector target = pos;
    colPoint.depth = 3.4028234663852886e+38f;
    re::Ptr32<void> hitEntity = nullptr;
    if (self->reset) {
        SkipShotPeds(self);
        if (ProcessLineOfSight(&self->lookAt, &target, &colPoint, &hitEntity, 0x1B, 0, 0)) {
            CVector dir;
            VectorSub(&dir, &self->lookAt, &target);
            Vector_Normalise(&dir);
            CVector step = {float(double(dir.x) * double(0.1f)), float(double(dir.y) * double(0.1f)),
                            float(double(dir.z) * double(0.1f))};
            CVector tmp;
            target = *VectorAdd(&tmp, &colPoint.point, &step);
        }
    }
    SkipShotPeds(self);
    if (ProcessLineOfSight(&self->pos, &target, &colPoint, &hitEntity, 0x1B, 0, 0)) {
        CVector dir;
        VectorSub(&dir, &self->pos, &target);
        Vector_Normalise(&dir);
        CVector step = {float(double(dir.x) * double(0.0001f)), float(double(dir.y) * double(0.0001f)),
                        float(double(dir.z) * double(0.0001f))};
        CVector tmp;
        target = *VectorAdd(&tmp, &colPoint.point, &step);
    }
    SkipShotPeds(self);

    testingSphere = 1;
    alignas(4) uint8_t query[0x64];
    ColSphereQuery_Construct(query, &target, Butes(convSettings, 0x40), 0x1B, sphereColPoints, 0x20);
    int count = ColSphereQuery_Scan(query);
    if (count != 0) {
        for (int i = 0; i < count; i++) {
            ColPoint* point = reinterpret_cast<ColPoint*>(sphereColPoints) + i;
            float depth = point->depth;
            if (!(0.0f <= depth)) {
                continue;
            }
            CVector normal = point->normal;
            CVector at = point->point;
            CVector dir;
            VectorSub(&dir, &target, &at);
            Vector_Normalise(&dir);
            CVector push = Scaled(dir, depth);
            target.x = push.x + target.x;
            target.y = target.y + push.y;
            target.z = push.z + target.z;
            if (-0.75f < normal.z) {
                // (towards the wanted height)
                float toWanted = pos.z - target.z;
                target.z = float(double(target.z) +
                                 (double(toWanted) * Butes(convSettings, 0x48) + double(Butes(convSettings, 0x44)) * depth));
            }
        }
        float w = Butes(convSettings, 0x4C);
        CVector a = Scaled(target, w);
        CVector b = Scaled(self->pos, float(1.0 - Butes(convSettings, 0x4C)));
        CVector tmp;
        target = *VectorAdd(&tmp, &b, &a);
    }
    self->pos = target;
    self->lookAt = lookAt;
    testingSphere = 0;
    EmptyThiscall(query);
    if (colPoint.entity) {
        CleanUpOldReference(colPoint.entity, &colPoint.entity);
    }
}
CONVCAM_REVERSED("ConvCamCtrl::MoveTo", 0x004F6800, RE_BODY(&ConvCamCtrl_MoveToOurs), nullptr, nullptr, re::Mode::Original);

// 0x4F6D40: starts a shot (or only sets the offset target); 0: no shot. `switchCamera`
// makes it the camera's controller (or hands back to the saved one when no shot is
// given), `blendBack` the blend for handing back
uint8_t __fastcall ConvCamCtrl_StartOurs(ConvCamCtrl* self, re::Edx, ConvCamStart* params, int switchCamera, int blendBack,
                                         int keep) {
    if (params->offsetOnly) {
        self->targetButes = params->shot;
        return 1;
    }
    int32_t blend = params->blendDuration;
    if (re::Flag(switchCamera)) {
        if (!ConvCamCtrl_IsActive()) {
            if (params->shot) {
                self->savedController = TheCamera.activeController;
                int arg = 0;
                if (blend != 0) {
                    self->to.peds[0] = nullptr;
                    self->to.peds[1] = nullptr;
                    arg = blend;
                }
                Camera_SetMode(&TheCamera, 10, arg, 1, 0, 1);
            }
        } else if (!params->shot) {
            if (Transition_IsActive(cameraTransition) && blend != 0) {
                Camera_SetMode(&TheCamera, TheCamera.activeController, blend, 1, 0, 1);
                blend = 0;
            } else {
                self->to.peds[0] = nullptr;
                self->to.peds[1] = nullptr;
                Transition_Stop(cameraTransition);
                Camera_SetMode(&TheCamera, self->savedController, 0, blendBack, 0, 1);
                self->savedController = 1;
            }
            self->shotList = nullptr;
            self->shotName = CinematicShot_Find("LShoulder");
        }
    }

    ConvShot old;
    ConvShot_Construct(&old);
    if (params->shot) {
        if (blend != 0) {
            for (int i = 0; i < 2; i++) {
                old.peds[i] = self->to.peds[i];
                old.pos[i] = self->to.pos[i];
                old.head[i] = self->to.head[i];
                old.unknown3C[i] = self->to.unknown3C[i];
                old.unknown54[i] = self->to.unknown54[i];
                old.forward[i] = self->to.forward[i];
                old.unknown84[i] = self->to.unknown84[i];
            }
        }
        if (params->shot) {
            if (!self->to.peds[0]) {
                self->to.peds[0] = playerPed;
            }
            if (params->shot && !self->to.peds[1]) {
                self->to.peds[1] = playerPed;
            }
        }
    }
    if (params->peds[0]) {
        self->to.peds[0] = params->peds[0];
    }
    if (params->peds[1]) {
        self->to.peds[1] = params->peds[1];
    }
    if (self->to.peds[0] && self->to.peds[1] && params->shot) {
        ConvShot_UpdatePeds(&self->to);
        ConvShot_Unknown4F4E00(&self->to);
    }
    if (params->shotList) {
        self->shotList = params->shotList;
    }
    if (!self->shotList) {
        self->shotList = ConvCamCtrl_FindShotList("Conv");
    }
    self->shotName = params->shot;
    uint8_t* shot = params->shot;
    if (params->findClear) {
        shot = ConvCamCtrl_FindClearShot(self, &self->to, shot, keep);
    }
    if (!shot) {
        if (old.butes) {
            VCall<void>(old.butes, 1, 1);
        }
        return 0;
    }
    if (!old.peds[0] || !old.peds[1]) {
        blend = 0;
    } else if (blend != 0) {
        // the old shot becomes the one blended from
        ConvCamCtrl_BlendButes(self, self->from.butes, self->baseButes, self->baseButes, 1.0f, 0.0f);
        ConvCamCtrl_BlendButes(self, self->to.butes, shot, shot, 1.0f, 0.0f);
        for (int i = 0; i < 2; i++) {
            self->from.peds[i] = old.peds[i];
            self->from.pos[i] = old.pos[i];
            self->from.head[i] = old.head[i];
            self->from.unknown3C[i] = old.unknown3C[i];
            self->from.unknown54[i] = old.unknown54[i];
            self->from.forward[i] = old.forward[i];
            self->from.unknown84[i] = old.unknown84[i];
        }
        self->smoothBlend = params->smoothBlend;
    }
    if (blend == 0) {
        CVector a = {0.0f, 0.0f, 0.0f}, b = {0.0f, 0.0f, 0.0f};
        ConvCamCtrl_BlendButes(self, self->to.butes, shot, shot, 1.0f, 0.0f);
        ConvShot_GetCameraPoints(&self->to, &a, &b, self->to.butes);
        ConvCamCtrl_PlaceCamera(self, &self->pos, &self->lookAt, a, b, self->to.butes);
    }
    ConvCamCtrl_BlendButes(self, self->offsetButes, self->offsetButes, self->offsetButes, 0.0f, 0.0f);
    self->blendStart = timeInMs;
    self->blendDuration = blend;
    self->targetButes = nullptr;
    self->reset = params->findClear;
    bool active = TheCamera.activeController == 10;
    bool toThis = false;
    if (Transition_IsActive(cameraTransition)) {
        int32_t controller;
        toThis = *Transition_GetController(cameraTransition, &controller, 1) == 10;
    }
    if (toThis || active) {
        VCall<void>(self, 7);
    }
    if (old.butes) {
        VCall<void>(old.butes, 1, 1);
    }
    return 1;
}
CONVCAM_REVERSED("ConvCamCtrl::Start", 0x004F6D40, RE_BODY(&ConvCamCtrl_StartOurs), nullptr, nullptr, re::Mode::Original);

// 0x4F7280 (vtable[7], process): the camera placed for the shot
void __fastcall ConvCamCtrl_ProcessOurs(ConvCamCtrl* self, re::Edx) {
    if (!self->to.peds[0] || !self->to.peds[1]) {
        return;
    }
    CVector a = {0.0f, 0.0f, 0.0f}, b = {0.0f, 0.0f, 0.0f};
    uint8_t* butes = self->baseButes;
    if (self->targetButes) {
        ConvCamCtrl_BlendButes(self, self->offsetButes, self->offsetButes, self->targetButes, 1.0f,
                               float(double(timeStep) / 30.0));
    }
    ConvShot_UpdatePeds(&self->to);
    ConvCamCtrl_GetBlendedPoints(self, &a, &b, butes);
    CVector pos = {0.0f, 0.0f, 0.0f}, look = {0.0f, 0.0f, 0.0f};
    ConvCamCtrl_PlaceCamera(self, &pos, &look, a, b, butes);
    ConvCamCtrl_MoveTo(self, pos, look);
    Camera_SetView(&TheCamera, &self->pos, &self->lookAt);
    if (!cameraFovLocked) {
        TheCamera.fovOverridden = 0;
        TheCamera.fov = Butes(butes, 0x40);
    }
    CVector zero = {0.0f, 0.0f, 0.0f};
    Camera_SetOffset180(&TheCamera, &zero);
    // TheCamera +0x18C cleared, +0x198 the look-at point
    At<CVector>(&TheCamera, 0x18C) = {0.0f, 0.0f, 0.0f};
    TheCamera.unknown198 = self->lookAt;
    Camera_SetNearClip(TheCamera.camera, 0.25f);
}
CONVCAM_REVERSED("ConvCamCtrl::Process", 0x004F7280, RE_BODY(&ConvCamCtrl_ProcessOurs), nullptr, nullptr, re::Mode::Original);

// ---- the cutscene camera's track helpers

// 0x4F74B0: the time through a key eased (accelerating over the first `a`, decelerating
// over the last `b`; a + b over 1 scaled down)
float __cdecl Cutscene_EaseOurs(float t, float a, float b) {
    float sum = a + b;
    if (sum == 0.0f) {
        return t;
    }
    if (1.0f < sum) {
        a = float(double(a) / sum);
        b = float(double(b) / sum);
    }
    float k = float(1.0 / ((2.0 - a) - b));
    if (t < a) {
        return float(double(t) * (double(k) / a) * t);
    }
    if (t < 1.0 - b) {
        return float((2.0 * t - a) * k);
    }
    float u = float(1.0 - t);
    return float(1.0 - double(u) * (double(k) / b) * u);
}
CONVCAM_REVERSED("CutsceneCameraController::Ease4F74B0", 0x004F74B0, &Cutscene_EaseOurs, nullptr, nullptr, re::Mode::Original);

// 0x4F75A0 (index in edx, the track in esi: not hookable): moves *index on to the last
// key (0x30 each) at or before `t`
void Cutscene_FindKey30(int32_t* index, TrackArray* track, float t) {
    while (uint32_t(*index + 1) < track->count) {
        float next = *reinterpret_cast<float*>(track->data + (*index + 1) * 0x30);
        // (on while the next key is at or before t)
        if (!(next <= t)) {
            return;
        }
        ++*index;
    }
}
RE_UNHOOKED_FN("CutsceneCameraController::FindKey4F75A0", 0x004F75A0, &Cutscene_FindKey30);

// 0x4F75E0 (the same for 0xC-byte keys)
void Cutscene_FindKeyC(int32_t* index, TrackArray* track, float t) {
    while (uint32_t(*index + 1) < track->count) {
        float next = *reinterpret_cast<float*>(track->data + (*index + 1) * 0xC);
        // (on while the next key is at or before t)
        if (!(next <= t)) {
            return;
        }
        ++*index;
    }
}
RE_UNHOOKED_FN("CutsceneCameraController::FindKey4F75E0", 0x004F75E0, &Cutscene_FindKeyC);

// 0x4F7620 (index in eax, the track in edx, out in edi: not hookable): the position at
// `t` between a key and the next (Hermite with the keys' tangents; mission 1-01's keys
// under 1/15 s apart are straight lines instead, under 1/60 s held)
void Cutscene_Interpolate(int32_t index, TrackArray* track, CVector* out, float t) {
    if (track->count == 0) {
        return;
    }
    CutsceneKey* key = re::PtrCast<CutsceneKey*>(track->data) + index;
    CutsceneKey* next = key + 1;
    if (uint32_t(index + 1) >= track->count) {
        *out = key->pos;
        return;
    }
    const char* name = missionRunning ? missionName : nullptr;
    if (name && Stricmp(name, "1-01") == 0) {
        if (!(key->time < t)) {
            *out = key->pos;
            return;
        }
        float span = float(double(next->time) - key->time);
        if (span < 0.06666667f) {
            if (span < 0.016666668f) {
                *out = key->pos;
                return;
            }
            float u = float((double(t) - key->time) / (double(next->time) - key->time));
            float off = float(std::fabs(double(float(double(u) - 1.0))));
            if (off <= 9.999999747378752e-06f) {
                *out = next->pos;
                return;
            }
            CVector a = Scaled(next->pos, u);
            CVector b = Scaled(key->pos, float(1.0 - u));
            CVector tmp;
            *out = *VectorAdd(&tmp, &b, &a);
            return;
        }
    } else if (!(key->time < t) || !(double(next->time) - key->time > 0.4000000059604645)) {
        *out = key->pos;
        return;
    }
    float r = Cutscene_Ease(float((double(t) - key->time) / (double(next->time) - key->time)), key->easeOut, next->easeIn);
    float r2 = float(double(r) * r);
    float r3 = float(double(r2) * r);
    float h00 = float((2.0 * r3 - 3.0 * r2) + 1.0);
    float h01 = float(3.0 * r2 - 2.0 * r3);
    float h10 = float(double(r) + (double(r3) - 2.0 * r2));
    float h11 = float(double(r3) - r2);
    CVector v1 = Scaled(next->tangentIn, h11);
    CVector v2 = Scaled(key->tangentOut, h10);
    CVector v3 = Scaled(next->pos, h01);
    CVector v4 = Scaled(key->pos, h00);
    CVector s1, s2, s3;
    VectorAdd(&s1, &v4, &v3);
    VectorAdd(&s2, &s1, &v2);
    *out = *VectorAdd(&s3, &s2, &v1);
}
RE_UNHOOKED_FN("CutsceneCameraController::Interpolate4F7620", 0x004F7620, &Cutscene_Interpolate);

// 0x4F78F0 / 0x4F7990: an array to a new capacity (elements copied over; the count as
// well when `setCount`)
template <size_t Size>
void ReserveArray(TrackArray* self, uint32_t capacity, int setCount) {
    uint8_t* data = nullptr;
    self->capacity = capacity;
    if (capacity != 0) {
        uint64_t bytes = uint64_t(capacity) * Size;
        data = static_cast<uint8_t*>(Malloc((bytes >> 32) != 0 ? 0xFFFFFFFFu : uint32_t(bytes)));
    }
    if (self->data) {
        for (uint32_t i = 0; i < self->count; i++) {
            std::memcpy(data + i * Size, self->data + i * Size, Size);
        }
        Free(self->data);
    }
    self->data = data;
    if (re::Flag(setCount)) {
        self->count = self->capacity;
    }
}

void __fastcall Array18_Reserve(TrackArray* self, re::Edx, uint32_t capacity, int setCount) {
    ReserveArray<0x18>(self, capacity, setCount);
}
CONVCAM_REVERSED("CutsceneCameraController::Reserve4F78F0", 0x004F78F0, RE_BODY(&Array18_Reserve), nullptr, nullptr, re::Mode::Original);

void __fastcall ArrayC_Reserve(TrackArray* self, re::Edx, uint32_t capacity, int setCount) {
    ReserveArray<0xC>(self, capacity, setCount);
}
CONVCAM_REVERSED("CutsceneCameraController::Reserve4F7990", 0x004F7990, RE_BODY(&ArrayC_Reserve), nullptr, nullptr, re::Mode::Original);

// 0x4F7A40: a line (to the '\n', not kept) from a file or a buffer: {+0 file, +4
// buffer, +0xC position}; 0 when `max` bytes came first. (The byte goes through the
// `max` argument's own stack slot: before anything is read, or with no stream, it is
// max's low byte)
uint8_t __cdecl CutsceneStream_ReadLine(uint8_t* stream, char* line, int max) {
    uint8_t c = uint8_t(max);
    for (int i = 0; i < max; i++) {
        if (stream) {
            void* file = At<void*>(stream, 0);
            if (file) {
                File_ReadItems(&c, 1, 1, file);
            } else if (uint8_t* buffer = At<uint8_t*>(stream, 4)) {
                c = buffer[At<int32_t>(stream, 0xC)];
                At<int32_t>(stream, 0xC)++;
            }
        }
        if (c == '\n') {
            line[i] = 0;
            return 1;
        }
        line[i] = char(c);
    }
    return 0;
}
CONVCAM_REVERSED("CutsceneStream::ReadLine", 0x004F7A40, &CutsceneStream_ReadLine, nullptr, nullptr, re::Mode::Original);

} // namespace
