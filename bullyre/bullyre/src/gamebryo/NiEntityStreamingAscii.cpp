// NiEntityStreamingAscii, the GSA scene files ("Gamebryo ASCII Scene files (*.gsa)"): XML read
// through NiDOMTool (TinyXML underneath). The reading half and what the class has besides:
//
//   <GSA Major Minor Patch>                 (version 1.0.0 at most)
//     <OBJECTS>
//       <ENTITY|COMPONENT Class Name LinkID [MasterLinkID] [TemplateID] [Hidden]>
//         <COMPONENT RefLinkID/>            (an entity's components)
//         <PROPERTY Class Name [SemanticType DisplayName Description]>value | <ITEM>...</PROPERTY>
//     <SCENES>
//       <SCENE Class="NiScene" Name>
//         <ENTITY RefLinkID/> ... <SELECTIONS><SET Name><ENTITY RefLinkID/>...</SET></SELECTIONS>
//
// Load reads OBJECTS twice: first every object is created through the entity / component factory
// by its class and known by its LinkID, then (LinkObjects) each one's master, template, hidden
// flag, components and properties are read with the IDs resolved. Errors go to the error handler
// (NiEntityStreaming::ReportError: the message, "GSA File Error: <file>", the object's name, the
// property's name). The writing half is NiEntityStreamingAscii2.cpp; layouts in NiEntity.h.
//
// Quirks kept: an unnamed SET and an object section that is neither ENTITY nor COMPONENT in the
// second pass are retried forever (the loops don't move on); MasterLinkID's sscanf result isn't
// checked; ITEM collections number their items including empty ones.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiEntity.h"
#include "reversed.h"

