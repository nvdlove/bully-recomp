// NiControllerManager (vtable 0x966D88, 0x6C bytes: src/gamebryo/NiControllerSequence.h): the
// time controller owning a target tree's sequences (by name: the index of each in the array),
// updating the active ones each frame; their cross-fades and morphs, adding and removing them,
// streaming, cloning (the sequences cloned and bound again, the blends' temporary sequences
// mapped), comparison, construction; and the NiTObjectArray<NiPointer> RemoveAt the code reaches
// here (the game's ModelBuilder code too).
//
// Main-thread float maths at the game's 24-bit x87 precision.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiControllerSequence.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiInterpolators.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::anim {

namespace {

using scene::LoadBytes;
using scene::SaveBytes;
using scene::Stream_GetNumberOfLinkIDs;
using scene::Stream_ReadMultipleLinkIDs;
using scene::Stream_ResolveLinkID;
using scene::StreamVersion;
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiControllerManager_RTTI, 0x020E50D0);

constexpr uintptr_t NiControllerManager_vtable = 0x00966D88;
constexpr uintptr_t SequenceArray_vtable = 0x009893C0;   // NiTObjectArray<NiControllerSequencePtr>
constexpr uintptr_t MapBase_vtable = 0x00950760;         // NiTMapBase's
constexpr uintptr_t StringIndexMap_vtable = 0x00951798;  // NiTStringMap<unsigned int>'s
constexpr uint32_t kVersion_10_1_0_104 = 0x0A010068;

GAME_FN(void, __cdecl, NiDeletePointerArray, 0x00836F80, void* p);
GAME_FN(NiObject*, __thiscall, Stream_ReadLinkIDObject, 0x0075A820, void* stream);
GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<const char>* self, const re::Ptr32<const char>* other);
GAME_FN(void, __thiscall, TimeController_Construct, 0x0077DFA0, void* self);
GAME_FN(void, __thiscall, TimeController_Destruct, 0x0077E010, void* self);
GAME_FN(void, __thiscall, TimeController_SetTarget, 0x0077E050, void* self, NiObjectNET* target);
GAME_FN(uint8_t, __thiscall, TimeController_RegisterStreamables, 0x0077DF70, void* self, void* stream);
GAME_FN(void, __thiscall, TimeController_LoadBinary, 0x0077E1F0, void* self, void* stream);
GAME_FN(void, __thiscall, TimeController_LinkObject, 0x0077DF10, void* self, void* stream);
GAME_FN(void, __thiscall, TimeController_SaveBinary, 0x0077E2F0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, TimeController_IsEqual, 0x0077DB90, void* self, void* other);
GAME_FN(void, __thiscall, TimeController_CopyMembers, 0x0077E100, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, TimeController_ProcessClone, 0x0077DEE0, void* self, void* cloning);
GAME_FN(void, __thiscall, Object_PostLinkObject, 0x008A20A0, void* self, void* stream);
// the containers' (the next chunks')
GAME_FN(void, __thiscall, PointerArray_RemoveAll, 0x0040E7A0, void* array);
GAME_FN(void, __thiscall, PointerArray_SetSize, 0x00856170, void* array, uint32_t size);
GAME_FN(uint32_t, __thiscall, ObjectArray_AddFirstEmpty, 0x007606C0, void* array, const re::Ptr32<NiRefObject>* value);
GAME_FN(void, __thiscall, ObjectSet_Add, 0x00787BE0, void* set, const re::Ptr32<NiRefObject>* element);
GAME_FN(void, __thiscall, ObjectSet_Realloc, 0x007E8680, void* set, uint32_t allocated);
GAME_FN(void*, __thiscall, StringMap_GetFirstPos, 0x008CA4E0, StringIndexMap* map);
GAME_FN(void, __thiscall, StringMap_GetNext, 0x0083D890, StringIndexMap* map, re::Ptr32<void>* position, re::Ptr32<const char>* key,
        uint32_t* value);
