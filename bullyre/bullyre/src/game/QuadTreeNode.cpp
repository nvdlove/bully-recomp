// CQuadTreeNode (0x576F20..0x5772F0): GTA's quad tree of rectangles (nodes from the pool
// at *0xCF3568; the leaves hold linked lists of items), plus three small helpers next to
// it: the name hash (0x576ED0), a dword setter (0x5772F0) and the random float
// generator (0x577300).

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "game/Collision.h"
#include "reversed.h"

namespace {

using col::CRect;

// An item link (0xC)
struct QuadTreeItem {
    re::Ptr32<QuadTreeItem> next;  // +0
    re::Ptr32<QuadTreeItem> prev;  // +4
    re::Ptr32<void> item;          // +8
};
static_assert(sizeof(QuadTreeItem) == 0xC);

// A node (0x30)
struct CQuadTreeNode {
    CRect rect;                  // +0 (left, top, right, bottom; top is the larger y)
    re::Ptr32<QuadTreeItem> head;          // +0x10
    re::Ptr32<QuadTreeItem> tail;          // +0x14
    int32_t count;               // +0x18
    re::Ptr32<CQuadTreeNode> children[4];  // +0x1C top-left, top-right, bottom-left, bottom-right
    int32_t level;               // +0x2C 0: a leaf
};
static_assert(offsetof(CQuadTreeNode, head) == 0x10);
static_assert(offsetof(CQuadTreeNode, children) == 0x1C);
static_assert(offsetof(CQuadTreeNode, level) == 0x2C);
static_assert(sizeof(CQuadTreeNode) == 0x30);

GAME_FN(void*, __cdecl, QT_Malloc, 0x005EEAA0, size_t size);
GAME_FN(void*, __thiscall, QT_PoolNew, 0x0056EDF0, void* pool);
GAME_FN(uint32_t, __thiscall, QT_OverlapsWith, 0x0056ED60, const CRect* self, const CRect* other);
inline char* __cdecl QT_StrUpr(char* s) { return crt::Strupr(s); }   // _strupr
// ours, through the game's addresses
GAME_FN(int, __thiscall, QT_FindSector, 0x00576F20, const CQuadTreeNode* node, const float* point);
GAME_FN(uint8_t, __thiscall, QT_InSector, 0x00577030, const CQuadTreeNode* node, const CRect* rect, int sector);
GAME_FN(void, __thiscall, QT_AddItem, 0x00577120, CQuadTreeNode* node, void* item, const CRect* rect);

GAME_VAR(void*, g_quadTreeNodePool, 0x00CF3568);
GAME_ARRAY(char, g_hashNameBuffer, 0x00CF3168);

// a sector's rectangle (the node's quartered)
CRect SectorRect(const CRect& r, int sector) {
    CRect s = r;
    switch (sector) {
    case 0:
        s.right = (s.left + s.right) * 0.5f;
        s.bottom = (s.bottom + s.top) * 0.5f;
        break;
    case 1:
        s.left = (s.left + s.right) * 0.5f;
        s.bottom = (s.bottom + s.top) * 0.5f;
        break;
    case 2:
        s.right = (s.left + s.right) * 0.5f;
        s.top = (s.bottom + s.top) * 0.5f;
        break;
    case 3:
        s.left = (s.left + s.right) * 0.5f;
        s.top = (s.bottom + s.top) * 0.5f;
        break;
    }
    return s;
}

} // namespace

