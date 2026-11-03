// NiDX9Renderer's geometry packing and transforms (NiDX9Renderer.h): PackGeometryBuffer /
// PackSkinnedGeometryBuffer (a geometry's vertices into its NiGeometryBufferData's streams, its
// triangles / strips into an index buffer), the buffer data's streams and vertex format set up
// (0x87A970), PerformPrecache (the geometry waiting in the precache map at +0x42C packed: alone,
// or many into one locked shared vertex buffer; the static ones' CPU copies dropped after),
// the world matrix (and the render state's model transform), a skinned partition's bone matrices
// set as the device's world matrices, and the skin instance's bone matrices (CalculateBoneMatrices).
//
// NiGeometryBufferData: +0x4 the vertex format flags (texture sets << 24, 0x400000 colours,
// 0x800000 normals, 0x200000 binormals and tangents), +0x8 its vertex buffer manager (slot 7 releases a
// stream), +0x14 software processing, +0x18 / +0x1C the vertex counts, +0x20 the stream count,
// +0x24 the strides, +0x28 the VB chips (+0x8 the VB, +0xC its offset, +0x10 lock flags, +0x14 its
// size), +0x2C / +0x30 / +0x34 the index count, size and buffer, +0x38, +0x3C the primitive type,
// +0x40 / +0x44 the triangle counts, +0x48 the strip count, +0x4C the strip lengths, +0x50 the
// indices. A precache entry (0x24): +0 the data, +4 the skin, +8 the partition, +0xC the shader
// declaration, +0x18 the buffer data, +0x1C the stream, +0x20 the next.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::dx9 {
namespace {

GAME_VAR(uint32_t, ms_uiMaxTextureSets, 0x00BCB7F8);
inline void* const NiTriShapeData_RTTI = reinterpret_cast<void*>(0x020E3130);
inline void* const NiTriStripsData_RTTI = reinterpret_cast<void*>(0x020E3B30);

GAME_FN(uint8_t, __cdecl, IsKindOfRTTI, 0x004066F0, void* rtti, void* object);   // NiIsKindOf (a bool)
GAME_FN(uint8_t, __thiscall, BufferData_IsVBValid, 0x00896EB0, void* self);
GAME_FN(void, __thiscall, BufferData_SetStreamCount, 0x00896F10, void* self, uint32_t count);
GAME_FN(void, __thiscall, BufferData_ReleaseIB, 0x00896EE0, void* self);
GAME_FN(void, __thiscall, BufferData_SetFVF, 0x00897010, void* self, uint32_t fvf);
GAME_FN(void, __thiscall, BufferData_SetVertexDeclaration, 0x00897040, void* self, void* declaration);
GAME_FN(uint32_t, __thiscall, VBManager_GetFrameID, 0x00887220, void* self);
// (both return the locked vertex data, a pointer: the callers test all of eax. Read as a byte, a
// pointer ending in 00 looked like a failure and the stream was packed again in the FVF layout over
// the declaration's: torn characters and buildings in the 64-bit build)
GAME_FN(uint8_t*, __thiscall, VBManager_PackUntransformedVB, 0x00887470, void* self, void* data, void* skin,
        uint32_t dirty, void* chip, void* dest);
GAME_FN(uint8_t*, __thiscall, VBManager_PackSkinnedVB, 0x00887EC0, void* self, void* data, void* skin,
        void* partition, uint32_t dirty, void* chip, void* dest);
GAME_FN(void, __thiscall, VBManager_GenerateVertexParameters, 0x008873B0, void* self, void* data, void* partition,
        uint32_t* fvf, uint32_t* stride, void* a, void* b, void* c, void* d, void* e);
GAME_FN(uint8_t, __thiscall, VBManager_CreateVertexBuffer, 0x00887230, void* self, void* bufferData, uint32_t stream);
GAME_FN(uint8_t*, __thiscall, VBManager_LockVB, 0x00887280, void* self, void* vb, uint32_t offset, uint32_t size,
        uint32_t flags);
GAME_FN(void, __thiscall, VBManager_UnlockVB, 0x00887360, void* self, void* vb);
GAME_FN(void*, __thiscall, IBManager_PackBuffer, 0x00897200, void* self, const void* indices, uint32_t count,
        uint32_t activeCount, void* oldBuffer, uint32_t* size, int writeOnly, uint32_t usage);
GAME_FN(void, __thiscall, TMapBase_RemoveAll, 0x005AC520, void* self);
// NiGeometryData::Replace / NiTriShapeData's / NiTriStripsData's (the CPU copies dropped)
GAME_FN(void, __thiscall, GeometryData_Replace, 0x0077A640, void* self, uint32_t vertices, void* points,
        void* normals, void* colors, void* textures, uint32_t sets, uint32_t binormalFlags);
GAME_FN(void, __thiscall, TriShapeData_ReplaceTriangles, 0x0077BA70, void* self, uint32_t triangles, void* list);
GAME_FN(void, __thiscall, TriStripsData_ReplaceStripData, 0x007D85E0, void* self, uint32_t strips, void* lengths,
        void* lists);
GAME_FN(void, __thiscall, SkinData_FreeBoneVertData, 0x007DB0C0, void* self);
// transforms
GAME_FN(void, __cdecl, MakeD3DMatrix, 0x00406670, void* d3d, const void* rotate, const void* translate, float scale);
GAME_FN(void, __cdecl, GetD3DFromNi, 0x008787C0, void* d3d, const void* rotate, const void* translate, float scale);
GAME_FN(void, __thiscall, RenderState_SetModelTransform, 0x00882E60, void* self, const void* transform);
GAME_FN(void*, __thiscall, Matrix3_TimesScalar, 0x00768A10, const void* self, void* result, float scalar);
GAME_FN(float*, __cdecl, Point3_TimesMatrix, 0x00768AD0, float* result, const float* point, const void* matrix);
GAME_FN(void, __thiscall, Transform_Invert, 0x00770950, const void* self, void* result);
GAME_FN(void*, __thiscall, Transform_Multiply, 0x00406830, const void* self, void* result, const void* other);
GAME_FN(void*, __cdecl, NiAlignedMalloc, 0x00752340, uint32_t size, uint32_t alignment);
GAME_FN(void, __cdecl, NiAlignedFree, 0x007523A0, void* p);
GAME_FN(void*, __cdecl, OperatorNew, 0x005EEAA0, size_t size);

// this file's (called where modes apply)
GAME_FN(uint8_t, __thiscall, Renderer_PackGeometryBuffer, 0x0087A2F0, void* self, void* bufferData, void* data,
        void* skin, void* declaration, int force);
GAME_FN(uint8_t, __thiscall, Renderer_PackSkinnedGeometryBuffer, 0x0087A690, void* self, void* bufferData,
        void* data, void* skin, void* partition, void* declaration, int force);
GAME_FN(void, __thiscall, Renderer_ReleaseSourceData, 0x0087C2D0, void* self, void* data, void* skin,
        void* partition);

inline void* Chip(uint8_t* bufferData, uint32_t stream) {
    return stream < At<uint32_t>(bufferData, 0x20) ? At<re::Ptr32<void>*>(bufferData, 0x28)[stream] : nullptr;
}

// the format flags (+0x4) from a geometry data
inline void SetFormatFlags(uint8_t* bufferData, uint8_t* data) {
    bool colors = At<void*>(data, 0x28) != nullptr;
    bool normals = At<void*>(data, 0x24) != nullptr;
    bool binormals = normals && (At<uint16_t>(data, 0x32) & 0xF000) != 0;
    uint32_t sets = data[0x32] & 0x3F;
    if (sets > ms_uiMaxTextureSets) {
        sets = ms_uiMaxTextureSets;
    }
    uint32_t flags = colors ? 0x400000u : 0u;
    if (normals) {
        flags |= 0x800000;
    }
    if (binormals) {
        flags |= 0x200000;
    }
    At<uint32_t>(bufferData, 4) = sets << 24 | flags;
}

// the renderer locks (Renderer_LockRendererAndSourceData's, inlined)
inline void LockRendererAndSourceData(uint8_t* self) {
    NiCriticalSection& renderer = At<NiCriticalSection>(self, kRendererLock);
    Import<void>(ni::sys::kEnterCriticalSection, &renderer.section);
    renderer.owner = Import<uint32_t>(ni::sys::kGetCurrentThreadId);
    renderer.lockCount++;
    if (renderer.lockCount == 1) {
        Virtual<void>(self, 80);
    }
    Lock(At<NiCriticalSection>(self, kSourceDataLock));
}
inline void UnlockRendererAndSourceData(uint8_t* self) {
    Unlock(At<NiCriticalSection>(self, kSourceDataLock));
    NiCriticalSection& renderer = At<NiCriticalSection>(self, kRendererLock);
    if (renderer.lockCount == 1) {
        Virtual<void>(self, 81);
    }
    Unlock(renderer);
}

} // namespace

