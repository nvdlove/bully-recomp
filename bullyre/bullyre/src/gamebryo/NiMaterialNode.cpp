// The material node graph's pieces (what the generated node libraries, NiMaterialLibraries.cpp,
// are built from, and what the configurator links):
//   NiMaterialNodeLibrary (vtable 0x961DEC, 0x1C): the nodes by name, a version
//   NiMaterialResource (vtable 0x961E2C, 0x2C): a typed, named input or output of a node
//   NiMaterialNode (vtable 0x961E34, 0x30): a type, a name, its inputs and outputs (references)
//   NiMaterialResourceConsumerNode / ProducerNode (vtables 0x962668 / 0x9626A8, 0x34): nodes
//     standing for a program's outputs / its inputs, uniforms and statics, with a resource type
//   NiMaterialFragmentNode (vtable 0x955FC4, 0x44): a node with HLSL code blocks and a
//     description
//   NiCodeBlock (vtable 0x962700, 0x18): a code fragment for a language, platforms and targets
// and the descriptors' key printer.
//
// NiMaterialNode's slots: 2 GetRTTI, 3 ContainsResource, 4 IsOutputResource, 5 IsInputResource,
// 6 Clone (a new node CopyMembers fills), 7 CopyMembers(source).
//
// The NiTObjectArray<NiPointer<...>> functions every pointer array shares (SetSize 0x856170,
// SetAt 0x40E800) and the NiPointer temporaries the inlined Add holds are as the code has them.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiMaterialNode_RTTI, 0x020E4118);
GAME_VAR(RTTI, NiMaterialResourceConsumerNode_RTTI, 0x020E414C);
GAME_VAR(RTTI, NiMaterialResourceProducerNode_RTTI, 0x020E4180);
GAME_VAR(long, ms_uiObjects, 0x020E2168);

constexpr uintptr_t kNiRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t kNodeLibraryVtable = 0x00961DEC;
constexpr uintptr_t kNodeArrayVtable = 0x00961DE8;      // NiTObjectArray<NiMaterialNodePtr>
constexpr uintptr_t kTArrayVtable = 0x009893C0;         // NiTArray's own
constexpr uintptr_t kResourceVtable = 0x00961E2C;
constexpr uintptr_t kNodeVtable = 0x00961E34;
constexpr uintptr_t kConsumerNodeVtable = 0x00962668;
constexpr uintptr_t kProducerNodeVtable = 0x009626A8;
constexpr uintptr_t kFragmentNodeVtable = 0x00955FC4;
constexpr uintptr_t kCodeBlockVtable = 0x00962700;

struct NiString {
    re::Ptr32<char> handle;
};

struct NiMaterialResource : NiRefObject {
    re::Ptr32<char> type;            // +0x8 (NiFixedString: "float3"...)
    re::Ptr32<char> semantic;        // +0xC
    re::Ptr32<char> label;           // +0x10
    re::Ptr32<char> variable;        // +0x14
    re::Ptr32<char> defaultValue;    // +0x18
    uint32_t count;        // +0x1C (1)
    uint32_t source;       // +0x20 (1 predefined, 2 object, 3 constant, 4 attribute, 5 global)
    uint32_t extraData;    // +0x24
    uint32_t objectType;   // +0x28
};
static_assert(sizeof(NiMaterialResource) == 0x2C);

struct NiMaterialNode : NiRefObject {
    re::Ptr32<char> type;                               // +0x8
    re::Ptr32<char> name;                               // +0xC
    NiTArray<NiMaterialResource*> inputs;     // +0x10
    NiTArray<NiMaterialResource*> outputs;    // +0x20
};
static_assert(offsetof(NiMaterialNode, inputs) == 0x10);
static_assert(sizeof(NiMaterialNode) == 0x30);

struct NiMaterialResourceNode : NiMaterialNode {
    re::Ptr32<char> resourceType;   // +0x30
};
static_assert(sizeof(NiMaterialResourceNode) == 0x34);

struct NiCodeBlock : NiRefObject {
    re::Ptr32<char> language;   // +0x8
    re::Ptr32<char> platform;   // +0xC
    re::Ptr32<char> target;     // +0x10
    NiString text;    // +0x14
};
static_assert(sizeof(NiCodeBlock) == 0x18);

struct NiMaterialFragmentNode : NiMaterialNode {
    NiTArray<NiCodeBlock*> codeBlocks;   // +0x30
    NiString description;                // +0x40
};
static_assert(sizeof(NiMaterialFragmentNode) == 0x44);

struct NiMaterialNodeLibrary : NiRefObject {
    NiTArray<NiMaterialNode*> nodes;   // +0x8
    uint16_t version;                  // +0x18
    uint16_t pad1A;
};
static_assert(sizeof(NiMaterialNodeLibrary) == 0x1C);

// a set of strings (NiTPrimitiveSet<const char*>): the elements, allocated, count
struct StringSet {
    re::Ptr32<re::Ptr32<const char>> base;
    uint32_t allocated;
    uint32_t count;
};

GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(re::Ptr32<char>*, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, const re::Ptr32<char>* other);
GAME_FN(uint8_t, __thiscall, FixedString_Contains, 0x00406510, const re::Ptr32<char>* self, const char* text);
GAME_FN(void, __thiscall, PointerArray_SetSize, 0x00856170, void* array, uint32_t size);
GAME_FN(void, __thiscall, PointerArray_SetAt, 0x0040E800, void* array, uint32_t index, const re::Ptr32<NiRefObject>* value);
GAME_FN(void, __cdecl, PointerArray_Delete, 0x00836F80, void* base);
GAME_FN(const char*, __thiscall, NiString_GetChars, 0x00773DB0, const NiString* self);
GAME_FN(void, __thiscall, NiString_AssignChars, 0x007745E0, NiString* self, const char* text);
GAME_FN(void, __thiscall, NiString_ConstructSized, 0x007740D0, NiString* self, uint32_t size);
GAME_FN(void, __thiscall, NiString_Destruct, 0x00774240, NiString* self);
// the getters and setters the code shares across classes (by offset)
GAME_FN(re::Ptr32<char>*, __thiscall, Get08, 0x0077C280, void* self);      // +0x8
GAME_FN(re::Ptr32<char>*, __thiscall, Get0C, 0x008BAC30, void* self);      // +0xC
GAME_FN(re::Ptr32<char>*, __thiscall, Get10, 0x007E2BC0, void* self);      // +0x10
GAME_FN(re::Ptr32<char>*, __thiscall, Get14, 0x007E2BD0, void* self);      // +0x14
GAME_FN(void, __thiscall, Set08, 0x0077C3A0, void* self, const re::Ptr32<char>* value);
GAME_FN(void, __thiscall, Set0C, 0x007EAD00, void* self, const re::Ptr32<char>* value);
GAME_FN(void, __thiscall, Set10, 0x007E2D60, void* self, const re::Ptr32<char>* value);
GAME_FN(void, __thiscall, Set14, 0x007E2D70, void* self, const re::Ptr32<char>* value);
GAME_FN(void, __thiscall, Set18, 0x007F80C0, void* self, const re::Ptr32<char>* value);
GAME_FN(void, __thiscall, Set1C, 0x008A2EF0, void* self, uintptr_t value);
GAME_FN(void, __thiscall, Set20, 0x00670BA0, void* self, uintptr_t value);
GAME_FN(void, __thiscall, Set24, 0x007E2BF0, void* self, uint32_t value);
GAME_FN(void, __thiscall, Set28, 0x007E2C00, void* self, uint32_t value);
// (this file's, called where modes apply)
GAME_FN(NiMaterialResource*, __thiscall, Resource_Construct, 0x007E2C10, void* self);
GAME_FN(void, __thiscall, Resource_Destruct, 0x007E2C70, NiMaterialResource* self);
GAME_FN(NiMaterialResource*, __thiscall, Resource_Clone, 0x007E2D80, NiMaterialResource* self);
GAME_FN(NiMaterialResource*, __thiscall, Node_GetOutputResourceByVariableName, 0x007E2EE0, NiMaterialNode* self,
        const re::Ptr32<char>* name);
GAME_FN(void, __thiscall, Node_Destruct, 0x007E2FB0, NiMaterialNode* self);
GAME_FN(NiMaterialNode*, __thiscall, Node_Construct, 0x007E3060, void* self);
GAME_FN(void, __thiscall, Node_CopyMembers, 0x007E3480, NiMaterialNode* self, NiMaterialNode* source);
GAME_FN(void, __thiscall, CodeBlock_Destruct, 0x007EAC50, NiCodeBlock* self);

// NiRefObject's constructor inlined
inline void RefObjectConstruct(NiRefObject* self) {
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    self->refCount = 0;
    Increment(&ms_uiObjects);
}

// ~NiRefObject inlined
inline void RefObjectDestruct(NiRefObject* self) {
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    Decrement(&ms_uiObjects);
}

