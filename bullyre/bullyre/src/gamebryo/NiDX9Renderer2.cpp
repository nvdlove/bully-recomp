// NiDX9Renderer's small virtuals and helpers (NiDX9Renderer.h): the default target group's
// buffers, the mipmap skip level, the purge slots (effects, skin instances, screen textures),
// shaders let go, the texture renderer data made under the precache lock, the legacy default
// material, the renderer / source data locks, SaveScreenShot, and the folded map / geometry
// helpers the renderer's code calls (NiTMap<unsigned int, NiPointer> SetAt / GetNext,
// NiGeometry's property / effect state setters).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::dx9 {
namespace {

constexpr uintptr_t kD3DXSaveSurfaceToFileA = 0x008FC320;

GAME_FN(void, __thiscall, LightManager_RemoveLight, 0x00892410, void* self, void* light);
GAME_FN(void, __cdecl, NiAlignedFree, 0x007523A0, void* p);
GAME_FN(void, __cdecl, OperatorDelete, 0x005EEAB0, void* p);
GAME_FN(void*, __cdecl, SourceTextureData_Create, 0x008968F0, void* texture, void* renderer);
GAME_FN(void*, __cdecl, SourceCubeMapData_Create, 0x00897D40, void* texture, void* renderer);
// NiFilename(path), GetFullPath, NiPath::Standardize, NiFile::CreateDirectoryRecursive
GAME_FN(void, __thiscall, Filename_Construct, 0x007538C0, void* self, const char* path);
GAME_FN(void, __thiscall, Filename_GetFullPath, 0x00753AD0, void* self, char* buffer, uint32_t size);
GAME_FN(void, __cdecl, Path_Standardize, 0x00752C20, char* path);
GAME_FN(uint8_t, __cdecl, File_CreateDirectoryRecursive, 0x007534E0, const char* path);

// NiTMapBase's layout (the vtable: 1 SetValue, 3 NewItem)
struct MapItem {
    re::Ptr32<MapItem> next;   // +0x0
    uint32_t key;    // +0x4
    re::Ptr32<void> value;     // +0x8 (a reference)
};
struct Map {
    re::Ptr32<void> vtable;        // +0x0
    uint32_t hashSize;   // +0x4
    re::Ptr32<re::Ptr32<MapItem>> table;     // +0x8
    uint32_t count;      // +0xC
};
static_assert(sizeof(Map) == 0x10);

// NiTPointerList (the allocator's slot 1 makes a node)
struct ListNode {
    re::Ptr32<ListNode> next;   // +0x0
    re::Ptr32<ListNode> prev;   // +0x4
    re::Ptr32<void> element;    // +0x8
};
struct List {
    re::Ptr32<void> vtable;      // +0x0
    re::Ptr32<ListNode> head;    // +0x4
    re::Ptr32<ListNode> tail;    // +0x8
    uint32_t count;    // +0xC
};

} // namespace

// 0x879540 (thiscall): NiDX9Renderer::GetDefaultRenderTargetGroup
void* __fastcall Renderer_GetDefaultRenderTargetGroup(uint8_t* self, re::Edx) {
    return At<void*>(self, kDefaultRenderTargetGroup);
}

// 0x8796F0 (thiscall): NiDX9Renderer::GetDefaultDepthStencilBuffer (the default group's)
void* __fastcall Renderer_GetDefaultDepthStencilBuffer(uint8_t* self, re::Edx) {
    return Virtual<void*>(At<void*>(self, kDefaultRenderTargetGroup), 28);   // GetDepthStencilBuffer
}

// 0x879700 (thiscall): NiDX9Renderer::GetDefaultBackBuffer (the default group's first buffer)
void* __fastcall Renderer_GetDefaultBackBuffer(uint8_t* self, re::Edx) {
    return Virtual<void*>(At<void*>(self, kDefaultRenderTargetGroup), 27, 0u);   // GetBuffer
}

