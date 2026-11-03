// NiInputMouse / NiInputDI8Mouse and NiInputKeyboard / NiInputDI8Keyboard (src/gamebryo/NiInput.h),
// 0x8D42D0..0x8D68B0: constructing them over a DirectInput device (data format, cooperative
// level, a 64-event buffer, acquired), reading DirectInput's buffered data each update, the
// keys' and buttons' state, and reporting the mapped actions they trigger to the input system.
//
// The devices' vtables: 0 deleting destructor, 2 GetRTTI, 3 UpdateDevice, 4 UpdateActionMap,
// 5 Unacquire, 6 Acquire, 7 KeepValues (the state kept as the last), 8 IsControlMapped,
// 9 AddMappedAction; the mouse's 10 and 11 report its button and axis actions.

#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiInput.h"
#include "reversed.h"
#include "runflags.h"
#include "sysabi.h"

namespace ni::input {
namespace {

GAME_VAR(uint8_t, NiInputDI8Mouse_RTTI, 0x020E9810);
GAME_VAR(uint8_t, NiInputDI8Keyboard_RTTI, 0x020E9844);
GAME_VAR(uint8_t, NiInputMouse_RTTI, 0x020E98B0);
GAME_VAR(uint8_t, NiInputKeyboard_RTTI, 0x020E98E4);
GAME_VAR(void*, gWindow, 0x020E96FC);   // the window of the create parameters

constexpr uintptr_t NiInputMouse_vtable = 0x009A1ECC;
constexpr uintptr_t NiInputDI8Mouse_vtable = 0x009A1DF0;
constexpr uintptr_t NiInputKeyboard_vtable = 0x009A1F0C;
constexpr uintptr_t NiInputDI8Keyboard_vtable = 0x009A1E30;
constexpr uintptr_t c_dfDIMouse2 = 0x009A2714;
constexpr uintptr_t c_dfDIKeyboard = 0x009A2304;

GAME_FN(uint32_t, __thiscall, ReturnZero, 0x00824EC0, void* self);   // (NiInputMouse's vf3: nothing)
GAME_FN(void, __thiscall, Device_Construct, 0x008D4080, NiInputDevice* self, NiInputDeviceDescription* description);
GAME_FN(void, __thiscall, Device_Destruct, 0x008D4110, NiInputDevice* self);
GAME_FN(uint8_t, __thiscall, Device_ActionApplies, 0x008D41C0, NiInputDevice* self, NiAction* action,
        uint32_t modifiers);
GAME_FN(uint32_t, __thiscall, InputSystem_ReportAction, 0x008D3DA0, void* system, NiInputDevice* device,
        uint32_t appData, uint32_t sequence, uint32_t data, uint32_t context);
GAME_FN(uint8_t, __thiscall, Button_WasPressed, 0x00836880, const GamePadButton* self, uint32_t* modifiers);
GAME_FN(uint8_t, __thiscall, Button_WasReleased, 0x008368A0, const GamePadButton* self, uint32_t* modifiers);
GAME_FN(uint8_t, __thiscall, Keyboard_KeyWasPressed, 0x00828E50, const NiInputKeyboard* self, int32_t key);
// this file's, called where modes apply
GAME_FN(void, __thiscall, DI8Mouse_PressButton, 0x008D44B0, NiInputDI8Mouse* self, int32_t index);
GAME_FN(void, __thiscall, DI8Mouse_HandleButton, 0x008D4500, NiInputDI8Mouse* self, uint32_t data, int32_t index);
GAME_FN(void, __thiscall, DI8Keyboard_BuildKeyCodes, 0x008D4980, NiInputDI8Keyboard* self);
GAME_FN(uint8_t, __thiscall, Keyboard_KeyIsDown, 0x008D4F30, const NiInputKeyboard* self, int32_t key);
GAME_FN(uint32_t, __thiscall, Keyboard_KeyModifier, 0x008D4F60, const NiInputKeyboard* self, int32_t key);
GAME_FN(void, __thiscall, Keyboard_PressKey, 0x008D50C0, NiInputKeyboard* self, int32_t key);
GAME_FN(void, __thiscall, DI8Keyboard_ReadState, 0x008D52A0, NiInputDI8Keyboard* self);
GAME_FN(void, __thiscall, Mouse_Construct, 0x008D5F20, NiInputMouse* self, NiInputDeviceDescription* description);
GAME_FN(void, __thiscall, Mouse_Destruct, 0x008D6100, NiInputMouse* self);
GAME_FN(void, __thiscall, Keyboard_Construct, 0x008D6630, NiInputKeyboard* self,
        NiInputDeviceDescription* description);
GAME_FN(void, __thiscall, Keyboard_Destruct, 0x008D6690, NiInputKeyboard* self);
GAME_FN(uint32_t, __thiscall, Keyboard_UpdateDevice, 0x008D66A0, NiInputKeyboard* self);
GAME_FN(uint8_t, __thiscall, Keyboard_KeyWasReleased, 0x008D66C0, const NiInputKeyboard* self, int32_t key);

template <typename R = void, typename... A>
R Virtual(const void* object, int slot, A... args) {
    return reinterpret_cast<R(__thiscall*)(const void*, re::Arg<A>...)>(static_cast<void*>(VtableOf(object)[slot]))(object, args...);
}

// a DirectInput (COM) call by slot
template <typename... A>
int32_t Com(void* object, int slot, A... args) {
    return re::ComCall<int32_t>(object, slot, args...);
}
enum : int {
    kComRelease = 2,
    kDISetProperty = 6,              // IDirectInputDevice8A
    kDIAcquire = 7,
    kDIUnacquire = 8,
    kDIGetDeviceState = 9,
    kDIGetDeviceData = 10,
    kDISetDataFormat = 11,
    kDISetCooperativeLevel = 13,
};

struct ObjectData {   // DIDEVICEOBJECTDATA (the system's: uAppData a UINT_PTR)
    uint32_t offset, data, timeStamp, sequence;
    uintptr_t appData;
};
static_assert(sizeof(void*) != 4 || sizeof(ObjectData) == 0x14);

// DirectInput's "lost the device" results: input lost, not acquired, another application has it
bool IsLost(int32_t hr) {
    return hr == int32_t(0x8007001E) || hr == int32_t(0x8007000C) || hr == int32_t(0x80070005);
}

// the create parameters' cooperation flags as DirectInput's (foreground 4, background 8,
// exclusive 1, not exclusive 2)
uint32_t CooperativeLevel(uint32_t flags) {
    uint32_t level = 0;
    if (flags & 0x10000000) {
        level = 4;
    }
    if (flags & 0x20000000) {
        level |= 8;
    }
    if (flags & 0x40000000) {
        level |= 1;
    }
    if (int32_t(flags) < 0) {
        level |= 2;
    }
    if (re::PlayWindow()) {
        level = (level & ~1u) | 2u;   // (bullyre_x64 played: shared with Windows, runflags.h)
    }
    return level;
}

// DIPROP_BUFFERSIZE for the device (DIPROPDWORD, DIPH_DEVICE)
int32_t SetBufferSize(void* device, uint32_t size) {
    struct {
        uint32_t size, headerSize, object, how, data;
    } property = {0x14, 0x10, 0, 0, size};
    return Com(device, kDISetProperty, uintptr_t(1), &property);
}

// a key's bit (key >> 3 as the game's arithmetic shift)
uint8_t KeyBit(const uint8_t* bits, int32_t key) {
    return uint8_t((bits[key >> 3] >> (key & 7)) & 1);
}

// a key let go, as the updates inline it: its bit cleared and its modifier with it
void ReleaseKey(NiInputKeyboard* self, int32_t key) {
    if (key != 0 && KeyBit(self->keysDown, key) != 0) {
        self->keysDown[key >> 3] &= uint8_t(~(1u << (key & 7)));
        self->modifiers &= ~Keyboard_KeyModifier(self, key);
    }
}

// ---- NiInputMouse

// the constructor: the three axes (raw range 0..255, mapped -1..1) and eight buttons cleared;
// each axis' value then kept as its last (zero)
NiInputMouse* __fastcall MouseConstruct(NiInputMouse* self, re::Edx, NiInputDeviceDescription* description) {
    Device_Construct(self, description);
    self->vtable = reinterpret_cast<void*>(NiInputMouse_vtable);
    for (GamePadAxis& axis : self->axes) {
        axis.raw = 0;
        axis.value = 0;
        axis.rawLow = 0;
        axis.rawHigh = 0xFF;
        axis.low = -1;
        axis.high = 1;
    }
    for (GamePadButton& button : self->buttons) {
        button.value = 0;
        button.pressed = 0;
        button.lastValue = 0;
        button.lastPressed = 0;
        button.field4 = 0;
        button.modifiers = 0;
        button.lastModifiers = 0;
    }
    for (GamePadAxis& axis : self->axes) {
        axis.lastValue = axis.value;
        axis.value = 0;
    }
    return self;
}

void* __fastcall MouseGetRTTI(NiInputMouse*, re::Edx) {
    return &NiInputMouse_RTTI;
}

void __fastcall MouseDestruct(NiInputMouse* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiInputMouse_vtable);
    Device_Destruct(self);
}

// vf4 UpdateActionMap: the device updated (vf3); with mapped actions and no error, its button
// and axis actions reported (either failing: 0x80000000)
uint32_t __fastcall MouseUpdateActionMap(NiInputMouse* self, re::Edx, void* system) {
    uint32_t result = Virtual<uint32_t>(self, 3);
    if (self->mappedActions != nullptr && self->mappedActions->count != 0 && result == 0) {
        if (Virtual<uint8_t>(self, 10, system) == 0) {
            result = 0x80000000;
        }
        if (Virtual<uint8_t>(self, 11, system) == 0) {
            result = 0x80000000;
        }
    }
    return result;
}

// vf7: every axis and button keeps its state as the last
void __fastcall MouseKeepValues(NiInputMouse* self, re::Edx) {
    for (GamePadAxis& axis : self->axes) {
        axis.lastRaw = axis.raw;
        axis.lastValue = axis.value;
    }
    for (GamePadButton& button : self->buttons) {
        button.lastPressed = button.pressed;
        button.lastValue = button.value;
        button.lastModifiers = button.modifiers;
    }
}

// vf8 IsControlMapped: whether a mapped action of the same semantic group already uses the
// control: when the new one needs modifiers, one that needs the same mouse modifiers; else
// one that needs none. (The last three arguments aren't read.)
uint8_t __fastcall MouseIsControlMapped(NiInputMouse* self, re::Edx, uint32_t control, uint32_t semantic,
                                        uint8_t useModifiers, uint32_t modifiers, uint32_t, uint32_t, uint32_t) {
    if (self->mappedActions == nullptr) {
        return 0;
    }
    for (NiTListNode<NiAction*>* node = self->mappedActions->head; node != nullptr;) {
        NiAction* action = node->element;
        node = node->next;
        if (action == nullptr || ((action->semantic ^ semantic) & 0xFFFFF000) != 0 || action->control != control) {
            continue;
        }
        uint8_t needs = uint8_t((action->flags >> 9) & 1);
        if (useModifiers != 0) {
            if (needs != 0 && action->mouseModifiers == modifiers) {
                return 1;
            }
        } else if (needs == 0) {
            return 1;
        }
    }
    return 0;
}

// vf10: the button actions reported (each that applies, as its button went down or up, or
// continuously; its value, or whether it's down when asked)
uint8_t __fastcall MouseReportButtonActions(NiInputMouse* self, re::Edx, void* system) {
    for (NiTListNode<NiAction*>* node = self->mappedActions->head; node != nullptr;) {
        NiAction* action = node->element;
        node = node->next;
        if (action == nullptr) {
            continue;
        }
        uint32_t flags = action->flags;
        uint32_t control = action->control;
        uint32_t semantic = action->semantic;
        if (semantic == 0x40000001 || semantic == 0x40000002 || semantic == 0x40000003 || semantic == 0x40010001 ||
            semantic == 0x40010002 || semantic == 0x40010003) {
            continue;   // the axes
        }
        if (Device_ActionApplies(self, action, action->mouseModifiers) == 0) {
            continue;
        }
        const GamePadButton* button = &self->buttons[control];
        uint32_t value = button->value;
        uint8_t pressed = button->pressed;
        uint32_t ignored;
        uint8_t wentDown = Button_WasPressed(button, &ignored);
        uint8_t wentUp = Button_WasReleased(button, &ignored);
        if ((flags & 0x1000) == 0 && (pressed != 0 ? wentDown : wentUp) == 0) {
            continue;
        }
        if (flags & 0x100) {
            value = pressed != 0;
        }
        InputSystem_ReportAction(system, self, action->appData, 0, value, action->context);
    }
    return 1;
}

// vf11: the axis actions reported (each that applies, its axis' movement when it changed or
// continuously; with a range, only on entering or leaving it; as a bool when asked)
uint8_t __fastcall MouseReportAxisActions(NiInputMouse* self, re::Edx, void* system) {
    for (NiTListNode<NiAction*>* node = self->mappedActions->head; node != nullptr;) {
        NiAction* action = node->element;
        node = node->next;
        if (action == nullptr) {
            continue;
        }
        uint32_t semantic = action->semantic;
        if (semantic >= 0x40001000 && semantic <= 0x40001007) {
            continue;   // the buttons
        }
        uint32_t flags = action->flags;
        uint32_t control = action->control;
        if (Device_ActionApplies(self, action, action->mouseModifiers) == 0 || control > 2) {
            continue;
        }
        int32_t value = self->axes[control].value;
        int32_t last = self->axes[control].lastValue;
        if ((flags & 0x1000) == 0 && value == last) {
            continue;
        }
        uint32_t data;
        if (flags & 0x40) {
            bool inNow = value >= action->rangeLow && value <= action->rangeHigh;
            bool inLast = last >= action->rangeLow && last <= action->rangeHigh;
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

NiInputMouse* __fastcall MouseDeletingDestructor(NiInputMouse* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiInputMouse_vtable);
    Device_Destruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(NiInputMouse));
    }
    return self;
}

// ---- NiInputDI8Mouse

// vf5
uint32_t __fastcall DI8MouseUnacquire(NiInputDI8Mouse* self, re::Edx) {
    Com(self->device, kDIUnacquire);
    return kOk;
}

// vf6 (lost: 0x80000006; other failures 0x80000009)
uint32_t __fastcall DI8MouseAcquire(NiInputDI8Mouse* self, re::Edx) {
    int32_t hr = Com(self->device, kDIAcquire);
    if (hr == 0) {
        self->lastError = kOk;
    } else if (IsLost(hr)) {
        self->lastError = kErrAcquireFailed;
    } else {
        self->lastError = kErrNotAcquired;
    }
    return self->lastError;
}

void* __fastcall DI8MouseGetRTTI(NiInputDI8Mouse*, re::Edx) {
    return &NiInputDI8Mouse_RTTI;
}

// the constructor: the device set to DIMOUSESTATE2's format, the cooperative level asked for, a
// buffer of 64 events, and acquired (any step failing: 0x80000008; not acquired: 0x80000006)
NiInputDI8Mouse* __fastcall DI8MouseConstruct(NiInputDI8Mouse* self, re::Edx, NiInputDeviceDescription* description,
                                              void* device, uint32_t cooperation) {
    Mouse_Construct(self, description);
    self->vtable = reinterpret_cast<void*>(NiInputDI8Mouse_vtable);
    self->device = device;
    self->bufferSize = 0x40;
    if (device == nullptr) {
        return self;
    }
    if (Com(device, kDISetDataFormat, re::DIDataFormat(reinterpret_cast<const void*>(c_dfDIMouse2)).get()) != 0 ||
        Com(self->device, kDISetCooperativeLevel, gWindow, CooperativeLevel(cooperation)) != 0 ||
        SetBufferSize(self->device, self->bufferSize) != 0) {
        self->lastError = 0x80000008;
        return self;
    }
    if (Com(self->device, kDIAcquire) != 0) {
        self->lastError = kErrAcquireFailed;
    }
    return self;
}

NiInputDI8Mouse* __fastcall DI8MouseDeletingDestructor(NiInputDI8Mouse* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiInputDI8Mouse_vtable);
    if (self->device != nullptr) {
        Com(self->device, kDIUnacquire);
        Com(self->device, kComRelease);
    }
    self->device = nullptr;
    Mouse_Destruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(NiInputDI8Mouse));
    }
    return self;
}

