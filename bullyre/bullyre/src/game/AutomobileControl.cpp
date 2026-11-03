// CAutomobile (vtable 0x905D04): the player's driving input (vf42: steering, throttle
// and brake, the go-kart's rockets), the steering eased to its target (vf56), the
// power slide (0x4A9000), the horn (vf55 and when stuck). Fields past CVehicle by
// offset: the wheels' +0x764 values (1: off the ground), +0x6EC / +0x73C two wheel
// points, +0x7D6 a word, +0x800 / +0x804 / +0x860 times, +0x84E a flag.

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
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);
GAME_VAR(void*, g_effects, 0x00B6F1D4);
GAME_VAR(uint16_t, g_rocketEffect, 0x020C8FD8);
GAME_VAR(void*, g_rocketBoneRoot, 0x01B6531C);
GAME_VAR(void*, g_rocketBone1, 0x01B65394);
GAME_VAR(void*, g_rocketBone2, 0x01B65440);
inline void* const g_rocketBoneNames = reinterpret_cast<void*>(0x01B65398);
inline void* const TheSounds = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const TheUserInput = reinterpret_cast<void*>(0x00A45A30);
// the models with their own horns
GAME_VAR(int32_t, modelBF430C, 0x00BF430C);
GAME_VAR(int32_t, modelBF42FC, 0x00BF42FC);
GAME_VAR(int32_t, modelBF4304, 0x00BF4304);
GAME_VAR(int32_t, modelBF4308, 0x00BF4308);
GAME_VAR(int32_t, modelBF42F8, 0x00BF42F8);
GAME_VAR(int32_t, modelBF4300, 0x00BF4300);
GAME_VAR(int32_t, modelBF4118, 0x00BF4118);
GAME_VAR(int32_t, modelBF4318, 0x00BF4318);
GAME_VAR(int32_t, modelBF431C, 0x00BF431C);

GAME_FN(Vec*, __thiscall, Entity_GetForward, 0x004154A0, void* entity, Vec* out);
GAME_FN(float*, __thiscall, Entity_GetMatrix, 0x008B6120, void* entity);
GAME_FN(int32_t*, __thiscall, Vehicle_GetVehicleClass, 0x004CAF30, void* vehicle, int32_t* out);
GAME_FN(float, __thiscall, Vehicle_GetSteerInput, 0x004CB350, void* vehicle, int);
GAME_FN(uint8_t, __thiscall, Vehicle_442120, 0x00442120, void* vehicle);
GAME_FN(void*, __thiscall, Seats_GetOccupant, 0x004CE560, void* seats, uint32_t seat);
GAME_FN(uint8_t, __thiscall, Ped_IsSeatedInVehicle, 0x004CE690, void* ped);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(uint8_t, __cdecl, Pad_JustPressed, 0x00738900, int control, int pad);
GAME_FN(uint8_t, __thiscall, UserInput_IsEnabled, 0x0050DF50, void* input);
GAME_FN(Vec*, __cdecl, Normalised, 0x00472D40, Vec* out, const Vec* v);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const Vec* a, const Vec* b);
GAME_FN(float, __cdecl, Max, 0x00414490, float a, float b);
GAME_FN(float, __cdecl, Min, 0x00414470, float a, float b);
GAME_FN(float, __thiscall, Vec_Magnitude, 0x0040D9F0, const Vec* v);
GAME_FN(Vec*, __cdecl, VectorScale, 0x00412350, Vec* out, const Vec* v, float s);
GAME_FN(Vec*, __cdecl, ScaleVector, 0x00412320, Vec* out, float s, const Vec* v);
GAME_FN(Vec*, __cdecl, VectorAdd, 0x004129A0, Vec* out, const Vec* a, const Vec* b);
GAME_FN(Vec*, __cdecl, VectorSub, 0x00414B70, Vec* out, const Vec* a, const Vec* b);
GAME_FN(void, __thiscall, Physical_ApplyMoveForce, 0x0045D3E0, void* entity, const Vec* force);
GAME_FN(void, __thiscall, Physical_ApplyTurnForce, 0x0045D410, void* entity, const Vec* point, const Vec* force);
GAME_FN(int, __cdecl, GetWeaponIndex, 0x0051AEA0, const char* name);
GAME_FN(void*, __cdecl, FindBoneInfo, 0x0072AEA0, void* path, void* names);
GAME_FN(void, __cdecl, Fn70CB60, 0x0070CB60, const Vec* pos, int, void* outA, void* outB);
GAME_FN(int, __thiscall, Sounds_GetId, 0x00595B80, void* sounds, const char* name);
GAME_FN(void, __thiscall, Sounds_Play, 0x0059BAB0, void* sounds, int id, void* a, void* b, int);
GAME_FN(void*, __thiscall, Sounds_PlayAt, 0x0059B8A0, void* sounds, int id, const Vec* pos, int, const Vec* pos2, int, int, int,
        int, uintptr_t, uintptr_t, int, int, float, int);
