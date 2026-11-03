// NiDX9RendererSDM: the DX9 renderer library's static data (its Init and Shutdown handed to
// NiStaticDataManager by the static object's constructor): the display formats the system
// description enumerates, the pools of render state groups, texture stage groups, passes and
// stages, the declaration packing functions, the shader factory, the shader program factory with
// its three creators (HLSL "hlsl", assembly "vsh"/"psh", compiled "vso"/"pso"), NiSCMExtraData's
// statics and the shader constant manager factory; Shutdown undoes them in the opposite order.
//
// The creators (NiD3DShaderProgramCreator, vtable 0x99DB38: the deleting destructor, then
// pure slots): HLSL 0x99DB5C (8 bytes: +4 its creation flags), assembly 0x99DB88 and compiled
// 0x99DBBC (0x10 bytes: +4 the flags, +8 a buffer NiMalloc'd).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::c41 {
using namespace ni::dx9;
namespace {

constexpr uintptr_t kCreatorVtable = 0x0099DB38;      // NiD3DShaderProgramCreator
constexpr uintptr_t kCreatorHLSLVtable = 0x0099DB5C;  // NiD3DShaderProgramCreatorHLSL
constexpr uintptr_t kCreatorAsmVtable = 0x0099DB88;   // NiD3DShaderProgramCreatorAsm
constexpr uintptr_t kCreatorObjVtable = 0x0099DBBC;   // NiD3DShaderProgramCreatorObj
constexpr uintptr_t kInit = 0x0089D8B0;
constexpr uintptr_t kShutdown = 0x0089D920;
// the file extensions they're registered for
constexpr uintptr_t kHLSLExtension = 0x0099D1C8;   // "hlsl"
constexpr uintptr_t kVshExtension = 0x0099DBB8;    // "vsh"
constexpr uintptr_t kPshExtension = 0x0099DBB4;    // "psh"
constexpr uintptr_t kVsoExtension = 0x0099DBEC;    // "vso"
constexpr uintptr_t kPsoExtension = 0x0099DBE8;    // "pso"

GAME_VAR(uint8_t, NiDX9RendererSDM_ms_bInitialized, 0x020E73A8);
GAME_VAR(int32_t, NiDX9RendererSDM_ms_iInitCount, 0x020E73AC);
GAME_VAR(uint8_t*, ms_pkCreatorHLSL, 0x020E7498);
GAME_VAR(uint8_t*, ms_pkCreatorAsm, 0x020E7510);
GAME_VAR(uint8_t*, ms_pkCreatorObj, 0x020E7514);

// the display formats (a NiTPointerList<D3DFORMAT> at 0x0BCAE1C: its allocator's vtable, head,
// tail, count)
inline uint8_t* const kDisplayFormats = reinterpret_cast<uint8_t*>(0x00BCAE1C);

GAME_FN(void, __cdecl, NiStaticDataManager_AddLibrary, 0x00753680, void(__cdecl* init)(), void(__cdecl* shutdown)());
GAME_FN(void, __cdecl, ProgramFactory_RegisterShaderCreator, 0x008A2B90, const char* extension, void* creator);
GAME_FN(void, __cdecl, ProgramFactory_UnregisterShaderCreator, 0x008A2220, void* creator);
GAME_FN(void, __cdecl, ProgramFactory_CreateCreatorMap, 0x008A2B60);
GAME_FN(void, __cdecl, ProgramFactory_DestroyCreatorMap, 0x008A2390);
GAME_FN(void, __cdecl, ProgramFactory_SetD3DRenderer, 0x00889F90, void* renderer);
GAME_FN(void, __cdecl, RenderStateGroup_CreatePools, 0x0089E060);
GAME_FN(void, __cdecl, RenderStateGroup_DestroyPools, 0x0089E3E0);
GAME_FN(void, __cdecl, StageGroup_CreatePool, 0x0089FFE0);
GAME_FN(void, __cdecl, StageGroup_DestroyPool, 0x008A01B0);
GAME_FN(void, __cdecl, Shader_CreatePools, 0x00888770);
GAME_FN(void, __cdecl, Shader_DestroyPools, 0x00888780);
GAME_FN(void, __cdecl, Declaration_InitPackingFunctions, 0x008A1EA0);
GAME_FN(void, __cdecl, Declaration_ShutdownPackingFunctions, 0x008A1CD0);
GAME_FN(void, __cdecl, ShaderFactory_Create, 0x0089D650);
GAME_FN(void, __cdecl, ShaderFactory_Shutdown, 0x0089CA70);
GAME_FN(void, __cdecl, SCMExtraData_InitStatics, 0x00889110);
GAME_FN(void, __cdecl, SCMExtraData_ShutdownStatics, 0x00889160);
GAME_FN(void, __cdecl, RenderState_InitConstantManagerCreator, 0x00882620);
GAME_FN(void, __cdecl, RenderState_ShutdownConstantManagerCreator, 0x00882650);
GAME_FN(void, __cdecl, Renderer_ShutdownStatics, 0x00878DC0);

// this file's (called where modes apply)
GAME_FN(void, __cdecl, Renderer_AddDisplayFormat, 0x0089D790, uint32_t format);
GAME_FN(void, __cdecl, CreatorHLSL_Create, 0x0089D6A0);
GAME_FN(void, __cdecl, CreatorAsm_Create, 0x0089D6F0);
GAME_FN(void, __cdecl, CreatorObj_Create, 0x0089D740);

// a creator made with its vtable and the flags cleared (and the buffer fields, for the 0x10 ones)
inline uint8_t* NewCreator(uint32_t size, uintptr_t vtable) {
    uint8_t* creator = static_cast<uint8_t*>(NiNew(size));
    if (creator == nullptr) {
        return nullptr;
    }
    At<uint32_t>(creator, 0) = vtable;
    At<uint32_t>(creator, 4) = 0;
    if (size == 0x10) {
        At<uint32_t>(creator, 8) = 0;
        At<uint32_t>(creator, 0xC) = 0;
    }
    return creator;
}

// a creator unregistered and deleted (slot 0) at Shutdown
inline void DeleteCreator(re::Ptr32<uint8_t>& creator) {
    if (creator != nullptr) {
        ProgramFactory_UnregisterShaderCreator(creator);
        if (creator != nullptr) {
            Virtual<void>(creator, 0, 1u);
        }
    }
    creator = nullptr;
}

} // namespace

