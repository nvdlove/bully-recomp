// The secondary health bar (HUD_SECONDARY_BAR): up to two peds' health (or a prop's)
// as bars that flash when hit and pulse when low, a capsule-shaped variant with the
// damage shown, the player's (Jimmy's) own bar beside it, their labels; the colour
// ramp they share, and the small script text's Draw. 0x551530..0x553B80 (its
// constructor, SetVisible and labels are Hud.cpp's).
//
// The bar, by offset (Hud.h's HUDSecondaryHealthBar has it unknown):
//   +0x20 / +0x28 the labels' x (and y), +0x30.. sizes      +0x40 / +0x50 the two bars' rects
//   +0x60 / +0x64 the full / empty colours                   +0x70 / +0x78 the labels' positions
//   +0x80.. the capsule's label, +0x88.. its Jimmy bar (x, y, w, h), +0x98.. the capsule (x, y, w, h)
//   +0xA8 the capsule's outline colour                        +0xAC / +0xB0 the peds, +0xB4 a prop
//   +0xB8 / +0xB9 labels shown, +0xBA capsule, +0xBB flipped (with Jimmy's), +0xBD no flash
//   +0xBE / +0xFE the labels' text keys                       +0x140 / +0x144 the last health
//   +0x148 / +0x14C hit flash, +0x150 / +0x154 low pulse     +0x158 damage fade, +0x15C damage
//   +0x160.. the Jimmy bar's (fade, flash, last, damage, pulse) +0x174 a fixed value shown
//   +0x178 / +0x17C its max / value

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>

#include "game/Collision.h"
#include "game/Hud.h"
#include "game/VCall.h"
#include "reversed.h"

namespace {

using vc::At;

struct CVector2D {
    float x, y;
};

GAME_VAR(uint8_t*, g_playerPed, 0x00C1AEA8);
GAME_VAR(float, g_screenScaleX, 0x00A957A8);
GAME_VAR(float, g_screenScaleY, 0x00A957AC);
GAME_VAR(float, g_timeStep, 0x00C1A9A4);
GAME_VAR(float, g_frameSeconds, 0x00AB74D0);  // (0.02: the step's seconds)
GAME_VAR(uint32_t, g_timeInMilliseconds, 0x00C1A9B4);
GAME_VAR(int32_t, g_language, 0x00C18724);    // 7: the bigger text
GAME_VAR(CRGBA, g_flashFrom, 0x00CF0A10);       // the outline, blended to...
GAME_VAR(CRGBA, g_flashHit, 0x00CF0A14);        // ...when hit
GAME_VAR(CRGBA, g_flashLow, 0x00CF0A18);        // ...when low
GAME_VAR(CRGBA, g_barBack, 0x00CF0A1C);
GAME_VAR(CRGBA, g_labelColour, 0x00CF0A2C);
GAME_VAR(float, g_labelOffsetY, 0x00CF0A08);
GAME_VAR(float, g_labelOffsetX, 0x00CF0A0C);
GAME_VAR(uint32_t, g_fontColour, 0x0208B764);
GAME_VAR(void*, g_hudText, 0x00C67548);  // the HUDText
inline void* const kTheText = reinterpret_cast<void*>(0x020C2FA4);

GAME_FN(CRGBA*, __cdecl, LerpColour_, 0x00551530, CRGBA* out, float t, CRGBA from, CRGBA to);
GAME_FN(CRGBA*, __thiscall, RGBA_Construct, 0x00510DE0, CRGBA* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, DrawRect, 0x0052C470, const col::CRect* rect, const CRGBA* colour);
GAME_FN(void, __cdecl, DrawRing, 0x0052D390, const CVector2D* centre, float outer, float inner, const CRGBA* colour,
        int segments, float start, float end);
GAME_FN(void, __cdecl, DrawSector, 0x0052D200, const CVector2D* centre, float radius, const CRGBA* colour, int segments,
        float start, float end);
GAME_FN(CVector2D*, __cdecl, ScreenPosition, 0x0052D4F0, CVector2D* out, float x, float y, int hAlign, int vAlign);
GAME_FN(col::CRect*, __cdecl, ScreenRect, 0x0052D650, col::CRect* out, float x, float y, float width, float height,
        int hAlign, int vAlign);
GAME_FN(void, __thiscall, HUDComponent_SetAlignment, 0x0053CE00, HUDComponent* self, const char* h, const char* v);
GAME_FN(void, __thiscall, HUDComponent_Reset, 0x0053CF00, HUDComponent* self);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, re::Ptr32<void>* ref);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, void* entity, re::Ptr32<void>* ref);
GAME_FN(float, __thiscall, PropAnim_GetHealthFraction, 0x004EDD80, void* prop);
GAME_FN(const uint16_t*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __cdecl, Font_Begin, 0x00688E60);
GAME_FN(void, __cdecl, Font_SetSizePreset, 0x00688050, int preset);
GAME_FN(void, __cdecl, Font_SetScale, 0x00688080, float scale);
GAME_FN(void, __cdecl, Font_SetScaleXY, 0x006880B0, float x, float y);
GAME_FN(void, __cdecl, Font_SetJustify, 0x00688160, int justify);
GAME_FN(void, __cdecl, Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __thiscall, SecondaryBar_DrawJimmy_, 0x005515F0, uint8_t* self);
// the text component's (Hud.cpp's)
GAME_FN(void, __thiscall, HUDText_AddFloat, 0x0055B020, void* text, int slot, float value);
GAME_FN(void, __thiscall, HUDText_AddString, 0x0055B070, void* text, int slot, const void* string);
GAME_FN(void, __thiscall, HUDText_AddText, 0x0055B0D0, void* text, int slot, const void* string);
GAME_FN(void, __thiscall, HUDText_SetText, 0x0055B130, void* text, int slot, const char* key);
GAME_FN(void, __thiscall, HUDScriptText_Clear, 0x005539D0, HUDComponent* self);

