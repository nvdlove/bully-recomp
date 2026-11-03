// CSprite2d (0x52C230..0x52D787): a texture drawn as 2D quads and rectangles (one or
// four corner colors, texture rectangles), lines, circles, pies and rings, and the
// 640x480 layout helpers (screen alignment 0 left/top, 1 centre, 2 right/bottom).
//
// Found here: 0x739820 and 0x739F50 draw four corners (two ways, the same arguments),
// 0x7395B0 a rectangle with the colour at 0x20DCD14; 0x738C00 sets the texture;
// 0x738C30 / 0x738C60 start a fan / strip of n vertices, 0x738CC0 a vertex, 0x738D80
// ends it, 0x73A300 draws. Semi-transparent colours switch alpha blending on (render
// state 0xC); some draws need all four corners' alpha below 0xFF, one needs any.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "reversed.h"

namespace sprite2d17 {

using col::CRect;

struct CSprite2d {
    re::Ptr32<void> texture;   // +0
};
static_assert(sizeof(CSprite2d) == 4);

struct CRGBA {
    uint8_t r, g, b, a;
};
static_assert(sizeof(CRGBA) == 4);

struct CVector2D {
    float x, y;
};

GAME_ARRAY(uint32_t, cornerColors, 0x020DCCF4);
GAME_VAR(uint32_t, quadColor, 0x020DCD14);
GAME_VAR(uint32_t, fanColor, 0x020DCCD0);
GAME_VAR(void*, currentRaster, 0x020DCCC8);
GAME_VAR(float, screenScaleX, 0x00A957A8);
GAME_VAR(float, screenScaleY, 0x00A957AC);
GAME_VAR(float, twoPi, 0x00A957CC);
GAME_VAR(float, halfPi, 0x00A957C8);

GAME_FN(void, __cdecl, Texture_Destroy, 0x005F0F90, void* texture);
GAME_FN(void*, __cdecl, Txd_FindTexture, 0x005F1770, const char* name, const char* mask);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(void, __cdecl, GetRenderState, 0x00519290, int state, int* value);
GAME_FN(uint8_t*, __cdecl, GetRaster, 0x0051A120, void* texture);
GAME_FN(void, __cdecl, Im2D_SetTexture, 0x00738C00, void* texture);
GAME_FN(void, __cdecl, Im2D_BeginFan, 0x00738C30, int count);
GAME_FN(void, __cdecl, Im2D_BeginStrip, 0x00738C60, int count);
GAME_FN(void, __cdecl, Im2D_Vertex, 0x00738CC0, float x, float y, uint32_t z);   // (z: an unsigned integer)
GAME_FN(void, __cdecl, Im2D_EndVertex, 0x00738D80);
GAME_FN(void, __cdecl, Im2D_Draw, 0x0073A300);
GAME_FN(void, __cdecl, SetClipRect, 0x0043A6D0, float x1, float y1, float x2, float y2);
GAME_FN(void, __cdecl, SetInsetRects, 0x0052AC90, float a0, float a1, float a2, float a3, float b0, float b1, float b2, float b3);
GAME_FN(void, __cdecl, DrawRectRaw, 0x007395B0, float x1, float y1, float z1, float x2, float y2, float z2);
GAME_FN(void, __cdecl, DrawQuad, 0x00739820, float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2,
        float x3, float y3, float z3);
GAME_FN(void, __cdecl, DrawQuadB, 0x00739F50, float x0, float y0, uint32_t z0, float x1, float y1, uint32_t z1, float x2, float y2,
        uint32_t z2, float x3, float y3, uint32_t z3);   // (the z's unsigned integers)
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, float* v);
GAME_FN(void, __cdecl, Draw2DPolygon_Game, 0x0052C270, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4,
        const CRGBA* color);
GAME_FN(void, __cdecl, DrawRectUV_Game, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture, const CRGBA* color,
        const CVector2D* uv1, const CVector2D* uv2);

