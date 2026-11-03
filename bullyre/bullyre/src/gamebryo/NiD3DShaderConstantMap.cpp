// NiD3DShaderConstantMap (vtable 0x99C348): the DX9 renderer's shader constant maps: their entries
// (NiShaderConstantMapEntry, vtable 0x99C2E4, 0x34 bytes) added by kind, found by key, and the
// constants set each draw by kind. The map: +0x1C its entries (NiTObjectArray: +0x20 the array,
// +0x26 its size), +0x30 the shader being set, +0x34 the last error, +0x38 the device (held),
// +0x3C the renderer, +0x40 its render state.
//
// An entry: +0x8 the key (NiFixedString), +0xC the internal id, +0x10 the flags (the kind in the top
// nibble: 1 constant, 2 predefined, 3 attribute, 4 global, 5 operator, 6 object; the low byte the
// attribute type), +0x14 the extra, +0x18 the shader register, +0x1C the register count, +0x20 the
// variable name, +0x24 the data size, +0x28 the stride, +0x2C the data, +0x30 whether it owns it.
//
// The map's slots here: 2 AddEntry, 3 AddPredefinedEntry, 4 AddAttributeEntry, 5 AddConstantEntry,
// 6 AddGlobalEntry, 7 AddOperatorEntry, 8 AddObjectEntry, 9 RemoveEntry, 10 GetEntry, 11
// GetEntryAtIndex, 12 SetShaderConstants, 13 GetEntryIndex, 14 InsertEntry, 15
// SetupPredefinedEntry, 16..22 SetDefined / Constant / Attribute / Global / Operator / (21)
// SetupObjectEntry / Object constants.

#include <cstddef>
#include <cstdint>
#include <initializer_list>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::c39 {
using namespace ni::dx9;
namespace {

constexpr uintptr_t kEntryVtable = 0x0099C2E4;
constexpr uintptr_t kNiRefObjectVtable = 0x008FEFD4;
GAME_VAR(uint32_t, NiRefObject_ms_uiObjects, 0x020E2168);
GAME_VAR(uint32_t, ms_uiEntryShift, 0x009A1914);
GAME_VAR(NiFixedString, ms_kSCMDataName, 0x020E6E54);
GAME_VAR(uint8_t, ms_bReportMissingEffects, 0x020E3238);
GAME_ARRAY(const uint32_t, ms_auiObjectTypes, 0x020E4040);       // by the flags' second byte
GAME_ARRAY(const uint16_t, ms_ausObjectFlags, 0x020E4020);       // by object type
GAME_ARRAY(const uint8_t, ms_aucAttributeTypes, 0x020E407C);
GAME_ARRAY(const uint32_t, ms_auiGlobalTypes, 0x020E408C);
GAME_ARRAY(const uint32_t, ms_auiObjectMappings, 0x020E330C);    // three per mapping: count, type, ..
constexpr uintptr_t kBoneMatrixDefault3 = 0x00BCB8EC;            // (predefined 0x50 / 0x51's data)
constexpr uintptr_t kBoneMatrixDefault4 = 0x00BCB8FC;

GAME_FN(int, __cdecl, NiStricmp, 0x00753820, const char* a, const char* b);
GAME_FN(void*, __thiscall, ObjectNET_GetExtraData, 0x00764250, void* self, const re::Ptr32<const char>* name);
GAME_FN(re::Ptr32<char>*, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<const char>* self, const re::Ptr32<const char>* other);
GAME_FN(void, __thiscall, FixedString_Destruct, 0x00517750, re::Ptr32<const char>* self);
GAME_FN(uint8_t, __cdecl, LookUpPredefinedMapping, 0x0077F210, const char* name, uint32_t* id, uint32_t* count);
GAME_FN(uint32_t, __cdecl, GetPredefinedMappingType, 0x0077EF10, uint32_t id, uint32_t count);
GAME_FN(uint8_t, __cdecl, LookUpObjectMapping, 0x0077F330, const char* name, uint32_t* id);
GAME_FN(uint8_t, __cdecl, ShaderFactory_RegisterGlobalShaderConstant, 0x00756A60, const re::Ptr32<const char>* key,
        uint32_t type, uint32_t size, const void* data);
GAME_FN(uint8_t, __cdecl, ShaderFactory_ReleaseGlobalShaderConstant, 0x00756B40, const re::Ptr32<const char>* key);
GAME_FN(void, __thiscall, EntryArray_ClearAt, 0x0040E800, void* array, uint32_t index, const re::Ptr32<void>* entry);

// this file's (called where modes apply)
GAME_FN(void*, __thiscall, Entry_Construct, 0x0088B520, void* self);
GAME_FN(void, __thiscall, Entry_SetData, 0x0088A500, void* self, uint32_t size, uint32_t stride, const void* data,
        int copy);
GAME_FN(uint8_t, __stdcall, Entry_GetDimensions, 0x0088AAC0, void* entry, uint32_t* registers, uint32_t* columns);

// a key / variable name set from text (NiFixedString's assignment through a temporary)
inline void SetFixedString(uint8_t* entry, int offset, const char* text) {
    NiFixedString temp = text != nullptr ? ni::sys::GlobalStringTable_AddString(text) : static_cast<char*>(ni::sys::kNullFixedString);
    FixedString_Assign(&At<NiFixedString>(entry, offset), &temp);
    ni::sys::FixedStringRelease(re::PtrCast<re::Ptr32<char>*>(&temp));
}

inline uint8_t* NewEntry(const char* key) {
    void* memory = NiNew(0x34);
    uint8_t* entry = memory != nullptr ? static_cast<uint8_t*>(Entry_Construct(memory)) : nullptr;
    SetFixedString(entry, 8, key);
    return entry;
}

// the entry inserted (slot 14), deleted again when it can't be; the error kept
inline uint32_t Insert(uint8_t* self, uint8_t* entry) {
    At<uint32_t>(self, 0x34) = Virtual<uint32_t>(self, 14, entry);
    if (At<uint32_t>(self, 0x34) != 0) {
        Virtual<void>(entry, 0, 1);
    }
    return At<uint32_t>(self, 0x34);
}

} // namespace

