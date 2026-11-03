// Gamebryo's generated material node libraries: five NiMaterialNodeLibrary builders made by
// Gamebryo's tools from its shader fragment files (fragments with their resources and HLSL code
// blocks), 246 KB of straight-line code. Written from the code by tools/gen_matlib.py (it
// executes them symbolically and prints them through their idioms): don't edit, regenerate.
// The calls go through the game's addresses, so the registry's modes apply. The originals make
// their temporary NiFixedStrings with 0x754760 or in place with 0x401050 and let them go
// through 0x517750, 0x401C90 or inline (the same operations); SetFixed does it one way.

#include "gamebryo/NiCore.h"

namespace ni {
namespace {

constexpr uintptr_t kFixedStringCreate = 0x00754760;
constexpr uintptr_t kFixedStringDestruct = 0x00517750;
constexpr uintptr_t kStringConstruct = 0x007741A0;
constexpr uintptr_t kStringDestruct = 0x00774240;

// NiMaterialNodeLibrary (0x1C), NiMaterialFragmentNode (0x44: Bully's, over the base 0x7E3060
// makes), NiMaterialResource (0x2C), NiCodeBlock (0x18)
constexpr uintptr_t kNodeLibraryConstruct = 0x007E2AC0;    // (version)
constexpr uintptr_t kFragmentNodeConstruct = 0x00798090;
constexpr uintptr_t kResourceConstruct = 0x007E2C10;
constexpr uintptr_t kCodeBlockConstruct = 0x007EAC00;
constexpr uintptr_t kFragmentNodeBaseConstruct = 0x007E3060;
constexpr uintptr_t kStringConstructCapacity = 0x007740D0;

// NiFixedString setters: the field at +8, +0xC, +0x10, +0x14, +0x18 (shared by the classes)
constexpr uintptr_t kSetType = 0x0077C3A0;                 // node, resource: +8
constexpr uintptr_t kBlockSetLanguage = 0x0077C3A0;        // code block: +8
constexpr uintptr_t kNodeSetName = 0x007EAD00;             // +0xC
constexpr uintptr_t kResourceSetSemantic = 0x007EAD00;
constexpr uintptr_t kBlockSetPlatform = 0x007EAD00;
constexpr uintptr_t kResourceSetLabel = 0x007E2D60;        // +0x10
constexpr uintptr_t kBlockSetTarget = 0x007E2D60;
constexpr uintptr_t kResourceSetVariable = 0x007E2D70;     // +0x14
constexpr uintptr_t kResourceSetDefaultValue = 0x007F80C0; // +0x18
constexpr uintptr_t kResourceSetCount = 0x008A2EF0;
// NiString setters and the rest
constexpr uintptr_t kNodeSetDescription = 0x007EA8E0;
constexpr uintptr_t kBlockSetText = 0x007EABE0;
constexpr uintptr_t kNodeAddInputResource = 0x007E3130;
constexpr uintptr_t kNodeAddOutputResource = 0x007E3410;
constexpr uintptr_t kNodeAddCodeBlock = 0x007EAA50;
constexpr uintptr_t kLibraryAddNode = 0x00856400;

template <typename... A>
uint32_t Thiscall(uintptr_t fn, uint32_t self, A... args) {
    return reinterpret_cast<uint32_t(__thiscall*)(uint32_t, re::Arg<A>...)>(fn)(self, args...);
}

// NiNew, then the constructor when it isn't null
template <typename... A>
uint32_t New(uint32_t size, uintptr_t ctor, A... args) {
    uint32_t p = reinterpret_cast<uint32_t>(NiNew(size));
    return p != 0 ? Thiscall(ctor, p, args...) : 0;
}

// the fragment node's constructor as 0x7B06F0 and its kin inline it: the base, the vtable, an
// empty NiTPrimitiveArray (0x9893C0) at +0x30 (its general constructor allocates nothing at
// size 0), a NiString of capacity 0x100 at +0x40
uint32_t NewFragmentNodeInline() {
    uint32_t p = reinterpret_cast<uint32_t>(NiNew(0x44));
    if (p == 0) {
        return 0;
    }
    Thiscall(kFragmentNodeBaseConstruct, p);
    uint8_t* q = reinterpret_cast<uint8_t*>(p);
    *reinterpret_cast<uint32_t*>(q) = 0x00955FC4;
    *reinterpret_cast<uint32_t*>(q + 0x30) = 0x009893C0;
    *reinterpret_cast<uint16_t*>(q + 0x38) = 0;
    *reinterpret_cast<uint16_t*>(q + 0x3E) = 1;
    *reinterpret_cast<uint16_t*>(q + 0x3A) = 0;
    *reinterpret_cast<uint16_t*>(q + 0x3C) = 0;
    *reinterpret_cast<uint32_t*>(q + 0x34) = 0;   // maximum size 0: no array
    *reinterpret_cast<uint32_t*>(q + 0x30) = 0x009893C0;
    Thiscall(kStringConstructCapacity, p + 0x40, 0x100u);
    return p;
}

// a temporary NiFixedString passed to a setter
void SetFixed(uintptr_t setter, uint32_t object, const char* text) {
    uint32_t s = reinterpret_cast<uint32_t(__cdecl*)(const char*)>(kFixedStringCreate)(text);
    Thiscall(setter, object, reinterpret_cast<uint32_t>(&s));
    Thiscall(kFixedStringDestruct, reinterpret_cast<uint32_t>(&s));
}

// a temporary NiString passed to a setter
void SetString(uintptr_t setter, uint32_t object, const char* text) {
    uint32_t s = 0;
    Thiscall(kStringConstruct, reinterpret_cast<uint32_t>(&s), text);
    Thiscall(setter, object, reinterpret_cast<uint32_t>(&s));
    Thiscall(kStringDestruct, reinterpret_cast<uint32_t>(&s));
}

void Call(uintptr_t method, uint32_t object, uint32_t arg) {
    Thiscall(method, object, arg);
}

} // namespace

// 0x798130: NiStandardMaterial's node library (its vertex and pixel shader fragments)
NiRefObject* __cdecl CreateStandardMaterialNodeLibrary() {
    uint32_t lib, node, res, block;
    lib = New(0x1C, kNodeLibraryConstruct, 1u);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "TransformNormal");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the world transform to the\n"
        "    normal.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Normal");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "Normal");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "World");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Normal");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldNrm");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    // Transform the normal into world space for lighting\n"
        "    WorldNrm = mul( Normal, (float3x3)World );\n"
        "\n"
        "    // Should not need to normalize here since we will normalize in the pixel \n"
        "    // shader due to linear interpolation across triangle not preserving\n"
        "    // normality.\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "TransformNBT");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the world transform to the\n"
        "    normal, binormal, and tangent.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Normal");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "Normal");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Binormal");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "Binormal");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Tangent");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "Tangent");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "World");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Normal");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldNrm");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Normal");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldBinormal");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Normal");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldTangent");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    // Transform the normal into world space for lighting\n"
        "    WorldNrm      = mul( Normal, (float3x3)World );\n"
        "    WorldBinormal = mul( Binormal, (float3x3)World );\n"
        "    WorldTangent  = mul( Tangent, (float3x3)World );\n"
        "    \n"
        "    // Should not need to normalize here since we will normalize in the pixel \n"
        "    // shader due to linear interpolation across triangle not preserving\n"
        "    // normality.\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "TransformPosition");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the view projection transform\n"
        "    to the input position. Additionally, this fragment applies the world \n"
        "    transform to the input position. \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "Position");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "World");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    // Transform the position into world space for lighting, and projected \n"
        "    // space for display\n"
        "    WorldPos = mul( float4(Position, 1.0f), World );\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "WorldToTangent");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for transforming a vector from world space\n"
        "    to tangent space.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "VectorIn");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Normal");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldNormalIn");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Binormal");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldBinormalIn");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Tangent");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldTangentIn");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetLabel, res, "Tangent");
    SetFixed(kResourceSetVariable, res, "VectorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    float3x3 xForm = float3x3(WorldTangentIn, WorldBinormalIn, WorldNormalIn);\n"
        "    VectorOut = mul(xForm, VectorIn.xyz);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "TransformSkinnedPosition");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the view projection and skinning \n"
        "    transform to the input position. Additionally, this fragment applies the \n"
        "    computed world transform to the input position. The weighted world \n"
        "    transform defined by the blendweights is output for use in normals or\n"
        "    other calculations as the new world matrix.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "Position");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "int4");
    SetFixed(kResourceSetSemantic, res, "BlendIndices");
    SetFixed(kResourceSetVariable, res, "BlendIndices");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "BlendWeight");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "BlendWeights");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x3");
    Call(kResourceSetCount, res, 0x1Eu);
    SetFixed(kResourceSetSemantic, res, "SkinBoneMatrix");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "Bones");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "SkinToWorldTransform");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "SkinBoneTransform");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    // TransformSkinnedPosition *********************************************\n"
        "    // Transform the skinned position into world space\n"
        "    // Composite the skinning transform which will take the vertex\n"
        "    // and normal to world space.\n"
        "    float fWeight3 = 1.0 - BlendWeights[0] - BlendWeights[1] - BlendWeights[2];\n"
        "    float4x3 ShortSkinBoneTransform;\n"
        "    ShortSkinBoneTransform  = Bones[BlendIndices[0]] * BlendWeights[0];\n"
        "    ShortSkinBoneTransform += Bones[BlendIndices[1]] * BlendWeights[1];\n"
        "    ShortSkinBoneTransform += Bones[BlendIndices[2]] * BlendWeights[2];\n"
        "    ShortSkinBoneTransform += Bones[BlendIndices[3]] * fWeight3;\n"
        "    SkinBoneTransform = float4x4(ShortSkinBoneTransform[0], 0.0f, \n"
        "        ShortSkinBoneTransform[1], 0.0f, \n"
        "        ShortSkinBoneTransform[2], 0.0f, \n"
        "        ShortSkinBoneTransform[3], 1.0f);\n"
        "\n"
        "    // Transform into world space.\n"
        "    WorldPos.xyz = mul(float4(Position, 1.0), ShortSkinBoneTransform);\n"
        "    WorldPos = mul(float4(WorldPos.xyz, 1.0), SkinToWorldTransform);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "TransformSkinnedPositionNoIndices");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the view projection and skinning \n"
        "    transform to the input position. Additionally, this fragment applies the \n"
        "    computed world transform to the input position. The weighted world \n"
        "    transform defined by the blendweights is output for use in normals or\n"
        "    other calculations as the new world matrix.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "Position");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "BlendWeight");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "BlendWeights");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x3");
    Call(kResourceSetCount, res, 4u);
    SetFixed(kResourceSetSemantic, res, "SkinBoneMatrix");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "Bones");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "SkinToWorldTransform");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "SkinBoneTransform");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    // TransformSkinnedPosition *********************************************\n"
        "    // Transform the skinned position into world space\n"
        "    // Composite the skinning transform which will take the vertex\n"
        "    // and normal to world space.\n"
        "    float fWeight3 = 1.0 - BlendWeights[0] - BlendWeights[1] - BlendWeights[2];\n"
        "    float4x3 ShortSkinBoneTransform;\n"
        "    ShortSkinBoneTransform  = Bones[0] * BlendWeights[0];\n"
        "    ShortSkinBoneTransform += Bones[1] * BlendWeights[1];\n"
        "    ShortSkinBoneTransform += Bones[2] * BlendWeights[2];\n"
        "    ShortSkinBoneTransform += Bones[3] * fWeight3;\n"
        "    SkinBoneTransform = float4x4(ShortSkinBoneTransform[0], 0.0f, \n"
        "        ShortSkinBoneTransform[1], 0.0f, \n"
        "        ShortSkinBoneTransform[2], 0.0f, \n"
        "        ShortSkinBoneTransform[3], 1.0f);\n"
        "\n"
        "    // Transform into world space.\n"
        "    WorldPos.xyz = mul(float4(Position, 1.0), ShortSkinBoneTransform);\n"
        "    WorldPos = mul(float4(WorldPos.xyz, 1.0), SkinToWorldTransform);\n"
        "\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "ProjectPositionWorldToProj");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the view projection transform\n"
        "    to the input world position.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPosition");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "ViewProjMatrix");
    SetFixed(kResourceSetVariable, res, "ViewProjection");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Proj");
    SetFixed(kResourceSetVariable, res, "ProjPos");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    ProjPos = mul(WorldPosition, ViewProjection);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "ProjectPositionWorldToViewToProj");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the view projection transform\n"
        "    to the input world position.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPosition");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "ViewMatrix");
    SetFixed(kResourceSetVariable, res, "ViewTransform");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "ProjMatrix");
    SetFixed(kResourceSetVariable, res, "ProjTransform");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "View");
    SetFixed(kResourceSetVariable, res, "ViewPos");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Proj");
    SetFixed(kResourceSetVariable, res, "ProjPos");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    ViewPos = mul(WorldPosition, ViewTransform);\n"
        "    ProjPos = mul(ViewPos, ProjTransform);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "9thOrderSphericalHarmonicLighting");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for generating the diffuse \n"
        "    lighting environment as compressed in spherical harmonics.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Normal");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldNormal");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "SHCoefficients");
    SetFixed(kResourceSetVariable, res, "SHCoefficients");
    Call(kResourceSetCount, res, 9u);
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "DiffuseColorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    DiffuseColorOut  = SHCoefficients[0];\n"
        "    DiffuseColorOut += SHCoefficients[1] * WorldNormal.x;\n"
        "    DiffuseColorOut += SHCoefficients[2] * WorldNormal.y;\n"
        "    DiffuseColorOut += SHCoefficients[3] * WorldNormal.z;\n"
        "    DiffuseColorOut += SHCoefficients[4] * WorldNormal.x * WorldNormal.z;\n"
        "    DiffuseColorOut += SHCoefficients[5] * WorldNormal.y * WorldNormal.z;\n"
        "    DiffuseColorOut += SHCoefficients[6] * WorldNormal.x * WorldNormal.y;\n"
        "    DiffuseColorOut += SHCoefficients[7] * \n"
        "        (3.0 * WorldNormal.z * WorldNormal.z  - 1.0);\n"
        "    DiffuseColorOut += SHCoefficients[8] * \n"
        "        (WorldNormal.x * WorldNormal.x  - WorldNormal.y * WorldNormal.y);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "CalculateNormalFromColor");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for sampling a normal map to generate the\n"
        "    new world-space normal.\n"
        "    \n"
        "    The normal map type is an enumerated value that indicates the following:\n"
        "        0 - Standard (rgb = normal/binormal/tangent)\n"
        "        1 - DXN (rg = normal.xy need to calculate z)\n"
        "        2 - DXT5 (ag = normal.xy need to calculate z)\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "NormalMap");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Normal");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldNormalIn");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Binormal");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldBinormalIn");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Tangent");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldTangentIn");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "int");
    SetFixed(kResourceSetSemantic, res, "NormalMapType");
    SetFixed(kResourceSetVariable, res, "NormalMapType");
    SetFixed(kResourceSetDefaultValue, res, "(0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Normal");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldNormalOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    \n"
        "    NormalMap = NormalMap * 2.0 - 1.0;\n"
        "    \n"
        "    // Do nothing extra for Standard\n"
        "    // Handle compressed types:\n"
        "    if (NormalMapType == 1) // DXN\n"
        "    {\n"
        "        NormalMap.rgb = float3(NormalMap.r, NormalMap.g, \n"
        "            sqrt(1 - NormalMap.r * NormalMap.r - NormalMap.g * NormalMap.g));\n"
        "    }\n"
        "    else if (NormalMapType == 2) // DXT5\n"
        "    {\n"
        "        NormalMap.rg = NormalMap.ag;\n"
        "        NormalMap.b = sqrt(1 - NormalMap.r*NormalMap.r -  \n"
        "            NormalMap.g * NormalMap.g);\n"
        "    }\n"
        "       \n"
        "    float3x3 xForm = float3x3(WorldTangentIn, WorldBinormalIn, WorldNormalIn);\n"
        "    xForm = transpose(xForm);\n"
        "    WorldNormalOut = mul(xForm, NormalMap.rgb);\n"
        "    \n"
        "    WorldNormalOut = normalize(WorldNormalOut);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "CalculateParallaxOffset");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for calculating the UV offset to apply\n"
        "    as a result of a parallax map.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Height");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "ParallaxOffsetScale");
    SetFixed(kResourceSetVariable, res, "OffsetScale");
    SetFixed(kResourceSetDefaultValue, res, "(0.05)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "ViewVector");
    SetFixed(kResourceSetLabel, res, "Tangent");
    SetFixed(kResourceSetVariable, res, "TangentSpaceEyeVec");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "ParallaxOffsetUV");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    // Calculate offset scaling constant bias.\n"
        "    float2 Bias = float2(OffsetScale, OffsetScale) * -0.5;\n"
        "\n"
        "    // Calculate offset\n"
        "    float2 Offset = Height.rg * OffsetScale + Bias;\n"
        "\n"
        "    // Get texcoord.\n"
        "    ParallaxOffsetUV = TexCoord + Offset * TangentSpaceEyeVec.xy;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "CalculateBumpOffset");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for calculating the UV offset to apply\n"
        "    as a result of a bump map.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "DuDv");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "BumpMatrix");
    SetFixed(kResourceSetVariable, res, "BumpMatrix");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "UVSet");
    SetFixed(kResourceSetVariable, res, "BumpOffset");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    BumpOffset.x = DuDv.x * BumpMatrix[0] + DuDv.y * BumpMatrix[2];\n"
        "    BumpOffset.y = DuDv.x * BumpMatrix[1] + DuDv.y * BumpMatrix[3];\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "OffsetUVFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying a UV offset to a texture\n"
        "    coordinate set.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoordIn");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoordOffset");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "UVSet");
    SetFixed(kResourceSetVariable, res, "TexCoordOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    TexCoordOut = TexCoordIn + TexCoordOffset;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "OffsetUVFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying a UV offset to a texture\n"
        "    coordinate set.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoordIn");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoordOffset");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "UVSet");
    SetFixed(kResourceSetVariable, res, "TexCoordOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    TexCoordOut = TexCoordIn + float3(TexCoordOffset.x, TexCoordOffset.y, 0.0);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "OffsetUVFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying a UV offset to a texture\n"
        "    coordinate set.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoordIn");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoordOffset");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "UVSet");
    SetFixed(kResourceSetVariable, res, "TexCoordOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    TexCoordOut = TexCoordIn + \n"
        "        float4(TexCoordOffset.x, TexCoordOffset.y, 0.0, 0.0);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "TexTransformApply");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying a transform to the input set\n"
        "    of texture coordinates.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "TexTransform");
    SetFixed(kResourceSetVariable, res, "TexTransform");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoordOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    \n"
        "    TexCoordOut = mul(float4(TexCoord.x, TexCoord.y, 0.0, 1.0), TexTransform);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "ProjectTextureCoordinates");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying a projection to the input set\n"
        "    of texture coordinates.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "TexTransform");
    SetFixed(kResourceSetVariable, res, "TexTransform");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoordOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "  \n"
        "    TexCoordOut = mul(float4(TexCoord, 1.0), TexTransform);\n"
        "    \n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "TextureRGBSample");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for sampling a texture and returning its value\n"
        "    as a RGB value.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "sampler2D");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "Sampler");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetVariable, res, "Saturate");
    SetFixed(kResourceSetDefaultValue, res, "(true)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    ColorOut.rgb = tex2D(Sampler, TexCoord).rgb;\n"
        "    if (Saturate)\n"
        "    {\n"
        "        ColorOut.rgb = saturate(ColorOut.rgb);\n"
        "    }\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "TextureRGBASample");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for sampling a texture and returning its value\n"
        "    as a RGB value and an A value.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "sampler2D");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "Sampler");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetVariable, res, "Saturate");
    SetFixed(kResourceSetDefaultValue, res, "(true)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    ColorOut = tex2D(Sampler, TexCoord);\n"
        "    if (Saturate)\n"
        "    {\n"
        "        ColorOut = saturate(ColorOut);\n"
        "    }\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "TextureRGBProjectSample");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for sampling a texture and returning its value\n"
        "    as a RGB value and an A value.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "sampler2D");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "Sampler");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetVariable, res, "Saturate");
    SetFixed(kResourceSetDefaultValue, res, "(true)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    float4 ProjTexCoord = TexCoord.xyzz;\n"
        "    ColorOut.rgb = tex2Dproj(Sampler, ProjTexCoord).rgb;\n"
        "    if (Saturate)\n"
        "    {\n"
        "        ColorOut.rgb = saturate(ColorOut.rgb);\n"
        "    }\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "TextureRGBCubeSample");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for sampling a texture and returning its value\n"
        "    as a RGB value.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "samplerCUBE");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "Sampler");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetVariable, res, "Saturate");
    SetFixed(kResourceSetDefaultValue, res, "(true)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    ColorOut.rgb = texCUBE(Sampler, TexCoord).rgb;\n"
        "    if (Saturate)\n"
        "    {\n"
        "        ColorOut.rgb = saturate(ColorOut.rgb);\n"
        "    }\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "CalculateFog");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for handling fogging calculations.\n"
        "    FogType can be one of 4 values:\n"
        "    \n"
        "        NONE   - 0\n"
        "        EXP    - 1\n"
        "        EXP2   - 2\n"
        "        LINEAR - 3\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "int");
    SetFixed(kResourceSetSemantic, res, "FogType");
    SetFixed(kResourceSetVariable, res, "FogType");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "FogDensity");
    SetFixed(kResourceSetVariable, res, "FogDensity");
    SetFixed(kResourceSetDefaultValue, res, "(1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "View");
    SetFixed(kResourceSetVariable, res, "ViewPosition");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetSemantic, res, "FogRange");
    SetFixed(kResourceSetVariable, res, "FogRange");
    SetFixed(kResourceSetDefaultValue, res, "(false)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "FogStartEnd");
    SetFixed(kResourceSetVariable, res, "FogStartEnd");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Fog");
    SetFixed(kResourceSetVariable, res, "FogOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    float d;\n"
        "    if (FogRange)\n"
        "    {\n"
        "        d = length(ViewPosition);\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        d = ViewPosition.z;\n"
        "    }\n"
        "    \n"
        "    if (FogType == 0) // NONE\n"
        "    {\n"
        "        FogOut = 1.0;\n"
        "    }\n"
        "    else if (FogType == 1) // EXP\n"
        "    {\n"
        "        FogOut = 1.0 / exp( d * FogDensity);\n"
        "    }\n"
        "    else if (FogType == 2) // EXP2\n"
        "    {\n"
        "        FogOut = 1.0 / exp( pow( d * FogDensity, 2));\n"
        "    }\n"
        "    else if (FogType == 3) // LINEAR\n"
        "    {\n"
        "        FogOut = saturate( (FogStartEnd.y - d) / \n"
        "            (FogStartEnd.y - FogStartEnd.x));\n"
        "    }\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "WorldReflect");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for computing the reflection vector.\n"
        "    The WorldViewVector is negated because the HLSL \"reflect\" function\n"
        "    expects a world-to-camera vector, rather than a camera-to-world vector.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Normal");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldNrm");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "ViewVector");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldViewVector");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetSemantic, res, "NormalizeNormal");
    SetFixed(kResourceSetVariable, res, "NormalizeNormal");
    SetFixed(kResourceSetDefaultValue, res, "(true)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "WorldReflect");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    if (NormalizeNormal)\n"
        "        WorldNrm = normalize(WorldNrm);\n"
        "    WorldReflect = reflect(-WorldViewVector, WorldNrm);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "CalculateViewVector");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for calculating the camera view vector.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "CameraPosition");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "CameraPos");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "ViewVector");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldViewVector");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    WorldViewVector = CameraPos - WorldPos;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "ClippingPlaneTest");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for calculating whether or not the current\n"
        "    position is on the positive or negative side of a clipping plane. If the\n"
        "    point is on the positive side, the return value will be 1.0. If the value \n"
        "    is on the negative side, the return value will be 0.0. If the point is on\n"
        "    the plane, the return value will be 0.0. If InvertClip is true, then the\n"
        "    fragment returns the opposite of the above.\n"
        "    \n"
        "    A plane is assumed to be the points X satisfying the expression:\n"
        "        X * normal = plane_constant\n"
        "        \n"
        "    The WorldClipPlane must match the form of NiPlane, which is of the form:\n"
        "        (normal.x, normal.y, normal.z, plane_constant)\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "ClippingPlane");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldClipPlane");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetSemantic, res, "InvertClip");
    SetFixed(kResourceSetVariable, res, "InvertClip");
    SetFixed(kResourceSetDefaultValue, res, "(false)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Scale");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "Scalar");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "       \n"
        "    float fDistance = dot(WorldClipPlane.xyz, WorldPos.xyz) - WorldClipPlane.w;\n"
        "    if (InvertClip)\n"
        "        Scalar = fDistance > 0.0 ? 0.0 : 1.0;\n"
        "    else\n"
        "        Scalar = fDistance > 0.0 ? 1.0 : 0.0;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "Light");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for accumulating the effect of a light\n"
        "    on the current pixel.\n"
        "    \n"
        "    LightType can be one of three values:\n"
        "        0 - Directional\n"
        "        1 - Point \n"
        "        2 - Spot\n"
        "        \n"
        "    Note that the LightType must be a compile-time variable,\n"
        "    not a runtime constant/uniform variable on most Shader Model 2.0 cards.\n"
        "    \n"
        "    The compiler will optimize out any constants that aren't used.\n"
        "    \n"
        "    Attenuation is defined as (const, linear, quad, range).\n"
        "    Range is not implemented at this time.\n"
        "    \n"
        "    SpotAttenuation is stored as (cos(theta/2), cos(phi/2), falloff)\n"
        "    theta is the angle of the inner cone and phi is the angle of the outer\n"
        "    cone in the traditional DX manner. Gamebryo only allows setting of\n"
        "    phi, so cos(theta/2) will typically be cos(0) or 1. To disable spot\n"
        "    effects entirely, set cos(theta/2) and cos(phi/2) to -1 or lower.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Normal");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldNrm");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "int");
    SetFixed(kResourceSetSemantic, res, "LightType");
    SetFixed(kResourceSetVariable, res, "LightType");
    SetFixed(kResourceSetDefaultValue, res, "(0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetSemantic, res, "Specularity");
    SetFixed(kResourceSetVariable, res, "SpecularEnable");
    SetFixed(kResourceSetDefaultValue, res, "(false)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Shadow");
    SetFixed(kResourceSetVariable, res, "Shadow");
    SetFixed(kResourceSetDefaultValue, res, "(1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "ViewVector");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldViewVector");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "LightPos");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "LightPos");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "LightAmbient");
    SetFixed(kResourceSetVariable, res, "LightAmbient");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "LightDiffuse");
    SetFixed(kResourceSetVariable, res, "LightDiffuse");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "LightSpecular");
    SetFixed(kResourceSetVariable, res, "LightSpecular");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "LightAttenuation");
    SetFixed(kResourceSetVariable, res, "LightAttenuation");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 1.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "LightSpotAttenuation");
    SetFixed(kResourceSetVariable, res, "LightSpotAttenuation");
    SetFixed(kResourceSetDefaultValue, res, "(-1.0, -1.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "LightDirection");
    SetFixed(kResourceSetVariable, res, "LightDirection");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetLabel, res, "Specular");
    SetFixed(kResourceSetVariable, res, "SpecularPower");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetLabel, res, "Ambient");
    SetFixed(kResourceSetVariable, res, "AmbientAccum");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetLabel, res, "Diffuse");
    SetFixed(kResourceSetVariable, res, "DiffuseAccum");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetLabel, res, "Specular");
    SetFixed(kResourceSetVariable, res, "SpecularAccum");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetLabel, res, "Ambient");
    SetFixed(kResourceSetVariable, res, "AmbientAccumOut");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetLabel, res, "Diffuse");
    SetFixed(kResourceSetVariable, res, "DiffuseAccumOut");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetLabel, res, "Specular");
    SetFixed(kResourceSetVariable, res, "SpecularAccumOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "   \n"
        "    // Get the world space light vector.\n"
        "    float3 LightVector;\n"
        "    float DistanceToLight;\n"
        "    float DistanceToLightSquared;\n"
        "        \n"
        "    if (LightType == 0)\n"
        "    {\n"
        "        LightVector = -LightDirection;\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        LightVector = LightPos - WorldPos;\n"
        "        DistanceToLightSquared = dot(LightVector, LightVector);\n"
        "        DistanceToLight = length(LightVector);\n"
        "        LightVector = normalize(LightVector);\n"
        "    }\n"
        "    \n"
        "    // Take N dot L as intensity.\n"
        "    float LightNDotL = dot(LightVector, WorldNrm);\n"
        "    float LightIntensity = max(0, LightNDotL);\n"
        "\n"
        "    float Attenuate = Shadow;\n"
        "    \n"
        "    if (LightType != 0)\n"
        "    {\n"
        "        // Attenuate Here\n"
        "        Attenuate = LightAttenuation.x +\n"
        "            LightAttenuation.y * DistanceToLight +\n"
        "            LightAttenuation.z * DistanceToLightSquared;\n"
        "        Attenuate = max(1.0, Attenuate);\n"
        "        Attenuate = 1.0 / Attenuate;\n"
        "        Attenuate *= Shadow;\n"
        "\n"
        "        if (LightType == 2)\n"
        "        {\n"
        "            // Get intensity as cosine of light vector and direction.\n"
        "            float CosAlpha = dot(-LightVector, LightDirection);\n"
        "\n"
        "            // Factor in inner and outer cone angles.\n"
        "            CosAlpha = saturate((CosAlpha - LightSpotAttenuation.y) / \n"
        "                (LightSpotAttenuation.x - LightSpotAttenuation.y));\n"
        "\n"
        "            // Power to falloff.\n"
        "            CosAlpha = pow(CosAlpha, LightSpotAttenuation.z);\n"
        "\n"
        "            // Multiply the spot attenuation into the overall attenuation.\n"
        "            Attenuate *= CosAlpha;\n"
        "        }\n"
        "\n"
        "    }\n"
        "    // Determine the interaction of diffuse color of light and material.\n"
        "    // Scale by the attenuated intensity.\n"
        "    DiffuseAccumOut = DiffuseAccum;\n"
        "    DiffuseAccumOut.rgb += LightDiffuse.rgb * LightIntensity * Attenuate;\n"
        "\n"
        "    // Determine ambient contribution - Is affected by shadow\n"
        "    AmbientAccumOut = AmbientAccum;\n"
        "    AmbientAccumOut.rgb += LightAmbient.rgb * Attenuate;\n"
        "\n"
        "    SpecularAccumOut = SpecularAccum;\n"
        "    if (SpecularEnable)\n"
        "    {\n"
        "        // Get the half vector.\n"
        "        float3 LightHalfVector = LightVector + WorldViewVector;\n"
        "        LightHalfVector = normalize(LightHalfVector);\n"
        "\n"
        "        // Determine specular intensity.\n"
        "        float LightNDotH = max(0, dot(WorldNrm, LightHalfVector));\n"
        "        float LightSpecIntensity = pow(LightNDotH, SpecularPower.x);\n"
        "        \n"
        "        //if (LightNDotL < 0.0)\n"
        "        //    LightSpecIntensity = 0.0;\n"
        "        // Must use the code below rather than code above.\n"
        "        // Using previous lines will cause the compiler to generate incorrect\n"
        "        // output.\n"
        "        float SpecularMultiplier = LightNDotL > 0.0 ? 1.0 : 0.0;\n"
        "        \n"
        "        // Attenuate Here\n"
        "        LightSpecIntensity = LightSpecIntensity * Attenuate * \n"
        "            SpecularMultiplier;\n"
        "        \n"
        "        // Determine the interaction of specular color of light and material.\n"
        "        // Scale by the attenuated intensity.\n"
        "        SpecularAccumOut.rgb += LightSpecIntensity * LightSpecular;\n"
        "    }       \n"
        "\n"
        "    \n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "ApplyFog");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the fog based on the \n"
        "    calculations in the vertex shader.\n"
        "    \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "UnfoggedColor");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "FogColor");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Fog");
    SetFixed(kResourceSetVariable, res, "FogAmount");
    SetFixed(kResourceSetDefaultValue, res, "(1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "FoggedColor");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    FoggedColor = lerp(FogColor, UnfoggedColor, FogAmount);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex");
    SetFixed(kNodeSetName, node, "ApplyAlphaTest");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for alpha testing based on the alpha\n"
        "    reference value and alpha test function.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Alpha");
    SetFixed(kResourceSetVariable, res, "AlphaTestValue");
    SetFixed(kResourceSetDefaultValue, res, "(1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Function");
    SetFixed(kResourceSetVariable, res, "AlphaTestFunction");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Fog");
    SetFixed(kResourceSetVariable, res, "AlphaTestRef");
    SetFixed(kResourceSetDefaultValue, res, "(0.0)");
    Call(kNodeAddInputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    // These tests will evaluate to less than 0 if they are true.\n"
        "    float fLessTest = (AlphaTestValue - AlphaTestRef);\n"
        "    float fGreaterTest = -fLessTest;\n"
        "    float fEqualTest = 0.0;\n"
        "    if (fLessTest == 0.0)\n"
        "        fEqualTest = -1.0;\n"
        "\n"
        "    // Clip if AlphaTestValue < AlphaTestRef for:\n"
        "    //  TEST_LESS \n"
        "    //  TEST_EQUAL \n"
        "    //  TEST_LESSEQUAL \n"
        "    clip(fGreaterTest * AlphaTestFunction.x);\n"
        "    \n"
        "    // Clip if AlphaTestValue > AlphaTestRef for:\n"
        "    //  TEST_EQUAL \n"
        "    //  TEST_GREATER \n"
        "    //  TEST_GREATEREQUAL \n"
        "    clip(fLessTest * AlphaTestFunction.y);\n"
        "    \n"
        "    // Clip if AlphaTestValue == AlphaTestRef for:\n"
        "    //  TEST_LESS \n"
        "    //  TEST_GREATER \n"
        "    //  TEST_NOTEQUAL \n"
        "    clip(fEqualTest * AlphaTestFunction.z);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "ComputeShadingCoefficients");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for computing the coefficients for the \n"
        "    following equations:\n"
        "    \n"
        "    Kdiffuse = MatEmissive + \n"
        "        MatAmbient * Summation(0...N){LightAmbientContribution[N]} + \n"
        "        MatDiffuse * Summation(0..N){LightDiffuseContribution[N]}\n"
        "        \n"
        "    Kspecular = MatSpecular * Summation(0..N){LightSpecularContribution[N]}\n"
        "    \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "MatEmissive");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "MatDiffuse");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "MatAmbient");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "MatSpecular");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "LightSpecularAccum");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "LightDiffuseAccum");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "LightAmbientAccum");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetVariable, res, "Saturate");
    SetFixed(kResourceSetDefaultValue, res, "(true)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Diffuse");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Specular");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Diffuse = MatEmissive + MatAmbient * LightAmbientAccum + \n"
        "        MatDiffuse * LightDiffuseAccum;\n"
        "    Specular = MatSpecular * LightSpecularAccum;\n"
        "    \n"
        "    if (Saturate)\n"
        "    {\n"
        "        Diffuse = saturate(Diffuse);\n"
        "        Specular = saturate(Specular);\n"
        "    }\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "SplitColorAndOpacity");
    SetString(kNodeSetDescription, node,
        "\n"
        "    Separate a float4 into a float3 and a float.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorAndOpacity");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Color");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Opacity");
    SetFixed(kResourceSetVariable, res, "Opacity");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Color.rgb = ColorAndOpacity.rgb;\n"
        "    Opacity = ColorAndOpacity.a;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "SplitRGBA");
    SetString(kNodeSetDescription, node,
        "\n"
        "    Separate a float4 into 4 floats.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorAndOpacity");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Red");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Green");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Blue");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Opacity");
    SetFixed(kResourceSetVariable, res, "Alpha");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Red = ColorAndOpacity.r;\n"
        "    Green = ColorAndOpacity.g;\n"
        "    Blue = ColorAndOpacity.b;\n"
        "    Alpha = ColorAndOpacity.a;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "CompositeFinalRGBColor");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for computing the final RGB color.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "DiffuseColor");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "SpecularColor");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "OutputColor");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    OutputColor.rgb = DiffuseColor.rgb + SpecularColor.rgb;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "CompositeFinalRGBAColor");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for computing the final RGBA color.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "FinalColor");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "FinalOpacity");
    SetFixed(kResourceSetDefaultValue, res, "(1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "OutputColor");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    OutputColor.rgb = FinalColor.rgb;\n"
        "    OutputColor.a = saturate(FinalOpacity);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "AddFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for adding two float4's. \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 + V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "AddFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for adding two float3's. \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 + V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "AddFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for adding two float2's. \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 + V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "AddFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for adding two floats. \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 + V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "MultiplyFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for multiplying two float4's. \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 * V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "MultiplyFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for multiplying two float3's. \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 * V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "MultiplyFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for multiplying two float2's. \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 * V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "MultiplyFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for multiplying two floats. \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 * V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "ScaleFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for scaling a float4 by a constant. \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Scale");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Scale * V1;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "ScaleFloat3");
    SetString(kNodeSetDescription, node,
        "    \n"
        "    This fragment is responsible for scaling a float3 by a constant. \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Scale");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Scale * V1;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "ScaleFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for scaling a float2 by a constant. \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Scale");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Scale * V1;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "SaturateFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for saturating a float4. \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = saturate(V1);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "SaturateFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for saturating a float3. \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = saturate(V1);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "SaturateFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for saturating a float2. \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = saturate(V1);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "SaturateFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for saturating a float. \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = saturate(V1);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "LerpFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for linearly interpolating two float4's. \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "LerpAmount");
    SetFixed(kResourceSetDefaultValue, res, "(0.5)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = lerp(V1, V2, LerpAmount);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "LerpFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for linearly interpolating two float3's. \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "LerpAmount");
    SetFixed(kResourceSetDefaultValue, res, "(0.5)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = lerp(V1, V2, LerpAmount);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "LerpFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for linearly interpolating two float2's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "LerpAmount");
    SetFixed(kResourceSetDefaultValue, res, "(0.5)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = lerp(V1, V2, LerpAmount);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "LerpFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for linearly interpolating two floats.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "LerpAmount");
    SetFixed(kResourceSetDefaultValue, res, "(0.5)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = lerp(V1, V2, LerpAmount);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "NormalizeFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for normalizing a float4.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "VectorIn");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "VectorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    VectorOut = normalize(VectorIn);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "NormalizeFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for normalizing a float3.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "VectorIn");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "VectorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    VectorOut = normalize(VectorIn);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "NormalizeFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for normalizing a float2.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "VectorIn");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "VectorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    VectorOut = normalize(VectorIn);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "MultiplyScalarSatAddFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for accumulating into the current color by\n"
        "    multiplying the sampled texture color with the current color. The \n"
        "    original input alpha channel is preserved. The scalar value is added to the\n"
        "    sampled color value and then clamped to the range [0.0, 1.0].\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "V1");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "V2");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Scalar");
    SetFixed(kResourceSetVariable, res, "Scalar");
    SetFixed(kResourceSetDefaultValue, res, "(0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 * saturate(V2.rgb + Scalar.rrr);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "MultiplyAddFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the equation:\n"
        "    Output = (V1 * V2) + V3\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "V1");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "V2");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Scalar");
    SetFixed(kResourceSetVariable, res, "V3");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = (V1 * V2) + V3;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "FloatToFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = float3(Input, Input, Input);\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.rrr;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "FloatToFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = float4(Input, Input, Input, Input);\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.rrrr;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "Float4ToFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = Input.r;\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.r;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "Float3ToFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = Input.r;\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.r;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "ShadowMap");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for calculating the Shadow coefficient.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "int");
    SetFixed(kResourceSetSemantic, res, "LightType");
    SetFixed(kResourceSetVariable, res, "LightType");
    SetFixed(kResourceSetDefaultValue, res, "(0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "sampler2D");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "ShadowMap");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "ShadowMapSize");
    SetFixed(kResourceSetDefaultValue, res, "(512,512)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetVariable, res, "WorldToLightProjMat");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "ShadowBias");
    SetFixed(kResourceSetDefaultValue, res, "(0.005)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Shadow");
    SetFixed(kResourceSetVariable, res, "ShadowOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    \n"
        "    if ( LightType != 0)\n"
        "    {\n"
        "      // This fragment only supports directional lights.\n"
        "        ShadowOut = 0.0;\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        float4 LightProjPos = mul(WorldPos, WorldToLightProjMat);\n"
        "        float2 ShadowTexC = 0.5 * LightProjPos.xy / LightProjPos.w + \n"
        "                  float2( 0.5, 0.5 );\n"
        "    \n"
        "        ShadowTexC.y = 1.0f - ShadowTexC.y;\n"
        "\n"
        "        float ShadowLookup = tex2D(ShadowMap, ShadowTexC.xy).r;\n"
        "        float LightSpaceDepth = (LightProjPos.z / LightProjPos.w) ;\n"
        "        float2 borderTest = saturate(ShadowTexC) - ShadowTexC;\n"
        "\n"
        "        if ( (ShadowLookup > LightSpaceDepth - ShadowBias) || \n"
        "            any(borderTest))\n"
        "        {\n"
        "            ShadowOut = 1.0;\n"
        "        }\n"
        "        else\n"
        "        {\n"
        "            ShadowOut = 0.0;\n"
        "        }\n"
        "        \n"
        "       }\n"
        "\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "VSMShadowMap");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for calculating the Shadow coefficient.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "int");
    SetFixed(kResourceSetSemantic, res, "LightType");
    SetFixed(kResourceSetVariable, res, "LightType");
    SetFixed(kResourceSetDefaultValue, res, "(0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "sampler2D");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "ShadowMap");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "ShadowMapSize");
    SetFixed(kResourceSetDefaultValue, res, "(512,512)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetVariable, res, "WorldToLightProjMat");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "ShadowBias");
    SetFixed(kResourceSetDefaultValue, res, "(0.005)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "ShadowVSMPowerEpsilon");
    SetFixed(kResourceSetDefaultValue, res, "(10.0, 0.001)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Shadow");
    SetFixed(kResourceSetVariable, res, "ShadowOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    \n"
        "    if ( LightType != 0)\n"
        "    {\n"
        "      // This fragment only supports directional lights.\n"
        "        ShadowOut = 0.0;\n"
        "    }\n"
        "    else\n"
        "    { \n"
        "        float4 LightProjPos = mul(WorldPos, WorldToLightProjMat);\n"
        "        float2 ShadowTexC = 0.5 * LightProjPos.xy / LightProjPos.w + \n"
        "            float2( 0.5, 0.5 );\n"
        "        ShadowTexC.y = 1.0f - ShadowTexC.y;\n"
        "        \n"
        "        float LightSpaceDepth = (LightProjPos.z / LightProjPos.w);\n"
        "        float2 borderTest = saturate(ShadowTexC) - ShadowTexC;\n"
        "\n"
        "        float4 vVSM   = tex2D( ShadowMap, ShadowTexC.xy );\n"
        "        float  fAvgZ  = vVSM.r; // Filtered z\n"
        "        float  fAvgZ2 = vVSM.g; // Filtered z-squared\n"
        "        \n"
        "        // Standard shadow map comparison\n"
        "        if((LightSpaceDepth) - ShadowBias <= fAvgZ || any(borderTest))\n"
        "        {\n"
        "            ShadowOut = 1.0f;\n"
        "        }\n"
        "        else\n"
        "        {            \n"
        "            // Use variance shadow mapping to compute the maximum probability \n"
        "            // that the pixel is in shadow\n"
        "            float variance = ( fAvgZ2 ) - ( fAvgZ * fAvgZ );\n"
        "            variance       = \n"
        "                min( 1.0f, max( 0.0f, variance + ShadowVSMPowerEpsilon.y ) );\n"
        "            ShadowOut = variance;\n"
        "            \n"
        "            float mean     = fAvgZ;\n"
        "            float d        = LightSpaceDepth - mean;\n"
        "            float p_max    = variance / ( variance + d*d );\n"
        "            \n"
        "            ShadowOut = pow( p_max, ShadowVSMPowerEpsilon.x);\n"
        "        }\n"
        "    }\n"
        "\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "PCFShadowMap");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for calculating the Shadow coefficient.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "int");
    SetFixed(kResourceSetSemantic, res, "LightType");
    SetFixed(kResourceSetVariable, res, "LightType");
    SetFixed(kResourceSetDefaultValue, res, "(0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "sampler2D");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "ShadowMap");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "ShadowMapSize");
    SetFixed(kResourceSetDefaultValue, res, "(512,512)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetVariable, res, "WorldToLightProjMat");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "ShadowBias");
    SetFixed(kResourceSetDefaultValue, res, "(0.005)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Shadow");
    SetFixed(kResourceSetVariable, res, "ShadowOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "DX9");
    SetFixed(kBlockSetTarget, block, "ps_2_0");
    SetString(kBlockSetText, block,
        "\n"
        "    \n"
        "    if ( LightType != 0)\n"
        "    {\n"
        "      // This fragment only supports directional lights.\n"
        "        ShadowOut = 0.0;\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        float4 LightProjPos = mul(WorldPos, WorldToLightProjMat);\n"
        "        float2 ShadowTexC = 0.5 * LightProjPos.xy / LightProjPos.w + \n"
        "                  float2( 0.5, 0.5 );\n"
        "    \n"
        "        ShadowTexC.y = 1.0f - ShadowTexC.y;\n"
        "    \n"
        "        float ShadowMapSizeInverse = 1.0f / ShadowMapSize;\n"
        "        float2 lerps = frac( ShadowTexC * ShadowMapSize);\n"
        "        float LightSpaceDepth = (LightProjPos.z / LightProjPos.w);\n"
        "        float2 borderTest = saturate(ShadowTexC) - ShadowTexC;\n"
        "\n"
        "        if (any(borderTest))\n"
        "        {\n"
        "            ShadowOut = 1.0;\n"
        "        }\n"
        "        else\n"
        "        {\n"
        "            LightSpaceDepth -= ShadowBias;\n"
        "                   \n"
        "            float4 SourceVals;\n"
        "            SourceVals.x = tex2D( ShadowMap, ShadowTexC ).r;\n"
        "    \n"
        "            ShadowTexC.x += ShadowMapSizeInverse;\n"
        "            SourceVals.y = tex2D( ShadowMap, ShadowTexC).r;  \n"
        "    \n"
        "            ShadowTexC.y += ShadowMapSizeInverse;                \n"
        "            SourceVals.w = tex2D( ShadowMap, ShadowTexC ).r;\n"
        "                \n"
        "            ShadowTexC.x -= ShadowMapSizeInverse;                \n"
        "            SourceVals.z = tex2D( ShadowMap, ShadowTexC).r;\n"
        "                \n"
        "            float4 Shade = (LightSpaceDepth < SourceVals);\n"
        "    \n"
        "            // lerp between the shadow values to calculate our light amount\n"
        "            ShadowOut = lerp(\n"
        "                lerp(Shade.x, Shade.y, lerps.x),\n"
        "                lerp(Shade.z, Shade.w, lerps.x), lerps.y );        \n"
        "        }\n"
        "    }\n"
        "\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "DX9/D3D10/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_3_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    \n"
        "    if ( LightType != 0)\n"
        "    {\n"
        "      // This fragment only supports directional lights.\n"
        "        ShadowOut = 0.0;\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        float4 LightProjPos = mul(WorldPos, WorldToLightProjMat);\n"
        "        float2 ShadowTexC = 0.5 * LightProjPos.xy / LightProjPos.w + \n"
        "                  float2( 0.5, 0.5 );\n"
        "        \n"
        "        ShadowTexC.y = 1.0f - ShadowTexC.y;\n"
        "        \n"
        "        float ShadowMapSizeInverse = 1.0f / ShadowMapSize;\n"
        "        float LightSpaceDepth = (LightProjPos.z / LightProjPos.w);\n"
        "        float2 borderTest = saturate(ShadowTexC) - ShadowTexC;\n"
        "\n"
        "        if (any(borderTest))\n"
        "        {\n"
        "            ShadowOut = 1.0;\n"
        "        }\n"
        "        else\n"
        "        {    \n"
        "            LightSpaceDepth -= ShadowBias;\n"
        "        \n"
        "            ShadowOut = 0;\n"
        "            float4 fOnes = float4(1.0, 1.0, 1.0, 1.0);\n"
        "            float4 fKernels = float4(1.0, 1.0, 1.0, 1.0);\n"
        "            float faKernels[4] = {1.0, 1.0, 1.0, 1.0};\n"
        "            \n"
        "            faKernels[0] = 1-frac(ShadowTexC.y * ShadowMapSize.y);\n"
        "            faKernels[3]= frac(ShadowTexC.y  * ShadowMapSize.y); \n"
        "            \n"
        "            fKernels.x = 1-frac(ShadowTexC.x * ShadowMapSize.x);\n"
        "            fKernels.w = frac(ShadowTexC.x  * ShadowMapSize.x);\n"
        "                      \n"
        "            float fTotalPercent = 0;\n"
        "            \n"
        "            // This loop is manually unrolled here to avoid long\n"
        "            // shader compilation times.\n"
        "            //for (int i=0; i < 4; i++)\n"
        "            // i == 0\n"
        "            {\n"
        "                float4 shadowMapDepth = 0;\n"
        "                float2 pos = ShadowTexC;                \n"
        "                shadowMapDepth.x = tex2D(ShadowMap, pos).x; \n"
        "                pos.x += ShadowMapSizeInverse;\n"
        "                shadowMapDepth.y = tex2D(ShadowMap, pos).x;\n"
        "                pos.x += ShadowMapSizeInverse;        \n"
        "                shadowMapDepth.z = tex2D(ShadowMap, pos).x;\n"
        "                pos.x += ShadowMapSizeInverse;                \n"
        "                shadowMapDepth.w = tex2D(ShadowMap, pos).x;\n"
        "                \n"
        "                float4 shad = (LightSpaceDepth < shadowMapDepth);\n"
        "    \n"
        "                ShadowOut += dot(shad, fKernels) * faKernels[0];\n"
        "                fTotalPercent += dot(fOnes, fKernels) * faKernels[0];\n"
        "            }\n"
        "            // i == 1\n"
        "            {\n"
        "                float4 shadowMapDepth = 0;\n"
        "                float2 pos = ShadowTexC;                \n"
        "                pos.y += ShadowMapSizeInverse;\n"
        "                shadowMapDepth.x = tex2D(ShadowMap, pos).x; \n"
        "                pos.x += ShadowMapSizeInverse;\n"
        "                shadowMapDepth.y = tex2D(ShadowMap, pos).x;\n"
        "                pos.x += ShadowMapSizeInverse;        \n"
        "                shadowMapDepth.z = tex2D(ShadowMap, pos).x;\n"
        "                pos.x += ShadowMapSizeInverse;                \n"
        "                shadowMapDepth.w = tex2D(ShadowMap, pos).x;\n"
        "                \n"
        "                float4 shad = (LightSpaceDepth < shadowMapDepth);\n"
        "    \n"
        "                ShadowOut += dot(shad, fKernels) * faKernels[1];\n"
        "                fTotalPercent += dot(fOnes, fKernels) * faKernels[1];\n"
        "            }\n"
        "            // i == 2\n"
        "            {\n"
        "                float4 shadowMapDepth = 0;\n"
        "                float2 pos = ShadowTexC;                \n"
        "                pos.y += 2 * ShadowMapSizeInverse;\n"
        "                shadowMapDepth.x = tex2D(ShadowMap, pos).x; \n"
        "                pos.x += ShadowMapSizeInverse;\n"
        "                shadowMapDepth.y = tex2D(ShadowMap, pos).x;\n"
        "                pos.x += ShadowMapSizeInverse;        \n"
        "                shadowMapDepth.z = tex2D(ShadowMap, pos).x;\n"
        "                pos.x += ShadowMapSizeInverse;                \n"
        "                shadowMapDepth.w = tex2D(ShadowMap, pos).x;\n"
        "                \n"
        "                float4 shad = (LightSpaceDepth < shadowMapDepth);\n"
        "    \n"
        "                ShadowOut += dot(shad, fKernels) * faKernels[2];\n"
        "                fTotalPercent += dot(fOnes, fKernels) * faKernels[2];\n"
        "            }\n"
        "            // i == 3\n"
        "            {\n"
        "                float4 shadowMapDepth = 0;\n"
        "                float2 pos = ShadowTexC;                \n"
        "                pos.y += 3 * ShadowMapSizeInverse;\n"
        "                shadowMapDepth.x = tex2D(ShadowMap, pos).x; \n"
        "                pos.x += ShadowMapSizeInverse;\n"
        "                shadowMapDepth.y = tex2D(ShadowMap, pos).x;\n"
        "                pos.x += ShadowMapSizeInverse;        \n"
        "                shadowMapDepth.z = tex2D(ShadowMap, pos).x;\n"
        "                pos.x += ShadowMapSizeInverse;                \n"
        "                shadowMapDepth.w = tex2D(ShadowMap, pos).x;\n"
        "                \n"
        "                float4 shad = (LightSpaceDepth < shadowMapDepth);\n"
        "    \n"
        "                ShadowOut += dot(shad, fKernels) * faKernels[3];\n"
        "                fTotalPercent += dot(fOnes, fKernels) * faKernels[3];\n"
        "            }\n"
        "            \n"
        "            ShadowOut = ShadowOut / fTotalPercent;         \n"
        "           }\n"
        "       }\n"
        "\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "SpotShadowMap");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for calculating the Shadow coefficient.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "int");
    SetFixed(kResourceSetSemantic, res, "LightType");
    SetFixed(kResourceSetVariable, res, "LightType");
    SetFixed(kResourceSetDefaultValue, res, "(0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "sampler2D");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "ShadowMap");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "ShadowMapSize");
    SetFixed(kResourceSetDefaultValue, res, "(512,512)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "LightDirection");
    SetFixed(kResourceSetVariable, res, "LightDirection");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Light");
    SetFixed(kResourceSetVariable, res, "LightPos");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "CosOfCutoff");
    SetFixed(kResourceSetDefaultValue, res, "(0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetVariable, res, "WorldToLightProjMat");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "ShadowBias");
    SetFixed(kResourceSetDefaultValue, res, "(0.0005)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Shadow");
    SetFixed(kResourceSetVariable, res, "ShadowOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    if (LightType != 2)\n"
        "    {\n"
        "        // This version only supports spotlights.\n"
        "        ShadowOut = 1.0;\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        float3 LightToWorldPos = normalize(float3(WorldPos - LightPos));\n"
        "\n"
        "        if (dot(LightDirection, LightToWorldPos) > CosOfCutoff)\n"
        "        {\n"
        "            float4 LightProjPos = mul(WorldPos, WorldToLightProjMat);\n"
        "\n"
        "            float3 ShadowTexC = LightProjPos.xyz / LightProjPos.w;               \n"
        "            ShadowTexC.xy =( 0.5 * ShadowTexC.xy) + float2( 0.5, 0.5 );\n"
        "            ShadowTexC.y = 1.0f - ShadowTexC.y;\n"
        "\n"
        "            float LightSpaceDepth = clamp(ShadowTexC.z - ShadowBias, \n"
        "                0.0, 1.0);\n"
        "            float ShadowLookup = tex2D(ShadowMap, ShadowTexC.xy).r;                \n"
        "            ShadowOut = ShadowLookup < LightSpaceDepth ? 0.0 : 1.0;\n"
        "        }\n"
        "        else\n"
        "        {\n"
        "            // Outside of the light cone is shadowed completely\n"
        "            ShadowOut = 0.0;\n"
        "        }\n"
        "    }\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "SpotVSMShadowMap");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for calculating the Shadow coefficient.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "int");
    SetFixed(kResourceSetSemantic, res, "LightType");
    SetFixed(kResourceSetVariable, res, "LightType");
    SetFixed(kResourceSetDefaultValue, res, "(0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "sampler2D");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "ShadowMap");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "ShadowMapSize");
    SetFixed(kResourceSetDefaultValue, res, "(512,512)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "LightDirection");
    SetFixed(kResourceSetVariable, res, "LightDirection");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Light");
    SetFixed(kResourceSetVariable, res, "LightPos");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "CosOfCutoff");
    SetFixed(kResourceSetDefaultValue, res, "(0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetVariable, res, "WorldToLightProjMat");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "ShadowBias");
    SetFixed(kResourceSetDefaultValue, res, "(0.0005)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "ShadowVSMPowerEpsilon");
    SetFixed(kResourceSetDefaultValue, res, "(10.0, 0.001)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Shadow");
    SetFixed(kResourceSetVariable, res, "ShadowOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    if (LightType != 2)\n"
        "    {\n"
        "        // This version only supports spotlights.\n"
        "        ShadowOut = 1.0;\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        float3 LightToWorldPos = normalize(float3(WorldPos - LightPos));\n"
        "\n"
        "        if (dot(LightDirection, LightToWorldPos) > CosOfCutoff)\n"
        "        {\n"
        "            float4 LightProjPos = mul(WorldPos, WorldToLightProjMat);\n"
        "            float3 ShadowTexC = LightProjPos.xyz / LightProjPos.w;               \n"
        "            ShadowTexC.xy =( 0.5 * ShadowTexC.xy) + float2( 0.5, 0.5 );\n"
        "            ShadowTexC.y = 1.0f - ShadowTexC.y;\n"
        "            float LightSpaceDepth = clamp(ShadowTexC.z - ShadowBias, \n"
        "                0.0, 1.0);\n"
        "                                                        \n"
        "            float4 vVSM   = tex2D( ShadowMap, ShadowTexC.xy );\n"
        "            float  fAvgZ  = vVSM.r; // Filtered z\n"
        "            float  fAvgZ2 = vVSM.g; // Filtered z-squared\n"
        "            \n"
        "            // Standard shadow map comparison\n"
        "            if( (LightSpaceDepth - ShadowBias) <= fAvgZ)\n"
        "            {\n"
        "                ShadowOut = 1.0f;\n"
        "            }\n"
        "            else\n"
        "            {               \n"
        "                // Use variance shadow mapping to compute the maximum \n"
        "                // probability that the pixel is in shadow\n"
        "                float variance = ( fAvgZ2 ) - ( fAvgZ * fAvgZ );\n"
        "                variance       = \n"
        "                    min( 1.0f, max( 0.0f, variance + ShadowVSMPowerEpsilon.y));\n"
        "                ShadowOut = variance;\n"
        "                \n"
        "                float mean     = fAvgZ;\n"
        "                float d        = LightSpaceDepth - mean;\n"
        "                float p_max    = variance / ( variance + d*d );\n"
        "                \n"
        "                ShadowOut = pow( p_max, ShadowVSMPowerEpsilon.x);\n"
        "            }\n"
        "\n"
        "        }\n"
        "        else\n"
        "        {\n"
        "            // Outside of the light cone is shadowed completely\n"
        "            ShadowOut = 0.0;\n"
        "        }\n"
        "    }\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "ShadowCubeMap");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for calculating the Shadow coefficient.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "int");
    SetFixed(kResourceSetSemantic, res, "LightType");
    SetFixed(kResourceSetVariable, res, "LightType");
    SetFixed(kResourceSetDefaultValue, res, "(0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "samplerCUBE");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "ShadowMap");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "ShadowMapSize");
    SetFixed(kResourceSetDefaultValue, res, "(512,512)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Light");
    SetFixed(kResourceSetVariable, res, "LightPos");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "ShadowBias");
    SetFixed(kResourceSetDefaultValue, res, "(0.0005)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Shadow");
    SetFixed(kResourceSetVariable, res, "ShadowOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    if (LightType == 0 || LightType == 2)\n"
        "    {\n"
        "        // This version only supports point lights.\n"
        "        ShadowOut = 1.0;\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        float3 ViewVector = WorldPos - LightPos;    \n"
        "        ViewVector.z = -ViewVector.z;\n"
        "        float3 ViewVectorNrm = normalize(ViewVector);\n"
        "\n"
        "        float fDepth = dot(ViewVector, ViewVector);\n"
        "        fDepth = sqrt(fDepth) * ShadowBias;\n"
        "        float ShadowLookup = texCUBE(ShadowMap, ViewVectorNrm).r;\n"
        "         \n"
        "        if (ShadowLookup == 0 || ShadowLookup > fDepth )\n"
        "            ShadowOut = 1.0;\n"
        "        else\n"
        "            ShadowOut = 0.0;\n"
        "    }\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "SpotPCFShadowMap");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for calculating the Shadow coefficient.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "int");
    SetFixed(kResourceSetSemantic, res, "LightType");
    SetFixed(kResourceSetVariable, res, "LightType");
    SetFixed(kResourceSetDefaultValue, res, "(0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "sampler2D");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "ShadowMap");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "ShadowMapSize");
    SetFixed(kResourceSetDefaultValue, res, "(512,512)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "LightDirection");
    SetFixed(kResourceSetVariable, res, "LightDirection");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Light");
    SetFixed(kResourceSetVariable, res, "LightPos");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "CosOfCutoff");
    SetFixed(kResourceSetDefaultValue, res, "(0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetVariable, res, "WorldToLightProjMat");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "ShadowBias");
    SetFixed(kResourceSetDefaultValue, res, "(0.0005)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Shadow");
    SetFixed(kResourceSetVariable, res, "ShadowOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "DX9");
    SetFixed(kBlockSetTarget, block, "ps_2_0");
    SetString(kBlockSetText, block,
        "\n"
        "    if (LightType != 2)\n"
        "    {\n"
        "        // This version only supports spotlights.\n"
        "        ShadowOut = 1.0;\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        float3 LightToWorldPos = normalize(float3(WorldPos - LightPos));\n"
        "\n"
        "        if (dot(LightDirection, LightToWorldPos) > CosOfCutoff)\n"
        "        {\n"
        "            float4 LightProjPos = mul(WorldPos, WorldToLightProjMat);\n"
        "            float3 ShadowTexC = LightProjPos.xyz / LightProjPos.w;               \n"
        "            ShadowTexC.xy =( 0.5 * ShadowTexC.xy) + float2( 0.5, 0.5 );\n"
        "            ShadowTexC.y = 1.0f - ShadowTexC.y;\n"
        "            float LightSpaceDepth = clamp(ShadowTexC.z - ShadowBias, \n"
        "                0.0, 1.0);\n"
        "            float2 ShadowMapSizeInverse = 1.0 / ShadowMapSize;\n"
        "            float2 lerps = frac( ShadowTexC.xy * ShadowMapSize);\n"
        "\n"
        "            float4 SourceVals;\n"
        "            SourceVals.x = tex2D( ShadowMap, ShadowTexC.xy ).r;\n"
        "\n"
        "            ShadowTexC.x += ShadowMapSizeInverse;\n"
        "            SourceVals.y = tex2D( ShadowMap, ShadowTexC.xy).r;  \n"
        "\n"
        "            ShadowTexC.y += ShadowMapSizeInverse;                \n"
        "            SourceVals.w = tex2D( ShadowMap, ShadowTexC.xy).r;\n"
        "                \n"
        "            ShadowTexC.x -= ShadowMapSizeInverse;                \n"
        "            SourceVals.z = tex2D( ShadowMap, ShadowTexC.xy).r;\n"
        "                \n"
        "            float4 Shade = (LightSpaceDepth <= SourceVals);\n"
        "\n"
        "            // lerp between the shadow values to calculate our light amount\n"
        "            ShadowOut = lerp(\n"
        "                lerp(Shade.x, Shade.y, lerps.x),\n"
        "                lerp(Shade.z, Shade.w, lerps.x), lerps.y );\n"
        "        }\n"
        "        else\n"
        "        {\n"
        "            // Outside of the light cone is shadowed completely\n"
        "            ShadowOut = 0.0;\n"
        "        }\n"
        "    }    \n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "DX9/D3D10/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_3_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    if (LightType != 2)\n"
        "    {\n"
        "        // This version only supports spotlights.\n"
        "        ShadowOut = 1.0;\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        float3 LightToWorldPos = normalize(float3(WorldPos - LightPos));\n"
        "\n"
        "        if (dot(LightDirection, LightToWorldPos) > CosOfCutoff)\n"
        "        {\n"
        "            float4 LightProjPos = mul(WorldPos, WorldToLightProjMat);\n"
        "            float3 ShadowTexC = LightProjPos.xyz / LightProjPos.w;               \n"
        "            ShadowTexC.xy =( 0.5 * ShadowTexC.xy) + float2( 0.5, 0.5 );\n"
        "            ShadowTexC.y = 1.0f - ShadowTexC.y;\n"
        "\n"
        "            float LightSpaceDepth = clamp(ShadowTexC.z - ShadowBias, \n"
        "                0.0, 1.0);\n"
        "            \n"
        "            float2 ShadowMapSizeInverse = 1.0 / ShadowMapSize;\n"
        "\n"
        "            ShadowTexC.xy = ShadowTexC.xy - ShadowMapSizeInverse;\n"
        "               \n"
        "            ShadowOut = 0;\n"
        "            float4 fOnes = float4(1.0, 1.0, 1.0, 1.0);\n"
        "            float4 fKernels = float4(1.0, 1.0, 1.0, 1.0);\n"
        "            float faKernels[4] = {1.0, 1.0, 1.0, 1.0};\n"
        "           \n"
        "            faKernels[0] = 1-frac(ShadowTexC.y * ShadowMapSize.y);\n"
        "            faKernels[3]= frac(ShadowTexC.y  * ShadowMapSize.y); \n"
        "           \n"
        "            fKernels.x = 1-frac(ShadowTexC.x * ShadowMapSize.x);\n"
        "            fKernels.w = frac(ShadowTexC.x  * ShadowMapSize.x);\n"
        "                              \n"
        "            float fTotalPercent = 0;\n"
        "            \n"
        "            // This loop is manually unrolled here to avoid long\n"
        "            // shader compilation times.            \n"
        "            //for (int i=0; i < 4; i++)\n"
        "            // i == 0\n"
        "            {\n"
        "                float4 shadowMapDepth = 0;\n"
        "                float2 pos = ShadowTexC.xy;\n"
        "                shadowMapDepth.x = tex2D(ShadowMap, pos).x; \n"
        "                pos.x += ShadowMapSizeInverse;\n"
        "                shadowMapDepth.y = tex2D(ShadowMap, pos).x;\n"
        "                pos.x += ShadowMapSizeInverse;        \n"
        "                shadowMapDepth.z = tex2D(ShadowMap, pos).x;\n"
        "                pos.x += ShadowMapSizeInverse;                \n"
        "                shadowMapDepth.w = tex2D(ShadowMap, pos).x;\n"
        "                \n"
        "                float4 shad = (LightSpaceDepth <= shadowMapDepth);\n"
        "                    \n"
        "                ShadowOut += dot(shad, fKernels) * faKernels[0];\n"
        "                fTotalPercent += dot(fOnes, fKernels) * faKernels[0];\n"
        "            }\n"
        "            // i == 1\n"
        "            {\n"
        "                float4 shadowMapDepth = 0;\n"
        "                float2 pos = ShadowTexC.xy;\n"
        "                pos.y += ShadowMapSizeInverse;\n"
        "                shadowMapDepth.x = tex2D(ShadowMap, pos).x; \n"
        "                pos.x += ShadowMapSizeInverse;\n"
        "                shadowMapDepth.y = tex2D(ShadowMap, pos).x;\n"
        "                pos.x += ShadowMapSizeInverse;        \n"
        "                shadowMapDepth.z = tex2D(ShadowMap, pos).x;\n"
        "                pos.x += ShadowMapSizeInverse;                \n"
        "                shadowMapDepth.w = tex2D(ShadowMap, pos).x;\n"
        "                \n"
        "                float4 shad = (LightSpaceDepth <= shadowMapDepth);\n"
        "                    \n"
        "                ShadowOut += dot(shad, fKernels) * faKernels[1];\n"
        "                fTotalPercent += dot(fOnes, fKernels) * faKernels[1];\n"
        "            }\n"
        "            // i == 2\n"
        "            {\n"
        "                float4 shadowMapDepth = 0;\n"
        "                float2 pos = ShadowTexC.xy;\n"
        "                pos.y += 2 * ShadowMapSizeInverse;\n"
        "                shadowMapDepth.x = tex2D(ShadowMap, pos).x; \n"
        "                pos.x += ShadowMapSizeInverse;\n"
        "                shadowMapDepth.y = tex2D(ShadowMap, pos).x;\n"
        "                pos.x += ShadowMapSizeInverse;        \n"
        "                shadowMapDepth.z = tex2D(ShadowMap, pos).x;\n"
        "                pos.x += ShadowMapSizeInverse;                \n"
        "                shadowMapDepth.w = tex2D(ShadowMap, pos).x;\n"
        "                \n"
        "                float4 shad = (LightSpaceDepth <= shadowMapDepth);\n"
        "                    \n"
        "                ShadowOut += dot(shad, fKernels) * faKernels[2];\n"
        "                fTotalPercent += dot(fOnes, fKernels) * faKernels[2];\n"
        "            }\n"
        "            // i == 3\n"
        "            {\n"
        "                float4 shadowMapDepth = 0;\n"
        "                float2 pos = ShadowTexC.xy;\n"
        "                pos.y += 3 * ShadowMapSizeInverse;\n"
        "                shadowMapDepth.x = tex2D(ShadowMap, pos).x; \n"
        "                pos.x += ShadowMapSizeInverse;\n"
        "                shadowMapDepth.y = tex2D(ShadowMap, pos).x;\n"
        "                pos.x += ShadowMapSizeInverse;        \n"
        "                shadowMapDepth.z = tex2D(ShadowMap, pos).x;\n"
        "                pos.x += ShadowMapSizeInverse;                \n"
        "                shadowMapDepth.w = tex2D(ShadowMap, pos).x;\n"
        "                \n"
        "                float4 shad = (LightSpaceDepth <= shadowMapDepth);\n"
        "                    \n"
        "                ShadowOut += dot(shad, fKernels) * faKernels[3];\n"
        "                fTotalPercent += dot(fOnes, fKernels) * faKernels[3];\n"
        "            }\n"
        "           \n"
        "            ShadowOut = ShadowOut / fTotalPercent;         \n"
        "        }\n"
        "        else\n"
        "        {\n"
        "            // Outside of the light cone is shadowed completely\n"
        "            ShadowOut = 0.0;\n"
        "        }\n"
        "    }    \n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    return reinterpret_cast<NiRefObject*>(lib);
}

// 0x7B06F0: the point light shadow-map writing material's node library
NiRefObject* __cdecl CreatePointShadowWriteLibrary() {
    uint32_t lib, node, res, block;
    lib = New(0x1C, kNodeLibraryConstruct, 1u);
    node = NewFragmentNodeInline();
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "TransformPosition");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the view projection transform\n"
        "    to the input position. Additionally, this fragment applies the world \n"
        "    transform to the input position. \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "Position");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "World");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    // Transform the position into world space for lighting, and projected \n"
        "    // space for display\n"
        "    WorldPos = mul( float4(Position, 1.0f), World );\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "TransformSkinnedPosition");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the view projection and skinning \n"
        "    transform to the input position. Additionally, this fragment applies the \n"
        "    computed world transform to the input position. The weighted world \n"
        "    transform defined by the blendweights is output for use in normals or\n"
        "    other calculations as the new world matrix.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "Position");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "int4");
    SetFixed(kResourceSetSemantic, res, "BlendIndices");
    SetFixed(kResourceSetVariable, res, "BlendIndices");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "BlendWeight");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "BlendWeights");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x3");
    Call(kResourceSetCount, res, 0x1Eu);
    SetFixed(kResourceSetSemantic, res, "SkinBoneMatrix");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "Bones");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "SkinToWorldTransform");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "SkinBoneTransform");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    // TransformSkinnedPosition *********************************************\n"
        "    // Transform the skinned position into world space\n"
        "    // Composite the skinning transform which will take the vertex\n"
        "    // and normal to world space.\n"
        "    float fWeight3 = 1.0 - BlendWeights[0] - BlendWeights[1] - BlendWeights[2];\n"
        "    float4x3 ShortSkinBoneTransform;\n"
        "    ShortSkinBoneTransform  = Bones[BlendIndices[0]] * BlendWeights[0];\n"
        "    ShortSkinBoneTransform += Bones[BlendIndices[1]] * BlendWeights[1];\n"
        "    ShortSkinBoneTransform += Bones[BlendIndices[2]] * BlendWeights[2];\n"
        "    ShortSkinBoneTransform += Bones[BlendIndices[3]] * fWeight3;\n"
        "    SkinBoneTransform = float4x4(ShortSkinBoneTransform[0], 0.0f, \n"
        "        ShortSkinBoneTransform[1], 0.0f, \n"
        "        ShortSkinBoneTransform[2], 0.0f, \n"
        "        ShortSkinBoneTransform[3], 1.0f);\n"
        "\n"
        "    // Transform into world space.\n"
        "    WorldPos.xyz = mul(float4(Position, 1.0), ShortSkinBoneTransform);\n"
        "    WorldPos = mul(float4(WorldPos.xyz, 1.0), SkinToWorldTransform);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "TransformSkinnedPositionNoIndices");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the view projection and skinning \n"
        "    transform to the input position. Additionally, this fragment applies the \n"
        "    computed world transform to the input position. The weighted world \n"
        "    transform defined by the blendweights is output for use in normals or\n"
        "    other calculations as the new world matrix.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "Position");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "BlendWeight");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "BlendWeights");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x3");
    Call(kResourceSetCount, res, 4u);
    SetFixed(kResourceSetSemantic, res, "SkinBoneMatrix");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "Bones");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "SkinToWorldTransform");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "SkinBoneTransform");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    // TransformSkinnedPosition *********************************************\n"
        "    // Transform the skinned position into world space\n"
        "    // Composite the skinning transform which will take the vertex\n"
        "    // and normal to world space.\n"
        "    float fWeight3 = 1.0 - BlendWeights[0] - BlendWeights[1] - BlendWeights[2];\n"
        "    float4x3 ShortSkinBoneTransform;\n"
        "    ShortSkinBoneTransform  = Bones[0] * BlendWeights[0];\n"
        "    ShortSkinBoneTransform += Bones[1] * BlendWeights[1];\n"
        "    ShortSkinBoneTransform += Bones[2] * BlendWeights[2];\n"
        "    ShortSkinBoneTransform += Bones[3] * fWeight3;\n"
        "    SkinBoneTransform = float4x4(ShortSkinBoneTransform[0], 0.0f, \n"
        "        ShortSkinBoneTransform[1], 0.0f, \n"
        "        ShortSkinBoneTransform[2], 0.0f, \n"
        "        ShortSkinBoneTransform[3], 1.0f);\n"
        "\n"
        "    // Transform into world space.\n"
        "    WorldPos.xyz = mul(float4(Position, 1.0), ShortSkinBoneTransform);\n"
        "    WorldPos = mul(float4(WorldPos.xyz, 1.0), SkinToWorldTransform);\n"
        "\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "ProjectPositionWorldToProj");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the view projection transform\n"
        "    to the input world position.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPosition");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "ViewProjMatrix");
    SetFixed(kResourceSetVariable, res, "ViewProjection");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Proj");
    SetFixed(kResourceSetVariable, res, "ProjPos");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    ProjPos = mul(WorldPosition, ViewProjection );\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "ProjectPositionWorldToViewToProj");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the view projection transform\n"
        "    to the input world position.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPosition");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "ViewMatrix");
    SetFixed(kResourceSetVariable, res, "ViewTransform");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "ProjMatrix");
    SetFixed(kResourceSetVariable, res, "ProjTransform");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "View");
    SetFixed(kResourceSetVariable, res, "ViewPos");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Proj");
    SetFixed(kResourceSetVariable, res, "ProjPos");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    ViewPos = mul(WorldPosition, ViewTransform );\n"
        "    ProjPos = mul(ViewPos, ProjTransform );\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "CalculateViewVector");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for calculating the camera view vector.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "CameraPosition");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "CameraPos");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "ViewVector");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldViewVector");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    WorldViewVector = CameraPos - WorldPos;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "AddFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for adding two float4's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 + V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "AddFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for adding two float3's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 + V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "AddFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for adding two float2's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 + V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "AddFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for adding two floats.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 + V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "MultiplyFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for multiplying two float4's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 * V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "MultiplyFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for multiplying two float3's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 * V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "MultiplyFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for multiplying two float2's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 * V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "MultiplyFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for multiplying two floats.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 * V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "ScaleFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for scaling a float4 by a constant.     \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Scale");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Scale * V1;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "ScaleFloat3");
    SetString(kNodeSetDescription, node,
        "    \n"
        "    This fragment is responsible for scaling a float3 by a constant.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Scale");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Scale * V1;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "ScaleFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for scaling a float2 by a constant.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Scale");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Scale * V1;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "SaturateFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for saturating a float4.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = saturate(V1);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "SaturateFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for saturating a float3.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = saturate(V1);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "SaturateFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for saturating a float2.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = saturate(V1);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "SaturateFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for saturating a float.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = saturate(V1);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "LerpFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for linearly interpolating two float4's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "LerpAmount");
    SetFixed(kResourceSetDefaultValue, res, "(0.5)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = lerp(V1, V2, LerpAmount);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "LerpFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for linearly interpolating two float3's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "LerpAmount");
    SetFixed(kResourceSetDefaultValue, res, "(0.5)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = lerp(V1, V2, LerpAmount);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "LerpFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for linearly interpolating two float2's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "LerpAmount");
    SetFixed(kResourceSetDefaultValue, res, "(0.5)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = lerp(V1, V2, LerpAmount);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "LerpFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for linearly interpolating two floats.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "LerpAmount");
    SetFixed(kResourceSetDefaultValue, res, "(0.5)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = lerp(V1, V2, LerpAmount);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "NormalizeFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for normalizing a float4.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "VectorIn");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "VectorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    VectorOut = normalize(VectorIn);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "NormalizeFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for normalizing a float2.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "VectorIn");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "VectorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    VectorOut = normalize(VectorIn);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "NormalizeFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for normalizing a float3.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "VectorIn");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "VectorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    VectorOut = normalize(VectorIn);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "MultiplyAddFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the equation:\n"
        "    Output = (V1 * V2) + V3\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "V1");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "V2");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Scalar");
    SetFixed(kResourceSetVariable, res, "V3");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = (V1 * V2) + V3;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "FloatToFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = float3(Input, Input, Input);\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.rrr;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "FloatToFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = float4(Input, Input, Input, Input);\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.rrrr;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "Float4ToFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = Input.r;\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.r;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "Float3ToFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = Input.r;\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.r;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "WriteDepthToColor");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment writes projected depth to all color component outputs.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "ViewVector");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldViewVector");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "OutputColor");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    float Depth = dot(WorldViewVector, WorldViewVector);\n"
        "    OutputColor.x = sqrt(Depth);\n"
        "    OutputColor.yzw = 1.0f;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "TeeFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment splits and passes through a single float4 input into two \n"
        "    float4 outputs.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Input");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output1");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output2");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output1 = Input;\n"
        "    Output2 = Input;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "TextureRGBSample");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for sampling a texture and returning its value\n"
        "    as a RGB value.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "sampler2D");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "Sampler");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetVariable, res, "Saturate");
    SetFixed(kResourceSetDefaultValue, res, "(true)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    ColorOut.rgb = tex2D(Sampler, TexCoord).rgb;\n"
        "    if (Saturate)\n"
        "    {\n"
        "        ColorOut.rgb = saturate(ColorOut.rgb);\n"
        "    }\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "TextureRGBASample");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for sampling a texture and returning its value\n"
        "    as a RGB value and an A value.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "sampler2D");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "Sampler");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetVariable, res, "Saturate");
    SetFixed(kResourceSetDefaultValue, res, "(true)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    ColorOut = tex2D(Sampler, TexCoord);\n"
        "    if (Saturate)\n"
        "    {\n"
        "        ColorOut = saturate(ColorOut);\n"
        "    }\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "TextureRGBProjectSample");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for sampling a texture and returning its value\n"
        "    as a RGB value and an A value.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "sampler2D");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "Sampler");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetVariable, res, "Saturate");
    SetFixed(kResourceSetDefaultValue, res, "(true)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    float4 ProjTexCoord = TexCoord.xyzz;\n"
        "    ColorOut.rgb = tex2Dproj(Sampler, ProjTexCoord).rgb;\n"
        "    if (Saturate)\n"
        "    {\n"
        "        ColorOut.rgb = saturate(ColorOut.rgb);\n"
        "    }\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "TextureRGBCubeSample");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for sampling a texture and returning its value\n"
        "    as a RGB value.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "samplerCUBE");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "Sampler");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetVariable, res, "Saturate");
    SetFixed(kResourceSetDefaultValue, res, "(true)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    ColorOut.rgb = texCUBE(Sampler, TexCoord).rgb;\n"
        "    if (Saturate)\n"
        "    {\n"
        "        ColorOut.rgb = saturate(ColorOut.rgb);\n"
        "    }\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "SplitColorAndOpacity");
    SetString(kNodeSetDescription, node,
        "\n"
        "    Separate a float4 into a float3 and a float.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorAndOpacity");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Color");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Opacity");
    SetFixed(kResourceSetVariable, res, "Opacity");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Color.rgb = ColorAndOpacity.rgb;\n"
        "    Opacity = ColorAndOpacity.a;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex");
    SetFixed(kNodeSetName, node, "ApplyAlphaTest");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for alpha testing based on the alpha\n"
        "    reference value and alpha test function.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Alpha");
    SetFixed(kResourceSetVariable, res, "AlphaTestValue");
    SetFixed(kResourceSetDefaultValue, res, "(1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Function");
    SetFixed(kResourceSetVariable, res, "AlphaTestFunction");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Fog");
    SetFixed(kResourceSetVariable, res, "AlphaTestRef");
    SetFixed(kResourceSetDefaultValue, res, "(0.0)");
    Call(kNodeAddInputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    // These tests will evaluate to less than 0 if they are true.\n"
        "    float fLessTest = (AlphaTestValue - AlphaTestRef);\n"
        "    float fGreaterTest = -fLessTest;\n"
        "    float fEqualTest = 0.0;\n"
        "    if (fLessTest == 0.0)\n"
        "        fEqualTest = -1.0;\n"
        "\n"
        "    // Clip if AlphaTestValue < AlphaTestRef for:\n"
        "    //  TEST_LESS \n"
        "    //  TEST_EQUAL \n"
        "    //  TEST_LESSEQUAL \n"
        "    clip(fGreaterTest * AlphaTestFunction.x);\n"
        "    \n"
        "    // Clip if AlphaTestValue > AlphaTestRef for:\n"
        "    //  TEST_EQUAL \n"
        "    //  TEST_GREATER \n"
        "    //  TEST_GREATEREQUAL \n"
        "    clip(fLessTest * AlphaTestFunction.y);\n"
        "    \n"
        "    // Clip if AlphaTestValue == AlphaTestRef for:\n"
        "    //  TEST_LESS \n"
        "    //  TEST_GREATER \n"
        "    //  TEST_NOTEQUAL \n"
        "    clip(fEqualTest * AlphaTestFunction.z);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "FloatToFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = float3(Input, Input, Input);\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.rrr;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "FloatToFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = float4(Input, Input, Input, Input);\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.rrrr;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "Float4ToFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = Input.r;\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.r;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "Float3ToFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = Input.r;\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.r;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    return reinterpret_cast<NiRefObject*>(lib);
}

// 0x7BA720: the spot light shadow-map writing material's node library
NiRefObject* __cdecl CreateSpotShadowWriteLibrary() {
    uint32_t lib, node, res, block;
    lib = New(0x1C, kNodeLibraryConstruct, 1u);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "TransformPosition");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the view projection transform\n"
        "    to the input position. Additionally, this fragment applies the world \n"
        "    transform to the input position. \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "Position");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "World");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    // Transform the position into world space for lighting, and projected \n"
        "    // space for display\n"
        "    WorldPos = mul( float4(Position, 1.0f), World );\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "TransformSkinnedPosition");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the view projection and skinning \n"
        "    transform to the input position. Additionally, this fragment applies the \n"
        "    computed world transform to the input position. The weighted world \n"
        "    transform defined by the blendweights is output for use in normals or\n"
        "    other calculations as the new world matrix.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "Position");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "int4");
    SetFixed(kResourceSetSemantic, res, "BlendIndices");
    SetFixed(kResourceSetVariable, res, "BlendIndices");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "BlendWeight");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "BlendWeights");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x3");
    Call(kResourceSetCount, res, 0x1Eu);
    SetFixed(kResourceSetSemantic, res, "SkinBoneMatrix");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "Bones");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "SkinToWorldTransform");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "SkinBoneTransform");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    // TransformSkinnedPosition *********************************************\n"
        "    // Transform the skinned position into world space\n"
        "    // Composite the skinning transform which will take the vertex\n"
        "    // and normal to world space.\n"
        "    float fWeight3 = 1.0 - BlendWeights[0] - BlendWeights[1] - BlendWeights[2];\n"
        "    float4x3 ShortSkinBoneTransform;\n"
        "    ShortSkinBoneTransform  = Bones[BlendIndices[0]] * BlendWeights[0];\n"
        "    ShortSkinBoneTransform += Bones[BlendIndices[1]] * BlendWeights[1];\n"
        "    ShortSkinBoneTransform += Bones[BlendIndices[2]] * BlendWeights[2];\n"
        "    ShortSkinBoneTransform += Bones[BlendIndices[3]] * fWeight3;\n"
        "    SkinBoneTransform = float4x4(ShortSkinBoneTransform[0], 0.0f, \n"
        "        ShortSkinBoneTransform[1], 0.0f, \n"
        "        ShortSkinBoneTransform[2], 0.0f, \n"
        "        ShortSkinBoneTransform[3], 1.0f);\n"
        "\n"
        "    // Transform into world space.\n"
        "    WorldPos.xyz = mul(float4(Position, 1.0), ShortSkinBoneTransform);\n"
        "    WorldPos = mul(float4(WorldPos.xyz, 1.0), SkinToWorldTransform);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "TransformSkinnedPositionNoIndices");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the view projection and skinning \n"
        "    transform to the input position. Additionally, this fragment applies the \n"
        "    computed world transform to the input position. The weighted world \n"
        "    transform defined by the blendweights is output for use in normals or\n"
        "    other calculations as the new world matrix.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "Position");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "BlendWeight");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "BlendWeights");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x3");
    Call(kResourceSetCount, res, 4u);
    SetFixed(kResourceSetSemantic, res, "SkinBoneMatrix");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "Bones");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "SkinToWorldTransform");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "SkinBoneTransform");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    // TransformSkinnedPosition *********************************************\n"
        "    // Transform the skinned position into world space\n"
        "    // Composite the skinning transform which will take the vertex\n"
        "    // and normal to world space.\n"
        "    float fWeight3 = 1.0 - BlendWeights[0] - BlendWeights[1] - BlendWeights[2];\n"
        "    float4x3 ShortSkinBoneTransform;\n"
        "    ShortSkinBoneTransform  = Bones[0] * BlendWeights[0];\n"
        "    ShortSkinBoneTransform += Bones[1] * BlendWeights[1];\n"
        "    ShortSkinBoneTransform += Bones[2] * BlendWeights[2];\n"
        "    ShortSkinBoneTransform += Bones[3] * fWeight3;\n"
        "    SkinBoneTransform = float4x4(ShortSkinBoneTransform[0], 0.0f, \n"
        "        ShortSkinBoneTransform[1], 0.0f, \n"
        "        ShortSkinBoneTransform[2], 0.0f, \n"
        "        ShortSkinBoneTransform[3], 1.0f);\n"
        "\n"
        "    // Transform into world space.\n"
        "    WorldPos.xyz = mul(float4(Position, 1.0), ShortSkinBoneTransform);\n"
        "    WorldPos = mul(float4(WorldPos.xyz, 1.0), SkinToWorldTransform);\n"
        "\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "ProjectPositionWorldToProj");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the view projection transform\n"
        "    to the input world position.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPosition");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "ViewProjMatrix");
    SetFixed(kResourceSetVariable, res, "ViewProjection");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Proj");
    SetFixed(kResourceSetVariable, res, "ProjPos");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    ProjPos = mul(WorldPosition, ViewProjection );\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "ProjectPositionWorldToViewToProj");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the view projection transform\n"
        "    to the input world position.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPosition");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "ViewMatrix");
    SetFixed(kResourceSetVariable, res, "ViewTransform");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "ProjMatrix");
    SetFixed(kResourceSetVariable, res, "ProjTransform");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "View");
    SetFixed(kResourceSetVariable, res, "ViewPos");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Proj");
    SetFixed(kResourceSetVariable, res, "ProjPos");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    ViewPos = mul(WorldPosition, ViewTransform );\n"
        "    ProjPos = mul(ViewPos, ProjTransform );\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "CalculateViewVector");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for calculating the camera view vector.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "CameraPosition");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "CameraPos");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "ViewVector");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldViewVector");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    WorldViewVector = CameraPos - WorldPos;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "AddFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for adding two float4's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 + V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "AddFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for adding two float3's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 + V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "AddFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for adding two float2's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 + V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "AddFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for adding two floats.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 + V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "MultiplyFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for multiplying two float4's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 * V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "MultiplyFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for multiplying two float3's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 * V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "MultiplyFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for multiplying two float2's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 * V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "MultiplyFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for multiplying two floats.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 * V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "ScaleFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for scaling a float4 by a constant.     \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Scale");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Scale * V1;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "ScaleFloat3");
    SetString(kNodeSetDescription, node,
        "    \n"
        "    This fragment is responsible for scaling a float3 by a constant.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Scale");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Scale * V1;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "ScaleFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for scaling a float2 by a constant.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Scale");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Scale * V1;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "SaturateFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for saturating a float4.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = saturate(V1);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "SaturateFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for saturating a float3.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = saturate(V1);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "SaturateFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for saturating a float2.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = saturate(V1);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "SaturateFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for saturating a float.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = saturate(V1);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "LerpFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for linearly interpolating two float4's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "LerpAmount");
    SetFixed(kResourceSetDefaultValue, res, "(0.5)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = lerp(V1, V2, LerpAmount);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "LerpFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for linearly interpolating two float3's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "LerpAmount");
    SetFixed(kResourceSetDefaultValue, res, "(0.5)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = lerp(V1, V2, LerpAmount);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "LerpFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for linearly interpolating two float2's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "LerpAmount");
    SetFixed(kResourceSetDefaultValue, res, "(0.5)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = lerp(V1, V2, LerpAmount);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "LerpFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for linearly interpolating two floats.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "LerpAmount");
    SetFixed(kResourceSetDefaultValue, res, "(0.5)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = lerp(V1, V2, LerpAmount);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "NormalizeFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for normalizing a float4.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "VectorIn");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "VectorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    VectorOut = normalize(VectorIn);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "NormalizeFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for normalizing a float2.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "VectorIn");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "VectorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    VectorOut = normalize(VectorIn);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "NormalizeFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for normalizing a float3.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "VectorIn");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "VectorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    VectorOut = normalize(VectorIn);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "MultiplyAddFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the equation:\n"
        "    Output = (V1 * V2) + V3\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "V1");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "V2");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Scalar");
    SetFixed(kResourceSetVariable, res, "V3");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = (V1 * V2) + V3;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "FloatToFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = float3(Input, Input, Input);\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.rrr;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "FloatToFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = float4(Input, Input, Input, Input);\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.rrrr;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "Float4ToFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = Input.r;\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.r;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "Float3ToFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = Input.r;\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.r;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "WriteDepthToColor");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment writes projected depth to all color component outputs.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Proj");
    SetFixed(kResourceSetVariable, res, "WorldPosProjected");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "ViewVector");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldViewVector");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "OutputColor");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "        \n"
        "    float Depth = WorldPosProjected.z / WorldPosProjected.w;    \n"
        "    OutputColor.x = Depth;\n"
        "    OutputColor.yzw = 1.0f;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "WriteVSMDepthToColor");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment writes projected depth to all color component outputs.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Proj");
    SetFixed(kResourceSetVariable, res, "WorldPosProjected");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "ViewVector");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldViewVector");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "OutputColor");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    float Depth = WorldPosProjected.z / WorldPosProjected.w;\n"
        "            \n"
        "    OutputColor.x = Depth;\n"
        "    OutputColor.y = Depth * Depth;\n"
        "    OutputColor.zw = 1.0;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "TeeFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "      This fragment splits and passes through a single float4 input into two\n"
        "      float4 outputs.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Input");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output1");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output2");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "      Output1 = Input;\n"
        "      Output2 = Input;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "TextureRGBSample");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for sampling a texture and returning its value\n"
        "    as a RGB value.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "sampler2D");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "Sampler");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetVariable, res, "Saturate");
    SetFixed(kResourceSetDefaultValue, res, "(true)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    ColorOut.rgb = tex2D(Sampler, TexCoord).rgb;\n"
        "    if (Saturate)\n"
        "    {\n"
        "        ColorOut.rgb = saturate(ColorOut.rgb);\n"
        "    }\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "TextureRGBASample");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for sampling a texture and returning its value\n"
        "    as a RGB value and an A value.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "sampler2D");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "Sampler");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetVariable, res, "Saturate");
    SetFixed(kResourceSetDefaultValue, res, "(true)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    ColorOut = tex2D(Sampler, TexCoord);\n"
        "    if (Saturate)\n"
        "    {\n"
        "        ColorOut = saturate(ColorOut);\n"
        "    }\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "TextureRGBProjectSample");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for sampling a texture and returning its value\n"
        "    as a RGB value and an A value.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "sampler2D");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "Sampler");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetVariable, res, "Saturate");
    SetFixed(kResourceSetDefaultValue, res, "(true)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    float4 ProjTexCoord = TexCoord.xyzz;\n"
        "    ColorOut.rgb = tex2Dproj(Sampler, ProjTexCoord).rgb;\n"
        "    if (Saturate)\n"
        "    {\n"
        "        ColorOut.rgb = saturate(ColorOut.rgb);\n"
        "    }\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "TextureRGBCubeSample");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for sampling a texture and returning its value\n"
        "    as a RGB value.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "samplerCUBE");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "Sampler");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetVariable, res, "Saturate");
    SetFixed(kResourceSetDefaultValue, res, "(true)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    ColorOut.rgb = texCUBE(Sampler, TexCoord).rgb;\n"
        "    if (Saturate)\n"
        "    {\n"
        "        ColorOut.rgb = saturate(ColorOut.rgb);\n"
        "    }\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "SplitColorAndOpacity");
    SetString(kNodeSetDescription, node,
        "\n"
        "    Separate a float4 into a float3 and a float.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorAndOpacity");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Color");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Opacity");
    SetFixed(kResourceSetVariable, res, "Opacity");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Color.rgb = ColorAndOpacity.rgb;\n"
        "    Opacity = ColorAndOpacity.a;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex");
    SetFixed(kNodeSetName, node, "ApplyAlphaTest");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for alpha testing based on the alpha\n"
        "    reference value and alpha test function.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Alpha");
    SetFixed(kResourceSetVariable, res, "AlphaTestValue");
    SetFixed(kResourceSetDefaultValue, res, "(1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Function");
    SetFixed(kResourceSetVariable, res, "AlphaTestFunction");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Fog");
    SetFixed(kResourceSetVariable, res, "AlphaTestRef");
    SetFixed(kResourceSetDefaultValue, res, "(0.0)");
    Call(kNodeAddInputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    // These tests will evaluate to less than 0 if they are true.\n"
        "    float fLessTest = (AlphaTestValue - AlphaTestRef);\n"
        "    float fGreaterTest = -fLessTest;\n"
        "    float fEqualTest = 0.0;\n"
        "    if (fLessTest == 0.0)\n"
        "        fEqualTest = -1.0;\n"
        "\n"
        "    // Clip if AlphaTestValue < AlphaTestRef for:\n"
        "    //  TEST_LESS \n"
        "    //  TEST_EQUAL \n"
        "    //  TEST_LESSEQUAL \n"
        "    clip(fGreaterTest * AlphaTestFunction.x);\n"
        "    \n"
        "    // Clip if AlphaTestValue > AlphaTestRef for:\n"
        "    //  TEST_EQUAL \n"
        "    //  TEST_GREATER \n"
        "    //  TEST_GREATEREQUAL \n"
        "    clip(fLessTest * AlphaTestFunction.y);\n"
        "    \n"
        "    // Clip if AlphaTestValue == AlphaTestRef for:\n"
        "    //  TEST_LESS \n"
        "    //  TEST_GREATER \n"
        "    //  TEST_NOTEQUAL \n"
        "    clip(fEqualTest * AlphaTestFunction.z);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "FloatToFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = float3(Input, Input, Input);\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.rrr;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "FloatToFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = float4(Input, Input, Input, Input);\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.rrrr;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "Float4ToFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = Input.r;\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.r;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "Float3ToFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = Input.r;\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.r;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    return reinterpret_cast<NiRefObject*>(lib);
}

// 0x7C4B90: the directional light shadow-map writing material's node library
NiRefObject* __cdecl CreateDirectionalShadowWriteLibrary() {
    uint32_t lib, node, res, block;
    lib = New(0x1C, kNodeLibraryConstruct, 1u);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "TransformPosition");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the view projection transform\n"
        "    to the input position. Additionally, this fragment applies the world \n"
        "    transform to the input position. \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "Position");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "World");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    // Transform the position into world space for lighting, and projected \n"
        "    // space for display\n"
        "    WorldPos = mul( float4(Position, 1.0f), World );\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "TransformSkinnedPosition");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the view projection and skinning \n"
        "    transform to the input position. Additionally, this fragment applies the \n"
        "    computed world transform to the input position. The weighted world \n"
        "    transform defined by the blendweights is output for use in normals or\n"
        "    other calculations as the new world matrix.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "Position");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "int4");
    SetFixed(kResourceSetSemantic, res, "BlendIndices");
    SetFixed(kResourceSetVariable, res, "BlendIndices");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "BlendWeight");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "BlendWeights");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x3");
    Call(kResourceSetCount, res, 0x1Eu);
    SetFixed(kResourceSetSemantic, res, "SkinBoneMatrix");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "Bones");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "SkinToWorldTransform");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "SkinBoneTransform");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    // TransformSkinnedPosition *********************************************\n"
        "    // Transform the skinned position into world space\n"
        "    // Composite the skinning transform which will take the vertex\n"
        "    // and normal to world space.\n"
        "    float fWeight3 = 1.0 - BlendWeights[0] - BlendWeights[1] - BlendWeights[2];\n"
        "    float4x3 ShortSkinBoneTransform;\n"
        "    ShortSkinBoneTransform  = Bones[BlendIndices[0]] * BlendWeights[0];\n"
        "    ShortSkinBoneTransform += Bones[BlendIndices[1]] * BlendWeights[1];\n"
        "    ShortSkinBoneTransform += Bones[BlendIndices[2]] * BlendWeights[2];\n"
        "    ShortSkinBoneTransform += Bones[BlendIndices[3]] * fWeight3;\n"
        "    SkinBoneTransform = float4x4(ShortSkinBoneTransform[0], 0.0f, \n"
        "        ShortSkinBoneTransform[1], 0.0f, \n"
        "        ShortSkinBoneTransform[2], 0.0f, \n"
        "        ShortSkinBoneTransform[3], 1.0f);\n"
        "\n"
        "    // Transform into world space.\n"
        "    WorldPos.xyz = mul(float4(Position, 1.0), ShortSkinBoneTransform);\n"
        "    WorldPos = mul(float4(WorldPos.xyz, 1.0), SkinToWorldTransform);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "TransformSkinnedPositionNoIndices");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the view projection and skinning \n"
        "    transform to the input position. Additionally, this fragment applies the \n"
        "    computed world transform to the input position. The weighted world \n"
        "    transform defined by the blendweights is output for use in normals or\n"
        "    other calculations as the new world matrix.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "Position");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "BlendWeight");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "BlendWeights");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x3");
    Call(kResourceSetCount, res, 4u);
    SetFixed(kResourceSetSemantic, res, "SkinBoneMatrix");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "Bones");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "SkinToWorldTransform");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "SkinBoneTransform");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    // TransformSkinnedPosition *********************************************\n"
        "    // Transform the skinned position into world space\n"
        "    // Composite the skinning transform which will take the vertex\n"
        "    // and normal to world space.\n"
        "    float fWeight3 = 1.0 - BlendWeights[0] - BlendWeights[1] - BlendWeights[2];\n"
        "    float4x3 ShortSkinBoneTransform;\n"
        "    ShortSkinBoneTransform  = Bones[0] * BlendWeights[0];\n"
        "    ShortSkinBoneTransform += Bones[1] * BlendWeights[1];\n"
        "    ShortSkinBoneTransform += Bones[2] * BlendWeights[2];\n"
        "    ShortSkinBoneTransform += Bones[3] * fWeight3;\n"
        "    SkinBoneTransform = float4x4(ShortSkinBoneTransform[0], 0.0f, \n"
        "        ShortSkinBoneTransform[1], 0.0f, \n"
        "        ShortSkinBoneTransform[2], 0.0f, \n"
        "        ShortSkinBoneTransform[3], 1.0f);\n"
        "\n"
        "    // Transform into world space.\n"
        "    WorldPos.xyz = mul(float4(Position, 1.0), ShortSkinBoneTransform);\n"
        "    WorldPos = mul(float4(WorldPos.xyz, 1.0), SkinToWorldTransform);\n"
        "\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "ProjectPositionWorldToProj");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the view projection transform\n"
        "    to the input world position.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPosition");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "ViewProjMatrix");
    SetFixed(kResourceSetVariable, res, "ViewProjection");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Proj");
    SetFixed(kResourceSetVariable, res, "ProjPos");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    ProjPos = mul(WorldPosition, ViewProjection );\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "ProjectPositionWorldToViewToProj");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the view projection transform\n"
        "    to the input world position.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPosition");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "ViewMatrix");
    SetFixed(kResourceSetVariable, res, "ViewTransform");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "ProjMatrix");
    SetFixed(kResourceSetVariable, res, "ProjTransform");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "View");
    SetFixed(kResourceSetVariable, res, "ViewPos");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Proj");
    SetFixed(kResourceSetVariable, res, "ProjPos");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    ViewPos = mul(WorldPosition, ViewTransform );\n"
        "    ProjPos = mul(ViewPos, ProjTransform );\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "CalculateViewVector");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for calculating the camera view vector.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "CameraPosition");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "CameraPos");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "ViewVector");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldViewVector");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    WorldViewVector = CameraPos - WorldPos;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "AddFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for adding two float4's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 + V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "AddFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for adding two float3's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 + V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "AddFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for adding two float2's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 + V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "AddFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for adding two floats.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 + V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "MultiplyFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for multiplying two float4's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 * V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "MultiplyFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for multiplying two float3's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 * V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "MultiplyFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for multiplying two float2's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 * V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "MultiplyFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for multiplying two floats.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 * V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "ScaleFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for scaling a float4 by a constant.     \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Scale");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Scale * V1;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "ScaleFloat3");
    SetString(kNodeSetDescription, node,
        "    \n"
        "    This fragment is responsible for scaling a float3 by a constant.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Scale");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Scale * V1;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "ScaleFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for scaling a float2 by a constant.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Scale");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Scale * V1;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "SaturateFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for saturating a float4.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = saturate(V1);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "SaturateFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for saturating a float3.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = saturate(V1);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "SaturateFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for saturating a float2.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = saturate(V1);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "SaturateFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for saturating a float.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = saturate(V1);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "LerpFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for linearly interpolating two float4's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "LerpAmount");
    SetFixed(kResourceSetDefaultValue, res, "(0.5)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = lerp(V1, V2, LerpAmount);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "LerpFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for linearly interpolating two float3's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "LerpAmount");
    SetFixed(kResourceSetDefaultValue, res, "(0.5)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = lerp(V1, V2, LerpAmount);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "LerpFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for linearly interpolating two float2's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "LerpAmount");
    SetFixed(kResourceSetDefaultValue, res, "(0.5)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = lerp(V1, V2, LerpAmount);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "LerpFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for linearly interpolating two floats.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "LerpAmount");
    SetFixed(kResourceSetDefaultValue, res, "(0.5)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = lerp(V1, V2, LerpAmount);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "NormalizeFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for normalizing a float4.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "VectorIn");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "VectorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    VectorOut = normalize(VectorIn);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "NormalizeFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for normalizing a float2.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "VectorIn");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "VectorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    VectorOut = normalize(VectorIn);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "NormalizeFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for normalizing a float3.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "VectorIn");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "VectorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    VectorOut = normalize(VectorIn);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "MultiplyAddFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the equation:\n"
        "    Output = (V1 * V2) + V3\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "V1");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "V2");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Scalar");
    SetFixed(kResourceSetVariable, res, "V3");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = (V1 * V2) + V3;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "FloatToFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = float3(Input, Input, Input);\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.rrr;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "FloatToFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = float4(Input, Input, Input, Input);\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.rrrr;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "Float4ToFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = Input.r;\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.r;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "Float3ToFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = Input.r;\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.r;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "WriteDepthToColor");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment writes projected depth to all color component outputs.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Proj");
    SetFixed(kResourceSetVariable, res, "WorldPosProjected");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "ViewVector");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldViewVector");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "OutputColor");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    float Depth = WorldPosProjected.z / WorldPosProjected.w;\n"
        "    OutputColor.x = Depth;\n"
        "    OutputColor.yzw = 1.0f;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "WriteVSMDepthToColor");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment writes projected depth to all color component outputs.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Proj");
    SetFixed(kResourceSetVariable, res, "WorldPosProjected");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "ViewVector");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldViewVector");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "OutputColor");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    float Depth = WorldPosProjected.z / WorldPosProjected.w;\n"
        "            \n"
        "    OutputColor.x = Depth;\n"
        "    OutputColor.y = Depth * Depth;\n"
        "    OutputColor.zw = 1.0;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "TeeFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment splits and passes through a single float4 input into two \n"
        "    float4 outputs.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Input");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output1");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output2");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output1 = Input;\n"
        "    Output2 = Input;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "TextureRGBSample");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for sampling a texture and returning its value\n"
        "    as a RGB value.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "sampler2D");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "Sampler");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetVariable, res, "Saturate");
    SetFixed(kResourceSetDefaultValue, res, "(true)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    ColorOut.rgb = tex2D(Sampler, TexCoord).rgb;\n"
        "    if (Saturate)\n"
        "    {\n"
        "        ColorOut.rgb = saturate(ColorOut.rgb);\n"
        "    }\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "TextureRGBASample");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for sampling a texture and returning its value\n"
        "    as a RGB value and an A value.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "sampler2D");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "Sampler");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetVariable, res, "Saturate");
    SetFixed(kResourceSetDefaultValue, res, "(true)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    ColorOut = tex2D(Sampler, TexCoord);\n"
        "    if (Saturate)\n"
        "    {\n"
        "        ColorOut = saturate(ColorOut);\n"
        "    }\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "TextureRGBProjectSample");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for sampling a texture and returning its value\n"
        "    as a RGB value and an A value.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "sampler2D");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "Sampler");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetVariable, res, "Saturate");
    SetFixed(kResourceSetDefaultValue, res, "(true)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    float4 ProjTexCoord = TexCoord.xyzz;\n"
        "    ColorOut.rgb = tex2Dproj(Sampler, ProjTexCoord).rgb;\n"
        "    if (Saturate)\n"
        "    {\n"
        "        ColorOut.rgb = saturate(ColorOut.rgb);\n"
        "    }\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "TextureRGBCubeSample");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for sampling a texture and returning its value\n"
        "    as a RGB value.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "samplerCUBE");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "Sampler");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetVariable, res, "Saturate");
    SetFixed(kResourceSetDefaultValue, res, "(true)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    ColorOut.rgb = texCUBE(Sampler, TexCoord).rgb;\n"
        "    if (Saturate)\n"
        "    {\n"
        "        ColorOut.rgb = saturate(ColorOut.rgb);\n"
        "    }\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "SplitColorAndOpacity");
    SetString(kNodeSetDescription, node,
        "\n"
        "    Separate a float4 into a float3 and a float.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorAndOpacity");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Color");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Opacity");
    SetFixed(kResourceSetVariable, res, "Opacity");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Color.rgb = ColorAndOpacity.rgb;\n"
        "    Opacity = ColorAndOpacity.a;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex");
    SetFixed(kNodeSetName, node, "ApplyAlphaTest");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for alpha testing based on the alpha\n"
        "    reference value and alpha test function.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Alpha");
    SetFixed(kResourceSetVariable, res, "AlphaTestValue");
    SetFixed(kResourceSetDefaultValue, res, "(1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Function");
    SetFixed(kResourceSetVariable, res, "AlphaTestFunction");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Fog");
    SetFixed(kResourceSetVariable, res, "AlphaTestRef");
    SetFixed(kResourceSetDefaultValue, res, "(0.0)");
    Call(kNodeAddInputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    // These tests will evaluate to less than 0 if they are true.\n"
        "    float fLessTest = (AlphaTestValue - AlphaTestRef);\n"
        "    float fGreaterTest = -fLessTest;\n"
        "    float fEqualTest = 0.0;\n"
        "    if (fLessTest == 0.0)\n"
        "        fEqualTest = -1.0;\n"
        "\n"
        "    // Clip if AlphaTestValue < AlphaTestRef for:\n"
        "    //  TEST_LESS \n"
        "    //  TEST_EQUAL \n"
        "    //  TEST_LESSEQUAL \n"
        "    clip(fGreaterTest * AlphaTestFunction.x);\n"
        "    \n"
        "    // Clip if AlphaTestValue > AlphaTestRef for:\n"
        "    //  TEST_EQUAL \n"
        "    //  TEST_GREATER \n"
        "    //  TEST_GREATEREQUAL \n"
        "    clip(fLessTest * AlphaTestFunction.y);\n"
        "    \n"
        "    // Clip if AlphaTestValue == AlphaTestRef for:\n"
        "    //  TEST_LESS \n"
        "    //  TEST_GREATER \n"
        "    //  TEST_NOTEQUAL \n"
        "    clip(fEqualTest * AlphaTestFunction.z);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "FloatToFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = float3(Input, Input, Input);\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.rrr;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "FloatToFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = float4(Input, Input, Input, Input);\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.rrrr;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "Float4ToFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = Input.r;\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.r;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "Float3ToFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = Input.r;\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.r;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    return reinterpret_cast<NiRefObject*>(lib);
}

// 0x7ECED0: the default shadow click generator's node library (made by 0x7D1380: transforms, shadow-map depth writing, VSM, blurs, texture samples)
NiRefObject* __cdecl CreateShadowClickGeneratorLibrary() {
    uint32_t lib, node, res, block;
    lib = New(0x1C, kNodeLibraryConstruct, 1u);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "TransformPosition");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the view projection transform\n"
        "    to the input position. Additionally, this fragment applies the world \n"
        "    transform to the input position. \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "Position");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "World");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    // Transform the position into world space for lighting, and projected \n"
        "    // space for display\n"
        "    WorldPos = mul( float4(Position, 1.0f), World );\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "TransformSkinnedPosition");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the view projection and skinning \n"
        "    transform to the input position. Additionally, this fragment applies the \n"
        "    computed world transform to the input position. The weighted world \n"
        "    transform defined by the blendweights is output for use in normals or\n"
        "    other calculations as the new world matrix.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "Position");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "int4");
    SetFixed(kResourceSetSemantic, res, "BlendIndices");
    SetFixed(kResourceSetVariable, res, "BlendIndices");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "BlendWeight");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "BlendWeights");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x3");
    Call(kResourceSetCount, res, 0x1Eu);
    SetFixed(kResourceSetSemantic, res, "SkinBoneMatrix");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "Bones");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "SkinToWorldTransform");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "SkinBoneTransform");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    // TransformSkinnedPosition *********************************************\n"
        "    // Transform the skinned position into world space\n"
        "    // Composite the skinning transform which will take the vertex\n"
        "    // and normal to world space.\n"
        "    float fWeight3 = 1.0 - BlendWeights[0] - BlendWeights[1] - BlendWeights[2];\n"
        "    float4x3 ShortSkinBoneTransform;\n"
        "    ShortSkinBoneTransform  = Bones[BlendIndices[0]] * BlendWeights[0];\n"
        "    ShortSkinBoneTransform += Bones[BlendIndices[1]] * BlendWeights[1];\n"
        "    ShortSkinBoneTransform += Bones[BlendIndices[2]] * BlendWeights[2];\n"
        "    ShortSkinBoneTransform += Bones[BlendIndices[3]] * fWeight3;\n"
        "    SkinBoneTransform = float4x4(ShortSkinBoneTransform[0], 0.0f, \n"
        "        ShortSkinBoneTransform[1], 0.0f, \n"
        "        ShortSkinBoneTransform[2], 0.0f, \n"
        "        ShortSkinBoneTransform[3], 1.0f);\n"
        "\n"
        "    // Transform into world space.\n"
        "    WorldPos.xyz = mul(float4(Position, 1.0), ShortSkinBoneTransform);\n"
        "    WorldPos = mul(float4(WorldPos.xyz, 1.0), SkinToWorldTransform);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "TransformSkinnedPositionNoIndices");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the view projection and skinning \n"
        "    transform to the input position. Additionally, this fragment applies the \n"
        "    computed world transform to the input position. The weighted world \n"
        "    transform defined by the blendweights is output for use in normals or\n"
        "    other calculations as the new world matrix.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "Position");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "BlendWeight");
    SetFixed(kResourceSetLabel, res, "Local");
    SetFixed(kResourceSetVariable, res, "BlendWeights");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x3");
    Call(kResourceSetCount, res, 4u);
    SetFixed(kResourceSetSemantic, res, "SkinBoneMatrix");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "Bones");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "SkinToWorldTransform");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "WorldMatrix");
    SetFixed(kResourceSetVariable, res, "SkinBoneTransform");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    // TransformSkinnedPosition *********************************************\n"
        "    // Transform the skinned position into world space\n"
        "    // Composite the skinning transform which will take the vertex\n"
        "    // and normal to world space.\n"
        "    float fWeight3 = 1.0 - BlendWeights[0] - BlendWeights[1] - BlendWeights[2];\n"
        "    float4x3 ShortSkinBoneTransform;\n"
        "    ShortSkinBoneTransform  = Bones[0] * BlendWeights[0];\n"
        "    ShortSkinBoneTransform += Bones[1] * BlendWeights[1];\n"
        "    ShortSkinBoneTransform += Bones[2] * BlendWeights[2];\n"
        "    ShortSkinBoneTransform += Bones[3] * fWeight3;\n"
        "    SkinBoneTransform = float4x4(ShortSkinBoneTransform[0], 0.0f, \n"
        "        ShortSkinBoneTransform[1], 0.0f, \n"
        "        ShortSkinBoneTransform[2], 0.0f, \n"
        "        ShortSkinBoneTransform[3], 1.0f);\n"
        "\n"
        "    // Transform into world space.\n"
        "    WorldPos.xyz = mul(float4(Position, 1.0), ShortSkinBoneTransform);\n"
        "    WorldPos = mul(float4(WorldPos.xyz, 1.0), SkinToWorldTransform);\n"
        "\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "ProjectPositionWorldToProj");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the view projection transform\n"
        "    to the input world position.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPosition");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "ViewProjMatrix");
    SetFixed(kResourceSetVariable, res, "ViewProjection");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Proj");
    SetFixed(kResourceSetVariable, res, "ProjPos");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    ProjPos = mul(WorldPosition, ViewProjection );\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "ProjectPositionWorldToViewToProj");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying the view projection transform\n"
        "    to the input world position.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPosition");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "ViewMatrix");
    SetFixed(kResourceSetVariable, res, "ViewTransform");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "ProjMatrix");
    SetFixed(kResourceSetVariable, res, "ProjTransform");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "View");
    SetFixed(kResourceSetVariable, res, "ViewPos");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Proj");
    SetFixed(kResourceSetVariable, res, "ProjPos");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    ViewPos = mul(WorldPosition, ViewTransform );\n"
        "    ProjPos = mul(ViewPos, ProjTransform );\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "CalculateViewVector");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for calculating the camera view vector.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldPos");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "CameraPosition");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "CameraPos");
    SetFixed(kResourceSetDefaultValue, res, "(0.0, 0.0, 0.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "ViewVector");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldViewVector");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    WorldViewVector = CameraPos - WorldPos;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "AddFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for adding two float4's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 + V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "AddFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for adding two float3's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 + V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "AddFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for adding two float2's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 + V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "AddFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for adding two floats.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 + V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "MultiplyFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for multiplying two float4's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 * V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "MultiplyFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for multiplying two float3's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 * V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "MultiplyFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for multiplying two float2's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 * V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "MultiplyFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for multiplying two floats.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = V1 * V2;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "ScaleFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for scaling a float4 by a constant.     \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Scale");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Scale * V1;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "ScaleFloat3");
    SetString(kNodeSetDescription, node,
        "    \n"
        "    This fragment is responsible for scaling a float3 by a constant.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Scale");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Scale * V1;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "ScaleFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for scaling a float2 by a constant.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Scale");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Scale * V1;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "SaturateFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for saturating a float4.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = saturate(V1);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "SaturateFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for saturating a float3.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = saturate(V1);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "SaturateFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for saturating a float2.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = saturate(V1);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "SaturateFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for saturating a float.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = saturate(V1);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "LerpFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for linearly interpolating two float4's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "LerpAmount");
    SetFixed(kResourceSetDefaultValue, res, "(0.5)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = lerp(V1, V2, LerpAmount);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "LerpFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for linearly interpolating two float3's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "LerpAmount");
    SetFixed(kResourceSetDefaultValue, res, "(0.5)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = lerp(V1, V2, LerpAmount);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "LerpFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for linearly interpolating two float2's.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "LerpAmount");
    SetFixed(kResourceSetDefaultValue, res, "(0.5)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = lerp(V1, V2, LerpAmount);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel/Vertex");
    SetFixed(kNodeSetName, node, "LerpFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for linearly interpolating two floats.   \n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V1");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "V2");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "LerpAmount");
    SetFixed(kResourceSetDefaultValue, res, "(0.5)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = lerp(V1, V2, LerpAmount);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "NormalizeFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for normalizing a float4.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "VectorIn");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "VectorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    VectorOut = normalize(VectorIn);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "NormalizeFloat2");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for normalizing a float2.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "VectorIn");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetVariable, res, "VectorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    VectorOut = normalize(VectorIn);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "NormalizeFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for normalizing a float3.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "VectorIn");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "VectorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    VectorOut = normalize(VectorIn);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "MultiplyAddFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the equation:\n"
        "    Output = (V1 * V2) + V3\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "V1");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "V2");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Scalar");
    SetFixed(kResourceSetVariable, res, "V3");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = (V1 * V2) + V3;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "FloatToFloat3");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = float3(Input, Input, Input);\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.rrr;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "FloatToFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = float4(Input, Input, Input, Input);\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.rrrr;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "Float4ToFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = Input.r;\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.r;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "Float3ToFloat");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment implements the operation:\n"
        "    Output = Input.r;\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetVariable, res, "Input");
    SetFixed(kResourceSetDefaultValue, res, "(1.0, 1.0, 1.0, 1.0)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "Output");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    Output = Input.r;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "BlurColor");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment writes projected depth to all color component outputs.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Proj");
    SetFixed(kResourceSetVariable, res, "WorldPosProjected");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "OutputColor");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    float Depth = WorldPosProjected.z / WorldPosProjected.w;\n"
        "    OutputColor = Depth;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "WriteDepthToColor");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment writes projected depth to all color component outputs.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Proj");
    SetFixed(kResourceSetVariable, res, "WorldPosProjected");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "ViewVector");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldViewVector");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "OutputColor");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    float Depth = WorldPosProjected.z / WorldPosProjected.w;\n"
        "    OutputColor = Depth;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "WriteVSMDepthToColor");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment writes projected depth to all color component outputs.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Position");
    SetFixed(kResourceSetLabel, res, "Proj");
    SetFixed(kResourceSetVariable, res, "WorldPosProjected");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "ViewVector");
    SetFixed(kResourceSetLabel, res, "World");
    SetFixed(kResourceSetVariable, res, "WorldViewVector");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "OutputColor");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    float Depth = WorldPosProjected.z / WorldPosProjected.w;\n"
        "            \n"
        "    OutputColor.x = Depth;\n"
        "    OutputColor.y = Depth * Depth;\n"
        "    OutputColor.z = 1.0;\n"
        "    OutputColor.w = 1.0;\n"
        "\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "TeeFloat4");
    SetString(kNodeSetDescription, node,
        "\n"
        "      This fragment splits and passes through a single float4 input into two\n"
        "      float4 outputs.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Input");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output1");
    Call(kNodeAddOutputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetVariable, res, "Output2");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "      Output1 = Input;\n"
        "      Output2 = Input;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "TextureRGBASample");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for sampling a texture and returning its value\n"
        "    as a RGB value and an A value.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "sampler2D");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "Sampler");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetVariable, res, "Saturate");
    SetFixed(kResourceSetDefaultValue, res, "(false)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    ColorOut = tex2D(Sampler, TexCoord);\n"
        "    if (Saturate)\n"
        "    {\n"
        "        ColorOut = saturate(ColorOut);\n"
        "    }\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "TextureRGBProjectSample");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for sampling a texture and returning its value\n"
        "    as a RGB value and an A value.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "sampler2D");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "Sampler");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetVariable, res, "Saturate");
    SetFixed(kResourceSetDefaultValue, res, "(false)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    float4 ProjTexCoord = TexCoord.xyzz;\n"
        "    ColorOut.a = 1.0f;    \n"
        "    ColorOut.rgb = tex2Dproj(Sampler, ProjTexCoord).rgb;    \n"
        "    if (Saturate)\n"
        "    {\n"
        "        ColorOut.rgb = saturate(ColorOut.rgb);\n"
        "    }\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "TextureRGBCubeSample");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for sampling a texture and returning its value\n"
        "    as a RGB value.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "samplerCUBE");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "Sampler");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetVariable, res, "Saturate");
    SetFixed(kResourceSetDefaultValue, res, "(false)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    ColorOut.a = 1.0f;\n"
        "    ColorOut.rgb = texCUBE(Sampler, TexCoord).rgb;\n"
        "    if (Saturate)\n"
        "    {\n"
        "        ColorOut.rgb = saturate(ColorOut.rgb);\n"
        "    }\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "TextureRGBSample");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for sampling a texture and returning its value\n"
        "    as a RGB value.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "sampler2D");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "Sampler");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetVariable, res, "Saturate");
    SetFixed(kResourceSetDefaultValue, res, "(false)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    ColorOut.a = 1.0f;\n"
        "    ColorOut.rgb = tex2D(Sampler, TexCoord).rgb;\n"
        "    if (Saturate)\n"
        "    {\n"
        "        ColorOut.rgb = saturate(ColorOut.rgb);\n"
        "    }\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "TexTransformApply");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying a transform to the input set\n"
        "    of texture coordinates.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "TexTransform");
    SetFixed(kResourceSetVariable, res, "TexTransform");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoordOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "    \n"
        "    TexCoordOut = mul(float4(TexCoord.x, TexCoord.y, 0.0, 1.0), TexTransform);\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Vertex/Pixel");
    SetFixed(kNodeSetName, node, "ProjectTextureCoordinates");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for applying a projection to the input set\n"
        "    of texture coordinates.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float3");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4x4");
    SetFixed(kResourceSetSemantic, res, "TexTransform");
    SetFixed(kResourceSetVariable, res, "TexTransform");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoordOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "vs_1_1/ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "  \n"
        "    TexCoordOut = mul(float4(TexCoord, 1.0), TexTransform);\n"
        "    \n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "HorzBlurSample");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for sampling a texture and returning its value\n"
        "    as a RGB value.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "sampler2D");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "Sampler");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetVariable, res, "Saturate");
    SetFixed(kResourceSetDefaultValue, res, "(false)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "TexSizeBase");
    SetFixed(kResourceSetVariable, res, "MapSize");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "KernelSize");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "\n"
        "    // Blurs the texel at vTexCoord in the X direction.   \n"
        "    float fInvTexSize = 1.0 / MapSize.x;\n"
        "    float fHalfBlurKernelSize = KernelSize * 0.5f;\n"
        "    float fInvTotalWeights = 1.0 / (KernelSize * KernelSize);    \n"
        "    float fBlurWeight = 2;\n"
        "   \n"
        "    float4 fTotalColor = 0;   \n"
        "    float2 vTexCoord = TexCoord;\n"
        "    vTexCoord.x -= fInvTexSize * fHalfBlurKernelSize;    \n"
        "    float i = 0;\n"
        "\n"
        "    // Blur x2\n"
        "    if ( 2 <= KernelSize)\n"
        "    {\n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;        \n"
        "        vTexCoord.x += fInvTexSize;\n"
        "        \n"
        "        float fSign = sign( i + 1 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;\n"
        "        \n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;\n"
        "        vTexCoord.x += fInvTexSize;            \n"
        "        \n"
        "        fSign = sign( i + 2 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;        \n"
        "        i+=2;\n"
        "    }\n"
        "\n"
        "    // Blur x4\n"
        "    if ( 4 <= KernelSize)\n"
        "    {\n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;        \n"
        "        vTexCoord.x += fInvTexSize;\n"
        "        \n"
        "        float fSign = sign( i + 1 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;\n"
        "        \n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;\n"
        "        vTexCoord.x += fInvTexSize;            \n"
        "        \n"
        "        fSign = sign( i + 2 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;        \n"
        "        i+=2;\n"
        "    }\n"
        "    \n"
        "    // Blur x6\n"
        "    if ( 6 <= KernelSize)\n"
        "    {\n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;        \n"
        "        vTexCoord.x += fInvTexSize;\n"
        "        \n"
        "        float fSign = sign( i + 1 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;\n"
        "        \n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;\n"
        "        vTexCoord.x += fInvTexSize;            \n"
        "        \n"
        "        fSign = sign( i + 2 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;        \n"
        "        i+=2;\n"
        "    }    \n"
        "    \n"
        "    // Blur x8\n"
        "    if ( 8 <= KernelSize)\n"
        "    {\n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;        \n"
        "        vTexCoord.x += fInvTexSize;\n"
        "        \n"
        "        float fSign = sign( i + 1 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;\n"
        "        \n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;\n"
        "        vTexCoord.x += fInvTexSize;            \n"
        "        \n"
        "        fSign = sign( i + 2 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;        \n"
        "        i+=2;\n"
        "    }    \n"
        "    \n"
        "    // Blur x10\n"
        "    if ( 10 <= KernelSize)\n"
        "    {\n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;        \n"
        "        vTexCoord.x += fInvTexSize;\n"
        "        \n"
        "        float fSign = sign( i + 1 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;\n"
        "        \n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;\n"
        "        vTexCoord.x += fInvTexSize;            \n"
        "        \n"
        "        fSign = sign( i + 2 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;        \n"
        "        i+=2;\n"
        "    }\n"
        "\n"
        "    // Blur x12\n"
        "    if ( 12 <= KernelSize)\n"
        "    {\n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;        \n"
        "        vTexCoord.x += fInvTexSize;\n"
        "        \n"
        "        float fSign = sign( i + 1 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;\n"
        "        \n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;\n"
        "        vTexCoord.x += fInvTexSize;            \n"
        "        \n"
        "        fSign = sign( i + 2 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;        \n"
        "        i+=2;\n"
        "    }\n"
        "       \n"
        "    // Blur x14\n"
        "    if ( 14 <= KernelSize)\n"
        "    {\n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;        \n"
        "        vTexCoord.x += fInvTexSize;\n"
        "        \n"
        "        float fSign = sign( i + 1 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;\n"
        "        \n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;\n"
        "        vTexCoord.x += fInvTexSize;            \n"
        "        \n"
        "        fSign = sign( i + 2 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;        \n"
        "        i+=2;\n"
        "    }    \n"
        "    \n"
        "    // Blur x16\n"
        "    if ( 16 <= KernelSize)\n"
        "    {\n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;        \n"
        "        vTexCoord.x += fInvTexSize;\n"
        "        \n"
        "        float fSign = sign( i + 1 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;\n"
        "        \n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;\n"
        "        vTexCoord.x += fInvTexSize;            \n"
        "        \n"
        "        fSign = sign( i + 2 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;        \n"
        "        i+=2;\n"
        "    }\n"
        "        \n"
        "    ColorOut = fTotalColor;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    node = New(0x44, kFragmentNodeConstruct);
    SetFixed(kSetType, node, "Pixel");
    SetFixed(kNodeSetName, node, "VertBlurSample");
    SetString(kNodeSetDescription, node,
        "\n"
        "    This fragment is responsible for sampling a texture and returning its value\n"
        "    as a RGB value.\n"
        "    ");
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "TexCoord");
    SetFixed(kResourceSetVariable, res, "TexCoord");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "sampler2D");
    SetFixed(kResourceSetSemantic, res, "Texture");
    SetFixed(kResourceSetVariable, res, "Sampler");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "bool");
    SetFixed(kResourceSetVariable, res, "Saturate");
    SetFixed(kResourceSetDefaultValue, res, "(false)");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float2");
    SetFixed(kResourceSetSemantic, res, "TexSizeBase");
    SetFixed(kResourceSetVariable, res, "MapSize");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float");
    SetFixed(kResourceSetVariable, res, "KernelSize");
    Call(kNodeAddInputResource, node, res);
    res = New(0x2C, kResourceConstruct);
    SetFixed(kSetType, res, "float4");
    SetFixed(kResourceSetSemantic, res, "Color");
    SetFixed(kResourceSetVariable, res, "ColorOut");
    Call(kNodeAddOutputResource, node, res);
    block = New(0x18, kCodeBlockConstruct);
    SetFixed(kBlockSetLanguage, block, "hlsl/Cg");
    SetFixed(kBlockSetPlatform, block, "D3D10/DX9/Xenon/PS3");
    SetFixed(kBlockSetTarget, block, "ps_2_0/vs_4_0/ps_4_0");
    SetString(kBlockSetText, block,
        "\n"
        "\n"
        "    // Blurs the texel at vTexCoord in the Y direction.   \n"
        "    float fInvTexSize = 1.0 / MapSize.y;\n"
        "    float fHalfBlurKernelSize = KernelSize * 0.5f;\n"
        "    float fInvTotalWeights = 1.0 / (KernelSize * KernelSize);    \n"
        "    float fBlurWeight = 2;\n"
        "   \n"
        "    float4 fTotalColor = 0;   \n"
        "    float2 vTexCoord = TexCoord;\n"
        "    vTexCoord.y -= fInvTexSize * fHalfBlurKernelSize;    \n"
        "    float i = 0;\n"
        "\n"
        "    // Blur x2\n"
        "    if ( 2 <= KernelSize)    \n"
        "    {\n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;        \n"
        "        vTexCoord.y += fInvTexSize;\n"
        "        \n"
        "        float fSign = sign( i + 1 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;\n"
        "        \n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;\n"
        "        vTexCoord.y += fInvTexSize;            \n"
        "        \n"
        "        fSign = sign( i + 2 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;        \n"
        "        i+=2;\n"
        "    }\n"
        "    \n"
        "    // Blur x4\n"
        "    if ( 4 <= KernelSize)    \n"
        "    {\n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;        \n"
        "        vTexCoord.y += fInvTexSize;\n"
        "        \n"
        "        float fSign = sign( i + 1 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;\n"
        "        \n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;\n"
        "        vTexCoord.y += fInvTexSize;            \n"
        "        \n"
        "        fSign = sign( i + 2 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;        \n"
        "        i+=2;\n"
        "    }    \n"
        "\n"
        "    // Blur x6\n"
        "    if ( 6 <= KernelSize)    \n"
        "    {\n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;        \n"
        "        vTexCoord.y += fInvTexSize;\n"
        "        \n"
        "        float fSign = sign( i + 1 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;\n"
        "        \n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;\n"
        "        vTexCoord.y += fInvTexSize;            \n"
        "        \n"
        "        fSign = sign( i + 2 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;        \n"
        "        i+=2;\n"
        "    }\n"
        "\n"
        "    // Blur x8\n"
        "    if ( 8 <= KernelSize)    \n"
        "    {\n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;        \n"
        "        vTexCoord.y += fInvTexSize;\n"
        "        \n"
        "        float fSign = sign( i + 1 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;\n"
        "        \n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;\n"
        "        vTexCoord.y += fInvTexSize;            \n"
        "        \n"
        "        fSign = sign( i + 2 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;        \n"
        "        i+=2;\n"
        "    }\n"
        "\n"
        "    // Blur x10\n"
        "    if ( 10 <= KernelSize)    \n"
        "    {\n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;        \n"
        "        vTexCoord.y += fInvTexSize;\n"
        "        \n"
        "        float fSign = sign( i + 1 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;\n"
        "        \n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;\n"
        "        vTexCoord.y += fInvTexSize;            \n"
        "        \n"
        "        fSign = sign( i + 2 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;        \n"
        "        i+=2;\n"
        "    }\n"
        "\n"
        "    // Blur x12\n"
        "    if ( 12 <= KernelSize)    \n"
        "    {\n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;        \n"
        "        vTexCoord.y += fInvTexSize;\n"
        "        \n"
        "        float fSign = sign( i + 1 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;\n"
        "        \n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;\n"
        "        vTexCoord.y += fInvTexSize;            \n"
        "        \n"
        "        fSign = sign( i + 2 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;        \n"
        "        i+=2;\n"
        "    }\n"
        "    \n"
        "    // Blur x14\n"
        "    if ( 14 <= KernelSize)    \n"
        "    {\n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;        \n"
        "        vTexCoord.y += fInvTexSize;\n"
        "        \n"
        "        float fSign = sign( i + 1 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;\n"
        "        \n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;\n"
        "        vTexCoord.y += fInvTexSize;            \n"
        "        \n"
        "        fSign = sign( i + 2 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;        \n"
        "        i+=2;\n"
        "    }    \n"
        "\n"
        "    // Blur x16\n"
        "    if ( 16 <= KernelSize)    \n"
        "    {\n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;        \n"
        "        vTexCoord.y += fInvTexSize;\n"
        "        \n"
        "        float fSign = sign( i + 1 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;\n"
        "        \n"
        "        fTotalColor += tex2D(Sampler, vTexCoord) * \n"
        "            fBlurWeight * fInvTotalWeights;\n"
        "        vTexCoord.y += fInvTexSize;            \n"
        "        \n"
        "        fSign = sign( i + 2 - fHalfBlurKernelSize);\n"
        "        fBlurWeight -= fSign * 4;        \n"
        "        i+=2;\n"
        "    }\n"
        "\n"
        "    ColorOut = fTotalColor;\n"
        "    ");
    Call(kNodeAddCodeBlock, node, block);
    Call(kLibraryAddNode, lib, node);
    return reinterpret_cast<NiRefObject*>(lib);
}

} // namespace ni

#define NIMATLIB_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

NIMATLIB_REVERSED("NiMaterialLibraries::CreateStandardMaterialNodeLibrary", 0x00798130, &ni::CreateStandardMaterialNodeLibrary);
NIMATLIB_REVERSED("NiMaterialLibraries::CreatePointShadowWriteLibrary", 0x007B06F0, &ni::CreatePointShadowWriteLibrary);
NIMATLIB_REVERSED("NiMaterialLibraries::CreateSpotShadowWriteLibrary", 0x007BA720, &ni::CreateSpotShadowWriteLibrary);
NIMATLIB_REVERSED("NiMaterialLibraries::CreateDirectionalShadowWriteLibrary", 0x007C4B90, &ni::CreateDirectionalShadowWriteLibrary);
NIMATLIB_REVERSED("NiMaterialLibraries::CreateShadowClickGeneratorLibrary", 0x007ECED0, &ni::CreateShadowClickGeneratorLibrary);
