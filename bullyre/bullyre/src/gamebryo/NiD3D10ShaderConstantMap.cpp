// NiD3D10ShaderConstantMap (vtable 0x9A1530, 0x28 bytes; NiShaderConstantMap's +0x8 the program
// type, then +0xC its entries: NiTObjectArray<NiShaderConstantMapEntryPtr>, vtable 0x9893C0, +0x10
// the array, +0x14 its allocated size, +0x16 its size, +0x18 the slots used, +0x1A the growth;
// +0x1C its constant buffer (NiD3D10ShaderConstantBuffer below, held), +0x20 the constant
// buffer's name (NiFixedString; "$Globals" when it has none), +0x24 whether the entries' places
// and the buffer are up to date). The DX9 map's (src/gamebryo/NiD3DShaderConstantMap*.cpp)
// counterpart: its entries are placed in a D3D10 constant buffer (registers of four floats; by the
// shader's reflection when there is one, else packed in order) and written into it each draw.
//
// Its slots: 0 the deleting destructor, 2 AddEntry (chunk 43's 0x8BB180), 3 AddPredefinedEntry, 4
// AddAttributeEntry, 5 AddConstantEntry, 6 AddGlobalEntry, 7 AddOperatorEntry, 8 AddObjectEntry, 9
// RemoveEntry, 10 GetEntry, 11 GetEntryAtIndex, 12 (chunk 43's). The constants' values (by kind)
// are in NiD3D10ShaderConstantMap2.cpp.
//
// NiD3D10ShaderConstantBuffer (a guess at the name; vtable 0x9A1564, 0x10 bytes): an NiRefObject
// with +0x8 its NiD3D10DataStream (a constant buffer: kind 4) and +0xC the slot it's bound to
// (-1 at first).

#include "gamebryo/NiD3D10C44.h"
#include "reversed.h"

