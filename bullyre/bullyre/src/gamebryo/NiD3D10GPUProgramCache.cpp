// NiD3D10GPUProgramCache (vtable 0x9A1078 on NiGPUProgramCache's, src/gamebryo/NiGPUProgramCache.h;
// 0x58 bytes, NiD3DGPUProgramCache's layout and deleting destructor (0x898D20): +0x4C the program
// type (0 vertex, 1 pixel, 2 geometry), +0x50 write each program's source to a debug file, +0x54
// the material's name): the D3D10 renderer's cache of compiled HLSL programs. Its file is
// "<material>_<profile>_D3D10.cache"; vs_4_0, gs_4_0 and ps_4_0 profiles are the ones kept in
// the base's set. Its slots: 4 GenerateProgram, 9 the extension ("hlsl"), 10 the platform
// ("D3D10", the shader factory's 0x8B9580), 25 SaveProgram, 26 LoadProgram, 27 WriteDebugFile
// (DX9's).

#include "gamebryo/NiD3D10C43.h"
#include "gamebryo/NiGPUProgramCache.h"
#include "reversed.h"

namespace ni::c43 {
namespace {

using ni::sys::FixedStringRelease;
using ni::sys::kNullFixedString;

constexpr uintptr_t kProgramCacheVtable = 0x009A1078;
const char* const kPlatform = reinterpret_cast<const char*>(0x00955DD0);    // "D3D10"
const char* const kExtension = reinterpret_cast<const char*>(0x0099D1C8);   // "hlsl"
const char* const kEntryPoint = reinterpret_cast<const char*>(0x0099D1D0);  // "Main"
const char* const kVS40 = reinterpret_cast<const char*>(0x009A1070);        // "vs_4_0"
const char* const kGS40 = reinterpret_cast<const char*>(0x009A1068);        // "gs_4_0"
const char* const kPS40 = reinterpret_cast<const char*>(0x009A1060);        // "ps_4_0"

GAME_VAR(const char*, NiRTTI_NiD3D10VertexShader_Name, 0x020E92D0);   // (the NiRTTIs' names)
GAME_VAR(const char*, NiRTTI_NiD3D10GeometryShader_Name, 0x020E929C);
GAME_VAR(const char*, NiRTTI_NiD3D10PixelShader_Name, 0x020E9268);

GAME_FN(void*, __thiscall, ProgramCache_Construct, 0x008CBCD0, void* self, uint32_t version, int autoAppend,
        int locked);
GAME_FN(void, __thiscall, ProgramCache_SetWorkingDirectory, 0x008CAD00, void* self, const re::Ptr32<char>* directory);
GAME_FN(uint8_t, __thiscall, ProgramCache_Load, 0x008CB1C0, void* self);
GAME_FN(void, __thiscall, ProgramCache_SaveString, 0x008CAF00, void* self, void* stream, const char* text);
GAME_FN(void, __thiscall, ProgramCache_LoadString, 0x008CAFA0, void* self, void* stream, char* buffer,
        uint32_t size);
GAME_FN(void, __thiscall, FixedString_AssignChars, 0x00406730, re::Ptr32<char>* self, const char* text);
GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, const re::Ptr32<char>* other);
GAME_FN(void, __thiscall, FixedString_Destruct, 0x00517750, re::Ptr32<char>* self);
GAME_FN(char*, __cdecl, StringTable_AddString, 0x00754760, const char* text);
GAME_FN(void, __thiscall, FixedStringSet_Add, 0x00848D60, void* set, const re::Ptr32<char>* value);
GAME_FN(void, __thiscall, FixedStringSet_Realloc, 0x00847E40, void* set, uint32_t size);
GAME_FN(int, __cdecl, NiStricmp, 0x00753820, const char* a, const char* b);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(void*, __thiscall, Program_GetBlob, 0x0077E8D0, void* program);   // (a folded getter)
GAME_FN(void, __cdecl, Stream_LoadBinary, 0x008885F0, void* stream, void* buffer, uint32_t size);
GAME_FN(long, __cdecl, D3D10_CreateBlob, 0x008A94C0, uint32_t size, re::Ptr32<void>* blob);
GAME_FN(void, __thiscall, NiPointer_Assign, 0x00800420, void* slot, void* object);
GAME_FN(void, __thiscall, NiPointer_Destruct, 0x007549D0, void* slot);
GAME_FN(void, __thiscall, NiPointer_AssignPointer, 0x00518100, void* slot, const re::Ptr32<void>* other);
GAME_FN(void*, __thiscall, Descriptor_Construct, 0x00898D80, void* self);
GAME_FN(void, __thiscall, ObjectSet_Add, 0x00787BE0, void* set, const re::Ptr32<void>* element);
GAME_FN(void, __thiscall, ObjectSet_RemoveAll, 0x00836940, void* set);
GAME_FN(void, __thiscall, ProgramMap_SetAt, 0x008B9E50, void* map, const re::Ptr32<char>* key, void* descriptor);
// the D3D10 shader program factory's (chunk 44's): compile flags; programs made from compiled code
// (a cache's) or compiled from source in a blob (cdecl: blob, file name, 2 unknown, entry point,
// profile, unknown, name, the result)
GAME_FN(uint32_t, __cdecl, ProgramFactory_GetCompileFlags, 0x008C21F0);
GAME_FN(void, __cdecl, ProgramFactory_SetCompileFlags, 0x008C2200, uint32_t flags);
using ProgramFromBlob = void(__cdecl*)(void* blob, const char* fileName, uint32_t a, uint32_t b, const char* entry,
                                       const char* profile, uint32_t c, const char* name, re::Ptr32<void>* result);
GAME_FN(void, __cdecl, CreateVertexShaderFromCompiled, 0x008C2F00, void* blob, const char* fileName, uint32_t a,
        uint32_t b, const char* entry, const char* profile, uint32_t c, const char* name, re::Ptr32<void>* result);
GAME_FN(void, __cdecl, CreateGeometryShaderFromCompiled, 0x008C3100, void* blob, const char* fileName, uint32_t a,
        uint32_t b, const char* entry, const char* profile, uint32_t c, const char* name, re::Ptr32<void>* result);
GAME_FN(void, __cdecl, CreatePixelShaderFromCompiled, 0x008C3300, void* blob, const char* fileName, uint32_t a,
        uint32_t b, const char* entry, const char* profile, uint32_t c, const char* name, re::Ptr32<void>* result);
GAME_FN(void, __cdecl, CompileVertexShader, 0x008C2E00, void* blob, const char* fileName, uint32_t a, uint32_t b,
        const char* entry, const char* profile, uint32_t c, const char* name, re::Ptr32<void>* result);
GAME_FN(void, __cdecl, CompileGeometryShader, 0x008C3000, void* blob, const char* fileName, uint32_t a, uint32_t b,
        const char* entry, const char* profile, uint32_t c, const char* name, re::Ptr32<void>* result);
GAME_FN(void, __cdecl, CompilePixelShader, 0x008C3200, void* blob, const char* fileName, uint32_t a, uint32_t b,
        const char* entry, const char* profile, uint32_t c, const char* name, re::Ptr32<void>* result);

// a binary stream's read / write (+0x8 / +0xC: (stream, buffer, bytes, component sizes, count);
// +0x4 the position moved on)
inline void StreamWrite(void* stream, const void* buffer, uint32_t bytes, uint32_t component) {
    auto write = At<uint32_t(__cdecl*)(void*, const void*, uint32_t, uint32_t*, uint32_t)>(stream, 0xC);
    At<uint32_t>(stream, 4) += write(stream, buffer, bytes, &component, 1);
}
inline void StreamRead(void* stream, void* buffer, uint32_t bytes, uint32_t component) {
    auto read = At<uint32_t(__cdecl*)(void*, void*, uint32_t, uint32_t*, uint32_t)>(stream, 8);
    At<uint32_t>(stream, 4) += read(stream, buffer, bytes, &component, 1);
}

// the profile compared as the code inlines it (the same characters, or no case difference)
inline bool ProfileIs(const char* profile, const char* name) {
    return profile == name || (profile != nullptr && NiStricmp(profile, name) == 0);
}

// a profile name added to the base's set (NiTObjectSet<NiFixedString>::Add inlined: grown by
// doubling when full)
void AddProfileInline(uint8_t* self, const char* name) {
    re::Ptr32<char> handle = StringTable_AddString(name);
    uint32_t alloced = At<uint32_t>(self, 0x3C);
    if (At<uint32_t>(self, 0x40) == alloced) {
        FixedStringSet_Realloc(self + 0x38, alloced > 0 ? alloced * 2 : 1);
    }
    uint32_t count = At<uint32_t>(self, 0x40);
    re::Ptr32<char>* slot = &At<re::Ptr32<char>*>(self, 0x38)[count];
    At<uint32_t>(self, 0x40) = count + 1;
    FixedString_Assign(slot, &handle);
    FixedStringRelease(&handle);
}

} // namespace