// a button down (0..7): the old state kept as the last, its value 255, the device's modifiers
// with it, its bit in them
void __fastcall DI8MousePressButton(NiInputDI8Mouse* self, re::Edx, int32_t index) {
    if (index >= 8) {
        return;
    }
    GamePadButton& button = self->buttons[index];
    uint32_t modifiers = self->modifiers;
    button.lastValue = button.value;
    button.lastPressed = button.pressed;
    button.lastModifiers = button.modifiers;
    button.modifiers = modifiers;
    button.value = 0xFF;
    button.pressed = 1;
    self->modifiers |= 1u << (index & 31);
}

// a buffered button event: marked changed; down (0x80) pressed, else let go (the old state kept
// as the last, cleared, its bit out of the modifiers)
void __fastcall DI8MouseHandleButton(NiInputDI8Mouse* self, re::Edx, uint32_t data, int32_t index) {
    uint32_t bit = 1u << (index & 31);
    self->buttonsChanged |= bit;
    if (data & 0x80) {
        DI8Mouse_PressButton(self, index);
        return;
    }
    if (index >= 8) {
        return;
    }
    GamePadButton& button = self->buttons[index];
    button.lastValue = button.value;
    button.lastPressed = button.pressed;
    button.lastModifiers = button.modifiers;
    button.value = 0;
    button.pressed = 0;
    button.modifiers = 0;
    self->modifiers &= ~bit;
}