inline void ReleaseObject(NiRefObject* object) {
    if (object != nullptr && Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

// NiTObjectArray<NiPointer<T>>::Add inlined: the object held by a temporary NiPointer around
// the SetAt (grown by growBy when full)
template <typename T>
void AddPointer(NiTArray<T*>& array, NiRefObject* object) {
    re::Ptr32<NiRefObject> held = object;
    if (held != nullptr) {
        Increment(&held->refCount);
    }
    uint32_t index = array.size;
    if (index >= array.maxSize) {
        PointerArray_SetSize(&array, array.growBy + index);
    }
    PointerArray_SetAt(&array, index, &held);
    ReleaseObject(held);
}

// NiTObjectArray's destructor inlined: the base vtable, the elements' delete[]
template <typename T>
void DestructPointerArray(NiTArray<T*>& array) {
    array.vtable = reinterpret_cast<void*>(kTArrayVtable);
    PointerArray_Delete(array.base);
}

template <typename T>
void ConstructArray(NiTArray<T*>& array, uintptr_t vtable) {
    array.maxSize = 0;
    array.growBy = 1;
    array.size = 0;
    array.esize = 0;
    array.base = nullptr;
    array.vtable = reinterpret_cast<void*>(vtable);
}

// a new resource with its fields set through the setters (the order the code has)
NiMaterialResource* NewResource(const re::Ptr32<char>* type, const re::Ptr32<char>* semantic, const re::Ptr32<char>* label, const re::Ptr32<char>* variable,
                                uint32_t count, uint32_t source, uint32_t objectType, uint32_t extraData) {
    void* memory = NiNew(0x2C);
    NiMaterialResource* resource = memory != nullptr ? Resource_Construct(memory) : nullptr;
    Set08(resource, type);
    Set14(resource, variable);
    Set10(resource, label);
    Set0C(resource, semantic);
    Set1C(resource, count);
    Set20(resource, source);
    Set24(resource, extraData);
    Set28(resource, objectType);
    return resource;
}

// ---- NiMaterialNodeLibrary

// 0x7E2A50 GetVersion
uint16_t __fastcall LibraryGetVersion(NiMaterialNodeLibrary* self, re::Edx) {
    return self->version;
}

// 0x7E2A60 GetAttachableNodeByName(name, clone): the node of that name (the handles compared),
// cloned (slot 6) when asked
NiMaterialNode* __fastcall LibraryGetAttachableNodeByName(NiMaterialNodeLibrary* self, re::Edx, const re::Ptr32<char>* name,
                                                          int clone) {
    for (uint32_t i = 0; i < self->nodes.size; i++) {
        NiMaterialNode* node = self->nodes.base[i];
        if (node != nullptr && *Get0C(node) == *name) {
            if (re::Flag(clone)) {
                return Virtual<NiMaterialNode*>(node, 6);
            }
            return node;
        }
    }
    return nullptr;
}

// 0x7E2AC0 NiMaterialNodeLibrary(version): room for 100 nodes
NiMaterialNodeLibrary* __fastcall LibraryConstruct(NiMaterialNodeLibrary* self, re::Edx, uint32_t version) {
    RefObjectConstruct(self);
    self->vtable = reinterpret_cast<void*>(kNodeLibraryVtable);
    ConstructArray(self->nodes, kNodeArrayVtable);
    self->version = uint16_t(version);
    PointerArray_SetSize(&self->nodes, 100);
    return self;
}

// 0x7E2B20 NiTObjectArray<NiMaterialNodePtr>'s deleting destructor
NiTArray<NiMaterialNode*>* __fastcall NodeArrayDeletingDestructor(NiTArray<NiMaterialNode*>* self, re::Edx,
                                                                  uint32_t flags) {
    DestructPointerArray(*self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x10);
    }
    return self;
}

// 0x7E2B50 the library's deleting destructor
NiMaterialNodeLibrary* __fastcall LibraryDeletingDestructor(NiMaterialNodeLibrary* self, re::Edx, uint32_t flags) {
    DestructPointerArray(self->nodes);
    RefObjectDestruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x1C);
    }
    return self;
}

// 0x7E2B90 NiGPUProgramDescriptor::GenerateKey(buffer, size): the four words in hex
uint8_t __fastcall DescriptorGenerateKey(const uint32_t* self, re::Edx, char* buffer, uint32_t size) {
    NiSprintf(buffer, size, "%.8X-%.8X-%.8X-%.8X", self[1], self[2], self[3], self[4]);
    return 1;
}

// ---- NiMaterialResource

// 0x7E2BF0 SetExtraData / 0x7E2C00 SetObjectType
void __fastcall ResourceSetExtraData(NiMaterialResource* self, re::Edx, uint32_t value) {
    self->extraData = value;
}
void __fastcall ResourceSetObjectType(NiMaterialResource* self, re::Edx, uint32_t value) {
    self->objectType = value;
}

// 0x7E2C10 NiMaterialResource(): the names empty, a count of 1
NiMaterialResource* __fastcall ResourceConstruct(NiMaterialResource* self, re::Edx) {
    RefObjectConstruct(self);
    self->vtable = reinterpret_cast<void*>(kResourceVtable);
    self->type = kNullFixedString;
    self->semantic = kNullFixedString;
    self->label = kNullFixedString;
    self->variable = kNullFixedString;
    self->defaultValue = kNullFixedString;
    self->source = 0;
    self->extraData = 0;
    self->objectType = 0;
    self->count = 1;
    return self;
}

// 0x7E2C70 ~NiMaterialResource
void __fastcall ResourceDestruct(NiMaterialResource* self, re::Edx) {
    FixedStringRelease(&self->defaultValue);
    FixedStringRelease(&self->variable);
    FixedStringRelease(&self->label);
    FixedStringRelease(&self->semantic);
    FixedStringRelease(&self->type);
    RefObjectDestruct(self);
}

