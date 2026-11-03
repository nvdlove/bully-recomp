// The D3D shader programs (NiD3DShaderProgram, vtable 0x99E350, NiRTTI 0x20E7518, an NiRefObject:
// +0x8 the kind (0 vertex, 1 pixel), +0xC its name, +0x10 its program file's name (both
// NiMalloc'd), +0x14 / +0x18 its code's size and code, +0x1C its creator, +0x20 the device (held),
// +0x24 the renderer, +0x28 its render state (+0x20F0 the shader constant manager)):
// NiD3DPixelShader (0x99DD00, 0x30 bytes: +0x2C the D3D shader), NiD3DVertexShader (0x99DD50,
// 0x3C: +0x2C software vertex processing, +0x30 its usage, +0x34 the D3D shader, +0x38 its
// declaration), NiD3DHLSLPixelShader (0x99DDB8, 0x3C: +0x30 / +0x34 the entry point and shader
// target, +0x38 the ID3DXConstantTable) and NiD3DHLSLVertexShader (0x99DE20, 0x48: +0x3C / +0x40 /
// +0x44 likewise); their slots here: 3 GetVariableCount, 4 GetVariableName, 6 SetName, 8
// SetShaderProgramName, 11 SetCode, 13 SetCreator, 14 SetShaderConstant, 15
// SetShaderConstantArray, the D3D shader's get / set (pixel 16/17, vertex 18/19), the vertex
// shader's usage (16/17), declaration (20/21) and software vertex processing (22/23), the HLSL
// ones' entry point, target and constant table, Destroy- / RecreateRendererData.
//
// And NiD3DShaderProgramCreatorHLSL's slots (0x99DB5C, `this` +0x4 its compile flags): 3/4
// CreateVertexShaderFromFile / FromBuffer, 5/6 the pixel ones, 7/8 RecreateVertexShader /
// RecreatePixelShader, 9/10 the shaders from an ID3DXBuffer of compiled code; the shaders compiled
// with D3DX (the flags or'd with 0x20E6EA0), made on the device (0x8A6F80 / 0x8A6FC0), the renderer
// at 0x20E6EA8 and its device at 0x20E6EA4 (NiD3DShaderProgramFactory::SetD3DRenderer's).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::c41 {
using namespace ni::dx9;
namespace {

constexpr uintptr_t kHLSLPixelShaderVtable = 0x0099DDB8;
constexpr uintptr_t kHLSLVertexShaderVtable = 0x0099DE20;
constexpr uintptr_t kPixelShaderRTTI = 0x020E754C;
constexpr uintptr_t kVertexShaderRTTI = 0x020E7580;
constexpr uintptr_t kHLSLPixelShaderRTTI = 0x020E749C;
constexpr uintptr_t kHLSLVertexShaderRTTI = 0x020E74D0;
inline const char* const kMain = reinterpret_cast<const char*>(0x0092CC9C);   // "main"

// D3DX through the game's import slots
constexpr uintptr_t kD3DXCompileShaderFromFileA = 0x008FC2BC;
constexpr uintptr_t kD3DXGetShaderConstantTable = 0x008FC2C0;
constexpr uintptr_t kD3DXCompileShader = 0x008FC2F0;
constexpr uintptr_t kD3DXGetVertexShaderProfile = 0x008FC314;
constexpr uintptr_t kD3DXGetPixelShaderProfile = 0x008FC318;
constexpr uintptr_t kD3DXMatrixTranspose = 0x008FC2EC;

GAME_VAR(uint8_t*, ms_pkCreatorRenderer, 0x020E6EA8);
GAME_VAR(void*, ms_pkCreatorDevice, 0x020E6EA4);
GAME_VAR(uint32_t, ms_uiUniversalShaderCreationFlags, 0x020E6EA0);
GAME_ARRAY(uint32_t, ms_auiAttributeTypes, 0x020E408C);   // by the constant map entry's flags' low byte

GAME_FN(uint32_t, __cdecl, ShaderFactory_ReportError, 0x00756700, int result, int recoverable, const char* format,
        ...);
GAME_FN(void, __cdecl, NiStrcpy, 0x00747280, char* dest, uint32_t size, const char* src);
GAME_FN(uint8_t, __cdecl, ProgramFactory_ResolveProgramFileName, 0x008A27D0, const char* name, char* out,
        size_t size);
GAME_FN(void*, __thiscall, ShaderProgram_Construct, 0x008A7070, void* self, void* renderer, uint32_t kind);
GAME_FN(void, __thiscall, PixelShader_Destruct, 0x008A7180, void* self);
GAME_FN(void, __thiscall, VertexShader_Destruct, 0x008A7350, void* self);
GAME_FN(void, __thiscall, PixelShader_DestroyRendererData, 0x008A7160, void* self);
GAME_FN(void, __thiscall, VertexShader_DestroyRendererData, 0x008A7330, void* self);
// the renderer's shader creation (`this` unused; the vertex one's other arguments unused too)
GAME_FN(void*, __thiscall, Renderer_CreateVertexShader, 0x008A6F80, void* self, const void* code, void* declaration,
        uint32_t usage, uint32_t a4, uint32_t a5, uint32_t softwareVP);
GAME_FN(void*, __thiscall, Renderer_CreatePixelShader, 0x008A6FC0, void* self, const void* code);
GAME_FN(re::Ptr32<char>*, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, const re::Ptr32<char>* other);
GAME_FN(void, __thiscall, FixedString_Destruct, 0x00517750, re::Ptr32<char>* self);
// the shader constant manager's float / integer constants set (register, data, count)
GAME_FN(uint8_t, __thiscall, ConstantManager_SetVertexShaderConstantF, 0x0074D2C0, void* self, uint32_t start,
        const void* data, uint32_t count);
GAME_FN(uint8_t, __thiscall, ConstantManager_SetVertexShaderConstantI, 0x0074D310, void* self, uint32_t start,
        const void* data, uint32_t count);
GAME_FN(uint8_t, __thiscall, ConstantManager_SetPixelShaderConstantF, 0x0074D360, void* self, uint32_t start,
        const void* data, uint32_t count);
GAME_FN(uint8_t, __thiscall, ConstantManager_SetPixelShaderConstantI, 0x0074D3B0, void* self, uint32_t start,
        const void* data, uint32_t count);

// this file's (called where modes apply)
GAME_FN(uint8_t, __thiscall, CreatorHLSL_CompileFromFile, 0x008A3560, void* self, const char* fileName,
        const char* entryPoint, const char* target, re::Ptr32<void>* code, uint32_t* size, re::Ptr32<void>* constantTable,
        int recoverable);
GAME_FN(uint8_t, __thiscall, CreatorHLSL_CompileFromBuffer, 0x008A3760, void* self, const void* buffer,
        uint32_t length, const char* entryPoint, const char* target, re::Ptr32<void>* code, uint32_t* size,
        re::Ptr32<void>* constantTable, int recoverable);
GAME_FN(void, __thiscall, HLSLPixelShader_Destruct, 0x008A3F10, void* self);
GAME_FN(void, __thiscall, HLSLVertexShader_Destruct, 0x008A44B0, void* self);

inline long D3DXGetShaderConstantTable(const void* code, re::Ptr32<void>* table) {
    return Import<long>(kD3DXGetShaderConstantTable, code, table);
}
inline void MatrixTranspose(float* out, const void* m) {
    Import<float*>(kD3DXMatrixTranspose, out, m);
}
inline size_t Length(const char* s) {
    return crt::Strcspn(s, "");
}

// COM calls on an ID3DXBuffer (3 GetBufferPointer, 4 GetBufferSize) and an
// ID3DXConstantTable (5 GetDesc, 6 GetConstantDesc, 8 GetConstant, 9 GetConstantByName)
inline void* BufferPointer(void* buffer) {
    return Com<void*>(buffer, 3);
}
inline uint32_t BufferSize(void* buffer) {
    return Com<uint32_t>(buffer, 4);
}

// a string field replaced by a copy (0 for none); `clearWhenEmpty` false keeps the old (freed)
// pointer when there's no new one: the game's
template <int Offset, bool ClearWhenEmpty>
void SetStringField(uint8_t* self, const char* text) {
    if (At<const char*>(self, Offset) == text) {
        return;
    }
    NiFree(At<void*>(self, Offset));
    if (text != nullptr && *text != '\0') {
        size_t size = Length(text) + 1;
        char* copy = static_cast<char*>(NiMalloc(uint32_t(size)));
        crt::StrcpyS(copy, size, text);
        At<char*>(self, Offset) = copy;
        return;
    }
    if (ClearWhenEmpty) {
        At<char*>(self, Offset) = nullptr;
    }
}

// a COM reference field assigned (the old released, the new held)
template <int Offset>
void SetComField(uint8_t* self, void* object) {
    void* old = At<void*>(self, Offset);
    if (old == object) {
        return;
    }
    if (old != nullptr) {
        Com<long>(old, kRelease);
    }
    At<void*>(self, Offset) = object;
    if (object != nullptr) {
        Com<long>(object, kAddRef);
    }
}

// a buffer string field (the name, the program file's name): reused when it's long enough
template <int Offset>
void SetBufferedName(uint8_t* self, const char* text) {
    if (text != nullptr && *text != '\0') {
        size_t size = Length(text) + 1;
        char* old = At<char*>(self, Offset);
        if (old != nullptr && Length(old) < size) {
            NiFree(old);
            At<char*>(self, Offset) = nullptr;
        }
        if (At<char*>(self, Offset) == nullptr) {
            At<char*>(self, Offset) = static_cast<char*>(NiMalloc(uint32_t(size)));
        }
        crt::StrcpyS(At<char*>(self, Offset), size, text);
        return;
    }
    NiFree(At<void*>(self, Offset));
    At<char*>(self, Offset) = nullptr;
}

// D3DXCONSTANT_DESC (the system's layout: native pointers)
struct ConstantDesc {
    const char* name;
    uint32_t registerSet;
    uint32_t registerIndex;
    uint32_t registerCount;
    uint32_t constantClass;   // 3 D3DXPC_MATRIX_COLUMNS
    uint32_t rest[6];         // type, rows, columns, elements, struct members, bytes
    const void* defaultValue;
};
static_assert(sizeof(void*) != 4 || sizeof(ConstantDesc) == 0x30);

// D3DXCONSTANTTABLE_DESC (likewise)
struct ConstantTableDesc {
    const char* creator;
    uint32_t version;
    uint32_t constants;
};

// a constant map entry's registers looked up by its variable name (its key when it has none,
// kept as its variable name) the first time; false (reported) when the table hasn't it. `desc`
// filled.
inline bool LookUpRegisters(uint8_t* program, void* table, uint8_t* entry, ConstantDesc* desc,
                            const char* notFound) {
    re::Ptr32<char> name = ni::sys::FixedStringAddRef(At<char*>(entry, 0x20));
    if (name == ni::sys::kNullFixedString) {
        FixedString_Assign(&name, &At<char*>(entry, 8));
        FixedString_Assign(&At<char*>(entry, 0x20), &name);
    }
    void* handle = Com<void*>(table, 9, static_cast<void*>(nullptr), name);
    if (handle == nullptr) {
        NiOutputDebugString(notFound, At<const char*>(program, 0xC), name);
        FixedString_Destruct(&name);
        return false;
    }
    uint32_t count = 1;
    Com<long>(table, 6, handle, desc, &count);
    At<uint32_t>(entry, 0x18) = desc->registerIndex;
    At<uint32_t>(entry, 0x1C) = desc->registerCount;
    At<uint8_t>(entry, 0x31) = 1;
    FixedString_Destruct(&name);
    return true;
}

// the register ranges the constant manager keeps per kind (bool and matrix arrays: their data,
// size, lowest and highest changed register)
struct ManagerRanges {
    int matrixData;
    int matrixLimit;
    int matrixLow;
    int matrixHigh;
    int boolData;
    int boolLimit;
    int boolLow;
    int boolHigh;
};
constexpr ManagerRanges kPixelRanges = {0x10, 0x2C, 0x20, 0x24, 0x60, 0x7C, 0x70, 0x74};
constexpr ManagerRanges kVertexRanges = {0x8, 0x28, 0x18, 0x1C, 0x58, 0x78, 0x68, 0x6C};

// SetShaderConstant: the entry's registers found; matrices transposed into the constant
// manager's array (as many registers as the entry has, from the one transposed matrix: past 4
// registers the game copies its stack beyond it), floats and
// integers through the manager, bools into its array; the changed range widened
template <int TableOffset, const ManagerRanges& Ranges,
          uint8_t(__thiscall* const& SetFloat)(void*, uint32_t, const void*, uint32_t),
          uint8_t(__thiscall* const& SetInt)(void*, uint32_t, const void*, uint32_t)>
uint8_t SetShaderConstant(uint8_t* self, uint8_t* entry, const void* data, const char* notFound) {
    void* table = At<void*>(self, TableOffset);
    if (table == nullptr || At<void*>(self, 0x20) == nullptr) {
        return 0;
    }
    if (data == nullptr) {
        data = At<void*>(entry, 0x2C);
    }
    float transposed[16];
    if (At<uint8_t>(entry, 0x31) == 0) {
        ConstantDesc desc;
        if (!LookUpRegisters(self, table, entry, &desc, notFound)) {
            return 0;
        }
    }
    uint32_t type = ms_auiAttributeTypes[At<uint32_t>(entry, 0x10) & 0xFF];
    uint8_t* manager = At<uint8_t*>(At<uint8_t*>(self, 0x28), 0x20F0);
    uint32_t start = At<uint32_t>(entry, 0x18);
    uint32_t count = At<uint32_t>(entry, 0x1C);
    if (type == 8 || type == 9) {   // matrix3, matrix4
        MatrixTranspose(transposed, data);
        uint32_t end = count + start;
        if (end > At<uint32_t>(manager, Ranges.matrixLimit)) {
            return 0;
        }
        crt::MemcpyS(At<uint8_t*>(manager, Ranges.matrixData) + start * 0x10, count * 0x10, transposed, count * 0x10);
        if (At<uint32_t>(manager, Ranges.matrixLow) > start) {
            At<uint32_t>(manager, Ranges.matrixLow) = start;
        }
        if (At<uint32_t>(manager, Ranges.matrixHigh) < end) {
            At<uint32_t>(manager, Ranges.matrixHigh) = end;
        }
        return 1;
    }
    if (type == 5 || type == 6 || type == 7 || type == 10 || type == 4) {   // points, colour, float
        return SetFloat(manager, start, data, count) != 0 ? 1 : 0;
    }
    if (type == 3) {   // unsigned int
        return SetInt(manager, start, data, count) != 0 ? 1 : 0;
    }
    if (type != 1) {   // bool
        return 0;
    }
    uint32_t end = start + count;
    if (end > At<uint32_t>(manager, Ranges.boolLimit)) {
        return 0;
    }
    crt::MemcpyS(At<uint8_t*>(manager, Ranges.boolData) + start * 4, count * 4, data, count * 4);
    if (At<uint32_t>(manager, Ranges.boolLow) > start) {
        At<uint32_t>(manager, Ranges.boolLow) = start;
    }
    if (At<uint32_t>(manager, Ranges.boolHigh) < end) {
        At<uint32_t>(manager, Ranges.boolHigh) = end;
    }
    return 1;
}

// the array setters' common start: the entry's registers (looked up the first time; else its
// description read again for its class); false when not found. (A failed GetConstantDesc leaves the
// game's description as its stack had it: zeros here.)
inline bool ArrayDesc(uint8_t* self, void* table, uint8_t* entry, ConstantDesc* desc, const char* notFound) {
    if (At<uint8_t>(entry, 0x31) == 0) {
        return LookUpRegisters(self, table, entry, desc, notFound);
    }
    void* handle = Com<void*>(table, 9, static_cast<void*>(nullptr), At<char*>(entry, 0x20));
    uint32_t count = 1;
    Com<long>(table, 6, handle, desc, &count);
    return true;
}

} // namespace

