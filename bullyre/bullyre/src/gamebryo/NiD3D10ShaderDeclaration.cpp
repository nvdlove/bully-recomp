// The D3D10 renderer's shader declaration (vtable 0x9A0800, 0x20 bytes; the name a guess: its
// GetRTTI is NiShaderDeclaration's own, 0x77C4A0): NiShaderDeclaration (src/gamebryo/
// NiShaderDeclaration.cpp: +0x8 the entries per stream, +0xC the streams, +0x10 the streams'
// array, +0x14 modified) with +0x18 its D3D10_INPUT_ELEMENT_DESC array (NiMalloc'd, 0x1C each) and
// +0x1C their count, made again from the streams' entries when the declaration has changed. A
// stream (0x10): +0x0 its entry count, +0x4 its entries (NiShaderDeclaration::ShaderRegisterEntry,
// 0x18 each: +0x0 the packing offset, +0x4 the input register (-1 none), +0x8 the D3D type (0x11
// unused), +0xC the usage name, +0x10 the usage index), +0x8 its vertex stride, +0xC used.
//
// Also the static tables of the D3D types (the D3DDECLTYPE numbers, 0x11 of them and "unused"):
// their DXGI formats (0x20E8FB8) and sizes in bytes (0x20E8F70), filled by the renderer's
// _SDMInit.

#include "gamebryo/NiD3D10C43.h"
#include "reversed.h"

namespace ni::c43 {
namespace {

constexpr uintptr_t kDeclarationVtable = 0x009A0800;
constexpr uintptr_t kStreamConstructor = 0x008A1200;   // NiD3DShaderDeclaration's stream (0x10)

GAME_ARRAY(uint32_t, ms_aeTypeFormats, 0x020E8FB8);   // DXGI_FORMAT by D3D type (0x12)
GAME_ARRAY(uint32_t, ms_auiTypeSizes, 0x020E8F70);    // bytes by D3D type (0x12)

struct InputElementDesc {
    re::Ptr32<const char> semanticName;        // +0x0
    uint32_t semanticIndex;          // +0x4
    uint32_t format;                 // +0x8
    uint32_t inputSlot;              // +0xC
    uint32_t alignedByteOffset;      // +0x10
    uint32_t inputSlotClass;         // +0x14
    uint32_t instanceDataStepRate;   // +0x18
};
static_assert(sizeof(InputElementDesc) == 0x1C);

struct RegisterEntry {
    uint32_t packingOffset;   // +0x0
    uint32_t input;           // +0x4
    int32_t type;             // +0x8
    re::Ptr32<const char> usage;        // +0xC
    uint32_t usageIndex;      // +0x10
    uint32_t extraData;       // +0x14
};
static_assert(sizeof(RegisterEntry) == 0x18);

struct Stream {
    uint32_t entryCount;      // +0x0
    re::Ptr32<RegisterEntry> entries;   // +0x4
    uint32_t stride;          // +0x8
    uint8_t used;             // +0xC
    uint8_t padD[3];
};
static_assert(sizeof(Stream) == 0x10);

struct Declaration {
    re::Ptr32<void> vtable;                 // +0x0
    uint32_t refCount;            // +0x4
    uint32_t maxEntries;          // +0x8
    uint32_t streamCount;         // +0xC
    re::Ptr32<Stream> streams;              // +0x10
    uint8_t modified;             // +0x14
    uint8_t pad15[3];
    re::Ptr32<InputElementDesc> elements;   // +0x18
    uint32_t elementCount;        // +0x1C
};
static_assert(sizeof(Declaration) == 0x20);

GAME_FN(uint32_t, __cdecl, D3D10_GetFormatBits, 0x008A87E0, uint32_t format);   // (chunk 42's)
GAME_FN(void*, __thiscall, ShaderDeclaration_ConstructBase, 0x0077C420, void* self);
GAME_FN(void, __thiscall, ShaderDeclaration_DestructBase, 0x0077D6E0, void* self);
GAME_FN(void, __thiscall, ShaderRegisterEntry_Construct, 0x0077C5F0, void* self);
GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __stdcall, VectorConstructor, 0x00401240, void* p, uint32_t size, uint32_t count, uintptr_t ctor);
// this file's (called where modes apply)
GAME_FN(uint8_t, __thiscall, Declaration_Initialize, 0x008B4160, Declaration* self, uint32_t maxEntries,
        uint32_t streams);
GAME_FN(uint32_t, __thiscall, Declaration_FillInputElement, 0x008B4250, Declaration* self, RegisterEntry* entry,
        uint32_t stream, uint32_t index);
GAME_FN(void, __thiscall, Declaration_UpdateInputElements, 0x008B43E0, Declaration* self);

// an array size as operator new[] computes it (-1 when it overflows; the count's word added)
inline uint32_t CheckedSize(uint32_t count, uint32_t size, bool header) {
    uint64_t bytes = uint64_t(count) * size;
    uint32_t total = (bytes >> 32) != 0 ? 0xFFFFFFFFu : uint32_t(bytes);
    if (header) {
        total = total > 0xFFFFFFFBu ? 0xFFFFFFFFu : total + 4;
    }
    return total;
}

// a declaration made (NiShaderDeclaration's constructor, this class's vtable, no elements)
Declaration* NewDeclaration() {
    Declaration* declaration = static_cast<Declaration*>(NiNew(0x20));
    if (declaration == nullptr) {
        return nullptr;
    }
    ShaderDeclaration_ConstructBase(declaration);
    declaration->vtable = reinterpret_cast<void*>(kDeclarationVtable);
    declaration->elements = nullptr;
    declaration->elementCount = 0;
    return declaration;
}

inline bool IsUsed(const RegisterEntry& entry) {
    return entry.input != 0xFFFFFFFF && entry.type < 0x11;
}

} // namespace

