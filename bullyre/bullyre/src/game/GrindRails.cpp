// Rails (grind and balance rails): a rail prop (CPropAnim) has a point list, by index
// +0x245 into the rail pool (0x20C7C98); the points (0x20C7C94) are chained by the link
// table at 0xBC5D38 (0xFFFE ends a chain, 0xFFFF is none). The player's rail fields:
// +0x13C4 balancing, +0x1548 grinding, +0x1E98 the rail prop, +0x1E9C the point,
// +0x1EA0 the grind target, +0x1EAC its heading, +0x1EB0 the point on the rail.

#include <cmath>
#include <cstdint>
#include <cstring>

#include "reversed.h"

namespace {

using re::At;

struct CVec {
    float x, y, z;
};

template <typename R, typename... A>
R VCall(void* obj, int offset, A... args) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(obj))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(fn)(obj, args...);
}

struct PoolHeader {
    re::Ptr32<uint8_t> objects;   // +0
    re::Ptr32<uint8_t> flags;     // +4
    int32_t size;       // +8
    int32_t elemSize;   // +0xC
    int32_t unknown10;  // +0x10
    int32_t lastUsed;   // +0x14
};

struct RailPoint {
    re::Ptr32<void> vtable;  // +0 (0x914150)
    CVec pos;      // +4
};
static_assert(sizeof(RailPoint) == 0x10);

struct RailPointList {
    re::Ptr32<void> vtable;     // +0 (0x914158)
    uint16_t first;   // +4
    uint16_t cursor;  // +6 the next point while walking
    float radius;     // +8 around the points' centre, plus 3
};
static_assert(sizeof(RailPointList) == 0xC);

GAME_VAR(uint8_t*, gr_playerPed, 0x00C1AEA8);
GAME_VAR(PoolHeader*, gr_pointPool, 0x020C7C94);
GAME_VAR(PoolHeader*, gr_railPool, 0x020C7C98);
GAME_VAR(uint16_t*, gr_links, 0x00BC5D38);
GAME_VAR(CVec, gr_up, 0x009E33DC);   // (0, 0, 1)
inline void* const gr_linkList = reinterpret_cast<void*>(0x00BC5D34);
inline void* const gr_balanceNode = reinterpret_cast<void*>(0x01B652B8);
inline void* const gr_pointVtable = reinterpret_cast<void*>(0x00914150);
inline void* const gr_listVtable = reinterpret_cast<void*>(0x00914158);
inline void* const gr_vectorConstruct = reinterpret_cast<void*>(0x00508410);

GAME_FN(CVec*, __cdecl, VectorAdd, 0x004129A0, CVec* out, const CVec* a, const CVec* b);
GAME_FN(CVec*, __cdecl, VectorSub, 0x00414B70, CVec* out, const CVec* a, const CVec* b);
GAME_FN(CVec*, __cdecl, VectorSub2, 0x00414BA0, CVec* out, const CVec* a, const CVec* b);
GAME_FN(CVec*, __cdecl, CrossProduct, 0x00414730, CVec* out, const CVec* a, const CVec* b);
GAME_FN(CVec*, __thiscall, Entity_GetForward, 0x004154A0, void* entity, CVec* out);
GAME_FN(float, __thiscall, Entity_GetHeading, 0x00415470, void* entity);
GAME_FN(float, __cdecl, Vector_Heading, 0x00433310, const CVec* v);
GAME_FN(float, __cdecl, HeadingBetween, 0x00433350, const CVec* from, const CVec* to);
GAME_FN(float, __cdecl, AngleDifference, 0x00432980, float a, float b);
GAME_FN(CVec*, __cdecl, VectorFromHeading, 0x00432DF0, CVec* out, float heading);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVec* v);
GAME_FN(float, __thiscall, Vector_NormaliseAndMag, 0x004294A0, float* v);
GAME_FN(void, __thiscall, Vector_Assign, 0x004129D0, void* dst, const float* src);
GAME_FN(void, __thiscall, Matrix_SetTranslate, 0x004127C0, void* matrix, const CVec* pos);
GAME_FN(uint8_t*, __thiscall, Ped_GetMatrix, 0x004734D0, void* ped);
GAME_FN(float*, __thiscall, ColLine_Set, 0x00564950, void* line, const CVec* start, const CVec* end);
GAME_FN(void, __cdecl, ClosestPoints, 0x005658C0, const float* lineA, const float* lineB, CVec* onA, CVec* onB);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, size_t size, int count, void* ctor);
GAME_FN(uint8_t, __thiscall, Controller_IsNodePlaying, 0x005F4800, void* controller, void* node, int, int);
GAME_FN(void, __thiscall, Controller_Play, 0x005F45F0, void* controller, void* node);
GAME_FN(void, __thiscall, PedUsePoint_Set, 0x00478850, void* usePoint, void* prop);
GAME_FN(void, __thiscall, LinkList_FreeChain, 0x00447A70, void* list, int first);
GAME_FN(void*, __thiscall, PointPool_New, 0x005F1640, PoolHeader* pool);
// (ours, through their addresses)
GAME_FN(RailPoint*, __thiscall, Rail_First, 0x004F0370, RailPointList* self);
GAME_FN(RailPoint*, __thiscall, Rail_Next, 0x004F03B0, RailPointList* self);
GAME_FN(float, __thiscall, Rail_GetRadius, 0x004F02D0, RailPointList* self);
GAME_FN(CVec*, __thiscall, Point_GetPos, 0x004F02A0, RailPoint* self);
GAME_FN(void, __thiscall, Rail_DeleteAll, 0x004F0770, RailPointList* self);
GAME_FN(RailPointList*, __thiscall, RailPool_New, 0x004F02E0, PoolHeader* pool);

