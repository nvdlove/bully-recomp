// The action tree's effect, camera, timer and game tracks: attached effects, breath,
// ceiling / decal / water ripple / impact effects, the dizzy screen effect, camera shots
// and shakes, camera freeze, the game timer, rail driver, shops, missions, gifts and
// items, stats, saving, completion displays, falling damage: their field initialisers,
// destructors and virtuals. 0x6096D0..0x60AD90.

#include <cmath>
#include <cstdint>

#include "game/ActionTracksCommon.h"

#include "game/Crt.h"
#include "runflags.h"
using namespace actc;

namespace {

constexpr uint32_t kVtDetachEffectTrack = 0x009313FC;
constexpr uint32_t kVtAddAttachedEffectTrack = 0x00931414;
constexpr uint32_t kVtBreathTrack = 0x0093142C;
constexpr uint32_t kVtFXCeilingTrack = 0x00931444;
constexpr uint32_t kVtFXDecalTrack = 0x0093145C;
constexpr uint32_t kVtFXWaterRipplesTrack = 0x00931474;
constexpr uint32_t kVtGroundImpactTrack = 0x0093148C;
constexpr uint32_t kVtWallImpactTrack = 0x009314A4;
constexpr uint32_t kVtFXWeaponImpactTrack = 0x009314BC;
constexpr uint32_t kVtScreenFxDizzyTrack = 0x009314D4;
constexpr uint32_t kVtCameraShotTrack = 0x009314EC;
constexpr uint32_t kVtCameraShakeTrack = 0x00931504;
constexpr uint32_t kVtCameraFreezeTrack = 0x0093151C;
constexpr uint32_t kVtGameTimerTrack = 0x00931534;
constexpr uint32_t kVtRailDriverTrack = 0x0093154C;
constexpr uint32_t kVtSetPosRotTrack = 0x00931564;
constexpr uint32_t kVtCreateGiftRewardPickupTrack = 0x0093157C;
constexpr uint32_t kVtAcceptGiftTrack = 0x00931594;
constexpr uint32_t kVtRemoveItemTrack = 0x009315AC;
constexpr uint32_t kVtSetCanAcceptGiftTrack = 0x009315C4;
constexpr uint32_t kVtSetOverheadIconTrack = 0x009315DC;
constexpr uint32_t kVtFallingDamageTrack = 0x009315F4;
constexpr uint32_t kVtFallingDamageHandleTrack = 0x00931610;
constexpr uint32_t kVtAttackTimerResetTrack = 0x00931628;

GAME_VAR(void*, effectLibrary, 0x00B6F1E8);
GAME_VAR(int32_t, dizzyOn, 0x00CF107C);
GAME_VAR(float, dizzyAmount, 0x00CF1080);
GAME_ARRAY(uint8_t, dizzyColour, 0x00AC762C);        // r, g, b, a
GAME_VAR(uint8_t*, screenEffects_CF4264, 0x00CF4264);
inline void* const shop = reinterpret_cast<void*>(0x00C18748);
inline void* const camera = reinterpret_cast<void*>(0x00C3CC68);      // TheCamera
inline void* const cameraShake = reinterpret_cast<void*>(0x00C3CE20);
inline void* const missionManager = reinterpret_cast<void*>(0x020C3CA0);
GAME_VAR(int32_t, weaponModelFirst, 0x00A136B0);
GAME_VAR(int32_t, weaponModelLast, 0x00A136B4);
GAME_VAR(int32_t, itemModelFirst, 0x00A136B8);
GAME_VAR(int32_t, itemModelLast, 0x00A136BC);
GAME_VAR(int32_t, saveGameArea_20CECE4, 0x020CECE4);
GAME_VAR(uint8_t, uberCompletion_20C87A8, 0x020C87A8);

GAME_FN(uint16_t, __thiscall, EffectLibrary_Find, 0x00667B60, void* library, const char* name);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(uint8_t*, __thiscall, Skeleton_FindBone, 0x00465580, void* skeleton, uint32_t hash, int32_t* bone);
GAME_FN(void, __thiscall, Keeper_Start, 0x00468770, uint8_t* keeper, int effect, int32_t bone);
GAME_FN(void, __thiscall, Keeper_StartAt, 0x00468AB0, uint8_t* keeper, int effect, const V3* pos);
GAME_FN(void, __thiscall, Effect_Stop, 0x00668E40, uint8_t* proxy);
GAME_FN(void, __thiscall, EffectProxy_Destruct, 0x00668DC0, uint8_t* proxy);
GAME_FN(void, __thiscall, AE_Proxy_Destruct, 0x00668EB0, uint8_t* proxy);
GAME_FN(void, __thiscall, AE_Proxy_Kill, 0x00668BA0, uint8_t* proxy);
GAME_FN(void, __thiscall, AE_Proxy_SlowKill, 0x00668D00, uint8_t* proxy, float time);
GAME_FN(void, __thiscall, AE_Proxy_Fade_x64, 0x00668C40, uint8_t* proxy, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void AE_Proxy_Fade(uint8_t* a0, float a1) {
    return AE_Proxy_Fade_x64(a0, re::BitsOf(a1));
}
GAME_FN(void, __thiscall, ScreenEffects_Update, 0x00593780, uint8_t* effects);
GAME_FN(void, __thiscall, Shop_Purchase, 0x00455D30, void* shop);
GAME_FN(void, __thiscall, Camera_SetShotName, 0x004F3490, void* camera, uint32_t name);
GAME_FN(void, __thiscall, Camera_ResetController, 0x004F2B60, void* camera);
GAME_FN(void, __thiscall, CameraShake_Enable, 0x004F3DA0, void* shake, int on);
GAME_FN(void, __thiscall, CameraShake_SetOffset, 0x004F3D10, void* shake, float x, float y, float z);
GAME_FN(void, __thiscall, CameraShake_SetRotation, 0x004F3D30, void* shake, float x, float y, float z);
GAME_FN(void, __thiscall, CameraShake_AddOffset, 0x004F3E20, void* shake, float x, float y, float z);
GAME_FN(void, __thiscall, CameraShake_AddRotation, 0x004F3E50, void* shake, float x, float y, float z);
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(void, __cdecl, BigMessage_Clear, 0x005142C0);
GAME_FN(void, __cdecl, BigMessage_AddNumber, 0x00514240, float value);
GAME_FN(void, __cdecl, BigMessage_Show, 0x005142A0, uintptr_t key, float time);
GAME_FN(uint8_t*, __thiscall, Missions_GetController0, 0x006AA680, void* manager);
GAME_FN(uint8_t*, __thiscall, Missions_GetController1, 0x006AA690, void* manager);
GAME_FN(void, __thiscall, MissionController_Fn6ACA70, 0x006ACA70, uint8_t* controller);
GAME_FN(uint8_t, __thiscall, MissionController_End, 0x006AA300, uint8_t* controller, int, int, int, int, int, int, int);
GAME_FN(int, __cdecl, GetModelIndexByHash, 0x0051AE20, uint32_t hash, int first, int last);
GAME_FN(void, __thiscall, Inventory_Give, 0x0045D030, uint8_t* inventory, int32_t weapon, int32_t ammo, int, int);
GAME_FN(void, __cdecl, Items_Give, 0x00541C30, int item, int n, int, int);
GAME_FN(uint8_t*, __cdecl, UsePoint_GetEntity, 0x004EABB0, uint8_t* owner);
GAME_FN(void, __cdecl, SaveGame_Request, 0x006A41F0);
GAME_FN(void, __cdecl, ShowCompletion, 0x007074E0, uintptr_t name, int, float, uintptr_t, uintptr_t, int);

// 0x609710 / 0x609890 (thiscall): AddAttachedEffectTrack's fields (and the dialog one's)
void __fastcall AddAttachedEffectTrack_InitFields(uint8_t* self, re::Edx) {
    self[8] = 0;
    ClearName(self, 0x18);
    ClearName(self, 0x1C);
    At<int32_t>(self, 0x20) = -1;
}
void __fastcall DialogAddAttachedEffectTrack_InitFields(uint8_t* self, re::Edx) {
    reinterpret_cast<void(__thiscall*)(uint8_t*)>(0x00609710)(self);
    At<uint32_t>(self, 0x28) = 0;
}

// 0x6097B0 (thiscall): the effect +0x18 attached to a ped standing on surface +0x20 (0
// or less: any): at bone +0x1C, else where the owner's hit record (+0x24 +0x10) says
uint8_t __fastcall AddAttachedEffectTrack_AttachTo(uint8_t* self, re::Edx, uint8_t* ped) {
    int32_t surface = At<int32_t>(self, 0x20);
    const char* boneName = At<const char*>(self, 0x1C);
    if (!ped) {
        return 0;
    }
    if (surface > 0 && int32_t(ped[0x1BE]) != surface) {
        return 0;
    }
    int effect = EffectLibrary_Find(effectLibrary, At<const char*>(self, 0x18));
    int32_t bone = 0;
    if (boneName && *boneName) {
        void* skeleton = At<void*>(At<uint8_t*>(ped, 0x114), 0x94);
        if (Skeleton_FindBone(skeleton, HashName(boneName), &bone)) {
            Keeper_Start(At<uint8_t*>(ped, 0x2E4), effect, bone);
        }
        return 0;
    }
    uint8_t* record = At<uint8_t*>(At<uint8_t*>(self, 0x24), 0x10);
    if (record[0] == 1) {
        V3 pos = {At<float>(record, 8), At<float>(record, 0xC), At<float>(record, 0x10)};
        Keeper_StartAt(At<uint8_t*>(ped, 0x2E4), effect, &pos);
    }
    return 0;
}

// 0x6098E0 (thiscall): DialogAddAttachedEffectTrack::Start
uint8_t __fastcall DialogAddAttachedEffectTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    self[0x2C] = 0;
    return 1;
}

// 0x6098F0 / 0x6099C0 (thiscall): BreathTrack's / FXCeilingTrack's fields
void __fastcall BreathTrack_InitFields(uint8_t* self, re::Edx) {
    self[8] = 0;
    ClearName(self, 0x18);
    ClearName(self, 0x1C);
    At<uint32_t>(self, 0x20) = 0;
    At<uint32_t>(self, 0x24) = 0;
    self[0x28] = 1;
}
void __fastcall FXCeilingTrack_InitFields(uint8_t* self, re::Edx) {
    self[8] = 0;
    ClearName(self, 0x18);
}

// 0x609950 (thiscall): BreathTrack's deleting destructor (its effect +0x2C stopped)
uint8_t* __fastcall BreathTrack_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kVtBreathTrack;
    Effect_Stop(self + 0x2C);
    EffectProxy_Destruct(self + 0x2C);
    NameKey_Destruct(self + 0x1C);
    NameKey_Destruct(self + 0x18);
    TrackBase_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x6099A0 (thiscall): BreathTrack::End
uint8_t __fastcall BreathTrack_End(uint8_t* self, re::Edx) {
    Effect_Stop(self + 0x2C);
    AE_Proxy_Kill(self + 0x2C);
    return 0;
}

// 0x609A20 / 0x609AC0 (thiscall): FXDecalTrack's fields (and the dialog one's)
void __fastcall FXDecalTrack_InitFields(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0x20) = 0;
    self[8] = 0;
    ClearName(self, 0x18);
    ClearName(self, 0x1C);
}
void __fastcall DialogFXDecalTrack_InitFields(uint8_t* self, re::Edx) {
    reinterpret_cast<void(__thiscall*)(uint8_t*)>(0x00609A20)(self);
    At<uint32_t>(self, 0x28) = 0;
}

