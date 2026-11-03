// NiShaderFactory's statics (the one factory: NiShaderFactory::ms_pkShaderFactory, its
// callbacks and its map of global shader constants), NiGlobalConstantEntry (vtable 0x95132C:
// a constant's name, type and data) and NiShaderLibraryDesc (vtable 0x9512A8: its array of
// NiShaderDesc).

#include <cstdarg>

#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

constexpr uintptr_t kNiRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t kNiGlobalConstantEntryVtable = 0x0095132C;
constexpr uintptr_t kNiShaderLibraryDescVtable = 0x009512A8;
GAME_VAR(long, ms_uiObjects, 0x020E2168);
GAME_VAR(void*, ms_pkRenderer, 0x020E2380);

// NiTFixedStringMap<NiGlobalConstantEntry*> (a node: next, the key's handle, the value)
struct NiTMapItem {
    re::Ptr32<NiTMapItem> next;   // +0x0
    re::Ptr32<char> key;          // +0x4
    re::Ptr32<void> value;        // +0x8
};

using ClassCreateFn = uint8_t(__cdecl*)(const char* name, void* renderer, int a, int b, int c, re::Ptr32<void>* library);
using RunParserFn = uint32_t(__cdecl*)(const char* name, void* renderer, int a, int b);
using ErrorFn = uint32_t(__cdecl*)(const char* message, int a, int b);

struct NiShaderFactory {
    re::Ptr32<void> vtable;                // +0x0 (8 and 13: called by the statics; 10 InsertLibrary; 31)
    uint32_t unknown4;
    re::Ptr32<void> constantMapVtable;     // +0x8 (the global constants)
    uint32_t constantHashSize;   // +0xC
    re::Ptr32<re::Ptr32<NiTMapItem>> constantTable;  // +0x10
    uint32_t constantCount;      // +0x14
    re::Ptr32<uint8_t __cdecl(const char* name, void* renderer, int a, int b, int c, re::Ptr32<void>* library)> classCreate;   // +0x18
    re::Ptr32<uint32_t __cdecl(const char* name, void* renderer, int a, int b)> runParser;       // +0x1C
    re::Ptr32<uint32_t __cdecl(const char* message, int a, int b)> errorCallback;       // +0x20
};
GAME_VAR(NiShaderFactory*, ms_pkShaderFactory, 0x020E23FC);

struct NiGlobalConstantEntry {
    re::Ptr32<void> vtable;        // +0x0
    uint32_t refCount;   // +0x4
    re::Ptr32<char> key;           // +0x8 (NiFixedString)
    int32_t type;        // +0xC (NiShaderAttributeDesc::AttributeType)
    uint32_t dataSize;   // +0x10
    re::Ptr32<void> data;          // +0x14 (NiMalloc'd)
};
static_assert(sizeof(NiGlobalConstantEntry) == 0x18);

GAME_FN(uint32_t, __cdecl, NiShaderFactory_ReportError, 0x00756700, int result, int recoverable,
        const char* format, ...);
GAME_FN(NiGlobalConstantEntry*, __thiscall, NiGlobalConstantEntry_Construct, 0x007567E0, void* self,
        const char* key, int type, uint32_t size, const void* data);
GAME_FN(void, __thiscall, NiGlobalConstantEntry_Destruct, 0x007568A0, NiGlobalConstantEntry* self);
GAME_FN(void, __thiscall, NiGlobalConstantEntry_SetData, 0x007AB900, NiGlobalConstantEntry* self, uint32_t size,
        const void* data);
GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, const re::Ptr32<char>* other);
GAME_FN(void, __thiscall, FixedString_Destruct, 0x00517750, re::Ptr32<char>* self);
GAME_FN(void, __thiscall, ConstantMap_SetAt, 0x008C27A0, void* map, const re::Ptr32<char>* key, void* value);
GAME_FN(void, __thiscall, ConstantMap_RemoveAt, 0x008B9860, void* map, const re::Ptr32<char>* key);
// NiShaderLibraryDesc's base (NiShaderDescBase) and its array's helpers
GAME_FN(void, __thiscall, NiShaderDescBase_Construct, 0x0077E870, void* self);
GAME_FN(void, __thiscall, NiShaderDescBase_Destruct, 0x0077E8C0, void* self);
GAME_FN(void, __thiscall, NiShaderLibraryDesc_Destruct, 0x00756500, void* self);
GAME_FN(void, __thiscall, PointerArray_RemoveAll, 0x0040E7A0, void* array);
GAME_FN(void, __thiscall, PointerArray_SetAt, 0x0040E800, void* array, uint32_t index, const re::Ptr32<NiRefObject>* value);
GAME_FN(uint32_t, __thiscall, PointerArray_Add, 0x007606C0, void* array, const re::Ptr32<NiRefObject>* value);
GAME_FN(void, __cdecl, NiArrayFree, 0x00836F80, void* p);

