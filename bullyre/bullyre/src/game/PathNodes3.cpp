// More of the ped paths (see PathNodes.cpp): the node searches by direction and facing,
// the area flags, node positions, the nodes' saving, the link lengths and the navi
// links' set-up.

#include <cmath>
#include <cstdint>

#include "game/Collision.h"
#include "game/PathNodes.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {
using namespace paths;

GAME_ARRAY(uint8_t, ms_linkLengths, 0x00C03D4C);   // by link
GAME_VAR(float, ms_twoPi, 0x00A144A8);
inline void* const ms_collisionWorld = reinterpret_cast<void*>(0x00C2AB70);

GAME_FN(col::CVector*, __thiscall, Node_GetPosition, 0x00417C00, const void* node, col::CVector* out);
GAME_FN(col::CVector*, __cdecl, VectorSub, 0x00414B70, col::CVector* out, const col::CVector* a, const col::CVector* b);
GAME_FN(col::CVector*, __cdecl, VectorAdd, 0x004129A0, col::CVector* out, const col::CVector* a, const col::CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, col::CVector* v);
GAME_FN(uint8_t, __thiscall, Fn_46EFB0, 0x0046EFB0, void* self, void* ped, const col::CVector* pos, int);
GAME_FN(uint8_t, __cdecl, Fn_48C990, 0x0048C990, const col::CVector* point, float radius, int, uintptr_t);
GAME_FN(uint8_t, __thiscall, Area_ContainsRect, 0x006D6EB0, void* area, const float* rect);
GAME_FN(PathNode*, __thiscall, Route_GetNode, 0x00417940, void* route);
GAME_FN(void, __thiscall, Navi_Setup, 0x004424C0, NaviLink* navi, int other);
GAME_FN(void, __thiscall, Link_SetGame, 0x00442A40, void* link, int dest, int src, NaviLink* navi);
GAME_FN(void, __thiscall, Archive_ReadObject, 0x005B0720, void* archive, void* object, const char* name);
GAME_FN(void, __thiscall, Archive_WriteObject, 0x005B0730, void* archive, void* object, const char* name);
GAME_FN(void, __thiscall, Paths_ReadLinks, 0x00442DF0, void* paths, void* archive, int count, int withNavi);
GAME_FN(int, __thiscall, Paths_WriteLinks, 0x00442D10, void* paths, void* archive, uint32_t index);
inline void* const ms_paths = reinterpret_cast<void*>(0x00BF6228);
GAME_ARRAY(PathNode*, ms_visited, 0x00BF46B0);   // the search's visited nodes
GAME_FN(void*, __thiscall, Ped_Fn_4CE560, 0x004CE560, void* self, int);
GAME_FN(PathNode*, __thiscall, Paths_FindNodeClosestToCoors, 0x00441950, void* paths, void* ped, float x, float y, float z, int kind, float maxDist, int, int, int, int);
GAME_FN(void, __thiscall, Paths_AddNodeToList, 0x00441690, void* paths, PathNode* node, int distance);
GAME_FN(void, __thiscall, Paths_RemoveNodeFromList, 0x00441540, void* paths, PathNode* node);
inline char* __cdecl StrNCpy(char* dst, const char* src, size_t size) { return crt::Strncpy(dst, src, size); }
inline char* __cdecl StrNCat(char* dst, const char* src, size_t size) { return crt::Strncat(dst, src, size); }
GAME_FN(void*, __cdecl, Archive_Open, 0x005B07E0, const char* path, int32_t* out, int);
GAME_FN(void, __cdecl, Archive_Close, 0x005B0860, void* archive);
GAME_FN(void, __thiscall, Node_CalcLinkLengths, 0x004428B0, PathNode* node);
inline void* const CArchivablePath_vftable = reinterpret_cast<void*>(0x009051B8);
inline void* const ms_sharedLink = reinterpret_cast<void*>(0x00A144B4);   // a CPathLink
GAME_VAR(NaviLink*, ms_sharedLinkNavi, 0x00A144C4);
GAME_VAR(int32_t, ms_sharedLinkId, 0x00A144C8);
GAME_FN(void*, __thiscall, Route_Fn_417920, 0x00417920, void* route);
GAME_FN(void*, __thiscall, Route_Fn_417960, 0x00417960, void* route);

template <typename R, typename... A>
R VCall(void* object, uint32_t offset, A... args) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(fn)(object, args...);
}

col::CVector NodePos(const PathNode* node) {
    return {float(node->pos[0]) * 0.125f, float(node->pos[1]) * 0.125f, float(node->pos[2]) * 0.125f};
}

// (x87: the float sum's fabs, then the CRT's sqrt: 0x85C500, its argument in st0)
float Length3D(const col::CVector& d) {
    float sum = float(double(d.x) * d.x + double(d.y) * d.y + double(d.z) * d.z);
    return float(std::sqrt(double(std::fabs(sum))));
}