GAME_FN(void, __thiscall, Effect_Construct, 0x00668E90, void* effect);
GAME_FN(void, __thiscall, Effect_Destruct, 0x00668EB0, void* effect);
GAME_FN(void, __thiscall, Effects_Start, 0x00668290, void* manager, uint32_t* type, void* effect, int);
GAME_FN(uint8_t*, __cdecl, Projectile_Fire_x64, 0x004D4D90, void* shooter, int weapon, re::Float3, re::Float3, void* bone, float, float, void* effect, uintptr_t, int, re::Float3, re::Float3, uintptr_t, int);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t* Projectile_Fire(void* a0, int a1, float a2, float a3, float a4, float a5, float a6, float a7, void* a8, float a9, float a10, void* a11, uintptr_t a12, int a13, float a14, float a15, float a16, float a17, float a18, float a19, int a20, int a21) {
    return Projectile_Fire_x64(a0, a1, re::Float3{a2, a3, a4}, re::Float3{a5, a6, a7}, a8, a9, a10, a11, a12, a13, re::Float3{a14, a15, a16}, re::Float3{a17, a18, a19}, a20, a21);
}
GAME_FN(void, __stdcall, DestructArray, 0x004017C0, void* array, size_t size, int count, void* destructor);

const Vec* EntityPos(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<Vec*>(matrix + 0x30) : reinterpret_cast<Vec*>(static_cast<uint8_t*>(entity) + 4);
}
Vec& MoveSpeed(CAutomobile* self) {
    return *reinterpret_cast<Vec*>(&self->moveSpeed);
}

} // namespace

