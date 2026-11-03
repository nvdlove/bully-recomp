// The walkable mesh query service, continued (see WalkableMeshQuery.cpp): dropping a
// point onto the meshes (a line down against each mesh), the triangles near a point, the
// nav point below or near a position, blocking and unblocking triangles under objects
// ("Walkable" nodes), a mesh's registration (its links and connections), and the small
// wrappers the game calls. 0x5EA250..0x5EB8B0.
//
// What the game keeps on the x87 stack is kept in doubles; floats where it stores them.

#include <cmath>
#include <cstddef>
#include <cstdint>

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
// An {entity, triangle} pair (the mesh's owner)
struct NavEntityRef {
    re::Ptr32<uint8_t> entity;
    int32_t tri;
};
// A triangle found near a point (0x5EA460)
struct NearTriangle {
    int32_t tri;      // +0x0
    WqVec closest;    // +0x4 (in the mesh's space)
    float dist;       // +0x10
};
static_assert(sizeof(NearTriangle) == 0x14);
// A connection being made (see WalkableMeshQuery.cpp)
struct NavConnection {
    re::Ptr32<uint8_t> colA;
    re::Ptr32<uint8_t> colB;
    uint16_t triA, triB;
    uint16_t edgeA, edgeB;
};
static_assert(sizeof(NavConnection) == 0x10);
// A line (0x564950 makes one from two points)
struct WqLine {
    float start[4];
    float end[4];
};

GAME_ARRAY(uint8_t*, wq2_navEntities, 0x00D134D8);
GAME_VAR(uint32_t, wq2_navEntityCount, 0x00D1353C);
GAME_VAR(uint32_t, wq2_cacheCount, 0x00D134D4);
GAME_VAR(NavEntityRef, wq2_noEntity, 0x00D13728);
inline void* const wq2_obstacles = reinterpret_cast<void*>(0x00D12AF8);
GAME_VAR(uint8_t*, wq2_objectPool, 0x00C0F608);
GAME_VAR(uint8_t, wq2_loading, 0x00CF3072);
GAME_VAR(uint8_t, wq2_loadingOverride, 0x00BF380F);
inline const char* const wq2_walkableName = reinterpret_cast<const char*>(0x0092CD8C);   // "Walkable"

GAME_FN(uint8_t*, __cdecl, WQ2_GetColModel, 0x0051AF50, void* entity);
GAME_FN(void*, __thiscall, WQ2_GetMatrix, 0x0046DD20, void* placeable);
GAME_FN(void, __cdecl, WQ2_MatrixInvert, 0x00413770, const void* matrix, void* out);
// (0x0045D500 stores 16 bytes through `out`: called through re::CallVec16)
static inline WqVec* WQ2_TransformPoint(WqVec* out, const void* matrix, const WqVec* v) { return re::CallVec16<0x0045D500, WqVec*>(out, matrix, v); }
GAME_FN(WqVec*, __cdecl, WQ2_InverseTransformPoint, 0x00414D20, WqVec* out, const void* matrix, const WqVec* v);
GAME_FN(WqLine*, __thiscall, WQ2_Line_Construct, 0x00564950, WqLine* self, const void* a, const void* b);
GAME_FN(WqVec*, __cdecl, WQ2_VectorSub, 0x00414B70, WqVec* out, const void* a, const void* b);
GAME_FN(WqVec*, __cdecl, WQ2_VectorAdd, 0x004129A0, WqVec* out, const void* a, const void* b);
GAME_FN(uint16_t, __thiscall, WQ2_Grid_FirstNear, 0x005617F0, void* grid, const WqVec* centre, float radius, int);
GAME_FN(uint16_t, __thiscall, WQ2_Grid_Next, 0x005618A0, void* grid);
GAME_FN(void, __thiscall, WQ2_ColPoint_Destruct, 0x00415500, void* colPoint);
GAME_FN(void, __thiscall, WQ2_ColGetVertex, 0x0056E6E0, void* col, WqVec* out, int index);
GAME_FN(void, __thiscall, WQ2_String_Construct, 0x005FA910, void* self, const char* s);
GAME_FN(void, __thiscall, WQ2_String_Destruct, 0x0049A650, void* self);
GAME_FN(uint8_t, __cdecl, WQ2_GetNodeTransform, 0x00467D80, void* entity, uint32_t name, void* transform, int);
GAME_FN(void, __cdecl, WQ2_5B33D0, 0x005B33D0, int model);
GAME_FN(void, __thiscall, WQ2_Point_Set, 0x005E7970, NavPoint* self, uint8_t* col, int32_t tri);
GAME_FN(uint8_t, __thiscall, WQ2_Point_Neighbour, 0x005E7FF0, const NavPoint* self, NavPoint* out, int edge, int flags,
        int area);
