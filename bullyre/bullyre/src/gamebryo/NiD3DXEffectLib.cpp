// The rest of Gamebryo's D3DX effect shader library (NiD3DXEffectShaderLib): the entry point the
// shader factory loads it through, NiD3DXEffectShaderLibrary's creation and its smaller slots,
// NiD3DXEffectLoader (the singleton, 0x14 bytes: an iterator and a list of the effect files found
// on disk, NiMalloc'd paths), the factory's and the parameters' statics let go, the state
// manager's SetPixelShader, and the library's static data (NiD3DXEffectShaderLibSDM: its
// init/shutdown pair and the table of object property names it builds).

#include "gamebryo/NiD3DXEffect.h"

#include <cstring>
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::d3dx;

constexpr uintptr_t Library_vtable = 0x00950774;
constexpr uintptr_t LibraryFileList_vtable = 0x0095145C;   // the files' NiTPointerList
constexpr uintptr_t PointerMapBase_vtable = 0x00950760;
constexpr uintptr_t TechniqueSetMap_vtable = 0x009507B0;   // NiTStringPointerMap<NiD3DXEffectTechniqueSet*>
constexpr uintptr_t PointerList_vtable = 0x00952E10;
constexpr uintptr_t PointerListBase_vtable = 0x0099C748;

GAME_VAR(uint32_t, NiD3DRenderer_ClassIndex, 0x020E6D20);   // (IsKindOf's bit)
GAME_VAR(void*, NiD3DXEffect_ms_pkD3DRenderer, 0x020DCE58);   // (what 0x7486A0 returns)

// NiD3DXEffectShaderLibrary (the factory file's layout, 0x3C bytes)
struct NiD3DXEffectShaderLibrary {
    re::Ptr32<void> vtable;                 // +0x0
    uint32_t refCount;            // +0x4
    uint8_t base[0xC];            // +0x8 (NiShaderLibrary's: the name, +0xC the renderer)
    re::Ptr32<NiRefObject> desc;            // +0x14
    PointerList files;            // +0x18 (NiD3DXEffectFile*)
    StringMap techniqueSets;      // +0x28 (37 buckets, keys copied)
};
static_assert(offsetof(NiD3DXEffectShaderLibrary, files) == 0x18);
static_assert(offsetof(NiD3DXEffectShaderLibrary, techniqueSets) == 0x28);
static_assert(sizeof(NiD3DXEffectShaderLibrary) == 0x3C);

// NiD3DXEffectLoader: the list's iterator, the paths found
struct NiD3DXEffectLoader {
    re::Ptr32<ListNode> iterator;   // +0x0
    PointerList files;    // +0x4 (char*, NiMalloc'd)
};
static_assert(sizeof(NiD3DXEffectLoader) == 0x14);

GAME_VAR(NiD3DXEffectLoader*, NiD3DXEffectLoader_ms_pkLoader, 0x020DCE5C);

// the object property names (NiTPair<unsigned int, NiFixedString>, new[]'d: the count before it)
struct PropertyName {
    uint32_t id;          // +0x0
    re::Ptr32<const char> name;     // +0x4 (NiFixedString)
};
static_assert(sizeof(PropertyName) == 8);
GAME_VAR(PropertyName*, ms_pkObjectPropertyNames, 0x020DDEDC);
GAME_VAR(uint32_t, ms_uiObjectPropertyNames, 0x00BC74E4);   // (18)
GAME_VAR(uint8_t, NiD3DXEffectShaderLibSDM_ms_bInitialized, 0x020DDEAC);
GAME_VAR(int32_t, NiD3DXEffectShaderLibSDM_ms_iInitCount, 0x020DDEB0);
GAME_VAR(float*, ms_pfBoneMatrices, 0x020DCEA0);

// the Win32 file search, through the game's import slots
constexpr uintptr_t kFindClose = 0x008FC0CC;
constexpr uintptr_t kFindNextFileA = 0x008FC0D0;
constexpr uintptr_t kGetFileAttributesA = 0x008FC0D4;
constexpr uintptr_t kFindFirstFileA = 0x008FC0D8;

// WIN32_FIND_DATAA (0x140 bytes: the name at +0x2C)
struct FindData {
    uint8_t header[0x2C];
    char fileName[0x104];   // +0x2C
    char alternate[0x10];
};
static_assert(sizeof(FindData) == 0x140);