// 0x8B4010 (cdecl): the D3D types' DXGI formats (FLOAT1..4: R32 .. R32G32B32A32_FLOAT,
// D3DCOLOR and UBYTE4N R8G8B8A8_UNORM, UBYTE4 R8G8B8A8_UINT, SHORT2/4 R16G16(B16A16)_SINT, SHORT2N/
// 4N _SNORM, USHORT2N/4N _UNORM, UDEC3 R10G10B10A2_UINT, DEC3N R10G10B10A2_UNORM, FLOAT16_2/4
// R16G16(B16A16)_FLOAT, unused UNKNOWN) and their sizes from the formats' bits
void __cdecl ShaderDeclaration_InitTypeTables() {
    static const uint32_t kFormats[0x12] = {0x29, 0x10, 6, 2, 0x1C, 0x1E, 0x26, 0xE, 0x1C,
                                            0x25, 0xD, 0x23, 0xB, 0x19, 0x18, 0x22, 0xA, 0};
    for (uint32_t i = 0; i < 0x12; i++) {
        ms_aeTypeFormats[i] = kFormats[i];
    }
    for (uint32_t i = 0; i < 0x12; i++) {
        ms_auiTypeSizes[i] = D3D10_GetFormatBits(ms_aeTypeFormats[i]) >> 3;
    }
}

// 0x8B4100 (cdecl): a D3D type's size in bytes (0 past the table)
uint32_t __cdecl ShaderDeclaration_GetTypeSize(int32_t type) {
    if (type < 0x12) {
        return ms_auiTypeSizes[type];
    }
    return 0;
}

// 0x8B4120 (thiscall): the declaration's deleting destructor (its elements freed)
Declaration* __fastcall ShaderDeclaration_DeletingDestructor(Declaration* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(kDeclarationVtable);
    NiFree(self->elements);
    ShaderDeclaration_DestructBase(self);
    if (flags & 1) {
        NiDelete(self, 0x20);
    }
    return self;
}