// the range of a search: kind 0 the first +0x1071C nodes, 1 from there to +0x10718,
// else none (the whole word as both ends)
void SearchRange(const uint8_t* self, int kind, int& start, int& end) {
    if (uint8_t(kind) == 0) {
        start = 0;
        end = *reinterpret_cast<const int32_t*>(self + 0x1071C);
    } else if (uint8_t(kind) == 1) {
        start = *reinterpret_cast<const int32_t*>(self + 0x1071C);
        end = *reinterpret_cast<const int32_t*>(self + 0x10718);
    } else {
        start = kind;
        end = kind;
    }
}

PathNode* NodeAt(uint8_t* self, int i) {
    return uint32_t(i) > 0x6D5 ? nullptr : reinterpret_cast<PathNode*>(self + 0x19C8 + i * 0x18);
}

// a search list reference (below 0x200: a bucket at 0xC0695C; else a node + 0x200)
PathNode* ListEntry(int16_t ref) {
    if (ref < 0) {
        return nullptr;
    }
    if (ref < 0x200) {
        return reinterpret_cast<PathNode*>(0x00C0695C + ref * 0x18);
    }
    return NodeById(uint32_t(ref - 0x200));
}

NaviLink* LinkNavi(const PathNode* node, int i, int links) {
    if (i < links) {
        int id = ms_naviLinkIds[node->firstLink + i];
        if (id >= 0 && id < ms_numNaviLinks) {
            return &ms_naviLinks[id];
        }
    }
    return nullptr;
}

} // namespace