// vf3 UpdateDevice: the buffered events read one at a time (acquired before each read): the
// axes' movement summed, the buttons pressed and let go; then each axis' sum is its value (the
// old one kept as the last), the buttons no event changed keep their state as the last, and
// what's left in the buffer is flushed
uint32_t __fastcall DI8MouseUpdate(NiInputDI8Mouse* self, re::Edx) {
    ReturnZero(self);
    int32_t z = 0;
    if (self->device == nullptr) {
        self->lastError = kErrDeviceLost;
        return kErrDeviceLost;
    }
    int32_t x = 0;
    int32_t y = 0;
    self->buttonsChanged = 0;
    Virtual<uint32_t>(self, 6);
    ObjectData event;
    uint32_t count = 1;
    int32_t hr = Com(self->device, kDIGetDeviceData, uint32_t(sizeof(ObjectData)), &event, &count, 0u);
    bool done = false;
    while (hr == 0) {
        if (count == 0) {
            self->lastError = kOk;
            done = true;
            break;
        }
        switch (event.offset) {   // DIMOUSESTATE2's
        case 0x0:
            x += int32_t(event.data);
            break;
        case 0x4:
            y += int32_t(event.data);
            break;
        case 0x8:
            z += int32_t(event.data);
            break;
        case 0xC:
        case 0xD:
        case 0xE:
        case 0xF:
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
            DI8Mouse_HandleButton(self, event.data, int32_t(event.offset) - 0xC);
            break;
        default:
            break;
        }
        Virtual<uint32_t>(self, 6);
        count = 1;
        hr = Com(self->device, kDIGetDeviceData, uint32_t(sizeof(ObjectData)), &event, &count, 0u);
    }
    if (!done) {
        self->lastError = IsLost(hr) ? kErrAcquireFailed : kErrNotAcquired;
    }
    int32_t sums[3] = {x, y, z};
    for (int i = 0; i < 3; i++) {
        self->axes[i].lastValue = self->axes[i].value;
        self->axes[i].value = sums[i];
    }
    for (int i = 0; i < 8; i++) {
        if ((self->buttonsChanged & (1u << i)) == 0) {
            GamePadButton& button = self->buttons[i];
            button.lastValue = button.value;
            button.lastPressed = button.pressed;
            button.lastModifiers = button.modifiers;
        }
    }
    count = 0xFFFFFFFF;   // INFINITE: the rest flushed
    Com(self->device, kDIGetDeviceData, uint32_t(sizeof(ObjectData)), static_cast<void*>(nullptr), &count, 0u);
    return self->lastError;
}