namespace ni::entity {
namespace {

using Ascii = NiEntityStreamingAscii;

// the strings (the game's)
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
constexpr uintptr_t kUnsignedFormat = 0x0092305C;   // "%u"
constexpr uintptr_t kTemplateIDFormat = 0x0098AB04;   // "%x, %x, ..." (16)
constexpr uintptr_t kGsaFileError = 0x0098A7E0;     // "GSA File Error: %s"
constexpr uintptr_t kEmpty = 0x008FEFBB;

// the errors
constexpr uintptr_t kErrNoGsaSection = 0x0098AC18;
constexpr uintptr_t kErrVersion = 0x0098ABEC;
constexpr uintptr_t kErrEmpty = 0x0098ABDC;
constexpr uintptr_t kErrParseObject = 0x0098A8CC;
constexpr uintptr_t kErrNoFactory = 0x0098A858;      // "Could not create object of type "%s". ... factory"
constexpr uintptr_t kErrNoCreator = 0x0098A7F8;      // "... No creation function has been registered"
constexpr uintptr_t kErrNoScenes = 0x0098ABAC;
constexpr uintptr_t kErrNonNiScene = 0x0098AB60;
constexpr uintptr_t kErrSceneTag = 0x0098AB44;
constexpr uintptr_t kErrUnresolvedEntity = 0x0098AA28;
constexpr uintptr_t kErrUnnamedSet = 0x0098A990;
constexpr uintptr_t kErrSetEntity = 0x0098A920;      // "Unable to resolve an entity RefLinkID in the "%s" ..."
constexpr uintptr_t kErrTemplateID = 0x0098AAE0;
constexpr uintptr_t kErrHidden = 0x0098AAB8;
constexpr uintptr_t kErrAddComponent = 0x0098AA90;
constexpr uintptr_t kErrComponentRef = 0x0098AA6C;
constexpr uintptr_t kErrInvalidComponent = 0x0098AA4C;
constexpr uintptr_t kErrPropertyTag = 0x0098A9DC;
constexpr uintptr_t kErrPropertyValue = 0x0098A9F8;
constexpr uintptr_t kErrInterfaceType = 0x0098A768;
constexpr uintptr_t kErrXml = 0x0098AC64;

GAME_VAR(NiFixedString, NiEntityStreamingAscii_STREAMING_EXTENSION, 0x020E5BF4);     // "GSA"
GAME_VAR(NiFixedString, NiEntityStreamingAscii_STREAMING_DESCRIPTION, 0x020E5BF8);
GAME_VAR(NiFixedString, IT_COMPONENTINTERFACE, 0x020E5C1C);
GAME_VAR(NiFixedString, IT_ENTITYINTERFACE, 0x020E5C20);
constexpr uintptr_t kStreamingDescriptionText = 0x0098A7B4;   // "Gamebryo ASCII Scene files (*.gsa)|*.gsa"

// NiPath (the system library's)
GAME_FN(uint8_t, __cdecl, NiPath_Standardize, 0x00752C20, char* path);
GAME_FN(uint8_t, __cdecl, NiPath_IsUniqueAbsolute, 0x00752C90, const char* path);
GAME_FN(void, __cdecl, NiPath_RemoveDotDots, 0x00752D90, char* path);
GAME_FN(uint32_t, __cdecl, NiPath_ConvertToAbsolute, 0x007531F0, char* path, uint32_t bytes, const char* relativeTo);
GAME_FN(uint32_t, __cdecl, NiPath_ConvertToAbsoluteFromCwd, 0x00753280, char* path, uint32_t bytes);
GAME_FN(uint8_t, __cdecl, NiPath_IsRelative, 0x00753B50, const char* path);

// the other classes'
GAME_FN(void*, __thiscall, Scene_Construct, 0x00841A80, void* self, const re::Ptr32<const char>* name, uint32_t entities,
        uint32_t selectionSets);
GAME_FN(uint8_t, __thiscall, Scene_AddSelectionSet, 0x00840CE0, void* scene, void* set);
GAME_FN(void, __thiscall, EntitySet_Realloc, 0x008409C0, NiTSet<void*>* set, uint32_t size);
GAME_FN(void, __thiscall, BoolPointerMap_SetAt, 0x00755310, PointerMap* map, void* key, int value);
GAME_FN(void, __thiscall, Streaming_InsertScene, 0x00856400, NiEntityStreaming* self, void* scene);
GAME_FN(void*, __thiscall, UniqueID_Construct, 0x0083DF20, void* self, uint32_t b0, uint32_t b1, uint32_t b2,
        uint32_t b3, uint32_t b4, uint32_t b5, uint32_t b6, uint32_t b7, uint32_t b8, uint32_t b9, uint32_t b10,
        uint32_t b11, uint32_t b12, uint32_t b13, uint32_t b14, uint32_t b15);
GAME_FN(void, __thiscall, LinkIDMap_Construct, 0x0083F220, LinkIDMap* self);
GAME_FN(void, __thiscall, LinkIDMap_Destruct, 0x0083E470, LinkIDMap* self);
GAME_FN(void, __thiscall, DOM_Construct, 0x006CC2D0, NiDOMTool* self);   // (folded: two zeroes)

// this file's (called where modes apply)
GAME_FN(uint8_t, __thiscall, ReadRefLinkID, 0x0083DCF0, Ascii* self, uint32_t* id);
GAME_FN(uint8_t, __thiscall, ReadObjectHeader, 0x0083DD50, Ascii* self, re::Ptr32<const char>* className,
        re::Ptr32<const char>* section, re::Ptr32<const char>* name, uint32_t* linkID);
GAME_FN(uint8_t, __thiscall, ReadVersion, 0x0083DDE0, Ascii* self);
GAME_FN(void, __thiscall, Reset, 0x0083E4D0, Ascii* self);
GAME_FN(uint8_t, __thiscall, ReadMasterLinkID, 0x0083F350, Ascii* self, void* object);
GAME_FN(uint8_t, __thiscall, SetFilename, 0x0083FBB0, Ascii* self, const char* file);
GAME_FN(uint8_t, __thiscall, ReadObjects, 0x0083FC30, Ascii* self, LinkIDMap* links);
GAME_FN(uint8_t, __thiscall, ReadItems, 0x008400F0, Ascii* self, const char* type, const char* name, void* object);
GAME_FN(uint8_t, __thiscall, ReadPropertyValues, 0x00840440, Ascii* self, const char* type, const char* name,
        void* object);
GAME_FN(uint8_t, __thiscall, AddAndReadProperty, 0x00840A90, Ascii* self, const char* type, const char* name,
        const char* semantic, const char* display, const char* description, void* object);
GAME_FN(uint8_t, __thiscall, ReadSelections, 0x00840D60, Ascii* self, void* scene);
GAME_FN(uint8_t, __thiscall, ReadProperty, 0x00841260, Ascii* self, void* object);
GAME_FN(uint8_t, __thiscall, ReadScene, 0x00841C10, Ascii* self, void* scene);
GAME_FN(uint8_t, __thiscall, ReadObjectContents, 0x00841EB0, Ascii* self, void* object);
GAME_FN(uint8_t, __thiscall, LinkObjects, 0x008424A0, Ascii* self);
GAME_FN(uint8_t, __thiscall, ReadScenes, 0x00842660, Ascii* self);
GAME_FN(uint8_t, __thiscall, ReadDocument, 0x00842AF0, Ascii* self);
GAME_FN(void, __thiscall, Destruct, 0x0083F2A0, Ascii* self);

// the readers: thiscall (object, property name, index)
using ReaderFn = uint8_t(__thiscall*)(Ascii* self, void* object, const char* name, uint32_t index);
inline ReaderFn Reader(uintptr_t address) {
    return reinterpret_cast<ReaderFn>(address);
}
constexpr uintptr_t kReadEntityPointer = 0x0083E690;
constexpr uintptr_t kReadObjectPointer = 0x0083E770;
constexpr uintptr_t kReadFloat = 0x0083E7F0;
constexpr uintptr_t kReadBool = 0x0083E890;
constexpr uintptr_t kReadInt = 0x0083E930;
constexpr uintptr_t kReadUInt = 0x0083E9D0;
constexpr uintptr_t kReadShort = 0x0083EA70;
constexpr uintptr_t kReadUShort = 0x0083EB10;
constexpr uintptr_t kReadString = 0x0083EBB0;
constexpr uintptr_t kReadPoint2 = 0x0083EDB0;
constexpr uintptr_t kReadPoint3 = 0x0083EE50;
constexpr uintptr_t kReadQuaternion = 0x0083EEF0;
constexpr uintptr_t kReadMatrix3 = 0x0083EF90;
constexpr uintptr_t kReadColor = 0x0083F030;
constexpr uintptr_t kReadColorA = 0x0083F0E0;
constexpr uintptr_t kReadData = 0x0083F9F0;

// the reader for a primitive type, in the order the code tests them (anything else: binary data)
uintptr_t ReaderFor(const char* type) {
    if (NiStricmp(type, PT_ENTITYPOINTER) == 0) {
        return kReadEntityPointer;
    }
    if (NiStricmp(type, PT_NIOBJECTPOINTER) == 0) {
        return kReadObjectPointer;
    }
    if (NiStricmp(type, PT_FLOAT) == 0) {
        return kReadFloat;
    }
    if (NiStricmp(type, PT_BOOL) == 0) {
        return kReadBool;
    }
    if (NiStricmp(type, PT_INT) == 0) {
        return kReadInt;
    }
    if (NiStricmp(type, PT_UINT) == 0) {
        return kReadUInt;
    }
    if (NiStricmp(type, PT_SHORT) == 0) {
        return kReadShort;
    }
    if (NiStricmp(type, PT_USHORT) == 0) {
        return kReadUShort;
    }
    if (NiStricmp(type, PT_STRING) == 0) {
        return kReadString;
    }
    if (NiStricmp(type, PT_POINT2) == 0) {
        return kReadPoint2;
    }
    if (NiStricmp(type, PT_POINT3) == 0) {
        return kReadPoint3;
    }
    if (NiStricmp(type, PT_QUATERNION) == 0) {
        return kReadQuaternion;
    }
    if (NiStricmp(type, PT_MATRIX3) == 0) {
        return kReadMatrix3;
    }
    if (NiStricmp(type, PT_COLOR) == 0) {
        return kReadColor;
    }
    if (NiStricmp(type, PT_COLORA) == 0) {
        return kReadColorA;
    }
    return kReadData;
}

// ---- errors

// ReportError(message, "GSA File Error: <file>", the object's name (null: none), the property's
// name), the handles made and let go as the code does: [property], description, message,
// [object name]; let go: [object name], message, description, [property]
void Report(Ascii* self, const char* message, void* object = nullptr, const char* propertyName = nullptr) {
    NiFixedString property = propertyName != nullptr ? MakeFixedString(propertyName) : NullFixedString();
    NiFixedString description = MakeFixedString(self->error);
    NiFixedString text = ni::sys::GlobalStringTable_AddString(message);
    NiFixedString objectName = NullFixedString();
    if (object != nullptr) {
        Virtual<re::Ptr32<const char>*>(object, prop::GetName, &objectName);
    }
    Streaming_ReportError(self, &text, &description, &objectName, &property);
    ReleaseFixedString(&objectName);
    ReleaseFixedString(&text);
    ReleaseFixedString(&description);
    ReleaseFixedString(&property);
}

// whether an object's GetInterfaceType is the given one
bool IsInterface(void* object, NiFixedString type) {
    NiFixedString found = NullFixedString();
    bool same = *Virtual<re::Ptr32<const char>*>(object, prop::GetInterfaceType, &found) == type;
    ReleaseFixedString(&found);
    return same;
}

// an interface pointer added to an NiTObjectSet<IPtr> (NiScene's / a selection set's
// entities): grown (doubled, 1 when empty), then the slot assigned (references through slots 0 / 1)
void AddEntity(NiTSet<void*>* set, void* entity) {
    if (set->used == set->alloced) {
        EntitySet_Realloc(set, set->alloced != 0 ? set->alloced * 2 : 1);
    }
    re::Ptr32<void>* slot = &set->base[set->used];
    set->used++;
    if (*slot != entity) {
        RemoveReference(*slot);
        *slot = entity;
        AddReference(entity);
    }
}

// ---- the small readers

// 0x83DCF0 ReadRefLinkID(id): "NULL" is -1
uint8_t __fastcall ReadRefLinkIDImpl(Ascii* self, re::Edx, uint32_t* id) {
    const char* text = DOM_GetAttribute(&self->dom, Str(kRefLinkID));
    if (text == nullptr) {
        return 0;
    }
    if (NiStricmp(text, Str(kNULL)) == 0) {
        *id = 0xFFFFFFFF;
        return 1;
    }
    return crt::Sscanf(text, Str(kUnsignedFormat), id) == 1 ? 1 : 0;
}

// 0x83DD50 ReadObjectHeader(class, section, name, link ID): all four there
uint8_t __fastcall ReadObjectHeaderImpl(Ascii* self, re::Edx, re::Ptr32<const char>* className, re::Ptr32<const char>* section,
                                        re::Ptr32<const char>* name, uint32_t* linkID) {
    *className = DOM_GetAttribute(&self->dom, Str(kClass));
    *section = DOM_GetSectionName(&self->dom);
    *name = DOM_GetAttribute(&self->dom, Str(kName));
    const char* id = DOM_GetAttribute(&self->dom, Str(kLinkID));
    if (*section == nullptr || *name == nullptr || *className == nullptr || id == nullptr) {
        return 0;
    }
    return crt::Sscanf(id, Str(kUnsignedFormat), linkID) == 1 ? 1 : 0;
}

// 0x83DDE0 ReadVersion: Major.Minor.Patch no later than 1.0.0
uint8_t __fastcall ReadVersionImpl(Ascii* self, re::Edx) {
    const char* majorText = DOM_GetAttribute(&self->dom, Str(kMajor));
    const char* minorText = DOM_GetAttribute(&self->dom, Str(kMinor));
    const char* patchText = DOM_GetAttribute(&self->dom, Str(kPatch));
    if (majorText == nullptr || minorText == nullptr || patchText == nullptr) {
        return 0;
    }
    uint32_t major = 0;
    uint32_t minor = 0;
    uint32_t patch = 0;
    if (crt::Sscanf(majorText, Str(kUnsignedFormat), &major) != 1 || major > 1) {
        return 0;
    }
    if (crt::Sscanf(minorText, Str(kUnsignedFormat), &minor) != 1 || (major == 1 && minor > 0)) {
        return 0;
    }
    if (crt::Sscanf(patchText, Str(kUnsignedFormat), &patch) != 1 || (major == 1 && minor == 0 && patch > 0)) {
        return 0;
    }
    return 1;
}

// 0x83E4D0 Reset: the document closed, the link IDs forgotten, no file
void __fastcall ResetImpl(Ascii* self, re::Edx) {
    DOM_Close(&self->dom);
    Links_RemoveAll(&self->links);
    crt::StrcpyS(self->path, sizeof(self->path), Str(kEmpty));
}

// ---- the property readers (object, name, index): the value read, then SetPropertyData

// 0x83E690 an entity pointer (RefLinkID, "NULL" none)
uint8_t __fastcall ReadEntityPointerImpl(Ascii* self, re::Edx, void* object, const char* name, uint32_t index) {
    uint32_t id = 0;
    if (ReadRefLinkID(self, &id) == 0) {
        return 0;
    }
    void* entity = nullptr;
    if (id != 0xFFFFFFFF) {
        entity = Links_GetObject(&self->links, id);
    }
    NiFixedString key = MakeFixedString(name);
    uint8_t result = Virtual<uint8_t>(object, prop::SetEntityPointer, &key, entity, index);
    ReleaseFixedString(&key);
    return result;
}

// 0x83E770 an NiObject pointer: always set to null (GSA files don't hold objects)
uint8_t __fastcall ReadObjectPointerImpl(Ascii*, re::Edx, void* object, const char* name, uint32_t index) {
    NiFixedString key = MakeFixedString(name);
    uint8_t result = Virtual<uint8_t>(object, prop::SetObjectPointer, &key, static_cast<void*>(nullptr), index);
    ReleaseFixedString(&key);
    return result;
}

// the by-value kinds (float, bool, int, unsigned int, short, unsigned short)
template <typename T, uint8_t(__thiscall* const* Read)(NiDOMTool*, T*), int Slot>
uint8_t __fastcall ReadValue(Ascii* self, re::Edx, void* object, const char* name, uint32_t index) {
    T value{};
    if ((*Read)(&self->dom, &value) == 0) {
        return 0;
    }
    NiFixedString key = MakeFixedString(name);
    uint8_t result = Virtual<uint8_t>(object, Slot, &key, value, index);
    ReleaseFixedString(&key);
    return result;
}

// the by-reference kinds (point2, point3, quaternion, matrix3; colour and colour-alpha start at
// zero)
template <int Floats, uint8_t(__thiscall* const* Read)(NiDOMTool*, float*), int Slot>
uint8_t __fastcall ReadFloats(Ascii* self, re::Edx, void* object, const char* name, uint32_t index) {
    float value[Floats] = {};
    if ((*Read)(&self->dom, value) == 0) {
        return 0;
    }
    NiFixedString key = MakeFixedString(name);
    uint8_t result = Virtual<uint8_t>(object, Slot, &key, value, index);
    ReleaseFixedString(&key);
    return result;
}

// 0x83EBB0 a string; an Asset="TRUE" one is a path relative to the file's directory, made
// absolute (standardized first)
uint8_t __fastcall ReadStringImpl(Ascii* self, re::Edx, void* object, const char* name, uint32_t index) {
    NiFixedString value = NullFixedString();
    if (DOM_ReadString(&self->dom, &value) == 0) {
        ReleaseFixedString(&value);
        return 0;
    }
    uint8_t result = 0;
    const char* asset = DOM_GetAttribute(&self->dom, Str(kAsset));
    if (asset != nullptr && NiStricmp(asset, Str(kTRUE)) == 0) {
        char path[0x104];
        crt::StrcpyS(path, sizeof(path), value);
        NiPath_Standardize(path);
        if (NiPath_IsRelative(path) != 0) {
            NiPath_ConvertToAbsolute(path, sizeof(path), self->path);
        }
        NiFixedString absolute = ni::sys::GlobalStringTable_AddString(path);
        NiFixedString key = MakeFixedString(name);
        result = Virtual<uint8_t>(object, prop::SetString, &key, &absolute, index);
        ReleaseFixedString(&key);
        ReleaseFixedString(&absolute);
    } else {
        NiFixedString key = MakeFixedString(name);
        result = Virtual<uint8_t>(object, prop::SetString, &key, &value, index);
        ReleaseFixedString(&key);
    }
    ReleaseFixedString(&value);
    return result;
}

// 0x83F9F0 anything else: binary data (NiMalloc'd by the DOM tool, freed after)
uint8_t __fastcall ReadDataImpl(Ascii* self, re::Edx, void* object, const char* name, uint32_t index) {
    re::Ptr32<void> data = nullptr;
    uint32_t size = 0;
    if (DOM_ReadData(&self->dom, &data, &size) == 0 || data == nullptr || size == 0) {
        return 0;
    }
    NiFixedString key = MakeFixedString(name);
    uint8_t result = Virtual<uint8_t>(object, prop::SetData, &key, data, size, index);
    FixedString_Destruct(&key);
    NiFree(data);
    return result;
}

// ---- the class

// 0x83F2A0 ~NiEntityStreamingAscii
void __fastcall DestructImpl(Ascii* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiEntityStreamingAsciiVtable);
    DOM_Close(&self->dom);
    Links_RemoveAll(&self->links);
    crt::StrcpyS(self->path, sizeof(self->path), Str(kEmpty));
    DOM_Close(&self->dom);
    LinkIDMap_Destruct(&self->links);
    Streaming_Destruct(self);
}

// 0x83F2F0 / 0x83F320 GetFileExtension / GetFileDescription
re::Ptr32<const char>* __fastcall GetFileExtension(Ascii*, re::Edx, re::Ptr32<const char>* result) {
    *result = CopyFixedString(NiEntityStreamingAscii_STREAMING_EXTENSION);
    return result;
}
re::Ptr32<const char>* __fastcall GetFileDescription(Ascii*, re::Edx, re::Ptr32<const char>* result) {
    *result = CopyFixedString(NiEntityStreamingAscii_STREAMING_DESCRIPTION);
    return result;
}

// 0x83FA90 / 0x83FAB0 the statics set and cleared
void __cdecl SDMInit() {
    FixedString_AssignText(&NiEntityStreamingAscii_STREAMING_EXTENSION, Str(kGSA));
    FixedString_AssignText(&NiEntityStreamingAscii_STREAMING_DESCRIPTION, Str(kStreamingDescriptionText));
}
void __cdecl SDMShutdown() {
    FixedString_AssignText(&NiEntityStreamingAscii_STREAMING_EXTENSION, nullptr);
    FixedString_AssignText(&NiEntityStreamingAscii_STREAMING_DESCRIPTION, nullptr);
}

// 0x83FB50: the path and the error text aren't set until a file is
Ascii* __fastcall Construct(Ascii* self, re::Edx) {
    Streaming_Construct(self);
    self->vtable = reinterpret_cast<void*>(kNiEntityStreamingAsciiVtable);
    LinkIDMap_Construct(&self->links);
    DOM_Construct(&self->dom);
    return self;
}

// 0x83FB80
Ascii* __fastcall DeletingDestructor(Ascii* self, re::Edx, int flags) {
    Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(Ascii));
    }
    return self;
}

