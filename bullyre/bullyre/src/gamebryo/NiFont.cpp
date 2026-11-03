// NiFont, NiFontString and NiString2D (src/gamebryo/NiFont.h), 0x8D6920..0x8D8570: loading an
// .nff font (the old header of versions 0..2 with one texture, the current one with a glyph
// table and several), the glyph map, measuring text, splitting a string into lines, and
// drawing a 2D string as a screen texture's rectangles. Float maths in float, as the game's
// 24-bit x87.

#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiFile.h"
#include "gamebryo/NiFont.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni::font {
namespace {

using ni::scene::AddViewerString;
using ni::scene::At;
using ni::scene::BinaryStream;
using ni::scene::GetViewerString;
using ni::scene::GetViewerStringString;
using ni::scene::GetViewerStringUint;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiFont_RTTI, 0x020EA918);
GAME_VAR(RTTI, NiFontString_RTTI, 0x020EAB58);
GAME_VAR(uint32_t, NiRefObject_ms_uiObjects, 0x020E2168);
GAME_VAR(uint32_t, NiFont_ms_uiCurrentVersion, 0x00BCBB30);
GAME_VAR(float, kDefaultColor, 0x00BC75E4);              // white: 4 floats
GAME_VAR(uint8_t, kFontPixelFormat, 0x00BC7750);         // NiPixelFormat (32-bit RGBA)
GAME_VAR(uint8_t, kFontTexturePrefs, 0x00BC7624);        // NiSourceTexture::FormatPrefs
GAME_VAR(uint16_t, gLineBuffer, 0x020EA950);             // one line of a string, while measuring

constexpr uintptr_t NiRefObject_vtable = 0x008FEFD4;
constexpr uintptr_t NiFont_vtable = 0x009A1F78;
constexpr uintptr_t NiFontString_vtable = 0x009A1FD8;
constexpr uintptr_t NiString2D_vtable = 0x009A1F44;
constexpr uintptr_t GlyphMapBase_vtable = 0x009A1F50;
constexpr uintptr_t GlyphMap_vtable = 0x009A1F64;
constexpr uintptr_t TextureArray_vtable = 0x009893C0;
constexpr uint32_t kNffMagic = 0x46464E;   // "NFF"

GAME_FN(void, __thiscall, GlyphMap_RemoveAll, 0x005AC520, GlyphMap* map);
GAME_FN(void, __cdecl, PointerArray_Delete, 0x00836F80, re::Ptr32<NiRefObject>* base);   // delete[] of NiPointers
GAME_FN(void, __thiscall, PointerArray_SetSize, 0x00856170, NiTArray<NiRefObject*>* array, uint32_t size);
GAME_FN(void, __thiscall, PointerArray_SetAt, 0x0040E800, NiTArray<NiRefObject*>* array, uint32_t index,
        re::Ptr32<NiRefObject>* value);
GAME_FN(void*, __cdecl, NiRealloc, 0x007523C0, void* p, uint32_t size);
GAME_FN(NiRefObject*, __thiscall, PixelData_Construct, 0x0076A610, void* self, uint32_t width, uint32_t height,
        const void* format, uint32_t mipmaps, uint32_t faces);
GAME_FN(NiRefObject*, __cdecl, SourceTexture_Create, 0x007699A0, NiRefObject* pixels, const void* prefs);
GAME_FN(NiRefObject*, __thiscall, ScreenTexture_Construct, 0x007DBAB0, void* self, NiRefObject* texture);
GAME_FN(void, __thiscall, ScreenTexture_RemoveAllScreenRects, 0x007DB6B0, NiRefObject* self);
GAME_FN(void, __thiscall, ScreenTexture_AddScreenRect, 0x007DBBB0, NiRefObject* self, uint32_t top, uint32_t left,
        uint32_t width, uint32_t height, uint32_t textureTop, uint32_t textureLeft, float r, float g, float b,
        float a);
GAME_FN(void, __cdecl, Stream_LoadString, 0x008885F0, BinaryStream* stream, char* buffer, uint32_t size);
GAME_FN(void, __cdecl, Stream_LoadUint, 0x00822840, BinaryStream* stream, void* value);
GAME_FN(void, __cdecl, Stream_LoadUshort, 0x008CD5B0, BinaryStream* stream, void* value);
GAME_FN(void, __thiscall, NiPath_Standardize, 0x00752C20, char* path);
GAME_FN(void, __thiscall, NiFile_Construct, 0x00752590, NiFile* self, const char* name, uint32_t mode,
        uint32_t bufferSize);
GAME_FN(void, __thiscall, NiFile_SetEndianSwap, 0x007534B0, NiFile* self, uint8_t swap);
GAME_FN(void, __thiscall, NiFile_Destruct, 0x00752720, NiFile* self);
// this file's, called where modes apply
GAME_FN(void, __thiscall, String2D_UpdateScreenTexture, 0x008D69E0, NiString2D* self);
GAME_FN(void, __thiscall, String2D_CreateScreenTexture, 0x008D6C50, NiString2D* self);
GAME_FN(uint16_t, __thiscall, Font_GetGlyphIndex, 0x008D6940, NiFont* self, uint16_t c);
GAME_FN(void, __cdecl, ConvertToWide, 0x008D6DC0, const char* text, uint32_t count, uint16_t* wide, uint32_t max);
GAME_FN(void, __thiscall, GlyphMap_SetAt, 0x008D6F50, GlyphMap* map, uint32_t key, uint32_t value);
GAME_FN(GlyphMap*, __thiscall, GlyphMap_Construct, 0x008D6E80, void* self, uint32_t hashSize);
GAME_FN(void, __cdecl, Stream_LoadUshorts, 0x008D6FC0, BinaryStream* stream, uint16_t* values, uint32_t count);
GAME_FN(void, __thiscall, Font_SizeOfChar, 0x008D6FF0, NiFont* self, uint16_t c, uint16_t* across, uint16_t* down,
        uint16_t* width, uint16_t* height, uint8_t direction);
