// CAutomobile::ProcessControl (vf12, 0x4AE430): a frame of a car's physics: GTA Vice
// City's CAutomobile::ProcessControl nearly line for line (the centre of mass eased to
// the handling's, the driving status's work, the springs and their dampening, the
// transmission, the four wheels' ProcessWheel, the wheels off the ground spinning,
// the suspension positions), with Bully's own: a probe ahead for mission cars, sand
// slowing the car sideways, the power slide, the wheelie, the go-kart's work, a map
// edge that turns cars back, and the wheels' landing sounds.
//
// Fields past CVehicle by offset (the names are GTA's): the damage manager at +0x580
// (+0x0 the burst wheels' grip factor), the doors at +0x59C, the wheel col points at
// +0x6C4 (0x28 bytes: +0x0 the point, +0x10 the normal, +0x20 / +0x22 the surfaces),
// the suspension ratios at +0x764 (and last frame's at +0x774), the wheel timers at
// +0x784, their skid kinds at +0x794 / +0x798, rotations at +0x79C, positions at
// +0x7AC, speeds at +0x7BC, +0x7CC flags, +0x7D4 a flag, +0x7D6 the burnout, the
// spring lengths at +0x7D8 and line lengths at +0x7E8, +0x7FC the rear grip, the
// ground offsets at +0x80C, the grounds at +0x83C, +0x84E / +0x84F the wheels / drive
// wheels on the ground, the wheel states at +0x850, +0x864 the average ground normal.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Vehicle.h"

#include "game/Crt.h"
#include "reversed.h"
namespace {

using re::At;

struct Vec {
    float x, y, z;
};

GAME_VAR(float, timeStep, 0x00C1A9A4);
GAME_VAR(uint32_t, frameCounter, 0x00C1A99C);
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);
GAME_VAR(uint8_t, g_C1AE6F, 0x00C1AE6F);
GAME_VAR(float, g_sandGrip, 0x00CE7564);
GAME_VAR(float, g_tractionScale, 0x00C2D16C);
GAME_VAR(Vec, g_defaultNormal, 0x009E33DC);
GAME_VAR(Vec, vecUnset, 0x00BCC564);
GAME_VAR(float, g_halfPi, 0x00A356DC);
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(uint8_t, g_chapter, 0x00A147E0);
GAME_ARRAY(int32_t, g_chapterSeason, 0x00A147EC);  // (2: winter)
GAME_ARRAY(int32_t, g_wheelState, 0x00C2D174);     // (GTA's static WheelState[4])
GAME_ARRAY(uint8_t*, modelInfoPtrs, 0x00C67738);   // CModelInfo::ms_modelInfoPtrs
inline uint8_t* const g_handlingData = reinterpret_cast<uint8_t*>(0x00C2DF60);   // +0 the drive type, 0xDC a car
inline void* const g_specialColModel = reinterpret_cast<void*>(0x00C1AEAC);

GAME_FN(void, __thiscall, Vehicle_UpdateFade, 0x004CC270, void* vehicle);
GAME_FN(void, __thiscall, Vehicle_4CC350, 0x004CC350, void* vehicle);
GAME_FN(void*, __thiscall, Seats_GetOccupant, 0x004CE560, void* seats, uint32_t seat);
GAME_FN(int32_t*, __thiscall, Vehicle_GetVehicleClass, 0x004CAF30, void* vehicle, int32_t* out);
GAME_FN(uint8_t, __thiscall, Vehicle_HasSirenModel, 0x004CB1F0, void* vehicle);
GAME_FN(float, __thiscall, Vehicle_GetUnknown470, 0x004CB510, void* vehicle);
GAME_FN(float, __thiscall, Vehicle_CollisionImpulse, 0x004CB870, void* vehicle, int, const Vec* forward, const Vec* speed,
        float);
GAME_FN(void, __thiscall, Vehicle_ProcessWheel, 0x004CB8D0, void* vehicle, Vec* wheelFwd, Vec* wheelRight, Vec* contactSpeed,
        Vec* contactPoint, int wheelsOnGround, float thrust, float brake, float adhesion, int wheel, float* wheelSpeed,
        int32_t* wheelState, int status);
GAME_FN(void, __thiscall, Automobile_4AB100, 0x004AB100, void* automobile, float, int);
GAME_FN(void, __thiscall, Automobile_4AD740, 0x004AD740, void* automobile);
GAME_FN(void, __thiscall, Automobile_4AD1D0, 0x004AD1D0, void* automobile);
GAME_FN(void, __thiscall, Automobile_4AE000, 0x004AE000, void* automobile);
GAME_FN(void, __thiscall, Automobile_ProcessPowerSlide, 0x004A9000, void* automobile);
GAME_FN(void, __thiscall, Automobile_HonkWhenStuck, 0x004A9E70, void* automobile);
GAME_FN(void, __thiscall, Automobile_CountDown368, 0x004A8860, void* automobile);
GAME_FN(void, __thiscall, Automobile_450970, 0x00450970, void* automobile);
GAME_FN(void, __thiscall, Physical_ProcessControl, 0x00469B70, void* physical);
GAME_FN(void, __thiscall, Physical_468DF0, 0x00468DF0, void* physical);
GAME_FN(void, __thiscall, Physical_ApplyMoveForce, 0x0045D3E0, void* entity, const Vec* force);
GAME_FN(void, __thiscall, Physical_ApplyTurnForce, 0x0046A950, void* entity, float fx, float fy, float fz, float px, float py,
        float pz);
GAME_FN(void, __thiscall, Physical_ApplySpringCollision, 0x0046C260, void* entity, float force, const Vec* springDir,
        const Vec* point, float ratio, float bias, const Vec* normal);
GAME_FN(void, __thiscall, Physical_ApplySpringDampening, 0x0046C3C0, void* entity, float damping, const Vec* springDir,
        const Vec* point, const Vec* speed);
GAME_FN(Vec*, __thiscall, Placeable_GetPosition, 0x00415460, void* entity);
GAME_FN(float*, __thiscall, Entity_GetMatrix, 0x008B6120, void* entity);
GAME_FN(uint8_t, __thiscall, Entity_GetIsOnScreen, 0x00467460, void* entity);
GAME_FN(Vec*, __thiscall, Entity_GetForward, 0x004154A0, void* entity, Vec* out);
GAME_FN(void, __thiscall, Matrix_SetRotateZOnly, 0x00412C20, float* matrix, float angle);
GAME_FN(int, __thiscall, Damage_GetWheelStatus, 0x004C7DE0, void* damage, int wheel);
GAME_FN(uint8_t, __thiscall, Damage_GetDoorStatus, 0x004C7E00, void* damage, int door);
GAME_FN(uint8_t, __thiscall, Damage_66A8D0, 0x0066A8D0, void* damage);
GAME_FN(float, __thiscall, Transmission_CalculateDriveAcceleration, 0x004CAC20, void* transmission, float* gas, float* speed,
        void* vehicle, float, float, float);
