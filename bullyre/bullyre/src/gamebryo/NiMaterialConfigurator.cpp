// NiMaterialConfigurator (vtable 0x962248, 0x34 bytes): the material nodes a program is built
// from and the bindings between their resources (NiMaterialResourceBinding, vtable 0x965414, 0x1C:
// the output resource +0x8, the input resource +0xC, the output's node +0x10, the input's node
// +0x14, the variable joining them +0x18), and Evaluate, which writes the program's HLSL: the
// nodes sorted so each comes after the nodes feeding it, the variables named, the uniforms listed,
// one function per code block (NiMaterialFragmentNode), the Input and Output structures, Main()
// calling the functions in order; then the program cache (slot 4) makes the program.
//
// Slots: 2 Evaluate(name, program type, uniforms), 3 Reset, 4 CanBindTypes(type, type), 5
// CanBind(binding). The program cache's slots used: 4 GenerateProgram, 5 the targets (a set of
// strings), 9 the language, 11 the semantic name for a resource semantic (an NiString), 12 its
// sort order.
//
// Its NiTObjectSet<NiPointer<...>> (base, allocated, count) Adds are inlined as the code has them
// (a temporary NiPointer around the slot's assignment); NiFixedStrings made from NiString text are
// the null handle for no text.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

GAME_VAR(long, ms_uiObjects, 0x020E2168);
// the class bits (each NiRTTI's word at +8) of the three node kinds
GAME_VAR(uint32_t, ConsumerNode_ClassBit, 0x020E4154);
GAME_VAR(uint32_t, ProducerNode_ClassBit, 0x020E4188);
GAME_VAR(uint32_t, FragmentNode_ClassBit, 0x020E41BC);

constexpr uintptr_t kNiRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t kConfiguratorVtable = 0x00962248;
constexpr uintptr_t kNodeArrayVtable = 0x00961DE8;   // NiTObjectArray<NiMaterialNodePtr>
constexpr uintptr_t kTArrayVtable = 0x009893C0;
constexpr uintptr_t kPointerListVtable = 0x00952E10;   // NiTPointerList's allocator

struct NiString {
    re::Ptr32<char> handle;
};

// NiTObjectSet<NiPointer<T>>
struct PtrSet {
    re::Ptr32<re::Ptr32<NiRefObject>> base;
    uint32_t allocated;
    uint32_t count;
};

struct NiMaterialConfigurator : NiRefObject {
    NiTArray<NiRefObject*> nodes;      // +0x8 (NiMaterialNodePtr)
    NiTArray<NiRefObject*> bindings;   // +0x18 (NiMaterialResourceBindingPtr)
    NiString description;              // +0x28
    re::Ptr32<char> platform;                    // +0x2C (NiFixedString: the cache's slot 10)
    re::Ptr32<NiRefObject> cache;                // +0x30 (NiGPUProgramCachePtr)
};
static_assert(offsetof(NiMaterialConfigurator, bindings) == 0x18);
static_assert(offsetof(NiMaterialConfigurator, description) == 0x28);
static_assert(sizeof(NiMaterialConfigurator) == 0x34);

GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(char*, __cdecl, FixedString_Create, 0x00754760, const char* text);
GAME_FN(uint8_t, __thiscall, FixedString_Equals, 0x00406480, const re::Ptr32<char>* self, const char* text);
GAME_FN(void, __thiscall, FixedString_AssignText, 0x00406730, re::Ptr32<char>* self, const char* text);
// NiString's
GAME_FN(NiString*, __thiscall, Str_Construct, 0x00774130, NiString* self, int length);
GAME_FN(NiString*, __thiscall, Str_ConstructSized, 0x007740D0, NiString* self, uint32_t length);
GAME_FN(NiString*, __thiscall, Str_ConstructFixed, 0x007741E0, NiString* self, const re::Ptr32<char>* text);
GAME_FN(NiString*, __thiscall, Str_ConstructCopy, 0x00774220, NiString* self, const NiString* other);
GAME_FN(void, __thiscall, Str_Destruct, 0x00774240, NiString* self);
GAME_FN(uint32_t, __thiscall, Str_Length, 0x00773D10, const NiString* self);
GAME_FN(uint8_t, __thiscall, Str_IsEmpty, 0x00773D20, const NiString* self);
GAME_FN(int, __thiscall, Str_Compare, 0x00773D60, const NiString* self, const char* text);
GAME_FN(const char*, __thiscall, Str_Chars, 0x00773DB0, const NiString* self);
GAME_FN(NiString*, __thiscall, Str_AssignString, 0x007745C0, NiString* self, const NiString* other);
GAME_FN(NiString*, __thiscall, Str_AssignText, 0x007745E0, NiString* self, const char* text);
GAME_FN(NiString*, __thiscall, Str_AppendText, 0x00774870, NiString* self, const char* text);
// (also given NiFixedStrings: the two share the handle's layout for it)
GAME_FN(NiString*, __thiscall, Str_Append, 0x00774890, NiString* self, const void* other);
GAME_FN(NiString*, __cdecl, Str_FromInt, 0x00774BF0, NiString* result, int value);
// the pointer arrays and sets
GAME_FN(re::Ptr32<NiRefObject>*, __cdecl, PointerArray_New, 0x00856120, uint32_t count);
GAME_FN(void, __cdecl, PointerArray_Delete, 0x00836F80, re::Ptr32<NiRefObject>* base);
GAME_FN(void, __thiscall, PointerArray_SetSize, 0x00856170, void* array, uint32_t size);
GAME_FN(void, __thiscall, PointerArray_SetAt, 0x0040E800, void* array, uint32_t index, const re::Ptr32<NiRefObject>* value);
GAME_FN(void, __thiscall, PointerArray_RemoveAll, 0x0040E7A0, void* array);
GAME_FN(void, __thiscall, Set_RemoveAll, 0x00836940, PtrSet* set);
GAME_FN(void, __thiscall, Set_Add, 0x00787BE0, PtrSet* set, const re::Ptr32<NiRefObject>* object);
GAME_FN(void, __thiscall, Set_RemoveAt, 0x008CA990, PtrSet* set, uint32_t index);
// the resources', nodes' and bindings' getters (shared by offset)
GAME_FN(re::Ptr32<char>*, __thiscall, Get08, 0x0077C280, void* self);          // a resource's type
GAME_FN(re::Ptr32<char>*, __thiscall, Get0C, 0x008BAC30, void* self);          // its semantic, a node's name
GAME_FN(re::Ptr32<char>*, __thiscall, Get10, 0x007E2BC0, void* self);          // its label
GAME_FN(re::Ptr32<char>*, __thiscall, Get14, 0x007E2BD0, void* self);          // its variable; a code block's text
GAME_FN(re::Ptr32<char>*, __thiscall, Get18, 0x007F7EE0, void* self);          // its default value; a binding's variable
GAME_FN(uint32_t, __thiscall, GetCount, 0x0077E8E0, void* self);     // a resource's count
GAME_FN(void*, __thiscall, GetWord08, 0x00856850, void* self);       // a binding's output
GAME_FN(void*, __thiscall, GetWord0C, 0x00853140, void* self);       // a binding's input
GAME_FN(void*, __thiscall, GetWord10, 0x0077E8D0, void* self);       // a binding's output node
GAME_FN(re::Ptr32<char>*, __thiscall, GetResourceType, 0x00655B80, void* self);   // a producer node's kind (+0x30)
GAME_FN(void, __thiscall, Set18, 0x007F80C0, void* self, const re::Ptr32<char>* value); // a binding's variable
GAME_FN(NiString*, __thiscall, Node_GetDescription, 0x007EA8D0, void* self);
GAME_FN(uint32_t, __thiscall, Node_GetInputResourceCount, 0x007E2E50, void* self);
GAME_FN(void*, __thiscall, Node_GetInputResource, 0x007E2E60, void* self, uint32_t index);
GAME_FN(uint32_t, __thiscall, Node_GetOutputResourceCount, 0x007E2EC0, void* self);
GAME_FN(void*, __thiscall, Node_GetOutputResource, 0x007E2ED0, void* self, uint32_t index);
GAME_FN(void*, __thiscall, Node_GetInputResourceByVariableName, 0x007E2E70, void* self, const void* name);
GAME_FN(void*, __thiscall, Node_GetOutputResourceByVariableName, 0x007E2EE0, void* self, const void* name);
GAME_FN(void*, __thiscall, FragmentNode_GetCodeBlock, 0x007EA900, void* self, const re::Ptr32<char>* language,
        const re::Ptr32<char>* platform, const void* targets);
