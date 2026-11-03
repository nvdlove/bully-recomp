// The assembled and compiled shader program creators and the D3D shader programs' own code:
//
// NiD3DShaderProgramCreatorAsm (vtable 0x99DB88: ".vsh" / ".psh", assembled with D3DX) and
// NiD3DShaderProgramCreatorObj (0x99DBBC: ".vso" / ".pso", the compiled code read as it is), 0x10
// bytes (+0x4 the creation flags, or'd with NiD3DShaderProgramFactory's universal ones 0x20E6EA0).
// Their slots (NiD3DShaderProgramCreator's order, src/gamebryo/NiD3DShaderProgram.cpp has the HLSL
// one's): 3/4 CreateVertexShaderFromFile / FromBuffer, 5/6 the pixel ones, 7/8 RecreateVertexShader
// / RecreatePixelShader (both creators share them), 9 LoadShaderCodeFromFile(file, &code, &size,
// two outputs only zeroed, recoverable), 10 LoadShaderCodeFromBuffer(buffer, size, &code, &size,
// the two, recoverable). The code is NiMalloc'd; the program keeps it (SetCode).
//
// NiD3DShaderProgram (vtable 0x99E350, 0x2C bytes, NiRTTI 0x20E7518 "NiD3DShaderProgram" on
// NiGPUProgram's; layout in src/gamebryo/NiD3DShaderProgram.cpp): its constructor, renderer,
// destructor; NiD3DPixelShader (0x99DD00, 0x30) and NiD3DVertexShader (0x99DD50, 0x3C): their
// SetShaderConstant / SetShaderConstantArray (slots 14/15: booleans into the shader constant
// manager's own arrays, integers and floats through its setters), Destroy- / RecreateRendererData
// (pixel 18/19, vertex 24/25), destructors. The program slots the creators call: 6 SetName, 8
// SetShaderProgramName, 10 GetCode, 11 SetCode(size, code), 13 SetCreator; the pixel shader's 17
// SetShaderHandle; the vertex shader's 16/17 Get/SetUsage, 19 SetShaderHandle, 20/21 Get/Set
// VertexDeclaration, 23 SetSoftwareVertexProcessing.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::c42 {
namespace {

using dx9::Com;
using sys::At;
using sys::Import;
using sys::Virtual;

constexpr uintptr_t kRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t kProgramVtable = 0x0099E350;
constexpr uintptr_t kPixelShaderVtable = 0x0099DD00;
constexpr uintptr_t kVertexShaderVtable = 0x0099DD50;
constexpr uintptr_t kProgramRTTI = 0x020E7518;

// the strings the errors are reported with (the game's)
const char* const kFailedVertexShader = reinterpret_cast<const char*>(0x0099DEA0);   // "Failed CreateVertexShader call on %s\n"
const char* const kFailedPixelShader = reinterpret_cast<const char*>(0x0099DEC8);    // "Failed CreatePixelShader call on %s\n"
const char* const kInvalidFileName = reinterpret_cast<const char*>(0x0099DEF0);      // "Invalid shader file name\n"
const char* const kFileNotFound = reinterpret_cast<const char*>(0x0099DF80);         // "Failed to find shader program file %s\n"
const char* const kInvalidBuffer = reinterpret_cast<const char*>(0x0099DFA8);        // "Invalid shader buffer\n"
const char* const kAssembleNone = reinterpret_cast<const char*>(0x0099E20C);         // "Failed to assemble shader %s\nError: NONE REPORTED\n"
const char* const kAssembleError = reinterpret_cast<const char*>(0x0099E240);        // "Failed to assemble shader %s\nError: %s\n"
const char* const kAssembleMemoryNone = reinterpret_cast<const char*>(0x0099E268);   // "...from memory\nError: NONE REPORTED\n"
const char* const kAssembleMemoryError = reinterpret_cast<const char*>(0x0099E2A4);  // "...from memory\nError: %s\n"
const char* const kInvalidFile = reinterpret_cast<const char*>(0x0099E2D8);          // "Invalid shader file %s\n"
const char* const kCreateVertexFailed = reinterpret_cast<const char*>(0x0099E2F0);   // (the device's CreateVertexShader, HRESULT)
const char* const kCreatePixelFailed = reinterpret_cast<const char*>(0x0099E320);

// D3DX and Win32 through the game's import slots
constexpr uintptr_t kD3DXAssembleShader = 0x008FC2B4;
constexpr uintptr_t kD3DXAssembleShaderFromFileA = 0x008FC2B8;
constexpr uintptr_t kInterlockedIncrement = 0x008FC174;
constexpr uintptr_t kInterlockedDecrement = 0x008FC170;
constexpr uintptr_t kCloseHandle = 0x008FC050;
constexpr uintptr_t kCreateFileA = 0x008FC064;
constexpr uintptr_t kReadFile = 0x008FC06C;
constexpr uintptr_t kGetFileSize = 0x008FC098;

// COM slots: IUnknown, ID3DXBuffer (3 GetBufferPointer, 4 GetBufferSize), IDirect3DDevice9
constexpr int kAddRef = 1;
constexpr int kRelease = 2;
constexpr int kGetBufferPointer = 3;
constexpr int kGetBufferSize = 4;
constexpr int kDevCreateVertexShader = 91;
constexpr int kDevCreatePixelShader = 106;

GAME_VAR(uint32_t, ms_uiObjects, 0x020E2168);                       // NiRefObject's count
GAME_VAR(uint8_t*, ms_pkCreatorRenderer, 0x020E6EA8);               // NiD3DShaderProgramFactory's
GAME_VAR(void*, ms_pkCreatorDevice, 0x020E6EA4);
GAME_VAR(uint32_t, ms_uiUniversalShaderCreationFlags, 0x020E6EA0);
GAME_ARRAY(uint32_t, ms_auiAttributeTypes, 0x020E408C);              // by the entry's flags' low byte

GAME_FN(uint32_t, __cdecl, ShaderFactory_ReportError, 0x00756700, int error, int recoverable, const char* format,
        ...);
GAME_FN(void, __cdecl, NiStrcpy, 0x00747280, char* dest, uint32_t size, const char* src);
GAME_FN(uint8_t, __cdecl, ProgramFactory_ResolveProgramFileName, 0x008A27D0, const char* name, char* out,
        size_t size);
GAME_FN(uint8_t, __cdecl, ProgramFactory_RecreatePixelShader, 0x008A1F50, void* program);
GAME_FN(uint8_t, __cdecl, ProgramFactory_RecreateVertexShader, 0x008A1F10, void* program);
GAME_FN(void, __cdecl, ProgramFactory_ReleaseVertexShader, 0x008A24E0, void* program);
GAME_FN(void, __cdecl, ProgramFactory_ReleasePixelShader, 0x008A2510, void* program);
GAME_FN(void, __thiscall, Renderer_ReleaseVertexShader, 0x0087A230, void* renderer, void* program);
GAME_FN(void, __thiscall, Renderer_ReleasePixelShader, 0x0087A290, void* renderer, void* program);
GAME_FN(uint8_t, __thiscall, ConstantManager_SetVertexShaderConstantF, 0x0074D2C0, void* self, uint32_t start,
        const void* data, uint32_t count);
GAME_FN(uint8_t, __thiscall, ConstantManager_SetVertexShaderConstantI, 0x0074D310, void* self, uint32_t start,
        const void* data, uint32_t count);
GAME_FN(uint8_t, __thiscall, ConstantManager_SetPixelShaderConstantF, 0x0074D360, void* self, uint32_t start,
        const void* data, uint32_t count);
GAME_FN(uint8_t, __thiscall, ConstantManager_SetPixelShaderConstantI, 0x0074D3B0, void* self, uint32_t start,
        const void* data, uint32_t count);
// this file's (called where modes apply)
GAME_FN(void*, __thiscall, Creator_CreateVertexShader, 0x008A6F80, void* self, const void* code, void* declaration,
        uint32_t usage, uint32_t a4, uint32_t a5, int softwareVP);
GAME_FN(void*, __thiscall, Creator_CreatePixelShader, 0x008A6FC0, void* self, const void* code);
GAME_FN(void, __thiscall, Program_SetD3DRenderer, 0x008A7000, void* self, void* renderer);
GAME_FN(void*, __thiscall, Program_Construct, 0x008A7070, void* self, void* renderer, uint32_t kind);
GAME_FN(void, __thiscall, Program_Destruct, 0x008A70E0, void* self);

inline void ComRelease(void* object) {
    if (object != nullptr) {
        Com<long>(object, kRelease);
    }
}

// a new vertex / pixel shader (null when the allocation failed)
uint8_t* NewVertexShader() {
    auto* program = static_cast<uint8_t*>(NiNew(0x3C));
    if (program == nullptr) {
        return nullptr;
    }
    Program_Construct(program, ms_pkCreatorRenderer, 0);
    At<uint32_t>(program, 0) = kVertexShaderVtable;
    At<uint8_t>(program, 0x2C) = 0;
    At<uint32_t>(program, 0x30) = 0;
    At<void*>(program, 0x34) = nullptr;
    At<void*>(program, 0x38) = nullptr;
    return program;
}
uint8_t* NewPixelShader() {
    auto* program = static_cast<uint8_t*>(NiNew(0x30));
    if (program == nullptr) {
        return nullptr;
    }
    Program_Construct(program, ms_pkCreatorRenderer, 1);
    At<uint32_t>(program, 0) = kPixelShaderVtable;
    At<void*>(program, 0x2C) = nullptr;
    return program;
}

// a shader assembled by D3DX (from a file or memory): the code copied out on success, the error
// buffer's text reported otherwise
struct AssembleResult {
    void* shader;
    void* errors;
};

bool FinishAssembly(long hr, AssembleResult& result, re::Ptr32<void>* code, uint32_t* size, int recoverable,
                    const char* noneFormat, const char* errorFormat, const char* fileName) {
    if (hr < 0) {
        char* text = nullptr;
        if (result.errors != nullptr) {
            auto* message = Com<const char*>(result.errors, kGetBufferPointer);
            if (message != nullptr) {
                uint32_t length = Com<uint32_t>(result.errors, kGetBufferSize);
                text = static_cast<char*>(NiMalloc(length));
                NiStrcpy(text, length, message);
                if (fileName != nullptr) {
                    ShaderFactory_ReportError(1, recoverable, errorFormat, fileName, text);
                } else {
                    ShaderFactory_ReportError(1, recoverable, errorFormat, text);
                }
            }
            Com<long>(result.errors, kRelease);
        } else if (fileName != nullptr) {
            ShaderFactory_ReportError(1, recoverable, noneFormat, fileName);
        } else {
            ShaderFactory_ReportError(1, recoverable, noneFormat);
        }
        NiFree(text);
        ComRelease(result.shader);
        return false;
    }
    *size = Com<uint32_t>(result.shader, kGetBufferSize);
    *code = NiMalloc(*size);
    uint32_t bytes = *size;
    crt::MemcpyS(*code, bytes, Com<const void*>(result.shader, kGetBufferPointer), bytes);
    Com<long>(result.shader, kRelease);
    ComRelease(result.errors);
    return true;
}

// a SetShaderConstant's booleans (into the constant manager's array at `values`, its dirty range
// at `dirty`, its size at `limit`)
uint8_t SetBooleans(uint8_t* manager, int values, int dirty, int limit, uint32_t start, const void* data,
                    uint32_t count) {
    uint32_t end = start + count;
    if (end > At<uint32_t>(manager, limit)) {
        return 0;
    }
    crt::MemcpyS(At<uint8_t*>(manager, values) + start * 4, count * 4, data, count * 4);
    if (start < At<uint32_t>(manager, dirty)) {
        At<uint32_t>(manager, dirty) = start;
    }
    if (At<uint32_t>(manager, dirty + 4) < end) {
        At<uint32_t>(manager, dirty + 4) = end;
    }
    return 1;
}

// SetShaderConstantArray's copy: each element's registers (remapped by the index array when
// there is one) into the manager's float array, the ones that fit; true always
uint8_t SetArray(uint8_t* self, int values, int dirty, int limit, const uint8_t* entry, const uint8_t* data,
                 uint32_t count, uint32_t registers, const uint16_t* remapping) {
    uint32_t start = At<uint32_t>(entry, 0x18);
    uint32_t bytes = registers * 16;
    uint32_t to = start * 16;
    uint32_t end = registers + start;
    for (uint32_t i = 0; i < count; i++, to += bytes, end += registers, start += registers) {
        uint32_t from = remapping == nullptr ? (i & 0xFFFF) : remapping[i];
        uint8_t* manager = At<uint8_t*>(At<uint8_t*>(self, 0x28), 0x20F0);
        if (end <= At<uint32_t>(manager, limit)) {
            crt::MemcpyS(At<uint8_t*>(manager, values) + to, bytes, data + from * registers * 16, bytes);
            if (start < At<uint32_t>(manager, dirty)) {
                At<uint32_t>(manager, dirty) = start;
            }
            if (At<uint32_t>(manager, dirty + 4) < end) {
                At<uint32_t>(manager, dirty + 4) = end;
            }
        }
    }
    return 1;
}

} // namespace

