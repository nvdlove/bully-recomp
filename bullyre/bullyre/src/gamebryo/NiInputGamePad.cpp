// NiInputGamePad (src/gamebryo/NiInput.h), 0x8D0860..0x8D1150: the base game pad over its
// buttons and axes: construction, the queries the game asks (a button down, pressed, released,
// an axis' value), pressing and releasing a button, and reporting the mapped actions its
// buttons and axes trigger to the input system.

#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiInput.h"
#include "reversed.h"

namespace ni::input {
namespace {

GAME_VAR(uint8_t, NiInputGamePad_RTTI, 0x020E96C4);
GAME_VAR(uint32_t, gGamePadAxisCount, 0x00BCBB20);
GAME_VAR(int32_t, gButtonPressThreshold, 0x00BCBB24);   // a value at least this: pressed
GAME_VAR(float, gAxisDeadZone, 0x00BCBB28);

constexpr uintptr_t NiInputGamePad_vtable = 0x009A1B10;

GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* construct);
GAME_FN(uint8_t, __thiscall, Button_WasPressed, 0x00836880, const GamePadButton* self, uint32_t* modifiers);
GAME_FN(uint8_t, __thiscall, Button_WasReleased, 0x008368A0, const GamePadButton* self, uint32_t* modifiers);
GAME_FN(void, __thiscall, Axis_SetRaw, 0x008D3EB0, GamePadAxis* self, int32_t raw, uint8_t flip, float deadZone);
GAME_FN(void, __thiscall, Device_Construct, 0x008D4080, NiInputDevice* self, NiInputDeviceDescription* description);
GAME_FN(void, __thiscall, Device_Destruct, 0x008D4110, NiInputDevice* self);
GAME_FN(uint8_t, __thiscall, Device_ActionApplies, 0x008D41C0, NiInputDevice* self, NiAction* action,
        uint32_t modifiers);
GAME_FN(uint32_t, __thiscall, InputSystem_ReportAction, 0x008D3DA0, void* system, NiInputDevice* device,
        uint32_t appData, uint32_t sequence, uint32_t data, uint32_t context);
// this file's, called where modes apply
GAME_FN(void, __thiscall, GamePad_Destruct, 0x008D0A00, NiInputGamePad* self);

template <typename R = void, typename... A>
R Virtual(const void* object, int slot, A... args) {
    return reinterpret_cast<R(__thiscall*)(const void*, re::Arg<A>...)>(static_cast<void*>(VtableOf(object)[slot]))(object, args...);
}

// the game pad semantics of the axes (0x10000001..0x10000008)
bool IsAxisSemantic(uint32_t semantic) {
    return semantic >= 0x10000001 && semantic <= 0x10000008;
}

// a value inside an action's range
bool InRange(const NiAction* action, int32_t value) {
    return value >= action->rangeLow && value <= action->rangeHigh;
}

// vf3 UpdateDevice (the base's: the "went down this update" mask cleared)
uint32_t __fastcall BeginUpdate(NiInputGamePad* self, re::Edx) {
    self->pressedNow = 0;
    return 0;
}

// an axis' raw reading: inverted when the pad says so, through the global dead zone
void __fastcall SetAxis(NiInputGamePad* self, re::Edx, uint8_t axis, int32_t raw) {
    uint32_t bit = 1u << (axis & 31);
    Axis_SetRaw(&self->axes[axis], raw, uint8_t((self->invertedAxes & bit) != 0), gAxisDeadZone);
}

GamePadButton* __fastcall ButtonConstruct(GamePadButton* self, re::Edx) {
    self->value = 0;
    self->pressed = 0;
    self->lastValue = 0;
    self->lastPressed = 0;
    self->field4 = 0;
    self->modifiers = 0;
    self->lastModifiers = 0;
    return self;
}

GamePadAxis* __fastcall AxisConstruct(GamePadAxis* self, re::Edx) {
    self->raw = 0;
    self->value = 0;
    self->rawLow = 0;
    self->rawHigh = 0xFF;
    self->low = -1;
    self->high = 1;
    return self;
}

// the constructor: 16 buttons, the global count of axes (each mapped to low..high)
NiInputGamePad* __fastcall Construct(NiInputGamePad* self, re::Edx, NiInputDeviceDescription* description, int32_t low,
                                     int32_t high) {
    Device_Construct(self, description);
    self->vtable = reinterpret_cast<void*>(NiInputGamePad_vtable);
    self->held = 0;
    self->pressedNow = 0;
    self->buttons = nullptr;
    self->axes = nullptr;
    self->field3C = nullptr;
    self->invertedAxes = 0;
    self->buttonCount = 16;
    void* buttons = NiNewArray(16 * sizeof(GamePadButton));
    if (buttons != nullptr) {
        VectorConstruct(buttons, sizeof(GamePadButton), 16, reinterpret_cast<void*>(0x008D08B0));
    }
    self->buttons = static_cast<GamePadButton*>(buttons);
    uint32_t count = gGamePadAxisCount;
    self->axisCount = count;
    // new[] with the count before the elements (the size saturating as the compiler's does)
    uint64_t bytes = uint64_t(count) * sizeof(GamePadAxis);
    uint32_t size = bytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(bytes);
    size = size > 0xFFFFFFFBu ? 0xFFFFFFFFu : size + 4;
    auto* block = static_cast<uint32_t*>(NiNewArray(size));
    GamePadAxis* axes = nullptr;
    if (block != nullptr) {
        *block = count;
        axes = reinterpret_cast<GamePadAxis*>(block + 1);
        VectorConstruct(axes, sizeof(GamePadAxis), count, reinterpret_cast<void*>(0x008D08D0));
    }
    self->axes = axes;
    for (uint32_t i = 0; i < self->axisCount; i++) {
        self->axes[i].low = low;
        self->axes[i].high = high;
    }
    return self;
}

void* __fastcall GetRTTI(NiInputGamePad*, re::Edx) {
    return &NiInputGamePad_RTTI;
}

void __fastcall Destruct(NiInputGamePad* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiInputGamePad_vtable);
    NiFree(self->field3C);
    if (self->axes != nullptr) {
        NiDeleteArray(re::PtrCast<uint32_t*>(self->axes) - 1, sizeof(GamePadAxis));
    }
    NiDeleteArray(self->buttons, sizeof(GamePadButton));
    Device_Destruct(self);
}

