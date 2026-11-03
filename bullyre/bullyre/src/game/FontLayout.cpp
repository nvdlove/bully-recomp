// The font's text measuring and line layout (0x68A330..0x68B0FC; drawing in
// FontRender.cpp): a character's width (glyphs from 0x8C are characters, below are the
// button glyphs; the Japanese / Russian styles look their characters up in their own code
// tables), a string's width (wide text, or UTF-8 after a 0xFFFF marker), and the layout of
// a text into lines within the wrap bound (the layout at 0x208B288: +0 / +4 the start, +8
// the widest line, +0xC the line count, +0x10 the lines, +0x268 their count).

#include <cstdint>
#include <type_traits>

#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using vc::At;

GAME_VAR(int32_t, flLanguage, 0x00C18724);
GAME_VAR(int16_t, flStyle, 0x0208B788);
GAME_VAR(int16_t, flJustify, 0x0208B786);
GAME_VAR(uint8_t, flFlags, 0x0208B78A);
GAME_VAR(float, flScaleX, 0x0208B76C);
GAME_VAR(float, flWrapLeft, 0x0208B774);
GAME_VAR(float, flWrapCentre, 0x0208B778);
GAME_VAR(float, flWrapRight, 0x0208B77C);
GAME_VAR(int32_t, flDoubleFirst, 0x0208B270);   // button glyphs drawn twice as wide
GAME_VAR(int32_t, flDoubleLast, 0x0208B274);
GAME_VAR(int32_t, flWideFirst, 0x0208B278);     // and 1.6 times
GAME_VAR(int32_t, flWideLast, 0x0208B27C);
GAME_ARRAY(int32_t, flButtonGlyphs, 0x01B89258);
GAME_ARRAY(float, flSpacing, 0x00BA4E14);        // [10] extra spacing per style
GAME_VAR(float, flButtonScaleJ, 0x00BA4E10);
GAME_ARRAY(float, flHeightJ, 0x00BA4EB4);
GAME_ARRAY(int32_t, flMapJ, 0x00BA4EDC);
GAME_ARRAY(float, flHeightR, 0x00BA4F2C);
GAME_ARRAY(int32_t, flMapR, 0x00BA4F54);
inline uint8_t* const flGlyphs = reinterpret_cast<uint8_t*>(0x01B8A9C4);
constexpr int kFlGlyphStride = 0x7FF8A;
// the Japanese / Russian code tables (10 bytes: the code, then the glyph's left / right)
inline uint8_t* const flCodesJ = reinterpret_cast<uint8_t*>(0x00B9DBB8);   // 0x68F entries
inline uint8_t* const flCodesR = reinterpret_cast<uint8_t*>(0x00BA1D50);   // 0x4E0 entries
inline uint16_t* const flHashBuffer = reinterpret_cast<uint16_t*>(0x01B89D80);
inline uint8_t* const flLayout = reinterpret_cast<uint8_t*>(0x0208B288);

GAME_FN(uint16_t, __cdecl, Utf8_Decode, 0x00690150, const char* text);
GAME_FN(void, __cdecl, HashToText, 0x006900C0, const void* hashed, uint16_t* text);
// ours, through the game's addresses
GAME_FN(float, __cdecl, Font_ClampWidth, 0x00687A50, float x);
GAME_FN(void*, __cdecl, Font_GetButtonTexture, 0x00688980, int glyph, re::Ptr32<const uint16_t>* text);
GAME_FN(void, __cdecl, Font_FormatText, 0x00688ED0, uint16_t* out, const uint16_t* text, int size);
GAME_FN(float, __cdecl, Font_GetCharWidth, 0x0068A330, uint32_t glyph);
GAME_FN(float, __cdecl, Font_GetStringWidth8, 0x0068A810, const char* text, int withSpaces);
GAME_FN(float, __cdecl, Font_GetStringWidth, 0x0068A920, const uint16_t* text, int withSpaces);
GAME_FN(const uint16_t*, __cdecl, Font_FindWordEnd, 0x00687DE0, const uint16_t* text);
GAME_FN(const uint16_t*, __cdecl, Font_ParseToken, 0x00687EB0, const uint16_t* text, uint16_t* out);
GAME_FN(const char*, __cdecl, Font_ParseTokenA, 0x00688110, const char* text, uint16_t* out);
GAME_FN(void, __cdecl, FontLayout_AddLine, 0x006882E0, uint8_t* layout, float x, float y, const void* first, const void* last,
        float width, float lineWidth);
