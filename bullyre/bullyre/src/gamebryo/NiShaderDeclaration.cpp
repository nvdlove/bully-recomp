// NiShader's base (vtable 0x953C68, 0x14 bytes: the name, the implementation, initialized),
// NiD3DShaderInterface's empty virtuals, and NiShaderDeclaration (vtable 0x953D08, 0x18 bytes):
// per stream, its register entries (each 0x18 bytes: the stream, the input register, the type,
// the usage name, the usage index, the method); the declaration a geometry's materials ask for
// (CreateDeclForGeometry: their vertex inputs merged, the default ones when a material says so,
// the renderer's declaration made and set on the geometry, +0xE4).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiGeometry.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct NiShaderBase : NiRefObject {
    re::Ptr32<const char> name;       // +0x8
    uint32_t implementation;  // +0xC
    uint8_t initialized;      // +0x10
    uint8_t unknown11;        // +0x11
    uint8_t pad12[2];
};
static_assert(sizeof(NiShaderBase) == 0x14);

// a register entry (ShaderRegisterEntry); a list of them ends at a usage index of -1
struct RegisterEntry {
    uint32_t stream;          // +0x0
    uint32_t input;           // +0x4 (-1: none)
    uint32_t type;            // +0x8 (0x11: unused)
    re::Ptr32<const char> usage;      // +0xC (one of the usage names at 0x20E31CC)
    uint32_t usageIndex;      // +0x10 (-1 ends a list; -2 in a material's: "the defaults too")
    uint32_t method;          // +0x14
};
static_assert(sizeof(RegisterEntry) == 0x18);

struct StreamEntries {
    int32_t entryCount;       // +0x0 (the highest set + 1)
    re::Ptr32<RegisterEntry> entries;   // +0x4 (new[]'d)
    uint32_t unknown8[2];
};
static_assert(sizeof(StreamEntries) == 0x10);

struct NiShaderDeclaration : NiObject {
    uint32_t maxEntries;      // +0x8
    uint32_t streamCount;     // +0xC
    re::Ptr32<StreamEntries> streams;   // +0x10
    uint8_t modified;         // +0x14
    uint8_t pad15[3];
};
static_assert(sizeof(NiShaderDeclaration) == 0x18);

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiShader_RTTI, 0x020E3164);
GAME_VAR(RTTI, NiShaderDeclaration_RTTI, 0x020E3204);

constexpr uintptr_t NiRefObject_vtable = 0x008FEFD4;
constexpr uintptr_t NiShader_vtable = 0x00953C68;
constexpr uintptr_t NiShaderDeclaration_vtable = 0x00953D08;
constexpr uintptr_t RegisterEntry_Destruct = 0x0077C5B0;

GAME_VAR(uint32_t, NiRefObject_ms_uiObjects, 0x020E2168);
GAME_ARRAY(NiFixedString, UsageNames, 0x020E31CC);   // POSITION, BLENDWEIGHT, BLENDINDICES, NORMAL, ...
GAME_VAR(void*, NiRenderer_ms_pkRenderer, 0x020E2380);

GAME_FN(void, __stdcall, VectorDestructor, 0x004017C0, void* p, uint32_t size, uint32_t count, void* destructor);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(uint32_t, __thiscall, Geometry_GetMaterialCount, 0x00760FD0, NiGeometry* self);
GAME_FN(NiMaterialInstance*, __thiscall, Geometry_GetMaterialInstance, 0x00760FE0, NiGeometry* self, uint32_t index);
GAME_FN(uint8_t, __thiscall, Geometry_ApplyMaterial, 0x00761E80, NiGeometry* self, NiObject* material,
        uint32_t extraData);
GAME_FN(uint32_t, __thiscall, Instance_GetExtraData, 0x00856850, const NiMaterialInstance* self);
GAME_FN(uint8_t, __thiscall, Skin_ContainsVertexData, 0x00776F00, void* self, int parameter);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, Shader_Destruct, 0x0077C350, NiShaderBase* self);
GAME_FN(uint8_t, __cdecl, Declaration_MergeEntries, 0x0077CB90, RegisterEntry* merged, const RegisterEntry* entries,
        uint32_t* count);
