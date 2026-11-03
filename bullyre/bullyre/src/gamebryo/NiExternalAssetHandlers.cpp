// The entity library's external asset handlers: NiExternalAssetKFHandler ("KF": a file's
// controller sequences), NiExternalAssetKFMHandler ("KFM": actor managers made from KFM files,
// kept apart for cumulative animations) and NiExternalAssetNIFHandler ("NIF": a file's root
// NiAVObject, read with an NiStream of its own), and the string-map instantiations they use.
// Each keeps what it loaded by path and, per path, the clones it handed out (an NiTObjectSet of
// NiPointers: an asset whose clones and copies are all unreferenced can be unloaded).
//
// The handler interface (slots after NiRefObject's two; Gamebryo 2.x names, 9 a guess):
//   2 CanHandleParams(params)       3 GetNumAssetsRegistered(params)   4 Register(params)
//   5 LoadAll(params, errors, manager)   6 Load(params, errors, manager)
//   7 Unload(params, manager)       8 UnloadAll(params, manager)
//   9 UnloadUnused(params, manager) 10 GetAlwaysReload(params)  11 SetAlwaysReload(bool, params)
//   12 Retrieve(params)             13 GetAssetType(NiFixedString* result)
// The params (NiExternalAssetParams and its KF / KFM kinds, chunk 37's) are told apart by a class
// bit in what their slot 0 returns; the asset manager's slots used: 5 Register, 7 Retrieve,
// 15 GetHandler. Errors go to the error interface's ReportError (slot 2: the handler's static
// message, the file, two empty names).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiEntity.h"
#include "reversed.h"

