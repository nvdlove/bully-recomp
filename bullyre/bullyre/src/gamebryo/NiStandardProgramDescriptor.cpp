// NiStandardPixelProgramDescriptor and NiStandardVertexProgramDescriptor (NiMaterialDescriptor's
// layout: an identifier and four words of bit fields, +0x4..+0x10): NiStandardMaterial's program
// keys. The pixel one's UV set per texture map (4 bits each), its texture count and the UV sets
// it takes; the vertex one's output UVs (a 3-bit set and a texture-output bit each); and their
// descriptions, made by Gamebryo's bit-field macros: one function per field adds "NAME = value"
// to an NiString (a newline first unless empty), after the fields before it in its word when
// asked (each word's chain starts at its first field).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

struct NiString {
    re::Ptr32<char> handle;
};

GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(uint32_t, __thiscall, NiString_Length, 0x00773D10, const NiString* self);
GAME_FN(void, __thiscall, NiString_Concatenate, 0x00774870, NiString* self, const char* text);
GAME_FN(void, __thiscall, NiString_ConstructSized, 0x007740D0, NiString* self, uint32_t size);
// (this file's, called where modes apply)
GAME_FN(uint32_t, __thiscall, PixelDesc_GetStandardTextureCount, 0x007E37F0, const uint8_t* self);
GAME_FN(uint32_t, __thiscall, PixelDesc_GetUVSetForMap, 0x007E35C0, const uint8_t* self, uint32_t map);

// ---- the fields' descriptions: the function's address, the one before it in its word (0: the
// first), the word's offset, the shift, the mask, the name

struct ProgramField {   // (our table of the descriptor's fields, not a game layout)
    uint32_t address;
    uint32_t prev;
    uint32_t offset;
    uint32_t shift;
    uint32_t mask;
    const char* name;   // (our table, not the game's: a plain pointer)
};

