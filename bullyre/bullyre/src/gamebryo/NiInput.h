#pragma once

// Gamebryo's NiInput as Bully's exe has it: NiInputSystem (NiDI8InputSystem over DirectInput 8),
// its devices (NiInputDevice, the mouse, keyboard and game pads), the action map and the pool
// of action data reported to the game. The game's start-up creates the input system through
// its application object (0x832A20's virtual call), so the library runs although the game
// reads DirectInput itself. Layouts read from the constructors (NiInputSystem 0x8D35D0,
// NiInputDevice 0x8D4080, NiInputDI8GamePad 0x8D5B70) and checked with static_asserts; what
// isn't read by the code written so far is left as padding.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni::input {

// NiInputErr: the codes the library keeps as its last error and returns
enum : uint32_t {
    kOk = 0,
    kErrInvalidParams = 0x80000002,
    kErrDeviceLost = 0x80000004,        // acquiring or polling failed
    kErrAcquireFailed = 0x80000006,     // another application has the device
    kErrNotAcquired = 0x80000009,
    kErrDeviceBusy = 0x8000000A,        // acquire refused (status 2)
    kErrAllocFailed = 0x8000000D,
    kErrNoActionMap = 0x8000000F,
    kErrGamePadMapping = 0x80000011,
    kErrKeyboardMapping = 0x80000012,
    kErrMouseMapping = 0x80000013,
    kErrActionMapped = 0x80000014,      // the keyboard already has the control mapped
};

// NiActionData: what's reported for a mapped action (0x1C, kept in a pool: 0x8D2EB0 makes
// arrays of them, constructed by 0x8D2CA0)
struct ActionData {
    re::Ptr32<void> vtable;          // +0x0
    uint32_t appData;      // +0x4
    uint32_t deviceID;     // +0x8 (the device's slot << 16 | port; -1 constructed)
    uint32_t data;         // +0xC
    uint32_t context;      // +0x10
    uint32_t field14;      // +0x14
    uint32_t sequence;     // +0x18 (given, or the next of the global count 0x20E976C)
};
static_assert(sizeof(ActionData) == 0x1C);

// NiTObjectPool<ActionData> (0x18): the free elements (a NiTPrimitiveSet: its array, allocated,
// used), the blocks it has made, how many a new block holds and how many the first did
struct ActionDataPool {
    re::Ptr32<re::Ptr32<ActionData>> freeData;   // +0x0
    uint32_t freeAlloced;    // +0x4
    uint32_t freeCount;      // +0x8
    re::Ptr32<struct Block> blocks;    // +0xC
    uint32_t growBy;         // +0x10 (8, doubled each block)
    uint32_t initSize;       // +0x14 (8)
};
static_assert(sizeof(ActionDataPool) == 0x18);
// a block of the pool (0xC): its elements, how many, the next block
struct Block {
    re::Ptr32<ActionData> data;   // +0x0
    uint32_t count;     // +0x4
    re::Ptr32<Block> next;        // +0x8
};
static_assert(sizeof(Block) == 0xC);

// NiAction (as the mapping code reads it). The semantic's kind: 0x10000000 game pad,
// 0x20000000 keyboard (a key, or the modifier ones 0x2001xxxx), 0x40000000 mouse (the axes
// 0x40000001..3, the buttons 0x40001000..7). (The first pass had the keyboard and mouse the
// other way round; the DI8 mouse's and keyboard's code, chunk 45c, settles it.) The three
// modifier fields are what the action needs held on each kind of device.
struct NiAction {
    uint8_t pad0[0xC];
    uint32_t appData;    // +0xC (reported with it)
    uint32_t semantic;   // +0x10
    uint32_t flags;      // +0x14 (0x200 modifiers must match, 0x400 one device only, bit 9 up
                         //  passed on as the "in range" flag)
    uint32_t deviceID;   // +0x18 (with 0x400)
    int32_t rangeLow;    // +0x1C (with 0x40: reported entering or leaving the range)
    int32_t rangeHigh;   // +0x20
    uint32_t context;    // +0x24
    uint32_t keyboardModifiers;   // +0x28
    uint32_t mouseModifiers;      // +0x2C
    uint32_t modifiers;  // +0x30 (the game pad's)
    uint32_t control;    // +0x34 (the device's control the semantic maps to)
    uint32_t status;     // +0x38 (2 mapped, 4 failed, 6 not applicable)
};
static_assert(offsetof(NiAction, keyboardModifiers) == 0x28 && offsetof(NiAction, mouseModifiers) == 0x2C);
static_assert(offsetof(NiAction, control) == 0x34);

