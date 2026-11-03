// Collision distance and sweep math (0x572DD0..0x576ED0): Eberly's squared distances
// (segment/segment, point/segment, point/triangle, segment/triangle, as in Magic
// Software's MgcDist code, with Bully's tolerances), and the moving-sphere tests the
// swept col-model test (0x56C8C0) uses: a sphere moving along a direction against a
// triangle, a quad, a box (its six faces as quads) and another moving sphere.
//
// Conditions are written as the game's compare flags read (NaN included); sums in
// their source order (dot products x + y + z).

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "reversed.h"

namespace {

using col::CVector;

// A segment: origin + t * direction, t in [0, 1]
struct Segment {
    CVector origin;     // +0
    CVector direction;  // +0xC
};
static_assert(sizeof(Segment) == 0x18);

// A triangle: origin + s * edge0 + t * edge1
struct Triangle {
    CVector origin;  // +0
    CVector edge0;   // +0xC
    CVector edge1;   // +0x18
};
static_assert(sizeof(Triangle) == 0x24);

// A sphere: center and radius (CColSphere's first 16 bytes)
struct Sphere {
    CVector center;  // +0
    float radius;    // +0xC
};

// A plane: normal and distance (n . p + d = 0)
struct Plane {
    CVector normal;  // +0
    float d;         // +0xC

