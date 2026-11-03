// A collision model's triangle tree (0x5611B0..0x5618E0; CCollisionData+0x14, called
// TrianglePlanes in Collision.h): a bounding box split into two child boxes per node
// (each bound a byte fraction of the parent's), the leaves runs of triangles. Queried
// with a sphere, a vertical cylinder or a box through a static stack (0xCF2168..): the
// first triangle, then the next ones until -1.

#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace {

struct CVector {
    float x, y, z;
};

// a node (12 bytes)
struct TreeNode {
    uint8_t minFraction[3];  // +0 each moves one face of a child's box in: the second
    uint8_t flags;           // +3 child's unless its bit (0..5) is set; bit 6: the first is a
    uint8_t maxFraction[3];  // +4 leaf, bit 7: the second is
    uint8_t counts;          // +7 the leaves' triangle counts: low nibble the first's, high the second's
    uint16_t first;          // +8 a node, or a leaf's first triangle
    uint16_t second;         // +0xA
};
static_assert(sizeof(TreeNode) == 0xC);

struct TriangleTree {
    float min[4];            // +0x00 (w: whatever the loader's stack held)
    float max[4];            // +0x10
    int32_t numNodes;        // +0x20
    re::Ptr32<TreeNode> nodes;         // +0x24
};
static_assert(sizeof(TriangleTree) == 0x28);

// CColBox: min, max (with w), surface, piece
struct ColBox {
    float min[4];            // +0
    float max[4];            // +0x10
    uint8_t surface;         // +0x20
    uint8_t piece;           // +0x21
    uint8_t pad[2];
};
static_assert(sizeof(ColBox) == 0x24);

struct StackEntry {
    uint16_t node;           // +0 | 0x8000: the node's second child
    uint8_t pad[2];
    ColBox box;              // +4 the child's box
};
static_assert(sizeof(StackEntry) == 0x28);

enum { QUERY_NONE = 0, QUERY_SPHERE = 1, QUERY_BOX = 2, QUERY_CYLINDER = 3 };

GAME_VAR(TriangleTree*, ms_tree, 0x00CF2168);
GAME_VAR(uint16_t, ms_leafIndex, 0x00CF216C);
GAME_VAR(uint16_t, ms_leafCount, 0x00CF2170);
GAME_VAR(uint16_t, ms_leafFirst, 0x00CF2174);
GAME_VAR(uint16_t, ms_depth, 0x00CF2178);
GAME_VAR(int32_t, ms_queryType, 0x00CF217C);
GAME_ARRAY(float, ms_sphere, 0x00CF2180);    // a CColSphere (the cylinder: its x, y and radius)
GAME_ARRAY(uint32_t, ms_box, 0x00CF2194);    // [8] min, max
GAME_ARRAY(StackEntry, ms_stack, 0x00CF21B8); // [32]

GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __thiscall, ColSphere_Set, 0x0056EBA0, float* sphere, float radius, const CVector* center, int surface,
        int piece);
GAME_FN(void, __thiscall, ColBox_Set, 0x005648A0, ColBox* box, const float* min, const float* max, int surface,
        int piece);
GAME_FN(uint8_t, __cdecl, SphereTouchesBox, 0x00564B50, const float* sphere, const float* min, const float* max);
GAME_FN(uint8_t, __cdecl, BoxTouchesBox, 0x00564E90, const uint32_t* box, const float* min, const float* max);
GAME_FN(float, __cdecl, DistSqToRect2D, 0x00564310, const float* min, const float* max, const float* point);
GAME_FN(uint16_t, __cdecl, Tree_Pop, 0x005616F0);

