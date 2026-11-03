// NiAVObjectPalette (vtable 0x961570, 8 bytes: an NiObject naming the scene's objects; its
// viewer strings and comparison are in NiLODData.cpp / NiAlphaAccumulator.cpp) and
// NiDefaultAVObjectPalette (vtable 0x9615C0, 0x20 bytes): a string-keyed map of the objects
// (NiTStringPointerMap<NiAVObject*> at +0x8: 37 buckets, the keys copied) and the scene it looks
// names up in (+0x1C, not a reference). Slots 18 GetAVObject(name), 19 SetAVObject(name, object).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiAVObjectPalette_RTTI, 0x020E42AC);
GAME_VAR(RTTI, NiDefaultAVObjectPalette_RTTI, 0x020E3EB4);

constexpr uintptr_t NiAVObjectPalette_vtable = 0x00961570;
constexpr uintptr_t NiDefaultAVObjectPalette_vtable = 0x009615C0;
constexpr uintptr_t MapBase_vtable = 0x00950760;      // NiTMapBase
constexpr uintptr_t StringMap_vtable = 0x00951798;    // NiTStringPointerMap<NiAVObject*>

// a map item: the next in the bucket, the key, the value
struct MapItem {
    re::Ptr32<MapItem> next;   // +0x0
    re::Ptr32<void> key;       // +0x4
    re::Ptr32<void> value;     // +0x8
};

// NiTStringPointerMap<NiAVObject*>
struct StringMap {
    re::Ptr32<void> vtable;          // +0x0
    uint32_t hashSize;     // +0x4
    re::Ptr32<re::Ptr32<MapItem>> buckets;     // +0x8
    uint32_t count;        // +0xC
    uint8_t copyKeys;      // +0x10
    uint8_t pad11[3];
};
static_assert(sizeof(StringMap) == 0x14);

struct NiDefaultAVObjectPalette : NiObject {
    StringMap objects;    // +0x8
    re::Ptr32<NiAVObject> scene;    // +0x1C
};
static_assert(offsetof(NiDefaultAVObjectPalette, scene) == 0x1C);
static_assert(sizeof(NiDefaultAVObjectPalette) == 0x20);

// NiTPointerMap<NiObject*, NiObject*>: the cloning process's clone map (its first member)
struct CloneMap {
    re::Ptr32<void> vtable;          // +0x0
    uint32_t hashSize;     // +0x4
    re::Ptr32<re::Ptr32<MapItem>> buckets;     // +0x8
};

GAME_FN(uint8_t, __thiscall, AVObjectPalette_IsEqual, 0x007F79B0, void* self, void* other);
GAME_FN(void, __thiscall, AVObjectPalette_LoadBinary, 0x007F79A0, void* self, void* stream);
GAME_FN(void, __thiscall, AVObjectPalette_SaveBinary, 0x00806A50, void* self, void* stream);
GAME_FN(void, __thiscall, AVObjectPalette_CopyMembers, 0x007F7990, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, AVObjectPalette_GetViewerStrings, 0x007F79D0, void* self, NiTArray<char*>* strings);
GAME_FN(MapItem*, __thiscall, Map_GetFirstPos, 0x008CA4E0, StringMap* map);
GAME_FN(void, __thiscall, Map_GetNext, 0x0083D890, StringMap* map, re::Ptr32<MapItem>* position, re::Ptr32<char>* key, re::Ptr32<void>* value);
GAME_FN(uint8_t, __thiscall, Map_GetAt, 0x00800470, StringMap* map, const char* key, re::Ptr32<void>* value);
GAME_FN(void, __thiscall, Map_RemoveAt, 0x008A1F90, StringMap* map, const char* key);
GAME_FN(void, __thiscall, Map_SetAt, 0x00747F00, StringMap* map, const char* key, void* value);
GAME_FN(void, __thiscall, Map_Destruct, 0x0075A4E0, StringMap* map);
GAME_FN(void*, __thiscall, Stream_ReadLinkIDObject, 0x0075A820, void* stream);
GAME_FN(void, __thiscall, Stream_SaveCString, 0x0075AFB0, void* stream, const char* string);
GAME_FN(void, __thiscall, Stream_LoadCString, 0x0075AEA0, void* stream, re::Ptr32<char>* string);
// (this file's, called where modes apply)
GAME_FN(NiDefaultAVObjectPalette*, __thiscall, DefaultPalette_Construct, 0x007DE780, void* self, NiAVObject* scene);

// the clone map's entry for a key (the value left as given when there's none)
inline void* LookUp(CloneMap* map, void* key, void* missing) {
    for (MapItem* item = map->buckets[uintptr_t(key) % map->hashSize]; item != nullptr; item = item->next) {
        if (item->key == key) {
            return item->value;
        }
    }
    return missing;
}

// the map's first position (its first non-empty bucket), as the code inlines it
inline MapItem* FirstPosition(StringMap* map) {
    for (uint32_t i = 0; i < map->hashSize; i++) {
        if (map->buckets[i] != nullptr) {
            return map->buckets[i];
        }
    }
    return nullptr;
}

// ---- NiAVObjectPalette

