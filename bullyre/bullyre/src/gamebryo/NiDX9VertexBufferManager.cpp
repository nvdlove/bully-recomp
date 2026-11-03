// NiDX9VertexBufferManager (vtable 0x99C084: the deleting destructor and DeleteThis; 0x70 bytes,
// made by NiDX9Renderer::Initialize): the renderer's vertex buffers and the FVF packing of a
// geometry's data into them. Locking goes through a system-memory staging buffer the unlock copies
// back, under the manager's lock.
//
// Layout (from the constructor 0x887AA0): +0x8 the device (held), +0xC/+0x1C/+0x2C three maps of
// vertex buffer blocks (0x1C each: +0x8 five 0x3C-byte pool entries, an entry's vertex buffer at
// +0x20), +0x3C the frame id, +0x40 the staging buffer, +0x44 its size, +0x48 the locked bits,
// +0x4C the locked size, +0x50 the lock (NiCriticalSection).

#include <cstddef>
#include <cstdint>
#include <xmmintrin.h>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::c39 {
using namespace ni::dx9;
namespace {

constexpr uintptr_t kManagerVtable = 0x0099C084;
constexpr uintptr_t kNiRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t kTPointerMapBaseVtable = 0x00950760;
constexpr uintptr_t kTPointerMapVtable = 0x00965A9C;
constexpr uint32_t kManagerSize = 0x70;

GAME_VAR(uint32_t, NiRefObject_ms_uiObjects, 0x020E2168);
GAME_VAR(uint32_t, ms_uiMaxTextureBlendStages, 0x00BCB7F8);

GAME_FN(void, __thiscall, TMapBase_RemoveAll, 0x005AC520, void* map);
GAME_FN(void, __thiscall, BlockMap_GetNext, 0x008976C0, void* map, re::Ptr32<void>* position, uint32_t* key, re::Ptr32<void>* value);
GAME_FN(void, __thiscall, PoolEntry_Destruct, 0x008A2C50, void* entry);
GAME_FN(void, __thiscall, BufferData_SetFVF, 0x00897010, void* self, uint32_t fvf);
GAME_FN(uint8_t*, __thiscall, BufferData_GetVBChip, 0x00878550, void* self, uint32_t stream);
GAME_FN(const float*, __thiscall, GeometryData_GetTextureSet, 0x0077A000, void* data, uint32_t set);
GAME_FN(void, __thiscall, Declaration_PackBinormalsAndTangents, 0x00885E40, void* skin, void* data, void* positionDest,
        void* normalDest, void* binormalDest, void* tangentDest, uint32_t stride);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);

// this file's (called where modes apply)
GAME_FN(uint8_t, __thiscall, VBManager_CreateVertexBuffer, 0x00887230, void* self, void* bufferData,
        uint32_t stream);
GAME_FN(uint8_t*, __thiscall, VBManager_LockVB, 0x00887280, void* self, void* vb, uint32_t offset, uint32_t size,
        uint32_t flags);
GAME_FN(void, __stdcall, VBManager_CopyElements, 0x00887320, uint8_t* dest, const uint8_t* source,
        uint32_t destStride, uint32_t size, uint32_t count);
GAME_FN(void, __thiscall, VBManager_CalculateFVF, 0x008873B0, void* self, void* data, void* partition, uint32_t* fvf,
        uint32_t* stride, uint32_t* positionOffset, uint32_t* weightOffset, uint32_t* normalOffset,
        uint32_t* colorOffset, uint32_t* textureOffset);
GAME_FN(void, __thiscall, VBManager_Destruct, 0x00887BC0, void* self);

// a float to an integer with fistp in the default rounding mode (to nearest even; out of range the
// integer indefinite)
inline int32_t RoundToInt(float value) {
    return _mm_cvtss_si32(_mm_set_ss(value));
}

