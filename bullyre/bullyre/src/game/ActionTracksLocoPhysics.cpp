// The action tree's locomotion tracks' physics: the LocomotionTrack's start and end and
// its foot probe (the ground's grip pushing the ped's velocity and spin), and the
// LocomotionJumpTrack's start and update (the jump arc, air steering and the skateboard's
// lean). 0x60C710, 0x60C9F0, 0x60CAE0, 0x60D790, 0x60D9E0.
// The LocomotionTrack's layout is described in TracksLocomotion.cpp.

#include <cmath>
#include <cstdint>
#include <cstring>

#include "game/ActionTracksCommon.h"
#include "game/TrackMath.h"
#include "reversed.h"

using namespace actc;

namespace {

constexpr uint32_t kVtLocomotionTrack = 0x00930EC4;

GAME_VAR(uint8_t*, motionPool_20C9028, 0x020C9028);   // AM motion tracks: +0 used, +4 free, +8 size, +0x14 list
GAME_VAR(float, locoBlend_B0B13C, 0x00B0B13C);
GAME_VAR(int32_t, updateMoveOnEnd_1B65038, 0x01B65038);
GAME_VAR(float, groundGrip_1B65048, 0x01B65048);
GAME_VAR(uint8_t, currentChapter, 0x00A147E0);
GAME_ARRAY(int32_t, chapterSeason, 0x00A147EC);
GAME_VAR(uint8_t, altSteering, 0x00A14925);
inline void* const padA45A30 = reinterpret_cast<void*>(0x00A45A30);
inline const float* const yAxis = reinterpret_cast<const float*>(0x009E33D0);

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __thiscall, Track_Construct, 0x0061A880, uint8_t* track);
GAME_FN(uint8_t*, __thiscall, MotionTrack_Construct, 0x007315C0, uint8_t* track);
GAME_FN(void, __thiscall, AnimManager_BlendOut, 0x006BD480, void* manager, uint8_t* blend, uint8_t* anim,
        float time);
GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_FN(V3*, __thiscall, Placement_GetRight, 0x00473600, uint8_t* placement, V3* out);
GAME_FN(void, __thiscall, Ped_UpdateMoveDirection, 0x00474570, uint8_t* ped);
GAME_FN(V3*, __cdecl, CrossProduct, 0x00414730, V3* out, const void* a, const void* b);
GAME_FN(V3*, __cdecl, VectorAdd, 0x004129A0, V3* out, const float* a, const float* b);
GAME_FN(float, __cdecl, Surface_GetAdhesiveLimit, 0x0045B500, const void* colPoint);
GAME_FN(float, __cdecl, Surface_GetWetMultiplier, 0x0045B530, uint32_t surface);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(void, __thiscall, Physical_SetMoveSpeed, 0x0046A7E0, uint8_t* physical, float x, float y, float z);
GAME_FN(void, __thiscall, Physical_ApplyTurnForce, 0x0046A950, uint8_t* physical, float fx, float fy, float fz,
        float px, float py, float pz);
GAME_FN(void, __thiscall, Physical_ApplyTurnSpeed, 0x00469DC0, uint8_t* physical);
GAME_FN(void, __thiscall, Matrix_Reorthogonalise, 0x00413A80, uint8_t* matrix);
GAME_FN(float, __cdecl, ExpSeries, 0x00607420, float x);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const V3* v);
GAME_FN(float, __cdecl, TurnHeading, 0x00608380, float x, float y, float z, float toX, float toY, float toZ,
        float accel, float max, float rate, float* speed);
GAME_FN(uint8_t, __cdecl, VectorNotEqual, 0x004305F0, const void* a, const void* b);
GAME_FN(uint8_t*, __thiscall, Placeable_GetMatrix, 0x0046DD20, uint8_t* placeable);
GAME_FN(void, __cdecl, Matrix_Invert, 0x00413770, const void* matrix, void* out);
GAME_FN(V3*, __cdecl, Matrix_Multiply3x3, 0x0041C490, V3* out, const void* matrix, const void* v);
GAME_FN(uint8_t, __thiscall, Pad_50DF50, 0x0050DF50, void* pad);
GAME_FN(int32_t, __cdecl, Pad_GetActive, 0x00738070);
GAME_FN(uint8_t, __cdecl, Pad_738500, 0x00738500, int32_t pad);
GAME_FN(void, __thiscall, Ped_SetHeading, 0x004734C0, uint8_t* ped, float heading);

