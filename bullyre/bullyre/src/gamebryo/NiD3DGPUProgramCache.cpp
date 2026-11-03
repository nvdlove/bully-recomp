// NiD3DGPUProgramCache (vtable 0x99D220 on NiGPUProgramCache's, src/gamebryo/NiGPUProgramCache.h;
// 0x58 bytes: +0x4C the program type (0 vertex, 1 pixel), +0x50 write each program's source to a
// debug file, +0x54 the material's name (an NiFixedString)): the DX9 renderer's cache of compiled
// HLSL programs. Its file is "<material>_<profile>_DX9.cache"; the profiles a program compiled
// for one profile can be used with are kept in the base's set at +0x38 (vs_1_1..vs_3_0,
// ps_1_1..ps_3_0: the lower ones). Its slots: 2 GetRTTI (NiGPUProgramCache's NiRTTI), 4
// GenerateProgram, 10 the platform ("DX9"), 25 SaveProgram, 26 LoadProgram, 27 WriteDebugFile.
// Also GPUProgramDescriptor's constructor and deleting destructor (vtable 0x99D290).

#include "gamebryo/NiDX9RendererData.h"
#include "gamebryo/NiGPUProgramCache.h"
#include "reversed.h"

namespace ni::c40 {
namespace {

constexpr uintptr_t kRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t kProgramCacheVtable = 0x0099D220;
constexpr uintptr_t kDescriptorVtable = 0x0099D290;
constexpr uintptr_t kD3DXCreateBuffer = 0x008FC31C;
const char* const kPlatform = reinterpret_cast<const char*>(0x00955DAC);   // "DX9"
const char* const kEntryPoint = reinterpret_cast<const char*>(0x0099D1D0);   // "Main"

GAME_VAR(uint32_t, ms_uiObjects, 0x020E2168);
GAME_VAR(uint8_t, NiRTTI_NiGPUProgramCache, 0x020E9624);
GAME_VAR(const char*, NiRTTI_NiD3DHLSLVertexShader_Name, 0x020E74D0);   // (the NiRTTI's name)
GAME_VAR(const char*, NiRTTI_NiD3DHLSLPixelShader_Name, 0x020E749C);
GAME_VAR(void*, ms_pkShaderProgramCreator, 0x020E7498);   // the HLSL program creator

// the profile names, at the game's addresses (the code compares and adds these)
const char* const kVS11 = reinterpret_cast<const char*>(0x0099D208);
const char* const kVS20 = reinterpret_cast<const char*>(0x0094EA78);
const char* const kVS2A = reinterpret_cast<const char*>(0x0099B150);
const char* const kVS30 = reinterpret_cast<const char*>(0x0099B158);
const char* const kPS11 = reinterpret_cast<const char*>(0x0099D200);
const char* const kPS12 = reinterpret_cast<const char*>(0x0099D1F8);
const char* const kPS13 = reinterpret_cast<const char*>(0x0099D1F0);
const char* const kPS14 = reinterpret_cast<const char*>(0x0099D1E8);
const char* const kPS20 = reinterpret_cast<const char*>(0x0094DA54);
const char* const kPS2A = reinterpret_cast<const char*>(0x0099D1E0);
const char* const kPS2B = reinterpret_cast<const char*>(0x0099D1D8);
const char* const kPS30 = reinterpret_cast<const char*>(0x00955DA4);

GAME_FN(void*, __thiscall, ProgramCache_Construct, 0x008CBCD0, void* self, uint32_t version, int autoAppend,
        int locked);
GAME_FN(void, __thiscall, ProgramCache_Destruct, 0x008CBB90, void* self);
GAME_FN(void, __thiscall, ProgramCache_SetWorkingDirectory, 0x008CAD00, void* self, const re::Ptr32<char>* directory);
GAME_FN(uint8_t, __thiscall, ProgramCache_Load, 0x008CB1C0, void* self);
GAME_FN(uint8_t, __thiscall, ProgramCache_EnsureDirectory, 0x008CA510, void* self);
GAME_FN(void, __thiscall, ProgramCache_SaveString, 0x008CAF00, void* self, void* stream, const char* text);
GAME_FN(void, __thiscall, ProgramCache_LoadString, 0x008CAFA0, void* self, void* stream, char* buffer,
        uint32_t size);
GAME_FN(void, __thiscall, ProgramMap_SetAt, 0x008B9E50, void* map, const re::Ptr32<char>* key, void* descriptor); // (a
                                                                                                          //  NiPointer
                                                                                                          //  by value)
GAME_FN(void, __thiscall, FixedString_AssignChars, 0x00406730, re::Ptr32<char>* self, const char* text);
GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, const re::Ptr32<char>* other);
GAME_FN(void, __thiscall, FixedString_Construct, 0x00401050, re::Ptr32<char>* self, const char* text);
GAME_FN(void, __thiscall, FixedString_Destruct, 0x00517750, re::Ptr32<char>* self);
GAME_FN(uint8_t, __thiscall, FixedString_EqualsNoCase, 0x004064E0, const re::Ptr32<char>* self, const char* text);
GAME_FN(char*, __cdecl, StringTable_AddString, 0x00754760, const char* text);
GAME_FN(void, __thiscall, FixedStringSet_Add, 0x00848D60, void* set, const re::Ptr32<char>* value);
GAME_FN(void, __thiscall, FixedStringSet_Realloc, 0x00847E40, void* set, uint32_t size);
GAME_FN(void, __thiscall, ObjectSet_Add, 0x00787BE0, void* set, const re::Ptr32<void>* element);
GAME_FN(void, __thiscall, ObjectSet_RemoveAll, 0x00836940, void* set);
GAME_FN(void, __cdecl, NiDeleteObjectArray, 0x00836F80, void* p);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(int, __cdecl, NiStricmp, 0x00753820, const char* a, const char* b);
GAME_FN(void, __cdecl, Path_Standardize, 0x00752C20, char* path);
GAME_FN(uint8_t, __cdecl, File_DirectoryExists, 0x00753440, const char* path);
GAME_FN(uint8_t, __cdecl, File_CreateDirectoryRecursive, 0x007534E0, const char* path);
GAME_FN(void*, __cdecl, File_GetFile, 0x00753380, const char* name, int mode, uint32_t bufferSize);
GAME_FN(void, __thiscall, NiPointer_Assign, 0x00800420, void* slot, void* object);
// NiD3DShaderProgramFactory's (HLSL compiles from a buffer; the instance; the program maps)
GAME_FN(void*, __cdecl, ProgramFactory_CreateVertexShaderFromBuffer, 0x008A23D0, const char* code, uint32_t size,
        const char* fileName, const char* name, const char* entry, const char* profile, void* declaration,
        uint32_t usage, uint32_t flags, int recoverable);
GAME_FN(void*, __cdecl, ProgramFactory_CreatePixelShaderFromBuffer, 0x008A2460, const char* code, uint32_t size,
        const char* fileName, const char* name, const char* entry, const char* profile, int recoverable);
GAME_FN(void*, __cdecl, ProgramFactory_GetInstance, 0x008A1EE0);
GAME_FN(void, __thiscall, ProgramFactory_InsertVertexShader, 0x008A21C0, void* self, void* program);
GAME_FN(void, __thiscall, ProgramFactory_InsertPixelShader, 0x008A21F0, void* self, void* program);
// this file's (called where modes apply)
GAME_FN(void*, __thiscall, Descriptor_Construct, 0x00898D80, void* self);

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
inline void ReleaseObject(NiRefObject* object) {
    if (object != nullptr && Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}
// a profile name added to the set (NiTObjectSet<NiFixedString>::Add through its own code)
inline void AddProfile(uint8_t* self, const char* name) {
    re::Ptr32<char> handle;
    FixedString_Construct(&handle, name);
    FixedStringSet_Add(self + 0x38, &handle);
    FixedString_Destruct(&handle);
}
// the profile compared as the code inlines it (the same characters, or no case difference)
inline bool ProfileIs(const char* profile, const char* name) {
    return profile == name || (profile != nullptr && NiStricmp(profile, name) == 0);
}

} // namespace

// 0x898300 (thiscall): NiD3DGPUProgramCache::GetPlatform ("DX9")
const char* __fastcall NiD3DGPUProgramCache_GetPlatform(void*, re::Edx) {
    return kPlatform;
}

// 0x898310 (thiscall): NiD3DGPUProgramCache::SaveProgram (its NiRTTI's name, its code's size and
// code)
uint8_t __fastcall NiD3DGPUProgramCache_SaveProgram(uint8_t* self, re::Edx, void* stream, void* program) {
    if (program == nullptr) {
        return 0;
    }
    const char* const* rtti = Virtual<const char* const*>(program, 2);
    ProgramCache_SaveString(self, stream, *rtti);
    uint32_t size = Virtual<uint32_t>(program, 9);   // GetCodeSize
    const void* code = Virtual<const void*>(program, 10);   // GetCode
    StreamWrite(stream, &size, 4, 4);
    StreamWrite(stream, code, size, 1);
    return 1;
}

// 0x8983A0 (thiscall): NiD3DGPUProgramCache::LoadProgram (a vertex or pixel shader by the saved
// class name, made from its code by the HLSL creator and put in the factory's maps; skipped over
// when asked; false for an unknown class or no buffer)
uint8_t __fastcall NiD3DGPUProgramCache_LoadProgram(uint8_t* self, re::Edx, void* stream, const re::Ptr32<char>* name,
                                                    re::Ptr32<NiRefObject>* program, int skip) {
    ReleaseObject(*program);
    *program = nullptr;
    uint32_t size = 0;
    char className[0x100];
    ProgramCache_LoadString(self, stream, className, 0x100);
    bool vertex;
    if (NiStricmp(className, NiRTTI_NiD3DHLSLVertexShader_Name) == 0) {
        vertex = true;
    } else if (NiStricmp(className, NiRTTI_NiD3DHLSLPixelShader_Name) == 0) {
        vertex = false;
    } else {
        return 0;
    }
    StreamRead(stream, &size, 4, 4);
    if (uint8_t(skip) != 0) {
        Virtual<void>(stream, 2, size);   // Seek on
        return 1;
    }
    void* buffer = nullptr;
    if (Import<long>(kD3DXCreateBuffer, size, &buffer) < 0) {
        return 0;
    }
    StreamRead(stream, Com<void*>(buffer, 3), size, 1);   // ID3DXBuffer::GetBufferPointer
    void* creator = ms_pkShaderProgramCreator;
    char* const* codeId = Virtual<char* const*>(self, 6);
    if (vertex) {
        void* made = Virtual<void*>(creator, 9, buffer, size, *name, kEntryPoint, *codeId, nullptr, 0u, 0u, 0u);
        NiPointer_Assign(program, made);
        if (*program != nullptr) {
            ProgramFactory_InsertVertexShader(ProgramFactory_GetInstance(), *program);
        }
    } else {
        void* made = Virtual<void*>(creator, 10, buffer, size, *name, kEntryPoint, *codeId, 0u);
        NiPointer_Assign(program, made);
        if (*program != nullptr) {
            ProgramFactory_InsertPixelShader(ProgramFactory_GetInstance(), *program);
        }
    }
    Com<long>(buffer, 2);   // Release
    return 1;
}

// 0x8985B0 (thiscall): NiD3DGPUProgramCache::WriteDebugFile (the program's source next to the
// cache, its directory made when it needs to be)
uint8_t __fastcall NiD3DGPUProgramCache_WriteDebugFile(uint8_t* self, re::Edx, const char* path, const char* text) {
    if (ProgramCache_EnsureDirectory(self) == 0) {
        return 0;
    }
    char directory[0x104];
    crt::StrcpyS(directory, sizeof(directory), path);
    Path_Standardize(directory);
    char* slash = crt::Strrchr(directory, '/');
    if (slash != nullptr || (slash = crt::Strrchr(directory, '\\')) != nullptr) {
        *slash = 0;
    }
    if (File_DirectoryExists(directory) == 0 && File_CreateDirectoryRecursive(directory) == 0) {
        return 0;
    }
    void* file = File_GetFile(path, 1, 0x8000);
    if (file == nullptr) {
        return 0;
    }
    StreamWrite(file, text, uint32_t(std::strlen(text)), 1);
    Virtual<void*>(file, 0, 1);   // the deleting destructor
    return 1;
}

// 0x8986C0 (thiscall): NiD3DGPUProgramCache's constructor (the cache file's name from the material,
// profile and platform; the working directory; the profiles programs compiled for the given one can
// run on (ps_2_b's list leaves out ps_2_a, as the original's does); loaded when asked and there's a
// directory)
void* __fastcall NiD3DGPUProgramCache_Construct(uint8_t* self, re::Edx, uint32_t version, const char* directory,
                                                uint32_t type, const re::Ptr32<char>* profile, const char* material,
                                                int autoAppend, int writeDebug, int locked, int load) {
    ProgramCache_Construct(self, version, autoAppend, locked);
    At<uint32_t>(self, 0) = kProgramCacheVtable;
    At<char*>(self, 0x54) = ni::sys::kNullFixedString;
    At<uint8_t>(self, 0x50) = uint8_t(writeDebug);
    At<uint32_t>(self, 0x4C) = type;
    FixedString_AssignChars(&At<char*>(self, 0x54), material);
    char fileName[0x104];
    NiSprintf(fileName, 0x104, "%s_%s_%s.cache", material, *profile, kPlatform);
    FixedString_AssignChars(&At<char*>(self, 0x28), fileName);
    re::Ptr32<char> handle = directory != nullptr ? StringTable_AddString(directory) : static_cast<char*>(ni::sys::kNullFixedString);
    ProgramCache_SetWorkingDirectory(self, &handle);
    ni::sys::FixedStringRelease(&handle);
    re::Ptr32<char>* current = &At<char*>(self, 0x34);
    FixedString_Assign(current, profile);
    if (type == 0) {
        if (ProfileIs(*current, kVS11)) {
            // (inlined: NiTObjectSet<NiFixedString>::Add)
            re::Ptr32<char> name = StringTable_AddString(kVS11);
            uint32_t alloced = At<uint32_t>(self, 0x3C);
            if (At<uint32_t>(self, 0x40) == alloced) {
                FixedStringSet_Realloc(self + 0x38, alloced > 0 ? alloced * 2 : 1);
            }
            uint32_t count = At<uint32_t>(self, 0x40);
            re::Ptr32<char>* slot = &At<re::Ptr32<char>*>(self, 0x38)[count];
            At<uint32_t>(self, 0x40) = count + 1;
            FixedString_Assign(slot, &name);
            ni::sys::FixedStringRelease(&name);
        } else if (ProfileIs(*current, kVS20)) {
            re::Ptr32<char> name = StringTable_AddString(kVS20);
            FixedStringSet_Add(self + 0x38, &name);
            FixedString_Destruct(&name);
            name = StringTable_AddString(kVS11);
            FixedStringSet_Add(self + 0x38, &name);
            FixedString_Destruct(&name);
        } else if (FixedString_EqualsNoCase(current, kVS2A) != 0) {
            AddProfile(self, kVS2A);
            AddProfile(self, kVS20);
            AddProfile(self, kVS11);
        } else if (FixedString_EqualsNoCase(current, kVS30) != 0) {
            AddProfile(self, kVS30);
            AddProfile(self, kVS2A);
            AddProfile(self, kVS20);
            AddProfile(self, kVS11);
        }
    } else if (type == 1) {
        if (ProfileIs(*current, kPS11)) {
            re::Ptr32<char> name = StringTable_AddString(kPS11);
            FixedStringSet_Add(self + 0x38, &name);
            FixedString_Destruct(&name);
        } else {
            const char* chain[8];
            int n = 0;
            if (FixedString_EqualsNoCase(current, kPS12) != 0) {
                chain[n++] = kPS12;
            } else if (FixedString_EqualsNoCase(current, kPS13) != 0) {
                chain[n++] = kPS13;
                chain[n++] = kPS12;
            } else if (FixedString_EqualsNoCase(current, kPS14) != 0) {
                chain[n++] = kPS14;
                chain[n++] = kPS13;
                chain[n++] = kPS12;
            } else if (FixedString_EqualsNoCase(current, kPS20) != 0) {
                chain[n++] = kPS20;
                chain[n++] = kPS14;
                chain[n++] = kPS13;
                chain[n++] = kPS12;
            } else if (FixedString_EqualsNoCase(current, kPS2A) != 0) {
                chain[n++] = kPS2A;
                chain[n++] = kPS20;
                chain[n++] = kPS14;
                chain[n++] = kPS13;
                chain[n++] = kPS12;
            } else if (FixedString_EqualsNoCase(current, kPS2B) != 0) {
                chain[n++] = kPS2B;
                chain[n++] = kPS20;
                chain[n++] = kPS14;
                chain[n++] = kPS13;
                chain[n++] = kPS12;
            } else if (FixedString_EqualsNoCase(current, kPS30) != 0) {
                chain[n++] = kPS30;
                chain[n++] = kPS2A;
                chain[n++] = kPS20;
                chain[n++] = kPS14;
                chain[n++] = kPS13;
                chain[n++] = kPS12;
            }
            if (n != 0) {
                for (int i = 0; i < n; i++) {
                    AddProfile(self, chain[i]);
                }
                AddProfile(self, kPS11);
            }
        }
    }
    if (uint8_t(load) != 0 && directory != nullptr) {
        ProgramCache_Load(self);
    }
    return self;
}

// 0x898D10 (thiscall): NiD3DGPUProgramCache::GetRTTI (NiGPUProgramCache's)
void* __fastcall NiD3DGPUProgramCache_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiGPUProgramCache;
}

