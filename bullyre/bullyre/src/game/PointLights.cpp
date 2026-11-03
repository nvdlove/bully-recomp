// 0x522DB0 to 0x523060: CPointLights (GTA III's): AddLight (32 a frame), the lights
// affecting a position (given to the light accumulator at 0xD140B8), and the ground
// height under a light's fog, cached for 32 positions.
//
// (x87 code: the sums are done in double here and stored as float where the game
// stores them)

#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "reversed.h"

namespace {

struct PointLight {
    float coors[3];          // +0
    float dir[3];            // +0xC
    float radius;            // +0x18
    float red;               // +0x1C
    float green;             // +0x20
    float blue;              // +0x24
    uint8_t type;            // +0x28 0 omni, 1 directional, 2 and 3: not lighting objects
    uint8_t fogType;         // +0x29
    uint8_t castExtraShadows;// +0x2A
    uint8_t pad;
};
static_assert(sizeof(PointLight) == 0x2C);

constexpr int kMaxPointLights = 32;
GAME_VAR(int32_t, NumLights, 0x00C92668);
GAME_ARRAY(PointLight, aLights, 0x00C92670);
GAME_VAR(int32_t, ms_groundCacheNext, 0x00C9266C);
GAME_ARRAY(float, ms_groundCacheZ, 0x00C925E8);     // 32
GAME_ARRAY(float, ms_groundCachePos, 0x00C92BF0);   // 32 x (x, y, z)
inline void* const ms_lightAccumulator = reinterpret_cast<void*>(0x00D140B8);

GAME_FN(void, __thiscall, LightAccumulator_Add, 0x005EE540, void* self, const float* dir, const float* colour);
GAME_FN(uint8_t, __cdecl, ProcessVerticalLine, 0x004629C0, const float* pos, float zStop, void* colPoint,
        void* out, int, int);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, re::Ptr32<void>* ref);

} // namespace

namespace PointLights {

// 0x522DB0 (cdecl): CPointLights::AddLight (none past 32)
static void __cdecl AddLight(int type, float x, float y, float z, float dx, float dy, float dz, float radius,
                             float red, float green, float blue, int fogType, int castExtraShadows) {
    int32_t i = NumLights;
    if (i == kMaxPointLights) {
        return;
    }
    PointLight& light = aLights[i];
    light.type = uint8_t(type);
    light.fogType = uint8_t(fogType);
    light.coors[0] = x;
    light.coors[1] = y;
    light.coors[2] = z;
    light.dir[0] = dx;
    light.dir[1] = dy;
    light.dir[2] = dz;
    light.radius = radius;
    light.castExtraShadows = uint8_t(castExtraShadows);
    light.red = red;
    NumLights = i + 1;
    light.green = green;
    light.blue = blue;
}

// 0x522E50 (cdecl): the lights reaching a position (not types 2 and 3), full to half
// their radius then fading; a directional one by how much it faces the position; each
// given to the accumulator as its direction and colour
static void __cdecl AddLightsAffectingPosition(const float* pos) {
    float p[3] = {pos[0], pos[1], pos[2]};
    for (int i = 0; i < NumLights; i++) {
        const PointLight& light = aLights[i];
        uint8_t type = light.type;
        if (type == 2 || type == 3) {
            continue;
        }
        float radius = light.radius;
        float d[3] = {p[0] - light.coors[0], p[1] - light.coors[1], p[2] - light.coors[2]};
        float distSq = float((double(d[0]) * d[0] + double(d[1]) * d[1]) + double(d[2]) * d[2]);
        if (distSq == 0.0f || !(double(radius) * radius > distSq)) {
            continue;
        }
        float dist = float(std::sqrt(double(std::fabs(distSq))));
        float fraction = dist / radius;
        float intensity = 1.0f;
        if (fraction >= 0.5f) {
            intensity = float(1.0 - (double(fraction) - 0.5) * 2.0);
        }
        float inv = float(1.0 / dist);
        d[0] = d[0] * inv;
        d[1] = d[1] * inv;
        d[2] = inv * d[2];
        if (type == 1) {
            float dot = float((double(light.dir[1]) * d[1] + double(light.dir[0]) * d[0]) + double(light.dir[2]) * d[2]);
            float facing = float((double(dot) - 0.5) * 2.0);
            if (facing < 0.0f) {
                facing = 0.0f;
            }
            intensity = facing * intensity;
        }
        if (!(intensity > 0.0f)) {
            continue;
        }
        float colour[4] = {light.red * intensity, light.green * intensity, intensity * light.blue, 1.0f};
        LightAccumulator_Add(ms_lightAccumulator, d, colour);
    }
}

// 0x523060 (cdecl): the ground height below a position (a vertical line down 20),
// cached for 32 positions (matched exactly). False (nothing written) when there's no
// ground
static uint8_t __cdecl GetGroundZ(float x, float y, float z, float* out) {
    for (int i = 0; i < 32; i++) {
        const float* cached = &ms_groundCachePos[i * 3];
        if (x == cached[0] && y == cached[1] && z == cached[2]) {
            *out = ms_groundCacheZ[i];
            return 1;
        }
    }
    // the stack as the game lays it out: the line's end height, then its result (the
    // point, the entity hit at +0xC, the distance at +0x24)
    struct {
        float zStop;
        float point[3];
        re::Ptr32<void> entity;
        float unknown14[2];
        float unknown1C;
        uint32_t unknown20[2];
        float distance;
        uint32_t unknown2C;
    } buffer;
    std::memset(&buffer, 0, sizeof(buffer));
    buffer.distance = FLT_MAX;
    buffer.zStop = float(double(z) - 20.0);
    float pos[3] = {x, y, z};
    if (!ProcessVerticalLine(pos, buffer.zStop, &buffer.point, &buffer.zStop, 1, 0)) {
        if (buffer.entity) {
            CleanUpOldReference(buffer.entity, &buffer.entity);
        }
        return 0;
    }
    int32_t next = ms_groundCacheNext;
    ms_groundCacheZ[next] = buffer.point[2];
    ms_groundCachePos[next * 3] = x;
    ms_groundCachePos[next * 3 + 1] = y;
    ms_groundCachePos[next * 3 + 2] = z;
    ms_groundCacheNext = (next + 1) % 32;
    *out = buffer.point[2];
    if (buffer.entity) {
        CleanUpOldReference(buffer.entity, &buffer.entity);
    }
    return 1;
}

} // namespace PointLights

// ---- registration ----------------------------------------------------------------------

#define C16_POINTLIGHTS_REVERSED(name, fn, address)                                                  \
    static void* Original_##fn;                                                                      \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace PointLights {

C16_POINTLIGHTS_REVERSED("CPointLights::AddLight", AddLight, 0x00522DB0);
C16_POINTLIGHTS_REVERSED("CPointLights::AddLightsAffectingPosition", AddLightsAffectingPosition, 0x00522E50);
C16_POINTLIGHTS_REVERSED("CPointLights::GetGroundZ", GetGroundZ, 0x00523060);

} // namespace PointLights
