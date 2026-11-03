// CAutomobile (vtable 0x905D04): Teleport (vf14) and its per-frame pre-render work
// (vf17, GTA's PreRender: the wheels' contact points for a playback car, exhaust smoke,
// the lights (coronas, their shadows and point lights), the siren, the wheels' spin and
// matrices, and the wheel effects). Fields are reached by offset: CAutomobile's layout
// (Vehicle.h) only has a few named.

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
    return re::PtrCast<R(__thiscall*)(void*, re::Arg<A>...)>(vtable[offset / 4])(object, args...);
}

// 0x45D500 writes four floats
struct Vec4 {
    float x, y, z, w;
};

// the game's CMatrix on the stack (16 floats, the attached Gamebryo transform)
struct Matrix {
    float m[16];
    re::Ptr32<void> attached;
    uint32_t owns;
};

GAME_VAR(float, timeStep, 0x00C1A9A4);        // CTimer's
GAME_VAR(CVector, vec0, 0x00BCC564);          // (a zero vector)
GAME_ARRAY(uint8_t*, modelInfoPtrs, 0x00C67738);   // CModelInfo::ms_modelInfoPtrs
GAME_VAR(float, speedScale, 0x00C2D170);      // (the forward speed is divided by it)
GAME_VAR(uint8_t, clockHours, 0x00BCEB4B);    // CClock's
GAME_VAR(uint8_t, clockMinutes, 0x00BCEB4A);
GAME_VAR(float, lightsFogA, 0x00CE755C);      // (lights on in fog/rain above a car's own level)
GAME_VAR(float, lightsFogB, 0x00CE7564);
GAME_VAR(float, exhaustGasA, 0x00A35700);     // (the gas above which more smoke)
GAME_VAR(float, exhaustGasB, 0x00A35704);
GAME_VAR(void*, cameraNode, 0x00C3CCF4);      // (the camera's Gamebryo node)
GAME_VAR(void*, texHeadlightShadow, 0x00C93724);
GAME_VAR(void*, texTaillightShadow, 0x00C93728);
inline void* const theCamera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const lightShafts = reinterpret_cast<void*>(0x00C66B78);

inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(float, __cdecl, RandomFloatInRange, 0x0041A5B0, float lo, float hi);

// placing and matrices
GAME_FN(void, __cdecl, World_Remove, 0x0045DD70, CVehicle* entity);
GAME_FN(void, __cdecl, World_Add, 0x0045D6B0, CVehicle* entity);
GAME_FN(void, __thiscall, Placeable_SetPosition, 0x0041C4E0, CVehicle* entity, float x, float y, float z);
GAME_FN(void, __thiscall, Placeable_SetOrientation, 0x0046A690, CVehicle* entity, float x, float y, float z);
GAME_FN(float*, __thiscall, Entity_GetMatrix, 0x008B6120, CVehicle* entity);
GAME_FN(CVector*, __thiscall, Entity_GetForward, 0x004154A0, CVehicle* entity, CVector* out);
GAME_FN(uint8_t, __thiscall, Entity_GetIsOnScreen, 0x00467460, CVehicle* entity);
GAME_FN(void*, __cdecl, Node_GetLocalTransform, 0x006C9A90, void* node);   // (node + 0x14)
GAME_FN(void*, __cdecl, Node_6C82F0, 0x006C82F0, void* node);              // (its +0x140)
GAME_FN(void, __thiscall, Matrix_FromNode, 0x00412A70, Matrix* m, void* transform);
// (0x0045D500 stores 16 bytes through `out`: called through re::CallVec16)
static inline void Matrix_MultiplyPoint4(Vec4* out, const float* m, const CVector* v) { re::CallVec16<0x0045D500, void>(out, m, v); }
// (0x0041A9A0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* Matrix_TransformPoint(CVector* out, const float* m, const CVector* v) { return re::CallVec16<0x0041A9A0, CVector*>(out, m, v); }
GAME_FN(CVector*, __cdecl, Matrix_Multiply3x3, 0x0041C490, CVector* out, const float* m, const CVector* v);
// (0x004146B0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* Matrix_Multiply3x3b(CVector* out, const float* m, const CVector* v) { return re::CallVec16<0x004146B0, CVector*>(out, m, v); }
GAME_FN(CVector*, __thiscall, Camera_GetPosition, 0x0041AAC0, void* camera);

// vectors
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub2, 0x00414BA0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorScale, 0x00412350, CVector* out, const CVector* v, float s);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);       // (returns the length in st0)
GAME_FN(float, __thiscall, Vector_Normalise2, 0x004294A0, CVector* v);      // (returns the length in st0)
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const CVector* v);
GAME_FN(CVector*, __thiscall, Vector_SubAssign, 0x00465480, void* self, CVector* out, const CVector* v);
GAME_FN(CVector*, __thiscall, Vector_AddAssign, 0x00414670, void* self, CVector* out, const CVector* v);
GAME_FN(CVector*, __thiscall, Vector_ScaleAssign, 0x00413A20, void* self, CVector* out, float s);
GAME_FN(uint8_t, __cdecl, VectorNotEqual, 0x004305F0, const CVector* a, const CVector* b);
GAME_FN(float, __cdecl, SafeAcos, 0x00413D60, float x);
GAME_FN(float, __cdecl, Fn_414C20, 0x00414C20, float a, float b);   // (a / sqrt |b|)