GAME_FN(void, __cdecl, FontLayout_GetBox, 0x00688370, uint8_t* layout, float* rect, float x, float y);
GAME_FN(float, __cdecl, Font_GetLineHeight, 0x00688220);
GAME_FN(void, __cdecl, Font_LayoutLines, 0x0068AA40, uint8_t* layout, float x, float y, const uint16_t* text, int noWrap);

inline int16_t GlyphShort(int style, int glyph, int offset) {
    return At<int16_t>(flGlyphs, style * kFlGlyphStride + glyph * 8 + offset);
}
// a style's last table short (its default width)
inline int16_t StyleDefault(int style) {
    return At<int16_t>(reinterpret_cast<uint8_t*>(0x01C0A948), style * kFlGlyphStride);
}
// a code's entry in a language's table (the first when not found)
inline uint8_t* FindCode(uint8_t* table, int count, uint16_t code) {
    int i = 0;
    for (; i < count; i++) {
        if (At<uint16_t>(table, i * 10) == code) {
            break;
        }
    }
    if (i >= count) {
        i = 0;
    }
    return table + i * 10;
}

// 0x68A330 (cdecl): a glyph's width at the current scale: a button glyph (below 0x8C) by
// its texture's aspect at the glyph's height (0.7 for style 1; some doubled or 1.6 wide;
// a binding's name measured instead when there's no texture or it's a key), a character by
// its table (with bold (flags 2) its own glyph width, else the style's default), plus the
// style's spacing
float __cdecl Font_GetCharWidthImpl(uint32_t code) {
    uint16_t glyph = uint16_t(code);
    int style = flStyle;
    int mapped = style;
    float scale = flScaleX;
    bool character;
    if (flLanguage == 7) {
        mapped = flMapJ[style];
        character = glyph >= 0x8C;
        if (character) {
            scale = flHeightJ[style] * scale;
        }
    } else if (flLanguage == 6) {
        mapped = flMapR[style];
        character = glyph >= 0x8C;
        if (character) {
            scale = flHeightR[style] * scale;
        }
    } else {
        character = glyph >= 0x8C;
    }
    if (!character) {
        float factor = mapped == 1 ? 0.699999988079071f : 1.0f;
        float buttonScale = Font_ClampWidth(scale) * factor;
        int32_t index = glyph;
        if (index >= flDoubleFirst && index < flDoubleLast) {
            buttonScale = buttonScale + buttonScale;
        }
        if (index >= flWideFirst && index < flWideLast) {
            buttonScale = buttonScale * 1.600000023841858f;
        }
        float width = 0.0f;
        re::Ptr32<const uint16_t> text = nullptr;
        void* texture = Font_GetButtonTexture(flButtonGlyphs[index], &text);
        if (texture) {
            int32_t w = vc::Call<int32_t>(texture, 0x48);
            int32_t h = GlyphShort(mapped, index, 2);
            uint32_t scaled = uint32_t(w * h);
            uint32_t full = vc::Call<uint32_t>(texture, 0x4C);
            float fw = float(scaled / full);
            width = flSpacing[flStyle] + fw;
            if (flLanguage == 7 || flLanguage == 6) {
                width = flButtonScaleJ * width;
            }
            width = width * buttonScale;
            if (!text) {
                return width;
            }
            width = width + 4.0f;
        }
        if (text) {
            uint16_t formatted[512];
            Font_FormatText(formatted, text, 0x200);
            width = Font_GetStringWidth(formatted, 1) + width;
        }
        return width;
    }
    if (flLanguage == 7 || flLanguage == 6) {
        bool japanese = flLanguage == 7;
        if (uint16_t(glyph - 0x9D) <= 8) {
            glyph = 0x9C;
        }
        if (!(flFlags & 2)) {
            return (float(StyleDefault(mapped)) + flSpacing[mapped]) * scale;
        }
        float left;
        float right;
        // (the style mapped twice: the game indexes the map with the mapped style)
        int table = japanese ? flMapJ[mapped] : flMapR[mapped];
        if (glyph >= 0x8C && table == (japanese ? 8 : 9)) {
            uint16_t key = uint16_t(glyph - 0x6C);
            uint8_t* entry = japanese ? FindCode(flCodesJ, 0x68F, key) : FindCode(flCodesR, 0x4E0, key);
            left = float(int32_t(At<int16_t>(entry, 2)));
            right = float(int32_t(At<int16_t>(entry, 6)));
        } else {
            left = float(int32_t(GlyphShort(mapped, glyph, -4)));
            right = float(int32_t(GlyphShort(mapped, glyph, 0)));
        }
        return ((right - left) + flSpacing[style]) * scale;
    }
    if ((flFlags & 2) && glyph < 0x18C) {
        int32_t left = GlyphShort(mapped, glyph, -4);
        int32_t right = GlyphShort(mapped, glyph, 0);
        return (float(right - left) + flSpacing[mapped]) * scale;
    }
    return (float(StyleDefault(mapped)) + flSpacing[mapped]) * scale;
}

