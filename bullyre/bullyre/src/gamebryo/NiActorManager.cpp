// NiActorManager (src/gamebryo/NiActorManager.h): construction, cloning and destruction, the
// NIF root and its controller manager, loading the sequences the KFM tool names, the sequence
// map, resetting, the callbacks and the timeline of their events; with the small classes and
// template pieces it uses: NiTextKeyMatch, the completion infos' constructors and destructors,
// the lists' node allocator, the lists' and maps' functions the manager instantiates, and the two
// NiControllerManager helpers it calls. The timing (when each event falls) is
// NiActorManager2.cpp's.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiActorManager.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::actor {
namespace {

using sys::Decrement;
using sys::Increment;
using sys::kNullFixedString;
using sys::Virtual;

GAME_VAR(long, ms_uiObjects, 0x020E2168);
GAME_VAR(char*, ms_kEndTextKey, 0x020E4AD0);     // NiActorManager's "end"
GAME_VAR(char*, ms_kStartTextKey, 0x020E4AD4);   // and "start"
GAME_VAR(uint32_t, NiNode_ClassIndex, 0x020E2550);
GAME_VAR(uint32_t, NiAVObject_ClassIndex, 0x020E251C);
GAME_VAR(sys::NiCriticalSection, TimelineAllocator_ms_kCriticalSection, 0x020E4AB0);
GAME_VAR(void*, TimelineAllocator_ms_pBlockList, 0x020E4AA8);
GAME_VAR(uint8_t*, TimelineAllocator_ms_pFreeList, 0x020E4AAC);
GAME_VAR(uint32_t, TimelineAllocator_ms_uiBlockSize, 0x00BC8BDC);
constexpr uintptr_t kNiControllerManager_RTTI = 0x020E50D0;
constexpr uintptr_t kNiBoneLODController_RTTI = 0x020E5444;

constexpr uintptr_t kRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t kActorManagerVtable = 0x00965A88;
constexpr uintptr_t kTextKeyMatchVtable = 0x00965A60;
constexpr uintptr_t kChainCompletionInfoVtable = 0x00965A6C;
constexpr uintptr_t kCompletionInfoVtable = 0x00965AB0;
constexpr uintptr_t kChainIDListVtable = 0x0095145C;      // NiTPointerList<unsigned int> (the constructor's)
constexpr uintptr_t kChainIDListDtorVtable = 0x00952E10;  // (the destructor's)
constexpr uintptr_t kFloatListVtable = 0x00965A7C;        // NiTPointerList<float>
constexpr uintptr_t kTimelineListVtable = 0x00965A54;     // NiTPointerList<TimelineData>
constexpr uintptr_t kListBaseVtable = 0x0099C748;
constexpr uintptr_t kMapVtable = 0x00965A9C;
constexpr uintptr_t kMapBaseVtable = 0x00950760;
constexpr uintptr_t kCallbackArrayVtable = 0x00961DE8;
constexpr uintptr_t kArrayBaseVtable = 0x009893C0;
constexpr uintptr_t kInterpArrayVtable = 0x00965A90;
constexpr uintptr_t kTimelineNodeConstruct = 0x00801140;

GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* construct);
GAME_FN(void, __thiscall, ListBase_RemoveAll, 0x0075DB60, void* list);
GAME_FN(void, __thiscall, Map_RemoveAll, 0x005AC520, PointerMap* self);
GAME_FN(void, __thiscall, Map_SetAt, 0x008A5140, PointerMap* self, uint32_t key, void* value);
GAME_FN(void, __thiscall, Map_RemoveAt, 0x008A5A10, PointerMap* self, uint32_t key);
GAME_FN(void, __thiscall, Map_GetNext, 0x008976C0, PointerMap* self, re::Ptr32<MapNode>* pos, uint32_t* key, re::Ptr32<void>* value);
GAME_FN(void, __cdecl, Array_Free, 0x00836F80, void* p);   // NiTArray's allocator
GAME_FN(void, __thiscall, Array_SetSize, 0x00856170, void* array, uint32_t size);
GAME_FN(void, __thiscall, Array_RemoveAll, 0x0040E7A0, void* array);
GAME_FN(void, __thiscall, Array_SetAt, 0x0040E800, void* array, uint32_t index, const re::Ptr32<NiRefObject>* value);
GAME_FN(void, __thiscall, ObjectArray_Add, 0x007606C0, void* array, const re::Ptr32<NiRefObject>* value);
GAME_FN(uint32_t, __cdecl, StringHash, 0x005F09F0, const char* key, uint32_t hashSize);
GAME_FN(char*, __cdecl, StringTable_AddString, 0x00754760, const char* text);
GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, const re::Ptr32<char>* other);
GAME_FN(void, __thiscall, FixedString_AssignText, 0x00406730, re::Ptr32<char>* self, const char* text);
GAME_FN(void, __thiscall, FixedString_Destruct, 0x00517750, re::Ptr32<char>* self);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(void, __thiscall, Filename_Construct, 0x007538C0, void* self, const char* path);
GAME_FN(void*, __thiscall, Stream_Construct, 0x0075CE40, void* self);
GAME_FN(void*, __thiscall, CloningProcess_Construct, 0x006C7000, void* self, uint32_t hashSize);
GAME_FN(NiAVObject*, __thiscall, Object_Clone, 0x007551F0, void* self, void* cloning);
GAME_FN(NiRefObject*, __thiscall, ObjectNET_GetController, 0x007647D0, void* self, uintptr_t rtti);
GAME_FN(re::Ptr32<NiRefObject>*, __thiscall, ControllerManager_RemoveSequence, 0x008185F0, void* self, re::Ptr32<NiRefObject>* result,
        void* sequence);
GAME_FN(uint8_t, __thiscall, ControllerManager_AddSequence, 0x00818A90, void* self, void* sequence,
        const re::Ptr32<char>* name, int storeTargets);
GAME_FN(void*, __thiscall, ControllerManager_Construct, 0x008189A0, void* self, NiAVObject* target, int cumulative);
GAME_FN(uint8_t, __thiscall, ControllerSequence_StartBlend, 0x00816890, void* self, void* destination, float duration,
        const char* destinationKey, int32_t priority, float sourceWeight, float destinationWeight, void* timeSync);
GAME_FN(uint8_t, __thiscall, ControllerSequence_Deactivate, 0x00814A20, void* self, float easeOut, int transition);
GAME_FN(re::Ptr32<NiRefObject>*, __cdecl, ControllerSequence_CreateFromFile, 0x00814480, re::Ptr32<NiRefObject>* result, void* stream,
        uint32_t index);
GAME_FN(re::Ptr32<char>*, __thiscall, KFMTool_GetFullKFFilename, 0x00822AF0, NiKFMTool* self, re::Ptr32<char>* result, uint32_t id);
GAME_FN(void, __thiscall, KFMTool_GetSequenceIDs, 0x00822730, NiKFMTool* self, re::Ptr32<uint32_t>* ids, uint32_t* count);
GAME_FN(re::Ptr32<char>*, __thiscall, KFMTool_GetFullModelPath, 0x00822A20, NiKFMTool* self);
GAME_FN(float, __thiscall, ActorManager_GetNextEventTime, 0x00803900, NiActorManager* self, uint32_t eventType,
        uint32_t id, NiTextKeyMatch* match);
