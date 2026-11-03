// The nav mesh route search (A* over the meshes' triangles), one static finder at
// 0xD09660: its own route, the route being filled, the start and goal, up to 1000
// nodes and an open list of 200. 0x5E1150..0x5E2940. (See PedNavRoute.cpp for the
// mesh, NavPoint, NavNode and CPedRoute.)

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace {

struct CVector {
    float x, y, z;
};
struct NavPoint {
    re::Ptr32<uint8_t> col;  // +0x0
    int32_t tri;   // +0x4
    CVector pos;   // +0x8
};
static_assert(sizeof(NavPoint) == 0x14);
struct NavNode {
    NavPoint point;     // +0x0
    float cost;         // +0x14
    float estimate;     // +0x18
    re::Ptr32<NavNode> parent;    // +0x1C
    int16_t fromEdge;   // +0x20 (the edge it was reached by, -1)
    uint8_t depth;      // +0x22
    uint8_t pad23;
};
static_assert(sizeof(NavNode) == 0x24);
struct CPedRoute {
    uint8_t unknown0[0x24];
    int32_t status;             // +0x24
    uint8_t unknown28[0x42C - 0x28];
    int32_t count;              // +0x42C
    uint32_t unknown430;
};
static_assert(offsetof(CPedRoute, count) == 0x42C);
static_assert(sizeof(CPedRoute) == 0x434);

// A {entity, value} pair the start / goal lookups fill (0x5EB020's third argument)
struct NavEntityRef {
    re::Ptr32<uint8_t> entity;
    int32_t value;
};

struct NavFinder {
    CPedRoute route;       // +0x0 (when the caller gives none)
    re::Ptr32<CPedRoute> result;     // +0x434
    NavNode start;         // +0x438
    NavNode goal;          // +0x45C (its parent, +0x478, set when the search reaches it)
    re::Ptr32<NavPoint> door;        // +0x480 (0x5E9C30's out point)
    CVector startPos;      // +0x484
    CVector goalPos;       // +0x490
    int32_t maxDepth;      // +0x49C (-1: 50; reset each search)
    uint8_t flags;         // +0x4A0
    uint8_t area;          // +0x4A1 (0: any)
    uint8_t unknown4A2;    // +0x4A2
    uint8_t pad4A3[5];
    NavNode nodes[1000];   // +0x4A8
    re::Ptr32<NavNode> open[200];    // +0x9148
    int32_t openCount;     // +0x9468
};
static_assert(offsetof(NavFinder, start) == 0x438);
static_assert(offsetof(NavFinder, door) == 0x480);
static_assert(offsetof(NavFinder, flags) == 0x4A0);
static_assert(offsetof(NavFinder, nodes) == 0x4A8);
static_assert(offsetof(NavFinder, open) == 0x9148);
static_assert(offsetof(NavFinder, openCount) == 0x9468);

GAME_VAR(NavFinder, ns_finder, 0x00D09660);
GAME_VAR(int32_t, ns_nodeCount, 0x00D09658);
GAME_ARRAY(NavNode, ns_nodes, 0x00D09B08);     // ns_finder.nodes
GAME_ARRAY(NavNode*, ns_open, 0x00D127A8);     // ns_finder.open
GAME_VAR(int32_t, ns_openCount, 0x00D12AC8);   // ns_finder.openCount
GAME_VAR(NavEntityRef, ns_noEntity, 0x00D13728);

GAME_FN(CVector*, __cdecl, NS_VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, NS_Normalise, 0x00414E10, CVector* v);
// (0x0045D500 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* NS_TransformPoint(CVector* out, const void* matrix, const CVector* v) { return re::CallVec16<0x0045D500, CVector*>(out, matrix, v); }
GAME_FN(void*, __thiscall, NS_GetMatrix, 0x0046DD20, void* placeable);
GAME_FN(void, __cdecl, NS_MatrixInvert, 0x00413770, const void* matrix, void* out);
GAME_FN(uint8_t, __cdecl, NS_FindNavPoint, 0x005EB020, const CVector* pos, NavPoint* out, NavEntityRef* entity,
        uint8_t* offMesh);
GAME_FN(uint8_t, __cdecl, NS_WalkStraight, 0x005E9C30, NavPoint* from, NavPoint* to, int flags, int area, int check,
        NavPoint* door);
GAME_FN(uint8_t, __cdecl, NS_ModelsConnected, 0x005E90C0, int modelA, int modelB);
GAME_FN(float, __cdecl, NS_ClosestOnTriangle, 0x005E94A0, const CVector* p, const void* vertices, const void* tri,
        CVector* out);
GAME_FN(void, __thiscall, NS_EntityRefFromPoint, 0x005E88B0, NavEntityRef* self, const NavPoint* point);
GAME_FN(uint8_t, __thiscall, NS_Neighbour, 0x005E7FF0, const NavNode* self, NavNode* out, int edge, int flags, int area);
GAME_FN(uint8_t, __thiscall, NS_SetCosts, 0x005E7E40, NavNode* self, NavNode* parent, int edge, float x, float y,
        float z);
GAME_FN(void, __thiscall, NS_Node_Reset, 0x005E7B60, NavNode* self);
GAME_FN(void, __thiscall, NS_Node_FromPoint, 0x005E7980, NavNode* self, const NavPoint* point);
GAME_FN(uint8_t, __thiscall, NS_Point_IsValid, 0x005E7AF0, const NavPoint* self);
GAME_FN(float, __thiscall, NS_Point_DistToEdges, 0x005E79F0, const NavPoint* self, const CVector* p);
GAME_FN(void, __thiscall, NS_Point_Set, 0x005E7970, NavPoint* self, uint8_t* col, int32_t tri);
GAME_FN(void, __thiscall, NS_Point_Copy, 0x005E0C70, NavPoint* self, const NavPoint* other);
GAME_FN(void, __thiscall, NS_Route_Clear, 0x005E0E60, CPedRoute* self);
GAME_FN(void, __thiscall, NS_Route_Add, 0x005E0D10, CPedRoute* self, const NavPoint* point);
GAME_FN(uint8_t, __thiscall, NS_Route_AddFront, 0x005E0D70, CPedRoute* self, const NavPoint* point);
GAME_FN(void, __thiscall, NS_Route_AddNodes, 0x005E0CB0, CPedRoute* self, const NavNode* node);
GAME_FN(uint8_t, __thiscall, NS_Route_IsNearEnd, 0x005E1080, CPedRoute* self, int, float x, float y, float z,
        float maxDist);
GAME_FN(void, __thiscall, NS_ClearNodes, 0x005E1150, NavFinder* self);
GAME_FN(uint8_t, __stdcall, NS_AddOpen, 0x005E12F0, const NavNode* node);
GAME_FN(int, __thiscall, NS_OpenCheapest, 0x005E1380, NavFinder* self);
GAME_FN(int, __thiscall, NS_OpenNearest, 0x005E14C0, NavFinder* self, const CVector* pos);
GAME_FN(int, __thiscall, NS_OpenFarthest, 0x005E15A0, NavFinder* self);
GAME_FN(int, __thiscall, NS_OpenMostAway, 0x005E16D0, NavFinder* self, const CVector* from);
GAME_FN(uint8_t, __thiscall, NS_Expand, 0x005E1820, NavFinder* self, NavNode* node);
GAME_FN(uint8_t, __thiscall, NS_FleeSearch, 0x005E18E0, NavFinder* self, const CVector* from, float giveUpDist,
        float wantDist);
GAME_FN(uint8_t, __thiscall, NS_SearchNearest, 0x005E1CC0, NavFinder* self);
GAME_FN(uint8_t, __thiscall, NS_FindArea, 0x005E1E40, NavFinder* self, NavPoint* out, const NavPoint* from);
GAME_FN(uint8_t, __thiscall, NS_Search, 0x005E2060, NavFinder* self, NavEntityRef* startEntity, int fromPoints);
GAME_FN(uint8_t, __cdecl, NS_SearchPoints, 0x005E2500, CPedRoute* route, const NavPoint* start, const NavPoint* goal,
        int flags, uint32_t maxDepth, int area, int b, NavPoint* door);
GAME_FN(uint8_t, __cdecl, NS_RouteBetween, 0x005E2620, const NavPoint* start, const NavPoint* goal, int flags,
        CPedRoute* route, float maxDist);
GAME_FN(float, __cdecl, NS_Distance, 0x005E28D0, const CVector* a, const CVector* b);

uint8_t& TriFlags(const NavPoint& p) {
    uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(p.col + 0x38);
    return (*reinterpret_cast<re::Ptr32<uint8_t>*>(data + 0x10))[p.tri * 8 + 7];
}
uint8_t* TriData(const NavPoint& p) {
    uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(p.col + 0x38);
    return *reinterpret_cast<re::Ptr32<uint8_t>*>(data + 0x10) + p.tri * 8;
}
bool SameTriangle(const NavPoint& a, const NavPoint& b) {
    return a.tri == b.tri && a.col == b.col;
}
// drops open[index] (the last one takes its place)
void RemoveOpen(int index) {
    ns_open[index] = ns_open[ns_openCount - 1];
    ns_openCount--;
}
float FlatLength(const CVector& d) {
    float lengthSq = std::fabs(float(double(d.x) * d.x + double(d.y) * d.y));
    return float(std::sqrt(double(lengthSq)));
}

// 0x5E1150 (thiscall, this unused): frees the nodes (their triangles open again)
void __fastcall ClearNodes(NavFinder*) {
    for (int i = ns_nodeCount - 1; i >= 0; i--) {
        NavNode& node = ns_nodes[i];
        uint8_t& flags = TriFlags(node.point);
        flags = uint8_t((flags & 0xFD) | 1);
        node.point.col = nullptr;
        node.point.tri = -1;
        node.cost = 0.0f;
        node.estimate = 0.0f;
        node.parent = nullptr;
        node.fromEdge = -1;
        node.depth = 1;
    }
    ns_nodeCount = 0;
}

// 0x5E12F0 (thiscall, this unused): a copy of the node into the open list (its
// triangle taken)
uint8_t __stdcall AddOpen(const NavNode* node) {
    if (ns_openCount == 200 || ns_nodeCount >= 1000) {
        return 0;
    }
    NavNode* stored = &ns_nodes[ns_nodeCount];
    ns_nodeCount++;
    *stored = *node;
    TriFlags(stored->point) &= 0xFE;
    TriFlags(stored->point) |= 2;
    ns_open[ns_openCount] = stored;
    ns_openCount++;
    return 1;
}

// 0x5E1380 (thiscall, this unused): the open node with the least cost + estimate (-1)
int __fastcall OpenCheapest(NavFinder*) {
    if (ns_openCount == 0) {
        return -1;
    }
    float best = ns_open[0]->estimate + ns_open[0]->cost;
    int index = 0;
    for (int i = ns_openCount - 1; i >= 0; i--) {
        float value = ns_open[i]->estimate + ns_open[i]->cost;
        if (best > value) {
            best = value;
            index = i;
        }
    }
    return index;
}

// 0x5E14C0 (thiscall, this unused): the open node nearest pos (flat; -1)
int __fastcall OpenNearest(NavFinder*, re::Edx, const CVector* pos) {
    if (ns_openCount == 0) {
        return -1;
    }
    CVector p = ns_open[0]->point.pos;
    CVector d;
    NS_VectorSub(&d, &p, pos);
    float best = float(double(d.x) * d.x + double(d.y) * d.y);
    int index = 0;
    for (int i = ns_openCount - 1; i >= 0; i--) {
        p = ns_open[i]->point.pos;
        NS_VectorSub(&d, &p, pos);
        float value = float(double(d.x) * d.x + double(d.y) * d.y);
        if (best > value) {
            best = value;
            index = i;
        }
    }
    return index;
}

// 0x5E15A0 (thiscall, this unused): the open node with the greatest estimate (-1)
int __fastcall OpenFarthest(NavFinder*) {
    if (ns_openCount == 0) {
        return -1;
    }
    float best = ns_open[0]->estimate;
    int index = 0;
    for (int i = ns_openCount - 1; i >= 0; i--) {
        if (best < ns_open[i]->estimate) {
            best = ns_open[i]->estimate;
            index = i;
        }
    }
    return index;
}

// 0x5E16D0 (thiscall): the open node most in the direction away from `from` (seen from
// the start; -1)
int __fastcall OpenMostAway(NavFinder* self, re::Edx, const CVector* from) {
    if (ns_openCount == 0) {
        return -1;
    }
    CVector start = self->start.point.pos;
    float best = -3.4028235e+38f;
    CVector away;
    NS_VectorSub(&away, &start, from);
    NS_Normalise(&away);
    int index = -1;
    for (int i = ns_openCount - 1; i >= 0; i--) {
        CVector p = ns_open[i]->point.pos;
        CVector d;
        NS_VectorSub(&d, &p, &start);
        NS_Normalise(&d);
        float dot = float(double(away.z) * d.z + double(d.x) * away.x + double(away.y) * d.y);
        if (best < dot) {
            best = dot;
            index = i;
        }
    }
    return index;
}

// 0x5E17D0 (cdecl): out = matrix * v
CVector* __cdecl TransformPointCopy(CVector* out, const void* matrix, const CVector* v) {
    CVector copy = *v;
    CVector result;
    NS_TransformPoint(&result, matrix, &copy);
    *out = result;
    return out;
}

// 0x5E1820 (thiscall): opens the node's neighbours (not the way it came; those passing
// the flags and area, with their costs, and not taken). 0 when the lists are full.
uint8_t __fastcall Expand(NavFinder* self, re::Edx, NavNode* node) {
    NavNode next;
    NS_Node_Reset(&next);
    for (int edge = 0; edge < 3; edge++) {
        if (edge == node->fromEdge) {
            continue;
        }
        if (!NS_Neighbour(node, &next, edge, self->flags, self->area)) {
            continue;
        }
        if (!NS_SetCosts(&next, node, edge, self->goal.point.pos.x, self->goal.point.pos.y, self->goal.point.pos.z)) {
            continue;
        }
        if (TriFlags(next.point) & 1) {
            if (!NS_AddOpen(&next)) {
                return 0;
            }
        }
    }
    return 1;
}

// 0x5E18E0 (thiscall): a route away from `from`: straight on when the mesh allows,
// else the search's node that gets farthest, up to wantDist (or giveUpDist once the
// farthest and the one with the greatest estimate differ)
uint8_t __fastcall FleeSearch(NavFinder* self, re::Edx, const CVector* from, float giveUpDist, float wantDist) {
    int depthLimit = self->maxDepth;
    uint8_t offMesh = 0;
    if (depthLimit == -1) {
        depthLimit = 50;
    }
    self->maxDepth = -1;
    if (!NS_FindNavPoint(&self->startPos, &self->start.point, nullptr, &offMesh)) {
        return 0;
    }
    self->goalPos = self->startPos;
    CVector dir;
    NS_VectorSub(&dir, &self->startPos, from);
    float dist = FlatLength(dir);
    NS_Normalise(&dir);
    NavEntityRef noEntity = {nullptr, -1};
    dir.x = wantDist * dir.x;
    dir.y = dir.y * wantDist;
    dir.z = wantDist * dir.z;
    self->goalPos.x = self->goalPos.x + dir.x;
    self->goalPos.y = dir.y + self->goalPos.y;
    self->goalPos.z = self->goalPos.z + dir.z;
    NavPoint point;
    if (NS_FindNavPoint(&self->goalPos, &self->goal.point, &noEntity, nullptr)
        && NS_WalkStraight(&self->goal.point, &self->start.point, self->flags, 0, 1, self->door)) {
        self->result->status = 2;
        NS_Point_Copy(&point, &self->goal.point);
        NS_Route_Add(self->result, &point);
        if (offMesh) {
            NS_Point_Copy(&point, &self->start.point);
            NS_Route_Add(self->result, &point);
        }
        return 1;
    }
    float bestEstimate = dist;
    float farthestSq = dist * dist;
    self->goal = self->start;
    double want = wantDist;
    ns_openCount = 0;
    ns_nodes[0] = self->start;
    ns_nodeCount++;
    NavNode* node = &ns_nodes[0];
    NavNode* best = nullptr;
    NavNode* farthest = nullptr;
    if (dist < want) {
        for (;;) {
            TriFlags(node->point) &= 0xFD;
            if (!NS_Expand(self, node) || ns_openCount == 0) {
                break;
            }
            int index = SameTriangle(self->start.point, node->point) ? NS_OpenMostAway(self, from) : NS_OpenFarthest(self);
            node = ns_open[index];
            if (node->depth >= depthLimit) {
                break;
            }
            CVector p = node->point.pos;
            CVector d;
            NS_VectorSub(&d, from, &p);
            float distSq = float(double(d.y) * d.y + double(d.x) * d.x + double(d.z) * d.z);
            if (farthestSq < distSq) {
                farthestSq = distSq;
                farthest = node;
            }
            if (bestEstimate < node->estimate) {
                bestEstimate = node->estimate;
                best = node;
            }
            if (best != farthest) {
                float giveUpSq = giveUpDist * giveUpDist;
                if (giveUpSq < farthestSq) {
                    best = farthest;
                    break;
                }
            }
            RemoveOpen(index);
            if (!(bestEstimate < want)) {
                break;
            }
        }
        if (best) {
            self->result->status = 2;
            self->goal = *best;
            NS_Route_AddNodes(self->result, &self->goal);
            if (offMesh) {
                NS_Point_Copy(&point, &self->start.point);
                NS_Route_Add(self->result, &point);
            }
            NS_ClearNodes(self);
            return 1;
        }
    }
    self->result->status = 0;
    NS_ClearNodes(self);
    return 0;
}

// 0x5E1CC0 (thiscall): the search from the start towards the goal: 2 when it reaches
// the goal's triangle within the depth, else 1 with the goal moved to the node nearest
// it (0 when that's the start). (When the lists fill up it keeps expanding the same node:
// the game hangs there.)
uint8_t __fastcall SearchNearest(NavFinder* self) {
    float bestDist = 1.0e6f;
    ns_openCount = 0;
    ns_nodes[0] = self->start;
    ns_nodeCount++;
    int depthLimit = self->maxDepth;
    NavNode* node = &ns_nodes[0];
    if (depthLimit == -1) {
        depthLimit = 50;
    }
    self->maxDepth = -1;
    NavNode* nearest = nullptr;
    for (;;) {
        float d = NS_Point_DistToEdges(&node->point, &self->goalPos);
        if (d < bestDist) {
            bestDist = d;
            nearest = node;
        }
        if (SameTriangle(self->goal.point, node->point)) {
            if (node->depth < depthLimit - 1) {
                self->goal.parent = node;
                self->result->status = 2;
                return 1;
            }
            break;
        }
        TriFlags(node->point) &= 0xFD;
        if (!NS_Expand(self, node)) {
            continue;
        }
        int count = ns_openCount;
        if (count == 0) {
            break;
        }
        int index = NS_OpenCheapest(self);
        node = ns_open[index];
        if (node->depth >= depthLimit) {
            break;
        }
        ns_open[index] = ns_open[count - 1];
        ns_openCount--;
    }
    if (nearest && !SameTriangle(self->start.point, nearest->point)) {
        self->goal = *nearest;
        self->result->status = 1;
        return 1;
    }
    self->result->status = 0;
    return 0;
}

// 0x5E1E40 (thiscall): the nearest triangle (by the search, at most 299 steps) in the
// finder's area, and the point on it nearest the start (from the mesh's own space)
uint8_t __fastcall FindArea(NavFinder* self, re::Edx, NavPoint* out, const NavPoint* from) {
    ns_openCount = 0;
    NS_Node_FromPoint(&ns_nodes[0], from);
    ns_nodeCount++;
    int depthLimit = self->maxDepth;
    NavNode* node = &ns_nodes[0];
    if (depthLimit == -1) {
        depthLimit = 50;
    }
    self->maxDepth = -1;
    CVector startPos = ns_nodes[0].point.pos;
    for (int steps = 299; steps != 0; steps--) {
        uint8_t* tri = TriData(node->point);
        if (tri[6] == self->area) {
            uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(node->point.col + 0x38);
            void* owner = *reinterpret_cast<re::Ptr32<void>*>(node->point.col + 0x3C);
            void* vertices = *reinterpret_cast<re::Ptr32<void>*>(data + 0xC);
            void* matrix = NS_GetMatrix(owner);
            alignas(16) uint8_t inverse[0x48];
            *reinterpret_cast<uint32_t*>(inverse + 0x40) = 0;
            NS_MatrixInvert(matrix, inverse);
            CVector world = startPos;
            CVector local;
            NS_TransformPoint(&local, inverse, &world);
            CVector closest = {0.0f, 0.0f, 0.0f};
            CVector localCopy = local;
            NS_ClosestOnTriangle(&localCopy, vertices, tri, &closest);
            CVector closestCopy = closest;
            CVector result;
            NS_TransformPoint(&result, matrix, &closestCopy);
            out->col = node->point.col;
            out->tri = node->point.tri;
            out->pos = result;
            NS_ClearNodes(self);
            return 1;
        }
        TriFlags(node->point) &= 0xFD;
        if (NS_Expand(self, node)) {
            if (ns_openCount == 0) {
                break;
            }
            int index = NS_OpenNearest(self, &startPos);
            node = ns_open[index];
            if (node->depth >= depthLimit) {
                break;
            }
            RemoveOpen(index);
        }
    }
    NS_ClearNodes(self);
    self->result->status = 0;
    return 0;
}

// 0x5E2060 (thiscall): the search between the finder's start and goal (their points
// looked up, or the given points' entities): straight when it can be walked, else A*.
// The route gets the points goal-first.
uint8_t __fastcall Search(NavFinder* self, re::Edx, NavEntityRef* startEntity, int fromPoints) {
    NS_Route_Clear(self->result);
    if (!startEntity) {
        ns_noEntity.entity = nullptr;
        ns_noEntity.value = -1;
        startEntity = &ns_noEntity;
    }
    NavEntityRef goalEntity = {nullptr, -1};
    uint8_t goalOffMesh = 0;
    uint8_t startOffMesh = 0;
    NavPoint point;
    if (!re::Flag(fromPoints)) {
        if (!NS_FindNavPoint(&self->goalPos, &self->goal.point, &goalEntity, &goalOffMesh)) {
            return 0;
        }
        uint8_t* tri = TriData(self->goal.point);
        if (!tri || ((~(tri[7] >> 2)) & 1) == 0) {
            return 0;
        }
        if (NS_Point_IsValid(&self->goal.point) && TriData(self->goal.point)[6] == 6) {
            self->flags |= 0x14;
            point.pos = {0.0f, 0.0f, 0.0f};
            point.col = nullptr;
            point.tri = -1;
            self->area = 0;
            uint8_t found = NS_FindArea(self, &point, &self->goal.point);
            self->flags &= 0xEB;
            if (!found) {
                return 0;
            }
            NS_Node_FromPoint(&self->goal, &point);
        }
        if (!NS_FindNavPoint(&self->startPos, &self->start.point, startEntity, &startOffMesh)) {
            return 0;
        }
    } else {
        NS_EntityRefFromPoint(&goalEntity, &self->goal.point);
        NS_EntityRefFromPoint(startEntity, &self->start.point);
        if (!goalEntity.entity || !startEntity->entity) {
            return 0;
        }
    }
    if (self->area != 0) {
        if (TriData(self->start.point)[6] != self->area) {
            point.pos = {0.0f, 0.0f, 0.0f};
            point.col = nullptr;
            point.tri = -1;
            if (!NS_FindArea(self, &point, &self->start.point)) {
                return 0;
            }
            NS_Node_FromPoint(&self->start, &point);
        }
        if (TriData(self->goal.point)[6] != self->area) {
            point.pos = {0.0f, 0.0f, 0.0f};
            point.col = nullptr;
            point.tri = -1;
            if (!NS_FindArea(self, &point, &self->goal.point)) {
                return 0;
            }
            NS_Node_FromPoint(&self->goal, &point);
            self->goalPos = self->goal.point.pos;
        }
    }
    int goalModel = *reinterpret_cast<int16_t*>(goalEntity.entity + 0x10E);
    int startModel = *reinterpret_cast<int16_t*>(startEntity->entity + 0x10E);
    if (!NS_ModelsConnected(goalModel, startModel)) {
        return 0;
    }
    if (goalOffMesh) {
        self->unknown4A2 = 0;
    }
    if (NS_WalkStraight(&self->start.point, &self->goal.point, self->flags, self->area, self->unknown4A2, self->door)) {
        self->result->status = 2;
        if (goalOffMesh) {
            point.pos = {0.0f, 0.0f, 0.0f};
            point.col = nullptr;
            point.tri = -1;
            NS_Point_Set(&point, nullptr, -1);
            point.pos = self->goalPos;
            NS_Route_Add(self->result, &point);
        }
        NS_Point_Copy(&point, &self->goal.point);
        NS_Route_Add(self->result, &point);
        if (startOffMesh) {
            NS_Point_Copy(&point, &self->start.point);
            NS_Route_Add(self->result, &point);
        }
        return 1;
    }
    if (NS_SearchNearest(self)) {
        NS_Route_AddNodes(self->result, &self->goal);
        NS_ClearNodes(self);
        if (startOffMesh) {
            NS_Point_Copy(&point, &self->start.point);
            NS_Route_Add(self->result, &point);
        }
        if (goalOffMesh) {
            point.pos = {0.0f, 0.0f, 0.0f};
            point.col = nullptr;
            point.tri = -1;
            NS_Point_Set(&point, nullptr, -1);
            point.pos = self->goalPos;
            NS_Route_AddFront(self->result, &point);
        }
        return 1;
    }
    NS_ClearNodes(self);
    return 0;
}

// 0x5E2460 (cdecl): a route between two positions
uint8_t __cdecl SearchPositions(CPedRoute* route, const CVector* start, const CVector* goal, int flags,
                                NavEntityRef* startEntity, uint32_t maxDepth, int area, int b, NavPoint* door) {
    NavFinder& f = ns_finder;
    f.result = route;
    f.startPos = *start;
    f.goalPos = *goal;
    f.maxDepth = -1;
    if (maxDepth <= 50) {
        f.maxDepth = int32_t(maxDepth);
    }
    f.area = uint8_t(area);
    f.unknown4A2 = uint8_t(b);
    f.flags = uint8_t(flags);
    f.door = door;
    return NS_Search(&f, startEntity, 0);
}

// 0x5E2500 (cdecl): a route between two nav points
uint8_t __cdecl SearchPoints(CPedRoute* route, const NavPoint* start, const NavPoint* goal, int flags,
                             uint32_t maxDepth, int area, int b, NavPoint* door) {
    NavFinder& f = ns_finder;
    f.result = route;
    f.startPos = start->pos;
    f.goalPos = goal->pos;
    NS_Node_FromPoint(&f.start, start);
    NS_Node_FromPoint(&f.goal, goal);
    f.maxDepth = -1;
    if (maxDepth <= 50) {
        f.maxDepth = int32_t(maxDepth);
    }
    f.area = uint8_t(area);
    f.unknown4A2 = uint8_t(b);
    f.door = door;
    f.flags = uint8_t(flags);
    return NS_Search(&f, nullptr, 1);
}

// 0x5E25B0 (cdecl): a route from start away from `from`
uint8_t __cdecl FleePositions(CPedRoute* route, const CVector* start, const CVector* from, float giveUpDist,
                              float wantDist, int flags, NavPoint* door) {
    NS_Route_Clear(route);
    NavFinder& f = ns_finder;
    f.result = route;
    f.startPos = *start;
    f.door = door;
    f.flags = uint8_t(flags);
    return NS_FleeSearch(&f, from, giveUpDist, wantDist);
}

// 0x5E2620 (cdecl): a route between two nav points into `route` (the finder's own when
// none), unless it already ends within maxDist of the goal: 1 when found whole
uint8_t __cdecl RouteBetween(const NavPoint* start, const NavPoint* goal, int flags, CPedRoute* route, float maxDist) {
    if (!route) {
        route = &ns_finder.route;
        NS_Route_Clear(route);
    }
    if (route->count != 0 && NS_Route_IsNearEnd(route, 0, goal->pos.x, goal->pos.y, goal->pos.z, maxDist)) {
        return 1;
    }
    if (NS_SearchPoints(route, start, goal, flags, 50, 0, 1, nullptr) && route->status == 2) {
        return 1;
    }
    return 0;
}

// 0x5E26A0 (cdecl): the same between two positions (their nav points looked up)
uint8_t __cdecl RouteBetweenPositions(const CVector* start, const CVector* goal, int flags, CPedRoute* route,
                                      float maxDist) {
    NavPoint goalPoint = {nullptr, -1, {0.0f, 0.0f, 0.0f}};
    NavPoint startPoint = {nullptr, -1, {0.0f, 0.0f, 0.0f}};
    uint8_t offMesh = 1;
    if (!NS_FindNavPoint(goal, &goalPoint, nullptr, &offMesh)) {
        return 0;
    }
    if (!NS_FindNavPoint(start, &startPoint, nullptr, &offMesh)) {
        return 0;
    }
    return NS_RouteBetween(&startPoint, &goalPoint, flags, route, maxDist);
}

// 0x5E28D0 (cdecl): the distance between two points
float __cdecl Distance(const CVector* a, const CVector* b) {
    float x = a->x - b->x;
    float y = a->y - b->y;
    float z = a->z - b->z;
    float lengthSq = float(double(x) * x + double(y) * y + double(z) * z);
    lengthSq = std::fabs(lengthSq);
    return float(std::sqrt(double(lengthSq)));
}

// 0x5E2940 (cdecl): the same with b copied first
float __cdecl DistanceCopy(const CVector* a, const CVector* b) {
    CVector copy = *b;
    return NS_Distance(a, &copy);
}

#define NAVSEARCH_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(NavSearchOriginal_, __LINE__);                                        \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(NavSearchOriginal_, __LINE__), \
             re::Mode::Original)

