// The sample framework's UI elements: NiUIBaseElement (an element's normalized and pixel
// rects, focus and visibility with their signals), NiUILabel (a text in an NiString2D, word
// wrapped to its rect and aligned), NiUIRenderGroup (elements drawn through one
// NiScreenElements object with the UI skin's texture, in the UI manager's render order),
// NiUIGroup (a render group with a title label, an expand button and keyboard / game pad focus
// moving through its children) and the screen-quad helpers they share (one quad, or nine for a
// skin frame, set from normalized rects and the skin atlas' texture rects). Layouts in
// NiSampleFramework.h. Float maths in float, in the game's order.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSampleFramework.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::sample {
namespace {

GAME_VAR(void*, NiRenderer_ms_pkRenderer, 0x020E2380);
GAME_FN(re::Ptr32<char>*, __thiscall, NiString_Construct, 0x007741A0, re::Ptr32<char>* self, const char* text);
GAME_FN(re::Ptr32<char>*, __thiscall, NiString_ConstructSized, 0x007740D0, re::Ptr32<char>* self, uint32_t size);
GAME_FN(void, __thiscall, NiString_Destruct, 0x00774240, re::Ptr32<char>* self);
GAME_FN(uint32_t, __thiscall, NiString_Length, 0x00773D10, const re::Ptr32<char>* self);
GAME_FN(char, __thiscall, NiString_GetAt, 0x00773D40, const re::Ptr32<char>* self, uint32_t index);
GAME_FN(void, __thiscall, NiString_SetAt, 0x00774280, re::Ptr32<char>* self, uint32_t index, char c);
GAME_FN(int, __thiscall, NiString_Find, 0x007743B0, const re::Ptr32<char>* self, char c, uint32_t start);
GAME_FN(int, __thiscall, NiString_FindReverse, 0x007743F0, const re::Ptr32<char>* self, char c);
GAME_FN(re::Ptr32<char>*, __thiscall, NiString_AssignText, 0x007745E0, re::Ptr32<char>* self, const char* text);
GAME_FN(void, __thiscall, NiString_RemoveRange, 0x00774630, re::Ptr32<char>* self, uint32_t start, uint32_t count);
GAME_FN(void, __thiscall, NiString_Replace, 0x007748B0, re::Ptr32<char>* self, const char* what, const char* with);
GAME_FN(void, __thiscall, NiString_Insert, 0x00774AE0, re::Ptr32<char>* self, const char* text, uint32_t index);
GAME_FN(NiRefObject*, __thiscall, String2D_Construct, 0x008D6D10, void* self, NiRefObject* font, uint32_t flags,
        uint32_t bufferSize, const char* text, const float* color, uint32_t x, uint32_t y);
GAME_FN(void, __thiscall, String2D_Draw, 0x008D6D80, NiRefObject* self, void* renderer);
GAME_FN(void, __thiscall, FontString_GetTextExtent, 0x008D6920, NiRefObject* self, float* width, float* height);
GAME_FN(void, __thiscall, FontString_SetText, 0x008D83B0, NiRefObject* self, const char* text, uint32_t flags);
GAME_FN(void, __thiscall, Font_GetTextExtent, 0x008D70B0, NiRefObject* self, const char* text, float* width,
        float* height, int direction);
GAME_FN(void, __thiscall, Font_GetCharExtent, 0x008D7240, NiRefObject* self, uint32_t c, float* width,
        float* height);

GAME_FN(int32_t, __thiscall, ScreenData_Insert, 0x00788AA0, void* self, uint32_t vertices, uint32_t triangles,
        const uint16_t* triangleList);
GAME_FN(uint8_t, __thiscall, ScreenData_SetRectangle, 0x00789060, void* self, int32_t polygon, float left, float top,
        float width, float height);
GAME_FN(uint8_t, __thiscall, ScreenData_SetColors, 0x00789160, void* self, int32_t polygon, const float* color);
GAME_FN(uint8_t, __thiscall, ScreenData_SetTextures, 0x00789300, void* self, int32_t polygon, uint32_t set,
        float left, float top, float right, float bottom);
GAME_FN(void, __thiscall, ScreenData_UpdateBound, 0x00788830, void* self);
GAME_FN(void*, __thiscall, ScreenData_Construct, 0x007893A0, void* self, int wantNormals, int wantColors,
        uint32_t textureSets, int maxPolygons, int polygonGrowBy, int maxVertices, int vertexGrowBy,
        int maxTriangles, int triangleGrowBy);
GAME_FN(NiAVObject*, __thiscall, ScreenElements_Construct, 0x007AB380, void* self, void* data);
// NiScreenElements' forwards to its data (+0xCC)
GAME_FN(uint8_t, __thiscall, ScreenElements_SetRectangle, 0x007559F0, void* self, int32_t polygon, float left,
        float top, float width, float height);
GAME_FN(uint8_t, __thiscall, ScreenElements_SetTextures, 0x00755A30, void* self, int32_t polygon, uint32_t set,
        float left, float top, float right, float bottom);
GAME_FN(void, __thiscall, Geometry_RenderImmediate, 0x007AB310, void* self, void* renderer);   // (slot 34)
GAME_FN(void, __thiscall, AVObject_AttachProperty, 0x004099F0, void* self, void* property);
GAME_FN(void, __thiscall, AVObject_UpdateProperties, 0x0075E040, void* self);
GAME_FN(void, __thiscall, AVObject_Update, 0x0075DB00, void* self, float time, int updateControllers);
GAME_FN(void*, __thiscall, TexturingProperty_ConstructWithTexture, 0x00765F30, void* self, void* texture);
GAME_FN(void, __thiscall, TexturingProperty_SetBaseFilterMode, 0x0082BFE0, void* self, uint32_t mode);
GAME_FN(void, __thiscall, ObjectNET_Construct, 0x00764550, void* self);
GAME_FN(void*, __thiscall, MaterialProperty_Construct, 0x00775660, void* self);
GAME_FN(void*, __thiscall, AlphaProperty_Construct, 0x00407800, void* self);
GAME_FN(uint32_t, __thiscall, ChildArray_AddFirstEmpty, 0x007606C0, void* self, const re::Ptr32<void>* element);
GAME_FN(void, __thiscall, ObjectSet_AddUnique, 0x00845B40, void* self, const re::Ptr32<void>* element);
GAME_FN(uint32_t*, __thiscall, UIElement_GetPixelRect, 0x00846800, NiUIBaseElement* self, uint32_t* result);
GAME_FN(void, __thiscall, ImageButton_Construct, 0x0083C730, void* self);
GAME_FN(float*, __thiscall, AtlasMap_GetBorder, 0x0083C4A0, void* self, float* result, uint32_t index, uint32_t mode);
GAME_FN(float*, __thiscall, AtlasMap_ToUV, 0x0083C270, void* self, float* result, const float* rect);
constexpr uint32_t kNiVertexColorPropertyVtable = 0x0092D0D4;

// this chunk's (called where modes apply)
GAME_FN(uint8_t, __cdecl, SetQuad, 0x008396F0, void* screenElements, int32_t quad, const float* rect,
        const float* uv, const float* color);
GAME_FN(uint8_t, __cdecl, HideQuad, 0x008397E0, void* screenElements, uint32_t quad);
GAME_FN(uint8_t, __cdecl, SetNineQuads, 0x00839840, void* screenElements, int32_t quad, const float* outer,
        const float* inner, const float* outerUV, const float* innerUV, const float* color);
GAME_FN(uint8_t, __cdecl, HideNineQuads, 0x0083A0B0, void* screenElements, uint32_t quad);
GAME_FN(int32_t, __cdecl, InsertQuads, 0x00839220, int count, void* screenElements);
GAME_FN(float*, __cdecl, GetInnerRect, 0x00839350, float* result, uint32_t index, const float* rect);
GAME_FN(float, __thiscall, Rect_GetHeight, 0x00836910, const float* self);
GAME_FN(void, __thiscall, RenderGroup_RemoveChild, 0x0083AD80, NiUIRenderGroup* self, NiUIBaseElement* child);
GAME_FN(void, __thiscall, RenderGroup_Draw, 0x0083B3F0, NiUIRenderGroup* self, void* renderer);
GAME_FN(NiUIBaseElement*, __thiscall, RenderGroup_GetChildElement, 0x0083B470, NiUIRenderGroup* self, uint32_t i);
GAME_FN(uint32_t, __thiscall, RenderGroup_GetChildElementCount, 0x004D5AD0, const NiUIRenderGroup* self);
GAME_FN(void, __thiscall, ImageButton_SetState, 0x0083AE00, void* self, int state);
GAME_FN(float*, __thiscall, UIElement_GetNSCRect, 0x008391B0, const NiUIBaseElement* self, float* result);
GAME_FN(void, __thiscall, Group_Destruct, 0x0083AFE0, NiUIGroup* self);
GAME_FN(void, __thiscall, Label_DestructAt, 0x00833220, NiUILabel* self);

GAME_ARRAY(float, kLabelText, 0x00BC75C8);       // NiColor
GAME_ARRAY(float, kGroupColor, 0x00BC75E4);      // the skin frame's NiColorA
GAME_ARRAY(float, kWhite, 0x020E25E8);           // a static NiColor (NiColor::WHITE: a guess)
const char* const kEmpty = reinterpret_cast<const char*>(0x008FEFBB);

constexpr uintptr_t kNiUIBaseElementRTTI = 0x020E5A74;
constexpr uintptr_t kNiUILabelRTTI = 0x020E5A04;
constexpr uintptr_t kNiUIGroupRTTI = 0x020E5AA8;
constexpr uintptr_t kNiUIRenderGroupRTTI = 0x020E5ADC;

// |b - a| of two pixel coordinates (unsigned) as a float, as the code converts it
inline float PixelSpan(uint32_t a, uint32_t b) {
    uint32_t span = b > a ? b - a : a - b;
    return float(double(span));
}

// a span of floats as the code takes it: (a < b) ? b - a : a - b
inline float Span(float a, float b) {
    return a < b ? b - a : a - b;
}

// ---- NiUIBaseElement

// IsInside(x, y): within the pixel rect
uint8_t __fastcall IsInside(NiUIBaseElement* self, re::Edx, uint32_t x, uint32_t y) {
    if (x <= self->pixelRect[1] && self->pixelRect[0] <= x && y <= self->pixelRect[3] && self->pixelRect[2] <= y) {
        return 1;
    }
    return 0;
}

// NiUILabel::GetRTTI
const void* __fastcall Label_GetRTTI(const void*, re::Edx) {
    return reinterpret_cast<const void*>(kNiUILabelRTTI);
}

// MouseLocation(x, y): whether it's over it; an element that takes focus signals the mouse
// coming over (focus gained) and leaving (focus lost)
uint8_t __fastcall MouseLocation(NiUIBaseElement* self, re::Edx, uint32_t x, uint32_t y) {
    uint8_t inside = Virtual<uint8_t>(self, 14, x, y);   // IsInside
    if (self->canTakeFocus != 0) {
        if (inside != 0) {
            if (self->mouseInside == 0) {
                Signal0_Emit(&self->focusGained);
            }
        } else if (self->mouseInside != 0) {
            Signal0_Emit(&self->focusLost);
        }
        self->mouseInside = inside;
    }
    return (inside != 0 && self->canTakeFocus != 0) ? 1 : 0;
}

// SetVisible(visible): the children too, the layout updated, shown or hidden signalled
void __fastcall SetVisible(NiUIBaseElement* self, re::Edx, int visible) {
    self->visible = uint8_t(visible);
    uint32_t count = Virtual<uint32_t>(self, 10);   // GetChildElementCount
    for (uint32_t i = 0; i < count; i++) {
        NiUIBaseElement* child = Virtual<NiUIBaseElement*>(self, 11, i);
        if (child != nullptr) {
            Virtual<void>(child, 7, uint32_t(visible));
        }
    }
    Virtual<void>(self, 17);   // UpdateLayout
    const NiUISignal& signal = uint8_t(visible) != 0 ? self->shown : self->hidden;
    uint32_t slots = signal.set.used;
    for (uint32_t i = 0; i < slots; i++) {
        Virtual<void>(signal.set.base[i], 1);
    }
}

// SetFocus(focus): (when it can take it) signalled, kept
uint8_t __fastcall SetFocus(NiUIBaseElement* self, re::Edx, int focus) {
    if (self->canTakeFocus == 0) {
        return 0;
    }
    Signal0_Emit(uint8_t(focus) != 0 ? &self->focusGained : &self->focusLost);
    self->focused = uint8_t(focus);
    return 1;
}

// NiUILabel::Draw(renderer): its string, when visible
void __fastcall Label_Draw(NiUILabel* self, re::Edx, void* renderer) {
    if (self->visible != 0 && self->string2D != nullptr) {
        String2D_Draw(self->string2D, renderer);
    }
}

// NiUILabel's destructor
void __fastcall Label_Destruct(NiUILabel* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiUILabelVtable);
    ClearRef(self->string2D);
    Release(self->string2D);
    NiString_Destruct(&self->wrappedText);
    NiString_Destruct(&self->text);
    UIElement_Destruct(self);
}

