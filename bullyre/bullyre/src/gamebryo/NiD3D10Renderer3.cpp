// NiD3D10Renderer (src/gamebryo/NiD3D10Renderer.h): its creation (Create, the constructor,
// Initialize with the device, managers, default swap chain and render target group, the default
// materials and states), its destruction, the skinning scratch buffers and the bone matrices.

#include "gamebryo/NiD3D10Renderer.h"
#include "reversed.h"

namespace ni::c42 {
namespace {

using namespace ni::d3d10;

constexpr uintptr_t kRefObjectBaseListVtable = 0x0099C748;   // (NiTPointerList's allocator base)
constexpr uintptr_t kBuffersListVtable = 0x0095127C;
constexpr uintptr_t kPrecacheListVtable = 0x00952E10;
constexpr uintptr_t kPrecacheMapBaseVtable = 0x0099ACD4;
constexpr uintptr_t kPrecacheMapVtable = 0x00951700;
constexpr uintptr_t kPrimitiveArrayVtable = 0x00961988;
constexpr uintptr_t NiSingleShaderMaterial_RTTI = 0x020E3528;
constexpr long E_INVALIDARG_ = long(0x80070057);

// IDXGIFactory 8 MakeWindowAssociation
constexpr int kMakeWindowAssociation = 8;

GAME_VAR(uint32_t, NiMaterialClassBit_20E38A8, 0x020E38A8);
GAME_VAR(uint8_t*, ms_pkShadowManager, 0x020E2410);   // (+0x50 / +0x54 / +0x58 its materials)

GAME_FN(void*, __cdecl, DynamicCast, 0x00401FF0, uintptr_t rtti, void* object);
GAME_FN(void, __thiscall, NiPointer_Construct, 0x00401B10, re::Ptr32<void>* self, void* object);
GAME_FN(void, __thiscall, NiPointer_AssignPointer, 0x00518100, re::Ptr32<void>* self, const re::Ptr32<void>* other);
GAME_FN(void, __thiscall, NiPointer_Assign, 0x00800420, re::Ptr32<void>* slot, void* object);
GAME_FN(void, __thiscall, NiTMap_RemoveAll, 0x005AC520, void* map);
GAME_FN(void*, __cdecl, GameMalloc, 0x005EEAA0, uint32_t size);
GAME_FN(void*, __cdecl, AlignedMalloc, 0x00752340, uint32_t size, uint32_t alignment);
GAME_FN(void, __cdecl, AlignedFree, 0x007523A0, void* p);
GAME_FN(void*, __cdecl, NiMalloc_7524D0, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiFree_752570, 0x00752570, void* p, uint32_t size);
GAME_FN(void, __thiscall, NiRenderer_Construct, 0x00755A70, void* self);
GAME_FN(void, __thiscall, NiRenderer_Destruct, 0x007558C0, void* self);
GAME_FN(void, __thiscall, NiRenderer_PrecacheMaterial, 0x00755740, void* self, void* material);
GAME_FN(void*, __cdecl, MaterialLibrary_GetInstance, 0x00756620);
GAME_FN(void, __cdecl, ShaderNames_Report, 0x007574B0, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e,
        uint32_t f);
GAME_FN(void, __cdecl, ShaderLibraries_Shutdown, 0x00762BA0);
GAME_FN(void, __thiscall, NiTransform_Invert, 0x00770950, const void* self, void* out);
GAME_FN(void*, __thiscall, NiTransform_Multiply, 0x00406830, const void* self, void* out, const void* other);
GAME_FN(void, __thiscall, NiFrustum_Construct, 0x007718A0, void* self, float left, float right, float top,
        float bottom, float nearPlane, float farPlane, int ortho);
GAME_FN(void, __thiscall, Material_SetOwner, 0x007ABEF0, void* material, void* owner);
GAME_FN(void, __thiscall, PrecacheMap_GetNext, 0x0087AEC0, void* map, re::Ptr32<void>* position, re::Ptr32<void>* key, re::Ptr32<void>* value);
GAME_FN(void, __thiscall, PrecacheMap_SetAt, 0x0087ADD0, void* map, void* key, void* value);
GAME_FN(void, __cdecl, TextureData_ClearTextureData, 0x008930B0);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, int count, void* constructor);
// the D3D10 renderer's other parts (not this chunk's)
GAME_FN(void, __cdecl, SetModelTransform_8B60A0, 0x008B60A0, void* matrix, const void* transform);
GAME_FN(void, __cdecl, SetModelTransformTransposed_8B60C0, 0x008B60C0, void* matrix, const void* transform);
GAME_FN(void*, __cdecl, ShaderFactory_GetInstance, 0x008B94D0);
GAME_FN(void, __thiscall, ShaderFactory_Initialize, 0x008BA360, void* self);
GAME_FN(void*, __thiscall, DefaultMaterial_Construct, 0x008BAB00, void* self);
// this chunk's
GAME_FN(uint8_t*, __thiscall, CreationParameters_Construct, 0x008A8E60, void* self);
GAME_FN(void, __cdecl, GetSystemDesc, 0x008A8370, re::Ptr32<void>* result);
GAME_FN(uint8_t, __thiscall, SystemDesc_IsEnumerationValid, 0x008A7890, void* self);
GAME_FN(uint8_t, __thiscall, SystemDesc_Enumerate, 0x008A8350, void* self);
GAME_FN(uint32_t, __thiscall, SystemDesc_GetAdapterCount, 0x008A7A50, void* self);
GAME_FN(uint8_t*, __thiscall, SystemDesc_GetAdapterDesc, 0x008A7A60, void* self, uint32_t index);
GAME_FN(void*, __thiscall, SystemDesc_GetFactory, 0x007E2BE0, void* self);
GAME_FN(void*, __thiscall, AdapterDesc_GetAdapter, 0x0066B5D0, void* self);   // (+0x4)
GAME_FN(uint8_t, __cdecl, D3D10_EnsureLibrary, 0x008AD700);
GAME_FN(void, __thiscall, Renderer_CheckFormatSupport, 0x008A8F80, void* self);
GAME_FN(void, __thiscall, Renderer_CreateManagers, 0x008A9000, void* self);
GAME_FN(void*, __thiscall, Renderer_CreateSwapChain, 0x008A96C0, void* self, const void* description, uint32_t output);
GAME_FN(void*, __thiscall, Renderer_CreateRenderTargetGroupFromSwapChain, 0x008AB620, void* self, void* swapChain,
        int withDepthStencil, uint32_t format);
GAME_FN(uint8_t, __thiscall, Renderer_CreateDevice, 0x008AD760, void* self, uint32_t* parameters);
GAME_FN(void, __thiscall, Renderer_PurgePrecacheList, 0x008AD9E0, void* self);
GAME_FN(uint8_t*, __thiscall, Renderer_Construct, 0x008AD280, void* self);
GAME_FN(void, __thiscall, Renderer_Destruct, 0x008AED50, void* self);
GAME_FN(uint8_t, __thiscall, Renderer_Initialize, 0x008AF1C0, void* self, uint32_t* parameters);

inline uint8_t* Device(uint8_t* self) {
    return At<uint8_t*>(self, kDevice);
}

// an NiTPrimitiveArray's constructor (growth 1) and destructor, as the renderer inlines them
inline void ConstructArray(uint8_t* array) {
    At<uint16_t>(array, 0xE) = 1;
    At<uint16_t>(array, 8) = 0;
    At<uint16_t>(array, 0xA) = 0;
    At<uint16_t>(array, 0xC) = 0;
    At<void*>(array, 4) = nullptr;
    At<uint32_t>(array, 0) = kPrimitiveArrayVtable;
}
inline void DestructArray(uint8_t* array) {
    At<uint32_t>(array, 0) = kPrimitiveArrayVtable;
    NiFree(At<void*>(array, 4));
}

// an NiTPointerList's destructor (its nodes freed through its allocator's slot 2)
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
    At<uint32_t>(list, 0) = kRefObjectBaseListVtable;
}