namespace PathNodes3 {

// 0x441B60 (thiscall): the node closest to the point (x + y + 3 z distances, within
// maxDist) in the direction (the cosine of the angle), with the given flags, that
// 0x46EFB0 doesn't reject for the ped and 0x48C990 accepts. Nothing for a zero direction
static PathNode* __fastcall FindNodeInDirection(uint8_t* self, re::Edx, void* ped, float x, float y, float z, float dx,
                                                float dy, float dz, float angle, int kind, float maxDist,
                                                int allowSwitchedOff, int allowBetweenLevels, int allowFlag8,
                                                int water) {
    float lengthSq = float(double(dx) * dx + double(dy) * dy + double(dz) * dz);
    float check = float(double(lengthSq) - 1.0000000949949049e-06);
    if (check < 0.0f) {
        return nullptr;
    }
    float cosine = float(std::cos(double(angle)));   // (0x85AD80: the CRT's x87 cos)
    int start;
    int end;
    SearchRange(self, kind, start, end);
    float best = maxDist;
    PathNode* bestNode = nullptr;
    col::CVector pos = {x, y, z};
    for (int i = start; i < end; i++) {
        PathNode* node = NodeAt(self, i);
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
        col::CVector nodePos = NodePos(node);
        col::CVector d;
        VectorSub(&d, &nodePos, &pos);
        float dot = float(double(d.y) * dy + double(d.x) * dx + double(d.z) * dz);
        if (cosine > dot) {
            continue;
        }
        if ((node->flags14 & 0xF) <= 1) {
            continue;
        }
        col::CVector p;
        float ax = std::fabs(float(double(Node_GetPosition(node, &p)->x) - x));
        float ay = std::fabs(float(double(Node_GetPosition(node, &p)->y) - y));
        float az = std::fabs(float(double(Node_GetPosition(node, &p)->z) - z));
        float dist = float(double(ay) + ax + double(az) * 3.0);
        if (best < dist) {
            continue;
        }
        if (Fn_46EFB0(ms_collisionWorld, ped, Node_GetPosition(node, &p), 1)) {
            continue;
        }
        if (Fn_48C990(Node_GetPosition(node, &p), 1.0f, -1, 0)) {
            best = dist;
            bestNode = node;
        }
    }
    return bestNode;
}

// 0x441E90 (thiscall): the node of the range closest to the point (x + y + 3 z
// distances) less 20 per unit the direction to it turns from the heading (dx, dy)
static PathNode* __fastcall FindNodeFacing(uint8_t* self, re::Edx, float x, float y, float z, int kind, float dirX,
                                           float dirY) {
    col::CVector dir = {dirX, dirY, 0.0f};
    Vector_Normalise(&dir);
    int start;
    int end;
    SearchRange(self, kind, start, end);
    float best = 10000.0f;
    PathNode* bestNode = nullptr;
    col::CVector pos = {x, y, z};
    for (int i = start; i < end; i++) {
        PathNode* node = NodeAt(self, i);
        col::CVector nodePos = NodePos(node);
        float ax = std::fabs(float(double(nodePos.x) - x));
        float ay = std::fabs(float(double(nodePos.y) - y));
        float az = std::fabs(float(double(nodePos.z) - z));
        float dist = float(double(ax) + ay + double(az) * 3.0);
        if (!(best > dist)) {
            continue;
        }
        col::CVector d;
        VectorSub(&d, &nodePos, &pos);
        d.z = 0.0f;
        Vector_Normalise(&d);
        float dot = float(double(d.y) * dir.y + double(d.x) * dir.x + double(dir.z) * d.z);
        float score = float(double(dist) - (double(dot) - 1.0) * 20.0);
        if (best > score) {
            best = score;
            bestNode = node;
        }
    }
    return bestNode;
}

// 0x442090 (thiscall): flag 0x40 of the nodes (of the whole +0x10718) in the area set
static void __fastcall SetNodesFlagInArea(uint8_t* self, re::Edx, void* area, int flag) {
    uint8_t bit = uint8_t(flag);
    for (int i = 0; i < *reinterpret_cast<int32_t*>(self + 0x10718); i++) {
        PathNode* node = reinterpret_cast<PathNode*>(self + 0x19C8 + i * 0x18);
        float x = float(node->pos[0] * 0.125);
        float y = float(node->pos[1] * 0.125);
        float rect[4] = {x, y, x, y};
        if (Area_ContainsRect(area, rect)) {
            node->flags14 ^= (uint8_t(bit << 6) ^ node->flags14) & 0x40;
        }
    }
}

// the link to the target node (the entity's +0x2AC or +0x2B0) from the route's node has
// navi flags (+9 bits 0-1)
static bool LinkHasNaviFlags(PathNode* node, int target) {
    int n = node->NumLinks();
    for (int16_t i = 0; i < n; i++) {
        int id = i < n ? int(ms_linkNodes[node->firstLink + i] & 0x3FFF) : -1;
        if (id == target) {
            NaviLink* navi = LinkNavi(node, i, n);
            if (reinterpret_cast<uint8_t*>(navi)[9] & 3) {
                return true;
            }
        }
    }
    return false;
}

// 0x442120 (thiscall on an entity with a route at +0x2A8, status 2 or 3): its route's
// link to its next node (or the one after) has navi flags
static uint8_t __fastcall IsOnFlaggedLink(uint8_t* self, re::Edx) {
    uint8_t status = self[0x108] >> 3;
    if (status != 2 && status != 3) {
        return 0;
    }
    void* route = self + 0x2A8;
    PathNode* node = Route_GetNode(route);
    void* next = Route_Fn_417920(route);
    if (node && next && node->NumLinks() > 0) {
        if (LinkHasNaviFlags(node, *reinterpret_cast<int32_t*>(self + 0x2AC))) {
            return 1;
        }
    }
    void* after = Route_Fn_417960(route);
    if (node && after && node->NumLinks() > 0) {
        if (LinkHasNaviFlags(node, *reinterpret_cast<int32_t*>(self + 0x2B0))) {
            return 1;
        }
    }
    return 0;
}

// 0x442270 (thiscall): the node's position
static col::CVector* __fastcall GetNodePosition(uint8_t* self, re::Edx, col::CVector* out, int index) {
    const PathNode* node = reinterpret_cast<const PathNode*>(self + 0x19C8 + index * 0x18);
    out->x = float(node->pos[0]) * 0.125f;
    out->y = float(node->pos[1]) * 0.125f;
    out->z = float(node->pos[2]) * 0.125f;
    return out;
}

// 0x4422C0: a point on the node's disc: angle (the low byte, 256ths of a turn) and
// radius (the square root of the high byte over 255, times a tenth of the width)
static col::CVector* __cdecl GetPointOnNode(col::CVector* out, const PathNode* node, int packed) {
    out->z = float(node->pos[2] * 0.125);
    uint8_t low = uint8_t(packed);
    uint8_t high = uint8_t(packed >> 8);
    float angle = float(double(low) * 0.00390625 * ms_twoPi);
    float root = float(std::sqrt(double(std::fabs(float(double(high) / 255.0)))));
    float width = float(double(node->width) * 0.10000000149011612);
    float radius = root * width;
    float c = float(std::cos(double(angle)));   // (0x85AD80, 0x85AEF0: the CRT's x87 cos, sin)
    float x = float(node->pos[0] * 0.125);
    out->x = float(double(c) * radius + x);
    float s = float(std::sin(double(angle)));
    float y = float(node->pos[1] * 0.125);
    out->y = float(double(s) * radius + y);
    return out;
}

// 0x4423C0 (thiscall): the second range's nodes within the radius (3D) switched back on
static void __fastcall SwitchOnNodesNear(uint8_t* self, re::Edx, float x, float y, float z, float radius) {
    col::CVector pos = {x, y, z};
    for (int i = *reinterpret_cast<int32_t*>(self + 0x1071C); i < *reinterpret_cast<int32_t*>(self + 0x10718); i++) {
        PathNode* node = NodeAt(self, i);
        col::CVector nodePos = NodePos(node);
        col::CVector d;
        VectorSub(&d, &nodePos, &pos);
        float dist = Length3D(d);
        if (radius > dist) {
            node->flags14 &= 0xDF;
        }
    }
}

// 0x4424C0 (thiscall on a navi link): its position (the middle, in eighths) and direction
// (hundredths, flat) between its node and the given one
static void __fastcall NaviLink_Setup(NaviLink* self, re::Edx, uint32_t other) {
    PathNode* a = NodeById(uint32_t(int32_t(self->node)));
    PathNode* b = NodeById(other);
    col::CVector posB = NodePos(b);
    col::CVector posA = NodePos(a);
    col::CVector dir;
    VectorSub(&dir, &posA, &posB);
    col::CVector sum;
    VectorAdd(&sum, &posA, &posB);
    float midX = float(double(sum.x) * 0.5);
    float midY = float(double(sum.y) * 0.5);
    int16_t* xy = reinterpret_cast<int16_t*>(self);
    xy[0] = int16_t(int64_t(double(midX) * 8.0));
    xy[1] = int16_t(int64_t(double(midY) * 8.0));
    dir.z = 0.0f;
    Vector_Normalise(&dir);
    self->dirX = int8_t(int64_t(double(dir.x) * 100.0));
    self->dirY = int8_t(int64_t(double(dir.y) * 100.0));
}

// CPathNode vf2 (0x442690): saved (vf 0x48 a vector, 0x60 an int)
static void __fastcall Node_Save(PathNode* self, re::Edx, void* archive) {
    col::CVector pos = NodePos(self);
    VCall<void>(archive, 0x48, &pos, "Position");
    VCall<void>(archive, 0x60, int(self->flags14 & 0xF), "NLINKS");
    int index = NodeIndex(self);
    int n = self->flags14 & 0xF;
    for (int i = 0; i < n; i++) {
        uint32_t link = ms_linkNodes[self->firstLink + i] & 0x3FFF;
        int naviIndex = i < n ? self->firstLink + i : -1;
        int naviId = ms_naviLinkIds[naviIndex];
        // (an unused search of the other node's links for this one)
        volatile const PathNode* other = NodeById(link);
        int j = other->firstLink;
        int end = (other->flags14 & 0xF) + j;
        while (j < end && uint32_t(ms_linkNodes[j] & 0x3FFF) != uint32_t(index)) {
            j++;
        }
        VCall<void>(archive, 0x60, naviId, "LINKID");
        n = self->flags14 & 0xF;
    }
    VCall<void>(archive, 0x60, int(self->width), "WIDTH");
    VCall<void>(archive, 0x60, int((self->flags14 >> 4) & 1), "ONDEADEND");
    VCall<void>(archive, 0x60, int((self->flags14 >> 5) & 1), "SWITCHEDOFF");
    VCall<void>(archive, 0x60, int(self->flags14 >> 7), "BETWEENLEVELS");
    VCall<void>(archive, 0x60, int((self->flags15 >> 1) & 1), "WATERNODE");
    VCall<void>(archive, 0x60, int((self->flags15 >> 2) & 1), "UNDERBRIDGE");
    VCall<void>(archive, 0x60, int((self->flags15 >> 4) & 3), "SPEED");
    VCall<void>(archive, 0x60, int(self->flags16 & 0xF), "DENSITY");
}

// 0x4428B0 (thiscall): the node's links' lengths (at most 255, truncated) into 0xC03D4C
static void __fastcall CalcLinkLengths(PathNode* self, re::Edx) {
    col::CVector here = NodePos(self);
    for (int i = 0; i < self->NumLinks(); i++) {
        PathNode* other = i < self->NumLinks() ? NodeById(ms_linkNodes[self->firstLink + i] & 0x3FFF) : nullptr;
        int slot = self->firstLink + i;
        col::CVector there = NodePos(other);
        col::CVector d;
        VectorSub(&d, &here, &there);
        float length = Length3D(d);
        if (length > 255.0f) {
            length = 255.0f;
        }
        ms_linkLengths[slot] = uint8_t(int32_t(length));
    }
}

// 0x442A40 (thiscall on a CPathLink): its nodes, navi link and length
static void __fastcall Link_Set(uint8_t* self, re::Edx, int dest, int src, NaviLink* navi) {
    *reinterpret_cast<int32_t*>(self + 4) = dest;
    *reinterpret_cast<int32_t*>(self + 8) = src;
    *reinterpret_cast<re::Ptr32<NaviLink>*>(self + 0x10) = navi;
    PathNode* a = (dest >= 0 && dest < 0x6D6) ? &ms_nodes[dest] : nullptr;
    col::CVector posA = NodePos(a);
    PathNode* b = (src >= 0 && src < 0x6D6) ? &ms_nodes[src] : nullptr;
    col::CVector posB = NodePos(b);
    col::CVector d;
    VectorSub(&d, &posA, &posB);
    *reinterpret_cast<float*>(self + 0xC) = Length3D(d);
}

// CPathLink vf1 (0x442B60): loaded (vf 0x28 a float, 0x38 an int, false when missing: 0):
// the two first link slots with its navi id get its nodes (the higher first; + the node
// count without a navi link); the navi link's lanes (swapped when the source is the
// higher), width, node and position. (The distance is read and dropped)
static void __fastcall Link_Load(uint8_t* self, re::Edx, void* archive) {
    float distance;
    VCall<uint8_t>(archive, 0x28, &distance, "Dist");
    int32_t value;
    int dest = VCall<uint8_t>(archive, 0x38, &value, "DEST") ? value : 0;
    int src = VCall<uint8_t>(archive, 0x38, &value, "SRC") ? value : 0;
    int pair[2];
    if (dest > src) {
        pair[0] = dest;
        pair[1] = src;
    } else {
        pair[0] = src;
        pair[1] = dest;
    }
    NaviLink* navi = *reinterpret_cast<re::Ptr32<NaviLink>*>(self + 0x10);
    if (!navi) {
        pair[0] += ms_numNodes;
        pair[1] += ms_numNodes;
    }
    int slot = 0;
    for (int k = 0; k <= 1; k++) {
        int32_t id = *reinterpret_cast<int32_t*>(self + 0x14);
        for (; slot < 0xEA6; slot++) {
            if (ms_naviLinkIds[slot] == id) {
                ms_linkNodes[slot] = uint16_t(pair[k]);
                slot++;
                break;
            }
        }
    }
    if (!navi) {
        return;
    }
    uint8_t to = VCall<uint8_t>(archive, 0x38, &value, "LANESTO") ? uint8_t(value) : 0;
    navi->lanes ^= (navi->lanes ^ to) & 7;
    uint8_t from = (VCall<uint8_t>(archive, 0x38, &value, "LANESFRO") ? uint8_t(value) : 0) & 7;
    navi = *reinterpret_cast<re::Ptr32<NaviLink>*>(self + 0x10);
    navi->lanes = uint8_t((navi->lanes & 0xC7) | (from << 3));
    if (src > dest) {
        navi = *reinterpret_cast<re::Ptr32<NaviLink>*>(self + 0x10);
        uint8_t l = navi->lanes;
        navi->lanes = uint8_t((((l << 3) ^ l) & 0x38) ^ l);
        navi = *reinterpret_cast<re::Ptr32<NaviLink>*>(self + 0x10);
        navi->lanes = uint8_t((navi->lanes & 0xF8) | from);
    }
    uint8_t width = VCall<uint8_t>(archive, 0x38, &value, "WIDTH") ? uint8_t(value) : 0;
    (*reinterpret_cast<re::Ptr32<NaviLink>*>(self + 0x10))->width = width;
    (*reinterpret_cast<re::Ptr32<NaviLink>*>(self + 0x10))->node = int16_t(pair[1]);
    Navi_Setup(*reinterpret_cast<re::Ptr32<NaviLink>*>(self + 0x10), pair[0]);
}

// 0x442D10 (thiscall, on 0xBF6228): the node's links to higher nodes saved through the
// shared CPathLink at 0xA144B4 ("LINKID", then the link as "LINK"); how many
static int __fastcall WriteLinks(uint8_t* self, re::Edx, void* archive, uint32_t index) {
    PathNode* node = NodeById(index);
    int n = node->flags14 & 0xF;
    int first = node->firstLink;
    int count = 0;
    if (n <= 0) {
        return 0;
    }
    const uint16_t* links = reinterpret_cast<const uint16_t*>(self + 0xBDD8) + first;
    for (int i = 0; i < n; i++, links++) {
        int other = *links & 0x3FFF;
        if (other <= int(index)) {
            continue;
        }
        int id = reinterpret_cast<const int16_t*>(links)[0x15F9];   // (its navi id)
        VCall<void>(archive, 0x60, id, "LINKID");
        NaviLink* navi = id < *reinterpret_cast<int32_t*>(self + 0x10728) ? reinterpret_cast<NaviLink*>(self + id * 0xC) : nullptr;
        Link_SetGame(ms_sharedLink, int(index), other, navi);
        Archive_WriteObject(archive, ms_sharedLink, "LINK");
        count++;
    }
    return count;
}

// 0x442DF0 (thiscall, on 0xBF6228): `count` links loaded through the shared CPathLink
// (its navi id, and navi link when `withNavi`)
static void __fastcall ReadLinks(uint8_t* self, re::Edx, void* archive, int count, int withNavi) {
    if (count <= 0) {
        return;
    }
    uint8_t navi = uint8_t(withNavi);
    for (int i = count; i != 0; i--) {
        int32_t id;
        int32_t value = VCall<uint8_t>(archive, 0x38, &id, "LINKID") ? id : 0;
        ms_sharedLinkId = value;
        ms_sharedLinkNavi = navi ? reinterpret_cast<NaviLink*>(self + value * 0xC) : nullptr;
        Archive_ReadObject(archive, ms_sharedLink, "LINK");
    }
}

// CArchivablePath vf1 (0x442E70): loaded: its node count (under its first name) from its
// first node, the nodes, its link count (under its second name) and the links
static void __fastcall ArchivablePath_Load(uint8_t* self, re::Edx, void* archive) {
    int32_t value;
    uint8_t ok = VCall<uint8_t>(archive, 0x38, &value, *reinterpret_cast<re::Ptr32<const char>*>(self + 0x14));
    int32_t start = *reinterpret_cast<int32_t*>(self + 4);
    *reinterpret_cast<int32_t*>(self + 8) = (ok ? value : 0) + start;
    for (int i = start; i < *reinterpret_cast<int32_t*>(self + 8); i++) {
        int32_t id;
        VCall<uint8_t>(archive, 0x38, &id, "NODEID");
        Archive_ReadObject(archive, NodeById(uint32_t(i)), "NODE");
    }
    ok = VCall<uint8_t>(archive, 0x38, &value, *reinterpret_cast<re::Ptr32<const char>*>(self + 0x18));
    int32_t* links = reinterpret_cast<int32_t*>(self + 0xC);
    *links = ok ? value : 0;
    Paths_ReadLinks(ms_paths, archive, *links, self[0x10]);
}

// CArchivablePath vf2 (0x442F30): saved
static void __fastcall ArchivablePath_Save(uint8_t* self, re::Edx, void* archive) {
    int32_t start = *reinterpret_cast<int32_t*>(self + 4);
    VCall<void>(archive, 0x60, *reinterpret_cast<int32_t*>(self + 8) - start, *reinterpret_cast<re::Ptr32<const char>*>(self + 0x14));
    for (int i = *reinterpret_cast<int32_t*>(self + 4); i < *reinterpret_cast<int32_t*>(self + 8); i++) {
        VCall<void>(archive, 0x60, i, "NODEID");
        Archive_WriteObject(archive, &ms_nodes[i], "NODE");
    }
    VCall<void>(archive, 0x60, *reinterpret_cast<int32_t*>(self + 0xC), *reinterpret_cast<re::Ptr32<const char>*>(self + 0x18));
    for (int i = *reinterpret_cast<int32_t*>(self + 4); i < *reinterpret_cast<int32_t*>(self + 8); i++) {
        Paths_WriteLinks(ms_paths, archive, uint32_t(i));
    }
}

// the visited nodes' distances back to none (0x7FFE)
static void ResetVisited(int count) {
    for (int i = 0; i < count; i++) {
        ms_visited[i]->distance = 0x7FFE;
    }
}

// 0x442FC0 (thiscall): the route between two nodes (GTA's DoPathSearch): the start node
// (given, else the closest to the start point within maxDist) and the target (given, else
// the closest at all); within 1000 (2D) a search by distance through the 512 buckets
// (links without lanes our way skipped unless `kind`), then the route back from the
// target (target first) up to `max` nodes. The distance 100000 when there's none.
// (A node whose links don't lead back keeps the route loop going)
static void __fastcall DoPathSearch(uint8_t* self, re::Edx, int kind, float tx, float ty, float tz, int targetIndex,
                                    float sx, float sy, float sz, re::Ptr32<PathNode>* route, int16_t* count, int16_t max,
                                    uint8_t* ped, float* distance, float maxDist, int startIndex) {
    PathNode* start;
    if (startIndex < 0) {
        void* who = Ped_Fn_4CE560(ped + 0x1C4, 0);
        start = Paths_FindNodeClosestToCoors(self, who, sx, sy, sz, kind, maxDist, 0, 0, 0, 0);
    } else if (startIndex < 0x6D6) {
        start = reinterpret_cast<PathNode*>(self + 0x19C8 + startIndex * 0x18);
    } else {
        start = nullptr;
    }
    PathNode* target = nullptr;
    if (start) {
        if (targetIndex < 0) {
            void* who = Ped_Fn_4CE560(ped + 0x1C4, 0);
            target = Paths_FindNodeClosestToCoors(self, who, tx, ty, tz, kind, 999999.875f, 0, 0, 0, 0);
        } else if (targetIndex < 0x6D6) {
            target = reinterpret_cast<PathNode*>(self + 0x19C8 + targetIndex * 0x18);
        }
    }
    bool ok = start && target;
    if (ok && target == start) {
        *count = 0;
        if (distance) {
            *distance = 0.0f;
        }
        return;
    }
    if (ok) {
        col::CVector a;
        col::CVector b;
        col::CVector d;
        col::CVector* targetPos = Node_GetPosition(target, &a);
        col::CVector* startPos = Node_GetPosition(start, &b);
        VectorSub(&d, startPos, targetPos);
        float distSq = float(double(d.x) * d.x + double(d.y) * d.y);
        ok = !(distSq > 1000000.0f);
    }
    if (!ok) {
        *count = 0;
        if (distance) {
            *distance = 100000.0f;
        }
        return;
    }
    for (int i = 0; i < 0x200; i++) {
        *reinterpret_cast<int16_t*>(self + 0x1073A + i * 0x18) = -1;
    }
    Paths_AddNodeToList(self, start, 0);
    ms_visited[0] = start;
    int numVisited = 1;
    int bucket = 0;
    bool found = false;
    bool changed = false;
    const uint8_t* lengths = self + 0xDB24;
    for (;;) {
        PathNode* node = ListEntry(reinterpret_cast<PathNode*>(self + 0x10734 + int16_t(bucket) * 0x18)->next);
        while (node) {
            if (node == target) {
                found = true;
            }
            for (int j = 0; j < (node->flags14 & 0xF); j++) {
                int n = node->flags14 & 0xF;
                PathNode* other = j < n ? NodeById(ms_linkNodes[node->firstLink + j] & 0x3FFF) : nullptr;
                if (!uint8_t(kind)) {
                    NaviLink* navi = LinkNavi(node, j, n);
                    uint8_t lanes = navi->node == NodeIndex(other) ? (navi->lanes & 0x38) : (navi->lanes & 7);
                    if (lanes == 0) {
                        continue;
                    }
                }
                int d = lengths[node->firstLink + j] + node->distance;
                if (d < other->distance) {
                    if (other->distance != 0x7FFE) {
                        Paths_RemoveNodeFromList(self, other);
                    }
                    if (other->distance == 0x7FFE) {
                        ms_visited[numVisited++] = other;
                    }
                    Paths_AddNodeToList(self, other, d);
                    changed = true;
                }
            }
            Paths_RemoveNodeFromList(self, node);
            node = ListEntry(node->next);
        }
        bucket++;
        if (int16_t(bucket) >= 0x200) {
            if (!changed) {
                ResetVisited(numVisited);
                *count = 0;
                if (distance) {
                    *distance = 100000.0f;
                }
                return;
            }
            changed = false;
            bucket = 0;
        }
        if (found) {
            break;
        }
    }
    *count = 0;
    if (distance) {
        *distance = float(target->distance);
    }
    PathNode* current = target;
    route[*count] = current;
    (*count)++;
    while (*count < max && current != start) {
        int n = current->flags14 & 0xF;
        for (int j = 0; j < n; j++) {
            PathNode* other = NodeById(ms_linkNodes[current->firstLink + j] & 0x3FFF);
            int slot = j < n ? current->firstLink + j : -1;
            if (current->distance - lengths[slot] == other->distance) {
                current = reinterpret_cast<PathNode*>(self + 0x19C8 + (*reinterpret_cast<uint16_t*>(self + 0xBDD8 + (current->firstLink + j) * 2) & 0x3FFF) * 0x18);
                route[*count] = current;
                (*count)++;
                break;
            }
        }
    }
    ResetVisited(numVisited);
}

// a CArchivablePath on the stack (see its vf1, vf2)
struct ArchivablePath {
    re::Ptr32<void> vtable;         // +0
    int32_t start;        // +4 the first node
    int32_t end;          // +8
    int32_t links;        // +0xC
    uint8_t withNavi;     // +0x10
    uint8_t unknown11[3]; // +0x11
    re::Ptr32<const char> nodesName;  // +0x14
    re::Ptr32<const char> linksName;  // +0x18
};
static_assert(sizeof(ArchivablePath) == 0x1C);

// 0x4434E0 (thiscall): the grids file (CONFIG\ and the name): the counts, the ped grid
// (the nodes after the car grid's, their navi ids moved past the car grid's links) and the
// car grid, then the links' lengths
static void __fastcall LoadGrids(uint8_t* self, re::Edx, const char* name) {
    char path[0x24];
    path[0] = 0;
    StrNCpy(path, "CONFIG\\", 0x21);
    size_t length = 0;
    while (path[length]) {
        length++;
    }
    StrNCat(path, name, 0x21 - length);
    int32_t unknown = 0;
    void* archive = Archive_Open(path, &unknown, 1);
    if (!archive) {
        return;
    }
    int32_t* carNodes = reinterpret_cast<int32_t*>(self + 0x1071C);
    int32_t* pedNodes = reinterpret_cast<int32_t*>(self + 0x10720);
    int32_t* carLinks = reinterpret_cast<int32_t*>(self + 0x10728);
    int32_t* pedLinks = reinterpret_cast<int32_t*>(self + 0x1072C);
    int16_t* numLinks = reinterpret_cast<int16_t*>(self + 0x10726);
    VCall<uint8_t>(archive, 0x38, carNodes, "NCARGRIDNODES");
    VCall<uint8_t>(archive, 0x38, pedNodes, "NPEDGRIDNODES ");
    VCall<uint8_t>(archive, 0x38, carLinks, "NCARGRIDLINKS ");
    VCall<uint8_t>(archive, 0x38, pedLinks, "NPEDGRIDLINKS ");
    *reinterpret_cast<int32_t*>(self + 0x10718) = *carNodes + *pedNodes;
    int32_t value;
    VCall<uint8_t>(archive, 0x38, &value, "NPEDCOLLISIONAVOIDANCEGRIDS");
    VCall<uint8_t>(archive, 0x38, &value, "NGRIDLINKSINSECTORS");
    VCall<uint8_t>(archive, 0x38, &value, "NSECTORGRIDLINKKEYS");
    ArchivablePath grid;
    grid.start = *carNodes;
    int first = *carLinks * 2;
    *numLinks = int16_t(*carLinks * 2);
    grid.vtable = CArchivablePath_vftable;
    grid.end = *reinterpret_cast<int32_t*>(self + 0x10718);
    grid.links = *pedLinks;
    grid.nodesName = "NSIDEWALKNODES";
    grid.linksName = "NSIDEWALKLINKS";
    grid.withNavi = 0;
    Archive_ReadObject(archive, &grid, "PEDGRID");
    int16_t* naviIds = reinterpret_cast<int16_t*>(self + 0xE9CA);
    for (int i = first; i < *numLinks; i++) {
        naviIds[i] = int16_t(naviIds[i] + int16_t(*carLinks));
    }
    grid.links = *carLinks;
    *numLinks = 0;
    grid.start = 0;
    grid.end = *carNodes;
    grid.nodesName = "NROADNODES";
    grid.linksName = "NROADLINKS";
    grid.withNavi = 1;
    Archive_ReadObject(archive, &grid, "CARGRID");
    *numLinks = int16_t((int16_t(*pedLinks) + int16_t(*carLinks)) * 2);
    Archive_Close(archive);
    for (int i = 0; i < *reinterpret_cast<int32_t*>(self + 0x10718); i++) {
        Node_CalcLinkLengths(NodeAt(self, i));
    }
}

} // namespace PathNodes3

