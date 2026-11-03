// The action tree's weapon and camera tracks of 0x65E2C0..0x65EBAC: the photo camera's
// iris, a bone's position on a ped or its bike, the weapon's scale and charge, and the
// weapon's muzzle position a shot starts from; their field defaults and destructors.

#include <bit>
#include <cstdint>

#include "game/ActionFactories.h"
#include "game/ActionTracksCommon.h"
#include "game/PropTracks.h"

using namespace actc;
namespace af = actfac;

namespace {

GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(void, __thiscall, PlayerPed_TakeYearbookPhoto, 0x0049B9E0, uint8_t* ped);
GAME_FN(uint8_t, __cdecl, Model_FindDummy, 0x00612380, V3* out, const char* name, uint8_t* model);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void*, __thiscall, ElementTable_FindById, 0x00465580, void* table, uint32_t id, int* outIndex);
GAME_FN(int, __thiscall, AnimManager_GetNodePosition, 0x006C1190, void* manager, uint8_t* anim, int bone, V3* out,
        void* node);
GAME_FN(void, __thiscall, Weapon_SetScale, 0x004D9970, uint8_t* weapon, const float* scale);
GAME_FN(uint8_t*, __thiscall, Placeable_GetMatrix, 0x0046DD20, uint8_t* placeable);
// (0x0045D500 stores 16 bytes through `out4`: called through re::CallVec16)
static inline void Matrix_MultiplyPoint4(float* out4, const uint8_t* m, const float* v) { re::CallVec16<0x0045D500, void>(out4, m, v); }
// (0x0041A9A0 stores 16 bytes through `out`: called through re::CallVec16)
static inline V3* Matrix_MultiplyPoint(V3* out, const uint8_t* m, const float* v) { return re::CallVec16<0x0041A9A0, V3*>(out, m, v); }

GAME_VAR(uint8_t*, hudWeapons, 0x00C674F0);   // +0x128 the iris
GAME_VAR(uint8_t*, hudPhoto, 0x00C674E0);
GAME_VAR(float, timeScale_C1A9A8, 0x00C1A9A8);
GAME_VAR(int32_t, fireAmmoWeapon, 0x00B31C9C);
GAME_VAR(int32_t, halfChargeModel, 0x00BF444C);
GAME_ARRAY(float, fireStartOffset, 0x01B65C3C);
inline const char* const muzzleName = reinterpret_cast<const char*>(0x009146A8);   // "Muzzle"

inline uint8_t* OwnerWeapon(uint8_t* owner) {
    return At<uint8_t*>(OwnerPed(owner), 0x1D0);
}

// 0x65E2C0 (thiscall): CameraIrisControlTrack::Start: the iris open, the photo HUD
// frozen, time stopped (its scale kept, +0x30), closing (+0x2C -1)
uint8_t __fastcall CameraIrisControl_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x28) = owner;
    At<float>(hudWeapons, 0x128) = 1.0f;
    At<float>(self, 0x2C) = -1.0f;
    hudPhoto[0x38] = 1;
    At<float>(self, 0x30) = timeScale_C1A9A8;
    timeScale_C1A9A8 = 0.0f;
    return 1;
}

// 0x65E300 (thiscall): CameraIrisControlTrack::Update: +0x19 the player's (the pad's
// trigger 0x13 moves it, unless 0xF is held); else it closes to +0x1C, opens to +0x20 and
// ends on the second time it's open; the iris kept within
uint8_t __fastcall CameraIrisControl_Update(uint8_t* self, re::Edx, float) {
    uint8_t* hud = hudWeapons;
    if (self[0x19] != 0) {
        if (!(Pad_GetValue(0xF, 0) == 0.0f)) {
            return 1;
        }
        float iris = At<float>(hud, 0x128);
        float rate = At<float>(self, 0x24);
        iris = Pad_GetValue(0x13, 0) * rate + iris;
        if (At<float>(self, 0x1C) > iris) {
            iris = At<float>(self, 0x1C);
        }
        if (At<float>(self, 0x20) < iris) {
            iris = At<float>(self, 0x20);
        }
        At<float>(hud, 0x128) = iris;
        return 1;
    }
    float iris = At<float>(self, 0x24) * At<float>(self, 0x2C) + At<float>(hud, 0x128);
    if (At<float>(self, 0x1C) > iris) {
        iris = At<float>(self, 0x1C);
        At<float>(self, 0x2C) = 1.0f;
    }
    if (At<float>(self, 0x20) < iris) {
        iris = At<float>(self, 0x20);
        if (1.0f == At<float>(self, 0x2C)) {
            At<float>(self, 0x2C) = 2.0f;
        } else if (2.0f == At<float>(self, 0x2C)) {
            At<float>(hud, 0x128) = iris;
            return 0;
        }
    }
    At<float>(hud, 0x128) = iris;
    return 1;
}

