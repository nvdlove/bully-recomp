// CVehicle's shared physics and upkeep: a wheel's forces (GTA's CVehicle::ProcessWheel),
// damage taken (InflictDamage), the collision resolution after a move, the steering
// input, the "out of the loaded world" flag, a clear path ahead.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "game/Vehicle.h"
#include "reversed.h"

#include "game/Crt.h"
namespace c11vehmisc {

using veh::CVector;

// the game's CColPoint (0x28 bytes; +0xC a registered reference)
struct ColPoint {
    CVector point;          // +0x0
    re::Ptr32<CEntity> entity;        // +0xC a registered reference
    CVector normal;         // +0x10 (what SetDamage gets as the position)
    uint8_t unknown1C[0x7];
    uint8_t piece;          // +0x23
    float depth;            // +0x24
};
static_assert(offsetof(ColPoint, entity) == 0xC);
static_assert(offsetof(ColPoint, piece) == 0x23);
static_assert(offsetof(ColPoint, depth) == 0x24);
static_assert(sizeof(ColPoint) == 0x28);

// 0x4CB350 (thiscall, the vehicle unused): the steering pad's value (negated; with the
// second stick when the alternative controls are on); 0 while input is off
float __fastcall GetSteerInput(CVehicle* self, re::Edx edx, int primary);
// 0x4CB8D0: GTA's CVehicle::ProcessWheel
void __fastcall ProcessWheel(CVehicle* self, re::Edx edx, CVector* wheelFwd, CVector* wheelRight, CVector* contactSpeed,
                             CVector* contactPoint, int wheelsOnGround, float thrust, float brake, float adhesion,
                             int wheelId, int unused, int32_t* wheelState, int wheelStatus);
// 0x4CC350: flag 4 of +0x33C while the collision where it stands isn't loaded (not
// for random vehicles)
void __fastcall UpdateCollisionLoadedFlag(CVehicle* self, re::Edx edx);
// 0x4CC3D0: a radar blip on it
void __fastcall AddRadarBlip(CVehicle* self, re::Edx edx);
// 0x4CC4A0: damage from an attacker (GTA's CVehicle::InflictDamage, with the hit found
// by a sweep when there's no collision point)
void __fastcall InflictDamage(CVehicle* self, re::Edx edx, CEntity* attacker, int weaponType, float damage, float x,
                              float y, float z, ColPoint* colPoint, int notifyDriver);
// vf51 0x4CCA10: nothing else within (its length / 2 + 3) ahead
uint8_t __fastcall IsPathAheadClear(CVehicle* self, re::Edx edx);
// 0x4CCC00: the driver's move direction (its +0x2E0) in the vehicle's space, flat;
// returns out
CVector* __fastcall GetDriverMoveDirection(CVehicle* self, re::Edx edx, CVector* out);
// 0x4CCD30: moved up by *dz, then up to 4 rounds of collisions found and applied (the
// matrix saved into `savedMatrix` first and put back when it never settles: 2)
int32_t __fastcall ResolveCollisions(CVehicle* self, re::Edx edx, float* savedMatrix, float* dz, int unused,
                                     ColPoint* outColPoint, uint32_t keepFlags, int arg6);
// 0x4CD2A0: GTA's CVehicle::DoSunGlare (cars only): star coronas on the body's flat
// quads facing the sun and the camera
void __fastcall DoSunGlare(CVehicle* self, re::Edx edx);
// 0x4CDC30: put at the saved matrix's height plus dz (*outDz: how far that moved it),
// then up to 4 rounds of collisions: the deepest hit responded to in the first
// round (vehicle-vehicle and peds by the pair response; hits on a player bike's
// vehicle damage the other), the matrix put back when it never settles (2)
int32_t __fastcall ResolveCollisionsAtHeight(CVehicle* self, re::Edx edx, float* savedMatrix, float dz, float* outDz,
                                             ColPoint* outColPoint, uint32_t keepFlags, int arg6, CVector* outHeight);

} // namespace c11vehmisc

namespace {

using namespace c11vehmisc;

GAME_FN(uint8_t, __thiscall, UserInput_IsEnabled, 0x0050DF50, void* input);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int arg);
GAME_FN(void*, __cdecl, GetPlayerPadInput, 0x00738070);
GAME_FN(uint8_t, __cdecl, Input_738500, 0x00738500, void* input);
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(uint8_t, __cdecl, IsCollisionLoaded, 0x005701C0, const float* xy);
GAME_FN(int, __cdecl, GetVehicleRef, 0x0044CAC0, CVehicle* vehicle);
GAME_FN(int, __thiscall, Radar_AddBlipFor, 0x0054A5B0, void* radar, int kind, int handle, uint32_t colour, int a,
        int icon, int c);
GAME_FN(CVehicle*, __cdecl, FindPlayerVehicle, 0x00446970, uintptr_t player);
GAME_FN(void, __thiscall, Stimulus_Add, 0x004A6670, void* manager, int kind, CEntity* from, CEntity* to,
        const CVector* pos, float a, float b);
GAME_FN(void, __thiscall, Automobile_SetDamage, 0x004A8410, CVehicle* v, int piece, float impulse, CEntity* entity,
        const CVector* pos);