GAME_FN(void, __cdecl, NiDelete_, 0x00752540, void* p, uint32_t size);
GAME_FN(void*, __cdecl, NiNew_, 0x00752460, uint32_t size);
GAME_FN(void*, __cdecl, NiNewArray_, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiDeleteArray_, 0x00752570, void* p, uint32_t size);
GAME_FN(void*, __cdecl, NiMalloc_, 0x00752300, uint32_t size);
GAME_FN(void, __cdecl, NiFree_, 0x00752380, void* p);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);   // the game's operator delete
GAME_FN(void, __cdecl, NiD3DRenderer_Error, 0x00888620, const char* format, ...);
GAME_FN(void, __cdecl, NiD3DUtility_GetFileExtension, 0x00888490, const char* path, char* ext, uintptr_t size);
GAME_FN(void, __cdecl, NiD3DUtility_GetFileName, 0x00888680, const char* path, char* name, uint32_t size);
GAME_FN(void, __thiscall, NiTPointerList_RemoveAll, 0x0075DB60, PointerList* list);
GAME_FN(uint8_t, __thiscall, PointerMap_GetAt, 0x00800470, StringMap* map, const char* key, re::Ptr32<void>* value);
GAME_FN(void, __thiscall, ShaderLibrary_Construct, 0x00889E40, void* self, const char* name);
GAME_FN(void, __thiscall, D3DShaderLibrary_SetRenderer, 0x008C00F0, void* self, void* renderer);
GAME_FN(const re::Ptr32<const char>*, __thiscall, Shader_GetName, 0x0077C280, void* shader);
GAME_FN(uint32_t, __thiscall, Shader_GetImplementation, 0x00853140, void* shader);
GAME_FN(void, __cdecl, NiD3DShaderProgramFactory_SetDirectory, 0x00889ED0, const char* directory);
GAME_FN(void, __cdecl, NiStaticDataManager_AddLibrary, 0x00753680, void(__cdecl* init)(), void(__cdecl* shutdown)());
GAME_FN(const char*, __cdecl, StringTable_AddString, 0x00754760, const char* text);
GAME_FN(void, __thiscall, StringTable_RemoveString, 0x00754550, void* table, re::Ptr32<const char>* handle, uint32_t hash);
GAME_VAR(void*, GlobalStringTable, 0x020E216C);
GAME_VAR(const char*, NullFixedString, 0x00951028);
// `count` objects built at `p` by `construct` (the compiler's vector constructor iterator)
GAME_FN(void, __stdcall, VectorConstructor, 0x00401240, void* p, uint32_t size, uint32_t count,
        void*(__thiscall* construct)(void*));
// this library's, called where modes apply
GAME_FN(void, __thiscall, PointerList_AddTail, 0x00748170, PointerList* list, const re::Ptr32<void>* element);
GAME_FN(NiD3DXEffectFile*, __thiscall, Library_FindEffect, 0x00748130, NiD3DXEffectShaderLibrary* self,
        const char* name);
GAME_FN(uint8_t, __thiscall, Library_AddEffect, 0x00748310, NiD3DXEffectShaderLibrary* self,
        NiD3DXEffectFile* file);
GAME_FN(NiD3DXEffectShaderLibrary*, __thiscall, Library_Construct, 0x007484D0, void* self);
GAME_FN(NiD3DXEffectShaderLibrary*, __cdecl, Library_Create, 0x00748540, void* renderer, int count,
        re::Ptr32<char>* directories, int recurse);
GAME_FN(void*, __cdecl, GetD3DRenderer, 0x007486A0);
GAME_FN(NiD3DXEffectFile*, __cdecl, Factory_LoadEffect, 0x00748FF0, const char* name);
GAME_FN(void, __thiscall, Factory_DestroyRendererData, 0x007488D0, NiD3DXEffectFactory* self);
GAME_FN(void, __cdecl, Factory_SetD3DRenderer, 0x00748980, void* renderer);
GAME_FN(void, __cdecl, Factory_Shutdown, 0x00748BB0);
GAME_FN(void, __thiscall, Loader_Destruct, 0x00749170, NiD3DXEffectLoader* self);
GAME_FN(void, __cdecl, Loader_Destroy, 0x00749260);
GAME_FN(uint8_t, __thiscall, Loader_ProcessFile, 0x00749290, NiD3DXEffectLoader* self, const char* path,
        const char* ext, PointerList* list);
GAME_FN(uint8_t, __thiscall, Loader_LoadAllFiles, 0x00749370, NiD3DXEffectLoader* self,
        NiD3DXEffectShaderLibrary* library);
GAME_FN(NiD3DXEffectLoader*, __cdecl, Loader_Create, 0x00749610);
GAME_FN(uint32_t, __thiscall, Loader_FindFilesInDirectory, 0x007496A0, NiD3DXEffectLoader* self,
        const char* directory, const char* ext, int recurse, PointerList* list);
GAME_FN(void, __thiscall, Loader_FindAllFiles, 0x00749A60, NiD3DXEffectLoader* self, const char* directory,
        int recurse);
GAME_FN(uint8_t, __thiscall, Loader_LoadAll, 0x00749BA0, NiD3DXEffectLoader* self,
        NiD3DXEffectShaderLibrary* library, const char* directory, int recurse);