const CVec& Position(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? *reinterpret_cast<CVec*>(matrix + 0x30) : At<CVec>(entity, 4);
}

// a length as the game takes it (the square stored, its fabs rooted)
float Length(const CVec& v) {
    float sq = float(double(v.x) * v.x + double(v.y) * v.y + double(v.z) * v.z);
    sq = std::fabs(sq);
    return std::sqrt(sq);
}

RailPointList* RailOf(uint8_t* prop) {
    uint8_t index = prop[0x245];
    if (index == 0) {
        return nullptr;
    }
    PoolHeader* pool = gr_railPool;
    int slot = index - 1;
    if (pool->flags[slot] & 0x80) {
        return nullptr;
    }
    return reinterpret_cast<RailPointList*>(pool->objects + pool->elemSize * slot);
}

// the player takes the prop's grab action (props of kind 6 with an action tree, when
// not using anything)
void GrabRail(uint8_t* prop, uint8_t* player) {
    if ((prop[0x108] & 7) != 6 || At<void*>(prop, 0x230) == nullptr || At<void*>(player, 0x548) != nullptr) {
        return;
    }
    At<int32_t>(prop, 0x188) = 0;
    void* tree = At<void*>(prop, 0x230);
    void* key = VCall<void*>(player, 0x40);
    void* node = VCall<void*>(tree, 0x4C, key);
    Controller_Play(At<uint8_t*>(player, 0x2E0) + 0x258, node);
    PedUsePoint_Set(player + 0x3BC, prop);
}