GAME_FN(uint8_t, __thiscall, StringMap_GetAt, 0x00800470, StringIndexMap* map, const char* key, uint32_t* value);
GAME_FN(void, __thiscall, StringMap_SetAt, 0x00747F00, StringIndexMap* map, const char* key, uintptr_t value);
GAME_FN(void, __thiscall, StringMap_RemoveAt, 0x008A1F90, StringIndexMap* map, const char* key);
GAME_FN(void, __thiscall, StringMap_Destruct, 0x0075A4E0, StringIndexMap* map);
GAME_FN(NiObject*, __thiscall, DefaultPalette_Construct, 0x007DE780, void* self, NiAVObject* scene);
// NiControllerSequence's (this chunk's)
GAME_FN(void, __thiscall, Sequence_Update, 0x00816320, NiControllerSequence* self, float time,
        int updateInterpolators);
GAME_FN(uint8_t, __thiscall, Sequence_StoreTargets, 0x00816580, NiControllerSequence* self, NiAVObject* root);
GAME_FN(void, __thiscall, Sequence_ClearTargets, 0x00813950, NiControllerSequence* self);
GAME_FN(uint8_t, __thiscall, Sequence_Activate, 0x00814940, NiControllerSequence* self, int priority, int startOver,
        float weight, float easeInTime, NiControllerSequence* timeSync, int transition);
GAME_FN(uint8_t, __thiscall, Sequence_Deactivate, 0x00814A20, NiControllerSequence* self, float easeOutTime,
        int transition);
GAME_FN(uint8_t, __thiscall, Sequence_StartMorph, 0x00814BC0, NiControllerSequence* self,
        NiControllerSequence* destination, float duration, int priority, float sourceWeight, float destWeight);
// (this file's, called where modes apply)
GAME_FN(re::Ptr32<NiRefObject>*, __thiscall, ObjectArray_RemoveAt, 0x00818530, NiTArray<NiRefObject*>* self,
        re::Ptr32<NiRefObject>* result, uint32_t index);
GAME_FN(void, __thiscall, Manager_Destruct, 0x00818880, NiControllerManager* self);
GAME_FN(NiControllerManager*, __thiscall, Manager_Construct, 0x00818900, void* self);
GAME_FN(uint8_t, __thiscall, Manager_AddSequence, 0x00818A90, NiControllerManager* self, NiControllerSequence* sequence,
        const re::Ptr32<const char>* name, int storeTargets);
GAME_FN(void, __thiscall, Manager_CopyMembers, 0x00818B40, NiControllerManager* self, NiControllerManager* dest,
        void* cloning);

inline void FixedRelease(re::Ptr32<const char>* handle) {
    sys::FixedStringRelease(re::PtrCast<re::Ptr32<char>*>(handle));
}

inline void Hold(void* object) {
    if (object != nullptr) {
        sys::Increment(&static_cast<NiRefObject*>(object)->refCount);
    }
}

// the clone of an object in the cloning's map, or null
inline const re::Ptr32<void>* FindClone(void* cloning, const void* object) {
    uint8_t* map = *static_cast<re::Ptr32<uint8_t>*>(cloning);
    uint32_t count = *reinterpret_cast<uint32_t*>(map + 4);
    re::Ptr32<void>* buckets = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(map + 8);
    for (re::Ptr32<void>* node = static_cast<re::Ptr32<void>*>(buckets[reinterpret_cast<uintptr_t>(object) % count]); node != nullptr;
         node = static_cast<re::Ptr32<void>*>(node[0])) {
        if (node[1] == object) {
            return &node[2];
        }
    }
    return nullptr;
}

