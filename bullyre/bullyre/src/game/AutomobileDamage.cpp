// CAutomobile (vtable 0x905D04): damage from a collision (0x4AB100, GTA's
// VehicleDamage): by the piece hit, the part's damage applied (the damage manager at
// +0x580), shown on its frame, the particles, a smashed headlight's effect; the
// health taken (less for others' cars, more for the player's), the sounds.

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

GAME_ARRAY(uint8_t*, modelInfoPtrs, 0x00C67738);   // CModelInfo::ms_modelInfoPtrs
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);
GAME_VAR(int32_t, g_modelBF44E0, 0x00BF44E0);
GAME_VAR(uint16_t, g_lightSmashEffect, 0x020C9010);
GAME_VAR(uint16_t, g_glassEffect, 0x020C900C);
GAME_VAR(uint16_t, g_bumperEffect, 0x020C9008);
GAME_VAR(void*, g_effects, 0x00B6F1D4);
inline void* const TheSounds = reinterpret_cast<void*>(0x00CF3DA0);

GAME_FN(void*, __cdecl, FindPlayerVehicle, 0x00446970, uintptr_t player);
GAME_FN(uint8_t*, __cdecl, AsPed, 0x00468E10, void* entity);
GAME_FN(float*, __thiscall, Entity_GetMatrix, 0x008B6120, void* entity);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const Vec* a, const Vec* b);
// (0x0045D500 stores 16 bytes through `out`: called through re::CallVec16)
static inline float* MultiplyPoint(float* out, const float* m, const Vec* v) { return re::CallVec16<0x0045D500, float*>(out, m, v); }
GAME_FN(int32_t, __thiscall, Damage_GetLightStatus, 0x004C7DA0, void* damage, int light);
GAME_FN(void, __thiscall, Damage_SetLightStatus, 0x004C7D70, void* damage, int light, int status);
GAME_FN(uint8_t, __thiscall, Damage_GetPanelStatus, 0x004C7DC0, void* damage, int panel);
GAME_FN(uint8_t, __thiscall, Damage_GetDoorStatus, 0x004C7E00, void* damage, int door);
GAME_FN(uint8_t, __thiscall, Damage_ApplyDamage, 0x004C7E90, void* damage, int component, float amount, float multiplier);
GAME_FN(void, __thiscall, Effect_Construct, 0x00668E90, void* effect);
GAME_FN(void, __thiscall, Effect_Destruct, 0x00668EB0, void* effect);
GAME_FN(void, __thiscall, Effect_SetPosition, 0x00669040, void* effect, const void* pos, int);
GAME_FN(void, __thiscall, Effects_Start, 0x00668290, void* manager, uint32_t* type, void* effect, int);
GAME_FN(int, __thiscall, Sounds_GetId, 0x00595B80, void* sounds, const char* name);
GAME_FN(void*, __thiscall, Sounds_PlayAt, 0x0059B8A0, void* sounds, int id, const Vec* pos, int, const Vec* pos2, int, int, int,
        int, uintptr_t, uintptr_t, int, int, float, int);
// (its own, called where modes apply)
GAME_FN(void, __thiscall, Automobile_HitParticles, 0x004A85C0, void* automobile, const void* pos, float amount);
GAME_FN(void, __thiscall, Automobile_ShowPanelDamage, 0x004A8810, void* automobile, int frame, int panel, int);
GAME_FN(void, __thiscall, Automobile_ShowDoorDamage, 0x004A8F00, void* automobile, int frame, int door, int);

// 0x589B00 takes a registered entity reference by value (a stack slot the caller
// registers and the callee lets go), then the damage: built here the way the game does
__declspec(noinline) void Call589B00WithRef(void* entity, float damage) {
#if defined(_M_IX86)
    void* fn = reinterpret_cast<void*>(0x00589B00);
    void* setRef = reinterpret_cast<void*>(0x005D7F10);
    __asm {
        push 0
        push 0
        sub esp, 8
        mov ecx, esp
        mov dword ptr [ecx], 0
        mov eax, damage
        mov dword ptr [esp + 4], eax
        push entity
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
    re::Ours<void(__cdecl*)(EntityRef64, float, void*, int)>(0x00589B00)(ref, damage, nullptr, 0);
#endif
}

const Vec* EntityPos(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<Vec*>(matrix + 0x30) : reinterpret_cast<Vec*>(static_cast<uint8_t*>(entity) + 4);
}

void PlaySoundAtCar(CAutomobile* self, const char* name) {
    const Vec* pos = EntityPos(self);
    Sounds_PlayAt(TheSounds, Sounds_GetId(TheSounds, name), pos, 0, pos, 0, 0, 0, 0, 0, 0, 6, 1, 1.0f, 0);
}

// an effect started at a point (a temporary proxy)
void EffectAt(uint16_t effectType, const void* pos) {
    uint32_t type = effectType;   // (a word; the upper half stack garbage in the game)
    uint8_t effect[8];
    Effect_Construct(effect);
    Effects_Start(g_effects, &type, effect, 0);
    Effect_SetPosition(effect, pos, 0);
    Effect_Destruct(effect);
}

// the point beside a model dummy, in the world: the dummy moved out along the car's
// right by twice its own x
void DummyCorner(CAutomobile* self, const float* dummy, float* raw, float* corner) {
    Vec d = {dummy[0], dummy[1], dummy[2]};
    MultiplyPoint(raw, Entity_GetMatrix(self), &d);
    corner[0] = raw[0];
    corner[1] = raw[1];
    corner[2] = raw[2];
    float k = std::fabs(float(double(dummy[0]) + dummy[0]));
    float* m = Entity_GetMatrix(self);
    float r[3] = {m[0], m[1], m[2]};
    r[0] = float(double(r[0]) * k);
    r[1] = float(double(r[1]) * k);
    r[2] = float(double(k) * r[2]);
    corner[0] = r[0] + corner[0];
    corner[1] = r[1] + corner[1];
    corner[2] = r[2] + corner[2];
}

} // namespace

