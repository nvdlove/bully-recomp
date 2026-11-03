#pragma once

// Gamebryo's D3DX effect shader library (NiD3DXEffectShaderLib, statically linked), the layouts
// read from its constructors (0x7470B0, 0x747150, 0x747C50, 0x74A360, 0x74DD90, 0x748C20,
// 0x74D6D0, 0x74A810):
//
//   NiD3DShader (vtable 0x950668, 0x58): NiD3DShaderInterface (0x24: an NiShader with the device,
//     renderer and render state) plus the passes;
//   NiD3DXEffectShader (vtable 0x9502C0, 0x78): an NiD3DShader drawing one technique of an
//     ID3DXEffect;
//   NiD3DXEffectFile (0x54, no vtable): one effect, its technique sets by name, its parameters
//     and the attribute table the parser fills;
//   NiD3DXEffectTechniqueSet (vtable 0x950B08, 0x3C) and NiD3DXEffectTechnique (vtable 0x950A08,
//     0x48): the implementations of one shader name;
//   NiD3DXEffectParameter (0x4C, no vtable): one parameter, how it is filled each draw;
//   NiD3DXEffectFactory (vtable 0x950848, 0x14): the loaded files by name (the singleton);
//   NiD3DXEffectStateManager (vtable 0x950AB0, 0x14): the ID3DXEffectStateManager the effects
//     set their states through (the renderer's state cache);
//   the parser's parameter descriptions (static buffers, one per type, 0x20DDEE0..0x20E207C).
//
// Everything in ni::d3dx so nothing clashes with the other Gamebryo files.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::d3dx {

using ni::sys::At;
using ni::sys::Decrement;
using ni::sys::Import;
using ni::sys::Increment;
using ni::sys::Virtual;

// a COM call by vtable slot (stdcall, the interface first)
template <typename R = long, typename... A>
inline R Com(const void* object, int slot, A... args) {
    return re::ComCall<R>(object, slot, args...);
}

// a reference released as the library's NiPointer does (the import's InterlockedDecrement)
inline void ReleaseRef(void* object) {
    if (object != nullptr && Decrement(static_cast<uint8_t*>(object) + 4) == 0) {
        Virtual<void>(object, 1);
    }
}
inline void AddRefRef(void* object) {
    if (object != nullptr) {
        Increment(static_cast<uint8_t*>(object) + 4);
    }
}
// NiPointer::operator= (the old released first, then the new one held)
template <typename T>
inline void AssignRef(re::Ptr32<T>& slot, std::type_identity_t<T>* value) {
    if (slot != value) {
        ReleaseRef(slot);
        slot = value;
        AddRefRef(value);
    }
}

// ---- D3DX, through the exe's import slots (d3dx9_38.dll)
constexpr uintptr_t kD3DXCreateEffectCompiler = 0x008FC2CC;
constexpr uintptr_t kD3DXMatrixMultiply = 0x008FC2E4;
constexpr uintptr_t kD3DXCreateEffect = 0x008FC2E8;
constexpr uintptr_t kD3DXGetShaderInputSemantics = 0x008FC2F4;
constexpr uintptr_t kD3DXCreateTextureFromFileExA = 0x008FC2F8;
constexpr uintptr_t kD3DXCreateVolumeTextureFromFileExA = 0x008FC2FC;
constexpr uintptr_t kD3DXCreateCubeTextureFromFileExA = 0x008FC300;
constexpr uintptr_t kD3DXMatrixInverse = 0x008FC304;
constexpr uintptr_t kD3DXGetShaderVersion = 0x008FC308;
// (the file system's imports)
constexpr uintptr_t kCreateFileA = 0x008FC064;

