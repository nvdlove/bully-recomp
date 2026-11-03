// NiAdditionalGeometryData (vtable 0x961990, 0x28 bytes, constructor 0x7E1870): extra per-vertex
// streams for a geometry's data (the renderer reads them with GetDataStream). Each stream
// (NiAGDDataStream, 0x1C bytes) names a type (AGD_NITYPE_*: FLOAT1..4, LONG1..4, ULONG1..4,
// SHORT1..4, USHORT1..4, BYTE1..4, UBYTE1..4, BLEND1..4), its sizes, and where it sits in one of
// the data blocks (NiAGDDataBlock, 8 bytes: a size and the NiMalloc'd bytes). Streaming (a block
// written in runs of streams that follow each other, with their component sizes for the endian
// swap), comparison, viewer strings. And two static tables the library's SDM init fills, next to
// it in the exe (their owner isn't named anywhere), with the SDM shutdown beside them.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiAdditionalGeometryData_RTTI, 0x020E3FEC);
constexpr uintptr_t kNiAdditionalGeometryDataVtable = 0x00961990;
constexpr uintptr_t kBlockArrayVtable = 0x00961988;   // NiTPrimitiveArray<NiAGDDataBlock*>
constexpr uintptr_t kStreamConstruct = 0x007E1830;

struct NiAGDDataStream {
    uint8_t flags;          // +0x0 (bit 0 keep; bits 1..2 the consistency: 2 mutable, 4 volatile)
    uint8_t pad1[3];
    uint32_t type;          // +0x4
    uint32_t unitSize;      // +0x8
    uint32_t totalSize;     // +0xC
    uint32_t stride;        // +0x10
    uint32_t blockIndex;    // +0x14
    uint32_t blockOffset;   // +0x18
};
static_assert(sizeof(NiAGDDataStream) == 0x1C);

struct NiAGDDataBlock {
    uint32_t size;          // +0x0
    re::Ptr32<uint8_t> data;          // +0x4 (NiMalloc)
};
static_assert(sizeof(NiAGDDataBlock) == 8);

struct NiAdditionalGeometryData : NiObject {
    uint32_t unknown8;                     // +0x8 (0 from the constructor)
    uint16_t vertexCount;                  // +0xC
    uint16_t padE;
    uint32_t streamCount;                  // +0x10
    re::Ptr32<NiAGDDataStream> streams;              // +0x14 (NiNew[]'d, no count)
    NiTArray<NiAGDDataBlock*> blocks;      // +0x18
};
static_assert(offsetof(NiAdditionalGeometryData, vertexCount) == 0xC);
static_assert(offsetof(NiAdditionalGeometryData, streams) == 0x14);
static_assert(offsetof(NiAdditionalGeometryData, blocks) == 0x18);
static_assert(sizeof(NiAdditionalGeometryData) == 0x28);

// NiTArray<4-byte element>'s (shared by every kind)
GAME_FN(void, __thiscall, PtrArray_SetSize, 0x0084A110, void* array, uint32_t size);
GAME_FN(void, __thiscall, PtrArray_SetAt, 0x00833C70, void* array, uint32_t index, const re::Ptr32<void>* value);
GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* ctor);
GAME_FN(void, __thiscall, Object_SaveBinary, 0x008A20A0, void* self, void* stream);
GAME_FN(char*, __cdecl, GetViewerStringShort, 0x00785650, const char* prefix, uint32_t value);
GAME_FN(char*, __cdecl, GetViewerStringInt, 0x007856A0, const char* prefix, int32_t value);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, Stream_GetComponentSizes, 0x007E0CB0, NiAGDDataStream* self, uint32_t* sizes,
        uint32_t* count, uint32_t max);
GAME_FN(uint8_t, __thiscall, Block_IsEqual, 0x007E0DC0, NiAGDDataBlock* self, NiAGDDataBlock* other);
GAME_FN(uint8_t, __thiscall, AGD_RemoveDataBlock, 0x007E0E70, NiAdditionalGeometryData* self, uint32_t index,
        int removeStreams);