// the vehicle's
GAME_FN(CPed*, __thiscall, Seats_GetOccupant, 0x004CE560, CVehicleSeats* seats, uint32_t seat);
GAME_FN(int32_t*, __thiscall, Vehicle_GetVehicleClass, 0x004CAF30, CVehicle* vehicle, int32_t* out);
GAME_FN(uint8_t, __thiscall, Vehicle_HasSirenModel, 0x004CB1F0, CVehicle* vehicle);
GAME_FN(void, __thiscall, Vehicle_4CD2A0, 0x004CD2A0, CVehicle* vehicle);
GAME_FN(CVehicle*, __cdecl, FindPlayerVehicle, 0x00446970, uintptr_t player);
GAME_FN(void, __thiscall, Automobile_4A8880, 0x004A8880, CAutomobile* automobile);
GAME_FN(void, __thiscall, Automobile_4A8A50, 0x004A8A50, CAutomobile* automobile);
GAME_FN(void, __thiscall, Automobile_4AD740, 0x004AD740, CAutomobile* automobile);
GAME_FN(void, __thiscall, Automobile_4A9F00, 0x004A9F00, CAutomobile* automobile);
// its damage manager (+0x580): a light's state (0: working)
GAME_FN(int32_t, __thiscall, Damage_GetLightStatus, 0x004C7DA0, void* damage, int light);

// the wheels
GAME_FN(CVector*, __thiscall, Entity_GetPosition, 0x00415460, CVehicle* entity);
GAME_FN(CVector*, __cdecl, CrossProduct, 0x00414730, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vehicle_ProcessWheelRotation, 0x004CB870, CVehicle* vehicle, int state, const CVector* fwd,
        const CVector* speed, float radius);
GAME_FN(uint8_t, __thiscall, Damage_GetWheelStatus, 0x004C7DE0, void* damage, int wheel);   // (1: burst)
GAME_FN(void, __thiscall, Matrix_SetRotateX, 0x00412CA0, Matrix* m, float angle);
GAME_FN(void, __thiscall, Matrix_SetRotate, 0x00412D00, Matrix* m, const CVector* angles);
GAME_FN(void, __thiscall, Matrix_UpdateRW, 0x00413A70, Matrix* m);

// effects
GAME_FN(void, __cdecl, Particles_Add, 0x00521080, int kind, const CVector* pos, const CVector* speed, float size,
        const uint8_t* colour, int a6, int a7);
GAME_FN(void, __cdecl, Coronas_Register, 0x005124D0, uintptr_t id, int r, int g, int b, int a, const CVector* pos,
        float size, float range, int type, int a10, int a11, int a12, float a13, int a14, int a15, float a16);
GAME_FN(void, __cdecl, Coronas_Off, 0x00511480, uintptr_t id, const CVector* pos, float range, float angle);
GAME_FN(void, __cdecl, LightShadow_Add, 0x00528800, float px, float py, float pz, float ux, float uy, float uz, float rx,
        float ry, float rz, float fx, float fy, float fz, int kind, int a14, int a15, int a16);
GAME_FN(void, __cdecl, Shadows_StoreCarLight, 0x00527280, CAutomobile* vehicle, uintptr_t id, void* texture,
        const CVector* pos, float fwdX, float fwdY, float sideX, float sideY, int r, int g, int b, float maxDist);
GAME_FN(void, __cdecl, PointLights_Add, 0x00522DB0, int type, float px, float py, float pz, float dx, float dy, float dz,
        float radius, float r, float g, float b, int fog, int shadows);
GAME_FN(void, __thiscall, LightShafts_Add, 0x00513260, void* shafts, const void* shaft);

CVector* EntityPos(CVehicle* v) {
    return v->matrix ? reinterpret_cast<CVector*>(&v->matrix->m[12]) : &v->placementPos;
}

uintptr_t Id(CAutomobile* self, int n) {
    return reinterpret_cast<uintptr_t>(self) + n;
}

bool LightWorks(CAutomobile* self, int light) {
    return Damage_GetLightStatus(&At<uint8_t>(self, 0x580), light) == 0;
}

int32_t VehicleClass(CAutomobile* self) {
    int32_t cls;
    return *Vehicle_GetVehicleClass(self, &cls);
}

uint8_t* ModelInfo(CAutomobile* self) {
    return modelInfoPtrs[self->modelIndex];
}

// a light's shadow on the ground (the handling's +0xD8 / +0xD9 plus `add`)
void LightShadow(CAutomobile* self, const CVector& pos, int handlingByte, int add) {
    float* right = Entity_GetMatrix(self);
    float* m = Entity_GetMatrix(self);
    CVector tmp;
    const CVector* fwd = Entity_GetForward(self, &tmp);
    int kind = uint8_t(At<uint8_t>(self->handling, handlingByte) + add);
    LightShadow_Add(pos.x, pos.y, pos.z, m[8], m[9], m[10], right[0], right[1], right[2], fwd->x, fwd->y, fwd->z, kind, 0,
                    0, 0);
}

// vf14 0x4B2460: Teleport: out of the world, placed and levelled, stopped, back in
void __fastcall Automobile_Teleport(CAutomobile* self, re::Edx, float x, float y, float z) {
    World_Remove(self);
    Placeable_SetPosition(self, x, y, z);
    float angle = float(3.1415927410125732 * 0.0 / 180.0);
    Placeable_SetOrientation(self, angle, angle, angle);
    self->moveSpeed = {0.0f, 0.0f, 0.0f};
    self->turnSpeed = {0.0f, 0.0f, 0.0f};
    Automobile_4A8880(self);
    Virtual<void>(self, 0x9C, Entity_GetMatrix(self));
    World_Add(self);
}

// ---- vf17, in parts ------------------------------------------------------------------