NAVSEARCH_REVERSED("CNavFinder::ClearNodes", 0x005E1150, &ClearNodes);
NAVSEARCH_REVERSED("CNavFinder::AddOpen", 0x005E12F0, &AddOpen);
NAVSEARCH_REVERSED("CNavFinder::OpenCheapest", 0x005E1380, &OpenCheapest);
NAVSEARCH_REVERSED("CNavFinder::OpenNearest", 0x005E14C0, &OpenNearest);
NAVSEARCH_REVERSED("CNavFinder::OpenFarthest", 0x005E15A0, &OpenFarthest);
NAVSEARCH_REVERSED("CNavFinder::OpenMostAway", 0x005E16D0, &OpenMostAway);
NAVSEARCH_REVERSED("CNavFinder::TransformPointCopy", 0x005E17D0, &TransformPointCopy);
NAVSEARCH_REVERSED("CNavFinder::Expand", 0x005E1820, &Expand);
NAVSEARCH_REVERSED("CNavFinder::FleeSearch", 0x005E18E0, &FleeSearch);
NAVSEARCH_REVERSED("CNavFinder::SearchNearest", 0x005E1CC0, &SearchNearest);
NAVSEARCH_REVERSED("CNavFinder::FindArea", 0x005E1E40, &FindArea);
NAVSEARCH_REVERSED("CNavFinder::Search", 0x005E2060, &Search);
NAVSEARCH_REVERSED("CNavFinder::SearchPositions", 0x005E2460, &SearchPositions);
NAVSEARCH_REVERSED("CNavFinder::SearchPoints", 0x005E2500, &SearchPoints);
NAVSEARCH_REVERSED("CNavFinder::FleePositions", 0x005E25B0, &FleePositions);
NAVSEARCH_REVERSED("CNavFinder::RouteBetween", 0x005E2620, &RouteBetween);
NAVSEARCH_REVERSED("CNavFinder::RouteBetweenPositions", 0x005E26A0, &RouteBetweenPositions);
NAVSEARCH_REVERSED("NavMath::Distance", 0x005E28D0, &Distance);
NAVSEARCH_REVERSED("NavMath::DistanceCopy", 0x005E2940, &DistanceCopy);

} // namespace