GAME_FN(void, __thiscall, Stream_LoadBinary, 0x007E1080, NiAGDDataStream* self, void* stream);
GAME_FN(void, __thiscall, Stream_SaveBinary, 0x007E1180, NiAGDDataStream* self, void* stream);
GAME_FN(void, __thiscall, Block_LoadBinary, 0x007E1280, NiAGDDataBlock* self, void* stream, uint32_t index,
        NiAGDDataStream* streams, uint32_t streamCount, uint32_t vertexCount);
GAME_FN(void, __thiscall, Block_SaveBinary, 0x007E13E0, NiAGDDataBlock* self, void* stream, uint32_t index,
        NiAGDDataStream* streams, uint32_t streamCount, uint32_t vertexCount);
GAME_FN(void, __thiscall, AGD_Destruct, 0x007E1670, NiAdditionalGeometryData* self);
GAME_FN(NiAdditionalGeometryData*, __thiscall, AGD_Construct, 0x007E1870, void* self);
GAME_FN(void, __thiscall, Stream_GetViewerStrings, 0x007E1B10, NiAGDDataStream* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, Block_GetViewerStrings, 0x007E23A0, NiAGDDataBlock* self, NiTArray<char*>* strings);
GAME_FN(void, __cdecl, Tables_InitA, 0x007E27B0);
GAME_FN(void, __cdecl, Tables_InitB, 0x007E28C0);

// a stream read / write of `bytes` described as `components` components of the sizes given
inline void LoadRaw(void* stream, void* buffer, uint32_t bytes, uint32_t* componentSizes, uint32_t components) {
    BinaryStream* in = At<BinaryStream*>(stream, 0x188);
    in->position += in->read(in, buffer, bytes, componentSizes, components);
}
inline void SaveRaw(void* stream, const void* buffer, uint32_t bytes, uint32_t* componentSizes,
                    uint32_t components) {
    BinaryStream* out = At<BinaryStream*>(stream, 0x18C);
    out->position += out->write(out, buffer, bytes, componentSizes, components);
}

// NiTArray::SetAt inlined (the used count kept)
inline void SetBlockAt(NiTArray<NiAGDDataBlock*>& array, uint32_t index, NiAGDDataBlock* value) {
    if (index >= array.size) {
        array.size = uint16_t(index + 1);
        if (value != nullptr) {
            array.esize++;
        }
    } else if (value != nullptr) {
        if (array.base[index] == nullptr) {
            array.esize++;
        }
    } else if (array.base[index] != nullptr) {
        array.esize--;
    }
    array.base[index] = value;
}

// ---- NiAGDDataStream

// 0x7E0CB0 GetComponentSizes(sizes, count&, max): the type's components' sizes appended (4, 2 or
// 1 bytes; 1..4 of them). `max` (25 from the callers) isn't checked
void __fastcall StreamGetComponentSizes(NiAGDDataStream* self, re::Edx, uint32_t* sizes, uint32_t* count, uint32_t) {
    uint32_t type = self->type;
    if (type - 1 > 0x1F) {
        return;
    }
    uint32_t group = (type - 1) / 4;   // 0..2: 4 bytes, 3..4: 2 bytes, 5..7: 1 byte
    uint32_t size = group < 3 ? 4 : group < 5 ? 2 : 1;
    uint32_t components = (type - 1) % 4 + 1;
    for (uint32_t i = 0; i < components; i++) {
        sizes[*count] = size;
        *count = *count + 1;
    }
}

// 0x7E1080 LoadBinary(stream): the six words, then the flags byte
void __fastcall StreamLoadBinary(NiAGDDataStream* self, re::Edx, void* stream) {
    LoadBytes(stream, &self->type, 4);
    LoadBytes(stream, &self->unitSize, 4);
    LoadBytes(stream, &self->totalSize, 4);
    LoadBytes(stream, &self->stride, 4);
    LoadBytes(stream, &self->blockIndex, 4);
    LoadBytes(stream, &self->blockOffset, 4);
    LoadBytes(stream, &self->flags, 1);
}

// 0x7E1180 SaveBinary(stream)
void __fastcall StreamSaveBinary(NiAGDDataStream* self, re::Edx, void* stream) {
    SaveBytes(stream, &self->type, 4);
    SaveBytes(stream, &self->unitSize, 4);
    SaveBytes(stream, &self->totalSize, 4);
    SaveBytes(stream, &self->stride, 4);
    SaveBytes(stream, &self->blockIndex, 4);
    SaveBytes(stream, &self->blockOffset, 4);
    SaveBytes(stream, &self->flags, 1);
}