// a wheel's contact point (a colpoint at +0x6C4 + 0x28 each) put under its frame at the
// wheel's suspension length (+0x7AC.. times 1.5), its surface cleared
void PlaybackWheelPoint(CAutomobile* self, int frame, int length, int colPoint) {
    Matrix mat;
    mat.attached = nullptr;
    void* transform = Node_GetLocalTransform(At<void*>(self, frame));
    mat.attached = transform;
    Matrix_FromNode(&mat, transform);
    CVector v = {mat.m[12], mat.m[13], float(At<float>(self, length) * 1.5)};
    Vec4 out;
    Matrix_MultiplyPoint4(&out, Entity_GetMatrix(self), &v);
    At<CVector>(self, colPoint) = {out.x, out.y, out.z};
    At<uint8_t>(self, colPoint + 0x22) = 0;
}

// the exhaust smoke (the model's +0xAC: a second pipe mirrored with the handling's 0x100)
void Exhaust(CAutomobile* self, float fwdSpeed) {
    uint8_t* handling = self->handling;
    int32_t cls;
    if (!(self->flags33A & 2) || (At<uint32_t>(handling, 0xCC) & 0x100000) ||
        *Vehicle_GetVehicleClass(self, &cls) == 0xA || !(130.0f > fwdSpeed)) {
        return;
    }
    CVector pipe = At<CVector>(ModelInfo(self), 0xAC);
    if (!VectorNotEqual(&pipe, &vec0)) {
        return;
    }
    CVector speed = {0.0f, 0.0f, 0.0f};
    CVector speed2 = {0.0f, 0.0f, 0.0f};
    if (10.0f > fwdSpeed) {
        CVector dir = {-float(std::sin(double(self->steerAngle))), float(std::cos(double(self->steerAngle))), 0.0f};
        CVector turned;
        Matrix_Multiply3x3(&turned, Entity_GetMatrix(self), &dir);
        float r = RandomFloatInRange(-0.06f, -0.03f);
        speed.x = turned.x * r;
        speed.y = r * turned.y;
    } else {
        speed.x = self->moveSpeed.x;
        speed.y = self->moveSpeed.y;
    }
    struct { CVector v; float pad; } posPad16;   // (the game's transform writes a 4th float)
    CVector& pos = posPad16.v;
    Matrix_TransformPoint(&pos, Entity_GetMatrix(self), &pipe);
    Vec4 pos2 = {};
    if (At<uint32_t>(self->handling, 0xCC) & 0x100) {
        CVector mirrored = {-pipe.x, pipe.y, pipe.z};
        Matrix_MultiplyPoint4(&pos2, Entity_GetMatrix(self), &mirrored);
        speed2 = speed;
    }
    const CVector* p2 = reinterpret_cast<const CVector*>(&pos2);
    float gas = At<float>(self, 0x294);
    if (!(exhaustGasB < (gas + 1.100000023841858) * double(RandomFloatInRange(1.0f, 3.0f)))) {
        return;
    }
    uint8_t colour[4] = {0, 0, 0, 0};
    Particles_Add(4, &pos, &speed, 0.0f, colour, 0, 0);
    if (At<uint32_t>(self->handling, 0xCC) & 0x100) {
        Particles_Add(4, p2, &speed2, 0.0f, colour, 0, 0);
    }
    if (exhaustGasA < At<float>(self, 0x294) && self->unknown344 < 3) {
        if (Rand() & 1) {
            Particles_Add(4, &pos, &speed, 0.0f, colour, 0, 0);
        } else if (At<uint32_t>(self->handling, 0xCC) & 0x100) {
            Particles_Add(4, p2, &speed2, 0.0f, colour, 0, 0);
        }
    }
}

// whether its lights should be on: at night (from 20:xx / until 6:xx, by its own minute
// +0x10C), or in the weather's fog / rain above its own level
bool LightsWanted(CAutomobile* self) {
    uint8_t hour = clockHours;
    uint8_t minute = clockMinutes;
    uint8_t own = At<uint8_t>(self, 0x10C) & 0x3F;
    if (hour > 20) {
        return true;
    }
    if (hour > 19 && minute > own) {
        return true;
    }
    if (hour < 6) {
        return true;
    }
    if (hour < 7 && minute < own) {
        return true;
    }
    double level = double(At<uint16_t>(self, 0x10C)) / 50000.0;
    return lightsFogA > level || lightsFogB > level;
}

// its lights switched (flags33A 8); a parked car's only go off once the camera is 100
// away
void UpdateLightsState(CAutomobile* self) {
    uint8_t on = LightsWanted(self) ? 1 : 0;
    uint8_t flags = self->flags33A;
    uint8_t cur = (flags >> 3) & 1;
    if (on == cur) {
        return;
    }
    uint8_t status = self->typeAndStatus >> 3;
    if (status == 5) {
        return;
    }
    if (status != 4) {
        self->flags33A = uint8_t((flags & ~8) | (on << 3));
        return;
    }
    if (!cur) {
        return;
    }
    float dx = std::fabs(Camera_GetPosition(theCamera)->x - EntityPos(self)->x);
    float dy = std::fabs(Camera_GetPosition(theCamera)->y - EntityPos(self)->y);
    if (dy + dx > 100.0) {
        self->flags33A &= 0xF7;
    }
}