GAME_FN(void, __thiscall, Font_GetTextExtentW, 0x008D7170, NiFont* self, const uint16_t* text, float* width,
        float* height, uint8_t direction);
GAME_FN(void, __thiscall, Font_GetCharUVPairW, 0x008D72C0, NiFont* self, uint16_t c, float* uv);
GAME_FN(uint8_t, __thiscall, Font_LoadOldHeader, 0x008D73C0, NiFont* self, BinaryStream* stream, uint32_t version);
GAME_FN(void, __thiscall, Font_GetCharUVPair, 0x008D7750, NiFont* self, char c, float* uv);
GAME_FN(uint8_t, __thiscall, Font_LoadPixelData, 0x008D7760, NiFont* self, BinaryStream* stream);
GAME_FN(void, __thiscall, Font_Destruct, 0x008D78F0, NiFont* self);
GAME_FN(NiFont*, __thiscall, Font_Construct, 0x008D7B90, void* self, NiRefObject* renderer);
GAME_FN(void, __thiscall, Font_AddTexture, 0x008D7C50, NiFont* self, NiRefObject* pixels);
GAME_FN(void, __thiscall, Font_LoadOldPixelData, 0x008D7CF0, NiFont* self, BinaryStream* stream);
GAME_FN(uint8_t, __thiscall, Font_LoadFont, 0x008D7E00, NiFont* self, BinaryStream* stream, uint32_t* version);
GAME_FN(uint8_t, __thiscall, Font_Load, 0x008D8040, NiFont* self, const char* name);
GAME_FN(void, __thiscall, FontString_ShutdownText, 0x008D81A0, NiFontString* self);
GAME_FN(void, __thiscall, FontString_SplitString, 0x008D81E0, NiFontString* self);
GAME_FN(void, __thiscall, FontString_Destruct, 0x008D8350, NiFontString* self);
GAME_FN(void, __thiscall, FontString_SetText, 0x008D83B0, NiFontString* self, const char* text, uint32_t flags);
GAME_FN(void, __thiscall, FontString_Construct, 0x008D84C0, NiFontString* self, NiFont* font, uint32_t flags,
        uint32_t bufferSize, const char* text, const float* color, uint8_t direction);

template <typename R = void, typename... A>
R Virtual(const void* object, int slot, A... args) {
    return reinterpret_cast<R(__thiscall*)(const void*, re::Arg<A>...)>(static_cast<void*>(VtableOf(object)[slot]))(object, args...);
}

// a NiPointer slot assigned as the library inlines it here: the old one let go first
template <typename T>
void SetReference(re::Ptr32<T>& slot, std::type_identity_t<T>* value) {
    if (slot != value) {
        Release(slot);
        slot = value;
        AddRef(value);
    }
}

// the binary stream's read, its position moved on
void Read(BinaryStream* stream, void* buffer, uint32_t bytes, uint32_t componentSize) {
    stream->position += stream->read(stream, buffer, bytes, &componentSize, 1);
}

void RefObjectConstruct(NiRefObject* self) {
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    self->refCount = 0;
    _InterlockedIncrement(reinterpret_cast<volatile long*>(&NiRefObject_ms_uiObjects));
}

void RefObjectDestruct(NiRefObject* self) {
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    _InterlockedDecrement(reinterpret_cast<volatile long*>(&NiRefObject_ms_uiObjects));
}

// a texture's size (NiTexture vf18 GetWidth, vf19 GetHeight)
uint32_t TextureWidth(NiRefObject* texture) {
    return Virtual<uint32_t>(texture, 18);
}
uint32_t TextureHeight(NiRefObject* texture) {
    return Virtual<uint32_t>(texture, 19);
}

// ---- NiFontString / NiString2D

// NiFontString::GetTextExtent
void __fastcall GetTextExtent(NiFontString* self, re::Edx, float* width, float* height) {
    *width = self->width;
    *height = self->height;
}

// NiFont::GetGlyphIndex: a character's glyph (none: 0)
uint16_t __fastcall GetGlyphIndex(NiFont* self, re::Edx, uint16_t c) {
    GlyphMap* map = self->glyphMap;
    for (GlyphMapItem* item = map->table[c % map->hashSize]; item != nullptr; item = item->next) {
        if (item->key == c) {
            return item->value;
        }
    }
    return 0;
}

void* __fastcall FontStringGetRTTI(NiFontString*, re::Edx) {
    return &NiFontString_RTTI;
}

NiString2D* __fastcall String2DDeletingDestructor(NiString2D* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiString2D_vtable);
    Release(self->screenTexture);
    FontString_Destruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(NiString2D));
    }
    return self;
}

