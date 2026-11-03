// The ped paths (0xBF6228, GTA's ThePaths): 1750 nodes of 0x18 at 0xBF7BF0 (+0x19C8),
// their links (node ids at 0xC02000, navi links at 0xC04BF2), the navi links (0xC bytes at
// the object's start), 0x200 extra entries at 0xC0695C (+0x10734). The node and link
// loaders/savers (CPathNode, CPathLink: IArchivable), and the next node on a walk.

#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "game/PathNodes.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {
using namespace paths;

inline void* const ms_collisionWorld = reinterpret_cast<void*>(0x00C2AB70);
inline void* const IArchivable_vftable = reinterpret_cast<void*>(0x00900CDC);

inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(col::CVector*, __thiscall, Node_GetPosition, 0x00417C00, const void* node, col::CVector* out);
GAME_FN(col::CVector*, __cdecl, VectorSub, 0x00414B70, col::CVector* out, const col::CVector* a, const col::CVector* b);
GAME_FN(col::CVector*, __cdecl, VectorNormalise, 0x0041A520, col::CVector* out, const col::CVector* v, float length);
GAME_FN(uint8_t, __thiscall, Fn_46EFB0, 0x0046EFB0, void* self, void* ped, const col::CVector* pos, int);

template <typename R, typename... A>
R VCall(void* object, uint32_t offset, A... args) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(fn)(object, args...);
}

} // namespace

