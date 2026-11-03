// The action tree's conditions of 0x65B1C0..0x65C25F: the ped's projectile and scanned
// entity, its weapon (equipped, wanted, last, its ammo, flags, sockets, attachments), the
// prop it holds; their deleting destructors; and the catch frame the projectile catch
// works in (0x65BA70).

#include <cmath>
#include <cstdint>

#include "game/ActionFactories.h"
#include "game/ActionTracksCommon.h"
#include "game/TrackMath.h"

#include "game/Crt.h"
#include "reversed.h"
using namespace actc;
namespace af = actfac;

namespace {

GAME_FN(uint8_t*, __cdecl, GetProjectileInstance, 0x004D34B0, int index);
GAME_FN(void, __thiscall, Scanner_Set184, 0x0048C480, uint8_t* scanner, uint8_t* entity);
GAME_FN(uint8_t*, __thiscall, Scanner_Get184, 0x0048C060, uint8_t* scanner);
GAME_FN(uint8_t*, __cdecl, GetModelInfo, 0x0051C190, const char* name, int* id);
GAME_FN(int, __cdecl, GetWeaponIndex, 0x0051AEA0, const char* name);
GAME_FN(uint8_t*, __cdecl, GetWeaponInfo, 0x0051B450, int weapon);
GAME_FN(uint8_t, __thiscall, Ped_HasWeapon, 0x00474BF0, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Weapon_IsUnknown268InRange, 0x004D9440, uint8_t* weapon);
GAME_FN(int, __thiscall, Inventory_GetAmmo_486280, 0x00486280, uint8_t* inventory, int model);
GAME_FN(int, __cdecl, Prop_FindBone, 0x00655620, uint8_t* entity, int id);
GAME_FN(int, __thiscall, AnimManager_GetNodePosition, 0x006C1190, void* manager, uint8_t* anim, int bone, V3* out,
        void* node);
GAME_FN(uint8_t*, __thiscall, Placeable_GetMatrix, 0x0046DD20, uint8_t* placeable);
// (0x0045D500 stores 16 bytes through `out4`: called through re::CallVec16)
static inline void Matrix_MultiplyPoint4(float* out4, const uint8_t* m, const float* v) { re::CallVec16<0x0045D500, void>(out4, m, v); }
GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_FN(void*, __thiscall, FireManager_FindFireHitBy_x64, 0x0042DCF0, void* fires, re::Float3, re::Float3, float c);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void* FireManager_FindFireHitBy(void* a0, float a1, float a2, float a3, float a4, float a5, float a6, float a7) {
    return FireManager_FindFireHitBy_x64(a0, re::Float3{a1, a2, a3}, re::Float3{a4, a5, a6}, a7);
}
GAME_FN(uint8_t, __thiscall, FireManager_IsEntityOnFire, 0x0042DD80, void* fires, uint8_t* entity);
inline int __cdecl Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
GAME_FN(uint8_t, __cdecl, RequestWeaponModels, 0x004D9790, int weapon, int flags);
GAME_FN(uint8_t, __thiscall, HUDWeapons_Has64, 0x0055BBD0, void* hud, int weapon);
GAME_FN(uint8_t, __thiscall, Ped_HasMoney, 0x004730E0, uint8_t* ped, int32_t amount);
GAME_FN(uint8_t*, __thiscall, Ped_GetDamageRecord, 0x00474BE0, uint8_t* ped);
GAME_FN(int32_t, __thiscall, Inventory_FindSlot, 0x0045CF30, uint8_t* inventory, int32_t weapon);
GAME_FN(uint8_t*, __thiscall, Inventory_GetItemInSlot, 0x0045CC90, uint8_t* inventory, int32_t slot);
GAME_FN(int32_t, __thiscall, Inventory_Count, 0x0045CF40, uint8_t* inventory, int32_t model);
GAME_FN(void, __thiscall, WeaponInfo_Request, 0x007282A0, uint8_t* info);
GAME_FN(uint8_t, __thiscall, ModelInfo_MatchesName, 0x0050E940, uint8_t* mi, const char* name);
GAME_FN(int, __thiscall, Accessories_Count, 0x004DAB30, uint8_t* container);
GAME_FN(uint8_t*, __thiscall, Accessories_Get, 0x004DAB70, uint8_t* container, int i);
GAME_FN(uint8_t*, __cdecl, Clump_GetHierarchy, 0x005EE1A0, void* clump);
GAME_FN(uint8_t*, __cdecl, Hierarchy_GetMatrices, 0x006C7710, uint8_t* hierarchy);
GAME_FN(V3*, __cdecl, VectorAdd, 0x004129A0, V3* out, const float* a, const float* b);
// (0x0041A9A0 stores 16 bytes through `out`: called through re::CallVec16)
static inline V3* Matrix_MultiplyPoint(V3* out, const uint8_t* m, const V3* v) { return re::CallVec16<0x0041A9A0, V3*>(out, m, v); }

GAME_VAR(uint8_t*, hudWeapons, 0x00C674F0);
GAME_VAR(int32_t, fireBone_1B65300, 0x01B65300);
inline void* const fires = reinterpret_cast<void*>(0x00BD0CF8);
inline const char* const anyName = reinterpret_cast<const char*>(0x0093C2E8);       // "any"
inline const char* const unarmedName = reinterpret_cast<const char*>(0x00917FBC);   // "unarmed"

float CrtSqrt(float x) {
    return crt::Sqrtf(x);
}
// the ped's projectile (+0x544), else its scanner's entity
uint8_t* ProjectileOrScanned(uint8_t* ped) {
    uint8_t* e = At<uint8_t*>(ped, 0x544);
    return e ? e : Scanner_Get184(ped + 0x3BC);
}
inline uint8_t* Weapon(uint8_t* ped) {
    return At<uint8_t*>(ped, 0x1D0);
}
// the weapon wanted (the ped's inputs' +0x18)
inline int32_t& Desired(uint8_t* ped) {
    return At<int32_t>(At<uint8_t*>(ped, 0x2E0), 0x18);
}
// the name test the weapon conditions share: none or "any" passes as is
bool IsAny(const char* name) {
    return !name || *name == 0 || Stricmp(name, anyName) == 0;
}

// 0x65B1C0 (thiscall): the ped's projectile in flight has a shooter: the scanner's entity
uint8_t __fastcall GetProjectileOwner_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* projectile = At<uint8_t*>(ped, 0x544);
    if (!projectile || !At<void*>(projectile, 0xC4)) {
        return 0;
    }
    int32_t index = At<int32_t>(projectile, 0x238);
    if (GetProjectileInstance(index)[0x14] == 0) {
        return 0;
    }
    uint8_t* shooter = At<uint8_t*>(GetProjectileInstance(index), 4);
    if (!shooter) {
        return 0;
    }
    Scanner_Set184(OwnerPed(owner) + 0x3BC, shooter);
    return 1;
}

