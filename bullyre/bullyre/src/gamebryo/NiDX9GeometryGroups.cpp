// The DX9 renderer's geometry groups (the vertex buffers geometry's vertex data lives in; the
// group manager, src/gamebryo/NiDX9GeometryGroupManager.cpp, makes and picks them):
//
// NiGeometryGroup (vtable 0x99E3CC, 0xC bytes: +0x4 its object count (Interlocked*), +0x8 the
// device (held)). Its slots: 0 Purge, 1 AddObject(NiScreenTexture*), 2 AddObject(data, skin
// instance, partition), 3 RemoveObject(NiScreenTexture*), 4 RemoveObject(partition), 5
// RemoveObject(data), 6 CreateChip(buffer data, stream), 7 ReleaseChip(buffer data, stream), 8
// IsDynamic, 9 the deleting destructor. A screen texture's renderer data is at +0x1C, a
// partition's at +0x28, a geometry data's at +0x3C (NiGeometryBufferData, 0x54 bytes:
// src/gamebryo/NiDX9IndexBufferManager.cpp).
//
// NiUnsharedGeometryGroup (vtable 0x99E194, 0x1C bytes: +0xC a map of vertex buffer sets by FVF,
// NiTPointerMap with 37 buckets): the geometry's vertices packed into big shared buffers (sets of
// blocks of chips). NiStaticGeometryGroup (the "shared" one, vtable 0x99E1BC, 0xC bytes, one
// instance 0x20E7508): a buffer of its own for each geometry. NiDynamicGeometryGroup (vtable
// 0x99E1E4, 0x50 bytes, one instance 0x20E750C): dynamic buffers written with discard / no
// overwrite, by FVF (+0xC the sets, +0x1C the chips) or for multi-stream geometry by turn (+0x2C
// the sets, +0x3C the chips: NiTPrimitiveArrays; +0x4C the next).
//
// The buffers: NiVBSet (0x34: +0x0 usage, +0x4 the FVF key, +0x8 flags (bit 31 software
// vertex processing), +0xC the block size, +0x10 the device (held), +0x14 / +0x18 usage and pool,
// +0x1C its blocks (NiTPrimitiveArray), +0x2C the blocks with room (a list sorted by free size),
// +0x30 the size under which a block stays off that list), NiVBBlock (0x44: +0x0 its set, +0x4
// its index, +0x8 the vertex buffer, +0xC its size, +0x20 the used end, +0x24 the room after it,
// +0x28 all its room, +0x2C its chips (NiTPrimitiveArray), +0x3C / +0x40 the room list's links),
// NiVBChip (0x20: +0x0 its index in the block, +0x4 the block (or the dynamic set), +0x8 the
// vertex buffer, +0xC its offset, +0x10 lock flags, +0x14 its size, +0x18 / +0x1C the free list's
// links: a pool at 0x20E75B4), NiVBDynamicSet (0x18: +0x0 / +0x4 its key, +0x8 the buffer size,
// +0xC the device (held), +0x10 the buffer, +0x14 the offset written to).
//
// Also NiD3DDefaultShader's bump op set-up (0x8A6460, between them in the exe).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiActorManager.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::c42 {
namespace {

using actor::MapFirst;
using actor::MapNext;
using actor::MapNode;
using actor::PointerMap;
using dx9::Com;
using sys::At;
using sys::Import;
using sys::Virtual;

constexpr uintptr_t kGroupVtable = 0x0099E3CC;            // NiGeometryGroup
constexpr uintptr_t kUnsharedGroupVtable = 0x0099E194;    // NiUnsharedGeometryGroup
constexpr uintptr_t kStaticGroupVtable = 0x0099E1BC;      // NiStaticGeometryGroup
constexpr uintptr_t kDynamicGroupVtable = 0x0099E1E4;     // NiDynamicGeometryGroup
constexpr uintptr_t kBufferDataVtable = 0x0099CEF0;       // NiGeometryBufferData
constexpr uintptr_t kMapBaseVtable = 0x00950760;          // NiTMapBase
constexpr uintptr_t kPointerMapVtable = 0x00965A9C;       // NiTPointerMap
constexpr uintptr_t kPrimitiveArrayVtable = 0x00961988;   // NiTPrimitiveArray<void*>
constexpr uintptr_t kInterlockedExchange = 0x008FC194;

// IDirect3DDevice9::CreateVertexBuffer
constexpr int kCreateVertexBuffer = 26;
constexpr int kRelease = 2;
constexpr int kAddRef = 1;

GAME_VAR(uint8_t*, ms_pkStaticGroup, 0x020E7508);
GAME_VAR(uint8_t*, ms_pkDynamicGroup, 0x020E750C);
GAME_VAR(uint8_t*, ms_pkFreeChips, 0x020E75B4);            // NiVBChip's pool (linked by +0x18)
GAME_VAR(uint32_t, ms_uiChipsPerAllocation, 0x00BCB94C);
GAME_VAR(uint32_t, ms_uiDefaultBlockSize, 0x00BCB944);     // NiVBSet's
GAME_VAR(uint32_t, ms_uiDefaultMinFree, 0x00BCB948);
GAME_VAR(uint32_t, ms_uiMaxTextureSets, 0x00BCB7F8);       // (the texture sets a buffer takes)
GAME_VAR(uint32_t, NiTriShapeData_ClassBit, 0x020E3138);
constexpr uintptr_t NiTriStripsData_RTTI = 0x020E3B30;

GAME_FN(void, __thiscall, NiTMap_RemoveAll, 0x005AC520, void* map);
GAME_FN(uint8_t, __cdecl, IsKindOf, 0x004066F0, uintptr_t rtti, const void* object);
GAME_FN(void, __thiscall, PtrArray_SetSize, 0x0084A110, void* array, uint32_t size);
GAME_FN(void, __thiscall, PtrArray_SetAt, 0x00833C70, void* array, uint32_t index, const re::Ptr32<void>* value);
GAME_FN(uint32_t, __thiscall, PtrArray_AddFirstEmpty, 0x008A4D20, void* array, const re::Ptr32<void>* value);
GAME_FN(void, __thiscall, BufferData_RemoveIB, 0x00896EE0, void* data);
// NiVBSet's (chunk 41's)
GAME_FN(uint8_t*, __thiscall, VBSet_FindBlock, 0x008A4B60, void* set, uint32_t size);
GAME_FN(uint8_t*, __thiscall, VBSet_CreateBlock, 0x008A4F60, void* set, uint32_t size);
GAME_FN(void, __thiscall, VBSet_InsertBlock, 0x008A4BB0, void* set, uint8_t* block);
GAME_FN(void, __thiscall, VBSet_FreeChip, 0x008A4E70, void* set, uint32_t block, uint32_t chip);
GAME_FN(void, __thiscall, VBSet_Destruct, 0x008A4DA0, void* set);
GAME_FN(void, __cdecl, VBSet_SetPool, 0x008A4A90, uint8_t* set);
// this file's (called where modes apply)
GAME_FN(uint8_t*, __thiscall, VBSet_AllocateChip, 0x008A5030, void* set, uint32_t size, int forceNewBlock);
GAME_FN(void, __thiscall, Map_SetAt, 0x008A5140, void* map, uint32_t key, void* value);
GAME_FN(uint8_t, __thiscall, Map_RemoveAt, 0x008A5A10, void* map, uint32_t key);
GAME_FN(uint8_t*, __thiscall, BufferData_Construct, 0x008A51B0, void* self);
GAME_FN(void, __thiscall, UnsharedGroup_Purge, 0x008A5200, void* self);
GAME_FN(uint8_t*, __thiscall, UnsharedGroup_Construct, 0x008A5290, void* self);
GAME_FN(uint8_t*, __thiscall, VBSet_Construct, 0x008A5320, void* self);
GAME_FN(uint8_t*, __cdecl, VBSet_Create, 0x008A5390, uint32_t blockSize, void* device, uint32_t fvf, uint32_t flags,
        uint32_t usage);
GAME_FN(uint8_t*, __cdecl, VBChip_Create, 0x008A5610, void* block, void* vertexBuffer, uint32_t offset,
        uint32_t size, uint32_t index);
GAME_FN(uint8_t*, __cdecl, DynamicSet_Create, 0x008A5A80, uint32_t size, void* device, uint32_t fvf,
        uint32_t index);
GAME_FN(uint8_t*, __thiscall, DynamicGroup_Construct, 0x008A5B10, void* self);
GAME_FN(void, __thiscall, DynamicGroup_Destruct, 0x008A5BD0, void* self);
GAME_FN(void, __cdecl, VBChip_CreateNewObjects, 0x008A75C0);
GAME_FN(uint8_t*, __thiscall, VBBlock_AllocateChip, 0x008A7660, void* block, uint32_t size);
GAME_FN(void, __thiscall, Group_Construct, 0x008A76F0, void* self);
GAME_FN(void, __thiscall, Group_Destruct, 0x008A7710, void* self);
GAME_FN(void, __thiscall, Group_AddObject, 0x008A7730, void* self, uint8_t* data);
GAME_FN(void, __thiscall, Group_RemoveObject, 0x008A7750, void* self, uint8_t* data);
GAME_FN(void*, __thiscall, DynamicSet_AllocateBufferSpace, 0x008A77C0, void* set, uint32_t size, uint32_t* offset,
        uint32_t* lockFlags, int forceDiscard, int softwareVP);

inline PointerMap& MapAt(void* object, int offset) {
    return At<PointerMap>(object, offset);
}

inline void* MapGet(void* object, int offset, uint32_t key, bool* found) {
    PointerMap& map = MapAt(object, offset);
    for (MapNode* node = map.table[key % map.hashSize]; node != nullptr; node = node->next) {
        if (key == node->key) {
            *found = true;
            return node->value;
        }
    }
    *found = false;
    return nullptr;
}

// a map's constructor and destructor as the groups inline them
inline void ConstructMap(uint8_t* map) {
    At<uint32_t>(map, 0) = kMapBaseVtable;
    At<uint32_t>(map, 4) = 37;
    At<uint32_t>(map, 0xC) = 0;
    void* buckets = NiMalloc(0x94);
    At<void*>(map, 8) = buckets;
    crt::Memset(buckets, 0, At<uint32_t>(map, 4) * 4);
    At<uint32_t>(map, 0) = kPointerMapVtable;
}
inline void DestructMap(uint8_t* map) {
    At<uint32_t>(map, 0) = kPointerMapVtable;
    NiTMap_RemoveAll(map);
    At<uint32_t>(map, 0) = kMapBaseVtable;
    NiTMap_RemoveAll(map);
    NiFree(At<void*>(map, 8));
}

// NiTPrimitiveArray<void*>'s constructor (growth 1) and destructor, inlined
inline void ConstructArray(uint8_t* array) {
    At<uint16_t>(array, 8) = 0;
    At<uint16_t>(array, 0xA) = 0;
    At<uint16_t>(array, 0xC) = 0;
    At<void*>(array, 4) = nullptr;
    At<uint32_t>(array, 0) = kPrimitiveArrayVtable;
    At<uint16_t>(array, 0xE) = 1;
}
inline void DestructArray(uint8_t* array) {
    At<uint32_t>(array, 0) = kPrimitiveArrayVtable;
    NiFree(At<void*>(array, 4));
}

// NiTArray::RemoveAt, inlined (the element returned)
inline uint8_t* ArrayRemoveAt(uint8_t* array, uint32_t index) {
    re::Ptr32<uint8_t>* base = At<re::Ptr32<uint8_t>*>(array, 4);
    uint8_t* element = base[index];
    base[index] = nullptr;
    if (element != nullptr) {
        At<uint16_t>(array, 0xC)--;
    }
    uint16_t last = uint16_t(At<uint16_t>(array, 0xA) - 1);
    if (index == last) {
        At<uint16_t>(array, 0xA) = last;
    }
    return element;
}

inline void ComRelease(void* object) {
    if (object != nullptr) {
        Com<long>(object, kRelease);
    }
}

// a dynamic set deleted (its buffer and device let go)
inline void DeleteDynamicSet(uint8_t* set) {
    ComRelease(At<void*>(set, 0x10));
    ComRelease(At<void*>(set, 0xC));
    NiDelete(set, 0x18);
}

// a buffer data let go by its group (the chips released, deleted through its slot 0)
inline void ReleaseBufferData(void* group, re::Ptr32<uint8_t>& data) {
    if (data != nullptr) {
        uint8_t* object = data;
        Group_RemoveObject(group, object);
        Virtual<void*>(object, 0, 1);
        data = nullptr;
    }
}

// a new NiGeometryBufferData (null when the allocation failed: the code then writes through it)
inline uint8_t* NewBufferData() {
    void* memory = NiNew(0x54);
    return memory != nullptr ? BufferData_Construct(memory) : nullptr;
}

// the buffer data's flags from its geometry data (normals, colours, binormals / tangents, the
// texture sets)
inline void SetBufferDataFlags(uint8_t* data, uint8_t* bufferData) {
    bool binormals = At<void*>(data, 0x24) != nullptr && (At<uint16_t>(data, 0x32) & 0xF000) != 0;
    uint32_t textureSets = At<uint8_t>(data, 0x32) & 0x3F;
    if (textureSets > ms_uiMaxTextureSets) {
        textureSets = ms_uiMaxTextureSets;
    }
    uint32_t flags = 0;
    if (At<void*>(data, 0x28) != nullptr) {
        flags = 0x400000;
    }
    if (At<void*>(data, 0x24) != nullptr) {
        flags |= 0x800000;
    }
    if (binormals) {
        flags |= 0x200000;
    }
    At<uint32_t>(bufferData, 4) = textureSets << 24 | flags;
}

// AddObject(data, skin, partition)'s first part: the buffer data made for the partition or the
// geometry data (its primitive type: lists, or strips for strips data); false when it has one
bool MakeBufferData(uint8_t* data, uint8_t* partition, re::Ptr32<uint8_t>& made) {
    uint8_t* bufferData;
    if (partition == nullptr) {
        if (At<void*>(data, 0x3C) != nullptr) {
            return false;
        }
        bufferData = NewBufferData();
        const uint8_t* rtti = Virtual<const uint8_t*>(data, 2);
        if ((At<uint32_t>(rtti, 0xC + int(NiTriShapeData_ClassBit >> 5) * 4) & (1u << (NiTriShapeData_ClassBit & 0x1F))) ==
            0) {
            if (IsKindOf(NiTriStripsData_RTTI, data) != 0) {
                At<uint32_t>(bufferData, 0x3C) = 5;   // D3DPT_TRIANGLESTRIP
            }
        } else {
            At<uint32_t>(bufferData, 0x3C) = 4;   // D3DPT_TRIANGLELIST
        }
        At<uint8_t*>(data, 0x3C) = bufferData;
    } else {
        if (At<void*>(partition, 0x28) != nullptr) {
            return false;
        }
        bufferData = NewBufferData();
        At<uint32_t>(bufferData, 0x3C) = (At<uint16_t>(partition, 0x22) != 0 ? 1u : 0u) + 4;
        At<uint8_t*>(partition, 0x28) = bufferData;
    }
    made = bufferData;
    return true;
}

} // namespace

