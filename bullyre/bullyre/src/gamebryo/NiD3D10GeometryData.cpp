// The D3D10 renderer's geometry buffers.
//
// NiD3D10GeometryData (vtable 0x9A0FBC on NiGeometryBufferData's 0x99CEE8: 0 the deleting
// destructor, 1 ContainsVertexData; 0x4C bytes; "NiD3D10GeometryBuffer" in its messages): the
// buffers of a geometry, in sets (skin partitions): +0x4 the set count, +0x8 their primitives'
// running offsets (NiMalloc'd, count + 1: the last is the total), +0x10 / +0x14 its input element
// descriptions (copied, 0x1C each) and their count, +0x18 the input layout last chosen, +0x1C the
// input layouts made per shader signature (NiTPointerMap, 7 buckets: the values held), +0x2C the
// vertex buffer count, +0x30 their NiD3D10DataStreams (new[]'d NiPointers), +0x34 their
// ID3D10Buffers (held), +0x38 their strides, +0x3C the primitives (new[]'d, 0x24 each: vtable
// 0x9A0D3C, +0x4 the index NiD3D10DataStream (a reference), +0x8 its ID3D10Buffer (held), +0xC its
// index format, +0x10 the index buffer offset, +0x14 the vertex buffer offsets (NiMalloc'd), +0x18
// the index count, +0x1C the start index, +0x20 the base vertex: the names from the renderer's draw
// calls), +0x40 a value from the constructor, +0x44 an owned object
// (deleted by slot 0), +0x48 flags (0x200000 tangents and binormals, 0x400000 colours (a guess),
// 0x800000 normals (a guess), the top byte the texture set count).
//
// NiD3D10DataStream (vtable 0x9A0FFC: 0 the deleting destructor, 1 DeleteThis, 2 Lock, 3 Unlock;
// 0x28 bytes): an ID3D10Buffer (+0x8) of +0xC bytes, +0x10 its access flags and +0x14 its usage
// flags in Gamebryo's terms, +0x18 the misc flags, +0x1C its D3D10_USAGE, +0x20 locked, +0x24 the
// staging stream a default buffer is locked through (or, for a staging stand-in of an immutable
// buffer made without data, the usage flags it stands in for). Usage flags: 1 vertex, 2 index, 4
// constant buffer (D3D10's bind flags 1 and 2 swapped).

#include "gamebryo/NiD3D10C43.h"
#include "reversed.h"

namespace ni::c43 {
namespace {

constexpr uintptr_t kGeometryDataVtable = 0x009A0FBC;
constexpr uintptr_t kGeometryBufferDataVtable = 0x0099CEE8;   // NiGeometryBufferData
constexpr uintptr_t kPrimitiveVtable = 0x009A0D3C;
constexpr uintptr_t kMapBaseVtable = 0x00950760;              // NiTMapBase
constexpr uintptr_t kLayoutMapVtable = 0x0098AD94;            // NiTPointerMap<...>
constexpr uintptr_t kDataStreamVtable = 0x009A0FFC;
constexpr uintptr_t kPrimitiveConstructor = 0x008B76C0;

GAME_FN(uint8_t*, __cdecl, D3D10Renderer_GetRenderer, 0x008A9300);
GAME_FN(void*, __thiscall, D3D10Renderer_GetResourceManager, 0x008A9120, const void* renderer);
GAME_FN(void*, __thiscall, D3D10Renderer_GetD3DDevice, 0x008A9140, const void* renderer);
GAME_FN(void*, __thiscall, ResourceManager_CreateBuffer, 0x008AFE60, void* self, uint32_t size, uint32_t bindFlags,
        uint32_t usage, uint32_t cpuAccess, uint32_t misc, const void* initialData);
GAME_FN(void, __thiscall, LayoutMap_SetAt, 0x008A5140, void* map, void* key, void* layout);
GAME_FN(void, __thiscall, Map_RemoveAll, 0x005AC520, void* map);
GAME_FN(void, __thiscall, NiPointer_Assign, 0x00800420, void* slot, void* object);
GAME_FN(void, __thiscall, NiPointer_Destruct, 0x007549D0, void* slot);
GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __stdcall, VectorConstructor, 0x00401240, void* p, uint32_t size, uint32_t count, uintptr_t ctor);
GAME_FN(void*, __cdecl, OperatorNew, 0x005EEAA0, uint32_t size);     // the game's ::operator new
GAME_FN(void, __cdecl, OperatorDelete, 0x005EEAB0, void* p);         // the game's ::operator delete
GAME_FN(void*, __thiscall, DataStream_GetBuffer, 0x00856850, const void* stream);   // (+0x8: a folded getter)
// this file's (called where modes apply)
GAME_FN(void, __thiscall, Primitive_Destruct, 0x008B76F0, void* self);
GAME_FN(void, __thiscall, GeometryData_ReleaseInputLayouts, 0x008B8060, void* self);
GAME_FN(void, __thiscall, GeometryData_Destruct, 0x008B8100, void* self);
GAME_FN(uint32_t, __thiscall, DataStream_GetUsageFlags, 0x008B8290, const void* self);
GAME_FN(uint32_t, __cdecl, DataStream_GetD3D10Usage, 0x008B82A0, uint32_t access, uint32_t* usage);
GAME_FN(void, __thiscall, DataStream_ConvertD3D10Flags, 0x008B82F0, void* self, uint32_t usage, uint32_t bind,
        uint32_t cpuAccess, uint32_t* usageFlags, uint32_t* accessFlags);
GAME_FN(void, __thiscall, DataStream_AllocateBuffer, 0x008B8390, void* self, uint32_t size, uint32_t usage,
        uint32_t bind, uint32_t cpuAccess, uint32_t misc, const void* initialData);
GAME_FN(void*, __thiscall, DataStream_Construct, 0x008B8470, void* self, uint32_t size, uint32_t usage, uint32_t bind,
        uint32_t cpuAccess, uint32_t misc, const void* initialData);
GAME_FN(uint8_t, __cdecl, DataStream_CreateFull, 0x008B85C0, uint32_t size, uint32_t usage, uint32_t bind,
        uint32_t cpuAccess, uint32_t misc, const void* initialData, re::Ptr32<void>* result);
GAME_FN(void, __thiscall, DataStream_Unlock, 0x008B8760, void* self);
GAME_FN(void, __thiscall, DataStream_Destruct, 0x008B8930, void* self);

constexpr int kPrimitiveSize = 0x24;

inline uint32_t SetCount(const uint8_t* self) {
    return At<uint32_t>(self, 4);
}
inline uint32_t PrimitiveCount(const uint8_t* self) {
    return At<uint32_t*>(self, 8)[SetCount(self)];
}
inline uint8_t* PrimitiveAt(const uint8_t* self, uint32_t index) {
    return At<uint8_t*>(self, 0x3C) + index * kPrimitiveSize;
}
inline void InvalidPrimitive() {
    D3D10Warning("Attempting to access invalid primitive in NiD3D10GeometryBuffer!");
}

// a primitive's field read / written when the index is in range (warned about otherwise)
inline uint32_t GetPrimitiveField(const uint8_t* self, uint32_t index, int offset) {
    if (index < PrimitiveCount(self)) {
        return At<uint32_t>(PrimitiveAt(self, index), offset);
    }
    InvalidPrimitive();
    return 0;
}
inline void SetPrimitiveField(uint8_t* self, uint32_t index, int offset, uint32_t value) {
    if (index < PrimitiveCount(self)) {
        At<uint32_t>(PrimitiveAt(self, index), offset) = value;
        return;
    }
    InvalidPrimitive();
}

// Gamebryo's usage flags made D3D10 bind flags and back (1 <-> 2 swapped, 4 kept)
inline uint32_t UsageToBind(uint32_t usage) {
    uint32_t bind = 0;
    if (usage & 1) {
        bind = 2;
    }
    if (usage & 2) {
        bind |= 1;
    }
    if (usage & 4) {
        bind |= 4;
    }
    return bind;
}

// an NiPointer array (new[]'d with its count) let go and freed, its elements in reverse
void DeleteStreamArray(re::Ptr32<void>* streams) {
    if (streams == nullptr) {
        return;
    }
    int32_t count = int32_t(reinterpret_cast<uint32_t*>(streams)[-1]);
    for (int32_t i = count - 1; i >= 0; i--) {
        ReleaseRef(streams[i]);
    }
    NiDeleteArray(reinterpret_cast<uint32_t*>(streams) - 1, 4);
}

// the vertex buffers let go (their D3D buffers and streams), as the destructor and the vertex
// buffer count's change do
void ReleaseVertexBuffers(uint8_t* self) {
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x2C); i++) {
        re::Ptr32<void>& buffer = At<re::Ptr32<void>*>(self, 0x34)[i];
        if (buffer != nullptr) {
            Com<long>(buffer, kRelease);
        }
        re::Ptr32<void>& stream = At<re::Ptr32<void>*>(self, 0x30)[i];
        if (stream != nullptr) {
            ReleaseRef(stream);
            stream = nullptr;
        }
    }
}

