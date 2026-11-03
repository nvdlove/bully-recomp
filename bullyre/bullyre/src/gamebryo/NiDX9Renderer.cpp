// NiDX9Renderer's set-up (NiDX9Renderer.h): Direct3DCreate9 loaded from D3D9.DLL, the system
// description, the members' defaults, the present parameters, the device caps, the texture
// formats and default textures, the driver string; and the small static helpers (format,
// multisample and present interval mappings, the matrix from a rotation / translate / scale).

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::dx9 {
namespace {

constexpr uintptr_t kLoadLibraryA = 0x008FC0C8;
constexpr uintptr_t kGetProcAddress = 0x008FC0C4;
constexpr uintptr_t kFreeLibrary = 0x008FC0C0;
constexpr uintptr_t kTMapBaseVtable = 0x0099ACD4;   // NiTMapBase<NiTPointerAllocator<unsigned int>, ...>

GAME_ARRAY(const uint32_t, ms_akUnitAxes, 0x00BC8A90);   // NiPoint3::UNIT_X, UNIT_Y (6 floats, copied)
inline void* const kDisplayModeList = reinterpret_cast<void*>(0x00BCAE1C);

GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(void, __thiscall, TMapBase_RemoveAll, 0x005AC520, void* self);
// the descriptions of the system, an adapter, a device
GAME_FN(void*, __thiscall, SystemDesc_Construct, 0x00891880, void* self, void* d3d, void* modes);
GAME_FN(void, __thiscall, SystemDesc_Destruct, 0x008918C0, void* self);
GAME_FN(void, __thiscall, PointerList_RemoveAll, 0x0075DB60, void* list);
GAME_FN(uint32_t, __thiscall, DeviceDesc_GetDefaultFrameBufferFormat, 0x008909A0, void* self, int windowed,
        uint32_t bpp);
GAME_FN(uint32_t, __thiscall, DeviceDesc_FindDepthStencilFormat, 0x00891040, void* self, uint32_t adapterFormat,
        uint32_t frameBufferFormat, uint32_t depthBpp, uint32_t stencil);
GAME_FN(void*, __thiscall, AdapterDesc_ValidateFullscreenMode, 0x008910E0, void* self, uint32_t format,
        uint32_t width, uint32_t height, uint32_t* refreshRate);
GAME_FN(uint32_t, __cdecl, GetD3DFormat, 0x0082C6B0, uint32_t frameBufferFormat);
GAME_FN(uint32_t, __cdecl, GetNiFBFormat, 0x0082C780, uint32_t d3dFormat);
// the render state's limits from the caps (0x883CD0, 0x883CE0, 0x883CF0)
GAME_FN(void, __cdecl, RenderState_SetMaxTextureBlendStages, 0x00883CD0, uint32_t stages);
GAME_FN(void, __cdecl, RenderState_SetMaxSimultaneousTextures, 0x00883CE0, uint32_t textures);
GAME_FN(void, __cdecl, RenderState_SetMaxSamplers, 0x00883CF0, uint32_t samplers);
// the texture stage mapping statics InitializeDeviceCaps sets (0x20E6C98..0x20E6CFC)
GAME_ARRAY(uint32_t, ms_auiStageMap, 0x020E6C98);
GAME_VAR(uint32_t, ms_uiTexCoordFlags, 0x020E6D0C);
// NiPixelData(width, height, format, mipmaps, faces), NiSourceTexture::Create
GAME_FN(void*, __thiscall, PixelData_Construct, 0x0076A610, void* self, uint32_t width, uint32_t height,
        const void* format, uint32_t mipmaps, uint32_t faces);
GAME_FN(void*, __cdecl, SourceTexture_CreateFromData, 0x007699A0, void* pixelData, const uint32_t* prefs);
GAME_FN(void, __cdecl, PixelFormat_Initialize, 0x00754A50, void* format);
GAME_FN(void, __thiscall, Renderer_CreatePixelFormatArray, 0x00877F00, void* self, re::Ptr32<void>* formats, uint32_t usage,
        uint32_t resourceType);

// this file's (called where modes apply)
GAME_FN(int, __cdecl, CreateDirect3D9, 0x00878D10);
GAME_FN(uint32_t, __cdecl, GetCompatibleAdapterFormat, 0x00878600, uint32_t frameBufferFormat);
GAME_FN(uint32_t, __cdecl, GetMultiSampleType, 0x00878670, uint32_t mode);
GAME_FN(uint32_t, __cdecl, GetPresentInterval, 0x00878770, uint32_t interval);
GAME_FN(const char*, __thiscall, Renderer_GetBehaviorString, 0x00878570, void* self);
GAME_FN(uint8_t, __thiscall, Renderer_InitializeTextureDefaults, 0x00879400, void* self);

// NiRenderer::SetLastErrorString as the code inlines it
inline void SetError(const char* text) {
    crt::StrncpyS(ms_acErrorString, 0x100, text, 0xFF);
}

} // namespace

// 0x878550 (thiscall): NiGeometryBufferData::GetVBChip (null past the stream count)
void* __fastcall BufferData_GetVBChip(uint8_t* self, re::Edx, uint32_t stream) {
    if (stream < At<uint32_t>(self, 0x20)) {
        return At<re::Ptr32<void>*>(self, 0x28)[stream];
    }
    return nullptr;
}

namespace {

// ---- 0x877CA0 to 0x8784E6: the renderer's small virtuals (BullyNiDX9Renderer's vtable) and
// the pixel format arrays

GAME_FN(void*, __cdecl, TextureData_FindClosestPixelFormat, 0x008925F0, const uint32_t* prefs, const re::Ptr32<void>* formats);
GAME_FN(void, __cdecl, TextureData_ClearTextureData, 0x008930B0, void* renderer);
GAME_FN(void, __thiscall, VBManager_IncrementFrameID, 0x00887210, void* self);
GAME_FN(void*, __thiscall, FragmentShader_Construct, 0x008941D0, void* self, const void* descriptor);
GAME_FN(void*, __thiscall, ShadowWriteShader_Construct, 0x00884B30, void* self, void* descriptor);
GAME_FN(void*, __cdecl, AdditionalDepthStencilBufferData_Create, 0x00894D60, void* device, re::Ptr32<void>* buffer,
        const void* format, uint32_t multisample);
GAME_FN(void, __thiscall, RenderState_PurgeMaterial, 0x00882540, void* self, void* material);
GAME_FN(uint32_t, __cdecl, PixelFormat_GetD3DFormat, 0x00895340, const void* format);
GAME_VAR(re::Ptr32<uint8_t>, ms_pFreeListNodes_20E6D14, 0x020E6D14);   // the pointer lists' free nodes (next +0xC)

// 0x877EC0 (vf34): an additional depth / stencil buffer's data for `buffer` (none for none)
uint8_t __fastcall Renderer_CreateDepthStencilData(uint8_t* self, re::Edx, void* buffer, const void* format,
                                                   uint32_t multisample) {
    if (buffer == nullptr) {
        return 0;
    }
    re::Ptr32<void> held = buffer;
    return AdditionalDepthStencilBufferData_Create(At<void*>(self, kDevice), &held, format, multisample) != nullptr;
}

// 0x877CA0 (vf36): the texture format nearest the preferences
void* __fastcall Renderer_FindClosestPixelFormat(uint8_t* self, re::Edx, const uint32_t* prefs) {
    return TextureData_FindClosestPixelFormat(prefs, &At<void*>(self, kTextureFormats));
}

// 0x877CC0 (vf39): the most render targets at once
uint32_t __fastcall Renderer_GetMaxBuffersPerRenderTargetGroup(uint8_t* self, re::Edx) {
    return At<uint32_t>(self, kMaxRenderTargets);
}

// 0x877CD0 (vf40): whether the render targets may differ in bit depth
uint8_t __fastcall Renderer_GetIndependentBufferBitDepths(uint8_t* self, re::Edx) {
    return At<uint8_t>(self, kIndependentBitDepths);
}

// 0x877CE0 (vf43): the texture precached, by the texture manager (its slot 4)
uint8_t __fastcall Renderer_PrecacheTexture(uint8_t* self, re::Edx, void* texture, uint8_t a, uint8_t b) {
    return ::ni::sys::Virtual<uint8_t>(At<void*>(self, kTextureManager), 4, texture, a, b);
}

// 0x877E70 (vf49): a material's shader data purged, by the render state
void __fastcall Renderer_PurgeMaterial(uint8_t* self, re::Edx, void* material) {
    RenderState_PurgeMaterial(At<void*>(self, kRenderState), material);
}

// 0x877CF0 (vf55): every texture's renderer data cleared (the flag unused)
uint8_t __fastcall Renderer_PurgeAllTextures(uint8_t* self, re::Edx, uint8_t) {
    TextureData_ClearTextureData(self);
    return 1;
}

// 0x877E80 (vf75): a new fragment shader for the material descriptor
void* __fastcall Renderer_CreateFragmentShader(uint8_t*, re::Edx, const void* descriptor) {
    void* memory = NiNew(0x70);
    return memory != nullptr ? FragmentShader_Construct(memory, descriptor) : nullptr;
}

// 0x877EA0 (vf78): a new shadow-write shader for the material descriptor
void* __fastcall Renderer_CreateShadowWriteShader(uint8_t*, re::Edx, void* descriptor) {
    void* memory = NiNew(0x70);
    return memory != nullptr ? ShadowWriteShader_Construct(memory, descriptor) : nullptr;
}

// 0x877D00 (vf83): the frame's scene ended (unless the device is lost), the vertex buffers'
// frame counted
uint8_t __fastcall Renderer_EndScene(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, kDeviceLost) == 0) {
        if (Com<long>(At<void*>(self, kDevice), 0xA8 / 4) < 0) {   // EndScene
            return 0;
        }
        VBManager_IncrementFrameID(At<void*>(self, kVBManager));
    }
    return 1;
}

