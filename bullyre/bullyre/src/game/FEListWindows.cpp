// The frontend's list windows: FEScrollingSelectionListWindow (a vertical list of items
// that scrolls smoothly, 0x69ABF0..0x69B81E) and FESelectionListWindow (a row or column of
// text keys: the tabs, 0x69B9B0..0x69C7E3). Their small functions are Frontend.cpp's.
//
// FEScrollingSelectionListWindow (Frontend.h's struct; more of it here): +0x20 / +0x2C the
// arrows' positions, +0x44 the time of the last draw (ms, a float), +0x48 scrolling,
// +0x4C the scroll offset, +0x50 / +0x54 scrolling from / to, +0x58 its progress 0 .. 1,
// +0x5C the margin (a share of the height) kept around the selection, +0x60 a nudge
// (-1 .. 1, from the scroll buttons), +0x64 nudged this frame, +0x68 [n] the items.
// FESelectionListWindow: +0x04 its position (anchor, x, y), +0x10 horizontal 0 / vertical 1,
// +0x14 its text style (+0x14 font, +0x18 justification, +0x24 line spacing, +0x28 its
// colour, +0x2C flags), +0x30 / +0x34 the highlighted / greyed colours, +0x38 / +0x44 the
// arrows' positions, +0x50 / +0x54 the arrows' size, +0x58 how many shown, +0x5C the
// separator's space, +0x60 its kind (0 "FE_SEP", else the crest), +0x64 the entries,
// +0x68 the selection, +0x6A the count, +0x6B the first shown, +0x6C focused, +0x6D greyed.

#include <cmath>

#include "game/FEScreensCommon.h"
#include "game/Frontend.h"

#include "game/Crt.h"
using namespace fescr;

namespace {

constexpr uintptr_t VT_FESelectionListWindow = 0x009404B8;

GAME_VAR(uint32_t, g_menuTime, 0x00A14A50);  // the menus' clock (ms)
void* const kButtonMap = reinterpret_cast<void*>(0x00A14810);
GAME_FN(const int32_t*, __thiscall, ButtonMap_GetKeys, 0x00454CD0, void* map, int saved);
GAME_FN(float, __cdecl, Pad_GetAxis, 0x00738840, int axis, int pad);
GAME_FN(void*, __cdecl, Hud_GetArrowTexture, 0x00514EC0, int which);
GAME_FN(void, __cdecl, DrawQuad, 0x0052CCC0, float x1, float y1, float x2, float y2, float x3, float y3, float x4,
        float y4, void* texture, const void* colour, const float* uv1, const float* uv2);
GAME_FN(uint8_t*, __thiscall, RGBA_Construct, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __thiscall, Position_Load, 0x006A4940, void* position, void* archive, const char* name);
GAME_FN(float, __thiscall, Position_GetX, 0x006A2730, const void* position);
GAME_FN(float, __thiscall, Position_GetY, 0x006A2790, const void* position);
GAME_FN(float, __thiscall, Rect_GetX, 0x006A2800, const void* rect);
GAME_FN(float, __thiscall, Rect_GetY, 0x006A2890, const void* rect);
GAME_FN(float, __thiscall, Rect_GetWidth, 0x006A2920, const void* rect);
GAME_FN(float, __thiscall, Rect_GetHeight, 0x006A2940, const void* rect);
GAME_FN(int32_t, __cdecl, ParseColourName, 0x006A70D0, const char* name);
GAME_FN(int32_t, __cdecl, ParseSeparatorName, 0x006A7130, const char* name);
inline int __cdecl StrICmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
GAME_FN(void, __cdecl, FE_SetTextStyle, 0x006A3FC0, int colour, int font, int justify, int);
GAME_FN(float, __cdecl, Font_GetStringWidth, 0x0068A920, const uint16_t* text, int withSpaces);
GAME_FN(float, __cdecl, Font_GetLineHeight, 0x00688220);
GAME_VAR(uint8_t, g_menuFlagsByte, 0x0208B78A);
GAME_ARRAY(uint8_t, g_fontOutlineColour, 0x0208B78B);
GAME_VAR(uint32_t, g_menuOutlineColour, 0x020C333C);
GAME_ARRAY(uint8_t, g_menuColours, 0x020C331C);

GAME_FN(void, __thiscall, Scroll_MeasureHeight, 0x0069B200, void* list);
GAME_FN(void, __thiscall, Scroll_UpdateScroll, 0x0069ACC0, void* list);
GAME_FN(void, __thiscall, Scroll_ScrollToSelection, 0x0069B2A0, void* list);
GAME_FN(void, __thiscall, Scroll_DrawArrows, 0x0069AF30, void* list, float x, float y);
GAME_FN(void, __thiscall, Sel_DrawHorizontalArrows, 0x0069C170, void* list, float x, float y, float width);
GAME_FN(void, __thiscall, Sel_DrawVerticalArrows, 0x0069BF60, void* list);

// A position with a size (the arrows'): anchor, x, y, width, height, 1
struct AnchorRect {
    int32_t anchor;
    float x, y, width, height;
    int32_t one;
};
static_assert(sizeof(AnchorRect) == 0x18);

// the menus' clock (a uint32 as a float), and the ms since `since`, truncated
float ClockNow() {
    return float(g_menuTime);
}
uint32_t MsSince(float since) {
    return uint32_t(int64_t(ClockNow() - since));
}

// a scrolling list's item (Frontend.h's FEListItem): its extent and drawing
float ItemHeight(FEListItem* item, float* rect, float x, float width) {
    VCall<void>(item, 0, rect, x, width);
    return std::fabs(rect[3] - rect[1]);
}
FEListItem* GetItem(FEScrollingSelectionListWindow* list, int index) {
    return VCall<FEListItem*>(list, 4, index);
}

// a white arrow quad at (x, y) the size of the anchored rect
void DrawArrowQuad(void* texture, float left, float top, float right, float bottom) {
    float uv2[2] = {1.0f, 1.0f};
    float uv1[2] = {0.0f, 0.0f};
    uint8_t white[4];
    RGBA_Construct(white, 255, 255, 255, 255);
    DrawQuad(left, top, right, top, left, bottom, right, bottom, texture, white, uv1, uv2);
}

// the selection list's field by offset
template <typename T>
T& S(FESelectionListWindow* list, int offset) {
    return At<T>(list, offset);
}

} // namespace