// new[] with a count word as the game's operator new[] for these arrays does (::operator new(4)
// for none)
uint32_t* NewCountedArray(uint32_t count, uint32_t elementSize) {
    if (count == 0) {
        return static_cast<uint32_t*>(OperatorNew(4));
    }
    uint64_t bytes = uint64_t(count) * elementSize;
    uint32_t size = (bytes >> 32) != 0 ? 0xFFFFFFFFu : uint32_t(bytes);
    size = size > 0xFFFFFFFBu ? 0xFFFFFFFFu : size + 4;
    return static_cast<uint32_t*>(NiNewArray(size));
}

// the common part of the constructors (all empty, the layout map made)
void ConstructGeometryData(uint8_t* self, uint32_t sets, uint32_t value40) {
    At<uint32_t>(self, 0) = kGeometryDataVtable;
    At<uint32_t>(self, 4) = sets;
    At<void*>(self, 8) = nullptr;
    At<uint32_t>(self, 0xC) = 0;
    At<void*>(self, 0x10) = nullptr;
    At<uint32_t>(self, 0x14) = 0;
    At<void*>(self, 0x18) = nullptr;
    At<uint32_t>(self, 0x1C) = kMapBaseVtable;
    At<uint32_t>(self, 0x20) = 7;
    At<uint32_t>(self, 0x28) = 0;
    void* buckets = NiMalloc(0x1C);
    At<void*>(self, 0x24) = buckets;
    crt::Memset(buckets, 0, At<uint32_t>(self, 0x20) * 4);
    At<uint32_t>(self, 0x1C) = kLayoutMapVtable;
    At<uint32_t>(self, 0x2C) = 0;
    At<void*>(self, 0x30) = nullptr;
    At<void*>(self, 0x34) = nullptr;
    At<void*>(self, 0x38) = nullptr;
    At<void*>(self, 0x3C) = nullptr;
    At<uint32_t>(self, 0x40) = value40;
    At<void*>(self, 0x44) = nullptr;
    At<uint32_t>(self, 0x48) = 0;
}

} // namespace

// ---- NiD3D10GeometryData

// 0x8B72C0 (thiscall): a set's first primitive (0 past the sets)
uint32_t __fastcall GeometryData_GetSetOffset(uint8_t* self, re::Edx, uint32_t set) {
    if (set < SetCount(self)) {
        return At<uint32_t*>(self, 8)[set];
    }
    return 0;
}

// 0x8B72E0 (thiscall): a set's primitive count
uint32_t __fastcall GeometryData_GetSetPrimitiveCount(uint8_t* self, re::Edx, uint32_t set) {
    if (set < SetCount(self)) {
        uint32_t* offsets = At<uint32_t*>(self, 8) + set;
        return offsets[1] - offsets[0];
    }
    return 0;
}

// 0x8B7300 (thiscall): the primitive count (all sets')
uint32_t __fastcall GeometryData_GetPrimitiveCount(uint8_t* self, re::Edx) {
    return PrimitiveCount(self);
}

// 0x8B7310 / 0x8B7340 / 0x8B7370 / 0x8B73A0 / 0x8B73D0 / 0x8B7400 / 0x8B7430 / 0x8B7880 (thiscall):
// a primitive's fields (+0x8 its index ID3D10Buffer, +0xC its index format, +0x10 the index offset,
// +0x14 its vertex buffer offsets, +0x18 the index count, +0x1C the start index, +0x20 the base
// vertex, +0x4 its index stream)
uint32_t __fastcall GeometryData_GetIndexBuffer(uint8_t* self, re::Edx, uint32_t index) {
    return GetPrimitiveField(self, index, 8);
}
uint32_t __fastcall GeometryData_GetIndexFormat(uint8_t* self, re::Edx, uint32_t index) {
    return GetPrimitiveField(self, index, 0xC);
}
uint32_t __fastcall GeometryData_GetIndexOffset(uint8_t* self, re::Edx, uint32_t index) {
    return GetPrimitiveField(self, index, 0x10);
}
uint32_t __fastcall GeometryData_GetVBOffsets(uint8_t* self, re::Edx, uint32_t index) {
    return GetPrimitiveField(self, index, 0x14);
}
uint32_t __fastcall GeometryData_GetIndexCount(uint8_t* self, re::Edx, uint32_t index) {
    return GetPrimitiveField(self, index, 0x18);
}
uint32_t __fastcall GeometryData_GetStartIndex(uint8_t* self, re::Edx, uint32_t index) {
    return GetPrimitiveField(self, index, 0x1C);
}
uint32_t __fastcall GeometryData_GetBaseVertex(uint8_t* self, re::Edx, uint32_t index) {
    return GetPrimitiveField(self, index, 0x20);
}
uint32_t __fastcall GeometryData_GetIndexDataStream(uint8_t* self, re::Edx, uint32_t index) {
    return GetPrimitiveField(self, index, 4);
}

// 0x8B7460 (thiscall, 4 bytes: under a hook's size): the flags (+0x48)
uint32_t __fastcall GeometryData_GetFlags(uint8_t* self, re::Edx) {
    return At<uint32_t>(self, 0x48);
}
RE_UNHOOKED_FN("NiD3D10GeometryData::GetFlags", 0x008B7460, &GeometryData_GetFlags);

