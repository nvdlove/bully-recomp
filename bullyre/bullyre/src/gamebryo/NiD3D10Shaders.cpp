// The D3D10 renderer's shaders in this range (the NiDX9 ones' counterparts):
//
// NiD3D10ShaderInterface (vtable 0x9A10F8, NiRTTI 0x20E9170 on NiShader's; 0x1C bytes): NiShader's
// 0x14, then +0x14 a flag (slots 32 / 33 get and set it; a guess: "is the best implementation")
// and +0x18 its shader declaration (a reference). Made, it's registered with the renderer; let
// go, unregistered.
//
// NiD3D10FragmentShader (vtable 0x9A08A0, NiRTTI 0x20E9000 on NiD3D10Shader's; 0x88 bytes): the
// generically configurable NiD3D10Shader the fragment materials build, as NiDX9FragmentShader is:
// its passes (NiD3D10Shader's NiTObjectArray<NiD3D10PassPtr> at +0x54: +0x58 the array, +0x5C its
// size, +0x5E the count, +0x62 the growth), programs, constant maps, samplers and alpha overrides
// set from outside; its samplers pointed at the texturing property's / dynamic effects' textures
// each update. +0x70 its NiFragmentShaderInstanceDescriptor (vtable 0x99C7AC; +0x74 the material
// descriptor, its first a NiFixedString). Its own slots: 0 the deleting destructor, 2 GetRTTI, 4
// Initialize, 7 UpdatePipeline, 18 CreateShaderDeclaration, 19 AppendRenderPass, 20
// SetAlphaOverride, 22 SetGPUProgram, 23 CreateShaderConstantMap, 24 AppendTextureSampler, 25
// GetShaderInstanceDesc. NiD3D10Pass read by its methods (chunk 44's): its render state group
// (+0x18; NiD3D10RenderStateGroup, 0xB80 bytes), its samplers (up to 16).
//
// NiD3D10ShadowWriteShader (vtable 0x9A0940, NiRTTI 0x20E9034): the fragment shader the shadow
// write materials draw shadow maps with; its PreProcessPipeline flips the cull mode (front <->
// back) when Bully's flag at 0xBCB9A1 asks (DX9's at 0xBCB804).
//
// NiD3D10ErrorShader (vtable 0x9A1458, NiRTTI 0x20E91A8 on NiD3D10Shader's; 0x84 bytes): one pass
// (+0x70, a raw pointer to the one the pass array holds), a vertex shader putting the position
// through WorldViewProj and a pixel shader giving MaterialColor (+0x74, a float4: a random colour,
// each channel 0.5..1, each draw), the model transform jittered by up to a tenth of the bound's
// radius each draw (the "something is wrong" shimmer).

#include "gamebryo/NiD3D10C43.h"
#include "reversed.h"

