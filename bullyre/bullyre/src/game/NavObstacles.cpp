// The nav meshes' obstacles: the flat footprints (convex polygons of up to 4 points,
// clockwise, 0x64 bytes, from the pool at 0xC0F624) of the objects standing on the
// meshes, grouped when they're close (a ring through +0x5C, the group's id at +0x60,
// 64 ids), and their list (one, at 0xD12AF8: 200 footprints, objects waiting for their
// collision (+0x4C0) and objects to add next frame (+0x32C)). 0x5E4EB0..0x5E6990.
//
// The CRT's x87 sqrt (0x85C500) is the C library's here.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace {

struct NoVec {
    float x, y, z;
};

// A footprint's point
struct NoPoint {
    float x, y;       // +0x0
    uint8_t flag;     // +0x8 (0x5EB790's result for it)
    uint8_t pad9[3];
};
static_assert(sizeof(NoPoint) == 0xC);

struct CNavObstacle {
    NoPoint points[4];        // +0x0
    uint32_t count;           // +0x30
    float minX, minY;         // +0x34
    float maxX, maxY;         // +0x3C
    float centreX, centreY;   // +0x44
    float minZ, maxZ;         // +0x4C
    float radius;             // +0x54 (from the centre)
    re::Ptr32<uint8_t> entity;          // +0x58
    re::Ptr32<CNavObstacle> next;       // +0x5C (the group's ring)
    int8_t group;             // +0x60 (-1: none)
    uint8_t pad61[3];
};
static_assert(offsetof(CNavObstacle, count) == 0x30);
static_assert(offsetof(CNavObstacle, centreX) == 0x44);
static_assert(offsetof(CNavObstacle, entity) == 0x58);
static_assert(sizeof(CNavObstacle) == 0x64);

struct CNavObstacleList {
    re::Ptr32<CNavObstacle> obstacles[200];   // +0x0
    uint32_t count;                 // +0x320
    uint32_t groups[2];             // +0x324 (the ids in use)
    re::Ptr32<uint8_t> pending[100];          // +0x32C
    uint32_t pendingCount;          // +0x4BC
    re::Ptr32<uint8_t> waiting[100];          // +0x4C0 (no collision data yet)
    uint32_t waitingCount;          // +0x650
};
static_assert(offsetof(CNavObstacleList, groups) == 0x324);
static_assert(offsetof(CNavObstacleList, pendingCount) == 0x4BC);
static_assert(offsetof(CNavObstacleList, waitingCount) == 0x650);

// A box (min, max with a w each)
struct NoBox {
    float min[4];
    float max[4];
};

// A segment for the 2D tests (see AvoidSegment.cpp)
struct NoSegment {
    float start[4];
    float end[4];
};

inline CNavObstacleList* const no_list = reinterpret_cast<CNavObstacleList*>(0x00D12AF8);
GAME_VAR(uint8_t, no_removing, 0x00D12AF0);
GAME_VAR(uint32_t, no_maxPoints, 0x00D12AE8);
GAME_VAR(uint32_t, no_maxObstacles, 0x00D12AEC);
GAME_VAR(uint8_t*, no_pool, 0x00C0F624);
GAME_ARRAY(uint8_t*, no_modelInfos, 0x00C67738);