GAME_FN(int, __thiscall, WQ2_Point_EdgeTo, 0x005E7B10, const NavPoint* self, const NavPoint* other);
GAME_FN(uint8_t, __thiscall, WQ2_Point_CheckClearance, 0x005E8280, const NavPoint* self, const WqVec* p, int fromEdge,
        int flags, int area);
GAME_FN(int, __thiscall, WQ2_Point_FindExit, 0x005E8730, const NavPoint* self, const void* path, int except,
        WqVec* cross, uint8_t* blocked, int flags, int area);
GAME_FN(uint8_t, __thiscall, WQ2_Box_ContainsMargin, 0x005E8940, const void* box, const WqVec* p, float margin);
GAME_FN(uint8_t, __cdecl, WQ2_IsMeshLink, 0x005E8BE0, const WqVec* p);
GAME_FN(void, __cdecl, WQ2_AddEdge, 0x005E8CC0, const void* edge);
GAME_FN(void, __cdecl, WQ2_AddConnection, 0x005E8F70, const NavConnection* c);
GAME_FN(uint8_t, __cdecl, WQ2_ModelsConnected, 0x005E90C0, int modelA, int modelB);
GAME_FN(uint8_t, __cdecl, WQ2_LineHitsMesh, 0x005E9200, const void* line, const void* matrix, uint8_t* col,
        float* fraction, int32_t* tri);
GAME_FN(float, __cdecl, WQ2_ClosestOnTriangle, 0x005E94A0, const WqVec* p, const void* vertices, const void* tri,
        WqVec* out);
GAME_FN(uint8_t, __thiscall, WQ2_Obstacles_AnyContains, 0x005E5550, void* list, void* except, const WqVec* p);
GAME_FN(uint8_t, __thiscall, WQ2_Obstacles_AnyCrosses, 0x005E55D0, void* list, void* except, const WqVec* a,
        const WqVec* b);
// this file's
GAME_FN(uint8_t, __cdecl, WQ2_DropOntoMesh, 0x005EA250, float x, float y, float zTop, float drop, NavEntityRef* ref);
GAME_FN(uint8_t, __cdecl, WQ2_TrianglesNear, 0x005EA460, const WqVec* p, float radius, uint8_t* col,
        NearTriangle* out, int* count);
GAME_FN(uint8_t, __cdecl, WQ2_FindMeshLinkPartner, 0x005EA680, const WqVec* pos, NavConnection* out);
GAME_FN(uint8_t, __cdecl, WQ2_FindNavPointBelow, 0x005EA870, const WqVec* pos, float range, NavPoint* out,
        NavEntityRef* ref, float radius);
GAME_FN(void, __cdecl, WQ2_RegisterConnections, 0x005EACF0, uint8_t* col);
GAME_FN(uint8_t, __cdecl, WQ2_FindNavPointNear, 0x005EAEB0, const WqVec* pos, NavPoint* out, float range,
        NavEntityRef* ref, uint8_t* offMesh, float farRange);
GAME_FN(uint8_t, __cdecl, WQ2_IsOnWalkable, 0x005EAFA0, const WqVec* pos, float range, int checkOpen,
        NavEntityRef* ref);
GAME_FN(void, __cdecl, WQ2_BlockUnderObject, 0x005EB060, uint8_t* entity, int unblock);
GAME_FN(void, __cdecl, WQ2_BlockObjectsOver, 0x005EB6C0, uint8_t* owner, uint8_t* col);