GAME_FN(void, __cdecl, Declaration_GetDefaultEntries, 0x0077CCB0, RegisterEntry* entries, NiGeometry* geometry);
GAME_FN(void*, __thiscall, RegisterEntries_VectorDelete, 0x0077CFC0, RegisterEntry* self, uint32_t flags);
GAME_FN(void, __thiscall, Declaration_Destruct, 0x0077D6E0, NiShaderDeclaration* self);

// NiFixedString's assignment, inlined (the import slots' interlocked calls)
inline void FixedStringSet(re::Ptr32<const char>* slot, NiFixedString value) {
    if (*slot == value) {
        return;
    }
    if (value != NullFixedString) {
        ni::sys::Increment(re::PtrCast<char*>(value) - 8);
    }
    NiFixedString old = *slot;
    if (old != NullFixedString) {
        uint32_t length = *reinterpret_cast<const uint16_t*>(old - 2);
        if (ni::sys::Decrement(re::PtrCast<char*>(old) - 8) == 1) {
            StringTable_RemoveString(GlobalStringTable, slot, length);
        }
    }
    *slot = value;
}

// NiFixedString's destructor, inlined (the import's decrement)
inline void FixedStringFree(re::Ptr32<const char>* slot) {
    ni::sys::FixedStringRelease(re::PtrCast<re::Ptr32<char>*>(slot));
}

// an entry with the given fields (the name assigned through NiFixedString's operator=, 0x401CD0)
inline void SetDefaultEntry(RegisterEntry* entry, uint32_t input, uint32_t type, uint32_t usage) {
    entry->stream = 0;
    entry->input = input;
    entry->type = type;
    FixedString_Assign(&entry->usage, &UsageNames[usage]);
    entry->usageIndex = 0;
    entry->method = 0;
}

// ---- NiShader

// GetName (4 bytes: unhooked)
re::Ptr32<const char>* __fastcall ShaderGetName(NiShaderBase* self, re::Edx) {
    return &self->name;
}

uint8_t __fastcall ShaderInitialize(NiShaderBase* self, re::Edx) {
    self->initialized = 1;
    return 1;
}

// NiD3DShaderInterface's empty virtuals (the count of their arguments: 7, 8, 4, 6, 3)
uint32_t __fastcall ReturnZero7(void*, re::Edx, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) {
    return 0;
}
uint32_t __fastcall ReturnZero8(void*, re::Edx, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                                uint32_t) {
    return 0;
}
uint32_t __fastcall ReturnZero4(void*, re::Edx, uint32_t, uint32_t, uint32_t, uint32_t) {
    return 0;
}
uint8_t __fastcall ReturnFalse6(void*, re::Edx, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) {
    return 0;
}
uint32_t __fastcall ReturnZero3(void*, re::Edx, uint32_t, uint32_t, uint32_t) {
    return 0;
}

NiShaderBase* __fastcall ShaderConstruct(NiShaderBase* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    self->refCount = 0;
    ni::sys::Increment(&NiRefObject_ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(NiShader_vtable);
    self->name = NullFixedString;
    self->implementation = 0;
    self->initialized = 0;
    self->unknown11 = 0;
    return self;
}

// IsInitialized (4 bytes: unhooked)
uint8_t __fastcall ShaderIsInitialized(NiShaderBase* self, re::Edx) {
    return self->initialized;
}

const RTTI* __fastcall ShaderGetRTTI(NiShaderBase*, re::Edx) {
    return &NiShader_RTTI;
}

void __fastcall ShaderDestruct(NiShaderBase* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiShader_vtable);
    FixedStringFree(&self->name);
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    ni::sys::Decrement(&NiRefObject_ms_uiObjects);
}

re::Ptr32<const char>* __fastcall ShaderSetName(NiShaderBase* self, re::Edx, const re::Ptr32<const char>* name) {
    return FixedString_Assign(&self->name, name);
}