namespace felw {

// ---- FEScrollingSelectionListWindow ----------------------------------------------------------

// 0x69ABF0: Load
void __fastcall Scroll_Load(FEScrollingSelectionListWindow* self, re::Edx, void* archive) {
    struct {
        re::Ptr32<char> data;
        int32_t size;
        char buffer[0x81];
    } unused;  // (set up, never read)
    unused.data = unused.buffer;
    unused.size = 0x81;
    unused.buffer[0] = 0;
    (void)unused;
    VCall<void>(archive, 0x28, &self->x, "SelListXPos");
    VCall<void>(archive, 0x28, &self->y, "SelListYPos");
    VCall<void>(archive, 0x28, &self->width, "SelListWidth");
    VCall<void>(archive, 0x28, &self->height, "SelListHeight");
    Position_Load(Ptr(self, 0x20), archive, "UpOrLeftArrowPosInfo");
    Position_Load(Ptr(self, 0x2C), archive, "DownOrRightArrowPosInfo");
    VCall<void>(archive, 0x28, &self->arrowWidth, "ArrowWidth");
    VCall<void>(archive, 0x28, &self->arrowHeight, "ArrowHeight");
    VCall<void>(archive, 0x28, &self->scrollThreshold, "ScrollWindowThresholdPct");
}

// 0x69ACC0: the scroll eased toward its target (0.005 a ms, ease-out), the nudge fading
// (0.002 a ms) and moving the offset, kept within the list
void __fastcall Scroll_UpdateScrollImpl(FEScrollingSelectionListWindow* self, re::Edx) {
    float& offset = At<float>(self, 0x4C);
    float& nudge = At<float>(self, 0x60);
    if (At<uint8_t>(self, 0x48) != 0) {
        uint32_t ms = MsSince(At<float>(self, 0x44));
        float t = float(double(float(ms)) * 0.004999999888241291) + At<float>(self, 0x58);
        At<float>(self, 0x58) = t;
        if (1.0f < t) {
            At<uint8_t>(self, 0x48) = 0;
            At<float>(self, 0x50) = At<float>(self, 0x54);
            offset = At<float>(self, 0x54);
        } else {
            float a = 1.0f - t;
            float eased = 1.0f - a * a;
            offset = (At<float>(self, 0x54) - At<float>(self, 0x50)) * eased + At<float>(self, 0x50);
        }
    }
    if (!(std::fabs(nudge - 0.0f) > 1e-5f)) {
        return;
    }
    if (At<uint8_t>(self, 0x64) == 0) {
        uint32_t ms = MsSince(At<float>(self, 0x44));
        if (0.0f < nudge) {
            float step = float(double(float(ms)) * 0.001);
            nudge = nudge - (step + step);
            if (nudge < 0.0f) {
                nudge = 0.0f;
            }
        } else if (0.0f > nudge) {
            float step = float(double(float(ms)) * 0.001);
            nudge = step + step + nudge;
            if (nudge > 0.0f) {
                nudge = 0.0f;
            }
        }
    }
    float limit = -(At<float>(self, 0x40) - self->height);
    if (!(limit < 0.0f)) {
        return;
    }
    if (0.0f > nudge) {
        float c = nudge + 1.0f;
        float e = 1.0f - c * c;
        offset = float(double(e) * 10.0) + offset;
    } else if (0.0f < nudge) {
        float f = 1.0f - nudge;
        float g = 1.0f - f * f;
        offset = offset - float(double(g) * 10.0);
    }
    if (0.0f < offset) {
        nudge = 0.0f;
        offset = 0.0f;
        return;
    }
    if (offset < limit) {
        nudge = 0.0f;
        offset = limit;
    }
}

// 0x69AF30: the up / down arrows (when the list is taller than its window and there's
// more that way)
void __fastcall Scroll_DrawArrowsImpl(FEScrollingSelectionListWindow* self, re::Edx, float x, float y) {
    if (!(self->height < At<float>(self, 0x40))) {
        return;
    }
    float offset = At<float>(self, 0x4C);
    if (std::fabs(offset - 0.0f) > 0.5f) {
        void* texture = Hud_GetArrowTexture(2);
        AnchorRect rect = {0, 0.0f, 0.0f, self->arrowWidth, self->arrowHeight, 1};
        float left = At<float>(self, 0x24) + x + Rect_GetX(&rect);
        float top = At<float>(self, 0x28) + y + Rect_GetY(&rect);
        float width = Rect_GetWidth(&rect);
        float height = Rect_GetHeight(&rect);
        DrawArrowQuad(texture, left, top, width + left, height + top);
    }
    float limit = -(At<float>(self, 0x40) - self->height);
    if (std::fabs(At<float>(self, 0x4C) - limit) > 0.5f) {
        void* texture = Hud_GetArrowTexture(3);
        AnchorRect rect = {0, 0.0f, 0.0f, self->arrowWidth, self->arrowHeight, 1};
        float left = At<float>(self, 0x30) + x + Rect_GetX(&rect);
        float top = At<float>(self, 0x34) + y + Rect_GetY(&rect);
        float width = Rect_GetWidth(&rect);
        float height = Rect_GetHeight(&rect);
        DrawArrowQuad(texture, left, top, width + left, height + top);
    }
}

// 0x69B200: the items' total height (+100), measured once (-1: to measure)
void __fastcall Scroll_MeasureHeightImpl(FEScrollingSelectionListWindow* self, re::Edx) {
    if (!(self->totalHeight < 0.0f)) {
        return;
    }
    int32_t count = self->count;
    for (int i = 0; i < count; i++) {
        FEListItem* item = GetItem(self, i);
        float rect[4] = {1e6f, -1e6f, -1e6f, 1e6f};
        VCall<void>(item, 0, rect, self->x, self->width);
        self->totalHeight = std::fabs(rect[3] - rect[1]) + self->totalHeight;
    }
    self->totalHeight = float(double(self->totalHeight) + 100.0);
}

// 0x69B2A0: the scroll target that keeps the selection inside the margins (within the
// list), eased to
void __fastcall Scroll_ScrollToSelectionImpl(FEScrollingSelectionListWindow* self, re::Edx) {
    float margin = self->scrollThreshold * self->height;
    float topLimit = self->y + margin;
    float bottomLimit = (self->y + self->height) - margin;
    float top = self->y + At<float>(self, 0x4C);
    float bottom = margin;  // (an empty list compares the margin)
    float rect[4] = {1e6f, -1e6f, -1e6f, 1e6f};
    for (int i = 0; i < self->count; i++) {
        FEListItem* item = GetItem(self, i);
        VCall<void>(item, 0, rect, self->x, self->width);
        float height = std::fabs(rect[3] - rect[1]);
        bottom = height;
        float next = height + top;
        if (i == self->selected) {
            bottom = next;
            break;
        }
        top = next;
    }
    float& target = At<float>(self, 0x54);
    float limit;
    float wanted;
    if (topLimit > top) {
        wanted = (topLimit - top) + At<float>(self, 0x4C);
        limit = -(At<float>(self, 0x40) - self->height);
    } else if (bottomLimit < bottom) {
        wanted = At<float>(self, 0x4C) - (bottom - bottomLimit);
        limit = -(At<float>(self, 0x40) - self->height);
    } else {
        goto check;
    }
    if (limit > wanted) {
        target = limit;
    } else if (wanted > 0.0f) {
        target = 0.0f;
    } else {
        target = wanted;
    }
check:
    if (!(std::fabs(target - At<float>(self, 0x4C)) > 1e-5f)) {
        return;
    }
    if (At<uint8_t>(self, 0x48) == 0) {
        At<float>(self, 0x58) = 0.0f;
    }
    At<uint8_t>(self, 0x48) = 1;
    At<float>(self, 0x50) = At<float>(self, 0x4C);
}

// the scroll buttons (up: key slot 13 / 0x105 with axis 13; down: slot 11 / 0x104, axis
// 11): 7 times as fast while the axis is held
static int NudgeSpeed(int slot, int32_t key, int axis) {
    const int32_t* keys = ButtonMap_GetKeys(kButtonMap, 0);
    if (keys[slot] == key && Pad_GetAxis(axis, GetActivePad()) != 0.0f) {
        return 7;
    }
    return 1;
}

// 0x69B480: nudged down (toward the end); true while it can still scroll
uint8_t __fastcall Scroll_NudgeDown(FEScrollingSelectionListWindow* self, re::Edx) {
    Scroll_MeasureHeight(self);
    uint32_t ms = MsSince(At<float>(self, 0x44));
    int speed = NudgeSpeed(13, 0x105, 0xD);
    float& nudge = At<float>(self, 0x60);
    float step = float(double(float(ms)) * 0.001);
    nudge = step * float(speed) + nudge;
    if (1.0f < nudge) {
        nudge = 1.0f;
    }
    At<uint8_t>(self, 0x64) = 1;
    float limit = -(At<float>(self, 0x40) - self->height);
    if (!(limit < 0.0f)) {
        return 0;
    }
    return std::fabs(At<float>(self, 0x4C) - limit) > 0.5f ? 1 : 0;
}

// 0x69B5A0: nudged up; true while it can still scroll
uint8_t __fastcall Scroll_NudgeUp(FEScrollingSelectionListWindow* self, re::Edx) {
    Scroll_MeasureHeight(self);
    uint32_t ms = MsSince(At<float>(self, 0x44));
    int speed = NudgeSpeed(11, 0x104, 0xB);
    float& nudge = At<float>(self, 0x60);
    float step = float(double(float(ms)) * 0.001);
    nudge = nudge - step * float(speed);
    if (nudge < -1.0f) {
        nudge = -1.0f;
    }
    At<uint8_t>(self, 0x64) = 1;
    return std::fabs(At<float>(self, 0x4C) - 0.0f) > 0.5f ? 1 : 0;
}

// 0x69B6B0: the items inside the window (or the first when `forceFirst`), the arrows, the
// clock noted
void __fastcall Scroll_Draw(FEScrollingSelectionListWindow* self, re::Edx, float dx, float dy, int forceFirst) {
    float x = self->x + dx;
    float top = self->y + dy;
    Scroll_MeasureHeight(self);
    Scroll_UpdateScroll(self);
    int32_t count = self->count;
    float rect[4] = {1e6f, -1e6f, -1e6f, 1e6f};
    float y = top + At<float>(self, 0x4C);
    for (int i = 0; i < count; i++) {
        FEListItem* item = GetItem(self, i);
        VCall<void>(item, 0, rect, x, self->width);
        float height = std::fabs(rect[3] - rect[1]);
        float bottomLimit = self->height + top;
        bool inside = !(top > y) && !(bottomLimit < height + y);
        if (inside || (re::Flag(forceFirst) && i == 0)) {
            VCall<void>(item, 4, x, y, self->width, self->height, i == self->selected ? 1 : 0);
        }
        y = y + height;
    }
    Scroll_DrawArrows(self, x, top);
    At<uint8_t>(self, 0x64) = 0;
    At<float>(self, 0x44) = ClockNow();
}

// 0x69B830: the next selectable item (round); -1: none selectable
int __fastcall Scroll_Next(FEScrollingSelectionListWindow* self, re::Edx) {
    if (self->unknown08 < 1) {
        return -1;
    }
    Scroll_MeasureHeight(self);
    int32_t before = self->selected;
    do {
        self->selected += 1;
        if (self->selected >= self->count) {
            self->selected = 0;
        }
    } while (GetItem(self, self->selected)->selectable == 0);
    if (before != self->selected) {
        Scroll_ScrollToSelection(self);
    }
    return self->selected;
}

// 0x69B890: the previous one
int __fastcall Scroll_Previous(FEScrollingSelectionListWindow* self, re::Edx) {
    if (self->unknown08 < 1) {
        return -1;
    }
    Scroll_MeasureHeight(self);
    int32_t before = self->selected;
    do {
        self->selected -= 1;
        if (self->selected < 0) {
            self->selected = self->count - 1;
        }
    } while (GetItem(self, self->selected)->selectable == 0);
    if (before != self->selected) {
        Scroll_ScrollToSelection(self);
    }
    return self->selected;
}

// ---- FESelectionListWindow -----------------------------------------------------------------

// the entries shown's total width (horizontal: texts, separators between)
static float ShownWidth(FESelectionListWindow* self, float separatorWidth) {
    float total = 0.0f;
    if (S<int32_t>(self, 0x10) != 0) {
        return total;
    }
    for (int i = 0; i < self->visibleCount; i++) {
        int index = self->firstVisible + i;
        if (index >= self->count) {
            break;
        }
        total = Font_GetStringWidth(Text_GetByHash(kTheText, self->entries[index]), 1) + total;
        if (i != 0) {
            float space = S<float>(self, 0x5C);
            total = space + space + separatorWidth + total;
        }
    }
    return total;
}

static const char* SeparatorKey(FESelectionListWindow* self) {
    return S<int32_t>(self, 0x60) == 0 ? "FE_SEP" : "FE_BULLY_CREST_FE";
}

// 0x69B9B0: where the entries start (horizontal: centred on the position)
void __fastcall Sel_GetStart(FESelectionListWindow* self, re::Edx, float* out) {
    float cx = Position_GetX(Ptr(self, 4));
    float cy = Position_GetY(Ptr(self, 4));
    int justify = S<uint8_t>(self, 0x6C) != 0 ? 5 : S<int32_t>(self, 0x18);
    FE_SetTextStyle(0xB, S<int32_t>(self, 0x14), justify, 0);
    float separator = Font_GetStringWidth(Text_Get(kTheText, SeparatorKey(self)), 0);
    float total = ShownWidth(self, separator);
    out[0] = cx - float(double(total) * 0.5);
    out[1] = cy;
}

// 0x69BAB0
uint32_t __fastcall Sel_GetEntry(FESelectionListWindow* self, re::Edx, int index) {
    return self->entries[index];
}

// The two loaders (0x69BAF0 plain, 0x69BC80 with a prefix on every key)
template <bool Prefixed>
static void SelLoad(FESelectionListWindow* self, void* archive, const char* prefix) {
    char key[0x40];
    auto name = [&](const char* base) -> const char* {
        if (!Prefixed) {
            return base;
        }
        Sprintf(key, "%s%s", prefix, base);
        return key;
    };
    struct {
        re::Ptr32<char> data;
        int32_t size;
        char buffer[0x81];
    } value;
    value.data = value.buffer;
    value.size = 0x81;
    value.buffer[0] = 0;
    Position_Load(Ptr(self, 4), archive, name("SelListPosInfo"));
    VCall<void>(archive, 0x24, &value, 0x81, name("SelListOrientation"));
    if (StrICmp(value.data, "horizontal") == 0) {
        S<int32_t>(self, 0x10) = 0;
    } else if (StrICmp(value.data, "vertical") == 0) {
        S<int32_t>(self, 0x10) = 1;
    }
    TextStyle_Load(Ptr(self, 0x14), archive, name("ItemStyle"));
    VCall<void>(archive, 0x24, &value, 0x81, name("ItemHighlightedColour"));
    S<int32_t>(self, 0x30) = ParseColourName(value.data);
    VCall<void>(archive, 0x24, &value, 0x81, name("ItemGreyedColour"));
    S<int32_t>(self, 0x34) = ParseColourName(value.data);
    Position_Load(Ptr(self, 0x38), archive, name("UpOrLeftArrowPosInfo"));
    Position_Load(Ptr(self, 0x44), archive, name("DownOrRightArrowPosInfo"));
    VCall<void>(archive, 0x28, Ptr(self, 0x50), name("ArrowWidth"));
    VCall<void>(archive, 0x28, Ptr(self, 0x54), name("ArrowHeight"));
    int32_t shown;
    VCall<void>(archive, 0x38, &shown, name("NumItemsDisplayed"));
    self->visibleCount = int8_t(shown);
    VCall<void>(archive, 0x28, Ptr(self, 0x5C), name("SeparatorSpace"));
    VCall<void>(archive, 0x24, &value, 0x81, name("SeparatorType"));
    S<int32_t>(self, 0x60) = ParseSeparatorName(value.data);
}

void __fastcall Sel_Load(FESelectionListWindow* self, re::Edx, void* archive) {
    SelLoad<false>(self, archive, nullptr);
}
void __fastcall Sel_LoadPrefixed(FESelectionListWindow* self, re::Edx, void* archive, const char* prefix) {
    SelLoad<true>(self, archive, prefix);
}

// 0x69BF60 (not in the function list; reached by a jmp): a vertical list's arrows
void __fastcall Sel_DrawVerticalArrowsImpl(FESelectionListWindow* self, re::Edx) {
    void* up = Hud_GetArrowTexture(2);
    void* down = Hud_GetArrowTexture(3);
    if (self->firstVisible > 0) {
        AnchorRect rect = {S<int32_t>(self, 0x38), S<float>(self, 0x3C), S<float>(self, 0x40), S<float>(self, 0x50),
                           S<float>(self, 0x54), 1};
        float left = Rect_GetX(&rect);
        float top = Rect_GetY(&rect);
        float right = Rect_GetWidth(&rect) + left;
        float bottom = Rect_GetHeight(&rect) + top;
        DrawArrowQuad(up, left, top, right, bottom);
    }
    if (self->firstVisible < int(self->count) - self->visibleCount) {
        AnchorRect rect = {S<int32_t>(self, 0x44), S<float>(self, 0x48), S<float>(self, 0x4C), S<float>(self, 0x50),
                           S<float>(self, 0x54), 1};
        float left = Rect_GetX(&rect);
        float top = Rect_GetY(&rect);
        float right = Rect_GetWidth(&rect) + left;
        float bottom = Rect_GetHeight(&rect) + top;
        DrawArrowQuad(down, left, top, right, bottom);
    }
}

// 0x69C170: a horizontal list's arrows either side of its entries
void __fastcall Sel_DrawHorizontalArrowsImpl(FESelectionListWindow* self, re::Edx, float x, float y, float width) {
    void* left = Hud_GetArrowTexture(0);
    void* right = Hud_GetArrowTexture(1);
    if (self->firstVisible != 0) {
        AnchorRect rect = {S<int32_t>(self, 0x38), S<float>(self, 0x3C), S<float>(self, 0x40), S<float>(self, 0x50),
                           S<float>(self, 0x54), 1};
        float ax = Rect_GetX(&rect) + x;
        float ay = Rect_GetY(&rect) + y;
        float w = Rect_GetWidth(&rect);
        float h = Rect_GetHeight(&rect);
        DrawArrowQuad(left, ax, ay, w + ax, h + ay);
    }
    if (self->firstVisible < int(self->count) - self->visibleCount) {
        AnchorRect rect = {S<int32_t>(self, 0x44), S<float>(self, 0x48), S<float>(self, 0x4C), S<float>(self, 0x50),
                           S<float>(self, 0x54), 1};
        float ax = x + width + Rect_GetX(&rect);
        float ay = Rect_GetY(&rect) + y;
        float w = Rect_GetWidth(&rect);
        float h = Rect_GetHeight(&rect);
        DrawArrowQuad(right, ax, ay, w + ax, h + ay);
    }
}

// 0x69C3A0: the constructor (the entries' storage and capacity given)
void* __fastcall Sel_Construct(FESelectionListWindow* self, re::Edx, uint32_t* entries, int capacity) {
    SetVtable(self, VT_FESelectionListWindow);
    uint32_t* d = reinterpret_cast<uint32_t*>(self);
    d[2] = 0;
    d[3] = 0;
    d[1] = 0;
    d[7] = 0;
    d[8] = 0;
    d[5] = 0;
    d[9] = 0;
    d[6] = 3;
    d[10] = 2;
    S<uint8_t>(self, 0x2C) = 1;
    S<uint8_t>(self, 0x2D) = 0;
    S<uint8_t>(self, 0x2E) = 0;
    d[0xF] = 0;
    d[0x10] = 0;
    d[0xE] = 0;
    d[0x12] = 0;
    d[0x11] = 0;
    d[0x13] = 0;
    self->entries = entries;
    self->unknown69 = uint8_t(capacity);
    self->count = 0;
    self->selected = 0;
    self->firstVisible = 0;
    self->unknown6D = 0;
    return self;
}

// 0x69C410: the entries shown (the selection highlighted with a drop shadow, separators
// between horizontal ones), then the arrows
void __fastcall Sel_Draw(FESelectionListWindow* self, re::Edx) {
    float startX = Position_GetX(Ptr(self, 4));
    float cy = Position_GetY(Ptr(self, 4));
    float previousWidth = 0.0f;
    int justify = S<uint8_t>(self, 0x6C) != 0 ? 5 : S<int32_t>(self, 0x18);
    FE_SetTextStyle(0xB, S<int32_t>(self, 0x14), justify, 1);
    float separatorWidth = Font_GetStringWidth(Text_Get(kTheText, SeparatorKey(self)), 0);
    float total = ShownWidth(self, separatorWidth);
    startX = startX - float(double(total) * 0.5);
    float x = startX;
    float y = cy;
    for (int i = 0; i < self->visibleCount; i++) {
        int index = self->firstVisible + i;
        // (<=: one past the count is drawn too)
        if (index > self->count) {
            break;
        }
        int colour = S<int32_t>(self, 0x28);
        if (self->selected == index) {
            colour = S<int32_t>(self, 0x30);
        } else if (self->unknown6D != 0) {
            colour = S<int32_t>(self, 0x34);
        }
        const uint16_t* text = Text_GetByHash(kTheText, self->entries[index]);
        if (S<int32_t>(self, 0x10) == 1) {
            y = (Font_GetLineHeight() + S<float>(self, 0x24)) * float(i) + cy;
        } else {
            x = x + previousWidth;
            previousWidth = Font_GetStringWidth(text, 1);
        }
        if (S<int32_t>(self, 0x10) == 0 && i != 0) {
            FE_SetTextStyle(0xB, S<int32_t>(self, 0x14), justify, 0);
            const uint16_t* separator = Text_Get(kTheText, SeparatorKey(self));
            Font_PrintString(S<float>(self, 0x5C) + x, y, separator, 1);
            float space = S<float>(self, 0x5C);
            x = (space + space + separatorWidth) + x;
        }
        FE_SetTextStyle(colour, S<int32_t>(self, 0x14), justify, 0);
        if (self->selected == index) {
            uint8_t shadow[4];
            RGBA_Construct(shadow, 0, 0, 0, 0xA0);
            g_fontDropColour[0] = g_menuColours[0];
            g_fontDrop = 1;
            g_fontDropColour[1] = g_menuColours[1];
            g_fontDropColour[2] = g_menuColours[2];
            g_fontDropColour[3] = g_menuColours[3];
            if (S<uint8_t>(self, 0x2D) != 0) {
                g_menuFlagsByte |= 0x40;
                uint32_t outline = g_menuOutlineColour;
                g_fontOutlineColour[0] = uint8_t(outline);
                g_fontOutlineColour[1] = uint8_t(outline >> 8);
                g_fontOutlineColour[2] = uint8_t(outline >> 16);
                g_fontOutlineColour[3] = uint8_t(outline >> 24);
            }
        }
        if (S<uint8_t>(self, 0x6C) != 0) {
            uint8_t shadow[4];
            RGBA_Construct(shadow, 0, 0, 0, 0xA0);
            g_fontDropColour[0] = g_menuColours[0];
            g_fontDrop = 1;
            g_fontDropColour[1] = g_menuColours[1];
            g_fontDropColour[2] = g_menuColours[2];
            g_fontDropColour[3] = g_menuColours[3];
        }
        if (S<uint8_t>(self, 0x2C) != 0) {
            g_menuFlagsByte |= 0x10;
        }
        Font_PrintString(x, y, text, 1);
        if (self->selected == index && S<uint8_t>(self, 0x6C) != 0) {
            g_menuFlagsByte &= 0xBF;
        }
        if (S<uint8_t>(self, 0x2C) != 0) {
            g_menuFlagsByte &= 0xEF;
        }
        Font_Render();
    }
    if (S<int32_t>(self, 0x10) == 1) {
        Sel_DrawVerticalArrows(self);
        return;
    }
    Sel_DrawHorizontalArrows(self, startX, y, total);
}

} // namespace felw

