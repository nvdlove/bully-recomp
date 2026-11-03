// CSkidmarks (0x5276F0..0x5284A0): 32 marks (0xC9E358, 0x114 each) of up to 16 points,
// registered by id while a wheel skids, faded out once they stop being updated, drawn
// as one triangle strip through the render buffer ("particleskid" in particle.txd).
//
// Found here: a point's fourth float is its half width. Each mark adds 2n + 5 vertices to
// the strip: two degenerate ones at its start, a pair per point (u 0 / 1, v the point's
// index * 5.01), a degenerate one at its end. The first pair has no alpha, the last half.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "reversed.h"

namespace skidmarks17 {

using col::CVector;

struct CSkidmark {
    struct Point {
        float x, y, z;
        float width;
    } points[16];             // +0
    uint32_t id;              // +0x100
    uint32_t lastUpdate;      // +0x104
    uint32_t fadeStart;       // +0x108
    uint32_t fadeEnd;         // +0x10C
    uint8_t numPoints;        // +0x110 the last point's index
    uint8_t type;             // +0x111 its color (0..3)
    uint8_t state;            // +0x112 0 free, 1 being made, 2 fading
    uint8_t updated;          // +0x113 registered this frame
};
static_assert(offsetof(CSkidmark, id) == 0x100);
static_assert(offsetof(CSkidmark, numPoints) == 0x110);
static_assert(sizeof(CSkidmark) == 0x114);

struct BufferVertex {
    CVector pos;              // +0
    uint32_t unknownC[3];     // +0xC
    uint32_t color;           // +0x18
    float u;                  // +0x1C
    float v;                  // +0x20
};
static_assert(sizeof(BufferVertex) == 0x24);

GAME_ARRAY(CSkidmark, aSkidmarks, 0x00C9E358);
GAME_VAR(void*, skidTexture, 0x00C9E350);
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);
GAME_ARRAY(BufferVertex, TempBufferVertices, 0x00BD7800);
GAME_VAR(int32_t, TempBufferVerticesStored, 0x00CA2AE8);
GAME_VAR(int32_t, TempBufferIndicesStored, 0x00CA2AEC);
// the colors by type (only their low three bytes are used)
GAME_VAR(uint32_t, skidColor0, 0x0091848C);
GAME_VAR(uint32_t, skidColor1, 0x00918494);
GAME_VAR(uint32_t, skidColor2, 0x00918490);
GAME_VAR(uint32_t, skidColor3, 0x00918498);

