// 0x5231F0: CPointLights::RenderFogEffect (GTA III's): the fog sprites around the lights
// with fog (1: as foggy as the weather, 2: always). A directional light fills its 12 m
// cone on a 4 m grid (sprites 1.3 m above the ground found below each point); the
// others a 9 m disc on a 2 m grid around the ground below them (1.6 m up), faded from
// 14 m to 28 m from the camera. Half the grid points are skipped by a pattern, which
// also picks the sprite's size; the sprites turn with the time.
//
// (x87 code: the sums are done in double here and stored as float where the game
// stores them. The directional branch leaves one value on the x87 stack per light)

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
    uint8_t type;            // +0x28
    uint8_t fogType;         // +0x29
    uint8_t castExtraShadows;// +0x2A
    uint8_t pad;
};
static_assert(sizeof(PointLight) == 0x2C);

// the vertical line's result as the game lays it out (the entity hit at +0xC, the
// distance at +0x24)
struct ColPoint {
    float point[3];
    re::Ptr32<void> entity;
    float unknown10[3];
    uint32_t unknown1C[2];
    float distance;
};
static_assert(offsetof(ColPoint, distance) == 0x24);

GAME_VAR(int32_t, NumLights, 0x00C92668);
GAME_ARRAY(PointLight, aLights, 0x00C92670);
GAME_ARRAY(float, FogSizes, 0x00A76CF0);   // 8
GAME_VAR(uint8_t, ms_bCutsceneRunning_20C5BE4, 0x020C5BE4);
GAME_VAR(void*, gpPointlightRaster, 0x00C8E988);
GAME_VAR(float, ms_fFoggyness, 0x00CE755C);
GAME_VAR(uint32_t, m_snTimeInMilliseconds, 0x00C1A9B4);
GAME_VAR(void*, ms_pCamera_C3CCF4, 0x00C3CCF4);

GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(float*, __cdecl, VectorAdd, 0x004129A0, float* out, const float* a, const float* b);
GAME_FN(uint8_t, __cdecl, ProcessVerticalLine, 0x004629C0, const float* pos, float zStop, void* colPoint,
        void* out, int, int);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, re::Ptr32<void>* ref);
GAME_FN(uint8_t, __cdecl, GetGroundZ, 0x00523060, float x, float y, float z, float* out);
GAME_FN(uint8_t, __cdecl, CalcScreenCoors, 0x0052AEE0, const float* pos, float* out, float* w, float* h, int, int,
        int, int);
GAME_FN(void, __cdecl, RenderFogSprite_52B7E0, 0x0052B7E0, float x, float y, float z, float w, float h, int r, int g,
        int b, int intensity, float recipZ, int rotation);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);

const float* CameraPosition() {
    return reinterpret_cast<const float*>(GetLocalTransform(GetCameraNode(ms_pCamera_C3CCF4)) + 0x24);
}

int32_t Truncate(double value) {
    return int32_t(value);
}

// one fog sprite: its colour and intensity truncated, its size from the pattern, its
// turn from the time
void DrawSprite(const PointLight& light, const float* screen, float w, float h, float intensity, int sizeIndex,
                uint32_t timeMask, double turnScale) {
    int32_t turn = Truncate(double(int32_t(m_snTimeInMilliseconds & timeMask)) * turnScale) & 0xFF;
    float recipZ = float(1.0 / screen[2]);
    int32_t intensityInt = Truncate(intensity);
    int32_t b = Truncate(double(light.blue) * intensity) & 0xFF;
    int32_t g = Truncate(double(light.green) * intensity) & 0xFF;
    int32_t r = Truncate(double(intensity) * light.red) & 0xFF;
    float size = FogSizes[sizeIndex];
    RenderFogSprite_52B7E0(screen[0], screen[1], screen[2], size * w, size * h, r, g, b, intensityInt, recipZ, turn);
}

} // namespace