// ---- NiInputKeyboard

// the constructor: no keys down, caps lock off
NiInputKeyboard* __fastcall KeyboardConstruct(NiInputKeyboard* self, re::Edx, NiInputDeviceDescription* description) {
    Device_Construct(self, description);
    self->capsLock = 0;
    self->vtable = reinterpret_cast<void*>(NiInputKeyboard_vtable);
    for (int i = 0; i < 32; i++) {
        self->keysDown[i] = 0;
        self->lastKeysDown[i] = 0;
    }
    self->field64 = 0;
    return self;
}

void __fastcall KeyboardDestruct(NiInputKeyboard* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiInputKeyboard_vtable);
    Device_Destruct(self);
}

// vf3 UpdateDevice (the base's: the keys kept as the last, vf7)
uint32_t __fastcall KeyboardUpdateDevice(NiInputKeyboard* self, re::Edx) {
    Virtual(self, 7);
    return kOk;
}

void* __fastcall KeyboardGetRTTI(NiInputKeyboard*, re::Edx) {
    return &NiInputKeyboard_RTTI;
}

// a key down now (key 0: never)
uint8_t __fastcall KeyIsDown(const NiInputKeyboard* self, re::Edx, int32_t key) {
    if (key == 0) {
        return 0;
    }
    return KeyBit(self->keysDown, key);
}

