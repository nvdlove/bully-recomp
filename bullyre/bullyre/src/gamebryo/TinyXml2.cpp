// TinyXML 2.4.x's parsing side (TinyXml.h): TiXmlBase's statics (PutString, white space,
// names, entities, characters, text up to an end tag, UTF-32 to UTF-8), TiXmlParsingData::Stamp
// (the row / column of a position), TiXmlNode::Identify (what a '<' starts), TiXmlElement::
// ReadValue (an element's text and children), TiXmlUnknown::Parse, TiXmlText::Blank and
// TiXmlDocument's Parse, SetError and LoadFile. The other classes' Parse are at 0x85A010..
// (another chunk's).

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "game/Crt.h"
#include "gamebryo/TinyXml.h"
#include "reversed.h"

namespace tixml {
namespace {

// isalpha / isalnum in the "C" locale (not in Crt.h: ours, as Crt.h's character classes are)
inline int IsAlphaC(int c) {
    return std::isalpha(c);
}
inline int IsAlnumC(int c) {
    return std::isalnum(c);
}

// TiXmlBase::IsWhiteSpace, inlined
inline bool IsWhite(char c) {
    return crt::Isspace(uint8_t(c)) != 0 || c == '\n' || c == '\r';
}

// TiXmlBase::ToLower
inline int ToLower(int v, int encoding) {
    if (encoding == TIXML_ENCODING_UTF8) {
        if (v < 128) {
            return crt::Tolower(v);
        }
        return v;
    }
    return crt::Tolower(v);
}

// TiXmlBase::StringEqual where the code inlines it (p known not to be empty)
inline bool StringEqualInline(const char* p, const char* tag, bool ignoreCase, int encoding) {
    if (*p == '\0') {
        return false;
    }
    const char* q = p;
    if (ignoreCase) {
        while (*q != '\0' && *tag != '\0' && ToLower(*q, encoding) == ToLower(*tag, encoding)) {
            q++;
            tag++;
        }
    } else {
        while (*q != '\0' && *tag != '\0' && *q == *tag) {
            q++;
            tag++;
        }
    }
    return *tag == '\0';
}

// TiXmlDocument::SetError where the code inlines it (no position: the location cleared)
inline void SetErrorInline(TiXmlDocument* doc, int error) {
    if (doc->error != 0) {
        return;
    }
    doc->error = 1;
    doc->errorId = error;
    AssignCStr(&doc->errorDesc, TiXmlBase_errorString[error]);
    doc->errorLocation.col = -1;
    doc->errorLocation.row = -1;
}

inline void SetVtable(void* object, uintptr_t vtable) {
    *static_cast<uint32_t*>(object) = vtable;
}

} // namespace

// ---- TiXmlBase's statics

// 0x857260: TiXmlBase::PutString (the five entities and control characters as references;
// "&#x" references passed through to their ';')
void __cdecl Base_PutStringImpl(const TiXmlString* text, TiXmlString* out) {
    int i = 0;
    while (i < int(Length(*text))) {
        uint8_t c = uint8_t(CStr(*text)[i]);
        if (c == '&' && i < int(Length(*text)) - 2 && CStr(*text)[i + 1] == '#' && CStr(*text)[i + 2] == 'x') {
            while (i < int(Length(*text)) - 1) {
                String_Append(out, CStr(*text) + i, 1);
                i++;
                if (CStr(*text)[i] == ';') {
                    break;
                }
            }
        } else if (c == '&') {
            String_Append(out, TiXmlBase_entity[0].str, TiXmlBase_entity[0].strLength);
            i++;
        } else if (c == '<') {
            String_Append(out, TiXmlBase_entity[1].str, TiXmlBase_entity[1].strLength);
            i++;
        } else if (c == '>') {
            String_Append(out, TiXmlBase_entity[2].str, TiXmlBase_entity[2].strLength);
            i++;
        } else if (c == '"') {
            String_Append(out, TiXmlBase_entity[3].str, TiXmlBase_entity[3].strLength);
            i++;
        } else if (c == '\'') {
            String_Append(out, TiXmlBase_entity[4].str, TiXmlBase_entity[4].strLength);
            i++;
        } else if (c < 32) {
            char buffer[32];
            crt::Snprintf(buffer, sizeof(buffer), "&#x%02X;", unsigned(c));
            uint32_t length = 0;
            while (buffer[length] != '\0') {
                length++;
            }
            String_Append(out, buffer, length);
            i++;
        } else {
            char single = char(c);
            String_Append(out, &single, 1);
            i++;
        }
    }
}

// 0x858E30: TiXmlBase::IsWhiteSpace
int __cdecl Base_IsWhiteSpace(int c) {
    uint8_t b = uint8_t(c);
    if (crt::Isspace(b) != 0 || b == '\n' || b == '\r') {
        return 1;
    }
    return 0;
}

// 0x858E60: TiXmlBase::ConvertUTF32ToUTF8 (*length 0 past 0x1FFFFF)
void __cdecl Base_ConvertUTF32ToUTF8Impl(uint32_t input, char* output, int* length) {
    static const uint32_t kFirstByteMark[7] = {0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};
    if (input < 0x80) {
        *length = 1;
    } else if (input < 0x800) {
        *length = 2;
    } else if (input < 0x10000) {
        *length = 3;
    } else if (input < 0x200000) {
        *length = 4;
    } else {
        *length = 0;
        return;
    }
    output += *length;
    switch (*length) {
    case 4:
        *--output = char((input | 0x80) & 0xBF);
        input >>= 6;
        [[fallthrough]];
    case 3:
        *--output = char((input | 0x80) & 0xBF);
        input >>= 6;
        [[fallthrough]];
    case 2:
        *--output = char((input | 0x80) & 0xBF);
        input >>= 6;
        [[fallthrough]];
    case 1:
        *--output = char(input | kFirstByteMark[*length]);
        break;
    default:
        break;
    }
}

// 0x858F60: TiXmlBase::IsAlpha (anything past ASCII counts as a letter)
int __cdecl Base_IsAlphaImpl(int c, int) {
    uint8_t b = uint8_t(c);
    if (b < 127) {
        return IsAlphaC(b);
    }
    return 1;
}

// 0x8591A0: TiXmlBase::SkipWhiteSpace (UTF-8: the byte order marks too). (The other branch keeps
// 2.4's precedence: *p && IsWhiteSpace(*p) || *p == '\n' || *p == '\r'.)
const char* __cdecl Base_SkipWhiteSpaceImpl(const char* p, int encoding) {
    if (p == nullptr || *p == '\0') {
        return nullptr;
    }
    if (encoding == TIXML_ENCODING_UTF8) {
        while (*p != '\0') {
            const uint8_t* u = reinterpret_cast<const uint8_t*>(p);
            if (u[0] == 0xEF && u[1] == 0xBB && u[2] == 0xBF) {
                p += 3;
                continue;
            }
            if (u[0] == 0xEF && u[1] == 0xBF && u[2] == 0xBE) {
                p += 3;
                continue;
            }
            if (u[0] == 0xEF && u[1] == 0xBF && u[2] == 0xBF) {
                p += 3;
                continue;
            }
            if (IsWhite(*p) || *p == '\n' || *p == '\r') {
                p++;
            } else {
                break;
            }
        }
    } else {
        while ((*p != '\0' && IsWhite(*p)) || *p == '\n' || *p == '\r') {
            p++;
        }
    }
    return p;
}

// 0x859260: TiXmlBase::ReadName (a letter or '_', then letters, digits, '_', '-', '.', ':';
// null when it doesn't start with one)
const char* __cdecl Base_ReadName(const char* p, TiXmlString* name, int) {
    String_Assign(name, "", 0);
    if (p == nullptr || *p == '\0') {
        return nullptr;
    }
    uint8_t first = uint8_t(*p);
    if (!(first >= 127 || IsAlphaC(first) != 0 || *p == '_')) {
        return nullptr;
    }
    while (p != nullptr && *p != '\0') {
        uint8_t c = uint8_t(*p);
        if (!(c >= 127 || IsAlnumC(c) != 0 || *p == '_' || *p == '-' || *p == '.' || *p == ':')) {
            break;
        }
        char single = *p;
        String_Append(name, &single, 1);
        p++;
    }
    return p;
}

// 0x8592F0: TiXmlBase::StringEqual (does p start with tag?)
uint8_t __cdecl Base_StringEqualImpl(const char* p, const char* tag, int ignoreCase, int encoding) {
    if (p == nullptr || *p == '\0') {
        return 0;
    }
    return StringEqualInline(p, tag, re::Flag(ignoreCase), encoding) ? 1 : 0;
}

// 0x859530: TiXmlBase::GetEntity (&#x..; / &#..; as UTF-8 or a byte, the five named ones; else
// the '&' itself)
const char* __cdecl Base_GetEntityImpl(const char* p, char* value, int* length, int encoding) {
    *length = 0;
    if (p[1] != '\0' && p[1] == '#' && p[2] != '\0') {
        uint32_t ucs = 0;
        ptrdiff_t delta = 0;
        uint32_t mult = 1;
        if (p[2] == 'x') {
            if (p[3] == '\0') {
                return nullptr;
            }
            const char* q = crt::Strchr(p + 3, ';');
            if (q == nullptr || *q == '\0') {
                return nullptr;
            }
            delta = q - p;
            q--;
            while (*q != 'x') {
                if (*q >= '0' && *q <= '9') {
                    ucs += mult * uint32_t(*q - '0');
                } else if (*q >= 'a' && *q <= 'f') {
                    ucs += mult * uint32_t(*q - 'a' + 10);
                } else if (*q >= 'A' && *q <= 'F') {
                    ucs += mult * uint32_t(*q - 'A' + 10);
                } else {
                    return nullptr;
                }
                mult *= 16;
                q--;
            }
        } else {
            const char* q = crt::Strchr(p + 2, ';');
            if (q == nullptr || *q == '\0') {
                return nullptr;
            }
            delta = q - p;
            q--;
            while (*q != '#') {
                if (*q >= '0' && *q <= '9') {
                    ucs += mult * uint32_t(*q - '0');
                } else {
                    return nullptr;
                }
                mult *= 10;
                q--;
            }
        }
        if (encoding == TIXML_ENCODING_UTF8) {
            Base_ConvertUTF32ToUTF8(ucs, value, length);
        } else {
            *value = char(ucs);
            *length = 1;
        }
        return p + delta + 1;
    }
    for (int i = 0; i < kNumEntity; i++) {
        if (crt::Strncmp(TiXmlBase_entity[i].str, p, TiXmlBase_entity[i].strLength) == 0) {
            *value = TiXmlBase_entity[i].chr;
            *length = 1;
            return p + TiXmlBase_entity[i].strLength;
        }
    }
    *value = *p;
    return p + 1;
}

// 0x859B20: TiXmlBase::GetChar (one character: a UTF-8 sequence, an entity, or the byte)
const char* __cdecl Base_GetCharImpl(const char* p, char* value, int* length, int encoding) {
    if (encoding == TIXML_ENCODING_UTF8) {
        *length = TiXmlBase_utf8ByteTable[uint8_t(*p)];
    } else {
        *length = 1;
    }
    if (*length == 1) {
        if (*p == '&') {
            return Base_GetEntity(p, value, length, encoding);
        }
        *value = *p;
        return p + 1;
    }
    if (*length != 0) {
        for (int i = 0; p[i] != '\0' && i < *length; i++) {
            value[i] = p[i];
        }
        return p + *length;
    }
    return nullptr;
}

// 0x859BB0: TiXmlBase::ReadText (up to endTag: kept as it is, or with its white space condensed
// to single spaces; returns past the tag, null or not: 2.4's)
const char* __cdecl Base_ReadText(const char* p, TiXmlString* text, int trimWhiteSpace, const char* endTag,
                                  int ignoreCase, int encoding) {
    String_Assign(text, "", 0);
    if (!re::Flag(trimWhiteSpace) || TiXmlBase_condenseWhiteSpace == 0) {
        while (p != nullptr && *p != '\0' && Base_StringEqual(p, endTag, ignoreCase, encoding) == 0) {
            int length;
            char chars[4] = {0, 0, 0, 0};
            p = Base_GetChar(p, chars, &length, encoding);
            String_Append(text, chars, uint32_t(length));
        }
    } else {
        bool whitespace = false;
        p = Base_SkipWhiteSpace(p, encoding);
        while (p != nullptr && *p != '\0' && Base_StringEqual(p, endTag, ignoreCase, encoding) == 0) {
            if (*p == '\r' || *p == '\n') {
                whitespace = true;
                p++;
            } else if (IsWhite(*p)) {
                whitespace = true;
                p++;
            } else {
                if (whitespace) {
                    char space = ' ';
                    String_Append(text, &space, 1);
                    whitespace = false;
                }
                int length;
                char chars[4] = {0, 0, 0, 0};
                p = Base_GetChar(p, chars, &length, encoding);
                if (length == 1) {
                    char single = chars[0];
                    String_Append(text, &single, 1);
                } else {
                    String_Append(text, chars, uint32_t(length));
                }
            }
        }
    }
    uint32_t tagLength = 0;
    while (endTag[tagLength] != '\0') {
        tagLength++;
    }
    return p + tagLength;
}

// ---- TiXmlParsingData

// 0x858F80 (thiscall): TiXmlParsingData::Stamp (the cursor moved on from the last stamp to now:
// tabs to the next stop, \r\n / \n\r one line, UTF-8 one column a character, the byte order
// marks none; a null byte stops it without saving). (A UTF-8 lead byte with a null after it
// doesn't move on: 2.4's endless loop.)
void __fastcall ParsingData_StampImpl(TiXmlParsingData* self, re::Edx, const char* now, int encoding) {
    if (self->tabsize < 1) {
        return;
    }
    int32_t row = self->cursor.row;
    int32_t col = self->cursor.col;
    const char* p = self->stamp;
    while (p < now) {
        const uint8_t* u = reinterpret_cast<const uint8_t*>(p);
        switch (*u) {
        case 0:
            return;
        case '\r':
            row++;
            col = 0;
            p++;
            if (*p == '\n') {
                p++;
            }
            break;
        case '\n':
            row++;
            col = 0;
            p++;
            if (*p == '\r') {
                p++;
            }
            break;
        case '\t':
            p++;
            col = (col / self->tabsize + 1) * self->tabsize;
            break;
        case 0xEF:
            if (encoding == TIXML_ENCODING_UTF8) {
                if (p[1] != '\0' && p[2] != '\0') {
                    if (u[1] == 0xBB && u[2] == 0xBF) {
                        p += 3;
                    } else if (u[1] == 0xBF && u[2] == 0xBE) {
                        p += 3;
                    } else if (u[1] == 0xBF && u[2] == 0xBF) {
                        p += 3;
                    } else {
                        p += 3;
                        col++;
                    }
                }
            } else {
                p++;
                col++;
            }
            break;
        default:
            if (encoding == TIXML_ENCODING_UTF8) {
                int step = TiXmlBase_utf8ByteTable[*u];
                if (step == 0) {
                    step = 1;
                }
                p += step;
                col++;
            } else {
                p++;
                col++;
            }
            break;
        }
    }
    self->cursor.row = row;
    self->cursor.col = col;
    self->stamp = p;
}

// ---- the nodes

// 0x8596F0 (thiscall): TiXmlNode::Identify (a declaration, comment, CDATA text, DTD / unknown,
// or element, by what follows the '<'; its parent set for the errors)
TiXmlNode* __fastcall Node_IdentifyImpl(TiXmlNode* self, re::Edx, const char* p, int encoding) {
    p = Base_SkipWhiteSpace(p, encoding);
    if (p == nullptr || *p == '\0' || *p != '<') {
        return nullptr;
    }
    TiXmlDocument* doc = Node_GetDocument(self);
    p = Base_SkipWhiteSpace(p, encoding);
    if (p == nullptr || *p == '\0') {
        return nullptr;
    }
    TiXmlNode* node = nullptr;
    void* memory;
    if (Base_StringEqual(p, "<?xml", 1, encoding) != 0) {
        memory = OperatorNew(0x38);
        if (memory != nullptr) {
            node = Declaration_Construct(static_cast<TiXmlDeclaration*>(memory));
        }
    } else if (StringEqualInline(p, "<!--", false, encoding)) {
        memory = OperatorNew(0x2C);
        if (memory != nullptr) {
            node = Comment_Construct(static_cast<TiXmlComment*>(memory));
        }
    } else if (StringEqualInline(p, "<![CDATA[", false, encoding)) {
        memory = OperatorNew(0x30);
        if (memory != nullptr) {
            TiXmlText* text = Text_Construct(static_cast<TiXmlText*>(memory), "");
            text->cdata = 1;
            node = text;
        } else {
            *reinterpret_cast<volatile uint8_t*>(0x2C) = 1;   // (SetCDATA on the null: the game's crash)
        }
    } else if (StringEqualInline(p, "<!", false, encoding)) {
        memory = OperatorNew(0x2C);
        if (memory != nullptr) {
            node = Unknown_Construct(static_cast<TiXmlUnknown*>(memory));
        }
    } else if (Base_IsAlpha(uint8_t(p[1]), encoding) != 0 || p[1] == '_') {
        memory = OperatorNew(0x50);
        if (memory != nullptr) {
            node = Element_Construct(static_cast<TiXmlElement*>(memory), "");
        }
    } else {
        memory = OperatorNew(0x2C);
        if (memory != nullptr) {
            node = Unknown_Construct(static_cast<TiXmlUnknown*>(memory));
        }
    }
    if (node != nullptr) {
        node->parent = self;
        return node;
    }
    if (doc != nullptr) {
        Document_SetError(doc, TIXML_ERROR_OUT_OF_MEMORY, nullptr, nullptr, TIXML_ENCODING_UNKNOWN);
    }
    return nullptr;
}

// 0x8598F0 (thiscall): TiXmlElement::ReadValue (text and elements in any order, up to "</")
const char* __fastcall Element_ReadValue(TiXmlElement* self, re::Edx, const char* p, TiXmlParsingData* data,
                                         int encoding) {
    TiXmlDocument* doc = Node_GetDocument(self);
    const char* withWhiteSpace = p;
    p = Base_SkipWhiteSpace(p, encoding);
    while (p != nullptr) {
        if (*p == '\0') {
            return p;
        }
        if (*p != '<') {
            void* memory = OperatorNew(0x30);
            if (memory == nullptr) {
                if (doc != nullptr) {
                    SetErrorInline(doc, TIXML_ERROR_OUT_OF_MEMORY);
                }
                return nullptr;
            }
            // TiXmlText("") inlined
            TiXmlText* text = static_cast<TiXmlText*>(Node_Construct(static_cast<TiXmlNode*>(memory), TEXT));
            SetVtable(text, kTiXmlTextVtable);
            String_Assign(&text->value, "", 0);
            text->cdata = 0;
            if (TiXmlBase_condenseWhiteSpace != 0) {
                p = Virtual<const char*>(text, kParse, p, data, encoding);
            } else {
                p = Virtual<const char*>(text, kParse, withWhiteSpace, data, encoding);
            }
            if (Text_Blank(text) != 0) {
                Virtual<void*>(text, kDestroy, 1);
            } else {
                Node_LinkEndChild(self, text);
            }
        } else {
            if (StringEqualInline(p, "</", false, encoding)) {
                return p;
            }
            TiXmlNode* node = Node_Identify(self, p, encoding);
            if (node == nullptr) {
                return nullptr;
            }
            p = Virtual<const char*>(node, kParse, p, data, encoding);
            Node_LinkEndChild(self, node);
        }
        withWhiteSpace = p;
        p = Base_SkipWhiteSpace(p, encoding);
    }
    if (doc != nullptr) {
        SetErrorInline(doc, TIXML_ERROR_READING_ELEMENT_VALUE);
    }
    return p;
}

// 0x859410 (thiscall): TiXmlUnknown::Parse (everything up to the '>')
const char* __fastcall Unknown_Parse(TiXmlUnknown* self, re::Edx, const char* p, TiXmlParsingData* data, int encoding) {
    TiXmlDocument* doc = Node_GetDocument(self);
    p = Base_SkipWhiteSpace(p, encoding);
    if (data != nullptr) {
        // (callers read the cursor through ecx after Stamp: the game's Stamp keeps it)
        ParsingData_Stamp(data, p, encoding);
        self->location = data->cursor;
    }
    if (p == nullptr || *p == '\0' || *p != '<') {
        if (doc != nullptr) {
            Document_SetError(doc, TIXML_ERROR_PARSING_UNKNOWN, p, data, encoding);
        }
        return nullptr;
    }
    p++;
    String_Assign(&self->value, "", 0);
    while (p != nullptr && *p != '\0' && *p != '>') {
        char single = *p;
        String_Append(&self->value, &single, 1);
        p++;
    }
    if (p == nullptr) {
        if (doc != nullptr) {
            Document_SetError(doc, TIXML_ERROR_PARSING_UNKNOWN, nullptr, nullptr, encoding);
        }
    }
    if (*p == '>') {
        return p + 1;
    }
    return p;
}

// 0x8594E0 (thiscall): TiXmlText::Blank (white space only)
uint8_t __fastcall Text_BlankImpl(const TiXmlText* self, re::Edx) {
    for (uint32_t i = 0; i < Length(self->value); i++) {
        if (!IsWhite(CStr(self->value)[i])) {
            return 0;
        }
    }
    return 1;
}

// ---- TiXmlDocument

// 0x8593A0 (thiscall): TiXmlDocument::SetError (the first error kept; its position from the
// parsing data)
void __fastcall Document_SetErrorImpl(TiXmlDocument* self, re::Edx, int error, const char* where,
                                      TiXmlParsingData* data, int encoding) {
    if (self->error != 0) {
        return;
    }
    self->errorId = error;
    self->error = 1;
    AssignCStr(&self->errorDesc, TiXmlBase_errorString[error]);
    self->errorLocation.col = -1;
    self->errorLocation.row = -1;
    if (where != nullptr && data != nullptr) {
        ParsingData_Stamp(data, where, encoding);
        self->errorLocation = data->cursor;
    }
}

// 0x859D70 (thiscall): TiXmlDocument::Parse (the nodes at the top level; the encoding from a
// BOM or the declaration)
const char* __fastcall Document_Parse(TiXmlDocument* self, re::Edx, const char* p, TiXmlParsingData* previous,
                                      int encoding) {
    // ClearError
    self->error = 0;
    self->errorId = 0;
    String_Assign(&self->errorDesc, "", 0);
    self->errorLocation.col = 0;
    self->errorLocation.row = 0;
    if (p == nullptr || *p == '\0') {
        SetErrorInline(self, TIXML_ERROR_DOCUMENT_EMPTY);
        return nullptr;
    }
    self->location.col = -1;
    self->location.row = -1;
    if (previous != nullptr) {
        self->location.row = previous->cursor.row;
        self->location.col = previous->cursor.col;
    } else {
        self->location.row = 0;
        self->location.col = 0;
    }
    TiXmlParsingData data;
    data.tabsize = self->tabsize;
    data.stamp = p;
    data.cursor.row = self->location.row;
    data.cursor.col = self->location.col;
    self->location = data.cursor;
    if (encoding == TIXML_ENCODING_UNKNOWN) {
        const uint8_t* u = reinterpret_cast<const uint8_t*>(p);
        if (u[0] != 0 && u[0] == 0xEF && u[1] != 0 && u[1] == 0xBB && u[2] != 0 && u[2] == 0xBF) {
            encoding = TIXML_ENCODING_UTF8;
            self->useMicrosoftBOM = 1;
        }
    }
    p = Base_SkipWhiteSpace(p, encoding);
    if (p == nullptr) {
        SetErrorInline(self, TIXML_ERROR_DOCUMENT_EMPTY);
        return nullptr;
    }
    while (p != nullptr && *p != '\0') {
        TiXmlNode* node = Node_Identify(self, p, encoding);
        if (node == nullptr) {
            break;
        }
        p = Virtual<const char*>(node, kParse, p, &data, encoding);
        Node_LinkEndChild(self, node);
        if (encoding == TIXML_ENCODING_UNKNOWN && IsType(node, DECLARATION)) {
            const char* enc = CStr(static_cast<TiXmlDeclaration*>(node)->encoding);
            if (*enc == '\0') {
                encoding = TIXML_ENCODING_UTF8;
            } else if (StringEqualInline(enc, "UTF-8", true, TIXML_ENCODING_UNKNOWN)) {
                encoding = TIXML_ENCODING_UTF8;
            } else if (StringEqualInline(enc, "UTF8", true, TIXML_ENCODING_UNKNOWN)) {
                encoding = TIXML_ENCODING_UTF8;
            } else {
                encoding = TIXML_ENCODING_LEGACY;
            }
        }
        p = Base_SkipWhiteSpace(p, encoding);
    }
    if (self->firstChild == nullptr) {
        SetErrorInline(self, TIXML_ERROR_DOCUMENT_EMPTY);
        return nullptr;
    }
    return p;
}

// 0x857FC0 (thiscall): TiXmlDocument::LoadFile(name, encoding) (read whole, in binary; line ends
// made "\n"; then parsed)
uint8_t __fastcall Document_LoadFileNameImpl(TiXmlDocument* self, re::Edx, const char* name, int encoding) {
    // Clear
    TiXmlNode* node = self->firstChild;
    while (node != nullptr) {
        TiXmlNode* temp = node;
        node = node->next;
        Virtual<void*>(temp, kDestroy, 1);
    }
    self->firstChild = nullptr;
    self->lastChild = nullptr;
    self->location.col = -1;
    self->location.row = -1;
    AssignCStr(&self->value, name);
    FILE* file = crt::Fopen(CStr(self->value), "rb");
    if (file == nullptr) {
        Document_SetError(self, TIXML_ERROR_OPENING_FILE, nullptr, nullptr, TIXML_ENCODING_UNKNOWN);
        return 0;
    }
    crt::Fseek(file, 0, SEEK_END);
    long length = crt::Ftell(file);
    crt::Fseek(file, 0, SEEK_SET);
    if (length == 0) {
        crt::Fclose(file);
        return 0;
    }
    TiXmlString data = {kNullRep};
    String_Reserve(&data, uint32_t(length));
    char* buffer = static_cast<char*>(OperatorNew(size_t(length + 1)));
    buffer[0] = '\0';
    if (crt::Fread(buffer, size_t(length), 1, file) != 1) {
        Document_SetError(self, TIXML_ERROR_OPENING_FILE, nullptr, nullptr, TIXML_ENCODING_UNKNOWN);
        crt::Fclose(file);
        FreeString(data);   // (the buffer leaks: 2.4's)
        return 0;
    }
    crt::Fclose(file);
    buffer[length] = '\0';
    const char* last = buffer;
    const char* p = buffer;
    while (*p != '\0') {
        if (*p == '\n') {
            String_Append(&data, last, uint32_t(p - last + 1));
            p++;
            last = p;
        } else if (*p == '\r') {
            if (p - last > 0) {
                String_Append(&data, last, uint32_t(p - last));
            }
            char newline = '\n';
            String_Append(&data, &newline, 1);
            if (p[1] == '\n') {
                p += 2;
            } else {
                p++;
            }
            last = p;
        } else {
            p++;
        }
    }
    if (p - last != 0) {
        String_Append(&data, last, uint32_t(p - last));
    }
    OperatorDelete(buffer);
    Virtual<const char*>(self, kParse, CStr(data), static_cast<TiXmlParsingData*>(nullptr), encoding);
    uint8_t ok = self->error == 0 ? 1 : 0;
    FreeString(data);
    return ok;
}

#define TIXML38B_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

TIXML38B_REVERSED("TiXmlBase::PutString", 0x00857260, Base_PutStringImpl);
TIXML38B_REVERSED("TiXmlDocument::LoadFileNamed", 0x00857FC0, Document_LoadFileNameImpl);
TIXML38B_REVERSED("TiXmlBase::IsWhiteSpace", 0x00858E30, Base_IsWhiteSpace);
TIXML38B_REVERSED("TiXmlBase::ConvertUTF32ToUTF8", 0x00858E60, Base_ConvertUTF32ToUTF8Impl);
TIXML38B_REVERSED("TiXmlBase::IsAlpha", 0x00858F60, Base_IsAlphaImpl);
// (callers (SetError, Unknown::Parse, the other Parse functions) read the cursor through ecx
// after the call: the game's keeps ecx, ours needn't. Turn on with its callers)
TIXML38B_REVERSED("TiXmlParsingData::Stamp", 0x00858F80, ParsingData_StampImpl);
TIXML38B_REVERSED("TiXmlBase::SkipWhiteSpace", 0x008591A0, Base_SkipWhiteSpaceImpl);
TIXML38B_REVERSED("TiXmlBase::ReadName", 0x00859260, Base_ReadName);
TIXML38B_REVERSED("TiXmlBase::StringEqual", 0x008592F0, Base_StringEqualImpl);
TIXML38B_REVERSED("TiXmlDocument::SetError", 0x008593A0, Document_SetErrorImpl);
TIXML38B_REVERSED("TiXmlUnknown::Parse", 0x00859410, Unknown_Parse);
TIXML38B_REVERSED("TiXmlText::Blank", 0x008594E0, Text_BlankImpl);
TIXML38B_REVERSED("TiXmlBase::GetEntity", 0x00859530, Base_GetEntityImpl);
TIXML38B_REVERSED("TiXmlNode::Identify", 0x008596F0, Node_IdentifyImpl);
TIXML38B_REVERSED("TiXmlElement::ReadValue", 0x008598F0, Element_ReadValue);
TIXML38B_REVERSED("TiXmlBase::GetChar", 0x00859B20, Base_GetCharImpl);
TIXML38B_REVERSED("TiXmlBase::ReadText", 0x00859BB0, Base_ReadText);
TIXML38B_REVERSED("TiXmlDocument::Parse", 0x00859D70, Document_Parse);

} // namespace tixml
