// NiGeneralComponent (vtables 0x98A6C4 / 0x98A598 at +8, 0x30 bytes): the component whose
// properties are data, added at run time (the GSA files' "NiGeneralComponent"s): a named set of
// NiGeneralComponentProperty objects, and a master component it inherits from (a property only
// the master has is copied in when it's changed: "made unique"). Its interface methods get the
// interface pointer (+8). The constructors with no arguments and the destructor are the other
// pass's (0x83DAB0, 0x83DB20, 0x83DBB0); GetName (0x856A30) is too.
//
// NiGeneralComponentProperty (vtable 0x98AEB4, 0x40 bytes): the names, the collection flag and
// the data: one NiMalloc'd block per element and its size, in two NiTPrimitiveArrays.
//
// Also NiEntityPropertyInterface's static names (the primitive types), NiTObjectSet<NiFixedString>
// (Realloc, Add) and the property-data helpers the other components share (folded:
// GetTemplateID / SetTemplateID / GetMasterComponent / the pointer getters and setters).

#include <cstddef>
#include <cstdint>
#include <intrin.h>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiEntityComponentCommon.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni::ec {
namespace {

// ---- the layouts

struct GeneralProperty : NiRefObject {
    re::Ptr32<const char> name;            // +0x8
    re::Ptr32<const char> displayName;     // +0xC
    re::Ptr32<const char> primitiveType;   // +0x10
    re::Ptr32<const char> semanticType;    // +0x14
    re::Ptr32<const char> description;     // +0x18
    uint8_t collection;            // +0x1C
    uint8_t pad1D[3];
    NiTArray<void*> data;          // +0x20 (NiTPrimitiveArray<void*>, vtable 0x961988)
    NiTArray<uint32_t> sizes;      // +0x30 (the same: each block's size in bytes)
};
static_assert(offsetof(GeneralProperty, collection) == 0x1C);
static_assert(offsetof(GeneralProperty, data) == 0x20);
static_assert(offsetof(GeneralProperty, sizes) == 0x30);
static_assert(sizeof(GeneralProperty) == 0x40);

struct GeneralComponent : NiRefObject {
    re::Ptr32<void> ifaceVtable;                     // +0x8
    re::Ptr32<const char> name;                    // +0xC
    NiTSet<GeneralProperty*> properties;   // +0x10 (NiTObjectSet<NiGeneralComponentPropertyPtr>)
    re::Ptr32<GeneralComponent> master;              // +0x1C (a reference)
    NiUniqueID templateID;                 // +0x20
};
static_assert(offsetof(GeneralComponent, properties) == 0x10);
static_assert(offsetof(GeneralComponent, master) == 0x1C);
static_assert(offsetof(GeneralComponent, templateID) == 0x20);
static_assert(sizeof(GeneralComponent) == 0x30);

constexpr uintptr_t GeneralComponent_vtable = 0x0098A6C4;
constexpr uintptr_t GeneralComponentIface_vtable = 0x0098A598;
constexpr uintptr_t GeneralProperty_vtable = 0x0098AEB4;

GAME_VAR(NiFixedString, NiGeneralComponent_ms_kClassName, 0x020E5C60);
// NiPoint2::ZERO, NiQuaternion::IDENTITY, NiColor::BLACK, NiColorA::WHITE (as AddProperty uses
// them for the defaults; the last two by their place, a guess)
constexpr uintptr_t Point2_ZERO = 0x020E3940;
constexpr uintptr_t Quaternion_IDENTITY = 0x00BC8A80;
constexpr uintptr_t Color_DEFAULT = 0x020E25E8;
constexpr uintptr_t ColorA_DEFAULT = 0x00BC75D4;

// ---- callees

GAME_FN(GeneralProperty*, __thiscall, Component_GetPropertyByName, 0x008468A0, GeneralComponent* self,
        const re::Ptr32<const char>* name);
GAME_FN(uint32_t, __thiscall, Component_GetPropertyIndex, 0x008468D0, GeneralComponent* self, const re::Ptr32<const char>* name);
GAME_FN(void, __thiscall, Property_SetData32, 0x00847940, GeneralProperty* self, const void* data, uint32_t count,
        uint32_t index);
GAME_FN(void, __thiscall, Property_SetData, 0x00847A00, GeneralProperty* self, const void* data, uint32_t size,
        uint32_t index);
GAME_FN(void, __thiscall, Property_SetData16, 0x00847AC0, GeneralProperty* self, const void* data, uint32_t count,
        uint32_t index);
GAME_FN(GeneralProperty*, __thiscall, Property_Construct, 0x00847B80, void* self, const re::Ptr32<const char>* name,
        const re::Ptr32<const char>* displayName, const re::Ptr32<const char>* primitiveType, const re::Ptr32<const char>* semanticType,
        const re::Ptr32<const char>* description, int collection);
GAME_FN(void, __thiscall, Property_SetElementCount, 0x00847C90, GeneralProperty* self, uint32_t count);
GAME_FN(GeneralProperty*, __thiscall, Property_Clone, 0x00847D10, GeneralProperty* self);
GAME_FN(void, __thiscall, Property_Destruct, 0x00856C30, GeneralProperty* self);   // (the other pass's)
GAME_FN(GeneralComponent*, __thiscall, Component_Construct, 0x00848DA0, void* self, const re::Ptr32<const char>* name,
        const NiUniqueID* templateID, uint32_t propertyCount, GeneralComponent* master);
// NiTObjectSet<NiGeneralComponentPropertyPtr>
GAME_FN(void, __thiscall, PropertySet_Add, 0x00787BE0, NiTSet<GeneralProperty*>* self, const re::Ptr32<GeneralProperty>* element);
GAME_FN(void, __thiscall, PropertySet_RemoveAt, 0x00838680, NiTSet<GeneralProperty*>* self, uint32_t index);
// NiTPrimitiveSet<NiGeneralComponentProperty*>::Realloc
GAME_FN(void, __thiscall, PointerSet_Realloc, 0x0083AD20, NiTSet<GeneralProperty*>* self, uint32_t size);
GAME_FN(re::Ptr32<GeneralProperty>*, __cdecl, PointerArray_New, 0x00856120, uint32_t count);
// NiTPrimitiveArray<void*> (the folded one)
GAME_FN(void, __thiscall, PointerArray_SetSize, 0x0084A110, void* self, uint32_t size);
GAME_FN(void, __thiscall, PointerArray_SetAt, 0x00833C70, void* self, uint32_t index, const void* value);

inline GeneralComponent* Self(void* iface) {
    return FromIface<GeneralComponent>(iface);
}

// the component's own property of that name (the loop the methods inline)
inline GeneralProperty* FindLocal(GeneralComponent* self, const re::Ptr32<const char>* name) {
    for (uint32_t i = 0; i < self->properties.used; i++) {
        GeneralProperty* p = self->properties.base[i];
        if (p->name == *name) {
            return p;
        }
    }
    return nullptr;
}

// the same in another component (the master: also inlined in some)
inline GeneralProperty* FindIn(GeneralComponent* component, const re::Ptr32<const char>* name) {
    if (component == nullptr) {
        return nullptr;
    }
    for (uint32_t i = 0; i < component->properties.used; i++) {
        GeneralProperty* p = component->properties.base[i];
        if (p->name == *name) {
            return p;
        }
    }
    return nullptr;
}

// the property a getter reads: its own, else the master's (found by GetPropertyByName)
inline GeneralProperty* FindForGet(GeneralComponent* self, const re::Ptr32<const char>* name) {
    GeneralProperty* p = FindLocal(self, name);
    if (p == nullptr && self->master != nullptr) {
        p = Component_GetPropertyByName(self->master, name);
    }
    return p;
}

// the property a setter writes: its own, else the master's cloned and added first (null when
// neither has it or the clone failed). The clone's reference is the set's once the local one
// goes
inline GeneralProperty* FindForSet(GeneralComponent* self, const re::Ptr32<const char>* name) {
    GeneralProperty* p = FindLocal(self, name);
    if (p != nullptr) {
        return p;
    }
    if (self->master == nullptr) {
        return nullptr;
    }
    GeneralProperty* masterProperty = Component_GetPropertyByName(self->master, name);
    if (masterProperty == nullptr) {
        return nullptr;
    }
    GeneralProperty* clone = Property_Clone(masterProperty);
    re::Ptr32<GeneralProperty> reference = clone;
    AddRef(clone);
    PropertySet_Add(&self->properties, &reference);
    if (clone == nullptr) {
        return nullptr;
    }
    Release(clone);
    return clone;
}

// an index past 0 only for a collection (IsCollection's byte: its slot held the name pointer,
// whose low byte stays if the call doesn't write it)
inline bool IndexAllowed(void* iface, const re::Ptr32<const char>* name, uint32_t index) {
    if (index != 0) {
        uint8_t isCollection = uint8_t(reinterpret_cast<uintptr_t>(name));
        if (!V(iface, slot::IsCollection, name, &isCollection) || isCollection == 0) {
            return false;
        }
    }
    return true;
}

// the element a getter reads: null past the stored ones (the game then reads address 0)
inline const void* ElementOrNull(GeneralProperty* p, uint32_t index) {
    return index < p->data.size ? p->data.base[index] : nullptr;
}

inline uint32_t ElementCount(GeneralProperty* p) {
    return p->collection != 0 ? p->data.size : 1;
}

// GetPropertyData's skeleton: `read` copies the element out (null past the stored ones)
template <typename Read>
inline uint8_t GetData(void* iface, const re::Ptr32<const char>* name, uint32_t index, Read read) {
    if (!IndexAllowed(iface, name, index)) {
        return 0;
    }
    GeneralProperty* p = FindForGet(Self(iface), name);
    if (p == nullptr || index >= ElementCount(p)) {
        return 0;
    }
    read(ElementOrNull(p, index));
    return 1;
}

// SetPropertyData's skeleton
template <typename Write>
inline uint8_t SetData(void* iface, const re::Ptr32<const char>* name, uint32_t index, Write write) {
    if (!IndexAllowed(iface, name, index)) {
        return 0;
    }
    GeneralProperty* p = FindForSet(Self(iface), name);
    if (p == nullptr) {
        return 0;
    }
    write(p);
    return 1;
}

template <typename T>
inline T ReadAs(const void* p, int offset = 0) {
    return *reinterpret_cast<const volatile T*>(reinterpret_cast<uintptr_t>(p) + offset);
}

// ---- NiEntityPropertyInterface's static names

constexpr StaticString kPrimitiveTypes[] = {
    {0x020E5C24, 0x008FFEF8},   // "Float"
    {0x020E5C28, 0x008FFF10},   // "Boolean"
    {0x020E5C2C, 0x0098AEA8},   // "Integer"
    {0x020E5C30, 0x0098AE94},   // "Unsigned Integer"
    {0x020E5C34, 0x0090E46C},   // "Short"
    {0x020E5C38, 0x008FFF00},   // "Unsigned Short"
    {0x020E5C3C, 0x008FFEDC},   // "String"
    {0x020E5C40, 0x0098AE8C},   // "Point2"
    {0x020E5C44, 0x008FFEF0},   // "Point3"
    {0x020E5C48, 0x0098AE80},   // "Quaternion"
    {0x020E5C4C, 0x008FFED4},   // "Matrix3"
    {0x020E5C50, 0x008FFEE4},   // "Color (RGB)"
    {0x020E5C54, 0x0098AE70},   // "Color (RGBA)"
    {0x020E5C58, 0x0098AE5C},   // "NiObject Pointer"
    {0x020E5C5C, 0x008FFEC4},   // "Entity Pointer"
};

void __cdecl PropertyInterfaceSDMInit() {
    SetStaticStrings(kPrimitiveTypes);
}

void __cdecl PropertyInterfaceSDMShutdown() {
    ClearStaticStrings(kPrimitiveTypes);
}

// ---- the shared ones (folded across the components)

// SetTemplateID (the interface's +0x18: the object's +0x20)
uint8_t __fastcall SetTemplateIDAt18(void* iface, re::Edx, const NiUniqueID* id) {
    At<NiUniqueID>(iface, 0x18) = *id;
    return 1;
}

// GetTemplateID (also NiUIBaseElement::GetPixelRect: 16 bytes from +0x18)
NiUniqueID* __fastcall GetTemplateIDAt18(void* iface, re::Edx, NiUniqueID* result) {
    *result = At<NiUniqueID>(iface, 0x18);
    return result;
}

// SetPropertyData(name, NiEntityInterface* / NiObject*, index): the 4 bytes through SetPropertyData(void*)
uint8_t __fastcall SetPointerData(void* iface, re::Edx, const re::Ptr32<const char>* name, void* value, uint32_t index) {
    return V(iface, slot::SetVoid, name, &value, 4u, index);
}

// GetMasterComponent (the master object's interface)
void* __fastcall GetMasterAt14(void* iface, re::Edx) {
    void* master = At<void*>(iface, 0x14);
    return master != nullptr ? reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(master) + 8) : nullptr;
}