// 0x87A2F0 (thiscall): NiDX9Renderer::PackGeometryBuffer(buffer data, geometry data, skin,
// declaration, force) (a static one already packed is left; the dirty flags say what to repack,
// all of it when forced, mutable or the streams changed). (Neither triangles nor strips: the
// counts are the game's uninitialised stack and the triangle count its `force` argument.)
uint8_t __fastcall Renderer_PackGeometryBufferImpl(uint8_t* self, re::Edx, uint8_t* bufferData, uint8_t* data,
                                                   uint8_t* skin, void* declaration, int force) {
    if (bufferData == nullptr || data == nullptr) {
        return 0;
    }
    uint8_t hasVB = BufferData_IsVBValid(bufferData);
    if (!re::Flag(force) && hasVB != 0 && (At<uint16_t>(data, 0x34) & 0xF000) == 0x4000) {
        return 1;
    }
    SetFormatFlags(bufferData, data);
    uint32_t activeVertices = At<uint16_t>(data, 0x30);
    uint32_t vertices = At<uint16_t>(data, 0xC);
    void* indices = nullptr;
    void* stripLengths = nullptr;
    uint32_t activeTriangles = 0;   // (else: the game's stack)
    uint32_t strips = 0;
    uint32_t indexCount = 0;
    uint32_t activeIndexCount = 0;
    uint32_t triangles;
    if (HasClassBit(data, NiTriShapeData_ClassBit)) {
        activeTriangles = At<uint16_t>(data, 0x42);
        triangles = At<uint16_t>(data, 0x40);
        indices = At<void*>(data, 0x48);
        stripLengths = nullptr;
        strips = 1;
        indexCount = activeTriangles * 3;
        activeIndexCount = triangles * 3;
    } else if (IsKindOfRTTI(NiTriStripsData_RTTI, data) != 0) {
        stripLengths = At<void*>(data, 0x48);
        activeTriangles = At<uint16_t>(data, 0x42);
        triangles = At<uint16_t>(data, 0x40);
        indices = At<void*>(data, 0x4C);
        strips = At<uint16_t>(data, 0x44);
        indexCount = activeTriangles + strips * 2;
        activeIndexCount = triangles + strips * 2;
    } else {
        triangles = uint32_t(force);
    }
    uint32_t streams = declaration != nullptr ? At<uint32_t>(declaration, 0xC) : 1;
    uint32_t dirty = At<uint16_t>(data, 0x34) & 0xFFF;
    if (skin != nullptr) {
        uint32_t frame = At<uint32_t>(skin, 0x18);
        if (frame != VBManager_GetFrameID(At<void*>(self, kVBManager))) {
            dirty |= 3;
            At<uint32_t>(skin, 0x18) = VBManager_GetFrameID(At<void*>(self, kVBManager));
        }
    }
    if ((At<uint16_t>(data, 0x34) & 0xF000) == 0x8000 || re::Flag(force)) {
        dirty = 0xFFF;
    }
    bool changed = true;
    if (hasVB != 0) {
        if (streams == At<uint32_t>(bufferData, 0x20)) {
            changed = false;
        } else {
            BufferData_SetStreamCount(bufferData, streams);
        }
    } else {
        void* manager = At<void*>(bufferData, 8);
        for (uint32_t i = 0; i < streams; i++) {
            Virtual<void>(manager, 7, bufferData, i);
        }
        BufferData_SetStreamCount(bufferData, streams);
        At<uint32_t>(bufferData, 0x38) = 0;
    }
    if (changed) {
        dirty = 0xFFF;
    }
    At<uint32_t>(bufferData, 0x1C) = vertices & 0xFFFF;
    At<uint32_t>(bufferData, 0x18) = activeVertices & 0xFFFF;
    At<void*>(bufferData, 0x4C) = stripLengths;
    At<uint32_t>(bufferData, 0x40) = activeTriangles & 0xFFFF;
    At<uint32_t>(bufferData, 0x44) = triangles & 0xFFFF;
    At<void*>(bufferData, 0x50) = indices;
    At<uint32_t>(bufferData, 0x48) = strips & 0xFFFF;
    if (dirty & 0xFFEF) {
        for (uint32_t i = 0; i < streams; i++) {
            void* chip = Chip(bufferData, i);
            if (declaration == nullptr ||
                Virtual<void*>(declaration, 26, data, skin, static_cast<void*>(nullptr), dirty, chip, i,
                               static_cast<void*>(nullptr)) == nullptr) {
                if (streams != 1) {
                    return 0;
                }
                if (VBManager_PackUntransformedVB(At<void*>(self, kVBManager), data, skin, dirty, chip, nullptr) == nullptr) {
                    return 0;
                }
            }
        }
    }
    if (indices != nullptr && (dirty & 0x30)) {
        uint32_t size = At<uint32_t>(bufferData, 0x30);
        void* old = At<void*>(bufferData, 0x34);
        void* buffer = IBManager_PackBuffer(At<void*>(self, kIBManager), indices, indexCount, activeIndexCount, old,
                                            &size, 1, bufferData[0x14] != 0 ? 0x10u : 0u);
        if (buffer == nullptr) {
            return 0;
        }
        if (old != buffer) {
            BufferData_ReleaseIB(bufferData);
            At<void*>(bufferData, 0x34) = buffer;
            At<uint32_t>(bufferData, 0x2C) = indexCount;
            At<uint32_t>(bufferData, 0x30) = size;
        }
    }
    At<uint16_t>(data, 0x34) &= 0xF000;
    return 1;
}

