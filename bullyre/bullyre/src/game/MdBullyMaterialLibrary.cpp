// 0x518070 to 0x5186C0: MdBullyMaterialLibrary (the NiMaterialLibrary that hands out
// MdBullyMaterials by name, kept in an NiTFixedStringMap at 0xC67664) and
// MdBullyMaterialSwapProcessor (the render list processor that draws a list with every
// material swapped for "BullyDef" in its depth form: MdBullyMaterial's
// GetCurrentShader checks the flag at 0xC67668).

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/VCall.h"
#include "reversed.h"

namespace {

GAME_VAR(char*, ms_nullString, 0x00951028);
GAME_VAR(void*, ms_pStringTable, 0x020E216C);
GAME_VAR(uint8_t*, ms_pRenderer, 0x020E2380);
inline void* const ms_numRefObjects = reinterpret_cast<void*>(0x020E2168);
GAME_VAR(uint8_t*, ms_pMaterialLibrary, 0x00C67660);    // the MdBullyMaterialLibrary
GAME_VAR(uint8_t*, ms_pMaterials, 0x00C67664);          // NiTFixedStringMap<NiPointer<NiMaterial>>
GAME_VAR(uint8_t, ms_bSwapProcessing, 0x00C67668);

GAME_FN(void, __thiscall, StringTable_Release, 0x00754550, void* table, re::Ptr32<char>* handle, uint16_t length);
GAME_FN(char*, __cdecl, FixedString_Create, 0x00754760, const char* s);
GAME_FN(void*, __cdecl, NiNew, 0x00752460, size_t size);
GAME_FN(void*, __cdecl, NiMalloc, 0x00752300, size_t size);
GAME_FN(void, __cdecl, NiFree, 0x00752380, void* p);
GAME_FN(void, __cdecl, NiFreeSized, 0x00752540, void* p, size_t size);
GAME_FN(void*, __cdecl, ShaderLibrary_Get, 0x00756620);
GAME_FN(uint32_t, __thiscall, ShaderDesc_GetCount_7564C0, 0x007564C0, void* self);
GAME_FN(void*, __thiscall, ShaderDesc_GetAt_7564D0, 0x007564D0, void* self, uint32_t index);
GAME_FN(void, __thiscall, NiMaterialLibrary_Construct, 0x007628C0, void* self, const re::Ptr32<char>* name);
GAME_FN(void, __thiscall, NiMaterialLibrary_Destruct, 0x00762860, void* self);
GAME_FN(uint8_t*, __cdecl, NiMaterialLibrary_Get, 0x007627E0, const re::Ptr32<char>* name);
GAME_FN(void, __thiscall, StringMap_RemoveAll, 0x008C2230, void* map);
// NiTFixedStringMap<NiPointer<NiMaterial>>::SetAt: the NiPointer by value (released by it)
GAME_FN(void, __thiscall, StringMap_SetAt, 0x008B9E50, void* map, const re::Ptr32<char>* key, uint8_t* material);
GAME_FN(uint8_t, __thiscall, StringMap_GetAt, 0x00518180, void* map, const re::Ptr32<char>* key, re::Ptr32<uint8_t>* out);
GAME_FN(uint8_t*, __cdecl, MdBullyMaterial_Create, 0x005174A0, const char* name);
GAME_FN(void, __thiscall, NiPointer_AssignRaw, 0x00800420, re::Ptr32<uint8_t>* self, uint8_t* object);
GAME_FN(uint8_t*, __thiscall, MdBullyMaterialLibrary_Construct, 0x00518200, void* self);
GAME_FN(void, __thiscall, SwapProcessor_SetMaterial, 0x00518550, void* self, uint8_t* material, uint32_t extra);

constexpr uint32_t kNiRefObjectVtable = 0x008FEFD4;
constexpr uint32_t kLibraryVtable = 0x0091782C;
constexpr uint32_t kStringMapBaseVtable = 0x0091784C;
constexpr uint32_t kStringMapVtable = 0x0091785C;
constexpr uint32_t kSwapProcessorVtable = 0x0091789C;

struct MaterialLibrary {
    uint32_t vtable;     // +0
    int32_t refCount;    // +4
    re::Ptr32<char> name;          // +8 NiFixedString
    re::Ptr32<uint8_t> desc;       // +0xC the shader description being walked
    uint32_t index;      // +0x10
};
static_assert(sizeof(MaterialLibrary) == 0x14);

// NiTFixedStringMap<NiPointer<NiMaterial>>
struct StringMap {
    struct Item {
        re::Ptr32<Item> next;          // +0
        re::Ptr32<char> key;           // +4 NiFixedString
        re::Ptr32<uint8_t> material;   // +8 NiPointer
    };
    uint32_t vtable;    // +0
    uint32_t hashSize;  // +4
    re::Ptr32<re::Ptr32<Item>> buckets;     // +8
    uint32_t count;     // +0xC
};
static_assert(sizeof(StringMap) == 0x10);
static_assert(sizeof(StringMap::Item) == 0xC);

struct SwapProcessor {
    uint32_t vtable;      // +0
    int32_t refCount;     // +4
    re::Ptr32<uint8_t> material;    // +8 NiPointer
    uint32_t extra;       // +0xC the material's extra data (-1: any)
};
static_assert(sizeof(SwapProcessor) == 0x10);

// NiVisibleArray as the processor reads it
struct GeometryList {
    re::Ptr32<re::Ptr32<uint8_t>> items;  // +0
    uint32_t count;   // +4
};

void Release(uint8_t* object) {
    if (object && vc::Decrement(object + 4) == 0) {
        vc::Call<void>(object, 0x04);
    }
}

void ReleaseString(re::Ptr32<char>* handle) {
    char* s = *handle;
    if (s == ms_nullString) {
        return;
    }
    uint16_t length = *reinterpret_cast<uint16_t*>(s - 2);
    if (vc::Decrement(s - 8) == 1) {
        StringTable_Release(ms_pStringTable, handle, length);
    }
}

} // namespace