// 0x65B230 (thiscall): the projectile / scanned entity is the model named (+4, looked up
// once into +8)
uint8_t __fastcall EntityIsModel_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* entity = ProjectileOrScanned(OwnerPed(owner));
    if (At<int32_t>(self, 8) == -2) {
        GetModelInfo(At<const char*>(self, 4), &At<int32_t>(self, 8));
    }
    if (entity && At<int32_t>(self, 8) != -1) {
        return At<int16_t>(entity, 0x10E) == At<int32_t>(self, 8) ? 1 : 0;
    }
    return 0;
}

// 0x65B290 (thiscall): the projectile / scanned entity's speed compared (+4, +8)
uint8_t __fastcall EntitySpeed_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* entity = ProjectileOrScanned(OwnerPed(owner));
    if (!entity || !At<void*>(entity, 0xF0)) {
        return 0;
    }
    float x = At<float>(entity, 0x124);
    float y = At<float>(entity, 0x128);
    float z = At<float>(entity, 0x12C);
    float sq = std::fabs(x * x + y * y + z * z);
    return CompareOp(CrtSqrt(sq), At<float>(self, 4), At<int32_t>(self, 8));
}

// 0x65B370 (thiscall): the ped's projectile is the weapon named (+4, looked up once into
// +8)
uint8_t __fastcall EvaluatedEntityType_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* projectile = At<uint8_t*>(OwnerPed(owner), 0x544);
    if (!projectile) {
        return 0;
    }
    if (At<int32_t>(self, 8) == -1000) {
        if (!At<const char*>(self, 4)) {
            return 0;
        }
        At<int32_t>(self, 8) = GetWeaponIndex(At<const char*>(self, 4));
    }
    return At<int32_t>(self, 8) == At<int16_t>(projectile, 0x10E) ? 1 : 0;
}

