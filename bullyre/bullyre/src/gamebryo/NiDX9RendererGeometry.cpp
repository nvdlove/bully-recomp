// NiDX9Renderer's geometry and texture-data slots (NiDX9Renderer.h for the layout): the precache
// of a geometry's buffers (PrecacheGeometry, slot 46: the entries waiting in the map at +0x42C,
// one per vertex buffer, that PerformPrecache packs), the batch list (AddToBatch: the nodes at
// +0x43C..+0x440, from the free list at 0x20E6D14), Do_RenderShape, Do_RenderScreenTexture, the
// renderer data of rendered / cube / dynamic textures (the maps at +0x6C8, +0x6D8, +0x6E8),
// LockDynamicTexture / UnLockDynamicTexture, and the D3D shaders the renderer keeps (+0x70C).
//
// Precache entries (0x24, NiNew'd): +0x0 the geometry data, +0x4 the skin instance, +0x8 the
// partition, +0xC the shader declaration, +0x10 bones per partition, +0x14 bones per vertex,
// +0x18 the buffer data, +0x1C the stream, +0x20 the next one for the same vertex buffer.
// Batch nodes (0x10): +0x0 the geometry, +0x4 the partition, +0x8 the buffer data, +0xC next.
// A geometry's buffer data (NiGeometryBufferData): +0x4 the format flags, +0x8 its geometry
// group, +0x18/+0x1C vertex counts, +0x20 the streams, +0x28 the VB chips (+0x4 the vertex
// buffer), +0x38, +0x40/+0x44 triangle counts, +0x48 the strips, +0x4C/+0x50 the index data.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::c39 {
using namespace ni::dx9;
namespace {

// the NiRTTI objects and class bits tested here
inline void* const NiD3DShaderInterface_RTTI = reinterpret_cast<void*>(0x020E6E0C);
constexpr uintptr_t NiTriShape_RTTI = 0x020E30FC;
constexpr uintptr_t NiTriStrips_RTTI = 0x020E3B64;
GAME_VAR(uint32_t, NiTriBasedGeom_ClassBit, 0x020E2FDC);
GAME_VAR(uint32_t, NiD3DShaderInterface_ClassBit, 0x020E6E14);
GAME_VAR(uint8_t*, ms_pkBatchNodeFreeList, 0x020E6D14);   // the batch list's pool
GAME_ARRAY(const uint32_t, NiMatrix3_IDENTITY, 0x00BC75FC);
GAME_ARRAY(const uint32_t, NiPoint3_ZERO, 0x020E2D44);
GAME_VAR(void*, NiTexturingProperty_ms_spDefault, 0x020E28F0);   // (the property put back)

GAME_FN(uint8_t*, __cdecl, IsKindOf, 0x00401FF0, void* rtti, void* object);   // NiDynamicCast
GAME_FN(uint8_t, __cdecl, IsKindOfRTTI, 0x004066F0, uintptr_t rtti, const void* object);
GAME_FN(void, __thiscall, Map_SetAt, 0x008A5140, void* map, uint32_t key, void* value);
GAME_FN(uint8_t, __thiscall, Map_RemoveAt, 0x008A5A10, void* map, uint32_t key);
GAME_FN(const char*, __cdecl, NiDX9ErrorString, 0x00899150, long hr);
GAME_FN(uint8_t, __thiscall, BufferData_IsVBValid, 0x00896EB0, void* self);
GAME_FN(void, __thiscall, BufferData_SetStreamCount, 0x00896F10, void* self, uint32_t count);
GAME_FN(uint8_t, __thiscall, Renderer_CreateGeometryBuffers, 0x0087A970, void* self, void* bufferData, void* data,
        void* partition, void* declaration);
GAME_FN(void, __thiscall, Renderer_LockPrecache, 0x0087AD30, void* self);     // LockRendererAndSourceData
GAME_FN(void, __thiscall, Renderer_UnlockPrecache, 0x0087AD80, void* self);   // UnlockRendererAndSourceData
GAME_FN(void, __thiscall, Renderer_GenerateScreenTextureBuffers, 0x0087EE80, void* self, void* screenTexture,
        uint32_t dirtyFlags);
// the texture renderer data's creators (NiDX9RenderedTextureData::Create, ...)
GAME_FN(void*, __cdecl, RenderedTextureData_Create, 0x00899800, void* texture, void* renderer);
GAME_FN(void*, __cdecl, RenderedCubeMapData_Create, 0x00899A20, void* texture, void* renderer);
GAME_FN(void*, __cdecl, DynamicTextureData_Create, 0x00899CE0, void* texture);
// NiGeometry's
GAME_FN(void*, __thiscall, Geometry_GetShader, 0x00761630, void* geometry);
GAME_FN(void*, __thiscall, Geometry_GetActiveShader, 0x00761590, void* geometry);   // (from its material)
GAME_FN(void*, __thiscall, Geometry_GetActiveMaterial, 0x004028D0, void* geometry);
GAME_FN(uint8_t, __thiscall, Geometry_SetActiveMaterial, 0x00402880, void* geometry, void* material);
GAME_FN(uint8_t, __thiscall, Geometry_ApplyAndSetActiveMaterial, 0x00761F70, void* geometry, void* material,
        uint32_t extraData);
GAME_FN(void, __thiscall, Geometry_SetPropertyState, 0x0087AF70, void* geometry, void* state);
GAME_FN(void, __thiscall, Geometry_SetEffectState, 0x0087AFC0, void* geometry, void* state);
GAME_FN(void, __cdecl, ShaderDeclaration_CreateDeclForGeometry, 0x0077D090, void* geometry);
// NiGeometryGroupManager::AddObjectToGroup(group, data, skin, hardware skinned, bones per
// partition, bones per vertex)
GAME_FN(void, __thiscall, GroupManager_AddObjectToGroup, 0x00899470, void* self, void* group, void* data,
        void* skin, int hwSkinned, uint32_t bonesPerPartition, uint32_t bonesPerVertex);
// NiD3DShaderInterface::SetD3DRenderer
GAME_FN(void, __thiscall, D3DShaderInterface_SetD3DRenderer, 0x00886FE0, void* shader, void* renderer);
GAME_FN(void, __thiscall, ShaderList_Remove, 0x0075F630, void* list, const re::Ptr32<void>* element);
GAME_FN(void*, __thiscall, ShaderList_RemovePos, 0x00771DA0, void* list, re::Ptr32<void>* position);
// the screen texture's properties
GAME_FN(void, __thiscall, NiPointer_Assign, 0x00800420, re::Ptr32<void>* slot, void* object);
GAME_FN(void*, __thiscall, PropertyState_Construct, 0x007D2370, void* self);
GAME_FN(void, __thiscall, PropertyState_SetProperty, 0x0075DDF0, void* self, void* property);
GAME_FN(void*, __thiscall, VertexColorProperty_Construct, 0x005EFBD0, void* self);
GAME_FN(void*, __thiscall, AlphaProperty_Construct, 0x00407800, void* self);
GAME_FN(void*, __thiscall, ZBufferProperty_Construct, 0x00407B40, void* self);

// this file's (called where modes apply)
GAME_FN(uint8_t, __thiscall, Renderer_IsHardwareSkinned, 0x0087F9D0, void* self, void* geometry, void* shader);
GAME_FN(uint8_t, __thiscall, Renderer_PrecacheGeometryBuffers, 0x0087F450, void* self, void* geometry, void* data,
        uint32_t triangles, uint32_t maxTriangles, void* triList, void* stripLengths, uint32_t strips,
        void* declaration);
GAME_FN(uint8_t, __thiscall, Renderer_PrecacheSkinnedGeometryBuffers, 0x0087F560, void* self, void* geometry,
        void* data, void* skin, void* declaration, uint32_t bonesPerPartition, uint32_t bonesPerVertex);
GAME_FN(void, __thiscall, Renderer_PrepareScreenTextureData, 0x00881720, void* self, void* screenTexture);

constexpr int kPrecacheMap = 0x42C;      // NiTPointerMap<vertex buffer, entry*>
constexpr int kBatchHead = 0x43C;
constexpr int kBatchTail = 0x440;
constexpr int kBatchPropertyState = 0x444;
constexpr int kBatchEffectState = 0x448;
constexpr int kBatchShader = 0x44C;      // NiD3DShaderInterfacePtr
constexpr int kGroupManager = 0x6A8;
constexpr int kDefaultGroup = 0x6AC;     // (the static geometry group)
constexpr int kDynamicGroup = 0x6B0;
constexpr int kScreenTexturePropertyState = 0x6A4;
constexpr int kRenderedTextures = 0x6C8;   // NiTPointerMap<texture, renderer data>
constexpr int kRenderedCubeMaps = 0x6D8;
constexpr int kDynamicTextures = 0x6E8;
constexpr int kD3DShaders = 0x70C;         // NiTPointerList<NiD3DShaderInterface*>

// the map's buckets (NiTPointerMap: +0x4 the hash size, +0x8 the buckets; items next, key, value)
inline uint8_t* FindPrecacheEntry(uint8_t* renderer, uint32_t key) {
    uint8_t* map = renderer + kPrecacheMap;
    for (uint8_t* item = At<re::Ptr32<uint8_t>*>(map, 8)[key % At<uint32_t>(map, 4)]; item != nullptr;
         item = At<uint8_t*>(item, 0)) {
        if (key == At<uint32_t>(item, 4)) {
            return At<uint8_t*>(item, 8);
        }
    }
    return nullptr;
}

// the NiDX9TextureData behind a texture's renderer data when it's a dynamic one (bit 2 of +0x62)
inline uint8_t* AsDynamicTextureData(uint8_t* data) {
    return ((At<uint8_t>(data, 0x62) >> 2) & 1) ? data : nullptr;
}

// the D3D shader interface a geometry's shader is
inline uint8_t* CastToD3DShader(void* shader) {
    if (shader == nullptr) {
        return nullptr;
    }
    return HasClassBit(shader, NiD3DShaderInterface_ClassBit) ? static_cast<uint8_t*>(shader) : nullptr;
}

// a batch node from the pool (or NiNew'd), filled and put at the list's tail
inline void AppendBatchNode(uint8_t* self, void* geometry, void* partition, void* bufferData) {
    uint8_t* node = ms_pkBatchNodeFreeList;
    if (node == nullptr) {
        node = static_cast<uint8_t*>(NiNew(0x10));
    } else {
        ms_pkBatchNodeFreeList = At<uint8_t*>(node, 0xC);
        At<uint8_t*>(node, 0xC) = nullptr;
    }
    At<void*>(node, 0) = geometry;
    At<void*>(node, 4) = partition;
    At<void*>(node, 8) = bufferData;
    At<uint8_t*>(node, 0xC) = nullptr;
    uint8_t* tail = At<uint8_t*>(self, kBatchTail);
    if (tail != nullptr) {
        At<uint8_t*>(tail, 0xC) = node;
    } else {
        At<uint8_t*>(self, kBatchHead) = node;
    }
    At<uint8_t*>(self, kBatchTail) = node;
}

// an entry for each of the buffer data's vertex buffers, chained after the ones already waiting
inline void AddPrecacheEntries(uint8_t* self, uint8_t* bufferData, void* data, void* skin, void* partition,
                               void* declaration, uint32_t bonesPerPartition, uint32_t bonesPerVertex) {
    uint32_t streams = At<uint32_t>(bufferData, 0x20);
    for (uint32_t i = 0; i < streams; i++) {
        uint8_t* chip = i < At<uint32_t>(bufferData, 0x20) ? At<re::Ptr32<uint8_t>*>(bufferData, 0x28)[i] : nullptr;
        uint32_t vb = At<uint32_t>(chip, 4);
        uint8_t* entry = static_cast<uint8_t*>(NiNew(0x24));
        At<void*>(entry, 0) = data;
        At<void*>(entry, 4) = skin;
        At<void*>(entry, 8) = partition;
        At<void*>(entry, 0xC) = declaration;
        At<uint32_t>(entry, 0x10) = bonesPerPartition;
        At<uint32_t>(entry, 0x14) = bonesPerVertex;
        At<uint8_t*>(entry, 0x18) = bufferData;
        At<uint32_t>(entry, 0x1C) = i;
        At<uint8_t*>(entry, 0x20) = FindPrecacheEntry(self, vb);
        Map_SetAt(self + kPrecacheMap, vb, entry);
    }
}

} // namespace

