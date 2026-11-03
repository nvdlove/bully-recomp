#pragma once

// NiFragmentMaterial (vtable 0x9551B8, 0x3B8 bytes, from its constructor 0x788260) and
// NiStandardMaterial (vtable 0x955E60, 0x3C0 bytes, constructors 0x797F30 / 0x797F90): the
// materials that build their shaders from fragments of node libraries (src/gamebryo/
// NiMaterialLibraries.cpp), linked by a configurator into vertex and pixel programs, kept in a
// hash of 37 buckets by descriptor (and the descriptors that failed in another 37). With the
// pieces they use: the descriptors (0x14), the render pass descriptors (0x58) and their object
// offsets (0xC), the generation context (0x14), the material nodes' resources (0x2C).
//
// The node API (NiMaterialNode, NiMaterialResource, NiMaterialConfigurator, other chunks')
// called through its addresses, in ni::frag (as NiSceneCommon.h's helpers are in ni::scene).

#include <cstddef>
#include <cstdint>
#include <intrin.h>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiMaterial.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {

// NiMaterialDescriptor / NiGPUProgramDescriptor: an identifier and four words of bits (0x7D5470
// makes one, 0x7D5490 copies one, 0x7D5440 compares; 0x7E2B90 prints the bits)
struct NiMaterialDescriptor {
    re::Ptr32<const char> identifier;   // +0x0
    uint32_t bits[4];           // +0x4
};
static_assert(sizeof(NiMaterialDescriptor) == 0x14);

// NiTSet / NiTObjectSet / NiTPrimitiveSet: the elements, how many are allocated, how many used
template <typename T>
struct NiTSet {
    re::Ptr32<re::Field<T>> base;              // +0x0
    uint32_t allocated;   // +0x4
    uint32_t count;       // +0x8
};
static_assert(sizeof(NiTSet<void*>) == 0xC);

// NiFragmentMaterial::ObjectResourceOffset (0x786260 makes one: program type 3, none)
struct ObjectResourceOffset {
    uint32_t programType;   // +0x0 (0 vertex, 1 pixel, 2 geometry; 3 none)
    uint32_t objectType;    // +0x4 (NiShaderAttributeDesc::ObjectType)
    uint32_t offset;        // +0x8
};
static_assert(sizeof(ObjectResourceOffset) == 0xC);

// NiFragmentMaterial::RenderPassDescriptor (0x787A90 makes one, 0x786CA0 destroys it, 0x787AE0
// copies it)
struct RenderPassDescriptor {
    NiMaterialDescriptor vertexDesc;              // +0x0
    NiMaterialDescriptor geometryDesc;            // +0x14
    NiMaterialDescriptor pixelDesc;               // +0x28
    uint8_t usesNiRenderState;                    // +0x3C
    uint8_t alphaOverride;                        // +0x3D
    uint8_t alphaBlend;                           // +0x3E
    uint8_t resetObjectOffsets;                   // +0x3F
    NiTSet<ObjectResourceOffset> objectOffsets;   // +0x40 (allocated with NiNew[], 0x786770)
    uint8_t usePreviousSrcBlendMode;              // +0x4C
    uint8_t usePreviousDestBlendMode;             // +0x4D
    uint8_t pad4E[2];
    uint32_t srcBlendMode;                        // +0x50 (6: source alpha)
    uint32_t destBlendMode;                       // +0x54 (7: inverse source alpha)
};
static_assert(offsetof(RenderPassDescriptor, usesNiRenderState) == 0x3C);
static_assert(offsetof(RenderPassDescriptor, objectOffsets) == 0x40);
static_assert(offsetof(RenderPassDescriptor, srcBlendMode) == 0x50);
static_assert(sizeof(RenderPassDescriptor) == 0x58);

