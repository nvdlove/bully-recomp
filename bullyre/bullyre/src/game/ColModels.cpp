// Lines and swept spheres against collision models (0x569390..0x56B4D0): the line
// against a sphere or box, a model's lines (wheels) against another model, the swept
// sphere against a model (the camera's sweep), and the lines of sight against a model
// (GTA San Andreas' CCollision::ProcessLineOfSight/TestLineOfSight/ProcessLineSphere/
// ProcessLineBox where they match). Surfaces in the "see-through" set are skipped when
// the flags ask (bit 6: 15, 16, 25, 52, 20, 'A'; bit 8 the "shoot-through" set: 16, 25,
// 52, 20, 'A'; a byte flag: 'A' alone).

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
    int32_t index;      // +0x1C (the sweep: which sphere)
    uint8_t surfaceA;   // +0x20
    uint8_t pieceA;     // +0x21
    uint8_t surfaceB;   // +0x22
    uint8_t pieceB;     // +0x23
    float depth;        // +0x24
};
static_assert(sizeof(ColPoint) == 0x28);

// the triangle a line of sight hit (0x6C bytes; 0x564C90 copies one)
struct StoredPoly {
    float verts[3][4];  // +0x00 (world space once filled)
    CVector normal;     // +0x30
    float distance;     // +0x3C
    uint8_t unknown40[0x20];
    uint8_t surfaceA;   // +0x60
    uint8_t pieceA;     // +0x61
    uint8_t surfaceB;   // +0x62
    uint8_t pieceB;     // +0x63
    int32_t triangle;   // +0x64 (-1: none)
    uint8_t valid;      // +0x68
    uint8_t pad69[3];
};
static_assert(sizeof(StoredPoly) == 0x6C);

struct Sphere {
    CVector center;
    float radius;
};

GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, CrossProduct, 0x00414730, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(void, __cdecl, Matrix_Invert, 0x00413770, const col::CMatrix* in, col::CMatrix* out);
GAME_FN(void, __thiscall, Matrix_Multiply, 0x00412800, col::CMatrix* m, const col::CMatrix* by);
GAME_FN(void, __cdecl, TransformPoints, 0x00414DB0, void* out, int count, const col::CMatrix* m, const void* in,
        int stride);
// (0x0041A9A0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* MultiplyPoint(CVector* out, const col::CMatrix* m, const CVector* v) { return re::CallVec16<0x0041A9A0, CVector*>(out, m, v); }
// (0x0045D500 stores 16 bytes through `out4`: called through re::CallVec16)
static inline void MultiplyPoint4(float* out4, const col::CMatrix* m, const CVector* v) { re::CallVec16<0x0045D500, void>(out4, m, v); }
GAME_FN(float*, __cdecl, MatrixTimesPoint, 0x0045D4A0, float* out4, const col::CMatrix* m, const float* v);
// (0x004146B0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* Multiply3x3(CVector* out, const col::CMatrix* m, const CVector* v) { return re::CallVec16<0x004146B0, CVector*>(out, m, v); }
// (0x00414C80 stores 16 bytes through `out`: called through re::CallVec16)
static inline void TransformVector(CVector* out, const col::CMatrix* m, const CVector* v) { re::CallVec16<0x00414C80, void>(out, m, v); }
GAME_FN(void, __thiscall, ColModel_GetVertex, 0x0056E680, CColModel* col, CVector* out, int index);
GAME_FN(uint16_t, __thiscall, Tree_FirstInSphere, 0x005617F0, void* tree, const CVector* center, float radius,
        int cylinder);
GAME_FN(uint16_t, __thiscall, Tree_FirstInBox, 0x00561850, void* tree, const void* box);
GAME_FN(uint16_t, __cdecl, Tree_Next, 0x005618A0);
GAME_FN(uint8_t, __cdecl, TestLineBox, 0x00564E90, const ColLine* line, const CVector* min, const CVector* max);
GAME_FN(uint8_t*, __thiscall, Record6C_Assign, 0x00564C90, StoredPoly* self, const StoredPoly* other);
GAME_FN(uint8_t, __cdecl, TestLineSphere, 0x00568FC0, const ColLine* line, const CColSphere* sphere);
GAME_FN(uint8_t, __cdecl, TestLineTriangle, 0x005672F0, const ColLine* line, const CompressedVector* verts,
        const CColTriangle* tri);