uint8_t* TriData(const NavPoint& p) {
    uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(p.col + 0x38);
    return *reinterpret_cast<re::Ptr32<uint8_t>*>(data + 0x10) + p.tri * 8;
}
const float* EntityPos(const uint8_t* entity) {
    const uint8_t* matrix = *reinterpret_cast<const re::Ptr32<uint8_t>*>(entity + 0x14);
    return matrix ? reinterpret_cast<const float*>(matrix + 0x30) : reinterpret_cast<const float*>(entity + 4);
}
// the owner's inverse matrix times pos
WqVec ToMeshSpace(void* matrix, const float* pos) {
    alignas(16) uint8_t inverse[0x48];
    *reinterpret_cast<uint32_t*>(inverse + 0x40) = 0;
    WQ2_MatrixInvert(matrix, inverse);
    WqVec copy = {pos[0], pos[1], pos[2]};
    WqVec local;
    WQ2_TransformPoint(&local, inverse, &copy);
    return local;
}

// 0x5EA250: a line from (x, y, zTop) `drop` down against the meshes (the ref's own mesh
// first): the mesh and triangle hit into ref (the hit point is worked out and dropped)
uint8_t __cdecl DropOntoMesh(float x, float y, float zTop, float drop, NavEntityRef* ref) {
    float top[3] = {x, y, zTop};
    float bottom[3] = {x, y, float(zTop - drop)};
    float fraction = 1.0f;
    if (ref->entity == nullptr) {
        ref->tri = -1;
    } else {
        uint32_t i = 0;
        while (i < wq2_navEntityCount && wq2_navEntities[i] != ref->entity) {
            i++;
        }
        if (i >= wq2_navEntityCount) {
            ref->entity = nullptr;
        } else {
            uint8_t* col = WQ2_GetColModel(ref->entity);
            void* matrix = WQ2_GetMatrix(ref->entity);
            WqLine line;
            if (WQ2_LineHitsMesh(WQ2_Line_Construct(&line, top, bottom), matrix, col, &fraction, &ref->tri)) {
                return 1;
            }
        }
    }
    for (uint32_t i = 0; i < wq2_navEntityCount; i++) {
        if (ref->entity && wq2_navEntities[i] == ref->entity) {
            continue;
        }
        uint8_t* col = WQ2_GetColModel(wq2_navEntities[i]);
        void* matrix = WQ2_GetMatrix(wq2_navEntities[i]);
        WqLine line;
        if (WQ2_LineHitsMesh(WQ2_Line_Construct(&line, top, bottom), matrix, col, &fraction, &ref->tri)) {
            ref->entity = wq2_navEntities[i];
            return 1;
        }
    }
    ref->entity = nullptr;
    ref->tri = -1;
    return 0;
}

// 0x5EA460: the mesh's triangles within radius of p (in its space; flat bounds first),
// each with its nearest point, up to *count
uint8_t __cdecl TrianglesNear(const WqVec* p, float radius, uint8_t* col, NearTriangle* out, int* count) {
    uint8_t colPoint[0x2C] = {};
    *reinterpret_cast<float*>(colPoint + 0x24) = 3.4028235e+38f;
    uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(col + 0x38);
    if (!data) {
        return 0;
    }
    const float* min = reinterpret_cast<const float*>(col + 0x10);
    const float* max = reinterpret_cast<const float*>(col + 0x20);
    if (min[0] > double(p->x) + radius || max[0] < double(p->x) - radius) {
        return 0;
    }
    if (min[1] > double(p->y) + radius || max[1] < double(p->y) - radius) {
        return 0;
    }
    void* vertices = *reinterpret_cast<re::Ptr32<void>*>(data + 0xC);
    uint8_t* tris = *reinterpret_cast<re::Ptr32<uint8_t>*>(data + 0x10);
    void* grid = *reinterpret_cast<re::Ptr32<void>*>(data + 0x14);
    WqVec centre = {p->x, p->y, p->z};
    uint8_t any = 0;
    int found = 0;
    for (uint32_t i = WQ2_Grid_FirstNear(grid, &centre, radius, 1); i != 0xFFFF; i = WQ2_Grid_Next(grid)) {
        WqVec closest = {0.0f, 0.0f, 0.0f};
        float d = WQ2_ClosestOnTriangle(p, vertices, tris + i * 8, &closest);
        if (!(radius > d)) {
            continue;
        }
        out[found].dist = d;
        out[found].closest = closest;
        out[found].tri = int32_t(i);
        found++;
        any = 1;
        if (found >= *count) {
            break;
        }
    }
    *count = found;
    WQ2_ColPoint_Destruct(colPoint);
    return any;
}