namespace ni::c43 {
namespace {

using ni::sys::FixedStringRelease;

constexpr uintptr_t kShaderInterfaceVtable = 0x009A10F8;
constexpr uintptr_t kFragmentShaderVtable = 0x009A08A0;
constexpr uintptr_t kShadowWriteShaderVtable = 0x009A0940;
constexpr uintptr_t kErrorShaderVtable = 0x009A1458;
constexpr uintptr_t kInstanceDescriptorVtable = 0x0099C7AC;         // NiFragmentShaderInstanceDescriptor
constexpr uintptr_t kShaderInstanceDescriptorVtable = 0x0099C7A4;   // NiShader::NiShaderInstanceDescriptor

GAME_VAR(uint8_t, NiRTTI_NiD3D10ShaderInterface, 0x020E9170);
GAME_VAR(uint8_t, NiRTTI_NiD3D10FragmentShader, 0x020E9000);
GAME_VAR(uint8_t, NiRTTI_NiD3D10ShadowWriteShader, 0x020E9034);
GAME_VAR(uint8_t, NiRTTI_NiD3D10ErrorShader, 0x020E91A8);
GAME_VAR(uint8_t, ms_bShadowFlipCull, 0x00BCB9A1);      // Bully's (a guess: the shadow maps' cull flip)
GAME_VAR(uint8_t, ms_ucMatrix4Bits, 0x020E4085);        // the constant map entry's type bits for a matrix4
GAME_VAR(uint8_t, ms_ucPoint4Bits, 0x020E4083);         // ... for a point4

// NiShader / NiD3D10Shader
GAME_FN(void*, __thiscall, Shader_ConstructBase, 0x0077C2F0, void* self);   // NiShader
GAME_FN(void, __thiscall, Shader_DestructBase, 0x0077C350, void* self);
GAME_FN(void, __thiscall, Shader_SetName, 0x0077C3A0, void* self, re::Ptr32<char>* name);
GAME_FN(uint32_t, __thiscall, ShaderInterface_UpdatePipeline, 0x0077C2A0, void* self, void* geometry, void* skin,
        void* bufferData, void* state, void* effects, const void* world, const void* bound);
GAME_FN(void*, __thiscall, D3D10Shader_Construct, 0x008C0EE0, void* self);
GAME_FN(void, __thiscall, D3D10Shader_Destruct, 0x008C0E40, void* self);
GAME_FN(uint8_t, __thiscall, D3D10Shader_Initialize, 0x008C06C0, void* self);   // (a jmp to 0x8B93B0)
GAME_FN(void, __thiscall, D3D10Shader_SetFlags, 0x008C0770, void* self, uint32_t a, uint32_t b, uint32_t c);
GAME_FN(uint32_t, __thiscall, D3D10Shader_PreProcessPipeline, 0x008C07A0, void* self, void* geometry, void* skin,
        void* bufferData, void* state, void* effects, const void* world, const void* bound);
GAME_FN(uint32_t, __thiscall, D3D10Shader_SetupTransformations, 0x008C0710, void* self, void* geometry, void* skin,
        void* partition, void* bufferData, void* state, void* effects, const void* world, const void* bound);
GAME_FN(void, __thiscall, MaterialDescriptor_Construct, 0x007D5470, void* self);
GAME_FN(void, __thiscall, MaterialDescriptor_Assign, 0x007D5490, void* self, const void* other);
GAME_FN(void, __thiscall, PassArray_SetSize, 0x00856170, void* array, uint32_t size);
GAME_FN(void, __thiscall, PassArray_SetAt, 0x0040E800, void* array, uint32_t index, const re::Ptr32<void>* pass);
GAME_FN(void, __thiscall, PassArray_Destruct, 0x0040E7A0, void* array);
// the renderer's (chunk 42's)
GAME_FN(uint8_t*, __cdecl, D3D10Renderer_GetRenderer, 0x008A9300);
GAME_FN(void*, __thiscall, D3D10Renderer_GetRenderStateManager, 0x008A9110, const void* renderer);
GAME_FN(void, __thiscall, D3D10Renderer_RegisterShader, 0x008AB990, void* renderer, void* shader);
GAME_FN(void, __thiscall, D3D10Renderer_UnregisterShader, 0x008AD9C0, void* renderer, void* shader);
GAME_FN(long, __cdecl, D3D10_CreateBlob, 0x008A94C0, uint32_t size, re::Ptr32<void>* blob);
GAME_FN(void, __thiscall, Manager_GetRasterizerDesc, 0x008B0320, void* self, void* desc);
GAME_FN(void, __thiscall, Manager_SetRasterizerDesc, 0x008B0280, void* self, const void* desc, uint32_t valid);
// the render state manager's conversions (this chunk's)
GAME_FN(uint32_t, __cdecl, ConvertGbAlphaBlendToD3D10Blend, 0x008B0950, uint32_t mode);
GAME_FN(uint32_t, __cdecl, ConvertGbFilterModeToD3D10Filter, 0x008B0B20, uint32_t mode);
GAME_FN(uint8_t, __cdecl, ConvertGbFilterModeToMipmapEnable, 0x008B0B70, uint32_t mode);
GAME_FN(uint32_t, __cdecl, ConvertGbClampModeToD3D10AddressU, 0x008B0BB0, uint32_t mode);
GAME_FN(uint32_t, __cdecl, ConvertGbClampModeToD3D10AddressV, 0x008B0BF0, uint32_t mode);
// NiD3D10Pass
GAME_FN(void, __cdecl, Pass_CreateNewPass, 0x008C1F50, re::Ptr32<void>* pass);
GAME_FN(void*, __thiscall, Pass_GetRenderStateGroup, 0x008B40F0, void* pass);
GAME_FN(void, __thiscall, Pass_SetRenderStateGroup, 0x008C0FE0, void* pass, void* group);
GAME_FN(uint32_t, __thiscall, Pass_GetSamplerCount, 0x008A78B0, void* pass);
GAME_FN(uint32_t, __thiscall, Pass_AddSampler, 0x008C0F80, void* pass);
GAME_FN(uint32_t, __thiscall, Pass_GetSamplerMapFlags, 0x008C0FA0, void* pass, uint32_t sampler);
GAME_FN(uint16_t, __thiscall, Pass_GetSamplerEffectFlags, 0x008C0FC0, void* pass, uint32_t sampler);
GAME_FN(uint8_t, __thiscall, Pass_SetSamplerInfo, 0x008C1A10, void* pass, uint32_t sampler, const re::Ptr32<const char>* name,
        uint32_t mapFlags, uint32_t instance, uint32_t effectFlags);
GAME_FN(void, __thiscall, Pass_SetSamplerName, 0x008C1A70, void* pass, uint32_t sampler, const re::Ptr32<const char>* name);
GAME_FN(void, __thiscall, Pass_SetSamplerTexture, 0x008C1190, void* pass, uint32_t sampler, void* texture);
GAME_FN(void, __thiscall, Pass_SetVertexShader, 0x008C1050, void* pass, void* program);
GAME_FN(void, __thiscall, Pass_SetPixelShader, 0x008C1140, void* pass, void* program);
GAME_FN(void, __thiscall, Pass_SetGeometryShader, 0x008C10C0, void* pass, void* program);
GAME_FN(void*, __thiscall, Pass_GetVertexConstantMap, 0x008C1030, void* pass, uint32_t index);
GAME_FN(void*, __thiscall, Pass_GetPixelConstantMap, 0x008C1110, void* pass, uint32_t index);
GAME_FN(void*, __thiscall, Pass_GetGeometryConstantMap, 0x008C10A0, void* pass, uint32_t index);
GAME_FN(void, __thiscall, Pass_SetVertexConstantMap, 0x008C1FB0, void* pass, uint32_t index, void* map);
GAME_FN(void, __thiscall, Pass_SetPixelConstantMap, 0x008C2090, void* pass, uint32_t index, void* map);
GAME_FN(void, __thiscall, Pass_SetGeometryConstantMap, 0x008C2020, void* pass, uint32_t index, void* map);
GAME_FN(void*, __thiscall, ConstantMap_Construct, 0x008BF210, void* self, uint32_t programType);
// NiD3D10RenderStateGroup
GAME_FN(void*, __thiscall, Group_Construct, 0x008C05F0, void* self);
GAME_FN(void, __thiscall, Group_SetBlendEnable, 0x008C0260, void* group, uint32_t target, uint32_t enable);
GAME_FN(void, __thiscall, Group_SetSrcBlend, 0x008C0290, void* group, uint32_t blend);
GAME_FN(uint8_t, __thiscall, Group_GetSrcBlend, 0x008C02B0, void* group, uint32_t* blend);
GAME_FN(void, __thiscall, Group_RemoveSrcBlend, 0x008C02D0, void* group);
GAME_FN(void, __thiscall, Group_SetDestBlend, 0x008C02E0, void* group, uint32_t blend);
GAME_FN(uint8_t, __thiscall, Group_GetDestBlend, 0x008C0300, void* group, uint32_t* blend);
GAME_FN(void, __thiscall, Group_RemoveDestBlend, 0x008C0320, void* group);
GAME_FN(void, __thiscall, Group_SetBlendOp, 0x008C0330, void* group, uint32_t op);
GAME_FN(void, __thiscall, Group_SetSamplerFilter, 0x008C03D0, void* group, uint32_t stage, uint32_t sampler,
        uint32_t filter);
GAME_FN(void, __thiscall, Group_SetSamplerAddressU, 0x008C0400, void* group, uint32_t stage, uint32_t sampler,
        uint32_t mode);
GAME_FN(void, __thiscall, Group_SetSamplerAddressV, 0x008C0430, void* group, uint32_t stage, uint32_t sampler,
        uint32_t mode);
GAME_FN(void, __thiscall, Group_SetSamplerMaxLOD, 0x008C0460, void* group, uint32_t stage, uint32_t sampler,
        float lod);
// the texture names' lookups, the dynamic effects
GAME_FN(uint8_t, __cdecl, TexturingProperty_GetMapIDFromName, 0x00765B50, const re::Ptr32<const char>* name, uint32_t* id);
GAME_FN(uint8_t, __cdecl, TextureEffect_GetTextureTypeFromName, 0x0076BB50, const re::Ptr32<const char>* name, uint32_t* type);
GAME_FN(uint8_t, __cdecl, GetShadowSamplerIndex, 0x00893250, const re::Ptr32<const char>* name, uint32_t* index);
GAME_FN(uint8_t*, __thiscall, TexturingProperty_GetShaderMap, 0x0074A7E0, void* self, uint32_t index);
GAME_FN(uint8_t*, __cdecl, GetDynamicEffectForObject, 0x0077F380, void* effects, uint32_t objectType,
        uint32_t index);
// the shader programs made from a blob (cdecl: blob, extension, 2 unknown, entry point, profile,
// unknown, name, the result)
GAME_FN(void, __cdecl, ProgramFactory_CreateVertexShaderFromBlob, 0x008C2E00, void* blob, const char* extension,
        uint32_t a, uint32_t b, const char* entry, const char* profile, uint32_t c, const char* name,
        re::Ptr32<void>* result);
GAME_FN(void, __cdecl, ProgramFactory_CreatePixelShaderFromBlob, 0x008C3200, void* blob, const char* extension,
        uint32_t a, uint32_t b, const char* entry, const char* profile, uint32_t c, const char* name,
        re::Ptr32<void>* result);
// NiD3D10ShaderDeclaration::CreateInto (this chunk's)
GAME_FN(uint8_t, __cdecl, Declaration_CreateInto, 0x008B42C0, uint32_t maxEntries, uint32_t streams, re::Ptr32<void>* result);
// this file's (called where modes apply)
GAME_FN(void, __thiscall, FragmentShader_UpdateTextureSampler, 0x008B59E0, void* self, void* geometry, void* pass,
        uint32_t sampler, void* texturing, void* effects, uint32_t passIndex);
GAME_FN(void, __thiscall, FragmentShader_Destruct, 0x008B5D70, void* self);
GAME_FN(void*, __thiscall, FragmentShader_Construct, 0x008B5E80, void* self, const void* descriptor);
GAME_FN(uint8_t, __thiscall, ErrorShader_CreateShaders, 0x008BA760, void* self);
GAME_FN(uint8_t, __thiscall, ErrorShader_CreatePass, 0x008BAA90, void* self);

inline uint8_t* PassAt(const uint8_t* self, uint32_t index) {
    return At<re::Ptr32<uint8_t>*>(self, 0x58)[index];
}
inline uint16_t PassCount(const uint8_t* self) {
    return At<uint16_t>(self, 0x5E);
}

// a pass's render state group, made (and set) when it has none
uint8_t* GroupOf(uint8_t* pass) {
    uint8_t* group = static_cast<uint8_t*>(Pass_GetRenderStateGroup(pass));
    if (group == nullptr) {
        void* memory = NiNew(0xB80);
        group = memory != nullptr ? static_cast<uint8_t*>(Group_Construct(memory)) : nullptr;
        Pass_SetRenderStateGroup(pass, group);
    }
    return group;
}

// the instance descriptor's NiFixedString let go and its base vtable set, as the destructors do
void ReleaseInstanceDescriptor(uint8_t* self) {
    FixedStringRelease(reinterpret_cast<re::Ptr32<char>*>(self + 0x74));
    At<uint32_t>(self, 0x70) = kShaderInstanceDescriptorVtable;
}

} // namespace

// ---- NiD3D10ShaderInterface

// 0x8B93B0 (thiscall, slot 4): NiD3D10ShaderInterface::Initialize: a lone jmp to NiShader's
// (0x77C290)
RE_UNHOOKED("NiD3D10ShaderInterface::Initialize", 0x008B93B0);

// 0x8B93C0 (thiscall, slot 32, 4 bytes: under a hook's size): the flag at +0x14
uint8_t __fastcall ShaderInterface_GetFlag14(uint8_t* self, re::Edx) {
    return At<uint8_t>(self, 0x14);
}
RE_UNHOOKED_FN("NiD3D10ShaderInterface::GetFlag14", 0x008B93C0, &ShaderInterface_GetFlag14);

// 0x8B93D0 (thiscall, slot 33): the flag at +0x14 set
void __fastcall ShaderInterface_SetFlag14(uint8_t* self, re::Edx, int flag) {
    At<uint8_t>(self, 0x14) = uint8_t(flag);
}

// 0x8B93E0 (thiscall): NiD3D10ShaderInterface::NiD3D10ShaderInterface (registered with the
// renderer, which must be there)
uint8_t* __fastcall ShaderInterface_Construct(uint8_t* self, re::Edx) {
    Shader_ConstructBase(self);
    At<uint32_t>(self, 0) = kShaderInterfaceVtable;
    At<uint8_t>(self, 0x14) = 0;
    At<void*>(self, 0x18) = nullptr;
    D3D10Renderer_RegisterShader(D3D10Renderer_GetRenderer(), self);
    return self;
}

// 0x8B9410 (thiscall, slot 2): NiD3D10ShaderInterface::GetRTTI
void* __fastcall ShaderInterface_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiD3D10ShaderInterface;
}

