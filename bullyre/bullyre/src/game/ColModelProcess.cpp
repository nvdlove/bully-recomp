// Collision between col models (0x56BB20..0x56DC80): GTA's ProcessColModels (two models
// in contact: their spheres against the other's spheres, boxes and triangles, and the
// first model's lines), the swept version (both models moving from one matrix to
// another: the first contact along the way), the stored-triangle check for vertical
// lines (IsStoredPolyStillValidVerticalLine), a model's bounds under a height, and the
// triangle neighbour keys (0x5706F0) and the surface names (0x56EB60).

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Collision.h"
#include "reversed.h"

namespace {

using col::CMatrix;
using col::CVector;

// A contact (CColPoint, 0x28)
struct ColPoint {
    CVector point;       // +0
    re::Ptr32<void> entity;        // +0xC (a registered reference)
    CVector normal;      // +0x10
    int32_t index;       // +0x1C which of the first model's spheres
    uint8_t surfaceA;    // +0x20
    uint8_t pieceA;      // +0x21
    uint8_t surfaceB;    // +0x22
    uint8_t pieceB;      // +0x23
    float depth;         // +0x24 (the swept test: the fraction of the move)
};
static_assert(offsetof(ColPoint, normal) == 0x10);
static_assert(offsetof(ColPoint, surfaceA) == 0x20);
static_assert(offsetof(ColPoint, depth) == 0x24);
static_assert(sizeof(ColPoint) == 0x28);

// A box (CColBox, 0x24): min and max as 16-byte vectors, then surface and piece
struct ColBox {
    CVector min;      // +0
    float unknown0C;  // +0xC
    CVector max;      // +0x10
    float unknown1C;  // +0x1C
    uint8_t surface;  // +0x20
    uint8_t piece;    // +0x21
    uint8_t pad[2];
};
static_assert(offsetof(ColBox, max) == 0x10);
static_assert(offsetof(ColBox, surface) == 0x20);
static_assert(sizeof(ColBox) == 0x24);

// A line (CColLine, 0x20): 16-byte vectors
struct ColLine {
    CVector start;    // +0
    float unknown0C;  // +0xC
    CVector end;      // +0x10
    float unknown1C;  // +0x1C
};
static_assert(sizeof(ColLine) == 0x20);

// A sphere as the sweeps take it (center, radius)
struct SweepSphere {
    CVector center;
    float radius;
};

// A triangle remembered from a vertical line test (CStoredCollPoly): the corners as
// 16-byte vectors; +0x68 set while it's valid
struct StoredCollPoly {
    float verts[3][4];        // +0
    uint8_t unknown30[0x38];  // +0x30
    uint8_t valid;            // +0x68
};
static_assert(offsetof(StoredCollPoly, valid) == 0x68);

// What the swept test tells about the hit (0x10)
struct SweepInfo {
    uint8_t face;      // +0 the polygon's face (not an edge or vertex)
    uint8_t multiple;  // +1 more than one thing touched
    uint8_t hit;       // +2
    uint8_t pad;
    CVector normal;    // +4
};
static_assert(sizeof(SweepInfo) == 0x10);

// what the triangle data (+0x14) answers (a tree of the triangles in space)
struct TriangleTree;

GAME_FN(void, __cdecl, CMP_Matrix_Invert, 0x00413770, const CMatrix* in, CMatrix* out);
GAME_FN(void, __thiscall, CMP_Matrix_MultiplyBy, 0x00412800, CMatrix* m, const CMatrix* by);
GAME_FN(CMatrix*, __cdecl, CMP_Matrix_Multiply, 0x004138D0, CMatrix* out, const CMatrix* a, const CMatrix* b);
// (0x0041A9A0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* CMP_MultiplyPoint(CVector* out, const CMatrix* m, const CVector* v) { return re::CallVec16<0x0041A9A0, CVector*>(out, m, v); }
// (0x00414C80 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* CMP_Multiply3x3(CVector* out, const CMatrix* m, const CVector* v) { return re::CallVec16<0x00414C80, CVector*>(out, m, v); }
// (0x004146B0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* CMP_Multiply3x3b(CVector* out, const CMatrix* m, const CVector* v) { return re::CallVec16<0x004146B0, CVector*>(out, m, v); }
// (0x0045D500 stores 16 bytes through `out4`: called through re::CallVec16)
static inline float* CMP_MultiplyPoint4(float* out4, const CMatrix* m, const CVector* v) { return re::CallVec16<0x0045D500, float*>(out4, m, v); }  // (writes 4 floats)
GAME_FN(void, __cdecl, CMP_TransformPointsStrided, 0x00414DB0, void* out, int count, const CMatrix* m, const void* in, int stride);
GAME_FN(CVector*, __cdecl, CMP_VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, CMP_VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, CMP_CrossProduct, 0x00414730, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, CMP_Normalise, 0x00414E10, CVector* v);
GAME_FN(float, __thiscall, CMP_Normalise2, 0x004294A0, CVector* v);
GAME_FN(uint8_t, __cdecl, CMP_SphereTouchesBox, 0x00564B50, const void* sphere, const CVector* boxMin, const CVector* boxMax);
GAME_FN(uint8_t, __cdecl, CMP_ProcessSphereSphere, 0x00565DE0, const void* a, const CColSphere* b, ColPoint* point, float* dist);
GAME_FN(uint8_t, __cdecl, CMP_ProcessSphereBox, 0x00565F30, const void* sphere, const ColBox* box, ColPoint* point, float* dist);
GAME_FN(uint8_t, __cdecl, CMP_TestSphereTriangle, 0x00567D20, const void* sphere, const CompressedVector* verts, const CColTriangle* tri);
GAME_FN(uint8_t, __cdecl, CMP_ProcessSphereTriangle, 0x00568570, const void* sphere, const CompressedVector* verts, const CColTriangle* tri,
        ColPoint* point, float* dist);
GAME_FN(uint8_t, __cdecl, CMP_ProcessLineSphere, 0x00569390, const ColLine* line, const CColSphere* sphere, ColPoint* point, float* dist);
GAME_FN(uint8_t, __cdecl, CMP_ProcessLineBox, 0x005695B0, const ColLine* line, const ColBox* box, ColPoint* point, float* dist);
GAME_FN(uint8_t, __cdecl, CMP_ProcessLineTriangle, 0x00567790, const ColLine* line, const CompressedVector* verts, const CColTriangle* tri,
        ColPoint* point, float* dist);
GAME_FN(uint16_t, __thiscall, CMP_TreeFindFirst, 0x005617F0, TriangleTree* tree, const void* center, float radius, int flag);
GAME_FN(uint16_t, __thiscall, CMP_TreeFindNext, 0x005618A0, TriangleTree* tree);
GAME_FN(void, __thiscall, CMP_GetVertex, 0x0056E680, const CColModel* model, CVector* out, int index);
GAME_FN(uint8_t, __thiscall, CMP_BoxContains, 0x00565C20, const ColBox* box, const void* point);
GAME_FN(void, __cdecl, CMP_CleanUpOldReference, 0x004657E0, void* entity, re::Ptr32<void>* ref);
GAME_FN(CColModel*, __cdecl, CMP_FindColModelByKey, 0x005E8C80, int key);
GAME_FN(int, __cdecl, CMP_FindNameInList, 0x0073AC90, const re::Ptr32<const char>* names, int count, const char* name, int fallback, int);
// ours (the sweeps, ColSweepTests.cpp), through the game's addresses
GAME_FN(uint8_t, __cdecl, CMP_SweepSphereSphere, 0x00576B50, const SweepSphere* a, const SweepSphere* b, float maxT, const CVector* velA,
        const CVector* velB, float* t, CVector* point);
GAME_FN(uint8_t, __cdecl, CMP_SweepSphereBox, 0x005767B0, const CVector* boxMin, const CVector* boxMax, const SweepSphere* sphere,
        const CVector* dir, float* t, CVector* hitNormal, CVector* hitPoint, uint8_t* face, int* touched);
GAME_FN(uint8_t, __cdecl, CMP_SweepSphereTriangle, 0x00575660, const re::Ptr32<const CVector>* triangle, const CVector* normal, const SweepSphere* sphere,
        const CVector* dir, float* t, CVector* hitNormal, CVector* hitPoint, uint8_t* face);
GAME_FN(int, __cdecl, CMP_ProcessColModels, 0x0056C050, const CMatrix* matA, CColModel* colA, const CMatrix* matB, CColModel* colB,
        ColPoint* spherePoints, ColPoint* linePoints, float* lineDists, int* numPoints, int maxPoints);

// ProcessColModels' work arrays (static: the spheres transformed, and the indices of
// what passed the bounding tests)
GAME_VAR(uint32_t, g_colWorkInit, 0x00CF3068);             // (the arrays' constructed bits)
GAME_ARRAY(SweepSphere, g_colSpheresA, 0x00CF2D68);        // A's spheres in B's space
GAME_ARRAY(int32_t, g_colSphereIndicesA, 0x00CF2CA8);
GAME_ARRAY(SweepSphere, g_colSpheresB, 0x00CF29A8);        // B's spheres in A's space
GAME_ARRAY(int32_t, g_colSphereIndicesB, 0x00CF28E8);
GAME_ARRAY(int32_t, g_colBoxIndicesB, 0x00CF2868);
GAME_ARRAY(int32_t, g_colTriangleIndicesB, 0x00CF27E8);
GAME_ARRAY(ColLine, g_colLinesA, 0x00CF26E8);              // A's lines in B's space
GAME_ARRAY(uint8_t, g_colLineHit, 0x00CF26E0);
GAME_VAR(CVector, g_upVector, 0x009E33DC);                 // (0, 0, 1)
GAME_VAR(void*, g_entity_C2A8E8, 0x00C2A8E8);
inline const re::Ptr32<const char>* const kSurfaceNames = reinterpret_cast<const re::Ptr32<const char>*>(0x0091BDE0);

CColSphere* SpheresOf(CCollisionData* data) {
    return data->spheres;
}
ColLine* LinesOf(CCollisionData* data) {
    return static_cast<ColLine*>(data->unknown04);
}
ColBox* BoxesOf(CCollisionData* data) {
    return re::PtrCast<ColBox*>(data->boxes);
}
TriangleTree* TreeOf(CCollisionData* data) {
    return re::PtrCast<TriangleTree*>(data->planes);
}
uint16_t SurfaceWord(const void* p) {
    uint16_t w;
    std::memcpy(&w, p, 2);
    return w;
}

} // namespace

