// NiUIManager, the sample framework's UI singleton (vtable 0x9893C4, 0x6F0, at 0x20E5A3C; no
// NiRTTI: Gamebryo 2.x's name): the UI skin's atlas and font, the input bindings it dispatches
// every frame (the mouse buttons, keyboard keys and game pad buttons reserved by slots, with a
// down / pressed / released mask), the UI groups in focus and render order, the mouse picking
// the focused group, and the conversions between normalized screen space (within the renderer's
// safe area) and pixels. Created by NiSample::CreateUISystem, drawn by NiUIManagerRenderClick.

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
GAME_FN(void, __thiscall, Renderer_ConvertFromNDCToPixels, 0x00755810, void* self, float x, float y, uint32_t* px,
        uint32_t* py);
GAME_FN(NiRefObject*, __cdecl, NiFont_Create, 0x008D8150, void* renderer, const char* name);
GAME_FN(void, __thiscall, NiFont_GetCharExtent, 0x008D7240, NiRefObject* self, uint32_t c, float* width,
        float* height);
GAME_FN(uint8_t, __thiscall, GamePad_ButtonIsDown, 0x008D0AC0, void* self, uint32_t button, uint32_t* modifiers);
GAME_FN(uint8_t, __thiscall, GamePad_ButtonWasPressed, 0x008D0B10, void* self, uint32_t button, uint32_t* modifiers);
GAME_FN(uint8_t, __thiscall, GamePad_ButtonWasReleased, 0x008D0B80, void* self, uint32_t button,
        uint32_t* modifiers);
GAME_FN(re::Ptr32<char>*, __thiscall, NiString_Construct, 0x007741A0, re::Ptr32<char>* self, const char* text);
GAME_FN(re::Ptr32<char>*, __thiscall, NiString_ConstructCopy, 0x00774220, re::Ptr32<char>* self, const re::Ptr32<char>* other);
GAME_FN(void, __thiscall, NiString_Destruct, 0x00774240, re::Ptr32<char>* self);
GAME_FN(int, __thiscall, NiString_FindReverse, 0x007743F0, const re::Ptr32<char>* self, char c);
GAME_FN(re::Ptr32<char>*, __thiscall, NiString_GetSubstring, 0x00774310, const re::Ptr32<char>* self, re::Ptr32<char>* result, uint32_t begin,
        uint32_t end);
GAME_FN(re::Ptr32<char>*, __thiscall, NiString_AssignText, 0x007745E0, re::Ptr32<char>* self, const char* text);
GAME_FN(re::Ptr32<char>*, __thiscall, NiString_AppendText, 0x00774870, re::Ptr32<char>* self, const char* text);
GAME_FN(uint8_t, __thiscall, AtlasMap_ReadFile, 0x0083BC30, void* self, const char* filename);
GAME_FN(re::Ptr32<char>*, __thiscall, AtlasMap_GetFontFilename, 0x008368C0, void* self, re::Ptr32<char>* result, uint32_t index);
GAME_FN(void, __thiscall, AtlasMap_Destruct, 0x0083BBB0, void* self);
GAME_FN(void, __thiscall, AtlasMap_Construct, 0x0083C2C0, void* self);
GAME_FN(NiUIGamePadBinding*, __thiscall, GamePadBinding_Construct, 0x00832390, NiUIGamePadBinding* self);
GAME_FN(NiUIKeyBinding*, __thiscall, KeyBinding_Construct, 0x00832410, NiUIKeyBinding* self);

constexpr uint32_t kGroupArrayBaseVtable = 0x008FF5AC;   // NiTArray<NiPointer<NiUIRenderGroup>>
constexpr uint32_t kGroupArrayVtable = 0x00988FD0;

// |b - a| as the code computes a rect's span (an x87 compare: NaN takes the second way)
inline float Span(float a, float b) {
    return a < b ? b - a : a - b;
}

// the mouse's button state (NiInputMouse: 8 buttons of 0x10 from +0x80, as the game pad's):
// pressed now (+1) and before (+3), the modifiers (+8)
inline const uint8_t* MouseButton(const void* mouse, int32_t button) {
    return static_cast<const uint8_t*>(mouse) + (button + 8) * 0x10;
}

// a binding's signal emitted with the state bits (only their byte is meaningful: the game's
// pushes a dword whose upper bytes are stale stack)
inline void EmitBits(const NiUISignal& signal, uint8_t bits) {
    uint32_t count = signal.set.used;
    for (uint32_t i = 0; i < count; i++) {
        Virtual<void>(signal.set.base[i], 1, uint32_t(bits));
    }
}

// ReserveGamePadButton(button, slot, state, modifiers): a game pad binding made unless one
// exists for the button and modifiers
uint8_t __fastcall ReserveGamePadButton(NiUIManager* self, re::Edx, uint32_t button, NiUISignal* slot, int state,
                                        uint32_t modifiers) {
    if (self->hasGamePad == 0) {
        return 0;
    }
    if (UIManager_FindGamePadBinding(self, button, modifiers) != -1) {
        return 0;
    }
    auto* binding = static_cast<NiUIGamePadBinding*>(NiNew(0x28));
    if (binding != nullptr) {
        binding = GamePadBinding_Construct(binding);
    }
    NiUIGamePadBinding* held = binding;
    AddRef(binding);
    binding->button = button;
    binding->modifiers = modifiers;
    binding->isButton = 1;
    binding->stateMask = uint8_t(state);
    Signal1Uchar_Subscribe(&binding->signal, slot);
    binding->axis = 0;
    ObjectSet_Add(&self->gamePadBindings, re::PtrCast<const re::Ptr32<void>*>(&held));
    Release(binding);
    return 1;
}