// 0x609B10 (thiscall): DialogFXDecalTrack::Start
uint8_t __fastcall DialogFXDecalTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    self[0x2C] = 0;
    return 0;
}

// 0x609B20 / 0x609C00 / 0x609CA0 (thiscall): the effect tracks' fields
void __fastcall FXWaterRipplesTrack_InitFields(uint8_t* self, re::Edx) {
    self[8] = 0;
    ClearName(self, 0x18);
    ClearName(self, 0x1C);
}
void __fastcall GroundImpactTrack_InitFields(uint8_t* self, re::Edx) {
    self[8] = 0;
    ClearName(self, 0x1C);
    ClearName(self, 0x18);
}
void __fastcall WallImpactTrack_InitFields(uint8_t* self, re::Edx) {
    self[8] = 0;
    ClearName(self, 0x1C);
    ClearName(self, 0x18);
    At<float>(self, 0x20) = -1.0f;
}

// 0x609B70 / 0x609D40 (thiscall): the spawned effect tracks' deleting destructors (their
// effect +0x28)
template <uint32_t Vt>
uint8_t* __fastcall SpawnedEffectTrack_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = Vt;
    AE_Proxy_Destruct(self + 0x28);
    NameKey_Destruct(self + 0x1C);
    NameKey_Destruct(self + 0x18);
    TrackBase_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x609BB0 (thiscall): FXWaterRipplesTrack::Start: its effect's id
