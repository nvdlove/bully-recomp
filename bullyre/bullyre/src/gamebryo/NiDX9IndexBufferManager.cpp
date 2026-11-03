// NiDX9IndexBufferManager (vtable 0x99D06C, 0x4C bytes; an NiRefObject): +0x8 the device (held),
// +0xC / +0x10 the temporary point connectivity index buffer and its size, +0x14 / +0x18 the line
// one's, +0x1C / +0x2C / +0x3C three maps (NiTPointerMap, 37 buckets) of buffer sets (0x1C: five
// 0x38-byte buffer records from +0x8, each holding a D3D index buffer at +0x20). Its index buffers
// are created, packed (16-bit indices), the quads' and lines' connectivity built.
//
// Also NiGeometryBufferData (vtable 0x99CEF0 on an abstract base 0x99CEE8; 0x54 bytes): a
// geometry's renderer data: +0x4 its flags (bit 21 binormals / tangents, 22 normals, 23 colours;
// byte +0x7 the texture sets), +0x8 its geometry group, +0xC its FVF, +0x10 its vertex declaration
// (held), +0x20 its stream count, +0x24 / +0x28 the streams' arrays, +0x30 / +0x34 its index count
// and index buffer. And the NiTPointerMap::GetNext copy at 0x8976C0 (the game's code uses it).

#include "gamebryo/NiDX9RendererData.h"
#include "reversed.h"

namespace ni::c40 {
namespace {

constexpr uintptr_t kRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t kBufferDataBaseVtable = 0x0099CEE8;
constexpr uintptr_t kBufferDataVtable = 0x0099CEF0;
constexpr uintptr_t kIBManagerVtable = 0x0099D06C;
constexpr uintptr_t kMapBaseVtable = 0x00950760;
constexpr uintptr_t kPointerMapVtable = 0x00965A9C;
constexpr uint32_t D3DFMT_INDEX16 = 0x65;
constexpr uint32_t D3DRTYPE_INDEXBUFFER = 7;

GAME_VAR(uint32_t, ms_uiObjects, 0x020E2168);

// IDirect3DDevice9 / IDirect3DIndexBuffer9 slots
enum : int {
    kAddRef = 1,
    kRelease = 2,
    kCreateIndexBuffer = 27,
    kSetIndices = 104,
    kLock = 11,
    kUnlock = 12,
    kGetDesc = 13,
};

GAME_FN(void*, __cdecl, OperatorDelete, 0x005EEAB0, void* p);   // the game's operator delete
GAME_FN(void, __thiscall, PointerMap_RemoveAll, 0x005AC520, void* map);
GAME_FN(void, __cdecl, NiMemcpy, 0x0074D2A0, void* to, const void* from, uint32_t bytes);
GAME_FN(void, __thiscall, BufferRecord_Destruct, 0x008A2C50, void* self);
// this file's (called where modes apply)
GAME_FN(void*, __thiscall, BufferData_Destruct, 0x008970F0, void* self);
GAME_FN(void*, __thiscall, IBManager_CreateIndexBuffer, 0x00897190, void* self, uint32_t bytes, uint32_t usage,
        uint32_t format, uint32_t pool, void* desc);
GAME_FN(void*, __thiscall, PointerMap_GetNext, 0x008976C0, void* map, re::Ptr32<void>* position, uint32_t* key, re::Ptr32<void>* value);
GAME_FN(void, __thiscall, IBManager_Destruct, 0x00897840, void* self);

// D3DINDEXBUFFER_DESC
struct IndexBufferDesc {
    uint32_t format;
    uint32_t type;
    uint32_t usage;
    uint32_t pool;
    uint32_t size;
};

// an index buffer that can be used again (16-bit, this usage and pool, big enough)
inline bool Reusable(void* buffer, uint32_t usage, uint32_t pool, uint32_t bytes, bool* described) {
    IndexBufferDesc desc = {};
    *described = Com<long>(buffer, kGetDesc, &desc) >= 0;
    return *described && desc.format == D3DFMT_INDEX16 && desc.type == D3DRTYPE_INDEXBUFFER && desc.usage == usage &&
           desc.pool == pool && desc.size >= bytes;
}

// a buffer set (a map's value) deleted
inline void DeleteBufferSet(uint8_t* set) {
    for (int i = 0; i < 5; i++) {
        uint8_t* record = At<uint8_t*>(set, 8 + i * 4);
        if (record != nullptr) {
            void* buffer = At<void*>(record, 0x20);
            if (buffer != nullptr) {
                Com<long>(buffer, kRelease);
            }
            BufferRecord_Destruct(record);
            NiDelete(record, 0x38);
        }
    }
    NiDelete(set, 0x1C);
}

// a map's destructor part (NiTPointerMap's then NiTMapBase's RemoveAll, the buckets freed)
inline void DestructMap(uint8_t* map) {
    At<uint32_t>(map, 0) = kPointerMapVtable;
    PointerMap_RemoveAll(map);
    At<uint32_t>(map, 0) = kMapBaseVtable;
    PointerMap_RemoveAll(map);
    NiFree(At<void*>(map, 8));
}
inline void ConstructMap(uint8_t* map) {
    At<uint32_t>(map, 0) = kMapBaseVtable;
    At<uint32_t>(map, 4) = 37;
    At<uint32_t>(map, 0xC) = 0;
    void* buckets = NiMalloc(0x94);
    At<void*>(map, 8) = buckets;
    crt::Memset(buckets, 0, At<uint32_t>(map, 4) * 4);
    At<uint32_t>(map, 0) = kPointerMapVtable;
}

} // namespace

// 0x896E80 (thiscall): NiGeometryBufferData's base deleting destructor (4 bytes)
void* __fastcall NiGeometryBufferDataBase_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = kBufferDataBaseVtable;
    if (flags & 1) {
        NiDelete(self, 4);
    }
    return self;
}