// ReserveKeyboardButton(key, slot, state, modifiers)
uint8_t __fastcall ReserveKeyboardButton(NiUIManager* self, re::Edx, uint32_t key, NiUISignal* slot, int state,
                                         uint32_t modifiers) {
    if (self->hasKeyboard == 0) {
        return 0;
    }
    if (UIManager_FindKeyboardBinding(self, key, modifiers) != -1) {
        return 0;
    }
    auto* binding = static_cast<NiUIKeyBinding*>(NiNew(0x24));
    if (binding != nullptr) {
        binding = KeyBinding_Construct(binding);
    }
    NiUIKeyBinding* held = binding;
    AddRef(binding);
    binding->key = key;
    binding->modifiers = modifiers;
    binding->stateMask = uint8_t(state);
    Signal1Uchar_Subscribe(&binding->signal, slot);
    ObjectSet_Add(&self->keyboardBindings, re::PtrCast<const re::Ptr32<void>*>(&held));
    Release(binding);
    return 1;
}

// NiUIAtlasMap::GetFontFilename(result, index): a copy of the font name for the resolution
re::Ptr32<char>* __fastcall GetFontFilename(uint8_t* self, re::Edx, re::Ptr32<char>* result, uint32_t index) {
    NiString_ConstructCopy(result, reinterpret_cast<re::Ptr32<char>*>(self + 0x568 + index * 4));
    return result;
}

// GetUIManager
NiUIManager* __cdecl GetUIManager() {
    return NiUIManager_ms_pkUIManager;
}

// Shutdown: the manager deleted
void __cdecl Shutdown() {
    if (NiUIManager_ms_pkUIManager != nullptr) {
        Virtual<void>(NiUIManager_ms_pkUIManager, 0, 1);
    }
    NiUIManager_ms_pkUIManager = nullptr;
}

// the mouse binding for a button and modifiers (-1 none)
int __fastcall FindMouseBinding(NiUIManager* self, re::Edx, uint32_t button, uint32_t modifiers) {
    uint32_t count = self->mouseBindings.used;
    for (uint32_t i = 0; i < count; i++) {
        NiUIKeyBinding* binding = self->mouseBindings.base[i];
        if (binding->key == button && binding->modifiers == modifiers) {
            return int(i);
        }
    }
    return -1;
}

// Draw(renderer): when visible, the groups in render order
void __fastcall Draw(NiUIManager* self, re::Edx, void* renderer) {
    if (self->visible == 0) {
        return;
    }
    uint32_t count = self->renderGroups.size;
    for (uint32_t i = 0; i < count; i++) {
        Virtual<void>(self->renderGroups.base[i], 3, renderer);
    }
}

// the select-next-group slot's function: the focused group's focus moved on, else the next
// group's (wrapping once, then none)
void __fastcall SelectNextGroup(NiUIManager* self, re::Edx, uint32_t) {
    if (self->groupFocusActive == 0) {
        return;
    }
    uint32_t count = self->groups.size;
    if (count == 0) {
        return;
    }
    if (self->focusedGroup == -1 || uint32_t(self->focusedGroup) >= count) {
        self->focusedGroup = 0;
    }
    if (Virtual<uint8_t>(self->groups.base[self->focusedGroup], 21)) {   // AdvanceFocus
        return;
    }
    uint32_t wraps = 0;
    while (true) {
        self->focusedGroup++;
        if (uint32_t(self->focusedGroup) >= count) {
            wraps++;
            self->focusedGroup = 0;
            if (wraps > 1) {
                self->focusedGroup = -1;
                return;
            }
        }
        if (Virtual<uint8_t>(self->groups.base[self->focusedGroup], 21)) {
            return;
        }
    }
}

// the select-previous-group slot's function (backwards, ReverseFocus)
void __fastcall SelectPrevGroup(NiUIManager* self, re::Edx, uint32_t) {
    if (self->groupFocusActive == 0) {
        return;
    }
    uint32_t count = self->groups.size;
    if (count == 0) {
        return;
    }
    if (self->focusedGroup == -1 || uint32_t(self->focusedGroup) >= count) {
        self->focusedGroup = int32_t(count - 1);
    }
    if (Virtual<uint8_t>(self->groups.base[self->focusedGroup], 22)) {   // ReverseFocus
        return;
    }
    uint32_t wraps = 0;
    while (true) {
        self->focusedGroup--;
        if (self->focusedGroup < 0) {
            wraps++;
            self->focusedGroup = int32_t(count - 1);
            if (wraps > 1) {
                self->focusedGroup = -1;
                return;
            }
        }
        if (Virtual<uint8_t>(self->groups.base[self->focusedGroup], 22)) {
            return;
        }
    }
}

// the enter slot's function: to the focused group (when the UI's shown)
void __fastcall EnterPressed(NiUIManager* self, re::Edx, uint32_t state) {
    if (self->groupFocusActive != 0 && self->focusedGroup != -1 && self->groups.size != 0 && self->visible != 0) {
        Virtual<void>(self->groups.base[self->focusedGroup], 4, state);   // EnterPressed
    }
}

