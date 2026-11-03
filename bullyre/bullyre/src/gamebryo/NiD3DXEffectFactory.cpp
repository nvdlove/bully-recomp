// NiD3DXEffectShaderLibrary (vtable 0x950774, 0x3C bytes: the shader library over the effect files)
// and NiD3DXEffectFactory (vtable 0x950848, 0x14: the singleton keeping the loaded files by name,
// the device they're made on, the macros they're compiled with), the effects made from files and
// memory (D3DXCreateEffect with a device, else only compiled), and the ID3DXInclude the loading
// uses (vtable 0x950FB0: files looked for next to the including one).

#include "gamebryo/NiD3DXEffect.h"

#include <cstring>
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::d3dx;

constexpr uintptr_t Library_vtable = 0x00950774;
constexpr uintptr_t Factory_vtable = 0x00950848;
constexpr uintptr_t FileMap_vtable = 0x009508CC;          // NiTStringPointerMap<NiD3DXEffectFile*>
constexpr uintptr_t PointerMapBase_vtable = 0x00950760;
constexpr uintptr_t PointerList_vtable = 0x00952E10;
constexpr uintptr_t PointerListBase_vtable = 0x0099C748;
constexpr uintptr_t Include_vtable = 0x00950FB0;

GAME_VAR(uint32_t, NiD3DRenderer_ClassIndex, 0x020E6D20);   // (IsKindOf's bit)

// the library: NiShaderLibrary's part, its description, the files, the technique sets by name
struct NiD3DXEffectShaderLibrary {
    re::Ptr32<void> vtable;                 // +0x0
    uint32_t refCount;            // +0x4
    uint8_t base[0xC];            // +0x8 (NiShaderLibrary's)
    re::Ptr32<NiRefObject> desc;            // +0x14 (NiShaderLibraryDescPtr)
    PointerList files;            // +0x18 (NiD3DXEffectFile*)
    StringMap techniqueSets;      // +0x28 (NiTStringPointerMap<NiD3DXEffectTechniqueSet*>, vtable 0x9507B0)
};
static_assert(offsetof(NiD3DXEffectShaderLibrary, files) == 0x18);
static_assert(sizeof(NiD3DXEffectShaderLibrary) == 0x3C);

// the include handler: the directory of the file being compiled
struct NiD3DXEffectInclude {
    re::Ptr32<void> vtable;        // +0x0
    char path[0x104];    // +0x4
};
static_assert(sizeof(NiD3DXEffectInclude) == 0x108);

// NiFilename: the parts _splitpath makes
struct NiFilename {
    char dir[0x100];        // +0x0
    char drive[3];          // +0x100
    char fname[0x100];      // +0x103
    char ext[0x100];        // +0x203
    uint8_t subDir;         // +0x303
};
static_assert(sizeof(NiFilename) == 0x304);

GAME_FN(void, __cdecl, NiDelete_, 0x00752540, void* p, uint32_t size);
GAME_FN(void*, __cdecl, NiNew_, 0x00752460, uint32_t size);
GAME_FN(void*, __cdecl, NiMalloc_, 0x00752300, uint32_t size);
GAME_FN(void, __cdecl, NiFree_, 0x00752380, void* p);
GAME_FN(int, __cdecl, NiStricmp, 0x00753820, const char* a, const char* b);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, uintptr_t size, const char* format, ...);
GAME_FN(void, __cdecl, NiD3DRenderer_Error, 0x00888620, const char* format, ...);
GAME_FN(void, __cdecl, NiShaderFactory_ReportError, 0x00756700, int error, int recoverable, const char* format, ...);
GAME_FN(uint8_t, __cdecl, NiFile_Exists, 0x007533C0, const char* path, int mode);
GAME_FN(void, __cdecl, NiD3DUtility_SplitPath, 0x00888450, const char* path, char* drive, char* dir, char* fname,
        char* ext);
GAME_FN(void, __cdecl, NiD3DUtility_GetFileExtension, 0x00888490, const char* path, char* ext, uintptr_t size);
GAME_FN(void, __cdecl, NiD3DUtility_GetFileName, 0x00888680, const char* path, char* name, uint32_t size);
GAME_FN(void*, __thiscall, NiFile_Construct, 0x00752590, void* self, const char* path, int mode, uint32_t bufferSize);
GAME_FN(uint32_t, __thiscall, NiFile_Read, 0x00404F10, void* self, void* buffer, uint32_t bytes);
GAME_FN(uint8_t, __cdecl, NiPath_IsRelative, 0x00753B50, const char* path);
GAME_FN(uint32_t, __cdecl, NiPath_ConvertToAbsolute, 0x007530C0, char* out, uint32_t size, const char* relative,
        const char* base);
GAME_FN(void, __cdecl, NiPath_RemoveDotDots, 0x00752D90, char* path);
GAME_FN(void, __thiscall, NiFilename_Construct, 0x007538C0, NiFilename* self, const char* path);
GAME_FN(void, __thiscall, NiTMap_RemoveAll, 0x005AC520, void* map);
GAME_FN(void, __thiscall, NiTPointerList_RemoveAll, 0x0075DB60, PointerList* list);
GAME_FN(void*, __thiscall, NiTPointerList_RemovePos, 0x00771DA0, PointerList* list, re::Ptr32<ListNode>* position);
GAME_FN(uint8_t, __thiscall, PointerMap_GetAt, 0x00800470, StringMap* map, const char* key, re::Ptr32<void>* value);
GAME_FN(void, __thiscall, PointerMap_GetNext, 0x0083D890, StringMap* map, re::Ptr32<MapItem>* position, re::Ptr32<char>* key,
        re::Ptr32<void>* value);