// ---- the creators

// 0x8A64B0 (thiscall): NiD3DShaderProgramCreatorAsm::RecreateVertexShader (also Obj's: the D3D
// shader made again from the program's code, declaration and usage)
uint8_t __fastcall NiD3DShaderProgramCreatorAsm_RecreateVertexShader(uint8_t* self, re::Edx, uint8_t* program) {
    void* code = Virtual<void*>(program, 10);
    if (code == nullptr) {
        return 0;
    }
    void* declaration = Virtual<void*>(program, 20);
    uint32_t usage = Virtual<uint32_t>(program, 16);
    void* shader = Creator_CreateVertexShader(self, code, &declaration, usage, 0, 0, 0);
    if (shader == nullptr) {
        return 0;
    }
    Virtual<void>(program, 19, shader);
    return 1;
}

// 0x8A6510 (thiscall): NiD3DShaderProgramCreatorAsm::RecreatePixelShader (also Obj's)
uint8_t __fastcall NiD3DShaderProgramCreatorAsm_RecreatePixelShader(uint8_t* self, re::Edx, uint8_t* program) {
    void* code = Virtual<void*>(program, 10);
    if (code == nullptr) {
        return 0;
    }
    void* shader = Creator_CreatePixelShader(self, code);
    if (shader == nullptr) {
        return 0;
    }
    Virtual<void>(program, 17, shader);
    return 1;
}