// the box against the query
bool Touches(const float* min, const float* max, float radiusSq) {
    if (ms_queryType == QUERY_SPHERE) {
        return SphereTouchesBox(ms_sphere, min, max) != 0;
    }
    if (ms_queryType == QUERY_BOX) {
        return BoxTouchesBox(ms_box, min, max) != 0;
    }
    if (ms_queryType != QUERY_CYLINDER) {
        return false;
    }
    double x = ms_sphere[0], y = ms_sphere[1], r = ms_sphere[3];
    if (!(min[0] < x + r) || !(max[0] > x - r) || !(min[1] < y + r) || !(max[1] > y - r)) {
        return false;
    }
    float point[2] = {float(x), float(y)};
    return DistSqToRect2D(min, max, point) < radiusSq;
}

void Push(uint16_t node, const float* min, const float* max) {
    StackEntry& e = ms_stack[ms_depth];
    e.node = node;
    ColBox_Set(&e.box, min, max, 0, 0);
    ms_depth += 1;
}

// ---- 0x5611D0 (min in eax, max in ecx, the node in bx): a node's children that the
// query touches pushed (the second one first)
void PushChildren(const float* min, const float* max, uint16_t node) {
    float size[3] = {max[0] - min[0], max[1] - min[1], max[2] - min[2]};
    float secondMin[4] = {min[0], min[1], min[2], min[3]};
    float secondMax[4] = {max[0], max[1], max[2], max[3]};
    float firstMin[4] = {min[0], min[1], min[2], min[3]};
    float firstMax[4] = {max[0], max[1], max[2], max[3]};
    const TreeNode& n = ms_tree->nodes[node];
    uint8_t flags = n.flags;
    for (int i = 0; i < 3; i++) {
        float v = float(double(size[i]) * n.minFraction[i] / 255.0 + min[i]);
        if (flags & (1 << i)) {
            firstMin[i] = v;
        } else {
            secondMin[i] = v;
        }
    }
    for (int i = 0; i < 3; i++) {
        float v = float(double(n.maxFraction[i]) * size[i] / 255.0 + min[i]);
        if (flags & (8 << i)) {
            firstMax[i] = v;
        } else {
            secondMax[i] = v;
        }
    }
    float radiusSq = 0.0f;
    if (ms_queryType == QUERY_CYLINDER) {
        radiusSq = ms_sphere[3] * ms_sphere[3];
    }
    if (Touches(secondMin, secondMax, radiusSq)) {
        Push(uint16_t(node | 0x8000), secondMin, secondMax);
    }
    if (Touches(firstMin, firstMax, radiusSq)) {
        Push(node, firstMin, firstMax);
    }
}

// ---- 0x5611B0 (thiscall): the nodes freed
void __fastcall Tree_Destroy(TriangleTree* self, re::Edx) {
    Free(self->nodes);
    self->nodes = nullptr;
    self->numNodes = 0;
}

// ---- 0x5616F0: the next leaf off the stack (inner nodes' children pushed on the way):
// its first triangle, the rest by Next; -1 at the end (the query is over)
uint16_t __cdecl Pop() {
    while (ms_depth != 0) {
        StackEntry& e = ms_stack[ms_depth - 1];
        uint16_t node = e.node;
        const TreeNode& n = ms_tree->nodes[node & 0x7FFF];
        bool second = (node >> 15) != 0;
        uint8_t leaf = second ? uint8_t(n.flags >> 7) : uint8_t((n.flags >> 6) & 1);
        ms_depth -= 1;
        uint16_t child = second ? n.second : n.first;
        if (leaf) {
            ms_leafFirst = child;
            ms_leafCount = second ? uint16_t(n.counts >> 4) : uint16_t(n.counts & 0xF);
            ms_leafIndex = 1;
            return child;
        }
        PushChildren(e.box.min, e.box.max, child);
    }
    ms_queryType = QUERY_NONE;
    return 0xFFFF;
}

// ---- 0x5617E0 (thiscall)
TriangleTree* __fastcall Tree_Construct(TriangleTree* self, re::Edx) {
    self->numNodes = 0;
    self->nodes = nullptr;
    return self;
}