GAME_FN(void, __thiscall, PointerMap_RemoveAt, 0x008A1F90, StringMap* map, const char* key);
GAME_FN(void, __thiscall, NiPointer_Assign, 0x00800420, re::Ptr32<NiRefObject>* slot, NiRefObject* value);
GAME_FN(void, __thiscall, ShaderLibrary_Destruct, 0x00889E70, void* self);
GAME_FN(NiRefObject*, __thiscall, ShaderLibraryDesc_Construct, 0x00756570, void* self);
GAME_FN(void, __thiscall, ShaderLibraryDesc_AddShaderDesc, 0x007565D0, NiRefObject* self, void* desc);
// this library's, called where modes apply
GAME_FN(void, __thiscall, PointerMap_SetAt, 0x00747F00, StringMap* map, const char* key, void* value);
GAME_FN(void, __thiscall, PointerMap_Destruct, 0x00747FC0, StringMap* map);
GAME_FN(void, __thiscall, PointerList_AddTail, 0x00748170, PointerList* list, const re::Ptr32<void>* element);
GAME_FN(void, __thiscall, Library_Destruct, 0x007481C0, NiD3DXEffectShaderLibrary* self);
GAME_FN(NiD3DXEffectFile*, __thiscall, Library_FindEffect, 0x00748130, NiD3DXEffectShaderLibrary* self,
        const char* name);
GAME_FN(NiD3DXEffectFile*, __thiscall, File_Construct, 0x00747C50, void* self);
GAME_FN(void, __thiscall, File_Destruct, 0x00747CD0, NiD3DXEffectFile* self);
GAME_FN(void, __thiscall, File_SetName, 0x007471B0, NiD3DXEffectFile* self, const char* name);
GAME_FN(void, __thiscall, File_HandleLostDevice, 0x007471D0, NiD3DXEffectFile* self);
GAME_FN(void, __thiscall, File_HandleResetDevice, 0x007471F0, NiD3DXEffectFile* self);
GAME_FN(void, __thiscall, File_DestroyRendererData, 0x007476B0, NiD3DXEffectFile* self);
GAME_FN(uint8_t, __thiscall, File_SetD3DXEffect, 0x00747E60, NiD3DXEffectFile* self, void* effect, void* device);
GAME_FN(NiD3DXEffectTechniqueSet*, __thiscall, File_GetTechniqueSet, 0x007475F0, NiD3DXEffectFile* self,
        const char* name);
GAME_FN(NiD3DXEffectTechnique*, __thiscall, File_GetFirstTechnique, 0x007479D0, NiD3DXEffectFile* self);
GAME_FN(NiD3DXEffectTechnique*, __thiscall, File_GetNextTechnique, 0x00747550, NiD3DXEffectFile* self);
GAME_FN(void*, __thiscall, File_GetFirstShaderDesc, 0x00747A20, NiD3DXEffectFile* self);
GAME_FN(void*, __thiscall, File_GetNextShaderDesc, 0x00747640, NiD3DXEffectFile* self);
GAME_FN(void*, __thiscall, TechniqueSet_GetD3DShader, 0x0074DCD0, NiD3DXEffectTechniqueSet* self, void* renderer,
        uint32_t implementation);
GAME_FN(void, __thiscall, Factory_Construct, 0x00748C20, void* self);
GAME_FN(void, __thiscall, Factory_InitMacros, 0x007486B0, NiD3DXEffectFactory* self);
GAME_FN(void, __thiscall, Factory_DestroyRendererData, 0x007488D0, NiD3DXEffectFactory* self);
GAME_FN(void, __cdecl, Factory_SetD3DDevice, 0x00748940, void* device);
GAME_FN(uint8_t, __cdecl, Factory_ResolveEffectFile, 0x00748A30, const char* name, char* path, uint32_t size);
GAME_FN(uint8_t, __thiscall, Factory_LoadEffectFromFile, 0x00748CF0, NiD3DXEffectFactory* self, const char* name,
        re::Ptr32<NiD3DXEffectFile>* file);
GAME_FN(void*, __cdecl, Effect_Compile, 0x007491C0, const void* data, uint32_t size, uint32_t flags,
        const void* defines, re::Ptr32<char>* errors, void* include);
GAME_FN(void*, __cdecl, Effect_Create, 0x007494D0, void* device, const void* data, uint32_t size, uint32_t flags,
        const void* defines, re::Ptr32<char>* errors, void* include);
GAME_FN(void*, __cdecl, Effect_LoadFile, 0x00749590, const char* path, uint32_t* size);
GAME_FN(void*, __cdecl, Effect_CompileFromFile, 0x007499C0, const char* path, uint32_t flags, const void* defines,
        re::Ptr32<char>* errors);
GAME_FN(void*, __cdecl, Effect_CreateFromFile, 0x00749AC0, void* device, const char* path, uint32_t flags,
        const void* defines, re::Ptr32<char>* errors);
GAME_FN(void, __thiscall, Include_Construct, 0x007519C0, NiD3DXEffectInclude* self, const char* path);

inline uint32_t CompileFlags() {
    return NiD3DXEffectFactory_ms_uiExtraFlags | NiD3DXEffectFactory_ms_uiCompileFlags;
}