// 0x65E460 (thiscall): CameraIrisControlTrack::End: the iris open, the photo taken (+0x18:
// the yearbook's), time back
uint8_t __fastcall CameraIrisControl_End(uint8_t* self, re::Edx) {
    At<float>(hudWeapons, 0x128) = 1.0f;
    if (self[0x18] != 0) {
        hudPhoto[0x36] = 1;
        PlayerPed_TakeYearbookPhoto(playerPed);
    }
    hudPhoto[0x38] = 0;
    timeScale_C1A9A8 = At<float>(self, 0x30);
    return 1;
}

// 0x65E670 (cdecl): the named dummy's position on the ped's model, else the named bone's
// on its bike
uint8_t __cdecl PedOrBike_FindBonePosition(V3* out, const char* name, uint8_t* ped) {
    if (Model_FindDummy(out, name, At<uint8_t*>(ped, 0x114))) {
        return 1;
    }
    uint8_t* bike = At<uint8_t*>(ped, 0x1554);
    if (!bike || !name) {
        return 0;
    }
    void* table = At<void*>(At<uint8_t*>(bike, 0x114), 0x94);
    int index = 0;
    if (!ElementTable_FindById(table, HashName(name), &index)) {
        return 0;
    }
    AnimManager_GetNodePosition(animManager, At<uint8_t*>(At<uint8_t*>(ped, 0x1554), 0x114), index, out, nullptr);
    return 1;
}

// 0x65E740 (thiscall): WeaponScaleTrack::Update: the weapon's (+0x3C) scale grown at the
// rates (+0x30) within +0x18 .. +0x24
uint8_t __fastcall WeaponScale_Update(uint8_t* self, re::Edx, float dt) {
    uint8_t* weapon = At<uint8_t*>(self, 0x3C);
    if (!weapon) {
        return 0;
    }
    float scale[3];
    scale[0] = At<float>(self, 0x30) * dt + At<float>(weapon, 0x28C);
    scale[1] = At<float>(self, 0x34) * dt + At<float>(weapon, 0x290);
    scale[2] = dt * At<float>(self, 0x38) + At<float>(weapon, 0x294);
    for (int i = 0; i < 3; i++) {
        float low = At<float>(self, 0x18 + i * 4);
        float high = At<float>(self, 0x24 + i * 4);
        if (low > scale[i]) {
            scale[i] = low;
        } else if (high < scale[i]) {
            scale[i] = high;
        }
    }
    Weapon_SetScale(weapon, scale);
    return 1;
}

// 0x65E880 (thiscall): WeaponScaleTrack::End: the weapon at the end scale (+0x24)
uint8_t __fastcall WeaponScale_End(uint8_t* self, re::Edx) {
    uint8_t* weapon = At<uint8_t*>(self, 0x3C);
    if (weapon) {
        float scale[3] = {At<float>(self, 0x24), At<float>(self, 0x28), At<float>(self, 0x2C)};
        Weapon_SetScale(weapon, scale);
    }
    return 0;
}

// 0x65E900 (thiscall): WeaponAnimationTrack::Resolve: the blend is the ped's weapon's
uint8_t* __fastcall WeaponAnimation_Resolve(uint8_t* self, re::Edx) {
    uint8_t* weapon = OwnerWeapon(At<uint8_t*>(self, 0x40));
    if (!weapon) {
        return nullptr;
    }
    uint8_t* blend = At<uint8_t*>(weapon, 0x114);
    At<uint8_t*>(self, 0x4C) = blend;
    return blend;
}

// 0x65E920 (thiscall): WeaponSetChargeTrack::Start: the weapon's charge (+0x128) set
uint8_t __fastcall WeaponSetCharge_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    At<float>(OwnerWeapon(owner), 0x128) = At<float>(self, 0x18);
    return 1;
}

// 0x65E940 (thiscall): PropStartLifeTrack::Start: the weapon's life starts now (+0x140)
uint8_t __fastcall PropStartLife_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    At<uint32_t>(OwnerWeapon(owner), 0x140) = timeInMs;
    return 1;
}