constexpr ProgramField kFields[] = {
    {0x007E38C0, 0x00000000, 0x4, 0, 0x1, "APPLYMODE"},
    {0x007E3950, 0x007E38C0, 0x4, 1, 0x1, "WORLDPOSITION"},
    {0x007E3A00, 0x007E3950, 0x4, 2, 0x1, "WORLDNORMAL"},
    {0x007E3AB0, 0x007E3A00, 0x4, 3, 0x1, "WORLDNBT"},
    {0x007E3B60, 0x007E3AB0, 0x4, 4, 0x1, "WORLDVIEW"},
    {0x007E3C10, 0x007E3B60, 0x4, 5, 0x3, "NORMALMAPTYPE"},
    {0x007E3CC0, 0x007E3C10, 0x4, 7, 0x1, "PARALLAXMAPCOUNT"},
    {0x007E3D70, 0x007E3CC0, 0x4, 8, 0x1, "BASEMAPCOUNT"},
    {0x007E3E20, 0x007E3D70, 0x4, 9, 0x1, "NORMALMAPCOUNT"},
    {0x007E3ED0, 0x007E3E20, 0x4, 10, 0x1, "DARKMAPCOUNT"},
    {0x007E3F80, 0x007E3ED0, 0x4, 11, 0x1, "DETAILMAPCOUNT"},
    {0x007E4030, 0x007E3F80, 0x4, 12, 0x1, "BUMPMAPCOUNT"},
    {0x007E40E0, 0x007E4030, 0x4, 13, 0x1, "GLOSSMAPCOUNT"},
    {0x007E4190, 0x007E40E0, 0x4, 14, 0x1, "GLOWMAPCOUNT"},
    {0x007E4240, 0x007E4190, 0x4, 15, 0x1, "CUSTOMMAP00COUNT"},
    {0x007E42F0, 0x007E4240, 0x4, 16, 0x1, "CUSTOMMAP01COUNT"},
    {0x007E43A0, 0x007E42F0, 0x4, 17, 0x1, "CUSTOMMAP02COUNT"},
    {0x007E4450, 0x007E43A0, 0x4, 18, 0x1, "CUSTOMMAP03COUNT"},
    {0x007E4500, 0x007E4450, 0x4, 19, 0x1, "CUSTOMMAP04COUNT"},
    {0x007E45B0, 0x007E4500, 0x4, 20, 0x7, "DECALMAPCOUNT"},
    {0x007E4660, 0x007E45B0, 0x4, 23, 0x1, "FOGENABLED"},
    {0x007E4710, 0x007E4660, 0x4, 24, 0x7, "ENVMAPTYPE"},
    {0x007E47C0, 0x007E4710, 0x4, 27, 0x3, "PROJLIGHTMAPCOUNT"},
    {0x007E4870, 0x007E47C0, 0x4, 29, 0xFFFFFFFF, "PROJLIGHTMAPTYPES"},
    {0x007E4920, 0x00000000, 0x8, 0, 0x7, "PROJLIGHTMAPCLIPPED"},
    {0x007E49B0, 0x007E4920, 0x8, 3, 0x3, "PROJSHADOWMAPCOUNT"},
    {0x007E4A60, 0x007E49B0, 0x8, 5, 0x7, "PROJSHADOWMAPTYPES"},
    {0x007E4B10, 0x007E4A60, 0x8, 8, 0x7, "PROJSHADOWMAPCLIPPED"},
    {0x007E4BC0, 0x007E4B10, 0x8, 11, 0x1, "PERVERTEXLIGHTING"},
    {0x007E4C70, 0x007E4BC0, 0x8, 12, 0xF, "UVSETFORMAP00"},
    {0x007E4D20, 0x007E4C70, 0x8, 16, 0xF, "UVSETFORMAP01"},
    {0x007E4DD0, 0x007E4D20, 0x8, 20, 0xF, "UVSETFORMAP02"},
    {0x007E4E80, 0x007E4DD0, 0x8, 24, 0xF, "UVSETFORMAP03"},
    {0x007E4F30, 0x007E4E80, 0x8, 28, 0xFFFFFFFF, "UVSETFORMAP04"},
    {0x007E4FE0, 0x00000000, 0xC, 0, 0xF, "UVSETFORMAP05"},
    {0x007E5070, 0x007E4FE0, 0xC, 4, 0xF, "UVSETFORMAP06"},
    {0x007E5120, 0x007E5070, 0xC, 8, 0xF, "UVSETFORMAP07"},
    {0x007E51D0, 0x007E5120, 0xC, 12, 0xF, "UVSETFORMAP08"},
    {0x007E5280, 0x007E51D0, 0xC, 16, 0xF, "UVSETFORMAP09"},
    {0x007E5330, 0x007E5280, 0xC, 20, 0xF, "UVSETFORMAP10"},
    {0x007E53E0, 0x007E5330, 0xC, 24, 0xF, "UVSETFORMAP11"},
    {0x007E5490, 0x007E53E0, 0xC, 28, 0xFFFFFFFF, "POINTLIGHTCOUNT"},
    {0x007E5540, 0x00000000, 0x10, 0, 0xF, "SPOTLIGHTCOUNT"},
    {0x007E55D0, 0x007E5540, 0x10, 4, 0xF, "DIRLIGHTCOUNT"},
    {0x007E5680, 0x007E55D0, 0x10, 8, 0xFF, "SHADOWMAPFORLIGHT"},
    {0x007E5730, 0x007E5680, 0x10, 16, 0x1, "SPECULAR"},
    {0x007E57E0, 0x007E5730, 0x10, 17, 0x3, "AMBDIFFEMISSIVE"},
    {0x007E5890, 0x007E57E0, 0x10, 19, 0x3, "LIGHTINGMODE"},
    {0x007E5940, 0x007E5890, 0x10, 21, 0x1, "APPLYAMBIENT"},
    {0x007E59F0, 0x007E5940, 0x10, 22, 0x1, "BASEMAPALPHAONLY"},
    {0x007E5AA0, 0x007E59F0, 0x10, 23, 0x1, "APPLYEMISSIVE"},
    {0x007E5B50, 0x007E5AA0, 0x10, 24, 0xF, "SHADOWTECHNIQUE"},
    {0x007E5C00, 0x007E5B50, 0x10, 28, 0x1, "ALPHATEST"},
    {0x007E60E0, 0x00000000, 0x4, 0, 0x3, "TRANSFORM"},
    {0x007E6170, 0x007E60E0, 0x4, 2, 0x1, "OUTPUTWORLDPOS"},
    {0x007E6220, 0x007E6170, 0x4, 3, 0x1, "OUTPUTWORLDNBT"},
    {0x007E62D0, 0x007E6220, 0x4, 4, 0x1, "OUTPUTWORLDVIEW"},
    {0x007E6380, 0x007E62D0, 0x4, 5, 0x1, "OUTPUTTANGENTVIEW"},
    {0x007E6430, 0x007E6380, 0x4, 6, 0x3, "NORMAL"},
    {0x007E64E0, 0x007E6430, 0x4, 8, 0x1, "SPECULAR"},
    {0x007E6590, 0x007E64E0, 0x4, 9, 0x3, "FOGTYPE"},
    {0x007E6640, 0x007E6590, 0x4, 11, 0x7, "ENVMAPTYPE"},
    {0x007E66F0, 0x007E6640, 0x4, 14, 0x3, "PROJLIGHTMAPCOUNT"},
    {0x007E67A0, 0x007E66F0, 0x4, 16, 0x7, "PROJLIGHTMAPTYPES"},
    {0x007E6850, 0x007E67A0, 0x4, 19, 0x3, "PROJSHADOWMAPCOUNT"},
    {0x007E6900, 0x007E6850, 0x4, 21, 0x7, "PROJSHADOWMAPTYPES"},
    {0x007E69B0, 0x007E6900, 0x4, 24, 0xF, "OUTPUTUVCOUNT"},
    {0x007E6A60, 0x007E69B0, 0x4, 28, 0x7, "UVSET00"},
    {0x007E6B10, 0x007E6A60, 0x4, 31, 0xFFFFFFFF, "UVSET00TEXOUTPUT"},
    {0x007E6BC0, 0x00000000, 0x8, 0, 0x7, "UVSET01"},
    {0x007E6C50, 0x007E6BC0, 0x8, 3, 0x1, "UVSET01TEXOUTPUT"},
    {0x007E6D00, 0x007E6C50, 0x8, 4, 0x7, "UVSET02"},
    {0x007E6DB0, 0x007E6D00, 0x8, 7, 0x1, "UVSET02TEXOUTPUT"},
    {0x007E6E60, 0x007E6DB0, 0x8, 8, 0x7, "UVSET03"},
    {0x007E6F10, 0x007E6E60, 0x8, 11, 0x1, "UVSET03TEXOUTPUT"},
    {0x007E6FC0, 0x007E6F10, 0x8, 12, 0x7, "UVSET04"},
    {0x007E7070, 0x007E6FC0, 0x8, 15, 0x1, "UVSET04TEXOUTPUT"},
    {0x007E7120, 0x007E7070, 0x8, 16, 0x7, "UVSET05"},
    {0x007E71D0, 0x007E7120, 0x8, 19, 0x1, "UVSET05TEXOUTPUT"},
    {0x007E7280, 0x007E71D0, 0x8, 20, 0x7, "UVSET06"},
    {0x007E7330, 0x007E7280, 0x8, 23, 0x1, "UVSET06TEXOUTPUT"},
    {0x007E73E0, 0x007E7330, 0x8, 24, 0x7, "UVSET07"},
    {0x007E7490, 0x007E73E0, 0x8, 27, 0x1, "UVSET07TEXOUTPUT"},
    {0x007E7540, 0x007E7490, 0x8, 28, 0x7, "UVSET08"},
    {0x007E75F0, 0x007E7540, 0x8, 31, 0xFFFFFFFF, "UVSET08TEXOUTPUT"},
    {0x007E76A0, 0x00000000, 0xC, 0, 0x7, "UVSET09"},
    {0x007E7730, 0x007E76A0, 0xC, 3, 0x1, "UVSET09TEXOUTPUT"},
    {0x007E77E0, 0x007E7730, 0xC, 4, 0x7, "UVSET10"},
    {0x007E7890, 0x007E77E0, 0xC, 7, 0x1, "UVSET10TEXOUTPUT"},
    {0x007E7940, 0x007E7890, 0xC, 8, 0x7, "UVSET11"},
    {0x007E79F0, 0x007E7940, 0xC, 11, 0x1, "UVSET11TEXOUTPUT"},
    {0x007E7AA0, 0x007E79F0, 0xC, 12, 0xF, "POINTLIGHTCOUNT"},
    {0x007E7B50, 0x007E7AA0, 0xC, 16, 0xF, "SPOTLIGHTCOUNT"},
    {0x007E7C00, 0x007E7B50, 0xC, 20, 0xF, "DIRLIGHTCOUNT"},
    {0x007E7CB0, 0x007E7C00, 0xC, 24, 0x1, "VERTEXCOLORS"},
    {0x007E7D60, 0x007E7CB0, 0xC, 25, 0x1, "VERTEXLIGHTSONLY"},
    {0x007E7E10, 0x007E7D60, 0xC, 26, 0x3, "AMBDIFFEMISSIVE"},
    {0x007E7EC0, 0x007E7E10, 0xC, 28, 0x3, "LIGHTINGMODE"},
    {0x007E7F70, 0x007E7EC0, 0xC, 30, 0x1, "APPLYMODE"},
};