namespace ni::entity {
namespace {

constexpr uint32_t kNiRefObjectVtable = 0x008FEFD4;
GAME_VAR(uint32_t, NiRefObject_ms_uiObjects, 0x020E2168);

constexpr uint32_t kKFHandlerVtable = 0x0098ACB0;
constexpr uint32_t kKFMHandlerVtable = 0x0098AD5C;
constexpr uint32_t kNIFHandlerVtable = 0x0098ADFC;
constexpr uint32_t kPointerStringMapVtable = 0x0098AD48;      // NiTStringPointerMap<NiPointer<T>>
constexpr uint32_t kPointerStringMapMidVtable = 0x00950718;
constexpr uint32_t kPointerStringMapBaseVtable = 0x0099ACD4;
constexpr uint32_t kPointerMapVtable = 0x0098AD94;            // NiTPointerMap (folded)
constexpr uint32_t kObjectArrayVtable = 0x009893C0;           // NiTObjectArray<NiPointer> (folded)

// the statics (the SDM functions set them)
GAME_VAR(NiFixedString, KF_ASSETTYPE, 0x020E5BFC);
GAME_VAR(NiFixedString, KF_LOAD_FAILED, 0x020E5C00);
GAME_VAR(NiFixedString, KFM_LOAD_FAILED, 0x020E5C04);
GAME_VAR(NiFixedString, KFM_NO_ACTOR_MANAGER, 0x020E5C08);
GAME_VAR(NiFixedString, KFM_ASSETTYPE, 0x020E5C0C);
GAME_VAR(NiFixedString, NIF_ASSETTYPE, 0x020E5C10);
GAME_VAR(NiFixedString, NIF_LOAD_FAILED, 0x020E5C14);
GAME_VAR(NiFixedString, NIF_ROOT_NOT_AVOBJECT, 0x020E5C18);
inline const char* Str(uintptr_t address) {
    return reinterpret_cast<const char*>(address);
}
constexpr uintptr_t kEmptyText = 0x008FEFBB;

// the class bits the params and objects are tested with
GAME_VAR(uint32_t, KFParams_ClassID, 0x020E5F3C);
GAME_VAR(uint32_t, KFMParams_ClassID, 0x020E5FA4);
GAME_VAR(uint32_t, NIFParams_ClassID, 0x020E5FD8);
GAME_VAR(uint32_t, NiAVObject_ClassID, 0x020E251C);

// the params (chunk 37's and folded accessors)
GAME_FN(re::Ptr32<const char>*, __thiscall, Params_GetAssetPath, 0x00856A30, void* params, re::Ptr32<const char>* result);
GAME_FN(void, __thiscall, Params_SetAssetPath, 0x00856A00, void* params, const re::Ptr32<const char>* path);
GAME_FN(void, __thiscall, Params_Construct, 0x00856B40, void* params);
GAME_FN(void, __thiscall, Params_Destruct, 0x00856B90, void* params);
GAME_FN(void, __thiscall, Params_GetSceneRoot, 0x00856BF0, void* params, re::Ptr32<NiRefObject>* root);
GAME_FN(void, __thiscall, Params_SetSceneRoot, 0x00829350, void* params, NiRefObject* root);   // (folded)
GAME_FN(void, __thiscall, KFParams_Construct, 0x00856870, void* params);
GAME_FN(void, __thiscall, KFParams_Destruct, 0x008568C0, void* params);
GAME_FN(uint32_t, __thiscall, KFParams_GetSequenceIndex, 0x00856850, void* params);
GAME_FN(void, __thiscall, KFParams_SetSequenceIndex, 0x00856840, void* params, uint32_t index);   // (folded)
GAME_FN(void, __thiscall, KFParams_GetSequence, 0x00856860, void* params, re::Ptr32<NiRefObject>* sequence);
GAME_FN(void, __thiscall, KFParams_SetSequence, 0x00856920, void* params, NiRefObject* sequence);
GAME_FN(uint8_t, __thiscall, KFMParams_GetCumulative, 0x005390D0, void* params);   // (folded)
GAME_FN(void, __thiscall, KFMParams_SetActorManager, 0x008C00F0, void* params, NiRefObject* manager);   // (folded)
constexpr int kParamsSize = 0x10;     // (generous: the params objects live on the stack)
constexpr int kKFParamsSize = 0x20;

// the animation library and the rest
GAME_FN(uint8_t, __cdecl, ControllerSequence_CreateAllFromFile, 0x00818240, const char* path, NiTArray<NiRefObject*>* sequences);
GAME_FN(NiRefObject*, __thiscall, KFMTool_Construct, 0x00801A80, void* self, const re::Ptr32<const char>* basePath);
GAME_FN(uint32_t, __thiscall, KFMTool_LoadFile, 0x00824AE0, void* self, const char* path);   // (0: success)
GAME_FN(const re::Ptr32<const char>*, __thiscall, KFMTool_GetFullModelPath, 0x00822A20, void* self);
GAME_FN(void, __thiscall, KFMTool_GetSequenceIDs, 0x00822730, void* self, re::Ptr32<uint32_t>* ids, uint32_t* count);
GAME_FN(re::Ptr32<const char>*, __thiscall, KFMTool_GetFullKFFilename, 0x00822AF0, void* self, re::Ptr32<const char>* result,
        uint32_t id);
GAME_FN(NiRefObject*, __cdecl, ActorManager_Create, 0x00803970, void* kfmTool, const char* path, int cumulative,
        int loadFilesFromDisk, void* stream);
GAME_FN(NiRefObject*, __thiscall, ActorManager_Clone, 0x00801780, NiRefObject* self, void* cloning);
GAME_FN(uint8_t, __thiscall, ActorManager_ChangeNIFRoot, 0x00801E80, NiRefObject* self, NiRefObject* root);
GAME_FN(uint8_t, __thiscall, ActorManager_ChangeSequence, 0x00801390, NiRefObject* self, uint32_t id,
        NiRefObject* sequence);
GAME_FN(NiRefObject*, __thiscall, Object_Clone, 0x007551F0, NiRefObject* self, void* cloning);
GAME_FN(void*, __thiscall, CloningProcess_Construct, 0x006C7000, void* self, uint32_t buckets);
GAME_FN(void, __thiscall, CloningProcess_Destruct, 0x006C6A40, void* self);
GAME_FN(void, __thiscall, Stream_Construct, 0x0075CE40, void* self);
GAME_FN(void, __thiscall, Stream_Destruct, 0x0075C020, void* self);
GAME_FN(void, __thiscall, Stream_RemoveAllObjects, 0x0075B4D0, void* self);
GAME_FN(void, __thiscall, ObjectArray_SetSize, 0x00856170, void* array, uint32_t size);
GAME_FN(void, __thiscall, ObjectArray_SetAt, 0x0040E800, void* array, uint32_t index, const re::Ptr32<ni::NiRefObject>* value);
GAME_FN(void, __thiscall, ObjectArray_RemoveAll, 0x0040E7A0, void* array);
GAME_FN(void, __cdecl, PointerArray_Delete, 0x00836F80, void* base);
GAME_FN(void, __thiscall, ObjectSet_Add, 0x00787BE0, void* set, const re::Ptr32<NiRefObject>* element);
GAME_FN(void, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<NiRefObject>* self, NiRefObject* object);
GAME_FN(void, __thiscall, Pointer_Release, 0x007549D0, re::Ptr32<NiRefObject>* self);
GAME_FN(void, __thiscall, StringMap_RemoveAt, 0x008A1F90, StringMap* self, const char* key);
// NiTStringPointerMap<NiPointer<T>>: SetAt and GetNext take / give the pointer by value / held
GAME_FN(void, __thiscall, PointerStringMap_SetAt, 0x00747380, StringMap* self, const char* key, NiRefObject* value);
GAME_FN(void, __thiscall, PointerStringMap_GetNext, 0x007474A0, StringMap* self, re::Ptr32<MapItem>* position, re::Ptr32<const char>* key,
        re::Ptr32<NiRefObject>* value);
GAME_FN(uint32_t, __cdecl, StringHash, 0x005F09F0, const char* key, uint32_t hashSize);

// this file's (called where modes apply)
GAME_FN(uint8_t, __thiscall, PointerStringMap_GetAt, 0x00845270, StringMap* self, const char* key, re::Ptr32<NiRefObject>* value);
GAME_FN(void, __thiscall, PointerStringMap_Destruct, 0x00843D50, StringMap* self);
GAME_FN(void, __thiscall, StringPointerMap_Destruct, 0x00845110, StringMap* self);
GAME_FN(void, __thiscall, ObjectSet_AddUnique, 0x00845B40, void* set, const re::Ptr32<ni::NiRefObject>* element);
GAME_FN(void, __thiscall, Pointer_Copy, 0x00843D30, re::Ptr32<NiRefObject>* self, const re::Ptr32<ni::NiRefObject>* other);
GAME_FN(uint8_t, __thiscall, KF_LoadFile, 0x00843410, void* self, const char* path, void* errors, void* manager);
GAME_FN(void, __thiscall, KF_UnloadFile, 0x00843620, void* self, const char* path);
GAME_FN(void, __thiscall, KF_UnloadAll, 0x00843BB0, void* self, void* params, void* manager);
GAME_FN(uint8_t, __thiscall, KFM_RegisterAssets, 0x00843F90, void* self, void* kfmTool, void* manager, void* errors,
        re::Ptr32<uint32_t>* ids, uint32_t* count);
GAME_FN(uint8_t, __thiscall, KFM_RetrieveAssets, 0x00844160, void* self, void* kfmTool, void* manager,
        NiRefObject* actorManager, uint32_t* ids, uint32_t count);
GAME_FN(uint8_t, __thiscall, KFM_LoadFile, 0x008444B0, void* self, const char* path, int cumulative, void* errors,
        void* manager);
GAME_FN(void, __thiscall, KFM_UnloadFile, 0x008449D0, void* self, const char* path, int cumulative);
GAME_FN(void, __thiscall, KFM_UnloadAll, 0x00844F70, void* self, void* params, void* manager);
GAME_FN(uint8_t, __thiscall, NIF_LoadFile, 0x008456D0, void* self, const char* path, void* errors, void* manager);
GAME_FN(void, __thiscall, NIF_UnloadFile, 0x00845920, void* self, const char* path);
GAME_FN(void, __thiscall, NIF_UnloadAll, 0x00845F30, void* self, void* params, void* manager);

// ---- layouts

// NiExternalAssetKFHandler (0x34)
struct KFHandler : NiRefObject {
    StringMap sequences;   // +0x8 (path -> NiTObjectArray<NiControllerSequencePtr>*; null: registered only)
    StringMap clones;      // +0x1C (path -> NiTObjectSet<NiControllerSequencePtr>*)
    uint8_t alwaysReload;  // +0x30
    uint8_t pad31[3];
};
static_assert(offsetof(KFHandler, clones) == 0x1C);
static_assert(sizeof(KFHandler) == 0x34);

// NiExternalAssetKFMHandler (0x5C)
struct KFMHandler : NiRefObject {
    StringMap managers;            // +0x8 (NiTStringPointerMap<NiActorManagerPtr>)
    StringMap managerClones;       // +0x1C (path -> NiTObjectSet<NiActorManagerPtr>*)
    StringMap cumulativeManagers;  // +0x30 (the same for cumulative animations)
    StringMap cumulativeClones;    // +0x44
    uint8_t alwaysReload;          // +0x58
    uint8_t pad59[3];
};
static_assert(offsetof(KFMHandler, cumulativeClones) == 0x44);
static_assert(sizeof(KFMHandler) == 0x5C);

// NiExternalAssetNIFHandler (0x440)
struct NIFHandler : NiRefObject {
    StringMap roots;          // +0x8 (NiTStringPointerMap<NiAVObjectPtr>)
    StringMap clones;         // +0x1C (path -> NiTObjectSet<NiAVObjectPtr>*)
    uint8_t stream[0x40C];    // +0x30 (NiStream: its top-level objects at +0x15C)
    uint8_t alwaysReload;     // +0x43C
    uint8_t pad43D[3];
};
static_assert(offsetof(NIFHandler, stream) == 0x30);
static_assert(offsetof(NIFHandler, alwaysReload) == 0x43C);
static_assert(sizeof(NIFHandler) == 0x440);

using ObjectSet = NiTSet<NiRefObject*>;

// ---- shared pieces

inline void HoldRef(NiRefObject* object) {
    if (object != nullptr) {
        ni::sys::Increment(&object->refCount);
    }
}
inline void ReleaseRef(NiRefObject* object) {
    if (object != nullptr && ni::sys::Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

// whether the object its slot `slot` describes has the class bit
inline bool HasClassBit(const void* object, int slot, uint32_t classID) {
    const uint8_t* info = Virtual<const uint8_t*>(object, slot);
    return (At<uint32_t>(info, 0xC + int(classID >> 5) * 4) & (1u << (classID & 0x1F))) != 0;
}

// the params' asset path (the temporary handle let go at once: the params keep theirs)
inline const char* AssetPath(void* params) {
    NiFixedString path = NullFixedString();
    const char* text = *Params_GetAssetPath(params, &path);
    ReleaseFixedString(&path);
    return text;
}

// a string map's first item (GetFirstPos inlined)
inline MapItem* FirstPosition(const StringMap& map) {
    for (uint32_t i = 0; i < map.hashSize; i++) {
        if (map.table[i] != nullptr) {
            return map.table[i];
        }
    }
    return nullptr;
}

// an NiTObjectSet<NiPointer> deleted (its pointers let go with the array)
inline void DeleteObjectSet(ObjectSet* set) {
    set->used = 0;
    PointerArray_Delete(set->base);
    NiDelete(set, sizeof(ObjectSet));
}

// a new empty NiTObjectSet
inline ObjectSet* NewObjectSet() {
    auto* set = static_cast<ObjectSet*>(NiNew(sizeof(ObjectSet)));
    if (set != nullptr) {
        set->base = nullptr;
        set->alloced = 0;
        set->used = 0;
    }
    return set;
}

// the clones nobody else holds dropped from a set (NiTObjectSet::RemoveAt inlined: the last one
// assigned over, its old slot not cleared)
void PruneUnused(ObjectSet* set) {
    if (set->used == 0) {
        return;
    }
    for (int32_t i = int32_t(set->used) - 1; i >= 0; i--) {
        if (set->base[i]->refCount > 1) {
            continue;
        }
        set->used--;
        NiRefObject* old = set->base[i];
        NiRefObject* last = set->base[set->used];
        if (old != last) {
            ReleaseRef(old);
            set->base[i] = last;
            HoldRef(last);
        }
    }
}

// a load failure reported: the static message, the file, two empty names (made "" first,
// second, then the file; let go in the reverse)
void ReportLoadError(void* errors, const re::Ptr32<const char>* message, const char* path) {
    NiFixedString first = ni::sys::GlobalStringTable_AddString(Str(kEmptyText));
    NiFixedString second = ni::sys::GlobalStringTable_AddString(Str(kEmptyText));
    NiFixedString file = MakeFixedString(path);
    Virtual<void>(errors, 2, message, &file, &second, &first);
    ReleaseFixedString(&file);
    ReleaseFixedString(&second);
    ReleaseFixedString(&first);
}

// a NiTStringPointerMap made as the constructors inline it
inline void InitStringMap(StringMap* map, uint32_t baseVtable, uint32_t vtable) {
    map->vtable = reinterpret_cast<void*>(baseVtable);
    map->hashSize = 0x25;
    map->count = 0;
    map->table = static_cast<re::Ptr32<MapItem>*>(NiMalloc(0x94));
    crt::Memset(map->table, 0, map->hashSize * 4);
    map->copyKeys = 1;
    map->vtable = reinterpret_cast<void*>(vtable);
}

// NiRefObject's constructor / destructor inlined
inline void ConstructRefObject(NiRefObject* self) {
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    self->refCount = 0;
    ni::sys::Increment(&NiRefObject_ms_uiObjects);
}
inline void DestructRefObject(NiRefObject* self) {
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    ni::sys::Decrement(&NiRefObject_ms_uiObjects);
}

// ---- the string-map instantiations

// 0x843D30 NiPointer<T>(const NiPointer<T>&)
re::Ptr32<NiRefObject>* __fastcall PointerCopy(re::Ptr32<NiRefObject>* self, re::Edx, const re::Ptr32<NiRefObject>* other) {
    *self = *other;
    HoldRef(*self);
    return self;
}

// the copied keys freed (NiTStringMap's destructor part)
inline void FreeKeys(StringMap* map) {
    if (map->copyKeys == 0) {
        return;
    }
    for (uint32_t i = 0; i < map->hashSize; i++) {
        for (MapItem* item = map->table[i]; item != nullptr;) {
            MapItem* next = item->next;
            NiFree(item->key);
            item = next;
        }
    }
}

// 0x843D50 ~NiTStringPointerMap<NiPointer<T>>
void __fastcall PointerStringMapDestruct(StringMap* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kPointerStringMapVtable);
    FreeKeys(self);
    self->vtable = reinterpret_cast<void*>(kPointerStringMapMidVtable);
    StringMap_RemoveAll(self);
    self->vtable = reinterpret_cast<void*>(kPointerStringMapBaseVtable);
    StringMap_RemoveAll(self);
    NiFree(self->table);
}

// 0x843DC0 ClearValue(item): a copied key freed
void __fastcall StringMap_ClearValue(StringMap* self, re::Edx, MapItem* item) {
    if (self->copyKeys != 0) {
        NiFree(item->key);
    }
}

// 0x844110 NewItem (NiPointer value: a null one; NiNew failing is dereferenced, the code's)
MapItem* __fastcall PointerStringMap_NewItem(StringMap*, re::Edx) {
    auto* item = static_cast<MapItem*>(NiNew(sizeof(MapItem)));
    if (item != nullptr) {
        item->value = nullptr;
    }
    NiRefObject* old = static_cast<NiRefObject*>(item->value);
    if (old != nullptr) {
        ReleaseRef(old);
        item->value = nullptr;
    }
    return item;
}

// 0x844380 its deleting destructor
StringMap* __fastcall PointerStringMap_DeletingDestructor(StringMap* self, re::Edx, int flags) {
    PointerStringMap_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(StringMap));
    }
    return self;
}

// 0x845110 ~NiTStringPointerMap<T> (raw values)
void __fastcall StringPointerMapDestruct(StringMap* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiTStringPointerMapVtable);
    FreeKeys(self);
    self->vtable = reinterpret_cast<void*>(kPointerMapVtable);
    StringMap_RemoveAll(self);
    self->vtable = reinterpret_cast<void*>(kNiTMapBaseStringVtable);
    StringMap_RemoveAll(self);
    NiFree(self->table);
}