// 0x877D40 (vf88): the camera last set (unless the device is lost): its location, direction,
// up and right (+0x840, +0x830, +0x820, +0x810), frustum and port
void __fastcall Renderer_GetCameraData(uint8_t* self, re::Edx, float* location, float* direction, float* up,
                                       float* right, uint32_t* frustum, uint32_t* port) {
    if (At<uint8_t>(self, kDeviceLost) != 0) {
        return;
    }
    for (int i = 0; i < 3; i++) {
        right[i] = At<float>(self, 0x810 + i * 4);
        up[i] = At<float>(self, 0x820 + i * 4);
        direction[i] = At<float>(self, 0x830 + i * 4);
        location[i] = At<float>(self, 0x840 + i * 4);
    }
    for (int i = 0; i < 7; i++) {
        frustum[i] = At<uint32_t>(self, kFrustum + i * 4);
    }
    for (int i = 0; i < 4; i++) {
        port[i] = At<uint32_t>(self, 0x914 + i * 4);
    }
}

// 0x877E10 (vf91): the two values at +0x444
void __fastcall Renderer_Set444(uint8_t* self, re::Edx, uint32_t a, uint32_t b) {
    At<uint32_t>(self, 0x444) = a;
    At<uint32_t>(self, 0x448) = b;
}

// 0x877E30 (thiscall): the pointer list at +0x43C emptied: its nodes onto the free list
void __fastcall Renderer_ClearList43C(uint8_t* self, re::Edx) {
    uint8_t* node = At<uint8_t*>(self, 0x43C);
    while (node != nullptr) {
        uint8_t* next = At<uint8_t*>(node, 0xC);
        At<uint8_t*>(node, 0xC) = ms_pFreeListNodes_20E6D14;
        ms_pFreeListNodes_20E6D14 = node;
        node = next;
    }
    At<uint32_t>(self, 0x440) = 0;
    At<uint32_t>(self, 0x43C) = 0;
}

