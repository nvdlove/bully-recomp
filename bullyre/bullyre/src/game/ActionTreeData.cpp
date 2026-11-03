// The action tree's compressed files (.cat in Act\Act.IMG, or streamed): a header (node
// counts, the offsets to patch and the condition lists) read into a scratch buffer and a
// data block kept by the tree (ActionNodeBankWithCompressedData +0x20). The nodes are made
// in arrays (banks, file references, playable implementations, leaves), then built from
// records: a node's name, its conditions, its tracks' descriptors and its children by
// kind letter ('b' bank, 'n' playable, 'l' leaf, 'i' / 'r' file references). Also the
// name keys' small helpers, a track descriptor's fields and key, and the node arrays'
// vector destructors. 0x5FA870..0x5FB7E0.

#include <cstdint>
#include <cstring>

#include "game/ActionTree.h"

#include "game/Crt.h"
#include "reversed.h"
using namespace acttree;

namespace {

// a node vector with ten inline slots (on the stack while a record is built)
struct NodeVector10 {
    re::Ptr32<uint8_t> inline_[10];  // +0x0
    re::Ptr32<re::Ptr32<uint8_t>> heap;        // +0x28 the rest
    uint32_t capacity;     // +0x2C the heap's
    uint32_t count;        // +0x30
};
static_assert(sizeof(NodeVector10) == 0x34);

// the build's cursors into the node arrays, the strings and the data
struct CatContext {
    re::Ptr32<uint8_t> banks;     // +0x0 (0x20 each)
    re::Ptr32<uint8_t> refs;      // +0x4 (0x30)
    re::Ptr32<uint8_t> impls;     // +0x8 (0x28)
    re::Ptr32<uint8_t> leaves;    // +0xC (0x18)
    re::Ptr32<uint8_t> strings;   // +0x10
    re::Ptr32<uint8_t> data;      // +0x14
};

// the data block's arrays (freed by 0x5FAD30)
struct CatData {
    re::Ptr32<uint8_t> conditionLists;  // +0x0 new[] of 0x10
    re::Ptr32<uint8_t> banks;           // +0x4 new[]
    re::Ptr32<uint8_t> refs;            // +0x8
    re::Ptr32<uint8_t> impls;           // +0xC
    re::Ptr32<uint8_t> leaves;          // +0x10
    // +0x14 the file's data
};

constexpr uint32_t kVtBank = 0x0092DB34;
constexpr uint32_t kVtBankCompressed = 0x0092DC74;
constexpr uint32_t kVtLeaf = 0x0092E024;
constexpr uint32_t kVtPlayableImpl = 0x0092E08C;

GAME_VAR(uint32_t, dotDotHash, 0x01B64A7C);
GAME_VAR(uint32_t, dotHash, 0x01B64A80);
GAME_VAR(void*, actDirectory, 0x01B649D8);
GAME_VAR(uint8_t*, actImgStream, 0x01B649E0);
GAME_VAR(int32_t, scratchCursor, 0x01B64A88);
GAME_ARRAY(uint8_t, scratchBuffer, 0x00BD7800);
inline void* const conditionRegistry = reinterpret_cast<void*>(0x01B64A94);
inline void* const catPool = reinterpret_cast<void*>(0x00C0F7B4);

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
inline void* __cdecl Memcpy(void* to, const void* from, size_t size) { return crt::Memcpy(to, from, size); }
inline char* __cdecl Strrchr(const char* s, int c) { return crt::Strrchr(s, c); }
inline char* __cdecl Strncpy(char* to, const char* from, size_t size) { return crt::Strncpy(to, from, size); }
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, size_t size, int count, void* ctor);
GAME_FN(uint32_t*, __thiscall, NameKey_FromValue, 0x005FA870, uint32_t* key, uint32_t value);
GAME_FN(uint32_t*, __thiscall, NameKey_Copy, 0x005FA880, uint32_t* key, const uint32_t* from);
GAME_FN(uint32_t*, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(uint32_t*, __thiscall, NameKey_Default, 0x0066BEA0, uint32_t* key);
GAME_FN(void, __thiscall, NameKey_Destruct, 0x0049A650, void* key);
GAME_FN(void, __thiscall, TrackDesc_Apply, 0x005FA940, uint8_t* desc, uint8_t* track);
GAME_FN(uint32_t*, __thiscall, TrackDesc_GetKey, 0x005FA9B0, uint8_t* desc, uint32_t* out);
GAME_FN(uint8_t*, __thiscall, Cat_ReadTrackDescs, 0x005FAA10, uint8_t* self, uint8_t* p, uint8_t* data, uint8_t* node);
GAME_FN(uint8_t*, __thiscall, Cat_ReadNodeConditions, 0x005FAA70, uint8_t* self, uint8_t* p, uint8_t* data, uint8_t* node);
GAME_FN(uint8_t*, __thiscall, Cat_ReadConditionList, 0x005FAAD0, uint8_t* p, uint8_t* data, uint8_t* list);
GAME_FN(uint8_t*, __thiscall, Cat_ReadPlayable, 0x005FAB20, uint8_t* p, CatContext* ctx, uint8_t* node);
GAME_FN(uint8_t*, __thiscall, Cat_ApplyFixups, 0x005FABD0, uint8_t* header, uint8_t* data);
GAME_FN(void, __thiscall, Vector10_Reserve, 0x005FAED0, NodeVector10* v, uint32_t capacity);
GAME_FN(void, __thiscall, Vector10_PushBack, 0x005FB0B0, NodeVector10* v, re::Ptr32<uint8_t>* value);
GAME_FN(uint8_t*, __thiscall, Cat_ReadChildren, 0x005FB110, uint8_t* self, uint8_t* p, CatContext* ctx, uint8_t* node);
GAME_FN(uint8_t*, __thiscall, Cat_ReadBank, 0x005FB300, uint8_t* p, CatContext* ctx, uint8_t* node);
GAME_FN(uint8_t*, __thiscall, Cat_Build, 0x005FB380, uint8_t* header, uint8_t* data);
GAME_FN(uint8_t*, __thiscall, ConditionRegistry_Create, 0x0061A650, void* registry, uint8_t* name, uint8_t* data);
GAME_FN(void, __thiscall, ConditionRegistry_Shutdown, 0x0061A780, void* registry);
GAME_FN(void, __thiscall, Node_ReserveConditions, 0x005F60F0, uint8_t* node, uint32_t count);
GAME_FN(void, __thiscall, Conditions_Reserve, 0x005FBD30, uint8_t* list, uint32_t count);
GAME_FN(void, __thiscall, Conditions_Add, 0x005FBD50, uint8_t* list, uint8_t* condition);
GAME_FN(uint8_t*, __thiscall, Conditions_Construct, 0x005FBAD0, uint8_t* list);
GAME_FN(void, __thiscall, Conditions_Destruct, 0x005FBAE0, uint8_t* list);
GAME_FN(uint8_t*, __thiscall, Playable_Construct, 0x005F5FB0, uint8_t* node);
GAME_FN(void, __thiscall, Playable_Destruct, 0x005F60A0, uint8_t* node);
GAME_FN(void, __thiscall, Implementation_Destruct, 0x005F5DC0, uint8_t* node);
GAME_FN(uint8_t*, __thiscall, Bank_Construct, 0x005F65E0, uint8_t* node);
GAME_FN(void, __thiscall, FileReference_Destruct, 0x005F67E0, uint8_t* node);
GAME_FN(void, __thiscall, FileReference_SetFile, 0x005F56B0, uint8_t* node, const char* file);
GAME_FN(void, __thiscall, FileReference_SetFolderFromPath, 0x005F5700, uint8_t* node, const char* path);
GAME_FN(int16_t, __thiscall, CatPool_FindIndex, 0x004507B0, void* pool, const char* name);
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int id, int flags);
GAME_FN(void, __cdecl, Timing_Mark_43C4F0, 0x0043C4F0, const char* where, const char* what, uintptr_t);
GAME_FN(void, __cdecl, LoadAllRequestedModels, 0x00533EE0, int);
GAME_FN(int, __cdecl, FileMgr_OpenFile, 0x0042D260, const char* path, const char* mode, int);
GAME_FN(void, __cdecl, FileMgr_Seek, 0x0042D300, int handle, int offset, int from);
GAME_FN(int, __cdecl, FileMgr_Read, 0x0042D2A0, int handle, void* to, int size);
GAME_FN(void, __cdecl, FileMgr_CloseFile, 0x0042D450, int handle);
GAME_FN(int, __cdecl, Stream_Read, 0x0042E730, uint8_t* stream, void* to, int size);
GAME_FN(uint8_t, __thiscall, Directory_FindItem, 0x004290E0, void* dir, const char* name, int32_t* offset, int32_t* size);