// vf4 UpdateActionMap: the device updated (vf3); with mapped actions and no error, its button
// and axis actions reported (either failing: 0x80000000)
uint32_t __fastcall UpdateActionMap(NiInputGamePad* self, re::Edx, void* system) {
    uint32_t result = Virtual<uint32_t>(self, 3);
    if (self->mappedActions != nullptr && self->mappedActions->count != 0 && result == 0) {
        if (Virtual<uint8_t>(self, 16, system) == 0) {
            result = 0x80000000;
        }
        if (Virtual<uint8_t>(self, 17, system) == 0) {
            result = 0x80000000;
        }
    }
    return result;
}

// an axis' value (none: 0)
int32_t __fastcall GetAxisValue(NiInputGamePad* self, re::Edx, uint32_t axis) {
    if (self->axes != nullptr && axis < self->axisCount) {
        return self->axes[axis].value;
    }
    return 0;
}

// a button held, and the modifiers it went down with
uint8_t __fastcall ButtonIsDown(NiInputGamePad* self, re::Edx, uint32_t button, uint32_t* modifiers) {
    if (self->buttons != nullptr && button < self->buttonCount) {
        *modifiers = self->buttons[button].modifiers;
        return self->buttons[button].pressed;
    }
    return 0;
}

uint8_t __fastcall ButtonIsDownNoModifiers(NiInputGamePad* self, re::Edx, uint32_t button) {
    if (self->buttons != nullptr && button < self->buttonCount) {
        return self->buttons[button].pressed;
    }
    return 0;
}

