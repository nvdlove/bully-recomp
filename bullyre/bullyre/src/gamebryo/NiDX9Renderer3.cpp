// NiDX9Renderer's render targets and formats (NiDX9Renderer.h): ValidateRenderTargetGroup,
// IsDepthBufferCompatible, FindClosestDepthStencilFormat, Do_BeginUsingRenderTargetGroup /
// Do_EndUsingRenderTargetGroup (the buffers' renderer data set as the device's targets; the ones
// that can display remembered for Do_DisplayFrame), Do_ClearBuffer, SetDefaultProgramCache.
//
// The buffers' renderer data (NiDX92DBufferData and kinds, by NiRTTI class bit): +0x8 the
// buffer (Ni2DBuffer: +0x8 width, +0xC height), +0xC its pixel format (+0x1 bits per pixel),
// +0x14 the D3D surface; slots 3 CanDisplayFrame, 7 SetRenderTarget(device, index), 8 (depth)
// SetDepthTarget(device).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::dx9 {
namespace {

constexpr uintptr_t kD3DXGetVertexShaderProfile = 0x008FC314;
constexpr uintptr_t kD3DXGetPixelShaderProfile = 0x008FC318;

inline void* const NiDX9DepthStencilBufferData_RTTI = reinterpret_cast<void*>(0x020E7140);

GAME_FN(uint8_t*, __cdecl, IsKindOf, 0x00401FF0, void* rtti, void* object);
GAME_FN(void, __cdecl, BufferData_ClearRenderTarget, 0x008942C0, void* device, uint32_t index);
GAME_FN(uint8_t, __cdecl, DepthStencilData_ClearDepthTarget, 0x00894310, void* device);
GAME_FN(uint8_t, __thiscall, DepthStencilData_HasValidDepthBuffer, 0x008944C0, void* self);
GAME_FN(uint8_t, __thiscall, DepthStencilData_HasValidStencilBuffer, 0x008944E0, void* self);
GAME_FN(void, __thiscall, BufferList_AddTail, 0x007843D0, void* list, const re::Ptr32<void>* element);
GAME_FN(uint32_t, __cdecl, PixelFormat_GetD3DFormat, 0x00895340, const void* format);
GAME_FN(void*, __cdecl, PixelFormat_CreateFromD3DFormat, 0x00895B20, uint32_t format);
GAME_FN(uint32_t, __thiscall, DeviceDesc_FindDepthStencilFormat, 0x00891040, void* self, uint32_t adapterFormat,
        uint32_t frameBufferFormat, uint32_t depthBpp, uint32_t stencilBpp);
GAME_FN(void, __thiscall, Map_SetAt, 0x008A5140, void* map, uint32_t key, void* value);
// SetDefaultProgramCache's
GAME_FN(const char*, __cdecl, Material_GetDefaultWorkingDirectory, 0x00762A00);
GAME_FN(uint32_t, __thiscall, Material_GetProgramVersion, 0x00786440, void* material, uint32_t type);
GAME_FN(void*, __thiscall, GPUProgramCache_Construct, 0x008986C0, void* self, uint32_t version,
        const char* workingDir, uint32_t type, const re::Ptr32<const char>* profile, const char* materialName,
        int autoWriteToDisk, int writeDebugFile, int locked, int load);
GAME_FN(const char*, __cdecl, StringTable_AddString, 0x00754760, const char* text);   // (a native return: a Ptr32 by value is returned through memory on x64)
GAME_FN(void, __thiscall, FixedString_Destruct, 0x00517750, re::Ptr32<const char>* self);

// the pointer map at +0x8E8 (NiTPointerMap<D3DFORMAT, NiPixelFormat*>)
struct MapItem {   // (the game's layout: 0xC)
    re::Ptr32<MapItem> next;
    uint32_t key;
    re::Ptr32<void> value;
};
static_assert(sizeof(MapItem) == 0xC);

inline void* GroupBufferData(void* group, uint32_t index) {
    return Virtual<void*>(group, 31, index);   // GetBufferRendererData
}
inline uint32_t GroupBufferCount(void* group) {
    return Virtual<uint32_t>(group, 24);
}

} // namespace