// 0x4EBE80 (thiscall): balancing: the player's line (0.65 up, 0.3 ahead) against each
// rail segment; within 0.75 he's put on it facing along it. At the rail's end he's let
// off (within 0.1 of the last point, or 0.25 of the first)
void __fastcall CheckBalance(uint8_t* self) {
    uint8_t* player = gr_playerPed;
    CVec up = {float(gr_up.x * 0.6499999761581421), float(gr_up.y * 0.6499999761581421),
               float(gr_up.z * 0.6499999761581421)};
    CVec top;
    VectorAdd(&top, &Position(player), &up);
    CVec scratch;
    CVec* forward = Entity_GetForward(player, &scratch);
    CVec ahead;
    ahead.y = float(forward->y * 0.30000001192092896);
    ahead.z = float(forward->z * 0.30000001192092896);
    ahead.x = float(0.30000001192092896 * forward->x);
    CVec front;
    VectorAdd(&front, &top, &ahead);
    CVec a = {}, b = {};
    float lineA[8] = {}, lineB[8] = {};
    CVec onA = {}, onB = {};
    uint8_t line[0x20];
    RailPointList* rail = RailOf(self);
    if (Controller_IsNodePlaying(At<uint8_t*>(player, 0x2E0) + 0x258, gr_balanceNode, 1, 1)) {
        return;
    }
    CVec d;
    VectorSub(&d, &Position(player), &Position(self));
    double dist = Length(d);
    // (a null rail is read all the same)
    if (!(Rail_GetRadius(rail) > dist)) {
        return;
    }
    RailPoint* first = Rail_First(rail);
    RailPoint* point = first;
    while (point != nullptr) {
        RailPoint* next = Rail_Next(rail);
        if (next == nullptr) {
            uint8_t* matrix = Ped_GetMatrix(player);
            CVec* pos = reinterpret_cast<CVec*>(matrix + 0x30);
            CVec toEnd;
            VectorSub2(&toEnd, pos, Point_GetPos(point));
            if (!(Length(toEnd) < 0.10000000149011612)) {
                CVec toStart;
                VectorSub2(&toStart, pos, Point_GetPos(first));
                if (!(Length(toStart) < 0.25f)) {
                    return;
                }
            }
            At<int32_t>(player, 0x13C4) = 0;
            At<int32_t>(player, 0x1E98) = 0;
            At<int32_t>(player, 0x1E9C) = 0;
            return;
        }
        a = *Point_GetPos(point);
        b = *Point_GetPos(next);
        std::memcpy(lineA, ColLine_Set(line, &top, &front), sizeof(lineA));
        std::memcpy(lineB, ColLine_Set(line, &a, &b), sizeof(lineB));
        ClosestPoints(lineA, lineB, &onA, &onB);
        CVec gap;
        VectorSub(&gap, &onB, &onA);
        if (Length(gap) < 0.75f) {
            At<int32_t>(player, 0x13C4) = 1;
            CVec pos = onB;
            Matrix_SetTranslate(Ped_GetMatrix(player), &pos);
            CVec dir;
            CVec temp;
            dir.x = VectorSub(&temp, &b, &a)->x;
            dir.y = VectorSub(&temp, &b, &a)->y;
            dir.z = VectorSub(&temp, &b, &a)->z;
            // (the game also writes uninitialised stack into the matrix's pad at +0x1C)
            uint8_t* matrix = Ped_GetMatrix(player);
            At<CVec>(matrix, 0x10) = dir;
            At<uint8_t*>(player, 0x1E98) = self;
            At<RailPoint*>(player, 0x1E9C) = point;
        }
        point = next;
    }
}