// a buffer's renderer data reference (+0x10) let go
inline void ReleaseBufferData(uint8_t* buffer) {
    if (buffer != nullptr) {
        re::Ptr32<void>& data = At<void*>(buffer, 0x10);
        if (data != nullptr) {
            ReleaseRef(data);
            data = nullptr;
        }
    }
}

inline void SetIdentity(float* m) {
    for (int i = 0; i < 16; i++) {
        m[i] = (i % 5) == 0 ? 1.0f : 0.0f;
    }
}

} // namespace

// 0x8AD280 (thiscall): NiD3D10Renderer::NiD3D10Renderer
uint8_t* __fastcall NiD3D10Renderer_Construct(uint8_t* self, re::Edx) {
    NiRenderer_Construct(self);
    At<uint32_t>(self, 0) = kVtable;
    CreationParameters_Construct(self + kCreationParameters);
    At<void*>(self, kSystemDesc) = nullptr;
    At<void*>(self, kDevice) = nullptr;
    At<uint32_t>(self, kAdapterIndex) = 0xFFFFFFFF;
    At<uint8_t>(self, kInitialized) = 0;
    for (int offset = kDefaultStates; offset <= kCurrentRenderTargetGroup; offset += 4) {
        At<uint32_t>(self, offset) = 0;
    }
    At<void*>(self, kBuffersInUse + 0xC) = nullptr;
    At<void*>(self, kBuffersInUse + 4) = nullptr;
    At<void*>(self, kBuffersInUse + 8) = nullptr;
    At<uint32_t>(self, kBuffersInUse) = kBuffersListVtable;
    At<float>(self, kDepthClear) = 1.0f;
    At<uint8_t>(self, kStencilClear) = 0;
    At<uint8_t>(self, kLeftRightSwap) = 0;
    NiFrustum_Construct(self + kFrustum, -1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 0);
    At<float>(self, kPort) = 0.0f;
    At<float>(self, kPort + 4) = 1.0f;
    At<float>(self, kPort + 8) = 1.0f;
    At<float>(self, kPort + 0xC) = 0.0f;
    uint8_t* map = self + kPrecacheMap;
    At<uint32_t>(map, 0) = kPrecacheMapBaseVtable;
    At<uint32_t>(map, 4) = 0x25;
    At<uint32_t>(map, 0xC) = 0;
    void* buckets = NiMalloc(0x94);
    At<void*>(map, 8) = buckets;
    crt::Memset(buckets, 0, At<uint32_t>(map, 4) * 4);
    At<uint32_t>(map, 0) = kPrecacheMapVtable;
    At<float>(self, kMaxFogFactor) = 0.0f;
    At<float>(self, kMaxFogValue) = 1.0f;
    At<uint32_t>(self, kPrecacheList + 0xC) = 0;
    At<void*>(self, kPrecacheList + 4) = nullptr;
    At<void*>(self, kPrecacheList + 8) = nullptr;
    At<uint32_t>(self, kPrecacheList) = kPrecacheListVtable;
    for (int offset = 0x300; offset <= 0x330; offset += 4) {
        At<uint32_t>(self, offset) = 0;
    }
    At<uint32_t>(self, 0x440) = 1;
    for (int offset = 0x444; offset <= 0x470; offset += 4) {
        At<uint32_t>(self, offset) = 0;
    }
    At<uint8_t>(self, kOccluded) = 0;
    At<uint8_t>(self, kDeviceRemoved) = 0;
    ConstructArray(self + kOcclusionCallbacks);
    ConstructArray(self + kOcclusionCallbacks + 0x10);
    ConstructArray(self + kDeviceRemovedCallbacks);
    ConstructArray(self + kDeviceRemovedCallbacks + 0x10);
    for (int i = 0; i < 4; i++) {
        At<float>(self, kBackground + i * 4) = 0.0f;
    }
    crt::Memset(self + kFormatSupport, 0, 0x164);
    SetIdentity(&At<float>(self, kView));
    SetIdentity(&At<float>(self, kInvView));
    SetIdentity(&At<float>(self, kProjection));
    SetIdentity(&At<float>(self, kWorld));
    return self;
}