namespace PointLightsFog {

// 0x5231F0 (cdecl): CPointLights::RenderFogEffect
static void __cdecl RenderFogEffect() {
    float screen[3] = {0.0f, 0.0f, 0.0f};
    ColPoint colPoint;
    std::memset(&colPoint, 0, sizeof(colPoint));
    colPoint.distance = FLT_MAX;
    if (ms_bCutsceneRunning_20C5BE4) {
        return;
    }
    SetRenderState(8, 0);
    SetRenderState(0xC, 1);
    SetRenderState(0xA, 2);
    SetRenderState(0xB, 2);
    SetRenderState(1, int(re::PtrCast<intptr_t>(gpPointlightRaster)));
    for (int i = 0; i < NumLights; i++) {
        const PointLight& light = aLights[i];
        float fogginess;
        if (light.fogType == 1) {
            fogginess = ms_fFoggyness;
            if (fogginess == 0.0f) {
                continue;
            }
        } else if (light.fogType == 2) {
            fogginess = 1.0f;
        } else {
            continue;
        }

        if (light.type == 1) {
            float along[3] = {float(double(light.dir[0]) * 12.0), float(double(light.dir[1]) * 12.0),
                              float(12.0 * light.dir[2])};
            float coneEnd[3];
            VectorAdd(coneEnd, light.coors, along);
            float minX, maxX, minY, maxY;
            if (!(light.coors[0] > coneEnd[0])) {
                minX = light.coors[0];
                maxX = coneEnd[0];
            } else {
                minX = coneEnd[0];
                maxX = light.coors[0];
            }
            if (!(light.coors[1] > coneEnd[1])) {
                minY = light.coors[1];
                maxY = coneEnd[1];
            } else {
                minY = coneEnd[1];
                maxY = light.coors[1];
            }
            minX = float(minX - 5.0);
            maxX = float(maxX + 5.0);
            minY = float(minY - 5.0);
            maxY = float(5.0 + maxY);
            int32_t xStart = int16_t(Truncate(minX));
            xStart -= xStart % 4;
            int32_t xEnd = int16_t(Truncate(maxX)) + 4;
            if (xStart > xEnd) {
                continue;
            }
            int32_t yStart = int16_t(Truncate(minY));
            yStart -= yStart % 4;
            int32_t yEnd = int16_t(Truncate(maxY)) + 4;
            for (int32_t x = xStart; x <= xEnd; x += 4) {
                for (int32_t y = yStart; y <= yEnd; y += 4) {
                    uint32_t pattern = uint8_t((y >> 2) ^ (x >> 2)) & 0xF;
                    if (!(pattern & 1)) {
                        continue;
                    }
                    int sizeIndex = int16_t(pattern) >> 1;
                    float fx = float(x);
                    float dx = fx - light.coors[0];
                    float fy = float(y);
                    float dy = fy - light.coors[1];
                    float dot = float(double(dy) * light.dir[1] + double(dx) * light.dir[0]);
                    if (!(dot > 0.0f) || !(12.0f > dot)) {
                        continue;
                    }
                    float distSq = float(double(dx) * dx + double(dy) * dy);
                    float perp = float(double(distSq) - double(dot) * dot);
                    if (!(perp < 25.0f)) {
                        continue;
                    }
                    float fog[3] = {fx, fy, float(double(light.coors[2]) + 10.0)};
                    float zStop = float(double(fog[2]) - 20.0);
                    uint32_t out;
                    if (!ProcessVerticalLine(fog, zStop, &colPoint, &out, 1, 0)) {
                        continue;
                    }
                    fog[2] = float(double(colPoint.point[2]) + double(1.3f));
                    dx = fx - light.coors[0];
                    dy = fy - light.coors[1];
                    float dz = fog[2] - light.coors[2];
                    dot = float((double(dy) * light.dir[1] + double(dx) * light.dir[0]) + double(dz) * light.dir[2]);
                    if (!(dot > 0.0f) || !(12.0f > dot)) {
                        continue;
                    }
                    distSq = float((double(dx) * dx + double(dy) * dy) + double(dz) * dz);
                    perp = float(double(distSq) - double(dot) * dot);
                    if (!(perp < 25.0f)) {
                        continue;
                    }
                    float lineDist = float(std::sqrt(double(std::fabs(perp))));
                    double t = double(dot) / 12.0;
                    double strength = double(fogginess) * 158.0 * dot;
                    float dist = float(std::sqrt(double(std::fabs(distSq))));
                    float intensity = float(strength / dist);
                    intensity = float((1.0 - t * t) * intensity);
                    double q = double(lineDist) / 5.0;
                    intensity = float((1.0 - q * q) * intensity);
                    float w;
                    float h;
                    if (!CalcScreenCoors(fog, screen, &w, &h, 1, 0, 0, 1)) {
                        continue;
                    }
                    DrawSprite(light, screen, w, h, intensity, sizeIndex, 0x1FFF, 0.0007666015625);
                }
            }
            continue;
        }

        if (light.type != 0 && light.type != 3 && light.type != 2) {
            continue;
        }
        float groundZ;
        if (!GetGroundZ(light.coors[0], light.coors[1], light.coors[2], &groundZ)) {
            continue;
        }
        float minX = float(double(light.coors[0]) - 9.0);
        float maxX = float(double(light.coors[0]) + 9.0);
        float minY = float(double(light.coors[1]) - 9.0);
        float maxY = float(9.0 + light.coors[1]);
        int32_t xStart = int16_t(Truncate(minX));
        xStart -= xStart % 2;
        int32_t xEnd = int16_t(Truncate(maxX)) + 2;
        if (xStart > xEnd) {
            continue;
        }
        int32_t yStart = int16_t(Truncate(minY));
        yStart -= yStart % 2;
        int32_t yEnd = int16_t(Truncate(maxY)) + 2;
        for (int32_t x = xStart; x <= xEnd; x += 2) {
            for (int32_t y = yStart; y <= yEnd; y += 2) {
                uint32_t pattern = uint8_t((y / 2) ^ (x / 2)) & 0xF;
                if (!(pattern & 1)) {
                    continue;
                }
                int sizeIndex = int16_t(pattern) >> 1;
                float fy = float(y);
                float fx = float(x);
                double ldx = double(fx) - light.coors[0];
                double ldy = double(fy) - light.coors[1];
                float lightDist = float(std::sqrt(double(std::fabs(float(ldx * ldx + ldy * ldy)))));
                if (!(9.0f > lightDist)) {
                    continue;
                }
                const float* camera = CameraPosition();
                float camX = camera[0];
                float camY = camera[1];
                double cdy = double(fy) - camY;
                double cdx = double(fx) - camX;
                float camDist = float(std::sqrt(double(std::fabs(float(cdy * cdy + cdx * cdx)))));
                if (!(camDist < 28.0f)) {
                    continue;
                }
                float camFactor = (14.0 > camDist) ? 1.0f : float(1.0 - (double(camDist) - 14.0) / 14.0);
                double q = double(lightDist) / 9.0;
                float intensity = float(double(fogginess) * 132.0 * (1.0 - q * q) * camFactor);
                float fog[3] = {fx, fy, float(double(groundZ) + double(1.6f))};
                float w;
                float h;
                if (!CalcScreenCoors(fog, screen, &w, &h, 1, 0, 0, 1)) {
                    continue;
                }
                DrawSprite(light, screen, w, h, intensity, sizeIndex, 0x3FFF, 0.00038330078125);
            }
        }
    }
    if (colPoint.entity) {
        CleanUpOldReference(colPoint.entity, &colPoint.entity);
    }
}

} // namespace PointLightsFog

// ---- registration ----------------------------------------------------------------------

#define C16_FOG_REVERSED(name, fn, address)                                                          \
    static void* Original_##fn;                                                                      \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace PointLightsFog {

C16_FOG_REVERSED("CPointLights::RenderFogEffect", RenderFogEffect, 0x005231F0);

} // namespace PointLightsFog
