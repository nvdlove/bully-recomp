// More of the ped paths (see PathNodes.cpp): the constructor, the node away from a point,
// a lane's position, dropping a passed node from a route, and the ped creation point on
// a link (GTA's GeneratePedCreationCoors).

#include <cmath>
#include <cstdint>
#include <cstring>

#include "game/Collision.h"
#include "game/PathNodes.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {
using namespace paths;

inline void* const CPathNode_vftable = reinterpret_cast<void*>(0x009050E0);

inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(col::CVector*, __thiscall, Node_GetPosition, 0x00417C00, const void* node, col::CVector* out);
GAME_FN(col::CVector*, __cdecl, VectorSub, 0x00414B70, col::CVector* out, const col::CVector* a, const col::CVector* b);
GAME_FN(col::CVector*, __cdecl, VectorAdd, 0x004129A0, col::CVector* out, const col::CVector* a, const col::CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, col::CVector* v);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const col::CVector* a, const col::CVector* b);
GAME_FN(PathNode*, __thiscall, Node_GetLinkNode, 0x0043F7E0, PathNode* node, int i);
GAME_FN(int, __cdecl, RandomInt, 0x0040DA80, int low, int high);
GAME_FN(float, __cdecl, RandomFloat, 0x0041A5B0, float low, float high);
GAME_FN(uint8_t, __cdecl, Fn_48C9F0, 0x0048C9F0, PathNode* a, PathNode* b);
GAME_FN(uint8_t, __cdecl, Fn_5EB7D0, 0x005EB7D0, const col::CVector* point, int, uintptr_t);
GAME_FN(uint8_t, __cdecl, Fn_48C990, 0x0048C990, const col::CVector* point, float radius, int, uintptr_t);
GAME_VAR(col::CVector, ms_zeroVector, 0x00BCC564);
GAME_FN(void, __thiscall, Node_SetPrev, 0x0043FD20, PathNode* node, const PathNode* entry);
GAME_FN(void, __thiscall, Node_SetNext, 0x0043FD90, PathNode* node, const PathNode* entry);
GAME_FN(int, __thiscall, Node_FindNaviLinkTo, 0x0043F850, PathNode* node, int other);
GAME_FN(col::CVector*, __cdecl, CrossProduct, 0x00414730, col::CVector* out, const col::CVector* a, const col::CVector* b);
GAME_FN(uint8_t, __thiscall, Fn_46EFB0, 0x0046EFB0, void* self, void* ped, const col::CVector* pos, int);
inline void* const ms_collisionWorld = reinterpret_cast<void*>(0x00C2AB70);

col::CVector NodePos(const PathNode* node) {
    return {float(node->pos[0]) * 0.125f, float(node->pos[1]) * 0.125f, float(node->pos[2]) * 0.125f};
}

// (x87: fabs of the float sum, then the CRT's sqrt (0x85C500, its argument in st0))
float Distance2D(const col::CVector& d) {
    float sum = float(double(d.x) * d.x + double(d.y) * d.y);
    return float(std::sqrt(double(std::fabs(sum))));
}

// a node's or an extra entry's reset (the constructor's)
void ResetEntry(uint8_t* e) {
    e[0x15] &= 0xC0;
    e[0x16] &= 0xF0;
    *reinterpret_cast<re::Ptr32<void>*>(e) = CPathNode_vftable;
    for (int i = 0; i < 5; i++) {
        reinterpret_cast<int16_t*>(e + 4)[i] = -1;
    }
    *reinterpret_cast<int16_t*>(e + 0xE) = 0x7FFE;
    *reinterpret_cast<int16_t*>(e + 0x10) = -1;
    e[0x12] = 0;
    e[0x13] = 0;
    e[0x14] = 0;
}

} // namespace