GAME_FN(void, __thiscall, Automobile_DamageAt, 0x004A85C0, CVehicle* v, ColPoint* colPoint, float impulse);
GAME_FN(uint8_t, __cdecl, Fn_42E430, 0x0042E430);
GAME_FN(void, __cdecl, IncrementStat, 0x00458480, int stat, int by);
GAME_FN(CPed*, __thiscall, Seats_GetOccupant, 0x004CE560, CVehicleSeats* seats, uint32_t seat);
GAME_FN(int32_t*, __thiscall, Vehicle_GetVehicleClass, 0x004CAF30, CVehicle* v, int32_t* out);
GAME_FN(void, __thiscall, ColPoint_Construct, 0x00415FA0, ColPoint* cp);
GAME_FN(void, __thiscall, ColPoint_Destruct, 0x00415500, ColPoint* cp);
GAME_FN(ColPoint*, __thiscall, ColPoint_Assign, 0x0046AA70, ColPoint* self, const ColPoint* other);
GAME_FN(void, __thiscall, Matrix_SetUnity, 0x00412770, col::CMatrix* m);
GAME_FN(void, __thiscall, Matrix_SetTranslate, 0x004127C0, col::CMatrix* m, const CVector* pos);
GAME_FN(void, __thiscall, Matrix_CopyOnlyMatrix, 0x00412720, void* dst, const void* src);
GAME_FN(void, __cdecl, Matrix_Invert, 0x00413770, const void* m, col::CMatrix* out);
GAME_FN(CVector*, __cdecl, Multiply3x3, 0x0041C490, CVector* out, const void* m, const CVector* v);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(float, __thiscall, Vector_Normalise2, 0x004294A0, CVector* v);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, CrossProduct, 0x00414730, CVector* out, const CVector* a, const CVector* b);
GAME_FN(void, __thiscall, Box_Set, 0x00564830, void* box, const CVector* min, const CVector* max);
GAME_FN(uint8_t, __cdecl, Col_ProcessSweep, 0x0056C8C0, col::CMatrix* to, col::CMatrix* from, void* colModel,
        void* matrixA, void* matrixB, void* colModel2, ColPoint* colPoint, int, int, int, int, int, int lists, uintptr_t, int);
GAME_FN(uint8_t, __cdecl, ProcessLineOfSight, 0x00461EA0, const CVector* start, const CVector* end, ColPoint* colPoint,
        re::Ptr32<CEntity>* entity, int flags, int, int);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, CEntity* entity, re::Ptr32<CEntity>* ref);
GAME_FN(float*, __thiscall, Entity_GetMatrix, 0x008B6120, CVehicle* v);
GAME_FN(CVector*, __thiscall, Entity_GetForward, 0x004154A0, CVehicle* v, CVector* out);
GAME_FN(CVector*, __thiscall, Entity_GetUp, 0x00473730, CVehicle* v, CVector* out);
GAME_FN(void, __thiscall, Physical_ApplyMoveForce, 0x0046A7E0, CVehicle* v, float x, float y, float z);
GAME_FN(void, __thiscall, Physical_ApplyTurnForce, 0x0046A950, CVehicle* v, float fx, float fy, float fz, float px,
        float py, float pz);
GAME_FN(uint8_t, __thiscall, Physical_ProcessCollisionWith, 0x0046CAC0, CVehicle* v, void* matrix, CEntity* entity,
        ColPoint* colPoint, int arg, int arg5);
GAME_FN(void, __thiscall, Physical_ApplyCollisionResponse, 0x0046B9D0, CVehicle* v, ColPoint* colPoint, float* impulse,
        float elasticity, float factor, void* arg);
GAME_FN(void, __thiscall, Placeable_SetMatrix, 0x0046DE90, CVehicle* v, const void* m, int forceMatrix);
GAME_FN(void, __cdecl, World_ClearScanCodes, 0x0045D840);
GAME_FN(uint8_t, __thiscall, Physical_ApplyPairCollisionResponse, 0x0046BC60, CVehicle* v, CEntity* other,
        ColPoint* colPoint, float* impulseA, float* impulseB, int arg, float factorA, float factorB);
GAME_FN(CVector*, __thiscall, Entity_GetPosition, 0x00415460, CVehicle* v);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const CVector* a, const CVector* b);
GAME_FN(void, __thiscall, Vehicle_InflictDamage, 0x004CC4A0, CEntity* v, CEntity* attacker, int weaponType,
        float damage, float x, float y, float z, ColPoint* colPoint, int notifyDriver);
GAME_FN(int32_t, __thiscall, Vehicle_GetHandlingKind, 0x004CAEE0, CVehicle* v);
GAME_FN(float, __cdecl, FixedToFloat, 0x004CB590, int16_t value);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(float*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(CVector*, __cdecl, VectorSub2, 0x00414BA0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const CVector* v);
GAME_FN(CVector*, __thiscall, Vector_ScaleAssign, 0x00413A20, CVector* self, CVector* out, float s);
GAME_FN(void, __thiscall, Vector_Normalise2D, 0x00443730, CVector* v);
GAME_FN(float, __cdecl, MinF, 0x00414470, float a, float b);
GAME_FN(CVector*, __cdecl, ScaleVector, 0x00412320, CVector* out, float s, const CVector* v);
GAME_FN(CVector*, __cdecl, MultiplyMatrixVector, 0x0045D4A0, CVector* out, const void* m, const CVector* v);
GAME_FN(CVector*, __thiscall, Vector_AddAssign, 0x00414670, CVector* self, CVector* out, const CVector* v);
GAME_FN(void, __cdecl, Coronas_Register, 0x005124D0, uintptr_t id, int r, int g, int b, int a, const CVector* pos,
        float size, float range, int type, int a10, int a11, int a12, float a13, int a14, int a15, float a16);

inline void* const userInput = reinterpret_cast<void*>(0x00A45A30);
GAME_VAR(uint8_t, altSteering, 0x00A14925);
GAME_VAR(float, timeStep, 0x00C1A9A4);
GAME_VAR(float, wheelFriction, 0x00C2DED4);           // (the handling manager's rolling resistance)
GAME_VAR(float, burstTyreMod, 0x00A45288);
GAME_VAR(float, burstSpeedMax, 0x00A4528C);
GAME_VAR(uint8_t, sAlreadySkidding, 0x00C34E18);      // ProcessWheel's statics (never reset)
GAME_VAR(uint8_t, sDriving, 0x00C34E19);
GAME_VAR(uint8_t, sBraking, 0x00C34E1A);
GAME_VAR(void*, radar, 0x00C674CC);
GAME_VAR(void*, stimuli, 0x00C2C1A0);
GAME_VAR(CEntity*, playerPed, 0x00C1AEA8);
GAME_VAR(uint8_t*, sweepSphere, 0x00CF3140);          // a CColModel: the 0.2 sphere swept for damage
GAME_VAR(uint16_t, currentScanCode, 0x00C1AE88);
GAME_VAR(CVector, vecUnset, 0x00BCC564);
GAME_VAR(void*, rwCamera, 0x00C3CCF4);
GAME_VAR(float, sunGlare, 0x00CE7550);                // (CWeather::SunGlare)
GAME_VAR(CVector, sunDirection, 0x020C4884);          // (CTimeCycle's)
GAME_ARRAY(uint8_t, sunCoreColour, 0x020C4801);       // r, g, b
GAME_VAR(uint8_t, chapter, 0x00A147E0);
GAME_ARRAY(int32_t, chapterSeason, 0x00A147EC);       // (2: winter)
GAME_ARRAY(uint8_t*, modelInfoPtrs, 0x00C67738);

using re::At;

template <typename R, typename... A>
R CallVirtual(void* object, int slot, A... args) {
    re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object);
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>(vtable[slot]))(object, args...);
}