GAME_FN(Vec*, __cdecl, VectorScale, 0x00412350, Vec* out, const Vec* v, float s);
GAME_FN(Vec*, __cdecl, ScaleVector, 0x00412320, Vec* out, float s, const Vec* v);
GAME_FN(Vec*, __cdecl, VectorAdd, 0x004129A0, Vec* out, const Vec* a, const Vec* b);
GAME_FN(Vec*, __cdecl, VectorSub, 0x00414B70, Vec* out, const Vec* a, const Vec* b);
GAME_FN(Vec*, __cdecl, CrossProduct, 0x00414730, Vec* out, const Vec* a, const Vec* b);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const Vec* a, const Vec* b);
GAME_FN(float, __cdecl, Max, 0x00414490, float a, float b);
GAME_FN(float, __thiscall, Vec_Normalise, 0x00414E10, Vec* v);
GAME_FN(float, __thiscall, Vec_Magnitude, 0x0040D9F0, const Vec* v);
GAME_FN(float, __cdecl, SqrtAbs, 0x0040DF90, float value);
GAME_FN(Vec*, __cdecl, Fn565DA0, 0x00565DA0, Vec* out, const Vec* v, float s);
GAME_FN(void, __thiscall, ColPoint_Construct, 0x00415FA0, void* colPoint);
GAME_FN(void, __thiscall, ColPoint_Destruct, 0x00415500, void* colPoint);
GAME_FN(uint8_t, __cdecl, World_ProcessLineOfSight, 0x00461EA0, const Vec* start, const Vec* end, void* colPoint, re::Ptr32<void>* entity,
        int flags, int, int);
GAME_FN(float, __cdecl, CarAI_GoToCoorsSlowly, 0x0041A3B0, void* vehicle, const Vec* target);
GAME_FN(void, __cdecl, CarAI_UpdateCarAI, 0x00418A50, void* vehicle);
GAME_FN(void, __cdecl, CarCtrl_SteerAICarWithPhysics, 0x00423F70, void* vehicle);
GAME_FN(void, __cdecl, Fn41EE50, 0x0041EE50, void* vehicle);
GAME_FN(int32_t, __cdecl, Surface_GetGroup, 0x0045B310, uint32_t surface);
GAME_FN(float, __cdecl, Surface_GetAdhesiveLimit, 0x0045B500, const void* colPoint);
GAME_FN(float, __cdecl, Surface_GetWetMultiplier, 0x0045B530, uint32_t surface);
GAME_FN(void*, __cdecl, FindPlayerVehicle, 0x00446970, uintptr_t player);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(uint8_t, __cdecl, IsAreaInterior, 0x00430320, int area);
inline int __cdecl Rand() { return crt::Rand(); }

// 0x589B00 takes a registered entity reference by value (a stack slot the caller
// registers and the callee lets go), then 0.0f, 0, 1: built here as the game does
__declspec(noinline) void Call589B00WithRef(void* entity) {
#if defined(_M_IX86)
    void* fn = reinterpret_cast<void*>(0x00589B00);
    void* setRef = reinterpret_cast<void*>(0x005D7F10);
    __asm {
        push 1
        push 0
        sub esp, 8
        mov ecx, esp
        mov dword ptr [esp + 4], 0
        push entity
        mov dword ptr [ecx], 0
        call setRef
        call fn
        add esp, 0x10
    }
#else
    // (the 64-bit build: the reference built in a local, passed by value as the game does)
    struct EntityRef64 {
        void* entity;
    } ref = {nullptr};
    re::Ours<void(__fastcall*)(void*, void*, void*)>(0x005D7F10)(&ref, nullptr, entity);
    re::Ours<void(__cdecl*)(EntityRef64, float, void*, int)>(0x00589B00)(ref, 0.0f, nullptr, 1);
#endif
}

// The per-frame values the wheel blocks share
struct Frame {
    uint8_t* self;
    float accel;          // the transmission's
    float traction;
    float brakeForce;
    float brakeFront, brakeRear;
    float tractionFront, tractionRear;
    float forwardSpeed;   // signed, along the forward
    Vec* speeds;          // contact speeds [4]
    Vec* points;          // contact points [4]
};