// the screen texture's rectangles redone from the text when it changed: one per character,
// its glyph's part of the font's first texture, lines down by the font's line height,
// centred on the position (each line by its width) when asked
void __fastcall UpdateScreenTexture(NiString2D* self, re::Edx) {
    if (self->dirty == 0) {
        return;
    }
    ScreenTexture_RemoveAllScreenRects(self->screenTexture);
    const float* defaults = &kDefaultColor;
    float color[4] = {defaults[0], defaults[1], defaults[2], defaults[3]};
    if (self->flags & 1) {
        color[0] = self->color[0];
        color[1] = self->color[1];
        color[2] = self->color[2];
        color[3] = self->color[3];
    }
    uint32_t x;
    uint32_t y;
    if (self->flags & 2) {
        int32_t lineWidth = crt::FloatToInt(self->lineWidths[0]);
        x = uint16_t(uint16_t(self->x) - uint16_t(lineWidth / 2));
        y = uint16_t(uint16_t(self->y) - uint16_t((uint32_t(self->font->lineHeight) * self->lineCount) >> 1));
    } else {
        y = uint16_t(self->y);
        x = uint16_t(self->x);
    }
    uint32_t lineHeight = self->font->lineHeight;
    uint32_t line = 0;
    for (uint32_t i = 0; i < self->length; i++) {
        char c = self->text[i];
        if (c == 0) {
            break;
        }
        if (c == '\n') {
            line++;
            if (self->flags & 2) {
                int32_t lineWidth = crt::FloatToInt(self->lineWidths[line]);
                x = uint16_t(uint16_t(self->x) - uint16_t(lineWidth / 2));
            } else {
                x = uint16_t(self->x);
            }
            y += lineHeight;
            continue;
        }
        float uv[4];
        Font_GetCharUVPair(self->font, c, uv);
        NiRefObject* texture = self->font->textures.base[0];
        uint32_t width = uint16_t(crt::FloatToInt(float(TextureWidth(texture)) * (uv[2] - uv[0])));
        uint32_t textureLeft = uint16_t(int32_t(float(TextureWidth(texture)) * uv[0]));
        uint32_t textureTop = uint16_t(int32_t(float(TextureHeight(texture)) * uv[1]));
        ScreenTexture_AddScreenRect(self->screenTexture, y, x, width, lineHeight, textureTop, textureLeft, color[0],
                                    color[1], color[2], color[3]);
        x += width;
    }
    At<uint16_t>(self->screenTexture, 0x18) |= 0xF;
    self->dirty = 0;
}

// the screen texture made over the font's first texture (none for an empty string) and drawn
void __fastcall CreateScreenTexture(NiString2D* self, re::Edx) {
    if (self->length == 0) {
        if (self->screenTexture != nullptr) {
            Release(self->screenTexture);
            self->screenTexture = nullptr;
        }
        return;
    }
    void* p = NiNew(0x20);
    NiRefObject* screen = p != nullptr ? ScreenTexture_Construct(p, self->font->textures.base[0]) : nullptr;
    SetReference(self->screenTexture, screen);
    if (self->screenTexture != nullptr) {
        uint8_t* property = At<uint8_t*>(self->screenTexture, 0x14);
        At<uint16_t>(property, 0x18) = uint16_t((At<uint16_t>(property, 0x18) & 0xFFF5) | 4);
        String2D_UpdateScreenTexture(self);
    }
}

// the constructor: a font string with the given position (dirty when it differs from what the
// memory held), then its screen texture
NiString2D* __fastcall String2DConstruct(NiString2D* self, re::Edx, NiFont* font, uint32_t flags, uint32_t bufferSize,
                                         const char* text, const float* color, uint32_t x, uint32_t y) {
    FontString_Construct(self, font, flags, bufferSize, text, color, 0);
    self->vtable = reinterpret_cast<void*>(NiString2D_vtable);
    self->field50[0] = 0;
    self->field50[1] = 0;
    self->field50[2] = 0;
    self->field50[3] = 0;
    self->screenTexture = nullptr;
    if (self->x != x || self->y != y) {
        self->x = x;
        self->y = y;
        self->dirty = 1;
    }
    String2D_CreateScreenTexture(self);
    return self;
}

// Draw: the screen texture brought up to date and handed to the renderer (vf99)
void __fastcall String2DDraw(NiString2D* self, re::Edx, void* renderer) {
    if (self->screenTexture == nullptr) {
        String2D_CreateScreenTexture(self);
    } else if (self->dirty != 0) {
        String2D_UpdateScreenTexture(self);
    }
    Virtual(renderer, 99, self->screenTexture);
}

// a string's characters widened (sign extended), at most `count` and `max` of them
void __cdecl ConvertToWideImpl(const char* text, uint32_t count, uint16_t* wide, uint32_t max) {
    char c = *text;
    uint32_t i = 0;
    if (c == 0) {
        *wide = 0;
        return;
    }
    do {
        if (count == 0 || max <= i) {
            break;
        }
        text++;
        *wide = uint16_t(int16_t(c));
        c = *text;
        wide++;
        i++;
        count--;
    } while (c != 0);
    *wide = 0;
}

// ---- the glyph map (NiTMap<unsigned short, unsigned short>)

// vf1 SetValue (the map itself unused)
void __fastcall GlyphMapSetValue(GlyphMap*, re::Edx, GlyphMapItem* item, uint16_t key, uint16_t value) {
    item->key = key;
    item->value = value;
}

GlyphMap* __fastcall GlyphMapBaseDeletingDestructor(GlyphMap* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(GlyphMapBase_vtable);
    GlyphMap_RemoveAll(self);
    NiFree(self->table);
    if (flags & 1) {
        NiDelete(self, sizeof(GlyphMap));
    }
    return self;
}

GlyphMap* __fastcall GlyphMapConstruct(GlyphMap* self, re::Edx, uint32_t hashSize) {
    self->hashSize = hashSize;
    self->vtable = reinterpret_cast<void*>(GlyphMapBase_vtable);
    self->count = 0;
    self->table = static_cast<re::Ptr32<GlyphMapItem>*>(NiMalloc(hashSize * 4));
    std::memset(self->table, 0, self->hashSize * 4);
    self->vtable = reinterpret_cast<void*>(GlyphMap_vtable);
    return self;
}

// vf3 NewItem (written without a check)
GlyphMapItem* __fastcall GlyphMapNewItem(GlyphMap*, re::Edx) {
    auto* item = static_cast<GlyphMapItem*>(NiNew(sizeof(GlyphMapItem)));
    item->field8 = 0;
    return item;
}

