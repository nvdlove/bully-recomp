// Peds' navigation over the nav meshes (collision models whose triangles peds walk on;
// the model's +0x38 collision data: +0xC vertices (short x, y, z in 1/128 units), +0x10
// triangles (8 bytes: three vertex indices, +6 an area byte, +7 flags: 1 open for the
// search, 2 in the search's lists, 4 blocked), +0x18 each triangle's three neighbours).
//
// Here: a point on a mesh (NavPoint: the model, the triangle, a position), the search's
// node (NavNode: a point, its costs, its parent) and a ped's route (CPedRoute: up to 50
// points to walk through, +0x44). 0x5E0B90..0x5E1120 and 0x5E7970..0x5E7B60.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace pednav {

struct CVector {
    float x, y, z;
};

// A point on a nav mesh
struct NavPoint {
    re::Ptr32<uint8_t> col;    // +0x0 the nav mesh's collision model (null: none)
    int32_t tri;     // +0x4 its triangle (-1: none)
    CVector pos;     // +0x8
};
static_assert(sizeof(NavPoint) == 0x14);

// A node of the route search (A*)
struct NavNode {
    NavPoint point;     // +0x0
    float cost;         // +0x14 (from the start)
    float estimate;     // +0x18 (to the goal)
    re::Ptr32<NavNode> parent;    // +0x1C
    int16_t unknown20;  // +0x20 (-1)
    uint8_t depth;      // +0x22 (steps from the start)
    uint8_t pad23;
};
static_assert(offsetof(NavNode, parent) == 0x1C);
static_assert(sizeof(NavNode) == 0x24);

// The route's points (50, the count after them)
struct NavPointList {
    NavPoint points[50];  // +0x0
    uint32_t count;       // +0x3E8
};
static_assert(sizeof(NavPointList) == 0x3EC);

// A ped's route
struct CPedRoute {
    NavNode node;           // +0x0
    int32_t status;         // +0x24 (0 none, 1 partial, 2 found; 1 too when points were dropped)
    uint32_t unknown28;     // +0x28
    CVector start;          // +0x2C
    CVector end;            // +0x38
    NavPointList list;      // +0x44 (count at +0x42C)
    uint32_t unknown430;    // +0x430
};
static_assert(offsetof(CPedRoute, list) == 0x44);
static_assert(offsetof(CPedRoute, list.count) == 0x42C);
static_assert(sizeof(CPedRoute) == 0x434);

// the triangle's 8 bytes
inline uint8_t* TriData(const uint8_t* col, int32_t tri) {
    uint8_t* data = *reinterpret_cast<const re::Ptr32<uint8_t>*>(col + 0x38);
    return *reinterpret_cast<re::Ptr32<uint8_t>*>(data + 0x10) + tri * 8;
}

} // namespace pednav

