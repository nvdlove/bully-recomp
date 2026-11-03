// NiDX9Renderer's life (NiDX9Renderer.h for the layout): Create (0x882140, from NiApplication's
// CreateRenderer and Bully's own), the constructor, Initialize (the device made, trying the next
// device type down to the reference one when it can't be; the default render target group, the
// render state, the shader system, the legacy default material, the buffer / texture / geometry
// group / light managers, the camera, the default material), ResetDevice (everything that lives
// in the default pool let go, IDirect3DDevice9::Reset, all of it made again, the reset
// notifications called before and after), Recreate, LostDeviceRestore, Do_BeginFrame and the
// destructor.
//
// The renderer's members here (by offset, past NiDX9Renderer.h's): +0x42C the precache map,
// +0x43C/+0x440 the batch list (nodes from the pool at 0x20E6D14), +0x44C the batch shader, +0x66C
// the four default texture data, +0x680 the default render target group, +0x684 the current one,
// +0x68C the onscreen render target groups by window (NiTMap<HWND, NiRenderTargetGroupPtr>), +0x6A4
// the screen texture property state, +0x6A8 the geometry group manager (+0x6AC / +0x6B0 its static
// and dynamic groups), +0x6B4 the render state, +0x6B8 the vertex buffer manager, +0x6BC the index
// buffer manager, +0x6C0 the texture manager, +0x6C4 the light manager, +0x6C8 / +0x6D8 / +0x6E8
// the rendered / cube / dynamic textures' data, +0x6F8, +0x6FC and +0x70C two lists (the second
// the D3D shaders), +0x868..+0x89C Create's fourteen arguments, +0x8A0 the legacy default shader,
// +0x8A4 the legacy default material, +0x8A8/+0x8B8 the reset notifications and their data,
// +0x8C8/+0x8D8 the lost device notifications and theirs, +0x8E8 a map (7 buckets).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::c39 {
using namespace ni::dx9;
namespace {

constexpr uintptr_t kNiDX9RendererVtable = 0x0099B5A8;
constexpr uintptr_t kTPointerMapBaseVtable = 0x00950760;   // NiTMapBase<NiTPointerAllocator, ...>
constexpr uintptr_t kTPointerMapVtable = 0x00965A9C;       // NiTPointerMap<...>
constexpr uintptr_t kWindowMapBaseVtable = 0x0099ACD4;
constexpr uintptr_t kWindowMapVtable = 0x00951700;         // NiTMap<HWND, NiRenderTargetGroupPtr>
constexpr uintptr_t kFormatMapVtable = 0x0098AD94;         // (the map at +0x8E8)
constexpr uintptr_t kRefListVtable = 0x0095127C;           // NiTPointerList (references)
constexpr uintptr_t kPointerListVtable = 0x00952E10;
constexpr uintptr_t kListBaseVtable = 0x0099C748;
constexpr uintptr_t kPrimitiveArrayVtable = 0x00961988;    // NiTPrimitiveArray<void*>

GAME_VAR(void*, NiRTTI_NiDX9Renderer, 0x020E6D18);
GAME_VAR(uint8_t*, ms_pkShadowManager, 0x020E2410);   // NiShadowManager's
GAME_VAR(uint8_t*, ms_pkBatchNodeFreeList, 0x020E6D14);
GAME_VAR(uint32_t, ms_uiMaxTextureBlendStages, 0x00BCB7F8);   // NiDX9RenderState
GAME_VAR(uint32_t, NiSingleShaderMaterial_ClassBit, 0x020E38A8);
constexpr uintptr_t NiSingleShaderMaterial_RTTI = 0x020E38A0;
inline void* const NiStandardMaterial_RTTI = reinterpret_cast<void*>(0x020E3528);
GAME_ARRAY(const float, NiPoint3_ZERO, 0x020E2D44);
GAME_ARRAY(const float, NiPoint3_UNIT_X, 0x00BC8A90);
GAME_ARRAY(const float, NiPoint3_UNIT_Y, 0x00BC8A9C);
GAME_ARRAY(const float, NiPoint3_UNIT_Z, 0x00BC8AA8);
constexpr uintptr_t kPersistentSrcTextureCreate = 0x00886790;   // NiDX9PersistentSrcTextureRendererData's

GAME_FN(void*, __thiscall, Renderer_ConstructBase, 0x00755A70, void* self);   // NiRenderer::NiRenderer
GAME_FN(void, __thiscall, Renderer_DestructBase, 0x007558C0, void* self);     // NiRenderer::~NiRenderer
GAME_FN(void*, __thiscall, Frustum_Construct, 0x00771890, void* self, int ortho);
GAME_FN(void, __thiscall, TMapBase_RemoveAll, 0x005AC520, void* map);
GAME_FN(uint8_t*, __cdecl, IsKindOf, 0x00401FF0, void* rtti, void* object);
GAME_FN(uint8_t, __cdecl, IsKindOfRTTI, 0x004066F0, uintptr_t rtti, const void* object);
GAME_FN(void, __thiscall, Map_SetAt, 0x008A5140, void* map, uint32_t key, void* value);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __thiscall, NiPointer_Assign, 0x00800420, re::Ptr32<void>* slot, void* object);
GAME_FN(void, __thiscall, NiPointer_AssignFrom, 0x00518100, re::Ptr32<void>* slot, const re::Ptr32<void>* other);
GAME_FN(void, __cdecl, RemoveLoadFunction, 0x00759EE0, const char* name);   // NiObject's creators
GAME_FN(void, __cdecl, RegisterLoadFunction, 0x00759EC0, const char* name, uintptr_t create);
GAME_FN(void, __cdecl, PersistentSrcTextureRendererData_ResetLoader, 0x007DD9C0);
GAME_FN(void, __cdecl, Material_UnloadShadersForAllMaterials, 0x00762BA0);
// the device set-up (other chunks')
GAME_FN(int, __cdecl, CreateDirect3D9, 0x00878D10);
GAME_FN(void*, __cdecl, GetSystemDesc, 0x0087AC00);
GAME_FN(uint8_t, __cdecl, GetDeviceTypeAndBehavior, 0x00877BF0, uint32_t deviceType, uint32_t* d3dDevType,
        uint32_t* behaviorFlags);
GAME_FN(uint8_t, __thiscall, Renderer_InitializePresentParams, 0x00878E60, void* self, uint32_t window,
        uint32_t width, uint32_t height, uint32_t useFlags, uint32_t mode, uint32_t frameBufferFormat,
        uint32_t depthFormat, uint32_t backBuffers, uint32_t swapEffect, uint32_t refreshRate, uint32_t interval,
        void* params);
GAME_FN(uint8_t, __thiscall, Renderer_InitializeDeviceCaps, 0x00879150, void* self, void* params);
GAME_FN(uint8_t, __thiscall, Renderer_InitializeTextureFormats, 0x0087B280, void* self);
GAME_FN(void, __thiscall, Renderer_InitializeMembers, 0x00878880, void* self);
GAME_FN(void, __thiscall, Renderer_PerformPrecache, 0x0087C3E0, void* self);
GAME_FN(void, __thiscall, Renderer_ClearBatchArraysA, 0x0087B010, void* self);
GAME_FN(void, __thiscall, Renderer_ClearBatchArraysB, 0x0087B080, void* self);
GAME_FN(void, __thiscall, WindowMap_SetAt, 0x0087ADD0, void* map, uint32_t key, void* value);   // (holds value)
GAME_FN(void, __thiscall, WindowMap_GetNext, 0x0087AEC0, void* map, re::Ptr32<void>* position, uint32_t* key,
        re::Ptr32<void>* value);
