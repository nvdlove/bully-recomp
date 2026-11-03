// The end of the D3D10 renderer's range:
//
// NiD3D10RendererSDM (one instance at 0x20E8F2C, constructed by a static initializer): its Init
// and Shutdown (the renderer's statics, the packing tables, the render state group and pass pools,
// the shader factory, the shader program factory, the declaration's type tables; undone in
// reverse), handed to NiStaticDataManager by the first one constructed.
//
// NiD3D10ShaderProgram (vtable 0x9A14F4, NiRTTI 0x20E91E4 on NiGPUProgram's; 0x24 bytes): an
// NiRefObject with +0x8 the program type, +0xC its name (an NiFixedString), +0x10 its compiled
// code (an ID3D10Blob, held), +0x18 the D3D10 shader (held), +0x1C / +0x20 its variables' names
// (an NiFixedString array, new[]'d) and their count. Here its destructors, GetRTTI and the
// variable names' getter (its constructor, 0x8BAC40, is the next chunk's).
//
// NiD3D10ShaderConstantMap (vtable 0x9A1530, 0x28 bytes, constructed by 0x8BF210): its AddEntry
// (by the entry's kind: predefined, constant, attribute, global, operator, object), its entries
// by name and by index (an NiTPrimitiveArray at +0xC: +0x10 the entries, +0x16 the count, +0x18
// the count in use), and the static helpers it lays its constants out with in 16-byte registers:
// a register / component packed in one word (0x10000000 | component << 24 | register, 0x800000 a
// flag), registers allocated for a constant of some rows and columns, the constant data copied
// into a buffer register by register.

#include "gamebryo/NiD3D10C43.h"
#include "reversed.h"

