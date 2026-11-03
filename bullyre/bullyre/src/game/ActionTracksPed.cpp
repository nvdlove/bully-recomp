// The action tree's tracks acting on the playing ped: target override / sync, anchors,
// grapple release, entity and ped flags, struggle buttons, punishment points, HUD
// flashing, vibration, attributes for cash, money, forces, damage (and its handling
// and death), special / sprint charge, combos, and detaching effects: their Start /
// Update / End virtuals. 0x60F490..0x610120.

#include <cmath>
#include <cstdint>

#include "game/ActionTracksCommon.h"

#include "game/Crt.h"
using namespace actc;

namespace {

// CMatrix (0x48 bytes)
struct Matrix {
    float m[16];
    uint32_t attached;  // +0x40
    uint32_t owns;      // +0x44
};
static_assert(sizeof(Matrix) == 0x48);

GAME_ARRAY(uint8_t*, hudComponents, 0x00C674A0);
GAME_VAR(uint8_t*, struggleButton, 0x00C674E4);
GAME_VAR(uint8_t*, hudMoney, 0x00C674D4);
GAME_VAR(uint8_t, hudFlashPaused_20C43FD, 0x020C43FD);
GAME_VAR(uint32_t, hudFlashTime_A14A50, 0x00A14A50);
GAME_VAR(float, sprintRate_B0B194, 0x00B0B194);
GAME_VAR(void*, effectLibrary, 0x00B6F1E8);
GAME_VAR(uint8_t*, speechManager_CF4274, 0x00CF4274);
inline void* const painSounds_CF4378 = reinterpret_cast<void*>(0x00CF4378);
inline void* const sounds_CF3DA0 = reinterpret_cast<void*>(0x00CF3DA0);

GAME_FN(void, __thiscall, Matrix_SetRotateZ, 0x00412CE0, Matrix* m, float angle);
GAME_FN(uint8_t*, __thiscall, Placeable_GetMatrix, 0x0046DD20, uint8_t* placeable);
GAME_FN(Matrix*, __cdecl, Matrix_Multiply, 0x004138D0, Matrix* out, const uint8_t* a, const Matrix* b);
GAME_FN(void, __thiscall, Placeable_SetMatrix, 0x0046DE90, uint8_t* placeable, const Matrix* m, int force);
GAME_FN(void, __thiscall, Entity_UpdateRwFrame, 0x00443AA0, uint8_t* entity);
GAME_FN(void, __cdecl, Node_Update, 0x006C9AF0, void* node);
GAME_FN(float, __cdecl, LimitAngle, 0x00432EE0, float angle);
GAME_FN(void, __thiscall, Ped_SetHeading, 0x004734C0, uint8_t* ped, float heading);
GAME_FN(void, __thiscall, Ped_UpdateMoveDirection, 0x00474570, uint8_t* ped);
GAME_FN(void, __thiscall, Ped_SetWorldAnchor, 0x0049A810, uint8_t* ped);
GAME_FN(uint32_t, __thiscall, Ped_TryGrabVehicle, 0x00479B50, uint8_t* ped);
GAME_FN(void, __thiscall, Ped_ClearGrapple, 0x00477470, uint8_t* ped);
GAME_FN(uint8_t, __cdecl, Dodgeball_IsActive, 0x00683A30);
inline char* __cdecl Strncpy(char* dst, const char* src, size_t n) { return crt::Strncpy(dst, src, n); }
GAME_FN(void, __thiscall, Ped_SetPunishmentPoints, 0x00477520, uint8_t* ped, int32_t points);
GAME_FN(void, __thiscall, HudMoney_Flash, 0x00542810, uint8_t* money);
GAME_FN(void, __cdecl, Pad_Rumble, 0x00738680, int pad, uint32_t duration, int strength, int flag);
GAME_FN(void, __cdecl, Pad_StopVibration, 0x00738740, int pad);
GAME_FN(uint8_t, __thiscall, Ped_HasMoney, 0x004730E0, uint8_t* ped, int32_t amount);
GAME_FN(void, __thiscall, Ped_TakeMoney, 0x00473100, uint8_t* ped, int32_t amount);
GAME_FN(void, __thiscall, Ped_AddMoney, 0x00475320, uint8_t* ped, int32_t amount);
GAME_FN(void, __thiscall, Ped_AddSpecial, 0x0047CD90, uint8_t* ped, float amount, int);
GAME_FN(void, __thiscall, Ped_AddHealth, 0x00474CC0, uint8_t* ped, float health);
// (0x004146B0 stores 16 bytes through `out`: called through re::CallVec16)
static inline V3* Multiply3x3(V3* out, const uint8_t* m, const V3* v) { return re::CallVec16<0x004146B0, V3*>(out, m, v); }
GAME_FN(void, __thiscall, Physical_SetMoveSpeed, 0x0046A7E0, uint8_t* physical, float x, float y, float z);
GAME_FN(void, __thiscall, Physical_ApplyTurnForce, 0x0046A950, uint8_t* physical, float fx, float fy, float fz,
        float px, float py, float pz);
GAME_FN(void, __thiscall, Ped_TakeDamage, 0x0047B940, uint8_t* ped, float amount, uint8_t* attacker);
GAME_FN(void, __thiscall, PainSounds_Play, 0x005A59D0, void* sounds, uint8_t* ped, int, int);
GAME_FN(void, __thiscall, Speech_Stop, 0x005924E0, uint8_t* speech, uint8_t* ped, int);
GAME_FN(int32_t, __thiscall, Sounds_Play, 0x0059B8A0, void* sounds, int32_t sound, const float* pos, int, uintptr_t, int,
        int, int, int, uintptr_t, uintptr_t, int, int, float, int);
GAME_FN(void, __thiscall, Object_TakeDamage, 0x004ED0B0, uint8_t* object, const float* pos, int32_t amount,
        uint8_t* attacker);
GAME_FN(void, __thiscall, Ped_SetSpecialRate, 0x00474340, uint8_t* ped, float rate);
GAME_FN(void, __thiscall, Ped_SetSprintRate, 0x00474430, uint8_t* ped, float rate);
GAME_FN(void, __thiscall, Ped_AddSprint, 0x0047D030, uint8_t* ped, float amount);
GAME_FN(void, __thiscall, Ped_AddSpecialPoints, 0x004743D0, uint8_t* ped, int32_t points);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(uint8_t*, __thiscall, Skeleton_FindBone, 0x00465580, void* skeleton, uint32_t hash, int32_t* bone);
GAME_FN(uint16_t, __thiscall, EffectLibrary_Find, 0x00667B60, void* library, const char* name);
GAME_FN(uint8_t*, __thiscall, Keeper_FindEffect, 0x004688E0, uint8_t* keeper, int effect, uint8_t* bone);
GAME_FN(void, __thiscall, Effect_Stop, 0x00668E40, uint8_t* proxy);
GAME_FN(void, __thiscall, AE_Proxy_Kill, 0x00668BA0, uint8_t* proxy);
GAME_FN(void, __thiscall, AE_Proxy_SlowKill, 0x00668D00, uint8_t* proxy, float time);
GAME_FN(void, __thiscall, AE_Proxy_Fade_x64, 0x00668C40, uint8_t* proxy, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void AE_Proxy_Fade(uint8_t* a0, float a1) {
    return AE_Proxy_Fade_x64(a0, re::BitsOf(a1));
}

uint8_t* TrackOwnerPed(uint8_t* self, int at) {
    return OwnerPed(At<uint8_t*>(self, at));
}

// 0x60F490 (thiscall): TargetOverrideResetTrack::Start: the targeting's override off
uint8_t __fastcall TargetOverrideResetTrack_Start(uint8_t*, re::Edx, uint8_t* owner) {
    At<uint8_t*>(OwnerPed(owner), 0x1C8)[0x84] = 0;
    return 1;
}

// 0x60F4B0 (thiscall): TargetSyncTrack::Start: the grappled ped (+0x155C) put at +0x18..
// turned +0x24 in the ped's space, and its heading set from the ped's
uint8_t __fastcall TargetSyncTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x34) = owner;
    uint8_t* ped = OwnerPed(owner);
    uint8_t* target = At<uint8_t*>(ped, 0x155C);
    if (!target) {
        return 1;
    }
    float x = At<float>(self, 0x18);
    float y = At<float>(self, 0x1C);
    float z = At<float>(self, 0x20);
    float turn = At<float>(self, 0x24);
    Matrix offset;
    offset.attached = 0;
    Matrix_SetRotateZ(&offset, turn);
    offset.m[12] = offset.m[12] + x;
    offset.m[13] = offset.m[13] + y;
    offset.m[14] = offset.m[14] + z;
    // (the game's result overlaps the offset's last dword, which it never reads)
    Matrix world;
    Placeable_SetMatrix(target, Matrix_Multiply(&world, Placeable_GetMatrix(ped), &offset), 0);
    Entity_UpdateRwFrame(target);
    Node_Update(At<void*>(At<uint8_t*>(target, 0x114), 0xC));
    float heading = At<float>(ped, 0x1700) + turn;
    Ped_SetHeading(target, LimitAngle(heading));
    Ped_UpdateMoveDirection(target);
    return 1;
}