GAME_FN(uint32_t, __cdecl, GetNiFBFormat, 0x0082C780, uint32_t d3dFormat);
GAME_FN(uint32_t, __cdecl, GetCompatibleAdapterFormat, 0x00878600, uint32_t frameBufferFormat);
// the buffers' renderer data
GAME_FN(void*, __cdecl, OnscreenBufferData_CreateImplicit, 0x008950B0, void* device, void* params,
        re::Ptr32<void>* buffer);   // (returns its renderer data)
GAME_FN(void*, __cdecl, DepthStencilBufferData_CreateImplicit, 0x00895210, void* device, re::Ptr32<void>* buffer);
GAME_FN(void*, __cdecl, RenderTargetGroup_Create, 0x0075D620, uint32_t buffers, void* renderer);
GAME_FN(void, __cdecl, BufferData_ClearRenderTarget, 0x008942C0, void* device, uint32_t index);
GAME_FN(void, __cdecl, BufferData_ReleaseRenderTargets, 0x00894590);
GAME_FN(void, __cdecl, BufferData_RecreateRenderTargets, 0x008945F0, void* device);
// the managers
GAME_FN(void*, __cdecl, RenderState_Create, 0x00883B10, void* renderer, void* caps, int zBuffer);
GAME_FN(void, __thiscall, RenderState_InitRenderState, 0x00883040, void* self);
GAME_FN(void*, __thiscall, VBManager_Construct, 0x00887AA0, void* self, void* device);
GAME_FN(void, __thiscall, VBManager_DestroyDefaultBuffers, 0x00887DD0, void* self);
GAME_FN(void*, __thiscall, IBManager_Construct, 0x00897730, void* self, void* device);
GAME_FN(void, __thiscall, IBManager_DestroyDefaultBuffers, 0x00897A70, void* self);
GAME_FN(void*, __thiscall, TextureManager_Construct, 0x0089A2F0, void* self, void* renderer);
GAME_FN(void*, __cdecl, GeometryGroupManager_Create, 0x00899560, void* device, void* vbManager);
GAME_FN(void, __thiscall, GeometryGroupManager_SetHWSkinning, 0x008992D0, void* self, int hwSkinning);
GAME_FN(void*, __thiscall, LightManager_Construct, 0x008921D0, void* self, void* renderState, void* device);
GAME_FN(void, __thiscall, LightManager_Destruct, 0x00891DB0, void* self);
GAME_FN(void, __thiscall, LightManager_ReleaseLights, 0x00892500, void* self);
GAME_FN(void, __thiscall, LightManager_Reset, 0x00891BD0, void* self);
GAME_FN(void, __thiscall, LightManager_BeginFrame, 0x00891CC0, void* self);
GAME_FN(void, __thiscall, EmptyFunction, 0x008A20A0, void* self, int);   // (a lone ret 4)
// the shader system and the default materials
GAME_FN(void, __cdecl, CreateShaderSystem, 0x00887050, void* renderer);
GAME_FN(void, __cdecl, ReleaseShaderSystem, 0x00887100);
GAME_FN(void*, __cdecl, ShaderFactory_GetInstance, 0x0089C9C0);
GAME_FN(void, __thiscall, ShaderFactory_Initialize, 0x0089D2C0, void* self);
GAME_FN(void*, __cdecl, ShaderLibrary_Get, 0x00756620);
GAME_FN(void*, __thiscall, LegacyDefaultShader_Construct, 0x0089B820, void* self);
GAME_FN(void, __thiscall, LegacyDefaultShader_BeginFrame, 0x0089B120, void* self);
GAME_FN(void*, __cdecl, SingleShaderMaterial_Create, 0x007D36E0, void* shader, int);
GAME_FN(void, __thiscall, SingleShaderMaterial_SetCachedShader, 0x007ABEF0, void* self, void* shader);
GAME_FN(void*, __thiscall, Obj5C_Construct, 0x0089A260, void* self);   // (a 0x5C object given to slot 65)
// NiDX9TextureData::ClearTextureData (every texture's renderer data deleted; the renderer isn't read)
GAME_FN(void, __cdecl, TextureData_ClearTextureData, 0x008930B0, void* renderer);
// NiShadowManager's
GAME_FN(void, __cdecl, ShadowManager_ValidateShaderVersions, 0x007574B0, uint32_t vsMajor, uint32_t vsMinor,
        uint32_t gsMajor, uint32_t gsMinor, uint32_t psMajor, uint32_t psMinor);
GAME_FN(void, __cdecl, ShadowManager_ReinitializeShadowTextures, 0x00757410);

// this file's (called where modes apply)
GAME_FN(uint8_t, __thiscall, Renderer_ResetDevice, 0x008806C0, void* self);
GAME_FN(void*, __thiscall, Renderer_Construct, 0x0087FAA0, void* self);
GAME_FN(uint8_t, __thiscall, Renderer_Initialize, 0x0087FE00, void* self, uint32_t width, uint32_t height,
        uint32_t useFlags, uint32_t window, uint32_t focusWindow, uint32_t adapter, uint32_t deviceType,
        uint32_t frameBufferFormat, uint32_t depthFormat, uint32_t interval, uint32_t swapEffect, uint32_t mode,
        uint32_t backBuffers, uint32_t refreshRate);
GAME_FN(void, __thiscall, Renderer_Destruct, 0x008817A0, void* self);
GAME_FN(uint8_t, __thiscall, Renderer_LostDeviceRestore, 0x00881680, void* self);
GAME_FN(void, __thiscall, Renderer_PurgeAllD3DShaders, 0x00881610, void* self);

using PresentParams = uint32_t[14];   // D3DPRESENT_PARAMETERS (0x38): +0xC BackBufferCount

// NiTPointerMap's construction as the constructors inline it (base vtable, hash size, count, the
// buckets NiMalloc'd and zeroed, then the class's vtable)
inline void ConstructMap(uint8_t* map, uintptr_t baseVtable, uint32_t hashSize, uintptr_t vtable) {
    At<uint32_t>(map, 0) = baseVtable;
    At<uint32_t>(map, 4) = hashSize;
    At<uint32_t>(map, 0xC) = 0;
    void* table = NiMalloc(hashSize * 4);
    At<void*>(map, 8) = table;
    crt::Memset(table, 0, At<uint32_t>(map, 4) * 4);
    At<uint32_t>(map, 0) = vtable;
}

// and its destruction (RemoveAll as each class, the buckets freed)
inline void DestructMap(uint8_t* map, uintptr_t vtable, uintptr_t baseVtable) {
    At<uint32_t>(map, 0) = vtable;
    TMapBase_RemoveAll(map);
    At<uint32_t>(map, 0) = baseVtable;
    TMapBase_RemoveAll(map);
    NiFree(At<void*>(map, 8));
}

// a NiTPrimitiveArray: vtable, no elements, growing by 1
inline void ConstructArray(uint8_t* array) {
    At<uint16_t>(array, 8) = 0;
    At<uint16_t>(array, 0xE) = 1;
    At<uint16_t>(array, 0xA) = 0;
    At<uint16_t>(array, 0xC) = 0;
    At<void*>(array, 4) = nullptr;
    At<uint32_t>(array, 0) = kPrimitiveArrayVtable;
}

// a NiTPointerList emptied (the allocator's slot 2 frees each node) and given its base vtable
inline void DestructList(uint8_t* list, uintptr_t vtable) {
    At<uint32_t>(list, 0) = vtable;
    uint8_t* node = At<uint8_t*>(list, 4);
    while (node != nullptr) {
        uint8_t* next = At<uint8_t*>(node, 0);
        Virtual<void>(list, 2, node);
        node = next;
    }
    At<uint32_t>(list, 0xC) = 0;
    At<void*>(list, 4) = nullptr;
    At<void*>(list, 8) = nullptr;
    At<uint32_t>(list, 0) = kListBaseVtable;
}