// 0x8A2C50 (thiscall): a vertex / index buffer pool entry's buffer (+0x10) released. (Name a
// guess.)
void __fastcall PoolEntry_ReleaseBuffer(uint8_t* self, re::Edx) {
    if (At<void*>(self, 0x10) != nullptr) {
        Com<long>(At<void*>(self, 0x10), kRelease);
        At<void*>(self, 0x10) = nullptr;
    }
}

// 0x8A2C70 (thiscall, creator slot 7): NiD3DShaderProgramCreatorHLSL::RecreateVertexShader (the
// D3D shader made again from its code, its declaration and usage; its constant table read again).
// (The table is written into the argument's slot: when D3DX fails without writing it, the game
// releases the program pointer itself as a COM object.)
uint8_t __fastcall CreatorHLSL_RecreateVertexShader(uint8_t* self, re::Edx, uint8_t* program) {
    const void* code = Virtual<const void*>(program, 10);
    if (code == nullptr) {
        return 0;
    }
    void* declaration = Virtual<void*>(program, 20);
    uint32_t usage = Virtual<uint32_t>(program, 16);
    void* shader = Renderer_CreateVertexShader(self, code, &declaration, usage, 0, 0, 0);
    if (shader == nullptr) {
        return 0;
    }
    Virtual<void>(program, 19, shader);
    re::Ptr32<void> table = program;
    if (D3DXGetShaderConstantTable(code, &table) >= 0) {
        Virtual<void>(program, 31, table);
    }
    if (table != nullptr) {
        Com<long>(table, kRelease);
    }
    return 1;
}

