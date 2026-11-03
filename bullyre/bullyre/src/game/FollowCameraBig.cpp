#include "game/Camera.h"

#include <cmath>
#include "reversed.h"

// FollowCameraController3's per-step update (0x500B40): FollowCameraBase's process
// runs it three times a frame (see 0x4FDCF0). It blends between the normal follow
// view and the fight view (+0x160, the fight part; +0x25C the blend, run by
// +0x260..+0x26C), turns the camera after the ped (or by the stick), keeps its
// height (+0xCC) and distance (+0xC8) inside the attribute set's limits, and puts the
// camera (+0x34) at the look-at point (+0xB0) plus the direction (+0xD0) times the
// distance, raised by the height.
//
// Layout used here, beyond FollowCamera3b.cpp's: +0x34 the camera position, +0x40
// the look-at target, +0x4C the look-at point, +0x58/+0x5C a ped too close (from
// 0x5003C0), +0x88, +0x90 the smoothed look-from, +0x9C/+0xA0 the smoothing rates,
// +0xA4 the look-from last step, +0xB0 the look-at point, +0xBC the look-from before,
// +0xC8 the distance, +0xCC the height, +0xD0 the direction (horizontal, unit),
// +0xDC the position blended from, +0xE8/+0xEC/+0xF0 the speed kept, +0xF4 a
// FollowInterp (0 standing, 1 moving), +0x10C/+0x110, +0x114, +0x115 turning with the
// ped, +0x1BC..+0x1E0 the view saved for the fight, +0x1E4/+0x1F0 the fight's
// positions, +0x24C/+0x250, +0x254 blending into the fight view, +0x25C the blend,
// +0x260 its start, +0x264 its duration, +0x268 running, +0x269 towards the fight,
// +0x26C, +0x27C the vehicle part's driven controller, +0x284/+0x288 its follow,
// +0x28C/+0x290 its turn, +0x2C0/+0x2F8 a point to turn to, +0x50C, +0x510.
//
// sin, cos and sqrt are the CRT's (_CIsin 0x85AEF0, _CIcos 0x85AD80, _CIsqrt 0x85C500
// in the game), in double here where the game has the x87's 80 bits.

#define FOLLOWCAMBIG_REVERSED(...) REVERSED(__VA_ARGS__)

namespace {

using cam::CVector;

GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);   // CTimer's
GAME_VAR(float, timeStep, 0x00C1A9A4);      // CTimer::ms_fTimeStep
// tuning (0xA45820..0xA45898)
GAME_VAR(float, g_A45820, 0x00A45820);
GAME_VAR(float, g_A45824, 0x00A45824);
GAME_VAR(float, g_A45828, 0x00A45828);
GAME_VAR(float, g_A4582C, 0x00A4582C);
GAME_VAR(float, g_A45830, 0x00A45830);
GAME_VAR(float, g_A45834, 0x00A45834);
GAME_VAR(float, g_A45838, 0x00A45838);
GAME_VAR(float, g_A4583C, 0x00A4583C);
GAME_VAR(float, g_A45840, 0x00A45840);
GAME_VAR(float, g_A45844, 0x00A45844);
GAME_VAR(float, g_A45848, 0x00A45848);
GAME_VAR(uint32_t, g_A4584C, 0x00A4584C);
GAME_VAR(float, g_A45850, 0x00A45850);
GAME_VAR(float, g_A45854, 0x00A45854);
GAME_VAR(float, g_A45858, 0x00A45858);
GAME_VAR(float, g_A4585C, 0x00A4585C);
GAME_VAR(float, g_A45860, 0x00A45860);
GAME_VAR(float, g_A45864, 0x00A45864);
GAME_VAR(uint32_t, g_A45868, 0x00A45868);
GAME_VAR(float, g_A4586C, 0x00A4586C);
GAME_VAR(float, g_A45870, 0x00A45870);
GAME_VAR(float, g_A45874, 0x00A45874);
GAME_VAR(float, g_A45878, 0x00A45878);
GAME_VAR(float, g_A4587C, 0x00A4587C);
GAME_VAR(int32_t, g_A45880, 0x00A45880);   // the blend's duration out of the fight (ms)
GAME_VAR(uint32_t, g_A45884, 0x00A45884);
GAME_VAR(uint32_t, g_A45888, 0x00A45888);
GAME_VAR(float, g_A4588C, 0x00A4588C);
GAME_VAR(float, g_A45890, 0x00A45890);
GAME_VAR(int32_t, g_A45894, 0x00A45894);
GAME_VAR(int32_t, g_A45898, 0x00A45898);   // the blend's duration into the fight (ms)
// debug switches
GAME_VAR(int32_t, g_C3D0D8, 0x00C3D0D8);
GAME_VAR(uint8_t, g_C3D290, 0x00C3D290);
GAME_VAR(uint8_t, g_C3D291, 0x00C3D291);
GAME_VAR(uint8_t, g_C3D292, 0x00C3D292);
GAME_VAR(uint8_t, g_C3D293, 0x00C3D293);
GAME_VAR(int32_t, g_C3D294, 0x00C3D294);