// count * size, -1 on overflow
uint32_t MulBytes(uint32_t count, uint32_t size) {
    uint64_t bytes = uint64_t(count) * size;
    return bytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(bytes);
}

// the same plus new[]'s count, -1 on overflow
uint32_t ArrayBytes(uint32_t count, uint32_t size) {
    uint64_t bytes = uint64_t(count) * size;
    uint32_t n = bytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(bytes);
    return n > 0xFFFFFFFBu ? 0xFFFFFFFFu : n + 4;
}

// 0x5FA870 (thiscall): a name key from a hash
uint32_t* __fastcall NameKey_FromValueImpl(uint32_t* self, re::Edx, uint32_t value) {
    *self = value;
    return self;
}

// 0x5FA880 (thiscall): copied
uint32_t* __fastcall NameKey_CopyImpl(uint32_t* self, re::Edx, const uint32_t* from) {
    *self = *from;
    return self;
}

// 0x5FA890 (thiscall): assigned from a name (nullptr: 0)
void __fastcall NameKey_FromString(uint32_t* self, re::Edx, const char* name) {
    if (name) {
        *self = HashName(name);
        return;
    }
    *self = 0;
}

// 0x5FA8C0 (thiscall): a hashed path {count, hashes}
int32_t* __fastcall HashPath_Set(int32_t* self, re::Edx, int32_t count, uint32_t* hashes) {
    self[0] = count;
    self[1] = int32_t(re::Addr32(hashes));
    return self;
}

