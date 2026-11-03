// The D3D10 renderer data of an Ni2DBuffer (the DX9 ones' counterparts, src/gamebryo/
// NiDX92DBufferData.cpp). NiD3D102DBufferData (NiRTTI 0x20E9068, no base RTTI): an NiRefObject with
// +0x8 the buffer, +0xC its NiPixelFormat (the renderer's for the DXGI format), +0x10 the
// multisampling (samples << 8 | quality; 0x100 at first), +0x14 the view (render target / depth
// stencil, held), +0x18 the texture (held). Kinds (5 slots: 0 the deleting destructor, 2 GetRTTI,
// 3 CanDisplayFrame, 4 DisplayFrame): NiD3D10RenderTargetBufferData (vtable 0x9A0A34, NiRTTI
// 0x20E909C, 0x1C bytes), NiD3D10SwapChainBufferData (0x9A0A48, 0x20E90D0, 0x20 bytes: +0x1C the
// IDXGISwapChain), NiD3D10DepthStencilBufferData (0x9A0A5C, 0x20E9104, 0x1C bytes). Their Creates
// make the view of a texture and the data, and the buffer (or give an existing one the data).
//
// Also two cdecl helpers here: an NiTransform made a D3D matrix (0x406670, 0x8787C0: the game's own
// code calls the first).

#include "gamebryo/NiD3D10C43.h"
#include "reversed.h"