// 0x65E970 (thiscall): WeaponIncrementChargeTrack::Update: the weapon's charge up by the
// time (half as fast for the one model)
uint8_t __fastcall WeaponIncrementCharge_Update(uint8_t* self, re::Edx, float dt) {
    uint8_t* weapon = OwnerWeapon(At<uint8_t*>(self, 0x18));
    float charge = At<float>(weapon, 0x128);
    float rate = At<int16_t>(weapon, 0x10E) == halfChargeModel ? 0.5f : 1.0f;
    At<float>(OwnerWeapon(At<uint8_t*>(self, 0x18)), 0x128) = rate * dt + charge;
    return 1;
}

// 0x65E9E0 (thiscall): WeaponFireTrack's shot start: the entity's (+0x40) weapon's
// muzzle (its own ammo: the weapon's bone +0x138; else its "Muzzle" node; else 1 above
// the entity) plus the start offset turned with the entity
V3* __fastcall WeaponFire_MuzzlePosition(uint8_t* self, re::Edx, V3* out) {
    int32_t ammo = fireAmmoWeapon;
    if (ammo == -1) {
        ammo = At<int32_t>(OwnerWeapon(At<uint8_t*>(self, 0x3C)), 0x12C);
    }
    float pos[4];
    bool found = false;
    if (ammo == At<int16_t>(OwnerWeapon(At<uint8_t*>(self, 0x3C)), 0x10E)) {
        uint8_t* entity = At<uint8_t*>(self, 0x40);
        int32_t bone = At<int32_t>(At<uint8_t*>(entity, 0x1D0), 0x138);
        uint8_t* anim = At<uint8_t*>(entity, 0x114);
        if (bone != -1 && (At<uint32_t>(anim, 0x98) & 0x100)) {
            AnimManager_GetNodePosition(animManager, anim, bone, reinterpret_cast<V3*>(pos), nullptr);
            found = true;
        }
    } else {
        uint32_t key;
        NameKey_Construct(&key, muzzleName);
        proptr::NodeTransform transform;
        transform.scale = 1.0f;
        if (proptr::GetNodeTransform(At<uint8_t*>(At<uint8_t*>(self, 0x40), 0x1D0), int(key), &transform, 0)) {
            found = true;
            pos[0] = transform.pos.x;
            pos[1] = transform.pos.y;
            pos[2] = transform.pos.z;
        }
        NameKey_Destruct(&key);
    }
    if (!found) {
        float up[3] = {0.0f, 0.0f, 1.0f};
        Matrix_MultiplyPoint4(pos, Placeable_GetMatrix(At<uint8_t*>(self, 0x40)), up);
    }
    uint8_t* entity = At<uint8_t*>(self, 0x40);
    V3 at = *reinterpret_cast<V3*>(Pos(entity));
    pos[0] = pos[0] - at.x;
    pos[1] = pos[1] - at.y;
    pos[2] = pos[2] - at.z;
    V3 turned;
    Matrix_MultiplyPoint(&turned, Placeable_GetMatrix(entity), fireStartOffset);
    out->x = turned.x + pos[0];
    out->y = turned.y + pos[1];
    out->z = turned.z + pos[2];
    return out;
}

// 0x65E4B0 (thiscall): AmmoAttachmentTrack's field defaults
void __fastcall AmmoAttachment_InitFields(uint8_t* self, re::Edx) {
    At<float>(self, 0xC) = 0.0f;
    At<float>(self, 0x10) = -1.0f;
    self[8] = 0;
    At<int32_t>(self, 0x18) = 3;
    ClearName(self, 0x1C);
    SetNameRaw(self, 0x20, emptyString);
    self[0x24] = 0;
}

} // namespace

// (variadic: the template arguments' commas)
#define WPNTRK2_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

using af::Construct;
using af::Fields;
using af::Vt;
using af::KeyDtor;
using af::Call;
using af::F32;
using af::I32;
using af::U8;
using af::TrackBaseDtor;

// field defaults (thiscall, after construction)
WPNTRK2_REVERSED("AmmoAttachmentTrack::InitFields", 0x0065E4B0, &AmmoAttachment_InitFields);
WPNTRK2_REVERSED("AmmoChangeTrack::InitFields", 0x0065E550, &Fields<U8(0x8, 0), F32(0xC, 0.0f), I32(0x18, 0xFFFFFFFF)>);
WPNTRK2_REVERSED("ExecuteOnTargetTrack::InitFields", 0x0065E5A0, &Fields<Call(0x0049A650), U8(0x8, 0)>);
WPNTRK2_REVERSED("CTraceTrack::InitFields", 0x0065E5C0, &Fields<U8(0x8, 0), U8(0x1C, 0), af::ClearName(0x18)>);
WPNTRK2_REVERSED("WeaponCollisionTrack::InitFields", 0x0065E610, &Fields<Call(0x005F76D0), F32(0x5C, 0.300000012f), I32(0x218, 0), U8(0x21C, 0), U8(0x21D, 0), U8(0x21E, 0)>);
WPNTRK2_REVERSED("WeaponScaleTrack::InitFields", 0x0065E710, &Fields<U8(0x8, 0), F32(0x18, 1.0f), F32(0x1C, 1.0f), F32(0x20, 1.0f), F32(0x24, 1.0f), F32(0x28, 1.0f), F32(0x2C, 1.0f)>);
WPNTRK2_REVERSED("WeaponPlayerFireAutoTargetTrack::InitFields", 0x0065E8D0, &Fields<F32(0xC, 0.0f), U8(0x8, 0), U8(0x18, 0), F32(0x10, -1.0f)>);

