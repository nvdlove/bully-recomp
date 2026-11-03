// The TXD name tree (TxdNameTree::Insert/Remove/Find) against a std::set, outside the
// game: the game's variables' addresses are backed with our own memory.
//   build\Release\bullyre_unit.exe

#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <random>
#include <set>
#include <utility>
#include <vector>

#include "game/TxdStore.h"

using Key = std::pair<uint32_t, uint32_t>;  // (hash, season)

static int failures = 0;
#define EXPECT(cond, ...)                          \
    do {                                           \
        if (!(cond)) {                             \
            if (failures++ < 20) {                 \
                printf("FAIL line %d: ", __LINE__); \
                printf(__VA_ARGS__);               \
                printf("\n");                      \
            }                                      \
        }                                          \
    } while (0)

int main() {
    // the game's globals around ms_pTxdPool / ms_pNameTree (0x1B64650..)
    if (!VirtualAlloc(reinterpret_cast<void*>(0x01B60000), 0x10000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)) {
        printf("couldn't back the game's addresses (error %lu)\n", GetLastError());
        return 2;
    }
    const int slots = 10000;
    std::vector<TxdDef> defs(slots);
    std::vector<uint8_t> flags(slots, 0);
    CPool<TxdDef> pool{};
    pool.objects = defs.data();
    pool.flags = flags.data();
    pool.size = slots;
    pool.elemSize = sizeof(TxdDef);
    CTxdStore::ms_pTxdPool = &pool;
    auto* tree = static_cast<TxdNameTree*>(malloc(sizeof(TxdNameTree)));
    tree->root = -1;

    std::mt19937 rng(1234);
    std::set<Key> model;
    std::vector<int> inTree(slots, 0);  // slot -> in the tree
    std::vector<int> used;              // slots in the tree
    int inserts = 0, removes = 0;
    for (int step = 0; step < 200000; step++) {
        bool insert = used.size() < 50 || (rng() % 100) < (used.size() < 4000 ? 60u : 40u);
        if (insert) {
            int slot = int(rng() % slots);
            if (inTree[slot]) {
                continue;
            }
            // few hashes, so equal hashes with different seasons are common
            defs[slot].nameHash = rng() % 3000;
            defs[slot].season = uint8_t(rng() % 5);
            Key key{defs[slot].nameHash, defs[slot].season};
            int16_t got = tree->Insert(int16_t(slot));
            if (model.count(key)) {
                EXPECT(got != slot, "duplicate key added (slot %d)", slot);
                continue;
            }
            EXPECT(got == slot, "insert returned %d for %d", got, slot);
            model.insert(key);
            inTree[slot] = 1;
            used.push_back(slot);
            inserts++;
        } else {
            size_t i = rng() % used.size();
            int slot = used[i];
            Key key{defs[slot].nameHash, defs[slot].season};
            int16_t got = tree->Remove(key.first, key.second);
            EXPECT(got == slot, "remove returned %d, expected %d", got, slot);
            model.erase(key);
            inTree[slot] = 0;
            used[i] = used.back();
            used.pop_back();
            removes++;
        }
        if (step % 97 == 0 || used.size() < 20) {
            EXPECT(tree->Check(tree->root, nullptr, nullptr, 0) >= 0, "tree invalid at step %d", step);
        }
        // lookups: exact, the nearest below (2), the nearest above (4)
        if (step % 13 == 0) {
            Key q{rng() % 3000, rng() % 5};
            int16_t exact = tree->Find(q.first, q.second, 1);
            EXPECT((exact != -1) == (model.count(q) != 0), "exact %u/%u", q.first, q.second);
            auto above = model.upper_bound(q);
            int16_t up = tree->Find(q.first, q.second, 4);
            Key upKey = up == -1 ? Key{0, 0} : Key{defs[up].nameHash, defs[up].season};
            EXPECT((up == -1) == (above == model.end()) && (up == -1 || upKey == *above), "above %u/%u", q.first, q.second);
            auto below = model.lower_bound(q);
            int16_t down = tree->Find(q.first, q.second, 2);
            bool none = below == model.begin();
            Key downKey = down == -1 ? Key{0, 0} : Key{defs[down].nameHash, defs[down].season};
            EXPECT((down == -1) == none && (down == -1 || downKey == *std::prev(below)), "below %u/%u", q.first, q.second);
        }
    }
    // everything out again
    while (!used.empty()) {
        int slot = used.back();
        used.pop_back();
        EXPECT(tree->Remove(defs[slot].nameHash, defs[slot].season) == slot, "final remove %d", slot);
    }
    EXPECT(tree->root == -1, "not empty at the end");
    printf("%d inserts, %d removes: %s\n", inserts, removes, failures ? "FAILED" : "ok");
    return failures ? 1 : 0;
}