// ID3DXEffect's slots (ID3DXBaseEffect's first)
enum EffectSlot : int {
    kQueryInterface = 0,
    kRelease = 2,
    kGetDesc = 3,
    kGetParameterDesc = 4,
    kGetTechniqueDesc = 5,
    kGetPassDesc = 6,
    kGetParameter = 8,
    kGetParameterByName = 9,
    kGetParameterElement = 11,
    kGetTechnique = 12,
    kGetPass = 14,
    kGetAnnotation = 18,
    kGetAnnotationByName = 19,
    kSetBool = 22,
    kGetBool = 23,
    kSetInt = 26,
    kGetInt = 27,
    kSetFloat = 30,
    kGetFloat = 31,
    kSetFloatArray = 32,
    kGetFloatArray = 33,
    kSetVector = 34,
    kGetVector = 35,
    kSetVectorArray = 36,
    kGetVectorArray = 37,
    kSetMatrix = 38,
    kGetMatrix = 39,
    kSetMatrixArray = 40,
    kGetMatrixArray = 41,
    kSetMatrixTranspose = 44,
    kGetMatrixTranspose = 45,
    kSetMatrixTransposeArray = 46,
    kGetString = 51,
    kSetTexture = 52,
    kGetTexture = 53,
    kSetTechnique = 58,
    kValidateTechnique = 60,
    kIsParameterUsed = 62,
    kBegin = 63,
    kBeginPass = 64,
    kCommitChanges = 65,
    kEndPass = 66,
    kEnd = 67,
    kOnLostDevice = 69,
    kOnResetDevice = 70,
    kSetStateManager = 71,
};

// IDirect3DDevice9's slots the code here uses
enum DeviceSlot : int {
    kDevSetRenderState = 57,
    kDevSetTexture = 65,
    kDevSetTextureStageState = 67,
    kDevSetSamplerState = 69,
    kDevSetVertexDeclaration = 87,
    kDevSetFVF = 89,
    kDevSetVertexShader = 92,
    kDevSetPixelShader = 107,
    kDevSetTransform = 44,
    kDevSetMaterial = 49,
    kDevSetLight = 51,
    kDevLightEnable = 53,
    kDevSetNPatchMode = 79,
    kDevSetVertexShaderConstantF = 94,
    kDevSetPixelShaderConstantF = 109,
};

// ---- the renderer pieces read here (NiDX9Renderer, NiDX9RenderState: other chunks')
GAME_VAR(uint8_t*, NiDX9Renderer_ms_pkRenderer, 0x020E2380);   // the renderer
// renderer: +0xA4 the device, +0x6B4 the render state, +0x6C0 the texture manager, +0x750 the
// world, +0x790 the view, +0x7D0 the projection (D3DMATRIX each), +0x810 the inverse view,
// +0x3F0/+0x3F1 the hardware / mixed vertex processing switches, +0x4F4 the device's reset
// count. render state: +0xC the vertex blend values by bone count, +0x120 the render states,
// +0x5D8 the vertex blend in force, +0x920 the texture stage states (33 per stage), +0x19A0
// the sampler states (14 per sampler), +0x20A0 the textures, +0x20E0 the vertex shader, +0x20E8
// the pixel shader, +0x20F0 the shader constant cache, +0x20F6 software vertex processing, +0x20F7
// a declaration (else an FVF) in force, +0x20F8 the FVF, +0x2100 the declaration, +0x2238 the
// device. A geometry's renderer data: +0xC its FVF, +0x10 its vertex declaration

// ---- NiD3DShader (vtable 0x950668)

