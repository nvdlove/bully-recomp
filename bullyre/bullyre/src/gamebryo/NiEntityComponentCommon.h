#pragma once

// What the entity components (NiGeneralComponent.cpp, NiTransformationComponent.cpp,
// NiSceneGraphComponent.cpp, NiLightComponent.cpp, NiShadowGeneratorComponent.cpp,
// NiInheritedTransformationComponent.cpp: Gamebryo's NiEntity library, 0x846630..0x84F8C0)
// share. (The other half of the entity library, 0x83BC30..0x846620, is another pass's: these
// are local definitions under names of their own, ni::ec.)
//
// A component is an NiRefObject (+0, its vtable: slot 0 the deleting destructor, 1 DeleteThis)
// and an NiEntityComponentInterface (+8, the interface vtable): the interface's methods get
// `this` at +8 (MSVC's second base), so they work from the interface pointer. The interface
// (NiEntityPropertyInterface's slots, then NiEntityComponentInterface's), from the vtables at
// 0x989F30 (scene graph), 0x98A068 (transformation), 0x98A2F8 (light), 0x98A430 (shadow
// generator), 0x98A598 (general), 0x98BAC8 (inherited transformation) and their callers:
// overloaded methods sit together in reverse order of declaration, as MSVC lays them out.
// Names are Gamebryo 2.x's; IsAddPropertySupported / MakeCollection are guesses from what
// the general component does in them.
//
// "bool"s are bytes; the property names are NiFixedStrings, compared by handle.

#include <cstddef>
#include <cstdint>
#include <intrin.h>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni::ec {

using ni::scene::At;

namespace slot {
enum Slot : int {
    AddReference = 0,
    RemoveReference = 1,
    GetInterfaceType = 2,   // (NiFixedString* result)
    GetClassName = 3,       // (NiFixedString* result)
    GetTemplateID = 4,      // (NiUniqueID* result)
    SetTemplateID = 5,
    GetName = 6,            // (NiFixedString* result)
    SetName = 7,
    IsAnimated = 8,
    Update = 9,             // (parent entity, float time, errors, asset manager)
    BuildVisibleSet = 10,   // (rendering context, errors)
    GetPropertyNames = 11,  // (NiTObjectSet<NiFixedString>&)
    CanResetProperty = 12,
    ResetProperty = 13,
    MakePropertyUnique = 14,
    IsAddPropertySupported = 15,
    AddProperty = 16,       // (name, display name, primitive type, semantic type, description)
    MakeCollection = 17,    // (name, bool)
    RemoveProperty = 18,
    GetDisplayName = 19,
    SetDisplayName = 20,
    GetPrimitiveType = 21,
    SetPrimitiveType = 22,
    GetSemanticType = 23,
    SetSemanticType = 24,
    GetDescription = 25,
    SetDescription = 26,
    GetCategory = 27,
    IsPropertyReadOnly = 28,
    IsPropertyUnique = 29,
    IsPropertySerializable = 30,
    IsPropertyInheritable = 31,
    IsExternalAssetPath = 32,   // (name, index, bool&)
    GetElementCount = 33,       // (name, uint32_t&)
    SetElementCount = 34,       // (name, count, bool& set)
    IsCollection = 35,          // (name, bool&)
    // GetPropertyData(name, T& value, index)
    GetVoid = 36,   // (name, void*& data, size_t& size, index)
    GetEntity = 37,
    GetObject = 38,
    GetColorA = 39,
    GetColor = 40,
    GetMatrix3 = 41,
    GetQuaternion = 42,
    GetPoint3 = 43,
    GetPoint2 = 44,
    GetString = 45,
    GetUShort = 46,
    GetShort = 47,
    GetUInt = 48,
    GetInt = 49,
    GetBool = 50,
    GetFloat = 51,
    // SetPropertyData(name, value, index)
    SetVoid = 52,   // (name, const void* data, size_t size, index)
    SetEntity = 53,
    SetObject = 54,
    SetColorA = 55,
    SetColor = 56,
    SetMatrix3 = 57,
    SetQuaternion = 58,
    SetPoint3 = 59,
    SetPoint2 = 60,
    SetString = 61,
    SetUShort = 62,
    SetShort = 63,
    SetUInt = 64,
    SetInt = 65,
    SetBool = 66,
    SetFloat = 67,
    // NiEntityComponentInterface
    InterfaceDeletingDestructor = 68,   // (the adjustor thunk to the object's)
    Clone = 69,                         // (bool inherit) -> interface
    GetMasterComponent = 70,
    SetMasterComponent = 71,
    GetDependentPropertyNames = 72,
    CanAttachToEntity = 73,     // (the other pass's names: 0x846090)
    CanDetachFromEntity = 74,   // (0x8462D0)
};
} // namespace slot

