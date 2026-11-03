// CFireManager's fires (10 of 0x44 bytes at the manager's start): finding one, their
// frame (burning down, the "BURNCAN" loop sound, the damage around them, dropping to
// the ground), and putting them out along a line (a hose, an extinguisher).
// 0x42DCF0..0x42E350.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "reversed.h"

#define FIREMGR_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace {

struct FmVector {
    float x, y, z;
};

// A fire (GTA's CFire, Bully's own layout)
struct CFire {
    uint32_t unknown00;     // +0x0
    float damageInterval;   // +0x4 seconds between its damage events
    float damageArg;        // +0x8 (the damage event's last float)
    float damageScale;      // +0xC (times the strength, / 100: the event's radius)
    float strength;         // +0x10 > 0: burning
    float maxStrength;      // +0x14 (SetStrength clamps to it; the burn rate's scale)
    float lastDamageTime;   // +0x18 seconds
    uint32_t unknown1C;     // +0x1C
    uint8_t flags;          // +0x20 2: drop to the ground, 4: burns down (not up), 8: sound, 0xF0: frames before it burns down again
    uint8_t unknown21[3];
    FmVector pos;           // +0x24
    uint32_t unknown30;     // +0x30
    re::Ptr32<void> target;          // +0x34 what burns (a registered reference)
    uint8_t effect[4];      // +0x38 its effect's attachment (0x669040)
    re::Ptr32<void> effectObject;     // +0x3C
    uint8_t sound[4];       // +0x40 its sound (0x597010)
};
static_assert(offsetof(CFire, strength) == 0x10);
static_assert(offsetof(CFire, flags) == 0x20);
static_assert(offsetof(CFire, pos) == 0x24);
static_assert(offsetof(CFire, target) == 0x34);
static_assert(offsetof(CFire, effectObject) == 0x3C);
static_assert(offsetof(CFire, sound) == 0x40);
static_assert(sizeof(CFire) == 0x44);

constexpr int NUM_FIRES = 10;

GAME_VAR(float, fm_timeStep, 0x00C1A9A4);
GAME_VAR(uint32_t, fm_gameTimeMs, 0x00C1A9B4);
GAME_VAR(float, fm_burnRate, 0x009E52BC);  // (0.02)
inline void* const fm_audio = reinterpret_cast<void*>(0x00CF3DA0);
inline const char* const fm_burnCan = reinterpret_cast<const char*>(0x00901BDC);  // "BURNCAN"

GAME_FN(uint8_t, __thiscall, FM_Fire_IsHitBy, 0x0042D980, CFire* fire, FmVector a, FmVector b, float c);
GAME_FN(void, __thiscall, FM_Fire_SetStrength, 0x0042DAC0, CFire* fire, float strength);
GAME_FN(void, __thiscall, FM_Fire_Update42D8A0, 0x0042D8A0, CFire* fire);
GAME_FN(void, __thiscall, FM_Fire_ProcessFire, 0x0042DE40, CFire* fire);
GAME_FN(void, __thiscall, FM_Fire_Extinguish, 0x0042E0E0, CFire* fire, FmVector start, FmVector dir, float length, float strength);
GAME_FN(FmVector*, __cdecl, FM_Sub, 0x00414B70, FmVector* out, const FmVector* a, const FmVector* b);
GAME_FN(FmVector*, __cdecl, FM_Add, 0x004129A0, FmVector* out, const FmVector* a, const FmVector* b);
GAME_FN(FmVector*, __cdecl, FM_GetCameraPos, 0x00595630, FmVector* out);
GAME_FN(int32_t, __thiscall, FM_Audio_GetSoundId, 0x00595B80, void* audio, const char* name);
GAME_FN(float, __cdecl, FM_Audio_Fn59E4F0, 0x0059E4F0, float a, int b);
GAME_FN(void, __cdecl, FM_Sound_Play, 0x00597010, void* sound, int32_t id, const FmVector* pos, int a, int b, int c, int d, int e);
GAME_FN(void, __cdecl, FM_AddDamageEvent, 0x00463CC0, int type, const FmVector* pos, float r1, float r2, float f, float g, uintptr_t a, int b);
GAME_FN(void, __cdecl, FM_CleanUpOldReference, 0x004657E0, void* entity, re::Ptr32<void>* ref);
GAME_FN(float, __cdecl, FM_FindGroundZFor3DCoord, 0x004635A0, float x, float y, float z, uint8_t* found, int flag);
GAME_FN(void, __thiscall, FM_Effect_SetPosition, 0x00669040, void* effect, const FmVector* pos, int arg);