// 0x8B89B0 (thiscall, slot 9): the programs' file extension
const char* __fastcall GPUProgramCache_GetExtension(void*, re::Edx) {
    return kExtension;
}

// 0x8B89C0 (thiscall, slot 25): NiD3D10GPUProgramCache::SaveProgram (its NiRTTI's name, its compiled
// code's size and code)
uint8_t __fastcall GPUProgramCache_SaveProgram(uint8_t* self, re::Edx, void* stream, void* program) {
    if (program == nullptr) {
        return 0;
    }
    const char* const* rtti = Virtual<const char* const*>(program, 2);
    ProgramCache_SaveString(self, stream, *rtti);
    void* blob = Program_GetBlob(program);
    uint32_t size = Com<uint32_t>(blob, kGetBufferSize);
    StreamWrite(stream, &size, 4, 4);
    StreamWrite(stream, Com<void*>(blob, kGetBufferPointer), size, 1);
    return 1;
}

// 0x8B8A60 (thiscall, slot 26): NiD3D10GPUProgramCache::LoadProgram(stream, name, NiPointer& program,
// skip): a vertex, geometry or pixel shader by the saved class name, made from its code
// ("<name>.hlsl", "Main", the cache's profile); skipped over when asked; false for an unknown
// class or no blob
uint8_t __fastcall GPUProgramCache_LoadProgram(uint8_t* self, re::Edx, void* stream, const re::Ptr32<char>* name, re::Ptr32<void>* program,
                                               int skip) {
    void* old = *program;
    if (old != nullptr) {
        ReleaseRef(old);
        *program = nullptr;
    }
    uint32_t size = 0;
    char className[0x104];
    ProgramCache_LoadString(self, stream, className, 0x104);
    ProgramFromBlob create;
    if (NiStricmp(className, NiRTTI_NiD3D10VertexShader_Name) == 0) {
        create = CreateVertexShaderFromCompiled;
    } else if (NiStricmp(className, NiRTTI_NiD3D10GeometryShader_Name) == 0) {
        create = CreateGeometryShaderFromCompiled;
    } else if (NiStricmp(className, NiRTTI_NiD3D10PixelShader_Name) == 0) {
        create = CreatePixelShaderFromCompiled;
    } else {
        return 0;
    }
    StreamRead(stream, &size, 4, 4);
    if (uint8_t(skip) != 0) {
        Virtual<void>(stream, 2, size);   // Seek on
        return 1;
    }
    re::Ptr32<void> blob = nullptr;
    if (D3D10_CreateBlob(size, &blob) < 0) {
        ComRelease(blob);
        return 0;
    }
    if (blob == nullptr) {
        return 0;
    }
    Stream_LoadBinary(stream, Com<void*>(blob, kGetBufferPointer), size);
    char fileName[0x104];
    NiSprintf(fileName, 0x104, "%s.%s", *name, Virtual<const char*>(self, 9));
    const char* programName = *name;
    re::Ptr32<void> made = nullptr;   // (an NiPointer)
    char* const* profile = Virtual<char* const*>(self, 6);
    create(blob, fileName, 0, 0, kEntryPoint, *profile, 0, programName, &made);
    NiPointer_Assign(program, made);
    NiPointer_Destruct(&made);
    Com<long>(blob, kRelease);
    return 1;
}

