// The entity library's small classes and template instantiations in this part of the exe: NiScene's
// constructor, destructor and AddSelectionSet, NiEntitySelectionSet's destructor, NiUniqueID's
// byte constructor, the interface-pointer set (NiTObjectSet<NiEntityInterfaceIPtr>: RemoveAll,
// Realloc), the maps the GSA code keeps (NiTMap<unsigned int, NiEntityPropertyInterfaceIPtr>,
// NiTPointerMap<T, bool>, the link-ID pair) and NiTStringPointerMap's GetNext (folded: every
// string-keyed map of the library uses it).
//
// NiScene (vtable 0x98AA20, 0x34): an NiRefObject with its name, its entities (+0xC, an
// NiTObjectSet of interface pointers), a lookup map of them (+0x18, NiTPointerMap<entity, bool>)
// and its selection sets (+0x28, NiTObjectSet<NiEntitySelectionSetPtr>). NiEntitySelectionSet
// (vtable 0x98A914, 0x18): its name and entities (+0xC). Names Gamebryo 2.x's.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiEntity.h"
#include "reversed.h"

namespace ni::entity {
namespace {

GAME_VAR(uint32_t, NiRefObject_ms_uiObjects, 0x020E2168);
constexpr uint32_t kNiRefObjectVtable = 0x008FEFD4;
constexpr uint32_t kNiSceneVtable = 0x0098AA20;
constexpr uint32_t kBoolMapBaseVtable = 0x0098A70C;      // NiTMapBase<NiTPointerAllocator, T, bool>
constexpr uint32_t kBoolPointerMapVtable = 0x0098A73C;   // NiTPointerMap<T, bool>
constexpr uint32_t kIPtrMapBaseVtable = 0x0098A6F8;      // NiTMapBase<NiTDefaultAllocator, uint, IPtr>
constexpr uint32_t kIPtrMapVtable = 0x0098A720;          // NiTMap<unsigned int, IPtr>
constexpr uint32_t kObjectIDMapVtable = 0x0098AD94;      // NiTPointerMap<NiEntityPropertyInterface*, uint>

GAME_FN(void*, __cdecl, IPtrArray_New, 0x00856120, uint32_t count);   // NiNew[] of null interface pointers
GAME_FN(void, __cdecl, IPtrArray_Delete, 0x008544E0, void* base);
GAME_FN(void, __thiscall, ObjectSet_RemoveAll, 0x00836940, void* set);
GAME_FN(void, __cdecl, PointerArray_Delete, 0x00836F80, void* base);
GAME_FN(void, __thiscall, ObjectSet_Add, 0x00787BE0, void* set, const re::Ptr32<NiRefObject>* element);
GAME_FN(uint32_t, __cdecl, StringHash, 0x005F09F0, const char* key, uint32_t hashSize);   // KeyToHashIndex

// this file's (called where modes apply)
GAME_FN(void, __thiscall, IPtrSet_RemoveAll, 0x0083E380, NiTSet<void*>* self);
GAME_FN(void, __thiscall, SelectionSet_Destruct, 0x00840C60, NiRefObject* self);
GAME_FN(void, __thiscall, Scene_Destruct, 0x00841B50, NiRefObject* self);

// NiPointer<T> released (the game's import slots)
inline void ReleaseRef(NiRefObject* object) {
    if (object != nullptr && ni::sys::Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

// a map's buckets made as the constructors inline it (37, cleared)
inline void InitMap(PointerMap* map, uint32_t baseVtable, uint32_t vtable) {
    map->vtable = reinterpret_cast<void*>(baseVtable);
    map->hashSize = 0x25;
    map->count = 0;
    map->table = static_cast<re::Ptr32<MapItem>*>(NiMalloc(0x94));
    crt::Memset(map->table, 0, map->hashSize * 4);
    map->vtable = reinterpret_cast<void*>(vtable);
}

// a two-level map's destructor inlined: each level's RemoveAll, the buckets freed
inline void DestructMap(PointerMap* map, uint32_t vtable, uint32_t baseVtable) {
    map->vtable = reinterpret_cast<void*>(vtable);
    StringMap_RemoveAll(map);
    map->vtable = reinterpret_cast<void*>(baseVtable);
    StringMap_RemoveAll(map);
    NiFree(map->table);
}

// ---- NiTStringPointerMap

// 0x83D890 GetNext(position, key, value): the item's key and value, the position moved to the
// next item (the same bucket's, else the next non-empty bucket's, else none)
void __fastcall GetNext(PointerMap* self, re::Edx, re::Ptr32<MapItem>* position, re::Ptr32<void>* key, re::Ptr32<void>* value) {
    MapItem* item = *position;
    *key = item->key;
    *value = item->value;
    if (item->next != nullptr) {
        *position = item->next;
        return;
    }
    uint32_t size = self->hashSize;
    for (uint32_t i = StringHash(static_cast<const char*>(item->key), size) + 1; i < size; i++) {
        if (self->table[i] != nullptr) {
            *position = self->table[i];
            return;
        }
    }
    *position = nullptr;
}

// ---- NiUniqueID

// 0x83DF20 NiUniqueID(16 bytes)
uint8_t* __fastcall UniqueID_Construct(uint8_t* self, re::Edx, uint32_t b0, uint32_t b1, uint32_t b2, uint32_t b3,
                                       uint32_t b4, uint32_t b5, uint32_t b6, uint32_t b7, uint32_t b8, uint32_t b9,
                                       uint32_t b10, uint32_t b11, uint32_t b12, uint32_t b13, uint32_t b14,
                                       uint32_t b15) {
    const uint32_t bytes[16] = {b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15};
    for (int i = 0; i < 16; i++) {
        self[i] = uint8_t(bytes[i]);
    }
    return self;
}

// ---- NiTObjectSet<NiEntityInterfaceIPtr>

// 0x83E380 RemoveAll: every allocated slot's reference let go (slot 1), none used
void __fastcall IPtrSetRemoveAll(NiTSet<void*>* self, re::Edx) {
    for (uint32_t i = 0; i < self->alloced; i++) {
        re::Ptr32<void>& slot = self->base[i];
        if (slot != nullptr) {
            Virtual<void>(slot, prop::RemoveReference);
            slot = nullptr;
        }
    }
    self->used = 0;
}

// 0x8409C0 Realloc(size): a new array (none for 0), the used elements moved over (assigned), the
// old array deleted
void __fastcall IPtrSetRealloc(NiTSet<void*>* self, re::Edx, uint32_t size) {
    if (size == self->alloced) {
        return;
    }
    re::Ptr32<void>* fresh = nullptr;
    if (size != 0) {
        fresh = static_cast<re::Ptr32<void>*>(IPtrArray_New(size));
        for (uint32_t i = 0; i < self->used; i++) {
            void* value = self->base[i];
            if (fresh[i] != value) {
                RemoveReference(fresh[i]);
                fresh[i] = value;
                AddReference(value);
            }
        }
    }
    IPtrArray_Delete(self->base);
    self->base = fresh;
    self->alloced = size;
}

// ---- the maps

// 0x83E3D0 NiTMapBase<..., bool>::SetValue(item, key, value)
void __fastcall BoolMap_SetValue(void*, re::Edx, MapItem* item, void* key, int value) {
    item->key = key;
    *reinterpret_cast<uint8_t*>(&item->value) = uint8_t(value);
}

// 0x83F190 NiTMapBase<..., IPtr>::SetValue(item, key, value): the value comes as an interface
// pointer by value (the caller's reference), let go at the end
void __fastcall IPtrMap_SetValue(void*, re::Edx, MapItem* item, void* key, void* value) {
    item->key = key;
    if (item->value != value) {
        RemoveReference(item->value);
        item->value = value;
        if (value == nullptr) {
            return;
        }
        AddReference(value);
    }
    RemoveReference(value);
}

// 0x83E3F0 NiTMapBase<NiTDefaultAllocator, uint, IPtr>'s deleting destructor
PointerMap* __fastcall IPtrMapBase_DeletingDestructor(PointerMap* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(kIPtrMapBaseVtable);
    StringMap_RemoveAll(self);
    NiFree(self->table);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(PointerMap));
    }
    return self;
}

// 0x83E430 NiTMap<unsigned int, IPtr>'s deleting destructor
PointerMap* __fastcall IPtrMap_DeletingDestructor(PointerMap* self, re::Edx, int flags) {
    DestructMap(self, kIPtrMapVtable, kIPtrMapBaseVtable);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(PointerMap));
    }
    return self;
}