namespace {

// the vertex shaders' common end (the program filled in, or deleted when the device fails)
uint8_t* FinishVertexShader(uint8_t* self, uint8_t* program, void* code, uint32_t size, uint32_t out3, uint32_t out4,
                            const char* fileName, const char* name, void* declaration, uint32_t usage,
                            int softwareVP, int recoverable) {
    void* shader = Creator_CreateVertexShader(self, code, &declaration, usage, out3, out4, softwareVP);
    if (shader == nullptr) {
        ShaderFactory_ReportError(1, recoverable, kFailedVertexShader, name);
        Virtual<void*>(program, 0, 1);
        return nullptr;
    }
    Virtual<void>(program, 6, name);
    Virtual<void>(program, 8, fileName);
    Virtual<void>(program, 11, size, code);
    Virtual<void>(program, 13, self);
    Virtual<void>(program, 19, shader);
    Virtual<void>(program, 17, usage);
    Virtual<void>(program, 21, declaration);
    Virtual<void>(program, 23, softwareVP);
    return program;
}

} // namespace

// 0x8A6550 (thiscall): NiD3DShaderProgramCreatorAsm::CreateVertexShaderFromFile (also Obj's)
uint8_t* __fastcall NiD3DShaderProgramCreatorAsm_CreateVertexShaderFromFile(
    uint8_t* self, void*, const char* fileName, const char* name, const char*, const char*, void* declaration,
    uint32_t usage, int softwareVP, int recoverable) {
    uint8_t* program = NewVertexShader();
    if (program == nullptr) {
        return nullptr;
    }
    void* code = nullptr;
    uint32_t out4 = 0;
    uint32_t out3 = 0;
    uint32_t size = 0;
    if (Virtual<uint8_t>(self, 9, fileName, &code, &size, &out3, &out4, recoverable) == 0) {
        Virtual<void*>(program, 0, 1);
        return nullptr;
    }
    return FinishVertexShader(self, program, code, size, out3, out4, fileName, name, declaration, usage, softwareVP,
                              recoverable);
}