// 0x65B3C0 (thiscall): the ped's weapon is damaged
uint8_t __fastcall WeaponDamaged_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (Ped_HasWeapon(ped) && Weapon_IsUnknown268InRange(Weapon(ped))) {
        return 1;
    }
    return 0;
}

// 0x65B3F0 (thiscall): the ammo of the ped's weapon (+0xC: the player's HUD weapon's)
// compared (+4, +8)
uint8_t __fastcall WeaponAmmo_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    int ammo = 0;
    if (self[0xC] != 0) {
        if (At<int32_t>(ped, 0x1310) != 0xD) {
            return 0;
        }
        ammo = Inventory_GetAmmo_486280(ped + 0x668, At<int32_t>(hudWeapons, 0xC0));
    } else if (Ped_HasWeapon(ped)) {
        ammo = Inventory_GetAmmo_486280(ped + 0x668, At<int16_t>(Weapon(ped), 0x10E));
    }
    return CompareOp(float(ammo), At<float>(self, 4), At<int32_t>(self, 8));
}

// 0x65B500 (thiscall): a fire within +4 ahead of the ped's weapon's fire bone (or 1 above
// the ped)
uint8_t __fastcall FireInRange_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    float pos[4];
    int bone = Prop_FindBone(Weapon(ped), fireBone_1B65300);
    if (bone != -1) {
        AnimManager_GetNodePosition(animManager, At<uint8_t*>(Weapon(OwnerPed(owner)), 0x114), bone,
                                    reinterpret_cast<V3*>(pos), nullptr);
    } else {
        float up[3] = {0.0f, 0.0f, 1.0f};
        Matrix_MultiplyPoint4(pos, Placeable_GetMatrix(OwnerPed(owner)), up);
    }
    float range = At<float>(self, 4);
    V3 f;
    V3* forward = Placeable_GetForward(OwnerPed(owner), &f);
    return FireManager_FindFireHitBy(fires, pos[0], pos[1], pos[2], forward->x, forward->y, forward->z, range) ? 1 : 0;
}

// the weapon wanted / last used (the ped's) against the name (+4, "any": set; its index
// looked up once into +0xC); +8 the result wanted
uint8_t WeaponNamed(uint8_t* self, int32_t current) {
    if (current == 0) {
        return self[8] == 0 ? 1 : 0;
    }
    if (IsAny(At<const char*>(self, 4))) {
        return self[8];
    }
    if (At<int32_t>(self, 0xC) == -1000) {
        At<int32_t>(self, 0xC) = GetWeaponIndex(At<const char*>(self, 4));
    }
    return current == At<int32_t>(self, 0xC) ? self[8] : (self[8] == 0 ? 1 : 0);
}

// 0x65B610 (thiscall): the weapon wanted
uint8_t __fastcall WeaponDesired_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (Desired(ped) == 0) {
        return self[8] == 0 ? 1 : 0;
    }
    if (IsAny(At<const char*>(self, 4))) {
        return self[8];
    }
    if (At<int32_t>(self, 0xC) == -1000) {
        At<int32_t>(self, 0xC) = GetWeaponIndex(At<const char*>(self, 4));
    }
    return Desired(OwnerPed(owner)) == At<int32_t>(self, 0xC) ? self[8] : (self[8] == 0 ? 1 : 0);
}

// 0x65B7D0 (thiscall): the weapon last used (+0x1574)
uint8_t __fastcall LastWeapon_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    return WeaponNamed(self, At<int32_t>(OwnerPed(owner), 0x1574));
}

// 0x65B690 (thiscall): the weapon wanted goes in the sockets (+4, +8; either may be none)
uint8_t __fastcall WeaponDesiredSocket_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    int32_t weapon = Desired(OwnerPed(owner));
    if (weapon == 0) {
        return 0;
    }
    uint8_t* info = GetWeaponInfo(weapon);
    int32_t a = At<int32_t>(self, 4);
    int32_t b = At<int32_t>(self, 8);
    if (a != 0) {
        if (b != 0) {
            return (a == At<int32_t>(info, 0x90) && b == At<int32_t>(info, 0x94)) ? 1 : 0;
        }
        return a == At<int32_t>(info, 0x90) ? 1 : 0;
    }
    if (b == 0) {
        return 0;
    }
    return b == At<int32_t>(info, 0x94) ? 1 : 0;
}