const Vec* EntityPos(uint8_t* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<Vec*>(matrix + 0x30) : reinterpret_cast<Vec*>(entity + 4);
}
// the forward (the matrix's, else from the heading)
Vec ForwardOf(uint8_t* self) {
    uint8_t* matrix = At<uint8_t*>(self, 0x14);
    if (matrix) {
        return {At<float>(matrix, 0x10), At<float>(matrix, 0x14), At<float>(matrix, 0x18)};
    }
    float c = float(std::cos(double(At<float>(self, 0x10))));
    float s = float(std::sin(double(At<float>(self, 0x10))));
    return {-s, c, 0.0f};
}
// the heading (GTA's GetHeading)
float HeadingOf(uint8_t* self) {
    float* matrix = At<float*>(self, 0x14);
    if (matrix) {
        return float(std::atan2(-double(matrix[4]), double(matrix[5])));
    }
    return At<float>(self, 0x10);
}
uint8_t DriveType(uint8_t* self) {
    uint8_t* handling = At<uint8_t*>(self, 0x2A4);
    return g_handlingData[handling[0] * 0xDC];
}
uint8_t* WheelRecord(uint8_t* self, int i) {
    return self + 0x6C4 + i * 0x28;
}
float& Ratio(uint8_t* self, int i) {
    return At<float>(self, 0x764 + i * 4);
}
float& Timer(uint8_t* self, int i) {
    return At<float>(self, 0x784 + i * 4);
}
// the matrix's rotation applied (GTA's Multiply3x3)
Vec Multiply3x3(const float* m, const Vec& v) {
    Vec out;
    out.x = float((double(v.y) * m[4] + double(v.x) * m[0]) + double(v.z) * m[8]);
    out.y = float((double(v.x) * m[1] + double(v.y) * m[5]) + double(v.z) * m[9]);
    out.z = float((double(v.x) * m[2] + double(v.y) * m[6]) + double(v.z) * m[10]);
    return out;
}
// the wheel's forward flattened on the ground normal (GTA: wheelFwd -= (wheelFwd.n)n)
void Flatten(Vec& wf, const Vec& n) {
    float dot = float((double(n.y) * wf.y + double(wf.x) * n.x) + double(n.z) * wf.z);
    float ax = float(double(dot) * n.x);
    float ay = float(double(n.y) * dot);
    float az = float(double(dot) * n.z);
    wf.x = float(double(wf.x) - ax);
    wf.y = float(double(wf.y) - ay);
    wf.z = float(double(wf.z) - az);
}
// a point's speed (the turn about it plus the move), less its ground's
void ContactSpeed(uint8_t* self, int i, Vec* speeds, const Vec* points) {
    Vec c = points[i];
    Vec cross, sum;
    CrossProduct(&cross, &At<Vec>(self, 0x130), &c);
    VectorAdd(&sum, &cross, &At<Vec>(self, 0x124));
    speeds[i] = sum;
    re::Ptr32<uint8_t>* ground = &At<uint8_t*>(self, 0x83C + i * 4);
    if (*ground) {
        uint8_t* e = *ground;
        Vec offset = At<Vec>(self, 0x80C + i * 12);
        Vec ecross, esum;
        CrossProduct(&ecross, &At<Vec>(e, 0x130), &offset);
        VectorAdd(&esum, &ecross, &At<Vec>(e, 0x124));
        *ground = nullptr;
        speeds[i].x = speeds[i].x - esum.x;
        speeds[i].y = speeds[i].y - esum.y;
        speeds[i].z = speeds[i].z - esum.z;
    }
}
bool TooSlow(const Vec& v) {
    float sq = float((double(v.x) * v.x + double(v.y) * v.y) + double(v.z) * v.z);
    return sq < 1e-05f;
}
// the wheels' order this frame (alternating by frame)
int WheelOrder(int parity, int j) {
    int ebx = parity;
    int idx = 0;
    if (parity) {
        switch (j) {
        case 0: idx = ebx - 2; break;
        case 1: idx = ebx; break;
        case 2: idx = ebx - 1; break;
        case 3: idx = ebx + 1; break;
        default: return 0;
        }
    } else {
        switch (j) {
        case 0: idx = ebx; break;
        case 1: idx = ebx - 2; break;
        case 2: idx = ebx - 1; break;
        case 3: idx = ebx + 1; break;
        default: return 0;
        }
    }
    return idx & 3;
}

// a wheel's ProcessWheel: its adhesion from the col point (and the wet, for the
// player), the burst grip factor, its state from last frame
void RunWheel(const Frame& f, int i, Vec* wf, Vec* wr, float thrust, float brake, float bias, float tractionBias) {
    uint8_t* self = f.self;
    uint8_t* record = WheelRecord(self, i);
    record[0x20] = 0x30;
    float adhesion = float(double(Surface_GetAdhesiveLimit(record)) * f.traction);
    if ((At<uint8_t>(self, 0x108) & 0xF8) == 0) {
        adhesion = float(double(Surface_GetWetMultiplier(record[0x22])) * adhesion);
    }
    g_wheelState[i] = At<int32_t>(self, 0x850 + i * 4);
    uint8_t* damage = self + 0x580;
    int count = At<uint8_t>(self, 0x84E);
    float* wheelSpeed = &At<float>(self, 0x7BC + i * 4);
    if (Damage_GetWheelStatus(damage, i) == 1) {
        float grip = float(double(At<float>(damage, 0)) * adhesion * tractionBias);
        Vehicle_ProcessWheel(self, wf, wr, &f.speeds[i], &f.points[i], count, thrust, float(double(bias) * brake), grip, i,
                             wheelSpeed, &g_wheelState[i], 1);
    } else {
        float grip = float(double(adhesion) * tractionBias);
        Vehicle_ProcessWheel(self, wf, wr, &f.speeds[i], &f.points[i], count, thrust, float(double(bias) * brake), grip, i,
                             wheelSpeed, &g_wheelState[i], 0);
    }
}

// a front wheel on the ground: its forward and right turned by the steering
void FrontWheel(const Frame& f, int i, float sn, float cs) {
    uint8_t* self = f.self;
    float thrust = DriveType(self) == 'R' ? 0.0f : f.accel;
    Vec wf = ForwardOf(self);
    const Vec& n = At<Vec>(WheelRecord(self, i), 0x10);
    Flatten(wf, n);
    Vec_Normalise(&wf);
    Vec wr;
    CrossProduct(&wr, &wf, &n);
    Vec_Normalise(&wr);
    Vec a = {float(double(sn) * wr.x), float(double(wr.y) * sn), float(double(sn) * wr.z)};
    Vec b = {float(double(cs) * wf.x), float(double(wf.y) * cs), float(double(cs) * wf.z)};
    Vec fwd;
    VectorSub(&fwd, &b, &a);
    Vec c = {float(double(cs) * wr.x), float(double(wr.y) * cs), float(double(cs) * wr.z)};
    Vec d = {float(double(sn) * wf.x), float(double(wf.y) * sn), float(double(sn) * wf.z)};
    Vec right;
    VectorAdd(&right, &d, &c);
    wf = fwd;
    wr = right;
    RunWheel(f, i, &wf, &wr, thrust, f.brakeForce, f.brakeFront, f.tractionFront);
}

// a wheel off the ground spinning down (or up with the throttle)
void SpinFree(uint8_t* self, int i, float accel, uint8_t undriven, bool limited) {
    float& speed = At<float>(self, 0x7BC + i * 4);
    float& rotation = At<float>(self, 0x79C + i * 4);
    if (DriveType(self) == undriven || accel == 0.0f) {
        speed = float(double(speed) * double(0.95f));
    } else if (0.0f < accel) {
        if (!limited || 2.0f > speed) {
            speed = float(double(speed) - double(0.2f));
        }
    } else if (!limited || -2.0f < speed) {
        speed = float(double(speed) + double(0.1f));
    }
    rotation = float(double(rotation) + speed);
}

// the front wheels (GTA's "Process front wheels on ground" and "off ground")
void FrontWheels(const Frame& f) {
    uint8_t* self = f.self;
    if (0.0f < Timer(self, 0) || 0.0f < Timer(self, 2)) {
        float steer = At<float>(self, 0x2A0);
        float sn = float(std::sin(double(steer)));
        float cs = float(std::cos(double(steer)));
        if (0.0f < Timer(self, 0)) {
            FrontWheel(f, 0, sn, cs);
        }
        if (0.0f < Timer(self, 2)) {
            FrontWheel(f, 2, sn, cs);
        }
    }
    if (!(0.0f < Timer(self, 0))) {
        SpinFree(self, 0, f.accel, 'R', true);
    }
    if (!(0.0f < Timer(self, 2))) {
        SpinFree(self, 2, f.accel, 'R', true);
    }
}

