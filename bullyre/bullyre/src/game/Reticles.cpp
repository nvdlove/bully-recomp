// The aiming reticles (0xC3B708: three of 0x40 bytes): the crosshair (0; a texture of
// the HUD's crosshair set, in screen space or projected from a world position) and two
// markers (1, 2: billboards facing a direction, spinning). Their textures come from the
// "HUDElems" dictionary: up to 10 crosshairs named by the data tree (each with its
// "_x" mask), and "HUD_xhair" for the markers. Next to them the bone overlay's state
// (0xC3B6F8..0xC3B700, see BoneOverlayShapes.cpp).

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "reversed.h"

namespace c12reticle {

struct CVector {
    float x, y, z;
};

struct CReticle {
    uint8_t active;        // +0
    uint8_t unknown01[3];
    CVector pos;           // +4 (screen x, y in screen space)
    CVector dir;           // +0x10 the billboard's facing
    int32_t screenSpace;   // +0x1C
    int32_t unscaled;      // +0x20 (the projection's scale ignored)
    int32_t billboard;     // +0x24 (a 3D billboard)
    uint8_t color[4];      // +0x28 r g b a
    uint8_t brightness;    // +0x2C (r g b times it / 256)
    uint8_t unknown2D[3];
    float size;            // +0x30
    float angle;           // +0x34
    float spin;            // +0x38 added to the angle each render
    re::Ptr32<void> texture;         // +0x3C
};
static_assert(offsetof(CReticle, screenSpace) == 0x1C);
static_assert(offsetof(CReticle, color) == 0x28);
static_assert(offsetof(CReticle, texture) == 0x3C);
static_assert(sizeof(CReticle) == 0x40);

// RwIm3DVertex (the Im3D batch's)
struct Im3DVertex {
    CVector pos;       // +0
    CVector normal;    // +0xC
    uint32_t color;    // +0x18 (ARGB)
    float u, v;        // +0x1C
};
static_assert(sizeof(Im3DVertex) == 0x24);

GAME_ARRAY(CReticle, reticles, 0x00C3B708);
GAME_ARRAY(void*, crosshairTextures, 0x00C3B6D0);   // [10]
GAME_VAR(int32_t, currentCrosshair, 0x00C3B704);
GAME_VAR(uint8_t, overlayOn, 0x00C3B700);
GAME_VAR(int32_t, overlayShape, 0x00C3B6FC);
GAME_VAR(int32_t, overlayBone, 0x00C3B6F8);
GAME_VAR(float, screenScaleX, 0x00A957A8);
GAME_VAR(float, screenScaleY, 0x00A957AC);
GAME_VAR(float, twoPi, 0x00A453D4);
inline void* const treeKeyA = reinterpret_cast<void*>(0x01B6531C);
inline void* const treeKeyB = reinterpret_cast<void*>(0x01B6535C);
inline void* const treeKeyEntry = reinterpret_cast<void*>(0x01B65360);

GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, CrossProduct, 0x00414730, CVector* out, const CVector* a, const CVector* b);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(void*, __cdecl, Texture_GetRaster, 0x0051A120, void* texture);
// the Im3D batch: reset, room for indices and vertices, flushed
GAME_FN(void, __cdecl, Im3DBatch_Reset, 0x0052AE80);
GAME_FN(void, __cdecl, Im3DBatch_Get, 0x0052AE10, int numIndices, int numVertices, re::Ptr32<uint16_t>* indices,
        re::Ptr32<Im3DVertex>* vertices, int* firstIndex);
GAME_FN(void, __cdecl, Im3DBatch_Flush, 0x0052AE90);
GAME_FN(uint8_t, __cdecl, CalcScreenCoors, 0x0052AEE0, const CVector* pos, CVector* out, float* scaleX, float* scaleY,
        int, int, int, int);
GAME_FN(void, __cdecl, DrawSprite2D, 0x0052CCC0, float x0, float y0, float x1, float y1, float x2, float y2, float x3,
        float y3, void* texture, const uint8_t* color, const float* uv0, const float* uv1);
