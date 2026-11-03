// The shader library's descriptions, on NiShaderDescBase (its code another chunk's: the
// constructor 0x7AB6B0, the destructor 0x7AB6F0):
//   NiShaderRequirementDesc (vtable 0x953FF0, 0x44 bytes): the shader versions it needs (each a
//     min/max pair), its flags and bone count;
//   NiShaderDesc (vtable 0x954D00, 0x6C bytes): that, plus the techniques (an NiTPrimitiveArray at
//     +0x48) and the attributes (an NiTPointerList of references at +0x5C);
//   NiShaderAttributeDesc (vtable 0x954D84, 0xE4 bytes): one attribute's type, flags, value and
//     range (an NiMalloc'd string or array, or up to 0x40 bytes in place).

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct NiShaderRequirementDesc {
    re::Ptr32<void> vtable;                      // +0x0
    uint8_t base[0xC];                 // +0x4 (NiShaderDescBase's)
    uint32_t minVertexShaderVersion;   // +0x10 (-1)
    uint32_t maxVertexShaderVersion;   // +0x14 (0)
    uint32_t minPixelShaderVersion;    // +0x18 (-1)
    uint32_t maxPixelShaderVersion;    // +0x1C (0)
    uint32_t minUserVersion;           // +0x20 (-1)
    uint32_t maxUserVersion;           // +0x24 (0)
    uint32_t minPlatformVersion;       // +0x28 (-1)
    uint32_t maxPlatformVersion;       // +0x2C (0)
    uint32_t flags;                    // +0x30 (bit 1, bit 3, bit 5: the three switches below)
    uint32_t unknown34;                // +0x34
    uint32_t unknown38;                // +0x38
    uint16_t bonesPerPartition;        // +0x3C (0xFFFF)
    uint8_t unknown3E;                 // +0x3E
    uint8_t unknown3F;                 // +0x3F
    uint8_t unknown40;                 // +0x40 (0)
    uint8_t pad41[3];
};
static_assert(offsetof(NiShaderRequirementDesc, minVertexShaderVersion) == 0x10);
static_assert(offsetof(NiShaderRequirementDesc, flags) == 0x30);
static_assert(offsetof(NiShaderRequirementDesc, bonesPerPartition) == 0x3C);
static_assert(sizeof(NiShaderRequirementDesc) == 0x44);

struct NiShaderDesc : NiShaderRequirementDesc {
    uint32_t techniqueCount;                 // +0x44
    NiTArray<NiObject*> techniques;          // +0x48 (NiTPrimitiveArray, vtable 0x961988)
    uint32_t unknown58;                      // +0x58
    NiTPointerList<NiObject*> attributes;    // +0x5C (vtable 0x95127C; references held)
};
static_assert(offsetof(NiShaderDesc, techniques) == 0x48);
static_assert(offsetof(NiShaderDesc, attributes) == 0x5C);
static_assert(sizeof(NiShaderDesc) == 0x6C);

struct NiShaderAttributeDesc {
    re::Ptr32<void> vtable;                // +0x0
    uint8_t base[0xC];           // +0x4 (NiShaderDescBase's)
    uint32_t type;               // +0x10 (1 bool, 2 string, 3 unsigned int, 4 float, 5 point2,
                                 //  6 point3, 7 point4, 8 matrix3, 9 matrix4, 10 color,
                                 //  11 texture, 14 array)
    uint32_t flags;              // +0x14 (bit 0, bit 1)
    uint8_t value[0x40];         // +0x18 (a string or array: its pointer first; an array's
                                 //  element type +0x1C and size +0x20; a texture's slot)
    uint32_t count;              // +0x58 (a string's buffer size, an array's element count)
    uint8_t low[0x40];           // +0x5C (the range; an array's: a pointer)
    uint8_t high[0x40];          // +0x9C
    re::Ptr32<char> textureMap;            // +0xDC (a texture's map name)
    uint32_t textureMapSize;     // +0xE0
};
static_assert(offsetof(NiShaderAttributeDesc, count) == 0x58);
static_assert(offsetof(NiShaderAttributeDesc, low) == 0x5C);
static_assert(offsetof(NiShaderAttributeDesc, high) == 0x9C);
static_assert(offsetof(NiShaderAttributeDesc, textureMap) == 0xDC);
static_assert(sizeof(NiShaderAttributeDesc) == 0xE4);

constexpr uintptr_t NiShaderRequirementDesc_vtable = 0x00953FF0;
constexpr uintptr_t NiShaderDesc_vtable = 0x00954D00;
constexpr uintptr_t NiShaderAttributeDesc_vtable = 0x00954D84;
constexpr uintptr_t PrimitiveArray_vtable = 0x00961988;
constexpr uintptr_t PointerList_vtable = 0x0095127C;
constexpr uintptr_t PointerListBase_vtable = 0x0099C748;

