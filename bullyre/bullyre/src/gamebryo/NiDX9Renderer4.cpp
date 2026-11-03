// NiDX9Renderer's buffer copies, screenshots, display and purges (NiDX9Renderer.h):
// TakeScreenShot (a region of a group's first buffer read back into an NiPixelData), FastCopy /
// Copy (StretchRect between two buffers' surfaces), PurgeTexture, Do_DisplayFrame (the buffers
// Do_EndUsingRenderTargetGroup listed are shown), PurgeGeometryData /
// PurgeSkinPartitionRendererData (the data's vertex buffers' owner entries dropped from the
// renderer's map at +0x42C, under the renderer locks; then the geometry group manager's).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::dx9 {
namespace {

GAME_VAR(uint32_t, NiRenderedCubeMap_ClassBit, 0x020E3874);

GAME_FN(void*, __thiscall, PixelData_Construct, 0x0076A610, void* self, uint32_t width, uint32_t height,
        const void* format, uint32_t mipmaps, uint32_t faces);
GAME_FN(uint8_t, __thiscall, PixelFormat_NotEqual, 0x0076A090, const void* self, const void* other);
GAME_FN(const char*, __cdecl, NiDX9ErrorString, 0x00899150, long hr);
GAME_FN(void, __thiscall, Map_SetAt, 0x008A5140, void* map, uint32_t key, void* value);
GAME_FN(void, __thiscall, Map_RemoveAt, 0x008A5A10, void* map, uint32_t key);
GAME_FN(void, __thiscall, GroupManager_DestroyPartitionData, 0x008993B0, void* self, void* partition);
GAME_FN(void, __thiscall, Renderer_LockRendererAndSourceData, 0x0087AD30, void* self);

constexpr int kSurfLockRect = 13;
constexpr int kSurfUnlockRect = 14;

// the renderer locks let go as the purges inline it (Renderer_UnlockRendererAndSourceData's)
inline void UnlockRendererAndSourceData(uint8_t* self) {
    Unlock(At<NiCriticalSection>(self, kSourceDataLock));
    NiCriticalSection& renderer = At<NiCriticalSection>(self, kRendererLock);
    if (renderer.lockCount == 1) {
        Virtual<void>(self, 81);   // Do_UnlockRenderer
    }
    Unlock(renderer);
}

// the vertex buffers' owner lists (the map at +0x42C: VB -> entries {+0 owner, +0x20 next}):
// every entry of `owner` dropped
inline void DropOwnerEntries(uint8_t* self, uint8_t* bufferData, void* owner) {
    for (uint32_t i = 0; i < At<uint32_t>(bufferData, 0x20); i++) {
        uint8_t* chip = i < At<uint32_t>(bufferData, 0x20) ? At<re::Ptr32<uint8_t>*>(bufferData, 0x28)[i] : nullptr;
        uint32_t key = At<uint32_t>(chip, 4);
        uint8_t* item = At<re::Ptr32<uint8_t>*>(self, 0x434)[key % At<uint32_t>(self, 0x430)];
        while (item != nullptr && At<uint32_t>(item, 4) != key) {
            item = At<uint8_t*>(item, 0);
        }
        if (item == nullptr) {
            continue;
        }
        uint8_t* previous = nullptr;
        uint8_t* entry = At<uint8_t*>(item, 8);
        while (entry != nullptr) {
            if (At<void*>(entry, 0) != owner) {
                previous = entry;
                entry = At<uint8_t*>(entry, 0x20);
                continue;
            }
            uint8_t* next = At<uint8_t*>(entry, 0x20);
            uint8_t* following = nullptr;
            if (previous != nullptr) {
                At<uint8_t*>(previous, 0x20) = next;
                following = At<uint8_t*>(entry, 0x20);
            } else if (next != nullptr) {
                Map_SetAt(self + 0x42C, key, next);
                following = At<uint8_t*>(entry, 0x20);
            } else {
                Map_RemoveAt(self + 0x42C, key);
            }
            At<uint8_t*>(entry, 0x20) = nullptr;
            NiDelete(entry, 0x24);
            entry = following;
        }
    }
}

} // namespace

