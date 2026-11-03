#include "game/TxdStore.h"

#include <cstring>

#include "game/Streaming.h"
#include "log.h"

namespace game {
// the Gamebryo side, not reversed yet
GAME_FN(uint8_t, __thiscall, TexDictionary_IsInUse, 0x005F1240, TexDictionary* dict);   // a texture held elsewhere
GAME_FN(void, __cdecl, TexDictionary_Destroy, 0x005F17B0, TexDictionary* dict, int);
GAME_FN(TexDictionary*, __cdecl, TexDictionary_Create, 0x005F25B0);
GAME_FN(uint8_t, __cdecl, TexDictionary_Read, 0x005F2750, void* reader, TexDictionary* dict, const char*, const char*);
// the reader LoadTxd builds on its stack (0x40C bytes)
GAME_FN(void, __thiscall, Reader_Construct, 0x0075CE40, void* reader);
GAME_FN(void, __thiscall, Reader_Open, 0x007599F0, void* reader, void* data, uint32_t size);
GAME_FN(void, __thiscall, Reader_Close, 0x0075B4D0, void* reader);
GAME_FN(void, __thiscall, Reader_Destruct, 0x0075C020, void* reader);
inline const char* const emptyString = reinterpret_cast<const char*>(0x00900434);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, PushTxdMemoryTag, 0x0052E700);
GAME_FN(void, __cdecl, PopTxdMemoryTag, 0x0052E710);
GAME_FN(void, __thiscall, TxdPool_Init, 0x005F0F10, CPool<TxdDef>* pool);   // the arrays, for `size` slots
GAME_FN(void, __thiscall, Pool_Flush, 0x006D2370, void* pool);
GAME_FN(TxdDef*, __thiscall, TxdPool_New, 0x005F1640, CPool<TxdDef>* pool);
// (CTxdStore's own, called where modes apply)
GAME_FN(void, __thiscall, TxdNameTree_Insert, 0x005F1910, TxdNameTree* tree, int slot);
GAME_FN(int16_t, __thiscall, TxdNameTree_Remove, 0x005F1D70, TxdNameTree* tree, uint32_t hash, uint32_t season);
GAME_FN(int16_t, __thiscall, TxdNameTree_Find, 0x005F1C50, TxdNameTree* tree, uint32_t hash, uint32_t season, uint8_t flags);
GAME_FN(int16_t, __thiscall, TxdNameTree_FindExact, 0x005F21D0, TxdNameTree* tree, uint32_t hash, uint32_t season);
GAME_FN(int16_t, __thiscall, TxdNameTree_FindAnySeason, 0x005F21F0, TxdNameTree* tree, uint32_t hash);
GAME_FN(int, __cdecl, Txd_FindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(void, __cdecl, Txd_RemoveTxd, 0x005F1830, int slot);
GAME_FN(uint8_t, __cdecl, Txd_LoadTxdFromReader, 0x005F2960, int slot, void* reader, uint32_t size);
} // namespace game

