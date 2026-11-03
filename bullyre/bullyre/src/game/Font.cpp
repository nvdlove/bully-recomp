// The game's font (0x687A30..0x6884F0; the text layout and rendering in FontLayout.cpp):
// its styles (up to 10: a texture per style from TXD/<name>.NFT, glyph metrics in the
// tables at 0x1B8A9C4 with a stride of 0x7FF8A per style), the current details (scale,
// justification, colour, style), the numbers and strings a formatted text pulls in, the
// "~" control tokens, the laid-out lines' bookkeeping, and the HUD's font setup record.
//
// CFont's details (0x208B764..): +0 the colour, +8 the scale (x at 0x208B76C, y at
// 0x208B770), 0x208B774.. the wrap bounds, 0x208B784 flags, 0x208B786 the justification
// (0 left, 1 right, 2 left again, 3 centred), 0x208B788 the style.

#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using vc::At;

GAME_ARRAY(const char*, fontStyleNames, 0x00BA4FA4);    // [10]
GAME_ARRAY(void*, fontStyleTexture, 0x0208B218);         // [10] the loaded texture
GAME_ARRAY(float, fontStyleInvSize, 0x0208B790);         // [10][2] 1 / the texture's width, height
GAME_ARRAY(float, fontStyleHeights, 0x0208B240);         // [10]
GAME_ARRAY(float, fontStyleLineHeight, 0x00BA4E8C);      // [10] (0: from the glyph table)
GAME_ARRAY(float, fontStyleLeading, 0x00BA4E3C);         // [10]
GAME_ARRAY(float, fontStyleLeadingJ, 0x00BA4E64);        // [10] (language 7)
GAME_ARRAY(float, fontStyleHeightJ, 0x00BA4EB4);         // [10] (language 7)
GAME_ARRAY(int32_t, fontStyleMapJ, 0x00BA4EDC);          // [10] (language 7)
GAME_ARRAY(float, fontStyleHeightR, 0x00BA4F2C);         // [10] (language 6)
GAME_ARRAY(int32_t, fontStyleMapR, 0x00BA4F54);          // [10] (language 6)
inline char* const fontTxdNames = reinterpret_cast<char*>(0x0208B178);   // [10][16]
inline uint8_t* const fontGlyphs = reinterpret_cast<uint8_t*>(0x01B8A9C4);
constexpr int kGlyphStyleStride = 0x7FF8A;
GAME_VAR(float, fontScaleX, 0x0208B76C);
GAME_VAR(float, fontScaleY, 0x0208B770);
GAME_VAR(int16_t, fontJustify, 0x0208B786);
GAME_VAR(int16_t, fontStyle, 0x0208B788);
GAME_ARRAY(uint8_t, fontColour, 0x0208B764);
GAME_VAR(float, screenScaleX, 0x00A957A8);
GAME_VAR(float, screenScaleY, 0x00A957AC);
GAME_VAR(int32_t, gameLanguage, 0x00C18724);
GAME_VAR(int32_t, gameLanguage2, 0x00C18728);
GAME_VAR(uint8_t, clockHours, 0x00BCEB4B);
GAME_VAR(uint8_t, clockMinutes, 0x00BCEB4A);
// the numbers and strings a text's ~1~ / ~a~ take, in order
GAME_ARRAY(float, fontNumbers, 0x01B89560);   // [5] (+1 based)
GAME_VAR(int32_t, fontNumberCount, 0x01B89578);
GAME_ARRAY(void*, fontStrings, 0x01B89550);   // [5]
GAME_VAR(int32_t, fontStringCount, 0x01B8957C);
// the button glyphs (textures by index, registered into every style's glyph table)
GAME_ARRAY(void*, fontButtons, 0x01B89244);
GAME_VAR(int32_t, fontButtonFirst, 0x0208B278);
GAME_VAR(int32_t, fontButtonLast, 0x0208B27C);
// the token names (21 bytes each: the name, +0x14 its glyph)
GAME_VAR(int32_t, fontTokenCount, 0x0208A524);
inline uint8_t* const fontTokens = reinterpret_cast<uint8_t*>(0x0208A528);
// the help message
inline char* const fontMessage = reinterpret_cast<char*>(0x01B8A988);   // [0x30]
GAME_VAR(float, fontMessageX, 0x0208B268);
GAME_VAR(float, fontMessageY, 0x0208B26C);
GAME_VAR(uint8_t, fontMessageShown, 0x01B8A9B8);
GAME_VAR(uint8_t, fontMessageFlag, 0x0208B265);
inline const char* const fontColourFormat = reinterpret_cast<const char*>(0x009189C0);   // "%d %d %d"
inline const char* const fontTxdFormat = reinterpret_cast<const char*>(0x0093EDC8);      // "%st"
inline const char* const fontNftFormat = reinterpret_cast<const char*>(0x00906A74);      // "TXD/%s.NFT"
inline const char* const fontMaskFormat = reinterpret_cast<const char*>(0x0093EDC4);     // "%sm"
inline const char* const fontAmFormat = reinterpret_cast<const char*>(0x0093EDD8);       // "%d:%02d am"
inline const char* const fontPmFormat = reinterpret_cast<const char*>(0x0093EDCC);       // "%d:%02d pm"
inline const char* const fontMoneyFormat = reinterpret_cast<const char*>(0x0093EDE4);    // "$%.2f"

