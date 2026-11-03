// NiD3DShaderProgramFactory (vtable 0x99DCF8, an NiRefObject; 0x20 bytes; the instance at
// 0x20E7484): the D3D shader programs by name and the directories their files are looked for in.
// +0x8 the directories (an NiTPointerList<char*>, vtable 0x952E10: +0xC head, +0x10 tail, +0x14
// count; the strings NiMalloc'd), +0x18 / +0x1C the vertex / pixel shaders by name
// (NiTStringPointerMap, vtable 0x951798, 59 buckets, the keys not copied). The shader program
// creators are kept by file extension in a static map (0x20E7488: vtable 0x9508CC, 37 buckets, the
// keys copied).
//
// Also NiD3DShaderProgram's statics (0x20E7490 the renderer, 0x20E748C its device (held),
// 0x20E7494 its render state) and the string map / pointer list template pieces in this range
// (RemoveAt 0x8A1F90 is NiTStringMap's for every element type; 0x8A2080 NiTMapBase's SetValue;
// 0x8A23C0 NiTPointerListBase's NewItem).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::c41 {
using namespace ni::dx9;
namespace {

constexpr uintptr_t kProgramFactoryVtable = 0x0099DCF8;
constexpr uintptr_t kNiRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t kDirectoryListVtable = 0x00952E10;   // NiTPointerList<char*>
constexpr uintptr_t kAllocatorVtable = 0x0099C748;
constexpr uintptr_t kMapBaseVtable = 0x00950760;
constexpr uintptr_t kProgramMapVtable = 0x00951798;
constexpr uintptr_t kCreatorMapVtable = 0x009508CC;
constexpr uint32_t kProgramFactorySize = 0x20;

GAME_VAR(uint8_t*, ms_pkProgramFactory, 0x020E7484);
GAME_VAR(uint8_t*, ms_pkCreatorMap, 0x020E7488);
GAME_VAR(void*, ms_pkProgramDevice, 0x020E748C);
GAME_VAR(uint8_t*, ms_pkProgramRenderer, 0x020E7490);
GAME_VAR(uint8_t*, ms_pkProgramRenderState, 0x020E7494);
GAME_VAR(long, NiRefObject_ms_uiObjects, 0x020E2168);

GAME_FN(uint32_t, __cdecl, StringHash, 0x005F09F0, const char* key, uint32_t hashSize);
GAME_FN(void, __thiscall, Map_RemoveAll, 0x005AC520, void* map);
GAME_FN(void, __thiscall, Map_GetNext, 0x0083D890, void* map, re::Ptr32<void>* position, re::Ptr32<const char>* key, re::Ptr32<void>* value);
GAME_FN(void, __thiscall, StringMap_SetAt, 0x00747F00, void* map, const char* key, void* value);
GAME_FN(uint8_t, __thiscall, StringMap_GetAt, 0x00800470, void* map, const char* key, re::Ptr32<void>* value);
GAME_FN(void, __thiscall, PointerList_RemoveAll, 0x0075DB60, void* list);
GAME_FN(void, __thiscall, PointerList_Remove, 0x0075F630, void* list, const re::Ptr32<char>* element);
GAME_FN(void*, __thiscall, PointerList_RemovePos, 0x00771DA0, void* list, re::Ptr32<void>* position);
GAME_FN(void*, __thiscall, PointerAllocator_Allocate, 0x004028F0, void* self);
GAME_FN(uint8_t, __cdecl, FileExists, 0x007533C0, const char* path, int mode);
GAME_FN(uint8_t, __cdecl, D3DUtility_GetSplitPath, 0x00888450, const char* path, char* drive, char* directory,
        char* name, char* extension);
GAME_FN(uint8_t, __cdecl, D3DUtility_GetFileExtension, 0x00888490, const char* path, char* extension, size_t size);
GAME_FN(uint32_t, __cdecl, ShaderFactory_ReportError, 0x00756700, int result, int recoverable, const char* format,
        ...);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);