// 0x4EC3A0 (thiscall): grinding: the player's upright line (0.2 above and below his
// position) against each segment of a rail within its radius and 0.5 of his height; on
// one (within 0.35, between its ends) he's set up to grind towards the nearer end (or
// along), facing along the rail, and takes the grab action
void __fastcall CheckGrind(uint8_t* self) {
    uint8_t* player = gr_playerPed;
    CVec heading = {0.0f, 0.0f, 0.0f};
    if (At<int32_t>(player, 0x14F8) != 0) {
        At<int32_t>(player, 0x1548) = 1;
        At<CVec>(player, 0x1EB0) = Position(player);
        GrabRail(self, player);
        return;
    }
    CVec a = {}, b = {};
    float lineA[8] = {}, lineB[8] = {};
    CVec onA = {}, onB = {};
    uint8_t line[0x20];
    RailPointList* rail = RailOf(self);
    CVec top = Position(player);
    CVec bottom = top;
    top.z = float(top.z + 0.20000000298023224);
    bottom.z = float(bottom.z - 0.20000000298023224);
    float height = std::fabs(float(Position(player).z - Position(self).z));
    CVec d;
    VectorSub(&d, &Position(player), &Position(self));
    double dist = Length(d);
    if (!(Rail_GetRadius(rail) > dist)) {
        return;
    }
    if (!(0.5f > height)) {
        return;
    }
    uint8_t onRail = 0;
    uint8_t between = 0;
    uint8_t near = 0;
    RailPoint* point = Rail_First(rail);
    if (point == nullptr) {
        At<int32_t>(player, 0x1548) = 0;
        return;
    }
    while (point != nullptr) {
        RailPoint* next = Rail_Next(rail);
        if (next == nullptr) {
            break;
        }
        a = *Point_GetPos(point);
        b = *Point_GetPos(next);
        std::memcpy(lineA, ColLine_Set(line, &top, &bottom), sizeof(lineA));
        std::memcpy(lineB, ColLine_Set(line, &a, &b), sizeof(lineB));
        ClosestPoints(lineA, lineB, &onA, &onB);
        CVec edge;
        VectorSub(&edge, &b, &a);
        float edgeLength = Length(edge);
        CVec toA;
        VectorSub(&toA, &a, &onA);
        CVec toB;
        VectorSub(&toB, &b, &onA);
        if (Length(toA) <= edgeLength && Length(toB) <= edgeLength) {
            between = 1;
        }
        CVec gap;
        VectorSub(&gap, &onB, &onA);
        if (Length(gap) < 0.35f) {
            near = 1;
        }
        // (between and near stay set from earlier segments)
        if (between && near) {
            onRail = 1;
            if (At<int32_t>(player, 0x131C) != 0) {
                At<int32_t>(player, 0x131C) = 0;
            }
        } else {
            onRail = 0;
        }
        if (!onRail) {
            point = next;
            continue;
        }
        if (At<int32_t>(player, 0x1548) != 0) {
            // already grinding: only nearer than the current target
            CVec gap2;
            VectorSub(&gap2, &onB, &onA);
            CVec toTarget;
            VectorSub(&toTarget, &At<CVec>(player, 0x1EA0), &onA);
            if (!(Length(gap2) <= Length(toTarget))) {
                point = next;
                continue;
            }
        }
        CVec ends[2];
        VectorConstruct(ends, 0xC, 2, gr_vectorConstruct);
        At<int32_t>(player, 0x1548) = 1;
        CVec temp;
        ends[0] = *VectorSub(&temp, &onB, &a);
        ends[1] = *VectorSub(&temp, &onB, &b);
        int end = Length(ends[0]) < Length(ends[1]) ? 0 : 1;
        CVec target;
        if (Length(ends[end]) < 0.6f) {
            CVec dir = ends[end];
            Vector_Normalise(&dir);
            CVec step;
            step.x = float(dir.x * 0.6000000238418579);
            step.y = float(dir.y * 0.6000000238418579);
            step.z = float(0.6000000238418579 * dir.z);
            const CVec& from = end ? b : a;
            target.x = step.x + from.x;
            target.y = from.y + step.y;
            target.z = from.z + step.z;
        } else {
            target = onB;
        }
        At<CVec>(player, 0x1EA0) = target;
        float playerHeading = Entity_GetHeading(player);
        float turnForward = std::fabs(AngleDifference(Vector_Heading(VectorSub(&temp, &a, &b)), playerHeading));
        playerHeading = Entity_GetHeading(player);
        float turnBack = std::fabs(AngleDifference(Vector_Heading(VectorSub(&temp, &b, &a)), playerHeading));
        CVec along;
        if (turnForward <= turnBack) {
            VectorSub(&along, &a, &b);
        } else {
            VectorSub(&along, &b, &a);
        }
        At<float>(player, 0x1EAC) = Vector_Heading(&along);
        At<CVec>(player, 0x1EB0) = onA;
        uint8_t* matrix = Ped_GetMatrix(player);
        VectorFromHeading(&heading, At<float>(player, 0x1EAC));
        CVec zAxis = {0.0f, 0.0f, 1.0f};
        CVec side;
        CrossProduct(&side, &heading, &zAxis);
        // (the game also writes uninitialised stack into the pads at +0x1C, +0x2C, +0x3C)
        At<CVec>(matrix, 0x30) = At<CVec>(player, 0x1EA0);
        At<CVec>(matrix, 0x20) = zAxis;
        float forward[4] = {heading.x, heading.y, heading.z, At<float>(matrix, 0x1C)};
        Vector_NormaliseAndMag(forward);
        At<float>(matrix, 0x10) = forward[0];
        At<float>(matrix, 0x14) = forward[1];
        At<float>(matrix, 0x18) = forward[2];
        float right[4] = {side.x, side.y, side.z, 0.0f};
        Vector_NormaliseAndMag(right);
        Vector_Assign(matrix, right);
        GrabRail(self, player);
        point = next;
    }
    if (!onRail) {
        At<int32_t>(player, 0x1548) = 0;
    }
}

// 0x4F0260 (thiscall): RailPoint's constructor
RailPoint* __fastcall Point_Construct(RailPoint* self, re::Edx, float x, float y, float z) {
    self->pos.x = 0.0f;
    self->vtable = gr_pointVtable;
    self->pos.y = 0.0f;
    self->pos.z = 0.0f;
    self->pos.x = x;
    self->pos.y = y;
    self->pos.z = z;
    return self;
}

// 0x4F0290 (thiscall): its destructor
void __fastcall Point_Destruct(RailPoint* self) {
    self->vtable = gr_pointVtable;
}