// 0x8B8D20 (thiscall): NiD3D10GPUProgramCache's constructor(version, directory, type, profile,
// material, auto append, write debug, locked, load): the cache file's name, the working
// directory, the profile kept (vs_4_0 / gs_4_0 for vertex programs, ps_4_0 for pixel ones);
// loaded when asked and there's a directory
uint8_t* __fastcall GPUProgramCache_Construct(uint8_t* self, re::Edx, uint32_t version, const char* directory,
                                              uint32_t type, const re::Ptr32<char>* profile, const char* material,
                                              int autoAppend, int writeDebug, int locked, int load) {
    ProgramCache_Construct(self, version, autoAppend, locked);
    At<uint32_t>(self, 0) = kProgramCacheVtable;
    At<uint32_t>(self, 0x4C) = type;
    At<uint8_t>(self, 0x50) = uint8_t(writeDebug);
    At<char*>(self, 0x54) = material == nullptr ? static_cast<char*>(kNullFixedString) : StringTable_AddString(material);
    char fileName[0x104];
    NiSprintf(fileName, 0x104, "%s_%s_%s.cache", material, *profile, kPlatform);
    FixedString_AssignChars(&At<char*>(self, 0x28), fileName);
    re::Ptr32<char> handle = directory == nullptr ? static_cast<char*>(kNullFixedString) : StringTable_AddString(directory);
    ProgramCache_SetWorkingDirectory(self, &handle);
    FixedStringRelease(&handle);
    re::Ptr32<char>* current = &At<char*>(self, 0x34);
    FixedString_Assign(current, profile);
    if (type == 0) {
        if (ProfileIs(*current, kVS40)) {
            AddProfileInline(self, kVS40);
        }
        if (ProfileIs(At<char*>(self, 0x34), kGS40)) {
            AddProfileInline(self, kGS40);
        }
    } else if (type == 1) {
        if (ProfileIs(*current, kPS40)) {
            re::Ptr32<char> name = StringTable_AddString(kPS40);
            FixedStringSet_Add(self + 0x38, &name);
            FixedString_Destruct(&name);
        }
    }
    if (uint8_t(load) != 0 && directory != nullptr) {
        ProgramCache_Load(self);
    }
    return self;
}