constexpr double kPi = 3.1415927410125732;          // (float pi, as a double)
constexpr double kEpsilon = 9.999999747378752e-05;  // (0.0001f, as a double)

GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const CVector* v);
GAME_FN(CVector*, __thiscall, Entity_GetForward, 0x004154A0, void* entity, CVector* out);
GAME_FN(float, __cdecl, VecHeading, 0x00433190, const CVector* v);
GAME_FN(float, __cdecl, AngleDiff, 0x00432980, float a, float b);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int pad, int);
GAME_FN(uint8_t*, __thiscall, Ped_GetTarget, 0x00497100, void* ped);
GAME_FN(uint8_t*, __thiscall, Ped_Unknown4D5AD0, 0x004D5AD0, void* self);
// the follow camera's parts
GAME_FN(void, __thiscall, FollowCameraVehicle_Process, 0x004FFE20, void* self, float step);
GAME_FN(void, __thiscall, FollowCameraFight_Update, 0x004FFD10, void* self);
GAME_FN(void, __thiscall, FollowCameraFight_FindView, 0x00500650, void* self);
GAME_FN(void, __thiscall, FollowCameraBase_Smooth, 0x004FE590, void* self, float x, float y, float z);
GAME_FN(float, __thiscall, FollowCameraController3_GetDistance, 0x004FE110, void* self);
GAME_FN(float, __thiscall, FollowInterp_Get, 0x004FB3A0, void* self);
GAME_FN(uint8_t, __thiscall, FollowInterp_IsRising, 0x004FB3B0, void* self);
GAME_FN(int32_t, __thiscall, FollowInterp_IsMoving, 0x004FB3E0, void* self);
GAME_FN(void, __thiscall, FollowInterp_Update, 0x004FB400, void* self);
GAME_FN(void, __thiscall, FollowInterp_Start, 0x004FB4A0, void* self, float target, int duration, int flag);

using re::At;

uint8_t* Butes(void* self) {
    return At<uint8_t*>(self, 0x518);
}

// |v| as the game does it inline: the squared length to a float, fabs, sqrt, a float
float Length3(const CVector& v) {
    float squared = float((double(v.x) * v.x + double(v.y) * v.y) + double(v.z) * v.z);
    return float(std::sqrt(double(std::fabs(squared))));
}

float Length2(const CVector& v) {
    float squared = float(double(v.x) * v.x + double(v.y) * v.y);
    return float(std::sqrt(double(std::fabs(squared))));
}

// the blend's progress, 0..1 (NaN: 1)
float BlendProgress(uint8_t* self, uint32_t now) {
    float t = float(double(uint32_t(now - At<uint32_t>(self, 0x260))) / double(At<int32_t>(self, 0x264)));
    if (t < 0.0f) {
        return 0.0f;
    }
    if (t < 1.0f) {
        return t;
    }
    return 1.0f;
}

