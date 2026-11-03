// The font's drawing (0x68B100..0x68DC31): printing lays text out into lines (layout at
// 0x208B288) and queues each line into a render buffer, a 0x1C byte record (the render
// details below) followed by the line's characters and a 0 (the next record 4-aligned).
// Two buffers: the text buffer (0x1B8A588.., cursor 0x1B8A184, drawn by Render) and the
// button text buffer (0x1B8A188.., cursor 0x1B8A180, drawn by Flush; a button glyph's
// binding name is queued there). Drawing walks the characters: "~" controls parsed (the
// colour, bold (drawn three times), a token glyph), then each glyph drawn as a textured
// rectangle (a button glyph by its texture, the Japanese / Russian styles through their
// own code tables and scales). Also the underline, the background box, the drop shadow /
// outline, and the fading title text (the zone name).
//
// The render details (0x208B7E0, 0x1C bytes, the same layout as a buffer record):
// +0 x, +4 y, +8 / +0xC the scale, +0x10 the colour, +0x14 the extra spacing after glyph
// 0x8C (the space), +0x18 the style (short), +0x1A flags (1: keep the colour, 2: fixed
// width off (bold widths), 4: italic).

#include <cmath>
#include <cstdint>
#include <cstring>

#include "game/VCall.h"
#include "reversed.h"