// 0x8A66B0 (thiscall): NiD3DShaderProgramCreatorAsm::CreateVertexShaderFromBuffer (also Obj's)
uint8_t* __fastcall NiD3DShaderProgramCreatorAsm_CreateVertexShaderFromBuffer(
    uint8_t* self, void*, const void* buffer, uint32_t length, const char* name, const char*, const char*,
    void* declaration, uint32_t usage, int softwareVP, int recoverable) {
    uint8_t* program = NewVertexShader();
    if (program == nullptr) {
        return nullptr;
    }
    void* code = nullptr;
    uint32_t out4 = 0;
    uint32_t out3 = 0;
    uint32_t size = 0;
    if (Virtual<uint8_t>(self, 10, buffer, length, &code, &size, &out3, &out4, recoverable) == 0) {
        Virtual<void*>(program, 0, 1);
        return nullptr;
    }
    return FinishVertexShader(self, program, code, size, out3, out4, nullptr, name, declaration, usage, softwareVP,
                              recoverable);
}

namespace {

// the pixel shaders' common end
uint8_t* FinishPixelShader(uint8_t* self, uint8_t* program, void* code, uint32_t size, const char* fileName,
                           const char* name, int recoverable) {
    void* shader = Creator_CreatePixelShader(self, code);
    if (shader == nullptr) {
        ShaderFactory_ReportError(1, recoverable, kFailedPixelShader, name);
        Virtual<void*>(program, 0, 1);
        return nullptr;
    }
    Virtual<void>(program, 6, name);
    Virtual<void>(program, 8, fileName);
    Virtual<void>(program, 11, size, code);
    Virtual<void>(program, 13, self);
    Virtual<void>(program, 17, shader);
    return program;
}

} // namespace

// 0x8A6810 (thiscall): NiD3DShaderProgramCreatorAsm::CreatePixelShaderFromFile (also Obj's)
uint8_t* __fastcall NiD3DShaderProgramCreatorAsm_CreatePixelShaderFromFile(uint8_t* self, re::Edx,
                                                                         const char* fileName, const char* name,
                                                                         const char*, const char*, int recoverable) {
    uint8_t* program = NewPixelShader();
    if (program == nullptr) {
        return nullptr;
    }
    void* code = nullptr;
    uint32_t size = 0;
    uint32_t out3 = 0;
    uint32_t out4 = 0;
    if (Virtual<uint8_t>(self, 9, fileName, &code, &size, &out3, &out4, recoverable) == 0) {
        Virtual<void*>(program, 0, 1);
        return nullptr;
    }
    return FinishPixelShader(self, program, code, size, fileName, name, recoverable);
}

// 0x8A6920 (thiscall): NiD3DShaderProgramCreatorAsm::CreatePixelShaderFromBuffer (also Obj's)
uint8_t* __fastcall NiD3DShaderProgramCreatorAsm_CreatePixelShaderFromBuffer(uint8_t* self, re::Edx,
                                                                           const void* buffer, uint32_t length,
                                                                           const char* name, const char*,
                                                                           const char*, int recoverable) {
    uint8_t* program = NewPixelShader();
    if (program == nullptr) {
        return nullptr;
    }
    void* code = nullptr;
    uint32_t size = 0;
    uint32_t out3 = 0;
    uint32_t out4 = 0;
    if (Virtual<uint8_t>(self, 10, buffer, length, &code, &size, &out3, &out4, recoverable) == 0) {
        Virtual<void*>(program, 0, 1);
        return nullptr;
    }
    return FinishPixelShader(self, program, code, size, nullptr, name, recoverable);
}

