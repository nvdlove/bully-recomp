// CAutomobile (vtable 0x905D04): rendering (vf18: the colours set, the wheels' rotations
// handed to the model, the go-kart drawn tilted by its steering and flashing while
// wrecked), the wheel collision spheres (vf49, GTA's SetUpWheelColModel) and being set
// down on the ground at a heading (vf53).

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

// the game's CMatrix: right, forward, up, position (4 floats each), the attached
// NiTransform
struct Matrix {
    float m[16];
    re::Ptr32<void> attached;
};
static_assert(sizeof(Matrix) == 0x44);

GAME_ARRAY(uint8_t*, modelInfoPtrs, 0x00C67738);   // CModelInfo::ms_modelInfoPtrs
GAME_VAR(uint8_t, g_cutsceneRunning, 0x020C5BE4);
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);
GAME_VAR(int32_t, g_cameraMode, 0x00C3CCF8);
GAME_VAR(float, g_1B64874, 0x01B64874);
inline void* const TheRenderer = reinterpret_cast<void*>(0x00D140B8);

GAME_FN(void*, __thiscall, Seats_GetOccupant, 0x004CE560, void* seats, uint32_t seat);
GAME_FN(int32_t*, __thiscall, Vehicle_GetVehicleClass, 0x004CAF30, void* vehicle, int32_t* out);
GAME_FN(void, __thiscall, VehicleModel_SetColour, 0x00535680, void* modelInfo, int colour1, int colour2, void* rwObject);
GAME_FN(void, __thiscall, VehicleModel_GetFramePosition, 0x00534CD0, void* modelInfo, int frame, Vec* out);
GAME_FN(void, __thiscall, Renderer_5EE570, 0x005EE570, void* renderer, int);
GAME_FN(void, __thiscall, Renderer_8A20A0, 0x008A20A0, void* renderer, float);
GAME_FN(void, __thiscall, EmptyThiscall, 0x0049A650, void* self);
GAME_FN(Matrix*, __thiscall, Entity_GetMatrix, 0x008B6120, void* entity);
GAME_FN(void, __thiscall, Entity_SetMatrix, 0x0046DE90, void* entity, const Matrix* matrix, int);
GAME_FN(void, __thiscall, Entity_UpdateRwFrame, 0x00465760, void* entity);
GAME_FN(void, __thiscall, Entity_Render, 0x00466D40, void* entity);
GAME_FN(void, __thiscall, Entity_SetPosition, 0x0041C4E0, void* entity, float x, float y, float z);
GAME_FN(uint8_t*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(void, __thiscall, Matrix_Copy, 0x00412A10, Matrix* m, const Matrix* from);
GAME_FN(void, __thiscall, Matrix_FromTransform, 0x00412A70, Matrix* m, void* transform);
GAME_FN(void, __thiscall, Matrix_Assign, 0x00412720, Matrix* m, const Matrix* from);
GAME_FN(void, __thiscall, Matrix_SetRotateY, 0x00412CC0, Matrix* m, float angle);
GAME_FN(void, __thiscall, Matrix_SetRotateZOnly, 0x00412C20, void* m, float angle);
GAME_FN(Matrix*, __cdecl, Matrix_Multiply, 0x004138D0, Matrix* out, const Matrix* a, const Matrix* b);
GAME_FN(void, __thiscall, Matrix_UpdateRW, 0x00413A70, Matrix* m);
GAME_FN(void, __cdecl, SetFrameRotation, 0x00411700, void* rwObject, int frame, const float* rotation, int, int, int);
GAME_FN(void, __cdecl, FreeArray, 0x005EEA60, void* p);
GAME_FN(void*, __cdecl, AllocArray, 0x005EEA50, size_t size, int);
GAME_FN(void, __thiscall, ColSphere_Set, 0x0056EBA0, void* sphere, float radius, const Vec* center, int surface, int piece);
GAME_FN(float, __cdecl, LimitAngle, 0x00432EE0, float angle);
GAME_FN(float, __cdecl, FindGroundZ2D, 0x004634A0, float x, float y, int);
GAME_FN(float, __cdecl, FindGroundZFor3DCoord, 0x004635A0, float x, float y, float z, uint8_t* found, int);
GAME_FN(float, __thiscall, Vec_Normalise, 0x004294A0, float* v);

// a wheel frame's rotation handed to the model (the matrix's 3x3, by columns)
void SetWheelRotation(CAutomobile* self, int frameSlot, int frameId, Matrix* scratch, Matrix* rotation) {
    void* node = self->frames[frameSlot];
    if (!node) {
        return;
    }
    void* transform = GetLocalTransform(node);
    scratch->attached = transform;
    Matrix_FromTransform(scratch, transform);
    Matrix_Assign(rotation, scratch);
    const float* m = rotation->m;
    float r[9] = {m[0], m[4], m[8], m[1], m[5], m[9], m[2], m[6], m[10]};
    SetFrameRotation(self->rwObject, frameId, r, 1, 0, 0);
}

} // namespace