// ---- NiVBSet / NiTPointerMap<unsigned int, T*>

// 0x8A5030 (thiscall): NiVBSet::AllocateChip (a block with room, else a new one; the block put
// back on the room list when it still has more room than the set's minimum)
uint8_t* __fastcall NiVBSet_AllocateChip(uint8_t* self, re::Edx, uint32_t size, int forceNewBlock) {
    uint8_t* block = nullptr;
    if (!re::Flag(forceNewBlock)) {
        block = VBSet_FindBlock(self, size);
    }
    if (block == nullptr) {
        block = VBSet_CreateBlock(self, size);
        if (block == nullptr) {
            return nullptr;
        }
    }
    uint8_t* chip = VBBlock_AllocateChip(block, size);
    if (At<uint32_t>(block, 0x24) > At<uint32_t>(self, 0x30)) {
        VBSet_InsertBlock(self, block);
    }
    return chip;
}

// 0x8A5140 (thiscall): NiTPointerMap<unsigned int, T*>::SetAt (the value replaced, or a new node
// at the bucket's head: the map's slots 3 NewItem, 1 SetValue)
void __fastcall NiTPointerMap_SetAt(uint8_t* self, re::Edx, uint32_t key, void* value) {
    uint32_t bucket = key % At<uint32_t>(self, 4);
    re::Ptr32<void>* table = At<re::Ptr32<void>*>(self, 8);
    for (auto* node = static_cast<MapNode*>(table[bucket]); node != nullptr; node = node->next) {
        if (key == node->key) {
            node->value = value;
            return;
        }
    }
    auto* node = Virtual<MapNode*>(self, 3);
    Virtual<void>(self, 1, node, key, value);
    node->next = static_cast<MapNode*>(At<re::Ptr32<void>*>(self, 8)[bucket]);
    At<re::Ptr32<void>*>(self, 8)[bucket] = node;
    At<uint32_t>(self, 0xC)++;
}