namespace ni::c44 {
namespace {

constexpr uintptr_t kConstantMapVtable = 0x009A1530;
constexpr uintptr_t kConstantBufferVtable = 0x009A1564;
constexpr uintptr_t kEntryArrayVtable = 0x009893C0;   // NiTObjectArray<NiShaderConstantMapEntryPtr>

GAME_ARRAY(const float, ms_afSinCoefficients, 0x00950A88);   // (the bone matrix defaults' values)
GAME_ARRAY(const float, ms_afCosCoefficients, 0x00950A94);
GAME_VAR(const float, ms_fMinusHalf, 0x00900DFC);
GAME_VAR(char*, ms_kSCMExtraDataName, 0x020E9260);   // (NiFixedString)

// the library
GAME_FN(uint8_t, __cdecl, LookUpPredefinedMapping, 0x0077F210, const char* name, uint32_t* id, uint32_t* count);
GAME_FN(uint32_t, __cdecl, GetPredefinedMappingType, 0x0077EF10, uint32_t id, uint32_t count);
GAME_FN(uint8_t, __cdecl, LookUpObjectMapping, 0x0077F330, const char* name, uint32_t* id);
GAME_FN(uint8_t, __cdecl, ShaderFactory_RegisterGlobalShaderConstant, 0x00756A60, const re::Ptr32<char>* key, uint32_t type,
        uint32_t size, const void* data);
GAME_FN(uint8_t, __cdecl, ShaderFactory_ReleaseGlobalShaderConstant, 0x00756B40, const re::Ptr32<char>* key);
GAME_FN(void*, __thiscall, ObjectNET_GetExtraData, 0x00764250, void* self, const re::Ptr32<char>* name);
GAME_FN(void, __thiscall, ShaderConstantMap_Construct, 0x0077F0C0, void* self, uint32_t programType);
GAME_FN(ConstantMapEntry*, __thiscall, Entry_Construct, 0x0088B520, void* self);
GAME_FN(void, __thiscall, Entry_SetData, 0x0088A500, ConstantMapEntry* self, uint32_t size, uint32_t stride,
        const void* data, int copy);
GAME_FN(uint32_t, __thiscall, EntryArray_AddFirstEmpty, 0x007606C0, void* array, const re::Ptr32<ConstantMapEntry>* entry);
GAME_FN(void, __thiscall, EntryArray_SetAt, 0x0040E800, void* array, uint32_t index, const re::Ptr32<ConstantMapEntry>* entry);
GAME_FN(void, __thiscall, EntryArray_RemoveAll, 0x0040E7A0, void* array);
GAME_FN(void, __cdecl, NiTArray_Free, 0x00836F80, void* p);
// NiD3D10DataStream (chunk 43's): Create(size, usage, kind, result), Resize; +0xC its size; slot 2
// Lock(flags), slot 3 Unlock
GAME_FN(uint8_t, __cdecl, DataStream_Create, 0x008B84E0, uint32_t size, uint32_t usage, uint32_t kind, re::Ptr32<void>* result);
GAME_FN(uint8_t, __thiscall, DataStream_Resize, 0x008B8880, void* self, uint32_t size);
GAME_FN(void*, __thiscall, Object_Get8, 0x00856850, const void* self);   // (a folded getter of +0x8)
GAME_FN(uint32_t, __thiscall, Object_GetC, 0x00853140, const void* self);   // (a folded getter of +0xC)
GAME_FN(void, __thiscall, Object_SetC, 0x008C00F0, void* self, uint32_t value);   // (a folded setter of +0xC)
// the packing helpers (chunk 43's): a register / count word made, read back, and the next place
// for so many registers and components
GAME_FN(uint8_t, __cdecl, PackRegisterCount, 0x008BAEE0, uint32_t* out, uint32_t registers, uint32_t components,
        int flag);
GAME_FN(uint8_t, __cdecl, UnpackRegister, 0x008BAF10, uint32_t packed, uint32_t* reg, uint32_t* component,
        uint8_t* flag);
GAME_FN(uint8_t, __cdecl, PackNext, 0x008BB030, uint32_t* reg, uint32_t* component, uint32_t registers,
        uint32_t components, uint32_t* outReg, uint32_t* outCount, int flag);
GAME_FN(uint8_t, __cdecl, PackNextArray, 0x008BB0E0, uint32_t elements, uint32_t* reg, uint32_t* component,
        uint32_t registers, uint32_t components, uint32_t* outReg, uint32_t* outCount, int flag);
// the values written into the mapped buffer (chunk 43's): data, place, counts, elements, the flag,
// the bone indices (a skinned partition's); 0 returned
GAME_FN(uint32_t, __cdecl, WriteConstant, 0x008BAF50, void* mapped, const void* data, uint32_t reg, uint32_t component,
        uint32_t registers, uint32_t components, uint32_t elements, int flag, const uint16_t* bones);

// this file's (called where modes apply)
GAME_FN(uint32_t, __thiscall, Map_GetEntryIndex, 0x008BB410, void* self, const char* key);
GAME_FN(uint32_t, __cdecl, Map_SetupPredefinedEntry, 0x008BB460, ConstantMapEntry* entry);
GAME_FN(uint32_t, __cdecl, Map_SetupObjectEntry, 0x008BB590, ConstantMapEntry* entry);
GAME_FN(uint32_t, __thiscall, Map_AssignRegisters, 0x008BC000, void* self, void* constantBuffer);
GAME_FN(uint32_t, __thiscall, Map_SetConstantConstant, 0x008BC5C0, void* self, void* mapped, ConstantMapEntry* entry,
        uint32_t pass);
GAME_FN(void, __thiscall, Map_Destruct, 0x008BEBC0, void* self);
GAME_FN(uint8_t, __thiscall, Map_PrepareBuffer, 0x008BED10, void* self, void* constantBuffer, uint32_t slot);
GAME_FN(uint32_t, __thiscall, Map_SetPredefinedConstant, 0x008BEDA0, void* self, void* mapped,
        ConstantMapEntry* entry, void* geometry, void* skin, void* partition, void* bufferData, void* state,
        void* effects, const void* world, const void* bound, uint32_t pass);
GAME_FN(uint32_t, __thiscall, Map_SetAttributeConstant, 0x008BEF30, void* self, void* mapped,
        ConstantMapEntry* entry, void* geometry, void* skin, void* partition, void* bufferData, void* state,
        void* effects, const void* world, const void* bound, uint32_t pass, int global, void* extraData);
GAME_FN(uint32_t, __thiscall, Map_SetOperatorConstant, 0x008BF050, void* self, void* mapped,
        ConstantMapEntry* entry, void* geometry, void* skin, void* state, void* effects, const void* world,
        const void* bound, uint32_t pass, int global, void* extraData);
GAME_FN(uint32_t, __thiscall, Map_SetObjectConstant, 0x008BF3F0, void* self, void* mapped, ConstantMapEntry* entry,
        void* geometry, void* skin, void* partition, void* bufferData, void* state, void* effects,
        const void* world, const void* bound, uint32_t pass);
GAME_FN(uint32_t, __thiscall, Map_InsertEntry, 0x008BF520, void* self, ConstantMapEntry* entry);
GAME_FN(uint32_t, __thiscall, Map_SetShaderConstantsBody, 0x008BF580, void* self, void* geometry, void* skin,
        void* partition, void* bufferData, void* state, void* effects, const void* world, const void* bound,
        uint32_t pass, int global);
GAME_FN(re::Ptr32<char>*, __cdecl, Map_GetSCMExtraDataName, 0x008C0790);
GAME_FN(void*, __thiscall, ConstantBuffer_Construct, 0x008C0100, void* self, uint32_t size, uint32_t usage);
GAME_FN(uint8_t, __cdecl, ConstantBuffer_Create, 0x008C0180, uint32_t size, uint32_t usage, re::Ptr32<void>* result);
// (NiD3D10ShaderConstantMap2.cpp's)
GAME_FN(const void*, __thiscall, Map_GetPredefinedConstant, 0x008BC6D0, void* self, ConstantMapEntry* entry,
        void* geometry, void* skin, void* state, void* effects, const void* world, const void* bound, uint32_t pass,
        float* buffer);
GAME_FN(const void*, __thiscall, Map_GetAttributeConstant, 0x008BD6C0, void* self, ConstantMapEntry* entry,
        void* geometry, void* skin, void* state, void* effects, const void* world, const void* bound, uint32_t pass,
        int global, void* extraData, void* buffer);
GAME_FN(const void*, __thiscall, Map_GetOperatorConstant, 0x008BD870, void* self, ConstantMapEntry* entry,
        void* geometry, void* skin, void* state, void* effects, const void* world, const void* bound, uint32_t pass,
        int global, void* extraData, float* buffer);
GAME_FN(const void*, __thiscall, Map_GetObjectConstant, 0x008BF190, void* self, ConstantMapEntry* entry,
        void* geometry, void* skin, void* state, void* effects, const void* world, const void* bound, uint32_t pass,
        float* buffer);

struct Map {
    re::Ptr32<void> vtable;              // +0x0
    uint32_t refCount;         // +0x4
    uint32_t programType;      // +0x8
    re::Ptr32<void> entriesVtable;       // +0xC
    re::Ptr32<re::Ptr32<ConstantMapEntry>> entries;   // +0x10
    uint16_t maxEntries;       // +0x14
    uint16_t entryCount;       // +0x16
    uint16_t usedEntries;      // +0x18
    uint16_t growBy;           // +0x1A
    re::Ptr32<void> buffer;              // +0x1C (NiD3D10ShaderConstantBuffer, held)
    re::Ptr32<char> bufferName;          // +0x20 (NiFixedString)
    uint8_t valid;             // +0x24
    uint8_t pad25[3];
};
static_assert(offsetof(Map, entriesVtable) == 0xC);
static_assert(offsetof(Map, buffer) == 0x1C);
static_assert(sizeof(Map) == 0x28);

inline void* EntryArray(Map* self) {
    return &self->entriesVtable;
}

// a new entry, its key set
inline ConstantMapEntry* NewEntry(const char* key) {
    void* memory = NiNew(0x34);
    ConstantMapEntry* entry = memory != nullptr ? Entry_Construct(memory) : nullptr;
    SetString(&entry->key, key);
    return entry;
}

// the data copied into a block the entry owns, or pointed at
inline void SetEntryData(ConstantMapEntry* entry, uint32_t size, const void* data, int copy) {
    if (re::Flag(copy)) {
        entry->ownsData = 1;
        NiFree(entry->data);
        void* block = NiMalloc(size);
        entry->data = block;
        crt::MemcpyS(block, size, data, size);
    } else {
        entry->ownsData = 0;
        entry->data = const_cast<void*>(data);
    }
}

// inserted, or deleted again
inline uint32_t InsertOrDelete(Map* self, ConstantMapEntry* entry) {
    uint32_t result = Map_InsertEntry(self, entry);
    if (result != 0) {
        Virtual<void>(entry, 0, 1);
    }
    return result;
}

// a placed entry's register and count read back (the components, the flag)
struct Place {
    uint32_t reg = 0;
    uint32_t component = 0;
    uint32_t registers = 0;
    uint32_t components = 0;
    uint8_t flag = 0;
    bool regValid = false;
    bool countValid = false;
};
inline Place Unpack(const ConstantMapEntry* entry) {
    Place p;
    if ((entry->reg & 0xF0000000u) == 0x10000000u) {
        p.reg = entry->reg & 0xFFFF;
        p.component = (entry->reg >> 24) & 0xF;
        p.flag = uint8_t((entry->reg >> 23) & 1);
        p.regValid = true;
    }
    if ((entry->count & 0xF0000000u) == 0x10000000u) {
        p.registers = entry->count & 0xFFFF;
        p.components = (entry->count >> 24) & 0xF;
        p.flag = uint8_t((entry->count >> 23) & 1);
        p.countValid = true;
    }
    return p;
}

// an array entry's elements (its size over its stride), the registers split over them
inline uint32_t ArrayElements(const ConstantMapEntry* entry, uint32_t& registers) {
    if (ms_auiGlobalTypes[entry->flags & 0xFF] != 0xE) {
        return 1;
    }
    uint32_t elements = entry->size / entry->stride;
    uint32_t perElement = registers / elements;
    if (registers % elements != 0) {
        perElement++;
    }
    registers = perElement;
    return elements;
}

} // namespace

// 0x8BB370 (cdecl): a texture transform's 3x3 matrix as a D3D 4x4 (rows 0, 1 and the translation
// row 3; zeros elsewhere, its 3rd and 4th diagonal too), transposed when asked; the identity
// without one
void __cdecl ConstantMap_MakeTextureTransform(float* out, const float* m, int transpose) {
    if (m == nullptr) {
        out[14] = 0.0f; out[13] = 0.0f; out[12] = 0.0f; out[11] = 0.0f;
        out[9] = 0.0f; out[8] = 0.0f; out[7] = 0.0f; out[6] = 0.0f;
        out[4] = 0.0f; out[3] = 0.0f; out[2] = 0.0f; out[1] = 0.0f;
        out[15] = 1.0f; out[10] = 1.0f; out[5] = 1.0f; out[0] = 1.0f;
        return;
    }
    out[0] = m[0];
    out[1] = m[3];
    out[2] = 0.0f;
    out[3] = 0.0f;
    out[4] = m[1];
    out[5] = m[4];
    out[6] = 0.0f;
    out[7] = 0.0f;
    out[8] = 0.0f;
    out[9] = 0.0f;
    out[10] = 0.0f;
    out[11] = 0.0f;
    out[12] = m[2];
    out[13] = m[5];
    out[14] = 0.0f;
    out[15] = 0.0f;
    if (re::Flag(transpose)) {
        MatrixTranspose(out, out);
    }
}

// 0x8BB410 (thiscall): GetEntryIndex (by key, case blind; -1 when absent)
uint32_t __fastcall ConstantMap_GetEntryIndex(Map* self, re::Edx, const char* key) {
    uint32_t count = self->entryCount;
    for (uint32_t i = 0; i < count; i++) {
        ConstantMapEntry* entry = self->entries[i];
        if (entry != nullptr && NiStricmp(key, entry->key) == 0) {
            return i;
        }
    }
    return 0xFFFFFFFF;
}

// 0x8BB460 (cdecl): SetupPredefinedEntry (the mapping by name and its attribute type; 0x40 when
// unknown; the bone matrix defaults for 0x50 / 0x51 copied into a block of its own)
uint32_t __cdecl ConstantMap_SetupPredefinedEntry(ConstantMapEntry* entry) {
    uint32_t id;
    uint32_t count = 4;
    if (!LookUpPredefinedMapping(entry->key, &id, &count)) {
        return 0x40;
    }
    uint32_t type = GetPredefinedMappingType(id, count);
    entry->flags = (entry->flags & 0xFFFFFF00u) | ms_aucAttributeTypes[type];
    if (id == 0) {
        return 0x40;
    }
    entry->mapping = id;
    float values[4];
    if (id == 0x50) {
        values[0] = 1.0f;
        values[1] = ms_afCosCoefficients[2];
        values[2] = ms_afCosCoefficients[1];
        values[3] = ms_afCosCoefficients[0];
    } else if (id == 0x51) {
        values[0] = ms_fMinusHalf;
        values[1] = ms_afSinCoefficients[2];
        values[2] = ms_afSinCoefficients[1];
        values[3] = ms_afSinCoefficients[0];
    } else {
        return 0;
    }
    entry->size = 0x10;
    entry->stride = 0x10;
    entry->ownsData = 1;
    NiFree(entry->data);
    void* block = NiMalloc(0x10);
    entry->data = block;
    crt::MemcpyS(block, 0x10, values, 0x10);
    return 0;
}

// 0x8BB590 (cdecl): SetupObjectEntry (the object mapping by name, its attribute type; 0x40 when
// unknown)
uint32_t __cdecl ConstantMap_SetupObjectEntry(ConstantMapEntry* entry) {
    uint32_t id;
    if (!LookUpObjectMapping(entry->key, &id)) {
        return 0x40;
    }
    int32_t index = int32_t(id) < 0x1E ? int32_t(id) : 0x1E;
    entry->flags = (entry->flags & 0xFFFFFF00u) | ms_aucAttributeTypes[ms_auiObjectMappings[index * 3 + 1]];
    if (id == 0) {
        return 0x40;
    }
    entry->mapping = id;
    return 0;
}

// 0x8BBF60 (thiscall, slot 9): RemoveEntry (0x10 when absent; a global one's constant let go; the
// layout redone when it was placed)
uint32_t __fastcall ConstantMap_RemoveEntry(Map* self, re::Edx, const char* key) {
    uint32_t index = Map_GetEntryIndex(self, key);
    if (index == 0xFFFFFFFF) {
        return 0x10;
    }
    ConstantMapEntry* entry = self->entries[index];
    if (entry != nullptr) {
        if ((entry->flags & 0xF0000000u) == 0x40000000u) {
            re::Ptr32<char> name = StringAddRef(entry->key);
            ShaderFactory_ReleaseGlobalShaderConstant(&name);
            FixedString_Destruct(&name);
        }
        if (entry->reg != 0xFFFFFFFF) {
            self->valid = 0;
        }
    }
    re::Ptr32<ConstantMapEntry> none = nullptr;
    EntryArray_SetAt(EntryArray(self), index, &none);
    return 0;
}

// 0x8BC000 (thiscall): the entries placed in the constant buffer, its size in bytes returned. With
// the shader's reflection of the buffer (ID3D10ShaderReflectionConstantBuffer: 0 GetDesc, 2
// GetVariableByName): each entry's variable (by its variable name, else its key) and where the
// shader has it (0 when any is missing or of a type it can't be: the size from the description);
// without, packed in order by their types
uint32_t __fastcall ConstantMap_AssignRegisters(Map* self, re::Edx, void* constantBuffer) {
    if (constantBuffer != nullptr) {
        struct BufferDesc {
            re::Ptr32<const char> name;
            uint32_t type;
            uint32_t variables;
            uint32_t size;
            uint32_t flags;
        } desc;
        if (Com<long>(constantBuffer, 0, &desc) < 0) {
            return 0;
        }
        if (desc.type != 0) {   // (not a cbuffer)
            return 0;
        }
        for (uint32_t i = 0; i < self->entryCount; i++) {
            ConstantMapEntry* entry = self->entries[i];
            if (entry == nullptr) {
                continue;
            }
            re::Ptr32<char> name = StringAddRef(entry->variable);
            if (name == NullString()) {
                FixedString_Assign(&name, &entry->key);
            }
            void* variable = Com<void*>(constantBuffer, 2, name);
            void* type = variable != nullptr ? Com<void*>(variable, 1) : nullptr;
            struct TypeDesc {
                uint32_t typeClass;
                uint32_t type;
                uint32_t rows;
                uint32_t columns;
                uint32_t elements;
                uint32_t members;
                uint32_t offset;
            } typeDesc;
            if (type == nullptr || Com<long>(type, 0, &typeDesc) < 0) {
                FixedString_Destruct(&name);
                return 0;
            }
            uint8_t flag = 1;
            uint8_t columnMajor = 1;
            if (typeDesc.type == 3) {   // float
                if (typeDesc.typeClass == 2) {   // a row major matrix
                    columnMajor = 0;
                    if (typeDesc.columns != 4) {
                        flag = 0;
                    }
                } else if (typeDesc.typeClass == 3 && typeDesc.rows != 4) {   // column major
                    flag = 1;
                }
            } else if (typeDesc.type == 2 || typeDesc.type == 1) {   // int, bool: one scalar only
                if (typeDesc.elements != 1 || typeDesc.typeClass != 0) {
                    FixedString_Destruct(&name);
                    return 0;
                }
            }
            struct VariableDesc {
                re::Ptr32<const char> name;
                uint32_t startOffset;
                uint32_t size;
                uint32_t flags;
                void* defaultValue;
            } variableDesc;
            if (Com<long>(variable, 0, &variableDesc) < 0) {
                FixedString_Destruct(&name);
                return 0;
            }
            uint32_t start = variableDesc.startOffset >> 2;
            uint32_t reg = start >> 2;
            uint32_t component = start & 3;
            uint32_t end = (variableDesc.startOffset + variableDesc.size) >> 2;
            uint32_t endReg = end >> 2;
            uint32_t endComponent = end & 3;
            if (component > endComponent) {
                endComponent += 4;
                endReg -= 1;
            }
            uint32_t count;
            PackRegisterCount(&count, endReg - reg, endComponent - component, flag);
            entry->count = count;
            entry->reg = ((component | 0x10) << 24) | reg;
            entry->columnMajor = columnMajor;
            entry->placed = 1;
            FixedString_Destruct(&name);
        }
        return desc.size;
    }

    uint32_t reg = 0;
    uint32_t component = 0;
    for (uint32_t i = 0; i < self->entryCount; i++) {
        ConstantMapEntry* entry = self->entries[i];
        if (entry == nullptr || (entry->reg == 0xFFFFFFFF && entry->variable == nullptr)) {
            continue;
        }
        uint32_t outReg;
        uint32_t outCount;
        uint32_t type = ms_auiGlobalTypes[entry->flags & 0xFF];
        switch (type) {
        case 0:
            if (entry->placed != 0) {
                // (the outputs as the original reads them: its stack when a word isn't a place)
                uint32_t r = 0, c = 0, registers = 0, components = 0;
                uint8_t flag = 0;
                UnpackRegister(entry->reg, &r, &c, &flag);
                UnpackRegister(entry->count, &registers, &components, &flag);
                PackNext(&reg, &component, registers, components, &outReg, &outCount, flag);
            } else {
                PackNext(&reg, &component, entry->count, 0, &outReg, &outCount, 1);
            }
            break;
        case 1:
        case 3:
        case 4:
            PackNext(&reg, &component, 0, 1, &outReg, &outCount, 1);
            break;
        case 5:
            PackNext(&reg, &component, 0, 2, &outReg, &outCount, 1);
            break;
        case 6:
            PackNext(&reg, &component, 0, 3, &outReg, &outCount, 1);
            break;
        case 7:
        case 10:
            PackNext(&reg, &component, 1, 0, &outReg, &outCount, 1);
            break;
        case 8:   // a 3x3 matrix (the skin's bone matrices: one per bone)
            if (entry->mapping == 0x21 || entry->mapping == 0x23) {
                PackNextArray(uint16_t(entry->extra >> 16), &reg, &component, 3, 0, &outReg, &outCount, 0);
            } else if ((entry->flags & 0xF0000000u) == 0x20000000u) {
                PackNext(&reg, &component, 3, 0, &outReg, &outCount, 1);
            } else {
                PackNext(&reg, &component, 2, 3, &outReg, &outCount, 0);
            }
            break;
        case 9:   // a 4x4 matrix
            if (entry->mapping == 0x22 || entry->mapping == 0x24) {
                PackNextArray(uint16_t(entry->extra >> 16), &reg, &component, 4, 0, &outReg, &outCount, 1);
            } else {
                PackNext(&reg, &component, 4, 0, &outReg, &outCount, 1);
            }
            break;
        case 12:
            PackNext(&reg, &component, 2, 0, &outReg, &outCount, 1);
            break;
        case 13:
            PackNext(&reg, &component, 3, 0, &outReg, &outCount, 1);
            break;
        case 14: {   // an array: its elements by the stride
            uint32_t elements = entry->size / entry->stride;
            uint32_t words = entry->stride >> 2;
            uint32_t registers;
            uint32_t components;
            if (words == 9) {
                registers = 2;
                components = 3;
            } else {
                registers = words >> 2;
                components = words & 3;
            }
            PackNextArray(elements, &reg, &component, registers, components, &outReg, &outCount, words != 9);
            break;
        }
        default:
            continue;
        }
        entry->reg = outReg;
        entry->count = outCount;
        entry->placed = 1;
    }
    return ((component + reg * 4) & 0x07FFFFFF) << 2;
}

// 0x8BC5C0 (thiscall): a constant (or global) entry's data written where it's placed (1 when it
// has none or isn't placed)
uint32_t __fastcall ConstantMap_SetConstantConstant(Map*, re::Edx, void* mapped, ConstantMapEntry* entry, uint32_t) {
    if (entry->data == nullptr) {
        return 1;
    }
    Place p = Unpack(entry);
    if (!p.regValid || !p.countValid) {
        return 1;
    }
    uint32_t elements = ArrayElements(entry, p.registers);
    return WriteConstant(mapped, entry->data, p.reg, p.component, p.registers, p.components, elements, p.flag,
                         nullptr);
}


// 0x8BEBC0 (thiscall): the destructor (each global entry's constant let go; the entries, the
// buffer's name and the buffer let go)
void __fastcall ConstantMap_Destruct(Map* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kConstantMapVtable);
    for (uint32_t i = 0; i < self->maxEntries; i++) {
        ConstantMapEntry* entry = self->entries[i];
        if (entry == nullptr) {
            continue;
        }
        Increment(&entry->refCount);
        if ((entry->flags & 0xF0000000u) == 0x40000000u) {
            re::Ptr32<char> name = StringAddRef(entry->key);
            ShaderFactory_ReleaseGlobalShaderConstant(&name);
            StringRelease(&name);
        }
        if (Decrement(&entry->refCount) == 0) {
            Virtual<void>(entry, 1);
        }
    }
    EntryArray_RemoveAll(EntryArray(self));
    StringRelease(&self->bufferName);
    ReleaseRef(self->buffer);
    self->entriesVtable = reinterpret_cast<void*>(kEntryArrayVtable);
    NiTArray_Free(self->entries);
    self->vtable = reinterpret_cast<void*>(kRefObjectVtable);
    Decrement(&ms_uiObjects);
}