// 0x8A2D00 (thiscall, creator slot 8): NiD3DShaderProgramCreatorHLSL::RecreatePixelShader (as the
// vertex one)
uint8_t __fastcall CreatorHLSL_RecreatePixelShader(uint8_t* self, re::Edx, uint8_t* program) {
    const void* code = Virtual<const void*>(program, 10);
    if (code == nullptr) {
        return 0;
    }
    void* shader = Renderer_CreatePixelShader(self, code);
    if (shader == nullptr) {
        return 0;
    }
    Virtual<void>(program, 17, shader);
    re::Ptr32<void> table = program;
    if (D3DXGetShaderConstantTable(code, &table) >= 0) {
        Virtual<void>(program, 25, table);
    }
    if (table != nullptr) {
        Com<long>(table, kRelease);
    }
    return 1;
}

// 0x8A2D70 / 0x8A2E20 (thiscall, slots 6 / 8): NiD3DShaderProgram::SetName / SetShaderProgramName
void __fastcall ShaderProgram_SetName(uint8_t* self, re::Edx, const char* name) {
    SetBufferedName<0xC>(self, name);
}
void __fastcall ShaderProgram_SetShaderProgramName(uint8_t* self, re::Edx, const char* name) {
    SetBufferedName<0x10>(self, name);
}

// 0x8A2ED0 (thiscall, slot 11): NiD3DShaderProgram::SetCode (its size and code, kept as given)
void __fastcall ShaderProgram_SetCode(uint8_t* self, re::Edx, uint32_t size, void* code) {
    At<uint32_t>(self, 0x14) = size;
    At<void*>(self, 0x18) = code;
}

// 0x8A2EF0 (thiscall, slot 13): NiD3DShaderProgram::SetCreator (a plain +0x1C setter, shared)
void __fastcall ShaderProgram_SetCreator(uint8_t* self, re::Edx, void* creator) {
    At<void*>(self, 0x1C) = creator;
}

// 0x8A2F00 / 0x8A2F30 / 0x8A2FB0 / 0x8A30E0 (slot 2): GetRTTI
const void* __fastcall PixelShader_GetRTTI(void*, re::Edx) {
    return reinterpret_cast<const void*>(kPixelShaderRTTI);
}
const void* __fastcall VertexShader_GetRTTI(void*, re::Edx) {
    return reinterpret_cast<const void*>(kVertexShaderRTTI);
}
const void* __fastcall HLSLPixelShader_GetRTTI(void*, re::Edx) {
    return reinterpret_cast<const void*>(kHLSLPixelShaderRTTI);
}
const void* __fastcall HLSLVertexShader_GetRTTI(void*, re::Edx) {
    return reinterpret_cast<const void*>(kHLSLVertexShaderRTTI);
}

// 0x8A2F10 / 0x8A2F40 / 0x8A2FC0 / 0x8A30F0 (slot 0): the deleting destructors
uint8_t* __fastcall PixelShader_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    PixelShader_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x30);
    }
    return self;
}
uint8_t* __fastcall VertexShader_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    VertexShader_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x3C);
    }
    return self;
}
uint8_t* __fastcall HLSLPixelShader_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    HLSLPixelShader_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x3C);
    }
    return self;
}
uint8_t* __fastcall HLSLVertexShader_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    HLSLVertexShader_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x48);
    }
    return self;
}

// 0x8A2F60 (thiscall, vertex slot 17): NiD3DVertexShader::SetUsage (+0x30)
void __fastcall VertexShader_SetUsage(uint8_t* self, re::Edx, uint32_t usage) {
    At<uint32_t>(self, 0x30) = usage;
}
// 0x8A2F70 (vertex slot 18, the HLSL pixel shader's 22): a 4-byte `mov eax, [ecx+0x34]; ret`
// 0x8A2F70 (thiscall): the vertex shader's handle (+0x34)
void* __fastcall VertexShader_GetShaderHandle(uint8_t* self, re::Edx) {
    return reinterpret_cast<void*>(uintptr_t(*reinterpret_cast<const uint32_t*>(self + 0x34)));
}
RE_UNHOOKED_FN("NiD3DVertexShader::GetShaderHandle", 0x008A2F70, &VertexShader_GetShaderHandle);
// 0x8A2F80 (thiscall, vertex slot 19): NiD3DVertexShader::SetShaderHandle (+0x34)
void __fastcall VertexShader_SetShaderHandle(uint8_t* self, re::Edx, void* shader) {
    At<void*>(self, 0x34) = shader;
}
// 0x8A2F90 (vertex slot 22): a 4-byte byte getter of +0x2C
// 0x8A2F90 (thiscall): whether the vertex shader runs in software (+0x2C)
uint8_t __fastcall VertexShader_GetSoftwareVertexProcessing(uint8_t* self, re::Edx) {
    return self[0x2C];
}
RE_UNHOOKED_FN("NiD3DVertexShader::GetSoftwareVertexProcessing", 0x008A2F90, &VertexShader_GetSoftwareVertexProcessing);
// 0x8A2FA0 (thiscall, vertex slot 23): NiD3DVertexShader::SetSoftwareVertexProcessing
void __fastcall VertexShader_SetSoftwareVertexProcessing(uint8_t* self, re::Edx, int software) {
    At<uint8_t>(self, 0x2C) = uint8_t(software);
}

