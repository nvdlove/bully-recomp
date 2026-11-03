// NiD3D10ShaderProgramFactory (vtable 0x9A1638: one slot, the deleting destructor; 0x54 bytes; the
// instance at 0x20E9264): the DX9 NiD3DShaderProgramFactory's counterpart. +0x4 the program
// directories (NiTPointerList<char*>, vtable 0x952E10: +0x8 head, +0xC tail, +0x10 count; the strings
// NiMalloc'd), +0x14 / +0x24 / +0x34 the vertex / geometry / pixel shaders by name, +0x44 the
// shader program creators by file extension (NiTFixedStringMap: vtable 0x9A162C on NiTMapBase's
// 0x9A1978; +0x4 the hash size (59; 13 for the creators), +0x8 the buckets, +0xC the count; an
// item: +0x0 next, +0x4 the key (an NiFixedString: hashed by its handle), +0x8 the value). The
// compile flags are a static (0x20E9308).
//
// The programs themselves (NiD3D10ShaderProgram, chunk 43's: +0x24 the D3D10 shader object) have
// three kinds here: NiD3D10PixelShader (vtable 0x9A163C, NiRTTI 0x20E9268; 0x28 bytes), and the
// slots the geometry / vertex shaders share with it (5 DestroyRendererData).

#include "gamebryo/NiD3D10C44.h"
#include "reversed.h"