// 0x88A480 (thiscall): NiPoint3 scaled in place (the game's code shares it)
void __fastcall Point3_Scale(float* self, re::Edx, float scale) {
    self[0] = self[0] * scale;
    self[1] = self[1] * scale;
    self[2] = scale * self[2];
}

// 0x88A4B0 (thiscall): NiSCMExtraData::GetNextEntry (the next vertex / pixel entry's extra data
// when it's the one for this map index and pass)
void* __fastcall SCMExtraData_GetNextEntry(uint8_t* self, re::Edx, uint32_t index, uint32_t pass, uint32_t pixel,
                                           uint32_t) {
    uint32_t key = (pass << (ms_uiEntryShift & 0x1F)) | index;
    uint8_t* entries = At<uint8_t*>(self, 0x24 + int(pixel) * 4);
    int32_t& cursor = At<int32_t>(self, 0x18 + int(pixel) * 4);
    if (At<uint32_t>(entries, cursor * 8) == key) {
        void* extra = At<void*>(entries, cursor * 8 + 4);
        cursor++;
        return extra;
    }
    return nullptr;
}

// 0x88A500 (thiscall): NiShaderConstantMapEntry::SetData (copied into a NiMalloc'd block when asked)
void __fastcall Entry_SetDataImpl(uint8_t* self, re::Edx, uint32_t size, uint32_t stride, const void* data, int copy) {
    At<uint32_t>(self, 0x24) = size;
    At<uint32_t>(self, 0x28) = stride;
    if (uint8_t(copy) != 0) {
        At<uint8_t>(self, 0x30) = 1;
        NiFree(At<void*>(self, 0x2C));
        void* block = NiMalloc(size);
        At<void*>(self, 0x2C) = block;
        crt::MemcpyS(block, size, data, size);
        return;
    }
    At<uint8_t>(self, 0x30) = 0;
    At<const void*>(self, 0x2C) = data;
}

// 0x88A560 (thiscall): an entry's callback (+0x44) called with the entry third
uint32_t __fastcall Entry_CallCallback(uint8_t* self, re::Edx, void* a1, void* a2, void* a3, void* a4, void* a5,
                                       void* a6, void* a7, void* a8, void* a9, void* a10) {
    using Callback = uint32_t(__cdecl*)(void*, void*, void*, void*, void*, void*, void*, void*, void*, void*, void*);
    return At<Callback>(self, 0x44)(a1, a2, self, a3, a4, a5, a6, a7, a8, a9, a10);
}

