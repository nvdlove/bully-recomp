// The scripts' paths (loaded from the areas' data: 0x14 bytes each from the pool at
// 0x20C7C50; PatrolPaths.cpp reads the same objects): each a chain of nodes (0x24 bytes
// from the pool at 0x20C7C8C, linked through the u16 array at 0xBC5D14, 0xFFFE ending a
// chain) with their actions; the path manager (a list of references), and a path
// reference (0x34 bytes: something moving along a path: its segment, speed and
// distance along it, the segment's direction, heading and pitch). 0x5EC5E0..0x5EDA30.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace {

struct PnVec {
    float x, y, z;
};

// A node's action (0x14)
struct PathAction {
    int32_t type;      // +0x0
    PnVec value;       // +0x4
    float time;        // +0x10
};
static_assert(sizeof(PathAction) == 0x14);

// A growable list of pointers
struct PtrList {
    re::Ptr32<re::Ptr32<void>> data;       // +0x0
    uint32_t capacity; // +0x4
    uint32_t count;    // +0x8
};

struct PathNode {
    PnVec pos;             // +0x0
    PnVec look;            // +0xC
    int32_t id;            // +0x18
    float wait;            // +0x1C
    re::Ptr32<PtrList> actions;      // +0x20
};
static_assert(sizeof(PathNode) == 0x24);

struct Path {
    re::Ptr32<re::Ptr32<void>> vtable;         // +0x0 (0x92CF90)
    uint8_t flag4;         // +0x4
    uint8_t flag5;         // +0x5
    uint8_t datSet;        // +0x6 (which load it came with)
    uint8_t flag7;         // +0x7
    uint32_t name;         // +0x8 (hashed)
    int32_t scriptId;      // +0xC (-2: none)
    uint16_t first;        // +0x10 (0xFFFF: no nodes)
    uint16_t iterator;     // +0x12
};
static_assert(sizeof(Path) == 0x14);

struct PathManager {
    re::Ptr32<re::Ptr32<void>> vtable;         // +0x0
    re::Ptr32<re::Ptr32<void>> refs;           // +0x4
    uint32_t capacity;     // +0x8
    uint32_t count;        // +0xC
};

struct PathReference {
    re::Ptr32<re::Ptr32<void>> vtable;         // +0x0 (0x92CE50)
    re::Ptr32<Path> path;            // +0x4
    re::Ptr32<PathManager> owner;    // +0x8
    uint8_t atEnd;         // +0xC
    uint8_t padD[3];
    uint32_t segment;      // +0x10 (from 1: the node it's heading for)
    float speed;           // +0x14
    float along;           // +0x18
    PnVec dir;             // +0x1C
    float heading;         // +0x28
    float pitch;           // +0x2C
    float length;          // +0x30
};
static_assert(sizeof(PathReference) == 0x34);

// the node chains' list (0xBC5D10: +4 the next indices, +8 the last added)
struct NodeChains {
    uint32_t unknown0;
    re::Ptr32<uint16_t> next;
    uint16_t last;
};

GAME_VAR(uint8_t*, pn_pathPool, 0x020C7C50);
GAME_VAR(uint8_t*, pn_nodePool, 0x020C7C8C);
inline NodeChains* const pn_chains = reinterpret_cast<NodeChains*>(0x00BC5D10);
GAME_VAR(uint8_t, pn_datSet, 0x020C7E94);
GAME_VAR(void*, pn_pathMgr, 0x00C2B034);
inline void* const pn_scripts = reinterpret_cast<void*>(0x00D02850);
constexpr uintptr_t kPathVtable = 0x0092CF90;
constexpr uintptr_t kArchivableVtable = 0x00900CDC;
constexpr uintptr_t kPathManagerVtable = 0x0092D050;
constexpr uintptr_t kPathReferenceVtable = 0x0092CE50;
// the archive's keys
GAME_VAR(const char*, pn_keyName, 0x00AEB96C);
GAME_VAR(const char*, pn_keyFlag4, 0x00AEB970);
GAME_VAR(const char*, pn_keyFlag5, 0x00AEB974);
GAME_VAR(const char*, pn_keyPos, 0x00AEB978);
GAME_VAR(const char*, pn_keyLook, 0x00AEB97C);
GAME_VAR(const char*, pn_keyWait, 0x00AEB980);
GAME_VAR(const char*, pn_keyId, 0x00AEB988);
GAME_VAR(const char*, pn_keyNodeCount, 0x00AEB98C);
GAME_VAR(const char*, pn_keyActionCount, 0x00AEB990);
GAME_VAR(const char*, pn_keyActionType, 0x00AEB994);
GAME_VAR(const char*, pn_keyActionValue, 0x00AEB998);
GAME_VAR(const char*, pn_keyPathLoad, 0x00AEB9B0);