// the rear lights while its lights are off: dim, or brake / reversing lights (when the
// camera sees them) with their shadows
void RearLightsOff(CAutomobile* self) {
    const CVector& model = At<CVector>(ModelInfo(self), 0x70);
    CVector v = {model.x, float(model.y - 0.05000000074505806), model.z};
    Vec4 a;
    Matrix_MultiplyPoint4(&a, Entity_GetMatrix(self), &v);
    Vec4 b = a;
    float twice = model.x + model.x;
    CVector across, tmp;
    VectorScale(&across, reinterpret_cast<CVector*>(Entity_GetMatrix(self)), twice);
    Vector_SubAssign(&b, &tmp, &across);
    const CVector& posA = *reinterpret_cast<const CVector*>(&a);
    const CVector& posB = *reinterpret_cast<const CVector*>(&b);
    float brake = At<float>(self, 0x298);
    float gas = At<float>(self, 0x294);
    if (!(0.0f < brake) && !(0.0f > gas)) {
        if (LightWorks(self, 2)) {
            Coronas_Off(Id(self, 0xE), &posB, 50.0f, 0.0f);
        }
        if (LightWorks(self, 3)) {
            Coronas_Off(Id(self, 0xF), &posA, 50.0f, 0.0f);
        }
        return;
    }
    CVector toCar;
    const CVector* d = VectorSub(&tmp, EntityPos(self), Camera_GetPosition(theCamera));
    toCar = *d;
    Vector_Normalise(&toCar);
    float dot = DotProduct(&toCar, Entity_GetForward(self, &tmp));
    if (!(0.0f < dot)) {
        if (LightWorks(self, 2)) {
            Coronas_Off(Id(self, 0xE), &posB, 50.0f, 0.0f);
        }
        if (LightWorks(self, 3)) {
            Coronas_Off(Id(self, 0xF), &posA, 50.0f, 0.0f);
        }
        return;
    }
    int r = 0x78, g = 0x78, bl = 0x78, add = 4;   // reversing: white
    if (!(0.0f > At<float>(self, 0x294))) {
        g = 0;
        bl = 0;
        add = 8;   // braking: red
    }
    if (LightWorks(self, 2)) {
        Coronas_Register(Id(self, 0xE), r, g, bl, 0xFF, &posB, 0.9f, 50.0f, 0, 0, 0, 1, 0.0f, 1, 1, 0.25f);
    }
    if (LightWorks(self, 3)) {
        Coronas_Register(Id(self, 0xF), r, g, bl, 0xFF, &posA, 0.9f, 50.0f, 0, 0, 0, 1, 0.0f, 1, 1, 0.25f);
    }
    if (LightWorks(self, 2)) {
        LightShadow(self, posB, 0xD9, add);
    }
    if (LightWorks(self, 3)) {
        LightShadow(self, posA, 0xD9, add);
    }
}

// the camera's position (its node's world translation)
const CVector* CameraNodePos() {
    return reinterpret_cast<const CVector*>(static_cast<uint8_t*>(Node_GetLocalTransform(Node_6C82F0(cameraNode))) + 0x24);
}

// a light pair's world positions: `a` at the model's position, `b` mirrored across
void LightPair(CAutomobile* self, const CVector& model, Vec4* a, Vec4* b) {
    Matrix_MultiplyPoint4(a, Entity_GetMatrix(self), &model);
    *b = *a;
    float width = std::fabs(model.x + model.x);
    float* m = Entity_GetMatrix(self);
    CVector across = {m[0] * width, m[1] * width, m[2] * width};
    b->x = across.x + b->x;
    b->y = b->y + across.y;
    b->z = b->z + across.z;
}