// this file's (called where modes apply)
GAME_FN(uint8_t, __thiscall, StringMap_RemoveAt, 0x008A1F90, void* map, const char* key);
GAME_FN(void*, __thiscall, ProgramFactory_GetVertexShader, 0x008A2160, void* self, const char* name);
GAME_FN(void*, __thiscall, ProgramFactory_GetPixelShader, 0x008A2190, void* self, const char* name);
GAME_FN(void, __thiscall, ProgramFactory_InsertVertexShader, 0x008A21C0, void* self, void* program);
GAME_FN(void, __thiscall, ProgramFactory_InsertPixelShader, 0x008A21F0, void* self, void* program);
GAME_FN(void*, __cdecl, ProgramFactory_GetShaderCreator, 0x008A22B0, const char* fileName);
GAME_FN(void, __thiscall, ProgramFactory_RemoveAllDirectories, 0x008A2770, void* self);
GAME_FN(void, __cdecl, ProgramFactory_CreateCreatorMapImpl, 0x008A2AF0);
GAME_FN(void, __thiscall, ProgramFactory_Destruct, 0x008A2A70, void* self);

inline uint8_t* NewProgramMap() {
    uint8_t* map = static_cast<uint8_t*>(NiNew(0x14));
    if (map == nullptr) {
        return nullptr;
    }
    At<uint32_t>(map, 0) = kMapBaseVtable;
    At<uint32_t>(map, 4) = 0x3B;
    At<uint32_t>(map, 0xC) = 0;
    At<void*>(map, 8) = NiMalloc(0xEC);
    crt::Memset(At<void*>(map, 8), 0, At<uint32_t>(map, 4) * 4);
    At<uint8_t>(map, 0x10) = 0;
    At<uint32_t>(map, 0) = kProgramMapVtable;
    return map;
}

// a directory with its slashes made backslashes (the length counted again each step)
inline void NormalizeDirectory(char* buffer, const char* directory) {
    crt::StrcpyS(buffer, 0x104, directory);
    for (uint32_t i = 0; i < uint32_t(crt::Strcspn(buffer, "")); i++) {
        if (buffer[i] == '/') {
            buffer[i] = '\\';
        }
    }
}

inline size_t Length(const char* s) {
    return crt::Strcspn(s, "");
}

} // namespace

// 0x8A1EE0: NiD3DShaderProgramFactory::GetInstance
uint8_t* __cdecl ProgramFactory_GetInstance() {
    return ms_pkProgramFactory;
}

// 0x8A1EF0: NiD3DShaderProgramFactory::Shutdown (the instance deleted, slot 0)
void __cdecl ProgramFactory_Shutdown() {
    if (ms_pkProgramFactory != nullptr) {
        Virtual<void>(ms_pkProgramFactory, 0, 1u);
    }
    ms_pkProgramFactory = nullptr;
}

// 0x8A1F10: NiD3DShaderProgramFactory::RecreateVertexShader (true when it has its shader (slot
// 18); else by its creator (slot 12), the creator's slot 7)
uint8_t __cdecl ProgramFactory_RecreateVertexShader(void* program) {
    if (ms_pkProgramFactory == nullptr || program == nullptr) {
        return 0;
    }
    if (Virtual<void*>(program, 18) != nullptr) {
        return 1;
    }
    void* creator = Virtual<void*>(program, 12);
    if (creator == nullptr) {
        return 0;
    }
    return Virtual<uint8_t>(creator, 7, program);
}

// 0x8A1F50: NiD3DShaderProgramFactory::RecreatePixelShader (slot 16; the creator's slot 8)
uint8_t __cdecl ProgramFactory_RecreatePixelShader(void* program) {
    if (ms_pkProgramFactory == nullptr || program == nullptr) {
        return 0;
    }
    if (Virtual<void*>(program, 16) != nullptr) {
        return 1;
    }
    void* creator = Virtual<void*>(program, 12);
    if (creator == nullptr) {
        return 0;
    }
    return Virtual<uint8_t>(creator, 8, program);
}