GAME_FN(void*, __cdecl, PN_Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, PN_Free, 0x005EEAB0, void* p);
GAME_FN(uint32_t, __cdecl, PN_HashName, 0x00576ED0, const char* name);
GAME_FN(void, __thiscall, PN_Chains_Free, 0x00447A70, NodeChains* self, int first);
GAME_FN(void, __thiscall, PN_PtrList_Grow, 0x006DB480, PtrList* self, uint32_t capacity, int);
GAME_FN(void*, __cdecl, PN_PathPool_NewThunk, 0x005EC820);
GAME_FN(int, __thiscall, PN_Scripts_Current, 0x005DB960, void* scripts);
GAME_FN(void, __thiscall, PN_Archive_LoadObject, 0x005B0720, void* archive, void* object, const char* key);
GAME_FN(void, __thiscall, PN_PathMgr_Register, 0x00472230, void* manager, Path* path, uintptr_t script);
GAME_FN(void, __thiscall, PN_PathMgr_Unregister, 0x004728A0, void* manager, Path* path);
GAME_FN(PnVec*, __cdecl, PN_VectorSub, 0x00414B70, PnVec* out, const PnVec* a, const PnVec* b);
GAME_FN(float, __thiscall, PN_Normalise, 0x00414E10, PnVec* v);
GAME_FN(float, __cdecl, PN_PitchOf, 0x00433490, const PnVec* v);
// this file's
GAME_FN(void, __thiscall, PN_Chains_Append, 0x005EC630, NodeChains* self, int first, int index);
GAME_FN(uint32_t, __thiscall, PN_Path_NodeCount, 0x005EC670, const Path* self);
GAME_FN(PathNode*, __thiscall, PN_NodePool_New, 0x005EC6B0, uint8_t* pool);
GAME_FN(void, __thiscall, PN_Node_FreeActions, 0x005EC7B0, PathNode* self);
GAME_FN(PathNode*, __thiscall, PN_Node_Construct, 0x005EC570, PathNode* self);
GAME_FN(PathNode*, __thiscall, PN_Path_NodeAt, 0x005EC8C0, const Path* self, int index);
GAME_FN(PathNode*, __thiscall, PN_Node_Copy, 0x005EC9D0, PathNode* self, const PathNode* other);
GAME_FN(PathNode*, __thiscall, PN_Path_AddNode, 0x005ECB20, Path* self, const PathNode* node);
GAME_FN(void, __thiscall, PN_Path_Clear, 0x005ECBA0, Path* self);
GAME_FN(Path*, __thiscall, PN_Path_Construct, 0x005EC5E0, Path* self, int scriptId);
GAME_FN(int, __thiscall, PN_FindPathIndexByName, 0x005ED310, void* self, const char* name);
GAME_FN(int, __stdcall, PN_FindPathIndexById, 0x005ED3D0, uint32_t id);
GAME_FN(PathReference*, __thiscall, PN_Ref_Construct, 0x005EDA30, PathReference* self, Path* path, PathManager* owner);
GAME_FN(float, __stdcall, PN_HeadingOf, 0x005ED6D0, const PnVec* v);
GAME_FN(void, __thiscall, PN_Ref_UpdateSegment, 0x005ED780, PathReference* self);
GAME_FN(void, __thiscall, PN_Ref_Reset, 0x005ED850, PathReference* self);

template <typename T>
T* PoolAt(uint8_t* pool, int index) {
    if ((*reinterpret_cast<re::Ptr32<uint8_t>*>(pool + 4))[index] & 0x80) {
        return nullptr;
    }
    return reinterpret_cast<T*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(pool) + *reinterpret_cast<int32_t*>(pool + 0xC) * index);
}
int PoolSize(uint8_t* pool) {
    return *reinterpret_cast<int32_t*>(pool + 8);
}
void PoolDelete(uint8_t* pool, void* object) {
    int index = int(static_cast<uint8_t*>(object) - *reinterpret_cast<re::Ptr32<uint8_t>*>(pool)) / *reinterpret_cast<int32_t*>(pool + 0xC);
    (*reinterpret_cast<re::Ptr32<uint8_t>*>(pool + 4))[index] |= 0x80;
    if (index < *reinterpret_cast<int32_t*>(pool + 0x14)) {
        *reinterpret_cast<int32_t*>(pool + 0x14) = index;
    }
}
PathNode* NodeOrNull(uint32_t index) {
    return PoolAt<PathNode>(pn_nodePool, int(index));
}
void PtrListPush(PtrList* list, void* item) {
    if (list->count == list->capacity) {
        uint32_t capacity = list->capacity == 0 ? 1 : list->capacity * 2;
        list->capacity = capacity;
        re::Ptr32<void>* data = nullptr;
        if (capacity != 0) {
            data = static_cast<re::Ptr32<void>*>(PN_Malloc(capacity > 0x3FFFFFFFu ? 0xFFFFFFFFu : capacity * 4));
        }
        if (list->data) {
            for (uint32_t i = 0; i < list->count; i++) {
                data[i] = list->data[i];
            }
            PN_Free(list->data);
        }
        list->data = data;
    }
    list->data[list->count] = item;
    list->count++;
}
// a path's nodes freed (Clear and the destructor)
void FreeNodes(Path* self) {
    uint32_t index = self->first;
    self->iterator = pn_chains->next[index];
    PathNode* node = NodeOrNull(index);
    while (node) {
        PathNode* next = nullptr;
        uint16_t it = self->iterator;
        if (it != 0xFFFF && it != 0xFFFE) {
            self->iterator = pn_chains->next[it];
            next = NodeOrNull(it);
        }
        PN_Node_FreeActions(node);
        PoolDelete(pn_nodePool, node);
        node = next;
    }
    PN_Chains_Free(pn_chains, self->first);
}