// 0x87B8C0 (thiscall): NiDX9Renderer::ValidateRenderTargetGroup (not more buffers than the device
// takes; each with a surface, the same size as the ones before it, not multisampled, of their bit
// depth unless the device allows independent depths; the depth buffer compatible with each)
uint8_t __fastcall Renderer_ValidateRenderTargetGroup(uint8_t* self, re::Edx, void* group) {
    uint32_t count = GroupBufferCount(group);
    if (count > Virtual<uint32_t>(self, 39)) {   // GetMaxBuffersPerRenderTargetGroup
        return 0;
    }
    for (uint32_t i = 0; i < count; i++) {
        uint8_t* data = static_cast<uint8_t*>(GroupBufferData(group, i));
        if (data == nullptr || At<void*>(data, 0x14) == nullptr) {
            return 0;
        }
        for (uint32_t j = 0; j < i; j++) {
            uint8_t* other = static_cast<uint8_t*>(GroupBufferData(group, j));
            uint32_t desc[8];   // D3DSURFACE_DESC
            if (Com<long>(At<void*>(other, 0x14), kSurfGetDesc, desc) < 0) {
                return 0;
            }
            if (desc[4] != 0) {   // MultiSampleType
                return 0;
            }
            uint8_t* buffer = At<uint8_t*>(data, 8);
            if (At<uint32_t>(buffer, 8) != desc[6] || At<uint32_t>(buffer, 0xC) != desc[7]) {
                return 0;
            }
            if (Virtual<uint8_t>(self, 40) == 0) {   // GetIndependentBufferBitDepths
                uint8_t bits = desc[0] < 0x76 ? ms_aucD3DFormatSizes[desc[0]] : 0;
                if (bits != At<uint8_t>(At<void*>(data, 0xC), 1)) {
                    return 0;
                }
            }
        }
    }
    void* depth = Virtual<void*>(group, 28);   // GetDepthStencilBuffer
    if (depth != nullptr) {
        for (uint32_t k = 0; k < count; k++) {
            void* buffer = Virtual<void*>(group, 27, k);
            if (Virtual<uint8_t>(self, 28, buffer, depth) == 0) {   // IsDepthBufferCompatible
                return 0;
            }
        }
    }
    void* depthData = Virtual<void*>(group, 32);   // GetDepthStencilBufferRendererData
    if (depthData != nullptr && HasClassBit(depthData, NiDX9DepthStencilBufferData_ClassBit) &&
        At<void*>(depthData, 0x14) == nullptr) {
        return 0;
    }
    return 1;
}

// 0x87BA70 (thiscall): NiDX9Renderer::IsDepthBufferCompatible (no depth buffer: yes; one at least
// the buffer's size, of the same multisampling, that the adapter matches with the format)
uint8_t __fastcall Renderer_IsDepthBufferCompatible(uint8_t* self, re::Edx, uint8_t* buffer, uint8_t* depth) {
    if (buffer == nullptr) {
        return 0;
    }
    if (depth == nullptr) {
        return 1;
    }
    if (At<uint32_t>(buffer, 8) > At<uint32_t>(depth, 8) || At<uint32_t>(buffer, 0xC) > At<uint32_t>(depth, 0xC)) {
        return 0;
    }
    uint8_t* depthData = At<uint8_t*>(depth, 0x10);
    void* surface = At<void*>(At<void*>(buffer, 0x10), 0x14);
    uint32_t bufferDesc[8];
    uint32_t depthDesc[8];
    Com<long>(surface, kSurfGetDesc, bufferDesc);
    Com<long>(At<void*>(depthData, 0x14), kSurfGetDesc, depthDesc);
    if (bufferDesc[5] != depthDesc[5] || bufferDesc[4] != depthDesc[4]) {   // quality, type
        return 0;
    }
    // CheckDepthStencilMatch(adapter, type, adapter format, render target format, depth format)
    return Com<long>(ms_pkD3D9, 12, At<uint32_t>(self, kAdapter), At<uint32_t>(self, kDevType),
                     At<uint32_t>(self, kAdapterFormat), bufferDesc[0], depthDesc[0]) >= 0
               ? 1
               : 0;
}

