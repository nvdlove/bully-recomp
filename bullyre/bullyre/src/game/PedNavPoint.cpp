// A nav mesh point's queries (thiscall on a NavPoint, see PedNavRoute.cpp): doors
// (area 5 triangles, a door triangle's far vertex above its edge's middle: open;
// area 6 the same the other way), the neighbour over an edge (the neighbours are
// {triangle, mesh id} pairs), the search's costs, walking round a vertex, clearance for
// the ped's radius (0xAEB940), and the way out of a triangle along a line.
// 0x5E7B90..0x5E8940. Also the walkable mesh query service's (0xD13150) reset, its
// mesh links (Config\MeshLink.dat) and its initialisation.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "reversed.h"

#include "game/Crt.h"
namespace {

struct NpVec {
    float x, y, z;
};
struct NavPoint {
    re::Ptr32<uint8_t> col;  // +0x0
    int32_t tri;   // +0x4
    NpVec pos;     // +0x8
};
static_assert(sizeof(NavPoint) == 0x14);
struct NavNode {
    NavPoint point;     // +0x0
    float cost;         // +0x14
    float estimate;     // +0x18
    re::Ptr32<NavNode> parent;    // +0x1C
    int16_t fromEdge;   // +0x20
    uint8_t depth;      // +0x22
    uint8_t pad23;
};
static_assert(sizeof(NavNode) == 0x24);
// A segment (see AvoidSegment.cpp)
struct NpSegment {
    float start[4];
    float end[4];
};
// The walkable mesh query service (0xD13150)
struct CWalkableMeshQueryService {
    uint8_t modelEdges[800];     // +0x0 (80 of 10 bytes)
    uint32_t modelEdgeCount;     // +0x320
    uint8_t cache[0x60];         // +0x324 (16 of 6 bytes)
    uint32_t cacheCount;         // +0x384
    re::Ptr32<uint8_t> navEntities[25];    // +0x388
    uint32_t navEntityCount;     // +0x3EC
    uint8_t connections[0xC0];   // +0x3F0 (12 of 16 bytes)
    uint32_t connectionCount;    // +0x4B0
    NpVec meshLinks[24];         // +0x4B4
    uint32_t meshLinkCount;      // +0x5D4
    uint32_t unknown5D8;         // +0x5D8
    int32_t unknown5DC;          // +0x5DC
};
static_assert(offsetof(CWalkableMeshQueryService, cacheCount) == 0x384);
static_assert(offsetof(CWalkableMeshQueryService, navEntityCount) == 0x3EC);
static_assert(offsetof(CWalkableMeshQueryService, connectionCount) == 0x4B0);
static_assert(offsetof(CWalkableMeshQueryService, meshLinkCount) == 0x5D4);
static_assert(sizeof(CWalkableMeshQueryService) == 0x5E0);

inline CWalkableMeshQueryService* const np_service = reinterpret_cast<CWalkableMeshQueryService*>(0x00D13150);
GAME_VAR(float, np_pedRadius, 0x00AEB940);
GAME_VAR(const char*, np_meshLinkFile, 0x00AEB944);   // "Config\MeshLink.dat"
inline const char* const np_readMode = reinterpret_cast<const char*>(0x008FF548);
inline const char* const np_threeFloats = reinterpret_cast<const char*>(0x0092CD80);   // "%f %f %f"

using EdgeFn = int(__cdecl*)(int edge);
inline const EdgeFn np_prevEdge = reinterpret_cast<EdgeFn>(0x005E79C0);
inline const EdgeFn np_nextEdge = reinterpret_cast<EdgeFn>(0x005E79E0);

GAME_FN(void, __thiscall, NP_ColGetTriangle, 0x0056E9C0, void* col, int tri, NpVec* verts);
GAME_FN(void, __thiscall, NP_ColGetEdge, 0x0056E8C0, void* col, NpVec* a, NpVec* b, int tri, int edge);
// (thiscall: returns ecx when id is 0)
GAME_FN(uint8_t*, __thiscall, NP_NavColModel, 0x0056DBB0, void* self, int id);
GAME_FN(int, __cdecl, NP_NavNeighbourId, 0x005706F0, void* col, void* otherCol, int tri);
GAME_FN(NpVec*, __cdecl, NP_VectorSub, 0x00414B70, NpVec* out, const NpVec* a, const NpVec* b);
GAME_FN(void, __thiscall, NP_Seg_Construct, 0x004D5A60, NpSegment* self, float, float, float, float, float, float);
GAME_FN(uint8_t, __thiscall, NP_Seg_Intersect2D, 0x005E6B30, const NpSegment* seg, const void* other, float* t,
        float* point);
GAME_FN(float, __thiscall, NP_Seg_FlatDistance, 0x005E6E60, const NpSegment* seg, const float* point);
GAME_FN(float, __thiscall, NP_Seg_Length, 0x005DCF40, const NpSegment* seg);
GAME_FN(NpVec*, __thiscall, NP_Seg_Normal2D, 0x005E7220, const NpSegment* seg, NpVec* out);
GAME_FN(NpVec*, __thiscall, NP_Seg_Closest, 0x005E74A0, const NpSegment* seg, NpVec* out, const NpVec* p, float* t);
GAME_FN(void*, __cdecl, NP_FileOpen, 0x0042D260, const char* name, const char* mode, int);
GAME_FN(uint8_t, __cdecl, NP_FileReadLine, 0x0042D330, void* file, char* buffer, int size);
GAME_FN(void, __cdecl, NP_FileClose, 0x0042D450, void* file);
template <typename... A>
inline int __cdecl NP_sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
GAME_FN(uint8_t, __thiscall, NP_Point_IsOpen, 0x005E1120, const NavPoint* self);
GAME_FN(uint8_t, __thiscall, NP_Point_Equals, 0x005E1100, const NavPoint* self, const NavPoint* other);
GAME_FN(int, __thiscall, NP_Point_EdgeTo, 0x005E7B10, const NavPoint* self, const NavPoint* other);
// this file's
GAME_FN(uint8_t, __thiscall, NP_IsDoorOpen, 0x005E7B90, const NavPoint* self, int edge);
GAME_FN(uint8_t, __thiscall, NP_CanPass, 0x005E7C40, const NavPoint* self, int edge, int flags);
GAME_FN(uint8_t, __thiscall, NP_Neighbour, 0x005E7FF0, const NavPoint* self, NavPoint* out, int edge, int flags,
        int area);
GAME_FN(uint8_t, __thiscall, NP_Walk, 0x005E8110, const NavPoint* self, int edge, EdgeFn fn, NavPoint* out,
        int* edgeOut, int flags, int area);
GAME_FN(uint8_t, __thiscall, NP_IsFanBlocked, 0x005E8220, const NavPoint* self, int edge, EdgeFn fn, int flags,
        int area);
GAME_FN(uint8_t, __thiscall, NP_CheckClearance, 0x005E8280, const NavPoint* self, const NpVec* p, int fromEdge,
        int flags, int area);
GAME_FN(void, __thiscall, NP_Service_LoadMeshLinks, 0x005E8B10, CWalkableMeshQueryService* self);

uint8_t* TriData(const NavPoint& p) {
    uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(p.col + 0x38);
    return *reinterpret_cast<re::Ptr32<uint8_t>*>(data + 0x10) + p.tri * 8;
}
uint8_t TriArea(const NavPoint& p) {
    return TriData(p)[6];
}
float SqrtAbs(double v) {
    return float(std::sqrt(double(std::fabs(float(v)))));
}

// 0x5E7B90 (thiscall): is the door on this edge open (the far vertex above the edge's
// middle)? (Edge -1 reads the stack slot before the heights: z2's integer.)
uint8_t __fastcall IsDoorOpen(const NavPoint* self, re::Edx, int edge) {
    uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(self->col + 0x38);
    const uint16_t* tri = reinterpret_cast<const uint16_t*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(data + 0x10) + self->tri * 8);
    const uint8_t* vertices = *reinterpret_cast<re::Ptr32<uint8_t>*>(data + 0xC);
    int32_t heights[3];
    for (int i = 0; i < 3; i++) {
        heights[i] = *reinterpret_cast<const int16_t*>(vertices + tri[i] * 6 + 4);
    }
    float slots[4];
    std::memcpy(&slots[0], &heights[2], 4);
    slots[1] = float(heights[0]);
    slots[2] = float(heights[1]);
    slots[3] = float(heights[2]);
    int e1 = (edge + 1) % 3;
    int e2 = (e1 + 1) % 3;
    float middle = float((double(slots[e1 + 1]) + slots[edge + 1]) * 0.5);
    return middle < slots[e2 + 1];
}