// the members both constructors set (cumulative given)
inline void ConstructMembers(NiControllerManager* self, uint8_t cumulative) {
    TimeController_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiControllerManager_vtable);
    self->sequences.maxSize = 0;
    self->sequences.growBy = 1;
    self->sequences.size = 0;
    self->sequences.esize = 0;
    self->sequences.base = nullptr;
    self->sequences.vtable = reinterpret_cast<void*>(SequenceArray_vtable);
    self->sequenceMap.vtable = reinterpret_cast<void*>(MapBase_vtable);
    self->sequenceMap.hashSize = 37;
    self->sequenceMap.count = 0;
    re::Ptr32<void>* table = static_cast<re::Ptr32<void>*>(NiMalloc(0x94));
    self->sequenceMap.table = table;
    crt::Memset(table, 0, self->sequenceMap.hashSize * 4);
    self->sequenceMap.copy = 0;
    self->sequenceMap.vtable = reinterpret_cast<void*>(StringIndexMap_vtable);
    self->cumulative = cumulative;
    self->tempBlendSeqs = nullptr;
    self->tempBlendAllocated = 0;
    self->tempBlendCount = 0;
    self->objectPalette = nullptr;
}

// ---- NiControllerManager

// 0x8182E0: Morph(source, destination, duration, priority, sourceWeight, destWeight)
uint8_t __fastcall Mgr_Morph(NiControllerManager*, re::Edx, NiControllerSequence* source,
                             NiControllerSequence* destination, float duration, int priority, float sourceWeight,
                             float destWeight) {
    return Sequence_StartMorph(source, destination, duration, priority, sourceWeight, destWeight);
}

// 0x818310: Update(time) (slot 20): while active, every sequence updated
void __fastcall Mgr_Update(NiControllerManager* self, re::Edx, float time) {
    if (((uint8_t(self->flags) >> 3) & 1) == 0) {
        return;
    }
    for (uint32_t i = 0; i < self->sequences.size; i++) {
        NiControllerSequence* sequence = self->sequences.base[i];
        if (sequence != nullptr) {
            Sequence_Update(sequence, time, 1);
        }
    }
}

// 0x818350: CrossFade(source, destination, duration, priority, startOver, weight, timeSync): an
// active source eased out as a transition while an inactive destination eases in
uint8_t __fastcall Mgr_CrossFade(NiControllerManager*, re::Edx, NiControllerSequence* source,
                                 NiControllerSequence* destination, float duration, int priority, int startOver,
                                 float weight, NiControllerSequence* timeSync) {
    if (source->state == INACTIVE || destination->state != INACTIVE) {
        return 0;
    }
    Sequence_Deactivate(source, duration, 1);
    return Sequence_Activate(destination, priority, startOver, weight, duration, timeSync, 1);
}

// 0x8183B0: RegisterStreamables: the sequences and the object palette too
uint8_t __fastcall Mgr_RegisterStreamables(NiControllerManager* self, re::Edx, void* stream) {
    if (!TimeController_RegisterStreamables(self, stream)) {
        return 0;
    }
    for (uint32_t i = 0; i < self->sequences.size; i++) {
        NiControllerSequence* sequence = self->sequences.base[i];
        if (sequence != nullptr) {
            Virtual<uint8_t>(sequence, 6, stream);
        }
    }
    if (self->objectPalette != nullptr) {
        Virtual<uint8_t>(self->objectPalette, 6, stream);
    }
    return 1;
}

// 0x818410: PostLinkObject: every sequence bound to the target
void __fastcall Mgr_PostLinkObject(NiControllerManager* self, re::Edx, void* stream) {
    Object_PostLinkObject(self, stream);
    NiAVObject* target = static_cast<NiAVObject*>(self->target);
    for (uint32_t i = 0; i < self->sequences.size; i++) {
        NiControllerSequence* sequence = self->sequences.base[i];
        if (sequence != nullptr) {
            Sequence_StoreTargets(sequence, target);
        }
    }
}