// 0x5FA8E0 (cdecl): the hashes of ".." and "."
void __cdecl InitPathHashes() {
    dotDotHash = HashName("..");
    dotHash = HashName(".");
}

// 0x5FA910 (thiscall): a name key constructed from a name (nullptr: 0)
uint32_t* __fastcall NameKey_ConstructImpl(uint32_t* self, re::Edx, const char* name) {
    *self = 0;
    if (name) {
        *self = HashName(name);
        return self;
    }
    *self = 0;
    return self;
}

// 0x5FA940 (thiscall): a track's fields from its descriptor (a base descriptor first,
// then {offset << 3 | size bits | more} patches), its key the descriptor
void __fastcall TrackDesc_ApplyImpl(uint8_t* self, re::Edx, uint8_t* track) {
    uint16_t parent = vc::At<uint16_t>(self, 0);
    if (parent) {
        TrackDesc_Apply(self + parent, track);
    }
    uint8_t* p = self + 2;
    uint16_t head;
    do {
        head = *reinterpret_cast<uint16_t*>(p);
        uint32_t size = 1u << ((head >> 1) & 3);
        uint32_t offset = head >> 3;
        p += 2;
        if (offset) {
            Memcpy(track + offset, p, size);
        }
        p += size;
    } while (head & 1);
    vc::At<uint8_t*>(track, 0x14) = self;
}

// 0x5FA9B0 (thiscall): the track's type name (a derived descriptor: its base's)
uint32_t* __fastcall TrackDesc_GetKeyImpl(uint8_t* self, re::Edx, uint32_t* out) {
    if (vc::At<uint16_t>(self, 2) & 0xFFF8) {
        return TrackDesc_GetKey(self + vc::At<uint16_t>(self, 0), out);
    }
    uint32_t key;
    NameKey_Default(&key);
    key = vc::At<uint32_t>(self, 4);
    NameKey_Copy(out, &key);
    NameKey_Destruct(&key);
    return out;
}

// 0x5FAA10 (thiscall, this unused): a node's track descriptors {count byte, offsets}
uint8_t* __fastcall Cat_ReadTrackDescsImpl(uint8_t*, re::Edx, uint8_t* p, uint8_t* data, uint8_t* node) {
    uint8_t count = *p++;
    vc::At<uint32_t>(node, 0x10) = count;
    re::Ptr32<uint8_t>* descs = static_cast<re::Ptr32<uint8_t>*>(Malloc(MulBytes(count, 4)));
    vc::At<re::Ptr32<uint8_t>*>(node, 0x14) = descs;
    for (uint32_t i = 0; i < count; i++) {
        vc::At<re::Ptr32<uint8_t>*>(node, 0x14)[i] = data + vc::At<int32_t>(p, 0);
        p += 4;
    }
    return p;
}

// 0x5FAA70 (thiscall, this unused): a node's conditions {count byte, offsets}, each made
// by the registry from its name and data
uint8_t* __fastcall Cat_ReadNodeConditionsImpl(uint8_t*, re::Edx, uint8_t* p, uint8_t* data, uint8_t* node) {
    uint8_t count = *p++;
    Node_ReserveConditions(node, count);
    for (uint32_t i = count; i != 0; i--) {
        uint8_t* entry = data + vc::At<int32_t>(p, 0);
        p += 4;
        uint8_t* condition = ConditionRegistry_Create(conditionRegistry, entry, entry + 4);
        vc::Call<uint8_t*>(node, kNodeAddCondition, condition);
    }
    return p;
}

