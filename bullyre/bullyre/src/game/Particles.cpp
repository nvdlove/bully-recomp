// 0x520DA0 to 0x522150: CParticle and cParticleSystemMgr (GTA III's, cut down to six
// systems read from Config\PARTICLE.CFG): the textures ("particle" txd), the pool of 250
// particles, AddParticle, Update (movement, wind, growth, friction, gravity, fade,
// animation, spin) and Render (camera-facing sprites, stretched along the motion for
// the systems with flag 8).
//
// (x87 code: the sums are done in double here and stored as float where the game
// stores them)

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "reversed.h"

#include "game/Crt.h"
namespace {

struct Particle {
    float pos[3];        // +0
    float vel[3];        // +0xC
    float size;          // +0x18
    float lastScreenX;   // +0x1C (flag 8)
    float growth;        // +0x20 (flag 8: the last screen y)
    uint8_t colour[4];   // +0x24 RGBA
    re::Ptr32<Particle> next;      // +0x28
    uint32_t endTime;    // +0x2C
    int16_t fade;        // +0x30
    uint16_t animCounter;// +0x32
    int16_t spin;        // +0x34
    int16_t rotation;    // +0x36 degrees
    uint8_t frame;       // +0x38
    uint8_t alpha;       // +0x39
    uint8_t unknown3A[2];
};
static_assert(sizeof(Particle) == 0x3C);
static_assert(offsetof(Particle, next) == 0x28);

struct ParticleSystem {
    float createRangeSq;   // +0
    float defaultSize;     // +4
    float growth;          // +8
    float gravity;         // +0xC
    float jitterW;         // +0x10 sprite size jitter
    float jitterH;         // +0x14
    float wind;            // +0x18 how much the wind moves it
    float posJitter;       // +0x1C
    float velJitter;       // +0x20
    float stretch;         // +0x24
    int32_t lifespan;      // +0x28 ms
    uint32_t flags;        // +0x2C 4 additive, 8 stretched, 0x20 jitter z too, 0x100 taller when falling
    uint8_t colour[4];     // +0x30
    re::Ptr32<re::Ptr32<void>> rasters;        // +0x34
    re::Ptr32<Particle> head;        // +0x38
    int16_t fade;          // +0x3C
    uint16_t fadeDivisor;  // +0x3E
    uint16_t animSpeed;    // +0x40
    uint8_t index;         // +0x42
    uint8_t alpha;         // +0x43
    uint8_t lastFrame;     // +0x44
    uint8_t friction;      // +0x45 (index into 1, .5, .8, .9, .95, .96, .99)
    uint8_t unknown46[2];
};
static_assert(sizeof(ParticleSystem) == 0x48);
static_assert(offsetof(ParticleSystem, head) == 0x38);

constexpr int kNumParticles = 250;
constexpr int kNumSystems = 6;

GAME_ARRAY(ParticleSystem, mod_ParticleSystemManager, 0x00C92438);
GAME_ARRAY(Particle, gParticleArray, 0x00C8E9A0);
GAME_VAR(Particle*, m_pUnusedListHead, 0x00C8E998);
GAME_VAR(uint8_t, ms_bUserPause, 0x00C1A998);
GAME_VAR(uint8_t, ms_bCodePause, 0x00C1A999);
GAME_VAR(uint8_t, ms_bPause3, 0x00C1A99A);
GAME_VAR(uint32_t, m_snTimeInMilliseconds, 0x00C1A9B4);
GAME_VAR(float, ms_fTimeStep, 0x00C1A9A4);
GAME_VAR(float, ms_fWindStrength, 0x00CE7554);
GAME_VAR(uint8_t, ms_bRenderingParticles, 0x020DCD21);
GAME_ARRAY(char, ms_fileBuffer, 0x00BD7800);   // 0x1C000 bytes
GAME_VAR(const char*, ms_particleCfgName, 0x00A76BE4);   // "PARTICLE.CFG"
inline void* const TheCamera = reinterpret_cast<void*>(0x00C3CC68);

// the textures and their rasters
GAME_VAR(void*, gpSmokeTex, 0x00C8E994);
GAME_VAR(void*, gpSmokeRaster, 0x00C8E990);
GAME_ARRAY(void*, gpRubberTex, 0x00C8E974);      // 5
GAME_ARRAY(void*, gpRubberRaster, 0x00C8E960);
GAME_ARRAY(void*, gpCarDebrisTex, 0x00C8E940);   // 4
GAME_ARRAY(void*, gpCarDebrisRaster, 0x00C8E930);
GAME_VAR(void*, gpRainTex, 0x00C8E95C);
GAME_VAR(void*, gpRainRaster, 0x00C8E958);
GAME_VAR(void*, gpCollisionSmokeTex, 0x00C8E954);
GAME_VAR(void*, gpCollisionSmokeRaster, 0x00C8E950);
GAME_VAR(void*, gpPointlightTex, 0x00C8E98C);
GAME_VAR(void*, gpPointlightRaster, 0x00C8E988);
GAME_ARRAY(const char, ms_rubberNames, 0x00A76B84);     // 5 x 8 ("rubber1"..)
GAME_ARRAY(const char, ms_carDebrisNames, 0x00A76BAC);  // 4 x 14 ("cardebris_01"..)

GAME_FN(void, __cdecl, Texture_Destroy, 0x005F0F90, void* texture);
GAME_FN(int, __cdecl, Txd_FindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(void, __cdecl, Txd_RemoveTxdSlot, 0x005F2240, int slot);
GAME_FN(void, __cdecl, Txd_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, Txd_PopCurrentTxd, 0x005F0AA0);
GAME_FN(void, __cdecl, Txd_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void*, __cdecl, Txd_FindTexture, 0x005F1770, const char* name, const char* mask);
GAME_FN(void*, __cdecl, GetRaster, 0x0051A120, void* texture);
GAME_FN(void, __thiscall, ParticleMgr_Initialise, 0x00522150, void* self);
GAME_FN(void, __thiscall, ParticleMgr_LoadParticleData, 0x00521DA0, void* self);
GAME_FN(void, __cdecl, CParticle_ReloadConfig, 0x00520E50);
GAME_FN(int, __cdecl, LoadFile, 0x0042D180, const char* path, char* buffer, int size, const char* mode);
GAME_FN(const float*, __thiscall, Camera_GetPosition, 0x0041AAC0, void* camera);
GAME_FN(float*, __cdecl, VectorSub_414BA0, 0x00414BA0, float* out, const float* a, const float* b);
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(float, __cdecl, GetRandomNumberInRange, 0x0041A5B0, float min, float max);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(uint8_t, __cdecl, CalcScreenCoors, 0x0052AEE0, const float* pos, float* out, float* w, float* h, int, int,
        int, int);
GAME_FN(void, __cdecl, RenderOneXLUSprite, 0x0052B690, float x, float y, float z, float w, float h, int r, int g,
        int b, int intensity, int alpha);
GAME_FN(void, __cdecl, RenderOneXLUSprite_Rotate_Aspect, 0x0052BD50, float x, float y, float z, float w, float h,
        int r, int g, int b, int intensity, float rotate, int alpha);

float Length3(const float* v) {
    float sq = float((double(v[0]) * v[0] + double(v[1]) * v[1]) + double(v[2]) * v[2]);
    return float(std::sqrt(double(std::fabs(sq))));
}

} // namespace

namespace Particles {

// 0x520DA0 (cdecl): CParticle::Shutdown: the textures destroyed and the txd removed
static void __cdecl Shutdown() {
    Texture_Destroy(gpSmokeTex);
    gpSmokeTex = nullptr;
    for (int i = 0; i < 5; i++) {
        Texture_Destroy(gpRubberTex[i]);
        gpRubberTex[i] = nullptr;
    }
    for (int i = 0; i < 4; i++) {
        Texture_Destroy(gpCarDebrisTex[i]);
        gpCarDebrisTex[i] = nullptr;
    }
    Texture_Destroy(gpRainTex);
    gpRainTex = nullptr;
    Texture_Destroy(gpCollisionSmokeTex);
    gpCollisionSmokeTex = nullptr;
    Texture_Destroy(gpPointlightTex);
    gpPointlightTex = nullptr;
    Txd_RemoveTxdSlot(Txd_FindTxdSlot("particle", 0));
}

// 0x520E50 (cdecl): CParticle::ReloadConfig: the systems reloaded and the 250
// particles all put on the unused list
static void __cdecl ReloadConfig() {
    ParticleMgr_Initialise(mod_ParticleSystemManager);
    m_pUnusedListHead = gParticleArray;
    for (int i = 0; i < kNumParticles; i++) {
        Particle& p = gParticleArray[i];
        p.next = i < kNumParticles - 1 ? &gParticleArray[i + 1] : nullptr;
        p.size = 0.2f;
        p.vel[0] = 0.0f;
        p.growth = 0.0f;
        p.vel[1] = 0.0f;
        p.pos[0] = 0.0f;
        p.pos[1] = 0.0f;
        p.pos[2] = 0.0f;
        p.vel[2] = 0.0f;
        p.endTime = 0;
        p.alpha = 0xFF;
        p.fade = 0;
        p.frame = 0;
        p.animCounter = 0;
        p.rotation = 0;
        p.spin = 0;
    }
}

// 0x520F10 (cdecl): CParticle::Initialise: the config and the textures, each system
// given its rasters
static void __cdecl Initialise() {
    CParticle_ReloadConfig();
    int slot = Txd_FindTxdSlot("particle", 0);
    Txd_PushCurrentTxd();
    Txd_SetCurrentTxd(slot);
    gpSmokeTex = Txd_FindTexture("smokeII_3", nullptr);
    gpSmokeRaster = GetRaster(gpSmokeTex);
    for (int i = 0; i < 5; i++) {
        gpRubberTex[i] = Txd_FindTexture(&ms_rubberNames[i * 8], nullptr);
        gpRubberRaster[i] = GetRaster(gpRubberTex[i]);
    }
    for (int i = 0; i < 4; i++) {
        gpCarDebrisTex[i] = Txd_FindTexture(&ms_carDebrisNames[i * 14], nullptr);
        gpCarDebrisRaster[i] = GetRaster(gpCarDebrisTex[i]);
    }
    gpRainTex = Txd_FindTexture("rainsmall", nullptr);
    gpRainRaster = GetRaster(gpRainTex);
    gpCollisionSmokeTex = Txd_FindTexture("collisionsmoke", nullptr);
    gpCollisionSmokeRaster = GetRaster(gpCollisionSmokeTex);
    gpPointlightTex = Txd_FindTexture("pointlight", nullptr);
    gpPointlightRaster = GetRaster(gpPointlightTex);
    Txd_PopCurrentTxd();
    for (int i = 0; i < kNumSystems; i++) {
        ParticleSystem& system = mod_ParticleSystemManager[i];
        switch (i) {
            case 0: system.rasters = &gpRainRaster; break;
            case 1:
            case 4: system.rasters = &gpSmokeRaster; break;
            case 2: system.rasters = &gpCollisionSmokeRaster; break;
            case 3: system.rasters = gpCarDebrisRaster; break;
            case 5: system.rasters = gpRubberRaster; break;
        }
    }
}

// 0x521080 (cdecl): CParticle::AddParticle: none while paused, out of particles or
// beyond the system's range from the camera. The colour (the system's when `colour`
// has no alpha) doubled; the start spin random in sign; the position and velocity
// jittered; `size` when not 0. `zRotation` above 0 counts as 0
static Particle* __cdecl AddParticle(uint8_t type, const float* pos, const float* vel, float size,
                                     const uint8_t* colour, int spin, int zRotation) {
    if (ms_bPause3 || ms_bCodePause || ms_bUserPause) {
        return nullptr;
    }
    Particle* p = m_pUnusedListHead;
    if (!p) {
        return nullptr;
    }
    ParticleSystem& system = mod_ParticleSystemManager[type];
    if (system.createRangeSq != 0.0f) {
        float d[3];
        VectorSub_414BA0(d, Camera_GetPosition(TheCamera), pos);
        float distSq = float((double(d[0]) * d[0] + double(d[1]) * d[1]) + double(d[2]) * d[2]);
        if (system.createRangeSq < distSq) {
            return nullptr;
        }
    }
    p->size = system.defaultSize;
    p->growth = system.growth;
    p->endTime = uint32_t(system.lifespan) + m_snTimeInMilliseconds;
    p->alpha = system.alpha;
    p->fade = system.fade;
    if (system.fadeDivisor != 0) {
        p->fade = int16_t(int32_t(p->fade) / int32_t(system.fadeDivisor));
    }
    float z = float(zRotation);
    if (!(z <= 0.0f)) {
        z = 0.0f;
    }
    p->animCounter = 0;
    p->frame = uint8_t(int32_t(z));
    std::memcpy(p->pos, pos, sizeof(p->pos));
    std::memcpy(p->vel, vel, sizeof(p->vel));
    if (colour[3]) {
        std::memcpy(p->colour, colour, 4);
    } else {
        std::memcpy(p->colour, system.colour, 4);
    }
    for (int i = 0; i < 3; i++) {
        uint32_t c = uint32_t(p->colour[i]) * 2;
        p->colour[i] = uint8_t(c > 0xFF ? 0xFF : c);
    }
    p->rotation = 0;
    p->spin = spin ? int16_t(spin) : 0;
    if (Rand() & 1) {
        p->spin = int16_t(-p->spin);
    }
    if (system.posJitter != 0.0f) {
        p->pos[0] = float(double(GetRandomNumberInRange(-1.0f, 1.0f)) * system.posJitter + p->pos[0]);
        p->pos[1] = float(double(GetRandomNumberInRange(-1.0f, 1.0f)) * system.posJitter + p->pos[1]);
        if (system.flags & 0x20) {
            p->pos[2] = float(double(GetRandomNumberInRange(-1.0f, 1.0f)) * system.posJitter + p->pos[2]);
        }
    }
    if (system.velJitter != 0.0f) {
        p->vel[0] = float(double(GetRandomNumberInRange(-1.0f, 1.0f)) * system.velJitter + p->vel[0]);
        p->vel[1] = float(double(GetRandomNumberInRange(-1.0f, 1.0f)) * system.velJitter + p->vel[1]);
        if (system.flags & 0x20) {
            p->vel[2] = float(double(GetRandomNumberInRange(-1.0f, 1.0f)) * system.velJitter + p->vel[2]);
        }
    }
    if (size != 0.0f) {
        p->size = size;
    }
    m_pUnusedListHead = p->next;
    p->next = system.head;
    system.head = p;
    return p;
}

// 0x521390 (cdecl): CParticle::Update
static void __cdecl Update() {
    if (ms_bPause3 || ms_bCodePause || ms_bUserPause) {
        return;
    }
    float ts = ms_fTimeStep;
    float friction[7];
    friction[0] = 1.0f;
    static const double kFriction[6] = {0.5, double(0.8f), double(0.9f), double(0.95f), double(0.96f), double(0.99f)};
    for (int i = 0; i < 6; i++) {
        friction[i + 1] = float(std::pow(kFriction[i], double(ts)));
    }
    for (int s = 0; s < kNumSystems; s++) {
        ParticleSystem& system = mod_ParticleSystemManager[s];
        Particle* prev = nullptr;
        Particle* p = system.head;
        while (p) {
            float windStep[3] = {0.0f, 0.0f, 0.0f};
            float step[3];
            float newPos[3];
            float stepTs = ms_fTimeStep;
            for (int i = 0; i < 3; i++) {
                step[i] = float(double(p->vel[i]) * stepTs);
                newPos[i] = step[i] + p->pos[i];
            }
            bool kill = false;
            if (0.0f < ms_fWindStrength && Length3(step) != 0.0f) {
                float r = float(double(Rand()) / 32767.0 * 0.5 + 0.75);
                windStep[0] = float(double(-ms_fWindStrength) * r);
                r = float(double(Rand()) / 32767.0 * 0.5 + 0.75);
                windStep[1] = float(double(-ms_fWindStrength) * r);
                float k = float(double(system.wind) * double(0.002f) * ms_fTimeStep);
                windStep[0] = float(double(k) * windStep[0]);
                windStep[1] = float(double(k) * windStep[1]);
                windStep[2] = float(double(k) * 0.0);
                p->vel[0] = p->vel[0] + windStep[0];
                p->vel[1] = p->vel[1] + windStep[1];
                p->vel[2] = p->vel[2] + windStep[2];
            }
            if (m_snTimeInMilliseconds > p->endTime || p->alpha == 0) {
                kill = true;
            }
            if (!kill && !(system.flags & 8)) {
                float size = p->size;
                if (0.0f < p->growth) {
                    float speed = Length3(windStep);
                    float stepSpeed = Length3(step);
                    float m = (stepSpeed < speed) ? speed : stepSpeed;
                    if (system.index == 4) {
                        m = m + m;
                    }
                    size = float((double(m) + 1.0) * p->growth + size);
                } else {
                    size = p->growth + size;
                }
                if (size < 0.0f) {
                    kill = true;
                } else {
                    p->size = size;
                }
            }
            if (!kill) {
                float f = friction[system.friction];
                p->vel[0] = p->vel[0] * f;
                p->vel[1] = f * p->vel[1];
                p->vel[2] = f * p->vel[2];
                bool fall = false;
                if (0.0f < system.gravity) {
                    fall = float(double(system.gravity) * -50.0) < p->vel[2];
                } else if (0.0f > system.gravity) {
                    fall = float(double(system.gravity) * -5.0) > p->vel[2];
                }
                if (fall) {
                    p->vel[2] = float(double(p->vel[2]) - double(ms_fTimeStep) * system.gravity);
                }
                if (p->fade != 0) {
                    int32_t a = int32_t(p->alpha) - p->fade;
                    a = a < 0 ? 0 : (a > 0xFF ? 0xFF : a);
                    p->alpha = uint8_t(a);
                    if (p->alpha == 0) {
                        kill = true;
                    }
                }
            }
            if (kill) {
                if (prev) {
                    prev->next = p->next;
                    p->next = m_pUnusedListHead;
                    m_pUnusedListHead = p;
                    p = prev->next;
                } else {
                    system.head = p->next;
                    p->next = m_pUnusedListHead;
                    m_pUnusedListHead = p;
                    p = system.head;
                }
                continue;
            }
            if (system.animSpeed != 0) {
                if (p->animCounter > system.animSpeed) {
                    p->frame++;
                    p->animCounter = 0;
                    if (p->frame > system.lastFrame) {
                        p->frame--;
                    }
                } else {
                    p->animCounter++;
                }
            }
            if (p->spin != 0) {
                p->rotation = int16_t(p->rotation + p->spin);
            }
            std::memcpy(p->pos, newPos, sizeof(newPos));
            prev = p;
            p = p->next;
        }
    }
}

// 0x5218A0 (cdecl): CParticle::Render: per system its blend (additive with flag 4) and
// raster (by frame when animated); each particle as a sprite with its size jittered:
// a spinning one rotated, a flag-8 one stretched and turned along its screen motion
// (then its screen position kept), and every one also drawn upright (taller with its
// fall speed for flag 0x100)
static void __cdecl Render() {
    ms_bRenderingParticles = 1;
    SetRenderState(2, 1);
    SetRenderState(5, 1);
    SetRenderState(0xE, 0);
    SetRenderState(8, 0);
    SetRenderState(6, 1);
    SetRenderState(0xC, 1);
    uint32_t currentFlags = 4;
    SetRenderState(0xA, 5);
    SetRenderState(0xB, 6);
    void* currentRaster = nullptr;
    for (int s = 0; s < kNumSystems; s++) {
        ParticleSystem& system = mod_ParticleSystemManager[s];
        Particle* p = system.head;
        re::Ptr32<void>* rasters = system.rasters;
        if (!p) {
            continue;
        }
        if ((currentFlags & 4) != (system.flags & 4)) {
            if (system.flags & 4) {
                SetRenderState(0xA, 5);
                SetRenderState(0xB, 6);
            } else {
                SetRenderState(0xA, 2);
                SetRenderState(0xB, 2);
            }
            currentFlags = system.flags;
        }
        if (rasters && rasters[0] != currentRaster) {
            SetRenderState(1, int(re::PtrCast<intptr_t>(rasters[0])));
            currentRaster = rasters[0];
        }
        for (; p; p = p->next) {
            if (p->alpha == 0) {
                continue;
            }
            if (system.lastFrame && rasters && rasters[p->frame] != currentRaster) {
                SetRenderState(1, int(re::PtrCast<intptr_t>(rasters[p->frame])));
                currentRaster = rasters[p->frame];
            }
            int r = p->colour[0];
            int g = p->colour[1];
            int b = p->colour[2];
            float screen[3] = {0.0f, 0.0f, 0.0f};
            float w;
            float h;
            if (!CalcScreenCoors(p->pos, screen, &w, &h, 1, 0, 0, 1)) {
                continue;
            }
            float jitter = float(double(Rand()) / 32767.0 * 0.8999999985098839 + 0.10000000149011612);
            w = float(double(system.jitterW) * jitter + w);
            jitter = float(double(Rand()) / 32767.0 * 0.8999999985098839 + 0.10000000149011612);
            h = float(double(system.jitterH) * jitter + h);
            if (p->rotation != 0) {
                float angle = float(double(p->rotation) * 3.1415927410125732 / 180.0);
                RenderOneXLUSprite_Rotate_Aspect(screen[0], screen[1], screen[2], float(p->size * w),
                                                 float(p->size * h), r, g, b, 0xFF, angle, p->alpha);
            } else if (system.flags & 8) {
                float length;
                float angle;
                if (p->lastScreenX == 0.0f) {
                    length = 0.0f;
                    angle = 0.0f;
                } else {
                    float dx = screen[0] - p->lastScreenX;
                    float dy = screen[1] - p->growth;
                    length = float(std::sqrt(double(std::fabs(float(double(dx) * dx + double(dy) * dy)))));
                    angle = float(std::asin(double(float(dx / length))));
                    if (dy < 0.0f) {
                        angle = float(3.1415927410125732 - angle);
                    }
                    float speed = Length3(p->vel);
                    float t = float(double(speed) * ms_fTimeStep * w);
                    double twice = double(t) + t;
                    if (length > twice) {
                        length = float(twice);
                    }
                }
                float height = float(double(system.stretch) * length + double(p->size) * h);
                RenderOneXLUSprite_Rotate_Aspect(screen[0], screen[1], screen[2], float(p->size * w), height, r, g,
                                                 b, 0xFF, angle, p->alpha);
                p->lastScreenX = screen[0];
                p->growth = screen[1];
            }
            float height;
            if (system.flags & 0x100) {
                float fall = float(double(p->vel[2]) * 10.0);
                if (fall < 0.0f) {
                    fall = float(double(fall) * -1.0);
                }
                height = float((double(fall) * system.stretch + p->size) * h);
            } else {
                height = float(p->size * h);
            }
            RenderOneXLUSprite(screen[0], screen[1], screen[2], float(p->size * w), height, r, g, b, 0xFF, p->alpha);
        }
    }
    SetRenderState(0xC, 0);
    SetRenderState(8, 1);
    SetRenderState(6, 1);
    SetRenderState(0xA, 5);
    SetRenderState(0xB, 6);
    ms_bRenderingParticles = 0;
}

// 0x521DA0 (thiscall): cParticleSystemMgr::LoadParticleData: Config\PARTICLE.CFG read
// line by line up to ";the end"; each non-comment line's fields (space or tab
// separated; field 0 starts the next system) into the systems, six at most
static void __fastcall LoadParticleData(ParticleSystem* self, re::Edx) {
    char path[256];
    std::sprintf(path, "Config\\%s", static_cast<const char*>(ms_particleCfgName));
    LoadFile(path, ms_fileBuffer, 0x1C000, "r");
    int count = 0;
    ParticleSystem* system = nullptr;   // (fields before the first system: written through null)
    char* start = ms_fileBuffer;
    char* scan = ms_fileBuffer + 1;
    char line[500];
    for (;;) {
        while (*scan != '\n') {
            scan++;
        }
        int length = int(scan - start);
        std::strncpy(line, start, length);
        char* next = scan + 1;
        line[length] = '\0';
        scan += 2;
        if (std::strncmp(line, ";the end", 8) == 0) {
            return;
        }
        start = next;
        if (line[0] == ';') {
            continue;
        }
        char delimiters[3] = {' ', '\t', '\0'};
        char* token = std::strtok(line, delimiters);
        ParticleSystem* cursor = self + count;
        uint32_t field = 0;
        do {
            switch (field) {
                case 0:
                    system = cursor;
                    cursor++;
                    count++;
                    system->index = uint8_t(count - 1);
                    break;
                case 1: system->colour[0] = uint8_t(std::atol(token)); break;
                case 2: system->colour[1] = uint8_t(std::atol(token)); break;
                case 3: system->colour[2] = uint8_t(std::atol(token)); break;
                case 9: system->defaultSize = float(std::atof(token)); break;
                case 10: system->growth = float(std::atof(token)); break;
                case 14: system->alpha = uint8_t(std::atol(token)); break;
                case 15: system->fadeDivisor = uint16_t(std::atol(token)); break;
                case 16: system->fade = int16_t(std::atol(token)); break;
                case 23: system->animSpeed = uint16_t(std::atol(token)); break;
                case 25: system->lastFrame = uint8_t(std::atol(token)); break;
                case 27: system->gravity = float(std::atof(token)); break;
                case 28:
                    switch (std::atol(token)) {
                        case 50: system->friction = 1; break;
                        case 80: system->friction = 2; break;
                        case 90: system->friction = 3; break;
                        case 95: system->friction = 4; break;
                        case 96: system->friction = 5; break;
                        case 99: system->friction = 6; break;
                        default: system->friction = 0; break;
                    }
                    break;
                case 29: system->lifespan = std::atol(token); break;
                case 30: system->posJitter = float(std::atof(token)); break;
                case 31: system->velJitter = float(std::atof(token)); break;
                case 35: system->stretch = float(std::atof(token)); break;
                case 36: system->jitterW = float(std::atof(token)); break;
                case 37: system->jitterH = float(std::atof(token)); break;
                case 38: system->wind = float(std::atof(token)); break;
                case 39: {
                    float range = float(std::atof(token));
                    system->createRangeSq = range * range;
                    break;
                }
                case 40: system->flags = uint32_t(std::atol(token)); break;
                default: break;
            }
            token = std::strtok(nullptr, delimiters);
            field++;
            if (field > 0x28) {
                field = 0;
            }
        } while (token && count < kNumSystems);
    }
}

// 0x522150 (thiscall): cParticleSystemMgr::Initialise: the data loaded and the lists
// emptied
static void __fastcall MgrInitialise(ParticleSystem* self, re::Edx) {
    ParticleMgr_LoadParticleData(self);
    for (int i = 0; i < kNumSystems; i++) {
        self[i].head = nullptr;
    }
}

} // namespace Particles

// ---- registration ----------------------------------------------------------------------

#define C16_PARTICLE_REVERSED(name, fn, address)                                                     \
    static void* Original_##fn;                                                                      \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace Particles {

C16_PARTICLE_REVERSED("CParticle::Shutdown", Shutdown, 0x00520DA0);
C16_PARTICLE_REVERSED("CParticle::ReloadConfig", ReloadConfig, 0x00520E50);
C16_PARTICLE_REVERSED("CParticle::Initialise", Initialise, 0x00520F10);
C16_PARTICLE_REVERSED("CParticle::AddParticle", AddParticle, 0x00521080);
C16_PARTICLE_REVERSED("CParticle::Update", Update, 0x00521390);
C16_PARTICLE_REVERSED("CParticle::Render", Render, 0x005218A0);
C16_PARTICLE_REVERSED("cParticleSystemMgr::LoadParticleData", LoadParticleData, 0x00521DA0);
C16_PARTICLE_REVERSED("cParticleSystemMgr::Initialise", MgrInitialise, 0x00522150);

} // namespace Particles
