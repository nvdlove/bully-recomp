// The frontend's drawing and text helpers (boxes, windows, highlights, styled text, the
// controls line, the save-slot line, the exit/options popup), its data files' readers
// (text styles, positions, sprites), its reset/input/update/draw each frame, the title
// screen, the pause menu's items, and the screens' factory. 0x6A4340..0x6A71C1.
//
// The frontend's colours: 12 at 0x20C331C (0 drop shadows, 2 menu items, 3 highlighted /
// text, 8 messages, 11 selected items). A text style (0x1C): +0 justification, +4 font
// preset, +8 dx, +0xC dy, +0x10, +0x14 colour, +0x18.. flags (+0x1A drop shadow).
// A position (0xC) / sprite (0x18): +0 anchor, +4 x, +8 y (+0xC w, +0x10 h, +0x14 how
// it keeps its aspect: 0 width, 1 height, 2 none).
// The popup (FENameList, 0x5C): +0 its title, +0x10 [3] its choices (16 chars each),
// +0x40 their count, +0x44 the selected one, +0x48 the box, +0x4C its border, +0x50 the
// title, +0x54 selected and +0x58 other choices' colours.

#include <cmath>
#include <cstring>

#include "game/FEScreensCommon.h"

#include "game/Crt.h"
#include "reversed.h"
namespace fedraw {
using namespace fescr;

namespace {

GAME_ARRAY(uint8_t, g_feColours, 0x020C331C);  // [12] RGBA
GAME_VAR(float, g_screenScaleY, 0x00A957AC);
GAME_VAR(float, g_hudScale, 0x00A957B0);
GAME_VAR(int32_t, g_arcadeFont, 0x0208B218);  // (set: the font index forced to 0)
GAME_VAR(int16_t, g_fontIndex, 0x0208B788);
GAME_VAR(float, g_fontScaleY, 0x0208B770);
GAME_VAR(float, g_fontWrapX, 0x0208B774);
GAME_VAR(float, g_fontCentreWidth, 0x0208B778);
GAME_VAR(uint8_t, g_fontFlags, 0x0208B78A);
GAME_ARRAY(float, g_fontHeights, 0x00BA4E8C);
GAME_VAR(float, g_exitHiliteDx, 0x020C3584);  // (-7 from the first Japanese draw on)
GAME_FN(void, __cdecl, FD_FontAddString, 0x006880F0, const uint16_t* text);
GAME_FN(void, __cdecl, FD_FontSetScale, 0x006880B0, float x, float y);
GAME_FN(void, __cdecl, FD_FontGetExtent, 0x00688EA0, float* rect, uintptr_t);
GAME_FN(float, __cdecl, FD_FontLineHeight, 0x00688220);
GAME_FN(float, __cdecl, FD_FontStringWidth, 0x0068A920, const uint16_t* text, int withSpaces);
GAME_FN(void, __cdecl, FD_FontSetColour, 0x00424480, const uint8_t* colour);
GAME_FN(void*, __cdecl, FD_GetStringRect, 0x0068B090, float* rect, float x, float y, const uint16_t* text);
GAME_FN(void, __cdecl, FD_DrawRect, 0x0052C470, const float* rect, const uint8_t* colour);
GAME_FN(uint8_t*, __thiscall, FD_RGBA, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, FD_GetRenderState, 0x00519290, int state, int32_t* value);
GAME_FN(void, __cdecl, FD_SetRenderState, 0x00519790, int state, int value);
GAME_FN(int, __cdecl, FD_FindName, 0x0073AC90, const re::Ptr32<const char>* names, int count, const char* name, int fallback,
        int flag);
template <typename... A>
inline int __cdecl FD_Sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
inline int __cdecl FD_Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
GAME_FN(const char*, __cdecl, FD_AreaName, 0x004305B0, int area);
GAME_FN(uint8_t*, __thiscall, FD_MissionInfo, 0x006AA660, void* mgr, int id);
GAME_FN(double, __cdecl, FD_Version, 0x004120C0);

// the frontend's own
GAME_FN(void, __cdecl, FD_SetTextStyle, 0x006A3FC0, int colour, int justify, int font, int dropShadow);
GAME_FN(void, __cdecl, FD_DrawBox, 0x006A4420, float x1, float y1, float x2, float y2, float border,
        const uint8_t* colour, const uint8_t* borderColour);
GAME_FN(void, __cdecl, FD_DrawHighlight, 0x006A4790, float left, float right, float y);
GAME_FN(void, __cdecl, FD_DrawText, 0x006A5450, int x, int y, const uint16_t* text, int colour, int justify,
        int font, int drop);
GAME_FN(void, __cdecl, FD_MeasureText, 0x006A54D0, float* rect, int x, int y, const uint16_t* text, int colour,
        int justify, int font, int drop);
GAME_FN(void, __cdecl, FD_DrawTextKey, 0x006A6380, int x, int y, const char* key, int colour, int justify,
        int font, int drop);
GAME_FN(void, __cdecl, FD_DrawTextAt, 0x006A64A0, void* position, const uint16_t* text, int32_t* style);
GAME_FN(void, __cdecl, FD_DrawControls, 0x006A6510, const char* key);
GAME_FN(void, __cdecl, FD_DrawTitleScreen, 0x006A6810);
GAME_FN(void, __thiscall, FD_NameListInit, 0x006A4AC0, void* list);
GAME_FN(int, __cdecl, FD_AnchorFromName, 0x006A2630, const char* name);
GAME_FN(float, __thiscall, FD_PosX, 0x006A2730, void* position);
GAME_FN(float, __thiscall, FD_PosY, 0x006A2790, void* position);
GAME_FN(int, __cdecl, FD_JustifyFromName, 0x006A7110, const char* name);
GAME_FN(int, __cdecl, FD_FontFromName, 0x006A70F0, const char* name);
GAME_FN(int, __cdecl, FD_ColourFromName, 0x006A70D0, const char* name);
GAME_FN(int, __cdecl, FD_KindFromName, 0x006A70B0, const char* name);
GAME_FN(void, __cdecl, FD_UnloadTextures, 0x006A2D50);
GAME_FN(void, __cdecl, FD_LoadLayouts, 0x006A23B0);
GAME_FN(void, __cdecl, FD_ResetFrontEnd, 0x006A4E20);
GAME_FN(void*, __cdecl, FD_CreateScreen, 0x006A6DD0, int index);
GAME_FN(uint8_t, __cdecl, FD_ReadInput, 0x006A5190);
GAME_FN(void, __cdecl, FD_UpdateMenus, 0x006A3610);
GAME_FN(uint8_t, __cdecl, FD_PadRepeatReady, 0x006A3170, int pad);
GAME_FN(void, __cdecl, FD_ClearPad, 0x006A3120, int pad);
GAME_FN(uint8_t, __cdecl, FD_SetScreen, 0x006A25B0, int index);
GAME_FN(void, __cdecl, FD_DrawBackdrop, 0x006A2E00);
GAME_FN(void, __cdecl, FD_SetRenderStates, 0x006A2450);
GAME_FN(uint32_t, __cdecl, FD_CurrentScreenTitle, 0x006A2320);
GAME_FN(void, __cdecl, FD_DrawMessage, 0x006A2FF0, uint32_t hash);
GAME_FN(void, __cdecl, FD_PlayMenuSound, 0x006A31B0);
GAME_FN(uint8_t, __cdecl, FD_Pad7387E0, 0x007387E0);
GAME_FN(uint8_t, __cdecl, FD_Pad738800, 0x00738800);
GAME_FN(uint8_t, __cdecl, FD_Pad738820, 0x00738820);
GAME_FN(uint8_t, __cdecl, FD_PadJustDown, 0x00738900, int button, int pad);
GAME_FN(void, __cdecl, FD_PadsUpdate, 0x00738610);
GAME_FN(uint8_t, __thiscall, FD_InputLeft, 0x0050DFE0, void* input, int pad);
GAME_FN(uint8_t, __thiscall, FD_InputRight, 0x0050E040, void* input, int pad);
GAME_FN(uint8_t, __thiscall, FD_InputUp, 0x0050E0A0, void* input, int pad);
GAME_FN(uint8_t, __thiscall, FD_InputDown, 0x0050E110, void* input, int pad);
GAME_FN(int, __thiscall, FD_ScrollListSelected, 0x00853140, void* list);
GAME_FN(void, __thiscall, FD_SelListGetPosition, 0x0069B9B0, void* list, float* out);
GAME_FN(uint32_t, __thiscall, FD_SelListEntry, 0x0069BAB0, void* list, int index);

// the menus' state (see FrontEndMgr.cpp)
GAME_ARRAY(void*, g_screens, 0x020C3288);
GAME_VAR(int32_t, g_screen, 0x00BA6890);
GAME_VAR(int32_t, g_menuKind, 0x00BA6894);
GAME_VAR(uint8_t, g_pauseMenu, 0x020C32E9);
GAME_VAR(uint8_t, g_otherMenu, 0x020C32EA);
GAME_VAR(uint8_t, g_inTitle, 0x020C32EB);
GAME_VAR(uint8_t, g_screensMade, 0x020C32E8);
GAME_VAR(uint8_t, g_menu20C32F7, 0x020C32F7);
GAME_VAR(uint8_t, g_menu20C32FA, 0x020C32FA);
GAME_VAR(uint32_t, g_menuTime20C3308, 0x020C3308);
GAME_VAR(uint32_t, g_titleTimeout, 0x020C32FC);
GAME_VAR(uint32_t, g_frontendTime, 0x00A14A50);
GAME_ARRAY(uint8_t, g_left, 0x020C3280);   // [pad]
GAME_ARRAY(uint8_t, g_right, 0x020C327C);  // [pad]
GAME_ARRAY(uint8_t, g_up, 0x020C3278);     // [pad]
GAME_ARRAY(uint8_t, g_down, 0x020C3274);   // [pad]
GAME_ARRAY(uint8_t, g_soundsOn, 0x020C3270);
GAME_ARRAY(uint8_t, g_repeatSet, 0x020C31B0);
GAME_ARRAY(uint8_t, g_soundPending, 0x020C31B4);
GAME_ARRAY(uint8_t, g_prevDown, 0x020C31B8);
GAME_ARRAY(uint8_t, g_prevUp, 0x020C31BC);
GAME_ARRAY(uint8_t, g_prevRight, 0x020C31C0);
GAME_ARRAY(uint8_t, g_prevLeft, 0x020C31C4);
GAME_ARRAY(uint8_t, g_repeatFast, 0x020C31C8);
GAME_ARRAY(uint32_t, g_repeatTime, 0x020C31CC);
GAME_VAR(int32_t, g_sound, 0x020C3300);
GAME_VAR(uint8_t, g_frontendOn, 0x00BA688D);
GAME_VAR(uint8_t, g_storyStarted, 0x020C32F1);
GAME_VAR(uint8_t, g_texturesLoaded, 0x020C32F0);
GAME_VAR(void*, g_logo, 0x020C321C);
GAME_VAR(void*, g_period, 0x020C3208);
GAME_ARRAY(int32_t, g_titleScreenIds, 0x00BA6920);
GAME_ARRAY(int32_t, g_otherScreenIds, 0x00BA692C);
GAME_ARRAY(uint8_t, g_startMenuItems, 0x00BA68A8);
GAME_VAR(int8_t, g_titleTabCount, 0x020C3560);
GAME_VAR(float, g_crestGap, 0x020C3564);
GAME_ARRAY(char, g_attractMovie, 0x020C3220);
GAME_VAR(int32_t, g_region, 0x00C18728);
// the pointers (the mouse's, 16 bytes each: x, y, active, .., colour)
GAME_VAR(float, g_pointerX, 0x020C334C);
GAME_VAR(float, g_pointerY, 0x020C3350);
void* const kStartList = reinterpret_cast<void*>(0x020C33F8);
void* const kTitleTabs = reinterpret_cast<void*>(0x020C3508);
void* const kOtherTabs = reinterpret_cast<void*>(0x020C3488);
void* const kPopup = reinterpret_cast<void*>(0x020C3370);

// the name tables
const re::Ptr32<const char>* const kScreenNames = reinterpret_cast<const re::Ptr32<const char>*>(0x00941510);  // [24]
const re::Ptr32<const char>* const kColourNames = reinterpret_cast<const re::Ptr32<const char>*>(0x0094158C);  // [12]
const re::Ptr32<const char>* const kFloatNames = reinterpret_cast<const re::Ptr32<const char>*>(0x009415C0);   // [9]
const re::Ptr32<const char>* const kKindNames = reinterpret_cast<const re::Ptr32<const char>*>(0x00941574);    // [5]
const re::Ptr32<const char>* const kJustifyNames = reinterpret_cast<const re::Ptr32<const char>*>(0x009415E8); // [4]
const re::Ptr32<const char>* const kFlagNames = reinterpret_cast<const re::Ptr32<const char>*>(0x009415FC);    // [2]

inline int Ftol(float value) {
    return static_cast<int>(value);
}

uint8_t* Colour(int index) {
    return g_feColours + index * 4;
}

void SetFontColour(const uint8_t* colour) {
    g_fontColour[0] = colour[0];
    g_fontColour[1] = colour[1];
    g_fontColour[2] = colour[2];
    g_fontColour[3] = colour[3];
}

void SetDropColour(const uint8_t* colour) {
    g_fontDropColour[0] = colour[0];
    g_fontDropColour[1] = colour[1];
    g_fontDropColour[2] = colour[2];
    g_fontDropColour[3] = colour[3];
}

// a menu item's style (inlined in several draws): centred, font preset 3, no shadow
void ItemStyle(const uint8_t* colour) {
    Font_SetJustify(2);
    SetFontColour(colour);
    if (g_arcadeFont != 0) {
        g_fontIndex = 0;
    }
    Font_SetSizePreset(3);
    SetDropColour(Colour(0));
    g_fontDrop = 0;
}

float LineHeight() {
    return g_fontHeights[g_fontIndex] * g_fontScaleY;
}

// the frame's text reads a keyed string into a StringBuffer (archive vf9)
struct StringBuffer {
    re::Ptr32<char> text;
    int32_t size;
    char buffer[0x81];
};

void ReadString(void* archive, StringBuffer& out, const char* name) {
    out.text = out.buffer;
    out.size = 0x81;
    out.buffer[0] = 0;
    VCall<void>(archive, 0x24, &out, 0x81, name);
}

// a screen's virtual
void ScreenCall(void* screen, int offset) {
    VCall<void>(screen, offset);
}

} // namespace

// 0x6A3FC0: the font set up from a style's parts (justification 1..3, else 0)
void __cdecl SetTextStyle(int colour, int justify, int font, int dropShadow) {
    int mode;
    switch (justify) {
    case 1: mode = 1; break;
    case 2: mode = 2; break;
    case 3: mode = 3; break;
    default: mode = 0; break;
    }
    Font_SetJustify(mode);
    SetFontColour(Colour(colour));
    if (g_arcadeFont != 0) {
        g_fontIndex = 0;
    }
    Font_SetSizePreset(font);
    SetDropColour(Colour(0));
    g_fontDrop = re::Flag(dropShadow) ? 1 : 0;
}

// 0x6A4340: a warning (by key) centred at 320, 320, 400 wide
void __cdecl DrawWarning(const char* key) {
    const uint16_t* text = Text_Get(kTheText, key);
    Font_Reset();
    SetFontColour(Colour(3));
    Font_SetJustify(2);
    g_fontCentreWidth = 400.0f;
    Font_SetSizePreset(1);
    SetDropColour(Colour(0));
    g_fontFlags |= 0x10;
    g_fontDrop = 1;
    Font_PrintString(320.0f, 320.0f, text, 1);
    FD_FontGetExtent(g_lastTextRect, 0);
    Font_Render();
    g_fontFlags &= 0xEF;
}

// 0x6A4420: a box, and its border (when wider than 0) around it
void __cdecl DrawBox(float x1, float y1, float x2, float y2, float border, const uint8_t* colour,
                     const uint8_t* borderColour) {
    float uv1[2] = {0.0f, 0.0f};
    float uv2[2] = {1.0f, 1.0f};
    DrawRectUV(x1, y1, x2, y2, nullptr, colour, uv1, uv2);
    if (!(border > 0.0f)) {
        return;
    }
    float right = x2 + border;
    float left = x1 - border;
    DrawRectUV(left, y1 - border, right, y1, nullptr, borderColour, uv1, uv2);
    DrawRectUV(left, y2, right, y2 + border, nullptr, borderColour, uv1, uv2);
    DrawRectUV(left, y1, x1, y2, nullptr, borderColour, uv1, uv2);
    DrawRectUV(x2, y1, right, y2, nullptr, borderColour, uv1, uv2);
}

// 0x6A45F0: a colour between two (truncated per channel)
uint8_t* __cdecl LerpColour(uint8_t* out, const uint8_t* from, const uint8_t* to, float t) {
    std::memcpy(out, from, 4);
    for (int i = 0; i < 4; ++i) {
        int delta = int(to[i]) - int(from[i]);
        out[i] = uint8_t(Ftol(float(delta) * t + float(out[i])));
    }
    return out;
}

// 0x6A4730: a window (a rect, `width` wide centred on `centreX`, from `y` down `height`)
void __cdecl DrawWindow(float centreX, float width, float y, const uint8_t* colour, float height) {
    float half = width * 0.5f;
    float rect[4];
    rect[0] = centreX - half;
    rect[3] = y + height;
    rect[2] = centreX + half;
    rect[1] = y;
    FD_DrawRect(rect, colour);
}

// 0x6A4790: the dotted line under an item (the period texture tiled every 10 units)
void __cdecl DrawHighlight(float left, float right, float y) {
    int32_t saved;
    FD_GetRenderState(2, &saved);
    FD_SetRenderState(2, 1);
    float unit = g_screenScaleX * 10.0f;
    left = unit * 0.5f + left;
    float x1 = float(Ftol(left / unit)) * unit;
    float end = right - 5.0f;
    float x2 = float(Ftol(end / unit)) * unit + unit;
    float uv1[2] = {0.0f, 0.0f};
    float uv2[2] = {(x2 - x1) / unit, 1.0f};
    float top = y + 11.0f;
    DrawRectUV(x1, top, x2, top + 4.0f, g_period, Colour(3), uv1, uv2);
    FD_SetRenderState(2, saved);
}

// 0x6A48A0: a text style read ("justify font dx dy .. colour"; unparsed names stay
// whatever was on the stack)
void __fastcall TextStyle_Load(int32_t* self, re::Edx, void* archive, const char* name) {
    StringBuffer line;
    ReadString(archive, line, name);
    char justify[0x18];
    char font[0x18];
    char colour[0x18];
    FD_Sscanf(line.text, "%s %s %f %f %f %s", justify, font, self + 2, self + 3, self + 4, colour);
    self[0] = FD_JustifyFromName(justify);
    self[1] = FD_FontFromName(font);
    self[5] = FD_ColourFromName(colour);
}

// 0x6A4940: a position read ("anchor x y")
void __fastcall Position_Load(int32_t* self, re::Edx, void* archive, const char* name) {
    StringBuffer line;
    ReadString(archive, line, name);
    char anchor[0x18];
    FD_Sscanf(line.text, "%s %f %f", anchor, self + 1, self + 2);
    self[0] = FD_AnchorFromName(anchor);
}

// 0x6A49C0: a sprite read ("anchor x y w h HEIGHT|WIDTH|NONE")
void __fastcall Sprite_Load(int32_t* self, re::Edx, void* archive, const char* name) {
    StringBuffer line;
    ReadString(archive, line, name);
    char anchor[0x18];
    char keep[0x18];
    FD_Sscanf(line.text, "%s %f %f %f %f %s", anchor, self + 1, self + 2, self + 3, self + 4, keep);
    self[0] = FD_AnchorFromName(anchor);
    if (FD_Stricmp(keep, "HEIGHT") == 0) {
        self[5] = 1;
    } else if (FD_Stricmp(keep, "WIDTH") == 0) {
        self[5] = 0;
    } else if (FD_Stricmp(keep, "NONE") == 0) {
        self[5] = 2;
    } else {
        self[5] = 1;
    }
}

// 0x6A4AC0: the popup emptied, its colours set
void __fastcall NameList_Init(uint8_t* self) {
    At<int32_t>(self, 0x40) = 0;
    At<int32_t>(self, 0x44) = 0;
    uint8_t colour[4];
    std::memcpy(self + 0x48, FD_RGBA(colour, 0xF, 0xF, 0xF, 0xF0), 4);
    std::memcpy(self + 0x4C, FD_RGBA(colour, 0xF9, 0xCF, 0x5E, 0xFF), 4);
    std::memcpy(self + 0x50, FD_RGBA(colour, 0xA2, 0x7F, 0x16, 0xFF), 4);
    std::memcpy(self + 0x54, FD_RGBA(colour, 0xF9, 0xCF, 0x5E, 0xFF), 4);
    std::memcpy(self + 0x58, FD_RGBA(colour, 0x4B, 0x4B, 0x4B, 0xFF), 4);
}

// 0x6A4BD0: the popup made with a title
void __fastcall NameList_Construct(uint8_t* self, re::Edx, const char* title) {
    FD_NameListInit(self);
    Strncpy(reinterpret_cast<char*>(self), title, 0xF);
}

// 0x6A4BF0 (CStartMenuItem vf0): GetExtent(rect, x, width): its screen's title measured
// centred (the rect as the font leaves it)
void __fastcall StartMenuItem_GetExtent(uint8_t* self, re::Edx, float* rect, float x, float width) {
    ItemStyle(Colour(2));
    void* screen = g_screens[At<int32_t>(self, 8)];
    if (screen != nullptr) {
        const uint16_t* text = Text_GetByHash(kTheText, At<uint32_t>(screen, 4));
        FD_GetStringRect(rect, width * 0.5f + x, 0.0f, text);
        rect[3] = rect[3] + 0.0f;
    }
}

// 0x6A4CE0 (vf1): Draw(x, y, width, _, selected)
void __fastcall StartMenuItem_Draw(uint8_t* self, re::Edx, float x, float y, float width, int, int selected) {
    int colour = re::Flag(selected) ? 3 : 0xB;
    ItemStyle(Colour(colour));
    void* screen = g_screens[At<int32_t>(self, 8)];
    if (screen == nullptr) {
        return;
    }
    uint32_t hash = At<uint32_t>(screen, 4);
    if (re::Flag(selected)) {
        uint8_t unused[4];
        FD_RGBA(unused, 0, 0, 0, 0xA0);
        SetDropColour(Colour(0));
        g_fontDrop = 1;
    }
    Font_PrintString(width * 0.5f + x, y, Text_GetByHash(kTheText, hash), 1);
    Font_Render();
}

// 0x6A4E20: the frontend reset (menus down, input and sounds cleared, the pointers, the
// attract movie's name by region)
void __cdecl ResetFrontEnd() {
    if (g_pauseMenu != 0) {
        FD_UnloadTextures();
    }
    g_screensMade = 1;
    g_pauseMenu = 0;
    g_otherMenu = 0;
    g_inTitle = 0;
    g_menuActive = 0;
    g_texturesLoaded = 0;
    g_menuKind = -1;
    std::memset(g_repeatSet, 1, 4);
    std::memset(g_soundPending, 0, 4);
    std::memset(g_soundsOn, 0, 4);
    std::memset(g_prevDown, 0, 4);
    std::memset(g_prevUp, 0, 4);
    std::memset(g_prevRight, 0, 4);
    std::memset(g_prevLeft, 0, 4);
    std::memset(g_down, 0, 4);
    std::memset(g_up, 0, 4);
    std::memset(g_right, 0, 4);
    std::memset(g_left, 0, 4);
    g_repeatTime[0] = 0;
    g_repeatTime[1] = 0;
    g_repeatTime[2] = 0;
    g_repeatTime[3] = 0;
    g_sound = 0;
    FD_NameListInit(kPopup);
    uint8_t* pointers = reinterpret_cast<uint8_t*>(0x020C334C);
    std::memcpy(pointers + 0xA, reinterpret_cast<uint8_t*>(0x020C33E8), 4);
    g_frontendOn = 1;
    g_menu20C32F7 = 0;
    g_menuTime20C3308 = 0;
    g_menu20C32FA = 0;
    g_storyStarted = 0;
    At<uint8_t>(pointers, 8) = 1;
    float width = g_widescreen != 0 ? 853.0f : 640.0f;
    At<float>(pointers, 0) = width;
    At<float>(pointers, 4) = 480.0f;
    std::memcpy(pointers + 0x1A, reinterpret_cast<uint8_t*>(0x020C33EC), 4);
    At<uint8_t>(pointers, 9) = 0;
    At<uint8_t>(pointers, 0x18) = 0;
    At<float>(pointers, 0x10) = width;
    At<float>(pointers, 0x14) = 480.0f;
    At<uint8_t>(pointers, 0x19) = 0;
    const char* movie;
    switch (g_region) {
    case 1: movie = "Movies\\AttractModeF.wmv"; break;
    case 2: movie = "Movies\\AttractModeG.wmv"; break;
    case 3: movie = "Movies\\AttractModeI.wmv"; break;
    case 4: movie = "Movies\\AttractModeS.wmv"; break;
    case 6: movie = "Movies\\AttractModeR.wmv"; break;
    case 7: movie = "Movies\\AttractModeJ.wmv"; break;
    default: movie = "Movies\\AttractMode.wmv"; break;
    }
    std::memcpy(g_attractMovie, movie, std::strlen(movie) + 1);
}

// 0x6A5190: the menus' input for each pad: the directions (none while a pad's missing),
// their repeat, and whether a sound may play (a new direction or button 4..15)
uint8_t __cdecl ReadInput() {
    bool present = !FD_Pad7387E0() && !FD_Pad738800() && FD_Pad738820() == 1;
    for (int pad = 0; pad < 4; ++pad) {
        if (present) {
            g_left[pad] = FD_InputLeft(kUserInput, pad);
            g_right[pad] = FD_InputRight(kUserInput, pad);
            g_up[pad] = FD_InputUp(kUserInput, pad);
            g_down[pad] = FD_InputDown(kUserInput, pad);
        } else {
            g_left[pad] = 0;
            g_right[pad] = 0;
            g_up[pad] = 0;
            g_down[pad] = 0;
        }
        uint8_t left = g_left[pad];
        g_repeatFast[pad] = 0;
        if (left == 0 && g_right[pad] == 0 && g_up[pad] == 0 && g_down[pad] == 0) {
            g_repeatTime[pad] = 0;
        }
        if (g_repeatSet[pad] != 0 && FD_PadRepeatReady(0)) {
            g_prevLeft[pad] = 0;
            g_prevRight[pad] = 0;
            g_prevUp[pad] = 0;
            g_prevDown[pad] = 0;
            g_repeatSet[pad] = 0;
            g_repeatFast[pad] = 1;
        }
        if (!present) {
            g_soundsOn[pad] = 0;
        } else if ((left != 0 && g_prevLeft[pad] == 0) || (g_right[pad] != 0 && g_prevRight[pad] == 0) ||
                   (g_up[pad] != 0 && g_prevUp[pad] == 0) || (g_down[pad] != 0 && g_prevDown[pad] == 0)) {
            g_soundsOn[pad] = 1;
        } else {
            static const int kButtons[] = {4, 5, 6, 7, 9, 8, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF};
            uint8_t any = 0;
            for (int button : kButtons) {
                if (FD_PadJustDown(button, pad)) {
                    any = 1;
                    break;
                }
            }
            g_soundsOn[pad] = any;
        }
        g_prevLeft[pad] = g_left[pad];
        g_prevRight[pad] = g_right[pad];
        g_prevUp[pad] = g_up[pad];
        g_prevDown[pad] = g_down[pad];
        g_soundPending[pad] = 0;
    }
    return 1;
}

// 0x6A5430: a text style applied (the line height returned, from the font)
float __cdecl ApplyTextStyle(const int32_t* style) {
    FD_SetTextStyle(style[5], style[0], style[1], 0);
    return FD_FontLineHeight();
}

// 0x6A5450: text drawn at (x, y) in a style
void __cdecl DrawText(int x, int y, const uint16_t* text, int colour, int justify, int font, int drop) {
    FD_SetTextStyle(colour, justify, font, drop);
    uint16_t line[0x200];
    Font_Format(line, text, 0x200);
    Font_PrintString(float(x), float(y), line, 1);
    FD_FontGetExtent(g_lastTextRect, 0);
    Font_Render();
}

// 0x6A54D0: and measured instead
void __cdecl MeasureText(float* rect, int x, int y, const uint16_t* text, int colour, int justify, int font,
                         int drop) {
    FD_SetTextStyle(colour, justify, font, drop);
    uint16_t line[0x200];
    Font_Format(line, text, 0x200);
    FD_GetStringRect(rect, float(x), float(y), line);
}

// 0x6A5540: a save slot's two lines: chapter (or "5+") and area / completion, day, time /
// mission and date, each pair with the dotted line between
void __cdecl DrawSaveSlot(uint8_t* info, float left, float right, float top, float bottom, int colour,
                          float scaleX, float scaleY, int drop) {
    uint16_t time[0x40];  // (the game's hold 16, formatted up to 63)
    uint16_t date[0x40];
    uint16_t line[0x40];
    Font_AddNumber(float(At<int32_t>(info, 0x2C)));
    Font_AddNumber(float(At<int32_t>(info, 0x30)));
    Font_AddNumber(float(At<int32_t>(info, 0x34)));
    Font_Format(time, Text_Get(kTheText, "TIME24"), 0x3F);
    Font_AddNumber(float(At<int32_t>(info, 0x20)));
    Font_AddNumber(float(At<int32_t>(info, 0x24)));
    Font_AddNumber(float(At<int32_t>(info, 0x28)));
    Font_Format(date, Text_Get(kTheText, "YYYYMMDD"), 0x3F);
    float leftRect[4] = {1000000.0f, -1000000.0f, -1000000.0f, 1000000.0f};
    float rightRect[4] = {1000000.0f, -1000000.0f, -1000000.0f, 1000000.0f};
    Font_Reset();
    SetFontColour(Colour(colour));
    if (re::Flag(drop)) {
        SetDropColour(Colour(0));
        g_fontDrop = 1;
    } else {
        g_fontDrop = 0;
    }
    FD_FontSetScale(scaleX, scaleY);
    Font_SetJustify(0);
    int chapter = At<int32_t>(info, 8) + 1;
    if (chapter < 6) {
        Font_AddNumber(float(chapter));
        FD_FontAddString(Text_Get(kTheText, FD_AreaName(At<int32_t>(info, 0xC))));
        Font_Format(line, Text_Get(kTheText, "FESL_TL"), 0x3F);
    } else {
        FD_FontAddString(Text_Get(kTheText, FD_AreaName(At<int32_t>(info, 0xC))));
        Font_Format(line, Text_Get(kTheText, "FESL_TL5"), 0x3F);
    }
    Font_PrintString(left, top, line, 1);
    FD_FontGetExtent(leftRect, 0);
    Font_SetJustify(1);
    Font_AddNumber(At<float>(info, 4));
    Font_AddNumber(float(At<int32_t>(info, 0x14)) / 24.0f);
    Font_AddNumber(float(At<int32_t>(info, 0x14) % 24));
    Font_AddNumber(float(At<int32_t>(info, 0x18)));
    Font_AddNumber(float(At<int32_t>(info, 0x1C)));
    Font_Format(line, Text_Get(kTheText, "FESL_TR"), 0x3F);
    Font_PrintString(right, top, line, 1);
    FD_FontGetExtent(rightRect, 0);
    auto highlight = [&](float y) {
        float rightWidth = std::fabs(rightRect[2] - rightRect[0]);
        float to = right - rightWidth;
        float leftWidth = std::fabs(leftRect[2] - leftRect[0]);
        float from = leftWidth + left;
        FD_DrawHighlight(from, to, y);
    };
    highlight(top);
    Font_SetJustify(0);
    uint8_t* mission = FD_MissionInfo(kMissionMgr, At<int32_t>(info, 0x10));
    FD_FontAddString(Text_Get(kTheText, At<const char*>(mission, 0x20)));
    Font_Format(line, Text_Get(kTheText, "FESL_BL"), 0x3F);
    Font_PrintString(left, bottom, line, 1);
    FD_FontGetExtent(leftRect, 0);
    Font_SetJustify(1);
    FD_FontAddString(date);
    FD_FontAddString(time);
    Font_Format(line, Text_Get(kTheText, "FESL_BR"), 0x3F);
    Font_PrintString(right, bottom, line, 1);
    FD_FontGetExtent(rightRect, 0);
    highlight(bottom);
}

// 0x6A5A10: the popup: a box sized to its title and choices, the title, the choices
// (the selected one with EXIT_HILITE's marker)
void __fastcall NameList_Draw(uint8_t* self) {
    g_fontCentreWidth = 520.0f;
    ItemStyle(Colour(3));
    SetFontColour(self + 0x48);
    int count = At<int32_t>(self, 0x40);
    for (int i = 0; i < count; ++i) {  // (looked up and not used)
        Text_Get(kTheText, reinterpret_cast<const char*>(self + 0x10 + i * 0x10));
    }
    const uint16_t* title = Text_Get(kTheText, reinterpret_cast<const char*>(self));
    float rect[4] = {1000000.0f, -1000000.0f, -1000000.0f, 1000000.0f};
    FD_GetStringRect(rect, 320.0f, 190.0f, title);
    float titleWidth = std::fabs(rect[2] - rect[0]);
    float width = titleWidth + 20.0f;
    float titleHeight = std::fabs(rect[3] - rect[1]);
    float height = (titleHeight + LineHeight() * float(At<int32_t>(self, 0x40))) + 20.0f;
    float left = 320.0f - width * 0.5f;
    float top = 190.0f - height * 0.5f;
    FD_DrawBox(left, top, left + width, top + height, 2.0f, self + 0x48, self + 0x4C);
    SetFontColour(self + 0x50);
    float y = top + 10.0f;
    Font_PrintString(320.0f, y, title, 1);
    Font_Render();
    y = std::fabs(rect[3] - rect[1]) + y;
    for (int i = 0; i < At<int32_t>(self, 0x40); ++i) {
        const char* name = reinterpret_cast<const char*>(self + 0x10 + i * 0x10);
        if (i == At<int32_t>(self, 0x44)) {
            ItemStyle(Colour(3));
            SetFontColour(self + 0x54);
            if (g_language == 7) {
                g_exitHiliteDx = -7.0f;
            }
            Font_PrintString(g_exitHiliteDx + 320.0f, y, Text_Get(kTheText, "EXIT_HILITE"), 1);
            Font_Render();
        } else {
            ItemStyle(Colour(3));
            SetFontColour(self + 0x58);
        }
        Font_PrintString(320.0f, y, Text_Get(kTheText, name), 1);
        Font_Render();
        y = LineHeight() + y;
    }
}

// 0x6A5EE0: the frontend started: its screens made (once), reset, their layouts read
void __cdecl InitFrontEnd() {
    if (g_screensMade == 0) {
        for (int i = 0; i < 0x18; ++i) {
            g_screens[i] = FD_CreateScreen(i);
        }
        g_screensMade = 1;
    }
    FD_ResetFrontEnd();
    FD_LoadLayouts();
}

// 0x6A5F20: each frame: the input read, the menus opened/closed, then the open menu's
// own input (the pause list, the map menu's tabs, the title menu's tabs with the mouse)
void __cdecl UpdateFrontEnd() {
    if (!FD_ReadInput()) {
        return;
    }
    void* radar = g_radar;
    FD_UpdateMenus();
    uint8_t other = g_otherMenu;
    int32_t enter;
    if (g_pauseMenu == 0 && other == 0 && g_inTitle == 0) {
        goto leave;
    }
    g_menu20C32FA = other;
    g_menuTime20C3308 = g_timeInMs;
    if (g_menuKind == 0) {
        if (g_menuActive != 0) {
            return;
        }
        if (g_up[0] != 0 && FD_PadRepeatReady(0)) {
            ScrollList_Previous(kStartList);
            FD_ClearPad(0);
        } else if (g_down[0] != 0 && FD_PadRepeatReady(0)) {
            ScrollList_Next(kStartList);
            FD_ClearPad(0);
        }
        if (!Input_AcceptDown(kUserInput, 0)) {
            goto leave;
        }
        g_menuActive = 1;
        enter = At<int32_t>(g_startMenuItems + FD_ScrollListSelected(kStartList) * 0xC, 8);
    } else if (g_menuKind == 1) {
        if (At<int32_t>(radar, 0xE788) == 2) {
            goto leave;
        }
        if (g_left[0] != 0 && FD_PadRepeatReady(0)) {
            SelList_Previous(kOtherTabs);
        } else if (g_right[0] != 0 && FD_PadRepeatReady(0)) {
            SelList_Next(kOtherTabs);
        } else {
            goto leave;
        }
        if (FD_SetScreen(g_otherScreenIds[SelList_Selected(kOtherTabs)])) {
            FD_ClearPad(0);
        }
        goto leave;
    } else if (g_menuKind == 3) {
        if (g_menuActive != 0) {
            return;
        }
        int count = g_titleTabCount;
        float position[2];
        FD_SelListGetPosition(kTitleTabs, position);
        float width = g_widescreen != 0 ? 853.0f : 640.0f;
        float x = (position[0] - width * 0.5f) * g_screenScaleX;
        float y = (position[1] - 240.0f) * g_screenScaleY;
        float rect[4] = {1000000.0f, -1000000.0f, -1000000.0f, 1000000.0f};
        for (int i = 0; i < count; ++i) {
            const uint16_t* text = Text_GetByHash(kTheText, FD_SelListEntry(kTitleTabs, i));
            FD_GetStringRect(rect, x, y, text);
            float scale = g_hudScale;
            float textWidth = FD_FontStringWidth(text, 0) * scale;
            // (the pointer over a tab selects it)
            if (rect[0] < g_pointerX && rect[0] + textWidth > g_pointerX && rect[1] < g_pointerY &&
                rect[3] > g_pointerY) {
                SelList_Select(kTitleTabs, i);
            }
            if (i % 2 == 0) {
                float gap = g_crestGap;
                float crest = FD_FontStringWidth(Text_Get(kTheText, "FE_BULLY_CREST_FE"), 0);
                x = (crest + x) + (gap + gap);
            }
            x = FD_FontStringWidth(text, 0) + x;
        }
        bool moved = false;
        if (g_left[0] != 0 && FD_PadRepeatReady(0)) {
            SelList_Previous(kTitleTabs);
            moved = true;
            for (int pad = 0; pad < 4; ++pad) {
                FD_ClearPad(pad);
            }
        } else if (g_right[0] != 0 && FD_PadRepeatReady(0)) {
            SelList_Next(kTitleTabs);
            moved = true;
            for (int pad = 0; pad < 4; ++pad) {
                FD_ClearPad(pad);
            }
        }
        if (g_soundsOn[0] != 0 || g_soundsOn[1] != 0 || g_screen != -1 || moved) {
            g_titleTimeout = g_frontendTime + 60000;
        }
        if (!Input_AcceptDown(kUserInput, 0)) {
            goto leave;
        }
        g_menuActive = 1;
        enter = g_titleScreenIds[SelList_Selected(kTitleTabs)];
    } else {
        goto leave;
    }
    g_screen = enter;
    ScreenCall(g_screens[enter], 0xC);
    g_menu20C32F7 = 1;
    FD_PadsUpdate();
    g_soundPending[0] = 1;
    g_sound = 0;
leave:
    if (g_menuActive == 0 && g_screen != -1) {
        ScreenCall(g_screens[g_screen], 0x10);
        g_screen = -1;
    }
}

// 0x6A6380: a text key drawn at (x, y) in a style
void __cdecl DrawTextKey(int x, int y, const char* key, int colour, int justify, int font, int drop) {
    uint16_t line[0x200];
    Font_Format(line, Text_Get(kTheText, key), 0x200);
    FD_DrawText(x, y, line, colour, justify, font, drop);
}

// 0x6A63F0: and measured
void __cdecl MeasureTextKey(float* rect, int x, int y, const char* key, int colour, int justify, int font,
                            int drop) {
    uint16_t line[0x200];
    Font_Format(line, Text_Get(kTheText, key), 0x200);
    FD_MeasureText(rect, x, y, line, colour, justify, font, drop);
}

// 0x6A6460: text at (x, y) plus a text style's offset, in its style
void __cdecl DrawTextStyled(int x, int y, const uint16_t* text, int32_t* style) {
    int ty = Ftol(At<float>(style, 0xC) + float(y));
    int tx = Ftol(At<float>(style, 8) + float(x));
    FD_DrawText(tx, ty, text, style[5], style[0], style[1], At<uint8_t>(style, 0x1A));
}

// 0x6A64A0: text at a position plus a style's offset
void __cdecl DrawTextAt(void* position, const uint16_t* text, int32_t* style) {
    int ty = Ftol(float(Ftol(FD_PosY(position))) + At<float>(style, 0xC));
    int tx = Ftol(float(Ftol(FD_PosX(position))) + At<float>(style, 8));
    FD_DrawText(tx, ty, text, style[5], style[0], style[1], At<uint8_t>(style, 0x1A));
}

// 0x6A6510: the controls line at the bottom (the style set even without text)
void __cdecl DrawControls(const char* key) {
    float margin = g_screenScaleY * 8.0f;
    float base = 450.0f - margin;
    base = base + g_screenScaleY * 3.0f;
    FD_SetTextStyle(0xB, 2, 1, 0);
    g_fontWrapX = 640.0f;
    if (key == nullptr) {
        return;
    }
    const uint16_t* text = Text_Get(kTheText, key);
    FD_FontStringWidth(text, 1);
    g_fontFlags |= 0x10;
    float lineHeight = LineHeight();
    int y = Ftol(float(Ftol(base - lineHeight)) + 0.0f);
    int x = Ftol(0.0f + 320.0f);
    FD_DrawText(x, y, text, 0xB, 2, 1, 0);
    g_fontFlags &= 0xEF;
}

// 0x6A65F0: text at a position, centred 400 wide (a colour made and not used)
void __cdecl DrawTextAtCentred(void* position, const uint16_t* text, int32_t* style) {
    uint8_t unused[4];
    FD_RGBA(unused, 0x40, 0x40, 0x40, 0x20);
    g_fontCentreWidth = 400.0f;
    FD_DrawTextAt(position, text, style);
    g_fontFlags &= 0xFE;
}

// 0x6A6630: the same for a text key
void __cdecl DrawKeyAtCentred(void* position, const char* key, int32_t* style) {
    const uint16_t* text = Text_Get(kTheText, key);
    uint8_t unused[4];
    FD_RGBA(unused, 0x40, 0x40, 0x40, 0x20);
    g_fontCentreWidth = 400.0f;
    FD_DrawTextAt(position, text, style);
    g_fontFlags &= 0xFE;
}

// 0x6A6680: an upgrade's line: a dark bar across the screen, a thin line under it, the
// text over (the style's colour set to 8, its drop shadow on)
void __cdecl DrawUpgradeText(float x, float y, const uint16_t* text, int32_t* style) {
    uint8_t bar[4];
    FD_RGBA(bar, 0x40, 0x40, 0x40, 0x20);
    uint8_t line[4];
    std::memcpy(line, Colour(11), 4);
    SetDropColour(Colour(0));
    g_fontDrop = 1;
    At<uint8_t>(style, 0x1A) = 1;
    style[5] = 8;
    FD_SetTextStyle(8, style[0], style[1], 1);
    float left = x - 6.0f;
    float width = 640.0f - (left + left);
    float height = FD_FontLineHeight() + 0.0f;
    float rect[4];
    rect[0] = left;
    float bottom = height + y;
    rect[3] = bottom;
    float right = left + width;
    rect[2] = right;
    rect[1] = y;
    FD_DrawRect(rect, bar);
    rect[0] = left;
    rect[3] = 2.0f + bottom;
    rect[2] = right;
    rect[1] = bottom;
    FD_DrawRect(rect, line);
    int ty = Ftol(At<float>(style, 0xC) + float(Ftol(y)));
    int tx = Ftol(At<float>(style, 8) + float(Ftol(x)));
    FD_DrawText(tx, ty, text, style[5], style[0], style[1], At<uint8_t>(style, 0x1A));
}

// 0x6A6810: the title screen: the logo, the version, the controls
void __cdecl DrawTitleScreen() {
    float scaleX = g_screenScaleX;
    void* logo = g_logo;
    float scaleY = g_screenScaleY;
    uint8_t white[4];
    FD_RGBA(white, 0xFF, 0xFF, 0xFF, 0xFF);
    uint32_t w = VCall<uint32_t>(logo, 0x48);
    float width = float(w);  // (unsigned)
    uint32_t h = VCall<uint32_t>(logo, 0x4C);
    float height = float(h);
    float position[2];
    ScreenPosition(position, 0.0f, -40.0f, 1, 1);
    float uv1[2] = {0.0f, 0.0f};
    float uv2[2] = {1.0f, 1.0f};
    float halfHeight = height * 0.5f * scaleY;
    float halfWidth = width * 0.5f * scaleX;
    DrawRectUV(position[0] - halfWidth, position[1] - halfHeight, position[0] + halfWidth,
               position[1] + halfHeight, logo, white, uv1, uv2);
    SetFontColour(Colour(8));
    Font_SetSizePreset(1);
    Font_SetJustify(2);
    float dx = -130.0f;
    g_fontDrop = 1;
    if (g_language == 7) {
        dx = -120.0f;
    }
    float version[2];
    ScreenPosition(version, dx * scaleX, scaleY * -80.0f, 2, 2);
    char ascii[0x40];
    SprintfS(ascii, 0x40, "%1.3f", FD_Version());
    uint16_t text[0x40];
    AsciiToWide(ascii, text);
    Font_PrintString(version[0], version[1], text, 1);
    FD_DrawControls("FE_TITLE_CONTROLS");
}

// 0x6A6A40: each frame's draw: the backdrop, the open menu (the map tabs, the title
// screen, "game paused" with the list, or the screen's title), the screen, the controls
void __cdecl DrawFrontEnd() {
    if (g_inTitle == 0 && g_pauseMenu == 0 && g_otherMenu == 0) {
        return;
    }
    int32_t kind = g_menuKind;
    if (kind == 0 || kind == 1 || kind == 4 || kind == 3) {
        FD_DrawBackdrop();
    }
    FD_SetRenderStates();
    if (At<int32_t>(g_radar, 0xE788) != 2) {
        if (g_menuKind == 1) {
            SelList_Draw(kOtherTabs);
        } else if (g_menuKind == 3) {
            FD_DrawTitleScreen();
            SetFontColour(Colour(8));
            SetDropColour(Colour(0));
            g_fontDrop = 1;
            SelList_Draw(kTitleTabs);
        } else if (g_menuActive == 0) {
            FD_FontSetColour(Colour(8));
            Font_SetSizePreset(4);
            Font_SetJustify(2);
            float y = g_screenScaleY * -190.0f;
            g_fontDrop = 1;
            float position[2];
            ScreenPosition(position, g_screenScaleX * 0.0f, y, 1, 1);
            uint16_t text[0x40];
            Font_Format(text, Text_Get(kTheText, "FE_GAME_PAUSED"), 0x40);
            Font_PrintString(position[0], position[1], text, 1);
            ScrollList_Draw(kStartList, 0.0f, 18.0f, 0);
        } else if (g_screen != 0x15) {
            FD_DrawMessage(FD_CurrentScreenTitle());
        }
    }
    const char* controls = nullptr;
    bool draw = true;
    if (g_menuActive != 0 && g_menu20C32F7 == 0) {
        ScreenCall(g_screens[g_screen], 0x14);
        if (g_menuActive != 0) {
            ScreenCall(g_screens[g_screen], 0x18);
            draw = false;
        } else if (g_screen == -1) {
            controls = nullptr;
        } else if (g_inTitle != 0) {
            draw = false;
        } else {
            controls = "FE_PAUSE_C";
        }
    } else if (g_inTitle != 0) {
        draw = false;
    } else {
        controls = "FE_PAUSE_C";
    }
    if (draw) {
        FD_DrawControls(controls);
    }
    g_menu20C32F7 = 0;
    FD_PlayMenuSound();
    if (!FD_Pad7387E0() && !FD_Pad738800()) {
        FD_Pad738820();
    }
}

// 0x6A6CC0: a text key at (x, y) plus a style's offset
void __cdecl DrawKeyStyled(int x, int y, const char* key, int32_t* style) {
    int ty = Ftol(At<float>(style, 0xC) + float(y));
    int tx = Ftol(At<float>(style, 8) + float(x));
    FD_DrawTextKey(tx, ty, key, style[5], style[0], style[1], At<uint8_t>(style, 0x1A));
}

// 0x6A6D00: a text key at a position plus a style's offset
void __cdecl DrawKeyAt(void* position, const char* key, int32_t* style) {
    int ty = Ftol(float(Ftol(FD_PosY(position))) + At<float>(style, 0xC));
    int tx = Ftol(float(Ftol(FD_PosX(position))) + At<float>(style, 8));
    FD_DrawTextKey(tx, ty, key, style[5], style[0], style[1], At<uint8_t>(style, 0x1A));
}

// 0x6A6D70 / 0x6A6D90 / 0x6A6DB0: a screen's / colour's / float's name ("INVALID" past
// the table: the game's own string at 0x903168, the same pointer)
template <uintptr_t Table, uint32_t Count>
const char* __cdecl NameOf(uint32_t index) {
    if (index < Count) {
        return reinterpret_cast<const re::Ptr32<const char>*>(Table)[index];
    }
    return reinterpret_cast<const char*>(0x00903168);
}

// 0x6A6DD0: a screen made by index
void* __cdecl CreateScreen(uint32_t index) {
    struct Kind {
        uint32_t size;
        uintptr_t constructor;
    };
    static const Kind kKinds[24] = {
        {0x8C, 0x00695490},  {0x12C, 0x00693F10}, {0x418, 0x0069C980}, {0x2A0, 0x006A0020}, {0x8C0, 0x00692730},
        {0x1E8, 0x00698220}, {0x17C, 0x006984E0}, {0x10C, 0x00696140}, {0x94, 0x00699C20},  {0xC, 0x00690BC0},
        {0x70, 0x006998C0},  {0x5A0, 0x0069EC70}, {0xC, 0x00695FE0},   {0x80C, 0x0069D4A0}, {0x430, 0x006A0720},
        {0x28, 0x0069E500},  {0x28, 0x00690BD0},  {0x28, 0x006939A0},  {0x28, 0x0069AAC0},  {0x70, 0x00693620},
        {0x2C, 0x006905F0},  {0x28, 0x006A0580},  {0xC, 0x006998A0},   {0x69C, 0x00692A50}};
    if (index > 0x17) {
        return nullptr;
    }
    void* screen = Malloc(kKinds[index].size);
    if (screen == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<void*(__thiscall*)(void*)>(kKinds[index].constructor)(screen);
}

// 0x6A70B0.. 0x6A7130: a name's index in one of the tables (-1: none)
template <uintptr_t Table, int Count>
int __cdecl IndexOf(const char* name) {
    return FD_FindName(re::PtrCast<const re::Ptr32<const char>*>(Table), Count, name, -1, 0);
}

// 0x6A7150: a screen's Load: its title's hash and its kind
void __fastcall Screen_Load(uint8_t* self, re::Edx, void* archive) {
    struct {
        re::Ptr32<char> text;
        int32_t size;
        char buffer[0x20];
    } line;
    line.text = line.buffer;
    line.size = 0x20;
    line.buffer[0] = 0;
    VCall<void>(archive, 0x24, &line, 0x20, *reinterpret_cast<const re::Ptr32<const char>*>(0x00940EA4));
    At<uint32_t>(self, 4) = HashName(line.text);
    VCall<void>(archive, 0x24, &line, 0x20, *reinterpret_cast<const re::Ptr32<const char>*>(0x00940EA8));
    At<int32_t>(self, 8) = FD_KindFromName(line.text);
}

} // namespace fedraw

#define FEDRAW_REVERSED(name, address, fn)                                                          \
    static void* RE_CONCAT(fedraw_original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(fedraw_original_, __LINE__), \
             re::Mode::Original)

FEDRAW_REVERSED("FrontEnd::SetTextStyle", 0x006A3FC0, &fedraw::SetTextStyle);
FEDRAW_REVERSED("FrontEnd::DrawWarning", 0x006A4340, &fedraw::DrawWarning);
FEDRAW_REVERSED("FrontEnd::DrawBox", 0x006A4420, &fedraw::DrawBox);
FEDRAW_REVERSED("FrontEnd::LerpColour", 0x006A45F0, &fedraw::LerpColour);
FEDRAW_REVERSED("FrontEnd::DrawWindow", 0x006A4730, &fedraw::DrawWindow);
FEDRAW_REVERSED("FrontEnd::DrawHighlight", 0x006A4790, &fedraw::DrawHighlight);
FEDRAW_REVERSED("FETextStyle::Load", 0x006A48A0, &fedraw::TextStyle_Load);
FEDRAW_REVERSED("FEPosition::Load", 0x006A4940, &fedraw::Position_Load);
FEDRAW_REVERSED("FESprite::Load", 0x006A49C0, &fedraw::Sprite_Load);
FEDRAW_REVERSED("FENameList::Init", 0x006A4AC0, &fedraw::NameList_Init);
FEDRAW_REVERSED("FENameList::FENameList", 0x006A4BD0, &fedraw::NameList_Construct);
FEDRAW_REVERSED("FRONTEND::CStartMenuItem::GetExtent", 0x006A4BF0, &fedraw::StartMenuItem_GetExtent);
FEDRAW_REVERSED("FRONTEND::CStartMenuItem::Draw", 0x006A4CE0, &fedraw::StartMenuItem_Draw);
FEDRAW_REVERSED("FrontEnd::Reset", 0x006A4E20, &fedraw::ResetFrontEnd);
FEDRAW_REVERSED("FrontEnd::ReadInput", 0x006A5190, &fedraw::ReadInput);
FEDRAW_REVERSED("FrontEnd::ApplyTextStyle", 0x006A5430, &fedraw::ApplyTextStyle);
FEDRAW_REVERSED("FrontEnd::DrawText", 0x006A5450, &fedraw::DrawText);
FEDRAW_REVERSED("FrontEnd::MeasureText", 0x006A54D0, &fedraw::MeasureText);
FEDRAW_REVERSED("FrontEnd::DrawSaveSlot", 0x006A5540, &fedraw::DrawSaveSlot);
FEDRAW_REVERSED("FENameList::Draw", 0x006A5A10, &fedraw::NameList_Draw);
FEDRAW_REVERSED("FrontEnd::Init", 0x006A5EE0, &fedraw::InitFrontEnd);
FEDRAW_REVERSED("FrontEnd::Update", 0x006A5F20, &fedraw::UpdateFrontEnd);  // (loops over 0x6A3120 keeping edx)
FEDRAW_REVERSED("FrontEnd::DrawTextKey", 0x006A6380, &fedraw::DrawTextKey);
FEDRAW_REVERSED("FrontEnd::MeasureTextKey", 0x006A63F0, &fedraw::MeasureTextKey);
FEDRAW_REVERSED("FrontEnd::DrawTextStyled", 0x006A6460, &fedraw::DrawTextStyled);
FEDRAW_REVERSED("FrontEnd::DrawTextAt", 0x006A64A0, &fedraw::DrawTextAt);
FEDRAW_REVERSED("FrontEnd::DrawControls", 0x006A6510, &fedraw::DrawControls);
FEDRAW_REVERSED("FrontEnd::DrawTextAtCentred", 0x006A65F0, &fedraw::DrawTextAtCentred);
FEDRAW_REVERSED("FrontEnd::DrawKeyAtCentred", 0x006A6630, &fedraw::DrawKeyAtCentred);
FEDRAW_REVERSED("FrontEnd::DrawUpgradeText", 0x006A6680, &fedraw::DrawUpgradeText);
FEDRAW_REVERSED("FrontEnd::DrawTitleScreen", 0x006A6810, &fedraw::DrawTitleScreen);
FEDRAW_REVERSED("FrontEnd::Draw", 0x006A6A40, &fedraw::DrawFrontEnd);
FEDRAW_REVERSED("FrontEnd::DrawKeyStyled", 0x006A6CC0, &fedraw::DrawKeyStyled);
FEDRAW_REVERSED("FrontEnd::DrawKeyAt", 0x006A6D00, &fedraw::DrawKeyAt);
FEDRAW_REVERSED("FrontEnd::ScreenName", 0x006A6D70, (&fedraw::NameOf<0x00941510, 24>));
FEDRAW_REVERSED("FrontEnd::ColourName", 0x006A6D90, (&fedraw::NameOf<0x0094158C, 12>));
FEDRAW_REVERSED("FrontEnd::FloatName", 0x006A6DB0, (&fedraw::NameOf<0x009415C0, 9>));
FEDRAW_REVERSED("FrontEnd::CreateScreen", 0x006A6DD0, &fedraw::CreateScreen);
FEDRAW_REVERSED("FrontEnd::KindFromName", 0x006A70B0, (&fedraw::IndexOf<0x00941574, 5>));
FEDRAW_REVERSED("FrontEnd::ColourFromName", 0x006A70D0, (&fedraw::IndexOf<0x0094158C, 12>));
FEDRAW_REVERSED("FrontEnd::FloatFromName", 0x006A70F0, (&fedraw::IndexOf<0x009415C0, 9>));
FEDRAW_REVERSED("FrontEnd::JustifyFromName", 0x006A7110, (&fedraw::IndexOf<0x009415E8, 4>));
FEDRAW_REVERSED("FrontEnd::FlagFromName", 0x006A7130, (&fedraw::IndexOf<0x009415FC, 2>));
FEDRAW_REVERSED("FrontEndScreen::Load", 0x006A7150, &fedraw::Screen_Load);
