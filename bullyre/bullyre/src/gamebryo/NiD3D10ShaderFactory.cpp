// NiD3D10ShaderFactory (vtable 0x9A11D0, 0x68 bytes; the one instance at 0x20E91A4, made by Create
// from the renderer's _SDMInit and made NiShaderFactory's own by TakeOver): the D3D10 renderer's
// NiShaderFactory, as NiD3DShaderFactory (src/gamebryo/NiD3DShaderFactory.cpp) is the DX9 one's,
// but with NiTFixedStringMaps. Its shaders are kept by "name + implementation" ("%s%d"), its
// shader libraries by name; a shader asked for that isn't there is made by the libraries in turn.
//
// Layout past NiShaderFactory's 0x24: +0x24 the shaders (NiTFixedStringMap<NiShader*>, vtable
// 0x9A162C on the base's 0x9A1978: +0x28 its 59 buckets' count, +0x2C the buckets, +0x30 its
// count), +0x34 the libraries (NiTFixedStringMap<NiShaderLibraryPtr>, vtable 0x9A19AC: 37
// buckets), +0x44 the library map's iteration position, +0x48 a lock (NiCriticalSection).
//
// Its slots (NiShaderFactory's order, as NiD3DShaderFactory's): 0 the deleting destructor, 2
// RetrieveShader, 3 FetchShader, 4/5 IsDefaultImplementation (by shader / by name), 6
// InsertShader, 7 RemoveShader, 8 RemoveAllShaders, 9 (a shader's slot 32 when it's an
// NiD3D10ShaderInterface: a guess), 10 InsertLibrary, 11 FindLibrary, 12 RemoveLibrary, 13
// RemoveAllLibraries, 14/15 GetFirstLibrary / GetNextLibrary, 16..24 the shader version helpers
// (NiD3DShaderFactory's own functions), 25 the renderer's name ("D3D10"), 26..28 the callbacks (0
// here), 29..33 the program factory's directories, 34 / 35 every shader's slot 28 / slot 29 (when
// its slot 3 says no) (names a guess: their renderer data let go / made again).
//
// Also the NiTFixedStringMap pieces in this range (RemoveAt, SetAt and SetValue of the NiPointer
// kind, the shader map's NewItem), which the game's other users of those maps share.

#include "gamebryo/NiD3D10C43.h"
#include "reversed.h"