namespace MdBullyMaterialLibrary {

// 0x518070 (thiscall) vf5: the first entry of the shader library's descriptions
// (vtable[14] the first, [15] the next, skipping those without entries); null without
static void* __fastcall GetFirstEntry(MaterialLibrary* self, re::Edx) {
    void* library = ShaderLibrary_Get();
    uint8_t* desc = vc::Call<uint8_t*>(library, 0x38);
    self->desc = desc;
    self->index = 0;
    if (!desc) {
        return nullptr;
    }
    void* entries = vc::Call<void*>(desc, 0x14);
    while (ShaderDesc_GetCount_7564C0(entries) == 0) {
        desc = vc::Call<uint8_t*>(library, 0x3C);
        self->desc = desc;
        self->index = 0;
        entries = vc::Call<void*>(desc, 0x14);
    }
    if (entries && ShaderDesc_GetCount_7564C0(entries)) {
        return ShaderDesc_GetAt_7564D0(entries, self->index);
    }
    return nullptr;
}

// 0x518100 (thiscall): NiPointer<T>::operator=(const NiPointer&)
static re::Ptr32<uint8_t>* __fastcall NiPointer_Assign(re::Ptr32<uint8_t>* self, re::Edx, const re::Ptr32<uint8_t>* other) {
    uint8_t* old = *self;
    if (old != *other) {
        Release(old);
        *self = *other;
        if (*self) {
            vc::Increment(*self + 4);
        }
    }
    return self;
}

// 0x518150 (thiscall) vf0: the deleting destructor
static void* __fastcall Destroy(MaterialLibrary* self, re::Edx, int flags) {
    self->vtable = kLibraryVtable;
    self->desc = nullptr;
    self->index = 0;
    NiMaterialLibrary_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, sizeof(MaterialLibrary));
    }
    return self;
}

// 0x518180 (thiscall): NiTFixedStringMap<NiPointer<NiMaterial>>::GetAt (hashed by the
// string's pointer)
static uint8_t __fastcall GetAt(StringMap* self, re::Edx, const re::Ptr32<char>* key, re::Ptr32<uint8_t>* out) {
    StringMap::Item* item = self->buckets[re::PtrCast<uint32_t>(*key) % self->hashSize];
    for (; item; item = item->next) {
        if (item->key == *key) {
            break;
        }
    }
    if (!item) {
        return 0;
    }
    if (*out != item->material) {
        Release(*out);
        *out = item->material;
        if (*out) {
            vc::Increment(*out + 4);
        }
    }
    return 1;
}

// 0x518200 (thiscall): the constructor
static MaterialLibrary* __fastcall Construct(MaterialLibrary* self, re::Edx) {
    re::Ptr32<char> name = FixedString_Create("MdBullyMaterialLibrary");
    NiMaterialLibrary_Construct(self, &name);
    ReleaseString(&name);
    self->desc = nullptr;
    self->index = 0;
    self->vtable = kLibraryVtable;
    return self;
}

// 0x518270 (cdecl): the material map and the library deleted
static void __cdecl Shutdown() {
    if (ms_pMaterials) {
        StringMap_RemoveAll(ms_pMaterials);
        if (ms_pMaterials) {
            vc::Call<void*>(ms_pMaterials, 0x00, 1);
        }
    }
    ms_pMaterials = nullptr;
    if (ms_pMaterialLibrary) {
        vc::Call<void*>(ms_pMaterialLibrary, 0x00, 1);
    }
    ms_pMaterialLibrary = nullptr;
}

