// The action tree's ControllerTrack (the player's stick moving the ped): its Update,
// 0x62AF40.
// The track: +0x18 the pad, +0x20 the owner, +0x24 strafing (the heading held while
// the stick circles), +0x28 / +0x2C the stick's side / forward last frame, +0x30 / +0x34
// the stick when the strafe began, +0x38 the camera's heading less the stick's then,
// +0x3C the ped's heading, +0x40 the strafe's turn rate, +0x44 its correction.
// The ped's action inputs (+0x2E0): +0 the move direction, +0xC.. the facing, +0x1C the
// speed; buttons 0xF..0x12 forward / back / left / right.
// Register-hazard check (tools/regcheck/regcheck2.py, chunk 31): no caller keeps ecx/edx.

#include <cmath>
#include <cstdint>

#include "game/ActionTracksCommon.h"
#include "game/TrackMath.h"

using namespace actc;

namespace {

constexpr float kHalfPi = 1.5707963705062866f;
constexpr float kPi = 3.1415927410125732f;
constexpr float kDeadZone = 0.19634954631328583f;   // pi/16

inline void* const padA45A30 = reinterpret_cast<void*>(0x00A45A30);
inline void* const theCamera = reinterpret_cast<void*>(0x00C3CC68);
GAME_VAR(uint8_t*, controllerPed0, 0x00C2B120);
GAME_VAR(uint8_t*, controllerPed1, 0x00C2B124);
GAME_VAR(uint8_t, lockButtonInputs, 0x00C1AF8C);
GAME_VAR(uint8_t, lockExceptSide, 0x00C1AF9D);      // (control 0x10 still read)
GAME_VAR(uint8_t, lockExceptForward, 0x00C1AF9E);   // (control 0x11 still read)
GAME_VAR(uint8_t, altSteering, 0x00A14925);
GAME_VAR(uint8_t, useCameraHeading, 0x00A14926);
GAME_VAR(float, cameraHeading, 0x00C3CD08);
GAME_VAR(int32_t, cameraMode, 0x00C3CCF8);
GAME_VAR(uint8_t*, cameraSettings, 0x020C88B8);    // +0x60 the fov (degrees), +0xEC the stick's turn scale
GAME_VAR(float, stickSideSent, 0x01B656B0);
GAME_VAR(float, stickForwardSent, 0x01B656AC);
GAME_VAR(uint32_t, headingOffsetGuard, 0x01B656A8);   // (a function static's guard)
GAME_VAR(float, headingOffset, 0x01B656A4);
GAME_VAR(float, headingOffsetInit, 0x00B0C09C);

GAME_FN(uint8_t, __thiscall, Pad_IsEnabled, 0x0050DF50, void* pad);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(int32_t, __cdecl, Pad_GetActive, 0x00738070);
GAME_FN(uint8_t, __cdecl, Pad_738500, 0x00738500, int32_t pad);
GAME_FN(uint8_t, __thiscall, Ped_473E10, 0x00473E10, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Ped_IsSeatedInVehicle, 0x004CE690, uint8_t* ped);
GAME_FN(float, __thiscall, Entity_GetHeading, 0x00415470, uint8_t* entity);
GAME_FN(float, __thiscall, Camera_GetHeading, 0x004F1310, void* camera);
GAME_FN(uint8_t, __thiscall, Camera_IsFollowFlag30, 0x004F1640, void* camera);
GAME_FN(void, __thiscall, ActionInputs_SetRequest, 0x005F3A90, uint8_t* inputs, int button, int kind, float time);
GAME_FN(uint8_t*, __thiscall, ActionInputs_GetRequest, 0x005F3720, uint8_t* inputs, int button);
GAME_FN(float, __cdecl, LimitAngle, 0x00432EE0, float angle);
GAME_FN(float, __cdecl, AngleDifference, 0x00432980, float a, float b);
GAME_FN(float, __cdecl, MaxF, 0x00414490, float a, float b);
GAME_FN(float, __cdecl, MinF, 0x00414470, float a, float b);
GAME_FN(float, __cdecl, Clamp, 0x0041A470, float value, float low, float high);
GAME_FN(float, __cdecl, SqrtAbsFn, 0x0040DF90, float value);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const V3* v);
GAME_FN(V3*, __cdecl, NormalisedCopy, 0x00478820, V3* out, const V3* v);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const V3* a, const V3* b);
GAME_FN(uint8_t, __cdecl, NearlyEqual, 0x0061A8C0, float a, float b, float epsilon);
GAME_FN(uint8_t, __cdecl, Dodgeball_IsActive, 0x00683A30);

