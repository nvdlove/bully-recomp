// The walkable mesh query service (one, at 0xD13150; its layout in PedNavPoint.cpp):
// the nav meshes (the collision models of the entities registered, by their +0x32 id),
// their connections (a triangle edge of one joined to one of another: both triangles'
// neighbour entries set, the second's shared vertices moved onto the first's), the
// meshes' connectivity (edges between mesh ids, searched depth first, the last 16
// answers cached), and the geometric queries: a line against a mesh's triangles, the
// point of a triangle nearest a point, walking straight across the meshes.
// 0x5E8C80..0x5E9F50.
//
// The CRT's x87 sqrt (0x85C500) is the C library's here; what the game keeps on the x87
// stack is kept in doubles, what it stores as floats is stored as floats.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "reversed.h"

namespace {

struct WqVec {
    float x, y, z;
};
struct NavPoint {
    re::Ptr32<uint8_t> col;  // +0x0
    int32_t tri;   // +0x4
    WqVec pos;     // +0x8
};
static_assert(sizeof(NavPoint) == 0x14);

// An edge between two meshes (by id)
struct ModelEdge {
    uint16_t a, b;     // +0x0 the meshes
    uint16_t c, d;     // +0x4 (the triangles joined)
    uint8_t visited;   // +0x8
    uint8_t pad9;
};
static_assert(sizeof(ModelEdge) == 10);

// A connectivity answer
struct EdgeCacheEntry {
    uint16_t a, b;     // +0x0 model indices
    uint8_t result;    // +0x4
    uint8_t pad5;
};
static_assert(sizeof(EdgeCacheEntry) == 6);

// A connection between two meshes' triangles
struct NavConnection {
    re::Ptr32<uint8_t> colA;     // +0x0
    re::Ptr32<uint8_t> colB;     // +0x4
    uint16_t triA;     // +0x8
    uint16_t triB;     // +0xA
    uint16_t edgeA;    // +0xC
    uint16_t edgeB;    // +0xE
};
static_assert(sizeof(NavConnection) == 0x10);

// A segment (see AvoidSegment.cpp)
struct WqSegment {
    float start[4];
    float end[4];
};

GAME_ARRAY(ModelEdge, wq_edges, 0x00D13150);
GAME_VAR(uint32_t, wq_edgeCount, 0x00D13470);
GAME_ARRAY(EdgeCacheEntry, wq_cache, 0x00D13474);
GAME_VAR(uint32_t, wq_cacheCount, 0x00D134D4);
GAME_ARRAY(uint8_t*, wq_navEntities, 0x00D134D8);
GAME_VAR(uint32_t, wq_navEntityCount, 0x00D1353C);
GAME_ARRAY(NavConnection, wq_connections, 0x00D13540);
GAME_VAR(uint32_t, wq_connectionCount, 0x00D13600);
inline uint8_t* const wq_connectionList = reinterpret_cast<uint8_t*>(0x00D13540);
GAME_VAR(uint8_t, wq_busy, 0x00D1314C);
GAME_VAR(uint8_t*, wq_visibleEntity, 0x00C2A8E8);
GAME_ARRAY(uint8_t*, wq_modelInfos, 0x00C67738);

GAME_FN(uint8_t*, __cdecl, WQ_GetColModel, 0x0051AF50, void* entity);
GAME_FN(void, __cdecl, WQ_CleanUpOldReference, 0x004657E0, void* entity, re::Ptr32<uint8_t>* ref);
GAME_FN(void, __cdecl, WQ_MatrixInvert, 0x00413770, const void* matrix, void* out);
GAME_FN(void, __cdecl, WQ_TransformPoints, 0x00414DB0, void* out, int count, const void* matrix, const void* in,
        int stride);
GAME_FN(uint8_t, __cdecl, WQ_LineHitsBox, 0x00564E90, const void* line, const void* min, const void* max);
GAME_FN(uint8_t, __cdecl, WQ_LineHitsTriangle, 0x00567790, const void* line, const void* vertices, const void* tri,
        void* colPoint, float* fraction);
GAME_FN(uint16_t, __thiscall, WQ_Grid_First, 0x00561850, void* grid, const void* line);
GAME_FN(uint16_t, __thiscall, WQ_Grid_Next, 0x005618A0, void* grid);
GAME_FN(void, __thiscall, WQ_ColGetVertex, 0x0056E6E0, void* col, WqVec* out, int index);
GAME_FN(void, __thiscall, WQ_ColSetVertex, 0x0056E820, void* col, int index, const WqVec* pos);
GAME_FN(void, __cdecl, WQ_PlaneFromPoints, 0x005649C0, const WqVec* a, const WqVec* b, const WqVec* c, float* plane);
GAME_FN(WqVec*, __cdecl, WQ_VectorSub, 0x00414B70, WqVec* out, const WqVec* a, const WqVec* b);
GAME_FN(WqVec*, __cdecl, WQ_VectorAdd, 0x004129A0, WqVec* out, const WqVec* a, const WqVec* b);
GAME_FN(float, __thiscall, WQ_Normalise, 0x00414E10, WqVec* v);
GAME_FN(float, __thiscall, WQ_Length2D, 0x004246B0, const float* v);
GAME_FN(void, __thiscall, WQ_RemoveConnection, 0x005E88E0, void* list, uint32_t index);
GAME_FN(void, __thiscall, WQ_Point_Set, 0x005E7970, NavPoint* self, uint8_t* col, int32_t tri);
GAME_FN(uint8_t, __thiscall, WQ_Point_IsValid, 0x005E7AF0, const NavPoint* self);
GAME_FN(int, __thiscall, WQ_Point_EdgeTo, 0x005E7B10, const NavPoint* self, const NavPoint* other);
GAME_FN(uint8_t, __thiscall, WQ_Point_IsDoorOpen, 0x005E7B90, const NavPoint* self, int edge);
GAME_FN(uint8_t, __thiscall, WQ_Point_Neighbour, 0x005E7FF0, const NavPoint* self, NavPoint* out, int edge, int flags,
        int area);
GAME_FN(uint8_t, __thiscall, WQ_Point_CheckClearance, 0x005E8280, const NavPoint* self, const WqVec* p, int fromEdge,
        int flags, int area);
GAME_FN(int, __thiscall, WQ_Point_FindExit, 0x005E8730, const NavPoint* self, const void* path, int except,
        WqVec* cross, uint8_t* blocked, int flags, int area);
GAME_FN(void, __cdecl, WQ_AddEdge, 0x005E8CC0, const ModelEdge* edge);
GAME_FN(uint8_t, __cdecl, WQ_Connected, 0x005E8ED0, int unused, int from, int to);

uint8_t TriArea(const NavPoint& p) {
    uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(p.col + 0x38);
    return (*reinterpret_cast<re::Ptr32<uint8_t>*>(data + 0x10))[p.tri * 8 + 6];
}
float SqrtAbs(double v) {
    return float(std::sqrt(double(std::fabs(float(v)))));
}

// 0x5E8C80: the nav mesh with this id (0: none)
uint8_t* __cdecl FindColModelByKey(uint32_t key) {
    for (uint32_t i = 0; i < wq_navEntityCount; i++) {
        uint8_t* col = WQ_GetColModel(wq_navEntities[i]);
        if (*reinterpret_cast<uint16_t*>(col + 0x32) == key) {
            return col;
        }
    }
    return nullptr;
}

// 0x5E8CC0: an edge between two meshes, unless it's there (either way round)
void __cdecl AddEdge(const ModelEdge* edge) {
    for (uint32_t i = 0; i < wq_edgeCount; i++) {
        const ModelEdge& e = wq_edges[i];
        if (edge->a == e.a && edge->b == e.b && edge->c == e.c && edge->d == e.d) {
            return;
        }
        if (edge->a == e.b && edge->b == e.a && edge->c == e.d && edge->d == e.c) {
            return;
        }
    }
    std::memcpy(&wq_edges[wq_edgeCount], edge, sizeof(ModelEdge));
    wq_edgeCount++;
}

// 0x5E8D60: a mesh (its collision model) unregistered: its entity, edges and
// connections (the other side's neighbour entries cleared) go, the cache is emptied
void __cdecl Unregister(uint8_t* col) {
    uint8_t* owner = *reinterpret_cast<re::Ptr32<uint8_t>*>(col + 0x3C);
    uint32_t index = 0;
    while (index < wq_navEntityCount && wq_navEntities[index] != owner) {
        index++;
    }
    if (index >= wq_navEntityCount) {
        return;
    }
    for (; index < wq_navEntityCount - 1; index++) {
        wq_navEntities[index] = wq_navEntities[index + 1];
    }
    wq_navEntityCount--;
    uint32_t id = *reinterpret_cast<uint16_t*>(col + 0x32);
    for (uint32_t i = 0; i < wq_edgeCount; i++) {
        if (wq_edges[i].a != id && wq_edges[i].b != id) {
            continue;
        }
        for (uint32_t j = i; j < wq_edgeCount - 1; j++) {
            std::memcpy(&wq_edges[j], &wq_edges[j + 1], sizeof(ModelEdge));
        }
        wq_edgeCount--;
        i--;
    }
    for (uint32_t i = 0; i < wq_connectionCount; i++) {
        NavConnection& c = wq_connections[i];
        if (c.colA == col) {
            uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(c.colB + 0x38);
            (*reinterpret_cast<re::Ptr32<int32_t>*>(data + 0x18))[c.triB * 3 + c.edgeB] = -1;
        } else if (c.colB == col) {
            uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(c.colA + 0x38);
            (*reinterpret_cast<re::Ptr32<int32_t>*>(data + 0x18))[c.triA * 3 + c.edgeA] = -1;
        } else {
            continue;
        }
        WQ_RemoveConnection(wq_connectionList, i);
        i--;
    }
    wq_cacheCount = 0;
}

// 0x5E8ED0: is `to` reachable from `from` over the unvisited edges (depth first)?
uint8_t __cdecl Connected(int unused, int from, int to) {
    for (uint32_t i = 0; i < wq_edgeCount; i++) {
        ModelEdge& e = wq_edges[i];
        if (e.visited) {
            continue;
        }
        if (e.a == uint16_t(from)) {
            if (e.b == uint16_t(to)) {
                return 1;
            }
            e.visited = 1;
            if (WQ_Connected(unused, e.b, to)) {
                return 1;
            }
        }
        if (e.b == uint16_t(from)) {
            if (e.a == uint16_t(to)) {
                return 1;
            }
            e.visited = 1;
            if (WQ_Connected(unused, e.a, to)) {
                return 1;
            }
        }
    }
    return 0;
}

// 0x5E8F70: two meshes' triangle edges joined
void __cdecl AddConnection(const NavConnection* c) {
    uint8_t* dataA = *reinterpret_cast<re::Ptr32<uint8_t>*>(c->colA + 0x38);
    uint8_t* dataB = *reinterpret_cast<re::Ptr32<uint8_t>*>(c->colB + 0x38);
    uint32_t idA = *reinterpret_cast<uint16_t*>(c->colA + 0x32);
    uint32_t idB = *reinterpret_cast<uint16_t*>(c->colB + 0x32);
    (*reinterpret_cast<re::Ptr32<uint32_t>*>(dataA + 0x18))[c->triA * 3 + c->edgeA] = (idB << 16) | c->triB;
    (*reinterpret_cast<re::Ptr32<uint32_t>*>(dataB + 0x18))[c->triB * 3 + c->edgeB] = (idA << 16) | c->triA;
    const uint16_t* triA = reinterpret_cast<const uint16_t*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(dataA + 0x10) + c->triA * 8);
    const uint16_t* triB = reinterpret_cast<const uint16_t*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(dataB + 0x10) + c->triB * 8);
    int nextA = c->edgeA < 2 ? c->edgeA + 1 : 0;
    WqVec first;
    WqVec second;
    WQ_ColGetVertex(c->colA, &first, triA[c->edgeA]);
    WQ_ColGetVertex(c->colA, &second, triA[nextA]);
    int nextB = c->edgeB < 2 ? c->edgeB + 1 : 0;
    WQ_ColSetVertex(c->colB, triB[c->edgeB], &second);
    WQ_ColSetVertex(c->colB, triB[nextB], &first);
    wq_connections[wq_connectionCount] = *c;
    wq_connectionCount++;
    ModelEdge edge = {uint16_t(idA), uint16_t(idB), c->triA, c->triB, 0, 0};
    WQ_AddEdge(&edge);
}