GAME_FN(uint8_t, __cdecl, ProcessLineTriangle, 0x00567790, const ColLine* line, const CompressedVector* verts,
        const CColTriangle* tri, ColPoint* out, float* minT);
GAME_FN(uint8_t, __cdecl, ProcessLineSphere_, 0x00569390, const ColLine* line, const CColSphere* sphere,
        ColPoint* out, float* minT);
GAME_FN(uint8_t, __cdecl, ProcessLineBox_, 0x005695B0, const ColLine* line, const ColBox* box, ColPoint* out,
        float* minT);
GAME_FN(uint8_t, __cdecl, SweepSphereTriangle, 0x00575660, const re::Ptr32<const CVector>* triangle, const CVector* normal,
        const Sphere* sphere, const CVector* dir, float* t, CVector* hitNormal, CVector* hitPoint, uint8_t* face);
GAME_FN(uint8_t, __cdecl, SweepSphereBox, 0x005767B0, const CVector* boxMin, const CVector* boxMax,
        const Sphere* sphere, const CVector* dir, float* t, CVector* hitNormal, CVector* hitPoint, uint8_t* face,
        int* touched);
GAME_FN(uint8_t, __cdecl, SweepSphereSphere, 0x00576B50, const Sphere* a, const Sphere* b, float maxT,
        const CVector* velA, const CVector* velB, float* t, CVector* point);
inline const CVector* const kZeroVelocity = reinterpret_cast<const CVector*>(0x00BCC564);

// the model has what bit 9 of the flags asks for (+0x18 set or not)
bool KindMatches(const CCollisionData* data, uint32_t flags) {
    uint8_t has = (data && data->unknown18) ? 1 : 0;
    return uint8_t((flags >> 9) & 1) == has;
}

// a surface the flags skip
bool Skipped(uint8_t s, uint32_t flags, int skipA) {
    if ((flags & 0x40) && (s == 0x0F || s == 0x10 || s == 0x19 || s == 0x34 || s == 0x14 || s == 'A')) {
        return true;
    }
    if ((flags & 0x100) && (s == 0x10 || s == 0x19 || s == 0x34 || s == 0x14 || s == 'A')) {
        return true;
    }
    return skipA && s == 'A';
}

// ---- 0x569390: a line against a sphere (the entry point, nearer along the line than
// the best so far)
uint8_t __cdecl ProcessLineSphere(const ColLine* line, const CColSphere* sphere, ColPoint* out, float* minT) {
    CVector d;
    VectorSub(&d, &line->end, &line->start);
    float a = float(double(d.z) * d.z + double(d.x) * d.x + double(d.y) * d.y);
    CVector w;
    VectorSub(&w, &sphere->center, &line->start);
    float b = -float(double(w.z) * d.z + double(w.x) * d.x + double(w.y) * d.y);
    double c = double(w.x) * w.x + double(w.y) * w.y + double(w.z) * w.z - double(sphere->radius) * sphere->radius;
    double disc = double(b) * b - c * a;
    if (!(0.0 <= disc)) {
        return 0;
    }
    float t = float((double(-b) - double(float(std::sqrt(disc)))) / a);
    if (!(0.0f <= t) || !(t <= 1.0f)) {
        return 0;
    }
    if (*minT <= t) {
        return 0;
    }
    CVector e;
    VectorSub(&e, &line->end, &line->start);
    CVector along = {t * e.x, t * e.y, e.z * t};
    CVector p, n;
    VectorAdd(&p, &line->start, &along);
    VectorSub(&n, &p, &sphere->center);
    Vector_Normalise(&n);
    out->point = p;
    out->normal = n;
    out->surfaceB = sphere->surface;
    out->pieceB = sphere->piece;
    out->surfaceA = 0;
    out->pieceA = 0;
    *minT = t;
    return 1;
}

