#pragma once

// TinyXML as Bully's exe has it (linked with Gamebryo's entity library: NiDOMTool reads and
// writes the GSA scene files with it; the game's own light loader, src/game/LightXml*.cpp, reads
// .gsa files with it too). Version 2.4.x (2.4.2 / 2.4.3), built without TIXML_USE_STL:
//   - TiXmlString is the 2.4 one (a pointer to a Rep {size, capacity, characters}; the empty
//     string shares nullrep_), TIXML_OSTREAM is TiXmlOutStream (a TiXmlString appended to)
//   - five virtual slots, 2.4's order: 0 the destructor, 1 Print(FILE*, depth), 2 Parse(p,
//     data, encoding), 3 StreamOut(TiXmlOutStream*), 4 Clone (nodes only). No TiXmlVisitor /
//     Accept and no virtual ToElement / ToText...: those test the node type (2.5 changed both)
//   - TiXmlDocument has useMicrosoftBOM; TiXmlElement::GetText; StringToBuffer in LoadFile /
//     SaveFile; ReadText returns p + strlen(endTag) unchecked; TiXmlDocument::CopyTo copies
//     only error and errorDesc (2.4's)
// Memory through the game's operator new / delete (0x5EEAA0 / 0x5EEAB0); the C runtime's
// FILE functions are the game's (its FILE table), the rest ours (src/game/Crt.h).
//
// Element::Parse, Attribute::Parse, Comment::Parse, Text::Parse and Declaration::Parse are at
// 0x85A010..0x85A7D0 (another chunk's).

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "reversed.h"

