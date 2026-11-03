// NiEntityStreaming's code before 0x856380 (the rest, its constructor and InsertScene, is
// NiEntityStreaming.cpp's): its destructors, the scene array's accessors and ReportError; the
// link-ID maps the GSA code keeps (object to ID and ID to object: NiTPointerMap<
// NiEntityPropertyInterface*, unsigned int> and NiTMap<unsigned int,
// NiEntityPropertyInterfaceIPtr>, the latter's SetAt / GetAt here); and two templates the whole
// exe shares: NiNew[] of NiPointers and NiTObjectArray<NiPointer<T>>::SetSize.
//
// NiEntityStreaming (vtable 0x98C2B4, 0x1C bytes): the scenes it holds (+0x8), the error
// handler (+0x18). An IPtr is an interface pointer held through the interface's AddReference /
// RemoveReference (slots 0 / 1); an NiPointer an NiRefObject's count (through the Interlocked
// imports).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::streamlinks {
namespace {

template <typename R = void, typename... A>
inline R Virtual(const void* object, int slot, A... args) {
    return reinterpret_cast<R(__thiscall*)(const void*, re::Arg<A>...)>(static_cast<void*>(VtableOf(object)[slot]))(object, args...);
}

inline void AddReference(void* iface) {
    Virtual<void>(iface, 0);
}
inline void RemoveReference(void* iface) {
    Virtual<void>(iface, 1);
}

// an NiPointer let go (DeleteThis on the last)
inline void ReleaseRef(NiRefObject* object) {
    if (ni::sys::Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

// NiTMap / NiTPointerMap (vtable: 1 SetValue(item, key, value), 3 NewItem)
struct MapItem {
    re::Ptr32<MapItem> next;   // +0x0
    uint32_t key;    // +0x4
    re::Ptr32<void> value;     // +0x8
};
struct Map {
    re::Ptr32<void> vtable;        // +0x0
    uint32_t hashSize;   // +0x4
    re::Ptr32<re::Ptr32<MapItem>> table;     // +0x8
    uint32_t count;      // +0xC
};
static_assert(sizeof(Map) == 0x10);

// the link IDs both ways
struct LinkIDMap {
    Map objectToID;   // +0x0 (vtable 0x98AD94)
    Map idToObject;   // +0x10 (vtable 0x98A720: the objects held)
};
static_assert(sizeof(LinkIDMap) == 0x20);

// NiTObjectArray<NiPointer<T>> (NiTArray's layout, 16-bit counts)
struct ObjectArray {
    re::Ptr32<void> vtable;        // +0x0
    re::Ptr32<re::Ptr32<NiRefObject>> base;  // +0x4
    uint16_t maxSize;    // +0x8
    uint16_t size;       // +0xA
    uint16_t esize;      // +0xC
    uint16_t growBy;     // +0xE
};
static_assert(sizeof(ObjectArray) == 0x10);

struct EntityStreaming : NiRefObject {
    ObjectArray scenes;   // +0x8 (NiTObjectArray<NiScenePtr>, vtable 0x9893C0)
    re::Ptr32<void> errorHandler;   // +0x18 (NiEntityErrorInterface: slot 2 ReportError)
};
static_assert(offsetof(EntityStreaming, errorHandler) == 0x18);
static_assert(sizeof(EntityStreaming) == 0x1C);

constexpr uintptr_t EntityStreaming_vtable = 0x0098C2B4;
constexpr uintptr_t ObjectArray_vtable = 0x009893C0;
constexpr uintptr_t RefObject_vtable = 0x008FEFD4;
GAME_VAR(long, RefObject_ms_uiObjects, 0x020E2168);
constexpr uintptr_t PointerConstruct = 0x006D0EB0;   // NiPointer's constructor (cleared)

GAME_FN(void, __thiscall, Map_RemoveAll, 0x005AC520, Map* self);
GAME_FN(void, __thiscall, PointerMap_SetAt, 0x008A5140, Map* self, void* key, uintptr_t value);
GAME_FN(void, __thiscall, ObjectArray_RemoveAll, 0x0040E7A0, ObjectArray* self);
GAME_FN(void, __cdecl, PointerArray_Delete, 0x00836F80, re::Ptr32<NiRefObject>* base);   // NiDelete[] of NiPointers
GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, uintptr_t construct);

// this file's, through their addresses
GAME_FN(void, __thiscall, IPtrMap_SetAt_, 0x00855F60, Map* self, uint32_t key, void* value);
GAME_FN(uint8_t, __thiscall, IPtrMap_GetAt_, 0x00856020, Map* self, uint32_t key, re::Ptr32<void>* value);
GAME_FN(re::Ptr32<NiRefObject>*, __cdecl, NewPointerArray_, 0x00856120, uint32_t count);

// ---- NiEntityStreaming

// told to the error handler, if there's one
void __fastcall ReportError(EntityStreaming* self, re::Edx, const re::Ptr32<const char>* message,
                            const re::Ptr32<const char>* description, const re::Ptr32<const char>* entityName,
                            const re::Ptr32<const char>* propertyName) {
    if (self->errorHandler != nullptr) {
        Virtual<void>(self->errorHandler, 2, message, description, entityName, propertyName);
    }
}

// (folded: a getter of the word at +0x12, the game's MDXact command's too)
uint32_t __fastcall GetSceneCount(EntityStreaming* self, re::Edx) {
    return self->scenes.size;
}

// (folded: the element at an index of the array at +0x8)
NiRefObject* __fastcall GetSceneAt(EntityStreaming* self, re::Edx, uint32_t index) {
    return self->scenes.base[index];
}

void __fastcall RemoveAllScenes(EntityStreaming* self, re::Edx) {
    ObjectArray_RemoveAll(&self->scenes);
}

// the scenes let go, the array freed (NiTObjectArray's destructor inlined), NiRefObject's
inline void DestructBody(EntityStreaming* self) {
    self->vtable = reinterpret_cast<void*>(EntityStreaming_vtable);
    ObjectArray_RemoveAll(&self->scenes);
    self->scenes.vtable = reinterpret_cast<void*>(ObjectArray_vtable);
    PointerArray_Delete(self->scenes.base);
    self->vtable = reinterpret_cast<void*>(RefObject_vtable);
    ni::sys::Decrement(&RefObject_ms_uiObjects);
}

void __fastcall Destruct(EntityStreaming* self, re::Edx) {
    DestructBody(self);
}

EntityStreaming* __fastcall DeletingDestructor(EntityStreaming* self, re::Edx, int flags) {
    DestructBody(self);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(EntityStreaming));
    }
    return self;
}