GAME_FN(uint8_t, __thiscall, Technique_ReleaseShader, 0x00749F00, NiD3DXEffectTechnique* self);
GAME_FN(void, __cdecl, Parameter_ReleaseBoneArray, 0x0074A3E0);
GAME_FN(void, __cdecl, LibSDM_BuildPropertyNames, 0x0074FD40);
GAME_FN(PropertyName*, __thiscall, PropertyName_VectorDeletingDestructor, 0x00750770, PropertyName* self,
        uint32_t flags);
GAME_FN(void, __cdecl, LibSDM_DestroyPropertyNames, 0x00750840);
// NiTPair<unsigned int, NiFixedString> (the shader constant map's file: other chunks')
GAME_FN(PropertyName*, __thiscall, PropertyName_Construct, 0x0077F5A0, PropertyName* self, uint32_t id,
        const re::Ptr32<const char>* name);
GAME_FN(void*, __thiscall, PropertyName_ConstructDefault, 0x0077F5D0, void* self);
GAME_FN(PropertyName*, __thiscall, PropertyName_Assign, 0x0077F5F0, PropertyName* self, const PropertyName* other);

inline uint8_t IsKindOf(void* object, uint32_t classIndex) {
    const uint32_t* rtti = Virtual<const uint32_t*>(object, 2);
    return (rtti[3 + (classIndex >> 5)] & (1u << (classIndex & 31))) != 0 ? 1 : 0;
}

// a fixed string's handle let go (NiFixedString's destructor, inlined)
inline void FixedStringRelease(re::Ptr32<const char>* handle) {
    const char* s = *handle;
    if (s != NullFixedString) {
        uint32_t hash = *reinterpret_cast<const uint16_t*>(s - 2);
        if (Decrement(const_cast<char*>(s) - 8) == 1) {
            StringTable_RemoveString(GlobalStringTable, handle, hash);
        }
    }
}

// the renderer's lock (NiRenderer::LockRenderer / UnlockRenderer, inlined): its critical
// section at +0x30 (the owner at +0x48, the count at +0x4C); slots 80 / 81 on the first lock and
// the last unlock
inline void LockRenderer(uint8_t* renderer) {
    Import<void>(sys::kEnterCriticalSection, renderer + 0x30);
    At<uint32_t>(renderer, 0x48) = Import<uint32_t>(sys::kGetCurrentThreadId);
    At<uint32_t>(renderer, 0x4C) += 1;
    if (At<uint32_t>(renderer, 0x4C) == 1) {
        Virtual<void>(renderer, 80);
    }
}
inline void UnlockRenderer(uint8_t* renderer) {
    if (At<uint32_t>(renderer, 0x4C) == 1) {
        Virtual<void>(renderer, 81);
    }
    if (--At<uint32_t>(renderer, 0x4C) == 0) {
        At<uint32_t>(renderer, 0x48) = 0;
    }
    Import<void>(sys::kLeaveCriticalSection, renderer + 0x30);
}

// a directory as the loader keeps it: '\' at the end, every '/' made '\' (the ones before the
// added '\')
inline void StandardizeDirectory(char* path) {
    int length = int(std::strlen(path));
    if (path[length - 1] != '\\' && path[length - 1] != '/') {
        path[length] = '\\';
        path[length + 1] = 0;
    }
    for (int i = 0; i < length; i++) {
        if (path[i] == '/') {
            path[i] = '\\';
        }
    }
}

} // namespace

// ---- the library's entry point (what the shader factory's class creation callback calls)

// 0x746470: NiD3DXEffectShaderLib's LoadShaderLibrary: a renderer that isn't a D3D one gets no
// library (and succeeds); else the library made from the directories
uint8_t __cdecl NiD3DXEffectShaderLib_LoadShaderLibrary(void* renderer, int count, re::Ptr32<char>* directories,
                                                        int recurse, re::Ptr32<void>* library) {
    if (library == nullptr) {
        return 0;
    }
    if (renderer != nullptr && !IsKindOf(renderer, NiD3DRenderer_ClassIndex)) {
        *library = nullptr;
        return 1;
    }
    NiD3DXEffectShaderLibrary* created = Library_Create(renderer, count, directories, recurse);
    *library = created;
    return created != nullptr ? 1 : 0;
}

// ---- NiD3DXEffectShaderLibrary

// 0x747EC0 and 0x747ED0: slots 5 and 6, lone jumps to NiShaderLibrary's GetShaderLibraryDesc
// (0x8B6120) and SetShaderLibraryDesc (0x889F20)
RE_UNHOOKED("NiD3DXEffectShaderLibrary::GetShaderLibraryDesc", 0x00747EC0);
RE_UNHOOKED("NiD3DXEffectShaderLibrary::SetShaderLibraryDesc", 0x00747ED0);

// 0x747EE0: SetRenderer (slot 7): a different renderer given to the base and the factory
void __fastcall NiD3DXEffectShaderLibrary_SetRenderer(NiD3DXEffectShaderLibrary* self, re::Edx, void* renderer) {
    if (At<void*>(self, 0xC) != renderer) {
        D3DShaderLibrary_SetRenderer(self, renderer);
        Factory_SetD3DRenderer(renderer);
    }
}