// 0x818460: IsEqual (the same count, every name in the other's map, the sequences equal)
uint8_t __fastcall Mgr_IsEqual(NiControllerManager* self, re::Edx, NiControllerManager* other) {
    if (!TimeController_IsEqual(self, other) || self->sequences.size != other->sequences.size) {
        return 0;
    }
    re::Ptr32<void> position = StringMap_GetFirstPos(&self->sequenceMap);
    while (position != nullptr) {
        re::Ptr32<const char> key;
        uint32_t value;
        StringMap_GetNext(&self->sequenceMap, &position, &key, &value);
        uint32_t found;
        if (!StringMap_GetAt(&other->sequenceMap, key, &found)) {
            return 0;
        }
    }
    for (uint32_t i = 0; i < self->sequences.size; i++) {
        NiControllerSequence* mine = self->sequences.base[i];
        NiControllerSequence* theirs = other->sequences.base[i];
        if (mine != nullptr && theirs != nullptr) {
            if (!Virtual<uint8_t>(mine, 8, theirs)) {
                return 0;
            }
        } else if (mine != theirs) {
            return 0;
        }
    }
    return 1;
}

// 0x818530: NiTObjectArray<NiPointer>::RemoveAt(index): the element taken out (returned held), the
// used count and top lowered
re::Ptr32<NiRefObject>* __fastcall Array_RemoveAt(NiTArray<NiRefObject*>* self, re::Edx, re::Ptr32<NiRefObject>* result, uint32_t index) {
    if (index >= self->size) {
        *result = nullptr;
        return result;
    }
    NiRefObject* element = self->base[index];
    Hold(element);
    re::Ptr32<NiRefObject>* slot = &self->base[index];
    if (*slot != nullptr) {
        ReleaseRef(*slot);
        *slot = nullptr;
    }
    if (element != nullptr) {
        self->esize--;
    }
    uint16_t top = uint16_t(self->size - 1);
    if (index == top) {
        self->size = top;
    }
    *result = element;
    if (element != nullptr) {
        Hold(element);
        ReleaseRef(element);
    }
    return result;
}

// 0x8185F0: RemoveSequence(sequence): taken out of the array and the name map, its targets
// cleared, no owner (returned held); null when it isn't the manager's
re::Ptr32<NiControllerSequence>* __fastcall Mgr_RemoveSequence(NiControllerManager* self, re::Edx, re::Ptr32<NiControllerSequence>* result,
                                                     NiControllerSequence* sequence) {
    uint32_t index = 0;
    NiControllerSequence* found = nullptr;
    bool matched = false;
    for (; index < self->sequences.size; index++) {
        NiControllerSequence* current = self->sequences.base[index];
        Hold(current);
        if (current == sequence) {
            found = current;
            matched = true;   // (an empty slot matches a null sequence, as the game has it)
            break;
        }
        ReleaseRef(current);
    }
    if (!matched) {
        *result = nullptr;
        return result;
    }
    re::Ptr32<NiRefObject> removed;
    ObjectArray_RemoveAt(reinterpret_cast<NiTArray<NiRefObject*>*>(&self->sequences), &removed, index);
    ReleaseRef(removed);
    StringIndexMap* map = &self->sequenceMap;
    re::Ptr32<void> position = nullptr;
    for (uint32_t i = 0; i < map->hashSize; i++) {
        if (map->table[i] != nullptr) {
            position = map->table[i];
            break;
        }
    }
    while (position != nullptr) {
        re::Ptr32<const char> key;
        uint32_t value;
        StringMap_GetNext(map, &position, &key, &value);
        if (value == index) {
            StringMap_RemoveAt(map, key);
            break;
        }
    }
    Sequence_ClearTargets(found);
    found->owner = nullptr;
    *result = found;
    Hold(found);
    ReleaseRef(found);
    return result;
}

// 0x818730: SaveBinary (cumulative, the sequence links (null ones left out), the palette link)
void __fastcall Mgr_SaveBinary(NiControllerManager* self, re::Edx, void* stream) {
    TimeController_SaveBinary(self, stream);
    uint8_t cumulative = self->cumulative;
    SaveBytes(stream, &cumulative, 1);
    uint32_t count = self->sequences.size;
    SaveBytes(stream, &count, 4);
    for (uint32_t i = 0; i < count; i++) {
        NiControllerSequence* sequence = self->sequences.base[i];
        if (sequence != nullptr) {
            Virtual<void>(stream, 12, sequence);   // SaveLinkID
        }
    }
    Virtual<void>(stream, 12, self->objectPalette);
}