// 0x7E2D60 SetLabel / 0x7E2D70 SetVariable (also the code block's SetTarget)
re::Ptr32<char>* __fastcall ResourceSetLabel(NiMaterialResource* self, re::Edx, const re::Ptr32<char>* value) {
    return FixedString_Assign(&self->label, value);
}
re::Ptr32<char>* __fastcall ResourceSetVariable(NiMaterialResource* self, re::Edx, const re::Ptr32<char>* value) {
    return FixedString_Assign(&self->variable, value);
}

// 0x7E2D80 Clone (a failed allocation written through)
NiMaterialResource* __fastcall ResourceClone(NiMaterialResource* self, re::Edx) {
    void* memory = NiNew(0x2C);
    NiMaterialResource* clone = memory != nullptr ? Resource_Construct(memory) : nullptr;
    FixedString_Assign(&clone->type, &self->type);
    FixedString_Assign(&clone->semantic, &self->semantic);
    FixedString_Assign(&clone->label, &self->label);
    FixedString_Assign(&clone->variable, &self->variable);
    clone->count = self->count;
    FixedString_Assign(&clone->defaultValue, &self->defaultValue);
    clone->source = self->source;
    clone->objectType = self->objectType;
    clone->extraData = self->extraData;
    return clone;
}

// 0x7E2E00 the deleting destructor
NiMaterialResource* __fastcall ResourceDeletingDestructor(NiMaterialResource* self, re::Edx, uint32_t flags) {
    Resource_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x2C);
    }
    return self;
}

// ---- NiMaterialNode

// vf3 ContainsResource: an input (slot 5) or an output (slot 4)
uint8_t __fastcall NodeContainsResource(NiMaterialNode* self, re::Edx, NiMaterialResource* resource) {
    if (Virtual<uint8_t>(self, 5, resource) != 0) {
        return 1;
    }
    if (Virtual<uint8_t>(self, 4, resource) != 0) {
        return 1;
    }
    return 0;
}

// 0x7E2E50 GetInputResourceCount / 0x7E2E60 GetInputResource(i)
uint32_t __fastcall NodeGetInputResourceCount(NiMaterialNode* self, re::Edx) {
    return self->inputs.size;
}
NiMaterialResource* __fastcall NodeGetInputResource(NiMaterialNode* self, re::Edx, uint32_t index) {
    return self->inputs.base[index];
}

// 0x7E2E70 GetInputResourceByVariableName(name) (the handles compared)
NiMaterialResource* __fastcall NodeGetInputResourceByVariableName(NiMaterialNode* self, re::Edx, const re::Ptr32<char>* name) {
    for (uint32_t i = 0; i < self->inputs.size; i++) {
        NiMaterialResource* resource = self->inputs.base[i];
        if (resource != nullptr && *Get14(resource) == *name) {
            return resource;
        }
    }
    return nullptr;
}

// 0x7E2EC0 GetOutputResourceCount / 0x7E2ED0 GetOutputResource(i)
uint32_t __fastcall NodeGetOutputResourceCount(NiMaterialNode* self, re::Edx) {
    return self->outputs.size;
}
NiMaterialResource* __fastcall NodeGetOutputResource(NiMaterialNode* self, re::Edx, uint32_t index) {
    return self->outputs.base[index];
}

// 0x7E2EE0 GetOutputResourceByVariableName(name)
NiMaterialResource* __fastcall NodeGetOutputResourceByVariableName(NiMaterialNode* self, re::Edx, const re::Ptr32<char>* name) {
    for (uint32_t i = 0; i < self->outputs.size; i++) {
        NiMaterialResource* resource = self->outputs.base[i];
        if (resource != nullptr && *Get14(resource) == *name) {
            return resource;
        }
    }
    return nullptr;
}

// vf4 IsOutputResource / vf5 IsInputResource
uint8_t __fastcall NodeIsOutputResource(NiMaterialNode* self, re::Edx, NiMaterialResource* resource) {
    for (uint32_t i = 0; i < self->outputs.size; i++) {
        NiMaterialResource* r = self->outputs.base[i];
        if (r != nullptr && r == resource) {
            return 1;
        }
    }
    return 0;
}
uint8_t __fastcall NodeIsInputResource(NiMaterialNode* self, re::Edx, NiMaterialResource* resource) {
    for (uint32_t i = 0; i < self->inputs.size; i++) {
        NiMaterialResource* r = self->inputs.base[i];
        if (r != nullptr && r == resource) {
            return 1;
        }
    }
    return 0;
}

// 0x7E2FB0 ~NiMaterialNode
void __fastcall NodeDestruct(NiMaterialNode* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNodeVtable);
    DestructPointerArray(self->outputs);
    DestructPointerArray(self->inputs);
    FixedStringRelease(&self->name);
    FixedStringRelease(&self->type);
    RefObjectDestruct(self);
}