// GetPropertyData(name, NiEntityInterface*& / NiObject*&, index): through GetPropertyData(void*)
// (the size goes in the index's slot), then memcpy_s into the 4 bytes
uint8_t __fastcall GetPointerData(void* iface, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<void>* value, uint32_t index) {
    void* data = nullptr;
    uint32_t size = index;
    uint8_t found = V(iface, slot::GetVoid, name, &data, &size, index);
    if (found) {
        crt::MemcpyS(value, 4, data, size);
    }
    return found;
}

// ---- NiGeneralComponent

GeneralProperty* __fastcall GetPropertyByName(GeneralComponent* self, re::Edx, const re::Ptr32<const char>* name) {
    return FindIn(self, name);
}

uint32_t __fastcall GetPropertyIndex(GeneralComponent* self, re::Edx, const re::Ptr32<const char>* name) {
    for (uint32_t i = 0; i < self->properties.used; i++) {
        if (self->properties.base[i]->name == *name) {
            return i;
        }
    }
    return 0xFFFFFFFF;
}

re::Ptr32<const char>* __fastcall GetClassName_(void*, re::Edx, re::Ptr32<const char>* result) {
    return ReturnString(result, NiGeneralComponent_ms_kClassName);
}

uint8_t __fastcall SetName(void* iface, re::Edx, const re::Ptr32<const char>* name) {
    FixedString_Assign(&Self(iface)->name, name);
    return 1;
}

