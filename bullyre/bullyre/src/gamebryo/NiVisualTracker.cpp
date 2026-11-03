// NiApplication's visual trackers (Gamebryo 2.x's NiVisualTracker, NiScreenText and the graph
// callbacks; no NiRTTI, named after Gamebryo 2.x's sample framework: a guess): a tracker is a
// box of graph lines on the screen (an NiNode holding one NiLines per graph and the box with its
// quarter lines), a name and five value labels, each graph sampling a callback object every
// interval. NiScreenText puts a string on an NiScreenTexture with the built-in 11x21 font (the
// pixels at 0x968E60, 23 glyphs a row, '!'..'z'). Plus NiVisualTrackerRenderClick's rendering,
// the two small arrays' helpers that sit among them, and the callbacks NiApplication's
// CreateVisualTrackers makes (frame rate, visible objects, a float or an unsigned value).
//
// Layouts (from the constructors 0x82FAD0, 0x830880, 0x82F560 and the sizes the code allocates):
// NiVisualTracker 0x178 (vtable 0x988E7C, an NiRefObject), its graph 0x140 (no vtable),
// NiScreenText 0x40 (no vtable), the callbacks 8 or 0xC (base vtable 0x99CEE8: slot 0 the
// deleting destructor, slot 1 TakeSample(time)).
//
// Main-thread float maths at the game's 24-bit x87 precision: float operations in the game's
// order; the inline chopped conversions (fistp of a 64-bit integer, the low half kept) are
// crt::FloatToInt.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::app {
namespace {

using sys::At;
using sys::Virtual;

// NiScreenText (0x40): a string drawn as one screen rect per character on its own
// NiScreenTexture (all of them over the one font texture, ms_spTexture)
struct NiScreenText {
    re::Ptr32<NiRefObject> screenTexture;              // +0x0 (a reference: NiScreenTexture)
    re::Ptr32<NiTArray<NiRefObject*>> screenTextures;  // +0x4 (the owner's: where it's shown)
    float color[4];                          // +0x8 (NiColorA)
    int32_t x;                               // +0x18 (pixels: RecreateText uses the low 16 bits)
    int32_t y;                               // +0x1C
    re::Ptr32<char> text;                              // +0x20 (NiMalloc'd: maxChars)
    uint32_t maxChars;                       // +0x24
    uint32_t numChars;                       // +0x28 (the length + 1: the terminator counted)
    uint32_t maxCols;                        // +0x2C (40)
    uint32_t unknown30;                      // +0x30 (0)
    uint32_t row;                            // +0x34
    uint32_t column;                         // +0x38
    uint8_t scrollDown;                      // +0x3C (1: new lines below; 0: the old ones move up)
};
static_assert(offsetof(NiScreenText, x) == 0x18);
static_assert(offsetof(NiScreenText, numChars) == 0x28);
static_assert(offsetof(NiScreenText, scrollDown) == 0x3C);
static_assert(sizeof(NiScreenText) == 0x40);

// NiVisualTracker::GraphCallbackObject: slot 0 the deleting destructor, slot 1 TakeSample(time)
struct GraphCallback {
    re::Ptr32<const void> vtable;
};
struct FrameRateUpdate : GraphCallback {        // vtable 0x988E84 (8)
    re::Ptr32<const float> frameRate;                     // +0x4
};
struct VisibleArraySize : GraphCallback {       // vtable 0x988E8C (8)
    re::Ptr32<const void> visible;                        // +0x4 (NiVisibleArray: its count at +4)
};
struct GenericFloatUpdate : GraphCallback {     // vtable 0x988E94 (0xC)
    float scale;                                // +0x4 (the value divided by it)
    re::Ptr32<const float> value;                         // +0x8
};
struct GenericUnsignedUpdate : GraphCallback {  // vtable 0x988E9C (0xC)
    float scale;                                // +0x4
    re::Ptr32<const uint32_t> value;                      // +0x8
};
static_assert(sizeof(FrameRateUpdate) == 8);
static_assert(sizeof(GenericUnsignedUpdate) == 0xC);

// NiVisualTracker::GraphData (0x140): one graph's line strip, its sampling and its name
struct GraphData {
    re::Ptr32<GraphCallback> callback;    // +0x0 (owned: deleted with the graph)
    uint8_t show;               // +0x4
    uint32_t numSamples;        // +0x8
    float interval;             // +0xC (the sampling period, seconds)
    re::Ptr32<NiRefObject> lines;         // +0x10 (a reference: NiLines, numSamples vertices)
    float color[3];             // +0x14 (NiColor)
    float lastTime;             // +0x20 (-FLT_MAX: no sample yet)
    char name[0x100];           // +0x24
    float maxValue;             // +0x124 (samples clamped to it)
    uint32_t lastIndex;         // +0x128 (numSamples - 1)
    float rect[4];              // +0x12C (the tracker's graph rect: left, right, top, bottom)
    re::Ptr32<NiScreenText> text;         // +0x13C (owned: the name)
};
static_assert(offsetof(GraphData, lastTime) == 0x20);
static_assert(offsetof(GraphData, maxValue) == 0x124);
static_assert(offsetof(GraphData, rect) == 0x12C);
static_assert(sizeof(GraphData) == 0x140);

// NiVisualTracker (vtable 0x988E7C, 0x178): slot 0 the deleting destructor, 1 DeleteThis
struct NiVisualTracker : NiRefObject {
    NiTArray<GraphData*> graphs;           // +0x8 (NiTPrimitiveArray, vtable 0x961988: owned)
    float rect[4];                         // +0x18 (left, right, top, bottom: normalized screen)
    float graphRect[4];                    // +0x28 (0, the graphs' width, the name row's height,
                                           //  the bottom: in the node's space)
    re::Ptr32<NiNode> node;                          // +0x38 (a reference: the graphs' and the box's parent)
    re::Ptr32<NiRefObject> lines;                    // +0x3C (a reference: NiLines, the box and 3 lines)
    float maxValue;                        // +0x40
    uint8_t show;                          // +0x44
    char name[0x100];                      // +0x45
    uint8_t pad145[3];
    re::Ptr32<NiScreenText> nameText;                // +0x148 (owned)
    re::Ptr32<NiScreenText> labels[5];               // +0x14C (owned: 0, max/4, max/2, 3max/4, max)
    NiTArray<NiRefObject*> screenTextures; // +0x160 (NiTObjectArray, vtable 0x9893C0: the
                                           //  shown texts' screen textures, drawn by Draw)
    int32_t nextTextX;                     // +0x170 (pixels: where the next graph's name goes)
    int32_t nextTextY;                     // +0x174
};
static_assert(offsetof(NiVisualTracker, rect) == 0x18);
static_assert(offsetof(NiVisualTracker, node) == 0x38);
static_assert(offsetof(NiVisualTracker, show) == 0x44);
static_assert(offsetof(NiVisualTracker, nameText) == 0x148);
static_assert(offsetof(NiVisualTracker, screenTextures) == 0x160);
static_assert(offsetof(NiVisualTracker, nextTextY) == 0x174);
static_assert(sizeof(NiVisualTracker) == 0x178);

constexpr uintptr_t NiRefObject_vtable = 0x008FEFD4;
constexpr uintptr_t NiVisualTracker_vtable = 0x00988E7C;
constexpr uintptr_t NiTPrimitiveArray_vtable = 0x00961988;
constexpr uintptr_t NiTObjectArray_vtable = 0x009893C0;
constexpr uintptr_t GraphCallbackObject_vtable = 0x0099CEE8;
constexpr uintptr_t FrameRateUpdate_vtable = 0x00988E84;
constexpr uintptr_t VisibleArraySize_vtable = 0x00988E8C;
constexpr uintptr_t GenericFloatUpdate_vtable = 0x00988E94;
constexpr uintptr_t GenericUnsignedUpdate_vtable = 0x00988E9C;
constexpr uintptr_t NiZBufferProperty_vtable = 0x008FFCA4;

GAME_VAR(uint32_t, NiRefObject_ms_uiObjects, 0x020E2168);
GAME_VAR(void*, NiRenderer_ms_pkRenderer, 0x020E2380);
GAME_VAR(NiRefObject*, NiScreenText_ms_pkFontPixelData, 0x020E59C0);   // (raw: never let go)
GAME_VAR(int32_t, NiScreenText_ms_iCount, 0x020E59C4);
GAME_VAR(NiRefObject*, NiScreenText_ms_spTexture, 0x020E59C8);
// the font: a rect's size, a glyph's size, glyphs a row
GAME_VAR(uint16_t, NiScreenText_ms_usCharWidth, 0x00BC8F68);    // 11
GAME_VAR(uint16_t, NiScreenText_ms_usCharHeight, 0x00BC8F6C);   // 21
GAME_VAR(uint32_t, NiScreenText_ms_uiCharWidth, 0x00BC8F70);    // 11
GAME_VAR(uint32_t, NiScreenText_ms_uiCharHeight, 0x00BC8F74);   // 21
GAME_VAR(uint32_t, NiScreenText_ms_uiCharsPerRow, 0x00BC8F78);  // 23
GAME_ARRAY(uint8_t, kFontPixels, 0x00968E60);                   // 256 x 128 RGBA
GAME_VAR(uint8_t, kFontPixelFormat, 0x00BC7750);                // NiPixelFormat (32-bit RGBA)
GAME_VAR(uint8_t, kFontTexturePrefs, 0x00BC7624);               // NiSourceTexture::FormatPrefs
GAME_ARRAY(float, kWhite, 0x00BC75E4);                          // NiColorA (1, 1, 1, 1)
GAME_ARRAY(float, kBoxColor, 0x00BC75C8);                       // NiColor (1, 1, 1)
GAME_VAR(float, kTrackerDepth, 0x00900170);                     // 0.1
GAME_VAR(float, kNoSample, 0x009009BC);                         // -FLT_MAX
constexpr double kNoSampleDouble = -3.4028234663852886e+38;     // (0x953E90: -FLT_MAX as a double)

GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(float, __cdecl, NiGetCurrentTimeInSec, 0x007537B0);
GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(re::Ptr32<NiRefObject>*, __cdecl, NiNewPointerArray, 0x00856120, uint32_t count);
GAME_FN(void, __cdecl, Array_Free, 0x00836F80, void* p);   // (NiTObjectArray's allocator)
GAME_FN(NiRefObject*, __thiscall, PixelData_Construct, 0x0076A610, void* self, uint32_t width, uint32_t height,
        const void* format, uint32_t mipmaps, uint32_t faces);
GAME_FN(NiRefObject*, __cdecl, SourceTexture_Create, 0x007699A0, NiRefObject* pixels, const void* prefs);
GAME_FN(NiRefObject*, __thiscall, ScreenTexture_Construct, 0x007DBAB0, void* self, NiRefObject* texture);
GAME_FN(void, __thiscall, ScreenTexture_RemoveAllScreenRects, 0x007DB6B0, NiRefObject* self);
GAME_FN(void, __thiscall, ScreenTexture_AddScreenRect, 0x007DBBB0, NiRefObject* self, uint32_t top, uint32_t left,
        uint32_t width, uint32_t height, uint32_t textureTop, uint32_t textureLeft, float r, float g, float b,
        float a);
GAME_FN(void, __thiscall, ScreenTexture_Draw, 0x007DB6C0, void* self, void* renderer);
GAME_FN(uint32_t, __thiscall, ObjectArray_AddFirstEmpty, 0x007606C0, void* array, const re::Ptr32<ni::NiRefObject>* value);
GAME_FN(void, __thiscall, PointerArray_Compact, 0x0040EB20, void* self);
GAME_FN(uint32_t, __thiscall, GraphArray_AddFirstEmpty, 0x008A4D20, void* array, const re::Ptr32<GraphData>* value);
GAME_FN(NiNode*, __thiscall, Node_Construct, 0x00760780, void* self, uint32_t childCount);
GAME_FN(NiRefObject*, __thiscall, Lines_Construct, 0x00779910, void* self, uint32_t vertices, NiPoint3* points,
        void* colors, void* textures, uint32_t textureSets, uint32_t nbtMethod, uint8_t* connections);
GAME_FN(void, __thiscall, Matrix3_MakeIdentity, 0x00768680, NiMatrix3* self);
GAME_FN(void*, __thiscall, MaterialProperty_Construct, 0x00775660, void* self);
GAME_FN(void, __thiscall, NiProperty_Construct, 0x00764550, void* self);
GAME_FN(void, __thiscall, PropertyList_AddHead, 0x00407BD0, void* list, const re::Ptr32<NiRefObject>* element);
GAME_FN(void, __thiscall, AVObject_Update, 0x0075DB00, void* self, float time, int updateControllers);
GAME_FN(void, __thiscall, AVObject_UpdateProperties, 0x0075E040, void* self);
GAME_FN(void, __thiscall, AVObject_UpdateEffects, 0x0075E240, void* self);
GAME_FN(void, __thiscall, Bound_ComputeFromData, 0x007D2890, NiBound* self, uint32_t count, const NiPoint3* points);
// (this file's, called where modes apply)
GAME_FN(NiRefObject*, __cdecl, ScreenText_GetFontPixelData, 0x0082F180);
GAME_FN(int32_t, __thiscall, ObjectArray_Remove, 0x0082F200, void* array, const re::Ptr32<ni::NiRefObject>* value);
GAME_FN(void*, __thiscall, PrimitiveArray_Construct, 0x0082F290, void* self, uint32_t maxSize, uint32_t growBy);
GAME_FN(void, __thiscall, VisualTracker_Draw, 0x0082F2F0, NiVisualTracker* self);
GAME_FN(void, __thiscall, Graph_Update, 0x0082F3C0, GraphData* self, float time);
GAME_FN(NiScreenText*, __thiscall, ScreenText_Construct, 0x0082F560, NiScreenText* self, uint32_t maxChars,
        NiTArray<NiRefObject*>* screenTextures, const float* color);
GAME_FN(void, __thiscall, ScreenText_RecreateText, 0x0082F690, NiScreenText* self);
GAME_FN(NiRefObject*, __cdecl, CreateGraphLines, 0x0082F800, uint32_t count, const float* color, const float* rect);
GAME_FN(void, __thiscall, ScreenText_Destruct, 0x0082FA40, NiScreenText* self);
GAME_FN(void, __thiscall, VisualTracker_Destruct, 0x008306C0, NiVisualTracker* self);
GAME_FN(GraphData*, __thiscall, Graph_Construct, 0x00830880, void* self, GraphCallback* callback, const char* name,
        const float* color, uint32_t numSamples, float maxValue, float interval, uint32_t show, const float* rect,
        NiNode* parent, int32_t* x, int32_t* y, NiTArray<NiRefObject*>* screenTextures);

inline float Abs(float v) {
    return v < 0.0f ? -v : v;   // (fabs: -0 and NaN only differ in the sign bit, which nothing reads)
}

// a reference let go (DecRefCount: DeleteThis, slot 1, on the last)
inline void Unref(NiRefObject* object) {
    if (object != nullptr && sys::Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

// NiPointer::operator= as this code inlines it: the old one let go first, then the new one held
template <typename T>
inline void AssignRef(re::Ptr32<T>& slot, std::type_identity_t<T>* value) {
    T* old = slot;
    if (old != value) {
        Unref(old);
        slot = value;
        if (value != nullptr) {
            sys::Increment(&value->refCount);
        }
    }
}

// the unsigned conversions the code makes (fild, 2^32 added when negative)
inline float UnsignedToFloat(uint32_t v) {
    return float(double(v));
}

// new NiScreenText(maxChars, array, color): the caller zeroes the reference and the colour first
NiScreenText* NewScreenText(uint32_t maxChars, NiTArray<NiRefObject*>* screenTextures, const float* color) {
    NiScreenText* text = static_cast<NiScreenText*>(NiNew(0x40));
    if (text == nullptr) {
        return nullptr;
    }
    text->screenTexture = nullptr;
    text->color[0] = 0.0f;
    text->color[1] = 0.0f;
    text->color[2] = 0.0f;
    text->color[3] = 0.0f;
    ScreenText_Construct(text, maxChars, screenTextures, color);
    return text;
}

// NiScreenText::SetPosition (inlined): remade when it moves
inline void SetTextPosition(NiScreenText* text, int32_t x, int32_t y) {
    if (text->x != x || text->y != y) {
        text->x = x;
        text->y = y;
        ScreenText_RecreateText(text);
    }
}

// NiScreenText::Display (inlined): its screen texture added to the owner's
inline void DisplayText(NiScreenText* text) {
    ObjectArray_AddFirstEmpty(text->screenTextures, &text->screenTexture);
}

// NiScreenText::SetString (inlined): cut to fit, the count with the terminator, remade
inline void SetTextString(NiScreenText* text, const char* string) {
    uint32_t length = 0;
    while (string[length] != '\0') {
        length++;
    }
    uint32_t maxChars = text->maxChars;
    if (length >= maxChars) {
        length = maxChars - 1;
    }
    text->numChars = length + 1;
    crt::StrncpyS(text->text, maxChars, string, length);
    ScreenText_RecreateText(text);
}

// ---- NiVisualTracker

// 0x82F170: SetShow(show)
void __fastcall VisualTracker_SetShow(NiVisualTracker* self, re::Edx, int show) {
    self->show = uint8_t(show);
}

// 0x82F180: NiScreenText's font pixels (made once: 256x128 RGBA from the built-in bitmap)
NiRefObject* __cdecl GetFontPixelData() {
    if (NiScreenText_ms_pkFontPixelData != nullptr) {
        return NiScreenText_ms_pkFontPixelData;
    }
    void* memory = NiNew(0x70);
    NiRefObject* data =
        memory != nullptr ? PixelData_Construct(memory, 0x100, 0x80, &kFontPixelFormat, 1, 1) : nullptr;
    NiScreenText_ms_pkFontPixelData = data;
    const uint32_t* offsets = At<uint32_t*>(data, 0x5C);
    uint8_t* pixels = At<uint8_t*>(data, 0x50) + offsets[0];
    for (uint32_t i = 0; i < offsets[1] - offsets[0]; i++) {
        pixels[i] = kFontPixels[i];
        offsets = At<uint32_t*>(NiScreenText_ms_pkFontPixelData, 0x5C);   // (re-read each pass)
    }
    return NiScreenText_ms_pkFontPixelData;
}

// 0x82F200: NiTObjectArray<NiPointer<T>>::Remove(element): its slot let go and emptied, the
// counts dropped; the index, or -1 (none given or not there)
int32_t __fastcall ObjectArray_Remove_(NiTArray<NiRefObject*>* self, re::Edx, const re::Ptr32<NiRefObject>* value) {
    NiRefObject* element = *value;
    if (element != nullptr) {
        for (uint16_t i = 0; i < self->size; i++) {
            if (self->base[i] == element) {
                uint32_t index = i;
                Unref(self->base[index]);
                self->base[index] = nullptr;
                uint16_t size = self->size;
                self->esize--;
                if (index == uint32_t(size) - 1) {
                    self->size = uint16_t(size - 1);
                }
                return int32_t(index);
            }
        }
    }
    return -1;
}

// 0x82F290: NiTPrimitiveArray<T>(maxSize, growBy)
void* __fastcall PrimitiveArray_Construct_(NiTArray<void*>* self, re::Edx, uint32_t maxSize, uint32_t growBy) {
    self->growBy = uint16_t(growBy);
    self->vtable = reinterpret_cast<void*>(NiTPrimitiveArray_vtable);
    self->maxSize = uint16_t(maxSize);
    self->size = 0;
    self->esize = 0;
    self->base = uint16_t(maxSize) != 0 ? static_cast<re::Ptr32<void>*>(NiMalloc(uint32_t(uint16_t(maxSize)) * 4)) : nullptr;
    self->vtable = reinterpret_cast<void*>(NiTPrimitiveArray_vtable);
    return self;
}

// 0x82F2F0: Draw (when shown): the screen-space camera (inside a frame), the box, the shown
// graphs, the texts
void __fastcall VisualTracker_Draw_(NiVisualTracker* self, re::Edx) {
    if (!self->show) {
        return;
    }
    void* renderer = NiRenderer_ms_pkRenderer;
    uint32_t state = At<uint32_t>(renderer, 0x94);
    if ((state == 1 || state == 2 || state == 4) && (At<uint8_t>(renderer, 0x90) == 1 || state == 4)) {
        Virtual<void>(renderer, 87, static_cast<void*>(nullptr));   // SetScreenSpaceCameraData
        renderer = NiRenderer_ms_pkRenderer;
    }
    Virtual<void>(self->lines, 34, renderer);   // RenderImmediate
    for (uint32_t i = 0; i < self->graphs.size; i++) {
        GraphData* graph = self->graphs.base[i];
        if (graph->show) {
            Virtual<void>(graph->lines, 34, NiRenderer_ms_pkRenderer);
        }
    }
    for (uint32_t i = 0; i < self->screenTextures.size; i++) {
        ScreenTexture_Draw(self->screenTextures.base[i], NiRenderer_ms_pkRenderer);
    }
}

// 0x82F3C0: GraphData::Update(time): a sample when the interval's up (or none yet): the strip
// shifted one left, the new value (clamped) at the right end, the bound remade
void __fastcall Graph_Update_(GraphData* self, re::Edx, float time) {
    if (!(double(self->lastTime) == kNoSampleDouble)) {
        float elapsed = time - self->lastTime;
        if (self->interval > elapsed) {
            return;
        }
    }
    float value = Virtual<float>(self->callback, 1, time);   // TakeSample
    if (self->maxValue < value) {
        value = self->maxValue;
    }
    float width = Abs(self->rect[0] - self->rect[1]);
    float height = Abs(self->rect[2] - self->rect[3]);
    void* data = At<void*>(self->lines, 0xCC);
    if (data != nullptr) {
        NiPoint3* vertices = At<NiPoint3*>(data, 0x20);
        uint8_t* connections = At<uint8_t*>(data, 0x40);
        if (vertices != nullptr) {
            if (self->numSamples - 1 != 0) {
                uint32_t i = 0;
                do {
                    vertices[i] = vertices[i + 1];
                    vertices[i].x = UnsignedToFloat(i) / UnsignedToFloat(self->numSamples) * width;
                    i++;
                } while (i < self->numSamples - 1);
            }
            vertices[self->lastIndex].x = width;
            vertices[self->lastIndex].y = height - value / self->maxValue * height;
            vertices[self->lastIndex].z = 0.0f;
            connections[self->lastIndex] = 0;
            At<uint16_t>(data, 0x34) |= 1;   // the vertices changed
            Bound_ComputeFromData(&At<NiBound>(data, 0x10), self->numSamples, vertices);
        }
    }
    self->lastTime = time;
}

// ---- NiScreenText

// 0x82F560: NiScreenText(maxChars, screen textures, colour): the font texture made with the
// first; its own screen texture over it (modulated)
NiScreenText* __fastcall ScreenText_Construct_(NiScreenText* self, re::Edx, uint32_t maxChars,
                                               NiTArray<NiRefObject*>* screenTextures, const float* color) {
    NiScreenText_ms_iCount += 1;
    if (NiScreenText_ms_spTexture == nullptr) {
        NiRefObject* texture = SourceTexture_Create(ScreenText_GetFontPixelData(), &kFontTexturePrefs);
        AssignRef(NiScreenText_ms_spTexture, texture);
    }
    self->maxChars = maxChars;
    self->color[0] = color[0];
    self->color[1] = color[1];
    self->color[2] = color[2];
    self->color[3] = color[3];
    void* memory = NiNew(0x20);
    NiRefObject* screen = memory != nullptr ? ScreenTexture_Construct(memory, NiScreenText_ms_spTexture) : nullptr;
    AssignRef(self->screenTexture, screen);
    void* texturing = At<void*>(self->screenTexture, 0x14);   // its NiTexturingProperty
    uint16_t& flags = At<uint16_t>(texturing, 0x18);
    flags = uint16_t((flags & 0xFFF5) | 4);                    // APPLY_MODULATE
    self->screenTextures = screenTextures;
    self->x = 0;
    self->y = 0;
    self->scrollDown = 1;
    char* text = static_cast<char*>(NiMalloc(self->maxChars));
    self->numChars = 0;
    self->unknown30 = 0;
    self->row = 0;
    self->column = 0;
    self->text = text;
    self->maxCols = 0x28;
    return self;
}

// 0x82F690: RecreateText: a rect per printable character ('!'..'z'), wrapped at maxCols and at
// '\n' (each new line moving the earlier rects up when not scrolling down), the texture marked
void __fastcall ScreenText_RecreateText_(NiScreenText* self, re::Edx) {
    ScreenTexture_RemoveAllScreenRects(self->screenTexture);
    self->row = 0;
    self->column = 0;
    for (uint32_t i = 0; i < self->numChars; i++) {
        char c = self->text[i];
        if (self->column >= self->maxCols || c == '\n') {
            self->row++;
            self->column = 0;
            if (!self->scrollDown) {
                uint32_t count = At<uint32_t>(self->screenTexture, 0x10);
                for (uint32_t j = 0; j < count; j++) {
                    uint8_t* rects = At<uint8_t*>(self->screenTexture, 8);   // 0x1C each, the top first
                    uint16_t& top = *reinterpret_cast<uint16_t*>(rects + j * 0x1C);
                    top = uint16_t(top - uint16_t(NiScreenText_ms_uiCharHeight));
                }
            }
            if (c == '\n') {
                continue;
            }
        }
        uint32_t glyph = uint32_t(int32_t(c) - 0x21);
        if (glyph <= 0x59) {
            uint32_t charHeight = NiScreenText_ms_uiCharHeight;
            uint16_t top = uint16_t(self->y);
            if (self->scrollDown) {
                top = uint16_t(top + uint16_t(uint16_t(self->row) * uint16_t(charHeight)));
            }
            uint32_t row = glyph / NiScreenText_ms_uiCharsPerRow;
            uint32_t column = glyph % NiScreenText_ms_uiCharsPerRow;
            uint32_t charWidth = NiScreenText_ms_uiCharWidth;
            uint16_t left = uint16_t(uint16_t(uint16_t(self->column) * uint16_t(charWidth)) + uint16_t(self->x));
            ScreenTexture_AddScreenRect(self->screenTexture, top, left, NiScreenText_ms_usCharWidth,
                                        NiScreenText_ms_usCharHeight, row * charHeight, column * charWidth,
                                        self->color[0], self->color[1], self->color[2], self->color[3]);
        }
        self->column++;
    }
    At<uint16_t>(self->screenTexture, 0x18) |= 0xF;   // everything changed
}

// 0x82F7B0: NiVisualTracker::Update: every graph sampled at the time now, the node updated
void __fastcall VisualTracker_Update(NiVisualTracker* self, re::Edx) {
    float time = NiGetCurrentTimeInSec();
    for (uint32_t i = 0; i < self->graphs.size; i++) {
        Graph_Update(self->graphs.base[i], time);
    }
    AVObject_Update(self->node, time, 1);
}

// 0x82F800: the graph lines (a guess at the name): count vertices at (0, height), all
// connected, at the rect's corner, in the colour (emissive), the z-buffer off
NiRefObject* __cdecl CreateGraphLines_(uint32_t count, const float* color, const float* rect) {
    float height = Abs(rect[2] - rect[3]);
    uint8_t* connections = static_cast<uint8_t*>(NiMalloc(count));
    if (count > 0) {
        crt::Memset(connections, 1, count);
    }
    uint64_t bytes = uint64_t(count) * 0xC;
    NiPoint3* vertices = static_cast<NiPoint3*>(NiNewArray(bytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(bytes)));
    for (uint32_t i = 0; i < count; i++) {
        vertices[i].x = 0.0f;
        vertices[i].y = height;
        vertices[i].z = 0.0f;
    }
    void* memory = NiNew(0xEC);
    NiRefObject* lines = memory != nullptr ? Lines_Construct(memory, count, vertices, nullptr, nullptr, 0, 0, connections)
                                           : nullptr;
    At<uint16_t>(At<void*>(lines, 0xCC), 0x34) &= 0x0FFF;   // its data mutable
    NiAVObject* object = reinterpret_cast<NiAVObject*>(lines);
    object->local.translate.x = rect[0];
    object->local.translate.y = rect[2];
    object->local.translate.z = 0.0f;
    NiMatrix3 identity;
    Matrix3_MakeIdentity(&identity);
    object->local.rotate = identity;
    object->local.scale = Abs(1.0f);

    memory = NiNew(0x5C);
    NiRefObject* material = memory != nullptr ? static_cast<NiRefObject*>(MaterialProperty_Construct(memory)) : nullptr;
    At<float>(material, 0x40) = color[0];   // the emissive colour
    At<float>(material, 0x44) = color[1];
    At<float>(material, 0x48) = color[2];
    At<uint32_t>(material, 0x54) += 1;      // its revision
    re::Ptr32<NiRefObject> held = material;           // (the NiPointer AttachProperty takes)
    sys::Increment(&material->refCount);
    PropertyList_AddHead(&object->properties, &held);
    if (sys::Decrement(&material->refCount) == 0) {
        Virtual<void>(material, 1);
    }

    // NiZBufferProperty (its constructor inlined, 0x407B40): test and write on, then both off
    memory = NiNew(0x30);
    NiRefObject* zbuffer = nullptr;
    if (memory != nullptr) {
        NiProperty_Construct(memory);
        At<float>(memory, 0x1C) = 0.0f;
        At<uint32_t>(memory, 0) = NiZBufferProperty_vtable;
        At<float>(memory, 0x20) = 0.0f;
        At<uint32_t>(memory, 0x24) = 0;
        At<uint16_t>(memory, 0x18) = 0xF;
        At<float>(memory, 0x28) = 1.0f;
        At<float>(memory, 0x2C) = 1.0f;
        zbuffer = static_cast<NiRefObject*>(memory);
    }
    At<uint16_t>(zbuffer, 0x18) &= 0xFFFC;
    held = zbuffer;
    sys::Increment(&zbuffer->refCount);
    PropertyList_AddHead(&object->properties, &held);
    if (sys::Decrement(&zbuffer->refCount) == 0) {
        Virtual<void>(zbuffer, 1);
    }

    AVObject_Update(lines, 0.0f, 1);
    AVObject_UpdateProperties(lines);
    AVObject_UpdateEffects(lines);
    Virtual<void>(lines, 19);   // UpdateNodeBound
    return lines;
}

// 0x82FA40: ~NiScreenText: its screen texture taken off the owner's (compacted), the font
// texture let go with the last one, the string freed
void __fastcall ScreenText_Destruct_(NiScreenText* self, re::Edx) {
    ObjectArray_Remove(self->screenTextures, &self->screenTexture);
    PointerArray_Compact(self->screenTextures);
    int32_t count = NiScreenText_ms_iCount - 1;
    NiScreenText_ms_iCount = count;
    if (count < 1 && NiScreenText_ms_spTexture != nullptr) {
        Unref(NiScreenText_ms_spTexture);
        NiScreenText_ms_spTexture = nullptr;
    }
    NiFree(self->text);
    Unref(self->screenTexture);
}

// 0x82FAD0: NiVisualTracker(max, decimals, left, right, top, bottom, name, show, graphs): the
// box (the graphs' area under the name row, 90% of the width, the labels in the rest) with its
// quarter lines, the name centred over it, the five labels down the right ("%.<decimals>f" of
// max, 3/4, 1/2, 1/4 and 0), where the graphs' names will go (the bottom left)
NiVisualTracker* __fastcall VisualTracker_Construct(NiVisualTracker* self, re::Edx, float maxValue, uint32_t decimals,
                                                    float left, float right, float top, float bottom,
                                                    const char* name, uint32_t show, uint32_t numGraphs) {
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    self->refCount = 0;
    sys::Increment(&NiRefObject_ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(NiVisualTracker_vtable);
    PrimitiveArray_Construct(&self->graphs, numGraphs, 1);
    self->rect[0] = left;
    self->rect[1] = right;
    self->rect[2] = top;
    self->rect[3] = bottom;
    self->graphRect[0] = 0.0f;
    self->graphRect[1] = 0.0f;
    self->graphRect[2] = 0.0f;
    self->graphRect[3] = 0.0f;
    self->node = nullptr;
    self->lines = nullptr;
    self->maxValue = maxValue;
    self->show = uint8_t(show);
    NiTArray<NiRefObject*>& textures = self->screenTextures;   // (NiTObjectArray's constructor inlined)
    uint16_t maxSize = uint16_t(numGraphs + 1);
    textures.vtable = reinterpret_cast<void*>(NiTObjectArray_vtable);
    textures.maxSize = maxSize;
    textures.growBy = 1;
    textures.size = 0;
    textures.esize = 0;
    textures.base = maxSize != 0 ? NiNewPointerArray(maxSize) : nullptr;
    textures.vtable = reinterpret_cast<void*>(NiTObjectArray_vtable);
    NiSprintf(self->name, 0xFF, "%s", name);
    self->name[0xFF] = 0;

    void* buffer = Virtual<void*>(NiRenderer_ms_pkRenderer, 32);   // the default back buffer
    uint32_t width = At<uint32_t>(buffer, 8);
    uint32_t height = At<uint32_t>(buffer, 0xC);
    self->nameText = NewScreenText(0x20, &self->screenTextures, kWhite);
    void* memory = NiNew(0xF4);
    NiNode* node = memory != nullptr ? Node_Construct(memory, numGraphs) : nullptr;
    AssignRef(self->node, node);
    self->node->local.translate.x = self->rect[0];
    self->node->local.translate.y = self->rect[2];
    self->node->local.translate.z = kTrackerDepth;

    float trackerWidth = Abs(self->rect[0] - self->rect[1]);
    float widthF = UnsignedToFloat(width);
    float charWidth = UnsignedToFloat(uint32_t(int32_t(int16_t(NiScreenText_ms_uiCharWidth)))) / widthF;
    float heightF = UnsignedToFloat(height);
    float charHeight = float(double(int32_t(int16_t(NiScreenText_ms_uiCharHeight)))) / heightF;
    self->graphRect[0] = 0.0f;
    float graphWidth = trackerWidth - trackerWidth * 0.1f;   // (0x900128: 0.1f as a double)
    self->graphRect[1] = graphWidth;
    self->graphRect[2] = charHeight;
    float graphBottom = Abs(self->rect[2] - self->rect[3]) - charHeight;
    self->graphRect[3] = graphBottom;
    float boxWidth = Abs(0.0f - graphWidth);
    float boxHeight = Abs(charHeight - graphBottom);

    NiRefObject* lines = CreateGraphLines(11, kBoxColor, self->graphRect);
    AssignRef(self->lines, lines);
    void* data = At<void*>(self->lines, 0xCC);
    if (data != nullptr) {
        NiPoint3* v = At<NiPoint3*>(data, 0x20);
        uint8_t* connect = At<uint8_t*>(data, 0x40);
        if (v != nullptr) {
            // the box (0..4, closed) and the 3/4, 1/2 and 1/4 lines across it
            v[0] = {0.0f, 0.0f, 0.0f};
            connect[0] = 1;
            v[1] = {0.0f, boxHeight, 0.0f};
            connect[1] = 1;
            v[2] = {boxWidth, boxHeight, 0.0f};
            connect[2] = 1;
            v[3] = {boxWidth, 0.0f, 0.0f};
            connect[3] = 1;
            float threeQuarters = boxHeight * 0.75f;
            v[4] = {0.0f, 0.0f, 0.0f};
            connect[4] = 0;
            v[5] = {0.0f, threeQuarters, 0.0f};
            connect[5] = 1;
            v[6] = {boxWidth, threeQuarters, 0.0f};
            float half = boxHeight * 0.5f;
            connect[6] = 0;
            v[7] = {0.0f, half, 0.0f};
            connect[7] = 1;
            v[8] = {boxWidth, half, 0.0f};
            float quarter = boxHeight * 0.25f;
            connect[8] = 0;
            v[9] = {0.0f, quarter, 0.0f};
            connect[9] = 1;
            v[10] = {boxWidth, quarter, 0.0f};
            connect[10] = 0;
        }
        At<uint16_t>(data, 0x34) |= 1;   // the vertices changed
        Bound_ComputeFromData(&At<NiBound>(data, 0x10), 11, v);   // (as the game: null vertices too)
    }
    Virtual<void>(self->node, 34, self->lines, 0);   // AttachChild(lines, false)

    // the name, centred over the box (at the left when it doesn't fit)
    uint32_t length = 0;
    while (self->name[length] != '\0') {
        length++;
    }
    float nameX = boxWidth - UnsignedToFloat(length) * charWidth;
    if (1.0f < nameX) {
        nameX = 0.0f;
    }
    float halfX = nameX * 0.5f;
    int32_t textX = crt::FloatToInt(double((halfX + self->rect[0]) * widthF));
    int32_t textY = crt::FloatToInt(double(heightF * self->rect[2]));
    SetTextPosition(self->nameText, textX, textY);
    DisplayText(self->nameText);
    SetTextString(self->nameText, self->name);

    // the labels, from the top down a quarter of the box apart
    int32_t labelX = crt::FloatToInt(double(((self->graphRect[1] + self->rect[0]) + 0.01f) * widthF));
    int32_t labelY = crt::FloatToInt(double(((self->graphRect[2] + self->rect[2]) - charHeight * 0.5f) * heightF));
    int32_t step = crt::FloatToInt(double(boxHeight * 0.25f * heightF));
    char format[0x80];
    NiSprintf(format, 0x7F, "%%.%df", decimals);
    char label[0x10];
    NiSprintf(label, 0xE, format, double(self->maxValue));
    label[0xF] = 0;
    NiScreenText* text = NewScreenText(0xF, &self->screenTextures, kWhite);
    self->labels[4] = text;
    SetTextPosition(text, labelX, labelY);
    DisplayText(self->labels[4]);
    SetTextString(self->labels[4], label);

    const float fractions[3] = {0.75f, 0.5f, 0.25f};
    for (int i = 0; i < 3; i++) {
        labelY += step;
        NiSprintf(label, 0xE, format, double(self->maxValue * fractions[i]));
        label[0xF] = 0;
        text = NewScreenText(0xF, &self->screenTextures, kWhite);
        self->labels[3 - i] = text;
        SetTextPosition(text, labelX, labelY);
        DisplayText(self->labels[3 - i]);
        SetTextString(self->labels[3 - i], label);
    }
    labelY += step;
    NiSprintf(label, 0xE, format, 0.0);
    label[0xF] = 0;
    text = NewScreenText(0x20, &self->screenTextures, kWhite);   // (0x20 for this one: the game's)
    self->labels[0] = text;
    SetTextPosition(text, labelX, labelY);
    DisplayText(self->labels[0]);
    SetTextString(self->labels[0], label);

    self->nextTextX = crt::FloatToInt(double(widthF * self->rect[0]));
    self->nextTextY = crt::FloatToInt(double((self->rect[3] - charHeight) * heightF));
    return self;
}

// 0x8306C0: ~NiVisualTracker: the box let go, every graph (its callback, its name, its lines)
// and every text deleted, then the arrays, the node and NiRefObject's count
void __fastcall VisualTracker_Destruct_(NiVisualTracker* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiVisualTracker_vtable);
    if (self->lines != nullptr) {
        Unref(self->lines);
        self->lines = nullptr;
    }
    for (uint32_t i = 0; i < self->graphs.size; i++) {
        GraphData* graph = self->graphs.base[i];
        if (graph == nullptr) {
            continue;
        }
        if (graph->callback != nullptr) {
            Virtual<void*>(graph->callback, 0, 1u);   // the deleting destructor
        }
        NiScreenText* text = graph->text;
        if (text != nullptr) {
            ScreenText_Destruct(text);
            NiDelete(text, 0x40);
        }
        Unref(graph->lines);
        NiDelete(graph, 0x140);
    }
    for (int i = 0; i < 6; i++) {   // the name, then the labels (+0x148..+0x15C in order)
        NiScreenText* text = i == 0 ? self->nameText : self->labels[i - 1];
        if (text != nullptr) {
            ScreenText_Destruct(text);
            NiDelete(text, 0x40);
        }
    }
    re::Ptr32<NiRefObject>* base = self->screenTextures.base;
    self->screenTextures.vtable = reinterpret_cast<void*>(NiTObjectArray_vtable);
    Array_Free(base);
    Unref(self->lines);
    Unref(self->node);
    base = re::PtrCast<re::Ptr32<NiRefObject>*>(self->graphs.base);
    self->graphs.vtable = reinterpret_cast<void*>(NiTPrimitiveArray_vtable);
    NiFree(base);
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    sys::Decrement(&NiRefObject_ms_uiObjects);
}

// 0x830880: GraphData(callback, name, colour, samples, max, interval, show, rect, parent, &x,
// &y, screen textures): its lines under the parent, its name (in its colour) at (x, y), x moved
// past it
GraphData* __fastcall Graph_Construct_(GraphData* self, re::Edx, GraphCallback* callback, const char* name,
                                       const float* color, uint32_t numSamples, float maxValue, float interval,
                                       uint32_t show, const float* rect, NiNode* parent, int32_t* x, int32_t* y,
                                       NiTArray<NiRefObject*>* screenTextures) {
    self->lines = nullptr;
    self->color[0] = 0.0f;
    self->color[1] = 0.0f;
    self->color[2] = 0.0f;
    self->rect[0] = 0.0f;
    self->rect[1] = 0.0f;
    self->rect[2] = 0.0f;
    self->rect[3] = 0.0f;
    self->show = uint8_t(show);
    self->callback = callback;
    self->interval = interval;
    self->numSamples = numSamples;
    self->rect[0] = rect[0];
    self->rect[1] = rect[1];
    self->rect[2] = rect[2];
    self->rect[3] = rect[3];
    NiRefObject* lines = CreateGraphLines(numSamples, color, rect);
    AssignRef(self->lines, lines);
    Virtual<void>(parent, 34, self->lines, 0);   // AttachChild(lines, false)
    self->color[0] = color[0];
    self->color[1] = color[1];
    self->color[2] = color[2];
    self->lastTime = kNoSample;
    NiSprintf(self->name, 0xFF, "%s", name);
    self->maxValue = maxValue;
    const float textColor[4] = {self->color[0], self->color[1], self->color[2], 1.0f};
    self->name[0xFF] = 0;
    self->lastIndex = self->numSamples - 1;
    NiScreenText* text = NewScreenText(0xF, screenTextures, textColor);
    self->text = text;
    SetTextPosition(text, *x, *y);
    DisplayText(self->text);
    SetTextString(self->text, self->name);
    *x += int32_t(int16_t(uint16_t(uint16_t(self->text->numChars) * uint16_t(NiScreenText_ms_uiCharWidth))));
    return self;
}

// 0x830A90: NiVisualTracker's deleting destructor
NiVisualTracker* __fastcall VisualTracker_DeletingDestructor(NiVisualTracker* self, re::Edx, uint32_t flags) {
    VisualTracker_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x178);
    }
    return self;
}

// 0x830AC0: AddGraph(callback, name, colour, samples, interval, show): the index (the graph's
// slot; a failed allocation added as none)
uint32_t __fastcall VisualTracker_AddGraph(NiVisualTracker* self, re::Edx, GraphCallback* callback, const char* name,
                                           const float* color, uint32_t numSamples, float interval, uint32_t show) {
    void* memory = NiNew(0x140);
    re::Ptr32<GraphData> graph = memory != nullptr
                           ? Graph_Construct(memory, callback, name, color, numSamples, self->maxValue, interval, show,
                                             self->graphRect, self->node, &self->nextTextX, &self->nextTextY,
                                             &self->screenTextures)
                           : nullptr;
    return GraphArray_AddFirstEmpty(&self->graphs, &graph);
}

// ---- the graph callbacks

// 0x830B50: FrameRateUpdate::TakeSample: the rate (0 without one)
float __fastcall FrameRateUpdate_TakeSample(FrameRateUpdate* self, re::Edx, float) {
    return self->frameRate != nullptr ? *self->frameRate : 0.0f;
}

// 0x830B70: GenericFloatUpdate::TakeSample: the value over the scale (0 without one)
float __fastcall GenericFloatUpdate_TakeSample(GenericFloatUpdate* self, re::Edx, float) {
    if (self->value == nullptr) {
        return 0.0f;
    }
    return *self->value / self->scale;
}

// 0x830B90: GenericUnsignedUpdate::TakeSample: the value over the scale (0 without one)
float __fastcall GenericUnsignedUpdate_TakeSample(GenericUnsignedUpdate* self, re::Edx, float) {
    if (self->value == nullptr) {
        return 0.0f;
    }
    return UnsignedToFloat(*self->value) / self->scale;
}

// 0x830BC0
FrameRateUpdate* __fastcall FrameRateUpdate_Construct(FrameRateUpdate* self, re::Edx, const float* frameRate) {
    self->vtable = reinterpret_cast<const void*>(FrameRateUpdate_vtable);
    self->frameRate = frameRate;
    return self;
}

// 0x830BE0
VisibleArraySize* __fastcall VisibleArraySize_Construct(VisibleArraySize* self, re::Edx, const void* visible) {
    self->vtable = reinterpret_cast<const void*>(VisibleArraySize_vtable);
    self->visible = visible;
    return self;
}

// 0x830C00: VisibleArraySize::TakeSample: the visible array's count
float __fastcall VisibleArraySize_TakeSample(VisibleArraySize* self, re::Edx, float) {
    return UnsignedToFloat(At<uint32_t>(self->visible, 4));
}

// 0x830C20
GenericFloatUpdate* __fastcall GenericFloatUpdate_Construct(GenericFloatUpdate* self, re::Edx, float scale,
                                                            const float* value) {
    self->scale = scale;
    self->vtable = reinterpret_cast<const void*>(GenericFloatUpdate_vtable);
    self->value = value;
    return self;
}

// 0x830C40
GenericUnsignedUpdate* __fastcall GenericUnsignedUpdate_Construct(GenericUnsignedUpdate* self, re::Edx, float scale,
                                                                  const uint32_t* value) {
    self->scale = scale;
    self->vtable = reinterpret_cast<const void*>(GenericUnsignedUpdate_vtable);
    self->value = value;
    return self;
}

// 0x830C60: the 8-byte callbacks' deleting destructor (FrameRateUpdate, VisibleArraySize)
GraphCallback* __fastcall SmallCallback_DeletingDestructor(GraphCallback* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<const void*>(GraphCallbackObject_vtable);
    if ((flags & 1) != 0) {
        NiDelete(self, 8);
    }
    return self;
}

// 0x830C90: the 0xC-byte callbacks' (GenericFloatUpdate, GenericUnsignedUpdate)
GraphCallback* __fastcall ScaledCallback_DeletingDestructor(GraphCallback* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<const void*>(GraphCallbackObject_vtable);
    if ((flags & 1) != 0) {
        NiDelete(self, 0xC);
    }
    return self;
}

// 0x830CC0: NiVisualTrackerRenderClick::PerformRendering(frame) (slot 6): the screen-space
// camera on its viewport (inside a frame), every tracker drawn; the count drawn and the time
void __fastcall VisualTrackerRenderClick_PerformRendering(void* self, re::Edx, uint32_t) {
    void* renderer = NiRenderer_ms_pkRenderer;
    uint32_t state = At<uint32_t>(renderer, 0x94);
    if ((state == 1 || state == 2 || state == 4) && (At<uint8_t>(renderer, 0x90) == 1 || state == 4)) {
        Virtual<void>(renderer, 87, static_cast<uint8_t*>(self) + 0x14);   // SetScreenSpaceCameraData
    }
    float start = NiGetCurrentTimeInSec();
    for (NiTListNode<NiVisualTracker*>* item = At<NiTListNode<NiVisualTracker*>*>(self, 0x50); item != nullptr;) {
        NiVisualTracker* tracker = item->element;
        item = item->next;
        VisualTracker_Draw(tracker);
    }
    float elapsed = NiGetCurrentTimeInSec() - start;
    At<uint32_t>(self, 0x5C) = At<uint32_t>(self, 0x58);   // the trackers' count
    At<float>(self, 0x60) = elapsed;
}

}   // namespace
}   // namespace ni::app