// 0x5E7C40 (thiscall): can a door triangle be crossed by this edge? (6: not when open;
// 5: not when open without the flags' 1)
uint8_t __fastcall CanPass(const NavPoint* self, re::Edx, int edge, int flags) {
    uint8_t area = TriArea(*self);
    bool five = area == 5;
    bool six = area == 6;
    if (five || six) {
        uint8_t open = NP_IsDoorOpen(self, edge);
        if (six && open) {
            return 0;
        }
        if (five && open && (flags & 1) == 0) {
            return 0;
        }
    }
    return 1;
}

// 0x5E7CA0 (thiscall): the triangle's edge nearest the point (flat), its line into out
// (the w's zero here)
int __fastcall DoorLine(const NavPoint* self, re::Edx, NpSegment* out) {
    NpVec verts[3] = {};
    NP_ColGetTriangle(self->col, self->tri, verts);
    float best = 3.4028235e+38f;
    int index = -1;
    float p[3] = {self->pos.x, self->pos.y, self->pos.z};
    for (int i = 0; i < 3; i++) {
        int next = i < 2 ? i + 1 : 0;
        NpSegment edge = {{verts[i].x, verts[i].y, verts[i].z, 0.0f},
                          {verts[next].x, verts[next].y, verts[next].z, 0.0f}};
        float d = NP_Seg_FlatDistance(&edge, p);
        if (best > d) {
            best = d;
            index = i;
            if (out) {
                *out = edge;
            }
        }
    }
    return index;
}