// NiUILabel::UpdateLayout: its quad set with the skin's label rect and its background (hidden
// when it is), then the children's
void __fastcall Label_UpdateLayout(NiUILabel* self, re::Edx) {
    if (self->screenElements == nullptr || UIManager_Get() == nullptr) {
        return;
    }
    if (self->visible != 0) {
        const uint8_t* atlas = UIManager_Get()->atlasMap;
        float uv[4] = {At<float>(atlas, 0x198), At<float>(atlas, 0x19C), At<float>(atlas, 0x1A0),
                       At<float>(atlas, 0x1A4)};
        SetQuad(self->screenElements, int32_t(self->baseQuadIndex), self->nscRect, uv, self->backgroundColor);
    } else {
        HideQuad(self->screenElements, self->baseQuadIndex);
    }
    Virtual<void>(self, 18);   // UpdateChildLayout
}

// an alignment's offset within a span: 0 the start, 1 the end, 2 the middle (truncated)
inline int32_t AlignOffset(uint32_t alignment, uint32_t a, uint32_t b, float size) {
    uint32_t span = a > b ? a - b : b - a;
    switch (alignment) {
    case 1:
        return crt::FloatToInt(float(double(span) - double(size)));
    case 2:
        return crt::FloatToInt(float(double(span) - double(size)) * 0.5f);
    default:
        return 0;
    }
}

// NiUILabel::UpdateTextPosition: the string placed in the pixel rect by the alignments
void __fastcall Label_UpdateTextPosition(NiUILabel* self, re::Edx) {
    NiRefObject* string = self->string2D;
    if (string == nullptr) {
        return;
    }
    float width;
    float height;
    FontString_GetTextExtent(string, &width, &height);
    int32_t dy = AlignOffset(self->verticalAlign, self->pixelRect[2], self->pixelRect[3], height);
    int32_t dx = AlignOffset(self->horizontalAlign, self->pixelRect[1], self->pixelRect[0], width);
    uint32_t x = self->pixelRect[0] + uint32_t(dx);
    uint32_t y = self->pixelRect[2] + uint32_t(dy);
    if (At<uint32_t>(string, 0x48) != x || At<uint32_t>(string, 0x4C) != y) {
        At<uint32_t>(string, 0x48) = x;
        At<uint32_t>(string, 0x4C) = y;
        At<uint8_t>(string, 0x44) = 1;   // dirty
    }
}

NiUILabel* __fastcall Label_DeletingDestructor(NiUILabel* self, re::Edx, int flags) {
    Label_DestructAt(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0xDC);
    }
    return self;
}