    // 0x575290 (thiscall): through the point, the normal normalised
    void Set(const CVector* point, const CVector* normal);
    // 0x5752F0 (thiscall): through three points (the normal (c - b) x (a - b), normalised)
    void Set3(const CVector* a, const CVector* b, const CVector* c);
};
static_assert(sizeof(Plane) == 0x10);

GAME_FN(CVector*, __cdecl, CS_VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, CS_VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, CS_Normalise, 0x00414E10, CVector* v);
GAME_FN(CVector*, __cdecl, CS_CrossProduct, 0x00414730, CVector* out, const CVector* a, const CVector* b);
// ours, through the game's addresses
GAME_FN(float, __cdecl, CS_SegSegDistSq, 0x00572DD0, const Segment* a, const Segment* b, float* s, float* t);
GAME_FN(float, __cdecl, CS_PointSegDistSq, 0x005748A0, const CVector* p, const Segment* seg, float* t);
GAME_FN(float, __cdecl, CS_PointTriDistSq, 0x00574A30, const CVector* p, const Triangle* tri, float* s, float* t);
GAME_FN(uint8_t, __cdecl, CS_PointInTriangle, 0x00574F10, const CVector* a, const CVector* b, const CVector* c, const CVector* p);
GAME_FN(uint8_t, __cdecl, CS_PointInPolygon, 0x005750E0, int count, const CVector* vertices, const CVector* p);
GAME_FN(float, __cdecl, CS_PointLineDistSq, 0x00575130, const CVector* p, const CVector* a, const CVector* b, CVector* closest);
GAME_FN(float, __cdecl, CS_PointLineDist, 0x00575250, const CVector* p, const CVector* a, const CVector* b, CVector* closest);
GAME_FN(void, __thiscall, CS_PlaneSet, 0x00575290, Plane* plane, const CVector* point, const CVector* normal);
GAME_FN(void, __thiscall, CS_PlaneSet3, 0x005752F0, Plane* plane, const CVector* a, const CVector* b, const CVector* c);
GAME_FN(uint8_t, __cdecl, CS_LineIntersect2D, 0x005753B0, const float* p0, const float* p1, const float* q0, const float* q1, float* t);
GAME_FN(uint8_t, __cdecl, CS_SphereLine, 0x00575460, const Sphere* sphere, const CVector* a, const CVector* b, int* count, float* t1, float* t2);
GAME_FN(uint8_t, __cdecl, CS_SweepSphereQuad, 0x00575F00, const CVector* quad, const CVector* normal, const Sphere* sphere, const CVector* dir, float* t,
        CVector* hitNormal, CVector* hitPoint, uint8_t* face);

inline float Dot(const CVector& a, const CVector& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

} // namespace

namespace ColSweep {

// 0x572DD0: squared distance between two segments, and where on each (Eberly)
float __cdecl SegSegDistSq(const Segment* seg0, const Segment* seg1, float* pS, float* pT) {
    CVector diff;
    CS_VectorSub(&diff, &seg0->origin, &seg1->origin);
    float a = Dot(seg0->direction, seg0->direction);
    float b = -Dot(seg1->direction, seg0->direction);
    float c = Dot(seg1->direction, seg1->direction);
    float d = Dot(diff, seg0->direction);
    float f = Dot(diff, diff);
    float det = std::fabs(a * c - b * b);
    float s, t, e, sqr;

    if (!(det >= 1e-6f)) {
        // parallel
        if (b <= 0.0f) {
            if (-d < a) {
                if (!(d <= 0.0f)) {
                    e = -Dot(seg1->direction, diff);
                    s = 0.0f;
                    if (!(-b <= d)) {
                        t = -d / b;
                        sqr = t * (e + e + t * c);
                    } else {
                        t = 1.0f;
                        sqr = e + e + c;
                    }
                } else {
                    s = -d / a;
                    t = 0.0f;
                    sqr = d * s;
                }
            } else {
                s = 1.0f;
                t = 0.0f;
                sqr = d + d + a;
            }
        } else {
            if (!(d < 0.0f)) {
                s = 0.0f;
                t = 0.0f;
                sqr = f;
                goto done;
            }
            s = -d;
            if (!(s <= a)) {
                e = -Dot(seg1->direction, diff);
                s = 1.0f;
                if (-(d + a) < b) {
                    t = -(d + a) / b;
                    sqr = a + 2.0f * d + f + t * (c * t + 2.0f * (b + e));
                } else {
                    t = 1.0f;
                    float tmp = d + b + e;
                    sqr = a + c + f + 2.0f * tmp;
                }
                goto done;
            }
            s = s / a;
            t = 0.0f;
            sqr = d * s;
        }
        goto addF;
    }

    e = -Dot(seg1->direction, diff);
    s = e * b - d * c;
    t = d * b - e * a;
    if (s < 0.0f) {
        if (t < 0.0f) {
            if (d < 0.0f) {
                goto regionS0;
            }
        regionT0:
            s = 0.0f;
            if (e >= 0.0f) {
                t = 0.0f;
                sqr = f;
                goto done;
            }
        } else {
            if (det < t) {
                if (d + b < 0.0f) {
                regionT1:
                    t = 1.0f;
                    s = -(d + b);
                    if (a <= s) {
                        s = 1.0f;
                        float tmp = d + b + e;
                        sqr = a + c + f + 2.0f * tmp;
                        goto done;
                    }
                regionT1Divide:
                    t = 1.0f;
                    s = s / a;
                    sqr = (d + b) * s + c + 2.0f * e;
                    goto addF;
                }
                goto regionT0;
            }
            s = 0.0f;
            if (e >= 0.0f) {
                t = 0.0f;
                goto done_f;
            }
        }
        s = 0.0f;
        if (-e < c) {
            t = -e / c;
            sqr = e * t;
        } else {
            t = 1.0f;
            sqr = c + 2.0f * e;
        }
    } else if (det < s) {
        if (t < 0.0f) {
            if (a <= -d) {
            regionS1:
                s = 1.0f;
                e = e + b;
                if (e < 0.0f) {
                    if (c <= -e) {
                        t = 1.0f;
                        sqr = a + c + f + 2.0f * (d + e);
                        goto done;
                    }
                    t = -e / c;
                    sqr = e * t + a + 2.0f * d;
                } else {
                    t = 0.0f;
                    sqr = a + 2.0f * d;
                }
            } else {
                t = 0.0f;
                if (d >= 0.0f) {
                    s = 0.0f;
                    goto done_f;
                }
                s = -d / a;
                sqr = d * s;
            }
        } else if (det < t) {
            s = -(d + b);
            if (!(s <= a)) {
                goto regionS1;
            }
            t = 1.0f;
            if (d + b < 0.0f) {
                goto regionT1Divide;
            }
            s = 0.0f;
            sqr = c + 2.0f * e;
        } else {
            s = 1.0f;
            e = e + b;
            if (e < 0.0f) {
                if (c <= -e) {
                    t = 1.0f;
                    sqr = a + c + f + 2.0f * (d + e);
                    goto done;
                }
                t = -e / c;
                sqr = e * t + a + 2.0f * d;
            } else {
                t = 0.0f;
                sqr = a + 2.0f * d;
            }
        }
    } else if (t < 0.0f) {
        t = 0.0f;
        if (d >= 0.0f) {
            s = 0.0f;
            goto done_f;
        }
    regionS0:
        t = 0.0f;
        if (-d < a) {
            s = -d / a;
            sqr = d * s;
        } else {
            s = 1.0f;
            sqr = a + 2.0f * d;
        }
    } else if (det < t) {
        t = 1.0f;
        if (d + b < 0.0f) {
            goto regionT1;
        }
        s = 0.0f;
        sqr = c + 2.0f * e;
    } else {
        // interior
        float invDet = 1.0f / det;
        s = invDet * s;
        t = invDet * t;
        sqr = s * (a * s + b * t + 2.0f * d) + t * (b * s + c * t + 2.0f * e);
    }
addF:
    sqr = sqr + f;
    goto done;
done_f:
    sqr = f;
done:
    if (pS) {
        *pS = s;
    }
    if (pT) {
        *pT = t;
    }
    return std::fabs(sqr);
}

// 0x5748A0: squared distance from a point to a segment, and where on it
float __cdecl PointSegDistSq(const CVector* point, const Segment* seg, float* pT) {
    CVector diff;
    CS_VectorSub(&diff, point, &seg->origin);
    float t = Dot(diff, seg->direction);
    if (!(t <= 0.0f)) {
        float sqrLen = Dot(seg->direction, seg->direction);
        float x, y, z;
        if (!(sqrLen <= t)) {
            t = t / sqrLen;
            x = t * seg->direction.x;
            y = seg->direction.y * t;
            z = seg->direction.z * t;
        } else {
            x = seg->direction.x;
            y = seg->direction.y;
            z = seg->direction.z;
            t = 1.0f;
        }
        diff.z = diff.z - z;
        diff.x = diff.x - x;
        diff.y = diff.y - y;
    } else {
        t = 0.0f;
    }
    if (pT) {
        *pT = t;
    }
    return diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
}

// 0x574A00: the distance (not squared)
float __cdecl PointSegDist(const CVector* point, const Segment* seg, float* pT) {
    float sqr = std::fabs(CS_PointSegDistSq(point, seg, pT));
    return std::sqrt(sqr);
}

// 0x574A30: squared distance from a point to a triangle, and its (s, t) there (Eberly)
float __cdecl PointTriDistSq(const CVector* point, const Triangle* tri, float* pS, float* pT) {
    CVector diff;
    CS_VectorSub(&diff, &tri->origin, point);
    float a00 = Dot(tri->edge0, tri->edge0);
    float a01 = Dot(tri->edge0, tri->edge1);
    float a11 = Dot(tri->edge1, tri->edge1);
    float b0 = Dot(diff, tri->edge0);
    float b1 = Dot(diff, tri->edge1);
    float c = Dot(diff, diff);
    float det = std::fabs(a00 * a11 - a01 * a01);
    float s = a01 * b1 - a11 * b0;
    float t = a01 * b0 - a00 * b1;
    float sqr, numer, denom, tmp0, tmp1;

    if (det < t + s) {
        if (0.0f <= s) {
            if (0.0f <= t) {
                // region 1
                numer = a11 + b1 - a01 - b0;
                if (0.0f < numer) {
                    goto region1;
                }
                s = 0.0f;
                t = 1.0f;
                sqr = a11 + 2.0f * b1;
            } else {
                // region 6
                tmp0 = a01 + b1;
                tmp1 = a00 + b0;
                if (tmp1 <= tmp0) {
                    t = 0.0f;
                    if (tmp1 <= 0.0f) {
                        goto regionS1;
                    }
                    if (b0 >= 0.0f) {
                        s = 0.0f;
                        goto done_c;
                    }
                    s = -b0 / a00;
                    sqr = b0 * s;
                } else {
                    numer = tmp1 - tmp0;
                    denom = a00 - 2.0f * a01 + a11;
                    if (!(denom <= numer)) {
                        t = numer / denom;
                        s = 1.0f - t;
                        goto interior;
                    }
                    t = 1.0f;
                    s = 0.0f;
                    sqr = a11 + 2.0f * b1;
                }
            }
        } else {
            // region 2
            tmp0 = a01 + b0;
            tmp1 = a11 + b1;
            if (tmp1 <= tmp0) {
                s = 0.0f;
                if (0.0f < tmp1) {
                    if (0.0f <= b1) {
                        t = 0.0f;
                        goto done_c;
                    }
                    t = -b1 / a11;
                    sqr = b1 * t;
                } else {
                    t = 1.0f;
                    sqr = a11 + 2.0f * b1;
                }
            } else {
                numer = tmp1 - tmp0;
            region1:
                denom = a00 - 2.0f * a01 + a11;
                if (!(denom <= numer)) {
                    s = numer / denom;
                    t = 1.0f - s;
                interior:
                    sqr = s * (a00 * s + a01 * t + 2.0f * b0) + t * (a01 * s + a11 * t + 2.0f * b1);
                    goto addC;
                }
                s = 1.0f;
                t = 0.0f;
                sqr = a00 + 2.0f * b0;
            }
        }
    } else {
        if (0.0f <= s) {
            if (0.0f <= t) {
                // region 0
                float invDet = 1.0f / det;
                s = invDet * s;
                t = t * invDet;
                goto interior;
            }
            // region 5
            t = 0.0f;
            if (b0 >= 0.0f) {
                s = 0.0f;
                goto done_c;
            }
        } else if (0.0f <= t || 0.0f <= b0) {
            // region 3 (or 4 towards t)
            s = 0.0f;
            if (b1 >= 0.0f) {
                t = 0.0f;
                goto done_c;
            }
            if (-b1 < a11) {
                t = -b1 / a11;
                sqr = b1 * t;
            } else {
                t = 1.0f;
                sqr = a11 + 2.0f * b1;
            }
            goto addC;
        }
        // region 4 (towards s) / 5
        t = 0.0f;
        if (-b0 < a00) {
            s = -b0 / a00;
            sqr = b0 * s;
        } else {
        regionS1:
            t = 0.0f;
            s = 1.0f;
            sqr = a00 + 2.0f * b0;
        }
    }
addC:
    c = sqr + c;
done_c:
    if (pS) {
        *pS = s;
    }
    if (pT) {
        *pT = t;
    }
    return std::fabs(c);
}

// 0x573560: squared distance between a segment and a triangle; where on the segment
// (r) and the triangle (s, t) (Eberly, with the parallel test relative)
float __cdecl SegTriDistSq(const Segment* seg, const Triangle* tri, float* pR, float* pS, float* pT) {
    const CVector* e0 = &tri->edge0;
    const CVector* e1 = &tri->edge1;
    CVector diff;
    CS_VectorSub(&diff, &tri->origin, &seg->origin);
    float a00 = Dot(seg->direction, seg->direction);
    float a01 = -Dot(*e0, seg->direction);
    float a02 = -Dot(seg->direction, *e1);
    float a11 = Dot(*e0, *e0);
    float a12 = Dot(*e1, *e0);
    float a22 = Dot(*e1, *e1);
    float b0 = -Dot(diff, seg->direction);
    float b1 = Dot(diff, *e0);
    float b2 = Dot(diff, *e1);

    Segment triSeg = {};
    CVector point = {};
    CVector tmp;
    float sqr, sqr0, r, s, t, r0, s0, t0;

    CVector normal;
    CS_CrossProduct(&normal, e0, e1);
    float dot = Dot(normal, seg->direction);
    dot = dot * dot;
    float tolerance = a00 * 1e-6f * Dot(normal, normal);
    if (!(tolerance <= dot)) {
        // parallel: the segment against the three edges and the triangle against both
        // of the segment's ends
        triSeg.origin = tri->origin;
        triSeg.direction = *e0;
        sqr = CS_SegSegDistSq(seg, &triSeg, &r, &s);
        triSeg.direction = *e1;
        t = 0.0f;
        sqr0 = CS_SegSegDistSq(seg, &triSeg, &r0, &t0);
        s0 = 0.0f;
        if (sqr0 < sqr) {
            r = r0;
            s = 0.0f;
            t = t0;
            sqr = sqr0;
        }
        triSeg.origin = *CS_VectorAdd(&tmp, &tri->origin, e0);
        triSeg.direction = *CS_VectorSub(&tmp, e1, e0);
        sqr0 = CS_SegSegDistSq(seg, &triSeg, &r0, &t0);
        s0 = 1.0f - t0;
        if (sqr0 < sqr) {
            r = r0;
            t = t0;
            sqr = sqr0;
            s = s0;
        }
        sqr0 = CS_PointTriDistSq(&seg->origin, tri, &s0, &t0);
        r0 = 0.0f;
        if (sqr0 < sqr) {
            r = 0.0f;
            s = s0;
            t = t0;
            sqr = sqr0;
        }
        goto fromEnd;
    }

    {
        float cof00 = a22 * a11 - a12 * a12;
        float cof01 = a02 * a12 - a01 * a22;
        float cof02 = a12 * a01 - a11 * a02;
        float cof11 = a00 * a22 - a02 * a02;
        float cof12 = a02 * a01 - a00 * a12;
        float a01Sq = a01 * a01;
        float det = a00 * cof00 + a01 * cof01 + a02 * cof02;
        float invDet;
        if (std::fabs(det) < 1.1920929e-07f) {
            invDet = 0.0f;
        } else {
            invDet = 1.0f / det;
        }
        float rhs0 = invDet * -b0;
        float rhs1 = -b1 * invDet;
        float rhs2 = -b2 * invDet;
        r = cof00 * rhs0 + cof01 * rhs1 + cof02 * rhs2;
        s = cof01 * rhs0 + cof11 * rhs1 + cof12 * rhs2;
        t = cof02 * rhs0 + cof12 * rhs1 + (a11 * a00 - a01Sq) * rhs2;
    }

    if (r < 0.0f) {
        // the segment's start: the edges then the start point
        if (!(s + t <= 1.0f)) {
            if (0.0f <= s) {
                if (0.0f <= t) {
                    // region 1m
                    triSeg.origin = *CS_VectorAdd(&tmp, &tri->origin, e0);
                    triSeg.direction = *CS_VectorSub(&tmp, e1, e0);
                    sqr = CS_SegSegDistSq(seg, &triSeg, &r, &t);
                    s = 1.0f - t;
                    sqr0 = CS_PointTriDistSq(&seg->origin, tri, &s0, &t0);
                } else {
                    // region 6m
                    triSeg.origin = tri->origin;
                    triSeg.direction = *e0;
                    sqr = CS_SegSegDistSq(seg, &triSeg, &r, &s);
                    t = 0.0f;
                    triSeg.origin = *CS_VectorAdd(&tmp, &tri->origin, e0);
                    triSeg.direction = *CS_VectorSub(&tmp, e1, e0);
                    sqr0 = CS_SegSegDistSq(seg, &triSeg, &r0, &t0);
                    s0 = 1.0f - t0;
                    if (sqr0 < sqr) {
                        r = r0;
                        t = t0;
                        sqr = sqr0;
                        s = s0;
                    }
                    sqr0 = CS_PointTriDistSq(&seg->origin, tri, &s0, &t0);
                }
            } else {
                // region 2m
                triSeg.origin = tri->origin;
                triSeg.direction = *e1;
                sqr = CS_SegSegDistSq(seg, &triSeg, &r, &t);
                s = 0.0f;
                triSeg.origin = *CS_VectorAdd(&tmp, &tri->origin, e0);
                triSeg.direction = *CS_VectorSub(&tmp, e1, e0);
                sqr0 = CS_SegSegDistSq(seg, &triSeg, &r0, &t0);
                s0 = 1.0f - t0;
                if (sqr0 < sqr) {
                    r = r0;
                    t = t0;
                    sqr = sqr0;
                    s = s0;
                }
                sqr0 = CS_PointTriDistSq(&seg->origin, tri, &s0, &t0);
            }
        } else if (0.0f <= s) {
            if (0.0f <= t) {
                // region 0m
                sqr = CS_PointTriDistSq(&seg->origin, tri, &s, &t);
                r = 0.0f;
                goto done;
            }
            // region 5m
            triSeg.origin = tri->origin;
            triSeg.direction = *e0;
            sqr = CS_SegSegDistSq(seg, &triSeg, &r, &s);
            t = 0.0f;
            sqr0 = CS_PointTriDistSq(&seg->origin, tri, &s0, &t0);
        } else if (0.0f <= t) {
            // region 3m
            triSeg.origin = tri->origin;
            triSeg.direction = *e1;
            sqr = CS_SegSegDistSq(seg, &triSeg, &r, &t);
            s = 0.0f;
            sqr0 = CS_PointTriDistSq(&seg->origin, tri, &s0, &t0);
        } else {
            // region 4m
            triSeg.origin = tri->origin;
            triSeg.direction = *e1;
            sqr = CS_SegSegDistSq(seg, &triSeg, &r, &t);
            triSeg.origin = tri->origin;
            s = 0.0f;
            triSeg.direction = *e0;
            sqr0 = CS_SegSegDistSq(seg, &triSeg, &r0, &s0);
            t0 = 0.0f;
            if (sqr0 < sqr) {
                r = r0;
                s = s0;
                t = 0.0f;
                sqr = sqr0;
            }
            sqr0 = CS_PointTriDistSq(&seg->origin, tri, &s0, &t0);
        }
        if (sqr0 < sqr) {
            r = 0.0f;
            s = s0;
            t = t0;
            sqr = sqr0;
        }
        goto done;
    }

    {
        bool sNegative = s < 0.0f;
        float sum = s + t;
        if (r <= 1.0f) {
            // the closest point is along the segment
            if (!(sum <= 1.0f)) {
                if (sNegative) {
                    // region 2
                    triSeg.origin = tri->origin;
                    triSeg.direction = *e1;
                    sqr = CS_SegSegDistSq(seg, &triSeg, &r, &t);
                    s = 0.0f;
                    triSeg.origin = *CS_VectorAdd(&tmp, &tri->origin, e0);
                    triSeg.direction = *CS_VectorSub(&tmp, e1, e0);
                    sqr0 = CS_SegSegDistSq(seg, &triSeg, &r0, &t0);
                    if (sqr0 < sqr) {
                        r = r0;
                        t = t0;
                        sqr = sqr0;
                        s = 1.0f - t0;
                    }
                } else if (0.0f <= t) {
                    // region 1
                    triSeg.origin = *CS_VectorAdd(&tmp, &tri->origin, e0);
                    triSeg.direction = *CS_VectorSub(&tmp, e1, e0);
                    sqr = CS_SegSegDistSq(seg, &triSeg, &r, &t);
                    s = 1.0f - t;
                } else {
                    // region 6
                    triSeg.origin = tri->origin;
                    triSeg.direction = *e0;
                    sqr = CS_SegSegDistSq(seg, &triSeg, &r, &s);
                    t = 0.0f;
                    triSeg.origin = *CS_VectorAdd(&tmp, &tri->origin, e0);
                    triSeg.direction = *CS_VectorSub(&tmp, e1, e0);
                    sqr0 = CS_SegSegDistSq(seg, &triSeg, &r0, &t0);
                    if (sqr0 < sqr) {
                        r = r0;
                        t = t0;
                        sqr = sqr0;
                        s = 1.0f - t0;
                    }
                }
            } else if (sNegative) {
                triSeg.origin = tri->origin;
                triSeg.direction = *e1;
                if (0.0f <= t) {
                    // region 3
                    sqr = CS_SegSegDistSq(seg, &triSeg, &r, &t);
                    s = 0.0f;
                } else {
                    // region 4
                    sqr = CS_SegSegDistSq(seg, &triSeg, &r, &t);
                    triSeg.origin = tri->origin;
                    s = 0.0f;
                    triSeg.direction = *e0;
                    sqr0 = CS_SegSegDistSq(seg, &triSeg, &r0, &s0);
                    if (sqr0 < sqr) {
                        r = r0;
                        s = s0;
                        t = 0.0f;
                        sqr = sqr0;
                    }
                }
            } else if (0.0f <= t) {
                // region 0: through the triangle
                sqr = 0.0f;
            } else {
                // region 5
                triSeg.origin = tri->origin;
                triSeg.direction = *e0;
                sqr = CS_SegSegDistSq(seg, &triSeg, &r, &s);
                t = 0.0f;
            }
            goto done;
        }

        // the segment's end: the edges then the end point
        if (!(sum <= 1.0f)) {
            if (sNegative) {
                // region 2p
                triSeg.origin = tri->origin;
                triSeg.direction = *e1;
                sqr = CS_SegSegDistSq(seg, &triSeg, &r, &t);
                s = 0.0f;
                triSeg.origin = *CS_VectorAdd(&tmp, &tri->origin, e0);
                triSeg.direction = *CS_VectorSub(&tmp, e1, e0);
                sqr0 = CS_SegSegDistSq(seg, &triSeg, &r0, &t0);
                s0 = 1.0f - t0;
                if (sqr0 < sqr) {
                    goto takeEdge;
                }
                goto fromEnd;
            } else if (0.0f <= t) {
                // region 1p
                triSeg.origin = *CS_VectorAdd(&tmp, &tri->origin, e0);
                triSeg.direction = *CS_VectorSub(&tmp, e1, e0);
                sqr = CS_SegSegDistSq(seg, &triSeg, &r, &t);
                s = 1.0f - t;
                goto fromEnd;
            }
            // region 6p
            triSeg.origin = tri->origin;
            triSeg.direction = *e0;
            sqr = CS_SegSegDistSq(seg, &triSeg, &r, &s);
            t = 0.0f;
            triSeg.origin = *CS_VectorAdd(&tmp, &tri->origin, e0);
            triSeg.direction = *CS_VectorSub(&tmp, e1, e0);
            sqr0 = CS_SegSegDistSq(seg, &triSeg, &r0, &t0);
            s0 = 1.0f - t0;
            if (sqr0 < sqr) {
                r = r0;
                t = t0;
                sqr = sqr0;
                s = s0;
            }
            goto fromEnd;
        }
        if (sNegative) {
            if (t < 0.0f) {
                // region 4p
                triSeg.origin = tri->origin;
                triSeg.direction = *e1;
                sqr = CS_SegSegDistSq(seg, &triSeg, &r, &t);
                triSeg.origin = tri->origin;
                s = 0.0f;
                triSeg.direction = *e0;
                sqr0 = CS_SegSegDistSq(seg, &triSeg, &r0, &s0);
                t0 = 0.0f;
                if (sqr0 < sqr) {
                takeEdge:
                    r = r0;
                    s = s0;
                    sqr = sqr0;
                    t = t0;
                }
                goto fromEnd;
            }
            // region 3p
            triSeg.origin = tri->origin;
            triSeg.direction = *e1;
            sqr = CS_SegSegDistSq(seg, &triSeg, &r, &t);
            s = 0.0f;
            goto fromEnd;
        }
        if (t >= 0.0f) {
            // region 0p
            CS_VectorAdd(&point, &seg->origin, &seg->direction);
            sqr = CS_PointTriDistSq(&point, tri, &s, &t);
            r = 1.0f;
            goto done;
        }
        // region 5p
        triSeg.origin = tri->origin;
        triSeg.direction = *e0;
        sqr = CS_SegSegDistSq(seg, &triSeg, &r, &s);
        t = 0.0f;
    }

fromEnd:
    CS_VectorAdd(&point, &seg->origin, &seg->direction);
    sqr0 = CS_PointTriDistSq(&point, tri, &s0, &t0);
    if (sqr0 < sqr) {
        r = 1.0f;
        s = s0;
        t = t0;
        sqr = sqr0;
    }
done:
    if (pR) {
        *pR = r;
    }
    if (pS) {
        *pS = s;
    }
    if (pT) {
        *pT = t;
    }
    return sqr;
}

// 0x574F10: p inside the triangle abc (barycentric, including the edges)
uint8_t __cdecl PointInTriangle(const CVector* a, const CVector* b, const CVector* c, const CVector* p) {
    CVector e0, e1, w;
    CS_VectorSub(&e0, b, a);
    CS_VectorSub(&e1, c, a);
    CS_VectorSub(&w, p, a);
    float d00 = Dot(e0, e0);
    float d01 = Dot(e1, e0);
    float d11 = Dot(e1, e1);
    float w0 = Dot(w, e0);
    float w1 = Dot(w, e1);
    float det = d01 * d01 - d11 * d00;
    if (std::fabs(det) < 1.1920929e-07f) {
        return 0;
    }
    float inv = 1.0f / det;
    float u = (w1 * d01 - d11 * w0) * inv;
    if (u < 0.0f || 1.0f < u) {
        return 0;
    }
    float v = inv * (d01 * w0 - d00 * w1);
    if (v < 0.0f) {
        return 0;
    }
    return (u + v > 1.0f) ? 0 : 1;
}

// 0x5750E0: p inside the convex polygon (a fan of triangles from the first vertex)
uint8_t __cdecl PointInPolygon(int count, const CVector* vertices, const CVector* p) {
    const CVector* v = vertices;
    for (int i = 0; i < count - 2; i++) {
        if (CS_PointInTriangle(vertices, v + 1, v + 2, p)) {
            return 1;
        }
        v++;
    }
    return 0;
}

// 0x575130: squared distance from p to the line through a and b, and the closest point
float __cdecl PointLineDistSq(const CVector* p, const CVector* a, const CVector* b, CVector* closest) {
    CVector ap, ab, tmp;
    CS_VectorSub(&ap, p, a);
    CS_VectorSub(&ab, b, a);
    float t = Dot(ap, ab) / Dot(ab, ab);
    CVector proj;
    proj.x = t * ab.x;
    proj.y = t * ab.y;
    proj.z = ab.z * t;
    if (closest) {
        *closest = *CS_VectorAdd(&tmp, a, &proj);
    }
    float x = ap.x - proj.x;
    float y = ap.y - proj.y;
    float z = ap.z - proj.z;
    return x * x + y * y + z * z;
}

// 0x575250: the distance (not squared)
float __cdecl PointLineDist(const CVector* p, const CVector* a, const CVector* b, CVector* closest) {
    float sqr = std::fabs(CS_PointLineDistSq(p, a, b, closest));
    return std::sqrt(sqr);
}

// 0x5753B0: where (t along p0-p1) the 2D lines p0-p1 and q0-q1 cross; false when
// parallel
uint8_t __cdecl LineIntersect2D(const float* p0, const float* p1, const float* q0, const float* q1, float* pT) {
    float dx = p1[0] - p0[0];
    float dy = p1[1] - p0[1];
    float ex = q0[0] - q1[0];
    float ey = q0[1] - q1[1];
    float denom = dx * ey - dy * ex;
    if (denom == 0.0f) {
        return 0;
    }
    if (pT) {
        *pT = (ex * (p0[1] - q0[1]) - ey * (p0[0] - q0[0])) / denom;
    }
    return 1;
}

// 0x575460: where the line a + t (b - a) meets the sphere: how many roots (1, 2) and
// the t's; false when it misses
uint8_t __cdecl SphereLine(const Sphere* sphere, const CVector* a, const CVector* b, int* count, float* t1, float* t2) {
    const CVector& c = sphere->center;
    float dx = b->x - a->x;
    float dy = b->y - a->y;
    float dz = b->z - a->z;
    float qa = dx * dx + dy * dy + dz * dz;
    float qb = dx * (a->x - c.x) + dy * (a->y - c.y) + dz * (a->z - c.z);
    qb = qb + qb;
    float qc = c.x * c.x + c.y * c.y + c.z * c.z + a->x * a->x + a->y * a->y + a->z * a->z - Dot(c, *a) * 2.0f -
               sphere->radius * sphere->radius;
    float disc = qb * qb - 4.0f * qa * qc;
    if (disc < 0.0f) {
        return 0;
    }
    if (disc == 0.0f) {
        if (count) {
            *count = 1;
        }
        if (t1) {
            *t1 = -qb / (2.0f * qa);
        }
        return 1;
    }
    if (count) {
        *count = 2;
    }
    if (t1) {
        *t1 = (std::sqrt(std::fabs(disc)) - qb) / (qa + qa);
    }
    if (t2) {
        *t2 = (-qb - std::sqrt(std::fabs(disc))) / (qa + qa);
    }
    return 1;
}

} // namespace ColSweep

// 0x575290
void Plane::Set(const CVector* point, const CVector* n) {
    CVector v = *n;
    CS_Normalise(&v);
    normal = v;
    d = -(v.x * point->x + v.y * point->y + v.z * point->z);
}

// 0x5752F0
void Plane::Set3(const CVector* a, const CVector* b, const CVector* c) {
    CVector u, v;
    CS_VectorSub(&u, a, b);
    CS_VectorSub(&v, c, b);
    CVector n;
    n.x = u.z * v.y - u.y * v.z;
    n.y = v.z * u.x - u.z * v.x;
    n.z = v.x * u.y - u.x * v.y;
    CS_Normalise(&n);
    normal = n;
    d = -(n.x * a->x + n.y * a->y + n.z * a->z);
}

namespace ColSweep {

namespace {

// The triangle's vertices by pointer, the quad's in a row
struct TriangleVertices {
    const re::Ptr32<const CVector>* v;   // (the game's array of 4-byte pointers)
    const CVector& operator[](int i) const { return *v[i]; }
};
struct QuadVertices {
    const CVector* v;
    const CVector& operator[](int i) const { return v[i]; }
};

// 0x575660 / 0x575F00's body: the sphere moving along dir (normalised here) against a
// convex polygon with the normal: its face (the sphere's leading point), its vertices
// (the vertex against the sphere) and its edges (the circle where the edge's plane cuts
// the sphere). The earliest t (in dir's units) wins; hitNormal from the polygon to the
// sphere's center. (The quad writes *t before its early-outs; the triangle after.)
template <int N, class Vertices>
uint8_t SweepSpherePolygon(Vertices verts, const CVector* polyNormal, const Sphere* sphere, const CVector* direction, float* pT,
                           CVector* hitNormal, CVector* hitPoint, uint8_t* face, bool earlyT) {
    CVector dir = *direction;
    CS_Normalise(&dir);
    if (dir.x * polyNormal->x + dir.y * polyNormal->y + dir.z * polyNormal->z > -0.001f) {
        return 0;  // moving away (or along)
    }
    if (earlyT) {
        *pT = 3.4028235e+38f;
    }
    int result = -1;
    Plane plane;
    CS_PlaneSet(&plane, &verts[0], polyNormal);
    float dist = plane.normal.x * sphere->center.x + plane.normal.y * sphere->center.y + plane.normal.z * sphere->center.z + plane.d;
    float negRadius = -sphere->radius;
    if (negRadius > dist) {
        return 0;  // behind
    }
    if (!earlyT) {
        *pT = 3.4028235e+38f;
    }
    CVector tmp, tmp2;

    // the face
    if (sphere->radius < dist) {
        float denom = dir.x * polyNormal->x + dir.y * polyNormal->y + dir.z * polyNormal->z;
        if (denom != 0.0f) {
            float t = -(dist - sphere->radius) / denom;
            CVector offset = {negRadius * polyNormal->x, polyNormal->y * negRadius, negRadius * polyNormal->z};
            CVector move = {t * dir.x, t * dir.y, t * dir.z};
            CVector point = *CS_VectorAdd(&tmp2, CS_VectorAdd(&tmp, &sphere->center, &move), &offset);
            uint8_t inside;
            if constexpr (N == 3) {
                inside = CS_PointInTriangle(&verts[0], &verts[1], &verts[2], &point);
            } else {
                inside = CS_PointInPolygon(N, &verts[0], &point);
            }
            if (inside && *pT > t) {
                *pT = t;
                *hitNormal = *polyNormal;
                *face = 1;
                *hitPoint = point;
                result = 0;
            }
        }
    }

    // the vertices
    for (int i = 0; i < N; i++) {
        CVector v = verts[i];
        CVector end;
        CS_VectorSub(&end, &v, &dir);
        CVector back;
        if constexpr (N == 4) {
            CS_VectorSub(&back, &end, &v);
        }
        int count;
        float t1 = 3.4028235e+38f;
        float t2 = 3.4028235e+38f;
        if (CS_SphereLine(sphere, &v, &end, &count, &t1, &t2)) {
            float t = t1;
            if (t2 < t1) {
                t = t2;
            }
            if (!(t < 0.0f) && *pT > t) {
                *pT = t;
                if constexpr (N == 3) {
                    CS_VectorSub(&back, &end, &v);
                }
                CVector move = {t * back.x, back.y * t, t * back.z};
                CVector center = *CS_VectorAdd(&tmp, &v, &move);
                *hitNormal = *CS_VectorSub(&tmp2, &sphere->center, &center);
                *face = 0;
                *hitPoint = verts[i];
                result = 1;
            }
        }
    }

    // the edges
    for (int i = 0; i < N; i++) {
        CVector p0 = verts[i];
        CVector p1 = verts[i + 1 == N ? 0 : i + 1];
        Plane edgePlane;
        CS_PlaneSet3(&edgePlane, &p0, &p1, CS_VectorSub(&tmp, &p1, &dir));
        float edgeDist = edgePlane.normal.x * sphere->center.x + edgePlane.normal.y * sphere->center.y +
                         edgePlane.normal.z * sphere->center.z + edgePlane.d;
        if (sphere->radius < edgeDist || -sphere->radius > edgeDist) {
            continue;
        }
        // the circle the edge's plane cuts from the sphere, and its point nearest the
        // edge's line
        float circle = std::sqrt(std::fabs(sphere->radius * sphere->radius - edgeDist * edgeDist));
        float along = edgePlane.normal.x * sphere->center.x + edgePlane.normal.y * sphere->center.y +
                      edgePlane.normal.z * sphere->center.z + edgePlane.d;
        CVector centre = {sphere->center.x - along * edgePlane.normal.x, sphere->center.y - along * edgePlane.normal.y,
                          sphere->center.z - along * edgePlane.normal.z};
        CVector closest = {0.0f, 0.0f, 0.0f};
        CS_PointLineDist(&centre, &p0, &p1, &closest);
        CVector toEdge;
        CS_VectorSub(&toEdge, &closest, &centre);
        CS_Normalise(&toEdge);
        CVector scaled = {circle * toEdge.x, toEdge.y * circle, circle * toEdge.z};
        CVector lead = *CS_VectorAdd(&tmp, &scaled, &centre);
        // the 2D crossing on the plane's two largest axes
        float ax = std::fabs(edgePlane.normal.x);
        float ay = std::fabs(edgePlane.normal.y);
        float az = std::fabs(edgePlane.normal.z);
        int i0 = 0;
        int i1 = 1;
        if (!(ay < ax) || !(az < ax)) {
            if (ay > az) {
                i1 = 2;
            }
        } else {
            i0 = 1;
            i1 = 2;
        }
        CVector leadMoved = *CS_VectorAdd(&tmp2, &lead, &dir);
        const float* l = &lead.x;
        const float* m = &leadMoved.x;
        const float* a = &p0.x;
        const float* b = &p1.x;
        float lead2[2] = {l[i0], l[i1]};
        float moved2[2] = {m[i0], m[i1]};
        float a2[2] = {a[i0], a[i1]};
        float b2[2] = {b[i0], b[i1]};
        float t;
        if (!CS_LineIntersect2D(lead2, moved2, a2, b2, &t) || t < 0.0f) {
            continue;
        }
        CVector move = {dir.x * t, dir.y * t, t * dir.z};
        CVector contact = *CS_VectorAdd(&tmp, &lead, &move);
        CVector toA, toB;
        CS_VectorSub(&toA, &p0, &contact);
        CS_VectorSub(&toB, &p1, &contact);
        if (toB.x * toA.x + toB.y * toA.y + toB.z * toA.z > 0.0f) {
            continue;  // beyond the edge's ends
        }
        if (*pT < t) {
            continue;
        }
        *pT = t;
        *hitNormal = *CS_VectorSub(&tmp2, &sphere->center, &lead);
        *face = 0;
        *hitPoint = contact;
        result = 2;
    }
    if (result != -1) {
        CS_Normalise(hitNormal);
    }
    return result != -1;
}

} // namespace

// 0x575660: a sphere moving along dir against a triangle (its vertices by pointer)
uint8_t __cdecl SweepSphereTriangle(const re::Ptr32<const CVector>* triangle, const CVector* normal, const Sphere* sphere, const CVector* dir,
                                    float* t, CVector* hitNormal, CVector* hitPoint, uint8_t* face) {
    return SweepSpherePolygon<3>(TriangleVertices{triangle}, normal, sphere, dir, t, hitNormal, hitPoint, face, false);
}

// 0x575F00: the same against a quad (four vertices in a row)
uint8_t __cdecl SweepSphereQuad(const CVector* quad, const CVector* normal, const Sphere* sphere, const CVector* dir, float* t,
                                CVector* hitNormal, CVector* hitPoint, uint8_t* face) {
    return SweepSpherePolygon<4>(QuadVertices{quad}, normal, sphere, dir, t, hitNormal, hitPoint, face, true);
}

// 0x5767B0: the same against a box's six faces (quads, outward normals); how many
// faces it touches counted into *touched
uint8_t __cdecl SweepSphereBox(const CVector* boxMin, const CVector* boxMax, const Sphere* sphere, const CVector* dir, float* pT,
                               CVector* hitNormal, CVector* hitPoint, uint8_t* face, int* touched) {
    float t = 3.4028235e+38f;
    *pT = 3.4028235e+38f;
    CVector normal = {};
    CVector faceNormal = {};
    CVector facePoint = {};
    CVector quad[4] = {};
    uint8_t hit = 0;
    const CVector& lo = *boxMin;
    const CVector& hi = *boxMax;
    for (int side = 0; side < 6; side++) {
        switch (side) {
        case 0:
            normal = {-1.0f, 0.0f, 0.0f};
            quad[0] = {lo.x, hi.y, hi.z};
            quad[1] = {lo.x, lo.y, hi.z};
            quad[2] = {lo.x, lo.y, lo.z};
            quad[3] = {lo.x, hi.y, lo.z};
            break;
        case 1:
            normal = {0.0f, -1.0f, 0.0f};
            quad[0] = {hi.x, lo.y, hi.z};
            quad[1] = {lo.x, lo.y, hi.z};
            quad[2] = {lo.x, lo.y, lo.z};
            quad[3] = {hi.x, lo.y, lo.z};
            break;
        case 2:
            normal = {0.0f, 0.0f, -1.0f};
            quad[0] = {hi.x, hi.y, lo.z};
            quad[1] = {lo.x, hi.y, lo.z};
            quad[2] = {lo.x, lo.y, lo.z};
            quad[3] = {hi.x, lo.y, lo.z};
            break;
        case 3:
            normal = {1.0f, 0.0f, 0.0f};
            quad[0] = {hi.x, hi.y, hi.z};
            quad[1] = {hi.x, lo.y, hi.z};
            quad[2] = {hi.x, lo.y, lo.z};
            quad[3] = {hi.x, hi.y, lo.z};
            break;
        case 4:
            normal = {0.0f, 1.0f, 0.0f};
            quad[0] = {hi.x, hi.y, hi.z};
            quad[1] = {lo.x, hi.y, hi.z};
            quad[2] = {lo.x, hi.y, lo.z};
            quad[3] = {hi.x, hi.y, lo.z};
            break;
        case 5:
            normal = {0.0f, 0.0f, 1.0f};
            quad[0] = {hi.x, hi.y, hi.z};
            quad[1] = {lo.x, hi.y, hi.z};
            quad[2] = {lo.x, lo.y, hi.z};
            quad[3] = {hi.x, lo.y, hi.z};
            break;
        }
        if (CS_SweepSphereQuad(quad, &normal, sphere, dir, &t, &faceNormal, &facePoint, face) && t < *pT) {
            *pT = t;
            *hitNormal = faceNormal;
            *hitPoint = facePoint;
            ++*touched;
            hit = 1;
        }
    }
    return hit;
}

// 0x576B50: two spheres moving (velocities a and b, over t in [0, maxT]): when they
// first touch and where (between the centers in proportion to the radii); already
// touching: t 0, midway between the centers
uint8_t __cdecl SweepSphereSphere(const Sphere* a, const Sphere* b, float maxT, const CVector* velA, const CVector* velB, float* pT,
                                  CVector* point) {
    CVector rv, d, tmp, tmp2;
    CS_VectorSub(&rv, velB, velA);
    float qa = rv.x * rv.x + rv.y * rv.y + rv.z * rv.z;
    CS_VectorSub(&d, &b->center, &a->center);
    float qc = d.x * d.x + d.y * d.y + d.z * d.z;
    float radii = b->radius + a->radius;
    float radiiSq = radii * radii;
    if (!(qa > 0.0f)) {
        if (radiiSq < qc) {
            return 0;
        }
    } else {
        float qb = d.x * rv.x + d.y * rv.y + d.z * rv.z;
        if (!(qb <= 0.0f)) {
            return 0;  // separating
        }
        if (!(-maxT * qa <= qb) && radiiSq < qc + maxT * (qa * maxT + (qb + qb))) {
            return 0;  // doesn't get there within maxT
        }
        float c = qc - radiiSq;
        float disc = qb * qb - qa * c;
        if (disc < 0.0f) {
            return 0;
        }
        if (!(c <= 0.0f)) {
            float t = -((std::sqrt(std::fabs(disc)) + qb) / qa);
            *pT = t;
            if (t < 0.0f) {
                *pT = 0.0f;
            } else if (maxT < t) {
                *pT = maxT;
            }
            t = *pT;
            CVector move = {t * rv.x, rv.y * t, t * rv.z};
            CVector rel = *CS_VectorAdd(&tmp, &d, &move);
            float share = a->radius / radii;
            CVector towards = {share * rel.x, rel.y * share, share * rel.z};
            t = *pT;
            CVector moveA = {t * velA->x, velA->y * t, t * velA->z};
            *point = *CS_VectorAdd(&tmp2, CS_VectorAdd(&tmp, &a->center, &moveA), &towards);
            return 1;
        }
    }
    *pT = 0.0f;
    CS_VectorAdd(&tmp, &a->center, &b->center);
    point->x = tmp.x * 0.5f;
    point->y = tmp.y * 0.5f;
    point->z = 0.5f * tmp.z;
    return 1;
}

} // namespace ColSweep

// ---- registration: all original until tested ---------------------------------------

#define COLSWEEP_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(colsweep_original_, __LINE__);                                    \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(colsweep_original_, __LINE__), re::Mode::Original)

COLSWEEP_REVERSED("ColSweep::SegSegDistSq", 0x00572DD0, &ColSweep::SegSegDistSq);
COLSWEEP_REVERSED("ColSweep::SegTriDistSq", 0x00573560, &ColSweep::SegTriDistSq);
COLSWEEP_REVERSED("ColSweep::PointSegDistSq", 0x005748A0, &ColSweep::PointSegDistSq);
COLSWEEP_REVERSED("ColSweep::PointSegDist", 0x00574A00, &ColSweep::PointSegDist);
COLSWEEP_REVERSED("ColSweep::PointTriDistSq", 0x00574A30, &ColSweep::PointTriDistSq);
COLSWEEP_REVERSED("ColSweep::PointInTriangle", 0x00574F10, &ColSweep::PointInTriangle);
COLSWEEP_REVERSED("ColSweep::PointInPolygon", 0x005750E0, &ColSweep::PointInPolygon);
COLSWEEP_REVERSED("ColSweep::PointLineDistSq", 0x00575130, &ColSweep::PointLineDistSq);
COLSWEEP_REVERSED("ColSweep::PointLineDist", 0x00575250, &ColSweep::PointLineDist);
COLSWEEP_REVERSED("ColSweep::Plane::Set", 0x00575290, &Plane::Set);
COLSWEEP_REVERSED("ColSweep::Plane::Set3", 0x005752F0, &Plane::Set3);
COLSWEEP_REVERSED("ColSweep::LineIntersect2D", 0x005753B0, &ColSweep::LineIntersect2D);
COLSWEEP_REVERSED("ColSweep::SphereLine", 0x00575460, &ColSweep::SphereLine);
COLSWEEP_REVERSED("ColSweep::SweepSphereTriangle", 0x00575660, &ColSweep::SweepSphereTriangle);
COLSWEEP_REVERSED("ColSweep::SweepSphereQuad", 0x00575F00, &ColSweep::SweepSphereQuad);
COLSWEEP_REVERSED("ColSweep::SweepSphereBox", 0x005767B0, &ColSweep::SweepSphereBox);
COLSWEEP_REVERSED("ColSweep::SweepSphereSphere", 0x00576B50, &ColSweep::SweepSphereSphere);