// 0x879710 (thiscall): NiDX9Renderer::SetMipmapSkipLevel (NiDX9SourceTextureData's static)
uint8_t __fastcall Renderer_SetMipmapSkipLevel(uint8_t*, re::Edx, uint32_t skip) {
    ms_uiSkipLevels = skip;
    return 1;
}

// 0x879720 (thiscall): NiDX9Renderer::GetMipmapSkipLevel (6 bytes)
uint32_t __fastcall Renderer_GetMipmapSkipLevel(uint8_t*, re::Edx) {
    return ms_uiSkipLevels;
}

// 0x87A170 (thiscall): NiDX9Renderer::PurgeEffect (a light leaves the light manager)
void __fastcall Renderer_PurgeEffect(uint8_t* self, re::Edx, void* effect) {
    if (effect == nullptr) {
        return;
    }
    void* light = HasClassBit(effect, NiLight_ClassBit) ? effect : nullptr;
    if (light != nullptr) {
        LightManager_RemoveLight(At<void*>(self, kLightManager), light);
    }
}

// 0x87A1C0 (thiscall): NiDX9Renderer::PurgeScreenTextureRendererData (NiScreenTexture's
// destructor's: one with renderer data, +0x1C: the manager at +0x6A8's slot 6, tail-called)
void __fastcall Renderer_PurgeScreenTextureRendererData(uint8_t* self, re::Edx, uint8_t* screenTexture) {
    if (screenTexture != nullptr && At<void*>(screenTexture, 0x1C) != nullptr) {
        Virtual<void>(At<void*>(self, kManager6A8), 6, screenTexture);
    }
}

// 0x87A1F0 (thiscall): NiDX9Renderer::PurgeSkinInstance (~NiSkinInstance's: its bone matrices,
// +0x28 aligned, and +0x2C freed)
void __fastcall Renderer_PurgeSkinInstance(uint8_t*, re::Edx, uint8_t* skin) {
    NiAlignedFree(At<void*>(skin, 0x28));
    At<void*>(skin, 0x28) = nullptr;
    At<uint32_t>(skin, 0x24) = 0;
    OperatorDelete(At<void*>(skin, 0x2C));
    At<void*>(skin, 0x2C) = nullptr;
}

// 0x87A230 (thiscall): NiDX9Renderer::ReleaseVertexShader (unset when current; its handle
// released and cleared)
void __fastcall Renderer_ReleaseVertexShader(uint8_t* self, re::Edx, void* shader) {
    uint8_t* state = At<uint8_t*>(self, kRenderState);
    if (At<void*>(state, 0x20E0) != nullptr) {
        At<void*>(state, 0x20E0) = nullptr;
        Com<long>(At<void*>(state, 0x2238), 92, static_cast<void*>(nullptr));   // SetVertexShader
    }
    void* handle = Virtual<void*>(shader, 18);   // GetShaderHandle
    if (handle != nullptr) {
        Com<unsigned long>(handle, kRelease);
    }
    Virtual<void>(shader, 19, static_cast<void*>(nullptr));   // SetShaderHandle
}

// 0x87A290 (thiscall): NiDX9Renderer::ReleasePixelShader
void __fastcall Renderer_ReleasePixelShader(uint8_t* self, re::Edx, void* shader) {
    uint8_t* state = At<uint8_t*>(self, kRenderState);
    if (At<void*>(state, 0x20E8) != nullptr) {
        At<void*>(state, 0x20E8) = nullptr;
        Com<long>(At<void*>(state, 0x2238), 107, static_cast<void*>(nullptr));   // SetPixelShader
    }
    void* handle = Virtual<void*>(shader, 16);
    if (handle != nullptr) {
        Com<unsigned long>(handle, kRelease);
    }
    Virtual<void>(shader, 17, static_cast<void*>(nullptr));
}

// 0x87AB40 (thiscall): NiDX9Renderer::CreateSourceTextureRendererData (under the precache lock)
uint8_t __fastcall Renderer_CreateSourceTextureRendererData(uint8_t* self, re::Edx, uint8_t* texture) {
    NiCriticalSection& lock = At<NiCriticalSection>(self, kPrecacheLock);
    Lock(lock);
    void* data = At<void*>(texture, 0x24);
    if (data == nullptr) {
        data = SourceTextureData_Create(texture, self);
    }
    Unlock(lock);
    return data != nullptr ? 1 : 0;
}