// 0x83F1E0 NiTMapBase<..., bool>'s deleting destructor
PointerMap* __fastcall BoolMapBase_DeletingDestructor(PointerMap* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(kBoolMapBaseVtable);
    StringMap_RemoveAll(self);
    NiFree(self->table);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(PointerMap));
    }
    return self;
}

// 0x83FAD0 NiTPointerMap<T, bool>'s deleting destructor
PointerMap* __fastcall BoolPointerMap_DeletingDestructor(PointerMap* self, re::Edx, int flags) {
    DestructMap(self, kBoolPointerMapVtable, kBoolMapBaseVtable);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(PointerMap));
    }
    return self;
}

// 0x83FB10 NiTMap<unsigned int, IPtr>::NewItem: an item with a null value (NiNew failing is
// dereferenced, as the code does)
MapItem* __fastcall IPtrMap_NewItem(PointerMap*, re::Edx) {
    auto* item = static_cast<MapItem*>(NiNew(sizeof(MapItem)));
    if (item != nullptr) {
        item->value = nullptr;
    }
    if (item->value != nullptr) {
        RemoveReference(item->value);
        item->value = nullptr;
    }
    return item;
}

// 0x840A50 NiTMap<unsigned int, IPtr>::DeleteItem(item): the value let go (ClearValue, then the
// item's destructor), the item deleted
void __fastcall IPtrMap_DeleteItem(PointerMap*, re::Edx, MapItem* item) {
    if (item->value != nullptr) {
        RemoveReference(item->value);
        item->value = nullptr;
    }
    if (item != nullptr) {
        RemoveReference(item->value);
        NiDelete(item, sizeof(MapItem));
    }
}

