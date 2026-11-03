// The action tree's weapon, prop-attach and camera tracks of 0x65D220..0x65E2B8: the
// weapon fire settings the next shot takes (the globals at 0x1B65C14..), fading, camera
// locks / pull-backs / safe zones, the first-person render list, the zoom; their field
// defaults (the New functions call these after construction) and destructors.

#include <cstdint>

#include "game/ActionFactories.h"
#include "game/ActionTracksCommon.h"
#include "game/TrackMath.h"

using namespace actc;
namespace af = actfac;

namespace {

GAME_FN(void, __thiscall, Camera_Fade, 0x004F1AD0, void* camera, float seconds, int direction, int unused);
GAME_FN(int, __cdecl, GetWeaponIndex, 0x0051AEA0, const char* name);
GAME_FN(void, __thiscall, FollowCameraBase_StartEffect2B0, 0x004FCF70, void* camera, float a, float b, int flag);
GAME_FN(void, __thiscall, FollowCameraBase_StartEffect2C0_x64, 0x004FCFC0, void* camera, float a, float b, int flag, re::Float3, float w, float h);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void FollowCameraBase_StartEffect2C0(void* a0, float a1, float a2, int a3, float a4, float a5, float a6, float a7, float a8) {
    return FollowCameraBase_StartEffect2C0_x64(a0, a1, a2, a3, re::Float3{a4, a5, a6}, a7, a8);
}
GAME_FN(void, __thiscall, Camera_ApplyRequest, 0x004F1890, void* camera, void* request);
GAME_FN(void, __cdecl, FirstPersonList_Add, 0x004D99E0, uint8_t* track);
GAME_FN(void, __cdecl, FirstPersonList_Remove, 0x004D9A00, uint8_t* track);
GAME_FN(void, __thiscall, Sound_Set, 0x0059D110, void* sound, uintptr_t id);
GAME_FN(void, __thiscall, Camera_SetFovTransition, 0x004F33A0, void* camera, float value, int duration, int mode,
        float speed, int flag);
GAME_FN(void, __thiscall, HUDPhoto_Close, 0x00543F00, void* photo);
GAME_FN(void, __thiscall, NameKey_FromString, 0x005FA890, uint32_t* key, const char* name);
GAME_FN(uint32_t*, __thiscall, NameKey_Copy, 0x005FA880, uint32_t* key, const uint32_t* from);
GAME_FN(void, __thiscall, HashPath_Set, 0x005FA8C0, int32_t* path, int32_t count, uint32_t* hashes);
GAME_FN(uint8_t*, __cdecl, FindBoneInfo, 0x0072AEA0, int32_t* path, void* names);
GAME_FN(void, __thiscall, WeaponInfo_Request, 0x007282A0, uint8_t* info);

// the next weapon shot's settings
GAME_VAR(uint8_t, frisbeeShot, 0x01B65C14);
GAME_ARRAY(float, frisbeeFlight, 0x01B65C18);        // 6: +0x1C, +0x18, +0x20, 2 angles (radians), 0
GAME_ARRAY(float, fireOffset, 0x01B65C30);           // 3
GAME_ARRAY(float, fireStartOffset, 0x01B65C3C);      // 3
GAME_ARRAY(float, fireRotation, 0x01B65C48);         // 3 (radians)
GAME_VAR(int32_t, fireAmmoWeapon, 0x00B31C9C);       // -1: the weapon's own
GAME_VAR(int32_t, cameraDesired_C3CED8, 0x00C3CED8);
GAME_VAR(int32_t, cameraLock_C3CE0C, 0x00C3CE0C);
GAME_VAR(void*, followCamera_C3CE98, 0x00C3CE98);
GAME_VAR(uint8_t*, hudWeapons, 0x00C674F0);
GAME_VAR(void*, hudPhoto, 0x00C674E0);
GAME_VAR(uint8_t*, zoomButes_20C88B8, 0x020C88B8);
GAME_VAR(int32_t, zoomStep_1B65C0C, 0x01B65C0C);
inline void* const theCamera = reinterpret_cast<void*>(0x00C3CC68);
inline const char* const noneName = reinterpret_cast<const char*>(0x0090A264);   // "NONE"

inline float DegToRad(float degrees) {
    return float(degrees * 3.14159274f) / 180.0f;
}
inline float DegToRadR(float degrees) {   // (pi first)
    return float(3.14159274f * degrees) / 180.0f;
}

// field defaults with name keys set to "" raw (the steps in order)
enum InitOp : uint8_t { kU8, kF32, kI32, kEmpty, kNone };
struct Init {
    InitOp op;
    uint16_t at;
    uint32_t value;
};
constexpr Init U8(uint16_t at, uint32_t v) { return {kU8, at, v}; }
constexpr Init F32(uint16_t at, float v) { return {kF32, at, std::bit_cast<uint32_t>(v)}; }
constexpr Init Empty(uint16_t at) { return {kEmpty, at, 0}; }   // "" constructed, copied raw
constexpr Init None(uint16_t at) { return {kNone, at, 0}; }     // cleared through a temporary
template <Init S>
inline void DoInit(uint8_t* self) {
    if constexpr (S.op == kU8) {
        self[S.at] = uint8_t(S.value);
    } else if constexpr (S.op == kF32) {
        At<float>(self, S.at) = std::bit_cast<float>(S.value);
    } else if constexpr (S.op == kI32) {
        At<uint32_t>(self, S.at) = S.value;
    } else if constexpr (S.op == kEmpty) {
        SetNameRaw(self, S.at, emptyString);
    } else {
        ClearName(self, S.at);
    }
}
template <Init... S>
void __fastcall InitFields(uint8_t* self, re::Edx) {
    (DoInit<S>(self), ...);
}

// 0x65D250 (thiscall): CameraFadeTrack::Start: the camera faded (+0x1C seconds; +0x18 in)
uint8_t __fastcall CameraFade_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    Camera_Fade(theCamera, At<float>(self, 0x1C), self[0x18] != 0 ? 1 : 0, 0);
    return 1;
}

