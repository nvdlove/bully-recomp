// CAutomobile (vtable 0x905D04): bikes and karts righting themselves (0x4AD1D0) and
// floating (0x4AE000, GTA's ProcessBuoyancy). Fields by offset: +0x84 in water, +0x158
// the mass, +0x15C the turn mass, +0x168 the buoyancy, +0x1BC flags (0x10 sinking),
// +0x84E how many wheels are on the ground, +0x864 the ground's normal.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Vehicle.h"
#include "reversed.h"

namespace {

using re::At;

struct Vec {
    float x, y, z;
};

GAME_VAR(float, timeStep, 0x00C1A9A4);
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);
GAME_VAR(Vec, g_9E33DC, 0x009E33DC);
inline void* const g_actionNodeC4 = reinterpret_cast<void*>(0x01B652C4);
inline void* const TheBuoyancy = reinterpret_cast<void*>(0x00C2DB58);

GAME_FN(int32_t*, __thiscall, Vehicle_GetVehicleClass, 0x004CAF30, void* vehicle, int32_t* out);
GAME_FN(void, __thiscall, Vehicle_SetCreatedBy, 0x004CB230, void* vehicle, int createdBy);
GAME_FN(float*, __thiscall, Entity_GetMatrix, 0x008B6120, void* entity);
// (0x004146B0 stores 16 bytes through `out`: called through re::CallVec16)
static inline Vec* Multiply3x3(Vec* out, const float* m, const Vec* v) { return re::CallVec16<0x004146B0, Vec*>(out, m, v); }
GAME_FN(Vec*, __cdecl, Multiply3x3Transposed, 0x00414770, Vec* out, const Vec* v, const float* m);
GAME_FN(Vec*, __cdecl, VectorAdd, 0x004129A0, Vec* out, const Vec* a, const Vec* b);
GAME_FN(void, __thiscall, Physical_ApplyMoveForce, 0x0046A7E0, void* entity, float x, float y, float z);
GAME_FN(void, __thiscall, Physical_ApplyTurnForce, 0x0046A950, void* entity, float fx, float fy, float fz, float px, float py,
        float pz);
GAME_FN(void*, __thiscall, Seats_GetOccupant, 0x004CE560, void* seats, uint32_t seat);
GAME_FN(void, __thiscall, Seats_RemoveDriver, 0x004CE960, void* seats);
GAME_FN(uint8_t, __thiscall, ActionController_IsPlaying, 0x005F4800, void* controller, void* node, int, int);
GAME_FN(void, __thiscall, Entity_SetPosition, 0x0041C4E0, void* entity, float x, float y, float z);
GAME_FN(uint8_t, __thiscall, Buoyancy_Process, 0x004C87C0, void* buoyancy, void* entity, float buoyancyConstant, Vec* point,
        Vec* impulse);
GAME_FN(void, __thiscall, SetRef, 0x005D7F10, void* ref, void* entity);

float GravityFactor(CAutomobile* self) {
    using Fn = float(__thiscall*)(void*);
    return re::PtrCast<Fn>(self->vtable[0x8C / 4])(self);
}

// the ground's normal against one of the car's axes: the dot clamped to [-1, 1], its
// sign
float AxisDot(CAutomobile* self, const float* axis, float* sign) {
    uint8_t* p = reinterpret_cast<uint8_t*>(self);
    float d = float((double(At<float>(p, 0x868)) * axis[1] + double(axis[0]) * At<float>(p, 0x864)) +
                    double(At<float>(p, 0x86C)) * axis[2]);
    if (d > 1.0f) {
        d = 1.0f;
        *sign = 1.0f;
    } else if (-1.0f > d) {
        d = -1.0f;
        *sign = -1.0f;
    } else if (!(d < 0.0f)) {
        *sign = 1.0f;
    } else {
        *sign = -1.0f;
    }
    return d;
}

// the pull of the wheels on the ground (all four: -0.05, else -0.015 a wheel more)
float WheelPull(CAutomobile* self) {
    uint8_t wheels = At<uint8_t>(self, 0x84E);
    if (wheels == 4) {
        return -0.05f;
    }
    return float(double(int32_t(wheels) + 1) * double(-0.015f));
}