// 0x7E1830 NiAGDDataStream(): empty, mutable
NiAGDDataStream* __fastcall StreamConstruct(NiAGDDataStream* self, re::Edx) {
    self->type = 0;
    self->unitSize = 0;
    self->totalSize = 0;
    self->stride = 0;
    self->blockIndex = 0;
    self->blockOffset = 0;
    self->flags = 2;
    return self;
}

const char* const kTypeNames[0x22] = {
    "AGD_NITYPE_INVALID", "AGD_NITYPE_FLOAT1", "AGD_NITYPE_FLOAT2", "AGD_NITYPE_FLOAT3", "AGD_NITYPE_FLOAT4",
    "AGD_NITYPE_LONG1",   "AGD_NITYPE_LONG2",  "AGD_NITYPE_LONG3",  "AGD_NITYPE_LONG4",  "AGD_NITYPE_ULONG1",
    "AGD_NITYPE_ULONG2",  "AGD_NITYPE_ULONG3", "AGD_NITYPE_ULONG4", "AGD_NITYPE_SHORT1", "AGD_NITYPE_SHORT2",
    "AGD_NITYPE_SHORT3",  "AGD_NITYPE_SHORT4", "AGD_NITYPE_USHORT1", "AGD_NITYPE_USHORT2", "AGD_NITYPE_USHORT3",
    "AGD_NITYPE_USHORT4", "AGD_NITYPE_BYTE1",  "AGD_NITYPE_BYTE2",  "AGD_NITYPE_BYTE3",  "AGD_NITYPE_BYTE4",
    "AGD_NITYPE_UBYTE1",  "AGD_NITYPE_UBYTE2", "AGD_NITYPE_UBYTE3", "AGD_NITYPE_UBYTE4", "AGD_NITYPE_BLEND1",
    "AGD_NITYPE_BLEND2",  "AGD_NITYPE_BLEND3", "AGD_NITYPE_BLEND4", "AGD_NITYPE_COUNT",
};

// 0x7E1B10 GetViewerStrings(strings): the known types added with NiTArray::Add, the rest as the
// library inlines it
void __fastcall StreamGetViewerStrings(NiAGDDataStream* self, re::Edx, NiTArray<char*>* strings) {
    if (self->type <= 0x21) {
        re::Ptr32<char> s = GetViewerStringString("        m_uiType", kTypeNames[self->type]);
        StringArray_Add(strings, &s);
    } else {
        AddViewerString(strings, GetViewerStringString("        m_uiType", "UNKNOWN!!!"));
    }
    AddViewerString(strings, GetViewerStringUint("        m_uiUnitSize", self->unitSize));
    AddViewerString(strings, GetViewerStringUint("        m_uiTotalSize", self->totalSize));
    AddViewerString(strings, GetViewerStringUint("        m_uiStride", self->stride));
    AddViewerString(strings, GetViewerStringUint("        m_uiBlockIndex", self->blockIndex));
    AddViewerString(strings, GetViewerStringUint("        m_uiBlockOffset", self->blockOffset));
    AddViewerString(strings, GetViewerStringBool("        Keep", self->flags & 1));
    uint32_t consistency = self->flags & 6;
    if (consistency == 4) {
        re::Ptr32<char> s = GetViewerStringString("        Consistency", "VOLATILE");
        StringArray_Add(strings, &s);
        return;
    }
    const char* name = consistency == 2 ? "MUTABLE" : "STATIC";
    AddViewerString(strings, GetViewerStringString("        Consistency", name));
}

// ---- NiAGDDataBlock

