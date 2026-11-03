// Darts (MGDarts, vtable 0x948924, minigame kind 9): the aiming crosshair (it drifts with
// its own velocity, the left stick pushing it, and bounces off the board's edges), the
// points of where it is (the board's 20 segments and its rings), the darts stuck in the
// board (world quads) and the points floating up from them (0x700D40..0x702130, with the
// MGDarts natives NativesC2.cpp lacks).
//
// The minigame: +0xC4 the stick, +0xCC the crosshair's velocity, +0xD4 its position
// (board units, y down), +0xDC where the dart was thrown, +0xE4 / +0xE8 the dart (script),
// +0xEC the game, +0xF0 the points table, +0x148 the darts stuck (10 at most: 0x30 each
// from +0x14C, four corners), +0x32C five score popups (0x24 each).
// The x87 runs at 24-bit precision here (the main thread): float throughout, the double
// constants' products rounded once to float.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/SchoolClasses.h"
#include "game/ScriptApi.h"
#include "game/TrackMath.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using school::At;

struct DartVec {
    float x, y, z;
};
static_assert(sizeof(DartVec) == 0xC);

// a score floating up from a dart
struct DartScore {
    int32_t points;       // +0x0
    float time;           // +0x4 2 seconds down to 0 (0: unused)
    float unknown08;      // +0x8 (0 when made)
    float screenX;        // +0xC
    float screenY;        // +0x10
    DartVec world;        // +0x14
    uint8_t anchored;     // +0x20 follows the world position on the screen
    uint8_t unknown21[3];
};
static_assert(sizeof(DartScore) == 0x24);

// a dart in the board: four corners
struct DartQuad {
    DartVec corner[4];
};
static_assert(sizeof(DartQuad) == 0x30);

// an Im3D vertex
struct DartVertex {
    float x, y, z;
    float nx, ny, nz;
    int32_t colour;
    float u, v;
};
static_assert(sizeof(DartVertex) == 0x24);

constexpr int kDartsCount = 0x148;
constexpr int kDartsQuads = 0x14C;
constexpr int kDartsScores = 0x32C;

GAME_VAR(uint8_t*, dartCurrentMinigame, 0x020C8704);
GAME_VAR(uint8_t, dartHidden, 0x020C8398);              // (the darts aren't drawn or updated)
GAME_VAR(float, dartBoardScale, 0x00BC6ABC);            // board units per world unit
GAME_VAR(float, dartScoreFade, 0x00BC6AB8);             // a score's time per second
GAME_VAR(float, dartTwoPi, 0x00BC6AB4);
GAME_ARRAY(float, dartRings, 0x020C83B0);               // [6] the bull, bull's eye, treble in/out, double in/out radii
GAME_ARRAY(int32_t, dartSegments, 0x009488A0);          // [21] the segments' values round the board
GAME_VAR(uint32_t, dartScoreColour, 0x020C83A8);
GAME_VAR(uint32_t, dartScoreColour2, 0x020C83AC);
inline const uint8_t* const dartCrosshairColour = reinterpret_cast<const uint8_t*>(0x020C83A4);
GAME_VAR(int32_t, dartModelIndex, 0x00BF44DC);
GAME_VAR(float, dartScreenScaleX, 0x00A957A8);
GAME_VAR(float, dartScreenScaleY, 0x00A957AC);
GAME_VAR(float, dartScreenRatio, 0x00A957B4);
GAME_VAR(uint8_t, dartWidescreen, 0x00C1870E);
GAME_VAR(float, dartTimeStep, 0x00C1A9A4);
GAME_ARRAY(uint8_t, dartFontColour, 0x0208B764);
GAME_ARRAY(uint8_t, dartFontShadowColour, 0x0208B780);
GAME_VAR(int16_t, dartFontShadow, 0x0208B784);
GAME_VAR(int16_t, dartFontEdge, 0x0208B786);
GAME_VAR(float, dartFontWrap, 0x0208B778);
inline void* const dartSounds = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const dartCamera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const dartTheText = reinterpret_cast<void*>(0x020C2FA4);