// ---- registration ---------------------------------------------------------------------

#define C33_VISTRACKER_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

C33_VISTRACKER_REVERSED("NiVisualTracker::SetShow", 0x0082F170, &ni::app::VisualTracker_SetShow);
C33_VISTRACKER_REVERSED("NiScreenText::GetFontPixelData", 0x0082F180, &ni::app::GetFontPixelData);
C33_VISTRACKER_REVERSED("NiTObjectArray<NiScreenTexturePtr>::Remove", 0x0082F200, &ni::app::ObjectArray_Remove_);
C33_VISTRACKER_REVERSED("NiTPrimitiveArray<T>::NiTPrimitiveArray", 0x0082F290, &ni::app::PrimitiveArray_Construct_);
C33_VISTRACKER_REVERSED("NiVisualTracker::Draw", 0x0082F2F0, &ni::app::VisualTracker_Draw_);
C33_VISTRACKER_REVERSED("NiVisualTracker::GraphData::Update", 0x0082F3C0, &ni::app::Graph_Update_);
C33_VISTRACKER_REVERSED("NiScreenText::NiScreenText", 0x0082F560, &ni::app::ScreenText_Construct_);
C33_VISTRACKER_REVERSED("NiScreenText::RecreateText", 0x0082F690, &ni::app::ScreenText_RecreateText_);
C33_VISTRACKER_REVERSED("NiVisualTracker::Update", 0x0082F7B0, &ni::app::VisualTracker_Update);
C33_VISTRACKER_REVERSED("NiVisualTracker::CreateGraphLines", 0x0082F800, &ni::app::CreateGraphLines_);
C33_VISTRACKER_REVERSED("NiScreenText::~NiScreenText", 0x0082FA40, &ni::app::ScreenText_Destruct_);
C33_VISTRACKER_REVERSED("NiVisualTracker::NiVisualTracker", 0x0082FAD0, &ni::app::VisualTracker_Construct);
C33_VISTRACKER_REVERSED("NiVisualTracker::~NiVisualTracker", 0x008306C0, &ni::app::VisualTracker_Destruct_);
C33_VISTRACKER_REVERSED("NiVisualTracker::GraphData::GraphData", 0x00830880, &ni::app::Graph_Construct_);
C33_VISTRACKER_REVERSED("NiVisualTracker::`scalar deleting destructor'", 0x00830A90,
                        &ni::app::VisualTracker_DeletingDestructor);