// (this file's, called where modes apply)
GAME_FN(void, __cdecl, TimelineAllocator_Deallocate, 0x008004E0, void* node);
GAME_FN(void, __thiscall, ControllerManager_DeactivateAll, 0x00800AE0, void* self, float easeOut);
GAME_FN(void, __thiscall, TextKeyMatch_Destruct, 0x00800B20, NiTextKeyMatch* self);
GAME_FN(void, __thiscall, ActorManager_Reset, 0x00800BA0, NiActorManager* self);
GAME_FN(void*, __thiscall, ActorManager_GetSequence, 0x00800C30, NiActorManager* self, uint32_t id);
GAME_FN(void, __thiscall, TimelineData_SetValues, 0x00800C70, TimelineData* self, float time, uint32_t eventType,
        uint32_t id, NiTextKeyMatch* match, const re::Ptr32<char>* textKey);
GAME_FN(void, __thiscall, ChainCompletionInfo_Destruct, 0x00800CE0, ChainCompletionInfo* self);
GAME_FN(NiRefObject*, __thiscall, ActorManager_FindControllerManager, 0x00800DA0, NiActorManager* self,
        NiAVObject* object);
GAME_FN(NiRefObject*, __thiscall, ActorManager_FindBoneLODController, 0x00800E30, NiActorManager* self,
        NiAVObject* object);
GAME_FN(void, __thiscall, ActorManager_UnloadSequence, 0x00800EC0, NiActorManager* self, uint32_t id);
GAME_FN(void, __thiscall, ActorManager_RemoveInactiveExtraSequences, 0x008010C0, NiActorManager* self);
GAME_FN(void, __thiscall, ActorManager_Destruct, 0x00801270, NiActorManager* self);
GAME_FN(uint8_t, __thiscall, ActorManager_ChangeSequence, 0x00801390, NiActorManager* self, uint32_t id,
        void* sequence);
GAME_FN(void, __cdecl, TimelineAllocator_CreateFreeMem, 0x00801440);
GAME_FN(NiActorManager*, __thiscall, ActorManager_Construct, 0x00801540, void* self, NiKFMTool* kfmTool,
        int cumulative);
GAME_FN(uint8_t, __thiscall, ActorManager_LoadSequence, 0x00801690, NiActorManager* self, uint32_t id,
        void* stream, int loadKFFile);
GAME_FN(uint8_t, __thiscall, ActorManager_LoadAllSequences, 0x00801CF0, NiActorManager* self, void* stream);
GAME_FN(uint8_t, __thiscall, ActorManager_ChangeNIFRoot, 0x00801E80, NiActorManager* self, NiAVObject* root);
GAME_FN(void, __thiscall, CompletionInfo_Destruct, 0x008021D0, CompletionInfo* self);
GAME_FN(uint8_t, __thiscall, ActorManager_Initialize, 0x00803590, NiActorManager* self, void* stream);
GAME_FN(void, __thiscall, ActorManager_AddTimelineDataObject, 0x00803AE0, NiActorManager* self, uint32_t eventType,
        uint32_t id, NiTextKeyMatch* match);
GAME_FN(re::Ptr32<NiRefObject>*, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<NiRefObject>* self, NiRefObject* object);
GAME_FN(uint8_t, __thiscall, StringMap_GetAt, 0x00800470, void* self, const char* key, re::Ptr32<void>* value);
GAME_FN(TimelineData*, __thiscall, Timeline_AddNewTail, 0x008009E0, NiTPointerList<TimelineData>* self);
GAME_FN(TimelineData*, __thiscall, Timeline_InsertNewBefore, 0x00800A20, NiTPointerList<TimelineData>* self,
        NiTListNode<TimelineData>* position);

inline void ReleaseImport(NiRefObject* object) {
    if (object != nullptr && Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

// a reference slot assigned: the old one let go first, the new one stored, then held
template <typename T, typename U>
inline void AssignReleasingFirst(re::Ptr32<T>& slot, U* value) {
    T* v = reinterpret_cast<T*>(value);
    T* old = slot;
    if (old == v) {
        return;
    }
    ReleaseImport(reinterpret_cast<NiRefObject*>(old));
    slot = v;
    if (v != nullptr) {
        Increment(&reinterpret_cast<NiRefObject*>(v)->refCount);
    }
}
#if defined(_WIN64)
template <typename T, typename U>
inline void AssignReleasingFirst(re::Ptr32<T>& slot, re::Ptr32<U> value) {
    AssignReleasingFirst(slot, value.get());
}
#endif

// the NiRefObject part of a constructor, inlined
inline void RefObjectInit(NiRefObject* self) {
    self->vtable = reinterpret_cast<void*>(kRefObjectVtable);
    self->refCount = 0;
    Increment(&ms_uiObjects);
}

inline void RefObjectFinish(NiRefObject* self) {
    self->vtable = reinterpret_cast<void*>(kRefObjectVtable);
    Decrement(&ms_uiObjects);
}

// NiObject::IsKindOf through the RTTI's class bit set
inline bool KindOf(const void* object, uint32_t index) {
    const uint8_t* rtti = Virtual<const uint8_t*>(object, 2);
    return (*reinterpret_cast<const uint32_t*>(rtti + 0xC + (index >> 5) * 4) & (1u << (index & 31))) != 0;
}

// a list's head taken off and freed (its allocator, slot 2)
template <typename T>
inline NiTListNode<T>* UnlinkHead(NiTPointerList<T>& list) {
    NiTListNode<T>* node = list.head;
    NiTListNode<T>* next = node->next;
    list.head = next;
    if (next != nullptr) {
        next->prev = nullptr;
    } else {
        list.tail = nullptr;
    }
    return node;
}

// a node put at a list's tail
template <typename T>
inline void LinkTail(NiTPointerList<T>& list, NiTListNode<T>* node) {
    node->next = nullptr;
    node->prev = list.tail;
    if (list.tail != nullptr) {
        list.tail->next = node;
        list.count++;
        list.tail = node;
    } else {
        list.count++;
        list.head = node;
        list.tail = node;
    }
}

// the scene graph searched for a controller of the given class (the object's own first, then a
// node's children)
NiRefObject* FindController(NiActorManager* self, NiAVObject* object, uintptr_t rtti, uintptr_t recurse) {
    NiRefObject* controller = ObjectNET_GetController(object, rtti);
    if (controller != nullptr) {
        return controller;
    }
    if (object == nullptr || !KindOf(object, NiNode_ClassIndex)) {
        return nullptr;
    }
    auto* node = reinterpret_cast<NiNode*>(object);
    uint32_t count = node->children.size;
    if (count == 0) {
        return nullptr;
    }
    for (uint32_t i = 0; i < node->children.size; i++) {
        NiAVObject* child = node->children.base[i];
        if (child == nullptr) {
            continue;
        }
        auto find = reinterpret_cast<NiRefObject*(__thiscall*)(NiActorManager*, NiAVObject*)>(recurse);
        NiRefObject* found = find(self, child);
        if (found != nullptr) {
            return found;
        }
    }
    return nullptr;
}

// ---- the lists, maps and allocator

// 0x800360 NiTPointerList<unsigned int>::RemoveHead
uint32_t __fastcall UintListRemoveHead(NiTPointerList<uint32_t>* self, re::Edx) {
    NiTListNode<uint32_t>* node = UnlinkHead(*self);
    uint32_t element = node->element;
    Virtual<void>(self, 2, node);
    self->count--;
    return element;
}

// 0x800390 NiTPointerList<float>::RemoveHead
float __fastcall FloatListRemoveHead(NiTPointerList<float>* self, re::Edx) {
    NiTListNode<float>* node = UnlinkHead(*self);
    float element = node->element;
    Virtual<void>(self, 2, node);
    self->count--;
    return element;
}

// 0x800420 NiPointer<T>::operator=(object)
re::Ptr32<NiRefObject>* __fastcall PointerAssign(re::Ptr32<NiRefObject>* self, re::Edx, NiRefObject* object) {
    AssignReleasingFirst(*self, object);
    return self;
}

// 0x800470 NiTStringPointerMap<T>::GetAt(key, value out): by the string's hash, compared whole
uint8_t __fastcall StringMapGetAt(void* self, re::Edx, const char* key, re::Ptr32<void>* value) {
    uint32_t hashSize = *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(self) + 4);
    auto table = *reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(static_cast<uint8_t*>(self) + 8);
    for (uint8_t* node = table[StringHash(key, hashSize)]; node != nullptr; node = *reinterpret_cast<re::Ptr32<uint8_t>*>(node)) {
        if (crt::Strcmp(key, *reinterpret_cast<re::Ptr32<const char>*>(node + 4)) == 0) {
            *value = *reinterpret_cast<re::Ptr32<void>*>(node + 8);
            return 1;
        }
    }
    return 0;
}

// 0x8004E0 the timeline nodes' allocator: a node back on the free list, under the lock
void __cdecl TimelineDeallocate(void* node) {
    sys::Lock(TimelineAllocator_ms_kCriticalSection);
    *static_cast<re::Ptr32<uint8_t>*>(node) = TimelineAllocator_ms_pFreeList;
    TimelineAllocator_ms_pFreeList = static_cast<uint8_t*>(node);
    sys::Unlock(TimelineAllocator_ms_kCriticalSection);
}

// 0x800530 NiTPointerList<TimelineData>'s node free (slot 2)
void __fastcall TimelineListFreeNode(void*, re::Edx, void* node) {
    TimelineAllocator_Deallocate(node);
}

// 0x800540 NiTPointerList<float>'s deleting destructor
void* __fastcall FloatListDeletingDestructor(NiTPointerList<float>* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(kFloatListVtable);
    ListBase_RemoveAll(self);
    self->vtable = reinterpret_cast<void*>(kListBaseVtable);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x10);
    }
    return self;
}