// 0x65D2C0 (cdecl): the next shot's settings reset
void __cdecl WeaponFireSettings_Reset() {
    frisbeeShot = 0;
    fireAmmoWeapon = -1;
    for (int i = 3; i < 6; i++) {
        frisbeeFlight[i] = 0.0f;
    }
    for (int i = 0; i < 3; i++) {
        fireRotation[i] = 0.0f;
        fireOffset[i] = 0.0f;
        fireStartOffset[i] = 0.0f;
    }
}

// 0x65D380 (thiscall): WeaponFireFrisbeeTrack::Start: the next shot flies as a frisbee
uint8_t __fastcall WeaponFireFrisbee_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x2C) = owner;
    frisbeeShot = 1;
    frisbeeFlight[0] = At<float>(self, 0x1C);
    frisbeeFlight[1] = At<float>(self, 0x18);
    frisbeeFlight[2] = At<float>(self, 0x20);
    frisbeeFlight[3] = DegToRad(At<float>(self, 0x24));
    frisbeeFlight[4] = DegToRadR(At<float>(self, 0x28));
    frisbeeFlight[5] = 0.0f;
    return 1;
}

// 0x65D4B0 (thiscall): WeaponFireRotationTrack::Start: the next shot turned (degrees)
uint8_t __fastcall WeaponFireRotation_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    fireRotation[0] = DegToRad(At<float>(self, 0x18));
    fireRotation[1] = DegToRad(At<float>(self, 0x1C));
    fireRotation[2] = DegToRadR(At<float>(self, 0x20));
    return 1;
}

// 0x65D510 (thiscall): WeaponFireAmmoTrack::Start: the next shot's ammo (+0x18 named)
uint8_t __fastcall WeaponFireAmmo_Start(uint8_t* self, re::Edx, uint8_t*) {
    fireAmmoWeapon = GetWeaponIndex(At<const char*>(self, 0x18));
    return 1;
}

// 0x65D550 / 0x65D590 (thiscall): the next shot's offset / start offset
template <uintptr_t Target>
uint8_t __fastcall WeaponFireOffset_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    float* target = reinterpret_cast<float*>(Target);
    target[0] = At<float>(self, 0x18);
    target[1] = At<float>(self, 0x1C);
    target[2] = At<float>(self, 0x20);
    return 1;
}