inline NiD3DXEffectFactory* NewFactory() {
    void* memory = NiNew_(sizeof(NiD3DXEffectFactory));
    if (memory == nullptr) {
        return nullptr;
    }
    Factory_Construct(memory);
    return static_cast<NiD3DXEffectFactory*>(memory);
}

inline uint8_t IsKindOf(void* object, uint32_t classIndex) {
    const uint32_t* rtti = Virtual<const uint32_t*>(object, 2);
    return (rtti[3 + (classIndex >> 5)] & (1u << (classIndex & 31))) != 0 ? 1 : 0;
}

// the first item of a map (GetFirstPos, inlined)
inline MapItem* FirstItem(const StringMap* map) {
    for (uint32_t i = 0; i < map->hashSize; i++) {
        if (map->table[i] != nullptr) {
            return map->table[i];
        }
    }
    return nullptr;
}

} // namespace

// ---- NiD3DXEffectShaderLibrary

// 0x748130: FindEffect (by file name, ignoring case)
NiD3DXEffectFile* __fastcall NiD3DXEffectShaderLibrary_FindEffect(NiD3DXEffectShaderLibrary* self, re::Edx,
                                                                  const char* name) {
    for (ListNode* node = self->files.head; node != nullptr;) {
        NiD3DXEffectFile* file = static_cast<NiD3DXEffectFile*>(node->element);
        node = node->next;
        if (NiStricmp(file->name, name) == 0) {
            return file;
        }
    }
    return nullptr;
}

// 0x7481C0: the destructor: the files freed
void __fastcall NiD3DXEffectShaderLibrary_Destruct(NiD3DXEffectShaderLibrary* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(Library_vtable);
    re::Ptr32<ListNode> position = self->files.head;
    while (position != nullptr) {
        NiD3DXEffectFile* file = static_cast<NiD3DXEffectFile*>(NiTPointerList_RemovePos(&self->files, &position));
        if (file != nullptr) {
            File_Destruct(file);
            NiDelete_(file, sizeof(NiD3DXEffectFile));
        }
    }
    PointerMap_Destruct(&self->techniqueSets);
    self->files.vtable = reinterpret_cast<void*>(PointerList_vtable);
    NiTPointerList_RemoveAll(&self->files);
    self->files.vtable = reinterpret_cast<void*>(PointerListBase_vtable);
    ShaderLibrary_Destruct(self);
}

// 0x748240: GetShader (slot 2): for a D3D renderer (given to slot 7 first), the set of that name's
// shader for the implementation
void* __fastcall NiD3DXEffectShaderLibrary_GetShader(NiD3DXEffectShaderLibrary* self, re::Edx, void* renderer,
                                                     const char* name, uint32_t implementation) {
    if (renderer == nullptr || !IsKindOf(renderer, NiD3DRenderer_ClassIndex)) {
        return nullptr;
    }
    Virtual<void>(self, 7, renderer);
    re::Ptr32<void> set = nullptr;
    if (!PointerMap_GetAt(&self->techniqueSets, name, &set) || set == nullptr) {
        return nullptr;
    }
    return TechniqueSet_GetD3DShader(static_cast<NiD3DXEffectTechniqueSet*>(set), renderer, implementation);
}

// 0x748310: AddEffect: a file not already there added; its shader descriptions into the library's
// description (the versions they need folded in), its technique sets mapped by name
uint8_t __fastcall NiD3DXEffectShaderLibrary_AddEffect(NiD3DXEffectShaderLibrary* self, re::Edx,
                                                       NiD3DXEffectFile* file) {
    if (file == nullptr) {
        return 0;
    }
    if (Library_FindEffect(self, file->name) != nullptr) {
        NiD3DRenderer_Error("File %s already in list??\n", file->name);
        return 0;
    }
    PointerList_AddTail(&self->files, re::PtrCast<const re::Ptr32<void>*>(&file));
    if (self->desc == nullptr) {
        void* memory = NiNew_(0x54);
        NiRefObject* desc = memory != nullptr ? ShaderLibraryDesc_Construct(memory) : nullptr;
        NiPointer_Assign(&self->desc, desc);
    }
    for (void* shaderDesc = File_GetFirstShaderDesc(file); shaderDesc != nullptr;
         shaderDesc = File_GetNextShaderDesc(file)) {
        ShaderLibraryDesc_AddShaderDesc(self->desc, shaderDesc);
        // the versions: user (min, max), vertex shader (min, max), platform (min, max), the flags
        Virtual<void>(self->desc, 20, Virtual<uint32_t>(shaderDesc, 10));
        Virtual<void>(self->desc, 20, Virtual<uint32_t>(shaderDesc, 11));
        Virtual<void>(self->desc, 18, Virtual<uint32_t>(shaderDesc, 6));
        Virtual<void>(self->desc, 18, Virtual<uint32_t>(shaderDesc, 7));
        Virtual<void>(self->desc, 21, Virtual<uint32_t>(shaderDesc, 12));
        Virtual<void>(self->desc, 21, Virtual<uint32_t>(shaderDesc, 13));
        Virtual<void>(self->desc, 22, Virtual<uint32_t>(shaderDesc, 14));
    }
    for (NiD3DXEffectTechnique* technique = File_GetFirstTechnique(file); technique != nullptr;
         technique = File_GetNextTechnique(file)) {
        NiD3DXEffectTechniqueSet* set =
            File_GetTechniqueSet(file, Virtual<const char*>(technique->requirements, 2));   // GetName
        PointerMap_SetAt(&self->techniqueSets, Virtual<const char*>(technique->requirements, 2), set);
    }
    return 1;
}