// a button that went down since the last update
uint8_t __fastcall ButtonWasPressed(NiInputGamePad* self, re::Edx, uint32_t button, uint32_t* modifiers) {
    if (self->buttons != nullptr && button < self->buttonCount) {
        const GamePadButton& b = self->buttons[button];
        *modifiers = b.modifiers;
        return b.pressed != 0 && b.lastPressed == 0;
    }
    return 0;
}

uint8_t __fastcall ButtonWasPressedNoModifiers(NiInputGamePad* self, re::Edx, uint32_t button) {
    if (self->buttons != nullptr && button < self->buttonCount) {
        const GamePadButton& b = self->buttons[button];
        return b.pressed != 0 && b.lastPressed == 0;
    }
    return 0;
}

// a button that came up since the last update
uint8_t __fastcall ButtonWasReleased(NiInputGamePad* self, re::Edx, uint32_t button, uint32_t* modifiers) {
    if (self->buttons != nullptr && button < self->buttonCount) {
        const GamePadButton& b = self->buttons[button];
        *modifiers = b.modifiers;
        return b.pressed == 0 && b.lastPressed != 0;
    }
    return 0;
}

// vf7: nothing came in: every button and axis keeps its value as the last one
void __fastcall KeepValues(NiInputGamePad* self, re::Edx) {
    for (uint32_t i = 0; i < self->buttonCount; i++) {
        GamePadButton& b = self->buttons[i];
        b.lastValue = b.value;
        b.lastPressed = b.pressed;
        b.lastModifiers = b.modifiers;
    }
    for (uint32_t i = 0; i < self->axisCount; i++) {
        GamePadAxis& a = self->axes[i];
        a.lastRaw = a.raw;
        a.lastValue = a.value;
    }
}

// vf8: whether a mapped action already uses this control (its semantic's group the same, and
// with a device given, the same device; with a range given, overlapping one of its ends)
uint8_t __fastcall IsControlMapped(NiInputGamePad* self, re::Edx, uint32_t control, uint32_t semantic,
                                   uint8_t checkDevice, uint32_t deviceID, uint8_t checkRange, int32_t low,
                                   int32_t high) {
    if (self->mappedActions == nullptr) {
        return 0;
    }
    for (NiTListNode<NiAction*>* node = self->mappedActions->head; node != nullptr;) {
        NiAction* action = node->element;
        node = node->next;
        if (action == nullptr || ((action->semantic ^ semantic) & 0xFFFFF000) != 0 || action->control != control) {
            continue;
        }
        if (checkDevice != 0 && action->modifiers != deviceID) {
            return 0;
        }
        if (checkRange != 0 && ((low <= action->rangeLow && action->rangeHigh <= low) ||
                                (high <= action->rangeLow && action->rangeHigh <= high))) {
            return 0;
        }
        return 1;
    }
    return 0;
}

// vf16: the button actions reported (each that applies, when its button changed as the action
// asks: continuously, on any change of value, or as it went down or up; with a range, only on
// entering or leaving it, its value 0 outside; as a bool when asked)
uint8_t __fastcall ReportButtonActions(NiInputGamePad* self, re::Edx, void* system) {
    for (NiTListNode<NiAction*>* node = self->mappedActions->head; node != nullptr;) {
        NiAction* action = node->element;
        node = node->next;
        if (action == nullptr) {
            continue;
        }
        uint32_t flags = action->flags;
        uint32_t control = action->control;
        if (IsAxisSemantic(action->semantic)) {
            continue;
        }
        uint8_t applies = Device_ActionApplies(self, action, action->modifiers);
        const GamePadButton* button = &self->buttons[control];
        int32_t last = button->lastValue;
        uint8_t pressed = button->pressed;
        int32_t value = button->value;
        uint32_t ignored;
        uint8_t wentDown = Button_WasPressed(button, &ignored);
        uint8_t wentUp = Button_WasReleased(&self->buttons[control], &ignored);
        if (applies == 0) {
            continue;
        }
        if ((flags & 0x1000) == 0) {
            if (flags & 0x80) {
                if (value == last) {
                    continue;
                }
            } else if ((pressed != 0 ? wentDown : wentUp) == 0) {
                continue;
            }
        }
        if (flags & 0x40) {
            bool inNow = InRange(action, value);
            if (!inNow) {
                value = 0;
            }
            bool inLast = InRange(action, last);
            if ((flags & 0x1000) == 0 && inNow == inLast) {
                continue;
            }
        }
        if (flags & 0x100) {
            value = pressed != 0;
        }
        InputSystem_ReportAction(system, self, action->appData, 0, uint32_t(value), action->context);
    }
    return 1;
}