// 0x800570 NiTPointerList<TimelineData>'s deleting destructor
void* __fastcall TimelineListDeletingDestructor(NiTPointerList<TimelineData>* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(kTimelineListVtable);
    ListBase_RemoveAll(self);
    self->vtable = reinterpret_cast<void*>(kListBaseVtable);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x10);
    }
    return self;
}

// 0x800990 NiTPointerList<float>::AddTail(value)
void __fastcall FloatListAddTail(NiTPointerList<float>* self, re::Edx, const float* value) {
    auto* node = Virtual<NiTListNode<float>*>(self, 1);
    float element = *value;
    node->next = nullptr;
    node->element = element;
    node->prev = self->tail;
    if (self->tail != nullptr) {
        self->tail->next = node;
        self->count++;
        self->tail = node;
        return;
    }
    self->count++;
    self->head = node;
    self->tail = node;
}

// 0x8009E0 NiTPointerList<TimelineData>::AddNewTail: a new node at the tail, its element returned
TimelineData* __fastcall TimelineAddNewTail(NiTPointerList<TimelineData>* self, re::Edx) {
    auto* node = Virtual<NiTListNode<TimelineData>*>(self, 1);
    LinkTail(*self, node);
    return &node->element;
}

// 0x800A20 NiTPointerList<TimelineData>::InsertNewBefore(position)
TimelineData* __fastcall TimelineInsertNewBefore(NiTPointerList<TimelineData>* self, re::Edx,
                                                 NiTListNode<TimelineData>* position) {
    auto* node = Virtual<NiTListNode<TimelineData>*>(self, 1);
    node->next = position;
    node->prev = position->prev;
    if (position->prev != nullptr) {
        position->prev->next = node;
        position->prev = node;
        self->count++;
        return &node->element;
    }
    self->head = node;
    position->prev = node;
    self->count++;
    return &node->element;
}

// 0x801140 a timeline node's constructor: its element's match and text key empty
NiTListNode<TimelineData>* __fastcall TimelineNodeConstruct(NiTListNode<TimelineData>* self, re::Edx) {
    self->element.match = nullptr;
    self->element.textKey = kNullFixedString;
    return self;
}

// 0x801410 the interpolator array's deleting destructor (ChangeNIFRoot's local kind)
void* __fastcall InterpArrayDeletingDestructor(NiTArray<NiRefObject*>* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(kInterpArrayVtable);
    Array_Free(self->base);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x10);
    }
    return self;
}

// 0x801440 the timeline nodes' allocator's new block (NiNew[] of ms_uiBlockSize nodes, each
// constructed): its first node links the block list, the rest the free list. (The linking loop
// runs at least once, as the original's does.)
void __cdecl TimelineCreateFreeMem() {
    uint32_t count = TimelineAllocator_ms_uiBlockSize;
    uint64_t product = uint64_t(count) * 0x1C;
    uint32_t bytes = (product >> 32) != 0 ? 0xFFFFFFFF : uint32_t(product);
    auto* block = static_cast<uint32_t*>(NiNewArray(bytes > 0xFFFFFFFB ? 0xFFFFFFFF : bytes + 4));
    uint8_t* nodes = nullptr;
    if (block != nullptr) {
        nodes = reinterpret_cast<uint8_t*>(block + 1);
        *block = count;
        VectorConstruct(nodes, 0x1C, count, reinterpret_cast<void*>(kTimelineNodeConstruct));
    }
    TimelineAllocator_ms_pFreeList = nodes;
    *reinterpret_cast<re::Ptr32<void>*>(nodes + TimelineAllocator_ms_uiBlockSize * 0x1C - 0x1C) = nullptr;
    uint32_t i = 1;
    uint32_t offset = 0x1C;
    for (;;) {
        uint8_t* list = TimelineAllocator_ms_pFreeList;
        *reinterpret_cast<re::Ptr32<uint8_t>*>(list + offset) = list + offset + 0x1C;
        uint32_t limit = TimelineAllocator_ms_uiBlockSize - 2;
        uint32_t done = i;
        i++;
        offset += 0x1C;
        if (!(done < limit)) {
            break;
        }
    }
    if (TimelineAllocator_ms_pBlockList != nullptr) {
        *re::PtrCast<re::Ptr32<void>*>(TimelineAllocator_ms_pFreeList) = TimelineAllocator_ms_pBlockList;
        TimelineAllocator_ms_pBlockList = TimelineAllocator_ms_pFreeList;
        TimelineAllocator_ms_pFreeList = TimelineAllocator_ms_pFreeList + 0x1C;
        return;
    }
    TimelineAllocator_ms_pBlockList = TimelineAllocator_ms_pFreeList;
    *re::PtrCast<re::Ptr32<void>*>(TimelineAllocator_ms_pFreeList) = nullptr;
    TimelineAllocator_ms_pFreeList = TimelineAllocator_ms_pFreeList + 0x1C;
}

// 0x801A10 the timeline nodes' allocator: a free node (a new block made when there's none), its
// links cleared, under the lock
void* __cdecl TimelineAllocate() {
    sys::Lock(TimelineAllocator_ms_kCriticalSection);
    uint8_t* node = TimelineAllocator_ms_pFreeList;
    if (node == nullptr) {
        TimelineAllocator_CreateFreeMem();
        node = TimelineAllocator_ms_pFreeList;
    }
    TimelineAllocator_ms_pFreeList = *reinterpret_cast<re::Ptr32<uint8_t>*>(node);
    reinterpret_cast<re::Ptr32<void>*>(node)[0] = nullptr;
    reinterpret_cast<re::Ptr32<void>*>(node)[1] = nullptr;
    sys::Unlock(TimelineAllocator_ms_kCriticalSection);
    return node;
}

// ---- NiControllerManager's (called with the manager as this)

