// The window glows (0x536900..0x538C45): up to 300 lit quads (the manager at 0xCE7580,
// script natives EffectAddWindowGlowInArea / EffectTurnOn|OffWindowGlowInArea), of the
// area set with SetArea, faded in near the camera (at night, or always) and drawn in
// three passes through the render buffer: the glow ("windowglow"), a frame of two
// textures ("windowglow3" / "windowglow4", by +0x48) and the back ("windowglow2").
//
// Found here: a glow is 0x4C bytes (four corners, an effect object at +0x30, the frame's
// height and width scales at +0x38 / +0x3C, a tilt in degrees at +0x40, its area byte
// +0x44, a state +0x45, two random offsets' indices +0x46 / +0x47, +0x48 which frame
// texture, +0x49 the fade (0..100, bit 7: faded this frame), +0x4A flags: 1 jitter,
// 2 pull the frame in, 4 on). The manager keeps 9 random offsets at +0x5928 (0x28 each).

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "reversed.h"

#include "game/Crt.h"
namespace windowglow17 {

using col::CVector;

struct WindowGlow {
    CVector corners[4];   // +0
    uint8_t effect[4];    // +0x30 (an effect object, 0x668E90 / 0x668BA0)
    re::Ptr32<void> effectActive;   // +0x34
    float heightScale;    // +0x38
    float widthScale;     // +0x3C
    float tilt;           // +0x40 degrees
    uint8_t area;         // +0x44
    uint8_t state;        // +0x45
    uint8_t offset0;      // +0x46
    uint8_t offset1;      // +0x47
    uint8_t frame;        // +0x48 (the low byte of the word at +0x48)
    uint8_t fade;         // +0x49 bits 0-6 the fade, bit 7 faded this frame
    uint16_t flags;       // +0x4A
};
static_assert(offsetof(WindowGlow, heightScale) == 0x38);
static_assert(offsetof(WindowGlow, area) == 0x44);
static_assert(offsetof(WindowGlow, flags) == 0x4A);
static_assert(sizeof(WindowGlow) == 0x4C);

struct GlowOffset {
    CVector offset;       // +0
    CVector velocity;     // +0xC
    float unknown18[4];   // +0x18
};
static_assert(sizeof(GlowOffset) == 0x28);

struct WindowGlowManager {
    WindowGlow glows[300];    // +0
    int32_t count;            // +0x5910
    re::Ptr32<void> texGlow;            // +0x5914
    re::Ptr32<void> texBack;            // +0x5918
    re::Ptr32<void> texFrame[2];        // +0x591C
    int32_t area;             // +0x5924
    GlowOffset offsets[9];    // +0x5928
};
static_assert(offsetof(WindowGlowManager, count) == 0x5910);
static_assert(offsetof(WindowGlowManager, area) == 0x5924);
static_assert(offsetof(WindowGlowManager, offsets) == 0x5928);

struct BufferVertex {
    CVector pos;
    uint32_t unknownC[3];
    uint32_t color;       // +0x18
    float u, v;           // +0x1C
};
static_assert(sizeof(BufferVertex) == 0x24);

// a name with its own small buffer (0x669E20 makes it)
struct SmallString {
    re::Ptr32<char> text;
    int32_t size;
    char buffer[0x24];
};

GAME_VAR(float, fadeRange, 0x00AB4368);
GAME_VAR(int32_t, fadeStep, 0x00AB436C);
GAME_VAR(float, offsetRange, 0x00AB4370);
GAME_VAR(float, velocityRange, 0x00AB4374);
GAME_VAR(float, driftPull, 0x00AB4378);
inline void* const randomGenerator = reinterpret_cast<void*>(0x00CF356C);
GAME_VAR(uint8_t*, effectsManager, 0x00B6F1F0);   // (+0x5C + i*12: its textures)
GAME_VAR(void*, cameraNode, 0x00C3CCF4);
GAME_ARRAY(BufferVertex, TempBufferVertices, 0x00BD7800);
GAME_ARRAY(uint16_t, TempBufferIndices, 0x00BF0B2C);
GAME_VAR(int32_t, TempBufferVerticesStored, 0x00CA2AE8);
GAME_VAR(int32_t, TempBufferIndicesStored, 0x00CA2AEC);

GAME_FN(float, __thiscall, Random_GetFloat, 0x00577300, void* generator);
GAME_FN(void, __thiscall, Effect_Construct, 0x00668E90, void* effect);
GAME_FN(void, __thiscall, Effect_Reset, 0x00668BA0, void* effect);
GAME_FN(float, __thiscall, Vector_NormaliseSafe, 0x00415130, CVector* v);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(uint8_t, __cdecl, IsNightTime, 0x00426000);
GAME_FN(float*, __thiscall, NiTransform_Transform, 0x00450B10, const void* transform, float* out, const float* v);
GAME_FN(CVector*, __cdecl, AddVectors, 0x004D36B0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, ScaleVector, 0x004D3660, CVector* out, const CVector* v, float scale);
GAME_FN(CVector*, __cdecl, SubVectors, 0x00439F60, CVector* out, const CVector* a, const CVector* b);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(int, __cdecl, GetRaster, 0x0051A120, void* texture);
GAME_FN(void*, __cdecl, Im3D_Transform, 0x005EFAF0, void* vertices, int count, void* transform, uint32_t flags);
GAME_FN(void, __cdecl, Im3D_RenderIndexedPrimitive, 0x005F0170, int type, uint16_t* indices, int count, int);
GAME_FN(uint8_t, __cdecl, Im3D_End, 0x005EFB20);
GAME_FN(void, __thiscall, SmallString_Construct, 0x00669E20, SmallString* s);
inline char* __cdecl StrNCpy(char* dst, const char* src, size_t n) { return crt::Strncpy(dst, src, n); }
GAME_FN(int, __thiscall, Effects_LoadTexture, 0x005782C0, void* effects, const char* path, const char* name);
GAME_FN(int, __thiscall, Effects_FindTexture, 0x00578350, void* effects, uint32_t hash);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
// its own, where modes apply
GAME_FN(void, __thiscall, Offsets_Randomize_Game, 0x00536900, GlowOffset* offsets);
GAME_FN(void, __thiscall, Offsets_Drift_Game, 0x00536D20, GlowOffset* offsets);
GAME_FN(void, __thiscall, SetDefaultGlow_Game, 0x00536A60, WindowGlowManager* self);
GAME_FN(void, __thiscall, Clear_Game, 0x00536B30, WindowGlowManager* self);
GAME_FN(void, __thiscall, Glow_GetNormal_Game, 0x00536E00, WindowGlow* glow, CVector* out);
GAME_FN(void, __thiscall, Glow_GetTiltedNormal_Game, 0x00536ED0, WindowGlow* glow, CVector* out);
GAME_FN(void, __thiscall, FadeUpdate_Game, 0x00536FC0, WindowGlowManager* self, int index);
GAME_FN(void, __thiscall, RenderGlow_Game, 0x00537130, WindowGlowManager* self, int index);
GAME_FN(float, __thiscall, FrameWidth_Game, 0x00537640, void* self, const float* a, const float* b, const CVector* pos,
        const CVector* dir);
GAME_FN(CVector*, __thiscall, PullTowards_Game, 0x00537760, void* self, CVector* out, const CVector* a, const CVector* b, float k);
GAME_FN(void, __thiscall, BuildFrameQuad_Game, 0x00537820, void* self, CVector* out0, CVector* out1, CVector* out2, CVector* out3,
        const CVector* pos, const CVector* up, float height, float width, float widthScale);
GAME_FN(CVector*, __thiscall, MirrorScaled_Game, 0x00537B40, void* self, CVector* out, const CVector* corner, const CVector* center,
        float k);
GAME_FN(void, __thiscall, RenderBack_Game, 0x00537E70, WindowGlowManager* self, int index);
GAME_FN(void, __thiscall, RenderFrame_Game, 0x00538120, WindowGlowManager* self, int index);

static CVector CameraPos() {
    uint8_t* t = GetLocalTransform(GetCameraNode(cameraNode));
    return CVector{*reinterpret_cast<float*>(t + 0x24), *reinterpret_cast<float*>(t + 0x28), *reinterpret_cast<float*>(t + 0x2C)};
}

static CVector Center(const WindowGlow& g) {
    const CVector* p = g.corners;
    return CVector{(((p[0].x + p[1].x) + p[2].x) + p[3].x) * 0.25f, (((p[1].y + p[0].y) + p[2].y) + p[3].y) * 0.25f,
                   (((p[1].z + p[0].z) + p[2].z) + p[3].z) * 0.25f};
}

static float RandomRange(float lo, float hi) {
    float r = Random_GetFloat(randomGenerator);
    return lo + r * (hi - lo);
}

// 0x536900 (thiscall, on the offsets): random offsets and velocities
void __fastcall Offsets_Randomize(GlowOffset* self, re::Edx) {
    for (int i = 0; i < 9; i++) {
        GlowOffset& o = self[i];
        o.offset.x = RandomRange(-offsetRange, offsetRange);
        o.offset.y = RandomRange(-offsetRange, offsetRange);
        o.offset.z = RandomRange(-offsetRange, offsetRange);
        o.velocity.x = RandomRange(-velocityRange, velocityRange);
        o.velocity.y = RandomRange(-velocityRange, velocityRange);
        o.velocity.z = RandomRange(-velocityRange, velocityRange);
        o.unknown18[0] = 0.0f;
        o.unknown18[1] = 0.0f;
        o.unknown18[2] = 0.0f;
        o.unknown18[3] = 0.0f;
    }
}

// 0x536D20 (thiscall, on the offsets): each velocity pulled back along its offset's direction
// (by 0xAB4378), then the offset moved by it
void __fastcall Offsets_Drift(GlowOffset* self, re::Edx) {
    float pull = -driftPull;
    for (int i = 0; i < 9; i++) {
        GlowOffset& o = self[i];
        CVector direction = o.offset;
        Vector_NormaliseSafe(&direction);
        float dx = direction.x * pull;
        float dy = direction.y * pull;
        float dz = pull * direction.z;
        o.velocity.x = o.velocity.x + dx;
        o.velocity.y = o.velocity.y + dy;
        o.velocity.z = dz + o.velocity.z;
        o.offset.x = o.velocity.x + o.offset.x;
        o.offset.y = o.offset.y + o.velocity.y;
        o.offset.z = o.offset.z + o.velocity.z;
    }
}

// 0x536A60 (thiscall): glow 0 set to a fixed quad; one glow
void __fastcall SetDefaultGlow(WindowGlowManager* self, re::Edx) {
    WindowGlow& g = self->glows[0];
    uint16_t& w48 = *reinterpret_cast<uint16_t*>(&g.frame);
    w48 = uint16_t((w48 & 0xE4FF) | 0x6400);
    uint32_t* f = reinterpret_cast<uint32_t*>(&g);
    f[0] = 0x440C45F4;
    f[1] = 0xC2940E56;
    f[3] = 0x440CA5B2;
    f[2] = 0x40184189;
    f[4] = 0xC2940E56;
    f[5] = 0x40184189;
    f[6] = 0x440CA5B2;
    f[7] = 0xC2940E56;
    f[9] = 0x440C45F4;
    f[8] = 0x3F624DD3;
    g.area = 0;
    *reinterpret_cast<uint16_t*>(&g.area) |= 0xFF;
    f[10] = 0xC2940E56;
    self->count = 1;
    f[11] = 0x3F624DD3;
}

// 0x536B30 (thiscall): no area, the glows' effects reset
void __fastcall Clear(WindowGlowManager* self, re::Edx) {
    self->area = -1;
    for (int i = 0; i < self->count; i++) {
        Effect_Reset(self->glows[i].effect);
    }
}

// 0x536B70 (thiscall): a glow (not if one starts at the same first corner)
void __fastcall Add(WindowGlowManager* self, re::Edx, const CVector* p0, const CVector* p1, const CVector* p2, const CVector* p3, uint8_t area,
                    float heightScale, float widthScale, float tilt, uint8_t jitter, uint8_t pull, uint8_t frame) {
    int count = self->count;
    for (int i = 0; i < count; i++) {
        const CVector& c = self->glows[i].corners[0];
        if (p0->x == c.x && p0->y == c.y && p0->z == c.z) {
            return;
        }
    }
    WindowGlow& g = self->glows[count];
    self->count = count + 1;
    g.corners[0] = *p0;
    g.corners[1] = *p1;
    g.corners[2] = *p2;
    g.heightScale = heightScale;
    g.widthScale = widthScale;
    g.area = area;
    g.tilt = tilt;
    g.corners[3] = *p3;
    g.state = 0;
    *reinterpret_cast<uint16_t*>(&g.frame) = uint16_t(g.frame | 0x6400);
    double range = 9.0 - 0.0;
    g.offset0 = uint8_t(int(Random_GetFloat(randomGenerator) * float(range) + 0.0f));
    g.offset1 = uint8_t(int(Random_GetFloat(randomGenerator) * float(range) + 0.0f));
    uint16_t w = uint16_t(((g.flags & 0xFFFE) | (jitter & 1)) & 0xFFFD);
    g.flags = uint16_t(w | (((pull & 1) | 2) << 1));
    g.frame = frame;
}

// 0x536E00 (thiscall): the quad's normal (edges 0-1 and 0-3, normalised, crossed)
void __fastcall Glow_GetNormal(WindowGlow* self, re::Edx, CVector* out) {
    const CVector* p = self->corners;
    CVector a = {p[0].x - p[1].x, p[0].y - p[1].y, p[0].z - p[1].z};
    CVector b = {p[0].x - p[3].x, p[0].y - p[3].y, p[0].z - p[3].z};
    Vector_NormaliseSafe(&a);
    Vector_NormaliseSafe(&b);
    out->x = b.y * a.z - b.z * a.y;
    out->y = b.z * a.x - a.z * b.x;
    out->z = b.x * a.y - a.x * b.y;
}

// 0x536ED0 (thiscall): the normal tilted down by sin(tilt)
void __fastcall Glow_GetTiltedNormal(WindowGlow* self, re::Edx, CVector* out) {
    Glow_GetNormal_Game(self, out);
    double z = out->z;
    float rad = self->tilt * 3.14159274f / 180.0f;
    float s = static_cast<float>(std::sin(static_cast<double>(rad)));
    out->z = static_cast<float>(z - s);
    Vector_NormaliseSafe(out);
}

// 0x536F30 (thiscall): the constructor
WindowGlowManager* __fastcall Construct(WindowGlowManager* self, re::Edx) {
    for (int i = 0; i < 300; i++) {
        WindowGlow& g = self->glows[i];
        Effect_Construct(g.effect);
        g.heightScale = 1.0f;
        g.flags |= 7;
        g.widthScale = 1.0f;
        *reinterpret_cast<uint16_t*>(&g.frame) = uint16_t(g.frame | 0x6400);
        g.tilt = 0.0f;
        g.area = 0;
        g.state = 0;
        g.offset0 = 0;
        g.offset1 = 0;
        g.frame = 0;
    }
    Offsets_Randomize_Game(self->offsets);
    self->texBack = nullptr;
    self->texGlow = nullptr;
    self->texFrame[0] = nullptr;
    self->texFrame[1] = nullptr;
    self->area = -1;
    self->count = 0;
    SetDefaultGlow_Game(self);
    return self;
}

// 0x536FC0 (thiscall): a glow's fade stepped (in near the camera; out far away, or by
// day for the night-only ones)
void __fastcall FadeUpdate(WindowGlowManager* self, re::Edx, int index) {
    WindowGlow& g = self->glows[index];
    uint16_t& w = *reinterpret_cast<uint16_t*>(&g.frame);
    bool fadeIn = false;
    if (!IsNightTime() || !(g.flags & 1)) {
        CVector cam = CameraPos();
        float dx = g.corners[2].x - cam.x;
        float dy = g.corners[2].y - cam.y;
        float dz = g.corners[2].z - cam.z;
        float distSq = (dx * dx + dy * dy) + dz * dz;
        fadeIn = distSq <= fadeRange * fadeRange;
    }
    int step = fadeStep;
    int value = (w >> 8) & 0x7F;
    g.state = 0;
    if (fadeIn) {
        if (value <= 100 - step) {
            w = uint16_t((((w & 0xFF00) + (step << 8)) ^ w) & 0x7F00 ^ w);
        } else {
            w = uint16_t((w & 0xE4FF) | 0x6400);
        }
    } else if (value >= step) {
        w = uint16_t((((w & 0xFF00) - (step << 8)) ^ w) & 0x7F00 ^ w);
    } else {
        w = uint16_t(w & 0x80FF);
    }
    w |= 0x8000;
}

static uint32_t Grey(uint32_t a) {
    return ((a << 8 | a) << 8 | a) << 8 | a;
}

static void QuadIndices(uint16_t* idx, int base) {
    idx[0] = uint16_t(base);
    idx[1] = uint16_t(base + 3);
    idx[2] = uint16_t(base + 1);
    idx[3] = uint16_t(base + 1);
    idx[4] = uint16_t(base + 3);
    idx[5] = uint16_t(base + 2);
}

// 0x537130 (thiscall): the glow itself: its quad 1.2 times its size, pushed out 0.04
// along its normal, brighter the more it faces the camera
void __fastcall RenderGlow(WindowGlowManager* self, re::Edx, int index) {
    WindowGlow& g = self->glows[index];
    CVector cam = CameraPos();
    CVector c = Center(g);
    CVector t = {cam.x - c.x, cam.y - c.y, cam.z - c.z};
    Vector_NormaliseSafe(&t);
    CVector n;
    Glow_GetNormal_Game(&g, &n);
    float dot = n.y * t.y + n.x * t.x + n.z * t.z;
    if (dot < 0.0f) {
        dot = dot * -1.0f;
    }
    int k = 0;
    if (g.state != 4) {
        k = int((1.0f - float(int(g.state)) * 0.25f) * 50.0f);
    }
    int alpha = int(dot * float(k + 100));
    int a = alpha * (g.fade & 0x7F) / 100;
    if (a > 0xFF) {
        a = 0xFF;
    }
    int base = TempBufferVerticesStored;
    TempBufferVerticesStored = base + 4;
    uint16_t* idx = &TempBufferIndices[TempBufferIndicesStored];
    TempBufferIndicesStored += 6;
    CVector off = {n.x * 0.04f, n.y * 0.04f, n.z * 0.04f};
    uint32_t color = Grey(uint8_t(a));
    BufferVertex* v = &TempBufferVertices[base];
    static const float us[4] = {0.0f, 0.0f, 1.0f, 1.0f};
    static const float vs[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    for (int i = 0; i < 4; i++) {
        const CVector& p = g.corners[i];
        CVector d = {p.x - c.x, p.y - c.y, p.z - c.z};
        v[i].pos.x = (d.x * 1.2f + c.x) + off.x;
        v[i].pos.y = (d.y * 1.2f + c.y) + off.y;
        v[i].pos.z = (d.z * 1.2f + c.z) + off.z;
        v[i].u = us[i];
        v[i].v = vs[i];
        v[i].color = color;
    }
    QuadIndices(idx, base);
}

// 0x537640 (ecx unused): between *a and *b by how square `dir` faces the camera
// (flat), not below the smaller of them
float __fastcall FrameWidth(void*, re::Edx, const float* a, const float* b, const CVector* pos, const CVector* dir) {
    CVector cam = CameraPos();
    CVector t = {cam.x - pos->x, cam.y - pos->y, cam.z - pos->z};
    Vector_NormaliseSafe(&t);
    float dot = dir->y * t.y + dir->x * t.x + dir->z * 0.0f;
    float diff = *a - *b;
    if (dot < 0.0f) {
        dot = dot * -1.0f;
    }
    float r = (1.0f - dot) * diff + *b;
    if (*b > r && *a > r) {
        r = *b;
        if (!(*b <= *a)) {
            r = *a;
        }
    }
    return r;
}

// 0x537760 (ecx unused): a moved k back along the direction to b
CVector* __fastcall PullTowards(void*, re::Edx, CVector* out, const CVector* a, const CVector* b, float k) {
    CVector d = {b->x - a->x, b->y - a->y, b->z - a->z};
    Vector_NormaliseSafe(&d);
    float nk = k * -1.0f;
    float sx = d.x * nk;
    float sy = d.y * nk;
    float sz = nk * d.z;
    out->x = a->x + sx;
    out->y = sy + a->y;
    out->z = a->z + sz;
    return out;
}

// 0x537820 (ecx unused): a quad facing the camera about `up` at pos: out0 / out1 at
// its base (half `width` either side), out2 / out3 `height` up (scaled by widthScale)
void __fastcall BuildFrameQuad(void*, re::Edx, CVector* out0, CVector* out1, CVector* out2, CVector* out3, const CVector* pos,
                               const CVector* upDir, float height, float width, float widthScale) {
    CVector cam = CameraPos();
    CVector t = {cam.x - pos->x, cam.y - pos->y, cam.z - pos->z};
    Vector_NormaliseSafe(&t);
    struct {
        float rotate[9];
        CVector translate;
        float scale;
    } transform;
    transform.translate = *pos;
    transform.scale = 1.0f;
    CVector a = *upDir;
    Vector_NormaliseSafe(&a);
    transform.rotate[1] = a.x;
    transform.rotate[4] = a.y;
    transform.rotate[7] = a.z;
    CVector u = {-t.x, -t.y, -t.z};
    Vector_NormaliseSafe(&u);
    CVector c = {a.y * u.z - a.z * u.y, a.z * u.x - u.z * a.x, a.x * u.y - u.x * a.y};
    Vector_NormaliseSafe(&c);
    transform.rotate[0] = c.x;
    transform.rotate[3] = c.y;
    transform.rotate[6] = c.z;
    CVector d = {a.z * c.y - a.y * c.z, c.z * a.x - a.z * c.x, c.x * a.y - c.y * a.x};
    Vector_NormaliseSafe(&d);
    transform.rotate[2] = d.x;
    transform.rotate[5] = d.y;
    transform.rotate[8] = d.z;
    float half = width * 0.5f;
    float negHalf = -width * 0.5f;
    float top0[3] = {half * widthScale, height, 0.0f};
    float top1[3] = {negHalf * widthScale, height, 0.0f};
    float base0[3] = {negHalf, 0.0f, 0.0f};
    float base1[3] = {half, 0.0f, 0.0f};
    float r[3];
    float* p = NiTransform_Transform(&transform, r, base0);
    *out0 = CVector{p[0], p[1], p[2]};
    p = NiTransform_Transform(&transform, r, base1);
    *out1 = CVector{p[0], p[1], p[2]};
    p = NiTransform_Transform(&transform, r, top0);
    *out2 = CVector{p[0], p[1], p[2]};
    p = NiTransform_Transform(&transform, r, top1);
    *out3 = CVector{p[0], p[1], p[2]};
}

// 0x537B40 (ecx unused): center + (center - corner) * k
CVector* __fastcall MirrorScaled(void*, re::Edx, CVector* out, const CVector* corner, const CVector* center, float k) {
    CVector d = {center->x - corner->x, center->y - corner->y, center->z - corner->z};
    float sx = d.x * k;
    float sy = d.y * k;
    float sz = k * d.z;
    out->x = center->x + sx;
    out->y = sy + center->y;
    out->z = center->z + sz;
    return out;
}

// 0x537BD0 (thiscall): the area whose glows show
void __fastcall SetArea(WindowGlowManager* self, re::Edx, int area) {
    Clear_Game(self);
    self->area = area;
}

// the glows of the area within `radius` of `pos` switched off / on
static void Switch(WindowGlowManager* self, const CVector* pos, float radius, bool on) {
    for (int i = 0; i < self->count; i++) {
        WindowGlow& g = self->glows[i];
        uint16_t flags = g.flags;
        if (((flags & 4) != 0) == on || g.area != uint32_t(self->area)) {
            continue;
        }
        CVector c = {(((g.corners[0].x + g.corners[1].x) + g.corners[2].x) + g.corners[3].x) * 0.25f,
                     (((g.corners[0].y + g.corners[1].y) + g.corners[2].y) + g.corners[3].y) * 0.25f,
                     (((g.corners[1].z + g.corners[0].z) + g.corners[2].z) + g.corners[3].z) * 0.25f};
        float dx = c.x - pos->x;
        float dy = c.y - pos->y;
        float dz = c.z - pos->z;
        float dist = std::sqrt((dx * dx + dy * dy) + dz * dz);
        if (!(radius < dist)) {
            g.flags = on ? uint16_t(flags | 4) : uint16_t(flags & 0xFFFB);
        }
    }
}

// 0x537BF0 (thiscall): WindowGlow TurnOff
void __fastcall TurnOff(WindowGlowManager* self, re::Edx, const CVector* pos, float radius) {
    Switch(self, pos, radius, false);
}

// 0x537D00 (thiscall): WindowGlow TurnOn
void __fastcall TurnOn(WindowGlowManager* self, re::Edx, const CVector* pos, float radius) {
    Switch(self, pos, radius, true);
}

// 0x537E00 (thiscall): the area's glows faded, the effects of the unlit ones reset,
// the offsets drifting (while any glow is in the area)
void __fastcall Update(WindowGlowManager* self, re::Edx) {
    bool any = false;
    for (int i = 0; i < self->count; i++) {
        WindowGlow& g = self->glows[i];
        if (g.area == uint32_t(self->area)) {
            any = true;
            FadeUpdate_Game(self, i);
            if (g.fade & 0x7F) {
                continue;
            }
        }
        if (g.effectActive) {
            Effect_Reset(g.effect);
        }
    }
    if (any) {
        Offsets_Drift_Game(self->offsets);
    }
}

// 0x537E70 (thiscall): the back quad (mirrored through the centre, 1.5 times), dimmer
// by state, while it has faded this frame or has a state
void __fastcall RenderBack(WindowGlowManager* self, re::Edx, int index) {
    WindowGlow& g = self->glows[index];
    if ((*reinterpret_cast<uint16_t*>(&g.frame) & 0x8000) && g.state == 0) {
        return;
    }
    CVector cam = CameraPos();
    CVector c = Center(g);
    CVector t = {cam.x - c.x, cam.y - c.y, cam.z - c.z};
    Vector_NormaliseSafe(&t);
    CVector n;
    Glow_GetNormal_Game(&g, &n);
    int base = TempBufferVerticesStored;
    TempBufferVerticesStored = base + 4;
    uint16_t* idx = &TempBufferIndices[TempBufferIndicesStored];
    TempBufferIndicesStored += 6;
    float dot = n.y * t.y + n.x * t.x + n.z * t.z;
    if (dot < 0.0f) {
        dot = dot * -1.0f;
    }
    float f = float(int(g.state)) * 0.25f * dot;
    int alpha = int(f * 100.0f);
    uint32_t color = Grey(uint8_t(alpha * (g.fade & 0x7F) / 100));
    BufferVertex* v = &TempBufferVertices[base];
    static const float us[4] = {0.0f, 0.0f, 1.0f, 1.0f};
    static const float vs[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    for (int i = 0; i < 4; i++) {
        MirrorScaled_Game(self, &v[i].pos, &g.corners[i], &c, 1.5f);
        v[i].u = us[i];
        v[i].v = vs[i];
        v[i].color = color;
    }
    QuadIndices(idx, base);
}

// 0x538120 (thiscall): the frame: a camera-facing quad on the glow's (tilted) up,
// sized between its two edges by how square it faces the camera, maybe jittered and
// pulled in; drawn as two strips of quads (8 vertices)
void __fastcall RenderFrame(WindowGlowManager* self, re::Edx, int index) {
    WindowGlow& g = self->glows[index];
    if (!(0.0f < g.heightScale)) {
        return;
    }
    const CVector* p = g.corners;
    CVector cam = CameraPos();
    CVector c = Center(g);
    CVector d = {p[1].x - p[0].x, p[1].y - p[0].y, p[1].z - p[0].z};
    float w = std::sqrt((d.x * d.x + d.y * d.y) + d.z * d.z);
    d = CVector{p[1].x - p[2].x, p[1].y - p[2].y, p[1].z - p[2].z};
    float h = std::sqrt((d.x * d.x + d.y * d.y) + d.z * d.z);
    CVector t = {cam.x - c.x, cam.y - c.y, cam.z - c.z};
    Vector_NormaliseSafe(&t);
    CVector n;
    Glow_GetNormal_Game(&g, &n);
    CVector up;
    Glow_GetTiltedNormal_Game(&g, &up);
    int base = TempBufferVerticesStored;
    TempBufferVerticesStored = base + 8;
    uint16_t* idx = &TempBufferIndices[TempBufferIndicesStored];
    TempBufferIndicesStored += 0xC;
    float dot = n.y * t.y + n.x * t.x + n.z * t.z;
    if (dot < 0.0f) {
        dot = dot * -1.0f;
    }
    float f = 1.0f - dot;
    float rad = g.tilt * 3.14159274f / 180.0f;
    float cs = static_cast<float>(std::cos(static_cast<double>(rad)));
    float size = cs * (f * h) + (1.0f - f) * w;
    if (static_cast<double>(n.z) < static_cast<double>(-0.9f) && n.z > -1.1f) {
        // facing down: sized by the far edge's middle instead
        AddVectors(&t, &p[1], &p[2]);
        ScaleVector(&t, &t, 0.5f);
        SubVectors(&t, &t, &c);
        Vector_NormaliseSafe(&t);
        size = FrameWidth_Game(self, &w, &h, &c, &t);
    }
    int alpha = 0x14 - int(f * -30.0f);
    if (alpha > 0xFF) {
        alpha = 0xFF;
    }
    uint8_t a = uint8_t(alpha * (g.fade & 0x7F) / 100);
    CVector q0, q1, q2, q3;
    BuildFrameQuad_Game(self, &q0, &q1, &q2, &q3, &c, &up, g.heightScale, size, g.widthScale);
    if (g.flags & 1) {
        const CVector& o0 = self->offsets[g.offset0].offset;
        q3 = CVector{q3.x + o0.x, o0.y + q3.y, o0.z + q3.z};
        const CVector& o1 = self->offsets[g.offset1].offset;
        q2 = CVector{o1.x + q2.x, o1.y + q2.y, o1.z + q2.z};
    }
    if (g.flags & 2) {
        CVector tmp;
        PullTowards_Game(self, &tmp, &q1, &q2, 1.0f);
        q1 = tmp;
        PullTowards_Game(self, &tmp, &q0, &q3, 1.0f);
        q0 = tmp;
    }
    CVector m1 = {(q1.x + q0.x) * 0.5f, (q0.y + q1.y) * 0.5f, (q0.z + q1.z) * 0.5f};
    CVector m2 = {(q2.x + q3.x) * 0.5f, (q2.y + q3.y) * 0.5f, (q2.z + q3.z) * 0.5f};
    uint32_t color = Grey(a);
    float edgeU = g.frame == 1 ? 0.25f : 0.0f;
    BufferVertex* v = &TempBufferVertices[base];
    const CVector* pos[8] = {&q2, &q1, &m1, &m2, &q3, &q0, &m1, &m2};
    const float us[8] = {0.0f, edgeU, 0.5f, 0.5f, 0.0f, edgeU, 0.5f, 0.5f};
    const float vs[8] = {1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f};
    for (int i = 0; i < 8; i++) {
        v[i].pos = *pos[i];
        v[i].color = color;
        v[i].u = us[i];
        v[i].v = vs[i];
    }
    QuadIndices(idx, base);
    QuadIndices(idx + 6, base + 4);
}

static int TextureByName(SmallString* s, const char* name) {
    StrNCpy(s->text, name, size_t(s->size));
    Effects_LoadTexture(effectsManager, "", s->text);
    int i = Effects_FindTexture(effectsManager, HashName(s->text));
    return *reinterpret_cast<int*>(effectsManager + 0x5C + i * 12);
}

static void SetGlowStates(int zTest) {
    SetRenderState(10, 2);
    SetRenderState(11, 2);
    SetRenderState(7, 2);
    SetRenderState(6, zTest);
    SetRenderState(8, 0);
    SetRenderState(0xC, 1);
    SetRenderState(0xE, 0);
    SetRenderState(2, 1);
    SetRenderState(0x14, 1);
}

static void Flush() {
    if (TempBufferVerticesStored > 0) {
        Im3D_Transform(TempBufferVertices, TempBufferVerticesStored, nullptr, 0x19);
        Im3D_RenderIndexedPrimitive(3, TempBufferIndices, TempBufferIndicesStored, 0);
        Im3D_End();
    }
}

// 0x5387D0 (thiscall): the area's lit glows drawn (the textures found the first time).
// (The frame pass doesn't empty the buffer between its two textures, so the second
// draws the first's quads again)
void __fastcall Render(WindowGlowManager* self, re::Edx) {
    if (!self->texGlow) {
        SmallString s;
        SmallString_Construct(&s);
        self->texGlow = reinterpret_cast<void*>(TextureByName(&s, "windowglow"));
        self->texBack = reinterpret_cast<void*>(TextureByName(&s, "windowglow2"));
        self->texFrame[0] = reinterpret_cast<void*>(TextureByName(&s, "windowglow3"));
        self->texFrame[1] = reinterpret_cast<void*>(TextureByName(&s, "windowglow4"));
    }
    SetGlowStates(1);
    GetRaster(self->texGlow);
    SetRenderState(1, GetRaster(self->texGlow));
    TempBufferVerticesStored = 0;
    TempBufferIndicesStored = 0;
    for (int i = 0; i < self->count; i++) {
        WindowGlow& g = self->glows[i];
        if ((g.flags & 4) && g.area == uint32_t(self->area) && (g.fade & 0x7F)) {
            RenderGlow_Game(self, i);
        }
    }
    if (self->count > 0) {
        Flush();
    }
    SetGlowStates(1);
    TempBufferVerticesStored = 0;
    TempBufferIndicesStored = 0;
    for (uint32_t k = 0; k < 2; k++) {
        SetRenderState(1, GetRaster(self->texFrame[k]));
        for (int i = 0; i < self->count; i++) {
            WindowGlow& g = self->glows[i];
            uint16_t w48 = *reinterpret_cast<uint16_t*>(&g.frame);
            if ((g.flags & 4) && g.area == uint32_t(self->area) && (w48 & 0x7F00) && uint32_t(w48 & 0xFF) == k) {
                RenderFrame_Game(self, i);
            }
        }
        Flush();
    }
    SetGlowStates(0);
    SetRenderState(1, GetRaster(self->texBack));
    TempBufferVerticesStored = 0;
    TempBufferIndicesStored = 0;
    for (int i = 0; i < self->count; i++) {
        WindowGlow& g = self->glows[i];
        if ((g.flags & 4) && g.area == uint32_t(self->area) && (g.fade & 0x7F)) {
            RenderBack_Game(self, i);
        }
    }
    if (self->count > 0) {
        Flush();
    }
}

} // namespace windowglow17

// ---- registration ----------------------------------------------------------------------

#define WINDOWGLOW17_REVERSED(name, address, fn)                                             \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace windowglow17 {
WINDOWGLOW17_REVERSED("WindowGlow::RandomizeOffsets", 0x00536900, &Offsets_Randomize);
WINDOWGLOW17_REVERSED("WindowGlow::SetDefaultGlow", 0x00536A60, &SetDefaultGlow);
WINDOWGLOW17_REVERSED("WindowGlow::Clear", 0x00536B30, &Clear);
WINDOWGLOW17_REVERSED("WindowGlow::Add", 0x00536B70, &Add);
WINDOWGLOW17_REVERSED("WindowGlow::DriftOffsets", 0x00536D20, &Offsets_Drift);
WINDOWGLOW17_REVERSED("WindowGlow::GetNormal", 0x00536E00, &Glow_GetNormal);
WINDOWGLOW17_REVERSED("WindowGlow::GetTiltedNormal", 0x00536ED0, &Glow_GetTiltedNormal);
WINDOWGLOW17_REVERSED("WindowGlow::Construct", 0x00536F30, &Construct);
WINDOWGLOW17_REVERSED("WindowGlow::FadeUpdate", 0x00536FC0, &FadeUpdate);
WINDOWGLOW17_REVERSED("WindowGlow::RenderGlow", 0x00537130, &RenderGlow);
WINDOWGLOW17_REVERSED("WindowGlow::FrameWidth", 0x00537640, &FrameWidth);
WINDOWGLOW17_REVERSED("WindowGlow::PullTowards", 0x00537760, &PullTowards);
WINDOWGLOW17_REVERSED("WindowGlow::BuildFrameQuad", 0x00537820, &BuildFrameQuad);
WINDOWGLOW17_REVERSED("WindowGlow::MirrorScaled", 0x00537B40, &MirrorScaled);
WINDOWGLOW17_REVERSED("WindowGlow::SetArea", 0x00537BD0, &SetArea);
WINDOWGLOW17_REVERSED("WindowGlow::TurnOff", 0x00537BF0, &TurnOff);
WINDOWGLOW17_REVERSED("WindowGlow::TurnOn", 0x00537D00, &TurnOn);
WINDOWGLOW17_REVERSED("WindowGlow::Update", 0x00537E00, &Update);
WINDOWGLOW17_REVERSED("WindowGlow::RenderBack", 0x00537E70, &RenderBack);
WINDOWGLOW17_REVERSED("WindowGlow::RenderFrame", 0x00538120, &RenderFrame);
WINDOWGLOW17_REVERSED("WindowGlow::Render", 0x005387D0, &Render);
} // namespace windowglow17
