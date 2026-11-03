// CBike's physics: a wheel's forces (GTA's ProcessBikeWheel), the rider falling off,
// the wheelie and lean, the collision and damage handling, its sounds while riding.
// Fields are reached by offset: CBike's layout (Vehicle.h) only has a few named.

#include "game/Vehicle.h"

#include <cmath>

#include "game/Crt.h"
#include "reversed.h"
namespace {

using veh::CVector;

using re::At;

// a virtual by its byte offset in the vtable
template <typename R, typename... A>
R Virtual(void* object, int offset, A... args) {
    re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object);
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>(vtable[offset / 4]))(object, args...);
}

GAME_VAR(float, timeStep, 0x00C1A9A4);        // CTimer's
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);     // CTimer's
GAME_VAR(CPed*, playerPed, 0x00C1AEA8);       // (FindPlayerPed's)
GAME_VAR(CVector, vec0, 0x00BCC564);          // (a zero vector)
GAME_VAR(uint8_t*, bikeTuning, 0x020C8900);   // (the bike tuning values)

inline int __cdecl Rand() { return crt::Rand(); }

GAME_FN(CPed*, __thiscall, Seats_GetOccupant, 0x004CE560, CVehicleSeats* seats, uint32_t seat);
GAME_FN(int32_t*, __thiscall, Vehicle_GetVehicleClass, 0x004CAF30, CVehicle* vehicle, int32_t* out);
GAME_FN(float*, __thiscall, Entity_GetMatrix, 0x008B6120, CVehicle* entity);
GAME_FN(CVector*, __thiscall, Entity_4154A0, 0x004154A0, CVehicle* entity, CVector* out);   // (its forward)
GAME_FN(void, __thiscall, Physical_ApplyMoveForce, 0x0046A7E0, CVehicle* physical, float x, float y, float z);
GAME_FN(void, __thiscall, Physical_ApplyTurnForce, 0x0046A950, CVehicle* physical, float fx, float fy, float fz, float px,
        float py, float pz);

GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, CrossProduct, 0x00414730, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);

// the tuning of the wheels' grip
GAME_VAR(float, wobble, 0x00A3573C);          // (a random sideways push when bumpy)
GAME_VAR(float, wobbleSpeed, 0x00A35740);
GAME_VAR(float, turnForceScale, 0x00A35744);
GAME_VAR(float, rollingResistance, 0x00C2DED4);

GAME_FN(float, __cdecl, RandomFloatInRange, 0x0041A5B0, float lo, float hi);
GAME_FN(float, __thiscall, Vector_MagnitudeSqr, 0x0040D9F0, const CVector* v);
GAME_VAR(uint32_t, prevTimeInMs, 0x00C1A9AC);   // CTimer's (the frame before)

// the pad's rumble
GAME_FN(void, __cdecl, Pad_Rumble, 0x00738680, int pad, int strength, int length, int arg);
GAME_FN(uint8_t, __cdecl, Pad_IsRumbleOff, 0x007387D0);

// the sounds
GAME_VAR(void*, soundManager, 0x00B6F1D4);
GAME_VAR(uint16_t, soundBikeOnBigWet, 0x00A3572C);
GAME_VAR(uint16_t, soundBikeBrake, 0x00A35730);
GAME_VAR(uint16_t, soundBikeBrakeSnow, 0x00A35734);
GAME_VAR(int32_t, currArea, 0x00BD1008);       // CGame's
GAME_VAR(uint8_t, chapter, 0x00A147E0);
GAME_ARRAY(int32_t, chapterAreaKinds, 0x00A147EC);
GAME_FN(uint8_t, __cdecl, Fn_430320, 0x00430320, int area);
GAME_FN(void, __thiscall, SoundManager_Play, 0x00668290, void* manager, uint32_t* id, void* sound, int arg);
GAME_FN(void, __thiscall, Sound_SetPosition, 0x00669040, void* sound, const CVector* pos, int arg);
GAME_FN(void, __thiscall, Sound_SetVolume_x64, 0x00668C40, void* sound, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Sound_SetVolume(void* a0, float a1) {
    return Sound_SetVolume_x64(a0, re::BitsOf(a1));
}
GAME_FN(void, __thiscall, Sound_Stop, 0x00668D00, void* sound, float fade);
// the sound streams (0xCF3DA0)
inline void* const soundStreams = reinterpret_cast<void*>(0x00CF3DA0);
GAME_FN(int, __thiscall, SoundStreams_GetId, 0x00595B80, void* streams, const char* name);
GAME_FN(void, __thiscall, SoundStreams_Play, 0x0059B8A0, void* streams, int id, const CVector* pos, int a3,
        const CVector* pos2, int a5, int a6, int a7, int a8, uintptr_t a9, uintptr_t a10, int a11, int a12, float volume, int a14);

// the game's mode object (0x20C8704: +4 on, +0x3C the mode)
GAME_VAR(uint8_t*, gameMode, 0x020C8704);
GAME_FN(int32_t, __cdecl, GetGameMode, 0x004A84A0);   // (its +0x3C, 0 without it)

GAME_VAR(CVector, vec9E33DC, 0x009E33DC);   // (a direction: gravity's?)
GAME_FN(float, __cdecl, GetSurfaceAdhesion, 0x0045B500, const void* colPoint);   // (by its two surfaces)
// Gamebryo: a frame's local transform (its +0x14), its parent, transforms multiplied
struct NiTransform {
    float rotate[9];
    CVector translate;
    float scale;
};
static_assert(sizeof(NiTransform) == 0x34);
GAME_FN(NiTransform*, __cdecl, Frame_GetTransform, 0x006C9A90, void* frame);
GAME_FN(void*, __cdecl, Frame_GetParent, 0x006C7370, void* frame);
GAME_FN(NiTransform*, __thiscall, NiTransform_Multiply, 0x00406830, const NiTransform* a, NiTransform* out, const NiTransform* b);
GAME_FN(float, __thiscall, VehicleModel_GetWheelScale, 0x005350F0, CVehicleModel* mi);   // (its +0x58 plus 0xA95814)
GAME_ARRAY(CVehicleModel*, modelInfoPtrs, 0x00C67738);   // CModelInfo::ms_modelInfoPtrs

// CMatrix's
GAME_FN(void, __thiscall, Matrix_UpdateMatrix, 0x00412A70, veh::CMatrix* m, const NiTransform* t);
GAME_FN(void, __thiscall, Matrix_SetRotateX, 0x00412CA0, veh::CMatrix* m, float angle);
GAME_FN(void, __thiscall, Matrix_RotateY, 0x00412CC0, veh::CMatrix* m, float angle);
GAME_FN(void, __thiscall, Matrix_RotateZ, 0x00412CE0, veh::CMatrix* m, float angle);
GAME_FN(void, __thiscall, Matrix_UpdateRW, 0x00413A70, veh::CMatrix* m);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const CVector* a, const CVector* b);
GAME_FN(float, __cdecl, MaxFloat, 0x00414490, float a, float b);
GAME_FN(void, __thiscall, Vector_Add, 0x00415200, CVector* v, const CVector* add);   // (+=)
GAME_FN(void, __thiscall, SetRef, 0x005D7F10, void* ref, void* entity);
GAME_FN(void, __thiscall, Seats_SetDriver, 0x004CE600, CVehicleSeats* seats, CPed* ped);
// the events (0xC2C1A0)
GAME_VAR(void*, events, 0x00C2C1A0);
GAME_FN(void, __thiscall, Events_Add, 0x004A6670, void* events, int kind, CPed* ped, CEntity* other, const CVector* pos,
        float a, float b);