// 0x8BED10 (thiscall): the entries placed and the constant buffer made (or grown) for them when
// they've changed, its slot set; whether it's ready
uint8_t __fastcall ConstantMap_PrepareBuffer(Map* self, re::Edx, void* constantBuffer, uint32_t slot) {
    if (self->valid == 0) {
        uint32_t size = Map_AssignRegisters(self, constantBuffer);
        if (self->buffer != nullptr) {
            void* stream = Object_Get8(self->buffer);
            if (Object_GetC(stream) < size && !DataStream_Resize(stream, size)) {
                return 0;
            }
            self->valid = 1;
        } else if (size != 0) {
            uint8_t made = ConstantBuffer_Create((size + 0xF) & 0xFFFFFFF0u, 0x10, &self->buffer);
            if (made == 0) {
                return 0;
            }
            self->valid = made;
        } else {
            self->valid = 1;
        }
        if (self->buffer != nullptr) {
            Object_SetC(self->buffer, slot);
        }
    }
    return self->valid;
}

// 0x8BEDA0 (thiscall): a predefined entry's value written (a skinned partition's bone matrices,
// one per bone, by its bone indices); 1 when it can't be
uint32_t __fastcall ConstantMap_SetPredefinedConstant(Map* self, re::Edx, void* mapped, ConstantMapEntry* entry,
                                                      void* geometry, void* skin, uint8_t* partition, void*,
                                                      void* state, void* effects, const void* world,
                                                      const void* bound, uint32_t pass) {
    Place p = Unpack(entry);
    if (!p.regValid || !p.countValid) {
        return 1;
    }
    alignas(16) float buffer[16];
    const void* data =
        Map_GetPredefinedConstant(self, entry, geometry, skin, state, effects, world, bound, pass, buffer);
    if (data == nullptr) {
        return 1;
    }
    uint32_t registers = p.registers;
    uint32_t elements = ArrayElements(entry, registers);
    const uint16_t* bones = nullptr;
    uint32_t mapping = entry->mapping;
    if (mapping == 0x21 || mapping == 0x23 || mapping == 0x22 || mapping == 0x24) {
        if (partition != nullptr) {
            registers = (mapping == 0x21 || mapping == 0x23) ? 3 : 4;
            elements = uint16_t(entry->extra >> 16);
            uint32_t partitionBones = At<uint16_t>(partition, 0x20);
            if (elements > partitionBones) {
                elements = partitionBones;
            }
            bones = At<const uint16_t*>(partition, 4);
        }
    }
    return WriteConstant(mapped, data, p.reg, p.component, registers, p.components, elements, p.flag, bones);
}