// 0x83FBB0 SetFilename(file): made absolute without ".."s, the error text made from it, then the
// path cut to its directory (the last '\')
uint8_t __fastcall SetFilenameImpl(Ascii* self, re::Edx, const char* file) {
    crt::StrcpyS(self->path, sizeof(self->path), file);
    if (NiPath_IsUniqueAbsolute(self->path) == 0) {
        if (NiPath_IsRelative(self->path) != 0) {
            NiPath_ConvertToAbsoluteFromCwd(self->path, sizeof(self->path));
        } else {
            NiPath_RemoveDotDots(self->path);
        }
    }
    NiSprintf(self->error, sizeof(self->error), Str(kGsaFileError), self->path);
    char* slash = crt::Strrchr(self->path, '\\');
    if (slash != nullptr) {
        *slash = '\0';
    }
    return 1;
}

// ---- the first pass

// 0x83FC30 ReadObjects (the link map argument unused: the class's own): every ENTITY / COMPONENT
// created by class name, named and known by its LinkID (any other section's object is created and
// named but not kept)
uint8_t __fastcall ReadObjectsImpl(Ascii* self, re::Edx, LinkIDMap*) {
    DOM_SetSectionToFirstChild(&self->dom);
    while (DOM_IsCurrentSectionValid(&self->dom) != 0) {
        re::Ptr32<const char> className = nullptr;
        re::Ptr32<const char> section = nullptr;
        re::Ptr32<const char> name = nullptr;
        uint32_t linkID = 0;
        if (ReadObjectHeader(self, &className, &section, &name, &linkID) == 0) {
            Report(self, Str(kErrParseObject));
            return 0;
        }
        NiTFactory* factory = Factories_GetEntityCompFactory();
        if (factory == nullptr) {
            char text[0x400];
            NiSprintf(text, sizeof(text), Str(kErrNoFactory), className);
            Report(self, text);
            return 0;
        }
        CreateFn create = nullptr;
        void* object = nullptr;
        if (StringMap_GetAt(factory->creators, className, &create) != 0) {
            object = create(nullptr);
        }
        if (object == nullptr) {
            char text[0x400];
            NiSprintf(text, sizeof(text), Str(kErrNoCreator), className);
            Report(self, text);
            return 0;
        }
        NiFixedString objectName = MakeFixedString(name);
        Virtual<void>(object, prop::SetName, &objectName);
        ReleaseFixedString(&objectName);
        if (NiStricmp(section, Str(kCOMPONENT)) == 0 || NiStricmp(section, Str(kENTITY)) == 0) {
            Links_Add(&self->links, object, linkID);
        }
        DOM_SetSectionToNextSibling(&self->dom);
    }
    DOM_EndSection(&self->dom);
    return 1;
}