namespace {

using namespace pednav;

GAME_VAR(uint8_t, nav_skipNextNearEnd, 0x00D09654);
GAME_FN(CVector*, __cdecl, NR_VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(void, __thiscall, NR_ColGetTriangle, 0x0056E9C0, void* col, int tri, CVector* verts);
GAME_FN(float, __cdecl, NR_PointSegmentDist, 0x00564D50, const CVector* a, const CVector* b, const CVector* p);
GAME_FN(int, __cdecl, NR_NavNeighbourId, 0x005706F0, void* col, void* otherCol, int tri);
GAME_FN(void, __thiscall, NR_List_Remove, 0x005E0B90, NavPointList* self, uint32_t index);
GAME_FN(void, __thiscall, NR_List_Insert, 0x005E0BF0, NavPointList* self, const NavPoint* point, uint32_t index);
GAME_FN(void, __thiscall, NR_Route_Add, 0x005E0D10, CPedRoute* self, const NavPoint* point);
GAME_FN(void, __thiscall, NR_List_Init, 0x005E0EC0, NavPointList* self);
GAME_FN(void, __thiscall, NR_Node_Reset, 0x005E7B60, NavNode* self);

// 0x5E0B90 (thiscall): removes the point at index
void __fastcall List_Remove(NavPointList* self, re::Edx, uint32_t index) {
    for (; index < self->count - 1; index++) {
        self->points[index] = self->points[index + 1];
    }
    self->count--;
}

// 0x5E0BF0 (thiscall): inserts a point at index
void __fastcall List_Insert(NavPointList* self, re::Edx, const NavPoint* point, uint32_t index) {
    self->count++;
    for (uint32_t i = self->count - 1; i > index; i--) {
        self->points[i] = self->points[i - 1];
    }
    self->points[index] = *point;
}

// 0x5E0C70 (thiscall): the copy constructor
NavPoint* __fastcall NavPoint_Copy(NavPoint* self, re::Edx, const NavPoint* other) {
    self->col = nullptr;
    self->tri = -1;
    self->pos = {0.0f, 0.0f, 0.0f};
    *self = *other;
    return self;
}

// 0x5E0CB0 (thiscall): adds a search's result, the node then its parents (no check on
// the 50)
void __fastcall Route_AddNodes(CPedRoute* self, re::Edx, const NavNode* node) {
    do {
        self->list.points[self->list.count] = node->point;
        self->list.count++;
        node = node->parent;
    } while (node);
}

// 0x5E0D10 (thiscall): adds a point at the end (the oldest is dropped when full)
void __fastcall Route_Add(CPedRoute* self, re::Edx, const NavPoint* point) {
    if (self->list.count == 50) {
        NR_List_Remove(&self->list, 0);
        self->status = 1;
    }
    self->list.points[self->list.count] = *point;
    self->list.count++;
}

// 0x5E0D70 (thiscall): adds a point at the front (not when full)
uint8_t __fastcall Route_AddFront(CPedRoute* self, re::Edx, const NavPoint* point) {
    if (self->list.count == 50) {
        return 0;
    }
    NR_List_Insert(&self->list, point, 0);
    return 1;
}

// 0x5E0DA0 (thiscall): the last point
NavPoint* __fastcall Route_Last(CPedRoute* self) {
    int count = int(self->list.count);
    return count > 0 ? &self->list.points[count - 1] : nullptr;
}

// 0x5E0DC0 (thiscall): the point before the last
NavPoint* __fastcall Route_BeforeLast(CPedRoute* self) {
    int count = int(self->list.count);
    return count > 1 ? &self->list.points[count - 2] : nullptr;
}

// 0x5E0DE0 (thiscall): the first point
NavPoint* __fastcall Route_First(CPedRoute* self) {
    return self->list.count != 0 ? &self->list.points[0] : nullptr;
}

// 0x5E0DF0 (thiscall): drops the first point
void __fastcall Route_RemoveFirst(CPedRoute* self) {
    if (self->list.count != 0) {
        NR_List_Remove(&self->list, 0);
    }
}

// 0x5E0E10 (thiscall): the second point
NavPoint* __fastcall Route_Second(CPedRoute* self) {
    return int(self->list.count) > 1 ? &self->list.points[1] : nullptr;
}

// 0x5E0E20 (thiscall): the point at index (not the first)
NavPoint* __fastcall Route_Point(CPedRoute* self, re::Edx, int index) {
    if (index > 0 && index < int(self->list.count)) {
        return &self->list.points[index];
    }
    return nullptr;
}

// 0x5E0E40 (thiscall): drops the last point
void __fastcall Route_RemoveLast(CPedRoute* self) {
    if (self->list.count != 0) {
        self->list.count--;
    }
}

// 0x5E0E60 (thiscall): empties the route (start and end 1e11)
void __fastcall Route_Clear(CPedRoute* self) {
    self->list.count = 0;
    self->end.x = 1.0e11f;
    self->end.y = 1.0e11f;
    self->start.x = 1.0e11f;
    self->end.z = 1.0e11f;
    self->start.y = 1.0e11f;
    self->start.z = 1.0e11f;
}

// 0x5E0EC0 (thiscall): the points' constructors
void __fastcall List_Init(NavPointList* self) {
    for (int i = 0; i < 50; i++) {
        self->points[i].col = nullptr;
        self->points[i].tri = -1;
        self->points[i].pos = {0.0f, 0.0f, 0.0f};
    }
    self->count = 0;
}

// 0x5E0F00 (thiscall): the constructor
CPedRoute* __fastcall Route_Construct(CPedRoute* self) {
    NR_Node_Reset(&self->node);
    self->start = {0.0f, 0.0f, 0.0f};
    self->end = {0.0f, 0.0f, 0.0f};
    NR_List_Init(&self->list);
    self->list.count = 0;
    self->end = {1.0e11f, 1.0e11f, 1.0e11f};
    self->start = {1.0e11f, 1.0e11f, 1.0e11f};
    return self;
}

// 0x5E0F80 (thiscall): appends another route (its first point only when it isn't where
// this one ends; this one's last is read even when it's empty, from null)
void __fastcall Route_Append(CPedRoute* self, re::Edx, const CPedRoute* other) {
    int otherCount = int(other->list.count);
    const NavPoint* last = int(self->list.count) < 1 ? nullptr : &self->list.points[self->list.count - 1];
    const volatile float* lastPos = reinterpret_cast<const volatile float*>(reinterpret_cast<uintptr_t>(last) + 8);
    const NavPoint* first = &other->list.points[0];
    if (lastPos[0] != first->pos.x || lastPos[1] != first->pos.y || lastPos[2] != first->pos.z) {
        NR_Route_Add(self, first);
    }
    for (int i = 1; i < otherCount; i++) {
        if (self->list.count == 50) {
            NR_List_Remove(&self->list, 0);
            self->status = 1;
        }
        self->list.points[self->list.count] = other->list.points[i];
        self->list.count++;
    }
}

// 0x5E1080 (thiscall): is (x, y) within maxDist of the route's end (flat)? Not when the
// route is empty, nor once after nav_skipNextNearEnd was set.
uint8_t __fastcall Route_IsNearEnd(CPedRoute* self, re::Edx, int, float x, float y, float z, float maxDist) {
    if (nav_skipNextNearEnd) {
        nav_skipNextNearEnd = 0;
        return 0;
    }
    if (self->list.count == 0) {
        return 0;
    }
    CVector p = {x, y, z};
    CVector d;
    NR_VectorSub(&d, &p, &self->end);
    float lengthSq = float(double(d.x) * d.x + double(d.y) * d.y);
    lengthSq = std::fabs(lengthSq);
    float dist = float(std::sqrt(double(lengthSq)));
    return !(maxDist < dist);
}

// 0x5E1100 (thiscall): the same triangle?
uint8_t __fastcall NavPoint_Equals(const NavPoint* self, re::Edx, const NavPoint* other) {
    return other->tri == self->tri && other->col == self->col;
}

// 0x5E1120 (thiscall): the triangle isn't blocked
uint8_t __fastcall NavPoint_IsOpen(const NavPoint* self) {
    uint8_t* tri = TriData(self->col, self->tri);
    return tri && ((~(tri[7] >> 2)) & 1) != 0;
}

// 0x5E7970 (thiscall): sets the model and triangle
void __fastcall NavPoint_Set(NavPoint* self, re::Edx, uint8_t* col, int32_t tri) {
    self->col = col;
    self->tri = tri;
}

// 0x5E7980 (thiscall): a search node for a point
NavNode* __fastcall Node_FromPoint(NavNode* self, re::Edx, const NavPoint* point) {
    self->estimate = 0.0f;
    self->parent = nullptr;
    self->cost = 0.0f;
    self->depth = 0;
    self->unknown20 = -1;
    self->point = *point;
    return self;
}

// 0x5E79F0 (thiscall): the distance from p to the nearest of the triangle's edges
// (1e12 without the model's owner)
float __fastcall NavPoint_DistToEdges(const NavPoint* self, re::Edx, const CVector* p) {
    float best = 1.0e12f;
    CVector verts[3] = {};
    if (*reinterpret_cast<const re::Ptr32<void>*>(self->col + 0x3C) == nullptr) {
        return best;
    }
    NR_ColGetTriangle(self->col, self->tri, verts);
    float d = NR_PointSegmentDist(&verts[0], &verts[1], p);
    if (!(d > 1.0e12)) {
        best = d;
    }
    d = NR_PointSegmentDist(&verts[1], &verts[2], p);
    if (!(d > best)) {
        best = d;
    }
    d = NR_PointSegmentDist(&verts[0], &verts[2], p);
    if (!(d > best)) {
        return d;
    }
    return best;
}

// 0x5E7AF0 (thiscall): on a mesh (with its owner and data) and a triangle
uint8_t __fastcall NavPoint_IsValid(const NavPoint* self) {
    uint8_t* col = self->col;
    if (!col || !*reinterpret_cast<re::Ptr32<void>*>(col + 0x3C) || !*reinterpret_cast<re::Ptr32<void>*>(col + 0x38)) {
        return 0;
    }
    return self->tri != -1;
}

// 0x5E7B10 (thiscall): which of the triangle's edges leads to other's triangle (-1)
int __fastcall NavPoint_EdgeTo(const NavPoint* self, re::Edx, const NavPoint* other) {
    int id = NR_NavNeighbourId(self->col, other->col, other->tri);
    uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(self->col + 0x38);
    int32_t* neighbours = reinterpret_cast<int32_t*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(data + 0x18) + self->tri * 0xC);
    for (int i = 0; i < 3; i++) {
        if (neighbours[i] == id) {
            return i;
        }
    }
    return -1;
}

// 0x5E7B60 (thiscall): the node's constructor
NavNode* __fastcall Node_Reset(NavNode* self) {
    self->point.pos = {0.0f, 0.0f, 0.0f};
    self->cost = 0.0f;
    self->point.col = nullptr;
    self->estimate = 0.0f;
    self->point.tri = -1;
    self->parent = nullptr;
    self->unknown20 = -1;
    self->depth = 1;
    return self;
}

#define NAVROUTE_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(NavRouteOriginal_, __LINE__);                                        \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(NavRouteOriginal_, __LINE__), \
             re::Mode::Original)

