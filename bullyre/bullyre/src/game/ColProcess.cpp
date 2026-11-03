// CCollision's pairwise tests (0x5658C0..0x568570): the nearest points of two lines,
// point in box, sphere against sphere and box, line against triangle, sphere against
// triangle (GTA San Andreas' CCollision::Process*/Test* where they match; the planes
// here are made from the triangle each time, as in 0x564A20).

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "reversed.h"

namespace {

using col::CVector;

struct ColLine {
    CVector start;    // +0
    float startW;
    CVector end;      // +0x10
    float endW;
};
static_assert(sizeof(ColLine) == 0x20);

struct ColBox {
    CVector min;      // +0
    float minW;
    CVector max;      // +0x10
    float maxW;
    uint8_t surface;  // +0x20
    uint8_t piece;    // +0x21
    uint8_t pad[2];
};
static_assert(sizeof(ColBox) == 0x24);

struct ColPoint {
    CVector point;      // +0
    re::Ptr32<void> entity;       // +0xC
    CVector normal;     // +0x10
    uint8_t unknown1C[4];
    uint8_t surfaceA;   // +0x20
    uint8_t pieceA;     // +0x21
    uint8_t surfaceB;   // +0x22
    uint8_t pieceB;     // +0x23
    float depth;        // +0x24
};
static_assert(sizeof(ColPoint) == 0x28);

GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorScale, 0x00412350, CVector* out, const CVector* v, float s);
GAME_FN(CVector*, __cdecl, CrossProduct, 0x00414730, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(float, __thiscall, Vector_MagnitudeSqr, 0x0040D9F0, const CVector* v);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const CVector* v);
GAME_FN(float, __cdecl, Sqrt, 0x0040DF90, float v);   // (of |v|)
GAME_FN(CVector*, __cdecl, VectorDivide, 0x00565DA0, CVector* out, const CVector* v, float s);
GAME_FN(void, __thiscall, ColPoint_SetNormal, 0x004A89A0, ColPoint* point, float x, float y, float z);
GAME_FN(void, __cdecl, TrianglePlane, 0x00564A20, const CVector* a, const CVector* b, const CVector* c,
        CVector* normal, float* distance, uint8_t* axis);

CVector Decompress(const CompressedVector& v) {
    return {float(double(v.x) * 0.0078125), float(double(v.y) * 0.0078125), float(double(v.z) * 0.0078125)};
}

// ---- 0x5658C0: the nearest points of two lines (their segments)
void __cdecl ClosestPoints(const ColLine* lineA, const ColLine* lineB, CVector* onA, CVector* onB) {
    CVector dA, dB, r;
    VectorSub(&dA, &lineA->end, &lineA->start);
    VectorSub(&dB, &lineB->end, &lineB->start);
    VectorSub(&r, &lineA->start, &lineB->start);
    float a = float(double(dA.y) * dA.y + double(dA.x) * dA.x + double(dA.z) * dA.z);
    float b = float(double(dB.x) * dA.x + double(dB.y) * dA.y + double(dB.z) * dA.z);
    float c = float(double(dB.z) * dB.z + double(dB.x) * dB.x + double(dB.y) * dB.y);
    float d = float(double(r.z) * dA.z + double(r.y) * dA.y + double(r.x) * dA.x);
    float e = float(double(r.y) * dB.y + double(r.x) * dB.x + double(r.z) * dB.z);
    float denom = float(double(a) * c - double(b) * b);
    float sD = denom, tD = c;
    float sN, tN;
    if (!(1e-06f > denom)) {
        sN = float(double(e) * b - double(d) * c);
        tN = float(double(e) * a - double(d) * b);
        if (sN < 0.0f) {
            sN = 0.0f;
            tN = e;
        } else {
            tD = denom;
            if (denom < sN) {
                tN = e + b;
                sN = denom;
                tD = c;
            }
        }
    } else {
        sN = 0.0f;
        sD = 1.0f;
        tN = e;
    }
    if (tN < 0.0f) {
        tN = 0.0f;
        sN = -d;
        if (sN < 0.0f) {
            sN = 0.0f;
        } else {
            float s = sD;
            sD = a;
            if (a < sN) {
                sN = s;
                sD = s;
            }
        }
    } else if (tD < tN) {
        tN = tD;
        sN = b - d;
        if (sN < 0.0f) {
            sN = 0.0f;
        } else {
            float s = sD;
            sD = a;
            if (a < sN) {
                sN = s;
                sD = s;
            }
        }
    }
    float sc = !(1e-06f > std::fabs(sN)) ? float(double(sN) / sD) : 0.0f;
    float tc = !(1e-06f > std::fabs(tN)) ? float(double(tN) / tD) : 0.0f;
    CVector p = {sc * dA.x, dA.y * sc, sc * dA.z};
    CVector q;
    *onA = *VectorAdd(&q, &lineA->start, &p);
    p = {tc * dB.x, dB.y * tc, tc * dB.z};
    *onB = *VectorAdd(&q, &lineB->start, &p);
}