namespace ni::c43 {
namespace {

using ni::sys::FixedStringRelease;
using ni::sys::kNullFixedString;

constexpr uintptr_t kShaderProgramVtable = 0x009A14F4;
constexpr uintptr_t kSDMInit = 0x008BAB80;
constexpr uintptr_t kSDMShutdown = 0x008BABC0;

GAME_VAR(uint8_t, NiRTTI_NiD3D10ShaderProgram, 0x020E91E4);
GAME_VAR(uint8_t, NiD3D10RendererSDM_ms_bInitialized, 0x020E91DC);
GAME_VAR(int32_t, NiD3D10RendererSDM_ms_iInitCount, 0x020E91E0);
GAME_ARRAY(uint32_t, ms_auiObjectTypes, 0x020E4040);   // the object constants' types (by the flags' byte 1)

GAME_FN(void, __cdecl, NiStaticDataManager_AddLibrary, 0x00753680, void(__cdecl* init)(), void(__cdecl* shutdown)());
GAME_FN(void*, __thiscall, FixedStringArray_VectorDeletingDestructor, 0x0075B1B0, void* array, uint32_t flags);
GAME_FN(int, __cdecl, NiStricmp, 0x00753820, const char* a, const char* b);
// what Init and Shutdown call (the renderer's, the pools', the factories')
GAME_FN(void, __cdecl, D3D10Renderer_InitStatics, 0x008AF960);
GAME_FN(void, __cdecl, GeometryPacker_InitPackingFunctions, 0x008B3530);
GAME_FN(void, __cdecl, RenderStateGroup_CreatePools, 0x008C0940);
GAME_FN(void, __cdecl, ShaderFactory_Create, 0x008BA590);
GAME_FN(void, __cdecl, ProgramFactory_Create, 0x008C2CE0);
GAME_FN(void, __cdecl, ShaderDeclaration_InitTypeTables, 0x008B4010);
GAME_FN(void, __cdecl, Init_8C4180, 0x008C4180);
GAME_FN(void*, __cdecl, NiMemObject_New_, 0x00752460, uint32_t size);   // NiMemObject::operator new
GAME_FN(void, __cdecl, ProgramFactory_RegisterShaderCreator, 0x008C2BE0, const char* extension, void* creator);
GAME_VAR(void*, ms_pkProgramCreator, 0x020E9304);
GAME_FN(void, __cdecl, Shutdown_8AB260, 0x008AB260);
GAME_FN(void, __cdecl, Shutdown_8C41F0, 0x008C41F0);
GAME_FN(void, __cdecl, NoOp_49A650, 0x0049A650);
GAME_FN(void, __cdecl, ProgramFactory_Shutdown, 0x008C21D0);
GAME_FN(void, __cdecl, GeometryPacker_ShutdownPackingFunctions, 0x008B33D0);
GAME_FN(void, __cdecl, ShaderFactory_Shutdown, 0x008B9950);
GAME_FN(void, __cdecl, RenderStateGroup_DestroyPools, 0x008C0990);
GAME_FN(void, __cdecl, D3D10Renderer_ShutdownStatics, 0x008AFA90);
// this file's (called where modes apply)
GAME_FN(void, __thiscall, ShaderProgram_Destruct, 0x008BADF0, void* self);
GAME_FN(void, __cdecl, ConstantMap_AllocateRegisters, 0x008BB030, uint32_t* row, uint32_t* column, uint32_t rows,
        uint32_t columns, uint32_t* start, uint32_t* packed, int flag);


// 0x8C4180: the program creator (an 8-byte object: its vtable 0x9A1700, then 0) made and
// registered for its four extensions (the strings at 0x99D1C8, 0x99DBB8, 0x9A1730, 0x99DBB4).
// The tail of the SDM's Init; not in Ghidra's list: the 64-bit dispatch's body
void __cdecl Init8C4180Impl() {
    auto* creator = static_cast<uint32_t*>(NiMemObject_New_(8));
    if (creator != nullptr) {
        creator[0] = 0x009A1700;
        creator[1] = 0;
    }
    ms_pkProgramCreator = creator;
    static const uintptr_t kExtensions[] = {0x0099D1C8, 0x0099DBB8, 0x009A1730, 0x0099DBB4};
    for (uintptr_t name : kExtensions) {
        ProgramFactory_RegisterShaderCreator(reinterpret_cast<const char*>(name), ms_pkProgramCreator);
    }
}

} // namespace

// ---- NiD3D10RendererSDM

// 0x8BAB80: NiD3D10RendererSDM::Init
void __cdecl D3D10RendererSDM_Init() {
    if (NiD3D10RendererSDM_ms_bInitialized != 0) {
        return;
    }
    NiD3D10RendererSDM_ms_bInitialized = 1;
    D3D10Renderer_InitStatics();
    GeometryPacker_InitPackingFunctions();
    RenderStateGroup_CreatePools();
    ShaderFactory_Create();
    ProgramFactory_Create();
    ShaderDeclaration_InitTypeTables();
    Init_8C4180();   // (a tail jump)
}

// 0x8BABC0: NiD3D10RendererSDM::Shutdown
void __cdecl D3D10RendererSDM_Shutdown() {
    if (NiD3D10RendererSDM_ms_bInitialized == 0) {
        return;
    }
    NiD3D10RendererSDM_ms_bInitialized = 0;
    Shutdown_8AB260();
    Shutdown_8C41F0();
    NoOp_49A650();
    ProgramFactory_Shutdown();
    GeometryPacker_ShutdownPackingFunctions();
    ShaderFactory_Shutdown();
    RenderStateGroup_DestroyPools();
    D3D10Renderer_ShutdownStatics();   // (a tail jump)
}

// 0x8BAC00 (thiscall): NiD3D10RendererSDM::NiD3D10RendererSDM (the first one hands Init and
// Shutdown to NiStaticDataManager)
void* __fastcall D3D10RendererSDM_Construct(void* self, re::Edx) {
    if (NiD3D10RendererSDM_ms_iInitCount++ == 0) {
        NiStaticDataManager_AddLibrary(reinterpret_cast<void(__cdecl*)()>(kSDMInit),
                                       reinterpret_cast<void(__cdecl*)()>(kSDMShutdown));
    }
    return self;
}

// ---- NiD3D10ShaderProgram

// 0x8BAC30 (thiscall, 4 bytes: under a hook's size): the address of +0xC (the name; the same code
// folded with other classes' getters)
void* __fastcall ShaderProgram_GetName(uint8_t* self, re::Edx) {
    return self + 0xC;
}
RE_UNHOOKED_FN("NiD3D10ShaderProgram::GetName", 0x008BAC30, &ShaderProgram_GetName);

// 0x8BACA0 (thiscall, slot 2): NiD3D10ShaderProgram::GetRTTI
void* __fastcall ShaderProgram_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiD3D10ShaderProgram;
}

