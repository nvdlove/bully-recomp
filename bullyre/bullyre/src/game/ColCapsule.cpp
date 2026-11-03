// Capsule collision (0x561A10..0x564080): a capsule (the camera's: CameraCapsule in
// FightCamera.cpp) against spheres, boxes, triangles and whole collision models, the
// capsule query over the world's sectors (FightCamera, FollowCamera4), and the swept
// sphere's (FollowCamera3b, FollowCamera4) that keeps the nearest hit.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "reversed.h"

namespace {

using col::CVector;

struct Capsule {
    CVector middle;     // +0
    CVector dir;        // +0xC a to b, normalised
    CVector a;          // +0x18
    CVector b;          // +0x24
    float length;       // +0x30
    float radius;       // +0x34
};
static_assert(sizeof(Capsule) == 0x38);

// CColPoint
struct ColPoint {
    CVector point;      // +0
    re::Ptr32<CEntity> entity;    // +0xC (a registered reference)
    CVector normal;     // +0x10
    uint8_t unknown1C[4];
    uint8_t surfaceA;   // +0x20
    uint8_t pieceA;     // +0x21
    uint8_t surfaceB;   // +0x22
    uint8_t pieceB;     // +0x23
    float depth;        // +0x24
};
static_assert(sizeof(ColPoint) == 0x28);

// CColBox
struct ColBox {
    float min[4];       // +0
    float max[4];       // +0x10
    uint8_t surface;    // +0x20
    uint8_t piece;      // +0x21
    uint8_t pad[2];
};
static_assert(sizeof(ColBox) == 0x24);

// a triangle as a corner and two edges (0x562990's)
struct TriangleEdges {
    CVector origin;
    CVector edge1;
    CVector edge2;
};

// a segment as a start and a direction
struct Segment {
    CVector start;
    CVector dir;
};

// the capsule query (0x64 bytes: FightCamera.cpp's CollisionQuery, FollowCamera4.cpp's
// CapsuleQuery)
struct CapsuleQuery {
    Capsule capsule;       // +0x00
    uint32_t flags;        // +0x38 the sector lists (1..0x10); 0x80: skip some entities
                           // (and collect some, below); 0x200: only models with +0x18
    CVector centre;        // +0x3C the bounding sphere
    float radius;          // +0x48
    re::Ptr32<ColPoint> hits;        // +0x4C
    int32_t numHits;       // +0x50
    int32_t maxHits;       // +0x54
    re::Ptr32<re::Ptr32<CEntity>> entities;    // +0x58 (with 0x80: the dynamic objects near the end collected here)
    int32_t maxEntities;   // +0x5C
    int32_t numEntities;   // +0x60
};
static_assert(offsetof(CapsuleQuery, flags) == 0x38);
static_assert(offsetof(CapsuleQuery, hits) == 0x4C);
static_assert(sizeof(CapsuleQuery) == 0x64);

// the swept sphere (0x48 bytes: FollowCamera3b.cpp's Sweep)
struct SweepQuery {
    re::Ptr32<CEntity> excluded;                       // +0x00 (a ped's: 0x4762F0)
    CVector from;                            // +0x04
    CVector to;                              // +0x10
    float radius;                            // +0x1C
    uint32_t flags;                          // +0x20 the sector lists; 0x80: the filter applies
    CVector centre;                          // +0x24 the bounding sphere
    float boundRadius;                       // +0x30
    re::Ptr32<ColPoint> hit;                           // +0x34 the nearest
    int32_t found;                           // +0x38
    float best;                              // +0x3C its distance
    uint8_t unknown40;                       // +0x40
    uint8_t skipSurfaceA;                    // +0x41 hits on surface 'A' don't count
    uint8_t pedModels;                       // +0x42 (peds' special collision; some entities pass anyway)
    uint8_t unknown43;
    re::Ptr32<uint8_t __cdecl(CEntity* e)> filter;    // +0x44 true: skipped
};
static_assert(offsetof(SweepQuery, radius) == 0x1C);
static_assert(offsetof(SweepQuery, hit) == 0x34);
static_assert(offsetof(SweepQuery, filter) == 0x44);
static_assert(sizeof(SweepQuery) == 0x48);

// ---- the game's ----
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorScale, 0x00412350, CVector* out, const CVector* v, float s);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(float, __thiscall, Vector_MagnitudeSqr, 0x0040D9F0, const CVector* v);
GAME_FN(float, __cdecl, Max, 0x00414490, float a, float b);
// (0x004146B0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* Multiply3x3(CVector* out, const col::CMatrix* m, const CVector* v) { return re::CallVec16<0x004146B0, CVector*>(out, m, v); }
// (0x0041A9A0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* MultiplyPoint(CVector* out, const col::CMatrix* m, const CVector* v) { return re::CallVec16<0x0041A9A0, CVector*>(out, m, v); }
// (0x0045D500 stores 16 bytes through `out4`: called through re::CallVec16)
static inline void MultiplyPoint4(float* out4, const col::CMatrix* m, const CVector* v) { re::CallVec16<0x0045D500, void>(out4, m, v); }
// (0x00414C80 stores 16 bytes through `out`: called through re::CallVec16)
static inline void TransformVector(CVector* out, const col::CMatrix* m, const CVector* v) { re::CallVec16<0x00414C80, void>(out, m, v); }
GAME_FN(void, __cdecl, Matrix_Invert, 0x00413770, const col::CMatrix* in, col::CMatrix* out);
GAME_FN(void, __thiscall, CameraCapsule_Set, 0x004F8AE0, Capsule* self, const CVector* a, const CVector* b, float radius);
GAME_FN(void, __thiscall, ColSphere_Set, 0x0056EB80, CColSphere* sphere, float radius, const CVector* center);
GAME_FN(void, __thiscall, ColModel_GetVertex, 0x0056E680, CColModel* col, CVector* out, int index);
GAME_FN(float, __cdecl, SegmentTriangleDistSqr, 0x00573560, const Segment* segment, const TriangleEdges* triangle,
        float* segmentParam, float* s, float* t);