// 0x5E7E40 (thiscall, on a node): the neighbour's costs: at its shared edge's middle,
// the way so far plus the step, the straight distance to the goal (5 more into a door)
uint8_t __fastcall SetCosts(NavNode* self, re::Edx, NavNode* parent, int, float gx, float gy, float gz) {
    self->parent = parent;
    int edge = NP_Point_EdgeTo(&self->point, &parent->point);
    self->fromEdge = int16_t(edge);
    if (int16_t(edge) == -1) {
        return 0;
    }
    self->depth = uint8_t(parent->depth + 1);
    NpVec a;
    NpVec b;
    NP_ColGetEdge(self->point.col, &a, &b, self->point.tri, int16_t(edge));
    float sx = b.x + a.x;
    float sy = b.y + a.y;
    float sz = b.z + a.z;
    self->point.pos.x = float(sx * 0.5);
    self->point.pos.y = float(sy * 0.5);
    self->point.pos.z = float(sz * 0.5);
    self->cost = parent->cost;
    NpVec parentPos = parent->point.pos;
    NpVec d;
    NP_VectorSub(&d, &self->point.pos, &parentPos);
    float step = SqrtAbs(double(d.y) * d.y + double(d.x) * d.x + double(d.z) * d.z);
    self->cost = step + self->cost;
    NpVec goal = {gx, gy, gz};
    NP_VectorSub(&d, &goal, &self->point.pos);
    float h = SqrtAbs(double(d.y) * d.y + double(d.x) * d.x + double(d.z) * d.z);
    self->estimate = h;
    if (TriArea(self->point) == 5 && TriArea(parent->point) != 5) {
        self->estimate = float(h + 5.0);
    }
    return 1;
}

// 0x5E7FF0 (thiscall): the neighbour over an edge, when it's open, passes the flags (0x10:
// not through a closed area-6 door; into a door triangle only when it can be crossed; 4:
// not out of the area) and exists
uint8_t __fastcall Neighbour(const NavPoint* self, re::Edx, NavPoint* out, int edge, int flags, int area) {
    uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(self->col + 0x38);
    const uint8_t* neighbours = *reinterpret_cast<re::Ptr32<uint8_t>*>(data + 0x18) + self->tri * 0xC;
    if (*reinterpret_cast<const int32_t*>(neighbours + edge * 4) == -1) {
        return 0;
    }
    uint16_t id = *reinterpret_cast<const uint16_t*>(neighbours + edge * 4 + 2);
    int tri = *reinterpret_cast<const uint16_t*>(neighbours + edge * 4);
    uint8_t* col = NP_NavColModel(self->col, id);
    if (!col || !*reinterpret_cast<re::Ptr32<void>*>(col + 0x3C)) {
        return 0;
    }
    uint8_t* colData = *reinterpret_cast<re::Ptr32<uint8_t>*>(col + 0x38);
    if (!colData || tri >= *reinterpret_cast<int16_t*>(colData + 0x1C)) {
        return 0;
    }
    out->col = col;
    out->tri = tri;
    if (!NP_Point_IsOpen(out)) {
        return 0;
    }
    if ((flags & 0x10) && TriArea(*self) == 6 && !NP_IsDoorOpen(self, edge)) {
        return 0;
    }
    uint8_t outArea = TriArea(*out);
    if (outArea == 5 || outArea == 6) {
        int back = NP_Point_EdgeTo(out, self);
        if (!NP_CanPass(out, back, flags)) {
            return 0;
        }
    }
    if ((flags & 4) && TriArea(*self) == uint8_t(area) && TriArea(*out) != uint8_t(area)) {
        return 0;
    }
    return 1;
}