// 0x65D660 (thiscall): SteerTrack::Start: the owner kept
uint8_t __fastcall Steer_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    return 1;
}

// 0x65DA20 (thiscall): PropScatterAttachmentTrack's butes (+0x38): by the name (+0x2C),
// else the weapon's (the owner's +0x264 weapon info's +0x80)
void __fastcall PropScatterAttachment_FindButes(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x38) = nullptr;
    if (At<uint32_t>(self, 0x2C) == 0) {
        uint8_t* info = At<uint8_t*>(owner, 0x264);
        WeaponInfo_Request(info);
        At<uint8_t*>(self, 0x38) = At<uint8_t*>(info, 0x80);
        return;
    }
    uint32_t keys[3];
    NameKey_Copy(&keys[0], reinterpret_cast<uint32_t*>(0x01B6531C));
    NameKey_Copy(&keys[1], reinterpret_cast<uint32_t*>(0x01B65394));
    NameKey_Copy(&keys[2], &At<uint32_t>(self, 0x2C));
    int32_t path[3];
    HashPath_Set(path, 3, keys);
    At<uint8_t*>(self, 0x38) = FindBoneInfo(path, reinterpret_cast<void*>(0x01B65398));
    for (int i = 2; i >= 0; i--) {
        NameKey_Destruct(&keys[i]);
    }
}

// 0x65DCA0 (thiscall): PropSpawnTrack's field defaults (its name "NONE")
void __fastcall PropSpawn_InitFields(uint8_t* self, re::Edx) {
    At<float>(self, 0xC) = -1.0f;
    self[8] = 0;
    NameKey_FromString(&At<uint32_t>(self, 0x18), noneName);
}

// 0x65DD10 (thiscall): CameraLockTrack::Start: the camera wanted cleared, locked (+0x18
// clear: unlocked)
uint8_t __fastcall CameraLock_Start(uint8_t* self, re::Edx, uint8_t*) {
    cameraDesired_C3CED8 = 0;
    cameraLock_C3CE0C = self[0x18] == 0 ? 1 : 0;
    return 1;
}

// 0x65DD50 (thiscall): CameraSetStartPointPullbackTrack::Start: the follow camera's pull
// back from its start point
uint8_t __fastcall CameraSetStartPointPullback_Start(uint8_t* self, re::Edx, uint8_t*) {
    FollowCameraBase_StartEffect2B0(followCamera_C3CE98, At<float>(self, 0x18), At<float>(self, 0x1C), self[0x20]);
    return 1;
}

// 0x65DDC0 (thiscall): CameraSetSafeZoneTrack::Start: the follow camera's safe zone (the
// point +0x24, its size +0x30 / +0x34 degrees)
uint8_t __fastcall CameraSetSafeZone_Start(uint8_t* self, re::Edx, uint8_t*) {
    float h = DegToRad(At<float>(self, 0x34));
    float w = DegToRadR(At<float>(self, 0x30));
    FollowCameraBase_StartEffect2C0(followCamera_C3CE98, At<float>(self, 0x18), At<float>(self, 0x1C), self[0x20],
                                    At<float>(self, 0x24), At<float>(self, 0x28), At<float>(self, 0x2C), w, h);
    return 1;
}

// 0x65DEA0 (thiscall): CameraSetDesiredActiveTrack::Start: the camera wanted applied
uint8_t __fastcall CameraSetDesiredActive_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    Camera_ApplyRequest(theCamera, &cameraDesired_C3CED8);
    return 1;
}

// 0x65DFF0 / 0x65E010 (thiscall): RenderFirstPersonTrack::Start / End: on the first-person
// render list
uint8_t __fastcall RenderFirstPerson_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    FirstPersonList_Add(self);
    return 1;
}
uint8_t __fastcall RenderFirstPerson_End(uint8_t* self, re::Edx) {
    FirstPersonList_Remove(self);
    return 1;
}

// 0x65E160 (thiscall): CameraZoomControlTrack's destructor (its sound let go)
void __fastcall CameraZoomControl_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = 0x00935F2C;
    Sound_Set(self + 0x4C, 0);
    TrackBase_Destruct(self);
}