// 0x8003E0 BlendFromSequence(source, destination, duration, destination key, priority, weights,
// time sync): the source's StartBlend (the manager itself unused)
uint8_t __fastcall BlendFromSequence(void*, re::Edx, void* source, void* destination, float duration,
                                     const re::Ptr32<char>* destinationKey, int32_t priority, float sourceWeight,
                                     float destinationWeight, void* timeSync) {
    return ControllerSequence_StartBlend(source, destination, duration, *destinationKey, priority, sourceWeight,
                                         destinationWeight, timeSync);
}

// 0x800AE0 DeactivateAll(ease out): each sequence deactivated
void __fastcall DeactivateAll(void* self, re::Edx, float easeOut) {
    uint8_t* manager = static_cast<uint8_t*>(self);
    for (uint32_t i = 0; i < *reinterpret_cast<uint16_t*>(manager + 0x3E); i++) {
        void* sequence = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(manager + 0x38))[i];
        if (sequence != nullptr) {
            ControllerSequence_Deactivate(sequence, easeOut, 0);
        }
    }
}

// ---- NiTextKeyMatch, the completion infos

// 0x800B20 ~NiTextKeyMatch
void __fastcall TextKeyMatchDestruct(NiTextKeyMatch* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kTextKeyMatchVtable);
    sys::FixedStringRelease(&self->matchKey);
    sys::FixedStringRelease(&self->lastMatchedKey);
    RefObjectFinish(self);
}

// 0x801210 NiTextKeyMatch's deleting destructor
NiTextKeyMatch* __fastcall TextKeyMatchDeletingDestructor(NiTextKeyMatch* self, re::Edx, uint32_t flags) {
    TextKeyMatch_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x10);
    }
    return self;
}

// 0x801230 IsKeyMatch(key) (slot 2): the key to match (the same handle); the last match
// remembered
uint8_t __fastcall TextKeyMatchIsKeyMatch(NiTextKeyMatch* self, re::Edx, const re::Ptr32<char>* key) {
    if (*key == self->matchKey) {
        FixedString_Assign(&self->lastMatchedKey, key);
        return 1;
    }
    return 0;
}

// 0x800CE0 ~ChainCompletionInfo
void __fastcall ChainCompletionInfoDestruct(ChainCompletionInfo* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kChainCompletionInfoVtable);
    sys::FixedStringRelease(&self->desName);
    sys::FixedStringRelease(&self->srcName);
    RefObjectFinish(self);
}

// 0x801250 ChainCompletionInfo's deleting destructor
ChainCompletionInfo* __fastcall ChainCompletionInfoDeletingDestructor(ChainCompletionInfo* self, re::Edx, uint32_t flags) {
    ChainCompletionInfo_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x2C);
    }
    return self;
}

// 0x8021A0 CompletionInfo(): no chain infos
CompletionInfo* __fastcall CompletionInfoConstruct(CompletionInfo* self, re::Edx) {
    RefObjectInit(self);
    self->vtable = reinterpret_cast<void*>(kCompletionInfoVtable);
    self->chainBase = nullptr;
    self->chainAllocated = 0;
    self->chainCount = 0;
    return self;
}

// 0x8021D0 ~CompletionInfo (the chain set's array freed; its references not let go)
void __fastcall CompletionInfoDestruct(CompletionInfo* self, re::Edx) {
    Array_Free(self->chainBase);
    RefObjectFinish(self);
}

// 0x802200 CompletionInfo's deleting destructor
CompletionInfo* __fastcall CompletionInfoDeletingDestructor(CompletionInfo* self, re::Edx, uint32_t flags) {
    Array_Free(self->chainBase);
    RefObjectFinish(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x28);
    }
    return self;
}

// ---- NiActorManager

// 0x8005A0 FindSequenceID(name): the ID of the sequence of that name, or INVALID
uint32_t __fastcall FindSequenceID(NiActorManager* self, re::Edx, const char* name) {
    for (MapNode* node = MapFirst(self->sequenceMap); node != nullptr;) {
        uint32_t id = node->key;
        void* sequence = node->value;
        node = MapNext(self->sequenceMap, node);
        const char* sequenceName = SeqField<const char*>(sequence, 8);
        if (sequenceName != nullptr && crt::Strcmp(name, sequenceName) == 0) {
            return id;
        }
    }
    return kInvalidSequenceID;
}

// 0x800670 ProcessCallbacks: each timeline event at or before now told to the callback object
// (and taken off, unless the callback changed the timeline's head); the timeline to rebuild
// when any went
void __fastcall ProcessCallbacks(NiActorManager* self, re::Edx) {
    if (self->callbackObject == nullptr || self->timeline.count == 0) {
        return;
    }
    bool processed = false;
    for (;;) {
        NiTListNode<TimelineData>* head = self->timeline.head;
        TimelineData* data = head != nullptr ? &head->element : nullptr;
        if (!(data->time <= self->time)) {
            if (processed) {
                self->rebuildTimeline = 1;
            }
            return;
        }
        void* callbacks = self->callbackObject;
        switch (data->eventType) {
        case 0:
            Virtual<void>(callbacks, 1, self, data->sequenceID, self->time, data->time);
            break;
        case 1:
            Virtual<void>(callbacks, 2, self, data->sequenceID, self->time, data->time);
            break;
        case 2:
            Virtual<void>(callbacks, 3, self, data->sequenceID, &data->textKey, data->match, self->time, data->time);
            break;
        case 3:
            Virtual<void>(callbacks, 4, self, data->sequenceID, self->time, data->time);
            break;
        default:
            break;
        }
        if (head == self->timeline.head) {
            NiTListNode<TimelineData>* node = UnlinkHead(self->timeline);
            Virtual<void>(&self->timeline, 2, node);
            self->timeline.count--;
        }
        processed = true;
        if (self->timeline.count == 0) {
            self->rebuildTimeline = 1;
            return;
        }
    }
}

// the activated / deactivated callbacks of one kind for a sequence (an "any" registration once
// per matching map entry)
void RaiseEvents(NiActorManager* self, uint32_t id, uint32_t eventType, int slot) {
    for (uint32_t i = 0; i < self->callbacks.size; i++) {
        CallbackData* callback = self->callbacks.base[i];
        if (callback == nullptr || callback->eventType != eventType) {
            continue;
        }
        uint32_t callbackID = callback->sequenceID;
        if (callbackID != kAnySequenceID) {
            if (callbackID == id) {
                Virtual<void>(self->callbackObject, slot, self, callbackID, self->time, self->time);
            }
            continue;
        }
        re::Ptr32<MapNode> position = MapFirst(self->sequenceMap);
        if (position == nullptr) {
            continue;
        }
        do {
            uint32_t key;
            re::Ptr32<void> value;
            Map_GetNext(&self->sequenceMap, &position, &key, &value);
            if (key == id) {
                Virtual<void>(self->callbackObject, slot, self, key, self->time, self->time);
            }
        } while (position != nullptr);
    }
}

// 0x800790 RaiseAnimActivatedEvents(id)
void __fastcall RaiseAnimActivatedEvents(NiActorManager* self, re::Edx, uint32_t id) {
    RaiseEvents(self, id, 0, 1);
}

// 0x800890 RaiseAnimDeactivatedEvents(id)
void __fastcall RaiseAnimDeactivatedEvents(NiActorManager* self, re::Edx, uint32_t id) {
    RaiseEvents(self, id, 1, 2);
}