namespace ni::c43 {
namespace {

constexpr uintptr_t kRenderTargetBufferDataVtable = 0x009A0A34;
constexpr uintptr_t kSwapChainBufferDataVtable = 0x009A0A48;
constexpr uintptr_t kDepthStencilBufferDataVtable = 0x009A0A5C;
constexpr uintptr_t kIIDTexture2D = 0x009A0A24;   // IID_ID3D10Texture2D

GAME_VAR(uint8_t, NiRTTI_NiD3D10RenderTargetBufferData, 0x020E909C);
GAME_VAR(uint8_t, NiRTTI_NiD3D10SwapChainBufferData, 0x020E90D0);
GAME_VAR(uint8_t, NiRTTI_NiD3D10DepthStencilBufferData, 0x020E9104);

GAME_FN(void, __cdecl, MakeD3DMatrix, 0x00406670, void* out, const void* rotate, const void* translate, float scale);
GAME_FN(void, __cdecl, GetD3DFromNi, 0x008787C0, void* out, const void* rotate, const void* translate, float scale);
GAME_FN(uint8_t*, __cdecl, D3D10Renderer_GetRenderer, 0x008A9300);
GAME_FN(void*, __thiscall, D3D10Renderer_GetD3DDevice, 0x008A9140, const void* renderer);
GAME_FN(const void*, __cdecl, PixelFormat_GetPixelFormat, 0x008A8450, uint32_t format);   // (chunk 42's)
GAME_FN(uint16_t, __cdecl, MakeMultisampleWord, 0x00894360, uint32_t count, uint32_t quality);   // NiDX92DBufferData's
GAME_FN(void*, __cdecl, Ni2DBuffer_Create, 0x007D1EA0, uint32_t width, uint32_t height, void* data);
GAME_FN(void*, __cdecl, NiDepthStencilBuffer_Create, 0x007ABFA0, uint32_t width, uint32_t height, void* data);
GAME_FN(void, __thiscall, Ni2DBuffer_SetRendererData, 0x007ABEF0, void* self, void* data);
// this file's (called where modes apply)
GAME_FN(void*, __thiscall, RenderTargetBufferData_Construct, 0x008B6130, void* self);
GAME_FN(void*, __thiscall, SwapChainBufferData_Construct, 0x008B6180, void* self);
GAME_FN(void, __thiscall, SwapChainBufferData_Destruct, 0x008B61D0, void* self);
GAME_FN(void*, __thiscall, DepthStencilBufferData_Construct, 0x008B6230, void* self);

// D3D10_TEXTURE2D_DESC
struct Texture2DDesc {
    uint32_t width;          // +0x0
    uint32_t height;         // +0x4
    uint32_t mipLevels;      // +0x8
    uint32_t arraySize;      // +0xC
    uint32_t format;         // +0x10
    uint32_t sampleCount;    // +0x14
    uint32_t sampleQuality;  // +0x18
    uint32_t usage;          // +0x1C
    uint32_t bindFlags;      // +0x20
    uint32_t cpuAccess;      // +0x24
    uint32_t miscFlags;      // +0x28
};
static_assert(sizeof(Texture2DDesc) == 0x2C);

// NiRefObject's constructor and the data's fields as the constructors set them
void ConstructData(uint8_t* self, uintptr_t vtable) {
    At<uint32_t>(self, 0) = kRefObjectVtable;
    At<uint32_t>(self, 4) = 0;
    Increment(&ms_uiObjects);
    At<void*>(self, 8) = nullptr;
    At<const void*>(self, 0xC) = nullptr;
    At<void*>(self, 0x14) = nullptr;
    At<void*>(self, 0x18) = nullptr;
    At<uint16_t>(self, 0x10) = 0x100;
    At<uint32_t>(self, 0) = vtable;
}

// the base's part of the destructors: the view and texture let go, NiRefObject's
void DestructData(uint8_t* self, uintptr_t vtable) {
    At<uint32_t>(self, 0) = vtable;
    ComRelease(At<void*>(self, 0x14));
    ComRelease(At<void*>(self, 0x18));
    At<uint32_t>(self, 0) = kRefObjectVtable;
    Decrement(&ms_uiObjects);
}

// the data finished from its texture's description (the format, the multisampling with the count
// at least 1 and the quality's low byte) and its buffer made or given the data
uint8_t* FinishData(uint8_t* data, void* texture, re::Ptr32<void>* buffer, bool depth) {
    Texture2DDesc desc;
    Com<void>(texture, kResourceGetDesc, &desc);
    At<const void*>(data, 0xC) = PixelFormat_GetPixelFormat(desc.format);
    int32_t count = int32_t(desc.sampleCount);
    if (count <= 1) {
        count = 1;
    }
    At<uint16_t>(data, 0x10) = uint16_t(uint32_t(uint8_t(count)) << 8 | uint8_t(desc.sampleQuality));
    if (*buffer == nullptr) {
        *buffer = depth ? NiDepthStencilBuffer_Create(desc.width, desc.height, data)
                        : Ni2DBuffer_Create(desc.width, desc.height, data);
        At<void*>(data, 8) = *buffer;
        return data;
    }
    Ni2DBuffer_SetRendererData(*buffer, data);
    At<void*>(data, 8) = *buffer;
    return data;
}

} // namespace

// 0x8B60A0 (cdecl): an NiTransform made a D3D matrix (0x406670)
void __cdecl Utility_MakeD3DMatrixFromTransform(void* out, const NiTransform* transform) {
    MakeD3DMatrix(out, &transform->rotate, &transform->translate, transform->scale);
}

// 0x8B60C0 (cdecl): an NiTransform made a D3D matrix by NiD3DUtility::GetD3DFromNi (0x8787C0)
void __cdecl Utility_GetD3DFromNiTransform(void* out, const NiTransform* transform) {
    GetD3DFromNi(out, &transform->rotate, &transform->translate, transform->scale);
}

// 0x8B60E0 (thiscall, slot 4 of the render target and depth stencil data): DisplayFrame: E_NOTIMPL
long __fastcall BufferData_DisplayFrameNotImplemented(void*, re::Edx, uint32_t, int) {
    return long(0x80004001);
}

// 0x8B60F0 (thiscall, slot 4): NiD3D10SwapChainBufferData::DisplayFrame(sync interval, flags):
// Present (0 without a swap chain)
long __fastcall SwapChainBufferData_DisplayFrame(uint8_t* self, re::Edx, uint32_t syncInterval, int flags) {
    void* swapChain = At<void*>(self, 0x1C);
    if (swapChain == nullptr) {
        return 0;
    }
    return Com<long>(swapChain, 8, syncInterval, uint32_t(uint8_t(flags) != 0 ? 1 : 0));   // Present
}

// 0x8B6120 (thiscall, 4 bytes: under a hook's size): the pointer at +0x14 (the view; the same code
// folded with other classes' getters)
void* __fastcall BufferData_GetView(uint8_t* self, re::Edx) {
    return At<void*>(self, 0x14);
}
RE_UNHOOKED_FN("NiD3D102DBufferData::GetView", 0x008B6120, &BufferData_GetView);

// 0x8B6130 (thiscall): NiD3D10RenderTargetBufferData::NiD3D10RenderTargetBufferData
uint8_t* __fastcall RenderTargetBufferData_ConstructImpl(uint8_t* self, re::Edx) {
    ConstructData(self, kRenderTargetBufferDataVtable);
    return self;
}

// 0x8B6170 (thiscall, slot 2): NiD3D10RenderTargetBufferData::GetRTTI
void* __fastcall RenderTargetBufferData_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiD3D10RenderTargetBufferData;
}