// a NiColorA (r, g, b, a) packed as D3DCOLOR: each channel times 255.0 (a float at the main
// thread's precision), rounded; not masked (an out of range channel spills into the next)
inline uint32_t PackColor(const float* color) {
    int32_t a = RoundToInt(float(double(color[3]) * 255.0));
    int32_t r = RoundToInt(float(double(color[0]) * 255.0));
    int32_t g = RoundToInt(float(double(color[1]) * 255.0));
    int32_t b = RoundToInt(float(double(color[2]) * 255.0));
    return uint32_t((((a << 8 | r) << 8 | g) << 8) | b);
}

// the staging buffer copied back, the lock let go, the vertex buffer unlocked (UnlockVB inlined)
// +0x48, the locked bits: the vertex buffer's memory as D3D mapped it (in the 64-bit build
// anywhere, so kept as a native pointer: re::NativeField)
inline void*& LockedBits(uint8_t* self) {
    return re::NativeField<void>(self, 0x48);
}

inline bool UnlockStaged(uint8_t* self, void* vb) {
    crt::MemcpyS(LockedBits(self), At<uint32_t>(self, 0x4C), At<void*>(self, 0x40), At<uint32_t>(self, 0x4C));
    LockedBits(self) = nullptr;
    At<uint32_t>(self, 0x4C) = 0;
    Unlock(At<NiCriticalSection>(self, 0x50));
    return Com<long>(vb, 12) >= 0;   // Unlock
}

// the blocks of a map deleted (each with its five pool entries, their vertex buffers released)
inline void DeleteBlock(uint8_t* block) {
    for (int i = 0; i < 5; i++) {
        uint8_t* entry = At<uint8_t*>(block, 8 + i * 4);
        if (entry != nullptr) {
            if (At<void*>(entry, 0x20) != nullptr) {
                Com<long>(At<void*>(entry, 0x20), kRelease);
            }
            PoolEntry_Destruct(entry);
            NiDelete(entry, 0x3C);
        }
    }
    NiDelete(block, 0x1C);
}

inline uint8_t* MapFirst(const uint8_t* map) {
    for (uint32_t i = 0; i < At<uint32_t>(map, 4); i++) {
        if (At<re::Ptr32<uint8_t>*>(map, 8)[i] != nullptr) {
            return At<re::Ptr32<uint8_t>*>(map, 8)[i];
        }
    }
    return nullptr;
}
inline uint8_t* MapNext(const uint8_t* map, const uint8_t* item) {
    if (At<uint8_t*>(item, 0) != nullptr) {
        return At<uint8_t*>(item, 0);
    }
    uint32_t size = At<uint32_t>(map, 4);
    for (uint32_t i = At<uint32_t>(item, 4) % size + 1; i < size; i++) {
        if (At<re::Ptr32<uint8_t>*>(map, 8)[i] != nullptr) {
            return At<re::Ptr32<uint8_t>*>(map, 8)[i];
        }
    }
    return nullptr;
}

inline void ConstructMap(uint8_t* map) {
    At<uint32_t>(map, 0) = kTPointerMapBaseVtable;
    At<uint32_t>(map, 4) = 0x25;
    At<uint32_t>(map, 0xC) = 0;
    void* table = NiMalloc(0x94);
    At<void*>(map, 8) = table;
    crt::Memset(table, 0, At<uint32_t>(map, 4) * 4);
    At<uint32_t>(map, 0) = kTPointerMapVtable;
}
inline void DestructMap(uint8_t* map) {
    At<uint32_t>(map, 0) = kTPointerMapVtable;
    TMapBase_RemoveAll(map);
    At<uint32_t>(map, 0) = kTPointerMapBaseVtable;
    TMapBase_RemoveAll(map);
    NiFree(At<void*>(map, 8));
}

} // namespace

// 0x887210 (thiscall): NiDX9VertexBufferManager::IncrementFrameID (at the end of each frame)
void __fastcall VBManager_IncrementFrameID(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0x3C)++;
}