GAME_FN(void*, __cdecl, NO_GetColModel, 0x0051AF50, void* entity);
GAME_FN(void*, __thiscall, NO_GetMatrix, 0x0046DD20, void* placeable);
// (0x0041A9A0 stores 16 bytes through `out`: called through re::CallVec16)
static inline NoVec* NO_TransformPoint(NoVec* out, const void* m, const NoVec* v) { return re::CallVec16<0x0041A9A0, NoVec*>(out, m, v); }
GAME_FN(NoVec*, __cdecl, NO_InverseTransformPoint, 0x00414D20, NoVec* out, const void* m, const NoVec* v);
GAME_FN(void, __thiscall, NO_Box_Set, 0x005648A0, NoBox* box, const float* min, const float* max, int, int);
GAME_FN(void, __thiscall, NO_Box_GetCorners, 0x00564700, NoBox* box, void* entity, NoVec* corners);
GAME_FN(uint8_t, __thiscall, NO_Col_GetBoundsBelow, 0x0056DBE0, void* colData, float* outMin, float* outMax, float z);
GAME_FN(uint8_t, __thiscall, NO_ModelInfo_520440, 0x00520440, void* modelInfo);
GAME_FN(void*, __thiscall, NO_Pool_New, 0x005E5620, void* pool);
GAME_FN(uint8_t, __cdecl, NO_FindNavPointBelow, 0x005EA870, const NoVec* pos, float range, void* out, uintptr_t, float radius);
GAME_FN(uint8_t, __cdecl, NO_PointFlag, 0x005EB790, float x, float y, float z, float range, int, uintptr_t);
GAME_FN(void, __thiscall, NO_Seg_Set_x64, 0x005DC630, NoSegment* seg, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void NO_Seg_Set(NoSegment* a0, float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8) {
    return NO_Seg_Set_x64(a0, re::BitsOf(a1), re::BitsOf(a2), re::BitsOf(a3), re::BitsOf(a4), re::BitsOf(a5), re::BitsOf(a6), re::BitsOf(a7), re::BitsOf(a8));
}
GAME_FN(uint8_t, __thiscall, NO_Seg_Intersect2D, 0x005E6B30, const NoSegment* seg, const NoSegment* other, float* t,
        float* point);
GAME_FN(NoVec*, __thiscall, NO_Seg_CrossingClamped, 0x005E6CB0, const NoSegment* seg, NoVec* out,
        const NoSegment* other, float margin);
GAME_FN(float, __thiscall, NO_Seg_FlatDistance, 0x005E6E60, const NoSegment* seg, const float* point);
// this file's
GAME_FN(uint8_t, __thiscall, NO_Contains3D, 0x005E4EB0, const CNavObstacle* self, const NoVec* p);
GAME_FN(uint8_t, __thiscall, NO_AllocGroup, 0x005E4F20, CNavObstacleList* self);
GAME_FN(void, __thiscall, NO_FreeGroup, 0x005E4F70, CNavObstacleList* self, int id);
GAME_FN(void, __thiscall, NO_Link, 0x005E4FB0, CNavObstacle* self, CNavObstacle* other);
GAME_FN(uint8_t, __thiscall, NO_BoxContains2D, 0x005E5060, const NoBox* self, const float* p);
GAME_FN(uint8_t, __thiscall, NO_ContainsPoint, 0x005E5130, const CNavObstacle* self, const float* xy, float zLow,
        float zHigh);
GAME_FN(uint8_t, __thiscall, NO_CrossesSegment, 0x005E52F0, const CNavObstacle* self, const NoVec* a, const NoVec* b);
GAME_FN(void, __thiscall, NO_UpdateCentre, 0x005E56A0, CNavObstacle* self);
GAME_FN(CNavObstacle*, __thiscall, NO_Reset, 0x005E57A0, CNavObstacle* self);
GAME_FN(float, __thiscall, NO_MinDistance, 0x005E57F0, const CNavObstacle* self, const CNavObstacle* other, float maxGap);
GAME_FN(void, __thiscall, NO_AppendPoint, 0x005E5AA0, CNavObstacle* self, const NoPoint* p);
GAME_FN(void, __thiscall, NO_InsertPoint, 0x005E5AD0, CNavObstacle* self, const NoPoint* p, uint32_t index);
GAME_FN(int, __thiscall, NO_AddPoint, 0x005E5B30, CNavObstacle* self, const float* xy, float zLow, float zHigh, int flag);
GAME_FN(void, __thiscall, NO_LinkIfClose, 0x005E5FF0, CNavObstacle* self, CNavObstacle* other);
GAME_FN(CNavObstacle*, __thiscall, NO_Destroy, 0x005E6040, CNavObstacle* self, int flags);
GAME_FN(int, __thiscall, NO_AddCorner, 0x005E6120, CNavObstacle* self, const float* xy, float zLow, float zHigh);
GAME_FN(uint8_t, __thiscall, NO_SetFromEntity, 0x005E61B0, CNavObstacle* self, void* entity, float, float, float,
        float, float, float, float, float);
GAME_FN(void, __thiscall, NO_Unlink, 0x005E6390, CNavObstacle* self);
GAME_FN(void, __thiscall, NO_List_Add, 0x005E6510, CNavObstacleList* self, uint8_t* entity);

// the footprint's fields cleared (0x5E6040 without the pool)
void ClearFields(CNavObstacle* o) {
    o->centreX = 0.0f;
    o->centreY = 0.0f;
    o->minZ = 0.0f;
    o->maxZ = 0.0f;
    o->minX = 0.0f;
    o->minY = 0.0f;
    o->maxX = 0.0f;
    o->maxY = 0.0f;
    o->entity = nullptr;
    o->group = -1;
    o->next = nullptr;
    o->count = 0;
}
// back to the pool (CPool::Delete, inline)
void PoolFree(CNavObstacle* o) {
    uint8_t* pool = no_pool;
    int index = int(reinterpret_cast<uint8_t*>(o) - *reinterpret_cast<re::Ptr32<uint8_t>*>(pool))
                / *reinterpret_cast<int32_t*>(pool + 0xC);
    (*reinterpret_cast<re::Ptr32<uint8_t>*>(pool + 4))[index] |= 0x80;
    if (index < *reinterpret_cast<int32_t*>(pool + 0x14)) {
        *reinterpret_cast<int32_t*>(pool + 0x14) = index;
    }
}
float SqrtAbs(double v) {
    return float(std::sqrt(double(std::fabs(float(v)))));
}

// 0x5E4EB0 (thiscall): inside the bounding box (and height range)?
uint8_t __fastcall Contains3D(const CNavObstacle* self, re::Edx, const NoVec* p) {
    if (self->maxZ < p->z || self->minZ > p->z) {
        return 0;
    }
    if (self->maxX < p->x || self->minX > p->x) {
        return 0;
    }
    if (self->maxY < p->y || self->minY > p->y) {
        return 0;
    }
    return 1;
}

// 0x5E4F20 (thiscall): a free group id (-1 when all 64 are taken)
uint8_t __fastcall AllocGroup(CNavObstacleList* self) {
    for (int word = 0; word < 2; word++) {
        for (int bit = 0; bit < 32; bit++) {
            uint32_t mask = 1u << bit;
            if ((mask & self->groups[word]) == 0) {
                self->groups[word] |= mask;
                return uint8_t(word * 32 + bit);
            }
        }
    }
    return 0xFF;
}

// 0x5E4F70 (thiscall): frees a group id (a signed byte: -1 frees bit 31 of the first word)
void __fastcall FreeGroup(CNavObstacleList* self, re::Edx, int id) {
    int8_t c = int8_t(id);
    bool low = c < 32;
    int word = low ? 0 : 1;
    int bit = low ? c : c - 32;
    self->groups[word] &= ~(1u << (bit & 31));
}

// 0x5E4FB0 (thiscall): puts two footprints in one group (a new one, one's, or the two
// rings merged into this one's, the other's id freed)
void __fastcall Link(CNavObstacle* self, re::Edx, CNavObstacle* other) {
    if (self->group == -1 && other->group == -1) {
        int8_t id = int8_t(NO_AllocGroup(no_list));
        self->group = id;
        self->next = other;
        other->group = id;
        other->next = self;
        return;
    }
    if (other->group == -1) {
        CNavObstacle* after = self->next;
        self->next = other;
        other->next = after;
        other->group = self->group;
        return;
    }
    if (self->group == -1) {
        CNavObstacle* after = other->next;
        other->next = self;
        self->next = after;
        self->group = other->group;
        return;
    }
    if (self->group == other->group) {
        return;
    }
    CNavObstacle* after = self->next;
    self->next = other;
    int8_t oldGroup = other->group;
    other->group = self->group;
    CNavObstacle* p = other->next;
    do {
        p->group = self->group;
        p = p->next;
    } while (p->next != other);
    p->next = after;
    p->group = self->group;
    NO_FreeGroup(no_list, oldGroup);
}

// 0x5E5060 (thiscall, on a box): strictly inside it (flat)?
uint8_t __fastcall BoxContains2D(const NoBox* self, re::Edx, const float* p) {
    if (!(self->min[0] < p[0])) {
        return 0;
    }
    if (!(self->min[1] < p[1])) {
        return 0;
    }
    if (!(self->max[0] > p[0])) {
        return 0;
    }
    if (!(self->max[1] > p[1])) {
        return 0;
    }
    return 1;
}

// 0x5E5130 (thiscall): (x, y) inside the polygon (0.001 in from its edges) and the height
// range overlapping?
uint8_t __fastcall ContainsPoint(const CNavObstacle* self, re::Edx, const float* xy, float zLow, float zHigh) {
    if (self->minZ > zHigh || self->maxZ < zLow) {
        return 0;
    }
    if (self->minX > xy[0] || self->minY > xy[1] || self->maxX < xy[0] || self->maxY < xy[1]) {
        return 0;
    }
    uint32_t count = self->count;
    if (count < 3) {
        return 0;
    }
    for (uint32_t i = 0; i < count; i++) {
        const NoPoint& a = self->points[i];
        const NoPoint& b = i == count - 1 ? self->points[0] : self->points[i + 1];
        float ex = xy[0] - a.x;
        float ey = xy[1] - a.y;
        float dx = b.x - a.x;
        float dy = b.y - a.y;
        float t = float(dx * -1.0);
        float cross = float(double(t) * ey + double(dy) * ex);
        float size = std::fabs(cross);
        if (size < 0.001f || size == 0.001f) {
            return 0;
        }
        if (cross > 0.001f) {
            return 0;
        }
    }
    return 1;
}

// 0x5E52B0 (thiscall): the same for a point at its own height
uint8_t __fastcall ContainsPoint3D(const CNavObstacle* self, re::Edx, const NoVec* p) {
    float xy[2] = {p->x, p->y};
    return NO_ContainsPoint(self, xy, p->z, p->z);
}

// 0x5E52F0 (thiscall): does a..b cross one of the edges (flat)?
uint8_t __fastcall CrossesSegment(const CNavObstacle* self, re::Edx, const NoVec* a, const NoVec* b) {
    NoSegment line = {{a->x, a->y, a->z, 0.0f}, {b->x, b->y, b->z, 0.0f}};
    for (uint32_t i = 0; i < self->count; i++) {
        const NoPoint& p = self->points[i];
        const NoPoint& q = i < self->count - 1 ? self->points[i + 1] : self->points[0];
        NoSegment edge = {{p.x, p.y, self->minZ, 0.0f}, {q.x, q.y, self->minZ, 0.0f}};
        if (NO_Seg_Intersect2D(&line, &edge, nullptr, nullptr)) {
            return 1;
        }
    }
    return 0;
}

// 0x5E5450 (thiscall): the fields cleared
void __fastcall Clear(CNavObstacle* self) {
    ClearFields(self);
}

// 0x5E5480 (thiscall): an object to add next frame (100 at most)
void __fastcall List_AddPending(CNavObstacleList* self, re::Edx, uint8_t* entity) {
    if (self->pendingCount != 100) {
        self->pending[self->pendingCount] = entity;
        self->pendingCount++;
    }
}

// 0x5E54B0 (thiscall): the waiting objects whose collision has come move to the pending
// list (the one after each moved is skipped)
void __fastcall List_RetryWaiting(CNavObstacleList* self) {
    for (uint32_t i = 0; i < self->waitingCount; i++) {
        uint8_t* entity = self->waiting[i];
        uint8_t* col = static_cast<uint8_t*>(NO_GetColModel(entity));
        if (*reinterpret_cast<re::Ptr32<void>*>(col + 0x38) == nullptr) {
            continue;
        }
        if (self->pendingCount != 100) {
            self->pending[self->pendingCount] = self->waiting[i];
            self->pendingCount++;
        }
        for (uint32_t j = i; j < self->waitingCount - 1; j++) {
            self->waiting[j] = self->waiting[j + 1];
        }
        self->waitingCount--;
    }
}

// 0x5E5550 (thiscall): is p inside a footprint (but `except`)?
uint8_t __fastcall List_AnyContains(CNavObstacleList* self, re::Edx, CNavObstacle* except, const NoVec* p) {
    uint32_t count = self->count;
    for (uint32_t i = 0; i < count; i++) {
        CNavObstacle* o = self->obstacles[i];
        if (o == except || !NO_Contains3D(o, p)) {
            continue;
        }
        float xy[2] = {p->x, p->y};
        if (NO_ContainsPoint(o, xy, p->z, p->z)) {
            return 1;
        }
    }
    return 0;
}

// 0x5E55D0 (thiscall): does a..b cross a footprint (but `except`)?
uint8_t __fastcall List_AnyCrosses(CNavObstacleList* self, re::Edx, CNavObstacle* except, const NoVec* a, const NoVec* b) {
    for (uint32_t i = 0; i < self->count; i++) {
        CNavObstacle* o = self->obstacles[i];
        if (o != except && NO_CrossesSegment(o, a, b)) {
            return 1;
        }
    }
    return 0;
}

// 0x5E56A0 (thiscall): the centre (the points' mean) and radius
void __fastcall UpdateCentre(CNavObstacle* self) {
    float sumX = 0.0f;
    float sumY = 0.0f;
    uint32_t count = self->count;
    for (uint32_t i = 0; i < count; i++) {
        sumX = sumX + self->points[i].x;
        sumY = self->points[i].y + sumY;
    }
    float n = float(double(count));
    self->centreX = sumX / n;
    self->centreY = sumY / n;
    self->radius = 0.0f;
    for (uint32_t i = 0; i < self->count; i++) {
        float dx = self->points[i].x - self->centreX;
        float dy = self->points[i].y - self->centreY;
        float d = SqrtAbs(double(dx) * dx + double(dy) * dy);
        self->radius = self->radius < d ? d : self->radius;
    }
}

// 0x5E57A0 (thiscall): the constructor's clearing
CNavObstacle* __fastcall Reset(CNavObstacle* self) {
    self->minX = 0.0f;
    self->minY = 0.0f;
    self->maxX = 0.0f;
    self->maxY = 0.0f;
    self->centreX = 0.0f;
    self->centreY = 0.0f;
    self->radius = 0.0f;
    self->count = 0;
    ClearFields(self);
    return self;
}

// 0x5E57F0 (thiscall): the least distance between the two outlines (their 4 edges each,
// whatever the counts), when their circles come within maxGap (else FLT_MAX)
float __fastcall MinDistance(const CNavObstacle* self, re::Edx, const CNavObstacle* other, float maxGap) {
    float dx = other->centreX - self->centreX;
    float dy = other->centreY - self->centreY;
    float best = 3.4028235e+38f;
    float dist = SqrtAbs(double(dx) * dx + double(dy) * dy);
    float radii = other->radius + self->radius;
    if (!(maxGap > double(dist) - radii)) {
        return best;
    }
    for (int i = 0; i < 4; i++) {
        const NoPoint& a = self->points[i];
        const NoPoint& b = self->points[(i + 1) % 4];
        NoSegment mine = {{a.x, a.y, 0.0f, 0.0f}, {b.x, b.y, 0.0f, 0.0f}};
        for (int j = 0; j < 4; j++) {
            const NoPoint& c = other->points[j];
            const NoPoint& d = other->points[(j + 1) % 4];
            NoSegment theirs = {{c.x, c.y, 0.0f, 0.0f}, {d.x, d.y, 0.0f, 0.0f}};
            NoVec point;
            NO_Seg_CrossingClamped(&mine, &point, &theirs, 0.0f);
            float value = NO_Seg_FlatDistance(&theirs, &point.x);
            if (best > value) {
                best = value;
            }
        }
    }
    return best;
}

// 0x5E5AA0 (thiscall): a point at the end
void __fastcall AppendPoint(CNavObstacle* self, re::Edx, const NoPoint* p) {
    NoPoint& slot = self->points[self->count];
    slot.x = p->x;
    slot.y = p->y;
    slot.flag = p->flag;
    self->count++;
}

// 0x5E5AD0 (thiscall): a point inserted at index
void __fastcall InsertPoint(CNavObstacle* self, re::Edx, const NoPoint* p, uint32_t index) {
    self->count++;
    for (uint32_t i = self->count - 1; i > index; i--) {
        self->points[i].x = self->points[i - 1].x;
        self->points[i].y = self->points[i - 1].y;
        self->points[i].flag = self->points[i - 1].flag;
    }
    self->points[index].x = p->x;
    self->points[index].y = p->y;
    self->points[index].flag = p->flag;
}

// 0x5E5B30 (thiscall): a point added keeping the polygon convex and clockwise (a fifth
// isn't; one on the line of the first two isn't; from 4 on, points that make a dent are
// dropped), the bounds grown: its index (-1 when not in)
int __fastcall AddPoint(CNavObstacle* self, re::Edx, const float* xy, float zLow, float zHigh, int flag) {
    int found = -1;
    uint32_t count = self->count;
    if (count == 4) {
        return -1;
    }
    NoPoint point;
    point.x = xy[0];
    point.y = xy[1];
    point.flag = uint8_t(flag);
    if (count < 2) {
        NoPoint& slot = self->points[count];
        slot.x = point.x;
        slot.flag = uint8_t(flag);
        slot.y = point.y;
        self->count++;
    } else if (count == 2) {
        const NoPoint& a = self->points[0];
        const NoPoint& b = self->points[1];
        float ex = point.x - a.x;
        float ey = point.y - a.y;
        float dy = b.y - a.y;
        float dx = b.x - a.x;
        float t = float(dx * -1.0);
        float cross = float(double(t) * ey + double(dy) * ex);
        if (cross == 0.0f) {
            return -1;
        }
        if (cross > 0.0f) {
            NO_InsertPoint(self, &point, 1);
        } else {
            NO_AppendPoint(self, &point);
        }
        NO_UpdateCentre(self);
    } else {
        NoSegment ray;
        NO_Seg_Set(&ray, self->centreX, self->centreY, 0.0f, 0.0f, point.x, point.y, 0.0f, 0.0f);
        for (uint32_t i = 0; i < self->count; i++) {
            const NoPoint& a = self->points[i];
            const NoPoint& b = i < self->count - 1 ? self->points[i + 1] : self->points[0];
            NoSegment edge = {{a.x, a.y, 0.0f, 0.0f}, {b.x, b.y, 0.0f, 0.0f}};
            float hit[4];
            if (!NO_Seg_Intersect2D(&ray, &edge, nullptr, hit)) {
                continue;
            }
            if (i < self->count - 1) {
                NO_InsertPoint(self, &point, i + 1);
            } else {
                NoPoint& slot = self->points[self->count];
                slot.x = point.x;
                slot.flag = uint8_t(flag);
                slot.y = point.y;
                self->count++;
            }
            NO_UpdateCentre(self);
            break;
        }
        if (self->count >= 4) {
            for (uint32_t i = 0; i < self->count;) {
                uint32_t n = self->count;
                const NoPoint& prev = i == 0 ? self->points[n - 1] : self->points[i - 1];
                const NoPoint& cur = self->points[i];
                const NoPoint& next = i == 0 ? self->points[1] : (i == n - 1 ? self->points[0] : self->points[i + 1]);
                float a = next.x - prev.x;
                float b = next.y - prev.y;
                float c = float(a * -1.0);
                float dx = cur.x - prev.x;
                float dy = cur.y - prev.y;
                float cross = float(double(c) * dy + double(b) * dx);
                if (!(cross < -0.01f)) {
                    i++;
                    continue;
                }
                for (uint32_t j = i; j < self->count - 1; j++) {
                    self->points[j].x = self->points[j + 1].x;
                    self->points[j].y = self->points[j + 1].y;
                    self->points[j].flag = self->points[j + 1].flag;
                }
                self->count--;
            }
        }
    }
    count = self->count;
    for (uint32_t i = 0; i < count; i++) {
        if (xy[0] == self->points[i].x && xy[1] == self->points[i].y) {
            found = int(i);
        }
    }
    if (count == 1) {
        self->minZ = zLow;
        self->maxZ = zHigh;
        self->minX = xy[0];
        self->maxX = xy[0];
        self->minY = xy[1];
        self->maxY = xy[1];
    } else {
        if (zLow < self->minZ) {
            self->minZ = zLow;
        }
        if (self->maxZ < zHigh) {
            self->maxZ = zHigh;
        }
        if (xy[0] < self->minX) {
            self->minX = xy[0];
        }
        if (self->maxX < xy[0]) {
            self->maxX = xy[0];
        }
        if (xy[1] < self->minY) {
            self->minY = xy[1];
        }
        if (self->maxY < xy[1]) {
            self->maxY = xy[1];
        }
    }
    if (no_maxPoints < count) {
        no_maxPoints = count;
    }
    return found;
}

// 0x5E5FF0 (thiscall): grouped with other when their heights overlap and the outlines
// come within 0.75
void __fastcall LinkIfClose(CNavObstacle* self, re::Edx, CNavObstacle* other) {
    if (!(other->minZ < self->maxZ)) {
        return;
    }
    if (!(other->maxZ > self->minZ)) {
        return;
    }
    if (NO_MinDistance(self, other, 0.75f) < 0.75f) {
        NO_Link(self, other);
    }
}

// 0x5E6040 (thiscall): the destructor (flags & 1: back to the pool)
CNavObstacle* __fastcall Destroy(CNavObstacle* self, re::Edx, int flags) {
    ClearFields(self);
    if (flags & 1) {
        PoolFree(self);
    }
    return self;
}

// 0x5E60A0 (thiscall): every footprint back to the pool (the count kept)
void __fastcall List_FreeAll(CNavObstacleList* self) {
    for (uint32_t i = 0; i < self->count; i++) {
        CNavObstacle* o = self->obstacles[i];
        if (o) {
            ClearFields(o);
            PoolFree(o);
            self->obstacles[i] = nullptr;
        }
    }
}

// 0x5E6120 (thiscall): a corner of an object's footprint, its point flagged by the
// meshes around it
int __fastcall AddCorner(CNavObstacle* self, re::Edx, const float* xy, float zLow, float zHigh) {
    int index = NO_AddPoint(self, xy, zLow, zHigh, 0);
    if (index >= 0 && uint32_t(index) < self->count) {
        double middle = (double(self->maxZ) + self->minZ) * 0.5;
        double range = 20.0 < middle ? middle : 20.0;
        NoPoint& p = self->points[index];
        p.flag = NO_PointFlag(p.x, p.y, float(middle), float(range), 1, 0);
    }
    return index;
}

// 0x5E61B0 (thiscall): the footprint of an object's box (min, max; half a unit taller
// each way): its 4 corners in the world
uint8_t __fastcall SetFromEntity(CNavObstacle* self, re::Edx, void* entity, float minX, float minY, float minZ,
                                 float minW, float maxX, float maxY, float maxZ, float maxW) {
    if (self->entity != nullptr) {
        return 0;
    }
    if (!entity) {
        return 0;
    }
    float min[4] = {minX, minY, minZ, minW};
    float max[4] = {maxX, maxY, maxZ, maxW};
    NoBox box;
    NO_Box_Set(&box, min, max, 0, 0);
    box.min[2] = float(box.min[2] - 0.5);
    self->entity = static_cast<uint8_t*>(entity);
    box.max[2] = float(0.5 + box.max[2]);
    NoVec corners[4] = {};
    NO_Box_GetCorners(&box, entity, corners);
    float lo = corners[0].z;
    float hi = corners[0].z;
    if (corners[1].z < corners[0].z) {
        lo = corners[1].z;
    }
    if (corners[1].z > corners[0].z) {
        hi = corners[1].z;
    }
    for (int i = 2; i < 4; i++) {
        if (lo > corners[i].z) {
            lo = corners[i].z;
        }
        if (hi < corners[i].z) {
            hi = corners[i].z;
        }
    }
    float xy[4][2];
    for (int i = 0; i < 4; i++) {
        xy[i][0] = corners[i].x;
        xy[i][1] = corners[i].y;
    }
    for (int i = 0; i < 4; i++) {
        NO_AddCorner(self, xy[i], lo, hi);
    }
    return 1;
}

// 0x5E6390 (thiscall): out of its group: the ring broken up and its other members
// regrouped among themselves. (The game's list holds 47; this one more.)
void __fastcall Unlink(CNavObstacle* self) {
    if (self->group == -1) {
        return;
    }
    CNavObstacle* members[64];
    int count = 0;
    for (CNavObstacle* p = self->next; p != self;) {
        CNavObstacle* after = p->next;
        members[count++] = p;
        p->next = nullptr;
        p->group = -1;
        p = after;
    }
    int8_t c = self->group;
    bool low = c < 32;
    int bit = low ? c : c - 32;
    no_list->groups[low ? 0 : 1] &= ~(1u << (bit & 31));
    self->group = -1;
    for (int i = 1; i < count; i++) {
        CNavObstacle* a = members[i];
        for (int j = 0; j < i; j++) {
            CNavObstacle* b = members[j];
            if (!(b->minZ < a->maxZ) || !(b->maxZ > a->minZ)) {
                continue;
            }
            if (0.75f > NO_MinDistance(a, b, 0.75f)) {
                NO_Link(a, b);
            }
        }
    }
}

// 0x5E6480 (thiscall): everything cleared
void __fastcall List_Clear(CNavObstacleList* self) {
    for (uint32_t i = 0; i < self->count; i++) {
        CNavObstacle* o = self->obstacles[i];
        if (o) {
            ClearFields(o);
            PoolFree(o);
            self->obstacles[i] = nullptr;
        }
    }
    self->count = 0;
    self->pendingCount = 0;
    self->waitingCount = 0;
    self->groups[0] = 0;
    self->groups[1] = 0;
}

// 0x5E6510 (thiscall): an object's footprint added (objects whose model has a collision
// model and no +9 flag aren't; without collision data yet it waits): the flat bounds of
// its collision under 2 above its bottom, kept when the meshes have a point within them
// near its middle, grouped with the others close by
void __fastcall List_Add(CNavObstacleList* self, re::Edx, uint8_t* entity) {
    if (!entity) {
        return;
    }
    uint8_t* mi = no_modelInfos[*reinterpret_cast<int16_t*>(entity + 0x10E)];
    if (mi && *reinterpret_cast<re::Ptr32<void>*>(mi + 0xC) && mi[9] == 0) {
        return;
    }
    for (uint32_t i = 0; i < self->count; i++) {
        if (self->obstacles[i]->entity == entity) {
            return;
        }
    }
    uint8_t* col = static_cast<uint8_t*>(NO_GetColModel(entity));
    void* data = *reinterpret_cast<re::Ptr32<void>*>(col + 0x38);
    if (!data) {
        if (self->waitingCount != 100) {
            self->waiting[self->waitingCount] = entity;
            self->waitingCount++;
        }
        return;
    }
    const float* colMin = reinterpret_cast<const float*>(col + 0x10);
    const float* colMax = reinterpret_cast<const float*>(col + 0x20);
    float height = colMax[2] - colMin[2];
    float halfHeight = float(height * 0.5);
    float boundsMin[4];
    float boundsMax[4];
    float below = float(colMin[2] + 2.0);
    if (!NO_Col_GetBoundsBelow(data, boundsMin, boundsMax, below)) {
        return;
    }
    NoVec middle = {float((double(boundsMin[0]) + boundsMax[0]) * 0.5),
                    float((double(boundsMin[1]) + boundsMax[1]) * 0.5), float(colMin[2] + double(0.2f))};
    NoVec world;
    NO_TransformPoint(&world, NO_GetMatrix(entity), &middle);
    float dx = boundsMax[0] - boundsMin[0];
    float dy = boundsMax[1] - boundsMin[1];
    NoBox bounds;
    NO_Box_Set(&bounds, boundsMin, boundsMax, 0, 0);
    float dxSq = dx * dx;
    float dySq = dy * dy;
    float size = float(std::sqrt(double(std::fabs(dxSq + dySq))));
    float radius = float(size * 0.5);
    uint8_t found[0x14] = {};
    *reinterpret_cast<int32_t*>(found + 4) = -1;
    float range = float(3.5f + double(0.2f));
    NoVec at = world;
    if (!NO_FindNavPointBelow(&at, range, found, 0, radius)) {
        at.z = at.z + halfHeight;
        range = halfHeight + range;
        if (!NO_FindNavPointBelow(&at, range, found, 0, radius)) {
            return;
        }
    }
    NoVec onMesh = {*reinterpret_cast<float*>(found + 8), *reinterpret_cast<float*>(found + 0xC), at.z};
    NoVec local;
    NO_InverseTransformPoint(&local, NO_GetMatrix(entity), &onMesh);
    if (!NO_BoxContains2D(&bounds, &local.x)) {
        return;
    }
    CNavObstacle* obstacle = static_cast<CNavObstacle*>(NO_Pool_New(no_pool));
    if (obstacle) {
        obstacle = NO_Reset(obstacle);
    }
    if (NO_SetFromEntity(obstacle, entity, boundsMin[0], boundsMin[1], boundsMin[2], boundsMin[3], boundsMax[0],
                         boundsMax[1], boundsMax[2], boundsMax[3])) {
        self->obstacles[self->count] = obstacle;
        self->count++;
    } else if (obstacle) {
        NO_Destroy(obstacle, 1);
    }
    for (uint32_t i = 0; i < self->count - 1; i++) {
        NO_LinkIfClose(obstacle, self->obstacles[i]);
    }
    if (no_maxObstacles < self->count) {
        no_maxObstacles = self->count;
    }
}

// 0x5E68F0 (thiscall): added unless its model says otherwise (0x520440)
void __fastcall List_AddObject(CNavObstacleList* self, re::Edx, uint8_t* entity) {
    uint8_t* mi = no_modelInfos[*reinterpret_cast<int16_t*>(entity + 0x10E)];
    if (!NO_ModelInfo_520440(mi)) {
        NO_List_Add(self, entity);
    }
}

// 0x5E6920 (thiscall): the pending objects added (prop anims only when their model
// allows)
void __fastcall List_AddPendingNow(CNavObstacleList* self) {
    for (uint32_t i = 0; i < self->pendingCount; i++) {
        uint8_t* entity = self->pending[i];
        if ((entity[0x108] & 7) == 6) {
            uint8_t* mi = no_modelInfos[*reinterpret_cast<int16_t*>(entity + 0x10E)];
            if (NO_ModelInfo_520440(mi)) {
                continue;
            }
        }
        NO_List_Add(self, entity);
    }
    self->pendingCount = 0;
}

// 0x5E6990 (thiscall): an object's footprint removed (and it from the waiting list)
void __fastcall List_Remove(CNavObstacleList* self, re::Edx, uint8_t* entity) {
    no_removing = 1;
    for (uint32_t i = 0; i < self->waitingCount; i++) {
        if (self->waiting[i] != entity) {
            continue;
        }
        for (uint32_t j = i; j < self->waitingCount - 1; j++) {
            self->waiting[j] = self->waiting[j + 1];
        }
        self->waitingCount--;
    }
    for (uint32_t i = 0; i < self->count; i++) {
        CNavObstacle* o = self->obstacles[i];
        if (o->entity != entity) {
            continue;
        }
        if (o->entity == entity) {
            o->entity = nullptr;
        }
        NO_Unlink(o);
        if (o->entity == nullptr) {
            ClearFields(o);
            PoolFree(o);
            self->obstacles[i] = nullptr;
            for (uint32_t j = i; j < self->count - 1; j++) {
                self->obstacles[j] = self->obstacles[j + 1];
            }
            self->count--;
        }
        break;
    }
    no_removing = 0;
}

#define NAVOBST_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(NavObstOriginal_, __LINE__);                                        \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(NavObstOriginal_, __LINE__), \
             re::Mode::Original)