GAME_FN(CEntity*, __cdecl, GetEntityFromNode, 0x0044A3E0, int type, int index);
GAME_FN(CColModel*, __cdecl, GetEntityColModel, 0x0051AF50, CEntity* entity);
GAME_FN(col::CMatrix*, __thiscall, Entity_GetMatrix, 0x0046DD20, CEntity* entity);
GAME_FN(uint8_t, __cdecl, Entity_45EE90, 0x0045EE90, CEntity* entity);
GAME_FN(uint8_t, __thiscall, Entity_TouchesSphere, 0x00465BA0, CEntity* entity, const CVector* center, float radius);
GAME_FN(uint8_t, __thiscall, Ped_4762F0, 0x004762F0, CEntity* ped, CEntity* other);
GAME_FN(uint8_t, __thiscall, Ped_4473E0, 0x004473E0, CEntity* ped);
GAME_FN(void, __cdecl, Ped_SetUpColModel_5727D0, 0x005727D0, CEntity* ped);   // (into 0xCF3138)
GAME_FN(void, __cdecl, CleanUpReference, 0x004657E0, CEntity* entity, re::Ptr32<CEntity>* slot);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, CEntity* entity, re::Ptr32<CEntity>* slot);
GAME_FN(void, __thiscall, ColPoint_Assign, 0x0046AA70, ColPoint* self, const ColPoint* other);
GAME_FN(void, __cdecl, ClearScanCodes, 0x0045D840);
// the triangle tree's queries (ColTriangleTree.cpp)
GAME_FN(uint16_t, __thiscall, Tree_FirstInSphere, 0x005617F0, void* tree, const CVector* center, float radius,
        int cylinder);
GAME_FN(uint16_t, __cdecl, Tree_Next, 0x005618A0);
GAME_FN(uint8_t, __cdecl, SphereTouchesBox, 0x00564B50, const CColSphere* sphere, const CVector* min,
        const CVector* max);
GAME_FN(uint8_t, __cdecl, ProcessSweptSphere, 0x0056A060, const CVector* from, const CVector* to, float radius,
        const col::CMatrix* matrix, CColModel* col, ColPoint* point, float* distance, uint32_t flags, int, int);
// this file's, where modes apply
GAME_FN(void, __cdecl, SegmentBoxClosest_, 0x00561A10, const CVector* a, const CVector* b, const CVector* centre,
        const float* rows, const CVector* size, CVector* onSegment, CVector* onBox);
GAME_FN(TriangleEdges*, __thiscall, TriangleEdges_Construct, 0x005621B0, TriangleEdges* self);
GAME_FN(int32_t, __cdecl, SphereVsSphere_, 0x00562340, const CVector* a, float ra, const CVector* b, float rb,
        ColPoint* out);
GAME_FN(int32_t, __cdecl, CapsuleVsSphere_, 0x005624E0, const Capsule* capsule, const float* sphere, ColPoint* out);
GAME_FN(int32_t, __cdecl, CapsuleVsBox_, 0x005625D0, const Capsule* capsule, const ColBox* box,
        const col::CMatrix* matrix, ColPoint* out);
GAME_FN(int32_t, __cdecl, CapsuleVsTriangle_, 0x00562990, Capsule capsule, const CVector* verts, ColPoint* out);
GAME_FN(int32_t, __cdecl, CapsuleVsModel_, 0x005632C0, CEntity* entity, Capsule capsule, CColModel* col,
        const col::CMatrix* matrix, ColPoint* out, int max);
GAME_FN(void, __thiscall, CapsuleQuery_ScanList_, 0x00563970, CapsuleQuery* self, re::Ptr32<void>* list);
GAME_FN(void, __thiscall, Sweep_ScanList_, 0x00563BF0, SweepQuery* self, re::Ptr32<void>* list, float radius);

GAME_VAR(uint16_t, ms_nCurrentScanCode, 0x00C1AE88);
GAME_VAR(uint32_t*, ms_pNodes, 0x00C0F788);            // the sector lists' nodes
GAME_ARRAY(uint8_t*, ms_modelInfoPtrs, 0x00C67738);
GAME_VAR(uint8_t, ms_currentChapter, 0x00A147E0);
GAME_VAR(int32_t, ms_poiPointModel, 0x00BF42DC);
GAME_VAR(CColModel*, ms_pedColModel, 0x00CF3138);
GAME_ARRAY(float, ms_triangleBounds, 0x00CF26B8);     // [6] min x, y, z, max x, y, z (the capsule's, in model space)
constexpr uintptr_t kSectors = 0x00C1B178;             // 36 x 36 of 0x14 (5 lists)

uint8_t* Bytes(void* p) {
    return static_cast<uint8_t*>(p);
}
int16_t& ScanCode(CEntity* e) {
    return *reinterpret_cast<int16_t*>(Bytes(e) + 0x10A);
}
int16_t ModelIndex(CEntity* e) {
    return *reinterpret_cast<int16_t*>(Bytes(e) + 0x10E);
}