// 0x887220 (thiscall): NiDX9VertexBufferManager::GetFrameID (4 bytes: not hooked)
uint32_t __fastcall VBManager_GetFrameID(uint8_t* self, re::Edx) {
    return At<uint32_t>(self, 0x3C);
}
RE_UNHOOKED_FN("NiDX9VertexBufferManager::GetFrameID", 0x00887220, &VBManager_GetFrameID);

// 0x887230 (thiscall): a stream's vertex buffer made again by the buffer data's geometry group
// (slot 7 lets the old chip go, slot 6 makes one) and kept as the stream's chip. (The manager
// isn't read.)
uint8_t __fastcall VBManager_CreateVertexBufferImpl(uint8_t*, re::Edx, uint8_t* bufferData, uint32_t stream) {
    void* group = At<void*>(bufferData, 8);
    Virtual<void>(group, 7, bufferData, stream);
    void* chip = Virtual<void*>(group, 6, bufferData, stream);
    if (chip == nullptr) {
        return 0;
    }
    if (stream < At<uint32_t>(bufferData, 0x20)) {
        At<re::Ptr32<void>*>(bufferData, 0x28)[stream] = chip;
    }
    return 1;
}

// 0x887280 (thiscall): NiDX9VertexBufferManager::LockVB (the buffer locked; the manager's lock
// held until UnlockVB; the staging buffer grown to the size, filled from the buffer unless the
// lock discards or doesn't overwrite; the staging buffer is what's returned)
uint8_t* __fastcall VBManager_LockVBImpl(uint8_t* self, re::Edx, void* vb, uint32_t offset, uint32_t size,
                                         uint32_t flags) {
    void* bits = nullptr;
    if (Com<long>(vb, 11, offset, size, &bits, flags) < 0) {   // Lock
        return nullptr;
    }
    Lock(At<NiCriticalSection>(self, 0x50));
    LockedBits(self) = bits;
    At<uint32_t>(self, 0x4C) = size;
    if (At<uint32_t>(self, 0x44) < size) {
        NiFree(At<void*>(self, 0x40));
        At<void*>(self, 0x40) = NiMalloc(size);
        At<uint32_t>(self, 0x44) = size;
    }
    if ((flags & 0x3000) == 0) {   // D3DLOCK_DISCARD | D3DLOCK_NOOVERWRITE
        crt::MemcpyS(At<void*>(self, 0x40), size, bits, size);
    }
    return At<uint8_t*>(self, 0x40);
}

// 0x887320 (stdcall): `count` elements of `size` bytes from a packed array to a strided one
void __stdcall VBManager_CopyElementsImpl(uint8_t* dest, const uint8_t* source, uint32_t destStride, uint32_t size,
                                          uint32_t count) {
    for (; count != 0; count--) {
        crt::MemcpyS(dest, size, source, size);
        dest += destStride;
        source += size;
    }
}

// 0x887360 (thiscall): NiDX9VertexBufferManager::UnlockVB
uint8_t __fastcall VBManager_UnlockVB(uint8_t* self, re::Edx, void* vb) {
    return UnlockStaged(self, vb) ? 1 : 0;
}