// 0x65E180 (thiscall): CameraZoomControlTrack's zoom to the step: 0 back out, else the
// fov scaled by +0x1C to the step's power (over +0x18 seconds, +0x30 / +0x34)
void __fastcall CameraZoomControl_Zoom(uint8_t* self, re::Edx, int step) {
    float scale = At<float>(self, 0x1C);
    int32_t fov = At<int32_t>(zoomButes_20C88B8, 0x6C);
    zoomStep_1B65C0C = step;
    int ms = int(At<float>(self, 0x18) * 1000.0f);
    if (step == 0) {
        Camera_SetFovTransition(theCamera, 0.0f, ms, 2, At<float>(self, 0x30), At<int32_t>(self, 0x34));
        return;
    }
    float power = trk::CrtPow(scale, float(step));
    Camera_SetFovTransition(theCamera, power * float(fov), ms, 3, At<float>(self, 0x30), At<int32_t>(self, 0x34));
}

// 0x65E230 (thiscall): CameraZoomControlTrack::End: the fov back over a second, the HUD's
// zoom off (the photo mode's (+0x2C 2) closed)
uint8_t __fastcall CameraZoomControl_End(uint8_t* self, re::Edx) {
    Camera_SetFovTransition(theCamera, 0.0f, 1000, 0, 0.5f, 0);
    At<uint32_t>(hudWeapons, 0x138) = 0;
    if (At<int32_t>(self, 0x2C) == 2) {
        HUDPhoto_Close(hudPhoto);
    }
    return 1;
}

} // namespace

// (variadic: the template arguments' commas)
#define WPNTRK_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

