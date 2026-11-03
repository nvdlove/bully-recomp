// The action tree's simpler tracks (locomotion jump / cycle / liquid / retard / event,
// loco sync, gravity, orient offset, target face / override, sync base, attach and IK,
// ped reset, anchors, grapple release, entity flags, struggle buttons, punishment
// points, the HUD's fade / components / blips / money / tag shape / weapon wheel,
// vibration, radar blips, cash, hold state, ped flags, forces, damage): their field
// initialisers, destructors, resets, and the small Start / Update / End virtuals.
// 0x6086F0..0x609670.

#include <cstdint>

#include "game/ActionTracksCommon.h"

using namespace actc;

namespace {

constexpr uint32_t kVtLocomotionSlideTrack = 0x0090CE38;
constexpr uint32_t kVtLocomotionAnimationNewTrack = 0x00930EDC;
constexpr uint32_t kVtLocomotionJumpTrack = 0x00931044;
constexpr uint32_t kVtLocomotionCycleTrack = 0x00931060;
constexpr uint32_t kVtLocomotionLiquidTrack = 0x0093107C;
constexpr uint32_t kVtLocomotionRetardTrack = 0x00931098;
constexpr uint32_t kVtAnimationLocoSyncTrack = 0x009310B4;
constexpr uint32_t kVtLocomotionEventTrack = 0x009310CC;
constexpr uint32_t kVtGravityTrack = 0x009310E4;
constexpr uint32_t kVtOrientOffSetTrack = 0x009310FC;
constexpr uint32_t kVtTargetFaceTrack = 0x0093111C;
constexpr uint32_t kVtTargetOverrideResetTrack = 0x00931134;
constexpr uint32_t kVtSyncBaseTrack = 0x0093114C;
constexpr uint32_t kVtAttachTrack = 0x00931164;
constexpr uint32_t kVtAttachIKTrack = 0x0093117C;
constexpr uint32_t kVtDetachIKTrack = 0x00931194;
constexpr uint32_t kVtPedResetTrack = 0x009311AC;
constexpr uint32_t kVtSetWorldAnchorTrack = 0x009311C4;
constexpr uint32_t kVtTargetGrappleReleaseTrack = 0x009311DC;
constexpr uint32_t kVtEntityFlagsTrack = 0x009311F4;
constexpr uint32_t kVtStruggleButtonOnTrack = 0x0093120C;
constexpr uint32_t kVtStruggleButtonOffTrack = 0x00931224;
constexpr uint32_t kVtSetPunishmentPointsTrack = 0x0093123C;
constexpr uint32_t kVtHudScreenFadeTrack = 0x00931254;
constexpr uint32_t kVtHUDComponentVisiblityTrack = 0x0093126C;
constexpr uint32_t kVtHUDComponentFlashingTrack = 0x00931284;
constexpr uint32_t kVtHUDFlashBlipTrack = 0x0093129C;
constexpr uint32_t kVtHudFlashMoneyTrack = 0x009312B4;
constexpr uint32_t kVtHUDSetTagShapeTrack = 0x009312CC;
constexpr uint32_t kVtWeaponWheelOffTrack = 0x009312E4;
constexpr uint32_t kVtVibrationTrack = 0x009312FC;
constexpr uint32_t kVtAddRadarBlipTrack = 0x00931314;
constexpr uint32_t kVtAttributesForCashTrack = 0x0093132C;
constexpr uint32_t kVtAddMoneyTrack = 0x00931350;
constexpr uint32_t kVtHoldStateTrack = 0x00931368;
constexpr uint32_t kVtPedFlagsTrack = 0x00931380;
constexpr uint32_t kVtAddForceTrack = 0x00931398;
constexpr uint32_t kVtAddTurnForceTrack = 0x009313B0;
constexpr uint32_t kVtDamageTrack = 0x009313C8;
constexpr uint32_t kVtHandleDamageTrack = 0x009313E4;

GAME_VAR(uint8_t*, screenFade, 0x00C6754C);
GAME_ARRAY(uint8_t*, hudComponents, 0x00C674A0);
GAME_VAR(uint8_t*, hudRadar, 0x00C674CC);
GAME_VAR(uint8_t*, hudWeapons, 0x00C674F0);
GAME_VAR(uint8_t*, hudTagging, 0x00C67538);
constexpr uint32_t kNoneText = 0x0090A264;   // "NONE"
inline const char* const cashPromptKey = reinterpret_cast<const char*>(0x00931340);   // "CMN_STR_06"

GAME_FN(void, __thiscall, ScreenFade_Set, 0x00435A00, uint8_t* fade, int value, int flag);
GAME_FN(void, __thiscall, Radar_FlashBlip, 0x0054AC00, uint8_t* radar, int32_t blip);
GAME_FN(void, __thiscall, Tagging_StartTag, 0x00558750, uint8_t* tagging, int32_t shape);
GAME_FN(void, __thiscall, HudWeapons_SetOff, 0x0055CA60, uint8_t* weapons, int off);
GAME_FN(void, __cdecl, Pad_StopVibration, 0x00738740, int pad);
GAME_FN(void, __thiscall, Radar_RemoveBlip, 0x0054A940, uint8_t* radar, int32_t blip);
GAME_FN(void, __cdecl, ShowPrompt, 0x0068F580, const char* key, float scale);

// 0x6086F0 (thiscall): LocomotionJumpTrack's reset (vf5)
void __fastcall LocomotionJumpTrack_Reset(uint8_t* self, re::Edx) {
    self[0x2C] = 0;
    At<float>(self, 0x18) = 1.0f;
    At<float>(self, 0x1C) = 2.0f;
    At<float>(self, 0x20) = 0.0f;
    At<float>(self, 0x24) = 0.0f;
    At<float>(self, 0x28) = 0.0f;
}

// 0x608740 (thiscall): a locomotion track's fields
void __fastcall LocomotionTrack_InitFields_608740(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<float>(self, 0x18) = 8.0f;
    At<float>(self, 0x1C) = 20.0f;
}

// 0x6087A0 (thiscall): LocomotionCycleTrack::Start: the owner's cycle (+0x14) set to +0x18
uint8_t __fastcall LocomotionCycleTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    *At<float*>(owner, 0x14) = At<float>(self, 0x18);
    return 1;
}