namespace ni::c44 {
namespace {

constexpr uintptr_t kFactoryVtable = 0x009A1638;
constexpr uintptr_t kDirectoryListVtable = 0x00952E10;   // NiTPointerList<char*>
constexpr uintptr_t kListAllocatorVtable = 0x0099C748;
constexpr uintptr_t kFixedStringMapVtable = 0x009A162C;
constexpr uintptr_t kMapBaseVtable = 0x009A1978;
constexpr uintptr_t kPixelShaderVtable = 0x009A163C;
constexpr uint32_t kFactorySize = 0x54;

GAME_VAR(uint8_t*, ms_pkFactory, 0x020E9264);
GAME_VAR(uint32_t, ms_uiCompileFlags, 0x020E9308);
GAME_VAR(void*, ms_pkHLSLCreator, 0x020E9304);   // (the HLSL program creator: a guess)
GAME_VAR(uint8_t, NiRTTI_NiD3D10PixelShader, 0x020E9268);

GAME_FN(void, __thiscall, PointerList_Remove, 0x0075F630, void* list, const re::Ptr32<char>* element);
GAME_FN(void, __thiscall, PointerList_RemovePos, 0x00771DA0, void* list, re::Ptr32<void>* position);
GAME_FN(void, __thiscall, PointerList_RemoveAll, 0x0075DB60, void* list);
GAME_FN(void, __thiscall, Filename_Construct, 0x007538C0, void* self, const char* path);
GAME_FN(void*, __thiscall, NiPointer_AssignPointer, 0x00800420, void* slot, void* object);
GAME_FN(const re::Ptr32<char>*, __thiscall, Program_GetName, 0x008BAC30, void* program);   // (+0xC)
GAME_FN(void*, __thiscall, Program_GetCreator, 0x008B6120, void* program);        // (+0x14)
GAME_FN(void*, __thiscall, Program_GetShader, 0x008C3450, void* program);         // (+0x24)
GAME_FN(void, __thiscall, ShaderProgram_Destruct, 0x008BADF0, void* self);        // (chunk 43's)
// this file's (called where modes apply)
GAME_FN(void, __thiscall, FixedMap_RemoveAll, 0x008C2230, void* map);
GAME_FN(void, __thiscall, FixedMap_GetNext, 0x008C2290, void* map, re::Ptr32<void>* position, re::Ptr32<char>* key, re::Ptr32<void>* value);
GAME_FN(void, __thiscall, FixedMap_SetAt, 0x008C27A0, void* map, const re::Ptr32<char>* key, void* value);
GAME_FN(void*, __thiscall, Factory_GetVertexShader, 0x008C2470, void* self, const re::Ptr32<char>* name);
GAME_FN(void*, __thiscall, Factory_GetGeometryShader, 0x008C24A0, void* self, const re::Ptr32<char>* name);
GAME_FN(void*, __thiscall, Factory_GetPixelShader, 0x008C24D0, void* self, const re::Ptr32<char>* name);
GAME_FN(void, __cdecl, Factory_UnregisterShaderCreator, 0x008C2500, void* creator);
GAME_FN(void*, __cdecl, Factory_GetShaderCreator, 0x008C2620, const char* fileName);
GAME_FN(void*, __thiscall, Factory_Construct, 0x008C2820, void* self);
GAME_FN(void, __thiscall, Factory_RemoveAllDirectories, 0x008C2A00, void* self);
GAME_FN(void, __thiscall, Factory_InsertVertexShader, 0x008C2A60, void* self, void* program);
GAME_FN(void, __thiscall, Factory_InsertGeometryShader, 0x008C2AE0, void* self, void* program);
GAME_FN(void, __thiscall, Factory_InsertPixelShader, 0x008C2B60, void* self, void* program);
GAME_FN(void, __thiscall, Factory_Destruct, 0x008C2D10, void* self);
GAME_FN(uint8_t, __cdecl, Factory_RecreatePixelShader, 0x008C2190, void* program);

// a NiTFixedStringMap's item
struct MapItem {
    re::Ptr32<MapItem> next;   // +0x0
    re::Ptr32<char> key;       // +0x4
    re::Ptr32<void> value;     // +0x8
};
inline uint32_t HashSize(const void* map) {
    return At<uint32_t>(map, 4);
}
inline re::Ptr32<MapItem>* Buckets(const void* map) {
    return At<re::Ptr32<MapItem>*>(map, 8);
}

// the value kept under a handle (null when there's none)
void* Lookup(const void* map, char* key) {
    for (MapItem* item = Buckets(map)[uint32_t(reinterpret_cast<uintptr_t>(key)) % HashSize(map)]; item != nullptr;
         item = item->next) {
        if (key == item->key) {
            return item->value;
        }
    }
    return nullptr;
}

// a directory with its slashes made backslashes (the length counted again each step)
void NormalizeDirectory(char* buffer, const char* directory) {
    crt::StrcpyS(buffer, 0x104, directory);
    for (uint32_t i = 0; i < uint32_t(crt::Strcspn(buffer, "")); i++) {
        if (buffer[i] == '/') {
            buffer[i] = '\\';
        }
    }
}

// a map made in place (NiTMapBase's vtable, its buckets NiMalloc'd and zeroed, then the fixed
// string map's)
void ConstructMap(uint8_t* map, uint32_t hashSize) {
    At<uint32_t>(map, 0) = kMapBaseVtable;
    At<uint32_t>(map, 4) = hashSize;
    At<uint32_t>(map, 0xC) = 0;
    void* buckets = NiMalloc(hashSize * 4);
    At<void*>(map, 8) = buckets;
    crt::Memset(buckets, 0, At<uint32_t>(map, 4) * 4);
    At<uint32_t>(map, 0) = kFixedStringMapVtable;
}

// a map's destructor as the factory's inlines it
void DestructMap(uint8_t* map) {
    At<uint32_t>(map, 0) = kFixedStringMapVtable;
    FixedMap_RemoveAll(map);
    At<uint32_t>(map, 0) = kMapBaseVtable;
    FixedMap_RemoveAll(map);
    NiFree(At<void*>(map, 8));
}

// a program kept by its name in one of the maps
void Insert(uint8_t* self, int mapOffset, void* program) {
    if (program == nullptr) {
        return;
    }
    re::Ptr32<char> name = StringAddRef(*Program_GetName(program));
    FixedMap_SetAt(self + mapOffset, &name, program);
    StringRelease(&name);
}

// a program made through the creator for its file's extension (slot `slot`, `extra` zero
// arguments before the result for the geometry shaders), kept; or the one of its name already
using Lookup_t = void*(__thiscall*)(void*, const re::Ptr32<char>*);
using Insert_t = void(__thiscall*)(void*, void*);
uint8_t CreateProgram(const char* source, const char* fileName, void* macros, void* include, const char* entry,
                      const char* target, uint32_t flags, const char* name, re::Ptr32<void>* result, Lookup_t lookup,
                      Insert_t insert, int slot, bool geometry) {
    if (ms_pkFactory == nullptr || source == nullptr) {
        return 0;
    }
    re::Ptr32<char> handle = name != nullptr ? FixedString_Add(name) : NullString();
    void* existing = lookup(ms_pkFactory, &handle);
    StringRelease(&handle);
    if (existing != nullptr) {
        NiPointer_AssignPointer(result, existing);
        return 1;
    }
    void* creator = Factory_GetShaderCreator(fileName);
    if (creator == nullptr) {
        return 0;
    }
    uint8_t made;
    if (geometry) {
        made = Virtual<uint8_t>(creator, slot, source, fileName, macros, include, entry, target, flags, name, 0u, 0u,
                                0u, result);
    } else {
        made = Virtual<uint8_t>(creator, slot, source, fileName, macros, include, entry, target, flags, name, result);
    }
    if (made != 0) {
        insert(ms_pkFactory, *result);
    }
    return made;
}

} // namespace

// 0x8C2100 (cdecl): GetInstance
uint8_t* __cdecl ProgramFactory_GetInstance() {
    return ms_pkFactory;
}

// 0x8C2190 (cdecl): RecreatePixelShader (true when it has its shader, else the creator's slot 11)
uint8_t __cdecl ProgramFactory_RecreatePixelShaderImpl(void* program) {
    if (ms_pkFactory == nullptr || program == nullptr) {
        return 0;
    }
    if (Program_GetShader(program) != nullptr) {
        return 1;
    }
    void* creator = Program_GetCreator(program);
    if (creator == nullptr) {
        return 0;
    }
    return Virtual<uint8_t>(creator, 11, program);
}

// 0x8C21D0 (cdecl): Shutdown (the instance deleted)
void __cdecl ProgramFactory_Shutdown() {
    if (ms_pkFactory != nullptr) {
        Virtual<void>(ms_pkFactory, 0, 1);
    }
    ms_pkFactory = nullptr;
}

// 0x8C21F0 (cdecl): GetCompileFlags
uint32_t __cdecl ProgramFactory_GetCompileFlags() {
    return ms_uiCompileFlags;
}

// 0x8C2200 (cdecl): SetCompileFlags
uint32_t __cdecl ProgramFactory_SetCompileFlags(uint32_t flags) {
    ms_uiCompileFlags = flags;
    return flags;   // (what the original leaves in eax)
}

// 0x8C2210 (thiscall): GetFirstProgramDirectory (the position on to the next)
const char* __fastcall ProgramFactory_GetFirstDirectory(uint8_t* self, re::Edx, re::Ptr32<void>* position) {
    uint8_t* head = At<uint8_t*>(self, 8);
    *position = head;
    if (head == nullptr) {
        return nullptr;
    }
    *position = At<void*>(head, 0);
    return At<const char*>(head, 8);
}

// 0x8C2230 (thiscall): NiTFixedStringMap::RemoveAll (each item deleted through slot 2)
void __fastcall FixedMap_RemoveAllImpl(uint8_t* self, re::Edx) {
    for (uint32_t i = 0; i < HashSize(self); i++) {
        while (Buckets(self)[i] != nullptr) {
            MapItem* item = Buckets(self)[i];
            Buckets(self)[i] = item->next;
            Virtual<void>(self, 2, item);
        }
    }
    At<uint32_t>(self, 0xC) = 0;
}

// 0x8C2290 (thiscall): NiTFixedStringMap::GetNext (the key and value at the position; the position
// on to the next item, in the bucket or the ones after it)
void __fastcall FixedMap_GetNextImpl(uint8_t* self, re::Edx, re::Ptr32<MapItem>* position, re::Ptr32<char>* key, re::Ptr32<void>* value) {
    MapItem* item = *position;
    FixedString_Assign(key, &item->key);
    *value = item->value;
    if (item->next != nullptr) {
        *position = item->next;
        return;
    }
    uint32_t size = HashSize(self);
    for (uint32_t i = uint32_t(re::PtrCast<uintptr_t>(item->key)) % size + 1; i < size; i++) {
        if (Buckets(self)[i] != nullptr) {
            *position = Buckets(self)[i];
            return;
        }
    }
    *position = nullptr;
}

// 0x8C2300 (thiscall, slot 0): NiTFixedStringMap's deleting destructor
void* __fastcall FixedMap_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    DestructMap(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x10);
    }
    return self;
}