// 0x5EC5E0 (thiscall): the constructor
Path* __fastcall Path_Construct(Path* self, re::Edx, int scriptId) {
    self->flag4 = 0xFF;
    self->flag5 = 0xFF;
    self->flag7 = 0xFF;
    self->scriptId = scriptId;
    self->vtable = reinterpret_cast<re::Ptr32<void>*>(kPathVtable);
    self->first = 0xFFFF;
    self->iterator = 0xFFFF;
    return self;
}

// 0x5EC610 (thiscall): the name (hashed)
void __fastcall Path_SetName(Path* self, re::Edx, const char* name) {
    self->name = PN_HashName(name);
}

// 0x5EC630 (thiscall, on the chains): index appended to the chain from first
void __fastcall Chains_Append(NodeChains* self, re::Edx, int first, int index) {
    uint32_t i = uint16_t(first);
    uint16_t next = self->next[i];
    while (next != 0xFFFE) {
        i = next;
        next = self->next[i];
    }
    self->next[i] = uint16_t(index);
    self->next[index] = 0xFFFE;
    self->last = uint16_t(index);
}

// 0x5EC670 (thiscall): how many nodes
uint32_t __fastcall Path_NodeCount(const Path* self) {
    if (self->first == 0xFFFF) {
        return 0;
    }
    uint32_t count = 1;
    for (uint16_t i = pn_chains->next[self->first]; i != 0xFFFE; i = pn_chains->next[i]) {
        count++;
    }
    return count;
}

// 0x5EC6B0 (thiscall, on the node pool): a new node (the free search wraps once)
PathNode* __fastcall NodePool_New(uint8_t* self) {
    uint8_t* flags = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 4);
    int32_t& index = *reinterpret_cast<int32_t*>(self + 0x14);
    bool wrapped = false;
    while ((flags[index] & 0x80) == 0) {
        index++;
        if (index >= *reinterpret_cast<int32_t*>(self + 8)) {
            if (wrapped) {
                return nullptr;
            }
            index = 0;
            wrapped = true;
        }
    }
    flags[index] &= 0x7F;
    uint8_t& f = (*reinterpret_cast<re::Ptr32<uint8_t>*>(self + 4))[index];
    f = uint8_t(f ^ ((uint8_t(f + 1) ^ f) & 0x7F));
    uint32_t* node = reinterpret_cast<uint32_t*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(self)
                                                 + *reinterpret_cast<int32_t*>(self + 0xC) * index);
    for (int i = 0; i < 9; i++) {
        node[i] = 0;
    }
    return reinterpret_cast<PathNode*>(node);
}

// 0x5EC7B0 (thiscall): a node's actions freed
void __fastcall Node_FreeActions(PathNode* self) {
    PtrList* list = self->actions;
    if (!list) {
        return;
    }
    for (uint32_t i = 0; i < self->actions->count; i++) {
        void* action = self->actions->data[i];
        if (action) {
            PN_Free(action);
        }
    }
    list = self->actions;
    if (list) {
        if (list->data) {
            PN_Free(list->data);
            list->data = nullptr;
        }
        PN_Free(list);
    }
    self->actions = nullptr;
}

// 0x5EC820: a new path from the pool
void* __cdecl PathPool_New() {
    using Fn = void*(__thiscall*)(void*);
    return reinterpret_cast<Fn>(0x005EC730)(pn_pathPool);
}

// 0x5EC830 (thiscall): the first node (the iterator set after it)
PathNode* __fastcall Path_FirstNode(Path* self) {
    if (self->first == 0xFFFF) {
        return nullptr;
    }
    uint32_t index = self->first;
    self->iterator = pn_chains->next[index];
    return NodeOrNull(index);
}

// 0x5EC870 (thiscall): the iterator's node
PathNode* __fastcall Path_NextNode(Path* self) {
    uint16_t index = self->iterator;
    if (index == 0xFFFF || index == 0xFFFE) {
        return nullptr;
    }
    self->iterator = pn_chains->next[index];
    return NodeOrNull(index);
}