// 0x6087C0 (thiscall): LocomotionLiquidTrack's reset (vf5)
void __fastcall LocomotionLiquidTrack_Reset(uint8_t* self, re::Edx) {
    self[8] = 0;
    At<float>(self, 0x18) = 1.4f;
    At<float>(self, 0x1C) = 1.0f;
    At<float>(self, 0x20) = 1.2f;
    At<float>(self, 0x24) = 0.0f;
}

// 0x608820 (thiscall): the owner kept (+0x28) (StruggleButtonOn's, AnimationDriverSteer's
// and LocomotionLiquid's Start)
uint8_t __fastcall KeepOwner28_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x28) = owner;
    return 1;
}

// 0x608830 (thiscall): LocomotionRetardTrack's reset (vf5)
void __fastcall LocomotionRetardTrack_Reset(uint8_t* self, re::Edx) {
    At<float>(self, 0x18) = 0.5f;
    self[8] = 0;
    self[0x1C] = 1;
    self[0x1D] = 0;
}

// 0x608880 (thiscall): a locomotion track's fields
void __fastcall LocomotionTrack_InitFields_608880(uint8_t* self, re::Edx) {
    SetNameRaw(self, 0x18, emptyString);
    At<float>(self, 0x1C) = 0.0f;
    self[8] = 1;
    At<float>(self, 0x20) = -1.0f;
    At<float>(self, 0x24) = -1.0f;
    At<float>(self, 0x28) = -1.0f;
    At<float>(self, 0x2C) = -1.0f;
}

// 0x6088C0 (thiscall): LocomotionAnimationNewTrack::Update: a pending cycle position
// (the locomotion's +0x14, from 0 up) handed to its animation (+0x18, +0x88)
uint8_t __fastcall LocomotionAnimationNewTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* loco = At<uint8_t*>(self, 0x30);
    uint8_t* anim = At<uint8_t*>(loco, 0x18);
    if (!anim) {
        return 1;
    }
    float position = *At<float*>(loco, 0x14);
    if (position < 0.0f || std::isnan(position)) {
        return 1;
    }
    At<float>(anim, 0x88) = position;
    *At<float*>(loco, 0x14) = -1.0f;
    return 1;
}

// 0x608910 (thiscall): LocomotionAnimationNewTrack's fields
void __fastcall LocomotionAnimationNewTrack_InitFields(uint8_t* self, re::Edx) {
    SetNameRaw(self, 0x18, emptyString);
    At<float>(self, 0x1C) = 0.0f;
    At<float>(self, 0x20) = -1.0f;
    self[8] = 1;
    At<float>(self, 0x24) = -1.0f;
    At<uint32_t>(self, 0x38) = 0;
    At<float>(self, 0x28) = -1.0f;
    At<float>(self, 0x2C) = -1.0f;
    ClearName(self, 0x34);
    At<float>(self, 0x3C) = -1.0f;
    self[8] = 1;
    At<float>(self, 0x40) = -1.0f;
}

// 0x6089D0 (thiscall): LocomotionEventTrack's fields
void __fastcall LocomotionEventTrack_InitFields(uint8_t* self, re::Edx) {
    ClearName(self, 0x18);
    At<float>(self, 0x1C) = 1.0f;
    At<int32_t>(self, 0x20) = 1;
    ClearName(self, 0x24);
    At<float>(self, 0x28) = 1.0f;
    At<int32_t>(self, 0x2C) = 1;
}

// 0x608A80 (thiscall): LocomotionEventTrack::Start
uint8_t __fastcall LocomotionEventTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x34) = owner;
    At<float>(self, 0x38) = 0.0f;
    return 1;
}