// 0x8873B0 (stdcall): a data's FVF, stride and element offsets (positions; 3 blend weights when a
// partition has bones; normals; colors; the texture sets, at least one, at most the blend stages)
void __fastcall VBManager_CalculateFVFImpl(void*, re::Edx, uint8_t* data, uint8_t* partition, uint32_t* fvf, uint32_t* stride,
                                          uint32_t* positionOffset, uint32_t* weightOffset, uint32_t* normalOffset,
                                          uint32_t* colorOffset, uint32_t* textureOffset) {
    *fvf = 0;
    *stride = 0;
    uint16_t bones = partition != nullptr ? At<uint16_t>(partition, 0x20) : 0;
    *positionOffset = 0;
    *stride += 0xC;
    *weightOffset = *stride;
    if (bones == 0) {
        *fvf |= 2;   // D3DFVF_XYZ
    } else {
        *fvf = 10;   // D3DFVF_XYZB3
        *stride += 0xC;
    }
    if (At<void*>(data, 0x24) != nullptr) {
        *fvf |= 0x10;   // D3DFVF_NORMAL
        *normalOffset = *stride;
        *stride += 0xC;
    }
    if (At<void*>(data, 0x28) != nullptr) {
        *fvf |= 0x40;   // D3DFVF_DIFFUSE
        *colorOffset = *stride;
        *stride += 4;
    }
    uint32_t sets = At<uint8_t>(data, 0x32) & 0x3F;
    if (sets == 0) {
        sets = 1;
    } else {
        if (ms_uiMaxTextureBlendStages < sets) {
            sets = ms_uiMaxTextureBlendStages;
        }
        if (sets == 0) {
            return;
        }
    }
    *fvf |= sets << 8;
    *textureOffset = *stride;
    *stride += sets * 8;
}

// 0x887470 (thiscall): NiDX9VertexBufferManager::PackUntransformedVB (an unskinned data's FVF
// vertices: the chip reused when it's a big enough vertex-data buffer of the same FVF with static
// data, else a new one with every part packed; positions and normals (with the basis when skinned),
// colors, texture sets). Returns the chip, or null.
uint8_t* __fastcall VBManager_PackUntransformedVB(uint8_t* self, re::Edx, uint8_t* data, void* skin, uint32_t parts,
                                                  uint8_t* chip, uint8_t* locked) {
    uint16_t vertices = At<uint16_t>(data, 0x30);
    if (vertices == 0) {
        return chip;
    }
    const uint8_t* positions = At<const uint8_t*>(data, 0x20);
    const uint8_t* normals = At<const uint8_t*>(data, 0x24);
    uint32_t sets = At<uint8_t>(data, 0x32) & 0x3F;
    const float* colors = At<const float*>(data, 0x28);
    if (sets > ms_uiMaxTextureBlendStages) {
        sets = ms_uiMaxTextureBlendStages;
    }
    uint32_t fvf = 0, stride = 0, positionOffset, weightOffset, normalOffset, colorOffset, textureOffset;
    VBManager_CalculateFVF(nullptr, data, nullptr, &fvf, &stride, &positionOffset, &weightOffset, &normalOffset, &colorOffset,
                           &textureOffset);
    uint8_t* bufferData = At<uint8_t*>(data, 0x3C);
    BufferData_SetFVF(bufferData, fvf);
    if (At<uint32_t>(bufferData, 0x20) != 0) {
        At<uint32_t*>(bufferData, 0x24)[0] = stride;
    }
    bool unlock = true;
    if (locked != nullptr) {
        unlock = false;
    } else {
        bool reuse = false;
        if (chip != nullptr && At<void*>(chip, 8) != nullptr && (At<uint16_t>(data, 0x34) & 0xF000) != 0x8000) {
            uint32_t desc[6];   // D3DVERTEXBUFFER_DESC
            Com<long>(At<void*>(chip, 8), 13, desc);
            reuse = desc[0] == 100 && desc[5] == fvf && desc[4] >= uint32_t(vertices) * stride;
        }
        if (!reuse) {
            if (!VBManager_CreateVertexBuffer(self, bufferData, 0)) {
                return nullptr;
            }
            chip = At<uint32_t>(bufferData, 0x20) != 0 ? At<re::Ptr32<uint8_t>*>(bufferData, 0x28)[0] : nullptr;
            parts = 0xFFF;
        }
        locked = VBManager_LockVB(self, At<void*>(chip, 8), At<uint32_t>(chip, 0xC), At<uint32_t>(chip, 0x14),
                                  At<uint32_t>(chip, 0x10));
    }
    if (skin != nullptr) {
        if (positions != nullptr && (parts & 1)) {
            Declaration_PackBinormalsAndTangents(skin, data, locked + positionOffset,
                                                 normals != nullptr ? locked + normalOffset : nullptr, nullptr,
                                                 nullptr, stride);
        }
    } else {
        if (positions != nullptr && (parts & 1)) {
            VBManager_CopyElements(locked + positionOffset, positions, stride, 0xC, vertices);
        }
        if (normals != nullptr && (parts & 2)) {
            VBManager_CopyElements(locked + normalOffset, normals, stride, 0xC, vertices);
        }
    }
    if (colors != nullptr && (parts & 4)) {
        uint8_t* dest = locked + colorOffset;
        for (uint32_t i = vertices; i != 0; i--) {
            *reinterpret_cast<uint32_t*>(dest) = PackColor(colors);
            colors += 4;
            dest += stride;
        }
    }
    if ((parts & 8) && sets != 0) {
        uint8_t* dest = locked + textureOffset;
        for (uint32_t set = 0; set < sets; set++) {
            VBManager_CopyElements(dest, reinterpret_cast<const uint8_t*>(GeometryData_GetTextureSet(data, set)),
                                   stride, 8, vertices);
            dest += 8;
        }
    }
    if (unlock && !UnlockStaged(self, At<void*>(chip, 8))) {
        return nullptr;
    }
    return chip;
}