// 0x8B9420 (thiscall): NiD3D10ShaderInterface::~NiD3D10ShaderInterface (unregistered when there's a
// renderer; the declaration let go)
void __fastcall ShaderInterface_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kShaderInterfaceVtable;
    uint8_t* renderer = D3D10Renderer_GetRenderer();
    if (renderer != nullptr) {
        D3D10Renderer_UnregisterShader(renderer, self);
    }
    ReleaseRef(At<void*>(self, 0x18));
    Shader_DestructBase(self);
}

// 0x8B9470 (thiscall, slot 0): NiD3D10ShaderInterface's deleting destructor
uint8_t* __fastcall ShaderInterface_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kShaderInterfaceVtable;
    uint8_t* renderer = D3D10Renderer_GetRenderer();
    if (renderer != nullptr) {
        D3D10Renderer_UnregisterShader(renderer, self);
    }
    ReleaseRef(At<void*>(self, 0x18));
    Shader_DestructBase(self);
    if (flags & 1) {
        NiDelete(self, 0x1C);
    }
    return self;
}

// ---- NiD3D10FragmentShader

// 0x8B40F0 (thiscall, 4 bytes: under a hook's size): the pointer at +0x18 (NiD3D10Pass's render
// state group, the shaders' declaration: the same code folded)
void* __fastcall Get18Impl(uint8_t* self, re::Edx) {
    return At<void*>(self, 0x18);
}
RE_UNHOOKED_FN("NiD3D10Pass::GetRenderStateGroup", 0x008B40F0, &Get18Impl);

