// CTxdStore leftovers: the name tree's balance helpers and rotation as the game has them
// (TxdStore.cpp writes the tree with its own; these are the game's separate functions:
// 0x5F0BC0, 0x5F0BE0, 0x5F0C10), removing the unreferenced slots, and creating a slot's
// dictionary.

#include <cstdint>

#include "game/TxdStore.h"
#include "reversed.h"

namespace {

GAME_FN(int, __thiscall, TxdNameTree_GetBalance, 0x005F0BC0, TxdNameTree* tree, int node);
GAME_FN(void, __cdecl, Txd_RemoveTxdSlot, 0x005F2240, int slot);
GAME_FN(TexDictionary*, __cdecl, TexDictionary_Create, 0x005F25B0);

// a node's two links: [0] the greater side, [1] the less side (child << 1 | a bit)
uint16_t* Links(TxdNameTree* tree, int node) {
    return reinterpret_cast<uint16_t*>(&tree->nodes[node]);
}

int16_t ChildOf(uint16_t link) {
    return int16_t(int16_t(link) >> 1);
}

// 0x5F0BC0 (thiscall): -1 the greater link's bit, else the less link's bit
int __fastcall GetBalance(TxdNameTree* tree, re::Edx, int16_t node) {
    uint16_t* links = Links(tree, node);
    if (links[0] & 1) {
        return -1;
    }
    return links[1] & 1;
}

// 0x5F0BE0 (thiscall): 0 clears both bits; else the less link's bit, and the greater
// link's too below 0 (neither cleared first)
void __fastcall SetBalance(TxdNameTree* tree, re::Edx, int16_t node, int balance) {
    uint16_t* links = Links(tree, node);
    if (balance == 0) {
        links[1] &= 0xFFFE;
        links[0] &= 0xFFFE;
        return;
    }
    links[1] |= 1;
    if (balance < 0) {
        links[0] |= 1;
    }
}

// 0x5F0C10 (thiscall): the node rebalanced by a rotation towards its heavier side
// (single or double), the subtree's new top
int16_t __fastcall Rotate(TxdNameTree* tree, re::Edx, int16_t node) {
    uint16_t* n = Links(tree, node);
    uint16_t greater = n[0];
    if ((greater & 1) == 0 && (n[1] & 1) != 0) {
        // the greater side is higher
        int16_t childIndex = ChildOf(greater);
        uint16_t* c = Links(tree, childIndex);
        if ((c[0] & 1) == 0) {
            n[0] = uint16_t((ChildOf(c[1]) * 2) | (greater & 1));
            uint16_t childLess = c[1];
            uint16_t newLess = uint16_t((childLess & 1) | (node * 2));
            uint16_t childGreater = c[0];
            c[1] = newLess;
            if ((childGreater & 1) == 0 && (childLess & 1) == 0) {
                c[1] = uint16_t(newLess | 1);
                c[0] = uint16_t(childGreater | 1);
                n[1] |= 1;
                return childIndex;
            }
            c[1] = uint16_t(node * 2);
            c[0] = uint16_t(childGreater & 0xFFFE);
            n[1] &= 0xFFFE;
            n[0] &= 0xFFFE;
            return childIndex;
        }
        int16_t grandIndex = ChildOf(c[1]);
        uint16_t* g = Links(tree, grandIndex);
        n[0] = uint16_t((n[0] & 1) | (ChildOf(g[1]) * 2));
        c[1] = uint16_t((c[1] & 1) | (ChildOf(g[0]) * 2));
        g[1] = uint16_t((g[1] & 1) | (node * 2));
        g[0] = uint16_t((g[0] & 1) | (childIndex * 2));
        int balance = TxdNameTree_GetBalance(tree, grandIndex);
        if (balance == 0) {
            n[1] &= 0xFFFE;
            n[0] &= 0xFFFE;
            c[1] &= 0xFFFE;
            c[0] &= 0xFFFE;
            return grandIndex;
        }
        if (balance > 0) {
            n[1] |= 1;
            n[0] |= 1;
            c[1] &= 0xFFFE;
            c[0] &= 0xFFFE;
            g[1] &= 0xFFFE;
            g[0] &= 0xFFFE;
            return grandIndex;
        }
        c[1] |= 1;
        n[1] &= 0xFFFE;
        n[0] &= 0xFFFE;
        g[1] &= 0xFFFE;
        g[0] &= 0xFFFE;
        return grandIndex;
    }
    // the less side is higher
    int16_t childIndex = ChildOf(n[1]);
    uint16_t* c = Links(tree, childIndex);
    int16_t grandIndex = ChildOf(c[0]);
    if ((c[0] & 1) != 0 || (c[1] & 1) == 0) {
        n[1] = uint16_t((n[1] & 1) | (grandIndex * 2));
        uint16_t childGreater = c[0];
        c[0] = uint16_t((node * 2) | (childGreater & 1));
        if ((childGreater & 1) == 0 && (c[1] & 1) == 0) {
            c[1] |= 1;
            n[1] |= 1;
            n[0] |= 1;
            return childIndex;
        }
        c[1] &= 0xFFFE;
        c[0] = uint16_t(node * 2);
        n[1] &= 0xFFFE;
        n[0] &= 0xFFFE;
        return childIndex;
    }
    uint16_t* g = Links(tree, grandIndex);
    n[1] = uint16_t((n[1] & 1) | (ChildOf(g[0]) * 2));
    c[0] = uint16_t((ChildOf(g[1]) * 2) | (c[0] & 1));
    g[0] = uint16_t((g[0] & 1) | (node * 2));
    g[1] = uint16_t((g[1] & 1) | (childIndex * 2));
    int balance = TxdNameTree_GetBalance(tree, grandIndex);
    if (balance == 0) {
        n[1] &= 0xFFFE;
        n[0] &= 0xFFFE;
        c[1] &= 0xFFFE;
        c[0] &= 0xFFFE;
        return grandIndex;
    }
    if (balance < 0) {
        n[1] |= 1;
        c[1] &= 0xFFFE;
        c[0] &= 0xFFFE;
        g[1] &= 0xFFFE;
        g[0] &= 0xFFFE;
        return grandIndex;
    }
    c[1] |= 1;
    c[0] |= 1;
    n[1] &= 0xFFFE;
    n[0] &= 0xFFFE;
    g[1] &= 0xFFFE;
    g[0] &= 0xFFFE;
    return grandIndex;
}

// 0x5F2690 (cdecl): every slot nothing references removed (the loop's 10000 whatever
// the pool's size)
void __cdecl RemoveUnreferencedSlots() {
    for (int slot = 0; slot < 10000; slot++) {
        TxdDef* def = CTxdStore::ms_pTxdPool->GetAt(slot);
        if (def && def->numRefs == 0) {
            Txd_RemoveTxdSlot(slot);
        }
    }
}

// 0x5F28C0 (cdecl): a new (empty) dictionary for the slot (a free slot's GetAt is
// nullptr and the game writes through it: kept)
void __cdecl CreateTxd(int slot) {
    TxdDef* def = CTxdStore::ms_pTxdPool->GetAt(slot);
    TexDictionary* dict = TexDictionary_Create();
    re::StoreVolatile(def, dict);
}

} // namespace

#define TXDSTORE2_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

TXDSTORE2_REVERSED("TxdNameTree::GetBalance", 0x005F0BC0, &GetBalance);
TXDSTORE2_REVERSED("TxdNameTree::SetBalance", 0x005F0BE0, &SetBalance);
TXDSTORE2_REVERSED("TxdNameTree::Rotate", 0x005F0C10, &Rotate);
TXDSTORE2_REVERSED("CTxdStore::RemoveUnreferencedSlots", 0x005F2690, &RemoveUnreferencedSlots);
TXDSTORE2_REVERSED("CTxdStore::CreateTxd", 0x005F28C0, &CreateTxd);