constexpr const ProgramField& FieldAt(uintptr_t address) {
    for (const ProgramField& f : kFields) {
        if (f.address == address) {
            return f;
        }
    }
    return kFields[0];
}

// ToString<FIELD>(string, recurse): the fields before it first when `recurse` (its low byte; the
// whole word passed on)
template <uintptr_t Address>
void __fastcall FieldToString(const uint8_t* self, re::Edx, NiString* string, int recurse) {
    constexpr const ProgramField& f = FieldAt(Address);
    static_assert(f.address == Address);
    if constexpr (f.prev != 0) {
        if (re::Flag(recurse)) {
            reinterpret_cast<void(__thiscall*)(const uint8_t*, NiString*, int)>(f.prev)(self, string, recurse);
        }
    }
    uint32_t value = (At<uint32_t>(self, f.offset) >> f.shift) & f.mask;
    char buffer[0x100];
    if (NiString_Length(string) != 0) {
        NiString_Concatenate(string, "\n");
    }
    NiString_Concatenate(string, f.name);
    NiString_Concatenate(string, " = ");
    NiSprintf(buffer, 0x100, "%d", value);
    NiString_Concatenate(string, buffer);
}

template <uintptr_t Address>
void CallToString(const uint8_t* self, NiString* string) {
    reinterpret_cast<void(__thiscall*)(const uint8_t*, NiString*, int)>(Address)(self, string, 1);
}