// the first item of a pointer map (NiTMapBase::GetFirstPos) and the one after `item`
inline uint8_t* MapFirst(const uint8_t* map) {
    uint32_t size = At<uint32_t>(map, 4);
    for (uint32_t i = 0; i < size; i++) {
        if (At<re::Ptr32<uint8_t>*>(map, 8)[i] != nullptr) {
            return At<re::Ptr32<uint8_t>*>(map, 8)[i];
        }
    }
    return nullptr;
}
inline uint8_t* MapNext(const uint8_t* map, const uint8_t* item) {
    uint8_t* next = At<uint8_t*>(item, 0);
    if (next != nullptr) {
        return next;
    }
    uint32_t size = At<uint32_t>(map, 4);
    for (uint32_t i = At<uint32_t>(item, 4) % size + 1; i < size; i++) {
        if (At<re::Ptr32<uint8_t>*>(map, 8)[i] != nullptr) {
            return At<re::Ptr32<uint8_t>*>(map, 8)[i];
        }
    }
    return nullptr;
}

// the renderer data of a render target group's buffer 0 as an onscreen buffer's (its +0x18 the
// present parameters); null when it isn't one
inline uint8_t* OnscreenData(void* group) {
    uint8_t* buffer = Virtual<uint8_t*>(group, 27, 0u);   // GetBuffer
    uint8_t* data = At<uint8_t*>(buffer, 0x10);
    if (data == nullptr) {
        return nullptr;
    }
    return HasClassBit(data, NiDX9OnscreenBufferData_ClassBit) ? data : nullptr;
}

// the reset / lost device notifications: the arrays at +0x8A8 (functions) and +0x8B8 (data),
// +0x8C8 and +0x8D8
using ResetNotify = uint8_t(__cdecl*)(int beforeReset, void* data);
using LostNotify = uint8_t(__cdecl*)(void* data);

} // namespace

// 0x87FAA0 (thiscall): NiDX9Renderer::NiDX9Renderer (NiRenderer's constructor, then the maps,
// lists and arrays empty, the frustum and port zero; InitializeMembers for the rest)
uint8_t* __fastcall Renderer_ConstructImpl(uint8_t* self, re::Edx) {
    Renderer_ConstructBase(self);
    At<uint32_t>(self, 0) = kNiDX9RendererVtable;
    ConstructMap(self + 0x42C, kTPointerMapBaseVtable, 0x25, kTPointerMapVtable);
    At<void*>(self, 0x44C) = nullptr;
    for (int offset = 0x66C; offset <= 0x678; offset += 4) {
        At<void*>(self, offset) = nullptr;
    }
    At<void*>(self, 0x680) = nullptr;
    ConstructMap(self + 0x68C, kWindowMapBaseVtable, 0x25, kWindowMapVtable);
    At<void*>(self, 0x6A4) = nullptr;
    ConstructMap(self + 0x6C8, kTPointerMapBaseVtable, 0x25, kTPointerMapVtable);
    ConstructMap(self + 0x6D8, kTPointerMapBaseVtable, 0x25, kTPointerMapVtable);
    ConstructMap(self + 0x6E8, kTPointerMapBaseVtable, 0x25, kTPointerMapVtable);
    At<void*>(self, 0x6F8) = nullptr;
    At<uint32_t>(self, 0x708) = 0;
    At<void*>(self, 0x700) = nullptr;
    At<void*>(self, 0x704) = nullptr;
    At<uint32_t>(self, 0x6FC) = kRefListVtable;
    At<uint32_t>(self, 0x718) = 0;
    At<void*>(self, 0x710) = nullptr;
    At<void*>(self, 0x714) = nullptr;
    At<uint32_t>(self, 0x70C) = kPointerListVtable;
    for (int offset = 0x71C; offset <= 0x740; offset += 4) {
        At<uint32_t>(self, offset) = 0;
    }
    At<void*>(self, 0x8A0) = nullptr;
    At<void*>(self, 0x8A4) = nullptr;
    ConstructArray(self + 0x8A8);
    ConstructArray(self + 0x8B8);
    ConstructArray(self + 0x8C8);
    ConstructArray(self + 0x8D8);
    ConstructMap(self + 0x8E8, kTPointerMapBaseVtable, 7, kFormatMapVtable);
    Frustum_Construct(self + kFrustum, 0);
    At<float>(self, kPort) = 0.0f;
    At<float>(self, kPort + 4) = 0.0f;
    At<float>(self, kPort + 8) = 0.0f;
    At<float>(self, kPort + 0xC) = 0.0f;
    Renderer_InitializeMembers(self);
    return self;
}

// 0x87FDF0 (thiscall): NiDX9Renderer::GetRTTI
void* __fastcall Renderer_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiDX9Renderer;
}