// 0x5E90C0: are two models' meshes connected (the same one, or through the edges)? The
// last 16 answers are kept, newest first.
uint8_t __cdecl ModelsConnected(int modelA, int modelB) {
    if (modelA == modelB) {
        return 1;
    }
    uint8_t* colA = *reinterpret_cast<re::Ptr32<uint8_t>*>(wq_modelInfos[modelA] + 0xC);
    uint8_t* colB = *reinterpret_cast<re::Ptr32<uint8_t>*>(wq_modelInfos[modelB] + 0xC);
    for (uint32_t i = 0; i < wq_cacheCount; i++) {
        const EdgeCacheEntry& e = wq_cache[i];
        if ((e.a == uint32_t(modelA) && e.b == uint32_t(modelB)) || (e.a == uint32_t(modelB) && e.b == uint32_t(modelA))) {
            return e.result;
        }
    }
    for (uint32_t i = 0; i < wq_edgeCount; i++) {
        wq_edges[i].visited = 0;
    }
    uint8_t result = WQ_Connected(*reinterpret_cast<uint16_t*>(colA + 0x30), *reinterpret_cast<uint16_t*>(colA + 0x32),
                                  *reinterpret_cast<uint16_t*>(colB + 0x32));
    uint32_t count = wq_cacheCount;
    if (count == 16) {
        count = 15;
    }
    count++;
    wq_cacheCount = count;
    for (uint32_t i = count - 1; i > 0; i--) {
        wq_cache[i] = wq_cache[i - 1];
    }
    wq_cache[0].a = uint16_t(modelA);
    wq_cache[0].b = uint16_t(modelB);
    wq_cache[0].result = result;
    return result;
}