// 0x65B710 (thiscall): the weapon wanted is the one held (none wanted: yes; -1 wanted and
// held: the wish cleared)
uint8_t __fastcall WeaponDesiredIsEquiped_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    int32_t& desired = Desired(ped);
    if (desired == 0) {
        return 1;
    }
    uint8_t* weapon = Weapon(ped);
    if (!weapon || desired != At<int16_t>(weapon, 0x10E)) {
        return 0;
    }
    if (desired == -1) {
        desired = 0;
    }
    return 1;
}

// 0x65B750 (thiscall): the player's weapon wanted is the HUD's (none there: yes); +4 makes
// it so
uint8_t __fastcall WeaponDesiredIsFocusWeapon_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (At<int32_t>(ped, 0x1310) != 0xD) {
        return 0;
    }
    int32_t focus = At<int32_t>(hudWeapons, 0xC0);
    if (focus == -1) {
        return 1;
    }
    uint8_t same = Desired(ped) == focus ? 1 : 0;
    if (!same && self[4] != 0) {
        Desired(ped) = focus;
    }
    return same;
}

// 0x65B850 (thiscall): the models of the weapon (named at +4, else the one wanted)
// requested (+8 == 1: with flags 0xC); loaded
uint8_t __fastcall WeaponModelRequest_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    const char* name = At<const char*>(self, 4);
    int weapon;
    if (name && *name) {
        weapon = GetWeaponIndex(name);
    } else {
        weapon = Desired(OwnerPed(owner));
    }
    return RequestWeaponModels(weapon, self[8] == 1 ? 0xC : 0);
}

// 0x65B8A0 (thiscall): the prop's special state (+4: 1 on fire; others pass)
uint8_t __fastcall PropSpecialState_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    if (At<int32_t>(self, 4) != 1) {
        return 1;
    }
    uint8_t* prop = At<uint8_t*>(OwnerPed(owner), 0x438);
    if (!prop) {
        return 0;
    }
    return FireManager_IsEntityOnFire(fires, prop);
}

// 0x65B8E0 (thiscall): the ped's prop is two-handed (its weapon info's +0x2A)
uint8_t __fastcall PropTargetIsTwoHanded_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (ped && At<uint8_t*>(ped, 0x438)) {
        return GetWeaponInfo(At<int16_t>(At<uint8_t*>(ped, 0x438), 0x10E))[0x2A];
    }
    return 0;
}

// 0x65B910 (thiscall): the ped's weapon wasn't set up (its +0x120 is -1)
uint8_t __fastcall WeaponNeedInit_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return -1.0f == At<float>(Weapon(OwnerPed(owner)), 0x120) ? 1 : 0;
}

// 0x65B940 (thiscall): the ped's weapon is an inventory one (the HUD's)
uint8_t __fastcall WeaponIsInventory_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint16_t model = At<uint16_t>(Weapon(OwnerPed(owner)), 0x10E);
    if (model == 0xFFFF) {
        return 0;
    }
    return HUDWeapons_Has64(hudWeapons, int16_t(model));
}

// 0x65B980 (thiscall): the ped's weapon is thrown (no info: yes)
uint8_t __fastcall WeaponIsThrowable_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint16_t model = At<uint16_t>(Weapon(OwnerPed(owner)), 0x10E);
    if (model == 0xFFFF) {
        return 0;
    }
    uint8_t* info = GetWeaponInfo(int16_t(model));
    if (!info) {
        return 1;
    }
    return info[0x68];
}

// 0x65B9C0 (thiscall): the ped's weapon's age (ms since its +0x140) compared (+4, +8)
uint8_t __fastcall PropLifeTime_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint32_t age = timeInMs - At<uint32_t>(Weapon(OwnerPed(owner)), 0x140);
    return CompareOp(float(age), At<float>(self, 4), At<int32_t>(self, 8));
}

// 0x65BA20 (thiscall): the ped can afford +4 (in dollars: cents, truncated)
uint8_t __fastcall HasCash_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    float cents = At<float>(self, 4) * 100.0f;
    return Ped_HasMoney(OwnerPed(owner), int32_t(int64_t(cents)));
}

// the catch zone (0x24 bytes): where, facing (tilted up by the angle), its reach and its
// yaw / pitch limits (radians)
struct CatchFrame {
    V3 pos;       // +0x0
    V3 fwd;       // +0xC
    float reach;  // +0x18
    float yaw;    // +0x1C
    float pitch;  // +0x20
};
static_assert(sizeof(CatchFrame) == 0x24);