// 0x89D6A0: the HLSL creator made and registered for "hlsl" (null registered when it can't be)
void __cdecl CreatorHLSL_CreateImpl() {
    ms_pkCreatorHLSL = NewCreator(8, kCreatorHLSLVtable);
    ProgramFactory_RegisterShaderCreator(reinterpret_cast<const char*>(kHLSLExtension), ms_pkCreatorHLSL);
}

// 0x89D6F0: the assembly creator made and registered for "vsh" and "psh"
void __cdecl CreatorAsm_CreateImpl() {
    ms_pkCreatorAsm = NewCreator(0x10, kCreatorAsmVtable);
    ProgramFactory_RegisterShaderCreator(reinterpret_cast<const char*>(kVshExtension), ms_pkCreatorAsm);
    ProgramFactory_RegisterShaderCreator(reinterpret_cast<const char*>(kPshExtension), ms_pkCreatorAsm);
}

// 0x89D740: the compiled shader creator made and registered for "vso" and "pso"
void __cdecl CreatorObj_CreateImpl() {
    ms_pkCreatorObj = NewCreator(0x10, kCreatorObjVtable);
    ProgramFactory_RegisterShaderCreator(reinterpret_cast<const char*>(kVsoExtension), ms_pkCreatorObj);
    ProgramFactory_RegisterShaderCreator(reinterpret_cast<const char*>(kPsoExtension), ms_pkCreatorObj);
}

// 0x89D790: a D3DFORMAT added to the display formats unless it's there (a node from the list's
// allocator, slot 1, appended). (Name a guess.)
void __cdecl Renderer_AddDisplayFormatImpl(uint32_t format) {
    for (uint8_t* node = At<uint8_t*>(kDisplayFormats, 4); node != nullptr; node = At<uint8_t*>(node, 0)) {
        if (At<uint32_t>(node, 8) == format) {
            return;
        }
    }
    uint8_t* node = Virtual<uint8_t*>(kDisplayFormats, 1);
    At<uint32_t>(node, 8) = format;
    At<uint8_t*>(node, 0) = nullptr;
    At<uint8_t*>(node, 4) = At<uint8_t*>(kDisplayFormats, 8);
    if (At<uint8_t*>(kDisplayFormats, 8) != nullptr) {
        At<uint8_t*>(At<uint8_t*>(kDisplayFormats, 8), 0) = node;
        At<uint32_t>(kDisplayFormats, 0xC)++;
        At<uint8_t*>(kDisplayFormats, 8) = node;
        return;
    }
    At<uint32_t>(kDisplayFormats, 0xC)++;
    At<uint8_t*>(kDisplayFormats, 4) = node;
    At<uint8_t*>(kDisplayFormats, 8) = node;
}

// 0x89D810 (thiscall): NiD3DShaderProgramCreator's deleting destructor
void* __fastcall Creator_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kCreatorVtable;
    if (flags & 1) {
        NiDelete(self, 4);
    }
    return self;
}