// 0x60F5B0 (thiscall): SetWorldAnchorTrack::Start (the player)
uint8_t __fastcall SetWorldAnchorTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    if (At<int32_t>(OwnerPed(owner), 0x1310) == 13) {
        Ped_SetWorldAnchor(OwnerPed(owner));
    }
    return 1;
}

// 0x60F5D0 (thiscall): AttemptAnchorToVehicleTrack::Start
uint8_t __fastcall AttemptAnchorToVehicleTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    Ped_TryGrabVehicle(OwnerPed(owner));
    return 1;
}

// 0x60F5F0 (thiscall): TargetGrappleReleaseTrack::Start: the grapple let go (+0x18: the
// grappled one's too)
uint8_t __fastcall TargetGrappleReleaseTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* target = At<uint8_t*>(OwnerPed(owner), 0x155C);
    if (target) {
        if (self[0x18] != 0 && At<uint8_t*>(target, 0x155C) == OwnerPed(owner)) {
            Ped_ClearGrapple(target);
        }
        Ped_ClearGrapple(OwnerPed(owner));
    }
    return 1;
}

// 0x60F630 / 0x60F650 / 0x60FA40 (thiscall): a flag (+0x1C which, +0x18 the value) of the
// ped's entity flags (+0x1C..), its vehicle's, or its ped flags (+0x1314..)
uint8_t __fastcall EntityFlagsTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    At<uint32_t>(OwnerPed(owner), 0x1C + At<int32_t>(self, 0x1C) * 4) = self[0x18];
    return 1;
}
uint8_t __fastcall SetVehicleEntityFlagsTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    uint8_t* vehicle = At<uint8_t*>(OwnerPed(owner), 0x1554);
    if (vehicle) {
        At<uint32_t>(vehicle, 0x1C + At<int32_t>(self, 0x1C) * 4) = self[0x18];
    }
    return 0;
}
uint8_t __fastcall PedFlagsTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    At<uint32_t>(OwnerPed(owner), 0x1314 + At<int32_t>(self, 0x1C) * 4) = self[0x18];
    return 1;
}