// ---- registration ----------------------------------------------------------------------

#define PATHNODES3_REVERSED(name, address, fn)                                              \
    static void* RE_CONCAT(Original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace PathNodes3 {
PATHNODES3_REVERSED("CPaths::FindNodeInDirection", 0x00441B60, &FindNodeInDirection);
PATHNODES3_REVERSED("CPaths::FindNodeFacing", 0x00441E90, &FindNodeFacing);
PATHNODES3_REVERSED("CPaths::SetNodesFlagInArea", 0x00442090, &SetNodesFlagInArea);
PATHNODES3_REVERSED("CPhysical::IsOnFlaggedLink", 0x00442120, &IsOnFlaggedLink);
PATHNODES3_REVERSED("CPaths::GetNodePosition", 0x00442270, &GetNodePosition);
PATHNODES3_REVERSED("CPaths::GetPointOnNode", 0x004422C0, &GetPointOnNode);
PATHNODES3_REVERSED("CPaths::SwitchOnNodesNear", 0x004423C0, &SwitchOnNodesNear);
PATHNODES3_REVERSED("CNaviLink::Setup", 0x004424C0, &NaviLink_Setup);
PATHNODES3_REVERSED("CPathNode::Save", 0x00442690, &Node_Save);
PATHNODES3_REVERSED("CPathNode::CalcLinkLengths", 0x004428B0, &CalcLinkLengths);
PATHNODES3_REVERSED("CPathLink::Set", 0x00442A40, &Link_Set);
PATHNODES3_REVERSED("CPathLink::Load", 0x00442B60, &Link_Load);
PATHNODES3_REVERSED("CPaths::WriteLinks", 0x00442D10, &WriteLinks);
PATHNODES3_REVERSED("CPaths::ReadLinks", 0x00442DF0, &ReadLinks);
PATHNODES3_REVERSED("CArchivablePath::Load", 0x00442E70, &ArchivablePath_Load);
PATHNODES3_REVERSED("CArchivablePath::Save", 0x00442F30, &ArchivablePath_Save);
PATHNODES3_REVERSED("CPaths::DoPathSearch", 0x00442FC0, &DoPathSearch);
PATHNODES3_REVERSED("CPaths::LoadGrids", 0x004434E0, &LoadGrids);
} // namespace PathNodes3