GAME_FN(NiRefObject*, __thiscall, Binding_Construct, 0x007F7EF0, void* self, void* output, void* outputNode,
        void* input, void* inputNode);
// (this file's, called where modes apply)
GAME_FN(void*, __thiscall, Cfg_GetNodeContainingResource, 0x007E8170, NiMaterialConfigurator* self, void* resource);
GAME_FN(void*, __thiscall, Cfg_GetInputBinding, 0x007E8220, NiMaterialConfigurator* self, void* resource,
        uint32_t index);
GAME_FN(void*, __thiscall, Cfg_GetOutputBinding, 0x007E8270, NiMaterialConfigurator* self, void* resource,
        uint32_t index);
GAME_FN(void, __thiscall, Cfg_SetVariable, 0x007E82C0, NiMaterialConfigurator* self, void* resource,
        const re::Ptr32<char>* name, int force);
GAME_FN(NiString*, __thiscall, Cfg_GetVariableName, 0x007E8340, NiMaterialConfigurator* self, NiString* result,
        void* resource, void* node, int isInput, uint32_t unused);
GAME_FN(NiString*, __thiscall, Cfg_GenerateOutputAssignments, 0x007E8470, NiMaterialConfigurator* self,
        NiString* result, void* node);
GAME_FN(void, __thiscall, Set_Realloc, 0x007E8680, PtrSet* set, uint32_t allocated);
GAME_FN(NiString*, __thiscall, Cfg_GenerateResourceDeclaration, 0x007E8730, NiMaterialConfigurator* self,
        NiString* result, void* resource, int withSemantic);
GAME_FN(NiString*, __thiscall, Cfg_GenerateFunction, 0x007E8910, NiMaterialConfigurator* self, NiString* result,
        void* node, const void* targets);
GAME_FN(NiString*, __thiscall, Cfg_GenerateStructMembers, 0x007E8B40, NiMaterialConfigurator* self,
        NiString* result, PtrSet* resources);
GAME_FN(NiString*, __thiscall, Cfg_GenerateFunctionCall, 0x007E8DE0, NiMaterialConfigurator* self,
        NiString* result, void* node, uint32_t call);
GAME_FN(void, __thiscall, Cfg_Destruct, 0x007E93E0, NiMaterialConfigurator* self);
GAME_FN(uint8_t, __thiscall, Cfg_AddBinding, 0x007E94A0, NiMaterialConfigurator* self, NiRefObject* binding);

// ---- reference counting (the imports, as the code calls them)

inline void Hold(NiRefObject* object) {
    if (object != nullptr) {
        Increment(&object->refCount);
    }
}