// 0x800BA0 Reset: every sequence deactivated, no target, current or next, no transition or
// chain, the lists and the extra sequences emptied
void __fastcall Reset(NiActorManager* self, re::Edx) {
    if (self->manager != nullptr) {
        ControllerManager_DeactivateAll(self->manager, 0.0f);
    }
    self->time = -kMaxFloat;
    self->targetID = kInvalidSequenceID;
    self->nextChangeTime = kMaxFloat;
    self->rebuildTimeline = 0;
    self->transStartTime = kMaxFloat;
    self->transitionState = 0;
    self->startPending = 0;
    self->transEndTime = -kMaxFloat;
    self->startTextKey = nullptr;
    self->curID = kInvalidSequenceID;
    self->transStartFrameTime = -kMaxFloat;
    self->curSequence = nullptr;
    self->nextID = kInvalidSequenceID;
    self->chainTime = -kMaxFloat;
    self->nextSequence = nullptr;
    self->chainActive = 0;
    ListBase_RemoveAll(&self->chainIDs);
    ListBase_RemoveAll(&self->chainDurations);
    self->extraCount = 0;
    ListBase_RemoveAll(&self->timeline);
}

// 0x800C30 GetSequence(id)
void* __fastcall GetSequence(NiActorManager* self, re::Edx, uint32_t id) {
    return MapLookup(self->sequenceMap, id);
}

// 0x800C70 TimelineData::SetValues(time, type, id, match, text key)
void __fastcall TimelineDataSetValues(TimelineData* self, re::Edx, float time, uint32_t eventType, uint32_t id,
                                      NiTextKeyMatch* match, const re::Ptr32<char>* textKey) {
    self->time = time;
    self->eventType = eventType;
    self->sequenceID = id;
    AssignReleasingFirst(self->match, match);
    FixedString_Assign(&self->textKey, textKey);
}

// 0x800D60 _SDMInit: the end and start text keys
void __cdecl SDMInit() {
    FixedString_AssignText(&ms_kEndTextKey, reinterpret_cast<const char*>(0x00901780));     // "end"
    FixedString_AssignText(&ms_kStartTextKey, reinterpret_cast<const char*>(0x00965A74));   // "start"
}

// 0x800D80 _SDMShutdown
void __cdecl SDMShutdown() {
    FixedString_AssignText(&ms_kEndTextKey, nullptr);
    FixedString_AssignText(&ms_kStartTextKey, nullptr);
}

// 0x800DA0 FindControllerManager(object). (The object's controllers are asked before it's
// checked for null, as the original does.)
NiRefObject* __fastcall FindControllerManager(NiActorManager* self, re::Edx, NiAVObject* object) {
    return FindController(self, object, kNiControllerManager_RTTI, 0x00800DA0);
}

// 0x800E30 FindBoneLODController(object)
NiRefObject* __fastcall FindBoneLODController(NiActorManager* self, re::Edx, NiAVObject* object) {
    return FindController(self, object, kNiBoneLODController_RTTI, 0x00800E30);
}

// 0x800EC0 UnloadSequence(id): reset first when it's playing or wanted (or an extra one); the
// sequence taken out of the manager and the map
void __fastcall UnloadSequence(NiActorManager* self, re::Edx, uint32_t id) {
    MapNode* node = self->sequenceMap.table[id % self->sequenceMap.hashSize];
    while (node != nullptr && id != node->key) {
        node = node->next;
    }
    if (node == nullptr || node->value == nullptr) {
        return;
    }
    void* sequence = node->value;
    int32_t index = -1;
    for (uint32_t i = 0; i < self->extraCount; i++) {
        if (self->extraSequences[i] == id) {
            index = int32_t(i);
            break;
        }
    }
    if (id == self->curID || id == self->nextID || id == self->targetID || index > -1) {
        ActorManager_Reset(self);
    }
    re::Ptr32<NiRefObject> removed;
    ControllerManager_RemoveSequence(self->manager, &removed, sequence);
    ReleaseImport(removed);
    Map_RemoveAt(&self->sequenceMap, id);
}

// 0x8010C0 RemoveInactiveExtraSequences: each extra sequence no longer playing dropped (its slot
// filled with the last, the scan restarted). (A sequence missing from the map is read through
// null, as the original does.)
void __fastcall RemoveInactiveExtraSequences(NiActorManager* self, re::Edx) {
    uint32_t i = 0;
    while (i < self->extraCount) {
        void* sequence = MapLookup(self->sequenceMap, self->extraSequences[i]);
        if (SeqField<int32_t>(sequence, 0x44) == 0) {
            self->extraCount--;
            self->extraSequences[i] = self->extraSequences[self->extraCount];
            i = 0;
        } else {
            i++;
        }
    }
}

// 0x801270 ~NiActorManager
void __fastcall Destruct(NiActorManager* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kActorManagerVtable);
    NiFree(self->extraSequences);
    self->timeline.vtable = reinterpret_cast<void*>(kTimelineListVtable);
    ListBase_RemoveAll(&self->timeline);
    self->timeline.vtable = reinterpret_cast<void*>(kListBaseVtable);
    self->callbacks.vtable = reinterpret_cast<void*>(kArrayBaseVtable);
    Array_Free(self->callbacks.base);
    self->sequenceMap.vtable = reinterpret_cast<void*>(kMapVtable);
    Map_RemoveAll(&self->sequenceMap);
    self->sequenceMap.vtable = reinterpret_cast<void*>(kMapBaseVtable);
    Map_RemoveAll(&self->sequenceMap);
    NiFree(self->sequenceMap.table);
    self->chainDurations.vtable = reinterpret_cast<void*>(kFloatListVtable);
    ListBase_RemoveAll(&self->chainDurations);
    self->chainDurations.vtable = reinterpret_cast<void*>(kListBaseVtable);
    self->chainIDs.vtable = reinterpret_cast<void*>(kChainIDListDtorVtable);
    ListBase_RemoveAll(&self->chainIDs);
    self->chainIDs.vtable = reinterpret_cast<void*>(kListBaseVtable);
    ReleaseImport(self->boneLOD);
    ReleaseImport(self->manager);
    ReleaseImport(re::PtrCast<NiRefObject*>(self->nifRoot));
    ReleaseImport(self->kfmTool);
    RefObjectFinish(self);
}

// 0x801390 ChangeSequence(id, sequence): the old one unloaded, the new one added to the manager
// (under no name) and, when that worked, mapped
uint8_t __fastcall ChangeSequence(NiActorManager* self, re::Edx, uint32_t id, void* sequence) {
    ActorManager_UnloadSequence(self, id);
    re::Ptr32<char> name = kNullFixedString;
    uint8_t added = ControllerManager_AddSequence(self->manager, sequence, &name, 1);
    sys::FixedStringRelease(&name);
    if (added != 0) {
        Map_SetAt(&self->sequenceMap, id, sequence);
    }
    return added;
}