// 0x8877E0 (thiscall): screen-space vertices packed (XYZRHW with z 0 and rhw 1, colors, one
// texture set): the chip reused when it's a big enough vertex-data buffer of the same FVF, else
// stream 0's made again. Returns the chip, or null. (Name a guess.)
uint8_t* __fastcall VBManager_PackScreenSpaceVB(uint8_t* self, re::Edx, uint8_t* bufferData, uint32_t vertices,
                                                const uint8_t* positions, const float* colors,
                                                const uint8_t* textures, uint32_t parts, uint8_t* chip,
                                                uint8_t* locked) {
    uint16_t count = uint16_t(vertices);
    if (count == 0 || positions == nullptr) {
        return chip;
    }
    uint32_t fvf = 4;   // D3DFVF_XYZRHW
    uint32_t stride = 0x10;
    uint32_t colorOffset = 0, textureOffset = 0;
    if (colors != nullptr) {
        fvf = 0x44;
        colorOffset = 0x10;
        stride = 0x14;
    }
    if (textures != nullptr) {
        fvf |= 0x100;
        textureOffset = stride;
        stride += 8;
    }
    BufferData_SetFVF(bufferData, fvf);
    if (At<uint32_t>(bufferData, 0x20) != 0) {
        At<uint32_t*>(bufferData, 0x24)[0] = stride;
    }
    bool unlock = true;
    if (locked != nullptr) {
        unlock = false;
    } else {
        bool reuse = false;
        if (chip != nullptr && At<void*>(chip, 8) != nullptr) {
            uint32_t desc[6];
            Com<long>(At<void*>(chip, 8), 13, desc);
            reuse = desc[0] == 100 && desc[5] == fvf && desc[4] >= uint32_t(count) * stride;
        }
        if (!reuse) {
            if (!VBManager_CreateVertexBuffer(self, bufferData, 0)) {
                return nullptr;
            }
            chip = BufferData_GetVBChip(bufferData, 0);
        }
        locked = VBManager_LockVB(self, At<void*>(chip, 8), At<uint32_t>(chip, 0xC), At<uint32_t>(chip, 0x14),
                                  At<uint32_t>(chip, 0x10));
    }
    if ((parts & 1) && count != 0) {
        uint8_t* dest = locked;
        for (uint32_t i = count; i != 0; i--) {
            crt::MemcpyS(dest, 8, positions, 8);
            positions += 8;
            At<float>(dest, 8) = 0.0f;
            At<float>(dest, 0xC) = 1.0f;
            dest += stride;
        }
    }
    if (colors != nullptr && (parts & 4) && count != 0) {
        uint8_t* dest = locked + colorOffset;
        for (uint32_t i = count; i != 0; i--) {
            *reinterpret_cast<uint32_t*>(dest) = PackColor(colors);
            colors += 4;
            dest += stride;
        }
    }
    if (textures != nullptr && (parts & 2) && count != 0) {
        uint8_t* dest = locked + textureOffset;
        for (uint32_t i = count; i != 0; i--) {
            crt::MemcpyS(dest, 8, textures, 8);
            textures += 8;
            dest += stride;
        }
    }
    if (unlock && !UnlockStaged(self, At<void*>(chip, 8))) {
        return nullptr;
    }
    return chip;
}