// 0x7E0DC0 operator==: the same size and bytes
uint8_t __fastcall BlockIsEqual(NiAGDDataBlock* self, re::Edx, NiAGDDataBlock* other) {
    uint32_t size = self->size;
    if (size != other->size) {
        return 0;
    }
    const uint8_t* a = self->data;
    const uint8_t* b = other->data;
    for (uint32_t i = 0; i < size; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

// 0x7E1280 LoadBinary(stream, index, streams, stream count, vertex count): the size and the bytes
// (allocated), read in runs: their offsets, then each one's component sizes and its bytes (vertex
// count times the sum of the sizes). The runs and their components are stack arrays of 25, as in
// the original (no bound checked)
void __fastcall BlockLoadBinary(NiAGDDataBlock* self, re::Edx, void* stream, uint32_t, NiAGDDataStream*, uint32_t,
                                uint32_t vertexCount) {
    uint32_t componentSize = 4;
    LoadRaw(stream, &self->size, 4, &componentSize, 1);
    self->data = static_cast<uint8_t*>(NiMalloc(self->size));
    uint32_t runs;
    uint32_t offsets[25];
    uint32_t sizes[25];
    componentSize = 4;
    LoadRaw(stream, &runs, 4, &componentSize, 1);
    componentSize = 4;
    LoadRaw(stream, offsets, runs * 4, &componentSize, 1);
    for (uint32_t i = 0; i < runs; i++) {
        uint32_t components;
        uint32_t size = 4;
        LoadRaw(stream, &components, 4, &size, 1);
        size = 4;
        LoadRaw(stream, sizes, components * 4, &size, 1);
        uint8_t* to = self->data + offsets[i];
        uint32_t sum = 0;
        for (uint32_t k = 0; k < components; k++) {
            sum += sizes[k];
        }
        LoadRaw(stream, to, uint16_t(vertexCount) * sum, sizes, components);
    }
}

// 0x7E13E0 SaveBinary(stream, index, streams, stream count, vertex count): the block's streams
// sorted by offset and cut into runs (a new run where a stream starts past the end of the current
// run's first stream); the runs' offsets, then each run's component sizes and bytes
void __fastcall BlockSaveBinary(NiAGDDataBlock* self, re::Edx, void* stream, uint32_t index, NiAGDDataStream* streams,
                                uint32_t streamCount, uint32_t vertexCount) {
    uint32_t componentSize = 4;
    SaveRaw(stream, &self->size, 4, &componentSize, 1);
    NiAGDDataStream* list[25];
    uint32_t count = 0;
    for (uint32_t k = streamCount; k != 0; k--) {
        if (streams->blockIndex == index) {
            list[count++] = streams;
        }
        streams++;
    }
    for (uint32_t i = 0; i < count; i++) {
        NiAGDDataStream* current = list[i];
        for (uint32_t j = 0; j < i; j++) {
            NiAGDDataStream* other = list[j];
            if (other->blockOffset > current->blockOffset) {
                list[j] = current;
                current = other;
            }
        }
        list[i] = current;
    }
    uint32_t starts[25];
    uint32_t offsets[25];
    starts[0] = 0;
    offsets[0] = 0;
    uint32_t runs = 1;
    NiAGDDataStream* head = list[0];
    for (uint32_t k = 1; k < count; k++) {
        NiAGDDataStream* s = list[k];
        if (s->blockOffset > head->blockOffset + head->totalSize) {
            starts[runs] = k;
            offsets[runs] = s->blockOffset;
            runs++;
            head = s;
        }
    }
    starts[runs] = count;
    componentSize = 4;
    SaveRaw(stream, &runs, 4, &componentSize, 1);
    componentSize = 4;
    SaveRaw(stream, offsets, runs * 4, &componentSize, 1);
    uint32_t sizes[25];
    for (uint32_t r = 0; r < runs; r++) {
        uint32_t first = starts[r];
        uint32_t end = starts[r + 1];
        uint32_t components = 0;
        for (uint32_t k = first; k < end; k++) {
            Stream_GetComponentSizes(list[k], sizes, &components, 0x19);
        }
        uint32_t written = components;
        uint32_t size = 4;
        SaveRaw(stream, &written, 4, &size, 1);
        size = 4;
        SaveRaw(stream, sizes, written * 4, &size, 1);
        // (with no streams in the block, list[0] is read uninitialized, as in the original)
        const uint8_t* from = self->data + list[first]->blockOffset;
        uint32_t sum = 0;
        for (uint32_t k = 0; k < written; k++) {
            sum += sizes[k];
        }
        SaveRaw(stream, from, uint16_t(vertexCount) * sum, sizes, written);
    }
}

// 0x7E23A0 GetViewerStrings(strings)
void __fastcall BlockGetViewerStrings(NiAGDDataBlock* self, re::Edx, NiTArray<char*>* strings) {
    AddViewerString(strings, GetViewerStringUint("        m_uiDataBlockSize", self->size));
    AddViewerString(strings, GetViewerStringPointer("        m_pucDataBlock", self->data));
}

// ---- NiAdditionalGeometryData

// 0x7E0E70 RemoveDataBlock(index, remove streams): the block freed and its slot emptied; its
// streams cleared when asked (the flags kept)
uint8_t __fastcall AGDRemoveDataBlock(NiAdditionalGeometryData* self, re::Edx, uint32_t index, int removeStreams) {
    if (index >= self->blocks.size) {
        return 0;
    }
    NiAGDDataBlock* block = self->blocks.base[index];
    if (block == nullptr) {
        return 0;
    }
    NiFree(block->data);
    NiDelete(block, 8);
    NiAGDDataBlock* none = nullptr;
    PtrArray_SetAt(&self->blocks, index, re::PtrCast<const re::Ptr32<void>*>(&none));
    if (re::Flag(removeStreams) && self->streamCount != 0) {
        for (uint32_t k = 0; k < self->streamCount; k++) {
            NiAGDDataStream* streams = self->streams;
            if (index == streams[k].blockIndex && k < self->streamCount && streams != nullptr) {
                streams[k].type = 0;
                streams[k].totalSize = 0;
                streams[k].unitSize = 0;
                streams[k].stride = 0;
                streams[k].blockIndex = 0;
                streams[k].blockOffset = 0;
            }
        }
    }
    return 1;
}

// 0x7E0F10 GetDataStream(index, data&, type&, count&, total size&, unit size&, stride&): the
// type always given; the rest when it's set, the data when its block is there (an index equal to
// the block count passes the check, as in the original) and holds the offset
uint8_t __fastcall AGDGetDataStream(NiAdditionalGeometryData* self, re::Edx, uint32_t index, re::Ptr32<uint8_t>* data,
                                    uint32_t* type, uint16_t* count, uint32_t* totalSize, uint32_t* unitSize,
                                    uint32_t* stride) {
    if (index >= self->streamCount || self->streams == nullptr) {
        return 0;
    }
    NiAGDDataStream* s = &self->streams[index];
    *type = s->type;
    if (s->type == 0) {
        return 0;
    }
    *count = self->vertexCount;
    *totalSize = s->totalSize;
    *unitSize = s->unitSize;
    *stride = s->stride;
    if (s->blockIndex > self->blocks.size) {
        return 0;
    }
    NiAGDDataBlock* block = self->blocks.base[s->blockIndex];
    if (block == nullptr || block->size < s->blockOffset) {
        return 0;
    }
    *data = block->data + s->blockOffset;
    return 1;
}

// vf8 IsEqual: the counts, every stream, the blocks (the other's array read up to this one's
// size)
uint8_t __fastcall AGDIsEqual(NiAdditionalGeometryData* self, re::Edx, NiAdditionalGeometryData* other) {
    if (Object_IsEqual(self, other) == 0) {
        return 0;
    }
    if (self->vertexCount != other->vertexCount || self->streamCount != other->streamCount) {
        return 0;
    }
    for (uint32_t i = 0; i < self->streamCount; i++) {
        const NiAGDDataStream& a = self->streams[i];
        const NiAGDDataStream& b = other->streams[i];
        if (a.type != b.type || a.unitSize != b.unitSize || a.totalSize != b.totalSize || a.stride != b.stride ||
            a.blockIndex != b.blockIndex || a.blockOffset != b.blockOffset || a.flags != b.flags) {
            return 0;
        }
    }
    uint32_t blocks = self->blocks.size;
    for (uint32_t i = 0; i < blocks; i++) {
        NiAGDDataBlock* theirs = other->blocks.base[i];
        NiAGDDataBlock* mine = self->blocks.base[i];
        if (theirs == nullptr) {
            if (mine != nullptr) {
                return 0;
            }
        } else {
            if (mine == nullptr) {
                return 0;
            }
            if (Block_IsEqual(mine, theirs) == 0) {
                return 0;
            }
        }
    }
    return 1;
}

// 0x7E1670 ~NiAdditionalGeometryData (a tail call into ~NiObject)
void __fastcall AGDDestruct(NiAdditionalGeometryData* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiAdditionalGeometryDataVtable);
    for (uint32_t i = 0; i < self->blocks.size; i++) {
        AGD_RemoveDataBlock(self, i, 1);
    }
    if (self->streams != nullptr) {
        NiDeleteArray(self->streams, 0x1C);
    }
    self->blocks.vtable = reinterpret_cast<void*>(kBlockArrayVtable);
    NiFree(self->blocks.base);
    Object_Destruct(self);
}

// vf2
const RTTI* __fastcall AGDGetRTTI(NiAdditionalGeometryData*, re::Edx) {
    return &NiAdditionalGeometryData_RTTI;
}

// vf7 SaveBinary: the counts, the streams, then each block (a byte: there or not)
void __fastcall AGDSaveBinary(NiAdditionalGeometryData* self, re::Edx, void* stream) {
    Object_SaveBinary(self, stream);
    SaveBytes(stream, &self->vertexCount, 2);
    SaveBytes(stream, &self->streamCount, 4);
    for (uint32_t i = 0; i < self->streamCount; i++) {
        Stream_SaveBinary(&self->streams[i], stream);
    }
    uint32_t blocks = self->blocks.size;
    SaveBytes(stream, &blocks, 4);
    for (uint32_t i = 0; i < self->blocks.size; i++) {
        NiAGDDataBlock* block = self->blocks.base[i];
        uint8_t present = block != nullptr ? 1 : 0;
        SaveBytes(stream, &present, 1);
        if (present != 0) {
            Block_SaveBinary(block, stream, i, self->streams, self->streamCount, self->vertexCount);
        }
    }
}

// vf0 the deleting destructor
NiAdditionalGeometryData* __fastcall AGDDeletingDestructor(NiAdditionalGeometryData* self, re::Edx, uint32_t flags) {
    AGD_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x28);
    }
    return self;
}