// 0x5EA680: a mesh triangle within 0.1 of pos with a whole edge on mesh links: the other
// side of a connection
uint8_t __cdecl FindMeshLinkPartner(const WqVec* pos, NavConnection* out) {
    NearTriangle found[50];
    for (int i = 0; i < 50; i++) {
        found[i].closest = {0.0f, 0.0f, 0.0f};
    }
    for (uint32_t e = 0; e < wq2_navEntityCount; e++) {
        int count = 50;
        uint8_t* col = WQ2_GetColModel(wq2_navEntities[e]);
        void* matrix = WQ2_GetMatrix(wq2_navEntities[e]);
        WqVec local = ToMeshSpace(matrix, &pos->x);
        if (!WQ2_TrianglesNear(&local, 0.1f, col, found, &count)) {
            continue;
        }
        uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(col + 0x38);
        for (int k = 0; k < count; k++) {
            const uint16_t* tri = reinterpret_cast<const uint16_t*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(data + 0x10)
                                                                    + found[k].tri * 8);
            for (int j = 0; j < 3; j++) {
                WqVec v;
                WQ2_ColGetVertex(col, &v, tri[j]);
                if (!WQ2_IsMeshLink(&v)) {
                    continue;
                }
                int next = j < 2 ? j + 1 : 0;
                WqVec w;
                WQ2_ColGetVertex(col, &w, tri[next]);
                if (WQ2_IsMeshLink(&w)) {
                    out->edgeB = uint16_t(j);
                    out->colB = col;
                    out->triB = uint16_t(found[k].tri);
                    return 1;
                }
            }
        }
    }
    return 0;
}

// 0x5EA870: the nav point below pos (the nearest over the triangles near it within
// radius, above it by less than range): out and ref
uint8_t __cdecl FindNavPointBelow(const WqVec* pos, float range, NavPoint* out, NavEntityRef* ref, float radius) {
    if (!ref) {
        wq2_noEntity.entity = nullptr;
        wq2_noEntity.tri = -1;
        ref = &wq2_noEntity;
    }
    NearTriangle found[50];
    for (int i = 0; i < 50; i++) {
        found[i].closest = {0.0f, 0.0f, 0.0f};
    }
    float best = 3.4028235e+38f;
    for (uint32_t e = 0; e < wq2_navEntityCount; e++) {
        int count = 50;
        uint8_t* col = WQ2_GetColModel(wq2_navEntities[e]);
        const float* m = static_cast<const float*>(WQ2_GetMatrix(wq2_navEntities[e]));
        WqVec local = ToMeshSpace(const_cast<float*>(m), &pos->x);
        if (!WQ2_TrianglesNear(&local, radius, col, found, &count) || count <= 0) {
            continue;
        }
        for (int k = 0; k < count; k++) {
            WqVec p = found[k].closest;
            float wx = float(double(m[0]) * p.x + double(p.y) * m[4] + double(p.z) * m[8] + m[12]);
            float wy = float(double(m[1]) * p.x + double(m[5]) * p.y + double(p.z) * m[9] + m[13]);
            float wz = float(double(p.x) * m[2] + double(p.y) * m[6] + double(p.z) * m[10] + m[14]);
            float dz = pos->z - wz;
            float t = found[k].dist + dz;
            float sq = t * t;
            if (!(best > sq)) {
                continue;
            }
            if (!(0.0f < dz) || !(range > dz)) {
                continue;
            }
            WQ2_Point_Set(out, col, found[k].tri);
            best = sq;
            out->pos = {wx, wy, wz};
            ref->tri = found[k].tri;
            ref->entity = wq2_navEntities[e];
        }
    }
    return best < 3.4028234663852886e+38 ? 1 : 0;
}