// 0x8A6A30 (thiscall): NiD3DShaderProgramCreatorAsm::LoadShaderCodeFromFile (the program file
// found and assembled; quirk kept: a missing name is reported unrecoverable)
uint8_t __fastcall NiD3DShaderProgramCreatorAsm_LoadShaderCodeFromFile(uint8_t* self, re::Edx, const char* fileName,
                                                                     re::Ptr32<void>* code, uint32_t* size, uint32_t* out3,
                                                                     uint32_t* out4, int recoverable) {
    *code = nullptr;
    *size = 0;
    *out3 = 0;
    *out4 = 0;
    if (fileName == nullptr || *fileName == '\0') {
        ShaderFactory_ReportError(1, 0, kInvalidFileName);
        return 0;
    }
    char path[0x104];
    if (ProgramFactory_ResolveProgramFileName(fileName, path, 0x104) == 0) {
        ShaderFactory_ReportError(1, recoverable, kFileNotFound, fileName);
        return 0;
    }
    AssembleResult result = {};
    long hr = Import<long>(kD3DXAssembleShaderFromFileA, static_cast<const char*>(path), static_cast<void*>(nullptr),
                           static_cast<void*>(nullptr),
                           (At<uint32_t>(self, 4) | ms_uiUniversalShaderCreationFlags) & 3, &result.shader,
                           &result.errors);
    return FinishAssembly(hr, result, code, size, recoverable, kAssembleNone, kAssembleError, fileName) ? 1 : 0;
}

// 0x8A6C10 (thiscall): NiD3DShaderProgramCreatorAsm::LoadShaderCodeFromBuffer
uint8_t __fastcall NiD3DShaderProgramCreatorAsm_LoadShaderCodeFromBuffer(uint8_t* self, re::Edx, const void* buffer,
                                                                       uint32_t length, re::Ptr32<void>* code, uint32_t* size,
                                                                       uint32_t* out3, uint32_t* out4,
                                                                       int recoverable) {
    *code = nullptr;
    *size = 0;
    *out3 = 0;
    *out4 = 0;
    if (buffer == nullptr || length == 0) {
        ShaderFactory_ReportError(1, recoverable, kInvalidBuffer);
        return 0;
    }
    AssembleResult result = {};
    long hr = Import<long>(kD3DXAssembleShader, buffer, length, static_cast<void*>(nullptr),
                           static_cast<void*>(nullptr), (At<uint32_t>(self, 4) | ms_uiUniversalShaderCreationFlags) & 3,
                           &result.shader, &result.errors);
    return FinishAssembly(hr, result, code, size, recoverable, kAssembleMemoryNone, kAssembleMemoryError, nullptr)
               ? 1
               : 0;
}

// 0x8A6D80 (thiscall): NiD3DShaderProgramCreatorObj::LoadShaderCodeFromFile (the file read
// whole, 4 zero bytes after it; quirk kept: the file stays open when the buffer can't be had)
uint8_t __fastcall NiD3DShaderProgramCreatorObj_LoadShaderCodeFromFile(uint8_t*, re::Edx, const char* fileName,
                                                                     re::Ptr32<void>* code, uint32_t* size, uint32_t* out3,
                                                                     uint32_t* out4, int recoverable) {
    *code = nullptr;
    *size = 0;
    *out3 = 0;
    *out4 = 0;
    if (fileName == nullptr || *fileName == '\0') {
        ShaderFactory_ReportError(1, recoverable, kInvalidFileName);
        return 0;
    }
    char path[0x104];
    if (ProgramFactory_ResolveProgramFileName(fileName, path, 0x104) == 0) {
        ShaderFactory_ReportError(1, recoverable, kFileNotFound, fileName);
        return 0;
    }
    void* file = Import<void*>(kCreateFileA, static_cast<const char*>(path), 0x80000000u, 1u,
                               static_cast<void*>(nullptr), 3u, 0u, static_cast<void*>(nullptr));
    if (file == reinterpret_cast<void*>(intptr_t(-1))) {
        ShaderFactory_ReportError(1, recoverable, kInvalidFile, static_cast<const char*>(path));
        return 0;
    }
    uint32_t bytes = Import<uint32_t>(kGetFileSize, file, static_cast<void*>(nullptr));
    void* buffer = NiMalloc(bytes + 4);
    if (buffer == nullptr) {
        return 0;
    }
    crt::Memset(buffer, 0, bytes + 4);
    uint32_t read;
    Import<int>(kReadFile, file, buffer, bytes, &read, static_cast<void*>(nullptr));
    Import<int>(kCloseHandle, file);
    if (read != bytes) {
        NiFree(buffer);
        return 0;
    }
    *size = bytes;
    *code = buffer;
    return 1;
}