// 0x896EB0 (thiscall): NiGeometryBufferData::IsVBValid (every stream with a vertex buffer chip
// holding a buffer)
uint8_t __fastcall NiGeometryBufferData_IsVBValid(uint8_t* self, re::Edx) {
    uint32_t count = At<uint32_t>(self, 0x20);
    if (count == 0) {
        return 0;
    }
    re::Ptr32<uint8_t>* chips = At<re::Ptr32<uint8_t>*>(self, 0x28);
    for (uint32_t i = 0; i < count; i++) {
        if (chips[i] == nullptr || At<void*>(chips[i], 8) == nullptr) {
            return 0;
        }
    }
    return 1;
}

// 0x896EE0 (thiscall): NiGeometryBufferData::RemoveIB
void __fastcall NiGeometryBufferData_RemoveIB(uint8_t* self, re::Edx) {
    void* buffer = At<void*>(self, 0x34);
    if (buffer != nullptr) {
        Com<long>(buffer, kRelease);
        At<void*>(self, 0x34) = nullptr;
    }
    At<uint32_t>(self, 0x30) = 0;
}

// 0x896F10 (thiscall): NiGeometryBufferData::SetStreamCount (the arrays made again: the streams
// dropped taken out of the group, the new ones zeroed. Quirk kept: the chips' old array goes back
// through the game's operator delete, not NiFree)
void __fastcall NiGeometryBufferData_SetStreamCount(uint8_t* self, re::Edx, uint32_t count) {
    uint32_t current = At<uint32_t>(self, 0x20);
    if (count == current) {
        return;
    }
    uint32_t bytes = count * 4;
    void* chips = NiMalloc(bytes);
    void* streams = NiMalloc(bytes);
    if (count < At<uint32_t>(self, 0x20)) {
        crt::MemcpyS(chips, bytes, At<void*>(self, 0x28), bytes);
        crt::MemcpyS(streams, bytes, At<void*>(self, 0x24), bytes);
        if (At<void*>(self, 8) != nullptr) {
            for (uint32_t i = count; i < At<uint32_t>(self, 0x20); i++) {
                Virtual<void>(At<void*>(self, 8), 7, self, i);   // the group's RemoveObject
            }
        }
    } else {
        uint32_t old = At<uint32_t>(self, 0x20) * 4;
        crt::MemcpyS(chips, old, At<void*>(self, 0x28), old);
        old = At<uint32_t>(self, 0x20) * 4;
        crt::MemcpyS(streams, old, At<void*>(self, 0x24), old);
        crt::Memset(static_cast<uint8_t*>(chips) + At<uint32_t>(self, 0x20) * 4, 0, (count - At<uint32_t>(self, 0x20)) * 4);
        crt::Memset(static_cast<uint8_t*>(streams) + At<uint32_t>(self, 0x20) * 4, 0,
                    (count - At<uint32_t>(self, 0x20)) * 4);
    }
    OperatorDelete(At<void*>(self, 0x28));
    NiFree(At<void*>(self, 0x24));
    At<void*>(self, 0x28) = chips;
    At<void*>(self, 0x24) = streams;
    At<uint32_t>(self, 0x20) = count;
}