namespace {

using vc::At;

GAME_VAR(int32_t, frLanguage, 0x00C18724);
GAME_VAR(float, frScreenScaleX, 0x00A957A8);
GAME_VAR(float, frScreenScaleY, 0x00A957AC);
// the print details (CFont's)
GAME_VAR(float, frFontScaleX, 0x0208B76C);
GAME_VAR(float, frFontScaleY, 0x0208B770);
GAME_VAR(float, frWrapRight, 0x0208B77C);
GAME_VAR(int16_t, frShadow, 0x0208B784);
GAME_VAR(int16_t, frJustify, 0x0208B786);
GAME_VAR(int16_t, frFontStyle, 0x0208B788);
GAME_VAR(uint8_t, frFontFlags, 0x0208B78A);   // 1 background, 4 in a shadow, 8 bold, 0x20 outline, 0x40 underline
inline uint8_t* const frFontColour = reinterpret_cast<uint8_t*>(0x0208B764);
inline uint8_t* const frBackColour = reinterpret_cast<uint8_t*>(0x0208B768);
inline uint8_t* const frShadowColour = reinterpret_cast<uint8_t*>(0x0208B780);
GAME_ARRAY(float, frStyleHeights, 0x0208B240);
GAME_ARRAY(void*, frTextures, 0x0208B218);
GAME_ARRAY(float, frInvSize, 0x0208B790);          // [10][2] 1 / the texture's width, height
GAME_ARRAY(float, frLineHeight, 0x00BA4E8C);
GAME_ARRAY(float, frSpacing, 0x00BA4E14);
GAME_VAR(float, frButtonScaleJ, 0x00BA4E10);
GAME_ARRAY(float, frHeightJ, 0x00BA4EB4);
GAME_ARRAY(int32_t, frMapJ, 0x00BA4EDC);
GAME_ARRAY(float, frHeightR, 0x00BA4F2C);
GAME_ARRAY(int32_t, frMapR, 0x00BA4F54);
GAME_VAR(int32_t, frDoubleFirst, 0x0208B270);
GAME_VAR(int32_t, frDoubleLast, 0x0208B274);
GAME_VAR(int32_t, frWideFirst, 0x0208B278);
GAME_VAR(int32_t, frWideLast, 0x0208B27C);
GAME_ARRAY(int32_t, frButtonGlyphs, 0x01B89258);
GAME_VAR(int32_t, frCustomCount, 0x0208B280);
GAME_ARRAY(uint8_t*, frCustomSetups, 0x01B89230);
inline uint8_t* const frSetups = reinterpret_cast<uint8_t*>(0x0208B800);   // [5] of 0x38
// set by a "~B~" token (FontSystem.cpp's fsBoldNext): a dark box behind the next print
GAME_VAR(uint8_t, frBoxNext, 0x01B8A9B9);
// the render details
GAME_VAR(float, frX, 0x0208B7E0);
GAME_VAR(float, frY, 0x0208B7E4);
GAME_VAR(float, frScaleX, 0x0208B7E8);
GAME_VAR(float, frScaleY, 0x0208B7EC);
GAME_VAR(float, frExtraSpace, 0x0208B7F4);
GAME_VAR(int16_t, frStyle, 0x0208B7F8);
GAME_VAR(uint8_t, frFlags, 0x0208B7FA);
inline uint8_t* const frRender = reinterpret_cast<uint8_t*>(0x0208B7E0);
inline uint8_t* const frColour = reinterpret_cast<uint8_t*>(0x0208B7F0);
inline uint8_t* const frColourAlt = reinterpret_cast<uint8_t*>(0x0208B7FC);
inline uint8_t* const frKeptColour = reinterpret_cast<uint8_t*>(0x020C331C);   // (flags 1)
inline uint8_t* const frPlainColour = reinterpret_cast<uint8_t*>(0x020C333C);
// the buffers
GAME_VAR(uint8_t*, frButtonCursor, 0x01B8A180);
GAME_VAR(uint8_t*, frTextCursor, 0x01B8A184);
inline uint8_t* const frButtonBuffer = reinterpret_cast<uint8_t*>(0x01B8A188);
inline uint8_t* const frButtonBufferEnd = reinterpret_cast<uint8_t*>(0x01B8A568);
inline uint8_t* const frTextBuffer = reinterpret_cast<uint8_t*>(0x01B8A588);
inline uint8_t* const frTextBufferEnd = reinterpret_cast<uint8_t*>(0x01B8A968);
inline uint8_t* const frLayout = reinterpret_cast<uint8_t*>(0x0208B288);
inline uint8_t* const frButtonLayout = reinterpret_cast<uint8_t*>(0x0208B4F8);
inline uint16_t* const frHashBuffer = reinterpret_cast<uint16_t*>(0x01B89D80);
inline uint16_t* const frButtonHashBuffer = reinterpret_cast<uint16_t*>(0x01B89580);
// glyph tables: per style 0x7FF8A bytes of 8 byte glyphs (left, top, right, bottom shorts
// at -4, -2, 0, +2 from 0x1B8A9C4 + glyph * 8)
inline uint8_t* const frGlyphs = reinterpret_cast<uint8_t*>(0x01B8A9C4);
constexpr int kFrGlyphStride = 0x7FF8A;
inline uint8_t* const frCodesJ = reinterpret_cast<uint8_t*>(0x00B9DBB8);   // 0x68F of 10 bytes
inline uint8_t* const frCodesR = reinterpret_cast<uint8_t*>(0x00BA1D50);   // 0x4E0 of 10 bytes
// the title (zone name) text
GAME_VAR(uint8_t, frTitleFading, 0x0208B265);
GAME_VAR(float, frTitleTimer, 0x0208B268);
GAME_VAR(float, frTitleDuration, 0x0208B26C);
GAME_VAR(float, frTitleRate, 0x00BA4FCC);
inline char* const frTitleKey = reinterpret_cast<char*>(0x01B8A988);
inline uint8_t* const frTitleSetup = reinterpret_cast<uint8_t*>(0x0208B8E0);   // setup 4
inline void* const frTheText = reinterpret_cast<void*>(0x020C2FA4);
GAME_VAR(float, frFadeAlpha, 0x00C3CD28);
GAME_VAR(float, frTimeStep, 0x00C1A9A4);
GAME_VAR(uint8_t, frUserStart, 0x020C32E9);
GAME_VAR(uint8_t, frFlag20C32EA, 0x020C32EA);

GAME_FN(void, __cdecl, HashToText, 0x006900C0, const void* hashed, uint16_t* text);
GAME_FN(uint8_t*, __thiscall, RGBA_Construct, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(const uint16_t*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(uint32_t, __cdecl, RwRenderStateGet, 0x00519290, int state, uint32_t* value);
GAME_FN(uint32_t, __cdecl, RwRenderStateSet, 0x00519790, int state, uint32_t value);
GAME_FN(uint32_t, __cdecl, RwRasterFromTexture, 0x0051A120, void* texture);
GAME_FN(void, __cdecl, Im2D_Flush, 0x0052C5C0);
GAME_FN(void, __cdecl, DrawRect, 0x0052C470, const float* rect, const uint8_t* colour);
GAME_FN(void, __cdecl, DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture,
        const uint8_t* colour, const float* uv1, const float* uv2);
GAME_FN(void, __cdecl, DrawRectUVB, 0x0052CA30, float x1, float y1, float x2, float y2, void* texture,
        const uint8_t* colour, const float* uv1, const float* uv2);
GAME_FN(void, __cdecl, DrawQuadUVB, 0x0052CE00, float x1, float y1, float x2, float y2, float x3, float y3,
        float x4, float y4, void* texture, const uint8_t* colour, const float* uv1, const float* uv2);
GAME_FN(void, __cdecl, DrawLine, 0x0052CF40, float x1, float y1, float x2, float y2, const float* width,
        const uint8_t* colour);
GAME_FN(float, __thiscall, FontSetup_GetWrap, 0x0055B5F0, uint8_t* setup);
GAME_FN(void, __cdecl, Font_SetColour, 0x00424480, const uint8_t* colour);
// ours, through the game's addresses
GAME_FN(float, __cdecl, Font_ClampWidth, 0x00687A50, float x);
GAME_FN(float, __cdecl, Font_ClampHeight, 0x00687AB0, float y);
GAME_FN(const uint16_t*, __cdecl, Font_ParseControl, 0x00687F90, const uint16_t* text, uint8_t* colour, void*,
        uint8_t* bold, uint16_t* glyph);
GAME_FN(float, __cdecl, Font_GetLineHeight, 0x00688220);
GAME_FN(void, __cdecl, FontLayout_GetBox, 0x00688370, uint8_t* layout, float* rect, float x, float y);
GAME_FN(void*, __cdecl, Font_GetButtonTexture, 0x00688980, int glyph, re::Ptr32<const uint16_t>* text);
GAME_FN(uint8_t*, __thiscall, FontDetails_CopyFrom, 0x00688DD0, uint8_t* self, const uint8_t* other);
GAME_FN(void, __cdecl, Font_FormatText, 0x00688ED0, uint16_t* out, const uint16_t* text, int size);
GAME_FN(void, __thiscall, FontSetup_Apply, 0x00689400, uint8_t* setup);
GAME_FN(float, __cdecl, Font_GetStringWidth, 0x0068A920, const uint16_t* text, int withSpaces);
GAME_FN(void, __cdecl, Font_LayoutLines, 0x0068AA40, uint8_t* layout, float x, float y, const uint16_t* text, int noWrap);
GAME_FN(void, __cdecl, Font_LayoutText, 0x0068AFE0, float x, float y, const uint16_t* text);
GAME_FN(float, __cdecl, Font_RenderGlyphWidth, 0x0068B100, uint32_t glyph);
GAME_FN(void, __cdecl, Font_FlushButtons, 0x0068B460);
GAME_FN(float, __cdecl, Font_QueueButtonText, 0x0068BAE0, float x, float y, const uint16_t* text);
GAME_FN(void, __cdecl, Font_DrawGlyph, 0x0068BE00, float x, float y, uint32_t glyph);
GAME_FN(void, __cdecl, Font_Render, 0x0068C660);
GAME_FN(void, __cdecl, Font_QueueLine, 0x0068CE00, float x, float y, const uint16_t* first, const uint16_t* last,
        float extra);
GAME_FN(void, __cdecl, Font_QueueLayout, 0x0068D2F0, uint8_t* layout);
GAME_FN(void, __cdecl, Font_PrintLayout, 0x0068D340);
GAME_FN(void, __cdecl, Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int noColour);

inline int16_t& GlyphShort(int style, int glyph, int offset) {
    return At<int16_t>(frGlyphs, style * kFrGlyphStride + glyph * 8 + offset);
}
// a style's last table short (its default width)
inline int16_t StyleDefault(int style) {
    return At<int16_t>(reinterpret_cast<uint8_t*>(0x01C0A948), style * kFrGlyphStride);
}
inline uint8_t* FindCode(uint8_t* table, int count, int code) {
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
inline uint8_t Merge(uint8_t bits, uint8_t from, uint8_t mask) {
    return uint8_t(bits ^ ((bits ^ from) & mask));
}
inline bool InRange(int32_t index, int32_t first, int32_t last) {
    return index >= first && index < last;
}

// 0x68B100 (cdecl): a glyph's width at the render details, unscaled (a button glyph: its
// texture's aspect at the glyph's height, doubled / 1.6 wide for some, plus its binding
// name's width; a character: its table width with fixed width off (flags 2), else the
// style's default width)
float __cdecl Font_RenderGlyphWidthImpl(uint32_t code) {
    int32_t lang = frLanguage;
    if (lang == 7 && uint16_t(code - 0x9D) <= 8) {
        code = 0x9C;
    }
    int16_t style = frStyle;
    uint16_t mapped = uint16_t(style);
    if (lang == 7) {
        mapped = uint16_t(frMapJ[style]);
    } else if (lang == 6) {
        mapped = uint16_t(frMapR[style]);
    }
    uint16_t glyph = uint16_t(code);
    int32_t index = glyph;
    float factor = 1.0f;
    if (InRange(index, frDoubleFirst, frDoubleLast)) {
        factor = 2.0f;
    }
    if (InRange(index, frWideFirst, frWideLast)) {
        factor = 1.600000023841858f;
    }
    if (glyph < 0x8C) {
        float width = 0.0f;
        re::Ptr32<const uint16_t> text = nullptr;
        void* texture = Font_GetButtonTexture(frButtonGlyphs[index], &text);
        if (texture) {
            int32_t w = vc::Call<int32_t>(texture, 0x48);
            uint32_t scaled = uint32_t(w * GlyphShort(int16_t(mapped), index, 2));
            uint32_t full = vc::Call<uint32_t>(texture, 0x4C);
            width = float(scaled / full) * factor;
            if (frLanguage == 7 || frLanguage == 6) {
                width = frButtonScaleJ * width;
            }
            if (!text) {
                return width;
            }
            width = width + 4.0f;
        }
        if (text) {
            uint16_t formatted[512];
            Font_FormatText(formatted, text, 0x200);
            float textWidth = Font_GetStringWidth(formatted, 1);
            float clamped = Font_ClampWidth(frScaleX);
            textWidth = (1.0f / clamped) * textWidth;
            float m = mapped == 1 ? 0.699999988079071f : 1.0f;
            textWidth = (1.0f / m) * textWidth;
            width = textWidth + width;
        }
        return width;
    }
    if (!(frFlags & 2)) {
        return float(StyleDefault(style));
    }
    float left;
    float right;
    if (lang == 7) {
        uint8_t* entry = FindCode(frCodesJ, 0x68F, index - 0x6C);
        left = float(int32_t(At<int16_t>(entry, 2)));
        right = float(int32_t(At<int16_t>(entry, 6)));
    } else if (lang == 6) {
        uint8_t* entry = FindCode(frCodesR, 0x4E0, index - 0x6C);
        left = float(int32_t(At<int16_t>(entry, 2)));
        right = float(int32_t(At<int16_t>(entry, 6)));
    } else {
        left = float(int32_t(GlyphShort(style, index, -4)));
        right = float(int32_t(GlyphShort(style, index, 0)));
    }
    return (right - left) * factor;
}

// the Japanese / Russian style tables (null: other languages)
inline bool LanguageTables(re::Ptr32<const float>& heights, re::Ptr32<const int32_t>& maps) {
    if (frLanguage == 7) {
        heights = frHeightJ;
        maps = frMapJ;
        return true;
    }
    if (frLanguage == 6) {
        heights = frHeightR;
        maps = frMapR;
        return true;
    }
    return false;
}

// before a buffer's drawn: the render details' scale by the language's style height (the
// height itself for a 0 scale), the style mapped
void ScaleRenderForLanguage() {
    re::Ptr32<const float> heights;
    re::Ptr32<const int32_t> maps;
    if (!LanguageTables(heights, maps)) {
        return;
    }
    int16_t style = frStyle;
    float height = heights[style];
    frScaleX = frScaleX == 0.0f ? height : frScaleX * height;
    float y = frScaleY == 0.0f ? height : frScaleY * height;
    frStyle = int16_t(maps[style]);
    frScaleY = y;
}

// a buffer's segments drawn (the first record at base), the cursor reset. kSavedButtonScale:
// a button glyph's advance clamped at the segment's own scale (Flush) or at the current
// one (Render).
template <bool kSavedButtonScale>
void DrawBuffer(re::Ptr32<uint8_t>& cursor, uint8_t* base) {
    FontDetails_CopyFrom(frRender, base);
    // the segment's own scale and style (the J / R styles are mapped, scaled, for characters)
    float savedX = frScaleX;
    float savedY = frScaleY;
    int16_t savedStyle = frStyle;
    re::Ptr32<const float> heights;
    re::Ptr32<const int32_t> maps;
    if (LanguageTables(heights, maps)) {
        frStyle = int16_t(maps[savedStyle]);
        frScaleX = savedX * heights[savedStyle];
        frScaleY = savedY * heights[savedStyle];
    }
    uint8_t colour[4];
    uint8_t saved[4];
    std::memcpy(colour, frColour, 4);
    float x = frX;
    float y = frY;
    uint8_t bold = 0;
    bool restoreColour = false;
    uint16_t token = 0;
    uint16_t* p = reinterpret_cast<uint16_t*>(base + 0x1C);
    while (reinterpret_cast<uint8_t*>(p) < cursor) {
        if (*p == 0) {
            p++;
            if (reinterpret_cast<uintptr_t>(p) & 3) {
                p++;
            }
            if (reinterpret_cast<uint8_t*>(p) >= cursor) {
                break;
            }
            FontDetails_CopyFrom(frRender, reinterpret_cast<uint8_t*>(p));
            // (the game only saves these for the J / R languages, the only ones using them)
            if (LanguageTables(heights, maps)) {
                savedX = frScaleX;
                savedY = frScaleY;
                savedStyle = frStyle;
                frStyle = int16_t(maps[savedStyle]);
                frScaleX = savedX * heights[savedStyle];
                frScaleY = savedY * heights[savedStyle];
            }
            std::memcpy(colour, frColour, 4);
            x = frX;
            y = frY;
            p += 0x1C / 2;
        }
        if (*p == '~') {
            std::memcpy(saved, colour, 4);
            p = const_cast<uint16_t*>(Font_ParseControl(p, colour, &bold, &bold, &token));
            if (token != 0) {
                restoreColour = true;
            }
            if (!(frFlags & 1)) {
                std::memcpy(frColour, colour, 4);
            }
            if (token == 0) {
                continue;
            }
            p--;
        }
        uint16_t glyph;
        if (token == 0) {
            glyph = uint16_t(*p + 0x6C);
        } else {
            glyph = token < 0x8D ? uint16_t(token - 1) : uint16_t(token + 0x6C);
            token = 0;
        }
        if (frLanguage == 7 || frLanguage == 6) {
            if (glyph < 0x8C) {
                frScaleX = savedX;
                frScaleY = savedY;
            }
            frStyle = savedStyle;
        }
        Font_DrawGlyph(x, y, glyph);
        if (bold) {
            Font_DrawGlyph(x + 1.0f, y, glyph);
            x = x + 2.0f;
            Font_DrawGlyph(x, y, glyph);
        }
        int32_t lang = frLanguage;
        if (LanguageTables(heights, maps)) {
            int16_t style = frStyle;
            if (glyph < 0x8C) {
                frScaleX = heights[style] * frScaleX;
                frScaleY = heights[style] * frScaleY;
            }
            frStyle = int16_t(maps[style]);
        }
        float spacing;
        if (glyph < 0x8C) {
            float advance;
            if (lang == 7 || lang == 6) {
                if (kSavedButtonScale) {
                    float width = Font_RenderGlyphWidth(glyph);
                    advance = Font_ClampWidth(savedX) * width;
                } else {
                    float clamped = Font_ClampWidth(frScaleX);
                    advance = Font_RenderGlyphWidth(glyph) * clamped;
                }
            } else {
                float m = frStyle == 1 ? 0.699999988079071f : 1.0f;
                float clamped = Font_ClampWidth(frScaleX);
                advance = (Font_RenderGlyphWidth(glyph) * clamped) * m;
            }
            x = advance + x;
            spacing = frSpacing[frStyle] * frScaleX;
        } else {
            float width = Font_RenderGlyphWidth(glyph);
            float scale = frScaleX;
            x = width * scale + x;
            spacing = scale * frSpacing[frStyle];
        }
        x = spacing + x;
        if (glyph == 0x8C) {
            x = frExtraSpace + x;
        }
        if (restoreColour) {
            std::memcpy(colour, saved, 4);
            if (!(frFlags & 1)) {
                std::memcpy(frColour, saved, 4);
            }
            restoreColour = false;
        }
        p++;
    }
    cursor = base;
}

// 0x68B460 (cdecl): the button text buffer drawn
void __cdecl Font_FlushButtonsImpl() {
    if (frButtonCursor == frButtonBuffer) {
        return;
    }
    ScaleRenderForLanguage();
    DrawBuffer<true>(frButtonCursor, frButtonBuffer);
}

// 0x68BAE0 (cdecl): a button glyph's binding name laid out at the render details' scale
// and style and queued into the button text buffer; its width. (The "~" controls' text
// is copied to the text buffer's cursor instead, as the game does.)
float __cdecl Font_QueueButtonTextImpl(float x, float y, const uint16_t* text) {
    if (text && *text == 0xFFFF) {
        HashToText(text + 1, frButtonHashBuffer);
        text = frButtonHashBuffer;
    }
    float savedScale = frFontScaleX;
    int16_t savedStyle = frFontStyle;
    frFontScaleX = frScaleX;
    frJustify = 0;
    frFontStyle = frStyle;
    Font_LayoutLines(frButtonLayout, x, y, text, 1);
    // (rect[1] / rect[3] left unset by the game; the box sets all four)
    float rect[4] = {1000000.0f, 0.0f, -1000000.0f, 0.0f};
    FontLayout_GetBox(frButtonLayout, rect, At<float>(frButtonLayout, 0), At<float>(frButtonLayout, 4));
    frFontScaleX = savedScale;
    frFontStyle = savedStyle;
    for (int32_t i = 0; i < At<int32_t>(frButtonLayout, 0x268); i++) {
        uint8_t* line = frButtonLayout + 0x10 + i * 0x14;
        float lineX = At<float>(line, 0);
        float lineY = At<float>(line, 4);
        re::Ptr32<const uint16_t>& first = At<const uint16_t*>(line, 8);
        const uint16_t* last = At<const uint16_t*>(line, 0xC);
        int32_t count = int32_t(reinterpret_cast<intptr_t>(last) - re::PtrCast<intptr_t>(first)) >> 1;
        if (frButtonCursor >= frButtonBufferEnd - count * 2) {
            Font_FlushButtons();
        }
        uint8_t* record = frButtonCursor;
        At<float>(record, 0) = lineX;
        At<float>(record, 4) = lineY;
        At<float>(record, 8) = frScaleX;
        At<float>(record, 0xC) = frScaleY;
        const uint8_t* colour = (frFlags & 1) ? frKeptColour : frPlainColour;
        record[0x10] = colour[0];
        record[0x11] = colour[1];
        record[0x12] = colour[2];
        record[0x13] = colour[3];
        At<float>(record, 0x14) = At<float>(line, 0x10);
        record[0x1A] = Merge(record[0x1A], frFlags, 2);
        At<int16_t>(record, 0x18) = frStyle;
        frButtonCursor = record + 0x1C;
        record[0x1A] = Merge(record[0x1A], frFlags, 1);
        record[0x1A] = Merge(record[0x1A], frFlags, 4);
        record[0x1A] = Merge(record[0x1A], frFlags, 8);
        while (first < last) {
            uint16_t c = *first;
            if (c == '~') {
                uint8_t saved[4];
                std::memcpy(saved, frFontColour, 4);
                uint8_t bold = uint8_t((frFontFlags >> 3) & 1);
                uint16_t token = 0;
                const uint16_t* next = Font_ParseControl(first, frFontColour, &bold, &bold, &token);
                frFontFlags = Merge(frFontFlags, uint8_t(bold << 3), 8);
                if (token != 0) {
                    std::memcpy(frFontColour, saved, 4);
                }
                if (first != next) {
                    uint16_t* out = re::PtrCast<uint16_t*>(frTextCursor);
                    do {
                        if (first >= last) {
                            break;
                        }
                        *out++ = *first++;
                        frTextCursor = reinterpret_cast<uint8_t*>(out);
                    } while (first != next);
                }
            } else {
                *re::PtrCast<uint16_t*>(frButtonCursor) = c;
                frButtonCursor += 2;
                first++;
            }
        }
        *re::PtrCast<uint16_t*>(frButtonCursor) = 0;
        frButtonCursor += 2;
        if (re::PtrCast<uintptr_t>(frButtonCursor) & 3) {
            frButtonCursor += 2;
        }
    }
    return std::fabs(rect[2] - rect[0]);
}

// 0x68BE00 (cdecl): a glyph drawn at (x, y) with the render details (off screen, glyph 0x8C
// (the space) and styles without a texture drawn nothing): a button glyph as its texture,
// centred on the line (its width stored in the style's glyph table), then its binding
// name queued after it; a character as its table's rectangle of the style's texture,
// slanted by 4 scaled units for italic
void __cdecl Font_DrawGlyphImpl(float x, float y, uint32_t code) {
    int32_t lang = frLanguage;
    uint32_t mapped = uint16_t(frStyle);
    if (lang == 7) {
        mapped = uint16_t(frMapJ[frStyle]);
    } else if (lang == 6) {
        mapped = uint16_t(frMapR[frStyle]);
    }
    if (uint16_t(mapped) > 9) {
        return;
    }
    int32_t style = int16_t(mapped);
    void* texture = frTextures[style];
    if (!texture) {
        return;
    }
    uint16_t glyph = uint16_t(code);
    if (glyph == 0x8C) {
        return;
    }
    int16_t rect[4];   // left, top, right, bottom
    uint8_t* entry = nullptr;
    if (lang == 7 && uint16_t(mapped) == 8 && glyph >= 0x8C) {
        uint16_t i = 0;
        for (; i < 0x68F; i++) {
            if (At<uint16_t>(frCodesJ, i * 10) == glyph - 0x6C) {
                break;
            }
        }
        if (i < 0x68F) {
            glyph = uint16_t(i + 0x8C);
            entry = frCodesJ + i * 10 + 2;
        } else {
            glyph = 0x8C;
            entry = reinterpret_cast<uint8_t*>(&GlyphShort(style, 0x8C, -4));
        }
    } else if (lang == 6 && uint16_t(mapped) == 9 && glyph >= 0x8C) {
        uint16_t i = 0;
        for (; i < 0x4E0; i++) {
            if (At<uint16_t>(frCodesR, i * 10) == glyph - 0x6C) {
                break;
            }
        }
        if (i < 0x4E0) {
            glyph = uint16_t(i + 0x8C);
            entry = frCodesR + i * 10 + 2;
        } else {
            glyph = 0x8C;
            entry = reinterpret_cast<uint8_t*>(&GlyphShort(style, 0x8C, -4));
        }
    } else {
        entry = reinterpret_cast<uint8_t*>(&GlyphShort(style, glyph, -4));
    }
    std::memcpy(rect, entry, sizeof(rect));
    int32_t left = rect[0];
    int32_t right = rect[2];
    float drawWidth = float(right - left) * frScaleX;
    float drawHeight = float(int32_t(rect[3]) - int32_t(rect[1])) * frScaleY;
    if (!(x < 640.0f) || !(y < 480.0f) || !(-drawWidth < x) || !(-drawHeight < y)) {
        return;
    }
    if (glyph < 0x8C) {
        float clamped = Font_ClampHeight(frScaleY);
        drawHeight = clamped * float(int32_t(rect[3]) - int32_t(rect[1]));
        float m = uint16_t(mapped) == 1 ? 0.699999988079071f : 1.0f;
        drawHeight = m * drawHeight;
        if (lang == 7 || lang == 6) {
            drawHeight = frButtonScaleJ * drawHeight;
        }
        int32_t index = glyph;
        if (InRange(index, frDoubleFirst, frDoubleLast)) {
            drawHeight = drawHeight * 2.0f;
        }
        if (InRange(index, frWideFirst, frWideLast)) {
            drawHeight = drawHeight * 1.600000023841858f;
        }
        float nameY = y;
        float lineHeight = frLineHeight[style] * frScaleY;
        if (lang == 7) {
            lineHeight = lineHeight * frHeightJ[frStyle];
        } else if (lang == 6) {
            lineHeight = lineHeight * frHeightR[frStyle];
        }
        y = y + (lineHeight - drawHeight) * 0.5f;
        if (frStyle == 0) {
            y = frScaleY * 2.0f + y;
        }
        uint8_t colour[4];
        std::memcpy(colour, (frFlags & 1) ? frColour : frColourAlt, 4);
        colour[3] = frColour[3];
        re::Ptr32<const uint16_t> text = nullptr;
        void* button = Font_GetButtonTexture(frButtonGlyphs[index], &text);
        float width = 0.0f;
        if (button) {
            int32_t w = vc::Call<int32_t>(button, 0x48);
            uint32_t scaled = uint32_t(w * GlyphShort(style, index, 2));
            uint32_t full = vc::Call<uint32_t>(button, 0x4C);
            uint32_t buttonWidth = scaled / full;
            GlyphShort(style, index, 0) = int16_t(buttonWidth);
            float clampedX = Font_ClampWidth(frScaleX);
            width = clampedX * float(int32_t(int16_t(buttonWidth)));
            float m2 = uint16_t(mapped) == 1 ? 0.699999988079071f : 1.0f;
            width = m2 * width;
            if (frLanguage == 7 || frLanguage == 6) {
                width = frButtonScaleJ * width;
            }
            if (InRange(index, frDoubleFirst, frDoubleLast)) {
                width = width + width;
            }
            if (InRange(index, frWideFirst, frWideLast)) {
                width = width * 1.600000023841858f;
            }
            float uv1[2] = {0.0f, 0.0f};
            float uv2[2] = {1.0f, 1.0f};
            DrawRectUV(x, y, x + width, y + drawHeight, button, colour, uv1, uv2);
            width = width + 4.0f;
        }
        if (!text) {
            return;
        }
        uint16_t formatted[512];
        Font_FormatText(formatted, text, 0x200);
        x = (x - frScaleX * 3.0f) + width;
        float textWidth = Font_QueueButtonText(x, nameY, formatted);
        float clampedX = Font_ClampWidth(frScaleX);
        textWidth = (1.0f / clampedX) * textWidth;
        float m3 = uint16_t(mapped) == 1 ? 0.699999988079071f : 1.0f;
        float advance = (1.0f / m3) * textWidth;
        GlyphShort(style, index, 0) = int16_t(int32_t(advance + width));
        return;
    }
    float invWidth = frInvSize[style * 2];
    float invHeight = frInvSize[style * 2 + 1];
    float uv1[2] = {float(left) * invWidth, float(int32_t(rect[1])) * invHeight};
    float uv2[2] = {float(right) * invWidth, float(int32_t(rect[3])) * invHeight};
    if (frFlags & 4) {
        float slant = (lang == 7 || lang == 6) ? 0.0f : 4.0f;
        slant = frScaleX * slant;
        float right2 = x + drawWidth;
        float bottom = y + drawHeight;
        DrawQuadUVB(x + slant, y, right2 + slant, y, x - slant, bottom, right2 - slant, bottom, frTextures[frStyle],
                    frColour, uv1, uv2);
        return;
    }
    void* drawn = (lang == 7 || lang == 6) ? texture : static_cast<void*>(frTextures[frStyle]);
    DrawRectUVB(x, y, x + drawWidth, y + drawHeight, drawn, frColour, uv1, uv2);
}

// 0x68C660 (cdecl): the text buffer drawn (its render states set and put back), then the
// button text buffer
void __cdecl Font_RenderImpl() {
    if (frTextCursor == frTextBuffer) {
        return;
    }
    uint32_t states[7];
    static constexpr int kStates[7] = {8, 6, 0xC, 0xA, 0xB, 0xE, 9};
    for (int i = 0; i < 7; i++) {
        RwRenderStateGet(kStates[i], &states[i]);
    }
    ScaleRenderForLanguage();
    RwRenderStateSet(1, RwRasterFromTexture(frTextures[frStyle]));
    RwRenderStateSet(0xC, 1);
    RwRenderStateSet(9, 2);
    RwRenderStateSet(8, 0);
    RwRenderStateSet(6, 0);
    RwRenderStateSet(0xA, 5);
    RwRenderStateSet(0xB, 6);
    DrawBuffer<false>(frTextCursor, frTextBuffer);
    Font_FlushButtons();
    Im2D_Flush();
    for (int i = 0; i < 7; i++) {
        RwRenderStateSet(kStates[i], states[i]);
    }
}

// 0x68CE00 (cdecl): a line's characters queued into the text buffer with the print
// details (a style change draws what's queued first); a shadow first (offset by the
// shadow's size, or all around it for an outline (flags 0x20)) in the shadow colour.
// The "~" controls are parsed for the colour and bold as they're queued.
void __cdecl Font_QueueLineImpl(float x, float y, const uint16_t* first, const uint16_t* last, float extra) {
    if (frStyle != frFontStyle) {
        Font_Render();
        frStyle = frFontStyle;
    }
    int16_t shadow = frShadow;
    if (double(shadow) != 0.0) {
        uint8_t saved[4];
        std::memcpy(saved, frFontColour, 4);
        std::memcpy(frFontColour, frShadowColour, 4);
        frShadow = 0;
        float size = float(shadow);
        if (frFontFlags & 0x20) {
            frFontFlags |= 4;
            float dy = size * frScreenScaleY;
            float dx = size * frScreenScaleX;
            float up = y - dy;
            float leftX = x - dx;
            Font_QueueLine(leftX, up, first, last, extra);
            Font_QueueLine(leftX, y, first, last, extra);
            Font_QueueLine(x, up, first, last, extra);
            float down = dy + y;
            Font_QueueLine(leftX, down, first, last, extra);
            float rightX = dx + x;
            Font_QueueLine(rightX, up, first, last, extra);
            Font_QueueLine(x, down, first, last, extra);
            Font_QueueLine(rightX, y, first, last, extra);
            Font_QueueLine(rightX, down, first, last, extra);
        } else {
            frFontFlags |= 4;
            Font_QueueLine(x + size * frScreenScaleX, y + size * frScreenScaleY, first, last, extra);
        }
        frFontFlags &= ~4;
        std::memcpy(frFontColour, saved, 4);
        frShadow = shadow;
    }
    int32_t count = int32_t(reinterpret_cast<intptr_t>(last) - reinterpret_cast<intptr_t>(first)) >> 1;
    if (frTextCursor >= frTextBufferEnd - count * 2) {
        Font_Render();
    }
    uint8_t* record = frTextCursor;
    frTextCursor = record + 0x1C;
    At<float>(record, 0) = x;
    At<float>(record, 4) = y;
    At<float>(record, 8) = frFontScaleX;
    At<float>(record, 0xC) = frFontScaleY;
    std::memcpy(record + 0x10, frFontColour, 4);
    At<float>(record, 0x14) = extra;
    record[0x1A] = Merge(record[0x1A], frFontFlags, 2);
    At<int16_t>(record, 0x18) = frFontStyle;
    record[0x1A] = Merge(record[0x1A], uint8_t(frFontFlags >> 2), 1);
    record[0x1A] = Merge(record[0x1A], uint8_t(frFontFlags >> 2), 4);
    record[0x1A] = Merge(record[0x1A], uint8_t(frFontFlags >> 2), 8);
    uint16_t* out = re::PtrCast<uint16_t*>(frTextCursor);
    const uint16_t* p = first;
    while (p < last) {
        uint16_t c = *p;
        if (c == '~') {
            uint8_t saved[4];
            std::memcpy(saved, frFontColour, 4);
            uint8_t bold = uint8_t((frFontFlags >> 3) & 1);
            uint16_t token = 0;
            const uint16_t* next = Font_ParseControl(p, frFontColour, &bold, &bold, &token);
            frFontFlags = Merge(frFontFlags, uint8_t(bold << 3), 8);
            if (token != 0) {
                std::memcpy(frFontColour, saved, 4);
            }
            out = re::PtrCast<uint16_t*>(frTextCursor);
            if (p != next) {
                bool ended = false;
                do {
                    if (p >= last) {
                        ended = true;
                        break;
                    }
                    *out++ = *p++;
                    frTextCursor = reinterpret_cast<uint8_t*>(out);
                } while (p != next);
                if (ended) {
                    break;
                }
            }
        } else {
            *out++ = c;
            frTextCursor = reinterpret_cast<uint8_t*>(out);
            p++;
        }
    }
    *out++ = 0;
    frTextCursor = reinterpret_cast<uint8_t*>(out);
    if (reinterpret_cast<uintptr_t>(out) & 3) {
        frTextCursor = reinterpret_cast<uint8_t*>(out + 1);
    }
}

// 0x68D2F0 (cdecl): a layout's lines queued
void __cdecl Font_QueueLayoutImpl(uint8_t* layout) {
    for (int32_t i = 0; i < At<int32_t>(layout, 0x268); i++) {
        uint8_t* line = layout + 0x10 + i * 0x14;
        Font_QueueLine(At<float>(line, 0), At<float>(line, 4), At<const uint16_t*>(line, 8),
                       At<const uint16_t*>(line, 0xC), At<float>(line, 0x10));
    }
}

// 0x68D340 (cdecl): the laid out text queued, over its background box (flags 1, the
// render colour's rgb set to the print colour's) or the "~B~" dark box
void __cdecl Font_PrintLayoutImpl() {
    float rect[4] = {1000000.0f, -1000000.0f, -1000000.0f, 1000000.0f};
    if (frFontFlags & 1) {
        frColour[0] = frFontColour[0];
        frColour[1] = frFontColour[1];
        frColour[2] = frFontColour[2];
        FontLayout_GetBox(frLayout, rect, At<float>(frLayout, 0), At<float>(frLayout, 4));
        DrawRect(rect, frBackColour);
        Font_QueueLayout(frLayout);
        return;
    }
    if (frBoxNext) {
        FontLayout_GetBox(frLayout, rect, At<float>(frLayout, 0), At<float>(frLayout, 4));
        rect[3] = frScreenScaleY * 4.0f + rect[3];
        uint8_t colour[4];
        DrawRect(rect, RGBA_Construct(colour, 0x10, 0x10, 0x10, 0xA0));
        frBoxNext = 0;
    }
    Font_QueueLayout(frLayout);
}

// 0x68D450 (cdecl): a text printed at (x, y) (without noColour: in red at style 4's
// height), underlined for flags 0x40 (a black line under a print colour one)
void __cdecl Font_PrintStringImpl(float x, float y, const uint16_t* text, int noColour) {
    if (!uint8_t(noColour)) {
        uint8_t red[4];
        uint8_t* colour = RGBA_Construct(red, 0xC8, 0, 0, 0xFF);
        std::memcpy(frFontColour, colour, 4);
        float height = frStyleHeights[4];
        frFontScaleX = height * frScreenScaleX;
        frFontScaleY = height * frScreenScaleY;
    }
    const uint16_t* laid = text;
    if (text && *text == 0xFFFF) {
        HashToText(text + 1, frHashBuffer);
        laid = frHashBuffer;
    }
    Font_LayoutLines(frLayout, x, y, laid, 0);
    Font_PrintLayout();
    if (!(frFontFlags & 0x40)) {
        return;
    }
    float gap = 4.0f;
    if (frLanguage == 7) {
        gap = 1.0f;
    }
    float x1 = x;
    float y1 = (Font_GetLineHeight() + y) + gap;
    float x2 = Font_GetStringWidth(text, 1) + x;
    float y2 = (Font_GetLineHeight() + y) + gap;
    float shadowX1 = x;
    float shadowY1 = y2 + 1.0f;
    float shadowX2 = (Font_GetStringWidth(text, 1) + x) + 1.0f;
    float shadowY2 = ((Font_GetLineHeight() + y) + gap) + 1.0f;
    uint8_t shadowColour[4];
    std::memcpy(shadowColour, frKeptColour, 4);
    float one = 1.0f;
    DrawLine(shadowX1, shadowY1, shadowX2, shadowY2, &one, shadowColour);
    one = 1.0f;
    DrawLine(x1, y1, x2, y2, &one, frFontColour);
}

// 0x68D610 (cdecl): a text printed with a font setup (at its y, or the one given)
void __cdecl Font_PrintMessageImpl(int32_t index, const uint16_t* text, int32_t y) {
    uint8_t* setup = frSetups + index * 0x38;
    FontSetup_Apply(setup);
    float atY = y == 0 ? At<float>(setup, 0x10) : float(y);
    Font_LayoutText(At<float>(setup, 0xC), atY, text);
    Font_PrintLayout();
}

// 0x68D670 (cdecl): a text printed with a custom font setup (HUDGenericText)
void __cdecl Font_PrintAtImpl(int32_t id, const uint16_t* text) {
    if (id < 0 || id >= frCustomCount) {
        return;
    }
    uint8_t* setup = frCustomSetups[id];
    FontSetup_Apply(setup);
    Font_LayoutText(At<float>(setup, 0xC), At<float>(setup, 0x10), text);
    Font_PrintLayout();
}

// the colour with its alpha scaled
inline void ScaleAlpha(uint8_t* out, const uint8_t* colour, float alpha) {
    std::memcpy(out, colour, 4);
    out[3] = uint8_t(int32_t(float(int32_t(colour[3])) * alpha));
}

// 0x68D6C0 (cdecl): the title text (the zone name, its key at 0x1B8A988) while its timer
// runs (not while the screen's faded, then the timer counts down): gold, embossed by
// shaded copies around it, fading in over the last part of the timer's duration, and
// out with the screen's fade
void __cdecl Font_DrawTitleImpl() {
    float timer = frTitleTimer;
    if (timer == 0.0f) {
        return;
    }
    if (frTitleKey[0] == 0) {
        frTitleTimer = 0.0f;
        return;
    }
    if (frUserStart || frFlag20C32EA) {
        return;
    }
    if (0.0f == frFadeAlpha) {
        frTitleTimer = timer - frTitleRate * frTimeStep;
        if (!(0.0f < frTitleTimer)) {
            frTitleTimer = 0.0f;
        }
    }
    FontSetup_Apply(frTitleSetup);
    float alpha = 1.0f;
    float left = frTitleTimer;
    float duration = frTitleDuration;
    if (left < duration) {
        alpha = (duration - left) / duration;
        alpha = 1.0f - alpha * alpha;
    }
    float fade = frFadeAlpha;
    if (frTitleFading) {
        if (!(fade < 1.0f)) {
            alpha = (1.0f - fade / 255.0f) * alpha;
            if (!(0.003000000026077032f < alpha)) {
                frTitleTimer = 0.0f;
            }
        }
    }
    if (fade == 0.0f) {
        frTitleFading = 1;
    }
    float x = At<float>(frTitleSetup, 0xC);
    float y = At<float>(frTitleSetup, 0x10);
    frShadow = 0;
    uint8_t gold[4];
    uint8_t shades[5][4];
    RGBA_Construct(gold, 0xA6, 0x93, 0x3F, 0xD2);
    RGBA_Construct(shades[0], 0x10, 0x10, 0x10, 0xC0);
    RGBA_Construct(shades[1], 0x40, 0x40, 0x40, 0xC0);
    RGBA_Construct(shades[2], 0x80, 0x80, 0x80, 0xC0);
    RGBA_Construct(shades[3], 0xC0, 0xC0, 0xC0, 0x80);
    RGBA_Construct(shades[4], 0xEB, 0xEB, 0xEB, 0x80);
    uint8_t faded[5][4];
    for (int i = 0; i < 5; i++) {
        ScaleAlpha(faded[i], shades[i], alpha);
    }
    const uint16_t* text = Text_Get(frTheText, frTitleKey);
    float wrap = FontSetup_GetWrap(frTitleSetup);
    Font_SetColour(faded[4]);
    float wrapLess = wrap - 1.0f;
    frWrapRight = wrapLess;
    float up = y - 1.0f;
    float leftX = x - 1.0f;
    Font_PrintString(leftX, up, text, 1);
    Font_SetColour(faded[3]);
    frWrapRight = wrapLess;
    Font_PrintString(leftX, y, text, 1);
    frWrapRight = wrap;
    Font_PrintString(x, up, text, 1);
    Font_SetColour(faded[2]);
    frWrapRight = wrapLess;
    float down = y + 1.0f;
    Font_PrintString(leftX, down, text, 1);
    float wrapMore = wrap + 1.0f;
    frWrapRight = wrapMore;
    float rightX = 1.0f + x;
    Font_PrintString(rightX, up, text, 1);
    Font_SetColour(faded[1]);
    frWrapRight = wrapMore;
    Font_PrintString(rightX, y, text, 1);
    frWrapRight = wrap;
    Font_PrintString(x, down, text, 1);
    Font_SetColour(faded[0]);
    frWrapRight = wrapMore;
    Font_PrintString(rightX, down, text, 1);
    uint8_t goldFaded[4];
    ScaleAlpha(goldFaded, gold, alpha);
    Font_SetColour(goldFaded);
    frWrapRight = wrap;
    Font_PrintString(x, y, text, 1);
    Font_Render();
}

#define FONTRENDER_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

FONTRENDER_REVERSED("CFont::RenderGlyphWidth_68B100", 0x0068B100, &Font_RenderGlyphWidthImpl);
FONTRENDER_REVERSED("CFont::FlushButtonText_68B460", 0x0068B460, &Font_FlushButtonsImpl);
FONTRENDER_REVERSED("CFont::QueueButtonText_68BAE0", 0x0068BAE0, &Font_QueueButtonTextImpl);
FONTRENDER_REVERSED("CFont::DrawGlyph_68BE00", 0x0068BE00, &Font_DrawGlyphImpl);
FONTRENDER_REVERSED("CFont::RenderFontBuffer_68C660", 0x0068C660, &Font_RenderImpl);
RE_UNHOOKED("CFont::RenderFontBuffer_Thunk_68CDF0", 0x0068CDF0);   // a lone jmp to 0x68C660
FONTRENDER_REVERSED("CFont::QueueLine_68CE00", 0x0068CE00, &Font_QueueLineImpl);
FONTRENDER_REVERSED("CFont::QueueLayout_68D2F0", 0x0068D2F0, &Font_QueueLayoutImpl);
FONTRENDER_REVERSED("CFont::PrintLayout_68D340", 0x0068D340, &Font_PrintLayoutImpl);
FONTRENDER_REVERSED("CFont::PrintString_68D450", 0x0068D450, &Font_PrintStringImpl);
FONTRENDER_REVERSED("CFont::PrintMessage_68D610", 0x0068D610, &Font_PrintMessageImpl);
FONTRENDER_REVERSED("HUDGenericText::PrintAt_68D670", 0x0068D670, &Font_PrintAtImpl);
FONTRENDER_REVERSED("CFont::DrawTitle_68D6C0", 0x0068D6C0, &Font_DrawTitleImpl);

} // namespace