// NiFragmentMaterial's slots past NiMaterial's (3 IsShaderCurrent, 4 GetCurrentShader, 5
// GetVertexInputSemantics, 6 UnloadShaders, 7 SetWorkingDirectory):
//   8 PrecacheGPUPrograms(descriptor), 9 PrecacheGPUPrograms(geometry...), 10 SetProgramCache,
//   11 GetProgramCache, 12 SaveProgramCaches, 14/15 the descriptor's hash, 16
//   GetAttributeTypeFromString, 17 GetStringFromAttributeType, 18 GenerateDescriptor, 19
//   GenerateShader, 20 SetupShaderPrograms, 21..23 the vertex, geometry and pixel programs, 24
//   GenerateShaderDescArray, 25 VerifyShaderPrograms, 26 AddResourceToShaderConstantMap, 27
//   CreateShader, 28 SetupPackingRequirements, 29 GenerateUVSetName, 30 InsertTextureSampler,
//   32 AddOutputObject, 33 AddOutputAttribute, 34 AddOutputGlobal, 35
//   GetAttachableNodeFromLibrary
struct NiFragmentMaterial : NiMaterial {
    NiTSet<NiRefObject*> libraries;                    // +0xC (NiTObjectSet<NiMaterialNodeLibraryPtr>)
    re::Ptr32<NiRefObject> programCaches[3];                     // +0x18 (NiGPUProgramCachePtr by program type)
    NiTSet<NiRefObject*> shaders[37];                  // +0x24 (NiTObjectSet<NiShaderPtr>: the hash)
    NiTSet<NiMaterialDescriptor> failedDescriptors[37];   // +0x1E0 (allocated with NiNew[], 0x7867B0)
    NiTArray<void*> fallbacks;                         // +0x39C (NiTPrimitiveArray of SHADERFALLBACKFUNC)
    uint32_t maxPassCount;                             // +0x3AC (10)
    uint16_t vertexVersion;                            // +0x3B0
    uint16_t geometryVersion;                          // +0x3B2
    uint16_t pixelVersion;                             // +0x3B4
    uint16_t pad3B6;
};
static_assert(offsetof(NiFragmentMaterial, libraries) == 0xC);
static_assert(offsetof(NiFragmentMaterial, programCaches) == 0x18);
static_assert(offsetof(NiFragmentMaterial, shaders) == 0x24);
static_assert(offsetof(NiFragmentMaterial, failedDescriptors) == 0x1E0);
static_assert(offsetof(NiFragmentMaterial, fallbacks) == 0x39C);
static_assert(offsetof(NiFragmentMaterial, maxPassCount) == 0x3AC);
static_assert(offsetof(NiFragmentMaterial, vertexVersion) == 0x3B0);
static_assert(sizeof(NiFragmentMaterial) == 0x3B8);

// NiStandardMaterial's slots past NiFragmentMaterial's (this chunk's): 37 HandleNormals, 38 an
// empty hook, 40 HandleViewVector, 41 HandleReflectionVector, 43 HandleMaterialColors, 46/47
// the texture coordinate generation, 49 HandleShadingCoefficients, 52..62 the texture maps
// (parallax, base, dark, detail, gloss, bump, glow, custom, environment, decal), 63/64 the
// projected lights and shadows, 65 HandleLights, 70..77 the vector operators, 78/79
// InsertTexture (by name / by map), 80 SplitColorAndOpacity, 81 SplitRGBA, 82 TypeCastBind;
// 31 AddOutputPredefined and 66/67 (a light's shadow and lighting) are the next chunk's
struct NiStandardMaterial : NiFragmentMaterial {
    re::Ptr32<const char> descriptorName;   // +0x3B8 ("NiStandardMaterialDescriptor")
    uint8_t unknown3BC;             // +0x3BC (set: never per-vertex lighting)
    uint8_t unknown3BD;             // +0x3BD (the shading coefficients saturated)
    uint8_t unknown3BE;             // +0x3BE (the texture samples saturated)
    uint8_t pad3BF;
};
static_assert(offsetof(NiStandardMaterial, descriptorName) == 0x3B8);
static_assert(sizeof(NiStandardMaterial) == 0x3C0);

// NiFragmentMaterial::Context: the configurator and the nodes that stand for the program's
// statics, uniforms, inputs (resource producers) and outputs (a resource consumer); 0x78A030
// releases them
struct MaterialContext {
    re::Ptr32<NiRefObject> configurator;   // +0x0 (NiMaterialConfiguratorPtr: its nodes an array at +8)
    re::Ptr32<NiRefObject> statics;        // +0x4
    re::Ptr32<NiRefObject> uniforms;       // +0x8
    re::Ptr32<NiRefObject> inputs;         // +0xC
    re::Ptr32<NiRefObject> outputs;        // +0x10
};
static_assert(sizeof(MaterialContext) == 0x14);

// NiMaterialResource (0x2C bytes): type +0x8, semantic +0xC, label +0x10, variable +0x14 (all
// NiFixedStrings), default value +0x18, count +0x1C, source +0x20 (1 predefined, 2 object,
// 4 attribute, 5 global), extra data +0x24, object type +0x28. NiMaterialNode: type +0x8, name
// +0xC, its inputs (an NiTArray at +0x10) and outputs (+0x20).