// 0x608AA0 / 0x608AE0 (thiscall): GravityTrack's / OrientOffSetTrack's fields
void __fastcall GravityTrack_InitFields(uint8_t* self, re::Edx) {
    self[0x18] = 0;
    At<float>(self, 0x1C) = 0.016f;
}
void __fastcall OrientOffSetTrack_InitFields(uint8_t* self, re::Edx) {
    self[8] = 0;
    At<float>(self, 0x18) = 3.14159274f;
}

// 0x608B20 (thiscall): TargetFaceTrack's fields (its start time 0, no end)
void __fastcall TargetFaceTrack_InitFields(uint8_t* self, re::Edx) {
    At<float>(self, 0xC) = 0.0f;
    self[8] = 0;
    At<float>(self, 0x10) = -1.0f;
    self[0x31] = 1;
    self[0x30] = 1;
    At<float>(self, 0x18) = 0.0f;
    self[0x32] = 0;
    self[0x33] = 0;
    At<float>(self, 0x24) = 0.033333335f;
    self[0x34] = 0;
    self[0x35] = 1;
    At<float>(self, 0x1C) = 0.16666667f;
    self[0x36] = 1;
    self[0x37] = 1;
    At<float>(self, 0x20) = 1.57079637f;
    At<float>(self, 0x2C) = -1.0f;
    At<float>(self, 0x28) = 360.0f;
}

// 0x608BB0 (thiscall): TargetOverrideResetTrack's fields (at once, no end)
void __fastcall TargetOverrideResetTrack_InitFields(uint8_t* self, re::Edx) {
    self[8] = 0;
    At<float>(self, 0xC) = 0.0f;
    At<float>(self, 0x10) = -1.0f;
}

// 0x608C30 / 0x608C70 (thiscall): the sync tracks' fields
void __fastcall SyncTrack_InitFields_608C30(uint8_t* self, re::Edx) {
    At<float>(self, 0x18) = 0.0f;
    At<uint32_t>(self, 0x28) = 0;
    At<float>(self, 0x1C) = 0.0f;
    self[8] = 0;
    At<float>(self, 0x20) = 0.0f;
    At<float>(self, 0x30) = 0.0f;
    At<float>(self, 0x24) = 3.14159274f;
}
void __fastcall SyncTrack_InitFields_608C70(uint8_t* self, re::Edx) {
    At<float>(self, 0x18) = 0.0f;
    At<uint32_t>(self, 0x28) = 0;
    At<float>(self, 0x1C) = 0.0f;
    self[8] = 0;
    At<float>(self, 0x20) = 0.0f;
    At<float>(self, 0x24) = 0.0f;
    At<float>(self, 0x30) = 0.0f;
}

// 0x608CC0 (thiscall): AttachIKTrack's fields
void __fastcall AttachIKTrack_InitFields(uint8_t* self, re::Edx) {
    ClearName(self, 0x1C);
    At<int32_t>(self, 0x18) = 1;
    self[8] = 0;
    ClearName(self, 0x24);
    self[8] = 0;
}

// 0x608D60 (thiscall): DetachIKTrack's fields
void __fastcall DetachIKTrack_InitFields(uint8_t* self, re::Edx) {
    ClearName(self, 0x18);
    self[8] = 0;
}

// 0x608E20 (thiscall): a track's fields
void __fastcall Track_InitFields_608E20(uint8_t* self, re::Edx) {
    self[8] = 0;
    self[0x18] = 1;
}

// 0x608E90 (thiscall): EntityFlagsTrack's fields
void __fastcall EntityFlagsTrack_InitFields(uint8_t* self, re::Edx) {
    At<int32_t>(self, 0x20) = 7;
    StringRef_Assign(&At<uint32_t>(self, 0x1C), kNoneText);
    At<uint32_t>(self, 0x24) = 0;
    self[0x19] = 1;
    self[0x18] = 1;
    self[0x1A] = 1;
}

// 0x608F20 (thiscall): StruggleButtonOffTrack::Update: runs while +0x18
uint8_t __fastcall StruggleButtonOffTrack_Update(uint8_t* self, re::Edx, float) {
    return self[0x18] != 0 ? 1 : 0;
}

// 0x608F60 (thiscall): HudScreenFadeTrack's fields
void __fastcall HudScreenFadeTrack_InitFields(uint8_t* self, re::Edx) {
    self[0x18] = 1;
    self[0x19] = 1;
    At<int32_t>(self, 0x1C) = 0xFF;
}

// 0x608FA0 (thiscall): HudScreenFadeTrack::Start: the screen faded to +0x1C
uint8_t __fastcall HudScreenFadeTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    ScreenFade_Set(screenFade, At<int32_t>(self, 0x1C), self[0x19]);
    return 1;
}