// 0x5EC8C0 (thiscall): the node at an index (the last when past the end)
PathNode* __fastcall Path_NodeAt(const Path* self, re::Edx, int index) {
    uint32_t i = self->first;
    uint16_t next = pn_chains->next[i];
    PathNode* node = NodeOrNull(i);
    for (int n = 0; next != 0xFFFE && n != index; n++) {
        i = next;
        next = pn_chains->next[i];
        node = NodeOrNull(i);
    }
    return node;
}

// 0x5EC940 (thiscall): the index of the node with an id (-1)
int __fastcall Path_IndexOfId(const Path* self, re::Edx, int id) {
    if (self->first == 0xFFFF) {
        return -1;
    }
    uint32_t i = self->first;
    uint16_t next = pn_chains->next[i];
    PathNode* node = NodeOrNull(i);
    if (node->id == id) {
        return 0;
    }
    int n = 0;
    while (next != 0xFFFE) {
        i = next;
        next = pn_chains->next[i];
        node = NodeOrNull(i);
        n++;
        if (node->id == id) {
            return n;
        }
    }
    return -1;
}

// 0x5EC9D0 (thiscall): a node copied (its actions duplicated); returns this
PathNode* __fastcall Node_Copy(PathNode* self, re::Edx, const PathNode* other) {
    if (self == other) {
        return self;
    }
    self->pos = other->pos;
    self->look = other->look;
    self->wait = other->wait;
    self->id = other->id;
    if (!other->actions || other->actions->count == 0) {
        return self;
    }
    PtrList* list = static_cast<PtrList*>(PN_Malloc(0xC));
    if (list) {
        list->data = nullptr;
        list->capacity = 0;
        list->count = 0;
    }
    self->actions = list;
    for (uint32_t i = 0; i < other->actions->count; i++) {
        PathAction* action = static_cast<PathAction*>(PN_Malloc(0x14));
        if (action) {
            *action = *static_cast<PathAction*>(other->actions->data[i]);
        }
        PtrList* l = self->actions;
        if (l->count == l->capacity) {
            PN_PtrList_Grow(l, l->capacity == 0 ? 1 : l->capacity * 2, 0);
        }
        l->data[l->count] = action;
        l->count++;
    }
    return self;
}

// 0x5ECAF0: a path back to its pool
void __cdecl PathPool_Delete(void* path) {
    PoolDelete(pn_pathPool, path);
}

// 0x5ECB20 (thiscall): a node added at the end (a copy)
PathNode* __fastcall Path_AddNode(Path* self, re::Edx, const PathNode* node) {
    PathNode* added = PN_NodePool_New(pn_nodePool);
    added = added ? PN_Node_Construct(added) : nullptr;
    PN_Node_Copy(added, node);
    int index = int(reinterpret_cast<uint8_t*>(added) - *re::PtrCast<re::Ptr32<uint8_t>*>(pn_nodePool))
                / *reinterpret_cast<int32_t*>(pn_nodePool + 0xC);
    if (self->first == 0xFFFF) {
        pn_chains->next[index] = 0xFFFE;
        pn_chains->last = uint16_t(index);
        self->first = uint16_t(index);
        return added;
    }
    PN_Chains_Append(pn_chains, self->first, index);
    return added;
}

// 0x5ECBA0 (thiscall): the nodes freed
void __fastcall Path_Clear(Path* self) {
    if (self->first == 0xFFFF) {
        return;
    }
    FreeNodes(self);
    self->first = 0xFFFF;
}