// 0x887AA0 (thiscall): NiDX9VertexBufferManager::NiDX9VertexBufferManager(device)
uint8_t* __fastcall VBManager_Construct(uint8_t* self, re::Edx, void* device) {
    At<uint32_t>(self, 0) = kNiRefObjectVtable;
    At<uint32_t>(self, 4) = 0;
    Increment(&NiRefObject_ms_uiObjects);
    At<uint32_t>(self, 0) = kManagerVtable;
    ConstructMap(self + 0xC);
    ConstructMap(self + 0x1C);
    ConstructMap(self + 0x2C);
    At<void*>(self, 0x40) = nullptr;
    At<uint32_t>(self, 0x44) = 0;
    LockedBits(self) = nullptr;
    At<uint32_t>(self, 0x4C) = 0;
    NiCriticalSection& lock = At<NiCriticalSection>(self, 0x50);
    lock.owner = 0;
    lock.lockCount = 0;
    Import<void>(ni::sys::kInitializeCriticalSection, &lock.section);
    At<void*>(self, 8) = device;
    Com<long>(device, kAddRef);
    TMapBase_RemoveAll(self + 0xC);
    TMapBase_RemoveAll(self + 0x1C);
    TMapBase_RemoveAll(self + 0x2C);
    At<uint32_t>(self, 0x3C) = 0;
    return self;
}

// 0x887BC0 (thiscall): NiDX9VertexBufferManager::~NiDX9VertexBufferManager (every block of the
// three maps deleted, the device released, the staging buffer freed, the lock deleted)
void __fastcall VBManager_DestructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kManagerVtable;
    for (int m = 0; m < 3; m++) {
        uint8_t* map = self + 0xC + m * 0x10;
        for (uint8_t* item = MapFirst(map); item != nullptr;) {
            uint8_t* block = At<uint8_t*>(item, 8);
            item = MapNext(map, item);
            if (block != nullptr) {
                DeleteBlock(block);
            }
        }
    }
    TMapBase_RemoveAll(self + 0xC);
    TMapBase_RemoveAll(self + 0x1C);
    TMapBase_RemoveAll(self + 0x2C);
    Com<long>(At<void*>(self, 8), kRelease);
    At<void*>(self, 8) = nullptr;
    NiFree(At<void*>(self, 0x40));
    Import<void>(ni::sys::kDeleteCriticalSection, &At<NiCriticalSection>(self, 0x50).section);
    DestructMap(self + 0x2C);
    DestructMap(self + 0x1C);
    DestructMap(self + 0xC);
    At<uint32_t>(self, 0) = kNiRefObjectVtable;
    Decrement(&NiRefObject_ms_uiObjects);
}

// 0x887DD0 (thiscall): NiDX9VertexBufferManager::DestroyDefaultBuffers (the first map's blocks,
// the ones in the default pool, deleted before a reset)
void __fastcall VBManager_DestroyDefaultBuffers(uint8_t* self, re::Edx) {
    uint8_t* map = self + 0xC;
    re::Ptr32<void> position = MapFirst(map);
    while (position != nullptr) {
        uint32_t key;
        re::Ptr32<void> block = nullptr;
        BlockMap_GetNext(map, &position, &key, &block);
        if (block != nullptr) {
            DeleteBlock(static_cast<uint8_t*>(block));
        }
    }
    TMapBase_RemoveAll(map);
}

