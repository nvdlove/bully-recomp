// CBike: its suspension and wheels' grip, the lean and wheelie turned into forces, its
// rendering, what happens on hitting things, its matrices for the rider and its set-up.
// Fields are reached by offset: CBike's layout (Vehicle.h) only has a few named.

#include "game/Vehicle.h"

#include <cmath>
#include <cstring>

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

// Gamebryo's NiTransform: a 3x3 rotation, the translation, the scale
struct NiTransform {
    float rotate[9];      // +0
    CVector translate;    // +0x24
    float scale;          // +0x30
};
static_assert(sizeof(NiTransform) == 0x34);

GAME_VAR(float, timeStep, 0x00C1A9A4);        // CTimer's
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);     // CTimer's
GAME_VAR(CVector, vec0, 0x00BCC564);          // (a zero vector)
GAME_VAR(uint8_t*, bikeTuning, 0x020C8900);   // (the bike tuning values)
GAME_ARRAY(uint8_t*, modelInfoPtrs, 0x00C67738);   // CModelInfo::ms_modelInfoPtrs
GAME_VAR(uint8_t, cutsceneRunning, 0x020C5BE4);
GAME_VAR(void*, entityC2A8E8, 0x00C2A8E8);

inline int __cdecl Rand() { return crt::Rand(); }

GAME_FN(CPed*, __thiscall, Seats_GetOccupant, 0x004CE560, CVehicleSeats* seats, uint32_t seat);
GAME_FN(int32_t*, __thiscall, Vehicle_GetVehicleClass, 0x004CAF30, CVehicle* vehicle, int32_t* out);
GAME_FN(float*, __thiscall, Entity_GetMatrix, 0x008B6120, CVehicle* entity);
GAME_FN(CVector*, __thiscall, Entity_GetForward, 0x004154A0, CVehicle* entity, CVector* out);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, void* ref);

// matrices
GAME_FN(void, __thiscall, Matrix_SetRotateX, 0x00412CA0, float* m, float angle);
GAME_FN(void, __thiscall, Matrix_RotateY, 0x00413120, float* m, float angle);
GAME_FN(void, __thiscall, Matrix_Copy, 0x00412720, float* m, const float* from);
GAME_FN(void, __thiscall, Matrix_Multiply, 0x00412800, float* m, const float* by);   // (*=)
GAME_FN(void, __thiscall, Matrix_UpdateFromAttached, 0x00412A70, float* m, const NiTransform* t);
GAME_FN(NiTransform*, __cdecl, Frame_GetTransform, 0x006C9A90, void* frame);
GAME_FN(void*, __cdecl, Frame_GetParent, 0x006C7370, void* frame);
GAME_FN(NiTransform*, __thiscall, Transform_Multiply, 0x00406830, const NiTransform* t, NiTransform* out,
        const NiTransform* by);

// rendering
GAME_FN(void, __thiscall, VehicleModel_SetColours, 0x00535680, uint8_t* modelInfo, int colour1, int colour2,
        void* rwObject);
inline void* const objD140B8 = reinterpret_cast<void*>(0x00D140B8);
GAME_FN(void, __thiscall, Fn_5EE570, 0x005EE570, void* object, int arg);
GAME_FN(void, __cdecl, Object_ForAllFrames, 0x006C7380, void* rwObject, void* callback, void* data);
GAME_FN(void, __cdecl, Object_408380, 0x00408380, void* rwObject);
GAME_FN(void, __cdecl, Object_Render, 0x006C7420, void* rwObject);
inline void* const frameCallback4B97D0 = reinterpret_cast<void*>(0x004B97D0);

GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, CrossProduct, 0x00414730, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
// (0x004146B0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* Multiply3x3(CVector* out, const float* m, const CVector* v) { return re::CallVec16<0x004146B0, CVector*>(out, m, v); }
GAME_FN(CVector*, __thiscall, Placement_GetRight, 0x00473600, void* placement, CVector* out);
GAME_FN(void, __thiscall, Physical_ApplyTurnForce, 0x0046A950, CVehicle* physical, float fx, float fy, float fz, float px,
        float py, float pz);
GAME_FN(float, __thiscall, Vehicle_WheelRotation, 0x004CB870, void* self, int kind, const CVector* fwd, const CVector* speed,
        float radius);   // (Vehicle.cpp's CollisionImpulse; the bike's this in ecx is unused)
GAME_FN(void, __cdecl, Transform_SetRotation, 0x00415EB0, NiTransform* t, const CVector* axis, float angle, int degrees);
GAME_FN(float, __cdecl, DivideBySqrt, 0x00414C20, float a, float b);   // (a / sqrt(|b|))
GAME_FN(float*, __thiscall, Placeable_GetMatrixOrTemp, 0x0046DD20, CVehicle* placeable);
GAME_FN(float, __cdecl, LimitRadianAngle, 0x00432EE0, float angle);
GAME_FN(void, __thiscall, Matrix_SetRotateZ, 0x00412C20, float* m, float angle);
// (0x0045D500 stores 16 bytes through `out`: called through re::CallVec16)
static inline void Matrix_TransformPoint(CVector* out, const float* m, const CVector* point) { re::CallVec16<0x0045D500, void>(out, m, point); }
GAME_FN(float, __cdecl, FindGroundZFor3DCoord, 0x004635A0, float x, float y, float z, uint8_t* found, int arg);
GAME_FN(float, __thiscall, CVector_Normalise, 0x004294A0, CVector* v);
GAME_FN(float, __thiscall, VehicleModel_GetUnknown58, 0x005350F0, uint8_t* modelInfo);
GAME_FN(void, __thiscall, Placeable_SetMatrix, 0x0046DE90, CVehicle* placeable, const float* m, int arg);
GAME_FN(void, __thiscall, Physical_ApplySpringCollision, 0x0046C160, CVehicle* physical, float springConst,
        const CVector* springDir, const CVector* point, float springRatio, float bias);