GAME_FN(void, __thiscall, ShaderDescBase_Construct, 0x007AB6B0, void* self);
GAME_FN(void, __thiscall, ShaderDescBase_Destruct, 0x007AB6F0, void* self);
GAME_FN(void, __thiscall, PointerList_RemoveAll, 0x0075DB60, void* list);
GAME_FN(void, __thiscall, ObjectArray_SetSize, 0x0084A110, void* array, uint32_t size);
GAME_FN(void, __thiscall, ObjectArray_SetAt, 0x00833C70, void* array, uint32_t index, const re::Ptr32<NiObject>* value);
// (this file's, called where modes apply)
GAME_FN(NiShaderRequirementDesc*, __thiscall, RequirementDesc_Construct, 0x0077E870, void* self);
GAME_FN(void, __thiscall, RequirementDesc_Destruct, 0x0077E8C0, void* self);
GAME_FN(void, __thiscall, ShaderDesc_Destruct, 0x007842E0, NiShaderDesc* self);
GAME_FN(void, __thiscall, AttributeList_AddTail, 0x007843D0, NiTPointerList<NiObject*>* self, const re::Ptr32<NiObject>* element);
GAME_FN(void, __thiscall, AttributeDesc_Destruct, 0x007845C0, NiShaderAttributeDesc* self);

// ---- NiShaderRequirementDesc

NiShaderRequirementDesc* __fastcall RequirementConstruct(NiShaderRequirementDesc* self, re::Edx) {
    ShaderDescBase_Construct(self);
    self->maxVertexShaderVersion = 0;
    self->maxPixelShaderVersion = 0;
    self->maxUserVersion = 0;
    self->maxPlatformVersion = 0;
    self->flags = 0;
    self->unknown34 = 0;
    self->unknown38 = 0;
    self->unknown40 = 0;
    self->vtable = reinterpret_cast<void*>(NiShaderRequirementDesc_vtable);
    self->minVertexShaderVersion = 0xFFFFFFFF;
    self->minPixelShaderVersion = 0xFFFFFFFF;
    self->minUserVersion = 0xFFFFFFFF;
    self->minPlatformVersion = 0xFFFFFFFF;
    self->bonesPerPartition = 0xFFFF;
    return self;
}

void __fastcall RequirementDestruct(NiShaderRequirementDesc* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiShaderRequirementDesc_vtable);
    ShaderDescBase_Destruct(self);
}

// the getters (4 bytes each but the flag ones: unhooked)
uint32_t __fastcall GetMinVertexShaderVersion(NiShaderRequirementDesc* self, re::Edx) {
    return self->minVertexShaderVersion;
}
uint32_t __fastcall GetMaxPixelShaderVersion(NiShaderRequirementDesc* self, re::Edx) {
    return self->maxPixelShaderVersion;
}
uint32_t __fastcall GetMinPlatformVersion(NiShaderRequirementDesc* self, re::Edx) {
    return self->minPlatformVersion;
}
uint8_t __fastcall GetUnknown3E(NiShaderRequirementDesc* self, re::Edx) {
    return self->unknown3E;
}
uint8_t __fastcall GetUnknown3F(NiShaderRequirementDesc* self, re::Edx) {
    return self->unknown3F;
}

uint8_t __fastcall GetFlag1(NiShaderRequirementDesc* self, re::Edx) {
    return uint8_t(self->flags >> 1) & 1;
}
uint8_t __fastcall GetFlag5(NiShaderRequirementDesc* self, re::Edx) {
    return uint8_t(self->flags >> 5) & 1;
}
uint8_t __fastcall GetFlag3(NiShaderRequirementDesc* self, re::Edx) {
    return uint8_t(self->flags >> 3) & 1;
}

// a version added to a min/max pair
inline void AddVersion(uint32_t& low, uint32_t& high, uint32_t version) {
    if (version < low) {
        low = version;
    }
    if (high < version) {
        high = version;
    }
}

void __fastcall AddVertexShaderVersion(NiShaderRequirementDesc* self, re::Edx, uint32_t version) {
    AddVersion(self->minVertexShaderVersion, self->maxVertexShaderVersion, version);
}
void __fastcall AddPixelShaderVersion(NiShaderRequirementDesc* self, re::Edx, uint32_t version) {
    AddVersion(self->minPixelShaderVersion, self->maxPixelShaderVersion, version);
}
void __fastcall AddUserVersion(NiShaderRequirementDesc* self, re::Edx, uint32_t version) {
    AddVersion(self->minUserVersion, self->maxUserVersion, version);
}
void __fastcall AddPlatformVersion(NiShaderRequirementDesc* self, re::Edx, uint32_t version) {
    AddVersion(self->minPlatformVersion, self->maxPlatformVersion, version);
}