// 0x877F00 (thiscall): one of the four pixel format arrays (22 entries): each of Gamebryo's
// formats (6 and 7 skipped) copied from its static NiPixelFormat, given its Direct3D format
// and its index, the old entry freed, the new one kept when the adapter makes a resource of
// `type` in it (CheckDeviceFormat; the usage argument unused: the check passes 0)
void __fastcall Renderer_CreatePixelFormatArrayImpl(uint8_t* self, re::Edx, re::Ptr32<void>* formats, uint32_t,
                                                    uint32_t type) {
    static const uint32_t kStatic[22] = {0x00BC7FC0, 0x00BC8008, 0x00BC8290, 0x00BC8098, 0x00BC8128, 0x00BC8200,
                                         0,          0,          0x00BC7900, 0x00BC7948, 0x00BC7990, 0x00BC7870,
                                         0x00BC8440, 0x00BC78B8, 0x00BC7E58, 0x00BC7EA0, 0x00BC7B40, 0x00BC7AB0,
                                         0x00BC7A20, 0x00BC7AF8, 0x00BC7A68, 0x00BC79D8};
    for (uint32_t i = 0; i < 22; i++) {
        if (kStatic[i] == 0) {
            continue;
        }
        uint8_t* format = static_cast<uint8_t*>(NiNew(0x44));
        // (a failed allocation: the game writes through null)
        if (format != nullptr) {
            std::memcpy(format, reinterpret_cast<const void*>(uintptr_t(kStatic[i])), 0x44);
        }
        At<uint32_t>(format, 0xC) = PixelFormat_GetD3DFormat(format);
        At<uint32_t>(format, 0x10) = i;
        NiDelete(formats[i], 0x44);
        if (Com<long>(ms_pkD3D9, 10, At<uint32_t>(self, kAdapter), At<uint32_t>(self, kDevType),
                      At<uint32_t>(self, kAdapterFormat), 0u, type, At<uint32_t>(format, 0xC)) != 0) {   // CheckDeviceFormat
            NiDelete(format, 0x44);
            format = nullptr;
        }
        formats[i] = format;
    }
}

} // namespace

// 0x877BF0: a device description as Direct3D's device type and behaviour flags (bits 4..7
// replaced): 0 a pure HAL device with hardware vertex processing, 1..3 HAL with hardware /
// mixed / software vertex processing, 4..6 the reference rasterizer with software / hardware /
// mixed; 0 back for any other
uint8_t __cdecl GetDeviceTypeAndBehaviorImpl(uint32_t desc, uint32_t* devType, uint32_t* behavior) {
    static const uint32_t kDevType[7] = {1, 1, 1, 1, 2, 2, 2};                // D3DDEVTYPE_HAL, _REF
    static const uint32_t kFlags[7] = {0x50, 0x40, 0x80, 0x20, 0x20, 0x40, 0x80};   // D3DCREATE_*
    *behavior &= 0xFFFFFF0Fu;
    if (desc > 6) {
        return 0;
    }
    *devType = kDevType[desc];
    *behavior |= kFlags[desc];
    return 1;
}

// 0x878570 (thiscall): the behaviour flags as text (+0x40C: "PURE MT HWVP"...)
const char* __fastcall Renderer_GetBehaviorStringImpl(uint8_t* self, re::Edx) {
    char* text = reinterpret_cast<char*>(self + kBehavior);
    uint32_t flags = At<uint32_t>(self, kBehaviorFlags);
    text[0] = '\0';
    if (flags & 0x10) {   // D3DCREATE_PUREDEVICE
        crt::StrcatS(text, 0x20, "PURE");
    }
    if (At<uint32_t>(self, kBehaviorFlags) & 0x4) {   // MULTITHREADED
        crt::StrcatS(text, 0x20, " MT");
    }
    if (At<uint32_t>(self, kBehaviorFlags) & 0x40) {   // HARDWARE_VERTEXPROCESSING
        crt::StrcatS(text, 0x20, " HWVP");
    }
    if (At<uint32_t>(self, kBehaviorFlags) & 0x80) {   // MIXED_VERTEXPROCESSING
        crt::StrcatS(text, 0x20, " MIXVP");
    }
    if (At<uint32_t>(self, kBehaviorFlags) & 0x20) {   // SOFTWARE_VERTEXPROCESSING
        crt::StrcatS(text, 0x20, " SWVP");
    }
    return text;
}

// 0x878600: NiDX9Renderer::GetCompatibleAdapterFormat (a frame buffer format's display mode
// format: alpha dropped)
uint32_t __cdecl GetCompatibleAdapterFormatImpl(uint32_t format) {
    switch (format) {
    case 1:   // FBFMT_R8G8B8
        return 20;   // D3DFMT_R8G8B8
    case 2:
    case 3:   // A8R8G8B8, X8R8G8B8
        return 22;   // D3DFMT_X8R8G8B8
    case 4:   // R5G6B5
        return 23;
    case 5:
    case 6:   // X1R5G5B5, A1R5G5B5
        return 24;   // D3DFMT_X1R5G5B5
    case 7:
    case 11:   // A4R4G4B4, X4R4G4B4
        return 30;   // D3DFMT_X4R4G4B4
    case 8:   // R3G3B2
        return 27;
    default:
        return 0;
    }
}

// 0x878670: NiDX9Renderer::GetMultiSampleType (FBMODE_MULTISAMPLES_n: the high word; 0 / 1
// (default, lockable): none)
uint32_t __cdecl GetMultiSampleTypeImpl(uint32_t mode) {
    if (mode == 0 || mode == 1) {
        return 0;
    }
    switch (mode & 0xFFFF0000) {
    case 0x10000:
        return 2;
    case 0x20000:
        return 3;
    case 0x30000:
        return 4;
    case 0x40000:
        return 5;
    case 0x50000:
        return 6;
    case 0x60000:
        return 7;
    case 0x70000:
        return 8;
    case 0x80000:
        return 9;
    case 0x90000:
        return 10;
    case 0xA0000:
        return 11;
    case 0xB0000:
        return 12;
    case 0xC0000:
        return 13;
    case 0xD0000:
        return 14;
    case 0xE0000:
        return 15;
    case 0xF0000:
        return 16;
    case 0x80000000:
        return 1;   // D3DMULTISAMPLE_NONMASKABLE
    default:
        return 0;
    }
}

// 0x878770: NiDX9Renderer::GetPresentInterval (PRESENT_INTERVAL_* to D3DPRESENT_INTERVAL_*)
uint32_t __cdecl GetPresentIntervalImpl(uint32_t interval) {
    switch (interval) {
    case 0:
        return 0x80000000;   // IMMEDIATE
    case 1:
        return 1;
    case 2:
        return 2;
    case 3:
        return 4;
    case 4:
        return 8;
    default:
        return 0;
    }
}