// 0x898D20 (thiscall): NiD3DGPUProgramCache's deleting destructor
void* __fastcall NiD3DGPUProgramCache_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    ni::sys::FixedStringRelease(&At<char*>(self, 0x54));
    ProgramCache_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x58);
    }
    return self;
}

// 0x898D80 (thiscall): GPUProgramDescriptor's constructor
void* __fastcall GPUProgramDescriptor_Construct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kRefObjectVtable;
    At<uint32_t>(self, 4) = 0;
    Increment(&ms_uiObjects);
    At<uint32_t>(self, 0) = kDescriptorVtable;
    At<void*>(self, 8) = nullptr;
    At<void*>(self, 0xC) = nullptr;
    At<uint32_t>(self, 0x10) = 0;
    At<uint32_t>(self, 0x14) = 0;
    return self;
}

// 0x898DC0 (thiscall): GPUProgramDescriptor's deleting destructor (its constants, its program)
void* __fastcall GPUProgramDescriptor_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    ObjectSet_RemoveAll(self + 0xC);
    NiDeleteObjectArray(At<void*>(self, 0xC));
    ReleaseObject(At<NiRefObject*>(self, 8));
    At<uint32_t>(self, 0) = kRefObjectVtable;
    Decrement(&ms_uiObjects);
    if (flags & 1) {
        NiDelete(self, 0x18);
    }
    return self;
}