namespace ColModelProcess {

// 0x56BB20: a vertical line from pos down (or up) to z still crosses the stored triangle;
// the crossing into point. Otherwise the triangle is forgotten.
uint8_t __cdecl IsStoredPolyStillValidVerticalLine(const CVector* pos, float z, CVector* point, StoredCollPoly* poly) {
    if (!poly->valid) {
        return 0;
    }
    CVector p1 = *pos;
    float p2x = pos->x;
    float p2y = pos->y;
    const float* v0 = poly->verts[0];
    const float* v1 = poly->verts[1];
    const float* v2 = poly->verts[2];
    float e1x = v1[0] - v0[0];
    float e1y = v1[1] - v0[1];
    float e1z = v1[2] - v0[2];
    float e2x = v2[0] - v0[0];
    float e2y = v2[1] - v0[1];
    float e2z = v2[2] - v0[2];
    CVector n;
    n.x = e2y * e1z - e2z * e1y;
    n.y = e2z * e1x - e1z * e2x;
    n.z = e1y * e2x - e2y * e1x;
    CMP_Normalise2(&n);
    float d = n.x * v0[0] + n.y * v0[1] + n.z * v0[2];

    // the plane to test the corners in: the one the normal is least along
    float ax = std::fabs(n.x);
    float ay = std::fabs(n.y);
    float az = std::fabs(n.z);
    int axis;
    if (!(ay < ax) || !(az < ax)) {
        if (!(az < ay)) {
            axis = (0.0f < n.z) ? 4 : 5;
        } else {
            axis = (0.0f < n.y) ? 2 : 3;
        }
    } else {
        axis = (0.0f < n.x) ? 0 : 1;
    }

    // both ends on one side: no crossing
    float nxpx = n.x * p1.x;
    float side2 = n.x * p2x + n.y * p2y + n.z * z;
    side2 = side2 - d;
    float side1 = nxpx + n.y * p1.y + n.z * p1.z;
    side1 = side1 - d;
    if (!(side1 * side2 <= 0.0f)) {
        poly->valid = 0;
        return 0;
    }
    float dx = p2x - p1.x;
    float dy = p2y - p1.y;
    float dz = z - p1.z;
    float t = (d - nxpx - n.y * p1.y - n.z * p1.z) / (n.x * dx + n.y * dy + n.z * dz);
    CVector hit;
    hit.x = dx * t + p1.x;
    hit.y = p1.y + dy * t;
    hit.z = p1.z + t * dz;

    // the corners and the crossing in 2D (u, v), wound by the normal's side
    float au, av, bu, bv, cu, cv, pu, pv;
    switch (axis) {
    case 1:
        au = v0[1], av = v0[2], cu = v1[1], cv = v1[2], bu = v2[1], bv = v2[2], pu = hit.y, pv = hit.z;
        break;
    case 2:
        au = v0[2], av = v0[0], bu = v1[2], bv = v1[0], cu = v2[2], cv = v2[0], pu = hit.z, pv = hit.x;
        break;
    case 3:
        au = v0[2], av = v0[0], cu = v1[2], cv = v1[0], bu = v2[2], bv = v2[0], pu = hit.z, pv = hit.x;
        break;
    case 4:
        au = v0[0], av = v0[1], bu = v1[0], bv = v1[1], cu = v2[0], cv = v2[1], pu = hit.x, pv = hit.y;
        break;
    case 5:
        au = v0[0], av = v0[1], cu = v1[0], cv = v1[1], bu = v2[0], bv = v2[1], pu = hit.x, pv = hit.y;
        break;
    default:
        au = v0[1], av = v0[2], bu = v1[1], bv = v1[2], cu = v2[1], cv = v2[2], pu = hit.y, pv = hit.z;
        break;
    }
    if ((cu - au) * (pv - av) - (cv - av) * (pu - au) < 0.0f) {
        poly->valid = 0;
        return 0;
    }
    if ((pv - av) * (bu - au) - (pu - au) * (bv - av) > 0.0f) {
        poly->valid = 0;
        return 0;
    }
    if (!((bu - cu) * (pv - cv) - (pu - cu) * (bv - cv) < 0.0f)) {
        *point = hit;
        return 1;
    }
    poly->valid = 0;
    return 0;
}

// 0x56C050: GTA's ProcessColModels: B's volumes against A's spheres (in B's space: the
// contacts from *numPoints on, 32 at most, back in world space) and A's lines (one
// point and distance each); the contacts added. maxPoints isn't read (32 is fixed).
// Models with triangle neighbours (+0x18) aren't tested.
int __cdecl ProcessColModels(const CMatrix* matA, CColModel* colA, const CMatrix* matB, CColModel* colB, ColPoint* spherePoints,
                             ColPoint* linePoints, float* lineDists, int* numPoints, int /*maxPoints*/) {
    CCollisionData* dataB = colB->data;
    CCollisionData* dataA = colA->data;
    if (dataA == nullptr || dataB == nullptr || dataB->unknown18 != nullptr) {
        return 0;
    }
    if (dataA != nullptr && dataA->unknown18 != nullptr) {
        return 0;
    }
    if (!(g_colWorkInit & 1)) {
        g_colWorkInit |= 1;
    }
    if (!(g_colWorkInit & 2)) {
        g_colWorkInit |= 2;
    }
    if (!(g_colWorkInit & 4)) {
        g_colWorkInit |= 4;
    }

    // A in B's space, and A's bounding sphere there against B's box
    CMatrix aInB;
    aInB.attached = nullptr;
    CMP_Matrix_Invert(matB, &aInB);
    CMP_Matrix_MultiplyBy(&aInB, matA);
    CVector tmp;
    CMP_MultiplyPoint(&tmp, &aInB, &colA->sphereCenter);
    SweepSphere boundA = {tmp, colA->sphereRadius};
    SweepSphere boundA2 = boundA;
    const CVector* boxMinB = &colB->boxMin;
    if (!CMP_SphereTouchesBox(&boundA2, boxMinB, &colB->boxMax)) {
        return 0;
    }

    // A's spheres near B
    CColSphere* spheresA = SpheresOf(dataA);
    for (int i = 0; i < int8_t(dataA->numSpheres); i++) {
        CMP_MultiplyPoint(&tmp, &aInB, &spheresA[i].center);
        g_colSpheresA[i].center = tmp;
        g_colSpheresA[i].radius = spheresA[i].radius;
    }
    int numSpheresA = 0;
    if (int8_t(dataA->numSpheres) > 0) {
        for (int i = 0; i < int8_t(dataA->numSpheres); i++) {
            if (CMP_SphereTouchesBox(&g_colSpheresA[i], boxMinB, &colB->boxMax)) {
                g_colSphereIndicesA[numSpheresA++] = i;
            }
        }
        if (numSpheresA != 0) {
            goto spheresNear;
        }
    }
    if (dataA->unknown20 == 0) {
        return 0;
    }
spheresNear:

    // B's spheres near A (tested in A's space)
    int numSpheresB = 0;
    if (dataB->numSpheres != 0) {
        CMatrix bInA;
        bInA.attached = nullptr;
        CMP_Matrix_Invert(matA, &bInA);
        CMP_Matrix_MultiplyBy(&bInA, matB);
        CColSphere* spheresB = SpheresOf(dataB);
        for (int i = 0; i < int8_t(dataB->numSpheres); i++) {
            CMP_MultiplyPoint(&tmp, &bInA, &spheresB[i].center);
            g_colSpheresB[i].center = tmp;
            g_colSpheresB[i].radius = spheresB[i].radius;
        }
        for (int i = 0; i < int8_t(dataB->numSpheres); i++) {
            if (CMP_SphereTouchesBox(&g_colSpheresB[i], &colA->boxMin, &colA->boxMax)) {
                g_colSphereIndicesB[numSpheresB++] = i;
            }
        }
    }

    // B's boxes and triangles near A's bounding sphere
    int numBoxesB = 0;
    ColBox* boxesB = BoxesOf(dataB);
    for (int i = 0; i < int8_t(dataB->numBoxes); i++) {
        if (CMP_SphereTouchesBox(&boundA, &boxesB[i].min, &boxesB[i].max)) {
            g_colBoxIndicesB[numBoxesB++] = i;
        }
    }
    CColTriangle* trianglesB = dataB->triangles;
    CompressedVector* verticesB = dataB->vertices;
    int numTrianglesB = 0;
    if (dataB->numTriangles != 0) {
        TriangleTree* tree = TreeOf(dataB);
        if (tree == nullptr) {
            for (int i = 0; i < dataB->numTriangles; i++) {
                if (CMP_TestSphereTriangle(&boundA2, verticesB, &trianglesB[i])) {
                    g_colTriangleIndicesB[numTrianglesB++] = i;
                }
            }
        } else {
            for (uint16_t i = CMP_TreeFindFirst(tree, &boundA, colA->sphereRadius, 0); i != 0xFFFF; i = CMP_TreeFindNext(tree)) {
                if (CMP_TestSphereTriangle(&boundA2, verticesB, &trianglesB[i])) {
                    g_colTriangleIndicesB[numTrianglesB++] = i;
                }
            }
        }
    }
    if (numSpheresB == 0 && numBoxesB == 0 && numTrianglesB == 0) {
        return 0;
    }

    // A's spheres against them
    int first = *numPoints;
    int count = first;
    ColPoint* point = &spherePoints[first];
    point->depth = -1.0f;
    float touchDist = 1e+24f;
    CColSphere* spheresB = SpheresOf(dataB);
    for (int i = 0; i < numSpheresA; i++) {
        int index = g_colSphereIndicesA[i];
        const SweepSphere* sphere = &g_colSpheresA[index];
        const CColSphere* original = &spheresA[index];
        bool hit = false;
        for (int j = 0; j < numSpheresB; j++) {
            if (CMP_ProcessSphereSphere(sphere, &spheresB[g_colSphereIndicesB[j]], point, &touchDist)) {
                hit = true;
                std::memcpy(&point->surfaceA, &original->surface, 2);
            }
        }
        for (int j = 0; j < numBoxesB; j++) {
            if (CMP_ProcessSphereBox(sphere, &boxesB[g_colBoxIndicesB[j]], point, &touchDist)) {
                hit = true;
                std::memcpy(&point->surfaceA, &original->surface, 2);
            }
        }
        if (numTrianglesB != 0) {
            // (a CColSphere on the stack with only the center and radius set: the
            // triangle test reads the surface and piece from the uninitialised rest)
            CColSphere copy;
            copy.center = sphere->center;
            copy.radius = sphere->radius;
            for (int j = 0; j < numTrianglesB; j++) {
                if (CMP_ProcessSphereTriangle(&copy, verticesB, &trianglesB[g_colTriangleIndicesB[j]], point, &touchDist)) {
                    hit = true;
                }
            }
        }
        if (hit) {
            count++;
            point++;
            if (count == 32) {
                break;
            }
            point->depth = -1.0f;
        }
    }
    for (int i = first; i < count; i++) {
        ColPoint* p = &spherePoints[i];
        p->point = *CMP_MultiplyPoint(&tmp, matB, &p->point);
        p->normal = *CMP_Multiply3x3(&tmp, matB, &p->normal);
    }

    // A's lines against them: the nearest point on each
    if (dataA->unknown20 != 0) {
        CMP_TransformPointsStrided(g_colLinesA, int8_t(dataA->unknown20) * 2, &aInB, LinesOf(dataA), 0x10);
    }
    for (int i = 0; i < int8_t(dataA->unknown20); i++) {
        ColLine* line = &g_colLinesA[i];
        g_colLineHit[i] = 0;
        for (int j = 0; j < numSpheresB; j++) {
            if (CMP_ProcessLineSphere(line, &spheresB[g_colSphereIndicesB[j]], linePoints, lineDists)) {
                g_colLineHit[i] = 1;
            }
        }
        for (int j = 0; j < numBoxesB; j++) {
            if (CMP_ProcessLineBox(line, &boxesB[g_colBoxIndicesB[j]], linePoints, lineDists)) {
                g_colLineHit[i] = 1;
            }
        }
        if (numTrianglesB != 0) {
            for (int j = 0; j < numTrianglesB; j++) {
                if (CMP_ProcessLineTriangle(line, verticesB, &trianglesB[g_colTriangleIndicesB[j]], linePoints, lineDists)) {
                    g_colLineHit[i] = 1;
                }
            }
        }
        if (g_colLineHit[i]) {
            linePoints->point = *CMP_MultiplyPoint(&tmp, matB, &linePoints->point);
            linePoints->normal = *CMP_Multiply3x3(&tmp, matB, &linePoints->normal);
        }
        lineDists++;
        linePoints++;
    }
    *numPoints = count;
    return count - first;
}

// 0x56C880: ProcessColModels from no contacts yet; how many
int __cdecl ProcessColModelsFresh(const CMatrix* matA, CColModel* colA, const CMatrix* matB, CColModel* colB, ColPoint* spherePoints,
                                  ColPoint* linePoints, float* lineDists) {
    int numPoints = 0;
    return CMP_ProcessColModels(matA, colA, matB, colB, spherePoints, linePoints, lineDists, &numPoints, 32);
}

// 0x56C8C0: A moving from matA0 to matA1 and B from matB0 to matB1: the first contact
// along the way (result->depth: the fraction of the move, from 1 when already inside a
// box). flags: 1 spheres against B's spheres, 2 against B's boxes, 8 against its
// triangles. One sphere only (onlyFirst), each sphere's own move (sphereMoves), stop at
// the first hit (stopAtFirst), a sphere inside a box counts (insideBoxes), spheres/pieces
// flagged skipped (skipPieces), the hit's own normal on triangles (hitNormals) instead of
// the triangle's.
uint8_t __cdecl ProcessSweptColModels(const CMatrix* matA0, const CMatrix* matA1, CColModel* colA, const CMatrix* matB0, const CMatrix* matB1,
                                      CColModel* colB, ColPoint* result, int onlyFirst, int sphereMoves, int stopAtFirst, int insideBoxes,
                                      int skipPieces, int flags, SweepInfo* info, int hitNormals) {
    CCollisionData* dataA = colA->data;
    CCollisionData* dataB = colB->data;
    if (dataA == nullptr || dataB == nullptr) {
        return 0;
    }
    ColPoint local;
    local.point = {0.0f, 0.0f, 0.0f};
    local.entity = nullptr;
    local.normal = {0.0f, 0.0f, 0.0f};
    local.depth = 3.4028235e+38f;
    if (result == nullptr) {
        result = &local;
    }
    const float* a0 = matA0->m;
    const float* a1 = matA1->m;
    const float* b0 = matB0->m;
    const float* b1 = matB1->m;
    CVector moveA = {a1[12] - a0[12], a1[13] - a0[13], a1[14] - a0[14]};
    CVector moveB = {b1[12] - b0[12], b1[13] - b0[13], b1[14] - b0[14]};
    CMatrix invB;
    invB.attached = nullptr;
    CMP_Matrix_Invert(matB0, &invB);
    CMatrix aInB;
    aInB.attached = nullptr;
    CMP_Matrix_Multiply(&aInB, &invB, matA0);
    result->depth = 3.4028235e+38f;

    uint8_t found = 0;
    CVector contact = {0.0f, 0.0f, 0.0f};
    SweepSphere sphereA = {{0.0f, 0.0f, 0.0f}, 0.0f};
    SweepSphere sphereB = {{0.0f, 0.0f, 0.0f}, 0.0f};
    CVector hitNormal = {0.0f, 0.0f, 0.0f};
    CVector hitPoint = {0.0f, 0.0f, 0.0f};
    float out4[4];
    CVector tmp, tmp2;

    int count = re::Flag(onlyFirst) ? 1 : int8_t(dataA->numSpheres);
    for (int index = 0; index < count; index++) {
        const CColSphere* sa = &dataA->spheres[index];
        if (re::Flag(skipPieces) && sa->piece != 0) {
            continue;
        }
        CVector center = sa->center;
        CMP_MultiplyPoint4(out4, matA0, &center);
        sphereA.center = {out4[0], out4[1], out4[2]};
        sphereA.radius = sa->radius;
        if (re::Flag(sphereMoves)) {
            float end4[4];
            CMP_MultiplyPoint4(end4, matA1, &center);
            moveA = *CMP_VectorSub(&tmp, reinterpret_cast<CVector*>(end4), &sphereA.center);
        }

        // against B's spheres (both moving)
        if (flags & 1) {
            for (int j = 0; j < int8_t(dataB->numSpheres); j++) {
                const CColSphere* sb = &dataB->spheres[j];
                if (re::Flag(skipPieces) && sb->piece != 0) {
                    continue;
                }
                CVector centerB = sb->center;
                CMP_MultiplyPoint4(out4, matB0, &centerB);
                sphereB.center = {out4[0], out4[1], out4[2]};
                sphereB.radius = sb->radius;
                float t = result->depth;
                if (CMP_SweepSphereSphere(&sphereA, &sphereB, 1.0f, &moveA, &moveB, &t, &contact) && result->depth > t) {
                    result->depth = t;
                    result->point = contact;
                    std::memcpy(&result->surfaceA, &sa->surface, 2);
                    std::memcpy(&result->surfaceB, &sb->surface, 2);
                    result->index = index;
                    found = 1;
                    CVector movedA = {moveA.x * t, moveA.y * t, t * moveA.z};
                    CVector posA;
                    CMP_VectorAdd(&posA, &movedA, &sphereA.center);
                    CVector movedB = {t * moveB.x, moveB.y * t, t * moveB.z};
                    CVector posB;
                    CMP_VectorAdd(&posB, &movedB, &sphereB.center);
                    posA.x = posA.x - posB.x;
                    posA.y = posA.y - posB.y;
                    posA.z = posA.z - posB.z;
                    CMP_Normalise(&posA);
                    result->normal = posA;
                }
            }
            if (re::Flag(stopAtFirst) && found) {
                goto hitReturn;
            }
        }

        {
            // the sphere in B's space, moving by the relative move
            float inB4[4];
            CMP_MultiplyPoint4(inB4, &aInB, &center);
            SweepSphere sphereInB = {{inB4[0], inB4[1], inB4[2]}, sphereA.radius};
            CVector relative;
            relative.x = CMP_VectorSub(&tmp, &moveA, &moveB)->x;
            relative.y = CMP_VectorSub(&tmp, &moveA, &moveB)->y;
            relative.z = CMP_VectorSub(&tmp, &moveA, &moveB)->z;
            CVector move;
            CMP_Multiply3x3b(&move, &invB, &relative);
            float dist = std::sqrt(std::fabs(move.x * move.x + move.y * move.y + move.z * move.z));
            uint8_t face = 0;
            int touched = 0;

            // B's boxes
            if (flags & 2) {
                ColBox* boxes = BoxesOf(dataB);
                for (int j = 0; j < int8_t(dataB->numBoxes); j++) {
                    ColBox* box = &boxes[j];
                    float t = result->depth;
                    if (CMP_SweepSphereBox(&box->min, &box->max, &sphereInB, &move, &t, &hitNormal, &hitPoint, &face, &touched) && dist > t) {
                        if (touched > 1 && info) {
                            info->hit = 1;
                            info->multiple = 1;
                        }
                        float fraction = t / dist;
                        if (result->depth > fraction) {
                            CVector v = hitNormal;
                            CMP_Multiply3x3b(&tmp, matB0, &v);
                            hitNormal = tmp;
                            if (info) {
                                info->face = face;
                                info->normal = tmp;
                                info->hit = 1;
                            }
                            v = hitPoint;
                            CMP_MultiplyPoint4(out4, matB0, &v);
                            result->depth = fraction;
                            result->normal = hitNormal;
                            result->point = {out4[0], out4[1], out4[2]};
                            hitPoint = {out4[0], out4[1], out4[2]};
                            std::memcpy(&result->surfaceA, &sa->surface, 2);
                            std::memcpy(&result->surfaceB, &box->surface, 2);
                            result->index = index;
                            found = 1;
                        }
                    } else if (re::Flag(insideBoxes) && CMP_BoxContains(box, inB4)) {
                        CMP_MultiplyPoint4(out4, matB0, reinterpret_cast<CVector*>(inB4));
                        result->depth = 1.0f;
                        result->point = {out4[0], out4[1], out4[2]};
                        result->normal = g_upVector;
                        std::memcpy(&result->surfaceA, &sa->surface, 2);
                        std::memcpy(&result->surfaceB, &box->surface, 2);
                        result->index = index;
                        found = 1;
                    }
                }
                if (re::Flag(stopAtFirst) && found) {
                    goto hitReturn;
                }
            }

            // B's triangles (through its tree when it has one: those near the move)
            if (dataB->numTriangles > 0 && (flags & 8)) {
                CVector v0 = {0.0f, 0.0f, 0.0f};
                CVector v1 = {0.0f, 0.0f, 0.0f};
                CVector v2 = {0.0f, 0.0f, 0.0f};
                re::Ptr32<const CVector> corners[3] = {&v0, &v1, &v2};
                TriangleTree* tree = TreeOf(dataB);
                uint16_t next = 0xFFFF;
                int k = 0;
                if (tree != nullptr) {
                    float reach = dist + sphereA.radius;
                    next = CMP_TreeFindFirst(tree, inB4, reach, 0);
                    if (next == 0xFFFF) {
                        continue;
                    }
                } else if (dataB->numTriangles <= 0) {
                    continue;
                }
                for (;;) {
                    float t = result->depth;
                    const CColTriangle* ct = &dataB->triangles[tree != nullptr ? next : k];
                    CMP_GetVertex(colB, &v0, ct->a);
                    CMP_GetVertex(colB, &v1, ct->b);
                    CMP_GetVertex(colB, &v2, ct->c);
                    CVector edge1, edge2, normal;
                    CMP_VectorSub(&edge1, &v1, &v0);
                    CMP_VectorSub(&edge2, &v2, &v0);
                    CMP_CrossProduct(&normal, &edge2, &edge1);
                    CMP_Normalise(&normal);
                    if (CMP_SweepSphereTriangle(corners, &normal, &sphereInB, &move, &t, &hitNormal, &hitPoint, &face) && dist > t) {
                        touched++;
                        if (touched > 1 && info) {
                            info->hit = 1;
                            info->multiple = 1;
                        }
                        float fraction = t / dist;
                        if (result->depth > fraction) {
                            CVector v;
                            if (re::Flag(hitNormals)) {
                                v = hitNormal;
                                CMP_Multiply3x3b(&tmp, matB0, &v);
                                hitNormal = tmp;
                            } else {
                                v = normal;
                                CMP_Multiply3x3b(&tmp, matB0, &v);
                                normal = tmp;
                            }
                            result->normal = tmp;
                            if (info) {
                                info->normal = normal;  // (in B's space with hitNormals)
                                info->face = face;
                                info->hit = 1;
                            }
                            v = hitPoint;
                            CMP_MultiplyPoint4(out4, matB0, &v);
                            result->depth = fraction;
                            result->point = {out4[0], out4[1], out4[2]};
                            hitPoint = {out4[0], out4[1], out4[2]};
                            std::memcpy(&result->surfaceA, &sa->surface, 2);
                            result->surfaceB = ct->surface;
                            result->pieceB = 0;
                            result->index = index;
                            found = 1;
                        }
                    }
                    if (tree != nullptr) {
                        next = CMP_TreeFindNext(tree);
                        if (next == 0xFFFF) {
                            break;
                        }
                    } else if (++k >= dataB->numTriangles) {
                        break;
                    }
                }
            }
        }
    }
    // (the local point's entity is never set: nothing to release)
    if (local.entity != nullptr) {
        CMP_CleanUpOldReference(local.entity, &local.entity);
    }
    return found;

hitReturn:
    if (local.entity != nullptr) {
        CMP_CleanUpOldReference(local.entity, &local.entity);
    }
    (void)tmp2;
    return 1;
}

// 0x56DBE0 (thiscall): the bounds of the volumes (spheres, boxes, lines and triangle
// edges clipped to the height) under z; false when there are none at all
uint8_t __fastcall GetBoundsBelow(CCollisionData* data, re::Edx, CVector* min, CVector* max, float z) {
    min->x = 1e+11f;
    min->y = 1e+11f;
    min->z = 1e+11f;
    max->x = -1e+11f;
    max->y = -1e+11f;
    max->z = -1e+11f;
    if (data->spheres == nullptr && data->unknown04 == nullptr && data->boxes == nullptr && data->triangles == nullptr &&
        data->planes == nullptr) {
        return 0;
    }
    bool any = false;
    auto include = [&](float x0, float y0, float z0, float x1, float y1, float z1) {
        if (any) {
            min->x = (min->x <= x0) ? min->x : x0;
            min->y = (min->y <= y0) ? min->y : y0;
            min->z = (min->z <= z0) ? min->z : z0;
            max->x = (x1 <= max->x) ? max->x : x1;
            max->y = (y1 <= max->y) ? max->y : y1;
            max->z = (z1 <= max->z) ? max->z : z1;
        } else {
            min->x = x0;
            min->y = y0;
            min->z = z0;
            max->x = x1;
            max->y = y1;
            max->z = z1;
            any = true;
        }
    };
    for (int i = 0; i < int8_t(data->numSpheres); i++) {
        const CColSphere* s = &data->spheres[i];
        float r = s->radius;
        float lz = s->center.z - r;
        if (lz < z) {
            include(s->center.x - r, s->center.y - r, lz, r + s->center.x, r + s->center.y, r + s->center.z);
        }
    }
    const ColBox* boxes = re::PtrCast<const ColBox*>(data->boxes);
    for (int i = 0; i < int8_t(data->numBoxes); i++) {
        const ColBox* b = &boxes[i];
        if (b->min.z < z) {
            include(b->min.x, b->min.y, b->min.z, b->max.x, b->max.y, b->max.z);
        }
    }
    // a segment's box, its top cut down to z when it crosses it
    auto includeSegment = [&](const CVector& a, const CVector& b, bool always) {
        CVector lo, hi;
        lo.z = (a.z < b.z) ? a.z : b.z;
        lo.y = (a.y < b.y) ? a.y : b.y;
        lo.x = (a.x < b.x) ? a.x : b.x;
        hi.z = (b.z < a.z) ? a.z : b.z;
        hi.y = (b.y < a.y) ? a.y : b.y;
        hi.x = (b.x < a.x) ? a.x : b.x;
        if (lo.z < z && z < hi.z) {
            float f = (z - lo.z) / (hi.z - lo.z);
            CVector d;
            CMP_VectorSub(&d, &hi, &lo);
            hi.x = lo.x + f * d.x;
            hi.y = lo.y + d.y * f;
            hi.z = lo.z + d.z * f;
        }
        if (always || lo.z < z) {
            include(lo.x, lo.y, lo.z, hi.x, hi.y, hi.z);
        }
    };
    const ColLine* lines = static_cast<const ColLine*>(data->unknown04);
    for (int i = 0; i < int8_t(data->unknown20); i++) {
        includeSegment(lines[i].start, lines[i].end, false);
    }
    for (int i = 0; i < data->numTriangles; i++) {
        const uint16_t* corners = &data->triangles[i].a;
        for (int k = 0; k < 3;) {
            const CompressedVector& va = data->vertices[corners[k]];
            k++;
            const CompressedVector& vb = data->vertices[corners[k % 3]];
            CVector a = {float(va.x) * 0.0078125f, float(va.y) * 0.0078125f, float(va.z) * 0.0078125f};
            CVector b = {float(vb.x) * 0.0078125f, float(vb.y) * 0.0078125f, float(vb.z) * 0.0078125f};
            if (a.z <= z || b.z <= z) {
                includeSegment(a, b, true);
            }
        }
    }
    return 1;
}

// 0x56DBB0 (thiscall): the model a triangle neighbour key names: this one for 0, else
// the one with that key (+0x32)
CColModel* __fastcall FindByKey(CColModel* self, re::Edx, int key) {
    if (key != 0) {
        return CMP_FindColModelByKey(key);
    }
    return self;
}

// 0x5706F0: a triangle neighbour key: the index alone within the model, else with the
// other model's key in the high word
int __cdecl MakeTriangleKey(CColModel* model, CColModel* other, int triangle) {
    if (other == model) {
        return triangle;
    }
    return int(uint32_t(other->unknown32) * 0x10000u + uint32_t(triangle));
}

// 0x56EB60: a surface type by name (DEFAULT, TARMAC, DIRT, GRASS...; -1: unknown)
int __cdecl GetSurfaceTypeFromName(const char* name) {
    return CMP_FindNameInList(kSurfaceNames, 0x43, name, -1, 0);
}

} // namespace ColModelProcess