// the global constants' map lookup
inline re::Ptr32<NiGlobalConstantEntry>* FindConstant(NiShaderFactory* factory, char* key, bool* found) {
    NiTMapItem* item = factory->constantTable[reinterpret_cast<uintptr_t>(key) % factory->constantHashSize];
    for (; item != nullptr; item = item->next) {
        if (item->key == key) {
            *found = true;
            return reinterpret_cast<re::Ptr32<NiGlobalConstantEntry>*>(&item->value);
        }
    }
    *found = false;
    return nullptr;
}

inline void ReleaseRef(NiRefObject* object) {
    if (object != nullptr && Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

} // namespace

// ---- NiShaderLibraryDesc: its NiShaderDesc array (NiTObjectArray at +0x44: vtable, base +0x48,
// max +0x4C, size +0x4E, used +0x50, grow by +0x52)

// 0x7564C0: GetShaderDescCount
uint32_t __fastcall NiShaderLibraryDesc_GetShaderDescCount(uint8_t* self, re::Edx) {
    return *reinterpret_cast<uint16_t*>(self + 0x50);
}

// 0x7564D0: GetShaderDesc
void* __fastcall NiShaderLibraryDesc_GetShaderDesc(uint8_t* self, re::Edx, uint32_t index) {
    return At<re::Ptr32<void>*>(self, 0x48)[index];
}

// 0x7564E0: GetShaderName (the desc's slot 2), null without one
const char* __fastcall NiShaderLibraryDesc_GetShaderName(uint8_t* self, re::Edx, uint32_t index) {
    void* desc = At<re::Ptr32<void>*>(self, 0x48)[index];
    if (desc != nullptr) {
        return Virtual<const char*>(desc, 2);
    }
    return nullptr;
}

// 0x756500: ~NiShaderLibraryDesc
void __fastcall NiShaderLibraryDesc_DestructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kNiShaderLibraryDescVtable;
    for (uint32_t i = 0; i < *reinterpret_cast<uint16_t*>(self + 0x4E); i++) {
        re::Ptr32<NiRefObject> empty = nullptr;
        PointerArray_SetAt(self + 0x44, i, &empty);
    }
    PointerArray_RemoveAll(self + 0x44);
    At<uint32_t>(self, 0x44) = 0x009893C0;
    NiArrayFree(At<void*>(self, 0x48));
    NiShaderDescBase_Destruct(self);
}

// 0x756570: NiShaderLibraryDesc's constructor
void* __fastcall NiShaderLibraryDesc_Construct(uint8_t* self, re::Edx) {
    NiShaderDescBase_Construct(self);
    At<uint32_t>(self, 0) = kNiShaderLibraryDescVtable;
    At<uint16_t>(self, 0x4C) = 0;
    At<uint16_t>(self, 0x52) = 1;
    At<uint16_t>(self, 0x4E) = 0;
    At<uint16_t>(self, 0x50) = 0;
    At<void*>(self, 0x48) = nullptr;
    At<uint32_t>(self, 0x44) = 0x00961DE8;
    PointerArray_RemoveAll(self + 0x44);
    return self;
}

// 0x7565B0: its deleting destructor
void* __fastcall NiShaderLibraryDesc_Destroy(uint8_t* self, re::Edx, int flags) {
    NiShaderLibraryDesc_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x54);
    }
    return self;
}

// 0x7565D0: AddShaderDesc (held through a temporary reference)
uint8_t __fastcall NiShaderLibraryDesc_AddShaderDesc(uint8_t* self, re::Edx, NiRefObject* desc) {
    if (desc == nullptr) {
        return 0;
    }
    Increment(&desc->refCount);
    re::Ptr32<NiRefObject> held = desc;
    PointerArray_Add(self + 0x44, &held);
    ReleaseRef(desc);
    return 1;
}

// ---- NiShaderFactory's statics

// 0x756620: GetInstance
void* __cdecl NiShaderFactory_GetInstance() {
    return ms_pkShaderFactory;
}

