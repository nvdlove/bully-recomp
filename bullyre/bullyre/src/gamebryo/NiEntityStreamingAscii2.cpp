// NiEntityStreamingAscii, the writing half (the reading half and the format are in
// NiEntityStreamingAscii.cpp): Save numbers every object of the scenes it holds (chunk 37's
// AssignLinkIDs), then writes OBJECTS (each entity's and component's header, template ID, master
// and component references, and the properties it serializes, by primitive type) and SCENES.
// Nothing in Bully saves GSA files; written all the same.
//
// The stack slots the code reuses are kept where their contents are known (a value written
// when its GetPropertyData fails is the argument slot's old contents: the object pointer's bits;
// a link ID not found leaves the slot as it was). Where a slot is an uninitialised local, the
// value here starts at zero (the game's would be garbage): noted at each.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiEntity.h"
#include "reversed.h"

namespace ni::entity {
namespace {

using Ascii = NiEntityStreamingAscii;

inline const char* Str(uintptr_t address) {
    return reinterpret_cast<const char*>(address);
}
constexpr uintptr_t kGSA = 0x00900118;
constexpr uintptr_t kOBJECTS = 0x00900110;
constexpr uintptr_t kSCENES = 0x0098ABD4;
constexpr uintptr_t kSCENE = 0x0098A900;
constexpr uintptr_t kENTITY = 0x00900100;
constexpr uintptr_t kCOMPONENT = 0x009000F4;
constexpr uintptr_t kPROPERTY = 0x008FFF5C;
constexpr uintptr_t kITEM = 0x0098A8F8;
constexpr uintptr_t kSELECTIONS = 0x0098A78C;
constexpr uintptr_t kSET = 0x0098A788;
constexpr uintptr_t kClass = 0x008FFF68;
constexpr uintptr_t kName = 0x008FFF54;
constexpr uintptr_t kLinkID = 0x00900108;
constexpr uintptr_t kRefLinkID = 0x009000E8;
constexpr uintptr_t kMasterLinkID = 0x00900008;
constexpr uintptr_t kTemplateID = 0x0098A908;
constexpr uintptr_t kHidden = 0x0098AA18;
constexpr uintptr_t kAsset = 0x0098A734;
constexpr uintptr_t kSemanticType = 0x0098A7A4;
constexpr uintptr_t kDisplayName = 0x0098A798;
constexpr uintptr_t kDescription = 0x00940DD8;
constexpr uintptr_t kMajor = 0x0098A6F0;
constexpr uintptr_t kMinor = 0x0098A6E8;
constexpr uintptr_t kPatch = 0x0098A6E0;
constexpr uintptr_t kNULL = 0x008FFD60;
constexpr uintptr_t kTRUE = 0x00923004;
constexpr uintptr_t kNiScene = 0x008FFE78;
constexpr uintptr_t kHexFormat = 0x00912ED0;   // "%x"
constexpr uintptr_t kComma = 0x00949454;       // ","
constexpr uintptr_t kEmpty = 0x008FEFBB;
constexpr uintptr_t kErrSave = 0x0098AC38;     // "An error occurred when saving the GSA file."

GAME_VAR(NiFixedString, IT_COMPONENTINTERFACE, 0x020E5C1C);
GAME_VAR(NiFixedString, IT_ENTITYINTERFACE, 0x020E5C20);

GAME_FN(uint8_t, __cdecl, NiPath_Standardize, 0x00752C20, char* path);
GAME_FN(uint32_t, __cdecl, NiPath_ConvertToRelative, 0x00752E70, char* relative, uint32_t bytes, const char* absolute,
        const char* relativeTo);
GAME_FN(void, __thiscall, Streaming_AssignLinkIDs, 0x00856770, NiEntityStreaming* self, LinkIDMap* links);
GAME_FN(uint32_t, __thiscall, Links_GetCount, 0x0077E8E0, LinkIDMap* self);   // (folded: +0x1C)
GAME_FN(void, __thiscall, FixedStringArray_VectorDelete, 0x0075B1B0, re::Ptr32<const char>* base, uint32_t flags);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);

// this file's (called where modes apply)
GAME_FN(uint8_t, __thiscall, SetFilename, 0x0083FBB0, Ascii* self, const char* file);
GAME_FN(void, __thiscall, WriteEntityRef, 0x0083DEB0, Ascii* self, void* entity);
GAME_FN(void, __thiscall, WriteSelections, 0x0083F530, Ascii* self, void* scene);
GAME_FN(void, __thiscall, WriteValue, 0x0083F640, Ascii* self, void* object, const re::Ptr32<const char>* name,
        const re::Ptr32<const char>* type, uint32_t index);
GAME_FN(void, __thiscall, WritePropertyHeader, 0x0083F820, Ascii* self, void* object, const re::Ptr32<const char>* name);
GAME_FN(void, __thiscall, WriteScene, 0x00840710, Ascii* self, void* scene);
GAME_FN(void, __thiscall, WriteTemplateID, 0x00840790, Ascii* self, void* object);
GAME_FN(void, __thiscall, WriteItems, 0x00840870, Ascii* self, void* object, const re::Ptr32<const char>* name,
        const re::Ptr32<const char>* type, uint32_t count);
GAME_FN(void, __thiscall, WriteProperty, 0x008408E0, Ascii* self, void* object, const re::Ptr32<const char>* name);
GAME_FN(void, __thiscall, WriteEntity, 0x008415B0, Ascii* self, void* entity);
GAME_FN(void, __thiscall, WriteComponent, 0x00841860, Ascii* self, void* component);
GAME_FN(void, __thiscall, WriteObjects, 0x00842370, Ascii* self);
GAME_FN(void, __thiscall, WriteDocument, 0x00842A50, Ascii* self);

// the writers: thiscall (object, name, index)
using WriterFn = void(__thiscall*)(Ascii* self, void* object, const re::Ptr32<const char>* name, uint32_t index);

// NiTObjectSet<NiFixedString>'s destructor, inlined
inline void DeleteFixedStringArray(re::Ptr32<const char>* base) {
    uint32_t* header = reinterpret_cast<uint32_t*>(base) - 1;
    for (int32_t i = int32_t(*header) - 1; i >= 0; i--) {
        ReleaseFixedString(&base[i]);
    }
    NiDeleteArray(header, 4);
}

bool IsInterface(void* object, NiFixedString type) {
    NiFixedString found = NullFixedString();
    bool same = *Virtual<re::Ptr32<const char>*>(object, prop::GetInterfaceType, &found) == type;
    ReleaseFixedString(&found);
    return same;
}

// ---- references

// 0x83DEB0 WriteEntityRef(entity): <ENTITY RefLinkID=id|"NULL"/> (the ID slot is the argument's:
// left as the pointer when it isn't numbered)
void __fastcall WriteEntityRefImpl(Ascii* self, re::Edx, void* entity) {
    if (entity == nullptr) {
        DOM_BeginSection(&self->dom, Str(kENTITY));
        DOM_SetAttribute(&self->dom, Str(kRefLinkID), Str(kNULL));
        DOM_EndSection(&self->dom);
        return;
    }
    uint32_t id = uint32_t(reinterpret_cast<uintptr_t>(entity));
    Links_GetID(&self->links.objectToID, entity, &id);
    DOM_BeginSection(&self->dom, Str(kENTITY));
    DOM_SetAttributeUInt(&self->dom, Str(kRefLinkID), id);
    DOM_EndSection(&self->dom);
}

// ---- the property writers (object, name, index): GetPropertyData, then the value

// 0x83DFA0 an entity pointer: its RefLinkID attribute ("NULL" for none; nothing when not numbered)
void __fastcall WriteEntityPointer(Ascii* self, re::Edx, void* object, const re::Ptr32<const char>* name, uint32_t index) {
    void* entity = nullptr;
    Virtual<uint8_t>(object, prop::GetEntityPointer, name, &entity, index);
    if (entity == nullptr) {
        DOM_SetAttribute(&self->dom, Str(kRefLinkID), Str(kNULL));
        return;
    }
    uint32_t id = uint32_t(reinterpret_cast<uintptr_t>(object));   // (the object argument's slot)
    if (Links_GetID(&self->links.objectToID, entity, &id) != 0) {
        DOM_SetAttributeUInt(&self->dom, Str(kRefLinkID), id);
    }
}

// the by-value kinds: the value in the object argument's slot (its bits when the get fails)
template <typename T, int Slot, void(__thiscall* const* Write)(NiDOMTool*, const T*)>
void __fastcall WriteValueOf(Ascii* self, re::Edx, void* object, const re::Ptr32<const char>* name, uint32_t index) {
    uint32_t slot = uint32_t(reinterpret_cast<uintptr_t>(object));
    T* value = reinterpret_cast<T*>(&slot);
    Virtual<uint8_t>(object, Slot, name, value, index);
    (*Write)(&self->dom, value);
}

// 0x83E060 bool (the DOM tool's writer returns a byte)
void __fastcall WriteBool(Ascii* self, re::Edx, void* object, const re::Ptr32<const char>* name, uint32_t index) {
    uint32_t slot = uint32_t(reinterpret_cast<uintptr_t>(object));
    uint8_t* value = reinterpret_cast<uint8_t*>(&slot);
    Virtual<uint8_t>(object, prop::GetBool, name, value, index);
    DOM_WriteBool(&self->dom, value);
}

// the float-array kinds (point2, point3, quaternion, matrix3: an uninitialised local in the game,
// zero here; colour and colour-alpha start at zero in the game too)
template <int Floats, int Slot, void(__thiscall* const* Write)(NiDOMTool*, const float*)>
void __fastcall WriteFloats(Ascii* self, re::Edx, void* object, const re::Ptr32<const char>* name, uint32_t index) {
    float value[Floats] = {};
    Virtual<uint8_t>(object, Slot, name, value, index);
    (*Write)(&self->dom, value);
}

// 0x83E340 binary data (the data and size slots are the index's and the object's arguments)
void __fastcall WriteData(Ascii* self, re::Edx, void* object, const re::Ptr32<const char>* name, uint32_t index) {
    void* data = reinterpret_cast<void*>(uintptr_t(index));
    uint32_t size = uint32_t(reinterpret_cast<uintptr_t>(object));
    Virtual<uint8_t>(object, prop::GetData, name, &data, &size, index);
    DOM_WriteData(&self->dom, data, size);
}

// 0x83E500 a string; an external asset path is written relative to the file's directory (or
// standardized when it can't be) with Asset="TRUE". (The asset flag is an uninitialised byte in
// the game when IsExternalAssetPath fails; zero here)
void __fastcall WriteString(Ascii* self, re::Edx, void* object, const re::Ptr32<const char>* name, uint32_t index) {
    NiFixedString value = NullFixedString();
    Virtual<uint8_t>(object, prop::GetString, name, &value, index);
    if (value != nullptr) {
        uint8_t isAsset = 0;
        Virtual<uint8_t>(object, prop::IsExternalAssetPath, name, index, &isAsset);
        if (isAsset != 0) {
            char relative[0x104];
            if (NiPath_ConvertToRelative(relative, sizeof(relative), value, self->path) == 0) {
                char path[0x104];
                crt::StrcpyS(path, sizeof(path), value);
                NiPath_Standardize(path);
                crt::StrcpyS(relative, sizeof(relative), path);
            }
            DOM_SetAttribute(&self->dom, Str(kAsset), Str(kTRUE));
            NiFixedString text = ni::sys::GlobalStringTable_AddString(relative);
            DOM_WriteString(&self->dom, &text);
            ReleaseFixedString(&text);
        } else {
            DOM_WriteString(&self->dom, &value);
        }
    }
    ReleaseFixedString(&value);
}

// 0x83F640 WriteValue(object, name, type, index): the writer for the primitive type (an NiObject
// pointer isn't written; anything unknown is binary data)
void __fastcall WriteValueImpl(Ascii* self, re::Edx, void* object, const re::Ptr32<const char>* name, const re::Ptr32<const char>* type,
                               uint32_t index) {
    NiFixedString t = *type;
    uintptr_t writer;
    if (t == PT_ENTITYPOINTER) {
        writer = 0x0083DFA0;
    } else if (t == PT_NIOBJECTPOINTER) {
        return;
    } else if (t == PT_FLOAT) {
        writer = 0x0083E020;
    } else if (t == PT_BOOL) {
        writer = 0x0083E060;
    } else if (t == PT_INT) {
        writer = 0x0083E0A0;
    } else if (t == PT_UINT) {
        writer = 0x0083E0E0;
    } else if (t == PT_SHORT) {
        writer = 0x0083E120;
    } else if (t == PT_USHORT) {
        writer = 0x0083E160;
    } else if (t == PT_STRING) {
        writer = 0x0083E500;
    } else if (t == PT_POINT2) {
        writer = 0x0083E1A0;
    } else if (t == PT_POINT3) {
        writer = 0x0083E1E0;
    } else if (t == PT_QUATERNION) {
        writer = 0x0083E220;
    } else if (t == PT_MATRIX3) {
        writer = 0x0083E260;
    } else if (t == PT_COLOR) {
        writer = 0x0083E2A0;
    } else if (t == PT_COLORA) {
        writer = 0x0083E2F0;
    } else {
        writer = 0x0083E340;
    }
    reinterpret_cast<WriterFn>(writer)(self, object, name, index);
}

// ---- properties

// 0x83F820 WritePropertyHeader(object, name): Class (the primitive type) and Name; for objects
// that take new properties, their semantic type, display name and description (all three or none)
void __fastcall WritePropertyHeaderImpl(Ascii* self, re::Edx, void* object, const re::Ptr32<const char>* name) {
    NiFixedString display = NullFixedString();
    NiFixedString type = NullFixedString();
    NiFixedString semantic = NullFixedString();
    NiFixedString description = NullFixedString();
    if (Virtual<uint8_t>(object, prop::GetPrimitiveType, name, &type) != 0) {
        DOM_SetAttribute(&self->dom, Str(kClass), type);
        DOM_SetAttribute(&self->dom, Str(kName), *name);
        if (Virtual<uint8_t>(object, prop::IsAddPropertySupported) != 0 &&
            Virtual<uint8_t>(object, prop::GetSemanticType, name, &semantic) != 0 &&
            Virtual<uint8_t>(object, prop::GetDisplayName, name, &display) != 0 &&
            Virtual<uint8_t>(object, prop::GetDescription, name, &description) != 0) {
            DOM_SetAttribute(&self->dom, Str(kSemanticType), semantic);
            DOM_SetAttribute(&self->dom, Str(kDisplayName), display);
            DOM_SetAttribute(&self->dom, Str(kDescription), description);
        }
    }
    ReleaseFixedString(&description);
    ReleaseFixedString(&semantic);
    ReleaseFixedString(&type);
    ReleaseFixedString(&display);
}

// 0x840870 WriteItems(object, name, type, count): one ITEM per element (one empty for none)
void __fastcall WriteItemsImpl(Ascii* self, re::Edx, void* object, const re::Ptr32<const char>* name, const re::Ptr32<const char>* type,
                               uint32_t count) {
    if (count == 0) {
        DOM_BeginSection(&self->dom, Str(kITEM));
        DOM_EndSection(&self->dom);
        return;
    }
    for (uint32_t i = 0; i < count; i++) {
        DOM_BeginSection(&self->dom, Str(kITEM));
        WriteValue(self, object, name, type, i);
        DOM_EndSection(&self->dom);
    }
}

// 0x8408E0 WriteProperty(object, name): a PROPERTY section, a collection's items or the value (a
// collection's count is the name argument's slot: the pointer's bits if GetElementCount fails)
void __fastcall WritePropertyImpl(Ascii* self, re::Edx, void* object, const re::Ptr32<const char>* name) {
    DOM_BeginSection(&self->dom, Str(kPROPERTY));
    WritePropertyHeader(self, object, name);
    NiFixedString type = NullFixedString();
    Virtual<uint8_t>(object, prop::GetPrimitiveType, name, &type);
    uint8_t collection = 0;
    if (Virtual<uint8_t>(object, prop::IsCollection, name, &collection) != 0 && collection != 0) {
        uint32_t count = uint32_t(reinterpret_cast<uintptr_t>(name));
        Virtual<uint8_t>(object, prop::GetElementCount, name, &count);
        WriteItems(self, object, name, &type, count);
    } else {
        WriteValue(self, object, name, &type, 0);
    }
    DOM_EndSection(&self->dom);
    ReleaseFixedString(&type);
}

// 0x840790 WriteTemplateID(object): "b0, b1, ..." in hex, when SetTemplateID takes the ID back
void __fastcall WriteTemplateIDImpl(Ascii* self, re::Edx, void* object) {
    uint8_t id[16];
    Virtual<void>(object, prop::GetTemplateID, id);
    if (Virtual<uint8_t>(object, prop::SetTemplateID, id) == 0) {
        return;
    }
    char text[0x400];
    text[0] = '\0';
    crt::Memset(text + 1, 0, 0x3FF);
    for (uint32_t i = 0; i < 16; i++) {
        char part[0x20];
        NiSprintf(part, sizeof(part), Str(kHexFormat), uint32_t(id[i]));
        crt::StrcatS(text, sizeof(text), part);
        if (i != 15) {
            crt::StrcatS(text, sizeof(text), Str(kComma));
        }
    }
    DOM_SetAttribute(&self->dom, Str(kTemplateID), text);
}

// ---- objects

// 0x8415B0 WriteEntity(entity): LinkID, Class, Name, the template ID, Hidden, the master, the
// component references, then its own serializable properties (not its components'). The slot
// the IDs are found into is the argument's, reused for the class and name handles (the LinkID's is
// an uninitialised local in the game: zero here)
void __fastcall WriteEntityImpl(Ascii* self, re::Edx, void* entity) {
    PointerMap* ids = &self->links.objectToID;
    uint32_t linkID = 0;
    Links_GetID(ids, entity, &linkID);
    NiDOMTool* dom = &self->dom;
    DOM_BeginSection(dom, Str(kENTITY));
    DOM_SetAttributeUInt(dom, Str(kLinkID), linkID);
    NiFixedString slot = NullFixedString();
    DOM_SetAttribute(dom, Str(kClass), *Virtual<re::Ptr32<const char>*>(entity, prop::GetClassName, &slot));
    ReleaseFixedString(&slot);
    DOM_SetAttribute(dom, Str(kName), *Virtual<re::Ptr32<const char>*>(entity, prop::GetName, &slot));
    ReleaseFixedString(&slot);
    uint32_t id = uint32_t(re::PtrCast<uintptr_t>(slot));
    WriteTemplateID(self, entity);
    if (Virtual<uint8_t>(entity, ent::GetHidden) != 0) {
        DOM_SetAttribute(dom, Str(kHidden), Str(kTRUE));
    }
    void* master = Virtual<void*>(entity, ent::GetMasterEntity);
    if (master != nullptr) {
        Links_GetID(ids, master, &id);
        DOM_SetAttributeUInt(dom, Str(kMasterLinkID), id);
    }
    uint32_t count = Virtual<uint32_t>(entity, ent::GetComponentCount);
    for (uint32_t i = 0; i < count; i++) {
        void* component = Virtual<void*>(entity, ent::GetComponentAt, i);
        if (component == nullptr) {
            DOM_BeginSection(dom, Str(kCOMPONENT));
            DOM_SetAttribute(dom, Str(kRefLinkID), Str(kNULL));
        } else {
            Links_GetID(ids, component, &id);
            DOM_BeginSection(dom, Str(kCOMPONENT));
            DOM_SetAttributeUInt(dom, Str(kRefLinkID), id);
        }
        DOM_EndSection(dom);
    }
    NiTSet<NiFixedString> names = {nullptr, 0, 0};
    Virtual<void>(entity, prop::GetPropertyNames, &names);
    for (uint32_t i = 0; i < names.used; i++) {
        re::Ptr32<const char>* propertyName = &names.base[i];
        uint8_t ofComponent = 1;
        if (Virtual<uint8_t>(entity, ent::IsComponentProperty, propertyName, &ofComponent) != 0 && ofComponent == 0) {
            uint8_t serializable = 0;
            if (Virtual<uint8_t>(entity, prop::IsPropertySerializable, propertyName, &serializable) != 0 &&
                serializable != 0) {
                WriteProperty(self, entity, propertyName);
            }
        }
    }
    DOM_EndSection(dom);
    if (names.base != nullptr) {
        DeleteFixedStringArray(names.base);
    }
}

// 0x841860 WriteComponent(component): LinkID, Class, Name, the template ID, the master, then its
// serializable properties (each name copied for the write)
void __fastcall WriteComponentImpl(Ascii* self, re::Edx, void* component) {
    PointerMap* ids = &self->links.objectToID;
    uint32_t linkID = 0;   // (an uninitialised local in the game)
    Links_GetID(ids, component, &linkID);
    NiDOMTool* dom = &self->dom;
    DOM_BeginSection(dom, Str(kCOMPONENT));
    DOM_SetAttributeUInt(dom, Str(kLinkID), linkID);
    NiFixedString slot = NullFixedString();
    DOM_SetAttribute(dom, Str(kClass), *Virtual<re::Ptr32<const char>*>(component, prop::GetClassName, &slot));
    ReleaseFixedString(&slot);
    DOM_SetAttribute(dom, Str(kName), *Virtual<re::Ptr32<const char>*>(component, prop::GetName, &slot));
    ReleaseFixedString(&slot);
    uint32_t id = uint32_t(re::PtrCast<uintptr_t>(slot));
    WriteTemplateID(self, component);
    void* master = Virtual<void*>(component, comp::GetMasterComponent);
    if (master != nullptr) {
        Links_GetID(ids, master, &id);
        DOM_SetAttributeUInt(dom, Str(kMasterLinkID), id);
    }
    NiTSet<NiFixedString> names = {nullptr, 0, 0};
    Virtual<void>(component, prop::GetPropertyNames, &names);
    for (uint32_t i = 0; i < names.used; i++) {
        NiFixedString propertyName = CopyFixedString(names.base[i]);
        uint8_t serializable = 0;
        if (Virtual<uint8_t>(component, prop::IsPropertySerializable, &propertyName, &serializable) != 0 &&
            serializable != 0) {
            WriteProperty(self, component, &propertyName);
        }
        ReleaseFixedString(&propertyName);
    }
    DOM_EndSection(dom);
    if (names.base != nullptr) {
        FixedStringArray_VectorDelete(names.base, 3);
    }
}

// 0x842370 WriteObjects: every numbered object by its ID (0 up), components and entities
void __fastcall WriteObjectsImpl(Ascii* self, re::Edx) {
    DOM_BeginSection(&self->dom, Str(kOBJECTS));
    uint32_t count = Links_GetCount(&self->links);
    for (uint32_t i = 0; i < count; i++) {
        void* object = Links_GetObject(&self->links, i);
        uint32_t id = 0;
        Links_GetID(&self->links.objectToID, object, &id);
        if (IsInterface(object, IT_COMPONENTINTERFACE)) {
            WriteComponent(self, object);
        } else if (IsInterface(object, IT_ENTITYINTERFACE)) {
            WriteEntity(self, object);
        }
    }
    DOM_EndSection(&self->dom);
}

// ---- scenes

constexpr int kSceneName = 0x8;
constexpr int kSceneEntities = 0xC;
constexpr int kSceneSelectionSets = 0x28;

// 0x83F530 WriteSelections(scene): SELECTIONS (when there are any), a SET per selection set with
// its entities' references (an ID slot not found is an uninitialised local: zero here)
void __fastcall WriteSelectionsImpl(Ascii* self, re::Edx, void* scene) {
    const NiTSet<NiRefObject*>& sets = At<NiTSet<NiRefObject*>>(scene, kSceneSelectionSets);
    uint32_t count = sets.used;
    if (count == 0) {
        return;
    }
    NiDOMTool* dom = &self->dom;
    DOM_BeginSection(dom, Str(kSELECTIONS));
    for (uint32_t i = 0; i < count; i++) {
        NiRefObject* set = At<NiTSet<NiRefObject*>>(scene, kSceneSelectionSets).base[i];
        const NiTSet<void*>& entities = At<NiTSet<void*>>(set, 0xC);
        uint32_t entityCount = entities.used;
        DOM_BeginSection(dom, Str(kSET));
        DOM_SetAttribute(dom, Str(kName), At<NiFixedString>(set, 8));
        for (uint32_t j = 0; j < entityCount; j++) {
            void* entity = At<NiTSet<void*>>(set, 0xC).base[j];
            if (entity == nullptr) {
                DOM_BeginSection(dom, Str(kENTITY));
                DOM_SetAttribute(dom, Str(kRefLinkID), Str(kNULL));
            } else {
                uint32_t id = 0;
                Links_GetID(&self->links.objectToID, entity, &id);
                DOM_BeginSection(dom, Str(kENTITY));
                DOM_SetAttributeUInt(dom, Str(kRefLinkID), id);
            }
            DOM_EndSection(dom);
        }
        DOM_EndSection(dom);
    }
    DOM_EndSection(dom);
}

// 0x840710 WriteScene(scene): SCENE Class="NiScene" Name, its selection sets, then its entities'
// references
void __fastcall WriteSceneImpl(Ascii* self, re::Edx, void* scene) {
    NiDOMTool* dom = &self->dom;
    DOM_BeginSection(dom, Str(kSCENE));
    DOM_SetAttribute(dom, Str(kClass), Str(kNiScene));
    DOM_SetAttribute(dom, Str(kName), At<NiFixedString>(scene, kSceneName));
    WriteSelections(self, scene);
    uint32_t count = At<NiTSet<void*>>(scene, kSceneEntities).used;
    for (uint32_t i = 0; i < count; i++) {
        WriteEntityRef(self, At<NiTSet<void*>>(scene, kSceneEntities).base[i]);
    }
    DOM_EndSection(dom);
}

// 0x842A50 WriteDocument: the declaration, GSA 1.0.0, OBJECTS, then SCENES (null scenes skipped)
void __fastcall WriteDocumentImpl(Ascii* self, re::Edx) {
    NiDOMTool* dom = &self->dom;
    DOM_WriteHeader(dom);
    DOM_BeginSection(dom, Str(kGSA));
    DOM_SetAttributeUInt(dom, Str(kMajor), 1);
    DOM_SetAttributeUInt(dom, Str(kMinor), 0);
    DOM_SetAttributeUInt(dom, Str(kPatch), 0);
    WriteObjects(self);
    DOM_BeginSection(dom, Str(kSCENES));
    uint32_t count = Streaming_GetSceneCount(self);
    for (uint32_t i = 0; i < count; i++) {
        NiRefObject* scene = Streaming_GetSceneAt(self, i);
        if (scene != nullptr) {
            WriteScene(self, scene);
        }
    }
    DOM_EndSection(dom);
    DOM_EndSection(dom);
}

// 0x842E50 Save(file): its objects numbered, the document built and saved (an error reported
// when it can't be), everything reset after
uint8_t __fastcall Save(Ascii* self, re::Edx, const char* file) {
    DOM_Close(&self->dom);
    Links_RemoveAll(&self->links);
    crt::StrcpyS(self->path, sizeof(self->path), Str(kEmpty));
    if (SetFilename(self, file) == 0) {
        return 0;
    }
    Streaming_AssignLinkIDs(self, &self->links);
    DOM_Init(&self->dom, file);
    WriteDocument(self);
    uint8_t saved = DOM_SaveFile(&self->dom);
    if (saved == 0) {
        NiFixedString arg4 = NullFixedString();
        NiFixedString arg3 = NullFixedString();
        NiFixedString description = MakeFixedString(self->error);
        NiFixedString text = ni::sys::GlobalStringTable_AddString(Str(kErrSave));
        Streaming_ReportError(self, &text, &description, &arg3, &arg4);
        ReleaseFixedString(&text);
        ReleaseFixedString(&description);
        ReleaseFixedString(&arg3);
        ReleaseFixedString(&arg4);
    }
    DOM_Close(&self->dom);
    Links_RemoveAll(&self->links);
    crt::StrcpyS(self->path, sizeof(self->path), Str(kEmpty));
    return saved;
}

// the writer instantiations' DOM writers, by pointer
inline void(__thiscall* const kDomWriteFloat)(NiDOMTool*, const float*) = DOM_WriteFloat;
inline void(__thiscall* const kDomWriteInt)(NiDOMTool*, const int32_t*) = DOM_WriteInt;
inline void(__thiscall* const kDomWriteUInt)(NiDOMTool*, const uint32_t*) = DOM_WriteUInt;
inline void(__thiscall* const kDomWriteShort)(NiDOMTool*, const int16_t*) = DOM_WriteShort;
inline void(__thiscall* const kDomWriteUShort)(NiDOMTool*, const uint16_t*) = DOM_WriteUShort;

} // namespace