GAME_FN(float, __cdecl, ScreenCentreX, 0x0052C690, int which);   // (1: 320)
GAME_FN(float, __cdecl, ScreenCentreY, 0x0052C6B0, int which);   // (1: 240)
// textures
GAME_FN(void, __cdecl, Txd_Push, 0x005F0A90);
GAME_FN(void, __cdecl, Txd_Pop, 0x005F0AA0);
GAME_FN(int, __cdecl, FindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(void, __cdecl, SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void*, __cdecl, LoadTexture, 0x005F1770, const char* name, const char* mask);
GAME_FN(uint8_t, __cdecl, NiRelease, 0x005F0F90, void* object);
// the data tree: a path of names, its children
GAME_FN(void, __thiscall, TreeName_Copy, 0x005FA880, uint32_t* self, const void* source);
GAME_FN(void, __thiscall, TreePath_Set, 0x005FA8C0, void* self, int count, uint32_t* names);
GAME_FN(int, __cdecl, Tree_GetNumChildren, 0x0072B7D0, void* path);
GAME_FN(uint8_t*, __cdecl, Tree_GetChild, 0x0072B810, void* path, int index, void* key);
GAME_FN(void, __thiscall, Nop_49A650, 0x0049A650, void* self);   // (an empty destructor)
// (their own, called where modes apply)
GAME_FN(void, __cdecl, DrawBillboard_4DB740, 0x004DB740, CVector pos, CVector dir, float size, float angle, int r,
        int g, int b, int a);
GAME_FN(void, __thiscall, Reticle_Render, 0x004DBBF0, CReticle* self);

// 0x4DB290
void __cdecl SetCurrentCrosshair(int index) {
    currentCrosshair = index;
}

// 0x4DB2A0: the textures loaded (each crosshair name's index kept in its tree entry,
// +0x40: the same names share one) and the reticles' defaults
void __cdecl Init() {
    Txd_Push();
    SetCurrentTxd(FindTxdSlot("HUDElems", 0));
    uint32_t keys[2];
    TreeName_Copy(&keys[0], treeKeyA);
    TreeName_Copy(&keys[1], treeKeyB);
    struct {
        int32_t count;
        re::Ptr32<uint32_t> names;
    } path;
    TreePath_Set(&path, 2, keys);
    int numChildren = Tree_GetNumChildren(&path);
    char name[0x68];
    char names[10][100];
    int loaded = 0;
    for (int i = 0; i < numChildren;) {
        uint8_t* entry = Tree_GetChild(&path, i, treeKeyEntry);
        const char* entryName = *reinterpret_cast<re::Ptr32<const char>*>(entry + 0x18);
        int index = loaded;
        std::strcpy(name, entryName);
        std::strcat(name, "_x");
        int same = 0;
        for (; same < loaded; same++) {
            if (std::strcmp(name, names[same]) == 0) {
                break;
            }
        }
        if (same < loaded) {
            index = same;
        } else {
            crosshairTextures[loaded] = LoadTexture(entryName, name);
            std::strcpy(names[loaded], name);
            loaded++;
        }
        i++;
        *reinterpret_cast<int32_t*>(entry + 0x40) = index;
        if (loaded >= 10) {
            break;
        }
    }
    void* marker = LoadTexture("HUD_xhair", "HUD_xhair_x");
    reticles[2].texture = marker;
    reticles[1].texture = marker;
    Txd_Pop();
    for (int i = 1; i < 3; i++) {
        CReticle& r = reticles[i];
        r.spin = 0.02f;
        r.color[0] = 0xFF;
        r.color[1] = 0x58;
        r.color[2] = 0x64;
        r.color[3] = 0x80;
        r.screenSpace = 0;
        r.brightness = 0x80;
        r.unscaled = 0;
        r.billboard = 0;
    }
    CReticle& crosshair = reticles[0];
    crosshair.spin = 0.0f;
    crosshair.color[0] = 0xFF;
    crosshair.color[1] = 0xFF;
    crosshair.color[2] = 0x32;
    crosshair.color[3] = 0xFF;
    crosshair.screenSpace = 1;
    crosshair.brightness = 200;
    crosshair.unscaled = 1;
    crosshair.billboard = 0;
    crosshair.pos.x = ScreenCentreX(1);
    crosshair.pos.y = ScreenCentreY(1);
    crosshair.pos.z = 3.0f;
    overlayOn = 0;
    overlayShape = -1;
    overlayBone = -1;
    Nop_49A650(&keys[1]);
    Nop_49A650(&keys[0]);
}

// 0x4DB530: the textures let go (the second marker's shared one isn't cleared)
void __cdecl Shutdown() {
    for (int i = 0; i < 10; i++) {
        if (crosshairTextures[i]) {
            NiRelease(crosshairTextures[i]);
        }
        crosshairTextures[i] = nullptr;
    }
    NiRelease(reticles[1].texture);
    reticles[0].texture = nullptr;
    reticles[1].texture = nullptr;
}

// 0x4DB580: a marker shown as a billboard
void __cdecl SetMarker(int marker, float x, float y, float z, float dirX, float dirY, float dirZ, float spin, float size) {
    CReticle& r = reticles[1 + marker];
    r.pos = {x, y, z};
    r.dir = {dirX, dirY, dirZ};
    r.spin = spin;
    r.size = size;
    r.active = 1;
    r.billboard = 1;
}

// 0x4DB5F0
void __cdecl HideCrosshair() {
    reticles[0].active = 0;
}

// 0x4DB600
void __cdecl HideMarker(int marker) {
    reticles[1 + marker].active = 0;
    reticles[1 + marker].billboard = 0;
}

// 0x4DB620
void __cdecl SetMarkerColor(int marker, int r, int g, int b, int a, int brightness) {
    CReticle& m = reticles[1 + marker];
    m.color[0] = uint8_t(r);
    m.color[1] = uint8_t(g);
    m.color[2] = uint8_t(b);
    m.color[3] = uint8_t(a);
    m.brightness = uint8_t(brightness);
}

// 0x4DB660: the crosshair in the screen's centre (put back there when it was placed
// in the world)
void __cdecl ShowCrosshair(float size, int r, int g, int b, int a) {
    CReticle& c = reticles[0];
    c.size = size;
    c.color[0] = uint8_t(r);
    c.active = 1;
    c.color[1] = uint8_t(g);
    c.color[2] = uint8_t(b);
    c.color[3] = uint8_t(a);
    if (c.screenSpace == 0) {
        c.pos.x = ScreenCentreX(1);
        c.pos.y = ScreenCentreY(1);
        c.pos.z = 3.0f;
    }
    c.screenSpace = 1;
}

// 0x4DB6E0: the crosshair at a world position
void __cdecl ShowCrosshairAt(float x, float y, float z, float size, int r, int g, int b, int a) {
    CReticle& c = reticles[0];
    c.size = size;
    c.pos = {x, y, z};
    c.color[0] = uint8_t(r);
    c.active = 1;
    c.color[1] = uint8_t(g);
    c.color[2] = uint8_t(b);
    c.color[3] = uint8_t(a);
    c.screenSpace = 0;
}

// 0x4DB740: a textured quad at `pos` facing `dir`, turned by `angle`, through the Im3D
// batch. (Unless `dir` points up, its "up" is the cross product of a zero vector: zero,
// so the quad collapses to a line, as in the game)
void __cdecl DrawBillboard(CVector pos, CVector dir, float size, float angle, int r, int g, int b, int a) {
    CVector up{0.0f, 0.0f, 0.0f};
    if (!(double(dir.z) > 0.9998999834060669)) {
        CVector axis{0.0f, 1.0f, 0.0f};
        CVector cross;
        CrossProduct(&cross, &up, &axis);
        up = cross;
    } else {
        up = {1.0f, 0.0f, 0.0f};
    }
    CVector side;
    CrossProduct(&side, &dir, &up);
    float halfSin = float(double(float(std::sin(double(angle))) * size) * 0.5);
    float halfCos = float(double(float(std::cos(double(angle))) * size) * 0.5);
    float negHalfSin = -halfSin;
    CVector a1{up.x * halfSin, up.y * halfSin, up.z * halfSin};
    CVector b1{side.x * halfCos, side.y * halfCos, side.z * halfCos};
    CVector p1;
    VectorAdd(&p1, &a1, &b1);
    CVector a2{side.x * negHalfSin, side.y * negHalfSin, side.z * negHalfSin};
    CVector b2{up.x * halfCos, up.y * halfCos, up.z * halfCos};
    CVector p2;
    VectorAdd(&p2, &b2, &a2);
    CVector t, corners[4];
    VectorSub(&corners[0], VectorSub(&t, &pos, &p1), &p2);
    VectorSub(&corners[1], VectorAdd(&t, &pos, &p1), &p2);
    VectorAdd(&corners[2], VectorAdd(&t, &pos, &p1), &p2);
    VectorAdd(&corners[3], VectorSub(&t, &pos, &p1), &p2);

    Im3DBatch_Reset();
    re::Ptr32<uint16_t> indices;
    re::Ptr32<Im3DVertex> vertices;
    int first;
    Im3DBatch_Get(6, 4, &indices, &vertices, &first);
    uint32_t color = (uint32_t(uint8_t(a)) << 24) | (uint32_t(uint8_t(r)) << 16) | (uint32_t(uint8_t(g)) << 8) |
                     uint32_t(uint8_t(b));
    static const float uvs[4][2] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
    for (int i = 0; i < 4; i++) {
        vertices[i].pos = corners[i];
        vertices[i].color = color;
        vertices[i].u = uvs[i][0];
        vertices[i].v = uvs[i][1];
    }
    indices[0] = uint16_t(first);
    indices[1] = uint16_t(first + 1);
    indices[2] = uint16_t(first + 2);
    indices[3] = uint16_t(first);
    indices[4] = uint16_t(first + 2);
    indices[5] = uint16_t(first + 3);
    Nop_49A650(nullptr);
    Im3DBatch_Flush();
}

// 0x4DBBF0 (thiscall): drawn (a billboard; else a 2D sprite in screen space or at
// its projected position, scaled by the projection unless `unscaled`), then spun
void __fastcall Render(CReticle* self, re::Edx) {
    if (!self->active) {
        return;
    }
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    SetRenderState(8, 0);
    SetRenderState(6, 0);
    SetRenderState(0xC, 1);
    SetRenderState(0xA, 5);
    SetRenderState(0xB, 6);
    bool draw = true;
    float x = 0.0f;
    float y = 0.0f;
    if (self->billboard) {
        SetRenderState(6, 1);
        SetRenderState(1, int(reinterpret_cast<uintptr_t>(Texture_GetRaster(self->texture))));
        DrawBillboard_4DB740(self->pos, self->dir, self->size, self->angle, self->color[0], self->color[1],
                             self->color[2], self->color[3]);
        draw = false;
    } else if (self->screenSpace) {
        x = self->pos.x;
        y = self->pos.y;
    } else {
        CVector screen;
        if (!CalcScreenCoors(&self->pos, &screen, &scaleX, &scaleY, 1, 0, 0, 1)) {
            draw = false;
        } else {
            if (self->unscaled) {
                scaleX = 1.0f;
                scaleY = 1.0f;
            }
            x = screen.x;
            y = screen.y;
        }
    }
    if (draw) {
        float width = self->size * scaleX;
        float height = self->size * scaleY;
        uint8_t color[4];
        int brightness = self->brightness;
        color[0] = uint8_t((self->color[0] * brightness) >> 8);
        color[1] = uint8_t((self->color[1] * brightness) >> 8);
        color[2] = uint8_t((self->color[2] * brightness) >> 8);
        color[3] = self->color[3];
        float sine = float(std::sin(double(self->angle)));
        float cosine = float(std::cos(double(self->angle)));
        float kx = screenScaleX;
        float ky = screenScaleY;
        float ax = (cosine * width) * kx;
        float ay = (sine * width) * ky;
        float bx = (sine * height) * kx;
        float by = (cosine * height) * ky;
        float uv0[2] = {0.0f, 0.0f};
        float uv1[2] = {1.0f, 1.0f};
        DrawSprite2D((x - ax) + bx, (y - ay) - by, (x + ax) + bx, (y + ay) - by, (x - ax) - bx, (y - ay) + by,
                     (x + ax) - bx, (y + ay) + by, self->texture, color, uv0, uv1);
    }
    if (self->spin != 0.0f) {
        float angle = self->spin + self->angle;
        self->angle = angle;
        if (twoPi < angle) {
            self->angle = angle - twoPi;
        }
    }
    SetRenderState(0xC, 0);
    SetRenderState(8, 1);
    SetRenderState(6, 1);
}

// 0x4DC0B0: the crosshair (with the current texture) and the markers
void __cdecl RenderAll() {
    reticles[0].texture = crosshairTextures[currentCrosshair];
    Reticle_Render(&reticles[0]);
    for (int i = 1; i < 3; i++) {
        Reticle_Render(&reticles[i]);
    }
}

} // namespace c12reticle