// 0x8A2FE0 / 0x8A3040 (thiscall, HLSL pixel slots 21 / 23): SetEntryPoint / SetShaderTarget (the
// target's old pointer kept, freed, when no new one is given)
void __fastcall HLSLPixelShader_SetEntryPoint(uint8_t* self, re::Edx, const char* text) {
    SetStringField<0x30, true>(self, text);
}
void __fastcall HLSLPixelShader_SetShaderTarget(uint8_t* self, re::Edx, const char* text) {
    SetStringField<0x34, false>(self, text);
}
// 0x8A30A0 (HLSL pixel slot 24, vertex slot 20): a 4-byte getter of +0x38
// 0x8A30A0 (thiscall): the pixel shader's constant table (+0x38)
void* __fastcall HLSLPixelShader_GetConstantTable(uint8_t* self, re::Edx) {
    return reinterpret_cast<void*>(uintptr_t(*reinterpret_cast<const uint32_t*>(self + 0x38)));
}
RE_UNHOOKED_FN("NiD3DHLSLPixelShader::GetConstantTable", 0x008A30A0, &HLSLPixelShader_GetConstantTable);
// 0x8A30B0 (thiscall, HLSL pixel slot 25): SetConstantTable (held)
void __fastcall HLSLPixelShader_SetConstantTable(uint8_t* self, re::Edx, void* table) {
    SetComField<0x38>(self, table);
}

// 0x8A3110 / 0x8A3170 / 0x8A31D0 (thiscall, HLSL vertex slots 27 / 29 / 31): the same at
// +0x3C / +0x40 / +0x44
void __fastcall HLSLVertexShader_SetEntryPoint(uint8_t* self, re::Edx, const char* text) {
    SetStringField<0x3C, true>(self, text);
}
void __fastcall HLSLVertexShader_SetShaderTarget(uint8_t* self, re::Edx, const char* text) {
    SetStringField<0x40, false>(self, text);
}
void __fastcall HLSLVertexShader_SetConstantTable(uint8_t* self, re::Edx, void* table) {
    SetComField<0x44>(self, table);
}

// a new HLSL vertex / pixel shader (constructed as the creators inline it)
inline uint8_t* NewHLSLVertexShader() {
    uint8_t* program = static_cast<uint8_t*>(NiNew(0x48));
    if (program == nullptr) {
        return nullptr;
    }
    ShaderProgram_Construct(program, ms_pkCreatorRenderer, 0);
    At<uint8_t>(program, 0x2C) = 0;
    At<uint32_t>(program, 0x30) = 0;
    At<uint32_t>(program, 0x34) = 0;
    At<uint32_t>(program, 0x38) = 0;
    At<uint32_t>(program, 0) = kHLSLVertexShaderVtable;
    At<uint32_t>(program, 0x3C) = 0;
    At<uint32_t>(program, 0x40) = 0;
    At<uint32_t>(program, 0x44) = 0;
    return program;
}
inline uint8_t* NewHLSLPixelShader() {
    uint8_t* program = static_cast<uint8_t*>(NiNew(0x3C));
    if (program == nullptr) {
        return nullptr;
    }
    ShaderProgram_Construct(program, ms_pkCreatorRenderer, 1);
    At<uint32_t>(program, 0x2C) = 0;
    At<uint32_t>(program, 0) = kHLSLPixelShaderVtable;
    At<uint32_t>(program, 0x30) = 0;
    At<uint32_t>(program, 0x34) = 0;
    At<uint32_t>(program, 0x38) = 0;
    return program;
}

// a made vertex shader filled in (name, file name, code, creator, D3D shader, usage, software
// vertex processing, entry point, target, constant table; the table's reference let go)
inline void FillVertexShader(uint8_t* program, void* creator, const char* name, const char* fileName, uint32_t size,
                             void* code, void* shader, uint32_t usage, int softwareVP, const char* entryPoint,
                             const char* target, void* table) {
    Virtual<void>(program, 6, name);
    Virtual<void>(program, 8, fileName);
    Virtual<void>(program, 11, size, code);
    Virtual<void>(program, 13, creator);
    Virtual<void>(program, 19, shader);
    Virtual<void>(program, 17, usage);
    Virtual<void>(program, 23, softwareVP);
    Virtual<void>(program, 27, entryPoint);
    Virtual<void>(program, 29, target);
    Virtual<void>(program, 31, table);
    if (table != nullptr) {
        Com<long>(table, kRelease);
    }
}
inline void FillPixelShader(uint8_t* program, void* creator, const char* name, const char* fileName, uint32_t size,
                            void* code, void* shader, const char* entryPoint, const char* target, void* table) {
    Virtual<void>(program, 6, name);
    Virtual<void>(program, 8, fileName);
    Virtual<void>(program, 11, size, code);
    Virtual<void>(program, 13, creator);
    Virtual<void>(program, 17, shader);
    Virtual<void>(program, 21, entryPoint);
    Virtual<void>(program, 23, target);
    Virtual<void>(program, 25, table);
    if (table != nullptr) {
        Com<long>(table, kRelease);
    }
}

// 0x8A3200 (thiscall, creator slot 9): a vertex shader from an ID3DXBuffer of compiled code (its
// constant table, "main" and the device's profile by default, the code copied into a block of
// `size` bytes). (When the constant table can't be had the made program is leaked: the game's.)
uint8_t* __fastcall CreatorHLSL_CreateVertexShaderFromD3DXBuffer(uint8_t* self, re::Edx, void* buffer, uint32_t size,
                                                                 const char* name, const char* entryPoint,
                                                                 const char* target, void* declaration,
                                                                 uint32_t usage, int softwareVP, int recoverable) {
    uint8_t* program = NewHLSLVertexShader();
    if (program == nullptr) {
        return nullptr;
    }
    re::Ptr32<void> table = nullptr;
    if (D3DXGetShaderConstantTable(BufferPointer(buffer), &table) < 0) {
        return nullptr;
    }
    if (entryPoint == nullptr) {
        entryPoint = kMain;
    }
    if (target == nullptr) {
        target = Import<const char*>(kD3DXGetVertexShaderProfile, ms_pkCreatorDevice);
    }
    void* shader = Renderer_CreateVertexShader(self, BufferPointer(buffer), &declaration, usage, 0, 0,
                                               uint32_t(softwareVP));
    if (shader == nullptr) {
        ShaderFactory_ReportError(1, recoverable, "Failed CreateVertexShader call on %s\n", name);
        Virtual<void>(program, 0, 1u);
        return nullptr;
    }
    void* code = NiMalloc(size);
    uint32_t bytes = BufferSize(buffer);
    crt::MemcpyS(code, bytes, BufferPointer(buffer), bytes);
    FillVertexShader(program, self, name, nullptr, size, code, shader, usage, softwareVP, entryPoint, target, table);
    return program;
}