uint8_t __fastcall FXWaterRipplesTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    At<uint32_t>(self, 0x24) = EffectLibrary_Find(effectLibrary, At<const char*>(self, 0x18));
    return 0;
}

// 0x609BE0 (thiscall): FXWaterRipplesTrack::End
uint8_t __fastcall FXWaterRipplesTrack_End(uint8_t* self, re::Edx) {
    AE_Proxy_SlowKill(self + 0x28, 1.0f);
    return 0;
}

// 0x609C90 (thiscall): the tracks' keep-running Update (always true)
uint8_t __fastcall Track_KeepRunning(uint8_t*, re::Edx, float) {
    return 1;
}

// 0x609D80 (thiscall): FXWeaponImpactTrack::Start
uint8_t __fastcall FXWeaponImpactTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    return 0;
}

// 0x609D90 (thiscall): FXWeaponImpactTrack::End
uint8_t __fastcall FXWeaponImpactTrack_End(uint8_t* self, re::Edx) {
    AE_Proxy_Fade(self + 0x28, 0.0f);
    AE_Proxy_Kill(self + 0x28);
    return 0;
}

// 0x609DB0 (thiscall): ScreenFxDizzyTrack's fields
void __fastcall ScreenFxDizzyTrack_InitFields(uint8_t* self, re::Edx) {
    At<int32_t>(self, 0x18) = 1000;
    At<float>(self, 0x24) = 1000.0f;
    At<int32_t>(self, 0x20) = 1000;
    At<int32_t>(self, 0x1C) = 1000;
    self[8] = 1;
    At<int32_t>(self, 0x28) = 0x41;
    At<int32_t>(self, 0x2C) = 0xFF;
    At<int32_t>(self, 0x30) = 0x2D;
    At<int32_t>(self, 0x34) = 0xFF;
}

// 0x609E20 (thiscall): ScreenFxDizzyTrack::Start: the dizziness on, its colour set, from
// the current amount
uint8_t __fastcall ScreenFxDizzyTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x38) = owner;
    dizzyOn = 1;
    At<int32_t>(self, 0x3C) = 0;
    At<uint32_t>(self, 0x40) = timeInMs;
    float amount = dizzyAmount;
    dizzyColour[0] = self[0x28];
    dizzyColour[1] = self[0x2C];
    dizzyColour[2] = self[0x30];
    dizzyColour[3] = self[0x34];
    At<float>(self, 0x44) = amount;
    return 1;
}

// the milliseconds since `since` over a duration (the game's unsigned int conversions)
float Fraction(uint32_t since, int32_t duration) {
    return float(double(timeInMs - since) / double(float(duration)));
}

// 0x609E80 (thiscall): ScreenFxDizzyTrack::Update: fades in over +0x18 ms to +0x24, holds
// +0x1C ms, fades out over +0x20 ms (phase +0x3C since +0x40)
uint8_t __fastcall ScreenFxDizzyTrack_Update(uint8_t* self, re::Edx, float) {
    if (float(At<int32_t>(self, 0x18)) == 0.0f && float(At<int32_t>(self, 0x1C)) == 0.0f &&
        float(At<int32_t>(self, 0x20)) == 0.0f) {
        dizzyAmount = At<float>(self, 0x24);
        return 0;
    }
    for (;;) {
        int32_t phase = At<int32_t>(self, 0x3C);
        if (phase == 0) {
            if (float(At<int32_t>(self, 0x18)) == 0.0f) {
                At<int32_t>(self, 0x3C) = 1;
                At<uint32_t>(self, 0x40) = timeInMs;
            } else {
                float r = Fraction(At<uint32_t>(self, 0x40), At<int32_t>(self, 0x18));
                if (r >= 1.0f) {
                    At<int32_t>(self, 0x3C) = 1;
                    At<uint32_t>(self, 0x40) = timeInMs;
                    r = 1.0f;
                }
                dizzyAmount = (1.0f - r) * At<float>(self, 0x44) + At<float>(self, 0x24) * r;
                if (r >= 1.0f) {
                    At<float>(self, 0x44) = dizzyAmount;
                }
            }
        } else if (phase == 1) {
            if (timeInMs - At<uint32_t>(self, 0x40) >= At<uint32_t>(self, 0x1C)) {
                At<int32_t>(self, 0x3C) = 2;
                At<float>(self, 0x44) = dizzyAmount;
                At<uint32_t>(self, 0x40) = timeInMs;
            }
        } else if (phase == 2) {
            if (float(At<int32_t>(self, 0x20)) == 0.0f) {
                return 0;
            }
            float r = Fraction(At<uint32_t>(self, 0x40), At<int32_t>(self, 0x20));
            if (r >= 1.0f) {
                r = 1.0f;
            }
            dizzyAmount = (1.0f - r) * At<float>(self, 0x24);
            if (r >= 1.0f) {
                return 0;
            }
        }
        if (phase == At<int32_t>(self, 0x3C)) {
            return 1;
        }
    }
}

// 0x60A040 (thiscall): ScreenFxDizzyTrack::End: stays on while there's an amount
uint8_t __fastcall ScreenFxDizzyTrack_End(uint8_t*, re::Edx) {
    dizzyOn = 1;
    if (0.0f == dizzyAmount) {
        dizzyOn = 0;
    }
    ScreenEffects_Update(screenEffects_CF4264);
    return 0;
}

// 0x60A080 (thiscall): PurchaseItemTrack::Start: the shop's purchase
uint8_t __fastcall PurchaseItemTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    Shop_Purchase(shop);
    return 0;
}

// 0x60A0A0 (thiscall): CameraShotTrack's fields
void __fastcall CameraShotTrack_InitFields(uint8_t* self, re::Edx) {
    self[8] = 0;
    ClearName(self, 0x18);
    self[0x1C] = 0;
}

// 0x60A100 (thiscall): CameraShotTrack::Start: the camera's shot +0x18 (+0x1C: reset)
uint8_t __fastcall CameraShotTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    if (re::SeamlessDoorHoldsCamera("CameraShotTrack")) {
        return 1;   // (the remaster's seamless doors: runflags.h)
    }
    Camera_SetShotName(camera, At<uint32_t>(self, 0x18));
    if (self[0x1C] != 0) {
        Camera_ResetController(camera);
    }
    return 1;
}