// ---- 0x5621D0 (the capsule in eax, the point in edi; the point is the contact's own):
// the contact's normal from the capsule's axis (its nearest point) to the point, and the
// distance
void ContactFromPoint(ColPoint* out, const Capsule* c, const CVector* point) {
    CVector d;
    VectorSub(&d, point, &c->a);
    float t = float(double(c->dir.y) * d.y + double(c->dir.x) * d.x + double(c->dir.z) * d.z);
    CVector n;
    if (t < 0.0f) {
        VectorSub(&n, &c->a, point);
    } else if (c->length < t) {
        VectorSub(&n, &c->b, point);
    } else {
        CVector along = {c->dir.x * t, c->dir.y * t, t * c->dir.z};
        CVector q;
        VectorAdd(&q, &c->a, &along);
        VectorSub(&n, &q, point);
    }
    float lengthSq = float(double(n.x) * n.x + double(n.y) * n.y + double(n.z) * n.z);
    Vector_Normalise(&n);
    out->point = *point;
    out->normal = n;
    out->depth = float(std::sqrt(double(std::fabs(lengthSq))));
}

// ---- 0x561A10: the nearest points of a segment and an oriented box (its centre, rows
// of its rotation's columns as 4 floats each, its size): walked along the segment
// through the box's face planes until the distance stops falling
void __cdecl SegmentBoxClosest(const CVector* a, const CVector* b, const CVector* centre, const float* R,
                               const CVector* size, CVector* onSegment, CVector* onBox) {
    // the segment in the box's frame
    CVector d = {a->x - centre->x, a->y - centre->y, a->z - centre->z};
    float s[3], v[3], sign[3];
    s[0] = float(double(R[0]) * d.x + double(d.y) * R[4] + double(d.z) * R[8]);
    s[1] = float(double(R[1]) * d.x + double(R[5]) * d.y + double(d.z) * R[9]);
    s[2] = float(double(R[2]) * d.x + double(d.y) * R[6] + double(R[10]) * d.z);
    CVector e = {b->x - a->x, b->y - a->y, b->z - a->z};
    v[0] = float(double(R[0]) * e.x + double(e.y) * R[4] + double(e.z) * R[8]);
    v[1] = float(double(R[1]) * e.x + double(R[5]) * e.y + double(e.z) * R[9]);
    v[2] = float(double(R[2]) * e.x + double(e.y) * R[6] + double(R[10]) * e.z);
    for (int i = 0; i < 3; i++) {
        if (v[i] < 0.0f) {
            s[i] = -s[i];
            v[i] = -v[i];
            sign[i] = -1.0f;
        } else {
            sign[i] = 1.0f;
        }
    }
    float half[3] = {float(double(size->x) * 0.5), float(double(size->y) * 0.5), float(double(size->z) * 0.5)};
    float vSq[3] = {v[0] * v[0], v[1] * v[1], v[2] * v[2]};
    // per axis: where the segment next crosses a face plane, and which side it's on
    float next[3];
    int side[3];
    for (int i = 0; i < 3; i++) {
        if (!(v[i] > 0.0f)) {
            next[i] = 2.0f;
            side[i] = 0;
        } else if (-half[i] > s[i]) {
            side[i] = -1;
            next[i] = float((double(-half[i]) - s[i]) / v[i]);
        } else {
            side[i] = half[i] < s[i] ? 1 : 0;
            next[i] = float((double(half[i]) - s[i]) / v[i]);
        }
    }
    auto sel = [&](int i) { return side[i] != 0 ? vSq[i] : 0.0f; };
    float t = 0.0f;
    float slope = float(float(float(0.0 - double(next[0]) * sel(0)) - double(next[1]) * sel(1)) - double(next[2]) * sel(2));
    if (!(0.0f <= slope)) {
        for (;;) {
            float n = 1.0f;
            if (t < next[0] && next[0] < 1.0f && next[0] < n) {
                n = next[0];
            }
            if (t < next[1] && next[1] < 1.0f && next[1] < n) {
                n = next[1];
            }
            if (t < next[2] && next[2] < 1.0f && next[2] < n) {
                n = next[2];
            }
            float f = float((double(n) - next[0]) * sel(0) + 0.0);
            f = float((double(n) - next[1]) * sel(1) + f);
            f = float((double(n) - next[2]) * sel(2) + f);
            if (f >= 0.0f) {
                float k = float((double(f) - slope) / (double(n) - t));
                t = float(double(t) - double(slope) / k);
                break;
            }
            if (n == next[0]) {
                side[0]++;
                next[0] = float((double(half[0]) - s[0]) / v[0]);
            }
            if (n == next[1]) {
                side[1]++;
                next[1] = float((double(half[1]) - s[1]) / v[1]);
            }
            if (n == next[2]) {
                side[2]++;
                next[2] = float((double(half[2]) - s[2]) / v[2]);
            }
            t = n;
            slope = f;
            if (!(n < 1.0f)) {
                t = 1.0f;
                break;
            }
        }
    }
    onSegment->x = float(double(t) * e.x + a->x);
    onSegment->y = float(double(t) * e.y + a->y);
    onSegment->z = float(double(t) * e.z + a->z);
    // the box's point: the segment's, clamped to the box
    float c[3];
    for (int i = 0; i < 3; i++) {
        c[i] = float((double(t) * v[i] + s[i]) * sign[i]);
        if (-half[i] > c[i]) {
            c[i] = -half[i];
        } else if (c[i] > half[i]) {
            c[i] = half[i];
        }
    }
    float rx = float(double(R[1]) * c[1] + double(R[0]) * c[0] + double(R[2]) * c[2]);
    float ry = float(double(R[5]) * c[1] + double(c[0]) * R[4] + double(c[2]) * R[6]);
    float rz = float(double(R[9]) * c[1] + double(R[8]) * c[0] + double(R[10]) * c[2]);
    onBox->x = centre->x + rx;
    onBox->y = ry + centre->y;
    onBox->z = centre->z + rz;
}