GAME_FN(void, __thiscall, Physical_ApplySpringCollisionAlt, 0x0046C260, CVehicle* physical, float springConst,
        const CVector* springDir, const CVector* point, float springRatio, float bias, const CVector* normal);
GAME_FN(void, __thiscall, Physical_45D410, 0x0045D410, CVehicle* physical, const CVector* a, const CVector* b);   // (to ApplyTurnForce)
GAME_VAR(int32_t, currArea, 0x00BD1008);       // CGame's
GAME_VAR(uint8_t, chapter, 0x00A147E0);
GAME_ARRAY(int32_t, chapterAreaKinds, 0x00A147EC);   // (by chapter: 2 the snowy one)
GAME_FN(uint8_t, __cdecl, Fn_430320, 0x00430320, int area);
GAME_FN(void, __thiscall, Vehicle_Reset, 0x004CB5B0, CVehicle* vehicle, int unused);
GAME_FN(void, __thiscall, Seats_RemoveDriver, 0x004CE960, CVehicleSeats* seats);
GAME_FN(void, __thiscall, Seats_Init, 0x004CE470, CVehicleSeats* seats, CVehicle* vehicle);
GAME_FN(void, __thiscall, VehicleModel_ChooseVehicleColour, 0x00535D90, uint8_t* modelInfo, uint8_t* colour1,
        uint8_t* colour2);
GAME_FN(void*, __cdecl, MemoryMgr_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void*, __cdecl, MemoryMgr_Malloc2, 0x005EEA50, uint32_t size, int arg);
GAME_FN(void*, __thiscall, CollisionData_Construct, 0x0056DAD0, void* data);
GAME_ARRAY(uint8_t, handlingData, 0x00C2DEE4);       // (0xDC each)
GAME_ARRAY(uint8_t, bikeHandlingData, 0x00C32778);   // (0x5C each)
GAME_FN(void, __thiscall, Game_Bike_4BD850, 0x004BD850, CBike* bike, int arg);
GAME_FN(void, __thiscall, Game_StopSounds, 0x004B9BA0, CBike* bike);
GAME_FN(void, __thiscall, Game_PlayBaseAnim, 0x004B9D40, CBike* bike);
// this file's own and CBike's others, called where they are
GAME_FN(float, __thiscall, Game_GetWheelAdhesion, 0x004BD740, CBike* bike, const void* colPoint);
GAME_FN(CPed*, __thiscall, Game_KnockOffRider, 0x004BA050, CBike* bike, int kind, void* arg, CPed* ped, int unused);
GAME_FN(void, __thiscall, Game_CrashRider, 0x004BD0D0, CBike* bike, void* arg);

GAME_FN(float, __thiscall, Game_GetSteerLock, 0x004BCA60, CBike* bike);
CVehicleSeats* Seats(void* bike) {
    return &static_cast<CVehicle*>(bike)->seats;
}

uint8_t* ColModel(CVehicle* vehicle) {
    return At<uint8_t*>(modelInfoPtrs[vehicle->modelIndex], 0xC);
}

// 0x4BF6B0: two wheels' suspension heights (+0x658) eased (by 3/4) towards the col
// model's line's z (+8 of its 0x20-byte lines) less the compression times +0x680
void __fastcall Bike_UpdateWheelHeights(CBike* self, re::Edx, int wheel1, int wheel2) {
    uint8_t* lines = At<uint8_t*>(At<uint8_t*>(ColModel(self), 0x38), 4);
    const int wheels[2] = {wheel1, wheel2};
    for (int wheel : wheels) {
        float z = At<float>(lines, wheel * 0x20 + 8);
        float compression = At<float>(self, 0x62C + wheel * 4);
        if (0.0f < compression) {
            z = z - At<float>(self, 0x680 + wheel * 4) * compression;
        }
        float& height = At<float>(self, 0x658 + wheel * 4);
        height = float((z - height) * 0.75 + height);
    }
}

// 0x4BF780: the brake's grip: the bike data's +0x20 times 4 times `scale`, on a col
// point of the default surfaces (0x30, 1)
float __fastcall Bike_GetBrakeGrip(CBike* self, re::Edx, float scale) {
    // (a CColPoint on the stack: its +0xC a registered entity, let go at the end)
    uint8_t colPoint[0x28] = {};
    At<uint8_t>(colPoint, 0x20) = 0x30;
    At<uint8_t>(colPoint, 0x22) = 1;
    At<float>(colPoint, 0x24) = 3.4028234663852886e+38f;
    float adhesion = Game_GetWheelAdhesion(self, colPoint);
    float grip = float(At<float>(At<uint8_t*>(self, 0x5D4), 0x20) * 4.0 * scale * adhesion);
    void* entity = At<void*>(colPoint, 0xC);
    if (entity) {
        // (the same either way: compared with 0xC2A8E8 first)
        if (entity == entityC2A8E8) {
            CleanUpOldReference(entity, &At<uint8_t>(colPoint, 0xC));
            return grip;
        }
        CleanUpOldReference(entity, &At<uint8_t>(colPoint, 0xC));
    }
    return grip;
}