// 0x8787C0: NiD3DUtility::GetD3DFromNi (the rotation scaled, the translation in the 4th column)
void __cdecl GetD3DFromNi(float* d3d, const float* rotate, const float* translate, float scale) {
    d3d[0] = rotate[0] * scale;
    d3d[1] = rotate[1] * scale;
    d3d[2] = rotate[2] * scale;
    d3d[3] = translate[0];
    d3d[4] = rotate[3] * scale;
    d3d[5] = rotate[4] * scale;
    d3d[6] = rotate[5] * scale;
    d3d[7] = translate[1];
    d3d[8] = rotate[6] * scale;
    d3d[9] = rotate[7] * scale;
    d3d[10] = scale * rotate[8];
    d3d[11] = translate[2];
    d3d[12] = 0.0f;
    d3d[13] = 0.0f;
    d3d[14] = 0.0f;
    d3d[15] = 1.0f;
}

// 0x878840 (thiscall): NiTMapBase<NiTPointerAllocator<...>>'s deleting destructor (vtable 0x99ACD4)
void* __fastcall TMapBase_Destroy(uint8_t* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = kTMapBaseVtable;
    TMapBase_RemoveAll(self);
    NiFree(At<void*>(self, 8));
    if (flags & 1) {
        NiDelete(self, 0x10);
    }
    return self;
}

// 0x878880 (thiscall): the members' defaults (the constructor's last step: no device, grey
// background, identity matrices, a unit viewport, NiRenderer's +0x20..+0x2C, the D3DFORMAT bit
// depths table)
void __fastcall Renderer_InitializeMembers(uint8_t* self, re::Edx) {
    At<void*>(self, kDevice) = nullptr;
    crt::Memset(self + kCaps, 0, 0x130);
    At<float>(self, kZClear) = 1.0f;
    At<uint32_t>(self, kDeviceWindow) = 0;
    self[kDriverDesc] = 0;
    At<uint32_t>(self, kAdapter) = 0;
    At<uint32_t>(self, kFocusWindow) = 0;
    At<uint32_t>(self, kBehaviorFlags) = 0;
    At<uint32_t>(self, kAdapterFormat) = 0;
    self[kSWVertexCapable] = 0;
    self[kSWVertexSwitchable] = 0;
    At<uint32_t>(self, kHWBones) = 0;
    At<uint32_t>(self, kMaxStreams) = 0;
    At<uint32_t>(self, kPixelShaderVersion) = 0;
    At<uint32_t>(self, kVertexShaderVersion) = 0;
    self[kMipmapCubeMaps] = 0;
    self[kDynamicTexturesCapable] = 0;
    At<uint32_t>(self, kMaxRenderTargets) = 0;
    self[kIndependentBitDepths] = 0;
    self[kMRTPostPixelShaderBlending] = 0;
    At<uint32_t>(self, kResetCounter) = 0;
    self[kDeviceLost] = 0;
    At<uint32_t>(self, kBackground) = 0xFF808080;
    At<uint32_t>(self, kStencilClear) = 0;
    At<uint32_t>(self, kRendFlags) = 0;
    self[kBehavior] = 0;
    At<uint32_t>(self, kDevType) = 1;   // D3DDEVTYPE_HAL
    for (int i = 0; i < 6; i++) {
        At<uint32_t>(self, 0x450 + i * 4) = ms_akUnitAxes[i];
    }
    for (int i = 0; i < 6; i++) {
        At<uint32_t>(self, 0x468 + i * 4) = ms_akUnitAxes[i];
    }
    At<float>(self, 0x480) = 0.1f;
    At<float>(self, 0x484) = 100.0f;
    crt::Memset(self + 0x488, 0, 0x40);
    At<float>(self, 0x4C4) = 1.0f;
    At<float>(self, 0x4B0) = 1.0f;
    At<uint32_t>(self, 0x43C) = 0;
    At<float>(self, 0x49C) = 1.0f;
    At<uint32_t>(self, 0x440) = 0;
    At<float>(self, 0x488) = 1.0f;
    // the viewport (D3DVIEWPORT9 at +0x4C8: 0, 0, 1, 1, 0.0, 1.0)
    At<uint32_t>(self, 0x4D0) = 1;
    At<float>(self, 0x4D8) = 0.0f;
    At<uint32_t>(self, 0x4D4) = 1;
    At<uint32_t>(self, 0x4C8) = 0;
    At<float>(self, 0x4DC) = 1.0f;
    At<uint32_t>(self, 0x4CC) = 0;
    for (int usage = 0; usage < 4; usage++) {
        crt::Memset(self + kTextureFormats + usage * 0x58, 0, 0x58);
        At<void*>(self, kDefaultTextureFormat + usage * 4) = nullptr;
        re::Ptr32<void>& data = At<void*>(self, kDefaultTextureData + usage * 4);
        if (data != nullptr) {
            ReleaseRef(data);
            data = nullptr;
        }
    }
    At<float>(self, 0x75C) = 0.0f;
    At<float>(self, 0x76C) = 0.0f;
    At<float>(self, 0x77C) = 0.0f;
    At<uint32_t>(self, kReplacementDataFormat) = 0x16;
    At<uint32_t>(self, kCurrentRenderTargetGroup) = 0;
    At<float>(self, 0x78C) = 1.0f;
    At<uint32_t>(self, kCurrentOnscreenTarget) = 0;
    At<uint32_t>(self, kVBManager) = 0;
    At<uint32_t>(self, kIBManager) = 0;
    At<uint32_t>(self, kTextureManager) = 0;
    At<uint32_t>(self, kRenderState) = 0;
    At<uint32_t>(self, kLightManager) = 0;
    At<uint32_t>(self, 0x6A8) = 0;
    At<uint32_t>(self, 0x6AC) = 0;
    At<uint32_t>(self, 0x6B0) = 0;
    At<uint32_t>(self, 0x850) = 0;
    At<uint32_t>(self, 0x854) = 0;
    At<uint32_t>(self, 0x858) = 0;
    At<uint32_t>(self, 0x860) = 0;
    At<uint16_t>(self, 0x85C) = 0;
    At<uint32_t>(self, 0x864) = 0;
    // bits per pixel by D3DFORMAT
    uint8_t* sizes = ms_aucD3DFormatSizes;
    crt::Memset(sizes, 0, 0x76);
    static const struct {
        uint8_t format, bits;
    } kSizes[] = {
        {0x14, 0x18}, {0x15, 0x20}, {0x16, 0x20}, {0x17, 0x10}, {0x18, 0x10}, {0x19, 0x10}, {0x1A, 0x10},
        {0x1B, 8},    {0x1C, 8},    {0x1D, 0x10}, {0x1E, 0x10}, {0x1F, 0x20}, {0x20, 0x20}, {0x21, 0x20},
        {0x22, 0x20}, {0x23, 0x20}, {0x24, 0x40}, {0x28, 0x10}, {0x29, 8},    {0x32, 8},    {0x33, 0x10},
        {0x34, 8},    {0x3C, 0x10}, {0x3D, 0x10}, {0x3E, 0x20}, {0x3F, 0x20}, {0x40, 0x20}, {0x43, 0x20},
        {0x46, 0x10}, {0x47, 0x20}, {0x49, 0x10}, {0x4B, 0x20}, {0x4D, 0x20}, {0x4F, 0x20}, {0x50, 0x10},
        {0x52, 0x20}, {0x53, 0x20}, {0x51, 0x10}, {0x65, 0x10}, {0x66, 0x20}, {0x6E, 0x40}, {0x6F, 0x10},
        {0x70, 0x20}, {0x71, 0x40}, {0x72, 0x20}, {0x73, 0x40}, {0x74, 0x80}, {0x75, 0x10},
    };
    for (const auto& s : kSizes) {
        sizes[s.format] = s.bits;
    }
    At<float>(self, 0x28) = 0.01f;
    At<float>(self, 0x20) = 0.01f;
    At<float>(self, 0x24) = 0.99f;
    self[kFrustum + 0x18] = 0;   // (not orthographic)
    At<float>(self, 0x2C) = 0.99f;
    At<float>(self, kFrustum) = -0.5f;
    At<float>(self, kFrustum + 4) = 0.5f;
    At<float>(self, kFrustum + 8) = 0.5f;
    At<float>(self, kFrustum + 0xC) = -0.5f;
    At<float>(self, kFrustum + 0x10) = 1.0f;
    At<float>(self, kFrustum + 0x14) = 2.0f;
    At<float>(self, kPort) = 0.0f;
    At<float>(self, kPort + 0xC) = 0.0f;
    At<float>(self, kPort + 4) = 1.0f;
    At<float>(self, kPort + 8) = 1.0f;
}