// 0x8B7470 (thiscall): SetVBStrides(strides, count): one per vertex buffer (0 without an array)
void __fastcall GeometryData_SetVBStrides(uint8_t* self, re::Edx, const uint32_t* strides, uint32_t count) {
    uint32_t expected = At<uint32_t>(self, 0x2C);
    if (count != expected) {
        D3D10Warning("Attempting to set %d vertex buffer strides in a NiD3D10GeometryData object that expects %d "
                     "vertex buffers.",
                     count, expected);
        return;
    }
    for (uint32_t i = 0; i < count; i++) {
        At<uint32_t*>(self, 0x38)[i] = strides == nullptr ? 0 : strides[i];
    }
}

// 0x8B74C0 / 0x8B74F0 / 0x8B7590 / 0x8B75C0 / 0x8B75F0 (thiscall): a primitive's index format,
// index offset, index count, start index, base vertex set
void __fastcall GeometryData_SetIndexFormat(uint8_t* self, re::Edx, uint32_t index, uint32_t value) {
    SetPrimitiveField(self, index, 0xC, value);
}
void __fastcall GeometryData_SetIndexOffset(uint8_t* self, re::Edx, uint32_t index, uint32_t value) {
    SetPrimitiveField(self, index, 0x10, value);
}
void __fastcall GeometryData_SetIndexCount(uint8_t* self, re::Edx, uint32_t index, uint32_t value) {
    SetPrimitiveField(self, index, 0x18, value);
}
void __fastcall GeometryData_SetStartIndex(uint8_t* self, re::Edx, uint32_t index, uint32_t value) {
    SetPrimitiveField(self, index, 0x1C, value);
}
void __fastcall GeometryData_SetBaseVertex(uint8_t* self, re::Edx, uint32_t index, uint32_t value) {
    SetPrimitiveField(self, index, 0x20, value);
}

// 0x8B7520 (thiscall): SetVBOffsets(primitive, offsets, count): one per vertex buffer
void __fastcall GeometryData_SetVBOffsets(uint8_t* self, re::Edx, uint32_t index, const uint32_t* offsets,
                                          uint32_t count) {
    uint32_t expected = At<uint32_t>(self, 0x2C);
    if (count != expected) {
        D3D10Warning("Attempting to set %d VB offsets in a NiD3D10GeometryData object that expects %d VB offsets",
                     count, expected);
        return;
    }
    if (index >= PrimitiveCount(self)) {
        InvalidPrimitive();
        return;
    }
    uint8_t* primitive = PrimitiveAt(self, index);
    for (uint32_t i = 0; i < count; i++) {
        At<uint32_t*>(primitive, 0x14)[i] = offsets[i];
    }
}

// 0x8B7620 (thiscall): the flags set
void __fastcall GeometryData_SetFlags(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0x48) = flags;
}

// 0x8B7630 (cdecl): the flags made (0x400000, 0x800000, 0x200000 by the three bools, the count in
// the top byte)
uint32_t __cdecl GeometryData_MakeFlags(int flag400000, int flag800000, int flag200000, uint32_t count) {
    uint32_t flags = re::Flag(flag400000) ? 0x400000u : 0u;
    if (re::Flag(flag800000)) {
        flags |= 0x800000;
    }
    if (re::Flag(flag200000)) {
        flags |= 0x200000;
    }
    return count << 24 | flags;
}

// 0x8B7670 (cdecl): a flags word's 0x400000
uint8_t __cdecl GeometryData_FlagsHave400000(uint32_t flags) {
    return uint8_t((flags >> 0x16) & 1);
}

// 0x8B7680 (thiscall, 4 bytes: under a hook's size): +0x40
uint32_t __fastcall GeometryData_Get40(uint8_t* self, re::Edx) {
    return At<uint32_t>(self, 0x40);
}
RE_UNHOOKED_FN("NiD3D10GeometryData::Get40", 0x008B7680, &GeometryData_Get40);

// 0x8B7690 (thiscall): the object at +0x44 replaced (the old one deleted, slot 0)
void __fastcall GeometryData_Set44(uint8_t* self, re::Edx, void* object) {
    void* old = At<void*>(self, 0x44);
    if (old != nullptr) {
        Virtual<void*>(old, 0, 1u);
    }
    At<void*>(self, 0x44) = object;
}

// 0x8B76C0 (thiscall): a primitive's constructor (all zero)
uint8_t* __fastcall Primitive_Construct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kPrimitiveVtable;
    for (int offset = 4; offset < kPrimitiveSize; offset += 4) {
        At<uint32_t>(self, offset) = 0;
    }
    return self;
}

// 0x8B76F0 (thiscall): a primitive's destructor (its index buffer and stream let go, its offsets
// freed)
void __fastcall Primitive_DestructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kPrimitiveVtable;
    ComRelease(At<void*>(self, 8));
    NiFree(At<void*>(self, 0x14));
    ReleaseRef(At<void*>(self, 4));
}

// 0x8B7740 (thiscall, slot 0 of 0x9A0D3C): a primitive's vector deleting destructor
void* __fastcall Primitive_VectorDeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    if (flags & 2) {
        uint32_t* header = reinterpret_cast<uint32_t*>(self) - 1;
        for (int32_t i = int32_t(*header) - 1; i >= 0; i--) {
            uint8_t* primitive = self + i * kPrimitiveSize;
            At<uint32_t>(primitive, 0) = kPrimitiveVtable;
            ComRelease(At<void*>(primitive, 8));
            NiFree(At<void*>(primitive, 0x14));
            ReleaseRef(At<void*>(primitive, 4));
        }
        if (flags & 1) {
            NiDeleteArray(header, kPrimitiveSize);
        }
        return header;
    }
    Primitive_Destruct(self);
    if (flags & 1) {
        NiDelete(self, kPrimitiveSize);
    }
    return self;
}

// 0x8B77E0 (thiscall, slot 1): ContainsVertexData(parameter) by the flags (3 normal 0x800000, 4
// colour 0x400000, 5..12 a texture set below the count, 13 / 14 tangent / binormal 0x200000)
uint8_t __fastcall GeometryData_ContainsVertexData(uint8_t* self, re::Edx, uint32_t parameter) {
    switch (parameter) {
    case 3:
        return uint8_t((At<uint32_t>(self, 0x48) >> 0x17) & 1);
    case 4:
        return uint8_t((At<uint32_t>(self, 0x48) >> 0x16) & 1);
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
        return parameter - 5 < At<uint8_t>(self, 0x4B) ? 1 : 0;
    case 13:
    case 14:
        return uint8_t((At<uint32_t>(self, 0x48) >> 0x15) & 1);
    default:
        return 0;
    }
}

// 0x8B7850 (thiscall): a vertex buffer's NiD3D10DataStream (warned about past the count)
void* __fastcall GeometryData_GetVBDataStream(uint8_t* self, re::Edx, uint32_t index) {
    if (index < At<uint32_t>(self, 0x2C)) {
        return At<re::Ptr32<void>*>(self, 0x30)[index];
    }
    D3D10Warning("Attempting to access invalid VB in NiD3D10GeometryBuffer!");
    return nullptr;
}