// 0x8C2340 (thiscall): AddProgramDirectory (with backslashes; a copy added at the head unless it's
// there, case aside)
void __fastcall ProgramFactory_AddDirectory(uint8_t* self, re::Edx, const char* directory) {
    char buffer[0x104];
    if (directory == nullptr || *directory == '\0') {
        return;
    }
    NormalizeDirectory(buffer, directory);
    for (uint8_t* node = At<uint8_t*>(self, 8); node != nullptr; node = At<uint8_t*>(node, 0)) {
        const char* existing = At<const char*>(node, 8);
        if (existing != nullptr && NiStricmp(existing, buffer) == 0) {
            return;
        }
    }
    size_t size = crt::Strcspn(buffer, "") + 1;
    char* copy = static_cast<char*>(NiMalloc(uint32_t(size)));
    crt::StrcpyS(copy, size, buffer);
    uint8_t* node = Virtual<uint8_t*>(self + 4, 1);   // NewItem
    At<char*>(node, 8) = copy;
    At<uint8_t*>(node, 4) = nullptr;
    At<uint8_t*>(node, 0) = At<uint8_t*>(self, 8);
    if (At<uint8_t*>(self, 8) == nullptr) {
        At<uint8_t*>(self, 0xC) = node;
    } else {
        At<uint8_t*>(At<uint8_t*>(self, 8), 4) = node;
    }
    At<uint32_t>(self, 0x10)++;
    At<uint8_t*>(self, 8) = node;
}