// NiActionMap: its actions in a NiTPrimitiveArray at +0x14
struct NiActionMap : NiRefObject {
    uint8_t pad8[0x14 - 0x8];
    NiTArray<NiAction*> actions;   // +0x14 (base +0x18, size +0x1E, esize +0x20)
};
static_assert(offsetof(NiActionMap, actions) == 0x14);

// NiInputDevice::ControlDesc (0x14, constructed by 0x8D1E90): one of a device's controls as
// DirectInput enumerated it: its index in the description's list, its name (NiMalloc'd), its
// kind (0 a button or key, 1 an axis, 2 a point of view), flags (0x10 | 0x1 buttons and
// points of view, 0x10 | 0x2 axes) and DirectInput's object type
struct ControlDesc {
    uint32_t index;    // +0x0
    re::Ptr32<char> name;        // +0x4
    uint32_t type;     // +0x8
    uint32_t flags;    // +0xC
    uint32_t diType;   // +0x10 (DIDEVICEOBJECTINSTANCE's dwType)
};
static_assert(sizeof(ControlDesc) == 0x14);

// NiInputDevice::Description (vtable 0x9A1CC4, 0x40; NiDI8InputSystem's, vtable 0x9A1CCC,
// 0x50, adds the DirectInput instance GUID): the kind (1 keyboard, 2 mouse, 3 game pad), the
// port and slot, the product and instance names, whether a device object has it open, and
// its controls' descriptions and how many of each kind
struct NiInputDeviceDescription : NiRefObject {
    uint32_t type;                  // +0x8
    uint32_t port;                  // +0xC
    uint32_t slot;                  // +0x10
    re::Ptr32<char> productName;              // +0x14
    re::Ptr32<char> instanceName;             // +0x18
    uint8_t opened;                 // +0x1C
    NiTPointerList<void*> controls; // +0x20 (vtable 0x95145C; ControlDesc*)
    re::Ptr32<void> controlsIter;             // +0x30
    uint32_t buttonCount;           // +0x34 (keys too)
    uint32_t axisCount;             // +0x38
    uint32_t povCount;              // +0x3C
};
static_assert(offsetof(NiInputDeviceDescription, buttonCount) == 0x34);
static_assert(sizeof(NiInputDeviceDescription) == 0x40);
struct NiDI8InputDeviceDescription : NiInputDeviceDescription {
    uint32_t guidInstance[4];       // +0x40
};
static_assert(sizeof(NiDI8InputDeviceDescription) == 0x50);

// NiInputDevice (vtable 0x9A1DB8, 0x20): slot 9 AddMappedAction
struct NiInputDevice : NiRefObject {
    re::Ptr32<NiInputDeviceDescription> description;   // +0x8 (a reference)
    int32_t status;                          // +0xC (-1 until updated; 0 ready, 2 lost)
    uint32_t modifiers;                      // +0x10
    uint8_t field14;                         // +0x14
    re::Ptr32<NiTPointerList<NiAction*>> mappedActions;   // +0x18 (made on the first add)
    uint32_t lastError;                      // +0x1C
};
static_assert(sizeof(NiInputDevice) == 0x20);

// a game pad's button (0x10, constructed by 0x8D08B0): its value and whether it's pressed,
// now and before, and the device's modifiers when it was
struct GamePadButton {
    uint8_t value;           // +0x0
    uint8_t pressed;         // +0x1
    uint8_t lastValue;       // +0x2
    uint8_t lastPressed;     // +0x3
    uint8_t field4;          // +0x4
    uint32_t modifiers;      // +0x8
    uint32_t lastModifiers;  // +0xC
};
static_assert(sizeof(GamePadButton) == 0x10);