NAVROUTE_REVERSED("NavPointList::Remove", 0x005E0B90, &List_Remove);
NAVROUTE_REVERSED("NavPointList::Insert", 0x005E0BF0, &List_Insert);
NAVROUTE_REVERSED("NavPoint::Copy", 0x005E0C70, &NavPoint_Copy);
NAVROUTE_REVERSED("CPedRoute::AddNodes", 0x005E0CB0, &Route_AddNodes);
NAVROUTE_REVERSED("CPedRoute::Add", 0x005E0D10, &Route_Add);
NAVROUTE_REVERSED("CPedRoute::AddFront", 0x005E0D70, &Route_AddFront);
NAVROUTE_REVERSED("CPedRoute::Last", 0x005E0DA0, &Route_Last);
NAVROUTE_REVERSED("CPedRoute::BeforeLast", 0x005E0DC0, &Route_BeforeLast);
NAVROUTE_REVERSED("CPedRoute::First", 0x005E0DE0, &Route_First);
NAVROUTE_REVERSED("CPedRoute::RemoveFirst", 0x005E0DF0, &Route_RemoveFirst);
NAVROUTE_REVERSED("CPedRoute::Second", 0x005E0E10, &Route_Second);
NAVROUTE_REVERSED("CPedRoute::Point", 0x005E0E20, &Route_Point);
NAVROUTE_REVERSED("CPedRoute::RemoveLast", 0x005E0E40, &Route_RemoveLast);
NAVROUTE_REVERSED("CPedRoute::Clear", 0x005E0E60, &Route_Clear);
NAVROUTE_REVERSED("NavPointList::Init", 0x005E0EC0, &List_Init);
NAVROUTE_REVERSED("CPedRoute::Construct", 0x005E0F00, &Route_Construct);
NAVROUTE_REVERSED("CPedRoute::Append", 0x005E0F80, &Route_Append);
NAVROUTE_REVERSED("CPedRoute::IsNearEnd", 0x005E1080, &Route_IsNearEnd);
NAVROUTE_REVERSED("NavPoint::Equals", 0x005E1100, &NavPoint_Equals);
NAVROUTE_REVERSED("NavPoint::IsOpen", 0x005E1120, &NavPoint_IsOpen);
NAVROUTE_REVERSED("NavPoint::Set", 0x005E7970, &NavPoint_Set);
NAVROUTE_REVERSED("NavNode::FromPoint", 0x005E7980, &Node_FromPoint);
NAVROUTE_REVERSED("NavPoint::DistToEdges", 0x005E79F0, &NavPoint_DistToEdges);
NAVROUTE_REVERSED("NavPoint::IsValid", 0x005E7AF0, &NavPoint_IsValid);
NAVROUTE_REVERSED("NavPoint::EdgeTo", 0x005E7B10, &NavPoint_EdgeTo);
NAVROUTE_REVERSED("NavNode::Reset", 0x005E7B60, &Node_Reset);

} // namespace