// 0x88A5A0 (thiscall): NiD3DShaderConstantMap::SetD3DRenderer
void __fastcall ConstantMap_SetD3DRenderer(uint8_t* self, re::Edx, uint8_t* renderer) {
    At<uint8_t*>(self, 0x3C) = renderer;
    if (renderer != nullptr) {
        void* device = At<void*>(renderer, kDevice);
        if (At<void*>(self, 0x38) != nullptr) {
            Com<long>(At<void*>(self, 0x38), kRelease);
        }
        At<void*>(self, 0x38) = device;
        if (device != nullptr) {
            Com<long>(device, kAddRef);
        }
        At<void*>(self, 0x40) = At<void*>(At<uint8_t*>(self, 0x3C), kRenderState);
        return;
    }
    if (At<void*>(self, 0x38) != nullptr) {
        Com<long>(At<void*>(self, 0x38), kRelease);
    }
    At<void*>(self, 0x38) = nullptr;
    At<void*>(self, 0x40) = nullptr;
}

// 0x88A610 (thiscall, slot 2): NiD3DShaderConstantMap::AddEntry (a duplicate key with the same
// extra and flags refused; else by the kind in the flags)
uint32_t __fastcall ConstantMap_AddEntry(uint8_t* self, re::Edx, const char* key, uint32_t flags, uint32_t extra,
                                         uint32_t reg, uint32_t count, const char* name, uint32_t size,
                                         uint32_t stride, const void* data, int copy) {
    At<uint32_t>(self, 0x34) = 0;
    uint8_t* existing = Virtual<uint8_t*>(self, 10, key);
    if (existing != nullptr && At<uint32_t>(existing, 0x14) == extra && At<uint32_t>(existing, 0x10) == flags) {
        At<uint32_t>(self, 0x34) = 0x20;
        return 0x20;
    }
    uint32_t result;
    switch (flags & 0xF0000000u) {
    case 0x30000000:
        result = Virtual<uint32_t>(self, 4, key, flags, extra, reg, count, name, size, stride, data, copy);
        break;
    case 0x10000000:
        result = Virtual<uint32_t>(self, 5, key, flags, extra, reg, count, name, size, stride, data, copy);
        break;
    case 0x20000000:
        result = Virtual<uint32_t>(self, 3, key, extra, reg, name);
        break;
    case 0x40000000:
        result = Virtual<uint32_t>(self, 6, key, flags, extra, reg, count, name, size, stride, data, copy);
        break;
    case 0x50000000:
        result = Virtual<uint32_t>(self, 7, key, flags, extra, reg, count, name);
        break;
    case 0x60000000:
        result = Virtual<uint32_t>(self, 8, key, reg, name, extra, ms_auiObjectTypes[(flags >> 8) & 0xFF]);
        break;
    default:
        At<uint32_t>(self, 0x34) = 0x40;
        return At<uint32_t>(self, 0x34);
    }
    At<uint32_t>(self, 0x34) = result;
    return result;
}

// 0x88A7A0 (thiscall, slot 10): NiD3DShaderConstantMap::GetEntry (by key, case blind)
uint8_t* __fastcall ConstantMap_GetEntry(uint8_t* self, re::Edx, const char* key) {
    uint32_t count = At<uint16_t>(self, 0x26);
    for (uint32_t i = 0; i < count; i++) {
        uint8_t* entry = At<re::Ptr32<uint8_t>*>(self, 0x20)[i];
        if (entry != nullptr && NiStricmp(key, At<const char*>(entry, 8)) == 0) {
            return entry;
        }
    }
    return nullptr;
}

// 0x88A7F0 (thiscall, slot 11): NiD3DShaderConstantMap::GetEntryAtIndex
uint8_t* __fastcall ConstantMap_GetEntryAtIndex(uint8_t* self, re::Edx, uint32_t index) {
    return At<re::Ptr32<uint8_t>*>(self, 0x20)[index];
}

