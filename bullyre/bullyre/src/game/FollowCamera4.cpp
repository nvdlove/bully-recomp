#include "game/Camera.h"

#include <cmath>
#include <initializer_list>
#include "reversed.h"
#include "runflags.h"

// FollowCameraBase / FollowCameraController3, part 4 (0x5028F0..0x505300): the
// collision pass that keeps the camera out of walls (0x5028F0), placing it (0x5044A0,
// 0x5048E0), the base's process (vtable[7]) and constructors (the base, its fight
// part), and the start of OffsetCameraController (its constructor, attribute set by
// name, clear). Thiscalls are __fastcall wrappers with the unused edx.
//
// Layout used here (FollowCameraBase; see FollowCamera3.cpp/3b.cpp for the rest):
//   +0x08 the look-from position, +0x20 the turn this frame, +0x28 the time step
//   / 50, +0x2C the sub-step, +0x31 the ped's +0x1438, +0x34 the wanted camera
//   position, +0x40 the wanted look-at, +0x4C, +0x68/+0x6C/+0x78, +0x88 the turn
//   rate, +0x90, +0x9C/+0xA0, +0xA4 the camera position, +0xB0 the look-at, +0xBC,
//   +0xC8 the flat distance to the look-at, +0xCC the height above it, +0xD0 the
//   flat direction, +0xDC the smoothed position, +0xF4 an eased value
//   (FollowInterp), +0x10C/+0x110/+0x114 blocked (how much, since when, flag),
//   +0x115/+0x116 close-up flags, +0x118 the smoothed camera position, +0x124 the
//   smoothed look-at, +0x130 (from +0x4C), +0x13C/+0x148/+0x154 last frame's
//   +0x118/+0x124/+0x130, +0x160 the fight part (0xFC bytes), +0x25C, +0x274,
//   +0x278 the vehicle part (+0x27C the controller it drives), +0x284..+0x298,
//   +0x2AC/+0x2B0 (a raised sweep start), +0x2C0, +0x304 the contacts kept from
//   last frame (32 normals, +0x484 32 depths, +0x504 the count, +0x508 how many
//   frames the push holds, +0x50C the push, +0x510 the hits this frame), +0x518 the
//   attribute set (FollowCam3Butes), +0x51C.

#define FOLLOWCAM4_REVERSED(...) REVERSED(__VA_ARGS__)

namespace {

using cam::CVector;

// a capsule (0x38): 0x4F8AE0 sets it between two points
struct Capsule {
    CVector centre;  // +0
    CVector dir;     // +0xC unit, start to end
    CVector start;   // +0x18
    CVector end;     // +0x24
    float length;    // +0x30
    float radius;    // +0x34
};
static_assert(sizeof(Capsule) == 0x38);
static_assert(offsetof(Capsule, radius) == 0x34);

// the capsule's scan of the world (0x64 bytes on the stack): 0x562D80 sets it up,
// 0x563E80 runs it (the colpoints found), noting the entities touched
struct CapsuleQuery {
    uint8_t unknown0[0x58];  // +0 (the capsule, its lists, bounds, colpoints)
    re::Ptr32<re::Ptr32<void>> entities;         // +0x58 where the entities touched go
    int32_t maxEntities;     // +0x5C
    int32_t numEntities;     // +0x60
};
static_assert(sizeof(CapsuleQuery) == 0x64);

// a colpoint (0x28)
struct ColPoint {
    CVector point;          // +0
    re::Ptr32<uint8_t> entity;        // +0xC
    CVector normal;         // +0x10
    uint8_t unknown1C[8];   // +0x1C
    float depth;            // +0x24
};
static_assert(sizeof(ColPoint) == 0x28);
static_assert(offsetof(ColPoint, depth) == 0x24);

GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);            // CTimer's
GAME_VAR(uint16_t, currentScanCode, 0x00C1AE88);     // CWorld::ms_nCurrentScanCode
GAME_VAR(uint8_t, g_C1AE8A, 0x00C1AE8A);             // set around the scans below
GAME_VAR(uint8_t*, cameraTarget, 0x00C3CE14);        // the camera's target entity
GAME_VAR(uint8_t*, cameraSettings, 0x020C88B8);
GAME_VAR(uint8_t*, defaultOffsetButes, 0x020C88C4);  // OffsetCameraController's default attribute set
GAME_VAR(int32_t, g_C3D0D8, 0x00C3D0D8);             // the sub-step, while the base processes
GAME_VAR(uint8_t*, g_C3CE1C, 0x00C3CE1C);            // (a registered reference)
GAME_VAR(int32_t, g_C3CD70, 0x00C3CD70);
GAME_VAR(float, g_C3CD74, 0x00C3CD74);
GAME_VAR(float, g_C3CD78, 0x00C3CD78);
GAME_VAR(float, g_C3CD7C, 0x00C3CD7C);
GAME_VAR(float, g_C3CD80, 0x00C3CD80);
GAME_VAR(int32_t, g_C3B6F8, 0x00C3B6F8);
GAME_VAR(int32_t, g_C3B6FC, 0x00C3B6FC);
GAME_VAR(uint8_t, g_C3B700, 0x00C3B700);
// the collision pass's tunables
GAME_VAR(float, g_A4589C, 0x00A4589C);   // 0.5: the turn pull's share of distance vs height
GAME_VAR(float, g_A458A0, 0x00A458A0);   // 1.25 \ the flat distance the turn pull fades in over
GAME_VAR(float, g_A458A4, 0x00A458A4);   // 1.75 /
GAME_VAR(float, g_A458A8, 0x00A458A8);   // 0.1: the distance lost that counts as fully blocked
GAME_VAR(float, g_A458AC, 0x00A458AC);   // 180: the most the direction turns (degrees, scaled by +0x28)
GAME_VAR(float, g_A458B0, 0x00A458B0);   // 0.03 \ the flat distance the direction starts following at
GAME_VAR(float, g_A458B4, 0x00A458B4);   // 0.3  /
GAME_VAR(float, g_A458B8, 0x00A458B8);   // 0.3: the push's decay
GAME_VAR(float, g_A458BC, 0x00A458BC);   // 0.1
GAME_VAR(uint8_t, g_A458C0, 0x00A458C0); // 1: slide along what's hit
GAME_VAR(float, g_A458C4, 0x00A458C4);   // 0.05: the most the smoothed position moves in one go
GAME_VAR(float, g_C3D298, 0x00C3D298);
GAME_VAR(float, g_C3D29C, 0x00C3D29C);   // frames the push holds
GAME_VAR(float, g_C3D2A0, 0x00C3D2A0);
// the scans' results
GAME_ARRAY(ColPoint, colPoints, 0x00C216C0);      // 32
GAME_ARRAY(void*, touchedEntities, 0x00C3CFD8);   // 32
GAME_ARRAY(float, contactDepths, 0x00C3D058);     // 32
GAME_ARRAY(CVector, contactNormals, 0x00C3D0F0);  // 32
// the butes names (hashes) and roots the attribute sets are looked up by
inline const void* const name_1B6531C = reinterpret_cast<const void*>(0x01B6531C);
inline const void* const name_1B6535C = reinterpret_cast<const void*>(0x01B6535C);
inline const void* const root_1B65360 = reinterpret_cast<const void*>(0x01B65360);
inline const void* const name_1B653F4 = reinterpret_cast<const void*>(0x01B653F4);
inline const void* const root_1B653F0 = reinterpret_cast<const void*>(0x01B653F0);