// 0x87FE00 (thiscall): NiDX9Renderer::Initialize (Create's arguments kept at +0x868; the system
// description fetched outside the D3D lock; the adapter and device descriptions; the device made,
// again with the back buffer count Direct3D corrected, else with the next device type until the
// reference one; then everything that hangs off the device)
uint8_t __fastcall Renderer_InitializeImpl(uint8_t* self, re::Edx, uint32_t width, uint32_t height, uint32_t useFlags,
                                           uint32_t window, uint32_t focusWindow, uint32_t adapter,
                                           uint32_t deviceType, uint32_t frameBufferFormat, uint32_t depthFormat,
                                           uint32_t interval, uint32_t swapEffect, uint32_t mode,
                                           uint32_t backBuffers, uint32_t refreshRate) {
    At<uint32_t>(self, 0x868) = width;
    At<uint32_t>(self, 0x86C) = height;
    At<uint32_t>(self, 0x870) = useFlags;
    At<uint32_t>(self, 0x880) = deviceType;
    At<uint32_t>(self, 0x884) = frameBufferFormat;
    At<uint32_t>(self, 0x888) = depthFormat;
    At<uint32_t>(self, 0x88C) = interval;
    At<uint32_t>(self, 0x890) = swapEffect;
    At<uint32_t>(self, 0x874) = window;
    At<uint32_t>(self, 0x878) = focusWindow;
    At<uint32_t>(self, 0x87C) = adapter;
    At<uint32_t>(self, 0x894) = mode;
    At<uint32_t>(self, 0x898) = backBuffers;
    At<uint32_t>(self, 0x89C) = refreshRate;
    Unlock(ms_kD3DCriticalSection);
    GetSystemDesc();
    Lock(ms_kD3DCriticalSection);
    At<uint32_t>(self, kAdapter) = adapter;
    At<uint32_t>(self, kDeviceWindow) = window;
    At<uint32_t>(self, kFocusWindow) = focusWindow;
    uint32_t& devType = At<uint32_t>(self, kDevType);
    uint32_t& behavior = At<uint32_t>(self, kBehaviorFlags);
    if (!GetDeviceTypeAndBehavior(deviceType, &devType, &behavior)) {
        crt::StrncpyS(ms_acErrorString, 0x100, "Creation failed: Invalid 3D device type", 0xFF);
        NiOutputDebugString("NiDX9Renderer::Initialize> Invalid device ID specified - FAILING\n");
        return 0;
    }
    if (useFlags & 0x40) {   // USE_MULTITHREADED
        behavior |= 2;       // D3DCREATE_MULTITHREADED
    }
    if (useFlags & 0x20) {   // USE_FPU_PRESERVE
        behavior |= 4;       // D3DCREATE_FPU_PRESERVE
    }
    // the system description: +0x8 its adapters, +0xE their count; an adapter's +0x460 / +0x464
    // its HAL / reference device descriptions (+0x4: valid)
    auto adapterDesc = [&]() -> uint8_t* {
        uint8_t* system = static_cast<uint8_t*>(ms_pkSystemDesc);
        uint32_t index = At<uint32_t>(self, kAdapter);
        return index < At<uint16_t>(system, 0xE) ? At<re::Ptr32<uint8_t>*>(system, 8)[index] : nullptr;
    };
    uint8_t* adapterDescription = adapterDesc();
    At<uint8_t*>(self, kAdapterDesc) = adapterDescription;
    if (adapterDescription == nullptr) {
        crt::StrncpyS(ms_acErrorString, 0x100, "Creation failed: Invalid Adapter", 0xFF);
        NiOutputDebugString("NiDX9Renderer::Initialize> Invalid Adapter specified - FAILING\n");
        return 0;
    }
    uint8_t* deviceDesc = At<uint8_t*>(adapterDescription, devType == 1 ? 0x460 : 0x464);
    deviceDesc = At<uint32_t>(deviceDesc, 4) != 0 ? deviceDesc : nullptr;
    At<uint8_t*>(self, kDeviceDesc) = deviceDesc;
    if (deviceDesc == nullptr) {
        crt::StrncpyS(ms_acErrorString, 0x100, "Creation failed: Invalid 3D device type", 0xFF);
        NiOutputDebugString("NiDX9Renderer::Initialize> Invalid device ID specified - FAILING\n");
        return 0;
    }
    PresentParams params;
    crt::Memset(params, 0, sizeof(params));
    re::Ptr32<void>* device = &At<void*>(self, kDevice);
    for (;;) {
        NiOutputDebugString("NiDX9Renderer::Initialize> Attempting to create rendering device\n");
        uint8_t* desc = At<uint8_t*>(adapterDesc(), devType == 1 ? 0x460 : 0x464);
        bool created = false;
        if (At<uint32_t>(desc, 4) == 0 || desc == nullptr) {
            crt::StrncpyS(ms_acErrorString, 0x100, "Creation failed: Requested device not valid", 0xFF);
            NiOutputDebugString("NiDX9Renderer::Initialize> No device of specified type \n");
        } else if (Renderer_InitializePresentParams(self, At<uint32_t>(self, kDeviceWindow), width, height, useFlags,
                                                    mode, frameBufferFormat, depthFormat, backBuffers, swapEffect,
                                                    refreshRate, interval, params)) {
            backBuffers = params[3];
            void* d3d = ms_pkD3D9;
            if (CreateDevice9(d3d, At<uint32_t>(self, kAdapter), devType, At<uint32_t>(self, kFocusWindow), behavior,
                              static_cast<void*>(params), device) >= 0) {
                created = true;
            } else {
                At<uint8_t>(self, kDeviceLost) = 0;
                if (backBuffers != params[3]) {
                    NiOutputDebugString(
                        "NiDX9Renderer::Initialize> Specified number of backbuffers was invalid - trying again\n");
                    if (CreateDevice9(ms_pkD3D9, At<uint32_t>(self, kAdapter), devType,
                                      At<uint32_t>(self, kFocusWindow), behavior, static_cast<void*>(params),
                                      device) >= 0) {
                        created = true;
                    }
                }
            }
        }
        if (created) {
            break;
        }
        if (deviceType == 4) {   // DEVDESC_REF
            NiRenderer_SetLastErrorString("Creation failed: Could not create reference device");
            NiOutputDebugString("NiDX9Renderer::Initialize> Could not create requested reference device - FAILING\n");
            return 0;
        }
        if (++deviceType == 4) {
            NiOutputDebugString("NiDX9Renderer::Initialize> Could not create hardware device of any type - FAILING\n");
            return 0;
        }
        NiOutputDebugString("NiDX9Renderer::Initialize> Could not create device of desired type - trying "
                            "next available type\n");
        if (!GetDeviceTypeAndBehavior(deviceType, &devType, &behavior)) {
            NiRenderer_SetLastErrorString("Creation failed: Invalid 3D device type");
            NiOutputDebugString("NiDX9Renderer::Initialize> Invalid device ID specified - FAILING\n");
            return 0;
        }
    }
    if (*device == nullptr) {
        NiRenderer_SetLastErrorString("Creation failed: Could not create hardware device");
        NiOutputDebugString("NiDX9Renderer::Initialize> Could not create device of any type - FAILING\n");
        return 0;
    }
    if (!Renderer_InitializeDeviceCaps(self, params)) {
        return 0;
    }
    At<uint8_t>(self, kSWVertexSwitchable) = uint8_t((behavior >> 7) & 1);   // MIXED_VERTEXPROCESSING
    At<uint8_t>(self, kSWVertexCapable) = uint8_t(~(behavior >> 6) & 1);     // not HARDWARE_VERTEXPROCESSING
    // the default render target group: the implicit back buffer and depth buffer
    re::Ptr32<void> backBuffer = nullptr;
    re::Ptr32<void> depthBuffer = nullptr;
    uint8_t* onscreenData = static_cast<uint8_t*>(OnscreenBufferData_CreateImplicit(*device, params, &backBuffer));
    DepthStencilBufferData_CreateImplicit(*device, &depthBuffer);
    re::Ptr32<void>* defaultGroup = &At<void*>(self, kDefaultRenderTargetGroup);
    NiPointer_Assign(defaultGroup, RenderTargetGroup_Create(1, self));
    Virtual<void>(*defaultGroup, 25, backBuffer, 0u);   // AttachBuffer
    Virtual<void>(*defaultGroup, 26, depthBuffer);      // AttachDepthStencilBuffer
    AddRefRef(*defaultGroup);   // (SetAt's NiPointer argument, which it lets go)
    WindowMap_SetAt(self + 0x68C, window, *defaultGroup);
    uint8_t* format = Virtual<uint8_t*>(*defaultGroup, 22, 0u);   // the buffer's pixel format
    At<uint32_t>(self, kAdapterFormat) = GetCompatibleAdapterFormat(GetNiFBFormat(At<uint32_t>(format, 0xC)));
    if (!Renderer_InitializeTextureFormats(self)) {
        NiOutputDebugString("NiDX9Renderer::Initialize> Could not initialize texture defaults - FAILING\n");
        return 0;
    }
    At<void*>(self, kRenderState) = RenderState_Create(self, self + kCaps, 1);
    if (ms_pkShadowManager != nullptr) {
        ShadowManager_ValidateShaderVersions(At<uint8_t>(self, kVertexShaderVersion + 1),
                                             At<uint8_t>(self, kVertexShaderVersion), 0, 0,
                                             At<uint8_t>(self, kPixelShaderVersion + 1),
                                             At<uint8_t>(self, kPixelShaderVersion));
    }
    CreateShaderSystem(self);
    ShaderFactory_Initialize(ShaderFactory_GetInstance());
    // the legacy pipeline's default shader and the material made from it
    void* memory = NiNew(0xE4);
    re::Ptr32<void>* legacyShader = &At<void*>(self, 0x8A0);
    NiPointer_Assign(legacyShader, memory != nullptr ? LegacyDefaultShader_Construct(memory) : nullptr);
    Virtual<void>(*legacyShader, 4);   // Initialize
    re::Ptr32<void>* legacyMaterial = &At<void*>(self, kLegacyDefaultMaterial);
    NiPointer_Assign(legacyMaterial, SingleShaderMaterial_Create(*legacyShader, 1));
    if (*legacyMaterial != nullptr && IsKindOfRTTI(NiSingleShaderMaterial_RTTI, *legacyMaterial)) {
        SingleShaderMaterial_SetCachedShader(*legacyMaterial, *legacyShader);
    }
    memory = NiNew(0x70);
    At<void*>(self, kVBManager) = memory != nullptr ? VBManager_Construct(memory, *device) : nullptr;
    memory = NiNew(0x4C);
    At<void*>(self, kIBManager) = memory != nullptr ? IBManager_Construct(memory, *device) : nullptr;
    memory = NiNew(0x10);
    At<void*>(self, kTextureManager) = memory != nullptr ? TextureManager_Construct(memory, self) : nullptr;
    void* groupManager = GeometryGroupManager_Create(*device, At<void*>(self, kVBManager));
    At<void*>(self, kManager6A8) = groupManager;
    GeometryGroupManager_SetHWSkinning(groupManager, (Virtual<uint32_t>(self, 19) >> 1) & 0xFFFFFF01);
    At<void*>(self, 0x6AC) = Virtual<void*>(At<void*>(self, kManager6A8), 1, 1);   // CreateGroup(STATIC)
    At<void*>(self, 0x6B0) = Virtual<void*>(At<void*>(self, kManager6A8), 1, 2);   // CreateGroup(DYNAMIC)
    memory = NiNew(0x240);
    At<void*>(self, kLightManager) =
        memory != nullptr ? LightManager_Construct(memory, At<void*>(self, kRenderState), *device) : nullptr;
    if (At<uint32_t>(onscreenData, 0x28) != 0) {
        EmptyFunction(At<void*>(self, kRenderState), 1);
    }
    // the camera: at the origin looking down x, a frustum of -1..1 by -1..1, near 0.1, far 1
    At<void*>(self, kCurrentRenderTargetGroup) = *defaultGroup;
    struct {
        float left, right, top, bottom, nearPlane, farPlane;
        uint8_t ortho;
        uint8_t pad[3];
    } frustum;
    Frustum_Construct(&frustum, 0);
    frustum.bottom = -1.0f;
    frustum.left = -1.0f;
    frustum.top = 1.0f;
    frustum.right = 1.0f;
    frustum.nearPlane = 0.1f;
    frustum.farPlane = 1.0f;
    float port[4];   // left, right, top, bottom
    port[0] = 0.0f;
    port[3] = 0.0f;
    port[1] = 1.0f;
    port[2] = 1.0f;
    Virtual<void>(self, 86, NiPoint3_ZERO, NiPoint3_UNIT_X, NiPoint3_UNIT_Y, NiPoint3_UNIT_Z, &frustum,
                  port);   // Do_SetCameraData
    At<void*>(self, kCurrentRenderTargetGroup) = nullptr;
    At<void*>(self, kCurrentOnscreenTarget) = nullptr;
    // the default material: the standard one when the shader versions allow it (with the shadow
    // manager's materials given the program cache), else the legacy one
    void* library = ShaderLibrary_Get();
    if (At<uint32_t>(self, kPixelShaderVersion) < Virtual<uint32_t>(library, 24, 2, 0) ||
        At<uint32_t>(self, kVertexShaderVersion) < Virtual<uint32_t>(library, 18, 2, 0)) {
        NiPointer_AssignFrom(&At<void*>(self, 0x14), legacyMaterial);
    } else {
        void* standard = IsKindOf(NiStandardMaterial_RTTI, At<void*>(self, 0x14));
        if (standard != nullptr) {
            Virtual<void>(self, 77, standard);   // SetDefaultProgramCache
        }
        if (ms_pkShadowManager != nullptr) {
            if (At<void*>(ms_pkShadowManager, 0x50) != nullptr) {
                Virtual<void>(self, 77, At<void*>(ms_pkShadowManager, 0x50));
            }
            if (At<void*>(ms_pkShadowManager, 0x54) != nullptr) {
                Virtual<void>(self, 77, At<void*>(ms_pkShadowManager, 0x54));
            }
            if (At<void*>(ms_pkShadowManager, 0x58) != nullptr) {
                Virtual<void>(self, 77, At<void*>(ms_pkShadowManager, 0x58));
            }
        }
    }
    NiPointer_AssignFrom(&At<void*>(self, 0x18), &At<void*>(self, 0x14));
    memory = NiNew(0x5C);
    void* object = memory != nullptr ? Obj5C_Construct(memory) : nullptr;
    Virtual<void>(object, 4);
    Virtual<void>(self, 65, object);
    return 1;
}