// a key that came up since the last update
uint8_t __fastcall KeyWasReleased(const NiInputKeyboard* self, re::Edx, int32_t key) {
    if (key == 0) {
        return 0;
    }
    return KeyBit(self->keysDown, key) == 0 && KeyBit(self->lastKeysDown, key) != 0;
}

// the modifier a key is (the controls, alts, windows keys, shifts and caps lock)
uint32_t __fastcall KeyModifier(const NiInputKeyboard*, re::Edx, int32_t key) {
    switch (key) {
    case 0x1D: return 0x1;     // left control
    case 0x6B: return 0x2;     // right control
    case 0x38: return 0x4;     // left alt
    case 0x76: return 0x8;     // right alt
    case 0x82: return 0x10;    // left windows
    case 0x83: return 0x20;    // right windows
    case 0x2A: return 0x40;    // left shift
    case 0x36: return 0x80;    // right shift
    case 0x3A: return 0x100;   // caps lock
    default: return 0;
    }
}

// a key down (unless it's down already; key 0 is set anyway): its bit and modifier set, caps
// lock toggled by its key
void __fastcall PressKey(NiInputKeyboard* self, re::Edx, int32_t key) {
    if (key != 0 && KeyBit(self->keysDown, key) != 0) {
        return;
    }
    self->keysDown[key >> 3] |= uint8_t(1u << (key & 7));
    self->modifiers |= Keyboard_KeyModifier(self, key);
    if (key == 0x3A) {
        self->capsLock = self->capsLock == 0;
    }
}

// vf4 UpdateActionMap: the device updated (vf3, its error returned); each mapped action that
// applies (keyboard modifiers) and names a key code up to 0x90 reported: the key's value (255
// down, or whether it's down when asked), continuously, or as it went down or came up (held
// keys not reported)
uint32_t __fastcall KeyboardUpdateActionMap(NiInputKeyboard* self, re::Edx, void* system) {
    uint32_t result = Virtual<uint32_t>(self, 3);
    if (result != 0) {
        return result;
    }
    NiTPointerList<NiAction*>* list = self->mappedActions;
    if (list == nullptr || list->count == 0) {
        return 0;
    }
    for (NiTListNode<NiAction*>* node = list->head; node != nullptr;) {
        NiAction* action = node->element;
        node = node->next;
        if (action == nullptr) {
            continue;
        }
        uint32_t flags = action->flags;
        if (Device_ActionApplies(self, action, action->keyboardModifiers) == 0) {
            continue;
        }
        // (the game looks for the control among the key codes 0..0x90)
        uint32_t control = action->control;
        if (control >= 0x91) {
            continue;
        }
        int32_t key = int32_t(control);
        uint32_t value = 0;
        if (key != 0 && KeyBit(self->keysDown, key) != 0) {
            value = 0xFF;
        }
        if ((flags & 0x100) && value != 0) {
            value = 1;
        }
        if ((flags & 0x1000) == 0) {
            if (key != 0 && KeyBit(self->keysDown, key) != 0 && Keyboard_KeyWasPressed(self, key) == 0) {
                continue;   // held
            }
            if (Keyboard_KeyIsDown(self, key) == 0 && Keyboard_KeyWasReleased(self, key) == 0) {
                continue;
            }
        }
        InputSystem_ReportAction(system, self, action->appData, 0, value, action->context);
    }
    return 0;
}

NiInputKeyboard* __fastcall KeyboardDeletingDestructor(NiInputKeyboard* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiInputKeyboard_vtable);
    Device_Destruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(NiInputKeyboard));
    }
    return self;
}