namespace ni::c43 {
namespace {

using ni::sys::FixedStringRelease;
using ni::sys::Lock;
using ni::sys::NiCriticalSection;
using ni::sys::Unlock;

constexpr uintptr_t kFactoryVtable = 0x009A11D0;
constexpr uintptr_t kMapBaseVtable = 0x009A1978;       // NiTFixedStringMapBase
constexpr uintptr_t kShaderMapVtable = 0x009A162C;     // NiTFixedStringMap<NiShader*>
constexpr uintptr_t kLibraryMapVtable = 0x009A19AC;    // NiTFixedStringMap<NiShaderLibraryPtr>
constexpr uintptr_t kPointerListVtable = 0x0095127C;   // NiTPointerList<NiShaderLibraryPtr>
constexpr uintptr_t kOutputDebugStringA = 0x008FC0F4;
constexpr uint32_t kFactorySize = 0x68;

GAME_VAR(uint8_t*, ms_pkD3D10ShaderFactory, 0x020E91A4);
GAME_VAR(uint8_t*, NiShaderFactory_ms_pkShaderFactory, 0x020E23FC);
GAME_VAR(uint32_t, NiD3D10ShaderInterface_ClassBit, 0x020E9178);
GAME_VAR(char*, kNullFixedString, 0x00951028);

GAME_FN(void, __thiscall, ShaderFactory_ConstructBase, 0x00756920, void* self);
GAME_FN(void, __thiscall, ShaderFactory_DestructBase, 0x00756CC0, void* self);
GAME_FN(uint32_t, __cdecl, ShaderFactory_ReportError, 0x00756700, int result, int recoverable, const char* format,
        ...);
GAME_FN(const re::Ptr32<const char>*, __thiscall, Shader_GetName, 0x0077C280, void* shader);
GAME_FN(uint32_t, __thiscall, Shader_GetImplementation, 0x00853140, void* shader);
GAME_FN(const char*, __thiscall, ShaderLibrary_GetName, 0x00856850, void* library);
GAME_FN(uint8_t*, __cdecl, D3D10Renderer_GetRenderer, 0x008A9300);
GAME_FN(char*, __cdecl, GlobalStringTable_AddString, 0x00754760, const char* text);
GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, const re::Ptr32<char>* other);
// the shared one-field getters (the factory's three callbacks)
GAME_FN(void*, __thiscall, Get18, 0x008B40F0, void* self);
GAME_FN(void*, __thiscall, Get1C, 0x0077E8E0, void* self);
GAME_FN(void*, __thiscall, Get20, 0x007E2BE0, void* self);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(void, __thiscall, PointerList_AddTail, 0x007843D0, void* list, const re::Ptr32<void>* element);
GAME_FN(void, __thiscall, PointerList_RemoveAll, 0x0075DB60, void* list);
// the fixed string maps
GAME_FN(void, __thiscall, FixedMap_RemoveAll, 0x008C2230, void* map);
GAME_FN(void, __thiscall, ShaderMap_GetNext, 0x008C2290, void* map, re::Ptr32<void>* position, re::Ptr32<char>* key, re::Ptr32<void>* value);
GAME_FN(void, __thiscall, ShaderMap_SetAt, 0x008C27A0, void* map, const re::Ptr32<char>* key, void* shader);
GAME_FN(uint8_t, __thiscall, LibraryMap_GetAt, 0x00518180, void* map, const re::Ptr32<char>* key, re::Ptr32<void>* library);
GAME_FN(void, __thiscall, LibraryMap_GetNext, 0x008CB020, void* map, re::Ptr32<void>* position, re::Ptr32<char>* key, re::Ptr32<void>* library);
// the shader program factory (0x8C2100 its instance) and its directories
GAME_FN(uint8_t*, __cdecl, ProgramFactory_GetInstance, 0x008C2100);
GAME_FN(const char*, __thiscall, ProgramFactory_GetFirstDirectory, 0x008C2210, void* self, re::Ptr32<void>* position);
GAME_FN(const char*, __thiscall, ProgramFactory_GetNextDirectory, 0x008A2140, void* self, re::Ptr32<void>* position);
GAME_FN(void, __thiscall, ProgramFactory_AddDirectory, 0x008C2340, void* self, const char* directory);
GAME_FN(void, __thiscall, ProgramFactory_RemoveDirectory, 0x008C2920, void* self, const char* directory);
GAME_FN(void, __thiscall, ProgramFactory_RemoveAllDirectories, 0x008C2A00, void* self);
// this file's (called where modes apply)
GAME_FN(uint8_t, __thiscall, FixedMap_RemoveAt, 0x008B9860, void* map, const re::Ptr32<char>* key);
GAME_FN(void, __thiscall, FixedMap_SetValue, 0x008B98C0, void* map, void* item, const re::Ptr32<char>* key, void* value);
GAME_FN(void, __thiscall, FixedMap_SetAt, 0x008B9E50, void* map, const re::Ptr32<char>* key, void* value);
GAME_FN(void*, __thiscall, ShaderFactory_GetFirstLibrary, 0x008B9B00, void* self);
GAME_FN(void*, __thiscall, ShaderFactory_GetNextLibrary, 0x008B9BD0, void* self);
GAME_FN(uint8_t*, __thiscall, ShaderFactory_Construct, 0x008B9F30, void* self);
GAME_FN(void, __thiscall, ShaderFactory_RemoveAllLibraries, 0x008BA280, void* self);
GAME_FN(void, __thiscall, ShaderFactory_Destruct, 0x008BA510, void* self);

inline NiCriticalSection& FactoryLock(uint8_t* self) {
    return At<NiCriticalSection>(self, 0x48);
}
inline uint8_t* ShaderMap(uint8_t* self) {
    return self + 0x24;
}
inline uint8_t* LibraryMap(uint8_t* self) {
    return self + 0x34;
}

// a map's first item (the first non-empty bucket's head)
inline void* FirstPosition(const uint8_t* map) {
    for (uint32_t i = 0; i < At<uint32_t>(map, 4); i++) {
        if (At<re::Ptr32<void>*>(map, 8)[i] != nullptr) {
            return At<re::Ptr32<void>*>(map, 8)[i];
        }
    }
    return nullptr;
}

// a key made from a name (the empty handle for none)
inline char* MakeKey(const char* name) {
    return name == nullptr ? static_cast<char*>(kNullFixedString) : GlobalStringTable_AddString(name);
}

} // namespace

// 0x8B94D0: NiD3D10ShaderFactory::GetD3D10ShaderFactory
uint8_t* __cdecl ShaderFactory_GetD3D10ShaderFactory() {
    return ms_pkD3D10ShaderFactory;
}

