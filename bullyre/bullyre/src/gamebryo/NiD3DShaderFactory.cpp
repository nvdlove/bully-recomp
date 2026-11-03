// NiD3DShaderFactory (vtable 0x99DA80, 0x54 bytes; the one instance at 0x20E73A4, made by
// Create and made NiShaderFactory's own by TakeOver): the DX9 renderer's NiShaderFactory. Its
// shaders are kept by name and implementation ("%s%d") in a string map, its shader libraries in
// another, and a shader asked for that isn't there is made by the libraries in turn.
//
// Layout past NiShaderFactory's 0x24: +0x24 the renderer, +0x28 the shaders
// (NiTStringPointerMap<NiShader*>, vtable 0x9508CC, 59 buckets, copying its keys), +0x2C the
// library map's iteration position, +0x30 the libraries (vtable 0x951784, 37 buckets: its values
// held references), +0x34 a lock (NiCriticalSection).
//
// Its slots (NiShaderFactory's order, 2.x): 0 the deleting destructor, 2 RetrieveShader, 3
// FetchShader, 4/5 IsDefaultImplementation (by shader / by name), 6 InsertShader, 7 RemoveShader,
// 8 RemoveAllShaders, 9 (a shader's slot 36 when it's an NiD3DShaderInterface: a guess), 10
// InsertLibrary, 11 FindLibrary, 12 RemoveLibrary, 13 RemoveAllLibraries, 14/15 GetFirstLibrary /
// GetNextLibrary, 16..24 the shader version helpers (vertex, geometry, pixel: major, minor,
// create), 26..28 the callbacks (0 here), 29..33 the program factory's directories, 34
// SetRenderer, 35 the renderer's 0x881610, 36 the shaders' renderer data made again.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::c41 {
using namespace ni::dx9;
namespace {

constexpr uintptr_t kFactoryVtable = 0x0099DA80;
constexpr uintptr_t kMapBaseVtable = 0x00950760;         // NiTMapBase
constexpr uintptr_t kShaderMapVtable = 0x009508CC;       // NiTStringPointerMap<NiShader*>
constexpr uintptr_t kLibraryMapBaseVtable = 0x0099ACD4;
constexpr uintptr_t kLibraryMapVtable = 0x00951784;      // NiTStringMap<NiShaderLibraryPtr>
constexpr uintptr_t kPointerListVtable = 0x0095127C;     // NiTPointerList<NiShaderLibraryPtr>
constexpr uintptr_t kAllocatorVtable = 0x0099C748;
constexpr uint32_t kFactorySize = 0x54;
constexpr uintptr_t kOutputDebugStringA = 0x008FC0F4;

GAME_VAR(uint8_t*, ms_pkD3DShaderFactory, 0x020E73A4);
GAME_VAR(uint8_t*, NiShaderFactory_ms_pkShaderFactory, 0x020E23FC);
GAME_VAR(uint32_t, NiD3DShaderInterface_ClassBit, 0x020E6E14);

GAME_FN(void, __thiscall, ShaderFactory_ConstructBase, 0x00756920, void* self);
GAME_FN(void, __thiscall, ShaderFactory_DestructBase, 0x00756CC0, void* self);
GAME_FN(uint32_t, __cdecl, ShaderFactory_ReportError, 0x00756700, int result, int recoverable, const char* format,
        ...);
GAME_FN(const re::Ptr32<const char>*, __thiscall, Shader_GetName, 0x0077C280, void* shader);
GAME_FN(uint32_t, __thiscall, Shader_GetImplementation, 0x00853140, void* shader);
GAME_FN(const char*, __thiscall, ShaderLibrary_GetName, 0x00856850, void* library);
GAME_FN(void, __thiscall, D3DShaderInterface_SetD3DRenderer, 0x00886FE0, void* shader, void* renderer);
GAME_FN(void, __thiscall, Renderer_RegisterD3DShader, 0x0087EE30, void* renderer, void* shader);
GAME_FN(void, __thiscall, Renderer_881610, 0x00881610, void* renderer);
// the shared one-field getters (the factory's three callbacks)
GAME_FN(void*, __thiscall, Get18, 0x008B40F0, void* self);
GAME_FN(void*, __thiscall, Get1C, 0x0077E8E0, void* self);
GAME_FN(void*, __thiscall, Get20, 0x007E2BE0, void* self);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(void, __thiscall, Map_RemoveAll, 0x005AC520, void* map);
GAME_FN(void, __thiscall, Map_Destruct, 0x0075A4E0, void* map);
GAME_FN(void*, __thiscall, Map_GetFirstPos, 0x008CA4E0, void* map);
GAME_FN(void, __thiscall, Map_GetNext, 0x0083D890, void* map, re::Ptr32<void>* position, re::Ptr32<const char>* key, re::Ptr32<void>* value);
GAME_FN(void, __thiscall, StringMap_SetAt, 0x00747F00, void* map, const char* key, void* value);
GAME_FN(uint8_t, __thiscall, StringMap_GetAt, 0x00800470, void* map, const char* key, re::Ptr32<void>* value);
GAME_FN(uint8_t, __thiscall, StringMap_RemoveAt, 0x008A1F90, void* map, const char* key);
// the library map: its values NiPointers (SetAt's by value, released by it)
GAME_FN(void, __thiscall, LibraryMap_SetAt, 0x00747380, void* map, const char* key, void* library);
GAME_FN(uint8_t, __thiscall, LibraryMap_GetAt, 0x00845270, void* map, const char* key, re::Ptr32<void>* library);
GAME_FN(void, __thiscall, LibraryMap_GetNext, 0x007474A0, void* map, re::Ptr32<void>* position, re::Ptr32<const char>* key,
        re::Ptr32<void>* library);
GAME_FN(void, __thiscall, PointerList_AddTail, 0x007843D0, void* list, const re::Ptr32<void>* element);
GAME_FN(void, __thiscall, PointerList_RemoveAll, 0x0075DB60, void* list);
// the shader program factory (0x8A1EE0 its instance) and its directories
GAME_FN(uint8_t*, __cdecl, ProgramFactory_GetInstance, 0x008A1EE0);
GAME_FN(const char*, __thiscall, ProgramFactory_GetFirstDirectory, 0x008A2120, void* self, re::Ptr32<void>* position);
GAME_FN(const char*, __thiscall, ProgramFactory_GetNextDirectory, 0x008A2140, void* self, re::Ptr32<void>* position);   // (`this` unused)
GAME_FN(void, __thiscall, ProgramFactory_AddDirectory, 0x008A2540, void* self, const char* directory);
GAME_FN(void, __thiscall, ProgramFactory_RemoveDirectory, 0x008A2680, void* self, const char* directory);
GAME_FN(void, __thiscall, ProgramFactory_RemoveAllDirectories, 0x008A2770, void* self);

// this file's (called where modes apply)
GAME_FN(uint8_t*, __thiscall, ShaderFactory_Construct, 0x0089D490, void* self);
GAME_FN(void, __thiscall, ShaderFactory_Destruct, 0x0089D570, void* self);
GAME_FN(void*, __thiscall, ShaderFactory_GetFirstLibrary, 0x0089CF30, void* self);
GAME_FN(void, __thiscall, ShaderFactory_RemoveAllLibraries, 0x0089D230, void* self);
GAME_FN(uint8_t, __cdecl, ShaderFactory_TakeOver, 0x0089D2C0);

inline NiCriticalSection& FactoryLock(uint8_t* self) {
    return At<NiCriticalSection>(self, 0x34);
}

inline void ReleaseObject(void* object) {
    if (object != nullptr && Decrement(static_cast<uint8_t*>(object) + 4) == 0) {
        Virtual<void>(object, 1);
    }
}

} // namespace