// ---- 0x565C20 (thiscall): a point strictly inside the box
uint32_t __fastcall ColBox_IsPointInside(const ColBox* self, re::Edx, const CVector* p) {
    if (self->min.x < p->x && self->min.y < p->y && self->min.z < p->z && p->x < self->max.x && p->y < self->max.y &&
        p->z < self->max.z) {
        return 1;
    }
    return 0;
}

// ---- 0x565DE0: sphere against sphere (nearer than the best so far: mindistsq)
uint8_t __cdecl ProcessSphereSphere(const CColSphere* s1, const CColSphere* s2, ColPoint* out, float* minDistSq) {
    CVector dist;
    VectorSub(&dist, &s1->center, &s2->center);
    float lengthSq = float(double(dist.x) * dist.x + double(dist.y) * dist.y + double(dist.z) * dist.z);
    float d = float(double(float(std::sqrt(double(float(std::fabs(lengthSq)))))) - s2->radius);
    if (d < 0.0f) {
        d = 0.0f;
    }
    float dSq = d * d;
    if (!(*minDistSq > dSq)) {
        return 0;
    }
    if (!(s1->radius > d)) {
        return 0;
    }
    float depth = s1->radius - d;
    Vector_Normalise(&dist);
    CVector scaled = {dist.x * d, dist.y * d, d * dist.z};
    CVector p;
    out->point = *VectorSub(&p, &s1->center, &scaled);
    out->normal = dist;
    out->surfaceA = s1->surface;
    out->pieceA = s1->piece;
    out->depth = depth;
    out->surfaceB = s2->surface;
    out->pieceB = s2->piece;
    *minDistSq = dSq;
    return 1;
}