// 0x60A130 (thiscall): CameraShakeTrack's fields
void __fastcall CameraShakeTrack_InitFields(uint8_t* self, re::Edx) {
    At<float>(self, 0x1C) = 20.0f;
    self[8] = 1;
    self[0x18] = 1;
    At<float>(self, 0x24) = 0.1f;
    At<float>(self, 0x28) = 0.3f;
    At<float>(self, 0x2C) = 0.05f;
    At<float>(self, 0x20) = 0.0f;
    At<float>(self, 0x30) = 1.0f;
    At<float>(self, 0x34) = 0.1f;
    At<float>(self, 0x38) = 0.1f;
    At<float>(self, 0x3C) = 0.1f;
    At<float>(self, 0x40) = 5.0f;
    At<float>(self, 0x44) = 5.0f;
    At<float>(self, 0x48) = 5.0f;
}

// 0x60A1B0 (thiscall): CameraShakeTrack::Start (+0x18: the shake on)
uint8_t __fastcall CameraShakeTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x4C) = owner;
    self[0x58] = 1;
    At<uint32_t>(self, 0x54) = timeInMs;
    At<uint32_t>(self, 0x50) = timeInMs;
    if (self[0x18] != 0) {
        CameraShake_Enable(cameraShake, 1);
    }
    return 1;
}

// 0x60A1F0 (thiscall): CameraShakeTrack::Update: a sine shake (+0x1C Hz, +0x20 degrees
// phase) ramped in over +0x24 s and out over the last +0x28 s, clipped at +-+0x30,
// jittered by +-+0x2C; offsets +0x34.. and rotations +0x40.. set (+0x18) or added
uint8_t __fastcall CameraShakeTrack_Update(uint8_t* self, re::Edx, float) {
    double now = double(int32_t(timeInMs));
    int32_t endMs = static_cast<int32_t>(float(double(acttree::TrackEnd(self)) * -1000.0));
    double outStart = double(int32_t(At<uint32_t>(self, 0x54) - uint32_t(endMs))) - double(At<float>(self, 0x28)) * 1000.0;
    if (outStart < now) {
        self[0x58] = 0;
        At<uint32_t>(self, 0x54) = timeInMs;
    }
    uint8_t rampingIn = self[0x58];
    float ramp = rampingIn ? At<float>(self, 0x24) : At<float>(self, 0x28);
    uint32_t elapsed = timeInMs - At<uint32_t>(self, 0x54);
    float rampMs = float(double(ramp) * 1000.0);
    float r = float(double(elapsed) / double(rampMs));
    if (r < 0.0f) {
        r = 0.0f;
    } else if (!(r < 1.0f) && !std::isnan(r)) {
        r = 1.0f;
    }
    if (!rampingIn) {
        r = 1.0f - r;
    }
    float t = float(double(timeInMs - At<uint32_t>(self, 0x50)) * 0.001);
    float phase = float(double(At<float>(self, 0x20)) * double(3.14159274f) / 180.0);
    float angle = float(double(t * At<float>(self, 0x1C)) * double(6.28318548f)) + phase;
    float wave = float(std::sin(double(angle)));
    if (!(1.0f == At<float>(self, 0x30))) {
        if (-At<float>(self, 0x30) > wave) {
            wave = -1.0f;
        } else if (At<float>(self, 0x30) < wave) {
            wave = 1.0f;
        }
    }
    float hi = At<float>(self, 0x2C);
    float lo = -At<float>(self, 0x2C);
    float q = float(double(Rand()) / 32767.0);
    float jitter = q * (hi - lo) + lo;
    float scaled = wave * r;
    float amount = (jitter + 1.0f) * scaled;
    float ox = At<float>(self, 0x34) * amount;
    float oy = At<float>(self, 0x38) * amount;
    float oz = At<float>(self, 0x3C) * amount;
    float rx = At<float>(self, 0x40) * amount;
    float ry = At<float>(self, 0x44) * amount;
    float rz = amount * At<float>(self, 0x48);
    if (self[0x18] != 0) {
        CameraShake_SetOffset(cameraShake, ox, oy, oz);
        CameraShake_SetRotation(cameraShake, rx, ry, rz);
        return 1;
    }
    CameraShake_AddOffset(cameraShake, ox, oy, oz);
    CameraShake_AddRotation(cameraShake, rx, ry, rz);
    return 1;
}

// 0x60A4B0 (thiscall): CameraShakeTrack::End
uint8_t __fastcall CameraShakeTrack_End(uint8_t* self, re::Edx) {
    if (self[0x18] != 0) {
        CameraShake_Enable(cameraShake, 0);
    }
    return 1;
}

// 0x60A4D0 / 0x60A510 (thiscall): CameraFreezeTrack's / GameTimerTrack's fields
void __fastcall CameraFreezeTrack_InitFields(uint8_t* self, re::Edx) {
    self[8] = 0;
    self[0x18] = 0;
}
void __fastcall GameTimerTrack_InitFields(uint8_t* self, re::Edx) {
    At<float>(self, 0x1C) = 5.0f;
    self[8] = 1;
    self[0x20] = 1;
    self[0x21] = 0;
    ClearName(self, 0x18);
    self[0x22] = 1;
}

// 0x60A590 (thiscall): GameTimerTrack::Start (+0x22: the big message cleared)
uint8_t __fastcall GameTimerTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    At<uint32_t>(self, 0x28) = timeInMs;
    if (self[0x22] != 0) {
        BigMessage_Clear();
    }
    return 1;
}

// 0x60A5B0 (thiscall): GameTimerTrack::Update: the seconds shown (+0x20 counting down
// from +0x1C; +0x21 whole seconds) under text +0x18; runs until they're up
uint8_t __fastcall GameTimerTrack_Update(uint8_t* self, re::Edx, float) {
    float seconds = float(float(double(timeInMs) - double(At<uint32_t>(self, 0x28))) / 1000.0);
    BigMessage_Clear();
    if (self[0x20] != 0) {
        seconds = At<float>(self, 0x1C) - seconds;
    }
    float shown = seconds;
    if (self[0x21] != 0) {
        shown = float(static_cast<int>(seconds));
    }
    BigMessage_AddNumber(shown);
    BigMessage_Show(At<uint32_t>(self, 0x18), 1.1f);
    if (self[0x20] == 0) {
        seconds = At<float>(self, 0x1C) - seconds;
    }
    return 0.0f > seconds ? 0 : 1;
}

// 0x60A670 (thiscall): GameTimerTrack::End
uint8_t __fastcall GameTimerTrack_End(uint8_t* self, re::Edx) {
    if (self[0x22] != 0) {
        BigMessage_Clear();
    }
    return 1;
}