// 0x5EAB30: the triangles below pos (within radius, less than range under it) blocked
// (unblock: opened)
void __cdecl BlockTrianglesBelow(const WqVec* pos, float radius, float range, int unblock) {
    NearTriangle found[50];
    for (int i = 0; i < 50; i++) {
        found[i].closest = {0.0f, 0.0f, 0.0f};
    }
    for (uint32_t e = 0; e < wq2_navEntityCount; e++) {
        int count = 50;
        uint8_t* col = WQ2_GetColModel(wq2_navEntities[e]);
        const float* m = static_cast<const float*>(WQ2_GetMatrix(wq2_navEntities[e]));
        WqVec local = ToMeshSpace(const_cast<float*>(m), &pos->x);
        if (!WQ2_TrianglesNear(&local, radius, col, found, &count)) {
            continue;
        }
        uint8_t* tris = *reinterpret_cast<re::Ptr32<uint8_t>*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(col + 0x38) + 0x10);
        for (int k = 0; k < count; k++) {
            WqVec p = found[k].closest;
            float wz = float(double(m[2]) * p.x + double(m[6]) * p.y + double(p.z) * m[10] + m[14]);
            float dz = pos->z - wz;
            if (!(dz > 0.0f) || !(dz < range)) {
                continue;
            }
            uint8_t& flags = tris[found[k].tri * 8 + 7];
            if (uint8_t(unblock) != 0) {
                flags &= 0xFB;
            } else {
                flags |= 4;
            }
        }
    }
}

// 0x5EACF0: a mesh's triangle edges: those on mesh links joined to the mesh across,
// the others' neighbours on other meshes noted as edges between the meshes
void __cdecl RegisterConnections(uint8_t* col) {
    uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(col + 0x38);
    WQ2_GetMatrix(*reinterpret_cast<re::Ptr32<void>*>(col + 0x3C));
    for (int i = 0; i < *reinterpret_cast<int16_t*>(data + 0x1C); i++) {
        const uint16_t* tri = reinterpret_cast<const uint16_t*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(data + 0x10) + i * 8);
        const int32_t* neighbours = reinterpret_cast<const int32_t*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(data + 0x18) + i * 0xC);
        for (int j = 0; j < 3; j++) {
            WqVec v;
            WQ2_ColGetVertex(col, &v, tri[j]);
            bool linked = false;
            if (WQ2_IsMeshLink(&v)) {
                int next = j < 2 ? j + 1 : 0;
                WqVec w;
                WQ2_ColGetVertex(col, &w, tri[next]);
                WqVec copy = w;
                linked = WQ2_IsMeshLink(&copy) != 0;
            }
            if (linked) {
                NavConnection c = {col, nullptr, uint16_t(i), 0xFFFF, uint16_t(j), 0xFFFF};
                if (WQ2_FindMeshLinkPartner(&v, &c)) {
                    WQ2_AddConnection(&c);
                }
                continue;
            }
            int32_t n = neighbours[j];
            int16_t mesh = int16_t(uint32_t(n) >> 16);
            if (n != -1 && mesh != 0) {
                uint16_t edge[5] = {*reinterpret_cast<uint16_t*>(col + 0x32), uint16_t(mesh), uint16_t(i),
                                    uint16_t(n), 0};
                WQ2_AddEdge(edge);
            }
        }
    }
}

// 0x5EAE60: is there a nav point below pos (within 50, radius)?
uint8_t __cdecl HasNavPointBelow(const WqVec* pos, float radius, NavEntityRef* ref) {
    NavPoint point = {nullptr, -1, {0.0f, 0.0f, 0.0f}};
    return WQ2_FindNavPointBelow(pos, 50.0f, &point, ref, radius) != 0;
}

// 0x5EAEB0: the nav point at pos (dropped from half a unit above, range down), else
// (with offMesh) the one below within farRange: offMesh says which
uint8_t __cdecl FindNavPointNear(const WqVec* pos, NavPoint* out, float range, NavEntityRef* ref, uint8_t* offMesh,
                                 float farRange) {
    if (!ref) {
        wq2_noEntity.entity = nullptr;
        wq2_noEntity.tri = -1;
        ref = &wq2_noEntity;
    }
    if (WQ2_DropOntoMesh(pos->x, pos->y, float(pos->z + 0.5), float(0.5 + range), ref)) {
        WQ2_Point_Set(out, WQ2_GetColModel(ref->entity), ref->tri);
        out->pos = *pos;
        if (offMesh) {
            *offMesh = 0;
        }
        return 1;
    }
    if (offMesh) {
        *offMesh = 1;
        return WQ2_FindNavPointBelow(pos, farRange, out, ref, 2.0f);
    }
    return 0;
}