namespace tixml {

// ---- TiXmlString

struct Rep {
    uint32_t size;       // +0x0
    uint32_t capacity;   // +0x4
    char str[4];         // +0x8 (size + 1 used)
};
static_assert(offsetof(Rep, str) == 8);

struct TiXmlString {
    re::Ptr32<Rep> rep;   // nullrep_ when empty (never freed)
};
static_assert(sizeof(TiXmlString) == 4);

// TiXmlString::nullrep_ (static: {0, 0, ""})
inline Rep* const kNullRep = reinterpret_cast<Rep*>(0x020E6004);
constexpr uint32_t npos = 0xFFFFFFFF;

inline const char* CStr(const TiXmlString& s) {
    return s.rep->str;
}
inline uint32_t Length(const TiXmlString& s) {
    return s.rep->size;
}

// ---- the classes

struct TiXmlCursor {
    int32_t row;   // +0x0 (-1: none, Clear())
    int32_t col;   // +0x4
};
static_assert(sizeof(TiXmlCursor) == 8);

struct TiXmlBase {
    re::Ptr32<void> vtable;           // +0x0
    TiXmlCursor location;   // +0x4
    re::Ptr32<void> userData;         // +0xC
};
static_assert(sizeof(TiXmlBase) == 0x10);

enum NodeType : int32_t {
    DOCUMENT = 0,
    ELEMENT = 1,
    COMMENT = 2,
    UNKNOWN = 3,
    TEXT = 4,
    DECLARATION = 5,
};

struct TiXmlNode : TiXmlBase {
    re::Ptr32<TiXmlNode> parent;       // +0x10
    int32_t type;            // +0x14 (NodeType)
    re::Ptr32<TiXmlNode> firstChild;   // +0x18
    re::Ptr32<TiXmlNode> lastChild;    // +0x1C
    TiXmlString value;       // +0x20
    re::Ptr32<TiXmlNode> prev;         // +0x24
    re::Ptr32<TiXmlNode> next;         // +0x28
};
static_assert(offsetof(TiXmlNode, parent) == 0x10);
static_assert(offsetof(TiXmlNode, value) == 0x20);
static_assert(offsetof(TiXmlNode, next) == 0x28);
static_assert(sizeof(TiXmlNode) == 0x2C);

struct TiXmlDocument;

struct TiXmlAttribute : TiXmlBase {
    re::Ptr32<TiXmlDocument> document;   // +0x10 (for errors)
    TiXmlString name;          // +0x14
    TiXmlString value;         // +0x18
    re::Ptr32<TiXmlAttribute> prev;      // +0x1C
    re::Ptr32<TiXmlAttribute> next;      // +0x20
};
static_assert(offsetof(TiXmlAttribute, name) == 0x14);
static_assert(offsetof(TiXmlAttribute, next) == 0x20);
static_assert(sizeof(TiXmlAttribute) == 0x24);

// TiXmlAttributeSet: a ring through a sentinel attribute (no name, no value)
struct TiXmlAttributeSet {
    TiXmlAttribute sentinel;   // +0x0
};
static_assert(sizeof(TiXmlAttributeSet) == 0x24);

struct TiXmlElement : TiXmlNode {
    TiXmlAttributeSet attributeSet;   // +0x2C
};
static_assert(offsetof(TiXmlElement, attributeSet) == 0x2C);
static_assert(sizeof(TiXmlElement) == 0x50);

struct TiXmlComment : TiXmlNode {};
static_assert(sizeof(TiXmlComment) == 0x2C);

struct TiXmlUnknown : TiXmlNode {};
static_assert(sizeof(TiXmlUnknown) == 0x2C);

struct TiXmlText : TiXmlNode {
    uint8_t cdata;   // +0x2C
    uint8_t pad2D[3];
};
static_assert(sizeof(TiXmlText) == 0x30);

struct TiXmlDeclaration : TiXmlNode {
    TiXmlString version;      // +0x2C
    TiXmlString encoding;     // +0x30
    TiXmlString standalone;   // +0x34
};
static_assert(sizeof(TiXmlDeclaration) == 0x38);

struct TiXmlDocument : TiXmlNode {
    uint8_t error;                 // +0x2C
    uint8_t pad2D[3];
    int32_t errorId;               // +0x30
    TiXmlString errorDesc;         // +0x34
    int32_t tabsize;               // +0x38
    TiXmlCursor errorLocation;     // +0x3C
    uint8_t useMicrosoftBOM;       // +0x44
    uint8_t pad45[3];
};
static_assert(offsetof(TiXmlDocument, errorId) == 0x30);
static_assert(offsetof(TiXmlDocument, errorDesc) == 0x34);
static_assert(offsetof(TiXmlDocument, errorLocation) == 0x3C);
static_assert(offsetof(TiXmlDocument, useMicrosoftBOM) == 0x44);
static_assert(sizeof(TiXmlDocument) == 0x48);

struct TiXmlParsingData {
    TiXmlCursor cursor;   // +0x0
    re::Ptr32<const char> stamp;    // +0x8
    int32_t tabsize;      // +0xC
};
static_assert(sizeof(TiXmlParsingData) == 0x10);

// TiXmlBase::Entity (the five XML entities: &amp; &lt; &gt; &quot; &apos;)
struct Entity {
    re::Ptr32<const char> str;      // +0x0
    uint32_t strLength;   // +0x4
    char chr;             // +0x8
};
static_assert(sizeof(Entity) == 0xC);

enum Encoding : int32_t {
    TIXML_ENCODING_UNKNOWN = 0,
    TIXML_ENCODING_UTF8 = 1,
    TIXML_ENCODING_LEGACY = 2,
};

enum ErrorId : int32_t {
    TIXML_NO_ERROR = 0,
    TIXML_ERROR = 1,
    TIXML_ERROR_OPENING_FILE = 2,
    TIXML_ERROR_OUT_OF_MEMORY = 3,
    TIXML_ERROR_PARSING_ELEMENT = 4,
    TIXML_ERROR_FAILED_TO_READ_ELEMENT_NAME = 5,
    TIXML_ERROR_READING_ELEMENT_VALUE = 6,
    TIXML_ERROR_READING_ATTRIBUTES = 7,
    TIXML_ERROR_PARSING_EMPTY = 8,
    TIXML_ERROR_READING_END_TAG = 9,
    TIXML_ERROR_PARSING_UNKNOWN = 10,
    TIXML_ERROR_PARSING_COMMENT = 11,
    TIXML_ERROR_PARSING_DECLARATION = 12,
    TIXML_ERROR_DOCUMENT_EMPTY = 13,
    TIXML_ERROR_EMBEDDED_NULL = 14,
    TIXML_ERROR_PARSING_CDATA = 15,
};

// the virtual slots
enum Slot : int {
    kDestroy = 0,     // (flags: 1 deletes)
    kPrint = 1,       // (FILE*, int depth)
    kParse = 2,       // (const char* p, TiXmlParsingData* data, int encoding) -> const char*
    kStreamOut = 3,   // (TiXmlString* stream)
    kClone = 4,       // () -> TiXmlNode*
};

constexpr uintptr_t kTiXmlBaseVtable = 0x0098C3F4;
constexpr uintptr_t kTiXmlNodeVtable = 0x0098C468;
constexpr uintptr_t kTiXmlAttributeVtable = 0x0098C458;
constexpr uintptr_t kTiXmlCommentVtable = 0x0098C214;
constexpr uintptr_t kTiXmlTextVtable = 0x0098C228;
constexpr uintptr_t kTiXmlDeclarationVtable = 0x0098C4C4;
constexpr uintptr_t kTiXmlUnknownVtable = 0x0098C504;
constexpr uintptr_t kTiXmlElementVtable = 0x0098C518;
constexpr uintptr_t kTiXmlDocumentVtable = 0x008FFF84;   // (in the game's .rdata)

// the statics
GAME_VAR(uint8_t, TiXmlBase_condenseWhiteSpace, 0x00BC934C);
GAME_ARRAY(Entity, TiXmlBase_entity, 0x00BC9350);              // [5]
GAME_ARRAY(const char*, TiXmlBase_errorString, 0x00BC9390);    // [16]
GAME_ARRAY(const int32_t, TiXmlBase_utf8ByteTable, 0x0098C558);   // [256]
constexpr int kNumEntity = 5;

// ---- the game's functions this code calls (the C runtime's FILE side, operator new / delete)

GAME_FN(void*, __cdecl, OperatorNew, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, OperatorDelete, 0x005EEAB0, void* p);
// (not in src/game/Crt.h: the game's FILE table, as Crt.h's Fopen)
GAME_FN(int, __cdecl, Fprintf, 0x0085DD0D, FILE* file, const char* format, ...);
GAME_FN(int, __cdecl, Fputs, 0x00860796, const char* text, FILE* file);
// what fputc's calls reach in this exe: the game's shared "return 0" (the BOM isn't written)
GAME_FN(int, __cdecl, Fputc_824EC0, 0x00824EC0, int c, FILE* file);

// ---- TinyXML's own (this chunk's), called where modes apply

GAME_FN(void, __thiscall, String_Init, 0x00857030, TiXmlString* self, uint32_t size, uint32_t capacity);
GAME_FN(void, __thiscall, String_Reserve, 0x00857080, TiXmlString* self, uint32_t capacity);
GAME_FN(TiXmlString*, __thiscall, String_Assign, 0x008570E0, TiXmlString* self, const char* text, uint32_t length);
GAME_FN(TiXmlString*, __thiscall, String_Append, 0x00857190, TiXmlString* self, const char* text, uint32_t length);
GAME_FN(TiXmlString*, __thiscall, String_AppendCStr, 0x008571E0, TiXmlString* self, const char* text);
GAME_FN(void, __thiscall, Attribute_SetValue, 0x00857230, TiXmlAttribute* self, const char* value);
GAME_FN(void, __cdecl, Base_PutString, 0x00857260, const TiXmlString* text, TiXmlString* out);
GAME_FN(TiXmlNode*, __thiscall, Node_LinkEndChild, 0x008573C0, TiXmlNode* self, TiXmlNode* node);
GAME_FN(TiXmlNode*, __thiscall, Node_FirstChild, 0x008573F0, const TiXmlNode* self, const char* value);
GAME_FN(TiXmlNode*, __thiscall, Node_NextSibling, 0x00857440, const TiXmlNode* self, const char* value);
GAME_FN(TiXmlDocument*, __thiscall, Node_GetDocument, 0x00857570, const TiXmlNode* self);
GAME_FN(uint8_t, __thiscall, Document_SaveFileName, 0x008575B0, const TiXmlDocument* self, const char* file);
GAME_FN(void, __thiscall, Document_CopyTo, 0x00857620, const TiXmlDocument* self, TiXmlDocument* target);
GAME_FN(TiXmlAttribute*, __thiscall, AttributeSet_Find, 0x00857870, const TiXmlAttributeSet* self, const char* name);
GAME_FN(TiXmlAttribute*, __thiscall, Attribute_Construct, 0x008578C0, TiXmlAttribute* self, const char* name,
        const char* value);
GAME_FN(TiXmlNode*, __thiscall, Node_Construct, 0x00857980, TiXmlNode* self, int type);
GAME_FN(void, __thiscall, Node_Destruct, 0x008579C0, TiXmlNode* self);
GAME_FN(void, __thiscall, Element_ClearThis, 0x00857A00, TiXmlElement* self);
GAME_FN(void, __thiscall, Element_SetAttribute, 0x00857AB0, TiXmlElement* self, const char* name, const char* value);
GAME_FN(void, __thiscall, Element_CopyTo, 0x00857DA0, const TiXmlElement* self, TiXmlElement* target);
GAME_FN(TiXmlDocument*, __thiscall, Document_Construct, 0x00857E50, TiXmlDocument* self);
GAME_FN(uint8_t, __thiscall, Document_LoadFileName, 0x00857FC0, TiXmlDocument* self, const char* file, int encoding);
GAME_FN(void, __thiscall, Declaration_CopyTo, 0x00858820, const TiXmlDeclaration* self, TiXmlDeclaration* target);
GAME_FN(TiXmlDeclaration*, __thiscall, Declaration_Construct, 0x00858920, TiXmlDeclaration* self);
GAME_FN(TiXmlUnknown*, __thiscall, Unknown_Construct, 0x008589C0, TiXmlUnknown* self);
GAME_FN(TiXmlElement*, __thiscall, Element_Construct, 0x00858A50, TiXmlElement* self, const char* value);
GAME_FN(int, __cdecl, Base_IsAlpha, 0x00858F60, int c, int encoding);   // (an unsigned char)
GAME_FN(void, __cdecl, Base_ConvertUTF32ToUTF8, 0x00858E60, uint32_t input, char* output, int* length);
GAME_FN(void, __thiscall, ParsingData_Stamp, 0x00858F80, TiXmlParsingData* self, const char* now, int encoding);
GAME_FN(const char*, __cdecl, Base_SkipWhiteSpace, 0x008591A0, const char* p, int encoding);
GAME_FN(uint8_t, __cdecl, Base_StringEqual, 0x008592F0, const char* p, const char* tag, int ignoreCase, int encoding);
GAME_FN(void, __thiscall, Document_SetError, 0x008593A0, TiXmlDocument* self, int error, const char* where,
        TiXmlParsingData* data, int encoding);
GAME_FN(uint8_t, __thiscall, Text_Blank, 0x008594E0, const TiXmlText* self);
GAME_FN(const char*, __cdecl, Base_GetEntity, 0x00859530, const char* p, char* value, int* length, int encoding);
GAME_FN(TiXmlNode*, __thiscall, Node_Identify, 0x008596F0, TiXmlNode* self, const char* p, int encoding);
GAME_FN(const char*, __cdecl, Base_GetChar, 0x00859B20, const char* p, char* value, int* length, int encoding);
// (another chunk's)
GAME_FN(TiXmlComment*, __thiscall, Comment_Construct, 0x00854C60, TiXmlComment* self);
GAME_FN(TiXmlText*, __thiscall, Text_Construct, 0x00854CB0, TiXmlText* self, const char* value);

// ---- what the code inlines

// a TiXmlString's destructor
inline void FreeString(TiXmlString& s) {
    if (s.rep != kNullRep) {
        OperatorDelete(s.rep);
    }
}

// TiXmlString::operator=(const char*) / SetValue (strlen inlined, then assign)
inline void AssignCStr(TiXmlString* s, const char* text) {
    uint32_t length = 0;
    while (text[length] != '\0') {
        length++;
    }
    String_Assign(s, text, length);
}

// TiXmlNode's type tests (2.4's ToDocument / ToElement / ToText...: not virtual)
inline bool IsType(const TiXmlNode* node, int type) {
    return node->type == type;
}

template <typename R = void, typename... A>
inline R Virtual(const void* object, int slot, A... args) {
    re::Ptr32<void>* vtable = *static_cast<const re::Ptr32<re::Ptr32<void>>*>(object);
    return re::PtrCast<R(__thiscall*)(const void*, re::Arg<A>...)>(vtable[slot])(object, args...);
}

} // namespace tixml