// ---- 0x5695B0: a line against a box: the nearest face crossing (t < 1 and nearer
// than the best so far)
uint8_t __cdecl ProcessLineBox(const ColLine* line, const ColBox* box, ColPoint* out, float* minT) {
    const CVector& s = line->start;
    const CVector& e = line->end;
    const CVector& mn = box->min;
    const CVector& mx = box->max;
    float best = 1.0f;
    float px, py, pz, nx, ny, nz;
    float k = mn.x - s.x;
    float t, u, v;
    if (k * (mn.x - e.x) < 0.0f && ((t = k / (k - (mn.x - e.x))), (u = s.y + t * (e.y - s.y)),
                                     (v = (e.z - s.z) * t + s.z), mn.y < u && u < mx.y && mn.z < v && v < mx.z &&
                                     t < 1.0f)) {
        nx = -1.0f;
        px = mn.x;
        py = u;
        pz = v;
        best = t;
    } else {
        pz = 0.0f;
        py = 0.0f;
        px = 0.0f;
        nx = 0.0f;
    }
    k = s.x - mx.x;
    if (k * (e.x - mx.x) < 0.0f) {
        t = k / (k - (e.x - mx.x));
        u = s.y + t * (e.y - s.y);
        v = (e.z - s.z) * t + s.z;
        if (mn.y < u && u < mx.y && mn.z < v && v < mx.z && t < best) {
            nx = 1.0f;
            px = mx.x;
            py = u;
            pz = v;
            best = t;
        }
    }
    ny = 0.0f;
    k = mn.y - s.y;
    if (k * (mn.y - e.y) < 0.0f) {
        t = k / (k - (mn.y - e.y));
        u = s.x + t * (e.x - s.x);
        v = (e.z - s.z) * t + s.z;
        if (mn.x < u && u < mx.x && mn.z < v && v < mx.z && t < best) {
            nx = 0.0f;
            ny = -1.0f;
            px = u;
            py = mn.y;
            pz = v;
            best = t;
        }
    }
    k = s.y - mx.y;
    if (k * (e.y - mx.y) < 0.0f) {
        t = k / (k - (e.y - mx.y));
        u = s.x + t * (e.x - s.x);
        v = (e.z - s.z) * t + s.z;
        if (mn.x < u && u < mx.x && mn.z < v && v < mx.z && t < best) {
            nx = 0.0f;
            ny = 1.0f;
            px = u;
            py = mx.y;
            pz = v;
            best = t;
        }
    }
    nz = 0.0f;
    k = mn.z - s.z;
    if (k * (mn.z - e.z) < 0.0f) {
        t = k / (k - (mn.z - e.z));
        u = s.x + t * (e.x - s.x);
        v = (e.y - s.y) * t + s.y;
        if (mn.x < u && u < mx.x && mn.y < v && v < mx.y && t < best) {
            nx = 0.0f;
            ny = 0.0f;
            nz = -1.0f;
            px = u;
            py = v;
            pz = mn.z;
            best = t;
        }
    }
    k = s.z - mx.z;
    if (k * (e.z - mx.z) < 0.0f) {
        t = k / (k - (e.z - mx.z));
        u = s.x + t * (e.x - s.x);
        v = (e.y - s.y) * t + s.y;
        if (mn.x < u && u < mx.x && mn.y < v && v < mx.y && t < best) {
            nx = 0.0f;
            ny = 0.0f;
            nz = 1.0f;
            px = u;
            py = v;
            pz = mx.z;
            best = t;
        }
    }
    if (*minT <= best) {
        return 0;
    }
    out->point = {px, py, pz};
    out->normal = {nx, ny, nz};
    out->surfaceA = 0;
    out->pieceA = 0;
    out->surfaceB = box->surface;
    out->pieceB = box->piece;
    *minT = best;
    out->depth = best;
    return 1;
}