// vf18 0x4BFCC0: rendered (not while hidden, nor riderless in a cutscene unless its
// status is 7): its colours, 0xD140B8's 7, its frames told its +0x114's lean vector
// when +0x6B9 has 4
void __fastcall Bike_Render(CBike* self) {
    if (*reinterpret_cast<int32_t*>(&self->unknownD0) != 0) {
        return;
    }
    if (!Seats_GetOccupant(Seats(self), 0) && (self->typeAndStatus & 0xF8) != 0x38 && cutsceneRunning) {
        return;
    }
    VehicleModel_SetColours(modelInfoPtrs[self->modelIndex], self->colour1, self->colour2, self->rwObject);
    Fn_5EE570(objD140B8, 7);
    if (At<uint8_t>(self, 0x6B9) & 4) {
        uint8_t* data = At<uint8_t*>(At<uint8_t*>(self->animData, 0x14), 8);
        CVector v = At<CVector>(data, 0x58);
        Object_ForAllFrames(self->rwObject, frameCallback4B97D0, &v);
    }
    Object_408380(self->rwObject);
    Object_Render(self->rwObject);
}

// 0x4C0200: the rider's matrix (+0x734): its own turned by the lean (+0x6A8), and the
// seat moved (+0x764) along its up by how far the lean lowers it; +0x730 set
void __fastcall Bike_UpdateRiderMatrix(CBike* self) {
    float lean[17];   // (a CMatrix, +0x40 none)
    lean[16] = 0.0f;
    float tilt = float(std::fabs(At<float>(self, 0x6A8)) * -0.05000000074505806);
    Matrix_SetRotateX(lean, tilt);
    Matrix_RotateY(lean, At<float>(self, 0x6A8));
    float* rider = &At<float>(self, 0x734);
    Matrix_Copy(rider, Entity_GetMatrix(self));
    Matrix_Multiply(rider, lean);
    float c = float(std::cos(double(At<float>(self, 0x6A8))));
    float boxZ = At<float>(ColModel(self), 0x18);
    const float* up = Entity_GetMatrix(self) + 8;
    CVector u = {up[0], up[1], up[2] * boxZ};
    float k = 1.0f - c;
    CVector d = {u.x * k, u.y * k, u.z * k};
    At<float>(self, 0x764) = At<float>(self, 0x764) + d.x;
    At<float>(self, 0x768) = d.y + At<float>(self, 0x768);
    At<float>(self, 0x76C) = At<float>(self, 0x76C) + d.z;
    At<uint8_t>(self, 0x730) = 1;
}

// 0x4C08B0: a collision (+0xC the other one): a bike it hit loses its rider (-6); a
// ped that hit it loses its bike's rider (0x4BD0D0); nothing happens to this bike
void __fastcall Bike_OnCollision(CBike* self, re::Edx, uint8_t* collision) {
    (void)self;
    CVehicle* other = At<CVehicle*>(collision, 0xC);
    uint8_t type = other->typeAndStatus & 7;
    if (type == 2) {
        if (other->vehicleType == 1 && Seats_GetOccupant(&other->seats, 0)) {
            Game_KnockOffRider(static_cast<CBike*>(other), -6, collision, Seats_GetOccupant(&other->seats, 0), 0);
        }
    } else if (type == 3) {
        CVehicle* vehicle = veh::PedVehicle(reinterpret_cast<CPed*>(other));
        if (vehicle && vehicle->vehicleType == 1) {
            Game_CrashRider(static_cast<CBike*>(vehicle), collision);
        }
    }
}

// 0x4C0920: a frame's (+0x580) matrix in the model: its transform times its parents'
// up to below the root, into `out` (whose +0x40 is left pointing at the stack's
// transform: the game's)
void __fastcall Bike_GetFrameMatrix(CBike* self, re::Edx, int frameIndex, float* out) {
    NiTransform total;
    total.scale = 1.0f;
    void* frame = self->frames[frameIndex];
    std::memcpy(&total, Frame_GetTransform(frame), sizeof(NiTransform));
    for (void* parent = Frame_GetParent(frame); parent;) {
        NiTransform product;
        std::memcpy(&total, Transform_Multiply(Frame_GetTransform(parent), &product, &total), sizeof(NiTransform));
        parent = Frame_GetParent(parent);
        if (parent == frame || Frame_GetParent(parent) == parent || !Frame_GetParent(parent)) {
            break;
        }
    }
    *reinterpret_cast<re::Ptr32<NiTransform>*>(out + 16) = &total;
    Matrix_UpdateFromAttached(out, &total);
}