// 0x87F450 (thiscall): the buffers of an unskinned tri-based geometry's data set up and its vertex
// buffers queued for PerformPrecache (an entry per vertex buffer). The geometry isn't read. (Name
// a guess: the half of NiDX9Renderer::PrecacheGeometry for NiTriShape / NiTriStrips.)
uint8_t __fastcall Renderer_PrecacheGeometryBuffersImpl(uint8_t* self, re::Edx, void*, uint8_t* data,
                                                        uint32_t triangles, uint32_t maxTriangles, void* triList,
                                                        void* stripLengths, uint32_t strips, void* declaration) {
    uint8_t* bufferData = At<uint8_t*>(data, 0x3C);
    if (BufferData_IsVBValid(bufferData)) {
        return 1;
    }
    At<uint32_t>(bufferData, 0x18) = At<uint16_t>(data, 0x30);   // the active vertex count
    At<uint32_t>(bufferData, 0x1C) = At<uint16_t>(data, 0xC);
    At<void*>(bufferData, 0x50) = triList;
    At<uint32_t>(bufferData, 0x44) = uint16_t(maxTriangles);
    At<uint32_t>(bufferData, 0x40) = uint16_t(triangles);
    At<uint32_t>(bufferData, 0x48) = uint16_t(strips);
    At<void*>(bufferData, 0x4C) = stripLengths;
    uint8_t result = Renderer_CreateGeometryBuffers(self, bufferData, data, nullptr, declaration);
    if (result) {
        AddPrecacheEntries(self, bufferData, data, nullptr, nullptr, declaration, 0, 0);
    }
    return result;
}