#define NIGSA2_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(Original_, __LINE__);                                                \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIGSA2_REVERSED("NiEntityStreamingAscii::WriteEntityRef", 0x0083DEB0, &WriteEntityRefImpl);
NIGSA2_REVERSED("NiEntityStreamingAscii::WriteEntityPointer", 0x0083DFA0, &WriteEntityPointer);
NIGSA2_REVERSED("NiEntityStreamingAscii::WriteFloat", 0x0083E020, (&WriteValueOf<float, prop::GetFloat, &kDomWriteFloat>));
NIGSA2_REVERSED("NiEntityStreamingAscii::WriteBool", 0x0083E060, &WriteBool);
NIGSA2_REVERSED("NiEntityStreamingAscii::WriteInt", 0x0083E0A0, (&WriteValueOf<int32_t, prop::GetInt, &kDomWriteInt>));
NIGSA2_REVERSED("NiEntityStreamingAscii::WriteUInt", 0x0083E0E0,
                (&WriteValueOf<uint32_t, prop::GetUInt, &kDomWriteUInt>));
NIGSA2_REVERSED("NiEntityStreamingAscii::WriteShort", 0x0083E120,
                (&WriteValueOf<int16_t, prop::GetShort, &kDomWriteShort>));
NIGSA2_REVERSED("NiEntityStreamingAscii::WriteUShort", 0x0083E160,
                (&WriteValueOf<uint16_t, prop::GetUShort, &kDomWriteUShort>));