inline void LetGo(NiRefObject* object) {
    if (object != nullptr && Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

// an object's NiRTTI has the class with that bit
inline bool IsKindOf(void* object, uint32_t bit) {
    const uint8_t* rtti = Virtual<const uint8_t*>(object, 2);
    return (At<uint32_t>(rtti, 0xC + int(bit >> 5) * 4) & (1u << (bit & 31))) != 0;
}

// NiTObjectSet<NiPointer<T>>::Add inlined
void SetAdd(PtrSet* set, NiRefObject* object) {
    NiRefObject* held = object;
    Hold(held);
    if (set->count == set->allocated) {
        Set_Realloc(set, set->allocated != 0 ? set->allocated * 2 : 1);
    }
    re::Ptr32<NiRefObject>* slot = &set->base[set->count];
    NiRefObject* old = *slot;
    set->count++;
    if (old != held) {
        LetGo(old);
        *slot = held;
        Hold(held);
    }
    LetGo(held);
}

// NiTObjectArray<NiPointer<T>>::Add inlined
void ArrayAdd(NiTArray<NiRefObject*>& array, NiRefObject* object) {
    re::Ptr32<NiRefObject> held = object;
    Hold(held);
    uint32_t index = array.size;
    if (index >= array.maxSize) {
        PointerArray_SetSize(&array, array.growBy + index);
    }
    PointerArray_SetAt(&array, index, &held);
    LetGo(held);
}

// a fixed string made from text (the null handle for none)
inline char* MakeFixed(const char* text) {
    return text != nullptr ? FixedString_Create(text) : static_cast<char*>(kNullFixedString);
}

// a fixed string's characters are there and not empty (the length 4 bytes before them)
inline bool HasText(const char* s) {
    return s != kNullFixedString && *reinterpret_cast<const uint16_t*>(s - 4) != 0;
}

// the nth binding whose output (input) is that resource, searched as the code inlines it
void* NthBinding(NiMaterialConfigurator* self, void* resource, uint32_t n, bool input) {
    uint32_t found = 0;
    for (uint32_t i = 0; i < self->bindings.size; i++) {
        void* binding = self->bindings.base[i];
        void* r = input ? GetWord0C(binding) : GetWord08(binding);
        if (r == resource) {
            if (found == n) {
                return binding;
            }
            found++;
        }
    }
    return nullptr;
}

// every output binding of a resource given that variable name (the first found by a call to
// GetOutputBinding or inline)
void NameOutputBindings(NiMaterialConfigurator* self, void* resource, const re::Ptr32<char>* name, bool firstByCall) {
    void* binding = firstByCall ? Cfg_GetOutputBinding(self, resource, 0) : NthBinding(self, resource, 0, false);
    uint32_t n = 0;
    while (binding != nullptr) {
        Set18(binding, name);
        n++;
        binding = NthBinding(self, resource, n, false);
    }
}

// ---- the small ones

// vf5 CanBind(binding): both resources there and their types bindable (slot 4)
uint8_t __fastcall CfgCanBind(NiMaterialConfigurator* self, re::Edx, void* binding) {
    if (binding == nullptr) {
        return 0;
    }
    void* output = GetWord08(binding);
    void* input = GetWord0C(binding);
    if (input == nullptr || output == nullptr) {
        return 0;
    }
    re::Ptr32<char>* outputType = Get08(output);
    re::Ptr32<char>* inputType = Get08(input);
    return Virtual<uint8_t>(self, 4, outputType, inputType);
}

// vf4 CanBindTypes(output type, input type): the same type, or a float vector into a narrower one
uint8_t __fastcall CfgCanBindTypes(NiMaterialConfigurator*, re::Edx, const re::Ptr32<char>* output, const re::Ptr32<char>* input) {
    if (*output == *input) {
        return 1;
    }
    if (FixedString_Equals(output, "float4") != 0) {
        if (FixedString_Equals(input, "float3") != 0) {
            return 1;
        }
    } else if (FixedString_Equals(output, "float3") == 0) {
        if (FixedString_Equals(output, "float2") == 0) {
            return 0;
        }
        return FixedString_Equals(input, "float") != 0 ? 1 : 0;
    }
    if (FixedString_Equals(input, "float2") != 0) {
        return 1;
    }
    return FixedString_Equals(input, "float") != 0 ? 1 : 0;
}

// 0x7E80D0 SetDescription(text)
NiString* __fastcall CfgSetDescription(NiMaterialConfigurator* self, re::Edx, const char* text) {
    return Str_AssignText(&self->description, text);
}

// 0x7E8170 GetNodeContainingResource(resource) (the node's slot 3)
void* __fastcall CfgGetNodeContainingResource(NiMaterialConfigurator* self, re::Edx, void* resource) {
    for (uint32_t i = 0; i < self->nodes.size; i++) {
        NiRefObject* node = self->nodes.base[i];
        if (node != nullptr && Virtual<uint8_t>(node, 3, resource) != 0) {
            return node;
        }
    }
    return nullptr;
}

// 0x7E81C0 GetNode(name, n): the nth node of that name
void* __fastcall CfgGetNode(NiMaterialConfigurator* self, re::Edx, const re::Ptr32<char>* name, uint32_t n) {
    uint32_t found = 0;
    for (uint32_t i = 0; i < self->nodes.size; i++) {
        NiRefObject* node = self->nodes.base[i];
        if (node != nullptr && *Get0C(node) == *name) {
            if (found == n) {
                return node;
            }
            found++;
        }
    }
    return nullptr;
}

// 0x7E8220 GetInputBinding(resource, n) / 0x7E8270 GetOutputBinding(resource, n): the nth binding
// into (out of) the resource
void* __fastcall CfgGetInputBinding(NiMaterialConfigurator* self, re::Edx, void* resource, uint32_t n) {
    return NthBinding(self, resource, n, true);
}
void* __fastcall CfgGetOutputBinding(NiMaterialConfigurator* self, re::Edx, void* resource, uint32_t n) {
    return NthBinding(self, resource, n, false);
}

// 0x7E82C0 SetVariable(resource, name, force): the resource's output bindings named (those that
// have a name only when forced)
void __fastcall CfgSetVariable(NiMaterialConfigurator* self, re::Edx, void* resource, const re::Ptr32<char>* name, int force) {
    uint32_t n = 0;
    void* binding = Cfg_GetOutputBinding(self, resource, 0);
    while (binding != nullptr) {
        if (re::Flag(force)) {
            Set18(binding, name);
        } else {
            char* current = *Get18(binding);
            if (current == kNullFixedString || *reinterpret_cast<const uint16_t*>(current - 4) < 1) {
                Set18(binding, name);
            }
        }
        n++;
        binding = Cfg_GetOutputBinding(self, resource, n);
    }
}

// 0x7E8340 GetVariableName(result, resource, node, is input, unused): the first named binding of
// the resource; for an input whose first binding has no name, the output it comes from: its
// constant ("type" + default value) or, followed back, its own name. Empty for none
NiString* __fastcall CfgGetVariableName(NiMaterialConfigurator* self, re::Edx, NiString* result, void* resource, void*,
                                        int isInput, uint32_t unused) {
    uint32_t n = 0;
    void* binding = re::Flag(isInput) ? Cfg_GetInputBinding(self, resource, 0) : Cfg_GetOutputBinding(self, resource, 0);
    while (binding != nullptr) {
        const re::Ptr32<char>* name = Get18(binding);
        if (*name != kNullFixedString && *reinterpret_cast<const uint16_t*>(*name - 4) != 0) {
            Str_ConstructFixed(result, name);
            return result;
        }
        n++;
        if (re::Flag(isInput)) {
            void* output = GetWord08(binding);
            char* value = *Get18(output);
            if (value != kNullFixedString && *reinterpret_cast<const uint16_t*>(value - 4) != 0) {
                NiString text;
                Str_ConstructFixed(&text, Get08(output));
                Str_Append(&text, Get18(output));
                Str_ConstructCopy(result, &text);
                Str_Destruct(&text);
                return result;
            }
            return Cfg_GetVariableName(self, result, output, GetWord10(binding), 0, unused);
        }
        binding = Cfg_GetOutputBinding(self, resource, n);
    }
    Str_Construct(result, 0);
    return result;
}

// vf3 Reset: the nodes and bindings let go
void __fastcall CfgReset(NiMaterialConfigurator* self, re::Edx) {
    PointerArray_RemoveAll(&self->nodes);
    PointerArray_RemoveAll(&self->bindings);
}

// 0x7E8470 GenerateOutputAssignments(result, consumer node): "    Out.<input> = <source>;" for each
// input bound to a producer node's output: a static's constant (type + default value), a
// uniform's name, else the Input structure's member ("In.<name>")
NiString* __fastcall CfgGenerateOutputAssignments(NiMaterialConfigurator* self, re::Edx, NiString* result, void* node) {
    Str_Construct(result, 0x400);
    for (uint32_t i = 0; i < Node_GetInputResourceCount(node); i++) {
        void* input = Node_GetInputResource(node, i);
        uint32_t n = 0;
        void* binding = Cfg_GetInputBinding(self, input, 0);
        while (binding != nullptr) {
            NiRefObject* producer = static_cast<NiRefObject*>(GetWord10(binding));
            if (producer != nullptr && IsKindOf(producer, ProducerNode_ClassBit)) {
                Str_AppendText(result, "    Out.");
                Str_Append(result, Get14(input));
                Str_AppendText(result, " = ");
                void* output = GetWord08(binding);
                const char* kind = *GetResourceType(producer);
                if (kind == reinterpret_cast<const char*>(0x00955ADC) ||
                    (kind != nullptr && crt::Strcmp(kind, "Static") == 0)) {
                    NiString type;
                    Str_ConstructFixed(&type, Get08(output));
                    Str_AppendText(result, Str_Chars(&type));
                    Str_Destruct(&type);
                    Str_AppendText(result, *Get18(output));
                } else {
                    kind = *GetResourceType(producer);
                    if (!(kind == reinterpret_cast<const char*>(0x00955AF0) ||
                          (kind != nullptr && crt::Strcmp(kind, "Uniform") == 0))) {
                        Str_AppendText(result, "In.");
                    }
                    Str_Append(result, Get14(output));
                }
                Str_AppendText(result, ";\n");
            }
            n++;
            binding = NthBinding(self, input, n, true);
        }
    }
    return result;
}

// 0x7E8680 NiTObjectSet<NiPointer<T>>::Realloc(allocated): the pointers moved to a new array (the
// count not clamped to the new size)
void __fastcall SetRealloc(PtrSet* self, re::Edx, uint32_t allocated) {
    if (allocated == self->allocated) {
        return;
    }
    re::Ptr32<NiRefObject>* base = nullptr;
    if (allocated != 0) {
        base = PointerArray_New(allocated);
        for (uint32_t i = 0; i < self->count; i++) {
            NiRefObject* from = self->base[i];
            if (base[i] != from) {
                LetGo(base[i]);
                base[i] = from;
                Hold(from);
            }
        }
    }
    PointerArray_Delete(self->base);
    self->base = base;
    self->allocated = allocated;
}

// 0x7E8730 GenerateResourceDeclaration(result, resource, with semantic): "type name[count] :
// SEMANTIC_label"
NiString* __fastcall CfgGenerateResourceDeclaration(NiMaterialConfigurator*, re::Edx, NiString* result, void* resource,
                                                    int withSemantic) {
    Str_Construct(result, 0x400);
    NiString type;
    Str_ConstructFixed(&type, Get08(resource));
    Str_AppendText(result, Str_Chars(&type));
    Str_Destruct(&type);
    Str_AppendText(result, " ");
    Str_Append(result, Get14(resource));
    if (GetCount(resource) != 1) {
        char count[0x20];
        NiSprintf(count, 0x20, "%d", GetCount(resource));
        Str_AppendText(result, "[");
        Str_AppendText(result, count);
        Str_AppendText(result, "]");
    }
    if (!re::Flag(withSemantic)) {
        return result;
    }
    re::Ptr32<char>* semanticHandle = Get0C(resource);
    re::Ptr32<char> semantic = FixedStringAddRef(*semanticHandle);
    semantic = *semanticHandle;
    if (semantic == kNullFixedString) {
        return result;
    }
    if (*reinterpret_cast<const uint16_t*>(semantic - 4) != 0) {
        Str_AppendText(result, " : ");
        Str_Append(result, &semantic);
        re::Ptr32<char>* labelHandle = Get10(resource);
        re::Ptr32<char> label = FixedStringAddRef(*labelHandle);
        label = *labelHandle;
        if (label != kNullFixedString) {
            if (*reinterpret_cast<const uint16_t*>(label - 4) != 0) {
                Str_AppendText(result, "_");
                Str_Append(result, &label);
            }
            FixedStringRelease(&label);
        }
    }
    FixedStringRelease(&semantic);
    return result;
}

// 0x7E8910 GenerateFunction(result, fragment node, targets): its description as a comment, "void
// Name(inputs, out outputs)" and the body of its code block for the cache's language, this
// platform and the targets
NiString* __fastcall CfgGenerateFunction(NiMaterialConfigurator* self, re::Edx, NiString* result, void* node,
                                         const void* targets) {
    Str_Construct(result, 0x400);
    if (Str_Chars(Node_GetDescription(node)) != nullptr) {
        Str_AppendText(result, "/*\n");
        Str_AppendText(result, Str_Chars(Node_GetDescription(node)));
        Str_AppendText(result, "\n*/\n");
    }
    Str_AppendText(result, "\nvoid ");
    Str_Append(result, Get0C(node));
    Str_AppendText(result, "(");
    uint32_t written = 0;
    for (uint32_t i = 0; i < Node_GetInputResourceCount(node); i++) {
        void* resource = Node_GetInputResource(node, i);
        if (written > 0) {
            Str_AppendText(result, ",\n    ");
        }
        NiString declaration;
        Str_AppendText(result, Str_Chars(Cfg_GenerateResourceDeclaration(self, &declaration, resource, 0)));
        Str_Destruct(&declaration);
        written++;
    }
    for (uint32_t i = 0; i < Node_GetOutputResourceCount(node); i++) {
        void* resource = Node_GetOutputResource(node, i);
        if (written > 0) {
            Str_AppendText(result, ",\n    ");
        }
        Str_AppendText(result, "out ");
        NiString declaration;
        Str_AppendText(result, Str_Chars(Cfg_GenerateResourceDeclaration(self, &declaration, resource, 0)));
        Str_Destruct(&declaration);
        written++;
    }
    Str_AppendText(result, ")\n");
    Str_AppendText(result, "{\n");
    re::Ptr32<char> language = MakeFixed(Virtual<const char*>(self->cache, 9));
    void* block = FragmentNode_GetCodeBlock(node, &language, &self->platform, targets);
    FixedStringRelease(&language);
    Str_AppendText(result, Str_Chars(reinterpret_cast<NiString*>(Get14(block))));
    Str_AppendText(result, "\n}\n");
    Str_AppendText(result, "//---------------------------------------------------------------------------\n");
    return result;
}

// 0x7E8B40 GenerateStructMembers(result, resources): "    type name : SEMANTICn;" for each, in the
// order of the cache's slot 12 for their semantics (a list kept with the last node of each order:
// a resource goes after the last of its order or the nearest lower one), the index counting up
// while the semantic name repeats
NiString* __fastcall CfgGenerateStructMembers(NiMaterialConfigurator* self, re::Edx, NiString* result,
                                              PtrSet* resources) {
    NiTPointerList<void*> list;
    list.vtable = reinterpret_cast<void*>(kPointerListVtable);
    list.head = nullptr;
    list.tail = nullptr;
    list.count = 0;
    re::Ptr32<NiTListNode<void*>> last[17];
    crt::Memset(last, 0, sizeof(last));
    for (uint32_t i = 0; i < resources->count; i++) {
        void* cache = self->cache;
        void* resource = resources->base[i];
        int order = Virtual<int>(cache, 12, Get0C(resource));
        re::Ptr32<NiTListNode<void*>>* bucket = &last[order];
        NiTListNode<void*>* after = *bucket;
        for (int lower = order - 1; after == nullptr && lower >= 0; lower--) {
            after = last[lower];
        }
        auto* node = Virtual<NiTListNode<void*>*>(&list, 1);
        node->element = resource;
        node->prev = after;
        if (after != nullptr) {
            node->next = after->next;
            if (after->next != nullptr) {
                after->next->prev = node;
            } else {
                list.tail = node;
            }
            after->next = node;
        } else {
            node->next = list.head;
            if (list.head != nullptr) {
                list.head->prev = node;
                list.head = node;
            } else {
                list.tail = node;
                list.head = node;
            }
        }
        list.count++;
        *bucket = node;
    }
    Str_Construct(result, 0x400);
    NiString previous;
    Str_Construct(&previous, 0);
    uint32_t index = 0;
    for (NiTListNode<void*>* node = list.head; node != nullptr;) {
        void* resource = node->element;
        void* cache = self->cache;
        node = node->next;
        NiString semantic;
        Virtual<void>(cache, 11, &semantic, Get0C(resource));
        if (Str_Compare(&semantic, Str_Chars(&previous)) == 0) {
            index++;
        } else {
            Str_AssignString(&previous, &semantic);
            index = 0;
        }
        Str_AppendText(result, "    ");
        NiString type;
        Str_ConstructFixed(&type, Get08(resource));
        Str_AppendText(result, Str_Chars(&type));
        Str_Destruct(&type);
        Str_AppendText(result, " ");
        Str_Append(result, Get14(resource));
        Str_AppendText(result, " : ");
        Str_Append(result, &previous);
        NiString number;
        Str_AppendText(result, Str_Chars(Str_FromInt(&number, int(index))));
        Str_Destruct(&number);
        Str_AppendText(result, ";\n");
        Str_Destruct(&semantic);
    }
    Str_Destruct(&previous);
    // the list's destructor: its nodes freed (slot 2)
    list.vtable = reinterpret_cast<void*>(kPointerListVtable);
    for (NiTListNode<void*>* node = list.head; node != nullptr;) {
        NiTListNode<void*>* next = node->next;
        Virtual<void>(&list, 2, node);
        node = next;
    }
    return result;
}

// a value for the call line, wrapped at 80 columns
void AddArgument(NiString* call, uint32_t& column, NiString* value) {
    if (Str_Length(value) + column > 0x4F) {
        Str_AppendText(call, "\n        ");
        column = Str_Length(value) + 8;
    } else {
        column += Str_Length(value);
    }
    Str_Append(call, value);
}

// 0x7E8DE0 GenerateFunctionCall(result, fragment node, call): "Name(arguments);" after the
// declarations of the outputs no binding names (named "<variable>_CallOut<call>" and the name given
// to their bindings); the inputs' arguments their variables (or a constant)
NiString* __fastcall CfgGenerateFunctionCall(NiMaterialConfigurator* self, re::Edx, NiString* result, void* node,
                                             uint32_t call) {
    Str_Construct(result, 0x400);
    NiString line;
    Str_Construct(&line, 0x400);
    NiString declarations;
    Str_Construct(&declarations, 0x400);
    uint32_t written = 0;
    Str_AppendText(&line, "    ");
    Str_Append(&line, Get0C(node));
    Str_AppendText(&line, "(");
    uint32_t column = Str_Length(&line);
    for (uint32_t i = 0; i < Node_GetInputResourceCount(node); i++) {
        void* input = Node_GetInputResource(node, i);
        if (written != 0) {
            Str_AppendText(&line, ", ");
            column += 2;
        }
        NiString value;
        Cfg_GetVariableName(self, &value, input, node, 1, 1);
        if (Str_IsEmpty(&value) != 0 && HasText(*Get18(input))) {
            NiString type;
            Str_ConstructFixed(&type, Get08(input));
            Str_AssignText(&value, Str_Chars(&type));
            Str_Destruct(&type);
            Str_AppendText(&value, *Get18(input));
        }
        AddArgument(&line, column, &value);
        written++;
        Str_Destruct(&value);
    }
    for (uint32_t j = 0; j < Node_GetOutputResourceCount(node); j++) {
        void* output = Node_GetOutputResource(node, j);
        if (written != 0) {
            Str_AppendText(&line, ", ");
            column += 2;
        }
        NiString value;
        uint32_t n = 0;
        void* binding = Cfg_GetOutputBinding(self, output, 0);
        bool named = false;
        while (binding != nullptr) {
            const re::Ptr32<char>* name = Get18(binding);
            if (*name != kNullFixedString && *reinterpret_cast<const uint16_t*>(*name - 4) != 0) {
                Str_ConstructFixed(&value, name);
                named = true;
                break;
            }
            n++;
            binding = NthBinding(self, output, n, false);
        }
        if (!named) {
            Str_Construct(&value, 0);
        }
        if (Str_IsEmpty(&value) != 0) {
            char text[0x100];
            NiSprintf(text, 0x100, "%s_CallOut%d", *Get14(output), call);
            Str_AssignText(&value, text);
            Str_AppendText(&declarations, "    ");
            NiString type;
            Str_ConstructFixed(&type, Get08(output));
            Str_AppendText(&declarations, Str_Chars(&type));
            Str_Destruct(&type);
            Str_AppendText(&declarations, " ");
            Str_Append(&declarations, &value);
            Str_AppendText(&declarations, ";\n");
            re::Ptr32<char> name = MakeFixed(Str_Chars(&value));
            NameOutputBindings(self, output, &name, true);
            FixedStringRelease(&name);
        }
        AddArgument(&line, column, &value);
        written++;
        Str_Destruct(&value);
    }
    Str_AppendText(&line, ");");
    char comment[0x30];
    NiSprintf(comment, 0x30, "\t// Function call #%d\n", call);
    Str_AppendText(result, comment);
    Str_Append(result, &declarations);
    Str_Append(result, &line);
    Str_AppendText(result, "\n");
    Str_Destruct(&declarations);
    Str_Destruct(&line);
    return result;
}

// 0x7E92F0 NiMaterialConfigurator(program cache): room for 10 nodes and bindings; the platform
// the cache's (slot 10; a null cache isn't checked)
NiMaterialConfigurator* __fastcall CfgConstruct(NiMaterialConfigurator* self, re::Edx, NiRefObject* cache) {
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    self->refCount = 0;
    Increment(&ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(kConfiguratorVtable);
    self->nodes.vtable = reinterpret_cast<void*>(kTArrayVtable);
    self->nodes.maxSize = 10;
    self->nodes.growBy = 10;
    self->nodes.size = 0;
    self->nodes.esize = 0;
    self->nodes.base = PointerArray_New(10);
    self->nodes.vtable = reinterpret_cast<void*>(kNodeArrayVtable);
    self->bindings.vtable = reinterpret_cast<void*>(kTArrayVtable);
    self->bindings.maxSize = 10;
    self->bindings.growBy = 10;
    self->bindings.size = 0;
    self->bindings.esize = 0;
    self->bindings.base = PointerArray_New(10);
    self->bindings.vtable = reinterpret_cast<void*>(kTArrayVtable);
    Str_ConstructSized(&self->description, 0x100);
    self->platform = kNullFixedString;
    self->cache = nullptr;
    if (cache != nullptr) {
        self->cache = cache;
        Increment(&cache->refCount);
    }
    FixedString_AssignText(&self->platform, Virtual<const char*>(cache, 10));
    return self;
}

// 0x7E93E0 ~NiMaterialConfigurator
void __fastcall CfgDestruct(NiMaterialConfigurator* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kConfiguratorVtable);
    LetGo(self->cache);
    FixedStringRelease(&self->platform);
    Str_Destruct(&self->description);
    self->bindings.vtable = reinterpret_cast<void*>(kTArrayVtable);
    PointerArray_Delete(self->bindings.base);
    self->nodes.vtable = reinterpret_cast<void*>(kTArrayVtable);
    PointerArray_Delete(self->nodes.base);
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    Decrement(&ms_uiObjects);
}

// vf0 the deleting destructor
NiMaterialConfigurator* __fastcall CfgDeletingDestructor(NiMaterialConfigurator* self, re::Edx, uint32_t flags) {
    Cfg_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x34);
    }
    return self;
}