constexpr double kPi = 3.1415927410125732;       // (float pi, as a double)
constexpr double kHalfPi = 1.5707963705062866;   // (float pi / 2, as a double)

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __thiscall, EmptyThiscall, 0x0049A650, void* self);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, re::Ptr32<void>* ref);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(float, __cdecl, VecHeading, 0x00433190, const CVector* v);
GAME_FN(float, __cdecl, AngleDiff, 0x00432980, float a, float b);
GAME_FN(CVector*, __cdecl, VecFromHeading, 0x00432DF0, CVector* out, float heading);
// collision
GAME_FN(void, __thiscall, Capsule_Set, 0x004F8AE0, Capsule* self, const CVector* start, const CVector* end, float radius);
GAME_FN(void, __thiscall, CapsuleQuery_Setup, 0x00562D80, CapsuleQuery* self, const Capsule* capsule, int lists, ColPoint* points, int maxPoints);
GAME_FN(int32_t, __thiscall, CapsuleQuery_Scan, 0x00563E80, CapsuleQuery* self);
GAME_FN(void*, __thiscall, SphereQuery_Construct, 0x005707D0, void* self, const CVector* centre, float radius, uint32_t lists, ColPoint* points, int32_t maxPoints);
GAME_FN(int32_t, __thiscall, SphereQuery_ProcessEntity, 0x00570AA0, void* self, void* entity);
GAME_FN(void, __thiscall, Sweep_Construct, 0x00563060, void* self, const CVector* from, const CVector* to, float radius, int flags, void* hit, void* filter, int a, int b, uintptr_t c);
GAME_FN(int32_t, __thiscall, Sweep_Test, 0x00564080, void* self);
// the butes (names are 4-byte hashes; a path is {count, names})
GAME_FN(void, __thiscall, Name_Copy, 0x005FA880, void* self, const void* from);
GAME_FN(void, __thiscall, Name_Construct, 0x005FA910, void* self, uintptr_t value);
GAME_FN(void, __thiscall, Path_Build, 0x005FA8C0, void* self, int count, const void* names);
GAME_FN(void*, __cdecl, Butes_Find, 0x0072AEA0, const void* path, const void* root);
// the follow camera's parts
GAME_FN(void*, __thiscall, FollowInterp_Construct, 0x004FB360, void* self);
GAME_FN(void, __thiscall, FollowInterp_Set, 0x004FB380, void* self, float value);
GAME_FN(void*, __thiscall, FollowCameraFight_Construct, 0x00504A40, void* self);
GAME_FN(void, __thiscall, FollowCameraFight_Reset, 0x004FE640, void* self);
GAME_FN(void, __thiscall, FollowCameraFight_Unknown4FE980, 0x004FE980, void* self);
GAME_FN(void, __thiscall, FollowCameraFight_Unknown4FEB90, 0x004FEB90, void* self, void* a, void* b);
GAME_FN(void*, __thiscall, VehicleCameraController_Construct, 0x0050AF60, void* self);
GAME_FN(void, __thiscall, VehicleCameraController_Unknown509F50, 0x00509F50, void* self, CVector position, CVector lookAt, float distance, int hits);
GAME_FN(void*, __thiscall, Contacts_Construct, 0x004FE080, void* self);
GAME_FN(float, __thiscall, FollowCameraBase_GetDistance, 0x004FE110, void* self);
GAME_FN(void, __thiscall, FollowCameraBase_Unknown4FCD70, 0x004FCD70, void* self);   // (on +0x68)
GAME_FN(void, __thiscall, FollowCameraBase_Unknown4FFEC0, 0x004FFEC0, void* self);   // (on +0x2AC)
GAME_FN(void, __thiscall, FollowCameraBase_Unknown4FFF50, 0x004FFF50, void* self, CVector a, CVector b, CVector c);   // (on +0x2C0)
GAME_FN(void, __thiscall, FollowCameraBase_Unknown5003C0, 0x005003C0, void* self);
GAME_FN(void, __thiscall, FollowCameraBase_Unknown500B40, 0x00500B40, void* self);
GAME_FN(void, __thiscall, FollowCameraBase_Collide, 0x005028F0, void* self);
GAME_FN(void, __thiscall, FollowCameraController3_Place, 0x005044A0, void* self, CVector lookAt, CVector position, int sweep);
// OffsetCameraController
GAME_FN(void, __cdecl, Clear_C3B708, 0x004DB5F0);   // (clears 0xC3B708; called with ecx left over)
GAME_FN(void, __thiscall, OffsetCameraController_LookUpBones, 0x00504F90, void* self);

using re::At;

template <typename R, typename... A>
R VCall(void* object, int index, A... args) {
    auto fn = reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(object))[index]));
    return fn(object, args...);
}

uint8_t* Butes(void* self) {
    return At<uint8_t*>(self, 0x518);
}

// (the game: the float sum's fabs, then the CRT's sqrt)
float Root(float sumOfSquares) {
    return float(std::sqrt(double(std::fabs(sumOfSquares))));
}

