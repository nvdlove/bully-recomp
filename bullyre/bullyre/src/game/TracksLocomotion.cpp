// The action tree's locomotion tracks' Update virtuals (0x6135E0 LocomotionTrack,
// 0x615570 LocomotionSlideTrack).
// A locomotion track: +0x18..+0x24 the stick-to-speed map (fast, slow, from, to), +0x28
// steers the ped, +0x2C turn rate, +0x30 turn acceleration, +0x34 slope turn rate, +0x38
// face the target, +0x39 the input's second vector, +0x3A may walk backwards (+0x3C its
// angle), +0x4C mode (1: the player's steering), +0x4D keeps the side, +0x54 sprint,
// +0x68 bike lean force, +0x6C spin damping, +0x70 turn speed, +0x74 top speed, +0x7C
// maps the stick, +0x80 the owner (+0x14 its locomotion output: +0x1C speed, +0x24
// direction), +0x88 turn speed state, +0x8C steer, +0x90 forward speed, +0x94..+0x9C the
// wanted direction, +0xA0 walking backwards.

#include <cmath>
#include <cstdint>

#include "game/ActionTracksCommon.h"
#include "game/TrackMath.h"

using namespace actc;

namespace {

GAME_VAR(uint32_t, groundContacts, 0x00C2B10C);
GAME_VAR(uint32_t, slopeContacts, 0x00C2B110);
GAME_VAR(uint32_t, slopeContacts2, 0x00C2B114);
GAME_ARRAY(float, groundNormal, 0x00C2B144);
GAME_VAR(float, backwardsSpeed, 0x00B0B140);   // (-0.005)
GAME_VAR(float, bikeLeanScale, 0x00B0B144);
GAME_VAR(float, bikeLeanBase, 0x00B0B148);
GAME_VAR(float, bikePushScale, 0x00B0BC48);
GAME_VAR(float, sprintRate, 0x00B0B194);
GAME_VAR(uint8_t, altSteering, 0x00A14925);
GAME_VAR(int32_t, gameMode, 0x00C3CCF8);
inline void* const theCamera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const padA45A30 = reinterpret_cast<void*>(0x00A45A30);
inline const float* const yAxis = reinterpret_cast<const float*>(0x009E33D0);
inline const float* const zAxis = reinterpret_cast<const float*>(0x009E33DC);

GAME_FN(uint8_t*, __thiscall, Targeting_GetTarget, 0x004D5AD0, uint8_t* targeting);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(V3*, __thiscall, Camera_GetForward, 0x0041AA70, void* camera, V3* out);
GAME_FN(uint8_t, __cdecl, VectorNotEqual, 0x004305F0, const void* a, const void* b);
GAME_FN(uint8_t, __cdecl, VectorEqual, 0x00417870, const void* a, const void* b);
GAME_FN(uint8_t*, __thiscall, Placeable_GetMatrix, 0x0046DD20, uint8_t* placeable);
GAME_FN(void, __cdecl, Matrix_Invert, 0x00413770, const void* matrix, void* out);
GAME_FN(V3*, __cdecl, Matrix_Multiply3x3, 0x0041C490, V3* out, const void* matrix, const void* v);
GAME_FN(uint8_t, __thiscall, Pad_50DF50, 0x0050DF50, void* pad);
GAME_FN(int32_t, __cdecl, Pad_GetActive, 0x00738070);
GAME_FN(uint8_t, __cdecl, Pad_738500, 0x00738500, int32_t pad);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const void* a, const void* b);
GAME_FN(V3*, __cdecl, CrossProduct, 0x00414730, V3* out, const void* a, const void* b);
GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_FN(V3*, __thiscall, Ped_GetRight, 0x004736F0, uint8_t* ped, V3* out);
GAME_FN(float, __thiscall, Placeable_GetHeading, 0x00415470, uint8_t* placeable);
GAME_FN(float, __cdecl, LimitAngle, 0x00432EE0, float angle);
GAME_FN(float, __cdecl, SafeAcos, 0x00413D60, float x);
GAME_FN(float, __cdecl, TurnHeading, 0x00608380, float x, float y, float z, float toX, float toY, float toZ,
        float accel, float max, float rate, float* speed);