// 0x898E30 (thiscall): NiD3DGPUProgramCache::GenerateProgram (the HLSL compiled for its profile,
// the file name "<dir>\<material>\<name>-V.<ext>" (-P a pixel shader) given as #line 2; the source
// written out when asked; with the constants it uses kept, a descriptor stored by name (appended to
// the file when the cache does that). Its constants emptied when there's no program; nothing while
// +0x4A is set)
void* __fastcall NiD3DGPUProgramCache_GenerateProgram(uint8_t* self, re::Edx, const char* name, const char* code,
                                                      PrimitiveSet* constants) {
    if (At<uint8_t>(self, 0x4A) != 0) {
        return nullptr;
    }
    NiRefObject* program = nullptr;   // (an NiPointer)
    uint32_t size = uint32_t(std::strlen(code)) + 0x208;
    char* source = static_cast<char*>(NiMalloc(size));
    char path[0x104];
    path[0] = 0;   // (the original's is uninitialized stack when the type is neither)
    uint32_t type = At<uint32_t>(self, 0x4C);
    if (type == 0 || type == 1) {
        char* const* directory = Virtual<char* const*>(self, 7);   // the working directory
        const char* extension = Virtual<const char*>(self, 9);
        NiSprintf(path, 0x104, type == 0 ? "%s\\%s\\%s-V.%s" : "%s\\%s\\%s-P.%s", *directory, At<char*>(self, 0x54),
                  name, extension);
        NiSprintf(source, size, "#line 2 \"%s\"\n%s", path, code);
        uint32_t length = uint32_t(std::strlen(source));
        void* made;
        if (type == 0) {
            made = ProgramFactory_CreateVertexShaderFromBuffer(source, length, path, path, kEntryPoint,
                                                               At<char*>(self, 0x34), nullptr, 0, 0, 1);
        } else {
            made = ProgramFactory_CreatePixelShaderFromBuffer(source, length, path, path, kEntryPoint,
                                                              At<char*>(self, 0x34), 1);
        }
        NiPointer_Assign(&program, made);
    }
    if (At<uint8_t>(self, 0x50) != 0) {
        Virtual<uint8_t>(self, 27, path, source);   // WriteDebugFile
    }
    if (program != nullptr && Virtual<uint8_t>(self, 20, program, constants) != 0) {   // RemoveUnusedConstants
        void* memory = NiNew(0x18);
        uint8_t* descriptor = memory != nullptr ? static_cast<uint8_t*>(Descriptor_Construct(memory)) : nullptr;
        for (uint32_t i = 0; i < constants->count; i++) {
            ObjectSet_Add(descriptor + 0xC, &constants->data[i]);
        }
        re::Ptr32<NiRefObject>& held = At<NiRefObject*>(descriptor, 8);
        if (held != program) {
            ReleaseObject(held);
            held = program;
            Increment(&program->refCount);
        }
        re::Ptr32<char> key = name != nullptr ? StringTable_AddString(name) : static_cast<char*>(ni::sys::kNullFixedString);
        if (At<uint8_t>(self, 0x48) != 0) {
            Virtual<uint8_t>(self, 22, descriptor, &key);   // AppendEntry
        }
        if (descriptor != nullptr) {
            Increment(descriptor + 4);   // (the NiPointer passed by value)
        }
        ProgramMap_SetAt(self + 8, &key, descriptor);
        ni::sys::FixedStringRelease(&key);
    }
    NiFree(source);
    if (program == nullptr) {
        ObjectSet_RemoveAll(constants);
        return nullptr;
    }
    ReleaseObject(program);
    return program;
}