// the lights on: head and tail coronas (brighter towards the camera, dazzling right in
// front), their shadows, the ground shadows and the point lights
void LightsOn(CAutomobile* self) {
    uint8_t* handling = self->handling;
    float angle = 0.0f;
    float facing = 0.0f;   // (the camera's direction against its forward, -1..1)
    CVector headModel = {};
    bool isTaxi = VehicleClass(self) == 0xA;
    if (!isTaxi) {
        CVector tmp;
        CVector dir = *VectorSub(&tmp, EntityPos(self), CameraNodePos());
        float length = Vector_Magnitude(&dir);
        if (length != 0.0f) {
            Vector_ScaleAssign(&dir, &tmp, 1.0f / length);
        } else {
            dir = {1.0f, 0.0f, 0.0f};
        }
        CVector fwd = *Entity_GetForward(self, &tmp);
        facing = dir.x * fwd.x + dir.y * fwd.y + dir.z * fwd.z;
        if (facing > 1.0f) {
            facing = 1.0f;
        } else if (-1.0f > facing) {
            facing = -1.0f;
        }
        angle = std::fabs(SafeAcos(facing));
        uint8_t* mi = ModelInfo(self);
        headModel = At<CVector>(mi, 0x64);
        Vec4 a, b;
        LightPair(self, headModel, &a, &b);
        const CVector& posA = *reinterpret_cast<const CVector*>(&a);
        const CVector& posB = *reinterpret_cast<const CVector*>(&b);
        const CVector* cam = CameraNodePos();
        const CVector* f = Entity_GetForward(self, &tmp);
        CVector d;
        const CVector* e = VectorSub2(&d, &posB, cam);
        float dotB = e->y * f->y + f->x * e->x + e->z * f->z;
        if (dotB < 0.0f && self != FindPlayerVehicle(0)) {
            float n = -facing;
            float size = float(n * 0.5 + 0.30000001192092896);
            float range = float((n + 1.0) * 0.699999988079071);
            bool police = (At<uint32_t>(handling, 0xCC) & 0x8000) != 0;
            if (facing < -0.9700000286102295 && length < 30.0) {
                int r = police ? 0x96 : 0xA0, g = police ? 0x96 : 0xA0, bl = police ? 0xC3 : 0x8C;
                if (LightWorks(self, 0)) {
                    Coronas_Register(Id(self, 6), r, g, bl, 0xFF, &posA, 0.65f, 45.0f, 3, 0, 0, 0, angle, 0, 1, 1.5f);
                }
                if (LightWorks(self, 1)) {
                    Coronas_Register(Id(self, 7), r, g, bl, 0xFF, &posB, 0.65f, 45.0f, 3, 0, 0, 0, angle, 0, 1, 1.5f);
                }
            }
            int rg, bl;
            if (police) {
                rg = uint8_t(int(size * 190.0));
                bl = uint8_t(int(size * 255.0));
            } else {
                rg = uint8_t(int(size * 210.0));
                bl = uint8_t(int(size * 195.0));
            }
            if (LightWorks(self, 0)) {
                Coronas_Register(Id(self, 0), rg, rg, bl, 0xFF, &posA, range, 50.0f, 7, 0, 0, 1, angle, 0, 1, 1.5f);
            }
            if (LightWorks(self, 1)) {
                Coronas_Register(Id(self, 1), rg, rg, bl, 0xFF, &posB, range, 50.0f, 7, 0, 0, 1, angle, 0, 1, 1.5f);
            }
        } else {
            if (LightWorks(self, 0)) {
                Coronas_Off(Id(self, 0), &posA, 50.0f, angle);
            }
            if (LightWorks(self, 1)) {
                Coronas_Off(Id(self, 1), &posB, 50.0f, angle);
            }
        }
        if (LightWorks(self, 0) && At<int32_t>(self, 0x90) == 0) {
            LightShadow(self, posA, 0xD8, 4);
        }
        if (LightWorks(self, 1) && At<int32_t>(self, 0x90) == 0) {
            LightShadow(self, posB, 0xD8, 4);
        }
        // the tail lights
        CVector tailModel = At<CVector>(ModelInfo(self), 0x70);
        Vec4 ta, tb;
        LightPair(self, tailModel, &ta, &tb);
        const CVector& tailA = *reinterpret_cast<const CVector*>(&ta);
        const CVector& tailB = *reinterpret_cast<const CVector*>(&tb);
        cam = CameraNodePos();
        f = Entity_GetForward(self, &tmp);
        e = VectorSub2(&d, &tailB, cam);
        float dotT = e->y * f->y + f->x * e->x + e->z * f->z;
        if (0.0f < dotT) {
            float bright = float(facing * 0.4000000059604645 + 0.4000000059604645);
            float range = float((facing * 0.25 + 1.0) * 0.20000000298023224);
            if (0.0f > At<float>(self, 0x294)) {
                bright = float(0.4000000059604645 + bright);
                range = float(range + 0.30000001192092896);
                int v = uint8_t(int(bright * 128.0));
                if (LightWorks(self, 2)) {
                    Coronas_Register(Id(self, 0xE), v, v, v, 0xFF, &tailA, range, 50.0f, 7, 0, 0, 1, angle, 0, 1, 1.5f);
                }
                if (LightWorks(self, 3)) {
                    Coronas_Register(Id(self, 0xF), v, v, v, 0xFF, &tailB, range, 50.0f, 7, 0, 0, 1, angle, 0, 1, 1.5f);
                }
            } else {
                if (0.0f < At<float>(self, 0x298)) {
                    bright = float(0.4000000059604645 + bright);
                    range = float(range + 0.30000001192092896);
                }
                if (LightWorks(self, 2)) {
                    int v = uint8_t(int(bright * 128.0));
                    Coronas_Register(Id(self, 0xE), v, 0, 0, 0xFF, &tailA, range, 50.0f, 7, 0, 0, 1, angle, 0, 1, 1.5f);
                }
                if (LightWorks(self, 3)) {
                    int v = uint8_t(int(bright * 128.0));
                    Coronas_Register(Id(self, 0xF), v, 0, 0, 0xFF, &tailB, range, 50.0f, 7, 0, 0, 1, angle, 0, 1, 1.5f);
                }
            }
        } else {
            if (LightWorks(self, 2)) {
                Coronas_Off(Id(self, 0xE), &tailA, 50.0f, angle);
            }
            if (LightWorks(self, 3)) {
                Coronas_Off(Id(self, 0xF), &tailB, 50.0f, angle);
            }
        }
        if (LightWorks(self, 2)) {
            LightShadow(self, tailA, 0xD9, 8);
        }
        if (LightWorks(self, 3)) {
            LightShadow(self, tailB, 0xD9, 8);
        }
    }
    // the lights' shadows on the ground ahead and behind
    if (VehicleClass(self) != 0xA) {
        CVector pos = *EntityPos(self);
        CVector tmp, tmp2;
        float fx = Entity_GetForward(self, &tmp)->x;
        float fy = Entity_GetForward(self, &tmp)->y;
        float lengthSqr = fy * fy + fx * fx;
        float k = lengthSqr;
        if (lengthSqr != 0.0f) {
            k = Fn_414C20(1.0f, lengthSqr);
        }
        float kx = k * fx;
        float ky = k * fy;
        // (the headlights' model y from above; 0 for a taxi, which never gets here)
        double ahead = headModel.y + 6.0;
        CVector v = {float(ahead * kx), float(ahead * ky), 1.0f};
        Vector_AddAssign(&pos, &tmp2, &v);
        if (LightWorks(self, 0) || LightWorks(self, 1)) {
            Shadows_StoreCarLight(self, Id(self, 0x16), texHeadlightShadow, &pos, float(kx * 5.0), float(ky * 5.0),
                                  float(ky * 3.5), float(kx * -3.5), 0x2D, 0x2D, 0x2D, 7.0f);
        }
        // (from the ahead position's y, not a length: as the game does)
        double back = headModel.y + 8.5 - pos.y;
        CVector w = {float(kx * back), float(back * ky), 0.0f};
        Vector_SubAssign(&pos, &tmp2, &w);
        if (LightWorks(self, 2) || LightWorks(self, 3)) {
            Shadows_StoreCarLight(self, Id(self, 0x19), texTaillightShadow, &pos, 3.0f, 0.0f, 0.0f, -3.0f, 8, 0, 0, 4.0f);
        }
    }
    // the point lights
    if (LightWorks(self, 0) || LightWorks(self, 1)) {
        const CVector& v = self->moveSpeed;
        int still = double(v.y) * v.y + double(v.x) * v.x < 0.20249998569488525 ? 1 : 0;
        const CVector* pos = EntityPos(self);
        CVector tmp;
        const CVector* fwd = Entity_GetForward(self, &tmp);
        PointLights_Add(1, pos->x, pos->y, pos->z, fwd->x, fwd->y, fwd->z, 10.0f, 1.0f, 1.0f, 1.0f, still, 0);
    }
    CVector tmp;
    const CVector* fwd = Entity_GetForward(self, &tmp);
    CVector off = {float(fwd->x * 2.5999999046325684), float(fwd->y * 2.5999999046325684), float(fwd->z * 2.5999999046325684)};
    CVector p = *VectorSub(&tmp, EntityPos(self), &off);
    if (LightWorks(self, 2) || LightWorks(self, 3)) {
        if (0.0f < At<float>(self, 0x298)) {
            PointLights_Add(0, p.x, p.y, p.z, vec0.x, vec0.y, vec0.z, 10.0f, 0.5f, 0.0f, 0.0f, 0, 0);
        } else {
            PointLights_Add(0, p.x, p.y, p.z, vec0.x, vec0.y, vec0.z, 7.0f, 0.3f, 0.0f, 0.0f, 0, 0);
        }
    }
}