// 0x89D840 (thiscall): NiD3DShaderProgramCreatorHLSL's deleting destructor
void* __fastcall CreatorHLSL_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kCreatorVtable;
    if (flags & 1) {
        NiDelete(self, 8);
    }
    return self;
}

// 0x89D870 (thiscall): NiD3DShaderProgramCreatorAsm's (and Obj's) deleting destructor (its
// buffer freed)
void* __fastcall CreatorAsm_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kCreatorAsmVtable;
    NiFree(At<void*>(self, 8));
    At<uint32_t>(self, 0) = kCreatorVtable;
    if (flags & 1) {
        NiDelete(self, 0x10);
    }
    return self;
}

// 0x89D8B0: NiDX9RendererSDM::Init
void __cdecl NiDX9RendererSDM_Init() {
    if (NiDX9RendererSDM_ms_bInitialized != 0) {
        return;
    }
    NiDX9RendererSDM_ms_bInitialized = 1;
    Renderer_AddDisplayFormat(0x15);   // A8R8G8B8
    Renderer_AddDisplayFormat(0x16);   // X8R8G8B8
    Renderer_AddDisplayFormat(0x19);   // A1R5G5B5
    Renderer_AddDisplayFormat(0x18);   // X1R5G5B5
    Renderer_AddDisplayFormat(0x17);   // R5G6B5
    RenderStateGroup_CreatePools();
    StageGroup_CreatePool();
    Shader_CreatePools();
    Declaration_InitPackingFunctions();
    ShaderFactory_Create();
    ProgramFactory_CreateCreatorMap();
    CreatorAsm_Create();
    CreatorObj_Create();
    CreatorHLSL_Create();
    SCMExtraData_InitStatics();
    RenderState_InitConstantManagerCreator();
}

// 0x89D920: NiDX9RendererSDM::Shutdown
void __cdecl NiDX9RendererSDM_Shutdown() {
    if (NiDX9RendererSDM_ms_bInitialized == 0) {
        return;
    }
    NiDX9RendererSDM_ms_bInitialized = 0;
    RenderState_ShutdownConstantManagerCreator();
    SCMExtraData_ShutdownStatics();
    DeleteCreator(ms_pkCreatorHLSL);
    DeleteCreator(ms_pkCreatorObj);
    DeleteCreator(ms_pkCreatorAsm);
    ProgramFactory_SetD3DRenderer(nullptr);
    ProgramFactory_DestroyCreatorMap();
    ShaderFactory_Shutdown();
    Declaration_ShutdownPackingFunctions();
    Shader_DestroyPools();
    RenderStateGroup_DestroyPools();
    StageGroup_DestroyPool();
    Renderer_ShutdownStatics();
}

// 0x89D9F0 (thiscall): NiDX9RendererSDM::NiDX9RendererSDM (the first one hands Init and Shutdown
// to NiStaticDataManager)
void* __fastcall NiDX9RendererSDM_Construct(void* self, re::Edx) {
    if (NiDX9RendererSDM_ms_iInitCount++ == 0) {
        NiStaticDataManager_AddLibrary(reinterpret_cast<void(__cdecl*)()>(kInit),
                                       reinterpret_cast<void(__cdecl*)()>(kShutdown));
    }
    return self;
}

#define C41C_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C41C_REVERSED("NiD3DShaderProgramCreatorHLSL::Create", 0x0089D6A0, CreatorHLSL_CreateImpl);
C41C_REVERSED("NiD3DShaderProgramCreatorAsm::Create", 0x0089D6F0, CreatorAsm_CreateImpl);
C41C_REVERSED("NiD3DShaderProgramCreatorObj::Create", 0x0089D740, CreatorObj_CreateImpl);
C41C_REVERSED("NiDX9Renderer::AddDisplayFormat", 0x0089D790, Renderer_AddDisplayFormatImpl);
C41C_REVERSED("NiD3DShaderProgramCreator::DeletingDestructor", 0x0089D810, Creator_DeletingDestructor);
C41C_REVERSED("NiD3DShaderProgramCreatorHLSL::DeletingDestructor", 0x0089D840, CreatorHLSL_DeletingDestructor);
C41C_REVERSED("NiD3DShaderProgramCreatorAsm::DeletingDestructor", 0x0089D870, CreatorAsm_DeletingDestructor);
C41C_REVERSED("NiDX9RendererSDM::Init", 0x0089D8B0, NiDX9RendererSDM_Init);
C41C_REVERSED("NiDX9RendererSDM::Shutdown", 0x0089D920, NiDX9RendererSDM_Shutdown);
C41C_REVERSED("NiDX9RendererSDM::NiDX9RendererSDM", 0x0089D9F0, NiDX9RendererSDM_Construct);

} // namespace ni::c41
