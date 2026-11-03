// NiUIAtlasMap, the UI skin's description (vtable 0x989C68, embedded in NiUIManager at +0x9C,
// 0x5DC bytes; no NiRTTI: named after Gamebryo 2.x's sample framework): the skin texture (+4),
// two tables of 43 rects (+8 the outer ones, +0x2B8 the inner ones), the three font names for
// low, medium and high resolutions (+0x568, NiStrings) and the colours. Here: its destructor, the
// rect lookups and the helpers its file parser (0x83BC30, another pass's) reads "[FILES]",
// "[IMAGERECTS]" and "[COLORS]" sections with (the tokens matched without case by NiStrnicmp).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSampleFramework.h"
#include "reversed.h"

namespace ni::sample {
namespace {

GAME_FN(int, __cdecl, NiStrnicmp, 0x00753830, const char* a, const char* b, size_t n);
GAME_FN(re::Ptr32<char>*, __thiscall, NiString_AssignText, 0x007745E0, re::Ptr32<char>* self, const char* text);
GAME_FN(void, __thiscall, NiString_Destruct, 0x00774240, re::Ptr32<char>* self);
GAME_FN(float*, __thiscall, AtlasMap_ToPixels, 0x0083C1D0, const void* self, float* result, const float* rect);
GAME_FN(float*, __thiscall, AtlasMap_ToUV, 0x0083C270, const void* self, float* result, const float* rect);

GAME_ARRAY(const char*, kSectionNames, 0x00BC9258);   // FILES, IMAGERECTS, COLORS
GAME_ARRAY(const char*, kTextNames, 0x00BC9264);      // TEXT_ON_KEYBOARD_BUTTON... (5)
GAME_ARRAY(const char*, kImageNames, 0x00BC9278);     // GAMEPAD_BUTTON_RRIGHT... (43)
GAME_ARRAY(const char*, kFontNames, 0x00BC9324);      // TEXT_FOR_LOW_RESOLUTION... (3)

constexpr uint32_t kNiUIAtlasMapVtable = 0x00989C68;

// a rect of one of the tables: as stored (pixels), mode 1 and 2 converted
template <int Table>
float* __fastcall GetRect(const uint8_t* self, re::Edx, float* result, uint32_t index, int mode) {
    const float* rect = reinterpret_cast<const float*>(self + Table + index * 16);
    if (mode == 1) {
        AtlasMap_ToPixels(self, result, rect);
        return result;
    }
    if (mode == 2) {
        AtlasMap_ToUV(self, result, rect);
        return result;
    }
    result[0] = rect[0];
    result[1] = rect[1];
    result[2] = rect[2];
    result[3] = rect[3];
    return result;
}

// a section header ("[name]"): its index (-1 none)
uint32_t __fastcall ParseSection(void*, re::Edx, const re::Ptr32<const char>* cursor) {
    const char* line = *cursor;
    size_t length = std::strlen(line);
    if (line[0] == '[' && line[length - 1] == ']') {
        for (uint32_t i = 0; i < 3; i++) {
            if (NiStrnicmp(kSectionNames[i], *cursor + 1, length - 2) == 0) {
                return i;
            }
        }
    }
    return 0xFFFFFFFF;
}

// a name from a table at the cursor: its index, the cursor past it (-1 none)
template <uintptr_t Table, uint32_t Count>
uint32_t __fastcall ParseName(void*, re::Edx, re::Ptr32<const char>* cursor) {
    const re::Ptr32<const char>* names = reinterpret_cast<const re::Ptr32<const char>*>(Table);
    for (uint32_t i = 0; i < Count; i++) {
        const char* name = names[i];
        size_t length = std::strlen(name);
        if (NiStrnicmp(name, *cursor, length) == 0) {
            *cursor += length;
            return i;
        }
    }
    return 0xFFFFFFFF;
}

// a string value: blanks skipped, one trailing blank cut (the length isn't moved on: only the
// last character is checked, the game's), kept when not empty
uint8_t __fastcall ParseString(void*, re::Edx, re::Ptr32<char>* cursor, re::Ptr32<char>* out) {
    while ((**cursor == ' ' || **cursor == '\t') && **cursor != '\0') {
        (*cursor)++;
    }
    if (**cursor == '\0') {
        return 0;
    }
    size_t length = std::strlen(*cursor);
    while (true) {
        char* last = *cursor + length - 1;
        if ((*last != ' ' && *last != '\t') || length == 0) {
            break;
        }
        *last = '\0';
    }
    if (**cursor == '\0') {
        return 0;
    }
    NiString_AssignText(out, *cursor);
    return 1;
}

// an integer value: blanks and commas skipped, read, the digits skipped (a sign isn't)
uint8_t __fastcall ParseInt(void*, re::Edx, re::Ptr32<const char>* cursor, long* out) {
    while ((**cursor == ' ' || **cursor == '\t' || **cursor == ',') && **cursor != '\0') {
        (*cursor)++;
    }
    if (**cursor == '\0') {
        return 0;
    }
    *out = crt::Atol(*cursor);
    while (**cursor >= '0' && **cursor <= '9') {
        (*cursor)++;
    }
    return 1;
}

// a float value: blanks and commas skipped, read, the digits and points skipped
uint8_t __fastcall ParseFloat(void*, re::Edx, re::Ptr32<const char>* cursor, float* out) {
    while ((**cursor == ' ' || **cursor == '\t' || **cursor == ',') && **cursor != '\0') {
        (*cursor)++;
    }
    if (**cursor == '\0') {
        return 0;
    }
    *out = float(crt::Atof(*cursor));
    while (**cursor == '.' || (**cursor >= '0' && **cursor <= '9')) {
        (*cursor)++;
    }
    return 1;
}

// the destructor: the texture released, the font names destroyed (last first), the texture
// released again (null by then)
void __fastcall Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kNiUIAtlasMapVtable;
    re::Ptr32<NiRefObject>& texture = At<NiRefObject*>(self, 4);
    ClearRef(texture);
    for (int i = 2; i >= 0; i--) {
        NiString_Destruct(reinterpret_cast<re::Ptr32<char>*>(self + 0x568 + i * 4));
    }
    Release(texture);
}

} // namespace

#define NIUIATLAS_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                                \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIUIATLAS_REVERSED("NiUIAtlasMap::GetOuterRect", 0x00832EF0, &GetRect<8>);
NIUIATLAS_REVERSED("NiUIAtlasMap::GetInnerRect", 0x00839060, &GetRect<0x2B8>);
NIUIATLAS_REVERSED("NiUIAtlasMap::ParseSection", 0x0083B8E0, &ParseSection);
NIUIATLAS_REVERSED("NiUIAtlasMap::ParseImageName", 0x0083B950, (&ParseName<0x00BC9278, 0x2B>));
NIUIATLAS_REVERSED("NiUIAtlasMap::ParseTextName", 0x0083B9B0, (&ParseName<0x00BC9264, 5>));
NIUIATLAS_REVERSED("NiUIAtlasMap::ParseFontName", 0x0083BA10, (&ParseName<0x00BC9324, 3>));
NIUIATLAS_REVERSED("NiUIAtlasMap::ParseString", 0x0083BA70, &ParseString);
NIUIATLAS_REVERSED("NiUIAtlasMap::ParseInt", 0x0083BAF0, &ParseInt);
NIUIATLAS_REVERSED("NiUIAtlasMap::ParseFloat", 0x0083BB50, &ParseFloat);
NIUIATLAS_REVERSED("NiUIAtlasMap::~NiUIAtlasMap", 0x0083BBB0, &Destruct);

} // namespace ni::sample