// 0x756630 and 0x756650: the factory's slots 8 and 13 (jumps)
void __cdecl NiShaderFactory_Call8() {
    if (ms_pkShaderFactory != nullptr) {
        Virtual<void>(ms_pkShaderFactory, 8);
    }
}
void __cdecl NiShaderFactory_Call13() {
    if (ms_pkShaderFactory != nullptr) {
        Virtual<void>(ms_pkShaderFactory, 13);
    }
}

// 0x756670, 0x756690, 0x7566B0: the callbacks set (false without a factory)
uint8_t __cdecl NiShaderFactory_SetClassCreationCallback(ClassCreateFn callback) {
    if (ms_pkShaderFactory == nullptr) {
        return 0;
    }
    ms_pkShaderFactory->classCreate = callback;
    return 1;
}
uint8_t __cdecl NiShaderFactory_SetRunParserCallback(RunParserFn callback) {
    if (ms_pkShaderFactory == nullptr) {
        return 0;
    }
    ms_pkShaderFactory->runParser = callback;
    return 1;
}
uint8_t __cdecl NiShaderFactory_SetErrorCallback(ErrorFn callback) {
    if (ms_pkShaderFactory == nullptr) {
        return 0;
    }
    ms_pkShaderFactory->errorCallback = callback;
    return 1;
}

// 0x7566D0: a shader's slot that does nothing (false; two arguments)
uint32_t __fastcall NiShader_ReturnFalse(void*, re::Edx, void*, void*) {
    return 0;
}

// 0x7566E0: the factory's slot 31 with the argument
void __cdecl NiShaderFactory_Call31(void* arg) {
    if (ms_pkShaderFactory != nullptr) {
        Virtual<void>(ms_pkShaderFactory, 31, arg);
    }
}

// 0x756700: NiShaderFactory::ReportError, the message formatted (2 KB, truncated) to the
// error callback; what it returns (0 without one)
uint32_t __cdecl NiShaderFactory_ReportErrorImpl(int result, int recoverable, const char* format, ...) {
    if (ms_pkShaderFactory == nullptr) {
        return 0;
    }
    char message[0x800];
    message[0] = 0;
    va_list args;
    va_start(args, format);
    crt::VsnprintfS(message, 0x800, size_t(-1), format, args);
    va_end(args);
    if (ms_pkShaderFactory->errorCallback == nullptr) {
        return 0;
    }
    return ms_pkShaderFactory->errorCallback(message, result, recoverable);
}

// 0x7567A0: GetGlobalShaderConstantEntry (ret 4: `this` unused)
NiGlobalConstantEntry* __fastcall NiShaderFactory_GetGlobalShaderConstantEntry(void*, re::Edx, const re::Ptr32<char>* key) {
    bool found;
    re::Ptr32<NiGlobalConstantEntry>* value = FindConstant(ms_pkShaderFactory, *key, &found);
    return found ? *value : nullptr;
}

// 0x7567E0: NiGlobalConstantEntry(key, type, size, data)
NiGlobalConstantEntry* __fastcall NiGlobalConstantEntry_ConstructImpl(NiGlobalConstantEntry* self, re::Edx,
                                                                      const char* key, int type, uint32_t size,
                                                                      const void* data) {
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    self->refCount = 0;
    Increment(&ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(kNiGlobalConstantEntryVtable);
    self->key = kNullFixedString;
    self->type = 0;
    self->dataSize = 0;
    self->data = nullptr;
    re::Ptr32<char> name = key != nullptr ? GlobalStringTable_AddString(key) : static_cast<char*>(kNullFixedString);
    FixedString_Assign(&self->key, &name);
    FixedStringRelease(&name);
    self->type = type;
    NiGlobalConstantEntry_SetData(self, size, data);
    return self;
}

// 0x7568A0: ~NiGlobalConstantEntry
void __fastcall NiGlobalConstantEntry_DestructImpl(NiGlobalConstantEntry* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiGlobalConstantEntryVtable);
    NiFree(self->data);
    FixedStringRelease(&self->key);
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    Decrement(&ms_uiObjects);
}

// 0x756900: its deleting destructor
NiGlobalConstantEntry* __fastcall NiGlobalConstantEntry_Destroy(NiGlobalConstantEntry* self, re::Edx, int flags) {
    NiGlobalConstantEntry_Destruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(NiGlobalConstantEntry));
    }
    return self;
}

