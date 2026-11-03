// TinyXML 2.4.x's classes (TinyXml.h): TiXmlString, the node tree (linking, finding children
// and siblings by name), the attributes, the constructors and destructors, Print / StreamOut,
// Clone / CopyTo, and TiXmlDocument's constructors and Save / Load by name. The parsing side is
// TinyXml2.cpp.

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/TinyXml.h"
#include "reversed.h"

namespace tixml {
namespace {

inline void SetVtable(void* object, uintptr_t vtable) {
    *static_cast<uint32_t*>(object) = vtable;
}

// operator new's size for a Rep of `capacity` characters (new int[(12 + capacity + 3) / 4], the
// count's overflow check: all ones)
inline size_t RepBytes(uint32_t capacity) {
    uint64_t bytes = uint64_t((capacity + 15) >> 2) * 4;
    return bytes > 0xFFFFFFFFull ? size_t(0xFFFFFFFF) : size_t(bytes);
}

// TiXmlNode's constructor as the code inlines it (TiXmlBase's: the cursor cleared, no user data)
inline void InitNode(TiXmlNode* node, int type, uintptr_t vtable) {
    node->location.col = -1;
    node->location.row = -1;
    node->userData = nullptr;
    node->value.rep = kNullRep;
    node->parent = nullptr;
    node->type = type;
    node->firstChild = nullptr;
    node->lastChild = nullptr;
    node->prev = nullptr;
    node->next = nullptr;
    SetVtable(node, vtable);
}

// TiXmlNode::CopyTo, inlined: SetValue(value.c_str()) and the user data
inline void CopyNode(const TiXmlNode* self, TiXmlNode* target) {
    AssignCStr(&target->value, CStr(self->value));
    target->userData = self->userData;
}

// TiXmlNode::LinkEndChild, inlined
inline void LinkEnd(TiXmlNode* self, TiXmlNode* node) {
    node->parent = self;
    node->prev = self->lastChild;
    node->next = nullptr;
    if (self->lastChild != nullptr) {
        self->lastChild->next = node;
    } else {
        self->firstChild = node;
    }
    self->lastChild = node;
}

// TiXmlAttributeSet::First and TiXmlAttribute::Next, inlined (the sentinel has neither a name
// nor a value)
inline const TiXmlAttribute* FirstAttribute(const TiXmlElement* self) {
    const TiXmlAttribute* first = self->attributeSet.sentinel.next;
    return first == &self->attributeSet.sentinel ? nullptr : first;
}
inline const TiXmlAttribute* NextAttribute(const TiXmlAttribute* attribute) {
    const TiXmlAttribute* next = attribute->next;
    if (Length(next->value) == 0 && Length(next->name) == 0) {
        return nullptr;
    }
    return next;
}

// TiXmlString::find(char)
inline uint32_t Find(const TiXmlString& s, char c) {
    if (Length(s) == 0) {
        return npos;
    }
    for (const char* p = CStr(s); *p != '\0'; p++) {
        if (*p == c) {
            return uint32_t(p - CStr(s));
        }
    }
    return npos;
}

// (*stream) << text, with strlen folded
inline void Append(TiXmlString* stream, const char* text, uint32_t length) {
    String_Append(stream, text, length);
}
inline void AppendString(TiXmlString* stream, const TiXmlString& text) {
    String_Append(stream, CStr(text), Length(text));
}

// TiXmlBase::PutString(str, stream): through a temporary string
inline void PutStringToStream(const TiXmlString* text, TiXmlString* stream) {
    TiXmlString buffer = {kNullRep};
    Base_PutString(text, &buffer);
    String_Append(stream, CStr(buffer), Length(buffer));
    FreeString(buffer);
}

inline void Indent(FILE* file, int depth) {
    for (int i = depth; i > 0; i--) {
        Fprintf(file, "    ");
    }
}

// StringToBuffer: a copy of the string (operator new; strcpy inlined)
inline char* CopyBuffer(const TiXmlString& s) {
    char* buffer = static_cast<char*>(OperatorNew(Length(s) + 1));
    if (buffer != nullptr) {
        const char* from = CStr(s);
        char* to = buffer;
        while ((*to++ = *from++) != '\0') {
        }
    }
    return buffer;
}

} // namespace

// ---- TiXmlString

// 0x857030 (thiscall): TiXmlString::init(size, capacity)
void __fastcall String_InitImpl(TiXmlString* self, re::Edx, uint32_t size, uint32_t capacity) {
    if (capacity == 0) {
        self->rep = kNullRep;
        return;
    }
    self->rep = static_cast<Rep*>(OperatorNew(RepBytes(capacity)));
    self->rep->size = size;
    self->rep->str[size] = '\0';
    self->rep->capacity = capacity;
}

// 0x857080 (thiscall): TiXmlString::reserve (a bigger block, the characters copied)
void __fastcall String_ReserveImpl(TiXmlString* self, re::Edx, uint32_t capacity) {
    if (capacity <= self->rep->capacity) {
        return;
    }
    TiXmlString tmp = {kNullRep};
    String_Init(&tmp, self->rep->size, capacity);
    crt::Memcpy(tmp.rep->str, self->rep->str, self->rep->size);
    Rep* old = self->rep;
    self->rep = tmp.rep;
    if (old != kNullRep) {
        OperatorDelete(old);
    }
}

// 0x8570E0 (thiscall): TiXmlString::assign (in place when it fits and isn't 3x too big)
TiXmlString* __fastcall String_AssignImpl(TiXmlString* self, re::Edx, const char* text, uint32_t length) {
    uint32_t capacity = self->rep->capacity;
    if (length <= capacity && capacity <= length * 3 + 0x18) {
        std::memmove(self->rep->str, text, length);   // (memmove isn't in Crt.h: ours)
        self->rep->size = length;
        self->rep->str[length] = '\0';
        return self;
    }
    Rep* rep;
    if (length == 0) {
        rep = kNullRep;
    } else {
        rep = static_cast<Rep*>(OperatorNew(RepBytes(length)));
        rep->size = length;
        rep->str[length] = '\0';
        rep->capacity = length;
    }
    crt::Memcpy(rep->str, text, length);
    Rep* old = self->rep;
    self->rep = rep;
    if (old != kNullRep) {
        OperatorDelete(old);
    }
    return self;
}

// 0x857190 (thiscall): TiXmlString::append (reserve(newsize + capacity) when it doesn't fit)
TiXmlString* __fastcall String_AppendImpl(TiXmlString* self, re::Edx, const char* text, uint32_t length) {
    uint32_t size = self->rep->size + length;
    if (size > self->rep->capacity) {
        String_Reserve(self, self->rep->capacity + size);
    }
    std::memmove(self->rep->str + self->rep->size, text, length);
    self->rep->size = size;
    self->rep->str[size] = '\0';
    return self;
}

// 0x8571E0 (thiscall): TiXmlString::operator+=(const char*)
TiXmlString* __fastcall String_AppendCStrImpl(TiXmlString* self, re::Edx, const char* text) {
    uint32_t length = 0;
    while (text[length] != '\0') {
        length++;
    }
    return String_Append(self, text, length);
}

// 0x859AB0 (thiscall): TiXmlString(const char*)
TiXmlString* __fastcall String_ConstructCStr(TiXmlString* self, re::Edx, const char* text) {
    uint32_t length = 0;
    while (text[length] != '\0') {
        length++;
    }
    if (length == 0) {
        self->rep = kNullRep;
    } else {
        self->rep = static_cast<Rep*>(OperatorNew(RepBytes(length)));
        self->rep->size = length;
        self->rep->str[length] = '\0';
        self->rep->capacity = length;
    }
    crt::Memcpy(self->rep->str, text, self->rep->size);
    return self;
}

// ---- TiXmlBase, TiXmlNode

// 0x857210 (thiscall): TiXmlBase's deleting destructor
TiXmlBase* __fastcall Base_Destroy(TiXmlBase* self, re::Edx, int flags) {
    SetVtable(self, kTiXmlBaseVtable);
    if (flags & 1) {
        OperatorDelete(self);
    }
    return self;
}

// 0x857980 (thiscall): TiXmlNode(NodeType)
TiXmlNode* __fastcall Node_ConstructImpl(TiXmlNode* self, re::Edx, int type) {
    self->location.col = -1;
    self->location.row = -1;
    self->userData = nullptr;
    SetVtable(self, kTiXmlNodeVtable);
    self->value.rep = kNullRep;
    self->parent = nullptr;
    self->type = type;
    self->firstChild = nullptr;
    self->lastChild = nullptr;
    self->prev = nullptr;
    self->next = nullptr;
    return self;
}

// 0x8579C0 (thiscall): ~TiXmlNode (the children deleted)
void __fastcall Node_DestructImpl(TiXmlNode* self, re::Edx) {
    TiXmlNode* node = self->firstChild;
    SetVtable(self, kTiXmlNodeVtable);
    while (node != nullptr) {
        TiXmlNode* temp = node;
        node = node->next;
        Virtual<void*>(temp, kDestroy, 1);
    }
    FreeString(self->value);
    SetVtable(self, kTiXmlBaseVtable);
}

// 0x858A30 (thiscall): TiXmlNode's deleting destructor
TiXmlNode* __fastcall Node_Destroy(TiXmlNode* self, re::Edx, int flags) {
    Node_Destruct(self);
    if (flags & 1) {
        OperatorDelete(self);
    }
    return self;
}

// 0x8573C0 (thiscall): TiXmlNode::LinkEndChild
TiXmlNode* __fastcall Node_LinkEndChildImpl(TiXmlNode* self, re::Edx, TiXmlNode* node) {
    LinkEnd(self, node);
    return node;
}

// 0x8573F0 (thiscall): TiXmlNode::FirstChild(const char* value)
TiXmlNode* __fastcall Node_FirstChildImpl(const TiXmlNode* self, re::Edx, const char* value) {
    for (TiXmlNode* node = self->firstChild; node != nullptr; node = node->next) {
        if (crt::Strcmp(CStr(node->value), value) == 0) {
            return node;
        }
    }
    return nullptr;
}

// 0x857440 (thiscall): TiXmlNode::NextSibling(const char* value)
TiXmlNode* __fastcall Node_NextSiblingImpl(const TiXmlNode* self, re::Edx, const char* value) {
    for (TiXmlNode* node = self->next; node != nullptr; node = node->next) {
        if (crt::Strcmp(CStr(node->value), value) == 0) {
            return node;
        }
    }
    return nullptr;
}

// 0x857490 (thiscall): TiXmlNode::FirstChildElement()
TiXmlNode* __fastcall Node_FirstChildElement(const TiXmlNode* self, re::Edx) {
    for (TiXmlNode* node = self->firstChild; node != nullptr; node = node->next) {
        if (IsType(node, ELEMENT)) {
            return node;
        }
    }
    return nullptr;
}

// 0x8574C0 (thiscall): TiXmlNode::FirstChildElement(const char* value)
TiXmlNode* __fastcall Node_FirstChildElementNamed(const TiXmlNode* self, re::Edx, const char* value) {
    for (TiXmlNode* node = Node_FirstChild(self, value); node != nullptr; node = Node_NextSibling(node, value)) {
        if (IsType(node, ELEMENT)) {
            return node;
        }
    }
    return nullptr;
}

// 0x857500 (thiscall): TiXmlNode::NextSiblingElement()
TiXmlNode* __fastcall Node_NextSiblingElement(const TiXmlNode* self, re::Edx) {
    for (TiXmlNode* node = self->next; node != nullptr; node = node->next) {
        if (IsType(node, ELEMENT)) {
            return node;
        }
    }
    return nullptr;
}

// 0x857530 (thiscall): TiXmlNode::NextSiblingElement(const char* value)
TiXmlNode* __fastcall Node_NextSiblingElementNamed(const TiXmlNode* self, re::Edx, const char* value) {
    for (TiXmlNode* node = Node_NextSibling(self, value); node != nullptr; node = Node_NextSibling(node, value)) {
        if (IsType(node, ELEMENT)) {
            return node;
        }
    }
    return nullptr;
}

// 0x857570 (thiscall): TiXmlNode::GetDocument (up the parents)
TiXmlDocument* __fastcall Node_GetDocumentImpl(const TiXmlNode* self, re::Edx) {
    for (const TiXmlNode* node = self; node != nullptr; node = node->parent) {
        if (IsType(node, DOCUMENT)) {
            return static_cast<TiXmlDocument*>(const_cast<TiXmlNode*>(node));
        }
    }
    return nullptr;
}

// ---- TiXmlAttribute, TiXmlAttributeSet

// 0x8578C0 (thiscall): TiXmlAttribute(name, value)
TiXmlAttribute* __fastcall Attribute_ConstructImpl(TiXmlAttribute* self, re::Edx, const char* name, const char* value) {
    self->location.col = -1;
    self->location.row = -1;
    self->userData = nullptr;
    SetVtable(self, kTiXmlAttributeVtable);
    self->name.rep = kNullRep;
    self->value.rep = kNullRep;
    AssignCStr(&self->name, name);
    AssignCStr(&self->value, value);
    self->document = nullptr;
    self->next = nullptr;
    self->prev = nullptr;
    return self;
}

// 0x857940 (thiscall): ~TiXmlAttribute
void __fastcall Attribute_Destruct(TiXmlAttribute* self, re::Edx) {
    FreeString(self->value);
    FreeString(self->name);
    SetVtable(self, kTiXmlBaseVtable);
}

// 0x8588D0 (thiscall): TiXmlAttribute's deleting destructor
TiXmlAttribute* __fastcall Attribute_Destroy(TiXmlAttribute* self, re::Edx, int flags) {
    FreeString(self->value);
    FreeString(self->name);
    SetVtable(self, kTiXmlBaseVtable);
    if (flags & 1) {
        OperatorDelete(self);
    }
    return self;
}

// 0x857230 (thiscall): TiXmlAttribute::SetValue(const char*)
void __fastcall Attribute_SetValueImpl(TiXmlAttribute* self, re::Edx, const char* value) {
    AssignCStr(&self->value, value);
}

// 0x857730 (thiscall): TiXmlAttribute::Next (null at the sentinel)
const TiXmlAttribute* __fastcall Attribute_Next(const TiXmlAttribute* self, re::Edx) {
    return NextAttribute(self);
}

// 0x858200 (thiscall): TiXmlAttribute::Print (name="value", or name='value' when the value has
// a double quote)
void __fastcall Attribute_Print(const TiXmlAttribute* self, re::Edx, FILE* file, int) {
    TiXmlString n = {kNullRep};
    TiXmlString v = {kNullRep};
    Base_PutString(&self->name, &n);
    Base_PutString(&self->value, &v);
    if (Find(self->value, '"') == npos) {
        Fprintf(file, "%s=\"%s\"", CStr(n), CStr(v));
    } else {
        Fprintf(file, "%s='%s'", CStr(n), CStr(v));
    }
    FreeString(v);
    FreeString(n);
}

// 0x8582D0 (thiscall): TiXmlAttribute::StreamOut
void __fastcall Attribute_StreamOut(const TiXmlAttribute* self, re::Edx, TiXmlString* stream) {
    if (Find(self->value, '"') != npos) {
        PutStringToStream(&self->name, stream);
        Append(stream, "=", 1);
        Append(stream, "'", 1);
        PutStringToStream(&self->value, stream);
        Append(stream, "'", 1);
        return;
    }
    PutStringToStream(&self->name, stream);
    Append(stream, "=", 1);
    Append(stream, "\"", 1);
    PutStringToStream(&self->value, stream);
    Append(stream, "\"", 1);
}

// 0x857850 (thiscall): TiXmlAttributeSet::Add (at the end of the ring)
void __fastcall AttributeSet_Add(TiXmlAttributeSet* self, re::Edx, TiXmlAttribute* attribute) {
    attribute->next = &self->sentinel;
    attribute->prev = self->sentinel.prev;
    self->sentinel.prev->next = attribute;
    self->sentinel.prev = attribute;
}

// 0x857870 (thiscall): TiXmlAttributeSet::Find
TiXmlAttribute* __fastcall AttributeSet_FindImpl(const TiXmlAttributeSet* self, re::Edx, const char* name) {
    for (TiXmlAttribute* node = self->sentinel.next; node != &self->sentinel; node = node->next) {
        if (crt::Strcmp(CStr(node->name), name) == 0) {
            return node;
        }
    }
    return nullptr;
}

// ---- TiXmlElement

// 0x858A50 (thiscall): TiXmlElement(const char* value)
TiXmlElement* __fastcall Element_ConstructImpl(TiXmlElement* self, re::Edx, const char* value) {
    self->location.col = -1;
    self->location.row = -1;
    self->userData = nullptr;
    self->value.rep = kNullRep;
    self->parent = nullptr;
    self->type = ELEMENT;
    self->prev = nullptr;
    self->next = nullptr;
    SetVtable(self, kTiXmlElementVtable);
    TiXmlAttribute* sentinel = &self->attributeSet.sentinel;
    sentinel->location.col = -1;
    sentinel->location.row = -1;
    sentinel->userData = nullptr;
    SetVtable(sentinel, kTiXmlAttributeVtable);
    sentinel->name.rep = kNullRep;
    sentinel->value.rep = kNullRep;
    sentinel->document = nullptr;
    sentinel->next = sentinel;
    sentinel->prev = sentinel;
    self->lastChild = nullptr;
    self->firstChild = nullptr;
    AssignCStr(&self->value, value);
    return self;
}

// 0x858DD0 (thiscall): TiXmlElement's deleting destructor
TiXmlElement* __fastcall Element_Destroy(TiXmlElement* self, re::Edx, int flags) {
    SetVtable(self, kTiXmlElementVtable);
    Element_ClearThis(self);
    TiXmlAttribute* sentinel = &self->attributeSet.sentinel;
    FreeString(sentinel->value);
    FreeString(sentinel->name);
    SetVtable(sentinel, kTiXmlBaseVtable);
    Node_Destruct(self);
    if (flags & 1) {
        OperatorDelete(self);
    }
    return self;
}

// 0x857A00 (thiscall): TiXmlElement::ClearThis (the children, then each attribute unlinked and
// deleted)
void __fastcall Element_ClearThisImpl(TiXmlElement* self, re::Edx) {
    TiXmlNode* node = self->firstChild;
    while (node != nullptr) {
        TiXmlNode* temp = node;
        node = node->next;
        Virtual<void*>(temp, kDestroy, 1);
    }
    self->firstChild = nullptr;
    self->lastChild = nullptr;
    TiXmlAttribute* sentinel = &self->attributeSet.sentinel;
    for (;;) {
        TiXmlAttribute* first = sentinel->next;
        if (first == sentinel || first == nullptr) {
            return;
        }
        // TiXmlAttributeSet::Remove
        for (TiXmlAttribute* a = first; a != sentinel; a = a->next) {
            if (a == first) {
                a->prev->next = a->next;
                a->next->prev = a->prev;
                a->next = nullptr;
                a->prev = nullptr;
                break;
            }
        }
        Virtual<void*>(first, kDestroy, 1);
    }
}

// 0x857A90 (thiscall): TiXmlElement::Attribute(name) (its value, or null)
const char* __fastcall Element_Attribute(const TiXmlElement* self, re::Edx, const char* name) {
    TiXmlAttribute* node = AttributeSet_Find(&self->attributeSet, name);
    if (node != nullptr) {
        return CStr(node->value);
    }
    return nullptr;
}

// 0x857AB0 (thiscall): TiXmlElement::SetAttribute(name, value) (out of memory: the document's
// error)
void __fastcall Element_SetAttributeImpl(TiXmlElement* self, re::Edx, const char* name, const char* value) {
    TiXmlAttribute* node = AttributeSet_Find(&self->attributeSet, name);
    if (node != nullptr) {
        AssignCStr(&node->value, value);
        return;
    }
    void* memory = OperatorNew(0x24);
    TiXmlAttribute* attribute = memory != nullptr
                                    ? Attribute_Construct(static_cast<TiXmlAttribute*>(memory), name, value)
                                    : nullptr;
    if (attribute != nullptr) {
        TiXmlAttributeSet* set = &self->attributeSet;
        attribute->next = &set->sentinel;
        attribute->prev = set->sentinel.prev;
        set->sentinel.prev->next = attribute;
        set->sentinel.prev = attribute;
        return;
    }
    for (TiXmlNode* doc = self; doc != nullptr; doc = doc->parent) {
        if (IsType(doc, DOCUMENT)) {
            Document_SetError(static_cast<TiXmlDocument*>(doc), TIXML_ERROR_OUT_OF_MEMORY, nullptr, nullptr,
                              TIXML_ENCODING_UNKNOWN);
            return;
        }
    }
}

// 0x858AE0 (thiscall): TiXmlElement::SetAttribute(name, int)
void __fastcall Element_SetAttributeInt(TiXmlElement* self, re::Edx, const char* name, int value) {
    char buffer[64];
    crt::Snprintf(buffer, sizeof(buffer), "%d", value);
    Element_SetAttribute(self, name, buffer);
}

// 0x857590 (thiscall): TiXmlElement::GetText (the first child's, when it's text)
const char* __fastcall Element_GetText(const TiXmlElement* self, re::Edx) {
    const TiXmlNode* child = self->firstChild;
    if (child != nullptr && IsType(child, TEXT)) {
        return CStr(child->value);
    }
    return nullptr;
}

// 0x857B60 (thiscall): TiXmlElement::Print (<foo />, <foo>text</foo>, or the children on lines
// of their own)
void __fastcall Element_Print(const TiXmlElement* self, re::Edx, FILE* file, int depth) {
    Indent(file, depth);
    Fprintf(file, "<%s", CStr(self->value));
    for (const TiXmlAttribute* a = FirstAttribute(self); a != nullptr; a = NextAttribute(a)) {
        Fprintf(file, " ");
        Virtual<void>(a, kPrint, file, depth);
    }
    TiXmlNode* first = self->firstChild;
    if (first == nullptr) {
        Fprintf(file, " />");
        return;
    }
    if (first == self->lastChild && IsType(first, TEXT)) {
        Fprintf(file, ">");
        Virtual<void>(self->firstChild, kPrint, file, depth + 1);
        Fprintf(file, "</%s>", CStr(self->value));
        return;
    }
    Fprintf(file, ">");
    for (TiXmlNode* node = self->firstChild; node != nullptr; node = node->next) {
        if (!IsType(node, TEXT)) {
            Fprintf(file, "\n");
        }
        Virtual<void>(node, kPrint, file, depth + 1);
    }
    Fprintf(file, "\n");
    Indent(file, depth);
    Fprintf(file, "</%s>", CStr(self->value));
}

// 0x857CC0 (thiscall): TiXmlElement::StreamOut
void __fastcall Element_StreamOut(const TiXmlElement* self, re::Edx, TiXmlString* stream) {
    Append(stream, "<", 1);
    AppendString(stream, self->value);
    for (const TiXmlAttribute* a = FirstAttribute(self); a != nullptr; a = NextAttribute(a)) {
        Append(stream, " ", 1);
        Virtual<void>(a, kStreamOut, stream);
    }
    if (self->firstChild == nullptr) {
        Append(stream, " />", 3);
        return;
    }
    Append(stream, ">", 1);
    for (TiXmlNode* node = self->firstChild; node != nullptr; node = node->next) {
        Virtual<void>(node, kStreamOut, stream);
    }
    Append(stream, "</", 2);
    AppendString(stream, self->value);
    Append(stream, ">", 1);
}

// 0x857DA0 (thiscall): TiXmlElement::CopyTo (the attributes set, the children cloned)
void __fastcall Element_CopyToImpl(const TiXmlElement* self, re::Edx, TiXmlElement* target) {
    CopyNode(self, target);
    for (const TiXmlAttribute* a = FirstAttribute(self); a != nullptr; a = NextAttribute(a)) {
        Element_SetAttribute(target, CStr(a->name), CStr(a->value));
    }
    for (TiXmlNode* node = self->firstChild; node != nullptr; node = node->next) {
        LinkEnd(target, Virtual<TiXmlNode*>(node, kClone));
    }
}

// 0x858B30 (thiscall): TiXmlElement::Clone
TiXmlNode* __fastcall Element_Clone(const TiXmlElement* self, re::Edx) {
    void* memory = OperatorNew(0x50);
    if (memory == nullptr) {
        return nullptr;
    }
    TiXmlElement* clone = Element_Construct(static_cast<TiXmlElement*>(memory), CStr(self->value));
    if (clone == nullptr) {
        return nullptr;
    }
    Element_CopyTo(self, clone);
    return clone;
}

// ---- TiXmlComment, TiXmlText, TiXmlUnknown, TiXmlDeclaration

// 0x857750 (thiscall): TiXmlComment::Print
void __fastcall Comment_Print(const TiXmlComment* self, re::Edx, FILE* file, int depth) {
    for (int i = depth; i > 0; i--) {
        Fputs("    ", file);
    }
    Fprintf(file, "<!--%s-->", CStr(self->value));
}

// 0x858470 (thiscall): TiXmlComment::StreamOut
void __fastcall Comment_StreamOut(const TiXmlComment* self, re::Edx, TiXmlString* stream) {
    Append(stream, "<!--", 4);
    AppendString(stream, self->value);
    Append(stream, "-->", 3);
}

// 0x858BE0 (thiscall): TiXmlComment::Clone (the constructor and CopyTo inlined)
TiXmlNode* __fastcall Comment_Clone(const TiXmlComment* self, re::Edx) {
    TiXmlComment* clone = static_cast<TiXmlComment*>(OperatorNew(0x2C));
    if (clone == nullptr) {
        return nullptr;
    }
    InitNode(clone, COMMENT, kTiXmlCommentVtable);
    CopyNode(self, clone);
    return clone;
}

// 0x8584B0 (thiscall): TiXmlText::Print (CDATA on a line of its own, else the entities put back)
void __fastcall Text_Print(const TiXmlText* self, re::Edx, FILE* file, int depth) {
    if (self->cdata != 0) {
        Fprintf(file, "\n");
        Indent(file, depth);
        Fprintf(file, "<![CDATA[");
        Fprintf(file, "%s", CStr(self->value));
        Fprintf(file, "]]>\n");
        return;
    }
    TiXmlString buffer = {kNullRep};
    Base_PutString(&self->value, &buffer);
    Fprintf(file, "%s", CStr(buffer));
    FreeString(buffer);
}

// 0x858570 (thiscall): TiXmlText::StreamOut
void __fastcall Text_StreamOut(const TiXmlText* self, re::Edx, TiXmlString* stream) {
    if (self->cdata != 0) {
        Append(stream, "<![CDATA[", 9);
        AppendString(stream, self->value);
        Append(stream, "]]>", 3);
        return;
    }
    PutStringToStream(&self->value, stream);
}

// 0x858C70 (thiscall): TiXmlText::Clone
TiXmlNode* __fastcall Text_Clone(const TiXmlText* self, re::Edx) {
    void* memory = OperatorNew(0x30);
    if (memory == nullptr) {
        return nullptr;
    }
    TiXmlText* clone = Text_Construct(static_cast<TiXmlText*>(memory), "");
    if (clone == nullptr) {
        return nullptr;
    }
    CopyNode(self, clone);
    clone->cdata = self->cdata;
    return clone;
}

// 0x8589C0 (thiscall): TiXmlUnknown()
TiXmlUnknown* __fastcall Unknown_ConstructImpl(TiXmlUnknown* self, re::Edx) {
    InitNode(self, UNKNOWN, kTiXmlUnknownVtable);
    return self;
}

// 0x858A00 (thiscall): TiXmlUnknown's deleting destructor
TiXmlUnknown* __fastcall Unknown_Destroy(TiXmlUnknown* self, re::Edx, int flags) {
    SetVtable(self, kTiXmlUnknownVtable);
    Node_Destruct(self);
    if (flags & 1) {
        OperatorDelete(self);
    }
    return self;
}

// 0x857810 (thiscall): TiXmlUnknown::Print
void __fastcall Unknown_Print(const TiXmlUnknown* self, re::Edx, FILE* file, int depth) {
    Indent(file, depth);
    Fprintf(file, "<%s>", CStr(self->value));
}

// 0x858890 (thiscall): TiXmlUnknown::StreamOut
void __fastcall Unknown_StreamOut(const TiXmlUnknown* self, re::Edx, TiXmlString* stream) {
    Append(stream, "<", 1);
    AppendString(stream, self->value);
    Append(stream, ">", 1);
}

// 0x858D40 (thiscall): TiXmlUnknown::Clone
TiXmlNode* __fastcall Unknown_Clone(const TiXmlUnknown* self, re::Edx) {
    TiXmlUnknown* clone = static_cast<TiXmlUnknown*>(OperatorNew(0x2C));
    if (clone == nullptr) {
        return nullptr;
    }
    InitNode(clone, UNKNOWN, kTiXmlUnknownVtable);
    CopyNode(self, clone);
    return clone;
}

// 0x858920 (thiscall): TiXmlDeclaration()
TiXmlDeclaration* __fastcall Declaration_ConstructImpl(TiXmlDeclaration* self, re::Edx) {
    InitNode(self, DECLARATION, kTiXmlDeclarationVtable);
    self->version.rep = kNullRep;
    self->encoding.rep = kNullRep;
    self->standalone.rep = kNullRep;
    return self;
}

// 0x858600 (thiscall): TiXmlDeclaration(version, encoding, standalone)
TiXmlDeclaration* __fastcall Declaration_ConstructWith(TiXmlDeclaration* self, re::Edx, const char* version,
                                                       const char* encoding, const char* standalone) {
    InitNode(self, DECLARATION, kTiXmlDeclarationVtable);
    self->version.rep = kNullRep;
    self->encoding.rep = kNullRep;
    self->standalone.rep = kNullRep;
    AssignCStr(&self->version, version);
    AssignCStr(&self->encoding, encoding);
    AssignCStr(&self->standalone, standalone);
    return self;
}

// 0x858960 (thiscall): TiXmlDeclaration's deleting destructor
TiXmlDeclaration* __fastcall Declaration_Destroy(TiXmlDeclaration* self, re::Edx, int flags) {
    SetVtable(self, kTiXmlDeclarationVtable);
    FreeString(self->standalone);
    FreeString(self->encoding);
    FreeString(self->version);
    Node_Destruct(self);
    if (flags & 1) {
        OperatorDelete(self);
    }
    return self;
}

// 0x857790 (thiscall): TiXmlDeclaration::Print
void __fastcall Declaration_Print(const TiXmlDeclaration* self, re::Edx, FILE* file, int) {
    Fprintf(file, "<?xml ");
    if (Length(self->version) != 0) {
        Fprintf(file, "version=\"%s\" ", CStr(self->version));
    }
    if (Length(self->encoding) != 0) {
        Fprintf(file, "encoding=\"%s\" ", CStr(self->encoding));
    }
    if (Length(self->standalone) != 0) {
        Fprintf(file, "standalone=\"%s\" ", CStr(self->standalone));
    }
    Fprintf(file, "?>");
}

// 0x8586C0 (thiscall): TiXmlDeclaration::StreamOut
void __fastcall Declaration_StreamOut(const TiXmlDeclaration* self, re::Edx, TiXmlString* stream) {
    Append(stream, "<?xml ", 6);
    if (Length(self->version) != 0) {
        Append(stream, "version=\"", 9);
        PutStringToStream(&self->version, stream);
        Append(stream, "\" ", 2);
    }
    if (Length(self->encoding) != 0) {
        Append(stream, "encoding=\"", 10);
        PutStringToStream(&self->encoding, stream);
        Append(stream, "\" ", 2);
    }
    if (Length(self->standalone) != 0) {
        Append(stream, "standalone=\"", 12);
        PutStringToStream(&self->standalone, stream);
        Append(stream, "\" ", 2);
    }
    Append(stream, "?>", 2);
}

// 0x858820 (thiscall): TiXmlDeclaration::CopyTo
void __fastcall Declaration_CopyToImpl(const TiXmlDeclaration* self, re::Edx, TiXmlDeclaration* target) {
    CopyNode(self, target);
    String_Assign(&target->version, CStr(self->version), Length(self->version));
    String_Assign(&target->encoding, CStr(self->encoding), Length(self->encoding));
    String_Assign(&target->standalone, CStr(self->standalone), Length(self->standalone));
}

// 0x858CD0 (thiscall): TiXmlDeclaration::Clone
TiXmlNode* __fastcall Declaration_Clone(const TiXmlDeclaration* self, re::Edx) {
    TiXmlDeclaration* clone = static_cast<TiXmlDeclaration*>(OperatorNew(0x38));
    if (clone == nullptr) {
        return nullptr;
    }
    InitNode(clone, DECLARATION, kTiXmlDeclarationVtable);
    clone->version.rep = kNullRep;
    clone->encoding.rep = kNullRep;
    clone->standalone.rep = kNullRep;
    Declaration_CopyTo(self, clone);
    return clone;
}

// ---- TiXmlDocument (no parsing)

namespace {
// the constructors' common part (TiXmlNode(DOCUMENT), tab size 4, no BOM)
inline void InitDocument(TiXmlDocument* self) {
    self->location.col = -1;
    self->location.row = -1;
    self->userData = nullptr;
    self->value.rep = kNullRep;
    self->parent = nullptr;
    self->type = DOCUMENT;
    self->firstChild = nullptr;
    self->lastChild = nullptr;
    self->prev = nullptr;
    self->next = nullptr;
    SetVtable(self, kTiXmlDocumentVtable);
    self->errorDesc.rep = kNullRep;
    self->errorLocation.col = -1;
    self->errorLocation.row = -1;
    self->tabsize = 4;
    self->useMicrosoftBOM = 0;
}
// ClearError
inline void ClearError(TiXmlDocument* self) {
    self->error = 0;
    self->errorId = 0;
    String_Assign(&self->errorDesc, "", 0);
    self->errorLocation.col = 0;
    self->errorLocation.row = 0;
}
} // namespace

// 0x857E50 (thiscall): TiXmlDocument()
TiXmlDocument* __fastcall Document_ConstructImpl(TiXmlDocument* self, re::Edx) {
    InitDocument(self);
    ClearError(self);
    return self;
}

// 0x857EC0 (thiscall): TiXmlDocument(const char* name)
TiXmlDocument* __fastcall Document_ConstructNamed(TiXmlDocument* self, re::Edx, const char* name) {
    InitDocument(self);
    AssignCStr(&self->value, name);
    ClearError(self);
    return self;
}

// 0x8576C0 (thiscall): TiXmlDocument::Print (each child on a line)
void __fastcall Document_Print(const TiXmlDocument* self, re::Edx, FILE* file, int depth) {
    for (TiXmlNode* node = self->firstChild; node != nullptr; node = node->next) {
        Virtual<void>(node, kPrint, file, depth);
        Fprintf(file, "\n");
    }
}

// 0x857700 (thiscall): TiXmlDocument::StreamOut (stops after the root element)
void __fastcall Document_StreamOut(const TiXmlDocument* self, re::Edx, TiXmlString* stream) {
    for (TiXmlNode* node = self->firstChild; node != nullptr; node = node->next) {
        Virtual<void>(node, kStreamOut, stream);
        if (IsType(node, ELEMENT)) {
            break;
        }
    }
}

// 0x857620 (thiscall): TiXmlDocument::CopyTo (2.4's: the error flag and text only)
void __fastcall Document_CopyToImpl(const TiXmlDocument* self, re::Edx, TiXmlDocument* target) {
    CopyNode(self, target);
    target->error = self->error;
    AssignCStr(&target->errorDesc, CStr(self->errorDesc));
    for (TiXmlNode* node = self->firstChild; node != nullptr; node = node->next) {
        LinkEnd(target, Virtual<TiXmlNode*>(node, kClone));
    }
}

// 0x8581C0 (thiscall): TiXmlDocument::Clone
TiXmlNode* __fastcall Document_Clone(const TiXmlDocument* self, re::Edx) {
    void* memory = OperatorNew(0x48);
    if (memory == nullptr) {
        return nullptr;
    }
    TiXmlDocument* clone = Document_Construct(static_cast<TiXmlDocument*>(memory));
    if (clone == nullptr) {
        return nullptr;
    }
    Document_CopyTo(self, clone);
    return clone;
}

// 0x8575B0 (thiscall): TiXmlDocument::SaveFile(const char* name) (text mode; the BOM's fputc
// calls reach the game's no-op)
uint8_t __fastcall Document_SaveFileNameImpl(const TiXmlDocument* self, re::Edx, const char* name) {
    FILE* file = crt::Fopen(name, "w");
    if (file == nullptr) {
        return 0;
    }
    if (self->useMicrosoftBOM != 0) {
        Fputc_824EC0(0xEF, file);
        Fputc_824EC0(0xBB, file);
        Fputc_824EC0(0xBF, file);
    }
    Virtual<void>(self, kPrint, file, 0);
    crt::Fclose(file);
    return 1;
}

// 0x857F60 (thiscall): TiXmlDocument::SaveFile() (under its own name)
uint8_t __fastcall Document_SaveFile(const TiXmlDocument* self, re::Edx) {
    char* buffer = CopyBuffer(self->value);
    if (buffer != nullptr && Document_SaveFileName(self, buffer) != 0) {
        OperatorDelete(buffer);
        return 1;
    }
    OperatorDelete(buffer);
    return 0;
}

// 0x858B70 (thiscall): TiXmlDocument::LoadFile(encoding) (under its own name)
uint8_t __fastcall Document_LoadFile(TiXmlDocument* self, re::Edx, int encoding) {
    char* buffer = CopyBuffer(self->value);
    if (buffer != nullptr && Document_LoadFileName(self, buffer, encoding) != 0) {
        OperatorDelete(buffer);
        return 1;
    }
    OperatorDelete(buffer);
    return 0;
}

#define TIXML38_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

TIXML38_REVERSED("TiXmlString::init", 0x00857030, String_InitImpl);
TIXML38_REVERSED("TiXmlString::reserve", 0x00857080, String_ReserveImpl);
TIXML38_REVERSED("TiXmlString::assign", 0x008570E0, String_AssignImpl);
TIXML38_REVERSED("TiXmlString::append", 0x00857190, String_AppendImpl);
TIXML38_REVERSED("TiXmlString::operator+=", 0x008571E0, String_AppendCStrImpl);
TIXML38_REVERSED("TiXmlBase::Destroy", 0x00857210, Base_Destroy);
TIXML38_REVERSED("TiXmlAttribute::SetValue", 0x00857230, Attribute_SetValueImpl);
TIXML38_REVERSED("TiXmlNode::LinkEndChild", 0x008573C0, Node_LinkEndChildImpl);
TIXML38_REVERSED("TiXmlNode::FirstChild", 0x008573F0, Node_FirstChildImpl);
TIXML38_REVERSED("TiXmlNode::NextSibling", 0x00857440, Node_NextSiblingImpl);
TIXML38_REVERSED("TiXmlNode::FirstChildElement", 0x00857490, Node_FirstChildElement);
TIXML38_REVERSED("TiXmlNode::FirstChildElementNamed", 0x008574C0, Node_FirstChildElementNamed);
TIXML38_REVERSED("TiXmlNode::NextSiblingElement", 0x00857500, Node_NextSiblingElement);
TIXML38_REVERSED("TiXmlNode::NextSiblingElementNamed", 0x00857530, Node_NextSiblingElementNamed);
TIXML38_REVERSED("TiXmlNode::GetDocument", 0x00857570, Node_GetDocumentImpl);
TIXML38_REVERSED("TiXmlElement::GetText", 0x00857590, Element_GetText);
TIXML38_REVERSED("TiXmlDocument::SaveFileNamed", 0x008575B0, Document_SaveFileNameImpl);
TIXML38_REVERSED("TiXmlDocument::CopyTo", 0x00857620, Document_CopyToImpl);
TIXML38_REVERSED("TiXmlDocument::Print", 0x008576C0, Document_Print);
TIXML38_REVERSED("TiXmlDocument::StreamOut", 0x00857700, Document_StreamOut);
TIXML38_REVERSED("TiXmlAttribute::Next", 0x00857730, Attribute_Next);
TIXML38_REVERSED("TiXmlComment::Print", 0x00857750, Comment_Print);
TIXML38_REVERSED("TiXmlDeclaration::Print", 0x00857790, Declaration_Print);
TIXML38_REVERSED("TiXmlUnknown::Print", 0x00857810, Unknown_Print);
TIXML38_REVERSED("TiXmlAttributeSet::Add", 0x00857850, AttributeSet_Add);
TIXML38_REVERSED("TiXmlAttributeSet::Find", 0x00857870, AttributeSet_FindImpl);
TIXML38_REVERSED("TiXmlAttribute::TiXmlAttribute", 0x008578C0, Attribute_ConstructImpl);
TIXML38_REVERSED("TiXmlAttribute::~TiXmlAttribute", 0x00857940, Attribute_Destruct);
TIXML38_REVERSED("TiXmlNode::TiXmlNode", 0x00857980, Node_ConstructImpl);
TIXML38_REVERSED("TiXmlNode::~TiXmlNode", 0x008579C0, Node_DestructImpl);
TIXML38_REVERSED("TiXmlElement::ClearThis", 0x00857A00, Element_ClearThisImpl);
TIXML38_REVERSED("TiXmlElement::Attribute", 0x00857A90, Element_Attribute);
TIXML38_REVERSED("TiXmlElement::SetAttribute", 0x00857AB0, Element_SetAttributeImpl);
TIXML38_REVERSED("TiXmlElement::Print", 0x00857B60, Element_Print);
TIXML38_REVERSED("TiXmlElement::StreamOut", 0x00857CC0, Element_StreamOut);
TIXML38_REVERSED("TiXmlElement::CopyTo", 0x00857DA0, Element_CopyToImpl);
TIXML38_REVERSED("TiXmlDocument::TiXmlDocument", 0x00857E50, Document_ConstructImpl);
TIXML38_REVERSED("TiXmlDocument::TiXmlDocumentNamed", 0x00857EC0, Document_ConstructNamed);
TIXML38_REVERSED("TiXmlDocument::SaveFile", 0x00857F60, Document_SaveFile);
TIXML38_REVERSED("TiXmlDocument::Clone", 0x008581C0, Document_Clone);
TIXML38_REVERSED("TiXmlAttribute::Print", 0x00858200, Attribute_Print);
TIXML38_REVERSED("TiXmlAttribute::StreamOut", 0x008582D0, Attribute_StreamOut);
TIXML38_REVERSED("TiXmlComment::StreamOut", 0x00858470, Comment_StreamOut);
TIXML38_REVERSED("TiXmlText::Print", 0x008584B0, Text_Print);
TIXML38_REVERSED("TiXmlText::StreamOut", 0x00858570, Text_StreamOut);
TIXML38_REVERSED("TiXmlDeclaration::TiXmlDeclarationWith", 0x00858600, Declaration_ConstructWith);
TIXML38_REVERSED("TiXmlDeclaration::StreamOut", 0x008586C0, Declaration_StreamOut);
TIXML38_REVERSED("TiXmlDeclaration::CopyTo", 0x00858820, Declaration_CopyToImpl);
TIXML38_REVERSED("TiXmlUnknown::StreamOut", 0x00858890, Unknown_StreamOut);
TIXML38_REVERSED("TiXmlAttribute::Destroy", 0x008588D0, Attribute_Destroy);
TIXML38_REVERSED("TiXmlDeclaration::TiXmlDeclaration", 0x00858920, Declaration_ConstructImpl);
TIXML38_REVERSED("TiXmlDeclaration::Destroy", 0x00858960, Declaration_Destroy);
TIXML38_REVERSED("TiXmlUnknown::TiXmlUnknown", 0x008589C0, Unknown_ConstructImpl);
TIXML38_REVERSED("TiXmlUnknown::Destroy", 0x00858A00, Unknown_Destroy);
TIXML38_REVERSED("TiXmlNode::Destroy", 0x00858A30, Node_Destroy);
TIXML38_REVERSED("TiXmlElement::TiXmlElement", 0x00858A50, Element_ConstructImpl);
TIXML38_REVERSED("TiXmlElement::SetAttributeInt", 0x00858AE0, Element_SetAttributeInt);
TIXML38_REVERSED("TiXmlElement::Clone", 0x00858B30, Element_Clone);
TIXML38_REVERSED("TiXmlDocument::LoadFile", 0x00858B70, Document_LoadFile);
TIXML38_REVERSED("TiXmlComment::Clone", 0x00858BE0, Comment_Clone);
TIXML38_REVERSED("TiXmlText::Clone", 0x00858C70, Text_Clone);
TIXML38_REVERSED("TiXmlDeclaration::Clone", 0x00858CD0, Declaration_Clone);
TIXML38_REVERSED("TiXmlUnknown::Clone", 0x00858D40, Unknown_Clone);
TIXML38_REVERSED("TiXmlElement::Destroy", 0x00858DD0, Element_Destroy);
TIXML38_REVERSED("TiXmlString::TiXmlString", 0x00859AB0, String_ConstructCStr);

} // namespace tixml