// a motion track from the pool (none when it is empty or too small)
uint8_t* NewMotionTrack() {
    uint8_t* pool = motionPool_20C9028;
    bool room = At<int32_t>(pool, 8) >= 0xA8;
    if (At<int32_t>(pool, 4) > 0 && room) {
        At<int32_t>(pool, 4) += -1;
        At<int32_t>(pool, 0) += 1;
        uint8_t* item = At<re::Ptr32<uint8_t>*>(pool, 0x14)[At<int32_t>(pool, 4)];
        if (item) {
            return MotionTrack_Construct(item);
        }
    }
    return nullptr;
}

// a placeable's right vector: its matrix's, else its placement's
V3 RightOf(uint8_t* e) {
    V3 r;
    const float* m = At<float*>(e, 0x14);
    if (m) {
        r = {m[0], m[1], m[2]};
    } else {
        Placement_GetRight(e + 4, &r);
    }
    return r;
}

inline float LengthSq(const V3& v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

// 0x60C710 (thiscall): LocomotionTrack::Start: the stick map's validity (+0x7C), a motion
// track (the owner's +0x18) for the ped holding a copy of this track, blending in over
// +0x44 (negative: the default), the steering state reset, the facing speed (+0x90) and
// side (+0x98); the player's mode (+0x4C 1) lifts the ped off the ground state and
// runs a first update when the track has a length
uint8_t __fastcall LocomotionTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x80) = owner;
    self[0x7C] = (0.0f < At<float>(self, 0x24) && At<float>(self, 0x20) >= 0.0f && At<float>(self, 0x18) >= 0.0f &&
                  At<float>(self, 0x1C) >= 0.0f)
                     ? 1
                     : 0;
    uint8_t* motion = NewMotionTrack();
    At<uint8_t*>(At<uint8_t*>(self, 0x80), 0x18) = motion;
    At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x80), 0x18), 0x48) = OwnerPed(At<uint8_t*>(self, 0x80));
    uint8_t* copy = static_cast<uint8_t*>(Malloc(0xA4));
    if (copy) {
        Track_Construct(copy);
        At<uint32_t>(copy, 0) = kVtLocomotionTrack;
        At<float>(copy, 0x94) = 0.0f;
        At<float>(copy, 0x98) = 0.0f;
        At<float>(copy, 0x9C) = 0.0f;
    }
    // (the game copies over it unchecked: a failed allocation writes to null)
    std::memcpy(copy, self, 0xA4);
    At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x80), 0x18), 0x4C) = copy;
    At<uint8_t>(At<uint8_t*>(self, 0x80), 0x1C) = 0;
    float t = At<float>(self, 0x44);
    uint8_t* m = At<uint8_t*>(At<uint8_t*>(self, 0x80), 0x18);
    if (!(t >= 0.0f)) {   // (negative or NaN: the game tests the sign flag)
        t = locoBlend_B0B13C;
    }
    if (t == 0.0f) {
        At<float>(m, 0x10) = 0.0f;
        At<float>(m, 0x14) = 0.0f;
        At<float>(m, 0xC) = 1.0f;
    } else {
        float blendIn = -t;
        At<float>(m, 0x10) = blendIn;
        At<float>(m, 0x14) = blendIn;
        At<float>(m, 0xC) = 0.0f;
    }
    At<float>(self, 0x88) = 0.0f;
    At<float>(self, 0x8C) = 0.0f;
    At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x80)), 0x147C) = 0;
    owner = At<uint8_t*>(self, 0x80);
    V3 f;
    V3* fwd = Placeable_GetForward(At<uint8_t*>(At<uint8_t*>(owner, 0x18), 0x48), &f);
    uint8_t* ped = At<uint8_t*>(At<uint8_t*>(owner, 0x18), 0x48);
    float d = At<float>(ped, 0x128) * fwd->y + fwd->x * At<float>(ped, 0x124) + At<float>(ped, 0x12C) * fwd->z;
    At<float>(self, 0x94) = 0.0f;
    At<float>(self, 0x98) = At<int32_t>(OwnerPed(owner), 0x1488) != 0 ? -1.0f : 1.0f;
    At<float>(self, 0x9C) = 0.0f;
    At<float>(self, 0x90) = d;
    self[0xA0] = 0;
    ped = At<uint8_t*>(At<uint8_t*>(owner, 0x18), 0x48);
    if (self[0x4C] == 1) {
        At<int32_t>(ped, 0x1334) = 0;
        At<int32_t>(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x80), 0x18), 0x48), 0x1474) = 1;
        At<int32_t>(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x80), 0x18), 0x48), 0x1484) = self[0x28] ? 0 : 1;
        float span = std::fabs(At<float>(self, 0xC) - At<float>(self, 0x10));
        if (span > 0.001f) {
            reinterpret_cast<uint8_t(__thiscall*)(uint8_t*, float)>(static_cast<void*>(At<re::Ptr32<void>*>(self, 0)[3]))(self, timeStep);
        }
        return 1;
    }
    At<float>(ped, 0x1D5C) = 0.0f;
    return 1;
}