// field defaults (thiscall, after construction)
WPNTRK_REVERSED("CameraFadeTrack::InitFields", 0x0065D220, &InitFields<F32(0x1C, 0.5f), U8(0x8, 1), U8(0x18, 1)>);
WPNTRK_REVERSED("WeaponFireFrisbeeTrack::InitFields", 0x0065D290, &InitFields<F32(0x18, 15.0f), F32(0x1C, -2.0f), F32(0x20, -1.0f), F32(0x24, 0.100000001f), F32(0x28, 0.200000003f)>);
WPNTRK_REVERSED("WeaponAnimationTrack::InitFields", 0x0065D3E0, &af::Fields<af::Call(0x00607660), af::U8(0x8, 0)>);
WPNTRK_REVERSED("WeaponFireTrack::InitFields", 0x0065D3F0, &InitFields<F32(0x18, 0.0f), F32(0x1C, 0.0f), F32(0x20, 0.0f), F32(0x24, 0.0f), U8(0x8, 0), F32(0x28, 1.0f), Empty(0x38), F32(0x34, -1.0f), F32(0x2C, -1.0f), U8(0x30, 1), U8(0x31, 0), U8(0x32, 1)>);
WPNTRK_REVERSED("WeaponFireSpecificOrientationTrack::InitFields", 0x0065D470, &af::Fields<af::Call(0x0065D3F0), af::F32(0x4C, 180.0f), af::U8(0x8, 0), af::F32(0x50, 30.0f), af::F32(0x54, 0.200000003f)>);
WPNTRK_REVERSED("WeaponFireRotationTrack::InitFields", 0x0065D530, &InitFields<U8(0x8, 0), F32(0x18, 0.0f), F32(0x1C, 0.0f), F32(0x20, 0.0f)>);  // also WeaponFireOffsetTrack
WPNTRK_REVERSED("WeaponImpulseTrack::InitFields", 0x0065D5C0, &InitFields<U8(0x8, 0), F32(0x18, 1.0f), F32(0x1C, 30.0f)>);  // also WeaponTargetImpulseTrack
WPNTRK_REVERSED("WeaponMinigameImpulseTrack::InitFields", 0x0065D5F0, &InitFields<U8(0x8, 0), F32(0x24, -1.0f), F32(0x28, 1.0f), F32(0x2C, 40.0f), F32(0x30, 10.0f), F32(0x34, 1.0f)>);
WPNTRK_REVERSED("ExtinguisherFireTrack::InitFields", 0x0065D620, &InitFields<F32(0x18, 5.0f), U8(0x8, 0), U8(0x20, 0), F32(0x1C, 20.0f)>);  // also PoisonSprayFireTrack
WPNTRK_REVERSED("WeaponFXTrack::InitFields", 0x0065D670, &InitFields<U8(0x8, 0), U8(0x18, 0), U8(0x1A, 0), None(0x20), Empty(0x24), U8(0x1B, 0), U8(0x1C, 0), U8(0x19, 0)>);
WPNTRK_REVERSED("AnimationDriverTargetSyncTrack::InitFields_65D710", 0x0065D710, &InitFields<U8(0x8, 0)>);  // also ClearStimulusTrack
WPNTRK_REVERSED("PropAttachTrack::InitFields", 0x0065D730, &InitFields<U8(0x8, 0), Empty(0x18), Empty(0x1C), U8(0x20, 0), U8(0x21, 0), U8(0x22, 1)>);
WPNTRK_REVERSED("PropAttachExTrack::InitFields", 0x0065D7C0, &InitFields<U8(0x8, 0), Empty(0x18), Empty(0x1C), Empty(0x20), U8(0x24, 0), U8(0x25, 0), U8(0x26, 0)>);
WPNTRK_REVERSED("PropAttachToPropTrack::InitFields", 0x0065D870, &InitFields<U8(0x8, 0), Empty(0x18), Empty(0x1C), Empty(0x20), Empty(0x24)>);
WPNTRK_REVERSED("PropRemoveAttachmentTrack::InitFields", 0x0065D940, &InitFields<U8(0x8, 0), Empty(0x18)>);  // also PropDetachExTrack
WPNTRK_REVERSED("PropScatterAttachmentTrack::InitFields", 0x0065D990, &InitFields<U8(0x8, 0), U8(0x18, 0), Empty(0x1C), F32(0x20, 0.0f), F32(0x24, 0.0f), F32(0x28, -1.0f), Empty(0x2C)>);
WPNTRK_REVERSED("PropSetSocketTrack::InitFields", 0x0065DAD0, &InitFields<U8(0x8, 0), Empty(0x18), Empty(0x1C), U8(0x20, 0)>);
WPNTRK_REVERSED("PropDetachTrack::InitFields", 0x0065DB50, &InitFields<U8(0x8, 0), Empty(0x18), U8(0x1C, 1)>);
WPNTRK_REVERSED("PropSetDesiredTrack::InitFields", 0x0065DBC0, &InitFields<F32(0xC, -1.0f), U8(0x8, 0), Empty(0x18), U8(0x1C, 0), U8(0x1E, 0), U8(0x1D, 0)>);
WPNTRK_REVERSED("PropSetInTransitionTrack::InitFields", 0x0065DC20, &InitFields<F32(0xC, -1.0f), U8(0x8, 0), Empty(0x18)>);
WPNTRK_REVERSED("PropGetDesiredTrack::InitFields", 0x0065DC80, &InitFields<U8(0x8, 0), F32(0xC, -1.0f)>);
WPNTRK_REVERSED("PropSpawnTrack::InitFields", 0x0065DCA0, &PropSpawn_InitFields);
WPNTRK_REVERSED("SetCameraTrack::InitFields", 0x0065DCF0, &InitFields<F32(0xC, 0.0f), U8(0x8, 0), F32(0x1C, 0.0f), Init{kI32, 0x18, 1}, U8(0x20, 1)>);
WPNTRK_REVERSED("CameraSetStartPointPullbackTrack::InitFields", 0x0065DD30, &InitFields<U8(0x8, 0), F32(0xC, 0.0f), U8(0x20, 1), F32(0x18, 1.0f), F32(0x1C, 0.5f)>);
WPNTRK_REVERSED("CameraSetSafeZoneTrack::InitFields", 0x0065DD80, &InitFields<U8(0x8, 0), F32(0xC, 0.0f), U8(0x20, 1), F32(0x18, 1.0f), F32(0x1C, 3.0f), F32(0x24, 0.0f), F32(0x2C, 0.0f), F32(0x28, 1.0f), F32(0x30, 30.0f), F32(0x34, 45.0f)>);
WPNTRK_REVERSED("CameraSetLimitTrack::InitFields", 0x0065DE50, &InitFields<F32(0xC, 0.0f), U8(0x8, 0), U8(0x34, 0), F32(0x18, -20.0f), U8(0x3C, 0), F32(0x20, -45.0f), F32(0x1C, 20.0f), F32(0x24, 45.0f), F32(0x28, 0.0f), F32(0x2C, 0.0f), F32(0x30, 0.0f)>);
WPNTRK_REVERSED("CameraSetDesiredActiveTrack::InitFields", 0x0065DE90, &InitFields<U8(0x8, 0), F32(0xC, 0.0f)>);  // also WeaponSpawnDamageEffectTrack
WPNTRK_REVERSED("UpdateFirstPersonTrack::InitFields", 0x0065DEC0, &InitFields<F32(0xC, -1.0f), F32(0x18, 0.0f), U8(0x8, 0), F32(0x1C, 0.0f), F32(0x20, 0.0f), F32(0x24, 1.0f), Empty(0x28), F32(0x30, 5.0f), F32(0x34, 3.0f), U8(0x2C, 1), F32(0x3C, -1.0f), U8(0x2D, 1), F32(0x40, 0.00400000019f), F32(0x44, 0.400000006f), None(0x48), None(0x4C), U8(0x50, 0)>);
WPNTRK_REVERSED("RenderFirstPersonTrack::InitFields", 0x0065DFB0, &InitFields<F32(0xC, -1.0f), U8(0x8, 0), F32(0x18, 0.200000003f), F32(0x1C, 175.0f), F32(0x20, 6.0f)>);
WPNTRK_REVERSED("SetWeaponInfoTrack::InitFields", 0x0065E020, &InitFields<F32(0xC, 0.0f), F32(0x10, -1.0f), U8(0x8, 0), F32(0x18, 160.0f), F32(0x1C, 1.0f), F32(0x20, 4.0999999f), F32(0x24, 1.0f), F32(0x28, 0.100000001f), F32(0x2C, 7.0f), F32(0x30, 180.0f), F32(0x34, 1.0f), F32(0x38, 5.0f), F32(0x3C, 2.0f), F32(0x40, 3.5f), None(0x44), U8(0x48, 0), U8(0x49, 0)>);
WPNTRK_REVERSED("SelectBestDirTossTargetTrack::InitFields", 0x0065E0E0, &InitFields<F32(0xC, 0.0f), U8(0x8, 0), U8(0x18, 0)>);
WPNTRK_REVERSED("CameraZoomControlTrack::InitFields", 0x0065E100, &InitFields<F32(0xC, 0.0f), U8(0x8, 0), U8(0x24, 0), F32(0x10, -1.0f), Init{kI32, 0x2C, 1}, F32(0x18, 1.0f), Init{kI32, 0x34, 0x80}, Init{kI32, 0x3C, 1000}, F32(0x1C, 2.0f), U8(0x40, 1), F32(0x20, 2.0f), F32(0x28, 0.100000001f), F32(0x30, 0.100000001f), F32(0x38, 0.0199999996f)>);  // +0x40 = 1: the game stores al after eax = 1 (+0x2C's), not the zero it cleared first
WPNTRK_REVERSED("CameraIrisControlTrack::InitFields", 0x0065E280, &InitFields<F32(0xC, 0.0f), U8(0x8, 0), U8(0x19, 1), F32(0x10, -1.0f), U8(0x18, 1), F32(0x1C, 0.0f), F32(0x20, 1.0f), F32(0x24, 0.00999999978f)>);

