// The tagging meter (HUD_TAGGING_METER): spraying a tag, stroke by stroke, following
// the stick round each stroke's curve; the strokes drawn as ribbons or brush stamps
// (three sets of 7 shapes: two shadows and the paint), the paint's drips, the button
// prompts. 0x557C10..0x55A3C4 (its constructor and Reset are Hud.cpp's).
//
// A shape (0x1E8 bytes, 0x5593A0 makes one):
//   +0x00 progress drawn        +0x04 steps (0x80)    +0x08.. four corners (the stamp's)
//   +0x28 / +0x2C the angle's range (turns of pi)     +0x30 segment count
//   +0x34 segments (0x28 each): from, to (of 0..1), x, y, width, width2, 4 colours
//   +0x1DC the stamp's texture (nullptr: a ribbon)   +0x1E0 its length drawn
//   +0x1E4 1: not drawn, 2: a ribbon, else stamps
// The meter, by offset: +0x40 / +0xD98 / +0x1AF0 the three sets, +0x2848 tagging,
//   +0x284C the stroke's progress, +0x2850 its speed, +0x2854 the tag, +0x285C strokes
//   done, +0x2864 the next button, +0x2868 [8] each stroke's speed (+0x2884 the total),
//   +0x2888 drips done, +0x288C the fade out, +0x2890 [0x78] drips (pos, length, size),
//   +0x2E38 drips made, +0x2E3C a drip growing, +0x2E40 the drips' wobble angle

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Hud.h"
#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using vc::At;

struct CVector2D {
    float x, y;
};

GAME_VAR(float, g_screenScaleX, 0x00A957A8);
GAME_VAR(float, g_screenScaleY, 0x00A957AC);
GAME_VAR(float, g_timeStep, 0x00C1A9A4);
GAME_VAR(float, g_frameSeconds, 0x00AB7FF8);   // (0.02)
GAME_VAR(float, g_aimTolerance, 0x00AB7FFC);   // (the stick's angle that still counts)
GAME_VAR(void*, g_stampTexture, 0x00C66F6C);
GAME_VAR(void*, g_paintTexture, 0x00C66F70);
GAME_VAR(CRGBA, g_paintColour, 0x00CF0A38);
GAME_VAR(CRGBA, g_buttonColour, 0x00CF0A3C);
GAME_VAR(int32_t, g_padConnected, 0x020C8AE8);
GAME_VAR(HUDComponent*, g_hudWeapons, 0x00C674F0);  // (shown again when the tag's done)
GAME_ARRAY(int32_t, g_tagStrokeCounts, 0x00AB7718);  // [tag]
GAME_ARRAY(uint8_t, g_tagStrokes, 0x00AB7738);       // [tag] 0x118 bytes: 7 strokes of 0x28

inline uint8_t* Stroke(int tag, int stroke) {
    return g_tagStrokes + tag * 0x118 + stroke * 0x28;
}

GAME_FN(CRGBA*, __thiscall, RGBA_Construct, 0x00510DE0, CRGBA* rgba, int r, int g, int b, int a);
inline int __cdecl GameRand() { return crt::Rand(); }
GAME_FN(float, __cdecl, GetRandomNumberInRange, 0x0041A5B0, float min, float max);
GAME_FN(void, __cdecl, EmptyFunction, 0x0049A650, int);
GAME_FN(void*, __cdecl, GetButtonTexture, 0x00514EC0, int button);
GAME_FN(void, __cdecl, DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture, const CRGBA* colour,
        const CVector2D* uv1, const CVector2D* uv2);
GAME_FN(void, __cdecl, DrawStamp, 0x0052CA30, float x1, float y1, float x2, float y2, void* texture, const CRGBA* colour,
        const CVector2D* uv1, const CVector2D* uv2);
GAME_FN(void, __cdecl, DrawQuadTextured, 0x0052CE00, float x0, float y0, float x1, float y1, float x2, float y2, float x3,
        float y3, void* texture, const CRGBA* colour, const CVector2D* uv1, const CVector2D* uv2);
GAME_FN(void, __cdecl, DrawQuadColours, 0x0052C3B0, float x0, float y0, float x1, float y1, float x2, float y2, float x3,
        float y3, const CRGBA* c0, const CRGBA* c1, const CRGBA* c2, const CRGBA* c3);
GAME_FN(void, __cdecl, DrawRibbonQuad, 0x0052C5D0, float x0, float y0, float x1, float y1, float x2, float y2, float x3,
        float y3, const CRGBA* c0, const CRGBA* c1, const CRGBA* c2, const CRGBA* c3);
GAME_FN(void, __cdecl, Im2D_Flush, 0x0052C5C0);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(uint8_t, __cdecl, Pad_JustPressed, 0x00738900, int control, int pad);
// the shapes' (these), where modes apply
GAME_FN(void, __thiscall, Shape_LerpColour_, 0x00557C10, void* shape, const CRGBA* from, const CRGBA* to, float t,
        CRGBA* out);
GAME_FN(int, __thiscall, Shape_FindSegment_, 0x00557D40, void* shape, float t);
GAME_FN(float, __thiscall, Shape_Width_, 0x00557D90, void* shape, float t);
GAME_FN(float, __thiscall, Meter_TotalLength_, 0x00557E40, void* meter);
GAME_FN(void, __thiscall, Meter_ResetDrips_, 0x00557F60, void* meter);
GAME_FN(void, __thiscall, Shape_SetSegment_, 0x00557FB0, void* shape, int index, float length, float x, float y,
        float width, float width2, const CRGBA* colour);