// 0x60A680 / 0x60A6C0 / 0x60A700 / 0x60A730 (thiscall): the rail driver, set pos / rot,
// shop state and end mission tracks' fields
void __fastcall RailDriverTrack_InitFields(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<float>(self, 0x18) = 1.0f;
    self[0x28] = 0;
}
void __fastcall SetPosRotTrack_InitFields(uint8_t* self, re::Edx) {
    At<float>(self, 0x18) = 0.0f;
    At<float>(self, 0x1C) = 0.0f;
    At<float>(self, 0x20) = 0.0f;
    At<float>(self, 0x24) = 0.0f;
}
void __fastcall ShopSetStateTrack_InitFields(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0x18) = 0;
    self[8] = 1;
}
void __fastcall EndMissionTrack_InitFields(uint8_t* self, re::Edx) {
    self[8] = 1;
    self[0x18] = 0;
    self[0x19] = 0;
    self[0x1A] = 0;
}

// 0x60A710 (thiscall): ShopSetStateTrack::Start: the shop's state (its first field)
uint8_t __fastcall ShopSetStateTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    StringRef_Assign(static_cast<uint32_t*>(shop), At<uint32_t>(self, 0x18));
    return 0;
}

// a mission controller ended (the mission manager's +0x12 / +0xC flags first)
template <uintptr_t GetController>
void EndController() {
    auto get = reinterpret_cast<uint8_t*(__thiscall*)(void*)>(GetController);
    get(missionManager)[0x12] = 1;
    get(missionManager)[0xC] = 0;
    MissionController_Fn6ACA70(get(missionManager));
    MissionController_End(get(missionManager), 0, 0, 0, 0, 0, 0, 0);
}

// 0x60A740 (thiscall): EndMissionTrack::Start: the primary (+0x18) / secondary (+0x19)
// mission ended
uint8_t __fastcall EndMissionTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    if (self[0x18] != 0) {
        EndController<0x006AA680>();
    }
    if (self[0x19] != 0) {
        EndController<0x006AA690>();
    }
    return 0;
}

// 0x60A860 / 0x60A8A0 / 0x60A8E0 (thiscall): the gift tracks' fields
void __fastcall AcceptGiftTrack_InitFields(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0x18) = 0;
}
void __fastcall SetCanAcceptGiftTrack_InitFields(uint8_t* self, re::Edx) {
    self[0x18] = 1;
    At<uint32_t>(self, 0x1C) = 0;
}
void __fastcall SetOverheadIconTrack_InitFields(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0x18) = 0;
    self[0x1C] = 0;
}

// 0x60A920 / 0x60A940 / 0x60A960 / 0x60A980 (thiscall): the stat tracks' fields
void __fastcall StatTrack_InitFields_60A920(uint8_t* self, re::Edx) {
    self[0x18] = 0;
    self[0x19] = 0;
    self[8] = 1;
    At<uint32_t>(self, 0x20) = 0;
    At<uint32_t>(self, 0x24) = 0;
}
void __fastcall StatTrack_InitFields_60A940(uint8_t* self, re::Edx) {
    At<float>(self, 0x24) = 0.0f;
    self[0x18] = 0;
    self[0x19] = 0;
    self[8] = 1;
    At<uint32_t>(self, 0x20) = 0;
}
void __fastcall StatTrack_InitFields_60A960(uint8_t* self, re::Edx) {
    self[0x18] = 0;
    self[0x19] = 0;
    self[8] = 1;
    At<uint32_t>(self, 0x20) = 0;
    At<int32_t>(self, 0x24) = 1;
}
void __fastcall StatTrack_InitFields_60A980(uint8_t* self, re::Edx) {
    At<float>(self, 0x24) = 1.0f;
    self[0x18] = 0;
    self[0x19] = 0;
    self[8] = 1;
    At<uint32_t>(self, 0x20) = 0;
}

// 0x60A9A0 / 0x60A9F0 (thiscall): the balance meter's and a named track's fields
void __fastcall BalanceMeterTrack_InitFields(uint8_t* self, re::Edx) {
    At<float>(self, 0x18) = 20.0f;
    self[8] = 0;
    self[0x34] = 0;
    At<float>(self, 0x1C) = 0.03f;
    self[0x35] = 0;
    At<int32_t>(self, 0x30) = 1;
    At<float>(self, 0x20) = 0.05f;
    At<float>(self, 0x24) = 0.3f;
    At<float>(self, 0x28) = 0.03f;
    At<float>(self, 0x2C) = 0.1f;
}
void __fastcall NamedTrack_InitFields_60A9F0(uint8_t* self, re::Edx) {
    SetNameRaw(self, 0x18, emptyString);
    At<float>(self, 0xC) = 0.0f;
    At<int32_t>(self, 0x1C) = 1;
}

// 0x60AA30 (thiscall): GiveItemToPlayerTrack::Start: the model named +0x18 given to the
// player (a weapon with +0x1C ammo, or an item)
uint8_t __fastcall GiveItemToPlayerTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    int model = GetModelIndexByHash(At<uint32_t>(self, 0x18), 0, 0x2FA7);
    if (model >= weaponModelFirst && model <= weaponModelLast) {
        Inventory_Give(At<uint8_t*>(playerPed, 0x1C4), model, At<int32_t>(self, 0x1C), 1, 0);
        return 1;
    }
    if (model >= itemModelFirst && model <= itemModelLast) {
        Items_Give(model, 1, 1, 0);
    }
    return 1;
}

// 0x60AAB0 / 0x60AB10 / 0x60AB30 (thiscall): the offset camera tracks' fields
void __fastcall OffsetTrack_InitFields_60AAB0(uint8_t* self, re::Edx) {
    self[0x18] = 1;
    At<float>(self, 0x1C) = 0.0f;
    At<float>(self, 0x20) = 0.0f;
    At<float>(self, 0x28) = zeroVector[0];
    At<float>(self, 0x2C) = zeroVector[1];
    At<float>(self, 0x30) = zeroVector[2];
    At<float>(self, 0x34) = zeroVector[0];
    At<float>(self, 0x38) = zeroVector[1];
    At<float>(self, 0x40) = 0.0f;
    At<float>(self, 0x3C) = zeroVector[2];
    At<float>(self, 0x44) = zeroVector[0];
    At<float>(self, 0x48) = zeroVector[1];
    At<float>(self, 0x4C) = zeroVector[2];
}
void __fastcall OffsetTrack_InitFields_60AB10(uint8_t* self, re::Edx) {
    self[0x18] = 1;
    At<float>(self, 0x1C) = 0.0f;
    At<float>(self, 0x20) = 0.0f;
    At<float>(self, 0x28) = 0.0f;
    At<float>(self, 0x2C) = 0.0f;
    At<float>(self, 0x30) = 0.0f;
    At<float>(self, 0x34) = 0.0f;
}
void __fastcall Track_InitFields_60AB30(uint8_t* self, re::Edx) {
    At<float>(self, 0x18) = 0.0f;
}