// ---- 0x5621B0 (thiscall)
TriangleEdges* __fastcall TriangleEdges_Construct_(TriangleEdges* self, re::Edx) {
    self->origin = {0.0f, 0.0f, 0.0f};
    self->edge1 = {0.0f, 0.0f, 0.0f};
    self->edge2 = {0.0f, 0.0f, 0.0f};
    return self;
}

// ---- 0x562340: two spheres (0: apart); the contact halfway through the overlap, the
// normal from b to a, the depth the overlap (the radii's sum when the centres meet)
int32_t __cdecl SphereVsSphere(const CVector* a, float ra, const CVector* b, float rb, ColPoint* out) {
    CVector d;
    VectorSub(&d, a, b);
    float lengthSq = float(double(d.x) * d.x + double(d.y) * d.y + double(d.z) * d.z);
    float distance = float(std::sqrt(double(float(std::fabs(lengthSq)))));
    double sum = double(ra) + rb;
    if (distance > sum) {
        return 0;
    }
    if (distance <= 0.0f) {
        out->point = *a;
        out->normal = {1.0f, 0.0f, 0.0f};
        out->depth = float(sum);
        return 1;
    }
    float inverse = float(1.0 / distance);
    float h = float((double(rb) - ra - distance) * 0.5);
    VectorSub(&d, a, b);
    out->normal = {inverse * d.x, d.y * inverse, inverse * d.z};
    CVector p = {h * out->normal.x, out->normal.y * h, h * out->normal.z};
    CVector q;
    out->point = *VectorAdd(&q, a, &p);
    out->depth = float(sum - distance);
    return 1;
}

// ---- 0x5624E0: the capsule against a sphere (centre, radius)
int32_t __cdecl CapsuleVsSphere(const Capsule* c, const float* sphere, ColPoint* out) {
    const CVector* center = reinterpret_cast<const CVector*>(sphere);
    CVector d;
    VectorSub(&d, center, &c->middle);
    float t = float(double(c->dir.y) * d.y + double(c->dir.x) * d.x + double(c->dir.z) * d.z);
    float half = float(double(c->length) * 0.5);
    if (half < t) {
        t = half;
    }
    if (-half > t) {
        t = -half;
    }
    CVector along = {c->dir.x * t, c->dir.y * t, t * c->dir.z};
    CVector nearest;
    VectorAdd(&nearest, &c->middle, &along);
    int32_t hit = SphereVsSphere_(&nearest, c->radius, center, sphere[3], out);
    if (hit) {
        ContactFromPoint(out, c, &out->point);
    }
    return hit;
}

// ---- 0x5625D0: the capsule against a model's box under a matrix
int32_t __cdecl CapsuleVsBox(const Capsule* c, const ColBox* box, const col::CMatrix* matrix, ColPoint* out) {
    const float* m = matrix->m;
    CVector sum = {box->max[0] + box->min[0], box->max[1] + box->min[1], box->max[2] + box->min[2]};
    CVector local = {float(double(sum.x) * 0.5), float(double(sum.y) * 0.5), float(double(sum.z) * 0.5)};
    CVector rotated;
    Multiply3x3(&rotated, matrix, &local);
    CVector centre = {rotated.x + m[12], rotated.y + m[13], rotated.z + m[14]};
    float rows[12] = {m[0], m[4], m[8], 1.0f, m[1], m[5], m[9], 1.0f, m[2], m[6], m[10], 1.0f};
    CVector size = {box->max[0] - box->min[0], box->max[1] - box->min[1], box->max[2] - box->min[2]};
    CVector a = c->a;
    CVector b = c->b;
    CVector onSegment, onBox;
    SegmentBoxClosest_(&a, &b, &centre, rows, &size, &onSegment, &onBox);
    CVector p = onSegment;
    CVector q = onBox;
    int32_t hit = SphereVsSphere_(&p, c->radius, &q, 0.0f, out);
    if (hit) {
        ContactFromPoint(out, c, &q);
    }
    return hit;
}

float Min2(float a, float b) {  // (b on ties)
    return b > a ? a : b;
}
float Max2(float a, float b) {  // (b on ties)
    return b < a ? a : b;
}