// 0x8BEF30 (thiscall): an attribute entry's value written (1 when it can't be)
uint32_t __fastcall ConstantMap_SetAttributeConstant(Map* self, re::Edx, void* mapped, ConstantMapEntry* entry,
                                                     void* geometry, void* skin, void*, void*, void* state,
                                                     void* effects, const void* world, const void* bound,
                                                     uint32_t pass, int global, void* extraData) {
    if (geometry == nullptr) {
        return 1;
    }
    alignas(16) float buffer[16];
    const void* data = Map_GetAttributeConstant(self, entry, geometry, skin, state, effects, world, bound, pass,
                                                global, extraData, buffer);
    if (data == nullptr) {
        return 1;
    }
    Place p = Unpack(entry);
    if (!p.regValid || !p.countValid) {
        return 1;
    }
    uint32_t elements = ArrayElements(entry, p.registers);
    return WriteConstant(mapped, data, p.reg, p.component, p.registers, p.components, elements, p.flag, nullptr);
}

// 0x8BF050 (thiscall): an operator entry's value written (1 when it can't be)
uint32_t __fastcall ConstantMap_SetOperatorConstant(Map* self, re::Edx, void* mapped, ConstantMapEntry* entry,
                                                    void* geometry, void* skin, void* state, void* effects,
                                                    const void* world, const void* bound, uint32_t pass, int global,
                                                    void* extraData) {
    alignas(16) float buffer[16];
    const void* data = Map_GetOperatorConstant(self, entry, geometry, skin, state, effects, world, bound, pass,
                                               global, extraData, buffer);
    if (data == nullptr) {
        return 1;
    }
    Place p = Unpack(entry);
    if (!p.regValid || !p.countValid) {
        return 1;
    }
    uint32_t elements = ArrayElements(entry, p.registers);
    return WriteConstant(mapped, data, p.reg, p.component, p.registers, p.components, elements, p.flag, nullptr);
}