// 0x7E94A0 AddBinding(binding): kept when bindable (slot 5)
uint8_t __fastcall CfgAddBinding(NiMaterialConfigurator* self, re::Edx, NiRefObject* binding) {
    uint8_t ok = Virtual<uint8_t>(self, 5, binding);
    if (ok == 0) {
        return ok;
    }
    ArrayAdd(self->bindings, binding);
    return 1;
}

// a new binding (AddBinding given none when the allocation fails)
uint8_t NewBinding(NiMaterialConfigurator* self, void* output, void* outputNode, void* input, void* inputNode) {
    void* memory = NiNew(0x1C);
    if (memory == nullptr) {
        return Cfg_AddBinding(self, nullptr);
    }
    return Cfg_AddBinding(self, Binding_Construct(memory, output, outputNode, input, inputNode));
}

// 0x7E9520 AddBinding(output, input): their nodes looked up
uint8_t __fastcall CfgAddBindingResources(NiMaterialConfigurator* self, re::Edx, void* output, void* input) {
    if (input == nullptr || output == nullptr) {
        return 0;
    }
    void* outputNode = Cfg_GetNodeContainingResource(self, output);
    void* inputNode = Cfg_GetNodeContainingResource(self, input);
    if (inputNode == nullptr || outputNode == nullptr) {
        return 0;
    }
    return NewBinding(self, output, outputNode, input, inputNode);
}