// 0x5EAFA0: is pos on the meshes (within range; with checkOpen, on an unblocked
// triangle)?
uint8_t __cdecl IsOnWalkable(const WqVec* pos, float range, int checkOpen, NavEntityRef* ref) {
    NavPoint point = {nullptr, -1, {0.0f, 0.0f, 0.0f}};
    uint8_t ok = WQ2_FindNavPointNear(pos, &point, range, ref, nullptr, 50.0f);
    if (ok && uint8_t(checkOpen) != 0) {
        const uint8_t* tri = TriData(point);
        if (!tri || ((~(tri[7] >> 2)) & 1) == 0) {
            return 0;
        }
    }
    return ok;
}

// 0x5EB020: the nav point at pos (3.5 down, else within 50 below)
uint8_t __cdecl FindNavPoint(const WqVec* pos, NavPoint* out, NavEntityRef* ref, uint8_t* offMesh) {
    return WQ2_FindNavPointNear(pos, out, 3.5f, ref, offMesh, 50.0f);
}

// 0x5EB060: the triangle under an object's "Walkable" node (else its position) blocked
// (unblock: opened)
void __cdecl BlockUnderObject(uint8_t* entity, int unblock) {
    const float* at = EntityPos(entity);
    float x = at[0];
    float y = at[1];
    float z = at[2];
    NavPoint point = {nullptr, -1, {0.0f, 0.0f, 0.0f}};
    uint32_t name[4];
    WQ2_String_Construct(name, wq2_walkableName);
    float transform[0xD] = {};
    transform[0xC] = 1.0f;
    if (WQ2_GetNodeTransform(entity, name[0], transform, 1)) {
        x = transform[9];
        y = transform[10];
        z = transform[11];
    }
    wq2_noEntity.entity = nullptr;
    wq2_noEntity.tri = -1;
    if (WQ2_DropOntoMesh(x, y, float(z + 0.5), 4.0f, &wq2_noEntity)) {
        WQ2_Point_Set(&point, WQ2_GetColModel(wq2_noEntity.entity), wq2_noEntity.tri);
        point.pos = {x, y, z};
        uint8_t& flags = TriData(point)[7];
        if (uint8_t(unblock) == 0) {
            flags |= 4;
        } else {
            flags &= 0xFB;
        }
    }
    WQ2_String_Destruct(name);
}

// 0x5EB1B0: can a ped walk straight from a to b on the meshes (connected meshes, room at
// b, the flags and area allowing)? hitObstacle: a footprint is crossed on the way.
uint8_t __cdecl WalkStraightBetween(const WqVec* a, const WqVec* b, uint8_t* hitObstacle, int area, int flags) {
    NavPoint start = {nullptr, -1, {0.0f, 0.0f, 0.0f}};
    NavPoint end = {nullptr, -1, {0.0f, 0.0f, 0.0f}};
    *hitObstacle = 0;
    wq2_noEntity.entity = nullptr;
    wq2_noEntity.tri = -1;
    if (!WQ2_DropOntoMesh(a->x, a->y, float(a->z + 0.5), 4.0f, &wq2_noEntity)) {
        return 0;
    }
    WQ2_Point_Set(&start, WQ2_GetColModel(wq2_noEntity.entity), wq2_noEntity.tri);
    start.pos = *a;
    wq2_noEntity.entity = nullptr;
    wq2_noEntity.tri = -1;
    if (!WQ2_DropOntoMesh(b->x, b->y, float(b->z + 0.5), 4.0f, &wq2_noEntity)) {
        return 0;
    }
    WQ2_Point_Set(&end, WQ2_GetColModel(wq2_noEntity.entity), wq2_noEntity.tri);
    end.pos = *b;
    int modelA = *reinterpret_cast<int16_t*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(start.col + 0x3C) + 0x10E);
    int modelB = *reinterpret_cast<int16_t*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(end.col + 0x3C) + 0x10E);
    if (!WQ2_ModelsConnected(modelA, modelB)) {
        return 0;
    }
    if (!WQ2_Point_CheckClearance(&end, &end.pos, -1, uint8_t(flags), uint8_t(area))) {
        return 0;
    }
    NavPoint next = {nullptr, -1, {0.0f, 0.0f, 0.0f}};
    NavPoint cur = start;
    WqLine path = {{start.pos.x, start.pos.y, start.pos.z, 0.0f},
                   {end.pos.x + (end.pos.x - start.pos.x), end.pos.y + (end.pos.y - start.pos.y),
                    end.pos.z + (end.pos.z - start.pos.z), 0.0f}};
    WqVec cross = {0.0f, 0.0f, 0.0f};
    const uint8_t* tri = TriData(end);
    if (!tri || ((~(tri[7] >> 2)) & 1) == 0) {
        return 0;
    }
    if (uint8_t(area) != 0 && tri[6] != uint8_t(area)) {
        return 0;
    }
    int except = -1;
    for (;;) {
        if (end.tri == cur.tri && end.col == cur.col) {
            if (WQ2_Obstacles_AnyCrosses(wq2_obstacles, nullptr, a, b)) {
                *hitObstacle = 1;
            }
            return 1;
        }
        uint8_t blocked;
        int edge = WQ2_Point_FindExit(&cur, &path, except, &cross, &blocked, flags, area);
        if (edge == -1) {
            return 0;
        }
        path.start[0] = cross.x;
        path.start[1] = cross.y;
        path.start[2] = cross.z;
        if (!WQ2_Point_Neighbour(&cur, &next, edge, flags, area)) {
            return 0;
        }
        except = WQ2_Point_EdgeTo(&next, &cur);
        cur = next;
    }
}