// 0x8A6F00 (thiscall): NiD3DShaderProgramCreatorObj::LoadShaderCodeFromBuffer (copied)
uint8_t __fastcall NiD3DShaderProgramCreatorObj_LoadShaderCodeFromBuffer(uint8_t*, re::Edx, const void* buffer,
                                                                       uint32_t length, re::Ptr32<void>* code, uint32_t* size,
                                                                       uint32_t* out3, uint32_t* out4,
                                                                       int recoverable) {
    *code = nullptr;
    *size = 0;
    *out3 = 0;
    *out4 = 0;
    if (buffer != nullptr && length != 0) {
        void* copy = NiMalloc(length);
        *code = copy;
        crt::MemcpyS(copy, length, buffer, length);
        *size = length;
        return 1;
    }
    ShaderFactory_ReportError(1, recoverable, kInvalidBuffer);
    return 0;
}

// 0x8A6F80 (thiscall, `this` unused): NiD3DShaderProgramCreator::CreateVertexShader (on the
// factory's device; the declaration, usage and the rest unused)
void* __fastcall NiD3DShaderProgramCreator_CreateVertexShader(void*, re::Edx, const void* code, void*, uint32_t,
                                                              uint32_t, uint32_t, int) {
    void* shader = const_cast<void*>(code);   // (the code's slot holds the result)
    long hr = Com<long>(ms_pkCreatorDevice, kDevCreateVertexShader, code, &shader);
    if (hr < 0) {
        ShaderFactory_ReportError(1, 0, kCreateVertexFailed, hr);
        return nullptr;
    }
    return shader;
}

// 0x8A6FC0 (thiscall, `this` unused): NiD3DShaderProgramCreator::CreatePixelShader
void* __fastcall NiD3DShaderProgramCreator_CreatePixelShader(void*, re::Edx, const void* code) {
    void* shader = const_cast<void*>(code);
    long hr = Com<long>(ms_pkCreatorDevice, kDevCreatePixelShader, code, &shader);
    if (hr < 0) {
        ShaderFactory_ReportError(1, 0, kCreatePixelFailed, hr);
        return nullptr;
    }
    return shader;
}

// ---- NiD3DShaderProgram

// 0x8A7000 (thiscall): NiD3DShaderProgram::SetD3DRenderer (its device held, its render state)
void __fastcall NiD3DShaderProgram_SetD3DRenderer(uint8_t* self, re::Edx, uint8_t* renderer) {
    At<uint8_t*>(self, 0x24) = renderer;
    if (renderer != nullptr) {
        void* device = At<void*>(renderer, 0xA4);
        ComRelease(At<void*>(self, 0x20));
        At<void*>(self, 0x20) = device;
        if (device != nullptr) {
            Com<long>(device, kAddRef);
        }
        At<void*>(self, 0x28) = At<void*>(At<uint8_t*>(self, 0x24), 0x6B4);
        return;
    }
    At<void*>(self, 0x28) = nullptr;
    ComRelease(At<void*>(self, 0x20));
    At<void*>(self, 0x20) = nullptr;
}

// 0x8A7070 (thiscall): NiD3DShaderProgram::NiD3DShaderProgram(renderer, kind)
uint8_t* __fastcall NiD3DShaderProgram_Construct(uint8_t* self, re::Edx, uint8_t* renderer, uint32_t kind) {
    At<uint32_t>(self, 0) = kRefObjectVtable;
    At<uint32_t>(self, 4) = 0;
    Import<long>(kInterlockedIncrement, &ms_uiObjects);
    At<uint32_t>(self, 8) = kind;
    At<uint32_t>(self, 0) = kProgramVtable;
    for (int offset = 0xC; offset <= 0x28; offset += 4) {
        At<uint32_t>(self, offset) = 0;
    }
    Program_SetD3DRenderer(self, renderer);
    return self;
}

// 0x8A70D0 (thiscall): NiD3DShaderProgram::GetRTTI
const void* __fastcall NiD3DShaderProgram_GetRTTI(void*, re::Edx) {
    return reinterpret_cast<const void*>(kProgramRTTI);
}

// 0x8A70E0 (thiscall): NiD3DShaderProgram::~NiD3DShaderProgram (its names and code freed, the
// device let go)
void __fastcall NiD3DShaderProgram_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kProgramVtable;
    NiFree(At<void*>(self, 0xC));
    NiFree(At<void*>(self, 0x10));
    NiFree(At<void*>(self, 0x18));
    void* device = At<void*>(self, 0x20);
    At<void*>(self, 0x24) = nullptr;
    At<void*>(self, 0x28) = nullptr;
    ComRelease(device);
    At<void*>(self, 0x20) = nullptr;
    At<uint32_t>(self, 0) = kRefObjectVtable;
    Import<long>(kInterlockedDecrement, &ms_uiObjects);
}

// 0x8A7140 (thiscall): NiD3DShaderProgram's deleting destructor
void* __fastcall NiD3DShaderProgram_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    Program_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x2C);
    }
    return self;
}

// ---- NiD3DPixelShader

// 0x8A7160 (thiscall): NiD3DPixelShader::DestroyRendererData (the renderer lets its shader go)
void __fastcall NiD3DPixelShader_DestroyRendererData(uint8_t* self, re::Edx) {
    if (At<void*>(self, 0x2C) != nullptr) {
        Renderer_ReleasePixelShader(At<void*>(self, 0x24), self);
    }
}