// destructors (thiscall)
WPNTRK2_REVERSED("AmmoAttachmentTrack::Destruct", 0x0065E520, &Fields<Vt(0x00935D1C), KeyDtor(0x20), KeyDtor(0x1C), TrackBaseDtor>);
WPNTRK2_REVERSED("AmmoChangeTrack::Destruct", 0x0065E570, &Fields<Vt(0x00935CEC), TrackBaseDtor>);
WPNTRK2_REVERSED("WeaponRemoveCollisionProjectileTrack::Destruct", 0x0065E580, &Fields<Vt(0x0093644C), TrackBaseDtor>);
WPNTRK2_REVERSED("CPedTargetLockTrack::Destruct", 0x0065E590, &Fields<Vt(0x00935D64), TrackBaseDtor>);
WPNTRK2_REVERSED("ExecuteOnTargetTrack::Destruct", 0x0065E5B0, &Fields<Vt(0x00938314), Call(0x005F7BC0)>);
WPNTRK2_REVERSED("CTraceTrack::Destruct", 0x0065E5F0, &Fields<Vt(0x00935D7C), KeyDtor(0x18), TrackBaseDtor>);
WPNTRK2_REVERSED("WeaponCollisionTrack::Destruct", 0x0065E640, &Fields<Vt(0x009382F0), KeyDtor(0x214), KeyDtor(0x210), Call(0x005F8200)>);
WPNTRK2_REVERSED("SetWeaponFlagsTrack::Destruct", 0x0065E700, &Fields<Vt(0x00935F74), TrackBaseDtor>);
WPNTRK2_REVERSED("WeaponScaleTrack::Destruct", 0x0065E730, &Fields<Vt(0x009360C4), TrackBaseDtor>);
WPNTRK2_REVERSED("WeaponSpawnDamageEffectTrack::Destruct", 0x0065E8B0, &Fields<Vt(0x00935F8C), TrackBaseDtor>);
WPNTRK2_REVERSED("WeaponSetNextDamageLevelTrack::Destruct", 0x0065E8C0, &Fields<Vt(0x00935FA4), TrackBaseDtor>);
WPNTRK2_REVERSED("WeaponPlayerFireAutoTargetTrack::Destruct", 0x0065E8F0, &Fields<Vt(0x00936494), TrackBaseDtor>);

// the tracks' virtuals and helpers
WPNTRK2_REVERSED("CameraIrisControlTrack::Start", 0x0065E2C0, &CameraIrisControl_Start);
WPNTRK2_REVERSED("CameraIrisControlTrack::Update", 0x0065E300, &CameraIrisControl_Update);
WPNTRK2_REVERSED("CameraIrisControlTrack::End", 0x0065E460, &CameraIrisControl_End);
WPNTRK2_REVERSED("Ped::FindBonePosition_65E670", 0x0065E670, &PedOrBike_FindBonePosition);
WPNTRK2_REVERSED("WeaponScaleTrack::Update", 0x0065E740, &WeaponScale_Update);
WPNTRK2_REVERSED("WeaponScaleTrack::End", 0x0065E880, &WeaponScale_End);
WPNTRK2_REVERSED("WeaponAnimationTrack::Resolve", 0x0065E900, &WeaponAnimation_Resolve);
WPNTRK2_REVERSED("WeaponSetChargeTrack::Start", 0x0065E920, &WeaponSetCharge_Start);
WPNTRK2_REVERSED("PropStartLifeTrack::Start", 0x0065E940, &PropStartLife_Start);
WPNTRK2_REVERSED("WeaponIncrementChargeTrack::Update", 0x0065E970, &WeaponIncrementCharge_Update);
WPNTRK2_REVERSED("WeaponFireTrack::MuzzlePosition", 0x0065E9E0, &WeaponFire_MuzzlePosition);
