#include "reversed.h"

#include <cstddef>
#include <cstdint>

// CEntity::ProcessLightsForEntity (0x512790, GTA SA's): an upright entity's light
// effects (its model's 2d effects): each one on or off by its flashing kind (always,
// at night, randomly flickering, blinking at 1/2/4 second rates, on a timed cycle, in
// bad weather), then its corona, and a point light for it.
//
// A light effect: +0 the position (in the entity's space), +0xC..+0xE the colour,
// +0x10 the next effect's index, +0x14 the corona's texture, +0x1C its far clip,
// +0x20 the point light's range, +0x24 the corona's size, +0x2C/+0x30/+0x34 the timed
// cycle (start, on, off; seconds), +0x38 off when the entity's +0xCC, +0x39 the
// flashing kind, +0x3A the flare, +0x3C flags (1 line-of-sight test, 2/4 point light
// kinds, 8 only sets the entity's +0x94, 0x10 corona flag).

#define ENTITYLIGHTS_REVERSED(...) REVERSED(__VA_ARGS__)

namespace {

struct CVector {
    float x, y, z;
};
static_assert(sizeof(CVector) == 0xC);

GAME_ARRAY(uint8_t*, modelInfoPtrs, 0x00C67738);      // CModelInfo::ms_modelInfoPtrs
GAME_VAR(uint8_t, chapter, 0x00A147E0);               // the current chapter
GAME_ARRAY(int32_t, chapterSeason, 0x00A147EC);       // [chapter]
GAME_ARRAY(float, lightsOnHour, 0x00A645C4);          // [season]
GAME_ARRAY(float, lightsOffHour, 0x00A645D4);         // [season]
GAME_VAR(uint8_t, clockHours, 0x00BCEB4B);            // CClock::ms_nGameClockHours
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);             // CTimer::m_snTimeInMilliseconds
GAME_VAR(uint32_t, previousTimeInMs, 0x00C1A9AC);     // CTimer::m_snPreviousTimeInMilliseconds
GAME_VAR(float, g_CE7564, 0x00CE7564);                // (the weather: over 0.5 turns weather lights on)
GAME_VAR(float, lightBrightness, 0x020C4810);

inline void* const theSounds = reinterpret_cast<void*>(0x00CF3DA0);
inline const char* const hazardLiteName = reinterpret_cast<const char*>(0x00914C10);   // "HazardLite"

GAME_FN(float*, __thiscall, Placeable_GetMatrix, 0x0046DD20, void* placeable);
GAME_FN(CVector*, __thiscall, Placeable_GetPosition, 0x00415460, void* placeable);
GAME_FN(uint8_t*, __thiscall, Get2dEffect, 0x0050ED70, void* object);   // (the one its +0x10 indexes)
// (0x0041A9A0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* Matrix_TransformPoint(CVector* out, const float* m, const CVector* v) { return re::CallVec16<0x0041A9A0, CVector*>(out, m, v); }
GAME_FN(int, __thiscall, Sounds_Find, 0x00595B80, void* sounds, const char* name);
GAME_FN(void*, __thiscall, Sounds_PlayAt, 0x0059B8A0, void* sounds, int id, const CVector* pos, int a, const CVector* pos2,
        int b, int c, int d, int e, uintptr_t f, uintptr_t g, int h, int i, float j, int k);
GAME_FN(void, __cdecl, Coronas_RegisterCorona, 0x00511130, int id, int r, int g, int b, int a, const float* pos,
        float size, float farClip, void* texture, int flareType, int losCheck, int unused, float unknown18,
        int flag10, int fadeWhileOffScreen, float pull);
GAME_FN(void, __cdecl, PointLights_Add, 0x00522DB0, int type, float px, float py, float pz, float dx, float dy, float dz,
        float radius, float r, float g, float b, int fog, int shadows);

using re::At;

// the random flicker (kinds 2, 3, 10, 11): off only in the frames where the timer's
// 0x60 bits match `bits`, and then only when its 2-second phase matches the seed
bool Flicker(uint32_t seed, uint32_t bits, bool& flickering) {
    if (((bits ^ timeInMs) & 0x60) == 0) {
        flickering = true;
        return ((timeInMs >> 11) ^ seed) & 3;
    }
    return true;
}

// the timed cycle (kinds 15, 16): on for its first `on` ms of each on + off period
// after `start` (off before it)
bool Cycle(const uint8_t* effect) {
    uint32_t start = uint32_t(int64_t(double(At<float>(const_cast<uint8_t*>(effect), 0x2C)) * 1000.0));
    uint32_t on = uint32_t(int64_t(1000.0 * At<float>(const_cast<uint8_t*>(effect), 0x30)));
    if (timeInMs <= start) {
        return false;
    }
    uint32_t off = uint32_t(int64_t(double(At<float>(const_cast<uint8_t*>(effect), 0x34)) * -1000.0));
    return (timeInMs - start) % (on - off) < on;
}