void __fastcall SetFlags(NiShaderRequirementDesc* self, re::Edx, uint32_t flags) {
    self->flags |= flags;
}
void __fastcall ClearFlags(NiShaderRequirementDesc* self, re::Edx, uint32_t flags) {
    self->flags &= ~flags;
}

inline void SetBit(uint32_t& flags, uint32_t bit, int on) {
    if (uint8_t(on) != 0) {
        flags |= bit;
    } else {
        flags &= ~bit;
    }
}
void __fastcall SetFlag1(NiShaderRequirementDesc* self, re::Edx, int on) {
    SetBit(self->flags, 2, on);
}
void __fastcall SetFlag5(NiShaderRequirementDesc* self, re::Edx, int on) {
    SetBit(self->flags, 0x20, on);
}
void __fastcall SetFlag3(NiShaderRequirementDesc* self, re::Edx, int on) {
    SetBit(self->flags, 8, on);
}

void __fastcall SetUnknown40(NiShaderRequirementDesc* self, re::Edx, int value) {
    self->unknown40 = uint8_t(value);
}
void __fastcall SetBonesPerPartition(NiShaderRequirementDesc* self, re::Edx, int count) {
    self->bonesPerPartition = uint16_t(count);
}
void __fastcall SetUnknown3E(NiShaderRequirementDesc* self, re::Edx, int value) {
    self->unknown3E = uint8_t(value);
}
void __fastcall SetUnknown3F(NiShaderRequirementDesc* self, re::Edx, int value) {
    self->unknown3F = uint8_t(value);
}

NiShaderRequirementDesc* __fastcall RequirementDeletingDestructor(NiShaderRequirementDesc* self, re::Edx,
                                                                 uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiShaderRequirementDesc_vtable);
    ShaderDescBase_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x44);
    }
    return self;
}

// ---- NiShaderDesc

void __fastcall SetTechniqueCount(NiShaderDesc* self, re::Edx, uint32_t count) {
    self->techniqueCount = count;
}

// a list's nodes freed (its allocator's slot 2), the list emptied
inline void RemoveAllInline(NiTPointerList<NiObject*>* list) {
    for (NiTListNode<NiObject*>* node = list->head; node != nullptr;) {
        NiTListNode<NiObject*>* next = node->next;
        Virtual<void>(list, 2, node);
        node = next;
    }
    list->count = 0;
    list->head = nullptr;
    list->tail = nullptr;
}

// the destructor: the techniques deleted, the attributes' list emptied
void __fastcall ShaderDescDestruct(NiShaderDesc* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiShaderDesc_vtable);
    for (uint32_t i = 0; i < self->techniques.size; i++) {
        NiObject* technique = self->techniques.base[i];
        if (technique != nullptr) {
            re::Ptr32<NiObject> none = nullptr;
            ObjectArray_SetAt(&self->techniques, i, &none);
            Virtual<void*>(technique, 0, 1u);
        }
    }
    for (uint16_t i = 0; i < self->techniques.size; i++) {
        self->techniques.base[i] = nullptr;
    }
    self->techniques.size = 0;
    self->techniques.esize = 0;
    RemoveAllInline(&self->attributes);
    self->attributes.vtable = reinterpret_cast<void*>(PointerList_vtable);
    RemoveAllInline(&self->attributes);
    self->attributes.vtable = reinterpret_cast<void*>(PointerListBase_vtable);
    self->techniques.vtable = reinterpret_cast<void*>(PrimitiveArray_vtable);
    NiFree(self->techniques.base);
    RequirementDesc_Destruct(self);
}

// NiTPointerList<NiPointer<...>>::AddTail (the attributes' list here; the same code serves every
// list of references): a node from the allocator (slot 1), the element held
void __fastcall AttributeListAddTail(NiTPointerList<NiObject*>* self, re::Edx, const re::Ptr32<NiObject>* element) {
    NiTListNode<NiObject*>* node = Virtual<NiTListNode<NiObject*>*>(self, 1);
    NiObject* old = node->element;
    if (old != *element) {
        if (old != nullptr && ni::sys::Decrement(&old->refCount) == 0) {
            Virtual<void>(old, 1);
        }
        node->element = *element;
        if (*element != nullptr) {
            ni::sys::Increment(&(*element)->refCount);
        }
    }
    node->next = nullptr;
    node->prev = self->tail;
    if (self->tail != nullptr) {
        self->tail->next = node;
        self->count++;
        self->tail = node;
        return;
    }
    self->count++;
    self->head = node;
    self->tail = node;
}