// 0x60C9F0 (thiscall): LocomotionTrack::End: the ped back on the ground (its move
// direction refreshed unless +0x28 when asked); the motion track released when the owner
// did not keep it, else blended out over +0x48 (negative: 0.5) when it has a slot (+0x40)
uint8_t __fastcall LocomotionTrack_End(uint8_t* self, re::Edx) {
    At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x80)), 0x1334) = 1;
    At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x80)), 0x1474) = 0;
    At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x80)), 0x1484) = 0;
    if (!self[0x28] && updateMoveOnEnd_1B65038 != 0) {
        Ped_UpdateMoveDirection(OwnerPed(At<uint8_t*>(self, 0x80)));
    }
    uint8_t* owner = At<uint8_t*>(self, 0x80);
    uint8_t* blend = At<uint8_t*>(OwnerPed(owner), 0x114);
    int32_t slot = At<int32_t>(self, 0x40);
    uint8_t* motion = At<uint8_t*>(owner, 0x18);
    if (motion && !owner[0x1C]) {
        reinterpret_cast<void(__thiscall*)(uint8_t*)>(static_cast<void*>(At<re::Ptr32<void>*>(motion, 0)[1]))(motion);
        At<uint8_t*>(At<uint8_t*>(self, 0x80), 0x18) = nullptr;
        return 1;
    }
    if (slot > 0 && At<uint8_t*>(At<uint8_t*>(self, 0x80), 0x18)) {
        float t = At<float>(self, 0x48);
        if (0.0f > t) {
            t = 0.5f;
        }
        AnimManager_BlendOut(animManager, blend, At<uint8_t*>(At<uint8_t*>(self, 0x80), 0x18), t);
    }
    At<uint8_t*>(At<uint8_t*>(self, 0x80), 0x18) = nullptr;
    return 1;
}