// 0x87ABA0 (thiscall): NiDX9Renderer::CreateSourceCubeMapRendererData (one that has data already
// returns with the lock still held: the game's)
uint8_t __fastcall Renderer_CreateSourceCubeMapRendererData(uint8_t* self, re::Edx, uint8_t* texture) {
    NiCriticalSection& lock = At<NiCriticalSection>(self, kPrecacheLock);
    Lock(lock);
    if (At<void*>(texture, 0x24) != nullptr) {
        return 1;
    }
    void* data = SourceCubeMapData_Create(texture, self);
    Unlock(lock);
    return data != nullptr ? 1 : 0;
}

// 0x87ACE0 (thiscall): NiDX9Renderer::UseLegacyPipelineAsDefaultMaterial (NiRenderer's current
// default material: the legacy one)
void __fastcall Renderer_UseLegacyPipelineAsDefaultMaterial(uint8_t* self, re::Edx) {
    AssignRef(At<void*>(self, 0x18), At<void*>(self, kLegacyDefaultMaterial));
}

// 0x87AD30 (thiscall): the renderer lock (its first taker calls Do_LockRenderer) and the source
// data lock taken
void __fastcall Renderer_LockRendererAndSourceData(uint8_t* self, re::Edx) {
    NiCriticalSection& renderer = At<NiCriticalSection>(self, kRendererLock);
    Import<void>(ni::sys::kEnterCriticalSection, &renderer.section);
    renderer.owner = Import<uint32_t>(ni::sys::kGetCurrentThreadId);
    renderer.lockCount++;
    if (renderer.lockCount == 1) {
        Virtual<void>(self, 80);   // Do_LockRenderer
    }
    Lock(At<NiCriticalSection>(self, kSourceDataLock));
}

// 0x87AD80 (thiscall): the reverse (the last holder calls Do_UnlockRenderer)
void __fastcall Renderer_UnlockRendererAndSourceData(uint8_t* self, re::Edx) {
    Unlock(At<NiCriticalSection>(self, kSourceDataLock));
    NiCriticalSection& renderer = At<NiCriticalSection>(self, kRendererLock);
    if (renderer.lockCount == 1) {
        Virtual<void>(self, 81);   // Do_UnlockRenderer
    }
    Unlock(renderer);
}

// 0x87ADD0 (thiscall): NiTMap<unsigned int, NiPointer<T>>::SetAt (the value by value: this one
// releases it)
void __fastcall PointerMap_SetAt(Map* self, re::Edx, uint32_t key, void* value) {
    uint32_t index = key % self->hashSize;
    for (MapItem* item = self->table[index]; item != nullptr; item = item->next) {
        if (item->key == key) {
            AssignRef(item->value, value);
            ReleaseRef(value);
            return;
        }
    }
    MapItem* item = Virtual<MapItem*>(self, 3);   // NewItem
    AddRefRef(value);   // (SetValue's argument by value)
    Virtual<void>(self, 1, item, key, value);   // SetValue
    item->next = self->table[index];
    self->table[index] = item;
    self->count++;
    ReleaseRef(value);
}

// 0x87AEC0 (thiscall): NiTMap<unsigned int, NiPointer<T>>::GetNext (the key, the value held, the
// position moved on to the next item: in its bucket or the next bucket that has one)
void __fastcall PointerMap_GetNext(Map* self, re::Edx, re::Ptr32<MapItem>* position, uint32_t* key, re::Ptr32<void>* value) {
    MapItem* item = *position;
    *key = item->key;
    AssignRef(*value, item->value);
    if (item->next != nullptr) {
        *position = item->next;
        return;
    }
    for (uint32_t i = item->key % self->hashSize + 1; i < self->hashSize; i++) {
        if (self->table[i] != nullptr) {
            *position = self->table[i];
            return;
        }
    }
    *position = nullptr;
}

