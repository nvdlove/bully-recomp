// CAutomobile (vtable 0x905D04): the exhaust effects (0x4AD740): its four effect slots
// (+0x870, 8 bytes each, their types at +0x890 / +0x894) placed at the model's exhaust
// (+0xAC; the player's car has a second, mirrored), stretched by the throttle, the
// speed and a race start's boost, all off for a stopped wreck.

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

struct Matrix {
    float m[16];
    re::Ptr32<void> attached;
};
static_assert(sizeof(Matrix) == 0x44);

// what an effect is placed with (an NiTransform: a 3x3 rotation, the translation, the
// scale)
struct EffectTransform {
    float rot[9];
    Vec trans;
    float scale;
};
static_assert(sizeof(EffectTransform) == 0x34);

GAME_ARRAY(uint8_t*, modelInfoPtrs, 0x00C67738);   // CModelInfo::ms_modelInfoPtrs
GAME_VAR(uint16_t, g_exhaustEffectA, 0x020C9000);
GAME_VAR(uint16_t, g_exhaustEffectB, 0x020C8FFC);
GAME_VAR(void*, g_effects, 0x00B6F1D4);
GAME_VAR(uint8_t*, g_race, 0x020C8704);
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);
GAME_VAR(int32_t, g_cameraMode, 0x00C3CCF8);

GAME_FN(void, __thiscall, Effect_Kill, 0x00668BA0, void* effect);
GAME_FN(void, __thiscall, Effect_SetTransform, 0x00668EC0, void* effect, const EffectTransform* transform);
GAME_FN(void, __thiscall, Effects_Start, 0x00668290, void* manager, uint32_t* type, void* effect, int);
GAME_FN(void*, __thiscall, Seats_GetOccupant, 0x004CE560, void* seats, uint32_t seat);
GAME_FN(Matrix*, __thiscall, Entity_GetMatrix, 0x008B6120, void* entity);
GAME_FN(void, __thiscall, Matrix_SetRotateY, 0x00412CC0, Matrix* m, float angle);
GAME_FN(void, __thiscall, Matrix_Assign, 0x00412720, Matrix* m, const Matrix* from);
GAME_FN(Matrix*, __cdecl, Matrix_Multiply, 0x004138D0, Matrix* out, const Matrix* a, const Matrix* b);
// (0x0041A9A0 stores 16 bytes through `out`: called through re::CallVec16)
static inline Vec* MultiplyPoint(Vec* out, const Matrix* m, const Vec* v) { return re::CallVec16<0x0041A9A0, Vec*>(out, m, v); }
GAME_FN(uint8_t*, __thiscall, Race_GetRacer, 0x0070D390, void* race, void* ped);
GAME_FN(float, __thiscall, Vec_Magnitude, 0x0040D9F0, const Vec* v);
GAME_FN(void, __cdecl, Pad_Rumble, 0x00738680, int pad, uint32_t duration, int strength, int);

bool DrivenByPlayer(CAutomobile* self) {
    void* seats = &self->seats;
    return Seats_GetOccupant(seats, 0) && At<int32_t>(Seats_GetOccupant(seats, 0), 0x1310) == 0xD;
}
float SpeedSq(CAutomobile* self) {
    const veh::CVector& ms = self->moveSpeed;
    return float((double(ms.y) * ms.y + double(ms.x) * ms.x) + double(ms.z) * ms.z);
}
int32_t ThrottleSign(CAutomobile* self) {
    return 0.0f > At<float>(self, 0x294) ? -1 : 1;
}

// the pair of effects on one exhaust: the first stretched by `stretch`, the second by
// the speed
void PlaceExhaust(CAutomobile* self, EffectTransform* t, void* first, void* second, float stretch, const float* right,
                  const float* back) {
    t->rot[0] = float(double(stretch) * right[0]);
    t->rot[3] = float(double(right[1]) * stretch);
    t->rot[6] = float(double(stretch) * right[2]);
    t->rot[2] = float(double(stretch) * back[0]);
    t->rot[5] = float(double(back[1]) * stretch);
    t->rot[8] = float(double(stretch) * back[2]);
    Effect_SetTransform(first, t);
    float length = float(double(SpeedSq(self)) * ThrottleSign(self) * 2.0 + double(0.6f));
    t->rot[2] = float(double(length) * back[0]);
    t->rot[5] = float(double(back[1]) * length);
    t->rot[8] = float(double(length) * back[2]);
    float width = float(double(SpeedSq(self)) * ThrottleSign(self) * 1.5 + 1.0);
    t->rot[0] = float(double(width) * right[0]);
    t->rot[3] = float(double(right[1]) * width);
    t->rot[6] = float(double(width) * right[2]);
    Effect_SetTransform(second, t);
}

} // namespace