// 0x8AD1B0 (thiscall): NiD3D10Renderer::EnsureBoneScratchBuffers(count) (grown to the count: 12,
// 16 (zeroed four-float vectors) and 8 bytes for each)
void __fastcall NiD3D10Renderer_EnsureBoneScratchBuffers(uint8_t* self, re::Edx, uint32_t count) {
    if (count <= At<uint32_t>(self, 0x464)) {
        return;
    }
    auto bytes = [](uint32_t n, uint32_t size) {
        uint64_t product = uint64_t(n) * size;
        return product > 0xFFFFFFFFull ? 0xFFFFFFFFu : uint32_t(product);
    };
    NiFree_752570(At<void*>(self, 0x458), 0xC);
    At<void*>(self, 0x458) = NiMalloc_7524D0(bytes(count, 0xC));
    NiFree_752570(At<void*>(self, 0x460), 8);
    At<void*>(self, 0x460) = NiMalloc_7524D0(bytes(count, 8));
    NiFree_752570(At<void*>(self, 0x45C), 0x10);
    void* vectors = NiMalloc_7524D0(bytes(count, 0x10));
    if (vectors != nullptr) {
        VectorConstruct(vectors, 0x10, int(count), reinterpret_cast<void*>(0x005EFB40));   // (NiPoint4's)
    }
    At<void*>(self, 0x45C) = vectors;
    At<uint32_t>(self, 0x464) = count;
}