// the camera's target (and what it's in) marked as scanned, so the scans skip them
void MarkTargetScanned() {
    uint8_t* target = cameraTarget;
    if ((At<uint8_t>(target, 0x108) & 7) == 3 && At<uint8_t*>(target, 0x1554)) {
        At<uint16_t>(At<uint8_t*>(target, 0x1554), 0x10A) = uint16_t(currentScanCode + 1);
    }
    At<uint16_t>(cameraTarget, 0x10A) = uint16_t(currentScanCode + 1);
    if (uint8_t* other = At<uint8_t*>(cameraTarget, 0x155C)) {
        At<uint16_t>(other, 0x10A) = uint16_t(currentScanCode + 1);
    }
}

// the velocity's part into a surface taken out (when sliding along it)
void SlideAlong(CVector* velocity, const CVector& normal) {
    float dot = (normal.x * velocity->x + normal.y * velocity->y) + normal.z * velocity->z;
    if (dot < 0.0f) {
        float m = -dot;
        CVector out{m * normal.x, normal.y * m, normal.z * m};
        CVector sum;
        *velocity = *VectorAdd(&sum, velocity, &out);
    }
}

// ---- FollowCameraBase

// 0x5028F0: keeps the camera out of what's around it: the smoothed position eased
// towards the wanted one (at most 0.05 at a time; the rest as a velocity), the
// capsule from the look-at to the camera scanned against the world (the camera
// pushed out of each hit, the velocity sliding along them), the entities touched
// tested again with a sphere at the camera; then how far it got pushed in (held a
// few frames), the flat direction turned towards the look-at, the distance and
// height pulled in by the push and the turn rate, and the smoothed position and
// look-at blended towards where they should be, for the vehicle part.
void __fastcall FollowCameraBase_CollideOurs(uint8_t* self, re::Edx) {
    uint8_t* butes = Butes(self);
    CVector& position = At<CVector>(self, 0xA4);
    CVector& lookAt = At<CVector>(self, 0xB0);
    CVector oldLookAt = lookAt;
    position = At<CVector>(self, 0x34);
    lookAt = At<CVector>(self, 0x40);
    CVector wanted = At<CVector>(self, 0x34);
    CVector delta;
    VectorSub(&delta, &At<CVector>(self, 0xDC), &wanted);
    CVector velocity{0.0f, 0.0f, 0.0f};
    float deltaLength = Root((delta.x * delta.x + delta.y * delta.y) + delta.z * delta.z);
    CVector move = delta;
    if (!(deltaLength < g_A458C4)) {
        Vector_Normalise(&move);
        float most = g_A458C4;
        move.x = move.x * most;
        move.y = move.y * most;
        move.z = most * move.z;
        CVector rest;
        velocity = *VectorSub(&rest, &delta, &move);
    }
    uint8_t slide = (At<float>(self, 0x25C) != 0.0f && g_A458C0 != 0) ? 1 : 0;
    CVector tmp;
    At<CVector>(self, 0x6C) = *VectorSub(&tmp, &position, &lookAt);

    // the capsule the camera sits on the end of
    Capsule body{};
    Capsule_Set(&body, &lookAt, &position, At<float>(butes, 0x18));
    CVector from = lookAt;
    if (At<uint8_t>(self, 0x2B0)) {
        float along = At<float>(self, 0x2AC);
        CVector toCamera;
        VectorSub(&toCamera, &position, &lookAt);
        CVector offset{along * toCamera.x, toCamera.y * along, toCamera.z * along};
        CVector sum;
        from = *VectorAdd(&sum, &lookAt, &offset);
    }
    Capsule sweep{};
    Capsule_Set(&sweep, &from, &position, At<float>(butes, 0x18));

    float mostOpposed = -1.0f;   // how much two surfaces hit face each other
    float deepest = 0.0f;
    float pushedOut = 0.0f;
    int32_t count = 0;           // the contacts kept
    float blocked = 0.0f;
    int32_t oldHits = At<int32_t>(self, 0x510);
    At<int32_t>(self, 0x510) = 0;
    float radius = At<float>(butes, 0x18);
    float radiusSq = radius * radius;
    MarkTargetScanned();
    g_C1AE8A = 1;
    CapsuleQuery query;
    CapsuleQuery_Setup(&query, &sweep, 0x8F, colPoints, 0x20);
    query.entities = touchedEntities;
    query.numEntities = 0;
    query.maxEntities = 0x20;
    // (the remaster's seamless doors: just through one, the camera is still on the side left,
    // outside the new area's walls, which would pull it in: runflags.h)
    int32_t hits = re::SeamlessCameraFree() ? 0 : CapsuleQuery_Scan(&query);
    At<int32_t>(self, 0x510) = hits;
    for (int32_t i = 0; i < hits; i++) {
        const ColPoint& hit = colPoints[i];
        CVector point = hit.point;
        CVector normal = hit.normal;
        float depth = hit.depth;
        float overlap = body.radius - depth;
        if (std::fabs(depth) < 0.0001f) {
            // (touching: pushed back along the capsule, by what's past what it's
            // been pushed already)
            CVector toPoint;
            VectorSub(&toPoint, &hit.point, &position);
            float along = -((body.dir.y * toPoint.y + body.dir.x * toPoint.x) + body.dir.z * toPoint.z);
            if (pushedOut < along) {
                float extra = (along - pushedOut) + 0.05f;
                pushedOut = along;
                CVector back{-body.dir.x, -body.dir.y, -body.dir.z};
                float px = extra * back.x;
                float py = extra * back.y;
                float pz = extra * back.z;
                position.x = px + position.x;
                position.y = position.y + py;
                position.z = pz + position.z;
                if (slide) {
                    SlideAlong(&velocity, back);
                }
            }
            blocked = 1.0f;
            continue;
        }
        CVector fromStart;
        VectorSub(&fromStart, &point, &body.start);
        float distSq = (fromStart.y * fromStart.y + fromStart.x * fromStart.x) + fromStart.z * fromStart.z;
        if (radiusSq >= distSq) {
            continue;
        }
        uint16_t model = At<uint16_t>(hit.entity, 0x10E);
        if (model == 0xE7A || model == 0xE9E) {
            continue;
        }
        contactDepths[count] = overlap;
        contactNormals[count] = hit.normal;
        count++;
        float px = normal.x * overlap;
        float py = normal.y * overlap;
        float pz = normal.z * overlap;
        position.x = px + position.x;
        position.y = position.y + py;
        position.z = pz + position.z;
        if (slide) {
            SlideAlong(&velocity, normal);
        }
        // against the hits before this one
        for (int32_t j = 0; j < i; j++) {
            const ColPoint& other = colPoints[j];
            float otherDepth = other.depth;
            if (std::fabs(otherDepth) < 0.0001f) {
                continue;
            }
            float facing = -((other.normal.x * normal.x + other.normal.y * normal.y) + other.normal.z * normal.z);
            if (mostOpposed < facing) {
                mostOpposed = facing;
            }
            if (0.0f < facing) {
                float in = body.radius - otherDepth;
                if (in < overlap) {
                    in = overlap;
                }
                if (!(in < deepest)) {
                    deepest = in;
                }
            }
        }
        // and against last frame's
        for (int32_t k = 0; k < At<int32_t>(self, 0x504); k++) {
            const CVector& kept = At<CVector>(self, 0x304 + k * 0xC);
            float facing = -((kept.y * normal.y + kept.x * normal.x) + kept.z * normal.z);
            if (mostOpposed < facing) {
                mostOpposed = facing;
            }
            if (0.0f < facing) {
                float in = At<float>(self, 0x484 + k * 4);
                if (in < overlap) {
                    in = overlap;
                }
                if (!(in < deepest)) {
                    deepest = in;
                }
            }
        }
    }

    // the entities touched, against a sphere at the camera
    alignas(4) uint8_t sphere[0x64];   // a CColSphereQuery
    SphereQuery_Construct(sphere, &position, At<float>(butes, 0x18), 0, colPoints, 0x20);
    for (int32_t e = 0; e < query.numEntities; e++) {
        int32_t points = SphereQuery_ProcessEntity(sphere, touchedEntities[e]);
        At<int32_t>(self, 0x510) += points;
        for (int32_t j = 0; j < points; j++) {
            float depth = colPoints[j].depth;
            CVector normal = colPoints[j].normal;
            float px = depth * normal.x;
            float py = normal.y * depth;
            float pz = normal.z * depth;
            position.x = px + position.x;
            position.y = position.y + py;
            position.z = pz + position.z;
            if (slide) {
                SlideAlong(&velocity, normal);
            }
        }
    }
    g_C1AE8A = 0;
    EmptyThiscall(sphere);
    EmptyThiscall(&query);

    CVector moved;
    VectorAdd(&moved, &wanted, &move);
    CVector smoothed;
    At<CVector>(self, 0xDC) = *VectorAdd(&smoothed, &moved, &velocity);

    // how far it's pushed in, held for a few frames and decaying after
    if (!(0.0f < mostOpposed)) {
        mostOpposed = 0.0f;
    }
    float share = g_A458BC + (1.0f - g_A458BC) * deepest;
    float push = (share / body.radius + g_C3D2A0) * mostOpposed;
    int32_t& hold = At<int32_t>(self, 0x508);
    float& pushed = At<float>(self, 0x50C);
    if (hold != 0) {
        hold--;
    } else if (0.0f < pushed) {
        float less = pushed - g_A458B8;
        pushed = less;
        if (less < 0.0f) {
            pushed = 0.0f;
        }
    }
    if (0.0f < push) {
        hold = int32_t(g_C3D29C);
    }
    if (pushed < push) {
        pushed = push;
    }
    deepest = pushed;
    if (0.0f < deepest) {
        blocked = 1.0f;
    }
    for (int32_t k = 0; k < count; k++) {
        At<CVector>(self, 0x304 + k * 0xC) = contactNormals[k];
        At<float>(self, 0x484 + k * 4) = contactDepths[k];
    }
    At<int32_t>(self, 0x504) = count;

    // the flat direction: to the look-at when far enough, turned towards it (at most
    // 180 degrees scaled by +0x28, less the closer it is) when close, left alone when
    // on it
    CVector toCamera;
    VectorSub(&toCamera, &position, &lookAt);
    float flat = Root(toCamera.x * toCamera.x + toCamera.y * toCamera.y);
    CVector& direction = At<CVector>(self, 0xD0);
    if (g_A458B4 < flat) {
        direction = toCamera;
        direction.z = 0.0f;
        Vector_Normalise(&direction);
    } else if (g_A458B0 < flat) {
        float near = (flat - g_A458B0) / (g_A458B4 - g_A458B0);
        float speed = g_C3D298 + (1.0f - g_C3D298) * near;
        CVector want{toCamera.x, toCamera.y, 0.0f};
        Vector_Normalise(&want);
        float current = VecHeading(&direction);
        float wanted2 = VecHeading(&want);
        float turn = AngleDiff(current, wanted2);
        float most = float(double(g_A458AC) * kPi / 180.0 * double(At<float>(self, 0x28)) * double(speed));
        float least = -most;
        if (!(least < turn)) {
            turn = least;
        }
        if (!(turn < most)) {
            turn = most;
        }
        float s = float(std::sin(double(turn)));
        float c = float(std::cos(double(turn)));
        CVector turned{direction.x * c - s * direction.y, c * direction.y + direction.x * s, 0.0f};
        Vector_Normalise(&turned);
        direction = turned;
    }
    At<CVector>(self, 0x78) = direction;
    At<float>(self, 0x68) = At<float>(self, 0x20);
    FollowCameraBase_Unknown4FCD70(self + 0x68);

    // the height follows the camera being pushed up or down, within the butes' limits
    float& distance = At<float>(self, 0xC8);
    float& height = At<float>(self, 0xCC);
    float rise = position.z - At<float>(self, 0x3C);
    if (std::fabs(rise) > 0.001f) {
        if (0.0f < rise) {
            if (height < At<float>(butes, 0x74)) {
                height = rise + height;
            }
        } else if (rise < 0.0f && At<float>(butes, 0x70) < height) {
            height = rise + height;
        }
    }
    // the distance follows it being pulled in (blocked the more, the more)
    CVector fromOld;
    VectorSub(&fromOld, &position, &oldLookAt);
    float lost = Root(fromOld.x * fromOld.x + fromOld.y * fromOld.y) - distance;
    if (lost < -0.001f) {
        distance = distance + lost;
        if (blocked != 1.0f) {
            if (lost < -g_A458A8) {
                blocked = 1.0f;
            } else {
                blocked = -lost / g_A458A8;
            }
        }
    }
    float& blockedAmount = At<float>(self, 0x10C);
    if (blockedAmount < blocked) {
        blockedAmount = blocked;
        At<uint32_t>(self, 0x110) = timeInMs;
    }
    if (blockedAmount != 0.0f) {
        At<uint8_t>(self, 0x114) = 1;
    }
    // pushed in: the camera pulled in along the capsule (less when close)
    if (flat < 0.75f) {
        float closeness = flat - 0.25f;
        if (!(closeness > 0.0f)) {
            closeness = 0.0f;
        }
        deepest = (closeness + closeness) * deepest;
    }
    if (0.0f < deepest) {
        float pull = float(std::pow(double(deepest), double(At<float>(butes, 0xA0)))) * At<float>(butes, 0x9C);
        if (!(pull < 1.0f)) {
            pull = 1.0f;
        }
        float flatDir = Root(body.dir.x * body.dir.x + body.dir.y * body.dir.y);
        distance = distance - flatDir * pull;
        height = height - pull * body.dir.z;
    }
    // turning fast: pulled in too (past the butes' +0xB4; faded in over 1.25..1.75 of
    // flat distance when +0x31)
    float turnRate = std::fabs(At<float>(self, 0x88));
    float threshold = At<float>(butes, 0xB4);
    if (threshold < turnRate) {
        float over = (turnRate - threshold) / (1.0f - threshold);
        if (At<uint8_t>(self, 0x31) && flat < g_A458A4) {
            float fade = (flat - g_A458A0) / (g_A458A4 - g_A458A0);
            if (!(0.0f < fade)) {
                fade = 0.0f;
            }
            over = fade * over;
        }
        if (over < 0.0f) {
            over = 0.0f;
        } else if (1.0f < over) {
            over = 1.0f;
        }
        over = At<float>(butes, 0xB8) * over;
        float flatDir = Root(body.dir.x * body.dir.x + body.dir.y * body.dir.y);
        float split = g_A4589C;
        distance = distance - ((flatDir * over) * split) * 2.0f;
        height = height - ((over * body.dir.z) * (1.0f - split)) * 2.0f;
    }

    // how fast the smoothed look-at and position follow (slower while pushed about)
    uint8_t closeUp = At<uint8_t>(self, 0x115);
    float lookAtRate = closeUp ? 60.0f : At<float>(butes, 0x3C);
    float lookAtBlend = At<float>(self, 0x28) * lookAtRate;
    if (!(lookAtBlend < 1.0f)) {
        lookAtBlend = 1.0f;
    }
    float positionRate = closeUp ? 60.0f : At<float>(butes, 0x40);
    float positionBlend = At<float>(self, 0x28) * positionRate;
    if (!(positionBlend < 1.0f)) {
        positionBlend = 1.0f;
    }
    if (At<uint8_t>(butes, 0xEC)) {
        float oldHitsF = float(oldHits);
        float trouble = ((blockedAmount + pushed) + float(At<int32_t>(self, 0x510))) + oldHitsF;
        if (trouble > 0.0f) {
            float before = (delta.x * delta.x + delta.y * delta.y) + delta.z * delta.z;
            CVector now;
            VectorSub(&now, &At<CVector>(self, 0xDC), &wanted);
            float after = (now.y * now.y + now.x * now.x) + now.z * now.z;
            float closer = before - after;
            float speedSq = 0.0f;
            uint8_t* target = cameraTarget;
            if (target && (At<uint8_t>(target, 0x108) & 7) == 3) {
                float vx = At<float>(target, 0x124);
                float vy = At<float>(target, 0x128);
                speedSq = vx * vx + vy * vy;
            }
            float a = closer + 0.4f;
            float b = (0.2f - speedSq) / 0.2f;
            float slowLookAt = 1.0f - (((At<float>(butes, 0xF4) * 0.1f) * a) * b) * trouble;
            if (slowLookAt < 0.05f) {
                slowLookAt = 0.05f;
            } else if (1.0f < slowLookAt) {
                slowLookAt = 1.0f;
            }
            lookAtBlend = slowLookAt * lookAtBlend;
            float slowPosition = 1.0f - ((a * (0.1f * At<float>(butes, 0xF0))) * b) * trouble;
            if (slowPosition < 0.05f) {
                slowPosition = 0.05f;
            } else if (1.0f < slowPosition) {
                slowPosition = 1.0f;
            }
            positionBlend = slowPosition * positionBlend;
        }
    }
    CVector& smoothLookAt = At<CVector>(self, 0x124);
    CVector& smoothPosition = At<CVector>(self, 0x118);
    CVector& smooth130 = At<CVector>(self, 0x130);
    CVector step;
    VectorSub(&step, &lookAt, &smoothLookAt);
    float sx = lookAtBlend * step.x;
    float sy = step.y * lookAtBlend;
    float sz = lookAtBlend * step.z;
    smoothLookAt.x = sx + smoothLookAt.x;
    smoothLookAt.y = smoothLookAt.y + sy;
    smoothLookAt.z = smoothLookAt.z + sz;
    VectorSub(&step, &position, &smoothPosition);
    sx = positionBlend * step.x;
    sy = step.y * positionBlend;
    sz = positionBlend * step.z;
    smoothPosition.x = smoothPosition.x + sx;
    smoothPosition.y = smoothPosition.y + sy;
    smoothPosition.z = sz + smoothPosition.z;
    VectorSub(&step, &At<CVector>(self, 0x4C), &smooth130);
    sx = lookAtBlend * step.x;
    sy = step.y * lookAtBlend;
    sz = lookAtBlend * step.z;
    smooth130.x = smooth130.x + sx;
    smooth130.y = sy + smooth130.y;
    smooth130.z = smooth130.z + sz;
    int32_t hitsNow = At<int32_t>(self, 0x510);
    VehicleCameraController_Unknown509F50(At<void*>(self, 0x27C), smoothPosition, smoothLookAt, distance, hitsNow);
    if (hitsNow != 0) {
        At<float>(self, 0x294) = 0.0f;
        At<uint32_t>(self, 0x298) = timeInMs;
    }
}
FOLLOWCAM4_REVERSED("FollowCameraBase::Collide_5028F0", 0x005028F0, RE_BODY(&FollowCameraBase_CollideOurs), nullptr, nullptr, re::Mode::Original);