// GetVertexInputSemantics(entries): the position only
uint8_t __fastcall GetVertexInputSemantics(void*, re::Edx, RegisterEntry* entries) {
    entries[0].stream = 0;
    entries[0].input = 0;
    entries[0].type = 2;
    FixedString_Assign(&entries[0].usage, &UsageNames[0]);
    entries[0].usageIndex = 0;
    entries[0].method = 0;
    entries[1].usageIndex = 0xFFFFFFFF;
    return 1;
}

NiShaderBase* __fastcall ShaderDeletingDestructor(NiShaderBase* self, re::Edx, uint32_t flags) {
    Shader_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x14);
    }
    return self;
}

// ---- NiShaderDeclaration

NiShaderDeclaration* __fastcall DeclarationConstruct(NiShaderDeclaration* self, re::Edx) {
    RefObject_Construct(self);
    self->maxEntries = 0;
    self->streamCount = 0;
    self->streams = nullptr;
    self->modified = 0;
    self->vtable = reinterpret_cast<void*>(NiShaderDeclaration_vtable);
    return self;
}

// vf20 ClearEntry(entry, stream)
uint8_t __fastcall ClearEntry(NiShaderDeclaration* self, re::Edx, uint32_t entry, uint32_t stream) {
    if (entry >= self->maxEntries || stream >= self->streamCount) {
        return 0;
    }
    RegisterEntry* e = &self->streams[stream].entries[entry];
    e->input = 0xFFFFFFFF;
    e->type = 0x11;
    Virtual<void>(self, 21);   // MarkAsModified
    return 1;
}

void __fastcall MarkAsModified(NiShaderDeclaration* self, re::Edx) {
    self->modified = 1;
}

const RTTI* __fastcall DeclarationGetRTTI(NiShaderDeclaration*, re::Edx) {
    return &NiShaderDeclaration_RTTI;
}

// vf18 SetEntry(entry, parameter, type, stream): the parameter's usage name and index, through
// slot 19
uint8_t __fastcall SetEntry(NiShaderDeclaration* self, re::Edx, uint32_t entry, uint32_t parameter, uint32_t type,
                            uint32_t stream) {
    uint32_t usage;
    uint32_t usageIndex = 0;
    switch (parameter) {
    case 0:
        usage = 0;
        break;
    case 1:
        usage = 1;
        break;
    case 2:
        usage = 2;
        break;
    case 3:
        usage = 3;
        break;
    case 4:
        usage = 10;
        break;
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
        usage = 5;
        usageIndex = parameter - 5;
        break;
    case 13:
        usage = 6;
        break;
    case 14:
        usage = 7;
        break;
    default:
        return 0;
    }
    return Virtual<uint8_t>(self, 19, stream, entry, parameter, type, &UsageNames[usage], usageIndex, 0u);
}

// ShaderRegisterEntry's constructor
RegisterEntry* __fastcall RegisterEntryConstruct(RegisterEntry* self, re::Edx) {
    self->stream = 0;
    self->input = 0xFFFFFFFF;
    self->type = 0x11;
    self->usage = NullFixedString;
    self->usageIndex = 0;
    self->method = 0;
    FixedString_Assign(&self->usage, &UsageNames[0]);
    return self;
}

// vf19 SetEntry(stream, entry, input, type, usage, usageIndex, method): changed only when it
// differs (a texture coordinate input forces type 2)
uint8_t __fastcall SetEntryForStream(NiShaderDeclaration* self, re::Edx, uint32_t stream, uint32_t entry,
                                     uint32_t input, uint32_t type, const re::Ptr32<const char>* usage, uint32_t usageIndex,
                                     uint32_t method) {
    if (entry >= self->maxEntries || stream >= self->streamCount) {
        return 0;
    }
    StreamEntries* s = &self->streams[stream];
    RegisterEntry* e = &s->entries[entry];
    if (e->input != input || e->type != type || e->usage != *usage || e->usageIndex != usageIndex ||
        e->method != method) {
        e->type = type;
        e->input = input;
        FixedString_Assign(&e->usage, usage);
        e->method = method;
        e->usageIndex = usageIndex;
        int32_t count = s->entryCount;
        if (int32_t(entry + 1) > count) {
            count = int32_t(entry + 1);
        }
        s->entryCount = count;
        if (input == 1) {
            e->type = 2;
        }
        Virtual<void>(self, 21);
    }
    return 1;
}