// 0x8A33D0 (thiscall, creator slot 10): a pixel shader from an ID3DXBuffer (as the vertex one)
uint8_t* __fastcall CreatorHLSL_CreatePixelShaderFromD3DXBuffer(uint8_t* self, re::Edx, void* buffer, uint32_t size,
                                                                const char* name, const char* entryPoint,
                                                                const char* target, int recoverable) {
    uint8_t* program = NewHLSLPixelShader();
    if (program == nullptr) {
        return nullptr;
    }
    re::Ptr32<void> table = nullptr;
    if (D3DXGetShaderConstantTable(BufferPointer(buffer), &table) < 0) {
        return nullptr;
    }
    if (entryPoint == nullptr) {
        entryPoint = kMain;
    }
    if (target == nullptr) {
        target = Import<const char*>(kD3DXGetPixelShaderProfile, ms_pkCreatorDevice);
    }
    void* shader = Renderer_CreatePixelShader(self, BufferPointer(buffer));
    if (shader == nullptr) {
        ShaderFactory_ReportError(1, recoverable, "Failed CreatePixelShader call on %s\n", name);
        Virtual<void>(program, 0, 1u);
        return nullptr;
    }
    void* code = NiMalloc(size);
    uint32_t bytes = BufferSize(buffer);
    crt::MemcpyS(code, bytes, BufferPointer(buffer), bytes);
    FillPixelShader(program, self, name, nullptr, size, code, shader, entryPoint, target, table);
    return program;
}

// the compile failure reported with D3DX's message (copied out) or without one; the message and
// shader buffers let go
inline void ReportCompileError(void* errors, int recoverable, const char* withMessage, const char* withoutMessage,
                               const char* entryPoint, const char* fileName, bool memory) {
    char* message = nullptr;
    if (errors != nullptr) {
        const char* text = static_cast<const char*>(BufferPointer(errors));
        if (text != nullptr) {
            uint32_t size = BufferSize(errors);
            message = static_cast<char*>(NiMalloc(size));
            NiStrcpy(message, size, text);
            if (memory) {
                ShaderFactory_ReportError(1, recoverable, withMessage, entryPoint, message);
            } else {
                ShaderFactory_ReportError(1, recoverable, withMessage, entryPoint, fileName, message);
            }
        }
        Com<long>(errors, kRelease);
    } else if (memory) {
        ShaderFactory_ReportError(1, recoverable, withoutMessage, entryPoint);
    } else {
        ShaderFactory_ReportError(1, recoverable, withoutMessage, entryPoint, fileName);
    }
    NiFree(message);
}

// 0x8A3560 (thiscall): an HLSL file compiled (found through the program directories; the
// creator's flags with the universal ones): the code copied (NiMalloc'd) with its size, the
// constant table given out; reported failures false
uint8_t __fastcall CreatorHLSL_CompileFromFileImpl(uint8_t* self, re::Edx, const char* fileName,
                                                   const char* entryPoint, const char* target, re::Ptr32<void>* code,
                                                   uint32_t* size, re::Ptr32<void>* constantTable, int recoverable) {
    char path[0x104];
    *code = nullptr;
    *size = 0;
    if (fileName == nullptr || *fileName == '\0') {
        ShaderFactory_ReportError(1, recoverable, "Invalid shader file name\n");
        return 0;
    }
    if (ProgramFactory_ResolveProgramFileName(fileName, path, 0x104) == 0) {
        ShaderFactory_ReportError(1, recoverable, "Failed to find shader program file %s\n", fileName);
        return 0;
    }
    void* errors = nullptr;
    void* shader = nullptr;
    uint32_t flags = At<uint32_t>(self, 4) | ms_uiUniversalShaderCreationFlags;
    if (Import<long>(kD3DXCompileShaderFromFileA, static_cast<const char*>(path), static_cast<void*>(nullptr),
                     static_cast<void*>(nullptr), entryPoint, target, flags, &shader, &errors, constantTable) < 0) {
        ReportCompileError(errors, recoverable, "Failed to compile shader %s in file %s\nError: %s\n",
                           "Failed to compile shader %s in file %s\nError: NONE REPORTED\n", entryPoint, fileName,
                           false);
        if (shader != nullptr) {
            Com<long>(shader, kRelease);
        }
        return 0;
    }
    *size = BufferSize(shader);
    *code = NiMalloc(*size);
    uint32_t bytes = *size;
    crt::MemcpyS(*code, bytes, BufferPointer(shader), bytes);
    Com<long>(shader, kRelease);
    if (errors != nullptr) {
        Com<long>(errors, kRelease);
    }
    return 1;
}

// 0x8A3760 (thiscall): an HLSL buffer compiled (as from a file)
uint8_t __fastcall CreatorHLSL_CompileFromBufferImpl(uint8_t* self, re::Edx, const void* buffer, uint32_t length,
                                                     const char* entryPoint, const char* target, re::Ptr32<void>* code,
                                                     uint32_t* size, re::Ptr32<void>* constantTable, int recoverable) {
    *code = nullptr;
    *size = 0;
    if (buffer == nullptr || length == 0) {
        ShaderFactory_ReportError(1, recoverable, "Invalid shader buffer\n");
        return 0;
    }
    void* errors = nullptr;
    void* shader = nullptr;
    uint32_t flags = At<uint32_t>(self, 4) | ms_uiUniversalShaderCreationFlags;
    if (Import<long>(kD3DXCompileShader, buffer, length, static_cast<void*>(nullptr), static_cast<void*>(nullptr),
                     entryPoint, target, flags, &shader, &errors, constantTable) < 0) {
        ReportCompileError(errors, recoverable, "Failed to assemble shader %s from memory\nError: %s\n",
                           "Failed to assemble shader %s from memory\nError: NONE REPORTED\n", entryPoint, nullptr,
                           true);
        if (shader != nullptr) {
            Com<long>(shader, kRelease);
        }
        return 0;
    }
    *size = BufferSize(shader);
    *code = NiMalloc(*size);
    uint32_t bytes = *size;
    crt::MemcpyS(*code, bytes, BufferPointer(shader), bytes);
    Com<long>(shader, kRelease);
    if (errors != nullptr) {
        Com<long>(errors, kRelease);
    }
    return 1;
}

// 0x8A38D0 (thiscall, creator slot 3): CreateVertexShaderFromFile
uint8_t* __fastcall CreatorHLSL_CreateVertexShaderFromFile(uint8_t* self, re::Edx, const char* fileName,
                                                           const char* name, const char* entryPoint,
                                                           const char* target, void* declaration, uint32_t usage,
                                                           int softwareVP, int recoverable) {
    uint8_t* program = NewHLSLVertexShader();
    if (program == nullptr) {
        return nullptr;
    }
    re::Ptr32<void> table = nullptr;
    re::Ptr32<void> code = nullptr;
    uint32_t size = 0;
    if (entryPoint == nullptr) {
        entryPoint = kMain;
    }
    if (target == nullptr) {
        target = Import<const char*>(kD3DXGetVertexShaderProfile, ms_pkCreatorDevice);
    }
    if (CreatorHLSL_CompileFromFile(self, fileName, entryPoint, target, &code, &size, &table, recoverable) == 0) {
        Virtual<void>(program, 0, 1u);
        return nullptr;
    }
    void* shader = Renderer_CreateVertexShader(self, code, &declaration, usage, 0, 0, uint32_t(softwareVP));
    if (shader == nullptr) {
        ShaderFactory_ReportError(1, recoverable, "Failed CreateVertexShader call on %s\n", name);
        Virtual<void>(program, 0, 1u);
        return nullptr;
    }
    FillVertexShader(program, self, name, fileName, size, code, shader, usage, softwareVP, entryPoint, target, table);
    return program;
}