// 0x8806C0 (thiscall): NiDX9Renderer::ResetDevice (the managers' default-pool resources and the
// render targets let go, the onscreen buffers' and depth buffers' data told (slot 5), rendered
// textures purged, dynamic textures' data deleted, the D3D shaders told (slot 28), the reset
// notifications called (true); IDirect3DDevice9::Reset with the default group's present
// parameters; then the reset count, the buffers' data made again (slot 6), the texture formats,
// the textures' data, the shaders (slot 29), the render state, the lights, the notifications
// (false), the shadow textures). (A group whose buffer data isn't onscreen data crashes it, as in
// the game.)
uint8_t __fastcall Renderer_ResetDeviceImpl(uint8_t* self, re::Edx) {
    LightManager_ReleaseLights(At<void*>(self, kLightManager));
    IBManager_DestroyDefaultBuffers(At<void*>(self, kIBManager));
    VBManager_DestroyDefaultBuffers(At<void*>(self, kVBManager));
    BufferData_ClearRenderTarget(At<void*>(self, kDevice), 1);
    BufferData_ClearRenderTarget(At<void*>(self, kDevice), 2);
    BufferData_ClearRenderTarget(At<void*>(self, kDevice), 3);
    uint8_t* windows = self + 0x68C;
    re::Ptr32<void> position = MapFirst(windows);
    while (position != nullptr) {
        uint32_t key;
        re::Ptr32<void> group = nullptr;
        WindowMap_GetNext(windows, &position, &key, &group);
        Virtual<void>(OnscreenData(group), 5);
        uint8_t* depth = Virtual<uint8_t*>(group, 28);   // GetDepthStencilBuffer
        if (depth != nullptr && At<void*>(depth, 0x10) != nullptr &&
            HasClassBit(At<void*>(depth, 0x10), NiDX9DepthStencilBufferData_ClassBit)) {
            Virtual<void>(At<void*>(depth, 0x10), 5);
        }
        ReleaseRef(group);
    }
    for (uint8_t* item = MapFirst(self + 0x6C8); item != nullptr;) {
        uint32_t key = At<uint32_t>(item, 4);
        item = MapNext(self + 0x6C8, item);
        Virtual<void>(self, 54, key);   // PurgeTexture
        Map_SetAt(self + 0x6C8, key, nullptr);
    }
    for (uint8_t* item = MapFirst(self + 0x6D8); item != nullptr;) {
        uint32_t key = At<uint32_t>(item, 4);
        item = MapNext(self + 0x6D8, item);
        Virtual<void>(self, 54, key);
        Map_SetAt(self + 0x6D8, key, nullptr);
    }
    for (uint8_t* item = MapFirst(self + 0x6E8); item != nullptr;) {
        uint8_t* texture = At<uint8_t*>(item, 4);
        void* data = At<void*>(item, 8);
        item = MapNext(self + 0x6E8, item);
        At<void*>(texture, 0x24) = nullptr;
        if (data != nullptr) {
            Virtual<void>(data, 0, 1);   // deleted
        }
        Map_SetAt(self + 0x6E8, reinterpret_cast<uint32_t>(texture), nullptr);
    }
    BufferData_ReleaseRenderTargets();
    for (uint8_t* node = At<uint8_t*>(self, 0x710); node != nullptr;) {
        void* shader = At<void*>(node, 8);
        node = At<uint8_t*>(node, 0);
        Virtual<void>(shader, 28);
    }
    Virtual<void>(At<void*>(self, 0x6B0), 0);
    uint32_t notifications = At<uint16_t>(self, 0x8B2);
    for (uint32_t i = 0; i < notifications; i++) {
        ResetNotify notify = re::GameFnAt<ResetNotify>(&At<re::Ptr32<void>*>(self, 0x8AC)[i]);
        void* data = At<re::Ptr32<void>*>(self, 0x8BC)[i];
        if (notify != nullptr && !notify(1, data)) {
            NiOutputDebugString(
                "NiDX9Renderer::Recreate> Reset notification function failed before device reset - FAILING\n");
            return 0;
        }
    }
    uint8_t* defaultData = OnscreenData(At<void*>(self, kDefaultRenderTargetGroup));
    if (ResetDevice9(At<void*>(self, kDevice), defaultData + 0x18) < 0) {
        return 0;
    }
    At<uint32_t>(self, kResetCounter)++;
    At<uint8_t>(self, kDeviceLost) = 0;
    position = MapFirst(windows);
    while (position != nullptr) {
        uint32_t key;
        re::Ptr32<void> group = nullptr;
        WindowMap_GetNext(windows, &position, &key, &group);
        Virtual<void>(OnscreenData(group), 6, At<void*>(self, kDevice));
        uint8_t* depthData = Virtual<uint8_t*>(group, 32);   // the depth buffer's renderer data
        if (depthData != nullptr && HasClassBit(depthData, NiDX9DepthStencilBufferData_ClassBit)) {
            Virtual<void>(depthData, 6, At<void*>(self, kDevice));
        }
        ReleaseRef(group);
    }
    if (!Renderer_InitializeTextureFormats(self)) {
        NiOutputDebugString("NiDX9Renderer::Recreate> Could not initialize texture defaults - FAILING\n");
        return 0;
    }
    for (uint8_t* item = MapFirst(self + 0x6C8); item != nullptr;) {
        uint32_t key = At<uint32_t>(item, 4);
        item = MapNext(self + 0x6C8, item);
        Virtual<void>(self, 68, key, 0x100u);   // CreateRenderedTextureRendererData
    }
    for (uint8_t* item = MapFirst(self + 0x6D8); item != nullptr;) {
        uint32_t key = At<uint32_t>(item, 4);
        item = MapNext(self + 0x6D8, item);
        Virtual<void>(self, 70, key);   // CreateRenderedCubeMapRendererData
    }
    for (uint8_t* item = MapFirst(self + 0x6E8); item != nullptr;) {
        uint32_t key = At<uint32_t>(item, 4);
        item = MapNext(self + 0x6E8, item);
        Virtual<void>(self, 71, key);   // CreateDynamicTextureRendererData
    }
    BufferData_RecreateRenderTargets(At<void*>(self, kDevice));
    for (uint8_t* node = At<uint8_t*>(self, 0x710); node != nullptr;) {
        void* shader = At<void*>(node, 8);
        node = At<uint8_t*>(node, 0);
        Virtual<void>(shader, 29);
    }
    RenderState_InitRenderState(At<void*>(self, kRenderState));
    LightManager_Reset(At<void*>(self, kLightManager));
    for (uint32_t i = 0; i < notifications; i++) {
        ResetNotify notify = re::GameFnAt<ResetNotify>(&At<re::Ptr32<void>*>(self, 0x8AC)[i]);
        void* data = At<re::Ptr32<void>*>(self, 0x8BC)[i];
        if (notify != nullptr && !notify(0, data)) {
            NiOutputDebugString(
                "NiDX9Renderer::Recreate> Reset notification function failed after device reset - FAILING\n");
            return 0;
        }
    }
    if (ms_pkShadowManager != nullptr) {
        ShadowManager_ReinitializeShadowTextures();
    }
    return 1;
}

