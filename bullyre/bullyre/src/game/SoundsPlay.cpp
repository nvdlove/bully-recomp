// The audio manager's players (TheSounds, 0xCF3DA0): a sound at a point (its volume from
// the distance, its pan from the camera's axes), the material hits, the street light, the
// frontend's sounds. 0x59B540..0x59C150.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

namespace {

using re::At;

struct CVector {
    float x, y, z;
};

// what the SFX manager's play takes (0x24 bytes)
struct SoundParams {
    int32_t volume;       // +0
    int32_t unknown4;     // +4 (the game's uninitialised stack: zero here)
    CVector pos;          // +8
    uint8_t unknown14;    // +0x14 (set to a12 when that's 0, else uninitialised)
    uint8_t pad15[3];
    int32_t unknown18;    // +0x18 (uninitialised)
    int32_t pan;          // +0x1C (0x59B740's angle; an argument for the 2D ones)
    uint32_t flags;       // +0x20 1 no volume, 2, 4 a12 not set, 8 the volume from the distance, 0x10
};
static_assert(sizeof(SoundParams) == 0x24);

inline uint8_t* const theSounds = reinterpret_cast<uint8_t*>(0x00CF3DA0);
inline void* const speech = reinterpret_cast<void*>(0x00CF4378);
GAME_VAR(void*, sfxManager, 0x00CF68CC);
GAME_VAR(void*, theCamera, 0x00C3CCF4);

GAME_FN(CVector*, __cdecl, GetCameraPos, 0x00595630, CVector* out);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const CVector* v);
GAME_FN(float, __cdecl, SafeAcos, 0x00413D60, float x);
GAME_FN(void*, __cdecl, Camera_Object, 0x006C82F0, void* camera);
GAME_FN(float*, __cdecl, Node_WorldTransform, 0x006C9A90, void* node);   // rotation[9], translation
GAME_FN(int, __cdecl, Sfx_VolumeFor, 0x0059E3E0, float distance, int);
GAME_FN(void*, __thiscall, Sfx_Play, 0x0059E060, void* sfx, int id, int volume, int at, int a4, int a5,
        const CVector* pos2, int a7, int a8, int a9, int a10, void* callback, void* entity, int a13, int a14);
GAME_FN(void*, __thiscall, Sfx_Play2, 0x0059E0F0, void* sfx, int id, SoundParams* params, int a3, int a4,
        const CVector* pos2, int a6, int a7, int a8, int a9, void* callback, void* entity, int a12, int a13);
GAME_FN(void, __thiscall, Sound_Set, 0x0059D110, void* soundRef, void* sound);
GAME_FN(uint8_t, __thiscall, Sound_IsActive, 0x0059CF10, void* sound);
GAME_FN(void, __thiscall, Speech_Fn5A4C80, 0x005A4C80, void* speech);
// its own
GAME_FN(int, __thiscall, Sounds_GetId, 0x00595B80, void* sounds, const char* name);
GAME_FN(void, __thiscall, Sounds_DebugName, 0x005952D0, void* sounds, uintptr_t name);
GAME_FN(int32_t, __thiscall, Sounds_Angle, 0x0059B5D0, void* sounds, const CVector* pos, const CVector* listener,
        const CVector* forward, const CVector* right);
GAME_FN(int32_t, __thiscall, Sounds_Pan, 0x0059B740, void* sounds, const CVector* pos, const CVector* listener);
GAME_FN(uint8_t*, __thiscall, Sounds_PlayAt, 0x0059B8A0, void* sounds, int id, const CVector* pos, int a3,
        const CVector* pos2, int a5, int a6, int a7, int a8, void* callback, void* entity, int a11, int a12,
        float a13, int a14);

// ---- the listener's geometry ------------------------------------------------------------------

// 0x59B540 (thiscall, ecx unused): the squared distance from the listener (the height
// counting a fifth)
float __fastcall Sounds_DistanceSq(void*, re::Edx, const CVector* pos) {
    CVector listener;
    GetCameraPos(&listener);
    CVector d;
    VectorSub(&d, pos, &listener);
    float z = d.z * 0.2f;
    float xx = d.x * d.x;
    float yy = d.y * d.y;
    float zz = z * z;
    return (xx + yy) + zz;
}

