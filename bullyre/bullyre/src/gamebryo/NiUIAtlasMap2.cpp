// NiUIAtlasMap, the rest (the first part is NiUIAtlasMap.cpp; the layout is described there and
// in NiSampleFramework.h: the skin texture +4, the outer rects +8 and inner rects +0x2B8 (43
// each, texture-normalized), the three font names +0x568 (NiStrings), five NiColorA +0x574 and
// one pixel in normalized screen units +0x5D4 / +0x5D8): its constructor, the deleting
// destructor, the rect conversions, the border sizes and ReadUIFile, which loads the skin
// texture and reads its ".bdr" description ([FILES], [IMAGERECTS] and [COLORS] sections, '#'
// comments) next to it. Float maths in float, in the game's order.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiFile.h"
#include "gamebryo/NiSampleFramework.h"
#include "reversed.h"

namespace ni::sample {
namespace {

constexpr uint32_t kNiUIAtlasMapVtable = 0x00989C68;
constexpr uint32_t kRectCount = 0x2B;
constexpr int kOuterRects = 0x8;
constexpr int kInnerRects = 0x2B8;
constexpr int kFontNames = 0x568;
constexpr int kColors = 0x574;
constexpr int kPixelWidth = 0x5D4;    // one pixel, in normalized screen units
constexpr int kPixelHeight = 0x5D8;

GAME_VAR(void*, NiRenderer_ms_pkRenderer, 0x020E2380);
GAME_ARRAY(uint32_t, kDefaultFormatPrefs, 0x00BC7624);   // NiTexture::FormatPrefs()
const char* const kBdrExtension = reinterpret_cast<const char*>(0x00989C6C);   // ".bdr"

GAME_FN(re::Ptr32<char>*, __thiscall, NiString_Construct, 0x007741A0, re::Ptr32<char>* self, const char* text);
GAME_FN(re::Ptr32<char>*, __thiscall, NiString_ConstructSized, 0x007740D0, re::Ptr32<char>* self, uint32_t size);
GAME_FN(void, __thiscall, NiString_Destruct, 0x00774240, re::Ptr32<char>* self);
GAME_FN(const char*, __thiscall, NiString_CStr, 0x00773DB0, const re::Ptr32<char>* self); // operator const char*
GAME_FN(int, __thiscall, NiString_FindReverse, 0x007743F0, const re::Ptr32<char>* self, char c);
GAME_FN(re::Ptr32<char>*, __thiscall, NiString_GetSubstring, 0x00774310, const re::Ptr32<char>* self, re::Ptr32<char>* result, uint32_t begin,
        uint32_t end);
GAME_FN(re::Ptr32<char>*, __thiscall, NiString_AssignText, 0x007745E0, re::Ptr32<char>* self, const char* text);
GAME_FN(void, __thiscall, NiString_Concatenate, 0x00774870, re::Ptr32<char>* self, const char* text);
GAME_FN(NiRefObject*, __cdecl, NiSourceTexture_CreateFromFile, 0x00769910, const re::Ptr32<const char>* name,
        const uint32_t* prefs, int loadDirect, int srcRendererDataIsPersistent);
// NiRenderer: a pixel size to normalized screen units (through its default render target)
GAME_FN(void, __thiscall, Renderer_ConvertPixelsToNormalized, 0x00755790, void* self, uint32_t x, uint32_t y,
        float* outX, float* outY);
GAME_FN(NiFile*, __thiscall, NiFile_Construct, 0x00752590, NiFile* self, const char* name, int mode,
        uint32_t bufferSize);
GAME_FN(uint32_t, __thiscall, NiFile_GetFileSize, 0x00752680, NiFile* self);
GAME_FN(void, __thiscall, NiFile_Destruct, 0x00752720, NiFile* self);
GAME_FN(uint32_t, __thiscall, NiBinaryStream_GetLine, 0x00752B10, NiFile* self, char* buffer, uint32_t size);

// NiUIAtlasMap.cpp's (called where modes apply)
GAME_FN(void, __thiscall, AtlasMap_Destruct, 0x0083BBB0, void* self);
GAME_FN(uint32_t, __thiscall, AtlasMap_ParseSection, 0x0083B8E0, void* self, re::Ptr32<char>* cursor);
GAME_FN(uint32_t, __thiscall, AtlasMap_ParseImageName, 0x0083B950, void* self, re::Ptr32<char>* cursor);
GAME_FN(uint32_t, __thiscall, AtlasMap_ParseTextName, 0x0083B9B0, void* self, re::Ptr32<char>* cursor);
GAME_FN(uint32_t, __thiscall, AtlasMap_ParseFontName, 0x0083BA10, void* self, re::Ptr32<char>* cursor);
GAME_FN(uint8_t, __thiscall, AtlasMap_ParseString, 0x0083BA70, void* self, re::Ptr32<char>* cursor, re::Ptr32<char>* out);
GAME_FN(uint8_t, __thiscall, AtlasMap_ParseInt, 0x0083BAF0, void* self, re::Ptr32<char>* cursor, int32_t* out);
GAME_FN(uint8_t, __thiscall, AtlasMap_ParseFloat, 0x0083BB50, void* self, re::Ptr32<char>* cursor, float* out);
GAME_FN(float*, __thiscall, AtlasMap_GetOuterRect, 0x00832EF0, void* self, float* result, uint32_t index,
        uint32_t mode);
GAME_FN(float*, __thiscall, AtlasMap_GetInnerRect, 0x00839060, void* self, float* result, uint32_t index,
        uint32_t mode);
GAME_FN(float*, __thiscall, AtlasMap_ToPixels, 0x0083C1D0, void* self, float* result, const float* rect);
GAME_FN(void, __cdecl, OperatorDelete, 0x005EEAB0, void* p);

// a texture size (unsigned) as a float, as the code converts it
inline float SizeToFloat(uint32_t size) {
    return float(size);
}

// 0x83C1D0: a texture-normalized rect in the skin texture's pixels
float* __fastcall ToPixels(uint8_t* self, re::Edx, float* result, const float* rect) {
    NiRefObject* texture = At<NiRefObject*>(self, 4);
    float width = SizeToFloat(Virtual<uint32_t>(texture, 18));    // GetWidth
    float height = SizeToFloat(Virtual<uint32_t>(At<NiRefObject*>(self, 4), 19));   // GetHeight
    result[0] = rect[0];
    result[1] = rect[1];
    result[2] = rect[2];
    result[3] = rect[3];
    result[0] = result[0] * width;
    result[1] = width * result[1];
    result[2] = result[2] * height;
    result[3] = height * result[3];
    return result;
}

// 0x83C270: the same in normalized screen units (its pixels times one screen pixel's size)
float* __fastcall ToNSC(uint8_t* self, re::Edx, float* result, const float* rect) {
    AtlasMap_ToPixels(self, result, rect);
    result[0] = result[0] * At<float>(self, kPixelWidth);
    result[1] = result[1] * At<float>(self, kPixelWidth);
    result[2] = At<float>(self, kPixelHeight) * result[2];
    result[3] = At<float>(self, kPixelHeight) * result[3];
    return result;
}

// 0x83C2C0: everything zero, the rects (0, 1, 0, 1), the font names empty 256-character
// NiStrings, the five colours white (the 24 floats after them stay zero; the pixel size isn't set)
void* __fastcall Construct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kNiUIAtlasMapVtable;
    At<NiRefObject*>(self, 4) = nullptr;
    float* outer = reinterpret_cast<float*>(self + kOuterRects);
    float* inner = reinterpret_cast<float*>(self + kInnerRects);
    for (uint32_t i = 0; i < kRectCount * 4; i++) {
        outer[i] = 0.0f;
    }
    for (uint32_t i = 0; i < kRectCount * 4; i++) {
        inner[i] = 0.0f;
    }
    for (int i = 0; i < 3; i++) {
        NiString_ConstructSized(reinterpret_cast<re::Ptr32<char>*>(self + kFontNames + i * 4), 0x100);
    }
    float* colors = reinterpret_cast<float*>(self + kColors);
    for (int i = 0; i < 24; i++) {
        colors[i] = 0.0f;
    }
    for (uint32_t i = 0; i < kRectCount; i++) {
        outer[i * 4 + 0] = 0.0f;
        outer[i * 4 + 1] = 1.0f;
        outer[i * 4 + 2] = 0.0f;
        outer[i * 4 + 3] = 1.0f;
        inner[i * 4 + 0] = 0.0f;
        inner[i * 4 + 1] = 1.0f;
        inner[i * 4 + 2] = 0.0f;
        inner[i * 4 + 3] = 1.0f;
    }
    for (int i = 0; i < 20; i++) {
        colors[i] = 1.0f;
    }
    return self;
}

// 0x83C480: the deleting destructor (the game's operator delete: the object comes from new)
void* __fastcall DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    AtlasMap_Destruct(self);
    if ((flags & 1) != 0) {
        OperatorDelete(self);
    }
    return self;
}