// vf4 DeleteItem
void __fastcall GlyphMapDeleteItem(GlyphMap*, re::Edx, GlyphMapItem* item) {
    item->value = 0;
    NiDelete(item, sizeof(GlyphMapItem));
}

GlyphMap* __fastcall GlyphMapDeletingDestructor(GlyphMap* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(GlyphMap_vtable);
    GlyphMap_RemoveAll(self);
    self->vtable = reinterpret_cast<void*>(GlyphMapBase_vtable);
    GlyphMap_RemoveAll(self);
    NiFree(self->table);
    if (flags & 1) {
        NiDelete(self, sizeof(GlyphMap));
    }
    return self;
}

// SetAt: the key's value replaced, or a new item at the head of its bucket
void __fastcall GlyphMapSetAt(GlyphMap* self, re::Edx, uint32_t key, uint32_t value) {
    uint32_t index = (key & 0xFFFF) % self->hashSize;
    for (GlyphMapItem* item = self->table[index]; item != nullptr; item = item->next) {
        if (item->key == uint16_t(key)) {
            item->value = uint16_t(value);
            return;
        }
    }
    auto* item = Virtual<GlyphMapItem*>(self, 3);
    Virtual(self, 1, item, key, value);
    item->next = self->table[index];
    self->table[index] = item;
    self->count++;
}

// NiStreamLoadBinary of `count` unsigned shorts (components of 2 bytes)
void __cdecl LoadUshorts(BinaryStream* stream, uint16_t* values, uint32_t count) {
    Read(stream, values, count * 2, 2);
}

// ---- NiFont: measuring

// a character's part in a string's extent: across, a glyph's width (a new line: back to 0, a
// line height down); down, a line height each (a new line: a column across)
void __fastcall SizeOfChar(NiFont* self, re::Edx, uint16_t c, uint16_t* across, uint16_t* down, uint16_t* width,
                           uint16_t* height, uint8_t direction) {
    switch (direction) {
    case 0:
    case 1:
        if (c == '\n') {
            *across = 0;
            *height = uint16_t(*height + self->charHeight);
            return;
        }
        {
            uint16_t glyph = Font_GetGlyphIndex(self, c);
            *across = uint16_t(*across + (self->glyphRects[glyph][2] - self->glyphRects[glyph][0]));
        }
        if (*across > *width) {
            *width = *across;
        }
        return;
    case 2:
    case 3:
        if (c == '\n') {
            *down = 0;
            *width = uint16_t(*width + self->charHeight);
            return;
        }
        *down = uint16_t(*down + self->charHeight);
        if (*down > *height) {
            *height = *down;
        }
        return;
    default:
        return;
    }
}

// a string's extent: its widest line by its lines (across), or the reverse (down)
template <typename C>
void TextExtent(NiFont* self, const C* text, float* width, float* height, uint8_t direction) {
    if (text == nullptr) {
        return;
    }
    uint16_t across = 0;
    uint16_t down = 0;
    uint16_t w = 0;
    uint16_t h = 0;
    switch (direction) {
    case 0:
    case 1:
        h = uint16_t(self->charHeight);
        break;
    case 2:
    case 3:
        w = uint16_t(self->charHeight);
        break;
    }
    for (; *text != 0; text++) {
        Font_SizeOfChar(self, uint16_t(int16_t(*text)), &across, &down, &w, &h, direction);
    }
    *width = float(int32_t(w));
    *height = float(int32_t(h));
}

void __fastcall GetTextExtentA(NiFont* self, re::Edx, const char* text, float* width, float* height,
                               uint8_t direction) {
    TextExtent(self, text, width, height, direction);
}

void __fastcall GetTextExtentW(NiFont* self, re::Edx, const uint16_t* text, float* width, float* height,
                               uint8_t direction) {
    TextExtent(self, text, width, height, direction);
}

// a character's glyph size in pixels (no glyph: glyph 0's)
void __fastcall GetCharExtent(NiFont* self, re::Edx, char c, float* width, float* height) {
    uint16_t key = uint16_t(int16_t(c));
    uint32_t glyph = 0;
    GlyphMap* map = self->glyphMap;
    for (GlyphMapItem* item = map->table[key % map->hashSize]; item != nullptr; item = item->next) {
        if (item->key == key) {
            glyph = item->value;
            break;
        }
    }
    const uint16_t* rect = self->glyphRects[glyph];
    *height = float(int32_t(rect[3]) - int32_t(rect[1]));
    *width = float(int32_t(rect[2]) - int32_t(rect[0]));
}

// a character's glyph as texture coordinates in its texture (left, top, right, bottom)
void __fastcall GetCharUVPairW(NiFont* self, re::Edx, uint16_t c, float* uv) {
    if (uv == nullptr) {
        return;
    }
    uint16_t glyph = Font_GetGlyphIndex(self, c);
    uint8_t textureIndex = self->glyphTextures != nullptr ? self->glyphTextures[glyph] : 0;
    NiRefObject* texture = self->textures.base[textureIndex];
    int32_t width = int32_t(TextureWidth(texture));
    int32_t height = int32_t(TextureHeight(self->textures.base[textureIndex]));
    float w = float(width);
    float h = float(height);
    const uint16_t* rect = self->glyphRects[glyph];
    uv[0] = float(rect[0]) / w;
    uv[1] = float(rect[1]) / h;
    uv[2] = float(rect[2]) / w;
    uv[3] = float(rect[3]) / h;
}

void __fastcall GetCharUVPair(NiFont* self, re::Edx, char c, float* uv) {
    Font_GetCharUVPairW(self, uint16_t(int16_t(c)), uv);
}