// 0x878D10: NiDX9Renderer::CreateDirect3D9 (D3D9.DLL's Direct3DCreate9 once, under the D3D
// lock; 0 or -1)
int __cdecl CreateDirect3D9Impl() {
    int result = 0;
    Lock(ms_kD3DCriticalSection);
    if (ms_pkD3D9 == nullptr) {
        bool created = false;
        if (ms_pfnD3DCreate9 == nullptr) {
            ms_hDXLib = Import<void*>(kLoadLibraryA, "D3D9.DLL");
            if (ms_hDXLib != nullptr) {
                ms_pfnD3DCreate9 = Import<void*>(kGetProcAddress, ms_hDXLib, "Direct3DCreate9");
            }
        }
        if (ms_pfnD3DCreate9 != nullptr) {
            ms_pkD3D9 = re::PtrCast<void*(__stdcall*)(uint32_t)>(ms_pfnD3DCreate9)(0x20);   // D3D_SDK_VERSION
            created = ms_pkD3D9 != nullptr;
        }
        if (created) {
            NiOutputDebugString("Create D3D9 instance...SUCCESSFUL\n");
        } else {
            NiOutputDebugString("Create D3D9 instance...FAILED\n");
            result = -1;
        }
    }
    Unlock(ms_kD3DCriticalSection);
    return result;
}

// 0x878DC0: NiDX9Renderer's statics let go of, under the D3D lock: Direct3D released, the
// system description deleted, D3D9.DLL freed (its handle kept), Direct3DCreate9 forgotten, the
// display modes emptied
void __cdecl Renderer_ShutdownStaticsImpl() {
    Lock(ms_kD3DCriticalSection);
    if (ms_pkD3D9 != nullptr) {
        Com<unsigned long>(ms_pkD3D9, 2);   // Release
        ms_pkD3D9 = nullptr;
    }
    if (void* desc = ms_pkSystemDesc) {
        SystemDesc_Destruct(desc);
        NiDelete(desc, 0x14);
    }
    ms_pkSystemDesc = nullptr;
    if (ms_hDXLib != nullptr) {
        Import<int>(kFreeLibrary, static_cast<void*>(ms_hDXLib));
    }
    ms_pfnD3DCreate9 = nullptr;
    PointerList_RemoveAll(kDisplayModeList);
    Unlock(ms_kD3DCriticalSection);
}

// 0x87AC00: NiDX9Renderer::GetSystemDesc (made on first use: Direct3D created outside the lock)
void* __cdecl GetSystemDesc() {
    Lock(ms_kD3DCriticalSection);
    if (ms_pkSystemDesc != nullptr) {
        Unlock(ms_kD3DCriticalSection);
        return ms_pkSystemDesc;
    }
    Unlock(ms_kD3DCriticalSection);
    CreateDirect3D9();
    Lock(ms_kD3DCriticalSection);
    void* memory = NiNew(0x14);
    ms_pkSystemDesc = memory != nullptr ? SystemDesc_Construct(memory, ms_pkD3D9, kDisplayModeList) : nullptr;
    Unlock(ms_kD3DCriticalSection);
    return ms_pkSystemDesc;
}