// 0x7E95B0 AddBinding(output name, output node, input name, input node)
uint8_t __fastcall CfgAddBindingNames(NiMaterialConfigurator* self, re::Edx, const void* outputName, void* outputNode,
                                      const void* inputName, void* inputNode) {
    void* output = Node_GetOutputResourceByVariableName(outputNode, outputName);
    void* input = Node_GetInputResourceByVariableName(inputNode, inputName);
    if (input == nullptr || output == nullptr) {
        return 0;
    }
    return NewBinding(self, output, outputNode, input, inputNode);
}

// 0x7E9630 AddBinding(output, input name, input node)
uint8_t __fastcall CfgAddBindingByName(NiMaterialConfigurator* self, re::Edx, void* output, const void* inputName,
                                       void* inputNode) {
    void* input = Node_GetInputResourceByVariableName(inputNode, inputName);
    if (input == nullptr || output == nullptr) {
        return 0;
    }
    void* outputNode = Cfg_GetNodeContainingResource(self, output);
    if (inputNode == nullptr || outputNode == nullptr) {
        return 0;
    }
    return NewBinding(self, output, outputNode, input, inputNode);
}

// 0x7E96C0 AddBinding(output name, output node, input) (the input's node not checked)
uint8_t __fastcall CfgAddBindingFromOutput(NiMaterialConfigurator* self, re::Edx, const void* outputName,
                                           void* outputNode, void* input) {
    void* output = Node_GetOutputResourceByVariableName(outputNode, outputName);
    if (input == nullptr || output == nullptr) {
        return 0;
    }
    void* inputNode = Cfg_GetNodeContainingResource(self, input);
    return NewBinding(self, output, outputNode, input, inputNode);
}