// 0x5FAAD0 (thiscall on the record): a shared condition list's conditions
uint8_t* __fastcall Cat_ReadConditionListImpl(uint8_t* self, re::Edx, uint8_t* data, uint8_t* list) {
    uint8_t count = self[0];
    uint8_t* p = self + 1;
    Conditions_Reserve(list, count);
    for (uint32_t i = count; i != 0; i--) {
        uint8_t* entry = data + vc::At<int32_t>(p, 0);
        p += 4;
        Conditions_Add(list, ConditionRegistry_Create(conditionRegistry, entry, entry + 4));
    }
    return p;
}

// a record's name: the high bit a hash, else a string's offset
void ReadName(uint8_t* p, CatContext* ctx, uint8_t* node) {
    int32_t v = vc::At<int32_t>(p, 0);
    uint32_t key;
    if (v < 0) {
        NameKey_FromValue(&key, uint32_t(v) & 0x7FFFFFFF);
    } else {
        NameKey_Construct(&key, reinterpret_cast<const char*>(uint32_t(v) + ctx->strings));
    }
    vc::At<uint32_t>(node, 8) = key;
    NameKey_Destruct(&key);
}

// 0x5FAB20 (thiscall on the record): a playable node: name, conditions, tracks, children
uint8_t* __fastcall Cat_ReadPlayableImpl(uint8_t* self, re::Edx, CatContext* ctx, uint8_t* node) {
    ReadName(self, ctx, node);
    uint8_t* p = Cat_ReadNodeConditions(self, self + 4, ctx->data, node);
    p = Cat_ReadTrackDescs(self, p, ctx->data, node);
    return Cat_ReadChildren(self, p, ctx, node);
}

// 0x5FABB0 (thiscall): ActionNodePlayableLeafImplementation's constructor
uint8_t* __fastcall Leaf_Construct(uint8_t* self, re::Edx) {
    Playable_Construct(self);
    vc::At<uint32_t>(self, 0) = kVtLeaf;
    return self;
}

// 0x5FABD0 (thiscall on the header): the data's pointers patched (to strings, then to
// the condition lists made here), the lists filled; returns the root's record
uint8_t* __fastcall Cat_ApplyFixupsImpl(uint8_t* self, re::Edx, uint8_t* data) {
    uint8_t* strings = data + 0x14 + (vc::At<int32_t>(self, 8) - vc::At<int32_t>(self, 4));
    uint8_t* p = self + 0x20;
    uint32_t n = vc::At<uint32_t>(p, 0);
    p += 4;
    for (uint32_t i = n; i != 0; i--) {
        uint8_t* target = strings + vc::At<int32_t>(p, 0);
        p += 4;
        uint32_t fixes = vc::At<uint16_t>(p, 0);
        p += 2;
        for (; fixes != 0; fixes--) {
            vc::At<uint8_t*>(data, vc::At<int32_t>(p, 0) + 0x14) = target;
            p += 4;
        }
    }
    uint32_t lists = vc::At<uint32_t>(p, 0);
    p += 4;
    uint8_t* records = p;
    CatData* cat = reinterpret_cast<CatData*>(data);
    cat->conditionLists = nullptr;
    if (lists != 0) {
        uint32_t* block = static_cast<uint32_t*>(Malloc(ArrayBytes(lists, 0x10)));
        uint8_t* arr = nullptr;
        if (block) {
            block[0] = lists;
            arr = reinterpret_cast<uint8_t*>(block + 1);
            for (uint32_t i = 0; i < lists; i++) {
                Conditions_Construct(arr + i * 0x10);
            }
        }
        cat->conditionLists = arr;
        for (uint32_t i = 0; i < lists; i++) {
            uint8_t* list = cat->conditionLists + i * 0x10;
            uint32_t fixes = vc::At<uint16_t>(p, 4);
            p += 6;
            for (; fixes != 0; fixes--) {
                vc::At<uint8_t*>(data, vc::At<int32_t>(p, 0) + 0x14) = list;
                p += 4;
            }
        }
    }
    p = records;
    for (uint32_t i = 0; i < lists; i++) {
        uint8_t* source = self + vc::At<int32_t>(self, 0xC) + vc::At<int32_t>(p, 0);
        uint32_t fixes = vc::At<uint16_t>(p, 4);
        Cat_ReadConditionList(source, data + 0x14, cat->conditionLists + i * 0x10);
        p += 4 + fixes * 4 + 2;
    }
    return p;
}