// 0x5E9200: a line (start, end: 16 bytes apart) against a mesh in its owner's space,
// shortened to *fraction first: the nearest triangle hit (trying *tri first), *fraction
// scaled to it
uint8_t __cdecl LineHitsMesh(const float* line, const void* matrix, uint8_t* col, float* fraction, int32_t* tri) {
    uint8_t colPoint[0x40] = {};
    *reinterpret_cast<float*>(colPoint + 0x24) = 3.4028235e+38f;
    re::Ptr32<uint8_t>& ref = *reinterpret_cast<re::Ptr32<uint8_t>*>(colPoint + 0xC);
    uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(col + 0x38);
    if (!data) {
        return 0;
    }
    alignas(16) uint8_t inverse[0x48];
    *reinterpret_cast<uint32_t*>(inverse + 0x40) = 0;
    WQ_MatrixInvert(matrix, inverse);
    float local[8];
    WQ_TransformPoints(local, 2, inverse, line, 0x10);
    if (1.0f > *fraction) {
        float dx = local[4] - local[0];
        float dy = local[5] - local[1];
        float dz = local[6] - local[2];
        float t = *fraction;
        float fx = dx * t;
        float fy = dy * t;
        float fz = t * dz;
        local[4] = fx + local[0];
        local[5] = local[1] + fy;
        local[6] = local[2] + fz;
    }
    if (!WQ_LineHitsBox(local, col + 0x10, col + 0x20)) {
        if (ref) {
            WQ_CleanUpOldReference(ref, &ref);
        }
        return 0;
    }
    float best = 1.0f;
    void* vertices = *reinterpret_cast<re::Ptr32<void>*>(data + 0xC);
    uint8_t* tris = *reinterpret_cast<re::Ptr32<uint8_t>*>(data + 0x10);
    int32_t first = *tri;
    bool hit = first != -1 && first < *reinterpret_cast<int16_t*>(data + 0x1C)
               && WQ_LineHitsTriangle(local, vertices, tris + first * 8, colPoint, &best);
    if (!hit) {
        void* grid = *reinterpret_cast<re::Ptr32<void>*>(data + 0x14);
        for (uint32_t i = WQ_Grid_First(grid, local); i != 0xFFFF; i = WQ_Grid_Next(grid)) {
            if (WQ_LineHitsTriangle(local, vertices, tris + i * 8, colPoint, &best)) {
                *tri = int32_t(i);
            }
        }
    }
    if (best < 1.0f) {
        *fraction = best * *fraction;
        if (ref) {
            WQ_CleanUpOldReference(ref, &ref);
        }
        return 1;
    }
    if (ref) {
        WQ_CleanUpOldReference(ref, &ref);
    }
    return 0;
}

