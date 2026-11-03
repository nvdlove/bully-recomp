// Geometry helpers: CRect growing to take points and rects in, 2D line and segment
// intersection (Eberly's), and a few CVector ones (magnitude, a safe normalise, a
// direction clamped into a cone).
// (x87 extended precision in the game; what it stores as floats is kept as floats.)

#include <cmath>
#include <cstdint>

#include "reversed.h"

namespace c01geom {

struct CVector {
    float x, y, z;
};
struct CVector2D {
    float x, y;
};
// GTA's CRect: top is the larger y
struct CRect {
    float left;     // +0
    float top;      // +4
    float right;    // +8
    float bottom;   // +0xC
};
static_assert(sizeof(CRect) == 0x10);

GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(uint8_t, __cdecl, IntersectLines2D, 0x004147D0, const CVector2D* p0, const CVector2D* d0, const CVector2D* p1,
        const CVector2D* d1, CVector2D* diff, float* sqrLength0, int32_t* type, float* s);

float Sqrt(float a) {
    return static_cast<float>(std::sqrt(static_cast<double>(a)));
}

// 0x414410 (thiscall): grown to take in another rect; returns this
CRect* __fastcall Rect_Union(CRect* self, re::Edx, const CRect* other) {
    if (other->left < self->left) {
        self->left = other->left;
    }
    if (self->right < other->right) {
        self->right = other->right;
    }
    if (other->bottom < self->bottom) {
        self->bottom = other->bottom;
    }
    if (self->top < other->top) {
        self->top = other->top;
    }
    return self;
}

// 0x4144B0 (thiscall): grown to take in the point (x, y); returns this (every side
// stored, changed or not)
CRect* __fastcall Rect_ExtendXY(CRect* self, re::Edx, float x, float y) {
    self->left = x < self->left ? x : self->left;
    self->right = self->right < x ? x : self->right;
    self->bottom = y < self->bottom ? y : self->bottom;
    self->top = self->top < y ? y : self->top;
    return self;
}

// 0x414570 (thiscall): grown to take in the point; returns this (every side stored,
// changed or not)
CRect* __fastcall Rect_Extend(CRect* self, re::Edx, const CVector2D* point) {
    self->left = point->x < self->left ? point->x : self->left;
    self->right = self->right < point->x ? point->x : self->right;
    self->bottom = point->y < self->bottom ? point->y : self->bottom;
    self->top = self->top < point->y ? point->y : self->top;
    return self;
}

// 0x4147D0: the lines p0 + s d0 and p1 + t d1: type 1 crossing (s[0], s[1] the
// parameters), 0 parallel apart, 2 the same line. `diff` = p1 - p0 and `sqrLength0` =
// |d0|^2 are left for the caller. True unless parallel apart.
uint8_t __cdecl IntersectLines(const CVector2D* p0, const CVector2D* d0, const CVector2D* p1, const CVector2D* d1,
                               CVector2D* diff, float* sqrLength0, int32_t* type, float* s) {
    float kross = d1->x * d0->y - d0->x * d1->y;
    float dx = p1->x - p0->x;
    float dy = p1->y - p0->y;
    diff->x = dx;
    diff->y = dy;
    float sqrLen0 = d0->x * d0->x + d0->y * d0->y;
    *sqrLength0 = sqrLen0;
    double epsilon = sqrLen0 * 1e-06;
    float sqrLen1 = d1->y * d1->y + d1->x * d1->x;
    float limit = static_cast<float>(sqrLen1 * epsilon);
    if (limit < static_cast<double>(kross) * kross) {
        *type = 1;
        float invKross = 1.0f / kross;
        s[0] = (d1->x * diff->y - diff->x * d1->y) * invKross;
        s[1] = (d0->x * diff->y - diff->x * d0->y) * invKross;
        return *type != 0;
    }
    float kross2 = d0->x * diff->y - diff->x * d0->y;
    float sqrLenE = diff->y * diff->y + diff->x * diff->x;
    limit = static_cast<float>(sqrLenE * epsilon);
    if (limit < static_cast<double>(kross2) * kross2) {
        *type = 0;
    } else {
        *type = 2;
    }
    return *type != 0;
}

// 0x414930: the segments p0 + s d0 and p1 + t d1 (s, t in [0, 1]): type 1 one point
// (s[0], s[1]), 2 an overlap (s[0]..s[1] along the first), 0 none
uint8_t __cdecl IntersectSegments(const CVector2D* p0, const CVector2D* d0, const CVector2D* p1, const CVector2D* d1,
                                  int32_t* type, float* s) {
    CVector2D diff;
    float sqrLen0;
    if (!IntersectLines2D(p0, d0, p1, d1, &diff, &sqrLen0, type, s)) {
        return *type != 0;
    }
    if (*type == 1) {
        if (0.0f <= s[0] && s[0] <= 1.0f && 0.0f <= s[1]) {
            if (1.0f < s[1]) {
                *type = 0;
            }
            return *type != 0;
        }
        *type = 0;
        return *type != 0;
    }
    float dot = d0->x * d1->x + d1->y * d0->y;
    float s0 = d0->y * diff.y + d0->x * diff.x;
    float s1 = s0 + dot;
    float smin = s0;
    float smax = s1;
    if (dot <= 0.0f) {
        smin = s1;
        smax = s0;
    }
    if (0.0f <= smax && smin <= sqrLen0) {
        if (smax <= 0.0f) {
            *type = 1;
            s[0] = 0.0f;
            return *type != 0;
        }
        if (smin < sqrLen0) {
            *type = 2;
            s[0] = 0.0f <= smin ? (1.0f / sqrLen0) * smin : 0.0f;
            s[1] = sqrLen0 < smax ? 1.0f : smax * (1.0f / sqrLen0);
            return *type != 0;
        }
        *type = 1;
        s[0] = 1.0f;
        return *type != 0;
    }
    *type = 0;
    return *type != 0;
}

// 0x414E70 (thiscall): the vector turned (its length kept) into the cone around `axis`
// (a unit vector) of cosine `cosLimit`; false when it's inside already
uint8_t __fastcall ClampToCone(CVector* self, re::Edx, const CVector* axis, float cosLimit) {
    float length = Sqrt(std::fabs(self->y * self->y + self->x * self->x + self->z * self->z));
    CVector dir = *self;
    Vector_Normalise(&dir);
    float dot = axis->x * dir.x + axis->y * dir.y + axis->z * dir.z;
    if (!(cosLimit > dot)) {
        return 0;
    }
    float sinLimit = Sqrt(std::fabs(static_cast<float>(1.0f - cosLimit * cosLimit)));
    CVector along = {axis->x * dot, axis->y * dot, dot * axis->z};
    CVector perp = {dir.x - along.x, dir.y - along.y, dir.z - along.z};
    Vector_Normalise(&perp);
    CVector cone = {axis->x * cosLimit, axis->y * cosLimit, cosLimit * axis->z};
    CVector side = {perp.x * sinLimit, perp.y * sinLimit, sinLimit * perp.z};
    CVector result = {side.x + cone.x, side.y + cone.y, side.z + cone.z};
    Vector_Normalise(&result);
    self->x = result.x * length;
    self->y = result.y * length;
    self->z = length * result.z;
    return 1;
}

// 0x415100 (thiscall)
float __fastcall Magnitude(const CVector* self, re::Edx) {
    return Sqrt(self->y * self->y + self->x * self->x + self->z * self->z);
}

// 0x415130 (thiscall): normalised unless its length is 1e-6 or less (then zeroed);
// returns the length
float __fastcall NormaliseSafe(CVector* self, re::Edx) {
    float length = Sqrt(self->y * self->y + self->x * self->x + self->z * self->z);
    if (length > 1e-06f) {
        float inv = 1.0f / length;
        self->x = inv * self->x;
        self->y = inv * self->y;
        self->z = inv * self->z;
        return length;
    }
    self->x = 0.0f;
    self->y = 0.0f;
    self->z = 0.0f;
    return 0.0f;
}

} // namespace c01geom

// ---- registration ----------------------------------------------------------------------

#define GEOM_REVERSED_C01(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace c01geom {
GEOM_REVERSED_C01("CRect::Union", 0x00414410, &Rect_Union);
GEOM_REVERSED_C01("CRect::Extend (x, y)", 0x004144B0, &Rect_ExtendXY);
GEOM_REVERSED_C01("CRect::Extend", 0x00414570, &Rect_Extend);
GEOM_REVERSED_C01("Intersection2D::Lines", 0x004147D0, &IntersectLines);
GEOM_REVERSED_C01("Intersection2D::Segments", 0x00414930, &IntersectSegments);
GEOM_REVERSED_C01("CVector::ClampToCone", 0x00414E70, &ClampToCone);
GEOM_REVERSED_C01("CVector::Magnitude", 0x00415100, &Magnitude);
GEOM_REVERSED_C01("CVector::NormaliseSafe", 0x00415130, &NormaliseSafe);
} // namespace c01geom