// 0x8AD760 (thiscall): NiD3D10Renderer::CreateDevice(parameters) (the system enumerated; the
// adapter asked for, else the reference device; quirk kept: when the library can't be loaded its
// lock is never left)
uint8_t __fastcall NiD3D10Renderer_CreateDeviceFromParameters(uint8_t* self, re::Edx, uint32_t* parameters) {
    crt::Memcpy(self + kCreationParameters, parameters, 0x58);
    re::Ptr32<void>* systemDesc = &At<void*>(self, kSystemDesc);
    GetSystemDesc(systemDesc);
    if (SystemDesc_IsEnumerationValid(*systemDesc) == 0 && SystemDesc_Enumerate(*systemDesc) == 0) {
        Warning(Text(0x0099FE08));
        return 0;
    }
    uint32_t adapterIndex = parameters[0];
    if (adapterIndex == 0xFFFFFFFF) {
        parameters[2] = 1;   // D3D10_DRIVER_TYPE_REFERENCE
    } else if (adapterIndex >= SystemDesc_GetAdapterCount(*systemDesc)) {
        adapterIndex = 0;
    }
    void* adapter = nullptr;
    if (parameters[2] == 1) {
        adapterIndex = 0xFFFFFFFF;
    } else {
        adapter = AdapterDesc_GetAdapter(SystemDesc_GetAdapterDesc(*systemDesc, adapterIndex));
    }
    void* device = nullptr;
    sys::Lock(ms_kD3D10LibraryLock);
    if (D3D10_EnsureLibrary() == 0) {
        Warning(Text(0x0099FD9C));
        return 0;
    }
    long hr;
    if (ms_hD3D10 == nullptr) {
        hr = E_INVALIDARG_;
    } else {
        hr = re::PtrCast<long(__stdcall*)(void*, uint32_t, void*, uint32_t, uint32_t, void**)>(
            ms_apfnD3D10[kD3D10CreateDevice])(adapter, parameters[2], nullptr, parameters[3], 0x1D, &device);
    }
    sys::Unlock(ms_kD3D10LibraryLock);
    if (hr >= 0) {
        if (device != nullptr) {
            At<void*>(self, kDevice) = device;
            At<uint32_t>(self, kAdapterIndex) = adapterIndex;
            Renderer_CheckFormatSupport(self);
            return 1;
        }
        Error(4, Text(0x0099FDD4));
    } else {
        Error(4, Text(0x0099E584), hr);
    }
    if (device != nullptr) {
        ComRelease(device);
    }
    return 0;
}