// ---- properties

// 0x8400F0 ReadItems(type, name, object): a collection's ITEM sections (made a collection where
// properties can be added), each read at its index (empty ones skipped but counted)
uint8_t __fastcall ReadItemsImpl(Ascii* self, re::Edx, const char* type, const char* name, void* object) {
    uint32_t index = 0;
    if (Virtual<uint8_t>(object, prop::IsAddPropertySupported) != 0) {
        NiFixedString key = MakeFixedString(name);
        Virtual<uint8_t>(object, prop::MakeCollection, &key, 1);
        ReleaseFixedString(&key);
    }
    NiDOMTool* dom = &self->dom;
    while (DOM_IsCurrentSectionValid(dom) != 0) {
        if (NiStricmp(DOM_GetSectionName(dom), Str(kITEM)) != 0) {
            DOM_SetSectionToNextSibling(dom);
            continue;
        }
        if (DOM_IsLeaf(dom) == 0 || DOM_GetAttributeCount(dom) != 0) {
            if (Reader(ReaderFor(type))(self, object, name, index) == 0) {
                return 0;
            }
        }
        DOM_SetSectionToNextSibling(dom);
        index++;
    }
    DOM_EndSection(dom);
    return 1;
}

// 0x840440 ReadPropertyValues(type, name, object): a collection (ITEM children) or one value
uint8_t __fastcall ReadPropertyValuesImpl(Ascii* self, re::Edx, const char* type, const char* name, void* object) {
    if (DOM_SetSectionTo(&self->dom, Str(kITEM)) != nullptr) {
        return ReadItems(self, type, name, object);
    }
    DOM_EndSection(&self->dom);
    return Reader(ReaderFor(type))(self, object, name, 0);
}