NiShaderDesc* __fastcall ShaderDescDeletingDestructor(NiShaderDesc* self, re::Edx, uint32_t flags) {
    ShaderDesc_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x6C);
    }
    return self;
}

// AddAttribute(attribute): held while it goes on the list
void __fastcall AddAttribute(NiShaderDesc* self, re::Edx, NiObject* attribute) {
    if (attribute == nullptr) {
        return;
    }
    ni::sys::Increment(&attribute->refCount);
    re::Ptr32<NiObject> held = attribute;
    AttributeList_AddTail(&self->attributes, &held);
    if (ni::sys::Decrement(&attribute->refCount) == 0) {
        Virtual<void>(attribute, 1);
    }
}

NiShaderDesc* __fastcall ShaderDescConstruct(NiShaderDesc* self, re::Edx) {
    RequirementDesc_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiShaderDesc_vtable);
    self->techniqueCount = 0;
    self->techniques.maxSize = 0;
    self->techniques.growBy = 1;
    self->techniques.size = 0;
    self->techniques.esize = 0;
    self->techniques.base = nullptr;
    self->techniques.vtable = reinterpret_cast<void*>(PrimitiveArray_vtable);
    self->attributes.count = 0;
    self->attributes.head = nullptr;
    self->attributes.tail = nullptr;
    self->attributes.vtable = reinterpret_cast<void*>(PointerList_vtable);
    for (uint16_t i = 0; i < self->techniques.size; i++) {
        self->techniques.base[i] = nullptr;
    }
    self->techniques.size = 0;
    self->techniques.esize = 0;
    PointerList_RemoveAll(&self->attributes);
    return self;
}

// SetTechnique(index, technique): the array grown as needed
void __fastcall SetTechnique(NiShaderDesc* self, re::Edx, uint32_t index, re::Ptr32<NiObject> technique) {
    NiTArray<NiObject*>* array = &self->techniques;
    if (index >= array->maxSize) {
        ObjectArray_SetSize(array, array->growBy + index);
    }
    ObjectArray_SetAt(array, index, &technique);
}

// ---- NiShaderAttributeDesc

template <typename T>
inline re::Field<T>& Value(NiShaderAttributeDesc* self, int offset) {
    return *reinterpret_cast<re::Field<T>*>(reinterpret_cast<uint8_t*>(self) + offset);
}

// the allocations of the current type let go before the type changes (strings and arrays)
inline void FreeForType(NiShaderAttributeDesc* self) {
    if (self->type == 2) {
        NiFree(Value<void*>(self, 0x18));
        self->count = 0;
    } else if (self->type == 0xE) {
        NiFree(Value<void*>(self, 0x18));
        NiFree(Value<void*>(self, 0x9C));
        NiFree(Value<void*>(self, 0x5C));
        self->count = 0;
    }
}

NiShaderAttributeDesc* __fastcall AttributeConstruct(NiShaderAttributeDesc* self, re::Edx) {
    ShaderDescBase_Construct(self);
    self->type = 0;
    self->flags = 0;
    Value<uint32_t>(self, 0x18) = 0;
    self->count = 0;
    Value<uint32_t>(self, 0x5C) = 0;
    Value<uint32_t>(self, 0x9C) = 0;
    self->textureMap = nullptr;
    self->textureMapSize = 0;
    self->vtable = reinterpret_cast<void*>(NiShaderAttributeDesc_vtable);
    return self;
}

void __fastcall AttributeDestruct(NiShaderAttributeDesc* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiShaderAttributeDesc_vtable);
    if (self->type == 2) {
        NiFree(Value<void*>(self, 0x18));
    } else if (self->type == 0xE) {
        NiFree(Value<void*>(self, 0x18));
        NiFree(Value<void*>(self, 0x9C));
        NiFree(Value<void*>(self, 0x5C));
    }
    NiFree(self->textureMap);
    ShaderDescBase_Destruct(self);
}

// SetType(type): the old type's allocations let go, the new one's pointers cleared
void __fastcall SetType(NiShaderAttributeDesc* self, re::Edx, uint32_t type) {
    if (self->type == type) {
        return;
    }
    FreeForType(self);
    self->type = type;
    if (type == 2) {
        self->count = 0;
        Value<uint32_t>(self, 0x18) = 0;
    } else if (type == 0xE) {
        Value<uint32_t>(self, 0x9C) = 0;
        Value<uint32_t>(self, 0x5C) = 0;
        self->count = 0;
        Value<uint32_t>(self, 0x18) = 0;
    }
}

// the value setters: the type changed (as SetType's first half) unless it's already that
inline void BecomeType(NiShaderAttributeDesc* self, uint32_t type) {
    if (self->type != type) {
        FreeForType(self);
        self->type = type;
    }
}