// MergeEntries(merged, entries, count): the entries added unless their usage and index are
// there; false when the 16 run out
uint8_t __cdecl MergeEntries(RegisterEntry* merged, const RegisterEntry* entries, uint32_t* count) {
    for (const RegisterEntry* e = entries; e->usageIndex != 0xFFFFFFFF; e++) {
        uint32_t n = *count;
        if (n == 0x10) {
            return 0;
        }
        bool found = false;
        for (uint32_t i = 0; i < n; i++) {
            if (merged[i].usage == e->usage && merged[i].usageIndex == e->usageIndex) {
                found = true;
                break;
            }
        }
        if (found) {
            continue;
        }
        merged[n].stream = e->stream;
        merged[*count].input = e->input;
        merged[*count].type = e->type;
        FixedStringSet(&merged[*count].usage, e->usage);
        merged[*count].usageIndex = e->usageIndex;
        merged[*count].method = e->method;
        *count += 1;
    }
    return 1;
}

// a vertex input the data or its skin has
inline bool HasInput(NiGeometry* geometry, int parameter) {
    if (Virtual<uint8_t>(geometry->modelData, 19, parameter)) {   // ContainsVertexData
        return true;
    }
    return geometry->skinInstance != nullptr && Skin_ContainsVertexData(geometry->skinInstance, parameter);
}

// GetDefaultEntries(entries, geometry): position; the blend weights and (with bones) indices for
// a skin with a partition; normal, color, the texture coordinates, binormal and tangent as the
// data has them
void __cdecl GetDefaultEntries(RegisterEntry* entries, NiGeometry* geometry) {
    SetDefaultEntry(&entries[0], 0, 2, 0);
    uint32_t n = 1;
    NiObject* skin = geometry->skinInstance;
    if (skin != nullptr) {
        void* partition = At<void*>(skin, 0xC);
        if (partition != nullptr) {
            void* partitions = At<void*>(partition, 0xC);
            entries[1].input = 1;
            entries[1].stream = 0;
            entries[1].type = 2;
            FixedString_Assign(&entries[1].usage, &UsageNames[1]);
            entries[1].usageIndex = 0;
            entries[1].method = 0;
            n = 2;
            if (At<uint32_t>(partition, 8) != 0 && partitions != nullptr && At<uint32_t>(partitions, 0x10) != 0) {
                entries[2].stream = 0;
                entries[2].input = 2;
                entries[2].type = 5;
                FixedString_Assign(&entries[2].usage, &UsageNames[2]);
                entries[2].usageIndex = 0;
                entries[2].method = 0;
                n = 3;
            }
        }
    }
    if (HasInput(geometry, 3)) {
        SetDefaultEntry(&entries[n], 3, 2, 3);
        n++;
    }
    if (HasInput(geometry, 4)) {
        SetDefaultEntry(&entries[n], 4, 4, 10);
        n++;
    }
    for (uint32_t parameter = 5; parameter - 5 < 8; parameter++) {
        if (HasInput(geometry, int(parameter))) {
            RegisterEntry* e = &entries[n];
            e->stream = 0;
            e->input = parameter;
            e->type = 1;
            FixedStringSet(&e->usage, UsageNames[5]);
            e->usageIndex = parameter - 5;
            e->method = 0;
            n++;
        }
    }
    if ((At<uint16_t>(geometry->modelData, 0x32) & 0xF000) != 0 && HasInput(geometry, 0xE) && HasInput(geometry, 0xD)) {
        SetDefaultEntry(&entries[n], 0xE, 2, 7);
        n++;
        SetDefaultEntry(&entries[n], 0xD, 2, 6);
        n++;
    }
    entries[n].usageIndex = 0xFFFFFFFF;
}