// 0x4F02A0 (thiscall, 4 bytes)
CVec* __fastcall Point_GetPosition(RailPoint* self) {
    return &self->pos;
}
RE_UNHOOKED_FN("RailPoint::GetPos", 0x004F02A0, &Point_GetPosition);

// 0x4F02B0 (thiscall): RailPointList's constructor
RailPointList* __fastcall List_Construct(RailPointList* self, re::Edx, int first) {
    self->vtable = gr_listVtable;
    self->first = uint16_t(first);
    self->cursor = 0xFFFF;
    return self;
}

// 0x4F02D0 (thiscall, 4 bytes)
float __fastcall List_GetRadius(RailPointList* self) {
    return self->radius;
}
RE_UNHOOKED_FN("RailPointList::GetRadius", 0x004F02D0, &List_GetRadius);

// 0x4F02E0 (thiscall): the rail pool's New (the first free slot from the last used,
// wrapping once; the object's first 12 bytes cleared)
RailPointList* __fastcall RailPool_NewSlot(PoolHeader* self) {
    uint8_t* flags = self->flags;
    bool wrapped = false;
    while (!(flags[self->lastUsed] & 0x80)) {
        self->lastUsed++;
        if (self->lastUsed >= self->size) {
            if (wrapped) {
                return nullptr;
            }
            self->lastUsed = 0;
            wrapped = true;
        }
    }
    flags[self->lastUsed] &= 0x7F;
    uint8_t& flag = self->flags[self->lastUsed];
    flag = uint8_t(flag ^ ((uint8_t(flag + 1) ^ flag) & 0x7F));
    uint32_t* object = reinterpret_cast<uint32_t*>(self->objects + self->elemSize * self->lastUsed);
    object[0] = 0;
    object[1] = 0;
    object[2] = 0;
    return reinterpret_cast<RailPointList*>(object);
}

// 0x4F0350: a new point from its pool
void* __cdecl NewPoint() {
    return PointPool_New(gr_pointPool);
}

// 0x4F0360: a new point list from its pool
RailPointList* __cdecl NewList() {
    return RailPool_New(gr_railPool);
}

RailPoint* PointAt(uint32_t index) {
    PoolHeader* pool = gr_pointPool;
    if (pool->flags[index] & 0x80) {
        return nullptr;
    }
    return reinterpret_cast<RailPoint*>(pool->objects + pool->elemSize * index);
}

// 0x4F0370 (thiscall): the first point (the walk set up after it)
RailPoint* __fastcall List_First(RailPointList* self) {
    if (self->first == 0xFFFF) {
        return nullptr;
    }
    uint32_t index = self->first;
    self->cursor = gr_links[index];
    return PointAt(index);
}

// 0x4F03B0 (thiscall): the walk's next point
RailPoint* __fastcall List_Next(RailPointList* self) {
    uint16_t index = self->cursor;
    if (index == 0xFFFF || index == 0xFFFE) {
        return nullptr;
    }
    self->cursor = gr_links[index];
    return PointAt(index);
}

// 0x4F0400 (thiscall): the point after `point` (null when it's not in the list)
RailPoint* __fastcall List_NextAfter(RailPointList* self, re::Edx, RailPoint* point) {
    RailPoint* current = nullptr;
    if (self->first != 0xFFFF) {
        uint32_t index = self->first;
        self->cursor = gr_links[index];
        current = PointAt(index);
    }
    while (current != point) {
        uint16_t index = self->cursor;
        if (index == 0xFFFF || index == 0xFFFE) {
            return nullptr;
        }
        self->cursor = gr_links[index];
        current = PointAt(index);
        if (current == nullptr) {
            return nullptr;
        }
    }
    return Rail_Next(self);
}