// 0x500B40
void __fastcall FollowCameraController3_Update(uint8_t* self, re::Edx) {
    // the vehicle part, and its position when the attribute set says so
    if (At<uint8_t>(self, 0x280)) {
        FollowCameraVehicle_Process(self + 0x278, At<float>(self, 0x28));
        if (At<uint8_t>(Butes(self), 0xBD)) {
            At<CVector>(self, 8) = At<CVector>(At<uint8_t*>(self, 0x27C), 0xE0);
        }
    }

    // whether to go to the fight view: the ped targeting with the button held, the
    // attribute set, or a ped close by with the stick still
    uint8_t fight = 0;
    uint8_t* target = Ped_GetTarget(TheCamera.targetPed);
    if (target && !(Pad_GetValue(10, 0) == 0.0f)) {
        fight = 1;
    } else if (At<uint8_t>(Butes(self), 0xBC)) {
        fight = 1;
    } else if (At<uint8_t>(Butes(self), 0xC3) && At<void*>(&TheCamera, 0x1B4) &&
               !(0.001f < std::fabs(At<float>(self, 0x20))) && !(0.001f < std::fabs(At<float>(self, 0x24)))) {
        fight = 1;
    }

    if (At<uint8_t>(self, 0x280)) {
        fight = 0;
    } else if (fight) {
        // the view as it is, for the fight part
        At<float>(self, 0x1E0) = At<float>(self, 0x28);
        At<float>(self, 0x1D4) = At<float>(self, 0x14);
        At<float>(self, 0x1D8) = At<float>(self, 0x18);
        At<CVector>(self, 0x1BC) = At<CVector>(self, 0xA4);
        At<float>(self, 0x1DC) = At<float>(self, 0x1C);
        At<CVector>(self, 0x1C8) = At<CVector>(self, 0x4C);
        FollowCameraFight_Update(self + 0x160);
        uint8_t findView = At<uint8_t>(Butes(self), 0xC2);
        CVector moved;
        VectorSub(&moved, &At<CVector>(self, 0x4C), &At<CVector>(self, 0xA4));
        bool look;
        if (int32_t(timeInMs - At<uint32_t>(self, 0x24C)) > g_A45894) {
            look = true;
        } else if (double(g_C3D294) > double(Length3(moved))) {
            look = true;
        } else {
            look = At<void*>(&TheCamera, 0x1B4) && double(At<float>(self, 0x50C)) > double(0.1f);
        }
        if (look) {
            CVector away;
            VectorSub(&away, &At<CVector>(self, 0x1E4), &At<CVector>(self, 0xA4));
            if (g_A45890 < Length3(away) && g_A4588C > Vector_Magnitude(&At<CVector>(&TheCamera, 0x180)) &&
                !(At<float>(self, 0x25C) == 0.0f) && timeInMs - At<uint32_t>(self, 0x26C) > g_A45888 &&
                timeInMs - At<uint32_t>(self, 0x250) > g_A45884 && findView) {
                FollowCameraFight_FindView(self + 0x160);
            }
        }
        if (At<uint8_t>(self, 0x254)) {
            At<uint8_t>(self, 0x114) = 1;
        }
    }
    if (!fight && At<uint8_t>(self, 0x254)) {
        At<uint8_t>(self, 0x254) = 0;
    }

    // the blend between the views
    uint32_t now = timeInMs;
    CVector fightFrom = At<CVector>(self, 0x1E4);
    CVector fightAt = At<CVector>(self, 0x1F0);
    if (g_C3D0D8 == 0) {
        int32_t intoDuration = g_A45898;
        if (fight && At<uint8_t>(self, 0x268)) {
            if (!At<uint8_t>(self, 0x269)) {
                // turned around mid-way: towards the fight from where it is
                float t = BlendProgress(self, now);
                At<int32_t>(self, 0x264) = intoDuration;
                uint32_t start = timeInMs;
                At<uint8_t>(self, 0x269) = 1;
                float remaining = float(1.0 - t);
                // (truncated, the low 32 bits)
                uint32_t elapsed = uint32_t(int64_t(double(remaining) * double(g_A45898)));
                At<uint32_t>(self, 0x260) = start - elapsed;
                At<uint32_t>(self, 0x26C) = start - elapsed;
                At<float>(self, 0x25C) = remaining;
                now = timeInMs;
            }
        }
        if (At<uint8_t>(self, 0x268)) {
            float t = BlendProgress(self, now);
            uint8_t towards = At<uint8_t>(self, 0x269);
            if (!towards) {
                t = float(1.0 - t);
            }
            float s = float(std::sin(double(float((double(t) - 0.5) * kPi))));
            At<float>(self, 0x25C) = float((double(s) + 1.0) * 0.5);
            if ((t <= 0.0f && !towards) || (1.0f <= t && towards)) {
                At<uint8_t>(self, 0x268) = 0;
                if (towards) {
                    At<float>(self, 0x25C) = 1.0f;
                } else {
                    At<float>(self, 0x25C) = 0.0f;
                    At<uint8_t>(self, 0x254) = 0;
                }
            }
        } else {
            // (not running: started when the target view changed)
            uint8_t atFight = At<float>(self, 0x25C) == 1.0f ? 1 : 0;
            if (fight != atFight) {
                At<uint32_t>(self, 0x260) = now;
                At<int32_t>(self, 0x264) = fight ? intoDuration : g_A45880;
                At<uint8_t>(self, 0x268) = 1;
                At<uint8_t>(self, 0x269) = fight;
                At<float>(self, 0x25C) = fight ? 0.0f : 1.0f;
                if (fight) {
                    At<float>(self, 0xF0) = g_A4587C;
                    At<uint32_t>(self, 0x26C) = At<uint32_t>(self, 0x260);
                    At<float>(self, 0xE8) = 0.0f;
                    At<uint32_t>(self, 0xEC) = timeInMs;
                }
            }
        }
    }

    // the smoothing rate (+0xA0) towards its goal
    uint8_t inVehicle = At<uint8_t>(self, 0x280);
    float goal = g_A45878;
    if (inVehicle) {
        goal = g_A45874;
    } else if (fight) {
        goal = g_A45870;
    }
    float rate = At<float>(self, 0xA0);
    if (goal > rate) {
        float next = float(double(At<float>(self, 0x28)) * g_A4586C + At<float>(self, 0xA0));
        At<float>(self, 0xA0) = next;
        if (next > goal) {
            At<float>(self, 0xA0) = goal;
        }
    } else if (At<float>(self, 0xA0) > goal) {
        float next = float(double(At<float>(self, 0xA0)) - double(At<float>(self, 0x28)) * g_A4586C);
        At<float>(self, 0xA0) = next;
        if (next < goal) {
            At<float>(self, 0xA0) = goal;
        }
    }
    At<float>(self, 0x9C) = inVehicle ? 0.75f : 0.2f;
    if (!(At<float>(self, 0x10C) == 0.0f) && timeInMs > uint32_t(At<uint32_t>(self, 0x110) + g_A45868)) {
        float next = float(double(At<float>(self, 0x10C)) - double(At<float>(self, 0x28)) * g_A45864);
        At<float>(self, 0x10C) = next;
        if (next < 0.0f) {
            At<float>(self, 0x10C) = 0.0f;
        }
    }

    // the look-from, smoothed; moving or standing (the interp at +0xF4)
    CVector* from = &At<CVector>(self, 8);
    FollowCameraBase_Smooth(self, from->x, from->y, from->z);
    CVector moved;
    VectorSub(&moved, from, &At<CVector>(self, 0xBC));
    At<CVector>(self, 0xBC) = *from;
    void* interp = self + 0xF4;
    if (Length2(moved) > 0.01f) {
        if (!(FollowInterp_Get(interp) == 1.0f) && !FollowInterp_IsRising(interp)) {
            FollowInterp_Start(interp, 1.0f, 30, 1);
        }
    } else {
        if (!(FollowInterp_Get(interp) == 0.0f) && !uint8_t(FollowInterp_IsMoving(interp))) {
            FollowInterp_Start(interp, 0.0f, 30, 1);
        }
    }
    FollowInterp_Update(interp);

    // the look-at target (+0x40): the smoothed look-from, held within butes +0x34 of
    // the look-at point horizontally, and its height within +0x38
    CVector centre = At<CVector>(self, 0x90);
    CVector* lookAt = &At<CVector>(self, 0xB0);
    CVector offset;
    VectorSub(&offset, &centre, lookAt);
    CVector* target40 = &At<CVector>(self, 0x40);
    if (0.0f == At<float>(Butes(self), 0x34)) {
        *target40 = centre;
    } else {
        float length = Length2(offset);
        if (At<float>(Butes(self), 0x34) > length) {
            *target40 = *lookAt;
        } else {
            CVector flat{offset.x, offset.y, 0.0f};
            Vector_Normalise(&flat);
            float limit = At<float>(Butes(self), 0x34);
            CVector back{float(double(flat.x) * limit), float(double(flat.y) * limit), float(double(limit) * flat.z)};
            CVector result;
            *target40 = *VectorSub(&result, &centre, &back);
        }
    }
    CVector result40;
    offset = *VectorSub(&result40, target40, lookAt);
    float heightLimit = At<float>(Butes(self), 0x38);
    float z48 = centre.z;
    if (!(kEpsilon > heightLimit)) {
        float absZ = std::fabs(offset.z);
        if (!(absZ < kEpsilon)) {
            if (heightLimit < absZ) {
                float sign = float(double(offset.z) / absZ);
                z48 = float(double(centre.z) - double(sign) * heightLimit);
            } else {
                z48 = At<float>(self, 0xB8);
            }
        }
    }
    At<float>(self, 0x48) = z48;

    // the turn: by the stick (+0x20 degrees), or after the ped / the vehicle / the point
    CVector* direction = &At<CVector>(self, 0xD0);
    CVector oldDirection = *direction;
    uint8_t turning = double(std::fabs(At<float>(self, 0x20))) > kEpsilon ? 1 : 0;
    if (fight && At<uint8_t>(Butes(self), 0xC1) == 0) {
        turning = 0;
    }
    float angle;
    if (!inVehicle && !At<uint8_t>(self, 0x115) && !At<uint8_t>(self, 0x2C0) && (fight || turning)) {
        angle = float(double(At<float>(self, 0x20)) * kPi / 180.0);
    } else {
        float maxRate = At<float>(Butes(self), 0xD8);
        float gain = At<float>(Butes(self), 0xDC);
        CVector want{0.0f, 0.0f, 0.0f};
        if (At<uint8_t>(self, 0x2C0)) {
            CVector d;
            want = *VectorSub(&d, &At<CVector>(self, 0x2F8), from);
        } else if (inVehicle) {
            uint8_t* driven = At<uint8_t*>(self, 0x27C);
            CVector d;
            CVector* r = VectorSub(&d, &At<CVector>(driven, 0xD4), &At<CVector>(driven, 0xE0));
            maxRate = At<float>(self, 0x290);
            gain = At<float>(self, 0x28C);
            want = *r;
        } else if (At<uint8_t>(self, 0x115)) {
            uint8_t* ped = static_cast<uint8_t*>(TheCamera.targetPed);
            CVector forward;
            want = *Entity_GetForward(ped, &forward);
            if (ped && (At<int32_t>(ped, 0x1474) != 0 || At<int32_t>(ped, 0x1470) != 0) && At<int32_t>(ped, 0x1488) != 0) {
                want.x = float(double(want.x) * -1.0);
                want.y = float(double(want.y) * -1.0);
                want.z = float(double(want.z) * -1.0);
            }
        } else {
            uint8_t* ped = static_cast<uint8_t*>(TheCamera.targetPed);
            CVector forward;
            if (At<int32_t>(ped, 0x1414) != 0 || At<int32_t>(ped, 0x13DC) != 0 || At<int32_t>(ped, 0x1438) != 0 ||
                At<int32_t>(ped, 0x1320) != 0) {
                CVector* f = Entity_GetForward(ped, &forward);
                want = {-f->x, -f->y, -f->z};
                maxRate = 0.0f;
                gain = 0.0f;
            } else {
                CVector* f = Entity_GetForward(ped, &forward);
                if ((At<int32_t>(ped, 0x1474) == 0 && At<int32_t>(ped, 0x1470) == 0) || At<int32_t>(ped, 0x1488) == 0) {
                    want = {-f->x, -f->y, -f->z};
                } else {
                    want = *f;
                }
                float d0 = At<float>(Butes(self), 0xD0);
                maxRate = float(FollowInterp_Get(interp) * d0);
                float d4 = At<float>(Butes(self), 0xD4);
                gain = float(FollowInterp_Get(interp) * d4);
            }
        }
        float wantHeading = VecHeading(&want);
        float heading = VecHeading(direction);
        angle = float(double(AngleDiff(heading, wantHeading)) * gain);
        if (!At<uint8_t>(self, 0x115)) {
            float most = float(double(maxRate) * kPi / 180.0 * At<float>(self, 0x28));
            float least = -most;
            if (least > angle) {
                angle = least;
            }
            if (angle > most) {
                angle = most;
            }
        }
        At<float>(self, 0x20) = float(180.0 * angle / kPi);
    }
    float s = float(std::sin(double(angle)));
    float c = float(std::cos(double(angle)));
    CVector rotated{float(double(c) * oldDirection.x - double(s) * oldDirection.y),
                    float(double(c) * oldDirection.y + double(s) * oldDirection.x), 0.0f};
    Vector_Normalise(&rotated);
    if (g_A45860 < std::fabs(angle) && std::fabs(At<float>(self, 0x88)) < 1e-5f && At<float>(self, 0x10C) == 0.0f &&
        At<uint8_t>(self, 0x254) == 0) {
        At<uint8_t>(self, 0x114) = 0;
    }

    // the height (+0xCC): the stick's pitch, pushed off a ped close by, eased back
    // inside the limits
    float pitch = At<float>(self, 0x24);
    *direction = rotated;
    inVehicle = At<uint8_t>(self, 0x280);
    if (inVehicle && At<uint8_t>(Butes(self), 0xBE)) {
        float up = float(double(At<float>(At<uint8_t*>(self, 0x27C), 0xDC)) - At<float>(self, 0xAC));
        pitch = float(double(up) * g_A4585C);
    }
    uint8_t pedClose = At<uint8_t>(self, 0x5C);
    if (pedClose) {
        uint8_t* butes = Butes(self);
        float excess = float(double(At<float>(butes, 0xAC)) - At<float>(self, 0x58));
        if (excess > 0.0f) {
            float top = At<float>(butes, 0xE8);
            float band = top > g_A45858 ? g_A45858 : top;
            if (!(band > 0.0f)) {
                band = 0.0f;
            }
            float push = float(double(float(double(excess) / At<float>(butes, 0xAC))) * At<float>(butes, 0xB0));
            double height = double(pitch) + At<float>(self, 0xCC);
            float pushed = float(double(push) + height);
            if (At<float>(butes, 0xE8) < pushed) {
                push = float(double(At<float>(butes, 0xE8)) - height);
                if (0.0f < push) {
                    pitch = float(double(pitch) + push);
                } else {
                    pitch = float(double(pitch) + 0.0);
                }
            } else if (double(At<float>(butes, 0xE8)) - band < pushed) {
                float part = float((double(At<float>(butes, 0xE8)) - pushed) / band);
                push = float(double(push) * part);
                pitch = float(double(pitch) + push);
            } else {
                pitch = float(double(pitch) + push);
            }
        }
    }
    {
        uint8_t* butes = Butes(self);
        float below = float(double(At<float>(self, 0xCC)) - At<float>(butes, 0x70));
        if (!(below > 0.0f)) {
            below = 0.0f;
        }
        float above = float(double(At<float>(butes, 0x74)) - At<float>(self, 0xCC));
        if (!(above > 0.0f)) {
            above = 0.0f;
        }
        float room = pitch < 0.0f ? below : above;
        float scale = float(double(room) / At<float>(butes, 0x78));
        if (!(scale < 1.0f)) {
            scale = 1.0f;
        }
        pitch = float(double(pitch) * scale);
        if (((!fight && !inVehicle) || !At<uint8_t>(butes, 0xBE)) && double(std::fabs(At<float>(self, 0x24))) <= kEpsilon &&
            !pedClose && !(FollowInterp_Get(interp) == 0.0f)) {
            float toDistance = float(double(FollowCameraController3_GetDistance(self)) - At<float>(self, 0xCC));
            float k = At<float>(Butes(self), 0x7C);
            float moving = FollowInterp_Get(interp);
            float add = float(double(toDistance) * k * moving);
            pitch = float(double(add) + pitch);
        }
    }
    float height = float(double(pitch) + At<float>(self, 0xCC));
    float correction = 0.0f;
    if (At<float>(Butes(self), 0x70) > height) {
        correction = float(double(At<float>(Butes(self), 0x70)) - height);
    } else if (At<float>(Butes(self), 0x74) < height) {
        correction = float(double(At<float>(Butes(self), 0x74)) - height);
    }
    float correctionRate = float(double(timeStep) * g_A45854);
    if (correctionRate > 1.0f) {
        correctionRate = 1.0f;
    }
    float heightDelta = float(double(correction) * correctionRate + pitch);

    // the distance (+0xC8)
    CVector toCamera;
    VectorSub(&toCamera, &At<CVector>(self, 0xA4), lookAt);
    float distance = At<float>(self, 0xC8);
    if (!At<uint8_t>(self, 0x280) && !At<uint8_t>(self, 0x2C0)) {
        float flat = Length2(toCamera);
        float dot = float(double(At<float>(self, 0xD4)) * offset.y + double(offset.x) * At<float>(self, 0xD0) +
                          double(At<float>(self, 0xD8)) * 0.0);
        float away = -dot;
        uint8_t* butes = Butes(self);
        float over = float(double(flat) - At<float>(butes, 0x48));
        if (!(over > 0.0f)) {
            over = 0.0f;
        }
        float under = float(double(At<float>(butes, 0x4C)) - flat);
        if (!(under > 0.0f)) {
            under = 0.0f;
        }
        float room = away < 0.0f ? over : under;
        float scale = float(double(room) / At<float>(butes, 0x50));
        if (!(scale < 1.0f)) {
            scale = 1.0f;
        }
        float weight = float((1.0 - At<float>(self, 0x25C)) * scale);
        float add = float(double(away) * weight);
        distance = float(double(add) + distance);
    }
    if (At<float>(Butes(self), 0x4C) < distance) {
        float excess = float(double(distance) - At<float>(Butes(self), 0x4C));
        float k = At<float>(Butes(self), 0x54);
        float moving = FollowInterp_Get(interp);
        float add = float(-double(excess) * k * moving * At<float>(self, 0x28));
        distance = float(double(add) + distance);
    }
    if (((At<uint8_t>(self, 0x114) == 0 && At<uint8_t>(self, 0x280) == 0) || g_C3D293) &&
        (std::fabs(At<float>(self, 0x88)) < 1e-5f || g_C3D292)) {
        float flat = Length2(toCamera);
        uint8_t* butes = Butes(self);
        float gap = float(double(At<float>(butes, 0x80)) - flat);
        if ((gap > 0.0f || g_C3D291) && (At<float>(self, 0x10C) == 0.0f || g_C3D290)) {
            float fraction = float(double(gap) / At<float>(butes, 0x80));
            float k = float(double(fraction) * At<float>(butes, 0x84));
            float kFlat = float(double(k) * flat);
            float add = float((1.0 - At<float>(self, 0x25C)) * kFlat);
            distance = float(double(add) + distance);
            float want = FollowCameraController3_GetDistance(self);
            float r = float(double(k) * g_A45850 * At<float>(self, 0x28));
            if (!(r < 1.0f)) {
                r = 1.0f;
            }
            float step = float((double(want) - At<float>(self, 0xCC)) * r);
            At<float>(self, 0xCC) = float(double(step) + At<float>(self, 0xCC));
        }
    }
    if (At<uint8_t>(self, 0x280) && At<uint8_t>(Butes(self), 0xC0) && !At<uint8_t>(self, 0x2C0)) {
        float scaled = float(double(At<float>(self, 0x28)) * 150.0);
        float most = scaled < 1.0f ? scaled : 1.0f;
        float moving = FollowInterp_Get(interp);
        if (At<int32_t>(self, 0x510) != 0) {
            At<uint32_t>(self, 0x288) = timeInMs;
        }
        float wanted = At<float>(At<uint8_t*>(self, 0x27C), 0xF0);
        float gap = float(double(wanted) - distance);
        float follow = float(double(moving) * most * gap);
        if (timeInMs > uint32_t(At<uint32_t>(self, 0x288) + g_A4584C)) {
            float next = float(double(g_A45848) * At<float>(self, 0x28) + At<float>(self, 0x284));
            At<float>(self, 0x284) = next;
            if (1.0f < next) {
                At<float>(self, 0x284) = 1.0f;
            }
        } else {
            float next = float(double(At<float>(self, 0x284)) - double(g_A45844) * At<float>(self, 0x28));
            At<float>(self, 0x284) = next;
            if (0.0f > next) {
                At<float>(self, 0x284) = 0.0f;
            }
        }
        float add = float(double(follow) * At<float>(self, 0x284));
        distance = float(double(distance) + add);
    }
    if (At<uint8_t>(self, 0x31)) {
        float high = g_A45840;
        if (high > distance) {
            float low = g_A4583C;
            float r = float((double(distance) - low) / (double(high) - low));
            if (!(0.0f < r)) {
                r = 0.0f;
            }
            float g = float(double(float(1.0 - r)) * g_A45838 * At<float>(self, 0x28));
            if (g < 1.0f) {
                distance = float(double(distance) + g);
            } else {
                distance = float(double(distance) + 1.0);
            }
        }
    }
    if (At<uint8_t>(self, 0x2C0)) {
        CVector d;
        VectorSub(&d, &At<CVector>(self, 0x2F8), from);
        float gap = float(double(Length2(d)) - distance);
        if (gap > 0.01f) {
            float r = float(double(At<float>(self, 0x28)) * g_A45834);
            if (!(r < 1.0f)) {
                r = 1.0f;
            }
            float add = float(double(gap) * r);
            distance = float(double(add) + distance);
        }
    }
    At<float>(self, 0xCC) = float(double(heightDelta) + At<float>(self, 0xCC));
    if (!(distance > kEpsilon)) {
        distance = 0.0001f;
    }
    At<float>(self, 0xC8) = distance;

    // the camera position
    CVector* camera = &At<CVector>(self, 0x34);
    {
        CVector out{float(double(distance) * direction->x), float(double(direction->y) * distance),
                    float(double(distance) * direction->z)};
        CVector r;
        *camera = *VectorAdd(&r, lookAt, &out);
        At<CVector>(self, 0x4C) = *from;
        camera->z = float(double(camera->z) + At<float>(self, 0xCC));
    }
    float blend = At<float>(self, 0x25C);
    if (!(blend == 0.0f) && At<uint8_t>(Butes(self), 0xBD)) {
        At<float>(self, 0x4C) = float((double(fightAt.x) - from->x) * At<float>(self, 0x25C) + At<float>(self, 0x4C));
        At<float>(self, 0x50) = float((double(fightAt.y) - from->y) * At<float>(self, 0x25C) + At<float>(self, 0x50));
        if (At<uint8_t>(Butes(self), 0xBF)) {
            At<float>(self, 0x54) = float((double(fightAt.z) - from->z) * At<float>(self, 0x25C) + At<float>(self, 0x54));
        }
    }
    if (At<float>(self, 0x25C) == 0.0f) {
        At<CVector>(self, 0xDC) = *camera;
        return;
    }

    // blending with the fight view
    CVector* lookFrom = &At<CVector>(self, 0xA4);
    if (At<uint8_t>(self, 0x254)) {
        float w = float(double(g_A45830) * At<float>(self, 0x25C) * At<float>(self, 0x28));
        if (!(w < 1.0f)) {
            w = 1.0f;
        }
        CVector d;
        CVector* toFight = VectorSub(&d, &fightFrom, lookFrom);
        CVector part{float(double(w) * toFight->x), float(double(toFight->y) * w), float(double(toFight->z) * w)};
        CVector moveTo;
        VectorAdd(&moveTo, lookFrom, &part);
        CVector d2;
        CVector* fromAt = VectorSub(&d2, &moveTo, lookAt);
        CVector flat{fromAt->x, fromAt->y, 0.0f};
        Vector_Normalise(&flat);
        CVector d3;
        CVector* full = VectorSub(&d3, &moveTo, lookAt);
        float dot = float((double(full->y) * flat.y + double(full->x) * flat.x) + double(full->z) * flat.z);
        *direction = flat;
        At<float>(self, 0xCC) = float(double(moveTo.z) - At<float>(self, 0xB8));
        At<float>(self, 0xC8) = dot;
        Vector_Normalise(direction);
        float p = At<float>(self, 0xC8);
        CVector out{float(double(p) * direction->x), float(double(direction->y) * p), float(double(p) * direction->z)};
        CVector r;
        *camera = *VectorAdd(&r, lookAt, &out);
        camera->z = float(double(camera->z) + At<float>(self, 0xCC));
        At<CVector>(self, 0xDC) = *camera;
        return;
    }
    CVector blendFrom = At<CVector>(self, 0xDC);
    uint8_t* ped = static_cast<uint8_t*>(TheCamera.targetPed);
    float speed = Length3(At<CVector>(ped, 0x124));
    uint8_t* other = At<uint8_t*>(&TheCamera, 0x1B0);
    if (!other) {
        if (Ped_Unknown4D5AD0(At<void*>(ped, 0x1C8))) {
            other = Ped_Unknown4D5AD0(At<void*>(ped, 0x1C8));
        } else {
            other = At<uint8_t*>(&TheCamera, 0x1B4);
        }
    }
    if (other && At<int32_t>(other, 0xF0) != 0) {
        speed = float(double(Vector_Magnitude(&At<CVector>(other, 0x124))) + speed);
    }
    if (At<float>(self, 0xE8) < speed) {
        At<float>(self, 0xE8) = speed;
        At<uint32_t>(self, 0xEC) = timeInMs;
    } else if (double(g_A4582C) < double(uint32_t(timeInMs - At<uint32_t>(self, 0xEC)))) {
        float next = float(double(At<float>(self, 0xE8)) - double(At<float>(self, 0x28)) * g_A45828);
        At<float>(self, 0xE8) = next;
        if (0.0f > next) {
            At<float>(self, 0xE8) = 0.0f;
        }
    }
    float rateGoal = g_A45824 < At<float>(self, 0xE8) ? g_A45820 : g_A4587C;
    At<float>(self, 0xF0) = float((double(rateGoal) - At<float>(self, 0xF0)) * double(0.1f) + At<float>(self, 0xF0));
    float w = float(double(At<float>(self, 0xF0)) * At<float>(self, 0x25C) * At<float>(self, 0x28));
    if (!(w < 1.0f)) {
        w = 1.0f;
    }
    CVector d;
    CVector* fromAt = VectorSub(&d, &blendFrom, lookAt);
    CVector flat{fromAt->x, fromAt->y, 0.0f};
    Vector_Normalise(&flat);
    CVector d2;
    CVector* full = VectorSub(&d2, &blendFrom, lookAt);
    float dot = float((double(full->y) * flat.y + double(full->x) * flat.x) + double(full->z) * flat.z);
    float blendHeight = float(double(blendFrom.z) - At<float>(self, 0xB8));
    At<float>(self, 0xCC) = float(double(w) * (double(blendHeight) - At<float>(self, 0xCC)) + At<float>(self, 0xCC));
    At<float>(self, 0xC8) = float((double(dot) - At<float>(self, 0xC8)) * w + At<float>(self, 0xC8));
    CVector d3;
    CVector* turn = VectorSub(&d3, &flat, direction);
    CVector step{float(double(w) * turn->x), float(double(turn->y) * w), float(double(w) * turn->z)};
    direction->x = float(double(step.x) + direction->x);
    direction->y = float(double(direction->y) + step.y);
    direction->z = float(double(direction->z) + step.z);
    Vector_Normalise(direction);
    float p = At<float>(self, 0xC8);
    CVector out{float(double(p) * direction->x), float(double(direction->y) * p), float(double(p) * direction->z)};
    CVector r;
    *camera = *VectorAdd(&r, lookAt, &out);
    camera->z = float(double(camera->z) + At<float>(self, 0xCC));
    // (the fight's look-from, not the camera, is what's blended from next time)
    At<CVector>(self, 0xDC) = fightFrom;
}
FOLLOWCAMBIG_REVERSED("FollowCameraController3::Update500B40", 0x00500B40, RE_BODY(&FollowCameraController3_Update), nullptr, nullptr, re::Mode::Original);

} // namespace