// 0x8B78B0 (thiscall): SetInputElements(descriptions, count): copied (the array freed when the
// count changes; nothing kept for none)
void __fastcall GeometryData_SetInputElements(uint8_t* self, re::Edx, const uint32_t* elements, uint32_t count) {
    if (count != At<uint32_t>(self, 0x14)) {
        NiFree(At<void*>(self, 0x10));
        At<void*>(self, 0x10) = nullptr;
        At<uint32_t>(self, 0x14) = 0;
    }
    if (elements == nullptr || count == 0) {
        return;
    }
    if (At<void*>(self, 0x10) == nullptr) {
        At<void*>(self, 0x10) = NiMalloc(count * 0x1C);
    }
    for (uint32_t i = 0; i < count; i++) {
        crt::Memcpy(At<uint8_t*>(self, 0x10) + i * 0x1C, reinterpret_cast<const uint8_t*>(elements) + i * 0x1C, 0x1C);
    }
    At<uint32_t>(self, 0x14) = count;
}

// 0x8B7930 (thiscall): SetVertexBuffer(index, stream): a stream with a buffer and the vertex usage;
// kept and its buffer held
void __fastcall GeometryData_SetVertexBuffer(uint8_t* self, re::Edx, uint32_t index, void* stream) {
    uint32_t count = At<uint32_t>(self, 0x2C);
    if (index >= count) {
        D3D10Warning("Attempting to set vertex buffer index %d in a NiD3D10GeometryData object that expects %d "
                     "vertex buffers.",
                     index, count);
        return;
    }
    if (stream != nullptr) {
        if (DataStream_GetBuffer(stream) == nullptr) {
            D3D10Warning("Attempting to set an NiD3D10DataStream with no buffer as a vertex buffer on an "
                         "NiD3D10GeometryData object.");
            return;
        }
        if ((DataStream_GetUsageFlags(stream) & 2) == 0) {
            D3D10Warning("Attempting to set a non-vertex buffer NiD3D10DataStream object as a vertex buffer on an "
                         "NiD3D10GeometryData object.");
            return;
        }
    }
    re::Ptr32<void>* streams = At<re::Ptr32<void>*>(self, 0x30);
    if (streams[index] == stream) {
        return;
    }
    NiPointer_Assign(&streams[index], stream);
    re::Ptr32<void>* buffers = At<re::Ptr32<void>*>(self, 0x34);
    if (buffers[index] != nullptr) {
        Com<long>(buffers[index], kRelease);
    }
    if (stream != nullptr) {
        At<re::Ptr32<void>*>(self, 0x34)[index] = DataStream_GetBuffer(stream);
        Com<long>(DataStream_GetBuffer(stream), kAddRef);
        return;
    }
    At<re::Ptr32<void>*>(self, 0x34)[index] = nullptr;
}

// 0x8B7A00 (thiscall): SetIndexBuffer(primitive, stream): a stream with a buffer and the index
// usage; kept and its buffer held
void __fastcall GeometryData_SetIndexBuffer(uint8_t* self, re::Edx, uint32_t index, void* stream) {
    if (index >= PrimitiveCount(self)) {
        InvalidPrimitive();
        return;
    }
    uint8_t* primitive = PrimitiveAt(self, index);
    if (stream != nullptr) {
        if (DataStream_GetBuffer(stream) == nullptr) {
            D3D10Warning("Attempting to set an NiD3D10DataStream with no buffer as a index buffer on an "
                         "NiD3D10GeometryData object.");
            return;
        }
        if ((DataStream_GetUsageFlags(stream) & 1) == 0) {
            D3D10Warning("Attempting to set a non-index buffer NiD3D10DataStream object as a index buffer on an "
                         "NiD3D10GeometryData object.");
            return;
        }
    }
    if (At<void*>(primitive, 4) == stream) {
        return;
    }
    NiPointer_Assign(primitive + 4, stream);
    if (At<void*>(primitive, 8) != nullptr) {
        Com<long>(At<void*>(primitive, 8), kRelease);
    }
    if (stream != nullptr) {
        At<void*>(primitive, 8) = DataStream_GetBuffer(stream);
        Com<long>(DataStream_GetBuffer(stream), kAddRef);
        return;
    }
    At<void*>(primitive, 8) = nullptr;
}

// 0x8B7AC0 (thiscall): the vertex buffers' D3D buffers brought up to date with their streams'
void __fastcall GeometryData_UpdateVertexBuffers(uint8_t* self, re::Edx) {
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x2C); i++) {
        void* stream = At<re::Ptr32<void>*>(self, 0x30)[i];
        if (stream == nullptr) {
            continue;
        }
        void* buffer = DataStream_GetBuffer(stream);
        void* old = At<re::Ptr32<void>*>(self, 0x34)[i];
        if (buffer == old) {
            continue;
        }
        if (old != nullptr) {
            Com<long>(At<re::Ptr32<void>*>(self, 0x34)[i], kRelease);
        }
        At<re::Ptr32<void>*>(self, 0x34)[i] = buffer;
        if (At<re::Ptr32<void>*>(self, 0x34)[i] != nullptr) {
            Com<long>(At<re::Ptr32<void>*>(self, 0x34)[i], kAddRef);
        }
    }
}

// 0x8B7B30 (thiscall): the primitives' index buffers brought up to date with their streams'
void __fastcall GeometryData_UpdateIndexBuffers(uint8_t* self, re::Edx) {
    for (uint32_t i = 0; i < PrimitiveCount(self); i++) {
        uint8_t* primitive = PrimitiveAt(self, i);
        void* stream = At<void*>(primitive, 4);
        if (stream == nullptr) {
            continue;
        }
        void* buffer = DataStream_GetBuffer(stream);
        void* old = At<void*>(primitive, 8);
        if (buffer == old) {
            continue;
        }
        ComRelease(old);
        At<void*>(primitive, 8) = buffer;
        ComAddRef(buffer);
    }
}

// 0x8B7BB0 (thiscall): NiD3D10GeometryData::NiD3D10GeometryData(primitives): one set
uint8_t* __fastcall GeometryData_ConstructSingle(uint8_t* self, re::Edx, uint32_t primitives) {
    ConstructGeometryData(self, 1, 0);
    uint32_t* offsets = static_cast<uint32_t*>(NiMalloc(8));
    At<uint32_t*>(self, 8) = offsets;
    offsets[0] = 0;
    At<uint32_t*>(self, 8)[1] = primitives;
    uint32_t* memory = NewCountedArray(primitives, kPrimitiveSize);
    uint8_t* array = nullptr;
    if (memory != nullptr) {
        array = reinterpret_cast<uint8_t*>(memory + 1);
        *memory = primitives;
        VectorConstructor(array, kPrimitiveSize, primitives, kPrimitiveConstructor);
    }
    At<uint8_t*>(self, 0x3C) = array;
    return self;
}