// 0x840A90 AddAndReadProperty(type, name, semantic type, display name, description, object): the
// property added (its result not checked), then its value read
uint8_t __fastcall AddAndReadPropertyImpl(Ascii* self, re::Edx, const char* type, const char* name, const char* semantic,
                                          const char* display, const char* description, void* object) {
    NiFixedString descriptionString = MakeFixedString(description);
    NiFixedString semanticString = MakeFixedString(semantic);
    NiFixedString typeString = MakeFixedString(type);
    NiFixedString displayString = MakeFixedString(display);
    NiFixedString nameString = MakeFixedString(name);
    Virtual<uint8_t>(object, prop::AddProperty, &nameString, &displayString, &typeString, &semanticString,
                     &descriptionString);
    ReleaseFixedString(&nameString);
    ReleaseFixedString(&displayString);
    ReleaseFixedString(&typeString);
    ReleaseFixedString(&semanticString);
    ReleaseFixedString(&descriptionString);
    return ReadPropertyValues(self, type, name, object);
}

// 0x841260 ReadProperty(object): a PROPERTY section (an object that takes new properties needs
// their semantic type, display name and description)
uint8_t __fastcall ReadPropertyImpl(Ascii* self, re::Edx, void* object) {
    const char* type = DOM_GetAttribute(&self->dom, Str(kClass));
    const char* name = DOM_GetAttribute(&self->dom, Str(kName));
    if (type == nullptr || name == nullptr) {
        Report(self, Str(kErrPropertyTag), object);
        return 0;
    }
    if (Virtual<uint8_t>(object, prop::IsAddPropertySupported) != 0) {
        const char* semantic = DOM_GetAttribute(&self->dom, Str(kSemanticType));
        const char* display = DOM_GetAttribute(&self->dom, Str(kDisplayName));
        const char* description = DOM_GetAttribute(&self->dom, Str(kDescription));
        if (semantic == nullptr || display == nullptr || description == nullptr) {
            Report(self, Str(kErrPropertyTag), object, name);
            return 0;
        }
        if (AddAndReadProperty(self, type, name, semantic, display, description, object) == 0) {
            Report(self, Str(kErrPropertyValue), object, name);
            return 0;
        }
        return 1;
    }
    if (ReadPropertyValues(self, type, name, object) == 0) {
        Report(self, Str(kErrPropertyValue), object, name);
        return 0;
    }
    return 1;
}

// ---- the second pass

// 0x83F350 ReadMasterLinkID(object): the master entity or component by its LinkID (the sscanf
// result unchecked: the ID stays as it was)
uint8_t __fastcall ReadMasterLinkIDImpl(Ascii* self, re::Edx, void* object) {
    const char* text = DOM_GetAttribute(&self->dom, Str(kMasterLinkID));
    if (text == nullptr || NiStricmp(text, Str(kNULL)) == 0) {
        return 1;
    }
    uint32_t id = uint32_t(reinterpret_cast<uintptr_t>(self));   // (the slot's old contents: the code's)
    crt::Sscanf(text, Str(kUnsignedFormat), &id);
    if (IsInterface(object, IT_ENTITYINTERFACE)) {
        Virtual<void>(object, ent::SetMasterEntity, Links_GetObject(&self->links, id));
        return 1;
    }
    if (IsInterface(object, IT_COMPONENTINTERFACE)) {
        Virtual<void>(object, comp::SetMasterComponent, Links_GetObject(&self->links, id));
        return 1;
    }
    Report(self, Str(kErrInterfaceType), object);
    return 0;
}