// 0x8A3A80 (thiscall, creator slot 4): CreateVertexShaderFromBuffer
uint8_t* __fastcall CreatorHLSL_CreateVertexShaderFromBuffer(uint8_t* self, re::Edx, const void* buffer,
                                                             uint32_t length, const char* name,
                                                             const char* entryPoint, const char* target,
                                                             void* declaration, uint32_t usage, int softwareVP,
                                                             int recoverable) {
    uint8_t* program = NewHLSLVertexShader();
    if (program == nullptr) {
        return nullptr;
    }
    re::Ptr32<void> table = nullptr;
    re::Ptr32<void> code = nullptr;
    uint32_t size = 0;
    if (entryPoint == nullptr) {
        entryPoint = kMain;
    }
    if (target == nullptr) {
        target = Import<const char*>(kD3DXGetVertexShaderProfile, ms_pkCreatorDevice);
    }
    if (CreatorHLSL_CompileFromBuffer(self, buffer, length, entryPoint, target, &code, &size, &table,
                                      recoverable) == 0) {
        Virtual<void>(program, 0, 1u);
        return nullptr;
    }
    void* shader = Renderer_CreateVertexShader(self, code, &declaration, usage, 0, 0, uint32_t(softwareVP));
    if (shader == nullptr) {
        ShaderFactory_ReportError(1, recoverable, "Failed CreateVertexShader call on %s\n", name);
        Virtual<void>(program, 0, 1u);
        return nullptr;
    }
    FillVertexShader(program, self, name, nullptr, size, code, shader, usage, softwareVP, entryPoint, target, table);
    return program;
}

// 0x8A3C30 (thiscall, creator slot 5): CreatePixelShaderFromFile
uint8_t* __fastcall CreatorHLSL_CreatePixelShaderFromFile(uint8_t* self, re::Edx, const char* fileName,
                                                          const char* name, const char* entryPoint,
                                                          const char* target, int recoverable) {
    uint8_t* program = NewHLSLPixelShader();
    if (program == nullptr) {
        return nullptr;
    }
    re::Ptr32<void> table = nullptr;
    re::Ptr32<void> code = nullptr;
    uint32_t size = 0;
    if (entryPoint == nullptr) {
        entryPoint = kMain;
    }
    if (target == nullptr) {
        target = Import<const char*>(kD3DXGetPixelShaderProfile, ms_pkCreatorDevice);
    }
    if (CreatorHLSL_CompileFromFile(self, fileName, entryPoint, target, &code, &size, &table, recoverable) != 0) {
        void* shader = Renderer_CreatePixelShader(self, code);
        if (shader != nullptr) {
            FillPixelShader(program, self, name, fileName, size, code, shader, entryPoint, target, table);
            return program;
        }
        ShaderFactory_ReportError(1, recoverable, "Failed CreatePixelShader call on %s\n", name);
    }
    Virtual<void>(program, 0, 1u);
    return nullptr;
}

// 0x8A3DA0 (thiscall, creator slot 6): CreatePixelShaderFromBuffer
uint8_t* __fastcall CreatorHLSL_CreatePixelShaderFromBuffer(uint8_t* self, re::Edx, const void* buffer,
                                                            uint32_t length, const char* name,
                                                            const char* entryPoint, const char* target,
                                                            int recoverable) {
    uint8_t* program = NewHLSLPixelShader();
    if (program == nullptr) {
        return nullptr;
    }
    re::Ptr32<void> table = nullptr;
    re::Ptr32<void> code = nullptr;
    uint32_t size = 0;
    if (entryPoint == nullptr) {
        entryPoint = kMain;
    }
    if (target == nullptr) {
        target = Import<const char*>(kD3DXGetPixelShaderProfile, ms_pkCreatorDevice);
    }
    if (CreatorHLSL_CompileFromBuffer(self, buffer, length, entryPoint, target, &code, &size, &table,
                                      recoverable) != 0) {
        void* shader = Renderer_CreatePixelShader(self, code);
        if (shader != nullptr) {
            FillPixelShader(program, self, name, nullptr, size, code, shader, entryPoint, target, table);
            return program;
        }
        ShaderFactory_ReportError(1, recoverable, "Failed CreatePixelShader call on %s\n", name);
    }
    Virtual<void>(program, 0, 1u);
    return nullptr;
}

// 0x8A3F10 (thiscall): NiD3DHLSLPixelShader::~NiD3DHLSLPixelShader
void __fastcall HLSLPixelShader_DestructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kHLSLPixelShaderVtable;
    NiFree(At<void*>(self, 0x30));
    NiFree(At<void*>(self, 0x34));
    if (At<void*>(self, 0x38) != nullptr) {
        Com<long>(At<void*>(self, 0x38), kRelease);
        At<void*>(self, 0x38) = nullptr;
    }
    PixelShader_Destruct(self);
}

// 0x8A3F50 (thiscall, slot 18): NiD3DHLSLPixelShader::DestroyRendererData (the constant table
// released, then the D3D shader's)
void __fastcall HLSLPixelShader_DestroyRendererData(uint8_t* self, re::Edx) {
    if (At<void*>(self, 0x38) != nullptr) {
        Com<long>(At<void*>(self, 0x38), kRelease);
        At<void*>(self, 0x38) = nullptr;
    }
    PixelShader_DestroyRendererData(self);
}

// 0x8A3F80 / 0x8A4520: RecreateRendererData, lone jmps to NiD3DPixelShader's / NiD3DVertexShader's
// (0x8A7170 / 0x8A7340)
RE_UNHOOKED("NiD3DHLSLPixelShader::RecreateRendererData", 0x008A3F80);
RE_UNHOOKED("NiD3DHLSLVertexShader::RecreateRendererData", 0x008A4520);

// 0x8A3F90 / 0x8A4530 (thiscall, slot 3): GetVariableCount (the constant table's constants; 0
// without a table or when GetDesc fails)
uint32_t __fastcall HLSLPixelShader_GetVariableCount(uint8_t* self, re::Edx) {
    void* table = At<void*>(self, 0x38);
    if (table == nullptr) {
        return 0;
    }
    ConstantTableDesc desc;
    long result = Com<long>(table, 5, static_cast<void*>(&desc));
    return result < 0 ? 0 : desc.constants;
}
uint32_t __fastcall HLSLVertexShader_GetVariableCount(uint8_t* self, re::Edx) {
    void* table = At<void*>(self, 0x44);
    if (table == nullptr) {
        return 0;
    }
    ConstantTableDesc desc;
    long result = Com<long>(table, 5, static_cast<void*>(&desc));
    return result < 0 ? 0 : desc.constants;
}

