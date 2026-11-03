// HUDRacing's drawing: the race's time, position and lap under the top of the screen,
// in outlined digits at a fixed pitch, the last lap's time and the split. 0x546A30 (the
// rest of HUDRacing is Hud.cpp's).

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

// HUDRacing, with the timer's fields (Hud.h has them as unknowns)
struct Racing : HUDComponent {
    uint8_t unknown20[0x40];
    uint32_t start;           // +0x60 the race's start time (0: no timer)
    uint32_t elapsed;         // +0x64 ms
    uint32_t splitTime;       // +0x68 when the split was set
    int32_t split;            // +0x6C ms (negative: ahead)
    uint32_t lapTime;         // +0x70 ms
    uint32_t lapTimeSet;      // +0x74 when
};
static_assert(offsetof(Racing, start) == 0x60);
static_assert(sizeof(Racing) == sizeof(HUDRacing));

GAME_FN(void, __thiscall, RGBA_Construct, 0x00510DE0, CRGBA* rgba, int r, int g, int b, int a);
GAME_FN(CVector2D*, __cdecl, ScreenPosition, 0x0052D4F0, CVector2D* out, float x, float y, int hAlign, int vAlign);
GAME_FN(void, __cdecl, DrawRect, 0x0052C470, const col::CRect* rect, const CRGBA* colour);
GAME_FN(const uint16_t*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __cdecl, AsciiToText, 0x00690090, const char* ascii, uint16_t* text);
GAME_FN(int, __cdecl, TextLength, 0x00690020, const uint16_t* text);
GAME_FN(void, __cdecl, Font_Begin, 0x00688E60);
GAME_FN(void, __cdecl, Font_SetScale, 0x00688080, float scale);
GAME_FN(void, __cdecl, Font_AddNumber, 0x006880D0, float number);
GAME_FN(void, __cdecl, Font_FormatText, 0x00688ED0, uint16_t* out, const uint16_t* format, int size);
GAME_FN(float, __cdecl, Font_GetStringWidth, 0x0068A920, const uint16_t* text, int);
GAME_FN(float, __cdecl, Font_GetHeight, 0x00688220);
GAME_FN(void, __cdecl, Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __cdecl, Font_Render, 0x0068CDF0);
GAME_FN(uint8_t, __cdecl, HUD_Fn5142D0, 0x005142D0);
// the race (the minigame object)
GAME_FN(uint8_t*, __thiscall, CRace_GetEntry, 0x0070D390, void* race, void* ped);
GAME_FN(int32_t, __thiscall, CRace_GetPosition, 0x0070D420, void* race, void* ped);
GAME_FN(uint8_t, __thiscall, CRace_HasFinished, 0x0070D500, void* race, void* ped);

GAME_VAR(uint32_t, g_timeInMilliseconds, 0x00C1A9B4);
GAME_VAR(int32_t, g_language, 0x00C18724);
GAME_VAR(float, g_screenScaleX, 0x00A957A8);
GAME_VAR(uint8_t*, g_playerPed, 0x00C1AEA8);
GAME_VAR(uint8_t*, g_gameMode, 0x020C8704);          // (+4 active, +0x3C 14: a race)
GAME_VAR(HUDComponent*, g_hud_C674D4, 0x00C674D4);
GAME_VAR(CRGBA, g_raceColour, 0x00CF01CC);
GAME_VAR(CRGBA, g_raceNoTimerColour, 0x00CF01D0);
GAME_VAR(CRGBA, g_raceOutline, 0x00CF01DC);
GAME_VAR(CRGBA, g_raceBoxColour, 0x00CF01E0);
GAME_ARRAY(uint8_t, g_fontColour, 0x0208B764);
GAME_VAR(int16_t, g_fontStyle, 0x0208B788);
GAME_VAR(int16_t, g_fontEdge, 0x0208B784);
GAME_VAR(int16_t, g_font_208B786, 0x0208B786);
GAME_ARRAY(int32_t, g_fontStyleLoaded, 0x0208B218);
inline void* const kTheText = reinterpret_cast<void*>(0x020C2FA4);

void SetFontColour(const CRGBA& c) {
    std::memcpy(g_fontColour, &c, 4);
}

// text outlined: 8 copies 2 pixels round it, then the text
void PrintOutlined(float x, float y, const uint16_t* text, const CRGBA& colour) {
    SetFontColour(g_raceOutline);
    Font_PrintString(x - 2.0f, y - 2.0f, text, 1);
    Font_PrintString(x - 2.0f, y, text, 1);
    Font_PrintString(x, y - 2.0f, text, 1);
    Font_PrintString(x - 2.0f, y + 2.0f, text, 1);
    Font_PrintString(x + 2.0f, y - 2.0f, text, 1);
    Font_PrintString(x, y + 2.0f, text, 1);
    Font_PrintString(x + 2.0f, y, text, 1);
    Font_PrintString(x + 2.0f, y + 2.0f, text, 1);
    SetFontColour(colour);
    Font_PrintString(x, y, text, 1);
}

