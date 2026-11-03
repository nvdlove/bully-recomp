// COctTree / COctTreeBase: GTA's colour quantising octree (0x6D1CF0..0x6D2188), used to
// make a texture's palette. The nodes live in a pool (0x20C7C28); children are pool
// slots (int16, -1 none).
//
// COctTree (0x28): +0 vtable (0x943174: 0 InsertColour, 1 FillPalette), +4 the colours
// counted (a leaf: its palette index once filled), +8 leaf, +0xA children[8], +0x1C /
// +0x20 / +0x24 the red / green / blue sums. COctTreeBase (vtable 0x943180) adds +0x28
// the leaves left to make and +0x2C "has a transparent colour" (palette entry 0).

#include <cstdint>

#include "game/Pool.h"
#include "game/VCall.h"
#include "reversed.h"

namespace {

using AnyPool = CPool<uint8_t>;

using re::At;

GAME_VAR(AnyPool*, ot_pool, 0x020C7C28);
GAME_VAR(int32_t, ot_level, 0x020C7C38);   // (the depth while inserting; the next palette index while filling)
GAME_VAR(uint8_t, ot_failed, 0x020C7C34);  // (the pool ran out)
constexpr uintptr_t VT_COctTree = 0x00943174;
constexpr uintptr_t VT_COctTreeBase = 0x00943180;

GAME_FN(uint8_t*, __thiscall, OT_Pool_New, 0x0045CAD0, AnyPool* pool);
GAME_FN(uint8_t*, __thiscall, OT_Construct, 0x006D1CF0, void* node);
GAME_FN(void, __thiscall, OT_Empty, 0x006D1F50, void* node);
GAME_FN(void, __thiscall, OT_DeleteChildren, 0x006D1ED0, void* node);
GAME_FN(void*, __thiscall, OT_DeletingDestructor, 0x006D1E90, void* node, int flags);
GAME_FN(uint8_t, __thiscall, OT_InsertColour, 0x006D1FE0, void* node, int r, int g, int b);
GAME_FN(void, __thiscall, OT_FillPalette, 0x006D1D20, void* node, uint8_t* palette);

int16_t* Children(uint8_t* node) {
    return reinterpret_cast<int16_t*>(node + 0xA);
}

uint8_t* PoolAt(int32_t slot) {
    AnyPool* pool = ot_pool;
    if ((pool->flags[slot] & 0x80) != 0) {
        return nullptr;
    }
    return pool->objects + pool->elemSize * slot;
}

// a node back to the pool (its slot free; the pool's first free lowered)
void PoolFree(uint8_t* node) {
    AnyPool* pool = ot_pool;
    int32_t slot = int32_t(reinterpret_cast<uintptr_t>(node) - re::PtrCast<uintptr_t>(pool->objects)) / pool->elemSize;
    pool->flags[slot] |= 0x80;
    if (slot < At<int32_t>(pool, 0x14)) {
        At<int32_t>(pool, 0x14) = slot;
    }
}

// the children destroyed and freed (their slots -1)
void DeleteChildrenOf(uint8_t* node) {
    int16_t* children = Children(node);
    for (int i = 0; i < 8; i++) {
        if (children[i] < 0) {
            continue;
        }
        uint8_t* child = PoolAt(children[i]);
        if (child != nullptr) {
            At<uint32_t>(child, 0) = VT_COctTree;
            OT_Empty(child);
            PoolFree(child);
        }
        children[i] = -1;
    }
}

// the average of a colour sum (unsigned) over the count, truncated. (The game divides
// at the main thread's 24-bit precision; the double division rounded to float matches
// it barring double rounding.)
uint8_t Average(uint32_t sum, int32_t count) {
    float value = float(double(sum) / double(count));
    return uint8_t(int32_t(value));
}

} // namespace

#define OCTTREE_REVERSED(name, address, fn) REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

// 0x6D1CF0 (thiscall): the constructor
uint8_t* __fastcall COctTree_Construct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = VT_COctTree;
    At<uint32_t>(self, 4) = 0;
    At<uint32_t>(self, 0x1C) = 0;
    At<uint32_t>(self, 0x20) = 0;
    At<uint32_t>(self, 0x24) = 0;
    for (int i = 0; i < 8; i++) {
        Children(self)[i] = -1;
    }
    At<uint8_t>(self, 8) = 0;
    return self;
}
OCTTREE_REVERSED("COctTree::COctTree", 0x006D1CF0, &COctTree_Construct);

// 0x6D1D20 (thiscall): the leaves' average colours into the palette (RGBA, alpha 0x80),
// each leaf then keeping its index
void __fastcall COctTree_FillPalette(uint8_t* self, re::Edx, uint8_t* palette) {
    if (At<uint8_t>(self, 8) == 1) {
        int32_t count = At<int32_t>(self, 4);
        palette[ot_level * 4] = Average(At<uint32_t>(self, 0x1C), count);
        palette[ot_level * 4 + 1] = Average(At<uint32_t>(self, 0x20), count);
        palette[ot_level * 4 + 2] = Average(At<uint32_t>(self, 0x24), count);
        palette[ot_level * 4 + 3] = 0x80;
        At<int32_t>(self, 4) = ot_level;
        ot_level++;
        return;
    }
    for (int i = 0; i < 8; i++) {
        int16_t slot = Children(self)[i];
        if (slot >= 0) {
            vc::Call<void>(PoolAt(slot), 4, palette);   // (FillPalette)
        }
    }
}
OCTTREE_REVERSED("COctTree::FillPalette", 0x006D1D20, &COctTree_FillPalette);