// 0x5E8110 (thiscall): round the triangles about a vertex, each step's edge from `fn` of
// the edge it came in by: 1 when back at this one, 0 when a neighbour fails
uint8_t __fastcall Walk(const NavPoint* self, re::Edx, int edge, EdgeFn fn, NavPoint* out, int* edgeOut, int flags,
                        int area) {
    NavPoint previous = *self;
    *edgeOut = fn(edge);
    uint8_t ok = NP_Neighbour(&previous, out, *edgeOut, flags, area);
    for (;;) {
        if (!ok) {
            return 0;
        }
        if (NP_Point_Equals(out, self)) {
            return 1;
        }
        int id = NP_NavNeighbourId(out->col, previous.col, previous.tri);
        uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(out->col + 0x38);
        const int32_t* neighbours = reinterpret_cast<const int32_t*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(data + 0x18)
                                                                     + out->tri * 0xC);
        int back = 0;
        while (back < 3 && neighbours[back] != id) {
            back++;
        }
        if (back == 3) {
            back = -1;
        }
        previous = *out;
        *edgeOut = fn(back);
        ok = NP_Neighbour(&previous, out, *edgeOut, flags, area);
    }
}

// 0x5E8220 (thiscall): the walk round the vertex fails somewhere (the last edge goes to
// the area argument's stack slot, as in the game)
uint8_t __fastcall IsFanBlocked(const NavPoint* self, re::Edx, int edge, EdgeFn fn, int flags, int area) {
    NavPoint out = {nullptr, -1, {0.0f, 0.0f, 0.0f}};
    int areaArg = area;
    return NP_Walk(self, edge, fn, &out, &areaArg, flags, area) == 0 ? 1 : 0;
}

// 0x5E8280 (thiscall): is there room for the ped's radius at p (0 radius: yes): near an
// edge (not the one it came by), the fans round its ends within reach must be whole and
// the neighbour across (when p is on the far side of the edge's line, or when there's no
// edge it came by) has room too
uint8_t __fastcall CheckClearance(const NavPoint* self, re::Edx, const NpVec* p, int fromEdge, int flags, int area) {
    float r = std::fabs(float(np_pedRadius - 0.0));
    if (r < 1.0e-5f || r == 1.0e-5f) {
        return 1;
    }
    NpVec verts[3] = {};
    NP_ColGetTriangle(self->col, self->tri, verts);
    NpVec at = *p;
    int prevEdge = fromEdge == -1 ? -1 : (fromEdge == 0 ? 2 : fromEdge - 1);
    for (int i = 0; i < 3; i++) {
        if (i == fromEdge) {
            continue;
        }
        int next = i < 2 ? i + 1 : 0;
        NpSegment edge = {{verts[i].x, verts[i].y, verts[i].z, 0.0f},
                          {verts[next].x, verts[next].y, verts[next].z, 0.0f}};
        float d = NP_Seg_FlatDistance(&edge, &at.x);
        if (!(np_pedRadius > d)) {
            continue;
        }
        float radiusSq = np_pedRadius * np_pedRadius;
        float dSq = d * d;
        float half = SqrtAbs(double(radiusSq) - dSq);
        float reachF = float(half / double(NP_Seg_Length(&edge)));
        float t = 0.0f;
        NpVec closest;
        NP_Seg_Closest(&edge, &closest, &at, &t);
        bool fanA = false;
        bool fanB = false;
        if ((prevEdge == -1 || prevEdge == i) && reachF > t) {
            NavPoint out = {nullptr, -1, {0.0f, 0.0f, 0.0f}};
            int last;
            if (!NP_Walk(self, i, np_prevEdge, &out, &last, flags, area)) {
                return 0;
            }
            fanA = true;
        }
        if ((prevEdge == -1 || prevEdge == next) && double(reachF) > 1.0 - double(t)) {
            NavPoint out = {nullptr, -1, {0.0f, 0.0f, 0.0f}};
            int last;
            if (!NP_Walk(self, i, np_nextEdge, &out, &last, flags, area)) {
                return 0;
            }
            fanB = true;
        }
        bool across;
        if (fromEdge == -1) {
            across = true;
        } else if (fanA && fanB) {
            across = false;
        } else {
            NpVec n;
            NpVec normal = *NP_Seg_Normal2D(&edge, &n);
            NpVec away = {-normal.x, -normal.y, -normal.z};
            float dx = edge.start[0] - at.x;
            float dy = edge.start[1] - at.y;
            float dot = float(double(dy) * away.y + double(dx) * away.x + away.z * 0.0);
            across = 0.0f < dot;
        }
        if (across) {
            NavPoint other = {nullptr, -1, {0.0f, 0.0f, 0.0f}};
            if (!NP_Neighbour(self, &other, i, flags, area)) {
                return 0;
            }
            int back = NP_Point_EdgeTo(&other, self);
            if (!NP_CheckClearance(&other, p, back, flags, area)) {
                return 0;
            }
        }
    }
    return 1;
}

