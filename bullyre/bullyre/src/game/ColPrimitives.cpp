// Collision primitives (0x564310..0x569330): CColLine/CColBox setters, a use point's
// area, triangle normals and planes, the point-to-segment distances, the line against
// a box or sphere, sphere against box, and the compressed vertices. (GTA San Andreas'
// CCollision helpers where they match.)

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "reversed.h"

namespace {

using col::CVector;

// CColLine: start and end (16-byte vectors; w whatever the setter's stack held)
struct ColLine {
    float start[4];   // +0
    float end[4];     // +0x10
};
static_assert(sizeof(ColLine) == 0x20);

struct ColBox {
    float min[4];     // +0
    float max[4];     // +0x10
    uint8_t surface;  // +0x20
    uint8_t piece;    // +0x21
    uint8_t pad[2];
};
static_assert(sizeof(ColBox) == 0x24);

GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, CrossProduct, 0x00414730, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(col::CMatrix*, __thiscall, Entity_GetMatrix, 0x0046DD20, void* entity);
GAME_FN(void, __thiscall, Matrix_CopyConstruct, 0x00412A10, col::CMatrix* self, const col::CMatrix* other);
// (0x0045D500 stores 16 bytes through `out4`: called through re::CallVec16)
static inline void MultiplyPoint4(float* out4, const col::CMatrix* m, const float* v) { re::CallVec16<0x0045D500, void>(out4, m, v); }
GAME_FN(float, __cdecl, DistToLine_, 0x005691A0, const CVector* a, const CVector* b, const CVector* p);

float Sq(float v) {
    return float(double(v) * v);
}

// ---- 0x564310: the squared distance from a 2D point to a rectangle (0 inside)
float __cdecl DistSqToRect2D(const float* min, const float* max, const float* p) {
    if (min[0] > p[0]) {
        float dx = p[0] - min[0];
        if (min[1] > p[1]) {
            float dy = p[1] - min[1];
            return float(Sq(dx) + Sq(dy));
        }
        if (max[1] < p[1]) {
            float dy = p[1] - max[1];
            return float(Sq(dx) + Sq(dy));
        }
        return Sq(dx);
    }
    if (p[0] <= max[0]) {
        if (min[1] > p[1]) {
            return Sq(p[1] - min[1]);
        }
        if (max[1] < p[1]) {
            return Sq(p[1] - max[1]);
        }
        return 0.0f;
    }
    float dx = p[0] - max[0];
    if (min[1] > p[1]) {
        float dy = p[1] - min[1];
        return float(Sq(dx) + Sq(dy));
    }
    if (max[1] < p[1]) {
        float dy = p[1] - max[1];
        return float(Sq(dx) + Sq(dy));
    }
    return Sq(dx);
}

// ---- 0x564700 (thiscall on a use point's box, ret 8): its four corners (min, x max,
// max, x min at y max) placed by the entity's matrix
void __fastcall UsePoint_GetArea(const float* self, re::Edx, void* entity, CVector* corners) {
    col::CMatrix m;
    Matrix_CopyConstruct(&m, Entity_GetMatrix(entity));
    float points[4][4] = {
        {self[0], self[1], self[2], self[3]},
        {self[4], self[1], self[2], self[3]},
        {self[4], self[5], self[6], self[7]},
        {self[0], self[5], self[6], self[7]},
    };
    for (int i = 0; i < 4; i++) {
        float out[4];
        MultiplyPoint4(out, &m, points[i]);
        corners[i] = {out[0], out[1], out[2]};
    }
}

// ---- 0x564830 (thiscall): CColLine's constructor from two points (w: 0 here; the
// game's is whatever was on its stack)
ColLine* __fastcall ColLine_Construct(ColLine* self, re::Edx, const CVector* start, const CVector* end) {
    self->start[0] = start->x;
    self->start[1] = start->y;
    self->start[2] = start->z;
    self->start[3] = 0.0f;
    self->end[0] = end->x;
    self->end[1] = end->y;
    self->end[2] = end->z;
    self->end[3] = 0.0f;
    return self;
}

// ---- 0x5648A0 (thiscall): CColBox::Set (w: 0 here, as above)
void __fastcall ColBox_Set(ColBox* self, re::Edx, const CVector* min, const CVector* max, int surface, int piece) {
    self->min[0] = min->x;
    self->min[1] = min->y;
    self->min[2] = min->z;
    self->min[3] = 0.0f;
    self->max[0] = max->x;
    self->max[1] = max->y;
    self->max[2] = max->z;
    self->max[3] = 0.0f;
    self->surface = uint8_t(surface);
    self->piece = uint8_t(piece);
}

// ---- 0x564920 (thiscall): CColLine::Set (x, y, z only)
void __fastcall ColLine_Set(ColLine* self, re::Edx, const CVector* start, const CVector* end) {
    self->start[0] = start->x;
    self->start[1] = start->y;
    self->start[2] = start->z;
    self->end[0] = end->x;
    self->end[1] = end->y;
    self->end[2] = end->z;
}

// ---- 0x564950 (thiscall): the same, zeroed first; returns this
ColLine* __fastcall ColLine_Construct2(ColLine* self, re::Edx, const CVector* start, const CVector* end) {
    self->start[0] = 0.0f;
    self->start[1] = 0.0f;
    self->start[2] = 0.0f;
    self->end[0] = 0.0f;
    self->end[1] = 0.0f;
    self->end[2] = 0.0f;
    self->start[0] = start->x;
    self->start[1] = start->y;
    self->start[2] = start->z;
    self->end[0] = end->x;
    self->end[1] = end->y;
    self->end[2] = end->z;
    return self;
}

// ---- 0x5649C0: a triangle's normal ((c - a) x (b - a), normalised)
void __cdecl TriangleNormal(const CVector* a, const CVector* b, const CVector* c, CVector* normal) {
    CVector ab, ac, n;
    VectorSub(&ab, b, a);
    VectorSub(&ac, c, a);
    *normal = *CrossProduct(&n, &ac, &ab);
    Vector_Normalise(normal);
}

// ---- 0x564A20: a triangle's plane: its normal, the distance along it, and (when
// asked) the axis it faces most (0/1: +x/-x, 2/3: y, 4/5: z)
void __cdecl TrianglePlane(const CVector* a, const CVector* b, const CVector* c, CVector* normal, float* distance,
                           uint8_t* axis) {
    CVector ab, ac, n;
    VectorSub(&ab, b, a);
    VectorSub(&ac, c, a);
    *normal = *CrossProduct(&n, &ac, &ab);
    Vector_Normalise(normal);
    *distance = float(double(a->y) * normal->y + double(a->x) * normal->x + double(a->z) * normal->z);
    if (!axis) {
        return;
    }
    float ax = std::fabs(normal->x);
    float ay = std::fabs(normal->y);
    if (ay < ax && std::fabs(normal->z) < ax) {
        *axis = 0.0f < normal->x ? 0 : 1;
    } else if (std::fabs(normal->z) < ay) {
        *axis = 0.0f < normal->y ? 2 : 3;
    } else {
        *axis = 0.0f < normal->z ? 4 : 5;
    }
}

// ---- 0x564B50: a sphere (centre, radius) touches a box
uint8_t __cdecl SphereTouchesBox(const float* sphere, const float* min, const float* max) {
    if (min[0] > double(sphere[0]) + sphere[3]) {
        return 0;
    }
    if (max[0] < double(sphere[0]) - sphere[3]) {
        return 0;
    }
    if (min[1] > double(sphere[3]) + sphere[1]) {
        return 0;
    }
    if (max[1] < double(sphere[1]) - sphere[3]) {
        return 0;
    }
    if (min[2] > double(sphere[2]) + sphere[3]) {
        return 0;
    }
    if (max[2] < double(sphere[2]) - sphere[3]) {
        return 0;
    }
    return 1;
}

// ---- 0x564C90 (thiscall): a 0x6C-byte record's assignment (its vector at +0x30 copied
// unless it's the same one)
uint8_t* __fastcall Record6C_Assign(uint8_t* self, re::Edx, const uint8_t* other) {
    uint32_t* d = reinterpret_cast<uint32_t*>(self);
    const uint32_t* s = reinterpret_cast<const uint32_t*>(other);
    for (int i = 0; i < 12; i++) {
        d[i] = s[i];
    }
    if (other + 0x30 != self + 0x30) {
        float* dv = reinterpret_cast<float*>(self + 0x30);
        const float* sv = reinterpret_cast<const float*>(other + 0x30);
        dv[0] = sv[0];
        dv[1] = sv[1];
        dv[2] = sv[2];
    }
    for (int i = 0x10; i < 0x18; i++) {
        d[i] = s[i];
    }
    *reinterpret_cast<uint16_t*>(self + 0x60) = *reinterpret_cast<const uint16_t*>(other + 0x60);
    *reinterpret_cast<uint16_t*>(self + 0x62) = *reinterpret_cast<const uint16_t*>(other + 0x62);
    d[0x19] = s[0x19];
    self[0x68] = other[0x68];
    return self;
}

// ---- 0x564D50: the squared distance from a point to the segment a..b
float __cdecl DistToLineSqr(const CVector* a, const CVector* b, const CVector* p) {
    CVector d, w;
    VectorSub(&d, b, a);
    VectorSub(&w, p, a);
    float dot = float(double(w.z) * d.z + double(d.y) * w.y + double(w.x) * d.x);
    if (dot <= 0.0f) {
        return float(double(w.z) * w.z + double(w.x) * w.x + double(w.y) * w.y);
    }
    float lengthSq = float(double(d.x) * d.x + double(d.y) * d.y + double(d.z) * d.z);
    if (lengthSq <= dot) {
        CVector e;
        VectorSub(&e, p, b);
        return float(double(e.x) * e.x + double(e.y) * e.y + double(e.z) * e.z);
    }
    float r = float(double(w.z) * w.z + double(w.x) * w.x + double(w.y) * w.y - double(dot) * dot / lengthSq);
    return r <= 0.0f ? 0.0f : r;
}

// the line crosses the plane coordinate `k` (axis i) between its ends: the other two
// coordinates there inside the box
bool CrossesFace(const ColLine* line, const float* min, const float* max, int i, float k, bool fromMin) {
    const float* s = line->start;
    const float* e = line->end;
    float d0 = fromMin ? k - s[i] : s[i] - k;
    float d1 = fromMin ? k - e[i] : e[i] - k;
    if (!(d0 * d1 < 0.0f)) {
        return false;
    }
    float t = d0 / (d0 - d1);
    int j = i == 0 ? 1 : 0;
    int l = i == 2 ? 1 : 2;
    float u = s[j] + t * (e[j] - s[j]);
    float v = (e[l] - s[l]) * t + s[l];
    return min[j] < u && u < max[j] && min[l] < v && v < max[l];
}

// ---- 0x564E90: a line touches a box (an end inside, or it crosses a face)
uint8_t __cdecl TestLineBox(const ColLine* line, const float* min, const float* max) {
    const float* s = line->start;
    const float* e = line->end;
    if (min[0] < s[0] && min[1] < s[1] && min[2] < s[2] && s[0] < max[0] && s[1] < max[1] && s[2] < max[2]) {
        return 1;
    }
    if (min[0] < e[0] && min[1] < e[1] && min[2] < e[2] && e[0] < max[0] && e[1] < max[1] && e[2] < max[2]) {
        return 1;
    }
    for (int i = 0; i < 3; i++) {
        if (CrossesFace(line, min, max, i, min[i], true)) {
            return 1;
        }
        if (CrossesFace(line, min, max, i, max[i], false)) {
            return 1;
        }
    }
    return 0;
}

// ---- 0x565D50: a compressed vertex (1/128ths)
void __cdecl DecompressVector(CVector* out, const CompressedVector* v) {
    out->x = float(double(v->x) * 0.0078125);
    out->y = float(double(v->y) * 0.0078125);
    out->z = float(double(v->z) * 0.0078125);
}

// ---- 0x565DA0: a vector divided by a number
CVector* __cdecl VectorDivide(CVector* out, const CVector* v, float s) {
    float inverse = float(1.0 / s);
    out->x = inverse * v->x;
    out->y = v->y * inverse;
    out->z = inverse * v->z;
    return out;
}

// ---- 0x568FC0: a line (start, end) touches a sphere between its ends
uint8_t __cdecl TestLineSphere(const ColLine* line, const CColSphere* sphere) {
    const CVector* start = reinterpret_cast<const CVector*>(line->start);
    const CVector* end = reinterpret_cast<const CVector*>(line->end);
    CVector d;
    VectorSub(&d, end, start);
    float lengthSq = float(double(d.y) * d.y + double(d.x) * d.x + double(d.z) * d.z);
    float length = float(std::sqrt(double(float(std::fabs(lengthSq)))));
    CVector w;
    VectorSub(&w, &sphere->center, start);
    float b = float(double(float(double(w.z) * d.z + double(w.x) * d.x + double(w.y) * d.y)) * -2.0);
    float a = float(double(length) * length);
    float cc = float(double(start->x) * start->x + double(start->y) * start->y + double(start->z) * start->z);
    float cs = float(double(sphere->center.x) * sphere->center.x + double(sphere->center.y) * sphere->center.y +
                     double(sphere->center.z) * sphere->center.z);
    float dot = float(double(start->x) * sphere->center.x + double(sphere->center.y) * start->y +
                      double(start->z) * sphere->center.z);
    float c = float(double(cs) + cc - (double(dot) + dot));
    c = float(double(c) - Sq(sphere->radius));
    float disc = float(double(b) * b - double(a) * 4.0 * c);
    if (disc < 0.0f) {
        return 0;
    }
    float t;
    if (length == 0.0f) {
        t = 0.0f;
    } else {
        float root = float(std::sqrt(double(float(std::fabs(disc)))));
        t = float((double(-b) - root) / (double(a) + a));
    }
    if (t < 0.0f) {
        return 0;
    }
    if (1.0f < t) {
        return 0;
    }
    return 1;
}

// ---- 0x5691A0: the distance from a point to the segment a..b
float __cdecl DistToLine(const CVector* a, const CVector* b, const CVector* p) {
    CVector d, w;
    VectorSub(&d, b, a);
    VectorSub(&w, p, a);
    float dot = float(double(w.z) * d.z + double(d.y) * w.y + double(w.x) * d.x);
    float r;
    if (dot <= 0.0f) {
        r = float(double(w.z) * w.z + double(w.x) * w.x + double(w.y) * w.y);
    } else {
        float lengthSq = float(double(d.x) * d.x + double(d.y) * d.y + double(d.z) * d.z);
        if (lengthSq <= dot) {
            CVector e;
            VectorSub(&e, p, b);
            r = float(double(e.x) * e.x + double(e.y) * e.y + double(e.z) * e.z);
        } else {
            double q = double(w.z) * w.z + double(w.x) * w.x + double(w.y) * w.y - double(dot) * dot / lengthSq;
            if (q <= 0.0) {
                return 0.0f;
            }
            r = float(q);
        }
    }
    return float(std::sqrt(double(float(std::fabs(r)))));
}

// ---- 0x569330: the same in 2D (z 0)
float __cdecl DistToLine2D(const float* a, const float* b, const float* p) {
    CVector a3 = {a[0], a[1], 0.0f};
    CVector b3 = {b[0], b[1], 0.0f};
    CVector p3 = {p[0], p[1], 0.0f};
    return DistToLine_(&a3, &b3, &p3);
}

} // namespace