// 0x8ACEF0 (thiscall): NiD3D10Renderer::CalculateBoneMatrices(skin instance, world transform,
// transposed, registers per bone, world space) (once a frame: each bone's transform into the
// skin instance's buffer, 3 registers (3x4, transposed) or 4; the root's in its own matrix in
// world space)
void __fastcall NiD3D10Renderer_CalculateBoneMatrices(uint8_t* self, re::Edx, uint8_t* skin, const uint8_t* world,
                                                      int transposed, uint32_t registers, int worldSpace) {
    uint32_t frame = At<uint32_t>(self, 0x9C);
    if (At<uint32_t>(skin, 0x18) == frame && registers == At<uint32_t>(skin, 0x20)) {
        return;
    }
    auto buffer = At<uint8_t*>(skin, 0x28);
    At<uint32_t>(skin, 0x18) = frame;
    uint8_t* data = At<uint8_t*>(skin, 8);
    uint32_t bones = At<uint32_t>(data, 0x44);
    bool transpose = re::Flag(transposed);
    if (registers == 3) {
        transpose = true;
    }
    uint32_t size = bones * registers * 16;
    if (size > At<uint32_t>(skin, 0x24)) {
        AlignedFree(buffer);
        buffer = static_cast<uint8_t*>(AlignedMalloc(size, 0x10));
        At<uint8_t*>(skin, 0x28) = buffer;
        At<uint32_t>(skin, 0x24) = size;
    }
    At<uint32_t>(skin, 0x1C) = bones;
    At<uint32_t>(skin, 0x20) = registers;
    if (registers == 4) {
        for (uint32_t i = 0; i < bones; i++) {
            float* m = reinterpret_cast<float*>(buffer + i * 0x40);
            if (transpose) {
                m[13] = 0.0f, m[12] = 0.0f, m[14] = 0.0f;
            } else {
                m[11] = 0.0f, m[7] = 0.0f, m[3] = 0.0f;
            }
            m[15] = 1.0f;
        }
    }
    uint8_t rootInverse[0x34];
    NiTransform_Invert(At<uint8_t*>(skin, 0x10) + 0x60, rootInverse);
    uint8_t temp[0x34];
    uint8_t skinToWorld[0x34];
    crt::Memcpy(skinToWorld, NiTransform_Multiply(data + 0xC, temp, rootInverse), 0x34);
    uint8_t toWorld[0x34];
    crt::Memcpy(toWorld, NiTransform_Multiply(world, temp, skinToWorld), 0x34);
    if (re::Flag(worldSpace)) {
        uint8_t* rootMatrix = At<uint8_t*>(skin, 0x2C);
        if (rootMatrix == nullptr) {
            auto* memory = static_cast<uint8_t*>(GameMalloc(0x50));
            if (memory != nullptr) {
                uint8_t offset = uint8_t(0x10 - (uintptr_t(memory) & 0xF));
                memory += offset;
                memory[-1] = offset;
            }
            rootMatrix = memory;
            At<uint8_t*>(skin, 0x2C) = rootMatrix;
            At<float>(rootMatrix, 0xC) = 0.0f;
            At<float>(rootMatrix, 0x1C) = 0.0f;
            At<float>(rootMatrix, 0x2C) = 0.0f;
            At<float>(rootMatrix, 0x3C) = 1.0f;
        }
        SetModelTransform_8B60A0(rootMatrix, toWorld);
    }
    re::Ptr32<uint8_t>* boneObjects = At<re::Ptr32<uint8_t>*>(skin, 0x14);
    uint8_t* boneData = At<uint8_t*>(data, 0x40);
    uint8_t* matrix = buffer;
    for (uint32_t i = 0; i < bones; i++, boneData += 0x4C, matrix += 0x40) {
        uint8_t transform[0x34];
        NiTransform_Multiply(boneObjects[i] + 0x60, transform, boneData);
        if (!re::Flag(worldSpace)) {
            crt::Memcpy(transform, NiTransform_Multiply(toWorld, temp, transform), 0x34);
        }
        if (registers == 4) {
            if (transpose) {
                SetModelTransformTransposed_8B60C0(matrix, transform);
            } else {
                SetModelTransform_8B60A0(matrix, transform);
            }
            continue;
        }
        const auto* t = reinterpret_cast<const float*>(transform);   // rotate[3][3], translate, scale
        float scale = t[12];
        auto* out = reinterpret_cast<float*>(buffer + i * 0x30);
        out[0] = t[0] * scale;
        out[4] = t[3] * scale;
        out[8] = t[6] * scale;
        out[1] = t[1] * scale;
        out[5] = t[4] * scale;
        out[9] = t[7] * scale;
        out[2] = t[2] * scale;
        out[6] = t[5] * scale;
        out[10] = scale * t[8];
        out[3] = t[9];
        out[7] = t[10];
        out[11] = t[11];
    }
}