// 0x8B5440 (thiscall, slot 4): NiD3D10FragmentShader::Initialize (once: its flags set (1, 3, 1),
// then NiD3D10Shader's)
uint8_t __fastcall FragmentShader_Initialize(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 0x10) != 0) {
        return 1;
    }
    D3D10Shader_SetFlags(self, 1, 3, 1);
    return D3D10Shader_Initialize(self) != 0 ? 1 : 0;
}

// 0x8B5470 (thiscall, slot 25, 4 bytes: under a hook's size): GetShaderInstanceDesc (+0x70)
void* __fastcall FragmentShader_GetShaderInstanceDesc(uint8_t* self, re::Edx) {
    return self + 0x70;
}
RE_UNHOOKED_FN("NiD3D10FragmentShader::GetShaderInstanceDesc", 0x008B5470, &FragmentShader_GetShaderInstanceDesc);

// 0x8B5480 (thiscall, slot 18): CreateShaderDeclaration(entries, streams): a declaration made and
// set (slot 27); returned after the local reference let go (as the game does)
void* __fastcall FragmentShader_CreateShaderDeclaration(uint8_t* self, re::Edx, uint32_t maxEntries, uint32_t streams) {
    re::Ptr32<void> declaration = nullptr;
    Declaration_CreateInto(maxEntries, streams, &declaration);
    Virtual<void>(self, 27, declaration);
    ReleaseRef(declaration);
    return declaration;
}

// 0x8B54E0 (thiscall, slot 20): SetAlphaOverride(pass, alpha blending, the previous pass's source,
// source, the previous pass's destination, destination): every target's blending set; with
// blending, the source factor (the previous pass's, or removed when it has none) and the
// destination the same way, ADD. (The destination, when not the previous pass's, is set with the
// source factor's setter: the game's.)
uint8_t __fastcall FragmentShader_SetAlphaOverride(uint8_t* self, re::Edx, uint32_t index, int blending,
                                                   int usePreviousSrc, uint32_t src, int usePreviousDest,
                                                   uint32_t dest) {
    if (index >= PassCount(self)) {
        return 0;
    }
    uint8_t* pass = PassAt(self, index);
    if (pass == nullptr) {
        return 0;
    }
    uint8_t* previous = index != 0 ? PassAt(self, index - 1) : nullptr;
    uint8_t* group = GroupOf(pass);
    void* previousGroup = previous != nullptr ? Pass_GetRenderStateGroup(previous) : nullptr;
    if (!re::Flag(blending)) {
        for (uint32_t i = 0; i < 8; i++) {
            Group_SetBlendEnable(group, i, 0);
        }
        return 1;
    }
    for (uint32_t i = 0; i < 8; i++) {
        Group_SetBlendEnable(group, i, 1);
    }
    uint32_t blend = 0;
    if (!re::Flag(usePreviousSrc)) {
        Group_SetSrcBlend(group, ConvertGbAlphaBlendToD3D10Blend(src));
    } else if (previousGroup != nullptr && Group_GetSrcBlend(previousGroup, &blend) != 0) {
        Group_SetSrcBlend(group, blend);
    } else {
        Group_RemoveSrcBlend(group);
    }
    if (!re::Flag(usePreviousDest)) {
        Group_SetSrcBlend(group, ConvertGbAlphaBlendToD3D10Blend(dest));
        Group_SetBlendOp(group, 1);
        return 1;
    }
    if (previousGroup != nullptr && Group_GetDestBlend(previousGroup, &blend) != 0) {
        Group_SetDestBlend(group, blend);
    } else {
        Group_RemoveDestBlend(group);
    }
    Group_SetBlendOp(group, 1);
    return 1;
}

// 0x8B5650 (thiscall, slot 22): SetGPUProgram(pass, program, &type): the program set on the pass
// by its type (0 vertex, 1 pixel, 2 geometry: NiGPUProgram's), the type given back
uint8_t __fastcall FragmentShader_SetGPUProgram(uint8_t* self, re::Edx, uint32_t index, uint8_t* program,
                                                uint32_t* type) {
    if (index >= PassCount(self)) {
        return 0;
    }
    uint8_t* pass = PassAt(self, index);
    if (pass == nullptr) {
        return 0;
    }
    switch (At<uint32_t>(program, 8)) {
    case 0:
        Pass_SetVertexShader(pass, program);
        *type = 0;
        break;
    case 1:
        Pass_SetPixelShader(pass, program);
        *type = 1;
        break;
    case 2:
        Pass_SetGeometryShader(pass, program);
        *type = 2;
        break;
    default:
        break;
    }
    return 1;
}

// 0x8B56C0 (thiscall, slot 23): CreateShaderConstantMap(pass, program type, index): the pass's map
// of that kind, made (NiD3D10ShaderConstantMap, 0x28 bytes) and set when it has none
void* __fastcall FragmentShader_CreateShaderConstantMap(uint8_t* self, re::Edx, uint32_t index, uint32_t type,
                                                        uint32_t mapIndex) {
    if (index >= PassCount(self)) {
        return nullptr;
    }
    uint8_t* pass = PassAt(self, index);
    if (pass == nullptr) {
        return nullptr;
    }
    void* map = nullptr;
    auto make = [&](uint32_t programType, void(__thiscall * set)(void*, uint32_t, void*)) -> void* {
        void* memory = NiNew(0x28);
        void* made = memory != nullptr ? ConstantMap_Construct(memory, programType) : nullptr;
        set(pass, mapIndex, made);
        return made;
    };
    if (type == 0) {
        map = Pass_GetVertexConstantMap(pass, mapIndex);
        if (map == nullptr) {
            return make(0, Pass_SetVertexConstantMap);
        }
    } else if (type == 2) {
        map = Pass_GetGeometryConstantMap(pass, mapIndex);
        if (map == nullptr) {
            return make(2, Pass_SetGeometryConstantMap);
        }
    } else if (type == 1) {
        map = Pass_GetPixelConstantMap(pass, mapIndex);
        if (map == nullptr) {
            return make(1, Pass_SetPixelConstantMap);
        }
    }
    return map;
}