// 0x8B4160 (thiscall): the declaration's Initialize(entries per stream, streams): the streams'
// array (new[], each stream constructed) and each stream's entries (new[] with its count, each
// entry constructed), each stream's entry count set; true
uint8_t __fastcall ShaderDeclaration_Initialize(Declaration* self, re::Edx, uint32_t maxEntries, uint32_t streams) {
    self->maxEntries = maxEntries;
    self->streamCount = streams;
    void* streamArray = NiNewArray(CheckedSize(streams, 0x10, false));
    if (streamArray != nullptr) {
        VectorConstructor(streamArray, 0x10, streams, kStreamConstructor);
    } else {
        streamArray = nullptr;
    }
    self->streams = static_cast<Stream*>(streamArray);
    for (uint32_t s = 0; s < self->streamCount; s++) {
        uint32_t count = self->maxEntries;
        uint32_t* memory = static_cast<uint32_t*>(NiNewArray(CheckedSize(count, 0x18, true)));
        RegisterEntry* entries = nullptr;
        if (memory != nullptr) {
            *memory = count;
            entries = reinterpret_cast<RegisterEntry*>(memory + 1);
            RegisterEntry* entry = entries;
            for (int32_t i = int32_t(count) - 1; i >= 0; i--) {
                ShaderRegisterEntry_Construct(entry);
                entry++;
            }
        }
        self->streams[s].entries = entries;
        self->streams[s].entryCount = self->maxEntries;
    }
    return 1;
}

// 0x8B4250 (thiscall): an entry's input element filled at an index (its usage and index, the
// format of its type, the stream as the slot, its packing offset; per vertex); the type's size
uint32_t __fastcall ShaderDeclaration_FillInputElement(Declaration* self, re::Edx, const RegisterEntry* entry,
                                                       uint32_t stream, uint32_t index) {
    InputElementDesc& element = self->elements[index];
    element.semanticName = entry->usage;
    element.semanticIndex = entry->usageIndex;
    element.format = ms_aeTypeFormats[entry->type];
    element.inputSlot = stream;
    element.alignedByteOffset = entry->packingOffset;
    element.inputSlotClass = 0;
    element.instanceDataStepRate = 0;
    return ms_auiTypeSizes[entry->type];
}

// 0x8B42C0 (cdecl): the declaration Create(entries per stream, streams, NiPointer& result): no more
// than 15 streams; a new declaration assigned to the pointer and initialized (let go again when
// that fails); whether the pointer is set. (0x8B42CA, which the chunk lists apart, is this
// function's body after the stream check: the jbe's target, no callers.)
uint8_t __cdecl ShaderDeclaration_CreateInto(uint32_t maxEntries, uint32_t streams, re::Ptr32<Declaration>* result) {
    if (streams > 0xF) {
        return 0;
    }
    Declaration* declaration = NewDeclaration();
    Declaration* old = *result;
    if (old != declaration) {
        if (old != nullptr && Decrement(&old->refCount) == 0) {
            Virtual<void>(old, 1);
        }
        *result = declaration;
        if (declaration == nullptr) {
            return *result != nullptr ? 1 : 0;
        }
        Increment(&declaration->refCount);
    }
    if (*result != nullptr && Declaration_Initialize(*result, maxEntries, streams) == 0) {
        Declaration* made = *result;
        if (made != nullptr) {
            if (Decrement(&made->refCount) == 0) {
                Virtual<void>(made, 1);
            }
            *result = nullptr;
        }
    }
    return *result != nullptr ? 1 : 0;
}

// 0x8B4380 (cdecl): the declaration Create(entries per stream, streams): a new one initialized,
// deleted when that fails (initialized even when the allocation failed, as the game does)
Declaration* __cdecl ShaderDeclaration_Create(uint32_t maxEntries, uint32_t streams) {
    Declaration* declaration = NewDeclaration();
    if (Declaration_Initialize(declaration, maxEntries, streams) == 0) {
        if (declaration != nullptr) {
            Virtual<void>(declaration, 0, 1u);
        }
        return nullptr;
    }
    return declaration;
}