// 0x8B6180 (thiscall): NiD3D10SwapChainBufferData::NiD3D10SwapChainBufferData
uint8_t* __fastcall SwapChainBufferData_ConstructImpl(uint8_t* self, re::Edx) {
    ConstructData(self, kSwapChainBufferDataVtable);
    At<void*>(self, 0x1C) = nullptr;
    return self;
}

// 0x8B61C0 (thiscall, slot 2): NiD3D10SwapChainBufferData::GetRTTI
void* __fastcall SwapChainBufferData_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiD3D10SwapChainBufferData;
}

// 0x8B61D0 (thiscall): NiD3D10SwapChainBufferData::~NiD3D10SwapChainBufferData (the swap chain out
// of full screen and let go; then the base's)
void __fastcall SwapChainBufferData_DestructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kSwapChainBufferDataVtable;
    void* swapChain = At<void*>(self, 0x1C);
    if (swapChain != nullptr) {
        Com<long>(swapChain, 10, 0u, static_cast<void*>(nullptr));   // SetFullscreenState(FALSE, null)
        Com<long>(At<void*>(self, 0x1C), kRelease);
    }
    DestructData(self, kRenderTargetBufferDataVtable);
}

// 0x8B6230 (thiscall): NiD3D10DepthStencilBufferData::NiD3D10DepthStencilBufferData
uint8_t* __fastcall DepthStencilBufferData_ConstructImpl(uint8_t* self, re::Edx) {
    ConstructData(self, kDepthStencilBufferDataVtable);
    return self;
}

// 0x8B6270 (thiscall, slot 2): NiD3D10DepthStencilBufferData::GetRTTI
void* __fastcall DepthStencilBufferData_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiD3D10DepthStencilBufferData;
}

// 0x8B6280 (thiscall, slot 0): NiD3D10RenderTargetBufferData's deleting destructor
uint8_t* __fastcall RenderTargetBufferData_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    DestructData(self, kRenderTargetBufferDataVtable);
    if (flags & 1) {
        NiDelete(self, 0x1C);
    }
    return self;
}

