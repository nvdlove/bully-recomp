// A line segment's geometry (0x20 bytes: start and end, a w after each; the avoidance's
// and the nav meshes' door lines): midpoint, 2D crossings, distances, the closest point
// and a point ahead along it. 0x5E6AE0..0x5E76C0 (Ghidra's JitterMovementInfo names are
// wrong). Length (0x5DCF40) and PointAt (0x5DCFA0) are in PedAvoidance.cpp.
//
// The CRT's x87 sqrt and acos (0x85C500, 0x85CE00) are the C library's here; values are
// stored as floats where the game stores them.

#include <cmath>
#include <cstdint>

#include "reversed.h"

namespace {

struct SgVec {
    float x, y, z;
};
struct SgSegment {
    SgVec start;   // +0x0
    float w0;      // +0xC
    SgVec end;     // +0x10
    float w1;      // +0x1C
};
static_assert(sizeof(SgSegment) == 0x20);

GAME_FN(float, __thiscall, SG_Length, 0x005DCF40, const SgSegment* self);
GAME_FN(SgVec*, __thiscall, SG_PointAt, 0x005DCFA0, const SgSegment* self, SgVec* out, float t);
GAME_FN(SgVec*, __thiscall, SG_Midpoint, 0x005E6AE0, const SgSegment* self, SgVec* out);
GAME_FN(float, __thiscall, SG_AngleTo, 0x005E7140, const SgSegment* self, const SgVec* v);
GAME_FN(SgVec*, __thiscall, SG_Closest, 0x005E74A0, const SgSegment* self, SgVec* out, const SgVec* p, float* t);
GAME_FN(float, __thiscall, SG_Normalise2, 0x004294A0, SgVec* v);
GAME_FN(float, __cdecl, SG_RecipSqrt, 0x00414C20, float a, float b);

float SqrtAbs(double v) {
    float f = std::fabs(float(v));
    return float(std::sqrt(double(f)));
}
// the game's |a - b| with its x87 order ((y*y + x*x) + z*z)
float Dist3(const SgVec& a, const SgVec& b) {
    float x = a.x - b.x;
    float y = a.y - b.y;
    float z = a.z - b.z;
    return SqrtAbs(double(y) * y + double(x) * x + double(z) * z);
}

// 0x5E6AE0 (thiscall): the midpoint
SgVec* __fastcall Midpoint(const SgSegment* self, re::Edx, SgVec* out) {
    float x = self->end.x + self->start.x;
    float y = self->end.y + self->start.y;
    float z = self->end.z + self->start.z;
    out->x = float(x * 0.5);
    out->y = float(y * 0.5);
    out->z = float(z * 0.5);
    return out;
}

// 0x5E6B30 (thiscall): do the two cross (flat, strictly inside both)? The parameter along
// this one, and the point (4 dwords copied: the fourth is the game's uninitialised stack,
// 0 here)
uint8_t __fastcall Intersect2D(const SgSegment* self, re::Edx, const SgSegment* other, float* tOut, float* pointOut) {
    float ux = self->end.x - self->start.x;
    float uy = self->end.y - self->start.y;
    float vx = other->end.x - other->start.x;
    float vy = other->end.y - other->start.y;
    float det = float(double(ux) * vy - double(vx) * uy);
    if (det == 0.0f) {
        return 0;
    }
    float wx = self->start.x - other->start.x;
    float wy = self->start.y - other->start.y;
    float t = float((double(vx) * wy - double(vy) * wx) / det);
    if (t < 0.0f || t == 0.0f) {
        return 0;
    }
    if (1.0f < t || 1.0f == t) {
        return 0;
    }
    float s = float((double(ux) * wy - double(wx) * uy) / det);
    if (s < 0.0f || s == 0.0f) {
        return 0;
    }
    if (1.0f <= s) {
        return 0;
    }
    if (tOut) {
        *tOut = t;
    }
    if (pointOut) {
        float point[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        SG_PointAt(self, reinterpret_cast<SgVec*>(point), t);
        pointOut[0] = point[0];
        pointOut[1] = point[1];
        pointOut[2] = point[2];
        pointOut[3] = point[3];
    }
    return 1;
}

// 0x5E6CB0 (thiscall): where the other line crosses this one (flat), kept `margin` in
// from the ends; the midpoint when it's too short or they're parallel
SgVec* __fastcall CrossingClamped(const SgSegment* self, re::Edx, SgVec* out, const SgSegment* other, float margin) {
    float ux = self->end.x - self->start.x;
    float uy = self->end.y - self->start.y;
    float length = SqrtAbs(double(ux) * ux + double(uy) * uy);
    float m = float(double(margin) / length);
    if (m > 0.5f) {
        float x = self->start.x + self->end.x;
        float y = self->end.y + self->start.y;
        float z = self->end.z + self->start.z;
        out->x = float(x * 0.5);
        out->y = float(y * 0.5);
        out->z = float(z * 0.5);
        return out;
    }
    float vx = other->end.x - other->start.x;
    float vy = other->end.y - other->start.y;
    float det = float(double(vy) * ux - double(vx) * uy);
    if (det == 0.0f) {
        SG_Midpoint(self, out);
        return out;
    }
    float wx = self->start.x - other->start.x;
    float wy = self->start.y - other->start.y;
    float t = float((double(vx) * wy - double(vy) * wx) / det);
    float oneMinus = 1.0f - m;
    float r;
    if (t < m) {
        r = m;
    } else if (oneMinus < t) {
        r = oneMinus;
    } else {
        r = t;
    }
    SG_PointAt(self, out, r);
    return out;
}

// 0x5E6E60 (thiscall): the flat distance from p to the segment
float __fastcall FlatDistance(const SgSegment* self, re::Edx, const float* p) {
    float dx = self->start.x - self->end.x;
    float dy = self->start.y - self->end.y;
    float lengthSq = float(double(dx) * dx + double(dy) * dy);
    float cx;
    float cy;
    if (!(lengthSq > 0.0f)) {
        cx = self->start.x;
        cy = self->start.y;
    } else {
        double a = (double(self->end.x) - self->start.x) * (double(p[0]) - self->start.x);
        double b = (double(self->end.y) - self->start.y) * (double(p[1]) - self->start.y);
        float t = float(double(float(a + b)) / lengthSq);
        if (t < 0.0f) {
            t = 0.0f;
        }
        if (t > 1.0f) {
            t = 1.0f;
        }
        cx = float(self->start.x + t * (double(self->end.x) - self->start.x));
        cy = float(t * (double(self->end.y) - self->start.y) + self->start.y);
    }
    float ex = cx - p[0];
    float ey = cy - p[1];
    return SqrtAbs(double(ex) * ex + double(ey) * ey);
}

// 0x5E7140 (thiscall): the angle between the segment and v (acos of the clamped cosine)
float __fastcall AngleTo(const SgSegment* self, re::Edx, const SgVec* v) {
    SgVec d = {self->end.x - self->start.x, self->end.y - self->start.y, self->end.z - self->start.z};
    float dot = float(double(v->y) * d.y + double(v->x) * d.x + double(v->z) * d.z);
    float vLength = SqrtAbs(double(v->y) * v->y + double(v->x) * v->x + double(v->z) * v->z);
    float length = SG_Length(self);
    float c = float(dot / (double(length) * vLength));
    if (c < -1.0f) {
        c = -1.0f;
    } else if (1.0f < c) {
        c = 1.0f;
    }
    return float(std::acos(double(c)));
}

// 0x5E7220 (thiscall): the flat normal (d.y, -d.x, 0)
SgVec* __fastcall Normal2D(const SgSegment* self, re::Edx, SgVec* out) {
    float dx = self->end.x - self->start.x;
    out->x = self->end.y - self->start.y;
    out->y = -dx;
    out->z = 0.0f;
    return out;
}

// 0x5E7250 (thiscall): the distance from p to the segment (3D)
float __fastcall Distance3D(const SgSegment* self, re::Edx, const SgVec* p) {
    SgVec w = {p->x - self->start.x, p->y - self->start.y, p->z - self->start.z};
    SgVec d = {self->end.x - self->start.x, self->end.y - self->start.y, self->end.z - self->start.z};
    SgVec n = d;
    SG_Normalise2(&n);
    float proj = float(double(n.y) * w.y + double(n.x) * w.x + double(n.z) * w.z);
    if (proj < 0.0f) {
        return Dist3(self->start, *p);
    }
    float length = SqrtAbs(double(d.x) * d.x + double(d.y) * d.y + double(d.z) * d.z);
    if (length < proj) {
        return Dist3(self->end, *p);
    }
    SgVec f = {n.x * proj, n.y * proj, proj * n.z};
    SgVec foot = {f.x + self->start.x, f.y + self->start.y, self->start.z + f.z};
    return Dist3(foot, *p);
}

// 0x5E74A0 (thiscall): the point of the segment nearest p (measured flat along its
// direction) and its parameter
SgVec* __fastcall Closest(const SgSegment* self, re::Edx, SgVec* out, const SgVec* p, float* t) {
    float wx = p->x - self->start.x;
    float wy = p->y - self->start.y;
    SgVec d = {self->end.x - self->start.x, self->end.y - self->start.y, self->end.z - self->start.z};
    SgVec n = d;
    SG_Normalise2(&n);
    float flatSq = float(double(n.x) * n.x + double(n.y) * n.y);
    float r = SG_RecipSqrt(1.0f, flatSq);
    float mx = r * n.x;
    float my = r * n.y;
    float proj = float(double(my) * wy + double(mx) * wx + 0.0);
    float length = SqrtAbs(double(d.x) * d.x + double(d.y) * d.y);
    if (proj == 0.0f || length == 0.0f) {
        *t = 0.0f;
    } else {
        *t = proj / length;
    }
    if (proj <= 0.0f) {
        *out = self->start;
        return out;
    }
    if (length <= proj) {
        *out = self->end;
        return out;
    }
    SgVec f = {n.x * proj, n.y * proj, proj * n.z};
    out->x = self->start.x + f.x;
    out->y = f.y + self->start.y;
    out->z = self->start.z + f.z;
    return out;
}

// 0x5E7680 (thiscall): the angle between two segments
float __fastcall AngleToSegment(const SgSegment* self, re::Edx, const SgSegment* other) {
    SgVec d = {other->end.x - other->start.x, other->end.y - other->start.y, other->end.z - other->start.z};
    return SG_AngleTo(self, &d);
}

// 0x5E76C0 (thiscall): the point `radius` ahead of p's nearest along the segment (where
// a circle of that radius round p leaves it); t advanced to match
SgVec* __fastcall PointAhead(const SgSegment* self, re::Edx, SgVec* out, const SgVec* p, float radius, float* t) {
    SgVec closest;
    SG_Closest(self, &closest, p, t);
    if (1.0f <= *t) {
        *out = self->end;
        return out;
    }
    float dx = self->end.x - self->start.x;
    float dy = self->end.y - self->start.y;
    float nx = -dx;
    float r = SG_RecipSqrt(1.0f, float(double(nx) * nx + double(dy) * dy));
    float mx = r * dy;
    float my = r * nx;
    SgVec w = {self->start.x - p->x, self->start.y - p->y, self->start.z - p->z};
    float side = std::fabs(float(double(w.y) * my + double(w.x) * mx + w.z * 0.0));
    if (radius < side) {
        *out = closest;
        return out;
    }
    SgVec base;
    if (0.0f < *t || std::isnan(*t)) {
        base = closest;
    } else {
        SgVec at;
        base = *SG_PointAt(self, &at, *t);
    }
    float radiusSq = radius * radius;
    float sideSq = side * side;
    float h = SqrtAbs(double(radiusSq) - sideSq);
    float length = SG_Length(self);
    *t = float(double(h) / length) + *t;
    SgVec n = {self->end.x - self->start.x, self->end.y - self->start.y, self->end.z - self->start.z};
    SG_Normalise2(&n);
    if (1.0f <= *t) {
        *out = self->end;
        return out;
    }
    if (!(0.0f < *t) && !std::isnan(*t)) {
        *out = self->start;
        return out;
    }
    SgVec e = {n.x * h, n.y * h, h * n.z};
    out->x = e.x + base.x;
    out->y = e.y + base.y;
    out->z = e.z + base.z;
    return out;
}

#define AVOIDSEG_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(AvoidSegOriginal_, __LINE__);                                        \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(AvoidSegOriginal_, __LINE__), \
             re::Mode::Original)

AVOIDSEG_REVERSED("AvoidSegment::Midpoint", 0x005E6AE0, &Midpoint);
AVOIDSEG_REVERSED("AvoidSegment::Intersect2D", 0x005E6B30, &Intersect2D);
AVOIDSEG_REVERSED("AvoidSegment::CrossingClamped", 0x005E6CB0, &CrossingClamped);
AVOIDSEG_REVERSED("AvoidSegment::FlatDistance", 0x005E6E60, &FlatDistance);
AVOIDSEG_REVERSED("AvoidSegment::AngleTo", 0x005E7140, &AngleTo);
AVOIDSEG_REVERSED("AvoidSegment::Normal2D", 0x005E7220, &Normal2D);
AVOIDSEG_REVERSED("AvoidSegment::Distance3D", 0x005E7250, &Distance3D);
AVOIDSEG_REVERSED("AvoidSegment::Closest", 0x005E74A0, &Closest);
AVOIDSEG_REVERSED("AvoidSegment::AngleToSegment", 0x005E7680, &AngleToSegment);
AVOIDSEG_REVERSED("AvoidSegment::PointAhead", 0x005E76C0, &PointAhead);

} // namespace