namespace c09autocontrol {

// 0x4A9000: the power slide (the player's button 6 with +0x84E): while +0x470 runs
// down, the car pushed back against its motion and forward, and turned by its steering,
// by how many wheels are off the ground; started again (1) when the button's still
// held and it's slow (or not the player)
void __fastcall ProcessPowerSlide(CAutomobile* self, re::Edx) {
    void* seats = &self->seats;
    void* driver = Seats_GetOccupant(seats, 0);
    if (!driver || !Ped_IsSeatedInVehicle(Seats_GetOccupant(seats, 0))) {
        self->unknown474[0] = 0;
        self->unknown470 = 0.0f;
        return;
    }
    int32_t vehicleClass;
    if (*Vehicle_GetVehicleClass(self, &vehicleClass) == 6) {
        self->unknown474[0] = 0;
        self->unknown470 = 0.0f;
        return;
    }
    Vec& ms = MoveSpeed(self);
    float speedSq = float((double(ms.y) * ms.y + double(ms.x) * ms.x) + double(ms.z) * ms.z);
    float grip = float(double(1.2f) - double(speedSq) / double(0.15f));
    if (double(grip) < double(0.65f)) {
        grip = 0.65f;
    }
    if (grip > 1.0f) {
        grip = 1.0f;
    }
    uint8_t wasOn = self->unknown474[0];
    self->unknown470 = float(double(self->unknown470) - double(timeStep) * double(0.025f) * grip);
    if (At<int32_t>(Seats_GetOccupant(seats, 0), 0x1310) == 0xD) {
        float button = Pad_GetValue(6, 0);
        self->unknown474[0] = (0.0f < button && At<uint8_t>(self, 0x84E) > 0) ? 1 : 0;
    }
    if (!(0.0f < self->unknown470)) {
        self->unknown470 = 0.0f;
        if (!self->unknown474[0]) {
            return;
        }
        if (!wasOn) {
            float flatSq = float(double(ms.x) * ms.x + double(ms.y) * ms.y);
            if (flatSq > 0.0025000002f) {
                self->unknown470 = 1.0f;
                self->unknown478 = int32_t(timeInMs);
                return;
            }
        }
        if (At<int32_t>(Seats_GetOccupant(seats, 0), 0x1310) == 0xD) {
            return;
        }
        self->unknown470 = 1.0f;
        self->unknown478 = int32_t(timeInMs);
        return;
    }
    self->unknown478 = int32_t(timeInMs);
    if (!self->unknown474[0]) {
        self->unknown470 = 0.0f;
        return;
    }
    uint8_t* p = reinterpret_cast<uint8_t*>(self);
    float* wheels = &At<float>(p, 0x764);
    if (!(1.0f > wheels[0] || 1.0f > wheels[1] || 1.0f > wheels[2] || 1.0f > wheels[3])) {
        return;
    }
    float offGround = 0.0f;
    for (int i = 0; i < 4; i++) {
        if (1.0f > wheels[i]) {
            offGround = float(double(offGround) + 0.25);
        }
    }
    float mass = At<float>(p, 0x158);
    float step = timeStep;
    Vec forward, dir;
    Vec* f = Entity_GetForward(self, &forward);
    float along = DotProduct(Normalised(&dir, &ms), f);
    float sideways = Max(0.0f, float(double(1.15f) - along));
    float pace = Min(1.0f, Max(0.5f, float(double(Vec_Magnitude(&ms)) / double(0.2f))));
    float force = float(double(offGround) * double(0.165f) * mass * pace * step * 0.5 * sideways);
    if (At<int32_t>(Seats_GetOccupant(seats, 0), 0x1310) == 0xD && Pad_GetValue(7, 0) == 0.0f) {
        force = float(double(force) * 0.5);
    }
    Vec n;
    Normalised(&n, &ms);
    Vec back = {-n.x, -n.y, -n.z};
    Vec push = {float(double(back.x) * force), float(double(back.y) * force), float(double(force) * back.z)};
    float left = float(1.0 - self->unknown470);
    push = {float(double(push.x) * left), float(double(push.y) * left), float(double(left) * push.z)};
    Physical_ApplyMoveForce(self, &push);
    float drive = float(1.0 - double(self->unknown470) * double(0.67f));
    Vec a, b;
    Physical_ApplyMoveForce(self, VectorScale(&b, VectorScale(&a, Entity_GetForward(self, &forward), force), drive));
    float step2 = timeStep;
    float* matrix = Entity_GetMatrix(self);
    float steer = self->steerAngle;
    Vec fwd2, n2;
    float along2 = DotProduct(Normalised(&n2, &ms), Entity_GetForward(self, &fwd2));
    float turnPace = Min(1.2f, float(double(Vec_Magnitude(&ms)) / double(0.05f) + 0.5));
    float q = float(-double(steer) * self->unknown470 * At<float>(p, 0x15C) * double(0.017f) * offGround);
    Vec t1, t2, t3, t4, t5;
    ScaleVector(&t1, q, reinterpret_cast<Vec*>(matrix));
    VectorScale(&t2, &t1, turnPace);
    VectorScale(&t3, &t2, step2);
    VectorScale(&t4, &t3, 0.5f);
    Vec* torque = VectorScale(&t5, &t4, along2);
    const Vec* pos = EntityPos(self);
    Vec sum, mid, rel;
    VectorAdd(&sum, &At<Vec>(p, 0x6EC), &At<Vec>(p, 0x73C));
    VectorScale(&mid, &sum, 0.5f);
    VectorSub(&rel, &mid, pos);
    Physical_ApplyTurnForce(self, &rel, torque);
}

// 0x4A95F0 (vf42): the player's input: handbrake (flags33A 4) from control 12 (not for
// class 10), the steering (+0x360) eased to the stick, throttle (+0x294) and brake
// (+0x298) from controls 7 and 6 (reversing by the way it moves; both held when
// stopped: a burnout, +0x7D6), the steering angle set (vf56). Input off: braking, the
// speed capped at 0.28. Class 10 fires rockets on control 12
void __fastcall ProcessControlInputs(CAutomobile* self, re::Edx) {
    uint8_t* p = reinterpret_cast<uint8_t*>(self);
    Vec& ms = MoveSpeed(self);
    Vec forward;
    Vec* f = Entity_GetForward(self, &forward);
    float along = float(double(ms.y) * f->y + double(ms.x) * f->x + double(ms.z) * f->z);
    int32_t vehicleClass;
    if (*Vehicle_GetVehicleClass(self, &vehicleClass) == 10) {
        self->flags33A &= 0xFB;
    } else {
        uint8_t pressed = Pad_GetValue(0xC, 0) != 0.0f ? 1 : 0;
        self->flags33A ^= (uint8_t(pressed * 4) ^ self->flags33A) & 4;
    }
    float step = timeStep;
    float input = Vehicle_GetSteerInput(self, 1);
    float steer = float((-double(input) - self->unknown360) * double(0.2f) * step + self->unknown360);
    self->unknown360 = steer;
    if (steer <= -1.0f) {
        steer = -1.0f;
    } else if (!(steer < 1.0f)) {
        steer = 1.0f;
    }
    self->unknown360 = steer;
    float& gas = At<float>(p, 0x294);
    float& brake = At<float>(p, 0x298);
    float power = At<float>(p, 0x29C);
    if (At<uint32_t>(p, 0x800) + 0x32 > timeInMs) {
        gas = float(double(power) * 0.0);
    } else {
        double accelerate = Pad_GetValue(7, 0);
        float pedal = float(accelerate - Pad_GetValue(6, 0));
        float speed = std::fabs(along);
        if (speed < 0.01f) {
            if (Pad_GetValue(7, 0) > 0.5882353f && Pad_GetValue(6, 0) > 0.5882353f) {
                float throttle = Pad_GetValue(7, 0);
                gas = float(double(power) * throttle);
                brake = Pad_GetValue(6, 0);
                At<uint16_t>(p, 0x7D6) = 1;
            } else {
                gas = float(double(power) * pedal);
                brake = 0.0f;
            }
        } else if (!(0.0f < along || 0.0f == along)) {
            // moving backwards: the pedal forward brakes
            if (!(pedal < 0.0f)) {
                gas = float(double(power) * 0.0);
                brake = pedal;
            } else {
                gas = float(double(pedal) * power);
                brake = 0.0f;
            }
        } else if (pedal < 0.0f) {
            gas = float(double(power) * 0.0);
            brake = -pedal;
        } else {
            gas = float(double(pedal) * power);
            brake = 0.0f;
        }
    }
    float s = self->unknown360;
    float curve = float(double(s) * s);
    if (!(0.0f < s || 0.0f == s)) {
        curve = -curve;
    }
    float angle = float(double(float(double(At<float>(self->handling, 0x9C)) * double(3.1415927f) / 180.0 * curve)));
    using SetSteer = void(__thiscall*)(void*, float);
    re::PtrCast<SetSteer>(self->vtable[0xE0 / 4])(self, angle);
    if (!UserInput_IsEnabled(TheUserInput)) {
        self->flags33A |= 4;
        brake = 1.0f;
        gas = float(double(power) * 0.0);
        float speed = float(std::sqrt(std::fabs(float((double(ms.y) * ms.y + double(ms.x) * ms.x) + double(ms.z) * ms.z))));
        if (speed > 0.28f) {
            float scale = float(double(0.28f) / speed);
            ms.x = float(double(ms.x) * scale);
            ms.y = float(double(ms.y) * scale);
            ms.z = float(double(ms.z) * scale);
        }
        return;
    }
    if (*Vehicle_GetVehicleClass(self, &vehicleClass) != 10 || !(At<uint32_t>(p, 0x804) + 500 < timeInMs) ||
        !Pad_JustPressed(0xC, 0)) {
        return;
    }
    // a rocket from the kart's launcher bone
    int weapon = GetWeaponIndex("brocket");
    re::Ptr32<void> names[3] = {g_rocketBoneRoot, g_rocketBone1, g_rocketBone2};
    struct {
        int32_t count;
        re::Ptr32<re::Ptr32<void>> names;   // (ButesPath: the game's layout)
    } path = {3, names};
    void* bone = FindBoneInfo(&path, g_rocketBoneNames);
    uint32_t outA = 0, outB = 0;
    Fn70CB60(EntityPos(self), 2, &outB, &outA);
    Sounds_Play(TheSounds, Sounds_GetId(TheSounds, "MissleFire"), reinterpret_cast<void*>(uintptr_t(outB)),
                reinterpret_cast<void*>(uintptr_t(outA)), 0);
    uint32_t effectType = g_rocketEffect;   // (a word, the upper half left as the stack had it)
    uint8_t effect[8];
    Effect_Construct(effect);
    Effects_Start(g_effects, &effectType, effect, 0);
    Vec zeroA = {0.0f, 0.0f, 0.0f};
    Vec zeroB = {0.0f, 0.0f, 0.0f};
    Vec lift = {0.0f, 0.0f, 0.11f};
    Vec fwd;
    Vec* fw = Entity_GetForward(self, &fwd);
    Vec ahead = {float(double(fw->x) * 2.0), float(double(fw->y) * 2.0), float(double(fw->z) * 2.0)};
    const Vec* pos = EntityPos(self);
    void* shooter = Seats_GetOccupant(&self->seats, 0);
    Vec fwd2, aim, from;
    Vec* dir = VectorAdd(&aim, Entity_GetForward(self, &fwd2), &lift);
    Vec* start = VectorAdd(&from, pos, &ahead);
    uint8_t* rocket = Projectile_Fire(shooter, weapon, start->x, start->y, start->z, dir->x, dir->y, dir->z, bone, 0.0f, 0.0f,
                                      effect, 0, 0, zeroB.x, zeroB.y, zeroB.z, zeroA.x, zeroA.y, zeroA.z, 0, 0);
    Vec fwd3;
    Vec* f3 = Entity_GetForward(self, &fwd3);
    float forwardSpeed = float(double(ms.y) * f3->y + double(f3->x) * ms.x + double(ms.z) * f3->z);
    if (forwardSpeed > 0.0f) {
        At<float>(rocket, 0x124) = At<float>(rocket, 0x124) + ms.x;
        At<float>(rocket, 0x128) = ms.y + At<float>(rocket, 0x128);
    }
    At<uint32_t>(p, 0x804) = timeInMs;
    Effect_Destruct(effect);
    DestructArray(names, 4, 3, reinterpret_cast<void*>(0x0049A650));
}

// 0x4A9CA0 (vf56): the steering angle eased towards the target (at the time step a
// frame, half that when it turns back or less)
void __fastcall SetSteerAngle(CAutomobile* self, re::Edx, float target) {
    float rate;
    if (double(target) * self->steerAngle < 0.0 || !(std::fabs(self->steerAngle) <= std::fabs(target))) {
        rate = float(double(timeStep) * 0.5);
    } else {
        rate = timeStep;
    }
    float diff = std::fabs(float(double(target) - self->steerAngle));
    if (!(rate < diff)) {
        self->steerAngle = target;
        return;
    }
    if (!(self->steerAngle < target)) {
        self->steerAngle = float(double(self->steerAngle) - rate);
        return;
    }
    self->steerAngle = float(double(rate) + self->steerAngle);
}

// 0x4A9D60 (vf55): the horn (the model's own), unless the siren's on or it's already
// sounding
void __fastcall SoundHorn(CAutomobile* self, re::Edx) {
    if (self->siren || self->unknown368 != 0) {
        return;
    }
    if (self->unknown36E) {
        self->unknown36E--;
        return;
    }
    int model = self->modelIndex;
    self->unknown36E = 1;
    self->unknown368 = 0;
    int sound = 0;
    const char* name = nullptr;
    if (model == modelBF430C || model == modelBF42FC || model == modelBF4304) {
        name = "C_Horn";
    } else if (model == modelBF4308 || model == modelBF42F8) {
        name = "P_Horn";
    } else if (model == modelBF4300 || model == modelBF4118) {
        name = "F_Horn";
    } else if (model == modelBF4318 || model == modelBF431C) {
        name = "PoliceHorn";
    }
    if (name) {
        sound = Sounds_GetId(TheSounds, name);
    }
    const Vec* pos = EntityPos(self);
    void* playing = Sounds_PlayAt(TheSounds, sound, pos, 3, pos, 0, 0, 0, 0, 0, 0, 6, 1, 1.0f, 0);
    if (playing) {
        using Fn = void(__thiscall*)(void*, int);
        re::PtrCast<Fn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(playing))[0x80 / 4])(playing, 0);
    }
}