GAME_FN(void, __cdecl, Fn_418A50, 0x00418A50, CVehicle* vehicle);
GAME_FN(void, __cdecl, Fn_422A10, 0x00422A10, CVehicle* vehicle);
GAME_FN(void, __thiscall, Physical_ProcessControl, 0x00469B70, CVehicle* physical);   // (CPhysical's vf12)
GAME_FN(void, __thiscall, Vehicle_450970, 0x00450970, CVehicle* vehicle);
GAME_FN(float, __thiscall, Vehicle_CollisionImpulse, 0x004CB870, void* self, int kind, const CVector* a, const CVector* b, float divisor);
GAME_FN(uint8_t, __thiscall, Ped_IsSeatedInVehicle, 0x004CE690, CPed* ped);
GAME_FN(void, __thiscall, Game_SetTrails, 0x004B95C0, CBike* bike, int on);
GAME_FN(void, __thiscall, Game_UpdateSteering, 0x004B9800, CBike* bike);
// (0x004146B0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* Multiply3x3(CVector* out, const float* m, const CVector* v) { return re::CallVec16<0x004146B0, CVector*>(out, m, v); }            // m * v
GAME_FN(CVector*, __cdecl, Multiply3x3Transposed, 0x00414770, CVector* out, const CVector* v, const float* m);  // v * m
GAME_FN(CVector*, __thiscall, Physical_GetUp, 0x00473730, CVehicle* vehicle, CVector* out);
GAME_VAR(CVector, stabiliserDamping, 0x00A35750);   // (per axis: how much of its turn speed it keeps)
GAME_FN(float, __cdecl, GetSurfaceTraction, 0x0045B530, int surface);   // (by one surface; less on snow)
GAME_FN(int32_t, __cdecl, GetSurfaceKind, 0x0045B310, int surface);
GAME_FN(CVector*, __thiscall, Physical_GetSpeed, 0x00468E30, CVehicle* physical, CVector* out, const CVector* point);
GAME_FN(void, __thiscall, Game_ProcessWheel, 0x004BBD50, CBike* bike, const CVector* wheelFwd, const CVector* wheelRight,
        const CVector* contactSpeed, const CVector* contactPoint, uint32_t wheelsOnGround, float thrust, float brake,
        float adhesion, float destabTraction, int wheelId, float* wheelSpeed, int32_t* wheelState, int special,
        int wheelStatus);
GAME_FN(float, __thiscall, Game_GetWheelAdhesion, 0x004BD740, CBike* bike, const void* colPoint);
// this file's own and CBike's others, called where they are
GAME_FN(uint8_t, __thiscall, Game_Bike_4B9E10, 0x004B9E10, CBike* bike, int wheel);

GAME_FN(CPed*, __thiscall, Game_KnockOffRider, 0x004BA050, CBike* bike, int kind, const void* arg, CPed* ped, int unused);
CVehicleSeats* Seats(void* bike) {
    return &static_cast<CVehicle*>(bike)->seats;
}

// 0x4BBD50: GTA's ProcessBikeWheel: a wheel's grip turned into forces (the wheel's
// forward and right, the contact point's speed and position, the wheels on the ground,
// thrust, brake, adhesion, the traction lost, which wheel, its speed (unused), its state
// (in/out: 0 normal, 1 spinning, 2 skidding, 3 locked), special (2, 3: skidding), and
// its status (1: bumpy)). Its +0x72C "already skidding", +0x72D braking, +0x72E
// driving, +0x72F reversing
void __fastcall Bike_ProcessWheel(CBike* self, re::Edx, const CVector* wheelFwd, const CVector* wheelRight,
                                  const CVector* contactSpeed, const CVector* contactPoint, uint32_t wheelsOnGround,
                                  float thrust, float brake, float adhesion, float destabTraction, int wheelId,
                                  float* wheelSpeed, int32_t* wheelState, int special, int wheelStatus) {
    (void)wheelSpeed;
    float fwd = 0.0f;
    float right = 0.0f;
    float fwdSpeed = wheelFwd->x * contactSpeed->x + wheelFwd->y * contactSpeed->y + wheelFwd->z * contactSpeed->z;
    uint8_t& skidding = At<uint8_t>(self, 0x72C);
    uint8_t& braking = At<uint8_t>(self, 0x72D);
    uint8_t& driving = At<uint8_t>(self, 0x72E);
    uint8_t& reversing = At<uint8_t>(self, 0x72F);
    braking = brake != 0.0f;
    if (braking) {
        driving = 0;
        reversing = 0;
    } else {
        driving = thrust != 0.0f;
        reversing = thrust < 0.0f;
    }
    if (*wheelState != 0) {
        skidding = 1;
    }
    *wheelState = 0;
    uint8_t* handling = self->handling;
    adhesion = timeStep * adhesion;
    if (skidding) {
        adhesion = At<float>(handling, 0xA0) * adhesion;   // (the traction loss)
    }
    if (special == 2 || special == 3) {
        *wheelState = 2;
    } else {
        float rightSpeed = wheelRight->y * contactSpeed->y + wheelRight->x * contactSpeed->x + wheelRight->z * contactSpeed->z;
        if (rightSpeed != 0.0f) {
            right = -rightSpeed / float(double(wheelsOnGround));
            if (int16_t(wheelStatus) == 1) {
                float limit = fwdSpeed < wobbleSpeed ? fwdSpeed : wobbleSpeed;
                float lo = float(wobble * -1.0);
                float hi = wobble;
                float r = float(double(Rand()) / 32767.0 * (hi - lo) + lo);
                right = r * limit + right;
            }
        }
    }
    if (driving) {
        fwd = thrust;
        if (right > 0.0f) {
            if (right > adhesion) {
                right = adhesion;
            }
        } else if (-adhesion > right) {
            right = -adhesion;
        }
    } else if (fwdSpeed != 0.0f) {
        fwd = -fwdSpeed / float(double(wheelsOnGround));
        if (!braking) {
            // (rolling resistance while its +0x294 is under 0.01)
            int32_t slowing = 0.01f > At<float>(self, 0x294) ? 1 : 0;
            if (std::fabs(float(slowing)) != 0.0f) {
                brake = float(rollingResistance * 0.014000000432133675 / (At<float>(handling, 4) + 200.0));
                int32_t cls;
                if (*Vehicle_GetVehicleClass(self, &cls) == 3 || *Vehicle_GetVehicleClass(self, &cls) == 8) {
                    brake = float(brake * 100.0);
                }
            }
        }
        if (brake < adhesion || brake == adhesion) {
            if (fwd > 0.0f) {
                if (fwd > brake) {
                    fwd = brake;
                }
            } else if (-brake > fwd) {
                fwd = -brake;
            }
        } else if (std::fabs(fwdSpeed) > 0.005f) {
            *wheelState = 3;
        }
    }
    float adhesionSqr = adhesion * adhesion;
    float speedSqr = fwd * fwd + right * right;
    if (!(adhesionSqr < speedSqr)) {
        if (destabTraction < 1.0f) {
            if (!skidding) {
                destabTraction = destabTraction * At<float>(handling, 0xA0);
            }
            if (adhesionSqr * destabTraction * destabTraction < speedSqr) {
                double limit = destabTraction * adhesion;
                float k = float(limit / float(std::sqrt(double(std::fabs(speedSqr)))));
                right = k * right;
            }
        }
    } else {
        if (*wheelState != 3) {
            *wheelState = driving && fwdSpeed < 0.20000000298023224 ? 1 : 2;
        }
        float loss = skidding ? 1.0f : At<float>(handling, 0xA0);
        float k = float(double(adhesion) / float(std::sqrt(double(std::fabs(speedSqr)))) * loss);
        fwd = k * fwd;
        right = k * right;
        if (destabTraction < 1.0f) {
            right = destabTraction * right;
        }
    }
    if (fwd == 0.0f && right == 0.0f) {
        return;
    }
    CVector f = {wheelFwd->x * fwd, wheelFwd->y * fwd, wheelFwd->z * fwd};
    CVector r = {wheelRight->x * right, wheelRight->y * right, wheelRight->z * right};
    CVector impulse;
    VectorAdd(&impulse, &r, &f);
    // the turning part without the handling's +0xC0 share of the forward push
    CVector turnImpulse = impulse;
    bool split = false;
    if (At<float>(handling, 0xC0) > 9.999999747378752e-05) {
        float share = 0.0f;
        if (braking) {
            share = At<float>(handling, 0xC0);
            split = true;
        } else if (driving) {
            share = float(At<float>(handling, 0xC0) * 0.5);
            split = true;
        }
        if (split) {
            CVector s = {wheelFwd->x * share, wheelFwd->y * share, wheelFwd->z * share};
            s = {s.x * fwd, s.y * fwd, s.z * fwd};
            turnImpulse = {impulse.x - s.x, impulse.y - s.y, impulse.z - s.z};
        }
    }
    CVector moveDir = impulse;
    float moveLength = float(std::sqrt(double(std::fabs(impulse.y * impulse.y + impulse.x * impulse.x + impulse.z * impulse.z))));
    float turnLength = moveLength;
    if (split) {
        float sqr = turnImpulse.x * turnImpulse.x + turnImpulse.y * turnImpulse.y + turnImpulse.z * turnImpulse.z;
        turnLength = float(std::sqrt(double(std::fabs(sqr))));
    }
    if (0.0f < moveLength) {
        float inv = 1.0f / moveLength;
        moveDir = {inv * impulse.x, impulse.y * inv, inv * impulse.z};
    } else {
        moveDir.x = 1.0f;
    }
    CVector turnDir;
    if (!split) {
        turnDir = moveDir;
    } else if (!(turnLength <= 0.0f)) {
        float inv = 1.0f / turnLength;
        turnDir = {inv * turnImpulse.x, turnImpulse.y * inv, inv * turnImpulse.z};
    } else {
        turnDir = turnImpulse;
        turnDir.x = 1.0f;
    }
    float mass = At<float>(self, 0x158);
    CVector tmp;
    const CVector* arm = CrossProduct(&tmp, contactPoint, &moveDir);
    float moveImpulse = mass * moveLength;
    float armSqr = arm->y * arm->y + arm->x * arm->x + arm->z * arm->z;
    float k = float(1.0 / (armSqr / At<float>(self, 0x15C) + 1.0 / At<float>(self, 0x158)));
    k = k * turnLength;
    CVector moveForce = {moveImpulse * moveDir.x, moveDir.y * moveImpulse, moveImpulse * moveDir.z};
    CVector turnForce = {k * turnDir.x, turnDir.y * k, k * turnDir.z};
    uint8_t touching = Game_Bike_4B9E10(self, 1);
    if (touching) {
        Physical_ApplyMoveForce(self, moveForce.x, moveForce.y, moveForce.z);
    }
    float* m = Entity_GetMatrix(self);
    float turnRight = m[1] * turnForce.y + turnForce.x * m[0] + m[2] * turnForce.z;
    m = Entity_GetMatrix(self);
    float pointRight = m[1] * contactPoint->y + contactPoint->x * m[0] + m[2] * contactPoint->z;
    const CVector* forward = Entity_4154A0(self, &tmp);
    float pointFwd = forward->y * contactPoint->y + contactPoint->x * forward->x + forward->z * contactPoint->z;
    if ((uint8_t(wheelId) != 1 || (!braking && !reversing)) && touching) {
        m = Entity_GetMatrix(self);
        CVector rightAxis = {m[0], m[1], m[2]};
        m = Entity_GetMatrix(self);
        CVector pointOnRight = {m[0] * pointRight, m[1] * pointRight, m[2] * pointRight};
        CVector turnOnRight = {rightAxis.x * turnRight, rightAxis.y * turnRight, rightAxis.z * turnRight};
        CVector tmp2, tmp3;
        const CVector* d = VectorSub(&tmp2, &turnForce, &turnOnRight);
        CVector force = {d->x * turnForceScale, d->y * turnForceScale, d->z * turnForceScale};
        const CVector* p = VectorSub(&tmp3, contactPoint, &pointOnRight);
        Physical_ApplyTurnForce(self, force.x, force.y, force.z, p->x, p->y, p->z);
    }
    CVector flatRight = {wheelRight->x, wheelRight->y, 0.0f};
    Vector_Normalise(&flatRight);
    CVector leanForce = {flatRight.x * turnRight, flatRight.y * turnRight, flatRight.z * turnRight};
    CVector leanPoint = {wheelFwd->x * pointFwd, wheelFwd->y * pointFwd, wheelFwd->z * pointFwd};
    if (touching) {
        Physical_ApplyTurnForce(self, leanForce.x, leanForce.y, leanForce.z, leanPoint.x, leanPoint.y, leanPoint.z);
    }
    // (the world positions of the two, for a debug line left out)
    const CVector* pos = self->matrix ? reinterpret_cast<const CVector*>(&self->matrix->m[12]) : &self->placementPos;
    CVector a, b;
    VectorAdd(&a, pos, &leanPoint);
    VectorAdd(&b, &a, &leanForce);
}