// resettable: both it and the master have it
uint8_t __fastcall CanResetProperty(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* canReset) {
    GeneralComponent* self = Self(iface);
    GeneralProperty* local = FindLocal(self, name);
    GeneralProperty* inherited = FindIn(self->master, name);
    if (local == nullptr) {
        if (inherited == nullptr) {
            return 0;
        }
    } else if (inherited != nullptr) {
        *canReset = 1;
        return 1;
    }
    *canReset = 0;
    return 1;
}

// the local copy removed (the byte CanResetProperty writes is the name's slot)
uint8_t __fastcall ResetProperty(void* iface, re::Edx, const re::Ptr32<const char>* name) {
    uint8_t canReset = uint8_t(reinterpret_cast<uintptr_t>(name));
    if (V(iface, slot::CanResetProperty, name, &canReset) && canReset != 0) {
        PropertySet_RemoveAt(&Self(iface)->properties, Component_GetPropertyIndex(Self(iface), name));
        return 1;
    }
    return 0;
}

// (a guess at the name) the collection flag of a property of its own (none for the master's)
uint8_t __fastcall MakeCollection(void* iface, re::Edx, const re::Ptr32<const char>* name, int collection) {
    GeneralComponent* self = Self(iface);
    if (FindIn(self->master, name) != nullptr) {
        return 0;
    }
    for (uint32_t i = 0; i < self->properties.used; i++) {
        if (self->properties.base[i]->name == *name) {
            if (i != 0xFFFFFFFF) {
                self->properties.base[i]->collection = uint8_t(collection);
                return 1;
            }
            break;
        }
    }
    return 0;
}

// only its own (not the master's)
uint8_t __fastcall RemoveProperty(void* iface, re::Edx, const re::Ptr32<const char>* name) {
    GeneralComponent* self = Self(iface);
    if (FindIn(self->master, name) != nullptr) {
        return 0;
    }
    for (uint32_t i = 0; i < self->properties.used; i++) {
        if (self->properties.base[i]->name == *name) {
            if (i == 0xFFFFFFFF) {
                return 0;
            }
            PropertySet_RemoveAt(&self->properties, i);
            return 1;
        }
    }
    return 0;
}

// the getters of the property's strings
template <int Offset>
uint8_t __fastcall GetPropertyString(void* iface, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* value) {
    GeneralProperty* p = FindForGet(Self(iface), name);
    if (p == nullptr) {
        return 0;
    }
    FixedString_Assign(value, &At<NiFixedString>(p, Offset));
    return 1;
}

// the category: the component's name
uint8_t __fastcall GetCategory(void* iface, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* category) {
    GeneralProperty* p = FindForGet(Self(iface), name);
    if (p == nullptr) {
        return 0;
    }
    FixedString_Assign(category, &Self(iface)->name);
    return 1;
}

uint8_t __fastcall IsPropertyReadOnly(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* readOnly) {
    if (FindForGet(Self(iface), name) == nullptr) {
        return 0;
    }
    *readOnly = 0;
    return 1;
}

// unique: it has its own copy
uint8_t __fastcall IsPropertyUnique(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* unique) {
    GeneralComponent* self = Self(iface);
    GeneralProperty* local = FindLocal(self, name);
    GeneralProperty* inherited = FindIn(self->master, name);
    if (local == nullptr && inherited == nullptr) {
        return 0;
    }
    *unique = local != nullptr ? 1 : 0;
    return 1;
}

uint8_t __fastcall IsPropertyInheritable(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* inheritable) {
    GeneralComponent* self = Self(iface);
    GeneralProperty* local = FindLocal(self, name);
    GeneralProperty* inherited = FindIn(self->master, name);
    if (local == nullptr && inherited == nullptr) {
        return 0;
    }
    *inheritable = 1;
    return 1;
}

uint8_t __fastcall IsExternalAssetPath(void* iface, re::Edx, const re::Ptr32<const char>* name, uint32_t, uint8_t* isPath) {
    GeneralComponent* self = Self(iface);
    GeneralProperty* local = FindLocal(self, name);
    GeneralProperty* inherited = FindIn(self->master, name);
    if (local == nullptr && inherited == nullptr) {
        return 0;
    }
    *isPath = 0;
    return 1;
}

// (the byte IsCollection writes is the name's slot)
uint8_t __fastcall GetElementCount(void* iface, re::Edx, const re::Ptr32<const char>* name, uint32_t* count) {
    GeneralProperty* p = FindForGet(Self(iface), name);
    if (p == nullptr) {
        return 0;
    }
    uint8_t isCollection = uint8_t(reinterpret_cast<uintptr_t>(name));
    if (!V(iface, slot::IsCollection, name, &isCollection)) {
        return 0;
    }
    if (isCollection == 0) {
        *count = 1;
        return 1;
    }
    *count = p->collection != 0 ? p->data.size : 1;
    return 1;
}

uint8_t __fastcall IsCollection_(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* isCollection) {
    GeneralProperty* p = FindForGet(Self(iface), name);
    if (p == nullptr) {
        return 0;
    }
    *isCollection = p->collection != 0 ? 1 : 0;
    return 1;
}