// ConvertNSCToSafeArea(result, rect): a normalized rect mapped into the safe area (y from the
// bottom)
float* __fastcall ConvertNSCToSafeArea(NiUIManager* self, re::Edx, float* result, const float* rect) {
    const float* safe = self->safeRect;
    result[0] = safe[0];
    result[1] = safe[1];
    result[2] = safe[2];
    result[3] = safe[3];
    float width = Span(safe[0], safe[1]);
    result[0] = width * rect[0] + result[0];
    width = Span(self->safeRect[0], self->safeRect[1]);
    result[1] = result[1] - (1.0f - rect[1]) * width;
    float height = Span(self->safeRect[3], self->safeRect[2]);
    result[2] = rect[2] * height + result[2];
    height = Span(self->safeRect[3], self->safeRect[2]);
    result[3] = result[3] - (1.0f - rect[3]) * height;
    return result;
}

// ConvertNSCToPixels(result, rect): the safe-area rect times the screen size, truncated
uint32_t* __fastcall ConvertNSCToPixels(NiUIManager* self, re::Edx, uint32_t* result, const float* rect) {
    float area[4];
    UIManager_ConvertNSCToSafeArea(self, area, rect);
    result[0] = uint32_t(crt::FloatToInt(self->screenWidth * area[0]));
    result[1] = uint32_t(crt::FloatToInt(self->screenWidth * area[1]));
    result[2] = uint32_t(crt::FloatToInt(self->screenHeight * area[2]));
    result[3] = uint32_t(crt::FloatToInt(self->screenHeight * area[3]));
    return result;
}

// ConvertPixelsToNSC(result, x, y): pixels over the safe area's size in pixels
float* __fastcall ConvertPixelsToNSC(NiUIManager* self, re::Edx, float* result, uint32_t x, uint32_t y) {
    float fx = float(double(x));
    float fy = float(double(y));
    float width = self->screenWidth * Span(self->safeRect[0], self->safeRect[1]);
    float height = self->screenHeight * Span(self->safeRect[3], self->safeRect[2]);
    result[0] = fx / width;
    result[1] = fy / height;
    return result;
}

// ConvertSizeToNSC(result, width, height): a normalized size over the safe area's
float* __fastcall ConvertSizeToNSC(NiUIManager* self, re::Edx, float* result, float width, float height) {
    result[0] = width / Span(self->safeRect[0], self->safeRect[1]);
    result[1] = height / Span(self->safeRect[3], self->safeRect[2]);
    return result;
}

// PrepareRenderGroups: the render list sorted by render order (selection: the first lowest),
// then each group's screen elements made (with no groups the loop runs off: the game's)
void __fastcall PrepareRenderGroups(NiUIManager* self, re::Edx) {
    uint32_t count = self->renderGroups.size;
    uint32_t last = count - 1;
    for (uint32_t i = 0; i < last;) {
        NiUIRenderGroup* held = self->renderGroups.base[i];
        uint8_t lowest = held->renderOrder;
        uint32_t lowestIndex = i;
        uint32_t next = i + 1;
        for (uint32_t j = next; j < count; j++) {
            uint8_t order = self->renderGroups.base[j]->renderOrder;
            if (order < lowest) {
                lowestIndex = j;
                lowest = order;
            }
        }
        AddRef(held);
        ObjectArray_SetAt(&self->renderGroups, i,
                          reinterpret_cast<const re::Ptr32<void>*>(&self->renderGroups.base[lowestIndex]));
        ObjectArray_SetAt(&self->renderGroups, lowestIndex, re::PtrCast<const re::Ptr32<void>*>(&held));
        Release(held);
        i = next;
    }
    for (uint32_t i = 0; i < count; i++) {
        UIRenderGroup_InitializeScreenElements(self->renderGroups.base[i]);
    }
}