// 0x59B5D0 (thiscall, ecx unused): the angle (0..359 degrees) of pos around the listener
// from `forward` towards `right`, on the ground plane
int32_t __fastcall Sounds_AngleImpl(void*, re::Edx, const CVector* pos, const CVector* listener, const CVector* forward,
                                   const CVector* right) {
    CVector v;
    VectorSub(&v, pos, listener);
    v.z = 0.0f;
    CVector f = *forward;
    CVector r = *right;
    Vector_Normalise(&v);
    f.z = 0.0f;
    Vector_Normalise(&f);
    r.z = 0.0f;
    Vector_Normalise(&r);
    float dot = v.x * f.x + f.y * v.y + f.z * v.z;
    float angle;
    if (dot >= 1.0f) {
        angle = 0.0f;
    } else if (-1.0f >= dot) {
        angle = 3.14159274f;
    } else {
        angle = SafeAcos(dot);
    }
    float degrees = angle * 180.0f / 3.14159274f;
    float side = r.x * v.x + r.y * v.y + r.z * v.z;
    if (side < 0.0f) {
        degrees = 360.0f - degrees;
    }
    if (degrees == 360.0f) {
        return 0;
    }
    return int32_t(degrees);
}

// 0x59B740 (thiscall): the pan of a sound at pos (the listener: the camera when null)
int32_t __fastcall Sounds_PanImpl(uint8_t* self, re::Edx, const CVector* pos, const CVector* listener) {
    CVector at{0.0f, 0.0f, 0.0f};
    if (listener == nullptr) {
        listener = reinterpret_cast<const CVector*>(Node_WorldTransform(Camera_Object(theCamera)) + 9);
    }
    at = *listener;
    const float* m = Node_WorldTransform(Camera_Object(theCamera));
    CVector forward{m[2], m[5], m[8]};
    m = Node_WorldTransform(Camera_Object(theCamera));
    CVector right{m[0], m[3], m[6]};
    return Sounds_Angle(self, pos, &at, &forward, &right);
}

// 0x59B860 (thiscall): the frontend's sound (+0x2B28) stopped
void __fastcall Sounds_StopFrontend(uint8_t* self) {
    re::Ptr32<uint8_t>* ref = &At<uint8_t*>(self, 0x2B28);
    if (*ref != nullptr && (Sound_IsActive(*ref) != 0 || vc::Call<uint8_t>(*ref, 0x50) != 0)) {
        (*ref)[0x5C] = 1;
    }
    Sound_Set(ref, nullptr);
}

// ---- playing -----------------------------------------------------------------------------------

// 0x59B8A0 (thiscall): a sound at a point (null: without a position); beyond 90 m none.
// (a6..a8 of 0x7D, 0x7E, 0x7F: without the distance's volume and panned)
uint8_t* __fastcall Sounds_PlayAtImpl(uint8_t* self, re::Edx, int id, const CVector* pos, int a3, const CVector* pos2,
                                      int a5, int a6, int a7, int a8, void* callback, void* entity, int a11, int a12,
                                      float a13, int a14) {
    if (id == Sounds_GetId(self, "BRLHIT01")) {
        return nullptr;
    }
    if (id == -1) {
        Sounds_DebugName(self, id);
        return nullptr;
    }
    if (pos == nullptr) {
        return static_cast<uint8_t*>(Sfx_Play(sfxManager, id, 0x400, -1, a14, 0, pos2, a5, a6, a7, a8, callback, entity,
                                              a3, a11));
    }
    SoundParams params{};
    params.pos = {0.0f, 0.0f, 0.0f};
    params.flags = 0;
    CVector listener;
    GetCameraPos(&listener);
    if (uint8_t(a8) == 0x7F && uint8_t(a7) == 0x7E && uint8_t(a6) == 0x7D) {
        params.flags |= 1;
        params.volume = 0;
        a6 &= ~0xFF;
        a7 &= ~0xFF;
        a8 &= ~0xFF;
    } else {
        CVector d;
        VectorSub(&d, pos, &listener);
        float distance = Vector_Magnitude(&d);
        if (!(distance <= 90.0f)) {
            return nullptr;
        }
        int v = Sfx_VolumeFor(distance, a3);
        params.volume = int32_t(float(uint32_t(v)) * a13);
        params.flags |= 9;
    }
    uint8_t keep = uint8_t(a12);
    if (keep == 0) {
        params.unknown14 = keep;
    }
    params.pan = Sounds_Pan(theSounds, pos, nullptr);
    params.pos = *pos;
    params.flags |= (keep != 0 ? 0 : 4) | 0x12;
    uint8_t* sound = static_cast<uint8_t*>(Sfx_Play2(sfxManager, id, &params, a14, 0, pos2, a5, a6, a7, a8, callback,
                                                     entity, a3, a11));
    if (sound != nullptr && keep == 0) {
        vc::Call<void>(sound, 0x98, 0);
    }
    return sound;
}