// NiUILabel::ReformatText: the text (tabs as four spaces) word wrapped to the rect's width
// (at the last space, else broken mid-word), then lines dropped from the end until it fits the
// height, into the string. (Odd cases the game's: a line restarted after a removed character
// measures from the previous line's break; trimming everything leaves the position as it was)
void __fastcall Label_ReformatText(NiUILabel* self, re::Edx) {
    if (UIManager_Get() == nullptr) {
        return;
    }
    NiRefObject* font = UIManager_Get()->font;
    if (font == nullptr) {
        return;
    }
    if (self->string2D == nullptr) {
        void* memory = NiNew(0x70);
        NiRefObject* created = nullptr;
        if (memory != nullptr) {
            created = String2D_Construct(memory, font, 1, 0x100, kEmpty, self->textColor, self->pixelRect[0],
                                         self->pixelRect[2]);
        }
        Pointer_Assign(reinterpret_cast<re::Ptr32<void>*>(&self->string2D), created);
    }
    NiRefObject* string = self->string2D;
    if (At<uint32_t>(string, 0x48) != self->pixelRect[0] || At<uint32_t>(string, 0x4C) != self->pixelRect[2]) {
        At<uint32_t>(string, 0x48) = self->pixelRect[0];
        At<uint32_t>(string, 0x4C) = self->pixelRect[2];
        At<uint8_t>(string, 0x44) = 1;
    }
    float width = PixelSpan(self->pixelRect[0], self->pixelRect[1]);
    float height = PixelSpan(self->pixelRect[3], self->pixelRect[2]);
    if (width == 0.0f || height == 0.0f) {
        return;
    }
    float textWidth = 0.0f;
    float textHeight = 0.0f;
    re::Ptr32<char>* wrapped = &self->wrappedText;
    NiString_AssignText(wrapped, self->text);
    NiString_Replace(wrapped, "\t", "    ");
    Font_GetTextExtent(font, *wrapped, &textWidth, &textHeight, 0);
    if (!(width >= textWidth) || !(height >= textHeight)) {
        bool modified = false;
        if (width < textWidth) {
            int lineEnd = NiString_Find(wrapped, '\n', 0);
            uint32_t pos = 0;
            int lineStart = 0;
            if (lineEnd == -1) {
                lineEnd = int(NiString_Length(wrapped));
                if (lineEnd == -1) {
                    goto heightCheck;
                }
            }
            while (true) {
                // measure the line until it no longer fits
                float lineWidth = 0.0f;
                int lastSpace = -1;
                double widthLimit = width;
                if (double(width) > 0.0) {
                    while (pos != uint32_t(lineEnd)) {
                        float charWidth = 0.0f;
                        float charHeight = 0.0f;
                        char c = NiString_GetAt(wrapped, pos);
                        if (c == ' ') {
                            lastSpace = int(pos);
                        }
                        Font_GetCharExtent(font, uint8_t(c), &charWidth, &charHeight);
                        lineWidth = charWidth + lineWidth;
                        pos++;
                        if (!(lineWidth < widthLimit)) {
                            break;
                        }
                    }
                }
                if (width <= lineWidth) {
                    if (lastSpace != -1) {
                        NiString_SetAt(wrapped, uint32_t(lastSpace), '\n');
                        lineEnd = lastSpace;
                        modified = true;
                    } else if ((lineEnd - lineStart) == 1 || (int(pos) - lineStart) == 1) {
                        // a single character too wide: dropped, the line measured again
                        NiString_RemoveRange(wrapped, pos - 1, 1);
                        pos = uint32_t(lineStart);
                        lineEnd = lineEnd - 1;
                        modified = true;
                        if (lineEnd != -1) {
                            continue;
                        }
                        break;
                    } else {
                        lineEnd = int(pos) - 1;
                        NiString_Insert(wrapped, "\n", pos - 1);
                        modified = true;
                    }
                }
                // the next line
                pos = uint32_t(lineEnd + 1);
                lineStart = lineEnd;
                lineEnd = NiString_Find(wrapped, '\n', pos);
                if (pos >= NiString_Length(wrapped)) {
                    break;
                }
                if (lineEnd != -1) {
                    continue;
                }
                lineEnd = int(NiString_Length(wrapped));
                if (lineEnd == -1) {
                    break;
                }
            }
            if (modified) {
                Font_GetTextExtent(font, *wrapped, &textWidth, &textHeight, 0);
            }
        }
    heightCheck:
        double heightLimit = height;
        if (textHeight > heightLimit) {
            do {
                int lastBreak = NiString_FindReverse(wrapped, '\n');
                if (lastBreak == -1) {
                    NiString_AssignText(wrapped, kEmpty);
                    FontString_SetText(self->string2D, *wrapped, 0xFFFFFFFF);
                    return;
                }
                NiString_RemoveRange(wrapped, uint32_t(lastBreak), NiString_Length(wrapped) - uint32_t(lastBreak));
                Font_GetTextExtent(font, *wrapped, &textWidth, &textHeight, 0);
            } while (textHeight > heightLimit);
        }
    }
    FontString_SetText(self->string2D, *wrapped, 0xFFFFFFFF);
    UILabel_UpdateTextPosition(self);
}

// ReinitializeChildren: each child's display elements (the count asked each time)
void __fastcall ReinitializeChildren(NiUIBaseElement* self, re::Edx) {
    uint32_t i = 0;
    if (Virtual<uint32_t>(self, 10) == 0) {
        return;
    }
    do {
        NiUIBaseElement* child = Virtual<NiUIBaseElement*>(self, 11, i);
        if (child != nullptr) {
            Virtual<void>(child, 12);
        }
        i++;
    } while (i < Virtual<uint32_t>(self, 10));
}

// UpdateLayout (the render group's): the children's
void __fastcall RenderGroup_UpdateLayout(NiUIBaseElement* self, re::Edx) {
    Virtual<void>(self, 18);
}

// UpdateChildLayout: each child's layout
void __fastcall UpdateChildLayout(NiUIBaseElement* self, re::Edx) {
    uint32_t count = Virtual<uint32_t>(self, 10);
    for (uint32_t i = 0; i < count; i++) {
        NiUIBaseElement* child = Virtual<NiUIBaseElement*>(self, 11, i);
        if (child != nullptr) {
            Virtual<void>(child, 17);
        }
    }
}

// InsertQuads(count, screen elements): that many 4-vertex polygons added; the first one's
// index (-1 nothing to do)
int32_t __cdecl InsertQuadsImpl(int count, void* screenElements) {
    if (count == 0 || screenElements == nullptr) {
        return -1;
    }
    uint16_t first = At<uint16_t>(At<void*>(screenElements, 0xCC), 0x64);
    for (int i = count; i > 0; i--) {
        ScreenData_Insert(At<void*>(screenElements, 0xCC), 4, 0, nullptr);
    }
    return first;
}

// ReinitializeDisplayElements: the normalized rect from the offset (from the parent's corner)
// and the size, the pixel rect from it, then the rect, children and layout updated
void __fastcall ReinitializeDisplayElements(NiUIBaseElement* self, re::Edx) {
    if (NiRenderer_ms_pkRenderer == nullptr) {
        return;
    }
    float x = self->offset[0];
    float y = self->offset[1];
    float w = self->dimensions[0];
    float h = self->dimensions[1];
    NiUIBaseElement* parent = self->parent;
    if (parent != nullptr) {
        x = x + parent->nscRect[0];
        y = y + parent->nscRect[2];
    }
    self->nscRect[0] = x;
    self->nscRect[1] = x + w;
    self->nscRect[2] = y;
    self->nscRect[3] = y + h;
    uint32_t pixels[4];
    UIManager_ConvertNSCToPixels(UIManager_Get(), pixels, self->nscRect);
    self->pixelRect[0] = pixels[0];
    self->pixelRect[1] = pixels[1];
    self->pixelRect[2] = pixels[2];
    self->pixelRect[3] = pixels[3];
    Virtual<void>(self, 15);   // UpdateRect
    Virtual<void>(self, 16);   // ReinitializeChildren
    Virtual<void>(self, 17);   // UpdateLayout
}

// GetInnerRect(result, index, rect): the rect inset by the skin element's borders (in pixels,
// made normalized), not past its middle
float* __cdecl GetInnerRectImpl(float* result, uint32_t index, const float* rect) {
    NiUIManager* manager = UIManager_Get();
    result[0] = rect[0];
    result[1] = rect[1];
    result[2] = rect[2];
    result[3] = rect[3];
    float border[4];
    AtlasMap_GetBorder(manager->atlasMap, border, index, 2);
    float size[2];
    UIManager_ConvertSizeToNSC(manager, size, border[0], border[2]);
    result[0] = result[0] + size[0];
    result[2] = result[2] + size[1];
    UIManager_ConvertSizeToNSC(manager, size, border[1], border[3]);
    float right = result[1] - size[0];
    result[1] = right;
    float bottom = result[3] - size[1];
    result[3] = bottom;
    float middleX = (rect[1] + rect[0]) * 0.5f;
    float middleY = (rect[3] + rect[2]) * 0.5f;
    float left = result[0];
    result[0] = middleX > left ? left : middleX;
    float top = result[2];
    result[2] = middleY > top ? top : middleY;
    result[1] = right > middleX ? right : middleX;
    result[3] = bottom > middleY ? bottom : middleY;
    return result;
}

// GetScaledRect(result, index, rect): the skin element's size fitted in the rect (scaled down
// to fit, keeping its aspect) and centred
float* __cdecl GetScaledRect(float* result, uint32_t index, const float* rect) {
    NiUIManager* manager = UIManager_Get();
    result[0] = rect[0];
    result[1] = rect[1];
    result[2] = rect[2];
    result[3] = rect[3];
    float texture[4];
    AtlasMap_ToUV(manager->atlasMap, texture, reinterpret_cast<const float*>(manager->atlasMap + 8 + index * 16));
    float textureHeight = Span(texture[3], texture[2]);
    float textureWidth = Span(texture[0], texture[1]);
    float size[2];
    UIManager_ConvertSizeToNSC(manager, size, textureWidth, textureHeight);
    float rectHeight = Span(result[3], result[2]);
    float rectWidth = Span(result[0], result[1]);
    float scaleX = rectWidth / size[0];
    float scaleY = rectHeight / size[1];
    float scale = 1.0f;
    if (!(scaleX >= 1.0f) || !(scaleY >= 1.0f)) {
        scale = scaleX <= scaleY ? scaleX : scaleY;
    }
    float marginX = (rectWidth - size[0] * scale) * 0.5f;
    float marginY = 0.5f * (rectHeight - size[1] * scale);
    result[0] = result[0] + marginX;
    result[1] = result[1] - marginX;
    result[2] = result[2] + marginY;
    result[3] = result[3] - marginY;
    return result;
}