// 0x8B62D0 (cdecl): NiD3D10RenderTargetBufferData::Create(texture, &buffer, view description): the
// render target view made, the data made (unchecked) with it and the texture (held), its buffer
// made or given the data; 0 when the view can't be
void* __cdecl RenderTargetBufferData_Create(void* texture, re::Ptr32<void>* buffer, const void* desc) {
    if (texture == nullptr) {
        return nullptr;
    }
    void* device = D3D10Renderer_GetD3DDevice(D3D10Renderer_GetRenderer());
    void* view = nullptr;
    long hr = Com<long>(device, kCreateRenderTargetView, texture, desc, &view);
    if (hr < 0) {
        D3D10Error(9, "Error HRESULT = 0x%08X.", hr);
    } else if (view == nullptr) {
        D3D10Error(9, "No error message from D3D10, but render target view is NULL.");
    } else {
        void* memory = NiNew(0x1C);
        uint8_t* data = memory != nullptr ? static_cast<uint8_t*>(RenderTargetBufferData_Construct(memory)) : nullptr;
        At<void*>(data, 0x14) = view;
        At<void*>(data, 0x18) = texture;
        Com<long>(texture, kAddRef);
        return FinishData(data, texture, buffer, false);
    }
    ComRelease(view);
    return nullptr;
}

// 0x8B6400 (thiscall, slot 0): NiD3D10SwapChainBufferData's deleting destructor
uint8_t* __fastcall SwapChainBufferData_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    SwapChainBufferData_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x20);
    }
    return self;
}

// 0x8B6420 (cdecl): NiD3D10SwapChainBufferData::Create(swap chain, &buffer): its back buffer
// (taken over, as is the swap chain: neither held again) and a render target view of it; the
// multisampling by NiDX92DBufferData's word
void* __cdecl SwapChainBufferData_Create(void* swapChain, re::Ptr32<void>* buffer) {
    if (swapChain == nullptr) {
        return nullptr;
    }
    void* device = D3D10Renderer_GetD3DDevice(D3D10Renderer_GetRenderer());
    void* texture = nullptr;
    long hr = Com<long>(swapChain, 9, 0u, reinterpret_cast<const void*>(kIIDTexture2D), &texture);   // GetBuffer
    if (hr < 0) {
        D3D10Error(0x11, "Error HRESULT = 0x%08X.", hr);
    } else if (texture == nullptr) {
        D3D10Error(0x11, "No error message from D3D10, but buffer is NULL.");
    } else {
        void* view = nullptr;
        hr = Com<long>(device, kCreateRenderTargetView, texture, static_cast<const void*>(nullptr), &view);
        if (hr < 0) {
            D3D10Error(9, "Error HRESULT = 0x%08X.", hr);
        } else if (view == nullptr) {
            D3D10Error(9, "No error message from D3D10, but render target view is NULL.");
        } else {
            void* memory = NiNew(0x20);
            uint8_t* data = memory != nullptr ? static_cast<uint8_t*>(SwapChainBufferData_Construct(memory)) : nullptr;
            At<void*>(data, 0x1C) = swapChain;
            At<void*>(data, 0x14) = view;
            At<void*>(data, 0x18) = texture;
            Texture2DDesc desc;
            Com<void>(texture, kResourceGetDesc, &desc);
            At<const void*>(data, 0xC) = PixelFormat_GetPixelFormat(desc.format);
            At<uint16_t>(data, 0x10) = MakeMultisampleWord(desc.sampleCount, desc.sampleQuality);
            if (*buffer == nullptr) {
                *buffer = Ni2DBuffer_Create(desc.width, desc.height, data);
                At<void*>(data, 8) = *buffer;
                return data;
            }
            Ni2DBuffer_SetRendererData(*buffer, data);
            At<void*>(data, 8) = *buffer;
            return data;
        }
        ComRelease(view);
    }
    ComRelease(texture);
    return nullptr;
}

// 0x8B65B0 (thiscall, slot 0): NiD3D10DepthStencilBufferData's deleting destructor
uint8_t* __fastcall DepthStencilBufferData_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    DestructData(self, kDepthStencilBufferDataVtable);
    if (flags & 1) {
        NiDelete(self, 0x1C);
    }
    return self;
}