// 0x608FC0 (thiscall): HudScreenFadeTrack::End: faded back (+0x18)
uint8_t __fastcall HudScreenFadeTrack_End(uint8_t* self, re::Edx) {
    if (self[0x18] != 0) {
        ScreenFade_Set(screenFade, 0, self[0x19]);
    }
    return 1;
}

// 0x608FE0 / 0x609040 / 0x609080 (thiscall): the HUD tracks' fields
void __fastcall HudComponentTrack_InitFields_608FE0(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0x18) = 0;
    self[0x1C] = 1;
}
void __fastcall HudComponentTrack_InitFields_609040(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0x18) = 0;
    self[0x1C] = 1;
    self[0x1D] = 0;
}
void __fastcall HudTrack_InitFields_609080(uint8_t* self, re::Edx) {
    At<int32_t>(self, 0x18) = 8;
}

// 0x609020 (thiscall): HUDComponentVisiblityTrack::End: component +0x18 shown (+0x1C)
uint8_t __fastcall HUDComponentVisiblityTrack_End(uint8_t* self, re::Edx) {
    uint8_t* component = hudComponents[At<int32_t>(self, 0x18)];
    vc::Call<void>(component, 0x14, int(self[0x1C]));
    return 1;
}

// 0x6090C0 (thiscall): HUDFlashBlipTrack::End: the radar blip +0x18 flashed
uint8_t __fastcall HUDFlashBlipTrack_End(uint8_t* self, re::Edx) {
    Radar_FlashBlip(hudRadar, At<int32_t>(self, 0x18));
    return 1;
}

// 0x609140 (thiscall): HUDSetTagShapeTrack::Start: the tagging meter starts shape +0x18
uint8_t __fastcall HUDSetTagShapeTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    Tagging_StartTag(hudTagging, At<int32_t>(self, 0x18));
    return 1;
}

// 0x609190 (thiscall): WeaponWheelOffTrack::End
uint8_t __fastcall WeaponWheelOffTrack_End(uint8_t* self, re::Edx) {
    HudWeapons_SetOff(hudWeapons, self[0x18]);
    return 1;
}

// 0x6091B0 (thiscall): VibrationTrack's fields
void __fastcall VibrationTrack_InitFields(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0x18) = 0;
    At<int32_t>(self, 0x20) = 0x80;
    self[0x1C] = 1;
    self[8] = 0;
}

// 0x609200 (thiscall): VibrationTrack::Start: its length (end - start; 300 with no end)
uint8_t __fastcall VibrationTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    float end = At<float>(self, 0x10);
    if (0.0f < end || 0.0f == end) {
        At<float>(self, 0x28) = At<float>(self, 0x10) - At<float>(self, 0xC);
        return 1;
    }
    At<float>(self, 0x28) = 300.0f;
    return 1;
}

// 0x609230 (thiscall): VibrationTrack::End: the pad stops
uint8_t __fastcall VibrationTrack_End(uint8_t*, re::Edx) {
    Pad_StopVibration(0);
    return 1;
}

// 0x609240 (thiscall): AddRadarBlipTrack's fields
void __fastcall AddRadarBlipTrack_InitFields(uint8_t* self, re::Edx) {
    ClearName(self, 0x28);
    ClearName(self, 0x2C);
    self[0x18] = 0;
    At<int32_t>(self, 0x1C) = 2;
    At<int32_t>(self, 0x20) = 1;
    At<int32_t>(self, 0x24) = 4;
    StringRef_Assign(&At<uint32_t>(self, 0x28), kNoneText);
    StringRef_Assign(&At<uint32_t>(self, 0x2C), kNoneText);
    At<uint32_t>(self, 0x30) = 0;
}

// 0x609310 (thiscall): AddRadarBlipTrack::End: its blip (+0x38) removed
uint8_t __fastcall AddRadarBlipTrack_End(uint8_t* self, re::Edx) {
    if (At<int32_t>(self, 0x38) != -1) {
        Radar_RemoveBlip(hudRadar, At<int32_t>(self, 0x38));
        At<int32_t>(self, 0x38) = -1;
    }
    return 1;
}

// 0x609340 (thiscall): AttributesForCashTrack's fields
void __fastcall AttributesForCashTrack_InitFields(uint8_t* self, re::Edx) {
    At<float>(self, 0x18) = 1.5f;
    self[8] = 0;
    At<int32_t>(self, 0x1C) = 100;
    At<uint32_t>(self, 0x20) = 0;
}

// 0x609390 (thiscall): AttributesForCashTrack::Update: its prompt until +0x2C (ms)
uint8_t __fastcall AttributesForCashTrack_Update(uint8_t* self, re::Edx, float) {
    if (self[0x28] == 0) {
        return 0;
    }
    ShowPrompt(cashPromptKey, 1.0f);
    if (timeInMs >= At<uint32_t>(self, 0x2C)) {
        self[0x28] = 0;
    }
    return 1;
}

// 0x609430 (thiscall): HoldStateTrack::Start: not held yet
uint8_t __fastcall HoldStateTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    return 0;
}