// 0x845180 SetValue(item, key, value): the key copied when the map copies keys
void __fastcall StringMap_SetValue(StringMap* self, re::Edx, MapItem* item, const char* key, void* value) {
    if (self->copyKeys != 0) {
        size_t length = std::strlen(key) + 1;
        char* copy = static_cast<char*>(NiMalloc(uint32_t(length)));
        item->key = copy;
        crt::StrcpyS(copy, length, key);
        item->value = value;
        return;
    }
    item->key = const_cast<char*>(key);
    item->value = value;
}

// 0x8451E0 NiTPointerMap<T, unsigned int>'s deleting destructor
PointerMap* __fastcall ObjectIDMap_DeletingDestructor(PointerMap* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(kPointerMapVtable);
    StringMap_RemoveAll(self);
    self->vtable = reinterpret_cast<void*>(kNiTMapBaseStringVtable);
    StringMap_RemoveAll(self);
    NiFree(self->table);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(PointerMap));
    }
    return self;
}

// 0x845220 NiTStringPointerMap<T>'s deleting destructor
StringMap* __fastcall StringPointerMap_DeletingDestructor(StringMap* self, re::Edx, int flags) {
    StringPointerMap_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(StringMap));
    }
    return self;
}

// 0x845270 NiTStringPointerMap<NiPointer<T>>::GetAt(key, value): the value assigned to the
// caller's NiPointer
uint8_t __fastcall PointerStringMapGetAt(StringMap* self, re::Edx, const char* key, re::Ptr32<NiRefObject>* value) {
    for (MapItem* item = self->table[StringHash(key, self->hashSize)]; item != nullptr; item = item->next) {
        if (crt::Strcmp(key, static_cast<const char*>(item->key)) == 0) {
            NiRefObject* found = static_cast<NiRefObject*>(item->value);
            if (*value != found) {
                ReleaseRef(*value);
                *value = found;
                HoldRef(found);
            }
            return 1;
        }
    }
    return 0;
}

// an NiPointer assigned into an item, the by-value argument let go
inline void SetPointerValue(MapItem* item, NiRefObject* value) {
    NiRefObject* old = static_cast<NiRefObject*>(item->value);
    if (old != value) {
        ReleaseRef(old);
        item->value = value;
        if (value == nullptr) {
            return;
        }
        HoldRef(value);
    }
    ReleaseRef(value);
}

// 0x845310 NiTMapBase<..., NiPointer<T>>::SetValue(item, key, value by value)
void __fastcall PointerMap_SetValue(void*, re::Edx, MapItem* item, void* key, NiRefObject* value) {
    item->key = key;
    SetPointerValue(item, value);
}

// 0x845380 NiTStringPointerMap<NiPointer<T>>::SetValue(item, key, value by value)
void __fastcall PointerStringMap_SetValue(StringMap* self, re::Edx, MapItem* item, const char* key, NiRefObject* value) {
    if (self->copyKeys != 0) {
        size_t length = std::strlen(key) + 1;
        char* copy = static_cast<char*>(NiMalloc(uint32_t(length)));
        item->key = copy;
        crt::StrcpyS(copy, length, key);
    } else {
        item->key = const_cast<char*>(key);
    }
    SetPointerValue(item, value);
}

// 0x8455B0 DeleteItem(item): the value let go, the item deleted (a copied key isn't freed here)
void __fastcall PointerStringMap_DeleteItem(StringMap*, re::Edx, MapItem* item) {
    NiRefObject* value = static_cast<NiRefObject*>(item->value);
    if (value != nullptr) {
        ReleaseRef(value);
        item->value = nullptr;
    }
    if (item != nullptr) {
        ReleaseRef(static_cast<NiRefObject*>(item->value));
        NiDelete(item, sizeof(MapItem));
    }
}

// 0x845B40 NiTObjectSet<NiPointer>::AddUnique(element)
void __fastcall ObjectSetAddUnique(ObjectSet* self, re::Edx, const re::Ptr32<NiRefObject>* element) {
    for (uint32_t i = 0; i < self->used; i++) {
        if (self->base[i] == *element) {
            return;
        }
    }
    ObjectSet_Add(self, element);
}

// ---- NiExternalAssetKFHandler

// 0x8431F0 / 0x843200 GetAlwaysReload / SetAlwaysReload (only while nothing is registered)
uint8_t __fastcall KF_GetAlwaysReload(KFHandler* self, re::Edx, void*) {
    return self->alwaysReload;
}
uint8_t __fastcall KF_SetAlwaysReload(KFHandler* self, re::Edx, int alwaysReload, void*) {
    if (self->sequences.count != 0) {
        return 0;
    }
    self->alwaysReload = uint8_t(alwaysReload);
    return 1;
}

// 0x843220 / 0x843E70 / 0x845420 CanHandleParams: params of the handler's kind
template <uintptr_t ClassID>
uint8_t __fastcall CanHandleParams(void*, re::Edx, void* params) {
    if (params == nullptr) {
        return 0;
    }
    return HasClassBit(params, 0, *reinterpret_cast<const uint32_t*>(ClassID)) ? 1 : 0;
}

// 0x843260 / 0x843E40 / 0x845460 GetAssetType
template <uintptr_t AssetType>
re::Ptr32<const char>* __fastcall GetAssetType(void*, re::Edx, re::Ptr32<const char>* result) {
    *result = CopyFixedString(*reinterpret_cast<const re::Ptr32<const char>*>(AssetType));
    return result;
}

// 0x845260 GetNumAssetsRegistered (KF, NIF: the first map's count)
uint32_t __fastcall GetFirstMapCount(KFHandler* self, re::Edx, void*) {
    return self->sequences.count;
}

// 0x843290 Register(params): the path known (not loaded) unless it is already
uint8_t __fastcall KF_Register(KFHandler* self, re::Edx, void* params) {
    if (Virtual<uint8_t>(self, 2, params) == 0) {
        return 0;
    }
    const char* path = AssetPath(params);
    void* existing = nullptr;
    if (StringMap_GetAt(&self->sequences, path, &existing) == 0) {
        StringMap_SetAt(&self->sequences, path, nullptr);
    }
    return 1;
}

// 0x843320 / 0x843340 the statics
void __cdecl KF_SDMInit() {
    FixedString_AssignText(&KF_ASSETTYPE, Str(0x0098ACAC));
    FixedString_AssignText(&KF_LOAD_FAILED, Str(0x0098AC94));
}
void __cdecl KF_SDMShutdown() {
    FixedString_AssignText(&KF_ASSETTYPE, nullptr);
    FixedString_AssignText(&KF_LOAD_FAILED, nullptr);
}

// 0x843360 (always reload)
KFHandler* __fastcall KF_Construct(KFHandler* self, re::Edx, int alwaysReload) {
    ConstructRefObject(self);
    self->vtable = reinterpret_cast<void*>(kKFHandlerVtable);
    InitStringMap(&self->sequences, kNiTMapBaseStringVtable, kNiTStringPointerMapVtable);
    InitStringMap(&self->clones, kNiTMapBaseStringVtable, kNiTStringPointerMapVtable);
    self->alwaysReload = uint8_t(alwaysReload);
    return self;
}