// 0x83F220 the link-ID pair's constructor: object to ID, ID to object
LinkIDMap* __fastcall LinkIDMap_Construct(LinkIDMap* self, re::Edx) {
    InitMap(&self->objectToID, kNiTMapBaseStringVtable, kObjectIDMapVtable);
    InitMap(&self->idToObject, kIPtrMapBaseVtable, kIPtrMapVtable);
    return self;
}

// 0x83E470 its destructor (the second map first)
void __fastcall LinkIDMap_Destruct(LinkIDMap* self, re::Edx) {
    DestructMap(&self->idToObject, kIPtrMapVtable, kIPtrMapBaseVtable);
    DestructMap(&self->objectToID, kObjectIDMapVtable, kNiTMapBaseStringVtable);
}

// ---- NiEntitySelectionSet

// 0x840C60: its entities let go, the name, NiRefObject's
void __fastcall SelectionSetDestruct(NiRefObject* self, re::Edx) {
    IPtrSet_RemoveAll(&At<NiTSet<void*>>(self, 0xC));
    IPtrArray_Delete(At<void*>(self, 0xC));
    ReleaseFixedString(&At<NiFixedString>(self, 8));
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    ni::sys::Decrement(&NiRefObject_ms_uiObjects);
}

// 0x840CC0
NiRefObject* __fastcall SelectionSetDeletingDestructor(NiRefObject* self, re::Edx, int flags) {
    SelectionSet_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x18);
    }
    return self;
}

// ---- NiScene

// 0x840CE0 AddSelectionSet(set): refused when one of the same name (handle) is there
uint8_t __fastcall Scene_AddSelectionSet(uint8_t* self, re::Edx, NiRefObject* set) {
    NiTSet<NiRefObject*>& sets = At<NiTSet<NiRefObject*>>(self, 0x28);
    for (uint32_t i = 0; i < sets.used; i++) {
        if (At<NiFixedString>(sets.base[i], 8) == At<NiFixedString>(set, 8)) {
            return 0;
        }
    }
    re::Ptr32<NiRefObject> held = set;
    if (held != nullptr) {
        ni::sys::Increment(&held->refCount);
    }
    ObjectSet_Add(&sets, &held);
    ReleaseRef(held);
    return 1;
}