// 0x87F560 (thiscall): the same for a hardware-skinned geometry: per partition of its skin
// instance (0x2C each: +0x14/+0x18 the index data, +0x1C vertices, +0x1E triangles, +0x22 strips
// (0: one), +0x28 the buffer data), false if any partition's buffers couldn't be made
uint8_t __fastcall Renderer_PrecacheSkinnedGeometryBuffersImpl(uint8_t* self, re::Edx, void*, uint8_t* data,
                                                               uint8_t* skin, void* declaration,
                                                               uint32_t bonesPerPartition,
                                                               uint32_t bonesPerVertex) {
    uint8_t* skinPartition = At<uint8_t*>(skin, 0xC);
    uint32_t count = At<uint32_t>(skinPartition, 8);
    uint8_t result = 1;
    if (count == 0) {
        return result;
    }
    uint8_t* partition = At<uint8_t*>(skinPartition, 0xC);
    do {
        uint8_t* bufferData = At<uint8_t*>(partition, 0x28);
        if (!BufferData_IsVBValid(bufferData)) {
            uint16_t vertices = At<uint16_t>(partition, 0x1C);
            uint16_t strips = At<uint16_t>(partition, 0x22);
            uint16_t triangles = At<uint16_t>(partition, 0x1E);
            void* stripLists = At<void*>(partition, 0x14);
            void* stripLengths = At<void*>(partition, 0x18);
            At<uint32_t>(bufferData, 0x18) = vertices;
            At<uint32_t>(bufferData, 0x1C) = vertices;
            At<uint32_t>(bufferData, 0x48) = strips == 0 ? 1 : strips;
            At<uint32_t>(bufferData, 0x40) = triangles;
            At<uint32_t>(bufferData, 0x44) = triangles;
            At<void*>(bufferData, 0x4C) = stripLengths;
            At<void*>(bufferData, 0x50) = stripLists;
            if (!Renderer_CreateGeometryBuffers(self, bufferData, data, partition, declaration)) {
                result = 0;
            } else {
                AddPrecacheEntries(self, bufferData, data, skin, partition, declaration, bonesPerPartition,
                                   bonesPerVertex);
            }
        }
        partition += 0x2C;
    } while (--count != 0);
    return result;
}