// 0x8A5A10 (thiscall): NiTPointerMap<unsigned int, T*>::RemoveAt (slots 2 ClearValue, 4
// DeleteItem)
uint8_t __fastcall NiTPointerMap_RemoveAt(uint8_t* self, re::Edx, uint32_t key) {
    re::Ptr32<void>* link = &At<re::Ptr32<void>*>(self, 8)[key % At<uint32_t>(self, 4)];
    auto* node = static_cast<MapNode*>(*link);
    if (node == nullptr) {
        return 0;
    }
    while (key != node->key) {
        link = reinterpret_cast<re::Ptr32<void>*>(&node->next);
        node = node->next;
        if (node == nullptr) {
            return 0;
        }
    }
    *link = node->next;
    Virtual<void>(self, 2, node);
    Virtual<void>(self, 4, node);
    At<uint32_t>(self, 0xC)--;
    return 1;
}

// 0x8A5320 (thiscall): NiVBSet::NiVBSet (the defaults' block size and minimum)
uint8_t* __fastcall NiVBSet_Construct(uint8_t* self, re::Edx) {
    At<uint16_t>(self, 0x24) = 0;
    At<uint16_t>(self, 0x2A) = 1;
    At<uint16_t>(self, 0x26) = 0;
    At<uint16_t>(self, 0x28) = 0;
    At<void*>(self, 0x20) = nullptr;
    At<uint32_t>(self, 0x1C) = kPrimitiveArrayVtable;
    At<uint32_t>(self, 0xC) = ms_uiDefaultBlockSize;
    At<uint32_t>(self, 4) = 0;
    At<uint32_t>(self, 8) = 0;
    At<uint32_t>(self, 0) = 0;
    At<void*>(self, 0x10) = nullptr;
    At<uint32_t>(self, 0x14) = 0;
    At<uint32_t>(self, 0x18) = 0;
    // (the blocks' array emptied, as its RemoveAll inlined)
    for (uint16_t i = 0; i < At<uint16_t>(self, 0x26); i++) {
        At<re::Ptr32<void>*>(self, 0x20)[i] = nullptr;
    }
    At<uint16_t>(self, 0x26) = 0;
    At<uint16_t>(self, 0x28) = 0;
    At<void*>(self, 0x2C) = nullptr;
    At<uint32_t>(self, 0x30) = ms_uiDefaultMinFree;
    return self;
}

// 0x8A5390: NiVBSet::Create (the device held; the pool from the flags)
uint8_t* __cdecl NiVBSet_Create(uint32_t blockSize, void* device, uint32_t fvf, uint32_t flags, uint32_t usage) {
    void* memory = NiNew(0x34);
    if (memory == nullptr) {
        return nullptr;
    }
    uint8_t* set = VBSet_Construct(memory);
    if (set != nullptr) {
        At<uint32_t>(set, 0xC) = blockSize;
        if (device != nullptr) {
            At<void*>(set, 0x10) = device;
            Com<long>(device, kAddRef);
        } else {
            ComRelease(At<void*>(set, 0x10));
            At<void*>(set, 0x10) = nullptr;
        }
        At<uint32_t>(set, 4) = fvf;
        At<uint32_t>(set, 8) = flags;
        At<uint32_t>(set, 0) = usage;
        VBSet_SetPool(set);
    }
    return set;
}

// 0x8A5610: NiVBChip::Create (a chip of its own, not from the pool)
uint8_t* __cdecl NiVBChip_Create(void* block, void* vertexBuffer, uint32_t offset, uint32_t size, uint32_t index) {
    auto* chip = static_cast<uint8_t*>(NiNew(0x20));
    if (chip == nullptr) {
        return nullptr;
    }
    crt::Memset(chip, 0, 0x20);
    At<uint32_t>(chip, 0) = index;
    At<void*>(chip, 4) = block;
    At<void*>(chip, 8) = vertexBuffer;
    At<uint32_t>(chip, 0xC) = offset;
    At<uint32_t>(chip, 0x14) = size;
    At<uint32_t>(chip, 0x10) = 0;
    return chip;
}

// 0x8A5A80: NiVBDynamicSet::Create (the size rounded up past the next 16; quirk kept: +0x14 left
// as the allocator had it)
uint8_t* __cdecl NiVBDynamicSet_Create(uint32_t size, void* device, uint32_t fvf, uint32_t index) {
    auto* set = static_cast<uint8_t*>(NiNew(0x18));
    if (set == nullptr) {
        return nullptr;
    }
    At<uint32_t>(set, 0) = 0;
    At<uint32_t>(set, 4) = 0;
    At<uint32_t>(set, 8) = 0;
    At<void*>(set, 0xC) = nullptr;
    At<void*>(set, 0x10) = nullptr;
    if ((size & 0xF) != 0) {
        size = (size & 0xFFFFFFF0) + 0x20;
    }
    At<uint32_t>(set, 8) = size;
    ComRelease(At<void*>(set, 0xC));
    if (device != nullptr) {
        At<void*>(set, 0xC) = device;
        Com<long>(device, kAddRef);
    } else {
        At<void*>(set, 0xC) = nullptr;
    }
    At<uint32_t>(set, 4) = fvf;
    At<uint32_t>(set, 0) = index;
    return set;
}