// ShaderRegisterEntry's vector deleting destructor
RegisterEntry* __fastcall RegisterEntriesVectorDelete(RegisterEntry* self, re::Edx, uint32_t flags) {
    if ((flags & 2) != 0) {
        uint32_t* header = reinterpret_cast<uint32_t*>(self) - 1;
        for (int32_t i = int32_t(*header) - 1; i >= 0; i--) {
            FixedStringFree(&self[i].usage);
        }
        if ((flags & 1) != 0) {
            NiDeleteArray(header, 0x18);
        }
        return reinterpret_cast<RegisterEntry*>(header);
    }
    FixedStringFree(&self->usage);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x18);
    }
    return self;
}

// a vertex input semantics query (slot 5 of the material): merged when it answers; a material
// that doesn't but asks for the defaults (usage index -2) flags them
inline void QueryMaterial(NiGeometry* geometry, NiMaterialInstance* instance, RegisterEntry* entries,
                          RegisterEntry* merged, uint32_t* count, uint8_t* any, uint8_t* needDefault) {
    NiObject* material = instance->material;
    uint32_t extraData = Instance_GetExtraData(instance);
    uint8_t answered = Virtual<uint8_t>(material, 5, geometry, extraData, entries);
    *any |= answered;
    if (answered) {
        Declaration_MergeEntries(merged, entries, count);
    } else if (entries[0].usageIndex == 0xFFFFFFFE) {
        *needDefault = 1;
    }
}

// the entries' destructors (the last first)
inline void FreeEntries(RegisterEntry* entries) {
    for (int i = 15; i >= 0; i--) {
        FixedStringFree(&entries[i].usage);
    }
}