// ---- 0x569DC0: a model's lines (its +4 array, up to 8: a vehicle's wheels) under one
// matrix against another model under another: per line, the nearest hit (points and
// minTs are per line); whether any hit
uint8_t __cdecl ProcessLineModels(const col::CMatrix* matrixA, CColModel* modelA, const col::CMatrix* matrixB,
                                  CColModel* modelB, ColPoint* points, float* minTs) {
    CCollisionData* dataB = modelB->data;
    CCollisionData* dataA = modelA->data;
    if (!dataA || !dataB) {
        return 0;
    }
    col::CMatrix m;
    m.attached = nullptr;
    Matrix_Invert(matrixB, &m);
    Matrix_Multiply(&m, matrixA);
    ColLine lines[8];
    int numLines = int8_t(dataA->unknown20);
    if (numLines) {
        TransformPoints(lines, numLines * 2, &m, dataA->unknown04, 0x10);
    }
    uint8_t anyHit = 0;
    char hit[8];
    const ColBox* boxes = re::PtrCast<const ColBox*>(dataB->boxes);
    for (int i = 0; i < numLines; i++) {
        ColLine* line = &lines[i];
        float* minT = &minTs[i];
        hit[i] = 0;
        for (int j = 0; j < int8_t(dataB->numSpheres); j++) {
            if (ProcessLineSphere_(line, &dataB->spheres[j], points, minT)) {
                hit[i] = 1;
            }
        }
        for (int j = 0; j < int8_t(dataB->numBoxes); j++) {
            if (ProcessLineBox_(line, &boxes[j], points, minT)) {
                hit[i] = 1;
            }
        }
        const CompressedVector* verts = dataB->vertices;
        const CColTriangle* tris = dataB->triangles;
        if (dataB->numTriangles > 0) {
            if (!dataB->planes) {
                for (int j = 0; j < dataB->numTriangles; j++) {
                    if (ProcessLineTriangle(line, verts, &tris[j], points, minT)) {
                        hit[i] = 1;
                    }
                }
            } else {
                uint16_t index = Tree_FirstInBox(dataB->planes, line);
                while (index != 0xFFFF) {
                    if (ProcessLineTriangle(line, verts, &tris[index], points, minT)) {
                        hit[i] = 1;
                    }
                    index = Tree_Next();
                }
            }
        }
        if (hit[i]) {
            anyHit = 1;
            CVector p;
            MultiplyPoint(&p, matrixB, &points->point);
            points->point = p;
            TransformVector(&p, matrixB, &points->normal);
            points->normal = p;
        }
        points++;
    }
    return anyHit;
}