// 0x89C930 (thiscall, slot 35): the renderer's 0x881610 (when there is one)
void __fastcall ShaderFactory_Slot35(uint8_t* self, re::Edx) {
    if (At<void*>(self, 0x24) != nullptr) {
        Renderer_881610(At<void*>(self, 0x24));
    }
}

// 0x89C940 (thiscall, slots 16/19/22, `this` unused): GetMajor*ShaderVersion (D3D's version's byte 1)
uint32_t __fastcall ShaderFactory_GetMajorShaderVersion(void*, re::Edx, uint32_t version) {
    return uint8_t(version >> 8);
}

// 0x89C950 (thiscall, slot 18): CreateVertexShaderVersion (D3DVS_VERSION)
uint32_t __fastcall ShaderFactory_CreateVertexShaderVersion(void*, re::Edx, uint32_t major, uint32_t minor) {
    return (major | 0xFFFFFE00) << 8 | minor;
}

// 0x89C970 (thiscall, slot 21): CreateGeometryShaderVersion (0xFFFD....)
uint32_t __fastcall ShaderFactory_CreateGeometryShaderVersion(void*, re::Edx, uint32_t major, uint32_t minor) {
    return (major | 0xFFFFFD00) << 8 | minor;
}

// 0x89C990 (thiscall, slots 17/20/23): GetMinor*ShaderVersion (the low byte)
uint32_t __fastcall ShaderFactory_GetMinorShaderVersion(void*, re::Edx, uint32_t version) {
    return uint8_t(version);
}