// 0x8A77C0 (thiscall): NiVBDynamicSet::AllocateBufferSpace (a bigger buffer made when needed;
// the space appended with no-overwrite while it fits, else the buffer discarded from 0)
void* __fastcall NiVBDynamicSet_AllocateBufferSpace(uint8_t* self, re::Edx, uint32_t size, uint32_t* offset,
                                                    uint32_t* lockFlags, int forceDiscard, int softwareVP) {
    if (size > At<uint32_t>(self, 8)) {
        ComRelease(At<void*>(self, 0x10));
        At<void*>(self, 0x10) = nullptr;
        uint32_t rounded = size;
        if ((size & 0xF) != 0) {
            rounded = (size & 0xFFFFFFF0) + 0x20;
        }
        At<uint32_t>(self, 8) = rounded;
    }
    if (At<void*>(self, 0x10) == nullptr) {
        uint32_t usage = re::Flag(softwareVP) ? 0x218 : 0x208;   // dynamic, write only (software)
        long hr = Com<long>(At<void*>(self, 0xC), kCreateVertexBuffer, At<uint32_t>(self, 8), usage,
                            At<uint32_t>(self, 4), 0u, &At<void*>(self, 0x10), static_cast<void*>(nullptr));
        if (hr >= 0) {
            At<uint32_t>(self, 0x14) = 0;
        }
    }
    if (!re::Flag(forceDiscard) && size <= At<uint32_t>(self, 8) - At<uint32_t>(self, 0x14)) {
        *offset = At<uint32_t>(self, 0x14);
        At<uint32_t>(self, 0x14) += size;
        *lockFlags = 0x1000;   // D3DLOCK_NOOVERWRITE
    } else {
        *offset = 0;
        At<uint32_t>(self, 0x14) = size;
        *lockFlags = 0x2000;   // D3DLOCK_DISCARD
    }
    if (*offset == 0) {
        *lockFlags = 0x2000;
    }
    return At<void*>(self, 0x10);
}

// ---- NiVBBlock / NiVBChip's pool

// 0x8A7500 (thiscall): NiVBBlock::FreeChip (the used end found again when it was the last; the
// chip back to the pool; its size returned)
uint32_t __fastcall NiVBBlock_FreeChip(uint8_t* self, re::Edx, uint32_t index) {
    re::Ptr32<uint8_t>* chips = At<re::Ptr32<uint8_t>*>(self, 0x30);
    uint8_t* chip = chips[index];
    uint32_t size = 0;
    if (chip != nullptr) {
        size = At<uint32_t>(chip, 0x14);
        if (At<uint32_t>(self, 0x20) - size == At<uint32_t>(chip, 0xC)) {
            At<uint32_t>(self, 0x20) = 0;
            re::Ptr32<uint8_t>* other = chips;
            for (uint32_t i = index; i != 0; i--, other++) {
                uint8_t* before = *other;
                if (before != nullptr) {
                    uint32_t end = At<uint32_t>(before, 0x14) + At<uint32_t>(before, 0xC);
                    if (At<uint32_t>(self, 0x20) < end) {
                        At<uint32_t>(self, 0x20) = end;
                    }
                }
            }
        }
        re::Ptr32<void> none = nullptr;
        At<uint32_t>(self, 0x28) += size;
        At<uint32_t>(self, 0x24) = At<uint32_t>(self, 0xC) - At<uint32_t>(self, 0x20);
        PtrArray_SetAt(self + 0x2C, index, &none);
        At<uint8_t*>(chip, 0x1C) = nullptr;
        At<uint8_t*>(chip, 0x18) = ms_pkFreeChips;
        if (ms_pkFreeChips != nullptr) {
            At<uint8_t*>(ms_pkFreeChips, 0x1C) = chip;
        }
        ms_pkFreeChips = chip;
    }
    return size;
}

// 0x8A75A0 (thiscall): NiVBBlock's destructor (its chips' array)
void __fastcall NiVBBlock_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0x2C) = kPrimitiveArrayVtable;
    NiFree(At<void*>(self, 0x30));
}

// 0x8A75C0: NiVBChip's pool grown (that many zeroed chips at the free list's head)
void __cdecl NiVBChip_CreateNewObjects() {
    for (uint32_t i = 0; i < ms_uiChipsPerAllocation; i++) {
        auto* chip = static_cast<uint8_t*>(NiNew(0x20));
        if (chip != nullptr) {
            crt::Memset(chip, 0, 0x20);
        }
        if (ms_pkFreeChips != nullptr) {
            At<uint8_t*>(ms_pkFreeChips, 0x1C) = chip;
        }
        At<uint8_t*>(chip, 0x18) = ms_pkFreeChips;
        At<uint8_t*>(chip, 0x1C) = nullptr;
        ms_pkFreeChips = chip;
    }
}

// 0x8A7630: NiVBChip's pool emptied
void __cdecl NiVBChip_DestroyObjects() {
    uint8_t* chip = ms_pkFreeChips;
    while (chip != nullptr) {
        uint8_t* next = At<uint8_t*>(chip, 0x18);
        At<void*>(chip, 8) = nullptr;
        NiDelete(chip, 0x20);
        chip = next;
    }
    ms_pkFreeChips = nullptr;
}

// 0x8A7660 (thiscall): NiVBBlock::AllocateChip (a pooled chip at the used end; null when there
// is no room after it)
uint8_t* __fastcall NiVBBlock_AllocateChip(uint8_t* self, re::Edx, uint32_t size) {
    if (At<uint32_t>(self, 0x24) < size) {
        return nullptr;
    }
    uint8_t* chip = ms_pkFreeChips;
    if (chip == nullptr) {
        VBChip_CreateNewObjects();
        chip = ms_pkFreeChips;
    }
    uint8_t* next = At<uint8_t*>(chip, 0x18);
    ms_pkFreeChips = next;
    if (next != nullptr) {
        At<uint8_t*>(next, 0x1C) = nullptr;
    }
    At<uint8_t*>(chip, 0x18) = nullptr;
    At<uint8_t*>(chip, 0x1C) = nullptr;
    At<uint8_t*>(chip, 4) = self;
    At<void*>(chip, 8) = At<void*>(self, 8);
    At<uint32_t>(chip, 0xC) = At<uint32_t>(self, 0x20);
    At<uint32_t>(chip, 0x14) = size;
    At<uint32_t>(self, 0x20) += size;
    At<uint32_t>(self, 0x24) -= size;
    At<uint32_t>(self, 0x28) -= size;
    uint8_t* element = chip;
    At<uint32_t>(chip, 0) = PtrArray_AddFirstEmpty(self + 0x2C, re::PtrCast<const re::Ptr32<void>*>(&element));
    return chip;
}

// ---- NiGeometryGroup

// 0x8A76F0 (thiscall): NiGeometryGroup::NiGeometryGroup
uint8_t* __fastcall NiGeometryGroup_Construct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kGroupVtable;
    Import<long>(kInterlockedExchange, &At<uint32_t>(self, 4), 0L);
    At<void*>(self, 8) = nullptr;
    return self;
}

// 0x8A7710 (thiscall): NiGeometryGroup::~NiGeometryGroup (the device let go)
void __fastcall NiGeometryGroup_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kGroupVtable;
    ComRelease(At<void*>(self, 8));
}

// 0x8A7730 (thiscall): NiGeometryGroup::AddObject (the buffer data counted, made the group's)
void __fastcall NiGeometryGroup_AddObject(uint8_t* self, re::Edx, uint8_t* data) {
    sys::Increment(&At<uint32_t>(self, 4));
    At<uint8_t*>(data, 8) = self;
}

// 0x8A7750 (thiscall): NiGeometryGroup::RemoveObject (its chips released through slot 7, its
// index buffer let go)
void __fastcall NiGeometryGroup_RemoveObject(uint8_t* self, re::Edx, uint8_t* data) {
    sys::Decrement(&At<uint32_t>(self, 4));
    uint32_t streams = At<uint32_t>(data, 0x20);
    At<void*>(data, 8) = nullptr;
    for (uint32_t i = 0; i < streams; i++) {
        Virtual<void>(self, 7, data, i);
    }
    BufferData_RemoveIB(data);
}

// 0x8A7790 (thiscall): NiGeometryGroup's deleting destructor
void* __fastcall NiGeometryGroup_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kGroupVtable;
    ComRelease(At<void*>(self, 8));
    if (flags & 1) {
        NiDelete(self, 0xC);
    }
    return self;
}