// ---- 0x562990: the capsule (by value) against a triangle (inside the box at
// 0xCF26B8, which 0x5632C0 sets)
int32_t __cdecl CapsuleVsTriangle(Capsule c, const CVector* v, ColPoint* out) {
    const float* box = ms_triangleBounds;
    if (box[5] < Min2(Min2(v[0].z, v[1].z), v[2].z)) {
        return 0;
    }
    if (box[2] > Max2(Max2(v[0].z, v[1].z), v[2].z)) {
        return 0;
    }
    if (box[3] < Min2(Min2(v[0].x, v[1].x), v[2].x)) {
        return 0;
    }
    if (box[4] < Min2(Min2(v[0].y, v[1].y), v[2].y)) {
        return 0;
    }
    if (box[0] > Max2(Max2(v[0].x, v[1].x), v[2].x)) {
        return 0;
    }
    if (box[1] > Max(Max(v[0].y, v[1].y), v[2].y)) {
        return 0;
    }
    Segment segment;
    segment.start = c.a;
    segment.dir = {0.0f, 0.0f, 0.0f};
    CVector dir;
    segment.dir = *VectorScale(&dir, &c.dir, c.length);
    TriangleEdges triangle;
    TriangleEdges_Construct(&triangle);
    triangle.origin = v[0];
    CVector e;
    triangle.edge1 = *VectorSub(&e, &v[1], &v[0]);
    triangle.edge2 = *VectorSub(&e, &v[2], &v[0]);
    float segmentParam, s, t;
    float distanceSq = SegmentTriangleDistSqr(&segment, &triangle, &segmentParam, &s, &t);
    if (distanceSq > double(c.radius) * c.radius) {
        return 0;
    }
    float u = float(1.0 - (double(s) + t));
    CVector p0, p1, p2, sum, point;
    VectorScale(&p0, &v[0], u);
    VectorScale(&p2, &v[2], t);
    VectorScale(&p1, &v[1], s);
    VectorAdd(&sum, &p1, &p2);
    VectorAdd(&point, &sum, &p0);
    ContactFromPoint(out, &c, &point);
    return 1;
}

// ---- 0x562D80 (thiscall): the query for a capsule (its bounding sphere: round the
// box of both ends, grown by the radius)
CapsuleQuery* __fastcall CapsuleQuery_Construct(CapsuleQuery* self, re::Edx, const Capsule* capsule, uint32_t flags,
                                                ColPoint* hits, int32_t maxHits) {
    float* f = reinterpret_cast<float*>(self);
    for (int i = 0; i < 12; i++) {
        f[i] = 0.0f;
    }
    self->centre = {0.0f, 0.0f, 0.0f};
    self->maxHits = maxHits;
    self->capsule = *capsule;
    self->flags = flags;
    self->hits = hits;
    const Capsule* c = capsule;
    CVector v = {c->dir.x * c->length, c->dir.y * c->length, c->length * c->dir.z};
    CVector h = {float(double(v.x) * 0.5), float(double(v.y) * 0.5), float(double(v.z) * 0.5)};
    CVector a;
    VectorSub(&a, &c->middle, &h);
    float length = c->length;
    v = {c->dir.x * length, c->dir.y * length, length * c->dir.z};
    h = {float(double(v.x) * 0.5), float(double(v.y) * 0.5), float(double(v.z) * 0.5)};
    CVector b;
    VectorAdd(&b, &c->middle, &h);
    CVector lo = {b.x <= a.x ? b.x : a.x, b.y <= a.y ? b.y : a.y, b.z <= a.z ? b.z : a.z};
    CVector hi = {b.x < a.x ? a.x : b.x, b.y < a.y ? a.y : b.y, b.z < a.z ? a.z : b.z};
    lo = {lo.x - c->radius, lo.y - c->radius, lo.z - c->radius};
    hi = {c->radius + hi.x, c->radius + hi.y, c->radius + hi.z};
    CVector sum;
    VectorAdd(&sum, &hi, &lo);
    self->centre = {float(double(sum.x) * 0.5), float(double(sum.y) * 0.5), float(double(sum.z) * 0.5)};
    CVector size;
    VectorSub(&size, &hi, &lo);
    float lengthSq = float(double(size.x) * size.x + double(size.y) * size.y + double(size.z) * size.z);
    float diagonal = float(std::sqrt(double(float(std::fabs(lengthSq)))));
    self->entities = nullptr;
    self->radius = float(double(diagonal) * 0.5);
    self->numEntities = 0;
    self->maxEntities = 0;
    return self;
}

// ---- 0x563060 (thiscall): the swept sphere from `from` to `to` (its bounding sphere
// round the box of both, grown by the radius)
SweepQuery* __fastcall Sweep_Construct(SweepQuery* self, re::Edx, const CVector* from, const CVector* to, float radius,
                                       uint32_t flags, ColPoint* hit, uint8_t(__cdecl* filter)(CEntity*),
                                       int skipSurfaceA, int pedModels, CEntity* excluded) {
    self->from = {0.0f, 0.0f, 0.0f};
    self->to = {0.0f, 0.0f, 0.0f};
    self->centre = {0.0f, 0.0f, 0.0f};
    self->from = *from;
    self->to = *to;
    self->radius = radius;
    self->flags = flags;
    self->hit = hit;
    CVector lo = {from->x < to->x ? from->x : to->x, from->y < to->y ? from->y : to->y,
                  from->z < to->z ? from->z : to->z};
    CVector hi = {to->x < from->x ? from->x : to->x, to->y < from->y ? from->y : to->y,
                  to->z < from->z ? from->z : to->z};
    lo = {lo.x - radius, lo.y - radius, lo.z - radius};
    hi = {hi.x + radius, hi.y + radius, radius + hi.z};
    CVector sum;
    VectorAdd(&sum, &hi, &lo);
    self->centre = {float(double(sum.x) * 0.5), float(double(sum.y) * 0.5), float(double(sum.z) * 0.5)};
    CVector size;
    VectorSub(&size, &hi, &lo);
    float lengthSq = float(double(size.x) * size.x + double(size.y) * size.y + double(size.z) * size.z);
    float diagonal = float(std::sqrt(double(float(std::fabs(lengthSq)))));
    self->skipSurfaceA = uint8_t(skipSurfaceA);
    self->boundRadius = float(double(diagonal) * 0.5);
    self->filter = filter;
    self->pedModels = uint8_t(pedModels);
    self->excluded = excluded;
    return self;
}