// 0x4BF300: the lean (+0x6AC, copied to +0x6A8): decaying by 0.95 a step when neither
// wheel is down (+0x6B8 0x13 clear, or nothing touching); else towards the angle whose
// sine is what the steering (or, with +0x6EC, the speed towards `target` along +0x5AC,
// its forward across +0x5A0) asks for, within the bike data's +0x14 (0.4 of it with the
// front tyre burst), easing by its +0x1C; on the ground at a standstill the steering
// alone (scaled by the speed, within the tuning's +0x54; 1.85 times while pedalling)
void __fastcall Bike_UpdateLean(CBike* self, re::Edx, CVector target) {
    float& lean = At<float>(self, 0x6AC);
    uint8_t flags = At<uint8_t>(self, 0x6B8);
    bool touching = At<uint8_t>(self, 0x780) || At<float>(self, 0x63C) != 0.0f || At<float>(self, 0x640) != 0.0f;
    float value;
    if (!(flags & 0x13) || !touching) {
        float k = float(std::pow(0.949999988079071, double(timeStep)));
        value = k * lean;
    } else {
        CVector tmp, tmp2;
        CVector& across = At<CVector>(self, 0x5AC);
        across = *CrossProduct(&tmp2, Entity_GetForward(self, &tmp), &At<CVector>(self, 0x5A0));
        Vector_Normalise(&across);
        float want;
        if (!At<uint8_t>(self, 0x6EC)) {
            float steer = self->steerAngle;
            float step = timeStep;
            double lock = Game_GetSteerLock(self) * 3.1415927410125732 / 180.0;
            want = float(double(steer) / lock * -0.5 * 0.00800000037997961 * step);
        } else {
            target = *VectorSub(&tmp2, &self->moveSpeed, &target);
            want = across.y * target.y + across.x * target.x + across.z * target.z;
        }
        float step = timeStep < 0.5 ? 0.5f : timeStep;
        float a = float(want / (step * 0.00800000037997961));
        uint8_t* data = At<uint8_t*>(self, 0x5D4);
        float limit = At<float>(data, 0x14);
        if (At<uint8_t>(self, 0x5D8) == 1) {
            double burst = limit * 0.4000000059604645;
            if (a > burst) {
                a = float(burst);
            } else if (-At<float>(data, 0x14) * 0.4000000059604645 > a) {
                a = float(-At<float>(data, 0x14) * 0.4000000059604645);
            }
        } else if (limit < a) {
            a = limit;
        } else if (-At<float>(data, 0x14) > a) {
            a = -At<float>(data, 0x14);
        }
        float k = float(std::pow(double(At<float>(data, 0x1C)), double(timeStep)));
        float angle = float(std::asin(double(a)));
        value = float(k * lean + (1.0f - k) * (angle - 0.0));
    }
    lean = value;
    if (!(flags & 2) && touching) {
        CVector tmp;
        const CVector* fwd = Entity_GetForward(self, &tmp);
        const CVector& v = self->moveSpeed;
        float speed = v.y * fwd->y + v.x * fwd->x + v.z * fwd->z;
        float steer = -self->steerAngle * At<float>(bikeTuning, 0x40);
        lean = steer;
        float fraction = std::fabs(speed);
        if (At<int32_t>(self, 0x714) == 2) {
            fraction = float(fraction * 1.2000000476837158 / At<float>(self->handling, 0x84));
        } else {
            fraction = fraction / At<float>(self->handling, 0x84);
        }
        lean = fraction * steer;
        float max = At<float>(bikeTuning, 0x54);
        float l = lean;
        if (l > max) {
            l = max;
        }
        float min = -At<float>(bikeTuning, 0x54);
        if (l < min) {
            l = min;
        }
        lean = l;
        if (0.0f < At<float>(self, 0x470)) {
            lean = float(l * 1.850000023841858);
        }
    }
    At<float>(self, 0x6A8) = lean;
}

// 0x4BF820: the lean (+0x6A8) held by a turn force: towards the up it should have
// (leaning by +0x6A8 about its heading, less its part along its forward, or with +0x6EC
// 2 its right across +0x5A0), at the top of the bike (its centre of mass turned, plus
// its up); stronger when +0x6B8 has 1 (more when upside down). Not while doing a
// wheelie on class 2 / 0xB bikes (+0x708, +0x709, +0x6B0 past 0.01), nor with neither
// wheel down. At rest (no rider, slow) +0x6B8's 2 kept
void __fastcall Bike_HoldLean(CBike* self) {
    if (At<uint8_t>(self, 0x708) && std::fabs(At<float>(self, 0x6B0)) > 0.01f && At<uint8_t>(self, 0x709)) {
        int32_t cls;
        if (*Vehicle_GetVehicleClass(self, &cls) == 2 || *Vehicle_GetVehicleClass(self, &cls) == 0xB) {
            return;
        }
    }
    if (!(At<uint8_t>(self, 0x6B8) & 0x13)) {
        return;
    }
    float lean = At<float>(self, 0x6A8);
    float s = float(std::sin(double(lean)));
    float heading;
    if (float* m = re::PtrCast<float*>(self->matrix)) {
        heading = float(std::atan2(double(-m[4]), double(m[5])));
    } else {
        heading = At<float>(self, 0x10);
    }
    heading = -heading;
    CVector want;
    want.x = float(std::cos(double(heading))) * s;
    want.y = -float(std::sin(double(heading))) * s;
    want.z = float(std::cos(double(lean)));
    Vector_Normalise(&want);
    CVector fwd, tmp;
    Entity_GetForward(self, &fwd);
    if (At<uint8_t>(self, 0x6EC) == 2) {
        CVector right;
        if (float* m = re::PtrCast<float*>(self->matrix)) {
            right = {m[0], m[1], m[2]};
        } else {
            Placement_GetRight(&self->placementPos, &right);
        }
        fwd = *CrossProduct(&tmp, &right, &At<CVector>(self, 0x5A0));
    }
    float d = fwd.y * want.y + fwd.x * want.x + fwd.z * want.z;
    CVector along = {fwd.x * d, fwd.y * d, fwd.z * d};
    want = {want.x - along.x, want.y - along.y, want.z - along.z};
    Vector_Normalise(&want);
    Entity_GetMatrix(self);
    float* m = Entity_GetMatrix(self);
    CVector com = self->centreOfMass;
    struct { CVector v; float pad; } topPad16;   // (the game's transform writes a 4th float)
    CVector& top = topPad16.v;
    Multiply3x3(&top, m, &com);
    // ((want less its up is worked out too, unused))
    float factor;
    bool upright = At<uint8_t>(self, 0x6B8) & 1;
    if (upright) {
        float k = 1.0f;
        if (float* mm = re::PtrCast<float*>(self->matrix)) {
            if (0.0f > mm[10]) {
                k = 1.0f - mm[10];
            }
        }
        m = Entity_GetMatrix(self);
        factor = float(At<float>(self, 0x15C) * (k * 0.23000000417232513));
    } else {
        m = Entity_GetMatrix(self);
        factor = float(At<float>(self, 0x15C) * 0.18000000715255737);
    }
    CVector up = {m[8], m[9], m[10]};
    CVector force = {factor * want.x, want.y * factor, want.z * factor};
    const CVector* point = VectorAdd(&tmp, &top, &up);
    Physical_ApplyTurnForce(self, force.x, force.y, force.z, point->x, point->y, point->z);
    if (upright) {
        return;
    }
    const CVector& v = self->moveSpeed;
    if (Seats_GetOccupant(Seats(self), 0) || 0.0015999999595806003 < float(v.x * v.x + v.y * v.y)) {
        At<uint8_t>(self, 0x6B8) &= 0xFD;
    }
}

