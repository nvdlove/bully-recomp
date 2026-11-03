// The action tree's bike tracks of 0x659230..0x6599F9: pedalling (the bike's pedal state
// and speed from the stick's direction against the rider's) and steering (the stick
// eased through a power curve into the bike's steer, the lean blended).
//
// A bike (on top of the vehicle's fields, BikeTracks.cpp): +0x294 the pedal input, +0x298
// the brake, +0x29C the pedalling rate, +0x2A0 its lean, +0x2A4 its butes (+0x84 top
// speed, +0x88 reverse), +0x360 / +0x6B4 the steer, +0x694 / +0x69C the pedal speed,
// +0x6A0 the pedal state, +0x6A8 its turn.

#include <cmath>
#include <cstdint>

#include "game/ActionTracksCommon.h"
#include "game/TrackMath.h"

#include "game/Crt.h"
using namespace actc;

namespace {

GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_FN(V3*, __cdecl, VectorScale, 0x00412350, V3* out, const V3* v, float s);
GAME_FN(V3*, __thiscall, Vehicle_GetDriverMoveDirection, 0x004CCC00, uint8_t* vehicle, V3* out);
GAME_FN(float, __cdecl, AngleBetweenPoints, 0x00432D10, float x1, float y1, float x2, float y2);
GAME_FN(float, __cdecl, LimitRadianAngle, 0x00432EE0, float angle);
GAME_FN(uint8_t, __thiscall, UserInput_IsEnabled, 0x0050DF50, void* input);
GAME_FN(int32_t, __cdecl, Pad_GetActive, 0x00738070);
GAME_FN(uint8_t, __cdecl, Pad_738500, 0x00738500, int32_t pad);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(float, __thiscall, Bike_GetSteerLock, 0x004BCA60, uint8_t* bike);

GAME_VAR(uint8_t, altSteering, 0x00A14925);
inline void* const userInput = reinterpret_cast<void*>(0x00A45A30);
inline const float* const yAxis = reinterpret_cast<const float*>(0x009E33D0);   // (0, 1, 0)

float CrtSqrt(float x) {
    return crt::Sqrtf(x);
}

// the bike's speed along its facing
float ForwardSpeed(uint8_t* bike) {
    V3 f;
    V3* forward = Placeable_GetForward(bike, &f);
    return At<float>(bike, 0x128) * forward->y + At<float>(bike, 0x124) * forward->x + At<float>(bike, 0x12C) * forward->z;
}

// the pedal state flipped on from rest: pedalling at the idle speed, the input's sign
void StartPedalling(uint8_t* bike, float& input) {
    if (At<float>(bike, 0x6A0) == 0.0f) {
        At<float>(bike, 0x6A0) = 1.0f;
        At<float>(bike, 0x69C) = At<float>(bike, 0x694);
        input = float(0.0f > input ? -1 : 1);
    }
}

// 0x659230 (thiscall): PedalTrack::Update: +0x30 set: pedalling at the rate, an AI's
// bike pushed on at its top speed (+0x2C of the butes'); else the stick's push (squared)
// pedals forwards within +0x20 of the rider's facing and backwards within +0x24 of its
// back (braking (+0x28 the push past which) when rolling the other way), in between (or
// too light) coasting. The pedal speed kept between the reverse and top speeds
uint8_t __fastcall Pedal_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x34));
    uint8_t* bike = At<uint8_t*>(ped, 0x1554);
    if (!bike || At<int32_t>(bike, 0x46C) != 1) {
        return 1;
    }
    float input = At<float>(bike, 0x294);
    float brake = 0.0f;
    float along = ForwardSpeed(bike);
    uint8_t* butes = At<uint8_t*>(bike, 0x2A4);
    float top = At<float>(butes, 0x84) * At<float>(self, 0x2C);
    float reverse = At<float>(butes, 0x88);
    if (self[0x30] != 0) {
        At<float>(bike, 0x294) = At<float>(bike, 0x29C);
        At<float>(bike, 0x298) = 0.0f;
        if (At<int32_t>(ped, 0x1310) == 0xD || bike[0x6EC] == 0) {
            return 1;
        }
        V3 f;
        V3* forward = Placeable_GetForward(bike, &f);
        V3 v;
        V3* speed = VectorScale(&v, forward, top);
        At<V3>(bike, 0x124) = *speed;
        At<float>(bike, 0x69C) = top;
        return 1;
    }
    V3 direction;
    Vehicle_GetDriverMoveDirection(bike, &direction);
    float push = At<float>(At<uint8_t*>(ped, 0x2E0), 0x1C);
    float pushSq = push * push;
    float angle = LimitRadianAngle(AngleBetweenPoints(direction.x, direction.y, 0.0f, 0.0f));
    if (0.1f < pushSq) {
        float forwardArc = At<float>(self, 0x20);
        if (-forwardArc <= angle && forwardArc >= angle) {
            At<float>(bike, 0x69C) = top;
            At<float>(bike, 0x6A0) = 0.0f;
            if (along < -0.1f && At<float>(self, 0x28) < At<float>(At<uint8_t*>(ped, 0x2E0), 0x1C)) {
                input = 0.0f;
                brake = 1.0f;
            } else {
                input = pushSq;
            }
        } else {
            bool backwards = true;
            float low = At<float>(self, 0x24) - 3.14159274f;
            if (!(low >= angle)) {
                float high = 3.14159274f - At<float>(self, 0x24);
                if (!(high <= angle)) {
                    backwards = false;
                    StartPedalling(bike, input);
                }
            }
            if (backwards) {
                At<float>(bike, 0x69C) = reverse;
                At<float>(bike, 0x6A0) = 0.0f;
                if (along > 0.1f && At<float>(self, 0x28) < At<float>(At<uint8_t*>(ped, 0x2E0), 0x1C)) {
                    input = 0.0f;
                    brake = 1.0f;
                } else {
                    input = -pushSq;
                }
            }
        }
    } else {
        StartPedalling(bike, input);
    }
    float speed = At<float>(bike, 0x69C);
    if (speed < reverse) {
        At<float>(bike, 0x69C) = reverse;
    } else if (top < speed) {
        At<float>(bike, 0x69C) = top;
    }
    At<float>(bike, 0x29C) = 1.0f;
    At<float>(bike, 0x294) = input;
    At<float>(bike, 0x298) = brake;
    if (0.001f > At<float>(self, 0x2C)) {
        At<float>(bike, 0x298) = 1.0f;
    }
    return 1;
}