// 0x5E94A0: the point of a triangle nearest p (flat: in the triangle's own u/v frame
// from its first edge; inside, at the plane's height), nudged 0.001 towards the
// triangle's centre; the flat distance
float __cdecl ClosestOnTriangle(const float* p, const uint8_t* vertices, const uint16_t* tri, WqVec* out) {
    WqVec v[3];
    for (int i = 0; i < 3; i++) {
        const int16_t* s = reinterpret_cast<const int16_t*>(vertices + tri[i] * 6);
        v[i] = {float(s[0] * 0.0078125), float(s[1] * 0.0078125), float(s[2] * 0.0078125)};
    }
    const WqVec& v0 = v[0];
    const WqVec& v1 = v[1];
    const WqVec& v2 = v[2];
    float px = p[0];
    float py = p[1];
    float ax = v1.x - v0.x;
    float ay = v1.y - v0.y;
    float len = SqrtAbs(double(ax) * ax + double(ay) * ay);
    float inv = float(1.0 / len);
    float ux = float(double(inv) * ax);
    float uy = float(double(inv) * ay);
    float nux = -ux;
    float cx = v2.x - v0.x;
    float cy = v2.y - v0.y;
    float cu = float(double(uy) * cy + double(cx) * ux);
    float cv = float(double(cx) * uy + double(cy) * nux);
    float dx = px - v0.x;
    float dy = py - v0.y;
    float qu = float(double(ux) * dx + double(dy) * uy);
    float qv = float(double(dx) * uy + double(dy) * nux);
    double e1 = double(qv) * len - double(qu) * 0.0;
    bool inA = e1 >= 0.0;
    double e2 = double(qu) * cv - double(qv) * cu;
    bool inB = e2 >= 0.0;
    double d1 = double(qv) - 0.0;
    double d2 = double(cu) - len;
    double d3 = double(qu) - len;
    double w = double(cv) - 0.0;
    double e3 = d2 * d1 - d3 * w;
    int count = (inA ? 1 : 0) + (inB ? 1 : 0) + (e3 >= 0.0 ? 1 : 0);
    float dist = 0.0f;
    if (count == 1) {
        // (a corner: the one outside both other edges)
        const WqVec& corner = inA ? v2 : (inB ? v1 : v0);
        *out = corner;
        float ex = px - corner.x;
        float ey = py - corner.y;
        if (inA) {
            dist = SqrtAbs(double(ey) * ey + double(ex) * ex);
        } else {
            float e[2] = {ex, ey};
            dist = WQ_Length2D(e);
        }
    } else if (count == 2) {
        // (an edge: its start, end, or the point along it)
        WqVec start;
        WqVec end;
        float eu;
        float ev;
        float pu = qu;
        float pv = qv;
        if (!inA) {
            start = v0;
            end = v1;
            eu = len;
            ev = 0.0f;
        } else if (!inB) {
            start = v0;
            end = v2;
            eu = cu;
            ev = cv;
        } else {
            start = v1;
            end = v2;
            eu = float(d2);
            ev = float(w);
            pu = float(d3);
            pv = float(d1);
        }
        float lengthSq = float(double(eu) * eu + double(ev) * ev);
        float t = float((double(pv) * ev + double(pu) * eu) / lengthSq);
        if (!(t > 0.0f) && !std::isnan(t)) {
            *out = start;
            float ex = px - start.x;
            float ey = py - start.y;
            dist = SqrtAbs(double(ey) * ey + double(ex) * ex);
        } else if (!(1.0f > t) && !std::isnan(t)) {
            *out = end;
            float e[2] = {px - end.x, py - end.y};
            dist = WQ_Length2D(e);
        } else {
            WqVec d;
            WQ_VectorSub(&d, &end, &start);
            WqVec f = {float(double(t) * d.x), float(double(d.y) * t), float(double(d.z) * t)};
            WqVec r;
            *out = *WQ_VectorAdd(&r, &start, &f);
            double cross = double(eu) * pv - double(ev) * pu;
            float length = SqrtAbs(lengthSq);
            dist = std::fabs(float(cross / length));
        }
    } else if (count == 3) {
        // (inside: the plane's height)
        float plane[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        WQ_PlaneFromPoints(&v0, &v1, &v2, plane);
        double h = (double(plane[2]) * v0.z - ((double(p[0]) - v0.x) * plane[0] + (double(p[1]) - v0.y) * plane[1]))
                   / plane[2];
        out->x = p[0];
        out->y = p[1];
        out->z = float(h);
        dist = 0.0f;
    }
    WqVec sum;
    WQ_VectorAdd(&sum, &v0, &v1);
    WqVec total;
    WQ_VectorAdd(&total, &sum, &v2);
    WqVec centre = {float(total.x * double(0.33333334f)), float(total.y * double(0.33333334f)),
                    float(total.z * double(0.33333334f))};
    WqVec toCentre;
    WQ_VectorSub(&toCentre, &centre, out);
    WQ_Normalise(&toCentre);
    WqVec nudge = {float(toCentre.x * double(0.001f)), float(toCentre.y * double(0.001f)),
                   float(double(0.001f) * toCentre.z)};
    out->x = nudge.x + out->x;
    out->y = out->y + nudge.y;
    out->z = nudge.z + out->z;
    return dist;
}

// 0x5E9C30: can one walk straight from `from` to `to` (on to twice the way, the path's
// line) over the meshes' triangles, the flags and area allowing (and, with check, room
// at the end)? A door crossed on the way into `door`.
uint8_t __cdecl WalkStraight(const NavPoint* from, const NavPoint* to, int flags, int area, int check, NavPoint* door) {
    if (uint8_t(check) != 0) {
        if (!WQ_Point_CheckClearance(to, &to->pos, -1, uint8_t(flags), uint8_t(area))) {
            return 0;
        }
    }
    NavPoint next = {nullptr, -1, {0.0f, 0.0f, 0.0f}};
    NavPoint cur = *from;
    float dx = to->pos.x - from->pos.x;
    float dy = to->pos.y - from->pos.y;
    float dz = to->pos.z - from->pos.z;
    WqSegment path = {{from->pos.x, from->pos.y, from->pos.z, 0.0f},
                      {to->pos.x + dx, to->pos.y + dy, to->pos.z + dz, 0.0f}};
    WqVec cross = {0.0f, 0.0f, 0.0f};
    if (door) {
        WQ_Point_Set(door, nullptr, -1);
    }
    uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(from->col + 0x38);
    const uint8_t* triData = *reinterpret_cast<re::Ptr32<uint8_t>*>(data + 0x10) + from->tri * 8;
    if (!triData || ((~(triData[7] >> 2)) & 1) == 0) {
        return 0;
    }
    if (uint8_t(area) != 0 && triData[6] != uint8_t(area)) {
        return 0;
    }
    int except = -1;
    for (;;) {
        if (to->tri == cur.tri && to->col == cur.col) {
            return 1;
        }
        uint8_t blocked;
        int edge = WQ_Point_FindExit(&cur, &path, except, &cross, &blocked, flags, area);
        if (edge == -1) {
            return 0;
        }
        path.start[0] = cross.x;
        path.start[1] = cross.y;
        path.start[2] = cross.z;
        if (!WQ_Point_Neighbour(&cur, &next, edge, flags, area)) {
            return 0;
        }
        except = WQ_Point_EdgeTo(&next, &cur);
        if (door && TriArea(next) == 5 && TriArea(cur) != 5 && !WQ_Point_IsValid(door)
            && WQ_Point_IsDoorOpen(&next, except)) {
            door->col = next.col;
            door->tri = next.tri;
            door->pos = cross;
        }
        cur = next;
    }
}

// 0x5E9F50: walking from `from` towards a position: the point reached (out) when
// nothing blocks the way (and, with check, there's room there). Not while the service
// is busy.
uint8_t __cdecl WalkToward(const NavPoint* from, const WqVec* to, int flags, NavPoint* out, int area, int check,
                           NavPoint* door) {
    if (wq_busy) {
        return 0;
    }
    NavPoint cur = *from;
    WqSegment path = {{from->pos.x, from->pos.y, from->pos.z, 0.0f}, {to->x, to->y, to->z, 0.0f}};
    NavPoint next = {nullptr, -1, {0.0f, 0.0f, 0.0f}};
    WqVec cross = {0.0f, 0.0f, 0.0f};
    if (door) {
        WQ_Point_Set(door, nullptr, -1);
    }
    uint8_t blocked = 0;
    int edge = WQ_Point_FindExit(&cur, &path, -1, &cross, &blocked, flags, area);
    while (edge != -1) {
        path.start[0] = cross.x;
        path.start[1] = cross.y;
        path.start[2] = cross.z;
        if (!WQ_Point_Neighbour(&cur, &next, edge, flags, area)) {
            return 0;
        }
        int back = WQ_Point_EdgeTo(&next, &cur);
        if (door && TriArea(next) == 5 && TriArea(cur) != 5 && !WQ_Point_IsValid(door)
            && WQ_Point_IsDoorOpen(&next, back)) {
            door->col = next.col;
            door->tri = next.tri;
            door->pos = cross;
        }
        cur = next;
        edge = WQ_Point_FindExit(&cur, &path, back, &cross, &blocked, flags, area);
    }
    if (blocked) {
        return 0;
    }
    cur.pos = *to;
    if (uint8_t(check) != 0 && !WQ_Point_CheckClearance(&cur, &cur.pos, -1, uint8_t(flags), uint8_t(area))) {
        return 0;
    }
    if (out) {
        *out = cur;
    }
    return 1;
}

#define WALKQ_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(WalkQOriginal_, __LINE__);                                        \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(WalkQOriginal_, __LINE__), \
             re::Mode::Original)

WALKQ_REVERSED("WalkableMeshQueryService::FindColModelByKey", 0x005E8C80, &FindColModelByKey);
WALKQ_REVERSED("WalkableMeshQueryService::AddEdge", 0x005E8CC0, &AddEdge);
WALKQ_REVERSED("WalkableMeshQueryService::Unregister", 0x005E8D60, &Unregister);
WALKQ_REVERSED("WalkableMeshQueryService::Connected", 0x005E8ED0, &Connected);
WALKQ_REVERSED("WalkableMeshQueryService::AddConnection", 0x005E8F70, &AddConnection);
WALKQ_REVERSED("WalkableMeshQueryService::ModelsConnected", 0x005E90C0, &ModelsConnected);
WALKQ_REVERSED("WalkableMeshQueryService::LineHitsMesh", 0x005E9200, &LineHitsMesh);
WALKQ_REVERSED("WalkableMeshQueryService::ClosestOnTriangle", 0x005E94A0, &ClosestOnTriangle);
WALKQ_REVERSED("WalkableMeshQueryService::WalkStraight", 0x005E9C30, &WalkStraight);
WALKQ_REVERSED("WalkableMeshQueryService::WalkToward", 0x005E9F50, &WalkToward);

} // namespace