void __fastcall SetValueBool(NiShaderAttributeDesc* self, re::Edx, int value) {
    BecomeType(self, 1);
    Value<uint8_t>(self, 0x18) = uint8_t(value);
}

void __fastcall SetValueUnsignedInt(NiShaderAttributeDesc* self, re::Edx, uint32_t value) {
    BecomeType(self, 3);
    Value<uint32_t>(self, 0x18) = value;
}

void __fastcall SetValueFloat(NiShaderAttributeDesc* self, re::Edx, uint32_t value) {
    BecomeType(self, 4);
    Value<uint32_t>(self, 0x18) = value;   // (copied as bits)
}

void __fastcall SetValuePoint2(NiShaderAttributeDesc* self, re::Edx, const uint32_t* value) {
    BecomeType(self, 5);
    Value<uint32_t>(self, 0x18) = value[0];
    Value<uint32_t>(self, 0x1C) = value[1];
}

void __fastcall SetValuePoint3(NiShaderAttributeDesc* self, re::Edx, const uint32_t* value) {
    BecomeType(self, 6);
    Value<uint32_t>(self, 0x18) = value[0];
    Value<uint32_t>(self, 0x1C) = value[1];
    Value<uint32_t>(self, 0x20) = value[2];
}

// a color (alpha 1)
void __fastcall SetValueColor(NiShaderAttributeDesc* self, re::Edx, const uint32_t* value) {
    BecomeType(self, 10);
    Value<uint32_t>(self, 0x18) = value[0];
    Value<uint32_t>(self, 0x1C) = value[1];
    Value<uint32_t>(self, 0x20) = value[2];
    Value<float>(self, 0x24) = 1.0f;
}

void __fastcall SetValueColorA(NiShaderAttributeDesc* self, re::Edx, const uint32_t* value) {
    BecomeType(self, 10);
    Value<uint32_t>(self, 0x18) = value[0];
    Value<uint32_t>(self, 0x1C) = value[1];
    Value<uint32_t>(self, 0x20) = value[2];
    Value<uint32_t>(self, 0x24) = value[3];
}

inline void SetFlagBit(NiShaderAttributeDesc* self, uint32_t bit, int on) {
    if (uint8_t(on) != 0) {
        self->flags |= bit;
    } else {
        self->flags &= ~bit;
    }
}
void __fastcall AttributeSetFlag1(NiShaderAttributeDesc* self, re::Edx, int on) {
    SetFlagBit(self, 2, on);
}
void __fastcall AttributeSetFlag0(NiShaderAttributeDesc* self, re::Edx, int on) {
    SetFlagBit(self, 1, on);
}

// the range setters: only for their type (the unsigned int one takes strings too, sic)
uint8_t __fastcall SetRangeUnsignedInt(NiShaderAttributeDesc* self, re::Edx, uint32_t low, uint32_t high) {
    if (self->type != 3 && self->type != 2) {
        return 0;
    }
    Value<uint32_t>(self, 0x5C) = low;
    Value<uint32_t>(self, 0x9C) = high;
    return 1;
}

uint8_t __fastcall SetRangeFloat(NiShaderAttributeDesc* self, re::Edx, uint32_t low, uint32_t high) {
    if (self->type != 4) {
        return 0;
    }
    Value<uint32_t>(self, 0x5C) = low;
    Value<uint32_t>(self, 0x9C) = high;
    return 1;
}

inline uint8_t SetRange(NiShaderAttributeDesc* self, uint32_t type, const uint32_t* low, const uint32_t* high,
                        int components) {
    if (self->type != type) {
        return 0;
    }
    for (int i = 0; i < components; i++) {
        Value<uint32_t>(self, 0x5C + i * 4) = low[i];
    }
    for (int i = 0; i < components; i++) {
        Value<uint32_t>(self, 0x9C + i * 4) = high[i];
    }
    return 1;
}

uint8_t __fastcall SetRangePoint2(NiShaderAttributeDesc* self, re::Edx, const uint32_t* low, const uint32_t* high) {
    return SetRange(self, 5, low, high, 2);
}
uint8_t __fastcall SetRangePoint3(NiShaderAttributeDesc* self, re::Edx, const uint32_t* low, const uint32_t* high) {
    return SetRange(self, 6, low, high, 3);
}
uint8_t __fastcall SetRangePoint4(NiShaderAttributeDesc* self, re::Edx, const uint32_t* low, const uint32_t* high) {
    return SetRange(self, 7, low, high, 4);
}