// 0x8A1F90 (thiscall): NiTStringMap<...>::RemoveAt (the item by its key: its value cleared (slot
// 2), freed (slot 4)); whether it was there
uint8_t __fastcall StringMap_RemoveAtImpl(uint8_t* self, re::Edx, const char* key) {
    uint32_t bucket = StringHash(key, At<uint32_t>(self, 4));
    re::Ptr32<uint8_t>* slot = &At<re::Ptr32<uint8_t>*>(self, 8)[bucket];
    uint8_t* item = *slot;
    if (item == nullptr) {
        return 0;
    }
    if (crt::Strcmp(key, At<const char*>(item, 4)) == 0) {
        *slot = At<uint8_t*>(item, 0);
        Virtual<void>(self, 2, item);
        Virtual<void>(self, 4, item);
        At<uint32_t>(self, 0xC)--;
        return 1;
    }
    for (uint8_t* previous = item; (item = At<uint8_t*>(previous, 0)) != nullptr; previous = item) {
        if (crt::Strcmp(key, At<const char*>(item, 4)) == 0) {
            At<uint8_t*>(previous, 0) = At<uint8_t*>(item, 0);
            Virtual<void>(self, 2, item);
            Virtual<void>(self, 4, item);
            At<uint32_t>(self, 0xC)--;
            return 1;
        }
    }
    return 0;
}

// 0x8A2080 (thiscall, the map unused): NiTMapBase<...>::SetValue (an item's key and value)
void __fastcall MapBase_SetValue(void*, re::Edx, uint8_t* item, void* key, void* value) {
    At<void*>(item, 4) = key;
    At<void*>(item, 8) = value;
}

// 0x8A20A0: a lone `ret 4` (NiObject's empty slots and many others): too short to hook

// 0x8A20B0: NiD3DShaderProgram's static SetD3DRenderer (the renderer, its device held, its render
// state; 0x8A7000 is the member one)
void __cdecl Program_SetD3DRenderer(uint8_t* renderer) {
    ms_pkProgramRenderer = renderer;
    if (renderer != nullptr) {
        void* device = At<void*>(renderer, kDevice);
        if (ms_pkProgramDevice != nullptr) {
            Com<long>(ms_pkProgramDevice, kRelease);
        }
        ms_pkProgramDevice = device;
        if (device != nullptr) {
            Com<long>(device, kAddRef);
        }
        ms_pkProgramRenderState = At<uint8_t*>(ms_pkProgramRenderer, kRenderState);
        return;
    }
    ms_pkProgramRenderState = nullptr;
    if (ms_pkProgramDevice != nullptr) {
        Com<long>(ms_pkProgramDevice, kRelease);
    }
    ms_pkProgramDevice = nullptr;
}

// 0x8A2120 (thiscall): NiD3DShaderProgramFactory::GetFirstProgramDirectory
const char* __fastcall ProgramFactory_GetFirstDirectory(uint8_t* self, re::Edx, re::Ptr32<uint8_t>* position) {
    uint8_t* node = At<uint8_t*>(self, 0xC);
    *position = node;
    if (node != nullptr) {
        *position = At<uint8_t*>(node, 0);
        return At<const char*>(node, 8);
    }
    return nullptr;
}

// 0x8A2140 (thiscall, `this` unused): NiD3DShaderProgramFactory::GetNextProgramDirectory
const char* __fastcall ProgramFactory_GetNextDirectory(void*, re::Edx, re::Ptr32<uint8_t>* position) {
    uint8_t* node = *position;
    if (node != nullptr) {
        *position = At<uint8_t*>(node, 0);
        return At<const char*>(node, 8);
    }
    return nullptr;
}

// 0x8A2160 / 0x8A2190 (thiscall): the vertex / pixel shader by name (null when there's none)
void* __fastcall ProgramFactory_GetVertexShaderImpl(uint8_t* self, re::Edx, const char* name) {
    if (At<void*>(self, 0x18) == nullptr) {
        return nullptr;
    }
    re::Ptr32<void> program = nullptr;
    return StringMap_GetAt(At<void*>(self, 0x18), name, &program) != 0 ? program : nullptr;
}
void* __fastcall ProgramFactory_GetPixelShaderImpl(uint8_t* self, re::Edx, const char* name) {
    if (At<void*>(self, 0x1C) == nullptr) {
        return nullptr;
    }
    re::Ptr32<void> program = nullptr;
    return StringMap_GetAt(At<void*>(self, 0x1C), name, &program) != 0 ? program : nullptr;
}