// 0x87A690 (thiscall): NiDX9Renderer::PackSkinnedGeometryBuffer(buffer data, geometry data, skin,
// partition, declaration, force) (the partition's vertices, triangles or strips)
uint8_t __fastcall Renderer_PackSkinnedGeometryBufferImpl(uint8_t* self, re::Edx, uint8_t* bufferData, uint8_t* data,
                                                          uint8_t* skin, uint8_t* partition, void* declaration,
                                                          int force) {
    if (bufferData == nullptr || data == nullptr) {
        return 0;
    }
    uint8_t hasVB = BufferData_IsVBValid(bufferData);
    if (!re::Flag(force) && hasVB != 0 && (At<uint16_t>(data, 0x34) & 0xF000) == 0x4000) {
        return 1;
    }
    SetFormatFlags(bufferData, data);
    uint32_t vertices = At<uint16_t>(partition, 0x1C);
    void* triangleList = At<void*>(partition, 0x14);
    uint32_t triangles = At<uint16_t>(partition, 0x1E);
    void* stripLengths = At<void*>(partition, 0x18);
    uint32_t strips = At<uint16_t>(partition, 0x22);
    uint32_t indexCount;
    if (strips == 0) {
        indexCount = triangles * 3;
        strips = 1;
    } else {
        indexCount = triangles + strips * 2;
    }
    uint32_t streams = declaration != nullptr ? At<uint32_t>(declaration, 0xC) : 1;
    uint32_t dirty = At<uint16_t>(data, 0x34) & 0xFFF;
    if ((At<uint16_t>(data, 0x34) & 0xF000) == 0x8000 || re::Flag(force)) {
        dirty = 0xFFF;
    }
    bool changed = true;
    if (hasVB != 0) {
        if (streams == At<uint32_t>(bufferData, 0x20)) {
            changed = false;
        } else {
            BufferData_SetStreamCount(bufferData, streams);
        }
    } else {
        void* manager = At<void*>(bufferData, 8);
        for (uint32_t i = 0; i < streams; i++) {
            Virtual<void>(manager, 7, bufferData, i);
        }
        BufferData_SetStreamCount(bufferData, streams);
        At<uint32_t>(bufferData, 0x38) = 0;
    }
    if (changed) {
        dirty = 0xFFF;
    }
    At<uint32_t>(bufferData, 0x18) = vertices;
    At<uint32_t>(bufferData, 0x1C) = vertices;
    At<void*>(bufferData, 0x4C) = stripLengths;
    At<void*>(bufferData, 0x50) = triangleList;
    At<uint32_t>(bufferData, 0x40) = triangles;
    At<uint32_t>(bufferData, 0x44) = triangles;
    At<uint32_t>(bufferData, 0x48) = strips & 0xFFFF;
    if (dirty & 0xFFEF) {
        for (uint32_t i = 0; i < streams; i++) {
            void* chip = Chip(bufferData, i);
            if (declaration == nullptr ||
                Virtual<void*>(declaration, 26, data, skin, partition, dirty, chip, i, static_cast<void*>(nullptr)) ==
                    nullptr) {
                if (streams != 1) {
                    return 0;
                }
                if (VBManager_PackSkinnedVB(At<void*>(self, kVBManager), data, skin, partition, dirty, chip,
                                            nullptr) == nullptr) {
                    return 0;
                }
            }
        }
    }
    if (dirty & 0x30) {
        uint32_t size = At<uint32_t>(bufferData, 0x30);
        void* old = At<void*>(bufferData, 0x34);
        void* buffer = IBManager_PackBuffer(At<void*>(self, kIBManager), triangleList, indexCount, indexCount, old,
                                            &size, 1, bufferData[0x14] != 0 ? 0x10u : 0u);
        if (buffer == nullptr) {
            return 0;
        }
        if (old != buffer) {
            BufferData_ReleaseIB(bufferData);
            At<void*>(bufferData, 0x34) = buffer;
            At<uint32_t>(bufferData, 0x2C) = indexCount;
            At<uint32_t>(bufferData, 0x30) = size;
        }
    }
    return 1;
}