// 0x609470 / 0x6094C0 / 0x609510 (thiscall): PedFlagsTrack's / AddForceTrack's /
// AddTurnForceTrack's fields
void __fastcall PedFlagsTrack_InitFields(uint8_t* self, re::Edx) {
    At<float>(self, 0x18) = 0.0f;
    self[0x24] = 0;
    At<float>(self, 0x1C) = 0.0f;
    self[8] = 0;
    At<float>(self, 0x20) = 0.0f;
}
void __fastcall AddForceTrack_InitFields(uint8_t* self, re::Edx) {
    self[8] = 0;
    At<float>(self, 0x18) = 0.0f;
    At<float>(self, 0x1C) = 0.0f;
    At<float>(self, 0x20) = 0.0f;
    At<float>(self, 0x24) = 0.0f;
    At<float>(self, 0x28) = 0.0f;
    At<float>(self, 0x2C) = 0.0f;
}
void __fastcall AddTurnForceTrack_InitFields(uint8_t* self, re::Edx) {
    self[8] = 0;
    At<float>(self, 0x18) = 0.0f;
    At<int32_t>(self, 0x20) = -1;
    At<float>(self, 0x1C) = -1.0f;
}

// 0x609560 (thiscall): DamageTrack::Start: its node's (vf 0x14) controller's (vf 0x40)
// +0x10 +0x50 set to +0x1C when that's 0 or more
uint8_t __fastcall DamageTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<float>(self, 0x28) = 0.0f;
    At<uint8_t*>(self, 0x24) = owner;
    At<float>(self, 0x2C) = 0.0f;
    float amount = At<float>(self, 0x1C);
    if (0.0f < amount || 0.0f == amount) {
        if (vc::Call<uint8_t*>(self, acttree::kTrackResolve)) {
            uint8_t* node = vc::Call<uint8_t*>(self, acttree::kTrackResolve);
            float value = At<float>(self, 0x1C);
            uint8_t* target = vc::Call<uint8_t*>(node, 0x40);
            At<float>(At<uint8_t*>(target, 0x10), 0x50) = value;
        }
    }
    return 1;
}

// 0x6095B0 / 0x6095E0 / 0x609630 / 0x609640 / 0x609670 (thiscall): damage and effect
// tracks' fields
void __fastcall DamageTrack_InitFields(uint8_t* self, re::Edx) {
    At<int32_t>(self, 0x20) = -1;
    At<float>(self, 0x18) = 0.0f;
    self[8] = 0;
    At<float>(self, 0x1C) = -1.0f;
}
void __fastcall HandleDamageTrack_InitFields(uint8_t* self, re::Edx) {
    At<float>(self, 0x18) = 1.0f;
    self[0x1C] = 1;
    self[0x1D] = 1;
    self[8] = 0;
}
void __fastcall Track_InitFields_609630(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0x18) = 0;
    self[0x1C] = 0;
    self[8] = 0;
}
void __fastcall Track_InitFields_609640(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0x18) = 0;
    self[8] = 0;
}
void __fastcall EffectTrack_InitFields_609670(uint8_t* self, re::Edx) {
    self[8] = 0;
    ClearName(self, 0x18);
    ClearName(self, 0x1C);
    At<float>(self, 0x20) = 1.0f;
    self[0x24] = 1;
}

// 0x609650 (thiscall): the tracks' no-update (Update returns false: done at once)
uint8_t __fastcall Track_NoUpdate(uint8_t*, re::Edx, float) {
    return 0;
}

// 0x609660 (thiscall): LocomotionCycleTrack's reset (vf5)
void __fastcall LocomotionCycleTrack_Reset(uint8_t* self, re::Edx) {
    self[8] = 0;
    At<float>(self, 0x18) = 0.0f;
}

} // namespace

#define ACTTRACKSSIMPLE_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