// destructors (thiscall)
WPNTRK_REVERSED("CameraFadeTrack::Destruct", 0x0065D240, &af::Fields<af::Vt(0x00935F5C), af::TrackBaseDtor>);
WPNTRK_REVERSED("WeaponFireTrack::Destruct", 0x0065D450, &af::Fields<af::Vt(0x00935B94), af::KeyDtor(0x38), af::TrackBaseDtor>);  // also WeaponFireSpecificOrientationTrack's
WPNTRK_REVERSED("WeaponFireRotationTrack::Destruct", 0x0065D4A0, &af::Fields<af::Vt(0x00935CA4), af::TrackBaseDtor>);
WPNTRK_REVERSED("WeaponFireAmmoTrack::Destruct", 0x0065D4F0, &af::Fields<af::Vt(0x009360AC), af::KeyDtor(0x18), af::TrackBaseDtor>);
WPNTRK_REVERSED("WeaponFireOffsetTrack::Destruct", 0x0065D540, &af::Fields<af::Vt(0x00935CBC), af::TrackBaseDtor>);
WPNTRK_REVERSED("WeaponFireStartOffsetTrack::Destruct", 0x0065D580, &af::Fields<af::Vt(0x00935CD4), af::TrackBaseDtor>);
WPNTRK_REVERSED("WeaponImpulseTrack::Destruct", 0x0065D5E0, &af::Fields<af::Vt(0x00935BCC), af::TrackBaseDtor>);
WPNTRK_REVERSED("ExtinguisherFireTrack::Destruct", 0x0065D640, &af::Fields<af::Vt(0x00935C14), af::TrackBaseDtor>);
WPNTRK_REVERSED("PoisonSprayFireTrack::Destruct", 0x0065D650, &af::Fields<af::Vt(0x00935C2C), af::TrackBaseDtor>);
WPNTRK_REVERSED("WeaponFXTrack::Destruct", 0x0065D6D0, &af::Fields<af::Vt(0x00935C44), af::KeyDtor(0x24), af::KeyDtor(0x20), af::TrackBaseDtor>);
WPNTRK_REVERSED("ProjectilePickupTrack::Destruct", 0x0065D700, &af::Fields<af::Vt(0x00935D34), af::TrackBaseDtor>);
WPNTRK_REVERSED("PropPickupTrack::Destruct", 0x0065D720, &af::Fields<af::Vt(0x00935D94), af::TrackBaseDtor>);
WPNTRK_REVERSED("PropAttachTrack::Destruct", 0x0065D790, &af::Fields<af::Vt(0x00935DAC), af::KeyDtor(0x1C), af::KeyDtor(0x18), af::TrackBaseDtor>);
WPNTRK_REVERSED("PropAttachExTrack::Destruct", 0x0065D840, &af::Fields<af::Vt(0x00935DF4), af::KeyDtor(0x20), af::KeyDtor(0x1C), af::KeyDtor(0x18), af::TrackBaseDtor>);
WPNTRK_REVERSED("PropAttachToPropTrack::Destruct", 0x0065D900, &af::Fields<af::Vt(0x00935E24), af::KeyDtor(0x24), af::KeyDtor(0x20), af::KeyDtor(0x1C), af::KeyDtor(0x18), af::TrackBaseDtor>);
WPNTRK_REVERSED("PropRemoveAttachmentTrack::Destruct", 0x0065D970, &af::Fields<af::Vt(0x00935E3C), af::KeyDtor(0x18), af::TrackBaseDtor>);
WPNTRK_REVERSED("PropScatterAttachmentTrack::Destruct", 0x0065D9F0, &af::Fields<af::Vt(0x00935E54), af::KeyDtor(0x2C), af::KeyDtor(0x1C), af::TrackBaseDtor>);
WPNTRK_REVERSED("PropSetSocketTrack::Destruct", 0x0065DB20, &af::Fields<af::Vt(0x00935DC4), af::KeyDtor(0x1C), af::KeyDtor(0x18), af::TrackBaseDtor>);
WPNTRK_REVERSED("PropDetachTrack::Destruct", 0x0065DB80, &af::Fields<af::Vt(0x00935DDC), af::KeyDtor(0x18), af::TrackBaseDtor>);
WPNTRK_REVERSED("PropDetachExTrack::Destruct", 0x0065DBA0, &af::Fields<af::Vt(0x00935E0C), af::KeyDtor(0x18), af::TrackBaseDtor>);
WPNTRK_REVERSED("PropSetDesiredTrack::Destruct", 0x0065DC00, &af::Fields<af::Vt(0x00935E84), af::KeyDtor(0x18), af::TrackBaseDtor>);
WPNTRK_REVERSED("PropSetInTransitionTrack::Destruct", 0x0065DC60, &af::Fields<af::Vt(0x00935378), af::KeyDtor(0x18), af::TrackBaseDtor>);
WPNTRK_REVERSED("PropGetDesiredTrack::Destruct", 0x0065DC90, &af::Fields<af::Vt(0x00935E6C), af::TrackBaseDtor>);
WPNTRK_REVERSED("PropSpawnTrack::Destruct", 0x0065DCC0, &af::Fields<af::Vt(0x00935E9C), af::KeyDtor(0x1C), af::KeyDtor(0x18), af::TrackBaseDtor>);
WPNTRK_REVERSED("UpdateFirstPersonTrack::Destruct", 0x0065DF80, &af::Fields<af::Vt(0x00935EB4), af::KeyDtor(0x4C), af::KeyDtor(0x48), af::KeyDtor(0x28), af::TrackBaseDtor>);
WPNTRK_REVERSED("RenderFirstPersonTrack::Destruct", 0x0065DFE0, &af::Fields<af::Vt(0x00934440), af::TrackBaseDtor>);
WPNTRK_REVERSED("SetWeaponInfoTrack::Destruct", 0x0065E0C0, &af::Fields<af::Vt(0x00935D04), af::KeyDtor(0x44), af::TrackBaseDtor>);
WPNTRK_REVERSED("SelectBestDirTossTargetTrack::Destruct", 0x0065E0F0, &af::Fields<af::Vt(0x00936358), af::TrackBaseDtor>);
WPNTRK_REVERSED("CameraZoomControlTrack::Destruct", 0x0065E160, &CameraZoomControl_Destruct);
WPNTRK_REVERSED("CameraIrisControlTrack::Destruct", 0x0065E2B0, &af::Fields<af::Vt(0x00935ECC), af::TrackBaseDtor>);

