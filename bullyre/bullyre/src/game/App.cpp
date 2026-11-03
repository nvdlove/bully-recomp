// The application side of the top level (0x43A590 on): the loading screen's state, the
// credits lines, the text lookups the front end uses, and the frame's small steps the main
// loop (0x43C960, 0x43D340) calls. The loading screen's drawing itself (0x43C4F0,
// 0x43C060, 0x43BA00) is x87 and renderer work, not here yet.

#include <cstdint>

#include "game/Game.h"
#include "game/Pool.h"
#include "reversed.h"

namespace {

GAME_VAR(uint8_t, ms_bWidescreen, 0x00C1870E);
GAME_VAR(int32_t, ms_lastChannelRead, 0x00CA3920);      // CStreaming's (-1: none)
GAME_VAR(uint8_t, ms_bReturnToMain_20C32EC, 0x020C32EC);
GAME_VAR(uint8_t, ms_bReturnToMain_20C32ED, 0x020C32ED);
GAME_VAR(int32_t, ms_clipRange0, 0x00A135DC);
GAME_VAR(int32_t, ms_clipRange1, 0x00A135E0);
GAME_VAR(int32_t, ms_numCreditLines, 0x00BF381C);
GAME_VAR(uint8_t, ms_bInputIgnoredDisplay, 0x00BF381A);
GAME_VAR(uint8_t, ms_bLoadingScreenPaused, 0x00BF3828);
GAME_VAR(int32_t, ms_loadingScreenTxd, 0x00A1360C);     // the splash's texture dictionary (-1: none)
GAME_VAR(void*, ms_loadingScreenTexture, 0x00BF3E08);
GAME_VAR(uint8_t, ms_bLoadingScreenNew, 0x00BF382A);
GAME_VAR(void*, ms_camera, 0x00C1AE7C);
GAME_VAR(void*, ms_pHud_C674B4, 0x00C674B4);
GAME_VAR(int32_t, ms_pending_CA064C, 0x00CA064C);
GAME_VAR(uint8_t, ms_bUserStart, 0x020C32E9);
GAME_VAR(uint8_t, ms_bFlag_20C32EA, 0x020C32EA);
GAME_VAR(uint8_t, ms_bFlag_20C32EB, 0x020C32EB);
GAME_VAR(uint8_t, ms_bFlag_20C32F2, 0x020C32F2);
GAME_VAR(uint8_t, ms_bPlayerDown_20C87A8, 0x020C87A8);
GAME_VAR(uint8_t, ms_bPaused_C1A998, 0x00C1A998);
GAME_VAR(uint8_t, ms_bPaused_C1A999, 0x00C1A999);
GAME_VAR(uint8_t, ms_bPaused_C1A99A, 0x00C1A99A);
GAME_ARRAY(uint8_t, ms_textColour, 0x0208B780);
GAME_VAR(CPool<uint8_t>*, ms_pTxdPool, 0x01B64654);
inline void* const TheText = reinterpret_cast<void*>(0x020C2FA4);
inline const uint16_t* const ms_missingText = reinterpret_cast<const uint16_t*>(0x0093F0E8);

// a credits line (0x18), at 0xBF3860
struct CreditLine {
    re::Ptr32<const uint16_t> text;   // +0
    float x;                // +4
    float y;                // +8
    uint8_t colour[4];      // +0xC
    float scale;            // +0x10
    int32_t font;           // +0x14
};
static_assert(sizeof(CreditLine) == 0x18);
GAME_ARRAY(CreditLine, ms_creditLines, 0x00BF3860);   // 60

// a streaming channel's read retries (CStreaming::ms_channel[n].tries)
inline int32_t* ChannelTries(int channel) {
    return reinterpret_cast<int32_t*>(0x00CA38E0 + channel * 0x38);
}

GAME_FN(const uint16_t*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(int, __cdecl, Game_ParseTrailingNumber, 0x0043A5C0, const uint16_t* s);
GAME_FN(void, __cdecl, Camera_Fn_6C86C0, 0x006C86C0, void* camera);
GAME_FN(void, __cdecl, Fn_4096C0, 0x004096C0);
GAME_FN(void, __cdecl, Fn_409750, 0x00409750);
GAME_FN(void, __cdecl, Fn_4097D0, 0x004097D0);
GAME_FN(void, __cdecl, Fn_409520, 0x00409520, int, int);
GAME_FN(void, __cdecl, Game_ClearTidiedTextures, 0x0042E6B0, int);
GAME_FN(uint32_t, __cdecl, Fn_6C80A0, 0x006C80A0);
GAME_FN(uint32_t, __cdecl, Fn_6F2E70, 0x006F2E70);
GAME_FN(uint32_t, __cdecl, Fn_5EF860, 0x005EF860);
GAME_FN(uint8_t, __cdecl, Fn_5F2AF0, 0x005F2AF0);
GAME_FN(void, __cdecl, TxdStore_RemoveTxdSlot, 0x005F2240, int slot);
GAME_FN(void, __cdecl, SetRenderState_519790, 0x00519790, int state, int value);
GAME_FN(void, __cdecl, Fn_5F3500, 0x005F3500);
GAME_FN(void, __cdecl, Fn_452C70, 0x00452C70);
GAME_FN(void, __cdecl, Fn_4E8EB0, 0x004E8EB0);
GAME_FN(void, __cdecl, Fn_452DE0, 0x00452DE0);
GAME_FN(void, __cdecl, Fn_450C60, 0x00450C60);
GAME_FN(void, __cdecl, Fn_450C30, 0x00450C30);
GAME_FN(void, __cdecl, Fn_513010, 0x00513010);
GAME_FN(void, __cdecl, Fn_4DC0B0, 0x004DC0B0);
GAME_FN(void, __cdecl, Fn_5231F0, 0x005231F0);
GAME_FN(void, __thiscall, Fn_72D520, 0x0072D520, void*, int);
GAME_FN(void, __cdecl, Fn_52AC70, 0x0052AC70, int);
GAME_FN(void, __cdecl, Fn_5251C0, 0x005251C0);
GAME_FN(void, __cdecl, Fn_527800, 0x00527800);
GAME_FN(void, __thiscall, Hud_Fn_53CF50, 0x0053CF50, void*);
GAME_FN(void, __cdecl, Fn_513020, 0x00513020);
GAME_FN(void, __cdecl, Fn_4E8CE0, 0x004E8CE0);
GAME_FN(void, __cdecl, Fn_452EC0, 0x00452EC0);
GAME_FN(void, __thiscall, Fn_513780, 0x00513780, void*);
GAME_FN(void, __cdecl, Fn_528FE0, 0x00528FE0);
GAME_FN(void, __cdecl, Fn_6A6A40, 0x006A6A40);
GAME_FN(void, __cdecl, Fn_68D6C0, 0x0068D6C0);
GAME_FN(void, __thiscall, Fn_4257A0, 0x004257A0, void*);
GAME_FN(void, __cdecl, Fn_4256B0, 0x004256B0);
GAME_FN(void, __cdecl, Fn_425B00, 0x00425B00);
GAME_FN(void, __cdecl, Fn_6C3A10, 0x006C3A10);
GAME_FN(void, __cdecl, Fn_68CDF0, 0x0068CDF0);
GAME_FN(void, __cdecl, Fn_707510, 0x00707510);
GAME_FN(void, __cdecl, Fn_43B310, 0x0043B310);

} // namespace

namespace App {

// 0x43A590 (thiscall, on the display settings): a mode other than the desktop's, 4:3
static uint32_t __fastcall IsModeChangePending(uint8_t* settings, re::Edx) {
    int32_t mode = *reinterpret_cast<int32_t*>(settings + 0xA4);
    if ((mode == 1 || mode != *reinterpret_cast<int32_t*>(settings + 0xA8)) && !ms_bWidescreen) {
        return 1;
    }
    return 0;
}

// 0x43A5C0: the number a text ends in (0 when there's none; the whole text must be digits
// from its first to its last, counting back). Wide texts, or narrow ones marked 0xFFFF
static int __cdecl ParseTrailingNumber(const uint16_t* s) {
    if (!s || *s == 0) {
        return 0;
    }
    int value = 0;
    int scale = 1;
    int n = 0;
    if (*s == 0xFFFF) {
        const int8_t* bytes = reinterpret_cast<const int8_t*>(s);
        do {
            n++;
        } while (bytes[n + 2] != 0);   // (from the second character: the game's)
        while (true) {
            int8_t c = bytes[n + 1];
            n--;
            if (c < '0' || c > '9') {
                return 0;
            }
            value += (c - '0') * scale;
            scale *= 10;
            if (n == 0) {
                return value;
            }
        }
    }
    do {
        n++;
    } while (s[n] != 0);
    while (true) {
        uint16_t c = s[n - 1];
        n--;
        if (c < '0' || c > '9') {
            return 0;
        }
        value += (c - '0') * scale;
        scale *= 10;
        if (n == 0) {
            return value;
        }
    }
}

// 0x43A680: the last read keeps failing (3000 tries: the disc error screen)
static uint32_t __cdecl IsReadFailing() {
    if (ms_lastChannelRead != -1 && *ChannelTries(ms_lastChannelRead) >= 3000) {
        return 1;
    }
    return 0;
}

// 0x43A6B0
static uint32_t __cdecl IsReturningToMain() {
    if (!ms_bReturnToMain_20C32ED && !ms_bReturnToMain_20C32EC) {
        return 0;
    }
    return 1;
}

// 0x43A7A0
static void __cdecl SetClipRange(int32_t a, int32_t b) {
    ms_clipRange0 = a;
    ms_clipRange1 = b;
}

// 0x43A7C0: the style of the line being added
static void __cdecl SetCreditLineStyle(uint32_t colour, float scale, int32_t font) {
    CreditLine& line = ms_creditLines[ms_numCreditLines];
    line.colour[1] = uint8_t(colour >> 8);
    line.colour[0] = uint8_t(colour);
    line.colour[3] = uint8_t(colour >> 24);
    line.colour[2] = uint8_t(colour >> 16);
    line.scale = scale;
    line.font = font;
}

// 0x43A810: a line (the text by key), 60 at most
static void __cdecl AddCreditLine(float x, float y, const char* key) {
    if (ms_numCreditLines >= 60) {
        return;
    }
    const uint16_t* text = Text_Get(TheText, key);
    CreditLine& line = ms_creditLines[ms_numCreditLines];
    line.text = text;
    line.x = x;
    line.y = y;
    ms_numCreditLines++;
}

// 0x43A860
static uint8_t __cdecl TextKeyExists(const char* key) {
    return key && Text_Get(TheText, key) != ms_missingText;
}

// 0x43A890
static uint8_t __cdecl GetTextNumber(const char* key, int* out) {
    if (!key) {
        return 0;
    }
    const uint16_t* text = Text_Get(TheText, key);
    if (text == ms_missingText) {
        return 0;
    }
    *out = Game_ParseTrailingNumber(text);
    return 1;
}

// 0x43A8D0 / 0x43ACD0
static void __cdecl SetInputIgnoredDisplay(int value) {
    ms_bInputIgnoredDisplay = uint8_t(value);
}

static void __cdecl SetLoadingScreenPaused(int value) {
    ms_bLoadingScreenPaused = uint8_t(value);
}

// 0x43ACE0: the loading screen's frame shown (its argument unused)
static void __cdecl EndLoadingScreenFrame(int) {
    Camera_Fn_6C86C0(ms_camera);
    Fn_4096C0();
    Fn_409750();
    Fn_4097D0();
    Game_ClearTidiedTextures(0);
}

// 0x43AD10: the renderer can draw
static uint32_t __cdecl CanRender() {
    if (!Fn_6C80A0() || !Fn_6F2E70() || !Fn_5EF860()) {
        return 0;
    }
    return Fn_5F2AF0() != 0;
}

// 0x43AEB0
static void __cdecl Fn_43AEB0() {
    Fn_4096C0();
    Fn_409520(7, 1);
}

// 0x43AED0 (two empty calls on 0xD140B8 left out)
static void __cdecl Fn_43AED0() {
    SetRenderState_519790(0xE, 1);
    Fn_5F3500();
    Fn_452C70();
    Fn_4E8EB0();
    Fn_452DE0();
    Fn_450C60();
    Fn_450C30();
}

// 0x43AF10
static void __cdecl Fn_43AF10() {
    Fn_513010();
    Fn_4DC0B0();
    Fn_5231F0();
}

// 0x43AF20
static void __cdecl Fn_43AF20(int flag) {
    Fn_72D520(reinterpret_cast<void*>(0x020C8A00), flag);
    Fn_52AC70(flag);
    Fn_5251C0();
    Fn_527800();
    if (!re::Flag(flag)) {
        Hud_Fn_53CF50(ms_pHud_C674B4);
    }
    Fn_513020();
    Fn_4E8CE0();
    Fn_452EC0();
    Fn_513780(reinterpret_cast<void*>(0x00C66B78));
    if (ms_pending_CA064C != 0 && !re::Flag(flag)) {
        Fn_528FE0();
        ms_pending_CA064C = 0;
    }
}

// 0x43B030
static void __cdecl Fn_43B030() {
    if (ms_bUserStart || ms_bFlag_20C32EA || ms_bFlag_20C32EB || ms_bFlag_20C32F2) {
        Fn_6A6A40();
    }
}

// 0x43B110: the text colour (r, g, b, a)
static void __cdecl SetTextColour(const uint8_t* colour) {
    ms_textColour[0] = colour[0];
    ms_textColour[1] = colour[1];
    ms_textColour[2] = colour[2];
    ms_textColour[3] = colour[3];
}

// 0x43B870: the end of a frame (an empty call on g_GameLogic left out)
static void __cdecl FrameEnd() {
    Fn_68D6C0();
    Fn_4257A0(reinterpret_cast<void*>(0x009E5158));
    Fn_4256B0();
    Fn_425B00();
    Fn_6C3A10();
    Fn_68CDF0();
    if (ms_bPlayerDown_20C87A8 && !ms_bPaused_C1A99A && !ms_bPaused_C1A999 && !ms_bPaused_C1A998) {
        Fn_707510();
    }
    Fn_43B310();
}

// 0x43B9D0: a texture dictionary's (a free slot's: read through nullptr, the game's)
static void* __cdecl GetTxdDictionary(int slot) {
    CPool<uint8_t>* pool = ms_pTxdPool;
    uint8_t* def = (pool->flags[slot] & 0x80) ? nullptr : pool->objects + pool->elemSize * slot;
    return *reinterpret_cast<re::Ptr32<void>*>(def);
}

} // namespace App

namespace CLoadingScreen {

uint32_t __cdecl IsActive() {
    return ms_loadingScreenTxd != -1;
}

// 0x43AD50: the splash's dictionary freed
static void __cdecl Shutdown() {
    ms_loadingScreenTexture = nullptr;
    if (ms_loadingScreenTxd != -1) {
        TxdStore_RemoveTxdSlot(ms_loadingScreenTxd);
    }
    ms_bLoadingScreenNew = 1;
    ms_loadingScreenTxd = -1;
}

} // namespace CLoadingScreen

// ---- registration ----------------------------------------------------------------------

#define APP_REVERSED(name, fn, address)                                                               \
    static void* Original_##fn;                                                                       \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace App {
APP_REVERSED("App::IsModeChangePending", IsModeChangePending, 0x0043A590);
APP_REVERSED("App::ParseTrailingNumber", ParseTrailingNumber, 0x0043A5C0);
APP_REVERSED("App::IsReadFailing", IsReadFailing, 0x0043A680);
APP_REVERSED("App::IsReturningToMain", IsReturningToMain, 0x0043A6B0);
APP_REVERSED("App::SetClipRange", SetClipRange, 0x0043A7A0);
APP_REVERSED("App::SetCreditLineStyle", SetCreditLineStyle, 0x0043A7C0);
APP_REVERSED("App::AddCreditLine", AddCreditLine, 0x0043A810);
APP_REVERSED("App::TextKeyExists", TextKeyExists, 0x0043A860);
APP_REVERSED("App::GetTextNumber", GetTextNumber, 0x0043A890);
APP_REVERSED("App::SetInputIgnoredDisplay", SetInputIgnoredDisplay, 0x0043A8D0);
APP_REVERSED("App::SetLoadingScreenPaused", SetLoadingScreenPaused, 0x0043ACD0);
APP_REVERSED("App::EndLoadingScreenFrame", EndLoadingScreenFrame, 0x0043ACE0);
APP_REVERSED("App::CanRender", CanRender, 0x0043AD10);
APP_REVERSED("App::Fn_43AEB0", Fn_43AEB0, 0x0043AEB0);
APP_REVERSED("App::Fn_43AED0", Fn_43AED0, 0x0043AED0);
APP_REVERSED("App::Fn_43AF10", Fn_43AF10, 0x0043AF10);
APP_REVERSED("App::Fn_43AF20", Fn_43AF20, 0x0043AF20);
APP_REVERSED("App::Fn_43B030", Fn_43B030, 0x0043B030);
APP_REVERSED("App::SetTextColour", SetTextColour, 0x0043B110);
APP_REVERSED("App::FrameEnd", FrameEnd, 0x0043B870);
APP_REVERSED("App::GetTxdDictionary", GetTxdDictionary, 0x0043B9D0);
} // namespace App

namespace CLoadingScreen {
APP_REVERSED("CLoadingScreen::IsActive", IsActive, 0x0043AD40);
APP_REVERSED("CLoadingScreen::Shutdown", Shutdown, 0x0043AD50);
} // namespace CLoadingScreen