// 0x8B6600 (cdecl): NiD3D10DepthStencilBufferData::Create(texture, &buffer): the depth stencil view
// made (no description), the data and an NiDepthStencilBuffer (or the existing buffer given it)
void* __cdecl DepthStencilBufferData_Create(void* texture, re::Ptr32<void>* buffer) {
    if (texture == nullptr) {
        return nullptr;
    }
    void* device = D3D10Renderer_GetD3DDevice(D3D10Renderer_GetRenderer());
    void* view = nullptr;
    long hr = Com<long>(device, kCreateDepthStencilView, texture, static_cast<const void*>(nullptr), &view);
    if (hr < 0) {
        D3D10Error(10, "Error HRESULT = 0x%08X.", hr);
    } else if (view == nullptr) {
        D3D10Error(10, "No error message from D3D10, but depth stencil view is NULL.");
    } else {
        void* memory = NiNew(0x1C);
        uint8_t* data = memory != nullptr ? static_cast<uint8_t*>(DepthStencilBufferData_Construct(memory)) : nullptr;
        At<void*>(data, 0x14) = view;
        At<void*>(data, 0x18) = texture;
        Com<long>(texture, kAddRef);
        return FinishData(data, texture, buffer, true);
    }
    ComRelease(view);
    return nullptr;
}

#define GB43H_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

GB43H_REVERSED("NiD3DUtility::MakeD3DMatrixFromTransform", 0x008B60A0, Utility_MakeD3DMatrixFromTransform);
GB43H_REVERSED("NiD3DUtility::GetD3DFromNiTransform", 0x008B60C0, Utility_GetD3DFromNiTransform);
GB43H_REVERSED("NiD3D102DBufferData::DisplayFrameNotImplemented", 0x008B60E0, BufferData_DisplayFrameNotImplemented);
GB43H_REVERSED("NiD3D10SwapChainBufferData::DisplayFrame", 0x008B60F0, SwapChainBufferData_DisplayFrame);
GB43H_REVERSED("NiD3D10RenderTargetBufferData::NiD3D10RenderTargetBufferData", 0x008B6130,
               RenderTargetBufferData_ConstructImpl);
GB43H_REVERSED("NiD3D10RenderTargetBufferData::GetRTTI", 0x008B6170, RenderTargetBufferData_GetRTTI);
GB43H_REVERSED("NiD3D10SwapChainBufferData::NiD3D10SwapChainBufferData", 0x008B6180,
               SwapChainBufferData_ConstructImpl);
GB43H_REVERSED("NiD3D10SwapChainBufferData::GetRTTI", 0x008B61C0, SwapChainBufferData_GetRTTI);
GB43H_REVERSED("NiD3D10SwapChainBufferData::~NiD3D10SwapChainBufferData", 0x008B61D0,
               SwapChainBufferData_DestructImpl);
GB43H_REVERSED("NiD3D10DepthStencilBufferData::NiD3D10DepthStencilBufferData", 0x008B6230,
               DepthStencilBufferData_ConstructImpl);
GB43H_REVERSED("NiD3D10DepthStencilBufferData::GetRTTI", 0x008B6270, DepthStencilBufferData_GetRTTI);
GB43H_REVERSED("NiD3D10RenderTargetBufferData::DeletingDestructor", 0x008B6280,
               RenderTargetBufferData_DeletingDestructor);
GB43H_REVERSED("NiD3D10RenderTargetBufferData::Create", 0x008B62D0, RenderTargetBufferData_Create);
GB43H_REVERSED("NiD3D10SwapChainBufferData::DeletingDestructor", 0x008B6400, SwapChainBufferData_DeletingDestructor);
GB43H_REVERSED("NiD3D10SwapChainBufferData::Create", 0x008B6420, SwapChainBufferData_Create);
GB43H_REVERSED("NiD3D10DepthStencilBufferData::DeletingDestructor", 0x008B65B0,
               DepthStencilBufferData_DeletingDestructor);
GB43H_REVERSED("NiD3D10DepthStencilBufferData::Create", 0x008B6600, DepthStencilBufferData_Create);

} // namespace ni::c43