// 0x5044A0: puts the camera at `position` looking at `lookAt` (with `sweep`, pulled
// in to where a sweep from the look-at hits), everything eased reset to there
void __fastcall FollowCameraController3_PlaceOurs(uint8_t* self, re::Edx, CVector lookAt, CVector position, int sweep) {
    // (the remaster's seamless doors, runflags.h: just through one, the camera goes on from where
    // it was, carried through the doorway, and isn't pulled in by the new area's walls)
    re::SeamlessCameraPlace(&position.x);
    if (re::Flag(sweep) && !re::SeamlessCameraFree()) {
        g_C1AE8A = 1;
        alignas(4) uint8_t test[0x48];   // (the sweep; +0x3C how far along the hit is)
        Sweep_Construct(test, &lookAt, &position, At<float>(Butes(self), 0x18), 0x8B, colPoints, reinterpret_cast<void*>(0x0045EE90), 0, 0, 0);
        At<uint16_t>(cameraTarget, 0x10A) = uint16_t(currentScanCode + 1);
        uint8_t* target = cameraTarget;
        if ((At<uint8_t>(target, 0x108) & 7) == 3 && At<uint8_t*>(target, 0x1554)) {
            At<uint16_t>(At<uint8_t*>(target, 0x1554), 0x10A) = uint16_t(currentScanCode + 1);
        }
        if (Sweep_Test(test) != 0) {
            float along = At<float>(test, 0x3C);
            CVector toCamera;
            VectorSub(&toCamera, &position, &lookAt);
            CVector part{along * toCamera.x, toCamera.y * along, toCamera.z * along};
            CVector sum;
            position = *VectorAdd(&sum, &lookAt, &part);
        }
        g_C1AE8A = 0;
        EmptyThiscall(test);
    }
    uint8_t* fight = self + 0x160;
    At<float>(fight, 0x34) = 0.0f;
    At<float>(fight, 0x30) = 0.0f;
    At<int32_t>(fight, 0x28) = 0;
    At<uint8_t>(fight, 0x2C) = 0;
    At<float>(fight, 0xF8) = 0.0f;
    At<int32_t>(fight, 0xEC) = 0;
    At<uint8_t>(fight, 0xF4) = 0;
    At<int32_t>(fight, 0xF0) = 0;
    FollowCameraFight_Unknown4FE980(fight);
    FollowCameraFight_Unknown4FEB90(fight, fight + 4, fight + 0x10);
    At<float>(self, 0x284) = 0.0f;
    At<float>(self, 0x294) = 0.0f;
    At<uint32_t>(self, 0x288) = timeInMs;
    At<uint32_t>(self, 0x298) = 0;
    VCall<void>(At<void*>(self, 0x27C), 3);
    At<CVector>(self, 0xB0) = lookAt;
    At<CVector>(self, 0xA4) = position;
    At<CVector>(self, 0xBC) = lookAt;
    FollowInterp_Set(self + 0xF4, 0.0f);
    At<float>(self, 0x10C) = 1.0f;
    At<CVector>(self, 0xDC) = position;
    At<uint32_t>(self, 0x110) = timeInMs;
    At<uint8_t>(self, 0x114) = 1;
    CVector toCamera;
    VectorSub(&toCamera, &At<CVector>(self, 0xA4), &At<CVector>(self, 0xB0));
    At<CVector>(self, 0xD0) = toCamera;
    At<float>(self, 0xD8) = 0.0f;
    Vector_Normalise(&At<CVector>(self, 0xD0));
    At<float>(self, 0xC8) = Root(toCamera.x * toCamera.x + toCamera.y * toCamera.y);
    At<float>(self, 0xCC) = toCamera.z;
    At<float>(self, 0x9C) = 0.2f;
    At<float>(self, 0xA0) = 0.5f;
    At<CVector>(self, 0x118) = position;
    At<CVector>(self, 0x124) = lookAt;
    At<CVector>(self, 0x130) = lookAt;
    At<CVector>(self, 0x08) = lookAt;
    At<CVector>(self, 0x90) = lookAt;
    At<float>(self, 0x274) = At<float>(Butes(self), 0x90);
    At<int32_t>(self, 0x504) = 0;
    At<int32_t>(self, 0x510) = 0;
    At<float>(self, 0x50C) = 0.0f;
    At<int32_t>(self, 0x508) = 0;
    At<float>(self, 0x25C) = 0.0f;
    At<uint8_t>(self, 0x268) = 0;
    At<uint8_t>(self, 0x2B0) = 0;
    At<uint8_t>(self, 0x2C0) = 0;
    if (g_C3CE1C) {
        CleanUpOldReference(g_C3CE1C, reinterpret_cast<re::Ptr32<void>*>(&g_C3CE1C));
        g_C3CE1C = nullptr;
    }
    g_C3CD70 = 1;
    g_C3CD78 = At<float>(cameraSettings, 0x5C);
    g_C3CD74 = At<float>(cameraSettings, 0x58);
    g_C3CD7C = At<float>(Butes(self), 0xE4);
    g_C3CD80 = At<float>(Butes(self), 0xE0);
}
FOLLOWCAM4_REVERSED("FollowCameraController3::Place_5044A0", 0x005044A0, RE_BODY(&FollowCameraController3_PlaceOurs), nullptr, nullptr, re::Mode::Original);