// ---- 0x565F30: sphere against box: the nearest point of the box (or, the centre inside,
// out through the nearest face)
uint8_t __cdecl ProcessSphereBox(const CColSphere* s, const ColBox* box, ColPoint* out, float* minDistSq) {
    const CVector& c = s->center;
    float r = s->radius;
    if (box->min.x > double(c.x) + r || box->max.x < double(c.x) - r) {
        return 0;
    }
    if (box->min.y > double(r) + c.y || box->max.y < double(c.y) - r) {
        return 0;
    }
    if (box->min.z > double(r) + c.z || box->max.z < double(c.z) - r) {
        return 0;
    }
    int xs = c.x < box->min.x ? 1 : (c.x <= box->max.x ? 0 : 2);
    int ys = c.y < box->min.y ? 1 : (c.y <= box->max.y ? 0 : 2);
    int zs = c.z < box->min.z ? 1 : (c.z <= box->max.z ? 0 : 2);
    if (xs == 0 && ys == 0 && zs == 0) {
        // inside: out through the nearest face
        CVector sum, mid, d;
        VectorAdd(&sum, &box->max, &box->min);
        VectorDivide(&mid, &sum, 2.0f);
        VectorSub(&d, &c, &mid);
        float dx = d.x <= 0.0f ? c.x - box->min.x : box->max.x - c.x;
        float dy = d.y <= 0.0f ? c.y - box->min.y : box->max.y - c.y;
        float dz = d.z <= 0.0f ? c.z - box->min.z : box->max.z - c.z;
        if (!(dy <= dx) && !(dz <= dx)) {
            if (!(*minDistSq > dx * dx)) {
                return 0;
            }
            out->point = c;
            ColPoint_SetNormal(out, d.x <= 0.0f ? -1.0f : 1.0f, 0.0f, 0.0f);
            out->depth = dx + r;
        } else if (!(dx <= dy) && !(dz <= dy)) {
            if (!(*minDistSq > dy * dy)) {
                return 0;
            }
            out->point = c;
            ColPoint_SetNormal(out, 0.0f, d.y <= 0.0f ? -1.0f : 1.0f, 0.0f);
            out->depth = dy + r;
        } else {
            if (!(*minDistSq > dz * dz)) {
                return 0;
            }
            out->point = c;
            ColPoint_SetNormal(out, 0.0f, 0.0f, d.z <= 0.0f ? -1.0f : 1.0f);
            out->depth = dz + r;
        }
        out->surfaceA = s->surface;
        out->pieceA = s->piece;
        out->surfaceB = box->surface;
        out->pieceB = box->piece;
        // (with minDistSq not above 0, the contact is written but not reported)
        if (!(*minDistSq > 0.0f)) {
            return 0;
        }
        *minDistSq = 0.0f;
        return 1;
    }
    CVector p = {xs == 1 ? box->min.x : (xs == 2 ? box->max.x : c.x),
                 ys == 1 ? box->min.y : (ys == 2 ? box->max.y : c.y),
                 zs == 1 ? box->min.z : (zs == 2 ? box->max.z : c.z)};
    CVector dist;
    VectorSub(&dist, &c, &p);
    float lengthSq = Vector_MagnitudeSqr(&dist);
    if (!(*minDistSq > lengthSq)) {
        return 0;
    }
    out->point = p;
    float length = Sqrt(lengthSq);
    CVector n;
    out->normal = *VectorDivide(&n, &dist, length);
    out->surfaceA = s->surface;
    out->pieceA = s->piece;
    out->surfaceB = box->surface;
    out->pieceB = box->piece;
    out->depth = r - length;
    *minDistSq = lengthSq;
    return 1;
}

// the triangle's vertices and plane
struct TrianglePlaneInfo {
    CVector a, b, c;
    CVector normal;
    float distance;
    uint8_t axis;
};
void MakePlane(TrianglePlaneInfo& t, const CompressedVector* verts, const CColTriangle* tri) {
    t.a = Decompress(verts[tri->a]);
    t.b = Decompress(verts[tri->b]);
    t.c = Decompress(verts[tri->c]);
    t.normal = {0.0f, 0.0f, 0.0f};
    TrianglePlane(&t.a, &t.b, &t.c, &t.normal, &t.distance, &t.axis);
}

// the point inside the triangle, in the plane the triangle faces most: the three 2D
// edge tests (their results; the winding flips with the facing's sign)
void EdgeTests(const TrianglePlaneInfo& t, const CVector& p, double& f1, double& f2, double& f3) {
    // axis 1..5 as the plane says; 0 and anything else: as +x
    int axis = t.axis >= 1 && t.axis <= 5 ? t.axis : 0;
    auto uv = [&](const CVector& v, double& u, double& w) {
        if (axis <= 1) {
            u = v.y;
            w = v.z;
        } else if (axis <= 3) {
            u = v.z;
            w = v.x;
        } else {
            u = v.x;
            w = v.y;
        }
    };
    double au, av, su, sv, tu, tv, pu, pv;
    uv(t.a, au, av);
    bool positive = (axis & 1) == 0;
    uv(positive ? t.c : t.b, su, sv);
    uv(positive ? t.b : t.c, tu, tv);
    uv(p, pu, pv);
    f1 = (su - au) * (pv - av) - (sv - av) * (pu - au);
    f2 = (tu - au) * (pv - av) - (tv - av) * (pu - au);
    f3 = (pv - sv) * (tu - su) - (pu - su) * (tv - sv);
}