// a light shaft (0x513260's description, 0x60 bytes)
struct LightShaft {
    CVector pos;         // +0x0
    CVector dir;         // +0xC
    float unknown18;     // +0x18 1
    float length;        // +0x1C 3 * intensity
    float unknown20;     // +0x20 6
    float intensity;     // +0x24
    float colours[12];   // +0x28 three RGBA
    float unknown58;     // +0x58
    uint8_t unknown5C;   // +0x5C 0
    uint8_t pad5D[3];
};
static_assert(sizeof(LightShaft) == 0x60);

// the siren's two lights, turning (+0x7D0 its time, 25 a turn): red then blue
void Siren(CAutomobile* self) {
    uint8_t* mi = ModelInfo(self);
    CVector modelB8 = At<CVector>(mi, 0xB8);
    CVector modelC4 = At<CVector>(mi, 0xC4);
    // (the blue light at the model's +0xB8, the red at +0xC4)
    struct { CVector v; float pad; } bluePad16;   // (the game's transform writes a 4th float)
    CVector& blue = bluePad16.v;
    struct { CVector v; float pad; } redPad16;
    CVector& red = redPad16.v;
    Matrix_TransformPoint(&blue, Entity_GetMatrix(self), &modelB8);
    Matrix_TransformPoint(&red, Entity_GetMatrix(self), &modelC4);
    float& time = At<float>(self, 0x7D0);
    if (25.0 < time) {
        time = float(time - 25.0);
    }
    float turn = float(time / 25.0);
    double phase = (double(turn) + turn) * 3.1415927410125732;
    float phaseF = float(phase);
    float intensity1 = float((float(std::sin(double(phaseF))) + 1.0) * 0.5);
    float quarter = float(phaseF + 1.5707963705062866);
    float intensity2 = float((float(std::cos(double(quarter))) + 1.0) * 0.5);
    double phase2 = phase + 3.1415927410125732;
    float phase2F = float(phase2);
    float c = float(std::cos(double(phase2F)));
    float s = float(std::sin(double(phase2F)));
    float* m = Entity_GetMatrix(self);
    CVector fwd = {m[4], m[5], m[6]};
    CVector dir = {c * fwd.x - s * fwd.y, s * fwd.x + c * fwd.y, fwd.z};
    Vector_Normalise2(&dir);
    PointLights_Add(0, red.x, red.y, red.z, 0.0f, 0.0f, 0.0f, float(intensity1 * 1.5), 10.0f, 0.0f, 0.0f, 0, 0);
    if (0.125f < intensity1) {
        PointLights_Add(1, red.x, red.y, red.z, dir.x, dir.y, dir.z, 7.0f, 10.0f, 0.0f, 0.0f, 0, 0);
        Coronas_Off(Id(self, 0x11), &red, 50.0f, 0.0f);
        Coronas_Off(Id(self, 0x12), &red, 50.0f, 0.0f);
        LightShaft shaft = {};
        shaft.pos = red;
        shaft.dir = dir;
        shaft.unknown18 = 1.0f;
        shaft.length = float(intensity1 * 3.0);
        shaft.unknown20 = 6.0f;
        shaft.intensity = intensity1;
        const float colours[12] = {0.25f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, intensity1};
        for (int i = 0; i < 12; i++) {
            shaft.colours[i] = colours[i];
        }
        shaft.unknown58 = intensity1;
        LightShafts_Add(lightShafts, &shaft);
    }
    int alpha = uint8_t(int(intensity1 * 255.0));
    Coronas_Register(Id(self, 0x11), 0xFF, 0, 0, alpha, &red, 0.5f, 50.0f, 0, 0, 0, 0, 0.0f, 0, 1, 1.5f);
    Coronas_Register(Id(self, 0x12), 0xFF, 0, 0, alpha, &red, 0.5f, 50.0f, 0, 0, 0, 0, 0.0f, 0, 1, 1.5f);
    float phase3 = float(phase2 + 1.5707963705062866);
    c = float(std::cos(double(phase3)));
    s = float(std::sin(double(phase3)));
    m = Entity_GetMatrix(self);
    fwd = {m[4], m[5], m[6]};
    dir = {c * fwd.y - s * fwd.x, -fwd.x * c - fwd.y * s, fwd.z};
    Vector_Normalise2(&dir);
    PointLights_Add(0, blue.x, blue.y, blue.z, 0.0f, 0.0f, 0.0f, float(intensity2 * 1.5), 0.0f, 0.0f, 29.44598388671875f, 0,
                    0);
    if (0.125f < intensity2) {
        PointLights_Add(1, blue.x, blue.y, blue.z, dir.x, dir.y, dir.z, 7.0f, 0.0f, 0.0f, 29.44598388671875f, 0, 0);
        Coronas_Off(Id(self, 0x13), &blue, 50.0f, 0.0f);
        Coronas_Off(Id(self, 0x14), &blue, 50.0f, 0.0f);
        LightShaft shaft = {};
        shaft.pos = blue;
        shaft.dir = dir;
        shaft.unknown18 = 1.0f;
        shaft.length = float(intensity2 * 3.0);
        shaft.unknown20 = 6.0f;
        shaft.intensity = intensity2;
        // (the last alpha is the red light's intensity: as the game has it)
        const float colours[12] = {0.0f, 0.0f, 0.25f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, intensity1};
        for (int i = 0; i < 12; i++) {
            shaft.colours[i] = colours[i];
        }
        shaft.unknown58 = intensity2;
        LightShafts_Add(lightShafts, &shaft);
    }
    alpha = uint8_t(int(intensity2 * 255.0));
    Coronas_Register(Id(self, 0x13), 0, 0, 0xFF, alpha, &blue, 0.6f, 50.0f, 0, 0, 0, 0, 0.0f, 0, 1, 1.5f);
    Coronas_Register(Id(self, 0x14), 0, 0, 0xFF, alpha, &blue, 0.5f, 50.0f, 0, 0, 0, 0, 0.0f, 0, 1, 1.5f);
    time = timeStep + time;
}