// 0x880E60 (thiscall): NiDX9Renderer::Recreate (new present parameters into the default group's
// onscreen data, the old ones kept; the device reset with them, else with the old ones again:
// 2 done, 1 back as it was, 0 failed)
uint32_t __fastcall Renderer_Recreate(uint8_t* self, re::Edx, uint32_t width, uint32_t height, uint32_t useFlags,
                                      uint32_t window, uint32_t frameBufferFormat, uint32_t depthFormat,
                                      uint32_t interval, uint32_t swapEffect, uint32_t mode, uint32_t backBuffers,
                                      uint32_t refreshRate) {
    uint8_t* params = OnscreenData(At<void*>(self, kDefaultRenderTargetGroup)) + 0x18;
    PresentParams saved;
    crt::MemcpyS(saved, sizeof(saved), params, 0x38);
    if (!Renderer_InitializePresentParams(self, window, width, height, useFlags, mode, frameBufferFormat, depthFormat,
                                          backBuffers, swapEffect, refreshRate, interval, params)) {
        crt::MemcpyS(params, 0x38, saved, sizeof(saved));
        return 0;
    }
    if (window != 0) {
        At<uint32_t>(params, 0x1C) = window;   // hDeviceWindow
    }
    if (Renderer_ResetDevice(self)) {
        return 2;
    }
    NiOutputDebugString("NiDX9Renderer::Recreate> Could not reset device to desired parameters - trying to "
                        "recover original values\n");
    crt::MemcpyS(params, 0x38, saved, sizeof(saved));
    if (!Renderer_ResetDevice(self)) {
        NiOutputDebugString("NiDX9Renderer::Recreate> Could not recover original presentation parameters\n");
        return 0;
    }
    return 1;
}

// 0x881680 (thiscall): NiDX9Renderer::LostDeviceRestore (TestCooperativeLevel: reset when it
// can be; when lost, the lost device notifications called once; true when the device is fine)
uint8_t __fastcall Renderer_LostDeviceRestoreImpl(uint8_t* self, re::Edx) {
    long hr = Com<long>(At<void*>(self, kDevice), kDevTestCooperativeLevel);
    if (hr == long(0x88760869)) {   // D3DERR_DEVICENOTRESET
        return Renderer_ResetDevice(self);
    }
    if (hr != long(0x88760868)) {   // D3DERR_DEVICELOST
        return 1;
    }
    if (At<uint8_t>(self, kDeviceLost) == 0) {
        uint32_t count = At<uint16_t>(self, 0x8D2);
        At<uint8_t>(self, kDeviceLost) = 1;
        for (uint32_t i = 0; i < count; i++) {
            LostNotify notify = re::GameFnAt<LostNotify>(&At<re::Ptr32<void>*>(self, 0x8CC)[i]);
            void* data = At<re::Ptr32<void>*>(self, 0x8DC)[i];
            if (notify != nullptr && !notify(data)) {
                NiOutputDebugString("NiDX9Renderer::LostDeviceRestore> Lost device notification function failed\n");
                return 0;
            }
        }
    }
    return 0;
}