// 0x843410 LoadFile(path, errors, manager): every sequence of the file, kept (a copy of the
// array) with an empty clone set
uint8_t __fastcall KF_LoadFileImpl(KFHandler* self, re::Edx, const char* path, void* errors, void*) {
    NiTArray<NiRefObject*> sequences = {reinterpret_cast<void*>(kObjectArrayVtable), nullptr, 0, 0, 0, 1};
    if (ControllerSequence_CreateAllFromFile(path, &sequences) == 0) {
        if (errors != nullptr) {
            ReportLoadError(errors, &KF_LOAD_FAILED, path);
        }
        sequences.vtable = reinterpret_cast<void*>(kObjectArrayVtable);
        PointerArray_Delete(sequences.base);
        return 0;
    }
    auto* copy = static_cast<NiTArray<NiRefObject*>*>(NiNew(sizeof(NiTArray<NiRefObject*>)));
    if (copy != nullptr) {
        copy->maxSize = 0;
        copy->growBy = 1;
        copy->size = 0;
        copy->esize = 0;
        copy->base = nullptr;
        copy->vtable = reinterpret_cast<void*>(kObjectArrayVtable);
    }
    ObjectArray_SetSize(copy, sequences.size);
    for (uint32_t i = 0; i < sequences.size; i++) {
        ObjectArray_SetAt(copy, i, &sequences.base[i]);
    }
    StringMap_SetAt(&self->sequences, path, copy);
    StringMap_SetAt(&self->clones, path, NewObjectSet());
    sequences.vtable = reinterpret_cast<void*>(kObjectArrayVtable);
    PointerArray_Delete(sequences.base);
    return 1;
}

// 0x843620 UnloadFile(path): the sequences dropped (the array emptied, not deleted: the code's),
// the clone set deleted
void __fastcall KF_UnloadFileImpl(KFHandler* self, re::Edx, const char* path) {
    if (path != nullptr) {
        void* array = nullptr;
        if (StringMap_GetAt(&self->sequences, path, &array) != 0 && array != nullptr) {
            ObjectArray_RemoveAll(array);
        }
        StringMap_RemoveAt(&self->sequences, path);
    }
    void* set = nullptr;
    if (StringMap_GetAt(&self->clones, path, &set) != 0 && set != nullptr) {
        StringMap_RemoveAt(&self->clones, path);
        DeleteObjectSet(static_cast<ObjectSet*>(set));
    }
}

// 0x8436B0 UnloadUnused(params, manager): files whose clones and sequences nobody holds
void __fastcall KF_UnloadUnused(KFHandler* self, re::Edx, void* params, void*) {
    if (params != nullptr && Virtual<uint8_t>(self, 2, params) == 0) {
        return;
    }
    re::Ptr32<MapItem> position = FirstPosition(self->clones);
    while (position != nullptr) {
        re::Ptr32<void> key = nullptr;
        re::Ptr32<void> value = nullptr;
        Map_GetNext(&self->clones, &position, &key, &value);
        auto* set = static_cast<ObjectSet*>(value);
        PruneUnused(set);
        if (set->used != 0) {
            continue;
        }
        const char* path = static_cast<const char*>(key);
        if (Virtual<uint8_t>(self, 10, static_cast<void*>(nullptr)) != 0) {
            KF_UnloadFile(self, path);
            continue;
        }
        void* found = nullptr;
        if (StringMap_GetAt(&self->sequences, path, &found) == 0 || found == nullptr) {
            continue;
        }
        auto* array = static_cast<NiTArray<NiRefObject*>*>(found);
        bool held = false;
        for (uint32_t j = 0; j < array->size; j++) {
            if (array->base[j]->refCount != 1) {
                held = true;
                break;
            }
        }
        if (!held) {
            KF_UnloadFile(self, path);
        }
    }
}

// 0x843820 LoadAll(params, errors, manager): every registered file not loaded yet
uint8_t __fastcall KF_LoadAll(KFHandler* self, re::Edx, void* params, void* errors, void* manager) {
    if (params != nullptr && Virtual<uint8_t>(self, 2, params) == 0) {
        return 0;
    }
    uint8_t ok = 1;
    re::Ptr32<MapItem> position = FirstPosition(self->sequences);
    while (position != nullptr) {
        re::Ptr32<void> key = nullptr;
        re::Ptr32<void> value = nullptr;
        Map_GetNext(&self->sequences, &position, &key, &value);
        if (value == nullptr && KF_LoadFile(self, static_cast<const char*>(key), errors, manager) == 0) {
            ok = 0;
        }
    }
    return ok;
}

// 0x8438D0 Load(params, errors, manager)
uint8_t __fastcall KF_Load(KFHandler* self, re::Edx, void* params, void* errors, void* manager) {
    if (Virtual<uint8_t>(self, 2, params) == 0) {
        return 0;
    }
    const char* path = AssetPath(params);
    void* loaded = nullptr;
    StringMap_GetAt(&self->sequences, path, &loaded);
    if (loaded != nullptr) {
        return 1;
    }
    return KF_LoadFile(self, path, errors, manager);
}

// 0x843970 Retrieve(params): the params' sequence (by index) handed out, or a clone of it when
// always reloading or it's held elsewhere (the clone kept in the path's set). A path never loaded
// is read through a null array: the code's
uint8_t __fastcall KF_Retrieve(KFHandler* self, re::Edx, void* params) {
    if (Virtual<uint8_t>(self, 2, params) == 0) {
        return 0;
    }
    const char* path = AssetPath(params);
    void* found = nullptr;
    uint8_t got = StringMap_GetAt(&self->sequences, path, &found);
    auto* array = static_cast<NiTArray<NiRefObject*>*>(found);
    if (got != 0 && array == nullptr) {
        return 0;
    }
    uint32_t index = KFParams_GetSequenceIndex(params);
    if (index >= array->size) {
        return 0;
    }
    bool clone = false;
    if (Virtual<uint8_t>(self, 10, static_cast<void*>(nullptr)) == 0) {
        NiRefObject* sequence = array->base[index];
        HoldRef(sequence);
        if (sequence->refCount > 2) {
            clone = true;
        }
        ReleaseRef(sequence);
        if (!clone) {
            KFParams_SetSequence(params, array->base[index]);
            return 1;
        }
    }
    uint32_t cloning[6];   // (an NiCloningProcess: 0x18 bytes, its constructor writes a byte at +0x14)
    CloningProcess_Construct(cloning, 0x25);
    cloning[2] = 1;
    re::Ptr32<NiRefObject> copy = Object_Clone(array->base[index], cloning);
    HoldRef(copy);
    KFParams_SetSequence(params, copy);
    void* set = nullptr;
    StringMap_GetAt(&self->clones, path, &set);
    ObjectSet_AddUnique(set, &copy);
    ReleaseRef(copy);
    if (cloning[0] != 0) {
        Virtual<void>(reinterpret_cast<void*>(uintptr_t(cloning[0])), 0, 1);
    }
    if (cloning[1] != 0) {
        Virtual<void>(reinterpret_cast<void*>(uintptr_t(cloning[1])), 0, 1);
    }
    return 1;
}

// 0x843B40 Unload(params, manager)
void __fastcall KF_Unload(KFHandler* self, re::Edx, void* params, void*) {
    if (Virtual<uint8_t>(self, 2, params) == 0) {
        return;
    }
    KF_UnloadFile(self, AssetPath(params));
}

// 0x843BB0 UnloadAll(params, manager): every array emptied and deleted, every clone set deleted
void __fastcall KF_UnloadAllImpl(KFHandler* self, re::Edx, void* params, void*) {
    if (params != nullptr && Virtual<uint8_t>(self, 2, params) == 0) {
        return;
    }
    re::Ptr32<MapItem> position = FirstPosition(self->sequences);
    while (position != nullptr) {
        re::Ptr32<void> key = nullptr;
        re::Ptr32<void> value = nullptr;
        Map_GetNext(&self->sequences, &position, &key, &value);
        if (value != nullptr) {
            ObjectArray_RemoveAll(value);
            Virtual<void>(value, 0, 1);
        }
    }
    StringMap_RemoveAll(&self->sequences);
    position = FirstPosition(self->clones);
    while (position != nullptr) {
        re::Ptr32<void> key = nullptr;
        re::Ptr32<void> value = nullptr;
        Map_GetNext(&self->clones, &position, &key, &value);
        DeleteObjectSet(static_cast<ObjectSet*>(value));
    }
    StringMap_RemoveAll(&self->clones);
}

// 0x843CD0 the deleting destructor
KFHandler* __fastcall KF_DeletingDestructor(KFHandler* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(kKFHandlerVtable);
    KF_UnloadAll(self, nullptr, nullptr);
    StringPointerMap_Destruct(&self->clones);
    StringPointerMap_Destruct(&self->sequences);
    DestructRefObject(self);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(KFHandler));
    }
    return self;
}

// ---- NiExternalAssetKFMHandler

// 0x843D20 / 0x843DE0
uint8_t __fastcall KFM_GetAlwaysReload(KFMHandler* self, re::Edx, void*) {
    return self->alwaysReload;
}
uint8_t __fastcall KFM_SetAlwaysReload(KFMHandler* self, re::Edx, int alwaysReload, void*) {
    if (self->managers.count != 0 || self->cumulativeManagers.count != 0) {
        return 0;
    }
    self->alwaysReload = uint8_t(alwaysReload);
    return 1;
}

// 0x843E00 GetNumAssetsRegistered(params)
uint32_t __fastcall KFM_GetNumAssetsRegistered(KFMHandler* self, re::Edx, void* params) {
    if (params == nullptr || Virtual<uint8_t>(self, 2, params) == 0) {
        return 0;
    }
    if (KFMParams_GetCumulative(params) != 0) {
        return self->cumulativeManagers.count;
    }
    return self->managers.count;
}

