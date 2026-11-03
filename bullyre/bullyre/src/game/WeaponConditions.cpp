// The action tree's conditions of 0x65A970..0x65B1B8: buttons, the weapon a hit came
// from (the owner's +0x10 damage: +0x34 explosive, +0x5E the weapon, +0x60 the collision
// type), the camera's and HUD's state; the conditions' constructors (their CreateAt
// functions call these), field defaults (after construction) and destructors.

#include <cstdint>

#include "game/ActionFactories.h"
#include "game/ActionTracksCommon.h"
#include "game/PropTracks.h"

using namespace actc;
namespace af = actfac;

namespace {

GAME_FN(int, __cdecl, GetWeaponIndex, 0x0051AEA0, const char* name);
GAME_FN(uint8_t*, __cdecl, GetWeaponInfo, 0x0051B450, int weapon);
GAME_FN(int32_t, __thiscall, Camera_GetUnknown1A8, 0x004F12A0, void* camera);
GAME_FN(int32_t, __thiscall, Transition_IsActive, 0x00856850, void* transition);
GAME_FN(int, __thiscall, Camera_GetFadeState, 0x004F12F0, void* camera);

GAME_ARRAY(uint8_t, buttonStates, 0x00C3CEEC);   // per button: now, before
GAME_VAR(uint8_t, lockButtonInputsExcept, 0x00C1AF8C);
GAME_VAR(int32_t, cameraDesired_C3CED8, 0x00C3CED8);
GAME_VAR(int32_t, cameraMode_C3CCF8, 0x00C3CCF8);
GAME_VAR(int32_t, cameraLock_C3CE0C, 0x00C3CE0C);
GAME_VAR(uint8_t, cameraReset_C3CEE0, 0x00C3CEE0);
GAME_VAR(int32_t, widescreen_C3CD0C, 0x00C3CD0C);
GAME_VAR(uint8_t*, hudWeapons, 0x00C674F0);
GAME_VAR(uint8_t*, hudCamera_C674E0, 0x00C674E0);
GAME_ARRAY(uint8_t*, hudComponents, 0x00C674A0);
inline void* const theCamera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const cameraTransition = reinterpret_cast<void*>(0x00A45500);

inline uint8_t* Damage(uint8_t* owner) {
    return At<uint8_t*>(owner, 0x10);
}

// 0x65A970 (thiscall): the button (+4) in the state (+8: 0 down, 1 up, 2 / 3 just
// pressed); a button locked out (while buttons are locked, its own flag clear) is up
uint8_t __fastcall ButtonState_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    uint32_t button = At<uint32_t>(self, 4);
    // (read before the range check, as the game does)
    uint8_t now = buttonStates[button * 2];
    uint8_t before = buttonStates[button * 2 + 1];
    static const int8_t allowFlag[9] = {0xB, 3, 4, 7, 8, -1, 0xA, 3, 4};   // 0xC1AF8C + n
    if (button <= 8 && allowFlag[button] >= 0 && lockButtonInputsExcept != 0 &&
        reinterpret_cast<uint8_t*>(0x00C1AF8C)[allowFlag[button]] == 0) {
        now = 0;
        before = 0;
    }
    switch (At<int32_t>(self, 8)) {
    case 0:
        return now;
    case 1:
        return now == 0 ? 1 : 0;
    case 2:
    case 3:
        return (now && !before) ? 1 : 0;
    default:
        return 0;
    }
}

// 0x65AB90 (thiscall): +8 set: the hit's weapon is the one named (+0xC, looked up once
// into +0x10); else the weapon's collision kind (+4: 0 a melee hit, 1 a two-handed
// swing, 2 a thrown hit, 3 type 4)
uint8_t __fastcall CollisionWeaponType_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* damage = Damage(owner);
    int32_t type = At<int32_t>(damage, 0x60);
    int32_t weapon = At<int16_t>(damage, 0x5E);
    if (self[8] != 0) {
        if (At<int32_t>(self, 0x10) == -1000) {
            At<int32_t>(self, 0x10) = GetWeaponIndex(At<const char*>(self, 0xC));
        }
        return At<int32_t>(self, 0x10) == weapon ? 1 : 0;
    }
    if (weapon < 0) {
        return 0;
    }
    uint8_t* info = GetWeaponInfo(weapon);
    switch (At<uint32_t>(self, 4)) {
    case 0:
        return (info[0x68] && (type == 2 || type == 3)) ? 1 : 0;
    case 1:
        return (info[0x68] && info[0x69] && type == 2) ? 1 : 0;
    case 2:
        return (info[0x69] && type == 1) ? 1 : 0;
    case 3:
        return type == 4 ? 1 : 0;
    default:
        return 0;
    }
}