// 0x8187E0: LoadBinary (cumulative, the sequence links; from 10.1.0.104 the palette)
void __fastcall Mgr_LoadBinary(NiControllerManager* self, re::Edx, void* stream) {
    TimeController_LoadBinary(self, stream);
    uint8_t cumulative;
    LoadBytes(stream, &cumulative, 1);
    self->cumulative = cumulative != 0 ? 1 : 0;
    Stream_ReadMultipleLinkIDs(stream);
    if (StreamVersion(stream) >= kVersion_10_1_0_104) {
        NiObject* palette = Stream_ReadLinkIDObject(stream);
        AssignReleasingFirst(self->objectPalette, palette);
    }
}

// 0x818880: ~NiControllerManager
void __fastcall Mgr_Destruct(NiControllerManager* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiControllerManager_vtable);
    PointerArray_RemoveAll(&self->sequences);
    self->tempBlendCount = 0;
    ReleaseRef(self->objectPalette);
    NiDeletePointerArray(self->tempBlendSeqs);
    StringMap_Destruct(&self->sequenceMap);
    self->sequences.vtable = reinterpret_cast<void*>(SequenceArray_vtable);
    NiDeletePointerArray(self->sequences.base);
    TimeController_Destruct(self);
}

const RTTI* __fastcall Mgr_GetRTTI(NiControllerManager*, re::Edx) {
    return &NiControllerManager_RTTI;
}

// 0x818900: NiControllerManager()
NiControllerManager* __fastcall Mgr_Construct(NiControllerManager* self, re::Edx) {
    ConstructMembers(self, 0);
    return self;
}

// 0x818980: the deleting destructor
NiControllerManager* __fastcall Mgr_DeletingDestructor(NiControllerManager* self, re::Edx, uint32_t flags) {
    Manager_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x6C);
    }
    return self;
}

// 0x8189A0: NiControllerManager(target, cumulative): the target's object palette made
NiControllerManager* __fastcall Mgr_ConstructWithTarget(NiControllerManager* self, re::Edx, NiAVObject* target,
                                                        int cumulative) {
    ConstructMembers(self, uint8_t(cumulative));
    TimeController_SetTarget(self, target);
    void* memory = NiNew(0x20);
    NiObject* palette = memory != nullptr ? DefaultPalette_Construct(memory, target) : nullptr;
    AssignReleasingFirst(self->objectPalette, palette);
    return self;
}

// 0x818A90: AddSequence(sequence, name, storeTargets): one that has no owner yet taken (its
// accumulation root named after the target when it has no name for it; bound to the target when
// asked, given up when that fails; renamed when a name is given), added to the array and the
// name map
uint8_t __fastcall Mgr_AddSequence(NiControllerManager* self, re::Edx, NiControllerSequence* sequence,
                                   const re::Ptr32<const char>* name, int storeTargets) {
    if (self != nullptr && sequence->owner != nullptr) {
        return 0;
    }
    sequence->owner = self;
    if (sequence->accumRootName == nullptr) {
        FixedString_Assign(&sequence->accumRootName, &self->target->name);
    }
    if (re::Flag(storeTargets) && !Sequence_StoreTargets(sequence, static_cast<NiAVObject*>(self->target))) {
        sequence->owner = nullptr;
        return 0;
    }
    if (*name != sys::kNullFixedString) {
        FixedString_Assign(&sequence->name, name);
    }
    re::Ptr32<NiRefObject> held = sequence;
    sys::Increment(&sequence->refCount);
    NiFixedString key = sequence->name;
    uint32_t index = ObjectArray_AddFirstEmpty(&self->sequences, &held);
    StringMap_SetAt(&self->sequenceMap, key, index);
    ReleaseRef(sequence);
    return 1;
}

