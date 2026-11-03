// NiUIImageButton (vtable 0x989C78, 0xC0, NiRTTI 0x20E5B78): a two-state button drawn as one skin
// image, highlighted while it has the focus or the mouse is over it; Enter / the left button
// (released) flips its state and signals it (NiUIGroup's expand button). The images: state set
// +0xB0 (normal) / +0xB4 (highlighted), state clear +0xB8 / +0xBC (0x2B: none). Slots as
// NiUIBaseElement's (NiSampleFramework.h): 4 / 5 EnterPressed / LeftMouseButton, 6 SetFocus,
// 7 SetVisible (the base's, through a jmp), 8 MouseLocation, 17 UpdateLayout.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSampleFramework.h"
#include "reversed.h"

namespace ni::sample {
namespace {

// the button's own fields (a local name: the class is only reached by offset elsewhere)
struct ImageButtonLayout : NiUIBaseElement {
    NiUISignal stateChanged;   // +0x98 (NiUISignal1<bool>; the folded vtable 0x989430)
    uint8_t state;             // +0xA8
    uint8_t padA9[3];
    uint32_t highlighted;      // +0xAC (0 / 1: picks the image)
    uint32_t images[4];        // +0xB0 (set: normal, highlighted; clear: normal, highlighted)
};
static_assert(offsetof(ImageButtonLayout, stateChanged) == 0x98);
static_assert(offsetof(ImageButtonLayout, state) == 0xA8);
static_assert(offsetof(ImageButtonLayout, highlighted) == 0xAC);
static_assert(offsetof(ImageButtonLayout, images) == 0xB0);
static_assert(sizeof(ImageButtonLayout) == 0xC0);

constexpr uint32_t kNiUIImageButtonVtable = 0x00989C78;
constexpr uintptr_t kNiUIImageButtonRTTI = 0x020E5B78;
constexpr uint32_t kNoImage = 0x2B;
constexpr int kAtlasMap = 0x9C;     // NiUIManager's NiUIAtlasMap
GAME_ARRAY(float, kSkinColor, 0x00BC75E4);

GAME_FN(float*, __cdecl, GetScaledRect, 0x008394F0, float* result, uint32_t index, const float* rect);
GAME_FN(uint8_t, __cdecl, SetQuad, 0x008396F0, void* screenElements, int32_t quad, const float* rect,
        const float* uv, const float* color);
GAME_FN(uint8_t, __cdecl, HideQuad, 0x008397E0, void* screenElements, uint32_t quad);
GAME_FN(uint8_t, __thiscall, UIElement_MouseLocation, 0x008330B0, NiUIBaseElement* self, uint32_t x, uint32_t y);
GAME_FN(void, __thiscall, ImageButton_SetState, 0x0083AE00, ImageButtonLayout* self, int state);

inline uint32_t CurrentImage(const ImageButtonLayout* self) {
    return self->state != 0 ? self->images[self->highlighted] : self->images[2 + self->highlighted];
}

// 0x83C550 UpdateLayout: its image (the atlas' texture rect) fitted in its rect, or its quad
// hidden while it isn't visible; then the children's layout (slot 18)
void __fastcall UpdateLayout(ImageButtonLayout* self, re::Edx) {
    if (self->screenElements == nullptr || UIManager_Get() == nullptr) {
        return;
    }
    if (self->visible != 0) {
        uint8_t* atlas = reinterpret_cast<uint8_t*>(UIManager_Get()) + kAtlasMap;
        uint32_t image = CurrentImage(self);
        const float* stored = reinterpret_cast<const float*>(atlas + 8 + image * 16);
        float uv[4] = {stored[0], stored[1], stored[2], stored[3]};
        float rect[4];
        GetScaledRect(rect, image, self->nscRect);
        SetQuad(self->screenElements, int32_t(self->baseQuadIndex), rect, uv, kSkinColor);
    } else {
        HideQuad(self->screenElements, self->baseQuadIndex);
    }
    Virtual<void>(self, 18);   // UpdateChildLayout
}

// 0x83C640 SetFocus(focus): the focus signals (if it takes focus), the highlight following it
uint8_t __fastcall SetFocus(ImageButtonLayout* self, re::Edx, int focus) {
    if (UIManager_Get() == nullptr) {
        return 0;
    }
    uint8_t on = uint8_t(focus);
    if (self->canTakeFocus != 0) {
        Signal0_Emit(on != 0 ? &self->focusGained : &self->focusLost);
        self->focused = on;
    }
    if (on != 0) {
        if (self->highlighted == 0) {
            self->highlighted = 1;
            Virtual<void>(self, 17);
        }
    } else if (self->highlighted == 1) {
        self->highlighted = 0;
        Virtual<void>(self, 17);
    }
    return 1;
}

// 0x83C6C0 MouseLocation(x, y): the base's, highlighted when the mouse is over it (the highlight
// only goes when the focus does)
uint8_t __fastcall MouseLocation(ImageButtonLayout* self, re::Edx, uint32_t x, uint32_t y) {
    if (UIManager_Get() == nullptr) {
        return 0;
    }
    uint8_t inside = UIElement_MouseLocation(self, x, y);
    if (inside != 0 && self->highlighted == 0) {
        self->highlighted = 1;
        Virtual<void>(self, 17);
    }
    return inside;
}

// 0x83C710 EnterPressed / LeftMouseButton(state): on a release (2) the state flips
void __fastcall Press(ImageButtonLayout* self, re::Edx, int state) {
    if ((uint8_t(state) & 2) != 0) {
        ImageButton_SetState(self, self->state == 0 ? 1 : 0);
    }
}

// 0x83C730: focusable, state clear, no images
ImageButtonLayout* __fastcall Construct(ImageButtonLayout* self, re::Edx) {
    UIElement_Construct(self, 1);
    self->vtable = reinterpret_cast<void*>(kNiUIImageButtonVtable);
    self->stateChanged.vtable = reinterpret_cast<void*>(kNiUISignalVtable);
    self->stateChanged.set.base = nullptr;
    self->stateChanged.set.alloced = 0;
    self->stateChanged.set.used = 0;
    self->state = 0;
    self->highlighted = 0;
    self->images[0] = kNoImage;
    self->images[2] = kNoImage;
    self->images[1] = kNoImage;
    self->images[3] = kNoImage;
    return self;
}

// 0x83C790
const void* __fastcall GetRTTI(const void*, re::Edx) {
    return reinterpret_cast<const void*>(kNiUIImageButtonRTTI);
}

// 0x83C7B0: the deleting destructor (the signal let go of its slots, then the base)
ImageButtonLayout* __fastcall DeletingDestructor(ImageButtonLayout* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(kNiUIImageButtonVtable);
    Signal_Destruct(&self->stateChanged);
    UIElement_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(ImageButtonLayout));
    }
    return self;
}

} // namespace

// slot 7: a lone jmp to NiUIBaseElement::SetVisible (0x833120), covered by its hook
RE_UNHOOKED("NiUIImageButton::SetVisible", 0x0083C7A0);

#define NIUIIMGBTN_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                                \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIUIIMGBTN_REVERSED("NiUIImageButton::UpdateLayout", 0x0083C550, &UpdateLayout);
NIUIIMGBTN_REVERSED("NiUIImageButton::SetFocus", 0x0083C640, &SetFocus);
NIUIIMGBTN_REVERSED("NiUIImageButton::MouseLocation", 0x0083C6C0, &MouseLocation);
NIUIIMGBTN_REVERSED("NiUIImageButton::EnterPressed", 0x0083C710, &Press);   // (slot 5 too)
NIUIIMGBTN_REVERSED("NiUIImageButton::NiUIImageButton", 0x0083C730, &Construct);
NIUIIMGBTN_REVERSED("NiUIImageButton::GetRTTI", 0x0083C790, &GetRTTI);
NIUIIMGBTN_REVERSED("NiUIImageButton::DeletingDestructor", 0x0083C7B0, &DeletingDestructor);

} // namespace ni::sample