ACTTRACKSSIMPLE_REVERSED("LocomotionJumpTrack::Reset", 0x006086F0, &LocomotionJumpTrack_Reset);
ACTTRACKSSIMPLE_REVERSED("LocomotionJumpTrack::DeletingDestruct", 0x00608710, &TrackDeletingDtor<kVtLocomotionJumpTrack>);
ACTTRACKSSIMPLE_REVERSED("LocomotionTrack::InitFields_608740", 0x00608740, &LocomotionTrack_InitFields_608740);
ACTTRACKSSIMPLE_REVERSED("LocomotionSlideTrack::Destruct", 0x00608760, &TrackDtor<kVtLocomotionSlideTrack>);
ACTTRACKSSIMPLE_REVERSED("LocomotionCycleTrack::DeletingDestruct", 0x00608770, &TrackDeletingDtor<kVtLocomotionCycleTrack>);
ACTTRACKSSIMPLE_REVERSED("LocomotionCycleTrack::Start", 0x006087A0, &LocomotionCycleTrack_Start);
ACTTRACKSSIMPLE_REVERSED("LocomotionLiquidTrack::Reset", 0x006087C0, &LocomotionLiquidTrack_Reset);
ACTTRACKSSIMPLE_REVERSED("LocomotionLiquidTrack::DeletingDestruct", 0x006087F0, &TrackDeletingDtor<kVtLocomotionLiquidTrack>);
ACTTRACKSSIMPLE_REVERSED("Track::KeepOwner28_Start", 0x00608820, &KeepOwner28_Start);
ACTTRACKSSIMPLE_REVERSED("LocomotionRetardTrack::Reset", 0x00608830, &LocomotionRetardTrack_Reset);
ACTTRACKSSIMPLE_REVERSED("LocomotionRetardTrack::DeletingDestruct", 0x00608850, &TrackDeletingDtor<kVtLocomotionRetardTrack>);
ACTTRACKSSIMPLE_REVERSED("LocomotionTrack::InitFields_608880", 0x00608880, &LocomotionTrack_InitFields_608880);
ACTTRACKSSIMPLE_REVERSED("LocomotionAnimationNewTrack::Update", 0x006088C0, &LocomotionAnimationNewTrack_Update);
ACTTRACKSSIMPLE_REVERSED("LocomotionAnimationNewTrack::InitFields", 0x00608910, &LocomotionAnimationNewTrack_InitFields);
ACTTRACKSSIMPLE_REVERSED("AnimationLocoSyncTrack::DeletingDestruct", 0x00608990,
                         &TrackDeletingDtor<kVtAnimationLocoSyncTrack, 0x34, kVtLocomotionAnimationNewTrack, 0x18>);
ACTTRACKSSIMPLE_REVERSED("LocomotionEventTrack::InitFields", 0x006089D0, &LocomotionEventTrack_InitFields);
ACTTRACKSSIMPLE_REVERSED("LocomotionEventTrack::DeletingDestruct", 0x00608A40,
                         &TrackDeletingDtor<kVtLocomotionEventTrack, 0x24, 0x18>);
ACTTRACKSSIMPLE_REVERSED("LocomotionEventTrack::Start", 0x00608A80, &LocomotionEventTrack_Start);
ACTTRACKSSIMPLE_REVERSED("GravityTrack::InitFields", 0x00608AA0, &GravityTrack_InitFields);
ACTTRACKSSIMPLE_REVERSED("GravityTrack::DeletingDestruct", 0x00608AB0, &TrackDeletingDtor<kVtGravityTrack>);
ACTTRACKSSIMPLE_REVERSED("OrientOffSetTrack::InitFields", 0x00608AE0, &OrientOffSetTrack_InitFields);
ACTTRACKSSIMPLE_REVERSED("OrientOffSetTrack::DeletingDestruct", 0x00608AF0, &TrackDeletingDtor<kVtOrientOffSetTrack>);
ACTTRACKSSIMPLE_REVERSED("TargetFaceTrack::InitFields", 0x00608B20, &TargetFaceTrack_InitFields);
ACTTRACKSSIMPLE_REVERSED("TargetFaceTrack::DeletingDestruct", 0x00608B80, &TrackDeletingDtor<kVtTargetFaceTrack>);
ACTTRACKSSIMPLE_REVERSED("TargetOverrideResetTrack::InitFields", 0x00608BB0, &TargetOverrideResetTrack_InitFields);
ACTTRACKSSIMPLE_REVERSED("TargetOverrideResetTrack::DeletingDestruct", 0x00608BD0,
                         &TrackDeletingDtor<kVtTargetOverrideResetTrack>);
ACTTRACKSSIMPLE_REVERSED("SyncBaseTrack::DeletingDestruct", 0x00608C00, &TrackDeletingDtor<kVtSyncBaseTrack, 0x2C>);
ACTTRACKSSIMPLE_REVERSED("SyncTrack::InitFields_608C30", 0x00608C30, &SyncTrack_InitFields_608C30);
ACTTRACKSSIMPLE_REVERSED("SyncBaseTrack::Destruct", 0x00608C50, &TrackDtor<kVtSyncBaseTrack, 0x2C>);
ACTTRACKSSIMPLE_REVERSED("SyncTrack::InitFields_608C70", 0x00608C70, &SyncTrack_InitFields_608C70);
ACTTRACKSSIMPLE_REVERSED("AttachTrack::DeletingDestruct", 0x00608C90, &TrackDeletingDtor<kVtAttachTrack, 0x1C>);
ACTTRACKSSIMPLE_REVERSED("AttachIKTrack::InitFields", 0x00608CC0, &AttachIKTrack_InitFields);
ACTTRACKSSIMPLE_REVERSED("AttachIKTrack::DeletingDestruct", 0x00608D20,
                         &TrackDeletingDtor<kVtAttachIKTrack, 0x24, kVtAttachTrack, 0x1C>);