// 0x87B4A0 (thiscall): NiDX9Renderer::FindClosestDepthStencilFormat (the device's closest match
// to the front buffer's format, its NiPixelFormat made once per D3D format). (The default
// group's buffer data is fetched and its RTTI asked for nothing.)
void* __fastcall Renderer_FindClosestDepthStencilFormat(uint8_t* self, re::Edx, const void* front, uint32_t depthBpp,
                                                        uint32_t stencilBpp) {
    void* data = Virtual<void*>(At<void*>(self, kDefaultRenderTargetGroup), 31, 0u);
    if (data != nullptr) {
        Virtual<void*>(data, 2);
    }
    uint32_t frontFormat = PixelFormat_GetD3DFormat(front);
    uint32_t format = DeviceDesc_FindDepthStencilFormat(At<void*>(self, kDeviceDesc), At<uint32_t>(self, kAdapterFormat),
                                                        frontFormat, depthBpp, stencilBpp);
    uint8_t* map = self + 0x8E8;
    MapItem* item = At<re::Ptr32<MapItem>*>(map, 8)[format % At<uint32_t>(map, 4)];
    for (; item != nullptr; item = item->next) {
        if (item->key == format) {
            if (item->value != nullptr) {
                return item->value;
            }
            break;
        }
    }
    void* pixelFormat = PixelFormat_CreateFromD3DFormat(format);
    Map_SetAt(map, format, pixelFormat);
    return pixelFormat;
}

// 0x87B540 (thiscall): NiDX9Renderer::Do_BeginUsingRenderTargetGroup (targets 1..3 cleared, each
// buffer set as its target, the depth buffer set or cleared; on a failure the default group
// instead; then the viewport and clear)
uint8_t __fastcall Renderer_Do_BeginUsingRenderTargetGroup(uint8_t* self, re::Edx, void* group, uint32_t clearMode) {
    if (self[kDeviceLost] != 0) {
        return 1;
    }
    BufferData_ClearRenderTarget(At<void*>(self, kDevice), 1);
    BufferData_ClearRenderTarget(At<void*>(self, kDevice), 2);
    BufferData_ClearRenderTarget(At<void*>(self, kDevice), 3);
    for (uint32_t i = 0; i < GroupBufferCount(group); i++) {
        void* data = GroupBufferData(group, i);
        if (data == nullptr) {
            continue;
        }
        void* buffer = HasClassBit(data, NiDX92DBufferData_ClassBit) ? data : nullptr;
        if (buffer != nullptr && Virtual<uint8_t>(buffer, 7, At<void*>(self, kDevice), i) == 0) {
            void* fallback = At<void*>(self, kDefaultRenderTargetGroup);
            if (group != fallback) {
                Virtual<uint8_t>(self, 89, fallback, clearMode);
            }
            NiOutputDebugString("NiDX9Renderer::BeginUsingRenderTargetGroup> Failed - %s",
                                "Setting an output target failed. Resetting to default target.");
            return 0;
        }
    }
    void* depthData = Virtual<void*>(group, 32);
    void* depth = depthData != nullptr && HasClassBit(depthData, NiDX9DepthStencilBufferData_ClassBit) ? depthData
                                                                                                    : nullptr;
    if (depth != nullptr) {
        if (Virtual<uint8_t>(depth, 8, At<void*>(self, kDevice)) == 0) {
            Virtual<uint8_t>(self, 89, At<void*>(self, kDefaultRenderTargetGroup), clearMode);
            NiOutputDebugString("NiDX9Renderer::BeginUsingRenderTargetGroup> Failed - %s",
                                "Setting the depth/stencil buffer failed. Resetting to default target.");
            return 0;
        }
    } else if (DepthStencilData_ClearDepthTarget(At<void*>(self, kDevice)) == 0) {
        Virtual<uint8_t>(self, 89, At<void*>(self, kDefaultRenderTargetGroup), clearMode);
        NiOutputDebugString("NiDX9Renderer::BeginUsingRenderTargetGroup> Failed - %s",
                            "Setting the depth/stencil buffer to NULL failed. Resetting to default target.");
        return 0;
    }
    void* first = GroupBufferData(group, 0);
    void* onscreen = first != nullptr && HasClassBit(first, NiDX9OnscreenBufferData_ClassBit) ? first : nullptr;
    At<void*>(self, kCurrentRenderTargetGroup) = group;
    if (onscreen != nullptr) {
        At<void*>(self, kCurrentOnscreenTarget) = group;
    }
    Virtual<void>(self, 85, static_cast<const void*>(nullptr), clearMode);   // Do_ClearBuffer
    return 1;
}