// a righting turn force about one axis, applied at the centre of mass plus the axis
void ApplyRighting(CAutomobile* self, float d, float sign, float damped, const float* axis, const Vec* centre) {
    float pull = float(double(WheelPull(self)) - damped);
    float limit = -damped;
    float q = pull < limit || pull == limit ? pull : limit;
    float step = timeStep;
    Vec along = {axis[0], axis[1], axis[2]};
    float* up = Entity_GetMatrix(self) + 8;
    float half = float(double(q) * 0.5);
    float k = float(double(half) * sign * d * d * At<float>(self, 0x15C));
    Vec f = {float(double(up[0]) * k), float(double(up[1]) * k), float(double(up[2]) * k)};
    f = {float(double(f.x) * step), float(double(f.y) * step), float(double(f.z) * step)};
    f = {float(double(f.x) * 0.5), float(double(f.y) * 0.5), float(double(f.z) * 0.5)};
    Vec at;
    Vec* point = VectorAdd(&at, centre, &along);
    Physical_ApplyTurnForce(self, f.x, f.y, f.z, point->x, point->y, point->z);
}

} // namespace

namespace c09autophysics {

// 0x4AD1D0: bikes, karts and class 6 kept upright: about the right and forward axes,
// turned towards the ground's normal against their spin (damped to 0.65 / 0.5 of it),
// harder the more wheels are down
void __fastcall KeepUpright(CAutomobile* self, re::Edx) {
    int32_t vehicleClass;
    if (*Vehicle_GetVehicleClass(self, &vehicleClass) != 7 && *Vehicle_GetVehicleClass(self, &vehicleClass) != 10 &&
        *Vehicle_GetVehicleClass(self, &vehicleClass) != 6) {
        return;
    }
    Vec com = {self->centreOfMass.x, self->centreOfMass.y, self->centreOfMass.z};
    struct { Vec v; float pad; } centrePad16;   // (the game's transform writes a 4th float)
    Vec& centre = centrePad16.v;
    Multiply3x3(&centre, Entity_GetMatrix(self), &com);
    Vec spin;
    Multiply3x3Transposed(&spin, reinterpret_cast<Vec*>(&self->turnSpeed), Entity_GetMatrix(self));
    // about the right axis: against the spin's y
    float* m = Entity_GetMatrix(self);
    float right[3] = {m[0], m[1], m[2]};
    float sign;
    float d = AxisDot(self, right, &sign);
    float damped;
    float y = float(double(spin.y) * double(0.65f));
    if (sign > 0.0f) {
        damped = -(y < 0.0f ? 0.0f : y);
    } else {
        damped = y < 0.0f || y == 0.0f ? y : 0.0f;
    }
    ApplyRighting(self, d, sign, damped, right, &centre);
    // about the forward axis: against the spin's x
    m = Entity_GetMatrix(self);
    float forward[3] = {m[4], m[5], m[6]};
    d = AxisDot(self, forward, &sign);
    float x = float(-double(spin.x) * 0.5);
    if (sign > 0.0f) {
        damped = -(x < 0.0f ? 0.0f : x);
    } else {
        damped = x < 0.0f || x == 0.0f ? x : 0.0f;
    }
    ApplyRighting(self, d, sign, damped, forward, &centre);
}

// 0x4AE000: ProcessBuoyancy: a driver who's getting out (+0x16CC, action 0x1B652C4)
// lifted off and out; in water: pushed up (half for a driven car), the speeds damped by
// how deep it is, sinking (flag 0x10, no faster than 0.1) when deep or deep enough with
// a wheel fully out; an empty sinking car no longer the player's, a random one, its
// owner forgotten
void __fastcall ProcessBuoyancy(CAutomobile* self, re::Edx) {
    uint8_t* p = reinterpret_cast<uint8_t*>(self);
    void* seats = &self->seats;
    uint8_t* driver = static_cast<uint8_t*>(Seats_GetOccupant(seats, 0));
    if (driver && At<uint8_t>(driver, 0x16CC) &&
        ActionController_IsPlaying(At<uint8_t*>(driver, 0x2E0) + 0x258, g_actionNodeC4, 1, 1)) {
        uint8_t* matrix = At<uint8_t*>(driver, 0x14);
        const Vec* pos = matrix ? reinterpret_cast<Vec*>(matrix + 0x30) : reinterpret_cast<Vec*>(driver + 4);
        Vec to;
        Vec* at = VectorAdd(&to, pos, &g_9E33DC);
        Entity_SetPosition(driver, at->x, at->y, at->z);
        Seats_RemoveDriver(seats);
    }
    Vec point = {0.0f, 0.0f, 0.0f};
    Vec impulse = {0.0f, 0.0f, 0.0f};
    float buoyancy = float(double(GravityFactor(self)) * At<float>(p, 0x158) * 100.0 /
                           double(int32_t(At<uint8_t>(self->handling, 0x28))));
    At<float>(p, 0x168) = buoyancy;
    if (!Buoyancy_Process(TheBuoyancy, self, buoyancy, &point, &impulse)) {
        At<uint8_t>(p, 0x1BC) &= 0xEF;
        At<int32_t>(p, 0x84) = 0;
        return;
    }
    if (driver) {
        impulse = {float(double(impulse.x) * 0.5), float(double(impulse.y) * 0.5), float(double(impulse.z) * 0.5)};
    }
    At<int32_t>(p, 0x84) = 1;
    float step = timeStep < 0.5f ? 0.5f : timeStep;
    double up = impulse.z;
    float depth = float(up / (double(GravityFactor(self)) * At<float>(p, 0x158) * step));
    float base = float(1.0 - double(depth) * double(0.05f));
    float damp = float(std::pow(double(base), double(timeStep)));
    veh::CVector& ms = self->moveSpeed;
    veh::CVector& ts = self->turnSpeed;
    ms.x = float(double(ms.x) * damp);
    ms.y = float(double(ms.y) * damp);
    ms.z = float(double(ms.z) * damp);
    ts.x = float(double(ts.x) * damp);
    ts.y = float(double(ts.y) * damp);
    ts.z = float(double(damp) * ts.z);
    At<int32_t>(p, 0x84) = 1;
    Physical_ApplyMoveForce(self, impulse.x, impulse.y, impulse.z);
    Physical_ApplyTurnForce(self, impulse.x, impulse.y, impulse.z, point.x, point.y, point.z);
    bool sinking = double(depth) > double(0.85f);
    if (!sinking && 0.7f < depth) {
        float* wheels = &At<float>(p, 0x764);
        sinking = wheels[0] == 1.0f || wheels[1] == 1.0f || wheels[2] == 1.0f || wheels[3] == 1.0f;
    }
    if (!sinking) {
        At<uint8_t>(p, 0x1BC) &= 0xEF;
        if (!(double(depth) > double(0.1f))) {
            return;
        }
        float left = float(1.0 - double(depth) * depth);
        float e = float(double(timeStep) * double(0.15f));
        float d1 = float(std::pow(double(left), double(e)));
        ms.x = float(double(ms.x) * d1);
        ms.y = float(double(d1) * ms.y);
        ms.z = float(double(d1) * ms.z);
        e = float(double(timeStep) * double(0.15f));
        float d2 = float(std::pow(double(left), double(e)));
        ts.x = float(double(ts.x) * d2);
        ts.y = float(double(ts.y) * d2);
        ts.z = float(double(d2) * ts.z);
        return;
    }
    At<uint8_t>(p, 0x1BC) |= 0x10;
    if (double(ms.z) < double(-0.1f)) {
        ms.z = -0.1f;
    }
    if (driver) {
        At<int32_t>(driver, 0x84) = 1;
        return;
    }
    if (At<CAutomobile*>(playerPed, 0x1560) == self) {
        At<void*>(playerPed, 0x1560) = nullptr;
    }
    Vehicle_SetCreatedBy(self, 1);
    SetRef(&self->owner, nullptr);
}

} // namespace c09autophysics

// ---- registration ---------------------------------------------------------------------

#define AUTOPHYSICS_REVERSED(name, address, fn)                                              \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

AUTOPHYSICS_REVERSED("CAutomobile::KeepUpright", 0x004AD1D0, &c09autophysics::KeepUpright);
AUTOPHYSICS_REVERSED("CAutomobile::ProcessBuoyancy", 0x004AE000, &c09autophysics::ProcessBuoyancy);