// UpdateUI: the reserved mouse buttons, the group under the mouse focused, the reserved keys
// and game pad buttons, each binding emitting its down / pressed / released bits
void __fastcall UpdateUI(NiUIManager* self, re::Edx) {
    if (self->inputSystem == nullptr) {
        return;
    }
    void* mouse = At<void*>(self->inputSystem, 0x44);
    if (mouse != nullptr) {
        for (uint32_t i = 0; i < self->mouseBindings.used; i++) {
            NiUIKeyBinding* binding = self->mouseBindings.base[i];
            uint8_t bits = 0;
            int32_t button = int32_t(binding->key);
            if ((binding->stateMask & 1) != 0 && button < 8) {
                const uint8_t* state = MouseButton(mouse, button);
                if (state[1] != 0 && At<uint32_t>(state, 8) == binding->modifiers) {
                    bits = 1;
                }
            }
            if ((binding->stateMask & 2) != 0 && button < 8) {
                const uint8_t* state = MouseButton(mouse, button);
                if (state[1] != 0 && state[3] == 0 && At<uint32_t>(state, 8) == binding->modifiers) {
                    bits |= 2;
                }
            }
            if ((binding->stateMask & 4) != 0 && button < 8) {
                const uint8_t* state = MouseButton(mouse, button);
                if (state[1] == 0 && state[3] != 0 && At<uint32_t>(state, 8) == binding->modifiers) {
                    bits |= 4;
                }
            }
            if (bits != 0) {
                Signal1_Emit(&binding->signal, bits);
            }
        }
        NiRefObject* cursor = self->cursor;
        if (cursor != nullptr && self->groupFocusActive != 0) {
            int32_t x = At<int32_t>(cursor, 0x38);
            int32_t y = At<int32_t>(cursor, 0x3C);
            uint32_t count = self->groups.size;
            self->mouseOverGroup = 0;
            x = x <= 0 ? 0 : x;
            y = y <= 0 ? 0 : y;
            for (uint32_t i = 0; i < count; i++) {
                if (Virtual<uint8_t>(self->groups.base[i], 14, x, y)) {   // IsInside
                    int32_t focused = self->focusedGroup;
                    if (focused >= 0 && uint32_t(focused) != i) {
                        Virtual<uint8_t>(self->groups.base[focused], 6, 0);   // SetFocus
                    }
                    self->focusedGroup = int32_t(i);
                    self->mouseOverGroup = 1;
                    Virtual<uint8_t>(self->groups.base[i], 6, 1);
                    break;
                }
            }
        }
    }
    uint8_t* keyboard = At<uint8_t*>(self->inputSystem, 0x40);
    if (keyboard != nullptr) {
        for (uint32_t i = 0; i < self->keyboardBindings.used; i++) {
            NiUIKeyBinding* binding = self->keyboardBindings.base[i];
            uint8_t bits = 0;
            int32_t key = int32_t(binding->key);
            uint32_t modifiers = At<uint32_t>(keyboard, 0x10);
            auto now = [&] { return ((keyboard[(key >> 3) + 0x21] >> (key & 7)) & 1) != 0; };
            auto before = [&] { return ((keyboard[(key >> 3) + 0x41] >> (key & 7)) & 1) != 0; };
            if ((binding->stateMask & 1) != 0 && key != 0 && now() && modifiers == binding->modifiers) {
                bits = 1;
            }
            if ((binding->stateMask & 2) != 0 && key != 0 && now() && !before() &&
                At<uint32_t>(keyboard, 0x10) == binding->modifiers) {
                bits |= 2;
            }
            if ((binding->stateMask & 4) != 0 && key != 0 && !now() && before() &&
                At<uint32_t>(keyboard, 0x10) == binding->modifiers) {
                bits |= 4;
            }
            if (bits != 0) {
                EmitBits(binding->signal, bits);
            }
        }
    }
    for (uint32_t pad = 0; pad < 8; pad++) {
        void* gamePad = At<void*>(self->inputSystem, 0x48 + int(pad) * 4);
        if (gamePad == nullptr) {
            continue;
        }
        for (uint32_t i = 0; i < self->gamePadBindings.used; i++) {
            NiUIGamePadBinding* binding = self->gamePadBindings.base[i];
            if (binding->isButton == 0) {
                continue;
            }
            uint8_t bits = 0;
            uint32_t modifiers;
            if ((binding->stateMask & 1) != 0 && GamePad_ButtonIsDown(gamePad, binding->button, &modifiers) &&
                modifiers == binding->modifiers) {
                bits = 1;
            }
            if ((binding->stateMask & 2) != 0 && GamePad_ButtonWasPressed(gamePad, binding->button, &modifiers) &&
                modifiers == binding->modifiers) {
                bits |= 2;
            }
            if ((binding->stateMask & 4) != 0 && GamePad_ButtonWasReleased(gamePad, binding->button, &modifiers) &&
                modifiers == binding->modifiers) {
                bits |= 4;
            }
            if (bits != 0) {
                EmitBits(binding->signal, bits);
            }
        }
    }
}

// SortGroups: the focus list sorted by position (top first, then left: selection)
void __fastcall SortGroups(NiUIManager* self, re::Edx) {
    uint32_t count = self->groups.size;
    for (uint32_t i = 0; i < count;) {
        float best[2];
        float offset[2];
        UIElement_GetOffset(self->groups.base[i], offset);
        best[0] = offset[0];
        best[1] = offset[1];
        uint32_t bestIndex = i;
        uint32_t next = i + 1;
        for (uint32_t j = next; j < count; j++) {
            float other[2];
            UIElement_GetOffset(self->groups.base[j], other);
            if (best[1] > other[1] || (best[1] == other[1] && best[0] > other[0])) {
                best[0] = other[0];
                best[1] = other[1];
                bestIndex = j;
            }
        }
        NiUIRenderGroup* held = self->groups.base[bestIndex];
        AddRef(held);
        ObjectArray_SetAt(&self->groups, bestIndex, re::PtrCast<const re::Ptr32<void>*>(&self->groups.base[i]));
        ObjectArray_SetAt(&self->groups, i, re::PtrCast<const re::Ptr32<void>*>(&held));
        Release(held);
        i = next;
    }
}

// the left-mouse-button slot's function: pressing turns the group focus off, releasing (when
// shown) back on; over no group (or pressed outside) the reserved mouse slots get it, else
// the focused group
void __fastcall LeftMouseButton(NiUIManager* self, re::Edx, uint32_t state) {
    uint8_t pressed = uint8_t(state) & 2;
    if (pressed != 0) {
        self->groupFocusActive = 0;
    } else if ((state & 4) != 0 && self->visible != 0) {
        self->groupFocusActive = 1;
    }
    if ((self->mouseOverGroup == 0 && pressed != 0) || self->mouseOverNothing != 0 || self->focusedGroup == -1 ||
        self->groups.size == 0) {
        self->mouseOverNothing = 1;
        uint32_t count = self->mouseSignal.set.used;
        for (uint32_t i = 0; i < count; i++) {
            Virtual<void>(self->mouseSignal.set.base[i], 1, state);
        }
        if ((state & 4) != 0) {
            self->mouseOverNothing = 0;
        }
        return;
    }
    if (self->visible != 0) {
        self->mouseOverNothing = 0;
        Virtual<void>(self->groups.base[self->focusedGroup], 5, state);   // LeftMouseButton
    }
}