// vf2
const RTTI* __fastcall NodeGetRTTI(NiMaterialNode*, re::Edx) {
    return &NiMaterialNode_RTTI;
}

// 0x7E3060 NiMaterialNode()
NiMaterialNode* __fastcall NodeConstruct(NiMaterialNode* self, re::Edx) {
    RefObjectConstruct(self);
    self->vtable = reinterpret_cast<void*>(kNodeVtable);
    self->type = kNullFixedString;
    self->name = kNullFixedString;
    ConstructArray(self->inputs, kTArrayVtable);
    ConstructArray(self->outputs, kTArrayVtable);
    return self;
}

// vf0 the deleting destructor
NiMaterialNode* __fastcall NodeDeletingDestructor(NiMaterialNode* self, re::Edx, uint32_t flags) {
    Node_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x30);
    }
    return self;
}

// vf6 Clone: a new plain node, filled by its CopyMembers (slot 7; called on a failed allocation
// too). The consumer and producer nodes share it
NiMaterialNode* __fastcall NodeClone(NiMaterialNode* self, re::Edx) {
    void* memory = NiNew(0x30);
    NiMaterialNode* clone = memory != nullptr ? Node_Construct(memory) : nullptr;
    Virtual<void>(clone, 7, self);
    return clone;
}

// 0x7E3130 AddInputResource(resource)
void __fastcall NodeAddInputResourceObject(NiMaterialNode* self, re::Edx, NiMaterialResource* resource) {
    AddPointer(self->inputs, resource);
}

// 0x7E31A0 AddInputResource(type, semantic, label, variable, count, source, object type, extra
// data): a new input
NiMaterialResource* __fastcall NodeAddInputResource(NiMaterialNode* self, re::Edx, const re::Ptr32<char>* type,
                                                    const re::Ptr32<char>* semantic, const re::Ptr32<char>* label, const re::Ptr32<char>* variable,
                                                    uint32_t count, uint32_t source, uint32_t objectType,
                                                    uint32_t extraData) {
    NiMaterialResource* resource = NewResource(type, semantic, label, variable, count, source, objectType, extraData);
    AddPointer(self->inputs, resource);
    return resource;
}

// 0x7E3280 AddOutputResource(the same): the existing output of that variable, else a new one
NiMaterialResource* __fastcall NodeAddOutputResource(NiMaterialNode* self, re::Edx, const re::Ptr32<char>* type,
                                                     const re::Ptr32<char>* semantic, const re::Ptr32<char>* label, const re::Ptr32<char>* variable,
                                                     uint32_t count, uint32_t source, uint32_t objectType,
                                                     uint32_t extraData) {
    NiMaterialResource* existing = Node_GetOutputResourceByVariableName(self, variable);
    if (existing != nullptr) {
        return existing;
    }
    NiMaterialResource* resource = NewResource(type, semantic, label, variable, count, source, objectType, extraData);
    AddPointer(self->outputs, resource);
    return resource;
}

// 0x7E3370 AddOutputConstant(type, value): a new output, source 3 (a constant)
NiMaterialResource* __fastcall NodeAddOutputConstant(NiMaterialNode* self, re::Edx, const re::Ptr32<char>* type,
                                                     const re::Ptr32<char>* value) {
    void* memory = NiNew(0x2C);
    NiMaterialResource* resource = memory != nullptr ? Resource_Construct(memory) : nullptr;
    Set08(resource, type);
    Set18(resource, value);
    Set20(resource, 3);
    AddPointer(self->outputs, resource);
    return resource;
}

// 0x7E3410 AddOutputResource(resource)
void __fastcall NodeAddOutputResourceObject(NiMaterialNode* self, re::Edx, NiMaterialResource* resource) {
    AddPointer(self->outputs, resource);
}

// vf7 CopyMembers(source): the type and name, clones of its inputs and outputs
void __fastcall NodeCopyMembers(NiMaterialNode* self, re::Edx, NiMaterialNode* source) {
    FixedString_Assign(&self->type, &source->type);
    FixedString_Assign(&self->name, &source->name);
    for (uint32_t i = 0; i < source->inputs.size; i++) {
        AddPointer(self->inputs, Resource_Clone(source->inputs.base[i]));
    }
    for (uint32_t i = 0; i < source->outputs.size; i++) {
        AddPointer(self->outputs, Resource_Clone(source->outputs.base[i]));
    }
}

// ---- NiMaterialResourceConsumerNode / NiMaterialResourceProducerNode