// 0x8AED50 (thiscall): NiD3D10Renderer::~NiD3D10Renderer (its references, scratch buffers, the
// precached render target groups' buffer data, textures' data, managers, device and members)
void __fastcall NiD3D10Renderer_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kVtable;
    re::Ptr32<void>& screenTextureNode = At<void*>(self, 0x468);
    if (screenTextureNode != nullptr) {
        ReleaseRef(screenTextureNode);
        screenTextureNode = nullptr;
    }
    ShaderLibraries_Shutdown();
    for (int offset = 0x14; offset <= 0x18; offset += 4) {
        void* material = At<void*>(self, offset);
        if (material != nullptr && HasClassBit(material, NiMaterialClassBit_20E38A8)) {
            Material_SetOwner(At<void*>(self, offset), nullptr);
        }
    }
    re::Ptr32<void>& batch = At<void*>(self, 0x470);
    if (batch != nullptr) {
        ReleaseRef(batch);
        batch = nullptr;
    }
    NiFree_752570(At<void*>(self, 0x458), 0xC);
    NiFree_752570(At<void*>(self, 0x45C), 0x10);
    NiFree_752570(At<void*>(self, 0x460), 8);
    uint8_t* map = self + kPrecacheMap;
    re::Ptr32<void> position = nullptr;
    for (uint32_t i = 0; i < At<uint32_t>(map, 4); i++) {
        if (At<re::Ptr32<void>*>(map, 8)[i] != nullptr) {
            position = At<re::Ptr32<void>*>(map, 8)[i];
            break;
        }
    }
    while (position != nullptr) {
        re::Ptr32<void> key;
        re::Ptr32<void> group = nullptr;
        PrecacheMap_GetNext(map, &position, &key, &group);
        if (group == nullptr) {
            continue;
        }
        for (uint32_t i = 0; i < Virtual<uint32_t>(group, 24); i++) {
            ReleaseBufferData(Virtual<uint8_t*>(group, 27, i));
        }
        ReleaseBufferData(Virtual<uint8_t*>(group, 28));
        ReleaseRef(group);
    }
    NiTMap_RemoveAll(map);
    TextureData_ClearTextureData();
    Renderer_PurgePrecacheList(self);
    void* factory = ShaderFactory_GetInstance();
    if (factory != nullptr) {
        Virtual<void>(factory, 8);
        Virtual<void>(factory, 13);
    }
    for (int offset = kResourceManager; offset <= kShaderResourceManager; offset += 4) {
        void* manager = At<void*>(self, offset);
        if (manager != nullptr) {
            Virtual<void*>(manager, 0, 1);
        }
    }
    re::Ptr32<void>& group = At<void*>(self, kDefaultRenderTargetGroup);
    if (group != nullptr) {
        ReleaseRef(group);
        group = nullptr;
    }
    if (Device(self) != nullptr) {
        ComRelease(Device(self));
        At<void*>(self, kDevice) = nullptr;
    }
    DestructArray(self + kDeviceRemovedCallbacks + 0x10);
    DestructArray(self + kDeviceRemovedCallbacks);
    DestructArray(self + kOcclusionCallbacks + 0x10);
    DestructArray(self + kOcclusionCallbacks);
    ReleaseRef(At<void*>(self, 0x470));
    ReleaseRef(At<void*>(self, 0x468));
    DestructList(self + kPrecacheList, kPrecacheListVtable);
    At<uint32_t>(map, 0) = kPrecacheMapVtable;
    NiTMap_RemoveAll(map);
    At<uint32_t>(map, 0) = kPrecacheMapBaseVtable;
    NiTMap_RemoveAll(map);
    NiFree(At<void*>(map, 8));
    DestructList(self + kBuffersInUse, kBuffersListVtable);
    ReleaseRef(At<void*>(self, kDefaultRenderTargetGroup));
    ReleaseRef(At<void*>(self, kSystemDesc));
    NiRenderer_Destruct(self);
}