// 0x65BA70 (thiscall): the catch zone of an entity: at its bone (animated entities) or
// 1 above it (the reach and limits then 1.2 times), moved by the offset in its space;
// facing its forward tilted up by the angle
CatchFrame* __fastcall CatchFrame_Compute(CatchFrame* self, re::Edx, uint8_t* entity, int bone, float ox, float oy,
                                          float oz, float angle, float reach, float yaw, float pitch) {
    self->pos = V3{0.0f, 0.0f, 0.0f};
    self->fwd = V3{0.0f, 0.0f, 0.0f};
    self->pos = *reinterpret_cast<V3*>(Pos(entity));
    V3 f;
    self->fwd = *Placeable_GetForward(entity, &f);
    self->fwd.z = 0.0f;
    Vector_Normalise(&self->fwd);
    float c = trk::CrtCos(angle);
    self->fwd.x = c * self->fwd.x;
    self->fwd.y = c * self->fwd.y;
    self->fwd.z = trk::CrtSin(angle);
    const V3* at;
    V3 above;
    if (At<uint32_t>(At<uint8_t*>(entity, 0x114), 0x98) & 0x100) {
        uint8_t* matrices = Hierarchy_GetMatrices(Clump_GetHierarchy(At<void*>(entity, 0x18)));
        at = reinterpret_cast<V3*>(matrices + bone * 0x34 + 0x24);
    } else {
        V3 up{0.0f, 0.0f, 1.0f};
        at = VectorAdd(&above, Pos(entity), &up.x);
        reach = reach * 1.2f;
        yaw = yaw * 1.2f;
        pitch = 1.2f * pitch;
    }
    self->pos = *at;
    V3 offset{ox, oy, oz};
    V3 r;
    Matrix_MultiplyPoint(&r, Placeable_GetMatrix(entity), &offset);
    V3 d;
    VectorSub(&d, &r.x, Pos(entity));
    self->pos.x = d.x + self->pos.x;
    self->pos.y = d.y + self->pos.y;
    self->pos.z = d.z + self->pos.z;
    self->reach = reach;
    self->yaw = yaw;
    self->pitch = pitch;
    return self;
}

// 0x65BC30 (thiscall): the ped's attack power (its damage record's +0x3C) compared
uint8_t __fastcall AttackPower_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* record = Ped_GetDamageRecord(OwnerPed(owner));
    return CompareOp(At<float>(record, 0x3C), At<float>(self, 4), At<int32_t>(self, 8));
}

// 0x65BC80 (thiscall): the ped's weapon's flag (+8) set: its inventory item's, else the
// weapon's own
uint8_t __fastcall WeaponFlags_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    int32_t model = At<int16_t>(Weapon(ped), 0x10E);
    if (model == -1) {
        return 0;
    }
    int32_t slot = Inventory_FindSlot(At<uint8_t*>(ped, 0x1C4), model);
    if (slot != -1) {
        uint8_t* item = Inventory_GetItemInSlot(At<uint8_t*>(OwnerPed(owner), 0x1C4), slot);
        if (item) {
            return At<int32_t>(item, 4 + At<int32_t>(self, 8) * 4) != 0 ? 1 : 0;
        }
    }
    return At<int32_t>(Weapon(OwnerPed(owner)), 0x26C + At<int32_t>(self, 8) * 4) != 0 ? 1 : 0;
}

// 0x65BD00 (thiscall): the ped's weapon can be used on a bike (none held: yes)
uint8_t __fastcall WeaponCanBeUsedOnBike_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    int32_t model = At<int16_t>(Weapon(OwnerPed(owner)), 0x10E);
    if (model == -1) {
        return 1;
    }
    uint8_t* info = GetWeaponInfo(model);
    if (!info) {
        return 0;
    }
    return info[0x29];
}

// 0x65BD40 (thiscall): the ped's weapon fires when dropped
uint8_t __fastcall IsWeaponFiredWhenDropped_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint16_t model = At<uint16_t>(Weapon(OwnerPed(owner)), 0x10E);
    if (model == 0xFFFF) {
        return 0;
    }
    uint8_t* info = GetWeaponInfo(int16_t(model));
    if (!info) {
        return 0;
    }
    return info[0x2B];
}