// ---- Evaluate

const char* const kConstantsBanner =
    "//---------------------------------------------------------------------------\n"
    "// Constant variables:\n"
    "//---------------------------------------------------------------------------\n\n";
const char* const kFunctionsBanner =
    "//---------------------------------------------------------------------------\n"
    "// Functions:\n"
    "//---------------------------------------------------------------------------\n\n";
const char* const kInputBanner =
    "//---------------------------------------------------------------------------\n"
    "// Input:\n"
    "//---------------------------------------------------------------------------\n\n";
const char* const kOutputBanner =
    "//---------------------------------------------------------------------------\n"
    "// Output:\n"
    "//---------------------------------------------------------------------------\n\n";
const char* const kMainBanner =
    "//---------------------------------------------------------------------------\n"
    "// Main():\n"
    "//---------------------------------------------------------------------------\n\n";

// a set's pointers let go and its array freed
void FreeSet(PtrSet* set) {
    Set_RemoveAll(set);
    PointerArray_Delete(set->base);
}

// the nodes sorted so that each comes after the nodes its inputs are bound from (a node whose
// input's first binding comes from a node not placed yet waits for the next pass; RemoveAt moves
// the rest down and the index still moves on, as in the original; a cycle never ends)
void SortNodes(NiMaterialConfigurator* self, PtrSet* pending, PtrSet* sorted) {
    for (uint32_t i = 0; i < self->nodes.size; i++) {
        NiRefObject* node = self->nodes.base[i];
        re::Ptr32<NiRefObject> held = node;
        Hold(node);
        Set_Add(pending, &held);
        LetGo(node);
    }
    while (pending->count != 0) {
        for (uint32_t j = 0; j < pending->count; j++) {
            NiRefObject* node = pending->base[j];
            bool waiting = false;
            for (uint32_t k = 0; k < Node_GetInputResourceCount(node); k++) {
                void* input = Node_GetInputResource(node, k);
                void* binding = NthBinding(self, input, 0, true);
                if (binding == nullptr) {
                    continue;
                }
                NiRefObject* source = static_cast<NiRefObject*>(GetWord10(binding));
                Hold(source);
                bool placed = false;
                for (uint32_t s = 0; s < sorted->count; s++) {
                    if (sorted->base[s] == source) {
                        placed = true;
                        break;
                    }
                }
                LetGo(source);
                if (!placed) {
                    waiting = true;
                    break;
                }
            }
            if (waiting) {
                continue;
            }
            SetAdd(sorted, node);
            Set_RemoveAt(pending, j);
        }
    }
}