// 0x87AF70 (thiscall): NiGeometry::SetPropertyState (+0xC4, a reference)
void __fastcall Geometry_SetPropertyState(uint8_t* self, re::Edx, void* state) {
    AssignRef(At<void*>(self, 0xC4), state);
}

// 0x87AFC0 (thiscall): NiGeometry::SetEffectState (+0xC8)
void __fastcall Geometry_SetEffectState(uint8_t* self, re::Edx, void* state) {
    AssignRef(At<void*>(self, 0xC8), state);
}

namespace {
// NiTPrimitiveArray::RemoveAll, inlined (the slots zeroed by a 16-bit counter)
inline void ClearArray(uint8_t* array) {
    for (uint16_t i = 0; i < At<uint16_t>(array, 0xA);) {
        At<re::Ptr32<void>*>(array, 4)[i++] = nullptr;
    }
    At<uint16_t>(array, 0xA) = 0;
    At<uint16_t>(array, 0xC) = 0;
}
} // namespace

// 0x87B010 (thiscall): the renderer's first two batch arrays emptied (+0x8A8, +0x8B8)
void __fastcall Renderer_ClearBatchArraysA(uint8_t* self, re::Edx) {
    ClearArray(self + 0x8A8);
    ClearArray(self + 0x8B8);
}

// 0x87B080 (thiscall): the other two (+0x8C8, +0x8D8)
void __fastcall Renderer_ClearBatchArraysB(uint8_t* self, re::Edx) {
    ClearArray(self + 0x8C8);
    ClearArray(self + 0x8D8);
}

// 0x87EE30 (thiscall): NiDX9Renderer::RegisterD3DShader (a D3D shader put at the head of the
// renderer's NiTPointerList at +0x70C, the shaders it keeps)
void __fastcall Renderer_RegisterD3DShader(uint8_t* self, re::Edx, void* element) {
    List* list = reinterpret_cast<List*>(self + 0x70C);
    ListNode* node = Virtual<ListNode*>(list, 1);   // the allocator's new node
    node->element = element;
    node->prev = nullptr;
    node->next = list->head;
    if (list->head != nullptr) {
        list->head->prev = node;
        list->count++;
        list->head = node;
        return;
    }
    list->count++;
    list->tail = node;
    list->head = node;
}

// 0x879550 (thiscall): NiDX9Renderer::SaveScreenShot (format 1 PNG, 2 JPEG; the directory made
// first; the back buffer through D3DX)
uint8_t __fastcall Renderer_SaveScreenShot(uint8_t* self, re::Edx, const char* filename, int format) {
    uint32_t d3dxFormat;
    if (format == 1) {
        d3dxFormat = 3;   // D3DXIFF_PNG
    } else if (format == 2) {
        d3dxFormat = 1;   // D3DXIFF_JPG
    } else {
        NiOutputDebugString("NiDX9Renderer::SaveScreenShot> Unsupported image format %d\n", format);
        return 0;
    }
    alignas(4) uint8_t name[0x404];   // NiFilename: drive, dir, fname (+0x103), ext (+0x203)...
    char path[0x104];
    Filename_Construct(name, filename);
    crt::StrcpyS(reinterpret_cast<char*>(name + 0x203), 0x100, "");
    crt::StrcpyS(reinterpret_cast<char*>(name + 0x103), 0x100, "");
    Filename_GetFullPath(name, path, 0x104);
    Path_Standardize(path);
    if (path[0] != '\0' && File_CreateDirectoryRecursive(path) == 0) {
        NiOutputDebugString("NiDX9Renderer::SaveScreenShot> Could not create directory (\"%s\")\n", path);
        return 0;
    }
    alignas(4) uint8_t full[0x404];
    Filename_Construct(full, filename);
    Filename_GetFullPath(full, path, 0x104);
    Path_Standardize(path);
    void* surface = nullptr;
    if (Com<long>(At<void*>(self, kDevice), kDevGetBackBuffer, 0u, 0u, 0u, &surface) < 0) {
        return 0;
    }
    long hr = Import<long>(kD3DXSaveSurfaceToFileA, static_cast<const char*>(path), d3dxFormat, surface,
                           static_cast<const void*>(nullptr), static_cast<const void*>(nullptr));
    Com<unsigned long>(surface, kRelease);
    return hr >= 0 ? 1 : 0;
}