// 0x87BCF0 (thiscall): NiDX9Renderer::TakeScreenShot (the group's first buffer, the default
// group's when none; a rectangle inside it, its right and bottom clamped; read back row by row)
void* __fastcall Renderer_TakeScreenShot(uint8_t* self, re::Edx, const uint32_t* rect, void* group) {
    if (self[kDeviceLost] != 0) {
        return nullptr;
    }
    if (group == nullptr) {
        group = At<void*>(self, kDefaultRenderTargetGroup);
    }
    uint32_t width = Virtual<uint32_t>(group, 18, 0u);
    uint32_t height = Virtual<uint32_t>(group, 19, 0u);
    int32_t d3dRect[4];   // RECT: left, top, right, bottom
    const int32_t* lockRect = nullptr;
    uint32_t copyWidth;
    uint32_t rows;
    if (rect != nullptr) {
        d3dRect[0] = int32_t(rect[0]);
        d3dRect[2] = int32_t(rect[1]);
        d3dRect[1] = int32_t(rect[2]);
        d3dRect[3] = int32_t(rect[3]);
        float w = float(width);
        if (!(double(w) > double(d3dRect[0]))) {
            return nullptr;
        }
        float bottom = float(d3dRect[3]);
        float h = float(height);
        if (!(h > bottom)) {
            return nullptr;
        }
        if (double(d3dRect[2]) > double(w)) {
            d3dRect[2] = int32_t(width);
        }
        if (bottom > h) {
            d3dRect[3] = int32_t(height);
        }
        copyWidth = uint32_t(d3dRect[2] - d3dRect[0]);
        rows = uint32_t(d3dRect[3] - d3dRect[1]);
        lockRect = d3dRect;
    } else {
        copyWidth = width;
        rows = height;
    }
    void* data = Virtual<void*>(group, 31, 0u);
    uint8_t* buffer = data != nullptr && HasClassBit(data, NiDX92DBufferData_ClassBit) ? static_cast<uint8_t*>(data)
                                                                                     : nullptr;
    void* surface = At<void*>(buffer, 0x14);
    uint32_t desc[8];
    Com<long>(surface, kSurfGetDesc, desc);
    struct {
        int32_t pitch;
        uint8_t* bits;
    } locked;
    if (Com<long>(surface, kSurfLockRect, &locked, lockRect, 0x10u) < 0) {   // D3DLOCK_READONLY
        return nullptr;
    }
    void* memory = NiNew(0x70);
    uint8_t* pixels = nullptr;
    if (memory != nullptr) {
        const void* format = Virtual<const void*>(group, 22, 0u);   // GetPixelFormat
        pixels = static_cast<uint8_t*>(PixelData_Construct(memory, copyWidth, rows, format, 1, 1));
    }
    if (pixels == nullptr) {
        Com<long>(surface, kSurfUnlockRect);
        return nullptr;
    }
    uint32_t rowBytes = *At<uint32_t*>(pixels, 0x54) * At<uint32_t>(pixels, 0x64);
    uint8_t* to = *At<re::Ptr32<uint8_t>*>(pixels, 0x5C) + At<uint32_t>(pixels, 0x50);
    const uint8_t* from = locked.bits;
    for (uint32_t row = rows; row != 0; row--) {
        crt::MemcpyS(to, rowBytes, from, rowBytes);
        from += locked.pitch;
        to += rowBytes;
    }
    Com<long>(surface, kSurfUnlockRect);
    return pixels;
}

// 0x87BF10 (thiscall): NiDX9Renderer::FastCopy (no filtering: the formats must match; a region
// of the source to (x, y) of the destination, or all of it)
uint8_t __fastcall Renderer_FastCopy(uint8_t* self, re::Edx, uint8_t* source, uint8_t* destination,
                                     const uint32_t* rect, uint32_t x, uint32_t y) {
    if (self[kDeviceLost] != 0) {
        return 0;
    }
    uint8_t* sourceData = At<uint8_t*>(source, 0x10);
    uint8_t* destinationData = At<uint8_t*>(destination, 0x10);
    if (sourceData == nullptr || destinationData == nullptr) {
        NiOutputDebugString("NiDX9Renderer::FastCopy> Failed - %s", "No RendererData found");
        return 0;
    }
    if (PixelFormat_NotEqual(At<void*>(sourceData, 0xC), At<void*>(destinationData, 0xC)) != 0) {
        NiOutputDebugString("NiDX9Renderer::FastCopy> Failed - %s", "Pixel formats do not match");
        return 0;
    }
    void* sourceSurface = At<void*>(sourceData, 0x14);
    void* destinationSurface = At<void*>(destinationData, 0x14);
    if (sourceSurface == nullptr || destinationSurface == nullptr) {
        NiOutputDebugString("NiDX9Renderer::FastCopy> Failed - %s", "NULL Surface found");
        return 0;
    }
    long hr;
    if (rect != nullptr) {
        int32_t from[4] = {int32_t(rect[0]), int32_t(rect[2]), int32_t(rect[1]), int32_t(rect[3])};
        int32_t to[4] = {int32_t(x), int32_t(y), int32_t(rect[1] - rect[0] + x), int32_t(rect[3] - rect[2] + y)};
        hr = Com<long>(At<void*>(self, kDevice), kDevStretchRect, sourceSurface, from, destinationSurface, to, 0u);
    } else {
        hr = Com<long>(At<void*>(self, kDevice), kDevStretchRect, sourceSurface, static_cast<const void*>(nullptr),
                       destinationSurface, static_cast<const void*>(nullptr), 0u);
    }
    if (hr < 0) {
        NiOutputDebugString("NiDX9Renderer::FastCopy> Failed StretchRect - %s", NiDX9ErrorString(hr));
        return 0;
    }
    return 1;
}