// vf7: the keys down kept as the last
void __fastcall KeyboardKeepValues(NiInputKeyboard* self, re::Edx) {
    crt::MemcpyS(self->lastKeysDown, 0x20, self->keysDown, 0x20);
}

// vf8 IsControlMapped: as the mouse's, by the keyboard modifiers
uint8_t __fastcall KeyboardIsControlMapped(NiInputKeyboard* self, re::Edx, uint32_t control, uint32_t semantic,
                                           uint8_t useModifiers, uint32_t modifiers, uint32_t, uint32_t, uint32_t) {
    if (self->mappedActions == nullptr) {
        return 0;
    }
    for (NiTListNode<NiAction*>* node = self->mappedActions->head; node != nullptr;) {
        NiAction* action = node->element;
        node = node->next;
        if (action == nullptr || ((action->semantic ^ semantic) & 0xFFFFF000) != 0 || action->control != control) {
            continue;
        }
        uint8_t needs = uint8_t((action->flags >> 9) & 1);
        if (useModifiers != 0) {
            if (needs != 0 && action->keyboardModifiers == modifiers) {
                return 1;
            }
        } else if (needs == 0) {
            return 1;
        }
    }
    return 0;
}

// ---- NiInputDI8Keyboard

// vf5
uint32_t __fastcall DI8KeyboardUnacquire(NiInputDI8Keyboard* self, re::Edx) {
    Com(self->device, kDIUnacquire);
    return kOk;
}

// the key code of each DirectInput key: DIK_ESCAPE..DIK_DECIMAL (1..0x53) the same, the rest
// numbered on from 0x54 (0: none)
void __fastcall DI8KeyboardBuildKeyCodes(NiInputDI8Keyboard* self, re::Edx) {
    static const uint8_t kKeys[][2] = {   // DIK_ code, key code
        {0x56, 0x54}, {0x57, 0x55}, {0x58, 0x56}, {0x64, 0x57}, {0x65, 0x58}, {0x66, 0x59}, {0x70, 0x5A},
        {0x73, 0x5B}, {0x79, 0x5C}, {0x7B, 0x5D}, {0x7D, 0x5E}, {0x7E, 0x5F}, {0x8D, 0x60}, {0x90, 0x61},
        {0x91, 0x62}, {0x92, 0x63}, {0x93, 0x64}, {0x94, 0x65}, {0x95, 0x66}, {0x96, 0x67}, {0x97, 0x68},
        {0x99, 0x69}, {0x9C, 0x6A}, {0x9D, 0x6B}, {0xA0, 0x6C}, {0xA1, 0x6D}, {0xA2, 0x6E}, {0xA4, 0x6F},
        {0xAE, 0x70}, {0xB0, 0x71}, {0xB2, 0x72}, {0xB3, 0x73}, {0xB5, 0x74}, {0xB7, 0x75}, {0xB8, 0x76},
        {0xC5, 0x77}, {0xC7, 0x78}, {0xC8, 0x79}, {0xC9, 0x7A}, {0xCB, 0x7B}, {0xCD, 0x7C}, {0xCF, 0x7D},
        {0xD0, 0x7E}, {0xD1, 0x7F}, {0xD2, 0x80}, {0xD3, 0x81}, {0xDB, 0x82}, {0xDC, 0x83}, {0xDD, 0x84},
        {0xDE, 0x85}, {0xDF, 0x86}, {0xE3, 0x87}, {0xE5, 0x88}, {0xE6, 0x89}, {0xE7, 0x8A}, {0xE8, 0x8B},
        {0xE9, 0x8C}, {0xEA, 0x8D}, {0xEB, 0x8E}, {0xEC, 0x8F}, {0xED, 0x90},
    };
    crt::Memset(self->keyCodes, 0, sizeof(self->keyCodes));
    for (int32_t dik = 1; dik <= 0x53; dik++) {
        self->keyCodes[dik] = dik;
    }
    for (const auto& key : kKeys) {
        self->keyCodes[key[0]] = key[1];
    }
}

void* __fastcall DI8KeyboardGetRTTI(NiInputDI8Keyboard*, re::Edx) {
    return &NiInputDI8Keyboard_RTTI;
}

NiInputDI8Keyboard* __fastcall DI8KeyboardDeletingDestructor(NiInputDI8Keyboard* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiInputDI8Keyboard_vtable);
    if (self->device != nullptr) {
        Com(self->device, kDIUnacquire);
        Com(self->device, kComRelease);
    }
    self->device = nullptr;
    Keyboard_Destruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(NiInputDI8Keyboard));
    }
    return self;
}