// 0x60F680 (thiscall): StruggleButtonOnTrack::End: the struggle button shown (for the
// player, or in dodgeball): its text +0x1C, button +0x20, +0x24 and flags +0x18..+0x1A
uint8_t __fastcall StruggleButtonOnTrack_End(uint8_t* self, re::Edx) {
    if (TrackOwnerPed(self, 0x28) != playerPed && !Dodgeball_IsActive()) {
        return 1;
    }
    vc::Call<void>(struggleButton, 0x14, 1);
    uint8_t* button = struggleButton;
    At<float>(button, 0x164) = 0.0f;
    At<int32_t>(button, 0x20) = At<int32_t>(self, 0x20);
    Strncpy(reinterpret_cast<char*>(button + 0x44), At<const char*>(self, 0x1C), 0xFF);
    At<int32_t>(button, 0x15C) = At<int32_t>(self, 0x24);
    button[0x161] = self[0x19];
    button[0x160] = self[0x18];
    button[0x162] = self[0x1A];
    return 1;
}

// 0x60F700 (thiscall): StruggleButtonOffTrack::End: the struggle button hidden
uint8_t __fastcall StruggleButtonOffTrack_End(uint8_t* self, re::Edx) {
    if (TrackOwnerPed(self, 0x1C) != playerPed && !Dodgeball_IsActive()) {
        return 1;
    }
    vc::Call<void>(struggleButton, 0x14, 0);
    return 1;
}

// 0x60F730 (thiscall): SetPunishmentPointsTrack::Start
uint8_t __fastcall SetPunishmentPointsTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    if (OwnerPed(owner)) {
        Ped_SetPunishmentPoints(OwnerPed(owner), At<int32_t>(self, 0x18));
    }
    return 1;
}

// a HUD component's flashing on (from now, or the paused time) or off
void SetFlashing(uint8_t* component, uint8_t on) {
    component[0x14] = on;
    if (on) {
        At<uint32_t>(component, 0x18) = hudFlashPaused_20C43FD ? hudFlashTime_A14A50 : timeInMs;
    } else {
        At<uint32_t>(component, 0x18) = 0;
    }
}