GAME_FN(CVector2D*, __thiscall, Shape_GetPoint_, 0x00558080, void* shape, CVector2D* out, float t);
GAME_FN(CRGBA*, __thiscall, Shape_GetColour_, 0x00558140, void* shape, CRGBA* out, int which, float t);
GAME_FN(void, __thiscall, Shape_DrawRibbon_, 0x005581C0, void* shape, float progress);
GAME_FN(void, __thiscall, Meter_UpdateDrips_, 0x00558C10, void* meter);
GAME_FN(void, __thiscall, Meter_DrawDrips_, 0x00558E70, void* meter);
GAME_FN(void, __thiscall, Shape_DrawStamps_, 0x005595A0, void* shape, float progress);

constexpr double kPi = 3.1415927410125732;  // (float pi, as a double)

// a shape's track at +0x48 (the second width) at t, as the game inlines it
float Width2(uint8_t* shape, float t) {
    int count = At<int32_t>(shape, 0x30);
    int seg = 0;
    for (; seg < count; seg++) {
        float lo = At<float>(shape, 0x34 + seg * 0x28);
        if (lo <= t && t <= At<float>(shape, 0x38 + seg * 0x28)) {
            break;
        }
    }
    if (seg == count || seg == 0) {
        return At<float>(shape, 0x48);
    }
    uint8_t* e = shape + seg * 0x28;
    double f = (double(t) - At<float>(e, 0x34)) / (double(At<float>(e, 0x38)) - At<float>(e, 0x34));
    return float(f * (double(At<float>(e, 0x48)) - At<float>(e, 0x20)) + At<float>(e, 0x20));
}

// the same for the track at +0x44 (the width), inlined
float Width1(uint8_t* shape, float t) {
    int count = At<int32_t>(shape, 0x30);
    int seg = 0;
    for (; seg < count; seg++) {
        float lo = At<float>(shape, 0x34 + seg * 0x28);
        if (lo <= t && t <= At<float>(shape, 0x38 + seg * 0x28)) {
            break;
        }
    }
    if (seg == count || seg == 0) {
        return At<float>(shape, 0x44);
    }
    uint8_t* e = shape + seg * 0x28;
    double f = (double(t) - At<float>(e, 0x34)) / (double(At<float>(e, 0x38)) - At<float>(e, 0x34));
    return float(f * (double(At<float>(e, 0x44)) - At<float>(e, 0x1C)) + At<float>(e, 0x1C));
}

// the stroke's angle at t (turns of pi along the shape's range)
float ShapeAngle(uint8_t* shape, float t) {
    return float(((double(At<float>(shape, 0x2C)) - At<float>(shape, 0x28)) * t + At<float>(shape, 0x28)) * kPi);
}

} // namespace