// a control's value unless the buttons are locked (and it isn't the exception)
float ControlValue(int control, int pad, uint8_t except) {
    return !lockButtonInputs || except ? Pad_GetValue(control, pad) : 0.0f;
}
// the camera's heading, less a quarter turn
float CameraHeadingLessQuarter() {
    if (!useCameraHeading && !Pad_738500(Pad_GetActive())) {
        return cameraHeading - kHalfPi;
    }
    return Camera_GetHeading(theCamera) - kHalfPi;
}
// small angles (within pi/16) eased toward 0
float EaseSmallAngle(float a) {
    if (kDeadZone > a && a > -kDeadZone) {
        float q = std::fabs(a / kDeadZone);
        float f = a / (kPi / (q * 8.0f + 8.0f));
        float f2 = f * f;
        return a * f2;
    }
    return a;
}
void ReleaseDirections(uint8_t* inputs) {
    ActionInputs_SetRequest(inputs, 0xF, 3, 0.0f);
    ActionInputs_SetRequest(inputs, 0x10, 3, 0.0f);
    ActionInputs_SetRequest(inputs, 0x11, 3, 0.0f);
    ActionInputs_SetRequest(inputs, 0x12, 3, 0.0f);
}

// 0x62AF40 (thiscall): ControllerTrack::Update: none without the owner's ped or its pad's
// ped; the stick (forward 0x11, side 0x10, or with the alternate steering in a vehicle
// or 0x473E10 also 0x12) read unless the buttons are locked. Released: the direction
// buttons released and the strafe dropped. Else the stick turned into a direction from
// the camera's heading (or the vehicle's, or the train's offset, or held while strafing:
// the strafe begins with 0x12 held, the stick steady and along the ped's heading, and
// ends when it moves off), the direction buttons pressed for its main axis and the
// speed from its size (0.11..0.49, or 0.01..0.99 in dodgeball or a vehicle). The
// direction is set when off by more than 0.999; the facing cleared. 1.
uint8_t __fastcall ControllerTrack_UpdateImpl(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x20));
    if (!ped) {
        return 0;
    }
    int32_t pad = At<int32_t>(self, 0x18);
    if (pad == 0) {
        if (!controllerPed0) {
            return 0;
        }
    } else if (pad == 1 && !controllerPed1) {
        return 0;
    }
    float* move = At<float*>(ped, 0x2E0);
    uint8_t* inputs = reinterpret_cast<uint8_t*>(move);
    if (!Pad_IsEnabled(padA45A30)) {
        return 1;
    }
    float forward = 0.0f;
    if (Pad_IsEnabled(padA45A30)) {
        forward = ControlValue(0x11, pad, lockExceptForward);
    }
    float forwardUsed = forward;
    float side;
    if (altSteering && !Pad_738500(Pad_GetActive()) &&
        (At<uint8_t*>(ped, 0x1554) || Ped_473E10(ped))) {
        float left = 0.0f;
        if (Pad_IsEnabled(padA45A30)) {
            left = -ControlValue(0x10, pad, lockExceptSide);
        }
        float right = 0.0f;
        if (Pad_IsEnabled(padA45A30)) {
            right = -Pad_GetValue(0x12, pad);
        }
        side = right + left;
    } else {
        float v = 0.0f;
        if (Pad_IsEnabled(padA45A30)) {
            v = -ControlValue(0x10, pad, lockExceptSide);
        }
        side = v;
    }
    float dirX = move[0];
    float dirY = move[1];
    float speed = 0.0f;
    if (side == 0.0f && forward == 0.0f) {
        ReleaseDirections(inputs);
        stickSideSent = side;
        At<uint8_t>(self, 0x24) = 0;
        stickForwardSent = forward;
        At<float>(self, 0x28) = 0.0f;
        At<float>(self, 0x2C) = 0.0f;
        At<float>(self, 0x3C) = Entity_GetHeading(ped);
        At<float>(self, 0x40) = 0.0f;
        At<float>(self, 0x44) = 0.0f;
    } else {
        if (!At<uint8_t>(self, 0x24)) {
            float cameraAngle = -LimitAngle(CameraHeadingLessQuarter());
            V3 stick = {side, forward, 0.0f};
            float size = Vector_Magnitude(&stick);
            float a = EaseSmallAngle(-trk::CrtAtan2(side, forward));
            float stickAngle = LimitAngle(a + cameraAngle);
            float pedAngle = LimitAngle(Entity_GetHeading(ped) + kHalfPi);
            if (cameraMode == 1 && (useCameraHeading || Pad_738500(Pad_GetActive())) && !Ped_GetTarget(ped) &&
                !At<uint8_t*>(ped, 0x1554) && !At<int32_t>(ped, 0x1474)) {
                float held = 0.0f;
                if (Pad_IsEnabled(padA45A30)) {
                    held = -Pad_GetValue(0x12, pad);
                }
                if (std::fabs(held) > 0.01f && 0.005f > std::fabs(side - At<float>(self, 0x28)) &&
                    0.005f > std::fabs(forward - At<float>(self, 0x2C)) && std::fabs(stickAngle - pedAngle) < 0.15f &&
                    0.3f < size && std::fabs(LimitAngle(Entity_GetHeading(ped) - At<float>(self, 0x3C))) < 0.14f) {
                    At<uint8_t>(self, 0x24) = 1;
                    At<float>(self, 0x30) = side;
                    At<float>(self, 0x34) = forward;
                    At<float>(self, 0x40) = side * 0.035f;
                    At<float>(self, 0x44) = 0.0f;
                    At<float>(self, 0x38) = cameraAngle - stickAngle;
                }
            }
        } else {
            float held = 0.0f;
            if (Pad_IsEnabled(padA45A30)) {
                held = -Pad_GetValue(0x12, pad);
            }
            bool drop;
            if (0.005f > std::fabs(held) && (std::fabs(side - At<float>(self, 0x28)) > 0.05f ||
                                             std::fabs(forward - At<float>(self, 0x2C)) > 0.05f)) {
                drop = true;
            } else if (std::fabs(side) < 0.005f && 0.005f > std::fabs(forward)) {
                drop = true;
            } else if (Ped_GetTarget(ped) || At<uint8_t*>(ped, 0x1554) || At<int32_t>(ped, 0x1474) == 1 ||
                       cameraMode != 1) {
                drop = true;
            } else {
                V3 last = {At<float>(self, 0x28), At<float>(self, 0x2C), 0.0f};
                V3 now = {side, forward, 0.0f};
                V3 lastN;
                NormalisedCopy(&lastN, &last);
                drop = DotProduct(NormalisedCopy(&last, &now), &lastN) < 0.92f;
                if (!drop) {
                    float begun = trk::CrtAtan2(At<float>(self, 0x30), At<float>(self, 0x34));
                    float current = trk::CrtAtan2(side, forward);
                    float c = Clamp(LimitAngle(current - begun) * 0.08f, -0.08f, 0.08f);
                    At<float>(self, 0x44) = c;
                    float rate = At<float>(self, 0x40);
                    if (std::fabs(c) > 0.05f && std::fabs(rate) > 0.02f) {
                        int was = 0.0f > rate ? -1 : 1;
                        int now2 = c + At<float>(self, 0x40) < 0.0f ? -1 : 1;
                        if (was != now2) {
                            At<uint8_t>(self, 0x24) = 0;
                        }
                    }
                }
            }
            if (drop) {
                At<uint8_t>(self, 0x24) = 0;
            }
        }
        At<float>(self, 0x28) = side;
        At<float>(self, 0x2C) = forward;
        if (At<uint8_t>(self, 0x24)) {
            V3 now = {side, forward, 0.0f};
            float size = Vector_Magnitude(&now);
            side = size * At<float>(self, 0x30);
            forwardUsed = size * At<float>(self, 0x34);
        }
        uint8_t* m = At<uint8_t*>(ped, 0x14);
        At<float>(self, 0x3C) = m ? trk::CrtAtan2(-At<float>(m, 0x10), At<float>(m, 0x14)) : At<float>(ped, 0x10);
        float base = -LimitAngle(CameraHeadingLessQuarter());
        if (At<uint8_t*>(ped, 0x1414) && !At<uint8_t*>(ped, 0x1438)) {
            if (!(headingOffsetGuard & 1)) {
                headingOffsetGuard |= 1;
                headingOffset = headingOffsetInit;
            }
            base = Entity_GetHeading(ped) + headingOffset;
        }
        if (uint8_t* vehicle = At<uint8_t*>(ped, 0x1554)) {
            base = Entity_GetHeading(vehicle) + kHalfPi;
        } else if (At<uint8_t>(self, 0x24) == 1) {
            float h = Entity_GetHeading(ped) + kHalfPi + At<float>(self, 0x38) -
                      (At<float>(self, 0x44) + At<float>(self, 0x40)) * timeStep;
            base = LimitAngle(h);
        } else if (At<int32_t>(ped, 0x1474) || At<int32_t>(ped, 0x1480)) {
            base = LimitAngle(Entity_GetHeading(ped) + kHalfPi);
        }
        float angle = -trk::CrtAtan2(side, forwardUsed);
        if (Camera_IsFollowFlag30(theCamera) && !Ped_IsSeatedInVehicle(ped) && !At<uint8_t>(self, 0x24)) {
            float fov = At<float>(cameraSettings, 0x60) * kPi / 180.0f;
            float d = AngleDifference(0.0f, angle);
            angle = LimitAngle(MinF(MaxF(At<float>(cameraSettings, 0xEC) * d, -fov), fov)) * -1.0f;
            float size = SqrtAbsFn(forwardUsed * forwardUsed + side * side);
            side = trk::CrtCos(angle) * size;
            forwardUsed = trk::CrtSin(angle) * size;
        }
        if (!At<uint8_t*>(ped, 0x1554)) {
            angle = EaseSmallAngle(angle);
        }
        angle = LimitAngle(angle + base);
        dirX = trk::CrtCos(angle);
        dirY = trk::CrtSin(angle);
        float size = SqrtAbs(forwardUsed * forwardUsed + side * side);
        if (0.5f < size) {
            if (!NearlyEqual(side, stickSideSent, 0.05f) || !NearlyEqual(forwardUsed, stickForwardSent, 0.05f)) {
                stickSideSent = side;
                stickForwardSent = forwardUsed;
                int button;
                if (std::fabs(side) > std::fabs(forwardUsed)) {
                    button = side >= 0.0f ? 0x11 : 0x12;
                } else {
                    button = forwardUsed >= 0.0f ? 0xF : 0x10;
                }
                for (int i = 0xF; i <= 0x12; ++i) {
                    if (i == button) {
                        uint8_t* request = ActionInputs_GetRequest(inputs, i);
                        if (request && (*request & 3) == 0) {
                            ActionInputs_SetRequest(inputs, i, 2, 0.0f);
                        }
                    } else {
                        ActionInputs_SetRequest(inputs, i, 3, 0.0f);
                    }
                }
            }
        } else {
            ReleaseDirections(inputs);
        }
        float low = 0.1f;
        float high = 0.5f;
        if (Dodgeball_IsActive() || At<uint8_t*>(ped, 0x1554)) {
            low = 0.0f;
            high = 1.0f;
        }
        low = low + 0.01f;
        speed = low + (high - 0.01f - low) * size;
    }
    float along = dirX * move[0] + dirY * move[1] + move[2] * 0.0f;
    if (along < 0.999f) {
        move[0] = dirX;
        move[1] = dirY;
        move[2] = 0.0f;
    }
    move[7] = speed;
    move[3] = 0.0f;
    move[4] = 0.0f;
    move[5] = 0.0f;
    return 1;
}

}  // namespace

#define PLAYERCTRLTRK_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PLAYERCTRLTRK_REVERSED("ControllerTrack::Update", 0x0062AF40, &ControllerTrack_UpdateImpl);