// 0x8BACB0 (thiscall): a variable's name by index, returned as an NiFixedString (held; the empty
// one past the count)
re::Ptr32<char>* __fastcall ShaderProgram_GetVariableName(uint8_t* self, re::Edx, re::Ptr32<char>* result, uint32_t index) {
    if (index < At<uint32_t>(self, 0x20)) {
        re::Ptr32<char>* name = &At<re::Ptr32<char>*>(self, 0x1C)[index];
        if (*name != kNullFixedString) {
            Increment(*name - 8);
        }
        *result = *name;
        return result;
    }
    *result = kNullFixedString;
    return result;
}

// 0x8BADF0 (thiscall): NiD3D10ShaderProgram::~NiD3D10ShaderProgram (the names deleted, the shader
// and code let go, the name let go; NiRefObject's)
void __fastcall ShaderProgram_DestructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kShaderProgramVtable;
    void* names = At<void*>(self, 0x1C);
    if (names != nullptr) {
        FixedStringArray_VectorDeletingDestructor(names, 3);
        At<void*>(self, 0x1C) = nullptr;
        At<uint32_t>(self, 0x20) = 0;
    }
    ComRelease(At<void*>(self, 0x18));
    At<void*>(self, 0x18) = nullptr;
    ComRelease(At<void*>(self, 0x10));
    FixedStringRelease(&At<char*>(self, 0xC));
    At<uint32_t>(self, 0) = kRefObjectVtable;
    Decrement(&ms_uiObjects);
}

// 0x8BAEC0 (thiscall, slot 0): NiD3D10ShaderProgram's deleting destructor
uint8_t* __fastcall ShaderProgram_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    ShaderProgram_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x24);
    }
    return self;
}

// ---- NiD3D10ShaderConstantMap's static helpers

// 0x8BAEE0 (cdecl): a register and component packed (0x10000000 | component << 24 | register, the
// flag 0x800000); true
uint8_t __cdecl ConstantMap_PackRegister(uint32_t* packed, uint32_t reg, uint32_t component, int flag) {
    *packed = (re::Flag(flag) ? 0x800000u : 0u) | (component | 0x10) << 24 | reg;
    return 1;
}

// 0x8BAF10 (cdecl): a packed register unpacked (false when it isn't one)
uint8_t __cdecl ConstantMap_UnpackRegister(uint32_t packed, uint32_t* reg, uint32_t* component, uint8_t* flag) {
    if ((packed & 0xF0000000) != 0x10000000) {
        return 0;
    }
    *reg = packed & 0xFFFF;
    *component = (packed >> 0x18) & 0xF;
    *flag = uint8_t((packed >> 0x17) & 1);
    return 1;
}

// 0x8BAF50 (cdecl): constant data copied into a register buffer (from the register and component
// on; each element's rows (rows + 1 of the columns given; with no columns, the rows of 4), a
// whole register for each but the last row when padding (always with no columns); the elements
// in order or through the remap); 0
uint32_t __cdecl ConstantMap_CopyData(uint32_t* buffer, const uint32_t* source, uint32_t reg, uint32_t component,
                                      uint32_t rows, uint32_t columns, uint32_t count, int pad,
                                      const uint16_t* remap) {
    uint32_t* dest = buffer + reg * 4 + component;
    uint32_t lineCount = rows;
    bool padded = re::Flag(pad);
    if (columns != 0) {
        lineCount = rows + 1;
    } else {
        columns = 4;
        padded = true;
    }
    uint32_t elementBytes = lineCount * columns * 4;
    const uint8_t* cursor = reinterpret_cast<const uint8_t*>(source);
    for (uint32_t i = 0; i < count; i++) {
        const uint32_t* from = remap != nullptr
                                   ? reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(source) +
                                                                       uint32_t(remap[i]) * elementBytes)
                                   : reinterpret_cast<const uint32_t*>(cursor);
        for (uint32_t line = 0; line < lineCount; line++) {
            uint32_t n = (padded && line != lineCount - 1) ? 4 : columns;
            for (uint32_t k = 0; k < n; k++) {
                dest[k] = *from++;
            }
            dest += 4;
        }
        cursor += elementBytes;
    }
    return 0;
}