// ---- NiFont: loading

// the header of versions 0..2: (a name before version 1, read and dropped,) the sizes, the
// characters (one byte each) in glyph order, the texture's size, each glyph's corners as
// fractions of it (to pixels, truncated), (before version 2 four floats dropped)
uint8_t __fastcall LoadOldHeader(NiFont* self, re::Edx, BinaryStream* stream, uint32_t version) {
    if (version < 1) {
        char name[0x80];
        Read(stream, name, sizeof(name), 1);
    }
    Read(stream, &self->height, 4, 4);
    Read(stream, &self->charHeight, 4, 4);
    Read(stream, &self->flags, 4, 4);
    Read(stream, &self->glyphCount, 4, 4);
    auto* characters = static_cast<char*>(NiMalloc(self->glyphCount));
    Read(stream, characters, self->glyphCount, 1);
    void* p = NiNew(sizeof(GlyphMap));
    GlyphMap* map = nullptr;
    if (p != nullptr) {
        map = static_cast<GlyphMap*>(p);
        map->vtable = reinterpret_cast<void*>(GlyphMapBase_vtable);
        map->hashSize = 37;
        map->count = 0;
        map->table = static_cast<re::Ptr32<GlyphMapItem>*>(NiMalloc(37 * 4));
        std::memset(map->table, 0, map->hashSize * 4);
        map->vtable = reinterpret_cast<void*>(GlyphMap_vtable);
    }
    self->glyphMap = map;
    for (uint32_t i = 0; i < self->glyphCount; i++) {
        GlyphMap_SetAt(self->glyphMap, uint16_t(int16_t(characters[i])), i);
    }
    NiFree(characters);
    uint32_t textureWidth;
    uint32_t textureHeight;
    uint32_t unused;
    Read(stream, &textureWidth, 4, 4);
    Read(stream, &textureHeight, 4, 4);
    Read(stream, &unused, 4, 4);
    auto* corners = static_cast<float*>(NiMalloc(self->glyphCount * 16));
    Read(stream, corners, self->glyphCount * 16, 4);
    self->glyphRects = static_cast<uint16_t(*)[4]>(NiMalloc(self->glyphCount * 8));
    uint16_t* rects = self->glyphRects[0];
    for (uint32_t i = 0; i < self->glyphCount * 4; i++) {
        float size = float((i & 1) == 0 ? textureWidth : textureHeight);
        rects[i] = uint16_t(int32_t(size * corners[i]));
    }
    NiFree(corners);
    if (version < 2) {
        float dropped[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        Read(stream, &dropped[0], 4, 4);
        Read(stream, &dropped[1], 4, 4);
        Read(stream, &dropped[2], 4, 4);
        Read(stream, &dropped[3], 4, 4);
    }
    self->field96 = uint16_t(self->charHeight);
    self->lineHeight = uint16_t(self->charHeight);
    return 1;
}

// the current format's textures: each one's size and bits per pixel, its pixels, made a
// source texture and kept
uint8_t __fastcall LoadPixelData(NiFont* self, re::Edx, BinaryStream* stream) {
    uint32_t bitsPerPixel = 0x20;
    for (uint32_t i = 0; i < self->textureCount; i++) {
        uint32_t width;
        uint32_t height;
        Read(stream, &width, 4, 4);
        Read(stream, &height, 4, 4);
        Read(stream, &bitsPerPixel, 4, 4);
        void* p = NiNew(0x70);
        NiRefObject* pixels = nullptr;
        if (p != nullptr) {
            pixels = PixelData_Construct(p, width, height, &kFontPixelFormat, 1, 1);
            AddRef(pixels);
        }
        uint8_t* data = *At<re::Ptr32<uint8_t>*>(pixels, 0x5C) + At<uint32_t>(pixels, 0x50);
        Read(stream, data, (height * width * bitsPerPixel) >> 3, 1);
        re::Ptr32<NiRefObject> texture = SourceTexture_Create(pixels, &kFontTexturePrefs);
        AddRef(texture);
        PointerArray_SetAt(&self->textures, i, &texture);
        Release(texture);
        Release(pixels);
    }
    return 1;
}

// the destructor: the glyph tables, the map, the textures and the renderer let go
void __fastcall Destruct(NiFont* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiFont_vtable);
    NiFree(self->glyphTextures);
    NiFree(self->glyphRects);
    if (self->glyphMap != nullptr) {
        Virtual(self->glyphMap, 0, 1u);
    }
    self->textures.vtable = reinterpret_cast<void*>(TextureArray_vtable);
    PointerArray_Delete(self->textures.base);
    Release(self->renderer);
    RefObjectDestruct(self);
}

void* __fastcall GetRTTI(NiFont*, re::Edx) {
    return &NiFont_RTTI;
}

void __fastcall GetViewerStrings(NiFont* self, re::Edx, NiTArray<char*>* strings) {
    AddViewerString(strings, GetViewerString(NiFont_RTTI.name));
    AddViewerString(strings, GetViewerStringString("m_acName", self->name));
    AddViewerString(strings, GetViewerStringUint("m_uiHeight", self->height));
    AddViewerString(strings, GetViewerStringUint("m_uiCharHeight", self->charHeight));
    AddViewerString(strings, GetViewerStringUint("m_uiFlags", self->flags));
    AddViewerString(strings, GetViewerStringUint("m_uiGlyphCount", self->glyphCount));
    AddViewerString(strings, GetViewerStringUint("Num Textures", self->textures.esize));
}

NiFont* __fastcall DeletingDestructor(NiFont* self, re::Edx, uint32_t flags) {
    Font_Destruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(NiFont));
    }
    return self;
}