// 0x5ECC80 (thiscall, vf1): loaded from an archive: the name, two flags, the nodes (a
// position, a look, a wait, an id, the actions)
void __fastcall Path_Load(Path* self, re::Edx, void* archive) {
    PN_Path_Clear(self);
    self->datSet = pn_datSet;
    re::Ptr32<void>* vt = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(archive);
    using ReadString = void(__thiscall*)(void*, void*, int, const char*);
    using ReadInt = uint8_t(__thiscall*)(void*, int*, const char*);
    using ReadVector = void(__thiscall*)(void*, void*, const char*);
    using ReadFloat = void(__thiscall*)(void*, float*, const char*);
    struct {
        re::Ptr32<char> text;
        int capacity;
        char buffer[0x24];
    } name;
    name.text = name.buffer;
    name.capacity = 0x21;
    name.buffer[0] = 0;
    re::PtrCast<ReadString>(vt[0x24 / 4])(archive, &name, 0x20, pn_keyName);
    self->name = PN_HashName(name.text);
    int value;
    re::PtrCast<ReadInt>(vt[0x38 / 4])(archive, &value, pn_keyFlag4);
    self->flag4 = uint8_t(value);
    re::PtrCast<ReadInt>(vt[0x38 / 4])(archive, &value, pn_keyFlag5);
    self->flag5 = uint8_t(value);
    int nodes = 0;
    int count = re::PtrCast<ReadInt>(vt[0x38 / 4])(archive, &nodes, pn_keyNodeCount) ? nodes : 0;
    // the node's id slot isn't set per node: it keeps the previous node's (the first
    // time it's uninitialised stack; zero here)
    int32_t id = 0;
    for (int n = 0; n < count; n++) {
        PathNode node = {};
        node.id = id;
        node.actions = nullptr;
        re::PtrCast<ReadVector>(vt[0x20 / 4])(archive, &node.pos, pn_keyPos);
        re::PtrCast<ReadVector>(vt[0x20 / 4])(archive, &node.look, pn_keyLook);
        re::PtrCast<ReadFloat>(vt[0x28 / 4])(archive, &node.wait, pn_keyWait);
        re::PtrCast<ReadInt>(vt[0x38 / 4])(archive, &node.id, pn_keyId);
        id = node.id;
        int actions = 0;
        int actionCount = re::PtrCast<ReadInt>(vt[0x38 / 4])(archive, &actions, pn_keyActionCount) ? actions : 0;
        PtrList* list = nullptr;
        if (actionCount > 0) {
            list = static_cast<PtrList*>(PN_Malloc(0xC));
            if (list) {
                list->data = nullptr;
                list->capacity = 0;
                list->count = 0;
            }
            for (int a = actionCount; a != 0; a--) {
                PathAction* action = static_cast<PathAction*>(PN_Malloc(0x14));
                if (action) {
                    *action = {0, {0.0f, 0.0f, 0.0f}, 0.0f};
                }
                re::PtrCast<ReadInt>(vt[0x38 / 4])(archive, &action->type, pn_keyActionType);
                re::PtrCast<ReadVector>(vt[0x20 / 4])(archive, &action->value, pn_keyActionValue);
                re::PtrCast<ReadFloat>(vt[0x28 / 4])(archive, &action->time, pn_keyWait);
                PtrListPush(list, action);
            }
        }
        PathNode* added = PN_Path_AddNode(self, &node);
        added->actions = list;
        PN_Node_FreeActions(&node);
    }
}

// 0x5ECF80 (thiscall): the destructor
void __fastcall Path_Destruct(Path* self) {
    self->vtable = reinterpret_cast<re::Ptr32<void>*>(kPathVtable);
    if (self->first != 0xFFFF) {
        FreeNodes(self);
    }
    self->vtable = reinterpret_cast<re::Ptr32<void>*>(kArchivableVtable);
}

// 0x5ED060: `count` paths loaded from an archive for the current script (the path
// manager told of each)
void __fastcall LoadPaths(void*, re::Edx, uint8_t* archive, int count) {
    int script = PN_Scripts_Current(pn_scripts);
    int scriptId = -2;
    if (script) {
        scriptId = *reinterpret_cast<int32_t*>(script + 0x44);
    }
    archive[4] = 1;
    for (; count > 0; count--) {
        void* memory = PN_PathPool_NewThunk();
        Path* path = memory ? PN_Path_Construct(static_cast<Path*>(memory), scriptId) : nullptr;
        PN_Archive_LoadObject(archive, path, pn_keyPathLoad);
        PN_PathMgr_Register(pn_pathMgr, path, script);
    }
}

// 0x5ED0E0: the path pool's size
int __cdecl PathPool_Size() {
    return PoolSize(pn_pathPool);
}

// 0x5ED0F0 (thiscall, vf1): a reference forgotten
void __fastcall Manager_Remove(PathManager* self, re::Edx, void* ref) {
    for (uint32_t i = 0; i < self->count; i++) {
        if (self->refs[i] == ref) {
            if (int(i) >= 0) {
                self->refs[i] = nullptr;
            }
            return;
        }
    }
}

// 0x5ED130 (thiscall, vf2): the scalar deleting destructor
PathManager* __fastcall Manager_Delete(PathManager* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<re::Ptr32<void>*>(kPathManagerVtable);
    if (self->refs) {
        PN_Free(self->refs);
        self->refs = nullptr;
    }
    if (flags & 1) {
        PN_Free(self);
    }
    return self;
}

// the references to a path cleared (the first one found)
void ForgetReferences(PathManager* self, Path* path) {
    for (uint32_t i = 0; i < self->count; i++) {
        PathReference* ref = static_cast<PathReference*>(self->refs[i]);
        if (ref && ref->path == path) {
            static_cast<PathReference*>(self->refs[i])->path = nullptr;
            self->refs[i] = nullptr;
            return;
        }
    }
}

// 0x5ED170 (thiscall): the paths of a data set (of the current script, or all) removed
void __fastcall Manager_RemovePathsOfSet(PathManager* self, re::Edx, int set, int anyScript) {
    int script = PN_Scripts_Current(pn_scripts);
    int scriptId = -2;
    if (script) {
        scriptId = *reinterpret_cast<int32_t*>(script + 0x44);
    }
    int size = PoolSize(pn_pathPool);
    for (int i = 1; i < size; i++) {
        Path* path = PoolAt<Path>(pn_pathPool, i);
        if (!path || int8_t(path->datSet) != set) {
            continue;
        }
        if (path->scriptId != scriptId && uint8_t(anyScript) == 0) {
            continue;
        }
        ForgetReferences(self, path);
        PN_Path_Clear(path);
        PN_PathMgr_Unregister(pn_pathMgr, path);
        using Fn = void(__thiscall*)(void*, int);
        re::PtrCast<Fn>(path->vtable[0])(path, 1);
    }
}