// 0x87A970 (thiscall): NiDX9Renderer::CreateGeometryBuffers(buffer data, geometry data, partition,
// declaration) (the streams and their strides from the declaration, else the FVF the vertex
// buffer manager works out (not past 4 bones a vertex); the format flags; software processing
// when the device can switch; a vertex buffer per stream, all let go again if one fails)
uint8_t __fastcall Renderer_CreateGeometryBuffers(uint8_t* self, re::Edx, uint8_t* bufferData, uint8_t* data,
                                                  uint8_t* partition, void* declaration) {
    uint32_t fvf = 0;
    void* handle = nullptr;
    uint32_t streams = 0;
    uint32_t stride = 0;
    if (declaration == nullptr || Virtual<uint8_t>(declaration, 27, &handle, &streams) == 0) {
        if (partition != nullptr && At<uint16_t>(partition, 0x20) > 4) {
            return 0;
        }
        streams = 1;
        uint32_t scratch[5];
        VBManager_GenerateVertexParameters(At<void*>(self, kVBManager), data, partition, &fvf, &stride, &scratch[0],
                                           &scratch[1], &scratch[2], &scratch[3], &scratch[4]);
    }
    BufferData_SetStreamCount(bufferData, streams);
    if (fvf != 0) {
        BufferData_SetFVF(bufferData, fvf);
        if (At<uint32_t>(bufferData, 0x20) != 0) {
            At<uint32_t*>(bufferData, 0x24)[0] = stride;
        }
    } else {
        BufferData_SetVertexDeclaration(bufferData, handle);
        for (uint32_t i = 0; i < streams; i++) {
            uint32_t streamStride = Virtual<uint32_t>(declaration, 23, i);   // GetStreamStride
            if (i < At<uint32_t>(bufferData, 0x20)) {
                At<uint32_t*>(bufferData, 0x24)[i] = streamStride;
            }
        }
    }
    SetFormatFlags(bufferData, data);
    if (declaration != nullptr && self[kSWVertexCapable] != 0) {
        bufferData[0x14] = Virtual<uint8_t>(declaration, 25);   // GetSoftwareVertexProcessing
    }
    for (uint32_t i = 0; i < streams; i++) {
        if (VBManager_CreateVertexBuffer(At<void*>(self, kVBManager), bufferData, i) == 0) {
            for (; i != 0; i--) {
                Virtual<void>(At<void*>(bufferData, 8), 7, bufferData, i);
            }
            return 0;
        }
    }
    return 1;
}

