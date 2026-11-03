// NiD3DShaderInterface (vtable 0x99BFA8, NiRTTI 0x20E6E0C on NiShader's; 0x24 bytes): NiShader
// with the D3D device (+0x14, held), the renderer (+0x18), its render state (+0x1C) and whether
// it's the best implementation (+0x20). Also the DX9 shader system's set-up and release (the
// statics of passes, stages, programs and constant maps pointed at the renderer, the shader
// factory told, the shader program factory made) and NiDX9ShaderConstantManager's destructor.

#include <cstddef>
#include <cstdint>
#include <initializer_list>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::c39 {
using namespace ni::dx9;
namespace {

constexpr uintptr_t kInterfaceVtable = 0x0099BFA8;
constexpr uintptr_t kNiRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t kConstantManagerVtable = 0x008FF328;

GAME_VAR(void*, NiRTTI_NiD3DShaderInterface, 0x020E6E0C);
GAME_VAR(uint32_t, NiRefObject_ms_uiObjects, 0x020E2168);
GAME_VAR(void*, ms_pkShaderRenderState, 0x020E7430);       // (a static the shader code reads)
GAME_VAR(void*, ms_pkShaderProgramFactory, 0x020E7484);    // NiD3DShaderProgramFactory's instance

GAME_FN(void*, __thiscall, Shader_Construct, 0x0077C2F0, void* self);
GAME_FN(void, __thiscall, Shader_Destruct, 0x0077C350, void* self);
GAME_FN(uint8_t, __thiscall, Shader_Initialize, 0x0077C290, void* self);
GAME_FN(void, __cdecl, NiAlignedFree, 0x007523A0, void* p);
// the shader classes' renderer statics
GAME_FN(void, __cdecl, Pass_SetD3DRenderer, 0x00883D80, void* renderer);
GAME_FN(void, __cdecl, TextureStage_SetD3DRenderer, 0x0089F0E0, void* renderer);
GAME_FN(void, __cdecl, ShaderProgram_SetD3DRenderer, 0x008A20B0, void* renderer);
GAME_FN(void, __cdecl, ShaderProgramFactory_SetD3DRenderer, 0x00889F90, void* renderer);
GAME_FN(void, __cdecl, RenderStateGroup_SetD3DRenderer, 0x0089DBF0, void* renderer);
GAME_FN(void, __cdecl, ShaderProgram_ReleaseAll, 0x008A1EF0);
GAME_FN(void, __thiscall, RenderState_InitRenderState, 0x00883040, void* self);
GAME_FN(void*, __cdecl, ShaderFactory_GetInstance, 0x0089C9C0);
GAME_FN(void, __cdecl, ShaderFactory_LockLibraries, 0x00756630);     // (names a guess)
GAME_FN(void, __cdecl, ShaderFactory_UnlockLibraries, 0x00756650);
GAME_FN(void*, __thiscall, ShaderProgramFactory_Construct, 0x008A2990, void* self);
// this file's (called where modes apply)
GAME_FN(void, __thiscall, D3DShaderInterface_SetD3DRendererAt, 0x00886FE0, void* self, void* renderer);

} // namespace

// 0x886EB0 (thiscall): NiDX9ShaderConstantManager::~NiDX9ShaderConstantManager (its eight aligned
// blocks and four NiMalloc'd ones freed, the device let go, then NiRefObject's)
void __fastcall ShaderConstantManager_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kConstantManagerVtable;
    for (int offset : {0x8, 0xC, 0x10, 0x14, 0x30, 0x34, 0x38, 0x3C}) {
        NiAlignedFree(At<void*>(self, offset));
    }
    for (int offset : {0x58, 0x5C, 0x60, 0x64}) {
        NiFree(At<void*>(self, offset));
    }
    if (At<void*>(self, 0x80) != nullptr) {
        Com<long>(At<void*>(self, 0x80), kRelease);
        At<void*>(self, 0x80) = nullptr;
    }
    At<uint32_t>(self, 0) = kNiRefObjectVtable;
    Decrement(&NiRefObject_ms_uiObjects);
}