CVector& Position(CVehicle* v) {
    return v->matrix ? *reinterpret_cast<CVector*>(&v->matrix->m[12]) : v->placementPos;
}

uint8_t* ColModelOf(CVehicle* v) {
    return *reinterpret_cast<re::Ptr32<uint8_t>*>(modelInfoPtrs[v->modelIndex] + 0xC);
}

float Dot(const CVector* a, const CVector* b) {
    return float(double(a->x) * b->x + double(a->y) * b->y + double(a->z) * b->z);
}

// (inlined everywhere: a registered reference let go)
void ReleaseRef(re::Ptr32<CEntity>& ref) {
    if (ref) {
        CleanUpOldReference(ref, &ref);
    }
}

void InitColPoint(ColPoint& cp) {
    cp.point = {0.0f, 0.0f, 0.0f};
    cp.entity = nullptr;
    cp.normal = {0.0f, 0.0f, 0.0f};
    cp.depth = 3.4028235e+38f;
}

// the ped's damage virtual (vf15), as InflictDamage calls it
void PedDamage(CPed* ped, float amount, CEntity* attacker, int weaponType, void* colPoint, void* pos) {
    using Fn = void(__thiscall*)(CPed*, float, float, CEntity*, int, void*, void*, int, int, int, int, float, float, int,
                                 int, int, int, float, int);
    Fn fn = re::PtrCast<Fn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(ped))[15]);
    fn(ped, amount, 0.0f, attacker, weaponType, colPoint, pos, 0, 0, 0, 0, 0.0f, 0.0f, 0, 1, 0, -1, 0.0f, 0);
}

} // namespace