// 0x801540 NiActorManager(KFM tool, cumulative animations): the tool held, everything else
// empty, then Reset; three callback slots
NiActorManager* __fastcall Construct(NiActorManager* self, re::Edx, NiKFMTool* kfmTool, int cumulative) {
    RefObjectInit(self);
    self->vtable = reinterpret_cast<void*>(kActorManagerVtable);
    self->kfmTool = kfmTool;
    if (kfmTool != nullptr) {
        Increment(&kfmTool->refCount);
    }
    self->nifRoot = nullptr;
    self->manager = nullptr;
    self->boneLOD = nullptr;
    self->callbackObject = nullptr;
    self->chainIDs.count = 0;
    self->chainIDs.head = nullptr;
    self->chainIDs.tail = nullptr;
    self->chainIDs.vtable = reinterpret_cast<void*>(kChainIDListVtable);
    self->chainDurations.count = 0;
    self->chainDurations.head = nullptr;
    self->chainDurations.tail = nullptr;
    self->chainDurations.vtable = reinterpret_cast<void*>(kFloatListVtable);
    self->cumulative = uint8_t(cumulative);
    self->sequenceMap.vtable = reinterpret_cast<void*>(kMapBaseVtable);
    self->sequenceMap.hashSize = 0x25;
    self->sequenceMap.count = 0;
    self->sequenceMap.table = static_cast<re::Ptr32<MapNode>*>(NiMalloc(0x94));
    crt::Memset(self->sequenceMap.table, 0, self->sequenceMap.hashSize * 4);
    self->sequenceMap.vtable = reinterpret_cast<void*>(kMapVtable);
    self->callbacks.maxSize = 0;
    self->callbacks.growBy = 1;
    self->callbacks.size = 0;
    self->callbacks.esize = 0;
    self->callbacks.base = nullptr;
    self->callbacks.vtable = reinterpret_cast<void*>(kCallbackArrayVtable);
    self->timeline.count = 0;
    self->timeline.head = nullptr;
    self->timeline.tail = nullptr;
    self->timeline.vtable = reinterpret_cast<void*>(kTimelineListVtable);
    self->extraSequences = nullptr;
    self->extraAllocated = 0;
    self->extraCount = 0;
    ActorManager_Reset(self);
    Array_SetSize(&self->callbacks, 3);
    self->callbacks.growBy = 3;
    return self;
}

// 0x801660 the deleting destructor
NiActorManager* __fastcall DeletingDestructor(NiActorManager* self, re::Edx, uint32_t flags) {
    ActorManager_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0xB4);
    }
    return self;
}

// 0x801690 LoadSequence(id, stream, load the KF file): the tool's sequence's file (loaded into the
// stream when asked), the sequence made from it and changed to
uint8_t __fastcall LoadSequence(NiActorManager* self, re::Edx, uint32_t id, void* stream, int loadKFFile) {
    if (self->manager == nullptr) {
        return 0;
    }
    NiKFMTool* kfmTool = self->kfmTool;
    MapNode* node = kfmTool->sequences.table[id % kfmTool->sequences.hashSize];
    while (node != nullptr && id != node->key) {
        node = node->next;
    }
    if (node == nullptr || node->value == nullptr) {
        return 0;
    }
    auto* info = static_cast<Sequence*>(node->value);
    re::Ptr32<char> filename;
    KFMTool_GetFullKFFilename(kfmTool, &filename, id);
    if (re::Flag(loadKFFile) && Virtual<uint8_t>(stream, 3, filename) == 0) {
        FixedString_Destruct(&filename);
        return 0;
    }
    re::Ptr32<NiRefObject> sequence;
    ControllerSequence_CreateFromFile(&sequence, stream, info->animIndex);
    if (sequence == nullptr) {
        FixedString_Destruct(&filename);
        return 0;
    }
    uint8_t changed = ActorManager_ChangeSequence(self, id, sequence);
    ReleaseImport(sequence);
    FixedString_Destruct(&filename);
    return changed;
}

// 0x801780 Clone(cloning process): the NIF root cloned (an exact copy; a process of its own when
// none's given), a new manager on the tool with the clone's controller manager and bone LOD
// controller, the same callback object, the sequences mapped to the clone manager's of the same
// names, the callbacks shared
NiActorManager* __fastcall Clone(NiActorManager* self, re::Edx, void* cloning) {
    bool own = false;
    if (cloning == nullptr) {
        void* memory = NiNew(0x18);
        cloning = memory != nullptr ? CloningProcess_Construct(memory, 0x25) : nullptr;
        own = true;
    }
    *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(cloning) + 8) = 1;
    NiAVObject* root = Object_Clone(self->nifRoot, cloning);
    if (own) {
        re::Ptr32<void>* maps = static_cast<re::Ptr32<void>*>(cloning);
        if (maps[0] != nullptr) {
            Virtual<void>(maps[0], 0, 1u);
        }
        if (maps[1] != nullptr) {
            Virtual<void>(maps[1], 0, 1u);
        }
        NiDelete(cloning, 0x18);
    }
    NiRefObject* manager = ActorManager_FindControllerManager(self, root);
    void* memory = NiNew(0xB4);
    NiActorManager* clone =
        memory != nullptr ? ActorManager_Construct(memory, self->kfmTool, self->cumulative) : nullptr;
    AssignReleasingFirst(clone->nifRoot, root);
    AssignReleasingFirst(clone->manager, manager);
    AssignReleasingFirst(clone->boneLOD, ActorManager_FindBoneLODController(self, clone->nifRoot));
    clone->callbackObject = self->callbackObject;
    for (MapNode* node = MapFirst(self->sequenceMap); node != nullptr;) {
        uint32_t id = node->key;
        void* sequence = node->value;
        node = MapNext(self->sequenceMap, node);
        uint8_t* cloneManager = re::PtrCast<uint8_t*>(clone->manager);
        re::Ptr32<void> index;
        void* cloneSequence = nullptr;
        if (StringMap_GetAt(cloneManager + 0x44, SeqField<const char*>(sequence, 8), &index) != 0) {
            cloneSequence = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(cloneManager + 0x38))[re::PtrCast<uintptr_t>(index)];
        }
        Map_SetAt(&clone->sequenceMap, id, cloneSequence);
    }
    for (uint32_t i = 0; i < self->callbacks.size; i++) {
        CallbackData* callback = self->callbacks.base[i];
        if (callback == nullptr) {
            continue;
        }
        re::Ptr32<NiRefObject> held = callback;
        Increment(&callback->refCount);
        ObjectArray_Add(&clone->callbacks, &held);
        ReleaseImport(callback);
    }
    return clone;
}

// 0x801CF0 LoadAllSequences(stream): each of the tool's sequences loaded (a KF file read only
// when it differs from the last one's); true when all of them loaded. A stream of its own when
// none's given
uint8_t __fastcall LoadAllSequences(NiActorManager* self, re::Edx, void* stream) {
    bool own = false;
    if (stream == nullptr) {
        void* memory = NiNew(0x40C);
        stream = memory != nullptr ? Stream_Construct(memory) : nullptr;
        own = true;
    }
    uint8_t success = 1;
    re::Ptr32<uint32_t> ids;
    uint32_t count;
    KFMTool_GetSequenceIDs(self->kfmTool, &ids, &count);
    char lastPath[0x104];
    lastPath[0] = 0;
    for (uint32_t i = 0; i < count; i++) {
        uint32_t id = ids[i];
        re::Ptr32<char> filename;
        KFMTool_GetFullKFFilename(self->kfmTool, &filename, id);
        int load = 0;
        if (crt::Strcmp(lastPath, filename) != 0) {
            load = 1;
            crt::StrcpyS(lastPath, 0x104, filename);
        }
        if (ActorManager_LoadSequence(self, id, stream, load) == 0) {
            success = 0;
        }
        sys::FixedStringRelease(&filename);
    }
    NiFree(ids);
    if (own && stream != nullptr) {
        Virtual<void>(stream, 0, 1u);
    }
    return success;
}