// 0x8BF210 (thiscall): the constructor (the program type)
Map* __fastcall ConstantMap_Construct(Map* self, re::Edx, uint32_t programType) {
    ShaderConstantMap_Construct(self, programType);
    self->vtable = reinterpret_cast<void*>(kConstantMapVtable);
    self->maxEntries = 0;
    self->growBy = 1;
    self->entryCount = 0;
    self->usedEntries = 0;
    self->entries = nullptr;
    self->entriesVtable = reinterpret_cast<void*>(kEntryArrayVtable);
    self->buffer = nullptr;
    self->bufferName = NullString();
    self->valid = 0;
    EntryArray_RemoveAll(EntryArray(self));
    return self;
}

// 0x8BF260 (thiscall, slot 0): the deleting destructor
Map* __fastcall ConstantMap_DeletingDestructor(Map* self, re::Edx, uint32_t flags) {
    Map_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(Map));
    }
    return self;
}

// 0x8BF280 (thiscall): the shader's reflection (ID3D10ShaderReflection: 3 GetDesc, 4
// GetConstantBufferByIndex, 5 GetConstantBufferByName) of the map's constant buffer ("$Globals"
// when it has no name) found and the entries placed by it, the buffer bound at its index
uint8_t __fastcall ConstantMap_SetShaderReflection(Map* self, re::Edx, void* reflection) {
    re::Ptr32<char> name = StringAddRef(self->bufferName);
    if (name == NullString() || StringLength(name) == 0) {
        FixedString_AssignText(&name, Text(0x009A0288));   // "$Globals"
    }
    void* constantBuffer = Com<void*>(reflection, 5, name);
    if (constantBuffer == nullptr) {
        StringRelease(&name);
        return 0;
    }
    uint32_t desc[0x100];   // D3D10_SHADER_DESC (+0xC the constant buffers; the original's frame
                            // has 0x400 bytes for it)
    if (Com<long>(reflection, 3, desc) < 0) {
        StringRelease(&name);
        return 0;
    }
    for (uint32_t i = 0; i < desc[3]; i++) {
        if (Com<void*>(reflection, 4, i) == constantBuffer) {
            if (i == 0xFFFFFFFF) {
                break;
            }
            uint8_t ready = Map_PrepareBuffer(self, constantBuffer, i);
            StringRelease(&name);
            return ready;
        }
    }
    StringRelease(&name);
    return 0;
}