// 0x5E8730 (thiscall): the edge (not `except`) the path leaves the triangle by: where,
// and whether the fan round the nearer end is blocked within the ped's radius (then -1)
int __fastcall FindExit(const NavPoint* self, re::Edx, float* path, int except, NpVec* crossOut, uint8_t* blocked,
                        int flags, int area) {
    if (!self->col || !*reinterpret_cast<re::Ptr32<void>*>(self->col + 0x3C)) {
        return -1;
    }
    NpVec verts[3] = {};
    NP_ColGetTriangle(self->col, self->tri, verts);
    const float* line = path;
    for (int i = 0; i < 3; i++) {
        if (i == except) {
            continue;
        }
        int next = i < 2 ? i + 1 : 0;
        NpSegment edge;
        NP_Seg_Construct(&edge, verts[i].x, verts[i].y, verts[i].z, verts[next].x, verts[next].y, verts[next].z);
        float t;
        float point[4];
        if (!NP_Seg_Intersect2D(&edge, line, &t, point)) {
            continue;
        }
        crossOut->x = point[0];
        crossOut->y = point[1];
        crossOut->z = point[2];
        EdgeFn fn = np_prevEdge;
        if (t > 0.5f) {
            fn = np_nextEdge;
            t = 1.0f - t;
        }
        float dist = NP_Seg_Length(&edge) * t;
        if (np_pedRadius > dist && NP_IsFanBlocked(self, i, fn, flags, area)) {
            *blocked = 1;
            return -1;
        }
        *blocked = 0;
        return i;
    }
    *blocked = 0;
    return -1;
}

// 0x5E88B0 (thiscall, on an {entity, value} pair): the point's mesh owner and triangle
uint32_t* __fastcall EntityRefFromPoint(uint32_t* self, re::Edx, const NavPoint* point) {
    self[0] = point->col ? *reinterpret_cast<uint32_t*>(point->col + 0x3C) : 0;
    self[1] = uint32_t(point->tri);
    return self;
}

// 0x5E88E0 (thiscall, on the connections): drops one
void __fastcall Connections_Remove(uint8_t* self, re::Edx, uint32_t index) {
    uint32_t& count = *reinterpret_cast<uint32_t*>(self + 0xC0);
    for (; index < count - 1; index++) {
        std::memcpy(self + index * 0x10, self + index * 0x10 + 0x10, 0x10);
    }
    count--;
}

// 0x5E8940 (thiscall, on a box): inside it grown by margin?
uint8_t __fastcall BoxContainsMargin(const float* self, re::Edx, const float* p, float margin) {
    if (!(float(self[0] - margin) <= p[0])) {
        return 0;
    }
    if (!(float(self[1] - margin) <= p[1])) {
        return 0;
    }
    if (!(float(self[2] - margin) <= p[2])) {
        return 0;
    }
    if (!(float(self[4] + margin) >= p[0])) {
        return 0;
    }
    if (!(float(self[5] + margin) >= p[1])) {
        return 0;
    }
    if (!(float(self[6] + margin) >= p[2])) {
        return 0;
    }
    return 1;
}