// vf2 Evaluate(name, program type, uniforms): the program's HLSL made and handed to the program
// cache (slot 4); none without bindings. The uniforms are emptied, then filled
NiRefObject* __fastcall CfgEvaluate(NiMaterialConfigurator* self, re::Edx, const char* name, uint32_t,
                                    PtrSet* uniforms) {
    if (self->bindings.size == 0) {
        return nullptr;
    }
    NiString input, output, functions, header, main;
    Str_Construct(&input, 0x400);
    Str_Construct(&output, 0x400);
    Str_Construct(&functions, 0x400);
    Str_Construct(&header, 0x400);
    Str_Construct(&main, 0x400);
    Str_AppendText(&header, "/*\nShader description:\n");
    Str_Append(&header, &self->description);
    Str_AppendText(&header, "\n*/\n\n");
    Str_AppendText(&header, kConstantsBanner);
    Str_AppendText(&functions, kFunctionsBanner);
    Str_AppendText(&input, kInputBanner);
    Str_AppendText(&input, "struct Input\n{\n");
    Str_AppendText(&output, kOutputBanner);
    Str_AppendText(&output, "struct Output\n{\n");
    Str_AppendText(&main, kMainBanner);
    Str_AppendText(&main, "Output Main(Input In)\n{\n    Output Out;\n");
    PtrSet outputs = {};    // the consumer nodes' inputs
    PtrSet inputs = {};     // the producer nodes' outputs (not uniform, not static)
    PtrSet unused = {};
    Set_RemoveAll(uniforms);
    PtrSet sorted = {};
    PtrSet pending = {};
    PtrSet blocks = {};     // the code blocks written already
    if (self->nodes.size != 0) {
        SortNodes(self, &pending, &sorted);
    }
    for (uint32_t i = 0; i < sorted.count; i++) {
        NiRefObject* node = sorted.base[i];
        if (node == nullptr) {
            continue;
        }
        if (IsKindOf(node, ConsumerNode_ClassBit)) {
            // its inputs are the Output structure's members ("Out.<variable>")
            for (uint32_t k = 0; k < Node_GetInputResourceCount(node); k++) {
                NiString member;
                Str_ConstructSized(&member, 0x100);
                NiRefObject* resource = static_cast<NiRefObject*>(Node_GetInputResource(node, k));
                SetAdd(&outputs, resource);
                Str_AppendText(&member, "Out.");
                Str_Append(&member, Get14(resource));
                re::Ptr32<char> memberName = MakeFixed(Str_Chars(&member));
                uint32_t n = 0;
                void* binding = NthBinding(self, resource, 0, true);
                while (binding != nullptr) {
                    Set18(binding, &memberName);
                    n++;
                    binding = NthBinding(self, resource, n, true);
                }
                FixedStringRelease(&memberName);
                re::Ptr32<char> variable = MakeFixed(Str_Chars(&member));
                Cfg_SetVariable(self, resource, &variable, 0);
                FixedStringRelease(&variable);
                Str_Destruct(&member);
            }
        }
        if (IsKindOf(node, ProducerNode_ClassBit)) {
            // its outputs: uniforms (by name), statics (constants) or the Input structure's members
            for (uint32_t k = 0; k < Node_GetOutputResourceCount(node); k++) {
                NiString member;
                Str_ConstructSized(&member, 0x100);
                NiRefObject* resource = static_cast<NiRefObject*>(Node_GetOutputResource(node, k));
                const char* kind = *GetResourceType(node);
                if (kind == reinterpret_cast<const char*>(0x00955AF0) ||
                    (kind != nullptr && crt::Strcmp(kind, "Uniform") == 0)) {
                    SetAdd(uniforms, resource);
                } else {
                    kind = *GetResourceType(node);
                    if (!(kind == reinterpret_cast<const char*>(0x00955ADC) ||
                          (kind != nullptr && crt::Strcmp(kind, "Static") == 0))) {
                        SetAdd(&inputs, resource);
                        Str_AppendText(&member, "In.");
                    }
                }
                Str_Append(&member, Get14(resource));
                re::Ptr32<char> memberName = MakeFixed(Str_Chars(&member));
                NameOutputBindings(self, resource, &memberName, false);
                FixedStringRelease(&memberName);
                Str_Destruct(&member);
            }
        }
        if (IsKindOf(node, FragmentNode_ClassBit)) {
            // its code block's function, once per block
            re::Ptr32<char> language = MakeFixed(Virtual<const char*>(self->cache, 9));
            re::Ptr32<char>* platform = &self->platform;
            NiRefObject* block = static_cast<NiRefObject*>(
                FragmentNode_GetCodeBlock(node, &language, platform, Virtual<const void*>(self->cache, 5)));
            Hold(block);
            int32_t found = -1;
            for (uint32_t b = 0; b < blocks.count; b++) {
                if (blocks.base[b] == block) {
                    found = int32_t(b);
                    break;
                }
            }
            bool fresh = found == -1;
            LetGo(block);
            FixedStringRelease(&language);
            if (fresh) {
                NiString function;
                Str_AppendText(&functions, Str_Chars(Cfg_GenerateFunction(self, &function, node,
                                                                          Virtual<const void*>(self->cache, 5))));
                Str_Destruct(&function);
                re::Ptr32<char> language2 = MakeFixed(Virtual<const char*>(self->cache, 9));
                block = static_cast<NiRefObject*>(
                    FragmentNode_GetCodeBlock(node, &language2, platform, Virtual<const void*>(self->cache, 5)));
                SetAdd(&blocks, block);
                FixedStringRelease(&language2);
            }
        }
    }
    // the uniforms: "type name[count];"
    for (uint32_t i = 0; i < uniforms->count; i++) {
        void* uniform = uniforms->base[i];
        NiString type;
        Str_ConstructFixed(&type, Get08(uniform));
        Str_AppendText(&header, Str_Chars(&type));
        Str_Destruct(&type);
        Str_AppendText(&header, " ");
        Str_Append(&header, Get14(uniform));
        if (GetCount(uniform) != 1) {
            char count[0x20];
            NiSprintf(count, 0x20, "%d", GetCount(uniform));
            Str_AppendText(&header, "[");
            Str_AppendText(&header, count);
            Str_AppendText(&header, "]");
        }
        Str_AppendText(&header, ";\n");
    }
    NiString members;
    Str_AppendText(&input, Str_Chars(Cfg_GenerateStructMembers(self, &members, &inputs)));
    Str_Destruct(&members);
    Str_AppendText(&input, "\n};\n\n");
    Str_AppendText(&output, Str_Chars(Cfg_GenerateStructMembers(self, &members, &outputs)));
    Str_Destruct(&members);
    Str_AppendText(&output, "\n};\n\n");
    uint32_t calls = 0;
    for (uint32_t i = 0; i < sorted.count; i++) {
        NiRefObject* node = sorted.base[i];
        if (node == nullptr) {
            continue;
        }
        if (IsKindOf(node, FragmentNode_ClassBit)) {
            NiString text;
            Str_AppendText(&main, Str_Chars(Cfg_GenerateFunctionCall(self, &text, node, calls)));
            Str_Destruct(&text);
            Str_AppendText(&main, "\n");
            calls++;
        } else if (IsKindOf(node, ConsumerNode_ClassBit)) {
            NiString text;
            Str_AppendText(&main, Str_Chars(Cfg_GenerateOutputAssignments(self, &text, node)));
            Str_Destruct(&text);
        }
    }
    Str_AppendText(&main, "    return Out;\n");
    Str_AppendText(&main, "}\n\n");
    NiString empty;
    Str_Construct(&empty, 0x400);
    uint32_t total = Str_Length(&empty) + Str_Length(&main) + Str_Length(&header) + Str_Length(&functions) +
                     Str_Length(&output) + Str_Length(&input);
    NiString code;
    Str_Construct(&code, int(total));
    Str_Append(&code, &empty);
    Str_Append(&code, &header);
    Str_Append(&code, &functions);
    Str_Append(&code, &input);
    Str_Append(&code, &output);
    Str_Append(&code, &main);
    NiRefObject* program = Virtual<NiRefObject*>(self->cache, 4, name, Str_Chars(&code), uniforms);
    Str_Destruct(&code);
    Str_Destruct(&empty);
    FreeSet(&blocks);
    FreeSet(&pending);
    FreeSet(&sorted);
    FreeSet(&unused);
    FreeSet(&outputs);
    FreeSet(&inputs);
    Str_Destruct(&main);
    Str_Destruct(&header);
    Str_Destruct(&functions);
    Str_Destruct(&output);
    Str_Destruct(&input);
    return program;
}

} // namespace