// 0x4BFD90: its wheels turned (the front one steered): each wheel's rotation (+0x648,
// +0x64C, the step +0x650 / +0x654) from its speed along the wheel's forward (none
// when it's settling: no gas or no +0x29C, light braking, slow, both wheels near their
// rest, +0x780 clear), into the animation's transforms (+0x784 the fork's copied to
// +0x788 the front wheel's, +0x78C the rear one's); the last transform (the handlebars)
// turned flat to its xy and lowered by +0x35C
void __fastcall Bike_UpdateWheelTransforms(CBike* self) {
    float s = float(std::sin(double(self->steerAngle)));
    float c = float(std::cos(double(self->steerAngle)));
    float* m = Entity_GetMatrix(self);
    CVector local = {-s, c, 0.0f};
    struct { CVector v; float pad; } fwdPad16;   // (the game's transform writes a 4th float)
    CVector& fwd = fwdPad16.v;
    Multiply3x3(&fwd, m, &local);
    CVector axis = {1.0f, 0.0f, 0.0f};
    CVector zero = {0.0f, 0.0f, 0.0f};
    uint8_t* mi = modelInfoPtrs[self->modelIndex];
    NiTransform* transforms = At<NiTransform*>(At<uint8_t*>(self->animData, 0x14), 8);
    NiTransform* front = &transforms[At<int32_t>(self, 0x788)];
    NiTransform* fork = &transforms[At<int32_t>(self, 0x784)];
    NiTransform* rear = &transforms[At<int32_t>(self, 0x78C)];
    CVector tmp, speed;
    VectorAdd(&speed, CrossProduct(&tmp, &self->turnSpeed, &zero), &self->moveSpeed);
    float braking = At<float>(At<uint8_t*>(self, 0x2A4), 0x90) * At<float>(self, 0x298);
    float step = timeStep;
    float brakeStep = braking * step;
    if ((At<float>(self, 0x294) == 0.0f || At<float>(self, 0x29C) == 0.0f) && brakeStep <= 0.01f &&
        0.005f > std::fabs(self->moveSpeed.x) && 0.005f > std::fabs(self->moveSpeed.y)) {
        double z = std::fabs(self->moveSpeed.z);
        float r = Virtual<float>(self, 0x8C);
        if (r * step + 0.020010000094771385 > z && 0.8999999761581421 > At<float>(self, 0x634) &&
            At<float>(self, 0x638) < 0.8999999761581421 && !At<uint8_t>(self, 0x780)) {
            speed = vec0;
        }
    }
    CVector offset = {front->translate.x - fork->translate.x, front->translate.y - fork->translate.y,
                      front->translate.z - fork->translate.z};
    float radius = float(At<float>(mi, 0x58) * 0.5);
    float rot = Vehicle_WheelRotation(nullptr, 0, &fwd, &speed, radius);
    At<float>(self, 0x650) = rot;
    At<float>(self, 0x648) = rot + At<float>(self, 0x648);
    std::memcpy(front, fork, sizeof(NiTransform));
    float angle = float(At<float>(self, 0x648) * 180.0 / 3.1415927410125732);
    Transform_SetRotation(front, &axis, angle, 1);
    front->translate.x = front->translate.x + offset.x;
    front->translate.y = front->translate.y + offset.y;
    front->translate.z = front->translate.z + offset.z;
    radius = float(At<float>(mi, 0x58) * 0.5);
    rot = Vehicle_WheelRotation(nullptr, 0, &fwd, &speed, radius);
    At<float>(self, 0x654) = rot;
    float total = rot + At<float>(self, 0x64C);
    At<float>(self, 0x64C) = total;
    angle = float(total * 180.0 / 3.1415927410125732);
    Transform_SetRotation(rear, &axis, angle, 1);
    NiTransform* bars = &transforms[At<int32_t>(At<uint8_t*>(self->animData, 0x14), 4) - 1];
    float r1 = bars->rotate[1];
    float r4 = bars->rotate[4];
    float q = DivideBySqrt(1.0f, r1 * r1 + r4 * r4);
    float a = q * bars->rotate[1];
    bars->rotate[1] = a;
    float b = q * bars->rotate[4];
    bars->rotate[4] = b;
    bars->rotate[7] = 0.0f;
    bars->rotate[2] = 0.0f;
    bars->rotate[5] = 0.0f;
    bars->rotate[8] = 1.0f;
    bars->rotate[0] = b;
    bars->rotate[3] = -a;
    bars->rotate[6] = 0.0f;
    CVector down = {0.0f, 0.0f, -At<float>(self, 0x35C)};
    struct { CVector v; float pad; } movedPad16;   // (the game's transform writes a 4th float)
    CVector& moved = movedPad16.v;
    Multiply3x3(&moved, Placeable_GetMatrixOrTemp(self), &down);
    bars->translate.x = bars->translate.x + moved.x;
    bars->translate.y = bars->translate.y + moved.y;
    bars->translate.z = moved.z + bars->translate.z;
}