// the width of a "~token~" glyph (0: none)
inline float TokenWidth(uint16_t glyph) {
    if (glyph < 0x8D) {
        return Font_GetCharWidth(uint16_t(glyph - 1));
    }
    return Font_GetCharWidth(uint16_t(glyph + 0x6C));
}
// "~b~" and "~c<digit>..." tokens take no width (c: the character after the "~")
template <typename Char>
inline bool IsStyleToken(Char c, Char next) {
    if (c == 'b') {
        return next == '~';
    }
    if (c == 'c') {
        using S = std::conditional_t<sizeof(Char) == 1, signed char, uint16_t>;
        return S(next) >= S('0') && S(next) <= S('9');
    }
    return false;
}

// 0x68A810 (cdecl): a UTF-8 string's width (to its end or line end; to the first space
// unless withSpaces; a last space not counted)
float __cdecl Font_GetStringWidth8Impl(const char* text, int withSpaces) {
    if (!text) {
        return 0.0f;
    }
    float width = 0.0f;
    const char* p = text;
    for (;;) {
        char c = *p;
        if (c == ' ' && !re::Flag(withSpaces)) {
            return width;
        }
        if (c == 0 || c == '\n') {
            return width;
        }
        if (c == '~') {
            c = p[1];
            p++;
            if (c == 0) {
                continue;
            }
            if (IsStyleToken<char>(c, p[1])) {
                while (c != '~' && c != 0) {
                    c = p[1];
                    p++;
                }
            } else {
                uint16_t glyph = 0;
                p = Font_ParseTokenA(p, &glyph);
                if (glyph != 0) {
                    width = TokenWidth(glyph) + width;
                }
            }
        } else if (!(c == ' ' && p[1] == 0)) {
            width = Font_GetCharWidth(uint16_t(Utf8_Decode(p) + 0x6C)) + width;
        }
        // (past the character's continuation bytes)
        do {
            c = p[1];
            p++;
        } while (int8_t(c) < 0 && !(c & 0x40));
    }
}