namespace c09autoexhaust {

// 0x4AD740
void __fastcall UpdateExhaustEffects(CAutomobile* self, re::Edx) {
    uint8_t* p = reinterpret_cast<uint8_t*>(self);
    uint8_t* effects = self->effects;
    if ((self->typeAndStatus & 0xF8) == 0x28 && SpeedSq(self) < 0.01f) {
        Effect_Kill(effects);
        Effect_Kill(effects + 8);
        Effect_Kill(effects + 0x10);
        Effect_Kill(effects + 0x18);
        return;
    }
    if (self->effectIds[0] == 0xFFFF) {
        self->effectIds[0] = g_exhaustEffectA;
    }
    if (self->effectIds[1] == 0xFFFF) {
        self->effectIds[1] = g_exhaustEffectB;
    }
    uint32_t type;
    if (!At<void*>(effects, 4)) {
        type = uint16_t(self->effectIds[0]);
        Effects_Start(g_effects, &type, effects, 0);
    }
    if (!At<void*>(effects, 0xC)) {
        type = uint16_t(self->effectIds[1]);
        Effects_Start(g_effects, &type, effects + 8, 0);
    }
    if (DrivenByPlayer(self) && !At<void*>(effects, 0x14)) {
        type = uint16_t(self->effectIds[0]);
        Effects_Start(g_effects, &type, effects + 0x10, 0);
    }
    if (DrivenByPlayer(self) && !At<void*>(effects, 0x1C)) {
        type = uint16_t(self->effectIds[1]);
        Effects_Start(g_effects, &type, effects + 0x18, 0);
    }
    uint8_t* mi = modelInfoPtrs[self->modelIndex];
    Vec exhaust = {At<float>(mi, 0xAC), At<float>(mi, 0xB0), At<float>(mi, 0xB4)};
    if (!DrivenByPlayer(self)) {
        exhaust.x = 0.0f;
    }
    exhaust.z = float(double(exhaust.z) + double(0.2f));
    EffectTransform t;
    t.scale = 1.0f;
    Matrix tilt, product, q;
    q.attached = nullptr;
    tilt.attached = nullptr;
    Matrix_SetRotateY(&tilt, float(-double(self->steerAngle) * 0.5));
    Matrix_Assign(&q, Matrix_Multiply(&product, Entity_GetMatrix(self), &tilt));
    struct { Vec v; float pad; } atPad16;   // (the game's transform writes a 4th float)
    Vec& at = atPad16.v;
    MultiplyPoint(&at, &q, &exhaust);
    float right[3] = {q.m[0], q.m[1], q.m[2]};
    float up[3] = {q.m[8], q.m[9], q.m[10]};
    float back[3] = {-q.m[4], -q.m[5], -q.m[6]};
    float boost = 0.0f;
    float steer = 0.0f;
    void* seats = &self->seats;
    if (Seats_GetOccupant(seats, 0) && g_race && At<uint8_t>(g_race, 4) && At<int32_t>(g_race, 0x3C) == 0xE) {
        uint8_t* racer = Race_GetRacer(g_race, Seats_GetOccupant(seats, 0));
        if (racer) {
            uint32_t since = timeInMs - At<uint32_t>(racer, 0x60);
            if (since < 0x5DC) {
                double elapsed = double(int32_t(since));
                if (int32_t(since) < 0) {
                    elapsed = elapsed + double(4294967296.0f);
                }
                boost = float(1.5 - elapsed / 1000.0);
                if (At<int32_t>(Seats_GetOccupant(seats, 0), 0x1310) == 0xD && g_cameraMode != 0xE) {
                    int strength = int(double(Vec_Magnitude(reinterpret_cast<Vec*>(&self->moveSpeed))) * 35.0 + boost);
                    Pad_Rumble(0, 0x41, uint8_t(strength), 1);
                }
            }
        }
        if (At<int32_t>(Seats_GetOccupant(seats, 0), 0x1310) == 0xD) {
            steer = self->steerAngle;
        }
    }
    t.rot[1] = up[0];
    t.rot[4] = up[1];
    t.rot[7] = up[2];
    t.trans = at;
    float gas = At<float>(p, 0x294);
    float stretch = float(((double(gas) + 1.0) + boost) + steer);
    PlaceExhaust(self, &t, effects + 8, effects, stretch, right, back);
    if (!DrivenByPlayer(self)) {
        return;
    }
    exhaust.x = -exhaust.x;
    MultiplyPoint(&at, &q, &exhaust);
    t.trans = at;
    stretch = float(((double(gas) + 1.0) + boost) - steer);
    PlaceExhaust(self, &t, effects + 0x18, effects + 0x10, stretch, right, back);
}

} // namespace c09autoexhaust

// ---- registration ---------------------------------------------------------------------

#define AUTOEXHAUST_REVERSED(name, address, fn)                                              \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

AUTOEXHAUST_REVERSED("CAutomobile::UpdateExhaustEffects", 0x004AD740, &c09autoexhaust::UpdateExhaustEffects);
