// The render buffer (0x52AE10..0x52AED8: GTA's RenderBuffer, 0xB32 vertices at 0xBD7800,
// 0x1665 indices at 0xBF0B2C) and CSprite (0x52AEE0..0x52C21F: screen coordinates of a
// world point, the corona sprites as screen quads or camera-facing billboards, a
// circle, the horizon).
//
// Found here: the screen quads go through 0x7395B0 (a rectangle) and 0x739820 (four
// corners), with the corners' colors at 0x20DCCF4 and the texture rectangle at
// 0x20DCCD4 (0x52AC90); the z passed is truncated to a whole number unless 0x20DCD21.
// 0x52B1F0 takes r, g, b in al, cl, dl (not hookable).

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "reversed.h"

namespace sprite17 {

using col::CVector;

struct BufferVertex {
    CVector pos;              // +0
    CVector normal;           // +0xC
    uint32_t color;           // +0x18
    float u;                  // +0x1C
    float v;                  // +0x20
};
static_assert(sizeof(BufferVertex) == 0x24);

// Gamebryo's NiTransform (rotation rows, translation, scale)
struct NiTransform {
    float rotate[9];
    CVector translate;
    float scale;
};
static_assert(sizeof(NiTransform) == 0x34);

GAME_ARRAY(BufferVertex, TempBufferVertices, 0x00BD7800);
GAME_ARRAY(uint16_t, TempBufferIndices, 0x00BF0B2C);
GAME_VAR(int32_t, TempBufferVerticesStored, 0x00CA2AE8);
GAME_VAR(int32_t, TempBufferIndicesStored, 0x00CA2AEC);
GAME_VAR(void*, cameraNode, 0x00C3CCF4);
inline const float* const viewMatrix = reinterpret_cast<const float*>(0x00C3CCB0);   // TheCamera +0x48
GAME_VAR(int32_t, fovOverridden, 0x00C3CDD4);
GAME_VAR(float, fovOverride, 0x00C3CDD0);
GAME_VAR(float, ms_fov, 0x00C3CD04);
GAME_VAR(float, spriteScaleX, 0x00A957C4);
GAME_VAR(float, spriteScaleY, 0x00A957C0);
GAME_VAR(uint8_t*, currentRaster, 0x020DCCC8);   // (+0xC width, +0x10 height)
GAME_VAR(uint32_t, quadColor, 0x020DCD14);
GAME_ARRAY(uint32_t, cornerColors, 0x020DCCF4);
GAME_VAR(uint8_t, keepFractionalZ, 0x020DCD21);
GAME_VAR(uint8_t*, transitionTimes, 0x020C8910);  // (+0x20: the horizon distance)

GAME_FN(void*, __cdecl, Im3D_Transform, 0x005EFAF0, void* vertices, int count, void* transform, uint32_t flags);
GAME_FN(void, __cdecl, Im3D_RenderIndexedPrimitive, 0x005F0170, int type, uint16_t* indices, int count, int);
GAME_FN(void, __cdecl, Im3D_RenderPrimitive, 0x005F09D0, int type);
GAME_FN(uint8_t, __cdecl, Im3D_End, 0x005EFB20);
GAME_FN(void, __cdecl, RenderStuffInBuffer_Game, 0x0052AE90);
GAME_FN(float, __cdecl, Camera_GetNear, 0x006C82E0, void* camera);
GAME_FN(float, __cdecl, Camera_GetFar, 0x006C82D0, void* camera);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
// (0x0041A9A0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* TransformPoint(CVector* out, const float* m, const CVector* v) { return re::CallVec16<0x0041A9A0, CVector*>(out, m, v); }
// (0x0045D500 stores 16 bytes through `out4`: called through re::CallVec16)
static inline float* MultiplyPoint4(float* out4, const float* m, const CVector* v) { return re::CallVec16<0x0045D500, float*>(out4, m, v); }
GAME_FN(float, __thiscall, Vector_NormaliseSafe, 0x00415130, CVector* v);
GAME_FN(void, __thiscall, NiMatrix3_MakeZRotation, 0x007687D0, float* m, float angle);
GAME_FN(float*, __thiscall, NiMatrix3_Multiply, 0x00768920, const float* m, float* out, const float* other);
GAME_FN(void, __cdecl, GetRenderState, 0x00519290, int state, void* value);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(void, __cdecl, SetClipRect, 0x0043A6D0, float x1, float y1, float x2, float y2);
GAME_FN(void, __cdecl, SetInsetRects, 0x0052AC90, float a0, float a1, float a2, float a3, float b0, float b1, float b2, float b3);
GAME_FN(void, __cdecl, DrawRect, 0x007395B0, float x1, float y1, float z1, float x2, float y2, float z2);
GAME_FN(void, __cdecl, DrawQuad, 0x00739820, float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2,
        float x3, float y3, float z3);

// ---- RenderBuffer ----------------------------------------------------------------------

// 0x52AE10: RenderBuffer::StartStoring: room for the vertices and indices (the buffer
// drawn first when they wouldn't fit)
void __cdecl StartStoring(int numIndices, int numVerts, re::Ptr32<uint16_t>* indices, re::Ptr32<BufferVertex>* verts, int* base) {
    if (TempBufferVerticesStored + numVerts >= 0xB32 || TempBufferIndicesStored + numIndices >= 0x1665) {
        RenderStuffInBuffer_Game();
    }
    *base = TempBufferVerticesStored;
    *verts = &TempBufferVertices[TempBufferVerticesStored];
    TempBufferVerticesStored += numVerts;
    *indices = &TempBufferIndices[TempBufferIndicesStored];
    TempBufferIndicesStored += numIndices;
}

// 0x52AE80: RenderBuffer::ClearRenderBuffer
void __cdecl ClearRenderBuffer() {
    TempBufferVerticesStored = 0;
    TempBufferIndicesStored = 0;
}

// 0x52AE90: RenderBuffer::RenderStuffInBuffer: the stored triangles drawn, then emptied
void __cdecl RenderStuffInBuffer() {
    if (TempBufferVerticesStored != 0 && Im3D_Transform(TempBufferVertices, TempBufferVerticesStored, nullptr, 1)) {
        Im3D_RenderIndexedPrimitive(3, TempBufferIndices, TempBufferIndicesStored, 0);
        Im3D_End();
    }
    TempBufferVerticesStored = 0;
    TempBufferIndicesStored = 0;
}

// ---- CSprite ---------------------------------------------------------------------------

static float Fov() {
    return fovOverridden == 0 ? ms_fov : fovOverride;
}

// the z the screen quads get: truncated to a whole number (as unsigned) unless kept
static float QuadZ(float z) {
    if (keepFractionalZ) {
        return z;
    }
    return float(uint32_t(int64_t(z)));
}

// 0x52AEE0: CSprite::CalcScreenCoors: `in` on the 640x480 screen and the size of a unit
// there; false when behind the near plane (+1), beyond the far one, or (with the sprite's
// size, 1.5 times) off screen
uint8_t __cdecl CalcScreenCoorsSized(const CVector* in, CVector* out, float* outW, float* outH, uint8_t farClip, int sizeX, int sizeY,
                                     uint8_t nearClip) {
    float nearZ = Camera_GetNear(cameraNode);
    float farZ = Camera_GetFar(cameraNode);
    struct {
        CVector v;
        float w;   // (0x41A9A0 stores a 4th dword: the game's own stack slot)
    } p;
    TransformPoint(&p.v, viewMatrix, in);
    *out = p.v;
    if (nearClip && !(nearZ + 1.0f < out->z)) {
        return 0;
    }
    if (farClip && !(farZ > out->z)) {
        return 0;
    }
    float recip = 1.0f / out->z;
    out->x = out->x * (recip * 640.0f);
    out->y = out->y * (recip * 480.0f);
    *outW = recip * 640.0f;
    *outH = recip * 480.0f;
    *outW = *outW * (70.0f / Fov());
    *outH = (70.0f / Fov()) * *outH;
    float spanX = *outW * (float(sizeX) * 1.5f);
    float spanY = *outH * (1.5f * float(sizeY));
    if (!(-spanX <= out->x) || !(out->x < spanX + 640.0f)) {
        return 0;
    }
    if (!(-spanY <= out->y) || !(out->y < spanY + 480.0f)) {
        return 0;
    }
    return 1;
}

// 0x52B090: CSprite::CalcScreenCoors (the unit's size capped at 1 / 1)
uint8_t __cdecl CalcScreenCoors(const CVector* in, CVector* out, float* outW, float* outH) {
    float nearZ = Camera_GetNear(cameraNode);
    float farZ = Camera_GetFar(cameraNode);
    struct {
        CVector v;
        float w;   // (0x41A9A0 stores a 4th dword: the game's own stack slot)
    } p;
    TransformPoint(&p.v, viewMatrix, in);
    *out = p.v;
    if (!(out->z > nearZ + 1.0f) || !(farZ > out->z)) {
        return 0;
    }
    float recip = 1.0f / out->z;
    out->x = out->x * (recip * 640.0f);
    out->y = out->y * (recip * 480.0f);
    if (1.0f < recip) {
        recip = 1.0f;
    }
    *outW = recip * 640.0f;
    *outH = recip * 480.0f;
    *outW = *outW * (70.0f / Fov());
    *outH = (70.0f / Fov()) * *outH;
    return 1;
}

// 0x52B1F0: a camera-facing billboard at pos (r, g, b in al, cl, dl), turned by
// `rotation` about the view direction
void RenderBillboardRGB(uint8_t red, uint8_t green, uint8_t blue, const CVector* pos, float w, float h, int16_t intensity, float rotation,
                        uint8_t alpha) {
    float halfY = spriteScaleX * w;
    float halfX = spriteScaleY * h;
    int in = intensity;
    uint32_t color = uint32_t(alpha) << 24 | uint32_t(uint8_t((red * in) >> 8)) << 16 | uint32_t(uint8_t((green * in) >> 8)) << 8 |
                     uint8_t((blue * in) >> 8);
    BufferVertex verts[4];
    verts[0] = BufferVertex{{-halfX, halfY, 0.0f}, {0.0f, 0.0f, 1.0f}, color, 0.0f, 1.0f};
    verts[1] = BufferVertex{{-halfX, -halfY, 0.0f}, {0.0f, 0.0f, 1.0f}, color, 0.0f, 0.0f};
    verts[2] = BufferVertex{{halfX, -halfY, 0.0f}, {0.0f, 0.0f, 1.0f}, color, 1.0f, 0.0f};
    verts[3] = BufferVertex{{halfX, halfY, 0.0f}, {0.0f, 0.0f, 1.0f}, color, 1.0f, 1.0f};
    NiTransform transform;
    transform.translate = *pos;
    transform.scale = 1.0f;
    uint8_t* camera = GetLocalTransform(GetCameraNode(cameraNode));
    CVector d = {*reinterpret_cast<float*>(camera + 0x24) - pos->x, *reinterpret_cast<float*>(camera + 0x28) - pos->y,
                 *reinterpret_cast<float*>(camera + 0x2C) - pos->z};
    Vector_NormaliseSafe(&d);
    CVector a;
    if (d.x * 0.0f + d.y * 0.0f + d.z < 0.999f) {
        a = CVector{d.z * 0.0f - d.y, d.x - d.z * 0.0f, d.y * 0.0f - d.x * 0.0f};
        Vector_NormaliseSafe(&a);
    } else {
        a = CVector{0.0f, 1.0f, 0.0f};
    }
    CVector b = {a.y * d.z - a.z * d.y, a.z * d.x - a.x * d.z, a.x * d.y - a.y * d.x};
    float* m = transform.rotate;
    m[0] = b.x;
    m[1] = a.x;
    m[2] = d.x;
    m[3] = b.y;
    m[4] = a.y;
    m[5] = d.y;
    m[6] = b.z;
    m[7] = a.z;
    m[8] = d.z;
    if (rotation != 0.0f) {
        float spin[9];
        float result[10];
        result[0] = 1.0f;
        NiMatrix3_MakeZRotation(spin, -rotation);
        float* product = NiMatrix3_Multiply(m, result, spin);
        for (int i = 0; i < 9; i++) {
            m[i] = product[i];
        }
    }
    if (Im3D_Transform(verts, 4, &transform, 1)) {
        Im3D_RenderPrimitive(5);
        Im3D_End();
    }
}
RE_UNHOOKED_FN("CSprite::RenderBillboard (al, cl, dl)", 0x0052B1F0, &RenderBillboardRGB);

// the game's 0x52B1F0 with r, g, b in al, cl, dl (where modes apply)
static __declspec(noinline) void CallRenderBillboard(uint8_t red, uint8_t green, uint8_t blue, const CVector* pos, float w, float h, int intensity,
                                float rotation, int alpha) {
#if defined(_M_IX86)
    void* fn = reinterpret_cast<void*>(0x0052B1F0);
    __asm {
        push alpha
        push rotation
        push intensity
        push h
        push w
        push pos
        mov al, red
        mov cl, green
        mov dl, blue
        call fn
        add esp, 0x18
    }
#else
    re::Ours<void (*)(uint8_t, uint8_t, uint8_t, const CVector*, float, float, int16_t, float, int)>(0x0052B1F0)(
        red, green, blue, pos, w, h, int16_t(intensity), rotation, alpha);
#endif
}

// 0x52B590: CSprite::RenderBufferedOneXLUSprite (no rotation)
void __cdecl RenderBillboardUnrotated(const CVector* pos, float w, float h, uint8_t red, uint8_t green, uint8_t blue, int16_t intensity,
                                      uint8_t alpha) {
    CallRenderBillboard(red, green, blue, pos, w, h, intensity, 0.0f, alpha);
}

// 0x52B5D0: the same faded out between 2.3 and 1.3 from the camera (none nearer)
void __cdecl RenderBillboardFaded(const CVector* pos, float dist, float w, float h, uint8_t red, uint8_t green, uint8_t blue,
                                  int16_t intensity, float rotation, uint8_t alpha) {
    int r = red, g = green, b = blue, in = intensity;
    if (dist < 2.3f) {
        if (!(1.3f <= dist)) {
            return;
        }
        int fade = int((dist - 1.3f) * 255.0f);
        r = (r * fade) >> 8;
        g = (g * fade) >> 8;
        b = (b * fade) >> 8;
        in = uint16_t((in * fade) >> 8);
    }
    CallRenderBillboard(uint8_t(r), uint8_t(g), uint8_t(b), pos, w, h, int16_t(in), rotation, alpha);
}

// 0x52B690: CSprite::RenderOneXLUSprite: a screen rectangle at (x, y), half size w, h
void __cdecl RenderOneXLUSprite(float x, float y, float z, float w, float h, uint8_t red, uint8_t green, uint8_t blue, int16_t intensity,
                                uint8_t alpha) {
    GetRenderState(1, &currentRaster);
    int in = intensity;
    uint8_t c[4] = {uint8_t((red * in) >> 8), uint8_t((green * in) >> 8), uint8_t((blue * in) >> 8), alpha};
    quadColor = *reinterpret_cast<uint32_t*>(c);
    if (currentRaster) {
        SetClipRect(0.0f, 0.0f, float(*reinterpret_cast<int32_t*>(currentRaster + 0xC)), float(*reinterpret_cast<int32_t*>(currentRaster + 0x10)));
    }
    float qz = QuadZ(z);
    float sx = spriteScaleX;
    float sy = spriteScaleY;
    float dy = sy * h;
    float dx = sx * w;
    DrawRect(x - dx, y - dy, qz, x + dx, y + dy, qz);
}

static float ClampF(float v, float lo, double hiTest, float hi) {
    if (!(lo <= v)) {
        return lo;
    }
    return static_cast<double>(v) <= hiTest ? v : hi;
}

// 0x52B7E0: CSprite::RenderOneXLUSprite_Rotate_Aspect: a turned screen quad (faded
// out between 2.3 and 1.3 of depth, none nearer), its corners kept on the screen area
void __cdecl RenderOneXLUSprite_Rotate_Aspect(float x, float y, float z, float w, float h, uint8_t red, uint8_t green, uint8_t blue,
                                              int16_t intensity, float rotation, uint8_t alpha) {
    GetRenderState(1, &currentRaster);
    uint8_t* raster = currentRaster;
    if (raster) {
        SetClipRect(0.0f, 0.0f, float(*reinterpret_cast<int32_t*>(raster + 0xC)), float(*reinterpret_cast<int32_t*>(raster + 0x10)));
    }
    int r = red, g = green, b = blue;
    int in = intensity;
    if (z < 2.3f) {
        if (!(1.3f <= z)) {
            return;
        }
        int fade = int((z - 1.3f) * 255.0f);
        r = (r * fade) >> 8;
        g = (g * fade) >> 8;
        b = (b * fade) >> 8;
        in = uint16_t((int(intensity) * fade) >> 8);
    }
    int si = int16_t(in);
    uint8_t c[4] = {uint8_t((uint8_t(r) * si) >> 8), uint8_t((uint8_t(g) * si) >> 8), uint8_t((uint8_t(b) * si) >> 8), alpha};
    float s = static_cast<float>(std::sin(static_cast<double>(rotation)));
    float co = static_cast<float>(std::cos(static_cast<double>(rotation)));
    float sx = spriteScaleX;
    float sy = spriteScaleY;
    float kA = -co - s;
    float kB = s - co;
    float kC = co + s;
    float kD = co - s;
    const float lo = float(0.0 - 1400.0);
    float x0 = ClampF(kA * w * sx + x, lo, 2040.0, 2040.0f);
    float x1 = ClampF(kB * w * sx + x, lo, 2040.0, 2040.0f);
    float x2 = ClampF(kC * w * sx + x, lo, 2040.0, 2040.0f);
    float x3 = ClampF(x + kD * w * sx, lo, 2040.0, 2040.0f);
    float y0 = ClampF(kB * h * sy + y, lo, 1880.0, 1880.0f);
    float y1 = ClampF(h * kC * sy + y, lo, 1880.0, 1880.0f);
    float y2 = ClampF(h * kD * sy + y, lo, 1880.0, 1880.0f);
    float y3 = ClampF(h * kA * sy + y, lo, 1880.0, 1880.0f);
    if (raster) {
        float fh = float(*reinterpret_cast<int32_t*>(raster + 0x10) + 1);
        float fw = float(*reinterpret_cast<int32_t*>(raster + 0xC) + 1);
        SetInsetRects(0.0f, 0.0f, fw, 0.0f, 0.0f, fh, fw, fh);
    }
    uint32_t color = *reinterpret_cast<uint32_t*>(c);
    for (int i = 0; i < 4; i++) {
        cornerColors[i] = color;
    }
    float qz = QuadZ(z);
    DrawQuad(float(int(x0)), float(int(y0)), qz, float(int(x3)), float(int(y3)), qz, float(int(x1)), float(int(y1)), qz, float(int(x2)),
             float(int(y2)), qz);
}

// 0x52BD50: CSprite::RenderOneXLUSprite_Rotate: a turned screen quad
void __cdecl RenderOneXLUSprite_Rotate(float x, float y, float z, float w, float h, uint8_t red, uint8_t green, uint8_t blue,
                                       int16_t intensity, float rotation, uint8_t alpha) {
    GetRenderState(1, &currentRaster);
    int in = intensity;
    uint8_t c[4] = {uint8_t((red * in) >> 8), uint8_t((green * in) >> 8), uint8_t((blue * in) >> 8), alpha};
    float s = static_cast<float>(std::sin(static_cast<double>(rotation)));
    float co = static_cast<float>(std::cos(static_cast<double>(rotation)));
    float wx = spriteScaleX * w;
    float hy = spriteScaleY * h;
    float cw = co * wx;
    float sh = hy * s;
    float x0 = (x - cw) - sh;
    float x1 = (x - cw) + sh;
    float x2 = (x + cw) + sh;
    float x3 = (x + cw) - sh;
    float ch = co * hy;
    float sw = wx * s;
    float y0 = (y - ch) + sw;
    float y1 = (y + ch) + sw;
    float y2 = (y + ch) - sw;
    float y3 = (y - ch) - sw;
    uint8_t* raster = currentRaster;
    if (raster) {
        float fh = float(*reinterpret_cast<int32_t*>(raster + 0x10));
        float fw = float(*reinterpret_cast<int32_t*>(raster + 0xC));
        SetInsetRects(0.0f, 0.0f, fw, 0.0f, 0.0f, fh, fw, fh);
    }
    uint32_t color = *reinterpret_cast<uint32_t*>(c);
    for (int i = 0; i < 4; i++) {
        cornerColors[i] = color;
    }
    float qz = QuadZ(z);
    DrawQuad(float(int(x0)), float(int(y0)), qz, float(int(x3)), float(int(y3)), qz, float(int(x1)), float(int(y1)), qz, float(int(x2)),
             float(int(y2)), qz);
}

// 0x52BFF0: a ring (a strip between radii r1 and r2 over `degrees`, in `segments`;
// centred on the gap when `centred`) through the buffer, untextured
void __cdecl RenderRing(void* transform, uint32_t rgba, float r1, float r2, float degrees, int segments, uint8_t centred) {
    float span = degrees * 3.14159274f / 180.0f;
    TempBufferIndicesStored = 0;
    TempBufferVerticesStored = segments * 2 + 2;
    float step = span / float(segments);
    float angle = centred ? (6.28318548f - span) * 0.5f : 0.0f;
    uint32_t color = (rgba >> 24) << 24 | (rgba & 0xFF) << 16 | ((rgba >> 8) & 0xFF) << 8 | ((rgba >> 16) & 0xFF);
    BufferVertex* v = TempBufferVertices;
    for (int i = segments + 1; i > 0; i--) {
        v[0].color = color;
        float c = static_cast<float>(std::cos(static_cast<double>(angle)));
        v[0].pos.x = c * r1;
        float s = static_cast<float>(std::sin(static_cast<double>(angle)));
        v[0].pos.y = s * r1;
        v[0].pos.z = 0.0f;
        v[1].color = color;
        v[1].pos.x = c * r2;
        v[1].pos.y = r2 * s;
        v[1].pos.z = 0.0f;
        v += 2;
        angle = angle + step;
    }
    SetRenderState(1, 0);
    if (Im3D_Transform(TempBufferVertices, TempBufferVerticesStored, transform, 0x18)) {
        Im3D_RenderPrimitive(4);
        Im3D_End();
    }
}

// 0x52C130: CSprite::CalcHorizonCoors: the screen y of the horizon (a point that far
// ahead of the camera, at z 0)
float __cdecl CalcHorizonCoors() {
    uint8_t* camera = GetLocalTransform(GetCameraNode(cameraNode));
    CVector p = {*reinterpret_cast<float*>(camera + 0x24), *reinterpret_cast<float*>(camera + 0x28), 0.0f};
    uint8_t* camera2 = GetLocalTransform(GetCameraNode(cameraNode));
    float dist = *reinterpret_cast<float*>(transitionTimes + 0x20);
    float fx = dist * *reinterpret_cast<float*>(camera2 + 8);
    float fy = dist * *reinterpret_cast<float*>(camera2 + 0x14);
    p.x = p.x + fx;
    p.y = p.y + fy;
    p.z = 0.0f;
    float out[4];
    MultiplyPoint4(out, viewMatrix, &p);
    float recip;
    if (static_cast<double>(std::fabs(out[2])) <= 1e-07) {
        recip = 0.0020833334f;
    } else {
        recip = 1.0f / out[2];
    }
    return recip * 480.0f * out[1];
}

} // namespace sprite17