// 0x87C2D0 (thiscall, the renderer unused): a static geometry's CPU copies dropped after packing,
// but for what its keep flags (+0x36) keep: 1 vertices, 2 normals, 4 colours, 8 texture
// coordinates, 0x10 triangles / strips, 0x20 the skin's bone weights
void __fastcall Renderer_ReleaseSourceDataImpl(uint8_t*, re::Edx, uint8_t* data, uint8_t* skin, void*) {
    if (data == nullptr || (At<uint16_t>(data, 0x34) & 0xF000) != 0x4000) {
        return;
    }
    uint32_t keep = data[0x36];
    uint32_t binormals = At<uint16_t>(data, 0x32) & 0xF000;
    if (keep == 0) {
        GeometryData_Replace(data, At<uint16_t>(data, 0xC), nullptr, nullptr, nullptr, nullptr, 0, binormals);
    } else {
        void* points = (keep & 1) ? At<void*>(data, 0x20) : nullptr;
        void* normals = (keep & 2) ? At<void*>(data, 0x24) : nullptr;
        void* colors = (keep & 4) ? At<void*>(data, 0x28) : nullptr;   // (NiGeometryData: +0x20 vertices,
                                                                        //  +0x24 normals, +0x28 colours, +0x2C UVs)
        void* textures = nullptr;
        uint32_t sets = 0;
        if (keep & 8) {
            sets = data[0x32] & 0x3F;
            textures = At<void*>(data, 0x2C);
        }
        GeometryData_Replace(data, At<uint16_t>(data, 0xC), points, normals, colors, textures, sets, binormals);
    }
    if (skin != nullptr) {
        if ((keep & 0x20) == 0) {
            SkinData_FreeBoneVertData(At<void*>(skin, 8));
        }
        return;
    }
    if (keep & 0x10) {
        return;
    }
    if (IsKindOfRTTI(NiTriShapeData_RTTI, data) != 0) {
        TriShapeData_ReplaceTriangles(data, At<uint16_t>(data, 0x40), nullptr);
    } else if (IsKindOfRTTI(NiTriStripsData_RTTI, data) != 0) {
        TriStripsData_ReplaceStripData(data, At<uint16_t>(data, 0x44), At<void*>(data, 0x48), nullptr);
    }
}