// a master's property made the component's own first (MakePropertyUnique: its byte is the
// spare word the function pushed, uninitialised there: ours starts at 0), then the elements
// past `count` dropped (collections only)
uint8_t __fastcall SetElementCount(void* iface, re::Edx, const re::Ptr32<const char>* name, uint32_t count, uint8_t* countSet) {
    GeneralComponent* self = Self(iface);
    GeneralProperty* p = FindLocal(self, name);
    if (p == nullptr) {
        if (self->master == nullptr || Component_GetPropertyByName(self->master, name) == nullptr) {
            return 0;
        }
        uint8_t madeUnique = 0;
        V(iface, slot::MakePropertyUnique, name, &madeUnique);
        p = Component_GetPropertyByName(self, name);
        if (p == nullptr) {
            return 0;
        }
    }
    uint8_t isCollection = uint8_t(reinterpret_cast<uintptr_t>(name));   // (its slot: the name's)
    if (!V(iface, slot::IsCollection, name, &isCollection)) {
        return 0;
    }
    if (isCollection == 0) {
        *countSet = 0;
        return 1;
    }
    Property_SetElementCount(p, count);
    *countSet = 1;
    return 1;
}

// the getters by type
uint8_t __fastcall GetFloat(void* iface, re::Edx, const re::Ptr32<const char>* name, float* value, uint32_t index) {
    return GetData(iface, name, index, [&](const void* d) { *value = ReadAs<float>(d); });
}

uint8_t __fastcall GetBool(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* value, uint32_t index) {
    return GetData(iface, name, index, [&](const void* d) { *value = ReadAs<uint8_t>(d); });
}

// (the game's quirk: the first element whatever the index, once the index passes the count)
uint8_t __fastcall GetInt(void* iface, re::Edx, const re::Ptr32<const char>* name, int32_t* value, uint32_t index) {
    if (!IndexAllowed(iface, name, index)) {
        return 0;
    }
    GeneralProperty* p = FindForGet(Self(iface), name);
    if (p == nullptr || index >= ElementCount(p)) {
        return 0;
    }
    const void* d = p->data.size != 0 ? p->data.base[0] : nullptr;
    *value = ReadAs<int32_t>(d);
    return 1;
}

uint8_t __fastcall GetUInt(void* iface, re::Edx, const re::Ptr32<const char>* name, uint32_t* value, uint32_t index) {
    return GetData(iface, name, index, [&](const void* d) { *value = ReadAs<uint32_t>(d); });
}

// (unsigned short too)
uint8_t __fastcall GetShort(void* iface, re::Edx, const re::Ptr32<const char>* name, int16_t* value, uint32_t index) {
    return GetData(iface, name, index, [&](const void* d) { *value = ReadAs<int16_t>(d); });
}

// the string from the element's text (null past them)
uint8_t __fastcall GetString(void* iface, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* value, uint32_t index) {
    return GetData(iface, name, index,
                   [&](const void* d) { FixedString_SetString(value, static_cast<const char*>(d)); });
}

uint8_t __fastcall GetPoint2(void* iface, re::Edx, const re::Ptr32<const char>* name, NiPoint2* value, uint32_t index) {
    return GetData(iface, name, index, [&](const void* d) {
        value->x = ReadAs<float>(d);
        value->y = ReadAs<float>(d, 4);
    });
}

// (the colour too)
uint8_t __fastcall GetPoint3(void* iface, re::Edx, const re::Ptr32<const char>* name, NiPoint3* value, uint32_t index) {
    return GetData(iface, name, index, [&](const void* d) {
        value->x = ReadAs<float>(d);
        value->y = ReadAs<float>(d, 4);
        value->z = ReadAs<float>(d, 8);
    });
}

// stored by columns
uint8_t __fastcall GetMatrix3(void* iface, re::Edx, const re::Ptr32<const char>* name, NiMatrix3* value, uint32_t index) {
    return GetData(iface, name, index, [&](const void* d) {
        float* out = &value->m[0][0];
        out[0] = ReadAs<float>(d, 0x0);
        out[3] = ReadAs<float>(d, 0x4);
        out[6] = ReadAs<float>(d, 0x8);
        out[1] = ReadAs<float>(d, 0xC);
        out[4] = ReadAs<float>(d, 0x10);
        out[7] = ReadAs<float>(d, 0x14);
        out[2] = ReadAs<float>(d, 0x18);
        out[5] = ReadAs<float>(d, 0x1C);
        out[8] = ReadAs<float>(d, 0x20);
    });
}

// (the RGBA colour too)
uint8_t __fastcall GetQuaternion(void* iface, re::Edx, const re::Ptr32<const char>* name, NiQuaternion* value, uint32_t index) {
    return GetData(iface, name, index, [&](const void* d) {
        value->w = ReadAs<float>(d);
        value->x = ReadAs<float>(d, 4);
        value->y = ReadAs<float>(d, 8);
        value->z = ReadAs<float>(d, 0xC);
    });
}

// the block and its size (0 when there's none)
uint8_t __fastcall GetVoid(void* iface, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const void>* data, uint32_t* size,
                           uint32_t index) {
    if (!IndexAllowed(iface, name, index)) {
        return 0;
    }
    GeneralProperty* p = FindForGet(Self(iface), name);
    if (p == nullptr || index >= ElementCount(p)) {
        return 0;
    }
    *data = index < p->data.size ? p->data.base[index] : nullptr;
    if (index < p->data.size && p->data.base[index] != nullptr) {
        *size = p->sizes.base[index];
    } else {
        *size = 0;
    }
    return 1;
}

// ---- NiGeneralComponent's static names

constexpr StaticString kGeneralComponentNames[] = {
    {0x020E5C60, 0x0098A6CC},   // "NiGeneralComponent"
};

void __cdecl GeneralComponentSDMInit() {
    SetStaticStrings(kGeneralComponentNames);
}

void __cdecl GeneralComponentSDMShutdown() {
    ClearStaticStrings(kGeneralComponentNames);
}

// ---- NiGeneralComponentProperty

// the element's block replaced by a copy of `size` bytes (the old one freed and cleared first;
// no data: just that). The arrays grow to hold the index
void SetDataBytes(GeneralProperty* self, const void* data, uint32_t size, uint32_t index) {
    if (index < self->data.size && self->data.base[index] != nullptr) {
        NiFree(self->data.base[index]);
        void* none = nullptr;
        PointerArray_SetAt(&self->data, index, &none);
    }
    if (data != nullptr) {
        void* block = NiMalloc(size);
        uint32_t blockSize = size;
        crt::MemcpyS(block, size, data, size);
        if (self->data.maxSize <= index) {
            PointerArray_SetSize(&self->data, self->data.growBy + index);
        }
        PointerArray_SetAt(&self->data, index, &block);
        if (self->sizes.maxSize <= index) {
            PointerArray_SetSize(&self->sizes, self->sizes.growBy + index);
        }
        PointerArray_SetAt(&self->sizes, index, &blockSize);
    }
}

void __fastcall PropertySetData32(GeneralProperty* self, re::Edx, const void* data, uint32_t count, uint32_t index) {
    SetDataBytes(self, data, count * 4, index);
}