namespace hudtagging {

// 0x557C10 (the shape's; `self` unused): a colour between two, truncated
void __fastcall Shape_LerpColour(void*, re::Edx, const CRGBA* from, const CRGBA* to, float t, CRGBA* out) {
    out->r = uint8_t(int32_t(double(from->r) + double(t) * double(int32_t(to->r) - int32_t(from->r))));
    out->g = uint8_t(int32_t(double(int32_t(to->g) - int32_t(from->g)) * t + double(from->g)));
    out->b = uint8_t(int32_t(double(int32_t(to->b) - int32_t(from->b)) * t + double(from->b)));
    out->a = uint8_t(int32_t(double(t) * double(int32_t(to->a) - int32_t(from->a)) + double(from->a)));
}

// 0x557D40: the segment t falls in (from <= t <= to), 0 when none
int __fastcall Shape_FindSegment(uint8_t* self, re::Edx, float t) {
    int count = At<int32_t>(self, 0x30);
    for (int i = 0; i < count; i++) {
        if (At<float>(self, 0x34 + i * 0x28) <= t && t <= At<float>(self, 0x38 + i * 0x28)) {
            return i;
        }
    }
    return 0;
}

// 0x557D90: the width at t (across the segment from the last one's)
float __fastcall Shape_Width(uint8_t* self, re::Edx, float t) {
    int seg = Shape_FindSegment_(self, t);
    if (seg == 0) {
        return At<float>(self, 0x44);
    }
    uint8_t* e = self + seg * 0x28;
    double f = (double(t) - At<float>(self, 0x34 + seg * 0x28)) / (double(At<float>(e, 0x38)) - At<float>(e, 0x34));
    return float((double(At<float>(e, 0x44)) - At<float>(e, 0x1C)) * f + At<float>(e, 0x1C));
}

// 0x557DE0 (vf5 SetVisible): hiding while tagging fades it out instead
void __fastcall SetVisible(HUDComponent* self, re::Edx, int visibleArg) {
    uint8_t visible = uint8_t(visibleArg);
    uint8_t* raw = reinterpret_cast<uint8_t*>(self);
    if (raw[0x2848] && !visible) {
        if (self->visible != visible) {
            if (!(0.0f == At<float>(raw, 0x288C))) {
                return;
            }
            At<float>(raw, 0x288C) = 1.0f;
            return;
        }
    } else if (self->visible != 0) {
        self->visible = visible;
        if (!visible) {
            reinterpret_cast<void(__thiscall*)(HUDComponent*)>(static_cast<void*>(self->vtable[7]))(self);
        }
        return;
    }
    if (visible) {
        EmptyFunction(0);
    }
    self->visible = visible;
    if (!visible) {
        reinterpret_cast<void(__thiscall*)(HUDComponent*)>(static_cast<void*>(self->vtable[7]))(self);
    }
}

// 0x557E40: the tag's length sprayed (the strokes done, the current one's part), over
// the total
float __fastcall TotalLength(uint8_t* self, re::Edx) {
    int done = At<int32_t>(self, 0x285C);
    float sum = 0.0f;
    for (int i = 0; i < done; i++) {
        float speed = At<float>(self, 0x2868 + i * 4);
        if (speed != 0.0f) {
            sum = float(200.0 / double(speed) + sum);
        }
    }
    float speed = At<float>(self, 0x2868 + done * 4);
    if (speed != 0.0f) {
        sum = float(200.0 / double(speed) * At<float>(self, 0x284C) + sum);
    }
    if (At<float>(self, 0x2884) != 0.0f) {
        sum = float(double(sum) / At<float>(self, 0x2884));
    }
    return sum;
}

// 0x557F60: the drips cleared (and "done" until the next stroke)
void __fastcall ResetDrips(uint8_t* self, re::Edx) {
    At<int32_t>(self, 0x2E38) = 0;
    int r = GameRand();
    self[0x2E3C] = 0;
    self[0x2888] = 1;
    At<float>(self, 0x2E40) = float(double(r) / 32767.0 * (kPi - 0.0) + 0.0);
    At<float>(self, 0x288C) = 0.0f;
}

// 0x557FB0: a segment of the shape (following the last: from its end, `length` long)
void __fastcall Shape_SetSegment(uint8_t* self, re::Edx, int index, float length, float x, float y, float width,
                                 float width2, const CRGBA* colour) {
    uint8_t* e = self + index * 0x28;
    if (index == 0) {
        At<float>(self, 0x34) = 0.0f;
        At<float>(self, 0x38) = 0.0f;
    } else {
        float from = At<float>(self, 0x10 + index * 0x28);
        At<float>(e, 0x34) = from;
        At<float>(e, 0x38) = from + length;
    }
    At<float>(e, 0x3C) = x;
    At<float>(e, 0x40) = y;
    At<float>(e, 0x44) = width;
    At<float>(e, 0x48) = width2;
    for (int c = 0; c < 4; c++) {
        std::memcpy(e + 0x4C + c * 4, colour, 4);
    }
}

// 0x558080: the point at t (scaled about the screen's middle)
CVector2D* __fastcall Shape_GetPoint(uint8_t* self, re::Edx, CVector2D* out, float t) {
    int seg = Shape_FindSegment_(self, t);
    if (seg == 0) {
        out->x = At<float>(self, 0x3C);
        out->y = At<float>(self, 0x40);
    } else {
        uint8_t* e = self + seg * 0x28;
        float dx = At<float>(e, 0x3C) - At<float>(e, 0x14);
        float dy = At<float>(e, 0x40) - At<float>(e, 0x18);
        float f = float((double(t) - At<float>(e, 0x34)) / (double(At<float>(e, 0x38)) - At<float>(e, 0x34)));
        float ax = dx * f;
        float ay = f * dy;
        float x = At<float>(e, 0x14) + ax;
        float y = At<float>(e, 0x18) + ay;
        out->x = x;
        out->y = y;
    }
    out->x = float((double(out->x) - 320.0) * g_screenScaleX + 320.0);
    return out;
}

// 0x558140: one of the four colours at t
CRGBA* __fastcall Shape_GetColour(uint8_t* self, re::Edx, CRGBA* out, int which, float t) {
    int seg = Shape_FindSegment_(self, t);
    if (seg == 0) {
        std::memcpy(out, self + 0x4C + which * 4, 4);
        return out;
    }
    uint8_t* e = self + seg * 0x28;
    float f = float((double(t) - At<float>(e, 0x34)) / (double(At<float>(e, 0x38)) - At<float>(e, 0x34)));
    int index = which + seg * 10;
    CRGBA result;
    Shape_LerpColour_(self, reinterpret_cast<CRGBA*>(self + 0x24 + index * 4),
                      reinterpret_cast<CRGBA*>(self + 0x4C + index * 4), f, &result);
    *out = result;
    return out;
}

// the ribbon's two edge points at t
struct RibbonEdge {
    CVector2D a, b;
};
RibbonEdge RibbonAt(uint8_t* shape, float t, float sx, float sy) {
    float w2 = Width2(shape, t);
    CVector2D p;
    Shape_GetPoint_(shape, &p, t);
    float w1 = Width1(shape, t);
    float angle = ShapeAngle(shape, t);
    float s = float(std::sin(double(angle)));
    double a = double(w1) - double(w2) * At<float>(shape, 0xC);
    float dxA = float(a * s * sx);
    float c = float(std::cos(double(angle)));
    float dyA = float(-(a * c * sy));
    RibbonEdge edge;
    edge.a = {p.x + dxA, p.y + dyA};
    double b = double(w1) - double(At<float>(shape, 0x1C)) * w2;
    float dxB = float(double(s) * b * sx);
    float dyB = float(-(b * c * sy));
    edge.b = {p.x + dxB, p.y + dyB};
    return edge;
}

// 0x5581C0: the shape as a ribbon, `progress` of it, in steps of quads
void __fastcall Shape_DrawRibbon(uint8_t* self, re::Edx, float progress) {
    int steps = int32_t(double(At<int32_t>(self, 4)) * progress);
    float sx = g_screenScaleX;
    float sy = g_screenScaleY;
    RibbonEdge last = RibbonAt(self, 0.0f, sx, sy);
    CRGBA lastA, lastB;
    Shape_GetColour_(self, &lastA, 0, 0.0f);
    Shape_GetColour_(self, &lastB, 2, 0.0f);
    for (int i = 0; i < steps; i++) {
        float t = float((double(i) + 1.0) / double(At<int32_t>(self, 4)));
        RibbonEdge next = RibbonAt(self, t, sx, sy);
        CRGBA nextA, nextB;
        Shape_GetColour_(self, &nextA, 0, t);
        Shape_GetColour_(self, &nextB, 2, t);
        DrawRibbonQuad(last.a.x, last.a.y, next.a.x, next.a.y, last.b.x, last.b.y, next.b.x, next.b.y, &lastA, &nextA,
                       &lastB, &nextB);
        last = next;
        lastA = nextA;
        lastB = nextB;
    }
}

// 0x558750: a tag started: its strokes' shapes (the paint and two shadows) from the
// tag's table, the drips reset
void __fastcall StartTag(uint8_t* self, re::Edx, int tag) {
    Meter_ResetDrips_(self);
    At<float>(self, 0x2860) = 0.0f;
    At<float>(self, 0x284C) = 0.0f;
    At<float>(self, 0x2850) = 0.5f;
    At<int32_t>(self, 0x2854) = tag;
    At<int32_t>(self, 0x285C) = 0;
    At<int32_t>(self, 0x2864) = 7;
    int32_t button = *reinterpret_cast<int32_t*>(g_tagStrokes + tag * 0x118 + 0x24);
    if (button != -1) {
        At<int32_t>(self, 0x2864) = button;
        At<float>(self, 0x2890) = 2.0f;
        self[0x2888] = 1;
    }
    CRGBA dark, grey;
    RGBA_Construct(&dark, 0x10, 0x10, 0x10, 0x40);
    RGBA_Construct(&grey, 0x40, 0x40, 0x40, 0x40);
    float ox = At<float>(self, 0x28);
    float oy = At<float>(self, 0x2C);
    auto setUp = [](uint8_t* shape, int steps, void* texture, const float* stroke) {
        At<int32_t>(shape, 0x1E4) = 0;
        At<void*>(shape, 0x1DC) = texture;
        At<float>(shape, 0x08) = -10.0f;
        At<float>(shape, 0x0C) = -10.0f;
        At<float>(shape, 0x14) = -10.0f;
        At<float>(shape, 0x10) = 10.0f;
        At<float>(shape, 0x18) = -10.0f;
        At<float>(shape, 0x1C) = 10.0f;
        At<float>(shape, 0x20) = 10.0f;
        At<float>(shape, 0x24) = 10.0f;
        At<int32_t>(shape, 0x04) = steps;
        At<float>(shape, 0x28) = stroke[4];
        At<float>(shape, 0x2C) = stroke[5];
        At<int32_t>(shape, 0x30) = 2;
    };
    for (int i = 0; i < g_tagStrokeCounts[tag]; i++) {
        const float* stroke = reinterpret_cast<const float*>(Stroke(tag, i));  // [0..3] the ends, [4..5] the angles, [6..7] widths
        uint8_t* paint = self + 0x1AF0 + i * 0x1E8;
        setUp(paint, 0x20, g_paintTexture, stroke);
        Shape_SetSegment_(paint, 0, 0.0f, float(double(stroke[0]) * 0.5 + ox), float(double(stroke[1]) * 0.5 + oy),
                          float(0.5 * stroke[6]), 1.0f, &g_paintColour);
        Shape_SetSegment_(paint, 1, 1.0f, float(double(stroke[2]) * 0.5 + ox), float(double(stroke[3]) * 0.5 + oy),
                          float(0.5 * stroke[7]), 1.0f, &g_paintColour);
        uint8_t* shadow = self + 0x40 + i * 0x1E8;
        setUp(shadow, 0x7F, g_stampTexture, stroke);
        Shape_SetSegment_(shadow, 0, 0.0f, float(double(stroke[0]) * 0.5 + ox), float(double(stroke[1]) * 0.5 + oy),
                          float(0.5 * stroke[6]), 0.475f, &dark);
        Shape_SetSegment_(shadow, 1, 1.0f, float(double(stroke[2]) * 0.5 + ox), float(double(stroke[3]) * 0.5 + oy),
                          float(0.5 * stroke[7]), 0.475f, &dark);
        uint8_t* shadow2 = self + 0xD98 + i * 0x1E8;
        setUp(shadow2, 0x7F, g_stampTexture, stroke);
        Shape_SetSegment_(shadow2, 0, 0.0f, float(double(stroke[0]) * 0.5 + ox), float(double(stroke[1]) * 0.5 + oy),
                          float(0.5 * stroke[6]), 0.35f, &grey);
        Shape_SetSegment_(shadow2, 1, 1.0f, float(double(stroke[2]) * 0.5 + ox), float(double(stroke[3]) * 0.5 + oy),
                          float(0.5 * stroke[7]), 0.35f, &grey);
    }
    self[0x2848] = 1;
}

// 0x558C10: the drips: one grows at a time (while the stroke pauses), new ones start
// every so far along the tag
void __fastcall UpdateDrips(uint8_t* self, re::Edx) {
    if (self[0x2888]) {
        return;
    }
    float spacing = float(8.0 / double(At<float>(self, 0x2884)));
    float total = Meter_TotalLength_(self);
    int32_t& count = At<int32_t>(self, 0x2E38);
    float& angle = At<float>(self, 0x2E40);
    auto drip = [self](int i, int field) -> float& { return At<float>(self, 0x2890 + i * 0xC + field); };
    if (self[0x2E3C]) {
        drip(count, 0) = float(double(g_frameSeconds) * g_timeStep) + drip(count, 0);
        if (!(0.0f == At<float>(self, 0x2850))) {
            self[0x2E3C] = 0;
        }
        return;
    }
    if (count >= 0x78) {
        return;
    }
    auto sizeFromAngle = [](float a) {
        float s = float(std::sin(double(a)));
        return float((double(s) + 1.0) * 0.5 * 0.19999998807907104 + 0.800000011920929);
    };
    if (0.0f == At<float>(self, 0x2850)) {
        drip(count, 8) = total;   // (+0x2898: where it hangs)
        drip(count, 0xC) = 0.0f;  // (+0x289C: its length)
        angle = float(double(GetRandomNumberInRange(0.0625f, 0.5f)) * kPi + angle);
        drip(count, 0x10) = sizeFromAngle(angle);
        count++;
        self[0x2E3C] = 1;
        return;
    }
    for (;;) {
        int n = count;
        if (n != 0) {
            if (n <= 0) {
                return;
            }
            float next = float(double(drip(n - 1, 8)) + spacing);
            if (!(next < total)) {
                return;
            }
        }
        if (n >= 0x78) {
            return;
        }
        if (n == 0) {
            drip(0, 8) = 0.0f;
        } else {
            drip(n, 8) = float(double(spacing) + drip(n - 1, 8));
        }
        drip(count, 0xC) = 0.08f;
        int r = GameRand();
        float turn = float(double(r) / 32767.0 * 0.4375 + 0.0625);
        angle = float(double(turn) * kPi + angle);
        drip(count, 0x10) = sizeFromAngle(angle);
        count++;
    }
}

// 0x558E70: the drips drawn: each hangs off the paint at its place, a column of blots
// down its length (then smaller ones below 0.4)
void __fastcall DrawDrips(uint8_t* self, re::Edx) {
    float sx = g_screenScaleX;
    float sy = g_screenScaleY;
    for (int d = 0; d < At<int32_t>(self, 0x2E38); d++) {
        uint8_t* drip = self + 0x2890 + d * 0xC;
        int strokes = g_tagStrokeCounts[At<int32_t>(self, 0x2854)];
        float rem = At<float>(drip, 8) * At<float>(self, 0x2884);
        void* texture = g_paintTexture;
        int i = 0;
        for (; i < strokes; i++) {
            double length = 200.0 / double(At<float>(self, 0x2868 + i * 4));
            if (rem <= float(length)) {
                break;
            }
            rem = float(double(rem) - length);
        }
        float f = float(double(rem) * At<float>(self, 0x2868 + i * 4) / 200.0);
        uint8_t* shape = self + 0x1AF0 + i * 0x1E8;
        CVector2D p;
        Shape_GetPoint_(shape, &p, f);
        float angle = ShapeAngle(shape, f);
        float r = Width1(shape, f);
        float s = float(std::sin(double(angle)));
        p.x = float(double(s) * r * sx + p.x);
        double py = p.y;
        float c = float(std::cos(double(angle)));
        CRGBA colour = g_paintColour;
        p.y = float(py - double(c) * r * sy);
        float size = At<float>(drip, 0x10);
        if (!(0.0f == At<float>(self, 0x288C))) {
            float u = float(1.0 - double(At<float>(self, 0x288C)));
            colour.a = uint8_t(int32_t((1.0 - double(u) * u) * 255.0));
        }
        float length = At<float>(drip, 0xC);
        CVector2D uv2 = {1.0f, 1.0f};
        CVector2D uv1 = {0.0f, 0.0f};
        for (float j = 0.0f; j < length;) {
            double hh = double(sy) * 16.0 * size;
            double hw = double(size) * (16.0 * sx);
            DrawStamp(float(p.x - hw), float(p.y - hh), float(p.x + hw), float(p.y + hh), texture, &colour, &uv1, &uv2);
            j = float(double(j) + 0.07999999821186066);
            if (!(double(j) < 0.4000000059604645)) {
                break;
            }
        }
        if (!(double(length) < 0.4000000059604645)) {
            p.y = float(double(sy) * 12.0 * size + p.y);
            for (float j = 0.4f; j < length;) {
                double hh = double(sy) * 6.400000095367432 * size;
                double hw = double(size) * (6.400000095367432 * sx);
                DrawStamp(float(p.x - hw), float(p.y - hh), float(p.x + hw), float(p.y + hh), texture, &colour, &uv1,
                          &uv2);
                p.y = float(double(sy) * size + p.y);
                j = float(double(j) + 0.20000000298023224);
                if (!(j < 3.0f)) {
                    break;
                }
            }
        }
    }
    Im2D_Flush();
}

// 0x5593A0: a shape: 0x80 steps, the default corners and angles, two segments
void __fastcall Shape_Construct(uint8_t* self, re::Edx) {
    At<float>(self, 0) = 0.0f;
    At<int32_t>(self, 4) = 0x80;
    At<float>(self, 0x08) = -10.0f;
    At<float>(self, 0x0C) = -10.0f;
    At<float>(self, 0x10) = 10.0f;
    At<float>(self, 0x14) = -10.0f;
    At<float>(self, 0x18) = -10.0f;
    At<float>(self, 0x1C) = 10.0f;
    At<float>(self, 0x20) = 10.0f;
    At<float>(self, 0x24) = 10.0f;
    At<int32_t>(self, 0x1E4) = 1;
    At<int32_t>(self, 0x30) = 2;
    At<float>(self, 0x28) = -0.25f;
    At<float>(self, 0x2C) = 0.5f;
    CRGBA colour;
    RGBA_Construct(&colour, 0, 0xEB, 0, 100);
    Shape_SetSegment_(self, 0, 0.0f, 80.0f, 370.0f, 50.0f, 0.5f, &colour);
    RGBA_Construct(&colour, 0xEB, 0, 0, 100);
    Shape_SetSegment_(self, 1, 1.0f, 80.0f, 370.0f, 100.0f, 1.0f, &colour);
    At<float>(self, 0x1C4) = 0.5f;
    At<float>(self, 0x1C8) = 0.0f;
    CRGBA clear, yellow;
    RGBA_Construct(&clear, 0xEB, 0xEB, 0, 0);
    RGBA_Construct(&yellow, 0xEB, 0xEB, 0, 100);
    std::memcpy(self + 0x1CC, &yellow, 4);
    std::memcpy(self + 0x1D0, &clear, 4);
    At<float>(self, 0x1D4) = 1.0f;
    RGBA_Construct(&yellow, 0xEB, 0xEB, 0, 100);
    std::memcpy(self + 0x1D8, &yellow, 4);
    At<int32_t>(self, 0x1E0) = 0;
    At<int32_t>(self, 0x1DC) = 0;
}

// 0x5595A0: the shape as stamps (its texture, else a quad in its four colours) along
// `progress` of it; its length drawn kept at +0x1E0
void __fastcall Shape_DrawStamps(uint8_t* self, re::Edx, float progress) {
    int steps = int32_t(double(At<int32_t>(self, 4)) * progress);
    float sx = g_screenScaleX;
    float sy = g_screenScaleY;
    At<float>(self, 0x1E0) = 0.0f;
    CVector2D last = {0.0f, 0.0f};  // (the game's is uninitialised: not read on the first step)
    for (int i = 0; i < steps; i++) {
        float t = float(double(i) / double(At<int32_t>(self, 4)));
        float w = Width2(self, t);
        CVector2D p;
        Shape_GetPoint_(self, &p, t);
        float v = Width1(self, t);
        CVector2D q = p;
        float angle = ShapeAngle(self, t);
        float s = float(std::sin(double(angle)));
        double sv = double(s) * v;
        q.x = float(sv + p.x);
        float c = float(std::cos(double(angle)));
        double cv = double(c) * v;
        q.y = float(double(q.y) - cv);
        if (i != 0) {
            float dx = q.x - last.x;
            float dy = q.y - last.y;
            float lengthSq = float(double(dy) * dy + double(dx) * dx);
            lengthSq = std::fabs(lengthSq);
            float length = float(std::sqrt(double(lengthSq)));
            At<float>(self, 0x1E0) = length + At<float>(self, 0x1E0);
        }
        last = q;
        p.x = float(double(sx) * sv + p.x);
        p.y = float(double(p.y) - double(sy) * cv);
        float x0 = float(double(w) * At<float>(self, 0x08) * sx + p.x);
        float y0 = float(double(At<float>(self, 0x0C)) * w * sy + p.y);
        float x1 = float(double(w) * At<float>(self, 0x10) * sx + p.x);
        float y1 = float(double(At<float>(self, 0x14)) * w * sy + p.y);
        float x2 = float(double(At<float>(self, 0x18)) * w * sx + p.x);
        float y2 = float(double(At<float>(self, 0x1C)) * w * sy + p.y);
        float x3 = float(double(w) * At<float>(self, 0x20) * sx + p.x);
        float y3 = float(double(w) * At<float>(self, 0x24) * sy + p.y);
        void* texture = At<void*>(self, 0x1DC);
        if (texture) {
            CVector2D uv2 = {1.0f, 1.0f};
            CVector2D uv1 = {0.0f, 0.0f};
            CRGBA colour;
            Shape_GetColour_(self, &colour, 0, t);
            DrawQuadTextured(x0, y0, x1, y1, x2, y2, x3, y3, texture, &colour, &uv1, &uv2);
        } else {
            CRGBA c3, c2, c1, c0;
            Shape_GetColour_(self, &c3, 3, t);
            Shape_GetColour_(self, &c2, 2, t);
            Shape_GetColour_(self, &c1, 1, t);
            Shape_GetColour_(self, &c0, 0, t);
            DrawQuadColours(x0, y0, x1, y1, x2, y2, x3, y3, &c0, &c1, &c2, &c3);
        }
    }
    Im2D_Flush();
}

// the prompt / pen icon at a point of the stroke, `size` (the half size's scale)
void DrawIconAt(void* texture, float px, float py, float half, float sx, float sy) {
    CVector2D uv1 = {0.0f, 0.0f};
    CVector2D uv2 = {1.0f, 1.0f};
    double q = double(half) * 32.0 * 0.5;
    double hh = double(sy) * q;
    double hw = q * sx;
    DrawRectUV(float(px - hw), float(py - hh), float(px + hw), float(py + hh), texture, &g_buttonColour, &uv1, &uv2);
}

// 0x559B10 (vf8 Draw): the tag so far (the two shadows, the paint by stroke), the
// drips; then the next stroke's button prompt (missed: the tag's given up), or the
// pen following the stick round the stroke (on its line, the stroke moves on); the
// weapon HUD back when it's done. Fading out once finished
void __fastcall Draw(HUDComponent* self, re::Edx) {
    uint8_t* raw = reinterpret_cast<uint8_t*>(self);
    auto finish = [raw]() {
        At<float>(raw, 0x288C) = 1.0f;
        reinterpret_cast<void(__thiscall*)(HUDComponent*, int)>(static_cast<void*>(g_hudWeapons->vtable[5]))(g_hudWeapons, 1);
    };
    if (!(0.0f == At<float>(raw, 0x288C))) {
        Meter_DrawDrips_(raw);
        float fade = float(double(At<float>(raw, 0x288C)) - float(double(g_frameSeconds) * g_timeStep));
        At<float>(raw, 0x288C) = fade;
        if (0.0f < fade) {
            return;
        }
        At<float>(raw, 0x288C) = 0.0f;
        raw[0x2848] = 0;
        reinterpret_cast<void(__thiscall*)(HUDComponent*, int)>(static_cast<void*>(self->vtable[5]))(self, 0);
        return;
    }
    if (g_padConnected == 0 || Pad_JustPressed(9, 0)) {
        finish();
        return;
    }
    int tag = At<int32_t>(raw, 0x2854);
    At<float>(raw, 0x2884) = 0.0f;
    for (int i = 0; i < g_tagStrokeCounts[tag]; i++) {
        uint8_t* shape = raw + 0x40 + i * 0x1E8;
        At<float>(shape, 0) = 1.0f;
        int kind = At<int32_t>(shape, 0x1E4);
        if (kind != 1) {
            if (kind == 2) {
                Shape_DrawRibbon_(shape, 1.0f);
            } else {
                Shape_DrawStamps_(shape, 1.0f);
            }
        }
        At<float>(raw, 0x2868 + i * 4) = float(200.0 / double(At<float>(shape, 0x1E0)));
        At<float>(raw, 0x2884) = At<float>(shape, 0x1E0) + At<float>(raw, 0x2884);
        tag = At<int32_t>(raw, 0x2854);
    }
    tag = At<int32_t>(raw, 0x2854);
    for (int i = 0; i < g_tagStrokeCounts[tag]; i++) {
        uint8_t* shape = raw + 0xD98 + i * 0x1E8;
        At<float>(shape, 0) = 1.0f;
        int kind = At<int32_t>(shape, 0x1E4);
        if (kind != 1) {
            if (kind == 2) {
                Shape_DrawRibbon_(shape, 1.0f);
            } else {
                Shape_DrawStamps_(shape, 1.0f);
            }
        }
        tag = At<int32_t>(raw, 0x2854);
    }
    float sx = g_screenScaleX;
    float sy = g_screenScaleY;
    Meter_DrawDrips_(raw);
    int stroke = At<int32_t>(raw, 0x285C);
    float& progress = At<float>(raw, 0x284C);
    auto strokeButton = [](int t, int s) { return *reinterpret_cast<int32_t*>(g_tagStrokes + (t * 7 + s) * 0x28 + 0x24); };
    if (0.0f == progress && strokeButton(At<int32_t>(raw, 0x2854), stroke) != -1) {
        float timer = float(double(At<float>(raw, 0x2890)) - float(double(g_frameSeconds) * g_timeStep));
        At<float>(raw, 0x2890) = timer;
        if (timer <= 0.0f) {
            At<float>(raw, 0x2890) = 0.0f;
            raw[0x2888] = 0;
            finish();
            return;
        }
        uint8_t* shape = raw + 0x1AF0 + stroke * 0x1E8;
        raw[0x2888] = 1;
        CVector2D p;
        Shape_GetPoint_(shape, &p, 0.0f);
        float angle = ShapeAngle(shape, 0.0f);
        float w = Shape_Width_(shape, 0.0f);
        float s = float(std::sin(double(angle)));
        p.x = float(double(s) * w * sx + p.x);
        double py = p.y;
        float c = float(std::cos(double(angle)));
        int button = At<int32_t>(raw, 0x2864);
        p.y = float(py - double(c) * w * sy);
        float half = float(double(At<float>(raw, 0x2890)) * 0.5);
        DrawIconAt(GetButtonTexture(button), p.x, p.y, half, sx, sy);
        if (Pad_JustPressed(At<int32_t>(raw, 0x2864), 0)) {
            raw[0x2888] = 0;
            progress = float(double(At<float>(raw, 0x2868 + At<int32_t>(raw, 0x285C) * 4)) * 9.999999747378752e-05);
            At<float>(raw, 0x2890) = 0.0f;
            At<float>(raw, 0x2894) = 3.0f;
        }
        return;
    }
    float hold = float(double(At<float>(raw, 0x2894)) - float(double(g_frameSeconds) * g_timeStep));
    raw[0x2888] = 0;
    At<float>(raw, 0x2894) = hold;
    if (hold <= 0.0f) {
        At<float>(raw, 0x2894) = 0.0f;
        finish();
        return;
    }
    At<float>(raw, 0x2890) = 0.0f;
    uint8_t* shape = raw + 0x1AF0 + stroke * 0x1E8;
    CVector2D p;
    Shape_GetPoint_(shape, &p, progress);
    float angle = ShapeAngle(shape, progress);
    float t = progress;
    float w;
    int seg = Shape_FindSegment_(shape, t);
    if (seg == 0) {
        w = At<float>(shape, 0x44);
    } else {
        uint8_t* e = shape + seg * 0x28;
        double f = (double(t) - At<float>(e, 0x34)) / (double(At<float>(e, 0x38)) - At<float>(e, 0x34));
        w = float(f * (double(At<float>(e, 0x44)) - At<float>(e, 0x1C)) + At<float>(e, 0x1C));
    }
    float s = float(std::sin(double(angle)));
    p.x = float(double(s) * w * sx + p.x);
    double py = p.y;
    float c = float(std::cos(double(angle)));
    p.y = float(py - double(c) * w * sy);
    float scale = 1.0f;
    if (2.0f > At<float>(raw, 0x2894)) {
        scale = float(double(At<float>(raw, 0x2894)) * 0.5);
    }
    DrawIconAt(GetButtonTexture(0x19), p.x, p.y, scale, sx, sy);
    // aiming: the stick's direction against the stroke's (a quarter turn off it, by the
    // stroke's side), pushed past half way
    At<float>(raw, 0x2850) = 0.0f;
    float stickX = Pad_GetValue(0x10, 0);
    float stickY = Pad_GetValue(0x11, 0);
    float magSq = float(double(stickY) * stickY + double(stickX) * stickX);
    magSq = std::fabs(magSq);
    float mag = float(std::sqrt(double(magSq)));
    if (0.5f < mag) {
        At<float>(raw, 0x2894) = 3.0f;
        float stickAngle = float(std::atan2(-double(stickX), double(stickY)));
        int strokeNow = At<int32_t>(raw, 0x285C);
        bool side = g_tagStrokes[(At<int32_t>(raw, 0x2854) * 7 + strokeNow) * 0x28 + 0x20] != 0;
        double turn = side ? -1.5707963705062866 : 1.5707963705062866;
        uint8_t* strokeShape = raw + 0x1AF0 + strokeNow * 0x1E8;
        float along = float(((double(At<float>(strokeShape, 0x2C)) - At<float>(strokeShape, 0x28)) * progress +
                             At<float>(strokeShape, 0x28)) *
                            kPi);
        float want = float(double(along) + turn);
        while (want > 3.1415927f) {
            want = float(double(want) - 6.2831854820251465);
        }
        while (want < -3.1415927f) {
            want = float(double(want) + 6.2831854820251465);
        }
        float off = std::fabs(float(double(stickAngle) - want));
        if (!(off <= 3.1415927f)) {
            off = float(-(double(off) - 6.2831854820251465));
        }
        if (g_aimTolerance > off) {
            float push = float((double(mag) - 0.5) * 2.0);
            float close = float(1.0 - double(off) / g_aimTolerance);
            At<float>(raw, 0x2850) = float(double(close) * close * (double(push) * push) + At<float>(raw, 0x2850));
        }
    }
    float speed = float(double(At<float>(raw, 0x2868 + At<int32_t>(raw, 0x285C) * 4)) * At<float>(raw, 0x2850));
    At<float>(raw, 0x2850) = speed;
    float step = float(double(g_frameSeconds) * g_timeStep);
    float next = float(double(speed) * step + progress);
    progress = next;
    if (1.0f <= next) {
        progress = 1.0f;
    }
    Meter_UpdateDrips_(raw);
    if (!(1.0f <= progress)) {
        return;
    }
    At<int32_t>(raw, 0x285C) += 1;
    progress = 0.0f;
    int done = At<int32_t>(raw, 0x285C);
    if (done >= g_tagStrokeCounts[At<int32_t>(raw, 0x2854)]) {
        At<float>(raw, 0x288C) = 1.0f;
        reinterpret_cast<void(__thiscall*)(HUDComponent*, int)>(static_cast<void*>(g_hudWeapons->vtable[5]))(g_hudWeapons, 1);
        return;
    }
    int button = strokeButton(At<int32_t>(raw, 0x2854), done);
    if (button == -1) {
        return;
    }
    At<int32_t>(raw, 0x2864) = button;
    At<float>(raw, 0x2890) = 2.0f;
    raw[0x2888] = 1;
}

} // namespace hudtagging

