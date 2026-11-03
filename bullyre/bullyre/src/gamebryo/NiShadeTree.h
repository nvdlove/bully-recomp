#pragma once

// What NiStandardMaterial's shader generation shares (its vertex and pixel program builders, its
// handlers, its fallbacks, and the shadow write materials built on it), past what
// NiFragmentMaterial.h has: the configurator's and the material nodes' functions, the program
// descriptors' getters, the material's static helpers, all at their addresses; and the idioms the
// builders repeat (a context node made, a resource named by four temporary names, an input named
// after a resource).
//
// The program descriptors (NiMaterialDescriptor's layout): the vertex one's words are V0..V3
// (bits[0..3]), the pixel one's P0..P3. A render pass descriptor holds both (+0x0, +0x28).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiFragmentMaterial.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::shade {

using namespace ni::frag;

GAME_VAR(void*, ms_pkShadowManager, 0x020E2410);
GAME_VAR(void*, ms_pkRenderer, 0x020E2380);

GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(int, __cdecl, NiStricmp, 0x00753820, const char* a, const char* b);

// NiPointer<T>::operator=(T*) on a slot (the new one held, the old one let go)
GAME_FN(void, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<NiRefObject>* slot, NiRefObject* value);

// NiString (a handle): its characters (0x773DB0 returns what it points at), its destructor
struct NiString {
    re::Ptr32<char> handle;
};
GAME_FN(const char*, __thiscall, NiString_GetChars, 0x00773DB0, NiString* self);
GAME_FN(void, __thiscall, NiString_Destruct, 0x00774240, NiString* self);

// ---- NiMaterialConfigurator (0x34 bytes): constructed over a program cache; its description an
// NiString at +0x28, its platform an NiFixedString at +0x2C, its program cache at +0x30. Slot 2
// Evaluate(name, program type, uniforms&): the program; slot 4 CanBindTypes
GAME_FN(NiRefObject*, __thiscall, Configurator_Construct, 0x007E92F0, void* self, NiRefObject* cache);
GAME_FN(void, __thiscall, Configurator_SetDescription, 0x007E80D0, void* self, const char* text);
GAME_FN(const re::Ptr32<const char>*, __thiscall, Configurator_GetPlatform, 0x007E80C0, void* self);
GAME_FN(void*, __thiscall, Configurator_GetProgramCache, 0x007E8330, void* self);
// a node of the configurator by its name (from an index)
GAME_FN(void*, __thiscall, Configurator_GetNode, 0x007E81C0, void* self, const re::Ptr32<const char>* name, uint32_t start);
// AddBinding(the node's output of that name, input)
GAME_FN(uint8_t, __thiscall, Configurator_AddBindingFromOutput, 0x007E96C0, void* self,
        const re::Ptr32<const char>* outputName, void* node, void* input);

// the resource producer (the inputs, uniforms, statics) and consumer (the outputs) nodes,
// NiMaterialNodes of 0x34 bytes: (name, type)
GAME_FN(NiRefObject*, __thiscall, ProducerNode_Construct, 0x007EA880, void* self, const re::Ptr32<const char>* name,
        const re::Ptr32<const char>* type);
GAME_FN(NiRefObject*, __thiscall, ConsumerNode_Construct, 0x007EA7D0, void* self, const re::Ptr32<const char>* name,
        const re::Ptr32<const char>* type);
// AddInputResource(type, semantic, label, variable, count, source, object type, extra data)
GAME_FN(void*, __thiscall, Node_AddInputResource, 0x007E31A0, void* self, const re::Ptr32<const char>* type,
        const re::Ptr32<const char>* semantic, const re::Ptr32<const char>* label, const re::Ptr32<const char>* variable, uint32_t count,
        uint32_t source, uint32_t objectType, uint32_t extraData);
GAME_FN(const re::Ptr32<const char>*, __thiscall, Resource_GetLabel, 0x007E2BC0, void* self);   // +0x10