constexpr double kPi = 3.1415927410125732;          // (float pi, as a double)
constexpr float kPiF = 3.1415927f;
constexpr float kTwoPiF = 6.2831855f;
constexpr double kPointFour = 0.4000000059604645;   // double(0.4f)
constexpr double kPointTwo = 0.20000000298023224;   // double(0.2f)
constexpr double kPointEightA = 0.800000011920929;  // double(0.8f)
constexpr double kPointEightB = 0.7999999970197678; // (a double constant of its own)

float HealthFraction(const uint8_t* ped) {
    return *reinterpret_cast<const float*>(ped + 0x1CB8) / *reinterpret_cast<const float*>(ped + 0x1CA4);
}

// the flash's strength from its timer (t running down from `length`): in over the
// first fifth, out over the rest
float FlashCurve(float timer, double length) {
    float u = float(1.0 - double(timer) / length);
    if (kPointTwo > double(u)) {
        float v = float(double(u) / kPointTwo);
        return float(double(v) * v);
    }
    float w = float((double(u) - kPointTwo) / kPointEightB);
    return float(1.0 - double(w) * w);
}

// a timer run down by the frame (0 once it's through)
void RunDown(float& timer) {
    float step = float(double(g_frameSeconds) * g_timeStep);
    timer = float(double(timer) - step);
    if (timer <= 0.0f) {
        timer = 0.0f;
    }
}

// `from` + (`to` - `from`) * t per channel, truncated, in bytes
CRGBA BlendTruncated(CRGBA from, const uint8_t to[4], float t) {
    CRGBA out = from;
    out.r = uint8_t(from.r + uint8_t(int32_t(double(t) * double(int32_t(to[0]) - int32_t(from.r)))));
    out.g = uint8_t(from.g + uint8_t(int32_t(double(int32_t(to[1]) - int32_t(from.g)) * t)));
    out.b = uint8_t(from.b + uint8_t(int32_t(double(int32_t(to[2]) - int32_t(from.b)) * t)));
    out.a = uint8_t(from.a + uint8_t(int32_t(double(t) * double(int32_t(to[3]) - int32_t(from.a)))));
    return out;
}

// the damage colour faded towards the bar's: a * m + b * (1 - m), truncated (alpha 0xFF)
CRGBA FadeTruncated(CRGBA bar, CRGBA damage, float m) {
    double rest = 1.0 - double(m);
    CRGBA out;
    out.r = uint8_t(int32_t(double(bar.r) * m + double(damage.r) * rest));
    out.g = uint8_t(int32_t(double(bar.g) * m + double(damage.g) * rest));
    out.b = uint8_t(int32_t(double(bar.b) * m + double(damage.b) * rest));
    out.a = 0xFF;
    return out;
}

} // namespace

