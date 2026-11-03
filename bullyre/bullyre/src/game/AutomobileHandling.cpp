// CAutomobile (vtable 0x905D04): the pad's rumble by the surface under the wheels,
// running peds over (or shoving them), the go-kart blowing up and its status, the
// gravity a bike feels (vf35), and the AI's throttle and brake (vf59). Fields past
// CVehicle by offset: the wheel records at +0x6C4 (0x28 bytes, +0x22 the surface), the
// wheels' +0x764 / +0x774 values, +0x8A0 the wheels' surface now and last (8 bytes each).

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

GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);
GAME_VAR(uint32_t, timeInMsPrev, 0x00C1A9AC);
GAME_VAR(Vec, g_9E33DC, 0x009E33DC);
GAME_VAR(void*, g_rocketBoneRoot, 0x01B6531C);
GAME_VAR(void*, g_rocketBone1, 0x01B65394);
GAME_VAR(void*, g_rocketBone2, 0x01B65440);
inline void* const g_rocketBoneNames = reinterpret_cast<void*>(0x01B65398);
inline void* const g_actionNodeA = reinterpret_cast<void*>(0x01B65270);
inline void* const g_actionNodeB = reinterpret_cast<void*>(0x01B65274);
inline void* const TheSounds = reinterpret_cast<void*>(0x00CF3DA0);

GAME_FN(Vec*, __thiscall, Entity_GetForward, 0x004154A0, void* entity, Vec* out);
GAME_FN(int32_t*, __thiscall, Vehicle_GetVehicleClass, 0x004CAF30, void* vehicle, int32_t* out);
GAME_FN(void*, __thiscall, Seats_GetOccupant, 0x004CE560, void* seats, uint32_t seat);
GAME_FN(uint8_t, __cdecl, Pad_RumbleEnabled, 0x007387D0);
GAME_FN(void, __cdecl, Pad_Rumble, 0x00738680, int pad, uint32_t duration, int strength, int);
GAME_FN(float, __cdecl, RandomFloat, 0x0041A5B0, float low, float high);
GAME_FN(float, __thiscall, Vec_Magnitude, 0x0040D9F0, const Vec* v);
GAME_FN(Vec*, __cdecl, VectorSub, 0x00414B70, Vec* out, const Vec* a, const Vec* b);
GAME_FN(Vec*, __cdecl, VectorAdd, 0x004129A0, Vec* out, const Vec* a, const Vec* b);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const Vec* a, const Vec* b);
GAME_FN(float, __cdecl, Min, 0x00414470, float a, float b);
GAME_FN(void, __thiscall, Physical_SetMoveSpeed, 0x0046A7E0, void* entity, float x, float y, float z);
GAME_FN(uint8_t*, __thiscall, Ped_GetDamageRecord, 0x00474BE0, void* ped);   // (+4 who did it)
GAME_FN(uint8_t, __thiscall, ActionController_IsPlaying, 0x005F4800, void* controller, void* node, int, int);
GAME_FN(void, __cdecl, Stats_Increment, 0x00458480, int stat, int by);
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(int, __cdecl, GetWeaponIndex, 0x0051AEA0, const char* name);
GAME_FN(void*, __cdecl, FindBoneInfo, 0x0072AEA0, void* path, void* names);
GAME_FN(void, __cdecl, Explosion_Add, 0x004D2C90, int weapon, int, int32_t* type, uintptr_t, void* creator, const Vec* pos,
        const Vec* dir, void* bone);
GAME_FN(void, __cdecl, Fn70CB60, 0x0070CB60, const Vec* pos, int, void* outA, void* outB);
GAME_FN(int, __thiscall, Sounds_GetId, 0x00595B80, void* sounds, const char* name);
GAME_FN(void, __thiscall, Sounds_Play, 0x0059BAB0, void* sounds, int id, void* a, void* b, int);
GAME_FN(void, __stdcall, DestructArray, 0x004017C0, void* array, size_t size, int count, void* destructor);
GAME_FN(uint8_t, __cdecl, Fn42E430, 0x0042E430);
GAME_FN(int32_t, __cdecl, Get20C8704_3C, 0x004A84A0);

const Vec* EntityPos(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<Vec*>(matrix + 0x30) : reinterpret_cast<Vec*>(static_cast<uint8_t*>(entity) + 4);
}
float SpeedSq(CAutomobile* self) {
    const veh::CVector& ms = self->moveSpeed;
    return float((double(ms.y) * ms.y + double(ms.x) * ms.x) + double(ms.z) * ms.z);
}
void CallVirtual(void* object, int offset, int arg) {
    using Fn = void(__thiscall*)(void*, int);
    re::PtrCast<Fn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[offset / 4])(object, arg);
}

} // namespace