// 0x886F90 (thiscall): NiD3DShaderInterface::NiD3DShaderInterface
uint8_t* __fastcall D3DShaderInterface_Construct(uint8_t* self, re::Edx) {
    Shader_Construct(self);
    At<void*>(self, 0x14) = nullptr;
    At<void*>(self, 0x18) = nullptr;
    At<void*>(self, 0x1C) = nullptr;
    At<uint8_t>(self, 0x20) = 0;
    At<uint32_t>(self, 0) = kInterfaceVtable;
    return self;
}

// 0x886FB0 (thiscall): NiD3DShaderInterface::GetRTTI
void* __fastcall D3DShaderInterface_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiD3DShaderInterface;
}

// 0x886FC0 (thiscall): NiD3DShaderInterface::GetIsBestImplementation (4 bytes: not hooked)
uint8_t __fastcall D3DShaderInterface_GetIsBestImplementation(uint8_t* self, re::Edx) {
    return At<uint8_t>(self, 0x20);
}
RE_UNHOOKED_FN("NiD3DShaderInterface::GetIsBestImplementation", 0x00886FC0, &D3DShaderInterface_GetIsBestImplementation);

// 0x886FD0 (thiscall): NiD3DShaderInterface::SetIsBestImplementation
void __fastcall D3DShaderInterface_SetIsBestImplementation(uint8_t* self, re::Edx, int best) {
    At<uint8_t>(self, 0x20) = uint8_t(best);
}

// 0x886FE0 (thiscall): NiD3DShaderInterface::SetD3DRenderer (the renderer, its device held (the
// old one released first), its render state)
void __fastcall D3DShaderInterface_SetD3DRenderer(uint8_t* self, re::Edx, uint8_t* renderer) {
    At<uint8_t*>(self, 0x18) = renderer;
    if (renderer != nullptr) {
        void* device = At<void*>(renderer, kDevice);
        if (At<void*>(self, 0x14) != nullptr) {
            Com<long>(At<void*>(self, 0x14), kRelease);
        }
        At<void*>(self, 0x14) = device;
        if (device != nullptr) {
            Com<long>(device, kAddRef);
        }
        At<void*>(self, 0x1C) = At<void*>(At<uint8_t*>(self, 0x18), kRenderState);
        return;
    }
    void* device = At<void*>(self, 0x14);
    At<void*>(self, 0x1C) = nullptr;
    if (device != nullptr) {
        Com<long>(device, kRelease);
    }
    At<void*>(self, 0x14) = nullptr;
}

// 0x887050: the shader system pointed at a new renderer (the statics, the render state's defaults,
// the shader factory (slot 34), the shader program factory made once). (Name a guess.)
uint32_t __cdecl Renderer_CreateShaderSystem(uint8_t* renderer) {
    void* renderState = At<void*>(renderer, kRenderState);
    Pass_SetD3DRenderer(renderer);
    TextureStage_SetD3DRenderer(renderer);
    ms_pkShaderRenderState = renderState;
    ShaderProgram_SetD3DRenderer(renderer);
    ShaderProgramFactory_SetD3DRenderer(renderer);
    RenderStateGroup_SetD3DRenderer(renderer);
    RenderState_InitRenderState(renderState);
    void* factory = ShaderFactory_GetInstance();
    if (factory != nullptr) {
        Virtual<void>(factory, 34, renderer);
    }
    if (ms_pkShaderProgramFactory == nullptr) {
        void* memory = NiNew(0x20);
        if (memory == nullptr) {
            ms_pkShaderProgramFactory = nullptr;
            NiOutputDebugString("Failed to create shader program factory!");
            return 0;
        }
        ms_pkShaderProgramFactory = ShaderProgramFactory_Construct(memory);
        if (ms_pkShaderProgramFactory == nullptr) {
            NiOutputDebugString("Failed to create shader program factory!");
        }
    }
    return 0;
}