// 0x87C3E0 (thiscall): NiDX9Renderer::PerformPrecache (under the renderer locks: each precache map
// item's entries; key 0: each packed by itself; else they share a vertex buffer: locked once over
// all their chips and packed into it, the first stream's indices too. Then the map emptied and
// each packed static geometry's CPU copies dropped). (A shared item whose chips span nothing is
// left out of the list, its entries leaked: the game's. The VB unlocked is the last entry's.)
void __fastcall Renderer_PerformPrecache(uint8_t* self, re::Edx) {
    LockRendererAndSourceData(self);
    uint32_t hashSize = At<uint32_t>(self, 0x430);
    re::Ptr32<uint8_t>* table = At<re::Ptr32<uint8_t>*>(self, 0x434);
    uint8_t* item = nullptr;
    for (uint32_t i = 0; i < hashSize; i++) {
        if (table[i] != nullptr) {
            item = table[i];
            break;
        }
    }
    uint8_t* packed = nullptr;
    while (item != nullptr) {
        uint8_t* entries = At<uint8_t*>(item, 8);
        uint32_t key = At<uint32_t>(item, 4);
        uint8_t* nextItem = At<uint8_t*>(item, 0);
        if (nextItem == nullptr) {
            uint32_t size = At<uint32_t>(self, 0x430);
            for (uint32_t i = key % size + 1; i < size; i++) {
                if (At<re::Ptr32<uint8_t>*>(self, 0x434)[i] != nullptr) {
                    nextItem = At<re::Ptr32<uint8_t>*>(self, 0x434)[i];
                    break;
                }
            }
        }
        item = nextItem;
        if (entries == nullptr) {
            continue;
        }
        if (key == 0) {
            for (uint8_t* entry = entries; entry != nullptr;) {
                uint8_t* next = At<uint8_t*>(entry, 0x20);
                if (At<uint32_t>(entry, 0x1C) == 0) {
                    if (At<void*>(entry, 8) != nullptr) {
                        Renderer_PackSkinnedGeometryBuffer(self, At<void*>(entry, 0x18), At<void*>(entry, 0),
                                                           At<void*>(entry, 4), At<void*>(entry, 8),
                                                           At<void*>(entry, 0xC), 1);
                    } else {
                        Renderer_PackGeometryBuffer(self, At<void*>(entry, 0x18), At<void*>(entry, 0),
                                                    At<void*>(entry, 4), At<void*>(entry, 0xC), 1);
                    }
                }
                At<uint8_t*>(entry, 0x20) = packed;
                packed = entry;
                entry = next;
            }
            continue;
        }
        // the shared vertex buffer's span
        uint32_t low = 0xFFFFFFFF;
        uint32_t high = 0;
        uint8_t* chip = nullptr;
        for (uint8_t* entry = entries; entry != nullptr; entry = At<uint8_t*>(entry, 0x20)) {
            chip = static_cast<uint8_t*>(Chip(At<uint8_t*>(entry, 0x18), At<uint32_t>(entry, 0x1C)));
            uint32_t offset = At<uint32_t>(chip, 0xC);
            if (offset < low) {
                low = offset;
            }
            uint32_t end = offset + At<uint32_t>(chip, 0x14);
            if (end > high) {
                high = end;
            }
        }
        if (high <= low) {
            continue;
        }
        uint8_t* base = VBManager_LockVB(At<void*>(self, kVBManager), At<void*>(chip, 8), low, high - low,
                                         At<uint32_t>(chip, 0x10));
        for (uint8_t* entry = entries; entry != nullptr;) {
            uint8_t* bufferData = At<uint8_t*>(entry, 0x18);
            uint32_t stream = At<uint32_t>(entry, 0x1C);
            uint8_t* next = At<uint8_t*>(entry, 0x20);
            chip = static_cast<uint8_t*>(Chip(bufferData, stream));
            uint8_t* dest = base + At<uint32_t>(chip, 0xC);
            void* declaration = At<void*>(entry, 0xC);
            if (declaration == nullptr ||
                Virtual<void*>(declaration, 26, At<void*>(entry, 0), At<void*>(entry, 4), At<void*>(entry, 8), 0xFFFu,
                               static_cast<void*>(chip), stream, static_cast<void*>(dest)) == nullptr) {
                if (At<void*>(entry, 8) != nullptr) {
                    VBManager_PackSkinnedVB(At<void*>(self, kVBManager), At<void*>(entry, 0), At<void*>(entry, 4),
                                            At<void*>(entry, 8), 0xFFF, chip, dest);
                } else {
                    VBManager_PackUntransformedVB(At<void*>(self, kVBManager), At<void*>(entry, 0), At<void*>(entry, 4),
                                                  0xFFF, chip, dest);
                }
            }
            if (stream == 0 && At<void*>(bufferData, 0x50) != nullptr) {
                uint32_t count = 0;
                uint32_t activeCount = 0;
                uint16_t* lengths = At<uint16_t*>(bufferData, 0x4C);
                if (lengths != nullptr) {
                    for (uint32_t n = At<uint32_t>(bufferData, 0x48); n != 0; n--) {
                        count += lengths[n - 1];
                    }
                    activeCount = count;
                } else if (At<uint32_t>(bufferData, 0x3C) == 4) {   // D3DPT_TRIANGLELIST
                    count = At<uint32_t>(bufferData, 0x40) * 3;
                    activeCount = At<uint32_t>(bufferData, 0x44) * 3;
                } else if (At<uint32_t>(bufferData, 0x3C) == 5) {   // TRIANGLESTRIP
                    count = At<uint32_t>(bufferData, 0x40) + 2;
                    activeCount = At<uint32_t>(bufferData, 0x44) + 2;
                }
                uint32_t size = At<uint32_t>(bufferData, 0x30);
                void* buffer = IBManager_PackBuffer(At<void*>(self, kIBManager), At<void*>(bufferData, 0x50), count,
                                                    activeCount, At<void*>(bufferData, 0x34), &size, 1,
                                                    bufferData[0x14] != 0 ? 0x10u : 0u);
                At<void*>(bufferData, 0x34) = buffer;
                At<uint32_t>(bufferData, 0x2C) = count;
                At<uint32_t>(bufferData, 0x30) = size;
            }
            At<uint8_t*>(entry, 0x20) = packed;
            packed = entry;
            entry = next;
        }
        VBManager_UnlockVB(At<void*>(self, kVBManager), At<void*>(chip, 8));
    }
    TMapBase_RemoveAll(self + 0x42C);
    while (packed != nullptr) {
        uint8_t* data = At<uint8_t*>(packed, 0);
        uint8_t* next = At<uint8_t*>(packed, 0x20);
        if ((At<uint16_t>(data, 0x34) & 0xF000) == 0x4000 && At<uint32_t>(packed, 0x1C) == 0) {
            At<uint16_t>(data, 0x34) &= 0xF000;
            Renderer_ReleaseSourceData(self, data, At<void*>(packed, 4), At<void*>(packed, 8));
        }
        NiDelete(packed, 0x24);
        packed = next;
    }
    UnlockRendererAndSourceData(self);
}