// 0x5182C0 (thiscall): NiTFixedStringMapBase's deleting destructor
static void* __fastcall StringMapBase_Destroy(StringMap* self, re::Edx, int flags) {
    self->vtable = kStringMapBaseVtable;
    StringMap_RemoveAll(self);
    NiFree(self->buckets);
    if (flags & 1) {
        NiFreeSized(self, sizeof(StringMap));
    }
    return self;
}

// 0x518300 (thiscall, `this` unused) vf2: the material by name: from the map, else a
// new MdBullyMaterial added to it. Null for the null or an empty name. (The one found
// in the map is returned after dropping the reference taken: the map keeps it)
static uint8_t* __fastcall GetMaterial(MaterialLibrary*, re::Edx, const re::Ptr32<char>* name) {
    char* s = *name;
    if (s == ms_nullString || *reinterpret_cast<uint16_t*>(s - 4) == 0) {
        return nullptr;
    }
    re::Ptr32<uint8_t> material = nullptr;
    StringMap_GetAt(ms_pMaterials, name, &material);
    if (material) {
        Release(material);
        return material;
    }
    NiPointer_AssignRaw(&material, MdBullyMaterial_Create(*name));
    if (material) {
        vc::Increment(material + 4);   // (the NiPointer passed by value)
    }
    StringMap_SetAt(ms_pMaterials, name, material);
    Release(material);
    return material;
}

// 0x5183C0 (thiscall): NiTFixedStringMap's deleting destructor
static void* __fastcall StringMap_Destroy(StringMap* self, re::Edx, int flags) {
    self->vtable = kStringMapVtable;
    StringMap_RemoveAll(self);
    self->vtable = kStringMapBaseVtable;
    StringMap_RemoveAll(self);
    NiFree(self->buckets);
    if (flags & 1) {
        NiFreeSized(self, sizeof(StringMap));
    }
    return self;
}

// 0x518400 (thiscall, `this` unused) vf1: NewItem. (A failed allocation is written
// through anyway)
static StringMap::Item* __fastcall StringMap_NewItem(StringMap*, re::Edx) {
    auto* item = static_cast<StringMap::Item*>(NiNew(sizeof(StringMap::Item)));
    if (item) {
        item->material = nullptr;
    }
    if (item->material) {
        Release(item->material);
        item->material = nullptr;
    }
    item->key = nullptr;
    return item;
}

// 0x518450 (cdecl): the library and an empty material map of 0x43 buckets
static void __cdecl Init() {
    void* memory = NiNew(sizeof(MaterialLibrary));
    ms_pMaterialLibrary = memory ? MdBullyMaterialLibrary_Construct(memory) : nullptr;
    auto* map = static_cast<StringMap*>(NiNew(sizeof(StringMap)));
    if (!map) {
        ms_pMaterials = nullptr;
        return;
    }
    map->vtable = kStringMapBaseVtable;
    map->hashSize = 0x43;
    map->count = 0;
    map->buckets = static_cast<re::Ptr32<StringMap::Item>*>(NiMalloc(0x10C));
    std::memset(map->buckets, 0, map->hashSize * 4);
    map->vtable = kStringMapVtable;
    ms_pMaterials = reinterpret_cast<uint8_t*>(map);
}

// 0x5184E0 (thiscall): NiRenderListProcessor::GetRTTI
static void* __fastcall RenderListProcessor_GetRTTI(void*, re::Edx) {
    return reinterpret_cast<void*>(0x020E2E28);
}

// 0x5184F0 (thiscall) vf3: the list drawn right away (RenderImmediate) with the flag
// set
static void __fastcall SwapProcess(SwapProcessor*, re::Edx, GeometryList* list, void*, void*) {
    if (!list) {
        return;
    }
    uint8_t* renderer = ms_pRenderer;
    ms_bSwapProcessing = 1;
    uint32_t count = list->count;
    for (uint32_t i = 0; i < count; i++) {
        vc::Call<void>(list->items[i], 0x88, renderer);
    }
    ms_bSwapProcessing = 0;
}

// 0x518540 (thiscall): NiMaterialSwapProcessor::GetRTTI
static void* __fastcall NiSwapProcessor_GetRTTI(void*, re::Edx) {
    return reinterpret_cast<void*>(0x020E2E5C);
}

// 0x518550 (thiscall): SetMaterial
static void __fastcall SetMaterial(SwapProcessor* self, re::Edx, uint8_t* material, uint32_t extra) {
    if (self->material != material) {
        Release(self->material);
        self->material = material;
        if (material) {
            vc::Increment(material + 4);
        }
    }
    self->extra = extra;
}