// 0x60F750 (thiscall): HUDComponentFlashingTrack::Start: component +0x18 flashes (+0x1C)
uint8_t __fastcall HUDComponentFlashingTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    SetFlashing(hudComponents[At<int32_t>(self, 0x18)], self[0x1C]);
    if (At<int32_t>(self, 0x18) == 13 && self[0x1C] != 0) {
        HudMoney_Flash(hudMoney);
    }
    return 1;
}

// 0x60F7B0 (thiscall): HUDComponentFlashingTrack::End: flipped back (+0x1D)
uint8_t __fastcall HUDComponentFlashingTrack_End(uint8_t* self, re::Edx) {
    if (self[0x1D] == 0) {
        return 1;
    }
    SetFlashing(hudComponents[At<int32_t>(self, 0x18)], self[0x1C] == 0 ? 1 : 0);
    if (At<int32_t>(self, 0x18) == 13 && self[0x1C] != 0) {
        hudMoney[0x30] = 0;
    }
    return 1;
}

// 0x60F810 (thiscall): HudFlashMoneyTrack::End: the money display's key cleared (the
// player's)
uint8_t __fastcall HudFlashMoneyTrack_End(uint8_t* self, re::Edx) {
    if (TrackOwnerPed(self, 0x18) == playerPed) {
        NameKey_Destruct(hudMoney);
    }
    return 1;
}

// 0x60F830 (thiscall): VibrationTrack::Update: the pad rumbles (+0x18: 0 soft, 1-2 hard,
// 3 off) at strength +0x20 for its time left (+0x28; +0x1C: the player's only)
uint8_t __fastcall VibrationTrack_Update(uint8_t* self, re::Edx, float dt) {
    if (!(dt < At<float>(self, 0x28)) && !std::isnan(dt) && !std::isnan(At<float>(self, 0x28))) {
        return 0;
    }
    At<float>(self, 0x28) = At<float>(self, 0x28) - dt;
    if (self[0x1C] != 0 && TrackOwnerPed(self, 0x24) != playerPed) {
        return 1;
    }
    float strength = float(At<int32_t>(self, 0x20));
    if (strength < 0.0f) {
        strength = 0.0f;
    } else if (strength > 255.0f) {
        strength = 255.0f;
    }
    int level = uint16_t(static_cast<int32_t>(strength));
    switch (At<int32_t>(self, 0x18)) {
    case 0:
        Pad_Rumble(0, 0xA0, level, 0);
        break;
    case 1:
    case 2:
        Pad_Rumble(0, 0xA0, level, 1);
        break;
    case 3:
        Pad_StopVibration(0);
        break;
    default:
        break;
    }
    return 1;
}

// 0x60F910 (thiscall): AttributesForCashTrack::Start: +0x18 dollars buy health (+0x20 0)
// or special (1) of +0x1C; a player who can't pay gets a prompt for 2 s
uint8_t __fastcall AttributesForCashTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    int32_t cost = static_cast<int32_t>(At<float>(self, 0x18) * 100.0);
    self[0x28] = 0;
    uint8_t* ped = OwnerPed(owner);
    if (!ped) {
        return 1;
    }
    if (!Ped_HasMoney(ped, cost)) {
        if (TrackOwnerPed(self, 0x24) == playerPed) {
            self[0x28] = 1;
            At<uint32_t>(self, 0x2C) = timeInMs + 2000;
        }
        return 1;
    }
    Ped_TakeMoney(TrackOwnerPed(self, 0x24), cost);
    if (At<int32_t>(self, 0x20) == 0) {
        Ped_AddHealth(TrackOwnerPed(self, 0x24), float(At<int32_t>(self, 0x1C)));
    } else if (At<int32_t>(self, 0x20) == 1) {
        Ped_AddSpecial(TrackOwnerPed(self, 0x24), float(At<int32_t>(self, 0x1C)), 0);
    }
    return 1;
}

// 0x60F9C0 (thiscall): AddMoneyTrack::Start: +0x18 dollars given (or taken)
uint8_t __fastcall AddMoneyTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    int32_t cents = static_cast<int32_t>(At<float>(self, 0x18) * 100.0);
    uint8_t* ped = OwnerPed(owner);
    if (ped) {
        if (cents < 0) {
            float amount = std::fabs(float(cents));
            Ped_TakeMoney(ped, int32_t(int64_t(amount)));
            return 1;
        }
        Ped_AddMoney(ped, cents);
    }
    return 1;
}