namespace frag {

using ni::scene::At;
using ni::scene::FixedStringRelease;
using ni::scene::NullFixedString;
using ni::scene::Virtual;

// NiFixedString (the game's helpers)
GAME_FN(const char*, __cdecl, String, 0x00754760, const char* text);   // (a native return: a Ptr32 by value is returned through memory on x64)   // a new handle
GAME_FN(re::Ptr32<const char>*, __thiscall, FixedString_Construct, 0x00401050, re::Ptr32<const char>* self, const char* text);
GAME_FN(re::Ptr32<const char>*, __thiscall, FixedString_Copy, 0x00401080, re::Ptr32<const char>* self, const re::Ptr32<const char>* other);
GAME_FN(re::Ptr32<const char>*, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<const char>* self, const re::Ptr32<const char>* other);
GAME_FN(void, __thiscall, FixedString_Destruct, 0x00517750, re::Ptr32<const char>* self);
GAME_FN(void, __thiscall, FixedString_AssignText, 0x00406730, re::Ptr32<const char>* self, const char* text);
GAME_FN(uint8_t, __thiscall, FixedString_Equals, 0x00406480, const re::Ptr32<const char>* self, const char* text);
GAME_FN(uint8_t, __thiscall, FixedString_EqualsNoCase, 0x004064E0, const re::Ptr32<const char>* self, const char* text);

// the descriptors
GAME_FN(NiMaterialDescriptor*, __thiscall, Descriptor_Construct, 0x007D5470, NiMaterialDescriptor* self);
GAME_FN(NiMaterialDescriptor*, __thiscall, Descriptor_Assign, 0x007D5490, NiMaterialDescriptor* self,
        const NiMaterialDescriptor* other);
GAME_FN(uint8_t, __thiscall, Descriptor_Equals, 0x007D5440, const NiMaterialDescriptor* self,
        const NiMaterialDescriptor* other);
GAME_FN(uint8_t, __thiscall, Descriptor_ToString, 0x007E2B90, const NiMaterialDescriptor* self, char* buffer,
        uint32_t size);

// NiMaterialResource's getters
GAME_FN(const re::Ptr32<const char>*, __thiscall, Resource_GetType, 0x0077C280, void* self);        // +0x8
GAME_FN(const re::Ptr32<const char>*, __thiscall, Resource_GetSemantic, 0x008BAC30, void* self);    // +0xC
GAME_FN(const re::Ptr32<const char>*, __thiscall, Resource_GetVariable, 0x007E2BD0, void* self);    // +0x14
GAME_FN(uint32_t, __thiscall, Resource_GetCount, 0x0077E8E0, void* self);                   // +0x1C
GAME_FN(uint32_t, __thiscall, Resource_GetSource, 0x007E2BE0, void* self);                  // +0x20
GAME_FN(uint32_t, __thiscall, Resource_GetExtraData, 0x008C3450, void* self);               // +0x24
GAME_FN(uint32_t, __thiscall, Resource_GetObjectType, 0x0077E8F0, void* self);              // +0x28

// NiMaterialNode's
GAME_FN(void*, __thiscall, Node_GetInputResourceByVariableName, 0x007E2E70, void* self, const re::Ptr32<const char>* name);
GAME_FN(void*, __thiscall, Node_GetOutputResourceByVariableName, 0x007E2EE0, void* self, const re::Ptr32<const char>* name);
// AddOutputResource(type, semantic, label, variable, count, source, object type, extra data): the
// existing output of that variable, else a new one
GAME_FN(void*, __thiscall, Node_AddOutputResource, 0x007E3280, void* self, const re::Ptr32<const char>* type,
        const re::Ptr32<const char>* semantic, const re::Ptr32<const char>* label, const re::Ptr32<const char>* variable, uint32_t count,
        uint32_t source, uint32_t objectType, uint32_t extraData);
// AddOutputConstant(type, value): a new static output
GAME_FN(void*, __thiscall, Node_AddOutputConstant, 0x007E3370, void* self, const re::Ptr32<const char>* type,
        const re::Ptr32<const char>* value);
// NiMaterialNodeLibrary's
GAME_FN(uint16_t, __thiscall, Library_GetVersion, 0x007E2A50, void* self);
GAME_FN(void*, __thiscall, Library_GetAttachableNodeByName, 0x007E2A60, void* self, const re::Ptr32<const char>* name,
        int clone);

// NiMaterialConfigurator's: AddNode (the NiTObjectArray<NiMaterialNodePtr>::Add every node array
// shares), AddBinding(output, input), AddBinding(output, the node's input of that variable)
GAME_FN(void, __thiscall, Configurator_AddNode, 0x00856400, void* self, void* node);
GAME_FN(uint8_t, __thiscall, Configurator_AddBinding, 0x007E9520, void* self, void* output, void* input);
GAME_FN(uint8_t, __thiscall, Configurator_AddBindingByName, 0x007E9630, void* self, void* output,
        const re::Ptr32<const char>* inputName, void* node);

// the NiTObjectSet<NiPointer<...>> every pointer set shares: RemoveAll (references let go), the
// NiNew[]'d array's delete[], the reallocation
GAME_FN(void, __thiscall, ObjectSet_RemoveAll, 0x00836940, void* set);
GAME_FN(void, __cdecl, ObjectSet_DeleteArray, 0x00836F80, void* base);
GAME_FN(void, __thiscall, ObjectSet_Realloc, 0x007E8680, void* set, uint32_t allocated);

// NiMemObject's array operators
GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
// the compiler's array helpers (eh vector constructor / destructor iterators)
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* ctor);
GAME_FN(void, __stdcall, VectorDestruct, 0x004017C0, void* array, uint32_t size, uint32_t count, void* dtor);