// 0x59BAB0 (thiscall, ecx unused): a 2D sound (a volume and a pan)
uint8_t* __fastcall Sounds_Play2D(void*, re::Edx, int id, int volume, int pan, int a4) {
    SoundParams params{};
    params.pos = {0.0f, 0.0f, 0.0f};
    params.volume = volume;
    params.pan = pan;
    params.unknown14 = 0;
    params.flags = 0x15;
    uint8_t* sound =
        static_cast<uint8_t*>(Sfx_Play2(sfxManager, id, &params, 0, a4, nullptr, 0, 0, 0, 0, nullptr, nullptr, 1, 6));
    vc::Call<void>(sound, 0x94, 1);
    return sound;
}

// 0x59BB30 (thiscall): a street light broken (its hack sound, then the break at the point)
void __fastcall Sounds_StreetLightBreak(uint8_t* self, re::Edx, CVector pos) {
    int hack = Sounds_GetId(theSounds, "STREETLITE_HACK");
    if (hack != Sounds_GetId(self, "BRLHIT01")) {
        if (hack == -1) {
            Sounds_DebugName(self, hack);
        } else {
            Sfx_Play(sfxManager, hack, 0x400, -1, 0, 0, nullptr, 0, 0x78, 0, 0, nullptr, nullptr, 1, 6);
        }
    }
    Sounds_PlayAt(self, Sounds_GetId(theSounds, "STREETLITE_BRK"), &pos, 0, nullptr, 0, 0x78, 0, 0, nullptr,
                  nullptr, 6, 1, 1.0f, 0);
}

// 0x59BBD0 (thiscall): a hit on a surface (its material's sound set, panned between the
// speakers by the distance up to 70 m; `strength` 2..4 quieter, 5 on silent)
uint8_t* __fastcall Sounds_PlayHit(uint8_t* self, re::Edx, int id, const CVector* pos, int material, int a3,
                                   int strength) {
    static const uint8_t kMaterialSets[0x42] = {
        0x14, 0x48, 0x46, 0x47, 0x49, 0x1C, 0x15, 0x17, 0x16, 0x18, 0x19, 0x1A, 0x1B, 0x23, 0x0D, 0x3E,
        0x00, 0x00, 0x1E, 0x22, 0x26, 0x27, 0x00, 0x24, 0x20, 0x01, 0x4A, 0x28, 0x29, 0x2A, 0x2B, 0x03,
        0x00, 0x02, 0x04, 0x05, 0x06, 0x35, 0x33, 0x34, 0x07, 0x08, 0x4B, 0x4C, 0x50, 0x1F, 0x00, 0x00,
        0x0A, 0x0B, 0x0C, 0x00, 0x25, 0x00, 0x00, 0x21, 0x3C, 0x3D, 0x09, 0x3F, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x51,
    };
    int set = uint32_t(material - 1) <= 0x41 ? kMaterialSets[material - 1] : 0;
    CVector listener;
    CVector d;
    VectorSub(&d, pos, GetCameraPos(&listener));
    float distance = std::sqrt(std::fabs(d.x * d.x + d.y * d.y + d.z * d.z));
    if (70.0f < distance) {
        distance = 70.0f;
    }
    float t = distance / 70.0f;
    int32_t left;
    int32_t right;
    if (t <= 0.2f) {
        right = 0x7F;
        left = int32_t(t / 0.8f * 127.0f);
    } else if (t >= 0.8f) {
        right = 0;
        left = 0x7F;
    } else {
        left = int32_t(t / 0.8f * 127.0f);
        float u = (t - 0.2f) / 0.6f;
        right = 0x7F - int32_t(127.0f * u);
    }
    float volume = 1.0f;
    if (strength >= 5) {
        volume = 0.0f;
    } else if (strength > 1) {
        volume = float(0x8C >> (strength - 1)) / 100.0f;
    }
    uint8_t* sound = Sounds_PlayAt(self, id, pos, a3, nullptr, set, set, right, left, nullptr, nullptr, 6, 0, volume, 0);
    if (sound != nullptr) {
        vc::Call<void>(sound, 0x94, 1);
    }
    return sound;
}

// 0x59C0A0 (thiscall): the frontend's sound (+0x2B28) replaced
void __fastcall Sounds_PlayFrontend(uint8_t* self, re::Edx, int id) {
    if (id != Sounds_GetId(self, "BRLHIT01")) {
        if (id != -1) {
            Sound_Set(self + 0x2B28,
                      Sfx_Play(sfxManager, id, 0x400, -1, 0, 0, nullptr, 0, 0x78, 0, 0, nullptr, nullptr, 1, 6));
            return;
        }
        Sounds_DebugName(self, id);
    }
    Sound_Set(self + 0x2B28, nullptr);
}