void __fastcall PropertySetData(GeneralProperty* self, re::Edx, const void* data, uint32_t size, uint32_t index) {
    SetDataBytes(self, data, size, index);
}

void __fastcall PropertySetData16(GeneralProperty* self, re::Edx, const void* data, uint32_t count, uint32_t index) {
    SetDataBytes(self, data, count * 2, index);
}

GeneralProperty* __fastcall PropertyConstruct(GeneralProperty* self, re::Edx, const re::Ptr32<const char>* name,
                                              const re::Ptr32<const char>* displayName, const re::Ptr32<const char>* primitiveType,
                                              const re::Ptr32<const char>* semanticType, const re::Ptr32<const char>* description,
                                              int collection) {
    self->vtable = reinterpret_cast<void*>(RefObject_vtable);
    self->refCount = 0;
    _InterlockedIncrement(&RefObject_ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(GeneralProperty_vtable);
    FixedStringCopy(&self->name, name);
    FixedStringCopy(&self->displayName, displayName);
    FixedStringCopy(&self->primitiveType, primitiveType);
    FixedStringCopy(&self->semanticType, semanticType);
    FixedStringCopy(&self->description, description);
    self->collection = uint8_t(collection);
    self->data.maxSize = 0;
    self->data.size = 0;
    self->data.esize = 0;
    self->data.base = nullptr;
    self->data.vtable = reinterpret_cast<void*>(PrimitiveArray_vtable);
    self->data.growBy = 1;
    self->sizes.growBy = 1;
    self->sizes.maxSize = 0;
    self->sizes.size = 0;
    self->sizes.esize = 0;
    self->sizes.base = nullptr;
    self->sizes.vtable = reinterpret_cast<void*>(PrimitiveArray_vtable);
    return self;
}

GeneralProperty* __fastcall PropertyDeletingDestructor(GeneralProperty* self, re::Edx, uint32_t flags) {
    Property_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(GeneralProperty));
    }
    return self;
}

// a collection's elements from `count` on freed, the arrays cut to it
void __fastcall PropertySetElementCount(GeneralProperty* self, re::Edx, uint32_t count) {
    if (self->collection == 0) {
        return;
    }
    uint32_t size = self->data.size;
    for (uint32_t i = count; i < size; i++) {
        if (i < self->data.size && self->data.base[i] != nullptr) {
            NiFree(self->data.base[i]);
            void* none = nullptr;
            PointerArray_SetAt(&self->data, i, &none);
        }
    }
    PointerArray_SetSize(&self->data, count);
    PointerArray_SetSize(&self->sizes, count);
}

// a copy with copies of the blocks (null when the allocation failed: the copies then go
// through a null property, as the game's would)
GeneralProperty* __fastcall PropertyClone(GeneralProperty* self, re::Edx) {
    void* memory = NiNew(sizeof(GeneralProperty));
    GeneralProperty* clone = memory == nullptr
                                 ? nullptr
                                 : Property_Construct(memory, &self->name, &self->displayName, &self->primitiveType,
                                                      &self->semanticType, &self->description, self->collection);
    uint32_t size = self->data.size;
    for (uint32_t i = 0; i < size; i++) {
        Property_SetData(clone, self->data.base[i], self->sizes.base[i], i);
    }
    return clone;
}

// ---- NiTObjectSet<NiFixedString>

// a new array of `size` handles, the used ones assigned across, the old array deleted
void __fastcall StringSetRealloc(StringSet* self, re::Edx, uint32_t size) {
    if (size == self->alloced) {
        return;
    }
    re::Ptr32<const char>* base;
    if (size != 0) {
        base = FixedStringArray_New(size);
        for (uint32_t i = 0; i < self->used; i++) {
            FixedString_Assign(&base[i], &self->base[i]);
        }
    } else {
        base = nullptr;
    }
    if (self->base != nullptr) {
        FixedStringArray_Delete(self->base, 3);
    }
    self->base = base;
    self->alloced = size;
}

re::Ptr32<const char>* __fastcall StringSetAddImpl(StringSet* self, re::Edx, const re::Ptr32<const char>* string) {
    if (self->used == self->alloced) {
        StringSet_Realloc(self, self->alloced == 0 ? 1 : self->alloced * 2);
    }
    re::Ptr32<const char>* slot = &self->base[self->used];
    self->used++;
    return FixedString_Assign(slot, string);
}

// ---- NiGeneralComponent (continued)

// a master's property copied in (none to copy when it has its own, or there's no master: then
// not made unique)
uint8_t __fastcall MakePropertyUnique(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* madeUnique) {
    uint8_t canReset = uint8_t(reinterpret_cast<uintptr_t>(name));
    if (!V(iface, slot::CanResetProperty, name, &canReset)) {
        return 0;
    }
    GeneralComponent* self = Self(iface);
    if (canReset == 0 && self->master != nullptr) {
        GeneralProperty* clone = Property_Clone(Component_GetPropertyByName(self->master, name));
        re::Ptr32<GeneralProperty> reference = clone;
        AddRef(clone);
        PropertySet_Add(&self->properties, &reference);
        Release(clone);
        *madeUnique = 1;
        return 1;
    }
    *madeUnique = 0;
    return 1;
}