// 0x897010 (thiscall): NiGeometryBufferData::SetFVF (the declaration let go)
void __fastcall NiGeometryBufferData_SetFVF(uint8_t* self, re::Edx, uint32_t fvf) {
    At<uint32_t>(self, 0xC) = fvf;
    void* declaration = At<void*>(self, 0x10);
    if (declaration != nullptr) {
        Com<long>(declaration, kRelease);
        At<void*>(self, 0x10) = nullptr;
    }
}

// 0x897040 (thiscall): NiGeometryBufferData::SetVertexDeclaration (held; the FVF cleared)
void __fastcall NiGeometryBufferData_SetVertexDeclaration(uint8_t* self, re::Edx, void* declaration) {
    void* current = At<void*>(self, 0x10);
    if (declaration != current) {
        if (current != nullptr) {
            Com<long>(current, kRelease);
        }
        At<void*>(self, 0x10) = declaration;
        if (declaration != nullptr) {
            Com<long>(declaration, kAddRef);
        }
    }
    At<uint32_t>(self, 0xC) = 0;
}

// 0x897080 (thiscall): NiGeometryBufferData::ContainsVertexData (by semantic: 3 colours, 4 normals,
// 5..12 a texture set, 13/14 binormals / tangents; the result in al, eax's upper bits as the
// original leaves them)
uint32_t __fastcall NiGeometryBufferData_ContainsVertexData(uint8_t* self, re::Edx, uint32_t semantic) {
    uint32_t flags = At<uint32_t>(self, 4);
    switch (semantic) {
    case 3:
        return (flags >> 23) & 0xFFFFFF01;
    case 4:
        return (flags >> 22) & 0xFFFFFF01;
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
        return semantic - 5 < At<uint8_t>(self, 7) ? 1 : 0;
    case 13:
    case 14:
        return (flags >> 21) & 0xFFFFFF01;
    default:
        return (semantic - 3) & 0xFFFFFF00;
    }
}

// 0x8970F0 (thiscall): NiGeometryBufferData's destructor (its streams out of the group, the index
// buffer, the arrays, the declaration)
void* __fastcall NiGeometryBufferData_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kBufferDataVtable;
    if (At<void*>(self, 8) != nullptr) {
        for (uint32_t i = 0; i < At<uint32_t>(self, 0x20); i++) {
            Virtual<void>(At<void*>(self, 8), 7, self, i);
        }
    }
    void* buffer = At<void*>(self, 0x34);
    if (buffer != nullptr) {
        Com<long>(buffer, kRelease);
        At<void*>(self, 0x34) = nullptr;
    }
    At<uint32_t>(self, 0x30) = 0;
    NiFree(At<void*>(self, 0x28));
    NiFree(At<void*>(self, 0x24));
    void* declaration = At<void*>(self, 0x10);
    if (declaration != nullptr) {
        Com<long>(declaration, kRelease);
    }
    At<uint32_t>(self, 0) = kBufferDataBaseVtable;
    return self;
}

// 0x897170 (thiscall): NiGeometryBufferData's deleting destructor
void* __fastcall NiGeometryBufferData_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    BufferData_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x54);
    }
    return self;
}