// vf17: the axis actions reported (each that applies, its axis' value when it changed or
// continuously; with a range, only on entering or leaving it; as a bool when asked)
uint8_t __fastcall ReportAxisActions(NiInputGamePad* self, re::Edx, void* system) {
    for (NiTListNode<NiAction*>* node = self->mappedActions->head; node != nullptr;) {
        NiAction* action = node->element;
        node = node->next;
        if (action == nullptr) {
            continue;
        }
        uint32_t flags = action->flags;
        uint32_t control = action->control;
        if (!IsAxisSemantic(action->semantic) || Device_ActionApplies(self, action, action->modifiers) == 0) {
            continue;
        }
        // the control: a stick (1, 2, 4, 8 in the high half) and its axis (1, 2)
        int axis;
        switch (control) {
        case 0x10001: axis = 0; break;
        case 0x10002: axis = 1; break;
        case 0x20001: axis = 2; break;
        case 0x20002: axis = 3; break;
        case 0x40001: axis = 4; break;
        case 0x40002: axis = 5; break;
        case 0x80001: axis = 6; break;
        case 0x80002: axis = 7; break;
        default: continue;
        }
        int32_t value = self->axes[axis].value;
        int32_t last = self->axes[axis].lastValue;
        if ((flags & 0x1000) == 0 && value == last) {
            continue;
        }
        uint32_t data;
        if (flags & 0x40) {
            bool inNow = InRange(action, value);
            bool inLast = InRange(action, last);
            if ((flags & 0x1000) == 0 && inNow == inLast) {
                continue;
            }
            data = (flags & 0x100) ? uint32_t(inNow) : uint32_t(value);
        } else {
            data = (flags & 0x100) && value != 0 ? 1 : uint32_t(value);
        }
        InputSystem_ReportAction(system, self, action->appData, 0, data, action->context);
    }
    return 1;
}

NiInputGamePad* __fastcall DeletingDestructor(NiInputGamePad* self, re::Edx, uint32_t flags) {
    GamePad_Destruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(NiInputGamePad));
    }
    return self;
}

// a button's new value: the old one kept as the last; at the global threshold or above it's
// down (in the device's modifiers and held; newly held, in this update's pressed), below, up
void __fastcall PressButton(NiInputGamePad* self, re::Edx, uint32_t index, uint8_t value) {
    int32_t threshold = gButtonPressThreshold;
    if (index >= self->buttonCount) {
        return;
    }
    uint32_t bit = 1u << (index & 31);
    GamePadButton& b = self->buttons[index];
    uint32_t modifiers = self->modifiers;
    b.lastValue = b.value;
    b.lastPressed = b.pressed;
    b.value = value;
    b.lastModifiers = b.modifiers;
    b.modifiers = modifiers;
    if (threshold <= int32_t(value)) {
        b.pressed = 1;
        self->modifiers |= bit;
        if ((bit & self->held) == 0) {
            self->pressedNow |= bit;
        }
        self->held |= bit;
        return;
    }
    b.pressed = 0;
    self->modifiers &= ~bit;
    if ((bit & self->held) != 0) {
        self->held &= ~bit;
    }
}