// the contact back in world space
void ToWorld(ColPoint* out, const col::CMatrix* matrix) {
    CVector p, n;
    MultiplyPoint(&p, matrix, &out->point);
    TransformVector(&n, matrix, &out->normal);
    out->point = p;
    out->normal = n;
}

// ---- 0x5632C0: the capsule (by value) against an entity's collision model under a
// matrix: its spheres, boxes and triangles (in model space, through the triangle
// tree when it has one); the contacts made, at most `max`
int32_t __cdecl CapsuleVsModel(CEntity* entity, Capsule c, CColModel* model, const col::CMatrix* matrix,
                               ColPoint* out, int max) {
    col::CMatrix inverse;
    inverse.attached = nullptr;
    Matrix_Invert(matrix, &inverse);
    CVector localMiddle;
    MultiplyPoint(&localMiddle, &inverse, &c.middle);
    CColSphere bound;
    ColSphere_Set(&bound, float(double(c.length) * 0.5 + c.radius + 0.0010000000474974513), &localMiddle);
    if (!SphereTouchesBox(&bound, &model->boxMin, &model->boxMax)) {
        return 0;
    }
    CCollisionData* data = model->data;
    ColPoint* start = out;
    float worldSphere[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    CVector verts[3] = {};
    Capsule local = {};
    if (data) {
        uint8_t* e = Bytes(entity);
        int numSpheres = int8_t(data->numSpheres);
        if ((e[0x108] & 7) == 2 && *reinterpret_cast<int32_t*>(e + 0x46C) == 1 && numSpheres > 5) {
            numSpheres--;
        }
        for (int i = 0; i < numSpheres && max != 0; i++) {
            CVector center = data->spheres[i].center;
            float world[4];
            MultiplyPoint4(world, matrix, &center);
            worldSphere[0] = world[0];
            worldSphere[1] = world[1];
            worldSphere[2] = world[2];
            worldSphere[3] = data->spheres[i].radius;
            if (CapsuleVsSphere_(&c, worldSphere, out)) {
                out++;
                max--;
            }
        }
        const ColBox* boxes = re::PtrCast<const ColBox*>(data->boxes);
        for (int i = 0; i < int8_t(data->numBoxes) && max != 0; i++) {
            if (CapsuleVsBox_(&c, &boxes[i], matrix, out)) {
                out++;
                max--;
            }
        }
        if (data->planes || data->numTriangles) {
            CVector la, lb;
            MultiplyPoint(&la, &inverse, &c.a);
            MultiplyPoint(&lb, &inverse, &c.b);
            CameraCapsule_Set(&local, &la, &lb, c.radius);
            float r = c.radius;
            float* box = ms_triangleBounds;
            box[0] = float(double(Min2(la.x, lb.x)) - r);
            box[1] = float(double(Min2(la.y, lb.y)) - r);
            box[2] = float(double(Min2(la.z, lb.z)) - r);
            box[3] = float(double(Max2(la.x, lb.x)) + r);
            box[4] = float(double(la.y > lb.y ? la.y : lb.y) + r);
            box[5] = float(double(r) + Max2(la.z, lb.z));
        }
        auto testTriangle = [&](int index) {
            const CColTriangle& tri = data->triangles[index];
            ColModel_GetVertex(model, &verts[0], tri.a);
            ColModel_GetVertex(model, &verts[1], tri.b);
            ColModel_GetVertex(model, &verts[2], tri.c);
            if (CapsuleVsTriangle_(local, verts, out)) {
                ToWorld(out, matrix);
                out++;
                max--;
            }
        };
        if (data->planes) {
            CVector center = bound.center;
            uint16_t index = Tree_FirstInSphere(data->planes, &center, bound.radius, 0);
            while (index != 0xFFFF && max != 0) {
                testTriangle(index);
                index = Tree_Next();
            }
        } else {
            for (int i = 0; i < data->numTriangles && max != 0; i++) {
                testTriangle(i);
            }
        }
    }
    return int32_t((reinterpret_cast<uint8_t*>(out) - reinterpret_cast<uint8_t*>(start)) / int32_t(sizeof(ColPoint)));
}

// the entity passes the usual checks (scanned once, has a model and collision, is in
// the current chapter, isn't +0xF4'd)
bool Usable(CEntity* e) {
    uint8_t* b = Bytes(e);
    if (!*reinterpret_cast<re::Ptr32<void>*>(b + 0x20)) {
        return false;
    }
    if (!*reinterpret_cast<re::Ptr32<void>*>(b + 0x1C) && !*reinterpret_cast<re::Ptr32<void>*>(b + 0x60)) {
        return false;
    }
    if (!(ms_modelInfoPtrs[ModelIndex(e)][0xB] & uint8_t(1u << (ms_currentChapter & 31)))) {
        return false;
    }
    return *reinterpret_cast<re::Ptr32<void>*>(b + 0xF4) == nullptr;
}

// a hit's entity reference set to e
void SetHitEntity(re::Ptr32<CEntity>* slot, CEntity* e) {
    if (*slot != e) {
        if (*slot) {
            CleanUpReference(*slot, slot);
        }
        *slot = e;
        RegisterReference(e, slot);
    }
}

// ---- 0x563970 (thiscall): one sector list's entities against the capsule
void __fastcall CapsuleQuery_ScanList(CapsuleQuery* self, re::Edx, re::Ptr32<void>* list) {
    uint32_t* node = static_cast<uint32_t*>(*list);
    while (node) {
        uint32_t entry = *node;
        CEntity* e = GetEntityFromNode(int(entry & 0xF), int((entry >> 4) & 0x3FFF));
        node = (entry >> 18) == 0x3FFF ? nullptr : ms_pNodes + (entry >> 18);
        if (ScanCode(e) == int16_t(ms_nCurrentScanCode)) {
            continue;
        }
        ScanCode(e) = int16_t(ms_nCurrentScanCode);
        if (!Usable(e)) {
            continue;
        }
        CColModel* model = GetEntityColModel(e);
        uint32_t flags = self->flags;
        uint8_t has18 = (model->data && model->data->unknown18) ? 1 : 0;
        if (uint8_t((flags >> 9) & 1) != has18) {
            continue;
        }
        if (flags & 0x80) {
            if (Entity_45EE90(e)) {
                continue;
            }
            if (self->entities) {
                uint8_t* mi = ms_modelInfoPtrs[ModelIndex(e)];
                uint8_t type = mi[8];
                if (type == 8 || ((*reinterpret_cast<uint32_t*>(mi + 0x28) & 0x200000) && type == 1)) {
                    uint8_t* b = Bytes(e);
                    uint8_t* m = *reinterpret_cast<re::Ptr32<uint8_t>*>(b + 0x14);
                    const CVector* pos = reinterpret_cast<const CVector*>(m ? m + 0x30 : b + 4);
                    CVector centre, d;
                    VectorAdd(&centre, pos, &model->sphereCenter);
                    VectorSub(&d, &centre, &self->capsule.b);
                    float r = model->sphereRadius + self->capsule.radius;
                    float lengthSq = Vector_MagnitudeSqr(&d);
                    if (!(float(double(r) * r) < lengthSq) && self->numEntities < self->maxEntities) {
                        self->entities[self->numEntities] = e;
                        self->numEntities++;
                    }
                    continue;
                }
            }
        }
        int first = self->numHits;
        ColPoint* hits = self->hits + first;
        int room = self->maxHits - first;
        col::CMatrix* matrix = Entity_GetMatrix(e);
        int count = CapsuleVsModel_(e, self->capsule, model, matrix, hits, room);
        for (int i = self->numHits; i < self->numHits + count; i++) {
            SetHitEntity(&self->hits[i].entity, e);
        }
        self->numHits += count;
    }
}

// ---- 0x563BF0 (thiscall): one sector list's entities against the swept sphere
// (touching the sphere of `radius` round its start), keeping the nearest hit
void __fastcall Sweep_ScanList(SweepQuery* self, re::Edx, re::Ptr32<void>* list, float radius) {
    uint32_t* node = static_cast<uint32_t*>(*list);
    while (node) {
        uint32_t entry = *node;
        CEntity* e = GetEntityFromNode(int(entry & 0xF), int((entry >> 4) & 0x3FFF));
        node = (entry >> 18) == 0x3FFF ? nullptr : ms_pNodes + (entry >> 18);
        if (ScanCode(e) == int16_t(ms_nCurrentScanCode)) {
            continue;
        }
        ScanCode(e) = int16_t(ms_nCurrentScanCode);
        uint8_t* b = Bytes(e);
        if (!Usable(e) && (!self->pedModels || !*reinterpret_cast<re::Ptr32<void>*>(b + 0x100))) {
            continue;
        }
        if ((self->flags & 0x80) && self->filter && self->filter(e)) {
            continue;
        }
        CVector from = self->from;
        if (!Entity_TouchesSphere(e, &from, radius)) {
            continue;
        }
        CEntity* ped = nullptr;
        if ((b[0x108] & 7) == 3) {
            ped = e;
            if (self->excluded && Ped_4762F0(e, self->excluded)) {
                continue;
            }
        }
        CColModel* model;
        if (self->pedModels && ped && !*reinterpret_cast<re::Ptr32<void>*>(b + 0xA4) && ModelIndex(e) != ms_poiPointModel &&
            !Ped_4473E0(e)) {
            Ped_SetUpColModel_5727D0(e);
            model = ms_pedColModel;
        } else {
            model = GetEntityColModel(e);
        }
        ColPoint point;
        point.point = {0.0f, 0.0f, 0.0f};
        point.normal = {0.0f, 0.0f, 0.0f};
        point.depth = 3.4028234663852886e38f;
        point.entity = nullptr;
        float distance;
        col::CMatrix* matrix = Entity_GetMatrix(e);
        if (ProcessSweptSphere(&self->from, &self->to, self->radius, matrix, model, &point, &distance, self->flags, 0xF,
                               1) &&
            (!self->skipSurfaceA || point.surfaceB != 'A')) {
            if (self->best > distance) {
                self->best = distance;
                ColPoint_Assign(self->hit, &point);
                SetHitEntity(&self->hit->entity, e);
                self->found = 1;
            }
        }
        if (point.entity) {
            CleanUpReference(point.entity, &point.entity);
        }
    }
}

int SectorLow(double v) {
    float s = float(int(v / 50.0 + 18.0));
    return int(s > 0.0f ? s : 0.0f);
}
int SectorHigh(double v) {
    float s = float(int(v / 50.0 + 18.0));
    return int(s < 35.0f ? s : 35.0f);
}
void NextScanCode() {
    if (ms_nCurrentScanCode < 0xFFFF) {
        ms_nCurrentScanCode++;
    } else {
        ClearScanCodes();
        ms_nCurrentScanCode = 1;
    }
}
// the sectors' lists round a sphere, in the game's order
template <typename F>
void ForEachList(const CVector& centre, float radius, uint32_t lists, F scanList) {
    int x0 = SectorLow(double(centre.x) - radius);
    int y0 = SectorLow(double(centre.y) - radius);
    int x1 = SectorHigh(double(radius) + centre.x);
    int y1 = SectorHigh(double(radius) + centre.y);
    NextScanCode();
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            re::Ptr32<void>* sector = reinterpret_cast<re::Ptr32<void>*>(kSectors + (y * 36 + x) * 0x14);
            if (lists & 1) {
                scanList(&sector[0]);
            }
            if (lists & 2) {
                scanList(&sector[2]);
            }
            if (lists & 4) {
                scanList(&sector[3]);
            }
            if (lists & 8) {
                scanList(&sector[1]);
            }
            if (lists & 0x10) {
                scanList(&sector[4]);
            }
        }
    }
}