const RTTI* __fastcall PaletteGetRTTI(void*, re::Edx) {
    return &NiAVObjectPalette_RTTI;
}

NiObject* __fastcall PaletteDeletingDestructor(NiObject* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiAVObjectPalette_vtable);
    Object_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 8);
    }
    return self;
}

// ---- NiDefaultAVObjectPalette

// slot 11 ProcessClone: NiObject's; the clone's scene set to the scene's clone (the clone itself
// when the scene wasn't cloned: the game reuses the variable); each object's clone put in the
// clone's map under its name
void __fastcall DefaultProcessClone(NiDefaultAVObjectPalette* self, re::Edx, void* cloning) {
    Object_ProcessClone(self, cloning);
    CloneMap* map = *static_cast<re::Ptr32<CloneMap>*>(cloning);
    NiDefaultAVObjectPalette* clone = static_cast<NiDefaultAVObjectPalette*>(LookUp(map, self, nullptr));
    if (self->scene != nullptr) {
        clone->scene = static_cast<NiAVObject*>(LookUp(map, self->scene, clone));
    }
    if (self->objects.count == 0) {
        return;
    }
    re::Ptr32<MapItem> position = FirstPosition(&self->objects);
    while (position != nullptr) {
        re::Ptr32<char> name;
        re::Ptr32<void> object;
        Map_GetNext(&self->objects, &position, &name, &object);
        if (name == nullptr || object == nullptr) {
            continue;
        }
        CloneMap* clones = *static_cast<re::Ptr32<CloneMap>*>(cloning);
        for (MapItem* item = clones->buckets[uintptr_t(object) % clones->hashSize]; item != nullptr;
             item = item->next) {
            if (item->key == object) {
                Virtual<void>(clone, 19, static_cast<const char*>(name), item->value);   // SetAVObject
                break;
            }
        }
    }
}

// the scene's link, then the named objects (each name NiMalloc'd and freed after)
void __fastcall DefaultLoadBinary(NiDefaultAVObjectPalette* self, re::Edx, void* stream) {
    AVObjectPalette_LoadBinary(self, stream);
    self->scene = static_cast<NiAVObject*>(Stream_ReadLinkIDObject(stream));
    uint32_t count = 0;
    LoadBytes(stream, &count, 4);
    while (count != 0) {
        count--;
        re::Ptr32<char> name = nullptr;
        Stream_LoadCString(stream, &name);
        void* object = Stream_ReadLinkIDObject(stream);
        if (name != nullptr && object != nullptr) {
            Virtual<void>(self, 19, static_cast<const char*>(name), object);   // SetAVObject
        }
        NiFree(name);
    }
}

// the scene's link, the count, each name and object link
void __fastcall DefaultSaveBinary(NiDefaultAVObjectPalette* self, re::Edx, void* stream) {
    AVObjectPalette_SaveBinary(self, stream);
    Virtual<void>(stream, 12, self->scene);   // SaveLinkID
    uint32_t count = self->objects.count;
    SaveBytes(stream, &count, 4);
    if (self->objects.count == 0) {
        return;
    }
    re::Ptr32<MapItem> position = FirstPosition(&self->objects);
    while (position != nullptr) {
        re::Ptr32<char> name;
        re::Ptr32<void> object;
        Map_GetNext(&self->objects, &position, &name, &object);
        Stream_SaveCString(stream, name);
        Virtual<void>(stream, 12, object);
    }
}

// the base comparison, the same count, and every name found in the other (slot 18)
uint8_t __fastcall DefaultIsEqual(NiDefaultAVObjectPalette* self, re::Edx, NiDefaultAVObjectPalette* other) {
    if (!AVObjectPalette_IsEqual(self, other) || other->objects.count != self->objects.count) {
        return 0;
    }
    if (self->objects.count == 0) {
        return 1;
    }
    re::Ptr32<MapItem> position = Map_GetFirstPos(&self->objects);
    while (position != nullptr) {
        re::Ptr32<char> name;
        re::Ptr32<void> object;
        Map_GetNext(&self->objects, &position, &name, &object);
        if (Virtual<void*>(other, 18, static_cast<const char*>(name)) == nullptr) {
            return 0;
        }
    }
    return 1;
}

// NiDefaultAVObjectPalette(scene): an empty map of 37 buckets (keys copied)
NiDefaultAVObjectPalette* __fastcall DefaultConstruct(NiDefaultAVObjectPalette* self, re::Edx, NiAVObject* scene) {
    RefObject_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiDefaultAVObjectPalette_vtable);
    self->objects.vtable = reinterpret_cast<void*>(MapBase_vtable);
    self->objects.hashSize = 0x25;
    self->objects.count = 0;
    self->objects.buckets = static_cast<re::Ptr32<MapItem>*>(NiMalloc(0x94));
    crt::Memset(self->objects.buckets, 0, self->objects.hashSize * 4);
    self->objects.copyKeys = 1;
    self->objects.vtable = reinterpret_cast<void*>(StringMap_vtable);
    self->scene = scene;
    return self;
}