struct NiD3DShader {
    re::Ptr32<void> vtable;                 // +0x0
    uint32_t refCount;            // +0x4
    re::Ptr32<const char> name;             // +0x8 (NiShader's: an NiFixedString)
    uint32_t implementation;      // +0xC
    uint8_t initialized;          // +0x10
    uint8_t pad11[3];
    re::Ptr32<void> d3dDevice;              // +0x14 (NiD3DShaderInterface's: a held reference)
    re::Ptr32<uint8_t> d3dRenderer;         // +0x18
    re::Ptr32<uint8_t> d3dRenderState;      // +0x1C
    uint8_t isBestImplementation; // +0x20
    uint8_t pad21[3];
    uint8_t usesNiRenderState;    // +0x24
    uint8_t usesNiLightState;     // +0x25
    uint8_t pad26[2];
    re::Ptr32<NiRefObject> shaderDecl;      // +0x28 (NiShaderDeclarationPtr)
    uint32_t unknown2C;           // +0x2C
    uint32_t unknown30;           // +0x30
    uint32_t unknown34;           // +0x34
    uint32_t currentPass;         // +0x38
    uint32_t passCount;           // +0x3C
    re::Ptr32<void> currentPassObject;      // +0x40
    NiTArray<uint8_t*> passes;    // +0x44 (NiTObjectArray<NiD3DPassPtr>, vtable 0x950660: a pass
                                  //  counts its references at +0x60, plainly)
    uint32_t unknown54;           // +0x54
};
static_assert(offsetof(NiD3DShader, d3dRenderer) == 0x18);
static_assert(offsetof(NiD3DShader, usesNiRenderState) == 0x24);
static_assert(offsetof(NiD3DShader, currentPass) == 0x38);
static_assert(offsetof(NiD3DShader, passes) == 0x44);
static_assert(sizeof(NiD3DShader) == 0x58);

struct NiD3DXEffectTechnique;

struct NiD3DXEffectShader : NiD3DShader {
    re::Ptr32<NiD3DXEffectTechnique> technique;   // +0x58
    re::Ptr32<const char> techniqueHandle;        // +0x5C (D3DXHANDLE)
    re::Ptr32<void> effect;                       // +0x60 (the file's ID3DXEffect, not held)
    re::Ptr32<void> d3dxEffect;                   // +0x64 (the same, QueryInterface'd)
    uint32_t techniqueIndex;            // +0x68 (its technique's index in the set)
    uint32_t boneMatrixRegisters;       // +0x6C (3 or 4: from the skinning parameter)
    uint32_t transposeBones;            // +0x70 (1: the bone matrices transposed)
    uint8_t softwareVertexProcessing;   // +0x74
    uint8_t pad75[3];
};
static_assert(offsetof(NiD3DXEffectShader, technique) == 0x58);
static_assert(offsetof(NiD3DXEffectShader, softwareVertexProcessing) == 0x74);
static_assert(sizeof(NiD3DXEffectShader) == 0x78);

// ---- the templates' layouts

// NiTStringMap / NiTStringPointerMap: vtable, hash size, buckets, count, whether keys are copies
struct MapItem {
    re::Ptr32<MapItem> next;    // +0x0
    re::Ptr32<char> key;        // +0x4
    re::Ptr32<void> value;      // +0x8
};
struct StringMap {
    re::Ptr32<void> vtable;        // +0x0
    uint32_t hashSize;   // +0x4
    re::Ptr32<re::Ptr32<MapItem>> table;     // +0x8
    uint32_t count;      // +0xC
    uint8_t copyKeys;    // +0x10
    uint8_t pad11[3];
};
static_assert(sizeof(StringMap) == 0x14);

using PointerList = NiTPointerList<void*>;
using ListNode = NiTListNode<void*>;

// ---- NiD3DXEffectAttributeTable (0x14): the attributes the parser found (NiShaderAttributeDesc
// references), with an iterator
struct NiD3DXEffectAttributeTable {
    re::Ptr32<ListNode> iterator;       // +0x0
    PointerList attributes;   // +0x4 (vtable 0x952E10)
};
static_assert(sizeof(NiD3DXEffectAttributeTable) == 0x14);

// ---- NiD3DXEffectFile