float MagnitudeAbs(const FmVector* v) {
    float sq = v->x * v->x + v->y * v->y + v->z * v->z;
    return std::sqrt(std::fabs(sq));
}

} // namespace

namespace CFireManager {

// 0x42DCF0 (thiscall): the first burning fire 0x42D980 says the two vectors and the
// float hit (nullptr: none)
CFire* __fastcall FindFireHitBy(CFire* fires, re::Edx, FmVector a, FmVector b, float c) {
    for (int i = 0; i < NUM_FIRES; i++) {
        CFire* fire = &fires[i];
        if (0.0f < fire->strength && FM_Fire_IsHitBy(fire, a, b, c)) {
            return &fires[i];
        }
    }
    return nullptr;
}

// 0x42DD80 (thiscall): a burning fire on the entity, or within 1 unit of it
uint8_t __fastcall IsEntityOnFire(CFire* fires, re::Edx, uint8_t* entity) {
    uint8_t* matrix = *reinterpret_cast<re::Ptr32<uint8_t>*>(entity + 0x14);
    FmVector pos = *reinterpret_cast<FmVector*>(matrix ? matrix + 0x30 : entity + 4);
    for (int i = 0; i < NUM_FIRES; i++) {
        CFire* fire = &fires[i];
        if (!(0.0f < fire->strength)) {
            continue;
        }
        if (fire->target == entity) {
            return 1;
        }
        FmVector d;
        if (MagnitudeAbs(FM_Sub(&d, &pos, &fire->pos)) < 1.0f) {
            return 1;
        }
    }
    return 0;
}

// 0x42DE40 (thiscall): a fire's frame (GTA's CFire::ProcessFire): burning up (or down,
// flag 4, after the frames counted in the high nibble), its sound within 50 units, its
// damage event every damageInterval seconds, its target let go when it's a prop anim
// whose vf30 says so, and (flag 2, no target) dropped to the ground once
void __fastcall ProcessFire(CFire* fire, re::Edx) {
    uint8_t flags = fire->flags;
    uint8_t burnsDown = (flags >> 2) & 1;
    float sign = burnsDown ? -1.0f : 1.0f;
    float rate = fm_burnRate * fm_timeStep;
    float delta = float(fire->maxStrength * sign * (rate / 5.0));
    if (burnsDown && (flags >> 4) != 0) {
        fire->flags = uint8_t(((flags >> 4) - 1) << 4 | (flags & 0xF));
    } else {
        FM_Fire_SetStrength(fire, fire->strength + delta);
    }
    if (0.0f == fire->strength) {
        return;
    }
    FmVector* pos = &fire->pos;
    FmVector cam, d;
    FM_GetCameraPos(&cam);
    FM_Sub(&d, pos, &cam);
    if (MagnitudeAbs(&d) <= 50.0 && 0.0f != fire->strength && (fire->flags & 8)) {
        int32_t id = FM_Audio_GetSoundId(fm_audio, fm_burnCan);
        float volume = FM_Audio_Fn59E4F0(1024.0f, 4);
        double ratio = double(fire->strength) / fire->maxStrength;
        int a = int((1.0 - ratio) * 1200.0);
        int b = int(ratio * volume);
        FM_Sound_Play(fire->sound, id, pos, 3, b, a, 0, 0);
    }
    float seconds = float(double(fm_gameTimeMs) * 0.001);
    if (fire->damageInterval < seconds - fire->lastDamageTime) {
        fire->lastDamageTime = seconds;
        float radius = float(fire->damageScale * fire->strength / 100.0);
        FM_AddDamageEvent(-7, pos, radius, radius, 0.01f, fire->damageArg, 0, 1);
    }
    FM_Fire_Update42D8A0(fire);
    uint8_t* target = static_cast<uint8_t*>(fire->target);
    if (target && (target[0x108] & 7) == 6 && reinterpret_cast<uint8_t(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(target))[0x78 / 4]))(target)) {
        if (fire->target) {
            FM_CleanUpOldReference(fire->target, &fire->target);
        }
        fire->target = nullptr;
    }
    if ((fire->flags & 2) && !fire->target) {
        float z = float(fire->pos.z + 1.0);
        uint8_t found = 0;
        float ground = FM_FindGroundZFor3DCoord(pos->x, fire->pos.y, z, &found, 1);
        if (found) {
            fire->pos.z = ground;
            if (fire->effectObject) {
                FM_Effect_SetPosition(fire->effect, pos, 0);
            }
        }
        fire->flags = (fire->flags & 0xFD) | 4;
    }
}