// 0x65BD80 (thiscall): a ped passing (+0x508) becomes the scanner's; +4 forgets it
uint8_t __fastcall GetPassingPed_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* passing = At<uint8_t*>(ped, 0x508);
    if (!passing || (passing[0x108] & 7) != 3) {
        return 0;
    }
    Scanner_Set184(ped + 0x3BC, passing);
    if (self[4] != 0) {
        re::Ptr32<uint8_t>* ref = &At<uint8_t*>(OwnerPed(owner), 0x508);
        if (*ref) {
            Entity_CleanUpOldReference(*ref, ref);
            *ref = nullptr;
        }
    }
    return 1;
}

// 0x65BDF0 (thiscall): the ped's count of the weapon named (+4) compared (+8, +0xC);
// passing, +0x10 makes it the one wanted, +0x11 needs its ammo too; its models loaded
uint8_t __fastcall WeaponCount_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    int index = GetWeaponIndex(At<const char*>(self, 4));
    float count = 0.0f;
    int32_t weapon = -1;
    if (index != -1) {
        count = float(Inventory_Count(At<uint8_t*>(OwnerPed(owner), 0x1C4), index));
        weapon = index;
    }
    uint8_t result = CompareOp(count, At<float>(self, 8), At<int32_t>(self, 0xC));
    if (result && self[0x10] != 0 && weapon != 0) {
        Desired(OwnerPed(owner)) = weapon;
    }
    if (self[0x11] != 0) {
        if (weapon == -1) {
            return result;
        }
        uint8_t* mi = modelInfoPtrs[weapon];
        if (mi) {
            uint8_t* info = At<uint8_t*>(mi, 0x18);
            if (info) {
                WeaponInfo_Request(info);
                int32_t ammo = At<int32_t>(info, 0x84);
                if (ammo != -1) {
                    result = Inventory_Count(At<uint8_t*>(OwnerPed(owner), 0x1C4), ammo) > 0 ? 1 : 0;
                }
            }
        }
    }
    if (weapon == -1) {
        return result;
    }
    if (!RequestWeaponModels(weapon, 0)) {
        return 0;
    }
    return result;
}

// the weapon the ped holds, or its last one (with `last`, when it has one); its model info
int32_t HeldWeapon(uint8_t* ped, bool last, re::Ptr32<uint8_t>& mi) {
    int32_t weapon = 0;
    mi = nullptr;
    if (last && At<int32_t>(ped, 0x1574) != -1) {
        weapon = At<int32_t>(ped, 0x1574);
        mi = modelInfoPtrs[weapon];
    } else if (Ped_HasWeapon(ped)) {
        weapon = At<int16_t>(Weapon(ped), 0x10E);
        mi = modelInfoPtrs[weapon];
    }
    return weapon;
}

// 0x65BEE0 (thiscall): the ped holds (+9: or last held) the weapon named (+4: "any" any;
// none: +8 wants none held); none held: the name is none or "unarmed"
uint8_t __fastcall WeaponEquiped_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!ped) {
        return 0;
    }
    re::Ptr32<uint8_t> mi;
    HeldWeapon(ped, self[9] != 0, mi);
    const char* name = At<const char*>(self, 4);
    if (!mi) {
        if (!name || Stricmp(name, unarmedName) == 0) {
            return 1;
        }
        return 0;
    }
    if (!name) {
        return self[8] != 0 ? 0 : 1;
    }
    if (Stricmp(name, anyName) == 0) {
        return 1;
    }
    return ModelInfo_MatchesName(mi, At<const char*>(self, 4)) ? 1 : 0;
}

// 0x65BFB0 (thiscall): the kind of the weapon held (+8: or last held): +4 0 melee, 1 two
// handed melee, 2 two handed
uint8_t __fastcall WeaponEquipedType_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    re::Ptr32<uint8_t> mi;
    int32_t weapon = HeldWeapon(OwnerPed(owner), self[8] != 0, mi);
    uint8_t* info = GetWeaponInfo(weapon);
    if (!mi) {
        return 0;
    }
    switch (At<int32_t>(self, 4)) {
    case 0:
        return info[0x68];
    case 1:
        return (info[0x68] && info[0x69]) ? 1 : 0;
    case 2:
        return info[0x69];
    default:
        return 0;
    }
}