// a new[] block (its count before the array) deleted: through the first element's vector
// deleting destructor, or freed when empty
void DeleteNodeArray(uint8_t* arr) {
    if (!arr) {
        return;
    }
    if (vc::At<uint32_t>(arr, -4) == 0) {
        Free(arr - 4);
        return;
    }
    vc::Call<void>(arr, 0, 3);
}

// 0x5FAD30 (thiscall): the data block's arrays deleted
void __fastcall CatData_Destruct(CatData* self, re::Edx) {
    if (uint8_t* lists = self->conditionLists) {
        uint32_t count = vc::At<uint32_t>(lists, -4);
        for (int32_t i = int32_t(count) - 1; i >= 0; i--) {
            Conditions_Destruct(lists + i * 0x10);
        }
        Free(lists - 4);
    }
    DeleteNodeArray(self->banks);
    DeleteNodeArray(self->refs);
    DeleteNodeArray(self->impls);
    DeleteNodeArray(self->leaves);
}

// the vector deleting destructors: flags 2 an array (count before it), 1 freed
template <int Size, typename Destruct>
uint8_t* VectorDeletingDestruct(uint8_t* self, uint32_t flags, Destruct destruct) {
    if (flags & 2) {
        uint8_t* block = self - 4;
        int32_t count = vc::At<int32_t>(block, 0);
        for (int32_t i = count - 1; i >= 0; i--) {
            destruct(self + i * Size);
        }
        if (flags & 1) {
            Free(block);
        }
        return block;
    }
    destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x5FAE00 (thiscall): ActionNodeFileReference's
uint8_t* __fastcall FileReference_VectorDeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    return VectorDeletingDestruct<0x30>(self, flags, [](uint8_t* node) { FileReference_Destruct(node); });
}

// 0x5FAE70 (thiscall): ActionNodePlayableLeafImplementation's
uint8_t* __fastcall Leaf_VectorDeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    return VectorDeletingDestruct<0x18>(self, flags, [](uint8_t* node) { Playable_Destruct(node); });
}

// 0x5FAED0 (thiscall): the heap part grown to `capacity`
void __fastcall Vector10_ReserveImpl(NodeVector10* self, re::Edx, uint32_t capacity) {
    if (capacity <= self->capacity) {
        return;
    }
    re::Ptr32<uint8_t>* heap = static_cast<re::Ptr32<uint8_t>*>(Malloc(MulBytes(capacity, 4)));
    if (self->heap) {
        for (uint32_t i = 0; self->count - 10 != 0 && i < self->count - 10; i++) {
            heap[i] = self->heap[i];
        }
        Free(self->heap);
    }
    self->heap = heap;
    self->capacity = capacity;
}

// 0x5FAF40 (thiscall): ActionNodeBank's (its children's array freed first)
uint8_t* __fastcall Bank_VectorDeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    return VectorDeletingDestruct<0x20>(self, flags, [](uint8_t* node) {
        vc::At<uint32_t>(node, 0) = kVtBank;
        if (vc::At<void*>(node, 0x14)) {
            Free(vc::At<void*>(node, 0x14));
            vc::At<void*>(node, 0x14) = nullptr;
        }
        Implementation_Destruct(node);
    });
}

// 0x5FAFF0 (thiscall): ActionNodePlayableImplementation's
uint8_t* __fastcall PlayableImpl_VectorDeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    return VectorDeletingDestruct<0x28>(self, flags, [](uint8_t* node) {
        if (vc::At<void*>(node, 0x1C)) {
            Free(vc::At<void*>(node, 0x1C));
            vc::At<void*>(node, 0x1C) = nullptr;
        }
        Playable_Destruct(node);
    });
}

// 0x5FB090 (thiscall): ActionNodePlayableImplementation's constructor
uint8_t* __fastcall PlayableImpl_Construct(uint8_t* self, re::Edx) {
    Playable_Construct(self);
    vc::At<uint32_t>(self, 0) = kVtPlayableImpl;
    vc::At<uint32_t>(self, 0x1C) = 0;
    vc::At<uint32_t>(self, 0x20) = 0;
    vc::At<uint32_t>(self, 0x24) = 0;
    return self;
}