// 0x878E60 (thiscall): NiDX9Renderer::InitializePresentParams (the frame buffer and depth
// formats chosen when not given, multisampling checked against the adapter, the back buffer
// count clamped to 1..3, a fullscreen mode matched, the present interval made legal)
uint8_t __fastcall Renderer_InitializePresentParams(uint8_t* self, re::Edx, uint32_t window, uint32_t width,
                                                     uint32_t height, uint32_t useFlags, uint32_t mode,
                                                     uint32_t frameBufferFormat, uint32_t depthFormat,
                                                     uint32_t backBuffers, uint32_t swapEffect,
                                                     uint32_t refreshRate, uint32_t interval, uint32_t* params) {
    uint8_t windowed = uint8_t(~uint8_t(useFlags >> 2) & 1);   // (not USE_FULLSCREEN)
    void* deviceDesc = At<void*>(self, kDeviceDesc);
    uint32_t bpp = (useFlags & 0x10) ? 16 : 32;   // USE_16BITBUFFERS
    if (frameBufferFormat == 0) {
        frameBufferFormat = DeviceDesc_GetDefaultFrameBufferFormat(deviceDesc, windowed, bpp);
        if (frameBufferFormat == 0) {
            SetError("Creation failed: Could not find desired framebuffer format");
            NiOutputDebugString("NiDX9Renderer::Initialize> No matching framebuffer format available\n");
            return 0;
        }
    }
    uint32_t d3dFormat = GetD3DFormat(frameBufferFormat);
    if (depthFormat == 0) {
        uint32_t adapterFormat = GetCompatibleAdapterFormat(frameBufferFormat);
        depthFormat = DeviceDesc_FindDepthStencilFormat(At<void*>(self, kDeviceDesc), adapterFormat, d3dFormat, bpp,
                                                        useFlags & 8);   // USE_STENCIL
        if (depthFormat == 0) {
            SetError("Creation failed: Could not find desired depth/stencil format");
            NiOutputDebugString("NiDX9Renderer::Initialize> No matching depth/stencil format available\n");
            return 0;
        }
    }
    // D3DPRESENT_PARAMETERS
    crt::Memset(params, 0, 0x38);
    params[2] = d3dFormat;   // BackBufferFormat
    params[0] = width;
    params[1] = height;
    uint32_t multiSample = GetMultiSampleType(mode);
    params[4] = multiSample;
    params[5] = (mode == 0 || mode == 1) ? 0 : (mode & 0xFFFF);   // MultiSampleQuality
    params[10] = depthFormat;    // AutoDepthStencilFormat
    params[3] = backBuffers;
    params[9] = ~useFlags & 1;   // EnableAutoDepthStencil
    params[8] = windowed;
    uint32_t effect = 1;   // D3DSWAPEFFECT_DISCARD
    if (mode == 0 || mode == 1) {
        if (swapEffect == 2) {
            effect = 2;
        } else if (swapEffect == 3) {
            effect = 3;
        }
    }
    params[6] = effect;
    params[7] = window;   // hDeviceWindow
    params[11] = mode == 1 ? 1 : 0;   // D3DPRESENTFLAG_LOCKABLE_BACKBUFFER
    params[12] = refreshRate;
    params[13] = GetPresentInterval(interval);
    uint32_t qualityLevels;
    if (Com<long>(ms_pkD3D9, kD3DCheckDeviceMultiSampleType, At<uint32_t>(self, kAdapter), At<uint32_t>(self, kDevType),
                  d3dFormat, uint32_t(windowed), multiSample, &qualityLevels) < 0) {
        NiOutputDebugString(
            "NiDX9Renderer::Initialize> Could not support requested antialiasing mode - disabling\n");
        params[4] = 0;
    } else if (qualityLevels <= params[5]) {
        params[5] = qualityLevels - 1;
    }
    if (params[3] == 0) {
        params[3] = 1;
    } else if (params[3] > 3) {
        params[3] = 3;
    }
    uint32_t requested = params[12];
    if (params[8] != 0) {
        params[12] = 0;
    } else {
        uint32_t format = GetCompatibleAdapterFormat(GetNiFBFormat(params[2]));
        if (AdapterDesc_ValidateFullscreenMode(At<void*>(self, kAdapterDesc), format, params[0], params[1], &params[12]) ==
            nullptr) {
            NiRenderer_SetLastErrorString("Creation failed: Could not match desired fullscreen mode");
            NiOutputDebugString("NiDX9Renderer::Initialize> Could not find matching fullscreen display mode\n");
            return 0;
        }
        if (requested != params[12]) {
            NiOutputDebugString(
                "NiDX9Renderer::Initialize> Could not match requested fullscreen refresh rate - using closest\n");
        }
    }
    if (params[8] != 0) {
        uint32_t p = params[13];
        if (p != 0 && p != 0x80000000 && p != 1) {
            params[13] = 0x80000000;
        }
    } else if ((params[13] & At<uint32_t>(deviceDesc, 0x18)) == 0) {   // (the device's PresentationIntervals)
        params[13] = 1;
    }
    return 1;
}