// 0x87F6F0 (thiscall): NiDX9Renderer::CreateRenderedTextureRendererData (made unless the texture
// has one; kept in the map at +0x6C8). (The flags aren't read.)
uint8_t __fastcall Renderer_CreateRenderedTextureRendererData(uint8_t* self, re::Edx, uint8_t* texture, uint32_t) {
    if (At<void*>(texture, 0x24) == nullptr) {
        void* data = RenderedTextureData_Create(texture, self);
        if (data == nullptr) {
            return 0;
        }
        Map_SetAt(self + kRenderedTextures, reinterpret_cast<uint32_t>(texture), data);
    }
    return 1;
}

// 0x87F730 (thiscall): NiDX9Renderer::CreateRenderedCubeMapRendererData (the map at +0x6D8)
uint8_t __fastcall Renderer_CreateRenderedCubeMapRendererData(uint8_t* self, re::Edx, uint8_t* cubeMap) {
    if (At<void*>(cubeMap, 0x24) == nullptr) {
        void* data = RenderedCubeMapData_Create(cubeMap, self);
        if (data == nullptr) {
            return 0;
        }
        Map_SetAt(self + kRenderedCubeMaps, reinterpret_cast<uint32_t>(cubeMap), data);
    }
    return 1;
}

// 0x87F770 (thiscall): NiDX9Renderer::CreateDynamicTextureRendererData (made again when the
// texture's data isn't a dynamic one; the map at +0x6E8)
uint8_t __fastcall Renderer_CreateDynamicTextureRendererData(uint8_t* self, re::Edx, uint8_t* texture) {
    uint8_t* existing = At<uint8_t*>(texture, 0x24);
    if (existing == nullptr || AsDynamicTextureData(existing) == nullptr) {
        void* data = DynamicTextureData_Create(texture);
        if (data == nullptr) {
            return 0;
        }
        Map_SetAt(self + kDynamicTextures, reinterpret_cast<uint32_t>(texture), data);
    }
    return 1;
}

// 0x87F7C0 (thiscall): NiDX9Renderer::LockDynamicTexture (the bits, the pitch through `pitch`;
// with dynamic textures the one at +0x5C locked with D3DLOCK_DISCARD, else the data's own (slot
// 19); the data marked locked (slot 21); null when already locked or the lock fails)
void* __fastcall Renderer_LockDynamicTexture(uint8_t* self, re::Edx, uint8_t* rendererData, int32_t* pitch) {
    *pitch = 0;
    if (rendererData == nullptr) {
        return nullptr;
    }
    uint8_t* data = AsDynamicTextureData(rendererData);
    if (data == nullptr || Virtual<uint8_t>(data, 20)) {   // IsLocked
        return nullptr;
    }
    uint32_t flags = 0;
    void* texture;
    if (At<uint8_t>(self, kDynamicTexturesCapable) != 0) {
        texture = At<void*>(data, 0x5C);
        flags = 0x2000;   // D3DLOCK_DISCARD
    } else {
        texture = Virtual<void*>(data, 19);
    }
    if (texture == nullptr) {
        return nullptr;
    }
    struct {
        int32_t pitch;
        void* bits;
    } locked;   // D3DLOCKED_RECT
    long hr = Com<long>(texture, 19, 0u, &locked, static_cast<void*>(nullptr), flags);   // LockRect
    if (hr < 0) {
        NiOutputDebugString("NiDX9Renderer::LockDynamicTexture Failed> LockRect - %s", NiDX9ErrorString(hr));
        return nullptr;
    }
    *pitch = locked.pitch;
    Virtual<void>(data, 21, 1);   // SetLocked
    return locked.bits;
}

// 0x87F890 (thiscall): NiDX9Renderer::UnLockDynamicTexture (without dynamic textures the system
// copy is sent to the one at +0x5C with UpdateTexture)
uint8_t __fastcall Renderer_UnLockDynamicTexture(uint8_t* self, re::Edx, uint8_t* rendererData) {
    if (rendererData == nullptr) {
        return 0;
    }
    uint8_t* data = AsDynamicTextureData(rendererData);
    if (data == nullptr || !Virtual<uint8_t>(data, 20)) {
        return 0;
    }
    void* texture;
    if (At<uint8_t>(self, kDynamicTexturesCapable) != 0) {
        texture = At<void*>(data, 0x5C);
    } else {
        texture = Virtual<void*>(data, 19);
    }
    if (texture == nullptr) {
        return 0;
    }
    long hr = Com<long>(texture, 20, 0u);   // UnlockRect
    if (hr < 0) {
        NiOutputDebugString("NiDX9Renderer::UnLockDynamicTexture Failed> UnlockRect - %s", NiDX9ErrorString(hr));
        return 0;
    }
    if (At<uint8_t>(self, kDynamicTexturesCapable) == 0) {
        void* target = At<void*>(data, 0x5C);
        if (target == nullptr) {
            return 0;
        }
        hr = Com<long>(At<void*>(self, kDevice), 31, texture, target);   // UpdateTexture
        if (hr < 0) {
            NiOutputDebugString("NiDX9Renderer::UnLockDynamicTexture Failed> UpdateTexture - %s",
                                NiDX9ErrorString(hr));
            return 0;
        }
    }
    Virtual<void>(data, 21, 0);
    return 1;
}