// 0x8A21C0 / 0x8A21F0 (thiscall): a vertex / pixel shader kept by its name (slot 5)
void __fastcall ProgramFactory_InsertVertexShaderImpl(uint8_t* self, re::Edx, void* program) {
    if (program != nullptr && At<void*>(self, 0x18) != nullptr) {
        StringMap_SetAt(At<void*>(self, 0x18), Virtual<const char*>(program, 5), program);
    }
}
void __fastcall ProgramFactory_InsertPixelShaderImpl(uint8_t* self, re::Edx, void* program) {
    if (program != nullptr && At<void*>(self, 0x1C) != nullptr) {
        StringMap_SetAt(At<void*>(self, 0x1C), Virtual<const char*>(program, 5), program);
    }
}

// 0x8A2220: NiD3DShaderProgramFactory::UnregisterShaderCreator (every extension it's kept under
// removed)
void __cdecl ProgramFactory_UnregisterShaderCreator(void* creator) {
    uint8_t* map = ms_pkCreatorMap;
    if (map == nullptr) {
        return;
    }
    re::Ptr32<void> position = nullptr;
    for (uint32_t i = 0; i < At<uint32_t>(map, 4); i++) {
        if (At<re::Ptr32<void>*>(map, 8)[i] != nullptr) {
            position = At<re::Ptr32<void>*>(map, 8)[i];
            break;
        }
    }
    while (position != nullptr) {
        re::Ptr32<const char> key = nullptr;
        re::Ptr32<void> value = nullptr;
        Map_GetNext(map, &position, &key, &value);
        if (creator == value) {
            StringMap_RemoveAt(map, key);
            map = ms_pkCreatorMap;
        }
    }
}

// 0x8A22B0: NiD3DShaderProgramFactory::GetShaderCreator (by the file's extension, lowercased;
// the whole name when it has none)
void* __cdecl ProgramFactory_GetShaderCreatorImpl(const char* fileName) {
    char extension[0x100];
    if (ms_pkCreatorMap == nullptr || fileName == nullptr || *fileName == '\0') {
        return nullptr;
    }
    D3DUtility_GetFileExtension(fileName, extension, 0x100);
    const char* from = extension[1] != '\0' ? extension + 1 : fileName;
    for (uint32_t i = 0; i < 0x100; i++) {
        char c = char(crt::Tolower(from[i]));
        extension[i] = c;
        if (c == '\0') {
            break;
        }
    }
    re::Ptr32<void> creator = nullptr;
    return StringMap_GetAt(ms_pkCreatorMap, extension, &creator) != 0 ? creator : nullptr;
}

// 0x8A2390: NiD3DShaderProgramFactory::DestroyCreatorMap
void __cdecl ProgramFactory_DestroyCreatorMap() {
    Map_RemoveAll(ms_pkCreatorMap);
    if (ms_pkCreatorMap != nullptr) {
        Virtual<void>(ms_pkCreatorMap, 0, 1u);
    }
    ms_pkCreatorMap = nullptr;
}

// 0x8A23C0 (thiscall): NiTPointerListBase<...>::NewItem (its allocator's, at +0xC)
void* __fastcall PointerListBase_NewItem(uint8_t* self, re::Edx) {
    return PointerAllocator_Allocate(self + 0xC);
}

// 0x8A23D0: NiD3DShaderProgramFactory::CreateVertexShaderFromBuffer (the one kept under the shader
// name, else made from the buffer by the creator for the type's extension (its slot 4) and kept)
void* __cdecl ProgramFactory_CreateVertexShaderFromBuffer(const void* buffer, uint32_t size, const char* type,
                                                          const char* shaderName, void* a5, void* a6, void* a7,
                                                          void* a8, void* a9, void* a10) {
    if (ms_pkProgramFactory == nullptr) {
        return nullptr;
    }
    if (buffer == nullptr || size == 0) {
        return nullptr;
    }
    void* program = ProgramFactory_GetVertexShader(ms_pkProgramFactory, shaderName);
    if (program != nullptr) {
        return program;
    }
    void* creator = ProgramFactory_GetShaderCreator(type);
    if (creator == nullptr) {
        return nullptr;
    }
    program = Virtual<void*>(creator, 4, buffer, size, shaderName, a5, a6, a7, a8, a9, a10);
    ProgramFactory_InsertVertexShader(ms_pkProgramFactory, program);
    return program;
}