// 0x879150 (thiscall): NiDX9Renderer::InitializeDeviceCaps (the caps queried; two texture stages
// needed; the renderer's flags, bone and stream limits, shader versions; the render state's stage
// limits and the stage mapping statics). (Its argument isn't read.)
uint8_t __fastcall Renderer_InitializeDeviceCaps(uint8_t* self, re::Edx, uint32_t) {
    if (Com<long>(At<void*>(self, kDevice), kDevGetDeviceCaps, self + kCaps) < 0) {
        SetError("Creation failed: Could not query device caps");
        NiOutputDebugString("NiDX9Renderer::Initialize> Could not query device caps - FAILING\n");
        return 0;
    }
    uint32_t blendStages = At<uint32_t>(self, kCaps + 0x94);   // MaxTextureBlendStages
    if (blendStages < 2 || At<uint32_t>(self, kCaps + 0x98) < 2) {   // MaxSimultaneousTextures
        SetError("Creation failed: Device does not support single-pass multi-texture");
        NiOutputDebugString(
            "NiDX9Renderer::Initialize> Device does not support single-pass multi-texture - FAILING\n");
        return 0;
    }
    uint32_t textureCaps = At<uint32_t>(self, kCaps + 0x3C);
    At<uint32_t>(self, kRendFlags) = 0;
    if ((textureCaps & 2) == 0) {   // D3DPTEXTURECAPS_POW2
        At<uint32_t>(self, kRendFlags) = 8;
    } else if (textureCaps & 0x100) {   // NONPOW2CONDITIONAL
        At<uint32_t>(self, kRendFlags) = 4;
    }
    At<uint32_t>(self, kHWBones) = At<uint32_t>(self, kCaps + 0xA8);   // MaxVertexBlendMatrices
    if (self[kSWVertexCapable] != 0 && self[kSWVertexSwitchable] == 0) {
        At<uint32_t>(self, kHWBones) = 4;
    }
    if (At<uint32_t>(self, kHWBones) >= 4) {
        At<uint32_t>(self, kRendFlags) |= 2;   // hardware skinning
    }
    uint32_t miscCaps = At<uint32_t>(self, kCaps + 0x20);      // PrimitiveMiscCaps
    uint32_t vertexShader = At<uint32_t>(self, kCaps + 0xC4);  // VertexShaderVersion
    self[kMipmapCubeMaps] = uint8_t((textureCaps >> 16) & 1);
    self[kDynamicTexturesCapable] = uint8_t((At<uint32_t>(self, kCaps + 0xC) >> 29) & 1);   // Caps2
    At<uint32_t>(self, kMaxRenderTargets) = At<uint32_t>(self, kCaps + 0xF0);   // NumSimultaneousRTs
    self[kIndependentBitDepths] = uint8_t((miscCaps >> 18) & 1);
    At<uint32_t>(self, kMaxStreams) = At<uint32_t>(self, kCaps + 0xBC);
    uint32_t pixelShader = At<uint32_t>(self, kCaps + 0xCC);
    self[kMRTPostPixelShaderBlending] = uint8_t((miscCaps >> 19) & 1);
    At<uint32_t>(self, kPixelShaderVersion) = pixelShader;
    At<uint32_t>(self, kVertexShaderVersion) = vertexShader;
    // the shader version ranges (+0x71C..+0x740)
    At<uint32_t>(self, 0x71C) = pixelShader;
    At<uint32_t>(self, 0x720) = 0xFFFF0000;
    At<uint32_t>(self, 0x724) = pixelShader;
    At<uint32_t>(self, 0x728) = vertexShader;
    At<uint32_t>(self, 0x72C) = 0xFFFE0000;
    At<uint32_t>(self, 0x730) = vertexShader;
    At<uint32_t>(self, 0x734) = 0xFFFE0000;
    At<uint32_t>(self, 0x738) = 0xFFFE0000;
    At<uint32_t>(self, 0x73C) = 0xFFFE0000;
    At<uint32_t>(self, 0x740) = 2;
    RenderState_SetMaxTextureBlendStages(blendStages);
    RenderState_SetMaxSimultaneousTextures(At<uint32_t>(self, kCaps + 0x98));
    RenderState_SetMaxSamplers(At<uint32_t>(self, kPixelShaderVersion) >= 0xFFFF0200 ? 16
                                                                                     : At<uint32_t>(self, kCaps + 0x94));
    // the stage mapping (0x20E6C98..0x20E6CFC, by index from 0x20E6C98)
    static const struct {
        uint8_t index, value;
    } kStages[] = {
        {0x17, 3}, {0x14, 3}, {0x12, 3}, {0x13, 3}, {2, 0},   {5, 0},   {0x18, 1}, {0x19, 1}, {0x16, 1},
        {0x15, 1}, {1, 1},    {0, 1},    {4, 2},    {3, 2},   {7, 2},   {6, 2},    {8, 2},    {0xA, 1},
        {9, 1},    {0xB, 1},  {0xD, 1},  {0xC, 1},  {0xE, 2}, {0x10, 2}, {0xF, 2},  {0x11, 1},
    };
    for (const auto& s : kStages) {
        ms_auiStageMap[s.index] = s.value;
    }
    ms_uiTexCoordFlags = (At<uint32_t>(self, kCaps + 0x3C) & 0x400) ? 0x103 : 2;
    return 1;
}

// 0x879400 (thiscall): NiDX9Renderer::InitializeTextureDefaults (per usage the first format
// there is, in the order 1 0 2 4 3 5, and a 4x4 texture of it filled with 0xFF; the replacement
// format from the first). (A failed allocation is used anyway: the game's crash.)
uint8_t __fastcall Renderer_InitializeTextureDefaultsImpl(uint8_t* self, re::Edx) {
    static const uint32_t kOrder[6] = {1, 0, 2, 4, 3, 5};
    for (int usage = 0; usage < 4; usage++) {
        re::Ptr32<void>& format = At<void*>(self, kDefaultTextureFormat + usage * 4);
        format = nullptr;
        for (uint32_t k = 0; k < 6; k++) {
            void* candidate = At<void*>(self, kTextureFormats + int(kOrder[k] + usage * 0x16) * 4);
            if (candidate != nullptr) {
                format = candidate;
                break;
            }
        }
        if (format == nullptr) {
            continue;
        }
        void* memory = NiNew(0x70);
        void* data = memory != nullptr ? PixelData_Construct(memory, 4, 4, format, 1, 1) : nullptr;
        AssignRef(At<void*>(self, kDefaultTextureData + usage * 4), data);
        uint8_t* held = At<uint8_t*>(self, kDefaultTextureData + usage * 4);
        uint8_t* pixels = *At<re::Ptr32<uint8_t>*>(held, 0x5C) + At<uint32_t>(held, 0x50);
        crt::Memset(pixels, 0xFF, At<uint32_t>(held, 0x64) << 4);
    }
    void* first = At<void*>(self, kDefaultTextureFormat);
    if (first == nullptr) {
        return 0;
    }
    PixelFormat_Initialize(first);
    At<uint32_t>(self, kReplacementDataFormat) = At<uint32_t>(At<void*>(self, kDefaultTextureFormat), 0x10);
    return 1;
}

// 0x87B280 (thiscall): NiDX9Renderer::InitializeTextureFormats (the four format arrays: textures,
// rendered textures, cube maps, rendered cube maps; the defaults; the clipper image: a 2x1
// texture of the default format, half transparent)
uint8_t __fastcall Renderer_InitializeTextureFormats(uint8_t* self, re::Edx) {
    Renderer_CreatePixelFormatArray(self, &At<void*>(self, kTextureFormats), 0, 3);          // D3DRTYPE_TEXTURE
    Renderer_CreatePixelFormatArray(self, &At<void*>(self, kTextureFormats + 0x58), 1, 3);   // RENDERTARGET
    Renderer_CreatePixelFormatArray(self, &At<void*>(self, kTextureFormats + 0xB0), 0, 5);   // CUBETEXTURE
    Renderer_CreatePixelFormatArray(self, &At<void*>(self, kTextureFormats + 0x108), 1, 5);
    Renderer_InitializeTextureDefaults(self);
    uint8_t* format = At<uint8_t*>(self, kDefaultTextureFormat);
    void* memory = NiNew(0x70);
    uint8_t* data = nullptr;
    if (memory != nullptr) {
        data = static_cast<uint8_t*>(PixelData_Construct(memory, 2, 1, format, 1, 1));
        AddRefRef(data);
    }
    uint8_t* pixels = *At<re::Ptr32<uint8_t>*>(data, 0x5C) + At<uint32_t>(data, 0x50);
    uint8_t bits = format[1];
    if (bits == 0x10) {
        pixels[3] = 0xFF;
        pixels[2] = 0xFF;
        pixels[0] = 0;
        pixels[1] = 0;
    } else if (bits == 0x20) {
        pixels[3] = 0;
        pixels[2] = 0;
        pixels[7] = 0xFF;
        pixels[6] = 0xFF;
        pixels[5] = 0xFF;
        pixels[4] = 0xFF;
        pixels[0] = 0;
        pixels[1] = 0;
    }
    uint32_t prefs[3] = {6, 0, 0};   // NiTexture::FormatPrefs: pixel layout 6, no mipmaps, default alpha
    void* texture = SourceTexture_CreateFromData(data, prefs);
    AssignRef(At<void*>(self, kClipperImage), texture);
    ReleaseRef(data);
    return 1;
}