// 0x8817A0 (thiscall): NiDX9Renderer::~NiDX9Renderer (the device's textures, streams and indices
// unset; the format tables and default textures let go; the batch list back to its pool and the
// pool freed; the precache entries deleted; the default materials' cached shaders cleared; the
// shaders purged; the shader system and render targets released; the textures' maps emptied;
// the onscreen groups' buffer data let go; the managers deleted; the device released; then the
// members and NiRenderer's destructor)
void __fastcall Renderer_DestructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kNiDX9RendererVtable;
    if (At<void*>(self, kDevice) != nullptr) {
        if (At<void*>(self, kRenderState) != nullptr) {
            for (uint32_t i = 0; i < ms_uiMaxTextureBlendStages; i++) {
                uint8_t* state = At<uint8_t*>(self, kRenderState);
                if (At<void*>(state, 0x20A0 + int(i) * 4) != nullptr) {
                    At<void*>(state, 0x20A0 + int(i) * 4) = nullptr;
                    Com<long>(At<void*>(state, 0x2238), 65, i, static_cast<void*>(nullptr));   // SetTexture
                }
            }
        }
        for (uint32_t i = 0; i < At<uint32_t>(self, kMaxStreams); i++) {
            Com<long>(At<void*>(self, kDevice), 100, i, static_cast<void*>(nullptr), 0u, 0u);   // SetStreamSource
        }
        Com<long>(At<void*>(self, kDevice), 104, static_cast<void*>(nullptr));   // SetIndices
    }
    for (int usage = 0; usage < 4; usage++) {
        for (int format = 0; format < 22; format++) {
            NiDelete(At<void*>(self, kTextureFormats + (usage * 22 + format) * 4), 0x44);
        }
        re::Ptr32<void>& texture = At<void*>(self, kDefaultTextureData + usage * 4);
        if (texture != nullptr) {
            ReleaseRef(texture);
            texture = nullptr;
        }
    }
    // the batch list's nodes back to the pool, then the pool freed
    for (uint8_t* node = At<uint8_t*>(self, 0x43C); node != nullptr;) {
        uint8_t* next = At<uint8_t*>(node, 0xC);
        At<uint8_t*>(node, 0xC) = ms_pkBatchNodeFreeList;
        ms_pkBatchNodeFreeList = node;
        node = next;
    }
    At<void*>(self, 0x43C) = nullptr;
    At<void*>(self, 0x440) = nullptr;
    for (uint8_t* node = ms_pkBatchNodeFreeList; node != nullptr;) {
        uint8_t* next = At<uint8_t*>(node, 0xC);
        NiDelete(node, 0x10);
        node = next;
    }
    ms_pkBatchNodeFreeList = nullptr;
    // the precache entries deleted (each item's chain; the map updated as each goes)
    uint8_t* precache = self + 0x42C;
    for (uint8_t* item = MapFirst(precache); item != nullptr;) {
        uint32_t key = At<uint32_t>(item, 4);
        uint8_t* entry = At<uint8_t*>(item, 8);
        item = MapNext(precache, item);
        while (entry != nullptr) {
            uint8_t* next = At<uint8_t*>(entry, 0x20);
            if (next != nullptr) {
                Map_SetAt(precache, key, next);
            } else {
                // NiTPointerMap::RemoveAt, inlined
                re::Ptr32<uint8_t>* bucket = &At<re::Ptr32<uint8_t>*>(precache, 8)[key % At<uint32_t>(precache, 4)];
                uint8_t* found = *bucket;
                if (found != nullptr) {
                    if (key == At<uint32_t>(found, 4)) {
                        *bucket = At<uint8_t*>(found, 0);
                        Virtual<void>(precache, 2, found);   // ClearValue
                        Virtual<void>(precache, 4, found);   // DeleteItem
                        At<uint32_t>(precache, 0xC)--;
                    } else {
                        uint8_t* previous = found;
                        for (found = At<uint8_t*>(found, 0); found != nullptr; found = At<uint8_t*>(found, 0)) {
                            if (key == At<uint32_t>(found, 4)) {
                                At<uint8_t*>(previous, 0) = At<uint8_t*>(found, 0);
                                Virtual<void>(precache, 2, found);
                                Virtual<void>(precache, 4, found);
                                At<uint32_t>(precache, 0xC)--;
                                break;
                            }
                            previous = found;
                        }
                    }
                }
            }
            NiDelete(entry, 0x24);
            entry = next;
        }
    }
    Material_UnloadShadersForAllMaterials();
    void* legacy = At<void*>(self, kLegacyDefaultMaterial);
    if (legacy != nullptr && HasClassBit(legacy, NiSingleShaderMaterial_ClassBit)) {
        SingleShaderMaterial_SetCachedShader(At<void*>(self, kLegacyDefaultMaterial), nullptr);
    }
    if (At<void*>(self, 0x14) != nullptr && HasClassBit(At<void*>(self, 0x14), NiSingleShaderMaterial_ClassBit)) {
        SingleShaderMaterial_SetCachedShader(At<void*>(self, 0x14), nullptr);
    }
    if (At<void*>(self, 0x18) != nullptr && HasClassBit(At<void*>(self, 0x18), NiSingleShaderMaterial_ClassBit)) {
        SingleShaderMaterial_SetCachedShader(At<void*>(self, 0x18), nullptr);
    }
    Renderer_PurgeAllD3DShaders(self);
    ReleaseShaderSystem();
    BufferData_ReleaseRenderTargets();
    TMapBase_RemoveAll(self + 0x6C8);
    TMapBase_RemoveAll(self + 0x6D8);
    TMapBase_RemoveAll(self + 0x6E8);
    TextureData_ClearTextureData(self);
    // the onscreen groups' buffers lose their renderer data
    uint8_t* windows = self + 0x68C;
    re::Ptr32<void> position = MapFirst(windows);
    while (position != nullptr) {
        uint32_t key;
        re::Ptr32<void> group = nullptr;
        WindowMap_GetNext(windows, &position, &key, &group);
        if (group == nullptr) {
            continue;
        }
        for (uint32_t i = 0; i < Virtual<uint32_t>(group, 24); i++) {   // GetBufferCount
            uint8_t* buffer = Virtual<uint8_t*>(group, 27, i);
            if (buffer != nullptr && At<void*>(buffer, 0x10) != nullptr) {
                ReleaseRef(At<void*>(buffer, 0x10));
                At<void*>(buffer, 0x10) = nullptr;
            }
        }
        uint8_t* depth = Virtual<uint8_t*>(group, 28);
        if (depth != nullptr && At<void*>(depth, 0x10) != nullptr) {
            ReleaseRef(At<void*>(depth, 0x10));
            At<void*>(depth, 0x10) = nullptr;
        }
        ReleaseRef(group);
    }
    TMapBase_RemoveAll(windows);
    // the map at +0x8E8's values deleted
    uint8_t* formats = self + 0x8E8;
    for (uint8_t* item = MapFirst(formats); item != nullptr;) {
        void* value = At<void*>(item, 8);
        item = MapNext(formats, item);
        NiDelete(value, 0x44);
    }
    if (At<void*>(self, 0x6A4) != nullptr) {
        ReleaseRef(At<void*>(self, 0x6A4));
        At<void*>(self, 0x6A4) = nullptr;
    }
    if (At<void*>(self, kVBManager) != nullptr) {
        Virtual<void>(At<void*>(self, kVBManager), 0, 1);
    }
    At<void*>(self, kVBManager) = nullptr;
    if (At<void*>(self, kIBManager) != nullptr) {
        Virtual<void>(At<void*>(self, kIBManager), 0, 1);
        At<void*>(self, kIBManager) = nullptr;
    }
    if (At<void*>(self, kManager6A8) != nullptr) {
        Virtual<void>(At<void*>(self, kManager6A8), 2, At<void*>(self, 0x6AC));   // DestroyGroup
        Virtual<void>(At<void*>(self, kManager6A8), 2, At<void*>(self, 0x6B0));
        if (At<void*>(self, kManager6A8) != nullptr) {
            Virtual<void>(At<void*>(self, kManager6A8), 0, 1);
        }
        At<void*>(self, kManager6A8) = nullptr;
    }
    if (At<void*>(self, kRenderState) != nullptr) {
        Virtual<void>(At<void*>(self, kRenderState), 0, 1);
    }
    At<void*>(self, kRenderState) = nullptr;
    if (At<void*>(self, kTextureManager) != nullptr) {
        Virtual<void>(At<void*>(self, kTextureManager), 0, 1);
    }
    At<void*>(self, kTextureManager) = nullptr;
    void* lightManager = At<void*>(self, kLightManager);
    if (lightManager != nullptr) {
        LightManager_Destruct(lightManager);
        NiDelete(lightManager, 0x240);
    }
    At<void*>(self, kLightManager) = nullptr;
    if (At<void*>(self, kDevice) != nullptr) {
        Com<long>(At<void*>(self, kDevice), kRelease);
        At<void*>(self, kDevice) = nullptr;
    }
    NiDeleteArray(At<void*>(self, 0x850), 8);
    NiDeleteArray(At<void*>(self, 0x854), 0x10);
    NiDeleteArray(At<void*>(self, 0x858), 8);
    NiFree(At<void*>(self, 0x860));
    Renderer_ClearBatchArraysA(self);
    Renderer_ClearBatchArraysB(self);
    RemoveLoadFunction("NiPersistentSrcTextureRendererData");
    PersistentSrcTextureRendererData_ResetLoader();
    // the members' destructors
    DestructMap(self + 0x8E8, kFormatMapVtable, kTPointerMapBaseVtable);
    At<uint32_t>(self, 0x8D8) = kPrimitiveArrayVtable;
    NiFree(At<void*>(self, 0x8DC));
    At<uint32_t>(self, 0x8C8) = kPrimitiveArrayVtable;
    NiFree(At<void*>(self, 0x8CC));
    At<uint32_t>(self, 0x8B8) = kPrimitiveArrayVtable;
    NiFree(At<void*>(self, 0x8BC));
    At<uint32_t>(self, 0x8A8) = kPrimitiveArrayVtable;
    NiFree(At<void*>(self, 0x8AC));
    ReleaseRef(At<void*>(self, 0x8A4));
    ReleaseRef(At<void*>(self, 0x8A0));
    DestructList(self + 0x70C, kPointerListVtable);
    DestructList(self + 0x6FC, kRefListVtable);
    ReleaseRef(At<void*>(self, 0x6F8));
    DestructMap(self + 0x6E8, kTPointerMapVtable, kTPointerMapBaseVtable);
    DestructMap(self + 0x6D8, kTPointerMapVtable, kTPointerMapBaseVtable);
    DestructMap(self + 0x6C8, kTPointerMapVtable, kTPointerMapBaseVtable);
    ReleaseRef(At<void*>(self, 0x6A4));
    DestructMap(windows, kWindowMapVtable, kWindowMapBaseVtable);
    ReleaseRef(At<void*>(self, 0x680));
    for (int offset = 0x678; offset >= 0x66C; offset -= 4) {
        ReleaseRef(At<void*>(self, offset));
    }
    ReleaseRef(At<void*>(self, 0x44C));
    DestructMap(precache, kTPointerMapVtable, kTPointerMapBaseVtable);
    Renderer_DestructBase(self);
}