// 0x8B94E0..0x8B9560 (thiscall, slots 29..33, `this` unused): the program factory's directories
// (when there is a program factory)
const char* __fastcall ShaderFactory_GetFirstProgramDirectory(void*, re::Edx, re::Ptr32<void>* position) {
    uint8_t* programFactory = ProgramFactory_GetInstance();
    if (programFactory == nullptr) {
        return nullptr;
    }
    return ProgramFactory_GetFirstDirectory(programFactory, position);
}
const char* __fastcall ShaderFactory_GetNextProgramDirectory(void*, re::Edx, re::Ptr32<void>* position) {
    uint8_t* programFactory = ProgramFactory_GetInstance();
    if (programFactory == nullptr) {
        return nullptr;
    }
    return ProgramFactory_GetNextDirectory(programFactory, position);
}
void __fastcall ShaderFactory_AddProgramDirectory(void*, re::Edx, const char* directory) {
    uint8_t* programFactory = ProgramFactory_GetInstance();
    if (programFactory != nullptr) {
        ProgramFactory_AddDirectory(programFactory, directory);
    }
}
void __fastcall ShaderFactory_RemoveProgramDirectory(void*, re::Edx, const char* directory) {
    uint8_t* programFactory = ProgramFactory_GetInstance();
    if (programFactory != nullptr) {
        ProgramFactory_RemoveDirectory(programFactory, directory);
    }
}
void __fastcall ShaderFactory_RemoveAllProgramDirectories(void*, re::Edx) {
    uint8_t* programFactory = ProgramFactory_GetInstance();
    if (programFactory != nullptr) {
        ProgramFactory_RemoveAllDirectories(programFactory);
    }
}

// 0x8B9580 (thiscall, slot 25): the renderer's name
const char* __fastcall ShaderFactory_GetRendererString(void*, re::Edx) {
    return reinterpret_cast<const char*>(0x00955DD0);   // "D3D10"
}

// 0x8B9590 (thiscall, slot 2): RetrieveShader: the shader by name and implementation; when it isn't
// kept, under the lock, each library asked to make it in turn (the first one made kept); reported
// when asked to and not found. No renderer: none.
void* __fastcall ShaderFactory_RetrieveShader(uint8_t* self, re::Edx, const char* name, uint32_t implementation,
                                             int reportNotFound) {
    uint8_t* renderer = D3D10Renderer_GetRenderer();
    if (renderer == nullptr) {
        Import<void>(kOutputDebugStringA, "Attempting to RetrieveShader without a valid renderer!\n");
        return nullptr;
    }
    if (name == nullptr || *name == '\0') {
        ShaderFactory_ReportError(1, 0, "Attempting to RetrieveShader w/out a valid name!\n");
        return nullptr;
    }
    void* shader = Virtual<void*>(self, 3, name, implementation);
    if (shader == nullptr) {
        Lock(FactoryLock(self));
        void* library = Virtual<void*>(self, 14);
        shader = Virtual<void*>(self, 3, name, implementation);
        if (shader == nullptr) {
            while (library != nullptr) {
                shader = Virtual<void*>(library, 2, renderer, name, implementation);
                if (shader != nullptr) {
                    Virtual<void>(self, 6, shader, implementation);
                    break;
                }
                library = Virtual<void*>(self, 15);
            }
        }
        Unlock(FactoryLock(self));
    }
    if (uint8_t(reportNotFound) != 0 && shader == nullptr) {
        ShaderFactory_ReportError(1, 0, "Failed to find shader %s, Implementation %d\n", name, implementation);
    }
    return shader;
}

// 0x8B96C0 (thiscall, slot 5): IsDefaultImplementation by name (the shader kept under that
// implementation, or the default one (-1) having it; then whether a library says so)
uint8_t __fastcall ShaderFactory_IsDefaultImplementationByName(uint8_t* self, re::Edx, const char* name,
                                                               uint32_t implementation) {
    if (name == nullptr || *name == '\0') {
        return 0;
    }
    if (Virtual<void*>(self, 3, name, implementation) == nullptr) {
        void* shader = Virtual<void*>(self, 3, name, 0xFFFFFFFFu);
        if (shader == nullptr || Shader_GetImplementation(shader) != implementation) {
            return 0;
        }
    }
    Lock(FactoryLock(self));
    uint8_t result = 0;
    for (void* library = Virtual<void*>(self, 14); library != nullptr; library = Virtual<void*>(self, 15)) {
        if (Virtual<uint8_t>(library, 4, name, implementation) != 0) {
            result = 1;
            break;
        }
    }
    Unlock(FactoryLock(self));
    return result;
}

// 0x8B9790 (thiscall, slot 4): IsDefaultImplementation by shader
uint8_t __fastcall ShaderFactory_IsDefaultImplementation(uint8_t* self, re::Edx, void* shader) {
    if (shader == nullptr) {
        return 0;
    }
    const char* name = *Shader_GetName(shader);
    uint32_t implementation = Shader_GetImplementation(shader);
    void* kept = Virtual<void*>(self, 3, name, implementation);
    if (kept == nullptr || kept != shader) {
        kept = Virtual<void*>(self, 3, *Shader_GetName(shader), 0xFFFFFFFFu);
        if (kept == nullptr || kept != shader) {
            return 0;
        }
    }
    Lock(FactoryLock(self));
    uint8_t result = 0;
    for (void* library = Virtual<void*>(self, 14); library != nullptr; library = Virtual<void*>(self, 15)) {
        if (Virtual<uint8_t>(library, 3, shader) != 0) {
            result = 1;
            break;
        }
    }
    Unlock(FactoryLock(self));
    return result;
}

