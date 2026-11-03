// CWeather (0x535FD0..0x5368F7): the weather types (0 fine, 1 cloudy, 2 rain, 3 foggy, 4
// overcast?, 5 snow), the change every 1800 frames (a small chance each frame once the
// interpolation is done; 2 and 5 always change back to 1), lightning and thunder in a
// storm, the rain / wetness / cloud / fog amounts, the ambience sound, and the rain's
// splashes near the camera (0x536130) when it's heavy.
//
// Found here: 0xCE7570 is the new type, 0xCE7574 the old, 0xCE7578 the forced one (-1:
// none), 0xCE756C the one in effect (old until the interpolation is half done); the
// seasons by chapter (0xA147EC) make it snow (2) or not.

#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace weather17 {

GAME_VAR(int16_t, NewWeatherType, 0x00CE7570);
GAME_VAR(int16_t, OldWeatherType, 0x00CE7574);
GAME_VAR(int16_t, ForcedWeatherType, 0x00CE7578);
GAME_VAR(int16_t, WeatherType, 0x00CE756C);
GAME_VAR(int32_t, InterpolationCounter, 0x00CE7540);
GAME_VAR(float, InterpolationValue, 0x00CE7568);
GAME_VAR(float, Rain, 0x00CE7558);
GAME_VAR(float, WetRoads, 0x00CE7564);
GAME_VAR(float, CloudCoverage, 0x00CE7560);
GAME_VAR(float, Foggyness, 0x00CE755C);
GAME_VAR(float, Overcast, 0x00CE7550);
GAME_VAR(float, SplashAmount, 0x00CE7554);
GAME_VAR(uint8_t, ThunderstormOn, 0x00CE754F);
GAME_VAR(uint8_t, ForceSnow, 0x00CE754E);
GAME_VAR(uint8_t, LightningActive, 0x00CE754C);
GAME_VAR(uint8_t, LightningFlash, 0x00CE754D);
GAME_VAR(uint32_t, LightningFlashTime, 0x00CE7544);
GAME_VAR(uint32_t, ThunderTime, 0x00CE7548);
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(uint8_t, chapter, 0x00A147E0);
GAME_ARRAY(int32_t, chapterSeason, 0x00A147EC);
GAME_ARRAY(float, WeatherSplashes, 0x00AA4F2C);    // by type
GAME_VAR(float, overcastScale, 0x020C488C);
GAME_VAR(uint8_t, noRainSplashes, 0x00C0F6BC);
inline void* const ambience = reinterpret_cast<void*>(0x00C0F6B0);
inline void* const randomGenerator = reinterpret_cast<void*>(0x00CF356C);
inline void* const sounds = reinterpret_cast<void*>(0x00CF3DA0);

GAME_FN(void, __thiscall, Ambience_SetWeather, 0x0044DA40, void* ambience, int kind, int snow);
GAME_FN(void, __thiscall, Ambience_SetRain, 0x0044DA90, void* ambience, float rain);
GAME_FN(uint8_t, __cdecl, Area_430320, 0x00430320, int area);   // (true outside)
GAME_FN(float, __thiscall, Random_GetFloat, 0x00577300, void* generator);
GAME_FN(float, __cdecl, RandomRange, 0x0041A5B0, float lo, float hi);
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(int, __thiscall, Sounds_GetId, 0x00595B80, void* sounds, const char* name);
GAME_FN(void*, __thiscall, Sounds_Play, 0x0059B8A0, void* sounds, int id, uintptr_t, int, uintptr_t, int, int, int, int, uintptr_t, uintptr_t, int, int, float,
        int);