// 0x89C9A0 (thiscall, slot 24): CreatePixelShaderVersion (D3DPS_VERSION)
uint32_t __fastcall ShaderFactory_CreatePixelShaderVersion(void*, re::Edx, uint32_t major, uint32_t minor) {
    return (major | 0xFFFFFF00) << 8 | minor;
}

// 0x89C9C0: NiD3DShaderFactory::GetD3DShaderFactory
uint8_t* __cdecl ShaderFactory_GetD3DShaderFactory() {
    return ms_pkD3DShaderFactory;
}

// 0x89C9D0..0x89CA50 (thiscall, slots 29..33, `this` unused): the program factory's
// directories (when there is a program factory)
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

// 0x89CA70: the instance deleted (slot 0) and forgotten. (Name a guess: Shutdown.)
void __cdecl ShaderFactory_Shutdown() {
    if (ms_pkD3DShaderFactory != nullptr) {
        Virtual<void>(ms_pkD3DShaderFactory, 0, 1u);
        ms_pkD3DShaderFactory = nullptr;
    }
}

// 0x89CA90 (thiscall, slot 2): RetrieveShader: the shader by name and implementation; when it
// isn't kept, under the lock, each library asked to make it in turn (the first one made kept, the
// renderer told, given the renderer); reported when asked to and not found
void* __fastcall ShaderFactory_RetrieveShader(uint8_t* self, re::Edx, const char* name, uint32_t implementation,
                                             int reportNotFound) {
    if (At<void*>(self, 0x24) == nullptr) {
        Import<void>(kOutputDebugStringA, "Attempting to RetrieveShader w/out a valid renderer!\n");
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
                shader = Virtual<void*>(library, 2, At<void*>(self, 0x24), name, implementation);
                if (shader != nullptr) {
                    Virtual<void>(self, 6, shader, implementation);
                    Renderer_RegisterD3DShader(At<void*>(self, 0x24), shader);
                    D3DShaderInterface_SetD3DRenderer(shader, At<void*>(self, 0x24));
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

// 0x89CBD0 (thiscall, slot 5): IsDefaultImplementation by name: the shader kept under that
// implementation (or the default one (-1) having it), then whether a library says so
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

// 0x89CCA0 (thiscall, slot 4): IsDefaultImplementation by shader: the shader kept under its name
// and implementation (or as the default), then whether a library says so
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

// 0x89CD70 (thiscall, slot 8): RemoveAllShaders
void __fastcall ShaderFactory_RemoveAllShaders(uint8_t* self, re::Edx) {
    if (At<void*>(self, 0x28) != nullptr) {
        Map_RemoveAll(At<void*>(self, 0x28));
    }
}

// 0x89CD80 (thiscall, slot 36): every kept shader whose slot 3 says no given the renderer, its
// renderer data made again (slot 31) and registered with the renderer. (Name a guess.)
void __fastcall ShaderFactory_RecreateShaders(uint8_t* self, re::Edx) {
    if (At<void*>(self, 0x24) == nullptr || At<void*>(self, 0x28) == nullptr) {
        return;
    }
    re::Ptr32<void> shader = nullptr;
    re::Ptr32<void> position = Map_GetFirstPos(At<void*>(self, 0x28));
    while (position != nullptr) {
        re::Ptr32<const char> key;
        Map_GetNext(At<void*>(self, 0x28), &position, &key, &shader);
        if (shader != nullptr && Virtual<uint8_t>(shader, 3) == 0) {
            D3DShaderInterface_SetD3DRenderer(shader, At<void*>(self, 0x24));
            Virtual<void>(shader, 31);
            Renderer_RegisterD3DShader(At<void*>(self, 0x24), shader);
        }
    }
}

// 0x89CE10 (thiscall, slot 10): InsertLibrary (kept by its name; the map holds a reference)
void __fastcall ShaderFactory_InsertLibrary(uint8_t* self, re::Edx, uint8_t* library) {
    if (library == nullptr) {
        return;
    }
    Increment(library + 4);
    LibraryMap_SetAt(At<void*>(self, 0x30), ShaderLibrary_GetName(library), library);
}

// 0x89CE40 (thiscall, slot 11): FindLibrary (the reference the lookup took let go)
void* __fastcall ShaderFactory_FindLibrary(uint8_t* self, re::Edx, const char* name) {
    re::Ptr32<void> library = nullptr;
    if (LibraryMap_GetAt(At<void*>(self, 0x30), name, &library) != 0) {
        ReleaseObject(library);
        return library;
    }
    ReleaseObject(library);
    return nullptr;
}

// 0x89CEB0 (thiscall, slot 12): RemoveLibrary (only when the map and this lookup hold the only
// references); the iteration reset
void __fastcall ShaderFactory_RemoveLibrary(uint8_t* self, re::Edx, const char* name) {
    uint8_t* library = nullptr;
    if (LibraryMap_GetAt(At<void*>(self, 0x30), name, reinterpret_cast<re::Ptr32<void>*>(&library)) != 0 &&
        At<uint32_t>(library, 4) == 2) {
        StringMap_RemoveAt(At<void*>(self, 0x30), name);
        ReleaseObject(library);
        library = nullptr;
    }
    At<void*>(self, 0x2C) = nullptr;
    ReleaseObject(library);
}

// 0x89CF30 (thiscall, slot 14): GetFirstLibrary (the first bucket's first item; the reference
// the iteration took let go)
void* __fastcall ShaderFactory_GetFirstLibraryImpl(uint8_t* self, re::Edx) {
    uint8_t* map = At<uint8_t*>(self, 0x30);
    if (map == nullptr) {
        return nullptr;
    }
    void* position = nullptr;
    for (uint32_t i = 0; i < At<uint32_t>(map, 4); i++) {
        if (At<re::Ptr32<void>*>(map, 8)[i] != nullptr) {
            position = At<re::Ptr32<void>*>(map, 8)[i];
            break;
        }
    }
    At<void*>(self, 0x2C) = position;
    if (position == nullptr) {
        return nullptr;
    }
    re::Ptr32<void> library = nullptr;
    re::Ptr32<const char> key;
    LibraryMap_GetNext(map, &At<void*>(self, 0x2C), &key, &library);
    ReleaseObject(library);
    return library;
}

// 0x89CFC0 (thiscall, slot 15): GetNextLibrary
void* __fastcall ShaderFactory_GetNextLibrary(uint8_t* self, re::Edx) {
    if (At<void*>(self, 0x30) == nullptr || At<void*>(self, 0x2C) == nullptr) {
        return nullptr;
    }
    re::Ptr32<void> library = nullptr;
    re::Ptr32<const char> key;
    LibraryMap_GetNext(At<void*>(self, 0x30), &At<void*>(self, 0x2C), &key, &library);
    ReleaseObject(library);
    return library;
}

// 0x89D020 (thiscall, slot 9, `this` unused): a shader's slot 36 when it's an
// NiD3DShaderInterface, else false
uint8_t __fastcall ShaderFactory_Slot9(void*, re::Edx, void* shader) {
    if (shader != nullptr && HasClassBit(shader, NiD3DShaderInterface_ClassBit)) {
        return Virtual<uint8_t>(shader, 36);
    }
    return 0;
}

// 0x89D070 (thiscall): NiTPointerList<...> (vtable 0x95127C)'s deleting destructor (its nodes
// freed, its references let go)
void* __fastcall PointerList_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kPointerListVtable;
    PointerList_RemoveAll(self);
    At<uint32_t>(self, 0) = kAllocatorVtable;
    if (flags & 1) {
        NiDelete(self, 0x10);
    }
    return self;
}

// 0x89D0A0 (thiscall, slot 6): InsertShader (kept as name + implementation)
void __fastcall ShaderFactory_InsertShader(uint8_t* self, re::Edx, void* shader, uint32_t implementation) {
    char key[0x104];
    if (shader == nullptr) {
        return;
    }
    NiSprintf(key, sizeof(key), "%s%d", *Shader_GetName(shader), implementation);
    if (At<void*>(self, 0x28) != nullptr) {
        StringMap_SetAt(At<void*>(self, 0x28), key, shader);
    }
}

// 0x89D120 (thiscall, slot 3): FetchShader (the kept one, or null)
void* __fastcall ShaderFactory_FetchShader(uint8_t* self, re::Edx, const char* name, uint32_t implementation) {
    char key[0x104];
    if (At<void*>(self, 0x28) == nullptr) {
        return nullptr;
    }
    re::Ptr32<void> shader = nullptr;
    NiSprintf(key, sizeof(key), "%s%d", name, implementation);
    return StringMap_GetAt(At<void*>(self, 0x28), key, &shader) != 0 ? shader : nullptr;
}

// 0x89D1C0 (thiscall, slot 7): RemoveShader
void __fastcall ShaderFactory_RemoveShader(uint8_t* self, re::Edx, const char* name, uint32_t implementation) {
    char key[0x104];
    if (At<void*>(self, 0x28) != nullptr) {
        NiSprintf(key, sizeof(key), "%s%d", name, implementation);
        StringMap_RemoveAt(At<void*>(self, 0x28), key);
    }
}

// 0x89D230 (thiscall, slot 13): RemoveAllLibraries (each removed by name while iterating, the
// map emptied)
void __fastcall ShaderFactory_RemoveAllLibrariesImpl(uint8_t* self, re::Edx) {
    if (At<void*>(self, 0x30) == nullptr) {
        return;
    }
    for (void* library = ShaderFactory_GetFirstLibrary(self); library != nullptr;) {
        StringMap_RemoveAt(At<void*>(self, 0x30), ShaderLibrary_GetName(library));
        if (At<void*>(self, 0x30) == nullptr || At<void*>(self, 0x2C) == nullptr) {
            break;
        }
        re::Ptr32<void> next = nullptr;
        re::Ptr32<const char> key;
        LibraryMap_GetNext(At<void*>(self, 0x30), &At<void*>(self, 0x2C), &key, &next);
        ReleaseObject(next);
        library = next;
    }
    Map_RemoveAll(At<void*>(self, 0x30));
    At<void*>(self, 0x2C) = nullptr;
}

// 0x89D2C0: this factory made NiShaderFactory's (when another is): the other's callbacks taken
// where its slots 26..28 differ from them, its libraries moved here (gathered in a list, the
// other's removed, each inserted here). False when it already is. (Name a guess.)
uint8_t __cdecl ShaderFactory_TakeOverImpl() {
    if (NiShaderFactory_ms_pkShaderFactory == ms_pkD3DShaderFactory) {
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
            At<void*>(ms_pkD3DShaderFactory, 0x18) = Get18(NiShaderFactory_ms_pkShaderFactory);
        }
        callback = Virtual<void*>(NiShaderFactory_ms_pkShaderFactory, 27);
        if (Get1C(NiShaderFactory_ms_pkShaderFactory) != callback) {
            At<void*>(ms_pkD3DShaderFactory, 0x1C) = Get1C(NiShaderFactory_ms_pkShaderFactory);
        }
        callback = Virtual<void*>(NiShaderFactory_ms_pkShaderFactory, 28);
        if (Get20(NiShaderFactory_ms_pkShaderFactory) != callback) {
            At<void*>(ms_pkD3DShaderFactory, 0x20) = Get20(NiShaderFactory_ms_pkShaderFactory);
        }
        void* library = Virtual<void*>(NiShaderFactory_ms_pkShaderFactory, 14);
        while (library != nullptr) {
            re::Ptr32<void> held = library;
            Increment(static_cast<uint8_t*>(held) + 4);
            PointerList_AddTail(&libraries, &held);
            library = Virtual<void*>(NiShaderFactory_ms_pkShaderFactory, 15);
            ReleaseObject(held);
        }
        Virtual<void>(NiShaderFactory_ms_pkShaderFactory, 13);
    }
    NiShaderFactory_ms_pkShaderFactory = ms_pkD3DShaderFactory;
    for (NiTListNode<void*>* node = libraries.head; node != nullptr;) {
        void* library = node->element;
        node = node->next;
        if (library != nullptr) {
            Increment(static_cast<uint8_t*>(library) + 4);
        }
        Virtual<void>(ms_pkD3DShaderFactory, 10, library);
        ReleaseObject(library);
    }
    libraries.vtable = reinterpret_cast<void*>(kPointerListVtable);
    PointerList_RemoveAll(&libraries);
    return 1;
}

// 0x89D470 (thiscall): the string map's (vtable 0x9508CC) deleting destructor
void* __fastcall StringPointerMap_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    Map_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x14);
    }
    return self;
}

