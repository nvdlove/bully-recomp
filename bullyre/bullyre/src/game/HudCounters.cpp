// The HUD's counters and meters: Generic_Counter (a count or a bar, with rolling
// digits), Generic_Text's lines, and the health meter (the arc round the heart).
// 0x53F110..0x540580 (their constructors, Setup and Reset are Hud.cpp's).

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "game/Collision.h"
#include "game/Hud.h"
#include "reversed.h"

namespace {

struct CVector2D {
    float x, y;
};

// A number whose changed digits roll up (0x128 bytes; two in a counter)
struct CounterRoll {
    int32_t firstChanged;     // +0x00 the first digit that differs
    float oldWidth;           // +0x04 the old text's width
    float headWidth;          // +0x08 the width of the unchanged digits
    uint8_t unknown0C[8];
    float scroll;             // +0x14 how far the digits have rolled
    float lastTime;           // +0x18 ms
    float yOffset;            // +0x1C
    uint8_t rolling;          // +0x20
    uint8_t unknown21[3];
    re::Ptr32<const uint16_t> format;   // +0x24 the text it's shown in (nullptr: not shown)
    uint16_t oldText[0x40];   // +0x28
    uint16_t newText[0x40];   // +0xA8
};
static_assert(offsetof(CounterRoll, format) == 0x24);
static_assert(offsetof(CounterRoll, newText) == 0xA8);
static_assert(sizeof(CounterRoll) == 0x128);

// Generic_Counter, with what's understood here (Hud.h's HUDGenericCounter)
struct GenericCounter : HUDComponent {
    col::CRect rect;          // +0x20
    float x, y;               // +0x30 where the rolling numbers go
    col::CRect rect2;         // +0x38
    re::Ptr32<void> icon;               // +0x48
    int32_t max;              // +0x4C
    int32_t value;            // +0x50
    int32_t previousValue;    // +0x54
    int32_t previousMax;      // +0x58
    CRGBA colour;             // +0x5C
    uint8_t showMax;          // +0x60 "value/max"
    uint8_t bar;              // +0x61 a bar, not a number
    uint8_t rollingNumbers;   // +0x62
    char text[0x41];          // +0x63 a text key the numbers are shown in (empty: none)
    float textX, textY;       // +0xA4 where that goes (0, 0: the default)
    CounterRoll rolls[2];     // +0xAC the value's, the max's
};
static_assert(offsetof(GenericCounter, textX) == 0xA4);
static_assert(offsetof(GenericCounter, rolls) == 0xAC);
static_assert(sizeof(GenericCounter) == sizeof(HUDGenericCounter));

GAME_FN(void, __thiscall, RGBA_Construct, 0x00510DE0, CRGBA* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(CVector2D*, __cdecl, ScreenPosition, 0x0052D4F0, CVector2D* out, float x, float y, int hAlign, int vAlign);
GAME_FN(col::CRect*, __cdecl, ScreenRect, 0x0052D650, col::CRect* out, float x, float y, float width, float height,
        int hAlign, int vAlign);
GAME_FN(void, __cdecl, DrawRect, 0x0052C470, const col::CRect* rect, const CRGBA* colour);
GAME_FN(void, __cdecl, DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture, const CRGBA* colour,
        const CVector2D* uv1, const CVector2D* uv2);
GAME_FN(void, __cdecl, DrawRing, 0x0052D390, const CVector2D* centre, float outer, float inner, const CRGBA* colour,
        int segments, float start, float end);
GAME_FN(CRGBA*, __cdecl, LerpColour, 0x00551530, CRGBA* out, float t, CRGBA from, CRGBA to);
GAME_FN(const uint16_t*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __cdecl, AsciiToText, 0x00690090, const char* ascii, uint16_t* text);
GAME_FN(void, __cdecl, Font_Begin, 0x00688E60);
GAME_FN(void, __cdecl, Font_SetJustify, 0x00688160, int justify);
GAME_FN(void, __cdecl, Font_SetSizePreset, 0x00688050, int preset);
GAME_FN(void, __cdecl, Font_SetColour, 0x00424480, const CRGBA* colour);
GAME_FN(void, __cdecl, Font_AddNumber, 0x006880D0, float number);
GAME_FN(void, __cdecl, Font_FormatText, 0x00688ED0, uint16_t* out, const uint16_t* format, int size);
GAME_FN(float, __cdecl, Font_GetStringWidth, 0x0068A920, const uint16_t* text, int);
GAME_FN(void, __cdecl, Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __cdecl, Font_PrintAt, 0x0068D670, int id, const uint16_t* text);
GAME_FN(void, __cdecl, Font_Render, 0x0068CDF0);
GAME_FN(void, __cdecl, EmptyFunction, 0x0049A650, int);
GAME_FN(void, __thiscall, HUDComponent_SetTextJustify, 0x0053CE40, HUDComponent* self, int wrap);
// its own, where modes apply
GAME_FN(void, __thiscall, Counter_StartRoll, 0x0053F300, GenericCounter* self, int firstChanged, CounterRoll* roll);
GAME_FN(void, __thiscall, Counter_DrawRoll, 0x0053F3A0, GenericCounter* self, CounterRoll* roll, float now);
GAME_FN(void, __thiscall, Counter_DrawNumber, 0x0053F720, GenericCounter* self, int which, int from, int to);

GAME_VAR(uint32_t, g_timeInMilliseconds, 0x00C1A9B4);
GAME_VAR(float, g_timeStep, 0x00C1A9A4);
GAME_VAR(float, g_timeStepScale, 0x00AB45D8);        // (0.02)
GAME_VAR(float, g_screenScaleX, 0x00A957A8);
GAME_VAR(float, g_screenScaleY, 0x00A957AC);
GAME_VAR(int32_t, g_language, 0x00C18724);
GAME_VAR(uint8_t*, g_playerPed, 0x00C1AEA8);
GAME_VAR(HUDComponent*, g_hud_C67534, 0x00C67534);
GAME_VAR(HUDComponent*, g_hud_C67538, 0x00C67538);
GAME_VAR(uint8_t*, g_hudErrands, 0x00C67558);
GAME_VAR(uint8_t*, g_hudHealth, 0x00C674D0);         // (+0xBB: the meter hidden)
GAME_VAR(uint8_t*, g_gameMode, 0x020C8704);          // (+4 active, +0x3C the mode)
GAME_VAR(CRGBA, g_barBackColour, 0x00CEFCB8);
GAME_VAR(CRGBA, g_barLowColour, 0x00CEFCBC);
GAME_VAR(CRGBA, g_barMidColour, 0x00CEFCC0);
GAME_VAR(CRGBA, g_barHighColour, 0x00CEFCC4);
GAME_VAR(void*, g_heartIcon, 0x00C66F48);
GAME_ARRAY(uint8_t, g_fontColour, 0x0208B764);
GAME_VAR(float, g_fontScaleY, 0x0208B770);
GAME_VAR(int16_t, g_fontStyle, 0x0208B788);
GAME_VAR(int16_t, g_font_208B786, 0x0208B786);
GAME_ARRAY(float, g_fontStyleHeights, 0x00BA4E8C);
inline void* const kTheText = reinterpret_cast<void*>(0x020C2FA4);

template <typename R, typename... A>
R VCall(HUDComponent* self, int slot, A... args) {
    return reinterpret_cast<R(__thiscall*)(HUDComponent*, re::Arg<A>...)>(static_cast<void*>(self->vtable[slot]))(self, args...);
}

float LineHeight() {
    return g_fontStyleHeights[g_fontStyle] * g_fontScaleY;
}

// ---- Generic_Counter --------------------------------------------------------------------

// 0x53F110 (vf5 SetVisible): a lone jmp to HUDComponent::SetVisible (0x53CEB0); the x64
// dispatch points the jump at that body itself

// 0x53F140: rolling numbers on (or off), in these two texts
void __fastcall Counter_SetRolling(GenericCounter* self, re::Edx, int rolling, const char* valueKey, const char* maxKey,
                                   float maxYOffset) {
    std::memset(&self->rolls[0], 0, sizeof(CounterRoll));
    std::memset(&self->rolls[1], 0, sizeof(CounterRoll));
    self->rollingNumbers = uint8_t(rolling);
    self->rolls[0].format = Text_Get(kTheText, valueKey);
    const uint16_t* maxFormat = Text_Get(kTheText, maxKey);
    self->rolls[0].yOffset = 0.0f;
    self->rolls[1].format = maxFormat;
    self->rolls[1].yOffset = maxYOffset;
}

// 0x53F300: the old text drawn, the widths measured, the roll started
void __fastcall Counter_StartRoll_(GenericCounter* self, re::Edx, int firstChanged, CounterRoll* roll) {
    roll->oldWidth = Font_GetStringWidth(roll->oldText, 1);
    Font_PrintString(self->x, self->y + roll->yOffset, roll->oldText, 1);
    Font_Render();
    roll->firstChanged = firstChanged;
    uint16_t saved = roll->newText[firstChanged];
    roll->newText[firstChanged] = 0;
    roll->headWidth = Font_GetStringWidth(roll->newText, 1);
    roll->newText[roll->firstChanged] = saved;
    roll->scroll = 0.0f;
    roll->lastTime = float(g_timeInMilliseconds);
}

// 0x53F3A0: a frame of the roll: the unchanged head, the old digits going up and the
// new ones coming in under them, a line's height in all
void __fastcall Counter_DrawRoll_(GenericCounter* self, re::Edx, CounterRoll* roll, float now) {
    Font_Begin();
    std::memcpy(g_fontColour, &self->colour, 4);
    Font_SetJustify(0);
    uint16_t saved = roll->newText[roll->firstChanged];
    roll->newText[roll->firstChanged] = 0;
    Font_PrintString(self->x, roll->yOffset + self->y, roll->newText, 1);
    Font_Render();
    roll->newText[roll->firstChanged] = saved;
    Font_PrintString(self->x + roll->headWidth, roll->yOffset + self->y - roll->scroll,
                     &roll->oldText[roll->firstChanged], 1);
    float height = LineHeight();
    Font_PrintString(self->x + roll->headWidth, height + (roll->yOffset + self->y - roll->scroll),
                     &roll->newText[roll->firstChanged], 1);
    Font_Render();
    float elapsed = now - roll->lastTime;
    roll->lastTime = elapsed;
    float step = elapsed * LineHeight();
    step = float(double(step) * 0.002);
    if (1.0f > step) {
        step = 1.0f;
    }
    roll->lastTime = now;
    roll->scroll = roll->scroll + step;
    float lineHeight = LineHeight();
    if (lineHeight < roll->scroll || lineHeight == roll->scroll) {
        roll->rolling = 0;
    }
}

// 0x53F720: one of the numbers, rolling when it has changed
void __fastcall Counter_DrawNumber_(GenericCounter* self, re::Edx, int which, int from, int to) {
    CounterRoll* roll = &self->rolls[which];
    const uint16_t* format = roll->format;
    if (!format) {
        return;
    }
    if (roll->rolling) {
        Counter_DrawRoll(self, roll, float(g_timeInMilliseconds));
        return;
    }
    Font_Begin();
    Font_AddNumber(float(from));
    Font_FormatText(roll->oldText, format, 0x40);
    Font_Begin();
    Font_AddNumber(float(to));
    Font_FormatText(roll->newText, format, 0x40);
    int i = 0;
    for (const uint16_t* c = roll->oldText; *c; c++, i++) {
        if (*c != c[0x40]) {
            roll->rolling = 1;
            break;
        }
    }
    if (roll->rolling) {
        Counter_StartRoll(self, i, roll);
        return;
    }
    Font_PrintString(self->x, roll->yOffset + self->y, roll->newText, 1);
}

// where the count goes: by the icon, or at the right
CVector2D CountPosition(const GenericCounter* self, const col::CRect& iconRect, float sx, float sy) {
    CVector2D pos;
    ScreenPosition(&pos, 32.0f, 32.0f, 0, 0);
    g_font_208B786 = 0;
    if (self->icon) {
        g_font_208B786 = 3;
        pos.x = (iconRect.left + iconRect.right) * 0.5f;
        pos.y = (iconRect.top + iconRect.bottom) * 0.5f;
        pos.x = float(double(sx) * 0.0) + pos.x;
        pos.y = pos.y - sy * 55.0f;
    } else {
        CVector2D tmp;
        CVector2D* p = ScreenPosition(&tmp, -45.0f, -50.0f, 2, 2);
        pos.x = p->x;
        pos.y = p->y;
        g_font_208B786 = 1;
    }
    return pos;
}

// 0x53F840 (vf8 Draw)
void __fastcall Counter_Draw(GenericCounter* self, re::Edx) {
    if (!self->bar) {
        // (hidden under some of the HUD, in minigame 10, and while the errands' show)
        if (VCall<uint8_t>(g_hud_C67534, 4) && re::PtrCast<uint8_t*>(g_hud_C67534)[0x40]) {
            return;
        }
        if (VCall<uint8_t>(g_hud_C67538, 4)) {
            return;
        }
        if (g_gameMode && g_gameMode[4] && *reinterpret_cast<int32_t*>(g_gameMode + 0x3C) == 10) {
            return;
        }
        if (!(0.0f == *reinterpret_cast<float*>(g_hudErrands + 0x4C))) {
            return;
        }
    }
    float sx = g_screenScaleX;
    float sy = g_screenScaleY;
    col::CRect iconRect = {1000000.0f, -1000000.0f, -1000000.0f, 1000000.0f};
    col::CRect rect;
    if (self->icon) {
        ScreenRect(&rect, 60.0f, 50.0f, 56.0f, 56.0f, 0, 0);   // (unused)
        const col::CRect* r = self->bar ? ScreenRect(&rect, -50.0f, -40.0f, 64.0f, 64.0f, 2, 1)
                                        : ScreenRect(&rect, -50.0f, -50.0f, 64.0f, 64.0f, 2, 2);
        iconRect = *r;
        CVector2D uv2 = {1.0f, 1.0f};
        CVector2D uv1 = {0.0f, 0.0f};
        CRGBA white;
        RGBA_Construct(&white, 0xFF, 0xFF, 0xFF, 0xFF);
        DrawRectUV(iconRect.left, iconRect.top, iconRect.right, iconRect.bottom, self->icon, &white, &uv1, &uv2);
    }
    if (self->bar) {
        float fraction = 0.0f;
        CRGBA colour;
        if (self->max <= 0) {
            colour = g_barLowColour;
        } else {
            fraction = float(double(self->value) / double(self->max));
            if (fraction < 0.333333343f) {
                colour = g_barLowColour;
            } else if (fraction < 0.666666687f) {
                colour = g_barMidColour;
            } else {
                colour = g_barHighColour;
            }
        }
        col::CRect bar = *ScreenRect(&rect, -64.0f, 0.0f, 50.0f, 10.0f, 2, 1);
        if (self->icon) {
            float cx = (iconRect.left + iconRect.right) * 0.5f;
            cx = float(double(sx) * 0.0) + cx;
            float cy = (iconRect.top + iconRect.bottom) * 0.5f;
            cy = cy + sy * 40.0f;
            float halfW = sx * 30.0f;
            bar.left = cx - halfW;
            bar.right = cx + halfW;
            float halfH = sy * 5.0f;
            bar.top = cy - halfH;
            bar.bottom = cy + halfH;
        }
        DrawRect(&bar, &g_barBackColour);
        float borderX = sx * 2.0f;
        bar.left = bar.left + borderX;
        bar.right = bar.right - borderX;
        float borderY = 2.0f * sy;
        bar.top = bar.top + borderY;
        bar.bottom = bar.bottom - borderY;
        bar.right = std::fabs(bar.right - bar.left) * fraction + bar.left;
        DrawRect(&bar, &colour);
        return;
    }
    if (self->text[0]) {
        // the numbers in a text ("~1~ of ~1~")
        Font_Begin();
        std::memcpy(g_fontColour, &self->colour, 4);
        CVector2D pos = CountPosition(self, iconRect, sx, sy);
        float x = pos.x;
        float y = pos.y;
        if (g_language == 4 && g_gameMode && g_gameMode[4]) {
            y = float(double(pos.y) - 30.0);
        }
        if (self->textX != 0.0f || self->textY != 0.0f) {
            Font_SetJustify(0);
            x = self->textX * g_screenScaleX;
            y = self->textY * g_screenScaleY;
        }
        Font_AddNumber(float(self->value));
        Font_AddNumber(float(self->max));
        uint16_t text[0x40];
        Font_FormatText(text, Text_Get(kTheText, self->text), 0x40);
        Font_PrintString(x, y, text, 1);
        return;
    }
    if (self->rollingNumbers) {
        uint16_t text[0x40];
        if (const uint16_t* format = self->rolls[0].format) {
            if (self->previousValue != self->value) {
                Counter_DrawNumber(self, 0, self->previousValue, self->value);
                if (!self->rolls[0].rolling) {
                    self->previousValue = self->value;
                }
            } else {
                Font_Begin();
                Font_SetColour(&self->colour);
                Font_SetJustify(0);
                Font_AddNumber(float(self->value));
                Font_FormatText(text, format, 0x40);
                Font_PrintString(self->x, self->rolls[0].yOffset + self->y, text, 1);
            }
        }
        if (const uint16_t* format = self->rolls[1].format) {
            if (self->previousMax != self->max) {
                Counter_DrawNumber(self, 1, self->previousMax, self->max);
                if (!self->rolls[1].rolling) {
                    self->previousMax = self->max;
                }
            } else {
                Font_Begin();
                Font_SetColour(&self->colour);
                Font_SetJustify(0);
                Font_AddNumber(float(self->max));
                Font_FormatText(text, format, 0x40);
                Font_PrintString(self->x, self->rolls[1].yOffset + self->y, text, 1);
            }
        }
        return;
    }
    char ascii[0x98];
    if (self->showMax) {
        std::snprintf(ascii, sizeof(ascii), "%d/%d", self->value, self->max);
    } else {
        std::snprintf(ascii, sizeof(ascii), "%d", self->value);
    }
    uint16_t text[0x40];
    AsciiToText(ascii, text);
    Font_Begin();
    Font_SetColour(&self->colour);
    CVector2D pos = CountPosition(self, iconRect, sx, sy);
    Font_PrintString(pos.x, pos.y, text, 1);
}

// ---- Generic_Text -------------------------------------------------------------------------

// 0x5400F0 (vf8 Draw): its lines, each until its time (placed ones by their id)
void __fastcall GenericText_Draw(HUDGenericText* self, re::Edx) {
    uint32_t now = g_timeInMilliseconds;
    for (uint32_t i = 0; i < 5; i++) {
        HUDGenericText::Line& line = self->lines[i];
        if (line.id == -1) {
            if (line.text[0] == 0) {
                continue;
            }
            Font_Begin();
            Font_SetSizePreset(4);
            std::memcpy(g_fontColour, &self->colour, 4);
            CRGBA red;
            RGBA_Construct(&red, 0xA0, 0x30, 0x30, 0xFF);
            std::memcpy(g_fontColour, &red, 4);
            int y = int(float(self->unknown2C * int(i)) + self->y);
            HUDComponent_SetTextJustify(self, self->unknown30);
            Font_PrintString(self->x, float(y), line.text, 1);
            if (now > line.startTime && i <= 4) {
                line.startTime = 0;
                line.text[0] = 0;
            }
        } else if (line.text[0] != 0) {
            Font_PrintAt(line.id, line.text);
            if (now > line.startTime) {
                line.text[0] = 0;
                line.id = -1;
            }
        }
    }
}

// ---- the health meter ----------------------------------------------------------------------

// the meters' pulse: up over the first fifth, down over the rest
float Pulse(float t) {
    if (t < 0.2f) {
        float u = t / 0.2f;
        return u * u;
    }
    float u = float(double(t - 0.2f) / 0.7999999970197678);   // (1 - 0.2f, in double)
    return 1.0f - u * u;
}

// 0x540580 (vf8 Draw): the arc filled to the player's health (flashing grey when it
// drops, green when it rises), the overflow beyond it, the heart pulsing red when low
void __fastcall HealthMeter_Draw(HUDHealthMeter* self, re::Edx) {
    uint8_t* player = g_playerPed;
    if (!player || g_hudHealth[0xBB]) {
        return;
    }
    float sx = g_screenScaleX;
    float sy = g_screenScaleY;
    float iconX = sx * -40.0f;
    float iconY = sy * -40.0f;
    float iconW = sx * 20.0f;
    float iconH = sy * 20.0f;
    CRGBA c1, red, c3, grey, green, overColour, shadow, back;
    RGBA_Construct(&c1, 0xEB, 0xEB, 0xEB, 0x80);
    RGBA_Construct(&red, 0xEB, 0x40, 0x40, 0xFF);
    RGBA_Construct(&c3, 0xEB, 0xEB, 0xEB, 0xFF);
    RGBA_Construct(&grey, 0x40, 0x40, 0x40, 0xFF);
    RGBA_Construct(&green, 0x20, 0x40, 0x20, 0xFF);
    RGBA_Construct(&overColour, 0x96, 0x96, 0x14, 0x80);
    RGBA_Construct(&shadow, 0x10, 0x10, 0x10, 0x80);
    RGBA_Construct(&back, 0x32, 0x32, 0x32, 0x80);
    float health = *reinterpret_cast<float*>(player + 0x1CB8) / *reinterpret_cast<float*>(player + 0x1CA4);
    float over = health - 1.0f;
    if (0.0f < health) {
        if (health < 0.05f) {
            health = 0.05f;
        } else if (health > 1.0f) {
            health = 1.0f;
        }
    } else if (health <= 0.0f) {
        health = 0.0f;
    }
    if (over > 0.0f && over > 1.0f) {
        over = 1.0f;
    }
    CRGBA flash = grey;
    bool changed = false;
    if (self->unknown4C > health) {
        changed = true;
    } else if (self->unknown4C < health) {
        flash = green;
        changed = true;
    }
    if (changed) {
        self->unknown48 = 0.4f;
        EmptyFunction(0);
    }
    self->unknown4C = health;
    if (0.3f > health && !(0.0f < self->unknown44)) {
        self->unknown44 = 0.8f;
        EmptyFunction(0);
    }
    float pulse = 0.0f;
    if (0.0f < self->unknown48) {
        pulse = Pulse(1.0f - float(double(self->unknown48) / double(0.4f)));
        float left = self->unknown48 - g_timeStepScale * g_timeStep;
        self->unknown48 = left;
        if (left < 0.0f || left == 0.0f) {
            self->unknown48 = 0.0f;
        }
    }
    float arcEnd = self->angleFrom;
    CRGBA arcColour;
    arcColour = *LerpColour(&arcColour, health, self->colours[0], self->colours[1]);
    float span = self->angleTo - self->angleFrom;
    arcEnd = span * health + arcEnd;
    SetRenderState(1, 0);
    CRGBA ring;
    ring.r = uint8_t(int(float(flash.r - shadow.r) * pulse + float(shadow.r)));
    ring.g = uint8_t(int(float(flash.g - shadow.g) * pulse + float(shadow.g)));
    ring.b = uint8_t(int(float(flash.b - shadow.b) * pulse + float(shadow.b)));
    ring.a = uint8_t(int(pulse * float(flash.a - shadow.a) + float(shadow.a)));
    const auto* centre = reinterpret_cast<const CVector2D*>(&self->x);
    constexpr float kThreeDegrees = float(0.05235987901687622);
    DrawRing(centre, self->height - 2.0f, self->width + 2.0f, &ring, self->unknown38, self->angleFrom - kThreeDegrees,
             self->angleTo + kThreeDegrees);
    CRGBA backCopy = back;
    DrawRing(centre, self->height, self->width, &backCopy, self->unknown38, arcEnd, self->angleTo);
    DrawRing(centre, self->height, self->width, &arcColour, self->unknown38, self->angleFrom, arcEnd);
    if (!(over < 0.05f)) {
        float span2 = self->angleTo - self->angleFrom;
        CRGBA overCopy = overColour;
        DrawRing(centre, self->height, self->width, &overCopy, self->unknown38, self->angleFrom,
                 over * span2 + self->angleFrom);
    }
    // the heart: pulsing red while the health is low, else by the change pulse
    CRGBA iconTo = c3;
    if (0.0f < self->unknown44) {
        pulse = Pulse(1.0f - self->unknown44 / 0.8f);
        float left = self->unknown44 - g_timeStepScale * g_timeStep;
        self->unknown44 = left;
        if (!(left > 0.0f) && !std::isnan(left)) {
            self->unknown44 = 0.0f;
        }
        iconTo = red;
    }
    CRGBA icon;
    icon.r = uint8_t(int(float(iconTo.r - c1.r) * pulse + float(c1.r)));
    icon.g = uint8_t(int(float(iconTo.g - c1.g) * pulse + float(c1.g)));
    icon.b = uint8_t(int(float(iconTo.b - c1.b) * pulse + float(c1.b)));
    icon.a = uint8_t(int(float(iconTo.a - c1.a) * pulse + float(c1.a)));
    float scale = pulse * 0.200000048f + 1.0f;
    CVector2D uv2 = {1.0f, 1.0f};
    CVector2D uv1 = {0.0f, 0.0f};
    float halfH = iconH * 0.5f * scale;
    float cy = self->y + iconY;
    float halfW = iconW * 0.5f * scale;
    float cx = self->x + iconX;
    DrawRectUV(cx - halfW, cy - halfH, cx + halfW, cy + halfH, g_heartIcon, &icon, &uv1, &uv2);
}

} // namespace

#define HUDCOUNTERS_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(hudcounters_original_, __LINE__);                                     \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(hudcounters_original_, __LINE__), \
             re::Mode::Original)

HUDCOUNTERS_REVERSED("HUDGenericCounter::SetRolling", 0x0053F140, &Counter_SetRolling);
HUDCOUNTERS_REVERSED("HUDGenericCounter::StartRoll", 0x0053F300, &Counter_StartRoll_);
HUDCOUNTERS_REVERSED("HUDGenericCounter::DrawRoll", 0x0053F3A0, &Counter_DrawRoll_);
HUDCOUNTERS_REVERSED("HUDGenericCounter::DrawNumber", 0x0053F720, &Counter_DrawNumber_);
HUDCOUNTERS_REVERSED("HUDGenericCounter::Draw", 0x0053F840, &Counter_Draw);
HUDCOUNTERS_REVERSED("HUDGenericText::Draw", 0x005400F0, &GenericText_Draw);
HUDCOUNTERS_REVERSED("HUDHealthMeter::Draw", 0x00540580, &HealthMeter_Draw);