// SetQuad(screen elements, quad, rect, uv, color): one quad over the rect (in the safe area),
// coloured and textured
uint8_t __cdecl SetQuadImpl(void* screenElements, int32_t quad, const float* rect, const float* uv,
                            const float* color) {
    if (screenElements == nullptr || quad < 0) {
        return 0;
    }
    float area[4];
    UIManager_ConvertNSCToSafeArea(UIManager_Get(), area, rect);
    float height = Span(area[3], area[2]);
    float width = Span(area[0], area[1]);
    if (!ScreenData_SetRectangle(At<void*>(screenElements, 0xCC), quad, area[0], area[2], width, height)) {
        return 0;
    }
    if (!ScreenData_SetColors(At<void*>(screenElements, 0xCC), quad, color)) {
        return 0;
    }
    return ScreenElements_SetTextures(screenElements, quad, 0, uv[0], uv[2], uv[1], uv[3]) != 0 ? 1 : 0;
}

// HideQuad(screen elements, quad): its rect made empty
uint8_t __cdecl HideQuadImpl(void* screenElements, uint32_t quad) {
    if (screenElements == nullptr) {
        return 0;
    }
    for (uint32_t i = quad; i < quad + 1; i++) {
        ScreenData_SetRectangle(At<void*>(screenElements, 0xCC), int32_t(i), 0.0f, 0.0f, 0.0f, 0.0f);
    }
    return 1;
}

// SetNineQuads(screen elements, quad, outer, inner, outer uv, inner uv, color): a skin frame
// (corners, edges, middle: row by row) between the outer and inner rects; the rects set while
// they succeed, all coloured while that does, textured while that does; all three needed
uint8_t __cdecl SetNineQuadsImpl(void* screenElements, int32_t quad, const float* outerRect, const float* innerRect,
                                 const float* outerUV, const float* innerUV, const float* color) {
    if (screenElements == nullptr || quad < 0) {
        return 0;
    }
    uint8_t colorsOk = 1;
    NiUIManager* manager = UIManager_Get();
    float inner[4];
    UIManager_ConvertNSCToSafeArea(manager, inner, innerRect);
    float outer[4];
    UIManager_ConvertNSCToSafeArea(manager, outer, outerRect);
    const float columns[3][2] = {{outer[0], inner[0]}, {inner[0], inner[1]}, {inner[1], outer[1]}};
    const float rows[3][2] = {{outer[2], inner[2]}, {inner[2], inner[3]}, {inner[3], outer[3]}};
    bool rectsOk = true;
    for (int row = 0; row < 3 && rectsOk; row++) {
        for (int column = 0; column < 3 && rectsOk; column++) {
            float span[4] = {columns[column][0], columns[column][1], rows[row][0], rows[row][1]};
            float width = Span(span[0], span[1]);
            float height = Rect_GetHeight(span);
            rectsOk = ScreenElements_SetRectangle(screenElements, quad + row * 3 + column, span[0], span[2], width,
                                                  height) != 0;
        }
    }
    for (int32_t i = 0; i < 9; i++) {
        if (colorsOk != 0) {
            colorsOk = ScreenData_SetColors(At<void*>(screenElements, 0xCC), i + quad, color) != 0 ? 1 : 0;
        } else {
            colorsOk = 0;
        }
    }
    const float uColumns[3][2] = {{outerUV[0], innerUV[0]}, {innerUV[0], innerUV[1]}, {innerUV[1], outerUV[1]}};
    const float vRows[3][2] = {{outerUV[2], innerUV[2]}, {innerUV[2], innerUV[3]}, {innerUV[3], outerUV[3]}};
    bool texturesOk = true;
    for (int row = 0; row < 3 && texturesOk; row++) {
        for (int column = 0; column < 3 && texturesOk; column++) {
            texturesOk = ScreenData_SetTextures(At<void*>(screenElements, 0xCC), quad + row * 3 + column, 0,
                                                uColumns[column][0], vRows[row][0], uColumns[column][1],
                                                vRows[row][1]) != 0;
        }
    }
    return (rectsOk && colorsOk != 0 && texturesOk) ? 1 : 0;
}

// HideNineQuads(screen elements, quad)
uint8_t __cdecl HideNineQuadsImpl(void* screenElements, uint32_t quad) {
    if (screenElements == nullptr) {
        return 0;
    }
    for (uint32_t i = quad; i < quad + 9; i++) {
        ScreenData_SetRectangle(At<void*>(screenElements, 0xCC), int32_t(i), 0.0f, 0.0f, 0.0f, 0.0f);
    }
    return 1;
}

// AttachResources(screen elements): its quads inserted (kept as the base index), then the
// children's; false at the first child that fails
uint8_t __fastcall AttachResources(NiUIBaseElement* self, re::Edx, NiRefObject* screenElements) {
    if (screenElements == nullptr) {
        return 0;
    }
    int count = Virtual<int>(self, 13);   // NumQuadsRequired
    if (count != 0) {
        Pointer_Assign(reinterpret_cast<re::Ptr32<void>*>(&self->screenElements), screenElements);
        self->baseQuadIndex = uint32_t(InsertQuads(count, screenElements));
    }
    uint32_t children = Virtual<uint32_t>(self, 10);
    for (uint32_t i = 0; i < children; i++) {
        NiUIBaseElement* child = Virtual<NiUIBaseElement*>(self, 11, i);
        if (child != nullptr && !Virtual<uint8_t>(child, 9, screenElements)) {
            return 0;
        }
    }
    Virtual<void>(self, 17);
    return 1;
}

// NiUIBaseElement's constructor (whether it can take focus)
NiUIBaseElement* __fastcall Construct(NiUIBaseElement* self, re::Edx, int canTakeFocus) {
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    self->refCount = 0;
    ni::sys::Increment(&NiRefObject_ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(kNiUIBaseElementVtable);
    for (float& f : self->nscRect) {
        f = 0.0f;
    }
    for (uint32_t& u : self->pixelRect) {
        u = 0;
    }
    self->offset[0] = 0.0f;
    self->offset[1] = 0.0f;
    self->dimensions[0] = 0.0f;
    self->dimensions[1] = 0.0f;
    self->parent = nullptr;
    NiUISignal* signals[5] = {&self->shown, &self->hidden, &self->focusGained, &self->focusLost, &self->signal7C};
    for (NiUISignal* signal : signals) {
        signal->vtable = reinterpret_cast<void*>(kNiUISignalVtable);
        signal->set.base = nullptr;
        signal->set.alloced = 0;
        signal->set.used = 0;
    }
    self->baseQuadIndex = 0;
    self->screenElements = nullptr;
    self->canTakeFocus = uint8_t(canTakeFocus);
    self->focused = 0;
    self->mouseInside = 0;
    self->visible = 1;
    return self;
}

const void* __fastcall GetRTTI(const void*, re::Edx) {
    return reinterpret_cast<const void*>(kNiUIBaseElementRTTI);
}

// NiUIBaseElement's destructor
void __fastcall Destruct(NiUIBaseElement* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiUIBaseElementVtable);
    ClearRef(self->screenElements);
    Release(self->screenElements);
    Signal_Destruct(&self->signal7C);
    Signal_Destruct(&self->focusLost);
    Signal_Destruct(&self->focusGained);
    Signal_Destruct(&self->hidden);
    Signal_Destruct(&self->shown);
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    ni::sys::Decrement(&NiRefObject_ms_uiObjects);
}

NiUIBaseElement* __fastcall DeletingDestructor(NiUIBaseElement* self, re::Edx, int flags) {
    UIElement_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x98);
    }
    return self;
}

// the small accessors
void __fastcall SetParent(NiUIBaseElement* self, re::Edx, NiUIBaseElement* parent) {
    self->parent = parent;
}
void __fastcall SetDimensions(NiUIBaseElement* self, re::Edx, float w, float h) {
    self->dimensions[0] = w;
    self->dimensions[1] = h;
}
void __fastcall SetOffset(NiUIBaseElement* self, re::Edx, float x, float y) {
    self->offset[0] = x;
    self->offset[1] = y;
}
float* __fastcall GetOffset(NiUIBaseElement* self, re::Edx, float* result) {
    result[0] = self->offset[0];
    result[1] = self->offset[1];
    return result;
}
float* __fastcall GetNSCRect(const NiUIBaseElement* self, re::Edx, float* result) {
    result[0] = self->nscRect[0];
    result[1] = self->nscRect[1];
    result[2] = self->nscRect[2];
    result[3] = self->nscRect[3];
    return result;
}

