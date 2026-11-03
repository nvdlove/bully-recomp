#include "reversed.h"

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
// CCoronas: up to 0x38 registered coronas (lights, the sun), faded in and out each
// frame (the ones with the LOS flag only while nothing is in the way) and drawn as
// screen sprites or camera-facing quads, with lens flares for the sun; and CRGBA's
// constructor beside them.
//
// The x87 code keeps sums in extended precision between float stores; the float
// values here are computed in double and rounded where the game stores them.

#define CORONAS_REVERSED(...) REVERSED(__VA_ARGS__)

namespace {

struct RGBA {
    uint8_t r, g, b, a;
};

// (0x30 bytes, at 0xC660A0)
struct RegisteredCorona {
    float pos[3];           // +0
    int32_t id;             // +0xC 0: free
    re::Ptr32<void> texture;          // +0x10
    float size;             // +0x14
    float unknown18;        // +0x18 (the star texture's shape: 1 - it / g_A64858)
    float farClip;          // +0x1C
    float pullTowardsCam;   // +0x20
    uint8_t unknown24[4];   // +0x24
    uint8_t r, g, b;        // +0x28 (doubled when registered)
    uint8_t intensity;      // +0x2B this frame's
    uint8_t fadedIntensity; // +0x2C
    uint8_t flareType;      // +0x2D 0 none, 1, 2
    uint8_t flags;          // +0x2E see below
    uint8_t unknown2F;      // +0x2F
};
static_assert(sizeof(RegisteredCorona) == 0x30);
static_assert(offsetof(RegisteredCorona, r) == 0x28);
static_assert(offsetof(RegisteredCorona, flags) == 0x2E);

// flags
constexpr uint8_t kRegisteredThisFrame = 0x01;
constexpr uint8_t kLosCheck = 0x02;      // faded by line of sight, drawn as a screen sprite
constexpr uint8_t kOffScreen = 0x04;
constexpr uint8_t kKeepWhenFaded = 0x08;
constexpr uint8_t kFlag10 = 0x10;
constexpr uint8_t kFlag20 = 0x20;
constexpr uint8_t kFadeWhileOffScreen = 0x40;

constexpr int kNumCoronas = 0x38;
GAME_ARRAY(RegisteredCorona, coronas, 0x00C660A0);
GAME_ARRAY(void*, coronaTextures, 0x00C66080);     // [8]; [0] the sun's
inline const char* const coronaTextureNames = reinterpret_cast<const char*>(0x00A645E8);   // [8][0x20]
GAME_ARRAY(void*, flareTextures, 0x00C6608C);      // [sprite] (from coronaTextures[3])
inline const uint8_t* const flareDefs1 = reinterpret_cast<const uint8_t*>(0x00A646F0);
inline const uint8_t* const flareDefs2 = reinterpret_cast<const uint8_t*>(0x00A64790);

GAME_VAR(void*, theNiCamera, 0x00C3CCF4);          // TheCamera.camera
inline void* const theCamera = reinterpret_cast<void*>(0x00C3CC68);
GAME_VAR(void*, sceneCamera, 0x00C1AE7C);          // (App's ms_camera)
GAME_VAR(float, timeStep, 0x00C1A9A4);
GAME_VAR(int32_t, g_C1AE70, 0x00C1AE70);
GAME_VAR(uint8_t, losCheckInProgress, 0x00C1AE82);
GAME_VAR(uint8_t, hideCoronaTexture6, 0x00C936FA);
GAME_VAR(float, weatherCE755C, 0x00CE755C);
GAME_VAR(float, starShapeScale, 0x00A64858);
GAME_VAR(float, coronaSizeScale, 0x020C480C);
GAME_VAR(uint8_t, cutsceneRunning, 0x020C5BE4);
GAME_VAR(int32_t, g_C3CD0C, 0x00C3CD0C);
GAME_VAR(int32_t, currArea, 0x00BD1008);
// the sun (time-of-day)
GAME_VAR(uint8_t, sunCoreR, 0x020C4801);
GAME_VAR(uint8_t, sunCoreG, 0x020C4802);
GAME_VAR(uint8_t, sunCoreB, 0x020C4803);
GAME_VAR(uint8_t, sunCoronaR, 0x020C4804);
GAME_VAR(uint8_t, sunCoronaG, 0x020C4805);
GAME_VAR(uint8_t, sunCoronaB, 0x020C4806);
GAME_VAR(float, sunSize, 0x020C4808);
GAME_ARRAY(float, sunDirection, 0x020C4884);

GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(float*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);   // NiTransform (rotate[9], translate)
GAME_FN(uint8_t*, __cdecl, Camera_GetRaster, 0x006C8740, void* camera);   // +0xC width, +0x10 height
GAME_FN(float, __cdecl, Camera_GetFar, 0x006C82D0, void* camera);
GAME_FN(float*, __thiscall, Camera_GetPosition, 0x0041AAC0, void* camera);
GAME_FN(float, __cdecl, Normalised, 0x00415440, float* out, const float* v);
GAME_FN(float*, __cdecl, VectorSub, 0x00414BA0, float* out, const float* a, const float* b);
GAME_FN(uint8_t, __cdecl, Fn_45D450, 0x0045D450, float x, float y);
GAME_FN(uint8_t, __cdecl, Fn_4617D0, 0x004617D0, const float* a, const float* b, int flags);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(int, __cdecl, GetRaster, 0x0051A120, void* texture);
GAME_FN(uint8_t, __cdecl, CalcScreenCoors, 0x0052B090, const float* pos, float* out, float* w, float* h);
GAME_FN(void, __cdecl, RenderOneXLUSprite, 0x0052B690, float x, float y, float z, float w, float h, int r, int g,
        int b, int intensity, int recipZ);
GAME_FN(void, __cdecl, RenderOneXLUSprite2, 0x0052B7E0, float x, float y, float z, float w, float h, int r, int g,
        int b, int intensity, float recipZ, int a);
GAME_FN(void, __cdecl, RenderBillboard, 0x0052B590, const float* pos, float w, float h, int r, int g, int b,
        int intensity, int recipZ);
GAME_FN(void, __cdecl, RenderBillboard2, 0x0052B5D0, const float* pos, float z, float w, float h, int r, int g,
        int b, int intensity, float recipZ, int a);
GAME_FN(void, __cdecl, Txd_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, Txd_PopCurrentTxd, 0x005F0AA0);
GAME_FN(int, __cdecl, Txd_FindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(void, __cdecl, Txd_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void*, __cdecl, Txd_FindTexture, 0x005F1770, const char* name, const char* mask);
GAME_FN(void, __cdecl, Texture_Destroy, 0x005F0F90, void* texture);
GAME_FN(uint8_t, __cdecl, Area_Fn430320, 0x00430320, int area);   // (Game.cpp's CGame::IsAreaInterior)
GAME_FN(void, __cdecl, Coronas_RegisterCoronaFn, 0x00511130, int id, int r, int g, int b, int a, const float* pos,
        float size, float farClip, void* texture, int flareType, int losCheck, int unused, float unknown18,
        int flag10, int fadeWhileOffScreen, float pull);
GAME_FN(void, __cdecl, Coronas_RegisterCoronaIndexFn, 0x005124D0, int id, int r, int g, int b, int a,
        const float* pos, float size, float farClip, int textureIndex, int flareType, int losCheck, int unused,
        float unknown18, int flag10, int fadeWhileOffScreen, float pull);
GAME_FN(uint8_t*, __thiscall, Get2dEffect, 0x0050ED70, void* object);   // the 2d effect its +0x10 indexes
GAME_ARRAY(uint8_t*, modelInfoPtrs, 0x00C67738);   // CModelInfo::ms_modelInfoPtrs

float* CameraTransform() {
    return GetLocalTransform(GetCameraNode(theNiCamera));
}

// the camera's horizontal distance to `pos` (squared) against `range` (the game reads
// the camera four times)
bool WithinRange2D(const float* pos, float range) {
    float x1 = CameraTransform()[9];
    float x2 = CameraTransform()[9];
    float y1 = CameraTransform()[10];
    float y2 = CameraTransform()[10];
    double distSq = (double(x1) - pos[0]) * (double(x2) - pos[0]) + (double(y2) - pos[1]) * (double(y1) - pos[1]);
    return !(distSq > double(range) * range);
}

int FindCorona(int32_t id) {
    uint16_t i = 0;
    do {
        if (coronas[i].id == id) {
            break;
        }
        i++;
    } while (i < kNumCoronas);
    return i;
}

} // namespace

// 0x510DE0 (thiscall)
RGBA* __fastcall RGBA_Construct(RGBA* self, re::Edx, int r, int g, int b, int a) {
    self->r = uint8_t(r);
    self->g = uint8_t(g);
    self->b = uint8_t(b);
    self->a = uint8_t(a);
    return self;
}
CORONAS_REVERSED("CRGBA::CRGBA_510DE0", 0x00510DE0, RE_BODY(&RGBA_Construct), nullptr, nullptr, re::Mode::Original);

// 0x510E00: the corona textures (from the particle txd, the ones not loaded yet) and
// no coronas
void __cdecl Coronas_Init() {
    Txd_PushCurrentTxd();
    Txd_SetCurrentTxd(Txd_FindTxdSlot("particle", 0));
    for (int i = 0; i < 8; i++) {
        if (coronaTextures[i] == nullptr) {
            coronaTextures[i] = Txd_FindTexture(coronaTextureNames + i * 0x20, nullptr);
        }
    }
    Txd_PopCurrentTxd();
    for (int i = 0; i < kNumCoronas; i++) {
        coronas[i].id = 0;
    }
}
CORONAS_REVERSED("CCoronas::Init_510E00", 0x00510E00, &Coronas_Init, nullptr, nullptr, re::Mode::Original);

// 0x510E70
void __cdecl Coronas_Shutdown() {
    for (int i = 0; i < 8; i++) {
        if (coronaTextures[i] != nullptr) {
            Texture_Destroy(coronaTextures[i]);
            coronaTextures[i] = nullptr;
        }
    }
}
CORONAS_REVERSED("CCoronas::Shutdown_510E70", 0x00510E70, &Coronas_Shutdown, nullptr, nullptr, re::Mode::Original);

// 0x510EA0: fades each corona towards this frame's intensity (out when not
// registered, off screen, or blocked when it checks the line of sight), and frees
// the faded ones
void __cdecl Coronas_Update() {
    float cam[3];
    float pos[3] = {0.0f, 0.0f, 0.0f};
    const float* t = CameraTransform();
    float fadeStep = float(double(timeStep) * 15.0);
    cam[0] = t[9];
    cam[1] = t[10];
    cam[2] = t[11];
    g_C1AE70 = 0;
    for (int i = 0; i < kNumCoronas; i++) {
        RegisteredCorona& c = coronas[i];
        if (c.id == 0) {
            continue;
        }
        uint8_t flags = c.flags;
        if (!(flags & kRegisteredThisFrame)) {
            c.intensity = 0;
        }
        pos[1] = c.pos[1];
        pos[0] = c.pos[0];
        pos[2] = c.pos[2];
        uint8_t los = (flags >> 1) & 1;
        losCheckInProgress = los == 1 ? 1 : 0;
        bool visible;
        if ((flags & kOffScreen) && !(flags & kFadeWhileOffScreen)) {
            visible = false;
        } else if (los != 1) {
            visible = true;
        } else {
            visible = Fn_45D450(cam[0], cam[1]) && Fn_45D450(pos[0], pos[1]) && Fn_4617D0(pos, cam, 5);
        }
        if (!visible) {
            int faded = int(double(c.fadedIntensity) - fadeStep);
            float value = float(faded);
            if (value < 0.0) {
                value = 0.0f;
            }
            c.fadedIntensity = uint8_t(int(value));
        } else if (c.intensity > c.fadedIntensity) {
            float target = float(c.intensity);
            float value = float(int(c.fadedIntensity) + int(fadeStep));
            float result = value > target ? target : value;
            c.fadedIntensity = uint8_t(int(result));
        } else if (c.intensity < c.fadedIntensity) {
            float target = float(c.intensity);
            float value = float(int(c.fadedIntensity) - int(fadeStep));
            float result = value < target ? target : value;
            c.fadedIntensity = uint8_t(int(result));
        }
        losCheckInProgress = 0;
        if (c.fadedIntensity == 0 && !(c.flags & kKeepWhenFaded)) {
            c.id = 0;
        }
        c.flags &= 0xF6;
    }
}
CORONAS_REVERSED("CCoronas::Update_510EA0", 0x00510EA0, &Coronas_Update, nullptr, nullptr, re::Mode::Original);

// 0x511130: registers (or refreshes) a corona within `farClip` of the camera
// (horizontally). With flag10 it fades in between 35 and 50 away and hides nearer.
// The colour is doubled (to 0xFF at most); arg 12 is unused
void __cdecl Coronas_RegisterCorona(int32_t id, int r, int g, int b, int a, const float* pos, float size,
                                    float farClip, void* texture, int flareType, int losCheck, int, float unknown18,
                                    int flag10, int fadeWhileOffScreen, float pull) {
    if (!WithinRange2D(pos, farClip)) {
        return;
    }
    uint8_t intensity;
    if (uint8_t(flag10) != 0) {
        float delta[3];
        const float* d = VectorSub(delta, CameraTransform() + 9, pos);
        float x = d[0];
        float y = d[1];
        float z = d[2];
        float distSq = std::fabs(float(double(x) * x + double(y) * y + double(z) * z));
        float dist = float(std::sqrt(double(distSq)));
        if (dist < 35.0f) {
            return;
        }
        if (dist < 50.0) {
            intensity = uint8_t(int((double(dist) - 35.0) / 15.0 * uint8_t(a)));
        } else {
            intensity = uint8_t(a);
        }
    } else {
        intensity = uint8_t(a);
    }
    int i = FindCorona(id);
    if (i == kNumCoronas) {
        if (intensity == 0) {
            return;
        }
        i = FindCorona(0);
        if (i == kNumCoronas) {
            return;
        }
        coronas[i].fadedIntensity = 0;
        coronas[i].flags = uint8_t((coronas[i].flags & 0xDF) | 0xC);
    } else if (coronas[i].fadedIntensity == 0 && intensity == 0) {
        coronas[i].id = 0;
        return;
    }
    RegisteredCorona& c = coronas[i];
    c.id = id;
    int r2 = uint8_t(r) * 2;
    int g2 = uint8_t(g) * 2;
    int b2 = uint8_t(b) * 2;
    c.r = uint8_t(r2 > 0xFF ? 0xFF : r2);
    c.g = uint8_t(g2 > 0xFF ? 0xFF : g2);
    c.b = uint8_t(b2 > 0xFF ? 0xFF : b2);
    c.intensity = intensity;
    c.pos[0] = pos[0];
    c.pos[1] = pos[1];
    c.pos[2] = pos[2];
    c.size = float(double(coronaSizeScale) * size);
    c.unknown18 = unknown18;
    c.farClip = farClip;
    c.texture = texture;
    c.flareType = uint8_t(flareType);
    if (cutsceneRunning == 0 && g_C3CD0C == 0) {
        c.flags ^= (uint8_t(losCheck * 2) ^ c.flags) & kLosCheck;
    } else {
        c.flags &= 0xFD;
    }
    c.pullTowardsCam = pull;
    c.flags = uint8_t((((fadeWhileOffScreen & 1) << 2 | (flag10 & 1)) << 4) | (c.flags & 0xAF) | kRegisteredThisFrame);
}
CORONAS_REVERSED("CCoronas::RegisterCorona_511130", 0x00511130, &Coronas_RegisterCorona, nullptr, nullptr, re::Mode::Original);

// 0x511480: moves a registered corona (freeing it when it has faded out)
void __cdecl Coronas_UpdateCoronaCoors(int32_t id, const float* pos, float farClip, float unknown18) {
    if (!WithinRange2D(pos, farClip)) {
        return;
    }
    int i = FindCorona(id);
    if (i == kNumCoronas) {
        return;
    }
    RegisteredCorona& c = coronas[i];
    if (c.fadedIntensity == 0) {
        c.id = 0;
        return;
    }
    c.pos[0] = pos[0];
    c.pos[1] = pos[1];
    c.pos[2] = pos[2];
    c.unknown18 = unknown18;
}
CORONAS_REVERSED("CCoronas::UpdateCoronaCoors_511480", 0x00511480, &Coronas_UpdateCoronaCoors, nullptr, nullptr, re::Mode::Original);

namespace {

// the corona's position pulled towards the camera (to the far plane for the sun)
void PulledPosition(const RegisteredCorona& c, float* p) {
    p[0] = c.pos[0];
    p[1] = c.pos[1];
    p[2] = c.pos[2];
    const float* camPos = Camera_GetPosition(theCamera);
    float cam[3] = {camPos[0], camPos[1], camPos[2]};
    float d[3];
    d[0] = p[0] - cam[0];
    d[1] = p[1] - cam[1];
    d[2] = p[2] - cam[2];
    Normalised(d, d);
    if (c.id == 1) {
        float farPlane = Camera_GetFar(sceneCamera);
        float e[3] = {d[0] * farPlane, d[1] * farPlane, farPlane * d[2]};
        float f[3] = {float(e[0] * 0.949999988079071), float(e[1] * 0.949999988079071), float(e[2] * 0.949999988079071)};
        p[0] = f[0] + cam[0];
        p[1] = f[1] + cam[1];
        p[2] = f[2] + cam[2];
    }
    float pull = c.pullTowardsCam;
    float g[3] = {d[0] * pull, d[1] * pull, d[2] * pull};
    p[0] = p[0] - g[0];
    p[1] = p[1] - g[1];
    p[2] = p[2] - g[2];
}

int Faded(uint8_t colour, float fogScale) {
    return int(double(colour) / fogScale) & 0xFF;
}

} // namespace

// 0x5115A0: draws the coronas: the LOS-checked ones as screen sprites (reflections =
// 0), the others as camera-facing quads (reflections != 0); dimmer and larger in fog,
// fading over the far half of their range; the star texture (7) stretched; then the
// lens flares of flare types 1 and 2 (the table pointer stays from the last corona
// for other types, uninitialised before the first as in the game)
void __cdecl Coronas_Render(int reflections) {
    uint8_t* raster = Camera_GetRaster(sceneCamera);
    int32_t width = *reinterpret_cast<int32_t*>(raster + 0xC);
    int32_t height = *reinterpret_cast<int32_t*>(raster + 0x10);
    int32_t halfWidth = width / 2;
    int32_t halfHeight = height / 2;
    SetRenderState(8, 0);
    SetRenderState(0xC, 1);
    SetRenderState(10, 2);
    SetRenderState(0xB, 2);
    SetRenderState(9, 2);
    uint8_t hideTexture6 = hideCoronaTexture6;
    const float* t = CameraTransform();
    float forward[3] = {t[2], t[5], t[8]};
    const uint8_t* flare = nullptr;
    float sun[3] = {sunDirection[0], sunDirection[1], sunDirection[2]};
    float sunDot = float((double(sun[0]) * forward[0] + double(sun[1]) * forward[1]) + double(sun[2]) * forward[2]);
    for (int i = 0; i < kNumCoronas; i++) {
        RegisteredCorona& c = coronas[i];
        if (re::Flag(reflections) ? (c.flags & kLosCheck) != 0 : (c.flags & kLosCheck) == 0) {
            continue;
        }
        if (c.id == 0 || (c.fadedIntensity == 0 && c.intensity == 0)) {
            continue;
        }
        if (hideTexture6 != 0 && c.texture == coronaTextures[6]) {
            continue;
        }
        float screen[4];
        float w, h;
        if (!CalcScreenCoors(c.pos, screen, &w, &h)) {
            c.flags |= kOffScreen;
            continue;
        }
        uint8_t flags = c.flags & ~kOffScreen;
        c.flags = flags;
        if (!(c.farClip > screen[2])) {
            continue;
        }
        float z = screen[2];
        float recipZ = float(1.0 / z);
        double halfFar = c.farClip * 0.5;
        float fade = halfFar > z ? 1.0f : float(1.0 - (z - halfFar) / halfFar);
        uint16_t intensity = uint16_t(int(double(c.fadedIntensity) * fade));
        SetRenderState(6, ((flags >> 1) & 1) ? 0 : 1);
        if (c.texture != nullptr) {
            float nearZ = z < 40.0f ? z : 40.0f;
            float fogScale = float(double(nearZ) * weatherCE755C / 40.0 + 1.0);
            if (c.id == 1) {
                screen[2] = float(Camera_GetFar(sceneCamera) * 0.949999988079071);
            }
            SetRenderState(1, GetRaster(c.texture));
            screen[2] = screen[2] - c.pullTowardsCam;
            if (c.texture == coronaTextures[7]) {
                // the star: stretched by its shape value
                float shape = float(1.0 - double(c.unknown18) / starShapeScale);
                shape = shape * shape;
                shape = shape * shape;
                float s = shape * shape;
                float stretch = float(double(s) * 6.0 + 0.5);
                float squash = float(0.3499999940395355 - (double(stretch) - 0.5) * 0.05999999865889549);
                if (squash < 0.15000000596046448) {
                    squash = 0.15000000596046448f;
                }
                flags = c.flags & ~kOffScreen;
                c.flags = flags;
                float W = float(double(c.size) * w * stretch);
                float H = float(double(c.size) * h * fogScale * squash);
                if (0.0 > double(screen[0]) + W || double(screen[1]) + H < 0.0 || double(width) < double(screen[0]) - W ||
                    double(height) < double(screen[1]) - H) {
                    c.flags = flags | kOffScreen;
                } else if (!(flags & kLosCheck)) {
                    float p[3];
                    PulledPosition(c, p);
                    int recip = int(recipZ) & 0xFF;
                    int bb = Faded(c.b, fogScale);
                    int gg = Faded(c.g, fogScale);
                    int rr = Faded(c.r, fogScale);
                    RenderBillboard(p, float(double(c.size) * stretch), float(double(fogScale) * c.size * squash), rr, gg,
                                    bb, intensity, recip);
                } else {
                    int recip = int(recipZ) & 0xFF;
                    int bb = Faded(c.b, fogScale);
                    int gg = Faded(c.g, fogScale);
                    int rr = Faded(c.r, fogScale);
                    RenderOneXLUSprite(screen[0], screen[1], screen[2], W, H, rr, gg, bb, intensity, recip);
                }
            } else {
                flags = c.flags & ~kOffScreen;
                c.flags = flags;
                float W = float(double(c.size) * w * fogScale);
                float H = float(double(c.size) * h * fogScale);
                if (0.0 > double(screen[0]) + W || double(screen[1]) + H < 0.0 || double(width) < double(screen[0]) - W ||
                    double(height) < double(screen[1]) - H) {
                    c.flags = flags | kOffScreen;
                } else if (!(flags & kLosCheck)) {
                    float p[3];
                    PulledPosition(c, p);
                    float size = float(double(c.size) * fogScale);
                    int alpha = int(20.0 * recipZ) & 0xFF;
                    int bb = Faded(c.b, fogScale);
                    int gg = Faded(c.g, fogScale);
                    int rr = Faded(c.r, fogScale);
                    RenderBillboard2(p, screen[2], size, size, rr, gg, bb, intensity, recipZ, alpha);
                } else {
                    int alpha = int(20.0 * recipZ) & 0xFF;
                    int bb = Faded(c.b, fogScale);
                    int gg = Faded(c.g, fogScale);
                    int rr = Faded(c.r, fogScale);
                    RenderOneXLUSprite2(screen[0], screen[1], screen[2], W, H, rr, gg, bb, intensity, recipZ, alpha);
                }
            }
        }
        // the lens flares
        if (c.flareType == 0 || (c.flags & kOffScreen)) {
            continue;
        }
        if (c.flareType == 1) {
            flare = flareDefs1;
        } else if (c.flareType == 2) {
            flare = flareDefs2;
        }
        int16_t sprite = *reinterpret_cast<const int16_t*>(flare + 0x10);
        if (sprite == 0) {
            continue;
        }
        float towardsSun = float((double(sunDot) - 0.5) / 0.48000001907348633);
        do {
            SetRenderState(1, GetRaster(flareTextures[sprite]));
            float strength;
            if (towardsSun < 0.0f) {
                strength = 0.0f;
            } else if (1.0f < towardsSun) {
                strength = 1.0f;
            } else {
                strength = towardsSun;
            }
            if (c.flags & kLosCheck) {
                // (+0: along the line from the screen's centre, +4 size, +8 colour
                // scale (for all three), +0xE intensity scale; 0x14 bytes each)
                float size = float(*reinterpret_cast<const float*>(flare + 4) * 4.0);
                int colourScale = *reinterpret_cast<const int16_t*>(flare + 8);
                float halfH = float(halfHeight);
                int intensityScale = *reinterpret_cast<const int16_t*>(flare + 0xE);
                float halfW = float(halfWidth);
                int recip = int(recipZ) & 0xFF;
                int scaledIntensity = ((intensityScale * int16_t(intensity)) >> 8) & 0xFFFF;
                int flareIntensity = int(double(scaledIntensity) * strength) & 0xFF;
                int bb = int(double(((c.b * colourScale) >> 8) & 0xFF) * strength) & 0xFF;
                int gg = int(double(((c.g * colourScale) >> 8) & 0xFF) * strength) & 0xFF;
                int rr = int(double(strength) * (((c.r * colourScale) >> 8) & 0xFF)) & 0xFF;
                float along = *reinterpret_cast<const float*>(flare);
                float y = float(double(halfH) + (double(screen[1]) - halfH) * along);
                float x = float(double(halfW) + (double(screen[0]) - halfW) * along);
                RenderOneXLUSprite(x, y, screen[2], size, size, rr, gg, bb, flareIntensity, recip);
            }
            sprite = *reinterpret_cast<const int16_t*>(flare + 0x24);
            flare += 0x14;
        } while (sprite != 0);
    }
}
CORONAS_REVERSED("CCoronas::Render_5115A0", 0x005115A0, &Coronas_Render, nullptr, nullptr, re::Mode::Original);

// 0x5124D0: RegisterCorona with the texture by index (coronaTextures[index])
void __cdecl Coronas_RegisterCoronaIndex(int32_t id, int r, int g, int b, int a, const float* pos, float size,
                                         float farClip, int textureIndex, int flareType, int losCheck, int unused,
                                         float unknown18, int flag10, int fadeWhileOffScreen, float pull) {
    Coronas_RegisterCoronaFn(id, r, g, b, a, pos, size, farClip, coronaTextures[uint8_t(textureIndex)], flareType,
                             losCheck, unused, unknown18, flag10, fadeWhileOffScreen, pull);
}
CORONAS_REVERSED("CCoronas::RegisterCorona_5124D0", 0x005124D0, &Coronas_RegisterCoronaIndex, nullptr, nullptr, re::Mode::Original);

// 0x512540: the sun: its core (id 1, flickering a little) 150 away along the sun's
// direction while it's above -0.2, and while it's up and outside and the camera looks
// towards it, its corona with lens flares (id 2)
void __cdecl Coronas_DoSun() {
    const float* t = CameraTransform();
    float cam[3] = {t[9], t[10], t[11]};
    const float* t2 = CameraTransform();
    float forward[3] = {t2[2], t2[5], t2[8]};
    float sun[3] = {sunDirection[0], sunDirection[1], sunDirection[2]};
    float offset[3] = {float(sun[0] * 150.0), float(sun[1] * 150.0), float(sun[2] * 150.0)};
    float pos[3];
    pos[0] = offset[0] + cam[0];
    pos[1] = cam[1] + offset[1];
    pos[2] = cam[2] + offset[2];
    if (!(sun[2] > -0.20000000298023224)) {
        return;
    }
    int random = Rand() & 0xFF;
    float size = float((random * 0.004999999888241291 + 10.0) * sunSize);
    Coronas_RegisterCoronaFn(1, sunCoreR, sunCoreG, sunCoreB, 0xFF, pos, size, 999999.875f, coronaTextures[0], 0, 0, 0,
                             0.0f, 0, 0, 0.25f);
    if (!(0.0f < sunDirection[2])) {
        return;
    }
    if (!Area_Fn430320(currArea)) {
        return;
    }
    float dot = float((double(sun[1]) * forward[1] + double(sun[0]) * forward[0]) + double(sun[2]) * forward[2]);
    if (!(dot > 0.5f)) {
        return;
    }
    Coronas_RegisterCoronaIndexFn(2, sunCoronaR, sunCoronaG, sunCoronaB, 0xFF, pos, float(sunSize * 25.0), 999999.875f,
                                  0, 1, 1, 0, 0.0f, 0, 0, 0.25f);
}
CORONAS_REVERSED("CCoronas::DoSun_512540", 0x00512540, &Coronas_DoSun, nullptr, nullptr, re::Mode::Original);

// 0x512760 (thiscall): the entity's model has a 2d effect with +0x38 set
uint8_t __fastcall Entity_HasEffect38(uint8_t* entity, re::Edx) {
    int16_t model = *reinterpret_cast<int16_t*>(entity + 0x10E);
    uint8_t* effect = Get2dEffect(modelInfoPtrs[model]);
    while (effect != nullptr) {
        if (effect[0x38] != 0) {
            return 1;
        }
        effect = Get2dEffect(effect);
    }
    return 0;
}
CORONAS_REVERSED("CEntity::HasEffect38_512760", 0x00512760, RE_BODY(&Entity_HasEffect38), nullptr, nullptr, re::Mode::Original);