// ---- NiUnsharedGeometryGroup (slots 3..5 are NiStaticGeometryGroup's too: the same code)

// 0x8A5090 (thiscall): NiUnsharedGeometryGroup::RemoveObject(NiSkinPartition::Partition*)
void __fastcall NiUnsharedGeometryGroup_RemovePartition(uint8_t* self, re::Edx, uint8_t* partition) {
    ReleaseBufferData(self, At<uint8_t*>(partition, 0x28));
}

// 0x8A50C0 (thiscall): NiUnsharedGeometryGroup::RemoveObject(NiGeometryData*)
void __fastcall NiUnsharedGeometryGroup_RemoveData(uint8_t* self, re::Edx, uint8_t* data) {
    ReleaseBufferData(self, At<uint8_t*>(data, 0x3C));
}

// 0x8A55E0 (thiscall): NiUnsharedGeometryGroup::RemoveObject(NiScreenTexture*)
void __fastcall NiUnsharedGeometryGroup_RemoveScreenTexture(uint8_t* self, re::Edx, uint8_t* screenTexture) {
    ReleaseBufferData(self, At<uint8_t*>(screenTexture, 0x1C));
}

// 0x8A50F0 (thiscall): NiUnsharedGeometryGroup::ReleaseChip (its space back to its set)
void __fastcall NiUnsharedGeometryGroup_ReleaseChip(uint8_t*, re::Edx, uint8_t* data, uint32_t stream) {
    if (stream >= At<uint32_t>(data, 0x20)) {
        return;
    }
    uint8_t* chip = At<re::Ptr32<uint8_t>*>(data, 0x28)[stream];
    if (chip == nullptr) {
        return;
    }
    if (At<void*>(chip, 8) != nullptr) {
        uint8_t* block = At<uint8_t*>(chip, 4);
        VBSet_FreeChip(At<void*>(block, 0), At<uint32_t>(block, 4), At<uint32_t>(chip, 0));
    }
    if (stream < At<uint32_t>(data, 0x20)) {
        At<re::Ptr32<uint8_t>*>(data, 0x28)[stream] = nullptr;
    }
}

// 0x8A51B0 (thiscall): NiGeometryBufferData::NiGeometryBufferData (triangle lists, 4 streams'
// growth)
uint8_t* __fastcall NiGeometryBufferData_Construct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kBufferDataVtable;
    for (int offset = 4; offset < 0x54; offset += 4) {
        if (offset == 0x14) {
            At<uint8_t>(self, 0x14) = 0;   // (a byte: the rest left)
        } else {
            At<uint32_t>(self, offset) = offset == 0x3C ? 4u : 0u;
        }
    }
    return self;
}

// 0x8A5200 (thiscall): NiUnsharedGeometryGroup::Purge (every set taken out of the map and
// deleted)
void __fastcall NiUnsharedGeometryGroup_Purge(uint8_t* self, re::Edx) {
    PointerMap& map = MapAt(self, 0xC);
    MapNode* node = MapFirst(map);
    while (node != nullptr) {
        uint32_t key = node->key;
        auto* set = static_cast<uint8_t*>(node->value);
        node = MapNext(map, node);
        Map_RemoveAt(&map, key);
        if (set != nullptr) {
            VBSet_Destruct(set);
            NiDelete(set, 0x34);
        }
    }
}

// 0x8A5290 (thiscall): NiUnsharedGeometryGroup::NiUnsharedGeometryGroup
uint8_t* __fastcall NiUnsharedGeometryGroup_Construct(uint8_t* self, re::Edx) {
    Group_Construct(self);
    At<uint32_t>(self, 0) = kUnsharedGroupVtable;
    ConstructMap(self + 0xC);
    return self;
}

// 0x8A52E0 (thiscall): NiUnsharedGeometryGroup::AddObject(NiScreenTexture*) (quirk kept: the
// buffer data made is counted but never stored in the screen texture)
void __fastcall NiUnsharedGeometryGroup_AddScreenTexture(uint8_t* self, re::Edx, uint8_t* screenTexture) {
    if (At<void*>(screenTexture, 0x1C) != nullptr) {
        return;
    }
    uint8_t* bufferData = NewBufferData();
    At<uint32_t>(bufferData, 0x3C) = 4;
    At<uint32_t>(bufferData, 4) = 0x1400000;   // (one texture set, colours)
    Group_AddObject(self, bufferData);
}

// 0x8A5410 (thiscall): NiUnsharedGeometryGroup's deleting destructor
void* __fastcall NiUnsharedGeometryGroup_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kUnsharedGroupVtable;
    UnsharedGroup_Purge(self);
    DestructMap(self + 0xC);
    Group_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x1C);
    }
    return self;
}

// 0x8A5470: NiUnsharedGeometryGroup::Create
uint8_t* __cdecl NiUnsharedGeometryGroup_Create() {
    void* memory = NiNew(0x1C);
    if (memory != nullptr) {
        return UnsharedGroup_Construct(memory);
    }
    return nullptr;
}

// 0x8A5490 (thiscall): NiUnsharedGeometryGroup::CreateChip (from the set for its FVF, or its
// stride, made when there's none; the chip's first vertex returned in the data's +0x38)
uint8_t* __fastcall NiUnsharedGeometryGroup_CreateChip(uint8_t* self, re::Edx, uint8_t* data, uint32_t stream) {
    uint32_t key = At<uint32_t>(data, 0xC);
    uint32_t stride = stream < At<uint32_t>(data, 0x20) ? At<uint32_t*>(data, 0x24)[stream] : 0;
    if (key == 0) {
        key = stride | 0x80000000;
    }
    uint32_t flags = 0;
    if (At<uint8_t>(data, 0x14) != 0) {
        key |= 0x40000000;
        flags = 0x80000000;
    }
    bool found;
    auto* set = static_cast<uint8_t*>(MapGet(self, 0xC, key, &found));
    if (set == nullptr) {
        set = VBSet_Create(0x10000, At<void*>(self, 8), key, flags, 0);
        Map_SetAt(self + 0xC, key, set);
    }
    uint8_t* chip =
        VBSet_AllocateChip(set, At<uint32_t>(data, 0x1C) * stride, At<uint32_t>(data, 0x20) > 1 ? 1 : 0);
    At<uint32_t>(data, 0x38) = At<uint32_t>(chip, 0xC) / stride;
    return chip;
}

// 0x8A5660 (thiscall): NiUnsharedGeometryGroup::AddObject(data, skin instance, partition) (also
// NiStaticGeometryGroup's)
void __fastcall NiUnsharedGeometryGroup_AddObject(uint8_t* self, re::Edx, uint8_t* data, void*, uint8_t* partition) {
    re::Ptr32<uint8_t> bufferData;
    if (!MakeBufferData(data, partition, bufferData)) {
        return;
    }
    SetBufferDataFlags(data, bufferData);
    Group_AddObject(self, bufferData);
}

// ---- NiStaticGeometryGroup

// 0x8A5560 (thiscall): NiStaticGeometryGroup's deleting destructor (the instance cleared)
void* __fastcall NiStaticGeometryGroup_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kStaticGroupVtable;
    if (self == ms_pkStaticGroup) {
        ms_pkStaticGroup = nullptr;
    }
    Group_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0xC);
    }
    return self;
}

// 0x8A55A0: NiStaticGeometryGroup::Create (the one instance, returned)
uint8_t* __cdecl NiStaticGeometryGroup_Create() {
    if (ms_pkStaticGroup != nullptr) {
        return ms_pkStaticGroup;
    }
    auto* group = static_cast<uint8_t*>(NiNew(0xC));
    if (group != nullptr) {
        Group_Construct(group);
        At<uint32_t>(group, 0) = kStaticGroupVtable;
        ms_pkStaticGroup = group;
        return group;
    }
    ms_pkStaticGroup = nullptr;
    return nullptr;
}