// 0x7480F0: ReleaseShader(NiShader*) (slot 3): by its name and implementation (slot 4)
uint8_t __fastcall NiD3DXEffectShaderLibrary_ReleaseShaderObject(NiD3DXEffectShaderLibrary* self, re::Edx,
                                                                 void* shader) {
    if (shader == nullptr) {
        return 0;
    }
    const char* name = *Shader_GetName(shader);
    uint32_t implementation = Shader_GetImplementation(shader);
    return Virtual<uint8_t>(self, 4, name, implementation);
}

// 0x7482C0: ReleaseShader(name, implementation) (slot 4): the technique's shader let go
uint8_t __fastcall NiD3DXEffectShaderLibrary_ReleaseShader(NiD3DXEffectShaderLibrary* self, re::Edx,
                                                           const char* name, uint32_t implementation) {
    re::Ptr32<void> value = nullptr;
    if (!PointerMap_GetAt(&self->techniqueSets, name, &value) || value == nullptr) {
        return 0;
    }
    NiD3DXEffectTechniqueSet* set = static_cast<NiD3DXEffectTechniqueSet*>(value);
    if (implementation >= set->techniques.size) {
        return 0;
    }
    NiD3DXEffectTechnique* technique = set->techniques.base[implementation];
    if (technique == nullptr) {
        return 0;
    }
    return Technique_ReleaseShader(technique);
}

// 0x7484D0: the constructor
NiD3DXEffectShaderLibrary* __fastcall NiD3DXEffectShaderLibrary_Construct(NiD3DXEffectShaderLibrary* self, re::Edx) {
    ShaderLibrary_Construct(self, "NiD3DXEffectShaderLib");
    self->vtable = reinterpret_cast<void*>(Library_vtable);
    self->files.count = 0;
    self->files.head = nullptr;
    self->files.tail = nullptr;
    self->files.vtable = reinterpret_cast<void*>(LibraryFileList_vtable);
    StringMap& map = self->techniqueSets;
    map.vtable = reinterpret_cast<void*>(PointerMapBase_vtable);
    map.hashSize = 0x25;
    map.count = 0;
    map.table = static_cast<re::Ptr32<MapItem>*>(NiMalloc_(0x94));
    crt::Memset(map.table, 0, map.hashSize * 4);
    map.copyKeys = 1;
    map.vtable = reinterpret_cast<void*>(TechniqueSetMap_vtable);
    return self;
}

// 0x748540: Create: the library given the renderer (with none, only when there is a D3D
// renderer: the one passed on stays null, as the game has it) and each directory's effect files;
// none loaded: deleted, null
NiD3DXEffectShaderLibrary* __cdecl NiD3DXEffectShaderLibrary_Create(void* renderer, int count, re::Ptr32<char>* directories,
                                                                    int recurse) {
    if (renderer == nullptr && GetD3DRenderer() == nullptr) {
        return nullptr;
    }
    void* memory = NiNew_(sizeof(NiD3DXEffectShaderLibrary));
    if (memory == nullptr) {
        return nullptr;
    }
    NiD3DXEffectShaderLibrary* library = Library_Construct(memory);
    if (library == nullptr) {
        return nullptr;
    }
    Virtual<void>(library, 7, renderer);   // SetRenderer
    for (int i = 0; i < count; i++) {
        char* directory = directories[i];
        NiD3DXEffectLoader* loader = Loader_Create();
        if (!Loader_LoadAll(loader, library, directory, recurse)) {
            NiD3DRenderer_Error("LoadD3DXEffectFiles failed on %s\n", directories[i]);
        }
    }
    if (library->files.count != 0) {
        return library;
    }
    Virtual<void>(library, 0, 1);   // deleted
    return nullptr;
}

// 0x7486A0: the D3D renderer the effects were set up for
void* __cdecl NiD3DXEffect_GetD3DRenderer() {
    return NiD3DXEffect_ms_pkD3DRenderer;
}

// ---- NiD3DXEffectFactory's statics

// 0x748BB0: Shutdown: the factory deleted, the device released (the renderer data destroyed when
// there's a factory: there's none by then), the effect directory freed
void __cdecl NiD3DXEffectFactory_Shutdown() {
    if (NiD3DXEffectFactory_ms_pkFactory != nullptr) {
        Virtual<void>(NiD3DXEffectFactory_ms_pkFactory, 3, 1);
    }
    NiD3DXEffectFactory_ms_pkFactory = nullptr;
    if (NiD3DXEffectFactory_ms_pkD3DRenderer != nullptr) {
        if (NiD3DXEffectFactory_ms_pkD3DDevice != nullptr) {
            Com<unsigned long>(NiD3DXEffectFactory_ms_pkD3DDevice, 2);   // Release
            NiD3DXEffectFactory_ms_pkD3DDevice = nullptr;
            if (NiD3DXEffectFactory_ms_pkFactory != nullptr) {
                Factory_DestroyRendererData(NiD3DXEffectFactory_ms_pkFactory);
            }
        }
        NiD3DXEffectFactory_ms_pkD3DRenderer = nullptr;
    }
    Free(NiD3DXEffectFactory_ms_pcEffectDirectory);
    NiD3DXEffectFactory_ms_pcEffectDirectory = nullptr;
}