// a color range (alphas 1)
uint8_t __fastcall SetRangeColor(NiShaderAttributeDesc* self, re::Edx, const uint32_t* low, const uint32_t* high) {
    if (self->type != 10) {
        return 0;
    }
    Value<uint32_t>(self, 0x5C) = low[0];
    Value<uint32_t>(self, 0x60) = low[1];
    Value<uint32_t>(self, 0x64) = low[2];
    Value<float>(self, 0x68) = 1.0f;
    Value<uint32_t>(self, 0x9C) = high[0];
    Value<uint32_t>(self, 0xA0) = high[1];
    Value<uint32_t>(self, 0xA4) = high[2];
    Value<float>(self, 0xA8) = 1.0f;
    return 1;
}

uint8_t __fastcall SetRangeColorA(NiShaderAttributeDesc* self, re::Edx, const uint32_t* low, const uint32_t* high) {
    return SetRange(self, 10, low, high, 4);
}

// the object types' names (the game's literals)
const char* __cdecl GetObjectTypeName(uint32_t type) {
    switch (type) {
    case 1:
        return reinterpret_cast<const char*>(0x00950D80);   // "GeneralLight"
    case 2:
        return reinterpret_cast<const char*>(0x00950D74);   // "PointLight"
    case 3:
        return reinterpret_cast<const char*>(0x00950D60);   // "DirectionalLight"
    case 4:
        return reinterpret_cast<const char*>(0x00950D54);   // "SpotLight"
    case 8:
        return reinterpret_cast<const char*>(0x00950D08);   // "EnvironmentMap"
    case 9:
        return reinterpret_cast<const char*>(0x00950CF4);   // "ProjectedShadowMap"
    case 10:
        return reinterpret_cast<const char*>(0x00950CE0);   // "ProjectedLightMap"
    case 11:
        return reinterpret_cast<const char*>(0x00950CD8);   // "FogMap"
    default:
        return nullptr;
    }
}

// an attribute type's value size
uint32_t __cdecl GetAttributeTypeSize(uint32_t type) {
    switch (type) {
    case 1:
        return 1;
    case 3:
    case 4:
        return 4;
    case 5:
        return 8;
    case 6:
        return 0xC;
    case 7:
    case 10:
        return 0x10;
    case 8:
        return 0x24;
    case 9:
        return 0x40;
    default:
        return 0;
    }
}

NiShaderAttributeDesc* __fastcall AttributeDeletingDestructor(NiShaderAttributeDesc* self, re::Edx, uint32_t flags) {
    AttributeDesc_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0xE4);
    }
    return self;
}

// a string kept in an NiMalloc'd buffer, reused while big enough (none for null or "")
inline void CopyString(re::Ptr32<char>& buffer, uint32_t& size, const char* text) {
    if (text == nullptr || *text == 0) {
        NiFree(buffer);
        buffer = nullptr;
        size = 0;
        return;
    }
    if (buffer != nullptr && size <= std::strlen(text)) {
        NiFree(buffer);
        buffer = nullptr;
        size = 0;
    }
    if (buffer == nullptr) {
        size = uint32_t(std::strlen(text)) + 1;
        buffer = static_cast<char*>(NiMalloc(size));
    }
    crt::StrcpyS(buffer, size, text);
}

// SetValue_String(text): a string (an array's allocations let go)
void __fastcall SetValueString(NiShaderAttributeDesc* self, re::Edx, const char* text) {
    if (self->type != 2) {
        if (self->type == 0xE) {
            NiFree(Value<void*>(self, 0x18));
            NiFree(Value<void*>(self, 0x9C));
            NiFree(Value<void*>(self, 0x5C));
            self->count = 0;
        }
        self->type = 2;
        Value<char*>(self, 0x18) = nullptr;
        self->count = 0;
    }
    CopyString(Value<char*>(self, 0x18), self->count, text);
}

// a point4 (0x10 bytes) and matrix4 (0x40 bytes) copied into the value
void __fastcall SetValuePoint4(NiShaderAttributeDesc* self, re::Edx, const void* value) {
    BecomeType(self, 7);
    crt::MemcpyS(self->value, 0x40, value, 0x10);
}

// a matrix3, its rows padded to four
void __fastcall SetValueMatrix3(NiShaderAttributeDesc* self, re::Edx, const uint32_t* value) {
    BecomeType(self, 8);
    for (int row = 0; row < 3; row++) {
        for (int column = 0; column < 3; column++) {
            Value<uint32_t>(self, 0x18 + row * 0x10 + column * 4) = value[row * 3 + column];
        }
    }
}

void __fastcall SetValueMatrix4(NiShaderAttributeDesc* self, re::Edx, const void* value) {
    BecomeType(self, 9);
    crt::MemcpyS(self->value, 0x40, value, 0x40);
}