GAME_FN(void, __cdecl, Txd_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, Txd_PopCurrentTxd, 0x005F0AA0);
GAME_FN(int, __cdecl, Txd_FindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(void, __cdecl, Txd_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void*, __cdecl, Txd_FindTexture, 0x005F1770, const char* name, const char* mask);
GAME_FN(void, __cdecl, Texture_Destroy, 0x005F0F90, void* texture);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(int, __cdecl, GetRaster, 0x0051A120, void* texture);
GAME_FN(void*, __cdecl, Im3D_Transform, 0x005EFAF0, void* vertices, int count, void* transform, uint32_t flags);
GAME_FN(void, __cdecl, Im3D_RenderPrimitive, 0x005F09D0, int type);
GAME_FN(uint8_t, __cdecl, Im3D_End, 0x005EFB20);
GAME_FN(float, __thiscall, Vector_Normalise, 0x004294A0, CVector* v);

// 0x5276F0: CSkidmarks::Init
void __cdecl Init() {
    Txd_PushCurrentTxd();
    Txd_SetCurrentTxd(Txd_FindTxdSlot("particle", 0));
    skidTexture = Txd_FindTexture("particleskid", nullptr);
    Txd_PopCurrentTxd();
    for (int i = 0; i < 32; i++) {
        aSkidmarks[i].state = 0;
        aSkidmarks[i].updated = 0;
    }
}

// 0x527750: CSkidmarks::Shutdown
void __cdecl Shutdown() {
    Texture_Destroy(skidTexture);
    skidTexture = nullptr;
}

// 0x527770: CSkidmarks::Update: marks not registered this frame start fading (the
// longer, the slower); faded ones are freed
void __cdecl Update() {
    uint32_t now = timeInMs;
    for (int i = 0; i < 32; i++) {
        CSkidmark& mark = aSkidmarks[i];
        if (mark.state == 1) {
            if (!mark.updated) {
                mark.state = 2;
                if (mark.numPoints < 4) {
                    mark.fadeStart = now + 1250;
                    mark.fadeEnd = now + 2500;
                } else if (mark.numPoints < 9) {
                    mark.fadeStart = now + 2500;
                    mark.fadeEnd = now + 5000;
                } else {
                    mark.fadeStart = now + 5000;
                    mark.fadeEnd = now + 10000;
                }
            }
        } else if (mark.state == 2 && now > mark.fadeEnd) {
            mark.state = 0;
        }
        mark.updated = 0;
    }
}

// a point's pair of edge positions (d: the direction along the mark, normalised)
static void EdgePair(const CSkidmark::Point& p, CVector d, CVector* a, CVector* b) {
    Vector_Normalise(&d);
    float perpX = p.width * d.y;
    float perpY = -(p.width * d.x);
    *a = CVector{p.x + perpX, p.y + perpY, p.z + 0.0f};
    *b = CVector{p.x - perpX, p.y - perpY, p.z - 0.0f};
}

static void SetVertex(BufferVertex& v, const CVector& pos, uint32_t color, float u, float tv) {
    v.pos = pos;
    v.color = color;
    v.u = u;
    v.v = tv;
}

// 0x527800: CSkidmarks::Render
void __cdecl Render() {
    SetRenderState(8, 0);
    SetRenderState(0xC, 1);
    SetRenderState(10, 5);
    SetRenderState(11, 6);
    SetRenderState(1, GetRaster(skidTexture));
    int stored = 0;
    TempBufferVerticesStored = 0;
    TempBufferIndicesStored = 0;
    for (int m = 0; m < 32; m++) {
        CSkidmark& mark = aSkidmarks[m];
        if (mark.state == 0 || mark.numPoints == 0) {
            continue;
        }
        uint32_t colorWord = 0;
        switch (mark.type) {
        case 0: colorWord = skidColor0; break;
        case 1: colorWord = skidColor1; break;
        case 2: colorWord = skidColor2; break;
        case 3: colorWord = skidColor3; break;
        }
        uint32_t c0 = colorWord & 0xFF;
        uint32_t c1 = (colorWord >> 8) & 0xFF;
        uint32_t c2 = (colorWord >> 16) & 0xFF;
        uint32_t rgb = c0 << 16 | c1 << 8 | c2;
        int alpha;
        if (mark.state == 1 || timeInMs < mark.fadeStart) {
            alpha = 0xFF;
        } else {
            alpha = uint16_t(((mark.fadeEnd - timeInMs) * 0xFF) / (mark.fadeEnd - mark.fadeStart));
        }
        int n = mark.numPoints;
        BufferVertex* v = &TempBufferVertices[stored];
        TempBufferVerticesStored = stored + n * 2 + 5;

        // the first point (no alpha), after two degenerate vertices
        const CSkidmark::Point* p = mark.points;
        CVector dPrev = {p[1].x - p[0].x, p[1].y - p[0].y, p[1].z - p[0].z};
        CVector a, b;
        EdgePair(p[0], CVector{dPrev.x + dPrev.x, dPrev.y + dPrev.y, dPrev.z + dPrev.z}, &a, &b);
        v[0].pos = a;
        v[1].pos = a;
        SetVertex(v[2], a, rgb, 0.0f, 0.0f);
        SetVertex(v[3], b, rgb, 1.0f, 0.0f);
        v += 4;

        uint32_t color = uint32_t(int(int16_t(alpha))) << 24 | rgb;
        int i = 1;
        for (; i < n; i++) {
            CVector dNext = {p[i + 1].x - p[i].x, p[i + 1].y - p[i].y, p[i + 1].z - p[i].z};
            EdgePair(p[i], CVector{dNext.x + dPrev.x, dNext.y + dPrev.y, dNext.z + dPrev.z}, &a, &b);
            float tv = float(i) * 5.01f;
            SetVertex(v[0], a, color, 0.0f, tv);
            SetVertex(v[1], b, color, 1.0f, tv);
            v += 2;
            dPrev = dNext;
        }

        // the last point (half alpha, none while fading), then a degenerate vertex
        EdgePair(p[i], CVector{dPrev.x + dPrev.x, dPrev.y + dPrev.y, dPrev.z + dPrev.z}, &a, &b);
        int8_t lastAlpha = mark.state != 2 ? int8_t(alpha >> 1) : 0;
        uint32_t lastColor = uint32_t(int(lastAlpha)) << 24 | rgb;
        float tv = float(i) * 5.01f;
        SetVertex(v[0], a, lastColor, 0.0f, tv);
        SetVertex(v[1], b, lastColor, 1.0f, tv);
        v[2].pos = b;
        stored = TempBufferVerticesStored;
    }
    if (stored && Im3D_Transform(TempBufferVertices, stored, nullptr, 0x19)) {
        Im3D_RenderPrimitive(4);
        Im3D_End();
    }
    SetRenderState(0xC, 0);
    SetRenderState(8, 1);
    SetRenderState(6, 1);
}

static float WidthForKind(uint8_t kind) {
    if (kind == 1) {
        return 0.04f;
    }
    if (kind == 2) {
        return 0.125f;
    }
    return 0.037037037f;
}

// 0x5280C0: CSkidmarks::RegisterOne: the mark with this id gets a new point (every
// 100 ms, once it has moved 0.16; its width from how square to the mark `dir` is) or
// its last point moved; a new mark otherwise (in the last free slot). Full marks fade.
void __cdecl RegisterOne(uint32_t id, const CVector* pos, float dirX, float dirY, uint8_t type, uint8_t kind) {
    uint32_t now = timeInMs;
    CSkidmark* freeMark = nullptr;
    int i = 0;
    for (; i < 32; i++) {
        CSkidmark& mark = aSkidmarks[i];
        if (mark.state == 1 && mark.id == id) {
            break;
        }
        if (mark.state == 0) {
            freeMark = &mark;
        }
    }
    if (i < 32) {
        CSkidmark& mark = aSkidmarks[i];
        uint8_t n = mark.numPoints;
        mark.updated = 1;
        bool add = now - mark.lastUpdate > 100;
        if (n != 0) {
            const CSkidmark::Point& last = mark.points[n];
            const CSkidmark::Point& prev = mark.points[n - 1];
            float dx = prev.x - last.x;
            float dy = prev.y - last.y;
            float dz = prev.z - last.z;
            float distSq = dy * dy + dx * dx + dz * dz;
            add = add && static_cast<double>(distSq) > static_cast<double>(0.025f);
        }
        if (!add) {
            CSkidmark::Point& last = mark.points[n];
            last.x = pos->x;
            last.y = pos->y;
            last.z = pos->z + 0.05f;
            return;
        }
        mark.lastUpdate = now;
        if (n >= 15) {
            mark.fadeEnd = now + 10000;
            mark.state = 2;
            mark.fadeStart = now + 5000;
            return;
        }
        mark.numPoints = uint8_t(n + 1);
        CSkidmark::Point& point = mark.points[mark.numPoints];
        point.x = pos->x;
        point.y = pos->y;
        point.z = pos->z + 0.05f;
        const CSkidmark::Point& prev = mark.points[mark.numPoints - 1];
        CVector d = {point.x - prev.x, point.y - prev.y, 0.0f};
        Vector_Normalise(&d);
        CVector dir = {dirX, dirY, 0.0f};
        Vector_Normalise(&dir);
        float dot = std::fabs(dir.y * d.y + dir.x * d.x + dir.z * d.z);
        float width = WidthForKind(kind);
        point.width = ((1.0f - dot) + 1.0f) * width;
        return;
    }
    if (freeMark) {
        freeMark->state = 1;
        freeMark->id = id;
        freeMark->points[0].x = pos->x;
        freeMark->points[0].y = pos->y;
        freeMark->points[0].z = pos->z + 0.05f;
        freeMark->points[0].width = WidthForKind(kind);
        freeMark->updated = 1;
        freeMark->numPoints = 0;
        freeMark->lastUpdate = now - 1000;
        freeMark->type = type;
    }
}

} // namespace skidmarks17

// ---- registration ----------------------------------------------------------------------

#define SKIDMARKS17_REVERSED(name, address, fn)                                              \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace skidmarks17 {
SKIDMARKS17_REVERSED("CSkidmarks::Init", 0x005276F0, &Init);
SKIDMARKS17_REVERSED("CSkidmarks::Shutdown", 0x00527750, &Shutdown);
SKIDMARKS17_REVERSED("CSkidmarks::Update", 0x00527770, &Update);
SKIDMARKS17_REVERSED("CSkidmarks::Render", 0x00527800, &Render);
SKIDMARKS17_REVERSED("CSkidmarks::RegisterOne", 0x005280C0, &RegisterOne);
} // namespace skidmarks17