// ---- NiD3DXEffectLoader

// 0x749170: the destructor: the paths freed, the list emptied
void __fastcall NiD3DXEffectLoader_Destruct(NiD3DXEffectLoader* self, re::Edx) {
    self->iterator = self->files.head;
    while (self->iterator != nullptr) {
        ListNode* node = self->iterator;
        self->iterator = node->next;
        if (node->element != nullptr) {
            NiFree_(node->element);
        }
    }
    NiTPointerList_RemoveAll(&self->files);
    self->files.vtable = reinterpret_cast<void*>(PointerList_vtable);
    NiTPointerList_RemoveAll(&self->files);
    self->files.vtable = reinterpret_cast<void*>(PointerListBase_vtable);
}

// 0x749260: Destroy (the singleton deleted)
void __cdecl NiD3DXEffectLoader_Destroy() {
    NiD3DXEffectLoader* loader = NiD3DXEffectLoader_ms_pkLoader;
    if (loader != nullptr) {
        Loader_Destruct(loader);
        NiDelete_(loader, sizeof(NiD3DXEffectLoader));
    }
    NiD3DXEffectLoader_ms_pkLoader = nullptr;
}

// 0x749290: one file found: kept (a copy of its path) when its extension is the one looked for
uint8_t __fastcall NiD3DXEffectLoader_ProcessFile(NiD3DXEffectLoader*, re::Edx, const char* path, const char* ext,
                                                 PointerList* list) {
    if (path == nullptr || path[0] == 0) {
        return 0;
    }
    char extension[0x100];
    NiD3DUtility_GetFileExtension(path, extension, sizeof(extension));
    if (crt::Stricmp(extension, ext) != 0) {
        return 0;
    }
    NiD3DRenderer_Error("        Found %s File %s\n", ext, path);
    uint32_t size = uint32_t(std::strlen(path)) + 1;
    char* copy = static_cast<char*>(NiMalloc_(size));
    crt::StrcpyS(copy, size, path);
    PointerList_AddTail(list, re::PtrCast<const re::Ptr32<void>*>(&copy));
    return 1;
}

// 0x749370: every file found loaded into the library (the renderer locked meanwhile), the ones it
// has already skipped; true
uint8_t __fastcall NiD3DXEffectLoader_LoadAllFiles(NiD3DXEffectLoader* self, re::Edx,
                                                  NiD3DXEffectShaderLibrary* library) {
    uint8_t* renderer = NiDX9Renderer_ms_pkRenderer;
    if (renderer != nullptr) {
        LockRenderer(renderer);
    }
    self->iterator = self->files.head;
    if (self->iterator != nullptr) {
        ListNode* node = self->iterator;
        self->iterator = node->next;
        const char* path = static_cast<const char*>(node->element);
        while (path != nullptr) {
            char name[0x104];
            NiD3DUtility_GetFileName(path, name, sizeof(name));
            if (Library_FindEffect(library, name) != nullptr) {
                NiD3DRenderer_Error("%s (%s) already loaded?\n", name, path);
            } else {
                NiD3DRenderer_Error("Attempting to load %s\n", path);
                NiD3DXEffectFile* file = Factory_LoadEffect(path);
                if (file != nullptr) {
                    NiD3DRenderer_Error("Loaded %s\n", path);
                    Library_AddEffect(library, file);
                } else {
                    NiD3DRenderer_Error("Failed to load %s\n", path);
                }
            }
            node = self->iterator;
            if (node == nullptr) {
                break;
            }
            self->iterator = node->next;
            path = static_cast<const char*>(node->element);
        }
    }
    if (renderer != nullptr) {
        UnlockRenderer(renderer);
    }
    return 1;
}

// 0x749610: Create: the singleton (an existing one emptied of its paths)
NiD3DXEffectLoader* __cdecl NiD3DXEffectLoader_Create() {
    NiD3DXEffectLoader* loader = NiD3DXEffectLoader_ms_pkLoader;
    if (loader != nullptr) {
        for (ListNode* node = loader->files.head; node != nullptr;) {
            void* path = node->element;
            node = node->next;
            if (path != nullptr) {
                NiFree_(path);
            }
        }
        NiTPointerList_RemoveAll(&NiD3DXEffectLoader_ms_pkLoader->files);
        return NiD3DXEffectLoader_ms_pkLoader;
    }
    loader = static_cast<NiD3DXEffectLoader*>(NiNew_(sizeof(NiD3DXEffectLoader)));
    if (loader == nullptr) {
        NiD3DXEffectLoader_ms_pkLoader = nullptr;
        return nullptr;
    }
    loader->files.count = 0;
    loader->files.head = nullptr;
    loader->files.tail = nullptr;
    loader->files.vtable = reinterpret_cast<void*>(PointerList_vtable);
    NiTPointerList_RemoveAll(&loader->files);
    NiD3DXEffectLoader_ms_pkLoader = loader;
    return loader;
}