// 0x8B57F0 (thiscall, slot 24): AppendTextureSampler(pass, &sampler, semantic, variable name,
// instance): a sampler added to the pass; its source by the semantic: a texturing property map
// (base .. shader maps: 0x200000.. 0xB00000; decals and shader maps with the instance), a texture
// effect (0x800 .. 0xB00 with the instance), or a shadow map (0xC00 .. 0xE00); unknown map ids
// fail; its name set either way
uint8_t __fastcall FragmentShader_AppendTextureSampler(uint8_t* self, re::Edx, uint32_t index, uint32_t* sampler,
                                                       const re::Ptr32<const char>* semantic, const re::Ptr32<const char>* name,
                                                       uint32_t instance) {
    if (index >= PassCount(self)) {
        return 0;
    }
    uint8_t* pass = PassAt(self, index);
    if (pass == nullptr) {
        return 0;
    }
    uint32_t added = Pass_AddSampler(pass);
    uint32_t id = 0;
    if (TexturingProperty_GetMapIDFromName(semantic, &id) != 0) {
        static const uint32_t kMapFlags[10] = {0x200000, 0x300000, 0x400000, 0x500000, 0x600000,
                                               0x700000, 0x800000, 0x900000, 0xA00000, 0xB00000};
        if (id > 9) {
            return 0;
        }
        Pass_SetSamplerInfo(pass, added, name, kMapFlags[id], id >= 8 ? instance : 0, 0);
    } else {
        uint32_t effect = 0;
        bool found = true;
        if (TextureEffect_GetTextureTypeFromName(semantic, &id) != 0) {
            static const uint32_t kEffect[4] = {0xA00, 0x900, 0x800, 0xB00};
            effect = id <= 3 ? kEffect[id] : 0;
        } else if (GetShadowSamplerIndex(semantic, &id) != 0) {
            effect = id == 0 ? 0xD00 : id == 1 ? 0xC00 : id == 2 ? 0xE00 : 0;
        } else {
            found = false;
        }
        if (found) {
            Pass_SetSamplerInfo(pass, added, name, 0, 0, (effect & 0xFF00) | instance);
        }
    }
    *sampler = added;
    Pass_SetSamplerName(pass, added, name);
    return 1;
}

// 0x8B59E0 (thiscall): NiD3D10FragmentShader::UpdateTextureSampler(geometry, pass, sampler, the
// texturing property, the dynamic effects, the pass's index): the sampler's texture, filter and
// clamp from its map (by the map flags) or its effect (by the effect flags: environment / fog
// maps, the instance-th switched-on projected light / shadow, Bully's shadowed lights' source
// textures); none when there's none; then the pass's render state group (made when missing) given
// the pixel stage's sampler state (filter, max LOD: FLT_MAX when mipmapped, else 0; U, V)
void __fastcall FragmentShader_UpdateTextureSamplerImpl(uint8_t*, re::Edx, void* geometry, uint8_t* pass,
                                                        uint32_t sampler, uint8_t* texturing, uint8_t* effects,
                                                        uint32_t) {
    void* texture = nullptr;
    uint32_t clamp = 0;
    uint32_t filter = 0;
    uint32_t mapFlags = Pass_GetSamplerMapFlags(pass, sampler);
    uint16_t effectFlags = Pass_GetSamplerEffectFlags(pass, sampler);
    auto fromMap = [&](const uint8_t* map) {
        texture = At<void*>(map, 8);
        uint16_t flags = At<uint16_t>(map, 4);
        clamp = (flags >> 12) & 3;
        filter = (flags >> 8) & 0xF;
    };
    if (effectFlags == 0) {
        uint32_t kind = mapFlags & 0xF00000;
        if (mapFlags != 0 && kind != 0) {
            re::Ptr32<uint8_t>* maps = At<re::Ptr32<uint8_t>*>(texturing, 0x20);
            uint32_t which = mapFlags & 0xFFFF;
            const uint8_t* map = nullptr;
            bool known = true;
            switch (kind) {
            case 0x200000: map = maps[0]; break;
            case 0x300000: map = maps[1]; break;
            case 0x400000: map = maps[2]; break;
            case 0x500000: map = maps[3]; break;
            case 0x600000: map = maps[4]; break;
            case 0x700000: map = maps[5]; break;
            case 0x800000: map = maps[6]; break;
            case 0x900000: map = maps[7]; break;
            case 0xA00000:
                if (which < uint32_t(At<uint16_t>(texturing, 0x26)) - 8) {
                    map = maps[8 + which];
                } else {
                    known = false;
                }
                break;
            case 0xB00000: {
                uint8_t* shaderMaps = At<uint8_t*>(texturing, 0x30);
                if (shaderMaps != nullptr && which < At<uint16_t>(shaderMaps, 0xA)) {
                    map = TexturingProperty_GetShaderMap(texturing, which);
                } else {
                    known = false;
                }
                break;
            }
            default:
                known = false;
                break;
            }
            if (known && map != nullptr) {
                fromMap(map);
            }
        }
    } else if (effects != nullptr && (effectFlags >> 8) >= 8 && (effectFlags >> 8) <= 14) {
        uint32_t which = effectFlags & 0xFF;
        uint8_t* effect = nullptr;
        switch (effectFlags >> 8) {
        case 8:   // the environment map (+0x18)
            effect = At<uint8_t*>(effects, 0x18);
            break;
        case 11:   // the fog map (+0x1C)
            effect = At<uint8_t*>(effects, 0x1C);
            break;
        case 9:
        case 10: {   // the which-th switched-on one of a list (+0x14, +0x10)
            re::Ptr32<void>* node = At<re::Ptr32<void>*>(effects, (effectFlags >> 8) == 9 ? 0x14 : 0x10);
            uint16_t count = 0;
            while (node != nullptr) {
                uint8_t* candidate = static_cast<uint8_t*>(node[1]);
                node = static_cast<re::Ptr32<void>*>(node[0]);
                if (candidate != nullptr && At<uint8_t>(candidate, 0xC4) == 1) {
                    uint16_t n = count;
                    count++;
                    if (n == which) {
                        effect = candidate;
                        break;
                    }
                }
            }
            break;
        }
        default: {   // 12..14: the shadowed lights' source textures (object types 5..7)
            uint8_t* light = GetDynamicEffectForObject(effects, (effectFlags >> 8) - 7, which);
            uint8_t* source = At<uint8_t*>(light, 0xD4);
            auto find = re::PtrCast<uint8_t*(__cdecl*)(void*, void*, uint32_t)>(At<void*>(source, 0x30));
            fromMap(find(source, geometry, 0xFF));
            break;
        }
        }
        if (effect != nullptr && At<uint8_t>(effect, 0xC4) != 0) {
            texture = At<void*>(effect, 0x158);
            clamp = At<uint32_t>(effect, 0x160);
            filter = At<uint32_t>(effect, 0x15C);
        }
    }
    Pass_SetSamplerTexture(pass, sampler, texture);
    uint8_t* group = GroupOf(pass);
    Group_SetSamplerFilter(group, 1, sampler, ConvertGbFilterModeToD3D10Filter(filter));
    float maxLOD = ConvertGbFilterModeToMipmapEnable(filter) != 0 ? 3.402823466e+38f : 0.0f;
    Group_SetSamplerMaxLOD(group, 1, sampler, maxLOD);
    Group_SetSamplerAddressU(group, 1, sampler, ConvertGbClampModeToD3D10AddressU(clamp));
    Group_SetSamplerAddressV(group, 1, sampler, ConvertGbClampModeToD3D10AddressV(clamp));
}