// NiUILabel's constructor (text, background NiColorA, text NiColor)
NiUILabel* __fastcall Label_Construct(NiUILabel* self, re::Edx, const char* text, const float* background,
                                      const float* color) {
    UIElement_Construct(self, 0);
    self->vtable = reinterpret_cast<void*>(kNiUILabelVtable);
    for (uint32_t& u : self->unknown98) {
        u = 0;
    }
    std::memcpy(self->backgroundColor, background, sizeof(self->backgroundColor));
    self->textColor[0] = color[0];
    self->textColor[1] = color[1];
    self->textColor[2] = color[2];
    self->textColor[3] = 1.0f;
    NiString_Construct(&self->text, text);
    NiString_ConstructSized(&self->wrappedText, 0x100);
    self->string2D = nullptr;
    self->verticalAlign = 0;
    self->horizontalAlign = 0;
    return self;
}

// ---- NiUIGroup

// ReinitializeDisplayElements: as the base's, but only the header's height when collapsed and
// the parent's corner from its GetNSCRect
void __fastcall Group_ReinitializeDisplayElements(NiUIGroup* self, re::Edx) {
    if (NiRenderer_ms_pkRenderer == nullptr) {
        return;
    }
    float x = self->offset[0];
    float y = self->offset[1];
    float w = self->dimensions[0];
    float h = self->dimensions[1];
    if (self->expanded == 0) {
        h = self->headerHeight;
    }
    if (self->parent != nullptr) {
        float parentRect[4];
        UIElement_GetNSCRect(self->parent, parentRect);
        x = x + parentRect[0];
        y = y + parentRect[2];
    }
    self->nscRect[0] = x;
    self->nscRect[1] = x + w;
    self->nscRect[2] = y;
    self->nscRect[3] = y + h;
    uint32_t pixels[4];
    UIManager_ConvertNSCToPixels(UIManager_Get(), pixels, self->nscRect);
    self->pixelRect[0] = pixels[0];
    self->pixelRect[1] = pixels[1];
    self->pixelRect[2] = pixels[2];
    self->pixelRect[3] = pixels[3];
    Virtual<void>(self, 15);
    Virtual<void>(self, 16);
    Virtual<void>(self, 17);
}

// GetChildElementCount: the children, the button and the title
uint32_t __fastcall Group_GetChildElementCount(NiUIGroup* self, re::Edx) {
    return RenderGroup_GetChildElementCount(self) + 2;
}

// SetExpanded(expanded): the children shown or hidden with it, the layout redone
void __fastcall Group_SetExpanded(NiUIGroup* self, re::Edx, int expanded) {
    self->expanded = uint8_t(expanded);
    for (uint32_t i = 0; i < self->children.used; i++) {
        NiUIBaseElement* child = self->children.base[i];
        if (child != nullptr) {
            Virtual<void>(child, 7, uint32_t(self->expanded));
        }
    }
    Virtual<void>(self, 12);
    Virtual<void>(self, 17);
}

// GetFocusedElement: the button (-1), a child, or none (-2)
NiUIBaseElement* __fastcall Group_GetFocusedElement(NiUIGroup* self, re::Edx) {
    int32_t focused = self->focusedChild;
    if (focused == -2) {
        return nullptr;
    }
    if (focused == -1) {
        return self->expandButton;
    }
    return self->children.base[focused];
}

// UpdateLayout: the skin frame's nine quads between the outer and inner rects (hidden when it
// is), then the children's
void __fastcall Group_UpdateLayout(NiUIGroup* self, re::Edx) {
    if (self->screenElements == nullptr || UIManager_Get() == nullptr) {
        return;
    }
    if (self->visible != 0) {
        const uint8_t* atlas = UIManager_Get()->atlasMap;
        float outerUV[4] = {At<float>(atlas, 0x248), At<float>(atlas, 0x24C), At<float>(atlas, 0x250),
                            At<float>(atlas, 0x254)};
        float innerUV[4] = {At<float>(atlas, 0x4F8), At<float>(atlas, 0x4FC), At<float>(atlas, 0x500),
                            At<float>(atlas, 0x504)};
        SetNineQuads(self->screenElements, int32_t(self->baseQuadIndex), self->outerRect, self->innerRect, outerUV,
                     innerUV, kGroupColor);
    } else {
        HideNineQuads(self->screenElements, self->baseQuadIndex);
    }
    Virtual<void>(self, 18);
}

// EnterPressed / LeftMouseButton(state): to the focused element, when visible
template <int Slot>
void __fastcall Group_Forward(NiUIGroup* self, re::Edx, uint32_t state) {
    if (self->visible == 0) {
        return;
    }
    int32_t focused = self->focusedChild;
    if (focused == -1) {
        Virtual<void>(self->expandButton, Slot, state);
    } else if (focused >= 0 && uint32_t(focused) < self->children.used) {
        Virtual<void>(self->children.base[focused], Slot, state);
    }
}

// SetFocus(focus): losing it clears the focus (to none); gaining it with none focuses the
// button; the focused element told
uint8_t __fastcall Group_SetFocus(NiUIGroup* self, re::Edx, int focus) {
    int32_t old = self->focusedChild;
    if (uint8_t(focus) == 0) {
        self->focusedChild = -2;
    } else if (old < 0) {
        self->focusedChild = -1;
        Virtual<uint8_t>(self->expandButton, 6, uint32_t(focus));
        return 1;
    }
    if (old == -1) {
        Virtual<uint8_t>(self->expandButton, 6, uint32_t(focus));
        return 1;
    }
    if (old >= 0 && uint32_t(old) < self->children.used) {
        Virtual<uint8_t>(self->children.base[old], 6, uint32_t(focus));
    }
    return 1;
}

// AdvanceFocus: the focus to the next element that takes it (the button first; past the end
// or collapsed: none)
uint8_t __fastcall Group_AdvanceFocus(NiUIGroup* self, re::Edx) {
    self->focusedChild++;
    NiUIBaseElement* button = self->expandButton;
    uint32_t index = uint32_t(self->focusedChild);
    if (int32_t(index) == -1) {
        Virtual<uint8_t>(button, 6, 1u);
        return 1;
    }
    if (At<uint8_t>(button, 0xA8) == 0) {
        Virtual<uint8_t>(button, 6, 0u);
        self->focusedChild = -2;
        return 0;
    }
    uint32_t count = self->children.used;
    if (index < count) {
        NiUIBaseElement* previous = index != 0 ? static_cast<NiUIBaseElement*>(self->children.base[index - 1]) : button;
        Virtual<uint8_t>(previous, 6, 0u);
        if (Virtual<uint8_t>(self->children.base[index], 6, 1u)) {
            self->focusedChild = int32_t(index);
            return 1;
        }
        while (true) {
            Virtual<uint8_t>(self->children.base[index], 6, 0u);
            index++;
            if (index >= self->children.used) {
                break;
            }
            if (Virtual<uint8_t>(self->children.base[index], 6, 1u)) {
                self->focusedChild = int32_t(index);
                return 1;
            }
        }
    } else {
        NiUIBaseElement* last = (index == count && index != 0) ? static_cast<NiUIBaseElement*>(self->children.base[index - 1]) : button;
        Virtual<uint8_t>(last, 6, 0u);
    }
    self->focusedChild = -2;
    return 0;
}

// ReverseFocus: the focus to the previous element that takes it (from none: the last, or the
// button when collapsed; before the first: the button)
uint8_t __fastcall Group_ReverseFocus(NiUIGroup* self, re::Edx) {
    NiUIBaseElement* button = self->expandButton;
    if (self->focusedChild < -1) {
        self->focusedChild = At<uint8_t>(button, 0xA8) != 0 ? int32_t(self->children.used) : 0;
    }
    self->focusedChild--;
    int32_t index = self->focusedChild;
    if (index == -1) {
        if (At<uint8_t>(self->expandButton, 0xA8) != 0 && self->children.used != 0) {
            Virtual<uint8_t>(self->children.base[0], 6, 0u);
        }
        Virtual<uint8_t>(self->expandButton, 6, 1u);
        return 1;
    }
    if (index < -1) {
        Virtual<uint8_t>(self->expandButton, 6, 0u);
        return 0;
    }
    if (uint32_t(index + 1) < self->children.used) {
        Virtual<uint8_t>(self->children.base[index + 1], 6, 0u);
    }
    if (Virtual<uint8_t>(self->children.base[index], 6, 1u)) {
        self->focusedChild = index;
        return 1;
    }
    while (true) {
        Virtual<uint8_t>(self->children.base[index], 6, 0u);
        if (index == 0) {
            self->focusedChild = -1;
            Virtual<uint8_t>(self->expandButton, 6, 1u);
            return 1;
        }
        uint8_t taken = Virtual<uint8_t>(self->children.base[index - 1], 6, 1u);
        index--;
        if (taken) {
            self->focusedChild = index;
            return 1;
        }
    }
}