// 0x5EB5E0: is pos on the meshes (4 down)? inObstacle: inside a footprint.
uint8_t __cdecl IsOnMesh(const WqVec* pos, uint8_t* inObstacle) {
    NavPoint point = {nullptr, -1, {0.0f, 0.0f, 0.0f}};
    *inObstacle = 0;
    wq2_noEntity.tri = -1;
    wq2_noEntity.entity = nullptr;
    if (!WQ2_DropOntoMesh(pos->x, pos->y, float(pos->z + 0.5), 4.0f, &wq2_noEntity)) {
        return 0;
    }
    WQ2_Point_Set(&point, WQ2_GetColModel(wq2_noEntity.entity), wq2_noEntity.tri);
    point.pos = *pos;
    if (WQ2_Obstacles_AnyContains(wq2_obstacles, nullptr, pos)) {
        *inObstacle = 1;
    }
    return 1;
}

// 0x5EB6C0: the objects (with a +0x180) standing on a mesh (within its box grown by 5)
// block the triangles under them
void __cdecl BlockObjectsOver(uint8_t* owner, uint8_t* col) {
    uint8_t* pool = wq2_objectPool;
    void* matrix = WQ2_GetMatrix(owner);
    int size = *reinterpret_cast<int32_t*>(pool + 8);
    const void* box = col + 0x10;
    for (int i = 0; i < size; i++) {
        if ((*reinterpret_cast<re::Ptr32<uint8_t>*>(pool + 4))[i] & 0x80) {
            continue;
        }
        uint8_t* object = *reinterpret_cast<re::Ptr32<uint8_t>*>(pool) + i * *reinterpret_cast<int32_t*>(pool + 0xC);
        if (!object || *reinterpret_cast<re::Ptr32<void>*>(object + 0x180) == nullptr) {
            continue;
        }
        const float* at = EntityPos(object);
        WqVec p = {at[0], at[1], at[2]};
        WqVec local;
        WQ2_InverseTransformPoint(&local, matrix, &p);
        if (WQ2_Box_ContainsMargin(box, &local, 5.0f)) {
            WQ2_BlockUnderObject(object, 0);
        }
    }
}

// 0x5EB790: a point's flag for the footprints (0x5E6120): on the meshes near (x, y, z)?
uint8_t __cdecl PointFlag(float x, float y, float z, float range, int checkOpen, NavEntityRef* ref) {
    WqVec p = {x, y, z};
    return WQ2_IsOnWalkable(&p, range, checkOpen, ref);
}