// 0x8C2470 / 0x8C24A0 / 0x8C24D0 (thiscall): the vertex / geometry / pixel shader of a name
void* __fastcall ProgramFactory_GetVertexShader(uint8_t* self, re::Edx, const re::Ptr32<char>* name) {
    return Lookup(self + 0x14, *name);
}
void* __fastcall ProgramFactory_GetGeometryShader(uint8_t* self, re::Edx, const re::Ptr32<char>* name) {
    return Lookup(self + 0x24, *name);
}
void* __fastcall ProgramFactory_GetPixelShader(uint8_t* self, re::Edx, const re::Ptr32<char>* name) {
    return Lookup(self + 0x34, *name);
}

// 0x8C2500 (cdecl): UnregisterShaderCreator (every extension it's kept under removed)
void __cdecl ProgramFactory_UnregisterShaderCreator(void* creator) {
    uint8_t* map = ms_pkFactory + 0x44;
    MapItem* position = nullptr;
    for (uint32_t i = 0; i < HashSize(map); i++) {
        if (Buckets(map)[i] != nullptr) {
            position = Buckets(map)[i];
            break;
        }
    }
    while (position != nullptr) {
        re::Ptr32<char> key = NullString();
        re::Ptr32<void> value = nullptr;
        FixedMap_GetNext(ms_pkFactory + 0x44, reinterpret_cast<re::Ptr32<void>*>(&position), &key, &value);
        if (creator == value) {
            uint8_t* creators = ms_pkFactory + 0x44;
            re::Ptr32<MapItem>* link = &Buckets(creators)[uint32_t(re::PtrCast<uintptr_t>(key)) % HashSize(creators)];
            for (MapItem* item = *link; item != nullptr; link = &item->next, item = item->next) {
                if (key == item->key) {
                    *link = item->next;
                    Virtual<void>(creators, 2, item);
                    At<uint32_t>(creators, 0xC) -= 1;
                    break;
                }
            }
        }
        StringRelease(&key);
    }
}

// 0x8C2620 (cdecl): GetShaderCreator (by the file's extension, lowercased; the whole name when it
// has none; null when it has no extension at all)
void* __cdecl ProgramFactory_GetShaderCreatorImpl(const char* fileName) {
    if (fileName == nullptr || *fileName == '\0') {
        return nullptr;
    }
    alignas(4) uint8_t filename[0x404];   // NiFilename (+0x103 the extension)
    Filename_Construct(filename, fileName);
    char extension[0x100];
    crt::StrcpyS(extension, 0x100, reinterpret_cast<const char*>(filename + 0x103));
    if (extension[0] == '\0') {
        return nullptr;
    }
    const char* from = extension[1] != '\0' ? extension + 1 : fileName;
    for (uint32_t i = 0; i < 0x100; i++) {
        char c = char(crt::Tolower(from[i]));
        extension[i] = c;
        if (c == '\0') {
            break;
        }
    }
    re::Ptr32<char> key = FixedString_Add(extension);
    void* creator = Lookup(ms_pkFactory + 0x44, key);
    StringRelease(&key);
    return creator;
}