static uint32_t Word(const CRGBA* c) {
    return *reinterpret_cast<const uint32_t*>(c);
}
static void SetCorners(uint32_t c0, uint32_t c1, uint32_t c2, uint32_t c3) {
    cornerColors[0] = c0;
    cornerColors[1] = c1;
    cornerColors[2] = c2;
    cornerColors[3] = c3;
}
static void RasterSize(void* texture, float* w, float* h) {
    uint8_t* raster = GetRaster(texture);
    *w = float(*reinterpret_cast<int32_t*>(raster + 0xC));
    *h = float(*reinterpret_cast<int32_t*>(raster + 0x10));
}
// the texture's rectangle uv1..uv2 (in its pixels)
static void SetUVRect(void* texture, const CVector2D* uv1, const CVector2D* uv2) {
    uint8_t* raster = GetRaster(texture);
    float fw = float(*reinterpret_cast<int32_t*>(raster + 0xC));
    float fh = float(*reinterpret_cast<int32_t*>(raster + 0x10));
    float v2 = fh * uv2->y;
    float u2 = fw * uv2->x;
    float u1 = fw * uv1->x;
    float v1 = fh * uv1->y;
    SetInsetRects(u1, v1, u2, v1, u1, v2, u2, v2);
}

// 0x52C230 (thiscall): CSprite2d::SetTexture (the old one destroyed; by name)
void __fastcall SetTexture(CSprite2d* self, re::Edx, const char* name) {
    if (self->texture) {
        Texture_Destroy(self->texture);
        self->texture = nullptr;
    }
    if (name) {
        self->texture = Txd_FindTexture(name, nullptr);
    }
}

// 0x52C270: CSprite2d::Draw2DPolygon (untextured, one colour)
void __cdecl Draw2DPolygon(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, const CRGBA* color) {
    Im2D_SetTexture(nullptr);
    SetCorners(Word(color), Word(color), Word(color), Word(color));
    if (color->a != 0xFF) {
        SetRenderState(0xC, 1);
    }
    DrawQuad(x1, y1, 0.0f, x2, y2, 0.0f, x3, y3, 0.0f, x4, y4, 0.0f);
}

// 0x52C310: the same through 0x739F50
void __cdecl Draw2DPolygonB(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, const CRGBA* color) {
    Im2D_SetTexture(nullptr);
    SetCorners(Word(color), Word(color), Word(color), Word(color));
    if (color->a != 0xFF) {
        SetRenderState(0xC, 1);
    }
    DrawQuadB(x1, y1, 0, x2, y2, 0, x3, y3, 0, x4, y4, 0);
}

// 0x52C3B0: with a colour per corner (blending when none is opaque), through 0x739F50
void __cdecl Draw2DPolygonB4(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, const CRGBA* c1,
                             const CRGBA* c2, const CRGBA* c3, const CRGBA* c4) {
    Im2D_SetTexture(nullptr);
    if (c1->a != 0xFF && c2->a != 0xFF && c3->a != 0xFF && c4->a != 0xFF) {
        SetRenderState(0xC, 1);
    }
    SetCorners(Word(c1), Word(c2), Word(c3), Word(c4));
    DrawQuadB(x1, y1, 0, x2, y2, 0, x3, y3, 0, x4, y4, 0);
}

// 0x52C470: CSprite2d::DrawRect (untextured; no depth test or write while drawing)
void __cdecl DrawRect(const CRect* rect, const CRGBA* color) {
    int zWrite, zTest, alphaEnable;
    GetRenderState(8, &zWrite);
    GetRenderState(6, &zTest);
    GetRenderState(0xC, &alphaEnable);
    SetRenderState(6, 0);
    SetRenderState(8, 0);
    if (color->a != 0xFF) {
        SetRenderState(0xC, 1);
    }
    Im2D_SetTexture(nullptr);
    quadColor = Word(color);
    DrawRectRaw(rect->left, rect->bottom, 0.0f, rect->right, rect->top, 0.0f);
    SetRenderState(8, zWrite);
    SetRenderState(6, zTest);
    SetRenderState(0xC, alphaEnable);
}

// 0x52C530: CSprite2d::DrawRect with a colour per corner
void __cdecl DrawRect4(const CRect* rect, const CRGBA* c1, const CRGBA* c2, const CRGBA* c3, const CRGBA* c4) {
    Im2D_SetTexture(nullptr);
    SetCorners(Word(c1), Word(c2), Word(c3), Word(c4));
    DrawQuad(rect->left, rect->bottom, 0.0f, rect->right, rect->bottom, 0.0f, rect->left, rect->top, 0.0f, rect->right, rect->top, 0.0f);
}