// 0x89D490 (thiscall): NiD3DShaderFactory::NiD3DShaderFactory (the lock, the shader map copying its
// keys, the library map)
uint8_t* __fastcall ShaderFactory_ConstructImpl(uint8_t* self, re::Edx) {
    ShaderFactory_ConstructBase(self);
    At<uint32_t>(self, 0) = kFactoryVtable;
    At<void*>(self, 0x24) = nullptr;
    At<void*>(self, 0x2C) = nullptr;
    FactoryLock(self).owner = 0;
    FactoryLock(self).lockCount = 0;
    Import<void>(ni::sys::kInitializeCriticalSection, &FactoryLock(self).section);
    uint8_t* map = static_cast<uint8_t*>(NiNew(0x14));
    if (map != nullptr) {
        At<uint32_t>(map, 0) = kMapBaseVtable;
        At<uint32_t>(map, 4) = 0x3B;
        At<uint32_t>(map, 0xC) = 0;
        At<void*>(map, 8) = NiMalloc(0xEC);
        crt::Memset(At<void*>(map, 8), 0, At<uint32_t>(map, 4) * 4);
        At<uint8_t>(map, 0x10) = 1;
        At<uint32_t>(map, 0) = kShaderMapVtable;
    }
    At<uint8_t*>(self, 0x28) = map;
    map = static_cast<uint8_t*>(NiNew(0x14));
    if (map == nullptr) {
        At<uint8_t*>(self, 0x30) = nullptr;
        return self;
    }
    At<uint32_t>(map, 0) = kLibraryMapBaseVtable;
    At<uint32_t>(map, 4) = 0x25;
    At<uint32_t>(map, 0xC) = 0;
    At<void*>(map, 8) = NiMalloc(0x94);
    crt::Memset(At<void*>(map, 8), 0, At<uint32_t>(map, 4) * 4);
    At<uint8_t>(map, 0x10) = 0;
    At<uint32_t>(map, 0) = kLibraryMapVtable;
    At<uint8_t*>(self, 0x30) = map;
    return self;
}