// 0x8A2460: NiD3DShaderProgramFactory::CreatePixelShaderFromBuffer (the creator's slot 6)
void* __cdecl ProgramFactory_CreatePixelShaderFromBuffer(const void* buffer, uint32_t size, const char* type,
                                                         const char* shaderName, void* a5, void* a6, void* a7) {
    if (ms_pkProgramFactory == nullptr) {
        return nullptr;
    }
    if (buffer == nullptr || size == 0) {
        return nullptr;
    }
    void* program = ProgramFactory_GetPixelShader(ms_pkProgramFactory, shaderName);
    if (program != nullptr) {
        return program;
    }
    void* creator = ProgramFactory_GetShaderCreator(type);
    if (creator == nullptr) {
        return nullptr;
    }
    program = Virtual<void*>(creator, 6, buffer, size, shaderName, a5, a6, a7);
    ProgramFactory_InsertPixelShader(ms_pkProgramFactory, program);
    return program;
}

// 0x8A24E0 / 0x8A2510: NiD3DShaderProgramFactory::ReleaseVertexShader / ReleasePixelShader (its
// name taken out of the map)
void __cdecl ProgramFactory_ReleaseVertexShader(void* program) {
    if (ms_pkProgramFactory == nullptr || program == nullptr) {
        return;
    }
    const char* name = Virtual<const char*>(program, 5);
    if (name != nullptr && At<void*>(ms_pkProgramFactory, 0x18) != nullptr) {
        StringMap_RemoveAt(At<void*>(ms_pkProgramFactory, 0x18), name);
    }
}
void __cdecl ProgramFactory_ReleasePixelShader(void* program) {
    if (ms_pkProgramFactory == nullptr || program == nullptr) {
        return;
    }
    const char* name = Virtual<const char*>(program, 5);
    if (name != nullptr && At<void*>(ms_pkProgramFactory, 0x1C) != nullptr) {
        StringMap_RemoveAt(At<void*>(ms_pkProgramFactory, 0x1C), name);
    }
}

// 0x8A2540 (thiscall): NiD3DShaderProgramFactory::AddProgramDirectory (with backslashes; a copy
// added at the head unless it's there, case aside)
void __fastcall ProgramFactory_AddDirectory(uint8_t* self, re::Edx, const char* directory) {
    char buffer[0x104];
    if (directory == nullptr || *directory == '\0') {
        return;
    }
    NormalizeDirectory(buffer, directory);
    for (uint8_t* node = At<uint8_t*>(self, 0xC); node != nullptr; node = At<uint8_t*>(node, 0)) {
        const char* existing = At<const char*>(node, 8);
        if (existing != nullptr && crt::Stricmp(existing, buffer) == 0) {
            return;
        }
    }
    size_t size = Length(buffer) + 1;
    char* copy = static_cast<char*>(NiMalloc(uint32_t(size)));
    crt::StrcpyS(copy, size, buffer);
    uint8_t* node = Virtual<uint8_t*>(self + 8, 1);
    At<char*>(node, 8) = copy;
    At<uint8_t*>(node, 4) = nullptr;
    At<uint8_t*>(node, 0) = At<uint8_t*>(self, 0xC);
    if (At<uint8_t*>(self, 0xC) == nullptr) {
        At<uint8_t*>(self, 0x10) = node;
    } else {
        At<uint8_t*>(At<uint8_t*>(self, 0xC), 4) = node;
    }
    At<uint32_t>(self, 0x14)++;
    At<uint8_t*>(self, 0xC) = node;
}