// 0x4F04A0 (thiscall): the points' box centre, the heading from the first point to the
// last, and the radius (half the box's diagonal plus 3)
void __fastcall List_GetBounds(RailPointList* self, re::Edx, CVec* centre, float* heading) {
    CVec firstPos = {}, lastPos = {};
    CVec minimum = {3.4028235e38f, 3.4028235e38f, 3.4028235e38f};
    CVec maximum = {-3.4028235e38f, -3.4028235e38f, -3.4028235e38f};
    // (an empty list is read through null, as the game does)
    RailPoint* point = Rail_First(self);
    firstPos = point->pos;
    const CVec* pos;
    do {
        pos = &point->pos;
        minimum.x = !(pos->x > minimum.x) ? pos->x : minimum.x;
        minimum.y = !(pos->y > minimum.y) ? pos->y : minimum.y;
        minimum.z = !(pos->z > minimum.z) ? pos->z : minimum.z;
        maximum.x = pos->x < maximum.x ? maximum.x : pos->x;
        maximum.y = pos->y < maximum.y ? maximum.y : pos->y;
        maximum.z = pos->z < maximum.z ? maximum.z : pos->z;
        point = Rail_Next(self);
    } while (point != nullptr);
    lastPos = *pos;
    CVec size;
    VectorSub(&size, &maximum, &minimum);
    CVec half;
    half.y = float(size.y * 0.5);
    half.z = float(size.z * 0.5);
    half.x = float(0.5 * size.x);
    CVec middle;
    *centre = *VectorAdd(&middle, &minimum, &half);
    *heading = HeadingBetween(&firstPos, &lastPos);
    self->radius = float(double(Length(half)) + 3.0);
}

// 0x4F0770 (thiscall): deletes the points and frees their chain
void __fastcall List_DeleteAll(RailPointList* self) {
    if (self->first == 0xFFFF) {
        return;
    }
    PoolHeader* pool = gr_pointPool;
    uint32_t index = self->first;
    for (;;) {
        uint16_t next = gr_links[index];
        if (!(pool->flags[index] & 0x80)) {
            RailPoint* point = reinterpret_cast<RailPoint*>(pool->objects + pool->elemSize * index);
            if (point != nullptr) {
                VCall<void>(point, 0, 1);
            }
        }
        if (next == 0xFFFE) {
            break;
        }
        index = next;
    }
    LinkList_FreeChain(gr_linkList, self->first);
    self->first = 0xFFFF;
}

void PoolDelete(PoolHeader* pool, uint8_t* object) {
    int slot = int((object - pool->objects) / pool->elemSize);
    pool->flags[slot] |= 0x80;
    if (slot < pool->lastUsed) {
        pool->lastUsed = slot;
    }
}

// 0x4F07E0: RailPoint's operator delete
void __cdecl Point_Delete(uint8_t* point) {
    PoolDelete(gr_pointPool, point);
}

// 0x4F0810 (thiscall): RailPointList's destructor
void __fastcall List_Destruct(RailPointList* self) {
    self->vtable = gr_listVtable;
    Rail_DeleteAll(self);
}

// 0x4F0820: RailPointList's operator delete
void __cdecl List_Delete(uint8_t* list) {
    PoolDelete(gr_railPool, list);
}

#define C13C_RAIL_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C13C_RAIL_REVERSED("CPropAnim::CheckBalance", 0x004EBE80, &CheckBalance);
C13C_RAIL_REVERSED("CPropAnim::CheckGrind", 0x004EC3A0, &CheckGrind);
C13C_RAIL_REVERSED("RailPoint::RailPoint", 0x004F0260, &Point_Construct);
C13C_RAIL_REVERSED("RailPoint::~RailPoint", 0x004F0290, &Point_Destruct);
C13C_RAIL_REVERSED("RailPointList::RailPointList", 0x004F02B0, &List_Construct);
C13C_RAIL_REVERSED("CPool<RailPointList>::New", 0x004F02E0, &RailPool_NewSlot);
C13C_RAIL_REVERSED("RailLoaderImpl::NewPoint", 0x004F0350, &NewPoint);
C13C_RAIL_REVERSED("RailLoaderImpl::NewList", 0x004F0360, &NewList);
C13C_RAIL_REVERSED("RailPointList::First", 0x004F0370, &List_First);
C13C_RAIL_REVERSED("RailPointList::Next", 0x004F03B0, &List_Next);
C13C_RAIL_REVERSED("RailPointList::NextAfter", 0x004F0400, &List_NextAfter);
C13C_RAIL_REVERSED("RailPointList::GetBounds", 0x004F04A0, &List_GetBounds);
C13C_RAIL_REVERSED("RailPointList::DeleteAll", 0x004F0770, &List_DeleteAll);
C13C_RAIL_REVERSED("RailPoint::operator delete", 0x004F07E0, &Point_Delete);
C13C_RAIL_REVERSED("RailPointList::~RailPointList", 0x004F0810, &List_Destruct);
C13C_RAIL_REVERSED("RailPointList::operator delete", 0x004F0820, &List_Delete);

} // namespace