// SetValue_Texture(slot, map): the map's name kept
void __fastcall SetValueTexture(NiShaderAttributeDesc* self, re::Edx, uint32_t slot, const char* map) {
    BecomeType(self, 0xB);
    CopyString(self->textureMap, self->textureMapSize, map);
    Value<uint32_t>(self, 0x18) = slot;
}

// SetValue_Array(data, elementType, elementSize, count): copied into a new buffer (the old one
// freed only when the new data is bigger: a leak otherwise, as the game has it)
void __fastcall SetValueArray(NiShaderAttributeDesc* self, re::Edx, const void* data, uint32_t elementType,
                              uint32_t elementSize, uint32_t count) {
    if (self->type != 0xE) {
        if (self->type == 2) {
            NiFree(Value<void*>(self, 0x18));
            self->count = 0;
        }
        self->type = 0xE;
        Value<uint32_t>(self, 0x18) = 0;
        Value<uint32_t>(self, 0x9C) = 0;
        Value<uint32_t>(self, 0x5C) = 0;
        self->count = 0;
    }
    uint32_t size = elementSize * count;
    void* old = Value<void*>(self, 0x18);
    if (old != nullptr && size > Value<uint32_t>(self, 0x20) * self->count) {
        NiFree(old);
        self->count = 0;
    }
    void* copy = NiMalloc(size);
    Value<void*>(self, 0x18) = copy;
    crt::MemcpyS(copy, size, data, size);
    Value<uint32_t>(self, 0x20) = elementSize;
    self->count = count;
    Value<uint32_t>(self, 0x1C) = elementType;
}

} // namespace