ACTTRACKSSIMPLE_REVERSED("DetachIKTrack::InitFields", 0x00608D60, &DetachIKTrack_InitFields);
ACTTRACKSSIMPLE_REVERSED("DetachIKTrack::DeletingDestruct", 0x00608D90, &TrackDeletingDtor<kVtDetachIKTrack, 0x18>);
ACTTRACKSSIMPLE_REVERSED("PedResetTrack::DeletingDestruct", 0x00608DC0, &TrackDeletingDtor<kVtPedResetTrack>);
ACTTRACKSSIMPLE_REVERSED("SetWorldAnchorTrack::DeletingDestruct", 0x00608DF0, &TrackDeletingDtor<kVtSetWorldAnchorTrack>);
ACTTRACKSSIMPLE_REVERSED("Track::InitFields_608E20", 0x00608E20, &Track_InitFields_608E20);
ACTTRACKSSIMPLE_REVERSED("TargetGrappleReleaseTrack::DeletingDestruct", 0x00608E30,
                         &TrackDeletingDtor<kVtTargetGrappleReleaseTrack>);
ACTTRACKSSIMPLE_REVERSED("EntityFlagsTrack::DeletingDestruct", 0x00608E60, &TrackDeletingDtor<kVtEntityFlagsTrack>);
ACTTRACKSSIMPLE_REVERSED("EntityFlagsTrack::InitFields", 0x00608E90, &EntityFlagsTrack_InitFields);
ACTTRACKSSIMPLE_REVERSED("StruggleButtonOnTrack::DeletingDestruct", 0x00608EC0,
                         &TrackDeletingDtor<kVtStruggleButtonOnTrack, 0x1C>);
ACTTRACKSSIMPLE_REVERSED("StruggleButtonOffTrack::DeletingDestruct", 0x00608EF0,
                         &TrackDeletingDtor<kVtStruggleButtonOffTrack>);
ACTTRACKSSIMPLE_REVERSED("StruggleButtonOffTrack::Update", 0x00608F20, &StruggleButtonOffTrack_Update);
ACTTRACKSSIMPLE_REVERSED("SetPunishmentPointsTrack::DeletingDestruct", 0x00608F30,
                         &TrackDeletingDtor<kVtSetPunishmentPointsTrack>);
ACTTRACKSSIMPLE_REVERSED("HudScreenFadeTrack::InitFields", 0x00608F60, &HudScreenFadeTrack_InitFields);
ACTTRACKSSIMPLE_REVERSED("HudScreenFadeTrack::DeletingDestruct", 0x00608F70, &TrackDeletingDtor<kVtHudScreenFadeTrack>);
ACTTRACKSSIMPLE_REVERSED("HudScreenFadeTrack::Start", 0x00608FA0, &HudScreenFadeTrack_Start);
ACTTRACKSSIMPLE_REVERSED("HudScreenFadeTrack::End", 0x00608FC0, &HudScreenFadeTrack_End);
ACTTRACKSSIMPLE_REVERSED("HudComponentTrack::InitFields_608FE0", 0x00608FE0, &HudComponentTrack_InitFields_608FE0);
ACTTRACKSSIMPLE_REVERSED("HUDComponentVisiblityTrack::DeletingDestruct", 0x00608FF0,
                         &TrackDeletingDtor<kVtHUDComponentVisiblityTrack>);
ACTTRACKSSIMPLE_REVERSED("HUDComponentVisiblityTrack::End", 0x00609020, &HUDComponentVisiblityTrack_End);
ACTTRACKSSIMPLE_REVERSED("HudComponentTrack::InitFields_609040", 0x00609040, &HudComponentTrack_InitFields_609040);
ACTTRACKSSIMPLE_REVERSED("HUDComponentFlashingTrack::DeletingDestruct", 0x00609050,
                         &TrackDeletingDtor<kVtHUDComponentFlashingTrack>);
ACTTRACKSSIMPLE_REVERSED("HudTrack::InitFields_609080", 0x00609080, &HudTrack_InitFields_609080);
ACTTRACKSSIMPLE_REVERSED("HUDFlashBlipTrack::DeletingDestruct", 0x00609090, &TrackDeletingDtor<kVtHUDFlashBlipTrack>);
ACTTRACKSSIMPLE_REVERSED("HUDFlashBlipTrack::End", 0x006090C0, &HUDFlashBlipTrack_End);
ACTTRACKSSIMPLE_REVERSED("HudFlashMoneyTrack::DeletingDestruct", 0x006090E0, &TrackDeletingDtor<kVtHudFlashMoneyTrack>);
ACTTRACKSSIMPLE_REVERSED("HUDSetTagShapeTrack::DeletingDestruct", 0x00609110, &TrackDeletingDtor<kVtHUDSetTagShapeTrack>);
ACTTRACKSSIMPLE_REVERSED("HUDSetTagShapeTrack::Start", 0x00609140, &HUDSetTagShapeTrack_Start);
ACTTRACKSSIMPLE_REVERSED("WeaponWheelOffTrack::DeletingDestruct", 0x00609160, &TrackDeletingDtor<kVtWeaponWheelOffTrack>);
ACTTRACKSSIMPLE_REVERSED("WeaponWheelOffTrack::End", 0x00609190, &WeaponWheelOffTrack_End);
ACTTRACKSSIMPLE_REVERSED("VibrationTrack::InitFields", 0x006091B0, &VibrationTrack_InitFields);
ACTTRACKSSIMPLE_REVERSED("VibrationTrack::DeletingDestruct", 0x006091D0, &TrackDeletingDtor<kVtVibrationTrack>);
ACTTRACKSSIMPLE_REVERSED("VibrationTrack::Start", 0x00609200, &VibrationTrack_Start);
ACTTRACKSSIMPLE_REVERSED("VibrationTrack::End", 0x00609230, &VibrationTrack_End);
ACTTRACKSSIMPLE_REVERSED("AddRadarBlipTrack::InitFields", 0x00609240, &AddRadarBlipTrack_InitFields);
ACTTRACKSSIMPLE_REVERSED("AddRadarBlipTrack::DeletingDestruct", 0x006092D0,
                         &TrackDeletingDtor<kVtAddRadarBlipTrack, 0x2C, 0x28>);