// 0x4A9E70: an AI car stuck (its autopilot's flags 3, speed under 0.1) honks, every 3 s
void __fastcall HonkWhenStuck(CAutomobile* self, re::Edx) {
    if (!(self->autoPilotFlags & 3) || Vehicle_442120(self)) {
        return;
    }
    Vec& ms = MoveSpeed(self);
    float speed = float(std::sqrt(std::fabs(float(double(ms.x) * ms.x + double(ms.y) * ms.y))));
    if (!(double(speed) < double(0.1f))) {
        return;
    }
    uint8_t* p = reinterpret_cast<uint8_t*>(self);
    uint32_t now = timeInMs;
    uint32_t last = At<uint32_t>(p, 0x860);
    if (now > last + 3000 || last == 0) {
        using Fn = void(__thiscall*)(void*);
        re::PtrCast<Fn>(self->vtable[0xDC / 4])(self);
        At<uint32_t>(p, 0x860) = now;
    }
}

} // namespace c09autocontrol

// ---- registration ---------------------------------------------------------------------

#define AUTOCONTROL_REVERSED(name, address, fn)                                              \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

AUTOCONTROL_REVERSED("CAutomobile::ProcessPowerSlide", 0x004A9000, &c09autocontrol::ProcessPowerSlide);
AUTOCONTROL_REVERSED("CAutomobile::ProcessControlInputs", 0x004A95F0, &c09autocontrol::ProcessControlInputs);
AUTOCONTROL_REVERSED("CAutomobile::SetSteerAngle", 0x004A9CA0, &c09autocontrol::SetSteerAngle);
AUTOCONTROL_REVERSED("CAutomobile::SoundHorn", 0x004A9D60, &c09autocontrol::SoundHorn);
AUTOCONTROL_REVERSED("CAutomobile::HonkWhenStuck", 0x004A9E70, &c09autocontrol::HonkWhenStuck);