// IsInside(x, y): within the group, the button or (expanded) a child under the mouse takes the
// focus
uint8_t __fastcall Group_IsInside(NiUIGroup* self, re::Edx, uint32_t x, uint32_t y) {
    if (x < self->mouseRect[0] || x > self->mouseRect[1] || y < self->mouseRect[2] || y > self->mouseRect[3]) {
        return 0;
    }
    uint32_t count = self->children.used;
    if (Virtual<uint8_t>(self->expandButton, 8, x, y)) {   // MouseLocation
        int32_t focused = self->focusedChild;
        if (focused != -1 && focused >= 0 && uint32_t(focused) < count) {
            Virtual<uint8_t>(self->children.base[focused], 6, 0u);
        }
        self->focusedChild = -1;
        return 1;
    }
    if (At<uint8_t>(self->expandButton, 0xA8) == 0) {
        return 0;
    }
    for (uint32_t i = 0; i < count; i++) {
        if (Virtual<uint8_t>(self->children.base[i], 8, x, y)) {
            if (self->focusedChild == -1) {
                Virtual<uint8_t>(self->expandButton, 6, 0u);
            }
            int32_t focused = self->focusedChild;
            if (uint32_t(focused) != i && focused >= 0 && uint32_t(focused) < count) {
                Virtual<uint8_t>(self->children.base[focused], 6, 0u);
            }
            self->focusedChild = int32_t(i);
            return 1;
        }
    }
    return 0;
}

// GetChildElement(i): a child, then the button, then the title
NiUIBaseElement* __fastcall Group_GetChildElement(NiUIGroup* self, re::Edx, uint32_t i) {
    uint32_t count = RenderGroup_GetChildElementCount(self);
    if (i < count) {
        return RenderGroup_GetChildElement(self, i);
    }
    if (i == count) {
        return self->expandButton;
    }
    if (i - count == 1) {
        return self->titleLabel;
    }
    return nullptr;
}

// UpdateRect: the outer rect its own, the inner the skin frame's inside; the title over 90% of
// the inner width and the button over the rest, both the header's height less the frame's
void __fastcall Group_UpdateRect(NiUIGroup* self, re::Edx) {
    for (int i = 0; i < 4; i++) {
        self->outerRect[i] = self->nscRect[i];
    }
    float inner[4];
    GetInnerRect(inner, 0x24, self->outerRect);
    for (int i = 0; i < 4; i++) {
        self->innerRect[i] = inner[i];
    }
    float x = self->innerRect[0] - self->outerRect[0];
    float y = self->innerRect[2] - self->outerRect[2];
    float innerWidth = Span(self->innerRect[0], self->innerRect[1]);
    float labelHeight = self->headerHeight - (y + y);
    UIElement_SetOffset(self->titleLabel, x, y);
    // (the products by double constants: one double rounding, the x87's single rounding at
    // 24 bits otherwise)
    float labelWidth = float(double(innerWidth) * 0.9);
    UIElement_SetDimensions(self->titleLabel, labelWidth, labelHeight);
    UIElement_SetOffset(self->expandButton, float(double(x) + double(labelWidth)), y);
    UIElement_SetDimensions(self->expandButton, float(double(innerWidth) * 0.1), labelHeight);
    uint32_t pixels[4];
    UIElement_GetPixelRect(self, pixels);
    for (int i = 0; i < 4; i++) {
        self->mouseRect[i] = pixels[i];
    }
}

// Draw(renderer): the render group's, then (visible) the title and the button
void __fastcall Group_Draw(NiUIGroup* self, re::Edx, void* renderer) {
    RenderGroup_Draw(self, renderer);
    if (self->visible != 0) {
        Virtual<void>(self->titleLabel, 3, renderer);
        Virtual<void>(self->expandButton, 3, renderer);
    }
}

// NiUIRenderGroup::RemoveChild(child): taken out of the children (its parent set to the group
// again: the game's)
void __fastcall RenderGroup_RemoveChildImpl(NiUIRenderGroup* self, re::Edx, NiUIBaseElement* child) {
    if (child == nullptr) {
        return;
    }
    ni::sys::Increment(&child->refCount);
    uint32_t count = self->children.used;
    uint32_t found = 0xFFFFFFFF;
    for (uint32_t i = 0; i < count; i++) {
        if (self->children.base[i] == child) {
            found = i;
            break;
        }
    }
    if (ni::sys::Decrement(&child->refCount) == 0) {
        Virtual<void>(child, 1);
    }
    if (found != 0xFFFFFFFF) {
        UIElement_SetParent(child, self);
        ObjectSet_RemoveAt(&self->children, found);
    }
}

// NiUIImageButton::SetState(state): kept, the layout updated, its slots told
void __fastcall ImageButton_SetStateImpl(uint8_t* self, re::Edx, int state) {
    At<uint8_t>(self, 0xA8) = uint8_t(state);
    Virtual<void>(self, 17);
    uint32_t count = At<uint32_t>(self, 0xA4);
    uint8_t value = At<uint8_t>(self, 0xA8);
    for (uint32_t i = 0; i < count; i++) {
        Virtual<void>(At<re::Ptr32<void>*>(self, 0x9C)[i], 1, uint32_t(value));
    }
}

// NiUIGroup::RemoveChild(child)
void __fastcall Group_RemoveChild(NiUIGroup* self, re::Edx, NiUIBaseElement* child) {
    if (child != nullptr) {
        RenderGroup_RemoveChild(self, child);
    }
}

// NiUIGroup's destructor
void __fastcall Group_DestructImpl(NiUIGroup* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiUIGroupVtable);
    ClearRef(self->expandButton);
    self->expandSlot.vtable = reinterpret_cast<void*>(kNiUIMemberSlot1GroupVtable);
    self->expandSlot.instance = nullptr;
    self->expandSlot.function = nullptr;
    self->expandSlot.adjust = 0;
    BaseSlot1Bool_Destruct(&self->expandSlot);
    Release(self->expandButton);
    Release(self->titleLabel);
    UIRenderGroup_Destruct(self);
}

const void* __fastcall Group_GetRTTI(const void*, re::Edx) {
    return reinterpret_cast<const void*>(kNiUIGroupRTTI);
}

// SetVisible(visible): the base's, and shown with its expanded state
void __fastcall Group_SetVisible(NiUIGroup* self, re::Edx, int visible) {
    UIElement_SetVisible(self, visible);
    if (uint8_t(visible) != 0) {
        UIGroup_SetExpanded(self, self->expanded);
    }
}

NiUIGroup* __fastcall Group_DeletingDestructor(NiUIGroup* self, re::Edx, int flags) {
    Group_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x110);
    }
    return self;
}