// digits right to left at the pitch, centred on x (each printed as the tail of the
// text, then cut off)
void PrintDigits(float x, float y, uint16_t* text, float pitch, const CRGBA& colour) {
    int length = TextLength(text);
    x = (float(length) * pitch * 0.5f - pitch * 0.5f) + x;
    for (int i = length; i > 0; i--) {
        uint16_t* c = &text[i - 1];
        PrintOutlined(x, y, c, colour);
        *c = 0;
        x = x - pitch;
    }
}

// a line in a box centred on x
void PrintBoxed(float x, float y, const uint16_t* text, const CRGBA& colour) {
    float half = float(double(Font_GetStringWidth(text, 1)) * 0.5);
    col::CRect box;
    box.top = y - 3.0f;
    box.bottom = Font_GetHeight() + y + 5.0f;
    box.left = (x - half) - 6.0f;
    box.right = half + x + 6.0f;
    DrawRect(&box, &g_raceBoxColour);
    PrintOutlined(x, y, text, colour);
}

// 0x546A30 (vf8 Draw): the time, the position and the lap (headed, in boxes), the last
// lap's time for 2.5 seconds and the split for 1
void __fastcall HUDRacing_Draw(Racing* self, re::Edx) {
    if (reinterpret_cast<uint8_t(__thiscall*)(HUDComponent*)>(static_cast<void*>(g_hud_C674D4->vtable[4]))(g_hud_C674D4)) {
        return;
    }
    float sx = g_screenScaleX;
    Font_Begin();
    float scale;
    if (g_fontStyleLoaded[4] != 0) {
        g_fontStyle = 4;
        g_fontEdge = 0;
        scale = g_language == 7 ? 0.5f : 0.65f;
    } else {
        if (g_fontStyleLoaded[1] != 0) {
            g_fontStyle = 1;
            g_fontEdge = 0;
        }
        scale = g_language == 7 ? 0.6f : 1.0f;
    }
    Font_SetScale(scale);
    // the text's colour (its alpha from the same colour as its rgb)
    CRGBA colour = g_raceColour;
    uint8_t alpha = colour.a;
    if (self->start == 0) {
        colour.r = g_raceNoTimerColour.r;
        colour.g = g_raceNoTimerColour.g;
        colour.b = g_raceNoTimerColour.b;
        alpha = g_raceNoTimerColour.a;
    }
    CRGBA text = {colour.r, colour.g, colour.b, alpha};
    g_font_208B786 = 3;
    int32_t laps = 0;
    int32_t total = 0;
    int32_t lap = 0;
    int32_t position = 0;
    uint32_t minutes = 0;
    uint32_t seconds = 0;
    int32_t hundredths = 0;
    uint8_t* race = g_gameMode;
    if (self->start != 0 && race && race[4] && *reinterpret_cast<int32_t*>(race + 0x3C) == 0xE) {
        laps = *reinterpret_cast<int32_t*>(race + 0x11C);
        total = *reinterpret_cast<int32_t*>(race + 0x120);
        lap = *reinterpret_cast<int32_t*>(CRace_GetEntry(race, g_playerPed) + 8) + 1;
        position = CRace_GetPosition(race, g_playerPed);
        if (!CRace_HasFinished(race, g_playerPed) &&
            (!race[0x50C] || !(*reinterpret_cast<int32_t*>(race + 0x124) > 0))) {
            self->elapsed = g_timeInMilliseconds - self->start;
        }
        if (lap > laps) {
            lap = laps;
        }
        uint32_t ms = self->elapsed;
        hundredths = int(float(int32_t(ms % 1000)) * 0.1f);
        uint32_t secs = uint32_t(int64_t(float(double(ms) * double(0.001f))));
        minutes = secs / 60;
        seconds = secs % 60;
    }
    // the digits' pitch: the widest
    uint16_t buffer[0x100];
    buffer[0] = u'0';
    buffer[1] = 0;
    float pitch = Font_GetStringWidth(buffer, 0);
    for (int i = 0; i < 10; i++) {
        buffer[0] = uint16_t(u'0' + i);
        buffer[1] = 0;
        float width = Font_GetStringWidth(buffer, 0);
        if (!(pitch >= width)) {
            pitch = width;
        }
    }
    // the box under it all
    col::CRect box;
    box.left = 1000000.0f;
    box.bottom = 1000000.0f;
    box.right = -1000000.0f;
    box.top = 22.0f;
    float height = Font_GetHeight();
    box.bottom = ((height + height) + 25.0f) + 5.0f;
    float four = pitch * 4.0f;
    box.left = ((sx * -110.0f + 320.0f) - four) - 6.0f;
    box.right = ((sx * 110.0f + 320.0f) + four) + 6.0f;
    DrawRect(&box, &g_raceBoxColour);

    // the time
    CVector2D pos;
    ScreenPosition(&pos, -110.0f, 25.0f, 1, 0);
    Font_FormatText(buffer, Text_Get(kTheText, "RACE_TIMEH"), 0xFF);
    PrintOutlined(pos.x, pos.y, buffer, text);
    pos.y = (Font_GetHeight() + 2.0f) + pos.y;
    if (self->start == 0) {
        Font_FormatText(buffer, Text_Get(kTheText, "RACE_NOTIMER"), 0xFF);
    } else {
        Font_AddNumber(float(int32_t(minutes)));
        Font_AddNumber(float(int32_t(seconds)));
        Font_AddNumber(float(hundredths));
        Font_FormatText(buffer, Text_Get(kTheText, "RACE_TIMER"), 0xFF);
    }
    PrintDigits(pos.x, pos.y, buffer, pitch, text);

    // the position
    CVector2D tmp;
    CVector2D* p = ScreenPosition(&tmp, 0.0f, 25.0f, 1, 0);
    pos.x = p->x;
    pos.y = p->y;
    Font_FormatText(buffer, Text_Get(kTheText, "RACE_POSH"), 0xFF);
    PrintOutlined(pos.x, pos.y, buffer, text);
    pos.y = (Font_GetHeight() + 2.0f) + pos.y;
    if (self->start == 0) {
        Font_FormatText(buffer, Text_Get(kTheText, "RACE_NOPOS"), 0xFF);
    } else {
        Font_AddNumber(float(position));
        Font_AddNumber(float(total));
        Font_FormatText(buffer, Text_Get(kTheText, "RACE_POS"), 0xFF);
    }
    PrintDigits(pos.x, pos.y, buffer, pitch, text);

    // the lap
    p = ScreenPosition(&tmp, 110.0f, 25.0f, 1, 0);
    pos.x = p->x;
    pos.y = p->y;
    Font_FormatText(buffer, Text_Get(kTheText, "RACE_LAPH"), 0xFF);
    PrintOutlined(pos.x, pos.y, buffer, text);
    pos.y = (Font_GetHeight() + 2.0f) + pos.y;
    if (self->start == 0) {
        Font_FormatText(buffer, Text_Get(kTheText, "RACE_NOLAP"), 0xFF);
    } else {
        Font_AddNumber(float(lap));
        Font_AddNumber(float(laps));
        Font_FormatText(buffer, Text_Get(kTheText, "RACE_LAP"), 0xFF);
    }
    PrintDigits(pos.x, pos.y, buffer, pitch, text);

    if (!HUD_Fn5142D0()) {
        float centre = float(double(sx) * 0.0 + 320.0);
        pos.x = centre;
        float h = Font_GetHeight();
        pos.y = ((h + h) + 0.0f) + pos.y;
        if (self->lapTimeSet + 2500 > g_timeInMilliseconds) {
            if (g_fontStyleLoaded[2] != 0) {
                g_fontStyle = 2;
                g_fontEdge = 0;
            }
            Font_AddNumber(float(double(self->lapTime) / 1000.0));
            Font_FormatText(buffer, Text_Get(kTheText, "RACE_LAPTIME"), 0xFF);
            PrintBoxed(pos.x, pos.y, buffer, text);
        }
        pos.x = centre;
        h = Font_GetHeight();
        pos.y = ((h + h) + 0.0f) + pos.y;
        if (self->splitTime + 1000 > g_timeInMilliseconds) {
            // red behind, blue ahead (in all four bytes)
            CRGBA splitColour;
            RGBA_Construct(&splitColour, 0xB4, 0x5A, 0x5A, 0xFF);
            int32_t split = self->split;
            const char* format;
            if (split < 0) {
                CRGBA blue;
                RGBA_Construct(&blue, 0x5A, 0x5A, 0xB4, 0xFF);
                splitColour = blue;
                format = "%4.2f";
            } else {
                format = "+%4.2f";
            }
            // (the quotient rounded to a float, then widened for the format)
            char ascii[0x100];
            std::snprintf(ascii, sizeof(ascii), format, double(float(double(split) / 1000.0)));
            AsciiToText(ascii, buffer);
            PrintBoxed(pos.x, pos.y, buffer, splitColour);
        }
    }
    Font_Render();
}

} // namespace

#define HUDRACING_REVERSED(name, address, fn)                                                       static void* RE_CONCAT(hudracing_original_, __LINE__);                                           REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(hudracing_original_, __LINE__),              re::Mode::Original)

HUDRACING_REVERSED("HUDRacing::Draw", 0x00546A30, &HUDRacing_Draw);