// 0x60AB40 (thiscall): SaveGameTrack::Start: the save point's area remembered, a save
// requested
uint8_t __fastcall SaveGameTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    uint8_t* entity = UsePoint_GetEntity(owner);
    if (entity && At<uint8_t*>(entity, 0x23C)) {
        saveGameArea_20CECE4 = At<int32_t>(At<uint8_t*>(entity, 0x23C), 0x2C);
    }
    SaveGame_Request();
    return 1;
}

// 0x60AB70 / 0x60ABB0 / 0x60ABD0 / 0x60ABF0 / 0x60AC50 / 0x60AC80 / 0x60ACF0 (thiscall):
// more tracks' fields
void __fastcall NamedTrack_InitFields_60AB70(uint8_t* self, re::Edx) {
    self[8] = 0;
    SetNameRaw(self, 0x18, emptyString);
    self[0x1C] = 0;
    self[0x1D] = 0;
    self[0x1E] = 0;
    self[0x1F] = 0;
    At<uint32_t>(self, 0x20) = 0;
}
void __fastcall Track_InitFields_60ABB0(uint8_t* self, re::Edx) {
    self[8] = 0;
    At<float>(self, 0xC) = 0.0f;
    At<float>(self, 0x10) = -1.0f;
    At<float>(self, 0x18) = 0.5f;
}
void __fastcall Track_InitFields_60ABD0(uint8_t* self, re::Edx) {
    At<float>(self, 0xC) = 0.0f;
    self[8] = 0;
    At<uint32_t>(self, 0x18) = 0;
    At<float>(self, 0x10) = -1.0f;
    self[0x1C] = 0;
}
void __fastcall CompletionDisplayTrack_InitFields(uint8_t* self, re::Edx) {
    At<float>(self, 0x20) = 0.0f;
    self[8] = 0;
    self[0x1C] = 1;
    self[0x28] = 0;
}
void __fastcall HealthMeterTrack_InitFields(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0x20) = 0;
    At<float>(self, 0xC) = 0.0f;
    At<float>(self, 0x10) = -1.0f;
    At<float>(self, 0x18) = -1.0f;
    At<float>(self, 0x1C) = -1.0f;
}
void __fastcall FallingDamageTrack_InitFields(uint8_t* self, re::Edx) {
    At<float>(self, 0x1C) = 0.0f;
    self[8] = 0;
    self[0x18] = 1;
    At<float>(self, 0x20) = -1.0f;
    At<uint32_t>(self, 0x28) = 0;
    At<float>(self, 0x24) = 0.0f;
    At<float>(self, 0x30) = 0.0f;
    At<float>(self, 0x3C) = 0.0f;
    At<float>(self, 0x40) = zeroVector[0];
    At<float>(self, 0x44) = zeroVector[1];
    At<float>(self, 0x48) = zeroVector[2];
}
void __fastcall FallingDamageHandleTrack_InitFields(uint8_t* self, re::Edx) {
    self[8] = 0;
    At<float>(self, 0x18) = 1.0f;
}

// 0x60AC10 (thiscall): CompletionDisplayTrack::Start: the completion message (+0x28: the
// uber completion)
uint8_t __fastcall CompletionDisplayTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x2C) = owner;
    ShowCompletion(At<uint32_t>(self, 0x18), self[0x1C], At<float>(self, 0x20), At<int32_t>(self, 0x24), 0, 0);
    if (self[0x28] != 0) {
        uberCompletion_20C87A8 = 1;
    }
    return 1;
}

// 0x60AC70 (thiscall): usePelvisForHealthMeterTrack::Update: runs unless +0x20 is 1
uint8_t __fastcall HealthMeterTrack_Update(uint8_t* self, re::Edx, float) {
    return At<int32_t>(self, 0x20) != 1 ? 1 : 0;
}

// 0x60AD60 (thiscall): the first of a list (+0xDC) when it has any (+0xF0)
uint32_t __fastcall List_FirstOrNone(uint8_t* self, re::Edx) {
    if (At<int32_t>(self, 0xF0) != 0) {
        return *At<uint32_t*>(self, 0xDC);
    }
    return 0;
}

// 0x60AD80 / 0x60AD90 (thiscall): DamageTrack's / FallingDamageTrack's Resolve: the
// owner's (+0x24 / +0x2C) ped
template <int Offset>
uint8_t* __fastcall OwnerPedOf(uint8_t* self, re::Edx) {
    return OwnerPed(At<uint8_t*>(self, Offset));
}

} // namespace

#define ACTTRACKSFX_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

ACTTRACKSFX_REVERSED("DetachEffectTrack::DeletingDestruct", 0x006096D0,
                     &TrackDeletingDtor<kVtDetachEffectTrack, 0x1C, 0x18>);
ACTTRACKSFX_REVERSED("AddAttachedEffectTrack::InitFields", 0x00609710, &AddAttachedEffectTrack_InitFields);
ACTTRACKSFX_REVERSED("AddAttachedEffectTrack::DeletingDestruct", 0x00609770,
                     &TrackDeletingDtor<kVtAddAttachedEffectTrack, 0x1C, 0x18>);
ACTTRACKSFX_REVERSED("AddAttachedEffectTrack::AttachTo", 0x006097B0, &AddAttachedEffectTrack_AttachTo);
ACTTRACKSFX_REVERSED("DialogAddAttachedEffectTrack::InitFields", 0x00609890, &DialogAddAttachedEffectTrack_InitFields);
ACTTRACKSFX_REVERSED("DialogAddAttachedEffectTrack::Destruct", 0x006098B0,
                     &TrackDtor<kVtAddAttachedEffectTrack, 0x1C, 0x18>);