// 0x8B9860 (thiscall): NiTFixedStringMap::RemoveAt(key) (the item unlinked and deleted by the map's
// slot 2; true when there was one)
uint8_t __fastcall FixedMap_RemoveAtImpl(uint8_t* map, re::Edx, const re::Ptr32<char>* key) {
    char* k = *key;
    re::Ptr32<void>* link = &At<re::Ptr32<void>*>(map, 8)[uintptr_t(k) % At<uint32_t>(map, 4)];
    for (re::Ptr32<void>* item = static_cast<re::Ptr32<void>*>(*link); item != nullptr; item = static_cast<re::Ptr32<void>*>(*item)) {
        if (k == static_cast<char*>(item[1])) {
            *link = item[0];
            Virtual<void>(map, 2, item);
            At<uint32_t>(map, 0xC) -= 1;
            return 1;
        }
        link = item;
    }
    return 0;
}

// 0x8B98C0 (thiscall, `this` unused): NiTFixedStringMap<NiPointer<...>>::SetValue(item, key, value):
// the key assigned, the value held (the copy passed in let go)
void __fastcall FixedMap_SetValueImpl(void*, re::Edx, uint8_t* item, const re::Ptr32<char>* key, void* value) {
    FixedString_Assign(reinterpret_cast<re::Ptr32<char>*>(item + 4), key);
    re::Ptr32<void>& slot = At<void*>(item, 8);
    if (slot != value) {
        ReleaseRef(slot);
        slot = value;
        if (value == nullptr) {
            return;
        }
        Increment(&At<uint32_t>(value, 4));
    }
    ReleaseRef(value);
}

// 0x8B9930 (thiscall, the shader map's slot 1): a new item (key and value zero; unchecked)
void* __fastcall ShaderMap_NewItem(void*, re::Edx) {
    uint8_t* item = static_cast<uint8_t*>(NiNew(0xC));
    At<void*>(item, 8) = nullptr;
    At<void*>(item, 4) = nullptr;
    return item;
}

// 0x8B9950 (cdecl): NiD3D10ShaderFactory::Shutdown (the instance deleted; NiShaderFactory's pointer
// cleared when it was this one)
void __cdecl ShaderFactory_Shutdown() {
    uint8_t* factory = ms_pkD3D10ShaderFactory;
    if (factory != nullptr) {
        bool current = NiShaderFactory_ms_pkShaderFactory == factory;
        Virtual<void>(factory, 0, 1u);
        ms_pkD3D10ShaderFactory = nullptr;
        if (current) {
            NiShaderFactory_ms_pkShaderFactory = nullptr;
        }
    }
}

// 0x8B9990 (thiscall, slot 8): RemoveAllShaders
void __fastcall ShaderFactory_RemoveAllShaders(uint8_t* self, re::Edx) {
    FixedMap_RemoveAll(ShaderMap(self));
}

// 0x8B99A0 (thiscall, slot 11): FindLibrary (the reference the lookup took let go)
void* __fastcall ShaderFactory_FindLibrary(uint8_t* self, re::Edx, const char* name) {
    re::Ptr32<void> library = nullptr;
    re::Ptr32<char> key = MakeKey(name);
    LibraryMap_GetAt(LibraryMap(self), &key, &library);
    FixedStringRelease(&key);
    ReleaseRef(library);
    return library;
}

// 0x8B9A30 (thiscall, slot 12): RemoveLibrary (only when the map and this lookup hold the only
// references); the iteration reset
void __fastcall ShaderFactory_RemoveLibrary(uint8_t* self, re::Edx, const char* name) {
    uint8_t* library = nullptr;
    re::Ptr32<char> key = MakeKey(name);
    if (LibraryMap_GetAt(LibraryMap(self), &key, reinterpret_cast<re::Ptr32<void>*>(&library)) != 0 &&
        At<uint32_t>(library, 4) == 2) {
        FixedMap_RemoveAt(LibraryMap(self), &key);
        ReleaseRef(library);
        library = nullptr;
    }
    At<void*>(self, 0x44) = nullptr;
    FixedStringRelease(&key);
    ReleaseRef(library);
}

// 0x8B9B00 (thiscall, slot 14): GetFirstLibrary (the reference the iteration took let go)
void* __fastcall ShaderFactory_GetFirstLibraryImpl(uint8_t* self, re::Edx) {
    void* position = FirstPosition(LibraryMap(self));
    At<void*>(self, 0x44) = position;
    if (position == nullptr) {
        return nullptr;
    }
    re::Ptr32<char> key = kNullFixedString;
    re::Ptr32<void> library = nullptr;
    LibraryMap_GetNext(LibraryMap(self), &At<void*>(self, 0x44), &key, &library);
    ReleaseRef(library);
    FixedStringRelease(&key);
    return library;
}