// the object from its interface pointer and back
template <typename T>
inline T* FromIface(void* iface) {
    return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(iface) - 8);
}
inline void* IfaceOf(void* object) {
    return object == nullptr ? nullptr : reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(object) + 8);
}

// a virtual call on an interface by slot (thiscall)
template <typename R = uint8_t, typename... A>
inline R V(void* iface, int slot, A... args) {
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>(VtableOf(iface)[slot]))(iface, args...);
}

// NiUniqueID: 16 bytes
struct NiUniqueID {
    uint32_t value[4];
};
static_assert(sizeof(NiUniqueID) == 0x10);

struct NiPoint2 {
    float x, y;
};
struct NiColor {
    float r, g, b;
};
struct NiColorA {
    float r, g, b, a;
};
struct NiQuaternion {
    float w, x, y, z;
};

// NiTSet / NiTObjectSet / NiTPrimitiveSet: no vtable
template <typename T>
struct NiTSet {
    re::Ptr32<re::Field<T>> base;            // +0x0
    uint32_t alloced;   // +0x4
    uint32_t used;      // +0x8
};
static_assert(sizeof(NiTSet<void*>) == 0xC);
using StringSet = NiTSet<NiFixedString>;

// ---- NiFixedString, as the library inlines it (the empty handle is the one at 0x951028)

using ni::scene::NullFixedString;
using ni::scene::FixedString_Assign;
using ni::scene::FixedString_Destruct;
using ni::scene::FixedStringRelease;

// NiFixedString::operator=(const char*) (the string table's handle for the text)
GAME_FN(re::Ptr32<const char>*, __thiscall, FixedString_SetString, 0x00406730, re::Ptr32<const char>* self, const char* text);
// NiFixedString(const char*)
GAME_FN(re::Ptr32<const char>*, __thiscall, FixedString_Construct, 0x00401050, re::Ptr32<const char>* self, const char* text);

inline void FixedStringAddRef(NiFixedString s) {
    if (s != NullFixedString) {
        _InterlockedIncrement(reinterpret_cast<volatile long*>(re::PtrCast<char*>(s) - 8));
    }
}

// the copy constructor: the reference taken, then the handle read again
inline re::Ptr32<const char>* FixedStringCopy(re::Ptr32<const char>* to, const re::Ptr32<const char>* from) {
    FixedStringAddRef(*from);
    *to = *from;
    return to;
}

// the arrays of them (new[] with a count before them, and its vector deleting destructor)
GAME_FN(re::Ptr32<const char>*, __cdecl, FixedStringArray_New, 0x0075A760, uint32_t count);
GAME_FN(void*, __thiscall, FixedStringArray_Delete, 0x0075B1B0, re::Ptr32<const char>* self, uint32_t flags);

// NiTObjectSet<NiFixedString> (this pass's, through their addresses)
GAME_FN(void, __thiscall, StringSet_Realloc, 0x00847E40, StringSet* self, uint32_t size);
GAME_FN(re::Ptr32<const char>*, __thiscall, StringSet_Add, 0x00848D60, StringSet* self, const re::Ptr32<const char>* string);

// NiTObjectSet::Add as the components inline it: grown (doubled, or to 1) when full, then the
// handle assigned into the next slot
inline void StringSetAdd(StringSet* set, const re::Ptr32<const char>* string) {
    if (set->used == set->alloced) {
        StringSet_Realloc(set, set->alloced == 0 ? 1 : set->alloced * 2);
    }
    re::Ptr32<const char>* slot = &set->base[set->used];
    set->used++;
    FixedString_Assign(slot, string);
}

// ---- the property interface's names (NiEntityPropertyInterface's PT_*: the primitive types)