// the actor-manager map and clone map for cumulative or not
inline StringMap* ManagerMap(KFMHandler* self, bool cumulative) {
    return cumulative ? &self->cumulativeManagers : &self->managers;
}
inline StringMap* CloneMap(KFMHandler* self, bool cumulative) {
    return cumulative ? &self->cumulativeClones : &self->managerClones;
}

// 0x843EB0 Register(params): the path known (a null actor manager) unless it is already
uint8_t __fastcall KFM_Register(KFMHandler* self, re::Edx, void* params) {
    if (Virtual<uint8_t>(self, 2, params) == 0) {
        return 0;
    }
    StringMap* map = ManagerMap(self, KFMParams_GetCumulative(params) != 0);
    const char* path = AssetPath(params);
    if (path == nullptr) {
        return 0;
    }
    re::Ptr32<NiRefObject> existing = nullptr;
    if (PointerStringMap_GetAt(map, path, &existing) == 0) {
        PointerStringMap_SetAt(map, path, nullptr);
    }
    ReleaseRef(existing);
    return 1;
}

// 0x843F90 RegisterAssets(KFM tool, manager, errors, ids, count): its model and its sequences'
// KF files registered with the asset manager
uint8_t __fastcall KFM_RegisterAssetsImpl(KFMHandler*, re::Edx, void* kfmTool, void* manager, void*, re::Ptr32<uint32_t>* ids,
                                          uint32_t* count) {
    uint8_t ok = 1;
    const re::Ptr32<const char>* modelPath = KFMTool_GetFullModelPath(kfmTool);
    uint8_t params[kParamsSize];
    Params_Construct(params);
    Params_SetAssetPath(params, modelPath);
    if (Virtual<uint8_t>(manager, 5, static_cast<void*>(params)) == 0) {
        ok = 0;
    }
    KFMTool_GetSequenceIDs(kfmTool, ids, count);
    for (uint32_t i = 0; i < *count; i++) {
        NiFixedString file = NullFixedString();
        KFMTool_GetFullKFFilename(kfmTool, &file, (*ids)[i]);
        if (file == NullFixedString()) {
            continue;
        }
        uint8_t kfParams[kKFParamsSize];
        KFParams_Construct(kfParams);
        Params_SetAssetPath(kfParams, &file);
        if (Virtual<uint8_t>(manager, 5, static_cast<void*>(kfParams)) == 0) {
            ok = 0;
        }
        KFParams_Destruct(kfParams);
        ReleaseFixedString(&file);
    }
    Params_Destruct(params);
    return ok;
}

// 0x8440B0 / 0x8440E0 the statics
void __cdecl KFM_SDMInit() {
    FixedString_AssignText(&KFM_ASSETTYPE, Str(0x0098AD44));
    FixedString_AssignText(&KFM_LOAD_FAILED, Str(0x0098AD2C));
    FixedString_AssignText(&KFM_NO_ACTOR_MANAGER, Str(0x0098ACE8));
}
void __cdecl KFM_SDMShutdown() {
    FixedString_AssignText(&KFM_ASSETTYPE, nullptr);
    FixedString_AssignText(&KFM_LOAD_FAILED, nullptr);
    FixedString_AssignText(&KFM_NO_ACTOR_MANAGER, nullptr);
}

// 0x844160 RetrieveAssets(KFM tool, manager, actor manager, ids, count): the model retrieved and
// made the actor manager's root, then each sequence retrieved from its KF file and set (a
// sequence ID the tool doesn't know is read through null: the code's)
uint8_t __fastcall KFM_RetrieveAssetsImpl(KFMHandler*, re::Edx, void* kfmTool, void* manager, NiRefObject* actorManager,
                                          uint32_t* ids, uint32_t count) {
    const char* modelPath = *KFMTool_GetFullModelPath(kfmTool);
    if (modelPath == nullptr) {
        return 0;
    }
    uint8_t params[kParamsSize];
    Params_Construct(params);
    NiFixedString path = ni::sys::GlobalStringTable_AddString(modelPath);
    Params_SetAssetPath(params, &path);
    ReleaseFixedString(&path);
    if (Virtual<uint8_t>(manager, 7, static_cast<void*>(params)) == 0) {
        Params_Destruct(params);
        return 0;
    }
    re::Ptr32<NiRefObject> root = nullptr;
    Params_GetSceneRoot(params, &root);
    if (ActorManager_ChangeNIFRoot(actorManager, root) == 0) {
        Params_Destruct(params);
        return 0;
    }
    uint8_t kfParams[kKFParamsSize];
    KFParams_Construct(kfParams);
    const PointerMap& sequences = At<PointerMap>(kfmTool, 0x18);   // NiTMap<unsigned int, Sequence*>
    for (uint32_t i = 0; i < count; i++) {
        uint32_t id = ids[i];
        const MapItem* item = sequences.table[id % sequences.hashSize];
        while (item != nullptr && uint32_t(re::PtrCast<uintptr_t>(item->key)) != id) {
            item = item->next;
        }
        const uint8_t* sequence = item != nullptr ? static_cast<const uint8_t*>(item->value) : nullptr;
        uint32_t animIndex = *reinterpret_cast<const uint32_t*>(sequence + 8);
        NiFixedString file = NullFixedString();
        KFMTool_GetFullKFFilename(kfmTool, &file, id);
        Params_SetAssetPath(kfParams, &file);
        KFParams_SetSequenceIndex(kfParams, animIndex);
        if (Virtual<uint8_t>(manager, 7, static_cast<void*>(kfParams)) == 0) {
            ReleaseFixedString(&file);
            KFParams_Destruct(kfParams);
            Params_Destruct(params);
            return 0;
        }
        re::Ptr32<NiRefObject> controllerSequence = nullptr;
        KFParams_GetSequence(kfParams, &controllerSequence);
        ActorManager_ChangeSequence(actorManager, id, controllerSequence);
        ReleaseFixedString(&file);
    }
    KFParams_Destruct(kfParams);
    Params_Destruct(params);
    return 1;
}

// 0x8443A0 (always reload)
KFMHandler* __fastcall KFM_Construct(KFMHandler* self, re::Edx, int alwaysReload) {
    ConstructRefObject(self);
    self->vtable = reinterpret_cast<void*>(kKFMHandlerVtable);
    InitStringMap(&self->managers, kPointerStringMapBaseVtable, kPointerStringMapVtable);
    InitStringMap(&self->managerClones, kNiTMapBaseStringVtable, kNiTStringPointerMapVtable);
    InitStringMap(&self->cumulativeManagers, kPointerStringMapBaseVtable, kPointerStringMapVtable);
    InitStringMap(&self->cumulativeClones, kNiTMapBaseStringVtable, kNiTStringPointerMapVtable);
    self->alwaysReload = uint8_t(alwaysReload);
    return self;
}

// 0x8444B0 LoadFile(path, cumulative, errors, manager): the KFM read and an actor manager made
// (its model and sequences through the asset manager when there is one, else from disk), kept
// with an empty clone set
uint8_t __fastcall KFM_LoadFileImpl(KFMHandler* self, re::Edx, const char* path, int cumulative, void* errors,
                                    void* manager) {
    bool accumulate = uint8_t(cumulative) != 0;
    re::Ptr32<NiRefObject> actorManager = nullptr;
    PointerStringMap_GetAt(ManagerMap(self, accumulate), path, &actorManager);
    if (actorManager != nullptr) {
        ReleaseRef(actorManager);
        return 1;
    }
    re::Ptr32<NiRefObject> kfm = nullptr;
    void* memory = NiNew(0x40);
    if (memory != nullptr) {
        NiFixedString basePath = NullFixedString();
        kfm = KFMTool_Construct(memory, &basePath);
        ReleaseFixedString(&basePath);
    }
    HoldRef(kfm);
    if (KFMTool_LoadFile(kfm, path) != 0 || kfm == nullptr) {
        if (errors != nullptr) {
            ReportLoadError(errors, &KFM_LOAD_FAILED, path);
        }
        ReleaseRef(kfm);
        return 0;
    }
    re::Ptr32<uint32_t> ids = nullptr;
    uint32_t count = 0;
    bool failed = false;
    if (manager != nullptr) {
        if (KFM_RegisterAssets(self, kfm, manager, errors, &ids, &count) == 0) {
            failed = true;
        } else {
            Pointer_Assign(&actorManager, ActorManager_Create(kfm, path, cumulative, 0, nullptr));
            if (actorManager != nullptr &&
                KFM_RetrieveAssets(self, kfm, manager, actorManager, ids, count) == 0) {
                NiFree(ids);
                failed = true;
            }
        }
    } else {
        Pointer_Assign(&actorManager, ActorManager_Create(kfm, path, cumulative, 1, nullptr));
    }
    if (!failed && actorManager == nullptr) {
        if (errors != nullptr) {
            NiFixedString propertyName = NullFixedString();
            NiFixedString entityName = NullFixedString();
            NiFixedString file = MakeFixedString(path);
            Virtual<void>(errors, 2, &KFM_NO_ACTOR_MANAGER, &file, &entityName, &propertyName);
            FixedString_Destruct(&file);
            FixedString_Destruct(&entityName);
            FixedString_Destruct(&propertyName);
        }
        NiFree(ids);
        failed = true;
    }
    if (failed) {
        Pointer_Release(&kfm);
        Pointer_Release(&actorManager);
        return 0;
    }
    re::Ptr32<NiRefObject> copy = nullptr;
    Pointer_Copy(&copy, &actorManager);
    PointerStringMap_SetAt(ManagerMap(self, accumulate), path, copy);
    StringMap_SetAt(CloneMap(self, accumulate), path, NewObjectSet());
    NiFree(ids);
    Pointer_Release(&kfm);
    Pointer_Release(&actorManager);
    return 1;
}