// 0x801E80 ChangeNIFRoot(root): the tool's model root found in it (slot 23 GetObjectByName), a
// new controller manager on that; the old manager's sequences moved over with their
// interpolators kept (removing them from the old one would let them go); root, manager and bone
// LOD controller taken
uint8_t __fastcall ChangeNIFRoot(NiActorManager* self, re::Edx, NiAVObject* root) {
    if (root == nullptr) {
        return 0;
    }
    const char* modelRoot = self->kfmTool->modelRoot;
    if (modelRoot == nullptr) {
        return 0;
    }
    re::Ptr32<char> name = StringTable_AddString(modelRoot);
    NiAVObject* actorRoot = Virtual<NiAVObject*>(root, 23, &name);
    sys::FixedStringRelease(&name);
    if (actorRoot == nullptr) {
        return 0;
    }
    ActorManager_Reset(self);
    void* memory = NiNew(0x6C);
    void* newManager = memory != nullptr ? ControllerManager_Construct(memory, actorRoot, self->cumulative) : nullptr;
    if (self->manager != nullptr) {
        NiTArray<NiRefObject*> interpolators;
        interpolators.maxSize = 0;
        interpolators.growBy = 1;
        interpolators.size = 0;
        interpolators.esize = 0;
        interpolators.base = nullptr;
        interpolators.vtable = reinterpret_cast<void*>(kInterpArrayVtable);
        for (uint32_t i = 0; i < *reinterpret_cast<uint16_t*>(re::PtrCast<uint8_t*>(self->manager) + 0x3E); i++) {
            uint8_t* manager = re::PtrCast<uint8_t*>(self->manager);
            auto* sequence = static_cast<NiRefObject*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(manager + 0x38))[i]);
            if (sequence == nullptr) {
                continue;
            }
            Increment(&sequence->refCount);
            uint32_t count = SeqField<uint32_t>(sequence, 0xC);
            Array_RemoveAll(&interpolators);
            if (count > interpolators.maxSize) {
                Array_SetSize(&interpolators, count);
            }
            uint8_t* items = SeqField<uint8_t*>(sequence, 0x14);   // 0x10-byte items, the interpolator at +4
            for (uint32_t j = 0; j < count; j++) {
                NiRefObject* interpolator = *reinterpret_cast<re::Ptr32<NiRefObject>*>(items + j * 0x10 + 4);
                re::Ptr32<NiRefObject> held = interpolator;
                if (interpolator != nullptr) {
                    Increment(&interpolator->refCount);
                }
                uint32_t index = interpolators.size;
                if (index >= interpolators.maxSize) {
                    Array_SetSize(&interpolators, interpolators.growBy + index);
                }
                Array_SetAt(&interpolators, index, &held);
                ReleaseImport(interpolator);
            }
            re::Ptr32<NiRefObject> removed;
            ControllerManager_RemoveSequence(self->manager, &removed, sequence);
            ReleaseImport(removed);
            for (uint32_t j = 0; j < count; j++) {
                items = SeqField<uint8_t*>(sequence, 0x14);
                AssignReleasingFirst(*reinterpret_cast<re::Ptr32<NiRefObject>*>(items + j * 0x10 + 4), interpolators.base[j]);
            }
            re::Ptr32<char> none = kNullFixedString;
            ControllerManager_AddSequence(newManager, sequence, &none, 1);
            sys::FixedStringRelease(&none);
            ReleaseImport(sequence);
        }
        Array_Free(interpolators.base);
    }
    Pointer_Assign(reinterpret_cast<re::Ptr32<NiRefObject>*>(&self->nifRoot), reinterpret_cast<NiRefObject*>(root));
    Pointer_Assign(&self->manager, static_cast<NiRefObject*>(newManager));
    Pointer_Assign(&self->boneLOD, ActorManager_FindBoneLODController(self, self->nifRoot));
    return 1;
}

// 0x803590 Initialize(stream): the tool's model loaded into the stream, its first object (a
// scene graph) the NIF root, then every sequence
uint8_t __fastcall Initialize(NiActorManager* self, re::Edx, void* stream) {
    if (self->kfmTool->modelPath == nullptr) {
        return 0;
    }
    char* path = *KFMTool_GetFullModelPath(self->kfmTool);
    if (Virtual<uint8_t>(stream, 3, path) == 0) {
        return 0;
    }
    auto top = **reinterpret_cast<re::Ptr32<re::Ptr32<NiAVObject>>*>(static_cast<uint8_t*>(stream) + 0x15C);
    if (top == nullptr || !KindOf(top, NiAVObject_ClassIndex)) {
        return 0;
    }
    if (ActorManager_ChangeNIFRoot(self, top) == 0) {
        return 0;
    }
    ActorManager_LoadAllSequences(self, stream);
    return 1;
}

// 0x803970 Create(KFM tool, KFM file path, cumulative animations, load the files, stream): the
// tool's base path the file's directory, a manager on the tool, initialized when asked (a stream
// of its own when none's given; on failure the manager deleted, none returned)
NiActorManager* __cdecl Create(NiKFMTool* kfmTool, const char* kfmPath, int cumulative, int loadFiles, void* stream) {
    alignas(4) uint8_t filename[0x404];
    Filename_Construct(filename, kfmPath);
    char buffer[0x104];
    NiSprintf(buffer, 0x104, "%s%s", reinterpret_cast<char*>(filename + 0x100), reinterpret_cast<char*>(filename));
    re::Ptr32<char> basePath = StringTable_AddString(buffer);
    FixedString_Assign(&kfmTool->basePath, &basePath);
    sys::FixedStringRelease(&basePath);
    void* memory = NiNew(0xB4);
    NiActorManager* manager = memory != nullptr ? ActorManager_Construct(memory, kfmTool, cumulative) : nullptr;
    if (re::Flag(loadFiles)) {
        bool own = false;
        if (stream == nullptr) {
            void* streamMemory = NiNew(0x40C);
            stream = streamMemory != nullptr ? Stream_Construct(streamMemory) : nullptr;
            own = true;
        }
        if (ActorManager_Initialize(manager, stream) == 0) {
            if (manager != nullptr) {
                Virtual<void>(manager, 0, 1u);
            }
            return nullptr;
        }
        if (own && stream != nullptr) {
            Virtual<void>(stream, 0, 1u);
        }
    }
    return manager;
}

// 0x803AE0 AddTimelineDataObject(type, sequence, match): the event's next time put on the
// timeline in time order (before the first later one), with the match's last matched key
void __fastcall AddTimelineDataObject(NiActorManager* self, re::Edx, uint32_t eventType, uint32_t id,
                                      NiTextKeyMatch* match) {
    float time = ActorManager_GetNextEventTime(self, eventType, id, match);
    if (double(time) == -3.4028234663852886e+38) {
        return;
    }
    NiTListNode<TimelineData>* node = self->timeline.head;
    while (node != nullptr && !(node->element.time > time)) {
        node = node->next;
    }
    TimelineData* data = node != nullptr ? Timeline_InsertNewBefore(&self->timeline, node)
                                         : Timeline_AddNewTail(&self->timeline);
    re::Ptr32<char> key = (match != nullptr && match->lastMatchedKey != nullptr) ? StringTable_AddString(match->lastMatchedKey)
                                                                        : static_cast<char*>(kNullFixedString);
    TimelineData_SetValues(data, time, eventType, id, match, &key);
    sys::FixedStringRelease(&key);
}

// 0x803BF0 BuildTimeline: the extra sequences tidied, the timeline remade from the callbacks (an
// "any" one for every mapped sequence), the next change at its last event (FLT_MAX for none)
void __fastcall BuildTimeline(NiActorManager* self, re::Edx) {
    ActorManager_RemoveInactiveExtraSequences(self);
    ListBase_RemoveAll(&self->timeline);
    for (uint32_t i = 0; i < self->callbacks.size; i++) {
        CallbackData* callback = self->callbacks.base[i];
        if (callback == nullptr) {
            continue;
        }
        uint32_t callbackID = callback->sequenceID;
        if (callbackID != kAnySequenceID) {
            NiTextKeyMatch* match = callback->match;
            if (match != nullptr) {
                Increment(&match->refCount);
            }
            ActorManager_AddTimelineDataObject(self, callback->eventType, callbackID, match);
            ReleaseImport(match);
            continue;
        }
        for (MapNode* node = MapFirst(self->sequenceMap); node != nullptr;) {
            uint32_t id = node->key;
            node = MapNext(self->sequenceMap, node);
            NiTextKeyMatch* match = callback->match;
            if (match != nullptr) {
                Increment(&match->refCount);
            }
            ActorManager_AddTimelineDataObject(self, callback->eventType, id, match);
            ReleaseImport(match);
        }
    }
    if (self->timeline.tail != nullptr) {
        self->nextChangeTime = self->timeline.tail->element.time;
        return;
    }
    self->nextChangeTime = kMaxFloat;
}

} // namespace