// ---- NiStandardPixelProgramDescriptor

// the maps' UV set fields: the word and shift (4 bits each; map 4's is the top of its word)
constexpr uint32_t kUVSetOffset[12] = {8, 8, 8, 8, 8, 0xC, 0xC, 0xC, 0xC, 0xC, 0xC, 0xC};
constexpr uint32_t kUVSetShift[12] = {12, 16, 20, 24, 28, 0, 4, 8, 12, 16, 20, 24};

// 0x7E35C0 GetUVSetForMap(map) (-1 past the twelve)
uint32_t __fastcall PixelGetUVSetForMap(const uint8_t* self, re::Edx, uint32_t map) {
    if (map > 11) {
        return 0xFFFFFFFF;
    }
    uint32_t value = At<uint32_t>(self, kUVSetOffset[map]) >> kUVSetShift[map];
    return map == 4 ? value : value & 0xF;
}

// 0x7E3690 SetUVSetForMap(map, set) (the set not masked)
void __fastcall PixelSetUVSetForMap(uint8_t* self, re::Edx, uint32_t map, uint32_t set) {
    if (map > 11) {
        return;
    }
    uint32_t& word = At<uint32_t>(self, kUVSetOffset[map]);
    word = (word & ~(0xFu << kUVSetShift[map])) | (set << kUVSetShift[map]);
}

// 0x7E37F0 GetStandardTextureCount: the map count fields of the first word (the decals' three
// bits, the others' one)
uint32_t __fastcall PixelGetStandardTextureCount(const uint8_t* self, re::Edx) {
    uint32_t w = At<uint32_t>(self, 4);
    uint32_t count = (w >> 20) & 7;
    for (uint32_t bit = 19; bit >= 7; bit--) {
        count += (w >> bit) & 1;
    }
    return count;
}

// 0x7E3880 GetInputUVCount: one more than the highest UV set the textures use (the original
// keeps the count in edx across GetUVSetForMap: see the registration)
uint32_t __fastcall PixelGetInputUVCount(const uint8_t* self, re::Edx) {
    int32_t highest = -1;
    uint32_t count = PixelDesc_GetStandardTextureCount(self);
    for (uint32_t i = 0; i < count; i++) {
        int32_t set = static_cast<int32_t>(PixelDesc_GetUVSetForMap(self, i));
        if (set > highest) {
            highest = set;
        }
    }
    return uint32_t(highest + 1);
}

// 0x7E5CB0 ToString(result): every field, word by word
NiString* __fastcall PixelToString(const uint8_t* self, re::Edx, NiString* result) {
    NiString_ConstructSized(result, 0x100);
    CallToString<0x007E4870>(self, result);
    CallToString<0x007E4F30>(self, result);
    CallToString<0x007E5490>(self, result);
    CallToString<0x007E5C00>(self, result);
    return result;
}

