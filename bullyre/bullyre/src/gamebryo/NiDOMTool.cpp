// NiDOMTool (8 bytes: the TinyXML document, the stack of open sections), what the GSA scene
// files are read and written with (NiEntityStreamingAscii, the other pass's), and the few
// TinyXML members linked in next to it (TiXmlComment's and TiXmlText's constructors and
// deleting destructors, TiXmlString's assignment). The "current section" is the top of the
// stack, or the document's root element when it's empty; values are the sections' text
// ("TRUE" / "FALSE", sscanf / NiSprintf formats, "%.16f" for the floats, three "ROW"
// elements for a matrix, base64 for data).
//
// TinyXML as linked (an older 2.x: 5-slot vtables): a node (0x2C) is the vtable, the location
// (+0x4), the user data (+0xC), the parent (+0x10), the type (+0x14: 2 comment, 4 text), the
// first and last child (+0x18, +0x1C), the value (+0x20, a TiXmlString: a pointer to {size,
// capacity, characters}), the previous and next sibling (+0x24, +0x28); a text's CDATA flag
// +0x2C (0x30); an element (0x50) has its attribute set's sentinel at +0x2C.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni {
namespace {

template <typename T>
struct PtrSet {
    re::Ptr32<re::Field<T>> base; // +0x0
    uint32_t alloced;   // +0x4
    uint32_t used;      // +0x8
};
static_assert(sizeof(PtrSet<void*>) == 0xC);

// (NiEntity.h's ni::entity::NiDOMTool, the other pass's: the same layout)
struct DOMTool {
    re::Ptr32<void> document;                // +0x0 (TiXmlDocument, 0x48)
    re::Ptr32<PtrSet<void*>> sections;       // +0x4 (NiTPrimitiveSet<TiXmlElement*>, NiNew'd)
};
static_assert(sizeof(DOMTool) == 8);

constexpr uintptr_t TiXmlComment_vtable = 0x0098C214;
constexpr uintptr_t TiXmlText_vtable = 0x0098C228;
constexpr int kNodeComment = 2;
constexpr int kNodeText = 4;

inline const char* Str(uintptr_t address) {
    return reinterpret_cast<const char*>(address);
}

// the game's operator new / delete
GAME_FN(void*, __cdecl, OperatorNew, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, OperatorDelete, 0x005EEAB0, void* p);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(int, __cdecl, NiStricmp, 0x00753820, const char* a, const char* b);
GAME_FN(void, __thiscall, PtrSet_Realloc, 0x0083AD20, PtrSet<void*>* self, uint32_t size);
GAME_FN(re::Ptr32<const char>*, __thiscall, FixedStringSetText, 0x00406730, re::Ptr32<const char>* self, const char* text);
// base64 (the encoded text from the heap: OperatorDelete frees it; the decoder fills the size)
GAME_FN(char*, __cdecl, Base64Encode, 0x00856D70, const void* data, uint32_t size, int unknown);
GAME_FN(uint8_t, __cdecl, Base64Decode, 0x00856F50, void* buffer, const char* text, uint32_t* size);

// TinyXML's
GAME_FN(void*, __thiscall, TiXmlString_Assign, 0x008570E0, void* self, const char* text, uint32_t length);
GAME_FN(void, __thiscall, TiXmlNode_Construct, 0x00857980, void* self, int type);
GAME_FN(void, __thiscall, TiXmlNode_Destruct, 0x008579C0, void* self);
GAME_FN(void*, __thiscall, TiXmlNode_LinkEndChild, 0x008573C0, void* self, void* child);
GAME_FN(void*, __thiscall, TiXmlNode_FirstChildElement, 0x00857490, void* self);
GAME_FN(void*, __thiscall, TiXmlNode_FirstChildElementNamed, 0x008574C0, void* self, const char* name);
GAME_FN(void*, __thiscall, TiXmlNode_NextSiblingElement, 0x00857500, void* self);
GAME_FN(const char*, __thiscall, TiXmlElement_GetText, 0x00857590, void* self);
GAME_FN(void*, __thiscall, TiXmlAttribute_Next, 0x00857730, void* self);
GAME_FN(const char*, __thiscall, TiXmlElement_Attribute, 0x00857A90, void* self, const char* name);
GAME_FN(void, __thiscall, TiXmlElement_SetAttribute, 0x00857AB0, void* self, const char* name, const char* value);
GAME_FN(void, __thiscall, TiXmlElement_SetAttributeInt, 0x00858AE0, void* self, const char* name, int value);
GAME_FN(void*, __thiscall, TiXmlElement_Construct, 0x00858A50, void* self, const char* name);
GAME_FN(void*, __thiscall, TiXmlDeclaration_Construct, 0x00858600, void* self, const char* version,
        const char* encoding, const char* standalone);
GAME_FN(void*, __thiscall, TiXmlDocument_Construct, 0x00857EC0, void* self, const char* file);
GAME_FN(uint8_t, __thiscall, TiXmlDocument_LoadFile, 0x00858B70, void* self, int encoding);
GAME_FN(uint8_t, __thiscall, TiXmlDocument_SaveFile, 0x00857F60, void* self);

// this file's, through their addresses
GAME_FN(void*, __thiscall, TiXmlText_Construct, 0x00854CB0, void* self, const char* text);
GAME_FN(void*, __thiscall, DOM_SetSectionTo_, 0x00855560, DOMTool* self, const char* name);

inline uint32_t Strlen(const char* s) {
    const char* p = s;
    while (*p != 0) {
        p++;
    }
    return uint32_t(p - s);
}

// the open section (the top of the stack), or the document's root element
inline void* CurrentSection(DOMTool* self) {
    PtrSet<void*>* sections = self->sections;
    if (sections->used == 0) {
        return TiXmlNode_FirstChildElement(self->document);
    }
    return sections->base[sections->used - 1];
}

// the parent a new section goes under: the open one, or the document itself
inline void* ParentSection(DOMTool* self) {
    PtrSet<void*>* sections = self->sections;
    return sections->used != 0 ? sections->base[sections->used - 1] : self->document;
}

// NiTPrimitiveSet::Add
inline void PushSection(PtrSet<void*>* sections, void* element) {
    if (sections->used == sections->alloced) {
        PtrSet_Realloc(sections, sections->alloced != 0 ? sections->alloced * 2 : 1);
    }
    sections->base[sections->used] = element;
    sections->used++;
}

// NiTPrimitiveSet::RemoveAt(used - 1) (the last moved onto itself)
inline void PopSection(PtrSet<void*>* sections) {
    uint32_t old = sections->used;
    sections->used--;
    sections->base[old - 1] = sections->base[sections->used];
}

// new TiXmlText(text), inlined
inline void* NewText(const char* text) {
    void* node = OperatorNew(0x30);
    if (node == nullptr) {
        return nullptr;
    }
    TiXmlNode_Construct(node, kNodeText);
    *static_cast<uint32_t*>(node) = TiXmlText_vtable;
    TiXmlString_Assign(static_cast<uint8_t*>(node) + 0x20, text, Strlen(text));
    *(static_cast<uint8_t*>(node) + 0x2C) = 0;
    return node;
}

// ---- TinyXML's

// TiXmlString::operator=(const char*)
void* __fastcall TiXmlStringAssignText(void* self, re::Edx, const char* text) {
    return TiXmlString_Assign(self, text, Strlen(text));
}

void* __fastcall TiXmlCommentConstruct(void* self, re::Edx) {
    TiXmlNode_Construct(self, kNodeComment);
    *static_cast<uint32_t*>(self) = TiXmlComment_vtable;
    return self;
}

void* __fastcall TiXmlCommentDeletingDestructor(void* self, re::Edx, int flags) {
    *static_cast<uint32_t*>(self) = TiXmlComment_vtable;
    TiXmlNode_Destruct(self);
    if ((flags & 1) != 0) {
        OperatorDelete(self);
    }
    return self;
}

void* __fastcall TiXmlTextConstruct(void* self, re::Edx, const char* text) {
    TiXmlNode_Construct(self, kNodeText);
    *static_cast<uint32_t*>(self) = TiXmlText_vtable;
    TiXmlString_Assign(static_cast<uint8_t*>(self) + 0x20, text, Strlen(text));
    *(static_cast<uint8_t*>(self) + 0x2C) = 0;
    return self;
}

void* __fastcall TiXmlTextDeletingDestructor(void* self, re::Edx, int flags) {
    *static_cast<uint32_t*>(self) = TiXmlText_vtable;
    TiXmlNode_Destruct(self);
    if ((flags & 1) != 0) {
        OperatorDelete(self);
    }
    return self;
}

// ---- the document

uint8_t __fastcall LoadFile(DOMTool* self, re::Edx) {
    return TiXmlDocument_LoadFile(self->document, 0);
}

uint8_t __fastcall SaveFile(DOMTool* self, re::Edx) {
    return TiXmlDocument_SaveFile(self->document);
}

// the <?xml version="1.0"?> declaration
uint8_t __fastcall WriteHeader(DOMTool* self, re::Edx) {
    if (self->document == nullptr) {
        return 0;
    }
    void* memory = OperatorNew(0x38);
    void* declaration = memory != nullptr ? TiXmlDeclaration_Construct(memory, Str(0x0098C210), Str(0x008FEFBB),
                                                                       Str(0x008FEFBB))
                                          : nullptr;
    TiXmlNode_LinkEndChild(self->document, declaration);
    return 1;
}

// the document and the stack freed
void __fastcall Close(DOMTool* self, re::Edx) {
    PtrSet<void*>* sections = self->sections;
    if (sections != nullptr) {
        NiFree(sections->base);
        NiDelete(sections, sizeof(PtrSet<void*>));
    }
    void* document = self->document;
    if (document != nullptr) {
        re::PtrCast<void(__thiscall*)(void*, int)>(VtableOf(document)[0])(document, 1);
    }
    self->document = nullptr;
    self->sections = nullptr;
}

// a new document for the file, an empty stack
void __fastcall Init(DOMTool* self, re::Edx, const char* file) {
    PtrSet<void*>* sections = self->sections;
    if (sections != nullptr) {
        NiFree(sections->base);
        NiDelete(sections, sizeof(PtrSet<void*>));
    }
    void* document = self->document;
    if (document != nullptr) {
        re::PtrCast<void(__thiscall*)(void*, int)>(VtableOf(document)[0])(document, 1);
    }
    self->sections = nullptr;
    self->document = nullptr;
    void* memory = OperatorNew(0x48);
    self->document = memory != nullptr ? TiXmlDocument_Construct(memory, file) : nullptr;
    PtrSet<void*>* set = static_cast<PtrSet<void*>*>(NiNew(sizeof(PtrSet<void*>)));
    if (set != nullptr) {
        set->base = nullptr;
        set->alloced = 0;
        set->used = 0;
    }
    self->sections = set;
}

// ---- the sections

uint8_t __fastcall IsCurrentSectionValid(DOMTool* self, re::Edx) {
    return CurrentSection(self) != nullptr ? 1 : 0;
}

uint8_t __fastcall EndSection(DOMTool* self, re::Edx) {
    if (self->document == nullptr) {
        return 0;
    }
    PopSection(self->sections);
    return 1;
}

// a new element under the open section (or the document), opened
uint8_t __fastcall BeginSection(DOMTool* self, re::Edx, const char* name) {
    if (self->document == nullptr) {
        return 0;
    }
    void* memory = OperatorNew(0x50);
    void* element = memory != nullptr ? TiXmlElement_Construct(memory, name) : nullptr;
    TiXmlNode_LinkEndChild(ParentSection(self), element);
    PushSection(self->sections, element);
    return 1;
}

// the open section's (or the document's) first child of that name opened (even if none)
void* __fastcall SetSectionTo(DOMTool* self, re::Edx, const char* name) {
    if (self->document == nullptr) {
        return nullptr;
    }
    void* element = TiXmlNode_FirstChildElementNamed(ParentSection(self), name);
    PushSection(self->sections, element);
    return element;
}

// from the top again
void* __fastcall ResetSectionTo(DOMTool* self, re::Edx, const char* name) {
    if (self->document == nullptr) {
        return nullptr;
    }
    self->sections->used = 0;
    return DOM_SetSectionTo_(self, name);
}

void* __fastcall SetSectionToFirstChild(DOMTool* self, re::Edx) {
    void* element = TiXmlNode_FirstChildElement(CurrentSection(self));
    PushSection(self->sections, element);
    return element;
}

// the open section replaced by its next sibling element (even if none)
void* __fastcall SetSectionToNextSibling(DOMTool* self, re::Edx) {
    void* next = TiXmlNode_NextSiblingElement(CurrentSection(self));
    if (self->document != nullptr) {
        PopSection(self->sections);
    }
    PushSection(self->sections, next);
    return next;
}

uint8_t __fastcall SetAttributeUInt(DOMTool* self, re::Edx, const char* name, uint32_t value) {
    void* section = CurrentSection(self);
    if (section == nullptr) {
        return 0;
    }
    TiXmlElement_SetAttributeInt(section, name, int(value));
    return 1;
}

uint8_t __fastcall SetAttribute(DOMTool* self, re::Edx, const char* name, const char* value) {
    void* section = CurrentSection(self);
    if (section == nullptr) {
        return 0;
    }
    TiXmlElement_SetAttribute(section, name, value);
    return 1;
}

uint32_t __fastcall GetAttributeCount(DOMTool* self, re::Edx) {
    uint8_t* sentinel = static_cast<uint8_t*>(CurrentSection(self)) + 0x2C;
    uint32_t count = 0;
    void* attribute = *reinterpret_cast<re::Ptr32<void>*>(sentinel + 0x20);
    if (attribute == sentinel || attribute == nullptr) {
        return 0;
    }
    do {
        count++;
        attribute = TiXmlAttribute_Next(attribute);
    } while (attribute != nullptr);
    return count;
}

const char* __fastcall GetAttribute(DOMTool* self, re::Edx, const char* name) {
    return TiXmlElement_Attribute(CurrentSection(self), name);
}

// the open section's name (its value's characters)
const char* __fastcall GetSectionName(DOMTool* self, re::Edx) {
    uint8_t* section = static_cast<uint8_t*>(CurrentSection(self));
    return *reinterpret_cast<re::Ptr32<const char>*>(section + 0x20) + 8;
}

uint8_t __fastcall IsLeaf(DOMTool* self, re::Edx) {
    uint8_t* section = static_cast<uint8_t*>(CurrentSection(self));
    return *reinterpret_cast<re::Ptr32<void>*>(section + 0x18) == nullptr ? 1 : 0;
}

// ---- the values written (a text child of the open section)

uint8_t __fastcall WriteBool(DOMTool* self, re::Edx, const uint8_t* value) {
    void* section = CurrentSection(self);
    void* memory = OperatorNew(0x30);
    void* text = nullptr;
    if (memory != nullptr) {
        text = TiXmlText_Construct(memory, *value != 0 ? Str(0x00923004) : Str(0x008FFF70));   // "TRUE" / "FALSE"
    }
    TiXmlNode_LinkEndChild(section, text);
    return 1;
}

uint8_t __fastcall WriteString(DOMTool* self, re::Edx, const re::Ptr32<const char>* value) {
    void* section = CurrentSection(self);
    void* text = NewText(*value);
    TiXmlNode_LinkEndChild(section, text);
    return 1;
}

uint8_t __fastcall WriteData(DOMTool* self, re::Edx, const void* data, uint32_t size) {
    void* section = CurrentSection(self);
    char* encoded = Base64Encode(data, size, 0);
    void* text = NewText(encoded);
    TiXmlNode_LinkEndChild(section, text);
    OperatorDelete(encoded);
    return 1;
}

uint8_t __fastcall WriteFloat(DOMTool* self, re::Edx, const float* value) {
    char buffer[0x100];
    void* section = CurrentSection(self);
    NiSprintf(buffer, sizeof(buffer), Str(0x0098C26C), double(*value));   // "%.16f"
    TiXmlNode_LinkEndChild(section, NewText(buffer));
    return 1;
}

uint8_t __fastcall WriteInt(DOMTool* self, re::Edx, const int32_t* value) {
    char buffer[0x100];
    void* section = CurrentSection(self);
    NiSprintf(buffer, sizeof(buffer), Str(0x008FFF78), *value);   // "%d"
    TiXmlNode_LinkEndChild(section, NewText(buffer));
    return 1;
}

uint8_t __fastcall WriteUInt(DOMTool* self, re::Edx, const uint32_t* value) {
    char buffer[0x100];
    void* section = CurrentSection(self);
    NiSprintf(buffer, sizeof(buffer), Str(0x0092305C), *value);   // "%u"
    TiXmlNode_LinkEndChild(section, NewText(buffer));
    return 1;
}

uint8_t __fastcall WriteShort(DOMTool* self, re::Edx, const int16_t* value) {
    char buffer[0x100];
    void* section = CurrentSection(self);
    NiSprintf(buffer, sizeof(buffer), Str(0x0098C23C), int(*value));   // "%i"
    TiXmlNode_LinkEndChild(section, NewText(buffer));
    return 1;
}

uint8_t __fastcall WriteUShort(DOMTool* self, re::Edx, const uint16_t* value) {
    char buffer[0x100];
    void* section = CurrentSection(self);
    NiSprintf(buffer, sizeof(buffer), Str(0x0098C23C), int(*value));   // "%i"
    TiXmlNode_LinkEndChild(section, NewText(buffer));
    return 1;
}

uint8_t __fastcall WritePoint2(DOMTool* self, re::Edx, const float* value) {
    char buffer[0x100];
    void* section = CurrentSection(self);
    NiSprintf(buffer, sizeof(buffer), Str(0x0098C274), double(value[0]), double(value[1]));
    TiXmlNode_LinkEndChild(section, NewText(buffer));
    return 1;
}

// (also the quaternion's)
uint8_t __fastcall WriteColorA(DOMTool* self, re::Edx, const float* value) {
    char buffer[0x100];
    void* section = CurrentSection(self);
    NiSprintf(buffer, sizeof(buffer), Str(0x0098C298), double(value[0]), double(value[1]), double(value[2]),
              double(value[3]));
    TiXmlNode_LinkEndChild(section, NewText(buffer));
    return 1;
}

// three "ROW" elements, a row of the matrix in each
uint8_t __fastcall WriteMatrix3(DOMTool* self, re::Edx, const float* value) {
    char buffer[0x100];
    void* section = CurrentSection(self);
    const float* row = value;
    for (int i = 3; i != 0; i--) {
        void* memory = OperatorNew(0x50);
        void* element = memory != nullptr ? TiXmlElement_Construct(memory, Str(0x008FFF98)) : nullptr;   // "ROW"
        TiXmlNode_LinkEndChild(section, element);
        NiSprintf(buffer, sizeof(buffer), Str(0x0098C284), double(row[0]), double(row[1]), double(row[2]));
        TiXmlNode_LinkEndChild(element, NewText(buffer));
        row += 3;
    }
    return 1;
}

// (also the colour's)
uint8_t __fastcall WritePoint3(DOMTool* self, re::Edx, const float* value) {
    char buffer[0x100];
    void* section = CurrentSection(self);
    NiSprintf(buffer, sizeof(buffer), Str(0x0098C284), double(value[0]), double(value[1]), double(value[2]));
    TiXmlNode_LinkEndChild(section, NewText(buffer));
    return 1;
}

// ---- the values read (the open section's text)

uint8_t __fastcall ReadFloat(DOMTool* self, re::Edx, float* value) {
    const char* text = TiXmlElement_GetText(CurrentSection(self));
    return crt::Sscanf(text, Str(0x008FFF7C), value) == 1 ? 1 : 0;   // "%f"
}

// "TRUE" or "FALSE" (any case)
uint8_t __fastcall ReadBool(DOMTool* self, re::Edx, uint8_t* value) {
    const char* text = TiXmlElement_GetText(CurrentSection(self));
    if (NiStricmp(text, Str(0x00923004)) == 0) {
        *value = 1;
        return 1;
    }
    if (NiStricmp(text, Str(0x008FFF70)) == 0) {
        *value = 0;
        return 1;
    }
    return 0;
}

uint8_t __fastcall ReadInt(DOMTool* self, re::Edx, int32_t* value) {
    const char* text = TiXmlElement_GetText(CurrentSection(self));
    return crt::Sscanf(text, Str(0x0098C23C), value) == 1 ? 1 : 0;   // "%i"
}

uint8_t __fastcall ReadUInt(DOMTool* self, re::Edx, uint32_t* value) {
    const char* text = TiXmlElement_GetText(CurrentSection(self));
    return crt::Sscanf(text, Str(0x0092305C), value) == 1 ? 1 : 0;   // "%u"
}

uint8_t __fastcall ReadShort(DOMTool* self, re::Edx, int16_t* value) {
    const char* text = TiXmlElement_GetText(CurrentSection(self));
    return crt::Sscanf(text, Str(0x0098C240), value) == 1 ? 1 : 0;   // "%hd"
}

uint8_t __fastcall ReadUShort(DOMTool* self, re::Edx, uint16_t* value) {
    const char* text = TiXmlElement_GetText(CurrentSection(self));
    return crt::Sscanf(text, Str(0x0098C244), value) == 1 ? 1 : 0;   // "%hu"
}

uint8_t __fastcall ReadPoint2(DOMTool* self, re::Edx, float* value) {
    const char* text = TiXmlElement_GetText(CurrentSection(self));
    return crt::Sscanf(text, Str(0x0098C248), &value[0], &value[1]) == 2 ? 1 : 0;   // "%f, %f"
}

// (also the quaternion's)
uint8_t __fastcall ReadColorA(DOMTool* self, re::Edx, float* value) {
    const char* text = TiXmlElement_GetText(CurrentSection(self));
    return crt::Sscanf(text, Str(0x0098C250), &value[0], &value[1], &value[2], &value[3]) == 4 ? 1 : 0;
}

// the three "ROW" children (stops at the first that doesn't read; the rows before it written)
uint8_t __fastcall ReadMatrix3(DOMTool* self, re::Edx, float* value) {
    void* row = TiXmlNode_FirstChildElement(CurrentSection(self));
    float* out = value;
    for (int i = 0; i < 3; i++) {
        float a, b, c;
        const char* text = TiXmlElement_GetText(row);
        if (crt::Sscanf(text, Str(0x008FFFB0), &a, &b, &c) != 3) {   // "%f, %f, %f"
            return 0;
        }
        out[0] = a;
        out[1] = b;
        out[2] = c;
        out += 3;
        row = TiXmlNode_NextSiblingElement(row);
    }
    return 1;
}

// (also the colour's)
uint8_t __fastcall ReadPoint3(DOMTool* self, re::Edx, float* value) {
    const char* text = TiXmlElement_GetText(CurrentSection(self));
    return crt::Sscanf(text, Str(0x008FFFB0), &value[0], &value[1], &value[2]) == 3 ? 1 : 0;
}

// base64 into a new NiMalloc'd block (as long as the text: the decoded size the decoder's)
uint8_t __fastcall ReadData(DOMTool* self, re::Edx, re::Ptr32<void>* data, uint32_t* size) {
    const char* text = TiXmlElement_GetText(CurrentSection(self));
    void* buffer = NiMalloc(Strlen(text));
    *data = buffer;
    return Base64Decode(buffer, text, size);
}

uint8_t __fastcall ReadString(DOMTool* self, re::Edx, re::Ptr32<const char>* value) {
    const char* text = TiXmlElement_GetText(CurrentSection(self));
    FixedStringSetText(value, text);
    return 1;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIDOMTOOL_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIDOMTOOL_REVERSED("NiDOMTool::LoadFile", 0x00854BC0, &ni::LoadFile);
NIDOMTOOL_REVERSED("NiDOMTool::SaveFile", 0x00854BD0, &ni::SaveFile);
NIDOMTOOL_REVERSED("NiDOMTool::WriteHeader", 0x00854BE0, &ni::WriteHeader);
NIDOMTOOL_REVERSED("TiXmlString::operator=(const char*)", 0x00854C30, &ni::TiXmlStringAssignText);
NIDOMTOOL_REVERSED("TiXmlComment::TiXmlComment", 0x00854C60, &ni::TiXmlCommentConstruct);
NIDOMTOOL_REVERSED("TiXmlComment::DeletingDestructor", 0x00854C80, &ni::TiXmlCommentDeletingDestructor);
NIDOMTOOL_REVERSED("TiXmlText::TiXmlText", 0x00854CB0, &ni::TiXmlTextConstruct);
NIDOMTOOL_REVERSED("TiXmlText::DeletingDestructor", 0x00854CF0, &ni::TiXmlTextDeletingDestructor);
NIDOMTOOL_REVERSED("NiDOMTool::IsCurrentSectionValid", 0x00854D20, &ni::IsCurrentSectionValid);
NIDOMTOOL_REVERSED("NiDOMTool::EndSection", 0x00854D50, &ni::EndSection);
NIDOMTOOL_REVERSED("NiDOMTool::SetAttribute(unsigned int)", 0x00854D80, &ni::SetAttributeUInt);
NIDOMTOOL_REVERSED("NiDOMTool::SetAttribute(const char*)", 0x00854DC0, &ni::SetAttribute);
NIDOMTOOL_REVERSED("NiDOMTool::WriteData(bool)", 0x00854E00, &ni::WriteBool);
NIDOMTOOL_REVERSED("NiDOMTool::WriteData(NiFixedString)", 0x00854E90, &ni::WriteString);
NIDOMTOOL_REVERSED("NiDOMTool::WriteData(void*)", 0x00854F20, &ni::WriteData);
NIDOMTOOL_REVERSED("NiDOMTool::GetAttributeCount", 0x00854FB0, &ni::GetAttributeCount);
NIDOMTOOL_REVERSED("NiDOMTool::GetAttributeFromCurrent", 0x00855000, &ni::GetAttribute);
NIDOMTOOL_REVERSED("NiDOMTool::GetCurrentSectionName", 0x00855030, &ni::GetSectionName);
NIDOMTOOL_REVERSED("NiDOMTool::IsLeaf", 0x00855060, &ni::IsLeaf);
NIDOMTOOL_REVERSED("NiDOMTool::ReadData(float)", 0x00855090, &ni::ReadFloat);
NIDOMTOOL_REVERSED("NiDOMTool::ReadData(bool)", 0x008550D0, &ni::ReadBool);
NIDOMTOOL_REVERSED("NiDOMTool::ReadData(int)", 0x00855140, &ni::ReadInt);
NIDOMTOOL_REVERSED("NiDOMTool::ReadData(unsigned int)", 0x00855180, &ni::ReadUInt);
NIDOMTOOL_REVERSED("NiDOMTool::ReadData(short)", 0x008551C0, &ni::ReadShort);
NIDOMTOOL_REVERSED("NiDOMTool::ReadData(unsigned short)", 0x00855200, &ni::ReadUShort);
NIDOMTOOL_REVERSED("NiDOMTool::ReadData(NiPoint2)", 0x00855240, &ni::ReadPoint2);
NIDOMTOOL_REVERSED("NiDOMTool::ReadData(NiColorA)", 0x00855290, &ni::ReadColorA);
NIDOMTOOL_REVERSED("NiDOMTool::ReadData(NiMatrix3)", 0x008552E0, &ni::ReadMatrix3);
NIDOMTOOL_REVERSED("NiDOMTool::ReadData(NiPoint3)", 0x00855390, &ni::ReadPoint3);
NIDOMTOOL_REVERSED("NiDOMTool::ReadData(void*)", 0x008553E0, &ni::ReadData);
NIDOMTOOL_REVERSED("NiDOMTool::ReadData(NiFixedString)", 0x00855440, &ni::ReadString);
NIDOMTOOL_REVERSED("NiDOMTool::SetSectionToNextSibling", 0x00855480, &ni::SetSectionToNextSibling);
NIDOMTOOL_REVERSED("NiDOMTool::SetSectionToFirstChild", 0x00855500, &ni::SetSectionToFirstChild);
NIDOMTOOL_REVERSED("NiDOMTool::SetSectionTo", 0x00855560, &ni::SetSectionTo);
NIDOMTOOL_REVERSED("NiDOMTool::BeginSection", 0x008555D0, &ni::BeginSection);
NIDOMTOOL_REVERSED("NiDOMTool::WriteData(float)", 0x00855650, &ni::WriteFloat);
NIDOMTOOL_REVERSED("NiDOMTool::WriteData(int)", 0x00855720, &ni::WriteInt);
NIDOMTOOL_REVERSED("NiDOMTool::WriteData(unsigned int)", 0x008557F0, &ni::WriteUInt);
NIDOMTOOL_REVERSED("NiDOMTool::WriteData(short)", 0x008558C0, &ni::WriteShort);
NIDOMTOOL_REVERSED("NiDOMTool::WriteData(unsigned short)", 0x00855990, &ni::WriteUShort);
NIDOMTOOL_REVERSED("NiDOMTool::WriteData(NiPoint2)", 0x00855A60, &ni::WritePoint2);
NIDOMTOOL_REVERSED("NiDOMTool::WriteData(NiColorA)", 0x00855B30, &ni::WriteColorA);
NIDOMTOOL_REVERSED("NiDOMTool::WriteData(NiMatrix3)", 0x00855C10, &ni::WriteMatrix3);
NIDOMTOOL_REVERSED("NiDOMTool::WriteData(NiPoint3)", 0x00855D30, &ni::WritePoint3);
NIDOMTOOL_REVERSED("NiDOMTool::ResetSectionTo", 0x00855E10, &ni::ResetSectionTo);
NIDOMTOOL_REVERSED("NiDOMTool::Close", 0x00855E30, &ni::Close);
NIDOMTOOL_REVERSED("NiDOMTool::Init", 0x00855E70, &ni::Init);