#define C40K_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C40K_REVERSED("NiD3DGPUProgramCache::GetPlatform", 0x00898300, NiD3DGPUProgramCache_GetPlatform);
C40K_REVERSED("NiD3DGPUProgramCache::SaveProgram", 0x00898310, NiD3DGPUProgramCache_SaveProgram);
C40K_REVERSED("NiD3DGPUProgramCache::LoadProgram", 0x008983A0, NiD3DGPUProgramCache_LoadProgram);
C40K_REVERSED("NiD3DGPUProgramCache::WriteDebugFile", 0x008985B0, NiD3DGPUProgramCache_WriteDebugFile);
C40K_REVERSED("NiD3DGPUProgramCache::NiD3DGPUProgramCache", 0x008986C0, NiD3DGPUProgramCache_Construct);
C40K_REVERSED("NiD3DGPUProgramCache::GetRTTI", 0x00898D10, NiD3DGPUProgramCache_GetRTTI);
C40K_REVERSED("NiD3DGPUProgramCache::DeletingDestructor", 0x00898D20, NiD3DGPUProgramCache_DeletingDestructor);
C40K_REVERSED("NiGPUProgramDescriptor::NiGPUProgramDescriptor", 0x00898D80, GPUProgramDescriptor_Construct);
C40K_REVERSED("NiGPUProgramDescriptor::DeletingDestructor", 0x00898DC0, GPUProgramDescriptor_DeletingDestructor);
C40K_REVERSED("NiD3DGPUProgramCache::GenerateProgram", 0x00898E30, NiD3DGPUProgramCache_GenerateProgram);

} // namespace ni::c40