GAME_FN(void, __thiscall, Matrix_SetRotateZOnly, 0x00412C20, uint8_t* matrix, float angle);
GAME_FN(void, __thiscall, Ped_SetHeading, 0x004734C0, uint8_t* ped, float heading);
GAME_FN(V3*, __cdecl, Normalised, 0x00472D40, V3* out, const void* v);
GAME_FN(float, __thiscall, Vector_Magnitude3, 0x0040D9F0, const void* v);
GAME_FN(float, __cdecl, Min, 0x00414470, float a, float b);
GAME_FN(V3*, __cdecl, Vector_ScaleBy, 0x00412320, V3* out, float s, const V3* v);
GAME_FN(V3*, __cdecl, Vector_Scaled, 0x00412350, V3* out, const V3* v, float s);
GAME_FN(void, __thiscall, Physical_ApplyTurnForce, 0x0045D410, uint8_t* physical, const V3* force, const V3* point);
GAME_FN(void, __thiscall, Physical_ApplyTurnSpeed, 0x00469DC0, uint8_t* physical);
GAME_FN(void, __thiscall, Matrix_Reorthogonalise, 0x00413A80, uint8_t* matrix);
GAME_FN(void, __thiscall, Vector_ScaleInPlace, 0x0088A480, float* v, float s);
GAME_FN(float, __cdecl, DivideBySqrt, 0x00414C20, float a, float b);
// the foot probe (0x60CAE0): a turned forward, its side, an offset; which foot
GAME_FN(void, __thiscall, LocomotionTrack_Probe_x64, 0x0060CAE0, uint8_t* self, float, float, float, float, float, float, float, float, float, int second);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void LocomotionTrack_Probe(uint8_t* a0, V3 a1, V3 a2, V3 a3, int a4) {
    return LocomotionTrack_Probe_x64(a0, a1.x, a1.y, a1.z, a2.x, a2.y, a2.z, a3.x, a3.y, a3.z, a4);
}

constexpr float kPi = 3.14159274f;

uint8_t* LocoOutput(uint8_t* self) {
    return At<uint8_t*>(At<uint8_t*>(self, 0x80), 0x14);
}
void SetOutputDirection(uint8_t* self, const V3& d) {
    uint8_t* out = LocoOutput(self);
    At<float>(out, 0x24) = d.x;
    At<float>(out, 0x28) = d.y;
    At<float>(out, 0x2C) = d.z;
}
V3 Neg(const V3& v) {
    return {-v.x, -v.y, -v.z};
}
// a probe along the tangent turned by angle (sin / cos of it: t cos - r sin, t sin + r cos)
void TurnedProbe(uint8_t* self, const V3& t, const V3& r, float angle, V3 offset, int second) {
    float s = trk::CrtSin(angle);
    float c = trk::CrtCos(angle);
    V3 a = {c * t.x - s * r.x, c * t.y - s * r.y, c * t.z - s * r.z};
    V3 b = {s * t.x + c * r.x, s * t.y + c * r.y, s * t.z + c * r.z};
    LocomotionTrack_Probe(self, a, b, offset, second);
}
// the forward along the ground (normal ped +0x175C)
V3 GroundTangent(uint8_t* ped, V3 t) {
    const float* n = &At<float>(ped, 0x175C);
    float d = DotProduct(&t, n) * 0.45f;
    t.x = t.x - n[0] * d;
    t.y = t.y - n[1] * d;
    t.z = t.z - d * n[2];
    Vector_Normalise(&t);
    return t;
}

// the heading the ped turns to (a matrix's rotation, its heading and the ped's)
void SetPedHeading(uint8_t* ped, float heading) {
    if (At<uint8_t*>(ped, 0x14)) {
        Matrix_SetRotateZOnly(At<uint8_t*>(ped, 0x14), heading);
    }
    At<float>(ped, 0x10) = heading;
    Ped_SetHeading(ped, heading);
}