namespace QuadTree {

// 0x576F20 (thiscall): the sector the point is in (-1 at a leaf)
int __fastcall FindSector(const CQuadTreeNode* self, re::Edx, const float* point) {
    float midX = (self->rect.right + self->rect.left) * 0.5f;
    if (self->level == 0) {
        return -1;
    }
    float midY = (self->rect.bottom + self->rect.top) * 0.5f;
    bool upper = !(midY > point[1]);
    if (midX > point[0]) {
        return upper ? 0 : 2;
    }
    return upper ? 1 : 3;
}

// 0x576FA0 (thiscall)
CQuadTreeNode* __fastcall Construct(CQuadTreeNode* self, re::Edx, const CRect* rect, int level) {
    self->rect = *rect;
    self->head = nullptr;
    self->tail = nullptr;
    self->count = 0;
    self->level = level;
    for (int i = 0; i < 4; i++) {
        self->children[i] = nullptr;
    }
    return self;
}

// 0x576FE0 (thiscall): the callback for each item on the way down to the point's leaf
void __fastcall ForAllMatching(CQuadTreeNode* self, re::Edx, const float* point, void(__cdecl* callback)(const float*, void*)) {
    CQuadTreeNode* node = self;
    for (;;) {
        for (QuadTreeItem* link = node->head; link != nullptr; link = link->next) {
            callback(point, link->item);
        }
        int sector = QT_FindSector(node, point);
        if (sector == -1 || node->children[sector] == nullptr) {
            return;
        }
        node = node->children[sector];
    }
}

// 0x577030 (thiscall): the rectangle overlaps the sector (never at a leaf)
uint8_t __fastcall InSector(const CQuadTreeNode* self, re::Edx, const CRect* rect, int sector) {
    CRect sectorRect = self->rect;
    if (self->level == 0) {
        return 0;
    }
    sectorRect = SectorRect(sectorRect, sector);
    return QT_OverlapsWith(&sectorRect, rect) != 0;
}

// 0x577120 (thiscall): the item into the leaves its rectangle overlaps (the nodes made
// on the way from the pool)
void __fastcall AddItem(CQuadTreeNode* self, re::Edx, void* item, const CRect* rect) {
    if (self->level == 0) {
        QuadTreeItem* link = static_cast<QuadTreeItem*>(QT_Malloc(0xC));
        if (link != nullptr) {
            link->next = nullptr;
            link->prev = nullptr;
            link->item = item;
        } else {
            link = nullptr;
        }
        QuadTreeItem* tail = self->tail;
        if (self->head == tail && tail == nullptr) {
            self->head = link;
            link->prev = tail;
            self->count++;
            self->tail = link;
            return;
        }
        tail->next = link;
        link->prev = self->tail;
        self->count++;
        self->tail = link;
        return;
    }
    for (int sector = 0; sector < 4; sector++) {
        if (!QT_InSector(self, rect, sector)) {
            continue;
        }
        if (self->children[sector] == nullptr) {
            CRect childRect = SectorRect(self->rect, sector);
            CQuadTreeNode* child = static_cast<CQuadTreeNode*>(QT_PoolNew(g_quadTreeNodePool));
            if (child != nullptr) {
                child->rect = childRect;
                child->head = nullptr;
                child->tail = nullptr;
                child->count = 0;
                child->level = self->level - 1;
                for (int i = 0; i < 4; i++) {
                    child->children[i] = nullptr;
                }
            } else {
                child = nullptr;
            }
            self->children[sector] = child;
        }
        QT_AddItem(self->children[sector], item, rect);
    }
}

} // namespace QuadTree

namespace NameHash {

// 0x576ED0: a name's hash: upper-cased (in a static buffer), h * 131 + c, 31 bits
uint32_t __cdecl HashName(const char* name) {
    char* buffer = g_hashNameBuffer;
    size_t i = 0;
    char c;
    do {
        c = name[i];
        buffer[i] = c;
        i++;
    } while (c != 0);
    const char* p = QT_StrUpr(buffer);
    uint32_t hash = 0;
    for (; *p != 0; p++) {
        hash = hash * 0x83 + uint32_t(int32_t(*p));
    }
    return hash & 0x7FFFFFFF;
}

// 0x5772F0 (thiscall): *this = value
void __fastcall SetDword(uint32_t* self, re::Edx, uint32_t value) {
    *self = value;
}

// 0x577300 (thiscall on the generator's state): the next random float in [0, 1) (an
// LCG's low 23 bits as the mantissa of a number in [1, 2))
float __fastcall RandomFloat(uint32_t* state, re::Edx) {
    uint32_t x = *state * 0x19660Du + 0x3C6EF35Fu;
    *state = x;
    return std::bit_cast<float>((x & 0x7FFFFFu) | 0x3F800000u) - 1.0f;
}

} // namespace NameHash

// ---- registration: all original until tested ---------------------------------------

#define QUADTREE_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(quadtree_original_, __LINE__);                                    \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(quadtree_original_, __LINE__), re::Mode::Original)

QUADTREE_REVERSED("CQuadTreeNode::FindSector", 0x00576F20, &QuadTree::FindSector);
QUADTREE_REVERSED("CQuadTreeNode::CQuadTreeNode", 0x00576FA0, &QuadTree::Construct);
QUADTREE_REVERSED("CQuadTreeNode::ForAllMatching", 0x00576FE0, &QuadTree::ForAllMatching);
QUADTREE_REVERSED("CQuadTreeNode::InSector", 0x00577030, &QuadTree::InSector);
QUADTREE_REVERSED("CQuadTreeNode::AddItem", 0x00577120, &QuadTree::AddItem);
QUADTREE_REVERSED("CKeyGen::HashName_576ED0", 0x00576ED0, &NameHash::HashName);
QUADTREE_REVERSED("Dword::Set_5772F0", 0x005772F0, &NameHash::SetDword);
QUADTREE_REVERSED("CRandom::GetFloat_577300", 0x00577300, &NameHash::RandomFloat);