// 0x8A5790 (thiscall): NiStaticGeometryGroup::AddObject(NiScreenTexture*)
void __fastcall NiStaticGeometryGroup_AddScreenTexture(uint8_t* self, re::Edx, uint8_t* screenTexture) {
    if (At<void*>(screenTexture, 0x1C) != nullptr) {
        return;
    }
    uint8_t* bufferData = NewBufferData();
    At<uint32_t>(bufferData, 0x3C) = 4;
    At<uint8_t*>(screenTexture, 0x1C) = bufferData;
    At<uint32_t>(bufferData, 4) = 0x1400000;
    Group_AddObject(self, bufferData);
}

// 0x8A57E0 (thiscall): NiStaticGeometryGroup::CreateChip (a managed buffer of its own; quirk
// kept: the buffer's slot starts as the data pointer, which stays when the device writes nothing)
uint8_t* __fastcall NiStaticGeometryGroup_CreateChip(uint8_t* self, re::Edx, uint8_t* data, uint32_t stream) {
    uint32_t stride = stream < At<uint32_t>(data, 0x20) ? At<uint32_t*>(data, 0x24)[stream] : 0;
    uint32_t size = stride * At<uint32_t>(data, 0x1C);
    uint32_t usage = At<uint8_t>(data, 0x14) != 0 ? 0x18 : 8;
    void* buffer = data;
    Com<long>(At<void*>(self, 8), kCreateVertexBuffer, size, usage, At<uint32_t>(data, 0xC), 1u, &buffer,
              static_cast<void*>(nullptr));
    auto* chip = static_cast<uint8_t*>(NiNew(0x20));
    if (chip == nullptr) {
        return nullptr;
    }
    crt::Memset(chip, 0, 0x20);
    At<void*>(chip, 8) = buffer;
    At<uint32_t>(chip, 0x14) = size;
    return chip;
}

// 0x8A5880 (thiscall): NiStaticGeometryGroup::ReleaseChip (its buffer let go, the chip deleted)
void __fastcall NiStaticGeometryGroup_ReleaseChip(uint8_t*, re::Edx, uint8_t* data, uint32_t stream) {
    if (stream >= At<uint32_t>(data, 0x20)) {
        return;
    }
    uint8_t* chip = At<re::Ptr32<uint8_t>*>(data, 0x28)[stream];
    if (chip == nullptr) {
        return;
    }
    ComRelease(At<void*>(chip, 8));
    if (stream < At<uint32_t>(data, 0x20)) {
        At<re::Ptr32<uint8_t>*>(data, 0x28)[stream] = nullptr;
    }
    At<void*>(chip, 8) = nullptr;
    NiDelete(chip, 0x20);
}

// ---- NiDynamicGeometryGroup

// a dynamic group's buffer data let go (its streams' chips array zeroed first)
inline void ReleaseDynamicBufferData(void* group, re::Ptr32<uint8_t>& data) {
    if (data != nullptr) {
        uint8_t* object = data;
        Group_RemoveObject(group, object);
        void* chips = At<void*>(object, 0x28);
        if (chips != nullptr) {
            crt::Memset(chips, 0, At<uint32_t>(object, 0x20) * 4);
        }
        Virtual<void*>(object, 0, 1);
        data = nullptr;
    }
}

// 0x8A58D0 (thiscall): NiDynamicGeometryGroup::RemoveObject(NiGeometryData*)
void __fastcall NiDynamicGeometryGroup_RemoveData(uint8_t* self, re::Edx, uint8_t* data) {
    ReleaseDynamicBufferData(self, At<uint8_t*>(data, 0x3C));
}

// 0x8A5920 (thiscall): NiDynamicGeometryGroup::RemoveObject(NiSkinPartition::Partition*)
void __fastcall NiDynamicGeometryGroup_RemovePartition(uint8_t* self, re::Edx, uint8_t* partition) {
    ReleaseDynamicBufferData(self, At<uint8_t*>(partition, 0x28));
}

// 0x8A5970 (thiscall): NiDynamicGeometryGroup::RemoveObject(NiScreenTexture*)
void __fastcall NiDynamicGeometryGroup_RemoveScreenTexture(uint8_t* self, re::Edx, uint8_t* screenTexture) {
    ReleaseDynamicBufferData(self, At<uint8_t*>(screenTexture, 0x1C));
}

// 0x8A59C0 (thiscall): NiDynamicGeometryGroup::ReleaseChip (its buffer only forgotten: the chips
// are the group's)
void __fastcall NiDynamicGeometryGroup_ReleaseChip(uint8_t*, re::Edx, uint8_t* data, uint32_t stream) {
    if (stream >= At<uint32_t>(data, 0x20)) {
        return;
    }
    uint8_t* chip = At<re::Ptr32<uint8_t>*>(data, 0x28)[stream];
    if (chip == nullptr) {
        return;
    }
    if (At<void*>(chip, 8) != nullptr) {
        At<void*>(chip, 8) = nullptr;
    }
    if (stream < At<uint32_t>(data, 0x20)) {
        At<re::Ptr32<uint8_t>*>(data, 0x28)[stream] = nullptr;
    }
}

// 0x8A5B10 (thiscall): NiDynamicGeometryGroup::NiDynamicGeometryGroup
uint8_t* __fastcall NiDynamicGeometryGroup_Construct(uint8_t* self, re::Edx) {
    Group_Construct(self);
    At<uint32_t>(self, 0) = kDynamicGroupVtable;
    ConstructMap(self + 0xC);
    ConstructMap(self + 0x1C);
    ConstructArray(self + 0x2C);
    ConstructArray(self + 0x3C);
    At<uint32_t>(self, 0x4C) = 0;
    return self;
}

// 0x8A5BD0 (thiscall): NiDynamicGeometryGroup::~NiDynamicGeometryGroup (its sets and chips
// deleted; quirk kept: a chip slot past the array's end is written through as null)
void __fastcall NiDynamicGeometryGroup_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kDynamicGroupVtable;
    PointerMap& sets = MapAt(self, 0xC);
    for (MapNode* node = MapFirst(sets); node != nullptr;) {
        uint32_t key = node->key;
        auto* set = static_cast<uint8_t*>(node->value);
        node = MapNext(sets, node);
        Map_RemoveAt(&sets, key);
        if (set != nullptr) {
            DeleteDynamicSet(set);
        }
    }
    PointerMap& chips = MapAt(self, 0x1C);
    for (MapNode* node = MapFirst(chips); node != nullptr;) {
        uint32_t key = node->key;
        auto* chip = static_cast<uint8_t*>(node->value);
        node = MapNext(chips, node);
        Map_RemoveAt(&chips, key);
        At<void*>(chip, 8) = nullptr;
        NiDelete(chip, 0x20);
    }
    for (uint32_t i = 0; i < At<uint16_t>(self, 0x34); i++) {
        if (i < At<uint16_t>(self, 0x36)) {
            uint8_t* set = ArrayRemoveAt(self + 0x2C, i);
            if (set != nullptr) {
                DeleteDynamicSet(set);
            }
        }
        uint8_t* chip = i < At<uint16_t>(self, 0x46) ? ArrayRemoveAt(self + 0x3C, i) : nullptr;
        At<void*>(chip, 8) = nullptr;
        NiDelete(chip, 0x20);
    }
    if (self == ms_pkDynamicGroup) {
        ms_pkDynamicGroup = nullptr;
    }
    DestructArray(self + 0x3C);
    DestructArray(self + 0x2C);
    DestructMap(self + 0x1C);
    DestructMap(self + 0xC);
    Group_Destruct(self);
}