GAME_FN(uint8_t, __cdecl, CamNoRain, 0x00538C70);
GAME_FN(uint8_t, __cdecl, PlayerNoRain, 0x00538C80);
GAME_FN(void, __cdecl, UpdateAmbience_Game, 0x00535FD0);
GAME_FN(void, __cdecl, AddRainSplashes, 0x00536130);
GAME_VAR(void*, theCameraNode, 0x00C3CCF4);
GAME_VAR(float, splashHeading, 0x00C3CD08);
GAME_FN(void*, __cdecl, W_GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(float*, __cdecl, W_GetLocalTransform, 0x006C9A90, void* node);   // (+0x24: the translation)
GAME_FN(float, __cdecl, W_FindGroundZFor3DCoord, 0x004635A0, float x, float y, float z, uint8_t* found, int flags);
GAME_FN(void, __cdecl, W_AddParticle, 0x00521080, int kind, const void* pos, const void* velocity, float size,
        const uint8_t* colour, int a6, int a7);

// 0x535FD0: the ambience sound for the weather (snow seasons have no rain)
void __cdecl UpdateAmbience() {
    int season = chapterSeason[chapter];
    uint8_t force = ForceSnow;
    int kind = 0;
    if (season == 2 || force == 1) {
        WetRoads = 0.0f;
        Rain = 0.0f;
        if (WeatherType == 2) {
            Ambience_SetWeather(ambience, 1, force == 1);
            return;
        }
        if (WeatherType == 5) {
            kind = 4;
        }
    } else if (!(static_cast<double>(Rain) <= static_cast<double>(0.1f))) {
        if (WeatherType == 5) {
            Ambience_SetWeather(ambience, 3, force == 1);
            return;
        }
        if (WeatherType == 2) {
            Ambience_SetWeather(ambience, 2, force == 1);
            return;
        }
    }
    Ambience_SetWeather(ambience, kind, force == 1);
}

// 0x5360A0: CWeather::Init
void __cdecl Init() {
    InterpolationValue = 0.0f;
    NewWeatherType = 0;
    OldWeatherType = 0;
    WeatherType = 0;
    ForcedWeatherType = -1;
    ThunderTime = 0;
    InterpolationCounter = 1800;
    ForceSnow = 0;
    ThunderstormOn = 0;
}

// 0x5360E0: CWeather::SetWeather (a storm when `thunder` for rain; not raining otherwise
// unless snow)
void __cdecl SetWeather(int16_t type, uint8_t thunder) {
    if (type == 2) {
        ThunderstormOn = thunder;
    } else {
        ThunderstormOn = 0;
        if (type != 5) {
            Rain = 0.0f;
        }
    }
    ForcedWeatherType = type;
    OldWeatherType = type;
    NewWeatherType = type;
}

// 0x536320: CWeather::Update
void __cdecl Update() {
    if (!Area_430320(currArea)) {
        return;
    }
    int counter = InterpolationCounter;
    InterpolationCounter = counter - 1;
    float interp = 1.0f - float(counter) / 1800.0f;
    int16_t next;
    bool change;
    if (InterpolationCounter < 0) {
        InterpolationCounter = 0;
        change = true;
    } else {
        change = InterpolationCounter <= 0;
    }
    next = NewWeatherType;
    if (change) {
        OldWeatherType = NewWeatherType;
        if ((NewWeatherType == 2 || NewWeatherType == 5) && ForcedWeatherType < 0) {
            interp = 0.0f;
            NewWeatherType = 1;
            InterpolationCounter = 1800;
            next = 1;
        } else if (Random_GetFloat(randomGenerator) <= 0.00028f) {
            interp = 0.0f;
            InterpolationCounter = 1800;
            next = ForcedWeatherType;
            if (next >= 0) {
                NewWeatherType = next;
            } else {
                float v = RandomRange(0.0f, 1.0f);
                ThunderstormOn = 0;
                if (v < 0.75f) {
                    next = 0;
                } else if (v < 0.8f) {
                    next = 4;
                } else if (v < 0.83f) {
                    next = 2;
                } else if (v < 0.85f) {
                    next = 2;
                    if (chapterSeason[chapter] != 2 && ForceSnow != 1) {
                        ThunderstormOn = 1;
                    }
                } else {
                    next = 1;
                }
                NewWeatherType = next;
            }
        } else {
            next = NewWeatherType;
        }
    }
    InterpolationValue = interp;
    WeatherType = interp < 0.5f ? OldWeatherType : next;

    // lightning in a storm
    if (ThunderstormOn == 1 && 0.25f < Rain) {
        if (LightningActive) {
            if ((Rand() & 0xFF) < 0x20) {
                LightningActive = 0;
                ThunderTime = timeInMs;
                LightningFlash = 0;
            } else if (timeInMs - LightningFlashTime > 0x32) {
                uint8_t old = LightningFlash;
                LightningFlash = uint8_t(Rand() & 1);
                if (old != LightningFlash) {
                    LightningFlashTime = timeInMs;
                }
            }
        } else if ((Rand() & 0xFFFF) < 200) {
            LightningActive = 1;
            LightningFlash = 1;
            LightningFlashTime = timeInMs;
        } else {
            LightningFlash = 0;
        }
    } else {
        LightningFlash = 0;
        LightningActive = 0;
    }
    if (ThunderTime != 0 && timeInMs > ThunderTime) {
        int id = Sounds_GetId(sounds, "Thunder");
        Sounds_Play(sounds, id, 0, 1, 0, 0, 0x78, 0, 0, 0, 0, 6, 1, 1.0f, 0);
        ThunderTime = 0;
    }

    int16_t oldType = OldWeatherType;
    int16_t newType = NewWeatherType;
    float i = InterpolationValue;
    bool oldWet = oldType == 2 || oldType == 5;
    bool newWet = newType == 2 || newType == 5;
    if (oldWet) {
        if (newWet) {
            WetRoads = 1.0f;
            Rain = 1.0f;
        } else {
            WetRoads = 1.0f - i;
            float m = 0.5f <= i ? 0.5f : i;
            float m2 = m + m;
            if (m2 >= 1.0f) {
                m2 = 1.0f;
            }
            Rain = 1.0f - m2;
        }
    } else if (newWet) {
        WetRoads = i;
        float m = 0.5f >= i ? 0.5f : i;
        float r = (m - 0.5f) + (m - 0.5f);
        Rain = r >= 1.0f ? 1.0f : r;
    } else {
        WetRoads = 0.0f;
        Rain = 0.0f;
    }
    CloudCoverage = (oldType == 0 || oldType == 4) ? 0.0f : 1.0f - i;
    if (newType != 0 && newType != 4) {
        CloudCoverage = CloudCoverage + i;
    }
    Foggyness = oldType == 3 ? 1.0f - i : 0.0f;
    if (newType == 3) {
        Foggyness = Foggyness + i;
    }
    Overcast = oldType == 4 ? 1.0f - i : 0.0f;
    if (newType == 4) {
        Overcast = Overcast + i;
    }
    if (Overcast > 0.0f) {
        float k = overcastScale * 7.0f;
        if (!(k <= 1.0f)) {
            k = 1.0f;
        }
        Overcast = Overcast * k;
        float v = Overcast;
        if (!(v >= 0.0f)) {
            v = 0.0f;
        }
        Overcast = v <= 1.0f ? v : 1.0f;
    }
    SplashAmount = (1.0f - i) * WeatherSplashes[oldType] + i * WeatherSplashes[newType];
    Ambience_SetRain(ambience, Rain);
    if (!CamNoRain() && !PlayerNoRain()) {
        UpdateAmbience_Game();
        if (!noRainSplashes && 1.1f < SplashAmount) {
            AddRainSplashes();
        }
    }
}

// 0x536130: CWeather::AddRainSplashes: twenty splashes (particle 1, white at 0x20 alpha) around
// the camera on the ground under it (0.25 above; with no ground, half a metre above the camera
// and then none added), each at a random distance (0..40 - 10, plus 0..1 in 1/255 steps) and
// a random angle (half the time 0..255 * 0.0245, else -128..127 * 0.00625 off 0xC3CD08).
// (The main thread's 24-bit x87: the constants are floats, the order the game's, in float)
void __cdecl AddRainSplashesImpl() {
    struct Vec3 {
        float x, y, z;
    };
    uint8_t colour[4] = {0xFF, 0xFF, 0xFF, 0x20};
    const float* camera = W_GetLocalTransform(W_GetCameraNode(theCameraNode)) + 9;
    Vec3 at = {camera[0], camera[1], camera[2]};
    uint8_t found;
    float ground = W_FindGroundZFor3DCoord(at.x, at.y, at.z, &found, 1) + 0.25f;
    if (!found) {
        ground = at.z + 0.5f;
    }
    for (int n = 20; n != 0; n--) {
        float distance = float(Rand()) / 32767.0f * 40.0f - 10.0f;
        distance = float(Rand() & 0xFF) * 0.003921568859368563f + distance;
        int r = Rand() & 0xFFFF;
        float angle;
        if (r & 1) {
            angle = float(Rand() & 0xFF) * 0.024531250819563866f;
        } else {
            angle = float((r & 0xFF) - 0x80) * 0.0062500000931322575f + splashHeading;
        }
        Vec3 pos;
        camera = W_GetLocalTransform(W_GetCameraNode(theCameraNode)) + 9;
        float s = crt::Sinf(angle);
        pos.x = s * distance + camera[0];
        camera = W_GetLocalTransform(W_GetCameraNode(theCameraNode)) + 9;
        float c = crt::Cosf(angle);
        pos.y = c * distance + camera[1];
        pos.z = ground;
        if (found) {
            Vec3 velocity = {-0.0020000000949949026f, -0.0020000000949949026f, 0.014999999664723873f};
            W_AddParticle(1, &pos, &velocity, 0.0f, colour, 0, 0);
        }
    }
}

} // namespace weather17

// ---- registration ----------------------------------------------------------------------

#define WEATHER17_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace weather17 {
WEATHER17_REVERSED("CWeather::UpdateAmbience", 0x00535FD0, &UpdateAmbience);
WEATHER17_REVERSED("CWeather::Init", 0x005360A0, &Init);
WEATHER17_REVERSED("CWeather::SetWeather", 0x005360E0, &SetWeather);
WEATHER17_REVERSED("CWeather::AddRainSplashes", 0x00536130, &AddRainSplashesImpl);
WEATHER17_REVERSED("CWeather::Update", 0x00536320, &Update);
} // namespace weather17