// ---- the program descriptors
// their descriptions as an NiString (the configurator's description)
GAME_FN(NiString*, __thiscall, VertexDesc_ToString, 0x007E8020, NiMaterialDescriptor* self, NiString* result);
GAME_FN(NiString*, __thiscall, PixelDesc_ToString, 0x007E5CB0, NiMaterialDescriptor* self, NiString* result);
// the pixel program's UV set of its nth texture, and the UV sets it takes
GAME_FN(uint32_t, __thiscall, PixelDesc_GetUVSetForMap, 0x007E35C0, NiMaterialDescriptor* self, uint32_t map);
GAME_FN(uint32_t, __thiscall, PixelDesc_GetInputUVCount, 0x007E3880, NiMaterialDescriptor* self);
// the vertex program's output UV n: its source set and texgen
GAME_FN(void, __thiscall, VertexDesc_GetOutputUV, 0x007E5F20, NiMaterialDescriptor* self, uint32_t output,
        uint32_t* uvSet, uint32_t* texGen);
// the material descriptor's texture usage n: its UV set and transform (texgen)
GAME_FN(void, __thiscall, StandardDesc_GetTextureUsage, 0x007F80F0, NiMaterialDescriptor* self, uint32_t texture,
        uint32_t* uvSet, uint32_t* texGen);
// (the textures the descriptor has)
GAME_FN(uint32_t, __thiscall, StandardDesc_GetInputCount, 0x007779D0, NiMaterialDescriptor* self);

// ---- NiStandardMaterial's statics (chunk 05's and this one's)
GAME_FN(int32_t, __cdecl, MapIndexFromTextureEnum, 0x0078EDC0, uint32_t map, const NiMaterialDescriptor* desc);
GAME_FN(uint8_t, __cdecl, AssignTextureCoordinates, 0x007900F0, const uint32_t* uvSets, const uint32_t* texGens,
        uint32_t count, NiMaterialDescriptor* vertexDesc, NiMaterialDescriptor* pixelDesc);
GAME_FN(uint8_t, __cdecl, GetTextureNameFromTextureEnum, 0x0078EEE0, uint32_t map, re::Ptr32<const char>* name,
        uint32_t* occurrence);
GAME_FN(uint8_t, __cdecl, GetTextureTransformMappingFromEnum, 0x00789B30, uint32_t map, uint32_t* mapping,
        uint32_t* occurrence);
// the UV coordinates resource of a map (uvs[-1] when the pixel program hasn't it)
GAME_FN(void*, __cdecl, GetUVForMap, 0x00797650, uint32_t map, NiMaterialDescriptor* pixelDesc, re::Ptr32<void>* uvs);

// ---- NiFragmentMaterial's (chunk 05's)
GAME_FN(void, __thiscall, SetFailedGPUProgram, 0x00786520, void* self, uint32_t type, const NiMaterialDescriptor* desc);
GAME_FN(void, __thiscall, Context_Destruct, 0x0078A030, MaterialContext* self);
GAME_FN(RenderPassDescriptor*, __thiscall, RenderPass_Assign, 0x00787AE0, RenderPassDescriptor* self,
        const RenderPassDescriptor* other);
GAME_FN(uint32_t, __cdecl, GetHighestObjectOffset, 0x007866D0, uint32_t objectType, RenderPassDescriptor* passes,
        uint32_t pass);
GAME_FN(ObjectResourceOffset*, __thiscall, ObjectOffset_Construct, 0x00786260, ObjectResourceOffset* self);
GAME_FN(void, __thiscall, ObjectOffsets_Realloc, 0x007872C0, NiTSet<ObjectResourceOffset>* self, uint32_t allocated);
GAME_FN(void, __thiscall, ObjectOffsets_Add, 0x007879E0, NiTSet<ObjectResourceOffset>* self,
        const ObjectResourceOffset* element);

// ---- the idioms