// 0x844880 Retrieve(params): the actor manager handed out, or a clone of it when always
// reloading or it's held elsewhere (the clone kept in the path's set)
uint8_t __fastcall KFM_Retrieve(KFMHandler* self, re::Edx, void* params) {
    if (Virtual<uint8_t>(self, 2, params) == 0) {
        return 0;
    }
    bool cumulative = KFMParams_GetCumulative(params) != 0;
    const char* path = AssetPath(params);
    re::Ptr32<NiRefObject> actorManager = nullptr;
    StringMap* clones = CloneMap(self, cumulative);
    PointerStringMap_GetAt(ManagerMap(self, cumulative), path, &actorManager);
    if (Virtual<uint8_t>(self, 10, params) != 0 || actorManager->refCount > 2) {
        re::Ptr32<NiRefObject> clone = ActorManager_Clone(actorManager, nullptr);
        HoldRef(clone);
        KFMParams_SetActorManager(params, clone);
        void* set = nullptr;
        StringMap_GetAt(clones, path, &set);
        ObjectSet_AddUnique(set, &clone);
        ReleaseRef(clone);
    } else {
        KFMParams_SetActorManager(params, actorManager);
    }
    ReleaseRef(actorManager);
    return 1;
}

// 0x8449D0 UnloadFile(path, cumulative)
void __fastcall KFM_UnloadFileImpl(KFMHandler* self, re::Edx, const char* path, int cumulative) {
    bool accumulate = uint8_t(cumulative) != 0;
    StringMap_RemoveAt(ManagerMap(self, accumulate), path);
    StringMap* clones = CloneMap(self, accumulate);
    void* set = nullptr;
    if (StringMap_GetAt(clones, path, &set) != 0 && set != nullptr) {
        StringMap_RemoveAt(clones, path);
        DeleteObjectSet(static_cast<ObjectSet*>(set));
    }
}

// 0x844A50 UnloadUnused(params, manager): both kinds; then the model and KF handlers told to do
// the same (through the manager's handlers for plain and KF params)
void __fastcall KFM_UnloadUnused(KFMHandler* self, re::Edx, void* params, void* manager) {
    if (params != nullptr && Virtual<uint8_t>(self, 2, params) == 0) {
        return;
    }
    for (int pass = 0; pass < 2; pass++) {
        bool cumulative = pass != 0;
        StringMap* managers = ManagerMap(self, cumulative);
        StringMap* clones = CloneMap(self, cumulative);
        re::Ptr32<MapItem> position = FirstPosition(*clones);
        while (position != nullptr) {
            re::Ptr32<void> key = nullptr;
            re::Ptr32<void> value = nullptr;
            Map_GetNext(clones, &position, &key, &value);
            auto* set = static_cast<ObjectSet*>(value);
            PruneUnused(set);
            if (set->used != 0) {
                continue;
            }
            const char* path = static_cast<const char*>(key);
            if (Virtual<uint8_t>(self, 10, static_cast<void*>(nullptr)) != 0) {
                StringMap_RemoveAt(managers, path);
                void* other = nullptr;
                if (StringMap_GetAt(clones, path, &other) != 0 && other != nullptr) {
                    StringMap_RemoveAt(clones, path);
                    DeleteObjectSet(static_cast<ObjectSet*>(other));
                }
                continue;
            }
            re::Ptr32<NiRefObject> actorManager = nullptr;
            uint8_t found = PointerStringMap_GetAt(managers, path, &actorManager);
            if (found != 0 && actorManager == nullptr) {
                continue;
            }
            if (found != 0 && actorManager->refCount == 2) {
                KFM_UnloadFile(self, path, cumulative ? 1 : 0);
            }
            ReleaseRef(actorManager);
        }
    }
    if (manager == nullptr) {
        return;
    }
    uint8_t plain[kParamsSize];
    Params_Construct(plain);
    void* handler = Virtual<void*>(manager, 15, static_cast<void*>(plain));
    Virtual<void>(handler, 9, static_cast<void*>(plain), manager);
    uint8_t kf[kKFParamsSize];
    KFParams_Construct(kf);
    handler = Virtual<void*>(manager, 15, static_cast<void*>(kf));
    Virtual<void>(handler, 9, static_cast<void*>(kf), manager);
    KFParams_Destruct(kf);
    Params_Destruct(plain);
}

// 0x844CF0 LoadAll(params, errors, manager): every registered KFM not loaded yet, both kinds
uint8_t __fastcall KFM_LoadAll(KFMHandler* self, re::Edx, void* params, void* errors, void* manager) {
    if (params != nullptr && Virtual<uint8_t>(self, 2, params) == 0) {
        return 0;
    }
    uint8_t ok = 1;
    for (int pass = 0; pass < 2; pass++) {
        StringMap* map = ManagerMap(self, pass != 0);
        re::Ptr32<MapItem> position = FirstPosition(*map);
        while (position != nullptr) {
            re::Ptr32<const char> key = nullptr;
            re::Ptr32<NiRefObject> value = nullptr;
            PointerStringMap_GetNext(map, &position, &key, &value);
            if (value != nullptr) {
                ReleaseRef(value);
                continue;
            }
            if (KFM_LoadFile(self, key, pass, errors, manager) == 0) {
                ok = 0;
            }
        }
    }
    return ok;
}

// 0x844E60 Load(params, errors, manager)
uint8_t __fastcall KFM_Load(KFMHandler* self, re::Edx, void* params, void* errors, void* manager) {
    if (Virtual<uint8_t>(self, 2, params) == 0) {
        return 0;
    }
    uint8_t cumulative = KFMParams_GetCumulative(params);
    const char* path = AssetPath(params);
    return KFM_LoadFile(self, path, cumulative, errors, manager);
}

// 0x844EF0 Unload(params, manager)
void __fastcall KFM_Unload(KFMHandler* self, re::Edx, void* params, void*) {
    if (Virtual<uint8_t>(self, 2, params) == 0) {
        return;
    }
    uint8_t cumulative = KFMParams_GetCumulative(params);
    KFM_UnloadFile(self, AssetPath(params), cumulative);
}

// the clone sets of a map deleted, the map emptied
void DeleteCloneSets(StringMap* clones) {
    re::Ptr32<MapItem> position = FirstPosition(*clones);
    while (position != nullptr) {
        re::Ptr32<void> key = nullptr;
        re::Ptr32<void> value = nullptr;
        Map_GetNext(clones, &position, &key, &value);
        if (value != nullptr) {
            DeleteObjectSet(static_cast<ObjectSet*>(value));
        }
    }
    StringMap_RemoveAll(clones);
}

// 0x844F70 UnloadAll(params, manager)
void __fastcall KFM_UnloadAllImpl(KFMHandler* self, re::Edx, void* params, void*) {
    if (params != nullptr && Virtual<uint8_t>(self, 2, params) == 0) {
        return;
    }
    StringMap_RemoveAll(&self->managers);
    StringMap_RemoveAll(&self->cumulativeManagers);
    DeleteCloneSets(&self->managerClones);
    DeleteCloneSets(&self->cumulativeClones);
}

// 0x8450A0 the deleting destructor
KFMHandler* __fastcall KFM_DeletingDestructor(KFMHandler* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(kKFMHandlerVtable);
    KFM_UnloadAll(self, nullptr, nullptr);
    StringPointerMap_Destruct(&self->cumulativeClones);
    PointerStringMap_Destruct(&self->cumulativeManagers);
    StringPointerMap_Destruct(&self->managerClones);
    PointerStringMap_Destruct(&self->managers);
    DestructRefObject(self);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(KFMHandler));
    }
    return self;
}

// ---- NiExternalAssetNIFHandler

// 0x845100 / 0x845240
uint8_t __fastcall NIF_GetAlwaysReload(NIFHandler* self, re::Edx, void*) {
    return self->alwaysReload;
}
uint8_t __fastcall NIF_SetAlwaysReload(NIFHandler* self, re::Edx, int alwaysReload, void*) {
    if (self->roots.count != 0) {
        return 0;
    }
    self->alwaysReload = uint8_t(alwaysReload);
    return 1;
}

// 0x845490 Register(params)
uint8_t __fastcall NIF_Register(NIFHandler* self, re::Edx, void* params) {
    if (Virtual<uint8_t>(self, 2, params) == 0) {
        return 0;
    }
    const char* path = AssetPath(params);
    re::Ptr32<NiRefObject> existing = nullptr;
    if (PointerStringMap_GetAt(&self->roots, path, &existing) == 0) {
        PointerStringMap_SetAt(&self->roots, path, nullptr);
    }
    ReleaseRef(existing);
    return 1;
}

// 0x845550 / 0x845580 the statics
void __cdecl NIF_SDMInit() {
    FixedString_AssignText(&NIF_ASSETTYPE, Str(0x00918734));
    FixedString_AssignText(&NIF_LOAD_FAILED, Str(0x0098ADE4));
    FixedString_AssignText(&NIF_ROOT_NOT_AVOBJECT, Str(0x0098ADA8));
}
void __cdecl NIF_SDMShutdown() {
    FixedString_AssignText(&NIF_ASSETTYPE, nullptr);
    FixedString_AssignText(&NIF_LOAD_FAILED, nullptr);
    FixedString_AssignText(&NIF_ROOT_NOT_AVOBJECT, nullptr);
}