// 0x8BB030 (cdecl): registers allocated for a constant of some rows and columns from the current
// register / component (a new register when it doesn't fit in this one or has rows and this one's
// begun): its start and its packed extent (component count << 24 | registers | the flag |
// 0x10000000); the current position moved past it
void __cdecl ConstantMap_AllocateRegistersImpl(uint32_t* row, uint32_t* column, uint32_t rows, uint32_t columns,
                                               uint32_t* start, uint32_t* packed, int flag) {
    uint32_t c = *column;
    uint32_t r = *row;
    if ((rows != 0 && c != 0) || c + columns > 4) {
        r++;
        c = 0;
    }
    uint32_t end = r + rows;
    uint32_t last = c + columns;
    if (last > 4) {
        uint32_t extra = ((last - 5) >> 2) + 1;
        end += extra;
        for (; extra != 0; extra--) {
            last -= 4;
        }
    }
    *start = (c | 0x10) << 24 | r;
    *packed = (last - c) << 24 | (end - r) | (uint8_t(flag) != 0 ? 0x800000u : 0u) | 0x10000000;
    if (last == 4) {
        *row = end + 1;
        *column = 0;
        return;
    }
    *row = end;
    *column = last;
}

// 0x8BB0E0 (cdecl): an array's registers allocated (each element from a fresh register): the first
// element's start and extent, the rest's register counts added to the extent's low half
void __cdecl ConstantMap_AllocateArrayRegisters(uint32_t count, uint32_t* row, uint32_t* column, uint32_t rows,
                                                uint32_t columns, uint32_t* start, uint32_t* packed, int flag) {
    uint32_t elementStart = 0;
    uint32_t elementPacked = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (*column != 0) {
            *column = 0;
            *row += 1;
        }
        ConstantMap_AllocateRegisters(row, column, rows, columns, &elementStart, &elementPacked, flag);
        if (i == 0) {
            *start = elementStart;
            *packed = elementPacked;
        } else {
            *packed = (uint32_t(uint16_t(*packed)) + uint16_t(elementPacked)) | (elementPacked & 0xFFFF0000);
        }
    }
}

// ---- NiD3D10ShaderConstantMap

// 0x8BB180 (thiscall, slot 2): NiD3D10ShaderConstantMap::AddEntry(key, flags, extra, register,
// count, variable name, data size, stride, data, copy): 0x20 when the entry is there already (the
// same flags and extra), else the kind's own adder (slots 3..8) by the flags' top nibble; 0x40
// for an unknown kind
uint32_t __fastcall ConstantMap_AddEntry(uint8_t* self, re::Edx, const char* key, uint32_t flags, uint32_t extra,
                                         uint32_t reg, uint32_t count, const char* variable, uint32_t dataSize,
                                         uint32_t stride, void* data, int copy) {
    uint8_t* existing = Virtual<uint8_t*>(self, 10, key);
    if (existing != nullptr && At<uint32_t>(existing, 0x14) == extra && At<uint32_t>(existing, 0x10) == flags) {
        return 0x20;
    }
    switch (flags & 0xF0000000) {
    case 0x30000000:   // attribute
        return Virtual<uint32_t>(self, 4, key, flags, extra, reg, count, variable, dataSize, stride, data, copy);
    case 0x10000000:   // constant
        return Virtual<uint32_t>(self, 5, key, flags, extra, reg, count, variable, dataSize, stride, data, copy);
    case 0x20000000:   // predefined
        return Virtual<uint32_t>(self, 3, key, extra, reg, variable);
    case 0x40000000:   // global
        return Virtual<uint32_t>(self, 6, key, flags, extra, reg, count, variable, dataSize, stride, data, copy);
    case 0x50000000:   // operator
        return Virtual<uint32_t>(self, 7, key, flags, extra, reg, count, variable);
    case 0x60000000:   // object
        return Virtual<uint32_t>(self, 8, key, reg, variable, extra, ms_auiObjectTypes[(flags >> 8) & 0xFF]);
    default:
        return 0x40;
    }
}