float SpeedSqr(CBike* self) {
    const CVector& v = self->moveSpeed;
    return v.y * v.y + v.x * v.x + v.z * v.z;
}

// 0x4BC9B0: the rider falls off (-3) when the bike tips over (its up's z at most 0.1
// less its squared speed, up to 0.4)
void __fastcall Bike_CheckFallOff(CBike* self, re::Edx, const void* arg) {
    if (!Seats_GetOccupant(Seats(self), 0)) {
        return;
    }
    float upZ = Entity_GetMatrix(self)[10];
    float s = SpeedSqr(self);
    if (s > 0.4000000059604645) {
        s = 0.4f;
    }
    if (!(0.10000000149011612 - s < upZ)) {
        Game_KnockOffRider(self, -3, arg, Seats_GetOccupant(Seats(self), 0), 0);
    }
}

// 0x4BCA60: the steering lock (the handling's +0x9C) scaled by grip: when rolling
// forwards, by how its speed compares with the top speed (+0x84, raised to 1.2 by
// +0x720 while it's been going a while), never under the bike data's +0x40 (at most 0.1)
float __fastcall Bike_GetSteerLock(CBike* self) {
    uint8_t* handling = self->handling;
    float top = At<float>(handling, 0x84);
    CVector tmp;
    const CVector* forward = Entity_4154A0(self, &tmp);
    const CVector& v = self->moveSpeed;
    float speed = v.y * forward->y + v.x * forward->x + v.z * forward->z;
    if (!(speed > 0.0f)) {
        return At<float>(handling, 0x9C);
    }
    float& boost = At<float>(self, 0x720);
    uint32_t since = At<uint32_t>(self, 0x724);
    if (At<int32_t>(self, 0x714) <= 2 && since + 0x32 < timeInMs) {
        if (1.19f < boost || since + 0xBB8 < timeInMs ||
            At<float>(bikeTuning, 0x54) * 0.10000000149011612 > std::fabs(At<float>(self, 0x6A8))) {
            boost = 1.2f;
        } else {
            boost = float(double(timeInMs - since) / 3000.0 * 0.20000000298023224 + 1.0);
        }
        top = boost * top;
    } else {
        boost = 1.0f;
    }
    float lean = float((std::fabs(At<float>(self, 0x6A8)) + 0.05000000074505806) / (At<float>(bikeTuning, 0x54) * 1.100000023841858));
    top = float((lean * lean * 0.550000011920929 + 0.699999988079071) * top);
    boost = top;
    const float* minGrip = &At<float>(At<uint8_t*>(self, 0x5D4), 0x40);
    float grip;
    if (!(top < speed)) {
        float e = 1.0f - speed / top;
        float p = float(std::pow(double(e), 2.0));
        grip = float(p * (1.0f - *minGrip)) + *minGrip;
    } else {
        grip = *minGrip - (speed - top) * (1.0f - *minGrip) / top;
    }
    float least = *minGrip;
    if (!(least < 0.10000000149011612)) {
        least = 0.1f;
    }
    if (least > grip) {
        grip = least;
    }
    return At<float>(handling, 0x9C) * grip;
}

// 0x4BCCE0: the brake's squeal (+0x718) while braking hard with a wheel down and
// moving (on snow, or in water where 0x430320 says so, other sounds); else faded out
void __fastcall Bike_UpdateBrakeSound(CBike* self) {
    bool braking = 0.5f < At<float>(self, 0x298) &&
                   (1.0f > At<float>(self, 0x62C) || 1.0f > At<float>(self, 0x630));
    float s = float(SpeedSqr(self) * 100.0);
    void* sound = &At<uint8_t>(self, 0x718);
    if (1.0f > s || !braking) {
        if (At<int32_t>(self, 0x71C) != 0) {
            Sound_SetVolume(sound, 0.0f);
            Sound_Stop(sound, 1.0f);
        }
        return;
    }
    if (At<int32_t>(self, 0x71C) == 0) {
        uint32_t id;
        if (chapterAreaKinds[chapter] == 2 && Fn_430320(currArea)) {
            id = soundBikeBrakeSnow;
        } else if (0.0f > At<float>(self, 0x5E4) && Fn_430320(currArea)) {
            id = soundBikeOnBigWet;
        } else {
            id = soundBikeBrake;
        }
        SoundManager_Play(soundManager, &id, sound, 0);
        if (At<int32_t>(self, 0x71C) == 0) {
            return;
        }
    }
    CVector front = At<CVector>(self, 0x5DC);
    Sound_SetPosition(sound, &front, 0);
}