// ---- the link IDs

void __fastcall LinksRemoveAll(LinkIDMap* self, re::Edx) {
    Map_RemoveAll(&self->idToObject);
    Map_RemoveAll(&self->objectToID);
}

// NiTMap<unsigned int, IPtr>::SetAt: the value comes as an IPtr by value (the caller's
// reference, let go at the end)
void __fastcall IPtrMapSetAt(Map* self, re::Edx, uint32_t key, void* value) {
    uint32_t index = key % self->hashSize;
    for (MapItem* item = self->table[index]; item != nullptr; item = item->next) {
        if (key == item->key) {
            void* old = item->value;
            if (old != value) {
                if (old != nullptr) {
                    RemoveReference(old);
                }
                item->value = value;
                if (value == nullptr) {
                    return;
                }
                AddReference(value);
            }
            if (value != nullptr) {
                RemoveReference(value);
            }
            return;
        }
    }
    MapItem* item = Virtual<MapItem*>(self, 3);   // NewItem
    if (value != nullptr) {
        AddReference(value);   // (SetValue's IPtr by value: it lets it go)
    }
    Virtual<void>(self, 1, item, key, value);   // SetValue
    item->next = self->table[index];
    self->table[index] = item;
    self->count++;
    if (value != nullptr) {
        RemoveReference(value);
    }
}

// NiTMap<unsigned int, IPtr>::GetAt: assigned into the caller's IPtr
uint8_t __fastcall IPtrMapGetAt(Map* self, re::Edx, uint32_t key, re::Ptr32<void>* value) {
    for (MapItem* item = self->table[key % self->hashSize]; item != nullptr; item = item->next) {
        if (key == item->key) {
            void* old = *value;
            if (old != item->value) {
                if (old != nullptr) {
                    RemoveReference(old);
                }
                *value = item->value;
                if (*value != nullptr) {
                    AddReference(*value);
                }
            }
            return 1;
        }
    }
    return 0;
}

// an object known by an ID, both ways
void __fastcall LinksAdd(LinkIDMap* self, re::Edx, void* object, uint32_t id) {
    if (object != nullptr) {
        AddReference(object);   // (the IPtr by value SetAt lets go)
    }
    IPtrMap_SetAt_(&self->idToObject, id, object);
    PointerMap_SetAt(&self->objectToID, object, id);
}

// the object of an ID (the map's reference: none taken)
void* __fastcall LinksGetObject(LinkIDMap* self, re::Edx, uint32_t id) {
    re::Ptr32<void> held = nullptr;
    if (IPtrMap_GetAt_(&self->idToObject, id, &held)) {
        void* object = held;
        if (object != nullptr) {
            RemoveReference(object);
        }
        return object;
    }
    if (held != nullptr) {
        RemoveReference(held);
    }
    return nullptr;
}

// ---- the shared templates