namespace c09autohandling {

// 0x4A9F00: the player's pad rumbling as the wheels on the ground go over rough
// surfaces (8, 0x23, 0x2A, 0x38, 0x3A) or bumps (0x3B, every 256 ms by chance); each
// wheel's surface kept with the last
void __fastcall UpdateSurfaceRumble(CAutomobile* self, re::Edx) {
    void* seats = &self->seats;
    if (!Seats_GetOccupant(seats, 0) || At<int32_t>(Seats_GetOccupant(seats, 0), 0x1310) != 0xD) {
        return;
    }
    uint8_t* p = reinterpret_cast<uint8_t*>(self);
    for (uint8_t i = 0; i < 4; i++) {
        uint32_t last = At<uint32_t>(p, 0x8A0 + i * 8);
        At<uint32_t>(p, 0x8A0 + i * 8) = At<uint8_t>(p, 0x6E6 + i * 0x28);
        At<uint32_t>(p, 0x8A4 + i * 8) = last;
        if (!(1.0f > At<float>(p, 0x764 + i * 4))) {
            continue;
        }
        uint32_t surface = At<uint32_t>(p, 0x8A0 + i * 8);
        bool rough = surface == 8 || surface == 0x23 || surface == 0x2A || surface == 0x38 || surface == 0x3A;
        if (double(SpeedSq(self)) > double(0.007f) && rough && Pad_RumbleEnabled()) {
            float sq = SpeedSq(self);
            int strength = int(double(sq) * 40.0 + 1.0);
            Pad_Rumble(0, 0x4B, uint8_t(strength), 1);
            continue;
        }
        if (!(SpeedSq(self) > 0.04f) || At<uint32_t>(p, 0x8A0 + i * 8) != 0x3B || !((timeInMsPrev ^ timeInMs) & 0xFFFFFF00)) {
            continue;
        }
        float sq = SpeedSq(self);
        if (!(sq > RandomFloat(0.0f, 0.1f))) {
            return;
        }
        const Vec* ms = reinterpret_cast<const Vec*>(&self->moveSpeed);
        int strength = int(double(Vec_Magnitude(ms)) * 10.0);
        int32_t length = int32_t(double(Vec_Magnitude(ms)) * 30.0);
        // (the game multiplies the low word only: the high word is left as it was)
        uint32_t duration = (uint32_t(length) & 0xFFFF0000) | uint16_t(length * 0x1E);
        Pad_Rumble(0, duration, uint8_t(strength), 1);
        return;
    }
}

// 0x4AA160: the ped hit by the car: a bike launches it; run over (the car's handling
// damage) or, slow, just shoved. Not the ped the driver hit within the last second,
// nor one already reacting
void __fastcall HitPed(CAutomobile* self, re::Edx, uint8_t* ped) {
    const veh::CVector& ms = self->moveSpeed;
    float speed = float(std::sqrt(std::fabs(float(double(ms.y) * ms.y + double(ms.x) * ms.x))));
    int32_t vehicleClass;
    if (0.01f < speed && *Vehicle_GetVehicleClass(self, &vehicleClass) == 7) {
        float lift = float(double(SpeedSq(self)) + double(0.3f));
        Vec v = {float(double(g_9E33DC.x) * lift), float(double(g_9E33DC.y) * lift), float(double(lift) * g_9E33DC.z)};
        Vec launch = {float(double(v.x) * 10.0), float(double(v.y) * 10.0), float(10.0 * v.z)};
        Physical_SetMoveSpeed(ped, launch.x, launch.y, launch.z);
    }
    void* seats = &self->seats;
    uint8_t* record = Ped_GetDamageRecord(ped);
    if (At<void*>(record, 4) == Seats_GetOccupant(seats, 0)) {
        int32_t since = int32_t(timeInMs - At<uint32_t>(ped, 0x4CC));
        float elapsed = float(since);
        if (since < 0) {
            elapsed = elapsed + 4294967296.0f;
        }
        elapsed = std::fabs(elapsed);
        if (double(elapsed) < 1000.0) {
            return;
        }
    }
    if (!(0.01f < speed)) {
        return;
    }
    void* actions = At<uint8_t*>(ped, 0x2E0) + 0x258;
    if (ActionController_IsPlaying(actions, g_actionNodeA, 1, 1)) {
        return;
    }
    if (ActionController_IsPlaying(At<uint8_t*>(ped, 0x2E0) + 0x258, g_actionNodeB, 1, 1)) {
        return;
    }
    // (CPed's vf15 CPed_ApplyHit: the node source a pointer, full width)
    using Damage = void(__thiscall*)(void*, float, float, void*, int, const Vec*, const Vec*, uintptr_t, int, const char*, int, float,
                                     float, int, int, int, int, float, int);
    Damage damage = re::PtrCast<Damage>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(ped))[0x3C / 4]);
    if (!(double(speed) < double(0.025f) || double(speed) == double(0.025f))) {
        Vec carPos = *EntityPos(self);
        const Vec* pedPos = EntityPos(ped);
        Vec forward, d;
        Vec* f = Entity_GetForward(ped, &forward);
        float dot = DotProduct(VectorSub(&d, &carPos, pedPos), f);
        const char* name;
        if (*Vehicle_GetVehicleClass(self, &vehicleClass) == 7) {
            name = dot > 0.0f ? "bikerunover" : "carrunover";
        } else {
            name = dot > 0.0f ? "carrunover" : "bikerunover";
        }
        void* driver = Seats_GetOccupant(seats, 0);
        damage(ped, At<float>(self->handling, 0xC8), 0.0f, driver, -5, &carPos, nullptr, 0, 0, name, 0, 0.0f, 0.0f, 0, 1, 0, -1,
               0.0f, 0);
    } else {
        Vec d;
        VectorSub(&d, EntityPos(ped), EntityPos(self));
        if (!(0.0f < d.z || 0.0f == d.z)) {
            d.z = 0.0f;
        }
        damage(ped, 0.0f, 0.0f, nullptr, -1, nullptr, &d, 0, 0, "BUMPSHOVE", 0, 0.0f, 0.0f, 0, 1, 0, -1, 0.0f, 0);
    }
    if (At<int32_t>(ped, 0x1310) == 0xD) {
        Stats_Increment(0x11, 1);
    }
}