// ---- NiStandardVertexProgramDescriptor

// the output UVs' fields: the word and the shift of the set (3 bits; the texture-output bit
// after it)
constexpr uint32_t kOutputOffset[12] = {4, 8, 8, 8, 8, 8, 8, 8, 8, 0xC, 0xC, 0xC};
constexpr uint32_t kOutputShift[12] = {28, 0, 4, 8, 12, 16, 20, 24, 28, 0, 4, 8};

// 0x7E5D00 SetOutputUV(output, set, texture output) (the values not masked; stored in two steps)
void __fastcall VertexSetOutputUV(uint8_t* self, re::Edx, uint32_t output, uint32_t set, uint32_t textureOutput) {
    if (output > 11) {
        return;
    }
    uint32_t& word = At<uint32_t>(self, kOutputOffset[output]);
    uint32_t shift = kOutputShift[output];
    uint32_t value = (word & ~(7u << shift)) | (set << shift);
    word = value;
    word = (value & ~(1u << (shift + 3))) | (textureOutput << (shift + 3));
}

// 0x7E5F20 GetOutputUV(output, set&, texture output&)
void __fastcall VertexGetOutputUV(const uint8_t* self, re::Edx, uint32_t output, uint32_t* set,
                                  uint32_t* textureOutput) {
    if (output > 11) {
        return;
    }
    uint32_t shift = kOutputShift[output];
    *set = (At<uint32_t>(self, kOutputOffset[output]) >> shift) & 7;
    *textureOutput = (At<uint32_t>(self, kOutputOffset[output]) >> (shift + 3)) & 1;
}