// 0x887EA0 (thiscall): NiDX9VertexBufferManager's deleting destructor
uint8_t* __fastcall VBManager_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    VBManager_Destruct(self);
    if (flags & 1) {
        NiDelete(self, kManagerSize);
    }
    return self;
}

// 0x887EC0 (thiscall): NiDX9VertexBufferManager::PackSkinnedVB (a hardware-skinned partition's FVF
// vertices through its vertex map: positions, blend weights (their sum checked against 1 into a
// message nobody reads; the last weight dropped when there are four bones, kept with one or
// three), normals, colors, texture sets). Returns the chip, or null.
uint8_t* __fastcall VBManager_PackSkinnedVB(uint8_t* self, re::Edx, uint8_t* data, void* skin, uint8_t* partition,
                                            uint32_t parts, uint8_t* chip, uint8_t* locked) {
    (void)skin;
    const uint8_t* positions = At<const uint8_t*>(data, 0x20);
    const uint8_t* normals = At<const uint8_t*>(data, 0x24);
    const uint8_t* colors = At<const uint8_t*>(data, 0x28);
    uint32_t sets = At<uint8_t>(data, 0x32) & 0x3F;
    if (sets > ms_uiMaxTextureBlendStages) {
        sets = ms_uiMaxTextureBlendStages;
    }
    uint16_t bones = At<uint16_t>(partition, 0x20);
    uint16_t vertices = At<uint16_t>(partition, 0x1C);
    const uint16_t* vertexMap = At<const uint16_t*>(partition, 0xC);
    uint32_t fvf = 0, stride = 0, positionOffset, weightOffset, normalOffset, colorOffset, textureOffset;
    VBManager_CalculateFVF(nullptr, data, partition, &fvf, &stride, &positionOffset, &weightOffset, &normalOffset, &colorOffset,
                           &textureOffset);
    uint8_t* bufferData = At<uint8_t*>(partition, 0x28);
    BufferData_SetFVF(bufferData, fvf);
    if (At<uint32_t>(bufferData, 0x20) != 0) {
        At<uint32_t*>(bufferData, 0x24)[0] = stride;
    }
    bool unlock = true;
    if (locked != nullptr) {
        unlock = false;
    } else {
        bool reuse = false;
        if (chip != nullptr && At<void*>(chip, 8) != nullptr && (At<uint16_t>(data, 0x34) & 0xF000) != 0x8000) {
            uint32_t desc[6];
            Com<long>(At<void*>(chip, 8), 13, desc);
            reuse = desc[0] == 100 && desc[5] == fvf && desc[4] >= uint32_t(vertices) * stride;
        }
        if (!reuse) {
            if (!VBManager_CreateVertexBuffer(self, bufferData, 0)) {
                return nullptr;
            }
            chip = At<uint32_t>(bufferData, 0x20) != 0 ? At<re::Ptr32<uint8_t>*>(bufferData, 0x28)[0] : nullptr;
            parts = 0xFFF;
        }
        locked = VBManager_LockVB(self, At<void*>(chip, 8), At<uint32_t>(chip, 0xC), At<uint32_t>(chip, 0x14),
                                  At<uint32_t>(chip, 0x10));
    }
    if (positions != nullptr && (parts & 1)) {
        uint8_t* dest = locked + positionOffset;
        for (uint32_t i = 0; i < vertices; i++) {
            crt::MemcpyS(dest, 0xC, positions + vertexMap[i] * 0xC, 0xC);
            dest += stride;
        }
        const float* weights = At<const float*>(partition, 8);
        float* destWeights = reinterpret_cast<float*>(locked + weightOffset);
        for (uint32_t v = 0; v < vertices; v++) {
            float remaining = 1.0f;
            for (uint32_t b = 0; b < bones; b++) {
                remaining = remaining - weights[b];
            }
            float magnitude = remaining < 0 ? -remaining : remaining;
            if (magnitude > 1e-05f) {
                char message[0x100];
                NiSprintf(message, 0x100, "ILLEGAL WEIGHTS! Vertex %4d - Weight Sum %8.5f  (%8.5f)\n", v,
                          double(1.0f - remaining), double(remaining));
            }
            uint16_t b = 0;
            for (; int32_t(b) < int32_t(bones) - 1; b++) {
                destWeights[b] = weights[b];
            }
            if (bones == 3 || bones == 1) {
                destWeights[b] = weights[b];
            }
            destWeights = reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(destWeights) + stride);
            weights += bones;
        }
    }
    if (normals != nullptr && (parts & 2)) {
        uint8_t* dest = locked + normalOffset;
        for (uint32_t i = 0; i < vertices; i++) {
            crt::MemcpyS(dest, 0xC, normals + vertexMap[i] * 0xC, 0xC);
            dest += stride;
        }
    }
    if (colors != nullptr && (parts & 4)) {
        uint8_t* dest = locked + colorOffset;
        for (uint32_t i = 0; i < vertices; i++) {
            *reinterpret_cast<uint32_t*>(dest) = PackColor(reinterpret_cast<const float*>(colors + vertexMap[i] * 0x10));
            dest += stride;
        }
    }
    if ((parts & 8) && sets != 0) {
        uint8_t* setDest = locked + textureOffset;
        for (uint32_t set = 0; set < sets; set++) {
            const uint8_t* source = reinterpret_cast<const uint8_t*>(GeometryData_GetTextureSet(data, set));
            uint8_t* dest = setDest;
            for (uint32_t i = 0; i < vertices; i++) {
                crt::MemcpyS(dest, 8, source + vertexMap[i] * 8, 8);
                dest += stride;
            }
            setDest += 8;
        }
    }
    if (unlock && !UnlockStaged(self, At<void*>(chip, 8))) {
        return nullptr;
    }
    return chip;
}

