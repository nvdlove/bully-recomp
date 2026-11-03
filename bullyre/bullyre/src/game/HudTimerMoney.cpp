// The HUD's mission timer (a clock face, or digits), the money counter, the monitor
// overlay (a noisy screen with lines of text) and the newspaper counter (a paper flying
// into the pile). 0x541F50..0x543B50 (their constructors, Setup and Reset are Hud.cpp's).

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "game/Collision.h"
#include "game/Hud.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

struct CVector2D {
    float x, y;
};

// Newspaper, with its float angle (Hud.h has +0x54 as an int)
struct Newspaper : HUDComponent {
    col::CRect rect;          // +0x20 the flying paper
    col::CRect rect2;         // +0x30 the pile (its left: where the paper lands)
    int32_t spin;             // +0x40 degrees a frame
    float textX, textY;       // +0x44
    int32_t max;              // +0x4C
    int32_t count;            // +0x50
    float angle;              // +0x54 degrees
    float flyX, flyY;         // +0x58
    int32_t speed;            // +0x60 pixels a frame
    uint8_t showPaper;        // +0x64
    uint8_t flying;           // +0x65
    uint8_t unknown66[2];
};
static_assert(offsetof(Newspaper, flyX) == 0x58);
static_assert(sizeof(Newspaper) == sizeof(HUDNewspaper));