// 0x8A5E60 (thiscall): NiDynamicGeometryGroup::Purge (the sets deleted, the chips' buffers
// forgotten; quirk kept: a null chip slot is written through)
void __fastcall NiDynamicGeometryGroup_Purge(uint8_t* self, re::Edx) {
    PointerMap& sets = MapAt(self, 0xC);
    for (MapNode* node = MapFirst(sets); node != nullptr;) {
        uint32_t key = node->key;
        auto* set = static_cast<uint8_t*>(node->value);
        node = MapNext(sets, node);
        Map_RemoveAt(&sets, key);
        if (set != nullptr) {
            DeleteDynamicSet(set);
        }
    }
    PointerMap& chips = MapAt(self, 0x1C);
    for (MapNode* node = MapFirst(chips); node != nullptr; node = MapNext(chips, node)) {
        At<void*>(node->value, 8) = nullptr;
    }
    for (uint32_t i = 0; i < At<uint16_t>(self, 0x34); i++) {
        if (i < At<uint16_t>(self, 0x36)) {
            uint8_t* set = ArrayRemoveAt(self + 0x2C, i);
            if (set != nullptr) {
                DeleteDynamicSet(set);
            }
        }
        At<void*>(At<re::Ptr32<uint8_t>*>(self, 0x40)[i], 8) = nullptr;
    }
}

// 0x8A6020: NiDynamicGeometryGroup::Create (the one instance, returned)
uint8_t* __cdecl NiDynamicGeometryGroup_Create() {
    if (ms_pkDynamicGroup != nullptr) {
        return ms_pkDynamicGroup;
    }
    void* memory = NiNew(0x50);
    if (memory != nullptr) {
        ms_pkDynamicGroup = DynamicGroup_Construct(memory);
        return ms_pkDynamicGroup;
    }
    ms_pkDynamicGroup = nullptr;
    return nullptr;
}

// 0x8A6050 (thiscall): NiDynamicGeometryGroup::AddObject(data, skin instance, partition) (the
// data's consistency flags marked volatile while it's made)
void __fastcall NiDynamicGeometryGroup_AddObject(uint8_t* self, re::Edx, uint8_t* data, void*, uint8_t* partition) {
    At<uint16_t>(data, 0x34) = uint16_t((At<uint16_t>(data, 0x34) & 0xFFF) | 0x8000);
    re::Ptr32<uint8_t> bufferData;
    if (!MakeBufferData(data, partition, bufferData)) {
        return;
    }
    SetBufferDataFlags(data, bufferData);
    Group_AddObject(self, bufferData);
    At<uint16_t>(data, 0x34) |= 0xFFF;
}

// 0x8A61A0 (thiscall): NiDynamicGeometryGroup::AddObject(NiScreenTexture*) (its revision all
// ones)
void __fastcall NiDynamicGeometryGroup_AddScreenTexture(uint8_t* self, re::Edx, uint8_t* screenTexture) {
    if (At<void*>(screenTexture, 0x1C) != nullptr) {
        return;
    }
    uint8_t* bufferData = NewBufferData();
    At<uint32_t>(bufferData, 0x3C) = 4;
    At<uint8_t*>(screenTexture, 0x1C) = bufferData;
    At<uint32_t>(bufferData, 4) = 0x1400000;
    Group_AddObject(self, bufferData);
    At<uint16_t>(screenTexture, 0x18) = 0xFFFF;
}

// 0x8A61F0 (thiscall): NiDynamicGeometryGroup::CreateChip (multi-stream data takes the sets and
// chips by turn, discarding; the rest by FVF (or stride), appending)
uint8_t* __fastcall NiDynamicGeometryGroup_CreateChip(uint8_t* self, re::Edx, uint8_t* data, uint32_t stream) {
    uint32_t fvf = At<uint32_t>(data, 0xC);
    uint32_t streams = At<uint32_t>(data, 0x20);
    uint32_t stride = stream < streams ? At<uint32_t*>(data, 0x24)[stream] : 0;
    uint32_t size = At<uint32_t>(data, 0x1C) * stride;
    uint32_t offset = 0;
    uint32_t lockFlags = 0;
    if (streams > 1) {
        if (At<uint16_t>(self, 0x34) < streams) {
            PtrArray_SetSize(self + 0x2C, streams);
            PtrArray_SetSize(self + 0x3C, streams);
        }
        if (At<uint32_t>(self, 0x4C) >= At<uint16_t>(self, 0x34)) {
            At<uint32_t>(self, 0x4C) = 0;
        }
        uint8_t* set = At<re::Ptr32<uint8_t>*>(self, 0x30)[At<uint32_t>(self, 0x4C)];
        if (set == nullptr) {
            set = DynamicSet_Create(0x800, At<void*>(self, 8), 0, 0);
            uint8_t* element = set;
            PtrArray_SetAt(self + 0x2C, At<uint32_t>(self, 0x4C), re::PtrCast<const re::Ptr32<void>*>(&element));
        }
        void* buffer =
            DynamicSet_AllocateBufferSpace(set, size, &offset, &lockFlags, 1, At<uint8_t>(data, 0x14));
        uint8_t* chip = At<re::Ptr32<uint8_t>*>(self, 0x40)[At<uint32_t>(self, 0x4C)];
        if (chip == nullptr) {
            chip = VBChip_Create(nullptr, buffer, offset, size, 0);
            uint8_t* element = chip;
            PtrArray_SetAt(self + 0x3C, At<uint32_t>(self, 0x4C), re::PtrCast<const re::Ptr32<void>*>(&element));
        } else {
            At<void*>(chip, 8) = buffer;
            At<uint32_t>(chip, 0xC) = offset;
            At<uint32_t>(chip, 0x14) = size;
        }
        At<uint8_t*>(chip, 4) = set;
        At<uint32_t>(chip, 0x10) = lockFlags;
        At<uint32_t>(data, 0x38) = 0;
        At<uint32_t>(self, 0x4C)++;
        return chip;
    }
    uint32_t key = fvf != 0 ? fvf : (stride | 0x80000000);
    bool found;
    auto* set = static_cast<uint8_t*>(MapGet(self, 0xC, key, &found));
    if (set == nullptr) {
        set = DynamicSet_Create(0x800, At<void*>(self, 8), fvf, 0);
        Map_SetAt(self + 0xC, key, set);
    }
    void* buffer = DynamicSet_AllocateBufferSpace(set, size, &offset, &lockFlags, 0, At<uint8_t>(data, 0x14));
    auto* chip = static_cast<uint8_t*>(MapGet(self, 0x1C, key, &found));
    if (chip == nullptr) {
        chip = VBChip_Create(nullptr, buffer, offset, size, 0);
        Map_SetAt(self + 0x1C, key, chip);
    } else {
        At<void*>(chip, 8) = buffer;
        At<uint32_t>(chip, 0xC) = offset;
        At<uint32_t>(chip, 0x14) = size;
    }
    At<uint8_t*>(chip, 4) = set;
    At<uint32_t>(chip, 0x10) = lockFlags;
    At<uint32_t>(data, 0x38) = offset / stride;
    return chip;
}

// 0x8A6440 (thiscall): NiDynamicGeometryGroup's deleting destructor
void* __fastcall NiDynamicGeometryGroup_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    DynamicGroup_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x50);
    }
    return self;
}

// ---- NiD3DDefaultShader