#define COLPRIM_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(colprim_original_, __LINE__);                                        \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(colprim_original_, __LINE__), \
             re::Mode::Original)

COLPRIM_REVERSED("CCollision::DistSqToRect2D", 0x00564310, &DistSqToRect2D);
COLPRIM_REVERSED("UsePoint::GetArea", 0x00564700, &UsePoint_GetArea);
COLPRIM_REVERSED("CColLine::CColLine", 0x00564830, &ColLine_Construct);
COLPRIM_REVERSED("CColBox::Set_5648A0", 0x005648A0, &ColBox_Set);
COLPRIM_REVERSED("CColLine::Set_564920", 0x00564920, &ColLine_Set);
COLPRIM_REVERSED("CColLine::CColLine_564950", 0x00564950, &ColLine_Construct2);
COLPRIM_REVERSED("CCollision::TriangleNormal", 0x005649C0, &TriangleNormal);
COLPRIM_REVERSED("CCollision::TrianglePlane", 0x00564A20, &TrianglePlane);
COLPRIM_REVERSED("CCollision::SphereTouchesBox", 0x00564B50, &SphereTouchesBox);
COLPRIM_REVERSED("CCollision::Record6C_Assign", 0x00564C90, &Record6C_Assign);
COLPRIM_REVERSED("CCollision::DistToLineSqr", 0x00564D50, &DistToLineSqr);
COLPRIM_REVERSED("CCollision::TestLineBox", 0x00564E90, &TestLineBox);
COLPRIM_REVERSED("CCollision::DecompressVector", 0x00565D50, &DecompressVector);
COLPRIM_REVERSED("CCollision::VectorDivide", 0x00565DA0, &VectorDivide);
COLPRIM_REVERSED("CCollision::TestLineSphere", 0x00568FC0, &TestLineSphere);
COLPRIM_REVERSED("CCollision::DistToLine", 0x005691A0, &DistToLine);
COLPRIM_REVERSED("CCollision::DistToLine2D", 0x00569330, &DistToLine2D);