// 0x5048E0: the camera behind `from` along `dir` (at the butes' distance: close-up
// +0x58, +0x116's +0x5C, else +0x44), raised by 0x4FE110, placed with a sweep
void __fastcall FollowCameraController3_PlaceBehind(uint8_t* self, re::Edx, CVector from, CVector dir) {
    At<CVector>(self, 0x08) = from;
    At<CVector>(self, 0x90) = from;
    float back;
    if (At<uint8_t>(self, 0x115)) {
        back = At<float>(Butes(self), 0x58);
    } else if (At<uint8_t>(self, 0x116)) {
        back = At<float>(Butes(self), 0x5C);
    } else {
        back = At<float>(Butes(self), 0x44);
    }
    float up = FollowCameraBase_GetDistance(self);
    float zero = float(0.0 * double(up));
    CVector raise{zero, zero, up};
    CVector behind{dir.x * back, dir.y * back, back * dir.z};
    CVector t;
    VectorSub(&t, &from, &behind);
    CVector sum;
    dir = *VectorAdd(&sum, &t, &raise);
    FollowCameraController3_Place(self, from, dir, 1);
}
FOLLOWCAM4_REVERSED("FollowCameraController3::PlaceBehind_5048E0", 0x005048E0, RE_BODY(&FollowCameraController3_PlaceBehind), nullptr, nullptr, re::Mode::Original);