// four-byte getters (too short to hook), shared with other classes' slots
// 0x007E80C0: [8d 41 2c c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static void* __fastcall Tiny_007E80C0(uint8_t* self, re::Edx) {
    return self + 0x2C;
}
RE_UNHOOKED_FN("NiMaterialConfigurator::GetPlatform", 0x007E80C0, &Tiny_007E80C0);       // lea eax, [ecx+0x2C]
// 0x007E8330: [8b 41 30 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __fastcall Tiny_007E8330(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const uint32_t*>(self + 0x30);
}
RE_UNHOOKED_FN("NiMaterialConfigurator::GetProgramCache", 0x007E8330, &Tiny_007E8330);   // mov eax, [ecx+0x30]

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIMATCFG_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIMATCFG_REVERSED("NiMaterialConfigurator::CanBind", 0x007E8060, ni::CfgCanBind);
NIMATCFG_REVERSED("NiMaterialConfigurator::SetDescription", 0x007E80D0, ni::CfgSetDescription);
NIMATCFG_REVERSED("NiMaterialConfigurator::CanBindTypes", 0x007E80E0, ni::CfgCanBindTypes);
NIMATCFG_REVERSED("NiMaterialConfigurator::GetNodeContainingResource", 0x007E8170, ni::CfgGetNodeContainingResource);
NIMATCFG_REVERSED("NiMaterialConfigurator::GetNode", 0x007E81C0, ni::CfgGetNode);
NIMATCFG_REVERSED("NiMaterialConfigurator::GetInputBinding", 0x007E8220, ni::CfgGetInputBinding);
NIMATCFG_REVERSED("NiMaterialConfigurator::GetOutputBinding", 0x007E8270, ni::CfgGetOutputBinding);
NIMATCFG_REVERSED("NiMaterialConfigurator::SetVariable", 0x007E82C0, ni::CfgSetVariable);
NIMATCFG_REVERSED("NiMaterialConfigurator::GetVariableName", 0x007E8340, ni::CfgGetVariableName);
NIMATCFG_REVERSED("NiMaterialConfigurator::Reset", 0x007E8450, ni::CfgReset);
NIMATCFG_REVERSED("NiMaterialConfigurator::GenerateOutputAssignments", 0x007E8470, ni::CfgGenerateOutputAssignments);
NIMATCFG_REVERSED("NiTObjectSet<NiPointer>::Realloc", 0x007E8680, ni::SetRealloc);
NIMATCFG_REVERSED("NiMaterialConfigurator::GenerateResourceDeclaration", 0x007E8730, ni::CfgGenerateResourceDeclaration);
NIMATCFG_REVERSED("NiMaterialConfigurator::GenerateFunction", 0x007E8910, ni::CfgGenerateFunction);
NIMATCFG_REVERSED("NiMaterialConfigurator::GenerateStructMembers", 0x007E8B40, ni::CfgGenerateStructMembers);
NIMATCFG_REVERSED("NiMaterialConfigurator::GenerateFunctionCall", 0x007E8DE0, ni::CfgGenerateFunctionCall);
NIMATCFG_REVERSED("NiMaterialConfigurator::NiMaterialConfigurator", 0x007E92F0, ni::CfgConstruct);
NIMATCFG_REVERSED("NiMaterialConfigurator::~NiMaterialConfigurator", 0x007E93E0, ni::CfgDestruct);
NIMATCFG_REVERSED("NiMaterialConfigurator::DeletingDestructor", 0x007E9480, ni::CfgDeletingDestructor);
NIMATCFG_REVERSED("NiMaterialConfigurator::AddBindingObject", 0x007E94A0, ni::CfgAddBinding);
NIMATCFG_REVERSED("NiMaterialConfigurator::AddBinding", 0x007E9520, ni::CfgAddBindingResources);
NIMATCFG_REVERSED("NiMaterialConfigurator::AddBindingNames", 0x007E95B0, ni::CfgAddBindingNames);
NIMATCFG_REVERSED("NiMaterialConfigurator::AddBindingByName", 0x007E9630, ni::CfgAddBindingByName);
NIMATCFG_REVERSED("NiMaterialConfigurator::AddBindingFromOutput", 0x007E96C0, ni::CfgAddBindingFromOutput);
NIMATCFG_REVERSED("NiMaterialConfigurator::Evaluate", 0x007E9740, ni::CfgEvaluate);