// 0x88A800 (thiscall, slot 12): NiD3DShaderConstantMap::SetShaderConstants (each set entry by its
// kind; failures reported (to nowhere) and remembered: true when any failed)
uint8_t __fastcall ConstantMap_SetShaderConstants(uint8_t* self, re::Edx, void* shader, uint8_t* geometry, void* skin,
                                                  void* partition, void* bufferData, void* properties,
                                                  void* effects, const void* world, const void* bound,
                                                  uint32_t pass, int global) {
    At<void*>(self, 0x30) = shader;
    uint8_t ok = 1;
    void* scm = ObjectNET_GetExtraData(geometry, &ms_kSCMDataName);
    for (uint32_t i = 0; i < At<uint16_t>(self, 0x26); i++) {
        uint8_t* entry = At<re::Ptr32<uint8_t>*>(self, 0x20)[i];
        if (entry == nullptr || (At<int32_t>(entry, 0x18) == -1 && At<uint32_t>(entry, 0x20) == 0)) {
            continue;
        }
        uint32_t result;
        switch (At<uint32_t>(entry, 0x10) & 0xF0000000u) {
        case 0x20000000:
            result = Virtual<uint32_t>(self, 16, shader, entry, geometry, skin, partition, bufferData, properties,
                                       effects, world, bound, pass);
            break;
        case 0x10000000:
            result = Virtual<uint32_t>(self, 17, shader, entry, pass);
            break;
        case 0x30000000:
            result = Virtual<uint32_t>(self, 18, shader, entry, geometry, skin, partition, bufferData, properties,
                                       effects, world, bound, pass, global, scm);
            break;
        case 0x40000000:
            result = Virtual<uint32_t>(self, 19, shader, entry, pass);
            break;
        case 0x50000000:
            result = Virtual<uint32_t>(self, 20, shader, entry, geometry, skin, properties, effects, world, bound,
                                       pass);
            break;
        case 0x60000000:
            result = Virtual<uint32_t>(self, 22, shader, entry, geometry, skin, partition, bufferData, properties,
                                       effects, world, bound, pass);
            break;
        default:
            result = 0x40;
            break;
        }
        if (result == 0) {
            continue;
        }
        const char* entryKey = At<const char*>(entry, 8);
        switch (result) {
        case 0x10:
            NiOutputDebugString("NiD3DShaderConstantMap::SetShaderConstants - Constant %s has unknown entry 0x%8x\n",
                                entryKey, At<uint32_t>(entry, 0xC));
            break;
        case 0x40:
            NiOutputDebugString("NiD3DShaderConstantMap::SetShaderConstants - Constant %s has invalid mapping "
                                "0x%8x\n",
                                entryKey, At<uint32_t>(entry, 0x10) & 0xF0000000u);
            break;
        case 0x50:
            NiOutputDebugString("NiD3DShaderConstantMap::SetShaderConstants - Constant %s failed to be set\n",
                                entryKey);
            break;
        case 0x60:
            if (ms_bReportMissingEffects != 0) {
                NiOutputDebugString("NiD3DShaderConstantMap::SetShaderConstants - Constant %s references a "
                                    "nonexistent NiDynamicEffect object. Default values used.\n",
                                    entryKey);
            }
            break;
        default:
            NiOutputDebugString("NiD3DShaderConstantMap::SetShaderConstants - Constant %s failed to be set with "
                                "unknown error\n",
                                entryKey);
            break;
        }
        ok = 0;
    }
    return ok == 0 ? 1 : 0;
}

// 0x88AAC0 (stdcall): an entry's register count and columns by its stride (false for an unknown one)
uint8_t __stdcall Entry_GetDimensionsImpl(uint8_t* entry, uint32_t* registers, uint32_t* columns) {
    uint32_t r = 1, c = 1;
    uint8_t known = 1;
    switch (At<uint32_t>(entry, 0x28)) {
    case 4: c = 1; r = 1; break;
    case 8: c = 2; r = 1; break;
    case 0xC: c = 3; r = 1; break;
    case 0x10: c = 4; r = 1; break;
    case 0x20: c = 4; r = 2; break;
    case 0x24: c = 3; r = 3; break;
    case 0x30: c = 4; r = 3; break;
    case 0x40: c = 4; r = 4; break;
    default: c = 1; r = 1; known = 0; break;
    }
    *columns = c;
    *registers = r;
    return known;
}

// 0x88AC30 (thiscall, slot 13): NiD3DShaderConstantMap::GetEntryIndex (-1 when absent)
uint32_t __fastcall ConstantMap_GetEntryIndex(uint8_t* self, re::Edx, const char* key) {
    uint32_t count = At<uint16_t>(self, 0x26);
    for (uint32_t i = 0; i < count; i++) {
        uint8_t* entry = At<re::Ptr32<uint8_t>*>(self, 0x20)[i];
        if (entry != nullptr && NiStricmp(key, At<const char*>(entry, 8)) == 0) {
            return i;
        }
    }
    return 0xFFFFFFFF;
}