// 0x8A7170 (thiscall): NiD3DPixelShader::RecreateRendererData (by the program factory)
uint8_t __fastcall NiD3DPixelShader_RecreateRendererData(uint8_t* self, re::Edx) {
    return ProgramFactory_RecreatePixelShader(self);
}

// 0x8A7180 (thiscall): NiD3DPixelShader::~NiD3DPixelShader
void __fastcall NiD3DPixelShader_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kPixelShaderVtable;
    ProgramFactory_ReleasePixelShader(self);
    if (At<void*>(self, 0x2C) != nullptr) {
        Renderer_ReleasePixelShader(At<void*>(self, 0x24), self);
    }
    At<void*>(self, 0x2C) = nullptr;
    Program_Destruct(self);
}

// 0x8A71B0 (thiscall): NiD3DPixelShader::SetShaderConstant(entry, data, count) (the entry's own
// data and register count by default)
uint8_t __fastcall NiD3DPixelShader_SetShaderConstant(uint8_t* self, re::Edx, uint8_t* entry, const void* data,
                                                    uint32_t count) {
    if (data == nullptr) {
        data = At<void*>(entry, 0x2C);
    }
    if (count == 0) {
        count = At<uint32_t>(entry, 0x1C);
        if (count == 0) {
            count = 1;
        }
    }
    uint32_t type = ms_auiAttributeTypes[At<uint32_t>(entry, 0x10) & 0xFF];
    uint8_t* manager = At<uint8_t*>(At<uint8_t*>(self, 0x28), 0x20F0);
    if (type == 1) {
        return SetBooleans(manager, 0x60, 0x70, 0x7C, At<uint32_t>(entry, 0x18), data, count);
    }
    if (type == 3) {
        return ConstantManager_SetPixelShaderConstantI(manager, At<uint32_t>(entry, 0x18), data, count);
    }
    return ConstantManager_SetPixelShaderConstantF(manager, At<uint32_t>(entry, 0x18), data, count);
}

// 0x8A7260 (thiscall): NiD3DPixelShader::SetShaderConstantArray(entry, data, count, registers
// per element, remapping)
uint8_t __fastcall NiD3DPixelShader_SetShaderConstantArray(uint8_t* self, re::Edx, uint8_t* entry, const uint8_t* data,
                                                         uint32_t count, uint32_t registers,
                                                         const uint16_t* remapping) {
    return SetArray(self, 0x10, 0x20, 0x2C, entry, data, count, registers, remapping);
}

// ---- NiD3DVertexShader

// 0x8A7330 (thiscall): NiD3DVertexShader::DestroyRendererData
void __fastcall NiD3DVertexShader_DestroyRendererData(uint8_t* self, re::Edx) {
    if (At<void*>(self, 0x34) != nullptr) {
        Renderer_ReleaseVertexShader(At<void*>(self, 0x24), self);
    }
}

// 0x8A7340 (thiscall): NiD3DVertexShader::RecreateRendererData
uint8_t __fastcall NiD3DVertexShader_RecreateRendererData(uint8_t* self, re::Edx) {
    return ProgramFactory_RecreateVertexShader(self);
}

// 0x8A7350 (thiscall): NiD3DVertexShader::~NiD3DVertexShader (the handle left as it was)
void __fastcall NiD3DVertexShader_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kVertexShaderVtable;
    ProgramFactory_ReleaseVertexShader(self);
    if (At<void*>(self, 0x34) != nullptr) {
        Renderer_ReleaseVertexShader(At<void*>(self, 0x24), self);
    }
    Program_Destruct(self);
}

// 0x8A7380 (thiscall): NiD3DVertexShader::SetShaderConstant(entry, data, count)
uint8_t __fastcall NiD3DVertexShader_SetShaderConstant(uint8_t* self, re::Edx, uint8_t* entry, const void* data,
                                                     uint32_t count) {
    if (data == nullptr) {
        data = At<void*>(entry, 0x2C);
    }
    if (count == 0) {
        count = At<uint32_t>(entry, 0x1C);
        if (count == 0) {
            count = 1;
        }
    }
    uint32_t type = ms_auiAttributeTypes[At<uint32_t>(entry, 0x10) & 0xFF];
    uint8_t* manager = At<uint8_t*>(At<uint8_t*>(self, 0x28), 0x20F0);
    if (type == 1) {
        return SetBooleans(manager, 0x58, 0x68, 0x78, At<uint32_t>(entry, 0x18), data, count);
    }
    if (type == 3) {
        return ConstantManager_SetVertexShaderConstantI(manager, At<uint32_t>(entry, 0x18), data, count);
    }
    return ConstantManager_SetVertexShaderConstantF(manager, At<uint32_t>(entry, 0x18), data, count);
}