namespace CTxdStore {

static TxdDef* Def(int slot) {
    return ms_pTxdPool->GetAt(slot);
}

void __cdecl SetFromStream(int slot, uint8_t value) {
    Def(slot)->fromStream = value;
}

uint32_t __cdecl GetNameHash(int slot) {
    return Def(slot)->nameHash;
}

uint8_t __cdecl GetSeason(int slot) {
    return Def(slot)->season;
}

uint8_t __cdecl IsLoaded(int slot) {
    return Def(slot)->dict != nullptr;
}

void __cdecl SetCurrentTxd(int slot) {
    ms_pCurrentTxd = Def(slot)->dict;
}

void __cdecl AddRef(int slot) {
    Def(slot)->numRefs++;
}

void __cdecl RemoveRef(int slot) {
    TxdDef* def = Def(slot);
    if (def->numRefs > 0 && --def->numRefs <= 0) {
        game::RemoveModel(slot + TXD_ID_BASE);
    }
}

void __cdecl RemoveRefWithoutDelete(int slot) {
    Def(slot)->numRefs--;
}

int __cdecl GetNumRefs(int slot) {
    return Def(slot)->numRefs;
}

int __cdecl GetBytes(int slot) {
    TxdDef* def = Def(slot);
    if (!def || !def->dict) {
        return -1;
    }
    return def->dict->bytes;
}

void __cdecl RemoveTxd(int slot) {
    TxdDef* def = Def(slot);
    if (!def || !def->dict) {
        return;
    }
    TexDictionary* dict = def->dict;
    if (game::TexDictionary_IsInUse(dict)) {
        if (!dict->removeWhenUnused) {
            dict->removeWhenUnused = 1;
        }
        return;
    }
    if (ms_pCurrentTxd == dict) {
        ms_pCurrentTxd = nullptr;
    }
    if (ms_pStoredTxd == dict) {
        ms_pStoredTxd = nullptr;
    }
    ms_totalBytes -= dict->bytes;
    game::TexDictionary_Destroy(dict, 1);
    def->dict = nullptr;
    def->fromStream = 0;
    def->unknownB = 0;
}

void __cdecl RemoveUnused() {
    // (the game's loop stops at 10000 whatever the pool's size)
    for (int slot = 0; slot < 10000; slot++) {
        TxdDef* def = Def(slot);
        if (def && def->dict && def->dict->removeWhenUnused) {
            game::Txd_RemoveTxd(slot);
        }
    }
}

uint8_t __cdecl LoadTxdFromReader(int slot, void* reader, uint32_t) {
    TxdDef* def = Def(slot);
    if (def->dict) {
        def->dict->removeWhenUnused = 0;
        return 1;
    }
    def->dict = game::TexDictionary_Create();
    if (!def->dict) {
        return 0;
    }
    uint8_t ok = game::TexDictionary_Read(reader, def->dict, game::emptyString, game::emptyString);
    game::Reader_Close(reader);
    ms_totalBytes += def->dict->bytes;
    return ok;
}

uint8_t __cdecl LoadTxd(int slot, GameStream* stream) {
    TxdDef* def = Def(slot);
    if (def->dict) {
        def->dict->removeWhenUnused = 0;
        return 1;
    }
    alignas(16) uint8_t reader[0x40C];
    game::Reader_Construct(reader);
    uint32_t size = stream->size - stream->position;
    game::Reader_Open(reader, stream->data, size);
    def->fromStream = 1;
    uint8_t ok = game::Txd_LoadTxdFromReader(slot, reader, size);
    game::Reader_Destruct(reader);
    return ok;
}

// ---- shadows: the getters, compared with the game's --------------------------------

template <typename R>
static void Compare(const char* name, uint32_t& compared, int slot, R theirs, R ours) {
    re::Compared(name, compared);
    if (theirs != ours) {
        re::Mismatch(name, "slot %d: game %d, ours %d", slot, int(theirs), int(ours));
    }
}

static uint8_t(__cdecl* Original_IsLoaded)(int);
static int(__cdecl* Original_GetNumRefs)(int);
static int(__cdecl* Original_GetBytes)(int);
static uint32_t(__cdecl* Original_GetNameHash)(int);

static uint8_t __cdecl Shadow_IsLoaded(int slot) {
    uint8_t theirs = Original_IsLoaded(slot);
    static uint32_t compared = 0;
    Compare<uint8_t>("CTxdStore::IsLoaded", compared, slot, theirs, IsLoaded(slot));
    return theirs;
}
static int __cdecl Shadow_GetNumRefs(int slot) {
    int theirs = Original_GetNumRefs(slot);
    static uint32_t compared = 0;
    Compare<int>("CTxdStore::GetNumRefs", compared, slot, theirs, GetNumRefs(slot));
    return theirs;
}
static int __cdecl Shadow_GetBytes(int slot) {
    int theirs = Original_GetBytes(slot);
    static uint32_t compared = 0;
    Compare<int>("CTxdStore::GetBytes", compared, slot, theirs, GetBytes(slot));
    return theirs;
}
static uint32_t __cdecl Shadow_GetNameHash(int slot) {
    uint32_t theirs = Original_GetNameHash(slot);
    static uint32_t compared = 0;
    Compare<uint32_t>("CTxdStore::GetNameHash", compared, slot, theirs, GetNameHash(slot));
    return theirs;
}

#define TXD_SHADOWED(fn, address, mode)                                                                                   \
    REVERSED("CTxdStore::" #fn, address, RE_BODY(&fn), reinterpret_cast<void*>(&Shadow_##fn), \
             reinterpret_cast<void**>(&Original_##fn), mode)
#define TXD_REVERSED(fn, address, mode)                     \
    static decltype(&fn) Original_##fn;                     \
    REVERSED("CTxdStore::" #fn, address, RE_BODY(&fn), nullptr, \
             reinterpret_cast<void**>(&Original_##fn), mode)

TXD_REVERSED(SetFromStream, 0x005F1030, re::Mode::Ours);
TXD_SHADOWED(GetNameHash, 0x005F1060, re::Mode::Ours);
TXD_REVERSED(GetSeason, 0x005F1090, re::Mode::Ours);
TXD_SHADOWED(IsLoaded, 0x005F10C0, re::Mode::Ours);
TXD_REVERSED(SetCurrentTxd, 0x005F1100, re::Mode::Ours);
TXD_REVERSED(AddRef, 0x005F1130, re::Mode::Ours);
TXD_REVERSED(RemoveRef, 0x005F1160, re::Mode::Ours);
TXD_REVERSED(RemoveRefWithoutDelete, 0x005F11B0, re::Mode::Ours);
TXD_SHADOWED(GetNumRefs, 0x005F11E0, re::Mode::Ours);
TXD_SHADOWED(GetBytes, 0x005F1210, re::Mode::Ours);
TXD_REVERSED(RemoveTxd, 0x005F1830, re::Mode::Ours);
TXD_REVERSED(RemoveUnused, 0x005F18C0, re::Mode::Ours);
TXD_REVERSED(LoadTxdFromReader, 0x005F2960, re::Mode::Ours);
TXD_REVERSED(LoadTxd, 0x005F29F0, re::Mode::Ours);

} // namespace CTxdStore

// ---- names ---------------------------------------------------------------------------

int16_t TxdNameTree::Find(uint32_t hash, uint32_t season, uint8_t flags) {
    int nearest = flags & 2 ? 1 : flags & 4 ? -1 : 0;
    uint16_t found = 0xFFFF;
    uint16_t node = uint16_t(root);
    while (node != 0xFFFF) {
        TxdDef* def = CTxdStore::ms_pTxdPool->GetAt(int16_t(node));
        int order;
        if (hash == def->nameHash && season == def->season) {
            order = 0;
        } else if (hash > def->nameHash || (hash == def->nameHash && season > def->season)) {
            order = 1;
        } else {
            order = -1;
        }
        if (order == 0) {
            if (flags & 1) {
                return int16_t(node);
            }
            order = -nearest;
        } else if (nearest != 0 && (order ^ nearest) >= 0) {
            found = node;
        }
        int16_t link = order < 0 ? nodes[int16_t(node)].less : nodes[int16_t(node)].greater;
        node = uint16_t(int16_t(link >> 1));
    }
    return int16_t(found);
}

namespace CTxdStore {

// (the registry's entry points for the tree's thiscalls)
static int16_t __fastcall TxdNameTree_Find(TxdNameTree* tree, re::Edx, uint32_t hash, uint32_t season, uint8_t flags) {
    return tree->Find(hash, season, flags);
}

// 0x5F21D0 (thiscall)
static int16_t __fastcall TxdNameTree_FindExact(TxdNameTree* tree, re::Edx, uint32_t hash, uint32_t season) {
    return game::TxdNameTree_Find(tree, hash, season, 1);
}

// 0x5F21F0 (thiscall): the version without a season, else the next key up: the
// same name's first season if it has one
static int16_t __fastcall TxdNameTree_FindAnySeason(TxdNameTree* tree, re::Edx, uint32_t hash) {
    int16_t slot = game::TxdNameTree_Find(tree, hash, 0, 5);
    if (slot != -1 && Def(slot)->nameHash == hash) {
        return slot;
    }
    return -1;
}

uint8_t CurrentSeason() {
    return uint8_t(ms_chapterSeason[ms_currentChapter] + 1);
}

void __cdecl ParseName(const char* name, uint8_t* season, uint32_t* hash) {
    char key[128];
    strcpy(key, name);  // (the game copies without a limit too)
    char& last = key[strlen(key) - 1];
    switch (last) {
    case 'X': *season = 1; break;
    case 'F': *season = 2; break;
    case 'W': *season = 3; break;
    case 'S': *season = 4; break;
    default: *season = 0; break;
    }
    if (*season != 0) {
        last = '\0';
    }
    *hash = game::HashName(key);
}

// the first slot matching, through the pool
template <typename Match>
static int FindSlot(Match match) {
    for (int slot = 0; slot < ms_pTxdPool->size; slot++) {
        TxdDef* def = Def(slot);
        if (def && match(*def)) {
            return slot;
        }
    }
    return -1;
}

int __cdecl FindTxdSlot(const char* name, int seasonal) {
    uint8_t season;
    uint32_t hash;
    ParseName(name, &season, &hash);
    uint8_t current = CurrentSeason();
    if (TxdNameTree* tree = ms_pNameTree) {
        if (re::Flag(seasonal)) {
            int slot = game::TxdNameTree_Find(tree, hash, current, 1);
            if (slot != -1) {
                return slot;
            }
            slot = game::TxdNameTree_Find(tree, hash, 0, 1);
            if (slot != -1) {
                return slot;
            }
        }
        return game::TxdNameTree_Find(tree, hash, season, 1);
    }
    if (re::Flag(seasonal)) {
        int slot = FindSlot([&](const TxdDef& d) { return d.nameHash == hash && d.season == current; });
        if (slot != -1) {
            return slot;
        }
        slot = FindSlot([&](const TxdDef& d) { return d.nameHash == hash && d.season == 0; });
        if (slot != -1) {
            return slot;
        }
    }
    return FindSlot([&](const TxdDef& d) { return d.nameHash == hash && d.season == season; });
}

int __cdecl FindTxdSlotByHash(uint32_t hash, int season) {
    uint8_t current = CurrentSeason();
    if (TxdNameTree* tree = ms_pNameTree) {
        if (season == -1) {
            return game::TxdNameTree_FindAnySeason(tree, hash);
        }
        int slot = game::TxdNameTree_Find(tree, hash, current, 1);
        if (slot != -1) {
            return slot;
        }
        slot = game::TxdNameTree_Find(tree, hash, 0, 1);
        if (slot != -1) {
            return slot;
        }
        return game::TxdNameTree_FindExact(tree, hash, uint32_t(season));
    }
    if (season == -1) {
        return FindSlot([&](const TxdDef& d) { return d.nameHash == hash; });
    }
    // (the last of each kind: the game goes through all of them)
    int withoutSeason = -1, withSeason = -1;
    for (int slot = 0; slot < ms_pTxdPool->size; slot++) {
        TxdDef* def = Def(slot);
        if (!def || def->nameHash != hash) {
            continue;
        }
        if (def->season == current) {
            return slot;
        }
        if (def->season == 0) {
            withoutSeason = slot;
        }
        if (uint32_t(season) == def->season) {
            withSeason = slot;
        }
    }
    return withoutSeason != -1 ? withoutSeason : withSeason;
}

int __cdecl AddTxdSlot(const char* name) {
    int slot = game::Txd_FindTxdSlot(name, 0);
    if (slot != -1) {
        return slot;
    }
    TxdDef* def = game::TxdPool_New(ms_pTxdPool);
    def->dict = nullptr;
    def->numRefs = 0;
    def->unknownB = 0;
    def->fromStream = 0;
    ParseName(name, &def->season, &def->nameHash);
    slot = int((reinterpret_cast<uint8_t*>(def) - re::PtrCast<uint8_t*>(ms_pTxdPool->objects)) / ms_pTxdPool->elemSize);
    if (ms_pNameTree) {
        game::TxdNameTree_Insert(ms_pNameTree, slot);
    }
    return slot;
}

void __cdecl RemoveTxdSlot(int slot) {
    if (ms_pNameTree) {
        game::TxdNameTree_Remove(ms_pNameTree, Def(slot)->nameHash, Def(slot)->season);
    }
    TxdDef* def = Def(slot);
    if (!def) {
        return;
    }
    if (def->dict) {
        game::Txd_RemoveTxd(slot);
        if (def->dict) {
            return;
        }
    }
    ms_pTxdPool->flags[slot] |= 0x80;
    if (slot < ms_pTxdPool->lastUsed) {
        ms_pTxdPool->lastUsed = slot;
    }
}

static int16_t(__thiscall* Original_TxdNameTree_Find)(TxdNameTree*, uint32_t, uint32_t, uint8_t);

static int16_t __fastcall Shadow_TxdNameTree_Find(TxdNameTree* tree, re::Edx, uint32_t hash, uint32_t season, uint8_t flags) {
    int16_t theirs = Original_TxdNameTree_Find(tree, hash, season, flags);
    int16_t ours = tree->Find(hash, season, flags);
    static uint32_t compared = 0;
    re::Compared("TxdNameTree::Find", compared);
    if (theirs != ours) {
        re::Mismatch("TxdNameTree::Find", "hash %08x season %u flags %u: game %d, ours %d", hash, season, flags, theirs, ours);
    }
    return theirs;
}

REVERSED("TxdNameTree::Find", 0x005F1C50, RE_BODY(&TxdNameTree_Find), reinterpret_cast<void*>(&Shadow_TxdNameTree_Find),
         reinterpret_cast<void**>(&Original_TxdNameTree_Find), re::Mode::Ours);
static int16_t(__thiscall* Original_TxdNameTree_FindExact)(TxdNameTree*, uint32_t, uint32_t);
REVERSED("TxdNameTree::FindExact", 0x005F21D0, RE_BODY(&TxdNameTree_FindExact), nullptr,
         reinterpret_cast<void**>(&Original_TxdNameTree_FindExact), re::Mode::Ours);
static int16_t(__thiscall* Original_TxdNameTree_FindAnySeason)(TxdNameTree*, uint32_t);
REVERSED("TxdNameTree::FindAnySeason", 0x005F21F0, RE_BODY(&TxdNameTree_FindAnySeason), nullptr,
         reinterpret_cast<void**>(&Original_TxdNameTree_FindAnySeason), re::Mode::Ours);

static void(__cdecl* Original_ParseName)(const char*, uint8_t*, uint32_t*);

static void __cdecl Shadow_ParseName(const char* name, uint8_t* season, uint32_t* hash) {
    Original_ParseName(name, season, hash);
    uint8_t ourSeason;
    uint32_t ourHash;
    ParseName(name, &ourSeason, &ourHash);
    static uint32_t compared = 0;
    re::Compared("CTxdStore::ParseName", compared);
    if (ourSeason != *season || ourHash != *hash) {
        re::Mismatch("CTxdStore::ParseName", "%s: game %u %08x, ours %u %08x", name, *season, *hash, ourSeason, ourHash);
    }
}

REVERSED("CTxdStore::ParseName", 0x005F0AC0, RE_BODY(&ParseName), reinterpret_cast<void*>(&Shadow_ParseName),
         reinterpret_cast<void**>(&Original_ParseName), re::Mode::Ours);

static int(__cdecl* Original_FindTxdSlot)(const char*, int);

static int __cdecl Shadow_FindTxdSlot(const char* name, int seasonal) {
    int theirs = Original_FindTxdSlot(name, seasonal);
    int ours = FindTxdSlot(name, seasonal);
    static uint32_t compared = 0;
    re::Compared("CTxdStore::FindTxdSlot", compared);
    if (theirs != ours) {
        re::Mismatch("CTxdStore::FindTxdSlot", "%s (%d): game %d, ours %d", name, seasonal & 0xFF, theirs, ours);
    }
    return theirs;
}

REVERSED("CTxdStore::FindTxdSlot", 0x005F22D0, RE_BODY(&FindTxdSlot), reinterpret_cast<void*>(&Shadow_FindTxdSlot),
         reinterpret_cast<void**>(&Original_FindTxdSlot), re::Mode::Ours);

static int(__cdecl* Original_FindTxdSlotByHash)(uint32_t, int);

static int __cdecl Shadow_FindTxdSlotByHash(uint32_t hash, int season) {
    int theirs = Original_FindTxdSlotByHash(hash, season);
    int ours = FindTxdSlotByHash(hash, season);
    static uint32_t compared = 0;
    re::Compared("CTxdStore::FindTxdSlotByHash", compared);
    if (theirs != ours) {
        re::Mismatch("CTxdStore::FindTxdSlotByHash", "%08x (%d): game %d, ours %d", hash, season, theirs, ours);
    }
    return theirs;
}

REVERSED("CTxdStore::FindTxdSlotByHash", 0x005F2450, RE_BODY(&FindTxdSlotByHash),
         reinterpret_cast<void*>(&Shadow_FindTxdSlotByHash), reinterpret_cast<void**>(&Original_FindTxdSlotByHash), re::Mode::Ours);

TXD_REVERSED(AddTxdSlot, 0x005F26E0, re::Mode::Ours);
TXD_REVERSED(RemoveTxdSlot, 0x005F2240, re::Mode::Ours);

} // namespace CTxdStore

// ---- start and end -------------------------------------------------------------------

namespace CTxdStore {

void __cdecl Initialise() {
    if (!ms_pTxdPool) {
        auto* pool = static_cast<CPool<TxdDef>*>(game::Malloc(sizeof(CPool<TxdDef>)));
        if (pool) {
            pool->unknown10 = 0;
            pool->objects = nullptr;
            pool->flags = nullptr;
            pool->lastUsed = 0;
            pool->ownsArrays = 1;
            pool->size = 10000;
            pool->unknown19 = 0;
            game::TxdPool_Init(pool);
        }
        ms_pTxdPool = pool;
    }
    game::PushTxdMemoryTag();
    auto* tree = static_cast<TxdNameTree*>(game::Malloc(sizeof(TxdNameTree)));
    if (tree) {
        tree->root = -1;
    }
    ms_pNameTree = tree;
    game::PopTxdMemoryTag();
}

void __cdecl Shutdown() {
    game::Free(ms_pNameTree);
    ms_pNameTree = nullptr;
    // (the game leaves ms_pTxdPool pointing at the freed pool)
    if (CPool<TxdDef>* pool = ms_pTxdPool) {
        game::Pool_Flush(pool);
        game::Free(pool);
    }
}

TXD_REVERSED(Initialise, 0x005F2600, re::Mode::Ours);
TXD_REVERSED(Shutdown, 0x005F17F0, re::Mode::Ours);

} // namespace CTxdStore

// ---- the name tree: inserting and removing ------------------------------------------
// The game's own AVL code (0x5F1910, 0x5F1D70, its rotations 0x5F0C10) keeps the same
// encoding; lookups only need a valid tree, not the same shape, so ours is plain AVL.

namespace {

TxdDef* TreeDef(int16_t slot) {
    return CTxdStore::ms_pTxdPool->GetAt(slot);
}

// (hash, season) against a node's: 1 greater, -1 less, 0 the same
int Order(uint32_t hash, uint32_t season, int16_t node) {
    TxdDef* def = TreeDef(node);
    if (hash == def->nameHash && season == def->season) {
        return 0;
    }
    return hash > def->nameHash || (hash == def->nameHash && season > def->season) ? 1 : -1;
}

} // namespace

int16_t TxdNameTree::Child(int16_t node, int side) const {
    return int16_t((side > 0 ? nodes[node].greater : nodes[node].less) >> 1);
}

void TxdNameTree::SetChild(int16_t node, int side, int16_t child) {
    int16_t& link = side > 0 ? nodes[node].greater : nodes[node].less;
    link = int16_t((child << 1) | (link & 1));
}

// height(greater) - height(less): -1 both bits, +1 the less link's bit, 0 neither
// (0x5F0BC0, 0x5F0BE0)
int TxdNameTree::Balance(int16_t node) const {
    if (nodes[node].greater & 1) {
        return -1;
    }
    return nodes[node].less & 1;
}

void TxdNameTree::SetBalance(int16_t node, int balance) {
    nodes[node].greater &= ~1;
    nodes[node].less &= ~1;
    if (balance != 0) {
        nodes[node].less |= 1;
    }
    if (balance < 0) {
        nodes[node].greater |= 1;
    }
}

// `node` is two higher on side sign(balance): the subtree's new top
int16_t TxdNameTree::Rotate(int16_t node, int balance) {
    int side = balance > 0 ? 1 : -1;
    int16_t child = Child(node, side);
    int childBalance = Balance(child);
    if (childBalance == -side) {
        int16_t grandchild = Child(child, -side);
        int grandBalance = Balance(grandchild);
        SetChild(child, -side, Child(grandchild, side));
        SetChild(node, side, Child(grandchild, -side));
        SetChild(grandchild, side, child);
        SetChild(grandchild, -side, node);
        SetBalance(node, grandBalance == side ? -side : 0);
        SetBalance(child, grandBalance == -side ? side : 0);
        SetBalance(grandchild, 0);
        return grandchild;
    }
    SetChild(node, side, Child(child, -side));
    SetChild(child, -side, node);
    if (childBalance == 0) {  // (only when removing)
        SetBalance(node, side);
        SetBalance(child, -side);
    } else {
        SetBalance(node, 0);
        SetBalance(child, 0);
    }
    return child;
}

void TxdNameTree::Replace(int16_t parent, int side, int16_t child) {
    if (parent == -1) {
        root = child;
    } else {
        SetChild(parent, side, child);
    }
}

int16_t TxdNameTree::Insert(int16_t slot) {
    nodes[slot].greater = -2;  // no children, balanced
    nodes[slot].less = -2;
    if (root == -1) {
        root = slot;
        return slot;
    }
    TxdDef* def = TreeDef(slot);
    int16_t path[64];
    int sides[64];
    int depth = 0;
    for (int16_t node = root; node != -1;) {
        int order = Order(def->nameHash, def->season, node);
        if (order == 0) {
            return node;  // (the key is there already: as the game, not added)
        }
        path[depth] = node;
        sides[depth++] = order;
        node = Child(node, order);
    }
    SetChild(path[depth - 1], sides[depth - 1], slot);
    for (int i = depth - 1; i >= 0; i--) {
        int balance = Balance(path[i]) + sides[i];
        if (balance == 0) {
            SetBalance(path[i], 0);
            break;
        }
        if (balance == 1 || balance == -1) {
            SetBalance(path[i], balance);
            continue;
        }
        Replace(i > 0 ? path[i - 1] : int16_t(-1), i > 0 ? sides[i - 1] : 0, Rotate(path[i], balance));
        break;
    }
    return slot;
}

int16_t TxdNameTree::Remove(uint32_t hash, uint32_t season) {
    int16_t path[64];
    int sides[64];
    int depth = 0;
    int16_t found = root;
    for (int order; found != -1 && (order = Order(hash, season, found)) != 0; found = Child(found, order)) {
        path[depth] = found;
        sides[depth++] = order;
    }
    if (found == -1) {
        return -1;
    }
    int16_t parent = depth > 0 ? path[depth - 1] : int16_t(-1);
    int parentSide = depth > 0 ? sides[depth - 1] : 0;
    if (Child(found, 1) != -1 && Child(found, -1) != -1) {
        // the next one up (the least of its greater side) takes its place
        int at = depth;
        path[depth] = found;
        sides[depth++] = 1;
        int16_t next = Child(found, 1);
        while (Child(next, -1) != -1) {
            path[depth] = next;
            sides[depth++] = -1;
            next = Child(next, -1);
        }
        // out of where it was (it has no less side)
        if (path[depth - 1] == found) {
            SetChild(found, 1, Child(next, 1));
        } else {
            SetChild(path[depth - 1], -1, Child(next, 1));
        }
        // into the removed one's place, with its children and balance
        SetChild(next, 1, Child(found, 1));
        SetChild(next, -1, Child(found, -1));
        SetBalance(next, Balance(found));
        Replace(parent, parentSide, next);
        path[at] = next;
    } else {
        Replace(parent, parentSide, Child(found, Child(found, 1) != -1 ? 1 : -1));
    }
    // back up: the side a node was left through got lower
    for (int i = depth - 1; i >= 0; i--) {
        int balance = Balance(path[i]) - sides[i];
        if (balance == 1 || balance == -1) {
            SetBalance(path[i], balance);
            break;
        }
        if (balance == 0) {
            SetBalance(path[i], 0);
            continue;
        }
        int childBalance = Balance(Child(path[i], balance > 0 ? 1 : -1));
        Replace(i > 0 ? path[i - 1] : int16_t(-1), i > 0 ? sides[i - 1] : 0, Rotate(path[i], balance));
        if (childBalance == 0) {
            break;  // (as high as before)
        }
    }
    return found;
}

// The subtree's height, -1 when something's off (order, balance bits, a loop)
int TxdNameTree::Check(int16_t node, const TxdDef* low, const TxdDef* high, int depth) const {
    if (node == -1) {
        return 0;
    }
    TxdDef* def = TreeDef(node);
    if (depth > 64 || !def) {
        return -1;
    }
    auto less = [](const TxdDef* a, const TxdDef* b) {
        return a->nameHash < b->nameHash || (a->nameHash == b->nameHash && a->season < b->season);
    };
    if ((low && !less(low, def)) || (high && !less(def, high))) {
        return -1;
    }
    int lessHeight = Check(Child(node, -1), low, def, depth + 1);
    int greaterHeight = Check(Child(node, 1), def, high, depth + 1);
    if (lessHeight < 0 || greaterHeight < 0 || greaterHeight - lessHeight != Balance(node)) {
        return -1;
    }
    return 1 + (lessHeight > greaterHeight ? lessHeight : greaterHeight);
}

namespace CTxdStore {

static void __fastcall TxdNameTree_Insert(TxdNameTree* tree, re::Edx, int slot) {
    tree->Insert(int16_t(slot));
}

static int16_t __fastcall TxdNameTree_Remove(TxdNameTree* tree, re::Edx, uint32_t hash, uint32_t season) {
    return tree->Remove(hash, season);
}

// "shadow": ours, then the whole tree checked (the two can't both change it)
static void CheckTree(TxdNameTree* tree, const char* name, uint32_t& compared) {
    re::Compared(name, compared);
    if (tree->Check(tree->root, nullptr, nullptr, 0) < 0) {
        re::Mismatch(name, "the tree is broken (root %d)", tree->root);
    }
}

static void __fastcall Shadow_TxdNameTree_Insert(TxdNameTree* tree, re::Edx, int slot) {
    if (slot < 0 || slot >= 10000 || slot >= ms_pTxdPool->size) {
        re::Mismatch("TxdNameTree::Insert", "slot %d out of range", slot);
    }
    tree->Insert(int16_t(slot));
    static uint32_t compared = 0;
    CheckTree(tree, "TxdNameTree::Insert", compared);
    TxdDef* def = ms_pTxdPool->GetAt(int16_t(slot));
    if (tree->Find(def->nameHash, def->season, 1) == -1) {
        re::Mismatch("TxdNameTree::Insert", "slot %d not found after", slot);
    }
}

static int16_t __fastcall Shadow_TxdNameTree_Remove(TxdNameTree* tree, re::Edx, uint32_t hash, uint32_t season) {
    int16_t removed = tree->Remove(hash, season);
    static uint32_t compared = 0;
    CheckTree(tree, "TxdNameTree::Remove", compared);
    if (tree->Find(hash, season, 1) != -1) {
        re::Mismatch("TxdNameTree::Remove", "%08x %u still found after", hash, season);
    }
    return removed;
}

static void(__thiscall* Original_TxdNameTree_Insert)(TxdNameTree*, int);
static int16_t(__thiscall* Original_TxdNameTree_Remove)(TxdNameTree*, uint32_t, uint32_t);

REVERSED("TxdNameTree::Insert", 0x005F1910, RE_BODY(&TxdNameTree_Insert), reinterpret_cast<void*>(&Shadow_TxdNameTree_Insert),
         reinterpret_cast<void**>(&Original_TxdNameTree_Insert), re::Mode::Ours);
REVERSED("TxdNameTree::Remove", 0x005F1D70, RE_BODY(&TxdNameTree_Remove), reinterpret_cast<void*>(&Shadow_TxdNameTree_Remove),
         reinterpret_cast<void**>(&Original_TxdNameTree_Remove), re::Mode::Ours);

} // namespace CTxdStore
