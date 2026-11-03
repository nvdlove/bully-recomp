#pragma once

// Gamebryo's entity library (NiEntity) as Bully's exe has it: the property / entity / component
// interfaces, the factories, the GSA scene files (NiEntityStreamingAscii, XML through
// NiDOMTool and TinyXML), the scene and entity classes and the components, the external asset
// handlers (KF, KFM, NIF). Nothing of Bully's own code uses it beyond NiEntitySDM's start-up
// (its static object is constructed by the initializer at 0x8E7FA0, so NiEntitySDM::Init runs
// with the library's NiInit); the rest is reached through vtables.
//
// No NiRTTI on any of these: the class names are Gamebryo 2.x's (NiEntityPropertyInterface,
// NiEntityComponentInterface, NiEntityInterface, NiEntityStreaming, NiEntityStreamingAscii,
// NiDOMTool, NiFactories / NiTFactory, NiExternalAssetHandler...), matched by the strings the
// code keeps ("Component Interface", "Entity Interface", the factory registrations' class names,
// the GSA tags and errors). Slot names are 2.x's, checked against the default implementations
// (their `ret N`) and the GSA code's use of them; the order of the overloaded GetPropertyData /
// SetPropertyData slots is MSVC's (overloads in reverse declaration order). A guess where noted.
//
// The objects are NiRefObjects with the interface as a second base at +8 (the components,
// the entities): the factories hand out and the code passes the interface pointer; slot 0 / 1
// AddReference / RemoveReference count on the object's NiRefObject.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::entity {

using re::At;

// a virtual call by slot (thiscall), as the library's code makes them
template <typename R = void, typename... A>
inline R Virtual(const void* object, int slot, A... args) {
    return reinterpret_cast<R(__thiscall*)(const void*, re::Arg<A>...)>(static_cast<void*>(VtableOf(object)[slot]))(object, args...);
}

// ---- NiFixedString as the library inlines it (ni::sys's reference counting)

inline NiFixedString NullFixedString() {
    return ni::sys::kNullFixedString;
}
// NiFixedString(const char*): null text gives the null handle
inline NiFixedString MakeFixedString(const char* text) {
    return text == nullptr ? static_cast<char*>(ni::sys::kNullFixedString) : ni::sys::GlobalStringTable_AddString(text);
}
inline void ReleaseFixedString(re::Ptr32<const char>* handle) {
    ni::sys::FixedStringRelease(re::PtrCast<re::Ptr32<char>*>(handle));
}
inline NiFixedString CopyFixedString(NiFixedString handle) {
    return ni::sys::FixedStringAddRef(re::PtrCast<char*>(handle));
}

// ---- the interfaces' slots

// NiEntityPropertyInterface (vtable 0x989CE8, 69 slots, pure but for the defaults noted)
namespace prop {
enum Slot : int {
    AddReference = 0,
    RemoveReference = 1,
    GetInterfaceType = 2,      // (NiFixedString* result)
    GetClassName = 3,          // (NiFixedString* result)
    GetTemplateID = 4,         // (NiUniqueID* result)
    SetTemplateID = 5,         // (const NiUniqueID&)
    GetName = 6,               // (NiFixedString* result)
    SetName = 7,               // (const NiFixedString&)
    IsAnimated = 8,            // (default: false)
    Update = 9,                // (parent, time, errors, asset manager)
    BuildVisibleSet = 10,      // (rendering context, errors)
    GetPropertyNames = 11,     // (NiTObjectSet<NiFixedString>&)
    CanResetProperty = 12,     // (name, bool&)
    ResetProperty = 13,        // (name)
    MakePropertyUnique = 14,   // (name, bool&)
    IsAddPropertySupported = 15,   // (default: false)
    AddProperty = 16,          // (name, display name, primitive type, semantic type, description)
    MakeCollection = 17,       // (name, bool)
    RemoveProperty = 18,       // (name)
    GetDisplayName = 19,       // (name, NiFixedString&)
    SetDisplayName = 20,
    GetPrimitiveType = 21,
    SetPrimitiveType = 22,
    GetSemanticType = 23,
    SetSemanticType = 24,
    GetDescription = 25,
    SetDescription = 26,
    GetCategory = 27,
    IsPropertyReadOnly = 28,   // (name, bool&)
    IsPropertyUnique = 29,
    IsPropertySerializable = 30,
    IsPropertyInheritable = 31,
    IsExternalAssetPath = 32,  // (name, index, bool&)
    GetElementCount = 33,      // (name, unsigned int&)
    SetElementCount = 34,      // (name, count, bool&)
    IsCollection = 35,         // (name, bool&)
    // GetPropertyData(name, T&, index) by type (default: false)
    GetData = 36,              // (name, void*&, size&, index)
    GetEntityPointer = 37,
    GetObjectPointer = 38,
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
    // SetPropertyData(name, const T&, index) by type (default: false)
    SetData = 52,              // (name, const void*, size, index)
    SetEntityPointer = 53,
    SetObjectPointer = 54,
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
    DeletingDestructor = 68,
};
} // namespace prop