// a new property (none when it or the master has one of that name), set to its type's default
uint8_t __fastcall AddProperty(void* iface, re::Edx, const re::Ptr32<const char>* name, const re::Ptr32<const char>* displayName,
                               const re::Ptr32<const char>* primitiveType, const re::Ptr32<const char>* semanticType,
                               const re::Ptr32<const char>* description) {
    GeneralComponent* self = Self(iface);
    if (FindLocal(self, name) != nullptr) {
        return 0;
    }
    if (self->master != nullptr && Component_GetPropertyByName(self->master, name) != nullptr) {
        return 0;
    }
    void* memory = NiNew(sizeof(GeneralProperty));
    GeneralProperty* p = memory == nullptr ? nullptr
                                           : Property_Construct(memory, name, displayName, primitiveType,
                                                                semanticType, description, 0);
    re::Ptr32<GeneralProperty> reference = p;
    AddRef(p);
    PropertySet_Add(&self->properties, &reference);
    Release(p);

    NiFixedString type = *primitiveType;
    if (type == PT_FLOAT) {
        V(iface, slot::SetFloat, name, 0.0f, 0u);
    } else if (type == PT_BOOL) {
        V(iface, slot::SetBool, name, 0, 0u);
    } else if (type == PT_INT) {
        V(iface, slot::SetInt, name, 0, 0u);
    } else if (type == PT_UINT) {
        V(iface, slot::SetUInt, name, 0u, 0u);
    } else if (type == PT_SHORT) {
        V(iface, slot::SetShort, name, 0, 0u);
    } else if (type == PT_USHORT) {
        V(iface, slot::SetUShort, name, 0, 0u);
    } else if (type == PT_STRING) {
        NiFixedString empty;
        V(iface, slot::SetString, name, FixedString_Construct(&empty, ""), 0u);
        FixedString_Destruct(&empty);
    } else if (type == PT_POINT2) {
        V(iface, slot::SetPoint2, name, reinterpret_cast<const void*>(Point2_ZERO), 0u);
    } else if (type == PT_POINT3) {
        V(iface, slot::SetPoint3, name, &Point3_ZERO, 0u);
    } else if (type == PT_QUATERNION) {
        V(iface, slot::SetQuaternion, name, reinterpret_cast<const void*>(Quaternion_IDENTITY), 0u);
    } else if (type == PT_MATRIX3) {
        V(iface, slot::SetMatrix3, name, &Matrix3_IDENTITY, 0u);
    } else if (type == PT_COLOR) {
        V(iface, slot::SetColor, name, reinterpret_cast<const void*>(Color_DEFAULT), 0u);
    } else if (type == PT_COLORA) {
        V(iface, slot::SetColorA, name, reinterpret_cast<const void*>(ColorA_DEFAULT), 0u);
    } else if (type == PT_NIOBJECTPOINTER) {
        V(iface, slot::SetObject, name, static_cast<void*>(nullptr), 0u);
    } else if (type == PT_ENTITYPOINTER) {
        V(iface, slot::SetEntity, name, static_cast<void*>(nullptr), 0u);
    } else {
        V(iface, slot::SetVoid, name, static_cast<const void*>(nullptr), 0u, 0u);
    }
    return 1;
}

// the setters of the property's strings (a master's property copied in first)
template <int Offset>
uint8_t __fastcall SetPropertyString(void* iface, re::Edx, const re::Ptr32<const char>* name, const re::Ptr32<const char>* value) {
    GeneralProperty* p = FindForSet(Self(iface), name);
    if (p == nullptr) {
        return 0;
    }
    FixedString_Assign(&At<NiFixedString>(p, Offset), value);
    return 1;
}

// the setters by type
uint8_t __fastcall SetBool(void* iface, re::Edx, const re::Ptr32<const char>* name, int value, uint32_t index) {
    return SetData(iface, name, index, [&](GeneralProperty* p) { Property_SetData(p, &value, 1, index); });
}

// (int and float too: 4 bytes)
uint8_t __fastcall SetUInt(void* iface, re::Edx, const re::Ptr32<const char>* name, uint32_t value, uint32_t index) {
    return SetData(iface, name, index, [&](GeneralProperty* p) { Property_SetData32(p, &value, 1, index); });
}

// (unsigned short too)
uint8_t __fastcall SetShort(void* iface, re::Edx, const re::Ptr32<const char>* name, int value, uint32_t index) {
    return SetData(iface, name, index, [&](GeneralProperty* p) { Property_SetData16(p, &value, 1, index); });
}

// the text with its terminator (none for the null handle)
uint8_t __fastcall SetString(void* iface, re::Edx, const re::Ptr32<const char>* name, const re::Ptr32<const char>* value,
                             uint32_t index) {
    return SetData(iface, name, index, [&](GeneralProperty* p) {
        const char* text = *value;
        uint32_t size = 0;
        if (text != nullptr) {
            const char* end = text;
            while (*end++ != '\0') {
            }
            size = uint32_t(end - (text + 1)) + 1;
        }
        Property_SetData(p, text, size, index);
    });
}

uint8_t __fastcall SetPoint2(void* iface, re::Edx, const re::Ptr32<const char>* name, const NiPoint2* value, uint32_t index) {
    return SetData(iface, name, index, [&](GeneralProperty* p) { Property_SetData32(p, value, 2, index); });
}

// (the RGBA colour too)
uint8_t __fastcall SetQuaternion(void* iface, re::Edx, const re::Ptr32<const char>* name, const NiQuaternion* value,
                                 uint32_t index) {
    return SetData(iface, name, index, [&](GeneralProperty* p) { Property_SetData32(p, value, 4, index); });
}

// stored by columns
uint8_t __fastcall SetMatrix3(void* iface, re::Edx, const re::Ptr32<const char>* name, const NiMatrix3* value, uint32_t index) {
    return SetData(iface, name, index, [&](GeneralProperty* p) {
        const float* in = &value->m[0][0];
        float columns[9] = {in[0], in[3], in[6], in[1], in[4], in[7], in[2], in[5], in[8]};
        Property_SetData32(p, columns, 9, index);
    });
}

// (the colour too)
uint8_t __fastcall SetPoint3(void* iface, re::Edx, const re::Ptr32<const char>* name, const NiPoint3* value, uint32_t index) {
    return SetData(iface, name, index, [&](GeneralProperty* p) { Property_SetData32(p, value, 3, index); });
}

uint8_t __fastcall SetVoid(void* iface, re::Edx, const re::Ptr32<const char>* name, const void* data, uint32_t size,
                           uint32_t index) {
    return SetData(iface, name, index, [&](GeneralProperty* p) { Property_SetData(p, data, size, index); });
}

GeneralComponent* __fastcall ComponentConstruct(GeneralComponent* self, re::Edx, const re::Ptr32<const char>* name,
                                                const NiUniqueID* templateID, uint32_t propertyCount,
                                                GeneralComponent* master) {
    ConstructBases(self);
    self->vtable = reinterpret_cast<void*>(GeneralComponent_vtable);
    self->ifaceVtable = reinterpret_cast<void*>(GeneralComponentIface_vtable);
    FixedStringCopy(&self->name, name);
    self->properties.base = propertyCount != 0 ? PointerArray_New(propertyCount) : nullptr;
    self->properties.alloced = propertyCount;
    self->properties.used = 0;
    self->master = master;
    AddRef(master);
    self->templateID = *templateID;
    return self;
}

// a copy: inheriting (this as its master, no properties of its own) or with copies of its
// properties (and its master)
void* __fastcall Clone_(void* iface, re::Edx, int inherit) {
    GeneralComponent* self = Self(iface);
    GeneralComponent* clone;
    if (re::Flag(inherit)) {
        void* memory = NiNew(sizeof(GeneralComponent));
        if (memory == nullptr) {
            return nullptr;
        }
        clone = Component_Construct(memory, &self->name, &self->templateID, self->properties.used, self);
    } else {
        uint32_t count = self->properties.used;
        void* memory = NiNew(sizeof(GeneralComponent));
        clone = memory == nullptr
                    ? nullptr
                    : Component_Construct(memory, &self->name, &self->templateID, count, self->master);
        for (uint32_t i = 0; i < count; i++) {
            GeneralProperty* p = Property_Clone(self->properties.base[i]);
            re::Ptr32<GeneralProperty> reference = p;
            AddRef(p);
            PropertySet_Add(reinterpret_cast<NiTSet<GeneralProperty*>*>(reinterpret_cast<uintptr_t>(clone) + 0x10),
                            &reference);
            Release(p);
        }
    }
    return clone != nullptr ? reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(clone) + 8) : nullptr;
}