// 0x8A3FD0 / 0x8A4570 (thiscall, slot 4): GetVariableName (the index'th constant's name; null when
// there's none)
const char* __fastcall HLSLPixelShader_GetVariableName(uint8_t* self, re::Edx, uint32_t index) {
    void* handle = Com<void*>(At<void*>(self, 0x38), 8, static_cast<void*>(nullptr), index);
    if (handle == nullptr) {
        return nullptr;
    }
    ConstantDesc desc;
    uint32_t count = 1;
    if (Com<long>(At<void*>(self, 0x38), 6, handle, &desc, &count) < 0 || count == 0) {
        return nullptr;
    }
    return desc.name;
}
const char* __fastcall HLSLVertexShader_GetVariableName(uint8_t* self, re::Edx, uint32_t index) {
    void* handle = Com<void*>(At<void*>(self, 0x44), 8, static_cast<void*>(nullptr), index);
    if (handle == nullptr) {
        return nullptr;
    }
    ConstantDesc desc;
    uint32_t count = 1;
    if (Com<long>(At<void*>(self, 0x44), 6, handle, &desc, &count) < 0 || count == 0) {
        return nullptr;
    }
    return desc.name;
}

// 0x8A4030 (thiscall, slot 14): NiD3DHLSLPixelShader::SetShaderConstant
uint8_t __fastcall HLSLPixelShader_SetShaderConstant(uint8_t* self, re::Edx, uint8_t* entry, const void* data,
                                                     uint32_t) {
    return SetShaderConstant<0x38, kPixelRanges, ConstantManager_SetPixelShaderConstantF,
                             ConstantManager_SetPixelShaderConstantI>(
        self, entry, data, "NiD3DHLSLPixelShader::SetShaderConstant - %s - variable %s not found\n");
}

// 0x8A4290 (thiscall, slot 15): NiD3DHLSLPixelShader::SetShaderConstantArray (the entry's registers
// in elements of `perElement`, each (remapped, when given) set as floats, transposed when the
// constant is column-major)
uint8_t __fastcall HLSLPixelShader_SetShaderConstantArray(uint8_t* self, re::Edx, uint8_t* entry, const uint8_t* data,
                                                          uint32_t, uint32_t perElement, const uint16_t* remap) {
    void* table = At<void*>(self, 0x38);
    if (table == nullptr || At<void*>(self, 0x20) == nullptr) {
        return 0;
    }
    if (data == nullptr) {
        data = At<const uint8_t*>(entry, 0x2C);
    }
    ConstantDesc desc = {};
    if (!ArrayDesc(self, table, entry, &desc,
                   "NiD3DHLSLPixelShader::SetShaderConstantArray - %s - variable %s not found\n")) {
        return 0;
    }
    float transposed[16];
    uint32_t elements = At<uint32_t>(entry, 0x1C) / perElement;
    bool columns = desc.constantClass == 3;
    uint32_t start = At<uint32_t>(entry, 0x18);
    if (remap != nullptr) {
        for (uint32_t i = 0; i < elements; i++) {
            const void* from = data + remap[i] * perElement * 0x10;
            if (columns) {
                MatrixTranspose(transposed, from);
                from = transposed;
            }
            if (ConstantManager_SetPixelShaderConstantF(At<void*>(At<uint8_t*>(self, 0x28), 0x20F0), start, from,
                                                        perElement) == 0) {
                return 0;
            }
            start += perElement;
        }
        return 1;
    }
    for (uint32_t i = 0; i < elements; i++) {
        const void* from = data;
        if (columns) {
            MatrixTranspose(transposed, data);
            from = transposed;
        }
        if (ConstantManager_SetPixelShaderConstantF(At<void*>(At<uint8_t*>(self, 0x28), 0x20F0), start, from,
                                                    perElement) == 0) {
            return 0;
        }
        data += perElement * 0x10;
        start += perElement;
    }
    return 1;
}

// 0x8A44B0 (thiscall): NiD3DHLSLVertexShader::~NiD3DHLSLVertexShader
void __fastcall HLSLVertexShader_DestructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kHLSLVertexShaderVtable;
    NiFree(At<void*>(self, 0x3C));
    NiFree(At<void*>(self, 0x40));
    if (At<void*>(self, 0x44) != nullptr) {
        Com<long>(At<void*>(self, 0x44), kRelease);
        At<void*>(self, 0x44) = nullptr;
    }
    VertexShader_Destruct(self);
}

// 0x8A44F0 (thiscall, slot 24): NiD3DHLSLVertexShader::DestroyRendererData
void __fastcall HLSLVertexShader_DestroyRendererData(uint8_t* self, re::Edx) {
    if (At<void*>(self, 0x44) != nullptr) {
        Com<long>(At<void*>(self, 0x44), kRelease);
        At<void*>(self, 0x44) = nullptr;
    }
    VertexShader_DestroyRendererData(self);
}

// 0x8A45D0 (thiscall, slot 14): NiD3DHLSLVertexShader::SetShaderConstant
uint8_t __fastcall HLSLVertexShader_SetShaderConstant(uint8_t* self, re::Edx, uint8_t* entry, const void* data,
                                                      uint32_t) {
    return SetShaderConstant<0x44, kVertexRanges, ConstantManager_SetVertexShaderConstantF,
                             ConstantManager_SetVertexShaderConstantI>(
        self, entry, data, "NiD3DHLSLVertexShader::SetShaderConstant - %s - variable %s not found\n");
}

// 0x8A4830 (thiscall, slot 15): NiD3DHLSLVertexShader::SetShaderConstantArray (`count` elements of
// `perElement` registers; not transposed for the skinning bone matrices (mappings 0x21..0x24);
// without a remap and not transposed, all the entry's registers in one call)
uint8_t __fastcall HLSLVertexShader_SetShaderConstantArray(uint8_t* self, re::Edx, uint8_t* entry, const uint8_t* data,
                                                           uint32_t count, uint32_t perElement,
                                                           const uint16_t* remap) {
    void* table = At<void*>(self, 0x44);
    if (table == nullptr || At<void*>(self, 0x20) == nullptr) {
        return 0;
    }
    if (data == nullptr) {
        data = At<const uint8_t*>(entry, 0x2C);
    }
    ConstantDesc desc = {};
    if (!ArrayDesc(self, table, entry, &desc,
                   "NiD3DHLSLVertexShader::SetShaderConstantArray - %s - variable %s not found\n")) {
        return 0;
    }
    float transposed[16];
    bool columns = desc.constantClass == 3;
    uint32_t mapping = At<uint32_t>(entry, 0xC);
    if (mapping == 0x21 || mapping == 0x23 || mapping == 0x22 || mapping == 0x24) {
        columns = false;
    }
    if (remap != nullptr) {
        uint32_t start = At<uint32_t>(entry, 0x18);
        for (uint32_t i = 0; i < count; i++) {
            const void* from = data + remap[i] * perElement * 0x10;
            if (columns) {
                MatrixTranspose(transposed, from);
                from = transposed;
            }
            if (ConstantManager_SetVertexShaderConstantF(At<void*>(At<uint8_t*>(self, 0x28), 0x20F0), start, from,
                                                         perElement) == 0) {
                return 0;
            }
            start += perElement;
        }
        return 1;
    }
    if (!columns) {
        return ConstantManager_SetVertexShaderConstantF(At<void*>(At<uint8_t*>(self, 0x28), 0x20F0),
                                                        At<uint32_t>(entry, 0x18), data,
                                                        At<uint32_t>(entry, 0x1C)) != 0 ? 1 : 0;
    }
    uint32_t start = At<uint32_t>(entry, 0x18);
    for (uint32_t i = 0; i < count; i++) {
        MatrixTranspose(transposed, data);
        if (ConstantManager_SetVertexShaderConstantF(At<void*>(At<uint8_t*>(self, 0x28), 0x20F0), start, transposed,
                                                     perElement) == 0) {
            return 0;
        }
        start += perElement;
        data += perElement * 0x10;
    }
    return 1;
}