// 0x87F970 (thiscall, slot 102): a rendered cube map's data forgotten (cleared, out of the map)
uint8_t __fastcall Renderer_RemoveRenderedCubeMapData(uint8_t* self, re::Edx, uint8_t* cubeMap) {
    At<void*>(cubeMap, 0x24) = nullptr;
    return Map_RemoveAt(self + kRenderedCubeMaps, reinterpret_cast<uint32_t>(cubeMap));
}

// 0x87F990 (thiscall, slot 103): a rendered texture's
uint8_t __fastcall Renderer_RemoveRenderedTextureData(uint8_t* self, re::Edx, uint8_t* texture) {
    At<void*>(texture, 0x24) = nullptr;
    return Map_RemoveAt(self + kRenderedTextures, reinterpret_cast<uint32_t>(texture));
}

// 0x87F9B0 (thiscall, slot 104): a dynamic texture's
uint8_t __fastcall Renderer_RemoveDynamicTextureData(uint8_t* self, re::Edx, uint8_t* texture) {
    At<void*>(texture, 0x24) = nullptr;
    return Map_RemoveAt(self + kDynamicTextures, reinterpret_cast<uint32_t>(texture));
}

// 0x87F9D0 (thiscall): whether a geometry is skinned on the device: never once its data has
// renderer data; skinned with a partition whose first piece has buffer data; when the renderer
// has the hardware skinning flag; else when the shader (the geometry's active one if none given)
// can (slot 32) with vertex shaders past 0xFFFE0000, or the device can switch to software and the
// shader's declaration (slot 26) says so (slot 25)
uint8_t __fastcall Renderer_IsHardwareSkinnedImpl(uint8_t* self, re::Edx, uint8_t* geometry, uint8_t* shader) {
    if (At<void*>(At<uint8_t*>(geometry, 0xCC), 0x3C) != nullptr) {
        return 0;
    }
    uint8_t* skin = At<uint8_t*>(geometry, 0xD0);
    if (skin == nullptr) {
        return 0;
    }
    uint8_t* skinPartition = At<uint8_t*>(skin, 0xC);
    if (skinPartition == nullptr) {
        return 0;
    }
    if (At<void*>(At<uint8_t*>(skinPartition, 0xC), 0x28) != nullptr) {
        return 1;
    }
    if (Virtual<uint32_t>(self, 19) & 2) {   // GetFlags: CAPS_HARDWARESKINNING
        return 1;
    }
    if (shader == nullptr) {
        shader = IsKindOf(NiD3DShaderInterface_RTTI, Geometry_GetActiveShader(geometry));
    }
    if (At<uint32_t>(self, kVertexShaderVersion) > 0xFFFE0000u && shader != nullptr &&
        Virtual<uint8_t>(shader, 32)) {
        return 1;
    }
    if (At<uint8_t>(self, kSWVertexSwitchable) != 0 && shader != nullptr && Virtual<void*>(shader, 26) != nullptr &&
        Virtual<uint8_t>(Virtual<void*>(shader, 26), 25)) {
        return 1;
    }
    return 0;
}

// 0x880F90 (thiscall, slot 46): NiDX9Renderer::PrecacheGeometry (tri-based geometry only; its
// shader found, through its material when it has none yet (the material put back); volatile data
// when skinned but not on the device; its declaration made; added to the default geometry group
// and its buffers queued, under the precache lock)
uint8_t __fastcall Renderer_PrecacheGeometry(uint8_t* self, re::Edx, uint8_t* geometry, uint32_t bonesPerPartition,
                                             uint32_t bonesPerVertex) {
    if (geometry == nullptr || !HasClassBit(geometry, NiTriBasedGeom_ClassBit)) {
        return 0;
    }
    uint8_t* data = At<uint8_t*>(geometry, 0xCC);
    uint8_t* skin = At<uint8_t*>(geometry, 0xD0);
    if (IsKindOf(NiD3DShaderInterface_RTTI, Geometry_GetShader(geometry)) == nullptr) {
        void* material = Geometry_GetActiveMaterial(geometry);
        Geometry_ApplyAndSetActiveMaterial(geometry, At<void*>(self, 0x18), 0xFFFFFFFF);
        uint8_t* shader = IsKindOf(NiD3DShaderInterface_RTTI, Geometry_GetShader(geometry));
        Geometry_SetActiveMaterial(geometry, material);
        if (shader == nullptr) {
            NiOutputDebugString("PrecacheGeometry> No shader found for object %s!\n", At<const char*>(geometry, 8));
            return 0;
        }
    }
    uint8_t hwSkinned = Renderer_IsHardwareSkinned(self, geometry, nullptr);
    if (skin != nullptr && !hwSkinned) {
        At<uint16_t>(data, 0x34) = uint16_t((At<uint16_t>(data, 0x34) & 0xFFF) | 0x8000);   // VOLATILE
    }
    if ((At<uint16_t>(data, 0x34) & 0xF000) == 0x8000) {
        return 1;
    }
    Renderer_LockPrecache(self);
    void* declaration = At<void*>(geometry, 0xE4);
    if (declaration == nullptr) {
        ShaderDeclaration_CreateDeclForGeometry(geometry);
        declaration = At<void*>(geometry, 0xE4);
    }
    GroupManager_AddObjectToGroup(At<void*>(self, kGroupManager), At<void*>(self, kDefaultGroup), data, skin,
                                  hwSkinned, bonesPerPartition, bonesPerVertex);
    uint8_t result = 0;
    if (hwSkinned) {
        result = Renderer_PrecacheSkinnedGeometryBuffers(self, geometry, data, skin, declaration, bonesPerPartition,
                                                         bonesPerVertex);
        Renderer_UnlockPrecache(self);
        return result;
    }
    if (IsKindOfRTTI(NiTriShape_RTTI, geometry)) {
        result = Renderer_PrecacheGeometryBuffers(self, geometry, data, At<uint16_t>(data, 0x42),
                                                  At<uint16_t>(data, 0x40), At<void*>(data, 0x48), nullptr, 1,
                                                  declaration);
    } else if (IsKindOfRTTI(NiTriStrips_RTTI, geometry)) {
        result = Renderer_PrecacheGeometryBuffers(self, geometry, data, At<uint16_t>(data, 0x42),
                                                  At<uint16_t>(data, 0x40), At<void*>(data, 0x4C),
                                                  At<void*>(data, 0x48), At<uint16_t>(data, 0x44), declaration);
    }
    Renderer_UnlockPrecache(self);
    return result;
}