NAVOBST_REVERSED("NavObstacle::Contains3D", 0x005E4EB0, &Contains3D);
NAVOBST_REVERSED("NavObstacleList::AllocGroup", 0x005E4F20, &AllocGroup);
NAVOBST_REVERSED("NavObstacleList::FreeGroup", 0x005E4F70, &FreeGroup);
NAVOBST_REVERSED("NavObstacle::Link", 0x005E4FB0, &Link);
NAVOBST_REVERSED("NavObstacle::BoxContains2D", 0x005E5060, &BoxContains2D);
NAVOBST_REVERSED("NavObstacle::ContainsPoint", 0x005E5130, &ContainsPoint);
NAVOBST_REVERSED("NavObstacle::ContainsPoint3D", 0x005E52B0, &ContainsPoint3D);
NAVOBST_REVERSED("NavObstacle::CrossesSegment", 0x005E52F0, &CrossesSegment);
NAVOBST_REVERSED("NavObstacle::Clear", 0x005E5450, &Clear);
NAVOBST_REVERSED("NavObstacleList::AddPending", 0x005E5480, &List_AddPending);
NAVOBST_REVERSED("NavObstacleList::RetryWaiting", 0x005E54B0, &List_RetryWaiting);
NAVOBST_REVERSED("NavObstacleList::AnyContains", 0x005E5550, &List_AnyContains);
NAVOBST_REVERSED("NavObstacleList::AnyCrosses", 0x005E55D0, &List_AnyCrosses);
NAVOBST_REVERSED("NavObstacle::UpdateCentre", 0x005E56A0, &UpdateCentre);
NAVOBST_REVERSED("NavObstacle::Reset", 0x005E57A0, &Reset);
NAVOBST_REVERSED("NavObstacle::MinDistance", 0x005E57F0, &MinDistance);
NAVOBST_REVERSED("NavObstacle::AppendPoint", 0x005E5AA0, &AppendPoint);
NAVOBST_REVERSED("NavObstacle::InsertPoint", 0x005E5AD0, &InsertPoint);
NAVOBST_REVERSED("NavObstacle::AddPoint", 0x005E5B30, &AddPoint);
NAVOBST_REVERSED("NavObstacle::LinkIfClose", 0x005E5FF0, &LinkIfClose);
NAVOBST_REVERSED("NavObstacle::Destroy", 0x005E6040, &Destroy);
NAVOBST_REVERSED("NavObstacleList::FreeAll", 0x005E60A0, &List_FreeAll);
NAVOBST_REVERSED("NavObstacle::AddCorner", 0x005E6120, &AddCorner);
NAVOBST_REVERSED("NavObstacle::SetFromEntity", 0x005E61B0, &SetFromEntity);
NAVOBST_REVERSED("NavObstacle::Unlink", 0x005E6390, &Unlink);
NAVOBST_REVERSED("NavObstacleList::Clear", 0x005E6480, &List_Clear);
NAVOBST_REVERSED("NavObstacleList::Add", 0x005E6510, &List_Add);
NAVOBST_REVERSED("NavObstacleList::AddObject", 0x005E68F0, &List_AddObject);
NAVOBST_REVERSED("NavObstacleList::AddPendingNow", 0x005E6920, &List_AddPendingNow);
NAVOBST_REVERSED("NavObstacleList::Remove", 0x005E6990, &List_Remove);

} // namespace