// 0x65AC90 (thiscall): the hit's collision type is 3
uint8_t __fastcall AltWeaponCollisionType_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return At<int32_t>(Damage(owner), 0x60) == 3 ? 1 : 0;
}
// 0x65ACB0 (thiscall): the hit was an explosion's
uint8_t __fastcall DamageFromExplosion_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return Damage(owner)[0x34];
}
// 0x65ACC0 (thiscall): the hit was a fire's (weapon -7)
uint8_t __fastcall DamageFromBurningFire_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return At<int16_t>(Damage(owner), 0x5E) == -7 ? 1 : 0;
}

// 0x65AD40 (thiscall): ConditionWeaponEquipedSocket's field defaults
void __fastcall WeaponEquipedSocket_InitFields(uint8_t* self, re::Edx) {
    self[4] = 0;
    proptr::SetNoneRaw(self, 8);
    self[0xC] = 0;
}
// 0x65ADC0 (thiscall): ConditionWeaponDesiredSocket's field defaults
void __fastcall WeaponDesiredSocket_InitFields(uint8_t* self, re::Edx) {
    proptr::SetNoneRaw(self, 4);
    proptr::SetNoneRaw(self, 8);
}
// 0x65AF40 (thiscall): ConditionTargetProp's field defaults
void __fastcall TargetProp_InitFields(uint8_t* self, re::Edx) {
    At<float>(self, 4) = 1.0f;
    self[0xC] = 1;
    self[0xD] = 0;
    proptr::SetNoneRaw(self, 0x10);
    At<float>(self, 8) = 0.0f;
    self[0x20] = 0;
    self[0x28] = 0;
    At<float>(self, 0x24) = 2.0f;
    self[0x29] = 0;
    At<float>(self, 0x18) = 1000.0f;
}

// 0x65AFE0 / 0x65B000 (thiscall): the camera wanted / active is +4
uint8_t __fastcall CameraDesired_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    return cameraDesired_C3CED8 == At<int32_t>(self, 4) ? 1 : 0;
}
uint8_t __fastcall CameraActive_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    return cameraMode_C3CCF8 == At<int32_t>(self, 4) ? 1 : 0;
}
// 0x65B020 / 0x65B030 (thiscall): the weapon HUD locked / its wheel on
uint8_t __fastcall WeaponHudLocked_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    return hudWeapons[0x13C];
}
uint8_t __fastcall WeaponWheelOn_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    return hudWeapons[0x13D];
}
// 0x65B040 (thiscall): the HUD component (+4) is visible (its vf 0x10)
uint8_t __fastcall HudComponentVisible_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    return vc::Call<uint8_t>(hudComponents[At<int32_t>(self, 4)], 0x10);
}
// 0x65B060 (thiscall): the camera isn't locked
uint8_t __fastcall CameraLocked_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    return cameraLock_C3CE0C == 0 ? 1 : 0;
}
// 0x65B070 (thiscall): the camera allows a scripted change
uint8_t __fastcall CameraAllowScriptedChange_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    return Camera_GetUnknown1A8(theCamera) != 0 ? 1 : 0;
}
// 0x65B090 (thiscall): the camera wanted is being reset
uint8_t __fastcall CameraDesiredReset_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    return (cameraReset_C3CEE0 != 0 && cameraDesired_C3CED8 != 0) ? 1 : 0;
}
// 0x65B0B0 (thiscall): a camera transition is on
uint8_t __fastcall CameraBlending_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    return Transition_IsActive(cameraTransition) != 0 ? 1 : 0;
}
// 0x65B0D0 (thiscall): the camera is fading (state 2)
uint8_t __fastcall CameraFading_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    return Camera_GetFadeState(theCamera) == 2 ? 1 : 0;
}
// 0x65B120 (thiscall): the camera is in widescreen
uint8_t __fastcall CameraInWidescreen_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    return widescreen_C3CD0C != 0 ? 1 : 0;
}
// 0x65B1B0 (thiscall): the photo HUD's freeze is on
uint8_t __fastcall IsTakingPictureFreeze_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    return hudCamera_C674E0[0x38];
}

} // namespace

// (variadic: the template arguments' commas)
#define WPNCOND_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

using af::Construct;
using af::Fields;
using af::Vt;
using af::KeyDtor;
using af::Key;
using af::CondBase;
using af::ClearName;
using af::F32;
using af::I32;
using af::U8;