// NiEntityComponentInterface (vtable 0x989E00: GetInterfaceType "Component Interface")
namespace comp {
enum Slot : int {
    Clone = 69,                      // (bool inherit properties)
    GetMasterComponent = 70,
    SetMasterComponent = 71,         // (component)
    GetDependentPropertyNames = 72,  // (NiTObjectSet<NiFixedString>&)
    CanAttachToEntity = 73,          // (entity: 0x846090)
    CanDetachFromEntity = 74,        // (entity: 0x8462D0)
};
} // namespace comp

// NiEntityInterface (vtable 0x98BF68: GetInterfaceType "Entity Interface", 84 slots;
// NiGeneralEntity's 0x98C0B8)
namespace ent {
enum Slot : int {
    Clone = 69,                      // (new name, bool inherit properties)
    GetMasterEntity = 70,
    ReplaceMasterEntity = 71,        // (entity: a guess)
    GetHidden = 72,
    SetHidden = 73,                  // (bool)
    SupportsComponents = 74,         // (a guess: true)
    AddComponent = 75,               // (component, bool check)
    GetComponentCount = 76,
    GetComponentAt = 77,             // (index)
    GetComponentByTemplateID = 78,   // (const NiUniqueID&)
    RemoveComponent = 79,            // (component, bool check)
    RemoveComponentAt = 80,          // (index, bool check)
    RemoveAllComponents = 81,
    IsComponentProperty = 82,        // (name, bool&: a guess: one of its components has it)
    SetMasterEntity = 83,            // (entity)
};
} // namespace ent

// interface pointers' references (NiEntity...IPtr)
inline void AddReference(void* object) {
    if (object != nullptr) {
        Virtual<void>(object, prop::AddReference);
    }
}
inline void RemoveReference(void* object) {
    if (object != nullptr) {
        Virtual<void>(object, prop::RemoveReference);
    }
}

// ---- the templates' layouts

// NiTMap / NiTPointerMap / NiTStringPointerMap items and maps (vtable: 0 the deleting destructor,
// 1 SetValue(item, key, value), 2 ClearValue(item), 3 NewItem, 4 DeleteItem(item))
struct MapItem {
    re::Ptr32<MapItem> next;   // +0x0
    re::Ptr32<void> key;       // +0x4
    re::Ptr32<void> value;     // +0x8
};
struct PointerMap {
    re::Ptr32<void> vtable;        // +0x0
    uint32_t hashSize;   // +0x4
    re::Ptr32<re::Ptr32<MapItem>> table;     // +0x8
    uint32_t count;      // +0xC
};
static_assert(sizeof(PointerMap) == 0x10);

// NiTStringPointerMap<T> (keys copied: +0x10)
struct StringMap : PointerMap {
    uint8_t copyKeys;   // +0x10
    uint8_t pad11[3];
};
static_assert(sizeof(StringMap) == 0x14);

constexpr uint32_t kNiTMapBaseStringVtable = 0x00950760;   // NiTMapBase<NiTPointerAllocator, const char*, T>
constexpr uint32_t kNiTStringPointerMapVtable = 0x0098A564; // NiTStringPointerMap<T> (folded)

// NiTPrimitiveSet / NiTObjectSet (NiTSet's layout)
template <typename T>
struct NiTSet {
    re::Ptr32<re::Field<T>> base;            // +0x0
    uint32_t alloced;   // +0x4
    uint32_t used;      // +0x8
};
static_assert(sizeof(NiTSet<void*>) == 0xC);