// 0x845610 (always reload)
NIFHandler* __fastcall NIF_Construct(NIFHandler* self, re::Edx, int alwaysReload) {
    ConstructRefObject(self);
    self->vtable = reinterpret_cast<void*>(kNIFHandlerVtable);
    InitStringMap(&self->roots, kPointerStringMapBaseVtable, kPointerStringMapVtable);
    InitStringMap(&self->clones, kNiTMapBaseStringVtable, kNiTStringPointerMapVtable);
    Stream_Construct(self->stream);
    self->alwaysReload = uint8_t(alwaysReload);
    return self;
}

// 0x8456D0 LoadFile(path, errors, manager): the NIF read; its first top-level object kept when
// it's an NiAVObject (with an empty clone set)
uint8_t __fastcall NIF_LoadFileImpl(NIFHandler* self, re::Edx, const char* path, void* errors, void*) {
    void* stream = self->stream;
    Stream_RemoveAllObjects(stream);
    if (Virtual<uint8_t>(stream, 3, path) == 0) {   // Load(path)
        if (errors != nullptr) {
            ReportLoadError(errors, &NIF_LOAD_FAILED, path);
        }
        return 0;
    }
    NiRefObject* top = *At<re::Ptr32<NiRefObject>*>(stream, 0x15C);
    NiRefObject* root = nullptr;
    if (top != nullptr && HasClassBit(top, 2, NiAVObject_ClassID)) {
        root = top;
    }
    if (root == nullptr) {
        if (errors != nullptr) {
            ReportLoadError(errors, &NIF_ROOT_NOT_AVOBJECT, path);
        }
        return 0;
    }
    HoldRef(root);
    HoldRef(root);   // (the map's argument, by value)
    PointerStringMap_SetAt(&self->roots, path, root);
    StringMap_SetAt(&self->clones, path, NewObjectSet());
    Stream_RemoveAllObjects(stream);
    ReleaseRef(root);
    return 1;
}

// 0x845920 UnloadFile(path)
void __fastcall NIF_UnloadFileImpl(NIFHandler* self, re::Edx, const char* path) {
    StringMap_RemoveAt(&self->roots, path);
    void* set = nullptr;
    if (StringMap_GetAt(&self->clones, path, &set) != 0 && set != nullptr) {
        StringMap_RemoveAt(&self->clones, path);
        DeleteObjectSet(static_cast<ObjectSet*>(set));
    }
}

// 0x845980 UnloadUnused(params, manager)
void __fastcall NIF_UnloadUnused(NIFHandler* self, re::Edx, void* params, void*) {
    if (params != nullptr && Virtual<uint8_t>(self, 2, params) == 0) {
        return;
    }
    re::Ptr32<MapItem> position = FirstPosition(self->clones);
    while (position != nullptr) {
        re::Ptr32<void> key = nullptr;
        re::Ptr32<void> value = nullptr;
        Map_GetNext(&self->clones, &position, &key, &value);
        auto* set = static_cast<ObjectSet*>(value);
        PruneUnused(set);
        if (set->used != 0) {
            continue;
        }
        const char* path = static_cast<const char*>(key);
        if (Virtual<uint8_t>(self, 10, static_cast<void*>(nullptr)) != 0) {
            StringMap_RemoveAt(&self->roots, path);
            void* other = nullptr;
            if (StringMap_GetAt(&self->clones, path, &other) != 0 && other != nullptr) {
                StringMap_RemoveAt(&self->clones, path);
                DeleteObjectSet(static_cast<ObjectSet*>(other));
            }
            continue;
        }
        re::Ptr32<NiRefObject> root = nullptr;
        uint8_t found = PointerStringMap_GetAt(&self->roots, path, &root);
        if (found != 0 && root == nullptr) {
            continue;
        }
        if (found != 0 && root->refCount == 2) {
            NIF_UnloadFile(self, path);
        }
        ReleaseRef(root);
    }
}

// 0x845B80 LoadAll(params, errors, manager)
uint8_t __fastcall NIF_LoadAll(NIFHandler* self, re::Edx, void* params, void* errors, void* manager) {
    if (params != nullptr && Virtual<uint8_t>(self, 2, params) == 0) {
        return 0;
    }
    uint8_t ok = 1;
    re::Ptr32<MapItem> position = FirstPosition(self->roots);
    while (position != nullptr) {
        re::Ptr32<const char> key = nullptr;
        re::Ptr32<NiRefObject> value = nullptr;
        PointerStringMap_GetNext(&self->roots, &position, &key, &value);
        if (value != nullptr) {
            ReleaseRef(value);
            continue;
        }
        if (NIF_LoadFile(self, key, errors, manager) == 0) {
            ok = 0;
        }
    }
    return ok;
}

// 0x845C60 Load(params, errors, manager)
uint8_t __fastcall NIF_Load(NIFHandler* self, re::Edx, void* params, void* errors, void* manager) {
    if (Virtual<uint8_t>(self, 2, params) == 0) {
        return 0;
    }
    const char* path = AssetPath(params);
    re::Ptr32<NiRefObject> root = nullptr;
    if (PointerStringMap_GetAt(&self->roots, path, &root) != 0 && root != nullptr) {
        ReleaseRef(root);
        return 1;
    }
    uint8_t result = NIF_LoadFile(self, path, errors, manager);
    ReleaseRef(root);
    return result;
}

// 0x845D40 Retrieve(params): the root handed out, or a clone of it when always reloading or it's
// held elsewhere (the clone kept in the path's set)
uint8_t __fastcall NIF_Retrieve(NIFHandler* self, re::Edx, void* params) {
    if (Virtual<uint8_t>(self, 2, params) == 0) {
        return 0;
    }
    Params_SetSceneRoot(params, nullptr);
    const char* path = AssetPath(params);
    re::Ptr32<NiRefObject> root = nullptr;
    if (PointerStringMap_GetAt(&self->roots, path, &root) == 0) {
        ReleaseRef(root);
        return 0;
    }
    if (root == nullptr) {
        return 0;
    }
    if (Virtual<uint8_t>(self, 10, static_cast<void*>(nullptr)) == 0 && root->refCount <= 2) {
        Params_SetSceneRoot(params, root);
    } else {
        uint32_t cloning[6];   // (an NiCloningProcess: 0x18 bytes, its constructor writes a byte at +0x14)
        CloningProcess_Construct(cloning, 0x25);
        cloning[2] = 1;
        re::Ptr32<NiRefObject> clone = Object_Clone(root, cloning);
        HoldRef(clone);
        Params_SetSceneRoot(params, clone);
        void* set = nullptr;
        StringMap_GetAt(&self->clones, path, &set);
        ObjectSet_AddUnique(set, &clone);
        Pointer_Release(&clone);
        CloningProcess_Destruct(cloning);
    }
    ReleaseRef(root);
    return 1;
}

// 0x845EC0 Unload(params, manager)
void __fastcall NIF_Unload(NIFHandler* self, re::Edx, void* params, void*) {
    if (Virtual<uint8_t>(self, 2, params) == 0) {
        return;
    }
    NIF_UnloadFile(self, AssetPath(params));
}

// 0x845F30 UnloadAll(params, manager)
void __fastcall NIF_UnloadAllImpl(NIFHandler* self, re::Edx, void* params, void*) {
    if (params != nullptr && Virtual<uint8_t>(self, 2, params) == 0) {
        return;
    }
    StringMap_RemoveAll(&self->roots);
    DeleteCloneSets(&self->clones);
}

// 0x845FE0 the deleting destructor
NIFHandler* __fastcall NIF_DeletingDestructor(NIFHandler* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(kNIFHandlerVtable);
    NIF_UnloadAll(self, nullptr, nullptr);
    Stream_Destruct(self->stream);
    StringPointerMap_Destruct(&self->clones);
    PointerStringMap_Destruct(&self->roots);
    DestructRefObject(self);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(NIFHandler));
    }
    return self;
}

} // namespace

#define NIASSETHANDLER_REVERSED(name, address, fn)                                             \
    static void* RE_CONCAT(Original_, __LINE__);                                                \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIASSETHANDLER_REVERSED("NiExternalAssetKFHandler::GetAlwaysReload", 0x008431F0, &KF_GetAlwaysReload);