// 0x897190 (thiscall): NiDX9IndexBufferManager::CreateIndexBuffer (its description given back
// when asked)
void* __fastcall NiDX9IndexBufferManager_CreateIndexBuffer(uint8_t* self, re::Edx, uint32_t bytes, uint32_t usage,
                                                           uint32_t format, uint32_t pool, void* desc) {
    void* buffer = nullptr;
    if (Com<long>(At<void*>(self, 8), kCreateIndexBuffer, bytes, usage, format, pool, &buffer,
                  static_cast<void*>(nullptr)) < 0) {
        ni::dx9::NiOutputDebugString("NiDX9IndexBufferManager::CreateIndexBuffer FAILED");
        return nullptr;
    }
    if (desc != nullptr) {
        Com<long>(buffer, kGetDesc, desc);
    }
    return buffer;
}

// 0x897200 (thiscall): NiDX9IndexBufferManager::PackBuffer (the indices into the buffer given when
// it can take them, else into a new one (its size returned). Quirk kept: when the lock fails the
// buffer is let go but still returned)
void* __fastcall NiDX9IndexBufferManager_PackBuffer(uint8_t* self, re::Edx, const uint16_t* indices, uint32_t count,
                                                    uint32_t maxCount, void* buffer, uint32_t* size, uint32_t pool,
                                                    uint32_t usage) {
    if (At<void*>(self, 8) == nullptr || indices == nullptr) {
        return nullptr;
    }
    uint32_t maxBytes = maxCount * 2;
    uint32_t bytes = count * 2;
    bool described;
    if (buffer == nullptr || !Reusable(buffer, usage, pool, maxBytes, &described)) {
        void* created = nullptr;
        if (Com<long>(At<void*>(self, 8), kCreateIndexBuffer, maxBytes, usage, D3DFMT_INDEX16, pool, &created,
                      static_cast<void*>(nullptr)) < 0) {
            ni::dx9::NiOutputDebugString("NiDX9IndexBufferManager::CreateIndexBuffer FAILED");
            created = nullptr;
        }
        buffer = created;
        if (buffer == nullptr) {
            ni::dx9::NiOutputDebugString("NiDX9IndexBufferManager::PackBuffer> Failed to create index buffer\n");
            return nullptr;
        }
        *size = maxBytes;
    }
    if (bytes != 0) {
        void* data = nullptr;
        if (Com<long>(buffer, kLock, 0u, bytes, &data, 0u) < 0) {
            ni::dx9::NiOutputDebugString("NiDX9IndexBufferManager::PackBuffer> Failed to lock index buffer\n");
            Com<long>(buffer, kUnlock);
            Com<long>(buffer, kRelease);
            return buffer;
        }
        NiMemcpy(data, indices, bytes);
        Com<long>(buffer, kUnlock);
    }
    return buffer;
}

// 0x897360 (thiscall): NiDX9IndexBufferManager::ResizeTempPointConnectivity (two triangles per
// quad, 0 1 2 0 2 3; the buffer kept when it fits (refilled when asked). Quirk kept: a buffer let
// go for being unusable stays at +0xC when the new one can't be made)
void* __fastcall NiDX9IndexBufferManager_ResizeTempPointConnectivity(uint8_t* self, re::Edx, uint32_t quads,
                                                                     int refill, uint32_t usage, uint32_t pool) {
    if (At<void*>(self, 8) == nullptr || quads == 0) {
        return nullptr;
    }
    uint32_t bytes = quads * 12;
    void* buffer = At<void*>(self, 0xC);
    bool reuse = false;
    if (buffer != nullptr) {
        bool described;
        if (Reusable(buffer, usage, pool, bytes, &described)) {
            if (uint8_t(refill) == 0) {
                return buffer;
            }
            reuse = true;
        } else if (described) {
            Com<long>(buffer, kRelease);
        }
    }
    if (!reuse) {
        buffer = nullptr;
        if (Com<long>(At<void*>(self, 8), kCreateIndexBuffer, bytes, usage, D3DFMT_INDEX16, pool, &buffer,
                      static_cast<void*>(nullptr)) < 0) {
            ni::dx9::NiOutputDebugString("NiDX9IndexBufferManager::CreateIndexBuffer FAILED");
            buffer = nullptr;
        }
        if (buffer == nullptr) {
            ni::dx9::NiOutputDebugString(
                "NiDX9IndexBufferManager::ResizeTempPointConnectivity> Failed to create index buffer\n");
            return nullptr;
        }
    }
    uint16_t* data = nullptr;
    if (Com<long>(buffer, kLock, 0u, bytes, &data, 0u) < 0) {
        ni::dx9::NiOutputDebugString("NiDX9IndexBufferManager::PackBuffer> Failed to lock index buffer\n");
        Com<long>(buffer, kUnlock);
        Com<long>(buffer, kRelease);
        buffer = nullptr;
    } else {
        uint32_t base = 0;
        for (uint32_t i = quads; i != 0; i--) {
            uint16_t v = uint16_t(base);
            data[0] = v;
            data[1] = uint16_t(v + 1);
            data[2] = uint16_t(v + 2);
            data[3] = v;
            data[4] = uint16_t(v + 2);
            data[5] = uint16_t(v + 3);
            data += 6;
            base += 4;
        }
        Com<long>(buffer, kUnlock);
    }
    At<uint32_t>(self, 0x10) = bytes;
    At<void*>(self, 0xC) = buffer;
    return buffer;
}