// ---- NiTFactory<T> (vtable 0x98A58C, 0x14): what NiFactories keeps per kind. Three string maps
// by class name: the creation functions, the persistent instances (given to the destroy
// functions at the end) and their destroy functions (a guess at 2.x's names)
struct NiTFactory : NiRefObject {
    re::Ptr32<StringMap> creators;     // +0x8 (T (*)(void*))
    re::Ptr32<StringMap> instances;    // +0xC (T)
    re::Ptr32<StringMap> destroyers;   // +0x10 (void (*)(T))
};
static_assert(sizeof(NiTFactory) == 0x14);

constexpr uint32_t kNiTFactoryVtable = 0x0098A58C;

GAME_VAR(NiTFactory*, NiFactories_ms_pkStreamingFactory, 0x020E5BE8);
GAME_VAR(NiTFactory*, NiFactories_ms_pkAssetFactory, 0x020E5BEC);
GAME_VAR(NiTFactory*, NiFactories_ms_pkEntityCompFactory, 0x020E5BF0);

// ---- NiDOMTool (8 bytes): the TinyXML document and the stack of sections (elements) being read
// or written (an NiTPrimitiveSet<TiXmlElement*>, NiNew'd); chunk 37's
struct NiDOMTool {
    re::Ptr32<void> document;                 // +0x0 (TiXmlDocument, 0x48)
    re::Ptr32<NiTSet<void*>> sections;        // +0x4
};
static_assert(sizeof(NiDOMTool) == 8);