// 0x8A7430 (thiscall): NiD3DVertexShader::SetShaderConstantArray
uint8_t __fastcall NiD3DVertexShader_SetShaderConstantArray(uint8_t* self, re::Edx, uint8_t* entry, const uint8_t* data,
                                                          uint32_t count, uint32_t registers,
                                                          const uint16_t* remapping) {
    return SetArray(self, 8, 0x18, 0x28, entry, data, count, registers, remapping);
}

#define C42B_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C42B_REVERSED("NiD3DShaderProgramCreatorAsm::RecreateVertexShader", 0x008A64B0,
              NiD3DShaderProgramCreatorAsm_RecreateVertexShader);
C42B_REVERSED("NiD3DShaderProgramCreatorAsm::RecreatePixelShader", 0x008A6510,
              NiD3DShaderProgramCreatorAsm_RecreatePixelShader);
C42B_REVERSED("NiD3DShaderProgramCreatorAsm::CreateVertexShaderFromFile", 0x008A6550,
              NiD3DShaderProgramCreatorAsm_CreateVertexShaderFromFile);
C42B_REVERSED("NiD3DShaderProgramCreatorAsm::CreateVertexShaderFromBuffer", 0x008A66B0,
              NiD3DShaderProgramCreatorAsm_CreateVertexShaderFromBuffer);
C42B_REVERSED("NiD3DShaderProgramCreatorAsm::CreatePixelShaderFromFile", 0x008A6810,
              NiD3DShaderProgramCreatorAsm_CreatePixelShaderFromFile);
C42B_REVERSED("NiD3DShaderProgramCreatorAsm::CreatePixelShaderFromBuffer", 0x008A6920,
              NiD3DShaderProgramCreatorAsm_CreatePixelShaderFromBuffer);
C42B_REVERSED("NiD3DShaderProgramCreatorAsm::LoadShaderCodeFromFile", 0x008A6A30,
              NiD3DShaderProgramCreatorAsm_LoadShaderCodeFromFile);
C42B_REVERSED("NiD3DShaderProgramCreatorAsm::LoadShaderCodeFromBuffer", 0x008A6C10,
              NiD3DShaderProgramCreatorAsm_LoadShaderCodeFromBuffer);
C42B_REVERSED("NiD3DShaderProgramCreatorObj::LoadShaderCodeFromFile", 0x008A6D80,
              NiD3DShaderProgramCreatorObj_LoadShaderCodeFromFile);
C42B_REVERSED("NiD3DShaderProgramCreatorObj::LoadShaderCodeFromBuffer", 0x008A6F00,
              NiD3DShaderProgramCreatorObj_LoadShaderCodeFromBuffer);
C42B_REVERSED("NiD3DShaderProgramCreator::CreateVertexShader", 0x008A6F80, NiD3DShaderProgramCreator_CreateVertexShader);
C42B_REVERSED("NiD3DShaderProgramCreator::CreatePixelShader", 0x008A6FC0, NiD3DShaderProgramCreator_CreatePixelShader);
C42B_REVERSED("NiD3DShaderProgram::SetD3DRenderer", 0x008A7000, NiD3DShaderProgram_SetD3DRenderer);
C42B_REVERSED("NiD3DShaderProgram::NiD3DShaderProgram", 0x008A7070, NiD3DShaderProgram_Construct);
C42B_REVERSED("NiD3DShaderProgram::GetRTTI", 0x008A70D0, NiD3DShaderProgram_GetRTTI);
C42B_REVERSED("NiD3DShaderProgram::~NiD3DShaderProgram", 0x008A70E0, NiD3DShaderProgram_Destruct);
C42B_REVERSED("NiD3DShaderProgram::DeletingDestructor", 0x008A7140, NiD3DShaderProgram_DeletingDestructor);
C42B_REVERSED("NiD3DPixelShader::DestroyRendererData", 0x008A7160, NiD3DPixelShader_DestroyRendererData);
C42B_REVERSED("NiD3DPixelShader::RecreateRendererData", 0x008A7170, NiD3DPixelShader_RecreateRendererData);
C42B_REVERSED("NiD3DPixelShader::~NiD3DPixelShader", 0x008A7180, NiD3DPixelShader_Destruct);
C42B_REVERSED("NiD3DPixelShader::SetShaderConstant", 0x008A71B0, NiD3DPixelShader_SetShaderConstant);
C42B_REVERSED("NiD3DPixelShader::SetShaderConstantArray", 0x008A7260, NiD3DPixelShader_SetShaderConstantArray);
C42B_REVERSED("NiD3DVertexShader::DestroyRendererData", 0x008A7330, NiD3DVertexShader_DestroyRendererData);
C42B_REVERSED("NiD3DVertexShader::RecreateRendererData", 0x008A7340, NiD3DVertexShader_RecreateRendererData);
C42B_REVERSED("NiD3DVertexShader::~NiD3DVertexShader", 0x008A7350, NiD3DVertexShader_Destruct);
C42B_REVERSED("NiD3DVertexShader::SetShaderConstant", 0x008A7380, NiD3DVertexShader_SetShaderConstant);
C42B_REVERSED("NiD3DVertexShader::SetShaderConstantArray", 0x008A7430, NiD3DVertexShader_SetShaderConstantArray);

} // namespace ni::c42