// ---- 0x5672F0: a line crosses a triangle
uint8_t __cdecl TestLineTriangle(const ColLine* line, const CompressedVector* verts, const CColTriangle* tri) {
    TrianglePlaneInfo t;
    MakePlane(t, verts, tri);
    const CVector& n = t.normal;
    const CVector& p0 = line->start;
    const CVector& p1 = line->end;
    double side = (double(n.y) * p1.y + double(n.x) * p1.x + double(n.z) * p1.z - t.distance) *
                  (double(p0.x) * n.x + double(p0.y) * n.y + double(p0.z) * n.z - t.distance);
    if (!(side <= 0.0)) {
        return 0;
    }
    CVector dir;
    VectorSub(&dir, &p1, &p0);
    float s = float((double(t.distance) - double(n.x) * p0.x - double(n.y) * p0.y - double(n.z) * p0.z) /
                    (double(dir.y) * n.y + double(dir.x) * n.x + double(dir.z) * n.z));
    CVector along = {s * dir.x, dir.y * s, s * dir.z};
    CVector p;
    VectorAdd(&p, &p0, &along);
    double f1, f2, f3;
    EdgeTests(t, p, f1, f2, f3);
    if (f1 < 0.0) {
        return 0;
    }
    if (f2 > 0.0) {
        return 0;
    }
    return f3 < 0.0 ? 0 : 1;
}

// ---- 0x567790: a line against a triangle: the crossing, nearer along the line than
// the best so far (minT)
uint8_t __cdecl ProcessLineTriangle(const ColLine* line, const CompressedVector* verts, const CColTriangle* tri,
                                    ColPoint* out, float* minT) {
    TrianglePlaneInfo t;
    MakePlane(t, verts, tri);
    const CVector& n = t.normal;
    const CVector& p0 = line->start;
    const CVector& p1 = line->end;
    float s0 = float(double(p0.x) * n.x + double(p0.y) * n.y + double(p0.z) * n.z - t.distance);
    double side = (double(n.y) * p1.y + double(n.x) * p1.x + double(n.z) * p1.z - t.distance) * s0;
    if (!(side <= 0.0)) {
        return 0;
    }
    CVector dir;
    VectorSub(&dir, &p1, &p0);
    float denom = float(double(dir.z) * n.z + double(n.y) * dir.y + double(n.x) * dir.x);
    if (denom <= 0.0f && 0.0f <= denom) {
        return 0;
    }
    float s = float((double(t.distance) - double(p0.x) * n.x - double(p0.y) * n.y - double(p0.z) * n.z) / denom);
    CVector along = {s * dir.x, s * dir.y, s * dir.z};
    CVector p;
    VectorAdd(&p, &p0, &along);
    double f1, f2, f3;
    EdgeTests(t, p, f1, f2, f3);
    if (f1 < 0.0) {
        return 0;
    }
    if (!(f2 <= 0.0)) {
        return 0;
    }
    if (!(0.0 <= f3) || !(s < *minT)) {
        return 0;
    }
    CVector d, scaled, q;
    VectorSub(&d, &p1, &p0);
    VectorScale(&scaled, &d, s);
    out->point = *VectorAdd(&q, &p0, &scaled);
    out->normal = n;
    out->surfaceB = tri->surface;
    out->pieceB = 0;
    out->surfaceA = 0;
    out->pieceA = 0;
    *minT = s;
    return 1;
}

// the sphere's centre against a triangle, in the triangle's plane: the distance and
// the nearest point. (The first edge's direction is scaled to unit length only when
// it's shorter than FLT_EPSILON: the game's check is the wrong way round.)
struct SphereTriangle {
    CVector a, b, c;
    CVector normal;
    float planeDist;   // the centre's height over the plane
    float distance;
    CVector point;
};