// the destructor
void __fastcall Destruct(NiUIManager* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiUIManagerVtable);
    ClearRef(self->font);
    ClearRef(self->inputSystem);
    ClearRef(self->cursor);
    auto releaseSet = [](auto& set) {
        uint32_t count = set.used;
        for (uint32_t i = 0; i < count; i++) {
            auto& element = set.base[i];
            if (element != nullptr) {
                Release(element);
                element = nullptr;
            }
        }
    };
    releaseSet(self->mouseBindings);
    releaseSet(self->gamePadBindings);
    releaseSet(self->keyboardBindings);
    re::Ptr32<void> none = nullptr;
    uint32_t count = self->groups.size;
    for (uint32_t i = 0; i < count; i++) {
        ObjectArray_SetAt(&self->groups, i, &none);
    }
    count = self->renderGroups.size;
    for (uint32_t i = 0; i < count; i++) {
        ObjectArray_SetAt(&self->renderGroups, i, &none);
    }
    self->renderGroups.vtable = reinterpret_cast<void*>(kGroupArrayBaseVtable);
    PointerArray_Delete(self->renderGroups.base);
    self->groups.vtable = reinterpret_cast<void*>(kNiTObjectArrayVtable);
    PointerArray_Delete(self->groups.base);
    ObjectSet_RemoveAll(&self->keyboardBindings);
    PointerArray_Delete(self->keyboardBindings.base);
    ObjectSet_RemoveAll(&self->gamePadBindings);
    PointerArray_Delete(self->gamePadBindings.base);
    ObjectSet_RemoveAll(&self->mouseBindings);
    PointerArray_Delete(self->mouseBindings.base);
    Release(self->cursor);
    Release(self->inputSystem);
    Release(self->font);
    AtlasMap_Destruct(self->atlasMap);
    Signal1Uchar_Destruct(&self->mouseSignal);
    NiUIMemberSlot* slots[4] = {&self->leftMouseButton, &self->enterPressed, &self->selectPrevGroup,
                                &self->selectNextGroup};
    for (NiUIMemberSlot* slot : slots) {
        slot->vtable = reinterpret_cast<void*>(kNiUIMemberSlot1UcharVtable);
        slot->instance = nullptr;
        slot->function = nullptr;
        slot->adjust = 0;
        BaseSlot1_Destruct(slot);
    }
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    ni::sys::Decrement(&NiRefObject_ms_uiObjects);
}

// ReserveMouseButton(button, slot, state, modifiers)
uint8_t __fastcall ReserveMouseButton(NiUIManager* self, re::Edx, uint32_t button, NiUISignal* slot, int state,
                                      uint32_t modifiers) {
    if (self->hasMouse == 0) {
        return 0;
    }
    if (UIManager_FindMouseBinding(self, button, modifiers) != -1) {
        return 0;
    }
    auto* binding = static_cast<NiUIKeyBinding*>(NiNew(0x24));
    if (binding != nullptr) {
        binding = KeyBinding_Construct(binding);
    }
    NiUIKeyBinding* held = binding;
    AddRef(binding);
    binding->key = button;
    binding->modifiers = modifiers;
    binding->stateMask = uint8_t(state);
    Signal1Uchar_Subscribe(&binding->signal, slot);
    ObjectSet_Add(&self->mouseBindings, re::PtrCast<const re::Ptr32<void>*>(&held));
    Release(binding);
    return 1;
}

// the constructor: the four member slots on its own functions, the skin's atlas, the
// renderer's safe area and the screen's size in pixels
NiUIManager* __fastcall Construct(NiUIManager* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    self->refCount = 0;
    ni::sys::Increment(&NiRefObject_ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(kNiUIManagerVtable);
    NiUIMemberSlot* slots[4] = {&self->selectNextGroup, &self->selectPrevGroup, &self->enterPressed,
                                &self->leftMouseButton};
    for (NiUIMemberSlot* slot : slots) {
        slot->set.base = nullptr;
        slot->set.alloced = 0;
        slot->set.used = 0;
        slot->instance = nullptr;
        slot->adjust = 0;
        slot->vtable = reinterpret_cast<void*>(kNiUIMemberSlot1UcharVtable);
        slot->function = nullptr;
    }
    self->mouseSignal.vtable = reinterpret_cast<void*>(kNiUISignal1UcharVtable);
    self->mouseSignal.set.base = nullptr;
    self->mouseSignal.set.alloced = 0;
    self->mouseSignal.set.used = 0;
    self->focusedGroup = -1;
    AtlasMap_Construct(self->atlasMap);
    self->font = nullptr;
    self->inputSystem = nullptr;
    self->cursor = nullptr;
    self->mouseBindings = {nullptr, 0, 0};
    self->gamePadBindings = {nullptr, 0, 0};
    self->keyboardBindings = {nullptr, 0, 0};
    self->groups.maxSize = 0;
    self->groups.size = 0;
    self->groups.esize = 0;
    self->groups.base = nullptr;
    self->groups.vtable = reinterpret_cast<void*>(kNiTObjectArrayVtable);
    self->groups.growBy = 1;
    self->renderGroups.maxSize = 0;
    self->renderGroups.growBy = 1;
    self->renderGroups.size = 0;
    self->renderGroups.esize = 0;
    self->renderGroups.base = nullptr;
    self->renderGroups.vtable = reinterpret_cast<void*>(kGroupArrayVtable);
    self->safeRect[0] = 0.0f;
    self->safeRect[1] = 0.0f;
    self->safeRect[2] = 0.0f;
    self->safeRect[3] = 0.0f;
    self->maxCharWidth = 0.0f;
    self->maxCharHeight = 0.0f;
    self->hasMouse = 0;
    self->groupFocusActive = 1;
    self->visible = 1;
    self->hasKeyboard = 0;
    self->hasGamePad = 0;
    self->mouseReserved = 0;
    self->mouseOverNothing = 0;
    self->selectNextGroup.instance = self;
    self->selectNextGroup.adjust = 0;
    self->selectNextGroup.function = reinterpret_cast<void*>(0x00836A20);
    self->selectPrevGroup.instance = self;
    self->selectPrevGroup.adjust = 0;
    self->selectPrevGroup.function = reinterpret_cast<void*>(0x00836AE0);
    self->enterPressed.instance = self;
    self->enterPressed.adjust = 0;
    self->enterPressed.function = reinterpret_cast<void*>(0x00836B80);
    self->leftMouseButton.instance = self;
    self->leftMouseButton.adjust = 0;
    self->leftMouseButton.function = reinterpret_cast<void*>(0x00837690);
    void* renderer = NiRenderer_ms_pkRenderer;
    self->safeRect[0] = At<float>(renderer, 0x20);
    self->safeRect[1] = At<float>(renderer, 0x24);
    self->safeRect[2] = At<float>(renderer, 0x28);
    self->safeRect[3] = At<float>(renderer, 0x2C);
    uint32_t pixels[2];
    Renderer_ConvertFromNDCToPixels(renderer, 1.0f, 1.0f, &pixels[0], &pixels[1]);
    self->screenWidth = float(double(pixels[0]));
    self->screenHeight = float(double(pixels[1]));
    return self;
}

// the deleting destructor
NiUIManager* __fastcall DeletingDestructor(NiUIManager* self, re::Edx, int flags) {
    UIManager_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x6F0);
    }
    return self;
}