namespace c11vehmisc {

float __fastcall GetSteerInput(CVehicle*, re::Edx, int primary) {
    float result = 0.0f;
    if (!UserInput_IsEnabled(userInput)) {
        return result;
    }
    float value;
    if (!uint8_t(primary)) {
        value = -Pad_GetValue(0x11, 0);
    } else if (altSteering && !Input_738500(GetPlayerPadInput())) {
        double first = -Pad_GetValue(0x10, 0);
        value = float(first - Pad_GetValue(0x12, 0));
    } else {
        value = -Pad_GetValue(0x10, 0);
    }
    if (!(0.0f < std::fabs(value))) {
        value = 0.0f;
    }
    return value;
}

void __fastcall ProcessWheel(CVehicle* self, re::Edx, CVector* wheelFwd, CVector* wheelRight, CVector* contactSpeed,
                             CVector* contactPoint, int wheelsOnGround, float thrust, float brake, float adhesion,
                             int wheelId, int, int32_t* wheelState, int wheelStatus) {
    uint8_t* handling = self->handling;
    float right = 0.0f;
    float fwd = 0.0f;
    float fwdSpeed = Dot(wheelFwd, contactSpeed);
    bool driving;
    if (brake != 0.0f) {
        thrust = 0.0f;
        sBraking = 1;
        driving = false;
    } else {
        sBraking = 0;
        driving = thrust != 0.0f;
    }
    if (*wheelState != 0) {
        sAlreadySkidding = 1;
    }
    sDriving = driving ? 1 : 0;
    *wheelState = 0;
    adhesion = timeStep * adhesion;
    if (sAlreadySkidding) {
        adhesion = At<float>(handling, 0xA0) * adhesion;   // traction loss
    }

    float rightSpeed = Dot(wheelRight, contactSpeed);
    if (rightSpeed != 0.0f) {
        right = float(-double(rightSpeed) / wheelsOnGround);
        if (int16_t(wheelStatus) == 1) {
            // a burst tyre pulls at random
            float speed = fwdSpeed < burstSpeedMax ? fwdSpeed : burstSpeedMax;
            float lo = float(burstTyreMod * -1.0);
            float hi = burstTyreMod;
            float random = float(Rand() / 32767.0 * (double(hi) - lo) + lo);
            right = float(double(random) * speed + right);
        }
    }

    if (driving) {
        fwd = thrust;
        if (right > 0.0f) {
            if (right > adhesion) {
                right = adhesion;
            }
        } else {
            float limit = -adhesion;
            if (limit > right) {
                right = limit;
            }
        }
    } else if (fwdSpeed != 0.0f) {
        fwd = float(-double(fwdSpeed) / wheelsOnGround);
        if (!sBraking && At<float>(self, 0x294) < 0.01f) {   // no gas: rolling resistance
            float mass = At<float>(handling, 0x4);
            if (self->vehicleType == 1) {
                brake = float(wheelFriction * 0.6000000238418579 / (mass + 200.0));
            } else {
                double friction = wheelFriction;
                if (mass < 500.0) {
                    friction = friction * 0.20000000298023224;
                }
                brake = float(friction / mass);
            }
        }
        if (!(brake <= adhesion)) {
            if (std::fabs(fwdSpeed) > 0.005f) {
                *wheelState = 3;   // fixed
            }
        } else if (fwd > 0.0f) {
            if (fwd > brake) {
                fwd = brake;
            }
        } else {
            float limit = -brake;
            if (limit > fwd) {
                fwd = limit;
            }
        }
    }

    float adhesionSq = float(double(adhesion) * adhesion);
    float fwdSq = float(double(fwd) * fwd);
    float rightSq = float(double(right) * right);
    float speedSq = fwdSq + rightSq;
    float factor = 1.0f;
    if (0.0f < self->unknown470) {
        double scaled = (uint8_t(wheelId) == 0 || uint8_t(wheelId) == 2) ? self->unknown470 * 0.029999999329447746
                                                                          : self->unknown470 * 0.05999999865889549;
        factor = float(scaled * 0.75 + 0.25);
    }
    if (adhesionSq < speedSq) {
        if (*wheelState != 3) {
            *wheelState = (driving && fwdSpeed < 0.20000000298023224) ? 1 : 2;   // spinning / skidding
        }
        float tractionLoss = sAlreadySkidding ? 1.0f : At<float>(handling, 0xA0);
        float length = float(std::sqrt(double(std::fabs(speedSq))));
        float scale = float(double(adhesion) / length * tractionLoss);
        fwd = scale * fwd;
        right = scale * right;
        if (factor < 1.0f) {
            right = factor * right;
        }
    } else if (factor < 1.0f) {
        if (!sAlreadySkidding) {
            factor = At<float>(handling, 0xA0) * factor;
        }
        double limitSq = double(adhesionSq) * factor * factor;
        if (speedSq > limitSq) {
            double limit = double(factor) * adhesion;
            float length = float(std::sqrt(double(std::fabs(speedSq))));
            float scale = float(limit / length);
            right = scale * right;
        }
    }

    if (fwd == 0.0f && right == 0.0f) {
        return;
    }

    CVector fwdPart = {wheelFwd->x * fwd, fwd * wheelFwd->y, fwd * wheelFwd->z};
    CVector rightPart = {wheelRight->x * right, right * wheelRight->y, right * wheelRight->z};
    CVector total;
    CVector* sum = VectorAdd(&total, &rightPart, &fwdPart);
    total = *sum;
    CVector turn = total;
    bool separateTurn = false;
    float antiDive = At<float>(handling, 0xC0);
    if (antiDive > 9.999999747378752e-05) {
        float k = 0.0f;
        if (sBraking) {
            k = antiDive;
            separateTurn = true;
        } else if (sDriving) {
            k = float(antiDive * 0.5);
            separateTurn = true;
        }
        if (separateTurn) {
            CVector c = {wheelFwd->x * k, k * wheelFwd->y, k * wheelFwd->z};
            CVector d = {c.x * fwd, c.y * fwd, fwd * c.z};
            turn = {total.x - d.x, total.y - d.y, total.z - d.z};
        }
    }

    auto magnitude = [](const CVector& v) {
        float sq = float((double(v.x) * v.x + double(v.y) * v.y) + double(v.z) * v.z);
        return float(std::sqrt(double(std::fabs(sq))));
    };
    float speed = magnitude(total);
    float turnSpeed = separateTurn ? magnitude(turn) : speed;
    CVector dir = total;
    if (!(speed <= 0.0f)) {
        float inv = float(1.0 / speed);
        dir = {inv * total.x, total.y * inv, inv * total.z};
    } else {
        dir.x = 1.0f;
    }
    if (!separateTurn) {
        turn = dir;
    } else if (!(turnSpeed <= 0.0f)) {
        float inv = float(1.0 / turnSpeed);
        turn = {inv * turn.x, turn.y * inv, inv * turn.z};
    } else {
        turn.x = 1.0f;
    }

    float mass = At<float>(self, 0x158);
    CVector arm;
    CVector* c = CrossProduct(&arm, contactPoint, &turn);
    float impulse = mass * speed;
    float armSq = float((double(c->x) * c->x + double(c->y) * c->y) + double(c->z) * c->z);
    float effectiveMass = float(1.0 / (armSq / At<float>(self, 0x15C) + 1.0 / At<float>(self, 0x158)));
    float turnImpulse = effectiveMass * turnSpeed;
    CVector moveForce = {impulse * dir.x, dir.y * impulse, impulse * dir.z};
    CVector turnForce = {turnImpulse * turn.x, turn.y * turnImpulse, turnImpulse * turn.z};
    Physical_ApplyMoveForce(self, moveForce.x, moveForce.y, moveForce.z);
    Physical_ApplyTurnForce(self, turnForce.x, turnForce.y, turnForce.z, contactPoint->x, contactPoint->y,
                            contactPoint->z);
}

void __fastcall UpdateCollisionLoadedFlag(CVehicle* self, re::Edx) {
    switch (self->createdBy) {
    case 1:
    case 3:
        break;
    default: {
        CVector& pos = Position(self);
        float xy[2] = {pos.x, pos.y};
        if (!IsCollisionLoaded(xy)) {
            self->flags33C |= 4;
            return;
        }
        break;
    }
    }
    self->flags33C &= ~4;
}

void __fastcall AddRadarBlip(CVehicle* self, re::Edx) {
    int handle = GetVehicleRef(self);
    if (handle >= 0) {
        Radar_AddBlipFor(radar, 0xC, handle, 0x01000000, 1, 0x10, 1);
    }
}

void __fastcall InflictDamage(CVehicle* self, re::Edx, CEntity* attacker, int weaponType, float damage, float x, float y,
                              float z, ColPoint* colPoint, int notifyDriver) {
    if (!(self->flags33A & 0x40)) {
        return;
    }
    if ((self->typeAndStatus & 0xF8) && self->unknown70 && attacker != playerPed &&
        attacker != reinterpret_cast<CEntity*>(FindPlayerVehicle(0))) {
        return;
    }
    // (immune to some weapons)
    if ((weaponType == -5 && self->unknown68) || (weaponType == -4 && self->unknown44) ||
        (weaponType == -1 && self->unknown6C)) {
        return;
    }
    if (attacker && (At<uint8_t>(attacker, 0x108) & 7) == 3 && (self->vehicleType == 0 || self->vehicleType == 1)) {
        Stimulus_Add(stimuli, 0x1B, attacker, reinterpret_cast<CEntity*>(self), nullptr, -1.0f, -1.0f);
    }
    if (!(0.0f < self->health)) {
        return;
    }

    if (colPoint) {
        float impulse = float(damage * 5.0);
        Automobile_SetDamage(self, uint16_t(colPoint->piece), impulse, attacker, &colPoint->normal);
        if (self->vehicleType == 0) {
            Automobile_DamageAt(self, colPoint, impulse);
        }
        if (!self->unknown44C && self->vehicleType == 0 && attacker == playerPed && !Fn_42E430()) {
            self->unknown44C = 1;
            IncrementStat(0x18, 1);
        }
        if (uint8_t(notifyDriver) && Seats_GetOccupant(&self->seats, 0)) {
            float amount = 0.0f;
            int32_t kind;
            if (CallVirtual<uint8_t>(self, 4) ||
                (CallVirtual<uint8_t>(self, 5) &&
                 (*Vehicle_GetVehicleClass(self, &kind) == 6 || *Vehicle_GetVehicleClass(self, &kind) == 7))) {
                amount = damage;
            }
            PedDamage(Seats_GetOccupant(&self->seats, 0), amount, attacker, weaponType, colPoint, &colPoint->normal);
        }
    } else {
        float lengthSq = float((double(y) * y + double(x) * x) + double(z) * z);
        if (lengthSq > 0.01f) {
            // the hit found by sweeping a small sphere from 3 m out along the shot's
            // direction to the vehicle's position
            ColPoint cp;
            ColPoint_Construct(&cp);
            col::CMatrix from;
            from.attached = nullptr;
            Matrix_SetUnity(&from);
            CVector& pos = Position(self);
            CVector here = {pos.x, pos.y, pos.z};
            CVector dir = {x - here.x, y - here.y, z - here.z};
            Vector_Normalise2(&dir);
            CVector out = {float(dir.x * 3.0), float(dir.y * 3.0), float(dir.z * 3.0)};
            dir = {out.x + x, out.y + y, out.z + z};
            Matrix_SetTranslate(&from, &dir);
            col::CMatrix to;
            to.attached = nullptr;
            Matrix_SetUnity(&to);
            Matrix_SetTranslate(&to, &here);
            float* matrix = Entity_GetMatrix(self);
            uint8_t* colModel = ColModelOf(self);
            At<float>(sweepSphere, 0xC) = 0.2f;
            // (the col data's first sphere: [[sphere + 0x38]] + 0xC, its radius)
            At<float>(*At<re::Ptr32<uint8_t>*>(sweepSphere, 0x38), 0xC) = 0.2f;
            CVector boxMax = {0.2f, 0.2f, 0.2f};
            CVector boxMin = {-0.2f, -0.2f, -0.2f};
            Box_Set(sweepSphere + 0x10, &boxMin, &boxMax);
            if (Col_ProcessSweep(&from, &to, sweepSphere, matrix, matrix, colModel, &cp, 0, 0, 1, 0, 0, 0xF, 0, 1)) {
                float impulse = float(damage * 5.0);
                Automobile_SetDamage(self, uint16_t(cp.piece), impulse, attacker, &cp.normal);
                if (self->vehicleType == 0) {
                    Automobile_DamageAt(self, &cp, impulse);
                }
                if (!self->unknown44C && self->vehicleType == 0 && attacker == playerPed && !Fn_42E430()) {
                    self->unknown44C = 1;
                    IncrementStat(0x18, 1);
                }
            }
            if (uint8_t(notifyDriver) && Seats_GetOccupant(&self->seats, 0)) {
                PedDamage(Seats_GetOccupant(&self->seats, 0), 0.0f, attacker, weaponType, &cp, &cp.normal);
            }
            ColPoint_Destruct(&cp);
        }
    }
    // (lethal damage leaves the health as it is)
    if (damage < self->health) {
        self->health = self->health - damage;
    }
}

uint8_t __fastcall IsPathAheadClear(CVehicle* self, re::Edx) {
    CColModel* col = reinterpret_cast<CColModel*>(ColModelOf(self));
    float length = col->boxMax.y - col->boxMin.y;
    CVector* pos = &Position(self);
    CVector fwd;
    Entity_GetForward(self, &fwd);
    float distance = float(length * 0.5 + 3.0);
    CVector ahead = {distance * fwd.x, fwd.y * distance, fwd.z * distance};
    ColPoint cp;
    InitColPoint(cp);
    re::Ptr32<CEntity> hit = nullptr;
    CVector end;
    uint8_t found = ProcessLineOfSight(VectorAdd(&end, pos, &ahead), pos, &cp, &hit, 0xC3, 0, 0);
    uint8_t clear = (found && hit != reinterpret_cast<CEntity*>(self)) ? 0 : 1;
    ReleaseRef(cp.entity);
    return clear;
}

CVector* __fastcall GetDriverMoveDirection(CVehicle* self, re::Edx, CVector* out) {
    if (!Seats_GetOccupant(&self->seats, 0)) {
        *out = vecUnset;
        return out;
    }
    CPed* driver = Seats_GetOccupant(&self->seats, 0);
    CVector* move = At<CVector*>(driver, 0x2E0);
    CVector dir = {move->x, move->y, 0.0f};
    if (dir.x != vecUnset.x || dir.y != vecUnset.y || vecUnset.z != 0.0) {
        Vector_Normalise(&dir);
        col::CMatrix inverse;
        inverse.attached = nullptr;
        Matrix_Invert(Entity_GetMatrix(self), &inverse);
        CVector local;
        CVector* result = Multiply3x3(&local, &inverse, &dir);
        dir = {result->x, result->y, 0.0f};
        Vector_Normalise(&dir);
    }
    *out = dir;
    return out;
}

int32_t __fastcall ResolveCollisions(CVehicle* self, re::Edx, float* savedMatrix, float* dz, int, ColPoint* outColPoint,
                                     uint32_t keepFlags, int arg6) {
    ColPoint found;
    InitColPoint(found);
    ColPoint fallback;
    InitColPoint(fallback);
    if (!outColPoint) {
        outColPoint = &fallback;
    }
    Matrix_CopyOnlyMatrix(savedMatrix, Entity_GetMatrix(self));
    self->unknown1BD &= ~0x10;
    bool anyHit = false;
    CVector& pos = Position(self);
    pos.z = *dz + pos.z;
    CVehicle* bike = self->vehicleType == 1 ? self : nullptr;
    uint32_t round = 0;
    for (; round < 4; round++) {
        if (currentScanCode < 0xFFFF) {
            currentScanCode++;
        } else {
            World_ClearScanCodes();
            currentScanCode = 1;
        }
        bool hit = false;
        outColPoint->depth = 3.4028235e+38f;
        for (uint8_t* node = static_cast<uint8_t*>(self->collisionList); node; node = At<uint8_t*>(node, 0x10)) {
            if (Physical_ProcessCollisionWith(self, savedMatrix, At<CEntity*>(node, 8), &found, 1, arg6)) {
                hit = true;
                if (found.depth < outColPoint->depth) {
                    ColPoint_Assign(outColPoint, &found);
                }
            }
        }
        if (!hit) {
            break;
        }
        if (outColPoint->entity && outColPoint->entity == self->unknown450) {
            self->unknown454++;
        }
        anyHit = true;
        if (round == 0) {
            CVector turnSpeed = self->turnSpeed;
            CVector moveSpeed = self->moveSpeed;
            int32_t kind;
            float elasticity;
            if (*Vehicle_GetVehicleClass(self, &kind) == 7 || *Vehicle_GetVehicleClass(self, &kind) == 6) {
                elasticity = 0.05f;
            } else if (!bike) {
                elasticity = 1.0f;
            } else {
                CVector up;
                if (At<uint8_t>(bike, 0x780) || 0.02f > Entity_GetUp(self, &up)->z) {
                    elasticity = 2.5f;
                } else {
                    elasticity = 0.015f;
                }
            }
            float impulse;
            Physical_ApplyCollisionResponse(self, outColPoint, &impulse, elasticity, 1.0f, nullptr);
            if (uint8_t(keepFlags) & 2) {
                self->turnSpeed.x = turnSpeed.x;
                self->turnSpeed.y = turnSpeed.y;
            }
            if (uint8_t(keepFlags) & 4) {
                self->moveSpeed.z = moveSpeed.z;
            }
        }
    }
    int32_t result;
    if (round == 4) {
        Placeable_SetMatrix(self, savedMatrix, 0);
        result = 2;
    } else {
        result = anyHit ? 1 : 0;
    }
    ReleaseRef(found.entity);
    ReleaseRef(fallback.entity);
    return result;
}

void __fastcall DoSunGlare(CVehicle* self, re::Edx) {
    if (At<int32_t>(self, 0x50) != 0) {   // (scorched)
        return;
    }
    if (Entity_GetMatrix(self)[10] < 0.0f) {   // upside down
        return;
    }
    if (Vehicle_GetHandlingKind(self) != 1 || !(0.0f < sunGlare)) {
        return;
    }
    CVector* pos = &Position(self);
    CVector sub;
    CVector* d = VectorSub2(&sub, reinterpret_cast<CVector*>(GetLocalTransform(GetCameraNode(rwCamera)) + 9), pos);
    CVector camDir = {d->x, d->y, d->z};
    float dist = Vector_Magnitude(&camDir);
    CVector scaled;
    Vector_ScaleAssign(&camDir, &scaled, float(2.0 / dist));
    CVector sun = sunDirection;
    CVector dir2 = {camDir.x + sun.x, camDir.y + sun.y, camDir.z + sun.z};
    CVector tmp;
    CVector* fwd = Entity_GetForward(self, &tmp);
    float fwdness = float((double(fwd->y) * dir2.y + double(fwd->x) * dir2.x) + double(fwd->z) * dir2.z);
    float* right = Entity_GetMatrix(self);
    float sideness = float((double(right[1]) * dir2.y + double(right[0]) * dir2.x) + double(right[2]) * dir2.z);
    CVector localDir = {sideness, fwdness, 0.0f};
    Vector_Normalise2(&localDir);

    CVector fwd2D = {Entity_GetForward(self, &tmp)->x, 0.0f, 0.0f};
    fwd2D.y = Entity_GetForward(self, &tmp)->y;
    CVector cam2D = {camDir.x, camDir.y, 0.0f};
    Vector_Normalise2D(&fwd2D);
    Vector_Normalise2D(&cam2D);
    float fwdness2 = std::fabs(float(double(cam2D.y) * fwd2D.y + double(cam2D.x) * fwd2D.x));
    float strength;
    if (fwdness2 > 0.9950000047683716) {
        strength = 1.0f;
    } else if (0.9900000095367432 < fwdness2) {
        strength = float((fwdness2 - 0.9900000095367432) * 200.0);
    } else {
        return;
    }
    if (dist <= 30.0) {
        if (!(13.0 < dist)) {
            return;
        }
        strength = float((dist - 13.0) / 17.0 * strength);
    }
    strength = float(strength * 0.800000011920929);
    double intensity = double(strength) * sunGlare;
    // (the colours go out as bytes in otherwise stale words; ours are clean)
    uint8_t r = uint8_t(int((sunCoreColour[0] + 0x1FE) * intensity / 3.0));
    uint8_t g = uint8_t(int((sunCoreColour[1] + 0x1FE) * intensity / 3.0));
    uint8_t b = uint8_t(int((sunCoreColour[2] + 0x1FE) * intensity / 3.0));

    CCollisionData* data = reinterpret_cast<CColModel*>(ColModelOf(self))->data;
    if (!data || data->numTriangles - 2 < 0) {
        return;
    }
    const int16_t* vertices = re::PtrCast<const int16_t*>(data->vertices);
    const uint16_t* triangles = re::PtrCast<const uint16_t*>(data->triangles);
    auto vertex = [&](uint32_t index) {
        const int16_t* v = vertices + index * 3;
        return CVector{float(v[0] * 0.0078125), float(v[1] * 0.0078125), float(v[2] * 0.0078125)};
    };
    for (int i = 0; i <= data->numTriangles - 2; i += 2) {
        const uint16_t* tri = triangles + i * 4;   // 8 bytes a triangle: a, b, c, ...
        uint32_t a = tri[0];
        CVector pa = vertex(a);
        if (!(0.0f < pa.z)) {
            continue;
        }
        uint32_t bIndex = tri[1];
        uint32_t cIndex = tri[2];
        CVector pb = vertex(bIndex);
        CVector pc = vertex(cIndex);
        // the next triangle's corner that isn't one of these three: the quad's fourth
        int found = 0;
        CVector pd = {0.0f, 0.0f, 0.0f};
        for (int k = 0; k < 3; k++) {
            uint32_t other = tri[4 + k];
            if (other != a && other != bIndex && other != cIndex) {
                const uint16_t* v = reinterpret_cast<const uint16_t*>(vertices + other * 3);
                pd.x = FixedToFloat(int16_t(v[0]));
                pd.y = FixedToFloat(int16_t(v[1]));
                pd.z = FixedToFloat(int16_t(v[2]));
                found++;
            }
        }
        if (found != 1) {
            continue;
        }
        CVector s1 = {pb.x + pa.x, pb.y + pa.y, pb.z + pa.z};
        CVector s2 = {s1.x + pc.x, s1.y + pc.y, s1.z + pc.z};
        CVector s3 = {pd.x + s2.x, pd.y + s2.y, pd.z + s2.z};
        CVector mid = {float(s3.x * 0.25), float(s3.y * 0.25), float(s3.z * 0.25)};
        float dy = std::fabs(mid.y - pa.y);
        float dx = std::fabs(mid.x - pa.x);
        float size = float(MinF(dx, dy) * 1.399999976158142);
        if (!(size > 0.6f)) {
            continue;
        }
        CVector offset, glarePos, world;
        CVector* sum = VectorAdd(&glarePos, &mid, ScaleVector(&offset, size, &localDir));
        CVector* w = MultiplyMatrixVector(&world, Entity_GetMatrix(self), sum);
        CVector coronaPos = *w;
        CVector out;
        Vector_AddAssign(&coronaPos, &out, &camDir);
        Coronas_Register(reinterpret_cast<uintptr_t>(self) + 0x1B + i, r, g, b, 0xFF, &coronaPos,
                         float(sunGlare * 0.8999999761581421), 90.0f, 0, 0, 0, 0, 0.0f, 0, 0, 1.5f);
    }
}

int32_t __fastcall ResolveCollisionsAtHeight(CVehicle* self, re::Edx, float* savedMatrix, float dz, float* outDz,
                                             ColPoint* outColPoint, uint32_t keepFlags, int arg6, CVector* outHeight) {
    *outDz = 0.0f;
    float baseZ = savedMatrix[14];
    CVector& pos = Position(self);
    *outDz = float((double(pos.z) - baseZ) - dz);
    Position(self).z = float(double(baseZ) + dz);
    bool anyHit = false;
    ColPoint best;
    InitColPoint(best);
    ColPoint found;
    InitColPoint(found);
    CVehicle* bike = self->vehicleType == 1 ? self : nullptr;
    uint32_t round = 0;
    for (; round < 4; round++) {
        At<int32_t>(self, 0x1C0) = 0;
        if (currentScanCode < 0xFFFF) {
            currentScanCode++;
        } else {
            World_ClearScanCodes();
            currentScanCode = 1;
        }
        bool hit = false;
        best.depth = 3.4028235e+38f;
        for (uint8_t* node = static_cast<uint8_t*>(self->collisionList); node; node = At<uint8_t*>(node, 0x10)) {
            if (Physical_ProcessCollisionWith(self, savedMatrix, At<CEntity*>(node, 8), &found, 0, arg6) &&
                found.depth < best.depth) {
                ColPoint_Assign(&best, &found);
                hit = true;
                anyHit = true;
            }
        }
        if (!hit) {
            break;
        }
        if (outColPoint) {
            ColPoint_Assign(outColPoint, &best);
        }
        CEntity* other = best.entity;
        if (other && other == self->unknown450) {
            self->unknown454++;
        }
        if (round != 0) {
            continue;
        }
        CVector turnSpeed = self->turnSpeed;
        CVector moveSpeed = self->moveSpeed;
        int32_t kind;
        bool respond = *Vehicle_GetVehicleClass(self, &kind) != 3 && *Vehicle_GetVehicleClass(self, &kind) != 10;
        if (respond && other) {
            // (not against something heavier by its vf32, nor a ped)
            if (CallVirtual<uint8_t>(other, 31)) {
                double otherValue = CallVirtual<float>(other, 32);
                if (CallVirtual<float>(self, 57) > otherValue) {
                    respond = false;
                }
            }
            if (respond && (At<uint8_t>(other, 0x108) & 7) == 3) {
                respond = false;
            }
        }
        if (respond) {
            float impulse = 0.0f;   // (the responses write it)
            float impulseB;
            bool pair = false;
            if (CallVirtual<uint8_t>(self, 5)) {
                if (other && CallVirtual<uint8_t>(other, 5) && *Vehicle_GetVehicleClass(self, &kind) == 7 &&
                    *Vehicle_GetVehicleClass(reinterpret_cast<CVehicle*>(other), &kind) == 7) {
                    Physical_ApplyPairCollisionResponse(self, other, &best, &impulse, &impulseB, 1, 1.0f, 0.7f);
                    pair = true;
                }
            }
            if (!pair && other) {
                uint8_t type = At<uint8_t>(other, 0x108) & 7;
                uint8_t status = At<uint8_t>(other, 0x108) & 0xF8;
                if ((type == 2 && status != 0x10 &&
                     *Vehicle_GetVehicleClass(reinterpret_cast<CVehicle*>(other), &kind) != 3) ||
                    (type == 3 && self->vehicleType == 0)) {
                    Physical_ApplyPairCollisionResponse(self, other, &best, &impulse, &impulseB, 1, 1.0f, 1.0f);
                    pair = true;
                }
            }
            if (!pair) {
                float elasticity = 1.0f;
                float factor = 1.0f;
                if (!bike) {
                    if (*Vehicle_GetVehicleClass(self, &kind) == 7) {
                        elasticity = 0.1f;
                        factor = 3.0f;
                    }
                    if (outHeight) {
                        double hitZ = best.point.z;
                        outHeight->z = float(hitZ - Entity_GetPosition(self)->z);
                    }
                } else {
                    CVector up;
                    if (!At<uint8_t>(bike, 0x780) && !(0.02f > Entity_GetUp(self, &up)->z)) {
                        // a bike upright: bouncier the more head-on
                        CVector f;
                        float along = std::fabs(DotProduct(Entity_GetForward(self, &f), &best.normal));
                        elasticity = float(0.11999999731779099 - (1.0 - along) * 0.05000000074505806);
                        along = std::fabs(DotProduct(Entity_GetForward(self, &f), &best.normal));
                        factor = float((1.0 - along) * 9.0 + 4.0);
                        if (chapterSeason[chapter] == 2) {
                            factor = float(factor * 0.6000000238418579);
                        }
                        if (outHeight) {
                            double hitZ = best.point.z;
                            outHeight->z = float(hitZ - Entity_GetPosition(self)->z);
                        }
                    }
                }
                Physical_ApplyCollisionResponse(self, &best, &impulse, elasticity, factor,
                                                outHeight);
            }
            if (bike && Seats_GetOccupant(&bike->seats, 0) == re::PtrCast<CPed*>(playerPed) && other &&
                CallVirtual<uint8_t>(other, 5)) {
                // the player's bike hitting a vehicle damages it
                CVector unset = vecUnset;
                Vehicle_InflictDamage(other, reinterpret_cast<CEntity*>(self), -1, float(impulse / 9.0),
                                      unset.x, unset.y, unset.z, &best, 0);
            }
        }
        if (uint8_t(keepFlags) & 2) {
            self->turnSpeed.x = turnSpeed.x;
            self->turnSpeed.y = turnSpeed.y;
        }
        if (uint8_t(keepFlags) & 4) {
            self->moveSpeed.z = moveSpeed.z;
        }
    }
    int32_t result;
    if (round == 4) {
        Placeable_SetMatrix(self, savedMatrix, 0);
        // (no check for an entity: after four rounds of hits there is one)
        if ((At<uint8_t>(best.entity, 0x108) & 7) == 3 && CallVirtual<uint8_t>(self, 4)) {
            float impulse;
            Physical_ApplyCollisionResponse(self, &best, &impulse, best.depth, 1.0f, nullptr);
            Physical_ApplyMoveForce(self, 0.0f, 0.0f, 5.0f);
        }
        result = 2;
    } else {
        result = anyHit ? 1 : 0;
    }
    ReleaseRef(found.entity);
    ReleaseRef(best.entity);
    return result;
}

} // namespace c11vehmisc