// 0x8AF1C0 (thiscall): NiD3D10Renderer::Initialize(parameters) (the device and managers; the
// default swap chain and its group, kept in the precache map by window; the window associated;
// the shader factory's defaults; the default materials; the default material object; the clear
// planes)
uint8_t __fastcall NiD3D10Renderer_Initialize(uint8_t* self, re::Edx, uint8_t* parameters) {
    if (At<uint8_t>(self, kInitialized) != 0) {
        Warning(Text(0x0099FF88));
        return 0;
    }
    if (Renderer_CreateDevice(self, reinterpret_cast<uint32_t*>(parameters)) != 0) {
        Renderer_CreateManagers(self);
        if (At<uint8_t>(parameters, 0x10) == 0) {
            At<uint8_t>(self, kInitialized) = 1;
        } else {
            void* swapChain = Renderer_CreateSwapChain(self, parameters + 0x1C, At<uint32_t>(parameters, 4));
            if (swapChain == nullptr) {
                Warning(Text(0x0099FEB0));
            } else {
                void* group = Renderer_CreateRenderTargetGroupFromSwapChain(self, swapChain, At<uint8_t>(parameters, 0x11),
                                                                          At<uint32_t>(parameters, 0x18));
                if (group == nullptr) {
                    Warning(Text(0x0099FF10));
                } else {
                    NiPointer_Assign(&At<void*>(self, kDefaultRenderTargetGroup), group);
                    re::Ptr32<void> value;
                    NiPointer_Construct(&value, group);   // (handed by value: the map lets it go)
                    PrecacheMap_SetAt(self + kPrecacheMap, At<void*>(parameters, 0x48), value);
                    At<uint8_t>(self, kInitialized) = 1;
                }
            }
        }
    }
    if (At<uint8_t>(self, kInitialized) == 0) {
        if (Device(self) != nullptr) {
            ComRelease(Device(self));
            At<void*>(self, kDevice) = nullptr;
        }
        return 0;
    }
    if (At<uint8_t>(parameters, 0x12) != 0) {
        void* factory = SystemDesc_GetFactory(At<void*>(self, kSystemDesc));
        Com<long>(factory, kMakeWindowAssociation, At<void*>(parameters, 0x48), At<uint32_t>(parameters, 0x14));
    }
    void* shaderFactory = ShaderFactory_GetInstance();
    ShaderFactory_Initialize(shaderFactory);
    At<uint32_t>(self, kDefaultStates) = Virtual<uint32_t>(shaderFactory, 18, 4u, 0u);
    At<uint32_t>(self, kDefaultStates + 4) = Virtual<uint32_t>(shaderFactory, 21, 4u, 0u);
    At<uint32_t>(self, kDefaultStates + 8) = Virtual<uint32_t>(shaderFactory, 24, 4u, 0u);
    if (ms_pkShadowManager != nullptr) {
        uint32_t a = Virtual<uint32_t>(shaderFactory, 17, At<uint32_t>(self, kDefaultStates + 8));
        uint32_t b = Virtual<uint32_t>(shaderFactory, 16, At<uint32_t>(self, kDefaultStates + 8));
        uint32_t c = Virtual<uint32_t>(shaderFactory, 17, At<uint32_t>(self, kDefaultStates + 4));
        uint32_t d = Virtual<uint32_t>(shaderFactory, 16, At<uint32_t>(self, kDefaultStates + 4));
        uint32_t e = Virtual<uint32_t>(shaderFactory, 17, At<uint32_t>(self, kDefaultStates));
        uint32_t f = Virtual<uint32_t>(shaderFactory, 16, At<uint32_t>(self, kDefaultStates));
        ShaderNames_Report(f, e, d, c, b, a);
    }
    void* material = DynamicCast(NiSingleShaderMaterial_RTTI, At<void*>(self, 0x14));
    if (material != nullptr) {
        NiRenderer_PrecacheMaterial(self, material);
    }
    NiPointer_AssignPointer(&At<void*>(self, 0x18), &At<void*>(self, 0x14));
    if (ms_pkShadowManager != nullptr) {
        for (int offset = 0x50; offset <= 0x58; offset += 4) {
            void* shadowMaterial = At<void*>(ms_pkShadowManager, offset);
            if (shadowMaterial != nullptr) {
                NiRenderer_PrecacheMaterial(self, shadowMaterial);
            }
        }
    }
    At<uint32_t>(self, 0x300) = At<uint32_t>(self, kDefaultStates);
    At<uint32_t>(self, 0x304) = Virtual<uint32_t>(MaterialLibrary_GetInstance(), 18, 1u, 1u);
    At<uint32_t>(self, 0x308) = At<uint32_t>(self, kDefaultStates);
    At<uint32_t>(self, 0x30C) = At<uint32_t>(self, kDefaultStates + 4);
    At<uint32_t>(self, 0x310) = Virtual<uint32_t>(MaterialLibrary_GetInstance(), 21, 4u, 0u);
    At<uint32_t>(self, 0x314) = At<uint32_t>(self, kDefaultStates + 4);
    At<uint32_t>(self, 0x318) = At<uint32_t>(self, kDefaultStates + 8);
    At<uint32_t>(self, 0x31C) = Virtual<uint32_t>(MaterialLibrary_GetInstance(), 24, 1u, 1u);
    At<uint32_t>(self, 0x320) = At<uint32_t>(self, kDefaultStates + 8);
    At<uint32_t>(self, 0x324) = Virtual<uint32_t>(MaterialLibrary_GetInstance(), 18, 0u, 0u);
    At<uint32_t>(self, 0x328) = Virtual<uint32_t>(MaterialLibrary_GetInstance(), 18, 0u, 0u);
    At<uint32_t>(self, 0x32C) = Virtual<uint32_t>(MaterialLibrary_GetInstance(), 18, 0u, 0u);
    At<uint32_t>(self, 0x330) = 0x20;
    ShaderFactory_GetInstance();
    void* memory = NiNew(0x84);
    void* defaultMaterial = memory != nullptr ? DefaultMaterial_Construct(memory) : nullptr;
    Virtual<void>(defaultMaterial, 4);
    Virtual<void>(self, 65, defaultMaterial);
    float nearClip = *reinterpret_cast<const float*>(0x0090054C);   // 0.01
    float farClip = *reinterpret_cast<const float*>(0x0094430C);    // 0.99
    At<float>(self, 0x28) = nearClip;
    At<float>(self, 0x20) = nearClip;
    At<uint8_t>(self, kOccluded) = 0;
    At<float>(self, 0x24) = farClip;
    At<uint8_t>(self, kDeviceRemoved) = 0;
    At<float>(self, 0x2C) = farClip;
    return At<uint8_t>(self, kInitialized);
}