// 0x60FA60 (thiscall): AddForceTrack::Start: the move speed set to +0x18.. (in the ped's
// space unless +0x24); on its vehicle when it has one
uint8_t __fastcall AddForceTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    V3 force = {At<float>(self, 0x18), At<float>(self, 0x1C), At<float>(self, 0x20)};
    At<uint8_t*>(self, 0x28) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (self[0x24] == 0) {
        uint8_t* m = Placeable_GetMatrix(ped);
        V3 local = force;
        V3 out[2];   // (0x4146B0 writes a 4th float)
        force = *Multiply3x3(out, m, &local);
    }
    uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
    if (vehicle) {
        Physical_SetMoveSpeed(vehicle, force.x, force.y, force.z);
        V3 direction = force;
        Vector_Normalise(&direction);
        return 1;
    }
    Physical_SetMoveSpeed(ped, force.x, force.y, force.z);
    return 1;
}

// 0x60FB40 (thiscall): AddTurnForceTrack::Start: force +0x18.. at +0x24.. (the ped's
// space) on the ped or its vehicle
uint8_t __fastcall AddTurnForceTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    V3 force = {At<float>(self, 0x18), At<float>(self, 0x1C), At<float>(self, 0x20)};
    At<uint8_t*>(self, 0x30) = owner;
    uint8_t* ped = OwnerPed(owner);
    V3 point = {At<float>(self, 0x24), At<float>(self, 0x28), At<float>(self, 0x2C)};
    V3 local = force;
    V3 world[2][2];   // (0x4146B0 writes a 4th float)
    Multiply3x3(world[0], Placeable_GetMatrix(ped), &local);
    local = point;
    Multiply3x3(world[1], Placeable_GetMatrix(ped), &local);
    force = world[0][0];
    point = world[1][0];
    uint8_t* target = At<uint8_t*>(ped, 0x1554) ? static_cast<uint8_t*>(At<uint8_t*>(ped, 0x1554)) : ped;
    Physical_ApplyTurnForce(target, force.x, force.y, force.z, point.x, point.y, point.z);
    return 1;
}

// the damage's sphere (+0x20, 1-based) for the node's ped
void SetDamageSphere(uint8_t* self, uint8_t* target) {
    if (At<int32_t>(self, 0x20) > 0) {
        At<uint8_t*>(target, 0x2E0)[0x464] = uint8_t(At<int32_t>(self, 0x20) - 1);
    }
}

// 0x60FC30 (thiscall): DamageTrack::Update: damage +0x18 dealt over the track's time
// (+0x28 dealt, +0x2C time so far; at once with no end)
uint8_t __fastcall DamageTrack_Update(uint8_t* self, re::Edx, float dt) {
    uint8_t* ped = TrackOwnerPed(self, 0x24);
    uint8_t* target = vc::Call<uint8_t*>(self, acttree::kTrackResolve);
    if (!target) {
        return 0;
    }
    float end = acttree::TrackEnd(self);
    if (!(0.0f < end) && !(0.0f == end) && !std::isnan(end)) {
        At<float>(self, 0x28) = At<float>(self, 0x18);
        SetDamageSphere(self, target);
        Ped_TakeDamage(target, At<float>(self, 0x28), ped);
        return 0;
    }
    float length = end - acttree::TrackStart(self);
    float remaining = length - At<float>(self, 0x2C);
    if (remaining < 0.0f && 0.0f == At<float>(self, 0x2C)) {
        return 0;
    }
    float fraction = dt < remaining ? dt / remaining : 0.0f;
    float left = At<float>(self, 0x18) - At<float>(self, 0x28);
    float amount = left * fraction;
    SetDamageSphere(self, target);
    Ped_TakeDamage(target, amount, ped);
    At<float>(self, 0x28) = At<float>(self, 0x28) + amount;
    At<float>(self, 0x2C) = At<float>(self, 0x2C) + dt;
    return 1;
}

// 0x60FD40 (thiscall): DamageTrack::End: the rest of the damage dealt (unless the track
// ended early: its end past the controller's time)
uint8_t __fastcall DamageTrack_End(uint8_t* self, re::Edx) {
    uint8_t* ped = TrackOwnerPed(self, 0x24);
    uint8_t* target = vc::Call<uint8_t*>(self, acttree::kTrackResolve);
    if (!target) {
        return 0;
    }
    float end = acttree::TrackEnd(self);
    if (0.0f < end || 0.0f == end) {
        float now = At<float>(At<uint8_t*>(At<uint8_t*>(self, 0x24), 0), 8);
        if (end > now) {
            return 1;
        }
    }
    SetDamageSphere(self, target);
    float amount = At<float>(self, 0x18) - At<float>(self, 0x28);
    Ped_TakeDamage(target, amount, ped);
    At<float>(self, 0x28) = At<float>(self, 0x28) + amount;
    return 1;
}