// 0x7E1870 NiAdditionalGeometryData() (the block array's RemoveAll inlined on the empty array)
NiAdditionalGeometryData* __fastcall AGDConstruct(NiAdditionalGeometryData* self, re::Edx) {
    RefObject_Construct(self);
    self->vtable = reinterpret_cast<void*>(kNiAdditionalGeometryDataVtable);
    self->blocks.maxSize = 0;
    self->blocks.growBy = 1;
    self->blocks.size = 0;
    self->blocks.esize = 0;
    self->blocks.base = nullptr;
    self->blocks.vtable = reinterpret_cast<void*>(kBlockArrayVtable);
    self->unknown8 = 0;
    self->streamCount = 0;
    self->streams = nullptr;
    self->vertexCount = 0;
    for (uint16_t i = 0; i < self->blocks.size; i++) {
        self->blocks.base[i] = nullptr;
    }
    self->blocks.size = 0;
    self->blocks.esize = 0;
    return self;
}

// 0x7E18E0 CreateObject (the stream's factory)
NiAdditionalGeometryData* __cdecl AGDCreateObject() {
    void* memory = NiNew(0x28);
    if (memory == nullptr) {
        return nullptr;
    }
    return AGD_Construct(memory);
}

// vf4 LoadBinary: the counts, the streams (new[]'d, each read; a failed allocation is read into
// anyway), the block array sized, then each block that's there made and read
void __fastcall AGDLoadBinary(NiAdditionalGeometryData* self, re::Edx, void* stream) {
    Object_LoadBinary(self, stream);
    LoadBytes(stream, &self->vertexCount, 2);
    LoadBytes(stream, &self->streamCount, 4);
    uint32_t count = self->streamCount;
    if (count != 0) {
        uint64_t bytes = uint64_t(count) * 0x1C;
        void* memory = NiNewArray((bytes >> 32) != 0 ? 0xFFFFFFFF : uint32_t(bytes));
        NiAGDDataStream* streams = nullptr;
        if (memory != nullptr) {
            VectorConstruct(memory, 0x1C, count, reinterpret_cast<void*>(kStreamConstruct));
            streams = static_cast<NiAGDDataStream*>(memory);
        }
        self->streams = streams;
        for (uint32_t i = 0; i < self->streamCount; i++) {
            Stream_LoadBinary(&self->streams[i], stream);
        }
    }
    uint32_t blocks = 0;
    LoadBytes(stream, &blocks, 4);
    PtrArray_SetSize(&self->blocks, blocks);
    for (uint32_t i = 0; i < blocks; i++) {
        uint8_t present;
        LoadBytes(stream, &present, 1);
        if (present == 0) {
            SetBlockAt(self->blocks, i, nullptr);
            continue;
        }
        NiAGDDataBlock* block = static_cast<NiAGDDataBlock*>(NiNew(8));
        if (block != nullptr) {
            block->size = 0;
            block->data = nullptr;
        }
        Block_LoadBinary(block, stream, i, self->streams, self->streamCount, self->vertexCount);
        SetBlockAt(self->blocks, i, block);
    }
}