// 0x8BB2F0 (thiscall, slot 10): an entry by name (no case difference; null when none)
void* __fastcall ConstantMap_GetEntry(uint8_t* self, re::Edx, const char* key) {
    uint16_t count = At<uint16_t>(self, 0x16);
    for (uint32_t i = 0; i < count; i++) {
        uint8_t* entry = At<re::Ptr32<uint8_t>*>(self, 0x10)[i];
        if (entry != nullptr && NiStricmp(key, At<const char*>(entry, 8)) == 0) {
            return entry;
        }
    }
    return nullptr;
}

// 0x8BB340 (thiscall, slot 11): an entry by index (null past the count)
void* __fastcall ConstantMap_GetEntryAt(uint8_t* self, re::Edx, uint32_t index) {
    if (index < At<uint16_t>(self, 0x16)) {
        return At<re::Ptr32<void>*>(self, 0x10)[index];
    }
    return nullptr;
}

// 0x8BB360 (thiscall, slot 12): the entries in use
uint16_t __fastcall ConstantMap_GetEntryCount(uint8_t* self, re::Edx) {
    return At<uint16_t>(self, 0x18);
}

#define GB43K_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

GB43K_REVERSED("NiD3D10RendererSDM::Init", 0x008BAB80, D3D10RendererSDM_Init);
GB43K_REVERSED("NiD3D10RendererSDM::Shutdown", 0x008BABC0, D3D10RendererSDM_Shutdown);
GB43K_REVERSED("NiD3D10RendererSDM::NiD3D10RendererSDM", 0x008BAC00, D3D10RendererSDM_Construct);
GB43K_REVERSED("NiD3D10ShaderProgram::GetRTTI", 0x008BACA0, ShaderProgram_GetRTTI);
GB43K_REVERSED("NiD3D10ShaderProgram::GetVariableName", 0x008BACB0, ShaderProgram_GetVariableName);
GB43K_REVERSED("NiD3D10ShaderProgram::~NiD3D10ShaderProgram", 0x008BADF0, ShaderProgram_DestructImpl);
GB43K_REVERSED("NiD3D10ShaderProgram::DeletingDestructor", 0x008BAEC0, ShaderProgram_DeletingDestructor);
GB43K_REVERSED("NiD3D10ShaderConstantMap::PackRegister", 0x008BAEE0, ConstantMap_PackRegister);
GB43K_REVERSED("NiD3D10ShaderConstantMap::UnpackRegister", 0x008BAF10, ConstantMap_UnpackRegister);
GB43K_REVERSED("NiD3D10ShaderConstantMap::CopyData", 0x008BAF50, ConstantMap_CopyData);
GB43K_REVERSED("NiD3D10ShaderConstantMap::AllocateRegisters", 0x008BB030, ConstantMap_AllocateRegistersImpl);
GB43K_REVERSED("NiD3D10ShaderConstantMap::AllocateArrayRegisters", 0x008BB0E0, ConstantMap_AllocateArrayRegisters);
GB43K_REVERSED("NiD3D10ShaderConstantMap::AddEntry", 0x008BB180, ConstantMap_AddEntry);
GB43K_REVERSED("NiD3D10ShaderConstantMap::GetEntry", 0x008BB2F0, ConstantMap_GetEntry);
GB43K_REVERSED("NiD3D10ShaderConstantMap::GetEntryAt", 0x008BB340, ConstantMap_GetEntryAt);
GB43K_REVERSED("NiD3D10ShaderConstantMap::GetEntryCount", 0x008BB360, ConstantMap_GetEntryCount);

RE_UNHOOKED_FN("NiD3D10RendererSDM::Init (its tail, 0x8C4180)", 0x008C4180, &Init8C4180Impl);
} // namespace ni::c43