// 0x8B5D70 (thiscall): NiD3D10FragmentShader::~NiD3D10FragmentShader
void __fastcall FragmentShader_DestructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kFragmentShaderVtable;
    ReleaseInstanceDescriptor(self);
    D3D10Shader_Destruct(self);
}

// 0x8B5DC0 (thiscall, slot 2): NiD3D10FragmentShader::GetRTTI
void* __fastcall FragmentShader_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiD3D10FragmentShader;
}

// 0x8B5DD0 (thiscall, slot 7): NiD3D10FragmentShader::UpdatePipeline (every pass's samplers
// updated, then NiD3DShaderInterface's: 0)
uint32_t __fastcall FragmentShader_UpdatePipeline(uint8_t* self, re::Edx, void* geometry, void* skin, void* bufferData,
                                                  uint8_t* state, void* effects, const void* world,
                                                  const void* bound) {
    for (uint32_t i = 0; i < PassCount(self); i++) {
        uint8_t* pass = PassAt(self, i);
        if (pass != nullptr) {
            void* texturing = At<void*>(state, 0x28);
            uint32_t count = Pass_GetSamplerCount(pass);
            for (uint32_t s = 0; s < count; s++) {
                FragmentShader_UpdateTextureSampler(self, geometry, pass, s, texturing, effects, i);
            }
        }
    }
    return ShaderInterface_UpdatePipeline(self, geometry, skin, bufferData, state, effects, world, bound);
}

// 0x8B5E80 (thiscall): NiD3D10FragmentShader::NiD3D10FragmentShader(material descriptor)
uint8_t* __fastcall FragmentShader_ConstructImpl(uint8_t* self, re::Edx, const void* descriptor) {
    D3D10Shader_Construct(self);
    At<uint32_t>(self, 0) = kFragmentShaderVtable;
    At<uint32_t>(self, 0x70) = kInstanceDescriptorVtable;
    MaterialDescriptor_Construct(self + 0x74);
    MaterialDescriptor_Assign(self + 0x74, descriptor);
    return self;
}

// 0x8B5EB0 (thiscall, slot 0): NiD3D10FragmentShader's deleting destructor
uint8_t* __fastcall FragmentShader_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kFragmentShaderVtable;
    ReleaseInstanceDescriptor(self);
    D3D10Shader_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x88);
    }
    return self;
}

// 0x8B5F20 (thiscall, slot 19): AppendRenderPass(&index): a new pass put at the end of the passes
// (the array grown by its step when full)
uint8_t __fastcall FragmentShader_AppendRenderPass(uint8_t* self, re::Edx, uint32_t* index) {
    re::Ptr32<void> pass = nullptr;
    Pass_CreateNewPass(&pass);
    uint32_t count = PassCount(self);
    uint8_t* array = self + 0x54;
    if (count >= At<uint16_t>(self, 0x5C)) {
        PassArray_SetSize(array, At<uint16_t>(array, 0xE) + count);
    }
    PassArray_SetAt(array, count, &pass);
    *index = count;
    ReleaseRef(pass);
    return 1;
}

// ---- NiD3D10ShadowWriteShader

// 0x8B5FA0 (thiscall): NiD3D10ShadowWriteShader::NiD3D10ShadowWriteShader
uint8_t* __fastcall ShadowWriteShader_Construct(uint8_t* self, re::Edx, const void* descriptor) {
    FragmentShader_Construct(self, descriptor);
    At<uint32_t>(self, 0) = kShadowWriteShaderVtable;
    return self;
}

// 0x8B5FC0 (thiscall, slot 6): NiD3D10ShadowWriteShader::PreProcessPipeline (NiD3D10Shader's, then
// when Bully asks, the render state manager's cull mode flipped front <-> back)
uint32_t __fastcall ShadowWriteShader_PreProcessPipeline(uint8_t* self, re::Edx, void* geometry, void* skin,
                                                         void* bufferData, void* state, void* effects,
                                                         const void* world, const void* bound) {
    uint32_t result = D3D10Shader_PreProcessPipeline(self, geometry, skin, bufferData, state, effects, world, bound);
    if (ms_bShadowFlipCull != 0) {
        void* manager = D3D10Renderer_GetRenderStateManager(D3D10Renderer_GetRenderer());
        uint32_t desc[10];   // D3D10_RASTERIZER_DESC
        Manager_GetRasterizerDesc(manager, desc);
        if (desc[1] == 2) {
            desc[1] = 3;
        } else if (desc[1] == 3) {
            desc[1] = 2;
        }
        Manager_SetRasterizerDesc(manager, desc, 2);
    }
    return result;
}

// 0x8B6050 (thiscall, slot 2): NiD3D10ShadowWriteShader::GetRTTI
void* __fastcall ShadowWriteShader_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiD3D10ShadowWriteShader;
}