// vf9 GetViewerStrings (the per-stream and per-block lines added as the library inlines it)
void __fastcall AGDGetViewerStrings(NiAdditionalGeometryData* self, re::Edx, NiTArray<char*>* strings) {
    Object_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiAdditionalGeometryData_RTTI.name));
    AddViewerString(strings, GetViewerStringShort("m_usVertexCount", self->vertexCount));
    AddViewerString(strings, GetViewerStringUint("m_uiDataStreamCount", self->streamCount));
    for (uint32_t i = 0; i < self->streamCount; i++) {
        AddViewerStringInline(strings, GetViewerStringUint("    DataStream Index", i));
        Stream_GetViewerStrings(&self->streams[i], strings);
    }
    AddViewerStringInline(strings, GetViewerStringUint("m_aDataBlocks.GetSize()", self->blocks.size));
    for (uint32_t i = 0; i < self->blocks.size; i++) {
        AddViewerStringInline(strings, GetViewerStringUint("    DataBlock Index", i));
        NiAGDDataBlock* block = self->blocks.base[i];
        if (block != nullptr) {
            Block_GetViewerStrings(block, strings);
        } else {
            AddViewerStringInline(strings, GetViewerStringInt("        m_uiDataBlockSize", 0));
            AddViewerStringInline(strings, GetViewerStringString("        m_pucDataBlock", "NULL"));
        }
    }
}