GAME_VAR(NiFixedString, PT_FLOAT, 0x020E5C24);
GAME_VAR(NiFixedString, PT_BOOL, 0x020E5C28);
GAME_VAR(NiFixedString, PT_INT, 0x020E5C2C);
GAME_VAR(NiFixedString, PT_UINT, 0x020E5C30);
GAME_VAR(NiFixedString, PT_SHORT, 0x020E5C34);
GAME_VAR(NiFixedString, PT_USHORT, 0x020E5C38);
GAME_VAR(NiFixedString, PT_STRING, 0x020E5C3C);
GAME_VAR(NiFixedString, PT_POINT2, 0x020E5C40);
GAME_VAR(NiFixedString, PT_POINT3, 0x020E5C44);
GAME_VAR(NiFixedString, PT_QUATERNION, 0x020E5C48);
GAME_VAR(NiFixedString, PT_MATRIX3, 0x020E5C4C);
GAME_VAR(NiFixedString, PT_COLOR, 0x020E5C50);
GAME_VAR(NiFixedString, PT_COLORA, 0x020E5C54);
GAME_VAR(NiFixedString, PT_NIOBJECTPOINTER, 0x020E5C58);
GAME_VAR(NiFixedString, PT_ENTITYPOINTER, 0x020E5C5C);
// NiEntityComponentInterface::IT_COMPONENTINTERFACE ("Component Interface")
GAME_VAR(NiFixedString, IT_COMPONENTINTERFACE, 0x020E5C1C);

// the constants the defaults come from
GAME_VAR(NiPoint3, Point3_ZERO, 0x020E2D44);
GAME_VAR(NiMatrix3, Matrix3_IDENTITY, 0x00BC75FC);

// ---- the bases' vtables, and the objects' count

constexpr uintptr_t RefObject_vtable = 0x008FEFD4;
constexpr uintptr_t ComponentInterface_vtable = 0x00989E00;   // NiEntityComponentInterface
constexpr uintptr_t ComponentInterfaceBase_vtable = 0x00989CE8;   // (what the destructors leave)
constexpr uintptr_t PrimitiveArray_vtable = 0x00961988;   // NiTPrimitiveArray<T*>
GAME_VAR(volatile long, RefObject_ms_uiObjects, 0x020E2168);

// NiRefObject's constructor inlined, then NiEntityComponentInterface's
inline void ConstructBases(void* object) {
    At<uint32_t>(object, 0) = RefObject_vtable;
    At<uint32_t>(object, 4) = 0;
    _InterlockedIncrement(&RefObject_ms_uiObjects);
    At<uint32_t>(object, 8) = ComponentInterface_vtable;
}

// the returns of a string: the class's handle copied out (NiFixedString's copy constructor)
inline re::Ptr32<const char>* ReturnString(re::Ptr32<const char>* result, const re::Ptr32<const char>& string) {
    NiFixedString s = string;
    if (s != NullFixedString) {
        _InterlockedIncrement(reinterpret_cast<volatile long*>(re::PtrCast<char*>(s) - 8));
        s = string;
    }
    *result = s;
    return result;
}

// a template ID made on first use (a function's static, guarded by a bit)
inline NiUniqueID* ReturnTemplateID(NiUniqueID* result, uint32_t& guard, NiUniqueID& id, uint32_t a, uint32_t b,
                                    uint32_t c, uint32_t d) {
    if ((guard & 1) == 0) {
        guard |= 1;
        id.value[0] = a;
        id.value[1] = b;
        id.value[2] = c;
        id.value[3] = d;
    }
    *result = id;
    return result;
}

// the static names: set by the library's start-up, let go at its shutdown (NiFixedString's
// operator=(const char*), one after another in the table's order, with the exe's own texts)
struct StaticString {
    uintptr_t address;   // the NiFixedString
    uintptr_t text;      // the characters (in the exe)
};
template <size_t N>
inline void SetStaticStrings(const StaticString (&strings)[N]) {
    for (const StaticString& s : strings) {
        FixedString_SetString(reinterpret_cast<re::Ptr32<const char>*>(s.address), reinterpret_cast<const char*>(s.text));
    }
}
template <size_t N>
inline void ClearStaticStrings(const StaticString (&strings)[N]) {
    for (const StaticString& s : strings) {
        FixedString_SetString(reinterpret_cast<re::Ptr32<const char>*>(s.address), nullptr);
    }
}

} // namespace ni::ec