// 0x7484B0: the deleting destructor (slot 0)
NiD3DXEffectShaderLibrary* __fastcall NiD3DXEffectShaderLibrary_DeletingDestructor(NiD3DXEffectShaderLibrary* self,
                                                                                   re::Edx, int flags) {
    Library_Destruct(self);
    if (flags & 1) {
        NiDelete_(self, sizeof(NiD3DXEffectShaderLibrary));
    }
    return self;
}

// ---- NiD3DXEffectFactory

// 0x7485E0: LoadEffectFromMemory: the effect made on the device (compiled with the factory's
// macros), into the file (a new one when there's none) under the name; whether it was
uint8_t __fastcall NiD3DXEffectFactory_LoadEffectFromMemory(NiD3DXEffectFactory* self, re::Edx, const char* name,
                                                           uint32_t size, const void* data,
                                                           re::Ptr32<NiD3DXEffectFile>* file) {
    re::Ptr32<char> errors = nullptr;
    void* effect = Effect_Create(NiD3DXEffectFactory_ms_pkD3DDevice, data, size, CompileFlags(), self->macros,
                                 &errors, nullptr);
    if (effect == nullptr) {
        NiShaderFactory_ReportError(1, 0, "Failed to compile D3D Effect using Temp Device:\n %s\n", errors);
        NiFree_(errors);
        *file = nullptr;
        return 0;
    }
    NiFree_(errors);
    if (*file == nullptr) {
        void* memory = NiNew_(sizeof(NiD3DXEffectFile));
        *file = memory != nullptr ? File_Construct(memory) : nullptr;
    }
    File_SetName(*file, name);
    File_SetD3DXEffect(*file, effect, NiD3DXEffectFactory_ms_pkD3DDevice);
    return 1;
}

// 0x7486B0: InitMacros: GAMEBRYO=1, _DX9=1, GAMEBRYO_USE_D3DX_FALLBACKS=1 when fallbacks are on,
// then the terminator
void __fastcall NiD3DXEffectFactory_InitMacros(NiD3DXEffectFactory* self, re::Edx) {
    re::Ptr32<const char>* macros = static_cast<re::Ptr32<const char>*>(self->macros);
    if (macros == nullptr) {
        macros = static_cast<re::Ptr32<const char>*>(NiMalloc_(0x20));
        self->macros = macros;
        macros[0] = reinterpret_cast<const char*>(0x00917600);   // "GAMEBRYO"
        macros[1] = reinterpret_cast<const char*>(0x00950844);   // "1"
        macros[2] = reinterpret_cast<const char*>(0x0095083C);   // "_DX9"
        macros[3] = reinterpret_cast<const char*>(0x009175FC);   // "1"
        macros[4] = nullptr;
        macros[5] = nullptr;
        macros[6] = nullptr;
        macros[7] = nullptr;
    }
    if (NiD3DXEffectFactory_ms_bUseFallbacks) {
        macros[4] = reinterpret_cast<const char*>(0x00950820);   // "GAMEBRYO_USE_D3DX_FALLBACKS"
        macros[5] = reinterpret_cast<const char*>(0x009175FC);
        return;
    }
    macros[4] = nullptr;
    macros[5] = nullptr;
}

// 0x748740: GetD3DXEffectFile (slot 0)
NiD3DXEffectFile* __fastcall NiD3DXEffectFactory_GetD3DXEffectFile(NiD3DXEffectFactory* self, re::Edx,
                                                                   const char* name) {
    re::Ptr32<void> file = nullptr;
    return PointerMap_GetAt(self->files, name, &file) ? static_cast<NiD3DXEffectFile*>(file) : nullptr;
}

// 0x748770: InsertD3DXEffectFileIntoMap (slot 1): under its name, when it has one
void __fastcall NiD3DXEffectFactory_InsertD3DXEffectFileIntoMap(NiD3DXEffectFactory* self, re::Edx,
                                                                NiD3DXEffectFile* file) {
    if (file == nullptr || file->name == nullptr || file->name[0] == '\0' || self->files == nullptr) {
        return;
    }
    PointerMap_SetAt(self->files, file->name, file);
}

// 0x7487B0: RemoveD3DXEffectFileFromMap (slot 2)
void __fastcall NiD3DXEffectFactory_RemoveD3DXEffectFileFromMap(NiD3DXEffectFactory* self, re::Edx, const char* name) {
    if (name != nullptr && self->files != nullptr) {
        PointerMap_RemoveAt(self->files, name);
    }
}

// 0x7487D0 / 0x748850: HandleLostDevice / HandleResetDevice: every file's, once per reset count
void __fastcall NiD3DXEffectFactory_HandleLostDevice(NiD3DXEffectFactory* self, re::Edx, uint32_t id) {
    if (self->lostDeviceID == id) {
        return;
    }
    re::Ptr32<MapItem> position = FirstItem(self->files);
    while (position != nullptr) {
        re::Ptr32<char> key;
        re::Ptr32<void> file;
        PointerMap_GetNext(self->files, &position, &key, &file);
        File_HandleLostDevice(static_cast<NiD3DXEffectFile*>(file));
    }
    self->lostDeviceID = id;
}
void __fastcall NiD3DXEffectFactory_HandleResetDevice(NiD3DXEffectFactory* self, re::Edx, uint32_t id) {
    if (self->resetDeviceID == id) {
        return;
    }
    re::Ptr32<MapItem> position = FirstItem(self->files);
    while (position != nullptr) {
        re::Ptr32<char> key;
        re::Ptr32<void> file;
        PointerMap_GetNext(self->files, &position, &key, &file);
        File_HandleResetDevice(static_cast<NiD3DXEffectFile*>(file));
    }
    self->resetDeviceID = id;
}