// vf17's first half: up to the wheels
void PreRenderLights(CAutomobile* self) {
    CVector tmp;
    const CVector* fwd = Entity_GetForward(self, &tmp);
    const CVector& v = self->moveSpeed;
    float fwdSpeed = v.y * fwd->y + v.x * fwd->x + v.z * fwd->z;
    fwdSpeed = fwdSpeed / speedScale;
    if ((self->typeAndStatus & 0xF8) == 0x10) {
        PlaybackWheelPoint(self, 0x680, 0x7B8, 0x73C);
        PlaybackWheelPoint(self, 0x68C, 0x7B0, 0x6EC);
        PlaybackWheelPoint(self, 0x678, 0x7B4, 0x714);
        PlaybackWheelPoint(self, 0x684, 0x7AC, 0x6C4);
    }
    Automobile_4A8A50(self);
    if (VehicleClass(self) == 0xA && Entity_GetIsOnScreen(self)) {
        Automobile_4AD740(self);
    }
    if (Seats_GetOccupant(&self->seats, 0) && At<int32_t>(Seats_GetOccupant(&self->seats, 0), 0x1310) == 0xD) {
        Automobile_4A9F00(self);
    }
    Exhaust(self, fwdSpeed);
    if (VehicleClass(self) == 7 || VehicleClass(self) == 6) {
        return;
    }
    UpdateLightsState(self);
    uint8_t status = self->typeAndStatus >> 3;
    if ((self->flags33A & 2) && (self->flags33A & 8)) {
        LightsOn(self);
    } else if (status != 4 && status != 5 && VehicleClass(self) != 0xA) {
        RearLightsOff(self);
    }
    status = self->typeAndStatus >> 3;
    if (Vehicle_HasSirenModel(self) == 1 && self->siren == 1 && status != 5 && (status != 4 || (self->flags33C & 0x20))) {
        Siren(self);
    }
}


// a wheel's frame: under the suspension's position (a bike-like class 7 kept within the
// handling's +0xB8 less 0.5 and +0xB8), turned (a burst tyre wobbles), scaled to the
// wheel's width and radius
struct WheelTurn {
    bool rotateX;    // SetRotateX(angles.x), else SetRotate(angles)
    CVector angles;
};

void WheelFrame(CAutomobile* self, int frame, int suspension, float minZ, float maxZ, float width, float radius,
                const WheelTurn& turn) {
    Matrix mat;
    mat.attached = nullptr;
    void* transform = Node_GetLocalTransform(At<void*>(self, frame));
    mat.attached = transform;
    Matrix_FromNode(&mat, transform);
    CVector pos = {mat.m[12], mat.m[13], At<float>(self, suspension)};
    if (VehicleClass(self) == 7) {
        float z = pos.z;
        if (z < minZ) {
            z = minZ;
        }
        if (!(z <= maxZ)) {
            z = maxZ;
        }
        pos.z = z;
    }
    if (turn.rotateX) {
        Matrix_SetRotateX(&mat, turn.angles.x);
    } else {
        Matrix_SetRotate(&mat, &turn.angles);
    }
    float* m = mat.m;
    m[0] = width * m[0];
    m[1] = m[1] * radius;
    m[2] = m[2] * radius;
    m[4] = width * m[4];
    m[5] = m[5] * radius;
    m[6] = m[6] * radius;
    m[8] = m[8] * width;
    m[9] = m[9] * radius;
    m[10] = radius * m[10];
    m[12] = m[12] + pos.x;
    m[13] = pos.y + m[13];
    m[14] = pos.z + m[14];
    Matrix_UpdateRW(&mat);
}

bool WheelBurst(CAutomobile* self, int wheel) {
    return Damage_GetWheelStatus(&At<uint8_t>(self, 0x580), wheel) == 1;
}

// the left wheels (their spin +0x7A8 / 0x7A4 as is)
WheelTurn LeftTurn(CAutomobile* self, int wheel, int spin) {
    float x = At<float>(self, spin);
    if (WheelBurst(self, wheel)) {
        return {false, {x, 0.0f, float(float(std::sin(double(x))) * 0.30000001192092896)}};
    }
    return {true, {x, 0.0f, 0.0f}};
}