// 0x8B7C90 (thiscall): NiD3D10GeometryData::NiD3D10GeometryData(value, sets, primitives per set)
uint8_t* __fastcall GeometryData_ConstructSets(uint8_t* self, re::Edx, uint32_t value40, uint32_t sets,
                                               const uint32_t* counts) {
    ConstructGeometryData(self, sets, value40);
    uint32_t* offsets = static_cast<uint32_t*>(NiMalloc(sets * 4 + 4));
    At<uint32_t*>(self, 8) = offsets;
    offsets[0] = 0;
    for (uint32_t i = 0; i < sets; i++) {
        uint32_t* running = At<uint32_t*>(self, 8);
        running[i + 1] = counts[i] + running[i];
    }
    uint32_t total = PrimitiveCount(self);
    uint32_t* memory = NewCountedArray(total, kPrimitiveSize);
    if (memory == nullptr) {
        At<void*>(self, 0x3C) = nullptr;
        return self;
    }
    *memory = total;
    uint8_t* array = reinterpret_cast<uint8_t*>(memory + 1);
    uint8_t* primitive = array;
    for (int32_t i = int32_t(total) - 1; i >= 0; i--) {
        At<uint32_t>(primitive, 0) = kPrimitiveVtable;
        for (int offset = 4; offset < kPrimitiveSize; offset += 4) {
            At<uint32_t>(primitive, offset) = 0;
        }
        primitive += kPrimitiveSize;
    }
    At<uint8_t*>(self, 0x3C) = array;
    return self;
}

// 0x8B7DD0 (thiscall): the input layout for a shader signature (bytecode, length): the one made
// for it before, or one made now (with the input elements) and kept; chosen (+0x18)
void __fastcall GeometryData_UpdateInputLayout(uint8_t* self, re::Edx, void* signature, uint32_t length) {
    uint8_t* map = self + 0x1C;
    void* layout = nullptr;
    for (re::Ptr32<void>* item = At<re::Ptr32<re::Ptr32<void>>*>(map, 8)[uintptr_t(signature) % At<uint32_t>(map, 4)]; item != nullptr;
         item = static_cast<re::Ptr32<void>*>(item[0])) {
        if (signature == item[1]) {
            layout = item[2];
            if (layout != nullptr) {
                At<void*>(self, 0x18) = layout;
                return;
            }
            break;
        }
    }
    void* device = D3D10Renderer_GetD3DDevice(D3D10Renderer_GetRenderer());
    long hr = Com<long>(device, kCreateInputLayout, At<void*>(self, 0x10), At<uint32_t>(self, 0x14), signature, length,
                        &layout);
    if (hr < 0) {
        D3D10Error(0xF, "Error HRESULT = 0x%08X.", hr);
    } else if (layout == nullptr) {
        D3D10Error(0xF, "No error message from D3D10, but input layout is NULL.");
    } else {
        LayoutMap_SetAt(map, signature, layout);
        At<void*>(self, 0x18) = layout;
        return;
    }
    if (layout != nullptr) {
        Com<long>(layout, kRelease);
    }
}

// 0x8B7EA0 (thiscall): SetVBCount(count): the vertex buffers let go and their arrays freed (and the
// primitives' offsets); new zeroed ones for the count. (With none, the freed arrays' pointers are
// left, as in the game.)
void __fastcall GeometryData_SetVBCount(uint8_t* self, re::Edx, uint32_t count) {
    ReleaseVertexBuffers(self);
    NiFree(At<void*>(self, 0x34));
    NiFree(At<void*>(self, 0x38));
    DeleteStreamArray(At<re::Ptr32<void>*>(self, 0x30));
    for (uint32_t i = 0; i < PrimitiveCount(self); i++) {
        NiFree(At<void*>(PrimitiveAt(self, i), 0x14));
    }
    At<uint32_t>(self, 0x2C) = count;
    if (count == 0) {
        return;
    }
    uint32_t bytes = count * 4;
    void* buffers = NiMalloc(bytes);
    At<void*>(self, 0x34) = buffers;
    crt::Memset(buffers, 0, bytes);
    void* strides = NiMalloc(bytes);
    At<void*>(self, 0x38) = strides;
    crt::Memset(strides, 0, bytes);
    uint64_t size64 = uint64_t(count) * 4;
    uint32_t size = (size64 >> 32) != 0 ? 0xFFFFFFFFu : uint32_t(size64);
    size = size > 0xFFFFFFFBu ? 0xFFFFFFFFu : size + 4;
    uint32_t* memory = static_cast<uint32_t*>(NiNewArray(size));
    re::Ptr32<void>* streams = nullptr;
    if (memory != nullptr) {
        *memory = count;
        streams = reinterpret_cast<re::Ptr32<void>*>(memory + 1);
        for (uint32_t i = 0; i < count; i++) {
            streams[i] = nullptr;
        }
    }
    At<re::Ptr32<void>*>(self, 0x30) = streams;
    for (uint32_t i = 0; i < PrimitiveCount(self); i++) {
        void* offsets = NiMalloc(bytes);
        At<void*>(PrimitiveAt(self, i), 0x14) = offsets;
        crt::Memset(offsets, 0, bytes);
    }
}

// 0x8B8060 (thiscall): the input layouts let go (each map value released, the map emptied) and none
// chosen
void __fastcall GeometryData_ReleaseInputLayoutsImpl(uint8_t* self, re::Edx) {
    uint8_t* map = self + 0x1C;
    re::Ptr32<void>* item = nullptr;
    for (uint32_t i = 0; i < At<uint32_t>(map, 4); i++) {
        if (At<re::Ptr32<void>*>(map, 8)[i] != nullptr) {
            item = static_cast<re::Ptr32<void>*>(At<re::Ptr32<void>*>(map, 8)[i]);
            break;
        }
    }
    while (item != nullptr) {
        re::Ptr32<void>* next = static_cast<re::Ptr32<void>*>(item[0]);
        void* layout = item[2];
        if (next == nullptr) {
            uint32_t size = At<uint32_t>(map, 4);
            for (uint32_t bucket = uint32_t(uintptr_t(item[1]) % size) + 1; bucket < size; bucket++) {
                if (At<re::Ptr32<void>*>(map, 8)[bucket] != nullptr) {
                    next = static_cast<re::Ptr32<void>*>(At<re::Ptr32<void>*>(map, 8)[bucket]);
                    break;
                }
            }
        }
        item = next;
        if (layout != nullptr) {
            Com<long>(layout, kRelease);
        }
    }
    Map_RemoveAll(map);
    At<void*>(self, 0x18) = nullptr;
}

// 0x8B8100 (thiscall): NiD3D10GeometryData::~NiD3D10GeometryData
void __fastcall GeometryData_DestructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kGeometryDataVtable;
    GeometryData_ReleaseInputLayouts(self);
    NiFree(At<void*>(self, 0x10));
    At<void*>(self, 0x10) = nullptr;
    At<uint32_t>(self, 0x14) = 0;
    ReleaseVertexBuffers(self);
    NiFree(At<void*>(self, 8));
    DeleteStreamArray(At<re::Ptr32<void>*>(self, 0x30));
    NiFree(At<void*>(self, 0x34));
    NiFree(At<void*>(self, 0x38));
    uint8_t* primitives = At<uint8_t*>(self, 0x3C);
    if (primitives != nullptr) {
        if (reinterpret_cast<uint32_t*>(primitives)[-1] == 0) {
            OperatorDelete(reinterpret_cast<uint32_t*>(primitives) - 1);
        } else {
            Virtual<void*>(primitives, 0, 3u);
        }
    }
    At<void*>(self, 8) = nullptr;
    At<void*>(self, 0x30) = nullptr;
    At<void*>(self, 0x34) = nullptr;
    At<void*>(self, 0x38) = nullptr;
    At<void*>(self, 0x3C) = nullptr;
    void* owned = At<void*>(self, 0x44);
    if (owned != nullptr) {
        Virtual<void*>(owned, 0, 1u);
    }
    uint8_t* map = self + 0x1C;
    At<uint32_t>(map, 0) = kLayoutMapVtable;
    Map_RemoveAll(map);
    At<uint32_t>(map, 0) = kMapBaseVtable;
    Map_RemoveAll(map);
    NiFree(At<void*>(map, 8));
    At<uint32_t>(self, 0) = kGeometryBufferDataVtable;
}