// 0x8811A0 (thiscall, slots 95 and 96): NiDX9Renderer::Do_RenderShape / Do_RenderTristrips (not
// while the device is lost or with no active vertices: the data added to its geometry group
// (the dynamic one when volatile), then DrawSkinnedPrimitive (slot 101) or DrawPrimitive (100))
void __fastcall Renderer_Do_RenderShape(uint8_t* self, re::Edx, uint8_t* geometry) {
    uint8_t* data = At<uint8_t*>(geometry, 0xCC);
    if (At<uint8_t>(self, kDeviceLost) != 0 || At<uint16_t>(data, 0x30) == 0) {
        return;
    }
    uint8_t* skin = At<uint8_t*>(geometry, 0xD0);
    void* group;
    if (skin == nullptr) {
        group = At<void*>(self, kDefaultGroup);
        if ((At<uint16_t>(data, 0x34) & 0xF000) == 0x8000) {
            group = At<void*>(self, kDynamicGroup);
        }
    } else {
        if (Renderer_IsHardwareSkinned(self, geometry, nullptr)) {
            group = At<void*>(self, kDefaultGroup);
            if ((At<uint16_t>(data, 0x34) & 0xF000) == 0x8000) {
                group = At<void*>(self, kDynamicGroup);
            }
            GroupManager_AddObjectToGroup(At<void*>(self, kGroupManager), group, data, skin, 1, 0, 0);
            Virtual<void>(self, 101, geometry, data, skin);   // DrawSkinnedPrimitive
            return;
        }
        group = At<void*>(self, kDynamicGroup);
    }
    GroupManager_AddObjectToGroup(At<void*>(self, kGroupManager), group, data, skin, 0, 0, 0);
    Virtual<void>(self, 100, geometry, data, static_cast<void*>(nullptr), geometry + 0x60, geometry + 0x1C,
                  At<void*>(data, 0x3C));   // DrawPrimitive(world, world bound, buffer data)
}

// 0x881290 (thiscall): NiDX9Renderer::AddToBatch (the batch's shader taken from the first
// geometry, found through its material if need be with the batch's property and effect states set
// on it for the lookup; the data added to its group; a batch node per partition when skinned on the
// device, else one). (The game passes AddObjectToGroup's flag as a whole dword whose upper bytes
// are the skin instance's: only its low byte means anything.)
void __fastcall Renderer_AddToBatch(uint8_t* self, re::Edx, uint8_t* geometry) {
    uint8_t* data = At<uint8_t*>(geometry, 0xCC);
    uint8_t* skin = At<uint8_t*>(geometry, 0xD0);
    if (At<void*>(self, kBatchShader) == nullptr) {
        void* propertyState = At<void*>(geometry, 0xC4);
        AddRefRef(propertyState);
        void* effectState = At<void*>(geometry, 0xC8);
        AddRefRef(effectState);
        Geometry_SetPropertyState(geometry, At<void*>(self, kBatchPropertyState));
        Geometry_SetEffectState(geometry, At<void*>(self, kBatchEffectState));
        uint8_t* shader = CastToD3DShader(Geometry_GetShader(geometry));
        uint8_t* old = At<uint8_t*>(self, kBatchShader);
        if (old != shader) {
            ReleaseRef(old);
            At<uint8_t*>(self, kBatchShader) = shader;
            AddRefRef(shader);
        }
        if (At<void*>(self, kBatchShader) == nullptr) {
            void* material = Geometry_GetActiveMaterial(geometry);
            Geometry_ApplyAndSetActiveMaterial(geometry, At<void*>(self, 0x18), 0xFFFFFFFF);
            NiPointer_Assign(&At<void*>(self, kBatchShader),
                             IsKindOf(NiD3DShaderInterface_RTTI, Geometry_GetShader(geometry)));
            Geometry_SetActiveMaterial(geometry, material);
        }
        Geometry_SetPropertyState(geometry, propertyState);
        Geometry_SetEffectState(geometry, effectState);
        if (At<void*>(self, kBatchShader) == nullptr) {
            NiOutputDebugString("AddToBatch> No shader found for object %s!\n", At<const char*>(geometry, 8));
            ReleaseRef(effectState);
            ReleaseRef(propertyState);
            return;
        }
        ReleaseRef(effectState);
        ReleaseRef(propertyState);
    }
    if (At<uint16_t>(data, 0x30) == 0) {
        return;
    }
    uint8_t hwSkinned = Renderer_IsHardwareSkinned(self, geometry, At<void*>(self, kBatchShader));
    void* group;
    if ((At<uint16_t>(data, 0x34) & 0xF000) == 0x8000 || (skin != nullptr && !hwSkinned)) {
        group = At<void*>(self, kDynamicGroup);
    } else {
        group = At<void*>(self, kDefaultGroup);
    }
    GroupManager_AddObjectToGroup(At<void*>(self, kGroupManager), group, data, skin, hwSkinned, 0, 0);
    if (!hwSkinned) {
        AppendBatchNode(self, geometry, nullptr, At<void*>(data, 0x3C));
        return;
    }
    uint8_t* skinPartition = At<uint8_t*>(skin, 0xC);
    uint8_t* partition = At<uint8_t*>(skinPartition, 0xC);
    uint32_t count = At<uint32_t>(skinPartition, 8);
    if (count == 0) {
        return;
    }
    do {
        AppendBatchNode(self, geometry, partition, At<void*>(partition, 0x28));
        partition += 0x2C;
    } while (--count != 0);
}