// 0x5185C0 (thiscall): the constructor: "BullyDef" from the MdBullyMaterialLibrary
static SwapProcessor* __fastcall SwapProcessor_Construct(SwapProcessor* self, re::Edx) {
    self->vtable = kNiRefObjectVtable;
    self->refCount = 0;
    vc::Increment(ms_numRefObjects);
    self->material = nullptr;
    self->extra = 0xFFFFFFFF;
    self->vtable = kSwapProcessorVtable;
    re::Ptr32<char> libraryName = FixedString_Create("MdBullyMaterialLibrary");
    uint8_t* library = NiMaterialLibrary_Get(&libraryName);
    ReleaseString(&libraryName);
    re::Ptr32<char> materialName = FixedString_Create("BullyDef");
    uint8_t* material = vc::Call<uint8_t*>(library, 0x08, &materialName);
    ReleaseString(&materialName);
    if (material) {
        SwapProcessor_SetMaterial(self, material, 0xFFFFFFFF);
    }
    return self;
}

// 0x5186B0 (thiscall): MdBullyMaterialSwapProcessor::GetRTTI
static void* __fastcall SwapProcessor_GetRTTI(void*, re::Edx) {
    return reinterpret_cast<void*>(0x00C67670);
}

// 0x5186C0 (thiscall): NiMaterialSwapProcessor's deleting destructor
static void* __fastcall SwapProcessor_Destroy(SwapProcessor* self, re::Edx, int flags) {
    Release(self->material);
    self->vtable = kNiRefObjectVtable;
    vc::Decrement(ms_numRefObjects);
    if (flags & 1) {
        NiFreeSized(self, sizeof(SwapProcessor));
    }
    return self;
}

} // namespace MdBullyMaterialLibrary

// ---- registration ----------------------------------------------------------------------

#define C16_MATLIB_REVERSED(name, fn, address)                                                       \
    static void* Original_##fn;                                                                      \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace MdBullyMaterialLibrary {

C16_MATLIB_REVERSED("MdBullyMaterialLibrary::GetFirstEntry", GetFirstEntry, 0x00518070);
C16_MATLIB_REVERSED("NiPointer::Assign_518100", NiPointer_Assign, 0x00518100);
C16_MATLIB_REVERSED("MdBullyMaterialLibrary::Destroy", Destroy, 0x00518150);
C16_MATLIB_REVERSED("NiTFixedStringMap<NiPointer<NiMaterial>>::GetAt", GetAt, 0x00518180);
C16_MATLIB_REVERSED("MdBullyMaterialLibrary::Construct", Construct, 0x00518200);
C16_MATLIB_REVERSED("MdBullyMaterialLibrary::Shutdown", Shutdown, 0x00518270);
C16_MATLIB_REVERSED("NiTFixedStringMapBase<NiPointer<NiMaterial>>::Destroy", StringMapBase_Destroy, 0x005182C0);
C16_MATLIB_REVERSED("MdBullyMaterialLibrary::GetMaterial", GetMaterial, 0x00518300);
C16_MATLIB_REVERSED("NiTFixedStringMap<NiPointer<NiMaterial>>::Destroy", StringMap_Destroy, 0x005183C0);
C16_MATLIB_REVERSED("NiTFixedStringMap<NiPointer<NiMaterial>>::NewItem", StringMap_NewItem, 0x00518400);
C16_MATLIB_REVERSED("MdBullyMaterialLibrary::Init", Init, 0x00518450);
C16_MATLIB_REVERSED("NiRenderListProcessor::GetRTTI", RenderListProcessor_GetRTTI, 0x005184E0);
C16_MATLIB_REVERSED("MdBullyMaterialSwapProcessor::Process", SwapProcess, 0x005184F0);
C16_MATLIB_REVERSED("NiMaterialSwapProcessor::GetRTTI", NiSwapProcessor_GetRTTI, 0x00518540);
C16_MATLIB_REVERSED("MdBullyMaterialSwapProcessor::SetMaterial", SetMaterial, 0x00518550);
C16_MATLIB_REVERSED("MdBullyMaterialSwapProcessor::Construct", SwapProcessor_Construct, 0x005185C0);
C16_MATLIB_REVERSED("MdBullyMaterialSwapProcessor::GetRTTI", SwapProcessor_GetRTTI, 0x005186B0);
C16_MATLIB_REVERSED("NiMaterialSwapProcessor::Destroy", SwapProcessor_Destroy, 0x005186C0);

} // namespace MdBullyMaterialLibrary