// 0x42E0E0 (thiscall): water along a line (from start, its unit direction, its length):
// a fire within 1.3 units of it (its point 0.4 up) loses strength, less past halfway,
// and burns down for the next 15 frames
void __fastcall Extinguish(CFire* fire, re::Edx, FmVector start, FmVector dir, float length, float strength) {
    FmVector firePos = {fire->pos.x, fire->pos.y, float(fire->pos.z + 0.4f)};
    FmVector d;
    FM_Sub(&d, &firePos, &start);
    float t = dir.y * d.y + dir.x * d.x + dir.z * d.z;
    if (t < 0.0f) {
        return;
    }
    double reach = length + 1.3;
    if (reach < t) {
        return;
    }
    FmVector along = {dir.x * t, dir.y * t, dir.z * t};
    FmVector closest, off;
    FM_Add(&closest, &start, &along);
    if (!(MagnitudeAbs(FM_Sub(&off, &closest, &firePos)) < 1.3f)) {
        return;
    }
    float rate = fm_burnRate * fm_timeStep;
    strength = -rate * strength;
    double half = length * 0.5;
    if (t > half) {
        float scale = float((reach - t) / (half + 1.3));
        strength = scale * strength;
    }
    FM_Fire_SetStrength(fire, fire->strength + strength);
    fire->flags |= 0xF0;
}

// 0x42E2A0 (thiscall): every burning fire's frame
void __fastcall ProcessFires(CFire* fires, re::Edx) {
    for (int i = 0; i < NUM_FIRES; i++) {
        if (0.0f < fires[i].strength) {
            FM_Fire_ProcessFire(&fires[i]);
        }
    }
}

// 0x42E2D0 (thiscall): Extinguish on every burning fire
void __fastcall ExtinguishAlong(CFire* fires, re::Edx, FmVector start, FmVector dir, float length, float strength) {
    for (int i = 0; i < NUM_FIRES; i++) {
        if (0.0f < fires[i].strength) {
            FM_Fire_Extinguish(&fires[i], start, dir, length, strength);
        }
    }
}

FIREMGR_REVERSED("CFireManager::FindFireHitBy", 0x0042DCF0, &FindFireHitBy);
FIREMGR_REVERSED("CFireManager::IsEntityOnFire", 0x0042DD80, &IsEntityOnFire);
FIREMGR_REVERSED("CFire::ProcessFire", 0x0042DE40, &ProcessFire);
FIREMGR_REVERSED("CFire::Extinguish", 0x0042E0E0, &Extinguish);
FIREMGR_REVERSED("CFireManager::ProcessFires", 0x0042E2A0, &ProcessFires);
FIREMGR_REVERSED("CFireManager::ExtinguishAlong", 0x0042E2D0, &ExtinguishAlong);

} // namespace CFireManager