ACTTRACKSFX_REVERSED("DialogAddAttachedEffectTrack::Start", 0x006098E0, &DialogAddAttachedEffectTrack_Start);
ACTTRACKSFX_REVERSED("BreathTrack::InitFields", 0x006098F0, &BreathTrack_InitFields);
ACTTRACKSFX_REVERSED("BreathTrack::DeletingDestruct", 0x00609950, &BreathTrack_DeletingDestruct);
ACTTRACKSFX_REVERSED("BreathTrack::End", 0x006099A0, &BreathTrack_End);
ACTTRACKSFX_REVERSED("FXCeilingTrack::InitFields", 0x006099C0, &FXCeilingTrack_InitFields);
ACTTRACKSFX_REVERSED("FXCeilingTrack::DeletingDestruct", 0x006099F0, &TrackDeletingDtor<kVtFXCeilingTrack, 0x18>);
ACTTRACKSFX_REVERSED("FXDecalTrack::InitFields", 0x00609A20, &FXDecalTrack_InitFields);
ACTTRACKSFX_REVERSED("FXDecalTrack::DeletingDestruct", 0x00609A80, &TrackDeletingDtor<kVtFXDecalTrack, 0x1C, 0x18>);
ACTTRACKSFX_REVERSED("DialogFXDecalTrack::InitFields", 0x00609AC0, &DialogFXDecalTrack_InitFields);
ACTTRACKSFX_REVERSED("DialogFXDecalTrack::Destruct", 0x00609AE0, &TrackDtor<kVtFXDecalTrack, 0x1C, 0x18>);
ACTTRACKSFX_REVERSED("DialogFXDecalTrack::Start", 0x00609B10, &DialogFXDecalTrack_Start);
ACTTRACKSFX_REVERSED("FXWaterRipplesTrack::InitFields", 0x00609B20, &FXWaterRipplesTrack_InitFields);
ACTTRACKSFX_REVERSED("FXWaterRipplesTrack::DeletingDestruct", 0x00609B70,
                     &SpawnedEffectTrack_DeletingDestruct<kVtFXWaterRipplesTrack>);
ACTTRACKSFX_REVERSED("FXWaterRipplesTrack::Start", 0x00609BB0, &FXWaterRipplesTrack_Start);
ACTTRACKSFX_REVERSED("FXWaterRipplesTrack::End", 0x00609BE0, &FXWaterRipplesTrack_End);
ACTTRACKSFX_REVERSED("GroundImpactTrack::InitFields", 0x00609C00, &GroundImpactTrack_InitFields);
ACTTRACKSFX_REVERSED("GroundImpactTrack::DeletingDestruct", 0x00609C50,
                     &TrackDeletingDtor<kVtGroundImpactTrack, 0x1C, 0x18>);
ACTTRACKSFX_REVERSED("Track::KeepRunning_609C90", 0x00609C90, &Track_KeepRunning);
ACTTRACKSFX_REVERSED("WallImpactTrack::InitFields", 0x00609CA0, &WallImpactTrack_InitFields);
ACTTRACKSFX_REVERSED("WallImpactTrack::DeletingDestruct", 0x00609D00, &TrackDeletingDtor<kVtWallImpactTrack, 0x1C, 0x18>);
ACTTRACKSFX_REVERSED("FXWeaponImpactTrack::DeletingDestruct", 0x00609D40,
                     &SpawnedEffectTrack_DeletingDestruct<kVtFXWeaponImpactTrack>);
ACTTRACKSFX_REVERSED("FXWeaponImpactTrack::Start", 0x00609D80, &FXWeaponImpactTrack_Start);
ACTTRACKSFX_REVERSED("FXWeaponImpactTrack::End", 0x00609D90, &FXWeaponImpactTrack_End);
ACTTRACKSFX_REVERSED("ScreenFxDizzyTrack::InitFields", 0x00609DB0, &ScreenFxDizzyTrack_InitFields);
ACTTRACKSFX_REVERSED("ScreenFxDizzyTrack::DeletingDestruct", 0x00609DF0, &TrackDeletingDtor<kVtScreenFxDizzyTrack>);
ACTTRACKSFX_REVERSED("ScreenFxDizzyTrack::Start", 0x00609E20, &ScreenFxDizzyTrack_Start);
ACTTRACKSFX_REVERSED("ScreenFxDizzyTrack::Update", 0x00609E80, &ScreenFxDizzyTrack_Update);
ACTTRACKSFX_REVERSED("ScreenFxDizzyTrack::End", 0x0060A040, &ScreenFxDizzyTrack_End);
ACTTRACKSFX_REVERSED("PurchaseItemTrack::Start", 0x0060A080, &PurchaseItemTrack_Start);
ACTTRACKSFX_REVERSED("CameraShotTrack::InitFields", 0x0060A0A0, &CameraShotTrack_InitFields);
ACTTRACKSFX_REVERSED("CameraShotTrack::DeletingDestruct", 0x0060A0D0, &TrackDeletingDtor<kVtCameraShotTrack, 0x18>);
ACTTRACKSFX_REVERSED("CameraShotTrack::Start", 0x0060A100, &CameraShotTrack_Start);
ACTTRACKSFX_REVERSED("CameraShakeTrack::InitFields", 0x0060A130, &CameraShakeTrack_InitFields);
ACTTRACKSFX_REVERSED("CameraShakeTrack::DeletingDestruct", 0x0060A180, &TrackDeletingDtor<kVtCameraShakeTrack>);
ACTTRACKSFX_REVERSED("CameraShakeTrack::Start", 0x0060A1B0, &CameraShakeTrack_Start);
ACTTRACKSFX_REVERSED("CameraShakeTrack::Update", 0x0060A1F0, &CameraShakeTrack_Update);
ACTTRACKSFX_REVERSED("CameraShakeTrack::End", 0x0060A4B0, &CameraShakeTrack_End);
ACTTRACKSFX_REVERSED("CameraFreezeTrack::InitFields", 0x0060A4D0, &CameraFreezeTrack_InitFields);
ACTTRACKSFX_REVERSED("CameraFreezeTrack::DeletingDestruct", 0x0060A4E0, &TrackDeletingDtor<kVtCameraFreezeTrack>);
ACTTRACKSFX_REVERSED("GameTimerTrack::InitFields", 0x0060A510, &GameTimerTrack_InitFields);
ACTTRACKSFX_REVERSED("GameTimerTrack::DeletingDestruct", 0x0060A560, &TrackDeletingDtor<kVtGameTimerTrack, 0x18>);
ACTTRACKSFX_REVERSED("GameTimerTrack::Start", 0x0060A590, &GameTimerTrack_Start);
ACTTRACKSFX_REVERSED("GameTimerTrack::Update", 0x0060A5B0, &GameTimerTrack_Update);
ACTTRACKSFX_REVERSED("GameTimerTrack::End", 0x0060A670, &GameTimerTrack_End);
ACTTRACKSFX_REVERSED("RailDriverTrack::InitFields", 0x0060A680, &RailDriverTrack_InitFields);
ACTTRACKSFX_REVERSED("RailDriverTrack::DeletingDestruct", 0x0060A690, &TrackDeletingDtor<kVtRailDriverTrack>);
ACTTRACKSFX_REVERSED("SetPosRotTrack::InitFields", 0x0060A6C0, &SetPosRotTrack_InitFields);
ACTTRACKSFX_REVERSED("SetPosRotTrack::DeletingDestruct", 0x0060A6D0, &TrackDeletingDtor<kVtSetPosRotTrack>);
ACTTRACKSFX_REVERSED("ShopSetStateTrack::InitFields", 0x0060A700, &ShopSetStateTrack_InitFields);
ACTTRACKSFX_REVERSED("ShopSetStateTrack::Start", 0x0060A710, &ShopSetStateTrack_Start);
ACTTRACKSFX_REVERSED("EndMissionTrack::InitFields", 0x0060A730, &EndMissionTrack_InitFields);
ACTTRACKSFX_REVERSED("EndMissionTrack::Start", 0x0060A740, &EndMissionTrack_Start);
ACTTRACKSFX_REVERSED("CreateGiftRewardPickupTrack::DeletingDestruct", 0x0060A800,
                     &TrackDeletingDtor<kVtCreateGiftRewardPickupTrack>);
