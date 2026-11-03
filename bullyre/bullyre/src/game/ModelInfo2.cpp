// CModelInfo: the model table (ms_modelInfoPtrs), its name index and hashed names,
// the stores the model infos come from, and the lookups by name.

#include "game/ModelInfo.h"

#include <cstring>

#include "game/Crt.h"
#include "reversed.h"
namespace {
GAME_FN(uint32_t, __cdecl, CMI_HashName, 0x00576ED0, const char* name);
GAME_FN(void*, __cdecl, CMI_Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, CMI_Free, 0x005EEAB0, void* p);
GAME_FN(uint8_t, __cdecl, CMI_NiRelease, 0x005F0F90, NiRefObject* object);
inline int __cdecl CMI_Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
inline char* __cdecl CMI_Strlwr_8D89A2(char* s) { return crt::Strlwr(s); }   // _strlwr (lower case, in place)
inline void* __cdecl CMI_Bsearch(const void* key, const void* base, size_t count, size_t size, int(__cdecl* compare)(const void*, const void*)) { return crt::Bsearch(key, base, count, size, (int(__cdecl*)(const void*, const void*))compare); }
inline int(__cdecl* const CMI_CompareHash)(const void*, const void*) =
    reinterpret_cast<int(__cdecl*)(const void*, const void*)>(0x005A7D40);

// 0x5A7D40: bsearch's comparison of two hashes (unsigned: -1, 0, 1)
int __cdecl CompareHashImpl(const void* a, const void* b) {
    uint32_t x = *static_cast<const uint32_t*>(a);
    uint32_t y = *static_cast<const uint32_t*>(b);
    return x < y ? -1 : (x > y ? 1 : 0);
}
// the game's file reading (CFileMgr)
GAME_FN(int, __cdecl, CMI_OpenFile, 0x0042D260, const char* name, const char* mode, int);
GAME_FN(int, __cdecl, CMI_Read, 0x0042D2A0, int file, void* buffer, int size);
GAME_FN(void, __cdecl, CMI_CloseFile, 0x0042D450, int file);
GAME_FN(void, __cdecl, CMI_PushTreeMemoryTag, 0x0052E700);
GAME_FN(void, __cdecl, CMI_PopTreeMemoryTag, 0x0052E710);
// "eh vector constructor iterator" (new T[n])
GAME_FN(void, __stdcall, CMI_VectorConstruct, 0x00401240, void* objects, size_t size, int count, void* ctor);

// (their own and each other, where modes apply)
GAME_FN(int16_t, __thiscall, Tree_Find, 0x0051BCE0, ModelNameTree* tree, uint32_t hash, uint8_t flags);
GAME_FN(int16_t, __thiscall, Tree_Insert, 0x0051BA60, ModelNameTree* tree, int id);
GAME_FN(int16_t, __thiscall, Tree_Remove, 0x0051BD90, ModelNameTree* tree, uint32_t hash);
GAME_FN(int, __thiscall, HashTable_Find, 0x0051A8F0, ModelHashTable* table, uint32_t hash);
GAME_FN(void, __thiscall, HashTable_Load, 0x0051A870, ModelHashTable* table);
GAME_FN(int, __cdecl, CMI_GetModelIndex, 0x0051C330, const char* name);
GAME_FN(void, __cdecl, CMI_ClearModelInfo, 0x0051C650, int id);
GAME_FN(void, __cdecl, CMI_LoadCat, 0x0051B880, int cat);
GAME_FN(void, __thiscall, CMI_2dEffectDestroy, 0x0051B2A0, C2dEffect* effect, uint8_t flags);
GAME_FN(void, __thiscall, SimplePool_Init, 0x0051AFD0, void* pool);
GAME_FN(void, __thiscall, TimePool_Init, 0x0051B080, void* pool);
GAME_FN(void, __thiscall, PAnimPool_Init, 0x0051B130, void* pool);
GAME_FN(void, __thiscall, EffectPool_Init, 0x0051B1F0, void* pool);
GAME_FN(void, __thiscall, EffectPool_Flush, 0x0051B470, void* pool);
GAME_FN(void, __thiscall, Pool_Flush, 0x004498F0, void* pool);

// the model infos' own (where modes apply)
GAME_FN(void, __thiscall, CMI_BaseConstruct, 0x0050E8C0, void* mi, uint8_t type);
GAME_FN(void, __thiscall, CMI_BaseSetName, 0x0050ED40, void* mi, const char* name);
GAME_FN(void, __thiscall, CMI_ClumpConstruct, 0x005109E0, void* mi, uint8_t type);
GAME_FN(void, __thiscall, CMI_ClumpInit, 0x0050E7A0, void* mi);
GAME_FN(void, __thiscall, CMI_SimpleInit, 0x00527410, void* mi);
GAME_FN(void, __thiscall, CMI_PAnimUpdateAnimIndex, 0x00520A20, CPAnimModelInfo* mi);
GAME_FN(uint8_t, __thiscall, CMI_PAnimIsKind1, 0x00520440, CPAnimModelInfo* mi);
GAME_FN(uint8_t, __thiscall, CMI_PAnimIsKind3, 0x00520480, CPAnimModelInfo* mi);
GAME_FN(void, __thiscall, CMI_PAnimResetUnknown78, 0x00520030, CPAnimModelInfo* mi);
GAME_FN(int16_t, __cdecl, CMI_FindPAnimIndex, 0x00520960, const char* name, CBaseModelInfo* mi);
GAME_FN(void, __thiscall, CMI_Ped_522180, 0x00522180, CPedModelInfo* mi);
GAME_FN(void, __thiscall, CMI_Ped_522CB0, 0x00522CB0, CPedModelInfo* mi, int arg);
// operator new / the simple and time stores' (pool) allocations
GAME_FN(void*, __cdecl, CMI_NewSimple, 0x00527600, size_t size);
GAME_FN(void*, __cdecl, CMI_NewTime, 0x00527610, size_t size);
GAME_FN(void*, __cdecl, CMI_NewPAnim, 0x00520AD0, size_t size);
// a stand-in's object (collision side, thiscall): its +0x3C
GAME_FN(void, __thiscall, StandIn_SetObject, 0x0056E770, CBaseModelInfo* standIn, uint32_t tagged);

GAME_FN(void, __cdecl, CMI_RemoveModel, 0x0052F640, int id);
GAME_FN(void, __cdecl, CMI_RequestModel, 0x00532020, int id, uint32_t flags);
GAME_FN(void, __cdecl, CMI_LoadAllRequestedModels, 0x00533EE0, int onlyPriority);
GAME_FN(void, __cdecl, CMI_LoadingScreen, 0x0043C4F0, const char* title, const char* message, uintptr_t);

// the prop animations' act trees (CCatStore, 0xC0F7B4)
GAME_FN(void*, __cdecl, ActTree_FindNode, 0x005F5B10, void* name, uintptr_t);
GAME_FN(void, __cdecl, CatStore_SetUnknown, 0x0052D800, int cat, void* node);
GAME_FN(void*, __cdecl, CatStore_GetUnknown, 0x0052D810, int cat);
GAME_FN(void, __cdecl, ActTree_Release_5F5A60, 0x005F5A60, void* data, uintptr_t, uintptr_t);
GAME_FN(void, __thiscall, CatStore_AddRef, 0x004506F0, void* catStore, int cat);
inline void* const catStore = reinterpret_cast<void*>(0x00C0F7B4);

GAME_VAR(int32_t, firstVehicleModel, 0x00A136A8);
GAME_VAR(int32_t, lastVehicleModel, 0x00A136AC);
GAME_VAR(int32_t, firstWeaponIndex, 0x00A136B0);
GAME_VAR(int32_t, firstPedModel, 0x00A136C8);
GAME_VAR(int32_t, lastPedModel, 0x00A136CC);
GAME_VAR(void*, defaultButes, 0x020C88D8);
GAME_VAR(const float, vehicleUnknownE0Default, 0x00917BE4);   // 999.99
GAME_VAR(const float, weaponUnknown50Default, 0x00900E68);    // 30

// the default act files' names and nodes
GAME_VAR(const char*, defaultActFile1, 0x00A76B74);
GAME_VAR(const char*, defaultActFile2, 0x00A76B6C);
GAME_VAR(void*, actNodeName_A76B78, 0x00A76B78);
GAME_VAR(void*, actNodeName_A76B70, 0x00A76B70);
GAME_VAR(void*, actNodeName_A76B58, 0x00A76B58);
GAME_VAR(void*, actNodeName_A76B64, 0x00A76B64);
GAME_VAR(void*, actNodeName_A76B7C, 0x00A76B7C);
GAME_VAR(void*, actNodeName_A76B80, 0x00A76B80);
GAME_VAR(void*, actNode_C8E8FC, 0x00C8E8FC);
GAME_VAR(void*, actNode_C8E900, 0x00C8E900);
GAME_VAR(void*, actNode_C8E904, 0x00C8E904);
GAME_VAR(void*, actNode_C8E908, 0x00C8E908);
GAME_VAR(void*, actNode_C8E90C, 0x00C8E90C);
GAME_VAR(void*, actNode_C8E910, 0x00C8E910);

constexpr int NUM_MODEL_INFOS = 0x2FA8;
constexpr int NUM_WEAPON_MODELS = 150;
constexpr int NUM_ACCESSORY_MODELS = 21;
constexpr int NUM_VEHICLE_MODELS = 32;
constexpr int NUM_PED_MODELS = 0x123;
constexpr int NUM_CLOTH_MODELS = 1;
constexpr int CAT_ID_BASE_ = 0x5D41;   // (CStreaming's CAT_ID_BASE)

using namespace CModelInfo;

// A new model info into the table (and the name index)
void SetModelInfo(int id, CBaseModelInfo* mi) {
    ModelNameTree* tree = ms_pNameTree;
    ms_modelInfoPtrs[id] = mi;
    if (tree) {
        Tree_Insert(tree, id);
    }
}

// The table's entries can be a stand-in, whose +0x3C holds a model's object tagged with
// 0x40000000, or such a tagged value itself (32-bit values, as the game keeps them)
uintptr_t Entry(int id) {
    return re::PtrCast<uintptr_t>(ms_modelInfoPtrs[id]);
}
uint32_t TaggedObject(uintptr_t entry) {
    if (reinterpret_cast<CBaseModelInfo*>(entry)->unknown1C) {
        entry = *reinterpret_cast<uint32_t*>(entry + 0x3C);
    }
    return entry & 0x40000000 ? uint32_t(entry) & 0xBFFFFFFF : 0;
}

// each used object of a pool
template <typename T, typename Fn>
void ForEachInPool(CPool<T>& pool, Fn fn) {
    for (int i = 0; i < pool.size; i++) {
        if (!(pool.flags[i] & 0x80)) {
            fn(re::PtrCast<T*>(re::PtrCast<uint8_t*>(pool.objects) + pool.elemSize * i), i);
        }
    }
}

} // namespace