// 0x4BCE70: the pad's rumble for a driver 0xD: each wheel's surface (+0x794, the one
// before +0x798); rough ground (8, 0x23, 0x2A, 0x38, 0x3A) rumbles with speed, 0x3B
// now and then
void __fastcall Bike_UpdateRumble(CBike* self) {
    if (!Seats_GetOccupant(Seats(self), 0) || At<int32_t>(Seats_GetOccupant(Seats(self), 0), 0x1310) != 0xD) {
        return;
    }
    for (uint8_t i = 0; i < 2; i++) {
        int32_t& surface = At<int32_t>(self, 0x794 + i * 8);
        int32_t old = surface;
        surface = At<uint8_t>(self, 0x5FE + i * 0x28);
        At<int32_t>(self, 0x798 + i * 8) = old;
        if (!(1.0f > At<float>(self, 0x62C + i * 4))) {
            continue;
        }
        if (SpeedSqr(self) > 0.007000000216066837 &&
            (surface == 8 || surface == 0x23 || surface == 0x2A || surface == 0x38 || surface == 0x3A) &&
            !Pad_IsRumbleOff()) {
            int length = uint8_t(int(SpeedSqr(self) * 40.0 + 1.0));
            Pad_Rumble(0, 0x4B, length, 1);
            continue;
        }
        if (SpeedSqr(self) > 0.04f && surface == 0x3B && ((prevTimeInMs ^ timeInMs) & 0xFFFFFF00) != 0) {
            float s = SpeedSqr(self);
            if (s > RandomFloatInRange(0.0f, 0.1f)) {
                int length = uint8_t(int(Vector_MagnitudeSqr(&self->moveSpeed) * 10.0));
                int n = int(Vector_MagnitudeSqr(&self->moveSpeed) * 30.0);
                // (only the low 16 bits multiplied: imul ax)
                int strength = int(uint32_t(n) & 0xFFFF0000u) | uint16_t(n * 0x19);
                Pad_Rumble(0, strength, length, 1);
            }
            return;
        }
    }
}

// 0x4BD0D0: the rider thrown off by a crash (-6)
void __fastcall Bike_CrashRider(CBike* self, re::Edx, const void* arg) {
    if (Seats_GetOccupant(Seats(self), 0)) {
        Game_KnockOffRider(self, -6, arg, Seats_GetOccupant(Seats(self), 0), 0);
    }
}

// 0x4BD110: a gear change for a driver 0xD: a rumble and the "BikeShift" sound
void __fastcall Bike_ShiftGear(CBike* self) {
    if (!Seats_GetOccupant(Seats(self), 0) || At<int32_t>(Seats_GetOccupant(Seats(self), 0), 0x1310) != 0xD) {
        return;
    }
    Pad_Rumble(0, 0x32, 0x18, 1);
    const CVector* pos = self->matrix ? reinterpret_cast<const CVector*>(&self->matrix->m[12]) : &self->placementPos;
    int id = SoundStreams_GetId(soundStreams, "BikeShift");
    SoundStreams_Play(soundStreams, id, pos, 0, pos, 0, 0, 0, 0, 0, 0, 6, 1, 1.0f, 0);
}

// 0x4BD1A0: its speed across the ground kept within the handling's top speed times
// the tuning's +0x5C (not in mode 0xE for a driver that isn't 0xD)
void __fastcall Bike_LimitSpeed(CBike* self) {
    uint8_t* mode = gameMode;
    if (mode && At<uint8_t>(mode, 4) && At<int32_t>(mode, 0x3C) == 0xE && Seats_GetOccupant(Seats(self), 0) &&
        At<int32_t>(Seats_GetOccupant(Seats(self), 0), 0x1310) != 0xD) {
        return;
    }
    CVector& v = self->moveSpeed;
    float speed = float(std::sqrt(double(std::fabs(v.x * v.x + v.y * v.y))));
    float max = At<float>(self->handling, 0x84) * At<float>(bikeTuning, 0x5C);
    if (max < speed) {
        float k = max / speed;
        v.x = k * v.x;
        v.y = k * v.y;
    }
}

// 0x4BD280: the jump (+0x77C, set by BikeJump) done: a push up and along its up, more
// when fast and pointing up
void __fastcall Bike_DoJump(CBike* self) {
    if (!Seats_GetOccupant(Seats(self), 0) || At<float>(self, 0x77C) == 0.0f) {
        return;
    }
    CVector tmp;
    float up = Entity_4154A0(self, &tmp)->z;
    if (up < 0.0) {
        up = 0.0f;
    }
    float t = float(SpeedSqr(self) * up / 0.10000000149011612);
    if (t > 0.4000000059604645) {
        t = 0.4f;
    }
    float j = float(At<float>(At<uint8_t*>(self, 0x5D4), 0x44) * At<float>(self, 0x77C) * (t + 1.0));
    j = j * At<float>(self, 0x158);
    CVector g = {float(vec9E33DC.x * 0.8500000238418579), float(vec9E33DC.y * 0.8500000238418579),
                 float(vec9E33DC.z * 0.8500000238418579)};
    CVector upRow = {0.0f, 0.0f, 1.0f};
    if (float* m = re::PtrCast<float*>(self->matrix)) {
        upRow = {m[8], m[9], m[10]};
    }
    CVector u = {float(upRow.x * 0.15000000596046448), float(upRow.y * 0.15000000596046448),
                 float(upRow.z * 0.15000000596046448)};
    const CVector* sum = VectorAdd(&tmp, &u, &g);
    Physical_ApplyMoveForce(self, sum->x * j, sum->y * j, sum->z * j);
    Seats_GetOccupant(Seats(self), 0);
    At<float>(self, 0x77C) = 0.0f;
}

// vf59 0x4BD450: the autopilot's pedals (not for a driver 0xD) from where it wants to
// go (x, y; z unused): +0x294 the gas, +0x298 the brake, +0x29C the driver's +0x2E0's
// +0x1C, +0x474 pedalling
void __fastcall Bike_SetAutoPilotPedals(CBike* self, re::Edx, float x, float y, float) {
    if (!Seats_GetOccupant(Seats(self), 0) || At<int32_t>(Seats_GetOccupant(Seats(self), 0), 0x1310) == 0xD) {
        return;
    }
    CVector tmp;
    const CVector* fwd = Entity_4154A0(self, &tmp);
    const CVector& v = self->moveSpeed;
    float speed = v.y * fwd->y + v.x * fwd->x + v.z * fwd->z;
    int32_t sign = -1;
    if (!(y < 0.0f)) {
        sign = 1;
    }
    uint8_t wasPedalling = At<uint8_t>(self, 0x474);
    float half = std::fabs(float(x * 0.5));
    float t = half * float(sign) + y;
    float& gas = At<float>(self, 0x294);
    float& brake = At<float>(self, 0x298);
    float& scale = At<float>(self, 0x29C);
    scale = At<float>(At<uint8_t*>(Seats_GetOccupant(Seats(self), 0), 0x2E0), 0x1C);
    At<uint8_t>(self, 0x474) = 0;
    float limit = At<float>(At<uint8_t*>(Seats_GetOccupant(Seats(self), 0), 0x2E0), 0x1C) * At<float>(self->handling, 0x84);
    if (limit < speed) {
        gas = float(double(scale) * 0.0);
        if (t == 0.0f) {
            brake = 1.0f;
        }
        return;
    }
    if (t > 0.0f) {
        uint8_t* mode = gameMode;
        if (!mode || !At<uint8_t>(mode, 4) || GetGameMode() != 0xE) {
            scale = 1.0f;
        }
    } else if (t == 0.0f) {
        gas = float(double(scale) * 0.0);
        brake = std::fabs(speed) < 0.001f ? 0.1f : 1.0f;
        return;
    } else {
        float cruise = At<float>(bikeTuning, 0x78);
        if (0.001f > scale) {
            gas = float(double(scale) * 0.0);
            brake = 0.2f;
            return;
        }
        if (0.008f < speed) {
            bool pedal = wasPedalling == 1;
            if (!pedal) {
                float sqr = v.x * v.x + v.y * v.y;
                pedal = float(cruise * cruise) * 0.699999988079071 < sqr;
            }
            if (pedal) {
                At<uint8_t>(self, 0x474) = 1;
                gas = scale;
                brake = 0.5f;
                return;
            }
        }
        if (-0.65f < y && 0.008f < speed) {
            gas = float(scale * 0.25);
            brake = 0.0f;
            return;
        }
    }
    gas = scale * t;
    brake = 0.0f;
}