// 0x7496A0: the files of a directory with the extension (into the list), its subdirectories'
// too when recursing; how many were kept
uint32_t __fastcall NiD3DXEffectLoader_FindFilesInDirectory(NiD3DXEffectLoader* self, re::Edx, const char* directory,
                                                           const char* ext, int recurse, PointerList* list) {
    if (directory == nullptr || directory[0] == 0 || ext == nullptr || ext[0] == 0) {
        return 0;
    }
    char base[0x104];
    crt::StrncpyS(base, 0x104, directory, 0x103);
    StandardizeDirectory(base);
    char search[0x104];
    crt::StrcpyS(search, 0x104, directory);
    StandardizeDirectory(search);
    crt::StrcatS(search, 0x104, "*");
    FindData found;
    crt::Memset(&found, 0, sizeof(found));
    uint32_t count = 0;
    void* find = Import<void*>(kFindFirstFileA, search, &found);
    if (find == reinterpret_cast<void*>(-1)) {
        return count;
    }
    char path[0x104];
    crt::StrcpyS(path, 0x104, base);
    crt::StrcatS(path, 0x104, found.fileName);
    do {
        crt::StrcpyS(path, 0x104, base);
        crt::StrcatS(path, 0x104, found.fileName);
        uint32_t attributes = Import<uint32_t>(kGetFileAttributesA, path);
        if ((attributes & 0x10) != 0) {   // FILE_ATTRIBUTE_DIRECTORY
            if (std::strcmp(found.fileName, ".") != 0 && std::strcmp(found.fileName, "..") != 0 &&
                re::Flag(recurse)) {
                crt::StrcatS(path, 0x104, "\\");
                NiD3DRenderer_Error("    Recurse directory %s\n", path);
                count += Loader_FindFilesInDirectory(self, path, ext, recurse, list);
            }
        } else if (Loader_ProcessFile(self, path, ext, list)) {
            count++;
        }
    } while (Import<int>(kFindNextFileA, find, &found) != 0);
    Import<int>(kFindClose, find);
    return count;
}

// 0x749A60: the effect files of a directory: .FX, .FXO, .FXB, and .FXL when those are used
void __fastcall NiD3DXEffectLoader_FindAllFiles(NiD3DXEffectLoader* self, re::Edx, const char* directory,
                                               int recurse) {
    PointerList* list = &self->files;
    Loader_FindFilesInDirectory(self, directory, ".FX", recurse, list);
    Loader_FindFilesInDirectory(self, directory, ".FXO", recurse, list);
    Loader_FindFilesInDirectory(self, directory, ".FXB", recurse, list);
    if (NiD3DXEffectFactory_ms_bUseFallbacks != 0) {
        Loader_FindFilesInDirectory(self, directory, ".FXL", recurse, list);
    }
}

// 0x749BA0: a directory's effect files found and loaded into the library
uint8_t __fastcall NiD3DXEffectLoader_LoadAll(NiD3DXEffectLoader* self, re::Edx, NiD3DXEffectShaderLibrary* library,
                                             const char* directory, int recurse) {
    Loader_FindAllFiles(self, directory, recurse);
    return Loader_LoadAllFiles(self, library) != 0 ? 1 : 0;
}

// ---- NiD3DXEffectTechnique

// 0x749F00: ReleaseShader (whether it had one)
uint8_t __fastcall NiD3DXEffectTechnique_ReleaseShader(NiD3DXEffectTechnique* self, re::Edx) {
    NiD3DXEffectShader* shader = self->shader;
    uint8_t had = shader != nullptr ? 1 : 0;
    if (shader != nullptr) {
        if (Decrement(&shader->refCount) == 0) {
            Virtual<void>(shader, 1);
        }
        self->shader = nullptr;
    }
    return had;
}

// ---- NiD3DXEffectParameter

// 0x74A3E0: ReleaseBoneArray (the count stays)
void __cdecl NiD3DXEffectParameter_ReleaseBoneArray() {
    Free(ms_pfBoneMatrices);
    ms_pfBoneMatrices = nullptr;
}

// ---- NiD3DXEffectStateManager

// 0x74D6A0: SetPixelShader (the device's only when it changes)
long __stdcall NiD3DXEffectStateManager_SetPixelShader(NiD3DXEffectStateManager* self, void* shader) {
    uint8_t* renderState = self->renderState;
    if (At<void*>(renderState, 0x20E8) != shader) {
        At<void*>(renderState, 0x20E8) = shader;
        Com<long>(At<void*>(renderState, 0x2238), kDevSetPixelShader, shader);
    }
    return 0;
}

