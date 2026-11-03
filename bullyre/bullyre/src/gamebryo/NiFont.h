#pragma once

// Gamebryo's NiFont as Bully's exe has it: a bitmap font loaded from an .nff file (NiFont), a
// string laid out in it (NiFontString) and one drawn on screen through a screen texture
// (NiString2D). Layouts read from the constructors (NiFont 0x8D7B90, NiFontString 0x8D84C0,
// NiString2D 0x8D6D10) and the loaders, checked with static_asserts.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni::font {

// NiTMap<unsigned short, unsigned short> (vtable 0x9A1F64 over the base's 0x9A1F50, 0x10): a
// character to its glyph's index
struct GlyphMapItem {
    re::Ptr32<GlyphMapItem> next;   // +0x0
    uint16_t key;         // +0x4
    uint16_t value;       // +0x6
    uint16_t field8;      // +0x8
};
static_assert(sizeof(GlyphMapItem) == 0xC);
struct GlyphMap {
    re::Ptr32<void> vtable;              // +0x0 (1 SetValue, 3 NewItem, 4 DeleteItem)
    uint32_t hashSize;         // +0x4
    re::Ptr32<re::Ptr32<GlyphMapItem>> table;      // +0x8
    uint32_t count;            // +0xC
};
static_assert(sizeof(GlyphMap) == 0x10);

// NiFont (vtable 0x9A1F78, 0xC4)
struct NiFont : NiRefObject {
    re::Ptr32<NiRefObject> renderer;            // +0x8 (a reference)
    char name[0x80];                  // +0xC
    uint32_t height;                  // +0x8C
    uint32_t charHeight;              // +0x90
    uint16_t lineHeight;              // +0x94
    uint16_t field96;                 // +0x96
    uint32_t flags;                   // +0x98
    uint32_t textureCount;            // +0x9C
    uint32_t glyphCount;              // +0xA0
    re::Ptr32<GlyphMap> glyphMap;               // +0xA4
    re::Ptr32<uint8_t> glyphTextures;           // +0xA8 (each glyph's texture; none: texture 0)
    re::Ptr32<uint16_t[4]> glyphRects;   // +0xAC (left, top, right, bottom, in pixels)
    NiTArray<NiRefObject*> textures;  // +0xB0 (NiTObjectArray<NiSourceTexturePtr>, vtable 0x9893C0)
    uint32_t lastError;               // +0xC0
};
static_assert(sizeof(NiFont) == 0xC4);
static_assert(offsetof(NiFont, textures) == 0xB0);

// NiFontString (vtable 0x9A1FD8, 0x48)
struct NiFontString : NiRefObject {
    re::Ptr32<NiFont> font;             // +0x8 (a reference)
    uint32_t flags;           // +0xC (1 its own colour, 2 centred)
    uint32_t bufferSize;      // +0x10
    uint32_t length;          // +0x14 (the terminator counted)
    re::Ptr32<uint16_t> wideText;       // +0x18
    re::Ptr32<char> text;               // +0x1C
    float color[4];           // +0x20
    uint8_t direction;        // +0x30 (0/1 across, 2/3 down)
    uint32_t lineCount;       // +0x34 (also how many line widths there's room for)
    re::Ptr32<float> lineWidths;        // +0x38
    float width;              // +0x3C
    float height;             // +0x40
    uint8_t dirty;            // +0x44
};
static_assert(sizeof(NiFontString) == 0x48);

// NiString2D (vtable 0x9A1F44, 0x70)
struct NiString2D : NiFontString {
    uint32_t x;                     // +0x48 (used as 16 bits)
    uint32_t y;                     // +0x4C
    uint32_t field50[4];            // +0x50
    uint8_t pad60[0x6C - 0x60];
    re::Ptr32<NiRefObject> screenTexture;     // +0x6C (a reference)
};
static_assert(sizeof(NiString2D) == 0x70);

} // namespace ni::font