// ---- 0x56A060: a sphere swept from `from` to `to` against a model under a matrix
// (parts: 1 its spheres, 2 boxes, 8 triangles; triangleNormal: the triangle's own normal
// rather than the contact's): the nearest hit's point, normal and (in depth and
// *distance) the fraction along; whether it hit
uint8_t __cdecl ProcessSweptSphere(const CVector* from, const CVector* to, float radius, const col::CMatrix* matrix,
                                   CColModel* model, ColPoint* out, float* distance, uint32_t flags, int parts,
                                   int triangleNormal) {
    CCollisionData* data = model->data;
    if (!data || !KindMatches(data, flags)) {
        return 0;
    }
    col::CMatrix inverse;
    inverse.attached = nullptr;
    Matrix_Invert(matrix, &inverse);
    CVector d;
    VectorSub(&d, to, from);
    CVector hitPoint = {0.0f, 0.0f, 0.0f};
    Sphere sweep = {*from, radius};
    uint8_t found = 0;
    uint8_t p = uint8_t(parts);
    if (p & 1) {
        for (int i = 0; i < int8_t(data->numSpheres); i++) {
            const CColSphere& s = data->spheres[i];
            CVector c = s.center;
            float w[4];
            MultiplyPoint4(w, matrix, &c);
            Sphere world = {{w[0], w[1], w[2]}, s.radius};
            float t = out->depth;
            if (SweepSphereSphere(&sweep, &world, 1.0f, &d, kZeroVelocity, &t, &hitPoint) && t < out->depth) {
                out->depth = t;
                out->point = hitPoint;
                out->surfaceB = s.surface;
                out->pieceB = s.piece;
                out->index = i;
                *distance = t;
                CVector along = {d.x * t, d.y * t, t * d.z};
                CVector q;
                VectorAdd(&q, &along, &sweep.center);
                CVector n = {q.x - world.center.x, q.y - world.center.y, q.z - world.center.z};
                found = 1;
                Vector_Normalise(&n);
                out->normal = n;
            }
        }
    }
    // the sweep in the model's space
    float lf[4];
    MultiplyPoint4(lf, &inverse, &sweep.center);
    CVector localFrom = {lf[0], lf[1], lf[2]};
    Sphere local = {localFrom, radius};
    CVector localDir;
    Multiply3x3(&localDir, &inverse, &d);
    float length = float(std::sqrt(double(float(std::fabs(
        float(double(localDir.x) * localDir.x + double(localDir.y) * localDir.y + double(localDir.z) * localDir.z))))));
    uint8_t face = 0;
    int touched = 0;
    CVector hitNormal = {0.0f, 0.0f, 0.0f};
    auto report = [&](float t, uint8_t surface, uint8_t piece) {
        float fraction = float(double(t) / length);
        CVector n;
        Multiply3x3(&n, matrix, &hitNormal);
        hitNormal = n;
        float w4[4];
        MultiplyPoint4(w4, matrix, &hitPoint);
        out->depth = fraction;
        out->normal = n;
        out->point = {w4[0], w4[1], w4[2]};
        hitPoint = out->point;
        out->surfaceB = surface;
        out->pieceB = piece;
        *distance = fraction;
        found = 1;
    };
    if (p & 2) {
        const ColBox* boxes = re::PtrCast<const ColBox*>(data->boxes);
        for (int i = 0; i < int8_t(data->numBoxes); i++) {
            float t = out->depth;
            if (SweepSphereBox(&boxes[i].min, &boxes[i].max, &local, &localDir, &t, &hitNormal, &hitPoint, &face,
                               &touched) &&
                t < length && float(double(t) / length) < out->depth) {
                report(t, boxes[i].surface, boxes[i].piece);
            }
        }
    }
    if (data->numTriangles > 0 && (p & 8)) {
        CVector verts[3] = {};
        re::Ptr32<const CVector> corners[3] = {&verts[0], &verts[1], &verts[2]};
        auto testTriangle = [&](int index) {
            const CColTriangle& tri = data->triangles[index];
            float t = out->depth;
            ColModel_GetVertex(model, &verts[0], tri.a);
            ColModel_GetVertex(model, &verts[1], tri.b);
            ColModel_GetVertex(model, &verts[2], tri.c);
            CVector ab, ac, n;
            VectorSub(&ab, &verts[1], &verts[0]);
            VectorSub(&ac, &verts[2], &verts[0]);
            CrossProduct(&n, &ac, &ab);
            Vector_Normalise(&n);
            if (SweepSphereTriangle(corners, &n, &local, &localDir, &t, &hitNormal, &hitPoint, &face) && t < length &&
                float(double(t) / length) < out->depth) {
                float fraction = float(double(t) / length);
                CVector rotated;
                if (!triangleNormal) {
                    Multiply3x3(&rotated, matrix, &n);
                } else {
                    Multiply3x3(&rotated, matrix, &hitNormal);
                    hitNormal = rotated;
                }
                out->normal = rotated;
                float w4[4];
                MultiplyPoint4(w4, matrix, &hitPoint);
                out->depth = fraction;
                out->point = {w4[0], w4[1], w4[2]};
                hitPoint = out->point;
                out->surfaceB = tri.surface;
                out->pieceB = 0;
                *distance = fraction;
                found = 1;
            }
        };
        if (!data->planes) {
            for (int i = 0; i < data->numTriangles; i++) {
                testTriangle(i);
            }
        } else {
            uint16_t index = Tree_FirstInSphere(data->planes, &localFrom, length + radius, 0);
            while (index != 0xFFFF) {
                testTriangle(index);
                index = Tree_Next();
            }
        }
    }
    return found;
}

// ---- 0x56ACD0: a line of sight blocked by a model under a matrix
uint8_t __cdecl TestLineOfSight(const ColLine* line, const col::CMatrix* matrix, CColModel* model, uint32_t flags,
                                int skipA) {
    CCollisionData* data = model->data;
    if (!data || !KindMatches(data, flags)) {
        return 0;
    }
    col::CMatrix inverse;
    inverse.attached = nullptr;
    Matrix_Invert(matrix, &inverse);
    ColLine local;
    TransformPoints(&local, 2, &inverse, line, 0x10);
    if (!TestLineBox(&local, &model->boxMin, &model->boxMax)) {
        return 0;
    }
    if (((flags >> 9) & 1) == 0) {
        for (int i = 0; i < int8_t(data->numSpheres); i++) {
            if (!Skipped(data->spheres[i].surface, flags, skipA) && TestLineSphere(&local, &data->spheres[i])) {
                return 1;
            }
        }
        const ColBox* boxes = re::PtrCast<const ColBox*>(data->boxes);
        for (int i = 0; i < int8_t(data->numBoxes); i++) {
            if (!Skipped(boxes[i].surface, flags, skipA) && TestLineBox(&local, &boxes[i].min, &boxes[i].max)) {
                return 1;
            }
        }
    }
    if (data->numTriangles != 0) {
        const CompressedVector* verts = data->vertices;
        const CColTriangle* tris = data->triangles;
        if (!data->planes) {
            for (int i = 0; i < data->numTriangles; i++) {
                if (!Skipped(tris[i].surface, flags, skipA) && TestLineTriangle(&local, verts, &tris[i])) {
                    return 1;
                }
            }
        } else {
            uint16_t index = Tree_FirstInBox(data->planes, &local);
            while (index != 0xFFFF) {
                if (!Skipped(tris[index].surface, flags, skipA) && TestLineTriangle(&local, verts, &tris[index])) {
                    return 1;
                }
                index = Tree_Next();
            }
        }
    }
    return 0;
}