// 0x7EA7D0 / 0x7EA880 (name, resource type)
NiMaterialResourceNode* __fastcall ConsumerNodeConstruct(NiMaterialResourceNode* self, re::Edx, const re::Ptr32<char>* name,
                                                         const re::Ptr32<char>* resourceType) {
    Node_Construct(self);
    self->vtable = reinterpret_cast<void*>(kConsumerNodeVtable);
    self->resourceType = kNullFixedString;
    Set0C(self, name);
    FixedString_Assign(&self->resourceType, resourceType);
    return self;
}
NiMaterialResourceNode* __fastcall ProducerNodeConstruct(NiMaterialResourceNode* self, re::Edx, const re::Ptr32<char>* name,
                                                         const re::Ptr32<char>* resourceType) {
    Node_Construct(self);
    self->vtable = reinterpret_cast<void*>(kProducerNodeVtable);
    self->resourceType = kNullFixedString;
    Set0C(self, name);
    FixedString_Assign(&self->resourceType, resourceType);
    return self;
}

const RTTI* __fastcall ConsumerNodeGetRTTI(NiMaterialResourceNode*, re::Edx) {
    return &NiMaterialResourceConsumerNode_RTTI;
}
const RTTI* __fastcall ProducerNodeGetRTTI(NiMaterialResourceNode*, re::Edx) {
    return &NiMaterialResourceProducerNode_RTTI;
}

// 0x7EA820 the deleting destructor both share (no vtable of their own set)
NiMaterialResourceNode* __fastcall ResourceNodeDeletingDestructor(NiMaterialResourceNode* self, re::Edx,
                                                                  uint32_t flags) {
    FixedStringRelease(&self->resourceType);
    Node_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x34);
    }
    return self;
}

// ---- NiMaterialFragmentNode

// 0x7EA8E0 SetDescription(NiString)
void __fastcall FragmentNodeSetDescription(NiMaterialFragmentNode* self, re::Edx, const NiString* text) {
    NiString_AssignChars(&self->description, NiString_GetChars(text));
}

// 0x7EA900 GetCodeBlock(language, platform, targets): for each target in turn, the first block
// whose platform and language contain the ones asked and whose target contains it
NiCodeBlock* __fastcall FragmentNodeGetCodeBlock(NiMaterialFragmentNode* self, re::Edx, const re::Ptr32<char>* language,
                                                 const re::Ptr32<char>* platform, const StringSet* targets) {
    for (uint32_t t = 0; t < targets->count; t++) {
        for (uint32_t i = 0; i < self->codeBlocks.size; i++) {
            NiCodeBlock* block = self->codeBlocks.base[i];
            if (block == nullptr) {
                continue;
            }
            if (FixedString_Contains(Get0C(block), *platform) == 0) {
                continue;
            }
            if (FixedString_Contains(Get08(block), *language) == 0) {
                continue;
            }
            const re::Ptr32<char>* targetHandle = Get10(block);
            re::Ptr32<char> target = FixedStringAddRef(*targetHandle);   // a copy (NiFixedString's)
            target = *targetHandle;
            uint8_t found = FixedString_Contains(&target, targets->base[t]);
            FixedStringRelease(&target);
            if (found != 0) {
                return block;
            }
        }
    }
    return nullptr;
}

// 0x7EAA50 AddCodeBlock(block)
void __fastcall FragmentNodeAddCodeBlock(NiMaterialFragmentNode* self, re::Edx, NiCodeBlock* block) {
    AddPointer(self->codeBlocks, block);
}

// vf6 Clone: a new fragment node, filled by its CopyMembers (slot 7; called on a failed
// allocation too)
NiMaterialFragmentNode* __fastcall FragmentNodeClone(NiMaterialFragmentNode* self, re::Edx) {
    NiMaterialFragmentNode* clone = static_cast<NiMaterialFragmentNode*>(NiNew(0x44));
    if (clone != nullptr) {
        Node_Construct(clone);
        clone->vtable = reinterpret_cast<void*>(kFragmentNodeVtable);
        clone->codeBlocks.maxSize = 0;
        clone->codeBlocks.growBy = 1;
        clone->codeBlocks.size = 0;
        clone->codeBlocks.esize = 0;
        clone->codeBlocks.base = nullptr;
        clone->codeBlocks.vtable = reinterpret_cast<void*>(kTArrayVtable);
        NiString_ConstructSized(&clone->description, 0x100);
    } else {
        clone = nullptr;
    }
    Virtual<void>(clone, 7, self);
    return clone;
}

// vf7 CopyMembers(source): NiMaterialNode's, the source's code blocks themselves (shared), its
// description
void __fastcall FragmentNodeCopyMembers(NiMaterialFragmentNode* self, re::Edx, NiMaterialFragmentNode* source) {
    Node_CopyMembers(self, source);
    for (uint32_t i = 0; i < source->codeBlocks.size; i++) {
        AddPointer(self->codeBlocks, source->codeBlocks.base[i]);
    }
    NiString_AssignChars(&self->description, NiString_GetChars(&source->description));
}

// ---- NiCodeBlock

// 0x7EABE0 SetText(NiString)
void __fastcall CodeBlockSetText(NiCodeBlock* self, re::Edx, const NiString* text) {
    NiString_AssignChars(&self->text, NiString_GetChars(text));
}