// 0x8AF7D0 (thiscall, slot 0): the deleting destructor
void* __fastcall NiD3D10Renderer_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    Renderer_Destruct(self);
    if (flags & 1) {
        NiDelete(self, kRendererSize);
    }
    return self;
}

// 0x8AF800: NiD3D10Renderer::Create(parameters, result) (the reference let go first; true when
// the new renderer initialized)
uint8_t __cdecl NiD3D10Renderer_Create(uint32_t* parameters, re::Ptr32<void>* result) {
    if (*result != nullptr) {
        ReleaseRef(*result);
        *result = nullptr;
    }
    void* memory = NiNew(kRendererSize);
    void* renderer = memory != nullptr ? Renderer_Construct(memory) : nullptr;
    AssignRef(*result, renderer);
    return Renderer_Initialize(*result, parameters);
}

#define C42H_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C42H_REVERSED("NiD3D10Renderer::CalculateBoneMatrices", 0x008ACEF0, NiD3D10Renderer_CalculateBoneMatrices);
C42H_REVERSED("NiD3D10Renderer::EnsureBoneScratchBuffers", 0x008AD1B0, NiD3D10Renderer_EnsureBoneScratchBuffers);
C42H_REVERSED("NiD3D10Renderer::NiD3D10Renderer", 0x008AD280, NiD3D10Renderer_Construct);
C42H_REVERSED("NiD3D10Renderer::CreateDeviceFromParameters", 0x008AD760, NiD3D10Renderer_CreateDeviceFromParameters);
C42H_REVERSED("NiD3D10Renderer::~NiD3D10Renderer", 0x008AED50, NiD3D10Renderer_Destruct);
C42H_REVERSED("NiD3D10Renderer::Initialize", 0x008AF1C0, NiD3D10Renderer_Initialize);
C42H_REVERSED("NiD3D10Renderer::DeletingDestructor", 0x008AF7D0, NiD3D10Renderer_DeletingDestructor);
C42H_REVERSED("NiD3D10Renderer::Create", 0x008AF800, NiD3D10Renderer_Create);

} // namespace ni::c42