GAME_FN(void, __thiscall, RGBA_Construct, 0x00510DE0, CRGBA* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(CVector2D*, __cdecl, ScreenPosition, 0x0052D4F0, CVector2D* out, float x, float y, int hAlign, int vAlign);
GAME_FN(void, __cdecl, DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture, const CRGBA* colour,
        const CVector2D* uv1, const CVector2D* uv2);
GAME_FN(void, __cdecl, DrawSprite2D, 0x0052CCC0, float x0, float y0, float x1, float y1, float x2, float y2, float x3,
        float y3, void* texture, const CRGBA* colour, const CVector2D* uv1, const CVector2D* uv2);
GAME_FN(void, __cdecl, DrawRing, 0x0052D390, const CVector2D* centre, float outer, float inner, const CRGBA* colour,
        int segments, float start, float end);
GAME_FN(void, __cdecl, DrawPie, 0x0052D200, const CVector2D* centre, float radius, const CRGBA* colour, int segments,
        float start, float end);
GAME_FN(void, __cdecl, SetInsetRects, 0x0052AC90, float, float, float, float, float, float, float, float);
GAME_FN(void, __cdecl, DrawQuad2D, 0x00739820, float, float, float, float, float, float, float, float, float, float,
        float, float);
GAME_FN(void, __cdecl, Fn_5EE1D0, 0x005EE1D0);
inline int __cdecl Rand() { return crt::Rand(); }
// the RW emulation's rasters
GAME_FN(void*, __cdecl, RwRasterCreate, 0x00519F60, int width, int height, int depth, int flags);
GAME_FN(uint8_t*, __cdecl, RwRasterLock, 0x0051A250, void* raster, int level, int mode);
GAME_FN(void*, __cdecl, RwRasterUnlock, 0x0051A290, void* raster);
GAME_FN(void, __cdecl, RwRasterDestroy, 0x00519090, void* raster);
// textures
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(int, __cdecl, Txd_FindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(int, __cdecl, Txd_AddTxdSlot, 0x005F26E0, const char* name);
GAME_FN(void, __cdecl, Txd_RemoveTxdSlot, 0x005F2240, int slot);
GAME_FN(void, __cdecl, Txd_LoadTxdFile, 0x005F2A80, int slot, const char* path);
GAME_FN(void, __cdecl, Txd_AddRef, 0x005F1130, int slot);
GAME_FN(void, __cdecl, Txd_RemoveRef, 0x005F1160, int slot);
GAME_FN(void, __cdecl, Txd_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, Txd_PopCurrentTxd, 0x005F0AA0);
GAME_FN(void, __cdecl, Txd_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void*, __cdecl, Txd_FindTexture, 0x005F1770, const char* name, const char* mask);
GAME_FN(uint8_t, __cdecl, NiRelease, 0x005F0F90, void* object);
// text
GAME_FN(const uint16_t*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __cdecl, AsciiToText, 0x00690090, const char* ascii, uint16_t* text);
GAME_FN(int, __cdecl, TextLength, 0x00690020, const uint16_t* text);
GAME_FN(void, __cdecl, Font_Begin, 0x00688E60);
GAME_FN(void, __cdecl, Font_SetScale, 0x00688080, float scale);
GAME_FN(void, __cdecl, Font_SetJustify, 0x00688160, int justify);
GAME_FN(void, __cdecl, Font_SetSizePreset, 0x00688050, int preset);
GAME_FN(void, __cdecl, Font_SetColour, 0x00424480, const CRGBA* colour);
GAME_FN(void, __cdecl, Font_SetDropColour, 0x0043B110, const CRGBA* colour);
GAME_FN(void, __cdecl, Font_AddNumber, 0x006880D0, float number);
GAME_FN(void, __cdecl, Font_FormatText, 0x00688ED0, uint16_t* out, const uint16_t* format, int size);
GAME_FN(float, __cdecl, Font_GetStringWidth, 0x0068A920, const uint16_t* text, int);
GAME_FN(float, __cdecl, Font_GetHeight, 0x00688220, const uint16_t* text, int);
GAME_FN(void, __cdecl, Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __cdecl, Font_Render, 0x0068CDF0);
// the rest
GAME_FN(float, __thiscall, CMissionTimer_GetSecondsLeft, 0x0043F6F0, void* timer);
GAME_FN(void, __cdecl, EmptyFunction, 0x0049A650, int);
GAME_FN(uint8_t, __thiscall, CShop_IsShopInterior, 0x00455850, void* shop);
GAME_FN(uint8_t, __thiscall, CShop_IsInArea50, 0x004558A0, void* shop);
GAME_FN(uint8_t*, __thiscall, CPed_GetTargetPed, 0x0046ECE0, void* ped);
GAME_FN(void, __thiscall, CPed_GetGift, 0x00474AA0, void* ped, int32_t* gift);
GAME_FN(int32_t, __cdecl, Items_GetCount, 0x00541E00, int item);
GAME_FN(void, __thiscall, Inventory_Show, 0x00541110, void* self, int item, int from, int to, int always);
GAME_FN(void, __thiscall, HUDComponent_SetVisible, 0x0053CEB0, HUDComponent* self, int visible);
GAME_FN(void, __thiscall, HUDComponent_SetTextJustify, 0x0053CE40, HUDComponent* self, int wrap);
GAME_FN(void, __cdecl, HUD_SaveVisibility2, 0x00514130);
GAME_FN(void, __cdecl, HUD_HideAllButScriptText, 0x00514160);
GAME_FN(void, __cdecl, HUD_RestoreVisibility2, 0x00514190);
// its own, where modes apply
GAME_FN(void, __thiscall, HUDMoney_Update, 0x005429B0, HUDMoney* self);
GAME_FN(void, __thiscall, HUDMonitor_Unload, 0x00542F70, HUDMonitor* self);
GAME_FN(void, __thiscall, HUDMonitor_Load, 0x00543030, HUDMonitor* self);
GAME_FN(void, __thiscall, HUDNewspaper_DrawFlying, 0x00543600, Newspaper* self);

GAME_VAR(uint32_t, g_timeInMilliseconds, 0x00C1A9B4);
GAME_VAR(int32_t, g_language, 0x00C18724);
GAME_VAR(uint8_t, g_userPause, 0x00C1A99A);
GAME_VAR(uint8_t*, g_playerPed, 0x00C1AEA8);
inline uint8_t* const kMissionTimer = reinterpret_cast<uint8_t*>(0x00BF4508);  // (+0xC ms, +0x19 running, +0x1A paused)
inline uint8_t* const kShop = reinterpret_cast<uint8_t*>(0x00C18748);          // (+0 its kind, +0xC active)
inline void* const kTheText = reinterpret_cast<void*>(0x020C2FA4);
GAME_VAR(uint8_t*, g_hudHealth, 0x00C674D0);         // (+0xBB: the meters hidden)
GAME_VAR(HUDComponent*, g_hudRadar, 0x00C674CC);
GAME_VAR(HUDComponent*, g_hud_C674B8, 0x00C674B8);
GAME_VAR(HUDComponent*, g_hud_C67534, 0x00C67534);
GAME_VAR(HUDComponent*, g_hud_C674D8, 0x00C674D8);
GAME_VAR(HUDComponent*, g_hud_C674E8, 0x00C674E8);
GAME_VAR(HUDComponent*, g_hud_C674EC, 0x00C674EC);
GAME_VAR(void*, g_hudInventory, 0x00C67554);
GAME_VAR(float, g_twoPi, 0x00AB4624);
GAME_VAR(float, g_timerDigitsScale, 0x00AB4620);     // (1.0; 0.7 once drawn in language 7)
GAME_VAR(CRGBA, g_timerTextColour, 0x00CF01A0);
GAME_VAR(CRGBA, g_moneyMinusColour, 0x00CF01A4);
GAME_VAR(CRGBA, g_moneyPlusColour, 0x00CF01A8);
GAME_VAR(int32_t, g_itemMoney2, 0x00BF44E8);
GAME_VAR(int32_t, g_itemTickets, 0x00BF42A4);
GAME_VAR(void*, g_monitorNoise, 0x00CF01AC);         // a 64x64 raster
GAME_VAR(uint8_t, g_monitorUnder, 0x00CF01B0);       // the monitor drawn under the noise
GAME_VAR(void*, g_quadRaster, 0x020DCCC8);           // DrawQuad2D's
GAME_ARRAY(uint32_t, g_quadColours, 0x020DCCF4);     // [4]
GAME_VAR(void*, g_newspaperPile, 0x00C66E64);
GAME_VAR(void*, g_newspaperPaper, 0x00C66E6C);
GAME_ARRAY(uint8_t, g_fontColour, 0x0208B764);
GAME_ARRAY(uint8_t, g_fontDropColour, 0x0208B780);
GAME_VAR(int16_t, g_fontStyle, 0x0208B788);
GAME_VAR(int16_t, g_fontEdge, 0x0208B784);
GAME_VAR(uint8_t, g_fontFlags, 0x0208B78A);
GAME_ARRAY(int32_t, g_fontStyleLoaded, 0x0208B218);

template <typename R, typename... A>
R VCall(HUDComponent* self, int slot, A... args) {
    return reinterpret_cast<R(__thiscall*)(HUDComponent*, re::Arg<A>...)>(static_cast<void*>(self->vtable[slot]))(self, args...);
}

// ---- the mission timer ---------------------------------------------------------------------

// 0x541F50 (vf8 Draw): a clock face running down (its hand red and flashing in the last
// 30 seconds), or with the meters hidden, digits
void __fastcall HUDMissionTimer_Draw(HUDMissionTimer* self, re::Edx) {
    if (!kMissionTimer[0x19] || kMissionTimer[0x1A] || g_userPause) {
        self->unknown44 = 0;
        return;
    }
    float left = CMissionTimer_GetSecondsLeft(kMissionTimer);
    float total = float(int32_t(*reinterpret_cast<uint32_t*>(kMissionTimer + 0xC) / 1000));
    if (left < 0.0f || left == 0.0f || !(total > 0.0f)) {
        self->unknown44 = 0;
        return;
    }
    char* text = reinterpret_cast<char*>(&self->unknown44);
    if (!g_hudHealth[0xBB]) {
        float angle = left / total;
        angle = angle * g_twoPi;
        CRGBA face, red;
        RGBA_Construct(&face, 0x10, 0x10, 0x10, 0xFF);
        RGBA_Construct(&red, 0x96, 0, 0, 0xFF);
        CRGBA hand = self->colour;
        if (!(left > 30.0f) && !std::isnan(left)) {
            // flashing twice a second, towards red
            float twice = left + left;
            float fraction = twice - float(int(twice));
            float t = 1.0f - fraction;
            float p;
            if (!(t <= 0.1f)) {
                float u = float(double(t - 0.1f) / 0.8999999985098839);   // (1 - 0.1f, in double)
                p = 1.0f - u * u;
            } else {
                float u = t / 0.1f;
                p = u * u;
            }
            float hr = float(hand.r);
            hand.r = uint8_t(int((float(red.r) - hr) * p + hr));
            float hg = float(hand.g);
            hand.g = uint8_t(int(hg + (float(red.g) - hg) * p));
            float hb = float(hand.b);
            hand.b = uint8_t(int(hb + (float(red.b) - hb) * p));
            float ha = float(hand.a);
            hand.a = uint8_t(int(ha + p * (float(red.a) - ha)));
        }
        CRGBA shadow, back;
        RGBA_Construct(&shadow, 0x10, 0x10, 0x10, 0x80);
        RGBA_Construct(&back, 0x32, 0x32, 0x32, 0);
        const auto* centre = reinterpret_cast<const CVector2D*>(&self->centreX);
        if (!VCall<uint8_t>(g_hud_C674B8, 4)) {
            DrawPie(centre, 42.0f, &shadow, self->unknown3C, 0.0f, g_twoPi);
        }
        DrawRing(centre, 35.0f, 40.0f, &back, self->unknown3C, angle, g_twoPi);
        DrawRing(centre, 35.0f, 40.0f, &hand, self->unknown3C, 0.0f, angle);
        if (text[0]) {
            Font_Begin();
            Font_SetSizePreset(1);
            Font_SetColour(&g_timerTextColour);
            Font_SetJustify(2);
            float width = std::fabs(self->rect.right - self->rect.left);
            const uint16_t* label = Text_Get(kTheText, text);
            Font_PrintString(width * 0.5f + self->rect.right, self->rect.top + 60.0f, label, 1);
        }
        return;
    }

    // digits, each at the widest digit's pitch
    if (g_language == 7) {
        g_timerDigitsScale = 0.7f;
    }
    CRGBA labelColour, digitColour, drop;
    RGBA_Construct(&labelColour, 0xEB, 0xEB, 0x10, 0xFF);
    RGBA_Construct(&digitColour, 0xEB, 0xEB, 0x10, 0xFF);
    RGBA_Construct(&drop, 0x10, 0x10, 0x10, 0xFF);
    Font_Begin();
    if (g_fontStyleLoaded[1] != 0) {
        g_fontStyle = 1;
    }
    g_fontEdge = 2;
    Font_SetDropColour(&drop);
    Font_SetScale(g_timerDigitsScale);
    Font_SetJustify(2);
    Font_SetColour(&digitColour);
    int minutes = int(left / 60.0f);
    float fminutes = float(minutes);
    int seconds = int(left - 60.0f * fminutes);
    uint16_t digits[0x40];
    digits[0] = u'0';
    digits[1] = 0;
    float pitch = Font_GetStringWidth(digits, 0);
    for (int i = 0; i < 10; i++) {
        digits[0] = uint16_t(u'0' + i);
        digits[1] = 0;
        float width = Font_GetStringWidth(digits, 0);
        if (!(pitch >= width)) {
            pitch = width;
        }
    }
    static const uint16_t kFormat[] = {u'~', u'i', u'~', u':', u'~', u'z', u'~', 0};
    uint16_t format[8];
    std::memcpy(format, kFormat, sizeof(format));
    Font_AddNumber(fminutes);
    Font_AddNumber(float(seconds));
    Font_FormatText(digits, format, 0x40);
    CVector2D pos;
    ScreenPosition(&pos, 0.0f, 45.0f, 1, 0);
    int length = TextLength(digits);
    g_fontFlags |= 0x20;
    pos.x = (float(length) * pitch * 0.5f - pitch * 0.5f) + pos.x;
    for (int i = length; i > 0; i--) {
        Font_PrintString(pos.x, pos.y, &digits[i - 1], 1);
        digits[i - 1] = 0;
        pos.x = pos.x - pitch;
    }
    g_fontFlags &= 0xDF;
    if (text[0]) {
        Font_Begin();
        if (g_fontStyleLoaded[0] != 0) {
            g_fontStyle = 0;
        }
        g_fontEdge = 2;
        std::memcpy(g_fontDropColour, &drop, 4);
        Font_SetScale(0.55f);
        Font_SetJustify(2);
        g_fontFlags |= 0x20;
        std::memcpy(g_fontColour, &labelColour, 4);
        CVector2D labelPos;
        ScreenPosition(&labelPos, 0.0f, 25.0f, 1, 0);
        Font_PrintString(labelPos.x, labelPos.y, Text_Get(kTheText, text), 1);
        g_fontFlags &= 0xDF;
    }
}

// ---- money ----------------------------------------------------------------------------------

// 0x542810: shown, and kept up
void __fastcall HUDMoney_ShowAlways(HUDMoney* self, re::Edx) {
    VCall<void>(self, 5, 1);
    self->unknown30 = 1;
}

// 0x542890 (vf4 IsVisible): while counting, in a shop, or giving a gift the player
// pays for; not under some of the HUD
uint8_t __fastcall HUDMoney_IsVisible(HUDMoney* self, re::Edx) {
    if (!CShop_IsShopInterior(kShop) && !CShop_IsInArea50(kShop)) {
        int shown = int(int64_t(self->shownMoney));
        uint8_t* player = g_playerPed;
        if (shown != *reinterpret_cast<int32_t*>(player + 0x1CA0) || self->countEndTime > g_timeInMilliseconds) {
            self->visible = 1;
            return self->visible;
        }
        if (VCall<uint8_t>(g_hud_C67534, 4) == 1) {
            uint8_t* target = CPed_GetTargetPed(player);
            if (!target) {
                return self->visible;
            }
            int32_t gift;
            CPed_GetGift(target, &gift);
            if (gift == 0x16 || gift == 0x17 || gift == 0x18) {
                self->visible = 1;
            }
            return self->visible;
        }
        if (!self->unknown30) {
            self->visible = 0;
        }
        return self->visible;
    }
    if (!VCall<uint8_t>(g_hud_C674D8, 4) && !VCall<uint8_t>(g_hud_C674E8, 4) && !VCall<uint8_t>(g_hud_C674EC, 4)) {
        int32_t kind = *reinterpret_cast<int32_t*>(kShop);
        if (!kShop[0xC] || (kind != 1 && kind != 3)) {
            self->visible = 1;
            return self->visible;
        }
    }
    self->visible = 0;
    return self->visible;
}

// 0x5429B0: the shown amount counted towards the player's money
void __fastcall HUDMoney_Update_(HUDMoney* self, re::Edx) {
    uint32_t now = g_timeInMilliseconds;
    int money = *reinterpret_cast<int32_t*>(g_playerPed + 0x1CA0);
    if (self->money != money) {
        float difference = float(money - self->money);
        self->countFrom = difference;
        self->countEndTime = uint32_t(int(float(double(self->countSeconds * 1000.0f) + double(int32_t(now)))));
        Inventory_Show(g_hudInventory, g_itemMoney2, 0, int(difference), 0);
    }
    float target = float(money);
    float duration = self->countSeconds * 1000.0f;
    float shown = target - float(int32_t(self->countEndTime - now)) / duration * self->countFrom;
    self->shownMoney = shown;
    if (std::fabs(shown - target) < 1.0f || !(int32_t(now) < int32_t(self->countEndTime))) {
        self->shownMoney = target;
    }
    if (self->shownMoney == target && self->countFrom != 0.0f) {
        self->countFrom = 0.0f;
        self->countEndTime = uint32_t(int(float(double(duration) + double(int32_t(now)))));
    }
    self->money = money;
}

// 0x542BC0 (vf8 Draw): the money (tickets in the carnival), and the change under it
void __fastcall HUDMoney_Draw(HUDMoney* self, re::Edx) {
    uint8_t tickets = CShop_IsInArea50(kShop);
    uint32_t now = g_timeInMilliseconds;
    HUDMoney_Update(self);
    Font_Begin();
    uint16_t text[0x10];
    if (tickets) {
        Font_AddNumber(float(Items_GetCount(g_itemTickets)));
        Font_FormatText(text, Text_Get(kTheText, "TICKETS"), 0x10);
    } else {
        Font_AddNumber(float(int(self->shownMoney)));
        Font_FormatText(text, Text_Get(kTheText, "MONEY"), 0x10);
        g_fontFlags &= 0xFD;
    }
    std::memcpy(g_fontColour, &self->colour, 4);
    CRGBA drop;
    RGBA_Construct(&drop, 0x10, 0x10, 0x10, 0xFF);
    HUDComponent_SetTextJustify(self, 0);
    Font_SetSizePreset(1);
    std::memcpy(g_fontDropColour, &drop, 4);
    Font_PrintString(float(self->x), float(self->y), text, 1);
    Font_Render();
    if (self->countFrom != 0.0f) {
        EmptyFunction(0);
        CRGBA colour;
        if (0.0f > self->countFrom) {
            text[0] = u'-';
            colour = g_moneyMinusColour;
        } else {
            text[0] = u'+';
            colour = g_moneyPlusColour;
        }
        std::memcpy(g_fontColour, &colour, 4);
        Font_AddNumber(std::fabs(float(int(self->countFrom))));
        Font_FormatText(&text[1], Text_Get(kTheText, "MONEY"), 0x10);
        float height = Font_GetHeight(text, 1);
        Font_PrintString(float(self->x), height + float(self->y), text, 1);
    }
    g_fontFlags |= 2;
    if (now > self->countEndTime && !self->unknown30) {
        VCall<void>(self, 5, 0);
    }
}

// ---- the monitor ----------------------------------------------------------------------------

// 0x542F70: its texture and noise let go
void __fastcall HUDMonitor_Unload_(HUDMonitor* self, re::Edx) {
    RwRasterDestroy(g_monitorNoise);
    int slot = Txd_FindTxdSlot("HudMntor", 0);
    Txd_RemoveRef(slot);
    Txd_RemoveTxdSlot(slot);
    NiRelease(self->texture);
    self->texture = nullptr;
}

// 0x543030: its texture loaded, the noise made (grey, random)
void __fastcall HUDMonitor_Load_(HUDMonitor* self, re::Edx) {
    PushMemoryTag(8);
    Txd_FindTxdSlot("HudMontr", 0);   // (unused)
    int slot = Txd_AddTxdSlot("HudMntor");
    Txd_LoadTxdFile(slot, "TXD/HudMontr.nft");
    Txd_AddRef(slot);
    Txd_PushCurrentTxd();
    Txd_SetCurrentTxd(slot);
    self->texture = Txd_FindTexture("monitor", nullptr);
    Txd_PopCurrentTxd();
    PopMemoryTag();
    g_monitorNoise = RwRasterCreate(0x40, 0x40, 0x20, 0x504);
    uint8_t* pixel = RwRasterLock(g_monitorNoise, 0, 1);
    for (int i = 0; i < 0x1000; i++, pixel += 4) {
        uint8_t v = uint8_t(Rand());
        pixel[3] = v;
        pixel[2] = v;
        pixel[1] = v;
        pixel[0] = v;
    }
    RwRasterUnlock(g_monitorNoise);
}

void DrawMonitorTexture(HUDMonitor* self) {
    Fn_5EE1D0();
    SetRenderState(0xC, 1);
    SetRenderState(9, 2);
    SetRenderState(8, 0);
    SetRenderState(6, 0);
    SetRenderState(10, 5);
    SetRenderState(11, 6);
    CVector2D uv2 = {1.0f, 1.0f};
    CVector2D uv1 = {0.0f, 0.0f};
    CRGBA white;
    RGBA_Construct(&white, 0xFF, 0xFF, 0xFF, 0xFF);
    DrawRectUV(0.0f, 0.0f, 640.0f, 480.0f, self->texture, &white, &uv1, &uv2);
}

// 0x5430E0 (vf8 Draw): the monitor over (or under) fresh noise, scanlines on every
// other row, and its lines of text
void __fastcall HUDMonitor_Draw(HUDMonitor* self, re::Edx) {
    if (g_monitorUnder) {
        DrawMonitorTexture(self);
    }
    Fn_5EE1D0();
    SetRenderState(0xC, 1);
    SetRenderState(9, 2);
    SetRenderState(8, 0);
    SetRenderState(6, 0);
    SetRenderState(10, 5);
    SetRenderState(11, 6);
    uint8_t* pixel = RwRasterLock(g_monitorNoise, 0, 1);
    for (int row = 0; row < 0x40; row++) {
        int odd = row & 1;
        for (int i = 0; i < 0x40; i++, pixel += 4) {
            uint8_t v = uint8_t(Rand());
            pixel[2] = v;
            pixel[1] = v;
            pixel[0] = v;
            pixel[3] = uint8_t((odd ? 0 : 0xFF) & 0x1F);
            pixel[3] = uint8_t(pixel[3] + (Rand() & 0xF));
        }
    }
    RwRasterUnlock(g_monitorNoise);
    g_quadRaster = g_monitorNoise;
    SetRenderState(10, 5);
    SetRenderState(11, 6);
    SetRenderState(9, 1);
    SetRenderState(2, 1);
    SetRenderState(1, int(re::PtrCast<uintptr_t>(g_monitorNoise)));
    SetInsetRects(0.0f, 0.0f, 160.0f, 0.0f, 0.0f, 224.0f, 160.0f, 224.0f);
    SetRenderState(0xC, 1);
    CRGBA grey = {0x40, 0x40, 0x40, 0xFF};
    uint32_t colour;
    std::memcpy(&colour, &grey, 4);
    for (int i = 0; i < 4; i++) {
        g_quadColours[i] = colour;
    }
    DrawQuad2D(0.0f, 0.0f, 0.0f, 639.0f, 0.0f, 0.0f, 0.0f, 447.0f, 0.0f, 639.0f, 447.0f, 0.0f);
    if (!g_monitorUnder) {
        DrawMonitorTexture(self);
    }
    Font_Begin();
    Font_SetJustify(self->unknown28);
    Font_SetSizePreset(1);
    for (HUDMonitor::Line& line : self->lines) {
        if (std::strlen(line.text) == 0) {
            continue;
        }
        CRGBA c;
        if (line.unknown19) {
            RGBA_Construct(&c, 100, 100, 100, 0xFF);
        } else {
            RGBA_Construct(&c, 200, 200, 200, 0xFF);
        }
        std::memcpy(g_fontColour, &c, 4);
        const uint16_t* text = Text_Get(kTheText, line.text);
        Font_PrintString(line.x + 15.0f, line.y + 20.0f, text, 1);
        Font_Render();
    }
}

// 0x543520 (vf5 SetVisible): loaded when shown (the radar and the meters brought back
// with it), unloaded when hidden
void __fastcall HUDMonitor_SetVisible(HUDMonitor* self, re::Edx, int visible) {
    if (uint8_t(visible)) {
        if (!self->loaded) {
            HUD_SaveVisibility2();
            HUD_HideAllButScriptText();
            HUDComponent* radar = g_hudRadar;
            auto* health = re::PtrCast<HUDComponent*>(g_hudHealth);
            radar->suppressed = 0;
            health->suppressed = 0;
            VCall<void>(radar, 5, 1);
            VCall<void>(health, 5, 1);
            self->suppressed = 0;
            HUDMonitor_Load(self);
            self->loaded = 1;
            HUDComponent_SetVisible(self, visible);
            return;
        }
    } else if (self->loaded) {
        HUDMonitor_Unload(self);
        self->loaded = 0;
        HUD_RestoreVisibility2();
    }
    HUDComponent_SetVisible(self, visible);
}

// ---- the newspapers ---------------------------------------------------------------------------

// 0x543600: the paper flying (spinning) over to the pile; done there
void __fastcall HUDNewspaper_DrawFlying_(Newspaper* self, re::Edx) {
    void* texture = g_newspaperPaper;
    if (!(self->rect2.left > self->flyX)) {
        self->flyX = self->rect.left;
        self->flying = 0;
        self->angle = 0.0f;
        return;
    }
    float x = float(self->speed) + self->flyX;
    self->flyX = x;
    float angle = float(self->spin) + self->angle;
    self->angle = angle;
    if (360.0f < angle) {
        self->angle = angle - 360.0f;
    }
    float w = std::fabs(self->rect.right - self->rect.left);
    float px[4] = {x, x, w + x, x + w};
    float h = std::fabs(self->rect.bottom - self->rect.top);
    float py[4] = {self->flyY, h + self->flyY, h + self->flyY, self->flyY};
    float radians = self->angle * 3.14159274f / 180.0f;
    float c = float(std::cos(double(radians)));
    float s = float(std::sin(double(radians)));
    float halfW = std::fabs(self->rect.right - self->rect.left) * 0.5f;
    float halfH = 0.5f * std::fabs(self->rect.bottom - self->rect.top);
    float cx = halfW + self->flyX;
    float cy = self->flyY + halfH;
    float qx[4], qy[4];
    for (int i = 0; i < 4; i++) {
        float dx = (px[i] - self->flyX) - halfW;
        float dy = (py[i] - self->flyY) - halfH;
        qx[i] = (dx * c - dy * s) + cx;
        qy[i] = (dx * s + dy * c) + cy;
    }
    CVector2D uv2 = {1.0f, 1.0f};
    CVector2D uv1 = {0.0f, 0.0f};
    CRGBA white;
    RGBA_Construct(&white, 0xFF, 0xFF, 0xFF, 0xFF);
    DrawSprite2D(qx[0], qy[0], qx[1], qy[1], qx[2], qy[2], qx[3], qy[3], texture, &white, &uv1, &uv2);
}

// 0x543B50 (vf8 Draw): the pile, "count/max", the flying paper, the paper
void __fastcall HUDNewspaper_Draw(Newspaper* self, re::Edx) {
    void* paper = g_newspaperPaper;
    void* pile = g_newspaperPile;
    CVector2D uv2 = {1.0f, 1.0f};
    CVector2D uv1 = {0.0f, 0.0f};
    float height = std::fabs(self->rect2.bottom - self->rect2.top);
    CRGBA white;
    RGBA_Construct(&white, 0xFF, 0xFF, 0xFF, 0xFF);
    DrawRectUV(self->rect2.left, self->rect2.top, self->rect2.right, height + self->rect2.top, pile, &white, &uv1,
               &uv2);
    char ascii[0x18];
    std::snprintf(ascii, sizeof(ascii), "%d/%d", self->count, self->max);
    uint16_t text[0x18];
    AsciiToText(ascii, text);
    Font_Begin();
    CRGBA yellow;
    RGBA_Construct(&yellow, 0xCC, 0xCC, 0, 0xFF);
    std::memcpy(g_fontColour, &yellow, 4);
    HUDComponent_SetTextJustify(self, 0);
    Font_PrintString(self->textX, self->textY, text, 1);
    if (self->flying) {
        HUDNewspaper_DrawFlying(self);
    }
    if (self->showPaper) {
        CVector2D uv2b = {1.0f, 1.0f};
        CVector2D uv1b = {0.0f, 0.0f};
        float h = std::fabs(self->rect.bottom - self->rect.top);
        CRGBA white2;
        RGBA_Construct(&white2, 0xFF, 0xFF, 0xFF, 0xFF);
        DrawRectUV(self->rect.left, self->rect.top, self->rect.right, self->rect.top + h, paper, &white2, &uv1b, &uv2b);
    }
}

} // namespace

#define HUDTM_REVERSED(name, address, fn)                                                       \
    static void* RE_CONCAT(hudtm_original_, __LINE__);                                           \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(hudtm_original_, __LINE__), \
             re::Mode::Original)