// 0x897510 (thiscall): NiDX9IndexBufferManager::ResizeTempLineConnectivity (a segment from each
// vertex whose flag is set to the next, the last one's back to the first; the index count
// returned)
void* __fastcall NiDX9IndexBufferManager_ResizeTempLineConnectivity(uint8_t* self, re::Edx, uint32_t vertices,
                                                                    const uint8_t* flags, uint32_t* indexCount,
                                                                    uint32_t usage, uint32_t pool) {
    if (At<void*>(self, 8) == nullptr) {
        return nullptr;
    }
    if (vertices == 0 || flags == nullptr) {
        return nullptr;
    }
    uint32_t bytes = vertices * 4 + 4;
    void* buffer = At<void*>(self, 0x14);
    bool reuse = false;
    if (buffer != nullptr) {
        bool described;
        if (Reusable(buffer, usage, pool, bytes, &described)) {
            reuse = true;
        } else if (described) {
            Com<long>(buffer, kRelease);
        }
    }
    if (!reuse) {
        buffer = IBManager_CreateIndexBuffer(self, bytes, usage, D3DFMT_INDEX16, pool, nullptr);
        if (buffer == nullptr) {
            ni::dx9::NiOutputDebugString(
                "NiDX9IndexBufferManager::ResizeTempLineConnectivity> Failed to create index buffer\n");
            return nullptr;
        }
    }
    *indexCount = 0;
    uint16_t* data = nullptr;
    if (Com<long>(buffer, kLock, 0u, bytes, &data, 0u) < 0) {
        ni::dx9::NiOutputDebugString("NiDX9IndexBufferManager::PackBuffer> Failed to lock index buffer\n");
        Com<long>(buffer, kUnlock);
        Com<long>(buffer, kRelease);
        buffer = nullptr;
    } else {
        uint32_t last = vertices - 1;
        for (uint32_t i = 0; i < last; i++, flags++) {
            if (*flags != 0) {
                data[0] = uint16_t(i);
                data[1] = uint16_t(i + 1);
                data += 2;
                *indexCount += 2;
            }
        }
        if (*flags != 0) {
            data[0] = uint16_t(last);
            data[1] = 0;
            *indexCount += 2;
        }
        Com<long>(buffer, kUnlock);
    }
    At<void*>(self, 0x14) = buffer;
    At<uint32_t>(self, 0x18) = bytes;
    return buffer;
}

// 0x8976C0 (thiscall): NiTPointerMap<...>::GetNext (the item at the position, the position moved
// on; eax left as the original leaves it)
void* __fastcall NiTPointerMap_GetNext_8976C0(uint8_t* self, re::Edx, re::Ptr32<re::Ptr32<void>>* position, uint32_t* key, re::Ptr32<void>* value) {
    re::Ptr32<void>* item = *position;
    *key = re::PtrCast<uintptr_t>(item[1]);
    *value = item[2];
    if (item[0] != nullptr) {
        *position = static_cast<re::Ptr32<void>*>(item[0]);
        return item;
    }
    uint32_t size = At<uint32_t>(self, 4);
    uint32_t itemKey = re::PtrCast<uintptr_t>(item[1]);
    void* result = reinterpret_cast<void*>(uintptr_t(itemKey / size));
    for (uint32_t i = itemKey % size + 1; i < size; i++) {
        re::Ptr32<void>* bucket = &At<re::Ptr32<void>*>(self, 8)[i];
        result = bucket + 1;
        if (*bucket != nullptr) {
            *position = static_cast<re::Ptr32<void>*>(*bucket);
            return bucket;
        }
    }
    *position = nullptr;
    return result;
}