// ---- NiD3DXEffectShaderLibSDM

// 0x74D930: Init (once)
void __cdecl NiD3DXEffectShaderLibSDM_Init() {
    if (NiD3DXEffectShaderLibSDM_ms_bInitialized == 0) {
        NiD3DXEffectShaderLibSDM_ms_bInitialized = 1;
        LibSDM_BuildPropertyNames();
    }
}

// 0x74D950: Shutdown: the names, the bone array, the program directory, the loader, the factory
void __cdecl NiD3DXEffectShaderLibSDM_Shutdown() {
    if (NiD3DXEffectShaderLibSDM_ms_bInitialized != 0) {
        NiD3DXEffectShaderLibSDM_ms_bInitialized = 0;
        LibSDM_DestroyPropertyNames();
        Parameter_ReleaseBoneArray();
        NiD3DShaderProgramFactory_SetDirectory(nullptr);
        Loader_Destroy();
        Factory_Shutdown();
    }
}

// 0x74D980: the static object's constructor: the pair registered by the first one
void* __fastcall NiD3DXEffectShaderLibSDM_Construct(void* self, re::Edx) {
    int32_t count = NiD3DXEffectShaderLibSDM_ms_iInitCount;
    NiD3DXEffectShaderLibSDM_ms_iInitCount = count + 1;
    if (count == 0) {
        NiStaticDataManager_AddLibrary(reinterpret_cast<void(__cdecl*)()>(0x0074D930),
                                       reinterpret_cast<void(__cdecl*)()>(0x0074D950));
    }
    return self;
}

// 0x74FD40: the object property names (NiShaderConstantMap's object mappings by name): the array
// new[]'d, each entry assigned from a temporary pair (both of its strings let go after)
void __cdecl NiD3DXEffectShaderLibSDM_BuildPropertyNames() {
    static const struct {
        uint32_t id;
        const char* name;
    } kNames[] = {
        {1, "Dimmer"},
        {5, "Ambient"},
        {6, "Diffuse"},
        {7, "Specular"},
        {8, "Position"},
        {10, "Direction"},
        {12, "World"},
        {15, "Attenuation"},
        {14, "SpotAttenuation"},
        {16, "Projection"},
        {18, "ProjectionTranslation"},
        {20, "ClippingPlane"},
        {22, "TexCoordGen"},
        {25, "WorldToShadowMapMatrix"},
        {26, "ShadowMap"},
        {27, "TexSizeShadow"},
        {28, "ShadowBias"},
        {29, "ShadowVSMPowerEpsilon"},
    };
    uint32_t count = ms_uiObjectPropertyNames;
    // new[] of count * 8 + 4 (the count first), saturated on overflow as the compiler does it
    uint64_t wide = uint64_t(count) * 8;
    uint32_t bytes = (wide >> 32) != 0 ? 0xFFFFFFFFu : uint32_t(wide);
    bytes = bytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : bytes + 4;
    uint32_t* block = static_cast<uint32_t*>(NiNewArray_(bytes));
    if (block == nullptr) {
        ms_pkObjectPropertyNames = nullptr;
    } else {
        block[0] = count;
        VectorConstructor(block + 1, sizeof(PropertyName), count,
                          reinterpret_cast<void*(__thiscall*)(void*)>(0x0077F5D0));
        ms_pkObjectPropertyNames = reinterpret_cast<PropertyName*>(block + 1);
    }
    for (uint32_t i = 0; i < sizeof(kNames) / sizeof(kNames[0]); i++) {
        re::Ptr32<const char> name = StringTable_AddString(kNames[i].name);
        PropertyName pair;
        PropertyName* temporary = PropertyName_Construct(&pair, kNames[i].id, &name);
        PropertyName_Assign(&ms_pkObjectPropertyNames[i], temporary);
        FixedStringRelease(&pair.name);
        FixedStringRelease(&name);
    }
}

// 0x750770: the pair's vector deleting destructor (flags 2: the array, its count before it;
// 1: freed)
PropertyName* __fastcall NiD3DXEffectShaderLibSDM_PropertyNameDelete(PropertyName* self, re::Edx, uint32_t flags) {
    if ((flags & 2) == 0) {
        FixedStringRelease(&self->name);
        if ((flags & 1) != 0) {
            NiDelete_(self, sizeof(PropertyName));
        }
        return self;
    }
    uint32_t* block = reinterpret_cast<uint32_t*>(self) - 1;
    for (int32_t i = int32_t(block[0]) - 1; i >= 0; i--) {
        FixedStringRelease(&self[i].name);
    }
    if ((flags & 1) != 0) {
        NiDeleteArray_(block, sizeof(PropertyName));
    }
    return reinterpret_cast<PropertyName*>(block);
}