// 0x88AC80 (thiscall, slot 15): NiD3DShaderConstantMap::SetupPredefinedEntry (the mapping by name,
// its attribute type; register counts for the matrices by the extra's count; the bone matrix
// defaults for 0x50 / 0x51)
uint32_t __fastcall ConstantMap_SetupPredefinedEntry(uint8_t*, re::Edx, uint8_t* entry) {
    uint32_t id;
    uint32_t count = 4;
    if (!LookUpPredefinedMapping(At<const char*>(entry, 8), &id, &count)) {
        return 0x40;
    }
    uint32_t type = GetPredefinedMappingType(id, count);
    At<uint32_t>(entry, 0x10) = (At<uint32_t>(entry, 0x10) & 0xFFFFFF00u) | ms_aucAttributeTypes[type];
    if (id == 0) {
        return 0x40;
    }
    if (id < 0x45) {
        if (id == 0x21 || id == 0x23) {
            At<uint32_t>(entry, 0x1C) = At<uint16_t>(entry, 0x16) * 3;
        } else if (id == 0x22 || id == 0x24) {
            At<uint32_t>(entry, 0x1C) = At<uint16_t>(entry, 0x16) * 4;
        } else if ((id >= 0x19 && id <= 0x20) || id == 0x60 || id == 0x61 || id == 0x62 || id == 0x63) {
            At<uint32_t>(entry, 0x1C) = 4;
        } else {
            At<uint32_t>(entry, 0x1C) = count;
        }
    }
    At<uint32_t>(entry, 0xC) = id;
    if (id == 0x50) {
        At<uint32_t>(entry, 0x2C) = kBoneMatrixDefault3;
    } else if (id == 0x51) {
        At<uint32_t>(entry, 0x2C) = kBoneMatrixDefault4;
    } else {
        return 0;
    }
    At<uint8_t>(entry, 0x30) = 0;
    At<uint32_t>(entry, 0x28) = 0x10;
    At<uint32_t>(entry, 0x24) = 0x10;
    return 0;
}

// 0x88AD90 (thiscall, slot 21): NiD3DShaderConstantMap::SetupObjectEntry (the object mapping by
// name: its register count and attribute type)
uint32_t __fastcall ConstantMap_SetupObjectEntry(uint8_t*, re::Edx, uint8_t* entry) {
    uint32_t id;
    if (!LookUpObjectMapping(At<const char*>(entry, 8), &id)) {
        return 0x40;
    }
    uint32_t index = int32_t(id) < 0x1E ? id : 0x1E;
    uint32_t count = ms_auiObjectMappings[index * 3];
    At<uint32_t>(entry, 0x10) =
        (At<uint32_t>(entry, 0x10) & 0xFFFFFF00u) | ms_aucAttributeTypes[ms_auiObjectMappings[index * 3 + 1]];
    if (id == 0) {
        return 0x40;
    }
    At<uint32_t>(entry, 0xC) = id;
    At<uint32_t>(entry, 0x1C) = count;
    return 0;
}

// 0x88B520 (thiscall): NiShaderConstantMapEntry::NiShaderConstantMapEntry
uint8_t* __fastcall Entry_ConstructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kNiRefObjectVtable;
    At<uint32_t>(self, 4) = 0;
    Increment(&NiRefObject_ms_uiObjects);
    At<uint32_t>(self, 0) = kEntryVtable;
    At<uint32_t>(self, 8) = 0;
    for (int offset = 0x10; offset <= 0x2C; offset += 4) {
        At<uint32_t>(self, offset) = 0;
    }
    At<uint8_t>(self, 0x30) = 0;
    At<uint8_t>(self, 0x31) = 0;
    At<uint8_t>(self, 0x32) = 0;
    return self;
}