// 0x841EB0 ReadObjectContents(object): master, TemplateID (16 hex bytes), Hidden (entities
// only: else an error, the reading goes on), then the COMPONENT references and PROPERTY sections
uint8_t __fastcall ReadObjectContentsImpl(Ascii* self, re::Edx, void* object) {
    if (ReadMasterLinkID(self, object) == 0) {
        return 0;
    }
    NiDOMTool* dom = &self->dom;
    const char* templateText = DOM_GetAttribute(dom, Str(kTemplateID));
    if (templateText != nullptr) {
        uint32_t b[16];
        if (crt::Sscanf(templateText, Str(kTemplateIDFormat), &b[0], &b[1], &b[2], &b[3], &b[4], &b[5], &b[6], &b[7],
                        &b[8], &b[9], &b[10], &b[11], &b[12], &b[13], &b[14], &b[15]) != 16) {
            char text[0x400];
            NiSprintf(text, sizeof(text), Str(kErrTemplateID), templateText);
            Report(self, text, object);
            return 0;
        }
        uint8_t id[16];
        UniqueID_Construct(id, b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], b[10], b[11], b[12], b[13],
                           b[14], b[15]);
        Virtual<void>(object, prop::SetTemplateID, id);
    }
    const char* hidden = DOM_GetAttribute(dom, Str(kHidden));
    if (hidden != nullptr) {
        if (IsInterface(object, IT_ENTITYINTERFACE)) {
            uint8_t value = 0;
            if (NiStricmp(hidden, Str(kTRUE)) == 0) {
                value = 1;
            }
            Virtual<void>(object, ent::SetHidden, uint32_t(value));
        } else {
            Report(self, Str(kErrHidden), object);
        }
    }
    DOM_SetSectionToFirstChild(dom);
    if (DOM_IsCurrentSectionValid(dom) != 0) {
        while (DOM_IsCurrentSectionValid(dom) != 0) {
            const char* section = DOM_GetSectionName(dom);
            if (NiStricmp(section, Str(kCOMPONENT)) == 0) {
                uint32_t id = 0;
                if (ReadRefLinkID(self, &id) == 0) {
                    Report(self, Str(kErrComponentRef), object);
                    return 0;
                }
                if (id != 0xFFFFFFFF) {
                    void* component = Links_GetObject(&self->links, id);
                    if (component == nullptr) {
                        Report(self, Str(kErrInvalidComponent), object);
                        return 0;
                    }
                    if (Virtual<uint8_t>(object, ent::AddComponent, component, 0) == 0) {
                        NiFixedString componentName = NullFixedString();
                        Virtual<re::Ptr32<const char>*>(component, prop::GetName, &componentName);
                        char text[0x400];
                        NiSprintf(text, sizeof(text), Str(kErrAddComponent), componentName);
                        FixedString_Destruct(&componentName);
                        Report(self, text, object);
                        return 0;
                    }
                }
            } else if (NiStricmp(section, Str(kPROPERTY)) == 0) {
                if (ReadProperty(self, object) == 0) {
                    return 0;
                }
            }
            DOM_SetSectionToNextSibling(dom);
        }
    }
    DOM_EndSection(dom);
    return 1;
}

// 0x8424A0 LinkObjects: the second pass over OBJECTS (a section neither ENTITY nor COMPONENT
// isn't moved past: the code's endless loop)
uint8_t __fastcall LinkObjectsImpl(Ascii* self, re::Edx) {
    NiDOMTool* dom = &self->dom;
    DOM_SetSectionToFirstChild(dom);
    while (DOM_IsCurrentSectionValid(dom) != 0) {
        re::Ptr32<const char> className = nullptr;
        re::Ptr32<const char> section = nullptr;
        re::Ptr32<const char> name = nullptr;
        uint32_t linkID = 0;
        if (ReadObjectHeader(self, &className, &section, &name, &linkID) == 0) {
            Report(self, Str(kErrParseObject));
            return 0;
        }
        if (NiStricmp(section, Str(kCOMPONENT)) == 0 || NiStricmp(section, Str(kENTITY)) == 0) {
            if (ReadObjectContents(self, Links_GetObject(&self->links, linkID)) == 0) {
                return 0;
            }
            DOM_SetSectionToNextSibling(dom);
        }
    }
    DOM_EndSection(dom);
    return 1;
}

// ---- scenes

// NiScene's fields this reads (NiEntityTemplates.cpp has the class)
constexpr int kSceneEntities = 0xC;     // NiTObjectSet<NiEntityInterfaceIPtr>
constexpr int kSceneEntityMap = 0x18;   // NiTPointerMap<NiEntityInterface*, bool>
constexpr int kSetName = 0x8;
constexpr int kSetEntities = 0xC;

// 0x840D60 ReadSelections(scene): each SET a new NiEntitySelectionSet of the entities it names
// (unresolved ones reported and left out); always true. An unnamed SET is reported again and
// again: the loop doesn't move past it (the code's)
uint8_t __fastcall ReadSelectionsImpl(Ascii* self, re::Edx, void* scene) {
    NiDOMTool* dom = &self->dom;
    DOM_SetSectionToFirstChild(dom);
    while (DOM_IsCurrentSectionValid(dom) != 0) {
        if (NiStricmp(DOM_GetSectionName(dom), Str(kSET)) != 0) {
            DOM_SetSectionToNextSibling(dom);
            continue;
        }
        const char* setName = DOM_GetAttribute(dom, Str(kName));
        if (setName == nullptr) {
            Report(self, Str(kErrUnnamedSet));
            continue;
        }
        // NiEntitySelectionSet(name) inlined (vtable 0x98A914, 0x18)
        auto* set = static_cast<NiRefObject*>(NiNew(0x18));
        if (set != nullptr) {
            NiFixedString name = ni::sys::GlobalStringTable_AddString(setName);
            set->vtable = reinterpret_cast<void*>(0x008FEFD4);
            set->refCount = 0;
            ni::sys::Increment(reinterpret_cast<void*>(0x020E2168));
            set->vtable = reinterpret_cast<void*>(0x0098A914);
            At<NiFixedString>(set, kSetName) = CopyFixedString(name);
            At<uint32_t>(set, 0xC) = 0;
            At<uint32_t>(set, 0x10) = 0;
            At<uint32_t>(set, 0x14) = 0;
            ReleaseFixedString(&name);
        }
        DOM_SetSectionToFirstChild(dom);
        while (DOM_IsCurrentSectionValid(dom) != 0) {
            if (NiStricmp(DOM_GetSectionName(dom), Str(kENTITY)) != 0) {
                DOM_SetSectionToNextSibling(dom);
                continue;
            }
            const char* text = DOM_GetAttribute(dom, Str(kRefLinkID));
            void* entity = nullptr;
            bool resolved = false;
            if (text != nullptr) {
                uint32_t id = 0;
                if (NiStricmp(text, Str(kNULL)) == 0) {
                    resolved = true;
                } else if (crt::Sscanf(text, Str(kUnsignedFormat), &id) == 1) {
                    if (id == 0xFFFFFFFF) {
                        resolved = true;
                    } else {
                        entity = Links_GetObject(&self->links, id);
                        resolved = entity != nullptr;
                        AddReference(entity);
                    }
                }
            }
            if (!resolved) {
                DOM_SetSectionToNextSibling(dom);
                char message[0x400];
                NiSprintf(message, sizeof(message), Str(kErrSetEntity), At<NiFixedString>(set, kSetName));
                Report(self, message);
                continue;
            }
            AddEntity(&At<NiTSet<void*>>(set, kSetEntities), entity);
            RemoveReference(entity);
            DOM_SetSectionToNextSibling(dom);
        }
        DOM_EndSection(dom);
        Scene_AddSelectionSet(scene, set);
        DOM_SetSectionToNextSibling(dom);
    }
    DOM_EndSection(dom);
    return 1;
}