namespace PathNodes2 {

// 0x4401C0 (thiscall): the linked node most away from the point (the smallest cosine
// under 1 between here->point and here->node; `exclude` when there's none)
static PathNode* __fastcall FindNodeAwayFrom(PathNode* self, re::Edx, float x, float y, float z, PathNode* exclude) {
    int n = self->NumLinks();
    if (n == 0) {
        return nullptr;
    }
    if (n == 1) {
        return NodeById(ms_linkNodes[self->firstLink] & 0x3FFF);
    }
    col::CVector point = {x, y, z};
    col::CVector here = NodePos(self);
    col::CVector toPoint;
    VectorSub(&toPoint, &point, &here);
    Vector_Normalise(&toPoint);
    float best = 1.0f;
    PathNode* result = exclude;
    for (int i = 0; i < self->NumLinks(); i++) {
        PathNode* node = i < self->NumLinks() ? NodeById(ms_linkNodes[self->firstLink + i] & 0x3FFF) : nullptr;
        if (node == exclude) {
            continue;
        }
        col::CVector from = NodePos(self);
        col::CVector to = NodePos(node);
        col::CVector dir;
        VectorSub(&dir, &to, &from);
        Vector_Normalise(&dir);
        // (x87: y, x, z added in extended precision)
        float dot = float(double(toPoint.y) * dir.y + double(dir.x) * toPoint.x + double(toPoint.z) * dir.z);
        if (best > dot) {
            best = dot;
            result = node;
        }
    }
    return result;
}

// 0x4403C0 (thiscall on a navi link, its +0/+2 the position in eighths): the point `lane`
// lanes (and half the width) across the direction (dx, dy)
static col::CVector* __fastcall GetLanePosition(const uint8_t* self, re::Edx, col::CVector* out, float dx, float dy, int lane, float scale) {
    float width = float(double(self[0xA]) / 80.0 + 0.5);
    float offset = float((double(width) + lane) * scale);
    float x = float(*reinterpret_cast<const int16_t*>(self) * 0.125);
    out->x = float(double(dy) * offset + x);
    float y = float(*reinterpret_cast<const int16_t*>(self + 2) * 0.125);
    out->y = float(double(y) - double(dx) * offset);
    out->z = 0.0f;
    return out;
}

// 0x440450 (thiscall): the constructor: 550 navi links, the nodes and the extra entries
// reset, the link arrays cleared (the navi ids to -1)
static uint8_t* __fastcall Construct(uint8_t* self, re::Edx) {
    for (int i = 0; i < 550; i++) {
        uint8_t* link = self + i * 0xC;
        link[8] &= 0x80;
        link[9] &= 0xFC;
        reinterpret_cast<int16_t*>(link)[0] = -1;
        reinterpret_cast<int16_t*>(link)[1] = -1;
        reinterpret_cast<int16_t*>(link)[2] = -1;
        link[6] = 0xFF;
        link[7] = 0xFF;
        link[0xA] = 0;
    }
    for (int i = 0; i < 0x6D6; i++) {
        ResetEntry(self + 0x19C8 + i * 0x18);
    }
    *reinterpret_cast<int32_t*>(self + 0x10718) = -1;
    *reinterpret_cast<int32_t*>(self + 0x1071C) = -1;
    *reinterpret_cast<int32_t*>(self + 0x10720) = -1;
    *reinterpret_cast<int16_t*>(self + 0x10726) = -1;
    *reinterpret_cast<int32_t*>(self + 0x10728) = -1;
    *reinterpret_cast<int32_t*>(self + 0x1072C) = -1;
    for (int i = 0; i < 0x200; i++) {
        ResetEntry(self + 0x10734 + i * 0x18);
    }
    std::memset(self + 0xBDD8, 0, 0x1D4C);
    std::memset(self + 0xDB24, 0, 0xEA6);
    std::memset(self + 0xE9CA, 0xFF, 0x1D4C);
    return self;
}

// 0x440780 (thiscall, on 0xBF6228): the route's first node dropped once the point is
// between the first two (the dot product of point->first and point->second below 0)
static void __fastcall DropPassedNode(void*, re::Edx, float x, float y, float, re::Ptr32<PathNode>* nodes, int16_t* count) {
    int16_t n = *count;
    if (n < 2) {
        return;
    }
    const PathNode* a = nodes[0];
    const PathNode* b = nodes[1];
    float ay = float(double(a->pos[1]) * 0.125);
    float dy0 = float(double(ay) - y);
    float by = float(double(b->pos[1]) * 0.125);
    float dy1 = float(double(by) - y);
    float ax = float(double(a->pos[0]) * 0.125);
    float dx0 = float(double(ax) - x);
    float bx = float(double(b->pos[0]) * 0.125);
    float dx1 = float(double(bx) - x);
    if (!(double(dy0) * dy1 + double(dx0) * dx1 < 0.0)) {
        return;
    }
    *count = int16_t(n - 1);
    for (int16_t i = 0; i < *count; i++) {
        nodes[i] = nodes[i + 1];
    }
}

// 0x440870 (thiscall): a ped creation point: from a random node on (500 tries), a node
// within max(1.7r, r + 70) (2D) and a link with lanes our way whose ends are either side
// of the radius r (1.5r at water nodes); the point between them on the circle, then
// (useLinkDir) the cosine of point->link and the link or (else, within r) of point and
// `forward` against the threshold (above it when `above`, else at most)
static uint8_t __fastcall GenerateCreationPoint(uint8_t* self, re::Edx, float x, float y, float z, float fx, float fy, float fz,
                                                float radius, float threshold, int above, col::CVector* outPos,
                                                re::Ptr32<PathNode>* outNode, re::Ptr32<PathNode>* outOther, float* outFraction,
                                                int includeSwitchedOff, int useLinkDir) {
    int32_t count = *reinterpret_cast<int32_t*>(self + 0x1071C);
    if (count == 0) {
        return 0;
    }
    col::CVector pos = {x, y, z};
    col::CVector forward = {fx, fy, fz};
    float near = float(double(radius) + 70.0);
    float far = float(double(radius) * 1.7000000476837158);
    float maxDist = (far > near || far != far) ? far : near;
    int start = Rand() % count;
    int i = start;
    for (int tries = 0; tries < 500; tries++) {
        i++;
        if (i >= *reinterpret_cast<int32_t*>(self + 0x1071C)) {
            i = 0;
        }
        if (i == start) {
            return 0;
        }
        PathNode* node = uint32_t(i) <= 0x6D5 ? reinterpret_cast<PathNode*>(self + 0x19C8 + i * 0x18) : nullptr;
        if ((node->flags14 & 0x60) && !re::Flag(includeSwitchedOff)) {
            continue;
        }
        if ((node->flags14 & 0xF) <= 1) {
            continue;
        }
        col::CVector nodePos = NodePos(node);
        col::CVector d;
        float dist = Distance2D(*VectorSub(&d, &nodePos, &pos));
        if (maxDist < dist) {
            continue;
        }
        int links = node->flags14 & 0xF;
        for (int j = 0; j < links; j++) {
            int n = node->flags14 & 0xF;
            PathNode* other = j < n ? NodeById(ms_linkNodes[node->firstLink + j] & 0x3FFF) : nullptr;
            if ((other->flags14 & 0x60) && !re::Flag(includeSwitchedOff)) {
                continue;
            }
            if ((other->flags14 & 0xF) <= 1) {
                continue;
            }
            NaviLink* navi = nullptr;
            if (j < n) {
                int id = ms_naviLinkIds[node->firstLink + j];
                if (id >= 0 && id < ms_numNaviLinks) {
                    navi = &ms_naviLinks[id];
                }
            }
            uint8_t lanes = navi->node == NodeIndex(node) ? (navi->lanes & 0x38) : (navi->lanes & 7);
            if (lanes == 0) {
                continue;
            }
            col::CVector otherPos = NodePos(other);
            col::CVector e;
            float dist2 = Distance2D(*VectorSub(&e, &otherPos, &pos));
            double r = (node->flags15 & 2) ? double(radius) * 1.5 : double(radius);
            float a = float(double(dist) - r);
            float b = float(double(dist2) - r);
            if (!(double(a) * b < 0.0)) {
                continue;
            }
            float fa = std::fabs(a);
            float fb = std::fabs(b);
            float fraction = float(double(fa) / (double(fb) + fa));
            *outFraction = fraction;
            col::CVector p;
            Node_GetPosition(other, &p);
            col::CVector towards = {fraction * p.x, fraction * p.y, p.z * fraction};
            float rest = 1.0f - fraction;
            Node_GetPosition(node, &p);
            col::CVector from = {rest * p.x, rest * p.y, p.z * rest};
            col::CVector point;
            VectorAdd(&point, &from, &towards);
            col::CVector diff;
            VectorSub(&diff, &point, &pos);
            float dot;
            if (re::Flag(useLinkDir)) {
                col::CVector p1;
                col::CVector p2;
                col::CVector link;
                col::CVector* fromNode = Node_GetPosition(node, &p1);
                col::CVector* toNode = Node_GetPosition(other, &p2);
                VectorSub(&link, toNode, fromNode);
                diff.z = 0.0f;
                Vector_Normalise(&diff);
                link.z = 0.0f;
                Vector_Normalise(&link);
                dot = float(double(diff.y) * link.y + double(link.x) * diff.x + double(link.z) * diff.z);
            } else {
                float lengthSq = float(double(diff.x) * diff.x + double(diff.y) * diff.y);
                if (double(lengthSq) > double(radius) * radius) {   // (r * r not rounded)
                    return 0;
                }
                diff.z = 0.0f;
                Vector_Normalise(&diff);
                dot = DotProduct(&diff, &forward);
            }
            bool pass = re::Flag(above) ? threshold < dot : threshold >= dot;
            if (pass) {
                *outNode = node;
                *outOther = other;
                *outPos = point;
                return 1;
            }
        }
    }
    return 0;
}

// 0x440E10 (thiscall): the nodes of a range (kind 0: the first +0x1071C, 1: from there to
// +0x10718, else none) between minDist and maxDist of the point (2D) and within maxDz of
// its height, into `out` (cleared first) up to `capacity`
static void __fastcall FindNodesInRange(uint8_t* self, re::Edx, float x, float y, float z, float maxDist, float minDist,
                                        float maxDz, int kind, int capacity, re::Ptr32<PathNode>* out, int* count) {
    if (capacity == 0) {
        return;
    }
    std::memset(out, 0, capacity * 4);
    *count = 0;
    int start;
    int end;
    if (uint8_t(kind) == 0) {
        end = *reinterpret_cast<int32_t*>(self + 0x1071C);
        start = 0;
    } else if (uint8_t(kind) == 1) {
        end = *reinterpret_cast<int32_t*>(self + 0x10718);
        start = *reinterpret_cast<int32_t*>(self + 0x1071C);
    } else {
        start = kind;   // (the whole word: no nodes)
        end = kind;
    }
    col::CVector pos = {x, y, z};
    for (int i = start; i < end; i++) {
        PathNode* node = reinterpret_cast<PathNode*>(self + 0x19C8 + i * 0x18);
        if (uint32_t(i) > 0x6D5 || !node) {
            continue;
        }
        col::CVector nodePos = NodePos(node);
        col::CVector d;
        VectorSub(&d, &nodePos, &pos);
        float dist = Distance2D(d);
        if (!(maxDist >= dist)) {   // (NaN: out)
            continue;
        }
        if (!(minDist <= dist)) {
            continue;
        }
        if (!(maxDz > std::fabs(d.z))) {
            continue;
        }
        out[*count] = node;
        (*count)++;
        if (*count >= capacity) {
            return;
        }
    }
}

// 0x440FB0 (thiscall): a spawn point on a link of one of the nodes (a random one of
// them): from a random link on, links not switched off that 0x48C9F0 allows; `steps`
// points along the link from a random place on, the first that passes 0x5EB7D0 (when
// `checkArea`), 0x48C990 and the callback. The error codes: -2 no nodes loaded, -3 no
// nodes given, -4 none picked, -5 switched off, -6 no links, -1 none found
static uint8_t __fastcall FindSpawnPoint(uint8_t* self, re::Edx, int, int, int, int, int steps, col::CVector* out,
                                         int* error, re::Ptr32<PathNode>* nodes, int numNodes,
                                         uint8_t(__cdecl* callback)(col::CVector*, void*), void* context,
                                         int checkArea, re::Ptr32<PathNode>* outOther) {
    float step = float(1.0 / double(steps));
    if (*reinterpret_cast<int32_t*>(self + 0x10720) == 0) {
        *error = -2;
        return 0;
    }
    if (numNodes == 0) {
        *error = -3;
        return 0;
    }
    col::CVector from = ms_zeroVector;
    col::CVector to = ms_zeroVector;
    col::CVector along = ms_zeroVector;
    col::CVector point = ms_zeroVector;
    PathNode* node = nodes[Rand() * (numNodes - 1) / 0x7FFF];
    if (!node) {
        *error = -4;
        return 0;
    }
    if (node->flags14 & 0x60) {
        *error = -5;
        return 0;
    }
    int n = node->flags14 & 0xF;
    if (n == 0) {
        *error = -6;
        return 0;
    }
    col::CVector tmp;
    from = *Node_GetPosition(node, &tmp);
    int k = RandomInt(0, n);
    for (int j = 0; j < n; j++, k++) {
        if (k >= n) {
            k -= n;
        }
        if (*reinterpret_cast<uint16_t*>(self + 0xBDD8 + (node->firstLink + k) * 2) & 0x8000) {
            continue;
        }
        PathNode* other = Node_GetLinkNode(node, k);
        if (!other || (other->flags14 & 0x60) || !Fn_48C9F0(node, other)) {
            continue;
        }
        if (outOther) {
            *outOther = other;
        }
        to = *Node_GetPosition(other, &tmp);
        float t = RandomFloat(0.0f, 1.0f);
        for (int i = 0; i < steps; i++) {
            col::CVector d = *VectorSub(&tmp, &to, &from);
            along.x = float(double(t) * d.x);
            along.y = float(double(d.y) * t);
            along.z = float(double(d.z) * t);
            t = float(double(t) + step);
            if (!(t < 1.0f)) {
                t = float(double(t) - 1.0);
            }
            point = *VectorAdd(&tmp, &from, &along);
            if (re::Flag(checkArea) && !Fn_5EB7D0(&point, 1, 0)) {
                continue;
            }
            if (!Fn_48C990(&point, 1.0f, -1, 0)) {
                continue;
            }
            if (callback && !callback(&point, context)) {
                continue;
            }
            *out = point;
            return 1;
        }
    }
    *error = -1;
    return 0;
}

// 0x441330 (thiscall): the road density round (x, y): the links' lanes times their lengths
// for the nodes within 80 (each axis), over 2500
static float __fastcall CalcRoadDensity(uint8_t* self, re::Edx, float x, float y) {
    float sum = 0.0f;
    int count = *reinterpret_cast<int32_t*>(self + 0x1071C);
    for (int i = 0; i < count; i++) {
        PathNode* node = reinterpret_cast<PathNode*>(self + 0x19C8 + i * 0x18);
        float x8 = float(node->pos[0] * 0.125);
        if (!(std::fabs(float(double(x8) - x)) < 80.0f)) {
            continue;
        }
        float y8 = float(node->pos[1] * 0.125);
        if (!(std::fabs(float(double(y8) - y)) < 80.0f)) {
            continue;
        }
        int n = node->flags14 & 0xF;
        if (n == 0) {
            continue;
        }
        const int16_t* navi = reinterpret_cast<const int16_t*>(self + 0xE9CA) + node->firstLink;
        const uint16_t* links = reinterpret_cast<const uint16_t*>(self + 0xBDD8) + node->firstLink;
        for (int j = 0; j < n; j++) {
            const PathNode* other = reinterpret_cast<const PathNode*>(self + 0x19C8 + (links[j] & 0x3FFF) * 0x18);
            float dx = float(double(x8) - float(other->pos[0] * 0.125));
            float dy = float(double(y8) - float(other->pos[1] * 0.125));
            float length = Distance2D({dx, dy, 0.0f});
            uint8_t lanes = self[navi[j] * 0xC + 8];
            float partial = float(double(sum) + double(lanes & 7) * length);
            sum = float(double(partial) + double(length) * ((lanes >> 3) & 7));
        }
    }
    return float(double(sum) / 2500.0);
}

// a search list reference (below 0x200: a bucket at 0xC0695C; else a node + 0x200)
static PathNode* ListEntry(int16_t ref) {
    if (ref < 0) {
        return nullptr;
    }
    if (ref < 0x200) {
        return reinterpret_cast<PathNode*>(0x00C0695C + ref * 0x18);
    }
    return NodeById(uint32_t(ref - 0x200));
}

// 0x441540 (thiscall, on 0xBF6228; unused): the node out of its search list
static void __fastcall RemoveNodeFromList(void*, re::Edx, PathNode* node) {
    PathNode* next = ListEntry(node->next);
    Node_SetNext(ListEntry(node->prev), next);
    if (node->next >= 0 && ListEntry(node->next)) {
        Node_SetPrev(ListEntry(node->next), ListEntry(node->prev));
    }
}

// 0x441690 (thiscall): the node into the search list of its distance (the bucket of the
// distance and 0x1FF), at the front
static void __fastcall AddNodeToList(uint8_t* self, re::Edx, PathNode* node, int distance) {
    PathNode* bucket = reinterpret_cast<PathNode*>(self + 0x10734 + int16_t(distance & 0x1FF) * 0x18);
    Node_SetNext(node, ListEntry(bucket->next));
    Node_SetPrev(node, bucket);
    if (bucket->next >= 0 && ListEntry(bucket->next)) {
        Node_SetPrev(ListEntry(bucket->next), node);
    }
    Node_SetNext(bucket, node);
    node->distance = int16_t(distance);
}

// 0x441790 (thiscall): the offset across the link from the second node to the first
// (right of it, 2.25 on one-lane links, else 0.75 per lane from); zero without a navi
// link. (The navi id is looked up twice: FindNaviLinkTo returns one already)
static col::CVector* __fastcall GetLinkSideOffset(void*, re::Edx, col::CVector* out, PathNode* from, PathNode* to) {
    col::CVector a = NodePos(to);
    col::CVector b = NodePos(from);
    col::CVector dir;
    VectorSub(&dir, &b, &a);
    Vector_Normalise(&dir);
    int id = Node_FindNaviLinkTo(to, NodeIndex(from));
    NaviLink* navi = nullptr;
    if (id != -1) {
        int twice = ms_naviLinkIds[id];
        if (twice >= 0 && twice < ms_numNaviLinks) {
            navi = &ms_naviLinks[twice];
        }
    }
    if (!navi) {
        *out = ms_zeroVector;
        return out;
    }
    col::CVector up = {0.0f, 0.0f, 1.0f};
    col::CVector side;
    CrossProduct(&side, &dir, &up);
    uint8_t lanes = navi->lanes;
    float scale;
    if (int((lanes >> 3) & 7) + int(lanes & 7) <= 1) {
        scale = 2.25f;
    } else {
        scale = float(double((lanes >> 3) & 7) * 5.0 * 0.15000000596046448);
    }
    out->x = side.x * scale;
    out->y = side.y * scale;
    out->z = scale * side.z;
    return out;
}

// 0x441950 (thiscall): the node closest to the point (x + y + 3 z distances) of a range
// (kind 0: the first +0x1071C, 1: from there to +0x10718), with the given flags, that
// 0x46EFB0 doesn't reject for the ped and 0x48C990 accepts; nullptr unless within maxDist
static PathNode* __fastcall FindNodeClosestToCoors(uint8_t* self, re::Edx, void* ped, float x, float y, float z, int kind,
                                                   float maxDist, int allowSwitchedOff, int allowBetweenLevels,
                                                   int allowFlag8, int water) {
    int start;
    int end;
    if (uint8_t(kind) == 0) {
        start = 0;
        end = *reinterpret_cast<int32_t*>(self + 0x1071C);
    } else if (uint8_t(kind) == 1) {
        start = *reinterpret_cast<int32_t*>(self + 0x1071C);
        end = *reinterpret_cast<int32_t*>(self + 0x10718);
    } else {
        start = kind;
        end = kind;
    }
    float best = 10000.0f;
    PathNode* bestNode = nullptr;
    for (int i = start; i < end; i++) {
        PathNode* node = uint32_t(i) > 0x6D5 ? nullptr : reinterpret_cast<PathNode*>(self + 0x19C8 + i * 0x18);
        if (!re::Flag(allowSwitchedOff) && (node->flags14 & 0x60)) {
            continue;
        }
        if (!re::Flag(allowBetweenLevels) && (node->flags14 & 0x80)) {
            continue;
        }
        if (!re::Flag(allowFlag8) && (node->flags15 & 8)) {
            continue;
        }
        if (uint8_t(water) != ((node->flags15 >> 1) & 1)) {
            continue;
        }
        if ((node->flags14 & 0xF) <= 1) {
            continue;
        }
        col::CVector pos = NodePos(node);
        float dx = std::fabs(float(double(pos.x) - x));
        float dy = std::fabs(float(double(pos.y) - y));
        float dz = std::fabs(float(double(pos.z) - z));
        float dist = float(double(dx) + dy + double(dz) * 3.0);
        if (!(best > dist)) {
            continue;
        }
        if (Fn_46EFB0(ms_collisionWorld, ped, &pos, 1)) {
            continue;
        }
        col::CVector tmp;
        if (Fn_48C990(Node_GetPosition(node, &tmp), 1.0f, -1, 0)) {
            best = dist;
            bestNode = node;
        }
    }
    if (!(maxDist > best)) {
        return nullptr;
    }
    return bestNode;
}

} // namespace PathNodes2