struct NiD3DXEffectFile {
    re::Ptr32<char> name;                           // +0x0 (NiMalloc'd)
    uint32_t unknown4;                    // +0x4 (2 once the factory loaded it)
    StringMap techniqueSets;              // +0x8 (NiTStringPointerMap<NiD3DXEffectTechniqueSetPtr>,
                                          //  vtable 0x95072C, 37 buckets, keys copied)
    PointerList parameters;               // +0x1C (NiD3DXEffectParameter*, vtable 0x952E10)
    re::Ptr32<MapItem> setIterator;                 // +0x2C
    uint32_t techniqueIterator;           // +0x30
    re::Ptr32<struct NiD3DXEffectTechniqueSet> currentSet;   // +0x34 (not held)
    NiD3DXEffectAttributeTable attributeTable;     // +0x38
    re::Ptr32<void> effect;                         // +0x4C (ID3DXEffect)
    uint8_t rendererDataCreated;          // +0x50 (the effect's handles and state manager set)
    uint8_t parsed;                       // +0x51
    uint8_t pad52[2];
};
static_assert(offsetof(NiD3DXEffectFile, techniqueSets) == 0x8);
static_assert(offsetof(NiD3DXEffectFile, parameters) == 0x1C);
static_assert(offsetof(NiD3DXEffectFile, attributeTable) == 0x38);
static_assert(offsetof(NiD3DXEffectFile, effect) == 0x4C);
static_assert(sizeof(NiD3DXEffectFile) == 0x54);

// ---- NiD3DXEffectTechnique (vtable 0x950A08)

struct NiD3DXEffectTechnique {
    re::Ptr32<void> vtable;                    // +0x0
    uint32_t refCount;               // +0x4
    re::Ptr32<char> techniqueName;             // +0x8 (the effect's technique name: a D3DXHANDLE too)
    uint32_t implementation;         // +0xC
    re::Ptr32<const char> handle;              // +0x10 (D3DXHANDLE)
    re::Ptr32<NiD3DXEffectFile> file;          // +0x14
    re::Ptr32<NiD3DXEffectShader> shader;      // +0x18 (a reference)
    PointerList parameters;          // +0x1C (NiD3DXEffectParameter*, vtable 0x952E10)
    re::Ptr32<ListNode> parameterIterator;     // +0x2C
    uint32_t passCount;              // +0x30
    re::Ptr32<uint8_t> passHasVertexShader;    // +0x34 (one per pass)
    re::Ptr32<uint8_t> passHasPixelShader;     // +0x38
    uint8_t usesTime;                // +0x3C (a time parameter: a time controller wanted)
    uint8_t usesNiRenderState;       // +0x3D
    uint8_t usesNiLightState;        // +0x3E
    uint8_t softwareVPRequired;      // +0x3F
    uint8_t softwareVPFallback;      // +0x40
    uint8_t pad41[3];
    re::Ptr32<void> requirements;              // +0x44 (NiShaderRequirementDesc: never freed)
};
static_assert(offsetof(NiD3DXEffectTechnique, shader) == 0x18);
static_assert(offsetof(NiD3DXEffectTechnique, parameters) == 0x1C);
static_assert(offsetof(NiD3DXEffectTechnique, usesTime) == 0x3C);
static_assert(offsetof(NiD3DXEffectTechnique, requirements) == 0x44);
static_assert(sizeof(NiD3DXEffectTechnique) == 0x48);

// ---- NiD3DXEffectTechniqueSet (vtable 0x950B08)

struct NiD3DXEffectTechniqueSet {
    re::Ptr32<void> vtable;                                // +0x0
    uint32_t refCount;                           // +0x4
    re::Ptr32<char> name;                                  // +0x8
    uint8_t unknownC[0x18];                      // +0xC (never written)
    NiTArray<NiD3DXEffectTechnique*> techniques; // +0x24 (NiTObjectArray of references, by
                                                 //  implementation; vtable 0x9893C0)
    re::Ptr32<NiD3DXEffectTechnique> best;                 // +0x34 (the one in use)
    re::Ptr32<NiRefObject> shaderDesc;                     // +0x38 (NiShaderDescPtr)
};
static_assert(offsetof(NiD3DXEffectTechniqueSet, techniques) == 0x24);
static_assert(offsetof(NiD3DXEffectTechniqueSet, best) == 0x34);
static_assert(sizeof(NiD3DXEffectTechniqueSet) == 0x3C);