// Create: the one manager, once there's a renderer
void __cdecl Create() {
    if (NiRenderer_ms_pkRenderer == nullptr || NiUIManager_ms_pkUIManager != nullptr) {
        return;
    }
    auto* manager = static_cast<NiUIManager*>(NiNew(0x6F0));
    NiUIManager_ms_pkUIManager = manager != nullptr ? UIManager_Construct(manager) : nullptr;
}

// Initialize(inputSystem, skin filename, cursor): the skin's atlas read, the font for the
// screen's height from the skin's directory, the widest and tallest printable character as the
// normalized character size, the devices there are, the navigation keys and buttons reserved
// (Tab / Shift-Tab, the shoulder buttons, Enter / A, the left mouse button), the cursor kept
uint8_t __fastcall Initialize(NiUIManager* self, re::Edx, NiRefObject* inputSystem, const char* filename,
                              NiRefObject* cursor) {
    if (inputSystem == nullptr || At<uint32_t>(inputSystem, 0x10) != 0) {
        return 0;
    }
    if (filename == nullptr) {
        return 0;
    }
    void* atlas = self->atlasMap;
    if (!AtlasMap_ReadFile(atlas, filename)) {
        return 0;
    }
    re::Ptr32<char> path;
    NiString_Construct(&path, filename);
    int slash = NiString_FindReverse(&path, '/');
    int backslash = NiString_FindReverse(&path, '\\');
    re::Ptr32<char> directory;
    if (slash != -1) {
        if (backslash != -1) {
            uint32_t end = uint32_t(slash) > uint32_t(backslash) ? uint32_t(slash) + 1 : uint32_t(backslash) + 1;
            NiString_GetSubstring(&path, &directory, 0, end);
        } else {
            NiString_GetSubstring(&path, &directory, 0, uint32_t(slash) + 1);
        }
    } else if (backslash != -1) {
        NiString_GetSubstring(&path, &directory, 0, uint32_t(backslash) + 1);
    } else {
        NiString_Construct(&directory, reinterpret_cast<const char*>(0x008FEFBB));   // ""
    }
    NiString_AssignText(&path, directory);
    NiString_Destruct(&directory);
    uint32_t pixels[2];
    Renderer_ConvertFromNDCToPixels(NiRenderer_ms_pkRenderer, 1.0f, 1.0f, &pixels[0], &pixels[1]);
    uint32_t resolution = pixels[1] <= 600 ? 0 : pixels[1] <= 768 ? 1 : 2;
    re::Ptr32<char> fontName;
    AtlasMap_GetFontFilename(atlas, &fontName, resolution);
    NiString_AppendText(&path, fontName);
    NiString_Destruct(&fontName);
    Pointer_Assign(reinterpret_cast<re::Ptr32<void>*>(&self->font), NiFont_Create(NiRenderer_ms_pkRenderer, path));
    if (self->font == nullptr) {
        NiString_Destruct(&path);
        return 0;
    }
    float maxWidth = 0.0f;
    float maxHeight = 0.0f;
    for (int8_t c = 0x21; c < 0x7F; c++) {
        float width = 0.0f;
        float height = 0.0f;
        // (the character passed in a dword whose upper bytes are stale: only its byte is read)
        NiFont_GetCharExtent(self->font, uint8_t(c), &width, &height);
        if (!(width < maxWidth)) {
            maxWidth = width;
        }
        if (!(height < maxHeight)) {
            maxHeight = height;
        }
    }
    float size[2];
    UIManager_ConvertPixelsToNSC(self, size, uint32_t(crt::FloatToInt(maxWidth)),
                                 uint32_t(crt::FloatToInt(maxHeight)));
    self->maxCharWidth = size[0];
    self->maxCharHeight = size[1];
    Pointer_Assign(reinterpret_cast<re::Ptr32<void>*>(&self->inputSystem), inputSystem);
    uint32_t devices = Virtual<uint32_t>(self->inputSystem, 27);   // GetAvailableDeviceCount
    for (uint32_t i = 0; i < devices; i++) {
        void* description = i == 0 ? Virtual<void*>(self->inputSystem, 28)    // GetFirstDeviceDesc
                                   : Virtual<void*>(self->inputSystem, 29);   // GetNextDeviceDesc
        switch (At<uint32_t>(description, 8)) {
        case 1:
            self->hasKeyboard = 1;
            break;
        case 2:
            self->hasMouse = 1;
            break;
        case 3:
            self->hasGamePad = 1;
            break;
        default:
            break;
        }
    }
    if (self->hasKeyboard == 0 && self->hasMouse == 0 && self->hasGamePad == 0) {
        NiString_Destruct(&path);
        return 0;
    }
    UIManager_ReserveGamePadButton(self, 1, &self->selectNextGroup, 2, 0);
    UIManager_ReserveGamePadButton(self, 0, &self->selectPrevGroup, 2, 0);
    UIManager_ReserveGamePadButton(self, 9, &self->enterPressed, 7, 0);
    UIManager_ReserveKeyboardButton(self, 0xF, &self->selectNextGroup, 2, 0);
    UIManager_ReserveKeyboardButton(self, 0xF, &self->selectPrevGroup, 2, 1);
    UIManager_ReserveKeyboardButton(self, 0xF, &self->selectPrevGroup, 2, 2);
    UIManager_ReserveKeyboardButton(self, 0x1C, &self->enterPressed, 7, 0);
    UIManager_ReserveMouseButton(self, 0, &self->leftMouseButton, 7, 0);
    AssignRef(self->cursor, cursor);
    NiString_Destruct(&path);
    return 1;
}