// 0x60FDC0 (thiscall): TargetDamageTrack::Resolve: the grappled ped
uint8_t* __fastcall TargetDamageTrack_Resolve(uint8_t* self, re::Edx) {
    return At<uint8_t*>(TrackOwnerPed(self, 0x24), 0x155C);
}

// 0x60FDD0 (thiscall): HandleDamageTrack::Start: the hit (the owner's record +0x10)
// taken: pain sounds, speech stopped, (+0x1C) a hit sound half a unit up, the damage
// (scaled by +0x18) to the ped or the object (+0xC)
uint8_t __fastcall HandleDamageTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (ped) {
        if (At<int32_t>(ped, 0x1348) == 0) {
            PainSounds_Play(painSounds_CF4378, ped, 2, 0x19);
        }
        ped = TrackOwnerPed(self, 0x20);
        if (At<int32_t>(ped, 0x143C) != 0) {
            Speech_Stop(speechManager_CF4274, ped, 0);
        }
    }
    if (self[0x1C] != 0) {
        uint8_t* context = At<uint8_t*>(self, 0x20);
        uint8_t* entity = OwnerPed(context);
        if (!entity) {
            entity = At<uint8_t*>(context, 0xC);
            if (!entity) {
                return 0;
            }
        }
        float* p = Pos(entity);
        float pos[3] = {p[0], p[1], p[2]};
        if (At<uint8_t*>(context, 0xC) == nullptr) {
            pos[2] = pos[2] + 0.5f;
            Sounds_Play(sounds_CF3DA0, At<int32_t>(At<uint8_t*>(context, 0x10), 0x40), pos, 1, 0, 0, 0x78, 0, 0, 0, 0,
                        6, 1, 1.0f, 0);
        }
    }
    uint8_t* record = At<uint8_t*>(At<uint8_t*>(self, 0x20), 0x10);
    float amount = At<float>(record, 0x38) * At<float>(self, 0x18);
    uint8_t* target = TrackOwnerPed(self, 0x20);
    if (target) {
        Ped_TakeDamage(target, amount, At<uint8_t*>(record, 4));
    }
    uint8_t* object = At<uint8_t*>(At<uint8_t*>(self, 0x20), 0xC);
    if (object) {
        uint8_t* hit = At<uint8_t*>(At<uint8_t*>(self, 0x20), 0x10);
        Object_TakeDamage(object, reinterpret_cast<float*>(hit + 8), static_cast<int32_t>(amount), At<uint8_t*>(hit, 4));
    }
    return 0;
}

// 0x60FF20 (thiscall): DeathTrack::Start: the ped's death virtual (vf 0xA8, +0x18)
uint8_t __fastcall DeathTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    vc::Call<void>(OwnerPed(owner), 0xA8, int(self[0x18]), 0);
    return 1;
}

// 0x60FF50 / 0x60FF80 (thiscall): SpecialIncrementChargeTrack: the special charge rate
// set to +0x18 (the old one kept in +0x20 and put back)
uint8_t __fastcall SpecialIncrementChargeTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    At<float>(self, 0x20) = At<float>(OwnerPed(owner), 0x18C4);
    Ped_SetSpecialRate(OwnerPed(owner), At<float>(self, 0x18));
    return 1;
}
uint8_t __fastcall SpecialIncrementChargeTrack_End(uint8_t* self, re::Edx) {
    Ped_SetSpecialRate(TrackOwnerPed(self, 0x1C), At<float>(self, 0x20));
    return 1;
}

// 0x60FFA0 (thiscall): SpecialIncreaseTrack::Start: +0x18 special added
uint8_t __fastcall SpecialIncreaseTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    Ped_AddSpecial(OwnerPed(owner), float(At<int32_t>(self, 0x18)), 0);
    return 1;
}