ACTTRACKSSIMPLE_REVERSED("AddRadarBlipTrack::End", 0x00609310, &AddRadarBlipTrack_End);
ACTTRACKSSIMPLE_REVERSED("AttributesForCashTrack::InitFields", 0x00609340, &AttributesForCashTrack_InitFields);
ACTTRACKSSIMPLE_REVERSED("AttributesForCashTrack::DeletingDestruct", 0x00609360,
                         &TrackDeletingDtor<kVtAttributesForCashTrack>);
ACTTRACKSSIMPLE_REVERSED("AttributesForCashTrack::Update", 0x00609390, &AttributesForCashTrack_Update);
ACTTRACKSSIMPLE_REVERSED("AddMoneyTrack::DeletingDestruct", 0x006093D0, &TrackDeletingDtor<kVtAddMoneyTrack>);
ACTTRACKSSIMPLE_REVERSED("HoldStateTrack::DeletingDestruct", 0x00609400, &TrackDeletingDtor<kVtHoldStateTrack>);
ACTTRACKSSIMPLE_REVERSED("HoldStateTrack::Start", 0x00609430, &HoldStateTrack_Start);
ACTTRACKSSIMPLE_REVERSED("PedFlagsTrack::DeletingDestruct", 0x00609440, &TrackDeletingDtor<kVtPedFlagsTrack>);
ACTTRACKSSIMPLE_REVERSED("PedFlagsTrack::InitFields", 0x00609470, &PedFlagsTrack_InitFields);
ACTTRACKSSIMPLE_REVERSED("AddForceTrack::DeletingDestruct", 0x00609490, &TrackDeletingDtor<kVtAddForceTrack>);
ACTTRACKSSIMPLE_REVERSED("AddForceTrack::InitFields", 0x006094C0, &AddForceTrack_InitFields);
ACTTRACKSSIMPLE_REVERSED("AddTurnForceTrack::DeletingDestruct", 0x006094E0, &TrackDeletingDtor<kVtAddTurnForceTrack>);
ACTTRACKSSIMPLE_REVERSED("AddTurnForceTrack::InitFields", 0x00609510, &AddTurnForceTrack_InitFields);
ACTTRACKSSIMPLE_REVERSED("DamageTrack::DeletingDestruct", 0x00609530, &TrackDeletingDtor<kVtDamageTrack>);
ACTTRACKSSIMPLE_REVERSED("DamageTrack::Start", 0x00609560, &DamageTrack_Start);
ACTTRACKSSIMPLE_REVERSED("DamageTrack::InitFields", 0x006095B0, &DamageTrack_InitFields);
ACTTRACKSSIMPLE_REVERSED("TargetDamageTrack::Destruct", 0x006095D0, &TrackDtor<kVtDamageTrack>);
ACTTRACKSSIMPLE_REVERSED("HandleDamageTrack::InitFields", 0x006095E0, &HandleDamageTrack_InitFields);
ACTTRACKSSIMPLE_REVERSED("HandleDamageTrack::DeletingDestruct", 0x00609600, &TrackDeletingDtor<kVtHandleDamageTrack>);
ACTTRACKSSIMPLE_REVERSED("Track::InitFields_609630", 0x00609630, &Track_InitFields_609630);
ACTTRACKSSIMPLE_REVERSED("Track::InitFields_609640", 0x00609640, &Track_InitFields_609640);
ACTTRACKSSIMPLE_REVERSED("Track::NoUpdate_609650", 0x00609650, &Track_NoUpdate);
ACTTRACKSSIMPLE_REVERSED("LocomotionCycleTrack::Reset", 0x00609660, &LocomotionCycleTrack_Reset);
ACTTRACKSSIMPLE_REVERSED("EffectTrack::InitFields_609670", 0x00609670, &EffectTrack_InitFields_609670);