// AddUIGroup(group): appended to the focus list unless there (the first one focused), then the
// list sorted
void __fastcall AddUIGroup(NiUIManager* self, re::Edx, NiUIRenderGroup* group) {
    uint32_t count = self->groups.size;
    for (uint32_t i = 0; i < count; i++) {
        if (self->groups.base[i] == group) {
            return;
        }
    }
    NiUIRenderGroup* held = group;
    AddRef(group);
    uint32_t index = self->groups.size;
    if (index >= self->groups.maxSize) {
        ObjectArray_SetSize(&self->groups, self->groups.growBy + index);
    }
    ObjectArray_SetAt(&self->groups, index, re::PtrCast<const re::Ptr32<void>*>(&held));
    Release(group);
    if (count == 0) {
        self->focusedGroup = 0;
        Virtual<uint8_t>(group, 6, 1);   // SetFocus
    }
    UIManager_SortGroups(self);
}

// the game pad axis binding for an axis (-1 none)
int __fastcall FindGamePadAxisBinding(NiUIManager* self, re::Edx, uint32_t axis) {
    uint32_t count = self->gamePadBindings.used;
    for (uint32_t i = 0; i < count; i++) {
        NiUIGamePadBinding* binding = self->gamePadBindings.base[i];
        if (binding->isButton == 0 && binding->axis == axis) {
            return int(i);
        }
    }
    return -1;
}

// UnreserveMouse(slot): off the mouse signal (the mouse free once nothing's on it)
void __fastcall UnreserveMouse(NiUIManager* self, re::Edx, NiUISignal* slot) {
    if (self->hasMouse == 0) {
        return;
    }
    BaseSlot0_Unsubscribe(&self->mouseSignal, slot);
    if (self->mouseSignal.set.used == 0) {
        self->mouseReserved = 0;
    }
}

// the unreserves: the binding removed when the slot's on it
template <typename Binding, int SignalOffset>
void RemoveBinding(NiTSet<Binding*>* set, int index, NiUISignal* slot) {
    if (index < 0) {
        return;
    }
    auto* signal = re::PtrCast<const NiUISignal*>(re::PtrCast<uint8_t*>(set->base[index]) + SignalOffset);
    if (Signal_IsSubscribed(signal, slot)) {
        ObjectSet_RemoveAt(set, uint32_t(index));
    }
}

void __fastcall UnreserveMouseButton(NiUIManager* self, re::Edx, uint32_t button, NiUISignal* slot,
                                     uint32_t modifiers) {
    if (self->hasMouse == 0) {
        return;
    }
    RemoveBinding<NiUIKeyBinding, 0x10>(&self->mouseBindings, UIManager_FindMouseBinding(self, button, modifiers),
                                        slot);
}

void __fastcall UnreserveGamePadButton(NiUIManager* self, re::Edx, uint32_t button, NiUISignal* slot,
                                       uint32_t modifiers) {
    if (self->hasGamePad == 0) {
        return;
    }
    RemoveBinding<NiUIGamePadBinding, 0x14>(&self->gamePadBindings,
                                            UIManager_FindGamePadBinding(self, button, modifiers), slot);
}

void __fastcall UnreserveGamePadAxis(NiUIManager* self, re::Edx, uint32_t axis, NiUISignal* slot) {
    if (self->hasGamePad == 0) {
        return;
    }
    RemoveBinding<NiUIGamePadBinding, 0x14>(&self->gamePadBindings, UIManager_FindGamePadAxisBinding(self, axis),
                                            slot);
}

void __fastcall UnreserveKeyboardButton(NiUIManager* self, re::Edx, uint32_t key, NiUISignal* slot,
                                        uint32_t modifiers) {
    if (self->hasKeyboard == 0) {
        return;
    }
    RemoveBinding<NiUIKeyBinding, 0x10>(&self->keyboardBindings, UIManager_FindKeyboardBinding(self, key, modifiers),
                                        slot);
}