// the constructor: the renderer held, nothing loaded
NiFont* __fastcall Construct(NiFont* self, re::Edx, NiRefObject* renderer) {
    RefObjectConstruct(self);
    self->vtable = reinterpret_cast<void*>(NiFont_vtable);
    self->renderer = renderer;
    AddRef(renderer);
    self->textures.maxSize = 0;
    self->textures.size = 0;
    self->textures.esize = 0;
    self->textures.base = nullptr;
    self->textures.growBy = 1;
    self->textures.vtable = reinterpret_cast<void*>(TextureArray_vtable);
    self->height = 0;
    self->charHeight = 0;
    self->lineHeight = 0;
    self->field96 = 0;
    self->flags = 0;
    self->textureCount = 0;
    self->glyphCount = 0;
    self->glyphMap = nullptr;
    self->name[0] = 0;
    self->glyphTextures = nullptr;
    self->glyphRects = nullptr;
    self->lastError = 0;
    return self;
}

// a texture made from pixel data and appended (the pixel data a reference passed in, let go)
void __fastcall AddTexture(NiFont* self, re::Edx, NiRefObject* pixels) {
    re::Ptr32<NiRefObject> texture = SourceTexture_Create(pixels, &kFontTexturePrefs);
    AddRef(texture);
    uint32_t index = self->textures.size;
    if (self->textures.maxSize <= index) {
        PointerArray_SetSize(&self->textures, self->textures.growBy + index);
    }
    PointerArray_SetAt(&self->textures, index, &texture);
    Release(texture);
    Release(pixels);
}

// the old format's one texture: its size, bits per pixel and pixels
void __fastcall LoadOldPixelData(NiFont* self, re::Edx, BinaryStream* stream) {
    uint32_t width;
    uint32_t height;
    uint32_t bitsPerPixel;
    Read(stream, &width, 4, 4);
    Read(stream, &height, 4, 4);
    Read(stream, &bitsPerPixel, 4, 4);
    void* p = NiNew(0x70);
    NiRefObject* pixels = nullptr;
    if (p != nullptr) {
        pixels = PixelData_Construct(p, width, height, &kFontPixelFormat, 1, 1);
        AddRef(pixels);
    }
    uint8_t* data = *At<re::Ptr32<uint8_t>*>(pixels, 0x5C) + At<uint32_t>(pixels, 0x50);
    Read(stream, data, (bitsPerPixel * height * width) >> 3, 1);
    _InterlockedIncrement(reinterpret_cast<volatile long*>(&pixels->refCount));   // (the reference passed on)
    Font_AddTexture(self, pixels);
    Release(pixels);
}

// an .nff stream: its magic and version (newer than this library's: refused), then the old
// header and texture, or the current header (name, sizes, the characters as unsigned shorts
// in glyph order, each glyph's texture and pixel corners, how many textures)
uint8_t __fastcall LoadFont(NiFont* self, re::Edx, BinaryStream* stream, uint32_t* version) {
    uint32_t magic;
    Read(stream, &magic, 4, 4);
    if (magic != kNffMagic) {
        self->lastError = 0x80000001;
        return 0;
    }
    Read(stream, version, 4, 4);
    if (*version > NiFont_ms_uiCurrentVersion) {
        self->lastError = 0x80000002;
        return 0;
    }
    if (*version <= 2) {
        if (Font_LoadOldHeader(self, stream, *version) == 0) {
            return 0;
        }
        Font_LoadOldPixelData(self, stream);
        return 1;
    }
    uint32_t nameLength;
    Read(stream, &nameLength, 4, 4);
    Stream_LoadString(stream, self->name, nameLength + 1);
    Stream_LoadUint(stream, &self->height);
    Stream_LoadUint(stream, &self->charHeight);
    Stream_LoadUshort(stream, &self->field96);
    Stream_LoadUshort(stream, &self->lineHeight);
    Stream_LoadUint(stream, &self->flags);
    Stream_LoadUint(stream, &self->glyphCount);
    auto* characters = static_cast<uint16_t*>(NiMalloc(self->glyphCount * 2));
    Stream_LoadUshorts(stream, characters, self->glyphCount);
    void* p = NiNew(sizeof(GlyphMap));
    self->glyphMap = p != nullptr ? GlyphMap_Construct(p, 37) : nullptr;
    for (uint32_t i = 0; i < self->glyphCount; i++) {
        GlyphMap_SetAt(self->glyphMap, characters[i], i);
    }
    NiFree(characters);
    self->glyphTextures = static_cast<uint8_t*>(NiMalloc(self->glyphCount));
    Read(stream, self->glyphTextures, self->glyphCount, 1);
    self->glyphRects = static_cast<uint16_t(*)[4]>(NiMalloc(self->glyphCount * 8));
    Read(stream, self->glyphRects, self->glyphCount * 8, 2);
    Read(stream, &self->textureCount, 4, 4);
    PointerArray_SetSize(&self->textures, self->textureCount);
    return 1;
}

// a font file read (its path standardised; a file that won't open: error 0x80000001); the
// current format's textures after its header
uint8_t __fastcall Load(NiFont* self, re::Edx, const char* name) {
    char path[0x104];
    crt::StrcpyS(path, sizeof(path), name);
    NiPath_Standardize(path);
    alignas(4) uint8_t storage[sizeof(NiFile)];
    auto* file = reinterpret_cast<NiFile*>(storage);
    NiFile_Construct(file, path, 0, 0x8000);
    NiFile_SetEndianSwap(file, 0);
    if (Virtual<uint8_t>(file, 1) == 0) {
        self->lastError = 0x80000001;
        NiFile_Destruct(file);
        return 0;
    }
    uint32_t version;
    if (Font_LoadFont(self, reinterpret_cast<BinaryStream*>(file), &version) == 0) {
        NiFile_Destruct(file);
        return 0;
    }
    if (version >= 3) {
        Font_LoadPixelData(self, reinterpret_cast<BinaryStream*>(file));
    }
    NiFile_Destruct(file);
    return 1;
}