// ---- 0x563E80 (thiscall): the capsule against the world; the hits made
int32_t __fastcall CapsuleQuery_Scan(CapsuleQuery* self, re::Edx) {
    self->numHits = 0;
    ForEachList(self->centre, self->radius, self->flags, [self](re::Ptr32<void>* list) { CapsuleQuery_ScanList_(self, list); });
    return self->numHits;
}

// ---- 0x564080 (thiscall): the swept sphere against the world; whether it hit
int32_t __fastcall Sweep_Test(SweepQuery* self, re::Edx) {
    self->best = 3.4028234663852886e38f;
    self->found = 0;
    int x0 = SectorLow(double(self->centre.x) - self->boundRadius);
    int y0 = SectorLow(double(self->centre.y) - self->boundRadius);
    int x1 = SectorHigh(double(self->boundRadius) + self->centre.x);
    int y1 = SectorHigh(double(self->boundRadius) + self->centre.y);
    NextScanCode();
    CVector d;
    VectorSub(&d, &self->to, &self->from);
    float lengthSq = float(double(d.y) * d.y + double(d.z) * d.z + double(d.x) * d.x);
    float length = float(std::sqrt(double(float(std::fabs(lengthSq)))));
    float radius = length + self->radius;
    uint32_t lists = self->flags;
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            re::Ptr32<void>* sector = reinterpret_cast<re::Ptr32<void>*>(kSectors + (y * 36 + x) * 0x14);
            if (self->flags & 1) {
                Sweep_ScanList_(self, &sector[0], radius);
            }
            if (self->flags & 2) {
                Sweep_ScanList_(self, &sector[2], radius);
            }
            if (self->flags & 4) {
                Sweep_ScanList_(self, &sector[3], radius);
            }
            if (self->flags & 8) {
                Sweep_ScanList_(self, &sector[1], radius);
            }
            if (self->flags & 0x10) {
                Sweep_ScanList_(self, &sector[4], radius);
            }
        }
    }
    (void)lists;
    return self->found;
}

} // namespace