#define C41H_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C41H_REVERSED("NiDX9VertexBufferManager::PoolEntry::ReleaseBuffer", 0x008A2C50, PoolEntry_ReleaseBuffer);
C41H_REVERSED("NiD3DShaderProgramCreatorHLSL::RecreateVertexShader", 0x008A2C70, CreatorHLSL_RecreateVertexShader);
C41H_REVERSED("NiD3DShaderProgramCreatorHLSL::RecreatePixelShader", 0x008A2D00, CreatorHLSL_RecreatePixelShader);
C41H_REVERSED("NiD3DShaderProgram::SetName", 0x008A2D70, ShaderProgram_SetName);
C41H_REVERSED("NiD3DShaderProgram::SetShaderProgramName", 0x008A2E20, ShaderProgram_SetShaderProgramName);
C41H_REVERSED("NiD3DShaderProgram::SetCode", 0x008A2ED0, ShaderProgram_SetCode);
C41H_REVERSED("NiD3DShaderProgram::SetCreator", 0x008A2EF0, ShaderProgram_SetCreator);
C41H_REVERSED("NiD3DPixelShader::GetRTTI", 0x008A2F00, PixelShader_GetRTTI);
C41H_REVERSED("NiD3DPixelShader::DeletingDestructor", 0x008A2F10, PixelShader_DeletingDestructor);
C41H_REVERSED("NiD3DVertexShader::GetRTTI", 0x008A2F30, VertexShader_GetRTTI);
C41H_REVERSED("NiD3DVertexShader::DeletingDestructor", 0x008A2F40, VertexShader_DeletingDestructor);
C41H_REVERSED("NiD3DVertexShader::SetUsage", 0x008A2F60, VertexShader_SetUsage);
C41H_REVERSED("NiD3DVertexShader::SetShaderHandle", 0x008A2F80, VertexShader_SetShaderHandle);
C41H_REVERSED("NiD3DVertexShader::SetSoftwareVertexProcessing", 0x008A2FA0, VertexShader_SetSoftwareVertexProcessing);
C41H_REVERSED("NiD3DHLSLPixelShader::GetRTTI", 0x008A2FB0, HLSLPixelShader_GetRTTI);
C41H_REVERSED("NiD3DHLSLPixelShader::DeletingDestructor", 0x008A2FC0, HLSLPixelShader_DeletingDestructor);
C41H_REVERSED("NiD3DHLSLPixelShader::SetEntryPoint", 0x008A2FE0, HLSLPixelShader_SetEntryPoint);
C41H_REVERSED("NiD3DHLSLPixelShader::SetShaderTarget", 0x008A3040, HLSLPixelShader_SetShaderTarget);
C41H_REVERSED("NiD3DHLSLPixelShader::SetConstantTable", 0x008A30B0, HLSLPixelShader_SetConstantTable);
C41H_REVERSED("NiD3DHLSLVertexShader::GetRTTI", 0x008A30E0, HLSLVertexShader_GetRTTI);
C41H_REVERSED("NiD3DHLSLVertexShader::DeletingDestructor", 0x008A30F0, HLSLVertexShader_DeletingDestructor);
C41H_REVERSED("NiD3DHLSLVertexShader::SetEntryPoint", 0x008A3110, HLSLVertexShader_SetEntryPoint);
C41H_REVERSED("NiD3DHLSLVertexShader::SetShaderTarget", 0x008A3170, HLSLVertexShader_SetShaderTarget);
C41H_REVERSED("NiD3DHLSLVertexShader::SetConstantTable", 0x008A31D0, HLSLVertexShader_SetConstantTable);
C41H_REVERSED("NiD3DShaderProgramCreatorHLSL::CreateVertexShaderFromD3DXBuffer", 0x008A3200,
              CreatorHLSL_CreateVertexShaderFromD3DXBuffer);
C41H_REVERSED("NiD3DShaderProgramCreatorHLSL::CreatePixelShaderFromD3DXBuffer", 0x008A33D0,
              CreatorHLSL_CreatePixelShaderFromD3DXBuffer);
C41H_REVERSED("NiD3DShaderProgramCreatorHLSL::CompileFromFile", 0x008A3560, CreatorHLSL_CompileFromFileImpl);
C41H_REVERSED("NiD3DShaderProgramCreatorHLSL::CompileFromBuffer", 0x008A3760, CreatorHLSL_CompileFromBufferImpl);
C41H_REVERSED("NiD3DShaderProgramCreatorHLSL::CreateVertexShaderFromFile", 0x008A38D0,
              CreatorHLSL_CreateVertexShaderFromFile);
C41H_REVERSED("NiD3DShaderProgramCreatorHLSL::CreateVertexShaderFromBuffer", 0x008A3A80,
              CreatorHLSL_CreateVertexShaderFromBuffer);
C41H_REVERSED("NiD3DShaderProgramCreatorHLSL::CreatePixelShaderFromFile", 0x008A3C30,
              CreatorHLSL_CreatePixelShaderFromFile);
C41H_REVERSED("NiD3DShaderProgramCreatorHLSL::CreatePixelShaderFromBuffer", 0x008A3DA0,
              CreatorHLSL_CreatePixelShaderFromBuffer);
C41H_REVERSED("NiD3DHLSLPixelShader::~NiD3DHLSLPixelShader", 0x008A3F10, HLSLPixelShader_DestructImpl);
C41H_REVERSED("NiD3DHLSLPixelShader::DestroyRendererData", 0x008A3F50, HLSLPixelShader_DestroyRendererData);
C41H_REVERSED("NiD3DHLSLPixelShader::GetVariableCount", 0x008A3F90, HLSLPixelShader_GetVariableCount);
C41H_REVERSED("NiD3DHLSLPixelShader::GetVariableName", 0x008A3FD0, HLSLPixelShader_GetVariableName);
C41H_REVERSED("NiD3DHLSLPixelShader::SetShaderConstant", 0x008A4030, HLSLPixelShader_SetShaderConstant);
C41H_REVERSED("NiD3DHLSLPixelShader::SetShaderConstantArray", 0x008A4290, HLSLPixelShader_SetShaderConstantArray);
C41H_REVERSED("NiD3DHLSLVertexShader::~NiD3DHLSLVertexShader", 0x008A44B0, HLSLVertexShader_DestructImpl);
C41H_REVERSED("NiD3DHLSLVertexShader::DestroyRendererData", 0x008A44F0, HLSLVertexShader_DestroyRendererData);
C41H_REVERSED("NiD3DHLSLVertexShader::GetVariableCount", 0x008A4530, HLSLVertexShader_GetVariableCount);
C41H_REVERSED("NiD3DHLSLVertexShader::GetVariableName", 0x008A4570, HLSLVertexShader_GetVariableName);
C41H_REVERSED("NiD3DHLSLVertexShader::SetShaderConstant", 0x008A45D0, HLSLVertexShader_SetShaderConstant);
C41H_REVERSED("NiD3DHLSLVertexShader::SetShaderConstantArray", 0x008A4830, HLSLVertexShader_SetShaderConstantArray);

} // namespace ni::c41