// 0x8B9010 (thiscall, slot 4): NiD3D10GPUProgramCache::GenerateProgram(name, code, constants): the
// HLSL put in a blob and compiled for the cache's profile (the compile flags with 0x1000 while
// it does), the file name "<dir>\<material>\<name>-V.<ext>" (-G a geometry, -P a pixel shader);
// the source written out when asked; with a program, a descriptor of it and its constants stored
// by name (appended to the file when the cache does that); the constants emptied when there's
// none. Nothing while +0x4A is set, or for no code. (The debug file's path is uninitialized stack
// in the game when the type is none of the three; zeros here.)
void* __fastcall GPUProgramCache_GenerateProgram(uint8_t* self, re::Edx, const char* name, const char* code,
                                                 PrimitiveSet* constants) {
    if (At<uint8_t>(self, 0x4A) != 0 || code == nullptr) {
        return nullptr;
    }
    uint32_t length = uint32_t(std::strlen(code));
    if (length == 0) {
        return nullptr;
    }
    re::Ptr32<void> blob = nullptr;
    D3D10_CreateBlob(length + 1, &blob);
    uint32_t size = Com<uint32_t>(blob, kGetBufferSize);
    crt::StrcpyS(Com<char*>(blob, kGetBufferPointer), size, code);
    uint32_t flags = ProgramFactory_GetCompileFlags();
    ProgramFactory_SetCompileFlags(flags | 0x1000);
    re::Ptr32<void> program = nullptr;   // (an NiPointer)
    char path[0x104] = {};
    auto compile = [&](const char* format, ProgramFromBlob create) {
        char* const* workingDirectory = Virtual<char* const*>(self, 7);
        const char* extension = Virtual<const char*>(self, 9);
        NiSprintf(path, 0x104, format, *workingDirectory, At<char*>(self, 0x54), name, extension);
        re::Ptr32<void> made = nullptr;
        create(blob, path, 0, 0, kEntryPoint, At<char*>(self, 0x34), 0, path, &made);
        NiPointer_Assign(&program, made);
        NiPointer_Destruct(&made);
    };
    if (At<uint32_t>(self, 0x4C) == 0) {
        compile("%s\\%s\\%s-V.%s", CompileVertexShader);
    }
    if (At<uint32_t>(self, 0x4C) == 2) {
        compile("%s\\%s\\%s-G.%s", CompileGeometryShader);
    } else if (At<uint32_t>(self, 0x4C) == 1) {
        compile("%s\\%s\\%s-P.%s", CompilePixelShader);
    }
    if (At<uint8_t>(self, 0x50) != 0) {
        Virtual<uint8_t>(self, 27, path, Com<const char*>(blob, kGetBufferPointer));   // WriteDebugFile
    }
    ProgramFactory_SetCompileFlags(flags);
    Com<long>(blob, kRelease);
    if (program == nullptr) {
        ObjectSet_RemoveAll(constants);
        return nullptr;
    }
    void* memory = NiNew(0x18);
    uint8_t* descriptor = memory != nullptr ? static_cast<uint8_t*>(Descriptor_Construct(memory)) : nullptr;
    for (uint32_t i = 0; i < constants->count; i++) {
        ObjectSet_Add(descriptor + 0xC, &constants->data[i]);
    }
    NiPointer_AssignPointer(descriptor + 8, &program);
    re::Ptr32<char> key = name == nullptr ? static_cast<char*>(kNullFixedString) : StringTable_AddString(name);
    if (At<uint8_t>(self, 0x48) != 0) {
        Virtual<uint8_t>(self, 22, descriptor, &key);   // AppendEntry
    }
    AddRefRef(descriptor);   // (the NiPointer passed by value)
    ProgramMap_SetAt(self + 8, &key, descriptor);
    FixedStringRelease(&key);
    void* result = program;
    if (Decrement(&At<uint32_t>(result, 4)) == 0) {
        Virtual<void>(result, 1);
    }
    return result;
}

#define GB43J_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

GB43J_REVERSED("NiD3D10GPUProgramCache::GetExtension", 0x008B89B0, GPUProgramCache_GetExtension);
GB43J_REVERSED("NiD3D10GPUProgramCache::SaveProgram", 0x008B89C0, GPUProgramCache_SaveProgram);
GB43J_REVERSED("NiD3D10GPUProgramCache::LoadProgram", 0x008B8A60, GPUProgramCache_LoadProgram);
GB43J_REVERSED("NiD3D10GPUProgramCache::NiD3D10GPUProgramCache", 0x008B8D20, GPUProgramCache_Construct);
GB43J_REVERSED("NiD3D10GPUProgramCache::GenerateProgram", 0x008B9010, GPUProgramCache_GenerateProgram);

} // namespace ni::c43