// ReserveMouse(slot): the whole mouse to one slot at a time
uint8_t __fastcall ReserveMouse(NiUIManager* self, re::Edx, NiUISignal* slot) {
    if (self->mouseReserved != 0 || self->hasMouse == 0) {
        return 0;
    }
    self->mouseReserved = 1;
    Signal1Uchar_Subscribe(&self->mouseSignal, slot);
    return 1;
}

// ReserveGamePadAxis(axis, slot): an axis binding (button -1, no state)
uint8_t __fastcall ReserveGamePadAxis(NiUIManager* self, re::Edx, uint32_t axis, NiUISignal* slot) {
    if (self->hasGamePad == 0) {
        return 0;
    }
    if (UIManager_FindGamePadAxisBinding(self, axis) != -1) {
        return 0;
    }
    auto* binding = static_cast<NiUIGamePadBinding*>(NiNew(0x28));
    if (binding != nullptr) {
        binding = GamePadBinding_Construct(binding);
    }
    NiUIGamePadBinding* held = binding;
    AddRef(binding);
    binding->axis = axis;
    binding->isButton = 0;
    binding->stateMask = 0;
    binding->modifiers = 0;
    binding->button = 0xFFFFFFFF;
    Signal1Uchar_Subscribe(&binding->signal, slot);
    ObjectSet_Add(&self->gamePadBindings, re::PtrCast<const re::Ptr32<void>*>(&held));
    Release(binding);
    return 1;
}

} // namespace

#define NIUIMGR_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(Original_, __LINE__);                                                \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIUIMGR_REVERSED("NiUIManager::ReserveGamePadButton", 0x008328B0, &ReserveGamePadButton);
NIUIMGR_REVERSED("NiUIManager::ReserveKeyboardButton", 0x00832970, &ReserveKeyboardButton);
NIUIMGR_REVERSED("NiUIAtlasMap::GetFontFilename", 0x008368C0, &GetFontFilename);
NIUIMGR_REVERSED("NiUIManager::GetUIManager", 0x008368E0, &GetUIManager);
NIUIMGR_REVERSED("NiUIManager::Shutdown", 0x008368F0, &Shutdown);
NIUIMGR_REVERSED("NiUIManager::FindMouseBinding", 0x008369A0, &FindMouseBinding);
NIUIMGR_REVERSED("NiUIManager::Draw", 0x008369E0, &Draw);
NIUIMGR_REVERSED("NiUIManager::SelectNextGroup", 0x00836A20, &SelectNextGroup);
NIUIMGR_REVERSED("NiUIManager::SelectPrevGroup", 0x00836AE0, &SelectPrevGroup);
NIUIMGR_REVERSED("NiUIManager::EnterPressed", 0x00836B80, &EnterPressed);
// (the game's ConvertNSCToPixels, 0x836D00, reads ecx after calling this one: turn them on together)
NIUIMGR_REVERSED("NiUIManager::ConvertNSCToSafeArea", 0x00836BC0, &ConvertNSCToSafeArea);
NIUIMGR_REVERSED("NiUIManager::ConvertNSCToPixels", 0x00836D00, &ConvertNSCToPixels);
NIUIMGR_REVERSED("NiUIManager::ConvertPixelsToNSC", 0x00836DD0, &ConvertPixelsToNSC);
NIUIMGR_REVERSED("NiUIManager::ConvertSizeToNSC", 0x00836EB0, &ConvertSizeToNSC);
NIUIMGR_REVERSED("NiUIManager::PrepareRenderGroups", 0x00836FE0, &PrepareRenderGroups);
NIUIMGR_REVERSED("NiUIManager::UpdateUI", 0x008370F0, &UpdateUI);
NIUIMGR_REVERSED("NiUIManager::SortGroups", 0x00837540, &SortGroups);
NIUIMGR_REVERSED("NiUIManager::LeftMouseButton", 0x00837690, &LeftMouseButton);
NIUIMGR_REVERSED("NiUIManager::~NiUIManager", 0x00837750, &Destruct);
NIUIMGR_REVERSED("NiUIManager::ReserveMouseButton", 0x00837A90, &ReserveMouseButton);
NIUIMGR_REVERSED("NiUIManager::NiUIManager", 0x00837B40, &Construct);
NIUIMGR_REVERSED("NiUIManager::DeletingDestructor", 0x00837DB0, &DeletingDestructor);
NIUIMGR_REVERSED("NiUIManager::Create", 0x00837DE0, &Create);
NIUIMGR_REVERSED("NiUIManager::Initialize", 0x00837E20, &Initialize);
NIUIMGR_REVERSED("NiUIManager::AddUIGroup", 0x00838210, &AddUIGroup);
NIUIMGR_REVERSED("NiUIManager::FindGamePadAxisBinding", 0x00838320, &FindGamePadAxisBinding);
NIUIMGR_REVERSED("NiUIManager::UnreserveMouse", 0x00838710, &UnreserveMouse);
NIUIMGR_REVERSED("NiUIManager::UnreserveMouseButton", 0x00838740, &UnreserveMouseButton);
NIUIMGR_REVERSED("NiUIManager::UnreserveGamePadButton", 0x008387A0, &UnreserveGamePadButton);
NIUIMGR_REVERSED("NiUIManager::UnreserveGamePadAxis", 0x00838800, &UnreserveGamePadAxis);
NIUIMGR_REVERSED("NiUIManager::UnreserveKeyboardButton", 0x00838850, &UnreserveKeyboardButton);
NIUIMGR_REVERSED("NiUIManager::ReserveMouse", 0x008388B0, &ReserveMouse);
NIUIMGR_REVERSED("NiUIManager::ReserveGamePadAxis", 0x00838900, &ReserveGamePadAxis);

} // namespace ni::sample