// vf53 0x4C0350: placed at (x, y, z) facing `heading`, standing on the ground: the col
// model's first two lines' ends (the front one first) dropped onto the ground (from
// z + 1) give its forward, the rest follows (the first line's end, less half the
// model's +0x58, touching the ground). Without ground under both, just put there.
// (The game's padding words of the vectors and matrix here are uninitialised stack;
// ours are 0)
void __fastcall Bike_PlaceOnGround(CBike* self, re::Edx, float x, float y, float z, float heading) {
    uint8_t* mi = modelInfoPtrs[self->modelIndex];
    float m[17] = {};   // (a CMatrix, +0x40 none)
    heading = LimitRadianAngle(heading);
    m[12] = x;
    m[13] = y;
    m[14] = 0.0f;
    Matrix_SetRotateZ(m, heading);
    const float* line = At<float*>(At<uint8_t*>(ColModel(self), 0x38), 0);
    struct { CVector p; float pad; } ends[2] = {{{line[0], line[1], line[2]}, 0.0f}, {{line[10], line[11], line[12]}, 0.0f}};
    if (ends[1].p.y > ends[0].p.y) {
        CVector first = ends[0].p;
        ends[0].p = ends[1].p;
        ends[1].p = first;
    }
    Matrix_TransformPoint(&ends[0].p, m, &ends[0].p);
    Matrix_TransformPoint(&ends[1].p, m, &ends[1].p);
    float from = float(z + 1.0);
    for (uint8_t i = 0; i < 2; i++) {
        uint8_t found;
        float ground = FindGroundZFor3DCoord(ends[i].p.x, ends[i].p.y, from, &found, 1);
        if (!found) {
            if (float* em = re::PtrCast<float*>(self->matrix)) {
                em[12] = x;
                em[13] = y;
                em[14] = z;
                em[15] = 0.0f;
            }
            self->placementPos = {x, y, z};
            if (self->matrix) {
                Matrix_SetRotateZ(re::PtrCast<float*>(self->matrix), heading);
            }
            At<float>(self, 0x10) = heading;
            return;
        }
        ends[i].p.z = ground;
    }
    CVector fwd = {ends[0].p.x - ends[1].p.x, ends[0].p.y - ends[1].p.y, ends[0].p.z - ends[1].p.z};
    CVector_Normalise(&fwd);
    CVector right = {fwd.y - fwd.z * 0.0f, fwd.z * 0.0f - fwd.x, fwd.x * 0.0f - fwd.y * 0.0f};
    CVector_Normalise(&right);
    CVector up = {right.y * fwd.z - right.z * fwd.y, right.z * fwd.x - right.x * fwd.z, right.x * fwd.y - fwd.x * right.y};
    CVector_Normalise(&up);
    std::memset(m, 0, sizeof(m));
    m[0] = right.x;
    m[1] = right.y;
    m[2] = right.z;
    m[4] = fwd.x;
    m[5] = fwd.y;
    m[6] = fwd.z;
    m[8] = up.x;
    m[9] = up.y;
    m[10] = up.z;
    CVector p = {line[0], line[1], line[2]};
    Matrix_TransformPoint(&p, m, &p);
    double lower = p.z - 0.10000000149011612;
    p.z = float(lower - VehicleModel_GetUnknown58(mi) * 0.5);
    CVector neg = {-p.x, -p.y, -p.z};
    m[12] = neg.x + ends[0].p.x;
    m[13] = neg.y + ends[0].p.y;
    m[14] = neg.z + ends[0].p.z;
    Placeable_SetMatrix(self, m, 0);
}