namespace PathNodes {

// 0x43F7B0 (thiscall): the node of the node's link (-1: none)
static uint32_t __fastcall GetLinkNodeId(PathNode* self, re::Edx, int i) {
    if (i >= self->NumLinks()) {
        return 0xFFFFFFFF;
    }
    return ms_linkNodes[self->firstLink + i] & 0x3FFF;
}

// 0x43F7E0 (thiscall)
static PathNode* __fastcall GetLinkNode(PathNode* self, re::Edx, int i) {
    if (i >= self->NumLinks()) {
        return nullptr;
    }
    return NodeById(ms_linkNodes[self->firstLink + i] & 0x3FFF);
}

// 0x43F820 (thiscall): the link's navi link (past the links: the word before the array)
static int __fastcall GetNaviLinkId(PathNode* self, re::Edx, int i) {
    if (i >= self->NumLinks()) {
        return ms_naviLinkIds[-1];
    }
    return ms_naviLinkIds[self->firstLink + i];
}

// 0x43F850 (thiscall): the navi link of the link to the node (-1: none)
static int __fastcall FindNaviLinkTo(PathNode* self, re::Edx, uint32_t node) {
    int i = self->firstLink;
    int end = self->NumLinks() + i;
    for (; i < end; i++) {
        if ((ms_linkNodes[i] & 0x3FFFu) == node) {
            return ms_naviLinkIds[i];
        }
    }
    return -1;
}

// 0x43F8A0 (thiscall)
static int __fastcall GetIndex(PathNode* self, re::Edx) {
    return NodeIndex(self);
}

// 0x43F8C0 (thiscall): one of the loaded nodes
static uint8_t __fastcall IsLoaded(PathNode* self, re::Edx) {
    return NodeIndex(self) < ms_numNodes;
}

// 0x43F8F0 (thiscall): the lanes towards the node (0 without one)
static uint32_t __fastcall GetLanesTo(NaviLink* self, re::Edx, PathNode* node) {
    if (!node) {
        return 0;
    }
    uint32_t lanes = self->lanes;
    if (node == NodeById(uint32_t(int32_t(self->node)))) {
        lanes >>= 3;
    }
    return lanes & 7;
}

// 0x43F930 (thiscall): the lanes from the node
static uint32_t __fastcall GetLanesFrom(NaviLink* self, re::Edx, PathNode* node) {
    if (!node) {
        return 0;
    }
    uint32_t lanes = self->lanes;
    if (node != NodeById(uint32_t(int32_t(self->node)))) {
        lanes >>= 3;
    }
    return lanes & 7;
}

// 0x43F970 (thiscall): the direction scaled
static void __fastcall GetDirection(NaviLink* self, re::Edx, float* x, float* y, int scale) {
    double s = double(float(scale));
    *x = float(double(self->dirX) * 0.009999999776482582 * s);
    *y = float(double(self->dirY) * 0.009999999776482582 * s);
}

// 0x43F9C0 (thiscall): no lanes from the node
static uint32_t __fastcall HasNoLanesFrom(NaviLink* self, re::Edx, PathNode* node) {
    if (self->node == NodeIndex(node)) {
        return (self->lanes & 0x38) == 0;
    }
    return (self->lanes & 7) == 0;
}

// 0x43FA00 (thiscall, on 0xBF6228): the search state cleared, no node's distance
static void __fastcall Paths_Init(uint8_t* self, re::Edx) {
    *reinterpret_cast<int32_t*>(self + 0x10718) = 0;
    *reinterpret_cast<int16_t*>(self + 0x10726) = 0;
    *reinterpret_cast<int32_t*>(self + 0x10728) = 0;
    *reinterpret_cast<int32_t*>(self + 0x1072C) = 0;
    ms_unknownBF6210 = 0;
    PathNode* nodes = reinterpret_cast<PathNode*>(self + 0x19C8);
    for (int i = 0; i < 0x6D6; i++) {
        nodes[i].distance = 0x7FFE;
    }
}

// 0x43FA40 (thiscall, on 0xBF6228)
static void __fastcall Paths_ClearFlag40(uint8_t* self, re::Edx) {
    PathNode* nodes = reinterpret_cast<PathNode*>(self + 0x19C8);
    for (int i = 0; i < 0x6D6; i++) {
        nodes[i].flags14 &= ~0x40;
    }
}

// 0x43FA60 (thiscall): the width in metres
static float __fastcall GetWidth(NaviLink* self, re::Edx) {
    return float(double(self->width) / 80.0 + 0.5);
}

// CPathNode vf1 (0x43FA80): loaded from the archive (vf 0x20 a vector, 0x38 an int)
static void __fastcall Node_Load(PathNode* self, re::Edx, void* archive) {
    col::CVector pos = {0.0f, 0.0f, 0.0f};
    VCall<void>(archive, 0x20, &pos, "Position");
    self->pos[0] = int16_t(int64_t(double(pos.x) * 8.0));
    self->pos[1] = int16_t(int64_t(double(pos.y) * 8.0));
    self->pos[2] = int16_t(int64_t(double(pos.z) * 8.0));
    int32_t value;
    VCall<void>(archive, 0x38, &value, "NLINKS");
    self->flags14 ^= (self->flags14 ^ uint8_t(value)) & 0xF;
    self->firstLink = ms_numLinksLoaded;
    ms_numLinksLoaded = int16_t(ms_numLinksLoaded + (self->flags14 & 0xF));
    for (int i = 0; i < (self->flags14 & 0xF); i++) {
        int32_t id;
        VCall<void>(archive, 0x38, &id, "LINKID");
        int index = i < (self->flags14 & 0xF) ? self->firstLink + i : -1;
        ms_naviLinkIds[index] = int16_t(id);
    }
    VCall<void>(archive, 0x38, &value, "WIDTH");
    self->width = uint8_t(value);
    VCall<void>(archive, 0x38, &value, "ONDEADEND");
    self->flags14 ^= (uint8_t(value << 4) ^ self->flags14) & 0x10;
    VCall<void>(archive, 0x38, &value, "SWITCHEDOFF");
    self->flags14 ^= (uint8_t(value << 5) ^ self->flags14) & 0x20;
    VCall<void>(archive, 0x38, &value, "BETWEENLEVELS");
    self->flags14 = uint8_t((self->flags14 & 0x7F) | uint8_t(value << 7));
    VCall<void>(archive, 0x38, &value, "WATERNODE");
    self->flags15 ^= (uint8_t(value * 2) ^ self->flags15) & 2;
    VCall<void>(archive, 0x38, &value, "UNDERBRIDGE");
    self->flags15 ^= (uint8_t(value * 4) ^ self->flags15) & 4;
    VCall<void>(archive, 0x38, &value, "SPEED");
    self->flags15 ^= (uint8_t(value << 4) ^ self->flags15) & 0x30;
    VCall<void>(archive, 0x38, &value, "DENSITY");
    self->flags16 ^= (self->flags16 ^ uint8_t(value)) & 0xF;
}

// CPathLink vf2 (0x43FC80): saved (vf 0x50 a float, 0x60 an int)
static void __fastcall Link_Save(PathLink* self, re::Edx, void* archive) {
    VCall<void>(archive, 0x50, self->distance, "Dist");
    VCall<void>(archive, 0x60, self->dest, "DEST");
    VCall<void>(archive, 0x60, self->src, "SRC");
    if (self->navi) {
        VCall<void>(archive, 0x60, int(self->navi->lanes & 7), "LANESTO");
        VCall<void>(archive, 0x60, int((self->navi->lanes >> 3) & 7), "LANESFRO");
        VCall<void>(archive, 0x60, int(self->navi->width), "WIDTH");
    }
}

// a search list reference to a bucket or a node (below 0x200: a bucket)
static int16_t EntryRef(const uint8_t* p) {
    uintptr_t a = reinterpret_cast<uintptr_t>(p);
    if (a >= 0x00C0695C && a < 0x00C0995C) {
        return int16_t(int32_t(a - 0x00C0695C) / 0x18);
    }
    return int16_t(int32_t(a - 0x00BF7BF0) / 0x18 + 0x200);
}

// 0x43FD20 (thiscall): the search list's previous entry
static void __fastcall SetPrev(PathNode* self, re::Edx, const uint8_t* entry) {
    self->prev = entry ? EntryRef(entry) : int16_t(-1);
}

// 0x43FD90 (thiscall): the search list's next entry
static void __fastcall SetNext(PathNode* self, re::Edx, const uint8_t* entry) {
    self->next = entry ? EntryRef(entry) : int16_t(-1);
}

// 0x43FE00 (thiscall, on 0xBF6228): the entries' and nodes' destructors (back to
// IArchivable's vtable), last first
static void __fastcall Paths_DestructEntries(uint8_t* self, re::Edx) {
    for (int i = 0x1FF; i >= 0; i--) {
        *reinterpret_cast<re::Ptr32<void>*>(self + 0x10734 + i * 0x18) = IArchivable_vftable;
    }
    for (int i = 0x6D5; i >= 0; i--) {
        *reinterpret_cast<re::Ptr32<void>*>(self + 0x19C8 + i * 0x18) = IArchivable_vftable;
    }
}

// 0x43FE40 (thiscall): the link's navi link (nullptr: none)
static NaviLink* __fastcall GetNaviLink(PathNode* self, re::Edx, int i) {
    if (i < self->NumLinks()) {
        int id = ms_naviLinkIds[self->firstLink + i];
        if (id >= 0 && id < ms_numNaviLinks) {
            return &ms_naviLinks[id];
        }
    }
    return nullptr;
}

// 0x43FE80 (thiscall): the next node of a walk from `previous`: from a random link on,
// the first node not back, not switched off or flagged 0x40, with lanes our way, that
// 0x46EFB0 doesn't reject for the ped; with a previous node the one straight on (within
// 100 degrees), else the one closest to straight on (else the previous one)
static PathNode* __fastcall FindNextNode(PathNode* self, re::Edx, PathNode* previous, void* ped) {
    int n = self->NumLinks();
    if (n == 0) {
        return nullptr;
    }
    if (n == 1) {
        return NodeById(ms_linkNodes[self->firstLink] & 0x3FFF);
    }
    PathNode* best = nullptr;
    int start = Rand() % self->NumLinks();
    int i = start;
    bool hasPrevious = previous && NodeIndex(self) < ms_numNodes;
    col::CVector here = {float(self->pos[0]) * 0.125f, float(self->pos[1]) * 0.125f, float(self->pos[2]) * 0.125f};
    col::CVector dir = {0.0f, 0.0f, 0.0f};
    if (hasPrevious) {
        col::CVector back;
        col::CVector diff;
        col::CVector norm;
        Node_GetPosition(previous, &back);
        dir = *VectorNormalise(&norm, VectorSub(&diff, &here, &back), 1.0f);
    }
    float bestDot = -99999997952.0f;
    do {
        int links = self->NumLinks();
        PathNode* node = nullptr;
        if (i >= links) {
            i = 0;
        }
        if (i < links) {
            node = NodeById(ms_linkNodes[self->firstLink + i] & 0x3FFF);
        }
        if (node != previous && !(node->flags14 & 0x60)) {
            int index = NodeIndex(self);
            bool open = true;
            if (index < ms_numNodes) {
                NaviLink* navi = nullptr;
                if (i < links) {
                    int id = ms_naviLinkIds[self->firstLink + i];
                    if (id >= 0 && id < ms_numNaviLinks) {
                        navi = &ms_naviLinks[id];
                    }
                }
                uint8_t lanes = navi->node == index ? (navi->lanes & 0x38) : (navi->lanes & 7);
                open = lanes != 0;
            }
            if (open) {
                col::CVector there = {float(node->pos[0]) * 0.125f, float(node->pos[1]) * 0.125f, float(node->pos[2]) * 0.125f};
                if (!Fn_46EFB0(ms_collisionWorld, ped, &there, 1)) {
                    if (!hasPrevious) {
                        return node;
                    }
                    col::CVector diff;
                    col::CVector norm;
                    VectorNormalise(&norm, VectorSub(&diff, &there, &here), 1.0f);
                    // (x87: y, x, z added in extended precision)
                    float dot = float(double(norm.y) * dir.y + double(norm.x) * dir.x + double(norm.z) * dir.z);
                    if (dot > -0.17364799976348877f) {
                        return node;
                    }
                    if (bestDot < dot) {
                        bestDot = dot;
                        best = node;
                    }
                }
            }
        }
        i++;
        if (i >= self->NumLinks()) {
            i = 0;
        }
    } while (i != start);
    return best ? best : previous;
}

} // namespace PathNodes