// 0x8C2BE0 (cdecl): RegisterShaderCreator (an extension without a '.', lowercased, to the creator
// in the factory's map; false for none or a dotted one)
uint8_t __cdecl ProgramFactory_RegisterShaderCreatorImpl(const char* extension, void* creator) {
    if (extension == nullptr || crt::Strchr(extension, '.') != nullptr) {
        return 0;
    }
    char lower[0x100];
    for (uint32_t i = 0; i < 0x100; i++) {
        lower[i] = char(crt::Tolower(extension[i]));
        if (extension[i] == '\0') {
            break;
        }
    }
    re::Ptr32<char> key = FixedString_Add(lower);
    FixedMap_SetAt(ms_pkFactory + 0x44, &key, creator);
    StringRelease(&key);
    return 1;
}

// 0x8C27A0 (thiscall): NiTFixedStringMap::SetAt (the key's item given the value; a new one, slot 1,
// at its bucket's head when there's none)
void __fastcall FixedMap_SetAtImpl(uint8_t* self, re::Edx, const re::Ptr32<char>* key, void* value) {
    uint32_t bucket = uint32_t(re::PtrCast<uintptr_t>(*key)) % HashSize(self);
    for (MapItem* item = Buckets(self)[bucket]; item != nullptr; item = item->next) {
        if (*key == item->key) {
            FixedString_Assign(&item->key, key);
            item->value = value;
            return;
        }
    }
    MapItem* item = Virtual<MapItem*>(self, 1);   // NewItem
    FixedString_Assign(&item->key, key);
    item->value = value;
    item->next = Buckets(self)[bucket];
    Buckets(self)[bucket] = item;
    At<uint32_t>(self, 0xC) += 1;
}

// 0x8C2820 (thiscall): the constructor
void* __fastcall ProgramFactory_Construct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kFactoryVtable;
    At<uint32_t>(self, 0x10) = 0;
    At<void*>(self, 8) = nullptr;
    At<void*>(self, 0xC) = nullptr;
    At<uint32_t>(self, 4) = kDirectoryListVtable;
    ConstructMap(self + 0x14, 0x3B);
    ConstructMap(self + 0x24, 0x3B);
    ConstructMap(self + 0x34, 0x3B);
    ConstructMap(self + 0x44, 0xD);
    return self;
}

// 0x8C2920 (thiscall): RemoveProgramDirectory (its node removed; the string not freed)
void __fastcall ProgramFactory_RemoveDirectory(uint8_t* self, re::Edx, const char* directory) {
    char buffer[0x104];
    if (directory == nullptr || *directory == '\0') {
        return;
    }
    NormalizeDirectory(buffer, directory);
    for (uint8_t* node = At<uint8_t*>(self, 8); node != nullptr;) {
        re::Ptr32<char> existing = At<char*>(node, 8);
        node = At<uint8_t*>(node, 0);
        if (existing != nullptr && NiStricmp(existing, buffer) == 0) {
            PointerList_Remove(self + 4, &existing);
            return;
        }
    }
}

// 0x8C2A00 (thiscall): RemoveAllProgramDirectories (each node removed and its string freed)
void __fastcall ProgramFactory_RemoveAllDirectoriesImpl(uint8_t* self, re::Edx) {
    uint8_t* node = At<uint8_t*>(self, 8);
    while (node != nullptr) {
        char* directory = At<char*>(node, 8);
        node = At<uint8_t*>(node, 0);
        if (directory == nullptr) {
            continue;
        }
        re::Ptr32<void> position = At<uint8_t*>(self, 8);
        while (position != nullptr && At<char*>(position, 8) != directory) {
            position = At<uint8_t*>(position, 0);
        }
        if (position != nullptr) {
            PointerList_RemovePos(self + 4, &position);
        }
        NiFree(directory);
    }
}