// 0x8B8270 (thiscall, slot 0): NiD3D10GeometryData's deleting destructor
uint8_t* __fastcall GeometryData_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    GeometryData_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x4C);
    }
    return self;
}

// ---- NiD3D10DataStream

// 0x8B8290 (thiscall): the usage flags (a staging stand-in's: the ones it stands in for)
uint32_t __fastcall DataStream_GetUsageFlagsImpl(uint8_t* self, re::Edx) {
    if (At<uint32_t>(self, 0x1C) == 3) {
        return At<uint32_t>(self, 0x24);
    }
    return At<uint32_t>(self, 0x14);
}

// 0x8B82A0 (cdecl): the D3D10 usage for Gamebryo access flags (0x10 dynamic, exactly 4 immutable,
// 4 default, else staging); the access flags that usage really gives
uint32_t __cdecl DataStream_GetD3D10UsageImpl(uint32_t access, uint32_t* usage) {
    if (access & 0x10) {
        *usage = 2;
        return 0x16;
    }
    if (access == 4) {
        *usage = 1;
        return 4;
    }
    if (access & 4) {
        *usage = 0;
        return 0xE;
    }
    *usage = 3;
    return 3;
}

// 0x8B82F0 (thiscall, `this` unused): D3D10's usage, bind and CPU access flags made Gamebryo's
// usage and access flags
void __fastcall DataStream_ConvertD3D10FlagsImpl(void*, re::Edx, uint32_t usage, int bind, uint32_t cpuAccess,
                                                 uint32_t* usageFlags, uint32_t* accessFlags) {
    uint8_t b = uint8_t(bind);
    *usageFlags = 0;
    if (b & 2) {
        *usageFlags = 1;
    }
    if (b & 1) {
        *usageFlags |= 2;
    }
    if (b & 4) {
        *usageFlags |= 4;
    }
    if (usage == 2) {
        *accessFlags = 0x16;
        return;
    }
    if (usage == 1) {
        *accessFlags = 4;
        return;
    }
    if (usage == 0) {
        *accessFlags = 0xE;
        return;
    }
    *accessFlags = 0;
    if (cpuAccess & 0x10000) {
        *accessFlags = 2;
    }
    if (cpuAccess & 0x20000) {
        *accessFlags |= 1;
    }
}

// 0x8B8390 (thiscall): NiD3D10DataStream::AllocateBuffer(size, usage, bind, CPU access, misc,
// initial data): an immutable buffer without data made a staging one (its bind flags kept as
// usage flags to stand in for); the buffer made by the resource manager and the flags kept; all
// cleared and warned about when it can't be
void __fastcall DataStream_AllocateBufferImpl(uint8_t* self, re::Edx, uint32_t size, uint32_t usage, uint32_t bind,
                                              uint32_t cpuAccess, uint32_t misc, const void* initialData) {
    void* manager = D3D10Renderer_GetResourceManager(D3D10Renderer_GetRenderer());
    uint32_t standIn = 0;
    uint32_t bindFlags = bind;
    if (usage == 1 && initialData == nullptr) {
        uint8_t b = uint8_t(bind);
        cpuAccess = 0x30000;
        if (b & 2) {
            standIn = 1;
        }
        if (b & 1) {
            standIn |= 2;
        }
        if (b & 4) {
            standIn |= 4;
        }
        bindFlags = 0;
        usage = 3;
    }
    void* buffer = ResourceManager_CreateBuffer(manager, size, bindFlags, usage, cpuAccess, misc, initialData);
    At<void*>(self, 8) = buffer;
    if (buffer != nullptr) {
        At<uint32_t>(self, 0xC) = size;
        At<uint32_t>(self, 0x24) = standIn;
        DataStream_ConvertD3D10Flags(self, usage, bindFlags, cpuAccess, &At<uint32_t>(self, 0x14),
                                     &At<uint32_t>(self, 0x10));
        At<uint32_t>(self, 0x1C) = usage;
        At<uint32_t>(self, 0x18) = misc;
        return;
    }
    D3D10Warning("NiD3D10DataStream::AllocateBuffer failed because buffer could not be created.");
    At<uint32_t>(self, 0xC) = 0;
    At<uint32_t>(self, 0x10) = 0;
    At<uint32_t>(self, 0x14) = 0;
    At<uint32_t>(self, 0x18) = 0;
    At<uint32_t>(self, 0x1C) = 0;
}

// 0x8B8470 (thiscall): NiD3D10DataStream::NiD3D10DataStream(size, usage, bind, CPU access, misc,
// initial data)
uint8_t* __fastcall DataStream_ConstructImpl(uint8_t* self, re::Edx, uint32_t size, uint32_t usage, uint32_t bind,
                                             uint32_t cpuAccess, uint32_t misc, const void* initialData) {
    At<uint32_t>(self, 0) = kRefObjectVtable;
    At<uint32_t>(self, 4) = 0;
    Increment(&ms_uiObjects);
    At<uint32_t>(self, 0) = kDataStreamVtable;
    At<void*>(self, 8) = nullptr;
    At<uint32_t>(self, 0xC) = 0;
    At<uint32_t>(self, 0x10) = 0;
    At<uint32_t>(self, 0x14) = 0;
    At<uint32_t>(self, 0x18) = 0;
    At<uint32_t>(self, 0x1C) = 0;
    At<uint8_t>(self, 0x20) = 0;
    At<uint32_t>(self, 0x24) = 0;
    DataStream_AllocateBuffer(self, size, usage, bind, cpuAccess, misc, initialData);
    return self;
}

// a stream made and assigned to an NiPointer; whether it has a buffer
uint8_t AssignNewStream(re::Ptr32<void>* result, uint32_t size, uint32_t usage, uint32_t bind, uint32_t cpuAccess,
                        uint32_t misc, const void* initialData) {
    void* memory = NiNew(0x28);
    void* stream = memory != nullptr ? DataStream_Construct(memory, size, usage, bind, cpuAccess, misc, initialData)
                                     : nullptr;
    void* old = *result;
    if (old != stream) {
        ReleaseRef(old);
        *result = stream;
        if (stream == nullptr) {
            return 0;
        }
        Increment(&At<uint32_t>(stream, 4));
    }
    return *result != nullptr && At<void*>(*result, 8) != nullptr ? 1 : 0;
}

// 0x8B84E0 (cdecl): NiD3D10DataStream::Create(size, access flags, usage flags, NiPointer& result):
// D3D10's usage, bind and CPU access from Gamebryo's (staging: no bind, read and write; dynamic:
// write); whether it got a buffer. (A size of 0: none, false.)
uint8_t __cdecl DataStream_Create(uint32_t size, uint32_t access, int usageFlags, re::Ptr32<void>* result) {
    if (size == 0) {
        return 0;
    }
    uint32_t usage = 0;
    DataStream_GetD3D10Usage(access, &usage);
    uint32_t bind = UsageToBind(uint8_t(usageFlags));
    uint32_t cpuAccess = 0;
    if (usage == 3) {
        bind = 0;
        cpuAccess = 0x30000;
    } else if (usage == 2) {
        cpuAccess = 0x10000;
    }
    return AssignNewStream(result, size, usage, bind, cpuAccess, 0, nullptr);
}