// 0x65C050 (thiscall): the weapon held is in the ped's socket (+4: its own socket, else
// +8); held last (+0xC) never
uint8_t __fastcall WeaponEquipedSocket_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    re::Ptr32<uint8_t> mi;
    int32_t weapon = HeldWeapon(OwnerPed(owner), self[0xC] != 0, mi);
    uint8_t* info = GetWeaponInfo(weapon);
    int32_t socket = self[4] != 0 ? At<int32_t>(info, 0x90) : At<int32_t>(self, 8);
    int bone = Prop_FindBone(OwnerPed(owner), socket);
    if (self[0xC] != 0) {
        return 0;
    }
    return bone == At<int32_t>(Weapon(OwnerPed(owner)), 0x138) ? 1 : 0;
}

// 0x65C0E0 (thiscall): the ped has the accessory named (+4: none, no accessories; "any",
// some)
uint8_t __fastcall PropAttached_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    int count = Accessories_Count(At<uint8_t*>(OwnerPed(owner), 0x1D4));
    const char* name = At<const char*>(self, 4);
    if (!name || *name == 0) {
        return count == 0 ? 1 : 0;
    }
    if (Stricmp(name, anyName) == 0) {
        return count != 0 ? 1 : 0;
    }
    for (int i = 0; i < count; i++) {
        uint8_t* item = Accessories_Get(At<uint8_t*>(OwnerPed(owner), 0x1D4), i);
        int32_t model = At<int16_t>(item, 0x10E);
        if (model != -1 && modelInfoPtrs[model] && ModelInfo_MatchesName(modelInfoPtrs[model], At<const char*>(self, 4))) {
            return 1;
        }
    }
    return 0;
}

// 0x65C190 (thiscall): the ped's prop's height (+0xC: its model's own; else above the
// ped) compared (+8, +4 the operator)
uint8_t __fastcall PropTargetHeight_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!ped) {
        return 0;
    }
    uint8_t* prop = At<uint8_t*>(ped, 0x438);
    if (!prop) {
        return 0;
    }
    if (self[0xC] != 0) {
        uint8_t* mi = modelInfoPtrs[At<int16_t>(prop, 0x10E)];
        if (!mi || !At<uint8_t*>(mi, 0xC)) {
            return 0;
        }
        uint8_t* col = At<uint8_t*>(mi, 0xC);
        float height = At<float>(col, 0x28) - At<float>(col, 0x18);
        return CompareOp(height, At<float>(self, 8), At<int32_t>(self, 4));
    }
    float height = Pos(prop)[2] - Pos(ped)[2];
    return CompareOp(height, At<float>(self, 8), At<int32_t>(self, 4));
}

} // namespace

// (variadic: the template arguments' commas)
#define WPNCOND2_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)


using af::Vt;
using af::KeyDtor;

// deleting destructors (thiscall, the flags)
WPNCOND2_REVERSED("ConditionEvaluatedEntityType::DeletingDestruct", 0x0065B340, &af::DeletingDtor<Vt(0x0093C274), KeyDtor(0x4)>);
WPNCOND2_REVERSED("ConditionWeaponCount::DeletingDestruct", 0x0065B480, &af::DeletingDtor<Vt(0x0093C288), KeyDtor(0x4)>);
WPNCOND2_REVERSED("ConditionCollisionWeaponType::DeletingDestruct", 0x0065B4B0, &af::DeletingDtor<Vt(0x0093C29C), KeyDtor(0xC)>);
WPNCOND2_REVERSED("ConditionAltWeaponCollisionType::DeletingDestruct", 0x0065B4E0, &af::DeletingDtor<Vt(0x0093C2B0)>);
WPNCOND2_REVERSED("ConditionWeaponDesired::DeletingDestruct", 0x0065B5E0, &af::DeletingDtor<Vt(0x0093C2C4), KeyDtor(0x4)>);
WPNCOND2_REVERSED("ConditionLastWeapon::DeletingDestruct", 0x0065B7A0, &af::DeletingDtor<Vt(0x0093C2D8), KeyDtor(0x4)>);