// 0x4BD740: a wheel's grip on its surfaces (0x45B500), times 1.4 on smooth ground (0,
// 1, 6) and always for class 0xD; a scooter (8) 0.65 on grass, dirt, sand, 0x36; none
// of it in the snowy chapter
float __fastcall Bike_GetWheelAdhesion(CBike* self, re::Edx, const void* colPoint) {
    float adhesion = GetSurfaceAdhesion(colPoint);
    int32_t cls;
    uint8_t surface = At<uint8_t>(self, 0x5FE);
    if (*Vehicle_GetVehicleClass(self, &cls) != 0xD) {
        if (chapterAreaKinds[chapter] == 2) {
            return adhesion;
        }
        if (*Vehicle_GetVehicleClass(self, &cls) == 8) {
            switch (surface) {
            case 0:
            case 1:
            case 6:
                break;
            case 2:
            case 3:
            case 0x1B:
            case 0x36:
                return float(adhesion * 0.6499999761581421);
            default:
                return adhesion;
            }
        } else if (surface > 1 && surface != 6) {
            return adhesion;
        }
    }
    return float(adhesion * 1.399999976158142);
}

// 0x4BD850: GTA's SetupSuspensionLines: each wheel's suspension line in the collision
// (from its frame's position, `fromFrames`, kept in +0x660/+0x664; else from those),
// the lengths (+0x680.., +0x688..), the height above the road (+0x690, +0x658..), the
// collision's box and radius grown to fit
void __fastcall Bike_SetupSuspensionLines(CBike* self, re::Edx, int fromFrames) {
    CVehicleModel* mi = modelInfoPtrs[self->modelIndex];
    uint8_t* col = mi->colModel;
    uint8_t* colData = At<uint8_t*>(col, 0x38);
    uint8_t* handling = self->handling;
    void* frame = nullptr;
    NiTransform t;
    t.scale = 1.0f;
    for (int i = 0; i < 2; i++) {
        CVector p;
        if (!re::Flag(fromFrames)) {
            const float* line = reinterpret_cast<float*>(At<uint8_t*>(colData, 4) + i * 0x20);
            p = {line[0], line[1], At<float>(self, 0x660 + i * 4)};
        } else {
            if (i == 0) {
                frame = At<void*>(self, 0x590);
            } else if (i == 1) {
                frame = At<void*>(self, 0x594);
            }
            void* wheelFrame = frame;
            t = *Frame_GetTransform(frame);
            // the transform up to the root
            for (void* parent = Frame_GetParent(frame); parent;) {
                NiTransform product;
                t = *NiTransform_Multiply(Frame_GetTransform(parent), &product, &t);
                parent = Frame_GetParent(parent);
                if (parent == wheelFrame) {
                    parent = nullptr;
                }
                frame = parent;
            }
            p = {0.0f, t.translate.y, t.translate.z};
            if (i == 0) {
                At<float>(self, 0x660) = p.z;
            } else if (i == 1) {
                At<float>(self, 0x664) = p.z;
            }
        }
        p.z = At<float>(handling, 0xB4) + p.z;
        float* line = reinterpret_cast<float*>(At<uint8_t*>(colData, 4) + i * 0x20);
        line[0] = p.x;
        line[1] = p.y;
        line[2] = p.z;
        double travel = At<float>(handling, 0xB8) - At<float>(handling, 0xB4);
        float end = float(travel - VehicleModel_GetWheelScale(mi) * 0.5 + p.z);
        line[4] = p.x;
        line[5] = p.y;
        line[6] = end;
        At<float>(self, 0x680 + i * 4) = At<float>(handling, 0xB4) - At<float>(handling, 0xB8);
        At<float>(self, 0x688 + i * 4) = line[2] - line[6];
    }
    const float* lines = re::PtrCast<float*>(At<uint8_t*>(colData, 4));
    float top = lines[2];
    float bias = At<float>(handling, 0xAC);
    float k = float((1.0 - 1.0 / (bias + bias)) * At<float>(self, 0x680));
    At<float>(self, 0x690) = float(k - top + VehicleModel_GetWheelScale(mi) * 0.5);
    for (int j = 0; j < 2; j++) {
        At<float>(self, 0x658 + j * 4) = float(VehicleModel_GetWheelScale(mi) * 0.5 - At<float>(self, 0x690));
    }
    CVector& boxMin = At<CVector>(col, 0x10);
    CVector& boxMax = At<CVector>(col, 0x20);
    if (boxMin.z > lines[6]) {
        boxMin.z = lines[6];
    }
    float a = float(std::sqrt(double(std::fabs(boxMax.y * boxMax.y + boxMax.x * boxMax.x + boxMax.z * boxMax.z))));
    float b = float(std::sqrt(double(std::fabs(boxMin.y * boxMin.y + boxMin.x * boxMin.x + boxMin.z * boxMin.z))));
    float radius = a < b ? b : a;
    if (At<float>(col, 0xC) < radius) {
        At<float>(col, 0xC) = radius;
    }
    At<float>(self, 0x35C) = At<float>(self, 0x690);
    Virtual<void>(self, 0xC4, col);
}

// vf45 0x4BDBE0: a frame (+0x580..) turned to angles in degrees about x, then y, then z
// (its position kept)
void __fastcall Bike_SetComponentRotation(CBike* self, re::Edx, int frame, float x, float y, float z) {
    NiTransform* t = Frame_GetTransform(self->frames[frame]);
    veh::CMatrix m;
    m.attached = t;
    Matrix_UpdateMatrix(&m, t);
    CVector pos = {m.m[12], m.m[13], m.m[14]};
    Matrix_SetRotateX(&m, float(x * 3.1415927410125732 / 180.0));
    Matrix_RotateY(&m, float(y * 3.1415927410125732 / 180.0));
    Matrix_RotateZ(&m, float(z * 3.1415927410125732 / 180.0));
    m.m[12] = m.m[12] + pos.x;
    m.m[13] = m.m[13] + pos.y;
    m.m[14] = m.m[14] + pos.z;
    Matrix_UpdateRW(&m);
}