// 0x52C5D0: Draw2DPolygon with a colour per corner
void __cdecl Draw2DPolygon4(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, const CRGBA* c1,
                            const CRGBA* c2, const CRGBA* c3, const CRGBA* c4) {
    Im2D_SetTexture(nullptr);
    if (c1->a != 0xFF && c2->a != 0xFF && c3->a != 0xFF && c4->a != 0xFF) {
        SetRenderState(0xC, 1);
    }
    SetCorners(Word(c1), Word(c2), Word(c3), Word(c4));
    DrawQuad(x1, y1, 0.0f, x2, y2, 0.0f, x3, y3, 0.0f, x4, y4, 0.0f);
}

// 0x52C690 / 0x52C6B0: the screen x / y of an alignment
float __cdecl AlignX(int align) {
    if (align == 1) {
        return 320.0f;
    }
    if (align == 2) {
        return 639.0f;
    }
    return 0.0f;
}
float __cdecl AlignY(int align) {
    if (align == 1) {
        return 240.0f;
    }
    if (align == 2) {
        return 479.0f;
    }
    return 0.0f;
}

// 0x52C6D0 (thiscall): CSprite2d::Delete
void __fastcall Delete(CSprite2d* self, re::Edx) {
    if (self->texture) {
        Texture_Destroy(self->texture);
        self->texture = nullptr;
    }
}

// 0x52C6F0 (thiscall): CSprite2d::Draw (a rectangle)
void __fastcall Draw(CSprite2d* self, re::Edx, const CRect* rect, const CRGBA* color) {
    Im2D_SetTexture(self->texture);
    quadColor = Word(color);
    if (self->texture) {
        float w, h;
        RasterSize(self->texture, &w, &h);
        SetClipRect(0.0f, 0.0f, w, h);
    }
    DrawRectRaw(rect->left, rect->bottom, 0.0f, rect->right, rect->top, 0.0f);
}

// 0x52C770 (thiscall, a third argument unused): drawn only when its texture has a raster
void __fastcall DrawIfLoaded(CSprite2d* self, re::Edx, const CRect* rect, const CRGBA* color, int unused) {
    (void)unused;
    uint8_t* raster = GetRaster(self->texture);
    if (!raster) {
        return;
    }
    quadColor = Word(color);
    currentRaster = raster;
    SetClipRect(0.0f, 0.0f, float(*reinterpret_cast<int32_t*>(raster + 0xC)), float(*reinterpret_cast<int32_t*>(raster + 0x10)));
    DrawRectRaw(rect->left, rect->bottom, 0.0f, rect->right, rect->top, 0.0f);
}

// 0x52C7F0 (thiscall): CSprite2d::Draw (four corners, the whole texture)
void __fastcall DrawCorners(CSprite2d* self, re::Edx, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4,
                            const CRGBA* color) {
    Im2D_SetTexture(self->texture);
    if (self->texture) {
        float w, h;
        RasterSize(self->texture, &w, &h);
        SetInsetRects(0.0f, 0.0f, w, 0.0f, 0.0f, h, w, h);
    }
    SetCorners(Word(color), Word(color), Word(color), Word(color));
    DrawQuad(x1, y1, 0.0f, x2, y2, 0.0f, x3, y3, 0.0f, x4, y4, 0.0f);
}

// 0x52C8F0: a textured rectangle (x1, y1)..(x2, y2) showing the texture's uv1..uv2
void __cdecl DrawRectUV(float x1, float y1, float x2, float y2, void* texture, const CRGBA* color, const CVector2D* uv1,
                        const CVector2D* uv2) {
    Im2D_SetTexture(texture);
    if (texture) {
        SetUVRect(texture, uv1, uv2);
    }
    if (color->a != 0xFF) {
        SetRenderState(0xC, 1);
    }
    SetCorners(Word(color), Word(color), Word(color), Word(color));
    DrawQuad(x1, y1, 0.0f, x2, y1, 0.0f, x1, y2, 0.0f, x2, y2, 0.0f);
}