// 0x7EAC00 NiCodeBlock()
NiCodeBlock* __fastcall CodeBlockConstruct(NiCodeBlock* self, re::Edx) {
    RefObjectConstruct(self);
    self->vtable = reinterpret_cast<void*>(kCodeBlockVtable);
    self->language = kNullFixedString;
    self->platform = kNullFixedString;
    self->target = kNullFixedString;
    NiString_ConstructSized(&self->text, 0x100);
    return self;
}

// 0x7EAC50 ~NiCodeBlock
void __fastcall CodeBlockDestruct(NiCodeBlock* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kCodeBlockVtable);
    NiString_Destruct(&self->text);
    FixedStringRelease(&self->target);
    FixedStringRelease(&self->platform);
    FixedStringRelease(&self->language);
    RefObjectDestruct(self);
}

// 0x7EAD00 the +0xC setter (the node's SetName, the resource's SetSemantic, the code block's
// SetPlatform)
re::Ptr32<char>* __fastcall NodeSetName(NiMaterialNode* self, re::Edx, const re::Ptr32<char>* value) {
    return FixedString_Assign(&self->name, value);
}

// 0x7EAD10 the code block's deleting destructor
NiCodeBlock* __fastcall CodeBlockDeletingDestructor(NiCodeBlock* self, re::Edx, uint32_t flags) {
    CodeBlock_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x18);
    }
    return self;
}

} // namespace