// 0x8B9BD0 (thiscall, slot 15): GetNextLibrary
void* __fastcall ShaderFactory_GetNextLibraryImpl(uint8_t* self, re::Edx) {
    if (At<void*>(self, 0x44) == nullptr) {
        return nullptr;
    }
    re::Ptr32<char> key = kNullFixedString;
    re::Ptr32<void> library = nullptr;
    LibraryMap_GetNext(LibraryMap(self), &At<void*>(self, 0x44), &key, &library);
    ReleaseRef(library);
    FixedStringRelease(&key);
    return library;
}

// 0x8B9C60 (thiscall, slot 34): every kept shader's slot 28 (a guess: its renderer data let go)
void __fastcall ShaderFactory_PurgeAllShaderRendererData(uint8_t* self, re::Edx) {
    re::Ptr32<void> position = FirstPosition(ShaderMap(self));
    while (position != nullptr) {
        re::Ptr32<char> key = kNullFixedString;
        re::Ptr32<void> shader = nullptr;
        ShaderMap_GetNext(ShaderMap(self), &position, &key, &shader);
        if (shader != nullptr) {
            Virtual<void>(shader, 28);
        }
        FixedStringRelease(&key);
    }
}

// 0x8B9D20 (thiscall, slot 35): with a renderer, every kept shader whose slot 3 says no given its
// slot 29 (a guess: its renderer data made again)
void __fastcall ShaderFactory_RecreateAllShaderRendererData(uint8_t* self, re::Edx) {
    if (D3D10Renderer_GetRenderer() == nullptr) {
        return;
    }
    re::Ptr32<void> position = FirstPosition(ShaderMap(self));
    while (position != nullptr) {
        re::Ptr32<char> key = kNullFixedString;
        re::Ptr32<void> shader = nullptr;
        ShaderMap_GetNext(ShaderMap(self), &position, &key, &shader);
        if (shader != nullptr && Virtual<uint8_t>(shader, 3) == 0) {
            Virtual<void>(shader, 29);
        }
        FixedStringRelease(&key);
    }
}

// 0x8B9E00 (thiscall, slot 9, `this` unused): a shader's slot 32 when it's an
// NiD3D10ShaderInterface, else false
uint8_t __fastcall ShaderFactory_Slot9(void*, re::Edx, void* shader) {
    if (shader != nullptr && ni::dx9::HasClassBit(shader, NiD3D10ShaderInterface_ClassBit)) {
        return Virtual<uint8_t>(shader, 32);
    }
    return 0;
}

// 0x8B9E50 (thiscall): NiTFixedStringMap<NiPointer<...>>::SetAt(key, value) (value by value: a new
// item (slot 1) at its bucket's head, or the existing one's value set; the copy let go)
void __fastcall FixedMap_SetAtImpl(uint8_t* map, re::Edx, const re::Ptr32<char>* key, void* value) {
    uint32_t bucket = uint32_t(uintptr_t(*key) % At<uint32_t>(map, 4));
    re::Ptr32<void>* item = static_cast<re::Ptr32<void>*>(At<re::Ptr32<void>*>(map, 8)[bucket]);
    while (item != nullptr && *key != static_cast<char*>(item[1])) {
        item = static_cast<re::Ptr32<void>*>(item[0]);
    }
    if (item == nullptr) {
        item = Virtual<re::Ptr32<void>*>(map, 1);
        AddRefRef(value);
        FixedMap_SetValue(map, item, key, value);
        item[0] = At<re::Ptr32<void>*>(map, 8)[bucket];
        At<re::Ptr32<void>*>(map, 8)[bucket] = item;
        At<uint32_t>(map, 0xC) += 1;
    } else {
        AddRefRef(value);
        FixedMap_SetValue(map, item, key, value);
    }
    ReleaseRef(value);
}

// 0x8B9F30 (thiscall): NiD3D10ShaderFactory::NiD3D10ShaderFactory (the shader map (59 buckets), the
// library map (37), the lock)
uint8_t* __fastcall ShaderFactory_ConstructImpl(uint8_t* self, re::Edx) {
    ShaderFactory_ConstructBase(self);
    At<uint32_t>(self, 0) = kFactoryVtable;
    At<uint32_t>(self, 0x24) = kMapBaseVtable;
    At<uint32_t>(self, 0x28) = 0x3B;
    At<uint32_t>(self, 0x30) = 0;
    void* buckets = NiMalloc(0xEC);
    At<void*>(self, 0x2C) = buckets;
    crt::Memset(buckets, 0, At<uint32_t>(self, 0x28) * 4);
    At<uint32_t>(self, 0x24) = kShaderMapVtable;
    At<uint32_t>(self, 0x34) = kMapBaseVtable;
    At<uint32_t>(self, 0x38) = 0x25;
    At<uint32_t>(self, 0x40) = 0;
    buckets = NiMalloc(0x94);
    At<void*>(self, 0x3C) = buckets;
    crt::Memset(buckets, 0, At<uint32_t>(self, 0x38) * 4);
    At<uint32_t>(self, 0x34) = kLibraryMapVtable;
    At<void*>(self, 0x44) = nullptr;
    FactoryLock(self).owner = 0;
    FactoryLock(self).lockCount = 0;
    Import<void>(ni::sys::kInitializeCriticalSection, &FactoryLock(self).section);
    return self;
}