// a rear wheel on the ground: its forward flattened on its ground, its right across
void RearWheel(const Frame& f, int i, Vec& wf, float brake, float traction) {
    uint8_t* self = f.self;
    float thrust = DriveType(self) == 'F' ? 0.0f : f.accel;
    const Vec& n = At<Vec>(WheelRecord(self, i), 0x10);
    Flatten(wf, n);
    Vec_Normalise(&wf);
    Vec wr;
    CrossProduct(&wr, &wf, &n);
    Vec_Normalise(&wr);
    Frame g = f;
    g.traction = traction;
    RunWheel(g, i, &wf, &wr, thrust, brake, f.brakeRear, f.tractionRear);
}

void RearWheels(const Frame& f) {
    uint8_t* self = f.self;
    if (!(0.0f < Timer(self, 1) || 0.0f < Timer(self, 3))) {
        return;
    }
    Vec wf = ForwardOf(self);
    float* matrix = Entity_GetMatrix(self);
    Vec right = {matrix[0], matrix[1], matrix[2]};   // (overwritten by each wheel's own)
    (void)right;
    float brake = f.brakeForce;
    float traction = f.traction;
    if (At<uint8_t>(self, 0x33A) & 4) {
        // the handbrake
        brake = 20000.0f;
        if (!(double(f.forwardSpeed) < 0.1 || double(f.forwardSpeed) == 0.1) &&
            (At<uint32_t>(At<uint8_t*>(self, 0x2A4), 0xCC) & 0x400000)) {
            float grip = float(double(timeStep) * double(0.005f) + At<float>(self, 0x7FC));
            At<float>(self, 0x7FC) = grip;
            if (2.0f < grip) {
                At<float>(self, 0x7FC) = 2.0f;
            }
        }
    } else if (At<uint16_t>(self, 0x7D6) != 0 && DriveType(self) != 'F') {
        // a burnout: no brake, no grip, the car turned by the steering
        brake = 0.0f;
        traction = 0.0f;
        float* m = Entity_GetMatrix(self);
        float steer = At<float>(self, 0x2A0);
        float t = float(double(At<float>(self, 0x15C)) * steer * double(-0.001f));
        Vec force = {float(double(m[0]) * t), float(double(m[1]) * t), float(double(t) * m[2])};
        const Vec& point = f.points[1];
        // (GTA's argument order: the point as the force, the force as the point)
        Physical_ApplyTurnForce(self, point.x, point.y, point.z, force.x, force.y, force.z);
    } else if (1.0f < At<float>(self, 0x7FC)) {
        traction = float(double(traction) * At<float>(self, 0x7FC));
    }
    float slide = At<float>(self, 0x470);
    if (0.0f < slide) {
        brake = float(double(brake) * 0.75 + 0.25 * brake * double(1.2f) * slide);
    }
    if (0.0f < Timer(self, 1)) {
        RearWheel(f, 1, wf, brake, traction);
    }
    if (0.0f < Timer(self, 3)) {
        Vec wf3 = ForwardOf(self);
        RearWheel(f, 3, wf3, brake, traction);
    }
}

// the rear wheels off the ground (stopped by the handbrake; the right one's spin
// unlimited)
void RearOffGround(uint8_t* self, float accel) {
    if (!(0.0f < Timer(self, 1))) {
        if (At<uint8_t>(self, 0x33A) & 4) {
            At<float>(self, 0x7C0) = 0.0f;
        } else {
            SpinFree(self, 1, accel, 'F', true);
        }
    }
    if (!(0.0f < Timer(self, 3))) {
        if (At<uint8_t>(self, 0x33A) & 4) {
            At<float>(self, 0x7C8) = 0.0f;
        } else {
            SpinFree(self, 3, accel, 'F', false);
        }
    }
}

// the heading set (the matrix turned, the placement's heading)
void SetHeading(uint8_t* self, float heading) {
    float* matrix = At<float*>(self, 0x14);
    if (matrix) {
        Matrix_SetRotateZOnly(matrix, heading);
    }
    At<float>(self, 0x10) = heading;
}

} // namespace