// the four-byte getters (unhooked; the first three's code is shared with other classes')
// 0x0077E8D0: [8b 41 10 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __fastcall Tiny_0077E8D0(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const uint32_t*>(self + 0x10);
}
RE_UNHOOKED_FN("NiShaderRequirementDesc::GetMinVertexShaderVersion", 0x0077E8D0, &Tiny_0077E8D0);
// 0x0077E8E0: [8b 41 1c c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __fastcall Tiny_0077E8E0(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const uint32_t*>(self + 0x1C);
}
RE_UNHOOKED_FN("NiShaderRequirementDesc::GetMaxPixelShaderVersion", 0x0077E8E0, &Tiny_0077E8E0);
// 0x0077E8F0: [8b 41 28 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __fastcall Tiny_0077E8F0(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const uint32_t*>(self + 0x28);
}
RE_UNHOOKED_FN("NiShaderRequirementDesc::GetMinPlatformVersion", 0x0077E8F0, &Tiny_0077E8F0);
// 0x0077E930: [8a 41 3e c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __fastcall Tiny_0077E930(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const uint8_t*>(self + 0x3E);
}
RE_UNHOOKED_FN("NiShaderRequirementDesc::GetUnknown3E", 0x0077E930, &Tiny_0077E930);
// 0x0077E940: [8a 41 3f c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __fastcall Tiny_0077E940(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const uint8_t*>(self + 0x3F);
}
RE_UNHOOKED_FN("NiShaderRequirementDesc::GetUnknown3F", 0x0077E940, &Tiny_0077E940);

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISHADERDESC_REVERSED(name, address, fn)                                               \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISHADERDESC_REVERSED("NiShaderRequirementDesc::NiShaderRequirementDesc", 0x0077E870, &ni::RequirementConstruct);
NISHADERDESC_REVERSED("NiShaderRequirementDesc::~NiShaderRequirementDesc", 0x0077E8C0, &ni::RequirementDestruct);
NISHADERDESC_REVERSED("NiShaderRequirementDesc::GetFlag1", 0x0077E900, &ni::GetFlag1);
NISHADERDESC_REVERSED("NiShaderRequirementDesc::GetFlag5", 0x0077E910, &ni::GetFlag5);
NISHADERDESC_REVERSED("NiShaderRequirementDesc::GetFlag3", 0x0077E920, &ni::GetFlag3);
NISHADERDESC_REVERSED("NiShaderRequirementDesc::AddVertexShaderVersion", 0x0077E950, &ni::AddVertexShaderVersion);
NISHADERDESC_REVERSED("NiShaderRequirementDesc::AddPixelShaderVersion", 0x0077E970, &ni::AddPixelShaderVersion);
NISHADERDESC_REVERSED("NiShaderRequirementDesc::AddUserVersion", 0x0077E990, &ni::AddUserVersion);
NISHADERDESC_REVERSED("NiShaderRequirementDesc::AddPlatformVersion", 0x0077E9B0, &ni::AddPlatformVersion);
NISHADERDESC_REVERSED("NiShaderRequirementDesc::SetFlags", 0x0077E9D0, &ni::SetFlags);
NISHADERDESC_REVERSED("NiShaderRequirementDesc::ClearFlags", 0x0077E9E0, &ni::ClearFlags);
NISHADERDESC_REVERSED("NiShaderRequirementDesc::SetFlag1", 0x0077E9F0, &ni::SetFlag1);
NISHADERDESC_REVERSED("NiShaderRequirementDesc::SetFlag5", 0x0077EA10, &ni::SetFlag5);
NISHADERDESC_REVERSED("NiShaderRequirementDesc::SetFlag3", 0x0077EA30, &ni::SetFlag3);
NISHADERDESC_REVERSED("NiShaderRequirementDesc::SetUnknown40", 0x0077EA50, &ni::SetUnknown40);
NISHADERDESC_REVERSED("NiShaderRequirementDesc::SetBonesPerPartition", 0x0077EA60, &ni::SetBonesPerPartition);
NISHADERDESC_REVERSED("NiShaderRequirementDesc::SetUnknown3E", 0x0077EA70, &ni::SetUnknown3E);
NISHADERDESC_REVERSED("NiShaderRequirementDesc::SetUnknown3F", 0x0077EA80, &ni::SetUnknown3F);
NISHADERDESC_REVERSED("NiShaderRequirementDesc::DeletingDestructor", 0x0077EA90, &ni::RequirementDeletingDestructor);
NISHADERDESC_REVERSED("NiShaderDesc::SetTechniqueCount", 0x007842D0, &ni::SetTechniqueCount);
NISHADERDESC_REVERSED("NiShaderDesc::~NiShaderDesc", 0x007842E0, &ni::ShaderDescDestruct);
// (NiTPointerList<NiPointer<...>>::AddTail: one copy shared by every such list in the exe)
NISHADERDESC_REVERSED("NiTPointerList::AddTail(NiPointer)", 0x007843D0, &ni::AttributeListAddTail);
NISHADERDESC_REVERSED("NiShaderDesc::DeletingDestructor", 0x00784450, &ni::ShaderDescDeletingDestructor);
NISHADERDESC_REVERSED("NiShaderDesc::AddAttribute", 0x00784470, &ni::AddAttribute);
NISHADERDESC_REVERSED("NiShaderDesc::NiShaderDesc", 0x007844C0, &ni::ShaderDescConstruct);
NISHADERDESC_REVERSED("NiShaderDesc::SetTechnique", 0x00784540, &ni::SetTechnique);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::NiShaderAttributeDesc", 0x00784580, &ni::AttributeConstruct);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::~NiShaderAttributeDesc", 0x007845C0, &ni::AttributeDestruct);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::SetType", 0x00784620, &ni::SetType);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::SetValue_Bool", 0x00784690, &ni::SetValueBool);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::SetValue_UnsignedInt", 0x00784700, &ni::SetValueUnsignedInt);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::SetValue_Float", 0x00784770, &ni::SetValueFloat);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::SetValue_Point2", 0x007847D0, &ni::SetValuePoint2);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::SetValue_Point3", 0x00784840, &ni::SetValuePoint3);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::SetValue_Color", 0x007848B0, &ni::SetValueColor);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::SetValue_ColorA", 0x00784920, &ni::SetValueColorA);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::SetFlag1", 0x007849A0, &ni::AttributeSetFlag1);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::SetFlag0", 0x007849C0, &ni::AttributeSetFlag0);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::SetRange_UnsignedInt", 0x007849E0, &ni::SetRangeUnsignedInt);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::SetRange_Float", 0x00784A10, &ni::SetRangeFloat);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::SetRange_Point2", 0x00784A40, &ni::SetRangePoint2);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::SetRange_Point3", 0x00784A80, &ni::SetRangePoint3);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::SetRange_Point4", 0x00784AD0, &ni::SetRangePoint4);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::SetRange_Color", 0x00784B30, &ni::SetRangeColor);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::SetRange_ColorA", 0x00784B80, &ni::SetRangeColorA);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::GetObjectTypeName", 0x00784BE0, &ni::GetObjectTypeName);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::GetAttributeTypeSize", 0x00784C60, &ni::GetAttributeTypeSize);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::DeletingDestructor", 0x00784CD0, &ni::AttributeDeletingDestructor);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::SetValue_String", 0x00784D00, &ni::SetValueString);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::SetValue_Point4", 0x00784DE0, &ni::SetValuePoint4);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::SetValue_Matrix3", 0x00784E50, &ni::SetValueMatrix3);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::SetValue_Matrix4", 0x00784EE0, &ni::SetValueMatrix4);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::SetValue_Texture", 0x00784F50, &ni::SetValueTexture);
NISHADERDESC_REVERSED("NiShaderAttributeDesc::SetValue_Array", 0x00785070, &ni::SetValueArray);