// 0x68A920 (cdecl): a string's width (as 0x68A810; hashed / UTF-8 text after 0xFFFF)
float __cdecl Font_GetStringWidthImpl(const uint16_t* text, int withSpaces) {
    if (text && *text == 0xFFFF) {
        return Font_GetStringWidth8(reinterpret_cast<const char*>(text + 1), withSpaces);
    }
    float width = 0.0f;
    const uint16_t* p = text;
    for (;;) {
        uint16_t c = *p;
        if (c == ' ' && !re::Flag(withSpaces)) {
            return width;
        }
        if (c == 0 || c == '\n') {
            return width;
        }
        if (c == '~') {
            c = p[1];
            p++;
            if (c == 0) {
                continue;
            }
            if (IsStyleToken<uint16_t>(c, p[1])) {
                while (c != '~' && c != 0) {
                    c = p[1];
                    p++;
                }
            } else {
                uint16_t glyph = 0;
                p = Font_ParseToken(p, &glyph);
                if (glyph != 0) {
                    width = TokenWidth(glyph) + width;
                }
            }
        } else if (!(p[1] == 0 && c == ' ')) {
            width = Font_GetCharWidth(uint32_t(c) + 0x6C) + width;
        }
        p++;
    }
}

// a word's width from p (to a space, the end or a line end): UTF-8 after a 0xFFFF marker
float WordWidth8(const char* p) {
    float width = 0.0f;
    for (;;) {
        char c = *p;
        if (c == ' ' || c == 0 || c == '\n') {
            return width;
        }
        if (c == '~') {
            c = p[1];
            p++;
            if (c == 0) {
                continue;
            }
            if (IsStyleToken<char>(c, p[1])) {
                while (c != '~' && c != 0) {
                    c = p[1];
                    p++;
                }
            } else {
                uint16_t glyph = 0;
                p = Font_ParseTokenA(p, &glyph);
                if (glyph != 0) {
                    width = TokenWidth(glyph) + width;
                }
            }
        } else {
            width = Font_GetCharWidth(uint16_t(Utf8_Decode(p) + 0x6C)) + width;
        }
        do {
            c = p[1];
            p++;
        } while (int8_t(c) < 0 && !(c & 0x40));
    }
}
// the same for wide text (a token left open steps past the string's end, as the game)
float WordWidth(const uint16_t* p) {
    float width = 0.0f;
    for (;;) {
        uint16_t c = *p;
        if (c == ' ' || c == 0 || c == '\n') {
            return width;
        }
        if (c == '~') {
            c = p[1];
            p++;
            if (c == 0) {
                continue;
            }
            if (IsStyleToken<uint16_t>(c, p[1])) {
                while (c != '~' && c != 0) {
                    c = p[1];
                    p++;
                }
            } else {
                uint16_t glyph = 0;
                p = Font_ParseToken(p, &glyph);
                if (glyph != 0) {
                    width = TokenWidth(glyph) + width;
                }
            }
        } else {
            width = Font_GetCharWidth(uint32_t(c) + 0x6C) + width;
        }
        p++;
    }
}