// four-byte getters (too short to hook)
// 0x007E2BC0: [8d 41 10 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static void* __fastcall Tiny_007E2BC0(uint8_t* self, re::Edx) {
    return self + 0x10;
}
RE_UNHOOKED_FN("NiMaterialResource::GetLabel", 0x007E2BC0, &Tiny_007E2BC0);          // lea eax, [ecx+0x10]
// 0x007E2BD0: [8d 41 14 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static void* __fastcall Tiny_007E2BD0(uint8_t* self, re::Edx) {
    return self + 0x14;
}
RE_UNHOOKED_FN("NiMaterialResource::GetVariable", 0x007E2BD0, &Tiny_007E2BD0);       // lea eax, [ecx+0x14]
// 0x007E2BE0: [8b 41 20 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __fastcall Tiny_007E2BE0(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const uint32_t*>(self + 0x20);
}
RE_UNHOOKED_FN("NiMaterialResource::GetSource", 0x007E2BE0, &Tiny_007E2BE0);         // mov eax, [ecx+0x20] (shared)
// 0x007EA8D0: [8d 41 40 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static void* __fastcall Tiny_007EA8D0(uint8_t* self, re::Edx) {
    return self + 0x40;
}
RE_UNHOOKED_FN("NiMaterialFragmentNode::GetDescription", 0x007EA8D0, &Tiny_007EA8D0);   // lea eax, [ecx+0x40]

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIMATNODE_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIMATNODE_REVERSED("NiMaterialNodeLibrary::GetVersion", 0x007E2A50, ni::LibraryGetVersion);
NIMATNODE_REVERSED("NiMaterialNodeLibrary::GetAttachableNodeByName", 0x007E2A60, ni::LibraryGetAttachableNodeByName);
NIMATNODE_REVERSED("NiMaterialNodeLibrary::NiMaterialNodeLibrary", 0x007E2AC0, ni::LibraryConstruct);
NIMATNODE_REVERSED("NiTObjectArray<NiMaterialNodePtr>::DeletingDestructor", 0x007E2B20, ni::NodeArrayDeletingDestructor);
NIMATNODE_REVERSED("NiMaterialNodeLibrary::DeletingDestructor", 0x007E2B50, ni::LibraryDeletingDestructor);
NIMATNODE_REVERSED("NiGPUProgramDescriptor::GenerateKey", 0x007E2B90, ni::DescriptorGenerateKey);
NIMATNODE_REVERSED("NiMaterialResource::SetExtraData", 0x007E2BF0, ni::ResourceSetExtraData);
NIMATNODE_REVERSED("NiMaterialResource::SetObjectType", 0x007E2C00, ni::ResourceSetObjectType);
NIMATNODE_REVERSED("NiMaterialResource::NiMaterialResource", 0x007E2C10, ni::ResourceConstruct);
NIMATNODE_REVERSED("NiMaterialResource::~NiMaterialResource", 0x007E2C70, ni::ResourceDestruct);
NIMATNODE_REVERSED("NiMaterialResource::SetLabel", 0x007E2D60, ni::ResourceSetLabel);
NIMATNODE_REVERSED("NiMaterialResource::SetVariable", 0x007E2D70, ni::ResourceSetVariable);
NIMATNODE_REVERSED("NiMaterialResource::Clone", 0x007E2D80, ni::ResourceClone);
NIMATNODE_REVERSED("NiMaterialResource::DeletingDestructor", 0x007E2E00, ni::ResourceDeletingDestructor);
NIMATNODE_REVERSED("NiMaterialNode::ContainsResource", 0x007E2E20, ni::NodeContainsResource);
NIMATNODE_REVERSED("NiMaterialNode::GetInputResourceCount", 0x007E2E50, ni::NodeGetInputResourceCount);
NIMATNODE_REVERSED("NiMaterialNode::GetInputResource", 0x007E2E60, ni::NodeGetInputResource);
NIMATNODE_REVERSED("NiMaterialNode::GetInputResourceByVariableName", 0x007E2E70, ni::NodeGetInputResourceByVariableName);
NIMATNODE_REVERSED("NiMaterialNode::GetOutputResourceCount", 0x007E2EC0, ni::NodeGetOutputResourceCount);
NIMATNODE_REVERSED("NiMaterialNode::GetOutputResource", 0x007E2ED0, ni::NodeGetOutputResource);
NIMATNODE_REVERSED("NiMaterialNode::GetOutputResourceByVariableName", 0x007E2EE0, ni::NodeGetOutputResourceByVariableName);
NIMATNODE_REVERSED("NiMaterialNode::IsOutputResource", 0x007E2F30, ni::NodeIsOutputResource);
NIMATNODE_REVERSED("NiMaterialNode::IsInputResource", 0x007E2F70, ni::NodeIsInputResource);
NIMATNODE_REVERSED("NiMaterialNode::~NiMaterialNode", 0x007E2FB0, ni::NodeDestruct);
NIMATNODE_REVERSED("NiMaterialNode::GetRTTI", 0x007E3050, ni::NodeGetRTTI);
NIMATNODE_REVERSED("NiMaterialNode::NiMaterialNode", 0x007E3060, ni::NodeConstruct);
NIMATNODE_REVERSED("NiMaterialNode::DeletingDestructor", 0x007E30D0, ni::NodeDeletingDestructor);
NIMATNODE_REVERSED("NiMaterialNode::Clone", 0x007E30F0, ni::NodeClone);
NIMATNODE_REVERSED("NiMaterialNode::AddInputResourceObject", 0x007E3130, ni::NodeAddInputResourceObject);
NIMATNODE_REVERSED("NiMaterialNode::AddInputResource", 0x007E31A0, ni::NodeAddInputResource);
NIMATNODE_REVERSED("NiMaterialNode::AddOutputResource", 0x007E3280, ni::NodeAddOutputResource);
NIMATNODE_REVERSED("NiMaterialNode::AddOutputConstant", 0x007E3370, ni::NodeAddOutputConstant);
NIMATNODE_REVERSED("NiMaterialNode::AddOutputResourceObject", 0x007E3410, ni::NodeAddOutputResourceObject);
NIMATNODE_REVERSED("NiMaterialNode::CopyMembers", 0x007E3480, ni::NodeCopyMembers);
NIMATNODE_REVERSED("NiMaterialResourceConsumerNode::NiMaterialResourceConsumerNode", 0x007EA7D0, ni::ConsumerNodeConstruct);
NIMATNODE_REVERSED("NiMaterialResourceConsumerNode::GetRTTI", 0x007EA810, ni::ConsumerNodeGetRTTI);
NIMATNODE_REVERSED("NiMaterialResourceConsumerNode::DeletingDestructor", 0x007EA820, ni::ResourceNodeDeletingDestructor);
NIMATNODE_REVERSED("NiMaterialResourceProducerNode::NiMaterialResourceProducerNode", 0x007EA880, ni::ProducerNodeConstruct);
NIMATNODE_REVERSED("NiMaterialResourceProducerNode::GetRTTI", 0x007EA8C0, ni::ProducerNodeGetRTTI);
NIMATNODE_REVERSED("NiMaterialFragmentNode::SetDescription", 0x007EA8E0, ni::FragmentNodeSetDescription);
NIMATNODE_REVERSED("NiMaterialFragmentNode::GetCodeBlock", 0x007EA900, ni::FragmentNodeGetCodeBlock);
NIMATNODE_REVERSED("NiMaterialFragmentNode::AddCodeBlock", 0x007EAA50, ni::FragmentNodeAddCodeBlock);
NIMATNODE_REVERSED("NiMaterialFragmentNode::Clone", 0x007EAAC0, ni::FragmentNodeClone);
NIMATNODE_REVERSED("NiMaterialFragmentNode::CopyMembers", 0x007EAB30, ni::FragmentNodeCopyMembers);
NIMATNODE_REVERSED("NiCodeBlock::SetText", 0x007EABE0, ni::CodeBlockSetText);
NIMATNODE_REVERSED("NiCodeBlock::NiCodeBlock", 0x007EAC00, ni::CodeBlockConstruct);
NIMATNODE_REVERSED("NiCodeBlock::~NiCodeBlock", 0x007EAC50, ni::CodeBlockDestruct);
NIMATNODE_REVERSED("NiMaterialNode::SetName", 0x007EAD00, ni::NodeSetName);
NIMATNODE_REVERSED("NiCodeBlock::DeletingDestructor", 0x007EAD10, ni::CodeBlockDeletingDestructor);