// 0x89D570 (thiscall): NiD3DShaderFactory::~NiD3DShaderFactory (the shaders and libraries let go,
// the maps deleted, the lock deleted)
void __fastcall ShaderFactory_DestructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kFactoryVtable;
    if (At<void*>(self, 0x28) != nullptr) {
        Map_RemoveAll(At<void*>(self, 0x28));
    }
    ShaderFactory_RemoveAllLibraries(self);
    if (At<void*>(self, 0x28) != nullptr) {
        Virtual<void>(At<void*>(self, 0x28), 0, 1u);
    }
    if (At<void*>(self, 0x30) != nullptr) {
        Virtual<void>(At<void*>(self, 0x30), 0, 1u);
    }
    At<void*>(self, 0x28) = nullptr;
    At<void*>(self, 0x30) = nullptr;
    At<void*>(self, 0x24) = nullptr;
    Import<void>(ni::sys::kDeleteCriticalSection, &FactoryLock(self).section);
    ShaderFactory_DestructBase(self);
}

// 0x89D5E0 (thiscall, slot 34): SetRenderer (this factory made NiShaderFactory's first; each
// library given the new renderer, slot 7)
void __fastcall ShaderFactory_SetRenderer(uint8_t* self, re::Edx, void* renderer) {
    if (NiShaderFactory_ms_pkShaderFactory != ms_pkD3DShaderFactory) {
        ShaderFactory_TakeOver();
    }
    if (At<void*>(self, 0x24) == renderer) {
        return;
    }
    At<void*>(self, 0x24) = renderer;
    for (void* library = Virtual<void*>(self, 14); library != nullptr; library = Virtual<void*>(self, 15)) {
        Virtual<void>(library, 7, renderer);
    }
}