// 0x52CA30: the same through 0x739F50
void __cdecl DrawRectUVB(float x1, float y1, float x2, float y2, void* texture, const CRGBA* color, const CVector2D* uv1,
                         const CVector2D* uv2) {
    Im2D_SetTexture(texture);
    if (texture) {
        SetUVRect(texture, uv1, uv2);
    }
    if (color->a != 0xFF) {
        SetRenderState(0xC, 1);
    }
    SetCorners(Word(color), Word(color), Word(color), Word(color));
    DrawQuadB(x1, y1, 0, x2, y1, 0, x1, y2, 0, x2, y2, 0);
}

// 0x52CB60: with a colour per corner (blending when any isn't opaque)
void __cdecl DrawRectUV4(float x1, float y1, float x2, float y2, void* texture, const CRGBA* c1, const CRGBA* c2, const CRGBA* c3,
                         const CRGBA* c4, const CVector2D* uv1, const CVector2D* uv2) {
    Im2D_SetTexture(texture);
    if (texture) {
        SetUVRect(texture, uv1, uv2);
    }
    if (c1->a != 0xFF || c2->a != 0xFF || c3->a != 0xFF || c4->a != 0xFF) {
        SetRenderState(0xC, 1);
    }
    SetCorners(Word(c1), Word(c2), Word(c3), Word(c4));
    DrawQuad(x1, y1, 0.0f, x2, y1, 0.0f, x1, y2, 0.0f, x2, y2, 0.0f);
}

// 0x52CCC0: a textured quad (four corners) showing uv1..uv2
void __cdecl DrawQuadUV(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, void* texture, const CRGBA* color,
                        const CVector2D* uv1, const CVector2D* uv2) {
    Im2D_SetTexture(texture);
    if (texture) {
        SetUVRect(texture, uv1, uv2);
    }
    if (color->a != 0xFF) {
        SetRenderState(0xC, 1);
    }
    SetCorners(Word(color), Word(color), Word(color), Word(color));
    DrawQuad(x1, y1, 0.0f, x2, y2, 0.0f, x3, y3, 0.0f, x4, y4, 0.0f);
}

// 0x52CE00: the same through 0x739F50
void __cdecl DrawQuadUVB(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, void* texture, const CRGBA* color,
                         const CVector2D* uv1, const CVector2D* uv2) {
    Im2D_SetTexture(texture);
    if (texture) {
        SetUVRect(texture, uv1, uv2);
    }
    if (color->a != 0xFF) {
        SetRenderState(0xC, 1);
    }
    SetCorners(Word(color), Word(color), Word(color), Word(color));
    DrawQuadB(x1, y1, 0, x2, y2, 0, x3, y3, 0, x4, y4, 0);
}

// 0x52CF40: a line (x1, y1)..(x2, y2), *width to each side
void __cdecl DrawLine(float x1, float y1, float x2, float y2, const float* width, const CRGBA* color) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float c = static_cast<float>(std::cos(1.5707963705062866));
    float s = static_cast<float>(std::sin(1.5707963705062866));
    float n[3] = {dx * c - dy * s, c * dy + s * dx, 0.0f};
    Vector_Normalise(n);
    float w = *width;
    float nx = w * n[0];
    float ny = w * n[1];
    Draw2DPolygon_Game(x1 + nx, ny + y1, x1 - nx, y1 - ny, x2 + nx, ny + y2, x2 - nx, y2 - ny, color);
}

// 0x52D090: a filled circle (a fan of `segments`)
void __cdecl DrawCircle(const CVector2D* center, float radius, const CRGBA* color, int segments) {
    float step = twoPi / float(segments);
    fanColor = Word(color);
    Im2D_SetTexture(nullptr);
    if (color->a != 0xFF) {
        SetRenderState(0xC, 1);
    }
    Im2D_BeginFan(segments + 2);
    Im2D_Vertex(center->x, center->y, 0);
    Im2D_EndVertex();
    float kx = screenScaleX;
    float ky = screenScaleY;
    float s = static_cast<float>(std::sin(0.0));
    float y = s * radius * ky + center->y;
    float c = static_cast<float>(std::cos(0.0));
    float x = c * radius * kx + center->x;
    Im2D_Vertex(x, y, 0);
    Im2D_EndVertex();
    float angle = step + 0.0f;
    for (int i = segments; i > 0; i--) {
        s = static_cast<float>(std::sin(static_cast<double>(angle)));
        y = s * radius * ky + center->y;
        c = static_cast<float>(std::cos(static_cast<double>(angle)));
        x = c * radius * kx + center->x;
        Im2D_Vertex(x, y, 0);
        Im2D_EndVertex();
        angle = angle + step;
    }
    Im2D_Draw();
}