// 0x7488D0: DestroyRendererData (every file's)
void __fastcall NiD3DXEffectFactory_DestroyRendererData(NiD3DXEffectFactory* self, re::Edx) {
    re::Ptr32<MapItem> position = FirstItem(self->files);
    while (position != nullptr) {
        re::Ptr32<char> key;
        re::Ptr32<void> file;
        PointerMap_GetNext(self->files, &position, &key, &file);
        File_DestroyRendererData(static_cast<NiD3DXEffectFile*>(file));
    }
}

// 0x748940: SetD3DDevice (held; the files' renderer data dropped when it changes)
void __cdecl NiD3DXEffectFactory_SetD3DDevice(void* device) {
    void* old = NiD3DXEffectFactory_ms_pkD3DDevice;
    if (device == old) {
        return;
    }
    if (old != nullptr) {
        Com<unsigned long>(old, kRelease);
    }
    NiD3DXEffectFactory_ms_pkD3DDevice = device;
    if (device != nullptr) {
        Com<unsigned long>(device, 1);   // AddRef
    }
    if (NiD3DXEffectFactory_ms_pkFactory != nullptr) {
        Factory_DestroyRendererData(NiD3DXEffectFactory_ms_pkFactory);
    }
}

// 0x748980: SetD3DRenderer (its device taken; none: the device released, the renderer data dropped)
void __cdecl NiD3DXEffectFactory_SetD3DRenderer(uint8_t* renderer) {
    if (NiD3DXEffectFactory_ms_pkD3DRenderer == renderer) {
        return;
    }
    if (renderer != nullptr) {
        Factory_SetD3DDevice(At<void*>(renderer, 0xA4));
        NiD3DXEffectFactory_ms_pkD3DRenderer = renderer;
        return;
    }
    void* device = NiD3DXEffectFactory_ms_pkD3DDevice;
    if (device != nullptr) {
        Com<unsigned long>(device, kRelease);
        NiD3DXEffectFactory_ms_pkD3DDevice = nullptr;
        if (NiD3DXEffectFactory_ms_pkFactory != nullptr) {
            Factory_DestroyRendererData(NiD3DXEffectFactory_ms_pkFactory);
        }
    }
    NiD3DXEffectFactory_ms_pkD3DRenderer = renderer;
}

// 0x7489E0: the deleting destructor (slot 3)
NiD3DXEffectFactory* __fastcall NiD3DXEffectFactory_DeletingDestructor(NiD3DXEffectFactory* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(Factory_vtable);
    Factory_DestroyRendererData(self);
    if (self->files != nullptr) {
        NiTMap_RemoveAll(self->files);
    }
    if (self->files != nullptr) {
        Virtual<void>(self->files, 0, 1);
    }
    NiFree_(self->macros);
    if (flags & 1) {
        NiDelete_(self, sizeof(NiD3DXEffectFactory));
    }
    return self;
}

// 0x748A30: ResolveEffectFile: the name as it is when the file exists, else its file name in the
// effect directory; whether the file was found (reported when not)
uint8_t __cdecl NiD3DXEffectFactory_ResolveEffectFile(const char* name, char* path, uint32_t size) {
    if (NiFile_Exists(name, 0)) {
        crt::StrcpyS(path, size, name);
        return 1;
    }
    if (NiD3DXEffectFactory_ms_pkFactory == nullptr) {
        NiShaderFactory_ReportError(1, 0, "No valid D3D Effect factory\n");
        return 0;
    }
    char drive[4];
    char ext[0x100];
    char fname[0x104];
    char dir[0x104];
    NiD3DUtility_SplitPath(name, drive, dir, fname, ext);
    const char* directory = NiD3DXEffectFactory_ms_pcEffectDirectory;
    if (directory == nullptr || directory[0] == '\0') {
        NiShaderFactory_ReportError(1, 0, "Invalid or no D3D Effect directory\n");
        return 0;
    }
    size_t length = std::strlen(directory);
    const char* format = (directory[length - 1] == '/' || directory[length - 1] == '\\') ? "%s%s%s" : "%s\\%s%s";
    NiSprintf(path, size, format, directory, fname, ext);
    if (NiFile_Exists(path, 0)) {
        return 1;
    }
    NiShaderFactory_ReportError(1, 0, "D3D Effect file not found %s\n", name);
    return 0;
}

// 0x748C20: the constructor (a 59-bucket file map, the macros; no device reset seen)
NiD3DXEffectFactory* __fastcall NiD3DXEffectFactory_Construct(NiD3DXEffectFactory* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(Factory_vtable);
    self->macros = nullptr;
    self->lostDeviceID = 0xFFFFFFFF;
    self->resetDeviceID = 0xFFFFFFFF;
    StringMap* map = static_cast<StringMap*>(NiNew_(sizeof(StringMap)));
    if (map != nullptr) {
        map->vtable = reinterpret_cast<void*>(PointerMapBase_vtable);
        map->hashSize = 0x3B;
        map->count = 0;
        map->table = static_cast<re::Ptr32<MapItem>*>(NiMalloc_(0xEC));
        crt::Memset(map->table, 0, map->hashSize * 4);
        map->copyKeys = 0;
        map->vtable = reinterpret_cast<void*>(FileMap_vtable);
    }
    self->files = map;
    Factory_InitMacros(self);
    return self;
}