// 0x8A2680 (thiscall): NiD3DShaderProgramFactory::RemoveProgramDirectory (its node removed; the
// string not freed)
void __fastcall ProgramFactory_RemoveDirectory(uint8_t* self, re::Edx, const char* directory) {
    char buffer[0x104];
    if (directory == nullptr || *directory == '\0') {
        return;
    }
    NormalizeDirectory(buffer, directory);
    for (uint8_t* node = At<uint8_t*>(self, 0xC); node != nullptr;) {
        re::Ptr32<char> existing = At<char*>(node, 8);
        node = At<uint8_t*>(node, 0);
        if (existing != nullptr && crt::Stricmp(existing, buffer) == 0) {
            PointerList_Remove(self + 8, &existing);
            return;
        }
    }
}

// 0x8A2770 (thiscall): NiD3DShaderProgramFactory::RemoveAllProgramDirectories (each node removed
// and its string freed)
void __fastcall ProgramFactory_RemoveAllDirectoriesImpl(uint8_t* self, re::Edx) {
    uint8_t* node = At<uint8_t*>(self, 0xC);
    for (;;) {
        char* directory;
        do {
            if (node == nullptr) {
                return;
            }
            directory = At<char*>(node, 8);
            node = At<uint8_t*>(node, 0);
        } while (directory == nullptr);
        re::Ptr32<void> position = At<uint8_t*>(self, 0xC);
        while (position != nullptr && At<char*>(position, 8) != directory) {
            position = At<uint8_t*>(position, 0);
        }
        if (position != nullptr) {
            PointerList_RemovePos(self + 8, &position);
        }
        NiFree(directory);
    }
}

// 0x8A27D0: NiD3DShaderProgramFactory::ResolveProgramFileName (the name itself when the file is
// there, else each program directory with the name's file name and extension; reported when not
// found); whether one was
uint8_t __cdecl ProgramFactory_ResolveProgramFileName(const char* name, char* out, size_t size) {
    char drive[4];
    char extension[0x100];
    char fileName[0x104];
    char directory[0x104];
    if (FileExists(name, 0) != 0) {
        crt::StrcpyS(out, size, name);
        return 1;
    }
    if (ms_pkProgramFactory == nullptr) {
        ShaderFactory_ReportError(1, 0, "No valid shader program factory\n");
        return 0;
    }
    D3DUtility_GetSplitPath(name, drive, directory, fileName, extension);
    uint8_t* node = At<uint8_t*>(ms_pkProgramFactory, 0xC);
    if (node == nullptr) {
        return 0;
    }
    const char* path = At<const char*>(node, 8);
    node = At<uint8_t*>(node, 0);
    while (path != nullptr) {
        if (*path == '\0') {
            ShaderFactory_ReportError(1, 0, "Invalid or no shader program directory\n");
        } else {
            char last = path[Length(path) - 1];
            const char* format = (last == '/' || last == '\\') ? "%s%s%s" : "%s\\%s%s";
            NiSprintf(out, size, format, path, fileName, extension);
            if (FileExists(out, 0) != 0) {
                return 1;
            }
            ShaderFactory_ReportError(1, 1, "Shader program file not found %s in directory %s\n", name, path);
        }
        if (node == nullptr) {
            return 0;
        }
        path = At<const char*>(node, 8);
        node = At<uint8_t*>(node, 0);
    }
    return 0;
}

// 0x8A2990 (thiscall): NiD3DShaderProgramFactory::NiD3DShaderProgramFactory
uint8_t* __fastcall ProgramFactory_Construct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kNiRefObjectVtable;
    At<uint32_t>(self, 4) = 0;
    Increment(&NiRefObject_ms_uiObjects);
    At<uint32_t>(self, 0) = kProgramFactoryVtable;
    At<uint32_t>(self, 0x14) = 0;
    At<void*>(self, 0xC) = nullptr;
    At<void*>(self, 0x10) = nullptr;
    At<uint32_t>(self, 8) = kDirectoryListVtable;
    At<uint8_t*>(self, 0x18) = NewProgramMap();
    At<uint8_t*>(self, 0x1C) = NewProgramMap();
    return self;
}