namespace hudsecondarybar {

// 0x551530: a health colour: `full` over 2/3, yellow (0xD2, 0xD2, 0x0A) over 1/3 (with
// full's alpha), `empty` below
CRGBA* __cdecl LerpColour(CRGBA* out, float t, CRGBA full, CRGBA empty) {
    if (0.6666667f < t) {
        *out = full;
        return out;
    }
    if (0.33333334f < t) {
        RGBA_Construct(out, 0xD2, 0xD2, 10, full.a);
        return out;
    }
    *out = empty;
    return out;
}

// 0x5515F0: the player's (Jimmy's) bar beside the flipped capsule, with its damage
// and the "N_JIMMY" label
void __fastcall DrawJimmy(uint8_t* self, re::Edx) {
    uint8_t* player = g_playerPed;
    if (!player) {
        return;
    }
    float r = HealthFraction(player);
    if (r < 0.0f) {
        r = 0.0f;
    } else if (1.0f < r) {
        r = 1.0f;
    }
    float sx = g_screenScaleX;
    float sy = g_screenScaleY;
    if (r <= 0.0f) {
        return;
    }
    if (double(r) < 0.05000000074505806) {
        r = 0.05f;
    } else if (1.0f < r) {
        r = 1.0f;
    }
    if (At<float>(self, 0x164) > r) {
        At<float>(self, 0x148) = 0.4f;
    }
    At<float>(self, 0x168) = float(double(At<float>(self, 0x164)) - r + At<float>(self, 0x168));
    At<float>(self, 0x164) = r;
    if (0.25f > r && !(0.0f < At<float>(self, 0x170))) {
        At<float>(self, 0x170) = float(4.0 * r * kPointFour + kPointFour);
    }
    CRGBA bar;
    bar = *LerpColour_(&bar, r, At<CRGBA>(self, 0x60), At<CRGBA>(self, 0x64));
    if (0.0f < At<float>(self, 0x16C)) {
        RunDown(At<float>(self, 0x16C));
    } else {
        At<float>(self, 0x168) = float(-double(At<float>(self, 0x168)) * 0.44999998807907104 + At<float>(self, 0x168));
    }
    if (0.0f < At<float>(self, 0x170)) {
        RunDown(At<float>(self, 0x170));
    }
    // the capsule's outline, moved left for the Jimmy bar
    double shift = double(sx) * 160.0;
    float x = float(At<float>(self, 0x98) + shift);
    float y = float(double(At<float>(self, 0x9C)) + 0.0);
    float w = At<float>(self, 0xA0);
    float h = At<float>(self, 0xA4);
    CRGBA outline = At<CRGBA>(self, 0xA8);
    float x2 = float(double(x) + w);
    col::CRect rect = {x, float(double(y) - h - 1.0), x2, float(double(y) - h)};
    DrawRect(&rect, &outline);
    rect = {x, float(double(h) + y + 1.0), x2, float(double(h) + y)};
    DrawRect(&rect, &outline);
    CVector2D centre = {x, y};
    float inner = float(double(h) + 1.0);
    DrawRing(&centre, h, inner, &outline, 0x1E, kPiF, kTwoPiF);
    centre = {x2, y};
    float start = float(kPi * 0.0 / 180.0);
    DrawRing(&centre, h, inner, &outline, 0x1E, start, kPiF);
    CRGBA damage;
    RGBA_Construct(&damage, 0xA0, 0, 0, 0x9B);
    float jx = float(At<float>(self, 0x88) + shift);
    float jy = float(double(At<float>(self, 0x8C)) + 0.0);
    float jw = At<float>(self, 0x90);
    float jh = At<float>(self, 0x94);
    double hw = double(jw) * 0.5;
    double rhw;
    double hh;
    float hhf;
    CRGBA fill;
    if (0.005f < At<float>(self, 0x168)) {
        fill = {damage.r, damage.g, damage.b, 0xFF};
        rhw = double(r) * hw;
        hh = double(jh) * 0.5;
        float dmg = At<float>(self, 0x168);
        col::CRect hurt = {float(double(jx) + rhw), float(hh + jy), float(double(jx) + (double(r) + dmg) * hw),
                           float(double(jy) - hh)};
        DrawRect(&hurt, &fill);
        centre = {float((double(dmg) + r) * hw + jx), jy};
        hhf = float(hh);
        DrawSector(&centre, hhf, &fill, 0x12, start, kPiF);
        At<float>(self, 0x160) = 0.0f;
    } else {
        float m = At<float>(self, 0x160);
        fill = FadeTruncated(bar, damage, m);
        rhw = double(r) * hw;
        centre = {float(rhw + jx), jy};
        hh = 0.5 * jh;
        hhf = float(hh);
        DrawSector(&centre, hhf, &fill, 0x12, start, kPiF);
        float next = float((1.0 - double(At<float>(self, 0x160))) * 0.30000001192092896 + At<float>(self, 0x160));
        At<float>(self, 0x160) = next;
        if (1.0f < next) {
            At<float>(self, 0x160) = 1.0f;
        }
    }
    centre = {jx, jy};
    DrawSector(&centre, hhf, &bar, 0x12, kPiF, kTwoPiF);
    col::CRect body = {jx, float(double(jy) + hh), float(double(jx) + rhw), float(double(jy) - hh)};
    DrawRect(&body, &bar);
    Font_Begin();
    Font_SetSizePreset(1);
    std::memcpy(&g_fontColour, &g_labelColour, 4);
    Font_SetJustify(1);
    float lx = float(At<float>(self, 0x88) + shift);
    float ly = float(double(At<float>(self, 0x84)) + 0.0);
    lx = float(double(At<float>(self, 0x90)) * 0.5 + lx);
    float scaleY = float(double(sy) * 0.6000000238418579);
    float scaleX = float(double(sx) * 0.699999988079071);
    Font_SetScaleXY(scaleX, scaleY);
    Font_PrintString(lx, ly, Text_Get(kTheText, "N_JIMMY"), 1);
}

// 0x551E00 (vf7 Reset): the peds let go, everything back to empty
void __fastcall Reset(uint8_t* self, re::Edx) {
    HUDComponent_Reset(reinterpret_cast<HUDComponent*>(self));
    for (int offset : {0xAC, 0xB4, 0xB0}) {
        re::Ptr32<void>& ref = At<void*>(self, offset);
        if (ref) {
            CleanUpOldReference(ref, &ref);
            ref = nullptr;
        }
    }
    At<float>(self, 0x140) = 0.0f;
    At<uint8_t>(self, 0xB8) = 0;
    At<float>(self, 0x144) = 0.0f;
    At<uint8_t>(self, 0xB9) = 0;
    At<float>(self, 0x148) = 0.0f;
    At<uint8_t>(self, 0xBA) = 0;
    At<float>(self, 0x14C) = 0.0f;
    At<uint8_t>(self, 0xBB) = 0;
    At<float>(self, 0x150) = 0.0f;
    At<uint8_t>(self, 0xBC) = 0;
    At<float>(self, 0x154) = 0.0f;
    At<uint8_t>(self, 0xBD) = 0;
    At<float>(self, 0x15C) = 0.0f;
    At<uint8_t>(self, 0x174) = 0;
    At<float>(self, 0x158) = 1.0f;
    At<float>(self, 0x160) = 1.0f;
    At<float>(self, 0x164) = 0.0f;
    At<float>(self, 0x168) = 0.0f;
    At<float>(self, 0x16C) = 0.0f;
    At<float>(self, 0x170) = 0.0f;
    At<float>(self, 0x178) = 1.0f;
    At<float>(self, 0x17C) = 0.0f;
}

// 0x551EF0 (vf8 Draw): the first bar (a ped's, a prop's or the fixed value), the
// second ped's, their labels
void __fastcall Draw(uint8_t* self, re::Edx) {
    float second = 0.0f;
    float first;
    if (uint8_t* ped = At<uint8_t*>(self, 0xAC)) {
        first = HealthFraction(ped);
    } else if (void* prop = At<void*>(self, 0xB4)) {
        first = PropAnim_GetHealthFraction(prop);
    } else {
        if (At<uint8_t>(self, 0x174) != 1) {
            return;
        }
        first = At<float>(self, 0x17C) / At<float>(self, 0x178);
    }
    if (first < 0.0f) {
        first = 0.0f;
    } else if (1.0f < first) {
        first = 1.0f;
    }
    if (uint8_t* ped = At<uint8_t*>(self, 0xB0)) {
        second = HealthFraction(ped);
        if (second < 0.0f) {
            second = 0.0f;
        } else if (1.0f < second) {
            second = 1.0f;
        }
    }
    float sx = g_screenScaleX;
    float sy = g_screenScaleY;
    if (0.0f < first) {
        float r = first;
        if (double(r) < 0.05000000074505806) {
            r = 0.05f;
        } else if (1.0f < r) {
            r = 1.0f;
        }
        first = r;
        if (At<float>(self, 0x140) > r && !At<uint8_t>(self, 0xBD)) {
            At<float>(self, 0x148) = 0.4f;
        }
        At<float>(self, 0x15C) = float(double(At<float>(self, 0x140)) - r + At<float>(self, 0x15C));
        At<float>(self, 0x140) = r;
        if (0.25f > r && !(0.0f < At<float>(self, 0x150))) {
            At<float>(self, 0x150) = float(4.0 * r * kPointFour + kPointFour);
        }
        CRGBA bar;
        bar = *LerpColour_(&bar, r, At<CRGBA>(self, 0x60), At<CRGBA>(self, 0x64));
        float flash = 0.0f;
        uint8_t target[4];
        std::memcpy(target, &g_flashHit, 4);
        if (0.0f < At<float>(self, 0x148)) {
            flash = FlashCurve(At<float>(self, 0x148), kPointFour);
            RunDown(At<float>(self, 0x148));
        } else {
            At<float>(self, 0x15C) = float(-double(At<float>(self, 0x15C)) * 0.44999998807907104 + At<float>(self, 0x15C));
        }
        if (0.0f < At<float>(self, 0x150)) {
            flash = FlashCurve(At<float>(self, 0x150), kPointEightA);
            RunDown(At<float>(self, 0x150));
            std::memcpy(target, &g_flashLow, 4);
        }
        CRGBA edge = BlendTruncated(g_flashFrom, target, flash);
        if (!At<uint8_t>(self, 0xBA)) {
            const col::CRect& bounds = At<col::CRect>(self, 0x40);
            double dx = double(sx) * 2.0;
            double dy = 2.0 * sy;
            col::CRect rect = {float(bounds.left - dx), float(bounds.top - dy), float(bounds.right + dx),
                               float(dy + bounds.bottom)};
            DrawRect(&rect, &edge);
            float width = std::fabs(float(bounds.right - bounds.left));
            rect = {float(double(bounds.right) - width * (1.0 - double(r))), bounds.top, bounds.right, bounds.bottom};
            DrawRect(&rect, &g_barBack);
            width = std::fabs(float(bounds.right - bounds.left));
            rect = {bounds.left, bounds.top, float(double(width) * r + bounds.left), bounds.bottom};
            DrawRect(&rect, &bar);
        } else {
            // the capsule
            bool flipped = At<uint8_t>(self, 0xBB) != 0;
            float x = At<float>(self, 0x98);
            float y = At<float>(self, 0x9C);
            float w = At<float>(self, 0xA0);
            float h = At<float>(self, 0xA4);
            if (flipped) {
                x = float(double(x) - double(sx) * 160.0);
                y = float(double(y) + 0.0);
            }
            CRGBA* outline = &At<CRGBA>(self, 0xA8);
            float x2 = float(double(x) + w);
            col::CRect rect = {x, float(double(y) - h - 1.0), x2, float(double(y) - h)};
            DrawRect(&rect, outline);
            rect = {x, float(double(h) + y + 1.0), x2, float(double(h) + y)};
            DrawRect(&rect, outline);
            CVector2D centre = {x, y};
            float inner = float(double(At<float>(self, 0xA4)) + 1.0);
            DrawRing(&centre, At<float>(self, 0xA4), inner, outline, 0x1E, kPiF, kTwoPiF);
            centre = {x2, y};
            float start = float(kPi * 0.0 / 180.0);
            inner = float(double(h) + 1.0);
            DrawRing(&centre, h, inner, outline, 0x1E, start, kPiF);
            CRGBA damage;
            RGBA_Construct(&damage, 0xA0, 0, 0, 0x9B);
            float bx = At<float>(self, 0x88);
            float by = At<float>(self, 0x8C);
            float bw = At<float>(self, 0x90);
            float bh = At<float>(self, 0x94);
            if (flipped) {
                bx = float(double(bx) - double(sx) * 160.0);
                by = float(double(by) + 0.0);
            }
            double hw = double(bw) * 0.5;
            double rhw;
            double hh;
            float hhf;
            CRGBA fill;
            float dmg = At<float>(self, 0x15C);
            if (0.005f < dmg) {
                fill = {damage.r, damage.g, damage.b, 0xFF};
                rhw = double(r) * hw;
                hh = double(bh) * 0.5;
                hhf = float(hh);
                if (flipped) {
                    col::CRect hurt = {float(double(bx) + hw - rhw), float(hh + by),
                                       float(double(bx) + hw - (double(r) + dmg) * hw), float(double(by) - hh)};
                    DrawRect(&hurt, &fill);
                    centre = {float(double(bx) + hw - (double(dmg) + r) * hw), by};
                    DrawSector(&centre, hhf, &fill, 0x12, kPiF, kTwoPiF);
                } else {
                    col::CRect hurt = {float(double(bx) + rhw), float(hh + by),
                                       float(double(bx) + (double(r) + dmg) * hw), float(double(by) - hh)};
                    DrawRect(&hurt, &fill);
                    centre = {float((double(dmg) + r) * hw + bx), by};
                    DrawSector(&centre, hhf, &fill, 0x12, start, kPiF);
                }
                At<float>(self, 0x158) = 0.0f;
            } else {
                fill = FadeTruncated(bar, damage, At<float>(self, 0x158));
                hh = 0.5 * bh;
                hhf = float(hh);
                if (flipped) {
                    rhw = double(r) * hw;
                    centre = {float(double(bx) + hw - rhw), by};
                    DrawSector(&centre, hhf, &fill, 0x12, kPiF, kTwoPiF);
                } else {
                    rhw = hw * r;
                    centre = {float(rhw + bx), by};
                    DrawSector(&centre, hhf, &fill, 0x12, start, kPiF);
                }
                float next = float((1.0 - double(At<float>(self, 0x158))) * 0.30000001192092896 + At<float>(self, 0x158));
                At<float>(self, 0x158) = next;
                if (1.0f < next) {
                    At<float>(self, 0x158) = 1.0f;
                }
            }
            if (flipped) {
                centre = {float(double(bx) + hw), by};
                DrawSector(&centre, hhf, &bar, 0x12, start, kPiF);
                col::CRect body = {float(double(bx) + hw - rhw), float(double(by) + hh), float(double(bx) + hw),
                                   float(double(by) - hh)};
                DrawRect(&body, &bar);
            } else {
                centre = {bx, by};
                DrawSector(&centre, hhf, &bar, 0x12, kPiF, kTwoPiF);
                col::CRect body = {bx, float(double(by) + hh), float(double(bx) + rhw), float(double(by) - hh)};
                DrawRect(&body, &bar);
            }
            if (flipped) {
                SecondaryBar_DrawJimmy_(self);
            }
        }
    }
    if (0.0f < second) {
        float q = second;
        if (double(q) < 0.05000000074505806) {
            q = 0.05f;
        } else if (1.0f < q) {
            q = 1.0f;
        }
        second = q;
        if (At<float>(self, 0x144) > q && !At<uint8_t>(self, 0xBD)) {
            At<float>(self, 0x14C) = 0.4f;
        }
        At<float>(self, 0x144) = q;
        // (the pulse from the first bar's health: the game's)
        if (0.25f > q && !(0.0f < At<float>(self, 0x154))) {
            At<float>(self, 0x154) = float(4.0 * first * kPointFour + kPointFour);
        }
        CRGBA bar;
        bar = *LerpColour_(&bar, q, At<CRGBA>(self, 0x60), At<CRGBA>(self, 0x64));
        float flash = 0.0f;
        uint8_t target[4];
        std::memcpy(target, &g_flashHit, 4);
        if (0.0f < At<float>(self, 0x14C)) {
            flash = FlashCurve(At<float>(self, 0x14C), kPointFour);
            RunDown(At<float>(self, 0x14C));
        }
        // (the first bar's low pulse, run down a second time)
        if (0.0f < At<float>(self, 0x150)) {
            flash = FlashCurve(At<float>(self, 0x150), kPointEightA);
            RunDown(At<float>(self, 0x150));
            std::memcpy(target, &g_flashLow, 4);
        }
        CRGBA edge = BlendTruncated(g_flashFrom, target, flash);
        const col::CRect& bounds = At<col::CRect>(self, 0x50);
        double dx = double(sx) * 2.0;
        double dy = 2.0 * sy;
        col::CRect rect = {float(bounds.left - dx), float(bounds.top - dy), float(bounds.right + dx),
                           float(dy + bounds.bottom)};
        DrawRect(&rect, &edge);
        float width = std::fabs(float(bounds.right - bounds.left));
        rect = {float(double(bounds.right) - width * (1.0 - double(q))), bounds.top, bounds.right, bounds.bottom};
        DrawRect(&rect, &g_barBack);
        width = std::fabs(float(bounds.right - bounds.left));
        rect = {bounds.left, bounds.top, float(double(width) * q + bounds.left), bounds.bottom};
        DrawRect(&rect, &bar);
    }
    if (g_language == 7) {
        g_labelOffsetX = 8.0f;
        g_labelOffsetY = 8.0f;
    }
    if (At<uint8_t>(self, 0xB8) && 0.0f < first) {
        Font_Begin();
        if (g_language == 7) {
            Font_SetScale(0.4f);
        } else {
            Font_SetSizePreset(1);
        }
        std::memcpy(&g_fontColour, &g_labelColour, 4);
        Font_SetJustify(2);
        if (!At<uint8_t>(self, 0xBA)) {
            const uint16_t* text = Text_Get(kTheText, reinterpret_cast<const char*>(self + 0xBE));
            float y = float(At<float>(self, 0x7C) + g_labelOffsetY);
            float x = float(At<float>(self, 0x20) - g_labelOffsetX);
            Font_PrintString(x, y, text, 1);
        } else {
            float x = At<float>(self, 0x80);
            float y = At<float>(self, 0x84);
            if (At<uint8_t>(self, 0xBB)) {
                Font_SetJustify(0);
                x = float(double(At<float>(self, 0x88)) - double(sx) * 160.0);
                y = float(double(y) + 0.0);
            }
            float scaleY = float(double(sy) * 0.6000000238418579);
            float scaleX = float(double(sx) * 0.699999988079071);
            Font_SetScaleXY(scaleX, scaleY);
            const uint16_t* text = Text_Get(kTheText, reinterpret_cast<const char*>(self + 0xBE));
            Font_PrintString(x, y, text, 1);
        }
    }
    if (At<uint8_t>(self, 0xB9) && 0.0f < second) {
        Font_Begin();
        Font_SetJustify(2);
        if (g_language == 7) {
            Font_SetScale(0.4f);
        } else {
            Font_SetSizePreset(1);
        }
        std::memcpy(&g_fontColour, &g_labelColour, 4);
        const uint16_t* text = Text_Get(kTheText, reinterpret_cast<const char*>(self + 0xFE));
        float y = float(At<float>(self, 0x74) + g_labelOffsetY);
        float x = float(At<float>(self, 0x20) - g_labelOffsetX);
        Font_PrintString(x, y, text, 1);
    }
}

// 0x5532D0 (vf1 Setup): the two bars, the labels, the capsule and its Jimmy bar
void __fastcall Setup(uint8_t* self, re::Edx, const HUDLayoutRecord* record) {
    const uint8_t* d = record->data;
    auto s16 = [d](int offset) { return int32_t(*reinterpret_cast<const int16_t*>(d + offset)); };
    HUDComponent* component = reinterpret_cast<HUDComponent*>(self);
    int& h = At<int32_t>(self, 0xC);
    int& v = At<int32_t>(self, 0x10);
    HUDComponent_SetAlignment(component, reinterpret_cast<const char*>(d + 0x38), reinterpret_cast<const char*>(d + 0x40));
    float sx = g_screenScaleX;
    float sy = g_screenScaleY;
    col::CRect rect;
    At<col::CRect>(self, 0x40) = *ScreenRect(&rect, float(s16(0xC)), float(s16(0xE)), float(s16(0x10)), float(s16(0x12)), h, v);
    At<float>(self, 0x68) = float(s16(0x14));
    At<int32_t>(self, 0x6C) = s16(0x16);
    At<col::CRect>(self, 0x50) = *ScreenRect(&rect, float(s16(0x18)), float(s16(0x1A)), float(s16(0x10)), float(s16(0x12)), h, v);
    float halfW = float(double(float(s16(0x10))) * sx * 0.5);
    float halfH = float(double(sy) * float(s16(0x12)) * 0.5);
    At<float>(self, 0x20) = halfW + At<float>(self, 0x40);
    At<float>(self, 0x24) = halfH + At<float>(self, 0x44);
    At<float>(self, 0x28) = At<float>(self, 0x50) + halfW;
    At<float>(self, 0x2C) = halfH + At<float>(self, 0x54);
    At<float>(self, 0x30) = float(s16(0x10)) * sx;
    At<float>(self, 0x34) = float(s16(0x12)) * sy;
    At<float>(self, 0x38) = At<float>(self, 0x30);
    At<float>(self, 0x3C) = At<float>(self, 0x34);
    CVector2D point;
    At<CVector2D>(self, 0x78) = *ScreenPosition(&point, float(s16(0x20)), float(s16(0x22)), h, v);
    At<CVector2D>(self, 0x70) = *ScreenPosition(&point, float(s16(0x1C)), float(s16(0x1E)), h, v);
    HUDComponent_SetAlignment(component, reinterpret_cast<const char*>(d + 0x48), reinterpret_cast<const char*>(d + 0x50));
    col::CRect capsule = *ScreenRect(&rect, float(s16(0x24)), float(s16(0x26)), float(s16(0x28)), float(s16(0x2A)), h, v);
    At<float>(self, 0x88) = float(double(sx) * float(s16(0x28)) * 0.5) + capsule.left;
    At<float>(self, 0x8C) = capsule.top + float(double(sy) * float(s16(0x2A)) * 0.5);
    At<float>(self, 0x90) = float(s16(0x28)) * sx;
    At<float>(self, 0x94) = float(s16(0x2A)) * sy;
    At<CVector2D>(self, 0x80) = *ScreenPosition(&point, float(s16(0x2C)), float(s16(0x2E)), h, v);
    std::memcpy(self + 0x60, d + 4, 8);  // the full and empty colours
    col::CRect bar = *ScreenRect(&rect, float(s16(0x30)), float(s16(0x32)), float(s16(0x34)), float(s16(0x36)), h, v);
    At<float>(self, 0x98) = float(double(sx) * float(s16(0x34)) * 0.5) + bar.left;
    At<float>(self, 0x9C) = bar.top + float(double(sy) * float(s16(0x36)) * 0.5);
    At<float>(self, 0xA0) = float(s16(0x34)) * sx;
    At<float>(self, 0xA4) = float(s16(0x36)) * sy;
    std::memcpy(self + 0xA8, d, 4);  // the capsule's outline
}

// 0x5537E0: the bars for two peds (either nullptr), references kept; the flashes off
void __fastcall SetPeds(uint8_t* self, re::Edx, uint8_t* first, uint8_t* second) {
    re::Ptr32<void>& ref1 = At<void*>(self, 0xAC);
    if (first != ref1) {
        if (ref1) {
            CleanUpOldReference(ref1, &ref1);
        }
        ref1 = first;
        if (first) {
            RegisterReference(first, &ref1);
        }
    }
    re::Ptr32<void>& ref2 = At<void*>(self, 0xB0);
    if (second != ref2) {
        if (ref2) {
            CleanUpOldReference(ref2, &ref2);
        }
        ref2 = second;
        if (second) {
            RegisterReference(second, &ref2);
        }
    }
    re::Ptr32<void>& prop = At<void*>(self, 0xB4);
    if (prop) {
        CleanUpOldReference(prop, &prop);
        prop = nullptr;
    }
    At<float>(self, 0x148) = 0.0f;
    At<float>(self, 0x14C) = 0.0f;
    At<float>(self, 0x150) = 0.0f;
    At<float>(self, 0x154) = 0.0f;
    if (first) {
        At<float>(self, 0x140) = HealthFraction(first);
    }
    if (second) {
        At<float>(self, 0x144) = HealthFraction(second);
    }
}

// 0x5538C0: the first bar for a prop's health instead
void __fastcall SetProp(uint8_t* self, re::Edx, void* prop) {
    for (int offset : {0xAC, 0xB0}) {
        re::Ptr32<void>& ref = At<void*>(self, offset);
        if (ref) {
            CleanUpOldReference(ref, &ref);
            ref = nullptr;
        }
    }
    re::Ptr32<void>& ref = At<void*>(self, 0xB4);
    if (prop != ref) {
        if (ref) {
            CleanUpOldReference(ref, &ref);
        }
        ref = prop;
        if (prop) {
            RegisterReference(prop, &ref);
        }
    }
    At<float>(self, 0x148) = 0.0f;
    At<float>(self, 0x14C) = 0.0f;
    At<float>(self, 0x144) = 0.0f;
    At<float>(self, 0x150) = 0.0f;
    At<float>(self, 0x154) = 0.0f;
    if (prop) {
        At<float>(self, 0x140) = PropAnim_GetHealthFraction(prop);
    }
}

// 0x553AB0 (HUDSmallScriptText's vf8 Draw): its text through the Text component's
// slot 0 until its time is up (then hidden and cleared)
void __fastcall SmallScriptText_Draw(HUDScriptText* self, re::Edx) {
    double seconds = double(g_timeInMilliseconds) / 1000.0;
    if (double(self->endTime) < seconds) {
        reinterpret_cast<void(__thiscall*)(HUDComponent*, int)>(static_cast<void*>(self->vtable[5]))(self, 0);
        HUDScriptText_Clear(self);
        return;
    }
    void* text = g_hudText;
    for (int i = 0; i < self->numParams; i++) {
        HUDTextParam& param = self->params[i];
        switch (param.type) {
        case 1:
            HUDText_AddFloat(text, 0, float(*reinterpret_cast<int32_t*>(param.value)));
            break;
        case 2:
            HUDText_AddFloat(text, 0, *reinterpret_cast<float*>(param.value));
            break;
        case 3:
            HUDText_AddString(text, 0, param.value);
            break;
        case 4:
            HUDText_AddText(text, 0, param.value);
            break;
        default:
            break;
        }
    }
    HUDText_SetText(text, 0, self->text);
}

} // namespace hudsecondarybar