// 0x8815E0 (thiscall): NiDX9Renderer::ReleaseD3DShader (its renderer cleared, out of the list)
void __fastcall Renderer_ReleaseD3DShader(uint8_t* self, re::Edx, re::Ptr32<void> shader) {
    D3DShaderInterface_SetD3DRenderer(shader, nullptr);
    ShaderList_Remove(self + kD3DShaders, &shader);
}

// 0x881610 (thiscall): NiDX9Renderer::PurgeAllD3DShaders (each one's renderer data let go (slot
// 30) and renderer cleared, then taken out of the list, from the head)
void __fastcall Renderer_PurgeAllD3DShaders(uint8_t* self, re::Edx) {
    uint8_t* list = self + kD3DShaders;
    while (At<uint32_t>(self, 0x718) != 0) {
        void* shader = At<void*>(At<uint8_t*>(self, 0x710), 8);
        Virtual<void>(shader, 30);
        D3DShaderInterface_SetD3DRenderer(shader, nullptr);
        re::Ptr32<void> position = nullptr;
        for (uint8_t* node = At<uint8_t*>(list, 4); node != nullptr; node = At<uint8_t*>(node, 0)) {
            if (At<void*>(node, 8) == shader) {
                position = node;
                break;
            }
        }
        if (position != nullptr) {
            ShaderList_RemovePos(list, &position);
        }
    }
}

// 0x881720 (thiscall): a screen texture's buffer data readied for drawing (the format flags set,
// one stream; its group asked to make the buffers when they're missing (all dirty), or whether they
// were lost; then the dirty parts regenerated (0x87EE80) and the dirty flags cleared)
void __fastcall Renderer_PrepareScreenTextureDataImpl(uint8_t* self, re::Edx, uint8_t* screenTexture) {
    if (screenTexture == nullptr) {
        return;
    }
    uint8_t* bufferData = At<uint8_t*>(screenTexture, 0x1C);
    uint8_t valid = BufferData_IsVBValid(bufferData);
    At<uint32_t>(bufferData, 4) = 0x1400000;
    BufferData_SetStreamCount(bufferData, 1);
    void* group = At<void*>(bufferData, 8);
    uint32_t dirty = At<uint16_t>(screenTexture, 0x18);
    if (!valid) {
        Virtual<void>(group, 7, bufferData, 0);   // the buffers made
        At<uint32_t>(bufferData, 0x38) = 0;
        dirty = 0xF;
    } else if (Virtual<uint8_t>(group, 8)) {   // lost
        dirty = 0xF;
    } else if (uint16_t(dirty) == 0) {
        At<uint16_t>(screenTexture, 0x18) = 0;
        return;
    }
    Renderer_GenerateScreenTextureBuffers(self, screenTexture, dirty);
    At<uint16_t>(screenTexture, 0x18) = 0;
}