// 0x6135E0 (thiscall): LocomotionTrack::Update: the ped steered by its input (or its
// target / the camera), the wanted direction and speed handed to the locomotion
// output; the player's own mode (+0x4C 1): stick steering, sprint, bike lean and the
// foot probes
uint8_t __fastcall LocomotionTrack_Update(uint8_t* self, re::Edx, float dt) {
    if (self[0x3A] != 0 && self[0x4C] == 0) {
        self[0xA0] = 0;
        return 1;
    }
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x80));
    float* input = At<float*>(ped, 0x2E0);
    float stick = input[7];
    V3 move = {input[0], input[1], input[2]};
    V3 dir = move;
    if (self[0x39] != 0) {
        dir = {input[3], input[4], input[5]};
    }
    uint8_t* target = Targeting_GetTarget(At<uint8_t*>(ped, 0x1C8));
    if (self[0x38] != 0) {
        if (target) {
            V3 d;
            V3* r = VectorSub(&d, Pos(target), Pos(ped));
            dir = *r;
            stick = 0.3f;
        } else if (self[0x38] != 0 && At<int32_t>(ped, 0x1310) == 0xD && !(Pad_GetValue(10, 0) == 0.0f) &&
                   gameMode == 1) {
            V3 f;
            dir = *Camera_GetForward(theCamera, &f);
            stick = 0.3f;
        }
    }
    move.z = 0.0f;
    Vector_Normalise(&move);
    dir.z = 0.0f;
    Vector_Normalise(&dir);

    if (self[0x4C] != 1) {
        if (self[0x28] != 0) {
            float rate = At<float>(self, 0x2C);
            float accel = At<float>(self, 0x30);
            V3 fwd;
            Placeable_GetForward(ped, &fwd);
            Vector_Normalise(&fwd);
            if (At<int32_t>(ped, 0x1310) == 0xD && At<int32_t>(ped, 0x131C) != 0 && groundContacts != 0) {
                uint8_t steep = slopeContacts != 0 ? 1 : 0;
                float inv;
                bool slide = true;
                if (steep) {
                    inv = 1.0f / float(slopeContacts);
                    if (!(inv > 0.4f)) {
                        slide = false;
                    }
                } else {
                    inv = 1.0f / float(groundContacts);
                }
                // walking into the slope: along it instead
                if (slide) {
                    V3 g = {groundNormal[0], groundNormal[1], groundNormal[2]};
                    if (DotProduct(&g, &dir) < 0.0f) {
                        V3 across;
                        CrossProduct(&across, &g, zAxis);
                        if (0.0f < DotProduct(&across, &dir)) {
                            dir = across;
                        } else {
                            dir = Neg(across);
                        }
                        if (steep) {
                            rate = At<float>(self, 0x34) * 0.0625f;
                            if (0.0f == At<float>(ped, 0x124) && 0.0f == At<float>(ped, 0x128)) {
                                rate = inv * rate;
                            }
                        } else {
                            rate = At<float>(self, 0x34) * inv;
                        }
                        input[0] = dir.x;
                        input[1] = dir.y;
                        input[2] = dir.z;
                        move = dir;
                    }
                }
            }
            uint8_t flipped = 0;
            if (self[0x3A] != 0) {
                V3 back = Neg(fwd);
                float angle = SafeAcos(dir.y * back.y + back.x * dir.x + back.z * dir.z);
                if (!(At<float>(self, 0x3C) < angle)) {
                    fwd = back;
                    flipped = 1;
                }
            }
            float heading = TurnHeading(fwd.x, fwd.y, fwd.z, dir.x, dir.y, dir.z, dt, rate, accel,
                                        &At<float>(self, 0x88));
            if (flipped) {
                heading = heading + kPi;
            }
            SetPedHeading(ped, LimitAngle(heading));
        }
        uint8_t* out = LocoOutput(self);
        if (self[0x7C] == 0) {
            At<float>(out, 0x1C) = -1.0f;
            SetOutputDirection(self, move);
            return 1;
        }
        // the stick mapped onto the speed (+0x20..+0x24 onto +0x1C..+0x18)
        float fast = At<float>(self, 0x18);
        float from = At<float>(self, 0x20);
        float slow = At<float>(self, 0x1C);
        float to = At<float>(self, 0x24);
        float speed;
        if (to < stick || to == stick) {
            speed = fast;
        } else {
            float over = stick - from;
            if (!(over > 0.0f)) {
                speed = slow;
            } else {
                speed = (fast - slow) * (over / (to - from)) + slow;
            }
        }
        At<float>(LocoOutput(self), 0x1C) = speed;
        SetOutputDirection(self, move);
        return 1;
    }

    // the player's steering: the stick in the ped's frame
    V3 local = move;
    if (VectorNotEqual(&local, zeroVector)) {
        alignas(4) uint8_t inverse[0x44];
        At<uint32_t>(inverse, 0x40) = 0;
        Matrix_Invert(Placeable_GetMatrix(ped), inverse);
        V3 r;
        V3* p = Matrix_Multiply3x3(&r, inverse, &local);
        local.x = p->x;
        local.y = p->y;
        local.z = 0.0f;
        Vector_Normalise(&local);
    }
    float side = At<float*>(ped, 0x2E0)[7] * local.x;
    if (-0.035f < side && side < 0.035f) {
        local.x = 0.0f;
    } else if (At<int32_t>(ped, 0x1310) == 0xD) {
        if (Pad_50DF50(padA45A30)) {
            float steer;
            if (altSteering == 0 || Pad_738500(Pad_GetActive()) != 0) {
                steer = -Pad_GetValue(0x10, 0);
            } else {
                float a = -Pad_GetValue(0x12, 0);
                steer = a - Pad_GetValue(0x10, 0);
            }
            local.x = steer;
            float p11 = Pad_GetValue(0x11, 0);
            int32_t sign = 0.0f > local.x ? -1 : 1;
            float d = std::fabs(DotProduct(&local, yAxis));
            float p = Pad_GetValue(0x11, 0);
            float extra = std::fabs((1.0f - d) * p * (0.0f < p11 ? 0.6f : 6.5f));
            local.x = float(sign) * extra + local.x;
        }
        int32_t sign = local.x < 0.0f ? -1 : 1;
        local.x = trk::CrtPow(std::fabs(local.x), 1.91f) * float(sign);
    }
    V3 f0;
    V3* fwd = Placeable_GetForward(ped, &f0);
    float* vel = &At<float>(ped, 0x124);
    float fwdSpeed = fwd->y * vel[1] + vel[0] * fwd->x + fwd->z * vel[2];
    if (At<int32_t>(ped, 0x1310) == 0xD && self[0x3A] != 0 && At<int32_t>(ped, 0x1488) != 0) {
        self[0xA0] = 1;
        local.x = -local.x;
    } else {
        self[0xA0] = 0;
    }
    // the steer smoothed at +0x2C (0..1) per second
    float r = At<float>(self, 0x2C);
    float keep = r < 0.0f ? 0.0f : (r > 1.0f ? 1.0f : r);
    float k = trk::CrtPow(keep, timeStep);
    local.x = (local.x - At<float>(self, 0x8C)) * k + At<float>(self, 0x8C);
    float steer = local.x;
    if (steer < -1.0f) {
        steer = -1.0f;
    } else if (1.0f < steer) {
        steer = 1.0f;
    }
    local.x = steer;
    At<float>(self, 0x8C) = steer;
    float turn = -trk::CrtAsin(steer) * At<float>(self, 0x70);
    At<float>(LocoOutput(self), 0x1C) = fwdSpeed / At<float>(self, 0x74);
    float speedSq = vel[0] * vel[0] + vel[1] * vel[1] + vel[2] * vel[2];
    if (speedSq > 0.005f) {
        V3 n = {vel[0], vel[1], vel[2]};
        Vector_Normalise(&n);
        SetOutputDirection(self, n);
    } else {
        V3 f;
        SetOutputDirection(self, *Placeable_GetForward(ped, &f));
    }
    float k2 = trk::CrtPow(0.3f, timeStep);
    fwdSpeed = (fwdSpeed - At<float>(self, 0x90)) * k2 + At<float>(self, 0x90);
    float gait;
    if (At<int32_t>(ped, 0x147C) != 0 && 0.0f < At<float>(ped, 0x1D5C)) {
        gait = 1.0f;
    } else {
        float t = std::fabs(fwdSpeed / At<float>(self, 0x74));
        gait = t < 0.5f ? 0.5f : (1.0f < t ? 1.0f : t);
        if (At<int32_t>(ped, 0x148C) != 0) {
            gait = gait * 0.5f;
        }
        if (At<int32_t>(ped, 0x1488) != 0) {
            gait = gait * gait;
        }
    }
    // the wanted direction in the ped's frame: sideways by the steer, forward the rest
    V3 want;
    want.x = gait * local.x;
    want.y = trk::CrtCos(trk::CrtAsin(std::fabs(want.x)));
    want.z = 0.0f;
    if (fwdSpeed < backwardsSpeed) {
        float b = fwdSpeed / At<float>(self, 0x74) - gait * 1.6f;
        if (b < -1.0f) {
            b = -1.0f;
        } else if (b > -0.1f) {
            b = -0.1f;
        }
        want.y = b;
    }
    if (std::fabs(want.x) < 0.001f) {
        float q = At<float>(self, 0x74) * 0.25f;
        float s = std::fabs(fwdSpeed);
        if (s < q) {
            want.y = want.y * (1.0f - (q - s));
            want.x = (1.0f - std::fabs(want.y)) * 0.5f * gait;
        }
    }
    Vector_Normalise(&want);
    if (0.0f < At<float>(ped, 0x1D5C)) {
        want.x = (want.x - At<float>(self, 0x94)) * 0.35f + At<float>(self, 0x94);
        want.y = (want.y - At<float>(self, 0x98)) * 0.1f + At<float>(self, 0x98);
    } else {
        float s = std::fabs(fwdSpeed);
        if (At<float>(self, 0x74) * 0.45f < s) {
            want.x = (want.x - At<float>(self, 0x94)) * 0.75f + At<float>(self, 0x94);
            want.y = (want.y - At<float>(self, 0x98)) * 0.6f + At<float>(self, 0x98);
        } else if (At<float>(self, 0x74) * 0.15f < s) {
            want.x = (want.x - At<float>(self, 0x94)) * 0.6f + At<float>(self, 0x94);
            want.y = (want.y - At<float>(self, 0x98)) * 0.48f + At<float>(self, 0x98);
        } else {
            want.x = (want.x - At<float>(self, 0x94)) * 0.4f + At<float>(self, 0x94);
            want.y = (want.y - At<float>(self, 0x98)) * 0.2f + At<float>(self, 0x98);
        }
    }
    Vector_Normalise(&want);
    if (self[0x4D] == 0) {
        int32_t was = 0.0f > At<float>(self, 0x98) ? -1 : 1;
        int32_t now = 0.0f > want.y ? -1 : 1;
        if (was != now) {
            want = {At<float>(self, 0x94), At<float>(self, 0x98), At<float>(self, 0x9C)};
        }
    }
    At<float>(self, 0x94) = want.x;
    At<float>(self, 0x98) = want.y;
    At<float>(self, 0x9C) = want.z;
    // pushed sideways by what the ped leans on (+0x16F0)
    uint8_t* leanOn = At<uint8_t*>(ped, 0x16F0);
    if (At<int32_t>(ped, 0x13A4) != 0 && leanOn && vc::Call<uint8_t>(leanOn, 0x40)) {
        alignas(4) uint8_t info[0x44];
        vc::Call<void>(At<uint8_t*>(ped, 0x16F0), 0x10, info);
        V3 rt;
        V3* r2 = Ped_GetRight(ped, &rt);
        V3 right = *r2;
        float push;
        if (At<int32_t>(ped, 0x1488) != 0) {
            push = DotProduct(info + 0x20, &right) * bikePushScale * 0.4f;
        } else {
            push = DotProduct(info + 0x20, &right) * bikePushScale;
        }
        want.x = push + want.x;
        Vector_Normalise(&want);
    }
    V3 world;
    V3* w = Matrix_Multiply3x3(&world, Placeable_GetMatrix(ped), &want);
    want = *w;
    if (self[0x28] != 0) {
        float* in = At<float*>(ped, 0x2E0);
        in[0] = want.x;
        in[1] = want.y;
        in[2] = want.z;
    } else {
        turn = 0.0f;
    }
    At<float>(self, 0x90) = fwdSpeed;

    // the player's sprint (button 6): its charge (ped +0x1D5C, +0x1D60 when) and the
    // lean it puts into turns
    if (At<int32_t>(ped, 0x1310) == 0xD && self[0x28] != 0 && 1e-5f < At<float>(self, 0x54) &&
        At<int32_t>(ped, 0x1470) != 0) {
        uint8_t wasSprinting = At<int32_t>(ped, 0x147C) != 0 ? 1 : 0;
        At<float>(ped, 0x1D5C) = At<float>(ped, 0x1D5C) - timeStep * 0.09f;
        float button = Pad_GetValue(6, 0);
        int32_t sprinting = 0;
        if (0.0f < button && At<int32_t>(ped, 0x1758) != 0 && At<float>(self, 0x70) * 0.8f < std::fabs(turn)) {
            sprinting = 1;
        }
        At<int32_t>(ped, 0x147C) = sprinting;
        if (!(0.0f < At<float>(ped, 0x1D5C))) {
            At<float>(ped, 0x1D5C) = 0.0f;
            if (sprinting && !wasSprinting) {
                float sq = vel[1] * vel[1] + vel[0] * vel[0];
                if (sq > 0.018f && At<uint32_t>(ped, 0x1D60) + 0xFA < timeInMs) {
                    At<float>(ped, 0x1D5C) = 1.0f;
                    At<uint32_t>(ped, 0x1D60) = timeInMs;
                }
            }
        } else {
            At<uint32_t>(ped, 0x1D60) = timeInMs;
            if (!sprinting) {
                At<float>(ped, 0x1D5C) = 0.0f;
            } else if (self[0xA0] != 0) {
                V3 t0;
                V3* f = Placeable_GetForward(ped, &t0);
                V3 back = Neg(*f);
                float k3 = trk::CrtPow(1.9f, timeStep);
                V3 nv;
                float d = DotProduct(Normalised(&nv, vel), &back);
                float m = Min(1.2f, Vector_Magnitude3(vel) / 0.05f + 0.5f);
                V3* right = Ped_GetRight(ped, &back);
                float amount = -turn * At<float>(ped, 0x1D5C) * At<float>(self, 0x68) * 0.015f;
                V3 a;
                V3 b;
                V3 c;
                V3 e;
                Vector_ScaleBy(&a, amount, right);
                Vector_Scaled(&b, &a, m);
                Vector_Scaled(&c, &b, k3);
                Vector_Scaled(&e, &c, d);
                V3 t1;
                Physical_ApplyTurnForce(ped, Placeable_GetForward(ped, &t1), &e);
            } else {
                V3 t0;
                V3* f = Placeable_GetForward(ped, &t0);
                V3 back = Neg(*f);
                float k3 = trk::CrtPow(1.9f, timeStep);
                V3 t1;
                V3* f2 = Placeable_GetForward(ped, &t1);
                V3 nv;
                float d = DotProduct(Normalised(&nv, vel), f2);
                float m = Min(1.2f, Vector_Magnitude3(vel) / 0.05f + 0.5f);
                V3 rt;
                V3* right = Ped_GetRight(ped, &rt);
                float amount = -turn * At<float>(ped, 0x1D5C) * At<float>(self, 0x68) * 0.015f;
                V3 a;
                V3 b;
                V3 c;
                V3 e;
                Vector_ScaleBy(&a, amount, right);
                Vector_Scaled(&b, &a, m);
                Vector_Scaled(&c, &b, k3);
                Vector_Scaled(&e, &c, d);
                Physical_ApplyTurnForce(ped, &back, &e);
            }
        }
    }

    if (At<int32_t>(ped, 0x1758) != 0) {
        if (self[0x28] != 0) {
            // the feet probed along the ground, turned by the turn
            if (self[0xA0] != 0) {
                V3 t0;
                V3 f = Neg(*Placeable_GetForward(ped, &t0));
                V3 offset = {f.x * 4.5f, f.y * 4.5f, -0.5f};
                V3 t1;
                V3 tangent = GroundTangent(ped, Neg(*Placeable_GetForward(ped, &t1)));
                V3 rt;
                V3 right = Neg(*Ped_GetRight(ped, &rt));
                TurnedProbe(self, tangent, right, -turn, offset, 0);
                V3 t2;
                V3* f3 = Placeable_GetForward(ped, &t2);
                V3 offset2 = {f3->x, f3->y, -0.5f};
                TurnedProbe(self, tangent, right, turn * 0.5f, offset2, 1);
            } else {
                V3 t0;
                V3* f = Placeable_GetForward(ped, &t0);
                V3 offset = {f->x * 4.5f, f->y * 4.5f, -0.5f};
                V3 t1;
                V3 tangent = GroundTangent(ped, *Placeable_GetForward(ped, &t1));
                V3 rt;
                V3 right = *Ped_GetRight(ped, &rt);
                TurnedProbe(self, tangent, right, turn, offset, 0);
                V3 t2;
                V3 f3 = Neg(*Placeable_GetForward(ped, &t2));
                V3 offset2 = {f3.x, f3.y, -0.5f};
                TurnedProbe(self, tangent, right, -turn * 0.5f, offset2, 1);
            }
        }
        At<float>(ped, 0x130) = 0.0f;
        At<float>(ped, 0x134) = 0.0f;
        float sq = vel[0] * vel[0] + vel[1] * vel[1] + vel[2] * vel[2];
        if (sq < 5e-5f) {
            At<float>(ped, 0x138) = 0.0f;
        }
        if (self[0x28] != 0) {
            // on a bike (+0x16C8): spun towards its heading
            uint8_t* bike = At<uint8_t*>(ped, 0x16C8);
            if (At<int32_t>(ped, 0x148C) != 0 && bike) {
                V3 t0;
                V3 t1;
                V3* bf = Placeable_GetForward(bike, &t0);
                float d = DotProduct(Placeable_GetForward(ped, &t1), bf);
                V3 flat = {vel[0], vel[1], 0.0f};
                V3 t2;
                float lean = std::fabs(DotProduct(Placeable_GetForward(bike, &t2), &flat)) * bikeLeanScale +
                             bikeLeanBase;
                float heading = Placeable_GetHeading(ped);
                float diff;
                if (At<int32_t>(ped, 0x1488) != 0) {
                    float h = LimitAngle(heading + kPi);
                    if (LimitAngle(h - Placeable_GetHeading(bike)) < -0.157079637f) {
                        At<float>(ped, 0x138) =
                            (1.0f - d * d) * (timeStep * 0.1f) * (lean * lean) + At<float>(ped, 0x138);
                    }
                    bike = At<uint8_t*>(ped, 0x16C8);
                    float h2 = LimitAngle(Placeable_GetHeading(ped) + kPi);
                    diff = h2 - Placeable_GetHeading(bike);
                } else {
                    if (LimitAngle(heading - Placeable_GetHeading(bike)) < -0.157079637f) {
                        At<float>(ped, 0x138) =
                            (1.0f - d * d) * (timeStep * 0.1f) * (lean * lean) + At<float>(ped, 0x138);
                    }
                    bike = At<uint8_t*>(ped, 0x16C8);
                    float h2 = Placeable_GetHeading(ped);
                    diff = h2 - Placeable_GetHeading(bike);
                }
                if (LimitAngle(diff) > 0.157079637f) {
                    At<float>(ped, 0x138) =
                        At<float>(ped, 0x138) - (lean * lean) * ((1.0f - d * d) * (timeStep * 0.1f));
                }
            }
            Physical_ApplyTurnSpeed(ped);
            if (At<uint8_t*>(ped, 0x14)) {
                Matrix_Reorthogonalise(At<uint8_t*>(ped, 0x14));
            }
            Ped_SetHeading(ped, Placeable_GetHeading(ped));
            if (self[0x4D] != 0) {
                float threshold = backwardsSpeed;
                if (At<int32_t>(ped, 0x1488) == 0) {
                    if (At<float>(self, 0x98) < threshold) {
                        At<int32_t>(ped, 0x1488) = 1;
                    }
                } else if (-threshold < At<float>(self, 0x98)) {
                    At<int32_t>(ped, 0x1488) = 0;
                }
            }
        }
        float spin = trk::CrtPow(At<float>(self, 0x6C), timeStep);
        At<float>(ped, 0x130) = At<float>(ped, 0x130) * spin;
        At<float>(ped, 0x134) = At<float>(ped, 0x134) * spin;
        At<float>(ped, 0x138) = spin * At<float>(ped, 0x138);
        uint8_t state = ped[0x1BE];
        if (state == 3 || state == 0x1F || state == 2 || state == 4 || state == 5 || state == 0x1B ||
            state == 0x1E) {
            float damp = trk::CrtPow(0.95f, timeStep);
            vel[0] = vel[0] * damp;
            vel[1] = damp * vel[1];
            vel[2] = damp * vel[2];
        } else if (state == 8 || state == 0x23 || state == 0x2A || state == 0x38 || state == 0x3A) {
            Vector_ScaleInPlace(vel, trk::CrtPow(0.99f, timeStep));
        }
    }
    // the output: the flat velocity per animation step, its length the speed
    float q = timeStep / (sprintRate * timeStep);
    V3 v = {q * vel[0], q * vel[1], 0.0f};
    float mag = std::fabs(v.x * v.x + v.y * v.y + 0.0f * 0.0f);
    Vector_Normalise(&v);
    At<float>(LocoOutput(self), 0x1C) = std::sqrt(mag);
    SetOutputDirection(self, v);
    return 1;
}