// ---- NiD3DXEffectParameter

struct NiD3DXEffectParameter {
    re::Ptr32<const char> handle;          // +0x0 (D3DXHANDLE)
    re::Ptr32<const char> name;            // +0x4 (NiFixedString)
    uint8_t global;              // +0x8 (a global shader constant)
    uint8_t pad9[3];
    uint32_t predefined;         // +0xC (an NiShaderConstantMap predefined mapping)
    uint32_t objectMapping;      // +0x10 (an object parameter's NiShaderConstantMap object mapping)
    uint32_t type;               // +0x14 (the description's: 1 matrix .. 9 array)
    uint32_t textureType;        // +0x18 (1 2D, 2 volume, 3 cube)
    uint32_t arrayCount;         // +0x1C
    uint32_t maxCount;           // +0x20 (a predefined one's)
    uint32_t index;              // +0x24 (object, map or NTM index)
    re::Ptr32<char> textureFile;           // +0x28
    re::Ptr32<char> textureTarget;         // +0x2C ("tx_1_0" by default: a procedural texture's)
    uint32_t textureFlags;       // +0x30 (NiTexturingProperty map: 0x200000 base .. 0x2000000 shader)
    re::Ptr32<void> texture;               // +0x34 (IDirect3DBaseTexture9 made from the file)
    uint32_t rows;               // +0x38 (a matrix's; a texture's width)
    uint32_t columns;            // +0x3C (a matrix's; a texture's height)
    uint32_t depth;              // +0x40
    uint32_t objectType;         // +0x44 (the object it reads: 1 general light .. 11 fog map)
    uint8_t used;                // +0x48 (some technique uses it)
    uint8_t pad49[3];
};
static_assert(offsetof(NiD3DXEffectParameter, type) == 0x14);
static_assert(offsetof(NiD3DXEffectParameter, textureFile) == 0x28);
static_assert(offsetof(NiD3DXEffectParameter, objectType) == 0x44);
static_assert(sizeof(NiD3DXEffectParameter) == 0x4C);

// ---- NiD3DXEffectFactory (vtable 0x950848: 0 GetD3DXEffectFile, 1 InsertD3DXEffectFileIntoMap,
// 2 RemoveD3DXEffectFileFromMap, 3 the deleting destructor)

struct NiD3DXEffectFactory {
    re::Ptr32<void> vtable;               // +0x0
    re::Ptr32<StringMap> files;           // +0x4 (NiTStringPointerMap<NiD3DXEffectFile*>, vtable 0x9508CC)
    re::Ptr32<void> macros;               // +0x8 (D3DXMACRO[4], NiMalloc'd)
    uint32_t lostDeviceID;      // +0xC (the renderer's reset count the files were lost at)
    uint32_t resetDeviceID;     // +0x10
};
static_assert(sizeof(NiD3DXEffectFactory) == 0x14);

GAME_VAR(NiD3DXEffectFactory*, NiD3DXEffectFactory_ms_pkFactory, 0x020DCE44);
GAME_VAR(char*, NiD3DXEffectFactory_ms_pcEffectDirectory, 0x020DCE48);
GAME_VAR(uint8_t*, NiD3DXEffectFactory_ms_pkD3DRenderer, 0x020DCE4C);
GAME_VAR(void*, NiD3DXEffectFactory_ms_pkD3DDevice, 0x020DCE50);   // IDirect3DDevice9, held
GAME_VAR(uint32_t, NiD3DXEffectFactory_ms_uiCompileFlags, 0x020DCE54);
GAME_VAR(uint32_t, NiD3DXEffectFactory_ms_uiExtraFlags, 0x020E6EA0);
GAME_VAR(uint8_t, NiD3DXEffectFactory_ms_bUseFallbacks, 0x00BC74E0);