// ---- 0x5617F0 (thiscall): the first triangle near a sphere (or, flagged, a vertical
// cylinder through it)
uint16_t __fastcall Tree_FirstInSphere(TriangleTree* self, re::Edx, const CVector* center, float radius, int cylinder) {
    ms_queryType = re::Flag(cylinder) ? QUERY_CYLINDER : QUERY_SPHERE;
    ms_tree = self;
    ms_depth = 0;
    ColSphere_Set(ms_sphere, radius, center, 0, 0);
    ms_leafIndex = 0;
    ms_leafCount = 0;
    PushChildren(self->min, self->max, 0);
    return Tree_Pop();
}

// ---- 0x561850 (thiscall): the first triangle near a box
uint16_t __fastcall Tree_FirstInBox(TriangleTree* self, re::Edx, const uint32_t* box) {
    ms_tree = self;
    for (int i = 0; i < 8; i++) {
        ms_box[i] = box[i];
    }
    ms_queryType = QUERY_BOX;
    ms_depth = 0;
    ms_leafIndex = 0;
    ms_leafCount = 0;
    PushChildren(self->min, self->max, 0);
    return Tree_Pop();
}

// ---- 0x5618A0: the next triangle (-1: none)
uint16_t __cdecl Next() {
    if (ms_queryType == QUERY_NONE) {
        return 0xFFFF;
    }
    if (ms_leafIndex < ms_leafCount) {
        return uint16_t(ms_leafFirst + ms_leafIndex++);
    }
    return Tree_Pop();
}

// ---- 0x5618E0 (thiscall): from the file (the first argument isn't read): the bounds
// and the nodes, whose bytes are in another order there. (The bounds' w are whatever
// was on the game's stack: 0 here.)
void __fastcall Tree_Load(TriangleTree* self, re::Edx, int, const CVector* min, const CVector* max, int numNodes,
                          const uint8_t* data) {
    self->min[0] = min->x;
    self->min[1] = min->y;
    self->min[2] = min->z;
    self->min[3] = 0.0f;
    self->max[0] = max->x;
    self->max[1] = max->y;
    self->max[2] = max->z;
    self->max[3] = 0.0f;
    self->numNodes = numNodes;
    Free(self->nodes);
    uint64_t bytes = uint64_t(uint32_t(self->numNodes)) * 12;
    TreeNode* nodes = static_cast<TreeNode*>(Malloc((bytes >> 32) ? size_t(-1) : size_t(bytes)));
    self->nodes = nodes;
    for (int i = 0; i < numNodes; i++) {
        const uint8_t* s = data + i * 12;
        uint8_t* d = reinterpret_cast<uint8_t*>(&nodes[i]);
        d[3] = s[0];
        d[0] = s[2];
        d[1] = s[3];
        d[2] = s[4];
        d[4] = s[5];
        d[5] = s[6];
        d[6] = s[7];
        d[7] = s[1];
        nodes[i].second = *reinterpret_cast<const uint16_t*>(s + 10);
        nodes[i].first = *reinterpret_cast<const uint16_t*>(s + 8);
    }
    for (int i = 0; i < 32; i++) {
        ms_stack[i].node = 0xFFFF;
    }
}

} // namespace

RE_UNHOOKED("TriangleTree::PushChildren", 0x005611D0);

#define COLTREE_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(coltree_original_, __LINE__);                                        \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(coltree_original_, __LINE__), \
             re::Mode::Original)

COLTREE_REVERSED("TriangleTree::Destroy", 0x005611B0, &Tree_Destroy);
COLTREE_REVERSED("TriangleTree::Pop", 0x005616F0, &Pop);
COLTREE_REVERSED("TriangleTree::TriangleTree", 0x005617E0, &Tree_Construct);
COLTREE_REVERSED("TriangleTree::FirstInSphere", 0x005617F0, &Tree_FirstInSphere);
COLTREE_REVERSED("TriangleTree::FirstInBox", 0x00561850, &Tree_FirstInBox);
COLTREE_REVERSED("TriangleTree::Next", 0x005618A0, &Next);
COLTREE_REVERSED("TriangleTree::Load", 0x005618E0, &Tree_Load);