// 0x6D1E60 (thiscall): from index 0, or 1 after a transparent (0) entry
void __fastcall COctTreeBase_FillPalette(uint8_t* self, re::Edx, uint8_t* palette) {
    ot_level = 0;
    if (At<uint32_t>(self, 0x2C) != 0) {
        palette[0] = 0;
        palette[1] = 0;
        palette[2] = 0;
        palette[3] = 0;
        ot_level = 1;
    }
    OT_FillPalette(self, palette);
}
OCTTREE_REVERSED("COctTreeBase::FillPalette", 0x006D1E60, &COctTreeBase_FillPalette);

// 0x6D1E90 (thiscall): the deleting destructor (into the pool)
uint8_t* __fastcall COctTree_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = VT_COctTree;
    OT_Empty(self);
    if ((flags & 1) != 0) {
        PoolFree(self);
    }
    return self;
}
OCTTREE_REVERSED("COctTree::DeletingDestructor", 0x006D1E90, &COctTree_DeletingDestructor);

// 0x6D1ED0 (thiscall): the children deleted
void __fastcall COctTree_DeleteChildren(uint8_t* self, re::Edx) {
    DeleteChildrenOf(self);
}
OCTTREE_REVERSED("COctTree::DeleteChildren", 0x006D1ED0, &COctTree_DeleteChildren);

// 0x6D1F50 (thiscall): emptied: no colours, no children, not a leaf
void __fastcall COctTree_Empty(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 4) = 0;
    At<uint32_t>(self, 0x1C) = 0;
    At<uint32_t>(self, 0x20) = 0;
    At<uint32_t>(self, 0x24) = 0;
    DeleteChildrenOf(self);
    At<uint8_t>(self, 8) = 0;
}
OCTTREE_REVERSED("COctTree::Empty", 0x006D1F50, &COctTree_Empty);

// 0x6D1FE0 (thiscall): adds a colour down the tree (a bit of each channel per level);
// at depth 8, or at a leaf, it's a leaf. Returns true for a new leaf. A pool running out
// sets the failed flag, dropping the branch.
uint8_t __fastcall COctTree_InsertColour(uint8_t* self, re::Edx, int red, int green, int blue) {
    uint32_t r = uint8_t(red);
    uint32_t g = uint8_t(green);
    uint32_t b = uint8_t(blue);
    int32_t level = ot_level;
    int32_t index = ((int32_t(b << level) >> 7) & 1) + ((int32_t(g << level) >> 6) & 2) + ((int32_t(r << level) >> 5) & 4);
    ot_level = level + 1;
    At<uint32_t>(self, 0x1C) += r;
    At<uint32_t>(self, 0x20) += g;
    At<uint32_t>(self, 0x24) += b;
    At<int32_t>(self, 4) += 1;
    if (ot_level == 8 || At<uint8_t>(self, 8) == 1) {
        ot_level = 0;
        At<uint8_t>(self, 8) = 1;
        OT_DeleteChildren(self);
        return At<int32_t>(self, 4) == 1 ? 1 : 0;
    }
    int16_t slot = Children(self)[index];
    uint8_t* child;
    if (slot < 0) {
        uint8_t* memory = OT_Pool_New(ot_pool);
        child = memory != nullptr ? OT_Construct(memory) : nullptr;
        if (child == nullptr) {
            ot_failed = 1;
            return 0;
        }
        AnyPool* pool = ot_pool;
        Children(self)[index] =
            int16_t(int32_t(reinterpret_cast<uintptr_t>(child) - re::PtrCast<uintptr_t>(pool->objects)) / pool->elemSize);
    } else {
        child = PoolAt(slot);
    }
    uint8_t result = vc::Call<uint8_t>(child, 0, int(r), int(g), int(b));   // (InsertColour)
    if (ot_failed != 0 && At<int32_t>(child, 4) < 2) {
        OT_DeletingDestructor(child, 1);
        Children(self)[index] = -1;
        return 0;
    }
    return result;
}
OCTTREE_REVERSED("COctTree::InsertColour", 0x006D1FE0, &COctTree_InsertColour);

// 0x6D2110 (thiscall): from the root: a new leaf counts down the leaves left
uint8_t __fastcall COctTreeBase_InsertColour(uint8_t* self, re::Edx, int red, int green, int blue) {
    ot_level = 0;
    if (OT_InsertColour(self, red, green, blue) != 0) {
        At<int32_t>(self, 0x28) -= 1;
        return 1;
    }
    return 0;
}
OCTTREE_REVERSED("COctTreeBase::InsertColour", 0x006D2110, &COctTreeBase_InsertColour);

// 0x6D2150 (thiscall): the destructor
void __fastcall COctTree_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = VT_COctTree;
    OT_Empty(self);
}
OCTTREE_REVERSED("COctTree::~COctTree", 0x006D2150, &COctTree_Destruct);

// 0x6D2160 (thiscall): COctTreeBase's constructor
uint8_t* __fastcall COctTreeBase_Construct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 4) = 0;
    At<uint32_t>(self, 0x1C) = 0;
    At<uint32_t>(self, 0x20) = 0;
    At<uint32_t>(self, 0x24) = 0;
    for (int i = 0; i < 8; i++) {
        Children(self)[i] = -1;
    }
    At<uint8_t>(self, 8) = 0;
    At<uint32_t>(self, 0) = VT_COctTreeBase;
    return self;
}
OCTTREE_REVERSED("COctTreeBase::COctTreeBase", 0x006D2160, &COctTreeBase_Construct);