namespace autopc09 {

// 0x4AE430 (vf12): CAutomobile::ProcessControl
void __fastcall ProcessControl(CAutomobile* automobile, re::Edx) {
    uint8_t* self = reinterpret_cast<uint8_t*>(automobile);
    void* seats = self + 0x1C4;
    uint8_t* handling = At<uint8_t*>(self, 0x2A4);
    int32_t vehicleClass;
    Vehicle_UpdateFade(self);
    if ((At<uint8_t>(self, 0x108) & 0xF8) == 0x20 && Seats_GetOccupant(seats, 0)) {
        // wrecked with someone in it: back to player / AI driving
        int status;
        if (At<int32_t>(Seats_GetOccupant(seats, 0), 0x1310) == 0xD) {
            status = 0;
        } else if (!(At<uint8_t>(self, 0x33C) & 8) && *Vehicle_GetVehicleClass(self, &vehicleClass) != 7 &&
                   *Vehicle_GetVehicleClass(self, &vehicleClass) != 6 && *Vehicle_GetVehicleClass(self, &vehicleClass) != 10) {
            status = 2;
        } else {
            status = 3;
        }
        using SetStatus = void(__thiscall*)(void*, int);
        re::PtrCast<SetStatus>(automobile->vtable[0x18 / 4])(self, status);
    }
    Automobile_4AB100(self, 0.0f, 0);
    if ((At<uint8_t>(self, 0x108) & 0xF8) == 0x20) {
        Physical_468DF0(self);
        return;
    }
    {
        using Fn = void(__thiscall*)(void*, void*);
        void* fn = automobile->vtable[0x9C / 4];
        reinterpret_cast<Fn>(fn)(self, Entity_GetMatrix(self));
    }
    uint8_t flags33A = At<uint8_t>(self, 0x33A);
    uint8_t* colModel;
    if (flags33A & 0x80) {
        colModel = static_cast<uint8_t*>(g_specialColModel);
    } else {
        colModel = At<uint8_t*>(modelInfoPtrs[At<int16_t>(self, 0x10E)], 0xC);
    }
    At<uint8_t>(self, 0x7CC) &= 0xFE;
    At<uint8_t>(self, 0x33B) &= 0xBF;
    At<uint8_t>(self, 0x33A) = flags33A & 0xEF;
    uint8_t slidSideways = 0;
    At<uint16_t>(self, 0x7D6) = 0;
    if (*Vehicle_GetVehicleClass(self, &vehicleClass) == 10 && !Entity_GetIsOnScreen(self)) {
        Automobile_4AD740(self);
    }
    Vehicle_4CC350(self);
    if (At<uint8_t>(self, 0x33C) & 4) {
        return;
    }

    // the centre of mass eased back to the handling's
    Vec& com = At<Vec>(self, 0x16C);
    const Vec& handlingCom = At<Vec>(handling, 0x1C);
    float k = float(std::pow(double(0.77f), double(timeStep)));
    Vec d, kd, sum;
    VectorSub(&d, &com, &handlingCom);
    kd = {float(double(d.x) * k), float(double(d.y) * k), float(double(k) * d.z)};
    VectorAdd(&sum, &handlingCom, &kd);
    com = sum;
    if (double(std::fabs(float(double(com.x) - handlingCom.x))) < 0.02) {
        com.x = handlingCom.x;
    }
    if (double(std::fabs(float(double(com.y) - handlingCom.y))) < 0.01) {
        com.y = handlingCom.y;
    }

    // mission cars look ahead every 16 frames
    if ((At<uint8_t>(self, 0x33C) & 1) && !(At<uint8_t>(self, 0x33B) & 0x80) && At<uint8_t>(self, 0x290) != 2 &&
        At<uint8_t>(self, 0x2DA) == 1 && ((At<uint8_t>(self, 0x10C) + frameCounter) & 0xF) == 0) {
        const Vec* pos = EntityPos(self);
        Vec forward, a, b, c, end;
        Entity_GetForward(self, &forward);
        float* matrix = Entity_GetMatrix(self);
        VectorScale(&a, reinterpret_cast<Vec*>(matrix), 3.0f);
        VectorScale(&b, &forward, 10.0f);
        VectorAdd(&c, pos, &b);
        VectorAdd(&end, &c, &a);
        alignas(4) uint8_t colPoint[0x60];
        ColPoint_Construct(colPoint);
        re::Ptr32<void> hit = nullptr;
        if (!World_ProcessLineOfSight(pos, &end, colPoint, &hit, 7, 0, 0) || hit == self) {
            CarAI_GoToCoorsSlowly(self, &end);
        }
        ColPoint_Destruct(colPoint);
    }

    Vec& ms = At<Vec>(self, 0x124);
    auto onSand = [&]() {
        // a wheel on the ground on sand (surface group 4). (The surface goes as a dword
        // whose upper bytes are stack garbage in the game)
        for (int i = 0; i < 4; i++) {
            if (1.0f > Ratio(self, i) && Surface_GetGroup(WheelRecord(self, i)[0x22]) == 4) {
                return true;
            }
        }
        return false;
    };
    switch (At<uint8_t>(self, 0x108) >> 3) {
    case 0:
        if (Seats_GetOccupant(seats, 0)) {
            if (Seats_GetOccupant(seats, 0) && At<int32_t>(Seats_GetOccupant(seats, 0), 0x1310) == 0xD) {
                using Fn = void(__thiscall*)(void*);
                re::PtrCast<Fn>(automobile->vtable[0xA8 / 4])(self);
            }
            Automobile_450970(self);
            if (onSand()) {
                // sand drags the car sideways to a stop
                float* m1 = Entity_GetMatrix(self);
                float* m2 = Entity_GetMatrix(self);
                float along = DotProduct(&ms, reinterpret_cast<Vec*>(m2 + 8));
                Vec up, lateral;
                ScaleVector(&up, along, reinterpret_cast<Vec*>(m1 + 8));
                VectorSub(&lateral, &ms, &up);
                float mag = Vec_Magnitude(&lateral);
                float coef;
                if (!(mag < 0.09f || mag == 0.09f)) {
                    float r = SqrtAbs(mag);
                    Vec v = {float(double(lateral.x) * double(0.3f)), float(double(lateral.y) * double(0.3f)),
                             float(double(0.3f) * lateral.z)};
                    Vec out;
                    Vec* o = Fn565DA0(&out, &v, r);
                    lateral = *o;
                    float rest = float(1.0 - (double(r) + r));
                    coef = float(double(Max(0.2f, rest)) * double(0.01f));
                } else {
                    At<uint8_t>(self, 0x7CC) |= 1;
                    coef = 0.01f;
                }
                if (At<uint32_t>(handling, 0xCC) & 0x4000000) {
                    coef = float(double(coef) * 0.5);
                }
                if (!(double(g_sandGrip) < double(0.2f) || double(g_sandGrip) == double(0.2f))) {
                    coef = float((double(1.2f) - g_sandGrip) * coef);
                }
                float s = float(-double(coef) * At<float>(self, 0x158) * timeStep);
                Vec force;
                ScaleVector(&force, s, &lateral);
                Physical_ApplyMoveForce(self, &force);
                slidSideways = 1;
            }
        }
        break;
    case 2: {
        CarAI_UpdateCarAI(self);
        Physical_ProcessControl(self);
        At<uint8_t>(self, 0x84E) = 4;
        At<uint8_t>(self, 0x84F) = 4;
        Vec forward;
        float turn = Vehicle_CollisionImpulse(self, 0, Entity_GetForward(self, &forward), &ms, 0.35f);
        for (int i = 0; i < 4; i++) {
            At<float>(self, 0x79C + i * 4) = float(double(At<float>(self, 0x79C + i * 4)) + turn);
        }
        Automobile_HonkWhenStuck(self);
        Automobile_CountDown368(self);
        At<uint8_t>(self, 0x33B) &= 0xF5;
        return;
    }
    case 3:
        CarAI_UpdateCarAI(self);
        CarCtrl_SteerAICarWithPhysics(self);
        Automobile_HonkWhenStuck(self);
        if (onSand()) {
            At<uint8_t>(self, 0x7CC) |= 1;
            float s;
            if (g_sandGrip > 0.0f) {
                s = float((1.0 - g_sandGrip) * double(-0.01f) * At<float>(self, 0x158) * timeStep);
            } else {
                s = float(double(At<float>(self, 0x158)) * double(-0.01f) * timeStep);
            }
            Vec force;
            ScaleVector(&force, s, &ms);
            Physical_ApplyMoveForce(self, &force);
        }
        break;
    case 4:
    case 5:
    case 6:
    case 7: {
        // abandoned / wrecked / disabled: braking, the steering straight, no horn
        uint8_t status = At<uint8_t>(self, 0x108) >> 3;
        float brake;
        bool slow = status != 5 && Vec_Magnitude(&ms) < 0.01f;
        if (status == 5) {
            brake = 0.05f;
            At<uint8_t>(self, 0x33A) |= 4;
        } else if (status == 6 && slow) {
            At<uint8_t>(self, 0x33A) |= 4;
            At<float>(self, 0x298) = 1.0f;
            At<float>(self, 0x294) = float(double(At<float>(self, 0x29C)) * 0.0);
            goto straighten;
        } else {
            brake = (status != 6 && slow) ? 0.2f : 0.0f;
            At<uint8_t>(self, 0x33A) &= 0xFB;
        }
        At<float>(self, 0x298) = brake;
    straighten : {
        using Fn = void(__thiscall*)(void*, float);
        re::PtrCast<Fn>(automobile->vtable[0xE0 / 4])(self, 0.0f);
    }
        At<int32_t>(self, 0x368) = 0;
        At<float>(self, 0x294) = float(double(At<float>(self, 0x29C)) * 0.0);
        break;
    }
    default:
        break;
    }

    // resting on moving grounds: postponed while one isn't placed yet
    for (int i = 0; i < 4; i++) {
        uint8_t* ground = At<uint8_t*>(self, 0x83C + i * 4);
        if (ground) {
            At<uint8_t>(self, 0x33B) |= 0x40;
            if (!g_C1AE6F && At<int32_t>(ground, 0x38) != 0) {
                At<int32_t>(self, 0x40) = 1;
                return;
            }
        }
    }
    if (At<uint8_t>(self, 0x33B) & 0x40) {
        At<uint8_t>(self, 0x180) = 0;
    }
    Automobile_ProcessPowerSlide(self);
    Physical_ProcessControl(self);
    Automobile_4AE000(self);

    // the spring ratios rescaled (the wheel radius taken off)
    for (int i = 0; i < 4; i++) {
        float radius = float(1.0 - double(At<float>(self, 0x7D8 + i * 4)) / At<float>(self, 0x7E8 + i * 4));
        Ratio(self, i) = float((double(Ratio(self, i)) - radius) / (1.0 - radius));
    }
    Vec springDirs[4] = {};
    Vec speeds[4] = {};
    Vec points[4] = {};
    Vec forward = ForwardOf(self);
    float absSpeed = std::fabs(float((double(ms.y) * forward.y + double(forward.x) * ms.x) + double(ms.z) * forward.z));
    uint8_t* lines = At<uint8_t*>(At<uint8_t*>(colModel, 0x38), 4);
    uint8_t* damage = self + 0x580;
    for (int i = 0; i < 4; i++) {
        uint8_t* record = WheelRecord(self, i);
        if (Damage_GetWheelStatus(damage, i) == 2) {
            Ratio(self, i) = 1.0f;
            continue;
        }
        float length = At<float>(self, 0x7D8 + i * 4);
        float line = At<float>(self, 0x7E8 + i * 4);
        if (Damage_GetWheelStatus(damage, i) == 1) {
            // a burst wheel bumps, the more the faster
            uint32_t range = uint16_t(int(double(absSpeed) * 40.0)) + 0x61u;
            if (Rand() * int(range) / 0x7FFF < 100) {
                float r = float((double(line) - length) * double(0.3f) / length + Ratio(self, i));
                Ratio(self, i) = r;
                if (1.0f < r) {
                    Ratio(self, i) = 1.0f;
                    continue;
                }
            }
        } else if (Surface_GetGroup(record[0x22]) == 4) {
            // sand sinks the wheels, less the faster
            float sink = float(1.0 - double(absSpeed) * double(0.7f) / double(0.3f) - double(0.7f) * g_sandGrip);
            if (double(sink) < double(0.4f)) {
                sink = 0.4f;
            }
            float r = float((double(line) - length) * (double(sink) * double(0.35f)) / length + Ratio(self, i));
            Ratio(self, i) = r;
            if (1.0f < r) {
                Ratio(self, i) = 1.0f;
                continue;
            }
        }
        if (1.0f > Ratio(self, i)) {
            Vec rel;
            VectorSub(&rel, reinterpret_cast<Vec*>(record), Placeable_GetPosition(self));
            points[i] = rel;
            Vec dir;
            VectorSub(&dir, reinterpret_cast<Vec*>(lines + i * 0x20 + 0x10), reinterpret_cast<Vec*>(lines + i * 0x20));
            float* m = Entity_GetMatrix(self);
            springDirs[i] = Multiply3x3(m, dir);
            Vec_Normalise(&springDirs[i]);
        }
    }

    // the springs push the car up (in an order alternating by frame)
    uint8_t winter = g_chapterSeason[g_chapter] == 2;
    uint8_t interior = IsAreaInterior(currArea);
    uint8_t area42 = currArea == 0x2A;
    int parity = int(frameCounter & 1);
    for (int j = 0; j < 4; j++) {
        int idx = WheelOrder(parity, j);
        if (1.0f > Ratio(self, idx)) {
            float bias = At<float>(handling, 0xBC);
            if (idx == 1 || idx == 3) {
                bias = float(1.0 - bias);
            }
            Vec up = {0.0f, 0.0f, 1.0f};
            const Vec* normal;
            if (double(std::fabs(At<float>(self, 0x6DC))) > double(0.93f) &&
                double(std::fabs(At<float>(self, 0x704))) > double(0.93f)) {
                normal = &up;
            } else {
                normal = &At<Vec>(WheelRecord(self, idx), 0x10);
            }
            Physical_ApplySpringCollision(self, At<float>(handling, 0xAC), &springDirs[idx], &points[idx], Ratio(self, idx), bias,
                                          normal);
            uint8_t* surface = &WheelRecord(self, idx)[0x22];
            At<uint8_t>(self, 0x798 + idx) = 0;
            switch (*surface) {
            case 3:
            case 5:
                At<uint8_t>(self, 0x794 + idx) = 1;
                break;
            case 27:
            case 54:
                At<uint8_t>(self, 0x794 + idx) = 2;
                At<uint8_t>(self, 0x798 + idx) = 1;
                break;
            default:
                At<uint8_t>(self, 0x794 + idx) = 0;
                break;
            }
            if (winter == 1 && interior == 1 && !(area42 == 1 && *surface == 1)) {
                At<uint8_t>(self, 0x794 + idx) = 3;
            }
        } else {
            float* m = Entity_GetMatrix(self);
            points[idx] = Multiply3x3(m, *reinterpret_cast<Vec*>(lines + idx * 0x20 + 0x10));
        }
    }

    // the contact points' speeds; wheels on steep ground pushed along their normal
    for (int i = 0; i < 4; i++) {
        ContactSpeed(self, i, speeds, points);
        const Vec& n = At<Vec>(WheelRecord(self, i), 0x10);
        if (1.0f > Ratio(self, i) && 0.35f < n.z) {
            springDirs[i] = {-n.x, -n.y, -n.z};
        }
        if (TooSlow(speeds[i])) {
            speeds[i] = ms;
        }
    }
    // the springs dampened
    for (int j = 0; j < 4; j++) {
        int idx = WheelOrder(parity, j);
        if (double(Ratio(self, idx)) < double(0.99999f)) {
            Vec down = {0.0f, 0.0f, -1.0f};
            const Vec* dir;
            if (double(std::fabs(At<float>(self, 0x6DC))) > double(0.93f) &&
                double(std::fabs(At<float>(self, 0x704))) > double(0.93f)) {
                dir = &down;
            } else {
                dir = &springDirs[idx];
            }
            Physical_ApplySpringDampening(self, At<float>(handling, 0xB0), dir, &points[idx], &speeds[idx]);
        }
    }
    for (int i = 0; i < 4; i++) {
        ContactSpeed(self, i, speeds, points);
        if (TooSlow(speeds[i])) {
            speeds[i] = ms;
        }
    }

    // the transmission (the power slide adds throttle)
    forward = ForwardOf(self);
    float forwardSpeed = float((double(ms.y) * forward.y + double(ms.x) * forward.x) + double(ms.z) * forward.z);
    float gas = At<float>(self, 0x294);
    if (0.0f < At<float>(self, 0x470)) {
        At<float>(self, 0x29C) = 1.0f;
        double boost = double(Vehicle_GetUnknown470(self)) * 5.0 * 0.25;
        gas = float(double(gas) * 0.75 + boost);
    }
    float accel = Transmission_CalculateDriveAcceleration(handling + 0x34, &gas, &forwardSpeed, self, 0.0f, 1.0f, 3.4028235e38f);

    // a wheelie: the player's car flag 8, full throttle, no brake, moving on, a wheel of
    // each side on the ground
    if ((At<uint8_t>(self, 0x33B) & 8) && double(At<float>(self, 0x294)) > double(0.4f) &&
        double(At<float>(self, 0x298)) < double(0.1f) && 0.15f < forwardSpeed && FindPlayerVehicle(0) == self) {
        if ((1.0f > Ratio(self, 0) || 1.0f > Ratio(self, 2)) && (1.0f > Ratio(self, 1) || 1.0f > Ratio(self, 3))) {
            Vec f = ForwardOf(self);
            Vec point = {float(double(f.x) * -1.0), float(double(f.y) * -1.0), float(-1.0 * f.z)};
            float* m = Entity_GetMatrix(self);
            float turnMass = At<float>(self, 0x15C);
            float t = turnMass < 2500.0f ? turnMass : 2500.0f;
            t = float(double(t) * double(-0.008f));
            Vec force = {float(double(m[8]) * t), float(double(m[9]) * t), float(double(t) * m[10])};
            Physical_ApplyTurnForce(self, force.x, force.y, force.z, point.x, point.y, point.z);
        }
    }

    Frame fr;
    fr.self = self;
    fr.accel = accel;
    fr.forwardSpeed = forwardSpeed;   // (as the transmission left it)
    fr.speeds = speeds;
    fr.points = points;
    fr.brakeForce = float(double(At<float>(handling, 0x90)) * At<float>(self, 0x298) * timeStep);
    if ((At<uint8_t>(self, 0x108) & 0xF8) != 0 && (At<uint32_t>(handling, 0xCC) & 0x1000)) {
        fr.brakeFront = 1.0f;
        fr.brakeRear = 1.0f;
        fr.tractionFront = 1.0f;
        fr.tractionRear = 1.0f;
    } else {
        float brakeBias = At<float>(handling, 0x94);
        fr.brakeFront = float(2.0 * brakeBias);
        fr.brakeRear = float(2.0 - brakeBias);   // (not 2 - front: the game's)
        fr.tractionFront = float(At<float>(handling, 0xA4) * 2.0);
        fr.tractionRear = float(2.0 - double(fr.tractionFront));
    }

    // the wheels' timers and how many are on the ground; the average ground normal
    At<uint8_t>(self, 0x84E) = 0;
    At<uint8_t>(self, 0x84F) = 0;
    Vec& normal = At<Vec>(self, 0x864);
    for (int i = 0; i < 4; i++) {
        bool onGround;
        if (1.0f > Ratio(self, i)) {
            Timer(self, i) = 4.0f;
            onGround = true;
        } else {
            float v = float(double(Timer(self, i)) - timeStep);
            if (!(v > 0.0f)) {
                v = 0.0f;
            }
            Timer(self, i) = v;
            onGround = v > 0.0f;
        }
        if (onGround) {
            At<uint8_t>(self, 0x84E)++;
            uint8_t drive = At<uint8_t>(handling, 0x7C);
            if (drive == '4' || (drive == 'F' && (i == 0 || i == 2)) || (drive == 'R' && (i == 1 || i == 3))) {
                At<uint8_t>(self, 0x84F)++;
            }
        }
        // (every wheel, on the ground or not, is summed in)
        const Vec& n = At<Vec>(WheelRecord(self, i), 0x10);
        if (At<uint8_t>(self, 0x84E) == 1) {
            normal = n;
        } else {
            normal.x = normal.x + n.x;
            normal.y = normal.y + n.y;
            normal.z = normal.z + n.z;
        }
    }
    uint8_t onGround = At<uint8_t>(self, 0x84E);
    if (onGround == 0) {
        normal = g_defaultNormal;
    } else {
        float inv = float(1.0 / double(float(int(onGround))));
        normal = {float(double(normal.x) * inv), float(double(normal.y) * inv), float(double(inv) * normal.z)};
        float* m = Entity_GetMatrix(self);
        float up = float((double(m[9]) * normal.y + double(m[8]) * normal.x) + double(m[10]) * normal.z);
        if (up < -0.5f) {
            normal = {float(normal.x * -1.0), float(normal.y * -1.0), float(-1.0 * normal.z)};
        }
    }
    fr.traction = float(double(At<float>(handling, 0x30)) * g_tractionScale * 0.25);

    // the wheels: front then rear (rear first for handling flag 0x200000)
    uint8_t rearFirst = (At<uint32_t>(handling, 0xCC) >> 21) & 1;
    if (!rearFirst) {
        FrontWheels(fr);
    }
    RearWheels(fr);
    // the rear grip: built up in a burnout, else back to 1
    if (At<uint16_t>(self, 0x7D6) != 0 && DriveType(self) != 'F' &&
        (At<int32_t>(self, 0x854) == 1 || At<int32_t>(self, 0x85C) == 1)) {
        float grip = float(double(timeStep) * double(0.001f) + At<float>(self, 0x7FC));
        At<float>(self, 0x7FC) = grip;
        if (3.0f < grip) {
            At<float>(self, 0x7FC) = 3.0f;
        }
    } else if (1.0f < At<float>(self, 0x7FC)) {
        float p = float(std::pow(double(0.995f), double(timeStep)));
        At<float>(self, 0x7FC) = float((double(At<float>(self, 0x7FC)) - 1.0) * p + 1.0);
    }
    RearOffGround(self, accel);
    if (rearFirst) {
        FrontWheels(fr);
    }

    // the wheels' positions eased to their suspension
    for (int i = 0; i < 4; i++) {
        float pos = At<float>(lines, i * 0x20 + 8);
        if (0.0f < Ratio(self, i)) {
            pos = float(double(pos) - double(At<float>(self, 0x7D8 + i * 4)) * Ratio(self, i));
        }
        float& wheelPos = At<float>(self, 0x7AC + i * 4);
        wheelPos = float((double(pos) - wheelPos) * 0.75 + wheelPos);
    }
    for (int i = 0; i < 4; i++) {
        At<int32_t>(self, 0x850 + i * 4) = g_wheelState[i];
    }
    if (At<float>(self, 0x294) < 0.0f) {
        if (At<int32_t>(self, 0x854) == 1) {
            At<int32_t>(self, 0x854) = 0;
        }
        if (At<int32_t>(self, 0x85C) == 1) {
            At<int32_t>(self, 0x85C) = 0;
        }
    }
    Automobile_4AD1D0(self);
    // the player's horn (control 8; not for a siren model)
    if ((At<uint8_t>(self, 0x108) & 0xF8) == 0) {
        if (!Vehicle_HasSirenModel(self) && Pad_GetValue(8, 0) > 0.0f) {
            At<int32_t>(self, 0x368) = 1;
        } else {
            At<int32_t>(self, 0x368) = 0;
        }
    }
    Damage_66A8D0(damage);
    Damage_GetDoorStatus(damage, 0);
    Entity_GetMatrix(self);

    // the wheels landing hard: their sound (0x589B00)
    float speedSq = float((double(ms.x) * ms.x + double(ms.y) * ms.y) + double(ms.z) * ms.z);
    float hardest = 0.0f;
    float rough = 0.0f;
    for (int i = 0; i < 4; i++) {
        float drop = float(double(At<float>(self, 0x774 + i * 4)) - Ratio(self, i));
        if (drop > 0.3f && !slidSideways && 0.04f < speedSq) {
            if (Damage_GetWheelStatus(damage, i) != 1) {
                Call589B00WithRef(self);
            }
            if (hardest < drop) {
                hardest = drop;
            }
        } else if (drop > 0.25f && !slidSideways && double(speedSq) > 0.2) {
            Call589B00WithRef(self);
        }
        // (a roughness by the surface, unused)
        uint8_t surface = WheelRecord(self, i)[0x22];
        if (surface == 4 || surface == 0x1E || surface == 0x2B) {
            if (double(rough) < double(0.2f)) {
                rough = 0.3f;
            }
        } else if (surface == 5) {
            if (double(rough) < double(0.1f)) {
                rough = 0.2f;
            }
        } else if (surface == 3) {
            if (double(rough) < double(0.05f)) {
                rough = 0.1f;
            }
        }
        At<float>(self, 0x774 + i * 4) = Ratio(self, i);
        Ratio(self, i) = 1.0f;
    }
    (void)hardest;
    (void)rough;
    At<uint8_t>(self, 0x33B) &= 0xF5;
    if (!(At<uint8_t>(self, 0x33A) & 0x10)) {
        Fn41EE50(self);
    }

    // the map's edge (850 m out) turns cars back in
    const Vec* pos = EntityPos(self);
    if (pos->x > 850.0f) {
        if (0.0f < ms.x) {
            ms.x = float(ms.x * -1.0);
        }
        float h = HeadingOf(self);
        if (h < 0.0f) {
            SetHeading(self, -h);
        }
    } else if (double(EntityPos(self)->x) < -850.0) {
        if (!(0.0f < ms.x || 0.0f == ms.x)) {
            ms.x = float(ms.x * -1.0);
        }
        float h = HeadingOf(self);
        if (h > 0.0f) {
            SetHeading(self, -h);
        }
    }
    pos = EntityPos(self);
    if (pos->y > 850.0f) {
        if (0.0f < ms.y) {
            ms.y = float(ms.y * -1.0);
        }
        float h = HeadingOf(self);
        if (g_halfPi > h && 0.0f < h) {
            SetHeading(self, float(double(3.1415927f) - h));
        } else if (-g_halfPi > h && !(0.0f < h || 0.0f == h)) {
            SetHeading(self, float(-double(3.1415927f) - h));
        }
    } else if (double(EntityPos(self)->y) < -850.0) {
        if (!(0.0f < ms.y || 0.0f == ms.y)) {
            ms.y = float(ms.y * -1.0);
        }
        float h = HeadingOf(self);
        if (g_halfPi < h) {
            SetHeading(self, float(double(3.1415927f) - h));
        } else if (-g_halfPi > h) {
            SetHeading(self, float(-double(3.1415927f) - h));
        }
    }

    // frozen: stopped dead
    if (At<uint8_t>(self, 0x1BC) & 4) {
        ms = vecUnset;
        At<Vec>(self, 0x130) = vecUnset;
        return;
    }
    // stopped on all four wheels with no throttle or brake (or disabled): held still,
    // unless the player just hit it
    bool idle = At<float>(self, 0x294) == 0.0f && fr.brakeForce == 0.0f;
    if (!idle && (At<uint8_t>(self, 0x108) & 0xF8) != 0x28) {
        return;
    }
    if (!(0.005f > std::fabs(ms.x)) || !(0.005f > std::fabs(ms.y)) || !(std::fabs(ms.z) < 0.02f)) {
        return;
    }
    if (0.0f < At<float>(self, 0x1A0) && At<uint8_t*>(self, 0x1A4) == playerPed) {
        return;
    }
    for (int i = 0; i < 4; i++) {
        if (!(1.0f > At<float>(self, 0x774 + i * 4))) {
            return;
        }
    }
    ms.x = 0.0f;
    ms.y = 0.0f;
    At<float>(self, 0x138) = 0.0f;
}

} // namespace autopc09

// ---- registration ---------------------------------------------------------------------

#define AUTOPC09_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

AUTOPC09_REVERSED("CAutomobile::ProcessControl", 0x004AE430, &autopc09::ProcessControl);