// 0x882140: NiDX9Renderer::Create (Direct3D made; a renderer constructed and initialized under the
// D3D lock, deleted again when that fails; NiPersistentSrcTextureRendererData's creator set)
void* __cdecl Renderer_Create(uint32_t width, uint32_t height, uint32_t useFlags, uint32_t window,
                              uint32_t focusWindow, uint32_t adapter, uint32_t deviceType, uint32_t frameBufferFormat,
                              uint32_t depthFormat, uint32_t interval, uint32_t swapEffect, uint32_t mode,
                              uint32_t backBuffers, uint32_t refreshRate) {
    if (CreateDirect3D9() == -1) {
        crt::StrncpyS(ms_acErrorString, 0x100, "Creation failed: Could not initialize DirectX9", 0xFF);
        NiOutputDebugString("NiDX9Renderer::Create> Invalid device ID specified - FAILING\n");
        return nullptr;
    }
    void* memory = NiNew(kRendererSize);
    void* renderer = memory != nullptr ? Renderer_Construct(memory) : nullptr;
    Lock(ms_kD3DCriticalSection);
    uint8_t ok = Renderer_Initialize(renderer, width, height, useFlags, window, focusWindow, adapter, deviceType,
                                     frameBufferFormat, depthFormat, interval, swapEffect, mode, backBuffers,
                                     refreshRate);
    Unlock(ms_kD3DCriticalSection);
    if (!ok) {
        if (renderer != nullptr) {
            Virtual<void>(renderer, 0, 1);
        }
        return nullptr;
    }
    RemoveLoadFunction("NiPersistentSrcTextureRendererData");
    RegisterLoadFunction("NiPersistentSrcTextureRendererData", kPersistentSrcTextureCreate);
    return renderer;
}

// 0x882260 (thiscall, slot 82): NiDX9Renderer::Do_BeginFrame (the device restored if lost; the
// precache done; the legacy shader's and lights' frame begun; BeginScene; the list at +0x6FC
// emptied, its references let go). True unless BeginScene fails.
uint8_t __fastcall Renderer_Do_BeginFrame(uint8_t* self, re::Edx) {
    if (!Renderer_LostDeviceRestore(self)) {
        return 1;
    }
    Renderer_PerformPrecache(self);
    LegacyDefaultShader_BeginFrame(At<void*>(self, 0x8A0));
    LightManager_BeginFrame(At<void*>(self, kLightManager));
    if (Com<long>(At<void*>(self, kDevice), 41) < 0) {   // BeginScene
        return 0;
    }
    uint8_t* list = self + 0x6FC;
    while (At<uint32_t>(self, 0x708) != 0) {
        uint8_t* node = At<uint8_t*>(list, 4);
        uint8_t* next = At<uint8_t*>(node, 0);
        At<uint8_t*>(list, 4) = next;
        if (next != nullptr) {
            At<uint8_t*>(next, 4) = nullptr;
        } else {
            At<uint8_t*>(list, 8) = nullptr;
        }
        void* element = At<void*>(node, 8);
        AddRefRef(element);
        Virtual<void>(list, 2, node);
        At<uint32_t>(list, 0xC)--;
        ReleaseRef(element);
    }
    return 1;
}

// 0x8824F0 (thiscall): NiDX9Renderer's deleting destructor
uint8_t* __fastcall Renderer_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    Renderer_Destruct(self);
    if (flags & 1) {
        NiDelete(self, kRendererSize);
    }
    return self;
}

#define C39B_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C39B_REVERSED("NiDX9Renderer::NiDX9Renderer", 0x0087FAA0, Renderer_ConstructImpl);
C39B_REVERSED("NiDX9Renderer::GetRTTI", 0x0087FDF0, Renderer_GetRTTI);
C39B_REVERSED("NiDX9Renderer::Initialize", 0x0087FE00, Renderer_InitializeImpl);
C39B_REVERSED("NiDX9Renderer::ResetDevice", 0x008806C0, Renderer_ResetDeviceImpl);
C39B_REVERSED("NiDX9Renderer::Recreate", 0x00880E60, Renderer_Recreate);
C39B_REVERSED("NiDX9Renderer::LostDeviceRestore", 0x00881680, Renderer_LostDeviceRestoreImpl);
C39B_REVERSED("NiDX9Renderer::~NiDX9Renderer", 0x008817A0, Renderer_DestructImpl);
C39B_REVERSED("NiDX9Renderer::Create", 0x00882140, Renderer_Create);
C39B_REVERSED("NiDX9Renderer::Do_BeginFrame", 0x00882260, Renderer_Do_BeginFrame);
C39B_REVERSED("NiDX9Renderer::DeletingDestructor", 0x008824F0, Renderer_DeletingDestructor);

} // namespace ni::c39