namespace c09autodamage {

// 0x4AB100: impulse 0: the car's own last collision (+0x1A0, piece +0x1B4) at full
// strength, else a third. Nothing below 5, for a wreck, a mission / another's car hit
// by other than the player (unless +0x70 is 0), a car with +0x68, hits from buildings
// on its roof, or by a ped whose +0x1D0's model is 0xBF44E0
void __fastcall VehicleDamage(CAutomobile* self, re::Edx, float impulse, int pieceArg) {
    uint8_t* p = reinterpret_cast<uint8_t*>(self);
    void* damage = p + 0x580;
    float scale = 0.333f;
    uint8_t bodyHit = 0, bodyHit2 = 0, windowSmash = 0, lightSmash = 0;
    uint16_t piece;
    if (impulse == 0.0f) {
        impulse = self->damageImpulse;
        piece = self->damagePiece;
        scale = 1.0f;
    } else {
        piece = uint16_t(pieceArg);
    }
    Vec framePos = {0.0f, 0.0f, 0.0f};
    if (!(self->flags33A & 0x40)) {
        return;
    }
    if (!(5.0f < impulse)) {
        return;
    }
    uint8_t status = self->typeAndStatus >> 3;
    if (status == 5) {
        return;
    }
    if (status != 0 && self->unknown70 != 0 && re::PtrCast<uint8_t*>(self->damageEntity) != playerPed &&
        self->damageEntity != FindPlayerVehicle(0)) {
        return;
    }
    if (self->unknown68 != 0) {
        return;
    }
    uint8_t* hitBy = re::PtrCast<uint8_t*>(self->damageEntity);
    if (hitBy && (At<uint8_t>(hitBy, 0x108) & 7) == 1) {
        float* m = Entity_GetMatrix(self);
        if (DotProduct(reinterpret_cast<Vec*>(&self->damagePos), reinterpret_cast<Vec*>(m + 8)) > 0.6f) {
            return;
        }
    }
    if (self->damageEntity) {
        uint8_t* ped = AsPed(self->damageEntity);
        if (ped && At<int16_t>(At<uint8_t*>(ped, 0x1D0), 0x10E) == g_modelBF44E0) {
            return;
        }
    }
    int32_t lightsBefore[4];
    for (int i = 0; i < 4; i++) {
        lightsBefore[i] = Damage_GetLightStatus(damage, i);
    }
    float upZ = Entity_GetMatrix(self)[10];
    const veh::CVector& ms = self->moveSpeed;
    float speedSq = float((double(ms.y) * ms.y + double(ms.x) * ms.x) + double(ms.z) * ms.z);
    if (0.0f < upZ || double(speedSq) > double(0.1f)) {
        uint8_t* mi = modelInfoPtrs[self->modelIndex];
        float front[4], frontCorner[3], rear[4], rearCorner[3];   // (the transform writes four floats)
        DummyCorner(self, &At<float>(mi, 0x64), front, frontCorner);
        DummyCorner(self, &At<float>(mi, 0x70), rear, rearCorner);
        using GetFramePos = void(__thiscall*)(void*, int, Vec*);
        GetFramePos framePosition = re::PtrCast<GetFramePos>(self->vtable[0xAC / 4]);
        // a door: (frame, door, component)
        auto hitDoor = [&](int frame, int door, int component) {
            framePosition(self, frame, &framePos);
            Automobile_HitParticles(self, &framePos, float(double(scale) * impulse));
            uint8_t before = door >= 1 ? Damage_GetDoorStatus(damage, door) : 0;
            if (Damage_ApplyDamage(damage, component, float(double(impulse) * 4.0), At<float>(self->handling, 0xC4))) {
                Automobile_ShowDoorDamage(self, frame, door, 0);
                bodyHit = 1;
                bodyHit2 = 1;
                if (door >= 1) {
                    windowSmash = before != Damage_GetDoorStatus(damage, door);
                }
            }
        };
        // a light: (component, frame, panel, light, where)
        auto hitLight = [&](int component, int frame, int panel, int light, const float* where) {
            Automobile_HitParticles(self, where, float(double(scale) * impulse));
            if (Damage_ApplyDamage(damage, component, float(double(impulse) * 4.0), At<float>(self->handling, 0xC4))) {
                Automobile_ShowPanelDamage(self, frame, panel, 0);
                bodyHit = 1;
                bodyHit2 = 1;
            }
            if (Damage_GetLightStatus(damage, light) == 0) {
                Damage_SetLightStatus(damage, light, 1);
                EffectAt(g_lightSmashEffect, where);
                lightSmash = 1;
            }
        };
        switch (piece) {
        case 3:
            framePosition(self, 7, &framePos);
            Automobile_HitParticles(self, &framePos, float(double(scale) * impulse));
            if (Damage_ApplyDamage(damage, 0x10, float(double(impulse) * 4.0), At<float>(self->handling, 0xC4))) {
                Automobile_ShowPanelDamage(self, 7, 5, 0);
                bodyHit = 1;
                bodyHit2 = 1;
            }
            if (!self->frames[17] || Damage_GetPanelStatus(damage, 5) != 3) {
                break;
            }
            [[fallthrough]];
        case 1:
            hitDoor(0x11, 0, 5);
            break;
        case 4:
            framePosition(self, 8, &framePos);
            Automobile_HitParticles(self, &framePos, float(double(scale) * impulse));
            if (Damage_ApplyDamage(damage, 0x11, float(double(impulse) * 4.0), At<float>(self->handling, 0xC4))) {
                Automobile_ShowPanelDamage(self, 8, 6, 0);
                bodyHit = 1;
                bodyHit2 = 1;
            }
            if (!self->frames[18] || Damage_GetPanelStatus(damage, 6) != 3) {
                break;
            }
            [[fallthrough]];
        case 2:
            hitDoor(0x12, 1, 6);
            break;
        case 5:
            hitDoor(0xF, 2, 7);
            break;
        case 6:
            hitDoor(0xB, 3, 8);
            break;
        case 7:
            hitDoor(0x10, 4, 9);
            break;
        case 8:
            hitDoor(0xC, 5, 10);
            break;
        case 9:
            hitLight(0xB, 0xD, 0, 0, front);
            break;
        case 10:
            hitLight(0xC, 9, 1, 1, frontCorner);
            break;
        case 0xB:
            hitLight(0xD, 0xE, 2, 2, rear);
            break;
        case 0xC:
            hitLight(0xE, 0xA, 3, 3, rearCorner);
            break;
        case 0x11: {
            framePosition(self, 0x13, &framePos);
            uint8_t before = Damage_GetPanelStatus(damage, 4);
            if (!Damage_ApplyDamage(damage, 0xF, float(double(impulse) * 4.0), At<float>(self->handling, 0xC4))) {
                break;
            }
            Automobile_ShowPanelDamage(self, 0x13, 4, 0);
            if (before == Damage_GetPanelStatus(damage, 4)) {
                break;
            }
            bodyHit = 1;
            if (Damage_GetPanelStatus(damage, 4) == 3) {
                EffectAt(g_glassEffect, &framePos);
                windowSmash = 1;
            } else {
                EffectAt(g_bumperEffect, &framePos);
                lightSmash = 1;
            }
            break;
        }
        default:
            break;
        }
    }
    // the health
    float dmg = float(double(impulse) - 5.0);
    dmg = float(double(dmg) * At<float>(self->handling, 0xC4));
    dmg = float(double(dmg) * double(0.6f));
    dmg = float(double(dmg) * scale);
    if (0.0f < dmg) {
        int16_t before = int16_t(int32_t(self->health));
        if (static_cast<void*>(self) == FindPlayerVehicle(0)) {
            self->health = float(double(self->health) - double(dmg) * 0.5);
        } else if (self->damageEntity && self->damageEntity == FindPlayerVehicle(0)) {
            self->health = float(double(self->health) - double(dmg) / 1.5);
        } else {
            self->health = float(double(self->health) - double(dmg) * 0.25);
        }
        Call589B00WithRef(self, dmg);
        if (!(0.0f < self->health) && before > 0) {
            self->health = 1.0f;
        }
    }
    for (int i = 0; i < 4; i++) {
        if (lightsBefore[i] != 1 && Damage_GetLightStatus(damage, i) == 1) {
            PlaySoundAtCar(self, "HeadliteSmash");
            break;
        }
    }
    if (bodyHit && bodyHit2) {
        PlaySoundAtCar(self, "CarBodyHit");
    }
    if (windowSmash) {
        PlaySoundAtCar(self, "CarWindowSmash");
    }
    if (lightSmash) {
        PlaySoundAtCar(self, "HeadliteSmash");
    }
}

} // namespace c09autodamage

// ---- registration ---------------------------------------------------------------------

#define AUTODAMAGE_REVERSED(name, address, fn)                                               \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

AUTODAMAGE_REVERSED("CAutomobile::VehicleDamage", 0x004AB100, &c09autodamage::VehicleDamage);