// 0x8B85C0 (cdecl): NiD3D10DataStream::Create(size, usage, bind, CPU access, misc, initial data,
// NiPointer& result)
uint8_t __cdecl DataStream_CreateFullImpl(uint32_t size, uint32_t usage, uint32_t bind, uint32_t cpuAccess,
                                          uint32_t misc, const void* initialData, re::Ptr32<void>* result) {
    if (size == 0) {
        return 0;
    }
    return AssignNewStream(result, size, usage, bind, cpuAccess, misc, initialData);
}

// 0x8B8660 (thiscall, slot 2): NiD3D10DataStream::Lock(flags: 1 read, 2 write; as its access
// allows; not when locked): dynamic buffers mapped write-discard, staging ones read / write /
// both; a default one through a new staging stream (kept at +0x24, locked in its place). The data,
// or 0.
void* __fastcall DataStream_Lock(uint8_t* self, re::Edx, int flags) {
    void* buffer = At<void*>(self, 8);
    if (buffer == nullptr) {
        return nullptr;
    }
    uint8_t f = uint8_t(flags);
    if (f == 0) {
        return nullptr;
    }
    bool write = false;
    if (f & 2) {
        if ((At<uint8_t>(self, 0x10) & 2) == 0) {
            return nullptr;
        }
        write = true;
    }
    bool read = false;
    if (f & 1) {
        if ((At<uint8_t>(self, 0x10) & 1) == 0) {
            return nullptr;
        }
        read = true;
    }
    if (At<uint8_t>(self, 0x20) != 0) {
        return nullptr;
    }
    At<uint8_t>(self, 0x20) = 1;
    void* data = nullptr;
    uint32_t usage = At<uint32_t>(self, 0x1C);
    uint32_t mapType;
    if (usage == 2) {
        mapType = 4;   // WRITE_DISCARD
    } else if (usage == 3) {
        mapType = read ? (write ? 3u : 1u) : 2u;   // READ_WRITE / READ / WRITE
    } else {
        re::Ptr32<void> staging = nullptr;   // (the flags argument's slot)
        DataStream_CreateFull(At<uint32_t>(self, 0xC), 3, 0, 0x30000, 0, nullptr, &staging);
        Increment(&At<uint32_t>(staging, 4));   // (unchecked, as in the game)
        At<void*>(self, 0x24) = staging;
        void* locked = Virtual<void*>(staging, 2, uint32_t(flags));
        NiPointer_Destruct(&staging);
        return locked;
    }
    Com<long>(buffer, kMap, mapType, 0u, &data);
    return data;
}

// 0x8B8760 (thiscall, slot 3): NiD3D10DataStream::Unlock (staging: unmapped, and a stand-in made
// the immutable buffer it stands for from its data; dynamic: unmapped; default: the staging
// stream unlocked and copied into the buffer, then let go)
void __fastcall DataStream_UnlockImpl(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 0x20) == 0) {
        return;
    }
    uint32_t usage = At<uint32_t>(self, 0x1C);
    if (usage == 3) {
        Com<void>(At<void*>(self, 8), kUnmap);
        if (At<uint32_t>(self, 0x24) != 0) {
            void* old = At<void*>(self, 8);
            void* data = nullptr;
            Com<long>(old, kMap, 1u, 0u, &data);   // READ
            struct {
                void* data;
                uint32_t pitch;
                uint32_t slicePitch;
            } initial = {data, At<uint32_t>(self, 0xC), At<uint32_t>(self, 0xC)};
            uint32_t standIn = At<uint32_t>(self, 0x24);
            DataStream_AllocateBuffer(self, At<uint32_t>(self, 0xC), 1, UsageToBind(standIn), 0,
                                      At<uint32_t>(self, 0x18), &initial);
            Com<void>(old, kUnmap);
            Com<long>(old, kRelease);
            At<uint8_t>(self, 0x20) = 0;
            return;
        }
    } else if (usage == 2) {
        Com<void>(At<void*>(self, 8), kUnmap);
        At<uint8_t>(self, 0x20) = 0;
        return;
    } else {
        void* staging = At<void*>(self, 0x24);
        Virtual<void>(staging, 3);
        void* device = D3D10Renderer_GetD3DDevice(D3D10Renderer_GetRenderer());
        Com<void>(device, kCopyResource, At<void*>(self, 8), At<void*>(At<void*>(self, 0x24), 8));
        void* held = At<void*>(self, 0x24);
        if (Decrement(&At<uint32_t>(held, 4)) == 0) {
            Virtual<void>(held, 1);
        }
        At<void*>(self, 0x24) = nullptr;
    }
    At<uint8_t>(self, 0x20) = 0;
}

// 0x8B8880 (thiscall): NiD3D10DataStream::Resize(size): the buffer let go and made again with the
// same usage (bind and CPU access from the flags kept); whether it got one
uint8_t __fastcall DataStream_Resize(uint8_t* self, re::Edx, uint32_t size) {
    uint32_t access = At<uint32_t>(self, 0x10);
    uint32_t usage;
    if (access & 0x10) {
        usage = 2;
    } else if (access == 4) {
        usage = 1;
    } else {
        usage = (access & 4) != 0 ? 0u : 3u;
    }
    uint32_t bind = UsageToBind(At<uint32_t>(self, 0x14));
    uint32_t cpuAccess = 0;
    if (usage == 3) {
        bind = 0;
        cpuAccess = 0x30000;
    } else if (usage == 2) {
        cpuAccess = 0x10000;
    }
    void* buffer = At<void*>(self, 8);
    uint32_t misc = At<uint32_t>(self, 0x18);
    uint32_t d3dUsage = At<uint32_t>(self, 0x1C);
    if (buffer != nullptr) {
        Com<long>(buffer, kRelease);
    }
    At<uint32_t>(self, 0xC) = 0;
    At<uint32_t>(self, 0x10) = 0;
    DataStream_AllocateBuffer(self, size, d3dUsage, bind, cpuAccess, misc, nullptr);
    return At<void*>(self, 8) != nullptr ? 1 : 0;
}

// 0x8B8930 (thiscall): NiD3D10DataStream::~NiD3D10DataStream (unlocked first: a staging one's
// stand-in flags dropped so it doesn't make the buffer again)
void __fastcall DataStream_DestructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kDataStreamVtable;
    if (At<uint8_t>(self, 0x20) != 0) {
        if (At<uint32_t>(self, 0x1C) == 3) {
            At<uint32_t>(self, 0x24) = 0;
        }
        DataStream_Unlock(self);
    }
    ComRelease(At<void*>(self, 8));
    At<uint32_t>(self, 0xC) = 0;
    At<uint32_t>(self, 0x10) = 0;
    At<uint32_t>(self, 0) = kRefObjectVtable;
    Decrement(&ms_uiObjects);
}

// 0x8B8990 (thiscall, slot 0): NiD3D10DataStream's deleting destructor
uint8_t* __fastcall DataStream_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    DataStream_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x28);
    }
    return self;
}