// 0x4BDCC0: a hard collision (+0x1A0 its impulse, +0x1A8 its normal) throws the
// rider off (-5): harder head on, from the side, and from below; less for a parked
// bike or a mission's (+0x290 2); the lean (+0x6B8 2) stopped by one past 13
void __fastcall Bike_ProcessCrash(CBike* self) {
    float impulse = self->damageImpulse;
    float damage = float(impulse * 800.0 / At<float>(self, 0x158));
    if (!(self->typeAndStatus & 0xF8)) {
        damage = float(damage * 0.6499999761581421);
    } else if (self->createdBy == 2) {
        damage = float(damage * 0.4000000059604645);
    }
    if (!(self->flags33A & 0x40)) {
        return;
    }
    if ((At<uint8_t>(self, 0x6B8) & 2) && impulse > 13.0) {
        At<uint8_t>(self, 0x6B8) &= 0xFD;
    }
    if (!Seats_GetOccupant(Seats(self), 0) || veh::PedVehicle(Seats_GetOccupant(Seats(self), 0)) != self ||
        !(10.0f < damage)) {
        return;
    }
    const CVector* normal = &self->damagePos;
    CVector tmp;
    const CVector* fwd = Entity_4154A0(self, &tmp);
    float headOn = std::fabs(normal->y * fwd->y + normal->x * fwd->x + normal->z * fwd->z);
    float headOnScale;
    if (!(headOn <= 0.8500000238418579)) {
        float c = MaxFloat(0.0f, DotProduct(normal, &vec9E33DC));
        if (c < 0.8500000238418579) {
            c = 0.0f;
        }
        headOnScale = float(7.0 * c * c + 0.6000000238418579);
    } else {
        headOnScale = 0.6f;
    }
    float* m = Entity_GetMatrix(self);
    float side = std::fabs(normal->y * m[1] + normal->x * m[0] + normal->z * m[2]);
    m = Entity_GetMatrix(self);
    float up = normal->y * m[9] + normal->x * m[8] + normal->z * m[10];
    float fromBelow = 0.0f > up ? 0.0f : up;
    float fromAbove = 0.0f < up ? 0.0f : up;
    float total = float(((headOnScale * headOn + side * 0.44999998807907104 + fromBelow * 0.05000000074505806) -
                         fromAbove * 1.5) * damage);
    if (total <= 50.0) {
        return;
    }
    if (!Seats_GetOccupant(Seats(self), 0) || !Seats_GetOccupant(Seats(self), 0) ||
        veh::PedVehicle(Seats_GetOccupant(Seats(self), 0)) != self) {
        return;
    }
    CVector down = {0.0f, 0.0f, -1.0f};
    const CVector* pos = self->matrix ? reinterpret_cast<const CVector*>(&self->matrix->m[12]) : &self->placementPos;
    Vector_Add(&down, pos);
    Game_KnockOffRider(self, -5, &down, Seats_GetOccupant(Seats(self), 0), 0);
}

// 0x4BDF80: a ped gets on as the rider: stopped, its status (0 for a driver 0xD, else
// 3), an event (0x14) when it isn't the owner's or the ped's last, owner and +0x6BC
// set, the driver's seat, the ride animations, not static
void __fastcall Bike_SetRider(CBike* self, re::Edx, CPed* ped) {
    self->moveSpeed = vec0;
    self->turnSpeed = vec0;
    if (!ped) {
        return;
    }
    Virtual<void>(self, 0x18, At<int32_t>(ped, 0x1310) == 0xD ? 0 : 3);
    if (reinterpret_cast<CEntity*>(ped) != self->owner && veh::PedLastVehicle(ped) != self) {
        CVehicle* p = reinterpret_cast<CVehicle*>(ped);
        CVector pos = p->matrix ? *reinterpret_cast<const CVector*>(&p->matrix->m[12]) : p->placementPos;
        Events_Add(events, 0x14, ped, self->owner, &pos, -1.0f, -1.0f);
    }
    SetRef(&self->owner, ped);
    SetRef(&At<uint8_t>(self, 0x6BC), ped);
    Seats_SetDriver(Seats(self), ped);
    At<int32_t>(ped, 0x1338) = 0;
    At<int32_t>(ped, 0x13A4) = 0;
    self->flags33A |= 2;
    CVehicleModel* bikeInfo = modelInfoPtrs[self->modelIndex];
    CVehicleModel* pedInfo = modelInfoPtrs[reinterpret_cast<CVehicle*>(ped)->modelIndex];
    Virtual<void>(self, 0xE8, At<int32_t>(pedInfo, 0xC4), bikeInfo->animGroup);
    Virtual<void>(self, 0x1C, 0);
}

// 0x4BE0E0: per status (typeAndStatus >> 3): the rider's (0: the controls, 3: the
// autopilot's steering) and the physics for 2 (steering, the physical's control, the
// wheels' spin from the ground; returns 0); the others stopped, braked or at rest.
// +0x160 its friction (0.01 ridden, else 0.1); +0x6B8's 0x10 "being ridden"
uint8_t __fastcall Bike_ProcessStatus(CBike* self) {
    CVehicleModel* mi = modelInfoPtrs[self->modelIndex];
    float wheelScale = mi->unknown58;
    At<uint8_t>(self, 0x6B8) &= 0x7F;
    At<float>(self, 0x160) = Seats_GetOccupant(Seats(self), 0) ? 0.01f : 0.1f;
    uint8_t& flags = At<uint8_t>(self, 0x6B8);
    switch (self->typeAndStatus >> 3) {
    case 0:
        flags = uint8_t((flags & 0xFE) | 0x90);
        Virtual<void>(self, 0xA8);   // (ProcessControlInputs)
        Vehicle_450970(self);
        break;
    case 1:
        flags |= 0x10;
        break;
    case 2: {
        Fn_418A50(self);
        Game_SetTrails(self, 0);
        Game_UpdateSteering(self);
        Physical_ProcessControl(self);
        Fn_422A10(self);
        wheelScale = float(wheelScale * 0.5);
        At<uint8_t>(self, 0x6EC) = 2;
        At<uint8_t>(self, 0x6ED) = 2;
        CVector tmp;
        const CVector* fwd = Entity_4154A0(self, &tmp);
        float spin = Vehicle_CollisionImpulse(nullptr, 0, fwd, &self->moveSpeed, wheelScale);
        At<float>(self, 0x648) = At<float>(self, 0x648) + spin;
        At<float>(self, 0x64C) = spin + At<float>(self, 0x64C);
        if (self->autoPilotFlags & 3) {
            Virtual<void>(self, 0xDC);   // (UpdateHorn)
        }
        if (self->unknown368 != 0) {
            self->unknown368--;
        }
        self->flags33B &= 0xF5;
        return 0;
    }
    case 3: {
        uint8_t riding = Seats_GetOccupant(Seats(self), 0) != nullptr;
        flags = uint8_t(((riding & 1) | 0xF8) << 4 | (flags & 0xEE));
        Game_SetTrails(self, 0);
        Game_UpdateSteering(self);
        Vehicle_450970(self);
        break;
    }
    case 4:
        Game_SetTrails(self, 0);
        self->flags33A |= 4;
        At<float>(self, 0x298) = 0.0f;
        flags &= 0xEE;
        self->unknown368 = 0;
        At<float>(self, 0x294) = float(double(self->unknown29C) * 0.0);
        break;
    case 5:
    case 6: {
        Game_SetTrails(self, 0);
        bool braking;
        if ((self->typeAndStatus >> 3) == 5) {
            At<float>(self, 0x298) = 0.05f;
            braking = true;
        } else {
            const CVector& v = self->moveSpeed;
            float sqr = v.y * v.y + v.x * v.x + v.z * v.z;
            if (sqr < 0.01f) {
                At<float>(self, 0x298) = 1.0f;
                braking = true;
            } else {
                self->flags33A &= 0xFB;
                At<float>(self, 0x298) = 0.0f;
                braking = false;
            }
        }
        if (braking) {
            self->flags33A |= 4;
        }
        Virtual<void>(self, 0xE0, 0.0f);   // (SetSteerAngle)
        flags &= 0xEF;
        self->unknown368 = 0;
        At<float>(self, 0x294) = float(double(self->unknown29C) * 0.0);
        break;
    }
    case 7: {
        uint8_t riding = 0;
        if (Seats_GetOccupant(Seats(self), 0) && Ped_IsSeatedInVehicle(Seats_GetOccupant(Seats(self), 0))) {
            riding = 1;
        }
        At<float>(self, 0x298) = 1.0f;
        flags ^= uint8_t(((riding << 4) ^ flags) & 0x10);
        self->moveSpeed = vec0;
        Game_SetTrails(self, 0);
        break;
    }
    default:
        break;
    }
    if ((self->typeAndStatus & 0xF8) && self->unknown368 != 0) {
        self->unknown368--;
    }
    return 1;
}