// ---- 0x512790
void __fastcall Entity_ProcessLightsOurs(uint8_t* self, re::Edx) {
    if (!At<void*>(self, 0x48) || At<int32_t>(self, 0xD0) == 1) {
        return;
    }
    float* matrix = Placeable_GetMatrix(self);
    if (matrix[10] < 0.9599999785423279) {
        return;
    }
    int16_t model = At<int16_t>(self, 0x10E);
    uint8_t* modelInfo = modelInfoPtrs[model];
    float pull = model == 0xD6C ? 1.0f : 0.25f;
    int32_t season = chapterSeason[chapter];
    int32_t onHour = int32_t(lightsOnHour[season]);
    int32_t offHour = int32_t(double(lightsOffHour[season]) - 2.0);
    uint8_t* effect = Get2dEffect(modelInfo);
    if (!effect) {
        return;
    }
    for (uint32_t n = 0; effect; n++) {
        uint8_t* next = Get2dEffect(effect);
        int32_t id = int32_t(uintptr_t(self) + n);
        uint32_t phase80 = n * 0x80;
        uint32_t phase100 = n * 0x100;
        uint32_t phase200 = n * 0x200;
        CVector pos;
        Matrix_TransformPoint(&pos, matrix, reinterpret_cast<const CVector*>(effect));
        bool lit = false;
        bool flickering = false;
        bool night = int32_t(clockHours) > offHour || int32_t(clockHours) < onHour;
        if (!(At<int32_t>(self, 0xCC) != 0 && effect[0x38] != 0)) {
            uint16_t seed = At<uint16_t>(self, 0x10C);
            switch (effect[0x39]) {
            case 0:
                lit = true;
                break;
            case 1:
                lit = night;
                break;
            case 3:
                if (!night && !(0.5f < g_CE7564)) {
                    break;
                }
                [[fallthrough]];
            case 2:
                lit = Flicker(seed, seed, flickering);
                break;
            case 4:
                Matrix_TransformPoint(&pos, matrix, reinterpret_cast<const CVector*>(effect));
                if ((phase80 + timeInMs) & 0x200) {
                    lit = true;
                    if (!(((phase80 + previousTimeInMs) >> 9) & 1)) {
                        // (it came on this frame)
                        CVector* at = Placeable_GetPosition(self);
                        Sounds_PlayAt(theSounds, Sounds_Find(theSounds, hazardLiteName), at, 1, nullptr, 0, 0x78, 0, 0, 0, 0,
                                      6, 0, 1.0f, 0);
                    }
                }
                break;
            case 5:
                lit = night && ((phase80 + timeInMs) & 0x200);
                break;
            case 6:
                lit = ((phase100 + timeInMs) & 0x400) != 0;
                break;
            case 7:
                lit = night && ((phase100 + timeInMs) & 0x400);
                break;
            case 8:
                lit = ((phase200 + timeInMs) & 0x800) != 0;
                break;
            case 9:
                lit = night && ((phase200 + timeInMs) & 0x800);
                break;
            case 11:
                if (!night) {
                    break;
                }
                [[fallthrough]];
            case 10:
                if (uint8_t(seed) > 0x10) {
                    lit = true;
                } else {
                    lit = Flicker(seed, uint32_t(seed) * 8, flickering);
                }
                break;
            case 16:
                if (!night && !(0.5f < g_CE7564)) {
                    break;
                }
                [[fallthrough]];
            case 15:
                lit = Cycle(effect);
                break;
            default:
                break;
            }
        }
        uint8_t flags = effect[0x3C];
        if (flags & 8) {
            At<int32_t>(self, 0x94) = lit ? 0 : 1;
            return;
        }
        if (lit || flickering) {
            int r = lit ? effect[0xC] : 0;
            int g = lit ? effect[0xD] : 0;
            int b = lit ? effect[0xE] : 0;
            Coronas_RegisterCorona(id, r, g, b, 0xFF, &pos.x, At<float>(effect, 0x24), At<float>(effect, 0x1C),
                                   At<void*>(effect, 0x14), effect[0x3A], flags & 1, 0, 0.0f, (flags >> 4) & 1, 1, pull);
            // (the game passes the line-of-sight flag in al with eax's upper bytes left over)
        }
        float range = At<float>(effect, 0x20);
        if (!(range == 0.0f) && lit) {
            float k = lightBrightness;
            float red = float(double(effect[0xC]) * k * 0.003921568859368563);
            float green = float(double(effect[0xD]) * k * 0.003921568859368563);
            float blue = float(double(effect[0xE]) * k * 0.003921568859368563);
            PointLights_Add(0, pos.x, pos.y, pos.z, 0.0f, 0.0f, 0.0f, range, red, green, blue, (flags >> 1) & 3, 1);
        } else if (flags & 4) {
            PointLights_Add(2, pos.x, pos.y, pos.z, 0.0f, 0.0f, 0.0f, 0.0f, float(effect[0xC] * 0.00390625),
                            float(effect[0xD] * 0.00390625), float(effect[0xE] * 0.00390625), 2, 1);
        } else if ((flags & 2) && lit && range == 0.0f) {
            PointLights_Add(3, pos.x, pos.y, pos.z, 0.0f, 0.0f, 0.0f, 0.0f, float(effect[0xC] * 0.00390625),
                            float(effect[0xD] * 0.00390625), float(effect[0xE] * 0.00390625), 1, 1);
        }
        effect = next;
    }
}
ENTITYLIGHTS_REVERSED("CEntity::ProcessLightsForEntity_512790", 0x00512790, RE_BODY(&Entity_ProcessLightsOurs), nullptr, nullptr, re::Mode::Original);

} // namespace