// vf3 UpdateDevice: the keys kept as the last (the base's), then the buffered events read 16 at
// a time until none are left: each key's bit and modifier set or cleared (caps lock toggled as
// it goes down). Reading failing: lost (acquired again, vf6): 0x80000006, else 0x80000009
uint32_t __fastcall DI8KeyboardUpdate(NiInputDI8Keyboard* self, re::Edx) {
    self->lastError = kOk;
    Keyboard_UpdateDevice(self);
    if (self->device == nullptr) {
        self->lastError = kErrDeviceLost;
        return kErrDeviceLost;
    }
    ObjectData events[16];
    uint32_t count;
    do {
        count = 16;
        int32_t hr = Com(self->device, kDIGetDeviceData, uint32_t(sizeof(ObjectData)), events, &count, 0u);
        if (hr < 0) {
            if (!IsLost(hr)) {
                self->lastError = kErrNotAcquired;
                return self->lastError;
            }
            Virtual<uint32_t>(self, 6);
            self->lastError = kErrAcquireFailed;
            return self->lastError;
        }
        for (uint32_t i = 0; i < count; i++) {
            int32_t key = self->keyCodes[events[i].offset];
            if (events[i].data & 0x80) {
                if (key == 0 || KeyBit(self->keysDown, key) == 0) {
                    self->keysDown[key >> 3] |= uint8_t(1u << (key & 7));
                    self->modifiers |= Keyboard_KeyModifier(self, key);
                    if (key == 0x3A) {
                        self->capsLock = self->capsLock == 0;
                    }
                }
            } else {
                ReleaseKey(self, key);
            }
        }
    } while (count != 0);
    self->lastError = kOk;
    return kOk;
}

// the keyboard's whole state read (GetDeviceState): caps lock and the modifiers cleared, each
// key pressed or let go as it is. (When reading fails the error is kept and the game goes on
// over its uninitialised buffer, as here.)
void __fastcall DI8KeyboardReadState(NiInputDI8Keyboard* self, re::Edx) {
    uint8_t state[256];
    int32_t hr = Com(self->device, kDIGetDeviceState, 0x100u, state);
    re::NoteInput("NiInput keyboard", hr, state, 0x100);
    if (re::ScriptedKeysActive()) {   // (a test run's keys: runflags.h)
        // (only the run's own keys, as PlayerInput's)
        std::memset(state, 0, sizeof(state));
        hr = 0;
        re::ApplyScriptedKeys(state);
    }
    if (!re::GameInputAllowed()) {   // (another window in front: nothing pressed)
        std::memset(state, 0, sizeof(state));
        hr = 0;
    }
    if (hr < 0) {
        self->lastError = IsLost(hr) ? kErrAcquireFailed : kErrNotAcquired;
    }
    self->capsLock = 0;
    self->modifiers = 0;
    for (uint32_t dik = 0; dik < 0x100; dik++) {
        int32_t key = self->keyCodes[dik];
        if (key == 0) {
            continue;
        }
        if (state[dik] & 0x80) {
            Keyboard_PressKey(self, key);
        } else {
            ReleaseKey(self, key);
        }
    }
}

// the constructor: the key codes built, the device set to the keyboard's format, a buffer of
// 64 events, the cooperative level asked for, acquired and its state read (any step failing:
// 0x80000008; not acquired: 0x80000006)
NiInputDI8Keyboard* __fastcall DI8KeyboardConstruct(NiInputDI8Keyboard* self, re::Edx,
                                                    NiInputDeviceDescription* description, void* device,
                                                    uint32_t cooperation) {
    Keyboard_Construct(self, description);
    self->vtable = reinterpret_cast<void*>(NiInputDI8Keyboard_vtable);
    self->bufferSize = 0x40;
    self->device = device;
    if (device == nullptr) {
        return self;
    }
    DI8Keyboard_BuildKeyCodes(self);
    if (Com(self->device, kDISetDataFormat, re::DIDataFormat(reinterpret_cast<const void*>(c_dfDIKeyboard)).get()) < 0 || SetBufferSize(self->device, self->bufferSize) < 0 ||
        Com(self->device, kDISetCooperativeLevel, gWindow, CooperativeLevel(cooperation)) < 0) {
        self->lastError = 0x80000008;
        return self;
    }
    if (Com(self->device, kDIAcquire) < 0) {
        self->lastError = kErrAcquireFailed;
        return self;
    }
    DI8Keyboard_ReadState(self);
    return self;
}

// vf6: acquired, its state read (lost: 0x80000006; other failures 0x80000009)
uint32_t __fastcall DI8KeyboardAcquire(NiInputDI8Keyboard* self, re::Edx) {
    int32_t hr = Com(self->device, kDIAcquire);
    if (hr >= 0) {
        DI8Keyboard_ReadState(self);
        self->lastError = kOk;
        return self->lastError;
    }
    self->lastError = IsLost(hr) ? kErrAcquireFailed : kErrNotAcquired;
    return self->lastError;
}

} // namespace
} // namespace ni::input

// ---- registration ---------------------------------------------------------------------