// NiUIGroup's constructor (name, header height, expanded): render order 2, the title label in
// the skin's title colour (0x20E25E8's without a UI manager), the expand button on the skin's images
// with its state signal on SetExpanded
NiUIGroup* __fastcall Group_Construct(NiUIGroup* self, re::Edx, const char* name, float headerHeight, int expanded) {
    UIRenderGroup_Construct(self, 2);
    self->headerHeight = headerHeight;
    self->vtable = reinterpret_cast<void*>(kNiUIGroupVtable);
    self->titleLabel = nullptr;
    self->expanded = uint8_t(expanded);
    for (int i = 0; i < 4; i++) {
        self->outerRect[i] = 0.0f;
        self->innerRect[i] = 0.0f;
    }
    self->expandButton = nullptr;
    self->expandSlot.set.base = nullptr;
    self->expandSlot.set.alloced = 0;
    self->expandSlot.set.used = 0;
    self->expandSlot.vtable = reinterpret_cast<void*>(kNiUIMemberSlot1GroupVtable);
    for (uint32_t& u : self->mouseRect) {
        u = 0;
    }
    self->focusedChild = -2;
    self->expandSlot.instance = self;
    self->expandSlot.function = reinterpret_cast<void*>(0x0083A4F0);   // SetExpanded
    self->expandSlot.adjust = 0;
    float color[3] = {kWhite[0], kWhite[1], kWhite[2]};
    if (UIManager_Get() != nullptr) {
        NiUIManager* manager = UIManager_Get();
        color[0] = At<float>(manager, 0x620);
        color[1] = At<float>(manager, 0x624);
        color[2] = At<float>(manager, 0x628);
    }
    auto* label = static_cast<NiUILabel*>(NiNew(0xDC));
    if (label != nullptr) {
        float background[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        UILabel_Construct(label, name, background, color);
    }
    AssignRef(self->titleLabel, label);
    UIElement_SetParent(self->titleLabel, self);
    self->titleLabel->verticalAlign = 2;
    self->titleLabel->horizontalAlign = 0;
    UILabel_UpdateTextPosition(self->titleLabel);
    auto* button = static_cast<NiUIBaseElement*>(NiNew(0xC0));
    if (button != nullptr) {
        ImageButton_Construct(button);
    }
    AssignRef(self->expandButton, button);
    ImageButton_SetState(self->expandButton, expanded);
    At<uint32_t>(self->expandButton, 0xB0) = 0x25;
    At<uint32_t>(self->expandButton, 0xB4) = 0x27;
    At<uint32_t>(self->expandButton, 0xB8) = 0x28;
    At<uint32_t>(self->expandButton, 0xBC) = 0x2A;
    UIElement_SetParent(self->expandButton, self);
    Signal1Bool_Subscribe(reinterpret_cast<NiUISignal*>(re::PtrCast<uint8_t*>(self->expandButton) + 0x98),
                          &self->expandSlot);
    self->canTakeFocus = 1;
    return self;
}

// AddChild(child) (the group's: shown if expanded; the render group's: if visible): kept,
// parented
template <int VisibleField>
void __fastcall AddChild(NiUIRenderGroup* self, re::Edx, NiUIBaseElement* child) {
    if (child == nullptr) {
        return;
    }
    NiUIBaseElement* held = child;
    ni::sys::Increment(&child->refCount);
    Virtual<void>(child, 7, uint32_t(At<uint8_t>(self, VisibleField)));
    ObjectSet_AddUnique(&self->children, re::PtrCast<const re::Ptr32<void>*>(&held));
    UIElement_SetParent(child, self);
    if (ni::sys::Decrement(&child->refCount) == 0) {
        Virtual<void>(child, 1);
    }
}

// NiUIRenderGroup::Draw(renderer): (visible) the renderer's batch flushed when it needs it,
// the screen elements drawn, then each child
void __fastcall RenderGroup_DrawImpl(NiUIRenderGroup* self, re::Edx, void* renderer) {
    if (self->visible == 0) {
        return;
    }
    int32_t mode = At<int32_t>(renderer, 0x94);
    if ((mode == 1 || mode == 2 || mode == 4) && (At<uint8_t>(renderer, 0x90) == 1 || mode == 4)) {
        Virtual<void>(renderer, 87, 0u);
    }
    Geometry_RenderImmediate(self->screenElements, renderer);
    uint32_t count = self->children.used;
    for (uint32_t i = 0; i < count; i++) {
        Virtual<void>(self->children.base[i], 3, renderer);
    }
}

// NiUIRenderGroup::GetChildElement(i)
NiUIBaseElement* __fastcall RenderGroup_GetChildElementImpl(NiUIRenderGroup* self, re::Edx, uint32_t i) {
    return self->children.base[i];
}

// a property NiPointer dropped after attaching (the reference the code held)
inline void AttachAndRelease(void* screenElements, void* property) {
    AVObject_AttachProperty(screenElements, property);
    if (ni::sys::Decrement(static_cast<uint8_t*>(property) + 4) == 0) {
        Virtual<void>(property, 1);
    }
}

// NiUIRenderGroup::InitializeScreenElements: (once, with the skin's texture there) one screen
// elements object for the group with the skin's texture, vertex colours, a white material and
// alpha blending; the children's quads attached, the bound and properties updated
void __fastcall RenderGroup_InitializeScreenElements(NiUIRenderGroup* self, re::Edx) {
    if (self->screenElements != nullptr || UIManager_Get() == nullptr || At<void*>(UIManager_Get(), 0xA0) == nullptr) {
        return;
    }
    void* elements = NiNew(0xEC);
    NiAVObject* created = nullptr;
    if (elements != nullptr) {
        void* data = NiNew(0x70);
        if (data != nullptr) {
            data = ScreenData_Construct(data, 0, 1, 1, 1, 1, 4, 1, 2, 1);
        }
        created = ScreenElements_Construct(elements, data);
    }
    Pointer_Assign(reinterpret_cast<re::Ptr32<void>*>(&self->screenElements), created);
    void* texturing = NiNew(0x34);
    if (texturing != nullptr) {
        texturing = TexturingProperty_ConstructWithTexture(texturing, At<void*>(At<void*>(UIManager_Get(), 0xA0), 0x3C));
        AddRef(texturing);
    }
    At<uint16_t>(texturing, 0x18) = uint16_t((At<uint16_t>(texturing, 0x18) & 0xFFF5) | 4);
    TexturingProperty_SetBaseFilterMode(texturing, 1);
    AttachAndRelease(self->screenElements, texturing);
    auto* vertexColor = static_cast<uint8_t*>(NiNew(0x1C));
    if (vertexColor != nullptr) {
        ObjectNET_Construct(vertexColor);
        At<uint32_t>(vertexColor, 0) = kNiVertexColorPropertyVtable;
        At<uint16_t>(vertexColor, 0x18) = 8;
        ni::sys::Increment(vertexColor + 4);
    }
    At<uint16_t>(vertexColor, 0x18) = uint16_t((At<uint16_t>(vertexColor, 0x18) & 0xFFD7) | 0x10);
    AttachAndRelease(self->screenElements, vertexColor);
    void* material = NiNew(0x5C);
    if (material != nullptr) {
        material = MaterialProperty_Construct(material);
        AddRef(material);
    }
    At<float>(material, 0x1C) = kLabelText[0];   // ambient
    At<float>(material, 0x20) = kLabelText[1];
    At<float>(material, 0x50) = 1.0f;            // alpha
    At<uint32_t>(material, 0x54) += 2;           // (the revision)
    At<float>(material, 0x24) = kLabelText[2];
    At<float>(material, 0x28) = kLabelText[0];   // diffuse
    At<float>(material, 0x2C) = kLabelText[1];
    At<float>(material, 0x30) = kLabelText[2];
    At<uint32_t>(material, 0x54) += 1;
    AttachAndRelease(self->screenElements, material);
    void* alpha = NiNew(0x1C);
    if (alpha != nullptr) {
        alpha = AlphaProperty_Construct(alpha);
        AddRef(alpha);
    }
    At<uint16_t>(alpha, 0x18) = uint16_t((At<uint16_t>(alpha, 0x18) & 0xFEED) | 0xED);
    AttachAndRelease(self->screenElements, alpha);
    Virtual<uint8_t>(self, 9, self->screenElements);   // AttachResources
    ScreenData_UpdateBound(At<void*>(self->screenElements, 0xCC));
    AVObject_UpdateProperties(self->screenElements);
    AVObject_Update(self->screenElements, 0.0f, 1);
}

// NiUIRenderGroup's destructor
void __fastcall RenderGroup_Destruct(NiUIRenderGroup* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiUIRenderGroupVtable);
    ClearRef(self->screenElements);
    uint32_t count = self->children.used;
    for (uint32_t i = 0; i < count; i++) {
        re::Ptr32<NiUIBaseElement>& child = self->children.base[i];
        if (child != nullptr) {
            Release(child);
            child = nullptr;
        }
    }
    ObjectSet_RemoveAll(&self->children);
    PointerArray_Delete(self->children.base);
    UIElement_Destruct(self);
}

const void* __fastcall RenderGroup_GetRTTI(const void*, re::Edx) {
    return reinterpret_cast<const void*>(kNiUIRenderGroupRTTI);
}

// NiUIRenderGroup's constructor (render order): added to the UI manager's render list
NiUIRenderGroup* __fastcall RenderGroup_Construct(NiUIRenderGroup* self, re::Edx, int renderOrder) {
    UIElement_Construct(self, 0);
    self->vtable = reinterpret_cast<void*>(kNiUIRenderGroupVtable);
    self->children.base = nullptr;
    self->children.alloced = 0;
    self->children.used = 0;
    self->renderOrder = uint8_t(renderOrder);
    if (UIManager_Get() != nullptr) {
        NiUIManager* manager = UIManager_Get();
        NiUIRenderGroup* held = self;
        ni::sys::Increment(&self->refCount);
        ChildArray_AddFirstEmpty(&manager->renderGroups, re::PtrCast<const re::Ptr32<void>*>(&held));
        if (ni::sys::Decrement(&self->refCount) == 0) {
            Virtual<void>(self, 1);
        }
    }
    return self;
}