// 0x60FFC0 / 0x60FFF0 (thiscall): SprintIncrementChargeTrack: the sprint charge rate set
// to +0x18 (the old one kept in +0x20 and put back)
uint8_t __fastcall SprintIncrementChargeTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    At<float>(self, 0x20) = At<float>(OwnerPed(owner), 0x18D4);
    Ped_SetSprintRate(OwnerPed(owner), At<float>(self, 0x18));
    return 1;
}
uint8_t __fastcall SprintIncrementChargeTrack_End(uint8_t* self, re::Edx) {
    Ped_SetSprintRate(TrackOwnerPed(self, 0x1C), At<float>(self, 0x20));
    return 1;
}

// 0x610010 (thiscall): SprintIncreaseTrack::Start: +0x18 sprint added at once (unless
// +0x1C: over time)
uint8_t __fastcall SprintIncreaseTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    if (self[0x1C] == 0) {
        Ped_AddSprint(OwnerPed(owner), float(At<int32_t>(self, 0x18)));
    }
    return 1;
}

// 0x610040 (thiscall): SprintIncreaseTrack::Update: +0x18 a second (+0x1C)
uint8_t __fastcall SprintIncreaseTrack_Update(uint8_t* self, re::Edx, float) {
    if (self[0x1C] == 0) {
        return 0;
    }
    float rate = sprintRate_B0B194 * timeStep;
    float amount = float(double(rate) * double(At<int32_t>(self, 0x18)));
    Ped_AddSprint(TrackOwnerPed(self, 0x20), amount);
    return 1;
}

// 0x610090 (thiscall): SpecialPointsAddTrack::Start
uint8_t __fastcall SpecialPointsAddTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    Ped_AddSpecialPoints(OwnerPed(owner), At<int32_t>(self, 0x18));
    return 1;
}

// 0x6100B0 / 0x6100D0 / 0x6100F0 (thiscall): the combo counter (+0x18CC) added to, set,
// or turned into special
uint8_t __fastcall ComboCounterAddTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* ped = OwnerPed(owner);
    At<float>(ped, 0x18CC) = At<float>(self, 0x18) + At<float>(ped, 0x18CC);
    return 1;
}
uint8_t __fastcall ComboCounterSetTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    At<float>(OwnerPed(owner), 0x18CC) = At<float>(self, 0x18);
    return 1;
}
uint8_t __fastcall TransferComboToSpecialTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    uint8_t* ped = OwnerPed(owner);
    Ped_AddSpecial(ped, At<float>(ped, 0x18CC), 0);
    At<float>(ped, 0x18CC) = 0.0f;
    return 1;
}

// 0x610120 (thiscall): DetachEffectTrack::Start: the effect +0x18 at bone +0x1C stopped
// (+0x20: faded out over that long, +0x24 at once to nothing; 0: killed)
uint8_t __fastcall DetachEffectTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!ped) {
        return 0;
    }
    const char* boneName = At<const char*>(self, 0x1C);
    const char* effectName = At<const char*>(self, 0x18);
    if (!boneName || !*boneName) {
        return 0;
    }
    void* skeleton = At<void*>(At<uint8_t*>(ped, 0x114), 0x94);
    int32_t index;
    uint8_t* bone = Skeleton_FindBone(skeleton, HashName(boneName), &index);
    if (!bone) {
        return 0;
    }
    int effect = EffectLibrary_Find(effectLibrary, effectName);
    uint8_t* proxy = Keeper_FindEffect(At<uint8_t*>(ped, 0x2E4), effect, bone);
    if (!proxy) {
        return 0;
    }
    Effect_Stop(proxy);
    float fade = At<float>(self, 0x20);
    if (fade == 0.0f) {
        AE_Proxy_Kill(proxy);
        return 0;
    }
    AE_Proxy_SlowKill(proxy, fade);
    if (self[0x24] != 0) {
        AE_Proxy_Fade(proxy, 0.0f);
    }
    return 0;
}

} // namespace

#define ACTTRACKSPED_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