#define NIINPUTMK_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIINPUTMK_REVERSED("NiInputDI8Mouse::Unacquire", 0x008D42D0, &ni::input::DI8MouseUnacquire);
NIINPUTMK_REVERSED("NiInputDI8Mouse::Acquire", 0x008D42F0, &ni::input::DI8MouseAcquire);
NIINPUTMK_REVERSED("NiInputDI8Mouse::GetRTTI", 0x008D4340, &ni::input::DI8MouseGetRTTI);
NIINPUTMK_REVERSED("NiInputDI8Mouse::NiInputDI8Mouse", 0x008D4350, &ni::input::DI8MouseConstruct);
NIINPUTMK_REVERSED("NiInputDI8Mouse::DeletingDestructor", 0x008D4450, &ni::input::DI8MouseDeletingDestructor);
NIINPUTMK_REVERSED("NiInputDI8Mouse::PressButton", 0x008D44B0, &ni::input::DI8MousePressButton);
NIINPUTMK_REVERSED("NiInputDI8Mouse::HandleButton", 0x008D4500, &ni::input::DI8MouseHandleButton);
NIINPUTMK_REVERSED("NiInputDI8Mouse::UpdateDevice", 0x008D4560, &ni::input::DI8MouseUpdate);
NIINPUTMK_REVERSED("NiInputDI8Keyboard::Unacquire", 0x008D4970, &ni::input::DI8KeyboardUnacquire);
NIINPUTMK_REVERSED("NiInputDI8Keyboard::BuildKeyCodes", 0x008D4980, &ni::input::DI8KeyboardBuildKeyCodes);
NIINPUTMK_REVERSED("NiInputKeyboard::KeyIsDown", 0x008D4F30, &ni::input::KeyIsDown);
NIINPUTMK_REVERSED("NiInputKeyboard::KeyModifier", 0x008D4F60, &ni::input::KeyModifier);
NIINPUTMK_REVERSED("NiInputDI8Keyboard::GetRTTI", 0x008D5060, &ni::input::DI8KeyboardGetRTTI);
NIINPUTMK_REVERSED("NiInputDI8Keyboard::DeletingDestructor", 0x008D5070,
                   &ni::input::DI8KeyboardDeletingDestructor);
NIINPUTMK_REVERSED("NiInputKeyboard::PressKey", 0x008D50C0, &ni::input::PressKey);
NIINPUTMK_REVERSED("NiInputDI8Keyboard::UpdateDevice", 0x008D5120, &ni::input::DI8KeyboardUpdate);
NIINPUTMK_REVERSED("NiInputDI8Keyboard::ReadState", 0x008D52A0, &ni::input::DI8KeyboardReadState);
NIINPUTMK_REVERSED("NiInputDI8Keyboard::NiInputDI8Keyboard", 0x008D5390, &ni::input::DI8KeyboardConstruct);
NIINPUTMK_REVERSED("NiInputDI8Keyboard::Acquire", 0x008D54A0, &ni::input::DI8KeyboardAcquire);
NIINPUTMK_REVERSED("NiInputMouse::NiInputMouse", 0x008D5F20, &ni::input::MouseConstruct);
NIINPUTMK_REVERSED("NiInputMouse::GetRTTI", 0x008D60F0, &ni::input::MouseGetRTTI);
NIINPUTMK_REVERSED("NiInputMouse::~NiInputMouse", 0x008D6100, &ni::input::MouseDestruct);
NIINPUTMK_REVERSED("NiInputMouse::UpdateActionMap", 0x008D6110, &ni::input::MouseUpdateActionMap);
NIINPUTMK_REVERSED("NiInputMouse::KeepValues", 0x008D6170, &ni::input::MouseKeepValues);
NIINPUTMK_REVERSED("NiInputMouse::IsControlMapped", 0x008D62D0, &ni::input::MouseIsControlMapped);
NIINPUTMK_REVERSED("NiInputMouse::ReportButtonActions", 0x008D6340, &ni::input::MouseReportButtonActions);
NIINPUTMK_REVERSED("NiInputMouse::ReportAxisActions", 0x008D6480, &ni::input::MouseReportAxisActions);
NIINPUTMK_REVERSED("NiInputMouse::DeletingDestructor", 0x008D6600, &ni::input::MouseDeletingDestructor);
NIINPUTMK_REVERSED("NiInputKeyboard::NiInputKeyboard", 0x008D6630, &ni::input::KeyboardConstruct);
NIINPUTMK_REVERSED("NiInputKeyboard::~NiInputKeyboard", 0x008D6690, &ni::input::KeyboardDestruct);
NIINPUTMK_REVERSED("NiInputKeyboard::UpdateDevice", 0x008D66A0, &ni::input::KeyboardUpdateDevice);
NIINPUTMK_REVERSED("NiInputKeyboard::GetRTTI", 0x008D66B0, &ni::input::KeyboardGetRTTI);
NIINPUTMK_REVERSED("NiInputKeyboard::KeyWasReleased", 0x008D66C0, &ni::input::KeyWasReleased);
NIINPUTMK_REVERSED("NiInputKeyboard::DeletingDestructor", 0x008D6700, &ni::input::KeyboardDeletingDestructor);
NIINPUTMK_REVERSED("NiInputKeyboard::UpdateActionMap", 0x008D6730, &ni::input::KeyboardUpdateActionMap);
NIINPUTMK_REVERSED("NiInputKeyboard::KeepValues", 0x008D6890, &ni::input::KeyboardKeepValues);
NIINPUTMK_REVERSED("NiInputKeyboard::IsControlMapped", 0x008D68B0, &ni::input::KeyboardIsControlMapped);