// 0x756990: NiShaderFactory::LoadAndRegisterShaderLibrary (the class-create callback, the
// library it makes inserted: the factory's slot 10)
uint8_t __cdecl NiShaderFactory_LoadAndRegisterShaderLibrary(const char* name, int a, int b, int c) {
    NiShaderFactory* factory = ms_pkShaderFactory;
    if (factory == nullptr) {
        return 0;
    }
    if (factory->classCreate == nullptr) {
        NiShaderFactory_ReportError(1, 0, "LoadAndRegisterShaderLibrary: No valid ClassCreate call\n");
        return 0;
    }
    re::Ptr32<void> library = nullptr;
    uint8_t ok = factory->classCreate(name, ms_pkRenderer, a, b, c, &library);
    if (ok && library != nullptr) {
        Virtual<void>(factory, 10, library);
    }
    return ok;
}

// 0x756A10: NiShaderFactory::LoadAndRunParserLibrary (the run-parser callback)
uint32_t __cdecl NiShaderFactory_LoadAndRunParserLibrary(const char* name, int a, int b) {
    NiShaderFactory* factory = ms_pkShaderFactory;
    if (factory == nullptr) {
        return 0;
    }
    if (factory->runParser == nullptr) {
        NiShaderFactory_ReportError(1, 0, "LoadAndRegisterShaderLibrary: No valid RunParser call\n");
        return 0;
    }
    return factory->runParser(name, ms_pkRenderer, a, b);
}

// 0x756A60: NiShaderFactory::RegisterGlobalShaderConstant: one more reference to an existing
// one of the same type (a different type fails), or a new entry
uint8_t __cdecl NiShaderFactory_RegisterGlobalShaderConstant(const re::Ptr32<char>* key, int type, uint32_t size,
                                                             const void* data) {
    NiShaderFactory* factory = ms_pkShaderFactory;
    if (factory == nullptr) {
        return 0;
    }
    bool found;
    re::Ptr32<NiGlobalConstantEntry>* value = FindConstant(factory, *key, &found);
    if (found && *value != nullptr) {
        NiGlobalConstantEntry* entry = *value;
        if (entry->type != type) {
            return 0;
        }
        Increment(&entry->refCount);
        return 1;
    }
    void* memory = NiNew(sizeof(NiGlobalConstantEntry));
    if (memory == nullptr) {
        return 0;
    }
    NiGlobalConstantEntry* entry = NiGlobalConstantEntry_Construct(memory, *key, type, size, data);
    if (entry == nullptr) {
        return 0;
    }
    Increment(&entry->refCount);
    re::Ptr32<char> name = FixedStringAddRef(*key);
    ConstantMap_SetAt(&ms_pkShaderFactory->constantMapVtable, &name, entry);
    FixedString_Destruct(&name);
    return 1;
}

// 0x756B40: NiShaderFactory::ReleaseGlobalShaderConstant (taken out of the map with its last
// reference)
uint8_t __cdecl NiShaderFactory_ReleaseGlobalShaderConstant(const re::Ptr32<char>* key) {
    NiShaderFactory* factory = ms_pkShaderFactory;
    if (factory == nullptr) {
        return 0;
    }
    bool found;
    re::Ptr32<NiGlobalConstantEntry>* value = FindConstant(factory, *key, &found);
    if (!found || *value == nullptr) {
        return 0;
    }
    NiGlobalConstantEntry* entry = *value;
    FixedStringAddRef(*key);
    re::Ptr32<char> name = *key;
    if (entry->refCount == 1) {
        ConstantMap_RemoveAt(&ms_pkShaderFactory->constantMapVtable, &name);
    }
    ReleaseRef(reinterpret_cast<NiRefObject*>(entry));
    FixedStringRelease(&name);
    return 1;
}

// 0x756C10: NiShaderFactory::RetrieveGlobalShaderConstant (its size and data)
uint8_t __cdecl NiShaderFactory_RetrieveGlobalShaderConstant(const re::Ptr32<char>* key, uint32_t* size, re::Ptr32<void>* data) {
    NiShaderFactory* factory = ms_pkShaderFactory;
    if (factory == nullptr) {
        return 0;
    }
    bool found;
    re::Ptr32<NiGlobalConstantEntry>* value = FindConstant(factory, *key, &found);
    if (!found || *value == nullptr) {
        return 0;
    }
    *size = (*value)->dataSize;
    *data = (*value)->data;
    return 1;
}

// 0x756C70: NiShaderFactory::UpdateGlobalShaderConstant (the entry's SetData)
uint8_t __cdecl NiShaderFactory_UpdateGlobalShaderConstant(const re::Ptr32<char>* key, uint32_t size, const void* data) {
    NiShaderFactory* factory = ms_pkShaderFactory;
    if (factory == nullptr) {
        return 0;
    }
    bool found;
    re::Ptr32<NiGlobalConstantEntry>* value = FindConstant(factory, *key, &found);
    if (!found || *value == nullptr) {
        return 0;
    }
    NiGlobalConstantEntry_SetData(*value, size, data);
    return 1;
}