// 0x5ED260 (thiscall): the paths with a +7 byte removed
void __fastcall Manager_RemovePathsOfKind(PathManager* self, re::Edx, int kind) {
    int size = PoolSize(pn_pathPool);
    for (int i = 1; i < size; i++) {
        Path* path = PoolAt<Path>(pn_pathPool, i);
        if (!path || int8_t(path->flag7) != kind) {
            continue;
        }
        ForgetReferences(self, path);
        PN_Path_Clear(path);
        using Fn = void(__thiscall*)(void*, int);
        re::PtrCast<Fn>(path->vtable[0])(path, 1);
    }
}

// 0x5ED310: a path's index by name (-1)
int __fastcall FindPathIndexByName(void*, re::Edx, const char* name) {
    uint32_t hash = PN_HashName(name);
    for (int i = 1; i < PoolSize(pn_pathPool); i++) {
        Path* path = PoolAt<Path>(pn_pathPool, i);
        if (path && path->name == hash) {
            return i;
        }
    }
    return -1;
}

// 0x5ED360: a path by name (the pool's flags read at -1 when there's none)
Path* __fastcall FindPathByName(void*, re::Edx, const char* name) {
    if (!name) {
        return nullptr;
    }
    int index = PN_FindPathIndexByName(nullptr, name);
    if (index == 0) {
        return nullptr;
    }
    return PoolAt<Path>(pn_pathPool, index);
}

// 0x5ED3A0: a path by index
Path* __fastcall GetPath(void*, re::Edx, int index) {
    if (index < 0 || index >= PoolSize(pn_pathPool)) {
        return nullptr;
    }
    return PoolAt<Path>(pn_pathPool, index);
}

// 0x5ED3D0: a path's index by its hashed name (-1)
int __stdcall FindPathIndexById(uint32_t id) {
    for (int i = 1; i < PoolSize(pn_pathPool); i++) {
        Path* path = PoolAt<Path>(pn_pathPool, i);
        if (path && path->name == id) {
            return i;
        }
    }
    return -1;
}

// 0x5ED420 (thiscall, vf0): a reference added (in a free slot, else at the end)
void __fastcall Manager_Add(PathManager* self, re::Edx, void* ref) {
    uint32_t count = self->count;
    for (uint32_t i = 0; i < count; i++) {
        if (self->refs[i] == ref) {
            break;
        }
    }
    for (uint32_t i = 0; i < count; i++) {
        if (self->refs[i] == nullptr) {
            self->refs[i] = ref;
            return;
        }
    }
    PtrListPush(reinterpret_cast<PtrList*>(&self->refs), ref);
}

// 0x5ED4F0 (thiscall): a new reference to a path (by index)
PathReference* __fastcall Manager_CreateReference(PathManager* self, re::Edx, int index) {
    if (index < 0 || index >= PoolSize(pn_pathPool)) {
        return nullptr;
    }
    Path* path = PoolAt<Path>(pn_pathPool, index);
    if (!path) {
        return nullptr;
    }
    PathReference* ref = static_cast<PathReference*>(PN_Malloc(0x34));
    if (!ref) {
        return nullptr;
    }
    return PN_Ref_Construct(ref, path, self);
}

// 0x5ED540 (thiscall): the manager's constructor
PathManager* __fastcall Manager_Construct(PathManager* self) {
    self->vtable = reinterpret_cast<re::Ptr32<void>*>(kPathManagerVtable);
    self->refs = nullptr;
    self->capacity = 0;
    self->count = 0;
    return self;
}

// 0x5ED560 (thiscall): the speed (only on a path of two nodes or more)
uint8_t __fastcall Ref_SetSpeed(PathReference* self, re::Edx, float speed) {
    if (self->path && PN_Path_NodeCount(self->path) >= 2) {
        self->speed = speed;
        return 1;
    }
    return 0;
}

// 0x5ED590 (thiscall): the path's index
int __fastcall Ref_PathIndex(PathReference* self) {
    // (the game passes the patrol paths at 0xD13CB4 in ecx, which the callee ignores)
    return PN_FindPathIndexById(self->path->name);
}

// 0x5ED5B0 (thiscall): the node last passed (the last one at the end)
int __fastcall Ref_NodeIndex(PathReference* self) {
    if (self->atEnd) {
        return int(PN_Path_NodeCount(self->path)) - 1;
    }
    return int(self->segment) - 1;
}