// 0x87C850 (thiscall): NiDX9Renderer::SetModelTransform(world, push) (the D3D world matrix, set
// on the device when asked; the render state's; the two directions at +0x450 / +0x45C into the
// model's space at +0x468 / +0x474)
void __fastcall Renderer_SetModelTransform(uint8_t* self, re::Edx, const uint8_t* world, int push) {
    MakeD3DMatrix(self + kWorld, world, world + 0x24, At<float>(world, 0x30));
    if (re::Flag(push)) {
        Com<long>(At<void*>(self, kDevice), kDevSetTransform, 0x100u, self + kWorld);   // D3DTS_WORLD
    }
    RenderState_SetModelTransform(At<void*>(self, kRenderState), world);
    alignas(4) uint8_t scaled[0x24];
    Matrix3_TimesScalar(world, scaled, At<float>(world, 0x30));
    float point[3];
    const float* p = Point3_TimesMatrix(point, reinterpret_cast<const float*>(self + 0x450), scaled);
    At<float>(self, 0x468) = p[0];
    At<float>(self, 0x46C) = p[1];
    At<float>(self, 0x470) = p[2];
    p = Point3_TimesMatrix(point, reinterpret_cast<const float*>(self + 0x45C), scaled);
    At<float>(self, 0x474) = p[0];
    At<float>(self, 0x478) = p[1];
    At<float>(self, 0x47C) = p[2];
}

// 0x87C920 (thiscall): NiDX9Renderer::SetSkinnedModelTransform(skin, partition, world) (up to 4
// bones a vertex: the partition's first four bones' matrices as the world matrices 0..3)
void __fastcall Renderer_SetSkinnedModelTransform(uint8_t* self, re::Edx, uint8_t* skin, uint8_t* partition,
                                                  const uint8_t* world) {
    RenderState_SetModelTransform(At<void*>(self, kRenderState), world);
    MakeD3DMatrix(self + kWorld, world, world + 0x24, At<float>(world, 0x30));
    if (At<uint16_t>(partition, 0x20) > 4) {
        return;
    }
    uint8_t* matrices = At<uint8_t*>(skin, 0x28);
    for (uint32_t i = 0; i < 4; i++) {
        uint32_t bone = At<uint16_t*>(partition, 4)[i];
        Com<long>(At<void*>(self, kDevice), kDevSetTransform, i + 0x100, matrices + bone * 0x40);
    }
}