// ---- registration ----------------------------------------------------------------------

#define PATHNODES_REVERSED(name, address, fn)                                               \
    static void* RE_CONCAT(Original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace PathNodes {
PATHNODES_REVERSED("CPathNode::GetLinkNodeId", 0x0043F7B0, &GetLinkNodeId);
PATHNODES_REVERSED("CPathNode::GetLinkNode", 0x0043F7E0, &GetLinkNode);
PATHNODES_REVERSED("CPathNode::GetNaviLinkId", 0x0043F820, &GetNaviLinkId);
PATHNODES_REVERSED("CPathNode::FindNaviLinkTo", 0x0043F850, &FindNaviLinkTo);
PATHNODES_REVERSED("CPathNode::GetIndex", 0x0043F8A0, &GetIndex);
PATHNODES_REVERSED("CPathNode::IsLoaded", 0x0043F8C0, &IsLoaded);
PATHNODES_REVERSED("CNaviLink::GetLanesTo", 0x0043F8F0, &GetLanesTo);
PATHNODES_REVERSED("CNaviLink::GetLanesFrom", 0x0043F930, &GetLanesFrom);
PATHNODES_REVERSED("CNaviLink::GetDirection", 0x0043F970, &GetDirection);
PATHNODES_REVERSED("CNaviLink::HasNoLanesFrom", 0x0043F9C0, &HasNoLanesFrom);
PATHNODES_REVERSED("CPaths::Init", 0x0043FA00, &Paths_Init);
PATHNODES_REVERSED("CPaths::ClearFlag40", 0x0043FA40, &Paths_ClearFlag40);
PATHNODES_REVERSED("CNaviLink::GetWidth", 0x0043FA60, &GetWidth);
PATHNODES_REVERSED("CPathNode::Load", 0x0043FA80, &Node_Load);
PATHNODES_REVERSED("CPathLink::Save", 0x0043FC80, &Link_Save);
PATHNODES_REVERSED("CPathNode::SetPrev", 0x0043FD20, &SetPrev);
PATHNODES_REVERSED("CPathNode::SetNext", 0x0043FD90, &SetNext);
PATHNODES_REVERSED("CPaths::DestructEntries", 0x0043FE00, &Paths_DestructEntries);
PATHNODES_REVERSED("CPathNode::GetNaviLink", 0x0043FE40, &GetNaviLink);
PATHNODES_REVERSED("CPathNode::FindNextNode", 0x0043FE80, &FindNextNode);
} // namespace PathNodes