// 0x4C09E0: the suspension: each wheel down (+0x62C under 1) pushed by its spring
// (GTA's ApplySpringCollision; against the ground's normal unless it's steep, or up when
// both are nearly flat), the handling's +0xBC the bias (plus its pitch when +0x708 is
// clear; the rear wheel's 1 less); the wheels' surface kinds (+0x644, +0x646) marked
// for snow or dirt; a wheel in the air: its point (in `points`) from the col model's
// line. Then the rear pedalling wobble (+0x6C4) while braking hard on the ground
void __fastcall Bike_ProcessSuspension(CBike* self, re::Edx, CVector* points) {
    uint8_t* handling = nullptr;
    for (uint32_t i = 0; i < 2; i++) {
        float compression = At<float>(self, 0x62C + i * 4);
        if (!(1.0f > compression)) {
            const float* line = &At<float>(At<uint8_t*>(At<uint8_t*>(ColModel(self), 0x38), 4), i * 0x20 + 0x10);
            float* m = Entity_GetMatrix(self);
            CVector local = {line[0], line[1], line[2]};
            struct { CVector v; float pad; } worldPad16;   // (the game's transform writes a 4th float)
            CVector& world = worldPad16.v;
            Multiply3x3(&world, m, &local);
            points[i] = world;
            continue;
        }
        handling = self->handling;
        float bias = At<float>(handling, 0xBC);
        if (!At<uint8_t>(self, 0x708)) {
            CVector tmp;
            bias = float(Entity_GetForward(self, &tmp)->z * 0.125 + bias);
        }
        if (1.0f < bias) {
            bias = 1.0f;
        }
        if (i == 1) {
            bias = 1.0f - bias;
        }
        const CVector* dir = &At<CVector>(self, 0x668 + i * 0xC);
        const CVector* point = &points[i];
        float k = At<float>(handling, 0xAC);
        compression = At<float>(self, 0x62C + i * 4);
        if (0.9300000071525574 < std::fabs(At<float>(self, 0x5F4)) && std::fabs(At<float>(self, 0x61C)) > 0.9300000071525574) {
            CVector up = {0.0f, 0.0f, 1.0f};
            Physical_ApplySpringCollisionAlt(self, k, dir, point, compression, bias, &up);
        } else if (0.35f < At<float>(self, 0x5F4 + i * 0x28)) {
            Physical_ApplySpringCollisionAlt(self, k, dir, point, compression, bias, &At<CVector>(self, 0x5EC + i * 0x28));
        } else {
            Physical_ApplySpringCollision(self, k, dir, point, compression, bias);
        }
        uint8_t surface = At<uint8_t>(self, 0x5FE + i * 0x28);
        if (chapterAreaKinds[chapter] == 2 && Fn_430320(currArea)) {
            switch (surface) {
            case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 9: case 0x13: case 0x1B: case 0x1C:
            case 0x1D: case 0x1F: case 0x2B: case 0x2D: case 0x36: case 0x37: case 0x39: case 0x3A: case 0x3B: case 0x3D:
                At<uint8_t>(self, 0x644 + i) = 3;
                At<uint8_t>(self, 0x646 + i) = 1;
                break;
            default:
                break;
            }
        } else {
            switch (surface) {
            case 3:
            case 5:
                At<uint8_t>(self, 0x644 + i) = 1;
                break;
            case 0x1B:
            case 0x36:
                At<uint8_t>(self, 0x644 + i) = 2;
                At<uint8_t>(self, 0x646 + i) = 1;
                break;
            default:
                break;
            }
        }
    }
    const CVector& v = self->moveSpeed;
    float speed = float(std::sqrt(double(std::fabs(v.y * v.y + v.x * v.x + v.z * v.z))));
    float& wobble = At<float>(self, 0x6C4);
    if (!(1.0f > At<float>(self, 0x62C)) || !(At<float>(self, 0x298) - At<float>(self, 0x294) > 0.8999999761581421) ||
        !(0.02f < speed) || (self->flags33A & 4)) {
        wobble = 0.0f;
        return;
    }
    float r = float(double(Rand()) / 32767.0 * 0.5 + 0.5);
    wobble = float(r * 0.20000000298023224 * timeStep + wobble);
    if (!(1.0f > At<float>(self, 0x630))) {
        return;
    }
    float* m = Entity_GetMatrix(self);
    float steer = self->steerAngle;
    if (speed < 0.5) {
        speed = 0.5f;
    }
    if (speed > 0.30000001192092896) {   // (always: the two limits the wrong way round)
        speed = 0.3f;
    }
    float s = float(std::sin(double(wobble)));
    float f = float(-steer * speed * s * At<float>(self, 0x15C) * 0.029999999329447746);
    CVector turn = {m[0] * f, m[1] * f, m[2] * f};
    Physical_45D410(self, &points[1], &turn);
}