// 0x89D630 (thiscall, slot 0): the deleting destructor
uint8_t* __fastcall ShaderFactory_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    ShaderFactory_Destruct(self);
    if (flags & 1) {
        NiDelete(self, kFactorySize);
    }
    return self;
}

// 0x89D650: NiD3DShaderFactory::Create (the instance, made NiShaderFactory's when there's none)
void __cdecl ShaderFactory_Create() {
    if (ms_pkD3DShaderFactory == nullptr) {
        void* memory = NiNew(kFactorySize);
        ms_pkD3DShaderFactory = memory != nullptr ? ShaderFactory_Construct(memory) : nullptr;
    }
    if (NiShaderFactory_ms_pkShaderFactory == nullptr) {
        NiShaderFactory_ms_pkShaderFactory = ms_pkD3DShaderFactory;
    }
    if (ms_pkD3DShaderFactory == nullptr) {
        NiOutputDebugString("Failed to create shader factory!");
    }
}

#define C41B_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C41B_REVERSED("NiD3DShaderFactory::Slot35", 0x0089C930, ShaderFactory_Slot35);
C41B_REVERSED("NiD3DShaderFactory::GetMajorShaderVersion", 0x0089C940, ShaderFactory_GetMajorShaderVersion);
C41B_REVERSED("NiD3DShaderFactory::CreateVertexShaderVersion", 0x0089C950, ShaderFactory_CreateVertexShaderVersion);
C41B_REVERSED("NiD3DShaderFactory::CreateGeometryShaderVersion", 0x0089C970,
              ShaderFactory_CreateGeometryShaderVersion);