// 0x87C070 (thiscall): NiDX9Renderer::Copy (rectangles of each, the whole buffers by default;
// filter 1 point, 2 linear, else none)
uint8_t __fastcall Renderer_Copy(uint8_t* self, re::Edx, uint8_t* source, uint8_t* destination,
                                 const uint32_t* sourceRect, const uint32_t* destinationRect, int filter) {
    if (self[kDeviceLost] != 0) {
        return 0;
    }
    uint8_t* destinationData = At<uint8_t*>(destination, 0x10);
    uint8_t* sourceData = At<uint8_t*>(source, 0x10);
    if (sourceData == nullptr || destinationData == nullptr) {
        NiOutputDebugString("NiDX9Renderer::Copy> Failed - %s", "No RendererData found");
        return 0;
    }
    void* sourceSurface = At<void*>(sourceData, 0x14);
    void* destinationSurface = At<void*>(destinationData, 0x14);
    if (sourceSurface == nullptr || destinationSurface == nullptr) {
        NiOutputDebugString("NiDX9Renderer::Copy> Failed - %s", "NULL Surface found");
        return 0;
    }
    int32_t from[4];
    if (sourceRect != nullptr) {
        from[0] = int32_t(sourceRect[0]);
        from[2] = int32_t(sourceRect[1]);
        from[1] = int32_t(sourceRect[2]);
        from[3] = int32_t(sourceRect[3]);
    } else {
        from[0] = 0;
        from[2] = At<int32_t>(source, 8);
        from[1] = 0;
        from[3] = At<int32_t>(source, 0xC);
    }
    int32_t to[4];
    if (destinationRect != nullptr) {
        to[0] = int32_t(destinationRect[0]);
        to[2] = int32_t(destinationRect[1]);
        to[1] = int32_t(destinationRect[2]);
        to[3] = int32_t(destinationRect[3]);
    } else {
        to[0] = 0;
        to[2] = At<int32_t>(destination, 8);
        to[1] = 0;
        to[3] = At<int32_t>(destination, 0xC);
    }
    uint32_t d3dFilter = filter == 1 ? 1u : (filter == 2 ? 2u : 0u);   // D3DTEXF_POINT / LINEAR / NONE
    long hr = Com<long>(At<void*>(self, kDevice), kDevStretchRect, sourceSurface, from, destinationSurface, to,
                        d3dFilter);
    if (hr < 0) {
        NiOutputDebugString("NiDX9Renderer::Copy> Failed StretchRect - %s", NiDX9ErrorString(hr));
        return 0;
    }
    return 1;
}

// 0x87C1E0 (thiscall): NiDX9Renderer::PurgeTexture (its renderer data deleted; a rendered
// texture's buffers' data, a rendered cube map's six faces', let go first)
uint8_t __fastcall Renderer_PurgeTexture(uint8_t*, re::Edx, uint8_t* texture) {
    uint8_t* data = At<uint8_t*>(texture, 0x24);
    if (data == nullptr) {
        return 1;
    }
    if ((data[0x62] >> 1) & 1) {   // a rendered texture's
        if (HasClassBit(texture, NiRenderedCubeMap_ClassBit)) {
            for (int face = 0; face < 6; face++) {
                re::Ptr32<void>& faceData = At<void*>(At<uint8_t*>(texture, 0x38 + face * 4), 0x10);
                if (faceData != nullptr) {
                    ReleaseRef(faceData);
                    faceData = nullptr;
                }
            }
        } else {
            re::Ptr32<void>& bufferData = At<void*>(Virtual<uint8_t*>(texture, 20), 0x10);   // GetBuffer
            if (bufferData != nullptr) {
                ReleaseRef(bufferData);
                bufferData = nullptr;
            }
        }
    }
    At<void*>(texture, 0x24) = nullptr;
    Virtual<void*>(data, 0, 1);
    return 1;
}