// 0x6595E0 (thiscall): SteerTrack's steering: the stick (the player's with the pad's
// own shaping) through a 1.9 power curve into the bike's steer; an AI braking into sharp
// turns, the brake on when standing; the owner's lean (+0x38 of the owner) eased towards
// the bike's lean past 0.1 of its lock
uint8_t __fastcall Steer_Apply(uint8_t* self, re::Edx) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x24));
    uint8_t* bike = At<uint8_t*>(ped, 0x1554);
    if (!bike || At<int32_t>(bike, 0x46C) != 1) {
        return 1;
    }
    V3 direction;
    Vehicle_GetDriverMoveDirection(bike, &direction);
    float side = direction.x;
    float steer;
    float push = At<float>(At<uint8_t*>(ped, 0x2E0), 0x1C) * direction.x;
    if (-0.03f < push && push < 0.03f) {
        side = 0.0f;
        steer = 0.0f;
    } else {
        if (UserInput_IsEnabled(userInput)) {
            float x;
            if (altSteering && !Pad_738500(Pad_GetActive())) {
                float back = -Pad_GetValue(0x12, 0);
                x = back - Pad_GetValue(0x10, 0);
            } else {
                x = -Pad_GetValue(0x10, 0);
            }
            int32_t sign = x < 0.0f ? -1 : 1;
            float dot = std::fabs(x * yAxis[0] + direction.y * yAxis[1] + direction.z * yAxis[2]);
            float pad = Pad_GetValue(0x11, 0);
            float w = std::fabs(pad * (1.0f - dot) * 0.6f);
            x = float(sign) * w + x;
            side = x;
        }
        int32_t sign = side < 0.0f ? -1 : 1;
        float curved = trk::CrtPow(std::fabs(side), 1.9f) * float(sign);
        if (curved < -1.0f) {
            steer = -1.0f;
        } else if (1.0f < curved) {
            steer = 1.0f;
        } else {
            steer = curved;
        }
    }
    side = steer;
    At<float>(bike, 0x360) = -steer;
    At<float>(bike, 0x6B4) = steer;
    float turn = Bike_GetSteerLock(bike) * -steer;
    // (a double constant, not a float's: the product rounded once)
    float radians = float(double(turn) * 0.017453293428246197);
    vc::Call<void>(bike, 0xE0, radians);
    if (At<int32_t>(ped, 0x1310) != 0xD) {
        if (std::fabs(radians) > 0.1f && At<float>(bike, 0x294) == 0.0f) {
            At<float>(bike, 0x294) = At<float>(bike, 0x29C) * 0.5f;
        }
        if (CrtSqrt(std::fabs(direction.y * direction.y + side * side)) == 0.0f) {
            At<float>(bike, 0x298) = 1.0f;
        }
    }
    float lean = At<float>(bike, 0x2A0);
    float value = 0.0f;
    if (lean * At<float>(bike, 0x6A8) < 0.0f) {
        float lock = Bike_GetSteerLock(bike) * 3.14159274f / 180.0f;
        float r = std::fabs(lean / lock);
        if (!(0.1f > r)) {
            float q = (r - 0.1f) * 1.1f;
            q = q * q;
            q = q * q;
            value = q * q;
        }
    }
    float* target = At<float*>(At<uint8_t*>(self, 0x24), 0x38);
    float d = value - *target;
    float rate = 0.0f < d ? trk::CrtPow(0.4f, timeStep) : trk::CrtPow(0.2f, timeStep);
    d = rate * d;
    float next = d + *target;
    if (next < 0.01f) {
        *target = 0.01f;
    } else if (1.0f < next) {
        *target = 1.0f;
    } else {
        *target = next;
    }
    return 1;
}

} // namespace

#define BIKETRK2_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

BIKETRK2_REVERSED("PedalTrack::Update", 0x00659230, &Pedal_Update);
BIKETRK2_REVERSED("SteerTrack::Apply_6595E0", 0x006595E0, &Steer_Apply);
