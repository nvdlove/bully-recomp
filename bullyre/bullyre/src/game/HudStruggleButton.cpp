// The struggle button (HUD_STRUGGLE_BUTTON): a pad button's icon that pulses when
// pressed, over its shadow, with a label; or the "AVOID_TEXT" line. 0x5572C0..0x557BA2
// (its constructor, SetVisible and Reset are Hud.cpp's).

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Collision.h"
#include "game/Hud.h"
#include "reversed.h"

namespace {

struct CVector2D {
    float x, y;
};

GAME_VAR(int32_t, g_language, 0x00C18724);
GAME_VAR(float, g_screenScaleX, 0x00A957A8);
GAME_VAR(float, g_screenScaleY, 0x00A957AC);
GAME_VAR(float, g_timeStep, 0x00C1A9A4);
GAME_VAR(float, g_frameSeconds, 0x00AB7710);  // (0.02)
GAME_VAR(float, g_iconOffsetY, 0x00CF0A34);   // (-30 for the language with bigger text)
GAME_VAR(uint32_t, g_fontColour, 0x0208B764);
GAME_VAR(uint32_t, g_fontDropColour, 0x0208B780);
GAME_VAR(int16_t, g_fontStyle, 0x0208B786);
inline void* const kTheText = reinterpret_cast<void*>(0x020C2FA4);

GAME_FN(CRGBA*, __thiscall, RGBA_Construct, 0x00510DE0, CRGBA* rgba, int r, int g, int b, int a);
GAME_FN(void*, __cdecl, GetButtonTexture, 0x00514EC0, int button);  // (vf 0x48 width, 0x4C height)
GAME_FN(void, __cdecl, DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture, const CRGBA* colour,
        const CVector2D* uv1, const CVector2D* uv2);
GAME_FN(CVector2D*, __cdecl, ScreenPosition, 0x0052D4F0, CVector2D* out, float x, float y, int hAlign, int vAlign);
GAME_FN(col::CRect*, __cdecl, ScreenRect, 0x0052D650, col::CRect* out, float x, float y, float width, float height,
        int hAlign, int vAlign);
GAME_FN(void, __thiscall, HUDComponent_SetAlignment, 0x0053CE00, HUDComponent* self, const char* h, const char* v);
GAME_FN(const uint16_t*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __cdecl, Font_Begin, 0x00688E60);
GAME_FN(void, __cdecl, Font_SetSizePreset, 0x00688050, int preset);
GAME_FN(void, __cdecl, Font_SetJustify, 0x00688160, int justify);
GAME_FN(void, __cdecl, Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __cdecl, Font_Render, 0x0068CDF0);

// a + (b - a) * t per channel, truncated, in bytes
CRGBA Blend(CRGBA a, CRGBA b, float t) {
    CRGBA out;
    out.r = uint8_t(a.r + uint8_t(int32_t(double(t) * double(int32_t(b.r) - int32_t(a.r)))));
    out.g = uint8_t(a.g + uint8_t(int32_t(double(int32_t(b.g) - int32_t(a.g)) * t)));
    out.b = uint8_t(a.b + uint8_t(int32_t(double(int32_t(b.b) - int32_t(a.b)) * t)));
    out.a = uint8_t(a.a + uint8_t(int32_t(double(t) * double(int32_t(b.a) - int32_t(a.a)))));
    return out;
}

uint32_t TextureSize(void* texture, int slot) {
    return reinterpret_cast<uint32_t(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(texture))[slot / 4]))(texture);
}

} // namespace