// 0x5FB0B0 (thiscall)
void __fastcall Vector10_PushBackImpl(NodeVector10* self, re::Edx, re::Ptr32<uint8_t>* value) {
    if (self->count == self->capacity + 10) {
        Vector10_Reserve(self, self->capacity == 0 ? 1 : self->capacity * 2);
    }
    if (self->count < 10) {
        self->inline_[self->count] = *value;
        self->count++;
        return;
    }
    self->heap[self->count - 10] = *value;
    self->count++;
}

// 0x5FB110 (thiscall, this unused): a node's children {count, kind letter + record}:
// made from the context's arrays, then added to the node
uint8_t* __fastcall Cat_ReadChildrenImpl(uint8_t*, re::Edx, uint8_t* p, CatContext* ctx, uint8_t* node) {
    uint16_t count = vc::At<uint16_t>(p, 0);
    p += 2;
    NodeVector10 children;
    children.heap = nullptr;
    children.capacity = 0;
    children.count = 0;
    for (uint32_t left = count; left != 0; left--) {
        char kind = char(*p++);
        re::Ptr32<uint8_t> child;
        switch (kind) {
        case 'b':
            child = ctx->banks;
            ctx->banks += 0x20;
            Vector10_PushBack(&children, &child);
            p = Cat_ReadBank(p, ctx, child);
            break;
        case 'n':
            child = ctx->impls;
            ctx->impls += 0x28;
            Vector10_PushBack(&children, &child);
            p = Cat_ReadPlayable(p, ctx, child);
            break;
        case 'l':
            child = ctx->leaves;
            ctx->leaves += 0x18;
            Vector10_PushBack(&children, &child);
            p = Cat_ReadPlayable(p, ctx, child);
            break;
        case 'i':
        case 'r': {
            child = ctx->refs;
            ctx->refs += 0x30;
            // (the push inlined)
            if (children.count == children.capacity + 10) {
                Vector10_Reserve(&children, children.capacity == 0 ? 1 : children.capacity * 2);
            }
            if (children.count < 10) {
                children.inline_[children.count] = child;
            } else {
                children.heap[children.count - 10] = child;
            }
            children.count++;
            uint8_t* strings = ctx->strings;
            FileReference_SetFile(child, reinterpret_cast<const char*>(strings + vc::At<int32_t>(p, 0)));
            FileReference_SetFolderFromPath(child, reinterpret_cast<const char*>(strings + vc::At<int32_t>(p, 4)));
            p += 8;
            child[0x28] = kind == 'i';
            break;
        }
        default:
            break;
        }
    }
    uint32_t n = children.count;
    vc::Call<void>(node, kNodeReserve, n);
    for (uint32_t i = 0; i < children.count; i++) {
        uint8_t* child = i < 10 ? children.inline_[i] : children.heap[i - 10];
        vc::Call<void>(node, kNodeAddChild, child);
    }
    if (children.heap) {
        Free(children.heap);
    }
    return p;
}

// 0x5FB300 (thiscall on the record): a bank: name, conditions, children
uint8_t* __fastcall Cat_ReadBankImpl(uint8_t* self, re::Edx, CatContext* ctx, uint8_t* node) {
    ReadName(self, ctx, node);
    uint8_t* p = Cat_ReadNodeConditions(self, self + 4, ctx->data, node);
    return Cat_ReadChildren(self, p, ctx, node);
}

// a new[] array of nodes made with their constructor (nullptr: out of memory)
uint8_t* NewNodeArray(uint32_t count, uint32_t size, uintptr_t ctor) {
    uint32_t* block = static_cast<uint32_t*>(Malloc(ArrayBytes(count, size)));
    if (!block) {
        return nullptr;
    }
    block[0] = count;
    VectorConstruct(block + 1, size, int(count), reinterpret_cast<void*>(ctor));
    return reinterpret_cast<uint8_t*>(block + 1);
}