// 0x8B9FD0 (thiscall, slot 6): InsertShader (kept as name + implementation)
void __fastcall ShaderFactory_InsertShader(uint8_t* self, re::Edx, void* shader, uint32_t implementation) {
    char buffer[0x104];
    if (shader == nullptr) {
        return;
    }
    NiSprintf(buffer, sizeof(buffer), "%s%d", *Shader_GetName(shader), implementation);
    re::Ptr32<char> key = GlobalStringTable_AddString(buffer);
    ShaderMap_SetAt(ShaderMap(self), &key, shader);
    FixedStringRelease(&key);
}

// 0x8BA090 (thiscall, slot 3): FetchShader (the kept one, or null)
void* __fastcall ShaderFactory_FetchShader(uint8_t* self, re::Edx, const char* name, uint32_t implementation) {
    char buffer[0x104];
    NiSprintf(buffer, sizeof(buffer), "%s%d", name, implementation);
    re::Ptr32<char> key = GlobalStringTable_AddString(buffer);
    void* shader = nullptr;
    uint8_t* map = ShaderMap(self);
    for (re::Ptr32<void>* item = At<re::Ptr32<re::Ptr32<void>>*>(map, 8)[uintptr_t(key) % At<uint32_t>(map, 4)]; item != nullptr;
         item = static_cast<re::Ptr32<void>*>(item[0])) {
        if (key == static_cast<char*>(item[1])) {
            shader = item[2];
            break;
        }
    }
    FixedStringRelease(&key);
    return shader;
}

// 0x8BA150 (thiscall, slot 7): RemoveShader
void __fastcall ShaderFactory_RemoveShader(uint8_t* self, re::Edx, const char* name, uint32_t implementation) {
    char buffer[0x104];
    NiSprintf(buffer, sizeof(buffer), "%s%d", name, implementation);
    re::Ptr32<char> key = GlobalStringTable_AddString(buffer);
    FixedMap_RemoveAt(ShaderMap(self), &key);
    FixedStringRelease(&key);
}

// 0x8BA200 (thiscall, slot 10): InsertLibrary (kept by its name; the map holds a reference)
void __fastcall ShaderFactory_InsertLibrary(uint8_t* self, re::Edx, uint8_t* library) {
    if (library == nullptr) {
        return;
    }
    re::Ptr32<char> key = MakeKey(ShaderLibrary_GetName(library));
    Increment(library + 4);   // (the NiPointer passed by value)
    FixedMap_SetAt(LibraryMap(self), &key, library);
    FixedStringRelease(&key);
}

// 0x8BA280 (thiscall, slot 13): RemoveAllLibraries (each one's item removed while iterating,
// the map emptied, the iteration reset)
void __fastcall ShaderFactory_RemoveAllLibrariesImpl(uint8_t* self, re::Edx) {
    void* library = ShaderFactory_GetFirstLibrary(self);
    while (library != nullptr) {
        re::Ptr32<char> key = MakeKey(ShaderLibrary_GetName(library));
        uint8_t* map = LibraryMap(self);
        re::Ptr32<void>* link = &At<re::Ptr32<void>*>(map, 8)[uintptr_t(key) % At<uint32_t>(map, 4)];
        for (re::Ptr32<void>* item = static_cast<re::Ptr32<void>*>(*link); item != nullptr; item = static_cast<re::Ptr32<void>*>(*item)) {
            if (key == static_cast<char*>(item[1])) {
                *link = item[0];
                Virtual<void>(map, 2, item);
                At<uint32_t>(map, 0xC) -= 1;
                break;
            }
            link = item;
        }
        library = ShaderFactory_GetNextLibrary(self);
        FixedStringRelease(&key);
    }
    FixedMap_RemoveAll(LibraryMap(self));
    At<void*>(self, 0x44) = nullptr;
}