// 0x52D200: a pie from `start` to `end` (radians, 0 up)
void __cdecl DrawPie(const CVector2D* center, float radius, const CRGBA* color, int segments, float start, float end) {
    float from = start - halfPi;
    float span = (end - halfPi) - from;
    fanColor = Word(color);
    float step = span / float(segments);
    Im2D_SetTexture(nullptr);
    if (color->a != 0xFF) {
        SetRenderState(0xC, 1);
    }
    float kx = screenScaleX;
    float ky = screenScaleY;
    Im2D_BeginFan(segments + 2);
    Im2D_Vertex(center->x, center->y, 0);
    Im2D_EndVertex();
    float s = static_cast<float>(std::sin(static_cast<double>(from)));
    float y = s * radius * ky + center->y;
    float c = static_cast<float>(std::cos(static_cast<double>(from)));
    float x = c * radius * kx + center->x;
    Im2D_Vertex(x, y, 0);
    Im2D_EndVertex();
    float angle = step + from;
    for (int i = segments; i > 0; i--) {
        s = static_cast<float>(std::sin(static_cast<double>(angle)));
        y = s * radius * ky + center->y;
        c = static_cast<float>(std::cos(static_cast<double>(angle)));
        x = c * radius * kx + center->x;
        Im2D_Vertex(x, y, 0);
        Im2D_EndVertex();
        angle = step + angle;
    }
    Im2D_Draw();
}

// 0x52D390: a ring between radii outer and inner from `start` to `end` (a strip)
void __cdecl DrawRing(const CVector2D* center, float outer, float inner, const CRGBA* color, int segments, float start, float end) {
    float from = start - halfPi;
    float span = (end - halfPi) - from;
    fanColor = Word(color);
    float step = span / float(segments);
    Im2D_SetTexture(nullptr);
    if (color->a != 0xFF) {
        SetRenderState(0xC, 1);
    }
    float kx = screenScaleX;
    float ky = screenScaleY;
    Im2D_BeginStrip(segments * 2 + 2);
    float angle = from;
    for (int i = segments + 1; i > 0; i--) {
        float c = static_cast<float>(std::cos(static_cast<double>(angle)));
        float xIn = c * inner * kx + center->x;
        float s = static_cast<float>(std::sin(static_cast<double>(angle)));
        float yIn = s * inner * ky + center->y;
        float yOut = s * outer * ky + center->y;
        float xOut = outer * c * kx + center->x;
        Im2D_Vertex(xOut, yOut, 0);
        Im2D_EndVertex();
        Im2D_Vertex(xIn, yIn, 0);
        Im2D_EndVertex();
        angle = step + angle;
    }
    Im2D_Draw();
}

// 0x52D4F0: a position on the 640x480 screen (scaled, from an alignment)
CVector2D* __cdecl AlignedPosScaled(CVector2D* out, float x, float y, int alignX, int alignY) {
    x = screenScaleX * x;
    y = screenScaleY * y;
    out->x = AlignX(alignX) + x;
    out->y = AlignY(alignY) + y;
    return out;
}

// 0x52D590: the same unscaled
CVector2D* __cdecl AlignedPos(CVector2D* out, float x, float y, int alignX, int alignY) {
    out->x = AlignX(alignX) + x;
    out->y = AlignY(alignY) + y;
    return out;
}

// 0x52D610: DrawRectUV for a rectangle
void __cdecl DrawRectUVRect(const CRect* rect, void* texture, const CRGBA* color, const CVector2D* uv1, const CVector2D* uv2) {
    DrawRectUV_Game(rect->left, rect->bottom, rect->right, rect->top, texture, color, uv1, uv2);
}