// 0x4AA530: a damaged go-kart (flags33A 0x40, class 10) blows up every 5 s: kicked up
// and spun, an explosion (0x12) at it, the sound, its status 5
void __fastcall KartExplode(CAutomobile* self, re::Edx) {
    if (!(self->flags33A & 0x40)) {
        return;
    }
    int32_t vehicleClass;
    if (*Vehicle_GetVehicleClass(self, &vehicleClass) != 10 || !(uint32_t(self->unknown348) + 5000 < timeInMs)) {
        return;
    }
    self->moveSpeed.z = float(double(self->moveSpeed.z) + double(0.2f));
    if (Rand() & 1) {
        self->turnSpeed.y = float(double(self->turnSpeed.y) + double(0.05f));
    } else {
        self->turnSpeed.y = float(double(self->turnSpeed.y) - double(0.05f));
    }
    int weapon = GetWeaponIndex("brocket");
    re::Ptr32<void> names[3] = {g_rocketBoneRoot, g_rocketBone1, g_rocketBone2};
    struct {
        int32_t count;
        re::Ptr32<re::Ptr32<void>> names;   // (ButesPath: the game's layout)
    } path = {3, names};
    void* bone = FindBoneInfo(&path, g_rocketBoneNames);
    Vec up = {0.0f, 0.0f, 1.0f};
    int32_t type = 0x12;
    Vec offset = {0.0f, 0.0f, 2.0f};
    const Vec* pos = EntityPos(self);
    void* driver = Seats_GetOccupant(&self->seats, 0);
    Vec at;
    Explosion_Add(weapon, 0, &type, 0, driver, VectorAdd(&at, pos, &offset), &up, bone);
    uint32_t outA = 0, outB = 0;
    Fn70CB60(EntityPos(self), 5, &outA, &outB);
    Sounds_Play(TheSounds, Sounds_GetId(TheSounds, "Explosion"), reinterpret_cast<void*>(uintptr_t(outA)),
                reinterpret_cast<void*>(uintptr_t(outB)), 0);
    CallVirtual(self, 0x18, 5);
    self->unknown348 = int32_t(timeInMs);
    DestructArray(names, 4, 3, reinterpret_cast<void*>(0x0049A650));
}

// 0x4AA700: a go-kart's status: 0 with the player in it, else 3
void __fastcall UpdateKartStatus(CAutomobile* self, re::Edx) {
    int32_t vehicleClass;
    if (*Vehicle_GetVehicleClass(self, &vehicleClass) != 10) {
        return;
    }
    void* seats = &self->seats;
    if (Seats_GetOccupant(seats, 0) && At<int32_t>(Seats_GetOccupant(seats, 0), 0x1310) == 0xD) {
        CallVirtual(self, 0x18, 0);
        return;
    }
    CallVirtual(self, 0x18, 3);
}