NIASSETHANDLER_REVERSED("NiExternalAssetKFHandler::SetAlwaysReload", 0x00843200, &KF_SetAlwaysReload);
NIASSETHANDLER_REVERSED("NiExternalAssetKFHandler::CanHandleParams", 0x00843220, &CanHandleParams<0x020E5F3C>);
NIASSETHANDLER_REVERSED("NiExternalAssetKFHandler::GetAssetType", 0x00843260, &GetAssetType<0x020E5BFC>);
NIASSETHANDLER_REVERSED("NiExternalAssetKFHandler::Register", 0x00843290, &KF_Register);
NIASSETHANDLER_REVERSED("NiExternalAssetKFHandler::_SDMInit", 0x00843320, &KF_SDMInit);
NIASSETHANDLER_REVERSED("NiExternalAssetKFHandler::_SDMShutdown", 0x00843340, &KF_SDMShutdown);
NIASSETHANDLER_REVERSED("NiExternalAssetKFHandler::NiExternalAssetKFHandler", 0x00843360, &KF_Construct);
NIASSETHANDLER_REVERSED("NiExternalAssetKFHandler::LoadFile", 0x00843410, &KF_LoadFileImpl);
NIASSETHANDLER_REVERSED("NiExternalAssetKFHandler::UnloadFile", 0x00843620, &KF_UnloadFileImpl);
NIASSETHANDLER_REVERSED("NiExternalAssetKFHandler::UnloadUnused", 0x008436B0, &KF_UnloadUnused);
NIASSETHANDLER_REVERSED("NiExternalAssetKFHandler::LoadAll", 0x00843820, &KF_LoadAll);
NIASSETHANDLER_REVERSED("NiExternalAssetKFHandler::Load", 0x008438D0, &KF_Load);
NIASSETHANDLER_REVERSED("NiExternalAssetKFHandler::Retrieve", 0x00843970, &KF_Retrieve);
NIASSETHANDLER_REVERSED("NiExternalAssetKFHandler::Unload", 0x00843B40, &KF_Unload);
NIASSETHANDLER_REVERSED("NiExternalAssetKFHandler::UnloadAll", 0x00843BB0, &KF_UnloadAllImpl);
NIASSETHANDLER_REVERSED("NiExternalAssetKFHandler::DeletingDestructor", 0x00843CD0, &KF_DeletingDestructor);
NIASSETHANDLER_REVERSED("NiExternalAssetKFMHandler::GetAlwaysReload", 0x00843D20, &KFM_GetAlwaysReload);
NIASSETHANDLER_REVERSED("NiPointer::NiPointer(copy)_843D30", 0x00843D30, &PointerCopy);
NIASSETHANDLER_REVERSED("NiTStringPointerMap<NiPointer>::~NiTStringPointerMap", 0x00843D50, &PointerStringMapDestruct);
NIASSETHANDLER_REVERSED("NiTStringMap::ClearValue_843DC0", 0x00843DC0, &StringMap_ClearValue);
NIASSETHANDLER_REVERSED("NiExternalAssetKFMHandler::SetAlwaysReload", 0x00843DE0, &KFM_SetAlwaysReload);
NIASSETHANDLER_REVERSED("NiExternalAssetKFMHandler::GetNumAssetsRegistered", 0x00843E00, &KFM_GetNumAssetsRegistered);
NIASSETHANDLER_REVERSED("NiExternalAssetKFMHandler::GetAssetType", 0x00843E40, &GetAssetType<0x020E5C0C>);
NIASSETHANDLER_REVERSED("NiExternalAssetKFMHandler::CanHandleParams", 0x00843E70, &CanHandleParams<0x020E5FA4>);
NIASSETHANDLER_REVERSED("NiExternalAssetKFMHandler::Register", 0x00843EB0, &KFM_Register);
NIASSETHANDLER_REVERSED("NiExternalAssetKFMHandler::RegisterAssets", 0x00843F90, &KFM_RegisterAssetsImpl);
NIASSETHANDLER_REVERSED("NiExternalAssetKFMHandler::_SDMInit", 0x008440B0, &KFM_SDMInit);
NIASSETHANDLER_REVERSED("NiExternalAssetKFMHandler::_SDMShutdown", 0x008440E0, &KFM_SDMShutdown);
NIASSETHANDLER_REVERSED("NiTStringPointerMap<NiPointer>::NewItem", 0x00844110, &PointerStringMap_NewItem);
NIASSETHANDLER_REVERSED("NiExternalAssetKFMHandler::RetrieveAssets", 0x00844160, &KFM_RetrieveAssetsImpl);
NIASSETHANDLER_REVERSED("NiTStringPointerMap<NiPointer>::DeletingDestructor", 0x00844380,
                        &PointerStringMap_DeletingDestructor);
NIASSETHANDLER_REVERSED("NiExternalAssetKFMHandler::NiExternalAssetKFMHandler", 0x008443A0, &KFM_Construct);
NIASSETHANDLER_REVERSED("NiExternalAssetKFMHandler::LoadFile", 0x008444B0, &KFM_LoadFileImpl);
NIASSETHANDLER_REVERSED("NiExternalAssetKFMHandler::Retrieve", 0x00844880, &KFM_Retrieve);
NIASSETHANDLER_REVERSED("NiExternalAssetKFMHandler::UnloadFile", 0x008449D0, &KFM_UnloadFileImpl);
NIASSETHANDLER_REVERSED("NiExternalAssetKFMHandler::UnloadUnused", 0x00844A50, &KFM_UnloadUnused);
NIASSETHANDLER_REVERSED("NiExternalAssetKFMHandler::LoadAll", 0x00844CF0, &KFM_LoadAll);
NIASSETHANDLER_REVERSED("NiExternalAssetKFMHandler::Load", 0x00844E60, &KFM_Load);
NIASSETHANDLER_REVERSED("NiExternalAssetKFMHandler::Unload", 0x00844EF0, &KFM_Unload);
NIASSETHANDLER_REVERSED("NiExternalAssetKFMHandler::UnloadAll", 0x00844F70, &KFM_UnloadAllImpl);
NIASSETHANDLER_REVERSED("NiExternalAssetKFMHandler::DeletingDestructor", 0x008450A0, &KFM_DeletingDestructor);
NIASSETHANDLER_REVERSED("NiExternalAssetNIFHandler::GetAlwaysReload", 0x00845100, &NIF_GetAlwaysReload);
NIASSETHANDLER_REVERSED("NiTStringPointerMap::~NiTStringPointerMap_845110", 0x00845110, &StringPointerMapDestruct);
NIASSETHANDLER_REVERSED("NiTStringPointerMap::SetValue_845180", 0x00845180, &StringMap_SetValue);
NIASSETHANDLER_REVERSED("NiTPointerMap<T,uint>::DeletingDestructor_8451E0", 0x008451E0, &ObjectIDMap_DeletingDestructor);
NIASSETHANDLER_REVERSED("NiTStringPointerMap::DeletingDestructor_845220", 0x00845220, &StringPointerMap_DeletingDestructor);
NIASSETHANDLER_REVERSED("NiExternalAssetNIFHandler::SetAlwaysReload", 0x00845240, &NIF_SetAlwaysReload);
NIASSETHANDLER_REVERSED("NiExternalAssetHandler::GetNumAssetsRegistered_845260", 0x00845260, &GetFirstMapCount);
NIASSETHANDLER_REVERSED("NiTStringPointerMap<NiPointer>::GetAt", 0x00845270, &PointerStringMapGetAt);
NIASSETHANDLER_REVERSED("NiTMapBase<NiPointer>::SetValue_845310", 0x00845310, &PointerMap_SetValue);
NIASSETHANDLER_REVERSED("NiTStringPointerMap<NiPointer>::SetValue", 0x00845380, &PointerStringMap_SetValue);
NIASSETHANDLER_REVERSED("NiExternalAssetNIFHandler::CanHandleParams", 0x00845420, &CanHandleParams<0x020E5FD8>);
NIASSETHANDLER_REVERSED("NiExternalAssetNIFHandler::GetAssetType", 0x00845460, &GetAssetType<0x020E5C10>);
NIASSETHANDLER_REVERSED("NiExternalAssetNIFHandler::Register", 0x00845490, &NIF_Register);
NIASSETHANDLER_REVERSED("NiExternalAssetNIFHandler::_SDMInit", 0x00845550, &NIF_SDMInit);
NIASSETHANDLER_REVERSED("NiExternalAssetNIFHandler::_SDMShutdown", 0x00845580, &NIF_SDMShutdown);
NIASSETHANDLER_REVERSED("NiTStringPointerMap<NiPointer>::DeleteItem", 0x008455B0, &PointerStringMap_DeleteItem);
NIASSETHANDLER_REVERSED("NiExternalAssetNIFHandler::NiExternalAssetNIFHandler", 0x00845610, &NIF_Construct);
NIASSETHANDLER_REVERSED("NiExternalAssetNIFHandler::LoadFile", 0x008456D0, &NIF_LoadFileImpl);
NIASSETHANDLER_REVERSED("NiExternalAssetNIFHandler::UnloadFile", 0x00845920, &NIF_UnloadFileImpl);
NIASSETHANDLER_REVERSED("NiExternalAssetNIFHandler::UnloadUnused", 0x00845980, &NIF_UnloadUnused);
NIASSETHANDLER_REVERSED("NiTObjectSet<NiPointer>::AddUnique", 0x00845B40, &ObjectSetAddUnique);
NIASSETHANDLER_REVERSED("NiExternalAssetNIFHandler::LoadAll", 0x00845B80, &NIF_LoadAll);
NIASSETHANDLER_REVERSED("NiExternalAssetNIFHandler::Load", 0x00845C60, &NIF_Load);
NIASSETHANDLER_REVERSED("NiExternalAssetNIFHandler::Retrieve", 0x00845D40, &NIF_Retrieve);
NIASSETHANDLER_REVERSED("NiExternalAssetNIFHandler::Unload", 0x00845EC0, &NIF_Unload);
NIASSETHANDLER_REVERSED("NiExternalAssetNIFHandler::UnloadAll", 0x00845F30, &NIF_UnloadAllImpl);
NIASSETHANDLER_REVERSED("NiExternalAssetNIFHandler::DeletingDestructor", 0x00845FE0, &NIF_DeletingDestructor);

} // namespace ni::entity