// 0x897730 (thiscall): NiDX9IndexBufferManager's constructor (the device held)
void* __fastcall NiDX9IndexBufferManager_Construct(uint8_t* self, re::Edx, void* device) {
    At<uint32_t>(self, 0) = kRefObjectVtable;
    At<uint32_t>(self, 4) = 0;
    Increment(&ms_uiObjects);
    At<uint32_t>(self, 0) = kIBManagerVtable;
    ConstructMap(self + 0x1C);
    ConstructMap(self + 0x2C);
    ConstructMap(self + 0x3C);
    At<void*>(self, 8) = device;
    Com<long>(device, kAddRef);
    At<void*>(self, 0xC) = nullptr;
    At<uint32_t>(self, 0x10) = 0;
    At<void*>(self, 0x14) = nullptr;
    At<uint32_t>(self, 0x18) = 0;
    PointerMap_RemoveAll(self + 0x1C);
    PointerMap_RemoveAll(self + 0x2C);
    PointerMap_RemoveAll(self + 0x3C);
    return self;
}

// 0x897840 (thiscall): NiDX9IndexBufferManager's destructor (every map's buffer sets deleted, the
// device's indices unset, the temporary buffers and the device let go)
void __fastcall NiDX9IndexBufferManager_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kIBManagerVtable;
    for (int m = 0; m < 3; m++) {
        uint8_t* map = self + 0x1C + m * 0x10;
        uint32_t size = At<uint32_t>(map, 4);
        re::Ptr32<void>* item = nullptr;
        for (uint32_t i = 0; i < size; i++) {
            if (At<re::Ptr32<re::Ptr32<void>>*>(map, 8)[i] != nullptr) {
                item = At<re::Ptr32<re::Ptr32<void>>*>(map, 8)[i];
                break;
            }
        }
        while (item != nullptr) {
            uint8_t* set = static_cast<uint8_t*>(item[2]);
            re::Ptr32<void>* next = static_cast<re::Ptr32<void>*>(item[0]);
            if (next == nullptr) {
                uint32_t count = At<uint32_t>(map, 4);
                for (uint32_t i = re::PtrCast<uintptr_t>(item[1]) % count + 1; i < count; i++) {
                    if (At<re::Ptr32<re::Ptr32<void>>*>(map, 8)[i] != nullptr) {
                        next = At<re::Ptr32<re::Ptr32<void>>*>(map, 8)[i];
                        break;
                    }
                }
            }
            item = next;
            if (set != nullptr) {
                DeleteBufferSet(set);
            }
        }
    }
    PointerMap_RemoveAll(self + 0x1C);
    PointerMap_RemoveAll(self + 0x2C);
    PointerMap_RemoveAll(self + 0x3C);
    Com<long>(At<void*>(self, 8), kSetIndices, static_cast<void*>(nullptr));
    if (At<void*>(self, 0xC) != nullptr) {
        Com<long>(At<void*>(self, 0xC), kRelease);
    }
    if (At<void*>(self, 0x14) != nullptr) {
        Com<long>(At<void*>(self, 0x14), kRelease);
    }
    if (At<void*>(self, 8) != nullptr) {
        Com<long>(At<void*>(self, 8), kRelease);
        At<void*>(self, 8) = nullptr;
    }
    DestructMap(self + 0x3C);
    DestructMap(self + 0x2C);
    DestructMap(self + 0x1C);
    At<uint32_t>(self, 0) = kRefObjectVtable;
    Decrement(&ms_uiObjects);
}