// NiNew NiPointer<T>[count]: the count before the elements, every one cleared (the size
// saturates on overflow)
re::Ptr32<NiRefObject>* __cdecl NewPointerArray(uint32_t count) {
    uint64_t bytes = uint64_t(count) * 4;
    uint32_t size = bytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(bytes);
    size = size > 0xFFFFFFFBu ? 0xFFFFFFFFu : size + 4;
    uint32_t* block = static_cast<uint32_t*>(NiNewArray(size));
    if (block == nullptr) {
        return nullptr;
    }
    *block = count;
    VectorConstruct(block + 1, 4, count, PointerConstruct);
    return reinterpret_cast<re::Ptr32<NiRefObject>*>(block + 1);
}

// NiTObjectArray<NiPointer<T>>::SetSize: the ones past a smaller size let go, then the
// elements copied into a new array (references taken; the old array's let go with it)
void __fastcall ObjectArraySetSize(ObjectArray* self, re::Edx, uint32_t maxSize) {
    if (maxSize == self->maxSize) {
        return;
    }
    uint16_t size = self->size;
    if (maxSize < size) {
        for (uint16_t i = uint16_t(maxSize); i < self->size; i++) {
            re::Ptr32<NiRefObject>* slotRef = &self->base[i];
            if (*slotRef != nullptr) {
                NiRefObject* object = *slotRef;
                if (object != nullptr) {
                    ReleaseRef(object);
                    *slotRef = nullptr;
                }
                self->esize--;
            }
        }
        self->size = uint16_t(maxSize);
    }
    re::Ptr32<NiRefObject>* old = self->base;
    self->maxSize = uint16_t(maxSize);
    if (maxSize == 0) {
        self->base = nullptr;
        PointerArray_Delete(old);
        return;
    }
    self->base = NewPointerArray_(uint16_t(maxSize));
    for (uint16_t i = 0; i < self->size; i++) {
        re::Ptr32<NiRefObject>* to = &self->base[i];
        NiRefObject* from = old[i];
        if (*to != from) {
            if (*to != nullptr) {
                ReleaseRef(*to);
            }
            *to = old[i];
            if (*to != nullptr) {
                ni::sys::Increment(&(*to)->refCount);
            }
        }
    }
    for (uint16_t i = self->size; i < self->maxSize; i++) {
        re::Ptr32<NiRefObject>* slotRef = &self->base[i];
        NiRefObject* object = *slotRef;
        if (object != nullptr) {
            ReleaseRef(object);
            *slotRef = nullptr;
        }
    }
    PointerArray_Delete(old);
}

} // namespace
} // namespace ni::streamlinks

// ---- registration ---------------------------------------------------------------------

#define NIENTSTREAMING_REVERSED(name, address, fn)                                             \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

namespace esl = ni::streamlinks;

NIENTSTREAMING_REVERSED("NiEntityStreaming::ReportError", 0x00855F00, &esl::ReportError);
// (folded with MDXactPrepareSoundCueByNameCommand's: the word at +0x12)
NIENTSTREAMING_REVERSED("NiEntityStreaming::GetSceneCount", 0x00855F20, &esl::GetSceneCount);
// (folded with MDXactPrepareSoundCueByNameCommand's)
NIENTSTREAMING_REVERSED("NiEntityStreaming::GetSceneAt", 0x00855F30, &esl::GetSceneAt);
NIENTSTREAMING_REVERSED("NiEntityStreaming::LinkIDMap::RemoveAll", 0x00855F40, &esl::LinksRemoveAll);
NIENTSTREAMING_REVERSED("NiTMap<unsigned int,NiEntityPropertyInterfaceIPtr>::SetAt", 0x00855F60,
                        &esl::IPtrMapSetAt);
NIENTSTREAMING_REVERSED("NiTMap<unsigned int,NiEntityPropertyInterfaceIPtr>::GetAt", 0x00856020,
                        &esl::IPtrMapGetAt);
NIENTSTREAMING_REVERSED("NiEntityStreaming::RemoveAllScenes", 0x00856080, &esl::RemoveAllScenes);
NIENTSTREAMING_REVERSED("NiEntityStreaming::LinkIDMap::Add", 0x00856090, &esl::LinksAdd);
NIENTSTREAMING_REVERSED("NiEntityStreaming::LinkIDMap::GetObject", 0x008560D0, &esl::LinksGetObject);
// (called all over the game's code: NiNew[] of NiPointers)
NIENTSTREAMING_REVERSED("NiNew[]<NiPointer>", 0x00856120, &esl::NewPointerArray);
// (called all over the game's code: the folded instance of every NiTObjectArray<NiPointer<T>>)
NIENTSTREAMING_REVERSED("NiTObjectArray<NiPointer>::SetSize", 0x00856170, &esl::ObjectArraySetSize);
NIENTSTREAMING_REVERSED("NiEntityStreaming::~NiEntityStreaming", 0x008562F0, &esl::Destruct);
NIENTSTREAMING_REVERSED("NiEntityStreaming::DeletingDestructor", 0x00856330, &esl::DeletingDestructor);