C41B_REVERSED("NiD3DShaderFactory::GetMinorShaderVersion", 0x0089C990, ShaderFactory_GetMinorShaderVersion);
C41B_REVERSED("NiD3DShaderFactory::CreatePixelShaderVersion", 0x0089C9A0, ShaderFactory_CreatePixelShaderVersion);
C41B_REVERSED("NiD3DShaderFactory::GetD3DShaderFactory", 0x0089C9C0, ShaderFactory_GetD3DShaderFactory);
C41B_REVERSED("NiD3DShaderFactory::GetFirstProgramDirectory", 0x0089C9D0, ShaderFactory_GetFirstProgramDirectory);
C41B_REVERSED("NiD3DShaderFactory::GetNextProgramDirectory", 0x0089C9F0, ShaderFactory_GetNextProgramDirectory);
C41B_REVERSED("NiD3DShaderFactory::AddProgramDirectory", 0x0089CA10, ShaderFactory_AddProgramDirectory);
C41B_REVERSED("NiD3DShaderFactory::RemoveProgramDirectory", 0x0089CA30, ShaderFactory_RemoveProgramDirectory);
C41B_REVERSED("NiD3DShaderFactory::RemoveAllProgramDirectories", 0x0089CA50,
              ShaderFactory_RemoveAllProgramDirectories);