// 0x8BA360 (cdecl): NiD3D10ShaderFactory::TakeOver: this factory made NiShaderFactory's (the
// other's callbacks taken where its slots 26..28 differ from them, its libraries moved here).
// False when it already is.
uint8_t __cdecl ShaderFactory_TakeOver() {
    if (NiShaderFactory_ms_pkShaderFactory == ms_pkD3D10ShaderFactory) {
        return 0;
    }
    NiTPointerList<void*> libraries;
    libraries.count = 0;
    libraries.head = nullptr;
    libraries.tail = nullptr;
    libraries.vtable = reinterpret_cast<void*>(kPointerListVtable);
    uint8_t* old = NiShaderFactory_ms_pkShaderFactory;
    if (old != nullptr) {
        void* callback = Virtual<void*>(old, 26);
        if (Get18(NiShaderFactory_ms_pkShaderFactory) != callback) {
            At<void*>(ms_pkD3D10ShaderFactory, 0x18) = Get18(NiShaderFactory_ms_pkShaderFactory);
        }
        callback = Virtual<void*>(NiShaderFactory_ms_pkShaderFactory, 27);
        if (Get1C(NiShaderFactory_ms_pkShaderFactory) != callback) {
            At<void*>(ms_pkD3D10ShaderFactory, 0x1C) = Get1C(NiShaderFactory_ms_pkShaderFactory);
        }
        callback = Virtual<void*>(NiShaderFactory_ms_pkShaderFactory, 28);
        if (Get20(NiShaderFactory_ms_pkShaderFactory) != callback) {
            At<void*>(ms_pkD3D10ShaderFactory, 0x20) = Get20(NiShaderFactory_ms_pkShaderFactory);
        }
        void* library = Virtual<void*>(NiShaderFactory_ms_pkShaderFactory, 14);
        while (library != nullptr) {
            re::Ptr32<void> held = library;
            Increment(static_cast<uint8_t*>(held) + 4);
            PointerList_AddTail(&libraries, &held);
            library = Virtual<void*>(NiShaderFactory_ms_pkShaderFactory, 15);
            ReleaseRef(held);
        }
        Virtual<void>(NiShaderFactory_ms_pkShaderFactory, 13);
    }
    NiShaderFactory_ms_pkShaderFactory = ms_pkD3D10ShaderFactory;
    for (NiTListNode<void*>* node = libraries.head; node != nullptr;) {
        void* library = node->element;
        node = node->next;
        AddRefRef(library);
        Virtual<void>(ms_pkD3D10ShaderFactory, 10, library);
        ReleaseRef(library);
    }
    libraries.vtable = reinterpret_cast<void*>(kPointerListVtable);
    PointerList_RemoveAll(&libraries);
    return 1;
}

// 0x8BA510 (thiscall): NiD3D10ShaderFactory::~NiD3D10ShaderFactory (the shaders and libraries let
// go, the lock deleted, both maps destroyed and their buckets freed)
void __fastcall ShaderFactory_DestructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kFactoryVtable;
    FixedMap_RemoveAll(ShaderMap(self));
    ShaderFactory_RemoveAllLibraries(self);
    Import<void>(ni::sys::kDeleteCriticalSection, &FactoryLock(self).section);
    At<uint32_t>(self, 0x34) = kLibraryMapVtable;
    FixedMap_RemoveAll(LibraryMap(self));
    At<uint32_t>(self, 0x34) = kMapBaseVtable;
    FixedMap_RemoveAll(LibraryMap(self));
    NiFree(At<void*>(self, 0x3C));
    At<uint32_t>(self, 0x24) = kShaderMapVtable;
    FixedMap_RemoveAll(ShaderMap(self));
    At<uint32_t>(self, 0x24) = kMapBaseVtable;
    FixedMap_RemoveAll(ShaderMap(self));
    NiFree(At<void*>(self, 0x2C));
    ShaderFactory_DestructBase(self);
}

// 0x8BA590 (cdecl): NiD3D10ShaderFactory::Create (the instance, made NiShaderFactory's when there's
// none)
void __cdecl ShaderFactory_Create() {
    if (ms_pkD3D10ShaderFactory == nullptr) {
        void* memory = NiNew(kFactorySize);
        ms_pkD3D10ShaderFactory = memory != nullptr ? ShaderFactory_Construct(memory) : nullptr;
    }
    if (NiShaderFactory_ms_pkShaderFactory == nullptr) {
        NiShaderFactory_ms_pkShaderFactory = ms_pkD3D10ShaderFactory;
    }
    if (ms_pkD3D10ShaderFactory == nullptr) {
        ni::dx9::NiOutputDebugString("Failed to create shader factory!");
    }
}

// 0x8BA5E0 (thiscall, slot 0): the deleting destructor
uint8_t* __fastcall ShaderFactory_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    ShaderFactory_Destruct(self);
    if (flags & 1) {
        NiDelete(self, kFactorySize);
    }
    return self;
}

#define GB43F_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

