// The action tree's node classes: ActionNode (+4 parent, +8 name hash) and its
// Implementation (+0xC conditions), Bank (+0x10 children), Playable (+0x10 / +0x14 its
// tracks' descriptors), PlayableImplementation (+0x18 children), Reference (+0xC the
// node it stands for, its virtuals forwarded there), FileReference (a file's tree
// loaded under it) and MultiParentReference (+0x30 a parent per key). Their virtuals,
// constructors and destructors, the small vector of children and the weights.
// 0x5F4E00..0x5F6CC0.

#include <cstdint>

#include "game/ActionTree.h"

#include "game/Crt.h"
#include "reversed.h"
using namespace acttree;

namespace {

// vtables
constexpr uint32_t kVtNode = 0x0092D9EC;
constexpr uint32_t kVtImplementation = 0x0092DA44;
constexpr uint32_t kVtReference = 0x0092DAAC;
constexpr uint32_t kVtBank = 0x0092DB34;
constexpr uint32_t kVtPlayable = 0x0092DB9C;
constexpr uint32_t kVtFileReference = 0x0092DC04;
constexpr uint32_t kVtBankCompressed = 0x0092DC74;
constexpr uint32_t kVtMultiParent = 0x0092DCDC;

// the children: the first inline, the rest in an array
struct PtrVector {
    re::Ptr32<uint8_t> first;         // +0
    re::Ptr32<re::Ptr32<uint8_t>> rest;         // +4
    uint32_t restCapacity;  // +8
    uint32_t count;         // +0xC
};
static_assert(sizeof(PtrVector) == 0x10);
re::Ptr32<uint8_t>* Elem(PtrVector* v, uint32_t i) {
    return i < 1 ? &v->first + i : &v->rest[i - 1];
}

// MultiParentReference's {key, parent} pairs
struct ParentPair {   // (the game's layout: 8)
    int32_t key;
    re::Ptr32<uint8_t> parent;
};
static_assert(sizeof(ParentPair) == 8);
struct PairArray {
    re::Ptr32<ParentPair> data;   // +0 (+0x30 in the node)
    uint32_t capacity;  // +4
    int32_t count;      // +8
};

GAME_VAR(uint32_t, actionNodeHash, 0x01B649E8);   // "ActionNode": never a child's name
GAME_VAR(uint32_t, unresolvedName, 0x01B64A14);   // "UNRESOLVED_NODE" (made once)
GAME_VAR(uint32_t, unresolvedNameGuard, 0x01B64A18);
GAME_VAR(uint32_t, fileReferenceIdGuard, 0x00CFB378);
GAME_VAR(int16_t, fileReferenceId, 0x00CFB374);
inline void* const fileCache_D02850 = reinterpret_cast<void*>(0x00D02850);

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, size_t size, int count, void* ctor);
inline int __cdecl AtExit(void* fn) { return crt::Atexit((void(__cdecl*)())fn); }
GAME_FN(uint32_t*, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(void, __thiscall, NameKey_Destruct, 0x0049A650, uint32_t* key);
GAME_FN(uint32_t*, __thiscall, NameKey_Default, 0x0066BEA0, uint32_t* key);
GAME_FN(uint32_t*, __thiscall, NameKey_FromHash, 0x005FA880, uint32_t* key, const uint32_t* from);
GAME_FN(void*, __thiscall, TrackDesc_GetKey, 0x005FA9B0, uint8_t* desc, uint32_t* out);
GAME_FN(void, __thiscall, TrackDesc_Apply, 0x005FA940, uint8_t* desc, uint8_t* track);
GAME_FN(uint8_t*, __thiscall, TrackRegistry_Create, 0x0061A610, void* registry, uint32_t* key);
inline void* const trackRegistry_1B6565C = reinterpret_cast<void*>(0x01B6565C);
GAME_FN(uint8_t*, __cdecl, ActionTree_Find, 0x005F5B10, const char* path, uint8_t* from);
GAME_FN(int32_t, __thiscall, Node_GetRootKey, 0x005F4FA0, uint8_t* node);
GAME_FN(uint8_t, __cdecl, Implementation_IsA, 0x005F5FE0, int id);
GAME_FN(uint8_t, __cdecl, Reference_IsA, 0x005F6320, int id);
GAME_FN(uint8_t*, __thiscall, Node_FindFirstPlayable, 0x005F5120, uint8_t* node);
GAME_FN(float, __thiscall, Node_GetWeight, 0x005F5250, uint8_t* node);
GAME_FN(float, __thiscall, Node_AccumulateWeight, 0x005F52D0, uint8_t* node, float acc, float total);
GAME_FN(float, __thiscall, Weighted_Accumulate, 0x005FC7E0, uint8_t* condition, float acc, float total);
GAME_FN(uint8_t, __thiscall, Conditions_Evaluate, 0x005FBB10, uint8_t* conditions, uint8_t* owner);
GAME_FN(uint8_t*, __thiscall, Conditions_Construct, 0x005FBAD0, uint8_t* conditions);
GAME_FN(void, __thiscall, Conditions_Destruct, 0x005FBAE0, uint8_t* conditions);
GAME_FN(void, __thiscall, Conditions_Reserve, 0x005FBD30, uint8_t* conditions, uint32_t count);
GAME_FN(void, __thiscall, Conditions_Add, 0x005FBD50, uint8_t* conditions, uint8_t* condition);
GAME_FN(void, __thiscall, Vector_RemoveAt, 0x005F57B0, PtrVector* v, uint32_t index);
GAME_FN(uint8_t, __thiscall, Vector_Remove, 0x005F5E20, PtrVector* v, uint8_t* value);
GAME_FN(uint8_t*, __thiscall, Vector_FindChild, 0x005F5E60, PtrVector* v, const uint32_t* name);
GAME_FN(void, __thiscall, Vector_DeleteAll, 0x005F5EB0, PtrVector* v);
GAME_FN(void, __thiscall, Vector_PushBack, 0x005F6740, PtrVector* v, uint8_t* value);
GAME_FN(void, __thiscall, Vector_ReserveRest, 0x005FBA30, PtrVector* v, uint32_t count);
GAME_FN(uint32_t, __cdecl, Vector_GrowCapacity, 0x005F4FE0, uint32_t capacity);
GAME_FN(void, __thiscall, Pairs_Reserve, 0x005F5810, PairArray* pairs, uint32_t count);
GAME_FN(void, __thiscall, MultiParent_RemoveParent, 0x005F6450, uint8_t* node, uint8_t* parent);
GAME_FN(void, __thiscall, MultiParent_AddParent, 0x005F6960, uint8_t* node, uint8_t* parent, int32_t key);
GAME_FN(void, __thiscall, Node_ResolveChildren, 0x005F5210, uint8_t* node);
GAME_FN(void, __thiscall, Implementation_Destruct, 0x005F5DC0, uint8_t* node);
GAME_FN(void, __thiscall, Playable_Destruct, 0x005F60A0, uint8_t* node);
GAME_FN(void, __thiscall, Bank_Destruct, 0x005F64B0, uint8_t* node);
GAME_FN(void, __thiscall, FileReference_Release, 0x005F6670, uint8_t* node);
GAME_FN(void, __thiscall, FileReference_Destruct, 0x005F67E0, uint8_t* node);
GAME_FN(void, __thiscall, MultiParent_Destruct, 0x005F68F0, uint8_t* node);
GAME_FN(void, __thiscall, FileNames_Destruct, 0x005F5930, uint8_t* names);
GAME_FN(void, __thiscall, CompressedData_Destruct, 0x005FAD30, uint8_t* data);
GAME_FN(void, __thiscall, FileCache_Remove, 0x005DBF80, void* cache, uint8_t* node);

void SetVtable(uint8_t* self, uint32_t vt) {
    vc::At<uint32_t>(self, 0) = vt;
}
re::Ptr32<uint8_t>& Parent(uint8_t* self) {
    return vc::At<uint8_t*>(self, 4);
}
uint32_t* Name(uint8_t* self) {
    return &vc::At<uint32_t>(self, 8);
}
re::Ptr32<uint8_t>& Conditions(uint8_t* self) {
    return vc::At<uint8_t*>(self, 0xC);
}
re::Ptr32<uint8_t>& Target(uint8_t* self) {
    return vc::At<uint8_t*>(self, 0xC);
}
PtrVector* BankChildren(uint8_t* self) {
    return reinterpret_cast<PtrVector*>(self + 0x10);
}
PtrVector* PlayableChildren(uint8_t* self) {
    return reinterpret_cast<PtrVector*>(self + 0x18);
}
PairArray* Pairs(uint8_t* self) {
    return reinterpret_cast<PairArray*>(self + 0x30);
}

// 0x5F4E00 (thiscall): ActionNode::IsA
uint8_t __fastcall Node_IsA(uint8_t*, re::Edx, int id) {
    return int16_t(id) == NodeId();
}

// 0x5F4E40 (thiscall): ActionNode::GetParent (the key unused)
uint8_t* __fastcall Node_GetParent(uint8_t* self, re::Edx, int32_t) {
    return Parent(self);
}

// 0x5F4E50 (thiscall): ActionNode's destructor
void __fastcall Node_Destruct(uint8_t* self, re::Edx) {
    SetVtable(self, kVtNode);
}

// 0x5F4F90 (thiscall): the node at +0xC (the argument unused)
uint8_t* __fastcall Node_Get0C(uint8_t* self, re::Edx, int32_t) {
    return vc::At<uint8_t*>(self, 0xC);
}

// 0x5F4FA0 (thiscall): the name of the ancestor right under the root (-1: none)
int32_t __fastcall Node_GetRootKeyImpl(uint8_t* self, re::Edx) {
    for (uint8_t* node = self; node;) {
        if (root == vc::Call<uint8_t*>(node, kNodeGetParent, -1)) {
            return int32_t(*vc::Call<uint32_t*>(node, kNodeGetName));
        }
        node = vc::Call<uint8_t*>(node, kNodeGetParent, -1);
    }
    return -1;
}

// 0x5F4FE0 (cdecl): a vector's next capacity
uint32_t __cdecl GrowCapacity(uint32_t capacity) {
    if (capacity < 3) return 3;
    if (capacity < 6) return 6;
    if (capacity < 0xC) return 0xC;
    if (capacity < 0x1E) return 0x1E;
    if (capacity < 0x46) return 0x46;
    if (capacity < 0x7D) return 0x7D;
    if (capacity < 200) return 200;
    return capacity * 2;
}

// 0x5F5040 (thiscall): a leaf's AddChild: only the child's parent set
void __fastcall Implementation_AddChild(uint8_t* self, re::Edx, uint8_t* child) {
    Parent(child) = self;
}

// 0x5F5050 (thiscall): a leaf's FindChild: itself by name
uint8_t* __fastcall Implementation_FindChild(uint8_t* self, re::Edx, const uint32_t* name) {
    if (*name == actionNodeHash) {
        return nullptr;
    }
    return *name == *vc::Call<uint32_t*>(self, kNodeGetName) ? self : nullptr;
}

// 0x5F5090 (thiscall): an ancestor (under `key`) has the name
uint8_t __fastcall Node_HasAncestorNamed(uint8_t* self, re::Edx, const uint32_t* name, int32_t key) {
    for (uint8_t* node = vc::Call<uint8_t*>(self, kNodeGetParent, key); node; node = vc::Call<uint8_t*>(node, kNodeGetParent, key)) {
        if (*name == *vc::Call<uint32_t*>(node, kNodeGetName)) {
            return 1;
        }
    }
    return 0;
}

// 0x5F50E0 (thiscall): `ancestor` is one (under `key`)
uint8_t __fastcall Node_HasAncestor(uint8_t* self, re::Edx, uint8_t* ancestor, int32_t key) {
    for (uint8_t* node = vc::Call<uint8_t*>(self, kNodeGetParent, key); node; node = vc::Call<uint8_t*>(node, kNodeGetParent, key)) {
        if (node == ancestor) {
            return 1;
        }
    }
    return 0;
}

// 0x5F5120 (thiscall): the first playable node in the subtree (nullptr: the game calls
// through it)
uint8_t* __fastcall Node_FindFirstPlayableImpl(uint8_t* self, re::Edx) {
    if (self && vc::Call<uint8_t>(self, kNodeIsA, int(uint16_t(PlayableId())))) {
        return self;
    }
    int count = vc::Call<int>(self, kNodeNumChildren);
    for (int i = 0; i < count; i++) {
        uint8_t* found = Node_FindFirstPlayable(vc::Call<uint8_t*>(self, kNodeGetChild, i));
        if (found) {
            return found;
        }
    }
    return nullptr;
}

// 0x5F51B0 (cdecl): the root's children resolved, each under its own name as the key
void __cdecl ResolveAll() {
    int count = vc::Call<int>(root, kNodeNumChildren);
    for (int i = 0; i < count; i++) {
        uint8_t* child = vc::Call<uint8_t*>(root, kNodeGetChild, i);
        currentKey = int32_t(*vc::Call<uint32_t*>(child, kNodeGetName));
        vc::Call<void>(child, kNodeResolve);
    }
    currentKey = -1;
}

// 0x5F5210 (thiscall): ActionNode::Resolve: the children's
void __fastcall Node_ResolveChildrenImpl(uint8_t* self, re::Edx) {
    int count = vc::Call<int>(self, kNodeNumChildren);
    for (int i = 0; i < count; i++) {
        vc::Call<void>(vc::Call<uint8_t*>(self, kNodeGetChild, i), kNodeResolve);
    }
}

// 0x5F5250 (thiscall): the weight of its first weighted-random condition (0: none)
float __fastcall Node_GetWeightImpl(uint8_t* self, re::Edx) {
    int count = vc::Call<int>(self, kNodeNumConditions);
    for (int i = 0; i < count; i++) {
        uint8_t* condition = vc::Call<uint8_t*>(self, kNodeGetCondition, i);
        if (condition && vc::Call<uint8_t>(condition, 0, int(uint16_t(WeightedId())))) {
            return vc::At<float>(condition, 4);
        }
    }
    return 0.0f;
}

// 0x5F52D0 (thiscall): the running total through its weighted-random condition
float __fastcall Node_AccumulateWeightImpl(uint8_t* self, re::Edx, float acc, float total) {
    int count = vc::Call<int>(self, kNodeNumConditions);
    for (int i = 0; i < count; i++) {
        uint8_t* condition = vc::Call<uint8_t*>(self, kNodeGetCondition, i);
        if (condition && vc::Call<uint8_t>(condition, 0, int(uint16_t(WeightedId())))) {
            return Weighted_Accumulate(condition, acc, total);
        }
    }
    return acc;
}

// 0x5F5370 (thiscall) / 0x5F5FE0 (cdecl): ActionNodeImplementation::IsA
uint8_t __fastcall Implementation_IsAImpl(uint8_t*, re::Edx, int id) {
    if (int16_t(id) == ImplementationId()) {
        return 1;
    }
    return int16_t(id) == NodeId();
}
uint8_t __cdecl Implementation_IsAStatic(int id) {
    if (int16_t(id) == ImplementationId()) {
        return 1;
    }
    return int16_t(id) == NodeId();
}

// 0x5F53E0 (thiscall): ActionNodeReference's constructor
uint8_t* __fastcall Reference_Construct(uint8_t* self, re::Edx) {
    Parent(self) = nullptr;
    SetVtable(self, kVtReference);
    NameKey_Default(Name(self));
    Target(self) = nullptr;
    return self;
}

// 0x5F5410 (thiscall) / 0x5F6320 (cdecl): ActionNodeReference::IsA
uint8_t __fastcall Reference_IsAImpl(uint8_t*, re::Edx, int id) {
    if (int16_t(id) == ReferenceId()) {
        return 1;
    }
    return int16_t(id) == NodeId();
}
uint8_t __cdecl Reference_IsAStatic(int id) {
    if (int16_t(id) == ReferenceId()) {
        return 1;
    }
    return int16_t(id) == NodeId();
}

// ActionNodeReference's virtuals: its target's (0 without one)
uint32_t __fastcall Reference_GetConditions(uint8_t* self, re::Edx) {   // 0x5F5480
    return Target(self) ? vc::Call<uint32_t>(Target(self), kNodeGetConditions) : 0;
}
uint32_t __fastcall Reference_NumChildren(uint8_t* self, re::Edx) {   // 0x5F54A0
    return Target(self) ? vc::Call<uint32_t>(Target(self), kNodeNumChildren) : 0;
}
uint32_t __fastcall Reference_GetChild(uint8_t* self, re::Edx, int i) {   // 0x5F54C0
    return Target(self) ? vc::Call<uint32_t>(Target(self), kNodeGetChild, i) : 0;
}
void __fastcall Reference_ClearChildren(uint8_t* self, re::Edx) {   // 0x5F54E0
    if (Target(self)) {
        vc::Call<void>(Target(self), kNodeClearChildren);
    }
}
uint32_t __fastcall Reference_NumConditions(uint8_t* self, re::Edx) {   // 0x5F5500
    return Target(self) ? vc::Call<uint32_t>(Target(self), kNodeNumConditions) : 0;
}
uint32_t __fastcall Reference_GetCondition(uint8_t* self, re::Edx, int i) {   // 0x5F5520
    return Target(self) ? vc::Call<uint32_t>(Target(self), kNodeGetCondition, i) : 0;
}
uint32_t __fastcall Reference_NumTracks(uint8_t* self, re::Edx) {   // 0x5F5540
    return Target(self) ? vc::Call<uint32_t>(Target(self), kNodeNumTracks) : 0;
}
uint32_t __fastcall Reference_CreateTrack(uint8_t* self, re::Edx, int i) {   // 0x5F5560
    return Target(self) ? vc::Call<uint32_t>(Target(self), kNodeCreateTrack, i) : 0;
}
uint32_t __fastcall Reference_GetTrackKey(uint8_t* self, re::Edx, int i) {   // 0x5F5580
    return Target(self) ? vc::Call<uint32_t>(Target(self), kNodeGetTrackKey, i) : 0;
}
uint32_t __fastcall Reference_FindPlayable(uint8_t* self, re::Edx, uint8_t* owner) {   // 0x5F55D0
    return Target(self) ? vc::Call<uint32_t>(Target(self), kNodeFindPlayable, owner) : 0;
}
void __fastcall Reference_AddChild(uint8_t* self, re::Edx, uint8_t* child) {   // 0x5F5790
    if (Target(self)) {
        vc::Call<void>(Target(self), kNodeAddChild, child);
    }
}

// 0x5F55A0 (thiscall)
void __fastcall Reference_ClearTarget(uint8_t* self, re::Edx) {
    Target(self) = nullptr;
}

// 0x5F55B0 (thiscall): the target (its parent set to this when asked; the key unused)
void __fastcall Reference_SetTarget(uint8_t* self, re::Edx, uint8_t* node, int32_t, int setParent) {
    Target(self) = node;
    if (node && re::Flag(setParent)) {
        Parent(node) = self;
    }
}

// 0x5F55F0 (thiscall): ActionNodeBank::FindPlayable: none unless its conditions hold
// (the owner told the node first), then the first child's
uint8_t* __fastcall Bank_FindPlayable(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* conditions = vc::Call<uint8_t*>(self, kNodeGetConditions);
    if (conditions) {
        vc::At<uint8_t*>(owner, 0x34) = self;
        if (!Conditions_Evaluate(conditions, owner)) {
            return nullptr;
        }
    }
    int count = vc::Call<int>(self, kNodeNumChildren);
    for (int i = 0; i < count; i++) {
        uint8_t* found = vc::Call<uint8_t*>(vc::Call<uint8_t*>(self, kNodeGetChild, i), kNodeFindPlayable, owner);
        if (found) {
            return found;
        }
    }
    return nullptr;
}

// 0x5F5660 (thiscall): ActionNodePlayable::FindPlayable: itself when its conditions hold
uint8_t* __fastcall Playable_FindPlayable(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* conditions = vc::Call<uint8_t*>(self, kNodeGetConditions);
    if (conditions) {
        vc::At<uint8_t*>(owner, 0x34) = self;
        return Conditions_Evaluate(conditions, owner) ? self : nullptr;
    }
    return self;
}

// 0x5F5690 (thiscall): its track descriptor `i`
uint8_t* __fastcall Playable_GetTrackDesc(uint8_t* self, re::Edx, uint32_t i) {
    if (i < vc::At<uint32_t>(self, 0x10)) {
        return vc::At<re::Ptr32<uint8_t>*>(self, 0x14)[i];
    }
    return nullptr;
}

// 0x5F57B0 (thiscall): the element out, the ones after moved down
void __fastcall Vector_RemoveAtImpl(PtrVector* self, re::Edx, uint32_t index) {
    while (index < self->count - 1) {
        uint32_t next = index + 1;
        re::Ptr32<uint8_t>* from = next < 1 ? reinterpret_cast<re::Ptr32<uint8_t>*>(reinterpret_cast<uint8_t*>(self) + index * 4 + 4) : &self->rest[index];
        *Elem(self, index) = *from;
        index = next;
    }
    self->count--;
}

// 0x5F5810 (thiscall): the pairs moved to an array of `count`
void __fastcall Pairs_ReserveImpl(PairArray* self, re::Edx, uint32_t count) {
    ParentPair* data = nullptr;
    if (count != 0) {
        uint64_t bytes = uint64_t(count) * 8;
        data = static_cast<ParentPair*>(Malloc(bytes > 0xFFFFFFFFu ? size_t(-1) : size_t(bytes)));
        if (data) {
            VectorConstruct(data, 8, int(count), reinterpret_cast<void*>(0x005F4CB0));
        } else {
            data = nullptr;
        }
    }
    if (self->data) {
        for (uint32_t i = 0; i < uint32_t(self->count); i++) {
            data[i] = self->data[i];
        }
        Free(self->data);
    }
    self->data = data;
    self->capacity = count;
}

// 0x5F5890 (thiscall): ActionNodeReference's deleting destructor
uint8_t* __fastcall Reference_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    SetVtable(self, kVtReference);
    NameKey_Destruct(Name(self));
    SetVtable(self, kVtNode);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x5F5AF0 (thiscall): ActionNode's deleting destructor
uint8_t* __fastcall Node_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    SetVtable(self, kVtNode);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x5F5D00 (thiscall): the children's weights: the total (1 with one child), each
// child's share accumulated, then theirs
void __fastcall Node_UpdateWeights(uint8_t* self, re::Edx) {
    int count = vc::Call<int>(self, kNodeNumChildren);
    float total = 0.0f;
    for (int i = 0; i < count; i++) {
        float weight = Node_GetWeight(vc::Call<uint8_t*>(self, kNodeGetChild, i));
        total = float(double(weight) + double(total));
    }
    if (count == 1) {
        total = 1.0f;
    }
    float acc = 0.0f;
    for (int i = 0; i < count; i++) {
        acc = Node_AccumulateWeight(vc::Call<uint8_t*>(self, kNodeGetChild, i), acc, total);
    }
    int count2 = vc::Call<int>(self, kNodeNumChildren);
    for (int i = 0; i < count2; i++) {
        vc::Call<void>(vc::Call<uint8_t*>(self, kNodeGetChild, i), kNodeUpdateWeights);
    }
}

void DeleteConditions(uint8_t* self) {
    uint8_t* conditions = Conditions(self);
    SetVtable(self, kVtImplementation);
    if (conditions) {
        Conditions_Destruct(conditions);
        Free(conditions);
    }
    NameKey_Destruct(Name(self));
    SetVtable(self, kVtNode);
}

// 0x5F5DC0 (thiscall): ActionNodeImplementation's destructor
void __fastcall Implementation_DestructImpl(uint8_t* self, re::Edx) {
    DeleteConditions(self);
}

// 0x5F5E00 (thiscall): ActionNodePlayableImplementation::GetChild (unchecked)
uint8_t* __fastcall PlayableImpl_GetChild(uint8_t* self, re::Edx, int i) {
    if (i == 0) {
        return vc::At<uint8_t*>(self, 0x18);
    }
    return vc::At<re::Ptr32<uint8_t>*>(self, 0x1C)[i - 1];
}

// 0x5F5E20 (thiscall): the value out of the vector (1: it was there)
uint8_t __fastcall Vector_RemoveImpl(PtrVector* self, re::Edx, uint8_t* value) {
    for (uint32_t i = 0; i < self->count; i++) {
        if (*Elem(self, i) == value) {
            Vector_RemoveAt(self, i);
            return 1;
        }
    }
    return 0;
}

// 0x5F5E60 (thiscall): the first element's FindChild that finds it
uint8_t* __fastcall Vector_FindChildImpl(PtrVector* self, re::Edx, const uint32_t* name) {
    int count = int(self->count);
    for (int i = 0; i < count; i++) {
        uint8_t* found = vc::Call<uint8_t*>(*Elem(self, uint32_t(i)), kNodeFindChild, name);
        if (found) {
            return found;
        }
    }
    return nullptr;
}

// 0x5F5EB0 (thiscall): every element deleted, the vector emptied
void __fastcall Vector_DeleteAllImpl(PtrVector* self, re::Edx) {
    int count = int(self->count);
    for (int i = 0; i < count; i++) {
        uint8_t* node = *Elem(self, uint32_t(i));
        if (node) {
            vc::Call<void>(node, kNodeDestruct, 1);
        }
    }
    self->count = 0;
}

// 0x5F5F00 (thiscall): ActionNodeReference::GetName: the target's, else "UNRESOLVED_NODE"
uint32_t* __fastcall Reference_GetName(uint8_t* self, re::Edx) {
    if (Target(self)) {
        return vc::Call<uint32_t*>(Target(self), kNodeGetName);
    }
    if (!(unresolvedNameGuard & 1)) {
        unresolvedNameGuard |= 1;
        NameKey_Construct(&unresolvedName, "UNRESOLVED_NODE");
        AtExit(reinterpret_cast<void*>(0x008F2FE0));
    }
    return &unresolvedName;
}

// 0x5F5F50 (thiscall): the target looked up by the path at +8 from the parent
void __fastcall Reference_ResolveByPath(uint8_t* self, re::Edx) {
    const char* path = vc::At<const char*>(self, 8);
    if (path && Parent(self)) {
        Target(self) = ActionTree_Find(path, Parent(self));
    }
}

// 0x5F5F70 (thiscall): ActionNodeBank's constructor, by name
uint8_t* __fastcall Bank_ConstructNamed(uint8_t* self, re::Edx, const char* name) {
    Parent(self) = nullptr;
    SetVtable(self, kVtImplementation);
    NameKey_Construct(Name(self), name);
    Conditions(self) = nullptr;
    SetVtable(self, kVtBank);
    vc::At<uint32_t>(self, 0x14) = 0;
    vc::At<uint32_t>(self, 0x18) = 0;
    vc::At<uint32_t>(self, 0x1C) = 0;
    return self;
}

// 0x5F5FB0 (thiscall): ActionNodePlayable's constructor
uint8_t* __fastcall Playable_Construct(uint8_t* self, re::Edx) {
    Parent(self) = nullptr;
    SetVtable(self, kVtImplementation);
    NameKey_Default(Name(self));
    Conditions(self) = nullptr;
    SetVtable(self, kVtPlayable);
    return self;
}

// 0x5F6050 (thiscall): ActionNodePlayable::IsA
uint8_t __fastcall Playable_IsA(uint8_t*, re::Edx, int id) {
    if (int16_t(id) == PlayableId()) {
        return 1;
    }
    return Implementation_IsA(id);
}

// 0x5F60A0 (thiscall): ActionNodePlayable's destructor (its descriptors' array freed)
void __fastcall Playable_DestructImpl(uint8_t* self, re::Edx) {
    SetVtable(self, kVtPlayable);
    Free(vc::At<void*>(self, 0x14));
    DeleteConditions(self);
}

uint8_t* EnsureConditions(uint8_t* self) {
    if (!Conditions(self)) {
        void* p = Malloc(0x10);
        Conditions(self) = p ? Conditions_Construct(static_cast<uint8_t*>(p)) : nullptr;
    }
    return Conditions(self);
}

// 0x5F60F0 (thiscall): room for `count` conditions
void __fastcall Implementation_ReserveConditions(uint8_t* self, re::Edx, uint32_t count) {
    PushMemoryTag(0x25);
    if (count != 0) {
        Conditions_Reserve(EnsureConditions(self), count);
    }
    PopMemoryTag();
}

// 0x5F6140 (thiscall): a condition added (returned)
uint8_t* __fastcall Implementation_AddCondition(uint8_t* self, re::Edx, uint8_t* condition) {
    PushMemoryTag(0x25);
    if (condition) {
        Conditions_Add(EnsureConditions(self), condition);
    }
    PopMemoryTag();
    return condition;
}

// 0x5F6190 / 0x5F6580 (thiscall): RemoveChild on the bank's / playable's children
uint8_t __fastcall Bank_RemoveChild(uint8_t* self, re::Edx, uint8_t* child) {
    return Vector_Remove(BankChildren(self), child);
}
uint8_t __fastcall PlayableImpl_RemoveChild(uint8_t* self, re::Edx, uint8_t* child) {
    return Vector_Remove(PlayableChildren(self), child);
}

// 0x5F61A0 / 0x5F6590 (thiscall): ClearChildren
void __fastcall Bank_ClearChildren(uint8_t* self, re::Edx) {
    Vector_DeleteAll(BankChildren(self));
}
void __fastcall PlayableImpl_ClearChildren(uint8_t* self, re::Edx) {
    Vector_DeleteAll(PlayableChildren(self));
}

// 0x5F61B0 / 0x5F65A0 (thiscall): FindChild: itself by name, else among its children
uint8_t* FindChildIn(uint8_t* self, const uint32_t* name, PtrVector* children) {
    if (*name == actionNodeHash) {
        return nullptr;
    }
    if (*name == *vc::Call<uint32_t*>(self, kNodeGetName)) {
        return self;
    }
    return Vector_FindChild(children, name);
}
uint8_t* __fastcall Bank_FindChild(uint8_t* self, re::Edx, const uint32_t* name) {
    return FindChildIn(self, name, BankChildren(self));
}
uint8_t* __fastcall PlayableImpl_FindChild(uint8_t* self, re::Edx, const uint32_t* name) {
    return FindChildIn(self, name, PlayableChildren(self));
}

// 0x5F61F0 (thiscall): ActionNodeBank::GetChild (unchecked)
uint8_t* __fastcall Bank_GetChild(uint8_t* self, re::Edx, int i) {
    if (i == 0) {
        return vc::At<uint8_t*>(self, 0x10);
    }
    return vc::At<re::Ptr32<uint8_t>*>(self, 0x14)[i - 1];
}

// 0x5F6210 (thiscall)
uint32_t __fastcall Implementation_NumConditions(uint8_t* self, re::Edx) {
    if (Conditions(self)) {
        return vc::At<uint32_t>(Conditions(self), 0xC);
    }
    return 0;
}

// 0x5F6220 (thiscall)
uint8_t* __fastcall Implementation_GetCondition(uint8_t* self, re::Edx, int i) {
    PtrVector* conditions = vc::Call<PtrVector*>(self, kNodeGetConditions);
    if (conditions && i >= 0 && i < int(conditions->count)) {
        return *Elem(conditions, uint32_t(i));
    }
    return nullptr;
}

// 0x5F6260 (thiscall): its track `i` made by the registry from the descriptor
uint8_t* __fastcall Playable_CreateTrack(uint8_t* self, re::Edx, uint32_t i) {
    if (i >= vc::At<uint32_t>(self, 0x10)) {
        return nullptr;
    }
    uint8_t* desc = vc::At<re::Ptr32<uint8_t>*>(self, 0x14)[i];
    uint32_t key;
    TrackDesc_GetKey(desc, &key);
    PushMemoryTag(0x26);
    uint8_t* track = TrackRegistry_Create(trackRegistry_1B6565C, &key);
    PopMemoryTag();
    if (track) {
        TrackDesc_Apply(desc, track);
        NameKey_Destruct(&key);
        return track;
    }
    NameKey_Destruct(&key);
    return nullptr;
}

// 0x5F62D0 (thiscall): ActionNodeFileReference's constructor
uint8_t* __fastcall FileReference_Construct(uint8_t* self, re::Edx) {
    Parent(self) = nullptr;
    SetVtable(self, kVtReference);
    NameKey_Default(Name(self));
    Target(self) = nullptr;
    SetVtable(self, kVtFileReference);
    NameKey_Default(&vc::At<uint32_t>(self, 0x18));
    NameKey_Default(&vc::At<uint32_t>(self, 0x1C));
    vc::At<uint32_t>(self, 0x24) &= 0xFFFFFFF8;
    vc::At<uint32_t>(self, 0x10) = 0;
    vc::At<uint32_t>(self, 0x14) = 0;
    vc::At<uint32_t>(self, 0x20) = 0;
    self[0x28] = 1;
    return self;
}

// 0x5F6390 (thiscall): ActionNodeFileReference::IsA
uint8_t __fastcall FileReference_IsA(uint8_t*, re::Edx, int id) {
    if (int16_t(id) == TypeId(fileReferenceIdGuard, fileReferenceId)) {
        return 1;
    }
    return Reference_IsA(id);
}

// 0x5F63E0 (thiscall)
const char* __fastcall FileReference_TypeName(uint8_t*, re::Edx) {
    return reinterpret_cast<const char*>(0x0092DC60);   // the game's "FileReference"
}

// 0x5F63F0 (thiscall): MultiParentReference::GetParent: the parent under the key (-1:
// the current key), else +4
uint8_t* __fastcall MultiParent_GetParent(uint8_t* self, re::Edx, int32_t key) {
    if (key == -1) {
        key = currentKey;
        if (key == -1) {
            return Parent(self);
        }
    }
    PairArray* pairs = Pairs(self);
    for (int i = 0; i < pairs->count; i++) {
        if (pairs->data[i].key == key) {
            return pairs->data[i].parent;
        }
    }
    return Parent(self);
}

// 0x5F6450 (thiscall): a parent's pair out
void __fastcall MultiParent_RemoveParentImpl(uint8_t* self, re::Edx, uint8_t* parent) {
    PairArray* pairs = Pairs(self);
    int count = pairs->count;
    uint32_t i = 0;
    if (count <= 0) {
        return;
    }
    while (pairs->data[i].parent != parent) {
        i++;
        if (int(i) >= count) {
            return;
        }
    }
    for (; i < uint32_t(count - 1); count = pairs->count) {
        pairs->data[i] = pairs->data[i + 1];
        i++;
    }
    pairs->count--;
}

// 0x5F64B0 (thiscall): ActionNodeBank's destructor
void __fastcall Bank_DestructImpl(uint8_t* self, re::Edx) {
    SetVtable(self, kVtBank);
    if (vc::At<void*>(self, 0x14)) {
        Free(vc::At<void*>(self, 0x14));
        vc::At<void*>(self, 0x14) = nullptr;
    }
    DeleteConditions(self);
}

// 0x5F6530 (thiscall): ActionNodeImplementation's deleting destructor
uint8_t* __fastcall Implementation_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    DeleteConditions(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x5F65E0 (thiscall): ActionNodeBank's constructor
uint8_t* __fastcall Bank_Construct(uint8_t* self, re::Edx) {
    Parent(self) = nullptr;
    SetVtable(self, kVtImplementation);
    NameKey_Default(Name(self));
    Conditions(self) = nullptr;
    SetVtable(self, kVtBank);
    vc::At<uint32_t>(self, 0x14) = 0;
    vc::At<uint32_t>(self, 0x18) = 0;
    vc::At<uint32_t>(self, 0x1C) = 0;
    return self;
}

// 0x5F6610 (thiscall): ActionNodePlayable's deleting destructor
uint8_t* __fastcall Playable_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    SetVtable(self, kVtPlayable);
    Free(vc::At<void*>(self, 0x14));
    DeleteConditions(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x5F6670 (thiscall): the file's tree let go: an owned one deleted (out of the file
// cache first); a shared one only drops this parent
void __fastcall FileReference_ReleaseImpl(uint8_t* self, re::Edx) {
    uint8_t* target = Target(self);
    if (!target) {
        return;
    }
    if (self[0x28]) {
        FileCache_Remove(fileCache_D02850, target);
        if (Target(self)) {
            vc::Call<void>(Target(self), kNodeDestruct, 1);
        }
        return;
    }
    uint8_t* child = vc::Call<uint8_t*>(target, kNodeGetChild, 0);
    if (!child) {
        return;
    }
    uint8_t* shared = vc::Call<uint8_t*>(child, kNodeGetParent, -1);
    if (shared && vc::Call<uint8_t>(shared, kNodeIsA, int(uint16_t(MultiParentId())))) {
        MultiParent_RemoveParent(shared, Parent(self));
    }
}

// 0x5F66E0 (thiscall): ActionNodeBankWithCompressedData's deleting destructor
uint8_t* __fastcall BankCompressed_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    uint8_t* data = vc::At<uint8_t*>(self, 0x20);
    SetVtable(self, kVtBankCompressed);
    if (data) {
        CompressedData_Destruct(data);
        Free(data);
    }
    Bank_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x5F6720 / 0x5F6790 (thiscall): room for `count` children (the first is inline)
void __fastcall PlayableImpl_Reserve(uint8_t* self, re::Edx, uint32_t count) {
    if (count > 1) {
        Vector_ReserveRest(PlayableChildren(self), count - 1);
    }
}
void __fastcall Bank_Reserve(uint8_t* self, re::Edx, uint32_t count) {
    if (count > 1) {
        Vector_ReserveRest(BankChildren(self), count - 1);
    }
}

// 0x5F6740 (thiscall): appended (grown when full)
void __fastcall Vector_PushBackImpl(PtrVector* self, re::Edx, uint8_t* value) {
    if (self->count == self->restCapacity + 1) {
        Vector_ReserveRest(self, Vector_GrowCapacity(self->restCapacity));
    }
    if (self->count < 1) {
        (&self->first)[self->count] = value;
        self->count++;
        return;
    }
    self->rest[self->count - 1] = value;
    self->count++;
}

// 0x5F67B0 (thiscall): ActionNodeBank::AddChild
void __fastcall Bank_AddChild(uint8_t* self, re::Edx, uint8_t* child) {
    if (child) {
        vc::Call<void>(child, kNodeSetParent, self);
        Vector_PushBack(BankChildren(self), child);
    }
}

void DestructReferenceBase(uint8_t* self) {
    FileNames_Destruct(self + 0x10);
    SetVtable(self, kVtReference);
    NameKey_Destruct(Name(self));
    SetVtable(self, kVtNode);
}

// 0x5F67E0 (thiscall): ActionNodeFileReference's destructor
void __fastcall FileReference_DestructImpl(uint8_t* self, re::Edx) {
    SetVtable(self, kVtFileReference);
    FileReference_Release(self);
    DestructReferenceBase(self);
}

// 0x5F6810 (thiscall): ActionNodeMultiParentReference's constructor
uint8_t* __fastcall MultiParent_Construct(uint8_t* self, re::Edx) {
    FileReference_Construct(self, nullptr);
    SetVtable(self, kVtMultiParent);
    vc::At<uint32_t>(self, 0x30) = 0;
    vc::At<uint32_t>(self, 0x34) = 0;
    vc::At<uint32_t>(self, 0x38) = 0;
    return self;
}

// 0x5F6870 (thiscall): ActionNodeMultiParentReference::IsA
uint8_t __fastcall MultiParent_IsA(uint8_t*, re::Edx, int id) {
    if (int16_t(id) == MultiParentId()) {
        return 1;
    }
    if (int16_t(id) == TypeId(fileReferenceIdGuard, fileReferenceId)) {
        return 1;
    }
    return Reference_IsA(id);
}

// 0x5F68F0 (thiscall): ActionNodeMultiParentReference's destructor (its tree always
// deleted, out of the file cache)
void __fastcall MultiParent_DestructImpl(uint8_t* self, re::Edx) {
    SetVtable(self, kVtMultiParent);
    if (Target(self)) {
        FileCache_Remove(fileCache_D02850, Target(self));
        if (Target(self)) {
            vc::Call<void>(Target(self), kNodeDestruct, 1);
        }
    }
    if (vc::At<void*>(self, 0x30)) {
        Free(vc::At<void*>(self, 0x30));
        vc::At<void*>(self, 0x30) = nullptr;
    }
    SetVtable(self, kVtFileReference);
    FileReference_Release(self);
    DestructReferenceBase(self);
}

// 0x5F6960 (thiscall): the parent under a key (-1: the default parent); an existing
// pair with the key or the parent updated, else a new one
void __fastcall MultiParent_AddParentImpl(uint8_t* self, re::Edx, uint8_t* parent, int32_t key) {
    if (key == -1) {
        Parent(self) = parent;
        return;
    }
    PairArray* pairs = Pairs(self);
    for (int i = 0; i < pairs->count; i++) {
        if (pairs->data[i].key == key) {
            pairs->data[i].parent = parent;
            return;
        }
        if (pairs->data[i].parent == parent) {
            pairs->data[i].key = key;
            return;
        }
    }
    if (uint32_t(pairs->count) == pairs->capacity) {
        Pairs_Reserve(pairs, pairs->capacity == 0 ? 3 : pairs->capacity * 2);
    }
    pairs->data[pairs->count].key = key;
    pairs->data[pairs->count].parent = parent;
    pairs->count++;
}

// 0x5F69F0 (thiscall): FileReference::Resolve: a shared tree gets this parent under the
// current key, then the children resolved
void __fastcall FileReference_Resolve(uint8_t* self, re::Edx) {
    uint8_t* target = Target(self);
    if (target && vc::Call<int>(target, kNodeNumChildren) != 0) {
        uint8_t* child = vc::Call<uint8_t*>(target, kNodeGetChild, 0);
        uint8_t* shared = vc::Call<uint8_t*>(child, kNodeGetParent, -1);
        if (shared && vc::Call<uint8_t>(shared, kNodeIsA, int(uint16_t(MultiParentId())))) {
            int32_t key = currentKey;
            MultiParent_AddParent(shared, vc::Call<uint8_t*>(self, kNodeGetParent, -1), key);
        }
    }
    Node_ResolveChildren(self);
}

// 0x5F6A60 (thiscall): ActionNodePlayableImplementation::AddChild
void __fastcall PlayableImpl_AddChild(uint8_t* self, re::Edx, uint8_t* child) {
    vc::Call<void>(child, kNodeSetParent, self);
    PtrVector* v = PlayableChildren(self);
    if (v->count == v->restCapacity + 1) {
        Vector_ReserveRest(v, Vector_GrowCapacity(v->restCapacity));
    }
    if (v->count < 1) {
        (&v->first)[v->count] = child;
        v->count++;
        return;
    }
    v->rest[v->count - 1] = child;
    v->count++;
}

// 0x5F6CC0 (thiscall): ActionNodeMultiParentReference's deleting destructor
uint8_t* __fastcall MultiParent_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    MultiParent_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x8F2FE0 (exit): the "UNRESOLVED_NODE" key's destructor
void __cdecl UnresolvedName_Exit() {
    NameKey_Destruct(&unresolvedName);
}

} // namespace

#define ACTNODE_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

ACTNODE_REVERSED("ActionNode::IsA", 0x005F4E00, &Node_IsA);
ACTNODE_REVERSED("ActionNode::UnresolvedNameExit", 0x008F2FE0, &UnresolvedName_Exit);
ACTNODE_REVERSED("ActionNode::GetParent", 0x005F4E40, &Node_GetParent);
ACTNODE_REVERSED("ActionNode::Destruct", 0x005F4E50, &Node_Destruct);
ACTNODE_REVERSED("ActionNode::Get0C_5F4F90", 0x005F4F90, &Node_Get0C);
ACTNODE_REVERSED("ActionNode::GetRootKey", 0x005F4FA0, &Node_GetRootKeyImpl);
ACTNODE_REVERSED("ActionNodeVector::GrowCapacity", 0x005F4FE0, &GrowCapacity);
ACTNODE_REVERSED("ActionNodeImplementation::AddChild", 0x005F5040, &Implementation_AddChild);
ACTNODE_REVERSED("ActionNodeImplementation::FindChild", 0x005F5050, &Implementation_FindChild);
ACTNODE_REVERSED("ActionNode::HasAncestorNamed", 0x005F5090, &Node_HasAncestorNamed);
ACTNODE_REVERSED("ActionNode::HasAncestor", 0x005F50E0, &Node_HasAncestor);
ACTNODE_REVERSED("ActionNode::FindFirstPlayable", 0x005F5120, &Node_FindFirstPlayableImpl);
ACTNODE_REVERSED("ActionTree::ResolveAll", 0x005F51B0, &ResolveAll);
ACTNODE_REVERSED("ActionNode::ResolveChildren", 0x005F5210, &Node_ResolveChildrenImpl);
ACTNODE_REVERSED("ActionNode::GetWeight", 0x005F5250, &Node_GetWeightImpl);
ACTNODE_REVERSED("ActionNode::AccumulateWeight", 0x005F52D0, &Node_AccumulateWeightImpl);
ACTNODE_REVERSED("ActionNodeImplementation::IsA", 0x005F5370, &Implementation_IsAImpl);
ACTNODE_REVERSED("ActionNodeReference::Construct", 0x005F53E0, &Reference_Construct);
ACTNODE_REVERSED("ActionNodeReference::IsA", 0x005F5410, &Reference_IsAImpl);
ACTNODE_REVERSED("ActionNodeReference::GetConditions", 0x005F5480, &Reference_GetConditions);
ACTNODE_REVERSED("ActionNodeReference::NumChildren", 0x005F54A0, &Reference_NumChildren);
ACTNODE_REVERSED("ActionNodeReference::GetChild", 0x005F54C0, &Reference_GetChild);
ACTNODE_REVERSED("ActionNodeReference::ClearChildren", 0x005F54E0, &Reference_ClearChildren);
ACTNODE_REVERSED("ActionNodeReference::NumConditions", 0x005F5500, &Reference_NumConditions);
ACTNODE_REVERSED("ActionNodeReference::GetCondition", 0x005F5520, &Reference_GetCondition);
ACTNODE_REVERSED("ActionNodeReference::NumTracks", 0x005F5540, &Reference_NumTracks);
ACTNODE_REVERSED("ActionNodeReference::CreateTrack", 0x005F5560, &Reference_CreateTrack);
ACTNODE_REVERSED("ActionNodeReference::GetTrackKey", 0x005F5580, &Reference_GetTrackKey);
ACTNODE_REVERSED("ActionNodeReference::ClearTarget", 0x005F55A0, &Reference_ClearTarget);
ACTNODE_REVERSED("ActionNodeReference::SetTarget", 0x005F55B0, &Reference_SetTarget);
ACTNODE_REVERSED("ActionNodeReference::FindPlayable", 0x005F55D0, &Reference_FindPlayable);
ACTNODE_REVERSED("ActionNodeBank::FindPlayable", 0x005F55F0, &Bank_FindPlayable);
ACTNODE_REVERSED("ActionNodePlayable::FindPlayable", 0x005F5660, &Playable_FindPlayable);
ACTNODE_REVERSED("ActionNodePlayable::GetTrackDesc", 0x005F5690, &Playable_GetTrackDesc);
ACTNODE_REVERSED("ActionNodeReference::AddChild", 0x005F5790, &Reference_AddChild);
ACTNODE_REVERSED("ActionNodeVector::RemoveAt", 0x005F57B0, &Vector_RemoveAtImpl);
ACTNODE_REVERSED("ActionNodeParents::Reserve", 0x005F5810, &Pairs_ReserveImpl);
ACTNODE_REVERSED("ActionNodeReference::DeletingDestruct", 0x005F5890, &Reference_DeletingDestruct);
ACTNODE_REVERSED("ActionNode::DeletingDestruct", 0x005F5AF0, &Node_DeletingDestruct);
ACTNODE_REVERSED("ActionNode::UpdateWeights", 0x005F5D00, &Node_UpdateWeights);
ACTNODE_REVERSED("ActionNodeImplementation::Destruct", 0x005F5DC0, &Implementation_DestructImpl);
ACTNODE_REVERSED("ActionNodePlayableImplementation::GetChild", 0x005F5E00, &PlayableImpl_GetChild);
ACTNODE_REVERSED("ActionNodeVector::Remove", 0x005F5E20, &Vector_RemoveImpl);
ACTNODE_REVERSED("ActionNodeVector::FindChild", 0x005F5E60, &Vector_FindChildImpl);
ACTNODE_REVERSED("ActionNodeVector::DeleteAll", 0x005F5EB0, &Vector_DeleteAllImpl);
ACTNODE_REVERSED("ActionNodeReference::GetName", 0x005F5F00, &Reference_GetName);
ACTNODE_REVERSED("ActionNodeReference::ResolveByPath", 0x005F5F50, &Reference_ResolveByPath);
ACTNODE_REVERSED("ActionNodeBank::ConstructNamed", 0x005F5F70, &Bank_ConstructNamed);
ACTNODE_REVERSED("ActionNodePlayable::Construct", 0x005F5FB0, &Playable_Construct);
ACTNODE_REVERSED("ActionNodeImplementation::IsAStatic", 0x005F5FE0, &Implementation_IsAStatic);
ACTNODE_REVERSED("ActionNodePlayable::IsA", 0x005F6050, &Playable_IsA);
ACTNODE_REVERSED("ActionNodePlayable::Destruct", 0x005F60A0, &Playable_DestructImpl);
ACTNODE_REVERSED("ActionNodeImplementation::ReserveConditions", 0x005F60F0, &Implementation_ReserveConditions);
ACTNODE_REVERSED("ActionNodeImplementation::AddCondition", 0x005F6140, &Implementation_AddCondition);
ACTNODE_REVERSED("ActionNodeBank::RemoveChild", 0x005F6190, &Bank_RemoveChild);
ACTNODE_REVERSED("ActionNodeBank::ClearChildren", 0x005F61A0, &Bank_ClearChildren);
ACTNODE_REVERSED("ActionNodeBank::FindChild", 0x005F61B0, &Bank_FindChild);
ACTNODE_REVERSED("ActionNodeBank::GetChild", 0x005F61F0, &Bank_GetChild);
ACTNODE_REVERSED("ActionNodeImplementation::NumConditions", 0x005F6210, &Implementation_NumConditions);
ACTNODE_REVERSED("ActionNodeImplementation::GetCondition", 0x005F6220, &Implementation_GetCondition);
ACTNODE_REVERSED("ActionNodePlayable::CreateTrack", 0x005F6260, &Playable_CreateTrack);
ACTNODE_REVERSED("ActionNodeFileReference::Construct", 0x005F62D0, &FileReference_Construct);
ACTNODE_REVERSED("ActionNodeReference::IsAStatic", 0x005F6320, &Reference_IsAStatic);
ACTNODE_REVERSED("ActionNodeFileReference::IsA", 0x005F6390, &FileReference_IsA);
ACTNODE_REVERSED("ActionNodeFileReference::TypeName", 0x005F63E0, &FileReference_TypeName);
ACTNODE_REVERSED("ActionNodeMultiParentReference::GetParent", 0x005F63F0, &MultiParent_GetParent);
ACTNODE_REVERSED("ActionNodeMultiParentReference::RemoveParent", 0x005F6450, &MultiParent_RemoveParentImpl);
ACTNODE_REVERSED("ActionNodeBank::Destruct", 0x005F64B0, &Bank_DestructImpl);
ACTNODE_REVERSED("ActionNodeImplementation::DeletingDestruct", 0x005F6530, &Implementation_DeletingDestruct);
ACTNODE_REVERSED("ActionNodePlayableImplementation::RemoveChild", 0x005F6580, &PlayableImpl_RemoveChild);
ACTNODE_REVERSED("ActionNodePlayableImplementation::ClearChildren", 0x005F6590, &PlayableImpl_ClearChildren);
ACTNODE_REVERSED("ActionNodePlayableImplementation::FindChild", 0x005F65A0, &PlayableImpl_FindChild);
ACTNODE_REVERSED("ActionNodeBank::Construct", 0x005F65E0, &Bank_Construct);
ACTNODE_REVERSED("ActionNodePlayable::DeletingDestruct", 0x005F6610, &Playable_DeletingDestruct);
ACTNODE_REVERSED("ActionNodeFileReference::Release", 0x005F6670, &FileReference_ReleaseImpl);
ACTNODE_REVERSED("ActionNodeBankWithCompressedData::DeletingDestruct", 0x005F66E0, &BankCompressed_DeletingDestruct);
ACTNODE_REVERSED("ActionNodePlayableImplementation::Reserve", 0x005F6720, &PlayableImpl_Reserve);
ACTNODE_REVERSED("ActionNodeVector::PushBack", 0x005F6740, &Vector_PushBackImpl);
ACTNODE_REVERSED("ActionNodeBank::Reserve", 0x005F6790, &Bank_Reserve);
ACTNODE_REVERSED("ActionNodeBank::AddChild", 0x005F67B0, &Bank_AddChild);
ACTNODE_REVERSED("ActionNodeFileReference::Destruct", 0x005F67E0, &FileReference_DestructImpl);
ACTNODE_REVERSED("ActionNodeMultiParentReference::Construct", 0x005F6810, &MultiParent_Construct);
ACTNODE_REVERSED("ActionNodeMultiParentReference::IsA", 0x005F6870, &MultiParent_IsA);
ACTNODE_REVERSED("ActionNodeMultiParentReference::Destruct", 0x005F68F0, &MultiParent_DestructImpl);
ACTNODE_REVERSED("ActionNodeMultiParentReference::AddParent", 0x005F6960, &MultiParent_AddParentImpl);
ACTNODE_REVERSED("ActionNodeFileReference::Resolve", 0x005F69F0, &FileReference_Resolve);
ACTNODE_REVERSED("ActionNodePlayableImplementation::AddChild", 0x005F6A60, &PlayableImpl_AddChild);
ACTNODE_REVERSED("ActionNodeMultiParentReference::DeletingDestruct", 0x005F6CC0, &MultiParent_DeletingDestruct);