// ---- two static tables (0..14 as words and bytes; 0..14 as words and 0..0xE00 in steps of 0x100
// as 16-bit values), each with its "initialized" byte

GAME_ARRAY(uint32_t, TableA_Words, 0x020E408C);    // 15
GAME_ARRAY(uint8_t, TableA_Bytes, 0x020E407C);     // 15
GAME_VAR(uint8_t, TableA_Initialized, 0x020E40C8);
GAME_ARRAY(uint32_t, TableB_Words, 0x020E4040);    // 15
GAME_ARRAY(uint16_t, TableB_Shorts, 0x020E4020);   // 15
GAME_VAR(uint8_t, TableB_Initialized, 0x020E40C9);

void __cdecl TablesInitA() {
    for (uint32_t i = 0; i < 15; i++) {
        TableA_Words[i] = i;
    }
    for (uint32_t i = 0; i < 15; i++) {
        TableA_Bytes[i] = uint8_t(i);
    }
    TableA_Initialized = 1;
}

void __cdecl TablesInitB() {
    for (uint32_t i = 0; i < 15; i++) {
        TableB_Words[i] = i;
    }
    for (uint32_t i = 0; i < 15; i++) {
        TableB_Shorts[i] = uint16_t(i << 8);
    }
    TableB_Initialized = 1;
}

void __cdecl TablesInit() {
    Tables_InitA();
    Tables_InitB();
}

// ---- the shutdown next to Init in the exe (NiMainSDM::Shutdown calls it after
// NiMaterialProperty's, where Init has the game's empty 0x49A650): under its lock (a
// CRITICAL_SECTION, made by the static initializer 0x8ECEC0), every slot of the NiTObjectArray
// of NiPointers at 0xBC8B74 set to null (the same empty pointer each time), then its size to 0