// the right wheels, turned round (their spin negated, pi about z)
WheelTurn RightTurn(CAutomobile* self, int wheel, int spin, float steer) {
    float x = -At<float>(self, spin);
    if (WheelBurst(self, wheel)) {
        return {false, {x, 0.0f, float(float(std::sin(double(x))) * 0.30000001192092896 + (steer + 3.1415927410125732))}};
    }
    return {false, {x, 0.0f, float(steer + 3.1415927410125732)}};
}

// vf17's second half: the wheels' speed at their contact points and spin, then their
// frames
void PreRenderWheels(CAutomobile* self) {
    Vehicle_4CD2A0(self);
    uint8_t* mi = ModelInfo(self);
    CVector offsets[4] = {};
    CVector contactSpeeds[4] = {};
    float s = float(std::sin(double(self->steerAngle)));
    float c = float(std::cos(double(self->steerAngle)));
    CVector steerDir = {-s, c, 0.0f};
    struct { CVector v; float pad; } steeredPad16;   // (the game's transform writes a 4th float)
    CVector& steered = steeredPad16.v;
    Matrix_Multiply3x3b(&steered, Entity_GetMatrix(self), &steerDir);
    CVector forward;
    Entity_GetForward(self, &forward);
    for (int i = 0; i < 4; i++) {
        if (!(0.0f < At<float>(self, 0x784 + i * 4))) {
            continue;
        }
        CVector tmp, tmp2;
        offsets[i] = *VectorSub(&tmp, &At<CVector>(self, 0x6C4 + i * 0x28), Entity_GetPosition(self));
        CVector off = offsets[i];
        const CVector* cross = CrossProduct(&tmp, &self->turnSpeed, &off);
        contactSpeeds[i] = *VectorAdd(&tmp2, cross, &self->moveSpeed);
        float step = timeStep;
        float brake = float(double(At<float>(self->handling, 0x90)) * At<float>(self, 0x298) * step);
        // (at rest: its contact speed taken as none)
        if ((At<float>(self, 0x294) == 0.0f || At<float>(self, 0x29C) == 0.0f) && !(0.01f < brake) &&
            0.005f > std::fabs(self->moveSpeed.x) && 0.005f > std::fabs(self->moveSpeed.y)) {
            float z = std::fabs(self->moveSpeed.z);
            double limit = double(Virtual<float>(self, 0x8C)) * step + 0.020010000094771385;
            if (z < limit && 1.0f > At<float>(self, 0x774) && 1.0f > At<float>(self, 0x778) &&
                1.0f > At<float>(self, 0x77C) && 1.0f > At<float>(self, 0x780)) {
                contactSpeeds[i] = vec0;
            }
        }
        const CVector* dir = (i == 0 || i == 2) ? &steered : &forward;
        float radius = float(float(At<float>(mi, 0x58)) * 0.5);
        float spin = Vehicle_ProcessWheelRotation(self, At<int32_t>(self, 0x850 + i * 4), dir, &contactSpeeds[i], radius);
        At<float>(self, 0x7BC + i * 4) = spin;
        At<float>(self, 0x79C + i * 4) = At<float>(self, 0x79C + i * 4) + spin;
    }
    float radius = At<float>(mi, 0x58);
    uint32_t flags = At<uint32_t>(self->handling, 0xCC);
    float frontWidth = radius;
    float rearWidth = radius;
    if (flags & 0x1000000) {
        frontWidth = float(1.149999976158142 * radius);
    }
    if (flags & 0x2000000) {
        rearWidth = float(radius * 0.699999988079071);
    }
    float maxZ = At<float>(self->handling, 0xB8);
    float minZ = float(maxZ - 0.5);
    WheelFrame(self, 0x680, 0x7B8, minZ, maxZ, frontWidth, radius, LeftTurn(self, 3, 0x7A8));
    WheelFrame(self, 0x68C, 0x7B0, minZ, maxZ, frontWidth, radius, RightTurn(self, 1, 0x7A0, 0.0f));
    if (At<void*>(self, 0x67C)) {
        WheelFrame(self, 0x67C, 0x7B8, minZ, maxZ, frontWidth, radius, LeftTurn(self, 3, 0x7A8));
    }
    if (At<void*>(self, 0x688)) {
        WheelFrame(self, 0x688, 0x7B0, minZ, maxZ, frontWidth, radius, RightTurn(self, 1, 0x7A0, 0.0f));
    }
    // the steered wheels (turned less on the outside of the turn)
    float steer = self->steerAngle;
    if (0.0f < self->steerAngle) {
        steer = float(steer * 0.6000000238418579);
    }
    float x = At<float>(self, 0x7A4);
    WheelTurn turn = {false, {x, 0.0f, steer}};
    if (WheelBurst(self, 2)) {
        turn.angles.z = float(float(std::sin(double(x))) * 0.30000001192092896 + steer);
    }
    WheelFrame(self, 0x678, 0x7B4, minZ, maxZ, rearWidth, radius, turn);
    if (0.0f > self->steerAngle) {
        steer = float(steer * 0.6000000238418579);
    }
    WheelFrame(self, 0x684, 0x7AC, minZ, maxZ, rearWidth, radius, RightTurn(self, 0, 0x79C, steer));
}

// vf17 0x4B2540: PreRender
void __fastcall Automobile_PreRender(CAutomobile* self) {
    PreRenderLights(self);
    PreRenderWheels(self);
}

} // namespace

// ---- registration ---------------------------------------------------------------------

#define AUTOPC_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

AUTOPC_REVERSED("CAutomobile::Teleport", 0x004B2460, &Automobile_Teleport);
AUTOPC_REVERSED("CAutomobile::PreRender", 0x004B2540, &Automobile_PreRender);