// 0x4AA760 (vf35): the gravity factor: more for a ridden bike (class 7) falling nose
// down with its wheels loaded and fast, by its speed and pitch (at most 4 times 0.008)
float __fastcall GetGravityFactor(CAutomobile* self, re::Edx) {
    int32_t vehicleClass;
    if (!Seats_GetOccupant(&self->seats, 0) || *Vehicle_GetVehicleClass(self, &vehicleClass) != 7 ||
        !(0.0f > self->moveSpeed.z)) {
        return 0.008f;
    }
    uint8_t* p = reinterpret_cast<uint8_t*>(self);
    if (double(At<float>(p, 0x774)) > double(0.9f) || double(At<float>(p, 0x778)) > double(0.9f) ||
        double(At<float>(p, 0x77C)) > double(0.9f) || double(At<float>(p, 0x780)) > double(0.9f)) {
        return 0.0108f;
    }
    if (0.001f < self->unknown470) {
        return 0.0108f;
    }
    Vec forward;
    if (-0.08f < Entity_GetForward(self, &forward)->z) {
        return 0.0108f;
    }
    const Vec* ms = reinterpret_cast<const Vec*>(&self->moveSpeed);
    if (Vec_Magnitude(ms) < 0.02f) {
        return 0.0108f;
    }
    double x = double(Vec_Magnitude(ms)) * 65.0 + 2.0;
    Vec forward2;
    float f = float(x - double(Entity_GetForward(self, &forward2)->z) * 6.0);
    return float(double(Min(4.0f, f)) * double(0.008f));
}

// 0x4AA8C0 (vf59): an AI driver's pedals for a wanted speed (`speed`, plus half the
// size of `a` its way): brake when faster than the model's cruise, else throttle
// (the boost 1 unless 0x42E430 and 0x4A84A0's 14), stopped: brake. (The third
// argument is unused)
void __fastcall SetDriveInput(CAutomobile* self, re::Edx, float a, float speed, float) {
    void* seats = &self->seats;
    if (!Seats_GetOccupant(seats, 0) || At<int32_t>(Seats_GetOccupant(seats, 0), 0x1310) == 0xD) {
        return;
    }
    uint8_t* p = reinterpret_cast<uint8_t*>(self);
    Vec forward;
    Vec* f = Entity_GetForward(self, &forward);
    const veh::CVector& ms = self->moveSpeed;
    int32_t sign = -1;
    float along = float(double(ms.y) * f->y + double(ms.x) * f->x + double(ms.z) * f->z);
    if (!(speed < 0.0f)) {
        sign = 1;
    }
    self->unknown474[0] = 0;
    float half = std::fabs(float(double(a) * 0.5));
    float want = float(double(half) * sign + speed);
    float& boost = At<float>(p, 0x29C);
    float& gas = At<float>(p, 0x294);
    float& brake = At<float>(p, 0x298);
    boost = 1.0f;
    void* driver = Seats_GetOccupant(seats, 0);
    float cruise = float(double(At<float>(At<uint8_t*>(driver, 0x2E0), 0x1C)) * At<float>(self->handling, 0x84));
    if (cruise < along) {
        gas = float(double(boost) * 0.0);
        if (want == 0.0f) {
            brake = 1.0f;
            return;
        }
        if (!(double(brake) < double(0.2f))) {
            return;
        }
        brake = 0.2f;
        return;
    }
    if (want != 0.0f) {
        if (!(0.0f < want)) {
            return;
        }
        if (!Fn42E430() || Get20C8704_3C() != 0xE) {
            boost = 1.0f;
        }
        gas = boost;
        brake = 0.0f;
        return;
    }
    float moving = std::fabs(along);
    gas = float(double(boost) * 0.0);
    if (!(moving < 0.001f)) {
        brake = 1.0f;
        return;
    }
    brake = 0.1f;
}

} // namespace c09autohandling

// ---- registration ---------------------------------------------------------------------

#define AUTOHANDLING_REVERSED(name, address, fn)                                             \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

AUTOHANDLING_REVERSED("CAutomobile::UpdateSurfaceRumble", 0x004A9F00, &c09autohandling::UpdateSurfaceRumble);
AUTOHANDLING_REVERSED("CAutomobile::HitPed", 0x004AA160, &c09autohandling::HitPed);
AUTOHANDLING_REVERSED("CAutomobile::KartExplode", 0x004AA530, &c09autohandling::KartExplode);
AUTOHANDLING_REVERSED("CAutomobile::UpdateKartStatus", 0x004AA700, &c09autohandling::UpdateKartStatus);
AUTOHANDLING_REVERSED("CAutomobile::GetGravityFactor", 0x004AA760, &c09autohandling::GetGravityFactor);
AUTOHANDLING_REVERSED("CAutomobile::SetDriveInput", 0x004AA8C0, &c09autohandling::SetDriveInput);