// 0x504A40: FollowCameraFight's constructor (then its reset)
void* __fastcall FollowCameraFight_ConstructOurs(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = 0x009145EC;
    for (size_t offset : {0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C, 0x20, 0x24, 0x38, 0x3C, 0x40, 0x44, 0x48, 0x4C,
                          0x5C, 0x60, 0x64, 0x68, 0x6C, 0x70, 0x74, 0x78, 0x7C, 0x84, 0x88, 0x8C, 0x90, 0x94, 0x98,
                          0xAC, 0xB0, 0xB4, 0xB8, 0xBC, 0xC0, 0xC4, 0xC8, 0xCC, 0xD4, 0xD8, 0xDC, 0xE0, 0xE4, 0xE8}) {
        At<float>(self, offset) = 0.0f;
    }
    At<int32_t>(self, 0xEC) = 0;
    At<float>(self, 0xF8) = 0.0f;
    At<uint8_t>(self, 0xF4) = 0;
    At<int32_t>(self, 0xF0) = 0;
    At<int32_t>(self, 0x50) = 0;
    At<uint8_t>(self, 0x54) = 0;
    At<int32_t>(self, 0x58) = -1;
    FollowCameraFight_Reset(self);
    return self;
}
FOLLOWCAM4_REVERSED("FollowCameraFight::FollowCameraFight_504A40", 0x00504A40, RE_BODY(&FollowCameraFight_ConstructOurs), nullptr, nullptr, re::Mode::Original);