// none: every property made its own, the master let go. A new one: held, and the properties
// it doesn't have removed
void __fastcall SetMasterComponent(void* iface, re::Edx, void* masterIface) {
    GeneralComponent* self = Self(iface);
    if (masterIface == nullptr) {
        StringSet names{FixedStringArray_New(10), 10, 0};
        V<void>(iface, slot::GetPropertyNames, &names);
        uint32_t count = names.used;
        for (uint32_t i = 0; i < count; i++) {
            uint8_t madeUnique;   // (its slot: the argument's, the null master)
            madeUnique = 0;
            V(iface, slot::MakePropertyUnique, &names.base[i], &madeUnique);
        }
        GeneralComponent* master = self->master;
        if (master != nullptr) {
            Release(master);
            self->master = nullptr;
        }
        if (names.base != nullptr) {
            FixedStringArray_Delete(names.base, 3);
        }
        return;
    }

    GeneralComponent* master = FromIface<GeneralComponent>(masterIface);
    if (self->master != master) {
        Release(self->master);
        self->master = master;
        AddRef(master);
    }
    uint32_t count = self->properties.used;
    NiTSet<GeneralProperty*> missing{count != 0 ? static_cast<re::Ptr32<GeneralProperty>*>(NiMalloc(count * 4)) : nullptr,
                                     count, 0};
    for (uint32_t i = 0; i < count; i++) {
        GeneralProperty* p = self->properties.base[i];
        if (FindIn(self->master, &p->name) != nullptr) {
            continue;
        }
        if (missing.used == missing.alloced) {
            PointerSet_Realloc(&missing, missing.alloced == 0 ? 1 : missing.alloced * 2);
        }
        missing.base[missing.used++] = p;
    }
    for (uint32_t j = 0; j < missing.used; j++) {
        for (uint32_t i = 0; i < self->properties.used; i++) {
            if (self->properties.base[i]->name == missing.base[j]->name) {
                if (i != 0xFFFFFFFF) {
                    PropertySet_RemoveAt(&self->properties, i);
                }
                break;
            }
        }
    }
    NiFree(missing.base);
}

// the master's names when it has one (they're all its), else its own
void __fastcall GetPropertyNames_(void* iface, re::Edx, StringSet* names) {
    GeneralComponent* self = Self(iface);
    if (self->master != nullptr) {
        V<void>(IfaceOf(self->master), slot::GetPropertyNames, names);
        return;
    }
    uint32_t count = self->properties.used;
    for (uint32_t i = 0; i < count; i++) {
        GeneralProperty* p = self->properties.base[i];
        if (names->used == names->alloced) {
            StringSet_Realloc(names, names->alloced == 0 ? 1 : names->alloced * 2);
        }
        re::Ptr32<const char>* slot = &names->base[names->used];
        names->used++;
        NiFixedString s = p->name;
        if (*slot != s) {
            FixedStringAddRef(s);
            FixedStringRelease(slot);
            *slot = s;
        }
    }
}

} // namespace
} // namespace ni::ec

// ---- registration ---------------------------------------------------------------------

#define NIGENCOMP_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

namespace gc = ni::ec;