// 0x748CB0: GetInstance (made when asked to)
NiD3DXEffectFactory* __cdecl NiD3DXEffectFactory_GetInstance(int create) {
    if (re::Flag(create) && NiD3DXEffectFactory_ms_pkFactory == nullptr) {
        NiD3DXEffectFactory_ms_pkFactory = NewFactory();
    }
    return NiD3DXEffectFactory_ms_pkFactory;
}

// 0x748CF0: LoadEffectFromFile: .fx / .fxo (/ .fxl with fallbacks) and .fxb made on the device (the
// path resolved), into the file (a new one, named after the file, when there's none)
uint8_t __fastcall NiD3DXEffectFactory_LoadEffectFromFile(NiD3DXEffectFactory* self, re::Edx, const char* name,
                                                         re::Ptr32<NiD3DXEffectFile>* file) {
    if (name == nullptr || name[0] == '\0') {
        NiShaderFactory_ReportError(1, 0, "Missing D3D Effect file name\n");
        return 0;
    }
    char ext[0x100];
    char first[0x104];    // the resolved path of an .fxb, the file name of the others
    char second[0x104];   // the other way round
    NiD3DUtility_GetFileExtension(name, ext, 0x100);
    char* path;
    char* fileName;
    if (NiStricmp(ext, ".fx") == 0 || NiStricmp(ext, ".fxo") == 0 ||
        (NiD3DXEffectFactory_ms_bUseFallbacks && NiStricmp(ext, ".fxl") == 0)) {
        path = second;
        fileName = first;
    } else if (NiStricmp(ext, ".fxb") != 0) {
        *file = nullptr;
        NiShaderFactory_ReportError(1, 0, "Invalid D3D Effect file %s\n", name);
        return 0;
    } else {
        path = first;
        fileName = second;
    }
    if (!Factory_ResolveEffectFile(name, path, 0x104)) {
        NiShaderFactory_ReportError(1, 0, "Failed to find D3D Effect file %s\n", name);
        return 0;
    }
    re::Ptr32<char> errors = nullptr;
    void* effect = Effect_CreateFromFile(NiD3DXEffectFactory_ms_pkD3DDevice, path, CompileFlags(), self->macros,
                                         &errors);
    if (effect == nullptr) {
        NiShaderFactory_ReportError(1, 0, "Failed to compile D3D Effect using Temp Device:\n %s\n", errors);
        NiFree_(errors);
        *file = nullptr;
        return 0;
    }
    NiFree_(errors);
    if (*file == nullptr) {
        void* memory = NiNew_(sizeof(NiD3DXEffectFile));
        *file = memory != nullptr ? File_Construct(memory) : nullptr;
        NiD3DUtility_GetFileName(name, fileName, 0x104);
        File_SetName(*file, fileName);
    }
    File_SetD3DXEffect(*file, effect, NiD3DXEffectFactory_ms_pkD3DDevice);
    return 1;
}

// 0x748FF0: LoadEffect: the file of that name (by its file name) from the factory, loaded when it
// isn't (or again when it lost its effect and there's a renderer); a new one is put in the factory
NiD3DXEffectFile* __cdecl NiD3DXEffectFactory_LoadEffect(const char* name) {
    if (NiD3DXEffectFactory_ms_pkFactory == nullptr) {
        NiD3DXEffectFactory_ms_pkFactory = NewFactory();
    }
    if (name == nullptr || name[0] == '\0') {
        return nullptr;
    }
    char fileName[0x104];
    NiD3DUtility_GetFileName(name, fileName, 0x104);
    re::Ptr32<NiD3DXEffectFile> file = Virtual<NiD3DXEffectFile*>(NiD3DXEffectFactory_ms_pkFactory, 0, fileName);
    if (file != nullptr) {
        if ((file->rendererDataCreated && file->effect != nullptr) || NiD3DXEffectFactory_ms_pkD3DRenderer == nullptr) {
            return file;
        }
        Factory_LoadEffectFromFile(NiD3DXEffectFactory_ms_pkFactory, name, &file);
        return file;
    }
    if (!Factory_LoadEffectFromFile(NiD3DXEffectFactory_ms_pkFactory, name, &file)) {
        return nullptr;
    }
    file->unknown4 = 2;
    if (!file->rendererDataCreated || file->effect == nullptr) {
        File_DestroyRendererData(file);
    }
    Virtual<void>(NiD3DXEffectFactory_ms_pkFactory, 1, file);   // InsertD3DXEffectFileIntoMap
    return file;
}

// 0x749150: NiStrcat (strcat_s, the destination returned)
char* __cdecl NiD3DXEffect_NiStrcat(char* to, uint32_t size, const char* from) {
    crt::StrcatS(to, size, from);
    return to;
}

namespace {
// the error buffer's text copied out (NiMalloc'd), the buffer released
inline void TakeErrors(void* buffer, re::Ptr32<char>* errors) {
    if (buffer == nullptr) {
        return;
    }
    const char* text = Com<const char*>(buffer, 3);   // GetBufferPointer
    if (text != nullptr) {
        uint32_t size = Com<uint32_t>(buffer, 4);     // GetBufferSize
        char* copy = static_cast<char*>(NiMalloc_(size));
        *errors = copy;
        crt::StrcpyS(copy, size, text);
    }
    Com<unsigned long>(buffer, kRelease);
}
} // namespace