// 0x8C2A60 / 0x8C2AE0 / 0x8C2B60 (thiscall): a vertex / geometry / pixel shader kept by its name
void __fastcall ProgramFactory_InsertVertexShader(uint8_t* self, re::Edx, void* program) {
    Insert(self, 0x14, program);
}
void __fastcall ProgramFactory_InsertGeometryShader(uint8_t* self, re::Edx, void* program) {
    Insert(self, 0x24, program);
}
void __fastcall ProgramFactory_InsertPixelShader(uint8_t* self, re::Edx, void* program) {
    Insert(self, 0x34, program);
}

// 0x8C2CE0 (cdecl): the instance made when there's none
void __cdecl ProgramFactory_Create() {
    if (ms_pkFactory != nullptr) {
        return;
    }
    void* memory = NiNew(kFactorySize);
    ms_pkFactory = memory != nullptr ? static_cast<uint8_t*>(Factory_Construct(memory)) : nullptr;
}

// 0x8C2D10 (thiscall): the destructor (the maps emptied, the directories freed; the maps' and the
// list's destructors)
void __fastcall ProgramFactory_DestructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kFactoryVtable;
    FixedMap_RemoveAll(self + 0x14);
    FixedMap_RemoveAll(self + 0x24);
    FixedMap_RemoveAll(self + 0x34);
    FixedMap_RemoveAll(self + 0x44);
    Factory_RemoveAllDirectories(self);
    DestructMap(self + 0x44);
    DestructMap(self + 0x34);
    DestructMap(self + 0x24);
    DestructMap(self + 0x14);
    At<uint32_t>(self, 4) = kDirectoryListVtable;
    PointerList_RemoveAll(self + 4);
    At<uint32_t>(self, 4) = kListAllocatorVtable;
}

// 0x8C2E00 / 0x8C2F00 (cdecl): a vertex shader from source / from compiled code (the creator's
// slots 3 / 4); the existing one of its name
uint8_t __cdecl ProgramFactory_CreateVertexShaderFromSource(const char* source, const char* fileName, void* macros,
                                                            void* include, const char* entry, const char* target,
                                                            uint32_t flags, const char* name, re::Ptr32<void>* result) {
    return CreateProgram(source, fileName, macros, include, entry, target, flags, name, result,
                         Factory_GetVertexShader, Factory_InsertVertexShader, 3, false);
}
uint8_t __cdecl ProgramFactory_CreateVertexShaderFromCompiled(const char* source, const char* fileName,
                                                              void* macros, void* include, const char* entry,
                                                              const char* target, uint32_t flags, const char* name,
                                                              re::Ptr32<void>* result) {
    return CreateProgram(source, fileName, macros, include, entry, target, flags, name, result,
                         Factory_GetVertexShader, Factory_InsertVertexShader, 4, false);
}

// 0x8C3000 / 0x8C3100 (cdecl): a geometry shader (slots 5 / 6: three more arguments, zero: the
// stream output's, a guess)
uint8_t __cdecl ProgramFactory_CreateGeometryShaderFromSource(const char* source, const char* fileName,
                                                              void* macros, void* include, const char* entry,
                                                              const char* target, uint32_t flags, const char* name,
                                                              re::Ptr32<void>* result) {
    return CreateProgram(source, fileName, macros, include, entry, target, flags, name, result,
                         Factory_GetGeometryShader, Factory_InsertGeometryShader, 5, true);
}
uint8_t __cdecl ProgramFactory_CreateGeometryShaderFromCompiled(const char* source, const char* fileName,
                                                                void* macros, void* include, const char* entry,
                                                                const char* target, uint32_t flags,
                                                                const char* name, re::Ptr32<void>* result) {
    return CreateProgram(source, fileName, macros, include, entry, target, flags, name, result,
                         Factory_GetGeometryShader, Factory_InsertGeometryShader, 6, true);
}

// 0x8C3200 / 0x8C3300 (cdecl): a pixel shader (slots 7 / 8)
uint8_t __cdecl ProgramFactory_CreatePixelShaderFromSource(const char* source, const char* fileName, void* macros,
                                                           void* include, const char* entry, const char* target,
                                                           uint32_t flags, const char* name, re::Ptr32<void>* result) {
    return CreateProgram(source, fileName, macros, include, entry, target, flags, name, result,
                         Factory_GetPixelShader, Factory_InsertPixelShader, 7, false);
}
uint8_t __cdecl ProgramFactory_CreatePixelShaderFromCompiled(const char* source, const char* fileName,
                                                             void* macros, void* include, const char* entry,
                                                             const char* target, uint32_t flags, const char* name,
                                                             re::Ptr32<void>* result) {
    return CreateProgram(source, fileName, macros, include, entry, target, flags, name, result,
                         Factory_GetPixelShader, Factory_InsertPixelShader, 8, false);
}