// constructors (thiscall)
WPNCOND_REVERSED("ConditionEvaluatedEntityType::Construct", 0x0065AA70, &Construct<CondBase, Vt(0x0093C274), Key(0x4), I32(0x8, 0xFFFFFC18)>);
WPNCOND_REVERSED("ConditionWeaponCount::Construct", 0x0065AAC0, &Construct<CondBase, Vt(0x0093C288), Key(0x4)>);
WPNCOND_REVERSED("ConditionCollisionWeaponType::Construct", 0x0065AB20, &Construct<CondBase, Vt(0x0093C29C), Key(0xC), I32(0x10, 0xFFFFFC18)>);
WPNCOND_REVERSED("ConditionAltWeaponCollisionType::Construct", 0x0065AC70, &Construct<CondBase, Vt(0x0093C2B0)>);
WPNCOND_REVERSED("ConditionWeaponDesired::Construct", 0x0065AD90, &Construct<CondBase, Vt(0x0093C2C4), Key(0x4), I32(0xC, 0xFFFFFC18)>);
WPNCOND_REVERSED("ConditionLastWeapon::Construct", 0x0065AE40, &Construct<CondBase, Vt(0x0093C2D8), Key(0x4), I32(0xC, 0xFFFFFC18)>);

// field defaults (thiscall, after construction)
WPNCOND_REVERSED("ConditionWeaponAmmo::InitFields", 0x0065AAA0, &Fields<I32(0x8, 3), F32(0x4, 0.0f), U8(0xC, 0)>);
WPNCOND_REVERSED("ConditionWeaponCount::InitFields", 0x0065AAE0, &Fields<F32(0x8, 0.0f), I32(0xC, 3), ClearName(0x4), U8(0x10, 0), U8(0x11, 0)>);
WPNCOND_REVERSED("ConditionCollisionWeaponType::InitFields", 0x0065AB50, &Fields<I32(0x4, 2), U8(0x8, 1), ClearName(0xC)>);
WPNCOND_REVERSED("ConditionWeaponEquiped::InitFields", 0x0065ACE0, &Fields<U8(0x8, 1), ClearName(0x4), U8(0x9, 0)>);
WPNCOND_REVERSED("ConditionWeaponEquipedType::InitFields", 0x0065AD20, &Fields<I32(0x4, 2), U8(0x8, 0)>);  // also ConditionCanSecondaryMissionPlay
WPNCOND_REVERSED("ConditionWeaponEquipedSocket::InitFields", 0x0065AD40, &WeaponEquipedSocket_InitFields);
WPNCOND_REVERSED("ConditionWeaponDesiredSocket::InitFields", 0x0065ADC0, &WeaponDesiredSocket_InitFields);
WPNCOND_REVERSED("ConditionWeaponDesired::InitFields", 0x0065AE70, &Fields<ClearName(0x4), U8(0x8, 1)>);  // also ConditionLastWeapon
WPNCOND_REVERSED("ConditionPropTargetHeight::InitFields", 0x0065AEC0, &Fields<U8(0xC, 0), F32(0x8, 0.0f)>);
WPNCOND_REVERSED("ConditionTargetProjectile::InitFields", 0x0065AEF0, &Fields<F32(0x4, 1.5f), ClearName(0x8), F32(0x10, -1.0f), U8(0xC, 0)>);
WPNCOND_REVERSED("ConditionTargetProp::InitFields", 0x0065AF40, &TargetProp_InitFields);
WPNCOND_REVERSED("ConditionPropLifeTime::InitFields", 0x0065B0F0, &Fields<I32(0x8, 3), F32(0x4, 5000.0f)>);
WPNCOND_REVERSED("ConditionHasCash::InitFields", 0x0065B110, &Fields<F32(0x4, 0.25f)>);
WPNCOND_REVERSED("CatchProjectileCond::InitFields", 0x0065B130, &Fields<F32(0x10, 60.0f), U8(0x22, 0), I32(0x4, 0xC), F32(0x14, 30.0f), U8(0x20, 1), U8(0x21, 0), F32(0x18, 1.0f), F32(0x1C, 0.0f), F32(0x8, -0.100000001f), F32(0xC, -0.100000001f)>);
WPNCOND_REVERSED("AttackPowerCond::InitFields", 0x0065B180, &Fields<I32(0x8, 5), F32(0x4, 2.0f)>);