// 0x52D650: a rectangle at (x, y) of size w, h placed by alignment (its own and the
// screen's: 0 from the left / top, 1 centred, 2 from the right / bottom)
CRect* __cdecl MakeAlignedRect(CRect* out, float x, float y, float w, float h, int alignX, int alignY) {
    float baseX = AlignX(alignX);
    float baseY = AlignY(alignY);
    out->left = 1e6f;
    out->bottom = 1e6f;
    out->right = -1e6f;
    out->top = -1e6f;
    float kx = screenScaleX;
    x = kx * x;
    w = kx * w;
    h = screenScaleY * h;
    if (alignX == 0) {
        out->left = baseX + x;
    } else if (alignX == 1) {
        out->left = (baseX + x) - w * 0.5f;
    } else if (alignX == 2) {
        out->left = (baseX + x) - w;
    }
    out->right = out->left + w;
    if (alignY == 0) {
        out->top = baseY + y;
    } else if (alignY == 1) {
        out->top = (baseY + y) - h * 0.5f;
    } else if (alignY == 2) {
        out->top = (baseY + y) - h;
    }
    out->bottom = h + out->top;
    return out;
}

} // namespace sprite2d17

// ---- registration ----------------------------------------------------------------------

#define SPRITE2D17_REVERSED(name, address, fn)                                               \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace sprite2d17 {
SPRITE2D17_REVERSED("CSprite2d::SetTexture", 0x0052C230, &SetTexture);
SPRITE2D17_REVERSED("CSprite2d::Draw2DPolygon", 0x0052C270, &Draw2DPolygon);
SPRITE2D17_REVERSED("CSprite2d::Draw2DPolygonB", 0x0052C310, &Draw2DPolygonB);
SPRITE2D17_REVERSED("CSprite2d::Draw2DPolygonB (4 colours)", 0x0052C3B0, &Draw2DPolygonB4);
SPRITE2D17_REVERSED("CSprite2d::DrawRect", 0x0052C470, &DrawRect);
SPRITE2D17_REVERSED("CSprite2d::DrawRect (4 colours)", 0x0052C530, &DrawRect4);
SPRITE2D17_REVERSED("CSprite2d::Draw2DPolygon (4 colours)", 0x0052C5D0, &Draw2DPolygon4);
SPRITE2D17_REVERSED("CSprite2d::AlignX", 0x0052C690, &AlignX);
SPRITE2D17_REVERSED("CSprite2d::AlignY", 0x0052C6B0, &AlignY);
SPRITE2D17_REVERSED("CSprite2d::Delete", 0x0052C6D0, &Delete);
SPRITE2D17_REVERSED("CSprite2d::Draw (rect)", 0x0052C6F0, &Draw);
SPRITE2D17_REVERSED("CSprite2d::DrawIfLoaded", 0x0052C770, &DrawIfLoaded);
SPRITE2D17_REVERSED("CSprite2d::Draw (corners)", 0x0052C7F0, &DrawCorners);
SPRITE2D17_REVERSED("CSprite2d::DrawRectUV", 0x0052C8F0, &DrawRectUV);
SPRITE2D17_REVERSED("CSprite2d::DrawRectUVB", 0x0052CA30, &DrawRectUVB);
SPRITE2D17_REVERSED("CSprite2d::DrawRectUV (4 colours)", 0x0052CB60, &DrawRectUV4);
SPRITE2D17_REVERSED("CSprite2d::DrawQuadUV", 0x0052CCC0, &DrawQuadUV);
SPRITE2D17_REVERSED("CSprite2d::DrawQuadUVB", 0x0052CE00, &DrawQuadUVB);
SPRITE2D17_REVERSED("CSprite2d::DrawLine", 0x0052CF40, &DrawLine);
SPRITE2D17_REVERSED("CSprite2d::DrawCircle", 0x0052D090, &DrawCircle);
SPRITE2D17_REVERSED("CSprite2d::DrawPie", 0x0052D200, &DrawPie);
SPRITE2D17_REVERSED("CSprite2d::DrawRing", 0x0052D390, &DrawRing);
SPRITE2D17_REVERSED("CSprite2d::AlignedPosScaled", 0x0052D4F0, &AlignedPosScaled);
SPRITE2D17_REVERSED("CSprite2d::AlignedPos", 0x0052D590, &AlignedPos);
SPRITE2D17_REVERSED("CSprite2d::DrawRectUV (rect)", 0x0052D610, &DrawRectUVRect);
SPRITE2D17_REVERSED("CSprite2d::MakeAlignedRect", 0x0052D650, &MakeAlignedRect);
} // namespace sprite2d17