// 0x4BE410: keeping it upright (while +0x6B8 has 0x13: leaning 2, 1, 0x10 ridden):
// its turn speed about its right and forward axes damped (+0x63C / +0x640 the wheels'
// ground contact; the bike data's +0x2C / +0x30 the pitch it settles to), then a turn
// force pulling its up towards 0x9E33DC's opposite. The lean flag (2) drops when
// tilted, a slow scooter's is set
void __fastcall Bike_ProcessStabiliser(CBike* self) {
    uint8_t& flags = At<uint8_t>(self, 0x6B8);
    CVector tmp;
    if (flags & 2) {
        if (std::fabs(Entity_GetMatrix(self)[2]) > 0.35f || std::fabs(Entity_4154A0(self, &tmp)->z) > 0.5f) {
            flags &= 0xFD;
        }
    }
    int32_t cls;
    if (*Vehicle_GetVehicleClass(self, &cls) == 8) {
        uint8_t f = flags;
        if (!(f & 0x10)) {
            const CVector& v = self->moveSpeed;
            float speed = float(std::sqrt(double(std::fabs(v.x * v.x + v.y * v.y))));
            if (speed < 0.006) {
                flags = f | 2;
            }
        }
    }
    if (At<uint8_t>(self, 0x708) && !(std::fabs(At<float>(self, 0x6B0)) <= 0.01f) && At<uint8_t>(self, 0x709)) {
        if (*Vehicle_GetVehicleClass(self, &cls) == 2 || *Vehicle_GetVehicleClass(self, &cls) == 0xB) {
            return;
        }
    }
    if (!(flags & 0x13)) {
        return;
    }
    float k = 40.0f;
    CVector damping = stabiliserDamping;
    CVector localTurn;
    Multiply3x3Transposed(&localTurn, &self->turnSpeed, Entity_GetMatrix(self));
    uint8_t* data = At<uint8_t*>(self, 0x5D4);
    if (At<float>(self, 0x63C) == 0.0f) {
        k = 100.0f;
        if ((At<float>(self, 0x640) == 0.0f && !At<uint8_t>(self, 0x780)) || !(0.0f < Entity_4154A0(self, &tmp)->z)) {
            damping.x = 0.98f;
        } else {
            double settle = At<float>(data, 0x2C);
            float e = float(std::fabs(float(settle - Entity_4154A0(self, &tmp)->z)) * 0.25);
            if (!(e <= 0.07000000029802322)) {
                e = 0.07f;
            }
            damping.x = damping.x - e;
        }
    } else if (At<float>(self, 0x640) == 0.0f) {
        k = 100.0f;
        if (0.0f > Entity_4154A0(self, &tmp)->z) {
            double settle = At<float>(data, 0x30);
            float e = float(std::fabs(float(settle - Entity_4154A0(self, &tmp)->z)) * 0.30000001192092896);
            if (!(e <= 0.10000000149011612)) {
                e = 0.1f;
            }
            damping.x = float((e + 0.8999999761581421) * damping.x);
        }
    }
    damping.x = 1.0f / (localTurn.x * k * localTurn.x + 1.0f) * damping.x;
    damping.y = float(1.0 / (localTurn.y * 1000.0 * localTurn.y + 1.0) * damping.y);
    damping.x = float(std::pow(double(damping.x), double(timeStep)));
    damping.y = float(std::pow(double(damping.y), double(timeStep)));
    float dx = localTurn.x * damping.x - localTurn.x;
    float dy = damping.y * localTurn.y - localTurn.y;
    float inertia = At<float>(self, 0x15C);
    // about the forward axis
    float* m = Entity_GetMatrix(self);
    CVector up = {float(m[8] * -1.0), float(m[9] * -1.0), float(m[10] * -1.0)};
    CVector t = {up.x * dy, up.y * dy, up.z * dy};
    CVector force = {t.x * inertia, t.y * inertia, t.z * inertia};
    m = Entity_GetMatrix(self);
    CVector com = self->centreOfMass;
    struct { CVector v; float pad; } offsetPad16;   // (the game's transform writes a 4th float)
    CVector& offset = offsetPad16.v;
    Multiply3x3(&offset, m, &com);
    CVector point;
    VectorAdd(&point, reinterpret_cast<const CVector*>(Entity_GetMatrix(self)), &offset);
    Physical_ApplyTurnForce(self, force.x, force.y, force.z, point.x, point.y, point.z);
    // about the right axis
    m = Entity_GetMatrix(self);
    t = {m[8] * dx, m[9] * dx, m[10] * dx};
    inertia = At<float>(self, 0x15C);
    force = {t.x * inertia, t.y * inertia, t.z * inertia};
    m = Entity_GetMatrix(self);
    com = self->centreOfMass;
    Multiply3x3(&offset, m, &com);
    const CVector* fwd = Entity_4154A0(self, &tmp);
    CVector sum;
    VectorAdd(&sum, fwd, &offset);
    point = sum;
    Physical_ApplyTurnForce(self, force.x, force.y, force.z, point.x, point.y, point.z);
    // upright
    CVector pull;
    if (*Vehicle_GetVehicleClass(self, &cls) == 3) {
        m = Entity_GetMatrix(self);
        up = {m[8], m[9], m[10]};
        const CVector* d = VectorSub(&tmp, &vec9E33DC, &up);
        pull = {float(d->x * 0.05999999865889549), float(d->y * 0.05999999865889549), float(d->z * 0.05999999865889549)};
    } else {
        m = Entity_GetMatrix(self);
        if (!(0.0f < m[10]) && !(flags & 1)) {
            return;
        }
        float strength = 0.01f;
        if (At<uint8_t>(self, 0x780)) {
            strength = 0.04f;
        } else if (At<uint8_t>(self, 0x6EC) == 1) {
            const CVector* u = Physical_GetUp(self, &tmp);
            float dot = u->y * At<float>(self, 0x5A4) + At<float>(self, 0x5A0) * u->x + u->z * At<float>(self, 0x5A8);
            if (dot < 0.8999999761581421) {
                strength = 0.04f;
            }
        }
        m = Entity_GetMatrix(self);
        up = {m[8], m[9], m[10]};
        const CVector* d = VectorSub(&tmp, &vec9E33DC, &up);
        pull = {strength * d->x, d->y * strength, strength * d->z};
    }
    inertia = At<float>(self, 0x15C);
    force = {pull.x * inertia, pull.y * inertia, pull.z * inertia};
    Physical_ApplyTurnForce(self, force.x, force.y, force.z, up.x, up.y, up.z);
}

// the wheel's col point (+0x5DC, 0x28 each: +0x10 the normal, +0x20 / +0x22 the surfaces)
uint8_t* ColPoint(CBike* self, int wheel) {
    return &At<uint8_t>(self, 0x5DC + wheel * 0x28);
}

// its forward along the ground (the normal's part taken out) and its right from that
void WheelAxes(const CVector& dir, const CVector* normal, CVector* fwd, CVector* right) {
    float d = normal->y * dir.y + normal->x * dir.x + normal->z * dir.z;
    CVector along = {d * normal->x, normal->y * d, d * normal->z};
    *fwd = {dir.x - along.x, dir.y - along.y, dir.z - along.z};
    Vector_Normalise(fwd);
    CVector tmp;
    *right = *CrossProduct(&tmp, fwd, normal);
    Vector_Normalise(right);
}