// 0x87B790 (thiscall): NiDX9Renderer::Do_EndUsingRenderTargetGroup (the current group's buffers
// that can display, once each in the list Do_DisplayFrame presents; no current group after).
// (A buffer data that isn't an NiDX92DBufferData is asked CanDisplayFrame through null: the
// game's.)
uint8_t __fastcall Renderer_Do_EndUsingRenderTargetGroup(uint8_t* self, re::Edx) {
    if (self[kDeviceLost] != 0) {
        return 1;
    }
    re::Ptr32<void> listed = nullptr;   // (the AddTail argument's NiPointer)
    for (uint32_t i = 0; i < GroupBufferCount(At<void*>(self, kCurrentRenderTargetGroup)); i++) {
        void* data = GroupBufferData(At<void*>(self, kCurrentRenderTargetGroup), i);
        void* buffer = data != nullptr && HasClassBit(data, NiDX92DBufferData_ClassBit) ? data : nullptr;
        if (Virtual<uint8_t>(buffer, 3) == 0) {   // CanDisplayFrame
            continue;
        }
        AddRefRef(buffer);
        bool found = false;
        for (uint8_t* node = At<uint8_t*>(self, kBuffersInUse + 4); node != nullptr; node = At<uint8_t*>(node, 0)) {
            if (At<void*>(node, 8) == buffer) {
                found = true;
                break;
            }
        }
        ReleaseRef(buffer);
        if (!found) {
            listed = buffer;
            AddRefRef(buffer);
            BufferList_AddTail(self + kBuffersInUse, &listed);
            ReleaseRef(buffer);
        }
    }
    At<void*>(self, kCurrentRenderTargetGroup) = nullptr;
    At<void*>(self, kCurrentOnscreenTarget) = nullptr;
    return 1;
}

// 0x87BB30 (thiscall): NiDX9Renderer::Do_ClearBuffer (the rectangle in the current group's
// pixels, y flipped; the back buffer, and the depth / stencil when the depth buffer has them;
// the viewport set to the whole group first)
void __fastcall Renderer_Do_ClearBuffer(uint8_t* self, re::Edx, const float* rect, uint32_t mode) {
    if (mode == 0 || self[kDeviceLost] != 0) {
        return;
    }
    uint32_t width = Virtual<uint32_t>(At<void*>(self, kCurrentRenderTargetGroup), 18, 0u);
    uint32_t height = Virtual<uint32_t>(At<void*>(self, kCurrentRenderTargetGroup), 19, 0u);
    int32_t d3dRect[4];
    if (rect != nullptr) {
        float w = float(width);
        float h = float(height);
        d3dRect[0] = crt::FloatToInt(rect[0] * w);
        d3dRect[1] = crt::FloatToInt((1.0f - rect[2]) * h);
        d3dRect[2] = crt::FloatToInt(w * rect[1]);
        d3dRect[3] = crt::FloatToInt(h * (1.0f - rect[3]));
    } else {
        d3dRect[0] = 0;
        d3dRect[1] = 0;
        d3dRect[2] = int32_t(width);
        d3dRect[3] = int32_t(height);
    }
    uint32_t flags = mode & 1;   // D3DCLEAR_TARGET
    void* depth = IsKindOf(NiDX9DepthStencilBufferData_RTTI,
                           Virtual<void*>(At<void*>(self, kCurrentRenderTargetGroup), 32));
    if (depth != nullptr) {
        flags |= ((mode & 4) && DepthStencilData_HasValidDepthBuffer(depth) != 0) ? 2u : 0u;     // ZBUFFER
        flags |= ((mode & 2) && DepthStencilData_HasValidStencilBuffer(depth) != 0) ? 4u : 0u;   // STENCIL
    }
    At<uint32_t>(self, 0x4D0) = Virtual<uint32_t>(At<void*>(self, kCurrentRenderTargetGroup), 18, 0u);
    At<uint32_t>(self, 0x4D4) = Virtual<uint32_t>(At<void*>(self, kCurrentRenderTargetGroup), 19, 0u);
    Com<long>(At<void*>(self, kDevice), kDevSetViewport, self + 0x4C8);
    Com<long>(At<void*>(self, kDevice), kDevClear, 1u, d3dRect, flags, At<uint32_t>(self, kBackground),
              At<float>(self, kZClear), At<uint32_t>(self, kStencilClear));
}