// a game pad's axis (0x20, constructed by 0x8D08D0: raw range 0..255, mapped -1..1): the
// raw and mapped values now and before, the raw range the device reports and the range it's
// mapped to
struct GamePadAxis {
    int32_t raw;         // +0x0
    int32_t value;       // +0x4
    int32_t lastRaw;     // +0x8
    int32_t lastValue;   // +0xC
    int32_t rawLow;      // +0x10
    int32_t rawHigh;     // +0x14
    int32_t low;         // +0x18
    int32_t high;        // +0x1C
};
static_assert(sizeof(GamePadAxis) == 0x20);

// NiInputGamePad (vtable 0x9A1B10, 0x44; its constructor 0x8D0900, destructor 0x8D0A00): 16
// buttons, its axes (0xBCBB20 of them: 4 sticks' two each), which buttons are held and which
// went down this update, which axes are inverted
struct NiInputGamePad : NiInputDevice {
    uint32_t index;           // +0x20 (its slot in the input system's pads)
    uint32_t held;            // +0x24
    uint32_t pressedNow;      // +0x28 (cleared each update)
    uint32_t buttonCount;     // +0x2C (16)
    re::Ptr32<GamePadButton> buttons;   // +0x30 (new[])
    uint32_t axisCount;       // +0x34
    re::Ptr32<GamePadAxis> axes;        // +0x38 (new[] with its count before it)
    re::Ptr32<void> field3C;            // +0x3C (NiFree'd)
    uint32_t invertedAxes;    // +0x40
};
static_assert(sizeof(NiInputGamePad) == 0x44);

// NiInputDI8GamePad (vtable 0x9A1E70, 0x160): the DirectInput device, which buttons and axes
// the last update's events changed
struct NiInputDI8GamePad : NiInputGamePad {
    re::Ptr32<void> device;              // +0x44 (IDirectInputDevice8A)
    uint8_t pad48[0x158 - 0x48];
    uint32_t buttonsChanged;   // +0x158
    uint32_t axesChanged;      // +0x15C
};
static_assert(sizeof(NiInputDI8GamePad) == 0x160);

// NiInputMouse (vtable 0x9A1ECC, 0x100; constructor 0x8D5F20, destructor 0x8D6100): its
// three axes (x, y and the wheel: the movement of the last update in value) and eight
// buttons, the same structs as the game pad's (constructed inline)
struct NiInputMouse : NiInputDevice {
    GamePadAxis axes[3];        // +0x20
    GamePadButton buttons[8];   // +0x80
};
static_assert(offsetof(NiInputMouse, buttons) == 0x80);
static_assert(sizeof(NiInputMouse) == 0x100);

// NiInputDI8Mouse (vtable 0x9A1DF0, 0x10C; constructor 0x8D4350): the DirectInput device
// (buffered: 64 events), which buttons the last update's events changed
struct NiInputDI8Mouse : NiInputMouse {
    re::Ptr32<void> device;              // +0x100 (IDirectInputDevice8A)
    uint32_t bufferSize;       // +0x104 (64)
    uint32_t buttonsChanged;   // +0x108
};
static_assert(sizeof(NiInputDI8Mouse) == 0x10C);

// NiInputKeyboard (vtable 0x9A1F0C, 0x68; constructor 0x8D6630, destructor 0x8D6690): caps
// lock's state, which keys are down now and were at the last update (a bit per
// NiInputKeyboard key code, 1..0x90: not DirectInput's). Its modifiers (+0x10): 0x1 left
// control, 0x2 right control, 0x4 left alt, 0x8 right alt, 0x10 left windows, 0x20 right
// windows, 0x40 left shift, 0x80 right shift, 0x100 caps lock
struct NiInputKeyboard : NiInputDevice {
    uint8_t capsLock;          // +0x20 (toggled by its key going down)
    uint8_t keysDown[32];      // +0x21
    uint8_t lastKeysDown[32];  // +0x41
    uint8_t pad61[3];
    uint32_t field64;          // +0x64
};
static_assert(offsetof(NiInputKeyboard, lastKeysDown) == 0x41 && offsetof(NiInputKeyboard, field64) == 0x64);
static_assert(sizeof(NiInputKeyboard) == 0x68);