// 0x615570 (thiscall): LocomotionSlideTrack::Update: sliding down the ground's slope
// (normal ped +0x173C, only while between flat and sheer): the ped turned downhill
// when it's sliding on its own (+0x130D), its slide velocity (+0x1708) pushed by the
// slope (times +0x18, at most +0x1C) and a grip that builds over time (+0x18D8)
uint8_t __fastcall LocomotionSlideTrack_Update(uint8_t* self, re::Edx, float dt) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x20));
    float nz = At<float>(ped, 0x1744);
    float nx = At<float>(ped, 0x173C);
    float ny = At<float>(ped, 0x1740);
    if (!(nz > 1e-4f) || !(nz < 0.98f)) {
        return 0;
    }
    // the downhill direction: the normal's flat part with z to make it perpendicular
    float flatSq = nx * nx + ny * ny;
    float z = -(flatSq / nz);
    float lenSq = z * z + flatSq;
    float inv = DivideBySqrt(1.0f, lenSq);
    V3 down = {inv * nx, inv * ny, inv * z};
    float steep = std::sqrt(std::fabs(lenSq));
    float speed = steep * At<float>(self, 0x18);
    float most = At<float>(self, 0x1C);
    if (ped[0x130D] != 0) {
        float turnSpeed = 0.0f;
        V3 fwd;
        Placeable_GetForward(ped, &fwd);
        Vector_Normalise(&fwd);
        V3 to = {down.x, down.y, 0.0f};
        Vector_Normalise(&to);
        float heading = TurnHeading(fwd.x, fwd.y, fwd.z, to.x, to.y, to.z, dt, 6.31f, 1.0f, &turnSpeed);
        SetPedHeading(ped, LimitAngle(heading));
    }
    if (most < speed) {
        speed = most;
    }
    float step = speed * dt;
    float push = step / timeStep;
    float grip;
    uint8_t* ground = At<uint8_t*>(ped, 0x1758);
    if (ground && (ground[0x108] & 7) == 2) {
        grip = 0.3f;
    } else if (ped[0x130D] != 0) {
        grip = 0.9f;
    } else {
        grip = 0.075f;
    }
    float amount = At<float>(ped, 0x18D8);
    if (grip < amount) {
        amount = grip;
    }
    float* slide = &At<float>(ped, 0x1708);
    if (At<int32_t>(ped, 0x1310) == 0xD && slopeContacts > 0 && slopeContacts2 > 1) {
        amount = amount * 3.0f + grip;
        if (3.0f < amount) {
            amount = 3.0f;
        }
    } else if (VectorEqual(slide, zeroVector)) {
        amount = grip;
        At<float>(ped, 0x18D8) = At<float>(ped, 0x18D8) + dt;
    }
    float f = amount * push;
    V3 add = {f * down.x, f * down.y, f * down.z};
    slide[0] = slide[0] + add.x;
    slide[1] = slide[1] + add.y;
    slide[2] = slide[2] + add.z;
    return 1;
}

} // namespace

#define TRLOCO_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

TRLOCO_REVERSED("LocomotionTrack::Update", 0x006135E0, &LocomotionTrack_Update);
TRLOCO_REVERSED("LocomotionSlideTrack::Update", 0x00615570, &LocomotionSlideTrack_Update);