// 0x4BEB60: the front wheel on the ground: its forward turned by the steering
// (+0x6A4), its grip (the handling's +0xA4 twice, the surface's, a parked bike's cubed,
// 0.4 burst; 1000 with the other wheel off the ground), the brake (+0x94 twice), GTA's
// ProcessBikeWheel (special 2 when the rear is off the ground); off the ground its spin
// (+0x650) runs down
void __fastcall Bike_ProcessFrontWheel(CBike* self, re::Edx, int wheel, float adhesionScale, CVector point, float brake,
                                       float thrust) {
    uint8_t* handling = self->handling;
    float brakeScale = float(At<float>(handling, 0x94) * 2.0);
    float gripScale = float(2.0 * At<float>(handling, 0xA4));
    float& wheelSpeed = At<float>(self, 0x650 + wheel * 4);
    if (At<uint8_t>(self, 0x6B8) & 8) {
        wheelSpeed = 0.0f;
    }
    if (!(0.0f < At<float>(self, 0x63C + wheel * 4))) {
        wheelSpeed = float(wheelSpeed * 0.949999988079071);
        At<float>(self, 0x648 + wheel * 4) = wheelSpeed + At<float>(self, 0x648 + wheel * 4);
        return;
    }
    int special = 0;
    if (!(0.0f < At<float>(self, 0x640))) {
        special = 2;
    }
    float s = float(std::sin(double(At<float>(self, 0x6A4))));
    float c = float(std::cos(double(At<float>(self, 0x6A4))));
    CVector local = {-s, c, 0.0f};
    struct { CVector v; float pad; } dirPad16;   // (the game's transform writes a 4th float)
    CVector& dir = dirPad16.v;
    Multiply3x3(&dir, Entity_GetMatrix(self), &local);
    uint8_t* cp = ColPoint(self, wheel);
    CVector fwd, right;
    WheelAxes(dir, reinterpret_cast<const CVector*>(cp + 0x10), &fwd, &right);
    float driven = thrust;
    cp[0x20] = 0x30;
    float adhesion = Game_GetWheelAdhesion(self, cp) * adhesionScale;
    float traction = GetSurfaceTraction(cp[0x22]);
    if (!(self->typeAndStatus & 0xF8)) {
        float f = GetSurfaceTraction(cp[0x22]);
        adhesion = adhesion * f;
        adhesion = adhesion * f;
        adhesion = f * adhesion;
    }
    if (At<uint8_t>(self, 0x5D8 + wheel) == 1) {
        adhesion = float(adhesion * 0.4000000059604645);
    }
    CVector speed;
    Physical_GetSpeed(self, &speed, &point);
    if (speed.y * speed.y + speed.x * speed.x + speed.z * speed.z < 9.999999747378752e-06f) {
        speed = self->moveSpeed;
    }
    adhesion = 0.0f < At<float>(self, 0x640) ? adhesion * gripScale : 1000.0f;
    if (!Seats_GetOccupant(Seats(self), 0)) {
        driven = 0.0f;
    }
    if (0.0f < At<float>(self, 0x470)) {
        traction = float(At<float>(self, 0x470) * 0.029999999329447746 * traction);
    }
    int32_t* state = &At<int32_t>(self, 0x70C + wheel * 4);
    Game_ProcessWheel(self, &fwd, &right, &speed, &point, 2, driven, brakeScale * brake, adhesion, traction, wheel,
                      &wheelSpeed, state, special, At<uint8_t>(self, 0x5D8 + wheel));
    if ((At<uint8_t>(self, 0x6B8) & 0x40) && (*state == 1 || *state == 2)) {
        *state = 0;
    }
}

// 0x4BEF50: the rear wheel: the same along the bike's forward, the handling's grip
// and brake as (1 - x) twice, the surface kind's traction (0.9, 0.7) while pedalling
// (+0x6C4), pedalling's push (+0x470) on traction and brake; special 1; its wheel
// state reset from spinning while going backwards (+0x294 below 0)
void __fastcall Bike_ProcessRearWheel(CBike* self, re::Edx, int wheel, float adhesionScale, CVector point, float brake,
                                      float thrust) {
    uint8_t* handling = self->handling;
    float brakeScale = float((1.0f - At<float>(handling, 0x94)) * 2.0);
    float gripScale = float(2.0 * (1.0f - At<float>(handling, 0xA4)));
    float& wheelSpeed = At<float>(self, 0x650 + wheel * 4);
    int32_t* state = &At<int32_t>(self, 0x70C + wheel * 4);
    if (At<uint8_t>(self, 0x6B8) & 8) {
        wheelSpeed = 0.0f;
    }
    if (0.0f < At<float>(self, 0x63C + wheel * 4)) {
        CVector tmp;
        CVector dir = *Entity_4154A0(self, &tmp);
        uint8_t* cp = ColPoint(self, wheel);
        CVector fwd, right;
        WheelAxes(dir, reinterpret_cast<const CVector*>(cp + 0x10), &fwd, &right);
        float driven = thrust;
        cp[0x20] = 0x30;
        float adhesion = Game_GetWheelAdhesion(self, cp) * adhesionScale;
        float traction = 1.0f;
        if (0.0f < At<float>(self, 0x6C4)) {
            switch (GetSurfaceKind(cp[0x22])) {
            case 1:
            case 3:
                traction = 0.9f;
                break;
            case 2:
                traction = 0.7f;
                break;
            default:
                break;
            }
        }
        if (!(self->typeAndStatus & 0xF8)) {
            float f = GetSurfaceTraction(cp[0x22]);
            adhesion = f * adhesion;
            adhesion = adhesion * f;
            adhesion = f * adhesion;
        }
        if (At<uint8_t>(self, 0x5D8 + wheel) == 1) {
            adhesion = float(adhesion * 0.4000000059604645);
        }
        CVector speed;
        Physical_GetSpeed(self, &speed, &point);
        if (speed.y * speed.y + speed.x * speed.x + speed.z * speed.z < 9.999999747378752e-06f) {
            speed = self->moveSpeed;
        }
        adhesion = 0.0f < At<float>(self, 0x63C) ? adhesion * gripScale : 1000.0f;
        if (!Seats_GetOccupant(Seats(self), 0)) {
            driven = 0.0f;
        }
        if (0.0f < At<float>(self, 0x470)) {
            traction = float(At<float>(self, 0x470) * 0.05999999865889549 * traction);
            brake = float(At<float>(self, 0x470) * 1.2000000476837158 * brake);
        }
        Game_ProcessWheel(self, &fwd, &right, &speed, &point, 2, driven, brakeScale * brake, adhesion, traction, wheel,
                          &wheelSpeed, state, 1, At<uint8_t>(self, 0x5D8 + wheel));
        if ((At<uint8_t>(self, 0x6B8) & 0x40) && (*state == 1 || *state == 2)) {
            *state = 0;
        }
    } else {
        At<float>(self, 0x648 + wheel * 4) = wheelSpeed + At<float>(self, 0x648 + wheel * 4);
    }
    if (0.0f > At<float>(self, 0x294) && *state == 1) {
        *state = 0;
    }
}

} // namespace

// ---- registration ---------------------------------------------------------------------

#define BIKEP_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

BIKEP_REVERSED("CBike::ProcessWheel", 0x004BBD50, &Bike_ProcessWheel);
BIKEP_REVERSED("CBike::CheckFallOff", 0x004BC9B0, &Bike_CheckFallOff);
BIKEP_REVERSED("CBike::GetSteerLock", 0x004BCA60, &Bike_GetSteerLock);
BIKEP_REVERSED("CBike::UpdateBrakeSound", 0x004BCCE0, &Bike_UpdateBrakeSound);
BIKEP_REVERSED("CBike::UpdateRumble", 0x004BCE70, &Bike_UpdateRumble);
BIKEP_REVERSED("CBike::CrashRider", 0x004BD0D0, &Bike_CrashRider);
BIKEP_REVERSED("CBike::ShiftGear", 0x004BD110, &Bike_ShiftGear);
BIKEP_REVERSED("CBike::LimitSpeed", 0x004BD1A0, &Bike_LimitSpeed);
BIKEP_REVERSED("CBike::DoJump", 0x004BD280, &Bike_DoJump);
BIKEP_REVERSED("CBike::SetAutoPilotPedals", 0x004BD450, &Bike_SetAutoPilotPedals);
BIKEP_REVERSED("CBike::GetWheelAdhesion", 0x004BD740, &Bike_GetWheelAdhesion);
BIKEP_REVERSED("CBike::SetupSuspensionLines", 0x004BD850, &Bike_SetupSuspensionLines);
BIKEP_REVERSED("CBike::SetComponentRotation", 0x004BDBE0, &Bike_SetComponentRotation);
BIKEP_REVERSED("CBike::ProcessCrash", 0x004BDCC0, &Bike_ProcessCrash);
BIKEP_REVERSED("CBike::SetRider", 0x004BDF80, &Bike_SetRider);
BIKEP_REVERSED("CBike::ProcessStatus", 0x004BE0E0, &Bike_ProcessStatus);
BIKEP_REVERSED("CBike::ProcessStabiliser", 0x004BE410, &Bike_ProcessStabiliser);
BIKEP_REVERSED("CBike::ProcessFrontWheel", 0x004BEB60, &Bike_ProcessFrontWheel);
BIKEP_REVERSED("CBike::ProcessRearWheel", 0x004BEF50, &Bike_ProcessRearWheel);