// a temporary handle let go inline (as most of the code has it)
inline void Drop(re::Ptr32<const char>& s) {
    FixedStringRelease(&s);
}

// the material's own virtuals, by slot
inline void* GetAttachableNodeFromLibrary(void* material, const re::Ptr32<const char>* name) {
    return Virtual<void*>(material, 35, name);
}

// ---- the idioms the node-building code repeats. The temporary names are made and let go the
// three ways the game's code does it: made with 0x754760 and let go inline, constructed with
// 0x401050 and destructed with 0x517750, or made with 0x754760 and destructed with 0x517750

enum class Temp { Inline, Constructed, Destructed };

inline NiFixedString MakeTemp(Temp kind, const char* text) {
    NiFixedString s;
    if (kind == Temp::Constructed) {
        FixedString_Construct(&s, text);
    } else {
        s = String(text);
    }
    return s;
}

inline void DropTemp(Temp kind, re::Ptr32<const char>& s) {
    if (kind == Temp::Inline) {
        Drop(s);
    } else {
        FixedString_Destruct(&s);
    }
}

// a clone of the libraries' node of that name (slot 35)
inline void* LibraryNode(void* material, const char* name, Temp kind = Temp::Inline) {
    NiFixedString s = MakeTemp(kind, name);
    void* node = GetAttachableNodeFromLibrary(material, &s);
    DropTemp(kind, s);
    return node;
}

inline void AddNode(MaterialContext* context, void* node) {
    Configurator_AddNode(context->configurator, node);
}

// the configurator's binding of an output to the node's input of that name
inline uint8_t BindInput(MaterialContext* context, void* output, void* node, const char* input,
                         Temp kind = Temp::Inline) {
    NiFixedString s = MakeTemp(kind, input);
    void* configurator = context->configurator;
    void* to = Node_GetInputResourceByVariableName(node, &s);
    uint8_t bound = Configurator_AddBinding(configurator, output, to);
    DropTemp(kind, s);
    return bound;
}

// the node's output of that name
inline void* OutputOf(void* node, const char* output, Temp kind = Temp::Inline) {
    NiFixedString s = MakeTemp(kind, output);
    void* resource = Node_GetOutputResourceByVariableName(node, &s);
    DropTemp(kind, s);
    return resource;
}

// a new output on a producer node (the inputs, the uniforms): its names made variable first and
// let go type first
inline void* AddResource(void* node, const char* type, const char* semantic, const char* label, const char* variable,
                         uint32_t count = 1, uint32_t source = 0, uint32_t objectType = 0, uint32_t extraData = 0) {
    NiFixedString v = String(variable);
    NiFixedString l = String(label);
    NiFixedString s = String(semantic);
    NiFixedString t = String(type);
    void* resource = Node_AddOutputResource(node, &t, &s, &l, &v, count, source, objectType, extraData);
    Drop(t);
    Drop(s);
    Drop(l);
    Drop(v);
    return resource;
}

// a static constant (statics' AddOutputConstant) bound to the node's input: the names made
// input, value, type and let go the other way round; the input looked up first
inline uint8_t BindConstant(MaterialContext* context, void* node, const char* input, const char* type,
                            const char* value) {
    NiFixedString i = String(input);
    NiFixedString v = String(value);
    NiFixedString t = String(type);
    void* to = Node_GetInputResourceByVariableName(node, &i);
    void* constant = Node_AddOutputConstant(context->statics, &t, &v);
    uint8_t bound = Configurator_AddBinding(context->configurator, constant, to);
    Drop(t);
    Drop(v);
    Drop(i);
    return bound;
}

// the same through AddBinding by the input's name (the constant made first)
inline uint8_t BindConstantByName(MaterialContext* context, void* node, const char* input, const char* type,
                                  const char* value) {
    NiFixedString i = String(input);
    NiFixedString v = String(value);
    NiFixedString t = String(type);
    void* constant = Node_AddOutputConstant(context->statics, &t, &v);
    uint8_t bound = Configurator_AddBindingByName(context->configurator, constant, &i, node);
    Drop(t);
    Drop(v);
    Drop(i);
    return bound;
}

// whether a resource's type is that HLSL type
inline bool TypeIs(void* resource, const char* type) {
    return FixedString_Equals(Resource_GetType(resource), type) != 0;
}

} // namespace frag
} // namespace ni