// 0x83C4A0: GetBorder(result, index, mode): an image's frame sizes, |inner - outer| per edge
float* __fastcall GetBorder(uint8_t* self, re::Edx, float* result, uint32_t index, uint32_t mode) {
    float outer[4];
    float inner[4];
    AtlasMap_GetOuterRect(self, outer, index, mode);
    AtlasMap_GetInnerRect(self, inner, index, mode);
    result[0] = std::fabs(inner[0] - outer[0]);
    result[1] = std::fabs(outer[1] - inner[1]);
    result[2] = std::fabs(inner[2] - outer[2]);
    result[3] = std::fabs(outer[3] - inner[3]);
    return result;
}

// one rect from the file: x, y, width, height in the texture's pixels, made texture-normalized
// (left, right, top, bottom)
inline void SetRect(float* rect, int32_t x, int32_t y, int32_t w, int32_t h, float width, float height) {
    float left = float(x) / width;
    rect[0] = left;
    rect[1] = float(w) / width + left;
    float top = float(y) / height;
    rect[2] = top;
    rect[3] = float(h) / height + top;
}

// 0x83BC30: ReadUIFile(filename): the skin texture loaded (its pixel data read for the size),
// then "<name without extension>.bdr" read line by line: [FILES] font name lines, [IMAGERECTS]
// image lines (the outer rect, then optionally the inner one: without it the inner is the
// outer), [COLORS] text colour lines. A bad line fails the whole read (what was read stays)
uint8_t __fastcall ReadUIFile(uint8_t* self, re::Edx, const char* filename) {
    if (filename == nullptr) {
        return 0;
    }
    re::Ptr32<NiRefObject>& texture = At<NiRefObject*>(self, 4);
    ClearRef(texture);
    re::Ptr32<char> file = nullptr;
    NiString_Construct(&file, filename);
    const char* text = NiString_CStr(&file);
    NiFixedString name = text == nullptr ? static_cast<char*>(ni::sys::kNullFixedString) : ni::sys::GlobalStringTable_AddString(text);
    NiRefObject* created = NiSourceTexture_CreateFromFile(&name, kDefaultFormatPrefs, 0, 0);
    AssignRef(texture, created);
    ni::sys::FixedStringRelease(re::PtrCast<re::Ptr32<char>*>(&name));
    uint8_t result = 0;
    if (texture != nullptr) {
        Virtual<void>(texture, 20);   // LoadPixelDataFromFile
        NiRefObject* loaded = texture;
        float width = SizeToFloat(Virtual<uint32_t>(loaded, 18));
        float height = SizeToFloat(Virtual<uint32_t>(loaded, 19));
        if (NiRenderer_ms_pkRenderer != nullptr) {
            Renderer_ConvertPixelsToNormalized(NiRenderer_ms_pkRenderer, 1, 1, &At<float>(self, kPixelWidth),
                                               &At<float>(self, kPixelHeight));
            re::Ptr32<char> bdr = nullptr;
            NiString_ConstructSized(&bdr, 0x100);
            int dot = NiString_FindReverse(&file, '.');
            re::Ptr32<char> stem = nullptr;
            NiString_GetSubstring(&file, &stem, 0, uint32_t(dot));
            NiString_AssignText(&bdr, NiString_CStr(&stem));
            NiString_Destruct(&stem);
            NiString_Concatenate(&bdr, kBdrExtension);
            NiFile in;
            NiFile_Construct(&in, NiString_CStr(&bdr), 0, 0x8000);
            bool ok = false;
            if (Virtual<uint8_t>(&in, 1) != 0 && NiFile_GetFileSize(&in) != 0) {   // operator bool
                char line[0x100];
                uint32_t section = 0xFFFFFFFF;
                ok = true;
                while (ok && NiBinaryStream_GetLine(&in, line, 0x100) != 0) {
                    if (line[0] == '#' || line[0] == '\0') {
                        continue;
                    }
                    re::Ptr32<char> cursor = line;
                    uint32_t found = AtlasMap_ParseSection(self, &cursor);
                    if (found != 0xFFFFFFFF) {
                        section = found;
                        continue;
                    }
                    if (section == 0xFFFFFFFF) {
                        continue;
                    }
                    if (section == 0) {
                        uint32_t i = AtlasMap_ParseFontName(self, &cursor);
                        if (i == 0xFFFFFFFF ||
                            AtlasMap_ParseString(self, &cursor, reinterpret_cast<re::Ptr32<char>*>(self + kFontNames + i * 4)) == 0) {
                            ok = false;
                        }
                    } else if (section == 1) {
                        uint32_t i = AtlasMap_ParseImageName(self, &cursor);
                        int32_t x = 0;
                        int32_t y = 0;
                        int32_t w = 0;
                        int32_t h = 0;
                        if (i == 0xFFFFFFFF || AtlasMap_ParseInt(self, &cursor, &x) == 0 ||
                            AtlasMap_ParseInt(self, &cursor, &y) == 0 || AtlasMap_ParseInt(self, &cursor, &w) == 0 ||
                            AtlasMap_ParseInt(self, &cursor, &h) == 0) {
                            ok = false;
                            continue;
                        }
                        float* outer = reinterpret_cast<float*>(self + kOuterRects + i * 16);
                        float* inner = reinterpret_cast<float*>(self + kInnerRects + i * 16);
                        SetRect(outer, x, y, w, h, width, height);
                        inner[0] = outer[0];
                        inner[1] = outer[1];
                        inner[2] = outer[2];
                        inner[3] = outer[3];
                        if (AtlasMap_ParseInt(self, &cursor, &x) != 0 && AtlasMap_ParseInt(self, &cursor, &y) != 0 &&
                            AtlasMap_ParseInt(self, &cursor, &w) != 0 && AtlasMap_ParseInt(self, &cursor, &h) != 0) {
                            SetRect(inner, x, y, w, h, width, height);
                        }
                    } else if (section == 2) {
                        uint32_t i = AtlasMap_ParseTextName(self, &cursor);
                        float color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
                        if (i == 0xFFFFFFFF || AtlasMap_ParseFloat(self, &cursor, &color[0]) == 0 ||
                            AtlasMap_ParseFloat(self, &cursor, &color[1]) == 0 ||
                            AtlasMap_ParseFloat(self, &cursor, &color[2]) == 0 ||
                            AtlasMap_ParseFloat(self, &cursor, &color[3]) == 0) {
                            ok = false;
                            continue;
                        }
                        float* out = reinterpret_cast<float*>(self + kColors + i * 16);
                        out[0] = color[0];
                        out[1] = color[1];
                        out[2] = color[2];
                        out[3] = color[3];
                    }
                }
            }
            NiFile_Destruct(&in);
            NiString_Destruct(&bdr);
            result = ok ? 1 : 0;
        }
    }
    NiString_Destruct(&file);
    return result;
}

} // namespace

#define NIUIATLAS2_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                                \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIUIATLAS2_REVERSED("NiUIAtlasMap::ReadUIFile", 0x0083BC30, &ReadUIFile);
NIUIATLAS2_REVERSED("NiUIAtlasMap::ConvertToPixels", 0x0083C1D0, &ToPixels);
NIUIATLAS2_REVERSED("NiUIAtlasMap::ConvertToNSC", 0x0083C270, &ToNSC);
NIUIATLAS2_REVERSED("NiUIAtlasMap::NiUIAtlasMap", 0x0083C2C0, &Construct);
NIUIATLAS2_REVERSED("NiUIAtlasMap::DeletingDestructor", 0x0083C480, &DeletingDestructor);
NIUIATLAS2_REVERSED("NiUIAtlasMap::GetBorder", 0x0083C4A0, &GetBorder);

} // namespace ni::sample