GAME_FN(void, __thiscall, DOM_Close, 0x00855E30, NiDOMTool* self);                     // the document freed
GAME_FN(void, __thiscall, DOM_Init, 0x00855E70, NiDOMTool* self, const char* file);   // a new document
GAME_FN(uint8_t, __thiscall, DOM_LoadFile, 0x00854BC0, NiDOMTool* self);
GAME_FN(uint8_t, __thiscall, DOM_SaveFile, 0x00854BD0, NiDOMTool* self);
GAME_FN(uint8_t, __thiscall, DOM_WriteHeader, 0x00854BE0, NiDOMTool* self);   // the <?xml ?> declaration
GAME_FN(uint8_t, __thiscall, DOM_IsCurrentSectionValid, 0x00854D20, NiDOMTool* self);
GAME_FN(uint8_t, __thiscall, DOM_EndSection, 0x00854D50, NiDOMTool* self);
GAME_FN(uint8_t, __thiscall, DOM_SetAttributeUInt, 0x00854D80, NiDOMTool* self, const char* name, uint32_t value);
GAME_FN(uint8_t, __thiscall, DOM_SetAttribute, 0x00854DC0, NiDOMTool* self, const char* name, const char* value);
GAME_FN(uint8_t, __thiscall, DOM_WriteBool, 0x00854E00, NiDOMTool* self, const uint8_t* value);
GAME_FN(uint8_t, __thiscall, DOM_WriteString, 0x00854E90, NiDOMTool* self, const re::Ptr32<const char>* value);
GAME_FN(uint8_t, __thiscall, DOM_WriteData, 0x00854F20, NiDOMTool* self, const void* data, uint32_t size);
GAME_FN(uint32_t, __thiscall, DOM_GetAttributeCount, 0x00854FB0, NiDOMTool* self);   // (the current section's)
GAME_FN(const char*, __thiscall, DOM_GetAttribute, 0x00855000, NiDOMTool* self, const char* name);
GAME_FN(const char*, __thiscall, DOM_GetSectionName, 0x00855030, NiDOMTool* self);
GAME_FN(uint8_t, __thiscall, DOM_IsLeaf, 0x00855060, NiDOMTool* self);   // no children
GAME_FN(uint8_t, __thiscall, DOM_ReadFloat, 0x00855090, NiDOMTool* self, float* value);
GAME_FN(uint8_t, __thiscall, DOM_ReadBool, 0x008550D0, NiDOMTool* self, uint8_t* value);
GAME_FN(uint8_t, __thiscall, DOM_ReadInt, 0x00855140, NiDOMTool* self, int32_t* value);
GAME_FN(uint8_t, __thiscall, DOM_ReadUInt, 0x00855180, NiDOMTool* self, uint32_t* value);
GAME_FN(uint8_t, __thiscall, DOM_ReadShort, 0x008551C0, NiDOMTool* self, int16_t* value);
GAME_FN(uint8_t, __thiscall, DOM_ReadUShort, 0x00855200, NiDOMTool* self, uint16_t* value);
GAME_FN(uint8_t, __thiscall, DOM_ReadPoint2, 0x00855240, NiDOMTool* self, float* value);
GAME_FN(uint8_t, __thiscall, DOM_ReadColorA, 0x00855290, NiDOMTool* self, float* value);   // (also the quaternion)
GAME_FN(uint8_t, __thiscall, DOM_ReadMatrix3, 0x008552E0, NiDOMTool* self, float* value);
GAME_FN(uint8_t, __thiscall, DOM_ReadPoint3, 0x00855390, NiDOMTool* self, float* value);   // (also the colour)
GAME_FN(uint8_t, __thiscall, DOM_ReadData, 0x008553E0, NiDOMTool* self, re::Ptr32<void>* data, uint32_t* size);   // (the decoder's al)
GAME_FN(uint8_t, __thiscall, DOM_ReadString, 0x00855440, NiDOMTool* self, re::Ptr32<const char>* value);
GAME_FN(void*, __thiscall, DOM_SetSectionToNextSibling, 0x00855480, NiDOMTool* self);
GAME_FN(void*, __thiscall, DOM_SetSectionToFirstChild, 0x00855500, NiDOMTool* self);
GAME_FN(void*, __thiscall, DOM_SetSectionTo, 0x00855560, NiDOMTool* self, const char* name);
GAME_FN(uint8_t, __thiscall, DOM_BeginSection, 0x008555D0, NiDOMTool* self, const char* name);
GAME_FN(void, __thiscall, DOM_WriteFloat, 0x00855650, NiDOMTool* self, const float* value);
GAME_FN(void, __thiscall, DOM_WriteInt, 0x00855720, NiDOMTool* self, const int32_t* value);
GAME_FN(void, __thiscall, DOM_WriteUInt, 0x008557F0, NiDOMTool* self, const uint32_t* value);
GAME_FN(void, __thiscall, DOM_WriteShort, 0x008558C0, NiDOMTool* self, const int16_t* value);
GAME_FN(void, __thiscall, DOM_WriteUShort, 0x00855990, NiDOMTool* self, const uint16_t* value);
GAME_FN(void, __thiscall, DOM_WritePoint2, 0x00855A60, NiDOMTool* self, const float* value);
GAME_FN(void, __thiscall, DOM_WriteColorA, 0x00855B30, NiDOMTool* self, const float* value);   // (four floats)
GAME_FN(void, __thiscall, DOM_WriteMatrix3, 0x00855C10, NiDOMTool* self, const float* value);
GAME_FN(void, __thiscall, DOM_WritePoint3, 0x00855D30, NiDOMTool* self, const float* value);   // (three floats)
GAME_FN(void*, __thiscall, DOM_ResetSectionTo, 0x00855E10, NiDOMTool* self, const char* name);

// ---- NiEntityStreaming (vtable 0x98C2B4, 0x1C): the scenes it holds, the error handler. Slots:
// 0 the deleting destructor, 1 DeleteThis, 2 Load(file), 3 Save(file), 4 GetFileExtension,
// 5 GetFileDescription (NiFixedString* results)
struct NiEntityStreaming : NiRefObject {
    NiTArray<NiRefObject*> scenes;   // +0x8 (NiTObjectArray<NiScenePtr>, vtable 0x9893C0, grows by 1)
    re::Ptr32<void> errorHandler;              // +0x18 (NiEntityErrorInterface: slot 2 ReportError)
};
static_assert(offsetof(NiEntityStreaming, errorHandler) == 0x18);
static_assert(sizeof(NiEntityStreaming) == 0x1C);