#define C39J_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C39J_REVERSED("NiDX9VertexBufferManager::IncrementFrameID", 0x00887210, VBManager_IncrementFrameID);
C39J_REVERSED("NiDX9VertexBufferManager::CreateVertexBuffer", 0x00887230, VBManager_CreateVertexBufferImpl);
C39J_REVERSED("NiDX9VertexBufferManager::LockVB", 0x00887280, VBManager_LockVBImpl);
C39J_REVERSED("NiDX9VertexBufferManager::CopyElements", 0x00887320, VBManager_CopyElementsImpl);
C39J_REVERSED("NiDX9VertexBufferManager::UnlockVB", 0x00887360, VBManager_UnlockVB);
C39J_REVERSED("NiDX9VertexBufferManager::CalculateFVF", 0x008873B0, VBManager_CalculateFVFImpl);
C39J_REVERSED("NiDX9VertexBufferManager::PackUntransformedVB", 0x00887470, VBManager_PackUntransformedVB);
C39J_REVERSED("NiDX9VertexBufferManager::PackScreenSpaceVB", 0x008877E0, VBManager_PackScreenSpaceVB);
C39J_REVERSED("NiDX9VertexBufferManager::NiDX9VertexBufferManager", 0x00887AA0, VBManager_Construct);
C39J_REVERSED("NiDX9VertexBufferManager::~NiDX9VertexBufferManager", 0x00887BC0, VBManager_DestructImpl);
C39J_REVERSED("NiDX9VertexBufferManager::DestroyDefaultBuffers", 0x00887DD0, VBManager_DestroyDefaultBuffers);
C39J_REVERSED("NiDX9VertexBufferManager::DeletingDestructor", 0x00887EA0, VBManager_DeletingDestructor);
C39J_REVERSED("NiDX9VertexBufferManager::PackSkinnedVB", 0x00887EC0, VBManager_PackSkinnedVB);

} // namespace ni::c39