bool SphereNearTriangle(const CColSphere* s, const CompressedVector* verts, const CColTriangle* tri,
                        SphereTriangle& r, bool withMinDist, const float* minDistSq, bool test) {
    r.a = Decompress(verts[tri->a]);
    r.b = Decompress(verts[tri->b]);
    r.c = Decompress(verts[tri->c]);
    r.normal = {0.0f, 0.0f, 0.0f};
    CVector ab, ac, cross;
    VectorSub(&ab, &r.b, &r.a);
    VectorSub(&ac, &r.c, &r.a);
    r.normal = *CrossProduct(&cross, &ac, &ab);
    Vector_Normalise(&r.normal);
    const CVector& n = r.normal;
    const CVector& c = s->center;
    if (test) {
        r.planeDist = float(double(n.y) * c.y + double(n.x) * c.x + double(n.z) * c.z -
                            (double(n.z) * r.a.z + double(r.a.y) * n.y + double(r.a.x) * n.x));
    } else {
        float na = float(double(n.z) * r.a.z + double(n.y) * r.a.y + double(n.x) * r.a.x);
        r.planeDist = float(double(c.y) * n.y + double(c.x) * n.x + double(c.z) * n.z - na);
    }
    float absPlane = std::fabs(r.planeDist);
    if (s->radius < absPlane) {
        return false;
    }
    double planeSq = double(r.planeDist) * r.planeDist;
    if (withMinDist && *minDistSq < planeSq) {
        return false;
    }
    CVector e;
    VectorSub(&e, &r.b, &r.a);
    float len = float(std::sqrt(double(float(std::fabs(float(double(e.x) * e.x + double(e.y) * e.y + double(e.z) * e.z))))));
    if (std::fabs(len) < 1.1920929e-07f) {
        float inverse = float(1.0 / len);
        e = {inverse * e.x, e.y * inverse, inverse * e.z};
    }
    CVector v1;
    CrossProduct(&v1, &e, &n);
    CVector w;
    VectorSub(&w, &r.c, &r.a);
    float xc = float(double(w.z) * e.z + double(w.y) * e.y + double(w.x) * e.x);
    float yc = float(double(w.z) * v1.z + double(w.y) * v1.y + double(v1.x) * w.x);
    CVector sc;
    VectorSub(&sc, &c, &r.a);
    float xs = float(double(sc.z) * e.z + double(sc.y) * e.y + double(sc.x) * e.x);
    float ys = float(double(sc.z) * v1.z + double(sc.y) * v1.y + double(v1.x) * sc.x);
    double crossAB = double(len) * ys - double(xs) * 0.0;
    double ycxs = double(yc) * xs;
    double xcys = double(xc) * ys;
    bool inAB = !(crossAB < 0.0);
    bool inAC = !(ycxs - xcys < 0.0);
    double dxc = double(xc) - len;
    double dxs = double(xs) - len;
    bool inBC = !(dxc * ys - dxs * yc < 0.0);
    int count = int(inAB) + int(inAC) + int(inBC);
    auto vertex = [&](const CVector& v) {
        r.point = v;
        CVector d;
        VectorSub(&d, &c, &v);
        r.distance = Vector_Magnitude(&d);
    };
    // the edge from p to q at t (t in (0, 1)): its 2D distance `edge` over the plane
    auto onEdge = [&](const CVector& p, const CVector& q, float t, double edge, float edgeLengthSq) {
        float h = float(edge / Sqrt(edgeLengthSq));
        r.distance = Sqrt(float(double(h) * h + planeSq));
        CVector d, scaled, sum;
        VectorSub(&d, &q, &p);
        VectorScale(&scaled, &d, t);
        r.point = *VectorAdd(&sum, &p, &scaled);
    };
    if (count == 1) {
        if (inAB) {
            vertex(r.c);
        } else if (inAC) {
            vertex(r.b);
        } else {
            vertex(r.a);
        }
    } else if (count == 2) {
        if (!inAB) {
            float lengthSq = float(double(len) * len + 0.0);
            float t = float((double(xs) * len + double(ys) * 0.0) / lengthSq);
            if (t <= 0.0f) {
                vertex(r.a);
            } else if (1.0f <= t) {
                vertex(r.b);
            } else {
                onEdge(r.a, r.b, t, crossAB, lengthSq);
            }
        } else if (!inAC) {
            float lengthSq = float(double(xc) * xc + double(yc) * yc);
            float t = float((double(ys) * yc + double(xs) * xc) / lengthSq);
            if (t <= 0.0f) {
                vertex(r.a);
            } else if (1.0f <= t) {
                vertex(r.c);
            } else {
                onEdge(r.a, r.c, t, xcys - ycxs, lengthSq);
            }
        } else {
            float lengthSq = float(dxc * dxc + double(yc) * yc);
            float t = float((double(yc) * ys + dxc * dxs) / lengthSq);
            if (t <= 0.0f) {
                vertex(r.b);
            } else if (1.0f <= t) {
                vertex(r.c);
            } else {
                onEdge(r.b, r.c, t, dxc * ys - double(yc) * dxs, lengthSq);
            }
        }
    } else {
        // over the triangle
        r.distance = absPlane;
        CVector scaled = {r.planeDist * n.x, n.y * r.planeDist, r.planeDist * n.z};
        CVector p;
        r.point = *VectorSub(&p, &c, &scaled);
    }
    return true;
}