// NiFont::Create: a font for a renderer from a file (deleted again when it doesn't load)
NiFont* __cdecl Create(NiRefObject* renderer, const char* name) {
    void* p = NiNew(sizeof(NiFont));
    if (p == nullptr) {
        return nullptr;
    }
    NiFont* font = Font_Construct(p, renderer);
    if (font != nullptr && Font_Load(font, name) == 0) {
        Virtual(font, 0, 1u);
        return nullptr;
    }
    return font;
}

// ---- NiFontString

// the text's buffers freed
void __fastcall ShutdownText(NiFontString* self, re::Edx) {
    NiFree(self->text);
    NiFree(self->wideText);
    self->text = nullptr;
    self->wideText = nullptr;
    self->length = 0;
    NiFree(self->lineWidths);
    self->lineWidths = nullptr;
}

// the string's lines measured: one line, the whole extent; more, each line's width (the
// widest the string's), the heights added up line by line (carriage returns made spaces)
void __fastcall SplitString(NiFontString* self, re::Edx) {
    if (self->text == nullptr) {
        return;
    }
    uint32_t lines = 1;
    for (uint32_t i = 0; self->wideText[i] != 0; i++) {
        if (self->wideText[i] == '\n') {
            lines++;
        }
    }
    if (lines > self->lineCount || self->lineWidths == nullptr) {
        self->lineWidths = static_cast<float*>(NiRealloc(self->lineWidths, lines * 4));
    }
    self->lineCount = lines;
    float lineHeight = 0.0f;
    if (lines == 1) {
        Font_GetTextExtentW(self->font, self->wideText, &self->width, &self->height, self->direction);
        self->lineWidths[0] = self->width;
        return;
    }
    float lineWidth = 0.0f;
    self->width = 0.0f;
    self->height = 0.0f;
    uint16_t* buffer = &gLineBuffer;
    uint32_t n = 0;
    uint32_t line = 0;
    for (uint32_t i = 0; i < self->length; i++) {
        if (self->wideText[i] == '\r') {
            self->wideText[i] = ' ';
        }
        uint16_t c = self->wideText[i];
        if (c != '\n' && c != 0) {
            buffer[n++] = c;
            continue;
        }
        buffer[n] = 0;
        Font_GetTextExtentW(self->font, buffer, &lineWidth, &lineHeight, self->direction);
        self->lineWidths[line++] = lineWidth;
        if (lineWidth > self->width) {
            self->width = lineWidth;
        }
        self->height = float(self->font->charHeight) + self->height;
        n = 0;
    }
}

void __fastcall FontStringDestruct(NiFontString* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiFontString_vtable);
    NiFree(self->lineWidths);
    NiFree(self->text);
    NiFree(self->wideText);
    Release(self->font);
    RefObjectDestruct(self);
}

// SetText (flags -1: kept): the same text, nothing; one that fits the buffer copied in; a
// longer one, the buffers made again (at least the new length); then the lines measured. (A
// null text with one already set is compared all the same, as the game does.)
void __fastcall SetText(NiFontString* self, re::Edx, const char* text, uint32_t flags) {
    if (flags != 0xFFFFFFFF) {
        self->flags = flags;
    }
    uint32_t length = self->length;
    if (text != nullptr) {
        length = uint32_t(std::strlen(text)) + 1;
    }
    if (self->text != nullptr) {
        if (crt::Strcmp(self->text, text) == 0) {
            return;
        }
        if (self->bufferSize > length) {
            crt::StrcpyS(self->text, self->bufferSize, text);
            ConvertToWide(self->text, self->bufferSize, self->wideText, self->bufferSize);
            self->length = length;
            self->dirty = 1;
        } else {
            FontString_ShutdownText(self);
            self->bufferSize = length + 1;
        }
    }
    if (self->text == nullptr) {
        self->length = length;
        if (!(int32_t(self->bufferSize) > int32_t(length))) {
            self->bufferSize = length;
        }
        self->text = static_cast<char*>(NiMalloc(self->bufferSize));
        self->wideText = static_cast<uint16_t*>(NiMalloc(self->bufferSize * 2));
        if (text != nullptr) {
            crt::StrcpyS(self->text, self->bufferSize, text);
            ConvertToWide(self->text, self->bufferSize, self->wideText, self->bufferSize);
        }
        self->dirty = 1;
    }
    FontString_SplitString(self);
}

// the constructor: the font held, the colour and direction kept, the text set (the colour
// copied again after)
NiFontString* __fastcall FontStringConstruct(NiFontString* self, re::Edx, NiFont* font, uint32_t flags,
                                             uint32_t bufferSize, const char* text, const float* color,
                                             uint8_t direction) {
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    self->refCount = 0;
    _InterlockedIncrement(reinterpret_cast<volatile long*>(&NiRefObject_ms_uiObjects));
    self->vtable = reinterpret_cast<void*>(NiFontString_vtable);
    self->font = font;
    AddRef(font);
    self->bufferSize = bufferSize;
    self->color[0] = color[0];
    self->color[1] = color[1];
    self->color[2] = color[2];
    self->width = 0.0f;
    self->height = 0.0f;
    self->color[3] = color[3];
    self->direction = direction;
    self->lineCount = 0;
    self->lineWidths = nullptr;
    self->flags = 0;
    self->text = nullptr;
    self->wideText = nullptr;
    self->length = 0;
    FontString_SetText(self, text, flags);
    self->color[0] = color[0];
    self->color[1] = color[1];
    self->color[2] = color[2];
    self->color[3] = color[3];
    return self;
}