// 0x8B43E0 (thiscall): the declaration's input elements made again when it has changed: the used
// entries (an input register, a type) counted and their streams marked; the array freed when the
// count changed and allocated when there's none; each used entry's packing offset set and its
// element filled, each stream's stride set. (The modified flag is only cleared when there's at
// least one element, as in the game.)
void __fastcall ShaderDeclaration_UpdateInputElements(Declaration* self, re::Edx) {
    if (self->modified == 0) {
        return;
    }
    uint32_t count = 0;
    for (uint32_t s = 0; s < self->streamCount; s++) {
        Stream* stream = self->streams + s;
        if (stream != nullptr) {
            stream->used = 0;
            for (uint32_t e = 0; e < self->maxEntries; e++) {
                if (IsUsed(stream->entries[e])) {
                    count++;
                    stream->used = 1;
                }
            }
        }
    }
    if (self->elementCount != count) {
        NiFree(self->elements);
        self->elements = nullptr;
    }
    self->elementCount = count;
    if (count == 0) {
        return;
    }
    if (self->elements == nullptr) {
        self->elements = static_cast<InputElementDesc*>(NiMalloc(count * 0x1C));
    }
    uint32_t index = 0;
    for (uint32_t s = 0; s < self->streamCount; s++) {
        Stream* stream = self->streams + s;
        if (stream != nullptr && stream->used != 0) {
            uint32_t offset = 0;
            for (uint32_t e = 0; e < self->maxEntries; e++) {
                RegisterEntry* entry = &stream->entries[e];
                if (IsUsed(*entry)) {
                    entry->packingOffset = offset;
                    offset += Declaration_FillInputElement(self, entry, s, index);
                    index++;
                }
            }
            stream->stride = offset;
        }
    }
    self->modified = 0;
}

// 0x8B4510 (thiscall): the declaration's vertex stride for a stream (the elements brought up to
// date first; 0 past the streams)
uint32_t __fastcall ShaderDeclaration_GetVertexStride(Declaration* self, re::Edx, uint32_t stream) {
    if (stream >= self->streamCount) {
        return 0;
    }
    Declaration_UpdateInputElements(self);
    return self->streams[stream].stride;
}

// 0x8B4540 (thiscall): an entry's packing offset in a stream (-1 past the streams or the stream's
// entries)
uint32_t __fastcall ShaderDeclaration_GetPackingOffset(Declaration* self, re::Edx, uint32_t stream, uint32_t entry) {
    if (stream < self->streamCount && entry < self->streams[stream].entryCount) {
        Declaration_UpdateInputElements(self);
        return self->streams[stream].entries[entry].packingOffset;
    }
    return 0xFFFFFFFF;
}

#define GB43D_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

GB43D_REVERSED("NiD3D10ShaderDeclaration::InitTypeTables", 0x008B4010, ShaderDeclaration_InitTypeTables);
GB43D_REVERSED("NiD3D10ShaderDeclaration::GetTypeSize", 0x008B4100, ShaderDeclaration_GetTypeSize);
GB43D_REVERSED("NiD3D10ShaderDeclaration::DeletingDestructor", 0x008B4120, ShaderDeclaration_DeletingDestructor);
GB43D_REVERSED("NiD3D10ShaderDeclaration::Initialize", 0x008B4160, ShaderDeclaration_Initialize);
GB43D_REVERSED("NiD3D10ShaderDeclaration::FillInputElement", 0x008B4250, ShaderDeclaration_FillInputElement);
GB43D_REVERSED("NiD3D10ShaderDeclaration::CreateInto", 0x008B42C0, ShaderDeclaration_CreateInto);
RE_UNHOOKED("NiD3D10ShaderDeclaration::CreateInto_8B42CA", 0x008B42CA);   // the body of 0x8B42C0
GB43D_REVERSED("NiD3D10ShaderDeclaration::Create", 0x008B4380, ShaderDeclaration_Create);
GB43D_REVERSED("NiD3D10ShaderDeclaration::UpdateInputElements", 0x008B43E0, ShaderDeclaration_UpdateInputElements);
GB43D_REVERSED("NiD3D10ShaderDeclaration::GetVertexStride", 0x008B4510, ShaderDeclaration_GetVertexStride);
GB43D_REVERSED("NiD3D10ShaderDeclaration::GetPackingOffset", 0x008B4540, ShaderDeclaration_GetPackingOffset);

} // namespace ni::c43