// 0x8A2A70 (thiscall): NiD3DShaderProgramFactory::~NiD3DShaderProgramFactory
void __fastcall ProgramFactory_DestructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kProgramFactoryVtable;
    if (At<void*>(self, 0x18) != nullptr) {
        Map_RemoveAll(At<void*>(self, 0x18));
    }
    if (At<void*>(self, 0x18) != nullptr) {
        Virtual<void>(At<void*>(self, 0x18), 0, 1u);
    }
    if (At<void*>(self, 0x1C) != nullptr) {
        Map_RemoveAll(At<void*>(self, 0x1C));
    }
    if (At<void*>(self, 0x1C) != nullptr) {
        Virtual<void>(At<void*>(self, 0x1C), 0, 1u);
    }
    ProgramFactory_RemoveAllDirectories(self);
    At<uint32_t>(self, 8) = kDirectoryListVtable;
    PointerList_RemoveAll(self + 8);
    At<uint32_t>(self, 8) = kAllocatorVtable;
    At<uint32_t>(self, 0) = kNiRefObjectVtable;
    Decrement(&NiRefObject_ms_uiObjects);
}

// 0x8A2AF0: the creator map made
void __cdecl ProgramFactory_CreateCreatorMapImplFn() {
    uint8_t* map = static_cast<uint8_t*>(NiNew(0x14));
    if (map == nullptr) {
        ms_pkCreatorMap = nullptr;
        return;
    }
    At<uint32_t>(map, 0) = kMapBaseVtable;
    At<uint32_t>(map, 4) = 0x25;
    At<uint32_t>(map, 0xC) = 0;
    At<void*>(map, 8) = NiMalloc(0x94);
    crt::Memset(At<void*>(map, 8), 0, At<uint32_t>(map, 4) * 4);
    At<uint8_t>(map, 0x10) = 1;
    At<uint32_t>(map, 0) = kCreatorMapVtable;
    ms_pkCreatorMap = map;
}

// 0x8A2B60: the creator map made when there's none
void __cdecl ProgramFactory_CreateCreatorMap() {
    if (ms_pkCreatorMap == nullptr) {
        ProgramFactory_CreateCreatorMapImpl();
    }
}

// 0x8A2B70 (thiscall, slot 0): the deleting destructor
uint8_t* __fastcall ProgramFactory_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    ProgramFactory_Destruct(self);
    if (flags & 1) {
        NiDelete(self, kProgramFactorySize);
    }
    return self;
}

// 0x8A2B90: NiD3DShaderProgramFactory::RegisterShaderCreator (by extension, lowercased; not one
// with a '.'); whether it was
uint8_t __cdecl ProgramFactory_RegisterShaderCreator(const char* extension, void* creator) {
    char lowered[0x100];
    if (extension == nullptr || crt::Strchr(extension, '.') != nullptr) {
        return 0;
    }
    for (uint32_t i = 0; i < 0x100; i++) {
        char c = extension[i];
        lowered[i] = char(crt::Tolower(c));
        if (c == '\0') {
            break;
        }
    }
    if (ms_pkCreatorMap == nullptr) {
        ProgramFactory_CreateCreatorMapImpl();
    }
    StringMap_SetAt(ms_pkCreatorMap, lowered, creator);
    return 1;
}