// 0x818B40: CopyMembers(dest, cloning): cumulative, every sequence's clone added (not bound), the
// palette's clone
void __fastcall Mgr_CopyMembers(NiControllerManager* self, re::Edx, NiControllerManager* dest, void* cloning) {
    TimeController_CopyMembers(self, dest, cloning);
    dest->cumulative = self->cumulative;
    for (uint32_t i = 0; i < self->sequences.size; i++) {
        NiControllerSequence* sequence = self->sequences.base[i];
        if (sequence == nullptr) {
            continue;
        }
        auto* clone = Virtual<NiControllerSequence*>(sequence, 3, cloning);   // CreateClone
        NiFixedString empty = sys::kNullFixedString;
        Manager_AddSequence(dest, clone, &empty, 0);
        FixedRelease(&empty);
    }
    if (self->objectPalette != nullptr) {
        NiObject* palette = Virtual<NiObject*>(self->objectPalette, 3, cloning);
        AssignReleasingFirst(dest->objectPalette, palette);
    }
}

// 0x818C20: ProcessClone: the sequences processed and bound again; the clone's temporary blend
// sequences the clones of these (or these when there's none); the palette processed
void __fastcall Mgr_ProcessClone(NiControllerManager* self, re::Edx, void* cloning) {
    TimeController_ProcessClone(self, cloning);
    for (uint32_t i = 0; i < self->sequences.size; i++) {
        NiControllerSequence* sequence = self->sequences.base[i];
        if (sequence != nullptr) {
            Virtual<void>(sequence, 11, cloning);
            Sequence_StoreTargets(sequence, static_cast<NiAVObject*>(self->target));
        }
    }
    const re::Ptr32<void>* found = FindClone(cloning, self);
    NiControllerManager* clone = found != nullptr ? static_cast<NiControllerManager*>(*found) : nullptr;
    for (uint32_t i = 0; i < self->tempBlendCount; i++) {
        re::Ptr32<NiControllerSequence>* element = &self->tempBlendSeqs[i];
        const re::Ptr32<void>* mapped = FindClone(cloning, *element);
        if (mapped == nullptr) {
            ObjectSet_Add(&clone->tempBlendSeqs, re::PtrCast<const re::Ptr32<ni::NiRefObject>*>(element));
            continue;
        }
        auto* sequence = static_cast<NiControllerSequence*>(*mapped);
        Hold(sequence);
        if (clone->tempBlendCount == clone->tempBlendAllocated) {
            uint32_t allocated = clone->tempBlendAllocated;
            ObjectSet_Realloc(&clone->tempBlendSeqs, allocated != 0 ? allocated * 2 : 1);
        }
        re::Ptr32<NiControllerSequence>* slot = &clone->tempBlendSeqs[clone->tempBlendCount];
        clone->tempBlendCount++;
        if (*slot != sequence) {
            ReleaseRef(*slot);
            *slot = sequence;
            Hold(sequence);
        }
        ReleaseRef(sequence);
    }
    if (self->objectPalette != nullptr) {
        Virtual<void>(self->objectPalette, 11, cloning);
    }
}

// 0x818DC0: CreateObject
NiObject* __cdecl Mgr_CreateObject() {
    void* memory = NiNew(0x6C);
    if (memory == nullptr) {
        return nullptr;
    }
    return Manager_Construct(memory);
}

// 0x818DE0: LinkObject: each sequence link added as AddSequence would (unnamed, not bound)
void __fastcall Mgr_LinkObject(NiControllerManager* self, re::Edx, void* stream) {
    TimeController_LinkObject(self, stream);
    uint32_t count = Stream_GetNumberOfLinkIDs(stream);
    PointerArray_SetSize(&self->sequences, count);
    for (uint32_t n = count; n != 0; n--) {
        auto* sequence = static_cast<NiControllerSequence*>(Stream_ResolveLinkID(stream));
        NiFixedString empty = sys::kNullFixedString;
        if (self != nullptr && sequence->owner != nullptr) {
            continue;
        }
        sequence->owner = self;
        if (sequence->accumRootName == nullptr) {
            FixedString_Assign(&sequence->accumRootName, &self->target->name);
        }
        if (empty != sys::kNullFixedString) {
            FixedString_Assign(&sequence->name, &empty);
        }
        re::Ptr32<NiRefObject> held = sequence;
        sys::Increment(&sequence->refCount);
        NiFixedString key = sequence->name;
        uint32_t index = ObjectArray_AddFirstEmpty(&self->sequences, &held);
        StringMap_SetAt(&self->sequenceMap, key, index);
        ReleaseRef(sequence);
        FixedRelease(&empty);
    }
}