// 0x87B0F0 (thiscall): NiDX9Renderer::SetDefaultProgramCache (the device's shader profiles:
// vs_3_0 without ps_3_0 drops to vs_2_a; a vertex and a pixel NiD3DGPUProgramCache for the
// material)
void __fastcall Renderer_SetDefaultProgramCache(uint8_t* self, re::Edx, uint8_t* material, int autoWriteToDisk,
                                                int writeDebugFile, int load, int locked, const char* workingDir) {
    const char* vertexProfile = Import<const char*>(kD3DXGetVertexShaderProfile, At<void*>(self, kDevice));
    const char* pixelProfile = Import<const char*>(kD3DXGetPixelShaderProfile, At<void*>(self, kDevice));
    if (vertexProfile == nullptr || pixelProfile == nullptr) {
        return;
    }
    if (crt::Strcmp(vertexProfile, "vs_3_0") == 0 && crt::Strcmp(pixelProfile, "ps_3_0") != 0) {   // (repe cmpsb)
        vertexProfile = "vs_2_a";
    }
    if (workingDir == nullptr) {
        workingDir = Material_GetDefaultWorkingDirectory();
    }
    void* memory = NiNew(0x58);
    void* cache = nullptr;
    if (memory != nullptr) {
        NiFixedString profile = vertexProfile == nullptr ? ni::sys::kNullFixedString : StringTable_AddString(vertexProfile);
        cache = GPUProgramCache_Construct(memory, Material_GetProgramVersion(material, 0), workingDir, 0, &profile,
                                          At<const char*>(material, 8), autoWriteToDisk, writeDebugFile, locked, load);
        FixedString_Destruct(&profile);
    }
    Virtual<void>(material, 10, cache, 0u);   // SetProgramCache(cache, PROGRAM_VERTEX)
    memory = NiNew(0x58);
    cache = nullptr;
    if (memory != nullptr) {
        NiFixedString profile = StringTable_AddString(pixelProfile);
        cache = GPUProgramCache_Construct(memory, Material_GetProgramVersion(material, 1), workingDir, 1, &profile,
                                          At<const char*>(material, 8), autoWriteToDisk, writeDebugFile, locked, load);
        FixedString_Destruct(&profile);
    }
    Virtual<void>(material, 10, cache, 1u);   // PROGRAM_PIXEL
}

#define DX9REND38C_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

DX9REND38C_REVERSED("NiDX9Renderer::SetDefaultProgramCache", 0x0087B0F0, Renderer_SetDefaultProgramCache);
DX9REND38C_REVERSED("NiDX9Renderer::FindClosestDepthStencilFormat", 0x0087B4A0,
                    Renderer_FindClosestDepthStencilFormat);
DX9REND38C_REVERSED("NiDX9Renderer::Do_BeginUsingRenderTargetGroup", 0x0087B540,
                    Renderer_Do_BeginUsingRenderTargetGroup);
DX9REND38C_REVERSED("NiDX9Renderer::Do_EndUsingRenderTargetGroup", 0x0087B790, Renderer_Do_EndUsingRenderTargetGroup);
DX9REND38C_REVERSED("NiDX9Renderer::ValidateRenderTargetGroup", 0x0087B8C0, Renderer_ValidateRenderTargetGroup);
DX9REND38C_REVERSED("NiDX9Renderer::IsDepthBufferCompatible", 0x0087BA70, Renderer_IsDepthBufferCompatible);
DX9REND38C_REVERSED("NiDX9Renderer::Do_ClearBuffer", 0x0087BB30, Renderer_Do_ClearBuffer);

} // namespace ni::dx9