// 0x88B580 (thiscall): NiShaderConstantMapEntry::~NiShaderConstantMapEntry (its data when owned,
// its strings let go)
void __fastcall Entry_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kEntryVtable;
    if (At<uint8_t>(self, 0x30) != 0) {
        NiFree(At<void*>(self, 0x2C));
    }
    for (int offset : {0x20, 0x8}) {
        char* s = At<char*>(self, offset);
        if (s != nullptr) {
            uint32_t hash = *reinterpret_cast<uint16_t*>(s - 2);
            if (Decrement(s - 8) == 1) {
                ni::sys::GlobalStringTable_RemoveString(ni::sys::kGlobalStringTable, &At<char*>(self, offset), hash);
            }
        }
    }
    At<uint32_t>(self, 0) = kNiRefObjectVtable;
    Decrement(&NiRefObject_ms_uiObjects);
}

// 0x88B610 (thiscall, slot 3): NiD3DShaderConstantMap::AddPredefinedEntry (set up from its mapping
// before it's inserted)
uint32_t __fastcall ConstantMap_AddPredefinedEntry(uint8_t* self, re::Edx, const char* key, uint32_t extra,
                                                   uint32_t reg, const char* name) {
    uint8_t* entry = NewEntry(key);
    At<uint32_t>(entry, 0x14) = extra;
    At<uint32_t>(entry, 0x18) = reg;
    SetFixedString(entry, 0x20, name);
    At<uint32_t>(entry, 0x10) = 0x20000000;
    At<uint32_t>(self, 0x34) = Virtual<uint32_t>(self, 15, entry);
    if (At<uint32_t>(self, 0x34) == 0) {
        At<uint32_t>(self, 0x34) = Virtual<uint32_t>(self, 14, entry);
        return At<uint32_t>(self, 0x34);
    }
    Virtual<void>(entry, 0, 1);
    return At<uint32_t>(self, 0x34);
}

// 0x88B740 (thiscall, slot 4): NiD3DShaderConstantMap::AddAttributeEntry
uint32_t __fastcall ConstantMap_AddAttributeEntry(uint8_t* self, re::Edx, const char* key, uint32_t flags,
                                                  uint32_t extra, uint32_t reg, uint32_t count, const char* name,
                                                  uint32_t size, uint32_t stride, const void* data, int copy) {
    uint8_t* entry = NewEntry(key);
    At<uint32_t>(entry, 0x10) = (flags & 0x0FFFFFFF) | 0x30000000;
    At<uint32_t>(entry, 0x1C) = count;
    At<uint32_t>(entry, 0x14) = extra;
    At<uint32_t>(entry, 0x18) = reg;
    SetFixedString(entry, 0x20, name);
    if (data != nullptr) {
        Entry_SetData(entry, size, stride, data, copy);
    }
    return Insert(self, entry);
}

// the data set as SetData does, inlined (even without data)
static void InlineSetData(uint8_t* entry, uint32_t size, uint32_t stride, const void* data, int copy) {
    At<uint32_t>(entry, 0x24) = size;
    At<uint32_t>(entry, 0x28) = stride;
    if (uint8_t(copy) != 0) {
        At<uint8_t>(entry, 0x30) = 1;
        NiFree(At<void*>(entry, 0x2C));
        void* block = NiMalloc(size);
        At<void*>(entry, 0x2C) = block;
        crt::MemcpyS(block, size, data, size);
    } else {
        At<uint8_t>(entry, 0x30) = 0;
        At<const void*>(entry, 0x2C) = data;
    }
}

// 0x88B890 (thiscall, slot 5): NiD3DShaderConstantMap::AddConstantEntry (the attribute type from the
// data size)
uint32_t __fastcall ConstantMap_AddConstantEntry(uint8_t* self, re::Edx, const char* key, uint32_t flags,
                                                 uint32_t extra, uint32_t reg, uint32_t count, const char* name,
                                                 uint32_t size, uint32_t stride, const void* data, int copy) {
    uint8_t* entry = NewEntry(key);
    uint32_t f = (flags & 0x0FFFFFFF) | 0x10000000;
    switch (size) {
    case 8: f |= 5; break;
    case 0xC: f |= 6; break;
    case 0x10: f |= 7; break;
    case 0x24: f |= 8; break;
    case 0x20: case 0x30: case 0x40: f |= 9; break;
    default: break;
    }
    At<uint32_t>(entry, 0x10) = f;
    At<uint32_t>(entry, 0x14) = extra;
    At<uint32_t>(entry, 0x18) = reg;
    At<uint32_t>(entry, 0x1C) = count;
    SetFixedString(entry, 0x20, name);
    InlineSetData(entry, size, stride, data, copy);
    return Insert(self, entry);
}