// a context node (a producer or consumer): NiNew'd, constructed with its names (made type first, in
// the given way), held in the context's slot, then its names let go (the name first)
template <typename Ctor>
inline NiRefObject* NewContextNode(Ctor construct, Temp kind, const char* type, const char* name, re::Ptr32<NiRefObject>* slot) {
    void* memory = NiNew(0x34);
    NiRefObject* node = nullptr;
    NiFixedString t = nullptr;
    NiFixedString n = nullptr;
    if (memory != nullptr) {
        t = MakeTemp(kind, type);
        n = MakeTemp(kind, name);
        node = construct(memory, &n, &t);
    }
    Pointer_Assign(slot, node);
    if (memory != nullptr) {
        DropTemp(kind, n);
        DropTemp(kind, t);
    }
    return node;
}

// a node's input (0x7E31A0) or output (0x7E3280) resource named by four temporary names: made
// variable, label, semantic, type, let go the other way round
inline void* NamedResource(bool input, void* node, Temp kind, const char* type, const char* semantic,
                           const char* label, const char* variable, uint32_t count = 1, uint32_t source = 0,
                           uint32_t objectType = 0, uint32_t extraData = 0) {
    NiFixedString v = MakeTemp(kind, variable);
    NiFixedString l = MakeTemp(kind, label);
    NiFixedString s = MakeTemp(kind, semantic);
    NiFixedString t = MakeTemp(kind, type);
    void* resource = input ? Node_AddInputResource(node, &t, &s, &l, &v, count, source, objectType, extraData)
                           : Node_AddOutputResource(node, &t, &s, &l, &v, count, source, objectType, extraData);
    DropTemp(kind, t);
    DropTemp(kind, s);
    DropTemp(kind, l);
    DropTemp(kind, v);
    return resource;
}

// an input of the node for a resource: its type and label, the semantic and variable given (made
// variable first, let go semantic first)
inline void* InputFor(void* node, Temp kind, void* resource, const char* semantic, const char* variable) {
    NiFixedString v = MakeTemp(kind, variable);
    NiFixedString s = MakeTemp(kind, semantic);
    const re::Ptr32<const char>* label = Resource_GetLabel(resource);
    const re::Ptr32<const char>* type = Resource_GetType(resource);
    void* input = Node_AddInputResource(node, type, &s, label, &v, 1, 0, 0, 0);
    DropTemp(kind, s);
    DropTemp(kind, v);
    return input;
}

// the configurator's binding of an output to the node's input by its name
inline void BindByName(MaterialContext* context, void* output, void* node, const char* input, Temp kind) {
    NiFixedString s = MakeTemp(kind, input);
    Configurator_AddBindingByName(context->configurator, output, &s, node);
    DropTemp(kind, s);
}

// slot 31 AddOutputPredefined(node, mapping, registers, count, extra) on the uniforms
inline void* Predefined(void* self, MaterialContext* context, uint32_t mapping) {
    return Virtual<void*>(self, 31, context->uniforms, mapping, 1u, 1u, 0u);
}

// slot 32 AddOutputObject(node, mapping, object type, object index, modifier, count) on the uniforms
inline void* ObjectUniform(void* self, MaterialContext* context, uint32_t mapping, uint32_t objectType,
                           uint32_t index, const char* modifier) {
    return Virtual<void*>(self, 32, context->uniforms, mapping, objectType, index, modifier, 1u);
}

// MapIndexFromTextureEnum for one of the fixed maps (0..12), as the code inlines it: the maps the
// pixel program has from the parallax one (bit 7 of its first word) up to it, less one
inline int32_t MapIndex(uint32_t p0, uint32_t map) {
    int32_t index = -1;
    for (uint32_t i = 0; i <= map; i++) {
        index += int32_t((p0 >> (7 + i)) & 1);
    }
    return index;
}

// a copy of an NiFixedString handle (its count raised through the import, as inlined)
inline NiFixedString CopyHandle(const re::Ptr32<const char>* from) {
    NiFixedString s = *from;
    if (s != NullFixedString) {
        ni::sys::Increment(re::PtrCast<char*>(s) - 8);
    }
    return s;
}

} // namespace ni::shade