const RTTI* __fastcall DefaultGetRTTI(void*, re::Edx) {
    return &NiDefaultAVObjectPalette_RTTI;
}

// slot 18 GetAVObject(name): from the map, else looked up in the scene (its slot 23
// GetObjectByName) and kept in the map
NiAVObject* __fastcall DefaultGetAVObject(NiDefaultAVObjectPalette* self, re::Edx, const char* name) {
    re::Ptr32<void> found = nullptr;
    if (Map_GetAt(&self->objects, name, &found)) {
        return static_cast<NiAVObject*>(found);
    }
    if (self->scene == nullptr) {
        return nullptr;
    }
    NiFixedString key = name != nullptr ? StringTable_AddString(name) : static_cast<const char*>(NullFixedString);
    NiAVObject* object = Virtual<NiAVObject*>(self->scene, 23, static_cast<const re::Ptr32<const char>*>(&key));
    FixedStringRelease(&key);
    if (object != nullptr) {
        Virtual<void>(self, 19, name, static_cast<void*>(object));   // SetAVObject
    }
    return object;
}

// slot 19 SetAVObject(name, object): removed for none, else set
void __fastcall DefaultSetAVObject(NiDefaultAVObjectPalette* self, re::Edx, const char* name, NiAVObject* object) {
    if (object == nullptr) {
        Map_RemoveAt(&self->objects, name);
        return;
    }
    Map_SetAt(&self->objects, name, object);
}

// the deleting destructor: the map's, NiAVObjectPalette's (NiObject's)
NiDefaultAVObjectPalette* __fastcall DefaultDeletingDestructor(NiDefaultAVObjectPalette* self, re::Edx,
                                                               uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiDefaultAVObjectPalette_vtable);
    Map_Destruct(&self->objects);
    self->vtable = reinterpret_cast<void*>(NiAVObjectPalette_vtable);
    Object_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x20);
    }
    return self;
}

// vf3 CreateClone: an empty palette with NiObject's members (the objects come in ProcessClone)
NiDefaultAVObjectPalette* __fastcall DefaultCreateClone(NiDefaultAVObjectPalette* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x20);
    NiDefaultAVObjectPalette* clone = memory != nullptr ? DefaultPalette_Construct(memory, nullptr) : nullptr;
    AVObjectPalette_CopyMembers(self, clone, cloning);
    return clone;
}

NiDefaultAVObjectPalette* __cdecl DefaultCreateObject() {
    void* memory = NiNew(0x20);
    if (memory == nullptr) {
        return nullptr;
    }
    return DefaultPalette_Construct(memory, nullptr);
}

void __fastcall DefaultGetViewerStrings(NiDefaultAVObjectPalette* self, re::Edx, NiTArray<char*>* strings) {
    AVObjectPalette_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiDefaultAVObjectPalette_RTTI.name));
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIAVPALETTE_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

RE_UNHOOKED("NiDefaultAVObjectPalette::RegisterStreamables (jump)", 0x007DE410);   // -> 0x7D1D10

NIAVPALETTE_REVERSED("NiAVObjectPalette::GetRTTI", 0x007DE420, ni::PaletteGetRTTI);
NIAVPALETTE_REVERSED("NiAVObjectPalette::DeletingDestructor", 0x007DE430, ni::PaletteDeletingDestructor);
NIAVPALETTE_REVERSED("NiDefaultAVObjectPalette::ProcessClone", 0x007DE460, ni::DefaultProcessClone);
NIAVPALETTE_REVERSED("NiDefaultAVObjectPalette::LoadBinary", 0x007DE580, ni::DefaultLoadBinary);
NIAVPALETTE_REVERSED("NiDefaultAVObjectPalette::SaveBinary", 0x007DE630, ni::DefaultSaveBinary);
NIAVPALETTE_REVERSED("NiDefaultAVObjectPalette::IsEqual", 0x007DE700, ni::DefaultIsEqual);
NIAVPALETTE_REVERSED("NiDefaultAVObjectPalette::NiDefaultAVObjectPalette", 0x007DE780, ni::DefaultConstruct);
NIAVPALETTE_REVERSED("NiDefaultAVObjectPalette::GetRTTI", 0x007DE7E0, ni::DefaultGetRTTI);
NIAVPALETTE_REVERSED("NiDefaultAVObjectPalette::GetAVObject", 0x007DE7F0, ni::DefaultGetAVObject);
NIAVPALETTE_REVERSED("NiDefaultAVObjectPalette::SetAVObject", 0x007DE8B0, ni::DefaultSetAVObject);
NIAVPALETTE_REVERSED("NiDefaultAVObjectPalette::DeletingDestructor", 0x007DE8E0, ni::DefaultDeletingDestructor);
NIAVPALETTE_REVERSED("NiDefaultAVObjectPalette::CreateClone", 0x007DE920, ni::DefaultCreateClone);
NIAVPALETTE_REVERSED("NiDefaultAVObjectPalette::CreateObject", 0x007DE970, ni::DefaultCreateObject);
NIAVPALETTE_REVERSED("NiDefaultAVObjectPalette::GetViewerStrings", 0x007DE990, ni::DefaultGetViewerStrings);