// 0x88BA40 (thiscall, slot 6): NiD3DShaderConstantMap::AddGlobalEntry (also registered with the
// shader factory as a global constant once inserted)
uint32_t __fastcall ConstantMap_AddGlobalEntry(uint8_t* self, re::Edx, const char* key, uint32_t flags,
                                               uint32_t extra, uint32_t reg, uint32_t count, const char* name,
                                               uint32_t size, uint32_t stride, const void* data, int copy) {
    uint8_t* entry = NewEntry(key);
    uint32_t f = (flags & 0x0FFFFFFF) | 0x40000000;
    At<uint32_t>(entry, 0x14) = extra;
    At<uint32_t>(entry, 0x10) = f;
    At<uint32_t>(entry, 0x18) = reg;
    At<uint32_t>(entry, 0x1C) = count;
    SetFixedString(entry, 0x20, name);
    InlineSetData(entry, size, stride, data, copy);
    At<uint32_t>(self, 0x34) = Virtual<uint32_t>(self, 14, entry);
    if (At<uint32_t>(self, 0x34) != 0) {
        Virtual<void>(entry, 0, 1);
        return At<uint32_t>(self, 0x34);
    }
    uint32_t type = ms_auiGlobalTypes[f & 0xFF];
    NiFixedString globalKey = key != nullptr ? ni::sys::GlobalStringTable_AddString(key) : static_cast<char*>(ni::sys::kNullFixedString);
    ShaderFactory_RegisterGlobalShaderConstant(&globalKey, type, size, data);
    ni::sys::FixedStringRelease(re::PtrCast<re::Ptr32<char>*>(&globalKey));
    return At<uint32_t>(self, 0x34);
}

// 0x88BC50 (thiscall, slot 7): NiD3DShaderConstantMap::AddOperatorEntry
uint32_t __fastcall ConstantMap_AddOperatorEntry(uint8_t* self, re::Edx, const char* key, uint32_t flags,
                                                 uint32_t extra, uint32_t reg, uint32_t count, const char* name) {
    uint8_t* entry = NewEntry(key);
    At<uint32_t>(entry, 0x10) = (flags & 0x0FFFFFFF) | 0x50000000;
    At<uint32_t>(entry, 0x1C) = count;
    At<uint32_t>(entry, 0x14) = extra;
    At<uint32_t>(entry, 0x18) = reg;
    SetFixedString(entry, 0x20, name);
    return Insert(self, entry);
}

// 0x88BD80 (thiscall, slot 8): NiD3DShaderConstantMap::AddObjectEntry (set up from its mapping)
uint32_t __fastcall ConstantMap_AddObjectEntry(uint8_t* self, re::Edx, const char* key, uint32_t reg, const char* name,
                                               uint32_t extra, uint32_t objectType) {
    uint8_t* entry = NewEntry(key);
    At<uint32_t>(entry, 0x18) = reg;
    SetFixedString(entry, 0x20, name);
    At<uint32_t>(entry, 0x14) = extra;
    At<uint32_t>(entry, 0x10) = uint32_t(ms_ausObjectFlags[objectType]) | 0x60000000;
    At<uint32_t>(self, 0x34) = Virtual<uint32_t>(self, 21, entry);
    if (At<uint32_t>(self, 0x34) == 0) {
        At<uint32_t>(self, 0x34) = Virtual<uint32_t>(self, 14, entry);
        return At<uint32_t>(self, 0x34);
    }
    Virtual<void>(entry, 0, 1);
    return At<uint32_t>(self, 0x34);
}

// 0x88BEB0 (thiscall, slot 9): NiD3DShaderConstantMap::RemoveEntry (a global one released from the
// shader factory; the slot cleared)
uint32_t __fastcall ConstantMap_RemoveEntry(uint8_t* self, re::Edx, const char* key) {
    uint32_t index = Virtual<uint32_t>(self, 13, key);
    if (index == 0xFFFFFFFF) {
        At<uint32_t>(self, 0x34) = 0x10;
        return At<uint32_t>(self, 0x34);
    }
    uint8_t* entry = At<re::Ptr32<uint8_t>*>(self, 0x20)[index];
    if (entry != nullptr && (At<uint32_t>(entry, 0x10) & 0xF0000000u) == 0x40000000u) {
        NiFixedString globalKey = ni::sys::FixedStringAddRef(At<char*>(entry, 8));
        ShaderFactory_ReleaseGlobalShaderConstant(&globalKey);
        FixedString_Destruct(&globalKey);
    }
    re::Ptr32<void> none = nullptr;
    EntryArray_ClearAt(self + 0x1C, index, &none);
    return At<uint32_t>(self, 0x34);
}