// 0x5FB380 (thiscall on the header): the tree built: fixups, the node arrays, then the
// root (a bank with the compressed data) from the record after them
uint8_t* __fastcall Cat_BuildImpl(uint8_t* self, re::Edx, uint8_t* data) {
    uint8_t* rootRecord = Cat_ApplyFixups(self, data) + 1;
    CatData* cat = reinterpret_cast<CatData*>(data);
    cat->banks = nullptr;
    cat->impls = nullptr;
    cat->refs = nullptr;
    cat->leaves = nullptr;
    uint32_t banks = vc::At<uint32_t>(self, 0x10);
    if (banks != 0) {
        cat->banks = NewNodeArray(banks, 0x20, 0x005F65E0);
    }
    uint32_t refs = vc::At<uint32_t>(self, 0x18);
    if (refs != 0) {
        cat->refs = NewNodeArray(refs, 0x30, 0x005F62D0);
    }
    uint32_t impls = vc::At<uint32_t>(self, 0x14) - vc::At<uint32_t>(self, 0x1C);
    if (impls != 0) {
        cat->impls = NewNodeArray(impls, 0x28, 0x005FB090);
    }
    uint32_t leaves = vc::At<uint32_t>(self, 0x1C);
    if (leaves != 0) {
        cat->leaves = NewNodeArray(leaves, 0x18, 0x005FABB0);
    }
    CatContext ctx;
    ctx.refs = cat->refs;
    ctx.banks = cat->banks;
    ctx.impls = cat->impls;
    ctx.leaves = cat->leaves;
    ctx.data = data + 0x14;
    ctx.strings = data + 0x14 + (vc::At<int32_t>(self, 8) - vc::At<int32_t>(self, 4));
    uint8_t* tree = static_cast<uint8_t*>(Malloc(0x24));
    if (tree) {
        Bank_Construct(tree);
        vc::At<uint32_t>(tree, 0) = kVtBankCompressed;
        vc::At<uint8_t*>(tree, 0x20) = data;
    } else {
        tree = nullptr;
    }
    Cat_ReadBank(rootRecord, &ctx, tree);
    return tree;
}

// 0x5FB550 (cdecl): a .cat tree loaded: a streamed one requested and loaded (0 returned:
// it arrives through CCatStore), else read from Act\Act.IMG (its header into the scratch
// buffer)
uint8_t* __cdecl LoadCatFile(const char* path) {
    const char* name = Strrchr(path, '\\');
    name = name ? name + 1 : path;
    if (const char* slash = Strrchr(name, '/')) {
        name = slash + 1;
    }
    char base[0x20];
    Strncpy(base, name, 0x20);
    if (char* dot = Strrchr(base, '.')) {
        *dot = 0;
    }
    int16_t slot = CatPool_FindIndex(catPool, base);
    if (slot != -1) {
        RequestModel(slot + 0x5D41, 0x17);
        Timing_Mark_43C4F0("CompressedActionTree::load()", "CStreaming::LoadAllRequestedModels()", 0);
        LoadAllRequestedModels(0);
        return nullptr;
    }
    int handle = 0;
    int32_t offset = 0;
    if (!actImgStream) {
        handle = FileMgr_OpenFile("Act\\Act.IMG", "r", 1);
        if (!handle) {
            return nullptr;
        }
    }
    int32_t sectors;
    if (!Directory_FindItem(actDirectory, name, &offset, &sectors)) {
        return nullptr;
    }
    offset <<= 11;
    if (actImgStream) {
        vc::At<int32_t>(actImgStream, 0xC) = offset;
    } else {
        FileMgr_Seek(handle, offset, 0);
    }
    int32_t total = 0;
    if (actImgStream) {
        Stream_Read(actImgStream, &total, 4);
    } else {
        FileMgr_Read(handle, &total, 4);
    }
    uint32_t headerSize = 0;
    if (actImgStream) {
        Stream_Read(actImgStream, &headerSize, 4);
    } else {
        FileMgr_Read(handle, &headerSize, 4);
    }
    if (actImgStream) {
        vc::At<int32_t>(actImgStream, 0xC) = offset;
    } else {
        FileMgr_Seek(handle, offset, 0);
    }
    uint8_t* header = scratchBuffer + scratchCursor;
    scratchCursor = scratchCursor + int32_t(headerSize - (headerSize & 0xF)) + 0x10;
    if (actImgStream) {
        Stream_Read(actImgStream, header, int(headerSize));
    } else {
        FileMgr_Read(handle, header, int(headerSize));
    }
    int32_t dataSize = total - int32_t(headerSize);
    uint8_t* data = static_cast<uint8_t*>(Malloc(size_t(dataSize + 0x14)));
    if (actImgStream) {
        Stream_Read(actImgStream, data + 0x14, dataSize);
    } else {
        FileMgr_Read(handle, data + 0x14, dataSize);
    }
    if (!actImgStream) {
        FileMgr_CloseFile(handle);
        handle = 0;
    }
    uint8_t* tree = Cat_Build(header, data);
    scratchCursor = scratchCursor + int32_t((headerSize & 0xF) - headerSize) - 0x10;
    if (handle) {
        FileMgr_CloseFile(handle);
    }
    return tree;
}