#define HUDSECONDARYBAR_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(hudsecondarybar_original_, __LINE__);                                       \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(hudsecondarybar_original_, __LINE__), \
             re::Mode::Original)

HUDSECONDARYBAR_REVERSED("HUD::LerpHealthColour", 0x00551530, &hudsecondarybar::LerpColour);
HUDSECONDARYBAR_REVERSED("HUDSecondaryHealthBar::DrawJimmy", 0x005515F0, &hudsecondarybar::DrawJimmy);
HUDSECONDARYBAR_REVERSED("HUDSecondaryHealthBar::Reset", 0x00551E00, &hudsecondarybar::Reset);
HUDSECONDARYBAR_REVERSED("HUDSecondaryHealthBar::Draw", 0x00551EF0, &hudsecondarybar::Draw);
HUDSECONDARYBAR_REVERSED("HUDSecondaryHealthBar::Setup", 0x005532D0, &hudsecondarybar::Setup);
HUDSECONDARYBAR_REVERSED("HUDSecondaryHealthBar::SetPeds", 0x005537E0, &hudsecondarybar::SetPeds);
HUDSECONDARYBAR_REVERSED("HUDSecondaryHealthBar::SetProp", 0x005538C0, &hudsecondarybar::SetProp);
HUDSECONDARYBAR_REVERSED("HUDSmallScriptText::Draw", 0x00553AB0, &hudsecondarybar::SmallScriptText_Draw);