// ---- registration ---------------------------------------------------------------------

#define VEHMISC_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

VEHMISC_REVERSED("CVehicle::GetSteerInput", 0x004CB350, &c11vehmisc::GetSteerInput);
VEHMISC_REVERSED("CVehicle::ProcessWheel", 0x004CB8D0, &c11vehmisc::ProcessWheel);
VEHMISC_REVERSED("CVehicle::UpdateCollisionLoadedFlag", 0x004CC350, &c11vehmisc::UpdateCollisionLoadedFlag);
VEHMISC_REVERSED("CVehicle::AddRadarBlip", 0x004CC3D0, &c11vehmisc::AddRadarBlip);
VEHMISC_REVERSED("CVehicle::InflictDamage", 0x004CC4A0, &c11vehmisc::InflictDamage);
VEHMISC_REVERSED("CVehicle::IsPathAheadClear", 0x004CCA10, &c11vehmisc::IsPathAheadClear);
VEHMISC_REVERSED("CVehicle::GetDriverMoveDirection", 0x004CCC00, &c11vehmisc::GetDriverMoveDirection);
VEHMISC_REVERSED("CVehicle::ResolveCollisions", 0x004CCD30, &c11vehmisc::ResolveCollisions);
VEHMISC_REVERSED("CVehicle::DoSunGlare", 0x004CD2A0, &c11vehmisc::DoSunGlare);
VEHMISC_REVERSED("CVehicle::ResolveCollisionsAtHeight", 0x004CDC30, &c11vehmisc::ResolveCollisionsAtHeight);