// 0x8BF3F0 (thiscall): an object entry's value written (1 when it can't be)
uint32_t __fastcall ConstantMap_SetObjectConstant(Map* self, re::Edx, void* mapped, ConstantMapEntry* entry,
                                                  void* geometry, void* skin, void*, void*, void* state,
                                                  void* effects, const void* world, const void* bound,
                                                  uint32_t pass) {
    alignas(16) float buffer[16];
    const void* data =
        Map_GetObjectConstant(self, entry, geometry, skin, state, effects, world, bound, pass, buffer);
    if (data == nullptr) {
        return 1;
    }
    Place p = Unpack(entry);
    if (!p.regValid || !p.countValid) {
        return 1;
    }
    uint32_t elements = ArrayElements(entry, p.registers);
    return WriteConstant(mapped, data, p.reg, p.component, p.registers, p.components, elements, p.flag, nullptr);
}

// 0x8BF520 (thiscall): InsertEntry (into the first empty slot; 0x30 when it can't be)
uint32_t __fastcall ConstantMap_InsertEntry(Map* self, re::Edx, ConstantMapEntry* entry) {
    re::Ptr32<ConstantMapEntry> held = entry;
    AddRefRef(entry);
    bool failed = EntryArray_AddFirstEmpty(EntryArray(self), &held) == 0xFFFFFFFF;
    ReleaseRef(entry);
    return failed ? 0x30 : 0;
}

// 0x8BF580 (thiscall): the constants written into the locked buffer by kind (the geometry's SCM
// extra data for the attributes); 1 when any failed (0 when there are none)
uint32_t __fastcall ConstantMap_SetShaderConstantsBody(Map* self, re::Edx, void* geometry, void* skin,
                                                       void* partition, void* bufferData, void* state,
                                                       void* effects, const void* world, const void* bound,
                                                       uint32_t pass, int global) {
    if (self->usedEntries == 0) {
        return 0;
    }
    if (self->buffer == nullptr) {
        return 1;
    }
    void* stream = Object_Get8(self->buffer);
    void* mapped = Virtual<void*>(stream, 2, 2u);   // Lock
    if (mapped == nullptr) {
        return 1;
    }
    uint8_t ok = 1;
    void* extraData = ObjectNET_GetExtraData(geometry, Map_GetSCMExtraDataName());
    for (uint32_t i = 0; i < self->entryCount; i++) {
        ConstantMapEntry* entry = self->entries[i];
        if (entry == nullptr || entry->reg == 0xFFFFFFFF) {
            continue;
        }
        uint32_t result;
        switch (entry->flags & 0xF0000000u) {
        case 0x20000000u:
            result = Map_SetPredefinedConstant(self, mapped, entry, geometry, skin, partition, bufferData, state,
                                               effects, world, bound, pass);
            break;
        case 0x10000000u:
        case 0x40000000u:
            result = Map_SetConstantConstant(self, mapped, entry, pass);
            break;
        case 0x30000000u:
            result = Map_SetAttributeConstant(self, mapped, entry, geometry, skin, partition, bufferData, state,
                                              effects, world, bound, pass, global, extraData);
            break;
        case 0x50000000u:
            result = Map_SetOperatorConstant(self, mapped, entry, geometry, skin, state, effects, world, bound, pass,
                                             global, extraData);
            break;
        case 0x60000000u:
            result = Map_SetObjectConstant(self, mapped, entry, geometry, skin, partition, bufferData, state,
                                           effects, world, bound, pass);
            break;
        default:
            result = 1;
            break;
        }
        if (result != 0) {
            ok = 0;
        }
    }
    Virtual<void>(stream, 3);   // Unlock
    return ok == 0 ? 1 : 0;
}

// 0x8BF780 (thiscall, slot 3): AddPredefinedEntry (key, extra, register, variable name)
uint32_t __fastcall ConstantMap_AddPredefinedEntry(Map* self, re::Edx, const char* key, uint32_t extra, uint32_t reg,
                                                   const char* name) {
    ConstantMapEntry* entry = NewEntry(key);
    entry->extra = extra;
    entry->reg = reg;
    SetString(&entry->variable, name);
    entry->flags = 0x20000000;
    uint32_t result = Map_SetupPredefinedEntry(entry);
    if (result == 0) {
        result = Map_InsertEntry(self, entry);
    } else {
        Virtual<void>(entry, 0, 1);
    }
    if (reg != 0xFFFFFFFF) {
        self->valid = 0;
    }
    return result;
}