// 0x882320 (thiscall, slot 99): NiDX9Renderer::Do_RenderScreenTexture (the screen texture
// property state made on first use: vertex colors, alpha blending, no z buffer; the texture's own
// texturing property set on it; drawn with the identity transform through DrawPrimitive; the
// default texturing property put back and the property / effect states restored). (The bound it
// passes is its own uninitialised stack.)
void __fastcall Renderer_Do_RenderScreenTexture(uint8_t* self, re::Edx, uint8_t* screenTexture) {
    if (At<uint8_t>(self, kDeviceLost) != 0 || screenTexture == nullptr || At<void*>(screenTexture, 0x10) == nullptr) {
        return;
    }
    void* savedProperties = At<void*>(self, 0xC);
    void* savedEffects = At<void*>(self, 0x10);
    re::Ptr32<void>* state = &At<void*>(self, kScreenTexturePropertyState);
    if (*state == nullptr) {
        void* memory = NiNew(0x38);
        NiPointer_Assign(state, memory != nullptr ? PropertyState_Construct(memory) : nullptr);
        memory = NiNew(0x1C);
        uint8_t* vertexColor = memory != nullptr ? static_cast<uint8_t*>(VertexColorProperty_Construct(memory)) : nullptr;
        At<uint16_t>(vertexColor, 0x18) = uint16_t((At<uint16_t>(vertexColor, 0x18) & 0xFFD7) | 0x10);
        PropertyState_SetProperty(*state, vertexColor);
        memory = NiNew(0x1C);
        uint8_t* alpha = memory != nullptr ? static_cast<uint8_t*>(AlphaProperty_Construct(memory)) : nullptr;
        At<uint16_t>(alpha, 0x18) = uint16_t((At<uint16_t>(alpha, 0x18) & 0xFEED) | 0xED);
        PropertyState_SetProperty(*state, alpha);
        memory = NiNew(0x30);
        uint8_t* zbuffer = memory != nullptr ? static_cast<uint8_t*>(ZBufferProperty_Construct(memory)) : nullptr;
        At<uint16_t>(zbuffer, 0x18) &= 0xFFFC;
        PropertyState_SetProperty(*state, zbuffer);
    }
    PropertyState_SetProperty(*state, At<void*>(screenTexture, 0x14));
    At<void*>(self, 0xC) = *state;
    At<void*>(self, 0x10) = nullptr;
    Virtual<void>(At<void*>(self, kGroupManager), 4, At<void*>(self, kDefaultGroup), screenTexture);
    Renderer_PrepareScreenTextureData(self, screenTexture);
    struct {
        uint32_t rotate[9];
        uint32_t translate[3];
        float scale;
    } world;   // NiTransform
    for (int i = 0; i < 9; i++) {
        world.rotate[i] = NiMatrix3_IDENTITY[i];
    }
    world.scale = 1.0f;
    world.translate[1] = NiPoint3_ZERO[1];
    world.translate[2] = NiPoint3_ZERO[2];
    world.translate[0] = NiPoint3_ZERO[0];
    uint32_t bound[4];   // (uninitialised in the game too)
    Virtual<void>(self, 100, static_cast<void*>(nullptr), static_cast<void*>(nullptr), static_cast<void*>(nullptr),
                  &world, bound, At<void*>(screenTexture, 0x1C));
    PropertyState_SetProperty(*state, NiTexturingProperty_ms_spDefault);
    At<void*>(self, 0xC) = savedProperties;
    At<void*>(self, 0x10) = savedEffects;
}

#define C39A_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C39A_REVERSED("NiDX9Renderer::PrecacheGeometryBuffers", 0x0087F450, Renderer_PrecacheGeometryBuffersImpl);
C39A_REVERSED("NiDX9Renderer::PrecacheSkinnedGeometryBuffers", 0x0087F560,
              Renderer_PrecacheSkinnedGeometryBuffersImpl);
C39A_REVERSED("NiDX9Renderer::CreateRenderedTextureRendererData", 0x0087F6F0,
              Renderer_CreateRenderedTextureRendererData);
C39A_REVERSED("NiDX9Renderer::CreateRenderedCubeMapRendererData", 0x0087F730,
              Renderer_CreateRenderedCubeMapRendererData);
C39A_REVERSED("NiDX9Renderer::CreateDynamicTextureRendererData", 0x0087F770,
              Renderer_CreateDynamicTextureRendererData);
C39A_REVERSED("NiDX9Renderer::LockDynamicTexture", 0x0087F7C0, Renderer_LockDynamicTexture);
C39A_REVERSED("NiDX9Renderer::UnLockDynamicTexture", 0x0087F890, Renderer_UnLockDynamicTexture);
C39A_REVERSED("NiDX9Renderer::RemoveRenderedCubeMapData", 0x0087F970, Renderer_RemoveRenderedCubeMapData);
C39A_REVERSED("NiDX9Renderer::RemoveRenderedTextureData", 0x0087F990, Renderer_RemoveRenderedTextureData);
C39A_REVERSED("NiDX9Renderer::RemoveDynamicTextureData", 0x0087F9B0, Renderer_RemoveDynamicTextureData);
C39A_REVERSED("NiDX9Renderer::IsHardwareSkinned", 0x0087F9D0, Renderer_IsHardwareSkinnedImpl);
C39A_REVERSED("NiDX9Renderer::PrecacheGeometry", 0x00880F90, Renderer_PrecacheGeometry);
C39A_REVERSED("NiDX9Renderer::Do_RenderShape", 0x008811A0, Renderer_Do_RenderShape);
C39A_REVERSED("NiDX9Renderer::AddToBatch", 0x00881290, Renderer_AddToBatch);
C39A_REVERSED("NiDX9Renderer::ReleaseD3DShader", 0x008815E0, Renderer_ReleaseD3DShader);
C39A_REVERSED("NiDX9Renderer::PurgeAllD3DShaders", 0x00881610, Renderer_PurgeAllD3DShaders);
C39A_REVERSED("NiDX9Renderer::PrepareScreenTextureData", 0x00881720, Renderer_PrepareScreenTextureDataImpl);
C39A_REVERSED("NiDX9Renderer::Do_RenderScreenTexture", 0x00882320, Renderer_Do_RenderScreenTexture);
// slots 93 and 94 (Do_BatchRenderShape / Do_BatchRenderStrips): a lone jmp to AddToBatch
RE_UNHOOKED("NiDX9Renderer::Do_BatchRenderShape", 0x008824E0);

} // namespace ni::c39