// 0x60CAE0 (thiscall): LocomotionTrack's foot probe: the ground at a contact point
// (offset from the ped) grips it along forward (braking, +0x74 the stop angle) and side
// (+0x50 / +0x54 the player's brake / push, +0x58 the grip, +0x5C..+0x68 its response,
// +0x78 the least brake): the ped's velocity set and its spin pushed about the point
// (second: the other foot, skipped while pushing)
void __fastcall LocomotionTrack_FootProbe(uint8_t* self, re::Edx, float fx, float fy, float fz, float sx, float sy,
                                          float sz, float ox, float oy, float oz, int second) {
    V3 offset = {ox, oy, oz};
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x80));
    const float* velocity = &At<float>(ped, 0x124);
    V3 cross;
    V3 point;
    VectorAdd(&point, &CrossProduct(&cross, &At<float>(ped, 0x130), &offset)->x, velocity);
    float sq = point.x * point.x + point.y * point.y + point.z * point.z;
    if (sq < 1e-5f) {
        point = {velocity[0], velocity[1], velocity[2]};
    }
    float g = groundGrip_1B65048;
    float grip = Surface_GetAdhesiveLimit(ped + 0x174C) * g;
    if (chapterSeason[currentChapter] != 2 || At<int32_t>(ped, 0x148C) == 0) {
        // (the game pushes the surface byte in a dword whose upper bytes are stale; the
        // callee reads the byte)
        float w = Surface_GetWetMultiplier(ped[0x176E]);
        float t = w * grip;
        t = t * w;
        t = t * w;
        t = t * w;
        grip = w * t;
    }
    float force = At<float>(self, 0x58) * grip;
    float brake;
    float push;
    if (At<int32_t>(ped, 0x1310) == 0xD) {
        if (-1e-5f > At<float>(self, 0x50)) {
            brake = 1.0f;
            push = 0.0f;
        } else {
            brake = 1e-6f < At<float>(self, 0x50) ? Pad_GetValue(7, 0) : 0.0f;
            push = 1e-6f < At<float>(self, 0x54) ? Pad_GetValue(6, 0) : 0.0f;
        }
    } else {
        float s = At<float>(At<uint8_t*>(ped, 0x2E0), 0x1C);
        brake = s < 1.0f ? s : 1.0f;
        push = 0.001f > At<float>(At<uint8_t*>(ped, 0x2E0), 0x1C) ? 1.0f : 0.0f;
    }
    float least = At<float>(self, 0x78);
    float b = least > brake ? least : (brake > 1.0f ? 1.0f : brake);
    float ts = timeStep;
    float brakeStep = ts * b * At<float>(self, 0x50);
    float pushStep = push * ts * At<float>(self, 0x54);
    bool pushing = double(push) > 0.1;
    bool braking = !pushing && double(b) > 0.1;
    float along = 0.0f;
    float side = 0.0f;
    float dotF = point.x * fx + point.y * fy + point.z * fz;
    float limit = ts * force;
    float dotS = point.x * sx + point.y * sy + point.z * sz;
    if (dotS != 0.0f) {
        side = float(double(-dotS) * 0.5);
    }
    if (braking) {
        along = At<float>(self, 0x74) < dotF ? 0.0f : brakeStep;
        if (side > 0.0f) {
            if (side > limit) {
                side = limit;
            }
        } else {
            float low = -limit;
            if (low > side) {
                side = low;
            }
        }
    } else if (dotF != 0.0f) {
        float h = float(double(-dotF) * 0.5);
        along = h;
        if (!pushing || double(pushStep) < 2.499999936844688e-05) {
            pushStep = 2.5e-5f;
        }
        if (!(pushStep > limit) && !std::isnan(pushStep) && !std::isnan(limit)) {
            if (h > 0.0f) {
                if (h > pushStep) {
                    along = pushStep;
                }
            } else {
                float low = -pushStep;
                if (low > h) {
                    along = low;
                }
            }
        }
    }
    float limitSq = limit * limit;
    float sum = along * along + side * side;
    float response = At<float>(self, 0x60);
    if (pushing && 0.0f < At<float>(ped, 0x1D5C)) {
        float wet = float(double(At<float>(ped, 0x1D5C)) * 0.6000000238418579);
        response = (1.0f - wet) * response;
    }
    float mass = At<float>(self, 0x64);
    float inertia = At<float>(self, 0x68);
    if (limitSq < sum) {
        float root = SqrtAbs(sum);
        float k = limit / root * At<float>(self, 0x5C);
        along = k * along;
        side = k * side;
        if (response < 1.0f) {
            side = response * side;
        }
    } else if (response < 1.0f) {
        response = response * At<float>(self, 0x5C);
        if (limitSq * response * response < sum) {
            float root = SqrtAbs(sum);
            float scaled = limit * response;
            side = scaled / root * side;
        }
    }
    if (along == 0.0f && side == 0.0f) {
        return;
    }
    float facing = 0.0f;
    if (0.01f > At<float>(ped, 0x1D5C)) {
        V3 f;
        V3* fw = Placeable_GetForward(ped, &f);
        facing = fw->y * sy + fw->x * sx + fw->z * sz;
    }
    V3 u = {along * fx, fy * along, along * fz};
    V3 f;
    V3* fw = Placeable_GetForward(ped, &f);
    V3 h = {facing * fw->x, fw->y * facing, facing * fw->z};
    V3 h8 = {float(double(h.x) * 1.08), float(double(h.y) * 1.08), float(double(h.z) * 1.08)};
    float negSide = -side;
    V3 j = {h8.x * negSide, h8.y * negSide, negSide * h8.z};
    V3 k = {sx * side, sy * side, side * sz};
    V3 kj;
    V3 l;
    VectorAdd(&l, &VectorAdd(&kj, &k.x, &j.x)->x, &u.x);
    V3 m;
    VectorAdd(&m, &k.x, &u.x);
    float lengthL = SqrtAbs(LengthSq(l));
    float lengthM = SqrtAbs(LengthSq(m));
    V3 ln = l;
    if (!(lengthL <= 0.0f)) {
        float inv = 1.0f / lengthL;
        ln = {inv * l.x, l.y * inv, inv * l.z};
    } else {
        ln.x = 1.0f;   // (only x: the game's quirk)
    }
    V3 mn = m;
    if (!(lengthM <= 0.0f)) {
        float inv = 1.0f / lengthM;
        mn = {inv * m.x, m.y * inv, inv * m.z};
    } else {
        mn.x = 1.0f;
    }
    V3 n;
    CrossProduct(&n, &offset, &mn);
    float q = mass * lengthL;
    float n2 = LengthSq(n);
    float z = 1.0f / (1.0f / mass + n2 / inertia) * lengthM;
    V3 a = {q * ln.x, ln.y * q, q * ln.z};
    V3 bv = {z * mn.x, mn.y * z, z * mn.z};
    Physical_SetMoveSpeed(ped, a.x, a.y, a.z);
    V3 r0 = RightOf(ped);
    float t3 = r0.y * bv.y + r0.x * bv.x + r0.z * bv.z;
    r0 = RightOf(ped);
    float t4 = r0.y * oy + r0.x * ox + r0.z * oz;
    V3 f2;
    fw = Placeable_GetForward(ped, &f2);
    float t5 = fw->y * oy + fw->x * ox + fw->z * oz;
    if (second != 1 || !pushing) {
        r0 = RightOf(ped);
        V3 v = {r0.x * t4, r0.y * t4, t4 * r0.z};
        V3 r1 = RightOf(ped);
        V3 y = {r1.x * t3, r1.y * t3, t3 * r1.z};
        V3 at;
        V3* p = VectorSub(&at, &offset.x, &v.x);
        V3 fo;
        V3* fr = VectorSub(&fo, &bv.x, &y.x);
        Physical_ApplyTurnForce(ped, fr->x, fr->y, fr->z, p->x, p->y, p->z);
    }
    V3 dn = {sx, sy, 0.0f};
    Vector_Normalise(&dn);
    Physical_ApplyTurnForce(ped, dn.x * t3, dn.y * t3, t3 * dn.z, fx * t5, fy * t5, t5 * fz);
}