// 0x7491C0: CompileEffect (no device: an ID3DXEffectCompiler; the errors' text on failure)
void* __cdecl NiD3DXEffect_Compile(const void* data, uint32_t size, uint32_t flags, const void* defines,
                                   re::Ptr32<char>* errors, void* include) {
    void* buffer = nullptr;     // (both uninitialised in the game: D3DX writes them)
    void* compiler = nullptr;
    long hr = Import<long>(kD3DXCreateEffectCompiler, data, size, defines, include, flags, &compiler, &buffer);
    if (hr < 0) {
        TakeErrors(buffer, errors);
        return nullptr;
    }
    if (buffer != nullptr) {
        Com<unsigned long>(buffer, kRelease);
    }
    return compiler;
}

// 0x7494D0: CreateEffect (on the device, no pool; without one only compiled, no include)
void* __cdecl NiD3DXEffect_Create(void* device, const void* data, uint32_t size, uint32_t flags,
                                  const void* defines, re::Ptr32<char>* errors, void* include) {
    if (device == nullptr) {
        return Effect_Compile(data, size, flags, defines, errors, nullptr);
    }
    void* buffer = device;   // (the game's errors slot is the device argument's: D3DX writes it)
    void* effect = nullptr;  // (uninitialised in the game: D3DX writes it)
    long hr = Import<long>(kD3DXCreateEffect, device, data, size, defines, include, flags, static_cast<void*>(nullptr),
                           &effect, &buffer);
    if (hr < 0) {
        TakeErrors(buffer, errors);
        return nullptr;
    }
    if (buffer != nullptr) {
        Com<unsigned long>(buffer, kRelease);
    }
    return effect;
}

// 0x749590: a whole file read into an NiMalloc'd buffer (null and size 0 when it can't be opened;
// the NiFile then leaks)
void* __cdecl NiD3DXEffect_LoadFile(const char* path, uint32_t* size) {
    void* memory = NiNew_(0x2C);
    if (memory != nullptr) {
        void* file = NiFile_Construct(memory, path, 0, 0x8000);
        if (file != nullptr && Virtual<uint8_t>(file, 1)) {   // operator bool
            uint32_t bytes = Virtual<uint32_t>(file, 6);      // GetFileSize
            *size = bytes;
            void* data = nullptr;
            if (bytes != 0) {
                data = NiMalloc_(bytes);
                NiFile_Read(file, data, *size);
            }
            Virtual<void>(file, 0, 1);   // deleted
            return data;
        }
    }
    *size = 0;
    return nullptr;
}

// 0x7499C0: CompileEffectFromFile (no device; includes looked for next to the file)
void* __cdecl NiD3DXEffect_CompileFromFile(const char* path, uint32_t flags, const void* defines, re::Ptr32<char>* errors) {
    uint32_t size = 0;
    void* data = Effect_LoadFile(path, &size);
    if (data == nullptr) {
        return nullptr;
    }
    NiD3DXEffectInclude include;
    Include_Construct(&include, path);
    void* compiler = Effect_Compile(data, size, flags, defines, errors, &include);
    NiFree_(data);
    return compiler;
}

// 0x749AC0: CreateEffectFromFile (on the device; without one only compiled)
void* __cdecl NiD3DXEffect_CreateFromFile(void* device, const char* path, uint32_t flags, const void* defines,
                                          re::Ptr32<char>* errors) {
    if (device == nullptr) {
        return Effect_CompileFromFile(path, flags, defines, errors);
    }
    uint32_t size = 0;
    void* data = Effect_LoadFile(path, &size);
    if (data == nullptr) {
        return nullptr;
    }
    NiD3DXEffectInclude include;
    Include_Construct(&include, path);
    void* effect = Effect_Create(device, data, size, flags, defines, errors, &include);
    NiFree_(data);
    return effect;
}

// ---- the include handler (ID3DXInclude: 0 Open, 1 Close)

// 0x7519A0: Close (the data freed)
long __stdcall NiD3DXEffectInclude_Close(NiD3DXEffectInclude*, void* data) {
    NiFree_(data);
    return 0;
}

// 0x7519C0: the constructor: the including file's drive and directory
NiD3DXEffectInclude* __fastcall NiD3DXEffectInclude_Construct(NiD3DXEffectInclude* self, re::Edx, const char* path) {
    NiFilename filename;
    self->vtable = reinterpret_cast<void*>(Include_vtable);
    NiFilename_Construct(&filename, path);
    crt::StrcpyS(self->path, 0x104, filename.drive);
    crt::StrcatS(self->path, 0x104, filename.dir);
    return self;
}