// ---- 0x567D20: a sphere touches a triangle
uint8_t __cdecl TestSphereTriangle(const CColSphere* s, const CompressedVector* verts, const CColTriangle* tri) {
    SphereTriangle r;
    if (!SphereNearTriangle(s, verts, tri, r, false, nullptr, true)) {
        return 0;
    }
    return s->radius > r.distance ? 1 : 0;
}

// ---- 0x568570: a sphere against a triangle (nearer than the best so far)
uint8_t __cdecl ProcessSphereTriangle(const CColSphere* s, const CompressedVector* verts, const CColTriangle* tri,
                                      ColPoint* out, float* minDistSq) {
    SphereTriangle r;
    if (!SphereNearTriangle(s, verts, tri, r, true, minDistSq, false)) {
        return 0;
    }
    float distanceSq = r.distance * r.distance;
    if (!(r.distance < s->radius) || !(distanceSq < *minDistSq)) {
        return 0;
    }
    CVector n;
    VectorSub(&n, &s->center, &r.point);
    Vector_Normalise(&n);
    out->point = r.point;
    out->normal = n;
    out->surfaceA = s->surface;
    out->pieceA = s->piece;
    out->surfaceB = tri->surface;
    out->pieceB = 0;
    out->depth = s->radius - r.distance;
    *minDistSq = distanceSq;
    return 1;
}

} // namespace

#define COLPROC_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(colproc_original_, __LINE__);                                        \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(colproc_original_, __LINE__), \
             re::Mode::Original)

COLPROC_REVERSED("CCollision::ClosestPoints", 0x005658C0, &ClosestPoints);
COLPROC_REVERSED("CColBox::IsPointInside", 0x00565C20, &ColBox_IsPointInside);
COLPROC_REVERSED("CCollision::ProcessSphereSphere", 0x00565DE0, &ProcessSphereSphere);
COLPROC_REVERSED("CCollision::ProcessSphereBox", 0x00565F30, &ProcessSphereBox);
COLPROC_REVERSED("CCollision::TestLineTriangle", 0x005672F0, &TestLineTriangle);
COLPROC_REVERSED("CCollision::ProcessLineTriangle", 0x00567790, &ProcessLineTriangle);
COLPROC_REVERSED("CCollision::TestSphereTriangle", 0x00567D20, &TestSphereTriangle);
COLPROC_REVERSED("CCollision::ProcessSphereTriangle", 0x00568570, &ProcessSphereTriangle);