// 0x841C10 ReadScene(scene): its ENTITY references (NiScene::AddEntity inlined: the set and the
// lookup map; "NULL" adds a null) and SELECTIONS
uint8_t __fastcall ReadSceneImpl(Ascii* self, re::Edx, void* scene) {
    NiDOMTool* dom = &self->dom;
    DOM_SetSectionToFirstChild(dom);
    while (DOM_IsCurrentSectionValid(dom) != 0) {
        const char* section = DOM_GetSectionName(dom);
        if (NiStricmp(section, Str(kENTITY)) == 0) {
            const char* text = DOM_GetAttribute(dom, Str(kRefLinkID));
            if (text == nullptr) {
                Report(self, Str(kErrUnresolvedEntity));
                return 0;
            }
            void* entity = nullptr;
            if (NiStricmp(text, Str(kNULL)) != 0) {
                uint32_t id = 0;
                if (crt::Sscanf(text, Str(kUnsignedFormat), &id) != 1) {
                    Report(self, Str(kErrUnresolvedEntity));
                    return 0;
                }
                if (id != 0xFFFFFFFF) {
                    entity = Links_GetObject(&self->links, id);
                    if (entity == nullptr) {
                        Report(self, Str(kErrUnresolvedEntity));
                        return 0;
                    }
                    AddReference(entity);
                }
            }
            AddEntity(&At<NiTSet<void*>>(scene, kSceneEntities), entity);
            RemoveReference(entity);
            BoolPointerMap_SetAt(&At<PointerMap>(scene, kSceneEntityMap), entity, 1);
        } else if (NiStricmp(section, Str(kSELECTIONS)) == 0) {
            if (ReadSelections(self, scene) == 0) {
                return 0;
            }
        }
        DOM_SetSectionToNextSibling(dom);
    }
    DOM_EndSection(dom);
    return 1;
}

// 0x842660 ReadScenes: each SCENE (Class "NiScene" only) a new NiScene given to the streaming
// object, then read
uint8_t __fastcall ReadScenesImpl(Ascii* self, re::Edx) {
    NiDOMTool* dom = &self->dom;
    DOM_SetSectionToFirstChild(dom);
    if (DOM_IsCurrentSectionValid(dom) == 0) {
        Report(self, Str(kErrNoScenes));
        return 0;
    }
    while (DOM_IsCurrentSectionValid(dom) != 0) {
        if (NiStricmp(DOM_GetSectionName(dom), Str(kSCENE)) == 0) {
            const char* className = DOM_GetAttribute(dom, Str(kClass));
            const char* name = DOM_GetAttribute(dom, Str(kName));
            if (className == nullptr || name == nullptr) {
                Report(self, Str(kErrSceneTag));
                return 0;
            }
            if (NiStricmp(className, Str(kNiScene)) != 0) {
                Report(self, Str(kErrNonNiScene));
                return 0;
            }
            void* scene = NiNew(0x34);
            if (scene != nullptr) {
                NiFixedString sceneName = ni::sys::GlobalStringTable_AddString(name);
                scene = Scene_Construct(scene, &sceneName, 0, 0);
                ReleaseFixedString(&sceneName);
            }
            Streaming_InsertScene(self, scene);
            if (ReadScene(self, scene) == 0) {
                return 0;
            }
        }
        DOM_SetSectionToNextSibling(dom);
    }
    DOM_EndSection(dom);
    return 1;
}

// 0x842AF0 ReadDocument: the GSA section, its version, then OBJECTS (both passes) and SCENES
uint8_t __fastcall ReadDocumentImpl(Ascii* self, re::Edx) {
    NiDOMTool* dom = &self->dom;
    if (DOM_ResetSectionTo(dom, Str(kGSA)) == nullptr) {
        Report(self, Str(kErrNoGsaSection));
        return 0;
    }
    if (ReadVersion(self) == 0) {
        Report(self, Str(kErrVersion));
        return 0;
    }
    DOM_SetSectionToFirstChild(dom);
    if (DOM_IsCurrentSectionValid(dom) == 0) {
        Report(self, Str(kErrEmpty));
        return 0;
    }
    while (DOM_IsCurrentSectionValid(dom) != 0) {
        const char* section = DOM_GetSectionName(dom);
        if (NiStricmp(section, Str(kOBJECTS)) == 0) {
            if (ReadObjects(self, &self->links) == 0) {
                return 0;
            }
            if (LinkObjects(self) == 0) {
                return 0;
            }
        } else if (NiStricmp(section, Str(kSCENES)) == 0) {
            if (ReadScenes(self) == 0) {
                return 0;
            }
        }
        DOM_SetSectionToNextSibling(dom);
    }
    DOM_EndSection(dom);
    return 1;
}

// 0x843010 Load(file): the scenes it held dropped, the file parsed and read; everything reset after
uint8_t __fastcall Load(Ascii* self, re::Edx, const char* file) {
    DOM_Close(&self->dom);
    Links_RemoveAll(&self->links);
    crt::StrcpyS(self->path, sizeof(self->path), Str(kEmpty));
    if (SetFilename(self, file) == 0) {
        return 0;
    }
    Streaming_RemoveAllScenes(self);
    DOM_Init(&self->dom, file);
    if (DOM_LoadFile(&self->dom) == 0) {
        DOM_Close(&self->dom);
        Links_RemoveAll(&self->links);
        crt::StrcpyS(self->path, sizeof(self->path), Str(kEmpty));
        Report(self, Str(kErrXml));
        return 0;
    }
    if (ReadDocument(self) == 0) {
        Reset(self);
        Streaming_RemoveAllScenes(self);
        return 0;
    }
    Reset(self);
    return 1;
}

// the reader instantiations' DOM readers, by pointer
inline uint8_t(__thiscall* const kDomReadFloat)(NiDOMTool*, float*) = DOM_ReadFloat;
inline uint8_t(__thiscall* const kDomReadBool)(NiDOMTool*, uint8_t*) = DOM_ReadBool;
inline uint8_t(__thiscall* const kDomReadInt)(NiDOMTool*, int32_t*) = DOM_ReadInt;
inline uint8_t(__thiscall* const kDomReadUInt)(NiDOMTool*, uint32_t*) = DOM_ReadUInt;
inline uint8_t(__thiscall* const kDomReadShort)(NiDOMTool*, int16_t*) = DOM_ReadShort;
inline uint8_t(__thiscall* const kDomReadUShort)(NiDOMTool*, uint16_t*) = DOM_ReadUShort;

} // namespace