C41B_REVERSED("NiD3DShaderFactory::Shutdown", 0x0089CA70, ShaderFactory_Shutdown);
C41B_REVERSED("NiD3DShaderFactory::RetrieveShader", 0x0089CA90, ShaderFactory_RetrieveShader);
C41B_REVERSED("NiD3DShaderFactory::IsDefaultImplementationByName", 0x0089CBD0,
              ShaderFactory_IsDefaultImplementationByName);
C41B_REVERSED("NiD3DShaderFactory::IsDefaultImplementation", 0x0089CCA0, ShaderFactory_IsDefaultImplementation);
C41B_REVERSED("NiD3DShaderFactory::RemoveAllShaders", 0x0089CD70, ShaderFactory_RemoveAllShaders);
C41B_REVERSED("NiD3DShaderFactory::RecreateShaders", 0x0089CD80, ShaderFactory_RecreateShaders);
C41B_REVERSED("NiD3DShaderFactory::InsertLibrary", 0x0089CE10, ShaderFactory_InsertLibrary);
C41B_REVERSED("NiD3DShaderFactory::FindLibrary", 0x0089CE40, ShaderFactory_FindLibrary);
C41B_REVERSED("NiD3DShaderFactory::RemoveLibrary", 0x0089CEB0, ShaderFactory_RemoveLibrary);
C41B_REVERSED("NiD3DShaderFactory::GetFirstLibrary", 0x0089CF30, ShaderFactory_GetFirstLibraryImpl);
C41B_REVERSED("NiD3DShaderFactory::GetNextLibrary", 0x0089CFC0, ShaderFactory_GetNextLibrary);
C41B_REVERSED("NiD3DShaderFactory::Slot9", 0x0089D020, ShaderFactory_Slot9);
C41B_REVERSED("NiTPointerList<NiShaderLibraryPtr>::DeletingDestructor", 0x0089D070, PointerList_DeletingDestructor);
C41B_REVERSED("NiD3DShaderFactory::InsertShader", 0x0089D0A0, ShaderFactory_InsertShader);
C41B_REVERSED("NiD3DShaderFactory::FetchShader", 0x0089D120, ShaderFactory_FetchShader);
C41B_REVERSED("NiD3DShaderFactory::RemoveShader", 0x0089D1C0, ShaderFactory_RemoveShader);
C41B_REVERSED("NiD3DShaderFactory::RemoveAllLibraries", 0x0089D230, ShaderFactory_RemoveAllLibrariesImpl);
C41B_REVERSED("NiD3DShaderFactory::TakeOver", 0x0089D2C0, ShaderFactory_TakeOverImpl);
C41B_REVERSED("NiTStringPointerMap<NiShader*>::DeletingDestructor", 0x0089D470, StringPointerMap_DeletingDestructor);
C41B_REVERSED("NiD3DShaderFactory::NiD3DShaderFactory", 0x0089D490, ShaderFactory_ConstructImpl);
C41B_REVERSED("NiD3DShaderFactory::~NiD3DShaderFactory", 0x0089D570, ShaderFactory_DestructImpl);
C41B_REVERSED("NiD3DShaderFactory::SetRenderer", 0x0089D5E0, ShaderFactory_SetRenderer);
C41B_REVERSED("NiD3DShaderFactory::DeletingDestructor", 0x0089D630, ShaderFactory_DeletingDestructor);
C41B_REVERSED("NiD3DShaderFactory::Create", 0x0089D650, ShaderFactory_Create);

} // namespace ni::c41