// 0x4C0EF0: (re)set up as a bike, `keep` its state (else reset, and without a rider
// lifted 1 up): its handling and bike data from the model, colours, its wheels and
// lean at rest, its col model's line data made sure of (0x4BD850 fills it), status 3,
// its seats, its base pose
void __fastcall Bike_SetUp(CBike* self, re::Edx, int keep) {
    if (!re::Flag(keep)) {
        Vehicle_Reset(self, 0);
        self->entities[0] = nullptr;
        self->entities[1] = nullptr;
    }
    self->vehicleType = 1;
    bool move;
    if (Seats_GetOccupant(Seats(self), 0)) {
        move = !re::Flag(keep);
        if (move) {
            Seats_RemoveDriver(Seats(self));
        }
    } else {
        move = !re::Flag(keep);
    }
    if (move) {
        float* m = re::PtrCast<float*>(self->matrix);
        CVector pos = m ? CVector{m[12], m[13], m[14]} : self->placementPos;
        float z = float(pos.z + 1.0);
        if (m) {
            m[12] = pos.x;
            m[13] = pos.y;
            m[14] = z;
            m[15] = 0.0f;   // (the game stores an uninitialised stack word)
        }
        self->placementPos = {pos.x, pos.y, z};
    }
    At<float>(self, 0x470) = 0.0f;
    At<uint16_t>(self, 0x6C0) = 0;
    At<uint8_t>(self, 0x6B8) = uint8_t((At<uint8_t>(self, 0x6B8) & 0x9F) | 0x10);
    uint8_t* mi = modelInfoPtrs[self->modelIndex];
    uint32_t id = uint8_t(At<int16_t>(mi, 0x5E));
    uint8_t* handling = &handlingData[id * 0xDC];
    self->handling = handling;
    At<uint8_t*>(self, 0x5D4) = &bikeHandlingData[id * 0x5C];
    At<float>(self, 0x158) = At<float>(handling, 4);
    At<float>(self, 0x15C) = At<float>(handling, 0xC);
    self->centreOfMass = At<CVector>(handling, 0x1C);
    VehicleModel_ChooseVehicleColour(mi, &self->colour1, &self->colour2);
    At<float>(self, 0x164) = 0.001f;
    At<float>(self, 0x168) = At<float>(self->handling, 0x2C);
    Virtual<void>(self, 0xE0, 0.0f);   // (SetSteerAngle)
    At<float>(self, 0x6A4) = 0.0f;
    At<uint8_t>(self, 0x6B8) &= 0xF8;
    At<float>(self, 0x6A8) = 0.0f;
    self->siren = 0;
    At<float>(self, 0x6AC) = 0.0f;
    At<float>(self, 0x294) = float(double(self->unknown29C) * 0.0);
    At<float>(self, 0x298) = 0.0f;
    At<float>(self, 0x6B0) = 0.0f;
    At<float>(self, 0x6C4) = 0.0f;
    At<float>(self, 0x6C8) = 0.0f;
    At<float>(self, 0x698) = 0.0f;
    At<float>(self, 0x6A0) = 0.0f;
    At<float>(self, 0x69C) = 0.0f;
    float rotation = float(0.0 * 3.1415927410125732 / 180.0);
    for (int i = 0; i < 2; i++) {
        At<float>(self, 0x648 + i * 4) = rotation;
        At<int32_t>(self, 0x70C + i * 4) = 0;
        At<float>(self, 0x650 + i * 4) = 0.0f;
        At<uint8_t>(self, 0x644 + i) = 0;
        At<uint8_t>(self, 0x646 + i) = 0;
        At<uint8_t>(self, 0x5D8 + i) = 0;
        At<CVector>(self, 0x6CC + i * 0xC) = vec0;
        At<float>(self, 0x62C + i * 4) = 1.0f;
        At<float>(self, 0x634 + i * 4) = 1.0f;
        At<float>(self, 0x63C + i * 4) = 0.0f;
    }
    At<uint8_t>(self, 0x6EC) = 0;
    At<uint8_t>(self, 0x6ED) = 0;
    At<float>(self, 0x690) = 0.0f;
    uint8_t* colModel = At<uint8_t*>(mi, 0xC);
    uint8_t* data = At<uint8_t*>(colModel, 0x38);
    if (!data) {
        void* memory = MemoryMgr_Malloc(0x24);
        data = memory ? static_cast<uint8_t*>(CollisionData_Construct(memory)) : nullptr;
        At<uint8_t*>(colModel, 0x38) = data;
    }
    if (At<void*>(data, 4) == nullptr) {
        At<uint8_t>(data, 0x20) = 2;
        At<void*>(data, 4) = MemoryMgr_Malloc2(0x40, 0);
    }
    Game_Bike_4BD850(self, 1);
    self->mission = 0;
    self->unknown2DB = 0;
    self->autoPilotFlags &= 0xFB;
    self->autoPilotTimer = timeInMs;
    Virtual<void>(self, 0x18, 3);   // (SetStatus)
    At<int32_t>(self, 0x3C) = 1;
    At<int32_t>(self, 0x30) = 0;
    Game_StopSounds(self);
    At<CVector>(self, 0x5B8) = vec0;
    At<CVector>(self, 0x5C4) = {0.0f, 0.0f, 0.0f};
    At<float>(self, 0x5D0) = 1.0f;
    Game_PlayBaseAnim(self);
    Seats_Init(Seats(self), self);
    At<float>(self, 0x720) = 1.0f;
}

} // namespace

// ---- registration ---------------------------------------------------------------------

#define BIKEE_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

BIKEE_REVERSED("CBike::UpdateWheelHeights", 0x004BF6B0, &Bike_UpdateWheelHeights);
BIKEE_REVERSED("CBike::GetBrakeGrip", 0x004BF780, &Bike_GetBrakeGrip);
BIKEE_REVERSED("CBike::Render", 0x004BFCC0, &Bike_Render);
BIKEE_REVERSED("CBike::UpdateRiderMatrix", 0x004C0200, &Bike_UpdateRiderMatrix);
BIKEE_REVERSED("CBike::OnCollision", 0x004C08B0, &Bike_OnCollision);
BIKEE_REVERSED("CBike::GetFrameMatrix", 0x004C0920, &Bike_GetFrameMatrix);
BIKEE_REVERSED("CBike::UpdateLean", 0x004BF300, &Bike_UpdateLean);
BIKEE_REVERSED("CBike::HoldLean", 0x004BF820, &Bike_HoldLean);
BIKEE_REVERSED("CBike::UpdateWheelTransforms", 0x004BFD90, &Bike_UpdateWheelTransforms);
BIKEE_REVERSED("CBike::PlaceOnGround", 0x004C0350, &Bike_PlaceOnGround);
BIKEE_REVERSED("CBike::ProcessSuspension", 0x004C09E0, &Bike_ProcessSuspension);
BIKEE_REVERSED("CBike::SetUp", 0x004C0EF0, &Bike_SetUp);