// 0x897A70 (thiscall): NiDX9IndexBufferManager::DestroyDefaultBuffers (the first map's buffer sets
// and the temporary buffers, before a reset)
void __fastcall NiDX9IndexBufferManager_DestroyDefaultBuffers(uint8_t* self, re::Edx) {
    uint8_t* map = self + 0x1C;
    re::Ptr32<void>* position = nullptr;
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x20); i++) {
        if (At<re::Ptr32<re::Ptr32<void>>*>(self, 0x24)[i] != nullptr) {
            position = At<re::Ptr32<re::Ptr32<void>>*>(self, 0x24)[i];
            break;
        }
    }
    while (position != nullptr) {
        uint32_t key;
        re::Ptr32<void> set;
        PointerMap_GetNext(map, reinterpret_cast<re::Ptr32<void>*>(&position), &key, &set);
        if (set != nullptr) {
            DeleteBufferSet(static_cast<uint8_t*>(set));
        }
    }
    PointerMap_RemoveAll(map);
    if (At<void*>(self, 0xC) != nullptr) {
        Com<long>(At<void*>(self, 0xC), kRelease);
        At<void*>(self, 0xC) = nullptr;
    }
    if (At<void*>(self, 0x14) != nullptr) {
        Com<long>(At<void*>(self, 0x14), kRelease);
        At<void*>(self, 0x14) = nullptr;
    }
}

// 0x897B60 (thiscall): NiDX9IndexBufferManager's deleting destructor
void* __fastcall NiDX9IndexBufferManager_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    IBManager_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x4C);
    }
    return self;
}

#define C40I_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C40I_REVERSED("NiGeometryBufferData::BaseDeletingDestructor", 0x00896E80, NiGeometryBufferDataBase_DeletingDestructor);
C40I_REVERSED("NiGeometryBufferData::IsVBValid", 0x00896EB0, NiGeometryBufferData_IsVBValid);
C40I_REVERSED("NiGeometryBufferData::RemoveIB", 0x00896EE0, NiGeometryBufferData_RemoveIB);
C40I_REVERSED("NiGeometryBufferData::SetStreamCount", 0x00896F10, NiGeometryBufferData_SetStreamCount);
C40I_REVERSED("NiGeometryBufferData::SetFVF", 0x00897010, NiGeometryBufferData_SetFVF);
C40I_REVERSED("NiGeometryBufferData::SetVertexDeclaration", 0x00897040, NiGeometryBufferData_SetVertexDeclaration);
C40I_REVERSED("NiGeometryBufferData::ContainsVertexData", 0x00897080, NiGeometryBufferData_ContainsVertexData);
C40I_REVERSED("NiGeometryBufferData::~NiGeometryBufferData", 0x008970F0, NiGeometryBufferData_Destruct);
C40I_REVERSED("NiGeometryBufferData::DeletingDestructor", 0x00897170, NiGeometryBufferData_DeletingDestructor);
C40I_REVERSED("NiDX9IndexBufferManager::CreateIndexBuffer", 0x00897190, NiDX9IndexBufferManager_CreateIndexBuffer);
C40I_REVERSED("NiDX9IndexBufferManager::PackBuffer", 0x00897200, NiDX9IndexBufferManager_PackBuffer);
C40I_REVERSED("NiDX9IndexBufferManager::ResizeTempPointConnectivity", 0x00897360,
              NiDX9IndexBufferManager_ResizeTempPointConnectivity);
C40I_REVERSED("NiDX9IndexBufferManager::ResizeTempLineConnectivity", 0x00897510,
              NiDX9IndexBufferManager_ResizeTempLineConnectivity);
C40I_REVERSED("NiTPointerMap::GetNext_8976C0", 0x008976C0, NiTPointerMap_GetNext_8976C0);
C40I_REVERSED("NiDX9IndexBufferManager::NiDX9IndexBufferManager", 0x00897730, NiDX9IndexBufferManager_Construct);
C40I_REVERSED("NiDX9IndexBufferManager::~NiDX9IndexBufferManager", 0x00897840, NiDX9IndexBufferManager_Destruct);
C40I_REVERSED("NiDX9IndexBufferManager::DestroyDefaultBuffers", 0x00897A70,
              NiDX9IndexBufferManager_DestroyDefaultBuffers);
C40I_REVERSED("NiDX9IndexBufferManager::DeletingDestructor", 0x00897B60, NiDX9IndexBufferManager_DeletingDestructor);

} // namespace ni::c40