// 0x5E8AB0 (thiscall): the service's lists emptied
CWalkableMeshQueryService* __fastcall Service_Reset(CWalkableMeshQueryService* self) {
    self->modelEdgeCount = 0;
    self->cacheCount = 0;
    self->navEntityCount = 0;
    self->connectionCount = 0;
    for (int i = 0; i < 24; i++) {
        self->meshLinks[i] = {0.0f, 0.0f, 0.0f};
    }
    self->meshLinkCount = 0;
    self->unknown5D8 = 0;
    self->unknown5DC = -1;
    return self;
}

// 0x5E8B10 (thiscall): the mesh links from Config\MeshLink.dat ("x y z" lines, # for
// comments; no check on the 24)
void __fastcall Service_LoadMeshLinks(CWalkableMeshQueryService* self) {
    void* file = NP_FileOpen(np_meshLinkFile, np_readMode, 1);
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    char line[0x100];
    while (NP_FileReadLine(file, line, 0x100)) {
        if (line[0] == '#' || std::strlen(line) == 0) {
            continue;
        }
        NP_sscanf(line, np_threeFloats, &x, &y, &z);
        self->meshLinks[self->meshLinkCount] = {x, y, z};
        self->meshLinkCount++;
    }
    NP_FileClose(file);
}

// 0x5E8BE0: is p at a mesh link (within 0.1 each way)?
uint8_t __cdecl IsMeshLink(const float* p) {
    for (uint32_t i = 0; i < np_service->meshLinkCount; i++) {
        const NpVec& link = np_service->meshLinks[i];
        if (!(std::fabs(link.x - p[0]) < double(0.1f))) {
            continue;
        }
        if (!(std::fabs(link.y - p[1]) < double(0.1f))) {
            continue;
        }
        if (std::fabs(link.z - p[2]) < double(0.1f)) {
            return 1;
        }
    }
    return 0;
}

// 0x5E79C0 (cdecl): a triangle's edge before (0 -> 2)
int __cdecl PrevEdge(int edge) {
    return edge > 0 ? edge - 1 : 2;
}

// 0x5E79E0 (cdecl): a triangle's edge after (2 -> 0)
int __cdecl NextEdge(int edge) {
    return edge < 2 ? edge + 1 : 0;
}

// 0x5E8F60: the service's initialisation (its mesh links)
void __cdecl Service_Initialize() {
    NP_Service_LoadMeshLinks(np_service);
}

#define NAVPOINT_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(NavPointOriginal_, __LINE__);                                        \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(NavPointOriginal_, __LINE__), \
             re::Mode::Original)

NAVPOINT_REVERSED("NavPoint::PrevEdge", 0x005E79C0, &PrevEdge);
NAVPOINT_REVERSED("NavPoint::NextEdge", 0x005E79E0, &NextEdge);
NAVPOINT_REVERSED("NavPoint::IsDoorOpen", 0x005E7B90, &IsDoorOpen);
NAVPOINT_REVERSED("NavPoint::CanPass", 0x005E7C40, &CanPass);
NAVPOINT_REVERSED("NavPoint::DoorLine", 0x005E7CA0, &DoorLine);
NAVPOINT_REVERSED("NavNode::SetCosts", 0x005E7E40, &SetCosts);
NAVPOINT_REVERSED("NavPoint::Neighbour", 0x005E7FF0, &Neighbour);
NAVPOINT_REVERSED("NavPoint::Walk", 0x005E8110, &Walk);
NAVPOINT_REVERSED("NavPoint::IsFanBlocked", 0x005E8220, &IsFanBlocked);
NAVPOINT_REVERSED("NavPoint::CheckClearance", 0x005E8280, &CheckClearance);
NAVPOINT_REVERSED("NavPoint::FindExit", 0x005E8730, &FindExit);
NAVPOINT_REVERSED("NavEntityRef::FromPoint", 0x005E88B0, &EntityRefFromPoint);
NAVPOINT_REVERSED("WalkableMeshQueryService::RemoveConnection", 0x005E88E0, &Connections_Remove);
NAVPOINT_REVERSED("NavBox::ContainsMargin", 0x005E8940, &BoxContainsMargin);
NAVPOINT_REVERSED("WalkableMeshQueryService::Reset", 0x005E8AB0, &Service_Reset);
NAVPOINT_REVERSED("WalkableMeshQueryService::LoadMeshLinks", 0x005E8B10, &Service_LoadMeshLinks);
NAVPOINT_REVERSED("WalkableMeshQueryService::IsMeshLink", 0x005E8BE0, &IsMeshLink);
NAVPOINT_REVERSED("WalkableMeshQueryService::Initialize", 0x005E8F60, &Service_Initialize);

} // namespace