// 0x60D790 (thiscall): LocomotionJumpTrack::Start: the ped airborne (+0x2C: a vertical
// jump) with its velocity kept (+0x1708); the time to the top (+0x18 the height, the
// ped's rise speed +0x1D58 kept at +0x38) gives the jump's length (+0x4C), the launch
// speed (a vertical jump: the velocity up, clamped at 0 when grounded (+0x1318); else
// +0x1C along the ground over the length, +0x20 the forward reach +0x3C)
uint8_t __fastcall LocomotionJump_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x30) = owner;
    At<int32_t>(OwnerPed(owner), 0x1334) = 0;
    At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x30)), 0x1474) = self[0x2C];
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x30));
    float reach = At<float>(self, 0x1C);
    float height = At<float>(self, 0x18);
    At<float>(ped, 0x1708) = At<float>(ped, 0x124);
    At<float>(ped, 0x170C) = At<float>(ped, 0x128);
    At<float>(ped, 0x1710) = At<float>(ped, 0x12C);
    float result;
    owner = At<uint8_t*>(self, 0x30);
    if (self[0x2C]) {
        At<float>(self, 0x40) = At<float>(ped, 0x124);
        At<float>(self, 0x44) = At<float>(ped, 0x128);
        At<float>(self, 0x48) = At<float>(ped, 0x12C);
        if (At<int32_t>(OwnerPed(owner), 0x1318) != 0) {
            float up = At<float>(self, 0x48);
            if (double(up) < 0.0) {
                up = 0.0f;
            }
            At<float>(self, 0x48) = up;
        }
        float rise = At<float>(OwnerPed(owner), 0x1D58);
        At<float>(self, 0x38) = rise;
        float r = height / rise;
        float t = SqrtAbs(r + r);
        At<float>(self, 0x4C) = t + t;
        At<float>(self, 0x50) = 0.0f;
        result = t * rise + At<float>(self, 0x48);
    } else {
        float ax = std::fabs(At<float>(ped, 0x1708));
        float ay = std::fabs(At<float>(ped, 0x170C));
        if (double(ax) > 9.999999747378752e-05 || double(ay) > 9.999999747378752e-05) {
            At<float>(self, 0x40) = At<float>(ped, 0x124);
            At<float>(self, 0x44) = At<float>(ped, 0x128);
            At<float>(self, 0x48) = At<float>(ped, 0x12C);
        } else {
            V3 f;
            V3* fwd = Placeable_GetForward(OwnerPed(At<uint8_t*>(self, 0x30)), &f);
            At<float>(self, 0x40) = fwd->x;
            At<float>(self, 0x44) = fwd->y;
            At<float>(self, 0x48) = fwd->z;
        }
        At<float>(self, 0x48) = 0.0f;
        Vector_Normalise(&At<V3>(self, 0x40));
        owner = At<uint8_t*>(self, 0x30);
        float rise = At<float>(OwnerPed(owner), 0x1D58);
        At<float>(self, 0x38) = rise;
        float r = height / rise;
        float t = SqrtAbs(r + r);
        float length = t + t;
        At<float>(self, 0x4C) = length;
        At<float>(self, 0x50) = 0.0f;
        float k = reach / length;
        At<float>(self, 0x3C) = At<float>(self, 0x20) / length;
        At<float>(self, 0x40) = k * At<float>(self, 0x40);
        At<float>(self, 0x44) = k * At<float>(self, 0x44);
        result = t * rise;
    }
    self[0x34] = 0;
    At<float>(self, 0x48) = result;
    *At<float*>(owner, 0x38) = 0.0f;
    return 1;
}