// 0x841A80 NiScene(name, entity array size, selection set array size)
NiRefObject* __fastcall Scene_Construct(NiRefObject* self, re::Edx, const re::Ptr32<const char>* name, uint32_t entities,
                                        uint32_t selectionSets) {
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    self->refCount = 0;
    ni::sys::Increment(&NiRefObject_ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(kNiSceneVtable);
    At<NiFixedString>(self, 8) = CopyFixedString(*name);
    NiTSet<void*>& entitySet = At<NiTSet<void*>>(self, 0xC);
    entitySet.base = entities != 0 ? static_cast<re::Ptr32<void>*>(IPtrArray_New(entities)) : nullptr;
    entitySet.alloced = entities;
    entitySet.used = 0;
    PointerMap& map = At<PointerMap>(self, 0x18);
    InitMap(&map, kBoolMapBaseVtable, kBoolPointerMapVtable);
    NiTSet<void*>& sets = At<NiTSet<void*>>(self, 0x28);
    sets.base = selectionSets != 0 ? static_cast<re::Ptr32<void>*>(IPtrArray_New(selectionSets)) : nullptr;
    sets.alloced = selectionSets;
    sets.used = 0;
    return self;
}

// 0x841B50 ~NiScene: the selection sets, the map, the entities, the name
void __fastcall SceneDestruct(NiRefObject* self, re::Edx) {
    ObjectSet_RemoveAll(&At<NiTSet<void*>>(self, 0x28));
    PointerArray_Delete(At<void*>(self, 0x28));
    DestructMap(&At<PointerMap>(self, 0x18), kBoolPointerMapVtable, kBoolMapBaseVtable);
    IPtrSet_RemoveAll(&At<NiTSet<void*>>(self, 0xC));
    IPtrArray_Delete(At<void*>(self, 0xC));
    ReleaseFixedString(&At<NiFixedString>(self, 8));
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    ni::sys::Decrement(&NiRefObject_ms_uiObjects);
}

// 0x841BF0
NiRefObject* __fastcall SceneDeletingDestructor(NiRefObject* self, re::Edx, int flags) {
    Scene_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x34);
    }
    return self;
}

} // namespace

#define NIENTITYTPL_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                                \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIENTITYTPL_REVERSED("NiTStringPointerMap::GetNext_83D890", 0x0083D890, &GetNext);
NIENTITYTPL_REVERSED("NiUniqueID::NiUniqueID(bytes)", 0x0083DF20, &UniqueID_Construct);
NIENTITYTPL_REVERSED("NiTObjectSet<NiEntityInterfaceIPtr>::RemoveAll", 0x0083E380, &IPtrSetRemoveAll);
NIENTITYTPL_REVERSED("NiTMapBase<bool>::SetValue_83E3D0", 0x0083E3D0, &BoolMap_SetValue);
NIENTITYTPL_REVERSED("NiTMapBase<uint,IPtr>::DeletingDestructor", 0x0083E3F0, &IPtrMapBase_DeletingDestructor);
NIENTITYTPL_REVERSED("NiTMap<uint,IPtr>::DeletingDestructor", 0x0083E430, &IPtrMap_DeletingDestructor);
NIENTITYTPL_REVERSED("NiEntityStreaming::LinkIDMap::~LinkIDMap", 0x0083E470, &LinkIDMap_Destruct);
NIENTITYTPL_REVERSED("NiTMapBase<uint,IPtr>::SetValue", 0x0083F190, &IPtrMap_SetValue);
NIENTITYTPL_REVERSED("NiTMapBase<bool>::DeletingDestructor", 0x0083F1E0, &BoolMapBase_DeletingDestructor);
NIENTITYTPL_REVERSED("NiEntityStreaming::LinkIDMap::LinkIDMap", 0x0083F220, &LinkIDMap_Construct);
NIENTITYTPL_REVERSED("NiTPointerMap<bool>::DeletingDestructor", 0x0083FAD0, &BoolPointerMap_DeletingDestructor);
NIENTITYTPL_REVERSED("NiTMap<uint,IPtr>::NewItem", 0x0083FB10, &IPtrMap_NewItem);
NIENTITYTPL_REVERSED("NiTObjectSet<NiEntityInterfaceIPtr>::Realloc", 0x008409C0, &IPtrSetRealloc);
NIENTITYTPL_REVERSED("NiTMap<uint,IPtr>::DeleteItem", 0x00840A50, &IPtrMap_DeleteItem);
NIENTITYTPL_REVERSED("NiEntitySelectionSet::~NiEntitySelectionSet", 0x00840C60, &SelectionSetDestruct);
NIENTITYTPL_REVERSED("NiEntitySelectionSet::DeletingDestructor", 0x00840CC0, &SelectionSetDeletingDestructor);
NIENTITYTPL_REVERSED("NiScene::AddSelectionSet", 0x00840CE0, &Scene_AddSelectionSet);
NIENTITYTPL_REVERSED("NiScene::NiScene", 0x00841A80, &Scene_Construct);
NIENTITYTPL_REVERSED("NiScene::~NiScene", 0x00841B50, &SceneDestruct);
NIENTITYTPL_REVERSED("NiScene::DeletingDestructor", 0x00841BF0, &SceneDeletingDestructor);

} // namespace ni::entity