// ---- registration ----------------------------------------------------------------------

#define PATHNODES2_REVERSED(name, address, fn)                                              \
    static void* RE_CONCAT(Original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace PathNodes2 {
PATHNODES2_REVERSED("CPathNode::FindNodeAwayFrom", 0x004401C0, &FindNodeAwayFrom);
PATHNODES2_REVERSED("CNaviLink::GetLanePosition", 0x004403C0, &GetLanePosition);
PATHNODES2_REVERSED("CPaths::Construct", 0x00440450, &Construct);
PATHNODES2_REVERSED("CPaths::DropPassedNode", 0x00440780, &DropPassedNode);
PATHNODES2_REVERSED("CPaths::GenerateCreationPoint", 0x00440870, &GenerateCreationPoint);
PATHNODES2_REVERSED("CPaths::FindNodesInRange", 0x00440E10, &FindNodesInRange);
PATHNODES2_REVERSED("CPaths::FindSpawnPoint", 0x00440FB0, &FindSpawnPoint);
PATHNODES2_REVERSED("CPaths::CalcRoadDensity", 0x00441330, &CalcRoadDensity);
PATHNODES2_REVERSED("CPaths::RemoveNodeFromList", 0x00441540, &RemoveNodeFromList);
PATHNODES2_REVERSED("CPaths::AddNodeToList", 0x00441690, &AddNodeToList);
PATHNODES2_REVERSED("CPaths::GetLinkSideOffset", 0x00441790, &GetLinkSideOffset);
PATHNODES2_REVERSED("CPaths::FindNodeClosestToCoors", 0x00441950, &FindNodeClosestToCoors);
} // namespace PathNodes2