// 0x60D9E0 (thiscall): LocomotionJumpTrack::Update: the jump's progress (+0x50 over +0x4C)
// is the parameter; along the ground (+0x2C clear) the velocity turns towards the stick
// (+0x28 the turn rate) and slows unless kept up (+0x24 the drag, +0x3C the forward
// reach); a vertical jump leans the ped (skateboard) by the stick's side, spinning it
// with its heading kept upright; the rise speed stays +0x38
uint8_t __fastcall LocomotionJump_Update(uint8_t* self, re::Edx, float dt) {
    uint8_t* owner = At<uint8_t*>(self, 0x30);
    float* move = &At<float>(OwnerPed(owner), 0x1708);
    if (!self[0x34]) {
        move[0] = At<float>(self, 0x40);
        move[1] = At<float>(self, 0x44);
        move[2] = At<float>(self, 0x48);
        uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x30));
        At<float>(ped, 0x124) = At<float>(self, 0x40);
        At<float>(ped, 0x128) = At<float>(self, 0x44);
        At<float>(ped, 0x12C) = At<float>(self, 0x48);
        At<float>(self, 0x48) = 0.0f;
        self[0x34] = 1;
    } else {
        float elapsed = At<float>(self, 0x50) + timeStep;
        At<float>(self, 0x50) = elapsed;
        float progress = elapsed / At<float>(self, 0x4C);
        if (1.0f < progress) {
            progress = 1.0f;
        }
        *At<float*>(owner, 0x38) = progress;
    }
    uint8_t* input = At<uint8_t*>(OwnerPed(At<uint8_t*>(self, 0x30)), 0x2E0);
    float speed = At<float>(input, 0x1C);
    V3 wanted = {At<float>(input, 0), At<float>(input, 4), At<float>(input, 8)};
    if (!self[0x2C]) {
        if (1.0f < speed) {
            speed = 1.0f;
        }
        V3* velocity = &At<V3>(self, 0x40);
        float align = (move[1] * wanted.y + move[0] * wanted.x) * speed;
        float length = SqrtAbs(LengthSq(*velocity));
        V3 dir = *velocity;
        Vector_Normalise(&dir);
        float c = (dir.y * wanted.y + dir.x * wanted.x) * speed;
        c = c + 1.0f;
        float q = float(double(c) * 0.5);
        float scale = 0.0f;
        bool rescale = false;
        if (q < 0.75f) {
            if (0.0f > q) {
                q = 0.0f;
            }
            float drag = (1.0f - q) * At<float>(self, 0x24);
            float e = ExpSeries(-dt * drag);
            length = e * length;
            if (length >= 0.0f) {
                scale = length;
                rescale = true;
            }
        } else if (0.0f != At<float>(self, 0x3C)) {
            float current = Vector_Magnitude(velocity);
            if (current < At<float>(self, 0x3C)) {
                float w = 1.0f - ExpSeries(-dt * At<float>(self, 0x24));
                float gap = At<float>(self, 0x3C) - current;
                scale = current + w * gap;
                rescale = true;
            }
        }
        if (rescale) {
            Vector_Normalise(velocity);
            velocity->x = scale * velocity->x;
            velocity->y = scale * velocity->y;
        }
        float turn = At<float>(self, 0x28);
        V3 out;
        if (double(turn) >= 9.999999747378752e-05 && 0.0f < align) {
            float h = TurnHeading(move[0], move[1], move[2], wanted.x, wanted.y, wanted.z, dt, turn, 1.0f, &align);
            float s = trk::CrtSin(h);
            out.x = -s * length;
            float co = trk::CrtCos(h);
            out.y = co * length;
        } else {
            out = *velocity;
        }
        move[0] = out.x;
        move[1] = out.y;
        At<float>(OwnerPed(At<uint8_t*>(self, 0x30)), 0x124) = out.x;
        At<float>(OwnerPed(At<uint8_t*>(self, 0x30)), 0x128) = out.y;
        At<float>(OwnerPed(At<uint8_t*>(self, 0x30)), 0x1D58) = At<float>(self, 0x38);
        return 1;
    }
    uint8_t* ped = OwnerPed(owner);
    if (VectorNotEqual(&wanted, zeroVector)) {
        uint8_t local[0x48];   // a CMatrix (its +0x40 attachment cleared)
        At<int32_t>(local, 0x40) = 0;
        Matrix_Invert(Placeable_GetMatrix(ped), local);
        V3 t;
        V3* r = Matrix_Multiply3x3(&t, local, &wanted);
        wanted = {r->x, r->y, 0.0f};
        Vector_Normalise(&wanted);
    }
    float side;
    float xs = wanted.x * At<float>(At<uint8_t*>(ped, 0x2E0), 0x1C);
    if (-0.029999999329447746 < double(xs) && 0.029999999329447746 > double(xs)) {
        side = 0.0f;
    } else {
        side = wanted.x;
        if (At<int32_t>(ped, 0x1310) == 0xD) {
            if (Pad_50DF50(padA45A30)) {
                if (!altSteering || Pad_738500(Pad_GetActive())) {
                    side = -Pad_GetValue(0x10, 0);
                } else {
                    double back = -double(Pad_GetValue(0x12, 0));
                    side = float(back - double(Pad_GetValue(0x10, 0)));
                }
                int32_t sign = side < 0.0f ? -1 : 1;
                float k = std::fabs(wanted.y * yAxis[1] + side * yAxis[0] + wanted.z * yAxis[2]);
                float pad = Pad_GetValue(0x11, 0);
                float m = std::fabs(float((1.0f - k) * pad * 0.6000000238418579));
                side = float(sign) * m + side;
            }
            int32_t sign = side < 0.0f ? -1 : 1;
            float p = trk::CrtPow(std::fabs(side), 1.9f);
            side = p * float(sign);
        }
        if (-1.0f > side) {
            side = -1.0f;
        } else if (1.0f < side) {
            side = 1.0f;
        }
    }
    float lean = trk::CrtAsin(side);
    float roll = float(double(-lean) * 0.8999999761581421);
    V3 f;
    V3* fwd = Placeable_GetForward(ped, &f);
    V3 back = {-fwd->x, -fwd->y, -fwd->z};
    float ts = timeStep;
    float v2 = LengthSq(At<V3>(ped, 0x124));
    float k2 = float(double(float(double(v2) / 0.05000000074505806)) + 0.699999988079071);
    if (double(k2) > 1.2000000476837158) {
        k2 = 1.2f;
    }
    V3 r0 = RightOf(ped);
    float w = At<float>(self, 0x28) * -roll;
    V3 t = {r0.x * w, r0.y * w, w * r0.z};
    V3 u = {t.x * k2, t.y * k2, k2 * t.z};
    V3 at = {u.x * ts, u.y * ts, ts * u.z};
    Physical_ApplyTurnForce(ped, back.x, back.y, back.z, at.x, at.y, at.z);
    At<float>(ped, 0x130) = 0.0f;
    At<float>(ped, 0x134) = 0.0f;
    float vv = LengthSq(At<V3>(ped, 0x124));
    if (double(vv) < 4.999999873689376e-05) {
        At<float>(ped, 0x138) = 0.0f;
    }
    Physical_ApplyTurnSpeed(ped);
    if (At<uint8_t*>(ped, 0x14)) {
        Matrix_Reorthogonalise(At<uint8_t*>(ped, 0x14));
    }
    float heading;
    const float* matrix = At<float*>(ped, 0x14);
    if (matrix) {
        heading = trk::CrtAtan2(-matrix[4], matrix[5]);
    } else {
        heading = At<float>(ped, 0x10);
    }
    Ped_SetHeading(ped, heading);
    float decay = trk::CrtPow(0.93f, ts);
    At<float>(ped, 0x130) = At<float>(ped, 0x130) * decay;
    At<float>(ped, 0x134) = decay * At<float>(ped, 0x134);
    At<float>(ped, 0x138) = decay * At<float>(ped, 0x138);
    At<float>(OwnerPed(At<uint8_t*>(self, 0x30)), 0x1D58) = At<float>(self, 0x38);
    return 1;
}

} // namespace

#define ACTTRACKSLOCOPHYS_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

ACTTRACKSLOCOPHYS_REVERSED("LocomotionTrack::Start", 0x0060C710, &LocomotionTrack_Start);
ACTTRACKSLOCOPHYS_REVERSED("LocomotionTrack::End", 0x0060C9F0, &LocomotionTrack_End);
ACTTRACKSLOCOPHYS_REVERSED("LocomotionTrack::FootProbe", 0x0060CAE0, &LocomotionTrack_FootProbe);
ACTTRACKSLOCOPHYS_REVERSED("LocomotionJumpTrack::Start", 0x0060D790, &LocomotionJump_Start);
ACTTRACKSLOCOPHYS_REVERSED("LocomotionJumpTrack::Update", 0x0060D9E0, &LocomotionJump_Update);