RE_UNHOOKED("CapsuleCollision::ContactFromPoint", 0x005621D0);

#define COLCAPSULE_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(colcapsule_original_, __LINE__);                                        \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(colcapsule_original_, __LINE__), \
             re::Mode::Original)

COLCAPSULE_REVERSED("CapsuleCollision::SegmentBoxClosest", 0x00561A10, &SegmentBoxClosest);
COLCAPSULE_REVERSED("CapsuleCollision::TriangleEdges", 0x005621B0, &TriangleEdges_Construct_);
COLCAPSULE_REVERSED("CapsuleCollision::SphereVsSphere", 0x00562340, &SphereVsSphere);
COLCAPSULE_REVERSED("CapsuleCollision::CapsuleVsSphere", 0x005624E0, &CapsuleVsSphere);
COLCAPSULE_REVERSED("CapsuleCollision::CapsuleVsBox", 0x005625D0, &CapsuleVsBox);
COLCAPSULE_REVERSED("CapsuleCollision::CapsuleVsTriangle", 0x00562990, &CapsuleVsTriangle);
COLCAPSULE_REVERSED("CapsuleQuery::CapsuleQuery", 0x00562D80, &CapsuleQuery_Construct);
COLCAPSULE_REVERSED("SweepQuery::SweepQuery", 0x00563060, &Sweep_Construct);
COLCAPSULE_REVERSED("CapsuleCollision::CapsuleVsModel", 0x005632C0, &CapsuleVsModel);
COLCAPSULE_REVERSED("CapsuleQuery::ScanList", 0x00563970, &CapsuleQuery_ScanList);
COLCAPSULE_REVERSED("SweepQuery::ScanList", 0x00563BF0, &Sweep_ScanList);
COLCAPSULE_REVERSED("CapsuleQuery::Scan", 0x00563E80, &CapsuleQuery_Scan);
COLCAPSULE_REVERSED("SweepQuery::Test", 0x00564080, &Sweep_Test);