// 0x8C3400 (thiscall, slot 0): the deleting destructor
void* __fastcall ProgramFactory_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    Factory_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, kFactorySize);
    }
    return self;
}

// 0x8C3490 (thiscall, the programs' slot 5): DestroyRendererData (the D3D10 shader released)
void __fastcall ShaderProgram_DestroyRendererData(uint8_t* self, re::Edx) {
    void* shader = At<void*>(self, 0x24);
    if (shader != nullptr) {
        Com<long>(shader, 2);
        At<void*>(self, 0x24) = nullptr;
    }
}

// 0x8C34B0 (thiscall, NiD3D10PixelShader's slot 6): RecreateRendererData
uint8_t __fastcall PixelShader_RecreateRendererData(uint8_t* self, re::Edx) {
    return Factory_RecreatePixelShader(self);
}

// 0x8C34C0 (slot 2): NiD3D10PixelShader::GetRTTI
void* __fastcall PixelShader_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiD3D10PixelShader;
}

// 0x8C34D0 (thiscall, slot 0): NiD3D10PixelShader's deleting destructor (its shader released)
void* __fastcall PixelShader_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    void* shader = At<void*>(self, 0x24);
    At<uint32_t>(self, 0) = kPixelShaderVtable;
    if (shader != nullptr) {
        Com<long>(shader, 2);
    }
    ShaderProgram_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x28);
    }
    return self;
}

// 0x8C41F0 (cdecl): the HLSL program creator let go (unregistered, deleted; a guess at the name)
void __cdecl ProgramCreatorHLSL_Shutdown() {
    if (ms_pkHLSLCreator != nullptr) {
        Factory_UnregisterShaderCreator(ms_pkHLSLCreator);
        if (ms_pkHLSLCreator != nullptr) {
            Virtual<void>(ms_pkHLSLCreator, 0, 1);
        }
    }
    ms_pkHLSLCreator = nullptr;
}

// 0x8C4220, 0x8C4250 (thiscall, vf0): the program creators' deleting destructors (the base's
// vtable 0x9A16D0 put back; the base 4 bytes, the HLSL one 8). Only the shutdown deletes them
// (ProgramCreatorHLSL_Shutdown): the 64-bit dispatch's bodies, not hooked in the 32-bit game
void* __fastcall ProgramCreator_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = 0x009A16D0;
    if ((flags & 1) != 0) {
        NiDelete(self, 4);
    }
    return self;
}
RE_UNHOOKED_FN("NiD3D10ShaderProgramCreator::DeletingDestructor", 0x008C4220, &ProgramCreator_DeletingDestructor);
void* __fastcall ProgramCreatorHLSL_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = 0x009A16D0;
    if ((flags & 1) != 0) {
        NiDelete(self, 8);
    }
    return self;
}
RE_UNHOOKED_FN("NiD3D10ShaderProgramCreatorHLSL::DeletingDestructor", 0x008C4250, &ProgramCreatorHLSL_DeletingDestructor);

// 0x8C3450 (4 bytes: under a hook's size): a folded getter of +0x24 (the programs' D3D10 shader;
// many of the game's classes share it)
// 0x8C3450 (thiscall): the program's shader (+0x24)
void* __fastcall D3D10ShaderProgram_GetShader(uint8_t* self, re::Edx) {
    return reinterpret_cast<void*>(uintptr_t(*reinterpret_cast<const uint32_t*>(self + 0x24)));
}
RE_UNHOOKED_FN("NiD3D10ShaderProgram::GetShader", 0x008C3450, &D3D10ShaderProgram_GetShader);

#define GB44F_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