// 0x5EB7D0: is pos on the meshes (3.5; with checkOpen, unblocked)?
uint8_t __cdecl IsOnWalkableNear(const WqVec* pos, int checkOpen, NavEntityRef* ref) {
    NavPoint point = {nullptr, -1, {0.0f, 0.0f, 0.0f}};
    uint8_t ok = WQ2_FindNavPointNear(pos, &point, 3.5f, ref, nullptr, 50.0f);
    if (ok && uint8_t(checkOpen) != 0) {
        const uint8_t* tri = TriData(point);
        if (!tri || ((~(tri[7] >> 2)) & 1) == 0) {
            return 0;
        }
    }
    return ok;
}

// 0x5EB850: is pos on the meshes (3.5)? (The first argument is unused.)
uint8_t __cdecl IsOnWalkable2(int, const WqVec* pos, NavEntityRef* ref) {
    NavPoint point = {nullptr, -1, {0.0f, 0.0f, 0.0f}};
    return WQ2_FindNavPointNear(pos, &point, 3.5f, ref, nullptr, 50.0f) != 0;
}

// 0x5EB8B0: a nav mesh registered (not while loading, unless overridden): its links and
// edges, its owner in the list, the objects on it blocking their triangles
void __cdecl RegisterNavModel(uint8_t* col) {
    if (wq2_loading == 1 && wq2_loadingOverride == 0) {
        return;
    }
    uint8_t* owner = *reinterpret_cast<re::Ptr32<uint8_t>*>(col + 0x3C);
    if (!owner) {
        return;
    }
    WQ2_RegisterConnections(col);
    wq2_navEntities[wq2_navEntityCount] = owner;
    wq2_navEntityCount++;
    wq2_cacheCount = 0;
    WQ2_5B33D0(*reinterpret_cast<int16_t*>(owner + 0x10E));
    WQ2_BlockObjectsOver(owner, col);
}

#define WALKQ2_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(WalkQ2Original_, __LINE__);                                        \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(WalkQ2Original_, __LINE__), \
             re::Mode::Original)

WALKQ2_REVERSED("WalkableMeshQueryService::DropOntoMesh", 0x005EA250, &DropOntoMesh);
WALKQ2_REVERSED("WalkableMeshQueryService::TrianglesNear", 0x005EA460, &TrianglesNear);
WALKQ2_REVERSED("WalkableMeshQueryService::FindMeshLinkPartner", 0x005EA680, &FindMeshLinkPartner);
WALKQ2_REVERSED("WalkableMeshQueryService::FindNavPointBelow", 0x005EA870, &FindNavPointBelow);
WALKQ2_REVERSED("WalkableMeshQueryService::BlockTrianglesBelow", 0x005EAB30, &BlockTrianglesBelow);
WALKQ2_REVERSED("WalkableMeshQueryService::RegisterConnections", 0x005EACF0, &RegisterConnections);
WALKQ2_REVERSED("WalkableMeshQueryService::HasNavPointBelow", 0x005EAE60, &HasNavPointBelow);
WALKQ2_REVERSED("WalkableMeshQueryService::FindNavPointNear", 0x005EAEB0, &FindNavPointNear);
WALKQ2_REVERSED("WalkableMeshQueryService::IsOnWalkable", 0x005EAFA0, &IsOnWalkable);
WALKQ2_REVERSED("WalkableMeshQueryService::FindNavPoint", 0x005EB020, &FindNavPoint);
WALKQ2_REVERSED("WalkableMeshQueryService::BlockUnderObject", 0x005EB060, &BlockUnderObject);
WALKQ2_REVERSED("WalkableMeshQueryService::WalkStraightBetween", 0x005EB1B0, &WalkStraightBetween);
WALKQ2_REVERSED("WalkableMeshQueryService::IsOnMesh", 0x005EB5E0, &IsOnMesh);
WALKQ2_REVERSED("WalkableMeshQueryService::BlockObjectsOver", 0x005EB6C0, &BlockObjectsOver);
WALKQ2_REVERSED("WalkableMeshQueryService::PointFlag", 0x005EB790, &PointFlag);
WALKQ2_REVERSED("WalkableMeshQueryService::IsOnWalkableNear", 0x005EB7D0, &IsOnWalkableNear);
WALKQ2_REVERSED("WalkableMeshQueryService::IsOnWalkable2", 0x005EB850, &IsOnWalkable2);
WALKQ2_REVERSED("WalkableMeshQueryService::RegisterNavModel", 0x005EB8B0, &RegisterNavModel);

} // namespace