ACTTRACKSPED_REVERSED("TargetOverrideResetTrack::Start", 0x0060F490, &TargetOverrideResetTrack_Start);
ACTTRACKSPED_REVERSED("TargetSyncTrack::Start", 0x0060F4B0, &TargetSyncTrack_Start);
ACTTRACKSPED_REVERSED("SetWorldAnchorTrack::Start", 0x0060F5B0, &SetWorldAnchorTrack_Start);
ACTTRACKSPED_REVERSED("AttemptAnchorToVehicleTrack::Start", 0x0060F5D0, &AttemptAnchorToVehicleTrack_Start);
ACTTRACKSPED_REVERSED("TargetGrappleReleaseTrack::Start", 0x0060F5F0, &TargetGrappleReleaseTrack_Start);
ACTTRACKSPED_REVERSED("EntityFlagsTrack::Start", 0x0060F630, &EntityFlagsTrack_Start);
ACTTRACKSPED_REVERSED("SetVehicleEntityFlagsTrack::Start", 0x0060F650, &SetVehicleEntityFlagsTrack_Start);
ACTTRACKSPED_REVERSED("StruggleButtonOnTrack::End", 0x0060F680, &StruggleButtonOnTrack_End);
ACTTRACKSPED_REVERSED("StruggleButtonOffTrack::End", 0x0060F700, &StruggleButtonOffTrack_End);
ACTTRACKSPED_REVERSED("SetPunishmentPointsTrack::Start", 0x0060F730, &SetPunishmentPointsTrack_Start);
ACTTRACKSPED_REVERSED("HUDComponentFlashingTrack::Start", 0x0060F750, &HUDComponentFlashingTrack_Start);
ACTTRACKSPED_REVERSED("HUDComponentFlashingTrack::End", 0x0060F7B0, &HUDComponentFlashingTrack_End);
ACTTRACKSPED_REVERSED("HudFlashMoneyTrack::End", 0x0060F810, &HudFlashMoneyTrack_End);
ACTTRACKSPED_REVERSED("VibrationTrack::Update", 0x0060F830, &VibrationTrack_Update);
ACTTRACKSPED_REVERSED("AttributesForCashTrack::Start", 0x0060F910, &AttributesForCashTrack_Start);
ACTTRACKSPED_REVERSED("AddMoneyTrack::Start", 0x0060F9C0, &AddMoneyTrack_Start);
ACTTRACKSPED_REVERSED("PedFlagsTrack::Start", 0x0060FA40, &PedFlagsTrack_Start);
ACTTRACKSPED_REVERSED("AddForceTrack::Start", 0x0060FA60, &AddForceTrack_Start);
ACTTRACKSPED_REVERSED("AddTurnForceTrack::Start", 0x0060FB40, &AddTurnForceTrack_Start);
ACTTRACKSPED_REVERSED("DamageTrack::Update", 0x0060FC30, &DamageTrack_Update);
ACTTRACKSPED_REVERSED("DamageTrack::End", 0x0060FD40, &DamageTrack_End);
ACTTRACKSPED_REVERSED("TargetDamageTrack::Resolve", 0x0060FDC0, &TargetDamageTrack_Resolve);
ACTTRACKSPED_REVERSED("HandleDamageTrack::Start", 0x0060FDD0, &HandleDamageTrack_Start);
ACTTRACKSPED_REVERSED("DeathTrack::Start", 0x0060FF20, &DeathTrack_Start);
ACTTRACKSPED_REVERSED("SpecialIncrementChargeTrack::Start", 0x0060FF50, &SpecialIncrementChargeTrack_Start);
ACTTRACKSPED_REVERSED("SpecialIncrementChargeTrack::End", 0x0060FF80, &SpecialIncrementChargeTrack_End);
ACTTRACKSPED_REVERSED("SpecialIncreaseTrack::Start", 0x0060FFA0, &SpecialIncreaseTrack_Start);
ACTTRACKSPED_REVERSED("SprintIncrementChargeTrack::Start", 0x0060FFC0, &SprintIncrementChargeTrack_Start);
ACTTRACKSPED_REVERSED("SprintIncrementChargeTrack::End", 0x0060FFF0, &SprintIncrementChargeTrack_End);
ACTTRACKSPED_REVERSED("SprintIncreaseTrack::Start", 0x00610010, &SprintIncreaseTrack_Start);
ACTTRACKSPED_REVERSED("SprintIncreaseTrack::Update", 0x00610040, &SprintIncreaseTrack_Update);
ACTTRACKSPED_REVERSED("SpecialPointsAddTrack::Start", 0x00610090, &SpecialPointsAddTrack_Start);
ACTTRACKSPED_REVERSED("ComboCounterAddTrack::Start", 0x006100B0, &ComboCounterAddTrack_Start);
ACTTRACKSPED_REVERSED("ComboCounterSetTrack::Start", 0x006100D0, &ComboCounterSetTrack_Start);
ACTTRACKSPED_REVERSED("TransferComboToSpecialTrack::Start", 0x006100F0, &TransferComboToSpecialTrack_Start);
ACTTRACKSPED_REVERSED("DetachEffectTrack::Start", 0x00610120, &DetachEffectTrack_Start);