// 0x5FB7A0 (cdecl): a tree from a streamed .cat (its header first, its data copied)
uint8_t* __cdecl LoadCatData(void*, uint8_t* file) {
    uint32_t size = vc::At<uint32_t>(file, 0) - vc::At<uint32_t>(file, 4);
    uint8_t* data = static_cast<uint8_t*>(Malloc(size + 0x14));
    Memcpy(data + 0x14, file + vc::At<uint32_t>(file, 4), size);
    return Cat_Build(file, data);
}

// 0x5FB7E0 (cdecl): the condition registry's shutdown
void __cdecl ShutdownConditions() {
    ConditionRegistry_Shutdown(conditionRegistry);
}

} // namespace

#define ACTDATA_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

ACTDATA_REVERSED("NameKey::FromValue", 0x005FA870, &NameKey_FromValueImpl);
ACTDATA_REVERSED("NameKey::Copy", 0x005FA880, &NameKey_CopyImpl);
ACTDATA_REVERSED("NameKey::FromString", 0x005FA890, &NameKey_FromString);
ACTDATA_REVERSED("HashPath::Set", 0x005FA8C0, &HashPath_Set);
ACTDATA_REVERSED("ActionTree::InitPathHashes", 0x005FA8E0, &InitPathHashes);
ACTDATA_REVERSED("NameKey::Construct", 0x005FA910, &NameKey_ConstructImpl);
ACTDATA_REVERSED("TrackDescriptor::Apply", 0x005FA940, &TrackDesc_ApplyImpl);
ACTDATA_REVERSED("TrackDescriptor::GetKey", 0x005FA9B0, &TrackDesc_GetKeyImpl);
ACTDATA_REVERSED("CompressedActionTree::ReadTrackDescs", 0x005FAA10, &Cat_ReadTrackDescsImpl);
ACTDATA_REVERSED("CompressedActionTree::ReadNodeConditions", 0x005FAA70, &Cat_ReadNodeConditionsImpl);
ACTDATA_REVERSED("CompressedActionTree::ReadConditionList", 0x005FAAD0, &Cat_ReadConditionListImpl);
ACTDATA_REVERSED("CompressedActionTree::ReadPlayable", 0x005FAB20, &Cat_ReadPlayableImpl);
ACTDATA_REVERSED("ActionNodePlayableLeafImplementation::Construct", 0x005FABB0, &Leaf_Construct);
ACTDATA_REVERSED("CompressedActionTree::ApplyFixups", 0x005FABD0, &Cat_ApplyFixupsImpl);
ACTDATA_REVERSED("CompressedActionTree::DestructData", 0x005FAD30, &CatData_Destruct);
ACTDATA_REVERSED("ActionNodeFileReference::VectorDeletingDestruct", 0x005FAE00, &FileReference_VectorDeletingDestruct);
ACTDATA_REVERSED("ActionNodePlayableLeafImplementation::VectorDeletingDestruct", 0x005FAE70, &Leaf_VectorDeletingDestruct);
ACTDATA_REVERSED("CompressedActionTree::Vector10Reserve", 0x005FAED0, &Vector10_ReserveImpl);
ACTDATA_REVERSED("ActionNodeBank::VectorDeletingDestruct", 0x005FAF40, &Bank_VectorDeletingDestruct);
ACTDATA_REVERSED("ActionNodePlayableImplementation::VectorDeletingDestruct", 0x005FAFF0, &PlayableImpl_VectorDeletingDestruct);
ACTDATA_REVERSED("ActionNodePlayableImplementation::Construct", 0x005FB090, &PlayableImpl_Construct);
ACTDATA_REVERSED("CompressedActionTree::Vector10PushBack", 0x005FB0B0, &Vector10_PushBackImpl);
ACTDATA_REVERSED("CompressedActionTree::ReadChildren", 0x005FB110, &Cat_ReadChildrenImpl);
ACTDATA_REVERSED("CompressedActionTree::ReadBank", 0x005FB300, &Cat_ReadBankImpl);
ACTDATA_REVERSED("CompressedActionTree::Build", 0x005FB380, &Cat_BuildImpl);
ACTDATA_REVERSED("CompressedActionTree::LoadFile", 0x005FB550, &LoadCatFile);
ACTDATA_REVERSED("CompressedActionTree::LoadData", 0x005FB7A0, &LoadCatData);
ACTDATA_REVERSED("ActionTree::ShutdownConditions", 0x005FB7E0, &ShutdownConditions);