// 0x8B6060 (thiscall, slot 0): NiD3D10ShadowWriteShader's deleting destructor
uint8_t* __fastcall ShadowWriteShader_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kShadowWriteShaderVtable;
    FragmentShader_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x88);
    }
    return self;
}

// ---- NiD3D10ErrorShader

// 0x8BA600 (thiscall, slot 9): NiD3D10ErrorShader::SetupTransformations: a new random colour
// (each channel rand() / 32767 * 0.5 + 0.5), the world transform copied with its translation
// moved by (2 * rand() / 32767 - 1) * radius / 10 on each axis; then NiD3D10Shader's with that, no
// skin and no partition. The x87 sequences in float, in the game's order (main thread).
uint32_t __fastcall ErrorShader_SetupTransformations(uint8_t* self, re::Edx, void* geometry, void*, void*,
                                                     void* bufferData, void* state, void* effects, const float* world,
                                                     const float* bound) {
    for (int channel = 0; channel < 3; channel++) {
        float c = float(double(crt::Rand()) / 32767.0);
        At<float>(self, 0x74 + channel * 4) = float(double(c) * 0.5 + 0.5);
    }
    float local[13];   // NiTransform: the rotation, +0x24 the translation, +0x30 the scale
    for (int i = 0; i < 13; i++) {
        local[i] = world[i];
    }
    float scale = float(double(bound[3]) * double(0.1f));   // 0.1 as the float the double holds
    for (int axis = 0; axis < 3; axis++) {
        int32_t r = crt::Rand();
        float jitter = float(double(r + r) / 32767.0);
        jitter = float(double(jitter) - 1.0);
        local[9 + axis] = local[9 + axis] + jitter * scale;
    }
    return D3D10Shader_SetupTransformations(self, geometry, nullptr, nullptr, bufferData, state, effects, local,
                                            bound);
}

// 0x8BA760 (thiscall): NiD3D10ErrorShader::CreateShaders (with a declaration): the HLSL of the
// vertex and pixel shaders copied into blobs and made programs ("vs_4_0" VSMain,
// NiD3D10ErrorShader_VS; "ps_4_0" PSMain, NiD3D10ErrorShader_PS), set on the pass; the vertex
// constant map with WorldViewProj and MaterialColor (the colour white at first). (A blob or map
// that couldn't be made isn't checked, as in the game.)
uint8_t __fastcall ErrorShader_CreateShadersImpl(uint8_t* self, re::Edx) {
    if (At<void*>(self, 0x18) == nullptr) {
        return 0;
    }
    char vertexSource[0x110];
    char pixelSource[0x88];
    crt::Memcpy(vertexSource, reinterpret_cast<const void*>(0x009A1348), sizeof(vertexSource));
    crt::Memcpy(pixelSource, reinterpret_cast<const void*>(0x009A12C0), sizeof(pixelSource));
    re::Ptr32<void> vertexProgram = nullptr;
    re::Ptr32<void> pixelProgram = nullptr;
    re::Ptr32<void> blob = nullptr;
    D3D10_CreateBlob(0x110, &blob);
    uint32_t size = Com<uint32_t>(blob, kGetBufferSize);
    crt::MemcpyS(Com<void*>(blob, kGetBufferPointer), size, vertexSource, 0x110);
    ProgramFactory_CreateVertexShaderFromBlob(blob, ".vsh", 0, 0, "VSMain", "vs_4_0", 0, "NiD3D10ErrorShader_VS",
                                              &vertexProgram);
    Pass_SetVertexShader(At<void*>(self, 0x70), vertexProgram);
    Com<long>(blob, kRelease);
    re::Ptr32<void> pixelBlob = nullptr;
    D3D10_CreateBlob(0x88, &pixelBlob);
    size = Com<uint32_t>(pixelBlob, kGetBufferSize);
    crt::MemcpyS(Com<void*>(pixelBlob, kGetBufferPointer), size, pixelSource, 0x88);
    ProgramFactory_CreatePixelShaderFromBlob(pixelBlob, ".psh", 0, 0, "PSMain", "ps_4_0", 0, "NiD3D10ErrorShader_PS",
                                             &pixelProgram);
    Pass_SetPixelShader(At<void*>(self, 0x70), pixelProgram);
    Com<long>(pixelBlob, kRelease);
    void* memory = NiNew(0x28);
    uint8_t* map = nullptr;
    if (memory != nullptr) {
        map = static_cast<uint8_t*>(ConstantMap_Construct(memory, 0));
        AddRefRef(map);
    }
    Pass_SetVertexConstantMap(At<void*>(self, 0x70), 0, map);
    // AddEntry(key, flags, extra, register, count, variable, data size, stride, data, copy)
    Virtual<void>(map, 2, "WorldViewProj", uint32_t(ms_ucMatrix4Bits) | 0x20000000u, 0u, 0u, 4u, "WorldViewProj", 0u,
                  0u, static_cast<void*>(nullptr), 0u);
    At<float>(self, 0x74) = 1.0f;
    At<float>(self, 0x78) = 1.0f;
    At<float>(self, 0x7C) = 1.0f;
    At<float>(self, 0x80) = 1.0f;
    Virtual<void>(map, 2, "MaterialColor", uint32_t(ms_ucPoint4Bits) | 0x10000000u, 0u, 0xAu, 1u, "MaterialColor",
                  0x10u, 4u, static_cast<void*>(self + 0x74), 0u);
    if (Decrement(&At<uint32_t>(map, 4)) == 0) {   // (unchecked, as in the game)
        Virtual<void>(map, 1);
    }
    ReleaseRef(pixelProgram);
    ReleaseRef(vertexProgram);
    return 1;
}

// 0x8BAA00 (thiscall, slot 4): NiD3D10ErrorShader::Initialize (NiD3D10Shader's; then a declaration
// of one entry (the position, FLOAT3, stream 0) and the shaders)
uint8_t __fastcall ErrorShader_Initialize(uint8_t* self, re::Edx) {
    if (D3D10Shader_Initialize(self) == 0) {
        return 0;
    }
    Declaration_CreateInto(1, 1, &At<void*>(self, 0x18));
    Virtual<void>(At<void*>(self, 0x18), 18, 0u, 0u, 2u, 0u);   // SetEntry
    ErrorShader_CreateShaders(self);
    return 1;
}