// 0x87C7B0 (thiscall): NiDX9Renderer::Do_DisplayFrame (each listed buffer taken off the list and
// shown: its slot 4)
uint8_t __fastcall Renderer_Do_DisplayFrame(uint8_t* self, re::Edx) {
    if (self[kDeviceLost] != 0) {
        return 1;
    }
    uint8_t* list = self + kBuffersInUse;
    while (At<uint32_t>(list, 0xC) != 0) {
        uint8_t* node = At<uint8_t*>(list, 4);
        uint8_t* next = At<uint8_t*>(node, 0);
        At<uint8_t*>(list, 4) = next;
        if (next != nullptr) {
            At<void*>(next, 4) = nullptr;
        } else {
            At<void*>(list, 8) = nullptr;
        }
        void* buffer = At<void*>(node, 8);
        AddRefRef(buffer);
        Virtual<void>(list, 2, node);   // the node freed
        At<uint32_t>(list, 0xC)--;
        Virtual<void>(buffer, 4);       // DisplayFrame
        ReleaseRef(buffer);
    }
    return 1;
}

// 0x87EB10 (thiscall): NiDX9Renderer::PurgeGeometryData
void __fastcall Renderer_PurgeGeometryData(uint8_t* self, re::Edx, uint8_t* data) {
    if (data == nullptr) {
        return;
    }
    uint8_t* bufferData = At<uint8_t*>(data, 0x3C);
    if (bufferData == nullptr) {
        return;
    }
    if (At<uint32_t>(bufferData, 0x20) != 0 && At<re::Ptr32<void>*>(bufferData, 0x28)[0] != nullptr) {
        Renderer_LockRendererAndSourceData(self);
        DropOwnerEntries(self, bufferData, data);
        UnlockRendererAndSourceData(self);
    }
    Virtual<void>(At<void*>(self, kManager6A8), 7, data, 0u);
}

// 0x87EC80 (thiscall): NiDX9Renderer::PurgeSkinPartitionRendererData (each partition's, 0x2C
// apart; the first one must have buffer data)
void __fastcall Renderer_PurgeSkinPartitionRendererData(uint8_t* self, re::Edx, uint8_t* partition) {
    if (partition == nullptr) {
        return;
    }
    uint8_t* part = At<uint8_t*>(partition, 0xC);
    if (part == nullptr || At<void*>(part, 0x28) == nullptr) {
        return;
    }
    for (uint32_t n = At<uint32_t>(partition, 8); n != 0; n--) {
        uint8_t* bufferData = At<uint8_t*>(part, 0x28);
        if (At<uint32_t>(bufferData, 0x20) != 0 && At<re::Ptr32<void>*>(bufferData, 0x28)[0] != nullptr) {
            Renderer_LockRendererAndSourceData(self);
            DropOwnerEntries(self, bufferData, part);
            UnlockRendererAndSourceData(self);
        }
        part += 0x2C;
    }
    GroupManager_DestroyPartitionData(At<void*>(self, kManager6A8), partition);
}

#define DX9REND38D_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

DX9REND38D_REVERSED("NiDX9Renderer::TakeScreenShot", 0x0087BCF0, Renderer_TakeScreenShot);
DX9REND38D_REVERSED("NiDX9Renderer::FastCopy", 0x0087BF10, Renderer_FastCopy);
DX9REND38D_REVERSED("NiDX9Renderer::Copy", 0x0087C070, Renderer_Copy);
DX9REND38D_REVERSED("NiDX9Renderer::PurgeTexture", 0x0087C1E0, Renderer_PurgeTexture);
DX9REND38D_REVERSED("NiDX9Renderer::Do_DisplayFrame", 0x0087C7B0, Renderer_Do_DisplayFrame);
DX9REND38D_REVERSED("NiDX9Renderer::PurgeGeometryData", 0x0087EB10, Renderer_PurgeGeometryData);
DX9REND38D_REVERSED("NiDX9Renderer::PurgeSkinPartitionRendererData", 0x0087EC80,
                    Renderer_PurgeSkinPartitionRendererData);

} // namespace ni::dx9