// 0x8BF8B0 (thiscall, slot 4): AddAttributeEntry
uint32_t __fastcall ConstantMap_AddAttributeEntry(Map* self, re::Edx, const char* key, uint32_t flags, uint32_t extra,
                                                  uint32_t reg, uint32_t count, const char* name, uint32_t size,
                                                  uint32_t stride, const void* data, int copy) {
    ConstantMapEntry* entry = NewEntry(key);
    entry->flags = (flags & 0x0FFFFFFFu) | 0x30000000u;
    entry->extra = extra;
    entry->reg = reg;
    entry->count = count;
    SetString(&entry->variable, name);
    if (data != nullptr) {
        Entry_SetData(entry, size, stride, data, copy);
    }
    uint32_t result = InsertOrDelete(self, entry);
    if (reg != 0xFFFFFFFF) {
        self->valid = 0;
    }
    return result;
}

// 0x8BFA00 (thiscall, slot 5): AddConstantEntry (placed at a register: its attribute type by its
// size, packed from register 0 for a float or a size it doesn't know (the other sizes not
// packed, as the original has it); the count unused)
uint32_t __fastcall ConstantMap_AddConstantEntry(Map* self, re::Edx, const char* key, uint32_t flags, uint32_t extra,
                                                 uint32_t reg, uint32_t, const char* name, uint32_t size,
                                                 uint32_t stride, const void* data, int copy) {
    ConstantMapEntry* entry = NewEntry(key);
    entry->extra = extra;
    entry->reg = reg;
    SetString(&entry->variable, name);
    uint32_t f = (flags & 0x0FFFFFFFu) | 0x10000000u;
    if (reg != 0xFFFFFFFF) {
        f &= 0xFFFFFF00u;
        bool pack = true;
        switch (size) {
        case 4:
            f |= 4;
            break;
        case 8:
            f |= 5;
            pack = false;
            break;
        case 0xC:
            f |= 6;
            pack = false;
            break;
        case 0x10:
            f |= 7;
            pack = false;
            break;
        case 0x20:
            f |= 0xC;
            pack = false;
            break;
        case 0x24:
            f |= 8;
            pack = false;
            break;
        case 0x30:
            f |= 0xD;
            pack = false;
            break;
        case 0x40:
            f |= 9;
            pack = false;
            break;
        default:
            break;
        }
        if (pack) {
            uint32_t r = 0;
            uint32_t c = 0;
            uint32_t outReg;
            uint32_t outCount;
            PackNext(&r, &c, size >> 4, (size >> 2) & 3, &outReg, &outCount, 1);
            entry->reg = outReg;
            entry->count = outCount;
            entry->placed = 1;
        }
    }
    entry->flags = f;
    entry->size = size;
    entry->stride = stride;
    SetEntryData(entry, size, data, copy);
    uint32_t result = InsertOrDelete(self, entry);
    if (reg != 0xFFFFFFFF) {
        self->valid = 0;
    }
    return result;
}

// 0x8BFC20 (thiscall, slot 6): AddGlobalEntry (the global constant registered with the shader
// factory once it's in)
uint32_t __fastcall ConstantMap_AddGlobalEntry(Map* self, re::Edx, const char* key, uint32_t flags, uint32_t extra,
                                               uint32_t reg, uint32_t count, const char* name, uint32_t size,
                                               uint32_t stride, const void* data, int copy) {
    ConstantMapEntry* entry = NewEntry(key);
    entry->extra = extra;
    entry->flags = (flags & 0x0FFFFFFFu) | 0x40000000u;
    entry->reg = reg;
    entry->count = count;
    SetString(&entry->variable, name);
    entry->size = size;
    entry->stride = stride;
    SetEntryData(entry, size, data, copy);
    uint32_t result = Map_InsertEntry(self, entry);
    if (result == 0) {
        uint32_t type = ms_auiGlobalTypes[flags & 0xFF];
        re::Ptr32<char> handle = key != nullptr ? FixedString_Add(key) : NullString();
        ShaderFactory_RegisterGlobalShaderConstant(&handle, type, size, data);
        StringRelease(&handle);
    } else {
        Virtual<void>(entry, 0, 1);
    }
    if (reg != 0xFFFFFFFF) {
        self->valid = 0;
    }
    return result;
}

// 0x8BFE20 (thiscall, slot 7): AddOperatorEntry
uint32_t __fastcall ConstantMap_AddOperatorEntry(Map* self, re::Edx, const char* key, uint32_t flags, uint32_t extra,
                                                 uint32_t reg, uint32_t count, const char* name) {
    ConstantMapEntry* entry = NewEntry(key);
    entry->flags = (flags & 0x0FFFFFFFu) | 0x50000000u;
    entry->extra = extra;
    entry->reg = reg;
    entry->count = count;
    SetString(&entry->variable, name);
    uint32_t result = InsertOrDelete(self, entry);
    if (reg != 0xFFFFFFFF) {
        self->valid = 0;
    }
    return result;
}

// 0x8BFF50 (thiscall, slot 8): AddObjectEntry (key, register, variable name, the effect's index,
// the object type)
uint32_t __fastcall ConstantMap_AddObjectEntry(Map* self, re::Edx, const char* key, uint32_t reg, const char* name,
                                               uint32_t extra, uint32_t objectType) {
    ConstantMapEntry* entry = NewEntry(key);
    entry->reg = reg;
    SetString(&entry->variable, name);
    entry->extra = extra;
    entry->flags = ms_ausObjectFlags[objectType] | 0x60000000u;
    uint32_t result = Map_SetupObjectEntry(entry);
    if (result == 0) {
        result = Map_InsertEntry(self, entry);
    } else {
        Virtual<void>(entry, 0, 1);
    }
    if (reg != 0xFFFFFFFF) {
        self->valid = 0;
    }
    return result;
}

// 0x8C0090 (thiscall): SetShaderConstants (the buffer made ready first: 1 when it can't be)
uint32_t __fastcall ConstantMap_SetShaderConstants(Map* self, re::Edx, void* geometry, void* skin, void* partition,
                                                   void* bufferData, void* state, void* effects, const void* world,
                                                   const void* bound, uint32_t pass, int global) {
    if (!Map_PrepareBuffer(self, nullptr, 0xFFFFFFFF)) {
        return 1;
    }
    return Map_SetShaderConstantsBody(self, geometry, skin, partition, bufferData, state, effects, world, bound, pass,
                                      global);
}