// 0x8BAA40 (thiscall, slot 2): NiD3D10ErrorShader::GetRTTI
void* __fastcall ErrorShader_GetRTTI(void*, re::Edx) {
    return &NiRTTI_NiD3D10ErrorShader;
}

// 0x8BAA50 (thiscall, slot 0): NiD3D10ErrorShader's deleting destructor (the pass array's
// destructor, then NiD3D10Shader's)
uint8_t* __fastcall ErrorShader_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kErrorShaderVtable;
    PassArray_Destruct(self + 0x54);
    D3D10Shader_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x84);
    }
    return self;
}

// 0x8BAA90 (thiscall): NiD3D10ErrorShader::CreatePass (a new pass kept at +0x70 and put first in
// the passes, the array sized by its step when empty)
uint8_t __fastcall ErrorShader_CreatePassImpl(uint8_t* self, re::Edx) {
    re::Ptr32<void> pass = nullptr;
    Pass_CreateNewPass(&pass);
    At<void*>(self, 0x70) = pass;
    uint8_t* array = self + 0x54;
    if (At<uint16_t>(array, 8) == 0) {
        PassArray_SetSize(array, At<uint16_t>(array, 0xE));
    }
    PassArray_SetAt(array, 0, &pass);
    ReleaseRef(pass);
    return 1;
}

// 0x8BAB00 (thiscall): NiD3D10ErrorShader::NiD3D10ErrorShader (its pass, its name, the flag at
// +0x14 set)
uint8_t* __fastcall ErrorShader_Construct(uint8_t* self, re::Edx) {
    D3D10Shader_Construct(self);
    At<uint32_t>(self, 0) = kErrorShaderVtable;
    ErrorShader_CreatePass(self);
    re::Ptr32<char> name = ni::sys::GlobalStringTable_AddString("NiD3D10ErrorShader");
    Shader_SetName(self, &name);
    FixedStringRelease(&name);
    At<uint8_t>(self, 0x14) = 1;
    return self;
}

#define GB43G_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

GB43G_REVERSED("NiD3D10ShaderInterface::SetFlag14", 0x008B93D0, ShaderInterface_SetFlag14);
GB43G_REVERSED("NiD3D10ShaderInterface::NiD3D10ShaderInterface", 0x008B93E0, ShaderInterface_Construct);
GB43G_REVERSED("NiD3D10ShaderInterface::GetRTTI", 0x008B9410, ShaderInterface_GetRTTI);
GB43G_REVERSED("NiD3D10ShaderInterface::~NiD3D10ShaderInterface", 0x008B9420, ShaderInterface_Destruct);
GB43G_REVERSED("NiD3D10ShaderInterface::DeletingDestructor", 0x008B9470, ShaderInterface_DeletingDestructor);
GB43G_REVERSED("NiD3D10FragmentShader::Initialize", 0x008B5440, FragmentShader_Initialize);
GB43G_REVERSED("NiD3D10FragmentShader::CreateShaderDeclaration", 0x008B5480, FragmentShader_CreateShaderDeclaration);
GB43G_REVERSED("NiD3D10FragmentShader::SetAlphaOverride", 0x008B54E0, FragmentShader_SetAlphaOverride);
GB43G_REVERSED("NiD3D10FragmentShader::SetGPUProgram", 0x008B5650, FragmentShader_SetGPUProgram);
GB43G_REVERSED("NiD3D10FragmentShader::CreateShaderConstantMap", 0x008B56C0, FragmentShader_CreateShaderConstantMap);
GB43G_REVERSED("NiD3D10FragmentShader::AppendTextureSampler", 0x008B57F0, FragmentShader_AppendTextureSampler);
GB43G_REVERSED("NiD3D10FragmentShader::UpdateTextureSampler", 0x008B59E0, FragmentShader_UpdateTextureSamplerImpl);
GB43G_REVERSED("NiD3D10FragmentShader::~NiD3D10FragmentShader", 0x008B5D70, FragmentShader_DestructImpl);
GB43G_REVERSED("NiD3D10FragmentShader::GetRTTI", 0x008B5DC0, FragmentShader_GetRTTI);
GB43G_REVERSED("NiD3D10FragmentShader::UpdatePipeline", 0x008B5DD0, FragmentShader_UpdatePipeline);
GB43G_REVERSED("NiD3D10FragmentShader::NiD3D10FragmentShader", 0x008B5E80, FragmentShader_ConstructImpl);
GB43G_REVERSED("NiD3D10FragmentShader::DeletingDestructor", 0x008B5EB0, FragmentShader_DeletingDestructor);
GB43G_REVERSED("NiD3D10FragmentShader::AppendRenderPass", 0x008B5F20, FragmentShader_AppendRenderPass);
GB43G_REVERSED("NiD3D10ShadowWriteShader::NiD3D10ShadowWriteShader", 0x008B5FA0, ShadowWriteShader_Construct);
GB43G_REVERSED("NiD3D10ShadowWriteShader::PreProcessPipeline", 0x008B5FC0, ShadowWriteShader_PreProcessPipeline);
GB43G_REVERSED("NiD3D10ShadowWriteShader::GetRTTI", 0x008B6050, ShadowWriteShader_GetRTTI);
GB43G_REVERSED("NiD3D10ShadowWriteShader::DeletingDestructor", 0x008B6060, ShadowWriteShader_DeletingDestructor);
GB43G_REVERSED("NiD3D10ErrorShader::SetupTransformations", 0x008BA600, ErrorShader_SetupTransformations);
GB43G_REVERSED("NiD3D10ErrorShader::CreateShaders", 0x008BA760, ErrorShader_CreateShadersImpl);
GB43G_REVERSED("NiD3D10ErrorShader::Initialize", 0x008BAA00, ErrorShader_Initialize);
GB43G_REVERSED("NiD3D10ErrorShader::GetRTTI", 0x008BAA40, ErrorShader_GetRTTI);
GB43G_REVERSED("NiD3D10ErrorShader::DeletingDestructor", 0x008BAA50, ErrorShader_DeletingDestructor);
GB43G_REVERSED("NiD3D10ErrorShader::CreatePass", 0x008BAA90, ErrorShader_CreatePassImpl);
GB43G_REVERSED("NiD3D10ErrorShader::NiD3D10ErrorShader", 0x008BAB00, ErrorShader_Construct);

} // namespace ni::c43