#define FELW_REVERSED(name, address, fn)                                                          \
    static void* RE_CONCAT(felw_original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(felw_original_, __LINE__), \
             re::Mode::Original)

FELW_REVERSED("FEScrollingSelectionListWindow::Load", 0x0069ABF0, &felw::Scroll_Load);
FELW_REVERSED("FEScrollingSelectionListWindow::UpdateScroll", 0x0069ACC0, &felw::Scroll_UpdateScrollImpl);
FELW_REVERSED("FEScrollingSelectionListWindow::DrawArrows", 0x0069AF30, &felw::Scroll_DrawArrowsImpl);
FELW_REVERSED("FEScrollingSelectionListWindow::MeasureHeight", 0x0069B200, &felw::Scroll_MeasureHeightImpl);
FELW_REVERSED("FEScrollingSelectionListWindow::ScrollToSelection", 0x0069B2A0, &felw::Scroll_ScrollToSelectionImpl);
FELW_REVERSED("FEScrollingSelectionListWindow::NudgeDown", 0x0069B480, &felw::Scroll_NudgeDown);
FELW_REVERSED("FEScrollingSelectionListWindow::NudgeUp", 0x0069B5A0, &felw::Scroll_NudgeUp);
FELW_REVERSED("FEScrollingSelectionListWindow::Draw", 0x0069B6B0, &felw::Scroll_Draw);
FELW_REVERSED("FEScrollingSelectionListWindow::Next", 0x0069B830, &felw::Scroll_Next);
FELW_REVERSED("FEScrollingSelectionListWindow::Previous", 0x0069B890, &felw::Scroll_Previous);
FELW_REVERSED("FESelectionListWindow::GetStart", 0x0069B9B0, &felw::Sel_GetStart);
FELW_REVERSED("FESelectionListWindow::GetEntry", 0x0069BAB0, &felw::Sel_GetEntry);
FELW_REVERSED("FESelectionListWindow::Load", 0x0069BAF0, &felw::Sel_Load);
FELW_REVERSED("FESelectionListWindow::LoadPrefixed", 0x0069BC80, &felw::Sel_LoadPrefixed);
FELW_REVERSED("FESelectionListWindow::DrawVerticalArrows", 0x0069BF60, &felw::Sel_DrawVerticalArrowsImpl);
FELW_REVERSED("FESelectionListWindow::DrawHorizontalArrows", 0x0069C170, &felw::Sel_DrawHorizontalArrowsImpl);
FELW_REVERSED("FESelectionListWindow::FESelectionListWindow", 0x0069C3A0, &felw::Sel_Construct);
FELW_REVERSED("FESelectionListWindow::Draw", 0x0069C410, &felw::Sel_Draw);