// ---- registration: all original until tested ---------------------------------------

#define COLMODELPROC_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(colmodelproc_original_, __LINE__);                                     \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(colmodelproc_original_, __LINE__), re::Mode::Original)

COLMODELPROC_REVERSED("CCollision::IsStoredPolyStillValidVerticalLine", 0x0056BB20, &ColModelProcess::IsStoredPolyStillValidVerticalLine);
COLMODELPROC_REVERSED("CCollision::ProcessColModels", 0x0056C050, &ColModelProcess::ProcessColModels);
COLMODELPROC_REVERSED("CCollision::ProcessColModelsFresh", 0x0056C880, &ColModelProcess::ProcessColModelsFresh);
COLMODELPROC_REVERSED("CCollision::ProcessSweptColModels", 0x0056C8C0, &ColModelProcess::ProcessSweptColModels);
COLMODELPROC_REVERSED("CColModel::FindByKey", 0x0056DBB0, &ColModelProcess::FindByKey);
COLMODELPROC_REVERSED("CCollisionData::GetBoundsBelow", 0x0056DBE0, &ColModelProcess::GetBoundsBelow);
COLMODELPROC_REVERSED("CColModel::MakeTriangleKey", 0x005706F0, &ColModelProcess::MakeTriangleKey);
COLMODELPROC_REVERSED("SurfaceTable::GetTypeFromName", 0x0056EB60, &ColModelProcess::GetSurfaceTypeFromName);