// 0x87B3D0 (thiscall): NiDX9Renderer::GetDriverInfo ("<description> (HAL-<behaviour>)", from the
// adapter's identifier; empty when that fails)
const char* __fastcall Renderer_GetDriverInfo(uint8_t* self, re::Edx) {
    alignas(8) uint8_t identifier[0x44C];   // D3DADAPTER_IDENTIFIER9
    char* text = reinterpret_cast<char*>(self + kDriverDesc);
    text[0] = '\0';
    crt::Memset(identifier, 0, sizeof(identifier));
    if (Com<long>(ms_pkD3D9, kD3DGetAdapterIdentifier, At<uint32_t>(self, kAdapter), 0u, identifier) >= 0) {
        const char* type;
        switch (At<uint32_t>(self, kDevType)) {
        case 1:
            type = "HAL";
            break;
        case 2:
            type = "REF";
            break;
        case 3:
            type = "SW";
            break;
        default:
            type = "???";
            break;
        }
        NiSprintf(text, 0x200, "%s (%s-%s)", reinterpret_cast<const char*>(identifier + 0x200), type,
                  Renderer_GetBehaviorString(self));
    }
    return text;
}

#define DX9REND38_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

DX9REND38_REVERSED("NiGeometryBufferData::GetVBChip", 0x00878550, BufferData_GetVBChip);
DX9REND38_REVERSED("NiDX9Renderer::GetDeviceTypeAndBehavior", 0x00877BF0, GetDeviceTypeAndBehaviorImpl);
DX9REND38_REVERSED("NiDX9Renderer::CreateDepthStencilData", 0x00877EC0, Renderer_CreateDepthStencilData);
DX9REND38_REVERSED("NiDX9Renderer::FindClosestPixelFormat", 0x00877CA0, Renderer_FindClosestPixelFormat);
DX9REND38_REVERSED("NiDX9Renderer::GetMaxBuffersPerRenderTargetGroup", 0x00877CC0, Renderer_GetMaxBuffersPerRenderTargetGroup);
DX9REND38_REVERSED("NiDX9Renderer::GetIndependentBufferBitDepths", 0x00877CD0, Renderer_GetIndependentBufferBitDepths);
DX9REND38_REVERSED("NiDX9Renderer::PrecacheTexture", 0x00877CE0, Renderer_PrecacheTexture);
DX9REND38_REVERSED("NiDX9Renderer::PurgeMaterial", 0x00877E70, Renderer_PurgeMaterial);
DX9REND38_REVERSED("NiDX9Renderer::PurgeAllTextures", 0x00877CF0, Renderer_PurgeAllTextures);
DX9REND38_REVERSED("NiDX9Renderer::CreateFragmentShader", 0x00877E80, Renderer_CreateFragmentShader);
DX9REND38_REVERSED("NiDX9Renderer::CreateShadowWriteShader", 0x00877EA0, Renderer_CreateShadowWriteShader);
DX9REND38_REVERSED("NiDX9Renderer::EndScene", 0x00877D00, Renderer_EndScene);
DX9REND38_REVERSED("NiDX9Renderer::GetCameraData", 0x00877D40, Renderer_GetCameraData);
DX9REND38_REVERSED("NiDX9Renderer::Set444", 0x00877E10, Renderer_Set444);
DX9REND38_REVERSED("NiDX9Renderer::ClearList43C", 0x00877E30, Renderer_ClearList43C);
DX9REND38_REVERSED("NiDX9Renderer::CreatePixelFormatArray", 0x00877F00, Renderer_CreatePixelFormatArrayImpl);
DX9REND38_REVERSED("NiDX9Renderer::GetBehaviorString", 0x00878570, Renderer_GetBehaviorStringImpl);
DX9REND38_REVERSED("NiDX9Renderer::GetCompatibleAdapterFormat", 0x00878600, GetCompatibleAdapterFormatImpl);
DX9REND38_REVERSED("NiDX9Renderer::GetMultiSampleType", 0x00878670, GetMultiSampleTypeImpl);
DX9REND38_REVERSED("NiDX9Renderer::GetPresentInterval", 0x00878770, GetPresentIntervalImpl);
DX9REND38_REVERSED("NiD3DUtility::GetD3DFromNi", 0x008787C0, GetD3DFromNi);
DX9REND38_REVERSED("NiTMapBase<NiTPointerAllocator>::Destroy_878840", 0x00878840, TMapBase_Destroy);
DX9REND38_REVERSED("NiDX9Renderer::InitializeMembers", 0x00878880, Renderer_InitializeMembers);
DX9REND38_REVERSED("NiDX9Renderer::CreateDirect3D9", 0x00878D10, CreateDirect3D9Impl);
DX9REND38_REVERSED("NiDX9Renderer::ShutdownStatics", 0x00878DC0, Renderer_ShutdownStaticsImpl);
DX9REND38_REVERSED("NiDX9Renderer::InitializePresentParams", 0x00878E60, Renderer_InitializePresentParams);
DX9REND38_REVERSED("NiDX9Renderer::InitializeDeviceCaps", 0x00879150, Renderer_InitializeDeviceCaps);
DX9REND38_REVERSED("NiDX9Renderer::InitializeTextureDefaults", 0x00879400, Renderer_InitializeTextureDefaultsImpl);
DX9REND38_REVERSED("NiDX9Renderer::GetSystemDesc", 0x0087AC00, GetSystemDesc);
DX9REND38_REVERSED("NiDX9Renderer::InitializeTextureFormats", 0x0087B280, Renderer_InitializeTextureFormats);
DX9REND38_REVERSED("NiDX9Renderer::GetDriverInfo", 0x0087B3D0, Renderer_GetDriverInfo);

} // namespace ni::dx9