HUDTM_REVERSED("HUDMissionTimer::Draw", 0x00541F50, &HUDMissionTimer_Draw);
HUDTM_REVERSED("HUDMoney::ShowAlways", 0x00542810, &HUDMoney_ShowAlways);
HUDTM_REVERSED("HUDMoney::IsVisible", 0x00542890, &HUDMoney_IsVisible);
HUDTM_REVERSED("HUDMoney::Update", 0x005429B0, &HUDMoney_Update_);
HUDTM_REVERSED("HUDMoney::Draw", 0x00542BC0, &HUDMoney_Draw);
HUDTM_REVERSED("HUDMonitor::Unload", 0x00542F70, &HUDMonitor_Unload_);
HUDTM_REVERSED("HUDMonitor::Load", 0x00543030, &HUDMonitor_Load_);
HUDTM_REVERSED("HUDMonitor::Draw", 0x005430E0, &HUDMonitor_Draw);
HUDTM_REVERSED("HUDMonitor::SetVisible", 0x00543520, &HUDMonitor_SetVisible);
HUDTM_REVERSED("HUDNewspaper::DrawFlying", 0x00543600, &HUDNewspaper_DrawFlying_);
HUDTM_REVERSED("HUDNewspaper::Draw", 0x00543B50, &HUDNewspaper_Draw);