#define GB43I_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

GB43I_REVERSED("NiD3D10GeometryData::GetSetOffset", 0x008B72C0, GeometryData_GetSetOffset);
GB43I_REVERSED("NiD3D10GeometryData::GetSetPrimitiveCount", 0x008B72E0, GeometryData_GetSetPrimitiveCount);
GB43I_REVERSED("NiD3D10GeometryData::GetPrimitiveCount", 0x008B7300, GeometryData_GetPrimitiveCount);
GB43I_REVERSED("NiD3D10GeometryData::GetIndexBuffer", 0x008B7310, GeometryData_GetIndexBuffer);
GB43I_REVERSED("NiD3D10GeometryData::GetIndexFormat", 0x008B7340, GeometryData_GetIndexFormat);
GB43I_REVERSED("NiD3D10GeometryData::GetIndexOffset", 0x008B7370, GeometryData_GetIndexOffset);
GB43I_REVERSED("NiD3D10GeometryData::GetVBOffsets", 0x008B73A0, GeometryData_GetVBOffsets);
GB43I_REVERSED("NiD3D10GeometryData::GetIndexCount", 0x008B73D0, GeometryData_GetIndexCount);
GB43I_REVERSED("NiD3D10GeometryData::GetStartIndex", 0x008B7400, GeometryData_GetStartIndex);
GB43I_REVERSED("NiD3D10GeometryData::GetBaseVertex", 0x008B7430, GeometryData_GetBaseVertex);
GB43I_REVERSED("NiD3D10GeometryData::SetVBStrides", 0x008B7470, GeometryData_SetVBStrides);
GB43I_REVERSED("NiD3D10GeometryData::SetIndexFormat", 0x008B74C0, GeometryData_SetIndexFormat);
GB43I_REVERSED("NiD3D10GeometryData::SetIndexOffset", 0x008B74F0, GeometryData_SetIndexOffset);
GB43I_REVERSED("NiD3D10GeometryData::SetVBOffsets", 0x008B7520, GeometryData_SetVBOffsets);
GB43I_REVERSED("NiD3D10GeometryData::SetIndexCount", 0x008B7590, GeometryData_SetIndexCount);
GB43I_REVERSED("NiD3D10GeometryData::SetStartIndex", 0x008B75C0, GeometryData_SetStartIndex);
GB43I_REVERSED("NiD3D10GeometryData::SetBaseVertex", 0x008B75F0, GeometryData_SetBaseVertex);
GB43I_REVERSED("NiD3D10GeometryData::SetFlags", 0x008B7620, GeometryData_SetFlags);
GB43I_REVERSED("NiD3D10GeometryData::MakeFlags", 0x008B7630, GeometryData_MakeFlags);
GB43I_REVERSED("NiD3D10GeometryData::FlagsHave400000", 0x008B7670, GeometryData_FlagsHave400000);
GB43I_REVERSED("NiD3D10GeometryData::Set44", 0x008B7690, GeometryData_Set44);
GB43I_REVERSED("NiD3D10GeometryData::Primitive::Primitive", 0x008B76C0, Primitive_Construct);
GB43I_REVERSED("NiD3D10GeometryData::Primitive::~Primitive", 0x008B76F0, Primitive_DestructImpl);
GB43I_REVERSED("NiD3D10GeometryData::Primitive::VectorDeletingDestructor", 0x008B7740,
               Primitive_VectorDeletingDestructor);
GB43I_REVERSED("NiD3D10GeometryData::ContainsVertexData", 0x008B77E0, GeometryData_ContainsVertexData);
GB43I_REVERSED("NiD3D10GeometryData::GetVBDataStream", 0x008B7850, GeometryData_GetVBDataStream);
GB43I_REVERSED("NiD3D10GeometryData::GetIndexDataStream", 0x008B7880, GeometryData_GetIndexDataStream);
GB43I_REVERSED("NiD3D10GeometryData::SetInputElements", 0x008B78B0, GeometryData_SetInputElements);
GB43I_REVERSED("NiD3D10GeometryData::SetVertexBuffer", 0x008B7930, GeometryData_SetVertexBuffer);
GB43I_REVERSED("NiD3D10GeometryData::SetIndexBuffer", 0x008B7A00, GeometryData_SetIndexBuffer);
GB43I_REVERSED("NiD3D10GeometryData::UpdateVertexBuffers", 0x008B7AC0, GeometryData_UpdateVertexBuffers);
GB43I_REVERSED("NiD3D10GeometryData::UpdateIndexBuffers", 0x008B7B30, GeometryData_UpdateIndexBuffers);
GB43I_REVERSED("NiD3D10GeometryData::NiD3D10GeometryData", 0x008B7BB0, GeometryData_ConstructSingle);
GB43I_REVERSED("NiD3D10GeometryData::NiD3D10GeometryData_8B7C90", 0x008B7C90, GeometryData_ConstructSets);
GB43I_REVERSED("NiD3D10GeometryData::UpdateInputLayout", 0x008B7DD0, GeometryData_UpdateInputLayout);
GB43I_REVERSED("NiD3D10GeometryData::SetVBCount", 0x008B7EA0, GeometryData_SetVBCount);
GB43I_REVERSED("NiD3D10GeometryData::ReleaseInputLayouts", 0x008B8060, GeometryData_ReleaseInputLayoutsImpl);
GB43I_REVERSED("NiD3D10GeometryData::~NiD3D10GeometryData", 0x008B8100, GeometryData_DestructImpl);
GB43I_REVERSED("NiD3D10GeometryData::DeletingDestructor", 0x008B8270, GeometryData_DeletingDestructor);
GB43I_REVERSED("NiD3D10DataStream::GetUsageFlags", 0x008B8290, DataStream_GetUsageFlagsImpl);
GB43I_REVERSED("NiD3D10DataStream::GetD3D10Usage", 0x008B82A0, DataStream_GetD3D10UsageImpl);
GB43I_REVERSED("NiD3D10DataStream::ConvertD3D10Flags", 0x008B82F0, DataStream_ConvertD3D10FlagsImpl);
GB43I_REVERSED("NiD3D10DataStream::AllocateBuffer", 0x008B8390, DataStream_AllocateBufferImpl);
GB43I_REVERSED("NiD3D10DataStream::NiD3D10DataStream", 0x008B8470, DataStream_ConstructImpl);
GB43I_REVERSED("NiD3D10DataStream::Create", 0x008B84E0, DataStream_Create);
GB43I_REVERSED("NiD3D10DataStream::CreateFull", 0x008B85C0, DataStream_CreateFullImpl);
GB43I_REVERSED("NiD3D10DataStream::Lock", 0x008B8660, DataStream_Lock);
GB43I_REVERSED("NiD3D10DataStream::Unlock", 0x008B8760, DataStream_UnlockImpl);
GB43I_REVERSED("NiD3D10DataStream::Resize", 0x008B8880, DataStream_Resize);
GB43I_REVERSED("NiD3D10DataStream::~NiD3D10DataStream", 0x008B8930, DataStream_DestructImpl);
GB43I_REVERSED("NiD3D10DataStream::DeletingDestructor", 0x008B8990, DataStream_DeletingDestructor);

} // namespace ni::c43