GB43F_REVERSED("NiD3D10ShaderFactory::GetD3D10ShaderFactory", 0x008B94D0, ShaderFactory_GetD3D10ShaderFactory);
GB43F_REVERSED("NiD3D10ShaderFactory::GetFirstProgramDirectory", 0x008B94E0, ShaderFactory_GetFirstProgramDirectory);
GB43F_REVERSED("NiD3D10ShaderFactory::GetNextProgramDirectory", 0x008B9500, ShaderFactory_GetNextProgramDirectory);
GB43F_REVERSED("NiD3D10ShaderFactory::AddProgramDirectory", 0x008B9520, ShaderFactory_AddProgramDirectory);
GB43F_REVERSED("NiD3D10ShaderFactory::RemoveProgramDirectory", 0x008B9540, ShaderFactory_RemoveProgramDirectory);
GB43F_REVERSED("NiD3D10ShaderFactory::RemoveAllProgramDirectories", 0x008B9560,
               ShaderFactory_RemoveAllProgramDirectories);
GB43F_REVERSED("NiD3D10ShaderFactory::GetRendererString", 0x008B9580, ShaderFactory_GetRendererString);
GB43F_REVERSED("NiD3D10ShaderFactory::RetrieveShader", 0x008B9590, ShaderFactory_RetrieveShader);
GB43F_REVERSED("NiD3D10ShaderFactory::IsDefaultImplementationByName", 0x008B96C0,
               ShaderFactory_IsDefaultImplementationByName);
GB43F_REVERSED("NiD3D10ShaderFactory::IsDefaultImplementation", 0x008B9790, ShaderFactory_IsDefaultImplementation);
GB43F_REVERSED("NiTFixedStringMap::RemoveAt", 0x008B9860, FixedMap_RemoveAtImpl);
GB43F_REVERSED("NiTFixedStringMap<NiPointer>::SetValue", 0x008B98C0, FixedMap_SetValueImpl);
GB43F_REVERSED("NiTFixedStringMap<NiShader*>::NewItem", 0x008B9930, ShaderMap_NewItem);
GB43F_REVERSED("NiD3D10ShaderFactory::Shutdown", 0x008B9950, ShaderFactory_Shutdown);
GB43F_REVERSED("NiD3D10ShaderFactory::RemoveAllShaders", 0x008B9990, ShaderFactory_RemoveAllShaders);
GB43F_REVERSED("NiD3D10ShaderFactory::FindLibrary", 0x008B99A0, ShaderFactory_FindLibrary);
GB43F_REVERSED("NiD3D10ShaderFactory::RemoveLibrary", 0x008B9A30, ShaderFactory_RemoveLibrary);
GB43F_REVERSED("NiD3D10ShaderFactory::GetFirstLibrary", 0x008B9B00, ShaderFactory_GetFirstLibraryImpl);
GB43F_REVERSED("NiD3D10ShaderFactory::GetNextLibrary", 0x008B9BD0, ShaderFactory_GetNextLibraryImpl);
GB43F_REVERSED("NiD3D10ShaderFactory::PurgeAllShaderRendererData", 0x008B9C60,
               ShaderFactory_PurgeAllShaderRendererData);
GB43F_REVERSED("NiD3D10ShaderFactory::RecreateAllShaderRendererData", 0x008B9D20,
               ShaderFactory_RecreateAllShaderRendererData);
GB43F_REVERSED("NiD3D10ShaderFactory::Slot9", 0x008B9E00, ShaderFactory_Slot9);
GB43F_REVERSED("NiTFixedStringMap<NiPointer>::SetAt", 0x008B9E50, FixedMap_SetAtImpl);
GB43F_REVERSED("NiD3D10ShaderFactory::NiD3D10ShaderFactory", 0x008B9F30, ShaderFactory_ConstructImpl);
GB43F_REVERSED("NiD3D10ShaderFactory::InsertShader", 0x008B9FD0, ShaderFactory_InsertShader);
GB43F_REVERSED("NiD3D10ShaderFactory::FetchShader", 0x008BA090, ShaderFactory_FetchShader);
GB43F_REVERSED("NiD3D10ShaderFactory::RemoveShader", 0x008BA150, ShaderFactory_RemoveShader);
GB43F_REVERSED("NiD3D10ShaderFactory::InsertLibrary", 0x008BA200, ShaderFactory_InsertLibrary);
GB43F_REVERSED("NiD3D10ShaderFactory::RemoveAllLibraries", 0x008BA280, ShaderFactory_RemoveAllLibrariesImpl);
GB43F_REVERSED("NiD3D10ShaderFactory::TakeOver", 0x008BA360, ShaderFactory_TakeOver);
GB43F_REVERSED("NiD3D10ShaderFactory::~NiD3D10ShaderFactory", 0x008BA510, ShaderFactory_DestructImpl);
GB43F_REVERSED("NiD3D10ShaderFactory::Create", 0x008BA590, ShaderFactory_Create);
GB43F_REVERSED("NiD3D10ShaderFactory::DeletingDestructor", 0x008BA5E0, ShaderFactory_DeletingDestructor);

} // namespace ni::c43