// Evaluate (vf1, thiscall, the owner)
WPNCOND2_REVERSED("ConditionGetProjectileOwner::Evaluate", 0x0065B1C0, &GetProjectileOwner_Evaluate);
WPNCOND2_REVERSED("ConditionEntityIsModel::Evaluate", 0x0065B230, &EntityIsModel_Evaluate);
WPNCOND2_REVERSED("ConditionEntitySpeed::Evaluate", 0x0065B290, &EntitySpeed_Evaluate);
WPNCOND2_REVERSED("ConditionEvaluatedEntityType::Evaluate", 0x0065B370, &EvaluatedEntityType_Evaluate);
WPNCOND2_REVERSED("ConditionWeaponDamaged::Evaluate", 0x0065B3C0, &WeaponDamaged_Evaluate);
WPNCOND2_REVERSED("ConditionWeaponAmmo::Evaluate", 0x0065B3F0, &WeaponAmmo_Evaluate);
WPNCOND2_REVERSED("ConditionFireInRange::Evaluate", 0x0065B500, &FireInRange_Evaluate);
WPNCOND2_REVERSED("ConditionWeaponDesired::Evaluate", 0x0065B610, &WeaponDesired_Evaluate);
WPNCOND2_REVERSED("ConditionWeaponDesiredSocket::Evaluate", 0x0065B690, &WeaponDesiredSocket_Evaluate);
WPNCOND2_REVERSED("ConditionWeaponDesiredIsEquiped::Evaluate", 0x0065B710, &WeaponDesiredIsEquiped_Evaluate);
WPNCOND2_REVERSED("ConditionWeaponDesiredIsFocusWeapon::Evaluate", 0x0065B750, &WeaponDesiredIsFocusWeapon_Evaluate);
WPNCOND2_REVERSED("ConditionLastWeapon::Evaluate", 0x0065B7D0, &LastWeapon_Evaluate);
WPNCOND2_REVERSED("ConditionWeaponModelRequest::Evaluate", 0x0065B850, &WeaponModelRequest_Evaluate);
WPNCOND2_REVERSED("ConditionPropSpecialState::Evaluate", 0x0065B8A0, &PropSpecialState_Evaluate);
WPNCOND2_REVERSED("ConditionPropTargetIsTwoHanded::Evaluate", 0x0065B8E0, &PropTargetIsTwoHanded_Evaluate);
WPNCOND2_REVERSED("ConditionWeaponNeedInit::Evaluate", 0x0065B910, &WeaponNeedInit_Evaluate);
WPNCOND2_REVERSED("ConditionWeaponIsInventory::Evaluate", 0x0065B940, &WeaponIsInventory_Evaluate);
WPNCOND2_REVERSED("ConditionWeaponIsThrowable::Evaluate", 0x0065B980, &WeaponIsThrowable_Evaluate);
WPNCOND2_REVERSED("ConditionPropLifeTime::Evaluate", 0x0065B9C0, &PropLifeTime_Evaluate);
WPNCOND2_REVERSED("ConditionHasCash::Evaluate", 0x0065BA20, &HasCash_Evaluate);
WPNCOND2_REVERSED("CatchZone::Compute_65BA70", 0x0065BA70, &CatchFrame_Compute);
WPNCOND2_REVERSED("AttackPowerCond::Evaluate", 0x0065BC30, &AttackPower_Evaluate);
WPNCOND2_REVERSED("ConditionWeaponFlags::Evaluate", 0x0065BC80, &WeaponFlags_Evaluate);
WPNCOND2_REVERSED("ConditionWeaponCanBeUsedOnBike::Evaluate", 0x0065BD00, &WeaponCanBeUsedOnBike_Evaluate);
WPNCOND2_REVERSED("ConditionIsWeaponFiredWhenDropped::Evaluate", 0x0065BD40, &IsWeaponFiredWhenDropped_Evaluate);
WPNCOND2_REVERSED("ConditionGetPassingPed::Evaluate", 0x0065BD80, &GetPassingPed_Evaluate);
WPNCOND2_REVERSED("ConditionWeaponCount::Evaluate", 0x0065BDF0, &WeaponCount_Evaluate);
WPNCOND2_REVERSED("ConditionWeaponEquiped::Evaluate", 0x0065BEE0, &WeaponEquiped_Evaluate);
WPNCOND2_REVERSED("ConditionWeaponEquipedType::Evaluate", 0x0065BFB0, &WeaponEquipedType_Evaluate);
WPNCOND2_REVERSED("ConditionWeaponEquipedSocket::Evaluate", 0x0065C050, &WeaponEquipedSocket_Evaluate);
WPNCOND2_REVERSED("ConditionPropAttached::Evaluate", 0x0065C0E0, &PropAttached_Evaluate);
WPNCOND2_REVERSED("ConditionPropTargetHeight::Evaluate", 0x0065C190, &PropTargetHeight_Evaluate);