// 0x504B50: FollowCameraBase's constructor (its parts: the eased value, the fight
// part, the vehicle part with a VehicleCameraController of its own, the contacts)
void* __fastcall FollowCameraBase_ConstructOurs(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = 0x00914600;
    for (size_t offset : {0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C, 0x34, 0x38, 0x3C, 0x40, 0x44, 0x48, 0x4C, 0x50, 0x54,
                          0x58, 0x6C, 0x70, 0x74, 0x78, 0x7C, 0x80, 0x68, 0x84, 0x88, 0x90, 0x94, 0x98, 0xA4, 0xA8,
                          0xAC, 0xB0, 0xB4, 0xB8, 0xBC, 0xC0, 0xC4, 0xD0, 0xD4, 0xD8, 0xDC, 0xE0, 0xE4}) {
        At<float>(self, offset) = 0.0f;
    }
    FollowInterp_Construct(self + 0xF4);
    At<uint8_t>(self, 0x115) = 0;
    At<uint8_t>(self, 0x116) = 0;
    for (size_t offset = 0x118; offset <= 0x15C; offset += 4) {
        At<float>(self, offset) = 0.0f;
    }
    FollowCameraFight_Construct(self + 0x160);
    At<uint8_t>(self, 0x268) = 0;
    At<float>(self, 0x25C) = 0.0f;
    At<uint8_t>(self, 0x269) = 0;
    At<int32_t>(self, 0x26C) = 0;
    At<float>(self, 0x274) = 0.0f;
    At<uint32_t>(self, 0x278) = 0x009145F4;
    void* driven = Malloc(0x120);
    At<void*>(self, 0x27C) = driven ? VehicleCameraController_Construct(driven) : nullptr;
    At<float>(self, 0x2AC) = 0.0f;
    At<uint8_t>(self, 0x2B0) = 0;
    for (size_t offset = 0x2D4; offset <= 0x300; offset += 4) {
        At<float>(self, offset) = 0.0f;
    }
    At<uint8_t>(self, 0x2C0) = 0;
    Contacts_Construct(self + 0x304);
    At<float>(self, 0x9C) = 0.2f;
    At<uint8_t>(self, 0x51C) = 0;
    At<float>(self, 0xA0) = 0.5f;
    return self;
}
FOLLOWCAM4_REVERSED("FollowCameraBase::FollowCameraBase_504B50", 0x00504B50, RE_BODY(&FollowCameraBase_ConstructOurs), nullptr, nullptr, re::Mode::Original);

// 0x504D90 (vtable[7]): the parts' updates, the collision pass, then this frame's
// smoothed vectors kept as last frame's (the sub-step published meanwhile)
void __fastcall FollowCameraBase_ProcessOurs(uint8_t* self, re::Edx) {
    g_C3D0D8 = At<int32_t>(self, 0x2C);
    FollowCameraBase_Unknown4FFEC0(self + 0x2AC);
    FollowCameraBase_Unknown4FFF50(self + 0x2C0, At<CVector>(self, 0x08), At<CVector>(self, 0xB0), At<CVector>(self, 0xA4));
    FollowCameraBase_Unknown5003C0(self);
    FollowCameraBase_Unknown500B40(self);
    FollowCameraBase_Collide(self);
    At<CVector>(self, 0x13C) = At<CVector>(self, 0x118);
    At<CVector>(self, 0x148) = At<CVector>(self, 0x124);
    At<CVector>(self, 0x154) = At<CVector>(self, 0x130);
    g_C3D0D8 = 0;
}
FOLLOWCAM4_REVERSED("FollowCameraBase::vf7_504D90", 0x00504D90, RE_BODY(&FollowCameraBase_ProcessOurs), nullptr, nullptr, re::Mode::Original);

// ---- OffsetCameraController
// +0x08, +0x0C the offset (the butes' +0x4C), +0x18 (the butes' +0x34), +0x28..+0x44,
// +0x110 its attribute set, +0x114/+0x118 bones looked up by the butes' +0x58/+0x5C,
// +0x11C..+0x138, +0x15C..+0x164, +0x16C, +0x170.

// 0x504ED0: the constructor (the default attribute set)
void* __fastcall OffsetCameraController_ConstructOurs(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = 0x00914668;
    for (size_t offset : {0x0C, 0x10, 0x14, 0x18, 0x1C, 0x20, 0x28, 0x2C, 0x30, 0x15C, 0x160, 0x164}) {
        At<float>(self, offset) = 0.0f;
    }
    uint8_t* butes = defaultOffsetButes;
    At<uint8_t*>(self, 0x110) = butes;
    At<CVector>(self, 0x0C) = At<CVector>(butes, 0x4C);
    At<CVector>(self, 0x18) = At<CVector>(butes, 0x34);
    At<uint8_t>(self, 0x44) = 0;
    At<int32_t>(self, 0x114) = 0;
    At<int32_t>(self, 0x118) = 0;
    At<uint8_t>(self, 0x16C) = 0;
    return self;
}
FOLLOWCAM4_REVERSED("OffsetCameraController::OffsetCameraController_504ED0", 0x00504ED0, RE_BODY(&OffsetCameraController_ConstructOurs), nullptr, nullptr, re::Mode::Original);