// ---- registration ---------------------------------------------------------------------

#define RETICLE_C12D_REVERSED(name, address, fn)   \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

RETICLE_C12D_REVERSED("CReticles::SetCurrentCrosshair", 0x004DB290, &c12reticle::SetCurrentCrosshair);
RETICLE_C12D_REVERSED("CReticles::Init", 0x004DB2A0, &c12reticle::Init);
RETICLE_C12D_REVERSED("CReticles::Shutdown", 0x004DB530, &c12reticle::Shutdown);
RETICLE_C12D_REVERSED("CReticles::SetMarker", 0x004DB580, &c12reticle::SetMarker);
RETICLE_C12D_REVERSED("CReticles::HideCrosshair", 0x004DB5F0, &c12reticle::HideCrosshair);
RETICLE_C12D_REVERSED("CReticles::HideMarker", 0x004DB600, &c12reticle::HideMarker);
RETICLE_C12D_REVERSED("CReticles::SetMarkerColor", 0x004DB620, &c12reticle::SetMarkerColor);
RETICLE_C12D_REVERSED("CReticles::ShowCrosshair", 0x004DB660, &c12reticle::ShowCrosshair);
RETICLE_C12D_REVERSED("CReticles::ShowCrosshairAt", 0x004DB6E0, &c12reticle::ShowCrosshairAt);
RETICLE_C12D_REVERSED("CReticles::DrawBillboard", 0x004DB740, &c12reticle::DrawBillboard);
RETICLE_C12D_REVERSED("CReticle::Render", 0x004DBBF0, &c12reticle::Render);
RETICLE_C12D_REVERSED("CReticles::RenderAll", 0x004DC0B0, &c12reticle::RenderAll);