// 0x88BF50 (stdcall): an entry's data sent through the constant manager (slot 15: entry, data,
// count, registers): 1 without data, 0x50 when it fails
uint32_t __fastcall ConstantMap_SetEntryData(void*, re::Edx, void* manager, uint8_t* entry, const void* data) {
    uint32_t count = At<uint32_t>(entry, 0x24) / At<uint32_t>(entry, 0x28);
    uint32_t registers, columns;
    Entry_GetDimensions(entry, &registers, &columns);
    if (data == nullptr) {
        return 1;
    }
    return Virtual<uint8_t>(manager, 15, entry, data, count, registers, 0u) ? 0 : 0x50;
}

#define C39O_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C39O_REVERSED("NiPoint3::Scale_88A480", 0x0088A480, Point3_Scale);
C39O_REVERSED("NiSCMExtraData::GetNextEntry", 0x0088A4B0, SCMExtraData_GetNextEntry);
C39O_REVERSED("NiShaderConstantMapEntry::SetData", 0x0088A500, Entry_SetDataImpl);
C39O_REVERSED("NiShaderConstantMapEntry::CallCallback", 0x0088A560, Entry_CallCallback);
C39O_REVERSED("NiD3DShaderConstantMap::SetD3DRenderer", 0x0088A5A0, ConstantMap_SetD3DRenderer);
C39O_REVERSED("NiD3DShaderConstantMap::AddEntry", 0x0088A610, ConstantMap_AddEntry);
C39O_REVERSED("NiD3DShaderConstantMap::GetEntry", 0x0088A7A0, ConstantMap_GetEntry);
C39O_REVERSED("NiD3DShaderConstantMap::GetEntryAtIndex", 0x0088A7F0, ConstantMap_GetEntryAtIndex);
C39O_REVERSED("NiD3DShaderConstantMap::SetShaderConstants", 0x0088A800, ConstantMap_SetShaderConstants);
C39O_REVERSED("NiShaderConstantMapEntry::GetDimensions", 0x0088AAC0, Entry_GetDimensionsImpl);
C39O_REVERSED("NiD3DShaderConstantMap::GetEntryIndex", 0x0088AC30, ConstantMap_GetEntryIndex);
C39O_REVERSED("NiD3DShaderConstantMap::SetupPredefinedEntry", 0x0088AC80, ConstantMap_SetupPredefinedEntry);
C39O_REVERSED("NiD3DShaderConstantMap::SetupObjectEntry_88AD90", 0x0088AD90, ConstantMap_SetupObjectEntry);
C39O_REVERSED("NiShaderConstantMapEntry::NiShaderConstantMapEntry", 0x0088B520, Entry_ConstructImpl);
C39O_REVERSED("NiShaderConstantMapEntry::~NiShaderConstantMapEntry", 0x0088B580, Entry_Destruct);
C39O_REVERSED("NiD3DShaderConstantMap::AddPredefinedEntry", 0x0088B610, ConstantMap_AddPredefinedEntry);
C39O_REVERSED("NiD3DShaderConstantMap::AddAttributeEntry", 0x0088B740, ConstantMap_AddAttributeEntry);
C39O_REVERSED("NiD3DShaderConstantMap::AddConstantEntry", 0x0088B890, ConstantMap_AddConstantEntry);
C39O_REVERSED("NiD3DShaderConstantMap::AddGlobalEntry", 0x0088BA40, ConstantMap_AddGlobalEntry);
C39O_REVERSED("NiD3DShaderConstantMap::AddOperatorEntry", 0x0088BC50, ConstantMap_AddOperatorEntry);
C39O_REVERSED("NiD3DShaderConstantMap::AddObjectEntry", 0x0088BD80, ConstantMap_AddObjectEntry);
C39O_REVERSED("NiD3DShaderConstantMap::RemoveEntry", 0x0088BEB0, ConstantMap_RemoveEntry);
C39O_REVERSED("NiD3DShaderConstantMap::SetEntryData", 0x0088BF50, ConstantMap_SetEntryData);

} // namespace ni::c39