// 0x59C110 (thiscall, ecx unused): a sound at a point with the usual arguments
void __fastcall Sounds_PlayAtSimple(void*, re::Edx, int id, int, const CVector* pos, int a3) {
    Sounds_PlayAt(theSounds, id, pos, a3, nullptr, 0, 0x78, 0, 0, nullptr, nullptr, 6, 1, 1.0f, 0);
}

// 0x59C150 (thiscall): the frontend's and pickups' sounds by kind
void __fastcall Sounds_PlayUiSound(uint8_t* self, re::Edx, uint32_t kind) {
    struct UiSound {
        const char* name;
        int a11;
        int a6;
    };
    static const UiSound kSounds[0x23] = {
        {"ButtonUp", 6, 0x78},       {"ButtonDown", 6, 0x78},    {"EndOfLine", 6, 0x78},
        {"Generic_Pickup", 6, 0x78}, {"Health_Pickup", 6, 0x78}, {"Generic_Pickup", 6, 0x78},
        {"WeaponUp", 6, 0x78},       {"Cash_Pickup", 6, 0x78},   {"Generic_Pickup", 6, 0x78},
        {"Generic_Pickup", 6, 0x78}, {"Gift_Pickup", 6, 0x78},   {"Health_Pickup", 6, 0x78},
        {"NavUp", 6, 0x78},          {"NavDwn", 6, 0x78},        {"NavUpXX", 6, 0x78},
        {"NavDwnXX", 6, 0x78},       {"NavUpXX", 2, 0},          {"NavDwnXX", 2, 0},
        {"NavUpXX", 1, 0},           {"NavDwnXX", 1, 0},         {"NavInvalid", 6, 0x78},
        {"WeaponArm", 6, 0x78},      {"WeaponUp", 6, 0x78},      {"WeaponDown", 6, 0x78},
        {"BuyItem", 6, 0x78},        {"CantBuyItem", 6, 0x78},   {"Razor", 6, 0x78},
        {"Scissor", 6, 0x78},        {"Generic_Pickup", 6, 0x78}, {"Tutorial", 6, 0x78},
        {"RollOver", 6, 0x78},       {"Select", 6, 0x78},        {"Back", 6, 0x78},
        {"Trophy", 6, 0x78},         {"Quit", 6, 0x78},
    };
    if (kind > 0x22) {
        return;
    }
    if (kind == 3 || kind == 5 || kind == 8 || kind == 9) {
        Speech_Fn5A4C80(speech);   // (the pickups but the kind 28 one)
    }
    const UiSound& s = kSounds[kind];
    Sounds_PlayAt(self, Sounds_GetId(theSounds, s.name), nullptr, 1, nullptr, 0, s.a6, 0, 0, nullptr, nullptr, s.a11,
                  1, 1.0f, 0);
}

// ---- registration -----------------------------------------------------------------------------

#define SOUNDSPLAY_REVERSED(name, address, fn)                                                          \
    static void* RE_CONCAT(soundsplay_original_, __LINE__);                                              \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(soundsplay_original_, __LINE__), \
             re::Mode::Original)

SOUNDSPLAY_REVERSED("TheSounds::DistanceSq", 0x0059B540, &Sounds_DistanceSq);
SOUNDSPLAY_REVERSED("TheSounds::Angle", 0x0059B5D0, &Sounds_AngleImpl);
SOUNDSPLAY_REVERSED("TheSounds::Pan", 0x0059B740, &Sounds_PanImpl);
SOUNDSPLAY_REVERSED("TheSounds::StopFrontend", 0x0059B860, &Sounds_StopFrontend);
SOUNDSPLAY_REVERSED("TheSounds::PlayAt", 0x0059B8A0, &Sounds_PlayAtImpl);
SOUNDSPLAY_REVERSED("TheSounds::Play2D", 0x0059BAB0, &Sounds_Play2D);
SOUNDSPLAY_REVERSED("TheSounds::StreetLightBreak", 0x0059BB30, &Sounds_StreetLightBreak);
SOUNDSPLAY_REVERSED("TheSounds::PlayHit", 0x0059BBD0, &Sounds_PlayHit);
SOUNDSPLAY_REVERSED("TheSounds::PlayFrontend", 0x0059C0A0, &Sounds_PlayFrontend);
SOUNDSPLAY_REVERSED("TheSounds::PlayAtSimple", 0x0059C110, &Sounds_PlayAtSimple);
SOUNDSPLAY_REVERSED("TheSounds::PlayUiSound", 0x0059C150, &Sounds_PlayUiSound);

} // namespace