// 0x751A40: Open: a relative name made absolute from the including file's directory, read whole
// (S_OK even when it isn't there: no data)
long __stdcall NiD3DXEffectInclude_Open(NiD3DXEffectInclude* self, uint32_t, const char* name, const void*,
                                        re::Ptr32<void>* data, uint32_t* bytes) {
    char path[0x104];
    if (NiPath_IsRelative(name)) {
        NiPath_ConvertToAbsolute(path, 0x104, name, self->path);
    } else {
        crt::StrcpyS(path, 0x104, name);
    }
    NiPath_RemoveDotDots(path);
    uint32_t size = 0;
    void* buffer = Effect_LoadFile(path, &size);
    *data = buffer;
    *bytes = size;
    return 0;
}

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NID3DXFACTORY_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                                 \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NID3DXFACTORY_REVERSED("NiD3DXEffectShaderLibrary::FindEffect", 0x00748130, &ni::NiD3DXEffectShaderLibrary_FindEffect);
NID3DXFACTORY_REVERSED("NiD3DXEffectShaderLibrary::~NiD3DXEffectShaderLibrary", 0x007481C0, &ni::NiD3DXEffectShaderLibrary_Destruct);
NID3DXFACTORY_REVERSED("NiD3DXEffectShaderLibrary::GetShader", 0x00748240, &ni::NiD3DXEffectShaderLibrary_GetShader);
NID3DXFACTORY_REVERSED("NiD3DXEffectShaderLibrary::AddEffect", 0x00748310, &ni::NiD3DXEffectShaderLibrary_AddEffect);
NID3DXFACTORY_REVERSED("NiD3DXEffectShaderLibrary::DeletingDestructor", 0x007484B0, &ni::NiD3DXEffectShaderLibrary_DeletingDestructor);
NID3DXFACTORY_REVERSED("NiD3DXEffectFactory::LoadEffectFromMemory", 0x007485E0, &ni::NiD3DXEffectFactory_LoadEffectFromMemory);
NID3DXFACTORY_REVERSED("NiD3DXEffectFactory::InitMacros", 0x007486B0, &ni::NiD3DXEffectFactory_InitMacros);
NID3DXFACTORY_REVERSED("NiD3DXEffectFactory::GetD3DXEffectFile", 0x00748740, &ni::NiD3DXEffectFactory_GetD3DXEffectFile);
NID3DXFACTORY_REVERSED("NiD3DXEffectFactory::InsertD3DXEffectFileIntoMap", 0x00748770, &ni::NiD3DXEffectFactory_InsertD3DXEffectFileIntoMap);
NID3DXFACTORY_REVERSED("NiD3DXEffectFactory::RemoveD3DXEffectFileFromMap", 0x007487B0, &ni::NiD3DXEffectFactory_RemoveD3DXEffectFileFromMap);
NID3DXFACTORY_REVERSED("NiD3DXEffectFactory::HandleLostDevice", 0x007487D0, &ni::NiD3DXEffectFactory_HandleLostDevice);
NID3DXFACTORY_REVERSED("NiD3DXEffectFactory::HandleResetDevice", 0x00748850, &ni::NiD3DXEffectFactory_HandleResetDevice);
NID3DXFACTORY_REVERSED("NiD3DXEffectFactory::DestroyRendererData", 0x007488D0, &ni::NiD3DXEffectFactory_DestroyRendererData);
NID3DXFACTORY_REVERSED("NiD3DXEffectFactory::SetD3DDevice", 0x00748940, &ni::NiD3DXEffectFactory_SetD3DDevice);
NID3DXFACTORY_REVERSED("NiD3DXEffectFactory::SetD3DRenderer", 0x00748980, &ni::NiD3DXEffectFactory_SetD3DRenderer);
NID3DXFACTORY_REVERSED("NiD3DXEffectFactory::DeletingDestructor", 0x007489E0, &ni::NiD3DXEffectFactory_DeletingDestructor);
NID3DXFACTORY_REVERSED("NiD3DXEffectFactory::ResolveEffectFile", 0x00748A30, &ni::NiD3DXEffectFactory_ResolveEffectFile);
NID3DXFACTORY_REVERSED("NiD3DXEffectFactory::NiD3DXEffectFactory", 0x00748C20, &ni::NiD3DXEffectFactory_Construct);
NID3DXFACTORY_REVERSED("NiD3DXEffectFactory::GetInstance", 0x00748CB0, &ni::NiD3DXEffectFactory_GetInstance);
NID3DXFACTORY_REVERSED("NiD3DXEffectFactory::LoadEffectFromFile", 0x00748CF0, &ni::NiD3DXEffectFactory_LoadEffectFromFile);
NID3DXFACTORY_REVERSED("NiD3DXEffectFactory::LoadEffect", 0x00748FF0, &ni::NiD3DXEffectFactory_LoadEffect);
NID3DXFACTORY_REVERSED("NiD3DXEffect::NiStrcat", 0x00749150, &ni::NiD3DXEffect_NiStrcat);
NID3DXFACTORY_REVERSED("NiD3DXEffect::Compile", 0x007491C0, &ni::NiD3DXEffect_Compile);
NID3DXFACTORY_REVERSED("NiD3DXEffect::Create", 0x007494D0, &ni::NiD3DXEffect_Create);
NID3DXFACTORY_REVERSED("NiD3DXEffect::LoadFile", 0x00749590, &ni::NiD3DXEffect_LoadFile);
NID3DXFACTORY_REVERSED("NiD3DXEffect::CompileFromFile", 0x007499C0, &ni::NiD3DXEffect_CompileFromFile);
NID3DXFACTORY_REVERSED("NiD3DXEffect::CreateFromFile", 0x00749AC0, &ni::NiD3DXEffect_CreateFromFile);
NID3DXFACTORY_REVERSED("NiD3DXEffectInclude::Close", 0x007519A0, &ni::NiD3DXEffectInclude_Close);
NID3DXFACTORY_REVERSED("NiD3DXEffectInclude::NiD3DXEffectInclude", 0x007519C0, &ni::NiD3DXEffectInclude_Construct);
NID3DXFACTORY_REVERSED("NiD3DXEffectInclude::Open", 0x00751A40, &ni::NiD3DXEffectInclude_Open);