C33_VISTRACKER_REVERSED("NiVisualTracker::AddGraph", 0x00830AC0, &ni::app::VisualTracker_AddGraph);
C33_VISTRACKER_REVERSED("FrameRateUpdate::TakeSample", 0x00830B50, &ni::app::FrameRateUpdate_TakeSample);
C33_VISTRACKER_REVERSED("GenericFloatUpdate::TakeSample", 0x00830B70, &ni::app::GenericFloatUpdate_TakeSample);
C33_VISTRACKER_REVERSED("GenericUnsignedUpdate::TakeSample", 0x00830B90, &ni::app::GenericUnsignedUpdate_TakeSample);
C33_VISTRACKER_REVERSED("FrameRateUpdate::FrameRateUpdate", 0x00830BC0, &ni::app::FrameRateUpdate_Construct);
C33_VISTRACKER_REVERSED("VisibleArraySize::VisibleArraySize", 0x00830BE0, &ni::app::VisibleArraySize_Construct);
C33_VISTRACKER_REVERSED("VisibleArraySize::TakeSample", 0x00830C00, &ni::app::VisibleArraySize_TakeSample);
C33_VISTRACKER_REVERSED("GenericFloatUpdate::GenericFloatUpdate", 0x00830C20, &ni::app::GenericFloatUpdate_Construct);
C33_VISTRACKER_REVERSED("GenericUnsignedUpdate::GenericUnsignedUpdate", 0x00830C40,
                        &ni::app::GenericUnsignedUpdate_Construct);
C33_VISTRACKER_REVERSED("FrameRateUpdate::`scalar deleting destructor'", 0x00830C60,
                        &ni::app::SmallCallback_DeletingDestructor);
C33_VISTRACKER_REVERSED("GenericFloatUpdate::`scalar deleting destructor'", 0x00830C90,
                        &ni::app::ScaledCallback_DeletingDestructor);
C33_VISTRACKER_REVERSED("NiVisualTrackerRenderClick::PerformRendering", 0x00830CC0,
                        &ni::app::VisualTrackerRenderClick_PerformRendering);