namespace hudstruggle {

// 0x5572C0 (vf8 Draw): mode 0 the button (6..9: its texture) over its shadow, both
// swelling for a moment when pressed (+0x161 starts it, +0x162 lets it flash); mode 1
// the avoid text; the label (+0x160)
void __fastcall Draw(HUDStruggleButton* self, re::Edx) {
    if (g_language == 7) {
        g_iconOffsetY = -30.0f;
    }
    bool drawn = false;
    if (self->unknown15C == 0) {
        void* texture = nullptr;  // (other buttons: the game reads an uninitialised stack slot)
        switch (self->button) {
        case 6:
            texture = GetButtonTexture(6);
            break;
        case 7:
            texture = GetButtonTexture(7);
            break;
        case 8:
            texture = GetButtonTexture(8);
            break;
        case 9:
            texture = GetButtonTexture(9);
            break;
        default:
            break;
        }
        float sx = g_screenScaleX;
        float sy = g_screenScaleY;
        CRGBA shadowFrom, shadowTo, iconFrom, iconTo;
        RGBA_Construct(&shadowFrom, 0x10, 0x10, 0x10, 0x5A);
        RGBA_Construct(&shadowTo, 0x10, 0x10, 0x10, 200);
        RGBA_Construct(&iconFrom, 0xC0, 0xC0, 0xC0, 100);
        RGBA_Construct(&iconTo, 0xEB, 0xEB, 0xEB, 0xFF);
        if (self->unknown161 && !(0.0f < self->unknown164)) {
            self->unknown164 = 0.3f;
        }
        float flash = 0.0f;
        if (0.0f < self->unknown164 && self->unknown162) {
            float u = float(1.0 - double(self->unknown164) / 0.30000001192092896);
            if (0.20000000298023224 > double(u)) {
                float v = float(double(u) / 0.20000000298023224);
                flash = float(double(v) * v);
            } else {
                float w = float((double(u) - 0.20000000298023224) / 0.7999999970197678);
                flash = float(1.0 - double(w) * w);
            }
            float step = float(double(g_frameSeconds) * g_timeStep);
            float left = float(double(self->unknown164) - step);
            self->unknown164 = left;
            if (left <= 0.0f) {
                self->unknown164 = 0.0f;
            }
        }
        float scale = float(double(flash) * 0.20000004768371582 + 1.0);
        if (!self->unknown162) {
            flash = 1.0f;
        }
        uint32_t width = TextureSize(texture, 0x48);
        double wide = double(width) * 32.0;
        uint32_t height = TextureSize(texture, 0x4C);
        float aspect = float(wide / double(height));
        drawn = true;
        CRGBA colour = Blend(shadowFrom, shadowTo, flash);
        const col::CRect& r = self->rect;
        float halfW = std::fabs(float(r.right - r.left));
        float cx = float(double(halfW) * 0.5 + r.left);
        float halfH = std::fabs(float(r.bottom - r.top));
        float cy = float(double(halfH) * 0.5 + r.top + g_iconOffsetY - 5.0);
        CVector2D uv2 = {1.0f, 1.0f};
        CVector2D uv1 = {0.0f, 0.0f};
        double hh = double(scale) * 17.600000381469727 * sy;
        double halfAspect = double(aspect) * 0.5;
        double hw = halfAspect * 1.100000023841858 * scale * sx;
        DrawRectUV(float(cx - hw), float(cy - hh), float(cx + hw), float(cy + hh), texture, &colour, &uv1, &uv2);
        colour = Blend(iconFrom, iconTo, flash);
        uv1 = {0.0f, 0.0f};
        hh = double(scale) * 16.0 * sy;
        hw = double(scale) * halfAspect * sx;
        DrawRectUV(float(cx - hw), float(cy - hh), float(cx + hw), float(cy + hh), texture, &colour, &uv1, &uv2);
    } else if (self->unknown15C == 1) {
        CRGBA colour;
        RGBA_Construct(&colour, 200, 200, 100, 200);
        Font_Begin();
        g_fontStyle = 3;
        Font_SetSizePreset(3);
        std::memcpy(&g_fontColour, &colour, 4);
        const uint16_t* text = Text_Get(kTheText, "AVOID_TEXT");
        float y = self->rect.top;
        float x = float((double(self->rect.right) + self->rect.left) * 0.5);
        Font_PrintString(x, y, text, 1);
        Font_Render();
    }
    if (self->unknown160) {
        Font_Begin();
        CRGBA drop;
        RGBA_Construct(&drop, 0, 0, 0, 0x80);
        std::memcpy(&g_fontDropColour, &drop, 4);
        Font_SetJustify(2);
        // (the label's position at +0x144, in the name's space: Hud.h's name is 0x100)
        double y = double(*reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(self) + 0x148));
        if (drawn) {
            y = y + g_iconOffsetY;
        }
        const uint16_t* text = Text_Get(kTheText, self->name);
        Font_PrintString(*reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(self) + 0x144), float(y), text, 1);
    }
}

// 0x5579E0 (vf1 Setup): the button's rect, its shadow's, the label's position and
// the avoid text's rect
void __fastcall Setup(HUDStruggleButton* self, re::Edx, const HUDLayoutRecord* record) {
    const int16_t* d = re::PtrCast<const int16_t*>(record->data);
    HUDComponent_SetAlignment(self, reinterpret_cast<const char*>(d + 0xE), reinterpret_cast<const char*>(d + 0x12));
    col::CRect rect;
    self->rect = *ScreenRect(&rect, float(d[0]), float(d[1]), float(d[2]), float(d[3]), self->hAlign, self->vAlign);
    self->rect2 = *ScreenRect(&rect, float(d[4]), float(d[5]), float(d[6]), float(d[7]), self->hAlign, self->vAlign);
    CVector2D point;
    *reinterpret_cast<CVector2D*>(reinterpret_cast<uint8_t*>(self) + 0x144) =
        *ScreenPosition(&point, float(d[8]), float(d[9]), self->hAlign, self->vAlign);
    *reinterpret_cast<col::CRect*>(reinterpret_cast<uint8_t*>(self) + 0x14C) =
        *ScreenRect(&rect, float(d[10]), float(d[11]), float(d[12]), float(d[13]), self->hAlign, self->vAlign);
}

} // namespace hudstruggle

#define HUDSTRUGGLE_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(hudstruggle_original_, __LINE__);                                       \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(hudstruggle_original_, __LINE__), \
             re::Mode::Original)

HUDSTRUGGLE_REVERSED("HUDStruggleButton::Draw", 0x005572C0, &hudstruggle::Draw);
HUDSTRUGGLE_REVERSED("HUDStruggleButton::Setup", 0x005579E0, &hudstruggle::Setup);