// 0x818EF0: CreateClone (a failed allocation still copied onto null, as the game has it)
NiControllerManager* __fastcall Mgr_CreateClone(NiControllerManager* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x6C);
    if (memory == nullptr) {
        Manager_CopyMembers(self, nullptr, cloning);
        return nullptr;
    }
    NiControllerManager* clone = Manager_Construct(memory);
    Manager_CopyMembers(self, clone, cloning);
    return clone;
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC31_MGR_REVERSED(name, address, fn)                                                      \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIC31_MGR_REVERSED("NiControllerManager::Morph", 0x008182E0, &ni::anim::Mgr_Morph);
NIC31_MGR_REVERSED("NiControllerManager::Update", 0x00818310, &ni::anim::Mgr_Update);
NIC31_MGR_REVERSED("NiControllerManager::CrossFade", 0x00818350, &ni::anim::Mgr_CrossFade);
NIC31_MGR_REVERSED("NiControllerManager::RegisterStreamables", 0x008183B0, &ni::anim::Mgr_RegisterStreamables);
NIC31_MGR_REVERSED("NiControllerManager::PostLinkObject", 0x00818410, &ni::anim::Mgr_PostLinkObject);
NIC31_MGR_REVERSED("NiControllerManager::IsEqual", 0x00818460, &ni::anim::Mgr_IsEqual);
NIC31_MGR_REVERSED("NiTObjectArray<NiPointer>::RemoveAt", 0x00818530, &ni::anim::Array_RemoveAt);
NIC31_MGR_REVERSED("NiControllerManager::RemoveSequence", 0x008185F0, &ni::anim::Mgr_RemoveSequence);
NIC31_MGR_REVERSED("NiControllerManager::SaveBinary", 0x00818730, &ni::anim::Mgr_SaveBinary);
NIC31_MGR_REVERSED("NiControllerManager::LoadBinary", 0x008187E0, &ni::anim::Mgr_LoadBinary);
NIC31_MGR_REVERSED("NiControllerManager::~NiControllerManager", 0x00818880, &ni::anim::Mgr_Destruct);
NIC31_MGR_REVERSED("NiControllerManager::GetRTTI", 0x008188F0, &ni::anim::Mgr_GetRTTI);
NIC31_MGR_REVERSED("NiControllerManager::NiControllerManager", 0x00818900, &ni::anim::Mgr_Construct);
NIC31_MGR_REVERSED("NiControllerManager::DeletingDestructor", 0x00818980, &ni::anim::Mgr_DeletingDestructor);
NIC31_MGR_REVERSED("NiControllerManager::NiControllerManager(target)", 0x008189A0,
                   &ni::anim::Mgr_ConstructWithTarget);
NIC31_MGR_REVERSED("NiControllerManager::AddSequence", 0x00818A90, &ni::anim::Mgr_AddSequence);
NIC31_MGR_REVERSED("NiControllerManager::CopyMembers", 0x00818B40, &ni::anim::Mgr_CopyMembers);
NIC31_MGR_REVERSED("NiControllerManager::ProcessClone", 0x00818C20, &ni::anim::Mgr_ProcessClone);
NIC31_MGR_REVERSED("NiControllerManager::CreateObject", 0x00818DC0, &ni::anim::Mgr_CreateObject);
NIC31_MGR_REVERSED("NiControllerManager::LinkObject", 0x00818DE0, &ni::anim::Mgr_LinkObject);
NIC31_MGR_REVERSED("NiControllerManager::CreateClone", 0x00818EF0, &ni::anim::Mgr_CreateClone);