// the line into the model's space, cut short at the best so far (*minT < 1)
void LocalLine(ColLine* local, const ColLine* line, const col::CMatrix* inverse, float minT) {
    TransformPoints(local, 2, inverse, line, 0x10);
    if (minT < 1.0f) {
        CVector d = {minT * (local->end.x - local->start.x), (local->end.y - local->start.y) * minT,
                     minT * (local->end.z - local->start.z)};
        CVector e = {d.x + local->start.x, local->start.y + d.y, local->start.z + d.z};
        local->endW = local->startW;
        local->end = e;
    }
}

// the hit back in world space
void HitToWorld(ColPoint* out, const col::CMatrix* matrix) {
    CVector p;
    MultiplyPoint(&p, matrix, &out->point);
    out->point = p;
    CVector n = out->normal;
    CVector r;
    Multiply3x3(&r, matrix, &n);
    out->normal = r;
}

// ---- 0x56B030: a line of sight against a model: the nearest hit (the fraction along
// *minT scaled by it)
uint8_t __cdecl ProcessLineOfSight(const ColLine* line, const col::CMatrix* matrix, CColModel* model, ColPoint* out,
                                   float* minT, uint32_t flags, int skipA) {
    CCollisionData* data = model->data;
    if (!data || !KindMatches(data, flags)) {
        return 0;
    }
    col::CMatrix inverse;
    inverse.attached = nullptr;
    Matrix_Invert(matrix, &inverse);
    ColLine local;
    LocalLine(&local, line, &inverse, *minT);
    if (!TestLineBox(&local, &model->boxMin, &model->boxMax)) {
        return 0;
    }
    float t = 1.0f;
    if (((flags >> 9) & 1) == 0) {
        for (int i = 0; i < int8_t(data->numSpheres); i++) {
            if (!Skipped(data->spheres[i].surface, flags, skipA)) {
                ProcessLineSphere_(&local, &data->spheres[i], out, &t);
            }
        }
        const ColBox* boxes = re::PtrCast<const ColBox*>(data->boxes);
        for (int i = 0; i < int8_t(data->numBoxes); i++) {
            if (!Skipped(boxes[i].surface, flags, skipA)) {
                ProcessLineBox_(&local, &boxes[i], out, &t);
            }
        }
    }
    if (data->numTriangles != 0) {
        const CompressedVector* verts = data->vertices;
        const CColTriangle* tris = data->triangles;
        if (!data->planes) {
            for (int i = 0; i < data->numTriangles; i++) {
                if (!Skipped(tris[i].surface, flags, skipA)) {
                    ProcessLineTriangle(&local, verts, &tris[i], out, &t);
                }
            }
        } else {
            uint16_t index = Tree_FirstInBox(data->planes, &local);
            while (index != 0xFFFF) {
                if (!Skipped(tris[index].surface, flags, skipA)) {
                    ProcessLineTriangle(&local, verts, &tris[index], out, &t);
                }
                index = Tree_Next();
            }
        }
    }
    if (t < 1.0f) {
        HitToWorld(out, matrix);
        *minT = *minT * t;
        return 1;
    }
    return 0;
}