GB44F_REVERSED("NiD3D10ShaderProgramFactory::GetInstance", 0x008C2100, ProgramFactory_GetInstance);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::RecreatePixelShader", 0x008C2190, ProgramFactory_RecreatePixelShaderImpl);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::Shutdown", 0x008C21D0, ProgramFactory_Shutdown);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::GetCompileFlags", 0x008C21F0, ProgramFactory_GetCompileFlags);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::SetCompileFlags", 0x008C2200, ProgramFactory_SetCompileFlags);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::GetFirstProgramDirectory", 0x008C2210, ProgramFactory_GetFirstDirectory);
GB44F_REVERSED("NiTFixedStringMap<NiD3D10ShaderProgram*>::RemoveAll", 0x008C2230, FixedMap_RemoveAllImpl);
GB44F_REVERSED("NiTFixedStringMap<NiD3D10ShaderProgram*>::GetNext", 0x008C2290, FixedMap_GetNextImpl);
GB44F_REVERSED("NiTFixedStringMap<NiD3D10ShaderProgram*>::DeletingDestructor", 0x008C2300, FixedMap_DeletingDestructor);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::AddProgramDirectory", 0x008C2340, ProgramFactory_AddDirectory);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::GetVertexShader", 0x008C2470, ProgramFactory_GetVertexShader);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::GetGeometryShader", 0x008C24A0, ProgramFactory_GetGeometryShader);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::GetPixelShader", 0x008C24D0, ProgramFactory_GetPixelShader);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::UnregisterShaderCreator", 0x008C2500,
               ProgramFactory_UnregisterShaderCreator);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::GetShaderCreator", 0x008C2620, ProgramFactory_GetShaderCreatorImpl);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::RegisterShaderCreator", 0x008C2BE0, ProgramFactory_RegisterShaderCreatorImpl);
GB44F_REVERSED("NiTFixedStringMap<NiD3D10ShaderProgram*>::SetAt", 0x008C27A0, FixedMap_SetAtImpl);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::NiD3D10ShaderProgramFactory", 0x008C2820, ProgramFactory_Construct);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::RemoveProgramDirectory", 0x008C2920, ProgramFactory_RemoveDirectory);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::RemoveAllProgramDirectories", 0x008C2A00,
               ProgramFactory_RemoveAllDirectoriesImpl);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::InsertVertexShader", 0x008C2A60, ProgramFactory_InsertVertexShader);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::InsertGeometryShader", 0x008C2AE0, ProgramFactory_InsertGeometryShader);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::InsertPixelShader", 0x008C2B60, ProgramFactory_InsertPixelShader);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::Create", 0x008C2CE0, ProgramFactory_Create);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::~NiD3D10ShaderProgramFactory", 0x008C2D10, ProgramFactory_DestructImpl);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::CreateVertexShaderFromSource", 0x008C2E00,
               ProgramFactory_CreateVertexShaderFromSource);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::CreateVertexShaderFromCompiled", 0x008C2F00,
               ProgramFactory_CreateVertexShaderFromCompiled);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::CreateGeometryShaderFromSource", 0x008C3000,
               ProgramFactory_CreateGeometryShaderFromSource);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::CreateGeometryShaderFromCompiled", 0x008C3100,
               ProgramFactory_CreateGeometryShaderFromCompiled);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::CreatePixelShaderFromSource", 0x008C3200,
               ProgramFactory_CreatePixelShaderFromSource);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::CreatePixelShaderFromCompiled", 0x008C3300,
               ProgramFactory_CreatePixelShaderFromCompiled);
GB44F_REVERSED("NiD3D10ShaderProgramFactory::DeletingDestructor", 0x008C3400, ProgramFactory_DeletingDestructor);
GB44F_REVERSED("NiD3D10ShaderProgram::DestroyRendererData", 0x008C3490, ShaderProgram_DestroyRendererData);
GB44F_REVERSED("NiD3D10PixelShader::RecreateRendererData", 0x008C34B0, PixelShader_RecreateRendererData);
GB44F_REVERSED("NiD3D10PixelShader::GetRTTI", 0x008C34C0, PixelShader_GetRTTI);
GB44F_REVERSED("NiD3D10PixelShader::DeletingDestructor", 0x008C34D0, PixelShader_DeletingDestructor);
GB44F_REVERSED("NiD3D10ShaderProgramCreatorHLSL::Shutdown", 0x008C41F0, ProgramCreatorHLSL_Shutdown);

} // namespace ni::c44