// ---- registration ----------------------------------------------------------------------

#define SPRITE17_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace sprite17 {
SPRITE17_REVERSED("RenderBuffer::StartStoring", 0x0052AE10, &StartStoring);
SPRITE17_REVERSED("RenderBuffer::ClearRenderBuffer", 0x0052AE80, &ClearRenderBuffer);
SPRITE17_REVERSED("RenderBuffer::RenderStuffInBuffer", 0x0052AE90, &RenderStuffInBuffer);
SPRITE17_REVERSED("CSprite::CalcScreenCoors (sized)", 0x0052AEE0, &CalcScreenCoorsSized);
SPRITE17_REVERSED("CSprite::CalcScreenCoors", 0x0052B090, &CalcScreenCoors);
SPRITE17_REVERSED("CSprite::RenderBufferedOneXLUSprite", 0x0052B590, &RenderBillboardUnrotated);
SPRITE17_REVERSED("CSprite::RenderBufferedOneXLUSprite_Rotate_Faded", 0x0052B5D0, &RenderBillboardFaded);
SPRITE17_REVERSED("CSprite::RenderOneXLUSprite", 0x0052B690, &RenderOneXLUSprite);
SPRITE17_REVERSED("CSprite::RenderOneXLUSprite_Rotate_Aspect", 0x0052B7E0, &RenderOneXLUSprite_Rotate_Aspect);
SPRITE17_REVERSED("CSprite::RenderOneXLUSprite_Rotate", 0x0052BD50, &RenderOneXLUSprite_Rotate);
SPRITE17_REVERSED("CSprite::RenderRing", 0x0052BFF0, &RenderRing);
SPRITE17_REVERSED("CSprite::CalcHorizonCoors", 0x0052C130, &CalcHorizonCoors);
} // namespace sprite17