// 0x887100: the shader system let go of the renderer (Name a guess.)
uint32_t __cdecl Renderer_ReleaseShaderSystem() {
    void* factory = ShaderFactory_GetInstance();
    if (factory != nullptr) {
        ShaderFactory_LockLibraries();
        Virtual<void>(factory, 34, static_cast<void*>(nullptr));
        ShaderFactory_UnlockLibraries();
    }
    Pass_SetD3DRenderer(nullptr);
    TextureStage_SetD3DRenderer(nullptr);
    ms_pkShaderRenderState = nullptr;
    ShaderProgram_ReleaseAll();
    ShaderProgram_SetD3DRenderer(nullptr);
    ShaderProgramFactory_SetD3DRenderer(nullptr);
    RenderStateGroup_SetD3DRenderer(nullptr);
    return 0;
}

// 0x887160 (thiscall): NiD3DShaderInterface::~NiD3DShaderInterface
void __fastcall D3DShaderInterface_Destruct(uint8_t* self, re::Edx) {
    void* device = At<void*>(self, 0x14);
    At<uint32_t>(self, 0) = kInterfaceVtable;
    At<void*>(self, 0x18) = nullptr;
    At<void*>(self, 0x1C) = nullptr;
    if (device != nullptr) {
        Com<long>(device, kRelease);
    }
    At<void*>(self, 0x14) = nullptr;
    Shader_Destruct(self);
}

// 0x8871A0 (thiscall, slot 4): NiD3DShaderInterface::Initialize (the renderer set, then NiShader's;
// false without a renderer)
uint8_t __fastcall D3DShaderInterface_Initialize(uint8_t* self, re::Edx) {
    if (ms_pkRenderer == nullptr) {
        return 0;
    }
    D3DShaderInterface_SetD3DRendererAt(self, ms_pkRenderer);
    return Shader_Initialize(self);
}

// 0x8871C0 (thiscall, slot 0): NiD3DShaderInterface's deleting destructor
uint8_t* __fastcall D3DShaderInterface_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    void* device = At<void*>(self, 0x14);
    At<uint32_t>(self, 0) = kInterfaceVtable;
    At<void*>(self, 0x18) = nullptr;
    At<void*>(self, 0x1C) = nullptr;
    if (device != nullptr) {
        Com<long>(device, kRelease);
    }
    At<void*>(self, 0x14) = nullptr;
    Shader_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x24);
    }
    return self;
}

#define C39I_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C39I_REVERSED("NiDX9ShaderConstantManager::~NiDX9ShaderConstantManager", 0x00886EB0, ShaderConstantManager_Destruct);
C39I_REVERSED("NiD3DShaderInterface::NiD3DShaderInterface", 0x00886F90, D3DShaderInterface_Construct);
C39I_REVERSED("NiD3DShaderInterface::GetRTTI", 0x00886FB0, D3DShaderInterface_GetRTTI);
C39I_REVERSED("NiD3DShaderInterface::SetIsBestImplementation", 0x00886FD0, D3DShaderInterface_SetIsBestImplementation);
C39I_REVERSED("NiD3DShaderInterface::SetD3DRenderer", 0x00886FE0, D3DShaderInterface_SetD3DRenderer);
C39I_REVERSED("NiDX9Renderer::CreateShaderSystem", 0x00887050, Renderer_CreateShaderSystem);
C39I_REVERSED("NiDX9Renderer::ReleaseShaderSystem", 0x00887100, Renderer_ReleaseShaderSystem);
C39I_REVERSED("NiD3DShaderInterface::~NiD3DShaderInterface", 0x00887160, D3DShaderInterface_Destruct);
C39I_REVERSED("NiD3DShaderInterface::Initialize", 0x008871A0, D3DShaderInterface_Initialize);
C39I_REVERSED("NiD3DShaderInterface::DeletingDestructor", 0x008871C0, D3DShaderInterface_DeletingDestructor);

} // namespace ni::c39