// ---- 0x56B4D0: the same by parts (1 spheres, 2 boxes, 8 triangles, no surfaces
// skipped), keeping the triangle hit (world space) in `poly` when given. (The record's
// other fields are whatever was on the game's stack: 0 here.)
uint8_t __cdecl ProcessLineOfSightPoly(const ColLine* line, const col::CMatrix* matrix, CColModel* model,
                                       ColPoint* out, float* minT, uint32_t flags, int parts, StoredPoly* poly) {
    CCollisionData* data = model->data;
    if (!data || !KindMatches(data, flags)) {
        return 0;
    }
    StoredPoly hitPoly = {};
    hitPoly.triangle = -1;
    hitPoly.valid = 0;
    col::CMatrix inverse;
    inverse.attached = nullptr;
    Matrix_Invert(matrix, &inverse);
    ColLine local;
    LocalLine(&local, line, &inverse, *minT);
    if (!TestLineBox(&local, &model->boxMin, &model->boxMax)) {
        return 0;
    }
    float t = 1.0f;
    uint8_t p = uint8_t(parts);
    if (p & 1) {
        for (int i = 0; i < int8_t(data->numSpheres); i++) {
            ProcessLineSphere_(&local, &data->spheres[i], out, &t);
        }
    }
    if (p & 2) {
        const ColBox* boxes = re::PtrCast<const ColBox*>(data->boxes);
        for (int i = 0; i < int8_t(data->numBoxes); i++) {
            ProcessLineBox_(&local, &boxes[i], out, &t);
        }
    }
    if (data->numTriangles > 0 && (p & 8)) {
        const CompressedVector* verts = data->vertices;
        const CColTriangle* tris = data->triangles;
        hitPoly.triangle = -1;
        hitPoly.valid = 0;
        auto keep = [&](int index) {
            const CColTriangle& tri = tris[index];
            const CompressedVector* v[3] = {&verts[tri.a], &verts[tri.b], &verts[tri.c]};
            hitPoly.valid = 1;
            for (int k = 0; k < 3; k++) {
                hitPoly.verts[k][0] = float(double(v[k]->x) * 0.0078125);
                hitPoly.verts[k][1] = float(double(v[k]->y) * 0.0078125);
                hitPoly.verts[k][2] = float(double(v[k]->z) * 0.0078125);
            }
            hitPoly.triangle = index;
        };
        if (!data->planes) {
            for (int i = 0; i < data->numTriangles; i++) {
                if (ProcessLineTriangle(&local, verts, &tris[i], out, &t)) {
                    keep(i);
                }
            }
        } else {
            uint16_t index = Tree_FirstInBox(data->planes, &local);
            while (index != 0xFFFF) {
                if (ProcessLineTriangle(&local, verts, &tris[index], out, &t)) {
                    keep(index);
                }
                index = Tree_Next();
            }
        }
    }
    if (!(t < 1.0f)) {
        return 0;
    }
    HitToWorld(out, matrix);
    if (poly) {
        Record6C_Assign(poly, &hitPoly);
        if (hitPoly.valid) {
            for (int k = 0; k < 3; k++) {
                float w4[4];
                float* r = MatrixTimesPoint(w4, matrix, poly->verts[k]);
                poly->verts[k][0] = r[0];
                poly->verts[k][1] = r[1];
                poly->verts[k][2] = r[2];
                poly->verts[k][3] = r[3];
            }
            poly->normal = out->normal;
            poly->distance = float(double(poly->normal.z) * poly->verts[0][2] + double(poly->normal.x) * poly->verts[0][0] +
                                   double(poly->normal.y) * poly->verts[0][1]);
            poly->surfaceA = out->surfaceA;
            poly->pieceA = out->pieceA;
            poly->surfaceB = out->surfaceB;
            poly->pieceB = out->pieceB;
        }
    }
    *minT = *minT * t;
    return 1;
}

} // namespace

#define COLMODELS_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(colmodels_original_, __LINE__);                                        \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(colmodels_original_, __LINE__), \
             re::Mode::Original)

COLMODELS_REVERSED("CCollision::ProcessLineSphere", 0x00569390, &ProcessLineSphere);
COLMODELS_REVERSED("CCollision::ProcessLineBox", 0x005695B0, &ProcessLineBox);
COLMODELS_REVERSED("CCollision::ProcessLineModels", 0x00569DC0, &ProcessLineModels);
COLMODELS_REVERSED("CCollision::ProcessSweptSphere", 0x0056A060, &ProcessSweptSphere);
COLMODELS_REVERSED("CCollision::TestLineOfSight", 0x0056ACD0, &TestLineOfSight);
COLMODELS_REVERSED("CCollision::ProcessLineOfSight", 0x0056B030, &ProcessLineOfSight);
COLMODELS_REVERSED("CCollision::ProcessLineOfSightPoly", 0x0056B4D0, &ProcessLineOfSightPoly);