#define HUDTAGGING_REVERSED(name, address, fn)                                                       static void* RE_CONCAT(hudtagging_original_, __LINE__);                                           REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(hudtagging_original_, __LINE__),              re::Mode::Original)

HUDTAGGING_REVERSED("TagShape::LerpColour", 0x00557C10, &hudtagging::Shape_LerpColour);
HUDTAGGING_REVERSED("TagShape::FindSegment", 0x00557D40, &hudtagging::Shape_FindSegment);
HUDTAGGING_REVERSED("TagShape::Width", 0x00557D90, &hudtagging::Shape_Width);
HUDTAGGING_REVERSED("HUDTaggingMeter::SetVisible", 0x00557DE0, &hudtagging::SetVisible);
HUDTAGGING_REVERSED("HUDTaggingMeter::TotalLength", 0x00557E40, &hudtagging::TotalLength);
HUDTAGGING_REVERSED("HUDTaggingMeter::ResetDrips", 0x00557F60, &hudtagging::ResetDrips);
HUDTAGGING_REVERSED("TagShape::SetSegment", 0x00557FB0, &hudtagging::Shape_SetSegment);
HUDTAGGING_REVERSED("TagShape::GetPoint", 0x00558080, &hudtagging::Shape_GetPoint);
HUDTAGGING_REVERSED("TagShape::GetColour", 0x00558140, &hudtagging::Shape_GetColour);
HUDTAGGING_REVERSED("TagShape::DrawRibbon", 0x005581C0, &hudtagging::Shape_DrawRibbon);
HUDTAGGING_REVERSED("HUDTaggingMeter::StartTag", 0x00558750, &hudtagging::StartTag);
HUDTAGGING_REVERSED("HUDTaggingMeter::UpdateDrips", 0x00558C10, &hudtagging::UpdateDrips);
HUDTAGGING_REVERSED("HUDTaggingMeter::DrawDrips", 0x00558E70, &hudtagging::DrawDrips);
HUDTAGGING_REVERSED("TagShape::TagShape", 0x005593A0, &hudtagging::Shape_Construct);
HUDTAGGING_REVERSED("TagShape::DrawStamps", 0x005595A0, &hudtagging::Shape_DrawStamps);
HUDTAGGING_REVERSED("HUDTaggingMeter::Draw", 0x00559B10, &hudtagging::Draw);