// ---- NiD3DXEffectStateManager (vtable 0x950AB0): the ID3DXEffectStateManager

struct NiD3DXEffectStateManager {
    re::Ptr32<void> vtable;                // +0x0
    int32_t highestTexture;      // +0x4 (the highest texture stage set this pass)
    uint32_t refCount;           // +0x8
    re::Ptr32<uint8_t> renderState;        // +0xC (NiDX9RenderState)
    re::Ptr32<void> device;                // +0x10 (IDirect3DDevice9, held)
};
static_assert(sizeof(NiD3DXEffectStateManager) == 0x14);

GAME_VAR(NiD3DXEffectStateManager*, NiD3DXEffectStateManager_ms_pkManager, 0x020DDEA8);

// ---- the parser's parameter descriptions (the type first: 1 matrix, 2 vector, 3 float, 4 bool,
// 5 integer, 6 texture, 7 predefined, 8 object, 9 array)

struct ParameterDesc {
    uint32_t type;             // +0x0
    uint8_t global;            // +0x4
    uint8_t pad5[3];
    re::Ptr32<const char> name;          // +0x8
    re::Ptr32<const char> description;   // +0xC
};

struct MatrixDesc : ParameterDesc {   // 0x20E2020
    float value[16];           // +0x10
    uint32_t rows;             // +0x50
    uint32_t columns;          // +0x54
    uint8_t hidden;            // +0x58
    uint8_t pad59[3];
};
static_assert(sizeof(MatrixDesc) == 0x5C);

struct VectorDesc : ParameterDesc {   // 0x20E1FD0
    float value[4];            // +0x10
    float maximum[4];          // +0x20
    float minimum[4];          // +0x30
    uint32_t rows;             // +0x40
    uint32_t columns;          // +0x44
    uint32_t elements;         // +0x48
    uint8_t hidden;            // +0x4C
    uint8_t color;             // +0x4D
    uint8_t pad4E[2];
};
static_assert(sizeof(VectorDesc) == 0x50);

struct FloatDesc : ParameterDesc {    // 0x20E1FB0
    float value;               // +0x10
    float maximum;             // +0x14
    float minimum;             // +0x18
    uint8_t hidden;            // +0x1C
    uint8_t pad1D[3];
};
static_assert(sizeof(FloatDesc) == 0x20);

struct BoolDesc : ParameterDesc {     // 0x20E1F34
    uint8_t value;             // +0x10
    uint8_t hidden;            // +0x11
    uint8_t pad12[2];
};
static_assert(sizeof(BoolDesc) == 0x14);

struct IntegerDesc : ParameterDesc {  // 0x20E1F90
    uint32_t value;            // +0x10
    uint32_t maximum;          // +0x14
    uint32_t minimum;          // +0x18
    uint8_t hidden;            // +0x1C
    uint8_t pad1D[3];
};
static_assert(sizeof(IntegerDesc) == 0x20);

struct TextureDesc : ParameterDesc {  // 0x20E1F48
    re::Ptr32<void> texture;             // +0x10 (the effect's texture, GetTexture'd)
    uint8_t hidden;            // +0x14
    uint8_t pad15[3];
    uint32_t textureType;      // +0x18 (1 2D, 2 volume, 3 cube)
    re::Ptr32<const char> file;          // +0x1C ("name")
    re::Ptr32<const char> function;      // +0x20
    re::Ptr32<const char> target;        // +0x24
    uint32_t unknown28;        // +0x28
    uint32_t width;            // +0x2C
    uint32_t height;           // +0x30
    uint32_t depth;            // +0x34
    re::Ptr32<const char> ntm;           // +0x38
    uint32_t ntmIndex;         // +0x3C
    uint32_t objectType;       // +0x40
    uint32_t objectIndex;      // +0x44
};
static_assert(sizeof(TextureDesc) == 0x48);