NiFontString* __fastcall FontStringDeletingDestructor(NiFontString* self, re::Edx, uint32_t flags) {
    FontString_Destruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(NiFontString));
    }
    return self;
}

} // namespace
} // namespace ni::font

// ---- registration ---------------------------------------------------------------------

#define NIFONT_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIFONT_REVERSED("NiFontString::GetTextExtent", 0x008D6920, &ni::font::GetTextExtent);
NIFONT_REVERSED("NiFont::GetGlyphIndex", 0x008D6940, &ni::font::GetGlyphIndex);
NIFONT_REVERSED("NiFontString::GetRTTI", 0x008D6980, &ni::font::FontStringGetRTTI);
NIFONT_REVERSED("NiString2D::DeletingDestructor", 0x008D6990, &ni::font::String2DDeletingDestructor);
NIFONT_REVERSED("NiString2D::UpdateScreenTexture", 0x008D69E0, &ni::font::UpdateScreenTexture);
NIFONT_REVERSED("NiString2D::CreateScreenTexture", 0x008D6C50, &ni::font::CreateScreenTexture);
NIFONT_REVERSED("NiString2D::NiString2D", 0x008D6D10, &ni::font::String2DConstruct);
NIFONT_REVERSED("NiString2D::Draw", 0x008D6D80, &ni::font::String2DDraw);
NIFONT_REVERSED("NiFontString::ConvertToWide", 0x008D6DC0, &ni::font::ConvertToWideImpl);
NIFONT_REVERSED("NiTMap<unsigned short,unsigned short>::SetValue", 0x008D6E20, &ni::font::GlyphMapSetValue);
NIFONT_REVERSED("NiTMapBase<unsigned short,unsigned short>::DeletingDestructor", 0x008D6E40,
                &ni::font::GlyphMapBaseDeletingDestructor);
NIFONT_REVERSED("NiTMap<unsigned short,unsigned short>::NiTMap", 0x008D6E80, &ni::font::GlyphMapConstruct);
NIFONT_REVERSED("NiTMap<unsigned short,unsigned short>::NewItem", 0x008D6ED0, &ni::font::GlyphMapNewItem);
NIFONT_REVERSED("NiTMap<unsigned short,unsigned short>::DeleteItem", 0x008D6EF0, &ni::font::GlyphMapDeleteItem);
NIFONT_REVERSED("NiTMap<unsigned short,unsigned short>::DeletingDestructor", 0x008D6F10,
                &ni::font::GlyphMapDeletingDestructor);
NIFONT_REVERSED("NiTMap<unsigned short,unsigned short>::SetAt", 0x008D6F50, &ni::font::GlyphMapSetAt);
NIFONT_REVERSED("NiFont::LoadUshorts", 0x008D6FC0, &ni::font::LoadUshorts);
NIFONT_REVERSED("NiFont::SizeOfChar", 0x008D6FF0, &ni::font::SizeOfChar);
NIFONT_REVERSED("NiFont::GetTextExtent", 0x008D70B0, &ni::font::GetTextExtentA);
NIFONT_REVERSED("NiFont::GetTextExtentW", 0x008D7170, &ni::font::GetTextExtentW);
NIFONT_REVERSED("NiFont::GetCharExtent", 0x008D7240, &ni::font::GetCharExtent);
NIFONT_REVERSED("NiFont::GetCharUVPairW", 0x008D72C0, &ni::font::GetCharUVPairW);
NIFONT_REVERSED("NiFont::LoadOldHeader", 0x008D73C0, &ni::font::LoadOldHeader);
NIFONT_REVERSED("NiFont::GetCharUVPair", 0x008D7750, &ni::font::GetCharUVPair);
NIFONT_REVERSED("NiFont::LoadPixelData", 0x008D7760, &ni::font::LoadPixelData);
NIFONT_REVERSED("NiFont::~NiFont", 0x008D78F0, &ni::font::Destruct);
NIFONT_REVERSED("NiFont::GetRTTI", 0x008D7980, &ni::font::GetRTTI);
NIFONT_REVERSED("NiFont::GetViewerStrings", 0x008D7990, &ni::font::GetViewerStrings);
NIFONT_REVERSED("NiFont::DeletingDestructor", 0x008D7B60, &ni::font::DeletingDestructor);
NIFONT_REVERSED("NiFont::NiFont", 0x008D7B90, &ni::font::Construct);
NIFONT_REVERSED("NiFont::AddTexture", 0x008D7C50, &ni::font::AddTexture);
NIFONT_REVERSED("NiFont::LoadOldPixelData", 0x008D7CF0, &ni::font::LoadOldPixelData);
NIFONT_REVERSED("NiFont::LoadFont", 0x008D7E00, &ni::font::LoadFont);
NIFONT_REVERSED("NiFont::Load", 0x008D8040, &ni::font::Load);
NIFONT_REVERSED("NiFont::Create", 0x008D8150, &ni::font::Create);
NIFONT_REVERSED("NiFontString::ShutdownText", 0x008D81A0, &ni::font::ShutdownText);
NIFONT_REVERSED("NiFontString::SplitString", 0x008D81E0, &ni::font::SplitString);
NIFONT_REVERSED("NiFontString::~NiFontString", 0x008D8350, &ni::font::FontStringDestruct);
NIFONT_REVERSED("NiFontString::SetText", 0x008D83B0, &ni::font::SetText);
NIFONT_REVERSED("NiFontString::NiFontString", 0x008D84C0, &ni::font::FontStringConstruct);
NIFONT_REVERSED("NiFontString::DeletingDestructor", 0x008D8570, &ni::font::FontStringDeletingDestructor);