// a button let go: its value, state and modifiers cleared (the old kept as the last), its
// bit out of the device's modifiers, the held mask and every other button's modifiers
void __fastcall ReleaseButton(NiInputGamePad* self, re::Edx, uint32_t index) {
    if (index >= self->buttonCount) {
        return;
    }
    GamePadButton& b = self->buttons[index];
    uint32_t bit = 1u << (index & 31);
    b.lastValue = b.value;
    b.lastPressed = b.pressed;
    b.lastModifiers = b.modifiers;
    b.value = 0;
    b.pressed = 0;
    b.modifiers = 0;
    self->modifiers &= ~bit;
    if ((bit & self->held) != 0) {
        self->held &= ~bit;
    }
    for (uint32_t i = 0; i < self->buttonCount; i++) {
        if (i != index) {
            uint32_t m = self->buttons[i].modifiers;
            if ((bit & m) != 0) {
                self->buttons[i].modifiers = m & ~bit;
            }
        }
    }
}

} // namespace
} // namespace ni::input

// ---- registration ---------------------------------------------------------------------

#define NIGAMEPAD_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIGAMEPAD_REVERSED("NiInputGamePad::UpdateDevice", 0x008D0860, &ni::input::BeginUpdate);
NIGAMEPAD_REVERSED("NiInputGamePad::SetAxis", 0x008D0870, &ni::input::SetAxis);
NIGAMEPAD_REVERSED("NiInputGamePad::Button::Button", 0x008D08B0, &ni::input::ButtonConstruct);
NIGAMEPAD_REVERSED("NiInputGamePad::Axis::Axis", 0x008D08D0, &ni::input::AxisConstruct);
NIGAMEPAD_REVERSED("NiInputGamePad::NiInputGamePad", 0x008D0900, &ni::input::Construct);
NIGAMEPAD_REVERSED("NiInputGamePad::GetRTTI", 0x008D09F0, &ni::input::GetRTTI);
NIGAMEPAD_REVERSED("NiInputGamePad::~NiInputGamePad", 0x008D0A00, &ni::input::Destruct);
NIGAMEPAD_REVERSED("NiInputGamePad::UpdateActionMap", 0x008D0A40, &ni::input::UpdateActionMap);
NIGAMEPAD_REVERSED("NiInputGamePad::GetAxisValue", 0x008D0AA0, &ni::input::GetAxisValue);
NIGAMEPAD_REVERSED("NiInputGamePad::ButtonIsDown", 0x008D0AC0, &ni::input::ButtonIsDown);
NIGAMEPAD_REVERSED("NiInputGamePad::ButtonIsDown(no modifiers)", 0x008D0AF0, &ni::input::ButtonIsDownNoModifiers);
NIGAMEPAD_REVERSED("NiInputGamePad::ButtonWasPressed", 0x008D0B10, &ni::input::ButtonWasPressed);
NIGAMEPAD_REVERSED("NiInputGamePad::ButtonWasPressed(no modifiers)", 0x008D0B50,
                   &ni::input::ButtonWasPressedNoModifiers);
NIGAMEPAD_REVERSED("NiInputGamePad::ButtonWasReleased", 0x008D0B80, &ni::input::ButtonWasReleased);
NIGAMEPAD_REVERSED("NiInputGamePad::KeepValues", 0x008D0BC0, &ni::input::KeepValues);
NIGAMEPAD_REVERSED("NiInputGamePad::IsControlMapped", 0x008D0C20, &ni::input::IsControlMapped);
NIGAMEPAD_REVERSED("NiInputGamePad::ReportButtonActions", 0x008D0CA0, &ni::input::ReportButtonActions);
NIGAMEPAD_REVERSED("NiInputGamePad::ReportAxisActions", 0x008D0E40, &ni::input::ReportAxisActions);
NIGAMEPAD_REVERSED("NiInputGamePad::DeletingDestructor", 0x008D1070, &ni::input::DeletingDestructor);
NIGAMEPAD_REVERSED("NiInputGamePad::PressButton", 0x008D10D0, &ni::input::PressButton);
NIGAMEPAD_REVERSED("NiInputGamePad::ReleaseButton", 0x008D1150, &ni::input::ReleaseButton);