// the tracks' virtuals and helpers
WPNTRK_REVERSED("CameraFadeTrack::Start", 0x0065D250, &CameraFade_Start);
WPNTRK_REVERSED("WeaponFireSettings::Reset_65D2C0", 0x0065D2C0, &WeaponFireSettings_Reset);
WPNTRK_REVERSED("WeaponFireFrisbeeTrack::Start", 0x0065D380, &WeaponFireFrisbee_Start);
WPNTRK_REVERSED("WeaponFireRotationTrack::Start", 0x0065D4B0, &WeaponFireRotation_Start);
WPNTRK_REVERSED("WeaponFireAmmoTrack::Start", 0x0065D510, &WeaponFireAmmo_Start);
WPNTRK_REVERSED("WeaponFireOffsetTrack::Start", 0x0065D550, &WeaponFireOffset_Start<0x01B65C30>);
WPNTRK_REVERSED("WeaponFireStartOffsetTrack::Start", 0x0065D590, &WeaponFireOffset_Start<0x01B65C3C>);
WPNTRK_REVERSED("SteerTrack::Start", 0x0065D660, &Steer_Start);
WPNTRK_REVERSED("PropScatterAttachmentTrack::FindButes", 0x0065DA20, &PropScatterAttachment_FindButes);
WPNTRK_REVERSED("CameraLockTrack::Start", 0x0065DD10, &CameraLock_Start);
WPNTRK_REVERSED("CameraSetStartPointPullbackTrack::Start", 0x0065DD50, &CameraSetStartPointPullback_Start);
WPNTRK_REVERSED("CameraSetSafeZoneTrack::Start", 0x0065DDC0, &CameraSetSafeZone_Start);
WPNTRK_REVERSED("CameraSetDesiredActiveTrack::Start", 0x0065DEA0, &CameraSetDesiredActive_Start);
WPNTRK_REVERSED("RenderFirstPersonTrack::Start", 0x0065DFF0, &RenderFirstPerson_Start);
WPNTRK_REVERSED("RenderFirstPersonTrack::End", 0x0065E010, &RenderFirstPerson_End);
WPNTRK_REVERSED("CameraZoomControlTrack::Zoom", 0x0065E180, &CameraZoomControl_Zoom);
WPNTRK_REVERSED("CameraZoomControlTrack::End", 0x0065E230, &CameraZoomControl_End);