// ---- the name index ------------------------------------------------------------------
// As CTxdStore's tree: the lookup is the game's; inserting and removing keep a valid AVL
// tree in the same encoding (not necessarily the same shape as the game's code would).

namespace {
uint32_t NodeHash(int16_t node) {
    return ms_modelInfoPtrs[node]->nameHash;
}
// the hash against a node's: 1 greater, -1 less, 0 the same (unsigned)
int Order(uint32_t hash, int16_t node) {
    uint32_t nodeHash = NodeHash(node);
    return hash == nodeHash ? 0 : hash > nodeHash ? 1 : -1;
}
} // namespace

int16_t ModelNameTree::Find(uint32_t hash, uint8_t findFlags) {
    int nearest = findFlags & 2 ? 1 : findFlags & 4 ? -1 : 0;
    uint16_t found = 0xFFFF;
    uint16_t node = uint16_t(root);
    while (node != 0xFFFF) {
        int order = Order(hash, int16_t(node));
        if (order == 0) {
            if (findFlags & 1) {
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

int16_t ModelNameTree::Child(int16_t node, int side) const {
    return int16_t((side > 0 ? nodes[node].greater : nodes[node].less) >> 1);
}

void ModelNameTree::SetChild(int16_t node, int side, int16_t child) {
    int16_t& link = side > 0 ? nodes[node].greater : nodes[node].less;
    link = int16_t((child << 1) | (link & 1));
}

// height(greater) - height(less): -1 both bits, +1 the less link's bit, 0 neither
int ModelNameTree::Balance(int16_t node) const {
    if (nodes[node].greater & 1) {
        return -1;
    }
    return nodes[node].less & 1;
}

void ModelNameTree::SetBalance(int16_t node, int balance) {
    nodes[node].greater &= ~1;
    nodes[node].less &= ~1;
    if (balance != 0) {
        nodes[node].less |= 1;
    }
    if (balance < 0) {
        nodes[node].greater |= 1;
    }
}

int16_t ModelNameTree::Rotate(int16_t node, int balance) {
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

void ModelNameTree::Replace(int16_t parent, int side, int16_t child) {
    if (parent == -1) {
        root = child;
    } else {
        SetChild(parent, side, child);
    }
}

int16_t ModelNameTree::Insert(int16_t id) {
    nodes[id].greater = -2;  // no children, balanced
    nodes[id].less = -2;
    if (root == -1) {
        root = id;
        return id;
    }
    uint32_t hash = NodeHash(id);
    int16_t path[64];
    int sides[64];
    int depth = 0;
    for (int16_t node = root; node != -1;) {
        int order = Order(hash, node);
        if (order == 0) {
            return node;  // (the name's there already: as the game, not added)
        }
        path[depth] = node;
        sides[depth++] = order;
        node = Child(node, order);
    }
    SetChild(path[depth - 1], sides[depth - 1], id);
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
    return id;
}

int16_t ModelNameTree::Remove(uint32_t hash) {
    int16_t path[64];
    int sides[64];
    int depth = 0;
    int16_t found = root;
    for (int order; found != -1 && (order = Order(hash, found)) != 0; found = Child(found, order)) {
        path[depth] = found;
        sides[depth++] = order;
    }
    if (found == -1) {
        return -1;
    }
    int16_t parent = depth > 0 ? path[depth - 1] : int16_t(-1);
    int parentSide = depth > 0 ? sides[depth - 1] : 0;
    if (Child(found, 1) != -1 && Child(found, -1) != -1) {
        // the next one up takes its place
        int at = depth;
        path[depth] = found;
        sides[depth++] = 1;
        int16_t next = Child(found, 1);
        while (Child(next, -1) != -1) {
            path[depth] = next;
            sides[depth++] = -1;
            next = Child(next, -1);
        }
        if (path[depth - 1] == found) {
            SetChild(found, 1, Child(next, 1));
        } else {
            SetChild(path[depth - 1], -1, Child(next, 1));
        }
        SetChild(next, 1, Child(found, 1));
        SetChild(next, -1, Child(found, -1));
        SetBalance(next, Balance(found));
        Replace(parent, parentSide, next);
        path[at] = next;
    } else {
        Replace(parent, parentSide, Child(found, Child(found, 1) != -1 ? 1 : -1));
    }
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
            break;
        }
    }
    return found;
}

int ModelNameTree::Check(int16_t node, int64_t low, int64_t high, int depth) const {
    if (node == -1) {
        return 0;
    }
    if (depth > 64 || node < 0 || node >= NUM_MODEL_INFOS || !ms_modelInfoPtrs[node]) {
        return -1;
    }
    int64_t hash = NodeHash(node);
    if (hash <= low || hash >= high) {
        return -1;
    }
    int lessHeight = Check(Child(node, -1), low, hash, depth + 1);
    int greaterHeight = Check(Child(node, 1), hash, high, depth + 1);
    if (lessHeight < 0 || greaterHeight < 0 || greaterHeight - lessHeight != Balance(node)) {
        return -1;
    }
    return 1 + (lessHeight > greaterHeight ? lessHeight : greaterHeight);
}

// ---- the hashed names ----------------------------------------------------------------

void ModelHashTable::Load() {
    if (!ms_hashedNames) {
        return;
    }
    char name[] = "Config\\Extra\\ModelInd.bin";
    int file = CMI_OpenFile(name, "r", 1);
    CMI_Read(file, &count, 4);
    // (count * 8, 0xFFFFFFFF when it overflows)
    uint64_t bytes = uint64_t(uint32_t(count)) * 8;
    entries = static_cast<Entry*>(CMI_Malloc(bytes > 0xFFFFFFFF ? 0xFFFFFFFF : size_t(bytes)));
    CMI_Read(file, entries, count * 8);
    CMI_CloseFile(file);
}

int ModelHashTable::Find(uint32_t hash) {
    if (!count) {
        return -1;
    }
    auto* found = static_cast<Entry*>(CMI_Bsearch(&hash, entries, count, sizeof(Entry), CMI_CompareHash));
    return found ? found->id : -1;
}

// ---- lookups -------------------------------------------------------------------------

namespace CModelInfo {

// 0x51C190. The linear search (no index) only writes the id when found; the index's
// always does
CBaseModelInfo* __cdecl GetModelInfo(const char* name, int* id) {
    uint32_t hash = CMI_HashName(name);
    if (ModelNameTree* tree = ms_pNameTree) {
        int found = Tree_Find(tree, hash, 1);
        *id = found;
        return found == -1 ? nullptr : ms_modelInfoPtrs[found];
    }
    for (int i = 0; i < NUM_MODEL_INFOS; i++) {
        CBaseModelInfo* mi = ms_modelInfoPtrs[i];
        if (mi && mi->nameHash == hash) {
            if (id) {
                *id = i;
            }
            return mi;
        }
    }
    return nullptr;
}

// 0x51C200 (the range is only for the linear search)
CBaseModelInfo* __cdecl GetModelInfoInRange(const char* name, int first, int last) {
    uint32_t hash = CMI_HashName(name);
    if (ModelNameTree* tree = ms_pNameTree) {
        int found = Tree_Find(tree, hash, 1);
        return found == -1 ? nullptr : ms_modelInfoPtrs[found];
    }
    for (int i = first; i <= last; i++) {
        CBaseModelInfo* mi = ms_modelInfoPtrs[i];
        if (mi && mi->nameHash == hash) {
            return mi;
        }
    }
    return nullptr;
}

// 0x51C260
CBaseModelInfo* __cdecl GetModelInfoInRange(const char* name, int* id, int first, int last) {
    uint32_t hash = CMI_HashName(name);
    if (ModelNameTree* tree = ms_pNameTree) {
        int found = Tree_Find(tree, hash, 1);
        *id = found;
        return found == -1 ? nullptr : ms_modelInfoPtrs[found];
    }
    for (int i = first; i <= last; i++) {
        CBaseModelInfo* mi = ms_modelInfoPtrs[i];
        if (mi && mi->nameHash == hash) {
            *id = i;
            return mi;
        }
    }
    *id = -1;
    return nullptr;
}

// 0x51C2E0
int __cdecl GetModelIndexInRange(const char* name, int first, int last) {
    uint32_t hash = CMI_HashName(name);
    if (ModelNameTree* tree = ms_pNameTree) {
        return Tree_Find(tree, hash, 1);
    }
    for (int i = first; i <= last; i++) {
        CBaseModelInfo* mi = ms_modelInfoPtrs[i];
        if (mi && mi->nameHash == hash) {
            return i;
        }
    }
    return -1;
}

// 0x51C330 (the linear search skips stand-ins; the other lookups don't)
int __cdecl GetModelIndex(const char* name) {
    uint32_t hash = CMI_HashName(name);
    if (ModelNameTree* tree = ms_pNameTree) {
        return Tree_Find(tree, hash, 1);
    }
    for (int i = 0; i < NUM_MODEL_INFOS; i++) {
        CBaseModelInfo* mi = ms_modelInfoPtrs[i];
        if (mi && !mi->unknown1C && mi->nameHash == hash) {
            return i;
        }
    }
    return -1;
}

// 0x51C390
int __cdecl GetModelIndexHashed(const char* name) {
    if (!ms_hashedNames) {
        return CMI_GetModelIndex(name);
    }
    if (!name || !*name) {
        return -1;
    }
    return HashTable_Find(&ms_hashTable, CMI_HashName(name));
}

// 0x51AE20: by hash, or peds' second name (no stand-ins)
int __cdecl GetModelIndexByHash(uint32_t hash, int first, int last) {
    for (int i = first; i <= last; i++) {
        CBaseModelInfo* mi = ms_modelInfoPtrs[i];
        if (mi && !mi->unknown1C) {
            if (mi->nameHash == hash) {
                return i;
            }
            if (mi->type == MODEL_INFO_PED && reinterpret_cast<CPedModelInfo*>(mi)->altNameHash == hash) {
                return i;
            }
        }
    }
    return -1;
}

// 0x51A960: the entry if it's a stand-in
CBaseModelInfo* __cdecl GetStandIn(int id) {
    CBaseModelInfo* mi = ms_modelInfoPtrs[id];
    return mi->unknown1C ? mi : nullptr;
}

// 0x51AFA0: the object tagged in an entry (or its stand-in), 0: none
uint32_t __cdecl GetStandInObject(int id) {
    return TaggedObject(Entry(id));
}

// 0x51B450: its butes, else the default ones
void* __cdecl GetButes(int id) {
    if (id != -1) {
        if (void* butes = ms_modelInfoPtrs[id]->unknown18) {
            return butes;
        }
    }
    return defaultButes;
}

// 0x51B3B0 / 0x51B420 / 0x51B3F0 (by the id ranges and the type)
uint8_t __cdecl IsVehicleClass1(int id) {
    if (id < firstVehicleModel || id > lastVehicleModel) {
        return 0;
    }
    CBaseModelInfo* mi = ms_modelInfoPtrs[id];
    return mi && mi->type == MODEL_INFO_VEHICLE && reinterpret_cast<CVehicleModelInfo*>(mi)->vehicleClass == 1;
}

uint8_t __cdecl IsVehicleModel(int id) {
    if (id < firstVehicleModel || id > lastVehicleModel) {
        return 0;
    }
    CBaseModelInfo* mi = ms_modelInfoPtrs[id];
    return mi && mi->type == MODEL_INFO_VEHICLE;
}

uint8_t __cdecl IsPedModel(int id) {
    if (id < firstPedModel || id > lastPedModel) {
        return 0;
    }
    CBaseModelInfo* mi = ms_modelInfoPtrs[id];
    return mi && mi->type == MODEL_INFO_PED;
}

// 0x51AEA0 / 0x51AF00: a weapon's index (after the first weapon item) by its model's
// name, -1: none or "unarmed". (The name is lower-cased in place first)
static int WeaponIndex(uint32_t hash) {
    int i = 0;
    for (CWeaponModelInfo* mi = ms_weaponModelStore; mi < ms_weaponModelStore + NUM_WEAPON_MODELS; mi++, i++) {
        if (i < ms_numWeaponModels && hash == mi->nameHash) {
            return firstWeaponIndex + i;
        }
    }
    return -1;
}

int __cdecl GetWeaponIndex(char* name) {
    if (!name || !CMI_Stricmp(name, "unarmed")) {
        return -1;
    }
    return WeaponIndex(CMI_HashName(CMI_Strlwr_8D89A2(name)));
}

int __cdecl GetWeaponIndexByHash(uint32_t hash) {
    if (!hash || hash == CMI_HashName("unarmed")) {
        return -1;
    }
    return WeaponIndex(hash);
}

// ---- the table's entries -------------------------------------------------------------

// 0x51C600: the model's objects and its info gone
void __cdecl DeleteModelInfo(int id) {
    if (ModelNameTree* tree = ms_pNameTree) {
        Tree_Remove(tree, ms_modelInfoPtrs[int16_t(id)]->nameHash);
    }
    CMI_RemoveModel(id);
    if (CBaseModelInfo* mi = ms_modelInfoPtrs[id]) {
        mi->CallDestroy(1);
    }
    ms_modelInfoPtrs[id] = nullptr;
}

// 0x51C650: out of the table (a stand-in only lets its object go)
void __cdecl ClearModelInfo(int id) {
    CBaseModelInfo* mi = ms_modelInfoPtrs[id];
    if (mi->unknown1C) {
        StandIn_SetObject(mi, 0);
        return;
    }
    if (ModelNameTree* tree = ms_pNameTree) {
        Tree_Remove(tree, ms_modelInfoPtrs[int16_t(id)]->nameHash);
    }
    ms_modelInfoPtrs[id] = nullptr;
}

// 0x51CAC0: frees the model's objects
void __cdecl RemoveModelObjects(int id) {
    if (CBaseModelInfo* mi = ms_modelInfoPtrs[id]) {
        mi->DeleteObjects();
    }
    // (else the game goes on to read the stand-in through the null entry: never taken)
}

// 0x51CB30: a stand-in takes the id; an object the entry had moves to it
void __cdecl SetStandIn(int id, CBaseModelInfo* standIn) {
    uint32_t object = TaggedObject(Entry(id));
    if (object) {
        CMI_ClearModelInfo(id);
    }
    ModelNameTree* tree = ms_pNameTree;
    standIn->unknown1C = 1;
    ms_modelInfoPtrs[id] = standIn;
    if (tree) {
        Tree_Insert(tree, id);
    }
    ms_numStandIns++;
    if (object) {
        CBaseModelInfo* entry = ms_modelInfoPtrs[id];
        if (entry->unknown1C) {
            StandIn_SetObject(entry, object | 0x40000000);
            return;
        }
        ms_modelInfoPtrs[id] = reinterpret_cast<CBaseModelInfo*>(uintptr_t(object | 0x40000000));
        if (tree) {
            Tree_Insert(tree, id);
        }
    }
}

// 0x51CBD0: the stand-in out; its object (if any) stays as the entry
void __cdecl RemoveStandIn(int id) {
    ModelNameTree* tree = ms_pNameTree;
    if (tree) {
        Tree_Remove(tree, ms_modelInfoPtrs[int16_t(id)]->nameHash);
    }
    uint32_t object = TaggedObject(Entry(id));
    ms_numStandIns--;
    ms_modelInfoPtrs[id] = nullptr;
    if (object) {
        ms_modelInfoPtrs[id] = reinterpret_cast<CBaseModelInfo*>(uintptr_t(object | 0x40000000));
        if (tree) {
            Tree_Insert(tree, id);
        }
    }
}

// 0x51B360: made again under a (new) name. `baseName`: the name only (no per-kind setup)
CBaseModelInfo* __cdecl ReInitModelInfo(int id, const char* name, int baseName) {
    CBaseModelInfo* mi = ms_modelInfoPtrs[id];
    mi->CallShutdown();
    reinterpret_cast<CClumpModelInfo*>(mi)->clump = nullptr;
    if (re::Flag(baseName)) {
        CMI_BaseSetName(mi, name);
    } else {
        mi->CallSetName(name);
    }
    return mi;
}

// ---- adding models -------------------------------------------------------------------
// From the IDE files. The stores' arrays hand out their next object (nullptr when full,
// which the game then uses anyway)

template <typename T>
static T* NextInStore(int32_t& count, T* store, int size) {
    if (count >= size) {
        return nullptr;
    }
    return &store[count++];
}

// 0x51C740
CBaseModelInfo* __cdecl AddSimpleModel(int id, const char* name) {
    auto* mi = static_cast<CSimpleModelInfo*>(CMI_NewSimple(sizeof(CSimpleModelInfo)));
    if (mi) {
        CMI_BaseConstruct(mi, MODEL_INFO_SIMPLE);
        mi->vtable = ms_simpleVtable;
    } else {
        mi = nullptr;
    }
    CMI_SimpleInit(mi);
    mi->Base()->CallSetName(name);
    SetModelInfo(id, mi->Base());
    return mi->Base();
}

// 0x51C7A0
CBaseModelInfo* __cdecl AddTimeModel(int id, const char* name) {
    auto* mi = static_cast<CTimeModelInfo*>(CMI_NewTime(sizeof(CTimeModelInfo)));
    if (mi) {
        CMI_BaseConstruct(mi, MODEL_INFO_TIME);
        mi->vtable = ms_timeVtable;
        mi->otherModel = -1;
    } else {
        mi = nullptr;
    }
    CMI_SimpleInit(mi);
    mi->Base()->CallSetName(name);
    SetModelInfo(id, mi->Base());
    return mi->Base();
}

// 0x51C800
CBaseModelInfo* __cdecl AddWeaponModel(int id, const char* name) {
    CWeaponModelInfo* mi = NextInStore(ms_numWeaponModels, ms_weaponModelStore, NUM_WEAPON_MODELS);
    CMI_ClumpInit(mi);
    mi->Base()->CallSetName(name);
    SetModelInfo(id, mi->Base());
    return mi->Base();
}

// 0x51C860
CBaseModelInfo* __cdecl AddAccessoryModel(int id, const char* name) {
    CAccessoryModelInfo* mi = NextInStore(ms_numAccessoryModels, ms_accessoryModelStore, NUM_ACCESSORY_MODELS);
    CMI_ClumpInit(mi);
    mi->Base()->CallSetName(name);
    SetModelInfo(id, mi->Base());
    return mi->Base();
}

// 0x51C8C0 (`baseName`: as ReInitModelInfo)
CBaseModelInfo* __cdecl AddVehicleModel(int id, const char* name, int baseName) {
    CVehicleModelInfo* mi = NextInStore(ms_numVehicleModels, ms_vehicleModelStore, NUM_VEHICLE_MODELS);
    mi->unknownE0 = vehicleUnknownE0Default;
    mi->vehicleClass = -1;
    mi->unknown5C = -1;
    mi->clump = nullptr;
    mi->unknownE4 = 0;
    mi->unknown150 = 0;
    if (re::Flag(baseName)) {
        CMI_BaseSetName(mi, name);
    } else {
        mi->Base()->CallSetName(name);
    }
    SetModelInfo(id, mi->Base());
    return mi->Base();
}

// 0x51C960
CBaseModelInfo* __cdecl AddPedModel(int id, const char* name, int baseName) {
    CPedModelInfo* mi = NextInStore(ms_numPedModels, ms_pedModelStore, NUM_PED_MODELS);
    mi->clump = nullptr;
    if (re::Flag(baseName)) {
        CMI_BaseSetName(mi, name);
    } else {
        mi->Base()->CallSetName(name);
    }
    SetModelInfo(id, mi->Base());
    return mi->Base();
}

// 0x51C9D0
CBaseModelInfo* __cdecl AddPAnimModel(int id, const char* name) {
    auto* mi = static_cast<CPAnimModelInfo*>(CMI_NewPAnim(sizeof(CPAnimModelInfo)));
    if (mi) {
        CMI_ClumpConstruct(mi, MODEL_INFO_PANIM);
        mi->vtable = ms_panimVtable;
        mi->actData = nullptr;
        mi->actData74 = nullptr;
        mi->unknown78 = nullptr;
        mi->unknown80 = 0;
        mi->panimIndex = -1;
        mi->startNode = nullptr;
        mi->unknown8C = -1;
    } else {
        mi = nullptr;
    }
    mi->clump = nullptr;
    mi->Base()->CallSetName(name);
    SetModelInfo(id, mi->Base());
    return mi->Base();
}

// 0x51CA60
CBaseModelInfo* __cdecl AddClothModel(int id, const char* name) {
    CClumpModelInfo* mi = NextInStore(ms_numClothModels, ms_clothModelStore, NUM_CLOTH_MODELS);
    mi->clump = nullptr;
    mi->Base()->CallSetName(name);
    SetModelInfo(id, mi->Base());
    return mi->Base();
}

// ---- the prop animations -------------------------------------------------------------

// 0x51BA20
void __cdecl UpdatePAnimIndices() {
    ForEachInPool(ms_panimModelStore, [](CPAnimModelInfo* mi, int) { CMI_PAnimUpdateAnimIndex(mi); });
}

// 0x51AE60: each ped model but the first
void __cdecl PedModels_522CB0(int arg) {
    for (int i = 1; i < ms_numPedModels; i++) {
        CMI_Ped_522CB0(&ms_pedModelStore[i], arg);
    }
}

// 0x51B800. (The prop animations: the first as many slots as are in use, used or not;
// a free one's nullptr is passed on)
void __cdecl Fn_51B800() {
    for (int n = ms_numPedModels, i = 0; n > 0; n--, i++) {
        CMI_Ped_522180(&ms_pedModelStore[i]);
    }
    CPool<CPAnimModelInfo>& pool = ms_panimModelStore;
    int used = 0;
    for (int i = 0; i < pool.size; i++) {
        if (!(pool.flags[i] & 0x80)) {
            used++;
        }
    }
    for (int i = 0; i < used; i++) {
        CMI_PAnimResetUnknown78(pool.GetAt(i));
    }
}

// 0x51C3D0: the two default act files, loaded now, and the act nodes everyone uses
void __cdecl LoadDefaultPropActFiles() {
    int cat1 = CMI_FindPAnimIndex(defaultActFile1, nullptr);
    int cat2 = CMI_FindPAnimIndex(defaultActFile2, nullptr);
    CMI_RequestModel(cat1 + CAT_ID_BASE_, 1);
    CMI_RequestModel(cat2 + CAT_ID_BASE_, 1);
    // ("CModelInfo::LoadDefaultPropActFiles()", "CStreaming::LoadAllRequestedModels()")
    CMI_LoadingScreen(reinterpret_cast<const char*>(0x00902C60), reinterpret_cast<const char*>(0x0090250C), 0);
    CMI_LoadAllRequestedModels(0);
    CatStore_AddRef(catStore, cat1);
    CatStore_AddRef(catStore, cat2);
    actNode_C8E900 = ActTree_FindNode(actNodeName_A76B78, 0);
    actNode_C8E8FC = ActTree_FindNode(actNodeName_A76B70, 0);
    actNode_C8E904 = ActTree_FindNode(actNodeName_A76B58, 0);
    actNode_C8E908 = ActTree_FindNode(actNodeName_A76B64, 0);
    actNode_C8E90C = ActTree_FindNode(actNodeName_A76B7C, 0);
    actNode_C8E910 = ActTree_FindNode(actNodeName_A76B80, 0);
    ForEachInPool(ms_panimModelStore, [](CPAnimModelInfo* mi, int) {
        if (CMI_PAnimIsKind1(mi)) {
            mi->startNode = actNode_C8E8FC;
        } else if (CMI_PAnimIsKind3(mi)) {
            mi->startNode = actNode_C8E900;
        }
    });
    CMI_LoadCat(cat1);
    CMI_LoadCat(cat2);
}

} // namespace CModelInfo

namespace ModelInfoCats {  // (CCatStore's, over the prop animation models)

// 0x51B880: a .cat loaded: its props' start nodes
void __cdecl Load(int cat) {
    ActTree_FindNode(actNodeName_A76B78, 0);   // (results unused)
    ActTree_FindNode(actNodeName_A76B70, 0);
    ForEachInPool(ms_panimModelStore, [cat](CPAnimModelInfo* mi, int) {
        if (mi->Base()->GetPAnimIndex() != cat) {
            return;
        }
        void* node = nullptr;
        auto* act = static_cast<uint8_t*>(mi->actData);
        if (void* name = *reinterpret_cast<re::Ptr32<void>*>(act + 0x70)) {
            node = ActTree_FindNode(name, 0);
        }
        if (void* name = *reinterpret_cast<re::Ptr32<void>*>(act + 0x68)) {
            void* node2 = ActTree_FindNode(name, 0);
            CatStore_SetUnknown(cat, node2);
            if (!node) {
                node = node2;
            }
        }
        mi->startNode = node;
    });
}

// 0x51B980
void __cdecl Remove(int cat) {
    ForEachInPool(ms_panimModelStore, [cat](CPAnimModelInfo* mi, int) {
        if (mi->Base()->GetPAnimIndex() == cat) {
            mi->startNode = nullptr;
        }
    });
    if (void* data = CatStore_GetUnknown(cat)) {
        ActTree_Release_5F5A60(data, 0, 0);
    }
    CatStore_SetUnknown(cat, nullptr);
}

} // namespace ModelInfoCats

// ---- start and end -------------------------------------------------------------------

namespace {

// CPool<T>::Init for the model stores (thiscall): new T[size] when the pool constructs
// its objects (+0x19), else plain memory
template <int ELEM_SIZE, uintptr_t CTOR>
void __fastcall InitStorePool(CPool<uint8_t>* pool, re::Edx) {
    pool->ownsArrays = 1;
    pool->elemSize = ELEM_SIZE;
    if (pool->unknown19) {
        // (count * size + 4, 0xFFFFFFFF when it overflows)
        uint64_t bytes = uint64_t(uint32_t(pool->size)) * ELEM_SIZE;
        bytes = bytes > 0xFFFFFFFF ? 0xFFFFFFFF : bytes + 4;
        bytes = bytes > 0xFFFFFFFF ? 0xFFFFFFFF : bytes;
        auto* block = static_cast<int32_t*>(CMI_Malloc(size_t(bytes)));
        if (block) {
            *block = pool->size;
            CMI_VectorConstruct(block + 1, ELEM_SIZE, pool->size, reinterpret_cast<void*>(CTOR));
            pool->objects = reinterpret_cast<uint8_t*>(block + 1);
        } else {
            pool->objects = nullptr;
        }
    } else {
        pool->objects = static_cast<uint8_t*>(CMI_Malloc(pool->size * ELEM_SIZE));
    }
    pool->flags = static_cast<uint8_t*>(CMI_Malloc(pool->size));
    pool->lastUsed = 0;
    for (int i = 0; i < pool->size; i++) {
        pool->flags[i] |= 0x80;
        pool->flags[i] &= 0x80;
    }
}

// 0x51B470: CPool<C2dEffect>::Flush (thiscall)
void __fastcall FlushEffectPool(CPool<C2dEffect>* pool, re::Edx) {
    if (pool->size <= 0) {
        return;
    }
    if (pool->ownsArrays) {
        if (pool->unknown19) {
            if (pool->objects) {
                CMI_2dEffectDestroy(pool->objects, 3);
            }
        } else {
            CMI_Free(pool->objects);
        }
        CMI_Free(pool->flags);
    }
    pool->objects = nullptr;
    pool->flags = nullptr;
    pool->size = 0;
    pool->lastUsed = 0;
}

// 0x51A980: the weapon store's constructor (thiscall; an argument it doesn't use)
struct WeaponModelStore {
    int32_t count;
    CWeaponModelInfo objects[NUM_WEAPON_MODELS];
};
WeaponModelStore* __fastcall ConstructWeaponStore(WeaponModelStore* store, re::Edx, int) {
    for (CWeaponModelInfo& mi : store->objects) {
        CMI_ClumpConstruct(&mi, MODEL_INFO_WEAPON);
        mi.unknown50 = weaponUnknown50Default;
        mi.vtable = ms_weaponVtable;
        mi.animFile2 = -1;
    }
    store->count = 0;
    return store;
}

template <typename T>
void ResetPoolFlags(CPool<T>& pool) {
    for (int i = 0; i < pool.size; i++) {
        pool.flags[i] |= 0x80;
        pool.flags[i] &= 0x80;
    }
}

template <typename T>
void ShutdownStore(int32_t count, T* store) {
    for (int i = 0; i < count; i++) {
        store[i].Base()->CallShutdown();
    }
}

} // namespace

namespace CModelInfo {

// 0x51C6A0
void __cdecl Initialise() {
    memset(ms_modelInfoPtrs, 0, NUM_MODEL_INFOS * sizeof(re::Field<CBaseModelInfo*>));   // (the game's 4-byte slots)
    SimplePool_Init(&ms_simpleModelStore);
    TimePool_Init(&ms_timeModelStore);
    ms_numWeaponModels = 0;
    ms_numAccessoryModels = 0;
    ms_numClumpModels = 0;
    ms_numVehicleModels = 0;
    ms_numPedModels = 0;
    EffectPool_Init(&ms_2dEffectPool);
    PAnimPool_Init(&ms_panimModelStore);
    ms_numClothModels = 0;
    HashTable_Load(&ms_hashTable);
    CMI_PushTreeMemoryTag();
    auto* tree = static_cast<ModelNameTree*>(CMI_Malloc(sizeof(ModelNameTree)));
    if (tree) {
        tree->root = -1;
    }
    ms_pNameTree = tree;
    CMI_PopTreeMemoryTag();
}

// 0x51B4D0
void __cdecl ShutDown() {
    CMI_Free(ms_hashTable.entries);
    ms_hashTable.entries = nullptr;
    ms_hashTable.count = 0;
    CMI_Free(ms_pNameTree);
    ms_pNameTree = nullptr;
    ForEachInPool(ms_simpleModelStore, [](CSimpleModelInfo* mi, int) { mi->Base()->CallShutdown(); });
    ForEachInPool(ms_timeModelStore, [](CTimeModelInfo* mi, int) { mi->Base()->CallShutdown(); });
    ForEachInPool(ms_2dEffectPool, [](C2dEffect* effect, int) {
        if (effect->object14) {
            CMI_NiRelease(effect->object14);
        }
        effect->object14 = nullptr;
        if (effect->object18) {
            CMI_NiRelease(effect->object18);
        }
        effect->object18 = nullptr;
    });
    ForEachInPool(ms_panimModelStore, [](CPAnimModelInfo* mi, int) { mi->Base()->CallShutdown(); });
    ShutdownStore(ms_numWeaponModels, ms_weaponModelStore);
    ShutdownStore(ms_numAccessoryModels, ms_accessoryModelStore);
    ShutdownStore(ms_numClumpModels, ms_clumpModelStore);
    ShutdownStore(ms_numVehicleModels, ms_vehicleModelStore);
    ShutdownStore(ms_numPedModels, ms_pedModelStore);
    ShutdownStore(ms_numClothModels, ms_clothModelStore);
    ResetPoolFlags(ms_simpleModelStore);
    ResetPoolFlags(ms_timeModelStore);
    ms_numWeaponModels = 0;
    ms_numAccessoryModels = 0;
    ms_numClumpModels = 0;
    ms_numVehicleModels = 0;
    ms_numPedModels = 0;
    ResetPoolFlags(ms_2dEffectPool);
    ResetPoolFlags(ms_panimModelStore);
    ms_numClothModels = 0;
    Pool_Flush(&ms_panimModelStore);
    EffectPool_Flush(&ms_2dEffectPool);
    Pool_Flush(&ms_timeModelStore);
    Pool_Flush(&ms_simpleModelStore);
}

// 0x51A940
void __cdecl DeleteNameTree() {
    CMI_Free(ms_pNameTree);
    ms_pNameTree = nullptr;
}

} // namespace CModelInfo

// ---- registry ------------------------------------------------------------------------

namespace {

// (the registry's entry points for the thiscalls)
int16_t __fastcall NameTree_Find(ModelNameTree* tree, re::Edx, uint32_t hash, uint8_t findFlags) {
    return tree->Find(hash, findFlags);
}
int16_t __fastcall NameTree_Insert(ModelNameTree* tree, re::Edx, int id) {
    return tree->Insert(int16_t(id));
}
int16_t __fastcall NameTree_Remove(ModelNameTree* tree, re::Edx, uint32_t hash) {
    return tree->Remove(hash);
}
int __fastcall HashTable_FindEntry(ModelHashTable* table, re::Edx, uint32_t hash) {
    return table->Find(hash);
}
void __fastcall HashTable_LoadEntry(ModelHashTable* table, re::Edx) {
    table->Load();
}

template <typename R>
void CompareResult(const char* name, uint32_t& compared, const char* what, R theirs, R ours) {
    re::Compared(name, compared);
    if (theirs != ours) {
        re::Mismatch(name, "%s: game %d, ours %d", what, int(intptr_t(theirs)), int(intptr_t(ours)));
    }
}

int16_t(__thiscall* Original_NameTree_Find)(ModelNameTree*, uint32_t, uint8_t);
int16_t __fastcall Shadow_NameTree_Find(ModelNameTree* tree, re::Edx, uint32_t hash, uint8_t findFlags) {
    int16_t theirs = Original_NameTree_Find(tree, hash, findFlags);
    static uint32_t compared = 0;
    CompareResult<int16_t>("ModelNameTree::Find", compared, "find", theirs, tree->Find(hash, findFlags));
    return theirs;
}

int(__cdecl* Original_GetModelIndex)(const char*);
int __cdecl Shadow_GetModelIndex(const char* name) {
    int theirs = Original_GetModelIndex(name);
    static uint32_t compared = 0;
    CompareResult<int>("CModelInfo::GetModelIndex", compared, name, theirs, CModelInfo::GetModelIndex(name));
    return theirs;
}

int(__cdecl* Original_GetModelIndexByHash)(uint32_t, int, int);
int __cdecl Shadow_GetModelIndexByHash(uint32_t hash, int first, int last) {
    int theirs = Original_GetModelIndexByHash(hash, first, last);
    static uint32_t compared = 0;
    CompareResult<int>("CModelInfo::GetModelIndexByHash", compared, "hash", theirs,
                       CModelInfo::GetModelIndexByHash(hash, first, last));
    return theirs;
}

int(__thiscall* Original_HashTable_Find)(ModelHashTable*, uint32_t);
int __fastcall Shadow_HashTable_Find(ModelHashTable* table, re::Edx, uint32_t hash) {
    int theirs = Original_HashTable_Find(table, hash);
    static uint32_t compared = 0;
    CompareResult<int>("ModelHashTable::Find", compared, "hash", theirs, table->Find(hash));
    return theirs;
}

// "shadow" for the tree's changes: ours, then the whole tree checked (as CTxdStore's)
void CheckTree(ModelNameTree* tree, const char* name, uint32_t& compared) {
    re::Compared(name, compared);
    if (tree->Check(tree->root, -1, int64_t(0x100000000), 0) < 0) {
        re::Mismatch(name, "the tree is broken (root %d)", tree->root);
    }
}
int16_t __fastcall Shadow_NameTree_Insert(ModelNameTree* tree, re::Edx, int id) {
    int16_t result = tree->Insert(int16_t(id));
    static uint32_t compared = 0;
    CheckTree(tree, "ModelNameTree::Insert", compared);
    return result;
}
int16_t __fastcall Shadow_NameTree_Remove(ModelNameTree* tree, re::Edx, uint32_t hash) {
    int16_t result = tree->Remove(hash);
    static uint32_t compared = 0;
    CheckTree(tree, "ModelNameTree::Remove", compared);
    return result;
}
int16_t(__thiscall* Original_NameTree_Insert)(ModelNameTree*, int);
int16_t(__thiscall* Original_NameTree_Remove)(ModelNameTree*, uint32_t);

} // namespace

REVERSED("ModelNameTree::Find", 0x0051BCE0, RE_BODY(&NameTree_Find), reinterpret_cast<void*>(&Shadow_NameTree_Find),
         reinterpret_cast<void**>(&Original_NameTree_Find), re::Mode::Original);
REVERSED("ModelNameTree::Insert", 0x0051BA60, RE_BODY(&NameTree_Insert),
         reinterpret_cast<void*>(&Shadow_NameTree_Insert), reinterpret_cast<void**>(&Original_NameTree_Insert), re::Mode::Original);
REVERSED("ModelNameTree::Remove", 0x0051BD90, RE_BODY(&NameTree_Remove),
         reinterpret_cast<void*>(&Shadow_NameTree_Remove), reinterpret_cast<void**>(&Original_NameTree_Remove), re::Mode::Original);
REVERSED("ModelHashTable::Find", 0x0051A8F0, RE_BODY(&HashTable_FindEntry),
         reinterpret_cast<void*>(&Shadow_HashTable_Find), reinterpret_cast<void**>(&Original_HashTable_Find), re::Mode::Original);
REVERSED("CModelInfo::GetModelIndex", 0x0051C330, RE_BODY(&CModelInfo::GetModelIndex),
         reinterpret_cast<void*>(&Shadow_GetModelIndex), reinterpret_cast<void**>(&Original_GetModelIndex), re::Mode::Original);
REVERSED("CModelInfo::GetModelIndexByHash", 0x0051AE20, RE_BODY(&CModelInfo::GetModelIndexByHash),
         reinterpret_cast<void*>(&Shadow_GetModelIndexByHash), reinterpret_cast<void**>(&Original_GetModelIndexByHash),
         re::Mode::Original);

#define CMI_REVERSED(name, fn, address)                                                                      \
    static void* RE_CONCAT(Original_, __LINE__);                                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

CMI_REVERSED("ModelHashTable::Load", &HashTable_LoadEntry, 0x0051A870);
CMI_REVERSED("CModelInfo::GetModelInfo", static_cast<CBaseModelInfo*(__cdecl*)(const char*, int*)>(&CModelInfo::GetModelInfo),
             0x0051C190);
CMI_REVERSED("CModelInfo::GetModelInfoInRange",
             static_cast<CBaseModelInfo*(__cdecl*)(const char*, int, int)>(&CModelInfo::GetModelInfoInRange), 0x0051C200);
CMI_REVERSED("CModelInfo::GetModelInfoInRangeWithId",
             static_cast<CBaseModelInfo*(__cdecl*)(const char*, int*, int, int)>(&CModelInfo::GetModelInfoInRange), 0x0051C260);
CMI_REVERSED("CModelInfo::GetModelIndexInRange", &CModelInfo::GetModelIndexInRange, 0x0051C2E0);
CMI_REVERSED("CModelInfo::GetModelIndexHashed", &CModelInfo::GetModelIndexHashed, 0x0051C390);
CMI_REVERSED("CModelInfo::GetStandIn", &CModelInfo::GetStandIn, 0x0051A960);
CMI_REVERSED("CModelInfo::GetStandInObject", &CModelInfo::GetStandInObject, 0x0051AFA0);
CMI_REVERSED("CModelInfo::GetButes", &CModelInfo::GetButes, 0x0051B450);
CMI_REVERSED("CModelInfo::IsVehicleClass1", &CModelInfo::IsVehicleClass1, 0x0051B3B0);
CMI_REVERSED("CModelInfo::IsPedModel", &CModelInfo::IsPedModel, 0x0051B3F0);
CMI_REVERSED("CModelInfo::IsVehicleModel", &CModelInfo::IsVehicleModel, 0x0051B420);
CMI_REVERSED("CModelInfo::GetWeaponIndex", &CModelInfo::GetWeaponIndex, 0x0051AEA0);
CMI_REVERSED("CModelInfo::GetWeaponIndexByHash", &CModelInfo::GetWeaponIndexByHash, 0x0051AF00);
CMI_REVERSED("CModelInfo::DeleteModelInfo", &CModelInfo::DeleteModelInfo, 0x0051C600);
CMI_REVERSED("CModelInfo::ClearModelInfo", &CModelInfo::ClearModelInfo, 0x0051C650);
CMI_REVERSED("CModelInfo::RemoveModelObjects", &CModelInfo::RemoveModelObjects, 0x0051CAC0);
CMI_REVERSED("CModelInfo::SetStandIn", &CModelInfo::SetStandIn, 0x0051CB30);
CMI_REVERSED("CModelInfo::RemoveStandIn", &CModelInfo::RemoveStandIn, 0x0051CBD0);
CMI_REVERSED("CModelInfo::ReInitModelInfo", &CModelInfo::ReInitModelInfo, 0x0051B360);
CMI_REVERSED("CModelInfo::AddSimpleModel", &CModelInfo::AddSimpleModel, 0x0051C740);
CMI_REVERSED("CModelInfo::AddTimeModel", &CModelInfo::AddTimeModel, 0x0051C7A0);
CMI_REVERSED("CModelInfo::AddWeaponModel", &CModelInfo::AddWeaponModel, 0x0051C800);
CMI_REVERSED("CModelInfo::AddAccessoryModel", &CModelInfo::AddAccessoryModel, 0x0051C860);
CMI_REVERSED("CModelInfo::AddVehicleModel", &CModelInfo::AddVehicleModel, 0x0051C8C0);
CMI_REVERSED("CModelInfo::AddPedModel", &CModelInfo::AddPedModel, 0x0051C960);
CMI_REVERSED("CModelInfo::AddPAnimModel", &CModelInfo::AddPAnimModel, 0x0051C9D0);
CMI_REVERSED("CModelInfo::AddClothModel", &CModelInfo::AddClothModel, 0x0051CA60);
CMI_REVERSED("CModelInfo::UpdatePAnimIndices", &CModelInfo::UpdatePAnimIndices, 0x0051BA20);
CMI_REVERSED("CModelInfo::PedModels_522CB0", &CModelInfo::PedModels_522CB0, 0x0051AE60);
CMI_REVERSED("CModelInfo::Fn_51B800", &CModelInfo::Fn_51B800, 0x0051B800);
CMI_REVERSED("CModelInfo::LoadDefaultPropActFiles", &CModelInfo::LoadDefaultPropActFiles, 0x0051C3D0);
CMI_REVERSED("CCatStore::Load", &ModelInfoCats::Load, 0x0051B880);
CMI_REVERSED("CCatStore::Remove", &ModelInfoCats::Remove, 0x0051B980);
CMI_REVERSED("CModelInfo::Initialise", &CModelInfo::Initialise, 0x0051C6A0);
CMI_REVERSED("CModelInfo::ShutDown", &CModelInfo::ShutDown, 0x0051B4D0);
CMI_REVERSED("CModelInfo::DeleteNameTree", &CModelInfo::DeleteNameTree, 0x0051A940);
CMI_REVERSED("CPool<CSimpleModelInfo>::Init", (&InitStorePool<0x34, 0x0051A770>), 0x0051AFD0);
CMI_REVERSED("CPool<CTimeModelInfo>::Init", (&InitStorePool<0x40, 0x0051A790>), 0x0051B080);
CMI_REVERSED("CPool<CPAnimModelInfo>::Init", (&InitStorePool<0x94, 0x0051A820>), 0x0051B130);
CMI_REVERSED("CPool<C2dEffect>::Init", (&InitStorePool<0x40, 0x0042A8C0>), 0x0051B1F0);
CMI_REVERSED("CPool<C2dEffect>::Flush", &FlushEffectPool, 0x0051B470);
CMI_REVERSED("CModelInfo::ConstructWeaponStore", &ConstructWeaponStore, 0x0051A980);
RE_UNHOOKED_FN("CModelInfo::CompareHash (bsearch)", 0x005A7D40, &CompareHashImpl);