#define NIGSA_REVERSED(name, address, fn)                                                      \
    static void* RE_CONCAT(Original_, __LINE__);                                                \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIGSA_REVERSED("NiEntityStreamingAscii::ReadRefLinkID", 0x0083DCF0, &ReadRefLinkIDImpl);
NIGSA_REVERSED("NiEntityStreamingAscii::ReadObjectHeader", 0x0083DD50, &ReadObjectHeaderImpl);
NIGSA_REVERSED("NiEntityStreamingAscii::ReadVersion", 0x0083DDE0, &ReadVersionImpl);
NIGSA_REVERSED("NiEntityStreamingAscii::Reset", 0x0083E4D0, &ResetImpl);
NIGSA_REVERSED("NiEntityStreamingAscii::ReadEntityPointer", 0x0083E690, &ReadEntityPointerImpl);
NIGSA_REVERSED("NiEntityStreamingAscii::ReadObjectPointer", 0x0083E770, &ReadObjectPointerImpl);
NIGSA_REVERSED("NiEntityStreamingAscii::ReadFloat", 0x0083E7F0, (&ReadValue<float, &kDomReadFloat, prop::SetFloat>));
NIGSA_REVERSED("NiEntityStreamingAscii::ReadBool", 0x0083E890, (&ReadValue<uint8_t, &kDomReadBool, prop::SetBool>));
NIGSA_REVERSED("NiEntityStreamingAscii::ReadInt", 0x0083E930, (&ReadValue<int32_t, &kDomReadInt, prop::SetInt>));
NIGSA_REVERSED("NiEntityStreamingAscii::ReadUInt", 0x0083E9D0, (&ReadValue<uint32_t, &kDomReadUInt, prop::SetUInt>));
NIGSA_REVERSED("NiEntityStreamingAscii::ReadShort", 0x0083EA70, (&ReadValue<int16_t, &kDomReadShort, prop::SetShort>));
NIGSA_REVERSED("NiEntityStreamingAscii::ReadUShort", 0x0083EB10,
               (&ReadValue<uint16_t, &kDomReadUShort, prop::SetUShort>));
NIGSA_REVERSED("NiEntityStreamingAscii::ReadString", 0x0083EBB0, &ReadStringImpl);
NIGSA_REVERSED("NiEntityStreamingAscii::ReadPoint2", 0x0083EDB0, (&ReadFloats<2, &DOM_ReadPoint2, prop::SetPoint2>));
NIGSA_REVERSED("NiEntityStreamingAscii::ReadPoint3", 0x0083EE50, (&ReadFloats<3, &DOM_ReadPoint3, prop::SetPoint3>));
NIGSA_REVERSED("NiEntityStreamingAscii::ReadQuaternion", 0x0083EEF0,
               (&ReadFloats<4, &DOM_ReadColorA, prop::SetQuaternion>));
NIGSA_REVERSED("NiEntityStreamingAscii::ReadMatrix3", 0x0083EF90, (&ReadFloats<9, &DOM_ReadMatrix3, prop::SetMatrix3>));
NIGSA_REVERSED("NiEntityStreamingAscii::ReadColor", 0x0083F030, (&ReadFloats<3, &DOM_ReadPoint3, prop::SetColor>));
NIGSA_REVERSED("NiEntityStreamingAscii::ReadColorA", 0x0083F0E0, (&ReadFloats<4, &DOM_ReadColorA, prop::SetColorA>));
NIGSA_REVERSED("NiEntityStreamingAscii::~NiEntityStreamingAscii", 0x0083F2A0, &DestructImpl);
NIGSA_REVERSED("NiEntityStreamingAscii::GetFileExtension", 0x0083F2F0, &GetFileExtension);
NIGSA_REVERSED("NiEntityStreamingAscii::GetFileDescription", 0x0083F320, &GetFileDescription);
NIGSA_REVERSED("NiEntityStreamingAscii::ReadMasterLinkID", 0x0083F350, &ReadMasterLinkIDImpl);
NIGSA_REVERSED("NiEntityStreamingAscii::ReadData", 0x0083F9F0, &ReadDataImpl);
NIGSA_REVERSED("NiEntityStreamingAscii::_SDMInit", 0x0083FA90, &SDMInit);
NIGSA_REVERSED("NiEntityStreamingAscii::_SDMShutdown", 0x0083FAB0, &SDMShutdown);
NIGSA_REVERSED("NiEntityStreamingAscii::NiEntityStreamingAscii", 0x0083FB50, &Construct);
NIGSA_REVERSED("NiEntityStreamingAscii::DeletingDestructor", 0x0083FB80, &DeletingDestructor);
NIGSA_REVERSED("NiEntityStreamingAscii::SetFilename", 0x0083FBB0, &SetFilenameImpl);
NIGSA_REVERSED("NiEntityStreamingAscii::ReadObjects", 0x0083FC30, &ReadObjectsImpl);
NIGSA_REVERSED("NiEntityStreamingAscii::ReadItems", 0x008400F0, &ReadItemsImpl);
NIGSA_REVERSED("NiEntityStreamingAscii::ReadPropertyValues", 0x00840440, &ReadPropertyValuesImpl);
NIGSA_REVERSED("NiEntityStreamingAscii::AddAndReadProperty", 0x00840A90, &AddAndReadPropertyImpl);
NIGSA_REVERSED("NiEntityStreamingAscii::ReadSelections", 0x00840D60, &ReadSelectionsImpl);
NIGSA_REVERSED("NiEntityStreamingAscii::ReadProperty", 0x00841260, &ReadPropertyImpl);
NIGSA_REVERSED("NiEntityStreamingAscii::ReadScene", 0x00841C10, &ReadSceneImpl);
NIGSA_REVERSED("NiEntityStreamingAscii::ReadObjectContents", 0x00841EB0, &ReadObjectContentsImpl);
NIGSA_REVERSED("NiEntityStreamingAscii::LinkObjects", 0x008424A0, &LinkObjectsImpl);
NIGSA_REVERSED("NiEntityStreamingAscii::ReadScenes", 0x00842660, &ReadScenesImpl);
NIGSA_REVERSED("NiEntityStreamingAscii::ReadDocument", 0x00842AF0, &ReadDocumentImpl);
NIGSA_REVERSED("NiEntityStreamingAscii::Load", 0x00843010, &Load);

} // namespace ni::entity