struct PredefinedDesc {               // 0x20E1F18
    uint32_t type;             // +0x0 (7)
    uint8_t global;            // +0x4
    uint8_t pad5[3];
    re::Ptr32<const char> name;          // +0x8
    re::Ptr32<const char> semantic;      // +0xC
    uint32_t mapping;          // +0x10
    uint32_t count;            // +0x14
    uint32_t ntmIndex;         // +0x18
};
static_assert(sizeof(PredefinedDesc) == 0x1C);

struct ObjectDesc {                   // 0x20E1F00
    uint32_t type;             // +0x0 (8)
    uint8_t global;            // +0x4
    uint8_t pad5[3];
    re::Ptr32<const char> name;          // +0x8
    uint32_t mapping;          // +0xC (the object property's NiShaderConstantMap mapping)
    uint32_t objectType;       // +0x10
    uint32_t objectIndex;      // +0x14
};
static_assert(sizeof(ObjectDesc) == 0x18);

struct ArrayDesc : ParameterDesc {    // 0x20DDEE0
    float value[0x400];        // +0x10
    float maximum[0x400];      // +0x1010
    float minimum[0x400];      // +0x2010
    float scratch[0x400];      // +0x3010 (the values as D3DX returns them, before compacting)
    uint32_t rows;             // +0x4010
    uint32_t columns;          // +0x4014
    uint32_t elements;         // +0x4018
    uint8_t hidden;            // +0x401C
    uint8_t color;             // +0x401D
    uint8_t pad401E[2];
};
static_assert(offsetof(ArrayDesc, rows) == 0x4010);
static_assert(sizeof(ArrayDesc) == 0x4020);

// the technique description the parser fills (0x20E207C, 0x2C bytes)
struct TechniqueDesc {
    re::Ptr32<const char> shaderName;          // +0x0
    re::Ptr32<const char> techniqueName;       // +0x4
    re::Ptr32<const char> description;         // +0x8
    uint32_t implementation;         // +0xC
    uint32_t pixelShaderVersion;     // +0x10
    uint32_t vertexShaderVersion;    // +0x14
    uint32_t userVersion;            // +0x18
    uint32_t bonesPerPartition;      // +0x1C
    uint32_t nbtMethod;              // +0x20
    uint32_t nbtSourceUV;            // +0x24
    uint8_t usesNiRenderState;       // +0x28
    uint8_t usesNiLightState;        // +0x29
    uint8_t softwareVPRequired;      // +0x2A
    uint8_t softwareVPFallback;      // +0x2B
};
static_assert(sizeof(TechniqueDesc) == 0x2C);

// D3DX's own structures
// (D3DX's descriptions are the system's structures: native pointers; on Win32 the game's
// offsets below)
struct D3DXParameterDesc {            // D3DXPARAMETER_DESC
    const char* name;          // +0x0
    const char* semantic;      // +0x4
    uint32_t parameterClass;   // +0x8
    uint32_t parameterType;    // +0xC
    uint32_t rows;             // +0x10
    uint32_t columns;          // +0x14
    uint32_t elements;         // +0x18
    uint32_t annotations;      // +0x1C
    uint32_t structMembers;    // +0x20
    uint32_t flags;            // +0x24
    uint32_t bytes;            // +0x28
};
static_assert(sizeof(void*) != 4 || sizeof(D3DXParameterDesc) == 0x2C);

struct D3DXTechniqueDesc {            // D3DXTECHNIQUE_DESC
    const char* name;
    uint32_t passes;
    uint32_t annotations;
};

struct D3DXPassDesc {                 // D3DXPASS_DESC
    const char* name;
    uint32_t annotations;
    const uint32_t* vertexShaderFunction;
    const uint32_t* pixelShaderFunction;
};
static_assert(sizeof(void*) != 4 || sizeof(D3DXPassDesc) == 0x10);

} // namespace ni::d3dx