// 0x8A6460 (thiscall): NiD3DDefaultShader's bump op from the renderer's caps (TextureOpCaps:
// bit 12 at +0xD4; +0xD8 D3DTOP_BUMPENVMAPLUMINANCE (0x17) or D3DTOP_BUMPENVMAP (0x16) when
// supported, else left)
void __fastcall NiD3DDefaultShader_InitializeBumpOp(uint8_t* self, re::Edx) {
    uint8_t* renderer = At<uint8_t*>(self, 0x18);
    At<uint8_t>(self, 0xD4) = uint8_t((At<uint32_t>(renderer, 0x138) >> 12) & 1);
    uint32_t caps = At<uint32_t>(renderer, 0x138);
    if ((caps & 0x400000) != 0) {
        At<uint32_t>(self, 0xD8) = 0x17;
        return;
    }
    if ((caps & 0x200000) != 0) {
        At<uint32_t>(self, 0xD8) = 0x16;
    }
}

#define C42A_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C42A_REVERSED("NiVBSet::AllocateChip", 0x008A5030, NiVBSet_AllocateChip);
C42A_REVERSED("NiUnsharedGeometryGroup::RemoveObject_Partition", 0x008A5090, NiUnsharedGeometryGroup_RemovePartition);
C42A_REVERSED("NiUnsharedGeometryGroup::RemoveObject_Data", 0x008A50C0, NiUnsharedGeometryGroup_RemoveData);
C42A_REVERSED("NiUnsharedGeometryGroup::ReleaseChip", 0x008A50F0, NiUnsharedGeometryGroup_ReleaseChip);
C42A_REVERSED("NiTPointerMap<unsigned int,T*>::SetAt_8A5140", 0x008A5140, NiTPointerMap_SetAt);
C42A_REVERSED("NiGeometryBufferData::NiGeometryBufferData", 0x008A51B0, NiGeometryBufferData_Construct);
C42A_REVERSED("NiUnsharedGeometryGroup::Purge", 0x008A5200, NiUnsharedGeometryGroup_Purge);
C42A_REVERSED("NiUnsharedGeometryGroup::NiUnsharedGeometryGroup", 0x008A5290, NiUnsharedGeometryGroup_Construct);
C42A_REVERSED("NiUnsharedGeometryGroup::AddObject_ScreenTexture", 0x008A52E0, NiUnsharedGeometryGroup_AddScreenTexture);
C42A_REVERSED("NiVBSet::NiVBSet", 0x008A5320, NiVBSet_Construct);
C42A_REVERSED("NiVBSet::Create", 0x008A5390, NiVBSet_Create);
C42A_REVERSED("NiUnsharedGeometryGroup::DeletingDestructor", 0x008A5410, NiUnsharedGeometryGroup_DeletingDestructor);
C42A_REVERSED("NiUnsharedGeometryGroup::Create", 0x008A5470, NiUnsharedGeometryGroup_Create);
C42A_REVERSED("NiUnsharedGeometryGroup::CreateChip", 0x008A5490, NiUnsharedGeometryGroup_CreateChip);
C42A_REVERSED("NiStaticGeometryGroup::DeletingDestructor", 0x008A5560, NiStaticGeometryGroup_DeletingDestructor);
C42A_REVERSED("NiStaticGeometryGroup::Create", 0x008A55A0, NiStaticGeometryGroup_Create);
C42A_REVERSED("NiUnsharedGeometryGroup::RemoveObject_ScreenTexture", 0x008A55E0,
              NiUnsharedGeometryGroup_RemoveScreenTexture);
C42A_REVERSED("NiVBChip::Create", 0x008A5610, NiVBChip_Create);
C42A_REVERSED("NiUnsharedGeometryGroup::AddObject", 0x008A5660, NiUnsharedGeometryGroup_AddObject);
C42A_REVERSED("NiStaticGeometryGroup::AddObject_ScreenTexture", 0x008A5790, NiStaticGeometryGroup_AddScreenTexture);
C42A_REVERSED("NiStaticGeometryGroup::CreateChip", 0x008A57E0, NiStaticGeometryGroup_CreateChip);
C42A_REVERSED("NiStaticGeometryGroup::ReleaseChip", 0x008A5880, NiStaticGeometryGroup_ReleaseChip);
C42A_REVERSED("NiDynamicGeometryGroup::RemoveObject_Data", 0x008A58D0, NiDynamicGeometryGroup_RemoveData);
C42A_REVERSED("NiDynamicGeometryGroup::RemoveObject_Partition", 0x008A5920, NiDynamicGeometryGroup_RemovePartition);
C42A_REVERSED("NiDynamicGeometryGroup::RemoveObject_ScreenTexture", 0x008A5970,
              NiDynamicGeometryGroup_RemoveScreenTexture);
C42A_REVERSED("NiDynamicGeometryGroup::ReleaseChip", 0x008A59C0, NiDynamicGeometryGroup_ReleaseChip);
C42A_REVERSED("NiTPointerMap<unsigned int,T*>::RemoveAt_8A5A10", 0x008A5A10, NiTPointerMap_RemoveAt);
C42A_REVERSED("NiVBDynamicSet::Create", 0x008A5A80, NiVBDynamicSet_Create);
C42A_REVERSED("NiDynamicGeometryGroup::NiDynamicGeometryGroup", 0x008A5B10, NiDynamicGeometryGroup_Construct);
C42A_REVERSED("NiDynamicGeometryGroup::~NiDynamicGeometryGroup", 0x008A5BD0, NiDynamicGeometryGroup_Destruct);
C42A_REVERSED("NiDynamicGeometryGroup::Purge", 0x008A5E60, NiDynamicGeometryGroup_Purge);
C42A_REVERSED("NiDynamicGeometryGroup::Create", 0x008A6020, NiDynamicGeometryGroup_Create);
C42A_REVERSED("NiDynamicGeometryGroup::AddObject", 0x008A6050, NiDynamicGeometryGroup_AddObject);
C42A_REVERSED("NiDynamicGeometryGroup::AddObject_ScreenTexture", 0x008A61A0, NiDynamicGeometryGroup_AddScreenTexture);
C42A_REVERSED("NiDynamicGeometryGroup::CreateChip", 0x008A61F0, NiDynamicGeometryGroup_CreateChip);
C42A_REVERSED("NiDynamicGeometryGroup::DeletingDestructor", 0x008A6440, NiDynamicGeometryGroup_DeletingDestructor);
C42A_REVERSED("NiD3DDefaultShader::InitializeBumpOp", 0x008A6460, NiD3DDefaultShader_InitializeBumpOp);
C42A_REVERSED("NiVBBlock::FreeChip", 0x008A7500, NiVBBlock_FreeChip);
C42A_REVERSED("NiVBBlock::~NiVBBlock", 0x008A75A0, NiVBBlock_Destruct);
C42A_REVERSED("NiVBChip::CreateNewObjects", 0x008A75C0, NiVBChip_CreateNewObjects);
C42A_REVERSED("NiVBChip::DestroyObjects", 0x008A7630, NiVBChip_DestroyObjects);
C42A_REVERSED("NiVBBlock::AllocateChip", 0x008A7660, NiVBBlock_AllocateChip);
C42A_REVERSED("NiGeometryGroup::NiGeometryGroup", 0x008A76F0, NiGeometryGroup_Construct);
C42A_REVERSED("NiGeometryGroup::~NiGeometryGroup", 0x008A7710, NiGeometryGroup_Destruct);
C42A_REVERSED("NiGeometryGroup::AddObject", 0x008A7730, NiGeometryGroup_AddObject);
C42A_REVERSED("NiGeometryGroup::RemoveObject", 0x008A7750, NiGeometryGroup_RemoveObject);
C42A_REVERSED("NiGeometryGroup::DeletingDestructor", 0x008A7790, NiGeometryGroup_DeletingDestructor);
C42A_REVERSED("NiVBDynamicSet::AllocateBufferSpace", 0x008A77C0, NiVBDynamicSet_AllocateBufferSpace);

} // namespace ni::c42