ACTTRACKSFX_REVERSED("AcceptGiftTrack::DeletingDestruct", 0x0060A830, &TrackDeletingDtor<kVtAcceptGiftTrack>);
ACTTRACKSFX_REVERSED("AcceptGiftTrack::InitFields", 0x0060A860, &AcceptGiftTrack_InitFields);
ACTTRACKSFX_REVERSED("RemoveItemTrack::DeletingDestruct", 0x0060A870, &TrackDeletingDtor<kVtRemoveItemTrack>);
ACTTRACKSFX_REVERSED("SetCanAcceptGiftTrack::InitFields", 0x0060A8A0, &SetCanAcceptGiftTrack_InitFields);
ACTTRACKSFX_REVERSED("SetCanAcceptGiftTrack::DeletingDestruct", 0x0060A8B0, &TrackDeletingDtor<kVtSetCanAcceptGiftTrack>);
ACTTRACKSFX_REVERSED("SetOverheadIconTrack::InitFields", 0x0060A8E0, &SetOverheadIconTrack_InitFields);
ACTTRACKSFX_REVERSED("SetOverheadIconTrack::DeletingDestruct", 0x0060A8F0, &TrackDeletingDtor<kVtSetOverheadIconTrack>);
ACTTRACKSFX_REVERSED("StatTrack::InitFields_60A920", 0x0060A920, &StatTrack_InitFields_60A920);
ACTTRACKSFX_REVERSED("StatTrack::InitFields_60A940", 0x0060A940, &StatTrack_InitFields_60A940);
ACTTRACKSFX_REVERSED("StatTrack::InitFields_60A960", 0x0060A960, &StatTrack_InitFields_60A960);
ACTTRACKSFX_REVERSED("StatTrack::InitFields_60A980", 0x0060A980, &StatTrack_InitFields_60A980);
ACTTRACKSFX_REVERSED("BalanceMeterTrack::InitFields", 0x0060A9A0, &BalanceMeterTrack_InitFields);
ACTTRACKSFX_REVERSED("NamedTrack::InitFields_60A9F0", 0x0060A9F0, &NamedTrack_InitFields_60A9F0);
ACTTRACKSFX_REVERSED("GiveItemToPlayerTrack::Start", 0x0060AA30, &GiveItemToPlayerTrack_Start);
ACTTRACKSFX_REVERSED("OffsetTrack::InitFields_60AAB0", 0x0060AAB0, &OffsetTrack_InitFields_60AAB0);
ACTTRACKSFX_REVERSED("OffsetTrack::InitFields_60AB10", 0x0060AB10, &OffsetTrack_InitFields_60AB10);
ACTTRACKSFX_REVERSED("Track::InitFields_60AB30", 0x0060AB30, &Track_InitFields_60AB30);
ACTTRACKSFX_REVERSED("SaveGameTrack::Start", 0x0060AB40, &SaveGameTrack_Start);
ACTTRACKSFX_REVERSED("NamedTrack::InitFields_60AB70", 0x0060AB70, &NamedTrack_InitFields_60AB70);
ACTTRACKSFX_REVERSED("Track::InitFields_60ABB0", 0x0060ABB0, &Track_InitFields_60ABB0);
ACTTRACKSFX_REVERSED("Track::InitFields_60ABD0", 0x0060ABD0, &Track_InitFields_60ABD0);
ACTTRACKSFX_REVERSED("CompletionDisplayTrack::InitFields", 0x0060ABF0, &CompletionDisplayTrack_InitFields);
ACTTRACKSFX_REVERSED("CompletionDisplayTrack::Start", 0x0060AC10, &CompletionDisplayTrack_Start);
ACTTRACKSFX_REVERSED("usePelvisForHealthMeterTrack::InitFields", 0x0060AC50, &HealthMeterTrack_InitFields);
ACTTRACKSFX_REVERSED("usePelvisForHealthMeterTrack::Update", 0x0060AC70, &HealthMeterTrack_Update);
ACTTRACKSFX_REVERSED("FallingDamageTrack::InitFields", 0x0060AC80, &FallingDamageTrack_InitFields);
ACTTRACKSFX_REVERSED("FallingDamageTrack::DeletingDestruct", 0x0060ACC0, &TrackDeletingDtor<kVtFallingDamageTrack>);
ACTTRACKSFX_REVERSED("FallingDamageHandleTrack::InitFields", 0x0060ACF0, &FallingDamageHandleTrack_InitFields);
ACTTRACKSFX_REVERSED("FallingDamageHandleTrack::DeletingDestruct", 0x0060AD00,
                     &TrackDeletingDtor<kVtFallingDamageHandleTrack>);
ACTTRACKSFX_REVERSED("AttackTimerResetTrack::DeletingDestruct", 0x0060AD30, &TrackDeletingDtor<kVtAttackTimerResetTrack>);
ACTTRACKSFX_REVERSED("ActionTracks::ListFirstOrNone_60AD60", 0x0060AD60, &List_FirstOrNone);
ACTTRACKSFX_REVERSED("DamageTrack::Resolve", 0x0060AD80, &OwnerPedOf<0x24>);
ACTTRACKSFX_REVERSED("FallingDamageTrack::Resolve", 0x0060AD90, &OwnerPedOf<0x2C>);