#define DX9REND38B_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

DX9REND38B_REVERSED("NiDX9Renderer::GetDefaultRenderTargetGroup", 0x00879540, Renderer_GetDefaultRenderTargetGroup);
DX9REND38B_REVERSED("NiDX9Renderer::SaveScreenShot", 0x00879550, Renderer_SaveScreenShot);
DX9REND38B_REVERSED("NiDX9Renderer::GetDefaultDepthStencilBuffer", 0x008796F0, Renderer_GetDefaultDepthStencilBuffer);
DX9REND38B_REVERSED("NiDX9Renderer::GetDefaultBackBuffer", 0x00879700, Renderer_GetDefaultBackBuffer);
DX9REND38B_REVERSED("NiDX9Renderer::SetMipmapSkipLevel", 0x00879710, Renderer_SetMipmapSkipLevel);
DX9REND38B_REVERSED("NiDX9Renderer::GetMipmapSkipLevel", 0x00879720, Renderer_GetMipmapSkipLevel);
DX9REND38B_REVERSED("NiDX9Renderer::PurgeEffect", 0x0087A170, Renderer_PurgeEffect);
DX9REND38B_REVERSED("NiDX9Renderer::PurgeScreenTextureRendererData", 0x0087A1C0,
                    Renderer_PurgeScreenTextureRendererData);
DX9REND38B_REVERSED("NiDX9Renderer::PurgeSkinInstance", 0x0087A1F0, Renderer_PurgeSkinInstance);
DX9REND38B_REVERSED("NiDX9Renderer::ReleaseVertexShader", 0x0087A230, Renderer_ReleaseVertexShader);
DX9REND38B_REVERSED("NiDX9Renderer::ReleasePixelShader", 0x0087A290, Renderer_ReleasePixelShader);
DX9REND38B_REVERSED("NiDX9Renderer::CreateSourceTextureRendererData", 0x0087AB40,
                    Renderer_CreateSourceTextureRendererData);
DX9REND38B_REVERSED("NiDX9Renderer::CreateSourceCubeMapRendererData", 0x0087ABA0,
                    Renderer_CreateSourceCubeMapRendererData);
DX9REND38B_REVERSED("NiDX9Renderer::UseLegacyPipelineAsDefaultMaterial", 0x0087ACE0,
                    Renderer_UseLegacyPipelineAsDefaultMaterial);
DX9REND38B_REVERSED("NiDX9Renderer::LockRendererAndSourceData", 0x0087AD30, Renderer_LockRendererAndSourceData);
DX9REND38B_REVERSED("NiDX9Renderer::UnlockRendererAndSourceData", 0x0087AD80, Renderer_UnlockRendererAndSourceData);
DX9REND38B_REVERSED("NiTMap<unsigned int,NiPointer>::SetAt_87ADD0", 0x0087ADD0, PointerMap_SetAt);
DX9REND38B_REVERSED("NiTMap<unsigned int,NiPointer>::GetNext_87AEC0", 0x0087AEC0, PointerMap_GetNext);
DX9REND38B_REVERSED("NiGeometry::SetPropertyState", 0x0087AF70, Geometry_SetPropertyState);
DX9REND38B_REVERSED("NiGeometry::SetEffectState", 0x0087AFC0, Geometry_SetEffectState);
DX9REND38B_REVERSED("NiDX9Renderer::ClearBatchArraysA", 0x0087B010, Renderer_ClearBatchArraysA);
DX9REND38B_REVERSED("NiDX9Renderer::ClearBatchArraysB", 0x0087B080, Renderer_ClearBatchArraysB);
DX9REND38B_REVERSED("NiDX9Renderer::RegisterD3DShader", 0x0087EE30, Renderer_RegisterD3DShader);

} // namespace ni::dx9