// 0x87C9B0 (thiscall): NiDX9Renderer::CalculateBoneMatrices(skin, world, transpose, registers,
// world space) (once a frame or when the register count changes: each bone's world transform times
// its skin-to-bone, into the model's space unless in world space (its matrix kept at +0x2C), as
// 4x4 or 3x4 matrices; the render state's non-uniform scale flag cleared: the test in the game
// can never be true)
void __fastcall Renderer_CalculateBoneMatrices(uint8_t* self, re::Edx, uint8_t* skin, const uint8_t* world,
                                               int transpose, uint32_t registers, int worldSpace) {
    void* manager = At<void*>(self, kVBManager);
    uint32_t frame = At<uint32_t>(skin, 0x18);
    if (frame == VBManager_GetFrameID(manager) && registers == At<uint32_t>(skin, 0x20)) {
        return;
    }
    At<uint32_t>(skin, 0x18) = VBManager_GetFrameID(At<void*>(self, kVBManager));
    uint8_t* skinData = At<uint8_t*>(skin, 8);
    uint32_t bones = At<uint32_t>(skinData, 0x44);
    uint8_t* matrices = At<uint8_t*>(skin, 0x28);
    if (registers == 3) {
        transpose = 1;
    }
    uint32_t size = bones * registers * 16;
    if (size > At<uint32_t>(skin, 0x24)) {
        NiAlignedFree(matrices);
        matrices = static_cast<uint8_t*>(NiAlignedMalloc(size, 0x10));
        At<uint8_t*>(skin, 0x28) = matrices;
        At<uint32_t>(skin, 0x24) = size;
    }
    At<uint32_t>(skin, 0x1C) = bones;
    At<uint32_t>(skin, 0x20) = registers;
    if (registers == 4) {
        for (uint32_t i = 0; i < bones; i++) {
            float* m = reinterpret_cast<float*>(matrices + i * 0x40);
            if (re::Flag(transpose)) {
                m[14] = 0.0f;
                m[13] = 0.0f;
                m[12] = 0.0f;
            } else {
                m[11] = 0.0f;
                m[7] = 0.0f;
                m[3] = 0.0f;
            }
            m[15] = 1.0f;
        }
    }
    alignas(4) uint8_t inverseRoot[0x34];
    alignas(4) uint8_t product[0x34];
    alignas(4) uint8_t toSkin[0x34];   // the root parent's to skin, times the root's inverse
    alignas(4) uint8_t model[0x34];    // the world times that
    Transform_Invert(At<uint8_t*>(skin, 0x10) + 0x60, inverseRoot);
    crt::Memcpy(toSkin, Transform_Multiply(skinData + 0xC, product, inverseRoot), 0x34);
    crt::Memcpy(model, Transform_Multiply(world, product, toSkin), 0x34);
    uint8_t nonUniform = 0;
    float rootScale = At<float>(toSkin, 0x30);
    if (rootScale < 0.99f && rootScale > 1.01f) {
        nonUniform = 1;
    }
    if (re::Flag(worldSpace)) {
        float* m = At<float*>(skin, 0x2C);
        if (m == nullptr) {
            m = static_cast<float*>(OperatorNew(0x40));
            At<float*>(skin, 0x2C) = m;
            m[3] = 0.0f;
            m[7] = 0.0f;
            m[11] = 0.0f;
            m[15] = 1.0f;
        }
        MakeD3DMatrix(m, model, model + 0x24, At<float>(model, 0x30));
    }
    re::Ptr32<uint8_t>* boneObjects = At<re::Ptr32<uint8_t>*>(skin, 0x14);
    uint8_t* boneData = At<uint8_t*>(skinData, 0x40);
    for (uint32_t i = 0; i < bones; i++) {
        alignas(4) uint8_t bone[0x34];
        Transform_Multiply(boneObjects[i] + 0x60, bone, boneData);
        float boneScale = At<float>(boneObjects[i], 0x90);
        if (boneScale < 0.99f && 1.01f < boneScale) {
            nonUniform = 1;
        }
        if (!re::Flag(worldSpace)) {
            crt::Memcpy(bone, Transform_Multiply(model, product, bone), 0x34);
        }
        uint8_t* out = matrices + i * 0x40;
        if (registers == 4) {
            if (re::Flag(transpose)) {
                GetD3DFromNi(out, bone, bone + 0x24, At<float>(bone, 0x30));
            } else {
                MakeD3DMatrix(out, bone, bone + 0x24, At<float>(bone, 0x30));
            }
        } else {
            const float* r = reinterpret_cast<const float*>(bone);
            float s = At<float>(bone, 0x30);
            float* o = reinterpret_cast<float*>(matrices + i * 0x30);   // 3x4, the rows transposed
            o[0] = r[0] * s;
            o[4] = r[3] * s;
            o[8] = r[6] * s;
            o[1] = r[1] * s;
            o[5] = r[4] * s;
            o[9] = r[7] * s;
            o[2] = r[2] * s;
            o[6] = r[5] * s;
            o[10] = s * r[8];
            o[3] = r[9];
            o[7] = r[10];
            o[11] = r[11];
        }
        boneData += 0x4C;
    }
    At<uint8_t>(At<void*>(self, kRenderState), 0x20F5) = nonUniform;
}

#define DX9REND38F_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

DX9REND38F_REVERSED("NiDX9Renderer::PackGeometryBuffer", 0x0087A2F0, Renderer_PackGeometryBufferImpl);
DX9REND38F_REVERSED("NiDX9Renderer::PackSkinnedGeometryBuffer", 0x0087A690, Renderer_PackSkinnedGeometryBufferImpl);
DX9REND38F_REVERSED("NiDX9Renderer::CreateGeometryBuffers", 0x0087A970, Renderer_CreateGeometryBuffers);
DX9REND38F_REVERSED("NiDX9Renderer::ReleaseSourceData", 0x0087C2D0, Renderer_ReleaseSourceDataImpl);
DX9REND38F_REVERSED("NiDX9Renderer::PerformPrecache", 0x0087C3E0, Renderer_PerformPrecache);
DX9REND38F_REVERSED("NiDX9Renderer::SetModelTransform", 0x0087C850, Renderer_SetModelTransform);
DX9REND38F_REVERSED("NiDX9Renderer::SetSkinnedModelTransform", 0x0087C920, Renderer_SetSkinnedModelTransform);
DX9REND38F_REVERSED("NiDX9Renderer::CalculateBoneMatrices", 0x0087C9B0, Renderer_CalculateBoneMatrices);

} // namespace ni::dx9