// CreateDeclForGeometry(geometry): the active material first, then the others but the
// renderer's default, then the default (applied when nothing answered); the renderer's
// declaration (slot 47) for the streams and entries, set on the geometry
uint8_t __cdecl CreateDeclForGeometry(NiGeometry* geometry) {
    if (geometry == nullptr) {
        return 0;
    }
    RegisterEntry merged[16];
    RegisterEntry entries[16];
    RegisterEntry* lists[2] = {merged, entries};
    for (RegisterEntry* list : lists) {
        for (int i = 0; i < 16; i++) {
            list[i].stream = 0;
            list[i].input = 0xFFFFFFFF;
            list[i].type = 0x11;
            list[i].usage = NullFixedString;
            list[i].usageIndex = 0;
            list[i].method = 0;
            FixedStringSet(&list[i].usage, UsageNames[0]);
        }
    }
    entries[0].usageIndex = 0xFFFFFFFF;
    uint32_t count = 0;
    uint8_t needDefault = 0;
    uint8_t any = 0;
    NiMaterialInstance* active = nullptr;
    if (geometry->activeMaterial < geometry->materialCount) {
        active = &geometry->materials[geometry->activeMaterial];
        if (active != nullptr) {
            QueryMaterial(geometry, active, entries, merged, &count, &any, &needDefault);
        }
    }
    void* renderer = NiRenderer_ms_pkRenderer;
    if (renderer == nullptr) {
        VectorDestructor(entries, 0x18, 0x10, reinterpret_cast<void*>(RegisterEntry_Destruct));
        VectorDestructor(merged, 0x18, 0x10, reinterpret_cast<void*>(RegisterEntry_Destruct));
        return 0;
    }
    NiObject* defaultMaterial = At<NiObject*>(renderer, 0x18);
    NiMaterialInstance* defaultInstance = nullptr;
    uint32_t materials = Geometry_GetMaterialCount(geometry);
    for (uint32_t i = 0; i < materials; i++) {
        NiMaterialInstance* instance = Geometry_GetMaterialInstance(geometry, i);
        if (instance == nullptr || instance == active) {
            continue;
        }
        if (instance->material == defaultMaterial) {
            defaultInstance = instance;
            continue;
        }
        QueryMaterial(geometry, instance, entries, merged, &count, &any, &needDefault);
    }
    NiMaterialInstance* last = defaultInstance;
    if (count == 0) {
        Geometry_ApplyMaterial(geometry, defaultMaterial, 0xFFFFFFFF);
        uint32_t n = Geometry_GetMaterialCount(geometry);
        for (uint32_t i = 0; i < n; i++) {
            NiMaterialInstance* instance = Geometry_GetMaterialInstance(geometry, i);
            if (instance != nullptr && instance->material == defaultMaterial) {
                last = instance;
                break;
            }
        }
    }
    bool buildDefault = false;
    if (last != nullptr) {
        NiObject* material = last->material;
        uint32_t extraData = Instance_GetExtraData(last);
        uint8_t answered = Virtual<uint8_t>(material, 5, geometry, extraData, entries);
        any |= answered;
        if (answered) {
            Declaration_MergeEntries(merged, entries, &count);
        } else if (entries[0].usageIndex == 0xFFFFFFFE) {
            buildDefault = true;
        }
    }
    if (!buildDefault) {
        if (needDefault) {
            buildDefault = true;
        } else if (!any) {
            FreeEntries(entries);
            FreeEntries(merged);
            return 0;
        }
    }
    if (buildDefault) {
        Declaration_GetDefaultEntries(entries, geometry);
        Declaration_MergeEntries(merged, entries, &count);
    }
    int32_t streams = 0;
    int32_t maxEntries = 0;
    for (uint32_t i = 0; i < count; i++) {
        int32_t next = int32_t(merged[i].stream) + 1;
        if (!(streams > next)) {
            streams = next;
        }
    }
    for (uint32_t s = 0; s < uint32_t(streams); s++) {
        int32_t c = 0;
        for (uint32_t i = 0; i < count; i++) {
            if (merged[i].stream == s) {
                c++;
            }
        }
        if (!(maxEntries > c)) {
            maxEntries = c;
        }
    }
    NiObject* declaration = Virtual<NiObject*>(renderer, 47, uint32_t(maxEntries), uint32_t(streams));
    if (declaration == nullptr) {
        FreeEntries(entries);
        FreeEntries(merged);
        return 0;
    }
    for (uint32_t s = 0; s < uint32_t(streams); s++) {
        uint32_t e = 0;
        for (uint32_t i = 0; i < count; i++) {
            RegisterEntry* m = &merged[i];
            if (m->stream == s) {
                Virtual<uint8_t>(declaration, 19, s, e, m->input, m->type, &m->usage, m->usageIndex, m->method);
                e++;
            }
        }
    }
    re::Ptr32<NiObject>& slot = *reinterpret_cast<re::Ptr32<NiObject>*>(&geometry->unknownE4);
    NiObject* old = slot;
    if (old != declaration) {
        if (old != nullptr && ni::sys::Decrement(&old->refCount) == 0) {
            Virtual<void>(old, 1);
        }
        slot = declaration;
        ni::sys::Increment(&declaration->refCount);
    }
    FreeEntries(entries);
    FreeEntries(merged);
    return 1;
}

// ~NiShaderDeclaration: the streams' entries (vector-deleted) and the streams
void __fastcall DeclarationDestruct(NiShaderDeclaration* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiShaderDeclaration_vtable);
    if (self->streams != nullptr) {
        for (uint32_t i = 0; i < self->streamCount; i++) {
            StreamEntries* s = &self->streams[i];
            if (s != nullptr && s->entries != nullptr) {
                RegisterEntries_VectorDelete(s->entries, 3);
            }
        }
        NiDeleteArray(self->streams, 0x10);
    }
    self->streams = nullptr;
    Object_Destruct(self);
}

NiShaderDeclaration* __fastcall DeclarationDeletingDestructor(NiShaderDeclaration* self, re::Edx, uint32_t flags) {
    Declaration_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x18);
    }
    return self;
}

} // namespace

