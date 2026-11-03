// CPhysical's collision helpers: its entity-against-entity test (vf41), its horizontal
// speed per second, a downward vector flattened, and an entity's destruction effects.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "game/Entity.h"
#include "reversed.h"

namespace {

using col::CMatrix;
using col::CVector;

GAME_VAR(float, s_timeStep, 0x00C1A9A4);
GAME_VAR(float, s_timeStepNonClipped, 0x00C1A9A0);
GAME_VAR(int32_t, s_specialPedModel, 0x00BF42DC);
GAME_VAR(CColModel*, s_pedLineColModel, 0x00CF3138);
GAME_VAR(uint16_t, s_destructionEffect, 0x020C8FF8);
GAME_VAR(void*, s_effectManager, 0x00B6F1D4);
inline void* const s_windowGlows = reinterpret_cast<void*>(0x00CE7580);
inline void* const s_sounds = reinterpret_cast<void*>(0x00CF3DA0);

GAME_FN(CColModel*, __cdecl, GetColModel, 0x0051AF50, CEntity* entity);
GAME_FN(CMatrix*, __thiscall, Placeable_GetMatrix, 0x0046DD20, CEntity* entity);
GAME_FN(uint8_t, __thiscall, Ped_Fn4473E0, 0x004473E0, CEntity* ped);
GAME_FN(void, __cdecl, Ped_SetupLineColModel, 0x005727D0, CEntity* ped);
GAME_FN(uint8_t, __thiscall, Physical_ProcessPedSpheres, 0x0046A0C0, CEntity* self, CMatrix* previous, CMatrix* matrix, CColModel* col,
        CMatrix* otherMatrix, CColModel* otherCol, CColPoint* points);
GAME_FN(uint8_t, __cdecl, Col_ProcessSweep, 0x0056C8C0, CMatrix* previous, CMatrix* matrix, CColModel* col, CMatrix* otherMatrix,
        CMatrix* otherMatrix2, CColModel* otherCol, CColPoint* points, int a, int b, int c, int d, int e, int surfaces, uintptr_t f, int g);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(void*, __thiscall, Get2dEffect, 0x0050ED70, void* modelInfoOrEffect);
GAME_FN(void, __thiscall, EffectProxy_Construct, 0x00668E90, void* proxy);
GAME_FN(void, __thiscall, EffectProxy_Destruct, 0x00668EB0, void* proxy);
GAME_FN(void, __thiscall, EffectManager_Start, 0x00668290, void* manager, const uint16_t* type, void* proxy, int arg);
GAME_FN(void, __thiscall, EffectProxy_SetPosition, 0x00669040, void* proxy, const CVector* pos, int arg);
// (0x0041A9A0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* Matrix_MultiplyPoint(CVector* out, const CMatrix* m, const CVector* v) { return re::CallVec16<0x0041A9A0, CVector*>(out, m, v); }
GAME_FN(void, __thiscall, WindowGlow_TurnOff, 0x00537BF0, void* glows, const CVector* centre, float radius);
GAME_FN(void, __thiscall, Sounds_Fn59BB30, 0x0059BB30, void* sounds, CVector pos);

// 0x46A490 vf41 (thiscall): a collision with another entity (a ped through its own
// spheres, unless it's special); a hit on surface 0x41 doesn't count for +0xC4 entities
uint8_t __fastcall ProcessEntityCollision(CEntity* self, re::Edx, CMatrix* previous, CEntity* other, CColPoint* points, int) {
    CColModel* col = GetColModel(self);
    uint8_t hits;
    if (self->flagC4 && other->GetType() == ENTITY_TYPE_PED && !other->flagA4 && other->modelIndex != s_specialPedModel &&
        !Ped_Fn4473E0(other)) {
        Ped_SetupLineColModel(other);
        CColModel* pedCol = s_pedLineColModel;
        CMatrix* otherMatrix = Placeable_GetMatrix(other);
        hits = Physical_ProcessPedSpheres(self, previous, Placeable_GetMatrix(self), col, otherMatrix, pedCol, points);
    } else {
        CColModel* otherCol = GetColModel(other);
        CMatrix* otherMatrix = Placeable_GetMatrix(other);
        CMatrix* otherMatrix2 = Placeable_GetMatrix(other);
        hits = Col_ProcessSweep(previous, Placeable_GetMatrix(self), col, otherMatrix2, otherMatrix, otherCol, points, 0, 0, 0, 0, 0, 0xF,
                                0, 1);
    }
    int result = hits;
    if (result > 0 && self->flagC4 && points->surfaceB == 0x41) {
        result = 0;
    }
    return uint8_t(result);
}

// 0x46A580 (thiscall): its horizontal speed, per second
float __fastcall GetHorizontalSpeedPerSecond(CPhysical* self, re::Edx) {
    CVector v = self->moveSpeed;
    float sq = float(double(v.x) * v.x + double(v.y) * v.y + 0.0 * 0.0);
    float len = float(std::sqrt(double(std::fabs(sq))));
    float perStep = len * s_timeStep;
    float perSecond = float(1000.0 / (double(s_timeStepNonClipped) * 1000.0 / 50.0));
    return perStep * perSecond;
}

// 0x46A610 (stdcall): a vector pointing down made horizontal, its length kept
void __fastcall FlattenDownward(void*, re::Edx, CVector* v) {
    if (!(0.0f < v->z) && !(0.0f == v->z)) {
        float sq = float(double(v->x) * v->x + double(v->y) * v->y + double(v->z) * v->z);
        float len = float(std::sqrt(double(std::fabs(sq))));
        v->z = 0.0f;
        Vector_Normalise(v);
        v->x = v->x * len;
        v->y = len * v->y;
        v->z = len * v->z;
    }
}

// 0x46AAF0: its model's effects spawned where they are (and the window glows there
// turned off), a sound at its position; +0xCC set
void __cdecl SpawnDestructionEffects(CEntity* e) {
    uint8_t* mi = *reinterpret_cast<re::Ptr32<uint8_t>*>(0x00C67738 + e->modelIndex * 4);
    for (uint8_t* effect = static_cast<uint8_t*>(Get2dEffect(mi)); effect;) {
        uint16_t type = s_destructionEffect;
        uint8_t proxy[0xC];
        EffectProxy_Construct(proxy);
        EffectManager_Start(s_effectManager, &type, proxy, 0);
        struct { CVector v; float pad; } posPad16;   // (the game's transform writes a 4th float)
        CVector& pos = posPad16.v;
        Matrix_MultiplyPoint(&pos, Placeable_GetMatrix(e), reinterpret_cast<CVector*>(effect));
        EffectProxy_SetPosition(proxy, &pos, 0);
        WindowGlow_TurnOff(s_windowGlows, &pos, 1.0f);
        effect = static_cast<uint8_t*>(Get2dEffect(effect));
        EffectProxy_Destruct(proxy);
    }
    Sounds_Fn59BB30(s_sounds, e->GetPosition());
    e->flagCC = 1;
}

} // namespace

#define PHYSC_C05_REVERSED(name, address, fn) REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PHYSC_C05_REVERSED("CPhysical::ProcessEntityCollision", 0x0046A490, &ProcessEntityCollision);
PHYSC_C05_REVERSED("CPhysical::GetHorizontalSpeedPerSecond", 0x0046A580, &GetHorizontalSpeedPerSecond);
PHYSC_C05_REVERSED("FlattenDownward", 0x0046A610, &FlattenDownward);
PHYSC_C05_REVERSED("CEntity::SpawnDestructionEffects", 0x0046AAF0, &SpawnDestructionEffects);