#define NISHADERFACTORY_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NISHADERFACTORY_REVERSED("NiShaderLibraryDesc::GetShaderDescCount", 0x007564C0, NiShaderLibraryDesc_GetShaderDescCount);
NISHADERFACTORY_REVERSED("NiShaderLibraryDesc::GetShaderDesc", 0x007564D0, NiShaderLibraryDesc_GetShaderDesc);
NISHADERFACTORY_REVERSED("NiShaderLibraryDesc::GetShaderName", 0x007564E0, NiShaderLibraryDesc_GetShaderName);
NISHADERFACTORY_REVERSED("NiShaderLibraryDesc::~NiShaderLibraryDesc", 0x00756500, NiShaderLibraryDesc_DestructImpl);
NISHADERFACTORY_REVERSED("NiShaderLibraryDesc::NiShaderLibraryDesc", 0x00756570, NiShaderLibraryDesc_Construct);
NISHADERFACTORY_REVERSED("NiShaderLibraryDesc::Destroy", 0x007565B0, NiShaderLibraryDesc_Destroy);
NISHADERFACTORY_REVERSED("NiShaderLibraryDesc::AddShaderDesc", 0x007565D0, NiShaderLibraryDesc_AddShaderDesc);
NISHADERFACTORY_REVERSED("NiShaderFactory::GetInstance", 0x00756620, NiShaderFactory_GetInstance);
NISHADERFACTORY_REVERSED("NiShaderFactory::Call8", 0x00756630, NiShaderFactory_Call8);
NISHADERFACTORY_REVERSED("NiShaderFactory::Call13", 0x00756650, NiShaderFactory_Call13);
NISHADERFACTORY_REVERSED("NiShaderFactory::SetClassCreationCallback", 0x00756670, NiShaderFactory_SetClassCreationCallback);
NISHADERFACTORY_REVERSED("NiShaderFactory::SetRunParserCallback", 0x00756690, NiShaderFactory_SetRunParserCallback);
NISHADERFACTORY_REVERSED("NiShaderFactory::SetErrorCallback", 0x007566B0, NiShaderFactory_SetErrorCallback);
NISHADERFACTORY_REVERSED("NiShader::ReturnFalse", 0x007566D0, NiShader_ReturnFalse);
NISHADERFACTORY_REVERSED("NiShaderFactory::Call31", 0x007566E0, NiShaderFactory_Call31);
NISHADERFACTORY_REVERSED("NiShaderFactory::ReportError", 0x00756700, NiShaderFactory_ReportErrorImpl);
NISHADERFACTORY_REVERSED("NiShaderFactory::GetGlobalShaderConstantEntry", 0x007567A0, NiShaderFactory_GetGlobalShaderConstantEntry);
NISHADERFACTORY_REVERSED("NiGlobalConstantEntry::NiGlobalConstantEntry", 0x007567E0, NiGlobalConstantEntry_ConstructImpl);
NISHADERFACTORY_REVERSED("NiGlobalConstantEntry::~NiGlobalConstantEntry", 0x007568A0, NiGlobalConstantEntry_DestructImpl);
NISHADERFACTORY_REVERSED("NiGlobalConstantEntry::Destroy", 0x00756900, NiGlobalConstantEntry_Destroy);
NISHADERFACTORY_REVERSED("NiShaderFactory::LoadAndRegisterShaderLibrary", 0x00756990, NiShaderFactory_LoadAndRegisterShaderLibrary);
NISHADERFACTORY_REVERSED("NiShaderFactory::LoadAndRunParserLibrary", 0x00756A10, NiShaderFactory_LoadAndRunParserLibrary);
NISHADERFACTORY_REVERSED("NiShaderFactory::RegisterGlobalShaderConstant", 0x00756A60, NiShaderFactory_RegisterGlobalShaderConstant);
NISHADERFACTORY_REVERSED("NiShaderFactory::ReleaseGlobalShaderConstant", 0x00756B40, NiShaderFactory_ReleaseGlobalShaderConstant);
NISHADERFACTORY_REVERSED("NiShaderFactory::RetrieveGlobalShaderConstant", 0x00756C10, NiShaderFactory_RetrieveGlobalShaderConstant);
NISHADERFACTORY_REVERSED("NiShaderFactory::UpdateGlobalShaderConstant", 0x00756C70, NiShaderFactory_UpdateGlobalShaderConstant);

} // namespace ni