// destructors (thiscall)
WPNCOND_REVERSED("ConditionWeaponEquiped::Destruct", 0x0065AD10, &Fields<Vt(0x009365A4), KeyDtor(0x4)>);
WPNCOND_REVERSED("ConditionWeaponEquipedType::Destruct", 0x0065AD30, &Fields<Vt(0x009365B8)>);
WPNCOND_REVERSED("ConditionWeaponEquipedSocket::Destruct", 0x0065AD70, &Fields<Vt(0x009365CC), KeyDtor(0x8)>);
WPNCOND_REVERSED("ConditionPropAttached::Destruct", 0x0065AD80, &Fields<Vt(0x00936608), KeyDtor(0x4)>);
WPNCOND_REVERSED("ConditionWeaponDesiredSocket::Destruct", 0x0065AE00, &Fields<Vt(0x00936568), KeyDtor(0x8), KeyDtor(0x4)>);
WPNCOND_REVERSED("ConditionWeaponDesiredIsEquiped::Destruct", 0x0065AE20, &Fields<Vt(0x0093657C)>);
WPNCOND_REVERSED("ConditionWeaponDesiredIsFocusWeapon::Destruct", 0x0065AE30, &Fields<Vt(0x00936590)>);
WPNCOND_REVERSED("ConditionWeaponModelRequest::Destruct", 0x0065AEA0, &Fields<Vt(0x0093661C), KeyDtor(0x4)>);
WPNCOND_REVERSED("ConditionPropSpecialState::Destruct", 0x0065AEB0, &Fields<Vt(0x00936718)>);
WPNCOND_REVERSED("ConditionPropTargetHeight::Destruct", 0x0065AED0, &Fields<Vt(0x0093672C)>);
WPNCOND_REVERSED("ConditionPropTargetIsTwoHanded::Destruct", 0x0065AEE0, &Fields<Vt(0x00937EA8)>);
WPNCOND_REVERSED("ConditionTargetProjectile::Destruct", 0x0065AF30, &Fields<Vt(0x00936740), KeyDtor(0x8)>);
WPNCOND_REVERSED("ConditionTargetProp::Destruct", 0x0065AFA0, &Fields<Vt(0x00936690), KeyDtor(0x1C), KeyDtor(0x14), KeyDtor(0x10)>);
WPNCOND_REVERSED("ConditionCameraDesired::Destruct", 0x0065AFD0, &Fields<Vt(0x00936754)>);
WPNCOND_REVERSED("CatchProjectileCond::Destruct", 0x0065B170, &Fields<Vt(0x00936810)>);
WPNCOND_REVERSED("AttackPowerCond::Destruct", 0x0065B1A0, &Fields<Vt(0x00936824)>);

// Evaluate (vf1, thiscall, the owner)
WPNCOND_REVERSED("ConditionButtonState::Evaluate", 0x0065A970, &ButtonState_Evaluate);
WPNCOND_REVERSED("ConditionCollisionWeaponType::Evaluate", 0x0065AB90, &CollisionWeaponType_Evaluate);
WPNCOND_REVERSED("ConditionAltWeaponCollisionType::Evaluate", 0x0065AC90, &AltWeaponCollisionType_Evaluate);
WPNCOND_REVERSED("ConditionDamageFromExplosion::Evaluate", 0x0065ACB0, &DamageFromExplosion_Evaluate);
WPNCOND_REVERSED("ConditionDamageFromBurningFire::Evaluate", 0x0065ACC0, &DamageFromBurningFire_Evaluate);
WPNCOND_REVERSED("ConditionCameraDesired::Evaluate", 0x0065AFE0, &CameraDesired_Evaluate);
WPNCOND_REVERSED("ConditionCameraActive::Evaluate", 0x0065B000, &CameraActive_Evaluate);
WPNCOND_REVERSED("ConditionWeaponHudLocked::Evaluate", 0x0065B020, &WeaponHudLocked_Evaluate);
WPNCOND_REVERSED("ConditionWeaponWheelOn::Evaluate", 0x0065B030, &WeaponWheelOn_Evaluate);
WPNCOND_REVERSED("ConditionHudComponentVisible::Evaluate", 0x0065B040, &HudComponentVisible_Evaluate);
WPNCOND_REVERSED("ConditionCameraLocked::Evaluate", 0x0065B060, &CameraLocked_Evaluate);
WPNCOND_REVERSED("ConditionCameraAllowScriptedChange::Evaluate", 0x0065B070, &CameraAllowScriptedChange_Evaluate);
WPNCOND_REVERSED("ConditionCameraDesiredReset::Evaluate", 0x0065B090, &CameraDesiredReset_Evaluate);
WPNCOND_REVERSED("ConditionCameraBlending::Evaluate", 0x0065B0B0, &CameraBlending_Evaluate);
WPNCOND_REVERSED("ConditionCameraFading::Evaluate", 0x0065B0D0, &CameraFading_Evaluate);
WPNCOND_REVERSED("ConditionCameraInWidescreen::Evaluate", 0x0065B120, &CameraInWidescreen_Evaluate);
WPNCOND_REVERSED("ConditionIsTakingPictureFreeze::Evaluate", 0x0065B1B0, &IsTakingPictureFreeze_Evaluate);