// the link IDs a file's objects are known by, both ways (chunk 37's helper): object to ID
// (NiTPointerMap<NiEntityPropertyInterface*, unsigned int>, vtable 0x98AD94) and ID to object
// (NiTMap<unsigned int, NiEntityPropertyInterfaceIPtr>, vtable 0x98A720: references held)
struct LinkIDMap {
    PointerMap objectToID;   // +0x0
    PointerMap idToObject;   // +0x10
};
static_assert(sizeof(LinkIDMap) == 0x20);

// NiEntityStreamingAscii (vtable 0x98A750, 0x348): the GSA format
struct NiEntityStreamingAscii : NiEntityStreaming {
    LinkIDMap links;       // +0x1C
    NiDOMTool dom;         // +0x3C
    char path[0x104];      // +0x44 (the file being read or written)
    char error[0x200];     // +0x148 ("GSA File Error: <file name>")
};
static_assert(offsetof(NiEntityStreamingAscii, links) == 0x1C);
static_assert(offsetof(NiEntityStreamingAscii, dom) == 0x3C);
static_assert(offsetof(NiEntityStreamingAscii, path) == 0x44);
static_assert(offsetof(NiEntityStreamingAscii, error) == 0x148);
static_assert(sizeof(NiEntityStreamingAscii) == 0x348);

constexpr uint32_t kNiEntityStreamingAsciiVtable = 0x0098A750;

// NiEntityStreaming's own (chunk 37's)
GAME_FN(void, __thiscall, Streaming_Construct, 0x008563B0, NiEntityStreaming* self);
GAME_FN(void, __thiscall, Streaming_Destruct, 0x008562F0, NiEntityStreaming* self);
GAME_FN(void, __thiscall, Streaming_ReportError, 0x00855F00, NiEntityStreaming* self, const re::Ptr32<const char>* message,
        const re::Ptr32<const char>* description, const re::Ptr32<const char>* entityName, const re::Ptr32<const char>* propertyName);
GAME_FN(uint32_t, __thiscall, Streaming_GetSceneCount, 0x00855F20, NiEntityStreaming* self);   // (a short)
GAME_FN(NiRefObject*, __thiscall, Streaming_GetSceneAt, 0x00855F30, NiEntityStreaming* self, uint32_t index);
GAME_FN(void, __thiscall, Streaming_RemoveAllScenes, 0x00856080, NiEntityStreaming* self);
GAME_FN(void, __thiscall, Links_RemoveAll, 0x00855F40, LinkIDMap* self);
GAME_FN(void, __thiscall, Links_Add, 0x00856090, LinkIDMap* self, void* object, uint32_t id);
GAME_FN(void*, __thiscall, Links_GetObject, 0x008560D0, LinkIDMap* self, uint32_t id);   // (a reference: the
                                                                                         //  caller's)
GAME_FN(uint8_t, __thiscall, Links_GetID, 0x005AC5F0, PointerMap* self, void* object, uint32_t* id);

// ---- the factories' calls

using CreateFn = void* (__cdecl*)(void*);
GAME_FN(uint8_t, __thiscall, StringMap_GetAt, 0x00800470, StringMap* self, const char* key, void* value);
GAME_FN(void, __thiscall, StringMap_SetAt, 0x00747F00, StringMap* self, const char* key, void* value);
GAME_FN(void, __thiscall, StringMap_RemoveAll, 0x005AC520, PointerMap* self);   // (NiTMapBase::RemoveAll, folded)
GAME_FN(void, __thiscall, Map_GetNext, 0x0083D890, PointerMap* self, re::Ptr32<MapItem>* position, re::Ptr32<void>* key, re::Ptr32<void>* value);

// NiFactories (this chunk's, called where modes apply)
GAME_FN(NiTFactory*, __cdecl, Factories_GetEntityCompFactory, 0x0083C9F0);

// the interfaces' type names (NiEntityPropertyInterface's statics: "Float", "Boolean", ...)
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

GAME_FN(int, __cdecl, NiStricmp, 0x00753820, const char* a, const char* b);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(re::Ptr32<const char>*, __thiscall, FixedString_AssignText, 0x00406730, re::Ptr32<const char>* self, const char* text);
GAME_FN(re::Ptr32<const char>*, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<const char>* self, const re::Ptr32<const char>* other);
GAME_FN(void, __thiscall, FixedString_Destruct, 0x00517750, re::Ptr32<const char>* self);

} // namespace ni::entity