template <typename... A>
inline int __cdecl Sprintf(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
template <typename... A>
inline int __cdecl Sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
inline char* __cdecl Strncpy(char* dst, const char* src, size_t n) { return crt::Strncpy(dst, src, n); }
inline char* __cdecl StrUpr(char* s) { return crt::Strupr(s); }   // _strupr
GAME_FN(int, __cdecl, Txd_AddTxdSlot, 0x005F26E0, const char* name);
GAME_FN(void, __cdecl, Txd_LoadTxdFile, 0x005F2A80, int slot, const char* path);
GAME_FN(void, __cdecl, Txd_AddRef, 0x005F1130, int slot);
GAME_FN(void, __cdecl, Txd_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, Txd_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void*, __cdecl, Txd_FindTexture, 0x005F1770, const char* name, const char* mask);
GAME_FN(void, __cdecl, Txd_PopCurrentTxd, 0x005F0AA0);
GAME_FN(uint8_t, __cdecl, NiRelease, 0x005F0F90, void* object);
GAME_FN(int, __cdecl, Txd_FindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(void, __cdecl, Txd_RemoveTxdSlot, 0x005F2240, int slot);
GAME_FN(int, __cdecl, HAlignFromName, 0x005DAF00, const char* name);
GAME_FN(int, __cdecl, VAlignFromName, 0x005DAF20, const char* name);
GAME_FN(int, __cdecl, JustifyFromName, 0x006A7110, const char* name);
GAME_FN(int, __cdecl, ScaleFromName, 0x006A70F0, const char* name);
GAME_FN(uint8_t*, __thiscall, RGBA_Construct, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
// ours, through the game's addresses
GAME_FN(uint16_t, __cdecl, Font_FindToken, 0x00687E40, const char* name);
GAME_FN(const uint16_t*, __cdecl, Font_ParseToken, 0x00687EB0, const uint16_t* text, uint16_t* out);
GAME_FN(const void*, __cdecl, Font_ParseColour, 0x00687F10, const uint16_t* text, uint8_t* out);
GAME_FN(float, __cdecl, Font_GetLineHeight, 0x00688220);

// 0x687A30 (cdecl): a text is the "none" marker (0xFFFF first)
int __cdecl Font_IsNone(const int16_t* text) {
    if (text && *text == -1) {
        return 1;
    }
    return 0;
}

// x clamped to [0.6, 0.8] of a screen scale
inline float ClampScreen(float scale, float x) {
    float low = scale * 0.6000000238418579f;
    float high = scale * 0.800000011920929f;
    if (low > x) {
        return low;
    }
    if (high < x) {
        return high;
    }
    return x;
}
// 0x687A50 / 0x687AB0 (cdecl): a width / a height kept within 60..80% of the screen's
float __cdecl Font_ClampWidth(float x) {
    return ClampScreen(screenScaleX, x);
}
float __cdecl Font_ClampHeight(float y) {
    return ClampScreen(screenScaleY, y);
}

// 0x687B10 (thiscall, ret 4): a text box's layout adjusted for its scale mode (+0x34) in
// widescreen modes 2 / 4 (and for language 1 at mode 2): its width (+0x28 / +0x2C) and
// height (+0x10)
void __fastcall FontBox_AdjustForScreen(uint8_t* self, re::Edx, int widescreen) {
    int32_t mode = At<int32_t>(self, 0x34);
    if (gameLanguage2 == 1 && mode == 2) {
        At<float>(self, 0x28) = At<float>(self, 0x28) + 15.0f;
        At<float>(self, 0x2C) = 15.0f + At<float>(self, 0x2C);
        return;
    }
    if (widescreen != 2 && widescreen != 4) {
        return;
    }
    if (mode == 2) {
        At<float>(self, 0x28) = At<float>(self, 0x28) + 20.0f;
        At<float>(self, 0x2C) = 20.0f + At<float>(self, 0x2C);
        At<float>(self, 0x10) = At<float>(self, 0x10) - 25.0f;
    } else if (mode == 4) {
        At<float>(self, 0x10) = At<float>(self, 0x10) - 40.0f;
    } else if (mode == 3) {
        At<float>(self, 0x28) = At<float>(self, 0x28) + 60.0f;
        At<float>(self, 0x2C) = 60.0f + At<float>(self, 0x2C);
        At<float>(self, 0x10) = At<float>(self, 0x10) - 10.0f;
    } else if (mode == 6) {
        At<float>(self, 0x10) = At<float>(self, 0x10) - 10.0f;
    }
}

// a glyph table entry's short at `offset` for a style
inline int16_t& Glyph(int style, int offset) {
    return *reinterpret_cast<int16_t*>(fontGlyphs + style * kGlyphStyleStride + offset);
}

// 0x687BC0 (cdecl): a font style loaded (its texture dictionary "<name>t" from
// TXD/<NAME>.NFT, the texture <name> with mask <name>m, 1 / its size; its line height from
// the glyph table when unset)
void __cdecl FontStyle_Load(int style) {
    if (fontStyleTexture[style] != nullptr) {
        return;
    }
    char* txdName = fontTxdNames + style * 16;
    Sprintf(txdName, fontTxdFormat, fontStyleNames[style]);
    char path[16];
    Sprintf(path, fontNftFormat, fontStyleNames[style]);
    char mask[16];
    Sprintf(mask, fontMaskFormat, fontStyleNames[style]);
    StrUpr(path);
    int slot = Txd_AddTxdSlot(txdName);
    Txd_LoadTxdFile(slot, path);
    Txd_AddRef(slot);
    Txd_PushCurrentTxd();
    Txd_SetCurrentTxd(slot);
    void* texture = Txd_FindTexture(fontStyleNames[style], mask);
    fontStyleTexture[style] = texture;
    uint32_t width = vc::Call<uint32_t>(texture, 0x48);
    fontStyleInvSize[style * 2] = 1.0f / float(width);
    uint32_t height = vc::Call<uint32_t>(fontStyleTexture[style], 0x4C);
    fontStyleInvSize[style * 2 + 1] = 1.0f / float(height);
    Txd_PopCurrentTxd();
    if (0.0f == fontStyleLineHeight[style]) {
        fontStyleLineHeight[style] = float(int32_t(Glyph(style, 0x672)) - int32_t(Glyph(style, 0x66E)));
    }
}

// 0x687D10 (cdecl): a font style unloaded
void __cdecl FontStyle_Unload(int style) {
    if (fontStyleTexture[style]) {
        NiRelease(fontStyleTexture[style]);
    }
    int slot = Txd_FindTxdSlot(fontTxdNames + style * 16, 0);
    if (slot != -1) {
        Txd_RemoveTxdSlot(slot);
    }
    fontStyleTexture[style] = nullptr;
}

// 0x687D60 (cdecl): a button glyph's texture set (its size into every style's table)
void __cdecl Font_SetButton(int index, void* texture) {
    if (!texture || index < 0 || index > fontButtonLast - fontButtonFirst) {
        return;
    }
    int glyph = fontButtonFirst + index;
    fontButtons[index] = texture;
    int16_t* entry = reinterpret_cast<int16_t*>(fontGlyphs + 0x2 + glyph * 8);
    for (int style = 0; style < 10; style++) {
        entry[-1 + 0] = int16_t(vc::Call<int32_t>(fontButtons[index], 0x48));
        entry[0] = int16_t(vc::Call<int32_t>(fontButtons[index], 0x4C));
        entry = reinterpret_cast<int16_t*>(reinterpret_cast<uint8_t*>(entry) + kGlyphStyleStride);
    }
}

// 0x687DE0 (cdecl): the end of the word at `text` (the next space, line end or string
// end; "~...~" tokens skipped whole)
const uint16_t* __cdecl Font_FindWordEnd(const uint16_t* text) {
    if (*text == ' ') {
        return text;
    }
    for (;;) {
        uint16_t c = *text;
        if (c == 0 || c == '\n') {
            return text;
        }
        if (c == '~') {
            c = text[1];
            text++;
            if (c != '~') {
                for (;;) {
                    if (c == 0) {
                        goto next;
                    }
                    c = text[1];
                    text++;
                    if (c == '~') {
                        break;
                    }
                }
            }
        }
        if (*text != 0) {
            text++;
        }
    next:
        if (*text == ' ') {
            return text;
        }
    }
}

// 0x687E40 (cdecl): a "~name~" token's glyph by name (0 for none)
uint16_t __cdecl Font_FindTokenImpl(const char* name) {
    int32_t count = fontTokenCount;
    for (int32_t i = 0; i < count; i++) {
        const char* entry = reinterpret_cast<const char*>(fontTokens + i * 0x15);
        int k = 0;
        while (entry[k] == name[k] && entry[k] != 0) {
            k++;
        }
        if (entry[k] == name[k]) {
            return fontTokens[i * 0x15 + 0x14];
        }
    }
    return 0;
}

// 0x687EB0 (cdecl): a token's name read (up to "~" or the end; the characters' low bytes)
// and its glyph stored; returns where it stopped. (The game's buffer is 20 bytes; a
// longer name overran its stack.)
const uint16_t* __cdecl Font_ParseTokenImpl(const uint16_t* text, uint16_t* out) {
    char name[256];
    int n = 0;
    while (*text != '~' && *text != 0) {
        name[n++] = char(*text);
        text++;
    }
    if (n != 0) {
        name[n] = 0;
        *out = Font_FindToken(name);
    }
    return text;
}

// 0x687F10 (cdecl): a "r g b" colour token read into out[0..2]. (Returns what the game
// leaves in eax: out when read, else where the scan stopped; 0x687F90 uses it.)
const void* __cdecl Font_ParseColourImpl(const uint16_t* text, uint8_t* out) {
    char buffer[256];
    int n = 0;
    while (*text != '~' && *text != 0) {
        buffer[n++] = char(*text);
        text++;
    }
    if (n == 0) {
        return text;
    }
    buffer[n] = 0;
    int32_t r, g, b;
    Sscanf(buffer, fontColourFormat, &r, &g, &b);
    out[0] = uint8_t(r);
    out[1] = uint8_t(g);
    out[2] = uint8_t(b);
    return out;
}

// 0x687F90 (cdecl): a control token at text (pointing at its opening "~"): "b~" toggles
// bold (*bold), "c<digit>" sets a colour (unless the current one is unset), others a
// glyph; returns past its closing "~". (After a colour the game scans on from the
// colour's own bytes, the parse's eax: kept.)
const uint16_t* __cdecl Font_ParseControl(const uint16_t* text, uint8_t* colour, void*, uint8_t* bold, uint16_t* glyph) {
    const uint16_t* p = text + 1;
    uint16_t c = *p;
    const uint16_t* next = p + 1;
    bool special = false;
    if (c == 'b') {
        special = *next == '~';
    } else if (c == 'c') {
        special = *next >= '0' && *next <= '9';
    }
    if (!special) {
        p = Font_ParseToken(p, glyph);
    } else {
        bool colourUnset = fontColour[0] == 0 && fontColour[1] == 0 && fontColour[2] == 0;
        if (c == 'b') {
            *bold = *bold == 0;
        } else if (c == 'c' && !colourUnset) {
            p = static_cast<const uint16_t*>(Font_ParseColour(next, colour));
        }
    }
    while (*p != '~') {
        if (*p == 0) {
            return p;
        }
        p++;
    }
    if (*p != 0) {
        p++;
    }
    return p;
}

// 0x688050 (cdecl): the scale from a style's height (times the screen's scale)
void __cdecl Font_SetStyleScale(int style) {
    fontScaleX = fontStyleHeights[style] * screenScaleX;
    fontScaleY = fontStyleHeights[style] * screenScaleY;
}

// 0x688080 (cdecl): the scale (times the screen's)
void __cdecl Font_SetScale(float scale) {
    fontScaleX = screenScaleX * scale;
    fontScaleY = scale * screenScaleY;
}

// 0x6880B0 (cdecl): the scale as given
void __cdecl Font_SetScaleXY(float x, float y) {
    fontScaleX = x;
    fontScaleY = y;
}

// 0x6880D0 (cdecl): a number for the next "~1~" (up to 5)
void __cdecl Font_AddNumber(float number) {
    int32_t n = fontNumberCount;
    if (n < 5) {
        n++;
        fontNumbers[n] = number;
        fontNumberCount = n;
    }
}

// 0x6880F0 (cdecl): a string for the next "~a~" (up to 5)
void __cdecl Font_AddString(void* text) {
    int32_t n = fontStringCount;
    if (n < 5) {
        fontStrings[n] = text;
        fontStringCount = n + 1;
    }
}

// 0x688110 (cdecl): the 8-bit version of 0x687EB0
const char* __cdecl Font_ParseTokenA(const char* text, uint16_t* out) {
    char name[256];
    int n = 0;
    while (*text != '~' && *text != 0) {
        name[n++] = *text;
        text++;
    }
    if (n != 0) {
        name[n] = 0;
        *out = Font_FindToken(name);
    }
    return text;
}

// 0x688160 (cdecl): the justification (0 left, 1 right, 2 left too (the game's 2 and 3
// swapped), 3 centred)
void __cdecl Font_SetJustify(int justify) {
    switch (justify) {
    case 0:
        fontJustify = 0;
        break;
    case 1:
        fontJustify = 1;
        break;
    case 2:
        fontJustify = 3;
        break;
    case 3:
        fontJustify = 2;
        break;
    default:
        break;
    }
}

// 0x6881B0 (cdecl): the help message's text (up to 48 characters) and position
void __cdecl Font_SetMessage(const char* text, float x, float y) {
    Strncpy(fontMessage, text, 0x30);
    fontMessageX = x;
    fontMessageY = y;
    fontMessageShown = 0;
    fontMessageFlag = 0;
}

// 0x6881F0 (cdecl): a style's leading (by language: 7 its own table)
void __cdecl Font_SetLeading(int style, float leading) {
    if (gameLanguage == 7) {
        fontStyleLeadingJ[style] = leading;
    } else {
        fontStyleLeading[style] = leading;
    }
}

// 0x688220 (cdecl): the current style's line height at the current scale (languages 7
// and 6 map the style to their own tables)
float __cdecl Font_GetLineHeightImpl() {
    int32_t language = gameLanguage;
    int style = fontStyle;
    if (language == 7) {
        float t = fontStyleHeightJ[style] * fontScaleY;
        int32_t mapped = fontStyleMapJ[style];
        return fontStyleLineHeight[mapped] * t + t * fontStyleLeadingJ[mapped];
    }
    if (language == 6) {
        float t = fontStyleHeightR[style] * fontScaleY;
        int32_t mapped = fontStyleMapR[style];
        return fontStyleLineHeight[mapped] * t + t * fontStyleLeading[mapped];
    }
    return fontStyleLineHeight[style] * fontScaleY + fontScaleY * fontStyleLeading[style];
}

// A text's layout: +8 its widest line, +0xC the line count, +0x10 the lines (20 bytes
// each: x, y, the first and past-last characters, the width), +0x268 their count
struct FontLine {
    float x;
    float y;
    int32_t first;
    int32_t last;
    float width;
};
static_assert(sizeof(FontLine) == 0x14);

// 0x6882E0 (cdecl): a line added to a layout (an empty one only counted); the widest
// kept
void __cdecl FontLayout_AddLine(uint8_t* layout, float x, float y, int32_t first, int32_t last, float width, float lineWidth) {
    if (first == last) {
        At<float>(layout, 0xC) = At<float>(layout, 0xC) + 1.0f;
        return;
    }
    int32_t n = At<int32_t>(layout, 0x268);
    FontLine& line = reinterpret_cast<FontLine*>(layout + 0x10)[n];
    line.x = x;
    line.last = last;
    line.first = first;
    line.y = y;
    line.width = width;
    At<int32_t>(layout, 0x268) = n + 1;
    float widest = At<float>(layout, 8);
    float kept = lineWidth < widest ? widest : lineWidth;
    At<float>(layout, 8) = kept;
    At<float>(layout, 0xC) = At<float>(layout, 0xC) + 1.0f;
}

// 0x688370 (cdecl): a layout's box at (x, y) by the justification, 4 units of margin
void __cdecl FontLayout_GetBox(uint8_t* layout, float* rect, float x, float y) {
    float lineHeight = Font_GetLineHeight();
    int32_t lines = int32_t(At<float>(layout, 0xC));
    float height = lineHeight * float(lines);
    float width = At<float>(layout, 8);
    if (fontJustify == 3) {
        width = width * 0.5f;
        rect[0] = (x - 4.0f) - width;
        rect[1] = y - 4.0f;
        rect[2] = width + (x + 4.0f);
        rect[3] = (y + 4.0f) + height;
        return;
    }
    if (fontJustify == 1) {
        rect[0] = (x - 4.0f) - width;
        rect[1] = y - 4.0f;
        rect[2] = x + 4.0f;
        rect[3] = (y + 4.0f) + height;
        return;
    }
    rect[0] = x - 4.0f;
    rect[1] = y - 4.0f;
    rect[2] = (x + 4.0f) + width;
    rect[3] = (y + 4.0f) + height;
}

// 0x688460 (cdecl): the clock as "h:mm am" / "h:mm pm"
void __cdecl Font_FormatClock(char* out) {
    int32_t hours = clockHours;
    int32_t minutes = clockMinutes;
    bool am = true;
    if (hours >= 12) {
        am = false;
        if (hours > 12) {
            hours -= 12;
        }
    } else if (hours == 0) {
        hours = 12;
    }
    Sprintf(out, am ? fontAmFormat : fontPmFormat, hours, minutes);
}

// 0x6884C0 (cdecl): cents as "$d.dd" (the float quotient, exact for the game's amounts)
void __cdecl Font_FormatMoney(char* out, int32_t cents) {
    float dollars = float(cents) / 100.0f;
    Sprintf(out, fontMoneyFormat, double(dollars));
}

// A config reader's getters (the HUD layout's)
enum ReaderVf : int {
    kReadString = 0x24,   // (buffer, size, key)
    kReadFloat = 0x28,    // (out, key)
    kReadInt = 0x38,      // (out, key)
    kReadBool = 0x3C,     // (out, key)
};
// the reader's string buffer
struct ReaderString {
    re::Ptr32<char> data;
    int32_t capacity;
    char buffer[0x20];
};

// 0x6884F0 (thiscall, ret 4): CFontSetup::Setup (vf1): its alignment, position (from the
// alignment's base, at least 50 across), justification, line widths, scale, colour,
// italic / drop shadow / outline flags; a half-black shadow colour, 2 units away
void __fastcall FontSetup_Setup(uint8_t* self, re::Edx, uint8_t* reader) {
    ReaderString text;
    text.data = text.buffer;
    text.capacity = 0x1F;
    text.buffer[0] = 0;
    vc::Call<void>(reader, kReadString, &text, 0x1F, *reinterpret_cast<const re::Ptr32<const char>*>(0x00917410));
    At<int32_t>(self, 4) = HAlignFromName(text.data);
    vc::Call<void>(reader, kReadString, &text, 0x1F, *reinterpret_cast<const re::Ptr32<const char>*>(0x00917414));
    At<int32_t>(self, 8) = VAlignFromName(text.data);
    int32_t x;
    vc::Call<void>(reader, kReadInt, &x, *reinterpret_cast<const re::Ptr32<const char>*>(0x009173D8));
    int32_t y;
    vc::Call<void>(reader, kReadInt, &y, *reinterpret_cast<const re::Ptr32<const char>*>(0x009173DC));
    float baseX = 0.0f;
    if (At<int32_t>(self, 4) == 1) {
        baseX = 320.0f;
    } else if (At<int32_t>(self, 4) == 2) {
        baseX = 639.0f;
    }
    int32_t xi = int32_t(float(x) + baseX);
    float baseY = 0.0f;
    if (At<int32_t>(self, 8) == 1) {
        baseY = 240.0f;
    } else if (At<int32_t>(self, 8) == 2) {
        baseY = 479.0f;
    }
    int32_t yi = int32_t(float(y) + baseY);
    if (xi < 50) {
        xi = 50;
    }
    At<float>(self, 0xC) = float(xi);
    At<float>(self, 0x10) = float(yi);
    vc::Call<void>(reader, kReadString, &text, 0x1F, reinterpret_cast<const char*>(0x0093EE44));   // "Justification"
    At<int32_t>(self, 0x24) = JustifyFromName(text.data);
    vc::Call<void>(reader, kReadFloat, self + 0x28, reinterpret_cast<const char*>(0x0093EE30));   // "Line Width Normal"
    vc::Call<void>(reader, kReadFloat, self + 0x2C, reinterpret_cast<const char*>(0x0093EE18));   // "Line Width Widescreen"
    vc::Call<void>(reader, kReadString, &text, 0x1F, reinterpret_cast<const char*>(0x008FFFA8));  // "Scale"
    At<int32_t>(self, 0x14) = ScaleFromName(text.data);
    int32_t r, g, b, a;
    vc::Call<void>(reader, kReadInt, &r, reinterpret_cast<const char*>(0x00917384));   // "Red"
    vc::Call<void>(reader, kReadInt, &g, reinterpret_cast<const char*>(0x0091737C));
    vc::Call<void>(reader, kReadInt, &b, reinterpret_cast<const char*>(0x00917374));
    vc::Call<void>(reader, kReadInt, &a, reinterpret_cast<const char*>(0x0091736C));
    uint8_t colour[4];
    uint8_t* c = RGBA_Construct(colour, r, g, b, a);
    self[0x18] = c[0];
    self[0x19] = c[1];
    self[0x1A] = c[2];
    self[0x1B] = c[3];
    vc::Call<void>(reader, kReadBool, self + 0x30, reinterpret_cast<const char*>(0x0093EE10));   // "Italic"
    vc::Call<void>(reader, kReadBool, self + 0x31, reinterpret_cast<const char*>(0x0093EE04));   // "Drop Shadow"
    vc::Call<void>(reader, kReadBool, self + 0x32, reinterpret_cast<const char*>(0x0093EDFC));   // "Outline"
    c = RGBA_Construct(colour, 0, 0, 0, 0x80);
    self[0x1C] = c[0];
    self[0x1D] = c[1];
    self[0x1E] = c[2];
    self[0x1F] = c[3];
    At<float>(self, 0x20) = 2.0f;
}

#define FONT_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

FONT_REVERSED("CFont::IsNone_687A30", 0x00687A30, &Font_IsNone);
FONT_REVERSED("CFont::ClampWidth_687A50", 0x00687A50, &Font_ClampWidth);
FONT_REVERSED("CFont::ClampHeight_687AB0", 0x00687AB0, &Font_ClampHeight);
FONT_REVERSED("CFontBox::AdjustForScreen_687B10", 0x00687B10, &FontBox_AdjustForScreen);
FONT_REVERSED("CFont::LoadStyle_687BC0", 0x00687BC0, &FontStyle_Load);
FONT_REVERSED("CFont::UnloadStyle_687D10", 0x00687D10, &FontStyle_Unload);
FONT_REVERSED("CFont::SetButton_687D60", 0x00687D60, &Font_SetButton);
// Register hazard (regcheck2): the game's CFont::LayoutLines 0x68AA40 keeps the justification
// in dx across its calls to 0x687DE0 and 0x688220: turn those on only with 0x68AA40.
FONT_REVERSED("CFont::FindWordEnd_687DE0", 0x00687DE0, &Font_FindWordEnd);
FONT_REVERSED("CFont::FindToken_687E40", 0x00687E40, &Font_FindTokenImpl);
FONT_REVERSED("CFont::ParseToken_687EB0", 0x00687EB0, &Font_ParseTokenImpl);
FONT_REVERSED("CFont::ParseColour_687F10", 0x00687F10, &Font_ParseColourImpl);
FONT_REVERSED("CFont::ParseControl_687F90", 0x00687F90, &Font_ParseControl);
FONT_REVERSED("CFont::SetStyleScale_688050", 0x00688050, &Font_SetStyleScale);
FONT_REVERSED("CFont::SetScale_688080", 0x00688080, &Font_SetScale);
FONT_REVERSED("CFont::SetScaleXY_6880B0", 0x006880B0, &Font_SetScaleXY);
FONT_REVERSED("CFont::AddNumber_6880D0", 0x006880D0, &Font_AddNumber);
FONT_REVERSED("CFont::AddString_6880F0", 0x006880F0, &Font_AddString);
FONT_REVERSED("CFont::ParseTokenA_688110", 0x00688110, &Font_ParseTokenA);
FONT_REVERSED("CFont::SetJustify_688160", 0x00688160, &Font_SetJustify);
FONT_REVERSED("CFont::SetMessage_6881B0", 0x006881B0, &Font_SetMessage);
FONT_REVERSED("CFont::SetLeading_6881F0", 0x006881F0, &Font_SetLeading);
FONT_REVERSED("CFont::GetLineHeight_688220", 0x00688220, &Font_GetLineHeightImpl);
FONT_REVERSED("CFontLayout::AddLine_6882E0", 0x006882E0, &FontLayout_AddLine);
FONT_REVERSED("CFontLayout::GetBox_688370", 0x00688370, &FontLayout_GetBox);
FONT_REVERSED("CFont::FormatClock_688460", 0x00688460, &Font_FormatClock);
FONT_REVERSED("CFont::FormatMoney_6884C0", 0x006884C0, &Font_FormatMoney);
FONT_REVERSED("CFontSetup::Setup", 0x006884F0, &FontSetup_Setup);

} // namespace