// 0x68AA40 (cdecl): a text laid out into lines from (x, y): words (by their widths and a
// space's) go on a line until one overflows the wrap bound (always one a line; noWrap:
// never wrapped); each line placed by the justification (justified lines' word gaps
// widened) and stepped down by the line height
void __cdecl Font_LayoutLinesImpl(uint8_t* layout, float x, float y, const uint16_t* text, int noWrap) {
    float spaceWidth = Font_GetCharWidth(0x8C);
    float curY = y;
    float accumulated = 0.0f;
    At<int32_t>(layout, 0x268) = 0;
    At<float>(layout, 8) = 0.0f;
    At<float>(layout, 0xC) = 0.0f;
    const uint16_t* lineStart = text;
    uint8_t firstWord = 1;
    At<float>(layout, 0) = x;
    int32_t spaces = 0;
    At<float>(layout, 4) = y;
    float penX = (flJustify == 3 || flJustify == 1) ? 0.0f : x;
    float limit;
    if (flJustify == 3) {
        limit = flWrapCentre;
    } else if (flJustify == 1) {
        limit = x - flWrapRight;
    } else {
        limit = flWrapLeft;
    }
    const uint16_t* p = text;
    while (*p != 0) {
        if (!firstWord) {
            spaces++;
            penX = spaceWidth + penX;
            accumulated = spaceWidth + accumulated;
        }
        float width = *p == 0xFFFF ? WordWidth8(reinterpret_cast<const char*>(p + 1)) : WordWidth(p);
        int16_t justify = flJustify;
        float end = penX + width;
        if (limit < end && !firstWord && !re::Flag(noWrap)) {
            // the word goes on the next line: this one closed before it
            spaces--;
            penX = penX - spaceWidth;
            accumulated = accumulated - spaceWidth;
            float lineX;
            float gap = 0.0f;
            if (justify == 3) {
                lineX = x - penX * 0.5f;
            } else if (justify == 1) {
                lineX = x - penX;
            } else {
                lineX = x;
                if (justify == 2 && spaces != 0) {
                    gap = (flWrapLeft - penX) / float(spaces);
                }
            }
            FontLayout_AddLine(layout, lineX, curY, lineStart, p, gap, gap + accumulated);
        } else {
            const uint16_t* e = Font_FindWordEnd(p);
            uint16_t c = *e;
            bool last = c == 0 || (e[1] == 0 && c == ' ');
            if (!last && c != '\n') {
                p = e + 1;
                firstWord = 0;
                penX = end;
                accumulated = width + accumulated;
                continue;
            }
            if (last && e[1] == 0 && c == ' ') {
                e++;
            }
            penX = end;
            float lineX;
            if (justify == 3) {
                lineX = x - penX * 0.5f;
            } else if (justify == 1) {
                lineX = x - penX;
            } else {
                lineX = x;
            }
            FontLayout_AddLine(layout, lineX, curY, lineStart, e, 0.0f, width + accumulated);
            if (last) {
                p = e;
                continue;
            }
            p = e + 1;
        }
        // a new line from p
        lineStart = p;
        penX = (flJustify == 3 || flJustify == 1) ? 0.0f : x;
        curY = Font_GetLineHeight() + curY;
        spaces = 0;
        firstWord = 1;
        accumulated = 0.0f;
    }
}

// the text to lay out (a hashed one decoded into the shared buffer)
inline const uint16_t* Decoded(const uint16_t* text) {
    if (text && *text == 0xFFFF) {
        HashToText(text + 1, flHashBuffer);
        return flHashBuffer;
    }
    return text;
}

// 0x68AFE0 (cdecl): a text laid out into the shared layout
void __cdecl Font_LayoutText(float x, float y, const uint16_t* text) {
    Font_LayoutLines(flLayout, x, y, Decoded(text), 0);
}

// 0x68B030 (cdecl): the number of lines a text takes
int32_t __cdecl Font_GetNumberOfLines(float x, float y, const uint16_t* text) {
    Font_LayoutLines(flLayout, x, y, Decoded(text), 0);
    return int32_t(At<float>(flLayout, 0xC));
}

// 0x68B090 (cdecl): the box a text takes at (x, y)
float* __cdecl Font_GetStringRect(float* rect, float x, float y, const uint16_t* text) {
    Font_LayoutLines(flLayout, x, y, Decoded(text), 0);
    FontLayout_GetBox(flLayout, rect, At<float>(flLayout, 0), At<float>(flLayout, 4));
    return rect;
}

#define FONTLAYOUT_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

FONTLAYOUT_REVERSED("CFont::GetCharWidth_68A330", 0x0068A330, &Font_GetCharWidthImpl);
FONTLAYOUT_REVERSED("CFont::GetStringWidth8_68A810", 0x0068A810, &Font_GetStringWidth8Impl);
FONTLAYOUT_REVERSED("CFont::GetStringWidth_68A920", 0x0068A920, &Font_GetStringWidthImpl);
FONTLAYOUT_REVERSED("CFont::LayoutLines_68AA40", 0x0068AA40, &Font_LayoutLinesImpl);
FONTLAYOUT_REVERSED("CFont::LayoutText_68AFE0", 0x0068AFE0, &Font_LayoutText);
FONTLAYOUT_REVERSED("CFont::GetNumberOfLines_68B030", 0x0068B030, &Font_GetNumberOfLines);
FONTLAYOUT_REVERSED("CFont::GetStringRect_68B090", 0x0068B090, &Font_GetStringRect);

} // namespace