namespace c09automodel {

// 0x4AC110 (vf18): Render. Not in cutscenes for an empty bike or class 6 (unless it's a
// mission car); a wrecked go-kart flashes; the colours set, the four wheels' rotations
// (frames 0xB, 0xF, 0xC, 0x10), then CEntity's. The go-kart is drawn tilted by half its
// steering and raised 0.2, its matrix put back after
void __fastcall Render(CAutomobile* self, re::Edx) {
    int32_t vehicleClass;
    if (!Seats_GetOccupant(&self->seats, 0) && (self->typeAndStatus & 0xF8) != 0x38 && g_cutsceneRunning) {
        if (*Vehicle_GetVehicleClass(self, &vehicleClass) == 7) {
            return;
        }
        if (*Vehicle_GetVehicleClass(self, &vehicleClass) == 6) {
            return;
        }
    }
    if (*Vehicle_GetVehicleClass(self, &vehicleClass) == 10) {
        uint32_t wrecked = uint32_t(self->unknown348);
        uint32_t now = timeInMs;
        if (wrecked + 5000 > now && wrecked + 0x9C4 < now && int8_t(uint8_t(now)) < 0) {
            return;
        }
        g_1B64874 = 10000.0f;
    }
    VehicleModel_SetColour(modelInfoPtrs[self->modelIndex], self->colour1, self->colour2, self->rwObject);
    Matrix d, e;
    if (*Vehicle_GetVehicleClass(self, &vehicleClass) != 10) {
        Renderer_5EE570(TheRenderer, 7);
        e.attached = nullptr;
        SetWheelRotation(self, 11, 0xB, &d, &e);
        SetWheelRotation(self, 15, 0xF, &d, &e);
        SetWheelRotation(self, 12, 0xC, &d, &e);
        SetWheelRotation(self, 16, 0x10, &d, &e);
        Entity_Render(self);
        return;
    }
    if (g_cameraMode == 0xE) {
        Renderer_8A20A0(TheRenderer, 1.0f);
    }
    Matrix saved;
    Matrix_Copy(&saved, Entity_GetMatrix(self));
    Matrix tilt, product, tilted;
    tilted.attached = nullptr;
    tilt.attached = nullptr;
    float angle = float(-double(self->steerAngle) * 0.5);
    Matrix_SetRotateY(&tilt, angle);
    Matrix_Assign(&tilted, Matrix_Multiply(&product, &saved, &tilt));
    tilted.m[12] = tilted.m[12] + 0.0f;
    tilted.m[13] = 0.0f + tilted.m[13];
    tilted.m[14] = float(double(tilted.m[14]) + double(0.2f));
    Entity_SetMatrix(self, &tilted, 0);
    Matrix_UpdateRW(Entity_GetMatrix(self));
    Entity_UpdateRwFrame(self);
    Renderer_5EE570(TheRenderer, 7);
    e.attached = nullptr;
    SetWheelRotation(self, 11, 0xB, &d, &e);
    SetWheelRotation(self, 15, 0xF, &d, &e);
    SetWheelRotation(self, 12, 0xC, &d, &e);
    SetWheelRotation(self, 16, 0x10, &d, &e);
    Entity_Render(self);
    EmptyThiscall(TheRenderer);
    Entity_SetMatrix(self, &saved, 0);
}

// 0x4AC830 (vf49): the wheel col model: eight spheres (surface 0x26), one on each
// wheel (half the wheel size, raised 0.35) and one between each pair (three quarters
// the size, raised by 0.7 of it and 0.35, pulled in to 0.6 across); the model's box
// copied to `col`
uint8_t __fastcall SetUpWheelColModel(CAutomobile* self, re::Edx, uint8_t* col) {
    uint8_t* mi = modelInfoPtrs[self->modelIndex];
    uint8_t* modelCol = At<uint8_t*>(mi, 0xC);
    uint8_t* data = At<uint8_t*>(modelCol, 0x38);
    At<int8_t>(data, 0x1E) = 8;
    FreeArray(At<void*>(data, 0));
    At<void*>(data, 0) = AllocArray(size_t(At<int8_t>(data, 0x1E) * 0x14), 0);
    for (int i = 0; i < 12; i++) {
        At<uint32_t>(col, i * 4) = At<uint32_t>(modelCol, i * 4);
    }
    Matrix m0, m1, m2, m3;
    m0.attached = m1.attached = m2.attached = m3.attached = nullptr;
    void* t;
    t = GetLocalTransform(self->frames[4]);
    m0.attached = t;
    Matrix_FromTransform(&m0, t);
    t = GetLocalTransform(self->frames[6]);
    m1.attached = t;
    Matrix_FromTransform(&m1, t);
    t = GetLocalTransform(self->frames[1]);
    m2.attached = t;
    Matrix_FromTransform(&m2, t);
    t = GetLocalTransform(self->frames[3]);
    m3.attached = t;
    Matrix_FromTransform(&m3, t);
    const Vec* c0 = reinterpret_cast<Vec*>(&m0.m[12]);
    const Vec* c1 = reinterpret_cast<Vec*>(&m1.m[12]);
    const Vec* c2 = reinterpret_cast<Vec*>(&m2.m[12]);
    const Vec* c3 = reinterpret_cast<Vec*>(&m3.m[12]);
    const Vec* wheels[4] = {c0, c1, c2, c3};
    for (int i = 0; i < 4; i++) {
        float radius = float(double(At<float>(mi, 0x58)) * 0.5);
        uint8_t* sphere = At<uint8_t*>(data, 0) + i * 0x14;
        ColSphere_Set(sphere, radius, wheels[i], 0x26, 0);
        At<float>(sphere, 8) = float(double(At<float>(sphere, 8)) + double(0.35f));
    }
    const Vec* pairs[4][2] = {{c2, c0}, {c1, c0}, {c3, c2}, {c3, c1}};
    for (int i = 0; i < 4; i++) {
        Vec sum = {pairs[i][0]->x + pairs[i][1]->x, pairs[i][0]->y + pairs[i][1]->y, pairs[i][0]->z + pairs[i][1]->z};
        Vec mid = {float(double(sum.x) * 0.5), float(double(sum.y) * 0.5), float(0.5 * sum.z)};
        uint8_t* sphere = At<uint8_t*>(data, 0) + (4 + i) * 0x14;
        ColSphere_Set(sphere, float(double(At<float>(mi, 0x58)) * 0.75), &mid, 0x26, 0);
        sphere = At<uint8_t*>(data, 0) + (4 + i) * 0x14;
        At<float>(sphere, 8) = float(double(At<float>(mi, 0x58)) * double(0.7f) + double(0.35f) + At<float>(sphere, 8));
        sphere = At<uint8_t*>(data, 0) + (4 + i) * 0x14;
        At<float>(sphere, 0) = float(double(At<float>(sphere, 0)) * double(0.6f));
    }
    return 1;
}

// 0x4ACCD0 (vf53): set down at (x, y) facing the heading: its wheels found on the
// ground and the car laid along them (the ride height +0x7F8 over the ground); where
// there's no ground just placed (at z; with no ground under the point at all, z as
// given too)
void __fastcall PlaceOnGround(CAutomobile* self, re::Edx, float x, float y, float z, float heading) {
    uint8_t* p = reinterpret_cast<uint8_t*>(self);
    uint8_t* mi = modelInfoPtrs[self->modelIndex];
    float h = LimitAngle(heading);
    Matrix m;
    m.attached = nullptr;
    m.m[12] = x;
    m.m[13] = y;
    m.m[14] = 0.0f;
    m.m[15] = 0.0f;   // (stack garbage in the game)
    Matrix_SetRotateZOnly(&m, h);
    float ground = FindGroundZ2D(x, y, 1);
    if (ground == 20.0f) {
        Entity_SetPosition(self, x, y, z);
        if (self->matrix) {
            Matrix_SetRotateZOnly(self->matrix, h);
        }
        At<float>(p, 0x10) = h;
        return;
    }
    float w[4][4];
    for (int i = 0; i < 4; i++) {
        VehicleModel_GetFramePosition(mi, i, reinterpret_cast<Vec*>(w[i]));
    }
    float top = float(double(z) + 1.0);
    for (uint8_t i = 0; i < 4; i++) {
        float* v = w[i];
        float nx = float(((double(v[1]) * m.m[4] + double(v[0]) * m.m[0]) + double(v[2]) * m.m[8]) + m.m[12]);
        float ny = float(((double(v[0]) * m.m[1] + double(v[1]) * m.m[5]) + double(v[2]) * m.m[9]) + m.m[13]);
        float nz = float(((double(v[0]) * m.m[2] + double(v[1]) * m.m[6]) + double(v[2]) * m.m[10]) + m.m[14]);
        v[0] = nx;
        v[1] = ny;
        v[2] = nz;
        uint8_t found = 0;
        float g = FindGroundZFor3DCoord(v[0], v[1], top, &found, 1);
        if (!found) {
            if (self->matrix) {
                self->matrix->m[12] = x;
                self->matrix->m[13] = y;
                self->matrix->m[14] = z;
            }
            self->placementPos = {x, y, z};
            if (self->matrix) {
                Matrix_SetRotateZOnly(self->matrix, h);
            }
            At<float>(p, 0x10) = h;
            return;
        }
        v[2] = g;
    }
    float along[4] = {w[0][0] - w[1][0], w[0][1] - w[1][1], w[0][2] - w[1][2], 0.0f};
    Vec_Normalise(along);
    float side[4] = {float(double(along[1]) - double(along[2]) * 0.0), float(double(along[2]) * 0.0 - along[0]),
                     float(double(along[0]) * 0.0 - double(along[1]) * 0.0), 0.0f};
    Vec_Normalise(side);
    float up[4] = {float(double(side[1]) * along[2] - double(side[2]) * along[1]),
                   float(double(side[2]) * along[0] - double(side[0]) * along[2]),
                   float(double(side[0]) * along[1] - double(side[1]) * along[0]), 0.0f};
    Vec_Normalise(up);
    Matrix n;
    n.m[0] = side[0];
    n.m[1] = side[1];
    n.m[2] = side[2];
    n.m[3] = 0.0f;
    n.m[4] = along[0];
    n.m[5] = along[1];
    n.m[6] = along[2];
    n.m[7] = 0.0f;
    n.m[8] = up[0];
    n.m[9] = up[1];
    n.m[10] = up[2];
    n.m[11] = 0.0f;
    n.m[12] = x;
    n.m[13] = y;
    n.m[14] = float(double(At<float>(p, 0x7F8)) + ground);
    n.m[15] = 0.0f;   // (the fourth floats are stack garbage in the game)
    n.attached = nullptr;
    Entity_SetMatrix(self, &n, 0);
}

} // namespace c09automodel

// ---- registration ---------------------------------------------------------------------

#define AUTOMODEL_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

AUTOMODEL_REVERSED("CAutomobile::Render", 0x004AC110, &c09automodel::Render);
AUTOMODEL_REVERSED("CAutomobile::SetUpWheelColModel", 0x004AC830, &c09automodel::SetUpWheelColModel);
AUTOMODEL_REVERSED("CAutomobile::PlaceOnGround", 0x004ACCD0, &c09automodel::PlaceOnGround);