// a bone by the butes' name `name`
void* LookUpBone(int name) {
    uint32_t names[3];
    uint32_t path[2];
    Name_Copy(&names[0], name_1B6531C);
    Name_Copy(&names[1], name_1B6535C);
    Name_Construct(&names[2], name);
    Path_Build(path, 3, names);
    void* found = Butes_Find(path, root_1B65360);
    for (int i = 2; i >= 0; i--) {
        EmptyThiscall(&names[i]);
    }
    return found;
}

// 0x504F90: +0x114/+0x118 by the butes' +0x58/+0x5C (+0x118 falls back to +0x114)
void __fastcall OffsetCameraController_LookUpBonesOurs(uint8_t* self, re::Edx) {
    uint8_t* butes = At<uint8_t*>(self, 0x110);
    if (At<int32_t>(butes, 0x58) == 0) {
        At<void*>(self, 0x114) = nullptr;
    } else {
        At<void*>(self, 0x114) = LookUpBone(At<int32_t>(butes, 0x58));
    }
    butes = At<uint8_t*>(self, 0x110);
    if (At<int32_t>(butes, 0x5C) == 0) {
        At<void*>(self, 0x118) = At<void*>(self, 0x114);
        return;
    }
    void* found = LookUpBone(At<int32_t>(butes, 0x5C));
    At<void*>(self, 0x118) = found;
    if (!found) {
        At<void*>(self, 0x118) = At<void*>(self, 0x114);
    }
}
FOLLOWCAM4_REVERSED("OffsetCameraController::LookUpBones_504F90", 0x00504F90, RE_BODY(&OffsetCameraController_LookUpBonesOurs), nullptr, nullptr, re::Mode::Original);

// 0x5050D0: the attribute set by name (0 or not found: the default), its bones, the
// globals it sets (0xC3B6F8 the hashed +0x80, 0xC3B6FC +0x7C, when +0x7B), then
// processed once (vtable[7]); always 1
uint8_t __fastcall OffsetCameraController_SetButesOurs(uint8_t* self, re::Edx, const int32_t* name) {
    if (*name == 0) {
        At<uint8_t*>(self, 0x110) = defaultOffsetButes;
    } else {
        uint32_t names[2];
        uint32_t path[2];
        Name_Copy(&names[0], name_1B653F4);
        Name_Copy(&names[1], name);
        Path_Build(path, 2, names);
        uint8_t* found = static_cast<uint8_t*>(Butes_Find(path, root_1B653F0));
        At<uint8_t*>(self, 0x110) = found;
        if (!found) {
            At<uint8_t*>(self, 0x110) = defaultOffsetButes;
        }
        for (int i = 1; i >= 0; i--) {
            EmptyThiscall(&names[i]);
        }
    }
    Clear_C3B708();
    OffsetCameraController_LookUpBones(self);
    uint8_t* butes = At<uint8_t*>(self, 0x110);
    if (At<uint8_t>(butes, 0x7B)) {
        int32_t hash = int32_t(HashName(At<const char*>(butes, 0x80)));
        g_C3B700 = 1;
        g_C3B6FC = At<int32_t>(At<uint8_t*>(self, 0x110), 0x7C);
        g_C3B6F8 = hash;
    } else {
        g_C3B700 = 0;
        g_C3B6FC = -1;
        g_C3B6F8 = -1;
    }
    At<uint8_t>(self, 0x11C) = 1;
    if (At<uint8_t>(At<uint8_t*>(self, 0x110), 0x60)) {
        At<uint8_t>(self, 8) = 1;
    }
    VCall<void>(self, 7);
    return 1;
}
FOLLOWCAM4_REVERSED("OffsetCameraController::SetButes_5050D0", 0x005050D0, RE_BODY(&OffsetCameraController_SetButesOurs), nullptr, nullptr, re::Mode::Original);

// 0x5051E0: back to the butes' defaults (the globals 0x5050D0 sets cleared)
void __fastcall OffsetCameraController_ClearOurs(uint8_t* self, re::Edx) {
    uint8_t* butes = At<uint8_t*>(self, 0x110);
    At<float>(self, 0x34) = 0.0f;
    At<float>(self, 0x38) = 0.0f;
    At<uint8_t>(self, 8) = 1;
    At<float>(self, 0x3C) = At<float>(butes, 0x28);
    At<uint8_t>(self, 0x44) = 1;
    At<float>(self, 0x40) = At<float>(butes, 0x28);
    At<uint8_t>(self, 0x11C) = 1;
    At<uint8_t>(self, 0x134) = 0;
    At<float>(self, 0x124) = 0.0f;
    At<int32_t>(self, 0x170) = 0;
    At<float>(self, 0x120) = 0.0f;
    At<float>(self, 0x128) = 0.0f;
    At<float>(self, 0x12C) = 0.5f;
    At<float>(self, 0x130) = 0.5f;
    At<float>(self, 0x138) = 0.0f;
    Clear_C3B708();
    g_C3B700 = 0;
    g_C3B6FC = -1;
    g_C3B6F8 = -1;
    OffsetCameraController_LookUpBones(self);
}
FOLLOWCAM4_REVERSED("OffsetCameraController::Clear_5051E0", 0x005051E0, RE_BODY(&OffsetCameraController_ClearOurs), nullptr, nullptr, re::Mode::Original);

// ---- 0x505270: a direction from a heading (+4) and a pitch (+0x24, from straight
// up), unit length (whose: not pinned down; Ghidra puts it with BalanceMeterTrack)
CVector* __fastcall HeadingPitch_GetDirection(uint8_t* self, re::Edx, CVector* out) {
    VecFromHeading(out, At<float>(self, 4));
    float fromUp = float(kHalfPi - double(At<float>(self, 0x24)));
    out->z = float(std::tan(double(fromUp)));
    Vector_Normalise(out);
    return out;
}
FOLLOWCAM4_REVERSED("BalanceMeterTrack::GetDirection_505270", 0x00505270, RE_BODY(&HeadingPitch_GetDirection), nullptr, nullptr, re::Mode::Original);

} // namespace