// 0x0077C280: [8d 41 08 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static void* __fastcall Tiny_0077C280(uint8_t* self, re::Edx) {
    return self + 0x8;
}
RE_UNHOOKED_FN("NiShader::GetName", 0x0077C280, &Tiny_0077C280);        // 4 bytes
// 0x0077C330: [8a 41 10 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __fastcall Tiny_0077C330(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const uint8_t*>(self + 0x10);
}
RE_UNHOOKED_FN("NiShader::IsInitialized", 0x0077C330, &Tiny_0077C330);  // 4 bytes

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISHADERDECL_REVERSED(name, address, fn)                                               \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISHADERDECL_REVERSED("NiShader::Initialize", 0x0077C290, &ni::ShaderInitialize);
NISHADERDECL_REVERSED("NiD3DShaderInterface::vf6", 0x0077C2A0, &ni::ReturnZero7);
NISHADERDECL_REVERSED("NiD3DShaderInterface::vf9", 0x0077C2B0, &ni::ReturnZero8);
NISHADERDECL_REVERSED("NiD3DShaderInterface::vf11", 0x0077C2C0, &ni::ReturnZero4);
NISHADERDECL_REVERSED("NiD3DShaderInterface::vf20", 0x0077C2D0, &ni::ReturnFalse6);
NISHADERDECL_REVERSED("NiD3DShaderInterface::vf23", 0x0077C2E0, &ni::ReturnZero3);
NISHADERDECL_REVERSED("NiShader::NiShader", 0x0077C2F0, &ni::ShaderConstruct);
NISHADERDECL_REVERSED("NiShader::GetRTTI", 0x0077C340, &ni::ShaderGetRTTI);
NISHADERDECL_REVERSED("NiShader::~NiShader", 0x0077C350, &ni::ShaderDestruct);
NISHADERDECL_REVERSED("NiShader::SetName", 0x0077C3A0, &ni::ShaderSetName);
NISHADERDECL_REVERSED("NiD3DShaderInterface::GetVertexInputSemantics", 0x0077C3B0, &ni::GetVertexInputSemantics);
NISHADERDECL_REVERSED("NiShader::DeletingDestructor", 0x0077C400, &ni::ShaderDeletingDestructor);
NISHADERDECL_REVERSED("NiShaderDeclaration::NiShaderDeclaration", 0x0077C420, &ni::DeclarationConstruct);
NISHADERDECL_REVERSED("NiShaderDeclaration::ClearEntry", 0x0077C440, &ni::ClearEntry);
NISHADERDECL_REVERSED("NiShaderDeclaration::MarkAsModified", 0x0077C490, &ni::MarkAsModified);
NISHADERDECL_REVERSED("NiShaderDeclaration::GetRTTI", 0x0077C4A0, &ni::DeclarationGetRTTI);
NISHADERDECL_REVERSED("NiShaderDeclaration::SetEntry", 0x0077C4B0, &ni::SetEntry);
NISHADERDECL_REVERSED("NiShaderDeclaration::ShaderRegisterEntry::ShaderRegisterEntry", 0x0077C5F0,
                      &ni::RegisterEntryConstruct);
NISHADERDECL_REVERSED("NiShaderDeclaration::SetEntryForStream", 0x0077CAE0, &ni::SetEntryForStream);
NISHADERDECL_REVERSED("NiShaderDeclaration::MergeEntries", 0x0077CB90, &ni::MergeEntries);
NISHADERDECL_REVERSED("NiShaderDeclaration::GetDefaultEntries", 0x0077CCB0, &ni::GetDefaultEntries);
NISHADERDECL_REVERSED("NiShaderDeclaration::ShaderRegisterEntry::VectorDeletingDestructor", 0x0077CFC0,
                      &ni::RegisterEntriesVectorDelete);
NISHADERDECL_REVERSED("NiShaderDeclaration::CreateDeclForGeometry", 0x0077D090, &ni::CreateDeclForGeometry);
NISHADERDECL_REVERSED("NiShaderDeclaration::~NiShaderDeclaration", 0x0077D6E0, &ni::DeclarationDestruct);
NISHADERDECL_REVERSED("NiShaderDeclaration::DeletingDestructor", 0x0077D750, &ni::DeclarationDeletingDestructor);