constexpr uintptr_t kPointerArrayLock = 0x020E40CC;
inline NiTArray<void*>* const PointerArray = reinterpret_cast<NiTArray<void*>*>(0x00BC8B74);
GAME_FN(void, __thiscall, ObjectArray_SetAt, 0x0040E800, NiTArray<void*>* self, uint32_t index,
        const re::Ptr32<void>* value);
GAME_FN(void, __thiscall, ObjectArray_SetSize, 0x00856170, NiTArray<void*>* self, uint32_t size);

void __cdecl TablesShutdown() {
    ni::sys::Import<void>(ni::sys::kEnterCriticalSection, reinterpret_cast<void*>(kPointerArrayLock));
    re::Ptr32<void> none = nullptr;
    for (uint32_t i = 0; i < PointerArray->size; i++) {
        ObjectArray_SetAt(PointerArray, i, &none);
    }
    ObjectArray_SetSize(PointerArray, 0);
    ni::sys::Import<void>(ni::sys::kLeaveCriticalSection, reinterpret_cast<void*>(kPointerArrayLock));
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIAGD_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIAGD_REVERSED("NiAGDDataStream::GetComponentSizes", 0x007E0CB0, ni::StreamGetComponentSizes);
NIAGD_REVERSED("NiAGDDataBlock::operator==", 0x007E0DC0, ni::BlockIsEqual);
NIAGD_REVERSED("NiAdditionalGeometryData::RemoveDataBlock", 0x007E0E70, ni::AGDRemoveDataBlock);
NIAGD_REVERSED("NiAdditionalGeometryData::GetDataStream", 0x007E0F10, ni::AGDGetDataStream);
NIAGD_REVERSED("NiAdditionalGeometryData::IsEqual", 0x007E0FA0, ni::AGDIsEqual);
NIAGD_REVERSED("NiAGDDataStream::LoadBinary", 0x007E1080, ni::StreamLoadBinary);
NIAGD_REVERSED("NiAGDDataStream::SaveBinary", 0x007E1180, ni::StreamSaveBinary);
NIAGD_REVERSED("NiAGDDataBlock::LoadBinary", 0x007E1280, ni::BlockLoadBinary);
NIAGD_REVERSED("NiAGDDataBlock::SaveBinary", 0x007E13E0, ni::BlockSaveBinary);
NIAGD_REVERSED("NiAdditionalGeometryData::~NiAdditionalGeometryData", 0x007E1670, ni::AGDDestruct);
NIAGD_REVERSED("NiAdditionalGeometryData::GetRTTI", 0x007E16D0, ni::AGDGetRTTI);
NIAGD_REVERSED("NiAdditionalGeometryData::SaveBinary", 0x007E16E0, ni::AGDSaveBinary);
NIAGD_REVERSED("NiAGDDataStream::NiAGDDataStream", 0x007E1830, ni::StreamConstruct);
NIAGD_REVERSED("NiAdditionalGeometryData::DeletingDestructor", 0x007E1850, ni::AGDDeletingDestructor);
NIAGD_REVERSED("NiAdditionalGeometryData::NiAdditionalGeometryData", 0x007E1870, ni::AGDConstruct);
NIAGD_REVERSED("NiAdditionalGeometryData::CreateObject", 0x007E18E0, ni::AGDCreateObject);
NIAGD_REVERSED("NiAdditionalGeometryData::LoadBinary", 0x007E1900, ni::AGDLoadBinary);
NIAGD_REVERSED("NiAGDDataStream::GetViewerStrings", 0x007E1B10, ni::StreamGetViewerStrings);
NIAGD_REVERSED("NiAGDDataBlock::GetViewerStrings", 0x007E23A0, ni::BlockGetViewerStrings);
NIAGD_REVERSED("NiAdditionalGeometryData::GetViewerStrings", 0x007E2430, ni::AGDGetViewerStrings);
NIAGD_REVERSED("NiStaticTables::InitA", 0x007E27B0, ni::TablesInitA);
NIAGD_REVERSED("NiStaticTables::InitB", 0x007E28C0, ni::TablesInitB);
NIAGD_REVERSED("NiStaticTables::Init", 0x007E29E0, ni::TablesInit);
// (never hooked in the 32-bit game: a body at its address for the 64-bit build)
RE_UNHOOKED_FN("NiStaticTables::Shutdown", 0x007E29F0, &ni::TablesShutdown);