// 0x8C0100 (thiscall): NiD3D10ShaderConstantBuffer's constructor (its data stream made: a constant
// buffer of the size and usage; none when that fails)
void* __fastcall ConstantBuffer_ConstructImpl(uint8_t* self, re::Edx, uint32_t size, uint32_t usage) {
    At<uint32_t>(self, 0) = kRefObjectVtable;
    At<uint32_t>(self, 4) = 0;
    Increment(&ms_uiObjects);
    re::Ptr32<void>& stream = At<void*>(self, 8);
    At<uint32_t>(self, 0) = kConstantBufferVtable;
    stream = nullptr;
    At<uint32_t>(self, 0xC) = 0xFFFFFFFF;
    if (!DataStream_Create(size, usage, 4, &stream)) {
        void* made = stream;
        if (made != nullptr) {
            ReleaseRef(made);
            stream = nullptr;
        }
    }
    return self;
}

// 0x8C0180 (cdecl): NiD3D10ShaderConstantBuffer::Create (into the reference; whether it has its
// data stream)
uint8_t __cdecl ConstantBuffer_CreateImpl(uint32_t size, uint32_t usage, re::Ptr32<void>* result) {
    void* memory = NiNew(0x10);
    void* made = memory != nullptr ? ConstantBuffer_Construct(memory, size, usage) : nullptr;
    void* old = *result;
    if (old != made) {
        ReleaseRef(old);
        *result = made;
        if (made == nullptr) {
            return 0;
        }
        Increment(&At<uint32_t>(made, 4));
    }
    void* buffer = *result;
    return buffer != nullptr && At<void*>(buffer, 8) != nullptr ? 1 : 0;
}

// 0x8C0200 (thiscall, slot 0): NiD3D10ShaderConstantBuffer's deleting destructor
void* __fastcall ConstantBuffer_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kConstantBufferVtable;
    ReleaseRef(At<void*>(self, 8));
    At<uint32_t>(self, 0) = kRefObjectVtable;
    Decrement(&ms_uiObjects);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x10);
    }
    return self;
}

// 0x8C0790 (cdecl): the name of the geometry's extra data the attribute constants read (the SCM
// extra data's)
re::Ptr32<char>* __cdecl ConstantMap_GetSCMExtraDataName() {
    return &ms_kSCMExtraDataName;
}

#define GB44M_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

GB44M_REVERSED("NiD3D10ShaderConstantMap::MakeTextureTransform", 0x008BB370, ConstantMap_MakeTextureTransform);
GB44M_REVERSED("NiD3D10ShaderConstantMap::GetEntryIndex", 0x008BB410, ConstantMap_GetEntryIndex);
GB44M_REVERSED("NiD3D10ShaderConstantMap::SetupPredefinedEntry", 0x008BB460, ConstantMap_SetupPredefinedEntry);
GB44M_REVERSED("NiD3D10ShaderConstantMap::SetupObjectEntry", 0x008BB590, ConstantMap_SetupObjectEntry);
GB44M_REVERSED("NiD3D10ShaderConstantMap::RemoveEntry", 0x008BBF60, ConstantMap_RemoveEntry);
GB44M_REVERSED("NiD3D10ShaderConstantMap::AssignRegisters", 0x008BC000, ConstantMap_AssignRegisters);
GB44M_REVERSED("NiD3D10ShaderConstantMap::SetConstantConstant", 0x008BC5C0, ConstantMap_SetConstantConstant);
GB44M_REVERSED("NiD3D10ShaderConstantMap::~NiD3D10ShaderConstantMap", 0x008BEBC0, ConstantMap_Destruct);
GB44M_REVERSED("NiD3D10ShaderConstantMap::PrepareBuffer", 0x008BED10, ConstantMap_PrepareBuffer);
GB44M_REVERSED("NiD3D10ShaderConstantMap::SetPredefinedConstant", 0x008BEDA0, ConstantMap_SetPredefinedConstant);
GB44M_REVERSED("NiD3D10ShaderConstantMap::SetAttributeConstant", 0x008BEF30, ConstantMap_SetAttributeConstant);
GB44M_REVERSED("NiD3D10ShaderConstantMap::SetOperatorConstant", 0x008BF050, ConstantMap_SetOperatorConstant);
GB44M_REVERSED("NiD3D10ShaderConstantMap::NiD3D10ShaderConstantMap", 0x008BF210, ConstantMap_Construct);
GB44M_REVERSED("NiD3D10ShaderConstantMap::DeletingDestructor", 0x008BF260, ConstantMap_DeletingDestructor);
GB44M_REVERSED("NiD3D10ShaderConstantMap::SetShaderReflection", 0x008BF280, ConstantMap_SetShaderReflection);
GB44M_REVERSED("NiD3D10ShaderConstantMap::SetObjectConstant", 0x008BF3F0, ConstantMap_SetObjectConstant);
GB44M_REVERSED("NiD3D10ShaderConstantMap::InsertEntry", 0x008BF520, ConstantMap_InsertEntry);
GB44M_REVERSED("NiD3D10ShaderConstantMap::SetShaderConstantsBody", 0x008BF580, ConstantMap_SetShaderConstantsBody);
GB44M_REVERSED("NiD3D10ShaderConstantMap::AddPredefinedEntry", 0x008BF780, ConstantMap_AddPredefinedEntry);
GB44M_REVERSED("NiD3D10ShaderConstantMap::AddAttributeEntry", 0x008BF8B0, ConstantMap_AddAttributeEntry);
GB44M_REVERSED("NiD3D10ShaderConstantMap::AddConstantEntry", 0x008BFA00, ConstantMap_AddConstantEntry);
GB44M_REVERSED("NiD3D10ShaderConstantMap::AddGlobalEntry", 0x008BFC20, ConstantMap_AddGlobalEntry);
GB44M_REVERSED("NiD3D10ShaderConstantMap::AddOperatorEntry", 0x008BFE20, ConstantMap_AddOperatorEntry);
GB44M_REVERSED("NiD3D10ShaderConstantMap::AddObjectEntry", 0x008BFF50, ConstantMap_AddObjectEntry);
GB44M_REVERSED("NiD3D10ShaderConstantMap::SetShaderConstants", 0x008C0090, ConstantMap_SetShaderConstants);
GB44M_REVERSED("NiD3D10ShaderConstantBuffer::NiD3D10ShaderConstantBuffer", 0x008C0100, ConstantBuffer_ConstructImpl);
GB44M_REVERSED("NiD3D10ShaderConstantBuffer::Create", 0x008C0180, ConstantBuffer_CreateImpl);
GB44M_REVERSED("NiD3D10ShaderConstantBuffer::DeletingDestructor", 0x008C0200, ConstantBuffer_DeletingDestructor);
GB44M_REVERSED("NiD3D10ShaderConstantMap::GetSCMExtraDataName", 0x008C0790, ConstantMap_GetSCMExtraDataName);
} // namespace ni::c44