NIGSA2_REVERSED("NiEntityStreamingAscii::WritePoint2", 0x0083E1A0, (&WriteFloats<2, prop::GetPoint2, &DOM_WritePoint2>));
NIGSA2_REVERSED("NiEntityStreamingAscii::WritePoint3", 0x0083E1E0, (&WriteFloats<3, prop::GetPoint3, &DOM_WritePoint3>));
NIGSA2_REVERSED("NiEntityStreamingAscii::WriteQuaternion", 0x0083E220,
                (&WriteFloats<4, prop::GetQuaternion, &DOM_WriteColorA>));
NIGSA2_REVERSED("NiEntityStreamingAscii::WriteMatrix3", 0x0083E260, (&WriteFloats<9, prop::GetMatrix3, &DOM_WriteMatrix3>));
NIGSA2_REVERSED("NiEntityStreamingAscii::WriteColor", 0x0083E2A0, (&WriteFloats<3, prop::GetColor, &DOM_WritePoint3>));
NIGSA2_REVERSED("NiEntityStreamingAscii::WriteColorA", 0x0083E2F0, (&WriteFloats<4, prop::GetColorA, &DOM_WriteColorA>));
NIGSA2_REVERSED("NiEntityStreamingAscii::WriteData", 0x0083E340, &WriteData);
NIGSA2_REVERSED("NiEntityStreamingAscii::WriteString", 0x0083E500, &WriteString);
NIGSA2_REVERSED("NiEntityStreamingAscii::WriteSelections", 0x0083F530, &WriteSelectionsImpl);
NIGSA2_REVERSED("NiEntityStreamingAscii::WriteValue", 0x0083F640, &WriteValueImpl);
NIGSA2_REVERSED("NiEntityStreamingAscii::WritePropertyHeader", 0x0083F820, &WritePropertyHeaderImpl);
NIGSA2_REVERSED("NiEntityStreamingAscii::WriteScene", 0x00840710, &WriteSceneImpl);
NIGSA2_REVERSED("NiEntityStreamingAscii::WriteTemplateID", 0x00840790, &WriteTemplateIDImpl);
NIGSA2_REVERSED("NiEntityStreamingAscii::WriteItems", 0x00840870, &WriteItemsImpl);
NIGSA2_REVERSED("NiEntityStreamingAscii::WriteProperty", 0x008408E0, &WritePropertyImpl);
NIGSA2_REVERSED("NiEntityStreamingAscii::WriteEntity", 0x008415B0, &WriteEntityImpl);
NIGSA2_REVERSED("NiEntityStreamingAscii::WriteComponent", 0x00841860, &WriteComponentImpl);
NIGSA2_REVERSED("NiEntityStreamingAscii::WriteObjects", 0x00842370, &WriteObjectsImpl);
NIGSA2_REVERSED("NiEntityStreamingAscii::WriteDocument", 0x00842A50, &WriteDocumentImpl);
NIGSA2_REVERSED("NiEntityStreamingAscii::Save", 0x00842E50, &Save);

} // namespace ni::entity