// 0x801CE0: jmp 0x801A10 (NiTPointerList<TimelineData>'s node allocation, slot 1)
RE_UNHOOKED("NiTPointerList<NiActorManager::TimelineData>::NewNode", 0x00801CE0);

} // namespace ni::actor

// ---- registration ---------------------------------------------------------------------

#define NIACTORMGR_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

using namespace ni::actor;

NIACTORMGR_REVERSED("NiTPointerList<unsigned int>::RemoveHead_800360", 0x00800360, UintListRemoveHead);
NIACTORMGR_REVERSED("NiTPointerList<float>::RemoveHead", 0x00800390, FloatListRemoveHead);
NIACTORMGR_REVERSED("NiControllerManager::BlendFromSequence", 0x008003E0, BlendFromSequence);
NIACTORMGR_REVERSED("NiPointer::operator=_800420", 0x00800420, PointerAssign);
NIACTORMGR_REVERSED("NiTStringPointerMap::GetAt_800470", 0x00800470, StringMapGetAt);
NIACTORMGR_REVERSED("NiTObjectAllocator<NiActorManager::TimelineData>::Deallocate", 0x008004E0, TimelineDeallocate);
NIACTORMGR_REVERSED("NiTPointerList<NiActorManager::TimelineData>::FreeNode", 0x00800530, TimelineListFreeNode);
NIACTORMGR_REVERSED("NiTPointerList<float>::DeletingDestructor", 0x00800540, FloatListDeletingDestructor);
NIACTORMGR_REVERSED("NiTPointerList<NiActorManager::TimelineData>::DeletingDestructor", 0x00800570,
                    TimelineListDeletingDestructor);
NIACTORMGR_REVERSED("NiActorManager::FindSequenceID", 0x008005A0, FindSequenceID);
NIACTORMGR_REVERSED("NiActorManager::ProcessCallbacks", 0x00800670, ProcessCallbacks);
NIACTORMGR_REVERSED("NiActorManager::RaiseAnimActivatedEvents", 0x00800790, RaiseAnimActivatedEvents);
NIACTORMGR_REVERSED("NiActorManager::RaiseAnimDeactivatedEvents", 0x00800890, RaiseAnimDeactivatedEvents);
NIACTORMGR_REVERSED("NiTPointerList<float>::AddTail", 0x00800990, FloatListAddTail);
NIACTORMGR_REVERSED("NiTPointerList<NiActorManager::TimelineData>::AddNewTail", 0x008009E0, TimelineAddNewTail);
NIACTORMGR_REVERSED("NiTPointerList<NiActorManager::TimelineData>::InsertNewBefore", 0x00800A20,
                    TimelineInsertNewBefore);
NIACTORMGR_REVERSED("NiControllerManager::DeactivateAll", 0x00800AE0, DeactivateAll);
NIACTORMGR_REVERSED("NiTextKeyMatch::~NiTextKeyMatch", 0x00800B20, TextKeyMatchDestruct);
NIACTORMGR_REVERSED("NiActorManager::Reset", 0x00800BA0, Reset);
NIACTORMGR_REVERSED("NiActorManager::GetSequence", 0x00800C30, GetSequence);
NIACTORMGR_REVERSED("NiActorManager::TimelineData::SetValues", 0x00800C70, TimelineDataSetValues);
NIACTORMGR_REVERSED("NiActorManager::ChainCompletionInfo::~ChainCompletionInfo", 0x00800CE0,
                    ChainCompletionInfoDestruct);
NIACTORMGR_REVERSED("NiActorManager::_SDMInit", 0x00800D60, SDMInit);
NIACTORMGR_REVERSED("NiActorManager::_SDMShutdown", 0x00800D80, SDMShutdown);
NIACTORMGR_REVERSED("NiActorManager::FindControllerManager", 0x00800DA0, FindControllerManager);
NIACTORMGR_REVERSED("NiActorManager::FindBoneLODController", 0x00800E30, FindBoneLODController);
NIACTORMGR_REVERSED("NiActorManager::UnloadSequence", 0x00800EC0, UnloadSequence);
NIACTORMGR_REVERSED("NiActorManager::RemoveInactiveExtraSequences", 0x008010C0, RemoveInactiveExtraSequences);
NIACTORMGR_REVERSED("NiActorManager::TimelineNode::TimelineNode", 0x00801140, TimelineNodeConstruct);
NIACTORMGR_REVERSED("NiTextKeyMatch::DeletingDestructor", 0x00801210, TextKeyMatchDeletingDestructor);
NIACTORMGR_REVERSED("NiTextKeyMatch::IsKeyMatch", 0x00801230, TextKeyMatchIsKeyMatch);
NIACTORMGR_REVERSED("NiActorManager::ChainCompletionInfo::DeletingDestructor", 0x00801250,
                    ChainCompletionInfoDeletingDestructor);
NIACTORMGR_REVERSED("NiActorManager::~NiActorManager", 0x00801270, Destruct);
NIACTORMGR_REVERSED("NiActorManager::ChangeSequence", 0x00801390, ChangeSequence);
NIACTORMGR_REVERSED("NiTArray<NiInterpolator*>::DeletingDestructor_801410", 0x00801410, InterpArrayDeletingDestructor);
NIACTORMGR_REVERSED("NiTObjectAllocator<NiActorManager::TimelineData>::CreateFreeMem", 0x00801440,
                    TimelineCreateFreeMem);
NIACTORMGR_REVERSED("NiActorManager::NiActorManager", 0x00801540, Construct);
NIACTORMGR_REVERSED("NiActorManager::DeletingDestructor", 0x00801660, DeletingDestructor);
NIACTORMGR_REVERSED("NiActorManager::LoadSequence", 0x00801690, LoadSequence);
NIACTORMGR_REVERSED("NiActorManager::Clone", 0x00801780, Clone);
NIACTORMGR_REVERSED("NiTObjectAllocator<NiActorManager::TimelineData>::Allocate", 0x00801A10, TimelineAllocate);
NIACTORMGR_REVERSED("NiActorManager::LoadAllSequences", 0x00801CF0, LoadAllSequences);
NIACTORMGR_REVERSED("NiActorManager::ChangeNIFRoot", 0x00801E80, ChangeNIFRoot);
NIACTORMGR_REVERSED("NiActorManager::CompletionInfo::CompletionInfo", 0x008021A0, CompletionInfoConstruct);
NIACTORMGR_REVERSED("NiActorManager::CompletionInfo::~CompletionInfo", 0x008021D0, CompletionInfoDestruct);
NIACTORMGR_REVERSED("NiActorManager::CompletionInfo::DeletingDestructor", 0x00802200, CompletionInfoDeletingDestructor);
NIACTORMGR_REVERSED("NiActorManager::Initialize", 0x00803590, Initialize);
NIACTORMGR_REVERSED("NiActorManager::Create", 0x00803970, Create);
NIACTORMGR_REVERSED("NiActorManager::AddTimelineDataObject", 0x00803AE0, AddTimelineDataObject);
NIACTORMGR_REVERSED("NiActorManager::BuildTimeline", 0x00803BF0, BuildTimeline);