// 0x7E8020 ToString(result)
NiString* __fastcall VertexToString(const uint8_t* self, re::Edx, NiString* result) {
    NiString_ConstructSized(result, 0x100);
    CallToString<0x007E6B10>(self, result);
    CallToString<0x007E75F0>(self, result);
    CallToString<0x007E7F70>(self, result);
    return result;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIPROGDESC_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

// (callers rely on edx: GetInputUVCount (0x7E3880) keeps its count in edx across GetUVSetForMap:
// turn GetUVSetForMap on together with it)
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::GetUVSetForMap", 0x007E35C0, ni::PixelGetUVSetForMap);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::SetUVSetForMap", 0x007E3690, ni::PixelSetUVSetForMap);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::GetStandardTextureCount", 0x007E37F0, ni::PixelGetStandardTextureCount);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::GetInputUVCount", 0x007E3880, ni::PixelGetInputUVCount);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToString", 0x007E5CB0, ni::PixelToString);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::SetOutputUV", 0x007E5D00, ni::VertexSetOutputUV);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::GetOutputUV", 0x007E5F20, ni::VertexGetOutputUV);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToString", 0x007E8020, ni::VertexToString);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringAPPLYMODE", 0x007E38C0, ni::FieldToString<0x007E38C0>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringWORLDPOSITION", 0x007E3950, ni::FieldToString<0x007E3950>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringWORLDNORMAL", 0x007E3A00, ni::FieldToString<0x007E3A00>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringWORLDNBT", 0x007E3AB0, ni::FieldToString<0x007E3AB0>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringWORLDVIEW", 0x007E3B60, ni::FieldToString<0x007E3B60>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringNORMALMAPTYPE", 0x007E3C10, ni::FieldToString<0x007E3C10>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringPARALLAXMAPCOUNT", 0x007E3CC0, ni::FieldToString<0x007E3CC0>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringBASEMAPCOUNT", 0x007E3D70, ni::FieldToString<0x007E3D70>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringNORMALMAPCOUNT", 0x007E3E20, ni::FieldToString<0x007E3E20>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringDARKMAPCOUNT", 0x007E3ED0, ni::FieldToString<0x007E3ED0>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringDETAILMAPCOUNT", 0x007E3F80, ni::FieldToString<0x007E3F80>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringBUMPMAPCOUNT", 0x007E4030, ni::FieldToString<0x007E4030>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringGLOSSMAPCOUNT", 0x007E40E0, ni::FieldToString<0x007E40E0>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringGLOWMAPCOUNT", 0x007E4190, ni::FieldToString<0x007E4190>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringCUSTOMMAP00COUNT", 0x007E4240, ni::FieldToString<0x007E4240>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringCUSTOMMAP01COUNT", 0x007E42F0, ni::FieldToString<0x007E42F0>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringCUSTOMMAP02COUNT", 0x007E43A0, ni::FieldToString<0x007E43A0>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringCUSTOMMAP03COUNT", 0x007E4450, ni::FieldToString<0x007E4450>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringCUSTOMMAP04COUNT", 0x007E4500, ni::FieldToString<0x007E4500>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringDECALMAPCOUNT", 0x007E45B0, ni::FieldToString<0x007E45B0>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringFOGENABLED", 0x007E4660, ni::FieldToString<0x007E4660>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringENVMAPTYPE", 0x007E4710, ni::FieldToString<0x007E4710>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringPROJLIGHTMAPCOUNT", 0x007E47C0, ni::FieldToString<0x007E47C0>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringPROJLIGHTMAPTYPES", 0x007E4870, ni::FieldToString<0x007E4870>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringPROJLIGHTMAPCLIPPED", 0x007E4920, ni::FieldToString<0x007E4920>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringPROJSHADOWMAPCOUNT", 0x007E49B0, ni::FieldToString<0x007E49B0>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringPROJSHADOWMAPTYPES", 0x007E4A60, ni::FieldToString<0x007E4A60>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringPROJSHADOWMAPCLIPPED", 0x007E4B10, ni::FieldToString<0x007E4B10>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringPERVERTEXLIGHTING", 0x007E4BC0, ni::FieldToString<0x007E4BC0>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringUVSETFORMAP00", 0x007E4C70, ni::FieldToString<0x007E4C70>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringUVSETFORMAP01", 0x007E4D20, ni::FieldToString<0x007E4D20>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringUVSETFORMAP02", 0x007E4DD0, ni::FieldToString<0x007E4DD0>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringUVSETFORMAP03", 0x007E4E80, ni::FieldToString<0x007E4E80>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringUVSETFORMAP04", 0x007E4F30, ni::FieldToString<0x007E4F30>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringUVSETFORMAP05", 0x007E4FE0, ni::FieldToString<0x007E4FE0>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringUVSETFORMAP06", 0x007E5070, ni::FieldToString<0x007E5070>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringUVSETFORMAP07", 0x007E5120, ni::FieldToString<0x007E5120>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringUVSETFORMAP08", 0x007E51D0, ni::FieldToString<0x007E51D0>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringUVSETFORMAP09", 0x007E5280, ni::FieldToString<0x007E5280>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringUVSETFORMAP10", 0x007E5330, ni::FieldToString<0x007E5330>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringUVSETFORMAP11", 0x007E53E0, ni::FieldToString<0x007E53E0>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringPOINTLIGHTCOUNT", 0x007E5490, ni::FieldToString<0x007E5490>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringSPOTLIGHTCOUNT", 0x007E5540, ni::FieldToString<0x007E5540>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringDIRLIGHTCOUNT", 0x007E55D0, ni::FieldToString<0x007E55D0>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringSHADOWMAPFORLIGHT", 0x007E5680, ni::FieldToString<0x007E5680>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringSPECULAR", 0x007E5730, ni::FieldToString<0x007E5730>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringAMBDIFFEMISSIVE", 0x007E57E0, ni::FieldToString<0x007E57E0>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringLIGHTINGMODE", 0x007E5890, ni::FieldToString<0x007E5890>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringAPPLYAMBIENT", 0x007E5940, ni::FieldToString<0x007E5940>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringBASEMAPALPHAONLY", 0x007E59F0, ni::FieldToString<0x007E59F0>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringAPPLYEMISSIVE", 0x007E5AA0, ni::FieldToString<0x007E5AA0>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringSHADOWTECHNIQUE", 0x007E5B50, ni::FieldToString<0x007E5B50>);
NIPROGDESC_REVERSED("NiStandardPixelProgramDescriptor::ToStringALPHATEST", 0x007E5C00, ni::FieldToString<0x007E5C00>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringTRANSFORM", 0x007E60E0, ni::FieldToString<0x007E60E0>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringOUTPUTWORLDPOS", 0x007E6170, ni::FieldToString<0x007E6170>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringOUTPUTWORLDNBT", 0x007E6220, ni::FieldToString<0x007E6220>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringOUTPUTWORLDVIEW", 0x007E62D0, ni::FieldToString<0x007E62D0>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringOUTPUTTANGENTVIEW", 0x007E6380, ni::FieldToString<0x007E6380>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringNORMAL", 0x007E6430, ni::FieldToString<0x007E6430>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringSPECULAR", 0x007E64E0, ni::FieldToString<0x007E64E0>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringFOGTYPE", 0x007E6590, ni::FieldToString<0x007E6590>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringENVMAPTYPE", 0x007E6640, ni::FieldToString<0x007E6640>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringPROJLIGHTMAPCOUNT", 0x007E66F0, ni::FieldToString<0x007E66F0>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringPROJLIGHTMAPTYPES", 0x007E67A0, ni::FieldToString<0x007E67A0>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringPROJSHADOWMAPCOUNT", 0x007E6850, ni::FieldToString<0x007E6850>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringPROJSHADOWMAPTYPES", 0x007E6900, ni::FieldToString<0x007E6900>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringOUTPUTUVCOUNT", 0x007E69B0, ni::FieldToString<0x007E69B0>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringUVSET00", 0x007E6A60, ni::FieldToString<0x007E6A60>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringUVSET00TEXOUTPUT", 0x007E6B10, ni::FieldToString<0x007E6B10>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringUVSET01", 0x007E6BC0, ni::FieldToString<0x007E6BC0>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringUVSET01TEXOUTPUT", 0x007E6C50, ni::FieldToString<0x007E6C50>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringUVSET02", 0x007E6D00, ni::FieldToString<0x007E6D00>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringUVSET02TEXOUTPUT", 0x007E6DB0, ni::FieldToString<0x007E6DB0>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringUVSET03", 0x007E6E60, ni::FieldToString<0x007E6E60>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringUVSET03TEXOUTPUT", 0x007E6F10, ni::FieldToString<0x007E6F10>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringUVSET04", 0x007E6FC0, ni::FieldToString<0x007E6FC0>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringUVSET04TEXOUTPUT", 0x007E7070, ni::FieldToString<0x007E7070>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringUVSET05", 0x007E7120, ni::FieldToString<0x007E7120>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringUVSET05TEXOUTPUT", 0x007E71D0, ni::FieldToString<0x007E71D0>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringUVSET06", 0x007E7280, ni::FieldToString<0x007E7280>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringUVSET06TEXOUTPUT", 0x007E7330, ni::FieldToString<0x007E7330>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringUVSET07", 0x007E73E0, ni::FieldToString<0x007E73E0>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringUVSET07TEXOUTPUT", 0x007E7490, ni::FieldToString<0x007E7490>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringUVSET08", 0x007E7540, ni::FieldToString<0x007E7540>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringUVSET08TEXOUTPUT", 0x007E75F0, ni::FieldToString<0x007E75F0>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringUVSET09", 0x007E76A0, ni::FieldToString<0x007E76A0>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringUVSET09TEXOUTPUT", 0x007E7730, ni::FieldToString<0x007E7730>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringUVSET10", 0x007E77E0, ni::FieldToString<0x007E77E0>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringUVSET10TEXOUTPUT", 0x007E7890, ni::FieldToString<0x007E7890>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringUVSET11", 0x007E7940, ni::FieldToString<0x007E7940>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringUVSET11TEXOUTPUT", 0x007E79F0, ni::FieldToString<0x007E79F0>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringPOINTLIGHTCOUNT", 0x007E7AA0, ni::FieldToString<0x007E7AA0>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringSPOTLIGHTCOUNT", 0x007E7B50, ni::FieldToString<0x007E7B50>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringDIRLIGHTCOUNT", 0x007E7C00, ni::FieldToString<0x007E7C00>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringVERTEXCOLORS", 0x007E7CB0, ni::FieldToString<0x007E7CB0>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringVERTEXLIGHTSONLY", 0x007E7D60, ni::FieldToString<0x007E7D60>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringAMBDIFFEMISSIVE", 0x007E7E10, ni::FieldToString<0x007E7E10>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringLIGHTINGMODE", 0x007E7EC0, ni::FieldToString<0x007E7EC0>);
NIPROGDESC_REVERSED("NiStandardVertexProgramDescriptor::ToStringAPPLYMODE", 0x007E7F70, ni::FieldToString<0x007E7F70>);