GAME_FN(void, __cdecl, Dart_Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, Dart_SetRenderState, 0x00519790, int state, int value);
GAME_FN(void*, __cdecl, Dart_TextureGetRaster, 0x0051A120, void* texture);
GAME_FN(void*, __cdecl, Dart_Im3DTransform, 0x005EFAF0, DartVertex* vertices, int count, void* matrix, int flags);
GAME_FN(void, __cdecl, Dart_Im3DRenderIndexed, 0x005F0170, int primitive, int16_t* indices, int count, int);
GAME_FN(void, __cdecl, Dart_Im3DEnd, 0x005EFB20);
GAME_FN(float, __cdecl, Dart_ScreenAnchorX, 0x0052C690, int which);
GAME_FN(float, __cdecl, Dart_ScreenAnchorY, 0x0052C6B0, int which);
GAME_FN(void, __cdecl, Dart_DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture,
        const uint8_t* colour, const float* uv1, const float* uv2);
GAME_FN(uint8_t*, __thiscall, Dart_RGBA_Set, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(int32_t, __thiscall, Dart_ProjectToScreen, 0x004F2240, void* camera, const DartVec* point, float* out, int);
GAME_FN(void, __cdecl, Dart_Font_Reset, 0x00688E60);
GAME_FN(void, __cdecl, Dart_Font_SetSizePreset, 0x00688050, int preset);
GAME_FN(void, __cdecl, Dart_Font_AddNumber, 0x006880D0, float number);
GAME_FN(void, __cdecl, Dart_Font_Format, 0x00688ED0, uint16_t* out, const uint16_t* text, int size);
GAME_FN(void, __cdecl, Dart_Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(const uint16_t*, __thiscall, Dart_Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(uint8_t, __cdecl, Dart_Pad_JustDown, 0x00738900, int control, int pad);
GAME_FN(float, __cdecl, Dart_Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(void, __thiscall, Dart_Vector2D_Normalise, 0x00443730, float* v);
GAME_FN(DartVec*, __cdecl, Dart_CrossProduct, 0x00414730, DartVec* out, const DartVec* a, const DartVec* b);
GAME_FN(void*, __cdecl, Dart_GetProjectile, 0x004D3DD0, int index);
GAME_FN(void, __cdecl, Dart_DestroyProjectile, 0x004D3F10, int index, int);
GAME_FN(void, __thiscall, Dart_Minigame_EndSounds, 0x00704DE0, void* mg);
GAME_FN(void, __thiscall, Dart_Minigame_EndTextures, 0x00704FD0, void* mg);
GAME_FN(void, __thiscall, Dart_Minigame_Construct, 0x007059D0, void* mg);
GAME_FN(void, __thiscall, Dart_Minigame_Destruct, 0x00705A50, void* mg);
GAME_FN(void*, __thiscall, Dart_Minigame_GetTexture, 0x006D7CB0, void* mg, int index);
GAME_FN(void, __thiscall, Dart_Minigame_LoadTextures, 0x00704D30, void* mg, int count,
        const school::ClassTextureEntry* entries, const char* txd);
GAME_FN(void, __thiscall, Dart_Minigame_LoadSounds, 0x00704E90, void* mg, int count,
        const school::ClassSoundEntry* entries, const char* bank);
GAME_FN(uint8_t*, __thiscall, Dart_Sounds_Play, 0x0059B8A0, void* sounds, uint32_t id, const float* pos, int, uintptr_t,
        int, int, int, int, uintptr_t, uintptr_t, int, int, float, int);
// (this file's)
GAME_FN(void, __thiscall, Dart_AddScore, 0x00700D60, void* mg, int32_t points, float x, float y, float z);
GAME_FN(void, __thiscall, Dart_PlayThrowSound, 0x00700FB0, void* mg, int);
GAME_FN(void, __thiscall, Dart_DrawDarts, 0x00701030, void* mg);
GAME_FN(void, __thiscall, Dart_DrawScores, 0x00701290, void* mg);
GAME_FN(int32_t, __thiscall, Dart_GetTargetPoints, 0x00701880, void* mg);
GAME_FN(void, __thiscall, Dart_AddDart, 0x00701CF0, void* mg, const DartVec* pos, const DartVec* dir);
GAME_FN(float*, __thiscall, Dart_GetTargetPos, 0x00700E00, void* mg, float* out);

inline DartScore* Scores(uint8_t* mg) {
    return reinterpret_cast<DartScore*>(mg + kDartsScores);
}
inline DartQuad* Quads(uint8_t* mg) {
    return reinterpret_cast<DartQuad*>(mg + kDartsQuads);
}
// the game's double constants (most are floats widened; 0.8's is not)
constexpr double kMinus002 = -0.019999999552965164;
constexpr double kTenth = 0.10000000149011612;
constexpr double kNudge = 0.029999999329447746;
constexpr double kFifth = 0.20000000298023224;
constexpr double kFourFifths = 0.7999999970197678;

// a float times a double constant, rounded once (the x87 at 24 bits)
inline float MulD(float x, double k) {
    return float(double(x) * k);
}
// the crosshair's starting height scale (162.5 board units, by the screen's ratio)
inline float DartsAimLimit() {
    float ratio = dartWidescreen != 0 ? dartScreenScaleX / dartScreenScaleY : dartScreenRatio;
    return MulD(ratio, 162.5);
}

// 0x700D40 (virtual 5): ends
void __fastcall DartsEnd(uint8_t* mg, re::Edx) {
    mg[4] = 0;
    mg[5] = 0;
    Dart_Minigame_EndSounds(mg);
    Dart_Minigame_EndTextures(mg);
}

// 0x700D60: a score popup in the first free slot (two seconds at the world position)
void __fastcall DartsAddScore(uint8_t* mg, re::Edx, int32_t points, float x, float y, float z) {
    DartScore* scores = Scores(mg);
    for (int i = 0; i < 5; i++) {
        if (scores[i].time == 0.0f) {
            scores[i].time = 2.0f;
            scores[i].unknown08 = 0.0f;
            scores[i].world.x = x;
            scores[i].points = points;
            scores[i].world.y = y;
            scores[i].world.z = z;
            scores[i].anchored = 1;
            return;
        }
    }
}

// 0x700DE0 (CRace's virtual 5, shared): stops
void __fastcall MinigameStop700DE0(uint8_t* mg, re::Edx) {
    mg[4] = 0;
}

// 0x700DF0: running (4 bytes)
uint8_t __fastcall DartsIsAiming(uint8_t* mg, re::Edx) {
    return mg[4];
}
RE_UNHOOKED_FN("MGDarts::IsAiming_700DF0", 0x00700DF0, &DartsIsAiming);

// 0x700E00: the crosshair as a target (world units, y up)
float* __fastcall DartsGetTargetPos(uint8_t* mg, re::Edx, float* out) {
    float scale = dartBoardScale;
    out[0] = At<float>(mg, 0xD4) / scale;
    out[1] = -At<float>(mg, 0xD8) / scale;
    out[2] = 0.0f;
    return out;
}

// 0x700E30 (virtual 0): the deleting destructor
uint8_t* __fastcall DartsDeletingDestructor(uint8_t* mg, re::Edx, int flags) {
    At<uint32_t>(mg, 0) = 0x00948924;
    Dart_Minigame_Destruct(mg);
    if ((flags & 1) != 0) {
        Dart_Free(mg);
    }
    return mg;
}

// 0x700E60 (virtual 4): starts: the crosshair at the board's top, the textures and sounds
void __fastcall DartsStart(uint8_t* mg, re::Edx) {
    mg[4] = 1;
    mg[5] = 0;
    At<float>(mg, 0xC4) = 0.0f;
    At<float>(mg, 0xC8) = 0.0f;
    At<float>(mg, 0xCC) = 0.5f;
    At<float>(mg, 0xD0) = 0.0f;
    float top = DartsAimLimit();
    At<float>(mg, 0xD4) = 0.0f;
    At<float>(mg, 0xD8) = top;
    At<int32_t>(mg, 0xE4) = 1;
    At<int32_t>(mg, 0xE8) = 0;
    At<float>(mg, 0x140) = 0.0f;
    At<int32_t>(mg, 0xEC) = 0;
    At<float>(mg, 0x144) = 0.0f;
    At<int32_t>(mg, kDartsCount) = 0;
    for (int i = 0; i < 5; i++) {
        Scores(mg)[i].time = 0.0f;
    }
    const school::ClassTextureEntry textures[4] = {
            {0, school::GameString(0x009489CC), school::GameString(0x009489BC)},   // dart_model
            {1, school::GameString(0x009489AC), school::GameString(0x00948998)},   // dart_tally_ver
            {2, school::GameString(0x00948988), school::GameString(0x00948974)},   // dart_tally_hor
            {3, school::GameString(0x00948968), school::GameString(0x0094895C)},   // dart_shad
    };
    Dart_Minigame_LoadTextures(mg, 4, textures, school::GameString(0x009489D8));   // MG_Darts
    const school::ClassSoundEntry sounds[2] = {
            {0, school::GameString(0x0094894C)},   // DartHit
            {1, school::GameString(0x00948940)},   // DartMiss
    };
    Dart_Minigame_LoadSounds(mg, 2, sounds, school::GameString(0x00948954));   // Darts
}

// 0x700FB0: the hit sound, or the miss one past 170 units out (its argument unused)
void __fastcall DartsPlayThrowSound(uint8_t* mg, re::Edx, int) {
    float x = At<float>(mg, 0xDC);
    float y = At<float>(mg, 0xE0);
    float distance2 = x * x + y * y;
    uint32_t* ids = At<uint32_t*>(mg, 0x4C);
    uint32_t id = distance2 > 28900.0f ? ids[1] : ids[0];
    Dart_Sounds_Play(dartSounds, id, nullptr, 1, 0, 0, 0x78, 0, 0, 0, 0, 6, 1, 1.0f, 0);
}

// 0x701030: draws the darts stuck in the board (each a textured quad)
void __fastcall DartsDrawDarts(uint8_t* mg, re::Edx) {
    Dart_SetRenderState(8, 1);
    Dart_SetRenderState(6, 1);
    Dart_SetRenderState(0xC, 1);
    Dart_SetRenderState(10, 5);
    Dart_SetRenderState(11, 6);
    Dart_SetRenderState(0x14, 1);
    DartVertex v[4];
    for (int k = 0; k < 4; k++) {
        v[k].nx = 0.0f;
        v[k].ny = 0.0f;
        v[k].nz = 1.0f;
    }
    for (int i = 0; i < At<int32_t>(mg, kDartsCount); i++) {
        const DartQuad& q = Quads(mg)[i];
        static const float kU[4] = {0.0f, 1.0f, 1.0f, 0.0f};
        static const float kV[4] = {0.0f, 0.0f, 1.0f, 1.0f};
        for (int k = 0; k < 4; k++) {
            v[k].x = q.corner[k].x;
            v[k].y = q.corner[k].y;
            v[k].z = q.corner[k].z;
            v[k].colour = -1;
            v[k].u = kU[k];
            v[k].v = kV[k];
        }
        int16_t indices[6] = {2, 1, 0, 0, 2, 3};
        Dart_SetRenderState(1, static_cast<int>(reinterpret_cast<uintptr_t>(
                                       Dart_TextureGetRaster(Dart_Minigame_GetTexture(mg, 3)))));
        Dart_Im3DTransform(v, 4, nullptr, 0x19);
        Dart_Im3DRenderIndexed(3, indices, 6, 0);
        Dart_Im3DEnd();
    }
}

// 0x701290: the score popups: each rises and fades over its two seconds, an outlined number
void __fastcall DartsDrawScores(uint8_t* mg, re::Edx) {
    DartScore* scores = Scores(mg);
    for (int n = 0; n < 5; n++) {
        DartScore& s = scores[n];
        if (s.time == 0.0f) {
            continue;
        }
        float t = 1.0f - s.time * 0.5f;
        float step = dartScoreFade * dartTimeStep;
        float left = s.time - step;
        s.time = left;
        if (left <= 0.0f) {
            s.time = 0.0f;
        }
        if (s.anchored != 0) {
            float screen[3] = {0.0f, 0.0f, 0.0f};
            if (Dart_ProjectToScreen(dartCamera, &s.world, screen, 0) == 0) {
                continue;
            }
            s.screenX = screen[0];
            s.screenY = screen[1];
        }
        float x = s.screenX;
        float y = (t * -50.0f * t - 10.0f) + s.screenY;
        float alpha;
        if (kFifth > double(t)) {
            float q = float(double(t) / kFifth);
            alpha = q * q;
        } else {
            float q = float(double(float(double(t) - kFifth)) / kFourFifths);
            alpha = 1.0f - q * q;
        }
        Dart_Font_Reset();
        dartFontShadow = 0;
        Dart_Font_SetSizePreset(1);
        dartFontWrap = 400.0f;
        uint8_t black[4];
        uint8_t* shadow = Dart_RGBA_Set(black, 0, 0, 0, 0xFF);
        dartFontShadowColour[0] = shadow[0];
        dartFontShadowColour[1] = shadow[1];
        dartFontShadowColour[2] = shadow[2];
        dartFontShadowColour[3] = shadow[3];
        dartFontEdge = 3;
        Dart_Font_AddNumber(float(s.points));
        uint16_t text[0x20];
        Dart_Font_Format(text, Dart_Text_Get(dartTheText, school::GameString(0x009489F8)), 0x20);   // MGDA_NUMBER
        uint32_t c1 = dartScoreColour;
        dartFontColour[1] = uint8_t(c1 >> 8);
        dartFontColour[0] = uint8_t(c1);
        dartFontColour[2] = uint8_t(c1 >> 16);
        dartFontColour[3] = uint8_t(int32_t(float(int32_t(c1 >> 24)) * alpha));
        Dart_Font_PrintString(x + 2.0f, y, text, 1);
        Dart_Font_PrintString(x, y + 2.0f, text, 1);
        Dart_Font_PrintString(x - 2.0f, y, text, 1);
        Dart_Font_PrintString(x, y - 2.0f, text, 1);
        uint32_t c2 = dartScoreColour2;
        dartFontColour[0] = uint8_t(c2);
        dartFontColour[1] = uint8_t(c2 >> 8);
        dartFontColour[2] = uint8_t(c2 >> 16);
        dartFontColour[3] = uint8_t(int32_t(float(int32_t(c2 >> 24)) * alpha));
        Dart_Font_PrintString(x, y, text, 1);
    }
}

// 0x7015D0: the aiming starts (the crosshair back at the top)
void __fastcall DartsStartAiming(uint8_t* mg, re::Edx) {
    mg[4] = 1;
    mg[6] = 0;
    At<float>(mg, 0xC4) = 0.0f;
    At<float>(mg, 0xC8) = 0.0f;
    At<float>(mg, 0xCC) = 0.5f;
    At<float>(mg, 0xD0) = 0.0f;
    float top = DartsAimLimit();
    At<float>(mg, 0xD4) = 0.0f;
    At<float>(mg, 0xD8) = top;
}

// 0x701640 (cdecl): the throw's sound, when darts are on
void __cdecl DartsPlayThrowSoundIfDarts(int arg) {
    uint8_t* mg = dartCurrentMinigame;
    if (mg != nullptr && At<int32_t>(mg, 0x3C) == 9) {
        Dart_PlayThrowSound(mg, arg);
    }
}

// 0x701660: the constructor
uint8_t* __fastcall DartsConstruct(uint8_t* mg, re::Edx) {
    Dart_Minigame_Construct(mg);
    At<uint32_t>(mg, 0) = 0x00948924;
    DartVec* corners = reinterpret_cast<DartVec*>(mg + kDartsQuads);
    for (int i = 0; i < 40; i++) {
        corners[i].x = 0.0f;
        corners[i].y = 0.0f;
        corners[i].z = 0.0f;
    }
    for (int i = 0; i < 5; i++) {
        Scores(mg)[i].world = DartVec{0.0f, 0.0f, 0.0f};
    }
    At<int32_t>(mg, 0x38) = 4;
    At<int32_t>(mg, 0x3C) = 9;
    return mg;
}

// 0x701700 (virtual 2): the darts, the scores, then the crosshair
void __fastcall DartsDraw(uint8_t* mg, re::Edx) {
    if (dartHidden != 0) {
        return;
    }
    Dart_DrawDarts(mg);
    Dart_DrawScores(mg);
    Dart_SetRenderState(6, 0);
    Dart_SetRenderState(8, 0);
    Dart_SetRenderState(0xC, 1);
    Dart_SetRenderState(10, 5);
    Dart_SetRenderState(11, 6);
    if (mg[5] == 0) {
        return;
    }
    float sx = dartScreenScaleX;
    float sy = dartScreenScaleY;
    float cx = float(double(Dart_ScreenAnchorX(1)) - 2.0);
    float cy = float(double(Dart_ScreenAnchorY(1)) - 1.0);
    float ry = dartWidescreen != 0 ? sy / sx : sy;
    float uv2[2] = {1.0f, 1.0f};
    float uv1[2] = {0.0f, 0.0f};
    re::Ptr32<void>* textures = At<re::Ptr32<void>*>(mg, 0x1C);
    void* texture = textures != nullptr ? textures[0] : nullptr;
    float px = At<float>(mg, 0xD4) + cx;
    float halfW = sx * 48.0f;
    float y = At<float>(mg, 0xD8) * ry;
    float bottom = (y + cy) + ry * 48.0f;
    float right = halfW + px;
    float top = (cy + At<float>(mg, 0xD8) * ry) - 48.0f * ry;
    float leftX = px - halfW;
    Dart_DrawRectUV(leftX, top, right, bottom, texture, dartCrosshairColour, uv1, uv2);
}

// 0x701880: the points where the crosshair is (25 the bull, 50 the bull's eye, else the
// segment's value: treble, single or double by the ring; 0 off the board)
int32_t __fastcall DartsGetTargetPoints(uint8_t* mg, re::Edx) {
    float x = At<float>(mg, 0xD4);
    float y = At<float>(mg, 0xD8);
    float sum = y * y + x * x;
    float r = school::CrtSqrt(std::fabs(sum));
    if (dartRings[0] > r) {
        return 0x19;
    }
    if (dartRings[1] > r) {
        return 0x32;
    }
    float angle = trk::CrtAtan2(x, y);
    float a = float(3.141592653589793 - double(angle));
    float half = dartTwoPi / 40.0f;
    float sum2 = a + half;
    float slice = dartTwoPi / 20.0f;
    int segment = int(sum2 / slice);   // (__ftol2)
    int32_t value = dartSegments[segment];
    if (dartRings[2] > r) {
        return value;
    }
    if (dartRings[3] > r) {
        return value * 3;
    }
    if (dartRings[4] > r) {
        return value;
    }
    if (dartRings[5] > r) {
        return value * 2;
    }
    return 0;
}

// 0x7019A0 (virtual 1): the tick: the throw (button 7) or the quit (9), then the crosshair
// drifts (the stick steers it, or it keeps drifting the way it went), held to a speed
// between 0.5 and 6, and bounces off the board's sides
void __fastcall DartsUpdate(uint8_t* mg, re::Edx) {
    if (mg[4] == 0) {
        return;
    }
    Dart_Pad_JustDown(6, 0);
    if (dartHidden != 0) {
        return;
    }
    if (mg[5] != 0 && Dart_Pad_JustDown(7, 0) != 0) {
        At<float>(mg, 0xDC) = At<float>(mg, 0xD4);
        At<float>(mg, 0xE0) = At<float>(mg, 0xD8);
        mg[4] = 0;
        mg[6] = 1;
    }
    if (Dart_Pad_JustDown(9, 0) != 0) {
        mg[4] = 0;
        mg[6] = 0;
    }
    float& stickX = At<float>(mg, 0xC4);
    float& stickY = At<float>(mg, 0xC8);
    float& velX = At<float>(mg, 0xCC);
    float& velY = At<float>(mg, 0xD0);
    float& posX = At<float>(mg, 0xD4);
    float& posY = At<float>(mg, 0xD8);
    stickX = MulD(-Dart_Pad_GetValue(0x10, 0), 0.5);
    stickY = MulD(-Dart_Pad_GetValue(0x11, 0), 0.5);
    float sum = stickY * stickY + stickX * stickX;
    float length = school::CrtSqrt(std::fabs(sum));
    if (length == 0.0f) {
        float v[2] = {velX, velY};
        Dart_Vector2D_Normalise(v);
        float ax = MulD(v[0], kNudge);
        float ay = MulD(v[1], kNudge);
        stickX = ax;
        stickY = ay;
    }
    float dt = dartTimeStep;
    float ax = stickX * dt;
    float ay = dt * stickY;
    velX = ax + velX;
    velY = velY + ay;
    float speed2 = velY * velY + velX * velX;
    if (speed2 > 36.0f) {
        Dart_Vector2D_Normalise(&velX);
        velX = velX * 6.0f;
        velY = 6.0f * velY;
    }
    speed2 = velY * velY + velX * velX;
    if (speed2 < 0.25f) {
        float sum3 = velY * velY + velX * velX;
        float len = school::CrtSqrt(std::fabs(sum3));
        float inv = 1.0f / len;
        float nx = inv * velX;
        float ny = inv * velY;
        velX = nx * 0.5f;
        velY = 0.5f * ny;
    }
    float dx = velX * dartTimeStep;
    float dy = dartTimeStep * velY;
    posX = posX + dx;
    posY = posY + dy;
    float scaleY = dartScreenScaleY;
    if (195.0f < posX) {
        posX = 195.0f;
        velX = velX * -1.0f;
    }
    if (-195.0f > posX) {
        posX = -195.0f;
        velX = velX * -1.0f;
    }
    float limit = float(162.5 / double(scaleY));
    if (limit < posY) {
        posY = limit;
        velY = velY * -1.0f;
    }
    float limit2 = float(-162.5 / double(scaleY));
    if (limit2 > posY) {
        posY = limit2;
        velY = -1.0f * velY;
    }
}

// 0x701CF0: a dart stuck in the board at pos (backed off along its direction): its
// points scored, a quad along the dart (ten at most)
void __fastcall DartsAddDart(uint8_t* mg, re::Edx, const DartVec* pos, const DartVec* dir) {
    int32_t& count = At<int32_t>(mg, kDartsCount);
    if (count >= 10) {
        return;
    }
    DartVec back = {MulD(dir->x, kMinus002), MulD(dir->y, kMinus002), MulD(dir->z, kMinus002)};
    DartVec p = {pos->x + back.x, pos->y + back.y, pos->z + back.z};
    int32_t points = Dart_GetTargetPoints(mg);
    Dart_AddScore(mg, points, p.x, p.y, p.z);
    DartQuad& q = Quads(mg)[count];
    q.corner[0] = p;
    Quads(mg)[count].corner[1] = p;
    Quads(mg)[count].corner[2] = p;
    Quads(mg)[count].corner[3] = p;
    DartVec up = {0.0f, 0.0f, 1.0f};
    DartVec side;
    Dart_CrossProduct(&side, dir, &up);
    DartVec out;
    DartVec* n = Dart_CrossProduct(&out, &side, dir);
    DartVec normal = *n;
    float sx = MulD(side.x, kTenth);
    float sy = MulD(side.y, kTenth);
    float sz = MulD(side.z, kTenth);
    DartVec& c0 = Quads(mg)[count].corner[0];
    c0.x = c0.x + sx;
    c0.y = c0.y + sy;
    c0.z = sz + c0.z;
    float nx = MulD(normal.x, kTenth);
    float ny = MulD(normal.y, kTenth);
    float nz = MulD(normal.z, kTenth);
    DartVec& c2 = Quads(mg)[count].corner[2];
    c2.x = c2.x - nx;
    c2.y = c2.y - ny;
    c2.z = c2.z - nz;
    DartVec& c3 = Quads(mg)[count].corner[3];
    c3.x = c3.x - nx;
    c3.y = c3.y - ny;
    c3.z = c3.z - nz;
    float tx = MulD(side.x, kTenth);
    float ty = MulD(side.y, kTenth);
    float tz = MulD(side.z, kTenth);
    DartVec& c3b = Quads(mg)[count].corner[3];
    c3b.x = c3b.x + tx;
    c3b.y = c3b.y + ty;
    c3b.z = c3b.z + tz;
    count += 1;
}

// 0x701FC0 (cdecl): a dart stuck, when darts are on
void __cdecl DartsAddDartIfDarts(const DartVec* pos, const DartVec* dir) {
    uint8_t* mg = dartCurrentMinigame;
    if (mg != nullptr && At<int32_t>(mg, 0x3C) == 9) {
        Dart_AddDart(mg, pos, dir);
    }
}

// MGDartsGetTargetPos(): x, y, z
int __cdecl Native_MGDartsGetTargetPos(lua_State* L) {
    float target[3];
    Dart_GetTargetPos(dartCurrentMinigame, target);
    Script::PushFloat(L, target[0]);
    Script::PushFloat(L, target[1]);
    Script::PushFloat(L, target[2]);
    return 3;
}

// MGDartsSetDart(a, b)
int __cdecl Native_MGDartsSetDart(lua_State* L) {
    uint8_t* mg = dartCurrentMinigame;
    int b = Script::GetInt(L, 1);
    int a = Script::GetInt(L, 0);
    At<int32_t>(mg, 0xE4) = a;
    At<int32_t>(mg, 0xE8) = b;
    return 0;
}

// MGDartsRemoveProjectiles(): the thrown darts (projectiles of the dart model) destroyed,
// the stuck ones forgotten
int __cdecl Native_MGDartsRemoveProjectiles(lua_State*) {
    for (int i = 0; i < 0x20; i++) {
        if (Dart_GetProjectile(i) != nullptr) {
            if (int32_t(At<int16_t>(Dart_GetProjectile(i), 0x10E)) == dartModelIndex) {
                Dart_DestroyProjectile(i, 0);
            }
        }
    }
    At<int32_t>(dartCurrentMinigame, kDartsCount) = 0;
    return 0;
}

} // namespace

#define DARTS44_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

DARTS44_REVERSED("MGDarts::End_700D40", 0x00700D40, &DartsEnd);
DARTS44_REVERSED("MGDarts::AddScore_700D60", 0x00700D60, &DartsAddScore);
DARTS44_REVERSED("Minigame::Stop_700DE0", 0x00700DE0, &MinigameStop700DE0);
DARTS44_REVERSED("MGDarts::GetTargetPos_700E00", 0x00700E00, &DartsGetTargetPos);
DARTS44_REVERSED("MGDarts::DeletingDestructor_700E30", 0x00700E30, &DartsDeletingDestructor);
DARTS44_REVERSED("MGDarts::Start_700E60", 0x00700E60, &DartsStart);
DARTS44_REVERSED("MGDarts::PlayThrowSound_700FB0", 0x00700FB0, &DartsPlayThrowSound);
DARTS44_REVERSED("MGDarts::DrawDarts_701030", 0x00701030, &DartsDrawDarts);
DARTS44_REVERSED("MGDarts::DrawScores_701290", 0x00701290, &DartsDrawScores);
DARTS44_REVERSED("MGDarts::StartAiming_7015D0", 0x007015D0, &DartsStartAiming);
DARTS44_REVERSED("MGDarts::PlayThrowSoundIfDarts_701640", 0x00701640, &DartsPlayThrowSoundIfDarts);
DARTS44_REVERSED("MGDarts::MGDarts_701660", 0x00701660, &DartsConstruct);
DARTS44_REVERSED("MGDarts::Draw_701700", 0x00701700, &DartsDraw);
// (0x701CF0 calls this with the dart's position already pushed for 0x700D60, expecting
// the stack left alone: ours, a plain thiscall, leaves it so too)
DARTS44_REVERSED("MGDarts::GetTargetPoints_701880", 0x00701880, &DartsGetTargetPoints);
DARTS44_REVERSED("MGDarts::Update_7019A0", 0x007019A0, &DartsUpdate);
DARTS44_REVERSED("MGDarts::AddDart_701CF0", 0x00701CF0, &DartsAddDart);
DARTS44_REVERSED("MGDarts::AddDartIfDarts_701FC0", 0x00701FC0, &DartsAddDartIfDarts);
DARTS44_REVERSED("Native_MGDartsGetTargetPos", 0x00702020, &Native_MGDartsGetTargetPos);
DARTS44_REVERSED("Native_MGDartsSetDart", 0x007020D0, &Native_MGDartsSetDart);
DARTS44_REVERSED("Native_MGDartsRemoveProjectiles", 0x00702130, &Native_MGDartsRemoveProjectiles);