// 0x5ED5D0 (thiscall): where along the path
void __fastcall Ref_Position(PathReference* self, re::Edx, PnVec* out) {
    if (!self->path || PN_Path_NodeCount(self->path) < 2) {
        return;
    }
    *out = {0.0f, 0.0f, 0.0f};
    float t = self->along;
    PnVec step = {self->dir.x * t, self->dir.y * t, t * self->dir.z};
    PathNode* node = PN_Path_NodeAt(self->path, int(self->segment) - 1);
    out->x = node->pos.x + step.x;
    out->y = node->pos.y + step.y;
    out->z = node->pos.z + step.z;
}

// 0x5ED680 (thiscall): the orientation (0, pitch, heading)
void __fastcall Ref_Orientation(PathReference* self, re::Edx, float* out) {
    if (!self->path || PN_Path_NodeCount(self->path) < 2) {
        return;
    }
    float pitch = self->pitch;
    float heading = self->heading;
    out[0] = 0.0f;
    out[1] = pitch;
    out[2] = heading;
}

// 0x5ED6D0: a direction's heading (from its x and y: atan of y / x, by quadrant)
float __stdcall HeadingOf(const PnVec* v) {
    if (0.0f == v->x) {
        if (0.0f > v->y || std::isnan(v->y)) {
            return -1.5707963705062866f;
        }
        return 1.5707963705062866f;
    }
    float a = float(std::atan(double(float(v->y / v->x))));
    bool xPositive = 0.0f < v->x;
    bool yNegative = 0.0f > v->y || std::isnan(v->y);
    if (xPositive) {
        return yNegative ? -a : a;
    }
    if (yNegative) {
        return float(a - 3.1415927410125732);
    }
    return float(3.1415927410125732 - a);
}

// 0x5ED780 (thiscall): the segment's direction, length, heading and pitch
void __fastcall Ref_UpdateSegment(PathReference* self) {
    if (!self->path || PN_Path_NodeCount(self->path) < 2) {
        return;
    }
    PathNode* from = PN_Path_NodeAt(self->path, int(self->segment) - 1);
    PathNode* to = PN_Path_NodeAt(self->path, int(self->segment));
    PnVec d;
    self->dir = *PN_VectorSub(&d, &to->pos, &from->pos);
    PnVec& v = self->dir;
    float lengthSq = float(double(v.y) * v.y + double(v.x) * v.x + double(v.z) * v.z);
    self->length = float(std::sqrt(double(std::fabs(lengthSq))));
    PN_Normalise(&self->dir);
    self->heading = PN_HeadingOf(&self->dir);
    self->pitch = PN_PitchOf(&self->dir);
}

// 0x5ED850 (thiscall): back to the path's start
void __fastcall Ref_Reset(PathReference* self) {
    self->speed = 0.0f;
    self->along = 0.0f;
    self->atEnd = 0;
    if (self->path && PN_Path_NodeCount(self->path) >= 2) {
        self->segment = 1;
        PN_Ref_UpdateSegment(self);
        return;
    }
    self->segment = 0;
    self->dir = {0.0f, 0.0f, 0.0f};
    self->heading = 0.0f;
    self->pitch = 0.0f;
    self->length = 0.0f;
}

// 0x5ED8B0 (thiscall): moved along by speed * step: reached says a node was passed (or
// an end reached, where it stops)
void __fastcall Ref_Advance(PathReference* self, re::Edx, float step, uint8_t* reached) {
    *reached = 0;
    if (!self->path || PN_Path_NodeCount(self->path) < 2) {
        return;
    }
    if (self->atEnd && 0.0f == self->speed) {
        return;
    }
    if (0.0f < self->speed) {
        if (!(self->length > self->along)) {
            return;
        }
        float v = float(self->speed * step);
        v = v + self->along;
        if (self->length < v) {
            uint32_t segment = self->segment;
            if (segment < PN_Path_NodeCount(self->path) - 1) {
                v = v - self->length;
                self->segment = segment + 1;
                self->atEnd = 0;
            } else {
                v = self->length;
                self->atEnd = 1;
                self->speed = 0.0f;
            }
            PN_Ref_UpdateSegment(self);
            *reached = 1;
        }
        self->along = v;
        return;
    }
    if (!(0.0f > self->speed) || !(0.0f < self->along)) {
        return;
    }
    float v = float(self->speed * step);
    v = v + self->along;
    if (!(0.0f > v)) {
        self->along = v;
        return;
    }
    uint32_t segment = self->segment;
    if (segment > 1) {
        self->segment = segment - 1;
        PN_Ref_UpdateSegment(self);
        v = self->length + v;
        self->atEnd = 0;
        *reached = 1;
        self->along = v;
        return;
    }
    if (segment == 1) {
        PN_Ref_UpdateSegment(self);
        self->speed = 0.0f;
        self->atEnd = 1;
        v = 0.0f;
    }
    *reached = 1;
    self->along = v;
}