// 0x750840: the names destroyed
void __cdecl NiD3DXEffectShaderLibSDM_DestroyPropertyNames() {
    if (ms_pkObjectPropertyNames != nullptr) {
        PropertyName_VectorDeletingDestructor(ms_pkObjectPropertyNames, 3);
    }
    ms_pkObjectPropertyNames = nullptr;
}

#define NID3DXLIB_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NID3DXLIB_REVERSED("NiD3DXEffectShaderLib::LoadShaderLibrary", 0x00746470, &ni::NiD3DXEffectShaderLib_LoadShaderLibrary);
NID3DXLIB_REVERSED("NiD3DXEffectShaderLibrary::SetRenderer", 0x00747EE0, &ni::NiD3DXEffectShaderLibrary_SetRenderer);
NID3DXLIB_REVERSED("NiD3DXEffectShaderLibrary::ReleaseShader(NiShader*)", 0x007480F0, &ni::NiD3DXEffectShaderLibrary_ReleaseShaderObject);
NID3DXLIB_REVERSED("NiD3DXEffectShaderLibrary::ReleaseShader", 0x007482C0, &ni::NiD3DXEffectShaderLibrary_ReleaseShader);
NID3DXLIB_REVERSED("NiD3DXEffectShaderLibrary::NiD3DXEffectShaderLibrary", 0x007484D0, &ni::NiD3DXEffectShaderLibrary_Construct);
NID3DXLIB_REVERSED("NiD3DXEffectShaderLibrary::Create", 0x00748540, &ni::NiD3DXEffectShaderLibrary_Create);
NID3DXLIB_REVERSED("NiD3DXEffect::GetD3DRenderer", 0x007486A0, &ni::NiD3DXEffect_GetD3DRenderer);
NID3DXLIB_REVERSED("NiD3DXEffectFactory::Shutdown", 0x00748BB0, &ni::NiD3DXEffectFactory_Shutdown);
NID3DXLIB_REVERSED("NiD3DXEffectLoader::~NiD3DXEffectLoader", 0x00749170, &ni::NiD3DXEffectLoader_Destruct);
NID3DXLIB_REVERSED("NiD3DXEffectLoader::Destroy", 0x00749260, &ni::NiD3DXEffectLoader_Destroy);
NID3DXLIB_REVERSED("NiD3DXEffectLoader::ProcessFile", 0x00749290, &ni::NiD3DXEffectLoader_ProcessFile);
NID3DXLIB_REVERSED("NiD3DXEffectLoader::LoadAllFiles", 0x00749370, &ni::NiD3DXEffectLoader_LoadAllFiles);
NID3DXLIB_REVERSED("NiD3DXEffectLoader::Create", 0x00749610, &ni::NiD3DXEffectLoader_Create);
NID3DXLIB_REVERSED("NiD3DXEffectLoader::FindFilesInDirectory", 0x007496A0, &ni::NiD3DXEffectLoader_FindFilesInDirectory);
NID3DXLIB_REVERSED("NiD3DXEffectLoader::FindAllFiles", 0x00749A60, &ni::NiD3DXEffectLoader_FindAllFiles);
NID3DXLIB_REVERSED("NiD3DXEffectLoader::LoadAll", 0x00749BA0, &ni::NiD3DXEffectLoader_LoadAll);
NID3DXLIB_REVERSED("NiD3DXEffectTechnique::ReleaseShader", 0x00749F00, &ni::NiD3DXEffectTechnique_ReleaseShader);
NID3DXLIB_REVERSED("NiD3DXEffectParameter::ReleaseBoneArray", 0x0074A3E0, &ni::NiD3DXEffectParameter_ReleaseBoneArray);
NID3DXLIB_REVERSED("NiD3DXEffectStateManager::SetPixelShader", 0x0074D6A0, &ni::NiD3DXEffectStateManager_SetPixelShader);
NID3DXLIB_REVERSED("NiD3DXEffectShaderLibSDM::Init", 0x0074D930, &ni::NiD3DXEffectShaderLibSDM_Init);
NID3DXLIB_REVERSED("NiD3DXEffectShaderLibSDM::Shutdown", 0x0074D950, &ni::NiD3DXEffectShaderLibSDM_Shutdown);
NID3DXLIB_REVERSED("NiD3DXEffectShaderLibSDM::NiD3DXEffectShaderLibSDM", 0x0074D980, &ni::NiD3DXEffectShaderLibSDM_Construct);
NID3DXLIB_REVERSED("NiD3DXEffectShaderLibSDM::BuildPropertyNames", 0x0074FD40, &ni::NiD3DXEffectShaderLibSDM_BuildPropertyNames);
NID3DXLIB_REVERSED("NiTPair<uint,NiFixedString>::VectorDeletingDestructor", 0x00750770, &ni::NiD3DXEffectShaderLibSDM_PropertyNameDelete);
NID3DXLIB_REVERSED("NiD3DXEffectShaderLibSDM::DestroyPropertyNames", 0x00750840, &ni::NiD3DXEffectShaderLibSDM_DestroyPropertyNames);

} // namespace ni