NiUIRenderGroup* __fastcall RenderGroup_DeletingDestructor(NiUIRenderGroup* self, re::Edx, int flags) {
    UIRenderGroup_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0xA8);
    }
    return self;
}

} // namespace

// a lone jmp to NiUILabel::ReformatText (its slot 15): covered by the target's hook
RE_UNHOOKED("NiUILabel::UpdateRect", 0x00833940);

#define NIUIELEM_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(Original_, __LINE__);                                                \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIUIELEM_REVERSED("NiUILabel::NiUILabel", 0x00832F70, &Label_Construct);
NIUIELEM_REVERSED("NiUIBaseElement::IsInside", 0x00833040, &IsInside);
NIUIELEM_REVERSED("NiUILabel::GetRTTI", 0x00833070, &Label_GetRTTI);
NIUIELEM_REVERSED("NiUIBaseElement::MouseLocation", 0x008330B0, &MouseLocation);
NIUIELEM_REVERSED("NiUIBaseElement::SetVisible", 0x00833120, &SetVisible);
NIUIELEM_REVERSED("NiUIBaseElement::SetFocus", 0x008331C0, &SetFocus);
NIUIELEM_REVERSED("NiUILabel::Draw", 0x00833200, &Label_Draw);
NIUIELEM_REVERSED("NiUILabel::~NiUILabel", 0x00833220, &Label_Destruct);
NIUIELEM_REVERSED("NiUILabel::UpdateLayout", 0x008332A0, &Label_UpdateLayout);
NIUIELEM_REVERSED("NiUILabel::UpdateTextPosition", 0x00833350, &Label_UpdateTextPosition);
NIUIELEM_REVERSED("NiUILabel::DeletingDestructor", 0x00833520, &Label_DeletingDestructor);
NIUIELEM_REVERSED("NiUILabel::ReformatText", 0x00833550, &Label_ReformatText);
NIUIELEM_REVERSED("NiUIBaseElement::SetParent", 0x008390E0, &SetParent);
NIUIELEM_REVERSED("NiUIBaseElement::SetDimensions", 0x008390F0, &SetDimensions);
NIUIELEM_REVERSED("NiUIBaseElement::SetOffset", 0x00839120, &SetOffset);
NIUIELEM_REVERSED("NiUIBaseElement::GetOffset", 0x00839150, &GetOffset);
NIUIELEM_REVERSED("NiUIBaseElement::ReinitializeChildren", 0x00839170, &ReinitializeChildren);
NIUIELEM_REVERSED("NiUIBaseElement::GetNSCRect", 0x008391B0, &GetNSCRect);
NIUIELEM_REVERSED("NiUIRenderGroup::UpdateLayout", 0x008391D0, &RenderGroup_UpdateLayout);
NIUIELEM_REVERSED("NiUIBaseElement::UpdateChildLayout", 0x008391E0, &UpdateChildLayout);
NIUIELEM_REVERSED("NiUIBaseElement::InsertQuads", 0x00839220, &InsertQuadsImpl);
NIUIELEM_REVERSED("NiUIBaseElement::ReinitializeDisplayElements", 0x00839270, &ReinitializeDisplayElements);
NIUIELEM_REVERSED("NiUIBaseElement::GetInnerRect", 0x00839350, &GetInnerRectImpl);
NIUIELEM_REVERSED("NiUIBaseElement::GetScaledRect", 0x008394F0, &GetScaledRect);
NIUIELEM_REVERSED("NiUIBaseElement::SetQuad", 0x008396F0, &SetQuadImpl);
NIUIELEM_REVERSED("NiUIBaseElement::HideQuad", 0x008397E0, &HideQuadImpl);
NIUIELEM_REVERSED("NiUIBaseElement::SetNineQuads", 0x00839840, &SetNineQuadsImpl);
NIUIELEM_REVERSED("NiUIBaseElement::HideNineQuads", 0x0083A0B0, &HideNineQuadsImpl);
NIUIELEM_REVERSED("NiUIBaseElement::AttachResources", 0x0083A1B0, &AttachResources);
NIUIELEM_REVERSED("NiUIBaseElement::NiUIBaseElement", 0x0083A260, &Construct);
NIUIELEM_REVERSED("NiUIBaseElement::GetRTTI", 0x0083A330, &GetRTTI);
NIUIELEM_REVERSED("NiUIBaseElement::~NiUIBaseElement", 0x0083A340, &Destruct);
NIUIELEM_REVERSED("NiUIBaseElement::DeletingDestructor", 0x0083A3D0, &DeletingDestructor);
NIUIELEM_REVERSED("NiUIGroup::ReinitializeDisplayElements", 0x0083A400, &Group_ReinitializeDisplayElements);
NIUIELEM_REVERSED("NiUIGroup::GetChildElementCount", 0x0083A4E0, &Group_GetChildElementCount);
NIUIELEM_REVERSED("NiUIGroup::SetExpanded", 0x0083A4F0, &Group_SetExpanded);
NIUIELEM_REVERSED("NiUIGroup::GetFocusedElement", 0x0083A550, &Group_GetFocusedElement);
NIUIELEM_REVERSED("NiUIGroup::UpdateLayout", 0x0083A580, &Group_UpdateLayout);
NIUIELEM_REVERSED("NiUIGroup::EnterPressed", 0x0083A690, &Group_Forward<4>);
NIUIELEM_REVERSED("NiUIGroup::LeftMouseButton", 0x0083A6E0, &Group_Forward<5>);
NIUIELEM_REVERSED("NiUIGroup::SetFocus", 0x0083A730, &Group_SetFocus);
NIUIELEM_REVERSED("NiUIGroup::AdvanceFocus", 0x0083A7B0, &Group_AdvanceFocus);
NIUIELEM_REVERSED("NiUIGroup::ReverseFocus", 0x0083A8A0, &Group_ReverseFocus);
NIUIELEM_REVERSED("NiUIGroup::IsInside", 0x0083A9D0, &Group_IsInside);
NIUIELEM_REVERSED("NiUIGroup::GetChildElement", 0x0083AB00, &Group_GetChildElement);
NIUIELEM_REVERSED("NiUIGroup::UpdateRect", 0x0083AB50, &Group_UpdateRect);
NIUIELEM_REVERSED("NiUIGroup::Draw", 0x0083ACE0, &Group_Draw);
NIUIELEM_REVERSED("NiUIRenderGroup::RemoveChild", 0x0083AD80, &RenderGroup_RemoveChildImpl);
NIUIELEM_REVERSED("NiUIImageButton::SetState", 0x0083AE00, &ImageButton_SetStateImpl);
NIUIELEM_REVERSED("NiUIGroup::RemoveChild", 0x0083AE50, &Group_RemoveChild);
NIUIELEM_REVERSED("NiUIGroup::~NiUIGroup", 0x0083AFE0, &Group_DestructImpl);
NIUIELEM_REVERSED("NiUIGroup::GetRTTI", 0x0083B080, &Group_GetRTTI);
NIUIELEM_REVERSED("NiUIGroup::SetVisible", 0x0083B090, &Group_SetVisible);
NIUIELEM_REVERSED("NiUIGroup::DeletingDestructor", 0x0083B0C0, &Group_DeletingDestructor);
NIUIELEM_REVERSED("NiUIGroup::NiUIGroup", 0x0083B0F0, &Group_Construct);
NIUIELEM_REVERSED("NiUIGroup::AddChild", 0x0083B390, &AddChild<0xB0>);
NIUIELEM_REVERSED("NiUIRenderGroup::Draw", 0x0083B3F0, &RenderGroup_DrawImpl);
NIUIELEM_REVERSED("NiUIRenderGroup::GetChildElement", 0x0083B470, &RenderGroup_GetChildElementImpl);
NIUIELEM_REVERSED("NiUIRenderGroup::InitializeScreenElements", 0x0083B480, &RenderGroup_InitializeScreenElements);
NIUIELEM_REVERSED("NiUIRenderGroup::~NiUIRenderGroup", 0x0083B710, &RenderGroup_Destruct);
NIUIELEM_REVERSED("NiUIRenderGroup::GetRTTI", 0x0083B7C0, &RenderGroup_GetRTTI);
NIUIELEM_REVERSED("NiUIRenderGroup::AddChild", 0x0083B7D0, &AddChild<0x95>);
NIUIELEM_REVERSED("NiUIRenderGroup::NiUIRenderGroup", 0x0083B830, &RenderGroup_Construct);
NIUIELEM_REVERSED("NiUIRenderGroup::DeletingDestructor", 0x0083B8B0, &RenderGroup_DeletingDestructor);

} // namespace ni::sample