// 0x5EDA30 (thiscall): the constructor (the owner told of it)
PathReference* __fastcall Ref_Construct(PathReference* self, re::Edx, Path* path, PathManager* owner) {
    self->path = path;
    PathManager* o = path ? owner : nullptr;
    self->vtable = reinterpret_cast<re::Ptr32<void>*>(kPathReferenceVtable);
    self->owner = o;
    if (o) {
        using Fn = void(__thiscall*)(void*, void*);
        re::PtrCast<Fn>(o->vtable[0])(o, self);
    }
    self->dir = {0.0f, 0.0f, 0.0f};
    PN_Ref_Reset(self);
    return self;
}

#define PATHNET_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(PathNetOriginal_, __LINE__);                                        \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(PathNetOriginal_, __LINE__), \
             re::Mode::Original)

PATHNET_REVERSED("Path::Construct", 0x005EC5E0, &Path_Construct);
PATHNET_REVERSED("Path::SetName", 0x005EC610, &Path_SetName);
PATHNET_REVERSED("PathNodeChains::Append", 0x005EC630, &Chains_Append);
PATHNET_REVERSED("Path::NodeCount", 0x005EC670, &Path_NodeCount);
PATHNET_REVERSED("CPool<PathNode>::New", 0x005EC6B0, &NodePool_New);
PATHNET_REVERSED("PathNode::FreeActions", 0x005EC7B0, &Node_FreeActions);
PATHNET_REVERSED("Path::PoolNew", 0x005EC820, &PathPool_New);
PATHNET_REVERSED("Path::FirstNode", 0x005EC830, &Path_FirstNode);
PATHNET_REVERSED("Path::NextNode", 0x005EC870, &Path_NextNode);
PATHNET_REVERSED("Path::NodeAt", 0x005EC8C0, &Path_NodeAt);
PATHNET_REVERSED("Path::IndexOfId", 0x005EC940, &Path_IndexOfId);
PATHNET_REVERSED("PathNode::Copy", 0x005EC9D0, &Node_Copy);
PATHNET_REVERSED("Path::PoolDelete", 0x005ECAF0, &PathPool_Delete);
PATHNET_REVERSED("Path::AddNode", 0x005ECB20, &Path_AddNode);
PATHNET_REVERSED("Path::Clear", 0x005ECBA0, &Path_Clear);
PATHNET_REVERSED("Path::Load", 0x005ECC80, &Path_Load);
PATHNET_REVERSED("Path::Destruct", 0x005ECF80, &Path_Destruct);
PATHNET_REVERSED("PathManager::LoadPaths", 0x005ED060, &LoadPaths);
PATHNET_REVERSED("PathManager::PathPoolSize", 0x005ED0E0, &PathPool_Size);
PATHNET_REVERSED("PathManager::Remove", 0x005ED0F0, &Manager_Remove);
PATHNET_REVERSED("PathManager::Delete", 0x005ED130, &Manager_Delete);
PATHNET_REVERSED("PathManager::RemovePathsOfSet", 0x005ED170, &Manager_RemovePathsOfSet);
PATHNET_REVERSED("PathManager::RemovePathsOfKind", 0x005ED260, &Manager_RemovePathsOfKind);
PATHNET_REVERSED("PathManager::FindPathIndexByName", 0x005ED310, &FindPathIndexByName);
PATHNET_REVERSED("PathManager::FindPathByName", 0x005ED360, &FindPathByName);
PATHNET_REVERSED("PathManager::GetPath", 0x005ED3A0, &GetPath);
PATHNET_REVERSED("PathManager::FindPathIndexById", 0x005ED3D0, &FindPathIndexById);
PATHNET_REVERSED("PathManager::Add", 0x005ED420, &Manager_Add);
PATHNET_REVERSED("PathManager::CreateReference", 0x005ED4F0, &Manager_CreateReference);
PATHNET_REVERSED("PathManager::Construct", 0x005ED540, &Manager_Construct);
PATHNET_REVERSED("PathReference::SetSpeed", 0x005ED560, &Ref_SetSpeed);
PATHNET_REVERSED("PathReference::PathIndex", 0x005ED590, &Ref_PathIndex);
PATHNET_REVERSED("PathReference::NodeIndex", 0x005ED5B0, &Ref_NodeIndex);
PATHNET_REVERSED("PathReference::Position", 0x005ED5D0, &Ref_Position);
PATHNET_REVERSED("PathReference::Orientation", 0x005ED680, &Ref_Orientation);
PATHNET_REVERSED("PathReference::HeadingOf", 0x005ED6D0, &HeadingOf);
PATHNET_REVERSED("PathReference::UpdateSegment", 0x005ED780, &Ref_UpdateSegment);
PATHNET_REVERSED("PathReference::Reset", 0x005ED850, &Ref_Reset);
PATHNET_REVERSED("PathReference::Advance", 0x005ED8B0, &Ref_Advance);
PATHNET_REVERSED("PathReference::Construct", 0x005EDA30, &Ref_Construct);

} // namespace