// NiInputDI8Keyboard (vtable 0x9A1E30, 0x470; constructor 0x8D5390): the DirectInput device
// (buffered: 64 events) and the key code of each DirectInput key (0x8D4980 fills it; 0: none)
struct NiInputDI8Keyboard : NiInputKeyboard {
    uint32_t bufferSize;       // +0x68 (64)
    re::Ptr32<void> device;              // +0x6C (IDirectInputDevice8A)
    int32_t keyCodes[256];     // +0x70 (by DIK_ code)
};
static_assert(offsetof(NiInputDI8Keyboard, keyCodes) == 0x70);
static_assert(sizeof(NiInputDI8Keyboard) == 0x470);

// NiInputSystem (vtable 0x9A1D10, 0x84; NiDI8InputSystem 0x9A1B78 builds on it)
struct NiInputSystem : NiRefObject {
    uint32_t lastError;                                // +0x8
    re::Ptr32<NiRefObject> renderer;                             // +0xC (a reference)
    uint32_t mode;                                     // +0x10 (1 mapped: an action map is set)
    uint32_t createParams[3];                          // +0x14..+0x1F (from the create parameters)
    int32_t axisLow;                                   // +0x20 (-100)
    int32_t axisHigh;                                  // +0x24 (100)
    uint8_t field28;                                   // +0x28 (1 once the devices are enumerated)
    NiTPointerList<NiInputDeviceDescription*> devices; // +0x2C (vtable 0x95127C)
    re::Ptr32<void> devicesIter;                                 // +0x3C (GetFirst/NextDeviceDesc's)
    re::Ptr32<NiInputDevice> keyboardDevice;                     // +0x40 (OpenKeyboard 0x8D2130's; references,
                                                       //  as are the next)
    re::Ptr32<NiInputDevice> mouseDevice;                        // +0x44 (OpenMouse 0x8D2030's)
    re::Ptr32<NiInputGamePad> gamePads[8];                       // +0x48
    re::Ptr32<NiActionMap> actionMap;                            // +0x68
    re::Ptr32<void> actionDataIter;                              // +0x6C
    NiTPointerList<ActionData*> actionData;            // +0x70 (vtable 0x952E10: reported this frame)
    re::Ptr32<ActionDataPool> actionDataPool;                    // +0x80
};
static_assert(sizeof(NiInputSystem) == 0x84);
static_assert(offsetof(NiInputSystem, keyboardDevice) == 0x40 && offsetof(NiInputSystem, mouseDevice) == 0x44);
static_assert(offsetof(NiInputSystem, gamePads) == 0x48 && offsetof(NiInputSystem, actionData) == 0x70);

// NiDI8InputSystem (vtable 0x9A1B78, 0x234): DirectInput 8, the game pad actions it maps
// (DIACTIONA, 0x28 each) and the action format they're enumerated by, the names the create
// parameters give the actions (by application data)
struct NiDI8InputSystem : NiInputSystem {
    uint32_t field84;               // +0x84
    re::Ptr32<void> directInput;              // +0x88 (IDirectInput8A)
    uint32_t actionCount;           // +0x8C (21)
    re::Ptr32<uint8_t> actions;               // +0x90 (the game's operator new)
    uint8_t actionFormat[0x148];    // +0x94 (DIACTIONFORMATA)
    uint32_t field1DC;              // +0x1DC
    re::Ptr32<char> actionNames[21];          // +0x1E0
};
static_assert(sizeof(NiDI8InputSystem) == 0x234);
static_assert(offsetof(NiDI8InputSystem, actionFormat) == 0x94);

} // namespace ni::input
