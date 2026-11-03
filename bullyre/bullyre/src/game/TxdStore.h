#pragma once

// CTxdStore: the texture dictionaries, in a pool of slots. The streamed ones are the
// .nft files in World.img (streaming id TXD_ID_BASE + slot); others are loaded by
// name. (GTA's CTxdStore, on Gamebryo textures instead of RenderWare's.)

#include <cstddef>
#include <cstdint>

#include "game/Pool.h"
#include "reversed.h"

// A dictionary's textures. The textures are Gamebryo objects (refcount at +4).
struct TexDictionary {
    re::Ptr32<struct TextureArray> textures;  // +0 +4 the pointers, +0xA how many
    int32_t bytes;                  // +4 its textures' memory (in the total)
    uint8_t removeWhenUnused;       // +8 a RemoveTxd found it in use
};

struct TxdDef {
    re::Ptr32<TexDictionary> dict;  // +0 nullptr: not loaded
    uint32_t nameHash;    // +4
    int16_t numRefs;      // +8 models (and others) using it
    uint8_t season;       // +0xA the name's last letter: 1 X, 2 F, 3 W, 4 S, 0 none (see ParseName)
    uint8_t unknownB;     // +0xB
    uint8_t fromStream;   // +0xC loaded by LoadTxd (from an archive entry)
    uint8_t pad[3];
};
static_assert(sizeof(TxdDef) == 16);

// The name index: a balanced (AVL) tree over the slots, by (name hash, season). Each
// slot's node holds its two children as (slot << 1 | a balance bit); -1: none.
struct TxdNameTree {
    struct Node {
        int16_t greater;  // +0
        int16_t less;     // +2
    } nodes[10000];
    int16_t root;  // +0x9C40

    // 0x5F1C50 (thiscall): flags 1: an exact match; 2 / 4: else the nearest below /
    // above (-1: none)
    int16_t Find(uint32_t hash, uint32_t season, uint8_t flags);
    // 0x5F1910 (thiscall): adds the slot under its key (nothing if the key's there)
    int16_t Insert(int16_t slot);
    // 0x5F1D70 (thiscall): the node with the key out, its slot (-1: none)
    int16_t Remove(uint32_t hash, uint32_t season);
    // a subtree's height, -1 if it isn't a valid tree (for testing)
    int Check(int16_t node, const struct TxdDef* low, const struct TxdDef* high, int depth) const;

private:
    int16_t Child(int16_t node, int side) const;
    void SetChild(int16_t node, int side, int16_t child);
    int Balance(int16_t node) const;
    void SetBalance(int16_t node, int balance);
    int16_t Rotate(int16_t node, int balance);
    void Replace(int16_t parent, int side, int16_t child);
};
static_assert(offsetof(TxdNameTree, root) == 0x9C40);
static_assert(sizeof(TxdNameTree) == 0x9C42);  // (what Initialise allocates)

// The stream LoadTxd reads from (what OpenStream returns)
struct GameStream {
    int32_t file;       // (the file handle)
    re::Ptr32<uint8_t> data;      // +4
    uint32_t size;      // +8
    uint32_t position;  // +0xC
};

namespace CTxdStore {

GAME_VAR(CPool<TxdDef>*, ms_pTxdPool, 0x01B64654);
GAME_VAR(TexDictionary*, ms_pCurrentTxd, 0x01B64658);  // where models look textures up
GAME_VAR(TexDictionary*, ms_pStoredTxd, 0x01B6465C);
GAME_VAR(int32_t, ms_totalBytes, 0x01B64868);          // all the loaded dictionaries' textures
GAME_VAR(TxdNameTree*, ms_pNameTree, 0x01B64650);      // nullptr: names are looked up slot by slot

// The season the world is in: 1 + the current chapter's entry (a season as in TxdDef)
GAME_VAR(uint8_t, ms_currentChapter, 0x00A147E0);
GAME_ARRAY(int32_t, ms_chapterSeason, 0x00A147EC);
uint8_t CurrentSeason();

// 0x5F2600: the pool (10000 slots) and the name tree
void __cdecl Initialise();
// 0x5F17F0
void __cdecl Shutdown();

// 0x5F0AC0: a dictionary's name to its key. A last letter F, S, W or X is the season
// (and not part of the hashed name).
void __cdecl ParseName(const char* name, uint8_t* season, uint32_t* hash);
// 0x5F22D0: the slot of a name, -1: none. With `seasonal`, the current season's
// version first, then the one without a season.
int __cdecl FindTxdSlot(const char* name, int seasonal);
// 0x5F2450: by hash: the current season's version, the one without a season, then
// `season`'s (-1: any season).
int __cdecl FindTxdSlotByHash(uint32_t hash, int season);
// 0x5F26E0: the slot of a name, made if there's none
int __cdecl AddTxdSlot(const char* name);
// 0x5F2240: frees the slot (unless its dictionary is still in use)
void __cdecl RemoveTxdSlot(int slot);

// 0x5F1030
void __cdecl SetFromStream(int slot, uint8_t value);
// 0x5F1060
uint32_t __cdecl GetNameHash(int slot);
// 0x5F1090
uint8_t __cdecl GetSeason(int slot);
// 0x5F10C0: (the game passes a second argument, unused)
uint8_t __cdecl IsLoaded(int slot);
// 0x5F1100: models made from now on take their textures from this one
void __cdecl SetCurrentTxd(int slot);
// 0x5F1130
void __cdecl AddRef(int slot);
// 0x5F1160: the last one out frees it (through the streaming)
void __cdecl RemoveRef(int slot);
// 0x5F11B0
void __cdecl RemoveRefWithoutDelete(int slot);
// 0x5F11E0
int __cdecl GetNumRefs(int slot);
// 0x5F1210: the textures' memory, -1 when it isn't loaded
int __cdecl GetBytes(int slot);
// 0x5F1830: frees the dictionary, unless something holds one of its textures:
// then it's flagged to be removed later (RemoveUnused)
void __cdecl RemoveTxd(int slot);
// 0x5F18C0
void __cdecl RemoveUnused();
// 0x5F2960: reads the dictionary from an open reader
uint8_t __cdecl LoadTxdFromReader(int slot, void* reader, uint32_t size);
// 0x5F29F0: from a stream (the rest of it)
uint8_t __cdecl LoadTxd(int slot, GameStream* stream);

} // namespace CTxdStore