// (the static names: from NiEntitySDM::Init / Shutdown)
NIGENCOMP_REVERSED("NiEntityPropertyInterface::_SDMInit", 0x00846630, &gc::PropertyInterfaceSDMInit);
NIGENCOMP_REVERSED("NiEntityPropertyInterface::_SDMShutdown", 0x00846720, &gc::PropertyInterfaceSDMShutdown);
NIGENCOMP_REVERSED("NiGeneralComponent::SetTemplateID", 0x008467E0, &gc::SetTemplateIDAt18);
// (folded: NiUIBaseElement's 16 bytes at +0x18 too)
NIGENCOMP_REVERSED("NiGeneralComponent::GetTemplateID", 0x00846800, &gc::GetTemplateIDAt18);
NIGENCOMP_REVERSED("NiGeneralComponent::SetPropertyData(NiObject*)", 0x00846820, &gc::SetPointerData);
NIGENCOMP_REVERSED("NiGeneralComponent::GetMasterComponent", 0x00846840, &gc::GetMasterAt14);
NIGENCOMP_REVERSED("NiGeneralComponent::GetPropertyData(NiObject*)", 0x00846850, &gc::GetPointerData);
NIGENCOMP_REVERSED("NiGeneralComponent::GetPropertyByName", 0x008468A0, &gc::GetPropertyByName);
NIGENCOMP_REVERSED("NiGeneralComponent::GetPropertyIndex", 0x008468D0, &gc::GetPropertyIndex);
NIGENCOMP_REVERSED("NiGeneralComponent::GetClassName", 0x00846900, &gc::GetClassName_);
NIGENCOMP_REVERSED("NiGeneralComponent::SetName", 0x00846930, &gc::SetName);
NIGENCOMP_REVERSED("NiGeneralComponent::CanResetProperty", 0x00846950, &gc::CanResetProperty);
NIGENCOMP_REVERSED("NiGeneralComponent::ResetProperty", 0x008469E0, &gc::ResetProperty);
NIGENCOMP_REVERSED("NiGeneralComponent::MakeCollection", 0x00846A30, &gc::MakeCollection);
NIGENCOMP_REVERSED("NiGeneralComponent::RemoveProperty", 0x00846AB0, &gc::RemoveProperty);
NIGENCOMP_REVERSED("NiGeneralComponent::GetDisplayName", 0x00846B30, &gc::GetPropertyString<0xC>);
NIGENCOMP_REVERSED("NiGeneralComponent::GetPrimitiveType", 0x00846B90, &gc::GetPropertyString<0x10>);
NIGENCOMP_REVERSED("NiGeneralComponent::GetSemanticType", 0x00846BF0, &gc::GetPropertyString<0x14>);
NIGENCOMP_REVERSED("NiGeneralComponent::GetDescription", 0x00846C50, &gc::GetPropertyString<0x18>);
NIGENCOMP_REVERSED("NiGeneralComponent::GetCategory", 0x00846CB0, &gc::GetCategory);
NIGENCOMP_REVERSED("NiGeneralComponent::IsPropertyReadOnly", 0x00846D20, &gc::IsPropertyReadOnly);
NIGENCOMP_REVERSED("NiGeneralComponent::IsPropertyUnique", 0x00846D80, &gc::IsPropertyUnique);
NIGENCOMP_REVERSED("NiGeneralComponent::IsPropertyInheritable", 0x00846E00, &gc::IsPropertyInheritable);
NIGENCOMP_REVERSED("NiGeneralComponent::IsExternalAssetPath", 0x00846E80, &gc::IsExternalAssetPath);
NIGENCOMP_REVERSED("NiGeneralComponent::GetElementCount", 0x00846F00, &gc::GetElementCount);
NIGENCOMP_REVERSED("NiGeneralComponent::IsCollection", 0x00846FB0, &gc::IsCollection_);
NIGENCOMP_REVERSED("NiGeneralComponent::GetPropertyData(float)", 0x00847010, &gc::GetFloat);
NIGENCOMP_REVERSED("NiGeneralComponent::GetPropertyData(bool)", 0x008470E0, &gc::GetBool);
NIGENCOMP_REVERSED("NiGeneralComponent::GetPropertyData(int)", 0x008471B0, &gc::GetInt);
NIGENCOMP_REVERSED("NiGeneralComponent::GetPropertyData(unsigned int)", 0x00847270, &gc::GetUInt);
NIGENCOMP_REVERSED("NiGeneralComponent::GetPropertyData(short)", 0x00847340, &gc::GetShort);
NIGENCOMP_REVERSED("NiGeneralComponent::GetPropertyData(NiFixedString)", 0x00847410, &gc::GetString);
NIGENCOMP_REVERSED("NiGeneralComponent::GetPropertyData(NiPoint2)", 0x008474E0, &gc::GetPoint2);
NIGENCOMP_REVERSED("NiGeneralComponent::GetPropertyData(NiPoint3)", 0x008475B0, &gc::GetPoint3);
NIGENCOMP_REVERSED("NiGeneralComponent::GetPropertyData(NiMatrix3)", 0x00847680, &gc::GetMatrix3);
NIGENCOMP_REVERSED("NiGeneralComponent::GetPropertyData(NiQuaternion)", 0x00847770, &gc::GetQuaternion);
NIGENCOMP_REVERSED("NiGeneralComponent::GetPropertyData(void*)", 0x00847840, &gc::GetVoid);
NIGENCOMP_REVERSED("NiGeneralComponent::_SDMInit", 0x00847920, &gc::GeneralComponentSDMInit);
NIGENCOMP_REVERSED("NiGeneralComponent::_SDMShutdown", 0x00847930, &gc::GeneralComponentSDMShutdown);
NIGENCOMP_REVERSED("NiGeneralComponentProperty::SetData32", 0x00847940, &gc::PropertySetData32);
NIGENCOMP_REVERSED("NiGeneralComponentProperty::SetData", 0x00847A00, &gc::PropertySetData);
NIGENCOMP_REVERSED("NiGeneralComponentProperty::SetData16", 0x00847AC0, &gc::PropertySetData16);
NIGENCOMP_REVERSED("NiGeneralComponentProperty::NiGeneralComponentProperty", 0x00847B80, &gc::PropertyConstruct);
NIGENCOMP_REVERSED("NiGeneralComponentProperty::DeletingDestructor", 0x00847C70, &gc::PropertyDeletingDestructor);
NIGENCOMP_REVERSED("NiGeneralComponentProperty::SetElementCount", 0x00847C90, &gc::PropertySetElementCount);
NIGENCOMP_REVERSED("NiGeneralComponentProperty::Clone", 0x00847D10, &gc::PropertyClone);
NIGENCOMP_REVERSED("NiGeneralComponent::SetElementCount", 0x00847D80, &gc::SetElementCount);
NIGENCOMP_REVERSED("NiTObjectSet<NiFixedString>::Realloc", 0x00847E40, &gc::StringSetRealloc);
NIGENCOMP_REVERSED("NiGeneralComponent::MakePropertyUnique", 0x00847EA0, &gc::MakePropertyUnique);
NIGENCOMP_REVERSED("NiGeneralComponent::AddProperty", 0x00847F40, &gc::AddProperty);
NIGENCOMP_REVERSED("NiGeneralComponent::SetDisplayName", 0x00848220, &gc::SetPropertyString<0xC>);
NIGENCOMP_REVERSED("NiGeneralComponent::SetPrimitiveType", 0x008482D0, &gc::SetPropertyString<0x10>);
NIGENCOMP_REVERSED("NiGeneralComponent::SetSemanticType", 0x00848380, &gc::SetPropertyString<0x14>);
NIGENCOMP_REVERSED("NiGeneralComponent::SetDescription", 0x00848430, &gc::SetPropertyString<0x18>);
NIGENCOMP_REVERSED("NiGeneralComponent::SetPropertyData(bool)", 0x008484E0, &gc::SetBool);
NIGENCOMP_REVERSED("NiGeneralComponent::SetPropertyData(unsigned int)", 0x008485B0, &gc::SetUInt);
NIGENCOMP_REVERSED("NiGeneralComponent::SetPropertyData(short)", 0x00848680, &gc::SetShort);
NIGENCOMP_REVERSED("NiGeneralComponent::SetPropertyData(NiFixedString)", 0x00848750, &gc::SetString);
NIGENCOMP_REVERSED("NiGeneralComponent::SetPropertyData(NiPoint2)", 0x00848840, &gc::SetPoint2);
NIGENCOMP_REVERSED("NiGeneralComponent::SetPropertyData(NiQuaternion)", 0x00848940, &gc::SetQuaternion);
NIGENCOMP_REVERSED("NiGeneralComponent::SetPropertyData(NiMatrix3)", 0x00848A50, &gc::SetMatrix3);
NIGENCOMP_REVERSED("NiGeneralComponent::SetPropertyData(NiPoint3)", 0x00848B80, &gc::SetPoint3);
NIGENCOMP_REVERSED("NiGeneralComponent::SetPropertyData(void*)", 0x00848C80, &gc::SetVoid);
NIGENCOMP_REVERSED("NiTObjectSet<NiFixedString>::Add", 0x00848D60, &gc::StringSetAddImpl);
NIGENCOMP_REVERSED("NiGeneralComponent::NiGeneralComponent", 0x00848DA0, &gc::ComponentConstruct);
NIGENCOMP_REVERSED("NiGeneralComponent::Clone", 0x00848E50, &gc::Clone_);
NIGENCOMP_REVERSED("NiGeneralComponent::SetMasterComponent", 0x00848F30, &gc::SetMasterComponent);
NIGENCOMP_REVERSED("NiGeneralComponent::GetPropertyNames", 0x00849120, &gc::GetPropertyNames_);