#define C41G_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C41G_REVERSED("NiD3DShaderProgramFactory::GetInstance", 0x008A1EE0, ProgramFactory_GetInstance);
C41G_REVERSED("NiD3DShaderProgramFactory::Shutdown", 0x008A1EF0, ProgramFactory_Shutdown);
C41G_REVERSED("NiD3DShaderProgramFactory::RecreateVertexShader", 0x008A1F10, ProgramFactory_RecreateVertexShader);
C41G_REVERSED("NiD3DShaderProgramFactory::RecreatePixelShader", 0x008A1F50, ProgramFactory_RecreatePixelShader);
C41G_REVERSED("NiTStringMap::RemoveAt", 0x008A1F90, StringMap_RemoveAtImpl);
C41G_REVERSED("NiTMapBase::SetValue", 0x008A2080, MapBase_SetValue);
C41G_REVERSED("NiD3DShaderProgram::SetStaticD3DRenderer", 0x008A20B0, Program_SetD3DRenderer);
C41G_REVERSED("NiD3DShaderProgramFactory::GetFirstProgramDirectory", 0x008A2120, ProgramFactory_GetFirstDirectory);
C41G_REVERSED("NiD3DShaderProgramFactory::GetNextProgramDirectory", 0x008A2140, ProgramFactory_GetNextDirectory);
C41G_REVERSED("NiD3DShaderProgramFactory::GetVertexShader", 0x008A2160, ProgramFactory_GetVertexShaderImpl);
C41G_REVERSED("NiD3DShaderProgramFactory::GetPixelShader", 0x008A2190, ProgramFactory_GetPixelShaderImpl);
C41G_REVERSED("NiD3DShaderProgramFactory::InsertVertexShader", 0x008A21C0, ProgramFactory_InsertVertexShaderImpl);
C41G_REVERSED("NiD3DShaderProgramFactory::InsertPixelShader", 0x008A21F0, ProgramFactory_InsertPixelShaderImpl);
C41G_REVERSED("NiD3DShaderProgramFactory::UnregisterShaderCreator", 0x008A2220,
              ProgramFactory_UnregisterShaderCreator);
C41G_REVERSED("NiD3DShaderProgramFactory::GetShaderCreator", 0x008A22B0, ProgramFactory_GetShaderCreatorImpl);
C41G_REVERSED("NiD3DShaderProgramFactory::DestroyCreatorMap", 0x008A2390, ProgramFactory_DestroyCreatorMap);
C41G_REVERSED("NiTPointerListBase::NewItem", 0x008A23C0, PointerListBase_NewItem);
C41G_REVERSED("NiD3DShaderProgramFactory::CreateVertexShaderFromBuffer", 0x008A23D0,
              ProgramFactory_CreateVertexShaderFromBuffer);
C41G_REVERSED("NiD3DShaderProgramFactory::CreatePixelShaderFromBuffer", 0x008A2460,
              ProgramFactory_CreatePixelShaderFromBuffer);
C41G_REVERSED("NiD3DShaderProgramFactory::ReleaseVertexShader", 0x008A24E0, ProgramFactory_ReleaseVertexShader);
C41G_REVERSED("NiD3DShaderProgramFactory::ReleasePixelShader", 0x008A2510, ProgramFactory_ReleasePixelShader);
C41G_REVERSED("NiD3DShaderProgramFactory::AddProgramDirectory", 0x008A2540, ProgramFactory_AddDirectory);
C41G_REVERSED("NiD3DShaderProgramFactory::RemoveProgramDirectory", 0x008A2680, ProgramFactory_RemoveDirectory);
C41G_REVERSED("NiD3DShaderProgramFactory::RemoveAllProgramDirectories", 0x008A2770,
              ProgramFactory_RemoveAllDirectoriesImpl);
C41G_REVERSED("NiD3DShaderProgramFactory::ResolveProgramFileName", 0x008A27D0, ProgramFactory_ResolveProgramFileName);
C41G_REVERSED("NiD3DShaderProgramFactory::NiD3DShaderProgramFactory", 0x008A2990, ProgramFactory_Construct);
C41G_REVERSED("NiD3DShaderProgramFactory::~NiD3DShaderProgramFactory", 0x008A2A70, ProgramFactory_DestructImpl);
C41G_REVERSED("NiD3DShaderProgramFactory::CreateCreatorMapAlways", 0x008A2AF0, ProgramFactory_CreateCreatorMapImplFn);
C41G_REVERSED("NiD3DShaderProgramFactory::CreateCreatorMap", 0x008A2B60, ProgramFactory_CreateCreatorMap);
C41G_REVERSED("NiD3DShaderProgramFactory::DeletingDestructor", 0x008A2B70, ProgramFactory_DeletingDestructor);
C41G_REVERSED("NiD3DShaderProgramFactory::RegisterShaderCreator", 0x008A2B90, ProgramFactory_RegisterShaderCreator);

} // namespace ni::c41
